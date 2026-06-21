#pragma once

#include <JuceHeader.h>
#include "TransportComponent.h"
#include "TrackListComponent.h"
#include "TimelineComponent.h"

class MainComponent : public juce::Component,
                      public juce::MenuBarModel
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
    // Viewport that reports its vertical scroll offset, so the track list and
    // timeline can be kept row-aligned as either one scrolls.
    class SyncViewport : public juce::Viewport
    {
    public:
        std::function<void(int)> onVerticalScroll;

        void visibleAreaChanged(const juce::Rectangle<int>& newVisibleArea) override
        {
            if (onVerticalScroll)
                onVerticalScroll(newVisibleArea.getY());
        }
    };

    void addTrack(TrackType type);
    void handleTracksChanged();
    void syncVerticalScroll(int y, bool fromTrackList);
    void updateContentBounds();

    // Project file operations (wired to the File menu).
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void writeProjectTo(const juce::File&);
    void setWindowTitle(const juce::String& projectName);

    juce::MenuBarComponent menuBar;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::File currentProjectFile;

    std::unique_ptr<TransportComponent> transport;
    juce::AudioDeviceManager deviceManager;

    juce::TextButton addAudioButton{"+ Audio Track"};
    juce::TextButton addMidiButton{"+ MIDI Track"};
    juce::Label tracksHeaderLabel{"tracksHeader", "TRACKS"};

    // Declaration order matters: timeline holds a reference to trackList, so
    // trackList must be constructed (and destroyed) around it correctly.
    TrackListComponent trackList;
    TimelineComponent timeline{trackList};

    SyncViewport trackListViewport;
    SyncViewport timelineViewport;

    bool syncingScroll = false;

    static constexpr int trackListWidth = 280;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
