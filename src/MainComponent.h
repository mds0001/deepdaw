#pragma once

#include <JuceHeader.h>
#include "TransportComponent.h"
#include "TrackListComponent.h"
#include "TimelineComponent.h"
#include "TimelineRulerComponent.h"

class MainComponent : public juce::Component,
                      public juce::MenuBarModel,
                      private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // MenuBarModel
    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

private:
    // Timer: advances the playhead during playback.
    void timerCallback() override;
    void setPlayheadBeats(double beats);
    // Viewport that reports its visible area, so the track list (vertical) and
    // ruler (horizontal) can be kept in sync as the timeline scrolls.
    class SyncViewport : public juce::Viewport
    {
    public:
        std::function<void(juce::Rectangle<int>)> onVisibleAreaChanged;

        void visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea) override
        {
            if (onVisibleAreaChanged)
                onVisibleAreaChanged(newVisibleArea);
        }
    };

    void addTrack(TrackType type);
    void handleTracksChanged();
    void syncVerticalScroll(int y, bool fromTrackList);
    void updateContentBounds();

    // Zoom the timeline by factor, keeping anchorContentX under the same screen
    // position (cursor for Ctrl+wheel, viewport centre for the +/- buttons).
    void applyZoom(double factor, double anchorContentX);
    double timelineViewportCentreX() const;

    // Project file operations (wired to the File menu).
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void writeProjectTo(const juce::File&);
    void setWindowTitle(const juce::String& projectName);

    void importAudioForTrack(int trackId);
    void addImportedClip(int trackId, double startBeat, const juce::File& file);

    juce::MenuBarComponent menuBar;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File currentProjectFile;

    std::unique_ptr<TransportComponent> transport;
    juce::AudioDeviceManager deviceManager;
    juce::AudioSourcePlayer audioSourcePlayer; // pulls the transport's audio to the device
    juce::AudioFormatManager formatManager;    // reads audio files (durations, later playback)

    juce::TextButton addAudioButton{"+ Audio Track"};
    juce::TextButton addMidiButton{"+ MIDI Track"};
    juce::TextButton zoomInButton{"+"};
    juce::TextButton zoomOutButton{"-"};
    juce::Label tracksHeaderLabel{"tracksHeader", "TRACKS"};
    juce::Label statusBar{"statusBar"};

    // Declaration order matters: timeline references trackList, and ruler
    // references timeline, so each must be constructed after the one it uses.
    TrackListComponent trackList;
    TimelineComponent timeline{trackList, formatManager};
    TimelineRulerComponent ruler{timeline};

    SyncViewport trackListViewport;
    SyncViewport timelineViewport;

    bool syncingScroll = false;

    double playheadBeats = 0.0;
    double lastTickMs = 0.0;

    static constexpr int trackListWidth = 280;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
