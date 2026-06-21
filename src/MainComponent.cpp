#include "MainComponent.h"
#include "LookAndFeel.h"
#include "Project.h"
#include "BuildInfo.h"

MainComponent::MainComponent()
{
    // Apply custom dark Pro Tools style LookAndFeel
    setLookAndFeel(&DeepDAWLookAndFeel::getInstance());

    menuBar.setModel(this);
    addAndMakeVisible(menuBar);

    // Initialize audio device with sensible real-time defaults
    deviceManager.initialiseWithDefaultDevices(2, 2);

    transport = std::make_unique<TransportComponent>(deviceManager);
    addAndMakeVisible(transport.get());

    // The transport IS the device audio callback: it mixes clips + metronome to
    // the output and reads the input (for metering, and recording in Phase 3).
    deviceManager.addAudioCallback(transport.get());

    formatManager.registerBasicFormats();

    // Persistent location (not temp, which the OS can purge) so recorded clips
    // survive across sessions and reopen correctly.
    recordingsDir = juce::File::getSpecialLocation(juce::File::userMusicDirectory)
                        .getChildFile("DeepDAW Recordings");
    recordingsDir.createDirectory();

    addAudioButton.onClick = [this] { addTrack(TrackType::audio); };
    addMidiButton.onClick  = [this] { addTrack(TrackType::midi);  };
    addAndMakeVisible(addAudioButton);
    addAndMakeVisible(addMidiButton);

    zoomInButton.onClick  = [this] { applyZoom(1.25, timelineViewportCentreX()); };
    zoomOutButton.onClick = [this] { applyZoom(0.80, timelineViewportCentreX()); };
    addAndMakeVisible(zoomInButton);
    addAndMakeVisible(zoomOutButton);

    tracksHeaderLabel.setJustificationType(juce::Justification::centredLeft);
    tracksHeaderLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a8a8a));
    tracksHeaderLabel.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
    addAndMakeVisible(tracksHeaderLabel);

    statusBar.setText("DeepDAW  v" + juce::String(ProjectInfo::versionString)
                          + "   build " + DEEPDAW_BUILD_ID,
                      juce::dontSendNotification);
    statusBar.setJustificationType(juce::Justification::centredRight);
    statusBar.setColour(juce::Label::textColourId, juce::Colour(0xff707070));
    statusBar.setColour(juce::Label::backgroundColourId, juce::Colour(0xff252526));
    statusBar.setFont(juce::Font(juce::FontOptions(12.0f)));
    addAndMakeVisible(statusBar);

    // Track list (left) and timeline (right) each live in a scrollable viewport.
    // The track list hides its own scrollbar; the timeline owns the shared one.
    trackListViewport.setViewedComponent(&trackList, false);
    // Hide the track-list scrollbar (the timeline owns the visible one) but
    // still allow the mouse wheel to scroll it — the 3rd arg enables wheel
    // scrolling without a visible scrollbar, which a plain (false,false) drops.
    trackListViewport.setScrollBarsShown(false, false, true, false);
    addAndMakeVisible(trackListViewport);

    timelineViewport.setViewedComponent(&timeline, false);
    timelineViewport.setScrollBarsShown(true, true);
    addAndMakeVisible(timelineViewport);

    // The ruler is pinned above the lanes (not scrolled vertically); it mirrors
    // the timeline's horizontal scroll and shares its seek/zoom behaviour.
    addAndMakeVisible(ruler);
    ruler.onSeek        = [this](double beats)                { setPlayheadBeats(beats); };
    ruler.onZoomGesture = [this](float deltaY, float anchorX) { applyZoom(deltaY > 0.0f ? 1.15 : 1.0 / 1.15, (double) anchorX); };

    trackList.onTracksChanged = [this] { handleTracksChanged(); };
    trackList.onImportAudioRequested = [this](int trackId) { importAudioForTrack(trackId); };
    trackList.inputLevelProvider = [this] { return transport->getInputLevel(); };
    timeline.onFileDropped = [this](int trackId, double startBeat, const juce::File& file)
    {
        addImportedClip(trackId, startBeat, file);
    };

    trackListViewport.onVisibleAreaChanged = [this](juce::Rectangle<int> area)
    {
        syncVerticalScroll(area.getY(), true);
    };
    timelineViewport.onVisibleAreaChanged = [this](juce::Rectangle<int> area)
    {
        syncVerticalScroll(area.getY(), false);
        ruler.setScrollX(area.getX());
    };

    // Playhead: the timeline reports clicks, the transport reports play state,
    // and a timer advances the position while playing.
    timeline.onSeek = [this](double beats) { setPlayheadBeats(beats); };

    timeline.onZoomGesture = [this](float deltaY, float anchorContentX)
    {
        applyZoom(deltaY > 0.0f ? 1.15 : 1.0 / 1.15, (double) anchorContentX);
    };

    transport->onRecordingChanged = [this](bool recording)
    {
        if (recording) startRecordingFlow();
        else           stopRecordingFlow();
    };

    transport->onReturnToZero = [this] { setPlayheadBeats(0.0); };
    transport->onPlayingChanged = [this](bool playing)
    {
        if (playing) startTimerHz(60); // follow the engine's position while playing
        else         stopTimer();
    };

    setSize(1200, 700);
}

