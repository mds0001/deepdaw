#include "TimelineComponent.h"
#include "LookAndFeel.h"

TimelineComponent::TimelineComponent(TrackListComponent& list, juce::AudioFormatManager& fm)
    : trackList(list), formatManager(fm)
{
    updateContentSize();
}

void TimelineComponent::updateContentSize()
{
    const int n = juce::jmax(1, trackList.getNumTracks());
    setSize(juce::roundToInt(numBars * getPixelsPerBar()),
            n * TrackListComponent::rowHeight); // lanes only; the ruler is a separate pinned strip
    layoutClips();
}

void TimelineComponent::rebuildClips()
{
    clipComponents.clear();

    const auto& tracks = trackList.getTracks();
    for (int i = 0; i < (int) tracks.size(); ++i)
        for (int j = 0; j < (int) tracks[i]->clips.size(); ++j)
        {
            auto comp = std::make_unique<ClipComponent>(tracks[i]->clips[j], i, tracks[i]->id, j,
                                                        tracks[i]->colour, formatManager, thumbnailCache);
            comp->onDragStart = [this](ClipComponent* c, const juce::MouseEvent& e) { clipDragStart(c, e.getEventRelativeTo(this).x); };
            comp->onDrag      = [this](ClipComponent* c, const juce::MouseEvent& e) { clipDrag(c, e.getEventRelativeTo(this).x); };
            comp->onDragEnd   = [this](ClipComponent* c, const juce::MouseEvent&)   { clipDragEnd(c); };
            comp->onDeleteRequested = [this](ClipComponent* c) { trackList.removeClip(c->getTrackId(), c->getClipIndex()); };
            addAndMakeVisible(comp.get());
            clipComponents.push_back(std::move(comp));
        }

    layoutClips();
}

void TimelineComponent::layoutClips()
{
    const double ppbeat = getPixelsPerBeat();
    const int rowH = TrackListComponent::rowHeight;

    for (auto& c : clipComponents)
        c->setBounds(juce::roundToInt(c->getStartBeat() * ppbeat),
                     c->getTrackIndex() * rowH + 3,
                     juce::jmax(4, juce::roundToInt(c->getLengthBeats() * ppbeat)),
                     rowH - 6);
}

void TimelineComponent::setZoom(double newZoom)
{
    newZoom = juce::jlimit(minZoom, maxZoom, newZoom);
    if (newZoom == zoom)
        return;

    zoom = newZoom;
    updateContentSize(); // width changes with the scale
    repaint();
}

void TimelineComponent::paint(juce::Graphics& g)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();

    g.fillAll(juce::Colour(0xff1b1b1c));

    const int rowH        = TrackListComponent::rowHeight;
    const int numTracks   = trackList.getNumTracks();
    const int lanesBottom = numTracks * rowH;
    const double ppb      = getPixelsPerBar();
    const double ppbeat   = ppb / 4.0;

    // Lane backgrounds, alternating to match the track headers row-for-row.
    for (int i = 0; i < numTracks; ++i)
    {
        g.setColour((i % 2 == 0) ? juce::Colour(0xff232324) : juce::Colour(0xff1f1f20));
        g.fillRect(0, i * rowH, getWidth(), rowH);
    }

    // Highlight the lane a file is being dragged over.
    if (dropTargetTrack >= 0 && dropTargetTrack < numTracks)
    {
        g.setColour(lf.getAccentColour().withAlpha(0.18f));
        g.fillRect(0, dropTargetTrack * rowH, getWidth(), rowH);
    }

    // Beat subdivisions (lighter), spanning the lanes.
    if (numTracks > 0)
    {
        g.setColour(juce::Colour(0xff2a2a2c));
        for (int bar = 0; bar < numBars; ++bar)
            for (int beat = 1; beat < 4; ++beat)
                g.fillRect(juce::roundToInt(bar * ppb + beat * ppbeat), 0, 1, lanesBottom);
    }

    // Bar lines (stronger).
    g.setColour(juce::Colour(0xff3a3a3c));
    for (int bar = 0; bar <= numBars; ++bar)
        g.fillRect(juce::roundToInt(bar * ppb), 0, 1, lanesBottom);

    if (numTracks == 0)
    {
        g.setColour(juce::Colour(0xff5a5a5a));
        g.setFont(14.0f);
        g.drawText("Add a track to begin arranging",
                   getLocalBounds(), juce::Justification::centred, true);
    }
}

