#pragma once

#include <JuceHeader.h>
#include "Track.h"

// One row in the track list: index, editable name, colour swatch, and the
// standard Mute / Solo / Record-arm toggles. Right-click anywhere on the row
// background to delete the track.
class TrackHeaderComponent : public juce::Component
{
public:
    TrackHeaderComponent(Track& trackToControl, int displayIndex);
    ~TrackHeaderComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;

    void setDisplayIndex(int newIndex);

    // Fired when the user requests deletion via the right-click menu.
    std::function<void()> onDeleteRequested;
    // Fired when any visible property changes (name, colour, M/S/R state).
    std::function<void()> onChanged;

private:
    void updateToggleStates();
    void chooseColour();

    Track& track;
    int displayIndex = 0;

    juce::Label nameLabel;
    juce::TextButton colourButton;
    juce::TextButton muteButton{"M"};
    juce::TextButton soloButton{"S"};
    juce::TextButton armButton{"R"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeaderComponent)
};