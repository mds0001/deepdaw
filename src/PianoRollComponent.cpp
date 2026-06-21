#include "PianoRollComponent.h"
#include "LookAndFeel.h"

//==============================================================================
// PianoRollGrid

PianoRollGrid::PianoRollGrid()
{
    setSize(200, numPitches * rowHeight);
}

void PianoRollGrid::setClip(std::vector<MidiNote>* n, double length, juce::Colour c)
{
    notes = n;
    lengthBeats = juce::jmax(0.25, length);
    colour = c;
    repaint();
}

double PianoRollGrid::pixelsPerBeat() const
{
    const int w = getWidth() - gutterWidth;
    return (lengthBeats > 0.0 && w > 0) ? (double) w / lengthBeats : 20.0;
}

double PianoRollGrid::beatAtX(int x) const     { return juce::jmax(0.0, (x - gutterWidth) / pixelsPerBeat()); }
int    PianoRollGrid::pitchAtY(int y) const    { return juce::jlimit(0, 127, 127 - y / rowHeight); }
int    PianoRollGrid::xForBeat(double b) const { return gutterWidth + juce::roundToInt(b * pixelsPerBeat()); }
double PianoRollGrid::snapBeat(double b) const { return snap > 0.0 ? std::round(b / snap) * snap : b; }

int PianoRollGrid::noteAt(juce::Point<int> p) const
{
    if (notes == nullptr)
        return -1;

    const double ppb = pixelsPerBeat();
    for (int i = (int) notes->size() - 1; i >= 0; --i)
    {
        const auto& n = (*notes)[i];
        const int x = xForBeat(n.startBeat);
        const int w = juce::jmax(2, juce::roundToInt(n.lengthBeats * ppb));
        const int y = yForPitch(n.pitch);
        if (juce::Rectangle<int>(x, y, w, rowHeight).contains(p))
            return i;
    }
    return -1;
}

void PianoRollGrid::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1b1b1c));

    const auto clip = g.getClipBounds();
    const int firstRow = juce::jmax(0, clip.getY() / rowHeight);
    const int lastRow  = juce::jmin(numPitches - 1, clip.getBottom() / rowHeight);
    const int w = getWidth();

    for (int row = firstRow; row <= lastRow; ++row)
    {
        const int pitch = 127 - row;
        const int y = row * rowHeight;
        const bool black = isBlackKey(pitch);

        // Note-area row background.
        g.setColour(black ? juce::Colour(0xff202022) : juce::Colour(0xff262628));
        g.fillRect(gutterWidth, y, w - gutterWidth, rowHeight);

        // Octave divider at each C.
        if (pitch % 12 == 0)
        {
            g.setColour(juce::Colour(0xff333336));
            g.fillRect(gutterWidth, y, w - gutterWidth, 1);
        }

        // Keyboard gutter key.
        g.setColour(black ? juce::Colour(0xff2c2c2c) : juce::Colour(0xffd6d6d6));
        g.fillRect(0, y, gutterWidth - 1, rowHeight - 1);
        if (pitch % 12 == 0)
        {
            g.setColour(juce::Colours::black);
            g.setFont(9.0f);
            g.drawText("C" + juce::String(pitch / 12 - 1), 3, y, gutterWidth - 5, rowHeight,
                       juce::Justification::centredLeft);
        }
    }

    // Vertical grid: snap lines, then beats, then bars (each stronger).
    auto vlines = [&](double step, juce::Colour c)
    {
        if (step <= 0.0) return;
        g.setColour(c);
        for (double b = 0.0; b <= lengthBeats + 1e-6; b += step)
            g.fillRect(xForBeat(b), clip.getY(), 1, clip.getHeight());
    };
    vlines(snap, juce::Colour(0xff2c2c2e));
    vlines(1.0,  juce::Colour(0xff3a3a3c));
    vlines(4.0,  juce::Colour(0xff4a4a4e));

    // Notes.
    if (notes != nullptr)
    {
        const double ppb = pixelsPerBeat();
        for (const auto& n : *notes)
        {
            const int x = xForBeat(n.startBeat);
            const int wd = juce::jmax(2, juce::roundToInt(n.lengthBeats * ppb));
            const int y = yForPitch(n.pitch);
            juce::Rectangle<int> r(x, y, wd, rowHeight - 1);
            g.setColour(colour.withAlpha(0.55f + 0.4f * juce::jlimit(0.0f, 1.0f, n.velocity)));
            g.fillRect(r);
            g.setColour(colour.brighter(0.5f));
            g.drawRect(r, 1);
        }
    }

    // Gutter separator.
    g.setColour(juce::Colour(0xff3a3a3c));
    g.fillRect(gutterWidth - 1, clip.getY(), 1, clip.getHeight());
}

