# Audio routing

Logical stereo inputs CH1–CH4 map by default to physical pairs 1/2, 3/4, 5/6, and 7/8. Master maps
to outputs 1/2, Booth to 3/4, and Headphones/Cue to 5/6. Every role can be remapped or marked
Unavailable in Settings. The device callback checks every selected channel before access. A
two-output interface therefore runs Master safely while Booth and Headphones remain unavailable.

Each channel passes trim and EQ/isolator, mute and fader, then A, B, or Thru. Cue is tapped
post-channel processing. The selected FX bus is processed exactly once: an individual channel,
crossfader A/B, or Master. The mono microphone input is duplicated to stereo, has independent
level/mute/cue controls, can be selected for Beat FX, and remains silent when its physical mapping
is unavailable. Routing, microphone controls, and automatic-BPM analysis source persist in the
versioned application state.
