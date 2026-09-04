#include "BassAmpProcessor.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double pi = juce::MathConstants<double>::pi;

float clampPercent(float value) noexcept
{
    return juce::jlimit(0.0f, 100.0f, value) * 0.01f;
}
}

float BassAmpProcessor::Biquad::process(float x) noexcept
{
    const auto y = b0 * static_cast<double>(x) + z1;
    z1 = b1 * static_cast<double>(x) - a1 * y + z2;
    z2 = b2 * static_cast<double>(x) - a2 * y;
    return static_cast<float>(y);
}

void BassAmpProcessor::Biquad::setNormalized(double nb0, double nb1, double nb2,
                                              double na0, double na1, double na2) noexcept
{
    const auto invA0 = 1.0 / na0;
    b0 = nb0 * invA0;
    b1 = nb1 * invA0;
    b2 = nb2 * invA0;
    a1 = na1 * invA0;
    a2 = na2 * invA0;
}

void BassAmpProcessor::Biquad::setLowPass(double sr, double frequency, double q) noexcept
{
    const auto w0 = 2.0 * pi * frequency / sr;
    const auto c = std::cos(w0);
    const auto s = std::sin(w0);
    const auto alpha = s / (2.0 * q);
    setNormalized((1.0 - c) * 0.5, 1.0 - c, (1.0 - c) * 0.5,
                  1.0 + alpha, -2.0 * c, 1.0 - alpha);
}

void BassAmpProcessor::Biquad::setHighPass(double sr, double frequency, double q) noexcept
{
    const auto w0 = 2.0 * pi * frequency / sr;
    const auto c = std::cos(w0);
    const auto s = std::sin(w0);
    const auto alpha = s / (2.0 * q);
    setNormalized((1.0 + c) * 0.5, -(1.0 + c), (1.0 + c) * 0.5,
                  1.0 + alpha, -2.0 * c, 1.0 - alpha);
}

void BassAmpProcessor::Biquad::setPeak(double sr, double frequency, double q, double gainDb) noexcept
{
    const auto a = std::pow(10.0, gainDb / 40.0);
    const auto w0 = 2.0 * pi * frequency / sr;
    const auto c = std::cos(w0);
    const auto s = std::sin(w0);
    const auto alpha = s / (2.0 * q);
    setNormalized(1.0 + alpha * a, -2.0 * c, 1.0 - alpha * a,
                  1.0 + alpha / a, -2.0 * c, 1.0 - alpha / a);
}

void BassAmpProcessor::Biquad::setLowShelf(double sr, double frequency, double gainDb) noexcept
{
    const auto a = std::pow(10.0, gainDb / 40.0);
    const auto w0 = 2.0 * pi * frequency / sr;
    const auto c = std::cos(w0);
    const auto s = std::sin(w0);
    const auto alpha = s * std::sqrt(2.0) * 0.5;
    const auto beta = 2.0 * std::sqrt(a) * alpha;

    setNormalized(a * ((a + 1.0) - (a - 1.0) * c + beta),
                  2.0 * a * ((a - 1.0) - (a + 1.0) * c),
                  a * ((a + 1.0) - (a - 1.0) * c - beta),
                  (a + 1.0) + (a - 1.0) * c + beta,
                  -2.0 * ((a - 1.0) + (a + 1.0) * c),
                  (a + 1.0) + (a - 1.0) * c - beta);
}

void BassAmpProcessor::Biquad::setHighShelf(double sr, double frequency, double gainDb) noexcept
{
    const auto a = std::pow(10.0, gainDb / 40.0);
    const auto w0 = 2.0 * pi * frequency / sr;
    const auto c = std::cos(w0);
    const auto s = std::sin(w0);
    const auto alpha = s * std::sqrt(2.0) * 0.5;
    const auto beta = 2.0 * std::sqrt(a) * alpha;

    setNormalized(a * ((a + 1.0) + (a - 1.0) * c + beta),
                  -2.0 * a * ((a - 1.0) + (a + 1.0) * c),
                  a * ((a + 1.0) + (a - 1.0) * c - beta),
                  (a + 1.0) - (a - 1.0) * c + beta,
                  2.0 * ((a - 1.0) - (a + 1.0) * c),
                  (a + 1.0) - (a - 1.0) * c - beta);
}

