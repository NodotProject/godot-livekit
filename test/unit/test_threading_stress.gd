extends GutTest
## Threading stress tests that exercise concurrent operation patterns which
## have historically caused crashes.  All tests use a 500ms frame budget to
## detect main-thread blocking, and target the boundaries between background
## threads (SDK callbacks, reader threads, detached RPC/stats threads) and the
## Godot main thread.

const FRAME_BUDGET_MS := 500


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

## Access every Room property that touches room_info() / participants_mutex_.
func _access_all_room_properties(room: LiveKitRoom) -> void:
	room.get_sid()
	room.get_name()
	room.get_metadata()
	room.get_connection_state()
	room.get_local_participant()
	room.get_remote_participants()


# ---------------------------------------------------------------------------
# 1. Room property access during async connect
# ---------------------------------------------------------------------------

func test_property_access_during_connect():
	var room := LiveKitRoom.new()
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	var before := Time.get_ticks_msec()
	for i in range(10):
		_access_all_room_properties(room)
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"Property access during connect should not block (took %d ms)" % elapsed)
	room.disconnect_from_room()


# ---------------------------------------------------------------------------
# 2. Room property access during disconnect
# ---------------------------------------------------------------------------

func test_property_access_after_disconnect():
	var room := LiveKitRoom.new()
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	room.disconnect_from_room()
	var before := Time.get_ticks_msec()
	for i in range(10):
		_access_all_room_properties(room)
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"Property access after disconnect should not block (took %d ms)" % elapsed)


# ---------------------------------------------------------------------------
# 3. Interleaved poll_events and disconnect
# ---------------------------------------------------------------------------

func test_interleaved_poll_and_disconnect():
	var room := LiveKitRoom.new()
	var before := Time.get_ticks_msec()

	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	room.poll_events()
	room.disconnect_from_room()
	room.poll_events()

	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	room.poll_events()
	room.disconnect_from_room()
	room.poll_events()

	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"Interleaved poll/disconnect cycles should not deadlock (took %d ms)" % elapsed)


# ---------------------------------------------------------------------------
# 4. Rapid connect_to_room reentrance (no disconnect between connects)
# ---------------------------------------------------------------------------

func test_rapid_connect_reentrance():
	var room := LiveKitRoom.new()
	var before := Time.get_ticks_msec()
	# Second connect without disconnect — exercises connect_thread_.join()
	# and disconnect_thread_.detach() paths.
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"Double connect without disconnect should not block (took %d ms)" % elapsed)
	room.disconnect_from_room()


# ---------------------------------------------------------------------------
# 5. Room free with pending events (poll never called)
# ---------------------------------------------------------------------------

func test_room_free_without_poll():
	var before := Time.get_ticks_msec()
	for i in range(10):
		var room := LiveKitRoom.new()
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		# Never poll — pending_events_ may be non-empty, connect_thread_ running
		room = null
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"Freeing rooms without polling should not crash (took %d ms)" % elapsed)


# ---------------------------------------------------------------------------
# 6. Massive event queue build-up then drain
# ---------------------------------------------------------------------------

func test_event_queue_buildup_then_drain():
	var rooms := []
	var before := Time.get_ticks_msec()
	for i in range(10):
		var room := LiveKitRoom.new()
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		rooms.append(room)
	# Let events accumulate briefly
	OS.delay_msec(50)
	# Drain all at once
	for room in rooms:
		room.poll_events()
	for room in rooms:
		room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS + 100,  # +100 for the intentional 50ms delay
		"Draining many event queues should not crash (took %d ms)" % elapsed)
	rooms.clear()


# ---------------------------------------------------------------------------
# 7. VideoStream double close
# ---------------------------------------------------------------------------

func test_video_stream_double_close():
	var stream := LiveKitVideoStream.new()
	stream.close()
	stream.close()
	assert_true(true, "VideoStream double close should not crash")


# ---------------------------------------------------------------------------
# 8. AudioStream double close
# ---------------------------------------------------------------------------

func test_audio_stream_double_close():
	var stream := LiveKitAudioStream.new()
	stream.close()
	stream.close()
	assert_true(true, "AudioStream double close should not crash")


# ---------------------------------------------------------------------------
# 9. VideoStream poll after close
# ---------------------------------------------------------------------------

func test_video_stream_poll_after_close():
	var stream := LiveKitVideoStream.new()
	stream.close()
	var result = stream.poll()
	assert_false(result,
		"poll() after close should return false without crash")


# ---------------------------------------------------------------------------
# 10. AudioStream poll after close
# ---------------------------------------------------------------------------

func test_audio_stream_poll_after_close():
	var stream := LiveKitAudioStream.new()
	stream.close()
	# poll() is not exposed on AudioStream — calling close + property access
	# exercises the same null-playback path
	assert_eq(stream.get_sample_rate(), 48000,
		"get_sample_rate() after close should return default")
	assert_eq(stream.get_num_channels(), 1,
		"get_num_channels() after close should return default")


