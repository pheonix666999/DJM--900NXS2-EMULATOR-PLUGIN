# QuadBeat FX contributor instructions

QuadBeat FX is an original-brand, four-channel standalone DJ mixer and Beat FX application.
Read this file and the relevant documents under `docs/` before changing code.

- Configure/build/test with the documented CMake presets in `docs/BUILDING.md`.
- Format project C++ with `clang-format -i src/**/*.{h,cpp} tests/**/*.{h,cpp}`.
- Keep commits focused; never commit outputs, credentials, packages, or private SDK files.
- The audio callback must allocate nothing, lock no blocking mutex, perform no I/O, log nothing,
  resize no container, and throw no exception. Prepare all buffers before playback.
- Update documentation and tests whenever behavior changes; keep GitHub Actions green.
- Required behavior must be functional: do not add placeholders, fake controls, or aliased effects.
- Never modify `docs/reference/djm900nxs2-effects-reference.png` or package it.
- Never introduce third-party manufacturer names, logos, product identity, or implied affiliation.
- Preserve unrelated user work and leave the working tree clean after a completed task.
