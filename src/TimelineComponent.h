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
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // Resize the component to fit the current track count plus the ruler, so a
    // host Viewport gets the correct scroll extents. Call after tracks change.
    void updateContentSize();

    // Playhead position is expressed in beats from the start (4 beats per bar).
    void setPlayheadBeats(double beats);
    double getPlayheadBeats() const { return playheadBeats; }

    // Horizontal scale. zoom multiplies the base bar width; the helpers return
    // the current pixels-per-bar/beat used by every position calculation.
    void setZoom(double newZoom);
    double getZoom() const { return zoom; }
    double getPixelsPerBar() const { return basePixelsPerBar * zoom; }
    double getPixelsPerBeat() const { return getPixelsPerBar() / 4.0; }

    // Fired when the user clicks the timeline to move the playhead.
    std::function<void(double beats)> onSeek;
    // Fired on Ctrl+wheel; carries the wheel delta and the content x under the
    // cursor so the host can zoom about that anchor.
    std::function<void(float wheelDeltaY, float anchorContentX)> onZoomGesture;

    static constexpr int rulerHeight = 32;
    static constexpr int basePixelsPerBar = 80;
    static constexpr int numBars = 64;
    static constexpr double minZoom = 0.25;
    static constexpr double maxZoom = 8.0;

private:
    int playheadX() const;

    TrackListComponent& trackList;
    double playheadBeats = 0.0;
    double zoom = 1.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineComponent)
};