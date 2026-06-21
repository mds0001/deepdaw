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

    int getNumTracks() const           { return (int) tracks.size(); }
    static constexpr int rowHeight = 64;

    const std::vector<std::unique_ptr<Track>>& getTracks() const { return tracks; }

    // Notified whenever tracks are added, removed, or edited so the host can
    // resize/repaint the timeline alongside the list.
    std::function<void()> onTracksChanged;

private:
    void rebuildHeaders();
    void notifyChanged();

    std::vector<std::unique_ptr<Track>> tracks;
    std::vector<std::unique_ptr<TrackHeaderComponent>> headers;
    int nextId = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackListComponent)
};