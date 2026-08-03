# Architecture

`MixerEngine` is the real-time boundary. It owns preallocated channel, crossfader, cue, master,
booth, and headphones buffers. Atomics transfer scalar controls from the message/MIDI threads;
the callback performs no file access, logging, locks, or intentional allocation. Four stereo input
pairs feed independent EQ/isolator strips, then A/B/Thru buses, effect assignment, master, booth,
and headphones destinations.

`EffectRack` uses one prepared eight-second stereo ring buffer, bounded feedback, smoothed time and
depth, finite-value clamps, band splitting, and mode-specific state. `TempoEngine`, `BeatClock`,
`MidiMapper`, and `StateStore` are independent, testable services. The JUCE application owns the
device manager and UI; state I/O occurs only during lifecycle events outside the audio callback.

Live tempo analysis is single-producer/single-consumer. The audio callback reduces the selected
Master, CH1–CH4, or microphone source to a 100 Hz positive-energy onset stream and writes it into a
fixed lock-free ring. A low-priority worker consumes fixed 12-second windows, runs deterministic
autocorrelation, and publishes BPM/confidence atomically. Queue overflow drops analysis samples
without ever delaying audio.

Physical device buffers are translated through persisted atomic routing tables into nine logical
inputs (four stereo pairs and microphone) and six logical outputs. Missing or out-of-range physical
channels remain silent. Duplicate input mappings intentionally support signal duplication; output
buses mapped to the same physical destination are summed.

The VST3 adapter presents the same nine logical inputs as four named stereo input buses plus one
mono microphone bus. It copies those buses into preallocated engine buffers in bounded chunks and
returns the engine's stereo Master bus to the host. Channel 1 and Master are enabled by default;
hosts may enable the remaining input buses. The shared `MainComponent` runs in hosted mode, where
the host owns audio-device configuration and UI scaling resizes the editor rather than the desktop.
