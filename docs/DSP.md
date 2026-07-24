# DSP design

Channel crossover points are centralized at 250 Hz and 2.5 kHz. The current topology uses smoothed
one-pole splits; the three bands recombine algebraically at unity. Classic EQ provides ±12 dB.
Isolator mode extends attenuation and implements a hard, stable band kill at its minimum.

Effect feedback is bounded below 0.9, buffers are allocated in `prepare`, outputs are finite-clamped,
and equal-power dry/wet mixing avoids level holes. Delay modes use interpolated ring-buffer reads.
Spiral combines cross-channel, polarity, and modulated unequal delay paths. Reverb uses cross-fed
decorrelated comb paths. Pitch uses dual overlap-window delay read heads (bounded-latency granular
shifting). Slip Roll keeps writing live input while reading its loop; Roll repeats the captured
region. Vinyl Brake decelerates a ring-buffer read head. Helix uses two rotating loop taps and
cross-channel feedback, distinct from delay/flanger. Pan uses an equal-power synchronized LFO.

Quantization aligns loop/effect operations to the beat model where capture timing is applicable;
continuous modulation and ordinary parameter smoothing remain sample-continuous.
