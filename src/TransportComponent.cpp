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
    metronomeButton.addListener(this);

    bpmSlider.setRange(40.0, 240.0, 1.0);
    bpmSlider.setValue(120.0);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    bpmSlider.onValueChange = [this] { currentBPM = bpmSlider.getValue(); };
    addAndMakeVisible(bpmSlider);

    bpmLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(bpmLabel);
}

TransportComponent::~TransportComponent() = default;

void TransportComponent::setBpm(double newBpm)
{
    currentBPM = juce::jlimit(40.0, 240.0, newBpm);
    bpmSlider.setValue(currentBPM, juce::dontSendNotification);
    bpmLabel.setText(juce::String((int) currentBPM), juce::dontSendNotification);

    if (sampleRate > 0.0)
        samplesPerBeat = static_cast<int>((60.0 / currentBPM) * sampleRate);
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
    if (button == &playButton)
    {
        isPlaying = true;
        isRecording = false;
    }
    else if (button == &stopButton)
    {
        isPlaying = false;
        isRecording = false;
        currentSample = 0;
    }
    else if (button == &recordButton)
    {
        isRecording = !isRecording;
        isPlaying = isRecording;
    }
    else if (button == &metronomeButton)
    {
        toggleMetronome();
    }

    updateTransportState();
}

void TransportComponent::updateTransportState()
{
    playButton.setToggleState(isPlaying && !isRecording, juce::dontSendNotification);
    recordButton.setToggleState(isRecording, juce::dontSendNotification);
    metronomeButton.setToggleState(metronomeEnabled, juce::dontSendNotification);
}

void TransportComponent::toggleMetronome()
{
    metronomeEnabled = !metronomeEnabled;

    if (metronomeEnabled)
    {
        metronomeSource.setFrequency(880.0); // High click
        mixer.addInputSource(&metronomeSource, false);
    }
    else
    {
        mixer.removeAllInputs();
    }
}

void TransportComponent::prepareToPlay(int samplesPerBlockExpected, double newSampleRate)
{
    sampleRate = newSampleRate;
    samplesPerBeat = static_cast<int>((60.0 / currentBPM) * sampleRate);
    metronomeSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    mixer.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void TransportComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();

    if (isPlaying && metronomeEnabled)
    {
        // Simple metronome click generation (real-time safe)
        auto* left = bufferToFill.buffer->getWritePointer(0, bufferToFill.startSample);
        auto* right = bufferToFill.buffer->getWritePointer(1, bufferToFill.startSample);

        for (int i = 0; i < bufferToFill.numSamples; ++i)
        {
            if ((currentSample + i) % samplesPerBeat < 200) // Short click
            {
                float click = 0.3f * std::sin(2.0 * juce::MathConstants<double>::pi * 880.0 * (currentSample + i) / sampleRate);
                left[i] = click;
                right[i] = click;
            }
        }
    }

    currentSample += bufferToFill.numSamples;
    if (currentSample > samplesPerBeat * 4) // 4 beats loop for demo
        currentSample = 0;
}

void TransportComponent::releaseResources()
{
    metronomeSource.releaseResources();
    mixer.releaseResources();
}