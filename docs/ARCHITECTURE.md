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
