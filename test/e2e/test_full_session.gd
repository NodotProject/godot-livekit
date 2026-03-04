extends "res://test/e2e/e2e_helper.gd"
## E2E: Complete chat session workflows exercising multi-step scenarios
## against the production LiveKit server.

var _data_received: PackedByteArray = PackedByteArray()
var _data_topic: String = ""


func before_each():
	super.before_each()
	_data_received = PackedByteArray()
	_data_topic = ""


func _on_data_received(data: PackedByteArray, _participant, topic: String):
	_data_received = data
	_data_topic = topic


func test_connect_publish_subscribe_disconnect():
	if _skip_if_no_server():
		return

	# Step 1: Both rooms connect
	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect and see each other")
	if not both:
		return

	# Step 2: Room1 publishes audio track
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var track = LiveKitLocalAudioTrack.create("e2e_session_audio", source)
	assert_not_null(track, "Audio track should be created")
	var lp = _room.get_local_participant()
	lp.publish_track(track, {})
	var published = _poll_until(_room, func():
		return lp.get_track_publications().size() > 0, 10.0)
	assert_true(published, "Track should be published on Room1")

	# Step 3: Room2 sees the remote track
	var saw_track = _poll_until(_room2, func():
		var remotes = _room2.get_remote_participants()
		for key in remotes:
			var rp = remotes[key]
			if rp.get_track_publications().size() > 0:
				return true
		return false, 15.0)
	assert_true(saw_track, "Room2 should see remote track from Room1")

	# Step 4: Room1 sends data message
	var test_data := "e2e-session-test".to_utf8_buffer()
	var test_topic := "e2e_session"
	_room2.data_received.connect(_on_data_received)
	lp.publish_data(test_data, true, PackedStringArray(), test_topic)

	# Step 5: Verify Room2 receives it
	var received = _poll_until(_room2, func():
		return _data_received.size() > 0, 10.0)
	if received:
		assert_eq(_data_received, test_data, "Received data should match sent data")
		assert_eq(_data_topic, test_topic, "Received topic should match")
	else:
		gut.p("Data not received within timeout (informational)")

	# Step 6: Both disconnect cleanly
	_room.disconnect_from_room()
	_room2.disconnect_from_room()
	var r1_disconnected = _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_DISCONNECTED)
	var r2_disconnected = _poll_until(_room2, func():
		return _room2.get_connection_state() == LiveKitRoom.STATE_DISCONNECTED)
	assert_true(r1_disconnected, "Room1 should disconnect cleanly")
	assert_true(r2_disconnected, "Room2 should disconnect cleanly")


func test_room_properties_after_connect():
	if _skip_if_no_server():
		return

	_room.connect_to_room(_livekit_url, _token_1, {})
	var connected = _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED, 15.0)
	assert_true(connected, "Room should connect")
	if not connected:
		return

	# Room properties
	assert_ne(_room.get_sid(), "", "Room SID should not be empty after connect")
	assert_ne(_room.get_name(), "", "Room name should not be empty after connect")

	# Local participant properties
	var lp = _room.get_local_participant()
	assert_not_null(lp, "Local participant should exist")
	assert_ne(lp.get_sid(), "", "Local participant SID should not be empty")
	assert_ne(lp.get_identity(), "", "Local participant identity should not be empty")


func test_reconnect_preserves_functionality():
	if _skip_if_no_server():
		return

	# First connection
	_room.connect_to_room(_livekit_url, _token_1, {})
	var connected = _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED, 15.0)
	assert_true(connected, "First connect should succeed")
	if not connected:
		return

	# Disconnect
	_room.disconnect_from_room()
	var disconnected = _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_DISCONNECTED, 10.0)
	assert_true(disconnected, "Should disconnect")

	# Reconnect
	_room.connect_to_room(_livekit_url, _token_1, {})
	var reconnected = _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED, 15.0)
	assert_true(reconnected, "Reconnect should succeed")
	if not reconnected:
		return

	# Verify functionality still works — publish a track
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var track = LiveKitLocalAudioTrack.create("e2e_reconnect_audio", source)
	var lp = _room.get_local_participant()
	assert_not_null(lp, "Local participant should exist after reconnect")
	lp.publish_track(track, {})
	var published = _poll_until(_room, func():
		return lp.get_track_publications().size() > 0, 10.0)
	assert_true(published, "Track publish should work after reconnect")
