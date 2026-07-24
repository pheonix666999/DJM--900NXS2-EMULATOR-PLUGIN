# Building

Requirements: CMake 3.24+, a C++20 compiler, Git, and Ninja. Initialize JUCE first:

```sh
git submodule update --init --recursive
```

Windows:

```sh
cmake --preset ci-windows
cmake --build --preset ci-windows
ctest --preset test-windows --output-on-failure
```

Set `-DQB_ENABLE_ASIO=OFF` to make a WASAPI-only development build. Windows CI intentionally uses
ASIO ON but the headless tests require no driver.

macOS:

```sh
cmake --preset ci-macos
cmake --build --preset ci-macos
ctest --preset test-macos --output-on-failure
```

The macOS presets set deployment target 12 and `CMAKE_OSX_ARCHITECTURES=arm64;x86_64`. All build
trees live under `out/`. The minimum application window is 1200×820.
