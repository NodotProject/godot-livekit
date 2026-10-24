# Installation

This guide covers how to install and set up the Godot-LiveKit GDExtension in your project.

## Prerequisites

To build the extension from source, you will need:

- **Python 3.x**
- **SCons** (`pip install scons`)
- **CMake** (Optional, for advanced builds)

**Platform-Specific Requirements:**
- **Linux:** `g++`, `curl`, `tar`, `unzip`
- **macOS:** Xcode Command Line Tools, `curl`, `tar`, `unzip`
- **Windows:** MSVC (Visual Studio Build Tools) or MinGW-w64 (if cross-compiling from Linux/macOS)
- **Web:** `emcc` from [emsdk](https://emscripten.org), at the version Godot's web templates were built with (4.0.11 for Godot 4.5), plus `curl` and `tar`

## Building from Source

This repository includes a custom `build.sh` script that automatically fetches the required LiveKit C++ SDK and `godot-cpp` prebuilt binaries, and compiles the extension.

1. **Clone the repository:**
   ```bash
   git clone https://github.com/NodotProject/godot-livekit.git
   cd godot-livekit
   ```

2. **Run the build script for your platform:**

   - **Linux:**
     ```bash
     ./build.sh linux
     ```
   - **macOS:**
     ```bash
     ./build.sh macos
     ```
   - **Windows:**
     ```bash
     ./build.sh windows
     ```
   - **Web:**
     ```bash
     ./build.sh web
     ```

Once the build is complete, the compiled dynamic libraries (`.so`, `.dylib`, or `.dll`) and the LiveKit shared libraries will be placed in the `addons/godot-livekit/bin/` directory.

## Web

Web exports use a separate build of the extension that implements the same classes on top of [livekit-client](https://github.com/livekit/client-sdk-js), which is embedded in it. Web support is in progress. So far it covers rooms, participants, track publications, data messages, and audio (sources, local audio tracks and streams). Classes that aren't available yet (such as video sources and streams) aren't registered on the web, so `ClassDB.class_exists()` reports them as missing.

To export for the web:

- Enable **Extensions Support** in the Web export preset, and leave **Thread Support** off. The web build is single-threaded, like Godot's default web templates.
- Audio runs on the browser's `AudioContext`, which browsers only start after a user gesture (a click or key press). Audio sources are resampled to its rate, so they needn't use 10ms frames as they do natively.
- LiveKit events are delivered by the browser between frames, so wait for them by yielding (e.g. `await get_tree().process_frame`), not with blocking loops like `OS.delay_msec()`.

## Installation in Godot

Once you have built the extension from source, or downloaded a pre-built release:

1. Copy the `addons/godot-livekit` folder into your Godot project's `addons/` directory.
2. Enable the plugin from the Godot Editor: **Project -> Project Settings -> Plugins**.

You are now ready to start using LiveKit in your Godot project. Proceed to the [Quickstart](quickstart) guide to see how to connect to a room.