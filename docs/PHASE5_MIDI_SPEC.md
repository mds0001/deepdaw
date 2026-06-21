# Phase 5: MIDI & Instruments Specification

**Version**: 1.1
**Status**: Approved – implementing
**Owner**: AI Agent (implementation + Git)
**Reviewer**: User (validation only)

**Scope decisions (locked):** piano-roll editor is a **dockable bottom panel**
(mixer-panel pattern); the instrument is a **built-in synth only** — VST3/AU
plugin hosting is deferred to its own later phase.

## 1. Objectives
Make MIDI tracks first-class: editable note data, a built-in instrument so they
make sound, and MIDI recording. Today `TrackType::midi` tracks exist (the
"+ MIDI Track" button creates them) but render as empty lanes and produce no
audio. This phase turns them into a working MIDI workflow that plugs into the
existing engine and mixer.

- **MIDI clips** on the timeline (a clip holds a list of notes), created/moved/
  deleted like audio clips.
- A **piano-roll editor** to draw, move, resize, and delete notes on a grid.
- A **built-in synth instrument** per MIDI track (oscillator + ADSR) so notes
  are audible, rendered on the audio thread and summed through the existing
  per-track gain/pan/mute/solo, master, and meters.
- **MIDI recording**: play an on-screen or hardware MIDI keyboard into an armed
  track (with live monitoring and the Phase 3 count-in) and capture a clip.
- **Persist** MIDI clips/notes and the per-track instrument in `.deepdaw`.

Success criteria: the user can add a MIDI track, draw a melody in the piano
roll (or record one from a keyboard), hear it play back through the built-in
synth in time, balance it in the mixer alongside audio tracks, and reopen the
project with everything intact.

**Explicitly out of scope** (later phases): hosting third-party instrument/
effect plugins (VST3/AU), multi-timbral instruments, MIDI CC/automation lanes,
quantize/groove tools, and importing/exporting `.mid` files. The instrument
here is a deliberately simple built-in synth — a placeholder for plugin hosting.

## 2. Non-Functional Requirements
- **Real-time safety**: the synth renders in the existing audio callback. Voices
  are pre-allocated (JUCE `Synthesiser`); per-track render buffers and the
  per-block `MidiBuffer` are pre-sized (in `audioDeviceAboutToStart`) and reused
  — **no allocations, locks-that-block, or file IO on the audio thread**. Note
  data reaches the audio thread via the existing `setClips`-style `SpinLock`
  hand-off (decoded/prepared on the message thread).
- **Timing**: notes are scheduled **sample-accurately** within each block
  (beats → samples at the current tempo); notes sustain correctly across block
  boundaries; **all notes off** on Stop/Rewind/seek so nothing hangs.
- **Latency**: live MIDI input (monitoring + recording) routes input events to
  the armed track's synth with block-level latency; incoming events arrive via a
  lock-free path (no locks shared with the audio thread).
- **Performance**: N MIDI tracks each render their own synth; stays light for a
  handful of tracks and typical note counts. Piano-roll repaints are scoped.
- **UI/UX**: dark Pro Tools palette. Piano roll has a vertical keyboard gutter,
  bar/beat grid with snap, and note blocks tinted with the track colour. MIDI
  clip blocks show a mini note preview.
- **Engine reuse**: MIDI is **additive** to the Phase 2–4 callback. MIDI tracks
  flow through the same per-track gain/pan/mute/solo, master gain, and output
  meters as audio tracks (the mixer already gives every track a strip).
- **Git**: increments committed atomically to `main`; tag `v0.6.0-phase5` after
  user validation.

## 3. Functional Requirements

### 3.1 MIDI data model
- **`MidiNote`** — `{ int pitch (0–127), double startBeat (relative to the
  clip), double lengthBeats, float velocity (0–1) }`.
- A clip becomes MIDI-capable: `Clip` gains `std::vector<MidiNote> notes` and is
  identified as MIDI by its track type (audio clips keep `filePath`; MIDI clips
  keep `notes` and have no file). Note positions are **relative to the clip
  start**, so moving the clip moves the notes.

### 3.2 MIDI clips on the timeline
- **Double-click an empty MIDI lane** creates an empty MIDI clip (default ~1 bar)
  at the snapped position. MIDI clips **move/right-click-delete** like audio
  clips and reuse the existing `ClipComponent` drag/seek/scale plumbing.
- The clip block shows a **mini piano-roll preview** of its notes (track-tinted),
  and "(empty)" when it has none.

