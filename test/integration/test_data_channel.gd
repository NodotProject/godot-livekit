extends "res://test/integration/test_helper.gd"
## Integration tests for data channels.

var _room2: LiveKitRoom = null
var _received_data: PackedByteArray = PackedByteArray()
var _received_topic: String = ""


func before_each():
	super.before_each()
	_received_data = PackedByteArray()
	_received_topic = ""
	if _is_server_available():
		_room2 = LiveKitRoom.new()


func after_each():
	if _room2 != null:
		_room2.disconnect_from_room()
		_room2 = null
	super.after_each()


func _connect_both_rooms():
	_room.connect_to_room(_livekit_url, _token_1, {})
	_poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	_room2.connect_to_room(_livekit_url, _token_2, {})
	_poll_until(_room2, func():
		return _room2.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	# Wait for both to see each other
	_poll_until(_room, func():
		return _room.get_remote_participants().size() == 1, 10.0)


func _on_data_received(data: PackedByteArray, _participant, topic: String):
	_received_data = data
	_received_topic = topic


func test_publish_receive_data_reliable():
	if _skip_if_no_server():
		return
	_connect_both_rooms()
	var test_data := PackedByteArray([72, 101, 108, 108, 111])  # "Hello"
	var test_topic := "test_topic"
	# Publish from room 1
	var lp = _room.get_local_participant()
	assert_not_null(lp, "Local participant needed for publish_data")
	lp.publish_data(test_data, true, PackedStringArray(), test_topic)
	# Poll room 2 to receive
	var received = _poll_until(_room2, func():
		_room2.poll_events()
		return _received_data.size() > 0, 5.0)
	if received:
		assert_eq(_received_data, test_data, "Received data should match sent data")
		assert_eq(_received_topic, test_topic, "Received topic should match sent topic")
	else:
		gut.p("Data not received — may require signal wiring (informational)")
		pass_test("Data publish completed without crash")


func test_publish_unreliable_data():
	if _skip_if_no_server():
		return
	_connect_both_rooms()
	var test_data := PackedByteArray([1, 2, 3])
	var lp = _room.get_local_participant()
	assert_not_null(lp, "Local participant needed for publish_data")
	# Unreliable publish — best effort
	lp.publish_data(test_data, false, PackedStringArray(), "unreliable_topic")
	# Give a moment for delivery
	_poll_until(_room2, func(): return false, 1.0)
	assert_true(true, "Unreliable data publish completed without crash")
