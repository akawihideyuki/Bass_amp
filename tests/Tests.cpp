#include <JuceHeader.h>

#include "dsp/BassAmpProcessor.h"
#include "preset/PresetManager.h"
#include "tuner/PitchDetector.h"

#include <cmath>
#include <iostream>

namespace
{
int failures = 0;

void expect(bool condition, const char* message)
{
    if (!condition)
    {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void fillSine(juce::AudioBuffer<float>& buffer, double sampleRate, double frequency, float amplitude = 0.25f)
{
    buffer.clear();
    auto* samples = buffer.getWritePointer(0);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto phase = juce::MathConstants<double>::twoPi
            * frequency * static_cast<double>(i) / sampleRate;
        samples[i] = amplitude * static_cast<float>(std::sin(phase));
    }
}

bool isFiniteBuffer(const juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (!std::isfinite(samples[i]))
                return false;
        }
    }

    return true;
}

void testDsp()
{
    for (const auto sampleRate : { 44100.0, 48000.0, 96000.0 })
    {
        for (const auto blockSize : { 64, 128, 256, 512 })
        {
            BassAmpProcessor processor;
            processor.prepare(sampleRate, blockSize, 2);

            for (const auto mode : {
                     BassAmpProcessor::OversamplingMode::off,
                     BassAmpProcessor::OversamplingMode::x2,
                     BassAmpProcessor::OversamplingMode::x4 })
            {
                processor.setOversamplingMode(mode);

                juce::AudioBuffer<float> buffer(2, blockSize);
                fillSine(buffer, sampleRate, 55.0);
                processor.processBlock(buffer, 0, blockSize);

                expect(isFiniteBuffer(buffer), "DSP produced NaN/Infinity");
                expect(buffer.getMagnitude(0, 0, blockSize) < 2.0f,
                       "DSP output exceeded safety range");
            }
        }
    }

    BassAmpProcessor extreme;
    extreme.prepare(48000.0, 512, 2);
    extreme.setInputGainDb(24.0f);
    extreme.setDrive(100.0f);
    extreme.setCharacter(100.0f);
    extreme.setLowClean(100.0f);
    extreme.setBassDb(12.0f);
    extreme.setLowMidDb(12.0f);
    extreme.setHighMidDb(12.0f);
    extreme.setTrebleDb(12.0f);
    extreme.setMasterDb(6.0f);
    extreme.setOversamplingMode(BassAmpProcessor::OversamplingMode::x4);

    juce::AudioBuffer<float> hot(2, 512);
    fillSine(hot, 48000.0, 41.2034, 1.0f);
    extreme.processBlock(hot, 0, hot.getNumSamples());

    expect(isFiniteBuffer(hot), "Extreme settings produced NaN/Infinity");
    expect(hot.getMagnitude(0, 0, hot.getNumSamples()) <= 1.25f,
           "Limiter failed to contain extreme output");

    BassAmpProcessor bypass;
    bypass.prepare(48000.0, 128, 2);
    bypass.setBypassed(true);

    juce::AudioBuffer<float> dry(2, 128);
    fillSine(dry, 48000.0, 82.4069, 0.2f);
    juce::AudioBuffer<float> reference;
    reference.makeCopyOf(dry);

    bypass.processBlock(dry, 0, dry.getNumSamples());

    expect(std::abs(dry.getSample(0, 20) - reference.getSample(0, 20)) < 1.0e-6f,
           "Bypass changed channel 0");
    expect(std::abs(dry.getSample(1, 20) - reference.getSample(0, 20)) < 1.0e-6f,
           "Bypass did not mirror mono input to stereo output");
}

void testPitchDetector()
{
    PitchDetector detector;
    constexpr int sampleCount = 8192;
    juce::AudioBuffer<float> buffer(1, sampleCount);

    fillSine(buffer, 48000.0, 41.2034, 0.4f);
    auto result = detector.detect(buffer.getReadPointer(0), sampleCount, 48000.0);
    expect(result.isValid(), "Pitch detector failed on E1");
    expect(std::abs(result.frequencyHz - 41.2034) < 0.7, "Pitch detector E1 frequency inaccurate");

    fillSine(buffer, 48000.0, 55.0, 0.4f);
    result = detector.detect(buffer.getReadPointer(0), sampleCount, 48000.0);
    expect(result.isValid(), "Pitch detector failed on A1");
    expect(std::abs(result.frequencyHz - 55.0) < 0.7, "Pitch detector A1 frequency inaccurate");
}

void testPresetRoundTrip()
{
    PresetState source;
    source.drive = 73.0f;
    source.character = 64.0f;
    source.lowClean = 81.0f;
    source.bassDb = 2.5f;
    source.voicing = 2;
    source.cabinet = 3;
    source.oversampling = 2;
    source.tunerReferenceA4 = 442.0f;
    source.userIrPath = "C:\\IR\\bass.wav";

    const auto json = PresetManager::toJson(source);

    PresetState restored;
    juce::String error;
    expect(PresetManager::fromJson(json, restored, error), "Preset JSON round-trip failed");
    expect(std::abs(restored.drive - source.drive) < 0.001f, "Preset drive mismatch");
    expect(restored.voicing == source.voicing, "Preset voicing mismatch");
    expect(restored.cabinet == source.cabinet, "Preset cabinet mismatch");
    expect(restored.oversampling == source.oversampling, "Preset oversampling mismatch");
    expect(std::abs(restored.tunerReferenceA4 - source.tunerReferenceA4) < 0.001f,
           "Preset tuner reference mismatch");

    PresetState invalid;
    expect(!PresetManager::fromJson("{ broken json", invalid, error),
           "Invalid preset JSON was accepted");
}
}

int main()
{
    testDsp();
    testPitchDetector();
    testPresetRoundTrip();

    if (failures == 0)
    {
        std::cout << "Bass_amp tests passed\n";
        return 0;
    }

    std::cerr << failures << " test(s) failed\n";
    return 1;
}
