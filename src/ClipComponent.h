#pragma once

#include <JuceHeader.h>
#include "Clip.h"

// Visual block for one audio clip on a timeline lane: a track-tinted rounded
// rectangle with the clip name and its waveform. Laid out by TimelineComponent
// (which knows the pixels-per-beat scale), so the clip stores its musical
// position for re-layout on zoom.
class ClipComponent : public juce::Component,
                      private juce::ChangeListener
{
public:
    ClipComponent(const Clip& clip, int trackIndex, juce::Colour trackColour,
                  juce::AudioFormatManager& formatManager, juce::AudioThumbnailCache& cache);
    ~ClipComponent() override;

    void paint(juce::Graphics&) override;

    double getStartBeat()   const { return startBeat; }
    double getLengthBeats() const { return lengthBeats; }
    int    getTrackIndex()  const { return trackIndex; }

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    juce::String name;
    juce::Colour colour;
    double startBeat = 0.0;
    double lengthBeats = 0.0;
    int trackIndex = 0;
    bool fileMissing = false;
    juce::AudioThumbnail thumbnail;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};
