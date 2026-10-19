extends "res://test/e2e/e2e_helper.gd"
## Web-only e2e tests for room and participant lifecycles. (Natively, these scenarios would use
## participants the SDK has already freed.) Run by test_web.sh when RUN_E2E is set.


func _connected(room: LiveKitRoom) -> Callable:
	return func() -> bool: return room.get_connection_state() == LiveKitRoom.STATE_CONNECTED


func test_local_participant_stays_bound_to_its_connection():
	if _skip_if_no_server():
		return
	_room.connect_to_room(_livekit_url, _token_1, {})
	assert_true(await _poll_until(_room, _connected(_room), 15.0), "first connection")
	var first: LiveKitLocalParticipant = _room.get_local_participant()

	_room.disconnect_from_room()
	_room.connect_to_room(_livekit_url, _token_2, {})
	assert_true(await _poll_until(_room, _connected(_room), 15.0), "second connection")

	assert_eq(
		[first.get_identity(), _room.get_local_participant().get_identity()],
		["e2e-user-1", "e2e-user-2"],
	)
	first.publish_data("stale".to_utf8_buffer())
	assert_push_error("no longer connected")


func test_participant_that_joins_and_leaves_between_polls_is_reported():
	if _skip_if_no_server():
		return
	_room.set_auto_poll(false)
	_room.connect_to_room(_livekit_url, _token_1, {})
	assert_true(await _poll_until(_room, _connected(_room), 15.0), "room connects")
	var seen := []
	var record := func(event: String, p: LiveKitParticipant) -> void:
		if p.get_identity() == "e2e-user-2":
			seen.append([event, p.get_sid() != ""])
	_room.participant_connected.connect(func(p): record.call("connected", p))
	_room.participant_disconnected.connect(func(p): record.call("disconnected", p))

	# Watches for room2's arrival and departure, since _room can't poll without dispatching.
	var observer := LiveKitRoom.new()
	observer.connect_to_room(_livekit_url, _generate_token("e2e-observer", _room_name), {})
	assert_true(await _poll_until(observer, _connected(observer), 15.0), "observer connects")
	var observer_sees_room2 := func() -> bool: return observer.get_remote_participants().has("e2e-user-2")

	_room2.connect_to_room(_livekit_url, _token_2, {})
	assert_true(await _poll_until(observer, observer_sees_room2, 15.0), "room2 joins")
	_room2.disconnect_from_room()
	assert_true(await _poll_until(observer, func(): return not observer_sees_room2.call(), 15.0), "room2 leaves")
	observer.disconnect_from_room()
	# _room is notified at about the same time as the observer.
	await get_tree().create_timer(0.5).timeout
	_room.poll_events()

	assert_eq(seen, [["connected", true], ["disconnected", true]], "room2's join and leave, with its state")


func test_disconnecting_from_a_handler_drops_the_rest_of_the_batch():
	if _skip_if_no_server():
		return
	_room.set_auto_poll(false)
	_room.connect_to_room(_livekit_url, _token_1, {})
	assert_true(await _poll_until(_room, _connected(_room), 15.0), "room connects")

	_room2.connect_to_room(_livekit_url, _token_2, {})
	assert_true(await _poll_until(_room2, _connected(_room2), 15.0), "room2 connects")
	for i in 3:
		_room2.get_local_participant().publish_data(("message %d" % i).to_utf8_buffer())
	await get_tree().create_timer(2.0).timeout

	var received := [0]
	_room.data_received.connect(func(_data, _p, _kind, _topic):
		received[0] += 1
		_room.disconnect_from_room()
	)
	_room.poll_events()

	assert_eq(
		[received[0], _room.get_remote_participants().size(), _room.get_connection_state()],
		[1, 0, LiveKitRoom.STATE_DISCONNECTED],
	)