# ---------------------------------------------------------------------------
# 11. ScreenCapture start/stop rapid cycles (display-dependent)
# ---------------------------------------------------------------------------

func test_screen_capture_rapid_start_stop():
	var perms = LiveKitScreenCapture.check_permissions()
	if perms.get("status", 2) == LiveKitScreenCapture.PERMISSION_ERROR:
		pending("No display available — skipping")
		return
	var capture = LiveKitScreenCapture.create()
	if capture == null:
		pending("create() returned null — capture not available")
		return
	var before := Time.get_ticks_msec()
	for i in range(5):
		capture.start()
		capture.stop()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"Rapid start/stop should not crash (took %d ms)" % elapsed)
	capture.close()


# ---------------------------------------------------------------------------
# 12. ScreenCapture close during active capture (display-dependent)
# ---------------------------------------------------------------------------

func test_screen_capture_close_during_capture():
	var perms = LiveKitScreenCapture.check_permissions()
	if perms.get("status", 2) == LiveKitScreenCapture.PERMISSION_ERROR:
		pending("No display available — skipping")
		return
	var capture = LiveKitScreenCapture.create()
	if capture == null:
		pending("create() returned null — capture not available")
		return
	capture.start()
	# Close immediately without stop — close() should call stop() internally
	var before := Time.get_ticks_msec()
	capture.close()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"Close during active capture should not block (took %d ms)" % elapsed)


# ---------------------------------------------------------------------------
# 13. ScreenCapture double close
# ---------------------------------------------------------------------------

func test_screen_capture_double_close():
	var capture := LiveKitScreenCapture.new()
	capture.close()
	capture.close()
	assert_true(true, "ScreenCapture double close should not crash")


# ---------------------------------------------------------------------------
# 14. ScreenCapture poll after close
# ---------------------------------------------------------------------------

func test_screen_capture_poll_after_close():
	var capture := LiveKitScreenCapture.new()
	capture.close()
	var result = capture.poll()
	assert_false(result,
		"poll() after close should return false without crash")


# ---------------------------------------------------------------------------
# 15. Room connect/disconnect interleaved with property access stress
# ---------------------------------------------------------------------------

func test_connect_disconnect_property_stress():
	var room := LiveKitRoom.new()
	var before := Time.get_ticks_msec()
	for i in range(20):
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		_access_all_room_properties(room)
		room.disconnect_from_room()
		_access_all_room_properties(room)
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"20 connect/disconnect + property access cycles should complete in budget (took %d ms)" % elapsed)


# ---------------------------------------------------------------------------
# 16. Multiple rooms with interleaved poll_events
# ---------------------------------------------------------------------------

func test_multiple_rooms_interleaved_poll():
	var rooms := []
	var before := Time.get_ticks_msec()
	for i in range(5):
		var room := LiveKitRoom.new()
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		rooms.append(room)
	# Interleave poll across all rooms
	for _round in range(5):
		for room in rooms:
			room.poll_events()
	for room in rooms:
		room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"5 rooms with interleaved polling should not deadlock (took %d ms)" % elapsed)
	rooms.clear()


# ---------------------------------------------------------------------------
# 17. Room disconnect then immediate reconnect (rapid reuse)
# ---------------------------------------------------------------------------

func test_rapid_disconnect_reconnect():
	var room := LiveKitRoom.new()
	var before := Time.get_ticks_msec()
	for i in range(5):
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		room.disconnect_from_room()
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		room.poll_events()
		room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"5 disconnect/reconnect cycles should not block (took %d ms)" % elapsed)


# ---------------------------------------------------------------------------
# 18. VideoStream create and immediately null (GC)
# ---------------------------------------------------------------------------

func test_video_stream_immediate_gc():
	var before := Time.get_ticks_msec()
	for i in range(50):
		var stream := LiveKitVideoStream.new()
		stream = null
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"50 VideoStream create/null cycles should not crash (took %d ms)" % elapsed)


# ---------------------------------------------------------------------------
# 19. AudioStream create and immediately null (GC)
# ---------------------------------------------------------------------------

func test_audio_stream_immediate_gc():
	var before := Time.get_ticks_msec()
	for i in range(50):
		var stream := LiveKitAudioStream.new()
		stream = null
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"50 AudioStream create/null cycles should not crash (took %d ms)" % elapsed)


# ---------------------------------------------------------------------------
# 20. Room connect with poll in tight loop then disconnect
# ---------------------------------------------------------------------------

func test_poll_tight_loop_during_connect():
	var room := LiveKitRoom.new()
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	var before := Time.get_ticks_msec()
	for i in range(100):
		room.poll_events()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"100 poll_events in tight loop should not block (took %d ms)" % elapsed)
	room.disconnect_from_room()
