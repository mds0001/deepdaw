#include "MainComponent.h"
#include "LookAndFeel.h"

MainComponent::MainComponent()
{
    // Apply custom dark Pro Tools style LookAndFeel
    setLookAndFeel(&DeepDAWLookAndFeel::getInstance());

    // Initialize audio device with sensible real-time defaults
    deviceManager.initialiseWithDefaultDevices(2, 2);

    transport = std::make_unique<TransportComponent>(deviceManager);
    addAndMakeVisible(transport.get());

    addAudioButton.onClick = [this] { addTrack(TrackType::audio); };
    addMidiButton.onClick  = [this] { addTrack(TrackType::midi);  };
    addAndMakeVisible(addAudioButton);
    addAndMakeVisible(addMidiButton);

    tracksHeaderLabel.setJustificationType(juce::Justification::centredLeft);
    tracksHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a8a8a));
    tracksHeaderLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    addAndMakeVisible(tracksHeaderLabel);

    // Track list (left) and timeline (right) each live in a scrollable viewport.
    // The track list hides its own scrollbar; the timeline owns the shared one.
    trackListViewport.setViewedComponent(&trackList, false);
    trackListViewport.setScrollBarsShown(false, false);
    addAndMakeVisible(trackListViewport);

    timelineViewport.setViewedComponent(&timeline, false);
    timelineViewport.setScrollBarsShown(true, true);
    addAndMakeVisible(timelineViewport);

    trackList.onTracksChanged = [this] { handleTracksChanged(); };

    trackListViewport.onVerticalScroll = [this](int y) { syncVerticalScroll(y, true);  };
    timelineViewport.onVerticalScroll  = [this](int y) { syncVerticalScroll(y, false); };

    setSize(1200, 700);
}

MainComponent::~MainComponent()
{
    setLookAndFeel(nullptr);
}

void MainComponent::addTrack(TrackType type)
{
    trackList.addTrack(type);
}

void MainComponent::handleTracksChanged()
{
    // The track list has already resized itself to its new row count; mirror
    // that in the timeline and refresh both viewports' content bounds.
    updateContentBounds();
    timeline.repaint();
}

void MainComponent::syncVerticalScroll(int y, bool fromTrackList)
{
    if (syncingScroll)
        return;

    const juce::ScopedValueSetter<bool> guard(syncingScroll, true);

    if (fromTrackList)
        timelineViewport.setViewPosition(timelineViewport.getViewPositionX(), y);
    else
        trackListViewport.setViewPosition(0, y);
}

void MainComponent::updateContentBounds()
{
    timeline.updateContentSize();

    // Keep the track-list content the width of its viewport, and at least as
    // tall as the viewport so the background fills when there are few tracks.
    trackList.setSize(trackListViewport.getWidth(),
                      juce::jmax(trackList.getHeight(), trackListViewport.getHeight()));
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e)); // Deep dark background
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    transport->setBounds(area.removeFromTop(48));

    auto controls = area.removeFromTop(36).reduced(8, 4);
    addAudioButton.setBounds(controls.removeFromLeft(120));
    controls.removeFromLeft(8);
    addMidiButton.setBounds(controls.removeFromLeft(120));

    auto left = area.removeFromLeft(trackListWidth);
    tracksHeaderLabel.setBounds(left.removeFromTop(TimelineComponent::rulerHeight)
                                    .withTrimmedLeft(10));
    trackListViewport.setBounds(left);
    timelineViewport.setBounds(area);

    updateContentBounds();
}
