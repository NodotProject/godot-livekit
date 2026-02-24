# API Reference

This is the API reference for the Godot-LiveKit GDExtension. Note that this extension is in active development and more classes will be added over time.

## Core Classes

### `LiveKitRoom`
Represents a LiveKit room. Handles connecting, disconnecting, and room-level signals.

**Methods:**
*   `connect_to_room(url: String, token: String, options: Dictionary) -> bool`: Connects to a LiveKit server using the provided WebSocket URL and access token.
*   `disconnect_from_room()`: Disconnects from the current room.
*   `get_local_participant() -> LiveKitParticipant`: Returns the local participant object.
*   `get_remote_participants() -> Dictionary`: Returns a dictionary of remote participants, keyed by their SID.

**Signals:**
*   `connected`: Emitted when successfully connected to the room.
*   `disconnected`: Emitted when disconnected from the room.
*   `participant_connected(participant: LiveKitParticipant)`: Emitted when a new remote participant joins the room.
*   `participant_disconnected(participant: LiveKitParticipant)`: Emitted when a remote participant leaves the room.

### `LiveKitParticipant`
Base class for a user in the room.

**Methods:**
*   `get_sid() -> String`: Returns the unique Session ID of the participant.
*   `get_name() -> String`: Returns the name of the participant.
*   `get_identity() -> String`: Returns the identity of the participant.
*   `get_metadata() -> String`: Returns the metadata associated with the participant.

### `LiveKitLocalParticipant` (Inherits `LiveKitParticipant`)
Represents the local user. Handles publishing tracks and data (Functionality upcoming).

### `LiveKitRemoteParticipant` (Inherits `LiveKitParticipant`)
Represents a remote user.

---

## Upcoming Classes (Planned)

The following classes are planned for future phases of the implementation:

### Tracks and Media
*   **`LiveKitTrackPublication`**: Base track publication.
*   **`LiveKitLocalTrackPublication`**: Represents a track published by the local participant.
*   **`LiveKitRemoteTrackPublication`**: Represents a track published by a remote participant.
*   **`LiveKitTrack`**: Base class for media tracks.
*   **`LiveKitLocalAudioTrack`**: Local audio stream.
*   **`LiveKitLocalVideoTrack`**: Local video stream.
*   **`LiveKitRemoteAudioTrack`**: Remote audio stream.
*   **`LiveKitRemoteVideoTrack`**: Remote video stream.
*   **`LiveKitAudioSource`**: Source for creating local audio tracks.
*   **`LiveKitVideoSource`**: Source for creating local video tracks.

### Media Streams
*   **`LiveKitVideoStream`**: Receives video frames from a `VideoTrack`. Should output an `ImageTexture`.
*   **`LiveKitAudioStream`**: Receives audio frames from an `AudioTrack`. Pipes to `AudioStreamGenerator`.

### Data and Advanced
*   **`LiveKitE2eeOptions`**: End-to-end encryption configuration.