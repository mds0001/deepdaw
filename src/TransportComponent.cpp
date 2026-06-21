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

    playButton.addListener(this);
    stopButton.addListener(this);
    recordButton.addListener(this);
    rewindButton.addListener(this);
    metronomeButton.addListener(this);

    bpmSlider.setRange(40.0, 240.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    bpmSlider.onValueChange = [this] { setBpm(bpmSlider.getValue()); };
    addAndMakeVisible(bpmSlider);

    bpmLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bpmLabel);
}

TransportComponent::~TransportComponent() = default;

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

    area.removeFromLeft(20);
    bpmLabel.setBounds(area.removeFromLeft(40));
    bpmSlider.setBounds(area.removeFromLeft(150));
}

void TransportComponent::buttonClicked(juce::Button* button)
{
    const bool wasPlaying = isPlaying;

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

void TransportComponent::prepareToPlay(int, double newSampleRate)
{
    sampleRate = newSampleRate;
    samplesPerBeat = static_cast<int>((60.0 / currentBPM) * sampleRate);
}

void TransportComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    if (! isPlaying)
        return;

    auto* buffer        = bufferToFill.buffer;
    const int start     = bufferToFill.startSample;
    const int numSamples = bufferToFill.numSamples;
    const int numOutCh  = buffer->getNumChannels();
    const int64_t pos   = positionSamples.load();
    const double sr     = sampleRate;
    const double secsPerBeat = 60.0 / currentBPM;

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
                        buffer->addSample(ch, start + i, s[i0] + fr * (s[i1] - s[i0]));
                    }
                }
            }
        }
    }

    // Metronome click on each beat.
    if (metronomeEnabled && samplesPerBeat > 0)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if ((pos + i) % samplesPerBeat < 200)
            {
                const float click = 0.3f * (float) std::sin(2.0 * juce::MathConstants<double>::pi
                                                            * 880.0 * (double) (pos + i) / sr);
                for (int ch = 0; ch < numOutCh; ++ch)
                    buffer->addSample(ch, start + i, click);
            }
        }
    }

    positionSamples.store(pos + numSamples);
}

void TransportComponent::releaseResources()
{
}
