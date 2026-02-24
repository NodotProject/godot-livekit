# Godot-LiveKit Implementation Plan

This document outlines the comprehensive plan for wrapping the LiveKit C++ SDK into a Godot 4.5 GDExtension (`godot-livekit`). It details the architecture, class mapping, and phased implementation strategy required to cover the entire LiveKit API.

## 1. Architectural Guidelines

### 1.1 Class Design
- **Base Classes**: Most LiveKit C++ classes are managed via `std::shared_ptr`. In Godot, these will be mapped to classes inheriting from `RefCounted` to ensure automatic memory management via Godot's reference counting.
- **Singletons/Managers**: There may be a need for a global initialization singleton (e.g., `LiveKitManager`) to handle SDK initialization and the global event loop if necessary.

### 1.2 Event Handling (Signals)
- The LiveKit C++ SDK uses delegates (e.g., `RoomDelegate`) for asynchronous events.
- These delegates will be implemented internally in the GDExtension. When a delegate fires a callback on a background thread, the event must be queued and deferred to Godot's main thread, then emitted as a Godot `Signal`.
- Use `call_deferred` or a custom message queue to safely dispatch events from LiveKit's WebRTC threads to Godot's rendering/main thread.

### 1.3 Media Handling
- **Audio**: Audio frames (`AudioFrame`) received from `AudioStream` need to be piped into Godot's `AudioStreamGeneratorPlayback` to play via Godot's audio engine.
- **Video**: Video frames (`VideoFrame`) received from `VideoStream` need to be converted to Godot `Image` and then updated in an `ImageTexture` so they can be applied to Godot `TextureRect` or 3D materials.
- **High-Frequency Data (Frame Buffers)**: Avoid using `call_deferred` for high-frequency events like receiving video or audio frames, as it incurs too much overhead. Instead, implement a thread-safe queue (e.g., a mutex-protected buffer). The WebRTC background threads will push frames into this queue, and Godot will pull and process them on the main thread during `_process()`.
- **Pixel Formats**: `livekit::VideoFrame` typically provides data in the I420 format. Efficient conversion from I420 to Godot's native RGBA format in C++ (or via a custom GPU shader) is required before handing the data to Godot's `ImageTexture`.

---

## 2. API Class Mapping

Below is the mapping of every major LiveKit SDK component to its corresponding Godot GDExtension class.

### 2.1 Core
| LiveKit C++ SDK | Godot GDExtension Class | Inherits | Description |
| --- | --- | --- | --- |
| `livekit::Room` | `LiveKitRoom` | `RefCounted` | Represents a LiveKit room. Handles connecting, disconnecting, and room-level signals. |
| `livekit::RoomDelegate` | *(Internal)* | N/A | Internally receives callbacks and translates them to signals on `LiveKitRoom`. |
| `livekit::ConnectOptions`, `RoomOptions` | `Dictionary` / `LiveKitRoomOptions` | `RefCounted` | Configuration objects passed during connection. |

### 2.2 Participants
| LiveKit C++ SDK | Godot GDExtension Class | Inherits | Description |
| --- | --- | --- | --- |
| `livekit::Participant` | `LiveKitParticipant` | `RefCounted` | Base class for a user in the room. |
| `livekit::LocalParticipant` | `LiveKitLocalParticipant` | `LiveKitParticipant` | Represents the local user. Handles publishing tracks and data. |
| `livekit::RemoteParticipant` | `LiveKitRemoteParticipant` | `LiveKitParticipant` | Represents a remote user. |

### 2.3 Publications
| LiveKit C++ SDK | Godot GDExtension Class | Inherits | Description |
| --- | --- | --- | --- |
| `livekit::TrackPublication` | `LiveKitTrackPublication` | `RefCounted` | Base track publication. |
| `livekit::LocalTrackPublication` | `LiveKitLocalTrackPublication` | `LiveKitTrackPublication` | Represents a track published by the local participant. |
| `livekit::RemoteTrackPublication` | `LiveKitRemoteTrackPublication` | `LiveKitTrackPublication` | Represents a track published by a remote participant. |

### 2.4 Tracks & Sources
| LiveKit C++ SDK | Godot GDExtension Class | Inherits | Description |
| --- | --- | --- | --- |
| `livekit::Track` | `LiveKitTrack` | `RefCounted` | Base class for media tracks. |
| `livekit::LocalAudioTrack` | `LiveKitLocalAudioTrack` | `LiveKitTrack` | Local audio stream. |
| `livekit::LocalVideoTrack` | `LiveKitLocalVideoTrack` | `LiveKitTrack` | Local video stream. |
| `livekit::RemoteAudioTrack` | `LiveKitRemoteAudioTrack` | `LiveKitTrack` | Remote audio stream. |
| `livekit::RemoteVideoTrack` | `LiveKitRemoteVideoTrack` | `LiveKitTrack` | Remote video stream. |
| `livekit::AudioSource` | `LiveKitAudioSource` | `RefCounted` | Source for creating local audio tracks. |
| `livekit::VideoSource` | `LiveKitVideoSource` | `RefCounted` | Source for creating local video tracks. |

