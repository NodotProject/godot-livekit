# API Reference

This is the API reference for the Godot-LiveKit GDExtension.

---

## Core Classes

### `LiveKitRoom`
Represents a LiveKit room. Handles connecting, disconnecting, and room-level signals.

**Properties:**
*   `local_participant: LiveKitLocalParticipant` (read-only): The local participant object.
*   `remote_participants: Dictionary` (read-only): Remote participants, keyed by their SID.
*   `sid: String` (read-only): The room's unique Session ID.
*   `name: String` (read-only): The room's name.
*   `metadata: String` (read-only): The room's metadata.
*   `connection_state: int` (read-only): The current connection state (see `ConnectionState` enum).

**Methods:**
*   `connect_to_room(url: String, token: String, options: Dictionary) -> bool`: Connects to a LiveKit server using the provided WebSocket URL and access token. Options: `auto_subscribe` (default `true`), `dynacast` (default `false`), `auto_reconnect` (default `true`), `e2ee` (a `LiveKitE2eeOptions`, Linux only).
*   `disconnect_from_room()`: Disconnects from the current room.
*   `get_local_participant() -> LiveKitLocalParticipant`: Returns the local participant object.
*   `get_remote_participants() -> Dictionary`: Returns a dictionary of remote participants, keyed by their SID.
*   `get_sid() -> String`: Returns the room's unique Session ID.
*   `get_name() -> String`: Returns the room's name.
*   `get_metadata() -> String`: Returns the room's metadata.
*   `get_connection_state() -> int`: Returns the current connection state.
*   `get_e2ee_manager() -> LiveKitE2eeManager`: Returns the E2EE manager for configuring end-to-end encryption. *(Linux only)*

**Signals:**
*   `connected`: Emitted when successfully connected to the room.
*   `disconnected`: Emitted when disconnected from the room.
*   `reconnecting`: Emitted when the connection is attempting to reconnect.
*   `reconnected`: Emitted when the connection has successfully reconnected.
*   `participant_connected(participant: LiveKitRemoteParticipant)`: Emitted when a new remote participant joins the room.
*   `participant_disconnected(participant: LiveKitRemoteParticipant)`: Emitted when a remote participant leaves the room.
*   `participant_metadata_changed(participant: LiveKitParticipant, old_metadata: String, new_metadata: String)`: Emitted when a participant's metadata changes.
*   `participant_name_changed(participant: LiveKitParticipant, old_name: String, new_name: String)`: Emitted when a participant's name changes.
*   `participant_attributes_changed(participant: LiveKitParticipant, changed_attributes: Dictionary)`: Emitted when a participant's attributes change.
*   `room_metadata_changed(old_metadata: String, new_metadata: String)`: Emitted when the room's metadata changes.
*   `connection_quality_changed(participant: LiveKitParticipant, quality: int)`: Emitted when a participant's connection quality changes.
*   `track_published(publication: LiveKitRemoteTrackPublication, participant: LiveKitRemoteParticipant)`: Emitted when a remote participant publishes a track.
*   `track_unpublished(publication: LiveKitRemoteTrackPublication, participant: LiveKitRemoteParticipant)`: Emitted when a remote participant unpublishes a track.
*   `track_subscribed(track: LiveKitTrack, publication: LiveKitRemoteTrackPublication, participant: LiveKitRemoteParticipant)`: Emitted when a remote track is subscribed to.
*   `track_unsubscribed(track: LiveKitTrack, publication: LiveKitRemoteTrackPublication, participant: LiveKitRemoteParticipant)`: Emitted when a remote track is unsubscribed from.
*   `track_muted(participant: LiveKitParticipant, publication: LiveKitTrackPublication)`: Emitted when a track is muted.
*   `track_unmuted(participant: LiveKitParticipant, publication: LiveKitTrackPublication)`: Emitted when a track is unmuted.
*   `local_track_published(publication: LiveKitLocalTrackPublication, track: LiveKitTrack)`: Emitted when the local participant publishes a track.
*   `local_track_unpublished(publication: LiveKitLocalTrackPublication)`: Emitted when the local participant unpublishes a track.
*   `data_received(data: PackedByteArray, participant: LiveKitRemoteParticipant, kind: int, topic: String)`: Emitted when a data message is received.
*   `e2ee_state_changed(participant: LiveKitParticipant, state: int)`: Emitted when a participant's E2EE state changes. *(Linux only)*
*   `participant_encryption_status_changed(participant: LiveKitParticipant, is_encrypted: bool)`: Emitted when a participant's encryption status changes. *(Linux only)*

