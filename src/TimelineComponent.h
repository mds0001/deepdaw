#pragma once

#include <JuceHeader.h>
#include "TrackListComponent.h"

// Draws the timeline ruler (bars) and one lane per track, kept vertically
// aligned with the track list rows. Editing/clip support arrives in a later
// Phase 1 increment; for now it provides the visual arrangement surface.
class TimelineComponent : public juce::Component
{
public:
    explicit TimelineComponent(TrackListComponent& list);
    ~TimelineComponent() override = default;

    void paint(juce::Graphics&) override;

    // Resize the component to fit the current track count plus the ruler, so a
    // host Viewport gets the correct scroll extents. Call after tracks change.
    void updateContentSize();

    static constexpr int rulerHeight = 32;
    static constexpr int pixelsPerBar = 80;
    static constexpr int numBars = 64;

private:
    TrackListComponent& trackList;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineComponent)
};