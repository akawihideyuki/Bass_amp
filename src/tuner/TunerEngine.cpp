#include "TunerEngine.h"

#include <cmath>

TunerEngine::TunerEngine() = default;

void TunerEngine::setSampleRate(double newSampleRate) noexcept
{
    if (newSampleRate > 1000.0)
        sampleRate.store(newSampleRate);
}

void TunerEngine::setReferenceA4(double frequency) noexcept
{
    referenceA4.store(juce::jlimit(430.0, 450.0, frequency));
}

void TunerEngine::pushSamples(const float* samples, int numSamples) noexcept
{
    if (samples == nullptr || numSamples <= 0)
        return;

    int start1 = 0;
    int size1 = 0;
    int start2 = 0;
    int size2 = 0;
    fifo.prepareToWrite(numSamples, start1, size1, start2, size2);

    if (size1 > 0)
        juce::FloatVectorOperations::copy(fifoBuffer.data() + start1, samples, size1);

    if (size2 > 0)
        juce::FloatVectorOperations::copy(fifoBuffer.data() + start2, samples + size1, size2);

    fifo.finishedWrite(size1 + size2);
}

TunerEngine::Display TunerEngine::analyse()
{
    Display display;

    for (;;)
    {
        const auto available = fifo.getNumReady();
        if (available <= 0)
            break;

        int start1 = 0;
        int size1 = 0;
        int start2 = 0;
        int size2 = 0;
        fifo.prepareToRead(juce::jmin(available, 4096), start1, size1, start2, size2);

        const auto consumeRange = [this](const float* input, int count)
        {
            for (int i = 0; i < count; ++i)
            {
                sourceWindow[static_cast<size_t>(sourceWritePosition)] = input[i];
                sourceWritePosition = (sourceWritePosition + 1) % sourceWindowSize;
                sourceSamplesAvailable = juce::jmin(sourceWindowSize, sourceSamplesAvailable + 1);
            }
        };

        if (size1 > 0)
            consumeRange(fifoBuffer.data() + start1, size1);

        if (size2 > 0)
            consumeRange(fifoBuffer.data() + start2, size2);

        fifo.finishedRead(size1 + size2);
    }

    if (sourceSamplesAvailable < sourceWindowSize)
        return lastDisplay;

    const auto nowMs = juce::Time::getMillisecondCounterHiRes();
    if (nowMs - lastAnalysisMs < 150.0)
        return lastDisplay;

    lastAnalysisMs = nowMs;

    const auto oldest = sourceWritePosition;
    for (int i = 0; i < analysisSize; ++i)
    {
        const auto sourceIndex = (oldest + i * downsampleFactor) % sourceWindowSize;
        analysisBuffer[static_cast<size_t>(i)] = sourceWindow[static_cast<size_t>(sourceIndex)];
    }

    const auto effectiveSampleRate = sampleRate.load() / static_cast<double>(downsampleFactor);
    const auto result = detector.detect(analysisBuffer.data(), analysisSize, effectiveSampleRate);

    if (!result.isValid() || result.confidence < 0.58f)
    {
        lastDisplay = {};
        return lastDisplay;
    }

    const auto reference = referenceA4.load();
    const auto note = 69.0 + 12.0 * std::log2(result.frequencyHz / reference);
    const auto nearest = static_cast<int>(std::lround(note));
    const auto cents = (note - static_cast<double>(nearest)) * 100.0;

    static constexpr const char* noteNames[] {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };

    const auto pitchClass = ((nearest % 12) + 12) % 12;
    const auto octave = nearest / 12 - 1;

    display.valid = true;
    display.noteName = juce::String(noteNames[pitchClass]) + juce::String(octave);
    display.frequencyHz = result.frequencyHz;
    display.cents = cents;
    display.confidence = result.confidence;
    lastDisplay = display;
    return lastDisplay;
}