**Enums:**
*   `ConnectionState`: `STATE_DISCONNECTED = 0`, `STATE_CONNECTED = 1`, `STATE_RECONNECTING = 2`

---

## Participants

### `LiveKitParticipant`
Base class for a user in the room.

**Methods:**
*   `get_sid() -> String`: Returns the unique Session ID of the participant.
*   `get_name() -> String`: Returns the name of the participant.
*   `get_identity() -> String`: Returns the identity of the participant.
*   `get_metadata() -> String`: Returns the metadata associated with the participant.
*   `get_attributes() -> Dictionary`: Returns the attributes associated with the participant.
*   `get_kind() -> int`: Returns the participant kind (see `ParticipantKind` enum).

**Enums:**
*   `ParticipantKind`: `KIND_STANDARD = 0`, `KIND_INGRESS = 1`, `KIND_EGRESS = 2`, `KIND_SIP = 3`, `KIND_AGENT = 4`

### `LiveKitLocalParticipant` (Inherits `LiveKitParticipant`)
Represents the local user. Handles publishing tracks, data, and RPC.

**Methods:**
*   `publish_data(data: PackedByteArray, reliable: bool, destination_identities: PackedStringArray, topic: String)`: Publishes a data message to the room.
*   `set_metadata(metadata: String)`: Sets the local participant's metadata.
*   `set_name(name: String)`: Sets the local participant's display name.
*   `set_attributes(attributes: Dictionary)`: Sets the local participant's attributes.
*   `get_track_publications() -> Dictionary`: Returns published track publications.
*   `publish_track(track: LiveKitTrack, options: Dictionary) -> LiveKitLocalTrackPublication`: Publishes a local track to the room.
*   `unpublish_track(track_sid: String)`: Unpublishes a track by its SID.
*   `perform_rpc(destination: String, method: String, payload: String, timeout: float)`: Performs an asynchronous remote procedure call to another participant. The result is delivered via the `rpc_response_received` signal; errors via the `rpc_error` signal.
*   `register_rpc_method(method: String)`: Registers a method name to receive RPC calls.
*   `unregister_rpc_method(method: String)`: Unregisters an RPC method.
*   `respond_to_rpc(request_id: String, payload: String)`: Responds to an incoming RPC request.
*   `respond_to_rpc_error(request_id: String, code: int, message: String)`: Responds to an incoming RPC request with an error.

**Signals:**
*   `rpc_method_invoked(method: String, request_id: String, caller_identity: String, payload: String, response_timeout: float)`: Emitted when an RPC method is invoked by a remote participant. Use `respond_to_rpc()` or `respond_to_rpc_error()` with the `request_id` to reply.
*   `rpc_response_received(method: String, result: String)`: Emitted when an outgoing RPC call succeeds. Contains the method name and the response payload.
*   `rpc_error(method: String, error_message: String)`: Emitted when an outgoing RPC call fails.

### `LiveKitRemoteParticipant` (Inherits `LiveKitParticipant`)
Represents a remote user.

**Methods:**
*   `get_track_publications() -> Dictionary`: Returns the remote participant's track publications.

---

## Tracks

### `LiveKitTrack`
Base class for media tracks.

