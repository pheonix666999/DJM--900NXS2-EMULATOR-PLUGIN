# QuadBeat FX

QuadBeat FX is an original DJ mixer and Beat FX application for Windows 10/11 x64 and
macOS 12+ (Apple Silicon and Intel). It is delivered as both a standalone application and a VST3
effect. Its engine combines multi-channel routing, three-band processing, cue/booth/master buses,
tempo analysis, MIDI mapping, and fifteen real-time stereo effects behind a tall hardware-inspired
effects workflow.

QuadBeat FX is independently designed. It is not affiliated with, endorsed by, or a product of any
other audio-equipment manufacturer. The documentation-only reference image is never used as a skin
or distributed with the application.

## Features

- Four stereo processing paths with trim, EQ/isolator, mute, cue, metering, and A/B/Thru routing
- Master, booth, and headphones buses with graceful degradation on smaller interfaces
- Automatic live background analysis, manual, and TAP tempo sources; sample-accurate beat clock
  and eight divisions
- Delay, Echo, Ping Pong, Spiral, Reverb, Trans, Filter, Flanger, Phaser, Pitch, Slip Roll, Roll,
  Vinyl Brake, Helix, and Pan
- LOW/MID/HIGH effect-band selection and logical-bus assignment
- ASIO (optional) and WASAPI on Windows; CoreAudio on macOS
- Host-compatible VST3 operation with one conventional stereo input and stereo output
- Persistent physical routing/microphone state and editable MIDI mappings with scaling, relative
  modes, inversion, deletion, and soft takeover

Current limitations: unsigned builds may trigger operating-system warnings; hardware routing,
relative encoder variations, and long-duration performance require the manual validation matrix.
Automatic BPM analysis is deterministic and intentionally optimized for steady dance tempos.

## Setup and build

```sh
git clone --recurse-submodules <repository-url>
cd <repository>
git submodule update --init --recursive
cmake --preset ci-windows
cmake --build --preset ci-windows
ctest --preset test-windows --output-on-failure
```

For macOS, substitute `ci-macos` and `test-macos`. The macOS preset produces a Universal
`arm64;x86_64` app and VST3. See [docs/BUILDING.md](docs/BUILDING.md) and
[docs/LICENSING.md](docs/LICENSING.md) before distributing a build.

## Using the application

Open **Settings** to choose the audio driver, device, sample rate, buffer size, physical role
mappings, microphone controls, automatic-BPM source, MIDI inputs, and diagnostics. Interfaces with
only two outputs can run the Master bus; unavailable Booth and Headphones destinations are not
accessed. Choose an effect, select one or more frequency bands, choose a beat on the X-PAD,
then raise LEVEL/DEPTH. Press `T` to tap tempo and use the arrow keys to navigate beat divisions.

For VST3 use on Windows, extract the complete package and run `Install-QuadBeatFX-VST3.cmd`. It
installs without administrator access to `%LOCALAPPDATA%\Programs\Common\VST3`. In FL Studio, enable **Verify plugins**,
**Rescan previously verified plugins**, and **Rescan plugins with errors**, then run **Find installed
plugins**. Load QuadBeat FX in a Mixer effect slot. See `INSTALL-VST3-WINDOWS.txt` in the package.
The hosted editor opens as a narrow vertical Beat FX strip, resizes down to 340 x 900, and does not
show the four internal channel strips.

GitHub Actions builds and tests both platforms. Open a workflow run's **Artifacts** section to
download `QuadBeatFX-Windows-x64` or `QuadBeatFX-macOS-Universal`. CI artifacts and releases are
unsigned unless explicitly stated.

## Repository map

- `src/audio`, `src/dsp`, `src/bpm`: real-time engine and tempo logic
- `src/midi`, `src/state`: mapping and persistence
- `src/ui`, `src/app`, `src/plugin`: shared JUCE interface, standalone lifecycle, and VST3 adapter
- `tests`: hardware-free deterministic validation
- `docs`: design, use, build, licensing, and hardware validation
- `external/JUCE`: JUCE 8.0.13 pinned submodule

Project code is currently all-rights-reserved. JUCE and optional ASIO distribution require an
independent licensing review. See `LICENSE.md` and `THIRD_PARTY_NOTICES.md`.
