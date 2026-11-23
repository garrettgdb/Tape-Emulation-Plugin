# TAPE / 01 — Magnetic Tape Processor

Initial JUCE source project for a stereo tape-saturation VST3 and standalone application.

## Current prototype features
- Stereo audio processing
- Input and output gain
- Drive, saturation, bias and program-dependent compression
- Three tape-speed voicings (7.5 / 15 / 30 IPS)
- Three tape-formula voicings (Vintage / Studio / Master)
- Simple low/high tone shaping
- Wow/flutter modulation using a short modulated delay
- Wet/dry mix and input/output peak meters
- Host-automatable parameters and saved plugin state
- Minimal dark graphite / cream / amber interface

## Important prototype limitations
This is an early DSP prototype, not a finished or validated commercial-quality tape model.
The saturation curve is an approximation; it does not yet model magnetic hysteresis, tape
head behavior, record/playback equalization standards, noise, crosstalk, or calibrated
tape formulations. The oversampling selector is currently a UI/parameter placeholder and
does not yet activate an oversampling stage. Parameter smoothing, latency reporting,
true bypass behavior, preset management, and extensive host/DAW testing remain to be done.

Do not use this prototype for critical mastering until it has been measured and tested.

## Build prerequisites
- CMake 3.22+
- A C++17 compiler
- Git
- Platform SDK / build tools (Windows: Visual Studio Build Tools or equivalent; macOS: Xcode Command Line Tools)
- Internet access on the first configure/build, because CMake fetches JUCE 8.0.6 from GitHub

## Build (Windows PowerShell)
```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --target TAPE_01_VST3
```

The resulting VST3 bundle is normally under:
`build/TAPE_01_artefacts/Release/VST3/`

## Build (macOS)
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target TAPE_01_VST3
```

## Next engineering milestones
1. Add parameter smoothing and robust bypass/gain staging.
2. Implement real 2x/4x oversampling.
3. Replace the current saturation approximation with a more complete tape model.
4. Validate delay modulation, stereo behavior, CPU use, denormal handling and plugin state.
5. Test in Ableton Live and other supported hosts.
6. Package signed/notarized macOS builds and a Windows installer.


## Optional cloud build (no Visual Studio installation on your PC)
A GitHub Actions workflow is included at `.github/workflows/build.yml`. To use it:
1. Create a GitHub repository and upload/push this project to its `main` branch.
2. Open the repository's **Actions** tab and enable Actions if GitHub asks.
3. The workflow builds Windows and macOS VST3 artifacts on GitHub-hosted runners.
4. Open the completed workflow run and download the matching artifact from **Artifacts**.

This uses Microsoft's preconfigured Windows build runner and Apple's macOS runner in the cloud; it does not install Visual Studio on your own computer. The workflow has been authored but has not yet been executed, so its first run is the build validation step. macOS distribution may additionally require signing/notarization before Gatekeeper will treat the plugin as a normal trusted download.