**Methods:**
*   `get_sid() -> String`: Returns the track's unique Session ID.
*   `get_name() -> String`: Returns the track's name.
*   `get_kind() -> int`: Returns the track kind (see `TrackKind` enum).
*   `get_source() -> int`: Returns the track source (see `TrackSource` enum).
*   `get_muted() -> bool`: Returns whether the track is muted.
*   `get_stream_state() -> int`: Returns the track's stream state (see `StreamState` enum).
*   `request_stats()`: Requests WebRTC statistics for this track asynchronously. Results are delivered via the `stats_received` signal.

**Signals:**
*   `stats_received(stats: Array)`: Emitted with an array of dictionaries containing WebRTC statistics (inbound/outbound RTP, codec, transport, candidate pair metrics, etc.).

**Enums:**
*   `TrackKind`: `KIND_UNKNOWN = 0`, `KIND_AUDIO = 1`, `KIND_VIDEO = 2`
*   `TrackSource`: `SOURCE_UNKNOWN = 0`, `SOURCE_CAMERA = 1`, `SOURCE_MICROPHONE = 2`, `SOURCE_SCREENSHARE = 3`, `SOURCE_SCREENSHARE_AUDIO = 4`
*   `StreamState`: `STATE_UNKNOWN = 0`, `STATE_ACTIVE = 1`, `STATE_PAUSED = 2`

### `LiveKitLocalAudioTrack` (Inherits `LiveKitTrack`)
A local audio track created from a `LiveKitAudioSource`.

**Static Methods:**
*   `create(name: String, source: LiveKitAudioSource) -> LiveKitLocalAudioTrack`: Creates a local audio track from an audio source.

**Methods:**
*   `mute()`: Mutes the track.
*   `unmute()`: Unmutes the track.

### `LiveKitLocalVideoTrack` (Inherits `LiveKitTrack`)
A local video track created from a `LiveKitVideoSource`.

**Static Methods:**
*   `create(name: String, source: LiveKitVideoSource) -> LiveKitLocalVideoTrack`: Creates a local video track from a video source.

**Methods:**
*   `mute()`: Mutes the track.
*   `unmute()`: Unmutes the track.

### `LiveKitRemoteAudioTrack` (Inherits `LiveKitTrack`)
A remote audio track received from another participant.

### `LiveKitRemoteVideoTrack` (Inherits `LiveKitTrack`)
A remote video track received from another participant.

---

## Track Publications

### `LiveKitTrackPublication`
Base class for track publications. A publication represents a track that has been published to the room.

**Methods:**
*   `get_sid() -> String`: Returns the publication's unique Session ID.
*   `get_name() -> String`: Returns the publication's name.
*   `get_kind() -> int`: Returns the track kind.
*   `get_source() -> int`: Returns the track source.
*   `get_muted() -> bool`: Returns whether the track is muted.
*   `get_mime_type() -> String`: Returns the MIME type of the track.
*   `get_simulcasted() -> bool`: Returns whether the track is simulcasted.
*   `get_track() -> LiveKitTrack`: Returns the underlying track object.

### `LiveKitLocalTrackPublication` (Inherits `LiveKitTrackPublication`)
Represents a track published by the local participant.

### `LiveKitRemoteTrackPublication` (Inherits `LiveKitTrackPublication`)
Represents a track published by a remote participant.

**Methods:**
*   `get_subscribed() -> bool`: Returns whether the local participant is subscribed to this track.
*   `set_subscribed(subscribed: bool)`: Subscribes or unsubscribes from this track.

---

## Media Sources

### `LiveKitAudioSource`
Source for creating local audio tracks. Allows capturing audio data from Godot and sending it to the room.

**Static Methods:**
*   `create(sample_rate: int, num_channels: int, queue_size_ms: int) -> LiveKitAudioSource`: Creates a new audio source.

**Methods:**
*   `capture_frame(data: PackedFloat32Array, sample_rate: int, num_channels: int, samples_per_channel: int)`: Captures an audio frame and queues it for sending.
*   `clear_queue()`: Clears the audio queue.
*   `get_queued_duration() -> float`: Returns the duration of queued audio in seconds.
*   `get_sample_rate() -> int`: Returns the configured sample rate.
*   `get_num_channels() -> int`: Returns the configured number of channels.

