extends "res://test/integration/test_helper.gd"
## Integration tests for participant lifecycle.
## Uses two rooms with different identities.

var _room2: LiveKitRoom = null


func before_each():
	super.before_each()
	if _is_server_available():
		_room2 = LiveKitRoom.new()


func after_each():
	if _room2 != null:
		_room2.disconnect_from_room()
		_room2 = null
	super.after_each()


func test_participant_connected_signal():
	if _skip_if_no_server():
		return
	# Connect first room
	_room.connect_to_room(_livekit_url, _token_1, {})
	_poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	# Connect second room
	_room2.connect_to_room(_livekit_url, _token_2, {})
	_poll_until(_room2, func():
		return _room2.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	# Wait for first room to see the second participant
	var saw_remote = _poll_until(_room, func():
		return _room.get_remote_participants().size() == 1, 10.0)
	assert_true(saw_remote,
		"First room should see one remote participant")


func test_participant_disconnected_signal():
	if _skip_if_no_server():
		return
	# Connect both rooms
	_room.connect_to_room(_livekit_url, _token_1, {})
	_poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	_room2.connect_to_room(_livekit_url, _token_2, {})
	_poll_until(_room2, func():
		return _room2.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	_poll_until(_room, func():
		return _room.get_remote_participants().size() == 1, 10.0)
	# Disconnect second room
	_room2.disconnect_from_room()
	# Wait for first room to see participant gone
	var saw_empty = _poll_until(_room, func():
		return _room.get_remote_participants().size() == 0, 10.0)
	assert_true(saw_empty,
		"First room should see zero remote participants after second disconnects")


func test_local_participant_properties():
	if _skip_if_no_server():
		return
	_room.connect_to_room(_livekit_url, _token_1, {})
	_poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	var lp = _room.get_local_participant()
	assert_not_null(lp, "Local participant should not be null")
	assert_ne(lp.get_sid(), "", "Local participant SID should not be empty")
	assert_ne(lp.get_identity(), "", "Local participant identity should not be empty")
