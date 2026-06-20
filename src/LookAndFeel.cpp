#include "LookAndFeel.h"

DeepDAWLookAndFeel& DeepDAWLookAndFeel::getInstance()
{
    static DeepDAWLookAndFeel instance;
    return instance;
}

DeepDAWLookAndFeel::DeepDAWLookAndFeel()
{
    // Dark Pro Tools / Cubase inspired palette
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xff1e1e1e));
    setColour(juce::DocumentWindow::backgroundColourId, juce::Colour(0xff1e1e1e));
    setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d2d));
    setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff7f00));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffcccccc));
    setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    setColour(juce::Slider::thumbColourId, juce::Colour(0xffff7f00));
    setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
}

void DeepDAWLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                              const juce::Colour& backgroundColour,
                                              bool shouldDrawButtonAsHighlighted,
                                              bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    auto baseColour = backgroundColour;

    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker(0.3f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter(0.1f);

    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, 4.0f);

    if (button.getToggleState())
    {
        g.setColour(getAccentColour());
        g.drawRoundedRectangle(bounds.reduced(1), 4.0f, 2.0f);
    }
}