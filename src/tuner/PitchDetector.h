#pragma once

#include <array>
#include <cstddef>

class PitchDetector
{
public:
    struct Result
    {
        double frequencyHz = 0.0;
        float confidence = 0.0f;

        [[nodiscard]] bool isValid() const noexcept
        {
            return frequencyHz > 0.0 && confidence > 0.0f;
        }
    };

    Result detect(const float* samples, int numSamples, double sampleRate) noexcept;

private:
    static constexpr int maxTauStorage = 2048;
    std::array<float, maxTauStorage> difference {};
    std::array<float, maxTauStorage> cumulative {};
};
