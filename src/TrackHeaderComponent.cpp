#include "TrackHeaderComponent.h"
#include "LookAndFeel.h"

namespace
{
    constexpr juce::uint32 muteOnColour = 0xfff2c500; // yellow
    constexpr juce::uint32 soloOnColour = 0xff00c853; // green
    constexpr juce::uint32 armOnColour  = 0xffff1744; // red
}

TrackHeaderComponent::TrackHeaderComponent(Track& trackToControl, int indexToShow)
    : track(trackToControl), displayIndex(indexToShow)
{
    nameLabel.setText(track.name, juce::dontSendNotification);
    nameLabel.setEditable(true);
    nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe0e0e0));
    nameLabel.setFont(juce::Font(juce::FontOptions(15.0f, juce::Font::bold)));
    nameLabel.onTextChange = [this]
    {
        track.name = nameLabel.getText();
        if (onChanged) onChanged();
    };
    addAndMakeVisible(nameLabel);

    colourButton.setColour(juce::TextButton::buttonColourId, track.colour);
    colourButton.onClick = [this] { chooseColour(); };
    addAndMakeVisible(colourButton);

    auto setupToggle = [this](juce::TextButton& b, juce::uint32 onColour)
    {
        b.setClickingTogglesState(true);
        b.setColour(juce::TextButton::buttonOnColourId, juce::Colour(onColour));
        b.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        addAndMakeVisible(b);
    };

    setupToggle(muteButton, muteOnColour);
    setupToggle(soloButton, soloOnColour);
    setupToggle(armButton, armOnColour);

    muteButton.onClick = [this] { track.muted  = muteButton.getToggleState(); if (onMixChanged) onMixChanged(); };
    soloButton.onClick = [this] { track.soloed = soloButton.getToggleState(); if (onMixChanged) onMixChanged(); };
    armButton.onClick  = [this] { track.armed  = armButton.getToggleState(); updateMeterTimer(); if (onChanged) onChanged(); };

    updateToggleStates();
    updateMeterTimer(); // armed tracks (e.g. loaded) start metering immediately
}

void TrackHeaderComponent::updateMeterTimer()
{
    if (track.armed && track.type == TrackType::audio)
    {
        startTimerHz(30);
    }
    else
    {
        stopTimer();
        meterLevel = 0.0f;
        repaint(meterBounds);
    }
}

void TrackHeaderComponent::timerCallback()
{
    const float level = getInputLevel ? getInputLevel() : 0.0f;
    meterLevel = juce::jmax(level, meterLevel * 0.80f); // jump up, decay down
    repaint(meterBounds);
}

void TrackHeaderComponent::setDisplayIndex(int newIndex)
{
    displayIndex = newIndex;
    repaint();
}

void TrackHeaderComponent::updateToggleStates()
{
    muteButton.setToggleState(track.muted,  juce::dontSendNotification);
    soloButton.setToggleState(track.soloed, juce::dontSendNotification);
    armButton.setToggleState(track.armed,   juce::dontSendNotification);
    colourButton.setColour(juce::TextButton::buttonColourId, track.colour);
}

void TrackHeaderComponent::chooseColour()
{
    static const juce::Colour palette[] = {
        juce::Colour(0xffff7f00), juce::Colour(0xff00c853), juce::Colour(0xff2196f3),
        juce::Colour(0xffe91e63), juce::Colour(0xff9c27b0), juce::Colour(0xfff2c500),
        juce::Colour(0xff00bcd4), juce::Colour(0xffff5722)
    };

    juce::PopupMenu menu;
    for (int i = 0; i < (int) std::size(palette); ++i)
        menu.addColouredItem(i + 1, "Colour", palette[i], true, false);

    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(colourButton),
        [this](int result)
        {
            if (result <= 0) return;
            track.colour = palette[result - 1];
            colourButton.setColour(juce::TextButton::buttonColourId, track.colour);
            repaint(); // refresh the whole row: swatch button + left colour stripe
            if (onChanged) onChanged();
        });
}

void TrackHeaderComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        if (track.type == TrackType::audio)
            menu.addItem(2, "Import Audio...");
        menu.addItem(1, "Delete Track");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this](int result)
            {
                if (result == 1 && onDeleteRequested)
                    onDeleteRequested();
                else if (result == 2 && onImportAudioRequested)
                    onImportAudioRequested();
            });
        return;
    }

    // Left-press on the row background begins a reorder drag. (Clicks that land
    // on the name field or the M/S/R/colour buttons are consumed by those
    // child components and never reach here.)
    if (onDragStart)
        onDragStart(this, e);
}

void TrackHeaderComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() && onDrag)
        onDrag(this, e);
}

void TrackHeaderComponent::mouseUp(const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() && onDragEnd)
        onDragEnd(this, e);
}

void TrackHeaderComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Alternating row shading for readability.
    auto base = (displayIndex % 2 == 0) ? juce::Colour(0xff2a2a2b)
                                        : juce::Colour(0xff252526);
    g.fillAll(base);

    // Colour stripe down the left edge.
    g.setColour(track.colour);
    g.fillRect(bounds.removeFromLeft(5));

    // Track number.
    g.setColour(juce::Colour(0xff8a8a8a));
    g.setFont(13.0f);
    g.drawText(juce::String(displayIndex + 1), 10, 0, 24, getHeight(),
               juce::Justification::centredLeft);

    // Type tag (AUD / MIDI).
    g.setColour(juce::Colour(0xff6f6f6f));
    g.setFont(11.0f);
    g.drawText(track.type == TrackType::audio ? "AUD" : "MIDI",
               40, getHeight() - 18, 40, 16, juce::Justification::centredLeft);

    // Input level meter (only while record-armed).
    if (track.armed && track.type == TrackType::audio && ! meterBounds.isEmpty())
    {
        g.setColour(juce::Colour(0xff141414));
        g.fillRect(meterBounds);

        const float level = juce::jlimit(0.0f, 1.0f, meterLevel);
        auto fill = meterBounds.toFloat().withWidth(meterBounds.getWidth() * level);
        g.setColour(level > 0.9f ? juce::Colour(0xffff1744) : juce::Colour(0xff00c853));
        g.fillRect(fill);

        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(meterBounds, 1);
    }

    // Bottom separator.
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRect(0, getHeight() - 1, getWidth(), 1);
}

void TrackHeaderComponent::resized()
{
    auto area = getLocalBounds().reduced(4);
    area.removeFromLeft(28); // space for the index number

    nameLabel.setBounds(area.removeFromTop(22).withTrimmedLeft(4));

    auto controls = area.removeFromTop(24).withTrimmedTop(2);
    colourButton.setBounds(controls.removeFromLeft(24));
    controls.removeFromLeft(6);

    const int btn = 26;
    muteButton.setBounds(controls.removeFromLeft(btn));
    controls.removeFromLeft(3);
    soloButton.setBounds(controls.removeFromLeft(btn));
    controls.removeFromLeft(3);
    armButton.setBounds(controls.removeFromLeft(btn));

    controls.removeFromLeft(8); // input meter fills the rest of the controls row
    meterBounds = controls.reduced(0, 5);
}