#include "TimelineRulerComponent.h"
#include "LookAndFeel.h"

TimelineRulerComponent::TimelineRulerComponent(TimelineComponent& timelineToTrack)
    : timeline(timelineToTrack)
{
}

void TimelineRulerComponent::setScrollX(int newScrollX)
{
    if (newScrollX == scrollX)
        return;

    scrollX = newScrollX;
    repaint();
}

void TimelineRulerComponent::paint(juce::Graphics& g)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();

    const double ppb    = timeline.getPixelsPerBar();
    const double ppbeat = ppb / 4.0;
    const int    h      = getHeight();

    g.fillAll(juce::Colour(0xff2a2a2b));

    g.setFont(12.0f);
    for (int bar = 0; bar <= TimelineComponent::numBars; ++bar)
    {
        const int x = juce::roundToInt(bar * ppb) - scrollX;
        if (x < -60 || x > getWidth() + 4)
            continue;

        g.setColour(juce::Colour(0xff4a4a4c));
        g.fillRect(x, h - 8, 1, 8); // bar tick

        g.setColour(juce::Colour(0xff5a5a5c));
        for (int beat = 1; beat < 4; ++beat)
            g.fillRect(x + juce::roundToInt(beat * ppbeat), h - 4, 1, 4); // beat ticks

        g.setColour(juce::Colour(0xff9a9a9a));
        g.drawText(juce::String(bar + 1), x + 4, 0, juce::roundToInt(ppb) - 4, h - 8,
                   juce::Justification::centredLeft);
    }

    g.setColour(lf.getAccentColour().withAlpha(0.6f));
    g.fillRect(0, h - 1, getWidth(), 1);

    // Playhead marker (line + tab), matching the lane playhead's x.
    const int px = juce::roundToInt(timeline.getPlayheadBeats() * ppbeat) - scrollX;
    if (px >= -8 && px <= getWidth() + 8)
    {
        g.setColour(lf.getAccentColour());
        g.fillRect(px, 0, 2, h);

        juce::Path marker;
        marker.addTriangle((float) px - 5.0f, 0.0f,
                           (float) px + 7.0f, 0.0f,
                           (float) px + 1.0f, 8.0f);
        g.fillPath(marker);
    }
}

void TimelineRulerComponent::mouseDown(const juce::MouseEvent& e)
{
    const double beats = juce::jmax(0.0, (e.position.x + scrollX) / timeline.getPixelsPerBeat());
    if (onSeek)
        onSeek(beats);
}

void TimelineRulerComponent::mouseWheelMove(const juce::MouseEvent& e,
                                            const juce::MouseWheelDetails& wheel)
{
    // Ctrl/Cmd + wheel zooms about the cursor (in content coordinates). A plain
    // wheel is ignored here so the thin strip does not desync from the lanes.
    if ((e.mods.isCtrlDown() || e.mods.isCommandDown()) && onZoomGesture && wheel.deltaY != 0.0f)
        onZoomGesture(wheel.deltaY, e.position.x + (float) scrollX);
}
