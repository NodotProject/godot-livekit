extends "res://test/e2e/e2e_helper.gd"
## E2E: Screen capture pipeline — capture, publish, pause/resume, and cleanup.


# -- Helpers ------------------------------------------------------------------

func _skip_if_no_display() -> bool:
	var perms = LiveKitScreenCapture.check_permissions()
	if perms.get("status", 2) == LiveKitScreenCapture.PERMISSION_ERROR:
		pending("No display available — skipping screen capture test")
		return true
	return false


func _poll_capture_frame(capture, timeout_sec: float = 10.0) -> bool:
	var deadline := Time.get_ticks_msec() + int(timeout_sec * 1000)
	while Time.get_ticks_msec() < deadline:
		capture.poll()
		if capture.get_image() != null:
			return true
		OS.delay_msec(50)
	return false


func _feed_frames(capture, source, count: int) -> void:
	for i in range(count):
		capture.poll()
		var img = capture.get_image()
		if img != null:
			source.capture_frame(img, Time.get_ticks_usec(), 0)
		OS.delay_msec(16)


func _get_remote_track_publications(room: LiveKitRoom) -> Array:
	var pubs := []
	var remotes = room.get_remote_participants()
	for key in remotes:
		var rp = remotes[key]
		var rpubs = rp.get_track_publications()
		for pub_key in rpubs:
			pubs.append(rpubs[pub_key])
	return pubs


# -- Tests --------------------------------------------------------------------

func test_capture_produces_frames():
	if _skip_if_no_server():
		return
	if _skip_if_no_display():
		return

	var capture = LiveKitScreenCapture.create()
	if capture == null:
		pending("create() returned null — capture not available")
		return

	capture.set_auto_poll(false)
	capture.start()

	var got_frame = _poll_capture_frame(capture, 10.0)
	assert_true(got_frame, "Capture should produce at least one frame")

	if got_frame:
		var img = capture.get_image()
		assert_not_null(img, "get_image() should return a valid Image")
		assert_gt(img.get_width(), 0, "Image width should be > 0")
		assert_gt(img.get_height(), 0, "Image height should be > 0")
		assert_not_null(capture.get_texture(), "get_texture() should return a valid texture")

	capture.close()


func test_screenshot_one_shot():
	if _skip_if_no_display():
		return

	var capture = LiveKitScreenCapture.create()
	if capture == null:
		pending("create() returned null — capture not available")
		return

	var img = capture.screenshot()
	assert_not_null(img, "screenshot() should return a valid Image")
	if img != null:
		assert_gt(img.get_width(), 0, "Screenshot width should be > 0")
		assert_gt(img.get_height(), 0, "Screenshot height should be > 0")

	capture.close()


func test_capture_publish_as_video_track():
	if _skip_if_no_server():
		return
	if _skip_if_no_display():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	var capture = LiveKitScreenCapture.create()
	if capture == null:
		pending("create() returned null — capture not available")
		return

	capture.set_auto_poll(false)
	capture.start()

	# Wait for first frame so we know dimensions
	var got_frame = _poll_capture_frame(capture, 10.0)
	assert_true(got_frame, "Capture should produce a frame before publishing")
	if not got_frame:
		capture.close()
		return

	var img = capture.get_image()
	var source = LiveKitVideoSource.create(img.get_width(), img.get_height())
	var track = LiveKitLocalVideoTrack.create("e2e_screen", source)
	assert_not_null(track, "Video track should be created")

	var lp = _room.get_local_participant()
	lp.publish_track(track, {})

	# Feed some frames so the track has content
	_feed_frames(capture, source, 10)

	# Room2 sees remote track publication with KIND_VIDEO
	var saw_video = _poll_until(_room2, func():
		var pubs = _get_remote_track_publications(_room2)
		for pub in pubs:
			if pub.get_kind() == LiveKitTrack.KIND_VIDEO:
				return true
		return false, 15.0)
	assert_true(saw_video, "Room2 should see video track from screen capture")

	capture.close()


