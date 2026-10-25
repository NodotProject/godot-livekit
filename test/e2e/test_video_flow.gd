extends "res://test/e2e/e2e_helper.gd"
## E2E: video captured by one room is received, as frames in a texture, in another.

const WIDTH := 320
const HEIGHT := 240


func test_published_video_reaches_remote_room():
	if _skip_if_no_server():
		return
	assert_true(await _connect_both_rooms(), "Both rooms should connect")

	var source = LiveKitVideoSource.create(WIDTH, HEIGHT)
	var track = LiveKitLocalVideoTrack.create("e2e_video", source)
	_room.get_local_participant().publish_track(track, {"source": LiveKitTrack.SOURCE_CAMERA})
	var state := {"stream": null, "frames": 0}
	_room2.track_subscribed.connect(func(remote_track, _publication, _participant):
		if remote_track.get_kind() == LiveKitTrack.KIND_VIDEO:
			state.stream = LiveKitVideoStream.from_track(remote_track)
			state.stream.frame_received.connect(func(): state.frames += 1)
	)

	# A solid color survives encoding well enough to check.
	var image := Image.create_empty(WIDTH, HEIGHT, false, Image.FORMAT_RGBA8)
	image.fill(Color(1.0, 0.0, 0.0))
	var start := Time.get_ticks_msec()
	var last_capture := -1000
	while Time.get_ticks_msec() - start < 15000 and state.frames < 10:
		if Time.get_ticks_msec() - last_capture >= 33:
			source.capture_frame(image)
			last_capture = Time.get_ticks_msec()
		_room.poll_events()
		_room2.poll_events()
		await get_tree().process_frame

	assert_not_null(state.stream, "Room2 should subscribe to the video track")
	if state.stream == null:
		return
	var texture: ImageTexture = state.stream.get_texture()
	var received := texture.get_image() if texture else null
	var center: Color = received.get_pixel(received.get_width() / 2, received.get_height() / 2) if received else Color()
	assert_eq(
		[state.frames >= 10, received != null and received.get_width() > 0, center.r > 0.7, center.g < 0.3, center.b < 0.3],
		[true, true, true, true, true],
		"Room2 should receive red frames (got %d frames, center %s)" % [state.frames, center],
	)
	state.stream.close()
	# The frame_received handler captures state, so this breaks their cycle.
	state.stream = null
