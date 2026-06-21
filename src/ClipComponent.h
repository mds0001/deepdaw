#pragma once

#include <JuceHeader.h>
#include "Clip.h"

// Visual block for one audio clip on a timeline lane: a track-tinted rounded
// rectangle with the clip name and its waveform. Laid out by TimelineComponent
// (which knows the pixels-per-beat scale), so the clip stores its musical
// position. Left-drag moves it; right-click deletes it (TimelineComponent does
// the actual model update via the callbacks).
class ClipComponent : public juce::Component,
                      private juce::ChangeListener
{
public:
    ClipComponent(const Clip& clip, int trackIndex, int trackId, int clipIndex,
                  juce::Colour trackColour, bool isMidi,
                  juce::AudioFormatManager& formatManager,
                  juce::AudioThumbnailCache& cache);
    ~ClipComponent() override;

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDoubleClick(const juce::MouseEvent&) override;

    double getStartBeat()   const { return startBeat; }
    double getLengthBeats() const { return lengthBeats; }
    int    getTrackIndex()  const { return trackIndex; }
    int    getTrackId()     const { return trackId; }
    int    getClipIndex()   const { return clipIndex; }

    // Drag forwarded to the timeline (which knows the scale and owns the model);
    // delete requested from the right-click menu.
    std::function<void(ClipComponent*, const juce::MouseEvent&)> onDragStart;
    std::function<void(ClipComponent*, const juce::MouseEvent&)> onDrag;
    std::function<void(ClipComponent*, const juce::MouseEvent&)> onDragEnd;
    std::function<void(ClipComponent*)> onDeleteRequested;
    // Double-click on a MIDI clip: open the piano-roll editor for it.
    std::function<void(ClipComponent*)> onOpenEditorRequested;

private:
    void changeListenerCallback(juce::ChangeBroadcaster*) override;

    juce::String name;
    juce::Colour colour;
    double startBeat = 0.0;
    double lengthBeats = 0.0;
    int trackIndex = 0;
    int trackId = 0;
    int clipIndex = 0;
    bool fileMissing = false;
    bool isMidi = false;
    std::vector<MidiNote> notes; // copy for the mini preview (MIDI clips)
    juce::AudioThumbnail thumbnail;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};
