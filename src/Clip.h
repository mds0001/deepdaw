#pragma once

#include <JuceHeader.h>
#include "MidiNote.h"
#include <vector>

// One clip placed on a track lane. Position and length are stored in beats so
// they stay put when the tempo or zoom changes. Audio clips reference a source
// file (absolute path for now); MIDI clips leave `filePath` empty and hold a
// list of `notes` (positions relative to the clip start).
struct Clip
{
    juce::String name;
    juce::String filePath;          // audio clips: source file; MIDI clips: empty
    double startBeat = 0.0;
    double lengthBeats = 0.0;
    std::vector<MidiNote> notes;    // MIDI clips: the note data
};
