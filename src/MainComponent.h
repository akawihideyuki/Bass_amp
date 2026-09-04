#pragma once

#include <JuceHeader.h>
#include "dsp/BassAmpProcessor.h"

class MainComponent final : public juce::AudioAppComponent,
                            private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void timerCallback() override;
    void configureKnob(juce::Slider& slider, juce::Label& label, const juce::String& name,
                       double minimum, double maximum, double initial, double interval = 0.1);
    void pushParameters() noexcept;
    void placeKnob(juce::Rectangle<int> area, juce::Label& label, juce::Slider& slider);

    BassAmpProcessor processor;
    juce::AudioDeviceSelectorComponent deviceSelector;

    juce::TextButton audioSetupButton { "AUDIO SETUP" };
    juce::TextButton bypassButton { "BYPASS" };
    juce::TextButton vintageButton { "VINTAGE" };
    juce::TextButton modernButton { "MODERN" };
    juce::TextButton aggressiveButton { "AGGRESSIVE" };

    juce::Slider inputGainKnob, gateKnob, compKnob, driveKnob, characterKnob, lowCleanKnob;
    juce::Slider bassKnob, lowMidKnob, highMidKnob, trebleKnob, masterKnob;

    juce::Label inputGainLabel, gateLabel, compLabel, driveLabel, characterLabel, lowCleanLabel;
    juce::Label bassLabel, lowMidLabel, highMidLabel, trebleLabel, masterLabel;

    juce::ComboBox cabinetBox;
    juce::Label cabinetLabel;
    juce::Label statusLabel;
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;

    bool audioPanelVisible = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