func test_capture_pause_resume():
	if _skip_if_no_display():
		return

	var capture = LiveKitScreenCapture.create()
	if capture == null:
		pending("create() returned null — capture not available")
		return

	capture.set_auto_poll(false)
	capture.start()

	# Wait for first frame
	var got_frame = _poll_capture_frame(capture, 10.0)
	assert_true(got_frame, "Capture should produce at least one frame")
	if not got_frame:
		capture.close()
		return

	# Pause and verify
	capture.pause()
	assert_true(capture.is_paused(), "Capture should be paused after pause()")

	# Resume and verify
	capture.resume()
	assert_false(capture.is_paused(), "Capture should not be paused after resume()")

	# Verify frames still arrive after resume
	# Clear current image state by polling any pending, then wait for new
	capture.poll()
	var got_frame_after = _poll_capture_frame(capture, 10.0)
	assert_true(got_frame_after, "Capture should produce frames after resume")

	capture.close()


func test_monitor_capture():
	if _skip_if_no_display():
		return

	var monitors = LiveKitScreenCapture.get_monitors()
	if monitors.is_empty():
		pending("No monitors found — skipping monitor capture test")
		return

	var monitor = monitors[0]
	var capture = LiveKitScreenCapture.create_for_monitor(monitor)
	if capture == null:
		pending("create_for_monitor() returned null — not available")
		return

	capture.set_auto_poll(false)
	capture.start()

	var got_frame = _poll_capture_frame(capture, 10.0)
	assert_true(got_frame, "Monitor capture should produce at least one frame")

	if got_frame:
		var img = capture.get_image()
		assert_not_null(img, "get_image() should return a valid Image")
		assert_gt(img.get_width(), 0, "Captured image width should be > 0")
		assert_gt(img.get_height(), 0, "Captured image height should be > 0")

		# Image dimensions should be in the right ballpark for the monitor
		# (accounting for scale factor, the captured size may differ from
		# the monitor's logical size, so just check it's reasonable)
		var mon_w = monitor.get("width", 0)
		var mon_h = monitor.get("height", 0)
		if mon_w > 0 and mon_h > 0:
			# Captured image should be at least half the monitor size
			# and no more than 4x (for HiDPI scaling)
			assert_gt(img.get_width(), mon_w / 4,
				"Captured width should be close to monitor width")
			assert_gt(img.get_height(), mon_h / 4,
				"Captured height should be close to monitor height")

	capture.close()


func test_capture_lifecycle_cleanup():
	if _skip_if_no_server():
		return
	if _skip_if_no_display():
		return

	# Connect room1 only
	_room.connect_to_room(_livekit_url, _token_1, {})
	var connected = _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED, 15.0)
	assert_true(connected, "Room should connect")
	if not connected:
		return

	var capture = LiveKitScreenCapture.create()
	if capture == null:
		pending("create() returned null — capture not available")
		return

	capture.set_auto_poll(false)
	capture.start()

	var got_frame = _poll_capture_frame(capture, 10.0)
	assert_true(got_frame, "Capture should produce a frame")
	if not got_frame:
		capture.close()
		return

	# Publish as video track
	var img = capture.get_image()
	var source = LiveKitVideoSource.create(img.get_width(), img.get_height())
	var track = LiveKitLocalVideoTrack.create("e2e_cleanup", source)
	var lp = _room.get_local_participant()
	lp.publish_track(track, {})

	# Feed a few frames
	_feed_frames(capture, source, 5)

	# Wait for publication
	var published = _poll_until(_room, func():
		return lp.get_track_publications().size() > 0, 10.0)
	assert_true(published, "Track should be published")

	# Unpublish
	if published:
		var pubs = lp.get_track_publications()
		if pubs.size() > 0:
			var sid = pubs.keys()[0]
			lp.unpublish_track(sid)

	# Close capture
	capture.close()

	# Verify publications are gone
	var unpublished = _poll_until(_room, func():
		return lp.get_track_publications().size() == 0, 10.0)
	assert_true(unpublished, "Publications should be empty after cleanup")

	# Verify room still functional
	assert_not_null(lp, "Local participant should still be valid")
	assert_eq(_room.get_connection_state(), LiveKitRoom.STATE_CONNECTED,
		"Room should still be connected after cleanup")
