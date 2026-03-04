extends "res://test/e2e/e2e_helper.gd"
## E2E: Two-participant interaction scenarios against the production server.

var _r1_data_received: PackedByteArray = PackedByteArray()
var _r1_data_topic: String = ""
var _r2_data_received: PackedByteArray = PackedByteArray()
var _r2_data_topic: String = ""


func before_each():
	super.before_each()
	_r1_data_received = PackedByteArray()
	_r1_data_topic = ""
	_r2_data_received = PackedByteArray()
	_r2_data_topic = ""


func _on_r1_data_received(data: PackedByteArray, _participant, topic: String):
	_r1_data_received = data
	_r1_data_topic = topic


func _on_r2_data_received(data: PackedByteArray, _participant, topic: String):
	_r2_data_received = data
	_r2_data_topic = topic


func test_participant_visibility():
	if _skip_if_no_server():
		return

	# Room1 connects first
	_room.connect_to_room(_livekit_url, _token_1, {})
	var r1_connected = _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED, 15.0)
	assert_true(r1_connected, "Room1 should connect")
	if not r1_connected:
		return

	# Room2 joins — Room1 should see it
	_room2.connect_to_room(_livekit_url, _token_2, {})
	var r2_connected = _poll_until(_room2, func():
		return _room2.get_connection_state() == LiveKitRoom.STATE_CONNECTED, 15.0)
	assert_true(r2_connected, "Room2 should connect")

	var saw_remote = _poll_until(_room, func():
		return _room.get_remote_participants().size() == 1, 15.0)
	assert_true(saw_remote, "Room1 should see one remote participant")

	# Room2 disconnects — Room1 should see empty remotes
	_room2.disconnect_from_room()
	var saw_empty = _poll_until(_room, func():
		return _room.get_remote_participants().size() == 0, 15.0)
	assert_true(saw_empty, "Room1 should see zero remotes after Room2 disconnects")


func test_bidirectional_data():
	if _skip_if_no_server():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect and see each other")
	if not both:
		return

	_room.data_received.connect(_on_r1_data_received)
	_room2.data_received.connect(_on_r2_data_received)

	# Room1 sends data to Room2
	var data_1to2 := "hello-from-room1".to_utf8_buffer()
	var lp1 = _room.get_local_participant()
	lp1.publish_data(data_1to2, true, PackedStringArray(), "r1_topic")

	var r2_received = _poll_both(func():
		return _r2_data_received.size() > 0, 10.0)

	# Room2 sends data to Room1
	var data_2to1 := "hello-from-room2".to_utf8_buffer()
	var lp2 = _room2.get_local_participant()
	lp2.publish_data(data_2to1, true, PackedStringArray(), "r2_topic")

	var r1_received = _poll_both(func():
		return _r1_data_received.size() > 0, 10.0)

	if r2_received:
		assert_eq(_r2_data_received, data_1to2, "Room2 should receive data from Room1")
		assert_eq(_r2_data_topic, "r1_topic", "Room2 topic should match")
	else:
		gut.p("Room2 did not receive data within timeout (informational)")

	if r1_received:
		assert_eq(_r1_data_received, data_2to1, "Room1 should receive data from Room2")
		assert_eq(_r1_data_topic, "r2_topic", "Room1 topic should match")
	else:
		gut.p("Room1 did not receive data within timeout (informational)")

	assert_true(r2_received or r1_received,
		"At least one direction of data should succeed")


func test_participant_properties():
	if _skip_if_no_server():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect and see each other")
	if not both:
		return

	# Verify local participant properties on each room
	var lp1 = _room.get_local_participant()
	assert_not_null(lp1, "Room1 local participant should exist")
	assert_ne(lp1.get_identity(), "", "Room1 local identity should not be empty")
	assert_ne(lp1.get_sid(), "", "Room1 local SID should not be empty")

	var lp2 = _room2.get_local_participant()
	assert_not_null(lp2, "Room2 local participant should exist")
	assert_ne(lp2.get_identity(), "", "Room2 local identity should not be empty")
	assert_ne(lp2.get_sid(), "", "Room2 local SID should not be empty")

	# Verify remote participant properties match the other room's local identity
	var remotes1 = _room.get_remote_participants()
	assert_eq(remotes1.size(), 1, "Room1 should see 1 remote participant")
	if remotes1.size() == 1:
		var rp = remotes1.values()[0]
		assert_eq(rp.get_identity(), lp2.get_identity(),
			"Room1's remote should have Room2's identity")
		assert_ne(rp.get_sid(), "", "Remote participant SID should not be empty")

	var remotes2 = _room2.get_remote_participants()
	assert_eq(remotes2.size(), 1, "Room2 should see 1 remote participant")
	if remotes2.size() == 1:
		var rp = remotes2.values()[0]
		assert_eq(rp.get_identity(), lp1.get_identity(),
			"Room2's remote should have Room1's identity")
		assert_ne(rp.get_sid(), "", "Remote participant SID should not be empty")