void TimelineComponent::paintOverChildren(juce::Graphics& g)
{
    // Playhead: a bright vertical line spanning the lanes, drawn on top of the
    // clip blocks (the ruler draws the matching marker tab).
    g.setColour(DeepDAWLookAndFeel::getInstance().getAccentColour());
    g.fillRect(playheadX(), 0, 2, getHeight());
}

int TimelineComponent::playheadX() const
{
    return juce::roundToInt(playheadBeats * getPixelsPerBeat());
}

void TimelineComponent::setPlayheadBeats(double beats)
{
    if (beats == playheadBeats)
        return;

    // Repaint only the columns the playhead leaves and enters, so dragging it
    // (and 60 fps playback) stays cheap on the wide timeline.
    const int oldX = playheadX();
    playheadBeats = beats;
    const int newX = playheadX();

    const int left  = juce::jmin(oldX, newX) - 8;
    const int width = std::abs(newX - oldX) + 16;
    repaint(left, 0, width, getHeight());
}

void TimelineComponent::mouseDown(const juce::MouseEvent& e)
{
    const double beats = juce::jmax(0.0, e.position.x / getPixelsPerBeat());
    if (onSeek)
        onSeek(beats);
}

void TimelineComponent::mouseWheelMove(const juce::MouseEvent& e,
                                       const juce::MouseWheelDetails& wheel)
{
    // Ctrl/Cmd + wheel zooms about the cursor; a plain wheel is forwarded to
    // the parent Viewport so normal scrolling keeps working.
    if (e.mods.isCtrlDown() || e.mods.isCommandDown())
    {
        if (onZoomGesture && wheel.deltaY != 0.0f)
            onZoomGesture(wheel.deltaY, e.position.x);
    }
    else
    {
        Component::mouseWheelMove(e, wheel);
    }
}

int TimelineComponent::trackIndexAt(int y) const
{
    const int i = y / TrackListComponent::rowHeight;
    return juce::isPositiveAndBelow(i, trackList.getNumTracks()) ? i : -1;
}

static bool looksLikeAudioFile(const juce::String& path)
{
    return juce::File(path).hasFileExtension("wav;aiff;aif;flac;mp3;ogg;m4a;wma");
}

bool TimelineComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& f : files)
        if (looksLikeAudioFile(f))
            return true;
    return false;
}

void TimelineComponent::fileDragMove(const juce::StringArray&, int, int y)
{
    const auto& tracks = trackList.getTracks();
    const int i = trackIndexAt(y);
    const int target = (i >= 0 && tracks[i]->type == TrackType::audio) ? i : -1;
    if (target != dropTargetTrack)
    {
        dropTargetTrack = target;
        repaint();
    }
}

void TimelineComponent::fileDragExit(const juce::StringArray&)
{
    dropTargetTrack = -1;
    repaint();
}

void TimelineComponent::filesDropped(const juce::StringArray& files, int x, int y)
{
    dropTargetTrack = -1;
    repaint();

    const int i = trackIndexAt(y);
    if (i < 0)
        return;

    const auto& tracks = trackList.getTracks();
    if (tracks[i]->type != TrackType::audio)
        return;

    const double startBeat = juce::jmax(0.0, x / getPixelsPerBeat());
    for (const auto& f : files)
        if (looksLikeAudioFile(f) && onFileDropped)
            onFileDropped(tracks[i]->id, startBeat, juce::File(f));
}

void TimelineComponent::clipDragStart(ClipComponent* c, int mouseXInTimeline)
{
    draggedClip     = c;
    dragClipStartX  = c->getX();
    dragMouseStartX = mouseXInTimeline;
    c->toFront(false);
}

void TimelineComponent::clipDrag(ClipComponent* c, int mouseXInTimeline)
{
    if (draggedClip != c)
        return;

    const int newX = juce::jlimit(0, juce::jmax(0, getWidth() - c->getWidth()),
                                  dragClipStartX + (mouseXInTimeline - dragMouseStartX));
    c->setTopLeftPosition(newX, c->getY()); // horizontal move only
}

void TimelineComponent::clipDragEnd(ClipComponent* c)
{
    if (draggedClip != c)
        return;

    draggedClip = nullptr;

    if (c->getX() == dragClipStartX)
        return; // a plain click, not a move — avoid a needless rebuild/reload

    const double newStartBeat = c->getX() / getPixelsPerBeat();
    trackList.setClipStart(c->getTrackId(), c->getClipIndex(), newStartBeat); // notifies -> rebuild + reload
}
