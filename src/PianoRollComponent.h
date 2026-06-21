#pragma once

#include <JuceHeader.h>
#include "TrackListComponent.h"
#include "MidiNote.h"

// The editable note grid: a tall component (one row per MIDI pitch) shown inside
// a vertically-scrolling viewport. Draws a keyboard gutter on the left, a
// bar/beat grid, and the clip's note blocks; handles draw / move / resize /
// delete with grid snap. Writes straight into the clip's note vector.
class PianoRollGrid : public juce::Component
{
public:
    PianoRollGrid();

    void setClip(std::vector<MidiNote>* notes, double lengthBeats, juce::Colour colour);
    void setSnap(double snapBeats) { snap = snapBeats; }
    int  preferredHeight() const { return numPitches * rowHeight; }
    int  yForPitch(int pitch) const { return (127 - pitch) * rowHeight; }

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;

    std::function<void()> onChanged;                 // notes edited
    std::function<void(int pitch, float vel)> onAudition;

    static constexpr int rowHeight = 12;
    static constexpr int gutterWidth = 46;
    static constexpr int numPitches = 128;

private:
    double pixelsPerBeat() const;
    double beatAtX(int x) const;
    int    pitchAtY(int y) const;
    int    xForBeat(double beat) const;
    double snapBeat(double beat) const;
    int    noteAt(juce::Point<int>) const; // index into *notes, or -1
    static bool isBlackKey(int pitch) { int n = pitch % 12; return n==1||n==3||n==6||n==8||n==10; }

    std::vector<MidiNote>* notes = nullptr;
    double lengthBeats = 4.0;
    double snap = 1.0; // beats
    juce::Colour colour { juce::Colour(0xff00c853) };

    enum class Drag { none, create, move, resize };
    Drag drag = Drag::none;
    int  activeNote = -1;
    double grabBeatOffset = 0.0; // (beat - note.start) at grab, for move
    int    grabPitchOffset = 0;  // (pitch - note.pitch) at grab, for move

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollGrid)
};

// The dockable piano-roll panel: a header (clip name, snap selector, Close) over
// a scrolling PianoRollGrid. Edits one MIDI clip, resolved from the track model
// by (trackId, clipIndex) so it survives note-vector reallocation.
class PianoRollComponent : public juce::Component
{
public:
    explicit PianoRollComponent(TrackListComponent& trackList);

    void openClip(int trackId, int clipIndex); // (re)bind to a clip
    void revalidate();                          // after structural changes: refresh or close
    bool isEditing(int trackId, int clipIndex) const { return trackId == editTrackId && clipIndex == editClipIndex; }

    void paint(juce::Graphics&) override;
    void resized() override;

    std::function<void()> onClose;
    std::function<void()> onNotesChanged;
    std::function<void(int pitch, float vel)> onAuditionNote;

private:
    Clip* resolveClip(juce::Colour* outColour = nullptr) const;
    void  bindGrid();

    TrackListComponent& trackList;
    int editTrackId = -1;
    int editClipIndex = -1;
    bool centerPending = false; // centre the view on middle C once laid out

    juce::Label titleLabel;
    juce::Label snapLabel{"snap", "Snap"};
    juce::ComboBox snapBox;
    juce::TextButton closeButton{"Close"};
    juce::Viewport viewport;
    PianoRollGrid grid;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};