### `LiveKitVideoSource`
Source for creating local video tracks. Allows capturing video frames from Godot and sending them to the room.

**Static Methods:**
*   `create(width: int, height: int) -> LiveKitVideoSource`: Creates a new video source with the specified resolution.

**Methods:**
*   `capture_frame(image: Image, timestamp_us: int, rotation: int)`: Captures a video frame from a Godot Image.
*   `get_width() -> int`: Returns the configured width.
*   `get_height() -> int`: Returns the configured height.

---

## Media Streams

### `LiveKitVideoStream`
Receives video frames from a remote video track and provides them as an `ImageTexture`.

**Static Methods:**
*   `from_track(track: LiveKitTrack) -> LiveKitVideoStream`: Creates a video stream from a track.
*   `from_participant(participant: LiveKitRemoteParticipant, source: int) -> LiveKitVideoStream`: Creates a video stream from a participant's track source.

**Methods:**
*   `get_texture() -> ImageTexture`: Returns the texture containing the latest video frame.
*   `poll() -> bool`: Polls for new frames and updates the texture. Call this in `_process()`. Returns `true` if a new frame was received.
*   `close()`: Closes the video stream and stops receiving frames.

**Signals:**
*   `frame_received`: Emitted when a new video frame is available.

### `LiveKitAudioStream`
Receives audio frames from a remote audio track and pipes them into Godot's audio system.

**Static Methods:**
*   `from_track(track: LiveKitTrack) -> LiveKitAudioStream`: Creates an audio stream from a track.
*   `from_participant(participant: LiveKitRemoteParticipant, source: int) -> LiveKitAudioStream`: Creates an audio stream from a participant's track source.

**Methods:**
*   `get_sample_rate() -> int`: Returns the audio sample rate.
*   `get_num_channels() -> int`: Returns the number of audio channels.
*   `poll(playback: AudioStreamGeneratorPlayback) -> int`: Polls for new audio data and pushes it to the playback buffer. Call this in `_process()`.
*   `close()`: Closes the audio stream and stops receiving audio.

---

## Screen Capture

