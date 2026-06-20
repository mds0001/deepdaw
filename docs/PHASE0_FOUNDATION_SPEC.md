# Phase 0: Foundation Specification

**Version**: 1.0  
**Status**: Ready for Implementation  
**Owner**: AI Agent (implementation + Git)  
**Reviewer**: User (validation only)

## 1. Objectives
Establish the minimal viable DAW shell that:
- Runs natively on Windows with real-time audio priority.
- Presents a professional dark UI closely resembling Pro Tools / Cubase.
- Provides working transport controls (Play, Stop, Record, Metronome).
- Supports basic project new/open/save (file I/O stubs + UI feedback).
- Initializes JUCE audio engine with sensible defaults.
- Is fully buildable and testable by a non-developer using simple instructions.

Success criteria: User can launch the app, see the intended look, interact with transport, toggle metronome, and perform project actions without crashes or UI lag.

## 2. Non-Functional Requirements
- **Performance**: Application window visible < 1.5 s on modern Windows 10/11 machine. Audio thread runs at real-time priority. No audio glitches on transport actions.
- **UI/UX**: Dark theme (#1E1E1E background, #2D2D2D panels, #FF7F00 Pro Tools orange accent). Clean typography. Resizable main window. No placeholder text visible in final UI.
- **Compatibility**: Windows 10 64-bit and above. Built with Visual Studio 2022 toolchain.
- **VST readiness**: JUCE VST3 hosting infrastructure initialized (no plugins loaded yet).
- **Git/DevOps**: All work on short-lived branch `feat/phase0-foundation`. Atomic commits. Tagged release `v0.1.0-phase0` only after user validation.

## 3. Functional Requirements

### 3.1 Application Bootstrap
- Main window titled "DeepDAW – Untitled Project".
- Dark Pro Tools-inspired color scheme applied globally via custom LookAndFeel.
- Menu bar: File (New, Open, Save, Save As, Exit), Edit, View, Transport, Help.
- Central layout (top-to-bottom):
  1. Transport bar (full width).
  2. Main content area (split: Track list on left, Timeline on right) – empty but visually defined.
  3. Status bar at bottom.

### 3.2 Transport Controls
- Buttons (left to right): Rewind | Play | Stop | Record | Forward | Metronome toggle.
- Visual states: Play button turns green when playing; Record button turns red when armed/recording.
- Metronome: Toggle produces audible click at current BPM (default 120). Uses JUCE `ToneGeneratorAudioSource` or equivalent low-CPU click.
- BPM display (editable numeric field, default 120).
- Time display: Bars:Beats | SMPTE timecode (stub values for now).

### 3.3 Project Management (Stubs)
- **New Project**: Clears current state, resets title to "Untitled Project", resets transport to stopped.
- **Open Project**: Shows native file dialog (filter `*.deepdaw`), loads stub (currently just sets project name from filename).
- **Save / Save As**: Shows save dialog, writes minimal JSON project file (version, BPM, empty track list). On success shows toast or status message.
- All file operations must be non-blocking to the audio thread.

### 3.4 Audio Engine
- Default audio device: WASAPI (or best available low-latency driver).
- Sample rate: 48000 Hz.
- Buffer size: 256 samples (target < 6 ms latency).
- Audio callback active from launch.
- CPU meter in status bar (updated every 500 ms, shows current audio thread load).

### 3.5 Error Handling & Logging
- Graceful handling if no audio device available (show clear message + fallback to "No Audio").
- All errors logged to console + visible in status bar.

## 4. UI Visual Specification (Pro Tools / Cubase Inspired)
- Background: #1E1E1E
- Panel background: #252526
- Accent: #FF7F00 (transport active states)
- Text: #CCCCCC / #FFFFFF
- Transport bar height: 48 px, dark with subtle gradient.
- Buttons: Flat with icon + hover highlight in accent color.
- No bright or colorful elements except for active states (Play = green #00C853, Record = red #FF1744).

## 5. Validation Test Plan (User Instructions)

**Prerequisites**
- Windows 10/11 64-bit.
- Visual Studio 2022 with "Desktop development with C++" workload installed.
- CMake 3.25+ (or use the provided `build.bat` that downloads it if missing).

**Step-by-step Validation**

1. **Build & Launch**
   - Run `build.bat` from project root (or follow manual CMake steps in README).
   - Confirm executable `DeepDAW.exe` appears in `build/Debug` or `build/Release`.
   - Launch `DeepDAW.exe`.
   - **Pass criterion**: Window appears within 2 seconds showing dark Pro Tools-style interface. No console errors.

2. **Transport Interaction**
   - Click Play → Play button turns green, time display begins advancing.
   - Click Stop → Transport stops, button returns to normal.
   - Click Record → Record button flashes red.
   - Toggle Metronome on → Hear regular click at 120 BPM. Toggle off → sound stops.
   - Change BPM value → Metronome rate changes immediately.
   - **Pass criterion**: No audio glitches, clicks are clean, UI remains responsive.

3. **Project Operations**
   - File → New Project → Title resets to "Untitled Project".
   - File → Save As → Choose location, save `TestProject.deepdaw`.
   - Confirm file is created and contains valid JSON with BPM and version.
   - File → Open Project → Select the saved file → Project title updates.
   - **Pass criterion**: All operations complete without crash; status messages appear.

4. **Window & Performance**
   - Resize window → UI elements reflow cleanly, no clipping.
   - Leave app running 2 minutes → CPU meter stays below 5% when idle, no memory growth visible in Task Manager.
   - **Pass criterion**: Smooth resizing, stable resource usage.

5. **Shutdown**
   - Close window or File → Exit.
   - **Pass criterion**: Clean exit, no lingering audio or crashes.

**Reporting**
Reply with:
- Pass / Fail for each numbered step above.
- Any visual or audio observations.
- Screenshots of the main window if possible.

## 6. Implementation Constraints
- Use JUCE 8 (latest stable) via CMake FetchContent or submodule.
- All audio code must be real-time safe (no allocations in audio callback).
- UI code must use `MessageManager::callAsync` for any cross-thread updates.
- No third-party UI frameworks beyond JUCE.

## 7. Git Workflow (Agent Responsibility)
- Create branch `feat/phase0-foundation` from `main`.
- Commit after each logical sub-feature.
- After user validation and fixes: merge to `main`, tag `v0.1.0-phase0`, delete feature branch.

---

**Next Action**: Upon approval of this spec, implementation begins immediately.