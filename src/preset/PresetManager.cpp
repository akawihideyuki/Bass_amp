#include "PresetManager.h"

namespace
{
float readFloat(const juce::var& object, const juce::Identifier& key, float fallback)
{
    if (auto* dynamic = object.getDynamicObject())
    {
        const auto value = dynamic->getProperty(key);
        if (!value.isVoid())
            return static_cast<float>(static_cast<double>(value));
    }

    return fallback;
}

int readInt(const juce::var& object, const juce::Identifier& key, int fallback)
{
    if (auto* dynamic = object.getDynamicObject())
    {
        const auto value = dynamic->getProperty(key);
        if (!value.isVoid())
            return static_cast<int>(value);
    }

    return fallback;
}

juce::String readString(const juce::var& object, const juce::Identifier& key, const juce::String& fallback)
{
    if (auto* dynamic = object.getDynamicObject())
    {
        const auto value = dynamic->getProperty(key);
        if (!value.isVoid())
            return value.toString();
    }

    return fallback;
}
}

PresetManager::PresetManager()
{
    getPresetDirectory().createDirectory();
}

PresetState PresetManager::getFactoryPreset(int index) const
{
    PresetState state;

    switch (juce::jlimit(0, factoryNames.size() - 1, index))
    {
        case 0:
            state.compAmount = 24.0f;
            state.drive = 8.0f;
            state.character = 42.0f;
            state.lowClean = 85.0f;
            state.bassDb = 1.0f;
            state.highMidDb = 1.5f;
            state.cabinet = 0;
            state.voicing = 1;
            break;

        case 1:
            state.compAmount = 38.0f;
            state.drive = 34.0f;
            state.character = 35.0f;
            state.lowClean = 72.0f;
            state.bassDb = 2.5f;
            state.lowMidDb = 2.0f;
            state.highMidDb = -1.5f;
            state.trebleDb = -2.0f;
            state.cabinet = 1;
            state.voicing = 0;
            break;

        case 2:
            state.compAmount = 42.0f;
            state.drive = 18.0f;
            state.character = 62.0f;
            state.lowClean = 76.0f;
            state.bassDb = 1.5f;
            state.lowMidDb = -1.0f;
            state.highMidDb = 2.5f;
            state.trebleDb = 2.0f;
            state.cabinet = 2;
            state.voicing = 1;
            break;

        case 3:
            state.compAmount = 34.0f;
            state.drive = 56.0f;
            state.character = 58.0f;
            state.lowClean = 68.0f;
            state.lowMidDb = 2.0f;
            state.highMidDb = 3.0f;
            state.trebleDb = 0.5f;
            state.cabinet = 2;
            state.voicing = 2;
            break;

        case 4:
        default:
            state.compAmount = 30.0f;
            state.drive = 72.0f;
            state.character = 78.0f;
            state.lowClean = 62.0f;
            state.bassDb = 1.0f;
            state.lowMidDb = -1.0f;
            state.highMidDb = 4.0f;
            state.trebleDb = 2.5f;
            state.cabinet = 3;
            state.voicing = 2;
            state.oversampling = 2;
            break;
    }

    return state;
}

juce::Array<juce::File> PresetManager::getUserPresetFiles() const
{
    juce::Array<juce::File> result;
    getPresetDirectory().findChildFiles(result, juce::File::findFiles, false, "*.json");
    return result;
}

bool PresetManager::saveToFile(const juce::File& file, const PresetState& state, juce::String& error) const
{
    if (!file.getParentDirectory().createDirectory())
    {
        error = "プリセット保存先フォルダーを作成できませんでした。";
        return false;
    }

    const auto text = toJson(state);
    if (!file.replaceWithText(text))
    {
        error = "プリセットを書き込めませんでした: " + file.getFullPathName();
        return false;
    }

    error.clear();
    return true;
}

bool PresetManager::loadFromFile(const juce::File& file, PresetState& state, juce::String& error) const
{
    if (!file.existsAsFile())
    {
        error = "プリセットファイルが見つかりません: " + file.getFullPathName();
        return false;
    }

    return fromJson(file.loadFileAsString(), state, error);
}

