#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>

// One audio clip preloaded into memory for real-time-safe playback. Stored at
// the file's own sample rate; the engine reads it by time so a sample-rate
// mismatch with the device is handled by interpolation.
struct LoadedClip
{
    juce::AudioBuffer<float> audio;
    double fileSampleRate = 44100.0;
    double startBeat = 0.0;
};

// Transport bar UI plus the audio engine. It is the AudioSource fed to the
// device: it owns the authoritative play position (advanced on the audio
// thread), mixes the loaded clips at their timeline positions, and generates
// the metronome. The UI playhead reads getPositionBeats().
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

    // Play position, in beats, shared with the UI playhead.
    double getPositionBeats() const;
    void setPositionBeats(double beats);

    // Replace the set of clips the engine plays (called on the message thread
    // when the project's clips change).
    void setClips(std::vector<LoadedClip>&& newClips);

    // Fired when playback starts/stops (Play, Stop, Record).
    std::function<void(bool isNowPlaying)> onPlayingChanged;
    // Fired when the position should jump back to the start (Stop, Rewind).
    std::function<void()> onReturnToZero;

private:
    void updateTransportState();

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
    int samplesPerBeat = 0;

    std::atomic<int64_t> positionSamples{ 0 }; // authoritative play position

    juce::SpinLock clipLock;
    std::vector<LoadedClip> loadedClips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportComponent)
};