### `LiveKitScreenCapture`
Captures screen content from monitors or individual windows using the native [frametap](https://github.com/krazyjakee/frametap) library. Frames are delivered as `ImageTexture` / `Image` objects that can be fed into a `LiveKitVideoSource` for publishing.

**Static Query Methods:**
*   `get_monitors() -> Array`: Returns an array of dictionaries describing available monitors. Each dictionary contains: `id`, `name`, `x`, `y`, `width`, `height`, `scale`.
*   `get_windows() -> Array`: Returns an array of dictionaries describing available windows. Each dictionary contains: `id`, `name`, `x`, `y`, `width`, `height`.
*   `check_permissions() -> Dictionary`: Checks screen capture permissions. Returns a dictionary with `status` (int, see `PermissionLevel` enum), `summary` (String), and `details` (Array of Strings).

**Factory Methods:**
*   `create() -> LiveKitScreenCapture`: Creates a screen capture for the default/primary monitor.
*   `create_for_monitor(monitor: Dictionary) -> LiveKitScreenCapture`: Creates a screen capture for a specific monitor (use a dictionary from `get_monitors()`).
*   `create_for_window(window: Dictionary) -> LiveKitScreenCapture`: Creates a screen capture for a specific window (use a dictionary from `get_windows()`).

**Capture Lifecycle:**
*   `start()`: Starts asynchronous screen capture. Frames arrive via `poll()`.
*   `stop()`: Stops capturing.
*   `pause()`: Pauses capture without stopping.
*   `resume()`: Resumes a paused capture.
*   `is_paused() -> bool`: Returns whether capture is currently paused.

**Frame Access:**
*   `poll() -> bool`: Polls for a new frame and updates the texture. Call this in `_process()`. Returns `true` if a new frame was received.
*   `get_texture() -> ImageTexture`: Returns the texture containing the latest captured frame.
*   `get_image() -> Image`: Returns the latest captured frame as an Image.
*   `screenshot() -> Image`: Takes a single screenshot and returns it immediately (does not require `start()`).

**Cleanup:**
*   `close()`: Stops capture and releases all resources.

**Signals:**
*   `frame_received`: Emitted when a new frame is available after `poll()`.

**Enums:**
*   `PermissionLevel`: `PERMISSION_OK = 0`, `PERMISSION_WARNING = 1`, `PERMISSION_ERROR = 2`

---

## End-to-End Encryption (E2EE)

> **Note:** E2EE is currently only available on Linux. macOS and Windows builds do not yet include E2EE support in the underlying LiveKit SDK. The classes below will not be registered on those platforms.

### `LiveKitE2eeOptions`
Configuration options for end-to-end encryption. Pass this to the `connect_to_room()` options dictionary under the `"e2ee"` key.

**Methods:**
*   `set_encryption_type(type: int)`: Sets the encryption type (see `EncryptionType` enum).
*   `get_encryption_type() -> int`: Returns the current encryption type.
*   `set_shared_key(key: PackedByteArray)`: Sets the shared encryption key.
*   `get_shared_key() -> PackedByteArray`: Returns the shared encryption key.
*   `set_ratchet_salt(salt: PackedByteArray)`: Sets the ratchet salt for key derivation.
*   `get_ratchet_salt() -> PackedByteArray`: Returns the ratchet salt.
*   `set_ratchet_window_size(size: int)`: Sets the ratchet window size.
*   `get_ratchet_window_size() -> int`: Returns the ratchet window size.
*   `set_failure_tolerance(tolerance: int)`: Sets the number of decryption failures to tolerate before dropping frames.
*   `get_failure_tolerance() -> int`: Returns the failure tolerance.

**Enums:**
*   `EncryptionType`: `ENCRYPTION_NONE = 0`, `ENCRYPTION_GCM = 1`, `ENCRYPTION_CUSTOM = 2`

### `LiveKitKeyProvider`
Manages encryption keys for E2EE. Supports shared keys and per-participant keys with ratcheting.

**Methods:**
*   `set_shared_key(key: PackedByteArray, key_index: int = 0)`: Sets a shared encryption key at the given index.
*   `get_shared_key(key_index: int = 0) -> PackedByteArray`: Returns the shared key at the given index.
*   `ratchet_shared_key(key_index: int = 0) -> PackedByteArray`: Ratchets the shared key and returns the new key.
*   `set_key(participant_identity: String, key: PackedByteArray, key_index: int = 0)`: Sets an encryption key for a specific participant.
*   `get_key(participant_identity: String, key_index: int = 0) -> PackedByteArray`: Returns the key for a specific participant.
*   `ratchet_key(participant_identity: String, key_index: int = 0) -> PackedByteArray`: Ratchets a participant's key and returns the new key.

### `LiveKitFrameCryptor`
Controls encryption for an individual participant's media frames.

**Methods:**
*   `get_participant_identity() -> String`: Returns the participant identity this cryptor is associated with.
*   `get_key_index() -> int`: Returns the current key index.
*   `get_enabled() -> bool`: Returns whether encryption is enabled for this participant.
*   `set_enabled(enabled: bool)`: Enables or disables encryption for this participant.
*   `set_key_index(key_index: int)`: Sets the key index to use for encryption.

### `LiveKitE2eeManager`
Top-level manager for E2EE. Access via `LiveKitRoom.get_e2ee_manager()`.

**Methods:**
*   `get_enabled() -> bool`: Returns whether E2EE is enabled.
*   `set_enabled(enabled: bool)`: Enables or disables E2EE.
*   `get_key_provider() -> LiveKitKeyProvider`: Returns the key provider for managing encryption keys.
*   `get_frame_cryptors() -> Array`: Returns an array of `LiveKitFrameCryptor` objects for all participants.