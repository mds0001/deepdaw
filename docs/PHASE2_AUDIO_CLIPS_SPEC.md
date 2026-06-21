# Phase 2: Audio Clips & Playback Specification

**Version**: 1.0
**Status**: Complete – tagged `v0.3.0-phase2`
**Owner**: AI Agent (implementation + Git)
**Reviewer**: User (validation only)

## 1. Objectives
Turn the Phase 1 visual shell into something that actually makes sound. Build on the existing tracks, timeline, playhead and transport to deliver:

- Import audio files onto audio tracks as **clips**.
- Render each clip's **waveform** on its lane.
- **Place, move and delete** clips on the timeline.
- **Play clips back** through their tracks, in sync with the playhead/transport, with real audio reaching the output device.
- Respect **mute/solo** during playback.
- **Persist clips** (and their positions) in the `.deepdaw` project file.

Success criteria: The user can import an audio file, see its waveform on a track, press Play, and hear it in time with the moving playhead; save and reopen the project with clips intact.

## 2. Non-Functional Requirements
- **Real-time safety**: the audio callback never allocates, locks, or does file I/O. Reading/thumbnailing happens off the audio thread.
- **Performance**: waveforms cached (JUCE `AudioThumbnail`); dragging a clip and 60 fps playback stay smooth. Importing a multi-minute file does not block the UI.
- **Latency**: playback starts promptly when the playhead reaches a clip; target buffer ≤ 512 samples.
- **UI/UX**: dark Pro Tools palette continues. Clips are rounded rectangles tinted with the track colour, showing the waveform and the file name.
- **Git**: work on branch `feat/phase2-audio-clips`; tag `v0.3.0-phase2` only after user validation.

## 3. Functional Requirements

### 3.1 Audio engine (end-to-end output)
- Register a `juce::AudioSourcePlayer` with the existing `AudioDeviceManager` so audio actually reaches the device. **This also fixes the Phase 0 gap where the transport `AudioSource` was never connected** — the metronome should become audible as part of this work.
- A master `MixerAudioSource` sums: each audio track's output + the metronome.
- Transport Play/Stop starts/stops playback; the playhead position (beats → samples via BPM and sample rate) is the playback position. Clicking the timeline / Rewind moves the play position.

### 3.2 Audio import
- Accept common formats via `AudioFormatManager::registerBasicFormats` (WAV, AIFF, FLAC, Ogg, MP3 where supported).
- Two ways to import: **drag-and-drop** an audio file from Explorer onto a track lane, and a **"Import Audio…"** action (track right-click menu and/or File menu) that targets the selected/clicked track.
- The drop position sets the clip's start time (snapped to the bar/beat grid, snap toggle is a stretch goal).

### 3.3 Clip data model
- A `Clip` holds: owning track id, source file (absolute path for now), `startBeat`, `lengthBeats` (derived from file duration × BPM/60), name, and an optional cached thumbnail.
- Clips live on **audio** tracks only (MIDI clips are a later phase).

### 3.4 Clip component & waveform
- A `ClipComponent` draws on the timeline lane at `x = startBeat × pixelsPerBeat`, width = `lengthBeats × pixelsPerBeat`, within its track's lane row.
- Waveform via `AudioThumbnail`; clip background tinted with the track colour; file name in the top-left.

### 3.5 Clip editing (minimum)
- **Move**: drag a clip horizontally to change `startBeat` (and vertically to move it to another audio track — stretch goal).
- **Delete**: right-click → Delete Clip, or select + Delete key.
- Trimming/looping/fades are explicitly **out of scope** for Phase 2.

### 3.6 Playback
- Each audio track owns a positionable source (e.g. `AudioFormatReaderSource` + `AudioTransportSource`, or a custom clip scheduler) added to the master mixer.
- When the transport plays, each clip is heard while the play position is within `[startBeat, startBeat + lengthBeats)`.
- Mute silences a track; any soloed track mutes all non-soloed tracks (standard convention, already wired visually via M/S).

### 3.7 Persistence (extends Phase 1)
- The `.deepdaw` JSON gains a `clips` array per track:
  ```json
  {"id":1,"name":"Drums","type":"audio","color":"#FF7F00","muted":false,"solo":false,"armed":false,
   "clips":[{"file":"C:/audio/loop.wav","startBeat":0.0,"lengthBeats":16.0,"name":"loop"}]}
  ```
- On load, clips are recreated and their thumbnails rebuilt. Missing files load as a greyed-out "offline" clip rather than failing.
- Existing Phase 1 files (no `clips`) load unchanged.

## 4. Validation Test Plan
1. **Engine/metronome** — toggle the metronome and press Play: a click is now actually audible (engine wired).
2. **Import** — drag a WAV onto an audio track (and via "Import Audio…"): a waveform clip appears at the drop point.
3. **Playback** — press Play: the clip plays in time with the playhead; Stop halts it; Rewind returns to start.
4. **Move/Delete** — drag a clip to a new bar (position updates); delete it (removed).
5. **Mute/Solo** — mute a track's clip (silent); solo another (only it sounds).
6. **Multi-track** — two clips on two tracks play together in sync.
7. **Persistence** — save with clips, reopen: clips restored at the right positions with waveforms.

Report Pass/Fail per step using `docs/TEST_RESULT_TEMPLATE.md`.

## 5. Implementation Constraints
- JUCE only: `AudioFormatManager`, `AudioThumbnail`/`AudioThumbnailCache`, `AudioFormatReaderSource`, `AudioTransportSource`, `MixerAudioSource`, `AudioSourcePlayer`.
- No allocations/locks/file reads on the audio thread; prepare sources on the message thread, hand them to the mixer safely.
- Project files store absolute paths in Phase 2 (portability/relative paths noted as future work).

## 6. Implementation Increments (validated in order)
1. **Engine wiring** — `AudioSourcePlayer` + master mixer to the device; metronome audible. *(Validate: hear the metronome.)*
2. **Clip model + ClipComponent** — draw an empty/colored clip block on a lane at a beat position.
3. **Import + waveform** — drag-drop / menu import; `AudioThumbnail` waveform in the clip.
4. **Playback** — per-track sources summed to the master; clips sound under the playhead; Play/Stop/Rewind.
5. **Move + delete** clips.
6. **Mute/solo** affect audio.
7. **Persistence** — clips in `.deepdaw`, with offline handling.
8. **Polish + full validation**, then tag `v0.3.0-phase2`.

## 7. Git Workflow
- Branch `feat/phase2-audio-clips`; atomic commits per increment.
- After user validation: merge to `main`, tag `v0.3.0-phase2`.

---

**Next Action**: On approval of this spec (and any scope edits), implementation begins with Increment 1 (engine wiring).
