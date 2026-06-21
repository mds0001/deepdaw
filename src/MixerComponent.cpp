#include "MixerComponent.h"
#include "LookAndFeel.h"

MixerComponent::MixerComponent(TrackListComponent& list)
    : trackList(list)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();

    masterFader.setSliderStyle(juce::Slider::LinearVertical);
    masterFader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterFader.setRange(0.0, 1.5, 0.001);
    masterFader.setDoubleClickReturnValue(true, 1.0);
    masterFader.setValue(1.0, juce::dontSendNotification);
    masterFader.setColour(juce::Slider::thumbColourId, lf.getAccentColour());
    masterFader.setColour(juce::Slider::trackColourId, juce::Colour(0xff3a3a3a));
    masterFader.onValueChange = [this]
    {
        if (onMasterChanged) onMasterChanged((float) masterFader.getValue());
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
    masterFader.setValue(v, juce::dontSendNotification);
}

void MixerComponent::paint(juce::Graphics& g)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();
    g.fillAll(lf.getCustomBackgroundColour());

    // Top accent border to separate the panel from the arrangement.
    g.setColour(lf.getAccentColour());
    g.fillRect(0, 0, getWidth(), 2);

    // Master output meter (post master gain), bottom-up fill.
    if (! masterMeterBounds.isEmpty())
    {
        g.setColour(juce::Colour(0xff141414));
        g.fillRect(masterMeterBounds);

        const float level = juce::jlimit(0.0f, 1.0f, masterMeterLevel);
        auto fill = masterMeterBounds.toFloat();
        fill = fill.withTrimmedTop(fill.getHeight() * (1.0f - level));
        g.setColour(level > 0.9f ? juce::Colour(0xffff1744) : juce::Colour(0xff00c853));
        g.fillRect(fill);

        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(masterMeterBounds, 1);
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
