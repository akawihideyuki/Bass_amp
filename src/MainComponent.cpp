#include "MainComponent.h"

#include <cmath>

MainComponent::MainComponent()
    : deviceSelector(deviceManager,
                     1, 2,
                     1, 2,
                     false, false,
                     true, false)
{
    setOpaque(true);
    setSize(1260, 760);

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
                          &lowCleanKnob, &bassKnob, &lowMidKnob, &highMidKnob, &trebleKnob,
                          &masterKnob })
    {
        slider->onValueChange = [this] { pushParameters(); };
    }

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
    audioSetupButton.onClick = [this] { setAudioPanelVisible(!audioPanelVisible); };

    addAndMakeVisible(advancedButton);
    advancedButton.onClick = [this] { setAdvancedPanelVisible(!advancedPanelVisible); };

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
    cabinetBox.addItem("USER IR", 5);
    cabinetBox.setSelectedId(1, juce::dontSendNotification);
    cabinetBox.onChange = [this]
    {
        const auto index = juce::jlimit(0, 4, cabinetBox.getSelectedId() - 1);
        if (index == static_cast<int>(BassAmpProcessor::Cabinet::userIr)
            && !processor.hasUserImpulseResponse())
        {
            showError("USER IR",
                      "USER IRがまだ読み込まれていません。ADVANCEDからWAV IRを選択してください。");
            cabinetBox.setSelectedId(1, juce::dontSendNotification);
            processor.setCabinet(BassAmpProcessor::Cabinet::di);
            return;
        }

        processor.setCabinet(static_cast<BassAmpProcessor::Cabinet>(index));
    };
    addAndMakeVisible(cabinetBox);

    presetLabel.setText("PRESET", juce::dontSendNotification);
    presetLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    presetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(presetLabel);

    addAndMakeVisible(presetBox);
    presetBox.onChange = [this] { loadSelectedPreset(); };

    for (auto* button : { &aButton, &bButton })
    {
        addAndMakeVisible(*button);
        button->setRadioGroupId(2001);
        button->setClickingTogglesState(true);
    }

    aButton.setToggleState(true, juce::dontSendNotification);
    aButton.onClick = [this] { switchAB(false); };
    bButton.onClick = [this] { switchAB(true); };

    addAndMakeVisible(savePresetButton);
    savePresetButton.onClick = [this] { savePresetAs(); };

    addAndMakeVisible(deletePresetButton);
    deletePresetButton.onClick = [this] { deleteSelectedPreset(); };

    statusLabel.setColour(juce::Label::textColourId, juce::Colours::silver);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    tunerLabel.setColour(juce::Label::textColourId, juce::Colour::fromRGB(110, 235, 195));
    tunerLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    tunerLabel.setJustificationType(juce::Justification::centred);
    tunerLabel.setText("TUNER --", juce::dontSendNotification);
    addAndMakeVisible(tunerLabel);

    for (auto* meter : { &inputMeterLabel, &outputMeterLabel })
    {
        meter->setColour(juce::Label::textColourId, juce::Colours::white);
        meter->setJustificationType(juce::Justification::centred);
        addAndMakeVisible(*meter);
    }

    addAndMakeVisible(deviceSelector);
    deviceSelector.setVisible(false);

    addAndMakeVisible(advancedPanel);
    advancedPanel.setVisible(false);

    oversamplingLabel.setText("OVERSAMPLING", juce::dontSendNotification);
    oversamplingLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    advancedPanel.addAndMakeVisible(oversamplingLabel);

    oversamplingBox.addItem("OFF", 1);
    oversamplingBox.addItem("2x", 2);
    oversamplingBox.addItem("4x", 3);
    oversamplingBox.setSelectedId(2, juce::dontSendNotification);
    oversamplingBox.onChange = [this] { pushParameters(); };
    advancedPanel.addAndMakeVisible(oversamplingBox);

    tunerReferenceLabel.setText("TUNER A4", juce::dontSendNotification);
    tunerReferenceLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    advancedPanel.addAndMakeVisible(tunerReferenceLabel);

    tunerReferenceSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tunerReferenceSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 24);
    tunerReferenceSlider.setRange(430.0, 450.0, 0.1);
    tunerReferenceSlider.setValue(440.0, juce::dontSendNotification);
    tunerReferenceSlider.onValueChange = [this]
    {
        tuner.setReferenceA4(tunerReferenceSlider.getValue());
    };
    advancedPanel.addAndMakeVisible(tunerReferenceSlider);

    limiterThresholdLabel.setText("LIMITER", juce::dontSendNotification);
    limiterThresholdLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    advancedPanel.addAndMakeVisible(limiterThresholdLabel);

    limiterThresholdSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    limiterThresholdSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 70, 24);
    limiterThresholdSlider.setRange(-12.0, -0.1, 0.1);
    limiterThresholdSlider.setValue(-1.0, juce::dontSendNotification);
    limiterThresholdSlider.onValueChange = [this]
    {
        processor.setLimiterThresholdDb(static_cast<float>(limiterThresholdSlider.getValue()));
    };
    advancedPanel.addAndMakeVisible(limiterThresholdSlider);

    advancedPanel.addAndMakeVisible(loadIrButton);
    loadIrButton.onClick = [this] { chooseUserIr(); };

    irStatusLabel.setColour(juce::Label::textColourId, juce::Colours::silver);
    irStatusLabel.setJustificationType(juce::Justification::centredLeft);
    irStatusLabel.setText("IR: none", juce::dontSendNotification);
    advancedPanel.addAndMakeVisible(irStatusLabel);

    diagnosticsLabel.setColour(juce::Label::textColourId, juce::Colours::silver);
    diagnosticsLabel.setJustificationType(juce::Justification::topLeft);
    advancedPanel.addAndMakeVisible(diagnosticsLabel);

    refreshPresetList();

    setAudioChannels(1, 2);
    AudioSettings::restore(deviceManager);

    pushParameters();
    stateA = captureState();
    stateB = stateA;

    startTimerHz(20);
}

