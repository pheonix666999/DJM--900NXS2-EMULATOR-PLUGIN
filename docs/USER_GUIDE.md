# User guide

1. Open Settings and choose the driver, device, sample rate, buffer, active channels, and explicit
   CH1–CH4/Microphone/Master/Booth/Headphones physical mappings.
2. Route stereo sources to CH1–CH4. Use trim for nominal level, shape with EQ/Isolator, and select
   A, B, or Thru under each channel fader.
3. Use CUE, CUE MIX, and PHONES for preview; set MASTER and BOOTH independently.
4. In Beat FX, select an effect and logical bus. Keep at least one LOW/MID/HIGH band active.
5. Select a beat on the X-PAD or use arrows. Choose AUTO or press TAP (`T` shortcut).
6. TIME changes the primary duration/rate/pitch behavior. LEVEL/DEPTH uses a smoothed equal-power
   dry/wet/intensity transition.

The console surface is arranged as four channel strips, a dedicated monitor strip, and a tall Beat
FX strip. The Beat FX workflow runs from the OLED and two-row X-PAD at the top through tempo,
frequency, effect, assignment, TIME, and LEVEL/DEPTH controls. Effect and assignment selectors
support vertical drag, mouse wheel, arrow keys, and keyboard focus.

Choose the automatic BPM analysis source in Settings. Live onset extraction runs outside the audio
worker; the display updates with the smoothed estimate. With Quantize enabled, effect and loop
requests wait for the selected beat boundary. Use MIDI LEARN for quick assignment and MIDI EDIT for
range, inversion, encoder mode, pickup tolerance, channel, and deletion.

Red meter persistence indicates clipping; lower trim or master. If a saved device is unavailable,
open Settings and choose an available fallback.
