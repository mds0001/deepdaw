#pragma once

#include <JuceHeader.h>
#include "Clip.h"

// Visual block for one audio clip on a timeline lane. Increment 2 draws a
// track-tinted rounded rectangle with the clip name; the waveform is added in
// Increment 3. The component is laid out by TimelineComponent, which knows the
// pixels-per-beat scale, so the clip stores its musical position for re-layout.
class ClipComponent : public juce::Component
{
public:
    ClipComponent(const Clip& clip, int trackIndex, juce::Colour trackColour);
    ~ClipComponent() override = default;

    void paint(juce::Graphics&) override;

    double getStartBeat()  const { return startBeat; }
    double getLengthBeats() const { return lengthBeats; }
    int    getTrackIndex() const { return trackIndex; }

private:
    juce::String name;
    juce::Colour colour;
    double startBeat = 0.0;
    double lengthBeats = 0.0;
    int trackIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};
