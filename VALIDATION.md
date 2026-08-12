# Validation report

Validated on Windows 11 x64 against the working tree based on commit `805ff05`.

## Local evidence

Executed with CMake 4.4.0, Visual Studio Build Tools 2022/MSVC 19.44, Windows SDK
10.0.26100.0, JUCE 8.0.13, and `QB_ENABLE_ASIO=ON`:

```text
cmake --preset ci-windows
cmake --build --preset ci-windows --parallel 4
ctest --preset test-windows --output-on-failure
clang-format --dry-run --Werror <changed src/tests C++ files>
git diff --check
cpack --config out/ci-windows/CPackConfig.cmake -C Release
```

Results:

- Windows Release standalone application: PASS
- Windows x64 VST3 bundle: PASS
- Core deterministic test suite: PASS; 218 checks, 0 failures
- Dedicated audible early-tail and sustained-decay reverb regression: PASS
- VST3 scan, instantiation, mono/stereo bus negotiation, processing, editor, and state round-trip:
  PASS
- pluginval 1.0.4 strictness level 5: PASS at 44.1/48/96 kHz and 64/128/256/512/1024-sample
  blocks, including editor automation, state, audio processing, and bus-layout tests
- CTest: 2/2 targets passed
- Editor resize to the documented 340 x 900 minimum and vertical aspect check: PASS
- Hosted editor visual inspection at 125% Windows display scaling: PASS; the controls form one tall
  display-to-illuminated-ON/OFF strip, the mixer/monitor columns are absent, and dense effect labels
  do not overlap
- VST3 metadata version: 0.1.2
- VST3 binary architecture: PE x86_64
- Dynamic dependency audit: PASS; the module imports no MSVC or Visual C++ runtime DLLs
- Installer validation: PASS; the build-tree bundle installs without elevation into the standard
  per-user VST3 location and pluginval successfully reopens that installed copy with the required
  `Contents/x86_64-win/QuadBeat FX.vst3` module
- Versioned Windows delivery ZIP: PASS; complete bundle, one-click installer, installation guide,
  standalone executable, licenses, and notices are present; source references, Git data, tests, and
  build intermediates are absent

The automated checks cover mixer/channel/booth/cue routing, mute, crossfader assignment, isolator
kill, beat clock, division bounds, manual/TAP/automatic tempo, generated click-track tempos, all
fifteen effects at 44.1/48/88.2/96 kHz, finite/bounded effect output, bypass safety, invalid-number
protection, state recovery, MIDI mapping behavior, reverb-tail energy, and the matching mono/stereo
one-input/one-output VST3 host contract.

## Required external validation

The target commercial host was not preinstalled in this development environment. Its current
official, signed trial installer was downloaded and verified, but Windows requires an administrator
approval to install the host and this automation session cannot grant that approval. An in-host scan
therefore cannot be claimed as completed here. The replacement VST3 deliberately uses the
conventional audio-effect layout and passes both the JUCE VST3 scanner/host and pluginval. The package
includes a validated no-administrator installer for the standard Windows per-user VST3 location. On
the client machine, run the installer and perform FL Studio's
verified scan with both previously verified plugins and plugins with errors enabled as described in
`INSTALL-VST3-WINDOWS.txt`.

This report records local results. The published commit must also pass the repository's hosted
GitHub Actions workflow before release.

The physical audio-interface and MIDI-controller matrix in `docs/HARDWARE_VALIDATION.md`, hosted
macOS result, long-duration stability run, licensing review, and signing/notarization remain external
release checks. Builds remain unsigned.