MainComponent::~MainComponent()
{
    stopTimer();
    AudioSettings::save(deviceManager);
    shutdownAudio();
}

void MainComponent::configureKnob(juce::Slider& slider,
                                  juce::Label& label,
                                  const juce::String& name,
                                  double minimum,
                                  double maximum,
                                  double initial,
                                  double interval)
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
    currentSampleRate.store(sampleRate);
    tuner.setSampleRate(sampleRate);
    diagnostics.reset();

    const auto outputChannels = deviceManager.getCurrentAudioDevice() != nullptr
        ? juce::jmax(1, deviceManager.getCurrentAudioDevice()
                            ->getActiveOutputChannels().countNumberOfSetBits())
        : 2;

    processor.prepare(sampleRate, samplesPerBlockExpected, outputChannels);
    pushParameters();
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    const auto startedMs = juce::Time::getMillisecondCounterHiRes();

    if (bufferToFill.buffer == nullptr || bufferToFill.numSamples <= 0)
        return;

    if (bufferToFill.buffer->getNumChannels() > 0)
    {
        tuner.pushSamples(bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample),
                          bufferToFill.numSamples);
    }

    processor.processBlock(*bufferToFill.buffer,
                           bufferToFill.startSample,
                           bufferToFill.numSamples);

    const auto elapsedSeconds =
        (juce::Time::getMillisecondCounterHiRes() - startedMs) * 0.001;
    const auto rate = currentSampleRate.load();
    const auto deadlineSeconds = rate > 0.0
        ? static_cast<double>(bufferToFill.numSamples) / rate
        : 0.0;

    diagnostics.recordCallback(elapsedSeconds, deadlineSeconds);
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
    processor.setLimiterThresholdDb(static_cast<float>(limiterThresholdSlider.getValue()));

    processor.setOversamplingMode(static_cast<BassAmpProcessor::OversamplingMode>(
        juce::jlimit(0, 2, oversamplingBox.getSelectedId() - 1)));

    tuner.setReferenceA4(tunerReferenceSlider.getValue());
}

PresetState MainComponent::captureState() const
{
    PresetState state;
    state.inputGainDb = static_cast<float>(inputGainKnob.getValue());
    state.gateThresholdDb = static_cast<float>(gateKnob.getValue());
    state.compAmount = static_cast<float>(compKnob.getValue());
    state.drive = static_cast<float>(driveKnob.getValue());
    state.character = static_cast<float>(characterKnob.getValue());
    state.lowClean = static_cast<float>(lowCleanKnob.getValue());

    state.bassDb = static_cast<float>(bassKnob.getValue());
    state.lowMidDb = static_cast<float>(lowMidKnob.getValue());
    state.highMidDb = static_cast<float>(highMidKnob.getValue());
    state.trebleDb = static_cast<float>(trebleKnob.getValue());
    state.masterDb = static_cast<float>(masterKnob.getValue());

    state.voicing = vintageButton.getToggleState()
        ? 0
        : aggressiveButton.getToggleState() ? 2 : 1;

    state.cabinet = juce::jlimit(0, 4, cabinetBox.getSelectedId() - 1);
    state.oversampling = juce::jlimit(0, 2, oversamplingBox.getSelectedId() - 1);
    state.tunerReferenceA4 = static_cast<float>(tunerReferenceSlider.getValue());

    if (currentUserIrFile.existsAsFile())
        state.userIrPath = currentUserIrFile.getFullPathName();

    return state;
}

