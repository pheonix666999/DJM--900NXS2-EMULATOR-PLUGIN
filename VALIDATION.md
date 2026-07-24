# Validation report

Validated source commit: `548a50a` (with foundations `f698719` and initial implementation
`f1083d7`).

## Local evidence

Executed on Windows 11 x64 with CMake 4.4.0, Visual Studio Build Tools 2022/MSVC 19.44,
Windows SDK 10.0.26100.0, JUCE 8.0.13, and `QB_ENABLE_ASIO=ON`:

```text
cmake --preset ci-windows
cmake --build --preset ci-windows --parallel 4
ctest --preset test-windows --output-on-failure
QuadBeatFXTests.exe
clang-format --dry-run --Werror <all src/tests C++ files>
git diff --check
cpack --config out/ci-windows/CPackConfig.cmake -C Release
```

Results:

- Windows Release application: PASS
- Windows Release `QuadBeatFXTests`: PASS
- CTest: 1/1 target passed
- Test harness: 216 checks, 0 failures
- Four-second GUI process smoke test: PASS
- C++ formatting and whitespace validation: PASS
- CPack ZIP: PASS; exactly `QuadBeat FX.exe`, `README.md`, `LICENSE.md`, and
  `THIRD_PARTY_NOTICES.md`
- Reference image, test executable, JUCE development files, and intermediates absent from package

The automated checks cover mixer/channel/booth/cue routing, mute, crossfader assignment, isolator
kill, beat clock, division bounds, manual/TAP/automatic tempo, five generated click-track tempos,
all fifteen effects at 44.1/48/88.2/96 kHz, finite/bounded/nontrivial effect output, bypass safety,
NaN/infinity protection, state round-trip/corrupt recovery, MIDI scaling/serialization, and soft
takeover.

## Hosted CI and artifacts

The repository is connected to `pheonix666999/DJM--900NXS2-EMULATOR-PLUGIN`. GitHub Actions for the
production-completion branch are pending at the time of this local validation record. The
workflows define:

- `QuadBeatFX-Windows-x64`
- `QuadBeatFX-macOS-Universal`
- `QuadBeatFX-Test-Results-Windows`
- `QuadBeatFX-Test-Results-macOS`

The macOS Universal build and `lipo` verification remain pending on GitHub Actions or macOS
hardware. They have not been claimed as passed.

## Required manual and external validation

Every item in `docs/HARDWARE_VALIDATION.md` remains to be completed with physical interfaces and
MIDI controllers, including ASIO/WASAPI/CoreAudio enumeration, 2/4/8+ channel routing, disconnect
recovery, physical relative encoders, latency/dropouts, sleep/wake, and long-duration CPU/stability.

The builds are unsigned and not notarized. Commercial distribution remains subject to JUCE and
ASIO licensing review; no signing identities or credentials are present.

The production-gap validation adds live background onset analysis, selected analysis sources,
explicit physical role mapping, the microphone bus, quantized activation/capture, fixed Roll/Slip
Roll capture, and editable MIDI ranges/inversion/channels/relative modes/pickup/deletion. Remaining
release limitations are the hosted macOS result, physical hardware matrix, licensing review, and
signing/notarization status described above.