MainComponent::~MainComponent()
{
    // Stop the audio thread touching the transport before anything is torn down.
    deviceManager.removeAudioCallback(transport.get());

    stopTimer();
    menuBar.setModel(nullptr);
    setLookAndFeel(nullptr);
}

void MainComponent::setPlayheadBeats(double beats)
{
    // Move both the engine's play position and the visual playhead (used for
    // seeking/rewind, which must agree whether or not we are playing).
    playheadBeats = juce::jlimit(0.0, (double) TimelineComponent::numBars * 4.0, beats);
    transport->setPositionBeats(playheadBeats);
    timeline.setPlayheadBeats(playheadBeats);
    ruler.repaint();
}

void MainComponent::timerCallback()
{
    // While playing, the audio engine owns the position; the playhead follows.
    playheadBeats = juce::jlimit(0.0, (double) TimelineComponent::numBars * 4.0,
                                 transport->getPositionBeats());
    timeline.setPlayheadBeats(playheadBeats);
    ruler.repaint();

    if (! transport->getIsPlaying())
        stopTimer();
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "File" };
}

juce::PopupMenu MainComponent::getMenuForIndex(int, const juce::String& menuName)
{
    juce::PopupMenu menu;

    if (menuName == "File")
    {
        menu.addItem(1, "New");
        menu.addItem(2, "Open...");
        menu.addSeparator();
        menu.addItem(3, "Save");
        menu.addItem(4, "Save As...");
        menu.addSeparator();
        menu.addItem(5, "Exit");
    }

    return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int)
{
    switch (menuItemID)
    {
        case 1: newProject();    break;
        case 2: openProject();   break;
        case 3: saveProject();   break;
        case 4: saveProjectAs(); break;
        case 5: juce::JUCEApplication::getInstance()->systemRequestedQuit(); break;
        default: break;
    }
}

void MainComponent::newProject()
{
    trackList.clear();
    transport->setBpm(120.0);
    audioCache.clear();
    currentProjectFile = juce::File();
    setWindowTitle("Untitled");
}

void MainComponent::openProject()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Open Project", juce::File(), "*.deepdaw");

    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File())
            return;

        ProjectData data;
        if (ProjectIO::loadFromFile(file, data))
        {
            transport->setBpm(data.bpm);
            timeline.setZoom(data.zoom);
            trackList.load(data.tracks);          // triggers updateContentBounds at the new zoom
            timelineViewport.setViewPosition(0, 0); // back to the start of the arrangement
            ruler.repaint();
            currentProjectFile = file;
            setWindowTitle(file.getFileNameWithoutExtension());
        }
    });
}

