# Godot-LiveKit

![Godot-LiveKit Icon](icon.png)

**Godot-LiveKit** is a GDExtension for Godot 4.5 that integrates the [LiveKit C++ SDK](https://github.com/livekit/client-sdk-cpp), allowing you to build real-time voice, video, and data applications directly within the Godot Engine using GDScript or C#.

## Features

- **Real-Time Communication**: Connect to LiveKit servers for audio, video, and data streaming.
- **Full Track Support**: Publish and subscribe to audio/video tracks, with local sources for capturing from Godot.
- **Screen Capture**: Capture monitors or individual windows natively using the built-in `LiveKitScreenCapture` class (macOS, Windows, Linux).
- **Audio Processing**: Apply WebRTC echo cancellation, noise suppression, and gain control to audio captured in Godot via `LiveKitAudioProcessingModule`.
- **Data Channels**: Send and receive arbitrary data messages with reliable or unreliable delivery.
- **RPC Support**: Perform remote procedure calls between participants.
- **End-to-End Encryption (E2EE)**: Secure your media streams with configurable encryption, key management, and per-participant frame cryptors.
- **Connection Statistics**: Access detailed WebRTC statistics including inbound/outbound RTP, codecs, transport, and candidate pair metrics.
- **Fast Build Times**: Uses prebuilt binaries for both `godot-cpp` and the LiveKit C++ SDK, reducing compilation time to seconds instead of hours.
- **Cross-Platform**: Supports Linux, macOS (Universal), and Windows, with Web support in progress.
- **Native GDExtension**: Works out-of-the-box with Godot 4.5 without requiring custom engine builds.

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

## Usage in Godot

1. Copy the `addons/godot-livekit` folder into your Godot project's `addons/` directory.
2. Enable the plugin from the Godot Editor: **Project -> Project Settings -> Plugins**.
3. You can now access LiveKit classes directly from GDScript:

```gdscript
extends Node

var room: LiveKitRoom

func _ready():
    room = LiveKitRoom.new()
    room.connected.connect(_on_connected)
    room.participant_connected.connect(_on_participant_connected)
    room.track_subscribed.connect(_on_track_subscribed)
    room.data_received.connect(_on_data_received)
    room.connect_to_room("wss://your-server.url", "your-token", {})

# No _process() needed — rooms, video streams, and screen captures are
# auto-polled every frame by default. Set auto_poll = false if you prefer
# to call poll_events() / poll() manually.

func _on_connected():
    print("Connected as: ", room.get_local_participant().get_identity())

func _on_participant_connected(participant):
    print("Joined: ", participant.get_identity())

func _on_track_subscribed(track, publication, participant):
    print("Track subscribed: ", track.get_name())

func _on_data_received(data, participant, kind, topic):
    print("Data from ", participant.get_identity(), ": ", data.get_string_from_utf8())
```

### Screen Capture

You can capture screens and windows natively:

```gdscript
var capture = LiveKitScreenCapture.create()
capture.frame_received.connect(_on_frame)
capture.start()

func _on_frame():
    var image = capture.get_image()
    # Feed into a LiveKitVideoSource for screen sharing
```

## Web

Web exports use a separate build of the extension that implements the same classes on top of [livekit-client](https://github.com/livekit/client-sdk-js), which is embedded in it. Web support is in progress. So far it covers rooms, participants, track publications and data messages. Classes that aren't available yet (such as audio and video sources) aren't registered on the web, so `ClassDB.class_exists()` reports them as missing.

To export for the web:

- Enable **Extensions Support** in the Web export preset, and leave **Thread Support** off. The web build is single-threaded, like Godot's default web templates.
- LiveKit events are delivered by the browser between frames, so wait for them by yielding (e.g. `await get_tree().process_frame`), not with blocking loops like `OS.delay_msec()`.

## Running Tests

If you have [GUT (Godot Unit Test)](https://github.com/bitwes/Gut) installed in `addons/gut`, you can run the test suite using the provided `test.sh` script:

```bash
./test.sh
```

`./test_web.sh` runs the tests that apply to the web build in headless Chrome. It requires a web build, Godot's web export templates, and `google-chrome`.

## Continuous Integration

The project is configured with GitHub Actions to automatically build releases for Linux, Windows, and macOS whenever a new tag (e.g., `v1.0.0`) is pushed. Check the `.github/workflows` directory for details.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for more information. Note that the LiveKit C++ SDK is licensed under the Apache License 2.0.