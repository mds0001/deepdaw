# Phase 1: Tracks, Timeline & Project Structure Specification

**Version**: 1.0  
**Status**: Draft – awaiting Phase 0 validation  
**Owner**: AI Agent  
**Reviewer**: User (validation only)

## 1. Objectives
Build on the Phase 0 foundation to deliver the core visual and structural elements of a professional DAW:

- Add / remove audio and MIDI tracks
- Basic timeline with ruler, scrolling, zooming
- Track headers (name, mute, solo, record-arm)
- Empty project load/save now persists track list
- Visually match Pro Tools / Cubase track list + timeline layout
- Maintain real-time audio thread safety

Success criteria: User can create a multi-track project, see a realistic timeline, interact with track controls, and have the project persist correctly.

## 2. Non-Functional Requirements
- **Performance**: Adding/removing 20 tracks must feel instant. Timeline scrolling/zooming must remain smooth at 60 fps.
- **UI/UX**: Dark Pro Tools palette continues. Track headers 48 px tall, timeline ruler 32 px. Subtle alternating track background colors.
- **Persistence**: Project file now stores track metadata (name, type, color, mute/solo/arm state).
- **Git**: Work on branch `feat/phase1-tracks-timeline`. Tag `v0.2.0-phase1` only after user validation.

## 3. Functional Requirements

### 3.1 Track List (Left Pane)
- Vertical list of tracks.
- Each track header contains (left to right):
  - Track number
  - Editable name field
  - Color swatch (click to change)
  - Mute (M), Solo (S), Record-arm (R) buttons
  - Type icon (audio waveform or MIDI notes)
- “+ Audio Track” and “+ MIDI Track” buttons above the list.
- Right-click on track header → Delete Track.
- Drag-and-drop reordering of tracks (future-proof for later phases).

### 3.2 Timeline (Right Pane)
- Horizontal ruler showing bars:beats or timecode (toggleable).
- Grid lines aligned to current BPM.
- Empty track lanes that match the track list height.
- Horizontal and vertical scrolling (mouse wheel + Shift for horizontal).
- Zoom in/out (mouse wheel + Ctrl, or dedicated +/- buttons).
- Playhead (vertical orange line) that moves during playback.
- Click anywhere on timeline to set playhead position (even while stopped).

### 3.3 Project Persistence (Extended)
- `.deepdaw` JSON now includes:
  ```json
  {
    "version": "1.0",
    "bpm": 120,
    "tracks": [
      {"id": 1, "name": "Drums", "type": "audio", "color": "#FF7F00", "muted": false, "solo": false, "armed": false},
      ...
    ]
  }
  ```
- On load, tracks are recreated with correct state.
- On save, current track list is serialized.

### 3.4 Audio Engine Updates
- Each audio track gets a dedicated `AudioTransportSource` (stubbed – no file playback yet).
- MIDI tracks prepare for future `MidiMessageSequence`.
- Master mixer remains; track gains default to 0 dB.

### 3.5 Visual Polish
- Track headers use the same dark panel color with subtle borders.
- Alternating row shading (#252526 vs #2A2A2B).
- Record-arm button turns red when active.
- Mute = yellow, Solo = green (standard DAW convention).

## 4. Validation Test Plan

**Prerequisites**
- Phase 0 executable validated and working.
- Download the new artifact after Phase 1 build completes.

**Step-by-step Validation**

1. **Create Tracks**
   - Click “+ Audio Track” three times.
   - Click “+ MIDI Track” twice.
   - **Pass**: 5 tracks appear, correctly typed, numbered 1–5.

2. **Track Controls**
   - Rename a track, change its color, toggle M/S/R.
   - **Pass**: Visual state updates immediately; no crashes.

3. **Timeline Interaction**
   - Scroll horizontally and vertically.
   - Zoom in/out with Ctrl+wheel.
   - Click on timeline → playhead jumps.
   - **Pass**: Smooth scrolling/zooming, playhead moves correctly.

4. **Project Save / Load**
   - Create 4 tracks with custom names/colors/states.
   - Save project.
   - Close and reopen app, load the project.
   - **Pass**: All tracks and their settings are restored exactly.

5. **Delete & Reorder**
   - Delete one track via right-click.
   - Drag tracks to reorder.
   - **Pass**: Track list updates correctly; no crashes.

6. **Performance**
   - Add 20 audio tracks rapidly.
   - Scroll and zoom aggressively.
   - **Pass**: UI remains responsive; no stuttering or memory spikes.

**Reporting**
Use the template in `docs/TEST_RESULT_TEMPLATE.md` (Phase 0 version can be reused with minor updates).

## 5. Implementation Constraints
- All track UI must be custom JUCE components (no third-party UI libs).
- Timeline must use `Viewport` + custom `Component` for smooth performance.
- Audio thread must never allocate or lock during playback.
- Project JSON must be human-readable and forward-compatible.

## 6. Git Workflow
- Branch: `feat/phase1-tracks-timeline`
- After user validation: merge to `main`, tag `v0.2.0-phase1`

---

**Next Action**: After Phase 0 is validated and tagged, implementation of Phase 1 begins immediately.