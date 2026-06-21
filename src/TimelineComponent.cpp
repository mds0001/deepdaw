#include "TimelineComponent.h"
#include "LookAndFeel.h"

TimelineComponent::TimelineComponent(TrackListComponent& list)
    : trackList(list)
{
    updateContentSize();
}

void TimelineComponent::updateContentSize()
{
    const int n = juce::jmax(1, trackList.getNumTracks());
    setSize(juce::roundToInt(numBars * getPixelsPerBar()),
            n * TrackListComponent::rowHeight); // lanes only; the ruler is a separate pinned strip
}

void TimelineComponent::setZoom(double newZoom)
{
    newZoom = juce::jlimit(minZoom, maxZoom, newZoom);
    if (newZoom == zoom)
        return;

    zoom = newZoom;
    updateContentSize(); // width changes with the scale
    repaint();
}

void TimelineComponent::paint(juce::Graphics& g)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();

    g.fillAll(juce::Colour(0xff1b1b1c));

    const int rowH        = TrackListComponent::rowHeight;
    const int numTracks   = trackList.getNumTracks();
    const int lanesBottom = numTracks * rowH;
    const double ppb      = getPixelsPerBar();
    const double ppbeat   = ppb / 4.0;

    // Lane backgrounds, alternating to match the track headers row-for-row.
    for (int i = 0; i < numTracks; ++i)
    {
        g.setColour((i % 2 == 0) ? juce::Colour(0xff232324) : juce::Colour(0xff1f1f20));
        g.fillRect(0, i * rowH, getWidth(), rowH);
    }

    // Beat subdivisions (lighter), spanning the lanes.
    if (numTracks > 0)
    {
        g.setColour(juce::Colour(0xff2a2a2c));
        for (int bar = 0; bar < numBars; ++bar)
            for (int beat = 1; beat < 4; ++beat)
                g.fillRect(juce::roundToInt(bar * ppb + beat * ppbeat), 0, 1, lanesBottom);
    }

    // Bar lines (stronger).
    g.setColour(juce::Colour(0xff3a3a3c));
    for (int bar = 0; bar <= numBars; ++bar)
        g.fillRect(juce::roundToInt(bar * ppb), 0, 1, lanesBottom);

    if (numTracks == 0)
    {
        g.setColour(juce::Colour(0xff5a5a5a));
        g.setFont(14.0f);
        g.drawText("Add a track to begin arranging",
                   getLocalBounds(), juce::Justification::centred, true);
    }

    // Playhead: a bright vertical line spanning the lanes (the ruler draws the
    // matching marker tab).
    g.setColour(lf.getAccentColour());
    g.fillRect(playheadX(), 0, 2, getHeight());
}

int TimelineComponent::playheadX() const
{
    return juce::roundToInt(playheadBeats * getPixelsPerBeat());
}

void TimelineComponent::setPlayheadBeats(double beats)
{
    if (beats == playheadBeats)
        return;

    // Repaint only the columns the playhead leaves and enters, so dragging it
    // (and 60 fps playback) stays cheap on the wide timeline.
    const int oldX = playheadX();
    playheadBeats = beats;
    const int newX = playheadX();

    const int left  = juce::jmin(oldX, newX) - 8;
    const int width = std::abs(newX - oldX) + 16;
    repaint(left, 0, width, getHeight());
}

void TimelineComponent::mouseDown(const juce::MouseEvent& e)
{
    const double beats = juce::jmax(0.0, e.position.x / getPixelsPerBeat());
    if (onSeek)
        onSeek(beats);
}

void TimelineComponent::mouseWheelMove(const juce::MouseEvent& e,
                                       const juce::MouseWheelDetails& wheel)
{
    // Ctrl/Cmd + wheel zooms about the cursor; a plain wheel is forwarded to
    // the parent Viewport so normal scrolling keeps working.
    if (e.mods.isCtrlDown() || e.mods.isCommandDown())
    {
        if (onZoomGesture && wheel.deltaY != 0.0f)
            onZoomGesture(wheel.deltaY, e.position.x);
    }
    else
    {
        Component::mouseWheelMove(e, wheel);
    }
}