void MainComponent::applyState(const PresetState& state)
{
    inputGainKnob.setValue(state.inputGainDb, juce::dontSendNotification);
    gateKnob.setValue(state.gateThresholdDb, juce::dontSendNotification);
    compKnob.setValue(state.compAmount, juce::dontSendNotification);
    driveKnob.setValue(state.drive, juce::dontSendNotification);
    characterKnob.setValue(state.character, juce::dontSendNotification);
    lowCleanKnob.setValue(state.lowClean, juce::dontSendNotification);

    bassKnob.setValue(state.bassDb, juce::dontSendNotification);
    lowMidKnob.setValue(state.lowMidDb, juce::dontSendNotification);
    highMidKnob.setValue(state.highMidDb, juce::dontSendNotification);
    trebleKnob.setValue(state.trebleDb, juce::dontSendNotification);
    masterKnob.setValue(state.masterDb, juce::dontSendNotification);

    updateVoicingButtons(state.voicing);
    processor.setVoicing(static_cast<BassAmpProcessor::Voicing>(juce::jlimit(0, 2, state.voicing)));

    auto requestedCabinet = juce::jlimit(0, 4, state.cabinet);
    if (requestedCabinet == static_cast<int>(BassAmpProcessor::Cabinet::userIr))
    {
        const juce::File irFile(state.userIrPath);
        if (irFile.existsAsFile() && processor.loadUserImpulseResponse(irFile))
        {
            currentUserIrFile = irFile;
            irStatusLabel.setText("IR: " + irFile.getFileName(), juce::dontSendNotification);
        }
        else
        {
            requestedCabinet = 0;
        }
    }

    cabinetBox.setSelectedId(requestedCabinet + 1, juce::dontSendNotification);
    processor.setCabinet(static_cast<BassAmpProcessor::Cabinet>(requestedCabinet));

    oversamplingBox.setSelectedId(juce::jlimit(0, 2, state.oversampling) + 1,
                                  juce::dontSendNotification);
    tunerReferenceSlider.setValue(state.tunerReferenceA4, juce::dontSendNotification);

    pushParameters();
}

void MainComponent::updateVoicingButtons(int voicingIndex)
{
    vintageButton.setToggleState(voicingIndex == 0, juce::dontSendNotification);
    modernButton.setToggleState(voicingIndex == 1, juce::dontSendNotification);
    aggressiveButton.setToggleState(voicingIndex == 2, juce::dontSendNotification);
}

void MainComponent::switchAB(bool useB)
{
    if (useB == usingB)
        return;

    if (usingB)
    {
        stateB = captureState();
        applyState(stateA);
    }
    else
    {
        stateA = captureState();
        applyState(stateB);
    }

    usingB = useB;
    aButton.setToggleState(!usingB, juce::dontSendNotification);
    bButton.setToggleState(usingB, juce::dontSendNotification);
}

void MainComponent::refreshPresetList()
{
    presetBox.clear(juce::dontSendNotification);

    const auto& factoryNames = presetManager.getFactoryNames();
    for (int i = 0; i < factoryNames.size(); ++i)
        presetBox.addItem(factoryNames[i], i + 1);

    userPresetFiles = presetManager.getUserPresetFiles();
    if (!userPresetFiles.isEmpty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading("User");
        for (int i = 0; i < userPresetFiles.size(); ++i)
            presetBox.addItem(userPresetFiles.getReference(i).getFileNameWithoutExtension(), 100 + i);
    }
}

