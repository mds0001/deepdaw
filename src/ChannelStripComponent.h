#pragma once

#include <JuceHeader.h>
#include "Track.h"

// One mixer channel strip for a track: colour/name header, pan, a vertical
// volume fader, and Mute/Solo. Edits the Track model directly and fires
// onMixChanged; the host reloads the engine and refreshes the matching track
// header (M/S stay in sync). refreshControls() pulls values back from the model.
class ChannelStripComponent : public juce::Component
{
public:
    ChannelStripComponent(Track& track, int displayIndex);
    ~ChannelStripComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

    void refreshControls(); // sync controls from the model (no callbacks)
    void refreshMeter();    // pull the output level + peak-hold/decay, repaint

    std::function<void()> onMixChanged;  // gain / pan / mute / solo edited here
    std::function<float()> getLevel;     // current post-fader output peak (0..1)

    static constexpr int stripWidth = 74;

private:
    Track& track;
    int displayIndex = 0;

    juce::Slider gainFader;
    juce::Slider panSlider;
    juce::TextButton muteButton{"M"};
    juce::TextButton soloButton{"S"};

    float meterLevel = 0.0f; // smoothed output level (UI thread)
    float meterHold  = 0.0f; // slow-decaying peak-hold marker
    juce::Rectangle<int> meterBounds;
    juce::Rectangle<int> readoutBounds; // dB value under the fader

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
};