void BassAmpProcessor::prepare(double sampleRate, int maximumBlockSize, int outputChannels)
{
    currentSampleRate = sampleRate > 1000.0 ? sampleRate : 48000.0;
    preparedOutputChannels = juce::jmax(1, outputChannels);
    maximumPreparedBlockSize = juce::jmax(4096, maximumBlockSize);

    const juce::dsp::ProcessSpec monoSpec {
        currentSampleRate,
        static_cast<juce::uint32>(maximumPreparedBlockSize),
        1
    };

    noiseGate.prepare(monoSpec);
    compressor.prepare(monoSpec);
    crossover.prepare(monoSpec);
    crossover.setCutoffFrequency(120.0f);
    convolution.prepare(monoSpec);
    cleanDelay.prepare(monoSpec);

    const juce::dsp::ProcessSpec outputSpec {
        currentSampleRate,
        static_cast<juce::uint32>(maximumPreparedBlockSize),
        static_cast<juce::uint32>(preparedOutputChannels)
    };

    limiter.prepare(outputSpec);
    limiter.setThreshold(-1.0f);
    limiter.setRelease(60.0f);

    oversampling2x.initProcessing(static_cast<size_t>(maximumPreparedBlockSize));
    oversampling4x.initProcessing(static_cast<size_t>(maximumPreparedBlockSize));

    ampWork.setSize(2, maximumPreparedBlockSize, false, true, false);
    cleanLowValues.assign(static_cast<size_t>(maximumPreparedBlockSize), 0.0f);
    driveValues.assign(static_cast<size_t>(maximumPreparedBlockSize), 0.0f);
    characterValues.assign(static_cast<size_t>(maximumPreparedBlockSize), 0.0f);
    cleanMixValues.assign(static_cast<size_t>(maximumPreparedBlockSize), 0.0f);

    inputGainLinear.reset(currentSampleRate, 0.02);
    driveSmoothed.reset(currentSampleRate, 0.02);
    characterSmoothed.reset(currentSampleRate, 0.02);
    lowCleanSmoothed.reset(currentSampleRate, 0.02);
    masterGainLinear.reset(currentSampleRate, 0.02);
    outputFade.reset(currentSampleRate, 0.025);

    inputGainLinear.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inputGainDb.load()));
    driveSmoothed.setCurrentAndTargetValue(clampPercent(drive.load()));
    characterSmoothed.setCurrentAndTargetValue(clampPercent(character.load()));
    lowCleanSmoothed.setCurrentAndTargetValue(clampPercent(lowClean.load()));
    masterGainLinear.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(masterDb.load()));
    outputFade.setCurrentAndTargetValue(0.0f);
    outputFade.setTargetValue(1.0f);

    activeCabinet = -1;
    lastBassDb = lastLowMidDb = lastHighMidDb = lastTrebleDb = 999.0f;
    updateToneFilters();
    updateCabinetFilters();
    reset();
}

void BassAmpProcessor::reset()
{
    noiseGate.reset();
    compressor.reset();
    crossover.reset();
    limiter.reset();
    oversampling2x.reset();
    oversampling4x.reset();
    cleanDelay.reset();

    {
        const juce::SpinLock::ScopedLockType lock(convolutionLock);
        convolution.reset();
    }

    bassFilter.reset();
    lowMidFilter.reset();
    highMidFilter.reset();
    trebleFilter.reset();
    cabHighPass.reset();
    cabLowPass.reset();
    cabMid.reset();

    inputPeak.store(0.0f);
    outputPeak.store(0.0f);

    outputFade.setCurrentAndTargetValue(0.0f);
    outputFade.setTargetValue(1.0f);
}

bool BassAmpProcessor::loadUserImpulseResponse(const juce::File& file)
{
    if (!file.existsAsFile())
        return false;

    const juce::SpinLock::ScopedLockType lock(convolutionLock);
    convolution.loadImpulseResponse(file,
                                    juce::dsp::Convolution::Stereo::no,
                                    juce::dsp::Convolution::Trim::yes,
                                    0,
                                    juce::dsp::Convolution::Normalise::yes);
    userIrLoaded.store(true);
    return true;
}

void BassAmpProcessor::clearUserImpulseResponse() noexcept
{
    userIrLoaded.store(false);
    if (cabinet.load() == static_cast<int>(Cabinet::userIr))
        cabinet.store(static_cast<int>(Cabinet::di));
}