void MainComponent::loadSelectedPreset()
{
    const auto id = presetBox.getSelectedId();
    if (id <= 0)
        return;

    if (id >= 1 && id <= presetManager.getFactoryNames().size())
    {
        applyState(presetManager.getFactoryPreset(id - 1));
        return;
    }

    if (id >= 100)
    {
        const auto index = id - 100;
        if (!juce::isPositiveAndBelow(index, userPresetFiles.size()))
            return;

        PresetState loaded;
        juce::String error;
        if (!presetManager.loadFromFile(userPresetFiles.getReference(index), loaded, error))
        {
            showError("Preset", error);
            return;
        }

        applyState(loaded);
    }
}

void MainComponent::savePresetAs()
{
    activeChooser = std::make_unique<juce::FileChooser>(
        "Save Bass_amp preset",
        presetManager.getSuggestedPresetFile("My Bass Preset"),
        "*.json");

    const auto flags = juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::canSelectFiles
        | juce::FileBrowserComponent::warnAboutOverwriting;

    activeChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        auto file = chooser.getResult();
        if (file == juce::File())
            return;

        file = file.withFileExtension(".json");

        juce::String error;
        if (!presetManager.saveToFile(file, captureState(), error))
        {
            showError("Preset", error);
            return;
        }

        refreshPresetList();
        statusLabel.setText("Preset saved: " + file.getFileNameWithoutExtension(),
                            juce::dontSendNotification);
    });
}

void MainComponent::deleteSelectedPreset()
{
    const auto id = presetBox.getSelectedId();
    if (id < 100)
    {
        showError("Preset", "Factory Presetは削除できません。");
        return;
    }

    const auto index = id - 100;
    if (!juce::isPositiveAndBelow(index, userPresetFiles.size()))
        return;

    const auto file = userPresetFiles.getReference(index);
    if (!file.deleteFile())
    {
        showError("Preset", "プリセットを削除できませんでした: " + file.getFullPathName());
        return;
    }

    presetBox.setSelectedId(0, juce::dontSendNotification);
    refreshPresetList();
}

void MainComponent::chooseUserIr()
{
    activeChooser = std::make_unique<juce::FileChooser>(
        "Load cabinet impulse response",
        currentUserIrFile.existsAsFile() ? currentUserIrFile : juce::File(),
        "*.wav;*.aif;*.aiff");

    const auto flags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;

    activeChooser->launchAsync(flags, [this](const juce::FileChooser& chooser)
    {
        const auto file = chooser.getResult();
        if (file == juce::File())
            return;

        if (!processor.loadUserImpulseResponse(file))
        {
            showError("USER IR", "IRファイルを読み込めませんでした。WAV/AIFFを確認してください。");
            return;
        }

        currentUserIrFile = file;
        irStatusLabel.setText("IR: " + file.getFileName(), juce::dontSendNotification);
        cabinetBox.setSelectedId(5, juce::sendNotificationAsync);
    });
}

void MainComponent::showError(const juce::String& title, const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,
                                           title,
                                           message);
}

void MainComponent::setAudioPanelVisible(bool visible)
{
    audioPanelVisible = visible;
    if (visible)
        setAdvancedPanelVisible(false);

    deviceSelector.setVisible(audioPanelVisible);
    audioSetupButton.setToggleState(audioPanelVisible, juce::dontSendNotification);
    resized();
    repaint();
}

void MainComponent::setAdvancedPanelVisible(bool visible)
{
    advancedPanelVisible = visible;
    if (visible)
    {
        audioPanelVisible = false;
        deviceSelector.setVisible(false);
        audioSetupButton.setToggleState(false, juce::dontSendNotification);
    }

    updateAdvancedVisibility();
    resized();
    repaint();
}

void MainComponent::updateAdvancedVisibility()
{
    advancedPanel.setVisible(advancedPanelVisible);
    advancedButton.setToggleState(advancedPanelVisible, juce::dontSendNotification);
}

