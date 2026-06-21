#pragma once

#include <JuceHeader.h>

class TransportComponent : public juce::Component,
                           public juce::Button::Listener,
                           public juce::AudioSource
{
public:
    explicit TransportComponent(juce::AudioDeviceManager& deviceManager);
    ~TransportComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button*) override;

    // AudioSource
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    double getBpm() const { return currentBPM; }
    void setBpm(double newBpm);

    bool getIsPlaying() const { return isPlaying; }

    // Fired when playback starts/stops (Play, Stop, Record).
    std::function<void(bool isNowPlaying)> onPlayingChanged;
    // Fired when the position should jump back to the start (Stop, Rewind).
    std::function<void()> onReturnToZero;

private:
    void updateTransportState();
    void toggleMetronome();

    juce::AudioDeviceManager& deviceManager;

    juce::TextButton playButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton recordButton{"Record"};
    juce::TextButton rewindButton{"<<"};
    juce::TextButton forwardButton{">>"};
    juce::ToggleButton metronomeButton{"Metronome"};

    juce::Slider bpmSlider;
    juce::Label bpmLabel{"BPM", "120"};

    bool isPlaying = false;
    bool isRecording = false;
    bool metronomeEnabled = false;
    double currentBPM = 120.0;
    double sampleRate = 48000.0;
    int64_t currentSample = 0;
    int samplesPerBeat = 0;

    juce::ToneGeneratorAudioSource metronomeSource;
    juce::MixerAudioSource mixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportComponent)
};