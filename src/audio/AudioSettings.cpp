#include "AudioSettings.h"

juce::File AudioSettings::getSettingsFile()
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Bass_amp")
        .getChildFile("audio-device.xml");
}

void AudioSettings::restore(juce::AudioDeviceManager& manager)
{
    const auto file = getSettingsFile();
    if (!file.existsAsFile())
        return;

    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr)
        return;

    const auto error = manager.initialise(1, 2, xml.get(), true);
    juce::ignoreUnused(error);
}

void AudioSettings::save(juce::AudioDeviceManager& manager)
{
    auto xml = manager.createStateXml();
    if (xml == nullptr)
        return;

    const auto file = getSettingsFile();
    file.getParentDirectory().createDirectory();
    file.replaceWithText(xml->toString());
}
