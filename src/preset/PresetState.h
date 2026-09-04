#pragma once

#include <JuceHeader.h>

struct PresetState
{
    int presetVersion = 1;

    float inputGainDb = 0.0f;
    float gateThresholdDb = -65.0f;
    float compAmount = 30.0f;
    float drive = 25.0f;
    float character = 50.0f;
    float lowClean = 65.0f;

    float bassDb = 0.0f;
    float lowMidDb = 0.0f;
    float highMidDb = 0.0f;
    float trebleDb = 0.0f;
    float masterDb = -6.0f;

    int voicing = 1;
    int cabinet = 0;
    int oversampling = 1;

    float tunerReferenceA4 = 440.0f;
    juce::String userIrPath;
};