void PianoRollGrid::mouseDown(const juce::MouseEvent& e)
{
    if (notes == nullptr)
        return;

    if (e.x < gutterWidth) // clicked the keyboard: audition only
    {
        if (onAudition) onAudition(pitchAtY(e.y), 0.8f);
        return;
    }

    if (e.mods.isPopupMenu()) // right-click deletes
    {
        const int i = noteAt(e.getPosition());
        if (i >= 0)
        {
            notes->erase(notes->begin() + i);
            repaint();
            if (onChanged) onChanged();
        }
        return;
    }

    const int i = noteAt(e.getPosition());
    if (i >= 0)
    {
        auto& n = (*notes)[i];
        const int rightX = xForBeat(n.startBeat + n.lengthBeats);
        if (e.x >= rightX - 6) // grab the right edge to resize
        {
            drag = Drag::resize;
            activeNote = i;
        }
        else
        {
            drag = Drag::move;
            activeNote = i;
            grabBeatOffset  = beatAtX(e.x) - n.startBeat;
            grabPitchOffset = pitchAtY(e.y) - n.pitch;
        }
        if (onAudition) onAudition(n.pitch, n.velocity);
    }
    else // draw a new note
    {
        MidiNote n;
        n.pitch       = pitchAtY(e.y);
        n.startBeat   = std::floor(beatAtX(e.x) / snap) * snap;
        n.lengthBeats = snap;
        n.velocity    = 0.8f;
        if (n.startBeat + n.lengthBeats > lengthBeats)
            n.startBeat = juce::jmax(0.0, lengthBeats - n.lengthBeats);

        notes->push_back(n);
        activeNote = (int) notes->size() - 1;
        drag = Drag::create;
        if (onAudition) onAudition(n.pitch, n.velocity);
    }
    repaint();
}

void PianoRollGrid::mouseDrag(const juce::MouseEvent& e)
{
    if (notes == nullptr || activeNote < 0 || activeNote >= (int) notes->size())
        return;

    auto& n = (*notes)[activeNote];
    const double b = beatAtX(e.x);

    if (drag == Drag::move)
    {
        double ns = snapBeat(b - grabBeatOffset);
        n.startBeat = juce::jlimit(0.0, juce::jmax(0.0, lengthBeats - n.lengthBeats), ns);
        n.pitch = juce::jlimit(0, 127, pitchAtY(e.y) - grabPitchOffset);
    }
    else if (drag == Drag::resize || drag == Drag::create)
    {
        double len = snapBeat(b) - n.startBeat;
        len = juce::jmax(snap, len);
        if (n.startBeat + len > lengthBeats)
            len = lengthBeats - n.startBeat;
        n.lengthBeats = juce::jmax(snap, len);
    }
    repaint();
}

void PianoRollGrid::mouseUp(const juce::MouseEvent&)
{
    if (drag != Drag::none)
    {
        drag = Drag::none;
        activeNote = -1;
        if (onChanged) onChanged();
    }
}

//==============================================================================
// PianoRollComponent

