#include "ChannelStripComponent.h"
#include "LookAndFeel.h"

ChannelStripComponent::ChannelStripComponent(Track& t, int index)
    : track(t), displayIndex(index)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();

    panSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setRange(-1.0, 1.0, 0.01);
    panSlider.setDoubleClickReturnValue(true, 0.0); // detented centre
    panSlider.setValue(track.pan, juce::dontSendNotification);
    panSlider.setColour(juce::Slider::thumbColourId, lf.getAccentColour());
    panSlider.onValueChange = [this]
    {
        track.pan = (float) panSlider.getValue();
        if (onMixChanged) onMixChanged();
    };
    addAndMakeVisible(panSlider);

    gainFader.setSliderStyle(juce::Slider::LinearVertical);
    gainFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    gainFader.setRange(0.0, 1.5, 0.001);
    gainFader.setDoubleClickReturnValue(true, 1.0); // unity
    gainFader.setValue(track.gain, juce::dontSendNotification);
    gainFader.setColour(juce::Slider::thumbColourId, lf.getAccentColour());
    gainFader.setColour(juce::Slider::trackColourId, juce::Colour(0xff3a3a3a));
    gainFader.onValueChange = [this]
    {
        track.gain = (float) gainFader.getValue();
        if (onMixChanged) onMixChanged();
    };
    addAndMakeVisible(gainFader);

    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd23b3b));
    muteButton.setToggleState(track.muted, juce::dontSendNotification);
    muteButton.onClick = [this]
    {
        track.muted = muteButton.getToggleState();
        if (onMixChanged) onMixChanged();
    };
    addAndMakeVisible(muteButton);

    soloButton.setClickingTogglesState(true);
    soloButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffd2a83b));
    soloButton.setToggleState(track.soloed, juce::dontSendNotification);
    soloButton.onClick = [this]
    {
        track.soloed = soloButton.getToggleState();
        if (onMixChanged) onMixChanged();
    };
    addAndMakeVisible(soloButton);
}

void ChannelStripComponent::refreshControls()
{
    gainFader.setValue(track.gain, juce::dontSendNotification);
    panSlider.setValue(track.pan, juce::dontSendNotification);
    muteButton.setToggleState(track.muted, juce::dontSendNotification);
    soloButton.setToggleState(track.soloed, juce::dontSendNotification);
}

void ChannelStripComponent::refreshMeter()
{
    const float level = getLevel ? getLevel() : 0.0f;
    meterLevel = juce::jmax(level, meterLevel * 0.82f); // jump up, decay down
    repaint(meterBounds);
}

void ChannelStripComponent::paint(juce::Graphics& g)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();
    auto bounds = getLocalBounds();

    g.setColour(lf.getPanelColour());
    g.fillRect(bounds);
    g.setColour(juce::Colour(0xff333334));
    g.drawRect(bounds, 1);

    // Colour header + track name.
    auto header = bounds.removeFromTop(26);
    g.setColour(track.colour);
    g.fillRect(header.removeFromTop(4));
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    g.drawText(track.name, header, juce::Justification::centred, true);

    // "PAN" caption above the pan slider area.
    g.setColour(juce::Colour(0xff8a8a8a));
    g.setFont(11.0f);
    g.drawText("PAN", panSlider.getBounds().translated(0, -13).withHeight(12),
               juce::Justification::centred, false);

    // Output level meter (post-fader/pan), bottom-up fill.
    if (! meterBounds.isEmpty())
    {
        g.setColour(juce::Colour(0xff141414));
        g.fillRect(meterBounds);

        const float level = juce::jlimit(0.0f, 1.0f, meterLevel);
        auto fill = meterBounds.toFloat();
        fill = fill.withTrimmedTop(fill.getHeight() * (1.0f - level));
        g.setColour(level > 0.9f ? juce::Colour(0xffff1744) : juce::Colour(0xff00c853));
        g.fillRect(fill);

        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(meterBounds, 1);
    }
}

void ChannelStripComponent::resized()
{
    auto area = getLocalBounds().reduced(6);
    area.removeFromTop(24);          // colour header + name (painted)
    area.removeFromTop(12);          // room for the PAN caption
    panSlider.setBounds(area.removeFromTop(20));
    area.removeFromTop(4);

    auto buttons = area.removeFromBottom(22);
    const int bw = (buttons.getWidth() - 4) / 2;
    muteButton.setBounds(buttons.removeFromLeft(bw));
    buttons.removeFromLeft(4);
    soloButton.setBounds(buttons);
    area.removeFromBottom(6);

    // Fader on the left, a thin output meter pinned to the right.
    meterBounds = area.removeFromRight(8).reduced(0, 2);
    area.removeFromRight(4);
    gainFader.setBounds(area.reduced(6, 0));
}