juce::File PresetManager::getPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Bass_amp")
        .getChildFile("Presets");
}

juce::File PresetManager::getSuggestedPresetFile(const juce::String& baseName) const
{
    auto safe = juce::File::createLegalFileName(baseName.trim());
    if (safe.isEmpty())
        safe = "Bass_amp preset";

    return getPresetDirectory().getChildFile(safe).withFileExtension(".json");
}

juce::String PresetManager::toJson(const PresetState& state)
{
    auto object = std::make_unique<juce::DynamicObject>();
    object->setProperty("presetVersion", state.presetVersion);
    object->setProperty("inputGainDb", state.inputGainDb);
    object->setProperty("gateThresholdDb", state.gateThresholdDb);
    object->setProperty("compAmount", state.compAmount);
    object->setProperty("drive", state.drive);
    object->setProperty("character", state.character);
    object->setProperty("lowClean", state.lowClean);
    object->setProperty("bassDb", state.bassDb);
    object->setProperty("lowMidDb", state.lowMidDb);
    object->setProperty("highMidDb", state.highMidDb);
    object->setProperty("trebleDb", state.trebleDb);
    object->setProperty("masterDb", state.masterDb);
    object->setProperty("voicing", state.voicing);
    object->setProperty("cabinet", state.cabinet);
    object->setProperty("oversampling", state.oversampling);
    object->setProperty("tunerReferenceA4", state.tunerReferenceA4);
    object->setProperty("userIrPath", state.userIrPath);

    return juce::JSON::toString(juce::var(object.release()), true);
}

bool PresetManager::fromJson(const juce::String& json, PresetState& state, juce::String& error)
{
    const auto parsed = juce::JSON::parse(json);
    if (!parsed.isObject())
    {
        error = "プリセットJSONが不正です。";
        return false;
    }

    PresetState loaded;
    loaded.presetVersion = readInt(parsed, "presetVersion", 1);
    loaded.inputGainDb = juce::jlimit(-24.0f, 24.0f, readFloat(parsed, "inputGainDb", loaded.inputGainDb));
    loaded.gateThresholdDb = juce::jlimit(-90.0f, -15.0f, readFloat(parsed, "gateThresholdDb", loaded.gateThresholdDb));
    loaded.compAmount = juce::jlimit(0.0f, 100.0f, readFloat(parsed, "compAmount", loaded.compAmount));
    loaded.drive = juce::jlimit(0.0f, 100.0f, readFloat(parsed, "drive", loaded.drive));
    loaded.character = juce::jlimit(0.0f, 100.0f, readFloat(parsed, "character", loaded.character));
    loaded.lowClean = juce::jlimit(0.0f, 100.0f, readFloat(parsed, "lowClean", loaded.lowClean));
    loaded.bassDb = juce::jlimit(-12.0f, 12.0f, readFloat(parsed, "bassDb", loaded.bassDb));
    loaded.lowMidDb = juce::jlimit(-12.0f, 12.0f, readFloat(parsed, "lowMidDb", loaded.lowMidDb));
    loaded.highMidDb = juce::jlimit(-12.0f, 12.0f, readFloat(parsed, "highMidDb", loaded.highMidDb));
    loaded.trebleDb = juce::jlimit(-12.0f, 12.0f, readFloat(parsed, "trebleDb", loaded.trebleDb));
    loaded.masterDb = juce::jlimit(-60.0f, 6.0f, readFloat(parsed, "masterDb", loaded.masterDb));
    loaded.voicing = juce::jlimit(0, 2, readInt(parsed, "voicing", loaded.voicing));
    loaded.cabinet = juce::jlimit(0, 4, readInt(parsed, "cabinet", loaded.cabinet));
    loaded.oversampling = juce::jlimit(0, 2, readInt(parsed, "oversampling", loaded.oversampling));
    loaded.tunerReferenceA4 = juce::jlimit(430.0f, 450.0f, readFloat(parsed, "tunerReferenceA4", loaded.tunerReferenceA4));
    loaded.userIrPath = readString(parsed, "userIrPath", {});

    state = loaded;
    error.clear();
    return true;
}