### 2.5 Media Streams & Frames
| LiveKit C++ SDK | Godot GDExtension Class | Inherits | Description |
| --- | --- | --- | --- |
| `livekit::VideoStream` | `LiveKitVideoStream` | `Node` or `RefCounted` | Receives video frames from a `VideoTrack`. Should output an `ImageTexture`. |
| `livekit::AudioStream` | `LiveKitAudioStream` | `Node` or `RefCounted` | Receives audio frames from an `AudioTrack`. Pipes to `AudioStreamGenerator`. |
| `livekit::VideoFrame` | *(Internal)* / `Image` | N/A | Converted internally to Godot `Image`. |
| `livekit::AudioFrame` | *(Internal)* / `PackedFloat32Array`| N/A | Converted internally to raw audio buffers for Godot. |

### 2.6 Data & Advanced
| LiveKit C++ SDK | Godot GDExtension Class | Inherits | Description |
| --- | --- | --- | --- |
| `livekit::DataPacket` | `PackedByteArray` | N/A | Translated directly to Godot byte arrays. |
| `livekit::RpcError` | `Dictionary` / `LiveKitRpcError` | `RefCounted` | Represents RPC call failures. |
| `livekit::E2eeOptions` | `LiveKitE2eeOptions` | `RefCounted` | End-to-end encryption configuration. |

---

## 3. Implementation Phases

To ensure steady progress and maintainable code, the development will be broken down into the following phases.

### Phase 1: Core Setup & Connection ✅
**Goal**: Connect to a room, disconnect, and handle basic room state.
- Initialize LiveKit SDK (if required by SDK versions).
- Implement `LiveKitRoom` shell.
- Implement the `RoomDelegate` wrapper to translate C++ callbacks into Godot deferred signals (e.g., `connected`, `disconnected`, `room_update`).
- Expose the `.connect(url, token, options)` and `.disconnect()` methods to GDScript.

### Phase 2: Participant Management ✅
**Goal**: Track who is in the room.
- Implement `LiveKitParticipant`, `LiveKitLocalParticipant`, and `LiveKitRemoteParticipant`.
- Handle `participant_connected` and `participant_disconnected` signals in the Room.
- Populate `LiveKitRoom` methods like `get_local_participant()` and `get_remote_participants()`.
- Add dictionary/properties for participant metadata, identity, attributes, and kind.

### Phase 3: Track Subscriptions & Data Channels ✅
**Goal**: Know when participants publish tracks and handle arbitrary data messages.
- Implement `LiveKitTrackPublication` hierarchy (`LocalTrackPublication`, `RemoteTrackPublication`).
- Expose properties for Track SID, Name, Kind, Source, and Muted state.
- Wire signals: `track_published`, `track_unpublished`, `track_subscribed`, `track_unsubscribed`, `track_muted`, `track_unmuted`, `local_track_published`, `local_track_unpublished`.
- Implement data channel publishing (`local_participant.publish_data()`) and receiving (`room.data_received` signal).

### Phase 4: Video Streaming ✅
**Goal**: Render remote video to the Godot screen.
- Implement `LiveKitTrack` base class with `LiveKitLocalVideoTrack` and `LiveKitRemoteVideoTrack`.
- Implement `LiveKitVideoStream` to consume video tracks or participant sources.
- Write the frame conversion logic: `livekit::VideoFrame` -> Godot `Image` -> `ImageTexture`.
- Expose a `get_texture()` method and `poll()` for frame updates with thread-safe texture updating.

### Phase 5: Audio Streaming ✅
**Goal**: Hear remote audio.
- Implement `LiveKitLocalAudioTrack` and `LiveKitRemoteAudioTrack`.
- Implement `LiveKitAudioStream` to consume audio tracks or participant sources.
- Write the audio sink logic: pipe audio data into Godot's `AudioStreamGeneratorPlayback` via `poll()`.

### Phase 6: Local Publishing (Camera/Mic) ✅
**Goal**: Send Godot viewport/camera or Godot microphone input to the room.
- Implement `LiveKitVideoSource` with `capture_frame()` accepting Godot `Image`.
- Implement `LiveKitAudioSource` with `capture_frame()` accepting `PackedFloat32Array`.
- Bind `local_participant.publish_track()` and `unpublish_track()`.

### Phase 7: Advanced Features (Partial)
**Goal**: Polish and complete the API.
- ✅ Implement RPC methods (`perform_rpc`, `register_rpc_method`, `respond_to_rpc`).
- ⬜ Expose End-to-End Encryption (E2EE) classes and setup.
- ⬜ Gather and expose network/webrtc connection statistics (`livekit::Stats`).

### Phase 8: Exporting & Packaging
**Goal**: Ensure the GDExtension can be successfully exported with Godot projects across target platforms.
- Configure `godot-livekit.gdextension` to correctly map and bundle the pre-built LiveKit dynamic libraries (`.dll`, `.so`, `.dylib`, frameworks) alongside the extension binaries.
- Ensure dependency paths for shared libraries are correctly set up (e.g., using `@rpath` on macOS) so they are found at runtime in exported builds.

---

## 4. Signal Naming Conventions
All Godot signals will follow Godot's `snake_case` convention.
*Example:*
- `onConnected` -> `connected`
- `onTrackSubscribed` -> `track_subscribed(track, publication, participant)`
- `onDataReceived` -> `data_received(data, participant, kind)`

## 5. Thread Safety & Memory
LiveKit's core runs heavily on background WebRTC threads.
- **Rule 1**: NEVER call Godot scene tree functions directly from a LiveKit delegate callback.
- **Rule 2**: ALWAYS use `call_deferred` or a thread-safe command queue to emit signals and update `RefCounted` states.
- **Rule 3**: Rely on Godot's `Ref<T>` internally when passing GDExtension objects to avoid memory leaks or dangling pointers during asynchronous callbacks.