# Validation report

Validated on Windows 11 x64 against the working tree based on commit `5996743`.

## Local evidence

Executed with CMake 4.4.0, Visual Studio Build Tools 2022/MSVC 19.44, Windows SDK
10.0.26100.0, JUCE 8.0.13, and `QB_ENABLE_ASIO=ON`:

```text
cmake --preset ci-windows
cmake --build --preset ci-windows --parallel 4
ctest --preset test-windows --output-on-failure
clang-format -i <all src/tests C++ files>
git diff --check
```

Results:

- Windows Release standalone application: PASS
- Windows x64 VST3 bundle: PASS
- Core deterministic test suite: PASS
- VST3 scan, instantiation, stereo bus layout, processing, editor, and state round-trip: PASS
- CTest: 2/2 targets passed
- Editor resize to the documented 900×720 minimum: PASS
- Hosted editor visual inspection at 125% Windows display scaling: PASS; four channel strips are
  absent and dense effect labels use separate non-overlapping columns
- VST3 metadata version: 0.1.1
- VST3 binary architecture: PE x86_64
- Versioned Windows delivery ZIP: PASS; complete bundle and installation guide are present, while
  source references, Git data, tests, and build intermediates are absent

The automated checks cover mixer/channel/booth/cue routing, mute, crossfader assignment, isolator
kill, beat clock, division bounds, manual/TAP/automatic tempo, generated click-track tempos, all
fifteen effects at 44.1/48/88.2/96 kHz, finite/bounded effect output, bypass safety, invalid-number
protection, state recovery, MIDI mapping behavior, and the standard one-input/one-output VST3 host
contract.

## Required external validation

The target commercial host is not installed in this development environment, so an in-host scan
cannot be claimed as completed here. The replacement VST3 deliberately uses the conventional stereo
effect layout and passes the JUCE VST3 scanner/host. On the client machine, install the complete
bundle in the Windows system VST3 location and rescan both previously verified plugins and plugins
with errors as described in `INSTALL-VST3-WINDOWS.txt`.

The physical audio-interface and MIDI-controller matrix in `docs/HARDWARE_VALIDATION.md`, hosted
macOS result, long-duration stability run, licensing review, and signing/notarization remain external
release checks. Builds remain unsigned.
