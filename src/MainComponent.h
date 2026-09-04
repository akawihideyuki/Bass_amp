#pragma once

#include <JuceHeader.h>

#include "audio/AudioSettings.h"
#include "diagnostics/Diagnostics.h"
#include "dsp/BassAmpProcessor.h"
#include "preset/PresetManager.h"
#include "preset/PresetState.h"
#include "tuner/TunerEngine.h"

#include <atomic>
#include <memory>

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

    void configureKnob(juce::Slider& slider,
                       juce::Label& label,
                       const juce::String& name,
                       double minimum,
                       double maximum,
                       double initial,
                       double interval = 0.1);

    void pushParameters() noexcept;
    void placeKnob(juce::Rectangle<int> area, juce::Label& label, juce::Slider& slider);
    void setAudioPanelVisible(bool visible);
    void setAdvancedPanelVisible(bool visible);

    PresetState captureState() const;
    void applyState(const PresetState& state);
    void switchAB(bool useB);
    void refreshPresetList();
    void loadSelectedPreset();
    void savePresetAs();
    void deleteSelectedPreset();
    void chooseUserIr();

    void showError(const juce::String& title, const juce::String& message);
    void updateVoicingButtons(int voicingIndex);
    void updateAdvancedVisibility();

    BassAmpProcessor processor;
    TunerEngine tuner;
    PresetManager presetManager;
    Diagnostics diagnostics;

    juce::AudioDeviceSelectorComponent deviceSelector;
    juce::Component advancedPanel;

    juce::TextButton audioSetupButton { "AUDIO SETUP" };
    juce::TextButton advancedButton { "ADVANCED" };
    juce::TextButton bypassButton { "BYPASS" };

    juce::TextButton vintageButton { "VINTAGE" };
    juce::TextButton modernButton { "MODERN" };
    juce::TextButton aggressiveButton { "AGGRESSIVE" };

    juce::TextButton aButton { "A" };
    juce::TextButton bButton { "B" };
    juce::TextButton savePresetButton { "SAVE AS" };
    juce::TextButton deletePresetButton { "DELETE" };

    juce::Slider inputGainKnob, gateKnob, compKnob, driveKnob, characterKnob, lowCleanKnob;
    juce::Slider bassKnob, lowMidKnob, highMidKnob, trebleKnob, masterKnob;

    juce::Label inputGainLabel, gateLabel, compLabel, driveLabel, characterLabel, lowCleanLabel;
    juce::Label bassLabel, lowMidLabel, highMidLabel, trebleLabel, masterLabel;

    juce::ComboBox cabinetBox;
    juce::Label cabinetLabel;

    juce::ComboBox presetBox;
    juce::Label presetLabel;

    juce::Label statusLabel;
    juce::Label tunerLabel;
    juce::Label inputMeterLabel;
    juce::Label outputMeterLabel;

    juce::ComboBox oversamplingBox;
    juce::Label oversamplingLabel;
    juce::Slider tunerReferenceSlider;
    juce::Label tunerReferenceLabel;
    juce::Slider limiterThresholdSlider;
    juce::Label limiterThresholdLabel;
    juce::TextButton loadIrButton { "LOAD USER IR" };
    juce::Label irStatusLabel;
    juce::Label diagnosticsLabel;

    juce::Array<juce::File> userPresetFiles;
    std::unique_ptr<juce::FileChooser> activeChooser;

    PresetState stateA;
    PresetState stateB;
    bool usingB = false;

    bool audioPanelVisible = false;
    bool advancedPanelVisible = false;

    juce::File currentUserIrFile;

    std::atomic<double> currentSampleRate { 48000.0 };

    int inputClipHoldTicks = 0;
    int outputClipHoldTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