void MainComponent::timerCallback()
{
    const auto inDb = juce::Decibels::gainToDecibels(processor.getInputPeak(), -100.0f);
    const auto outDb = juce::Decibels::gainToDecibels(processor.getOutputPeak(), -100.0f);

    if (processor.getInputPeak() >= 0.99f)
        inputClipHoldTicks = 20;
    else if (inputClipHoldTicks > 0)
        --inputClipHoldTicks;

    if (processor.getOutputPeak() >= 0.99f)
        outputClipHoldTicks = 20;
    else if (outputClipHoldTicks > 0)
        --outputClipHoldTicks;

    inputMeterLabel.setText("IN " + juce::String(inDb, 1) + " dB"
                                + (inputClipHoldTicks > 0 ? "  CLIP" : ""),
                            juce::dontSendNotification);
    outputMeterLabel.setText("OUT " + juce::String(outDb, 1) + " dB"
                                 + (outputClipHoldTicks > 0 ? "  CLIP" : ""),
                             juce::dontSendNotification);

    inputMeterLabel.setColour(juce::Label::textColourId,
                              inputClipHoldTicks > 0 ? juce::Colours::orangeRed : juce::Colours::white);
    outputMeterLabel.setColour(juce::Label::textColourId,
                               outputClipHoldTicks > 0 ? juce::Colours::orangeRed : juce::Colours::white);

    const auto tuning = tuner.analyse();
    if (tuning.valid)
    {
        const auto sign = tuning.cents >= 0.0 ? "+" : "";
        tunerLabel.setText("TUNER  " + tuning.noteName
                               + "  " + sign + juce::String(tuning.cents, 1) + " cents"
                               + "  " + juce::String(tuning.frequencyHz, 2) + " Hz",
                           juce::dontSendNotification);
    }
    else
    {
        tunerLabel.setText("TUNER  --", juce::dontSendNotification);
    }

    if (auto* device = deviceManager.getCurrentAudioDevice())
    {
        statusLabel.setText(device->getTypeName() + " | " + device->getName()
                                + " | " + juce::String(device->getCurrentSampleRate() / 1000.0, 1) + " kHz"
                                + " | " + juce::String(device->getCurrentBufferSizeSamples()) + " samples"
                                + " | CPU " + juce::String(deviceManager.getCpuUsage() * 100.0, 1) + "%",
                            juce::dontSendNotification);

        diagnosticsLabel.setText(
            "AUDIO DIAGNOSTICS\n\n"
            "Driver: " + device->getTypeName() + "\n"
            "Device: " + device->getName() + "\n"
            "Sample rate: " + juce::String(device->getCurrentSampleRate(), 0) + " Hz\n"
            "Buffer: " + juce::String(device->getCurrentBufferSizeSamples()) + " samples\n"
            "Input latency: " + juce::String(device->getInputLatencyInSamples()) + " samples\n"
            "Output latency: " + juce::String(device->getOutputLatencyInSamples()) + " samples\n"
            "DSP latency: " + juce::String(processor.getLatencySamples()) + " samples\n"
            "CPU: " + juce::String(deviceManager.getCpuUsage() * 100.0, 1) + "%\n"
            "Peak callback: " + juce::String(diagnostics.getCallbackPeakMs(), 3) + " ms\n"
            "Deadline misses: " + juce::String(diagnostics.getDeadlineMissCount()) + "\n"
            "IR: " + (currentUserIrFile.existsAsFile() ? currentUserIrFile.getFileName() : "none"),
            juce::dontSendNotification);
    }
    else
    {
        statusLabel.setText("No audio device", juce::dontSendNotification);
        diagnosticsLabel.setText("AUDIO DIAGNOSTICS\n\nNo audio device",
                                 juce::dontSendNotification);
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
    g.drawText("BASS_amp v1.0", 18, 9, 200, 38, juce::Justification::centredLeft);

    g.setColour(juce::Colour::fromRGB(38, 43, 50));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(12.0f).withTrimmedTop(56.0f),
                           10.0f,
                           1.0f);

    if (advancedPanelVisible)
    {
        g.setColour(juce::Colour::fromRGB(24, 28, 34));
        g.fillRoundedRectangle(advancedPanel.getBounds().toFloat(), 8.0f);
        g.setColour(juce::Colour::fromRGB(70, 200, 170));
        g.drawRoundedRectangle(advancedPanel.getBounds().toFloat(), 8.0f, 1.0f);
    }
}

