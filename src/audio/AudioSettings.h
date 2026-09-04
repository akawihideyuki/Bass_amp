#pragma once

#include <JuceHeader.h>

class AudioSettings
{
public:
    static void restore(juce::AudioDeviceManager& manager);
    static void save(juce::AudioDeviceManager& manager);

private:
    static juce::File getSettingsFile();
};
