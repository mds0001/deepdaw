# Phase 3: Audio Recording Specification

**Version**: 1.0
**Status**: Complete – tagged `v0.4.0-phase3`
**Owner**: AI Agent (implementation + Git)
**Reviewer**: User (validation only)

## 1. Objectives
Capture live audio input into record-armed tracks, producing real clips that drop straight into the Phase 2 timeline. Building on the existing transport, playhead, clips and playback:

- Capture the audio **device input** in the engine (the current output-only path does not expose input).
- **Record-arm** tracks (the `R` toggle already exists) and optionally **monitor** their input.
- On **Record**, write each armed track's input to a `.wav` while playback continues, real-time-safe.
- On **Stop**, finalize the file and create a **clip** on the armed track at the position where recording began — visible (waveform), playable, movable, and persisted, exactly like an imported clip.
- Show an **input level meter** so the user can confirm signal and set levels.

Success criteria: the user arms an audio track, presses Record, plays/sings into their input, presses Stop, and sees a waveform clip on that track that plays back in time.

## 2. Non-Functional Requirements
- **Real-time safety**: the audio callback must not allocate, lock-and-block, or do file I/O. Captured input is pushed to a lock-free FIFO / `AudioFormatWriter::ThreadedWriter`; disk writing happens on a background thread.
- **Latency / sync**: recorded audio lands at the transport position active when Record started (sample-accurate start), so it lines up with existing clips on playback.
- **No dropouts**: recording must not glitch playback even on long takes; buffer sizing handles slow disks.
- **UI/UX**: dark Pro Tools palette continues. Record button red while recording; armed tracks visually distinct (`R` already turns red); a meter on each armed track header.
- **Git**: increments committed atomically to `main`; tag `v0.4.0-phase3` after user validation.

## 3. Functional Requirements

### 3.1 Audio input path (engine change)
- The engine currently reaches the device through an `AudioSourcePlayer` (output only). Replace/augment it with a custom `juce::AudioIODeviceCallback` (or wrap the existing `AudioSource` mixing) so the callback receives **both** input and output buffers.
- The device is already opened with input channels (`initialiseWithDefaultDevices(2, 2)`); surface the input channel count and a level read for metering.
- Existing playback (clip mixing + metronome + atomic position) must keep working unchanged through the new callback.

### 3.2 Record-arm & input monitoring
- `R` on a track header arms it (model `Track::armed`, already wired). At least one armed **audio** track is required to record.
- **Input monitor** toggle (per-armed-track or global): when on, the armed track's input is passed to the output so the user hears themselves (watch for feedback; off by default).

### 3.3 Recording
- Pressing **Record** (with ≥1 armed audio track) starts the transport playing **and** opens a `ThreadedWriter`-backed `.wav` per armed track under a project recordings folder (e.g. `<project>_recordings/` or a temp dir until saved).
- The audio callback appends the device input for each armed track to its writer every block.
- Pressing **Stop** stops the transport, flushes/closes the writers, and for each armed track creates a `Clip` (file = the recorded wav, `startBeat` = the beat where Record began, `lengthBeats` from the recorded duration). The clip appears immediately (waveform) and is added to the engine for playback.
- Recording while the playhead is mid-arrangement records from that position (punch-in style); a take that is silent/zero-length is discarded.

### 3.4 Metering
- Show a small **input level meter** on armed track headers (peak/RMS), updated ~30 Hz from an atomic level the callback writes. Purely visual; never reads audio buffers on the UI thread.

### 3.5 Input source (scope)
- Phase 3 records the **default device input** into the armed track(s). Per-track input/channel routing and selecting the input device in-app are **out of scope** (future phase). If multiple tracks are armed, each gets its own clip from the same input.

### 3.6 Count-in (optional / stretch)
- An optional 1-bar metronome **count-in** before recording starts. May be deferred.

## 4. Validation Test Plan
*(Requires a working audio input — mic or loopback. Auditory/▶ steps are user-validated.)*
1. **Input present** — arm a track; the input meter responds to sound at the mic.
2. **Record a take** — arm an audio track, press Record, make sound for a few seconds, press Stop. A waveform clip appears on that track at the record start.
3. **Playback** — press Play from the start: the recorded clip plays back in time with any existing clips.
4. **Monitoring** — enable input monitor: the live input is audible while armed (and stops when disarmed).
5. **Placement** — move the playhead to bar 5, record: the new clip starts at bar 5.
6. **Persistence** — save, reopen: the recorded clip is restored (file path persists).
7. **No glitches** — record a 30 s take with a clip already playing: no dropouts in playback or the recording.

Report Pass/Fail per step using `docs/TEST_RESULT_TEMPLATE.md`.

## 5. Implementation Constraints
- JUCE only: `AudioIODeviceCallback`, `AudioFormatWriter::ThreadedWriter`, `WavAudioFormat`, `AbstractFifo`/`AudioBuffer` for the lock-free hand-off, `std::atomic` for levels/position.
- No allocations/locks/file I/O on the audio thread; all disk writing on a background thread.
- Recorded files are real `.wav` on disk; the resulting clips reuse the Phase 2 `Clip` model and persistence (absolute paths for now).
- Keep the engine's existing playback path intact; recording is additive.

## 6. Implementation Increments (validated in order)
1. **Input-capable engine** — swap to an `AudioIODeviceCallback` that still plays clips + metronome and now also receives input; expose an atomic input level. *(Validate: playback still works; an input meter moves.)*
2. **Input meter on armed tracks** — show the level on armed track headers.
3. **Record to disk** — on Record, stream armed-track input to a `ThreadedWriter` wav; on Stop, finalize. *(Validate: a wav file appears with the captured audio.)*
4. **Create clip on stop** — turn the recorded wav into a clip on the armed track at the record-start beat; it shows a waveform and plays back.
5. **Input monitoring** toggle.
6. **Placement + persistence** checks (record mid-arrangement; save/reopen).
7. *(Optional)* **count-in**.
8. **Polish + full validation**, then tag `v0.4.0-phase3`.

## 7. Git Workflow
- Implement increment-by-increment, committed atomically to `main` (matching Phases 1–2 practice).
- After user validation of the full phase: tag `v0.4.0-phase3`.

---

**Next Action**: On approval of this spec (and any scope edits), implementation begins with Increment 1 (input-capable engine). **Note:** recording validation needs a live audio input device on the test machine.
