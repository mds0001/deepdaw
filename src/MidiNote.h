#pragma once

// One note in a MIDI clip. Position/length are in beats relative to the clip
// start, so moving the clip moves its notes. Pitch is a MIDI note number
// (0-127, 60 = middle C); velocity is normalised 0..1.
struct MidiNote
{
    int pitch = 60;
    double startBeat = 0.0;   // relative to the clip start
    double lengthBeats = 1.0;
    float velocity = 0.8f;
};
