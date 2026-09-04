#include "MainComponent.h"

MainComponent::MainComponent()
    : deviceSelector(deviceManager,
                     1, 2,
                     1, 2,
                     false, false,
                     true, false)
{
    setOpaque(true);
    setSize(1180, 720);

    configureKnob(inputGainKnob, inputGainLabel, "INPUT dB", -24.0, 24.0, 0.0);
    configureKnob(gateKnob, gateLabel, "GATE dB", -90.0, -15.0, -65.0);
    configureKnob(compKnob, compLabel, "COMP %", 0.0, 100.0, 30.0, 1.0);
    configureKnob(driveKnob, driveLabel, "DRIVE", 0.0, 100.0, 25.0, 1.0);
    configureKnob(characterKnob, characterLabel, "CHARACTER", 0.0, 100.0, 50.0, 1.0);
    configureKnob(lowCleanKnob, lowCleanLabel, "LOW CLEAN", 0.0, 100.0, 65.0, 1.0);

    configureKnob(bassKnob, bassLabel, "BASS dB", -12.0, 12.0, 0.0);
    configureKnob(lowMidKnob, lowMidLabel, "LOW MID dB", -12.0, 12.0, 0.0);
    configureKnob(highMidKnob, highMidLabel, "HIGH MID dB", -12.0, 12.0, 0.0);
    configureKnob(trebleKnob, trebleLabel, "TREBLE dB", -12.0, 12.0, 0.0);
    configureKnob(masterKnob, masterLabel, "MASTER dB", -60.0, 6.0, -6.0);

    for (auto* slider : { &inputGainKnob, &gateKnob, &compKnob, &driveKnob, &characterKnob,
                          &lowCleanKnob, &bassKnob, &lowMidKnob, &highMidKnob, &trebleKnob, &masterKnob })
        slider->onValueChange = [this] { pushParameters(); };

    for (auto* button : { &vintageButton, &modernButton, &aggressiveButton })
    {
        addAndMakeVisible(*button);
        button->setRadioGroupId(1001);
        button->setClickingTogglesState(true);
    }

    modernButton.setToggleState(true, juce::dontSendNotification);
    vintageButton.onClick = [this] { processor.setVoicing(BassAmpProcessor::Voicing::vintage); };
    modernButton.onClick = [this] { processor.setVoicing(BassAmpProcessor::Voicing::modern); };
    aggressiveButton.onClick = [this] { processor.setVoicing(BassAmpProcessor::Voicing::aggressive); };

    addAndMakeVisible(audioSetupButton);
    audioSetupButton.onClick = [this]
    {
        audioPanelVisible = !audioPanelVisible;
        deviceSelector.setVisible(audioPanelVisible);
        resized();
    };

    addAndMakeVisible(bypassButton);
    bypassButton.setClickingTogglesState(true);
    bypassButton.onClick = [this] { processor.setBypassed(bypassButton.getToggleState()); };

    cabinetLabel.setText("CABINET", juce::dontSendNotification);
    cabinetLabel.setJustificationType(juce::Justification::centred);
    cabinetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(cabinetLabel);

    cabinetBox.addItem("DI / OFF", 1);
    cabinetBox.addItem("Compact 1x15", 2);
    cabinetBox.addItem("Punch 4x10", 3);
    cabinetBox.addItem("Massive 8x10", 4);
    cabinetBox.setSelectedId(1, juce::dontSendNotification);
    cabinetBox.onChange = [this]
    {
        processor.setCabinet(static_cast<BassAmpProcessor::Cabinet>(juce::jlimit(0, 3, cabinetBox.getSelectedId() - 1)));
    };
    addAndMakeVisible(cabinetBox);

    statusLabel.setColour(juce::Label::textColourId, juce::Colours::silver);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    for (auto* meter : { &inputMeterLabel, &outputMeterLabel })
    {
        meter->setColour(juce::Label::textColourId, juce::Colours::white);
        meter->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*meter);
    }

    addAndMakeVisible(deviceSelector);
    deviceSelector.setVisible(false);

    pushParameters();

    setAudioChannels(1, 2);
    startTimerHz(20);
}

MainComponent::~MainComponent()
{
    stopTimer();
    shutdownAudio();
}

void MainComponent::configureKnob(juce::Slider& slider, juce::Label& label, const juce::String& name,
                                  double minimum, double maximum, double initial, double interval)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 76, 22);
    slider.setRange(minimum, maximum, interval);
    slider.setValue(initial, juce::dontSendNotification);
    slider.setDoubleClickReturnValue(true, initial);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour::fromRGB(70, 200, 170));
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(label);
}

void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    const auto outputChannels = deviceManager.getCurrentAudioDevice() != nullptr
        ? juce::jmax(1, deviceManager.getCurrentAudioDevice()->getActiveOutputChannels().countNumberOfSetBits())
        : 2;

    processor.prepare(sampleRate, samplesPerBlockExpected, outputChannels);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (bufferToFill.buffer == nullptr || bufferToFill.numSamples <= 0)
        return;

    processor.processBlock(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);
}

void MainComponent::releaseResources()
{
    processor.reset();
}

