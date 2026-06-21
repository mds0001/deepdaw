#include "TransportComponent.h"
#include "LookAndFeel.h"

TransportComponent::TransportComponent(juce::AudioDeviceManager& dm)
    : deviceManager(dm)
{
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(recordButton);
    addAndMakeVisible(rewindButton);
    addAndMakeVisible(forwardButton);
    addAndMakeVisible(metronomeButton);
    addAndMakeVisible(monitorButton);

    playButton.addListener(this);
    stopButton.addListener(this);
    recordButton.addListener(this);
    rewindButton.addListener(this);
    metronomeButton.addListener(this);
    monitorButton.addListener(this);

    // Record button fills red while recording (its toggle state).
    recordButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff1744));
    recordButton.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    bpmSlider.setRange(40.0, 240.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    bpmSlider.onValueChange = [this] { setBpm(bpmSlider.getValue()); };
    addAndMakeVisible(bpmSlider);

    bpmLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bpmLabel);

    masterSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterSlider.setRange(-60.0, 6.0, 0.1); // dB; -60 = -inf, 0 = unity
    masterSlider.setDoubleClickReturnValue(true, 0.0);
    masterSlider.setValue(0.0, juce::dontSendNotification);
    masterSlider.setColour(juce::Slider::thumbColourId, DeepDAWLookAndFeel::getInstance().getAccentColour());
    masterSlider.onValueChange = [this]
    {
        const float gain = (float) juce::Decibels::decibelsToGain(masterSlider.getValue(), -60.0);
        setMasterGain(gain);
        if (onMasterGainChanged) onMasterGainChanged(gain);
    };
    addAndMakeVisible(masterSlider);

    masterLabel.setJustificationType(juce::Justification::centredRight);
    masterLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8a8a8a));
    addAndMakeVisible(masterLabel);

    startTimerHz(30); // input meter refresh
    backgroundThread.startThread();
}

TransportComponent::~TransportComponent()
{
    stopRecording();
    backgroundThread.stopThread(2000);
}

void TransportComponent::timerCallback()
{
    // Smooth the meters: jump up to new peaks, decay down gently.
    meterDisplay = juce::jmax(inputLevel.load(), meterDisplay * 0.80f);
    repaint(meterBounds);

    const float mp = masterPeak.load();
    outMeterDisplay = juce::jmax(mp, outMeterDisplay * 0.80f);
    outMeterHold    = juce::jmax(mp, outMeterHold - 0.010f);
    repaint(outMeterBounds);
}

void TransportComponent::setBpm(double newBpm)
{
    // Preserve the play position in beats across a tempo change.
    const double beats = getPositionBeats();

    currentBPM = juce::jlimit(40.0, 240.0, newBpm);
    bpmSlider.setValue(currentBPM, juce::dontSendNotification);
    bpmLabel.setText(juce::String((int) currentBPM), juce::dontSendNotification);

    if (sampleRate > 0.0)
        samplesPerBeat = static_cast<int>((60.0 / currentBPM) * sampleRate);

    setPositionBeats(beats);
}

double TransportComponent::getPositionBeats() const
{
    if (sampleRate <= 0.0)
        return 0.0;

    return (double) positionSamples.load() / sampleRate * (currentBPM / 60.0);
}

void TransportComponent::setPositionBeats(double beats)
{
    const double sr = sampleRate > 0.0 ? sampleRate : 48000.0;
    positionSamples.store((int64_t) (juce::jmax(0.0, beats) * (60.0 / currentBPM) * sr));
}

void TransportComponent::setClips(std::vector<LoadedClip>&& newClips)
{
    const juce::SpinLock::ScopedLockType sl(clipLock);
    loadedClips = std::move(newClips);
}

