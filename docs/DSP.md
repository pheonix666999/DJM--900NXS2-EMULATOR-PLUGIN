# DSP design

Channel crossover points are centralized at 250 Hz and 2.5 kHz. The current topology uses smoothed
one-pole splits; the three bands recombine algebraically at unity. Classic EQ provides ±12 dB.
Isolator mode extends attenuation and implements a hard, stable band kill at its minimum.

Effect feedback is bounded below 0.9, buffers are allocated in `prepare`, outputs are finite-clamped,
and equal-power dry/wet mixing avoids level holes. Delay modes use interpolated ring-buffer reads.
Spiral combines cross-channel, polarity, and modulated unequal delay paths. Reverb uses eight
parallel damped comb filters followed by four serial all-pass diffusers per side; TIME controls room
size and damping while the stereo width remains decorrelated. Pitch uses dual overlap-window delay
read heads (bounded-latency granular shifting). Slip Roll keeps writing live input while reading its loop; Roll repeats the captured
region. Vinyl Brake decelerates a ring-buffer read head. Helix uses two rotating loop taps and
cross-channel feedback, distinct from delay/flanger. Pan uses an equal-power synchronized LFO.

Quantization maintains a sample-counted boundary from BPM and the selected division. Enable/release
requests remain pending until that boundary; disabling Quantize applies them immediately. Roll and
Slip Roll snapshot a fixed beat-sized region at activation while the live ring continues advancing.
Slip Roll blends and returns to current live input on release. Continuous modulation and ordinary
parameter smoothing remain sample-continuous.
