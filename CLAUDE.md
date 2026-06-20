# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

DeepDAW is a Windows-native Digital Audio Workstation built on **JUCE 8** (C++20), targeting a Pro Tools / Cubase dark aesthetic and real-time audio performance. It is built incrementally in numbered phases (see `docs/`). Phase 0 (transport + UI shell) is the current `main`; Phase 1 (tracks + timeline) is in progress.

## Build & run

```powershell
.\build.bat            # configure + build Release via Visual Studio 2022 / CMake
.\build\DeepDAW_artefacts\Release\DeepDAW.exe   # run
```

`build.bat` requires VS 2022 ("Desktop development with C++") and CMake 3.25+, and must run where `cl.exe` is on PATH (a "Developer Command Prompt for VS 2022", or after MSVC env setup). The manual equivalent:

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

Notes:
- The **JUCE framework is fetched at configure time** via CMake `FetchContent` (pinned to tag `8.0.4`). The first configure clones JUCE and is slow; subsequent builds are cached under `build/`.
- The executable lands in `build/DeepDAW_artefacts/Release/` (JUCE's `*_artefacts` convention), **not** `build/Release/`. The README still says `build/Release/` — the artefacts path is correct.
- There is **no test suite** and no linter configured. "Validation" is manual: a human runs the produced `.exe` against the step-by-step test plan in the active phase spec.

## Cloud build & distribution

The primary delivery mechanism is GitHub Actions, not local builds:
- `.github/workflows/ci-build.yml` — builds on every push to `main` and on PRs, and **uploads `DeepDAW.exe` as a downloadable artifact** (`DeepDAW-Windows-<sha>`). The non-developer user downloads this artifact to test.
- `.github/workflows/build-pr.yml` — builds PRs and comments success on the PR.

Because the user validates the cloud-built artifact, **a broken build blocks the whole loop** — always keep `main` compiling.

## Architecture

Startup chain (all JUCE):
`Main.cpp` → `START_JUCE_APPLICATION(DeepDAWApplication)` → `DeepDAWApplication` (`JUCEApplication`, owns the window) → `MainWindow` (`DocumentWindow`) → `MainComponent` (root content) → child components.

- **`MainComponent`** owns the `juce::AudioDeviceManager` (initialised with 2-in/2-out defaults) and lays out children in `resized()` using JUCE's `removeFromTop`/`removeFromLeft` rectangle-slicing idiom. The transport bar takes the top 48 px; the remaining area is reserved for the track list + timeline.
- **`TransportComponent`** is both a `Component` and an `AudioSource`. It renders the transport bar and generates the metronome click in `getNextAudioBlock`. It takes the `AudioDeviceManager` by reference but **is not currently registered as the device's audio callback** (a prior commit removed an incorrect registration) — so audio output is not yet wired end-to-end. Keep `getNextAudioBlock` real-time-safe regardless (no allocations, no locks).
- **`DeepDAWLookAndFeel`** is a singleton (`getInstance()`) `LookAndFeel_V4` holding the palette: background `#1E1E1E`, panel `#252526`, accent orange `#FF7F00`. Apply it via `setLookAndFeel(&DeepDAWLookAndFeel::getInstance())` and pull colours from its accessors rather than hardcoding hex in components.

Phase 1 components (currently **untracked / work-in-progress**, see "Gotchas"):
- **`Track`** (`Track.h`) — plain data struct (id, name, `TrackType`, colour, mute/solo/arm). Owned via `unique_ptr` so addresses stay stable as the list mutates.
- **`TrackListComponent`** owns `vector<unique_ptr<Track>>` plus a parallel `vector<unique_ptr<TrackHeaderComponent>>`, rebuilt wholesale on add/remove. Exposes `onTracksChanged` so the host can resize/repaint the timeline in lockstep.
- **`TrackHeaderComponent`** — one row: editable name, colour swatch, M/S/R toggles; fires `onChanged` / `onDeleteRequested` callbacks upward.
- **`TimelineComponent`** holds a `TrackListComponent&` and draws the ruler + per-track lanes aligned to the list rows (`rowHeight = 64`, `rulerHeight = 32`, `pixelsPerBar = 80`).

Components communicate **upward via `std::function` callbacks** (`onChanged`, `onTracksChanged`, `onDeleteRequested`), not observers or a global event bus. Follow that pattern for new wiring.

## Phase-driven workflow

Work is specced before it's written. Each phase has a spec + checklist in `docs/` (`PHASE0_FOUNDATION_SPEC.md`, `PHASE1_TRACKS_TIMELINE_SPEC.md`, `PHASE1_IMPLEMENTATION_CHECKLIST.md`). The loop is: spec → implement on a `feat/phaseN-*` branch → cloud build → user validates the artifact → merge to `main` → tag `vX.Y.0-phaseN`. Consult the active phase spec for required behavior, the exact palette/sizing values, and the validation steps the user will run.

## Gotchas

- **New `.cpp` files must be added to `target_sources(DeepDAW ...)` in `CMakeLists.txt`** or they won't compile/link. The Phase 1 sources (`Track.h`, `Track*Component.*`, `TimelineComponent.h`) exist on disk and are untracked, but are **not yet listed in `target_sources` nor instantiated by `MainComponent`** — they are staged work, not live code. Wire them into both CMake and `MainComponent` when activating them.
- `#include <JuceHeader.h>` (generated by `juce_generate_juce_header`) is the umbrella include used throughout; new sources follow the same convention.
- File edits: use the Write/Edit tools only (per global rules) — never shell heredocs/`echo`, which corrupt files across the Windows VM boundary.
