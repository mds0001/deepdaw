#include "ClipComponent.h"

ClipComponent::ClipComponent(const Clip& clip, int indexOfTrack, juce::Colour trackColour,
                             juce::AudioFormatManager& formatManager, juce::AudioThumbnailCache& cache)
    : name(clip.name), colour(trackColour),
      startBeat(clip.startBeat), lengthBeats(clip.lengthBeats), trackIndex(indexOfTrack),
      thumbnail(512, formatManager, cache)
{
    // Clicks fall through to the timeline (so seeking still works); clip
    // interaction (move/delete) arrives in a later increment.
    setInterceptsMouseClicks(false, false);

    juce::File file(clip.filePath);
    if (file.existsAsFile())
        thumbnail.setSource(new juce::FileInputSource(file)); // loads async, repaints on change
    else
        fileMissing = true;

    thumbnail.addChangeListener(this);
}

ClipComponent::~ClipComponent()
{
    thumbnail.removeChangeListener(this);
}

void ClipComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint(); // thumbnail finished/progressed loading
}

void ClipComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    g.setColour((fileMissing ? juce::Colour(0xff404040) : colour).withAlpha(0.30f));
    g.fillRoundedRectangle(bounds, 3.0f);

    // Waveform below the name strip.
    auto waveArea = getLocalBounds().reduced(3).withTrimmedTop(14);
    if (thumbnail.getTotalLength() > 0.0 && waveArea.getHeight() > 2)
    {
        g.setColour(colour.brighter(0.5f).withAlpha(0.9f));
        thumbnail.drawChannels(g, waveArea, 0.0, thumbnail.getTotalLength(), 1.0f);
    }

    g.setColour(fileMissing ? juce::Colour(0xff6a6a6a) : colour);
    g.drawRoundedRectangle(bounds, 3.0f, 1.2f);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(11.0f);
    g.drawText(fileMissing ? (name + "  (offline)") : name,
               getLocalBounds().reduced(5, 2).removeFromTop(13),
               juce::Justification::topLeft, true);
}
