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

## Next Steps

Explore the full [API Reference](api/index) for details on all available classes and methods.