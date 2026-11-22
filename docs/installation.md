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
- **Android:** the Android NDK (r27 or later) in `ANDROID_NDK_ROOT`, plus `git`, CMake, Ninja and Rust (`rustup`) to build the LiveKit SDK
- **iOS:** macOS with Xcode, plus `git`, CMake, Ninja and Rust (`rustup`) to build the LiveKit SDK

## Building from Source

This repository includes a custom `build.sh` script that fetches the LiveKit C++ SDK (building it from source for Android and iOS) and `godot-cpp`, and compiles the extension.

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
   - **Android** (arm64, or `x86_64` for emulators):
     ```bash
     ./build.sh android
     ```
   - **iOS** (devices, then the simulator; each build adds to the same `.xcframework`s):
     ```bash
     ./build.sh ios
     ./build.sh ios --simulator
     ```

Once the build is complete, the compiled libraries (`.so`, `.dylib`, `.dll`, or `.xcframework`) and the LiveKit shared libraries will be placed in the `addons/godot-livekit/bin/` directory.

## Android and iOS

The Android and iOS builds include every class except `LiveKitScreenCapture`, which isn't registered on mobile.

To send microphone audio, capture it with Godot (e.g. an `AudioStreamMicrophone` and an `AudioEffectCapture`) and enable **Audio > Driver > Enable Input** in the project settings.

To export for Android:

- Enable **Use Gradle Build** in the Android export preset, along with the **Internet**, **Access Network State**, **Record Audio** and **Modify Audio Settings** permissions.
- The app also needs WebRTC's Java classes, which come in the addon as `bin/libwebrtc.jar`. The extension adds them to Gradle builds from the editor, so keep the library for the editor's platform in the addon too (e.g. `./build.sh linux` when building from source). Release packages include both.

To export for iOS:

- The iOS build runs on arm64 devices with iOS 14 or later, and in the iOS simulator on Macs with Apple silicon.
- The editor loads the extension's macOS library while exporting, so keep it in the addon (`./build.sh macos` when building from source). Release packages include it.
- To record audio, set **Privacy > Microphone Usage Description** in the iOS export preset, since iOS stops apps that use the microphone without one. Also set **Audio > General > iOS > Session Category** to **Play and Record** in the project settings, since Godot's default (Ambient) can't record.
- Godot's official export templates can't run in the simulator on Apple silicon ([godotengine/godot#122379](https://github.com/godotengine/godot/issues/122379)), so testing there needs a Godot simulator library built from source, as `test_ios.sh` describes.

## Web

Web exports use a separate build of the extension that implements the same classes on top of [livekit-client](https://github.com/livekit/client-sdk-js), which is embedded in it. Every class is available except `LiveKitAudioProcessingModule`, which isn't registered on the web, since browsers apply their own processing to captured audio, so `ClassDB.class_exists()` reports it as missing.

To export for the web:

- Enable **Extensions Support** in the Web export preset, and leave **Thread Support** off. The web build is single-threaded, like Godot's default web templates.
- LiveKit events are delivered by the browser between frames, so wait for them by yielding (e.g. `await get_tree().process_frame`), not with blocking loops like `OS.delay_msec()`.
- Browsers only start audio after a user gesture (a click or key press), and may only allow screen capture after one, so call `LiveKitScreenCapture.start()` from an input event handler.

Some behavior differs where browsers work differently from native WebRTC, e.g. video rotation is applied to the pixels, screen capture asks the user what to share, and encryption keys are ratcheted asynchronously. [Web build differences](https://github.com/NodotProject/godot-livekit/blob/main/src/web/README.md) lists them all.

## Installation in Godot

Once you have built the extension from source, or downloaded a pre-built release:

1. Copy the `addons/godot-livekit` folder into your Godot project's `addons/` directory. Godot loads the extension when the project opens.

You are now ready to start using LiveKit in your Godot project. Proceed to the [Quickstart](quickstart) guide to see how to connect to a room.