void MainComponent::placeKnob(juce::Rectangle<int> area,
                              juce::Label& label,
                              juce::Slider& slider)
{
    label.setBounds(area.removeFromTop(24));
    slider.setBounds(area.reduced(4));
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds().reduced(14);
    auto header = bounds.removeFromTop(44);

    header.removeFromLeft(205);
    inputMeterLabel.setBounds(header.removeFromLeft(120));
    outputMeterLabel.setBounds(header.removeFromLeft(130));

    auto rightHeader = header.removeFromRight(360);
    bypassButton.setBounds(rightHeader.removeFromRight(84).reduced(3));
    advancedButton.setBounds(rightHeader.removeFromRight(112).reduced(3));
    audioSetupButton.setBounds(rightHeader.removeFromRight(132).reduced(3));

    tunerLabel.setBounds(header.removeFromRight(300).reduced(2));
    statusLabel.setBounds(header.reduced(4));

    bounds.removeFromTop(18);
    auto voiceRow = bounds.removeFromTop(46);
    const auto voiceWidth = juce::jmin(160, voiceRow.getWidth() / 3);
    auto voiceGroup = voiceRow.withSizeKeepingCentre(voiceWidth * 3, 40);
    vintageButton.setBounds(voiceGroup.removeFromLeft(voiceWidth).reduced(3));
    modernButton.setBounds(voiceGroup.removeFromLeft(voiceWidth).reduced(3));
    aggressiveButton.setBounds(voiceGroup.removeFromLeft(voiceWidth).reduced(3));

    bounds.removeFromTop(10);
    auto row1 = bounds.removeFromTop(205);
    const auto cellWidth1 = row1.getWidth() / 6;
    placeKnob(row1.removeFromLeft(cellWidth1), inputGainLabel, inputGainKnob);
    placeKnob(row1.removeFromLeft(cellWidth1), gateLabel, gateKnob);
    placeKnob(row1.removeFromLeft(cellWidth1), compLabel, compKnob);
    placeKnob(row1.removeFromLeft(cellWidth1), driveLabel, driveKnob);
    placeKnob(row1.removeFromLeft(cellWidth1), characterLabel, characterKnob);
    placeKnob(row1, lowCleanLabel, lowCleanKnob);

    bounds.removeFromTop(6);
    auto row2 = bounds.removeFromTop(205);
    const auto cellWidth2 = row2.getWidth() / 6;
    placeKnob(row2.removeFromLeft(cellWidth2), bassLabel, bassKnob);
    placeKnob(row2.removeFromLeft(cellWidth2), lowMidLabel, lowMidKnob);
    placeKnob(row2.removeFromLeft(cellWidth2), highMidLabel, highMidKnob);
    placeKnob(row2.removeFromLeft(cellWidth2), trebleLabel, trebleKnob);
    placeKnob(row2.removeFromLeft(cellWidth2), masterLabel, masterKnob);

    auto cabArea = row2.reduced(10);
    cabinetLabel.setBounds(cabArea.removeFromTop(28));
    cabinetBox.setBounds(cabArea.removeFromTop(34));

    bounds.removeFromTop(8);
    auto presetRow = bounds.removeFromTop(54);
    presetLabel.setBounds(presetRow.removeFromLeft(70));
    presetBox.setBounds(presetRow.removeFromLeft(260).reduced(3));
    savePresetButton.setBounds(presetRow.removeFromLeft(92).reduced(3));
    deletePresetButton.setBounds(presetRow.removeFromLeft(82).reduced(3));
    presetRow.removeFromLeft(14);
    aButton.setBounds(presetRow.removeFromLeft(46).reduced(3));
    bButton.setBounds(presetRow.removeFromLeft(46).reduced(3));

    if (audioPanelVisible)
    {
        auto selectorBounds = getLocalBounds().reduced(22);
        selectorBounds.removeFromLeft(juce::jmax(0, selectorBounds.getWidth() - 470));
        selectorBounds.removeFromTop(54);
        deviceSelector.toFront(false);
        deviceSelector.setBounds(selectorBounds);
    }

    auto advancedBounds = getLocalBounds().reduced(22);
    advancedBounds.removeFromLeft(juce::jmax(0, advancedBounds.getWidth() - 430));
    advancedBounds.removeFromTop(54);
    advancedPanel.setBounds(advancedBounds);

    auto panel = advancedPanel.getLocalBounds().reduced(18);
    oversamplingLabel.setBounds(panel.removeFromTop(24));
    oversamplingBox.setBounds(panel.removeFromTop(34));
    panel.removeFromTop(14);

    tunerReferenceLabel.setBounds(panel.removeFromTop(24));
    tunerReferenceSlider.setBounds(panel.removeFromTop(36));
    panel.removeFromTop(10);

    limiterThresholdLabel.setBounds(panel.removeFromTop(24));
    limiterThresholdSlider.setBounds(panel.removeFromTop(36));
    panel.removeFromTop(14);

    loadIrButton.setBounds(panel.removeFromTop(34));
    irStatusLabel.setBounds(panel.removeFromTop(30));
    panel.removeFromTop(14);
    diagnosticsLabel.setBounds(panel);
}
