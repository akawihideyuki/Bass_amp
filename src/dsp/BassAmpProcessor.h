#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

class BassAmpProcessor
{
public:
    enum class Voicing { vintage = 0, modern, aggressive };
    enum class Cabinet { di = 0, compact115, punch410, massive810, userIr };
    enum class OversamplingMode { off = 0, x2, x4 };

    void prepare(double sampleRate, int maximumBlockSize, int outputChannels = 2);
    void reset();
    void processBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept;

    void setInputGainDb(float value) noexcept { inputGainDb.store(value); }
    void setGateThresholdDb(float value) noexcept { gateThresholdDb.store(value); }
    void setCompAmount(float value) noexcept { compAmount.store(value); }
    void setDrive(float value) noexcept { drive.store(value); }
    void setCharacter(float value) noexcept { character.store(value); }
    void setLowClean(float value) noexcept { lowClean.store(value); }
    void setBassDb(float value) noexcept { bassDb.store(value); }
    void setLowMidDb(float value) noexcept { lowMidDb.store(value); }
    void setHighMidDb(float value) noexcept { highMidDb.store(value); }
    void setTrebleDb(float value) noexcept { trebleDb.store(value); }
    void setMasterDb(float value) noexcept { masterDb.store(value); }
    void setLimiterThresholdDb(float value) noexcept { limiterThresholdDb.store(value); }
    void setVoicing(Voicing value) noexcept { voicing.store(static_cast<int>(value)); }
    void setCabinet(Cabinet value) noexcept { cabinet.store(static_cast<int>(value)); }
    void setOversamplingMode(OversamplingMode value) noexcept { oversamplingMode.store(static_cast<int>(value)); }
    void setBypassed(bool value) noexcept { bypassed.store(value); }

    bool loadUserImpulseResponse(const juce::File& file);
    void clearUserImpulseResponse() noexcept;
    [[nodiscard]] bool hasUserImpulseResponse() const noexcept { return userIrLoaded.load(); }

    [[nodiscard]] float getInputPeak() const noexcept { return inputPeak.load(); }
    [[nodiscard]] float getOutputPeak() const noexcept { return outputPeak.load(); }
    [[nodiscard]] int getLatencySamples() const noexcept;
    [[nodiscard]] OversamplingMode getOversamplingMode() const noexcept;

private:
    class Biquad
    {
    public:
        void reset() noexcept { z1 = z2 = 0.0; }
        float process(float x) noexcept;
        void setLowPass(double sampleRate, double frequency, double q = 0.70710678118) noexcept;
        void setHighPass(double sampleRate, double frequency, double q = 0.70710678118) noexcept;
        void setPeak(double sampleRate, double frequency, double q, double gainDb) noexcept;
        void setLowShelf(double sampleRate, double frequency, double gainDb) noexcept;
        void setHighShelf(double sampleRate, double frequency, double gainDb) noexcept;

    private:
        void setNormalized(double nb0, double nb1, double nb2,
                           double na0, double na1, double na2) noexcept;
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
        double z1 = 0.0, z2 = 0.0;
    };

    void updateBlockParameters() noexcept;
    void updateToneFilters() noexcept;
    void updateCabinetFilters() noexcept;
    float saturate(float sample, float driveAmount, float characterAmount, Voicing mode) const noexcept;
    static float sanitize(float sample) noexcept;
    void processAmpStage(int numSamples, Voicing voicingMode, OversamplingMode oversampling) noexcept;

    std::atomic<float> inputGainDb { 0.0f };
    std::atomic<float> gateThresholdDb { -65.0f };
    std::atomic<float> compAmount { 30.0f };
    std::atomic<float> drive { 25.0f };
    std::atomic<float> character { 50.0f };
    std::atomic<float> lowClean { 65.0f };
    std::atomic<float> bassDb { 0.0f };
    std::atomic<float> lowMidDb { 0.0f };
    std::atomic<float> highMidDb { 0.0f };
    std::atomic<float> trebleDb { 0.0f };
    std::atomic<float> masterDb { -6.0f };
    std::atomic<float> limiterThresholdDb { -1.0f };
    std::atomic<int> voicing { static_cast<int>(Voicing::modern) };
    std::atomic<int> cabinet { static_cast<int>(Cabinet::di) };
    std::atomic<int> oversamplingMode { static_cast<int>(OversamplingMode::x2) };
    std::atomic<bool> bypassed { false };
    std::atomic<bool> userIrLoaded { false };

    std::atomic<float> inputPeak { 0.0f };
    std::atomic<float> outputPeak { 0.0f };

    juce::dsp::NoiseGate<float> noiseGate;
    juce::dsp::Compressor<float> compressor;
    juce::dsp::LinkwitzRileyFilter<float> crossover;
    juce::dsp::Limiter<float> limiter;
    juce::dsp::Convolution convolution;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> cleanDelay { 128 };

    juce::dsp::Oversampling<float> oversampling2x {
        2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true
    };
    juce::dsp::Oversampling<float> oversampling4x {
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true
    };

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inputGainLinear;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> driveSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> characterSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> lowCleanSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> masterGainLinear;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> outputFade;

    Biquad bassFilter, lowMidFilter, highMidFilter, trebleFilter;
    Biquad cabHighPass, cabLowPass, cabMid;

    juce::AudioBuffer<float> ampWork;
    std::vector<float> cleanLowValues;
    std::vector<float> driveValues;
    std::vector<float> characterValues;
    std::vector<float> cleanMixValues;

    juce::SpinLock convolutionLock;

    double currentSampleRate = 48000.0;
    int preparedOutputChannels = 2;
    int maximumPreparedBlockSize = 512;
    int activeCabinet = -1;
    float lastBassDb = 999.0f;
    float lastLowMidDb = 999.0f;
    float lastHighMidDb = 999.0f;
    float lastTrebleDb = 999.0f;
};
