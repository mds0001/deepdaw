#pragma once

#include <JuceHeader.h>
#include "TrackListComponent.h"
#include "ClipComponent.h"

// Draws the timeline ruler (bars) and one lane per track, kept vertically
// aligned with the track list rows. Editing/clip support arrives in a later
// Phase 1 increment; for now it provides the visual arrangement surface.
class TimelineComponent : public juce::Component,
                          public juce::FileDragAndDropTarget
{
public:
    TimelineComponent(TrackListComponent& list, juce::AudioFormatManager& formatManager);
    ~TimelineComponent() override = default;

    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override; // playhead, on top of clips
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // FileDragAndDropTarget: drop audio files onto a lane to import them.
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragMove(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // Resize the component to fit the current track count plus the ruler, so a
    // host Viewport gets the correct scroll extents. Call after tracks change.
    void updateContentSize();

    // Recreate clip components from the track model (call when clips/tracks
    // change); layoutClips repositions existing ones (call on zoom/resize).
    void rebuildClips();
    void layoutClips();

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
    // Fired when an audio file is dropped on an audio lane (trackId, start beat).
    std::function<void(int trackId, double startBeat, const juce::File&)> onFileDropped;

    static constexpr int rulerHeight = 32;
    static constexpr int basePixelsPerBar = 80;
    static constexpr int numBars = 64;
    static constexpr double minZoom = 0.25;
    static constexpr double maxZoom = 8.0;

private:
    int playheadX() const;

    int trackIndexAt(int y) const;

    // Clip drag (move), coordinated here because the scale + model live here.
    void clipDragStart(ClipComponent*, int mouseXInTimeline);
    void clipDrag(ClipComponent*, int mouseXInTimeline);
    void clipDragEnd(ClipComponent*);

    TrackListComponent& trackList;
    juce::AudioFormatManager& formatManager;
    juce::AudioThumbnailCache thumbnailCache{ 64 };
    double playheadBeats = 0.0;
    double zoom = 1.0;
    int dropTargetTrack = -1; // lane highlighted during a file drag-over
    std::vector<std::unique_ptr<ClipComponent>> clipComponents;

    ClipComponent* draggedClip = nullptr;
    int dragClipStartX = 0;
    int dragMouseStartX = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineComponent)
};