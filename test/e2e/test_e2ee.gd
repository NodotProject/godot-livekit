extends "res://test/e2e/e2e_helper.gd"
## E2E: end-to-end encrypted media and data reach only rooms with the right key.

const WIDTH := 320
const HEIGHT := 240
const KEY := "correct horse battery staple"


func test_encrypted_media_and_data_reach_room_with_same_key():
	if _skip_if_unsupported():
		return
	assert_true(await _connect_both_rooms(_e2ee_options(KEY), _e2ee_options(KEY)), "Both rooms should connect")
	var statuses := {}
	_room2.participant_encryption_status_changed.connect(func(participant, is_encrypted):
		statuses[participant.get_identity()] = is_encrypted
	)
	var received := []
	_room2.data_received.connect(func(data, _participant, _kind, _topic): received.append(data.get_string_from_utf8()))

	var video := _publish_red_video()
	_room.get_local_participant().publish_data("secret".to_utf8_buffer(), true)
	var frames := await _count_red_frames(video, 10, 15.0)

	var manager: LiveKitE2eeManager = _room.get_e2ee_manager()
	assert_eq(
		[frames >= 10, received, statuses.get("e2e-user-1"), manager.get_enabled(), manager.get_key_provider().get_shared_key()],
		[true, ["secret"], true, true, KEY.to_utf8_buffer()],
	)
	_close(video)


func test_room_with_a_different_key_cannot_decode():
	if _skip_if_unsupported():
		return
	assert_true(await _connect_both_rooms(_e2ee_options(KEY), _e2ee_options("wrong key")), "Both rooms should connect")
	var received := []
	_room2.data_received.connect(func(data, _participant, _kind, _topic): received.append(data))

	var video := _publish_red_video()
	_room.get_local_participant().publish_data("secret".to_utf8_buffer(), true)
	var frames := await _count_red_frames(video, 1, 8.0)

	assert_eq([video.subscribed, frames, received], [true, 0, []])
	_close(video)


func test_rotated_key_is_used_once_switched_to():
	if _skip_if_unsupported():
		return
	assert_true(await _connect_both_rooms(_e2ee_options(KEY), _e2ee_options(KEY)), "Both rooms should connect")
	var video := _publish_red_video()
	assert_true(await _count_red_frames(video, 5, 15.0) >= 5, "Room2 should decode the first key's frames")

	# Rotate to a key that room2 doesn't have yet. Room1's frames stop decoding once it switches...
	var next_key := "next key".to_utf8_buffer()
	var key_provider := _room.get_e2ee_manager().get_key_provider()
	key_provider.set_shared_key(next_key, 1)
	assert_eq(key_provider.get_shared_key(1), next_key)
	var cryptors := _room.get_e2ee_manager().get_frame_cryptors().filter(func(cryptor):
		return cryptor.get_participant_identity() == "e2e-user-1"
	)
	assert_false(cryptors.is_empty(), "Room1 should have a frame cryptor for its track")
	for cryptor in cryptors:
		cryptor.set_key_index(1)
	# (Frames encrypted before the switch may still be in flight.)
	await _count_red_frames(video, 1000, 1.0)
	var frames_before_room2_has_key := await _count_red_frames(video, 1, 3.0)

	# ...and resume once room2 has it too.
	_room2.get_e2ee_manager().get_key_provider().set_shared_key(next_key, 1)
	var frames_after := await _count_red_frames(video, 5, 10.0)

	assert_eq([frames_before_room2_has_key, frames_after >= 5], [0, true])
	_close(video)


func test_receiver_follows_a_ratcheted_key():
	if _skip_if_unsupported():
		return
	assert_true(await _connect_both_rooms(_e2ee_options(KEY), _e2ee_options(KEY)), "Both rooms should connect")
	var video := _publish_red_video()
	assert_true(await _count_red_frames(video, 5, 15.0) >= 5, "Room2 should decode the first key's frames")

	# Room2 isn't told, but tries ratcheting its own key when frames stop decrypting.
	var key_provider := _room.get_e2ee_manager().get_key_provider()
	var ratcheted := key_provider.ratchet_shared_key()
	var deadline := Time.get_ticks_msec() + 5000
	while key_provider.get_shared_key() == KEY.to_utf8_buffer() and Time.get_ticks_msec() < deadline:
		await get_tree().process_frame
	var new_key := key_provider.get_shared_key()
	var frames_after := await _count_red_frames(video, 5, 10.0)

	# The ratcheted key is returned natively, but on the web it's only available once ready.
	assert_eq(
		[new_key.size(), new_key != KEY.to_utf8_buffer(), ratcheted.is_empty() or ratcheted == new_key, frames_after >= 5],
		[32, true, true, true],
	)
	_close(video)


func _skip_if_unsupported() -> bool:
	if _skip_if_no_server():
		return true
	if not OS.has_feature("web"):
		# client-sdk-cpp v0.3.1 builds its E2EEManager from a room handle that isn't set yet.
		pending("Native E2EE crashes on connect with client-sdk-cpp v0.3.1")
		return true
	return false


func _e2ee_options(key: String) -> Dictionary:
	var options := LiveKitE2eeOptions.new()
	options.shared_key = key.to_utf8_buffer()
	return {"e2ee": options}


## Publishes red video from room1 and starts listening for it in room2. Returns the state that
## _count_red_frames() updates.
func _publish_red_video() -> Dictionary:
	var source := LiveKitVideoSource.create(WIDTH, HEIGHT)
	var image := Image.create_empty(WIDTH, HEIGHT, false, Image.FORMAT_RGBA8)
	image.fill(Color(1.0, 0.0, 0.0))
	_room.get_local_participant().publish_track(LiveKitLocalVideoTrack.create("e2ee_video", source), {"source": LiveKitTrack.SOURCE_CAMERA})
	var video := {"source": source, "image": image, "stream": null, "subscribed": false, "red_frames": 0}
	_room2.track_subscribed.connect(func(remote_track, _publication, _participant):
		if remote_track.get_kind() == LiveKitTrack.KIND_VIDEO:
			video.subscribed = true
			video.stream = LiveKitVideoStream.from_track(remote_track)
			video.stream.frame_received.connect(func():
				var texture: ImageTexture = video.stream.get_texture()
				var received := texture.get_image() if texture else null
				if received and received.get_width() > 0:
					var center := received.get_pixel(received.get_width() / 2, received.get_height() / 2)
					if center.r > 0.7 and center.g < 0.3 and center.b < 0.3:
						video.red_frames += 1
			)
	)
	return video


## Keeps capturing video until room2 has received count more red frames, or timeout_sec has
## passed, and returns how many it received.
func _count_red_frames(video: Dictionary, count: int, timeout_sec: float) -> int:
	var initial: int = video.red_frames
	var start := Time.get_ticks_msec()
	var last_capture := -1000
	while Time.get_ticks_msec() - start < timeout_sec * 1000 and video.red_frames - initial < count:
		if Time.get_ticks_msec() - last_capture >= 33:
			video.source.capture_frame(video.image)
			last_capture = Time.get_ticks_msec()
		_room.poll_events()
		_room2.poll_events()
		await get_tree().process_frame
	return video.red_frames - initial


func _close(video: Dictionary) -> void:
	if video.stream:
		video.stream.close()
	# The frame_received handler captures video, so this breaks their cycle.
	video.stream = null
