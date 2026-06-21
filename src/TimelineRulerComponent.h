#pragma once

#include <JuceHeader.h>
#include "TimelineComponent.h"

// The bar-number ruler. It sits in a fixed strip above the timeline lanes (so
// it never scrolls vertically) and draws the visible bars shifted by the
// timeline's horizontal scroll, so it stays aligned with the lanes. Reads its
// scale and playhead position straight from the TimelineComponent.
class TimelineRulerComponent : public juce::Component
{
public:
    explicit TimelineRulerComponent(TimelineComponent& timelineToTrack);
    ~TimelineRulerComponent() override = default;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;

    // Horizontal scroll offset of the lanes, so the ruler can match them.
    void setScrollX(int newScrollX);

    // Same callbacks the timeline exposes, so a click/zoom on the ruler behaves
    // exactly like one on the lanes.
    std::function<void(double beats)> onSeek;
    std::function<void(float wheelDeltaY, float anchorContentX)> onZoomGesture;

private:
    TimelineComponent& timeline;
    int scrollX = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineRulerComponent)
};
