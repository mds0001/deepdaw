# DeepDAW

Modern, AI-augmented Digital Audio Workstation built for Windows with professional Pro Tools / Cubase aesthetics and real-time performance.

**Current Status**: Phase 0 – Foundation (transport, UI shell, project I/O stubs)

---

## Getting the Latest Build (No Local Compilation Required)

Once the repository is connected to GitHub, every push to `main` automatically builds a fresh Windows executable in the cloud. You simply download and test it.

### How it works
1. AI commits and pushes code changes.
2. GitHub Actions spins up a Windows VM, builds the project, and produces `DeepDAW.exe`.
3. You download the artifact from the Actions run page and run it locally.
4. You validate against the provided test specification.

### First-time setup (one time only)
1. Create a new GitHub repository (recommended name: `deepdaw`).
2. Run these commands in PowerShell / Command Prompt:

```powershell
cd C:\Users\mdsto\projects\deepdaw

# Connect to your GitHub repo (replace YOUR_USERNAME with your GitHub username)
git remote add origin https://github.com/YOUR_USERNAME/deepdaw.git

# Push the code (this triggers the first cloud build)
git push -u origin main
```

3. Go to your GitHub repository → **Actions** tab.
4. Click the latest workflow run.
5. Scroll to the bottom and download the artifact named `DeepDAW-Windows-...`.
6. Extract and run `DeepDAW.exe`.

Future builds will appear automatically after every push.

---

## For Developers: Local Build

If you prefer to build locally:

### Prerequisites (one-time)
- Visual Studio 2022 with "Desktop development with C++" workload
- CMake 3.25+

### Build
```powershell
.\build.bat
```

Executable will be at `build\DeepDAW_artefacts\Release\DeepDAW.exe`.

---

## Development Workflow
- All code changes are managed by the AI agent.
- Strict spec → implement → cloud build → user validation loop.
- See `docs/PHASE0_FOUNDATION_SPEC.md` for the current active specification.

## License
Proprietary – internal project.