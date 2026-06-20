# Phase 1 Implementation Checklist

**Goal**: Deliver a functional track list + timeline that feels like the foundation of a real DAW, with full project persistence.

Each item below will be implemented, built via GitHub Actions, and validated by you before moving to the next.

---

## 1. Project Data Model
- [ ] Define `Track` struct (id, name, type: audio/midi, color, muted, solo, armed)
- [ ] Extend `Project` model to hold a list of tracks + BPM
- [ ] Update JSON serialization/deserialization for the new structure
- [ ] Ensure backward compatibility with Phase 0 `.deepdaw` files

## 2. Track List UI Component
- [ ] Create `TrackHeaderComponent` (number, editable name, color swatch, M/S/R buttons)
- [ ] Create `TrackListComponent` (vertical list of headers)
- [ ] Add “+ Audio Track” and “+ MIDI Track” buttons
- [ ] Implement right-click → Delete Track
- [ ] Wire up Mute / Solo / Record-arm state changes (visual + model sync)
- [ ] Add subtle alternating row colors

## 3. Timeline UI Component
- [ ] Create `TimelineRulerComponent` (bars:beats display, grid lines)
- [ ] Create `TimelineComponent` (empty track lanes, playhead, click-to-set position)
- [ ] Implement horizontal/vertical scrolling (Viewport + mouse wheel)
- [ ] Implement zoom in/out (Ctrl + wheel, +/- buttons)
- [ ] Sync playhead position with transport (real-time update)
- [ ] Make timeline height match track list row height

## 4. Main Layout Integration
- [ ] Split main window into resizable left (Track List) + right (Timeline) panes
- [ ] Add vertical scrollbar that controls both panes together
- [ ] Ensure window resize behaves cleanly
- [ ] Maintain 60 fps smoothness during scrolling/zooming

## 5. Project Persistence (Phase 1)
- [ ] On Save: serialize full track list (names, colors, states)
- [ ] On Load: recreate all tracks with correct state
- [ ] Test round-trip: create tracks → save → close → open → verify exact state

## 6. Audio Engine Updates
- [ ] Give each audio track its own `AudioTransportSource` (stubbed for now)
- [ ] Prepare MIDI track infrastructure (empty `MidiMessageSequence`)
- [ ] Keep master mixer and transport working with multiple tracks

## 7. Visual Polish & Pro Tools Feel
- [ ] Match Pro Tools track header proportions and colors
- [ ] Record-arm button turns red when active
- [ ] Mute = yellow, Solo = green (standard convention)
- [ ] Subtle borders and hover states on track headers
- [ ] Clean playhead (bright orange vertical line)

## 8. Git & Release
- [ ] Work on branch `feat/phase1-tracks-timeline`
- [ ] Atomic commits for each major item above
- [ ] After user validation: merge to `main`, tag `v0.2.0-phase1`

---

## Validation Order (after each major group)
1. Data Model + Persistence
2. Track List UI
3. Timeline UI
4. Integrated Layout + Polish
5. Full multi-track project save/load

Each validation round uses the test report template and the detailed steps in `PHASE1_TRACKS_TIMELINE_SPEC.md`.

---

**Status**: Ready to begin implementation as soon as Phase 0 is validated and tagged.