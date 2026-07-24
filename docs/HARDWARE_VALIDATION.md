# Hardware validation matrix

Hosted CI cannot complete these tests. Record interface, driver/firmware, OS, sample rate, buffer,
duration, result, and observed latency/CPU/dropouts for each run.

- Windows ASIO enumeration and WASAPI fallback
- macOS CoreAudio enumeration
- 2-, 4-, and 8-or-more-channel interfaces
- Master, Booth, and Headphones/Cue physical outputs
- Sample-rate switching at 44.1, 48, 88.2, and 96 kHz
- Buffer switching from 32 through 2048 samples
- Device disconnect, safe fallback, reconnect, and mapping restoration
- Physical MIDI knobs, buttons, relative encoders, and soft takeover
- Clipping behavior, underrun/dropout observation, and latency observation
- One-hour long-duration stability and CPU use under all effects
- Sleep/wake, application restart, window/state restoration

Do not mark a platform release-ready until its relevant rows have documented passes.
