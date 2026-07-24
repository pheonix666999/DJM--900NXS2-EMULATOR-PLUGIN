# Continuous integration

`ci.yml` validates formatting/repository policy on Ubuntu, then builds and tests Windows x64 and
macOS Universal artifacts. The macOS job verifies both architectures with `lipo`; both platform
jobs verify expected products, generate SHA-256 files, and retain artifacts for 14 days. Hosted CI
does not perform the hardware matrix.

`release.yml` rebuilds tagged `v*` commits and creates unsigned release archives. Signing and
notarization are intentionally optional and require repository secrets plus a documented security
review.