void MainComponent::saveProject()
{
    if (currentProjectFile == juce::File())
        saveProjectAs();
    else
        writeProjectTo(currentProjectFile);
}

void MainComponent::saveProjectAs()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Save Project", juce::File(), "*.deepdaw");

    auto chooserFlags = juce::FileBrowserComponent::saveMode
                      | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file == juce::File())
            return;

        if (! file.hasFileExtension("deepdaw"))
            file = file.withFileExtension("deepdaw");

        writeProjectTo(file);
    });
}

void MainComponent::writeProjectTo(const juce::File& file)
{
    ProjectData data;
    data.bpm  = transport->getBpm();
    data.zoom = timeline.getZoom();
    for (const auto& t : trackList.getTracks())
        data.tracks.push_back(*t);

    if (ProjectIO::saveToFile(file, data))
    {
        currentProjectFile = file;
        setWindowTitle(file.getFileNameWithoutExtension());
    }
}

void MainComponent::setWindowTitle(const juce::String& projectName)
{
    if (auto* window = findParentComponentOfClass<juce::DocumentWindow>())
        window->setName("DeepDAW - " + projectName);
}

void MainComponent::addTrack(TrackType type)
{
    trackList.addTrack(type);
}

void MainComponent::handleTracksChanged()
{
    // The track list has already resized itself to its new row count; mirror
    // that in the timeline (including its clip blocks), reload the audio
    // engine's clips, and refresh bounds.
    timeline.rebuildClips();
    reloadEngineClips();
    updateContentBounds();
    timeline.repaint();
}

void MainComponent::startRecordingFlow()
{
    // Record into the first record-armed audio track.
    recordingTrackId = -1;
    for (const auto& t : trackList.getTracks())
        if (t->armed && t->type == TrackType::audio)
        {
            recordingTrackId = t->id;
            break;
        }

    if (recordingTrackId < 0)
        return; // nothing armed — record button just plays

    recordingFile = recordingsDir.getChildFile(
        "take_" + juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S") + ".wav");
    transport->startRecording(recordingFile);
    transport->startCountIn(); // 1-bar count-in before capture begins
}

void MainComponent::stopRecordingFlow()
{
    transport->stopRecording(); // finalizes the take file

    // Turn the take into a clip on the armed track, at the beat where recording
    // began. addImportedClip reads the file's length and adds it for playback.
    if (recordingTrackId >= 0 && recordingFile.existsAsFile() && recordingFile.getSize() > 1024)
        addImportedClip(recordingTrackId, transport->getRecordStartBeat(), recordingFile);

    recordingTrackId = -1;
    recordingFile = juce::File();
}

void MainComponent::reloadEngineClips()
{
    // Standard solo rule: if any track is soloed, only soloed tracks sound;
    // otherwise muted tracks are silent.
    bool anySolo = false;
    for (const auto& t : trackList.getTracks())
        if (t->soloed) { anySolo = true; break; }

    std::vector<LoadedClip> loaded;
    for (const auto& track : trackList.getTracks())
    {
        const bool trackAudible = anySolo ? track->soloed : ! track->muted;

        for (const auto& clip : track->clips)
        {
            const std::string key = clip.filePath.toStdString();
            auto it = audioCache.find(key);

            if (it == audioCache.end())
            {
                juce::File file(clip.filePath);
                std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
                if (reader == nullptr || reader->lengthInSamples <= 0)
                    continue;

                auto buf = std::make_shared<juce::AudioBuffer<float>>((int) reader->numChannels,
                                                                      (int) reader->lengthInSamples);
                reader->read(buf.get(), 0, (int) reader->lengthInSamples, 0, true, true);
                it = audioCache.emplace(key, CachedAudio{ buf, reader->sampleRate }).first;
            }

            LoadedClip lc;
            lc.audio          = it->second.buffer;
            lc.fileSampleRate = it->second.sampleRate;
            lc.startBeat      = clip.startBeat;
            lc.audible        = trackAudible;
            lc.gain           = track->gain;
            lc.pan            = track->pan;
            loaded.push_back(std::move(lc));
        }
    }

    transport->setClips(std::move(loaded));
}

void MainComponent::importAudioForTrack(int trackId)
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Import Audio", juce::File(),
        formatManager.getWildcardForAllFormats());

    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this, trackId](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file != juce::File())
            addImportedClip(trackId, playheadBeats, file); // place at the playhead
    });
}

