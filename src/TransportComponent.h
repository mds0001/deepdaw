#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <memory>

// One audio clip preloaded into memory for real-time-safe playback. The buffer
// is shared (and immutable) so reloading the clip set after a mute/solo change
// reuses cached audio without re-reading the file. Stored at the file's own
// sample rate; the engine reads it by time, so a device sample-rate mismatch is
// handled by interpolation.
struct LoadedClip
{
    std::shared_ptr<const juce::AudioBuffer<float>> audio;
    double fileSampleRate = 44100.0;
    double startBeat = 0.0;
    bool audible = true; // false when the track is muted / not soloed
};

// Transport bar UI plus the audio engine. It is the device audio callback: it
// owns the authoritative play position (advanced on the audio thread), mixes
// the loaded clips at their timeline positions, generates the metronome, and
// monitors the input level. The UI playhead reads getPositionBeats().
class TransportComponent : public juce::Component,
                           public juce::Button::Listener,
                           public juce::AudioIODeviceCallback,
                           private juce::Timer
{
public:
    explicit TransportComponent(juce::AudioDeviceManager& deviceManager);
    ~TransportComponent() override;

    // Recording (Phase 3): stream the device input to a wav while recording.
    void startRecording(const juce::File& file);
    void stopRecording();
    double getRecordStartBeat() const; // play position captured when recording began

    // Play a 1-bar metronome count-in before recording begins (position frozen
    // during it, so the take lands at the cursor).
    void startCountIn();
    bool isCountingIn() const { return countInSamplesLeft > 0; }

    void paint(juce::Graphics&) override;
    void resized() override;
    void buttonClicked(juce::Button*) override;

    // AudioIODeviceCallback
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData, int numInputChannels,
                                          float* const* outputChannelData, int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceStopped() override;

    double getBpm() const { return currentBPM; }
    void setBpm(double newBpm);

    bool getIsPlaying() const { return isPlaying; }

    // Play position, in beats, shared with the UI playhead.
    double getPositionBeats() const;
    void setPositionBeats(double beats);

    // Replace the set of clips the engine plays (called on the message thread
    // when the project's clips change).
    void setClips(std::vector<LoadedClip>&& newClips);

    // Most recent input peak (0..1), for metering.
    float getInputLevel() const { return inputLevel.load(); }

    // Fired when playback starts/stops (Play, Stop, Record).
    std::function<void(bool isNowPlaying)> onPlayingChanged;
    // Fired when recording starts/stops (host opens/closes the take file).
    std::function<void(bool isNowRecording)> onRecordingChanged;
    // Fired when the position should jump back to the start (Stop, Rewind).
    std::function<void()> onReturnToZero;

private:
    void updateTransportState();
    void renderNextBlock(juce::AudioBuffer<float>& output); // clips + metronome
    void renderCountIn(juce::AudioBuffer<float>& output);   // count-in clicks (position frozen)
    void timerCallback() override;                          // drives the input meter

    juce::AudioDeviceManager& deviceManager;

    juce::TextButton playButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton recordButton{"Record"};
    juce::TextButton rewindButton{"<<"};
    juce::TextButton forwardButton{">>"};
    juce::ToggleButton metronomeButton{"Metronome"};
    juce::ToggleButton monitorButton{"Monitor"};

    juce::Slider bpmSlider;
    juce::Label bpmLabel{"BPM", "120"};

    bool isPlaying = false;
    bool isRecording = false;
    bool metronomeEnabled = false;
    bool monitorEnabled = false;
    double currentBPM = 120.0;
    double sampleRate = 48000.0;
    int samplesPerBeat = 0;

    std::atomic<int64_t> positionSamples{ 0 }; // authoritative play position
    int64_t countInSamplesLeft = 0;            // > 0 while counting in (position frozen)
    int64_t countInElapsed = 0;
    std::atomic<float> inputLevel{ 0.0f };     // last input peak, for the meter
    float meterDisplay = 0.0f;                  // smoothed meter value (UI thread)
    juce::Rectangle<int> meterBounds;

    juce::SpinLock clipLock;
    std::vector<LoadedClip> loadedClips;

    // Recording to disk (JUCE AudioRecordingDemo pattern).
    juce::TimeSliceThread backgroundThread{ "DeepDAW Recorder" };
    std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> threadedWriter;
    juce::CriticalSection writerLock;
    std::atomic<juce::AudioFormatWriter::ThreadedWriter*> activeWriter{ nullptr };
    int deviceInputChannels = 2;
    int64_t recordStartSample = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportComponent)
};
