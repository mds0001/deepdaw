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
    setSize(numBars * pixelsPerBar,
            rulerHeight + n * TrackListComponent::rowHeight);
}

void TimelineComponent::paint(juce::Graphics& g)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();

    g.fillAll(juce::Colour(0xff1b1b1c));

    const int rowH       = TrackListComponent::rowHeight;
    const int numTracks  = trackList.getNumTracks();
    const int lanesBottom = rulerHeight + numTracks * rowH;
    const int beatWidth  = pixelsPerBar / 4;

    // Lane backgrounds, alternating to match the track headers row-for-row.
    for (int i = 0; i < numTracks; ++i)
    {
        const int laneY = rulerHeight + i * rowH;
        g.setColour((i % 2 == 0) ? juce::Colour(0xff232324) : juce::Colour(0xff1f1f20));
        g.fillRect(0, laneY, getWidth(), rowH);
    }

    // Beat subdivisions (lighter) then bar lines (stronger), spanning the lanes.
    if (numTracks > 0)
    {
        for (int bar = 0; bar < numBars; ++bar)
        {
            const int x = bar * pixelsPerBar;
            g.setColour(juce::Colour(0xff2a2a2c));
            for (int beat = 1; beat < 4; ++beat)
                g.fillRect(x + beat * beatWidth, rulerHeight, 1, numTracks * rowH);
        }
    }

    for (int bar = 0; bar <= numBars; ++bar)
    {
        g.setColour(juce::Colour(0xff3a3a3c));
        g.fillRect(bar * pixelsPerBar, 0, 1, lanesBottom);
    }

    // Ruler strip with bar numbers, drawn last so it sits above the grid.
    g.setColour(juce::Colour(0xff2a2a2b));
    g.fillRect(0, 0, getWidth(), rulerHeight);

    g.setColour(juce::Colour(0xff9a9a9a));
    g.setFont(12.0f);
    for (int bar = 0; bar < numBars; ++bar)
        g.drawText(juce::String(bar + 1),
                   bar * pixelsPerBar + 4, 0, pixelsPerBar - 4, rulerHeight,
                   juce::Justification::centredLeft);

    g.setColour(lf.getAccentColour().withAlpha(0.5f));
    g.fillRect(0, rulerHeight - 1, getWidth(), 1);

    if (numTracks == 0)
    {
        g.setColour(juce::Colour(0xff5a5a5a));
        g.setFont(14.0f);
        g.drawText("Add a track to begin arranging",
                   getLocalBounds().withTrimmedTop(rulerHeight),
                   juce::Justification::centred, true);
    }
}
