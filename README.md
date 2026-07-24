# QuadBeat FX

QuadBeat FX is an original, standalone four-channel DJ mixer and Beat FX desktop application for
Windows 10/11 x64 and macOS 12+ (Apple Silicon and Intel). It combines independent channel strips,
three-band EQ/isolator processing, crossfader routing, cue/booth/master buses, tempo analysis, MIDI
mapping, and fifteen real-time stereo effects in a scalable dark interface.

QuadBeat FX is independently designed. It is not affiliated with, endorsed by, or a product of any
other audio-equipment manufacturer. The documentation-only reference image is never used as a skin
or distributed with the application.

## Features

- Four stereo channels with trim, EQ/isolator, faders, mute, cue, metering, and A/B/Thru routing
- Master, booth, and headphones buses with graceful degradation on smaller interfaces
- Automatic live background analysis, manual, and TAP tempo sources; sample-accurate beat clock
  and eight divisions
- Delay, Echo, Ping Pong, Spiral, Reverb, Trans, Filter, Flanger, Phaser, Pitch, Slip Roll, Roll,
  Vinyl Brake, Helix, and Pan
- LOW/MID/HIGH effect-band selection and logical-bus assignment
- ASIO (optional) and WASAPI on Windows; CoreAudio on macOS
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
`arm64;x86_64` app. See [docs/BUILDING.md](docs/BUILDING.md) and [docs/LICENSING.md](docs/LICENSING.md)
before distributing a build.

## Using the application

Open **Settings** to choose the audio driver, device, sample rate, buffer size, physical role
mappings, microphone controls, automatic-BPM source, MIDI inputs, and diagnostics. Interfaces with
only two outputs can run the Master bus; unavailable Booth and Headphones destinations are not
accessed. Choose an effect and bus, select one or more frequency bands, choose a beat on the X-PAD,
then raise LEVEL/DEPTH. Press `T` to tap tempo and use the arrow keys to navigate beat divisions.

GitHub Actions builds and tests both platforms. Open a workflow run's **Artifacts** section to
download `QuadBeatFX-Windows-x64` or `QuadBeatFX-macOS-Universal`. CI artifacts and releases are
unsigned unless explicitly stated.

## Repository map

- `src/audio`, `src/dsp`, `src/bpm`: real-time engine and tempo logic
- `src/midi`, `src/state`: mapping and persistence
- `src/ui`, `src/app`: JUCE interface and application lifecycle
- `tests`: hardware-free deterministic validation
- `docs`: design, use, build, licensing, and hardware validation
- `external/JUCE`: JUCE 8.0.13 pinned submodule

Project code is currently all-rights-reserved. JUCE and optional ASIO distribution require an
independent licensing review. See `LICENSE.md` and `THIRD_PARTY_NOTICES.md`.
