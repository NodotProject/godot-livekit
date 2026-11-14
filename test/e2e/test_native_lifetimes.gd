extends "res://test/e2e/e2e_helper.gd"
## E2E: using objects after the native SDK has freed them. (test/web/test_room_lifecycle.gd covers
## the web build, where participants outlive their connection.)


func test_remote_participant_kept_after_it_leaves_is_unbound():
	if _skip_on_web():
		return
	assert_true(await _connect_both_rooms(), "Both rooms should connect")
	var departed := []
	_room.participant_disconnected.connect(func(p): departed.append(p))

	_room2.disconnect_from_room()
	await _poll_until(_room, func(): return not departed.is_empty(), 10.0)
	await _poll_until(_room, func(): return false, 1.0)

	var p: LiveKitRemoteParticipant = departed[0]
	assert_eq([p.get_identity(), p.get_attributes(), p.get_track_publications()], ["", {}, {}])


func test_local_participant_kept_after_disconnecting_is_unbound():
	if _skip_on_web():
		return
	assert_true(await _connect_both_rooms(), "Both rooms should connect")
	var local := _room.get_local_participant()

	_room.disconnect_from_room()
	# Used while the SDK is still tearing the room down, it may fail, but mustn't crash.
	local.publish_data("stale".to_utf8_buffer(), true)
	await get_tree().create_timer(2.0).timeout
	_take_errors()
	local.publish_data("stale".to_utf8_buffer(), true)

	assert_eq(
		[local.get_identity(), local.get_attributes(), local.get_track_publications(), _take_errors()],
		["", {}, {}, ["LiveKitLocalParticipant::publish_data: not bound"]],
	)


func test_e2ee_manager_kept_after_disconnecting_is_unbound():
	if _skip_on_web():
		return
	var options := LiveKitE2eeOptions.new()
	options.shared_key = "key".to_utf8_buffer()
	assert_true(await _connect_both_rooms({"e2ee": options}, {"e2ee": options}), "Both rooms should connect")
	var manager := _room.get_e2ee_manager()
	var key_provider := manager.get_key_provider()

	_room.disconnect_from_room()
	# Used while the SDK is still tearing the room down, they may fail, but mustn't crash.
	key_provider.set_shared_key("new key".to_utf8_buffer(), 0)
	manager.set_enabled(false)
	await get_tree().create_timer(2.0).timeout
	_take_errors()
	key_provider.set_shared_key("new key".to_utf8_buffer(), 0)

	assert_eq(
		[manager.get_enabled(), manager.get_key_provider() == null, _take_errors()],
		[false, true, ["LiveKitKeyProvider::set_shared_key: not bound"]],
	)


func _skip_on_web() -> bool:
	if _skip_if_no_server():
		return true
	if OS.has_feature("web"):
		pending("Native only")
		return true
	return false


## Returns the messages of the errors pushed so far in this test, so that they don't fail it.
func _take_errors() -> Array:
	var messages := []
	for error in get_errors():
		if not error.handled and error.is_push_error():
			error.handled = true
			messages.append(error.code)
	return messages
