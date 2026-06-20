#pragma once

#include <JuceHeader.h>

class DeepDAWLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static DeepDAWLookAndFeel& getInstance();

    DeepDAWLookAndFeel();

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Colour getCustomBackgroundColour() const { return juce::Colour(0xff1e1e1e); }
    juce::Colour getPanelColour() const { return juce::Colour(0xff252526); }
    juce::Colour getAccentColour() const { return juce::Colour(0xffff7f00); }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeepDAWLookAndFeel)
};