void MainComponent::addImportedClip(int trackId, double startBeat, const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr || reader->sampleRate <= 0.0)
        return;

    const double seconds     = (double) reader->lengthInSamples / reader->sampleRate;
    const double lengthBeats = seconds * (transport->getBpm() / 60.0);

    Clip clip;
    clip.name        = file.getFileNameWithoutExtension();
    clip.filePath    = file.getFullPathName();
    clip.startBeat   = juce::jmax(0.0, startBeat);
    clip.lengthBeats = lengthBeats;

    trackList.addClip(trackId, clip); // notifies -> rebuilds clip blocks
}

void MainComponent::syncVerticalScroll(int y, bool fromTrackList)
{
    if (syncingScroll)
        return;

    const juce::ScopedValueSetter<bool> guard(syncingScroll, true);

    if (fromTrackList)
        timelineViewport.setViewPosition(timelineViewport.getViewPositionX(), y);
    else
        trackListViewport.setViewPosition(0, y);
}

double MainComponent::timelineViewportCentreX() const
{
    return timelineViewport.getViewPositionX() + timelineViewport.getWidth() / 2.0;
}

void MainComponent::applyZoom(double factor, double anchorContentX)
{
    // Remember which beat sits under the anchor and where the anchor is within
    // the viewport, then restore that after rescaling so the point stays put.
    const double beatAtAnchor   = anchorContentX / timeline.getPixelsPerBeat();
    const double anchorInView   = anchorContentX - timelineViewport.getViewPositionX();

    timeline.setZoom(timeline.getZoom() * factor);

    const double newContentX = beatAtAnchor * timeline.getPixelsPerBeat();
    const int newScrollX = juce::jmax(0, juce::roundToInt(newContentX - anchorInView));
    timelineViewport.setViewPosition(newScrollX, timelineViewport.getViewPositionY());
    ruler.repaint(); // bar spacing changed
}

void MainComponent::updateContentBounds()
{
    timeline.updateContentSize();

    // Keep the track-list content the width of its viewport, and at least as
    // tall as the viewport so the background fills when there are few tracks.
    trackList.setSize(trackListViewport.getWidth(),
                      juce::jmax(trackList.getHeight(), trackListViewport.getHeight()));

    ruler.repaint(); // track count / scale may have changed
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e)); // Deep dark background
}

void MainComponent::resized()
{
    auto area = getLocalBounds();

    menuBar.setBounds(area.removeFromTop(24));
    statusBar.setBounds(area.removeFromBottom(22));
    transport->setBounds(area.removeFromTop(48));

    auto controls = area.removeFromTop(36).reduced(8, 4);
    addAudioButton.setBounds(controls.removeFromLeft(120));
    controls.removeFromLeft(8);
    addMidiButton.setBounds(controls.removeFromLeft(120));

    // Zoom controls live at the right edge of the strip.
    zoomInButton.setBounds(controls.removeFromRight(36));
    controls.removeFromRight(6);
    zoomOutButton.setBounds(controls.removeFromRight(36));

    auto left = area.removeFromLeft(trackListWidth);
    tracksHeaderLabel.setBounds(left.removeFromTop(TimelineComponent::rulerHeight)
                                    .withTrimmedLeft(10));
    trackListViewport.setBounds(left);

    // Right pane: pinned ruler strip on top, scrolling lanes below.
    ruler.setBounds(area.removeFromTop(TimelineComponent::rulerHeight));
    timelineViewport.setBounds(area);

    updateContentBounds();
}
