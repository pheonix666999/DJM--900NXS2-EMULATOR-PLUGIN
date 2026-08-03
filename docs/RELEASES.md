# Releases

Tags matching `v*` trigger clean Windows x64 and macOS Universal builds, tests, package checksums,
and a GitHub release. Packages include the standalone app, VST3 bundle, quick-start README, license,
and notices; they
exclude source references, tests, Git data, build intermediates, and credentials.

Ordinary CI and release outputs are unsigned. Future signing may use protected secrets for a
Windows signing certificate/password and Apple signing identity/certificate/notarization
credentials. Never commit those values. Mark release notes clearly until signing and notarization
are operational.
