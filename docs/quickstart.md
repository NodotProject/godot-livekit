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

## Next Steps

Explore the full [API Reference](api/index) for details on the available nodes and methods, including how to handle video/audio streams and data channels.