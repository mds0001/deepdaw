#include "Project.h"

namespace
{
    // Store colours as "#RRGGBB" so the file stays human-readable (the alpha is
    // always opaque for track colours).
    juce::String colourToHex(juce::Colour c)
    {
        return "#" + c.toDisplayString(false);
    }

    juce::Colour hexToColour(const juce::String& s)
    {
        auto hex = s.startsWithChar('#') ? s.substring(1) : s;
        return juce::Colour((juce::uint32) (0xff000000u | (juce::uint32) hex.getHexValue32()));
    }
}

juce::String ProjectIO::toJsonString(const ProjectData& data)
{
    juce::Array<juce::var> trackArray;
    for (const auto& t : data.tracks)
    {
        auto* obj = new juce::DynamicObject();
        obj->setProperty("id", t.id);
        obj->setProperty("name", t.name);
        obj->setProperty("type", t.type == TrackType::audio ? "audio" : "midi");
        obj->setProperty("color", colourToHex(t.colour));
        obj->setProperty("muted", t.muted);
        obj->setProperty("solo", t.soloed);
        obj->setProperty("armed", t.armed);
        trackArray.add(juce::var(obj));
    }

    auto* root = new juce::DynamicObject();
    root->setProperty("version", formatVersion);
    root->setProperty("bpm", data.bpm);
    root->setProperty("tracks", trackArray);

    return juce::JSON::toString(juce::var(root), false); // multi-line, readable
}

bool ProjectIO::parseJsonString(const juce::String& jsonText, ProjectData& out)
{
    auto parsed = juce::JSON::parse(jsonText);
    if (! parsed.isObject())
        return false;

    out = ProjectData{};
    out.bpm = (double) parsed.getProperty("bpm", 120.0);

    if (auto* arr = parsed.getProperty("tracks", juce::var()).getArray())
    {
        for (const auto& tv : *arr)
        {
            Track t;
            t.id     = (int) tv.getProperty("id", 0);
            t.name   = tv.getProperty("name", juce::String()).toString();
            t.type   = tv.getProperty("type", "audio").toString() == "midi"
                           ? TrackType::midi : TrackType::audio;
            t.colour = hexToColour(tv.getProperty("color", "#ff7f00").toString());
            t.muted  = (bool) tv.getProperty("muted", false);
            t.soloed = (bool) tv.getProperty("solo", false);
            t.armed  = (bool) tv.getProperty("armed", false);
            out.tracks.push_back(t);
        }
    }

    return true;
}

bool ProjectIO::saveToFile(const juce::File& file, const ProjectData& data)
{
    return file.replaceWithText(toJsonString(data));
}

bool ProjectIO::loadFromFile(const juce::File& file, ProjectData& out)
{
    if (! file.existsAsFile())
        return false;

    return parseJsonString(file.loadFileAsString(), out);
}
