#pragma once

#include <JuceHeader.h>
#include "TrackListComponent.h"
#include "ChannelStripComponent.h"

// The mixer panel: one ChannelStripComponent per track plus a master strip on
// the right. rebuild() recreates strips from the track model (after add/remove/
// reorder/rename/colour); refreshStrips() only re-pulls control values (after a
// mute/solo edit elsewhere). The master fader drives the engine master gain.
class MixerComponent : public juce::Component,
                       private juce::Timer
{
public:
    explicit MixerComponent(TrackListComponent& trackList);
    ~MixerComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    void rebuild();        // recreate strips from the model
    void refreshStrips();  // sync existing strips' controls from the model

    void setMasterValue(float v); // sync the master fader from the engine

    std::function<void()> onMixChanged;          // a strip edited gain/pan/M/S
    std::function<void(float)> onMasterChanged;   // master fader moved

    // Output-level sources for the meters (per track index, and master).
    std::function<float(int trackIndex)> trackLevelProvider;
    std::function<float()> masterLevelProvider;

private:
    void timerCallback() override; // refresh all meters at ~30 Hz

    TrackListComponent& trackList;
    std::vector<std::unique_ptr<ChannelStripComponent>> strips;

    juce::Slider masterFader;
    juce::Label masterLabel{"master", "MASTER"};

    float masterMeterLevel = 0.0f;
    juce::Rectangle<int> masterMeterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};