void MainComponent::pushParameters() noexcept
{
    processor.setInputGainDb(static_cast<float>(inputGainKnob.getValue()));
    processor.setGateThresholdDb(static_cast<float>(gateKnob.getValue()));
    processor.setCompAmount(static_cast<float>(compKnob.getValue()));
    processor.setDrive(static_cast<float>(driveKnob.getValue()));
    processor.setCharacter(static_cast<float>(characterKnob.getValue()));
    processor.setLowClean(static_cast<float>(lowCleanKnob.getValue()));
    processor.setBassDb(static_cast<float>(bassKnob.getValue()));
    processor.setLowMidDb(static_cast<float>(lowMidKnob.getValue()));
    processor.setHighMidDb(static_cast<float>(highMidKnob.getValue()));
    processor.setTrebleDb(static_cast<float>(trebleKnob.getValue()));
    processor.setMasterDb(static_cast<float>(masterKnob.getValue()));
}

void MainComponent::timerCallback()
{
    const auto inDb = juce::Decibels::gainToDecibels(processor.getInputPeak(), -100.0f);
    const auto outDb = juce::Decibels::gainToDecibels(processor.getOutputPeak(), -100.0f);
    inputMeterLabel.setText("IN " + juce::String(inDb, 1) + " dB", juce::dontSendNotification);
    outputMeterLabel.setText("OUT " + juce::String(outDb, 1) + " dB", juce::dontSendNotification);

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        statusLabel.setText(device->getTypeName() + " | " + device->getName()
                            + " | " + juce::String(device->getCurrentSampleRate() / 1000.0, 1) + " kHz"
                            + " | " + juce::String(device->getCurrentBufferSizeSamples()) + " samples",
                            juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("No audio device", juce::dontSendNotification);
    }
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(18, 20, 24));

    auto header = getLocalBounds().removeFromTop(58);
    g.setColour(juce::Colour::fromRGB(27, 31, 37));
    g.fillRect(header);

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(25.0f, juce::Font::bold));
    g.drawText("BASS_amp", 18, 9, 180, 38, juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(38, 43, 50));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(12.0f).withTrimmedTop(56.0f), 10.0f, 1.0f);
}

void MainComponent::placeKnob(juce::Rectangle<int> area, juce::Label& label, juce::Slider& slider)
{
    label.setBounds(area.removeFromTop(24));
    slider.setBounds(area.reduced(4));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(14);
    auto header = bounds.removeFromTop(44);

    header.removeFromLeft(190);
    inputMeterLabel.setBounds(header.removeFromLeft(110));
    outputMeterLabel.setBounds(header.removeFromLeft(120));
    audioSetupButton.setBounds(header.removeFromRight(126).reduced(3));
    bypassButton.setBounds(header.removeFromRight(95).reduced(3));
    statusLabel.setBounds(header.reduced(4));

    bounds.removeFromTop(18);
    auto voiceRow = bounds.removeFromTop(46);
    const auto voiceWidth = juce::jmin(160, voiceRow.getWidth() / 3);
    auto voiceGroup = voiceRow.withSizeKeepingCentre(voiceWidth * 3, 40);
    vintageButton.setBounds(voiceGroup.removeFromLeft(voiceWidth).reduced(3));
    modernButton.setBounds(voiceGroup.removeFromLeft(voiceWidth).reduced(3));
    aggressiveButton.setBounds(voiceGroup.removeFromLeft(voiceWidth).reduced(3));

    bounds.removeFromTop(12);
    auto row1 = bounds.removeFromTop(220);
    constexpr int firstRowCount = 6;
    const auto cellWidth1 = row1.getWidth() / firstRowCount;
    placeKnob(row1.removeFromLeft(cellWidth1), inputGainLabel, inputGainKnob);
    placeKnob(row1.removeFromLeft(cellWidth1), gateLabel, gateKnob);
    placeKnob(row1.removeFromLeft(cellWidth1), compLabel, compKnob);
    placeKnob(row1.removeFromLeft(cellWidth1), driveLabel, driveKnob);
    placeKnob(row1.removeFromLeft(cellWidth1), characterLabel, characterKnob);
    placeKnob(row1, lowCleanLabel, lowCleanKnob);

    bounds.removeFromTop(8);
    auto row2 = bounds.removeFromTop(220);
    constexpr int secondRowCount = 6;
    const auto cellWidth2 = row2.getWidth() / secondRowCount;
    placeKnob(row2.removeFromLeft(cellWidth2), bassLabel, bassKnob);
    placeKnob(row2.removeFromLeft(cellWidth2), lowMidLabel, lowMidKnob);
    placeKnob(row2.removeFromLeft(cellWidth2), highMidLabel, highMidKnob);
    placeKnob(row2.removeFromLeft(cellWidth2), trebleLabel, trebleKnob);
    placeKnob(row2.removeFromLeft(cellWidth2), masterLabel, masterKnob);

    auto cabArea = row2.reduced(10);
    cabinetLabel.setBounds(cabArea.removeFromTop(28));
    cabinetBox.setBounds(cabArea.removeFromTop(34));

    if (audioPanelVisible)
    {
        auto selectorBounds = getLocalBounds().reduced(22);
        selectorBounds.removeFromLeft(juce::jmax(0, selectorBounds.getWidth() - 430));
        selectorBounds.removeFromTop(56);
        deviceSelector.toFront(false);
        deviceSelector.setBounds(selectorBounds);
    }
}
