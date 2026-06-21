# Phase 4: Mixing Specification

**Version**: 1.0
**Status**: Draft – awaiting approval
**Owner**: AI Agent (implementation + Git)
**Reviewer**: User (validation only)

## 1. Objectives
Add per-track and master gain control so multiple tracks can be balanced into a mix. Building on the Phase 2/3 engine (which already sums clips and respects mute/solo):

- **Per-track volume** (fader) and **pan**.
- A **master volume** on the final mix.
- A **mixer view**: a panel of channel strips (fader, pan, output meter, mute/solo mirror) plus a master strip.
- **Output level meters** — per track and master — so the user can see what they're balancing.
- **Persist** gain/pan/master in `.deepdaw`.

Success criteria: with two tracks playing, the user can lower one track's fader, pan tracks left/right, ride the master, and see meters respond — all heard in real time and restored on reopen.

## 2. Non-Functional Requirements
- **Real-time safety**: gain/pan are applied in the existing audio callback with no allocations/locks-that-block. Fader/pan changes reach the audio thread as atomics or via the existing `setClips` hand-off; meter levels are written to atomics and read by the UI.
- **Smooth audio**: gain changes shouldn't click — ramp per block (or smooth the gain) so fader moves are zipper-free.
- **Performance**: mixing N tracks + per-track metering stays light; meters update ~30 Hz off atomics (never read audio buffers on the UI thread).
- **UI/UX**: dark Pro Tools palette. Faders calibrated in dB (e.g. −∞…+6 dB, unity at 0 dB). Pan centre detented. Meters peak with a brief hold.
- **Git**: increments committed atomically to `main`; tag `v0.5.0-phase4` after user validation.

## 3. Functional Requirements

### 3.1 Mix model (engine)
- `Track` gains a linear **`gain`** (default 1.0) and **`pan`** (−1 left … +1 right, default 0).
- `LoadedClip` carries its track's `gain`/`pan` (set in `reloadEngineClips`, like the existing `audible` flag). The engine applies, per clip: sample × trackGain, then equal-power pan into L/R, summed.
- A **master gain** (`std::atomic<float>`) scales the final summed output.
- Mute/solo continue to gate audibility (Phase 2); gain/pan apply on top.

### 3.2 Master volume
- A master volume control (master fader in the mixer, mirrored as a small control in the transport bar). dB-calibrated, unity default.

### 3.3 Mixer view
- A toggleable **mixer panel** (e.g. a bottom panel, shown/hidden via a "Mixer" button or the View/Window menu).
- One **channel strip** per track: colour/name header, **vertical fader** (volume), **pan** control, **output level meter**, and **M/S** buttons mirroring the track header.
- A **master strip** on the right: master fader + master output meter.
- Strips stay in sync with the track list (add/remove/reorder/rename/colour/M-S).

### 3.4 Level metering
- Per-track **output** meter (post-fader/pan peak) and a **master** meter, both fed by atomics the audio callback writes; UI reads at ~30 Hz with peak-hold/decay. (Phase 3's *input* meters stay for armed tracks.)

### 3.5 Persistence
- `.deepdaw` per-track JSON gains `gain` and `pan`; the project gains a `master` gain. Backward compatible (missing fields default to unity/centre).

## 4. Validation Test Plan
*(Loudness/pan are auditory — user-validated.)*
1. **Per-track volume** — two tracks playing; lower one fader: that track gets quieter, the meter drops; raise it back.
2. **Pan** — pan one track hard left, another hard right: they separate across the stereo field.
3. **Master** — pull the master down: the whole mix quietens; meters follow.
4. **Mute/solo still work** alongside gain/pan.
5. **Meters** — per-track and master meters track the audio; no UI-thread audio reads (no glitches).
6. **No zipper noise** when moving a fader during playback.
7. **Persistence** — set custom fader/pan/master, save, reopen: values restored and audibly identical.

Report Pass/Fail per step using `docs/TEST_RESULT_TEMPLATE.md`.

## 5. Implementation Constraints
- JUCE only: `Slider` (faders/pan), `std::atomic` for gains/levels, equal-power pan law, per-block gain ramping.
- No allocations/locks-that-block or audio-buffer reads on the audio thread; meter values via atomics.
- Reuse the existing `LoadedClip`/`setClips` hand-off and the Phase 3 callback; mixing is additive to it.

## 6. Implementation Increments (validated in order)
1. **Engine + master volume** — add `gain`/`pan` to the model and apply per-clip in the callback (defaults unity, so no audible change yet); add a **master volume** control and master gain. *(Validate: master volume changes overall loudness.)*
2. **Mixer panel** — toggleable channel strips (fader + pan + M/S) wired to the model → engine, plus the master strip. *(Validate: a track's fader/pan change the sound.)*
3. **Level meters** — per-track output + master meters in the mixer (and master in the transport bar).
4. **Persistence** — `gain`/`pan`/`master` in `.deepdaw`.
5. **Polish + full validation**, then tag `v0.5.0-phase4`.

## 7. Git Workflow
- Implement increment-by-increment, committed atomically to `main` (matching Phases 1–3).
- After user validation of the full phase: tag `v0.5.0-phase4`.

---

**Next Action**: On approval of this spec (and any scope edits), implementation begins with Increment 1 (mix engine + master volume).
