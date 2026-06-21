#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <array>
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
    float gain = 1.0f;   // track volume (linear)
    float pan = 0.0f;    // track pan (-1..+1)
    int trackSlot = 0;   // track index, for per-track output metering
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

    // Per-track and master OUTPUT peaks (post-fader/pan), for the mixer meters.
    static constexpr int maxMeteredTracks = 64;
    float getTrackOutputLevel(int slot) const
    {
        return (slot >= 0 && slot < maxMeteredTracks) ? trackPeaks[(size_t) slot].load() : 0.0f;
    }
    float getMasterOutputLevel() const { return masterPeak.load(); }

    // Master output gain (linear).
    void setMasterGain(float g) { masterGain.store(juce::jlimit(0.0f, 2.0f, g)); }
    float getMasterGain() const { return masterGain.load(); }
    // Move the transport's OUT slider without firing its callback (mirror the
    // mixer's master fader). v is linear gain; the slider is calibrated in dB.
    void setMasterSliderValue(float v)
    {
        masterSlider.setValue(juce::Decibels::gainToDecibels((double) v, -60.0), juce::dontSendNotification);
    }

    // Fired when playback starts/stops (Play, Stop, Record).
    std::function<void(bool isNowPlaying)> onPlayingChanged;
    // Fired when recording starts/stops (host opens/closes the take file).
    std::function<void(bool isNowRecording)> onRecordingChanged;
    // Fired when the position should jump back to the start (Stop, Rewind).
    std::function<void()> onReturnToZero;
    // Fired when the transport's OUT slider moves (so the mixer can mirror it).
    std::function<void(float)> onMasterGainChanged;

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
    juce::Slider masterSlider;
    juce::Label masterLabel{"master", "OUT"};

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

    std::atomic<float> masterGain{ 1.0f };
    float currentMasterGain = 1.0f;             // ramped on the audio thread

    std::array<std::atomic<float>, maxMeteredTracks> trackPeaks{}; // per-track output peaks
    std::atomic<float> masterPeak{ 0.0f };      // final mix output peak
    float outMeterDisplay = 0.0f;               // smoothed master meter (UI thread)
    float outMeterHold = 0.0f;                  // slow-decaying peak-hold marker
    juce::Rectangle<int> outMeterBounds;

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
