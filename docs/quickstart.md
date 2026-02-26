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

## Screen Capture

To capture a screen or window and publish it as a video track:

```gdscript
var screen_capture: LiveKitScreenCapture
var video_source: LiveKitVideoSource
var video_track: LiveKitLocalVideoTrack

func start_screen_share():
    # Check permissions first (important on macOS)
    var perms = LiveKitScreenCapture.check_permissions()
    if perms["status"] == LiveKitScreenCapture.PERMISSION_ERROR:
        print("Screen capture not permitted: ", perms["summary"])
        return

    # Create capture for the primary monitor
    screen_capture = LiveKitScreenCapture.create()
    screen_capture.start()

    # Create a video source and track to publish the captured frames
    video_source = LiveKitVideoSource.create(1920, 1080)
    video_track = LiveKitLocalVideoTrack.create("screen", video_source)
    room.get_local_participant().publish_track(video_track, {})

func _process(_delta):
    if screen_capture and screen_capture.poll():
        var image = screen_capture.get_image()
        if image:
            video_source.capture_frame(image, Time.get_ticks_usec(), 0)
```

You can also capture a specific window:

```gdscript
func share_specific_window():
    var windows = LiveKitScreenCapture.get_windows()
    for w in windows:
        if "Firefox" in w["name"]:
            screen_capture = LiveKitScreenCapture.create_for_window(w)
            screen_capture.start()
            break
```

Or take a one-shot screenshot without starting continuous capture:

```gdscript
func take_screenshot():
    var capture = LiveKitScreenCapture.create()
    var image = capture.screenshot()
    if image:
        image.save_png("res://screenshot.png")
    capture.close()
```

## End-to-End Encryption (E2EE)

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

## Connection Options

The `connect_to_room()` method accepts an options dictionary with the following keys:

| Key | Type | Default | Description |
|-----|------|---------|-------------|
| `auto_subscribe` | `bool` | `true` | Automatically subscribe to tracks published by remote participants. |
| `dynacast` | `bool` | `false` | Enable dynacast for adaptive simulcast. |
| `auto_reconnect` | `bool` | `true` | Allow the SDK to automatically reconnect on network disruption. When `false`, a `disconnected` signal is emitted instead, letting your application handle reconnection. |
| `e2ee` | `LiveKitE2eeOptions` | — | End-to-end encryption options. |

```gdscript
# Example: disable auto-reconnect so you can handle reconnection yourself
room.connect_to_room(url, token, {"auto_reconnect": false, "dynacast": true})
```

## Next Steps

Explore the full [API Reference](api/index) for details on all available classes and methods.