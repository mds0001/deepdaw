#include "ClipComponent.h"

ClipComponent::ClipComponent(const Clip& clip, int indexOfTrack, juce::Colour trackColour)
    : name(clip.name), colour(trackColour),
      startBeat(clip.startBeat), lengthBeats(clip.lengthBeats), trackIndex(indexOfTrack)
{
    // Clicks fall through to the timeline (so seeking still works); clip
    // interaction (move/delete) arrives in a later increment.
    setInterceptsMouseClicks(false, false);
}

void ClipComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    g.setColour(colour.withAlpha(0.35f));
    g.fillRoundedRectangle(bounds, 3.0f);

    g.setColour(colour);
    g.drawRoundedRectangle(bounds, 3.0f, 1.2f);

    // Name strip along the top.
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(11.0f);
    g.drawText(name, getLocalBounds().reduced(5, 2).removeFromTop(13),
               juce::Justification::topLeft, true);
}
