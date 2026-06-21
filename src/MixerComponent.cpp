#include "MixerComponent.h"
#include "LookAndFeel.h"

MixerComponent::MixerComponent(TrackListComponent& list)
    : trackList(list)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();

    masterFader.setSliderStyle(juce::Slider::LinearVertical);
    masterFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterFader.setRange(-60.0, 6.0, 0.1); // dB
    masterFader.setDoubleClickReturnValue(true, 0.0);
    masterFader.setValue(0.0, juce::dontSendNotification);
    masterFader.setColour(juce::Slider::thumbColourId, lf.getAccentColour());
    masterFader.setColour(juce::Slider::trackColourId, juce::Colour(0xff3a3a3a));
    masterFader.onValueChange = [this]
    {
        const float gain = (float) juce::Decibels::decibelsToGain(masterFader.getValue(), -60.0);
        repaint(masterReadoutBounds);
        if (onMasterChanged) onMasterChanged(gain);
    };
    addAndMakeVisible(masterFader);

    masterLabel.setJustificationType(juce::Justification::centred);
    masterLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    masterLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    addAndMakeVisible(masterLabel);

    startTimerHz(30); // meter refresh
}

void MixerComponent::timerCallback()
{
    for (int i = 0; i < (int) strips.size(); ++i)
        strips[(size_t) i]->refreshMeter();

    const float level = masterLevelProvider ? masterLevelProvider() : 0.0f;
    masterMeterLevel = juce::jmax(level, masterMeterLevel * 0.82f);
    masterMeterHold  = juce::jmax(level, masterMeterHold - 0.010f);
    repaint(masterMeterBounds);
}

void MixerComponent::rebuild()
{
    strips.clear();

    auto& tracks = trackList.getTracks();
    for (int i = 0; i < (int) tracks.size(); ++i)
    {
        auto strip = std::make_unique<ChannelStripComponent>(*tracks[i], i);
        strip->onMixChanged = [this] { if (onMixChanged) onMixChanged(); };
        strip->getLevel = [this, i] { return trackLevelProvider ? trackLevelProvider(i) : 0.0f; };
        addAndMakeVisible(strip.get());
        strips.push_back(std::move(strip));
    }

    resized();
    repaint();
}

void MixerComponent::refreshStrips()
{
    for (auto& s : strips)
        s->refreshControls();
}

void MixerComponent::setMasterValue(float v)
{
    masterFader.setValue(juce::Decibels::gainToDecibels((double) v, -60.0), juce::dontSendNotification);
    repaint(masterReadoutBounds);
}

void MixerComponent::paint(juce::Graphics& g)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();
    g.fillAll(lf.getCustomBackgroundColour());

    // Top accent border to separate the panel from the arrangement.
    g.setColour(lf.getAccentColour());
    g.fillRect(0, 0, getWidth(), 2);

    // Master output meter (post master gain), bottom-up fill, with peak-hold tick.
    if (! masterMeterBounds.isEmpty())
    {
        g.setColour(juce::Colour(0xff141414));
        g.fillRect(masterMeterBounds);

        const float level = juce::jlimit(0.0f, 1.0f, masterMeterLevel);
        auto fill = masterMeterBounds.toFloat();
        fill = fill.withTrimmedTop(fill.getHeight() * (1.0f - level));
        g.setColour(level > 0.9f ? juce::Colour(0xffff1744) : juce::Colour(0xff00c853));
        g.fillRect(fill);

        const float hold = juce::jlimit(0.0f, 1.0f, masterMeterHold);
        if (hold > 0.0f)
        {
            const int ty = masterMeterBounds.getBottom() - juce::roundToInt(masterMeterBounds.getHeight() * hold);
            g.setColour(hold > 0.9f ? juce::Colour(0xffff1744) : juce::Colour(0xffd0d0d0));
            g.fillRect(masterMeterBounds.getX(), ty - 1, masterMeterBounds.getWidth(), 2);
        }

        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(masterMeterBounds, 1);
    }

    // Master dB readout (the fader value is already in dB).
    if (! masterReadoutBounds.isEmpty())
    {
        const double dB = masterFader.getValue();
        juce::String dbText = dB <= -60.0 ? "-inf"
                            : (dB >= 0.0 ? "+" : "") + juce::String(dB, 1);
        g.setColour(juce::Colour(0xffb0b0b0));
        g.setFont(11.0f);
        g.drawText(dbText, masterReadoutBounds, juce::Justification::centred);
    }
}

void MixerComponent::resized()
{
    auto area = getLocalBounds().reduced(6);
    area.removeFromTop(2); // accent border

    // Master strip pinned to the right: fader on the left, meter on the right.
    auto masterArea = area.removeFromRight(ChannelStripComponent::stripWidth + 8);
    masterLabel.setBounds(masterArea.removeFromTop(20));
    masterArea.removeFromTop(4);
    masterReadoutBounds = masterArea.removeFromBottom(14); // dB value
    masterArea.removeFromBottom(4);
    masterMeterBounds = masterArea.removeFromRight(8).reduced(0, 2);
    masterArea.removeFromRight(4);
    masterFader.setBounds(masterArea.reduced(10, 0));

    area.removeFromRight(6); // divider gap

    // Track strips left to right.
    int x = area.getX();
    for (auto& s : strips)
    {
        s->setBounds(x, area.getY(), ChannelStripComponent::stripWidth, area.getHeight());
        x += ChannelStripComponent::stripWidth + 2;
    }
}
