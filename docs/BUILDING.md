# Building

Requirements: CMake 3.24+, a C++20 compiler, Git, and Ninja. Initialize JUCE first:

```sh
git submodule update --init --recursive
```

Windows:

```sh
cmake --preset ci-windows
cmake --build --preset ci-windows
ctest --preset test-windows --output-on-failure
```

Set `-DQB_ENABLE_ASIO=OFF` to make a WASAPI-only development build. Windows CI intentionally uses
ASIO ON but the headless tests require no driver.

macOS:

```sh
cmake --preset ci-macos
cmake --build --preset ci-macos
ctest --preset test-macos --output-on-failure
```

## Build outputs

The CI presets build the standalone application, VST3 effect, and test executable. Typical Release
outputs are:

- Windows standalone: `out/ci-windows/QuadBeatFX_artefacts/Release/QuadBeat FX.exe`
- Windows VST3: `out/ci-windows/QuadBeatFXPlugin_artefacts/Release/VST3/QuadBeat FX.vst3`
- macOS standalone: `out/ci-macos/QuadBeatFX_artefacts/Release/QuadBeat FX.app`
- macOS VST3: `out/ci-macos/QuadBeatFXPlugin_artefacts/Release/VST3/QuadBeat FX.vst3`

Keep the entire `.vst3` bundle intact. Windows release packages include
`Install-QuadBeatFX-VST3.cmd`, which copies the complete bundle to the standard per-user VST3
folder `%LOCALAPPDATA%\Programs\Common\VST3` without elevation. Pass `-AllUsers` to the PowerShell
installer from an elevated terminal to use `C:\Program Files\Common Files\VST3`. Ask the host to rescan previously failed and previously
verified plugins. Custom VST2 search paths are not used for VST3 bundles. On macOS, copy it to
`/Library/Audio/Plug-Ins/VST3` or the corresponding per-user folder. A build is not a host-loadable
plugin if only the standalone executable is copied.

Windows Release builds use the static MSVC runtime so the VST3 does not require a separately
installed matching Visual C++ redistributable.

The macOS presets set deployment target 12 and `CMAKE_OSX_ARCHITECTURES=arm64;x86_64`. All build
trees live under `out/`. The minimum application window is 900×720.
