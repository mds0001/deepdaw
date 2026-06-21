#include "ClipComponent.h"

ClipComponent::ClipComponent(const Clip& clip, int indexOfTrack, int idOfTrack, int indexOfClip,
                             juce::Colour trackColour, bool clipIsMidi,
                             juce::AudioFormatManager& formatManager,
                             juce::AudioThumbnailCache& cache)
    : name(clip.name), colour(trackColour),
      startBeat(clip.startBeat), lengthBeats(clip.lengthBeats),
      trackIndex(indexOfTrack), trackId(idOfTrack), clipIndex(indexOfClip),
      isMidi(clipIsMidi), notes(clip.notes),
      thumbnail(512, formatManager, cache)
{
    setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    setTitle(name); // accessible name

    if (! isMidi)
    {
        juce::File file(clip.filePath);
        if (file.existsAsFile())
            thumbnail.setSource(new juce::FileInputSource(file)); // loads async, repaints on change
        else
            fileMissing = true;

        thumbnail.addChangeListener(this);
    }
}

ClipComponent::~ClipComponent()
{
    thumbnail.removeChangeListener(this);
}

void ClipComponent::changeListenerCallback(juce::ChangeBroadcaster*)
{
    repaint(); // thumbnail finished/progressed loading
}

void ClipComponent::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Delete Clip");
        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this),
            [this](int result)
            {
                if (result == 1 && onDeleteRequested)
                    onDeleteRequested(this);
            });
        return;
    }

    if (onDragStart)
        onDragStart(this, e);
}

void ClipComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() && onDrag)
        onDrag(this, e);
}

void ClipComponent::mouseUp(const juce::MouseEvent& e)
{
    if (! e.mods.isPopupMenu() && onDragEnd)
        onDragEnd(this, e);
}

void ClipComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(1.0f);

    g.setColour((fileMissing ? juce::Colour(0xff404040) : colour).withAlpha(0.30f));
    g.fillRoundedRectangle(bounds, 3.0f);

    // Content below the name strip: a waveform (audio) or a note preview (MIDI).
    auto contentArea = getLocalBounds().reduced(3).withTrimmedTop(14);
    if (isMidi)
    {
        if (notes.empty())
        {
            g.setColour(juce::Colours::white.withAlpha(0.35f));
            g.setFont(10.0f);
            g.drawText("(empty)", contentArea, juce::Justification::centred, false);
        }
        else if (contentArea.getHeight() > 2 && lengthBeats > 0.0)
        {
            int minPitch = 127, maxPitch = 0;
            for (const auto& n : notes)
            {
                minPitch = juce::jmin(minPitch, n.pitch);
                maxPitch = juce::jmax(maxPitch, n.pitch);
            }
            const int span = juce::jmax(11, maxPitch - minPitch); // at least an octave of headroom
            const float rowH = contentArea.getHeight() / (float) (span + 1);

            g.setColour(colour.brighter(0.6f).withAlpha(0.95f));
            for (const auto& n : notes)
            {
                const float x = contentArea.getX() + (float) (n.startBeat / lengthBeats) * contentArea.getWidth();
                const float w = juce::jmax(1.5f, (float) (n.lengthBeats / lengthBeats) * contentArea.getWidth());
                const float y = contentArea.getBottom() - (n.pitch - minPitch + 1) * rowH;
                g.fillRect(x, y, juce::jmin(w, contentArea.getRight() - x), juce::jmax(1.0f, rowH - 1.0f));
            }
        }
    }
    else if (thumbnail.getTotalLength() > 0.0 && contentArea.getHeight() > 2)
    {
        g.setColour(colour.brighter(0.5f).withAlpha(0.9f));
        thumbnail.drawChannels(g, contentArea, 0.0, thumbnail.getTotalLength(), 1.0f);
    }

    g.setColour(fileMissing ? juce::Colour(0xff6a6a6a) : colour);
    g.drawRoundedRectangle(bounds, 3.0f, 1.2f);

    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.setFont(11.0f);
    g.drawText(fileMissing ? (name + "  (offline)") : name,
               getLocalBounds().reduced(5, 2).removeFromTop(13),
               juce::Justification::topLeft, true);
}
