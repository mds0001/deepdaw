# Phase 0 Validation – Quick Start Guide

**Goal**: Confirm the foundation build works correctly on your Windows machine.

Once the current GitHub Actions build completes with a green check, download the artifact and follow this checklist.

---

## Download Instructions

1. Go to: https://github.com/mds0001/deepdaw/actions
2. Click the latest successful workflow run (green check).
3. Scroll to the bottom and download the artifact:  
   `DeepDAW-Windows-<commit-sha>`
4. Extract the zip file.
5. Run `DeepDAW.exe`.

---

## Validation Checklist (5 Steps)

### 1. Build & Launch
- [ ] Launch `DeepDAW.exe`
- [ ] Window appears within 2 seconds
- [ ] Dark Pro Tools-style interface is visible
- [ ] No console errors or crash dialogs

**Result**: [ ] Pass   [ ] Fail  
**Notes**:

---

### 2. Transport Interaction
- [ ] Click **Play** → button turns green, time display advances
- [ ] Click **Stop** → transport stops cleanly
- [ ] Click **Record** → button flashes red
- [ ] Toggle **Metronome** on → hear regular click at 120 BPM
- [ ] Toggle **Metronome** off → sound stops
- [ ] Change **BPM** value → metronome rate updates immediately

**Result**: [ ] Pass   [ ] Fail  
**Notes**:

---

### 3. Project Operations
- [ ] File → **New Project** → title resets to “Untitled Project”
- [ ] File → **Save As** → save `TestProject.deepdaw`
- [ ] Confirm the `.deepdaw` file is created and contains valid JSON
- [ ] File → **Open Project** → load the saved file
- [ ] Project title updates to the saved name

**Result**: [ ] Pass   [ ] Fail  
**Notes**:

---

### 4. Window & Performance
- [ ] Resize the window → UI elements reflow cleanly, no clipping
- [ ] Leave the app running for 2 minutes
- [ ] CPU usage stays low when idle (check Task Manager)
- [ ] No memory growth or stuttering

**Result**: [ ] Pass   [ ] Fail  
**Notes**:

---

### 5. Shutdown
- [ ] Close the window or use File → Exit
- [ ] Clean exit with no lingering processes or crashes

**Result**: [ ] Pass   [ ] Fail  
**Notes**:

---

## Overall Verdict

[ ] **All tests passed** – ready to tag `v0.1.0-phase0` and proceed to Phase 1  
[ ] **Issues found** – see notes above

**Additional observations or suggestions**:

---

## Next Steps After Validation

- If everything passes → reply with “All tests passed” and I will:
  - Tag the release
  - Merge any final fixes
  - Begin Phase 1 implementation

- If issues are found → paste the failed step + any error messages and I will diagnose and push a fix.

---

**Test report template also available**: `docs/TEST_RESULT_TEMPLATE.md` (use whichever format you prefer).