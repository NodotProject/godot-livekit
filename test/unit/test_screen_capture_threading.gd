extends GutTest
## Threading stress tests for LiveKitScreenCapture, kept apart from test_threading_stress.gd
## because builds without screen capture (e.g. Android) can't compile them. Like those, they use a
## 500ms frame budget to detect main-thread blocking.

const FRAME_BUDGET_MS := 500


# ---------------------------------------------------------------------------
# 1. ScreenCapture start/stop rapid cycles (display-dependent)
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
# 2. ScreenCapture close during active capture (display-dependent)
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
# 3. ScreenCapture double close
# ---------------------------------------------------------------------------

func test_screen_capture_double_close():
	var capture := LiveKitScreenCapture.new()
	capture.close()
	capture.close()
	assert_true(true, "ScreenCapture double close should not crash")


# ---------------------------------------------------------------------------
# 4. ScreenCapture poll after close
# ---------------------------------------------------------------------------

func test_screen_capture_poll_after_close():
	var capture := LiveKitScreenCapture.new()
	capture.close()
	var result = capture.poll()
	assert_false(result,
		"poll() after close should return false without crash")
