# Quickstart

This guide will help you get started with the Godot-LiveKit plugin by connecting to a LiveKit room.

## Connecting to a Room

You can access LiveKit classes directly from GDScript once the plugin is installed and enabled.

The core class you will interact with is `LiveKitRoom`.

```gdscript
extends Node

var room: LiveKitRoom

func _ready():
    # 1. Instantiate the room
    room = LiveKitRoom.new()
    
    # 2. Connect to signals to handle room events
    room.connected.connect(_on_room_connected)
    room.disconnected.connect(_on_room_disconnected)
    room.participant_connected.connect(_on_participant_connected)
    
    # 3. Connect to the LiveKit server
    var url = "wss://your-livekit-server.url"
    var token = "your-access-token"
    
    var success = room.connect_to_room(url, token, {})
    if not success:
        print("Failed to initiate connection.")

func _on_room_connected():
    print("Successfully connected to the room!")
    print("Local participant: ", room.get_local_participant().get_identity())

func _on_room_disconnected():
    print("Disconnected from the room.")

func _on_participant_connected(participant):
    print("Participant joined: ", participant.get_identity())
```

## Receiving Video

To display a remote participant's video, use `LiveKitVideoStream`:

```gdscript
extends Node

var room: LiveKitRoom
var video_stream: LiveKitVideoStream

@onready var texture_rect: TextureRect = $TextureRect

func _ready():
    room = LiveKitRoom.new()
    room.track_subscribed.connect(_on_track_subscribed)
    room.connect_to_room("wss://your-livekit-server.url", "your-access-token", {})

func _on_track_subscribed(track, publication, participant):
    if track.get_kind() == LiveKitTrack.KIND_VIDEO:
        video_stream = LiveKitVideoStream.from_track(track)
        texture_rect.texture = video_stream.get_texture()

func _process(_delta):
    if video_stream:
        video_stream.poll()
```

## Publishing Audio

To publish audio from Godot to the room:

```gdscript
var audio_source: LiveKitAudioSource
var audio_track: LiveKitLocalAudioTrack

func publish_microphone():
    audio_source = LiveKitAudioSource.create(48000, 1, 200)
    audio_track = LiveKitLocalAudioTrack.create("microphone", audio_source)
    room.get_local_participant().publish_track(audio_track, {})
```

## Sending Data

To send arbitrary data messages:

```gdscript
func send_message(text: String):
    var data = text.to_utf8_buffer()
    room.get_local_participant().publish_data(data, true, [], "chat")
```

## Remote Procedure Calls (RPC)

To call methods on other participants and handle incoming calls:

```gdscript
func setup_rpc():
    # Register a method to receive RPC calls
    room.get_local_participant().register_rpc_method("greet")
    room.get_local_participant().rpc_method_invoked.connect(_on_rpc_invoked)

    # Connect signals for outgoing RPC results
    room.get_local_participant().rpc_response_received.connect(_on_rpc_response)
    room.get_local_participant().rpc_error.connect(_on_rpc_error)

func _on_rpc_invoked(method, request_id, caller_identity, payload, response_timeout):
    print("RPC from ", caller_identity, ": ", method, " -> ", payload)
    # Respond to the caller
    room.get_local_participant().respond_to_rpc(request_id, "Hello back!")

func call_remote_greet(target_identity: String):
    # perform_rpc is async — the result arrives via rpc_response_received signal
    room.get_local_participant().perform_rpc(target_identity, "greet", "Hello!", 10.0)

func _on_rpc_response(method, result):
    print("RPC response for ", method, ": ", result)

func _on_rpc_error(method, error_message):
    print("RPC error for ", method, ": ", error_message)
```

## End-to-End Encryption (E2EE)

> **Note:** E2EE is currently only available on Linux. macOS and Windows builds do not yet include E2EE support in the underlying LiveKit SDK.

To enable E2EE when connecting to a room:

```gdscript
func connect_with_e2ee():
    var e2ee_options = LiveKitE2eeOptions.new()
    e2ee_options.set_encryption_type(LiveKitE2eeOptions.ENCRYPTION_GCM)
    e2ee_options.set_shared_key("my-secret-key".to_utf8_buffer())

    room = LiveKitRoom.new()
    room.e2ee_state_changed.connect(_on_e2ee_state_changed)
    room.connect_to_room("wss://your-server.url", "your-token", {"e2ee": e2ee_options})

func _on_e2ee_state_changed(participant, state):
    print("E2EE state changed for ", participant.get_identity(), ": ", state)

func rotate_encryption_key():
    # Ratchet (rotate) the shared key
    var manager = room.get_e2ee_manager()
    var new_key = manager.get_key_provider().ratchet_shared_key()
    print("Key rotated")
```

## Track Statistics

To monitor WebRTC connection quality:

```gdscript
func _on_track_subscribed(track, publication, participant):
    # Connect the signal to receive stats asynchronously
    track.stats_received.connect(_on_stats_received)
    # Request stats (non-blocking)
    track.request_stats()

func _on_stats_received(stats):
    for stat in stats:
        print(stat)
```

## Next Steps

Explore the full [API Reference](api/index) for details on all available classes and methods.