BassAmpProcessor::OversamplingMode BassAmpProcessor::getOversamplingMode() const noexcept
{
    return static_cast<OversamplingMode>(juce::jlimit(0, 2, oversamplingMode.load()));
}

int BassAmpProcessor::getLatencySamples() const noexcept
{
    switch (getOversamplingMode())
    {
        case OversamplingMode::x2:
            return static_cast<int>(std::lround(oversampling2x.getLatencyInSamples()));
        case OversamplingMode::x4:
            return static_cast<int>(std::lround(oversampling4x.getLatencyInSamples()));
        case OversamplingMode::off:
        default:
            return 0;
    }
}

void BassAmpProcessor::updateBlockParameters() noexcept
{
    noiseGate.setThreshold(juce::jlimit(-90.0f, -15.0f, gateThresholdDb.load()));
    noiseGate.setRatio(6.0f);
    noiseGate.setAttack(2.0f);
    noiseGate.setRelease(120.0f);

    const auto amount = clampPercent(compAmount.load());
    compressor.setThreshold(juce::jmap(amount, -8.0f, -30.0f));
    compressor.setRatio(juce::jmap(amount, 1.2f, 6.0f));
    compressor.setAttack(juce::jmap(amount, 18.0f, 4.0f));
    compressor.setRelease(juce::jmap(amount, 150.0f, 70.0f));

    limiter.setThreshold(juce::jlimit(-12.0f, -0.1f, limiterThresholdDb.load()));

    inputGainLinear.setTargetValue(
        juce::Decibels::decibelsToGain(juce::jlimit(-24.0f, 24.0f, inputGainDb.load())));
    driveSmoothed.setTargetValue(clampPercent(drive.load()));
    characterSmoothed.setTargetValue(clampPercent(character.load()));
    lowCleanSmoothed.setTargetValue(clampPercent(lowClean.load()));
    masterGainLinear.setTargetValue(
        juce::Decibels::decibelsToGain(juce::jlimit(-60.0f, 6.0f, masterDb.load())));

    updateToneFilters();
    updateCabinetFilters();
}

void BassAmpProcessor::updateToneFilters() noexcept
{
    const auto bass = juce::jlimit(-12.0f, 12.0f, bassDb.load());
    const auto lowMid = juce::jlimit(-12.0f, 12.0f, lowMidDb.load());
    const auto highMid = juce::jlimit(-12.0f, 12.0f, highMidDb.load());
    const auto treble = juce::jlimit(-12.0f, 12.0f, trebleDb.load());

    if (bass != lastBassDb)
    {
        bassFilter.setLowShelf(currentSampleRate, 80.0, bass);
        lastBassDb = bass;
    }

    if (lowMid != lastLowMidDb)
    {
        lowMidFilter.setPeak(currentSampleRate, 250.0, 0.9, lowMid);
        lastLowMidDb = lowMid;
    }

    if (highMid != lastHighMidDb)
    {
        highMidFilter.setPeak(currentSampleRate, 1200.0, 0.9, highMid);
        lastHighMidDb = highMid;
    }

    if (treble != lastTrebleDb)
    {
        trebleFilter.setHighShelf(currentSampleRate, 4000.0, treble);
        lastTrebleDb = treble;
    }
}

void BassAmpProcessor::updateCabinetFilters() noexcept
{
    const auto requested = juce::jlimit(0, 4, cabinet.load());
    if (requested == activeCabinet)
        return;

    activeCabinet = requested;
    cabHighPass.reset();
    cabLowPass.reset();
    cabMid.reset();

    switch (static_cast<Cabinet>(activeCabinet))
    {
        case Cabinet::compact115:
            cabHighPass.setHighPass(currentSampleRate, 38.0);
            cabLowPass.setLowPass(currentSampleRate, 5200.0);
            cabMid.setPeak(currentSampleRate, 700.0, 0.75, 1.8);
            break;

        case Cabinet::punch410:
            cabHighPass.setHighPass(currentSampleRate, 48.0);
            cabLowPass.setLowPass(currentSampleRate, 6800.0);
            cabMid.setPeak(currentSampleRate, 950.0, 0.8, 2.4);
            break;

        case Cabinet::massive810:
            cabHighPass.setHighPass(currentSampleRate, 42.0);
            cabLowPass.setLowPass(currentSampleRate, 6100.0);
            cabMid.setPeak(currentSampleRate, 1100.0, 0.7, 3.2);
            break;

        case Cabinet::userIr:
        case Cabinet::di:
        default:
            break;
    }
}

