#pragma once

#include <JuceHeader.h>

enum class TrackType
{
    audio,
    midi
};

// Plain data model for a single track. Owned by TrackListComponent via
// unique_ptr so that addresses remain stable while the list grows/shrinks.
struct Track
{
    int id = 0;
    juce::String name;
    TrackType type = TrackType::audio;
    juce::Colour colour = juce::Colour(0xffff7f00);
    bool muted = false;
    bool soloed = false;
    bool armed = false;
};