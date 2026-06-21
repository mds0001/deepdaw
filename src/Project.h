#pragma once

#include <JuceHeader.h>
#include "Track.h"
#include <vector>

// In-memory representation of a .deepdaw project file. Phase 1 stores the
// tempo and the track list; later phases extend this with clips, automation,
// etc. Serialised as human-readable JSON (see ProjectIO).
struct ProjectData
{
    double bpm = 120.0;
    std::vector<Track> tracks;
};

namespace ProjectIO
{
    // Current on-disk format version written into the JSON.
    inline constexpr const char* formatVersion = "1.0";

    juce::String toJsonString(const ProjectData&);
    bool parseJsonString(const juce::String&, ProjectData&);

    bool saveToFile(const juce::File&, const ProjectData&);
    bool loadFromFile(const juce::File&, ProjectData&);
}