float BassAmpProcessor::saturate(float sample,
                                 float driveAmount,
                                 float characterAmount,
                                 Voicing mode) const noexcept
{
    const auto driveGain = 1.0f + driveAmount * 14.0f;
    const auto characterGain = 0.75f + characterAmount * 0.8f;
    const auto x = sample * driveGain;

    switch (mode)
    {
        case Voicing::vintage:
            return std::tanh(x * (0.72f + 0.35f * characterAmount)) * (0.92f / characterGain);

        case Voicing::aggressive:
        {
            const auto asymmetric = x + (0.10f + 0.16f * characterAmount) * x * std::abs(x);
            return std::tanh(asymmetric * (1.05f + 0.55f * characterAmount)) * (0.78f / characterGain);
        }

        case Voicing::modern:
        default:
        {
            const auto soft = std::tanh(x * (0.88f + 0.30f * characterAmount));
            const auto detail = x / (1.0f + std::abs(x));
            return (0.82f * soft + 0.18f * detail) * (0.88f / characterGain);
        }
    }
}

float BassAmpProcessor::sanitize(float sample) noexcept
{
    if (!std::isfinite(sample))
        return 0.0f;

    return juce::jlimit(-8.0f, 8.0f, sample);
}

void BassAmpProcessor::processAmpStage(int numSamples,
                                       Voicing voicingMode,
                                       OversamplingMode oversampling) noexcept
{
    juce::dsp::AudioBlock<float> wholeBaseBlock(ampWork);
    auto baseBlock = wholeBaseBlock.getSubBlock(0, static_cast<size_t>(numSamples));

    if (oversampling == OversamplingMode::off)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            ampWork.setSample(0, i,
                              saturate(ampWork.getSample(0, i),
                                       driveValues[static_cast<size_t>(i)] * 0.55f,
                                       characterValues[static_cast<size_t>(i)] * 0.45f,
                                       voicingMode));
            ampWork.setSample(1, i,
                              saturate(ampWork.getSample(1, i),
                                       driveValues[static_cast<size_t>(i)],
                                       characterValues[static_cast<size_t>(i)],
                                       voicingMode));
        }
        return;
    }

    auto& oversampler = oversampling == OversamplingMode::x4 ? oversampling4x : oversampling2x;
    const auto factor = oversampling == OversamplingMode::x4 ? 4 : 2;
    auto oversampledBlock = oversampler.processSamplesUp(baseBlock);

    for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel)
    {
        auto* samples = oversampledBlock.getChannelPointer(channel);
        const auto sampleCount = static_cast<int>(oversampledBlock.getNumSamples());

        for (int i = 0; i < sampleCount; ++i)
        {
            const auto baseIndex = juce::jlimit(0, numSamples - 1, i / factor);
            const auto driveAmount = driveValues[static_cast<size_t>(baseIndex)]
                * (channel == 0 ? 0.55f : 1.0f);
            const auto characterAmount = characterValues[static_cast<size_t>(baseIndex)]
                * (channel == 0 ? 0.45f : 1.0f);
            samples[i] = saturate(samples[i], driveAmount, characterAmount, voicingMode);
        }
    }

    oversampler.processSamplesDown(baseBlock);
}

void BassAmpProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                    int startSample,
                                    int numSamples) noexcept
{
    if (numSamples <= 0 || buffer.getNumChannels() <= 0)
        return;

    juce::ScopedNoDenormals noDenormals;
    updateBlockParameters();

    auto* mono = buffer.getWritePointer(0, startSample);
    float inPeak = 0.0f;
    float outPeak = 0.0f;

    if (bypassed.load())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            const auto sample = sanitize(mono[i]);
            mono[i] = sample;
            inPeak = juce::jmax(inPeak, std::abs(sample));
            outPeak = juce::jmax(outPeak, std::abs(sample));
        }
    }
    else
    {
        const auto selectedVoicing = static_cast<Voicing>(juce::jlimit(0, 2, voicing.load()));
        const auto selectedCabinet = static_cast<Cabinet>(juce::jlimit(0, 4, cabinet.load()));
        auto selectedOversampling = getOversamplingMode();

        if (numSamples > maximumPreparedBlockSize)
            selectedOversampling = OversamplingMode::off;

        for (int i = 0; i < numSamples; ++i)
        {
            auto x = sanitize(mono[i]);
            inPeak = juce::jmax(inPeak, std::abs(x));

            x *= inputGainLinear.getNextValue();
            x = noiseGate.processSample(0, x);
            x = compressor.processSample(0, x);

            float low = 0.0f;
            float high = 0.0f;
            crossover.processSample(0, x, low, high);

            if (i < maximumPreparedBlockSize)
            {
                ampWork.setSample(0, i, low);
                ampWork.setSample(1, i, high);
                cleanLowValues[static_cast<size_t>(i)] = low;
                driveValues[static_cast<size_t>(i)] = driveSmoothed.getNextValue();
                characterValues[static_cast<size_t>(i)] = characterSmoothed.getNextValue();
                cleanMixValues[static_cast<size_t>(i)] = lowCleanSmoothed.getNextValue();
            }
        }

        if (numSamples <= maximumPreparedBlockSize)
            processAmpStage(numSamples, selectedVoicing, selectedOversampling);

        const auto cleanDelaySamples = selectedOversampling == OversamplingMode::x4
            ? oversampling4x.getLatencyInSamples()
            : selectedOversampling == OversamplingMode::x2
                ? oversampling2x.getLatencyInSamples()
                : 0.0f;
        cleanDelay.setDelay(static_cast<float>(cleanDelaySamples));

        for (int i = 0; i < numSamples; ++i)
        {
            float x = 0.0f;

            if (i < maximumPreparedBlockSize)
            {
                const auto distortedLow = ampWork.getSample(0, i);
                cleanDelay.pushSample(0, cleanLowValues[static_cast<size_t>(i)]);
                const auto cleanLow = cleanDelay.popSample(0);
                const auto cleanAmount = cleanMixValues[static_cast<size_t>(i)];
                const auto lowOut = distortedLow + (cleanLow - distortedLow) * cleanAmount;
                const auto highOut = ampWork.getSample(1, i);
                x = lowOut + highOut;
            }
            else
            {
                x = sanitize(mono[i]);
            }

            x = bassFilter.process(x);
            x = lowMidFilter.process(x);
            x = highMidFilter.process(x);
            x = trebleFilter.process(x);

            if (selectedCabinet != Cabinet::di
                && selectedCabinet != Cabinet::userIr)
            {
                x = cabHighPass.process(x);
                x = cabMid.process(x);
                x = cabLowPass.process(x);
            }

            mono[i] = sanitize(x);
        }

        if (selectedCabinet == Cabinet::userIr && userIrLoaded.load())
        {
            const juce::SpinLock::ScopedTryLockType lock(convolutionLock);
            if (lock.isLocked())
            {
                juce::dsp::AudioBlock<float> wholeBlock(buffer);
                auto monoBlock = wholeBlock.getSingleChannelBlock(0)
                    .getSubBlock(static_cast<size_t>(startSample), static_cast<size_t>(numSamples));
                juce::dsp::ProcessContextReplacing<float> context(monoBlock);
                convolution.process(context);
            }
        }

        for (int i = 0; i < numSamples; ++i)
        {
            auto x = sanitize(mono[i]);
            x *= masterGainLinear.getNextValue();
            x *= outputFade.getNextValue();
            mono[i] = sanitize(x);
        }
    }

    for (int channel = 1; channel < buffer.getNumChannels(); ++channel)
        buffer.copyFrom(channel, startSample, buffer, 0, startSample, numSamples);

    if (!bypassed.load())
    {
        juce::dsp::AudioBlock<float> wholeBlock(buffer);
        auto subBlock = wholeBlock.getSubBlock(static_cast<size_t>(startSample),
                                               static_cast<size_t>(numSamples));
        juce::dsp::ProcessContextReplacing<float> context(subBlock);
        limiter.process(context);
    }

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* samples = buffer.getReadPointer(channel, startSample);
        for (int i = 0; i < numSamples; ++i)
            outPeak = juce::jmax(outPeak, std::abs(sanitize(samples[i])));
    }

    inputPeak.store(inPeak);
    outputPeak.store(outPeak);
}
