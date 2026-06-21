#pragma once

#include <JuceHeader.h>
#include "Track.h"
#include "TrackHeaderComponent.h"

// Owns the list of tracks and their header rows. Sizes itself to the total
// height of all rows so it can live inside a Viewport later.
class TrackListComponent : public juce::Component
{
public:
    TrackListComponent();
    ~TrackListComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    void addTrack(TrackType type);
    void removeTrack(int trackId);

    // Replace the entire track list (used when loading a project). Resets the
    // id counter past the highest loaded id so new tracks stay unique.
    void load(const std::vector<Track>& newTracks);
    void clear();

    // Add a clip to the track with the given id and notify listeners.
    void addClip(int trackId, const Clip& clip);
    // Move a clip's start position (in beats) / remove it; both notify.
    void setClipStart(int trackId, int clipIndex, double startBeat);
    void removeClip(int trackId, int clipIndex);

    int getNumTracks() const           { return (int) tracks.size(); }
    static constexpr int rowHeight = 64;

    const std::vector<std::unique_ptr<Track>>& getTracks() const { return tracks; }

    // Notified whenever tracks are added, removed, or edited so the host can
    // resize/repaint the timeline alongside the list.
    std::function<void()> onTracksChanged;
    // Fired when a track requests an audio import (host shows the file chooser).
    std::function<void(int trackId)> onImportAudioRequested;

private:
    void rebuildHeaders();
    void notifyChanged();

    // Row-reorder drag, driven by TrackHeaderComponent's mouse callbacks.
    void startDrag(TrackHeaderComponent* header, int mouseYInList);
    void dragRow(TrackHeaderComponent* header, int mouseYInList);
    void endDrag(TrackHeaderComponent* header);
    int indexOfHeader(const TrackHeaderComponent* header) const;
    void layoutHeaders(const TrackHeaderComponent* floating);

    std::vector<std::unique_ptr<Track>> tracks;
    std::vector<std::unique_ptr<TrackHeaderComponent>> headers;
    int nextId = 1;

    TrackHeaderComponent* draggedHeader = nullptr;
    int dragMouseStartY = 0;
    int dragHeaderStartY = 0;
    bool reorderHappened = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackListComponent)
};