### 3.3 Piano-roll editor
- **Double-click a MIDI clip** opens the piano-roll editor for that clip in a
  **dockable bottom panel** (the same pattern as the mixer panel — shown/hidden,
  docked along the bottom above the status bar, with a Close affordance and the
  edited clip's name shown). The arrangement stays visible above it.
- Layout: a **vertical keyboard gutter** (note names, octave labels), a
  horizontal **bar/beat grid** scaled to the clip length, and note blocks.
- Editing: **draw** a note (click-drag to set pitch + length), **move** (drag),
  **resize** (drag an edge), **delete** (right-click). **Snap** to a selectable
  grid (1/1, 1/2, 1/4, 1/8…). Edits write straight to the clip's `notes`.
- Drawing/clicking a note **auditions** it through the track's synth.

### 3.4 Built-in instrument (synth) + playback
- Each MIDI track owns a **polyphonic built-in synth**: a `juce::Synthesiser`
  with a simple oscillator voice (sine/saw/square, selectable) and an ADSR
  envelope. Voices pre-allocated.
- The engine, per block and per MIDI track, builds a `MidiBuffer` of the
  note-on/off events that fall in the block (from the preloaded clips at the
  current play position), renders the synth into a per-track buffer, and sums it
  into the output with the track's **gain + equal-power pan**, then the **master
  gain**, feeding the **per-track and master output meters** — identical to how
  audio clips are treated.
- Mute/solo gate MIDI tracks the same as audio tracks.

### 3.5 MIDI input & recording
- **Input sources**: an on-screen keyboard (JUCE `MidiKeyboardComponent`) and
  any enabled **hardware MIDI input** device (via the `AudioDeviceManager`).
- **Monitoring**: while a MIDI track is record-armed, incoming notes play live
  through that track's synth.
- **Recording**: reuse the Phase 3 flow — arm a MIDI track, hit Record, the
  1-bar **count-in** plays, then incoming MIDI is captured (timestamped to the
  play position) into a **new MIDI clip** on the armed track at
  `getRecordStartBeat()`; the take is immediately editable in the piano roll.

### 3.6 Persistence
- MIDI clips serialize their `notes` (pitch/startBeat/lengthBeats/velocity) into
  the clip JSON; the track serializes its **instrument** (waveform + ADSR, or at
  minimum the waveform). Backward compatible — missing `notes`/`instrument`
  default to empty / a default sine instrument; existing audio projects load
  unchanged.

## 4. Validation Test Plan
*(Note playback/timbre are auditory — user-validated.)*
1. **Create MIDI clips** — double-click a MIDI lane: an empty clip appears; it
   moves and deletes like an audio clip.
2. **Piano roll** — draw several notes, move/resize/delete them, change snap;
   the clip-block preview reflects the edits.
3. **Audition** — clicking/drawing a note sounds the synth.
4. **Playback** — press Play: the drawn notes play in time and in tune; they
   stop cleanly on Stop (no hung notes); Rewind/seek doesn't leave notes ringing.
5. **Mix integration** — a MIDI track's fader/pan/mute/solo and the master all
   affect it; its meter responds.
6. **MIDI input** — on-screen and (if available) hardware keyboard play live
   through an armed track's synth.
7. **MIDI recording** — arm, record with count-in, play a phrase: a clip appears
   at the cursor and plays back; it's editable in the piano roll.
8. **Persistence** — save a project with MIDI clips + a chosen instrument,
   reopen: notes, positions, and timbre are restored and play identically.

Report Pass/Fail per step using `docs/TEST_RESULT_TEMPLATE.md`.

## 5. Implementation Constraints
- JUCE only: `Synthesiser`/`SynthesiserVoice`/`SynthesiserSound`, `MidiBuffer`,
  `MidiMessage`, `MidiKeyboardState`/`MidiKeyboardComponent`, `AudioDeviceManager`
  MIDI input, `ADSR`.
- Reuse the existing `LoadedClip`/`setClips` hand-off pattern for note data and
  the Phase 3 recording/count-in flow; mixing/metering is additive to the
  Phase 2–4 callback.
- No allocations/locks-that-block or file IO on the audio thread; pre-size the
  per-track render buffers and per-block `MidiBuffer` in `audioDeviceAboutToStart`;
  reach the audio thread for live input via a lock-free FIFO; `allNotesOff` on
  Stop/Rewind/seek.

## 6. Implementation Increments (validated in order)
1. **MIDI clip model + MIDI lanes** — `MidiNote`, MIDI-capable `Clip`, double-
   click a MIDI lane to create/move/delete an empty MIDI clip; clip block shows
   the (empty) note-preview area. *(Validate, visual: MIDI clips behave like
   audio clips on the timeline.)*
2. **Piano-roll editor** — open on a MIDI clip; draw/move/resize/delete notes
   with grid snap and a keyboard gutter; edits persist into the clip and update
   the block preview. *(Validate, visual.)*
3. **Built-in synth + playback** — per-track synth, sample-accurate engine
   scheduling through gain/pan/mute/solo + master + meters, `allNotesOff` on
   stop/seek. *(Validate, auditory: drawn notes play in time; mix controls
   affect the MIDI track.)*
4. **MIDI input + recording** — on-screen + hardware keyboard, live monitoring
   through the armed track's synth, count-in record into a new editable clip.
   *(Validate, auditory.)*
5. **Persistence + instrument selection + polish** — notes + instrument in
   `.deepdaw`, waveform/ADSR selection, piano-roll polish; full validation, then
   tag `v0.6.0-phase5`.

## 7. Git Workflow
- Implement increment-by-increment, committed atomically to `main` (matching
  Phases 1–4).
- After user validation of the full phase: tag `v0.6.0-phase5`.

---

**Next Action**: On approval of this spec (and any scope edits), implementation
begins with Increment 1 (MIDI clip model + MIDI lanes).
