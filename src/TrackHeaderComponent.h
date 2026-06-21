#pragma once

#include <JuceHeader.h>
#include "Track.h"

// One row in the track list: index, editable name, colour swatch, and the
// standard Mute / Solo / Record-arm toggles. Right-click anywhere on the row
// background to delete the track; left-drag the row background to reorder.
class TrackHeaderComponent : public juce::Component,
                             private juce::Timer
{
public:
    TrackHeaderComponent(Track& trackToControl, int displayIndex);
    ~TrackHeaderComponent() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

    void setDisplayIndex(int newIndex);

    // Fired when the user requests deletion via the right-click menu.
    std::function<void()> onDeleteRequested;
    // Fired when the user picks "Import Audio…" from the right-click menu.
    std::function<void()> onImportAudioRequested;
    // Fired when a structural property changes (name, colour, record-arm).
    std::function<void()> onChanged;
    // Fired when mute/solo changes (audibility only — no structural rebuild).
    std::function<void()> onMixChanged;

    // Pull M/S/colour back from the model after an external (mixer) edit.
    void refreshControls() { updateToggleStates(); }
    // Row-reorder drag, forwarded to the owning list (which does the work).
    std::function<void(TrackHeaderComponent*, const juce::MouseEvent&)> onDragStart;
    std::function<void(TrackHeaderComponent*, const juce::MouseEvent&)> onDrag;
    std::function<void(TrackHeaderComponent*, const juce::MouseEvent&)> onDragEnd;

    // Returns the current input peak (0..1); used by the armed-track meter.
    std::function<float()> getInputLevel;

private:
    void updateToggleStates();
    void chooseColour();
    void updateMeterTimer();
    void timerCallback() override;

    Track& track;
    int displayIndex = 0;

    juce::Label nameLabel;
    juce::TextButton colourButton;
    juce::TextButton muteButton{"M"};
    juce::TextButton soloButton{"S"};
    juce::TextButton armButton{"R"};

    float meterLevel = 0.0f; // smoothed input level (only meaningful while armed)
    juce::Rectangle<int> meterBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackHeaderComponent)
};