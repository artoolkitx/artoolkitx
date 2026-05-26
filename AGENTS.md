# artoolkitX — Agent Guide

## Project Overview

artoolkitX is a cross-platform augmented-reality SDK (LGPL v3) written in C/C++ with a Java binding for Android. It provides camera-based marker tracking (square markers, NFT/natural-feature, 2D image), video capture, and supporting utilities.

Current version set in [Source/CMakeLists.txt](Source/CMakeLists.txt).

---

## Repository Layout

```
artoolkitX/
├── Source/
│   ├── CMakeLists.txt          # Master CMake entry point
│   ├── build.sh                # Master build script (wraps CMake per platform)
│   ├── ARX/                    # Core C++ library (libARX)
│   │   ├── AR/                 # Low-level AR tracking (C)
│   │   ├── AR2/                # NFT (natural-feature tracking)
│   │   ├── ARVideo/            # Video capture abstraction layer
│   │   │   ├── AVFoundation/   # macOS/iOS
│   │   │   ├── Android/
│   │   │   ├── GStreamer/      # Linux
│   │   │   ├── Video4Linux2/
│   │   │   ├── WindowsMediaCapture/ / WindowsMediaFoundation/
│   │   │   └── Emscripten/
│   │   ├── ARG/                # OpenGL rendering helpers
│   │   ├── OCVT/               # OpenCV-based tracker
│   │   ├── KPM/                # Keypoint matching
│   │   ├── ARUtil/             # Utilities (logging, threading, …)
│   │   ├── ARController.cpp    # High-level controller (main public API)
│   │   ├── ARX_c.cpp           # C API (used by Unity / Emscripten)
│   │   ├── ARTracker*.cpp      # Tracker implementations
│   │   ├── ARTrackable*.cpp    # Trackable types
│   │   └── include/ARX/        # Public headers
│   ├── ARXJ/                   # Android Java/JNI wrapper
│   │   └── ARXJProj/           # Android Studio / Gradle project
│   ├── Utilities/              # Command-line tools (mk_patt, genTexData, …)
│   ├── cmake/                  # Platform toolchain files + FindXxx modules
│   ├── depends/                # Pre-built third-party libs (platform-specific)
│   └── packaging/              # Platform package scripts
├── Examples/                   # Example applications
├── SDK/                        # Prebuilt SDK output (populated by build.sh)
├── Documentation/
├── README.md
├── Quick start.md
└── Release Notes.md
```

---

## Build System

### CMake (preferred)

```bash
# From a build directory, e.g. Source/build-macos
cmake ../Source
make -j$(nproc)
```

Platform toolchains live in `Source/cmake/`.

### build.sh (all platforms)

```bash
# Build for macOS:
./Source/build.sh macos

# Build for Android:
./Source/build.sh android

# Other targets: ios | linux | windows | emscripten | docs
# Additional flags: --debug  --verbose  --install-depends  tests  examples
```

### Android (Gradle)

Open `Source/ARXJ/ARXJProj/` in Android Studio, or run:

```bash
cd Source/ARXJ/ARXJProj
./gradlew assembleRelease
```

---

## Key Source Files

| File | Purpose |
| ------ | --------- |
| [Source/CMakeLists.txt](Source/CMakeLists.txt) | Version, platform detection, feature flags |
| [Source/ARX/ARController.cpp](Source/ARX/ARController.cpp) | High-level C++ SDK entry point |
| [Source/ARX/ARX_c.cpp](Source/ARX/ARX_c.cpp) | C/Unity/Emscripten API layer |
| [Source/ARX/ARVideo/video.c](Source/ARX/ARVideo/video.c) | Video backend dispatch |
| [Source/ARX/AR/arDetectMarker.c](Source/ARX/AR/arDetectMarker.c) | Square-marker detection |
| [Source/ARX/AR2/](Source/ARX/AR2/) | Natural-feature tracking |
| [Source/ARX/OCVT/](Source/ARX/OCVT/) | OpenCV-based 2D tracker |

---

## Supported Platforms

| Platform | Toolchain/Generator |
| ---------- | ---------------------
| macOS | Xcode / Unix Makefiles |
| iOS | Xcode / Unix Makefiles with `cmake/ios.toolchain.cmake` |
| Android | NDK + Gradle |
| Linux x86_64 | Unix Makefiles |
| Windows | MSVC / Visual Studio |
| Emscripten | Emscripten toolchain |

---

## Conventions & Patterns

- **Language and style**: C99 for core AR algorithms; C++14 for the controller/tracker layer; Java for the Android wrapper. For C, use K&R style. For C++, follow the Google C++ style guide. Always wrap branches in braces, unless part of a switch block.
- **Naming**: `AR` prefix for C symbols; `ARX` prefix for the C++ layer; `arx` prefix for C API functions exposed externally.
- **Error handling**: Return codes (`AR_E_*`), not exceptions. C++ layer logs via `ARLOGx()` macro where x is the log level (e=error, w=warning, i=info, d=debug).
- **CMake options**: Feature flags are `option()`-declared in the top-level CMakeLists. Platform guards use `ARX_PLATFORM_NAME`.
- **Headers**: Public API headers live in `Source/ARX/include/ARX/`. Internal headers are beside their `.c`/`.cpp` files.

---

## Common Agent Tasks

### Bumping the version
Edit three lines in [Source/CMakeLists.txt](Source/CMakeLists.txt):
```cmake
set(ARX_VERSION_MAJOR ...)
set(ARX_VERSION_MINOR ...)
set(ARX_VERSION_TINY  ...)
```
Also update [Release Notes.md](Release%20Notes.md) and [ChangeLog.txt](ChangeLog.txt).

### Running utilities
Built tools land in `SDK/bin/` after a build. Examples:
```bash
SDK/bin/mk_patt          # Generate square markers
SDK/bin/genTexData       # Generate NFT texture data
```

--

## Source control

- When naming branches, use the first name of the committer, followed by a few words describing the work, hyphenated, and all lower-case.
- Keep commit messages brief; don't put rationale into the commit message. Where multiple changes have been made in a single commit, place each on its own line preceded by a hyphen.
- When committing, don't add Co-authored-by for any agents; in this project this field is strictly reserved for legal persons.

---

## Do Not

- Do not run `git push` or open PRs without explicit user instruction.
- Do not modify pre-built artifacts in `Source/depends/` or `SDK/`.
- Do not add C++ exceptions or RTTI — the build disables them on some platforms.
- Do not introduce dependencies not already present in `Source/depends/` without discussing with the user first.
Do not add agents to the authored-by field  commit.
