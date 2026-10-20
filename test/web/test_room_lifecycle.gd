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


func test_local_participant_is_stale_once_the_server_disconnects_it():
	if _skip_if_no_server():
		return
	_room.connect_to_room(_livekit_url, _token_1, {})
	assert_true(await _poll_until(_room, _connected(_room), 15.0), "room connects")
	var local: LiveKitLocalParticipant = _room.get_local_participant()
	var disconnected := [false]
	_room.disconnected.connect(func(): disconnected[0] = true)

	assert_eq(await _remove_participant("e2e-user-1"), 200, "server removes the participant")
	assert_true(await _poll_until(_room, func(): return disconnected[0], 15.0), "room reports the disconnect")

	local.publish_data("stale".to_utf8_buffer())
	assert_push_error("no longer connected")


## Has the server remove a participant from the room, returning the HTTP status.
func _remove_participant(identity: String) -> int:
	var now := int(Time.get_unix_time_from_system())
	var claims := {
		"iss": _api_key, "sub": "e2e-admin", "iat": now, "nbf": now, "exp": now + 600,
		"video": {"room": _room_name, "roomAdmin": true},
	}
	var signing_input := (
		_base64url_encode(JSON.stringify({"alg": "HS256", "typ": "JWT"}).to_utf8_buffer())
		+ "." + _base64url_encode(JSON.stringify(claims).to_utf8_buffer())
	)
	var signature := Crypto.new().hmac_digest(
		HashingContext.HASH_SHA256, _api_secret.to_utf8_buffer(), signing_input.to_utf8_buffer()
	)
	var http := HTTPRequest.new()
	add_child_autofree(http)
	http.request(
		_livekit_url.replace("ws://", "http://").replace("wss://", "https://") + "/twirp/livekit.RoomService/RemoveParticipant",
		["Authorization: Bearer " + signing_input + "." + _base64url_encode(signature), "Content-Type: application/json"],
		HTTPClient.METHOD_POST,
		JSON.stringify({"room": _room_name, "identity": identity}),
	)
	var response: Array = await http.request_completed
	return response[1]


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


func test_audio_stream_rejects_a_participant_from_an_earlier_connection():
	if _skip_if_no_server():
		return
	_room.connect_to_room(_livekit_url, _token_1, {})
	_room2.connect_to_room(_livekit_url, _token_2, {})
	var sees_room2 := func() -> bool: return _room.get_remote_participants().has("e2e-user-2")
	assert_true(await _poll_until(_room, sees_room2, 15.0), "room sees room2")
	var earlier: LiveKitRemoteParticipant = _room.get_remote_participants()["e2e-user-2"]

	_room.disconnect_from_room()
	_room.connect_to_room(_livekit_url, _token_1, {})
	assert_true(await _poll_until(_room, sees_room2, 15.0), "room sees room2 again")

	assert_null(LiveKitAudioStream.from_participant(earlier, LiveKitTrack.SOURCE_MICROPHONE))
	assert_push_error("no longer connected")
