# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

DeepDAW is a Windows-native Digital Audio Workstation built on **JUCE 8** (C++20), targeting a Pro Tools / Cubase dark aesthetic and real-time audio performance. It is built incrementally in numbered phases (specs in `docs/`). Done so far: Phase 0 (transport + UI shell), Phase 1 (tracks + timeline), Phase 2 (audio clips & playback, tagged **`v0.3.0-phase2`**). It is now a working multitrack audio editor: import audio, see waveforms, arrange clips on a zoomable timeline, play in sync, balance with mute/solo, and save/reopen projects.

## Build & run

```powershell
.\build.bat            # configure + build Release (needs cmake + cl.exe on PATH)
.\build\DeepDAW_artefacts\Release\DeepDAW.exe   # run
```

The executable lands in `build/DeepDAW_artefacts/Release/` (JUCE's `*_artefacts` convention), **not** `build/Release/`.

**`cmake` may not be on PATH.** A standalone CMake was used early on but is no longer installed; use the **VS-bundled** one:
```powershell
$cmake = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
& $cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64
& $cmake --build build --config Release --parallel 4
```
(`build.bat` calls `cmake` directly, so it only works from a shell where CMake is on PATH, e.g. a VS Developer Prompt.)

Useful configure flags:
- `-DDEEPDAW_DEPLOY_DIR=C:/path` — after every build, copy the exe there (gated POST_BUILD). Used to keep `C:\Users\mdsto\DeepDAW\DeepDAW.exe` always pointing at the latest build. No-op if unset (so CI is unaffected). The flag persists in the build cache once configured.

Notes:
- **JUCE is fetched at configure time** via CMake `FetchContent` (pinned `8.0.4`); the first configure clones JUCE and is slow, then cached under `build/`.
- **No test suite / linter.** Validation is manual: run the `.exe` and check behaviour. Audio behaviour (does it actually make sound / mute / solo) can only be confirmed by ear — the user validates those.
- Build is warning-clean. Keep it that way (use `juce::FontOptions` not the deprecated `juce::Font(size, style)`; cast away `double→float` narrowing).

## Cloud build & distribution

GitHub Actions is the primary delivery mechanism:
- `.github/workflows/ci-build.yml` — builds on every push to `main` and on PRs, and **uploads `DeepDAW.exe`** as artifact `DeepDAW-Windows-<sha>`.
- `.github/workflows/build-pr.yml` — builds PRs and comments success.

A broken `main` blocks downloads, so **always keep `main` compiling**.

## Architecture

Startup: `Main.cpp` → `START_JUCE_APPLICATION(DeepDAWApplication)` → `DeepDAWApplication` (owns the window) → `MainWindow` (`DocumentWindow`) → `MainComponent` (root) → children.

**`MainComponent`** is the hub. It owns the `AudioDeviceManager`, an `AudioSourcePlayer` (feeds the transport's audio to the device — this is what makes sound come out), an `AudioFormatManager`, the **audio cache** (file path → shared immutable `AudioBuffer`, so mute/solo/move reloads don't re-read files), the **File menu** (New/Open/Save/Save As/Exit) and **status bar** (shows `build <sha>`), and a 60 fps `Timer`. Layout in `resized()` (top→bottom): menu bar (24) · transport (48) · controls strip (36: add-track + zoom `+`/`-` buttons) · bottom status bar (22) · left pane (`TRACKS` strip + track-list viewport, width 280) · right pane (pinned ruler strip + timeline-lanes viewport). The nested **`SyncViewport`** reports its whole visible area so the timeline drives the track list's vertical scroll and the ruler's horizontal scroll (re-entrancy guarded by `syncingScroll`).

**`TransportComponent` is the audio engine** (a `Component` *and* an `AudioSource`). It owns the **authoritative play position** (`std::atomic<int64_t> positionSamples`, advanced on the audio thread while playing). `getNextAudioBlock` mixes the preloaded clips (`LoadedClip`: a shared in-memory buffer read **by time** with interpolation, so file/device sample-rate mismatch is handled) plus the metronome. Clip hand-off from the message thread is via `setClips` under a `SpinLock` (the audio thread uses a try-lock). Exposes `getBpm/setBpm`, `getPositionBeats/setPositionBeats`, `getIsPlaying`, and `onPlayingChanged`/`onReturnToZero` callbacks. **Real-time rule: no allocations, locks-that-block, or file IO in `getNextAudioBlock`** — decode files on the message thread (`MainComponent::reloadEngineClips`) and hand buffers over.

**Playhead is engine-driven.** Don't reintroduce a wall-clock playhead. The engine owns the position; `MainComponent::timerCallback` reads `transport->getPositionBeats()` and pushes it to the timeline/ruler. Seek (timeline/ruler click) and rewind go through `MainComponent::setPlayheadBeats`, which sets *both* the engine position and the visual playhead.

Tracks & clips:
- **`Track`** (`Track.h`) — data struct: id, name, `TrackType`, colour, mute/solo/arm, and a `std::vector<Clip>`.
- **`Clip`** (`Clip.h`) — name, absolute file path, `startBeat`, `lengthBeats` (position/length in beats so they survive tempo/zoom changes).
- **`TrackListComponent`** owns `vector<unique_ptr<Track>>` + parallel header components. Add/remove/reorder tracks (drag a header by its gutter), and `addClip`/`setClipStart`/`removeClip`. Exposes `onTracksChanged` and `onImportAudioRequested`.
- **`TrackHeaderComponent`** — one row: editable name, colour swatch, M/S/R toggles, drag-to-reorder, right-click Delete Track / Import Audio.

Timeline (split into two components):
- **`TimelineComponent`** — the scrolling **lanes** (no ruler). Runtime horizontal **zoom** (`getPixelsPerBar/Beat`, `setZoom`; base 80 px/bar, `numBars = 64`, `rowHeight = 64`). Owns the **`ClipComponent`** children (`rebuildClips` on model change, `layoutClips` on zoom/scroll). It's a `FileDragAndDropTarget` (drop audio onto a lane to import). `mouseDown` seeks; Ctrl+wheel zooms (`onZoomGesture`); plain wheel forwards to the viewport. The **playhead is drawn in `paintOverChildren`** so it sits on top of clips.
- **`TimelineRulerComponent`** — the bar-number ruler, a **pinned strip** above the lanes (never scrolls vertically). It reads scale + playhead from the `TimelineComponent` and mirrors horizontal scroll via `setScrollX`; click-to-seek and Ctrl+wheel zoom work here too.
- **`ClipComponent`** — a track-tinted block with the clip name and its **waveform** (`AudioThumbnail`, loads async via a `ChangeListener`). Left-drag moves it (forwarded to the timeline, which knows the scale + model); right-click deletes. Missing files render greyed-out "(offline)". Sets its title for accessibility.

**`Project`** (`Project.h/.cpp`) — `ProjectData {bpm, zoom, tracks(with clips)}` serialized to human-readable `.deepdaw` JSON (colours as `#RRGGBB`). `ProjectIO::toJsonString/parseJsonString/saveToFile/loadFromFile`. Backward compatible (missing `zoom`/`clips` default).

**`DeepDAWLookAndFeel`** — singleton (`getInstance()`) `LookAndFeel_V4`: background `#1E1E1E`, panel `#252526`, accent orange `#FF7F00`. Apply via `setLookAndFeel(&DeepDAWLookAndFeel::getInstance())` and pull colours from its accessors.

Components communicate **upward via `std::function` callbacks** (e.g. `onTracksChanged`, `onSeek`, `onZoomGesture`, `onFileDropped`, `onDragStart/onDrag/onDragEnd`), not observers or a global bus. Follow that pattern.

## Phase-driven workflow

Each phase has a spec in `docs/` (`PHASE0_*`, `PHASE1_*`, `PHASE2_AUDIO_CLIPS_SPEC.md`) with numbered **increments**. The loop: draft/approve a spec → implement increment-by-increment, each one committed atomically, validated, and pushed → tag `vX.Y.0-phaseN` when the phase is done. Despite the early specs mentioning `feat/phaseN-*` branches, work has actually been committed straight to `main` (CI + the user's downloads track `main`). Auditory increments (playback, metronome, mute/solo) are validated by the user's ears before committing; visual ones by screenshot.

## Gotchas

- **New `.cpp` files must be added to `target_sources(DeepDAW ...)` in `CMakeLists.txt`** — only `.cpp` is listed (not headers). An unlisted source silently never builds.
- **Build id:** `cmake/GenBuildInfo.cmake` regenerates `build/generated/BuildInfo.h` (git short SHA, `+` if the tree is dirty) on every build; `MainComponent` shows it in the status bar. The git "LF will be replaced by CRLF" lines during a build come from that step running `git` — harmless noise, not build errors.
- `#include <JuceHeader.h>` (generated by `juce_generate_juce_header`) is the umbrella include used throughout.
- File edits: use the Write/Edit tools only (per global rules) — never shell heredocs/`echo`, which corrupt files across the Windows VM boundary.
