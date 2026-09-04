#pragma once

#include <JuceHeader.h>
#include "PresetState.h"

class PresetManager
{
public:
    PresetManager();

    [[nodiscard]] const juce::Array<juce::String>& getFactoryNames() const noexcept { return factoryNames; }
    [[nodiscard]] PresetState getFactoryPreset(int index) const;
    [[nodiscard]] juce::Array<juce::File> getUserPresetFiles() const;

    bool saveToFile(const juce::File& file, const PresetState& state, juce::String& error) const;
    bool loadFromFile(const juce::File& file, PresetState& state, juce::String& error) const;

    [[nodiscard]] juce::File getPresetDirectory() const;
    [[nodiscard]] juce::File getSuggestedPresetFile(const juce::String& baseName) const;

    static juce::String toJson(const PresetState& state);
    static bool fromJson(const juce::String& json, PresetState& state, juce::String& error);

private:
    juce::Array<juce::String> factoryNames {
        "Clean Studio",
        "Warm Vintage",
        "Modern Punch",
        "Rock Grind",
        "Aggressive Pick"
    };
};