namespace
{
    double snapBeatsForId(int id)
    {
        switch (id)
        {
            case 1: return 4.0;   // 1/1
            case 2: return 2.0;   // 1/2
            case 3: return 1.0;   // 1/4
            case 4: return 0.5;   // 1/8
            case 5: return 0.25;  // 1/16
            default: return 1.0;
        }
    }
}

PianoRollComponent::PianoRollComponent(TrackListComponent& list)
    : trackList(list)
{
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    addAndMakeVisible(titleLabel);

    snapLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a8a8a));
    snapLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(snapLabel);

    snapBox.addItem("1/1", 1);
    snapBox.addItem("1/2", 2);
    snapBox.addItem("1/4", 3);
    snapBox.addItem("1/8", 4);
    snapBox.addItem("1/16", 5);
    snapBox.setSelectedId(3, juce::dontSendNotification); // 1/4 default
    snapBox.onChange = [this]
    {
        grid.setSnap(snapBeatsForId(snapBox.getSelectedId()));
        grid.repaint();
    };
    addAndMakeVisible(snapBox);

    closeButton.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible(closeButton);

    viewport.setViewedComponent(&grid, false);
    viewport.setScrollBarsShown(true, false); // vertical only
    addAndMakeVisible(viewport);

    grid.setSnap(snapBeatsForId(snapBox.getSelectedId()));
    grid.onChanged  = [this] { if (onNotesChanged) onNotesChanged(); };
    grid.onAudition = [this](int pitch, float vel) { if (onAuditionNote) onAuditionNote(pitch, vel); };
}

Clip* PianoRollComponent::resolveClip(juce::Colour* outColour) const
{
    for (auto& t : trackList.getTracks())
        if (t->id == editTrackId)
        {
            if (outColour) *outColour = t->colour;
            return juce::isPositiveAndBelow(editClipIndex, (int) t->clips.size())
                       ? &t->clips[editClipIndex] : nullptr;
        }
    return nullptr;
}

void PianoRollComponent::bindGrid()
{
    juce::Colour c;
    auto* clip = resolveClip(&c);
    if (clip == nullptr)
        return;

    titleLabel.setText("Piano Roll - " + clip->name, juce::dontSendNotification);
    grid.setClip(&clip->notes, clip->lengthBeats, c);
    resized();
}

void PianoRollComponent::openClip(int trackId, int clipIndex)
{
    editTrackId = trackId;
    editClipIndex = clipIndex;
    centerPending = true; // centre on middle C once the panel has a size
    bindGrid();
}

void PianoRollComponent::revalidate()
{
    juce::Colour c;
    auto* clip = resolveClip(&c);
    if (clip == nullptr) // clip (or its track) was deleted
    {
        if (onClose) onClose();
        return;
    }

    titleLabel.setText("Piano Roll - " + clip->name, juce::dontSendNotification);
    grid.setClip(&clip->notes, clip->lengthBeats, c); // refresh pointer (vector may have moved)
    repaint();
}

void PianoRollComponent::paint(juce::Graphics& g)
{
    auto& lf = DeepDAWLookAndFeel::getInstance();
    g.fillAll(lf.getPanelColour());
    g.setColour(lf.getAccentColour());
    g.fillRect(0, 0, getWidth(), 2);
}

void PianoRollComponent::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop(2); // accent border

    auto header = area.removeFromTop(28).reduced(6, 2);
    closeButton.setBounds(header.removeFromRight(66));
    header.removeFromRight(8);
    snapBox.setBounds(header.removeFromRight(70));
    snapLabel.setBounds(header.removeFromRight(44));
    titleLabel.setBounds(header);

    viewport.setBounds(area);
    grid.setSize(juce::jmax(80, viewport.getMaximumVisibleWidth()), grid.preferredHeight());

    // Centre the view on middle C the first time the panel is laid out for a clip.
    if (centerPending && viewport.getHeight() > 0)
    {
        viewport.setViewPosition(0, juce::jmax(0, grid.yForPitch(60) - viewport.getHeight() / 2));
        centerPending = false;
    }
}
