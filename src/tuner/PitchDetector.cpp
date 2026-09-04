#include "PitchDetector.h"

#include <algorithm>
#include <cmath>

PitchDetector::Result PitchDetector::detect(const float* samples, int numSamples, double sampleRate) noexcept
{
    Result result;

    if (samples == nullptr || numSamples < 1024 || sampleRate < 1000.0)
        return result;

    double energy = 0.0;
    for (int i = 0; i < numSamples; ++i)
        energy += static_cast<double>(samples[i]) * static_cast<double>(samples[i]);

    const auto rms = std::sqrt(energy / static_cast<double>(numSamples));
    if (rms < 0.0008)
        return result;

    constexpr double minFrequency = 25.0;
    constexpr double maxFrequency = 350.0;
    constexpr float threshold = 0.16f;

    const auto minTau = std::max(2, static_cast<int>(std::floor(sampleRate / maxFrequency)));
    const auto maxTau = std::min({ maxTauStorage - 1,
                                   numSamples / 2,
                                   static_cast<int>(std::ceil(sampleRate / minFrequency)) });

    if (maxTau <= minTau + 2)
        return result;

    std::fill(difference.begin(), difference.end(), 0.0f);
    std::fill(cumulative.begin(), cumulative.end(), 1.0f);

    for (int tau = 1; tau <= maxTau; ++tau)
    {
        double sum = 0.0;
        const auto count = numSamples - tau;

        for (int i = 0; i < count; ++i)
        {
            const auto delta = static_cast<double>(samples[i]) - static_cast<double>(samples[i + tau]);
            sum += delta * delta;
        }

        difference[static_cast<size_t>(tau)] = static_cast<float>(sum);
    }

    double running = 0.0;
    cumulative[0] = 1.0f;

    for (int tau = 1; tau <= maxTau; ++tau)
    {
        running += static_cast<double>(difference[static_cast<size_t>(tau)]);
        cumulative[static_cast<size_t>(tau)] = running > 0.0
            ? static_cast<float>(difference[static_cast<size_t>(tau)] * static_cast<float>(tau) / running)
            : 1.0f;
    }

    int bestTau = 0;

    for (int tau = minTau; tau <= maxTau; ++tau)
    {
        if (cumulative[static_cast<size_t>(tau)] < threshold)
        {
            while (tau + 1 <= maxTau
                   && cumulative[static_cast<size_t>(tau + 1)] < cumulative[static_cast<size_t>(tau)])
                ++tau;

            bestTau = tau;
            break;
        }
    }

    if (bestTau == 0)
    {
        auto bestValue = 1.0f;
        for (int tau = minTau; tau <= maxTau; ++tau)
        {
            if (cumulative[static_cast<size_t>(tau)] < bestValue)
            {
                bestValue = cumulative[static_cast<size_t>(tau)];
                bestTau = tau;
            }
        }

        if (bestTau == 0 || bestValue > 0.35f)
            return result;
    }

    double refinedTau = static_cast<double>(bestTau);
    if (bestTau > 1 && bestTau < maxTau)
    {
        const auto left = static_cast<double>(cumulative[static_cast<size_t>(bestTau - 1)]);
        const auto centre = static_cast<double>(cumulative[static_cast<size_t>(bestTau)]);
        const auto right = static_cast<double>(cumulative[static_cast<size_t>(bestTau + 1)]);
        const auto denominator = left - 2.0 * centre + right;

        if (std::abs(denominator) > 1.0e-12)
            refinedTau += 0.5 * (left - right) / denominator;
    }

    if (refinedTau <= 0.0)
        return result;

    result.frequencyHz = sampleRate / refinedTau;
    result.confidence = std::clamp(1.0f - cumulative[static_cast<size_t>(bestTau)], 0.0f, 1.0f);

    if (!std::isfinite(result.frequencyHz)
        || result.frequencyHz < minFrequency
        || result.frequencyHz > maxFrequency)
    {
        return {};
    }

    return result;
}
