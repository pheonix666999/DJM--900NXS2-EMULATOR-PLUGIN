# MIDI

Enable MIDI inputs in Settings. MIDI mappings identify parameter, channel, CC/note number, min/max,
inversion, and mode. Absolute CC uses pickup/soft takeover; button/note mapping supports toggles;
two's-complement and binary-offset relative encoder models are available in the mapping model.
Mappings serialize with application state and are processed outside the audio callback before
atomic parameter publication.

To create a mapping, touch the target UI control, click **MIDI LEARN**, then move a hardware CC or
press a note. Open **MIDI EDIT** to select any mapping and change its MIDI channel, absolute or
relative encoder mode, custom minimum/maximum, inversion, and pickup tolerance. The editor also
deletes mappings. Custom range and inversion are applied before soft-takeover comparison so pickup
remains jump-free.

Major mixer, microphone level/cue/mute, crossfader, monitoring, effect, X-PAD, TAP, quantize, band,
TIME, and LEVEL/DEPTH controls have stable parameter identities for mapping. Controller behavior
varies, so validate relative mode and pickup with the hardware checklist.
