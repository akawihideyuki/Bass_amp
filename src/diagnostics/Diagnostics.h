#pragma once

#include <JuceHeader.h>
#include <atomic>

class Diagnostics
{
public:
    void reset() noexcept
    {
        callbackPeakMs.store(0.0);
        deadlineMissCount.store(0);
    }

    void recordCallback(double elapsedSeconds, double deadlineSeconds) noexcept
    {
        const auto elapsedMs = elapsedSeconds * 1000.0;
        auto current = callbackPeakMs.load();

        while (elapsedMs > current && !callbackPeakMs.compare_exchange_weak(current, elapsedMs))
        {
        }

        if (deadlineSeconds > 0.0 && elapsedSeconds > deadlineSeconds)
            deadlineMissCount.fetch_add(1);
    }

    [[nodiscard]] double getCallbackPeakMs() const noexcept { return callbackPeakMs.load(); }
    [[nodiscard]] int getDeadlineMissCount() const noexcept { return deadlineMissCount.load(); }

private:
    std::atomic<double> callbackPeakMs { 0.0 };
    std::atomic<int> deadlineMissCount { 0 };
};
