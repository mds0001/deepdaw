#pragma once

#include <JuceHeader.h>

// One audio clip placed on a track lane. Position and length are stored in
// beats so they stay put when the tempo or zoom changes. The source file path
// is absolute for now (relative-path portability is future work).
struct Clip
{
    juce::String name;
    juce::String filePath;
    double startBeat = 0.0;
    double lengthBeats = 0.0;
};