void TransportComponent::paint(juce::Graphics& g)
{
    g.fillAll(DeepDAWLookAndFeel::getInstance().getPanelColour());
    g.setColour(DeepDAWLookAndFeel::getInstance().getAccentColour());
    g.drawRect(getLocalBounds(), 1);

    // Input level meter.
    if (! meterBounds.isEmpty())
    {
        g.setColour(juce::Colour(0xff8a8a8a));
        g.setFont(11.0f);
        auto labelArea = meterBounds.withWidth(20).translated(-22, 0);
        g.drawText("IN", labelArea, juce::Justification::centredRight);

        g.setColour(juce::Colour(0xff141414));
        g.fillRect(meterBounds);

        const float level = juce::jlimit(0.0f, 1.0f, meterDisplay);
        auto fill = meterBounds.toFloat().withWidth(meterBounds.getWidth() * level);
        g.setColour(level > 0.9f ? juce::Colour(0xffff1744)
                                 : juce::Colour(0xff00c853)); // green, red near clip
        g.fillRect(fill);

        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(meterBounds, 1);
    }

    // Master output meter (right of the OUT slider), with a peak-hold tick.
    if (! outMeterBounds.isEmpty())
    {
        g.setColour(juce::Colour(0xff141414));
        g.fillRect(outMeterBounds);

        const float level = juce::jlimit(0.0f, 1.0f, outMeterDisplay);
        auto fill = outMeterBounds.toFloat().withWidth(outMeterBounds.getWidth() * level);
        g.setColour(level > 0.9f ? juce::Colour(0xffff1744) : juce::Colour(0xff00c853));
        g.fillRect(fill);

        const float hold = juce::jlimit(0.0f, 1.0f, outMeterHold);
        if (hold > 0.0f)
        {
            const int tx = outMeterBounds.getX() + juce::roundToInt(outMeterBounds.getWidth() * hold);
            g.setColour(hold > 0.9f ? juce::Colour(0xffff1744) : juce::Colour(0xffd0d0d0));
            g.fillRect(tx - 1, outMeterBounds.getY(), 2, outMeterBounds.getHeight());
        }

        g.setColour(juce::Colour(0xff3a3a3a));
        g.drawRect(outMeterBounds, 1);
    }
}

void TransportComponent::resized()
{
    auto area = getLocalBounds().reduced(8);
    const int buttonWidth = 70;

    rewindButton.setBounds(area.removeFromLeft(buttonWidth));
    playButton.setBounds(area.removeFromLeft(buttonWidth));
    stopButton.setBounds(area.removeFromLeft(buttonWidth));
    recordButton.setBounds(area.removeFromLeft(buttonWidth));
    forwardButton.setBounds(area.removeFromLeft(buttonWidth));

    area.removeFromLeft(20);
    metronomeButton.setBounds(area.removeFromLeft(100));
    area.removeFromLeft(8);
    monitorButton.setBounds(area.removeFromLeft(90));

    area.removeFromLeft(14);
    bpmLabel.setBounds(area.removeFromLeft(36));
    bpmSlider.setBounds(area.removeFromLeft(140));

    area.removeFromLeft(14);
    masterLabel.setBounds(area.removeFromLeft(34));
    masterSlider.setBounds(area.removeFromLeft(96));
    area.removeFromLeft(6);
    outMeterBounds = area.removeFromLeft(56).reduced(0, 10); // master output meter

    area.removeFromLeft(24); // gap + room for the "IN" label
    meterBounds = area.removeFromLeft(100).reduced(0, 10);
}

void TransportComponent::buttonClicked(juce::Button* button)
{
    const bool wasPlaying = isPlaying;
    const bool wasRecording = isRecording;

    if (button == &playButton)
    {
        isPlaying = true;
        isRecording = false;
    }
    else if (button == &stopButton)
    {
        isPlaying = false;
        isRecording = false;
        positionSamples.store(0);
        if (onReturnToZero) onReturnToZero();
    }
    else if (button == &recordButton)
    {
        isRecording = !isRecording;
        isPlaying = isRecording;
    }
    else if (button == &rewindButton)
    {
        positionSamples.store(0);
        if (onReturnToZero) onReturnToZero();
    }
    else if (button == &metronomeButton)
    {
        metronomeEnabled = metronomeButton.getToggleState();
    }
    else if (button == &monitorButton)
    {
        monitorEnabled = monitorButton.getToggleState();
    }

    // Recording change first, so the host opens/closes the take file at the
    // current position before playback start/stop is handled.
    if (isRecording != wasRecording && onRecordingChanged)
        onRecordingChanged(isRecording);

    if (isPlaying != wasPlaying && onPlayingChanged)
        onPlayingChanged(isPlaying);

    updateTransportState();
}

