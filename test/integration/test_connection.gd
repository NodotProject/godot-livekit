extends "res://test/integration/test_helper.gd"
## Integration tests for room connection lifecycle.


func test_connect_emits_connected():
	if _skip_if_no_server():
		return
	_room.connect_to_room(_livekit_url, _token_1, {})
	var connected = await _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	assert_true(connected, "Room should reach STATE_CONNECTED")
	assert_not_null(_room.get_local_participant(),
		"Local participant should not be null after connecting")
	assert_ne(_room.get_sid(), "",
		"Room SID should not be empty after connecting")


func test_invalid_token_emits_connection_failed():
	if _skip_if_no_server():
		return
	# An Array so the lambda can mutate shared state (lambdas capture locals by value).
	var state := [false]  # [failed]
	_room.connection_failed.connect(func(_err: String):
		state[0] = true
	)
	_room.connect_to_room(_livekit_url, "clearly-invalid-token", {})
	var failed = await _poll_until(_room, func(): return state[0], 10.0)
	assert_true(failed, "connection_failed signal should fire with invalid token")
	assert_eq(_room.get_connection_state(), LiveKitRoom.STATE_DISCONNECTED,
		"Room should be STATE_DISCONNECTED after connection_failed")


func test_disconnect_after_connect():
	if _skip_if_no_server():
		return
	_room.connect_to_room(_livekit_url, _token_1, {})
	await _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	_room.disconnect_from_room()
	var disconnected = await _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_DISCONNECTED)
	assert_true(disconnected,
		"Room should return to STATE_DISCONNECTED after disconnect")
	assert_null(_room.get_local_participant(),
		"Local participant should be null after disconnect")


func test_reconnect_after_disconnect():
	if _skip_if_no_server():
		return
	_room.connect_to_room(_livekit_url, _token_1, {})
	await _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	_room.disconnect_from_room()
	await _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_DISCONNECTED)
	# Second connect should succeed
	_room.connect_to_room(_livekit_url, _token_1, {})
	var reconnected = await _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	assert_true(reconnected, "Second connect should succeed")
