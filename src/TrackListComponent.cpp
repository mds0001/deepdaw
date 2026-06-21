#include "TrackListComponent.h"

TrackListComponent::TrackListComponent() = default;

void TrackListComponent::addTrack(TrackType type)
{
    auto track = std::make_unique<Track>();
    track->id = nextId++;
    track->type = type;

    const int sameTypeCount = [this, type]
    {
        int n = 0;
        for (auto& t : tracks)
            if (t->type == type) ++n;
        return n;
    }();

    track->name = (type == TrackType::audio ? "Audio " : "MIDI ")
                + juce::String(sameTypeCount + 1);

    // Default colours differ a little per track for visual separation.
    static const juce::Colour audioColours[] = {
        juce::Colour(0xffff7f00), juce::Colour(0xff2196f3), juce::Colour(0xff00bcd4)
    };
    static const juce::Colour midiColours[] = {
        juce::Colour(0xff00c853), juce::Colour(0xff9c27b0), juce::Colour(0xffe91e63)
    };
    track->colour = (type == TrackType::audio)
        ? audioColours[sameTypeCount % (int) std::size(audioColours)]
        : midiColours[sameTypeCount % (int) std::size(midiColours)];

    tracks.push_back(std::move(track));
    rebuildHeaders();
    notifyChanged();
}

void TrackListComponent::removeTrack(int trackId)
{
    auto it = std::find_if(tracks.begin(), tracks.end(),
                           [trackId](const std::unique_ptr<Track>& t) { return t->id == trackId; });
    if (it == tracks.end())
        return;

    tracks.erase(it);
    rebuildHeaders();
    notifyChanged();
}

void TrackListComponent::load(const std::vector<Track>& newTracks)
{
    tracks.clear();

    int maxId = 0;
    for (const auto& t : newTracks)
    {
        tracks.push_back(std::make_unique<Track>(t));
        maxId = juce::jmax(maxId, t.id);
    }

    nextId = maxId + 1;
    rebuildHeaders();
    notifyChanged();
}

void TrackListComponent::clear()
{
    load({});
    nextId = 1;
}

void TrackListComponent::rebuildHeaders()
{
    headers.clear();

    for (int i = 0; i < (int) tracks.size(); ++i)
    {
        auto header = std::make_unique<TrackHeaderComponent>(*tracks[i], i);
        const int trackId = tracks[i]->id;

        header->onDeleteRequested = [this, trackId] { removeTrack(trackId); };
        header->onChanged = [this] { notifyChanged(); };

        addAndMakeVisible(header.get());
        headers.push_back(std::move(header));
    }

    setSize(getWidth() > 0 ? getWidth() : 280, juce::jmax(1, (int) tracks.size()) * rowHeight);
    resized();
    repaint();
}

void TrackListComponent::notifyChanged()
{
    if (onTracksChanged)
        onTracksChanged();
}

void TrackListComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff202021));

    if (tracks.empty())
    {
        g.setColour(juce::Colour(0xff6f6f6f));
        g.setFont(14.0f);
        g.drawText("No tracks yet - use + Audio / + MIDI above",
                   getLocalBounds().reduced(12), juce::Justification::centredTop, true);
    }
}

void TrackListComponent::resized()
{
    for (int i = 0; i < (int) headers.size(); ++i)
        headers[i]->setBounds(0, i * rowHeight, getWidth(), rowHeight);
}