void TransportComponent::updateTransportState()
{
    playButton.setToggleState(isPlaying && !isRecording, juce::dontSendNotification);
    recordButton.setToggleState(isRecording, juce::dontSendNotification);
    metronomeButton.setToggleState(metronomeEnabled, juce::dontSendNotification);
}

void TransportComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device->getCurrentSampleRate();
    if (sampleRate <= 0.0)
        sampleRate = 48000.0;
    samplesPerBeat = static_cast<int>((60.0 / currentBPM) * sampleRate);
    deviceInputChannels = juce::jmax(1, device->getActiveInputChannels().countNumberOfSetBits());
}

void TransportComponent::audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                                          float* const* outputChannelData, int numOutputChannels,
                                                          int numSamples,
                                                          const juce::AudioIODeviceCallbackContext&)
{
    juce::AudioBuffer<float> output(outputChannelData, numOutputChannels, numSamples);
    output.clear();

    const bool countingIn = countInSamplesLeft > 0;
    if (countingIn)
        renderCountIn(output); // clicks only; play position frozen
    else
        renderNextBlock(output);

    // Master output gain, ramped across the block to avoid zipper noise.
    const float targetMaster = masterGain.load();
    output.applyGainRamp(0, numSamples, currentMasterGain, targetMaster);
    currentMasterGain = targetMaster;

    // Input peak for the meter.
    float peak = 0.0f;
    for (int ch = 0; ch < numInputChannels; ++ch)
        if (const float* in = inputChannelData[ch])
            for (int i = 0; i < numSamples; ++i)
                peak = juce::jmax(peak, std::abs(in[i]));
    inputLevel.store(peak);

    // Input monitoring: pass the input through to the output so the user hears
    // themselves (off by default — this can feed back through speakers).
    if (monitorEnabled)
        for (int ch = 0; ch < numOutputChannels; ++ch)
        {
            const int srcCh = juce::jmin(ch, numInputChannels - 1);
            if (srcCh >= 0 && inputChannelData[srcCh] != nullptr && outputChannelData[ch] != nullptr)
                juce::FloatVectorOperations::add(outputChannelData[ch], inputChannelData[srcCh], numSamples);
        }

    // Master output peak (post everything that reaches the speakers).
    float mpeak = 0.0f;
    for (int ch = 0; ch < numOutputChannels; ++ch)
        if (const float* out = outputChannelData[ch])
            for (int i = 0; i < numSamples; ++i)
                mpeak = juce::jmax(mpeak, std::abs(out[i]));
    masterPeak.store(mpeak);

    // Stream the input to the recording file (ThreadedWriter buffers; the disk
    // write happens on the background thread). Skipped during the count-in.
    if (! countingIn)
    {
        const juce::ScopedLock sl(writerLock);
        if (auto* w = activeWriter.load(); w != nullptr && numInputChannels > 0)
            w->write(inputChannelData, numSamples);
    }
}

void TransportComponent::renderCountIn(juce::AudioBuffer<float>& output)
{
    const int numSamples = output.getNumSamples();
    const int numOutCh   = output.getNumChannels();

    if (samplesPerBeat > 0)
        for (int i = 0; i < numSamples; ++i)
            if ((countInElapsed + i) % samplesPerBeat < 200)
            {
                const float click = 0.3f * (float) std::sin(2.0 * juce::MathConstants<double>::pi
                                                            * 880.0 * (double) (countInElapsed + i) / sampleRate);
                for (int ch = 0; ch < numOutCh; ++ch)
                    output.addSample(ch, i, click);
            }

    countInElapsed     += numSamples;
    countInSamplesLeft -= numSamples; // position is intentionally not advanced
}

void TransportComponent::startCountIn()
{
    countInElapsed = 0;
    countInSamplesLeft = (int64_t) (samplesPerBeat) * 4; // one bar
}

void TransportComponent::audioDeviceStopped()
{
    inputLevel.store(0.0f);
}

void TransportComponent::startRecording(const juce::File& file)
{
    stopRecording();

    if (sampleRate <= 0.0)
        return;

    file.deleteFile();
    if (auto fileStream = std::unique_ptr<juce::FileOutputStream>(file.createOutputStream()))
    {
        juce::WavAudioFormat wav;
        if (auto* writer = wav.createWriterFor(fileStream.get(), sampleRate,
                                               (unsigned int) juce::jmax(1, deviceInputChannels),
                                               16, {}, 0))
        {
            fileStream.release(); // the writer owns the stream now
            threadedWriter.reset(new juce::AudioFormatWriter::ThreadedWriter(writer, backgroundThread, 32768));
            recordStartSample = positionSamples.load();

            const juce::ScopedLock sl(writerLock);
            activeWriter = threadedWriter.get();
        }
    }
}

