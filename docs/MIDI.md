# MIDI

Enable MIDI inputs in Settings. MIDI mappings identify parameter, channel, CC/note number, min/max,
inversion, and mode. Absolute CC uses pickup/soft takeover; button/note mapping supports toggles;
two's-complement and binary-offset relative encoder models are available in the mapping model.
Mappings serialize with application state and are processed outside the audio callback before
atomic parameter publication.

Major mixer, crossfader, monitoring, effect, X-PAD, TAP, quantize, band, TIME, and LEVEL/DEPTH
controls have stable parameter identities for mapping. Controller behavior varies, so validate
relative mode and pickup with the hardware checklist.
