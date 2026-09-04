#pragma once

#include <JuceHeader.h>
#include "PitchDetector.h"

#include <array>
#include <atomic>

class TunerEngine
{
public:
    struct Display
    {
        bool valid = false;
        juce::String noteName { "--" };
        double frequencyHz = 0.0;
        double cents = 0.0;
        float confidence = 0.0f;
    };

    TunerEngine();

    void setSampleRate(double newSampleRate) noexcept;
    void setReferenceA4(double frequency) noexcept;
    [[nodiscard]] double getReferenceA4() const noexcept { return referenceA4.load(); }

    void pushSamples(const float* samples, int numSamples) noexcept;
    Display analyse();

private:
    static constexpr int fifoSize = 32768;
    static constexpr int sourceWindowSize = 8192;
    static constexpr int downsampleFactor = 2;
    static constexpr int analysisSize = sourceWindowSize / downsampleFactor;

    juce::AbstractFifo fifo { fifoSize };
    std::array<float, fifoSize> fifoBuffer {};
    std::array<float, sourceWindowSize> sourceWindow {};
    std::array<float, analysisSize> analysisBuffer {};

    int sourceWritePosition = 0;
    int sourceSamplesAvailable = 0;

    std::atomic<double> sampleRate { 48000.0 };
    std::atomic<double> referenceA4 { 440.0 };

    double lastAnalysisMs = 0.0;
    PitchDetector detector;
    Display lastDisplay;
};