void TransportComponent::stopRecording()
{
    countInSamplesLeft = 0; // cancel any count-in in progress
    {
        const juce::ScopedLock sl(writerLock);
        activeWriter = nullptr;
    }
    threadedWriter.reset(); // flushes + closes the file
}

double TransportComponent::getRecordStartBeat() const
{
    if (sampleRate <= 0.0)
        return 0.0;

    return (double) recordStartSample / sampleRate * (currentBPM / 60.0);
}

void TransportComponent::renderNextBlock(juce::AudioBuffer<float>& output)
{
    if (! isPlaying)
    {
        for (auto& p : trackPeaks) p.store(0.0f); // meters decay to silence when stopped
        return;
    }

    const int numSamples = output.getNumSamples();
    const int numOutCh   = output.getNumChannels();
    const int64_t pos    = positionSamples.load();
    const double sr      = sampleRate;
    const double secsPerBeat = 60.0 / currentBPM;

    float slotPeak[maxMeteredTracks] = { 0.0f }; // per-track output peak this block

    // Mix clips by reading each one at the play position (interpolated).
    {
        const juce::SpinLock::ScopedTryLockType sl(clipLock);
        if (sl.isLocked())
        {
            for (const auto& clip : loadedClips)
            {
                if (! clip.audible || clip.audio == nullptr)
                    continue;

                const auto& buf = *clip.audio;
                const int clipLen = buf.getNumSamples();
                const int clipCh  = buf.getNumChannels();
                if (clipLen <= 0 || clipCh <= 0)
                    continue;

                const double clipStartSec = clip.startBeat * secsPerBeat;
                const double clipDurSec = clipLen / clip.fileSampleRate;

                // Track volume + equal-power pan (left/right gains).
                const float g     = clip.gain;
                const float angle = (juce::jlimit(-1.0f, 1.0f, clip.pan) + 1.0f) * 0.25f
                                        * juce::MathConstants<float>::pi;
                const float lGain = std::cos(angle);
                const float rGain = std::sin(angle);
                const int slot = juce::jlimit(0, maxMeteredTracks - 1, clip.trackSlot);
                float clipPeak = 0.0f;

                for (int i = 0; i < numSamples; ++i)
                {
                    const double local = (double) (pos + i) / sr - clipStartSec;
                    if (local < 0.0 || local >= clipDurSec)
                        continue;

                    const double srcIdx = local * clip.fileSampleRate;
                    const int i0   = (int) srcIdx;
                    const int i1   = juce::jmin(i0 + 1, clipLen - 1);
                    const float fr = (float) (srcIdx - i0);

                    for (int ch = 0; ch < numOutCh; ++ch)
                    {
                        const float* s = buf.getReadPointer(juce::jmin(ch, clipCh - 1));
                        const float panG = (numOutCh >= 2) ? (ch == 0 ? lGain : (ch == 1 ? rGain : 1.0f)) : 1.0f;
                        const float v = (s[i0] + fr * (s[i1] - s[i0])) * g;
                        output.addSample(ch, i, v * panG);
                        clipPeak = juce::jmax(clipPeak, std::abs(v));
                    }
                }

                slotPeak[slot] = juce::jmax(slotPeak[slot], clipPeak);
            }
        }
    }

    for (int s = 0; s < maxMeteredTracks; ++s)
        trackPeaks[(size_t) s].store(slotPeak[s]);

    // Metronome click on each beat.
    if (metronomeEnabled && samplesPerBeat > 0)
    {
        for (int i = 0; i < numSamples; ++i)
            if ((pos + i) % samplesPerBeat < 200)
            {
                const float click = 0.3f * (float) std::sin(2.0 * juce::MathConstants<double>::pi
                                                            * 880.0 * (double) (pos + i) / sr);
                for (int ch = 0; ch < numOutCh; ++ch)
                    output.addSample(ch, i, click);
            }
    }

    positionSamples.store(pos + numSamples);
}
