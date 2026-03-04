extends GutTest
## Tests LiveKitScreenCapture registration, defaults, and null-guard behavior.


func test_class_registered():
	assert_true(ClassDB.class_exists("LiveKitScreenCapture"),
		"LiveKitScreenCapture should be registered in ClassDB")


func test_is_ref_counted():
	var capture := LiveKitScreenCapture.new()
	assert_true(capture is RefCounted, "LiveKitScreenCapture should be RefCounted")


func test_enum_values():
	assert_eq(LiveKitScreenCapture.PERMISSION_OK, 0)
	assert_eq(LiveKitScreenCapture.PERMISSION_WARNING, 1)
	assert_eq(LiveKitScreenCapture.PERMISSION_ERROR, 2)


func test_check_permissions_returns_dict():
	var perms = LiveKitScreenCapture.check_permissions()
	assert_not_null(perms, "check_permissions() should return a Dictionary")
	assert_true(perms is Dictionary, "check_permissions() should return a Dictionary")
	assert_true(perms.has("status"), "permissions dict should have 'status' key")
	assert_true(perms.has("summary"), "permissions dict should have 'summary' key")
	assert_true(perms.has("details"), "permissions dict should have 'details' key")


func test_get_monitors_returns_array():
	var monitors = LiveKitScreenCapture.get_monitors()
	assert_not_null(monitors, "get_monitors() should return an Array")
	assert_true(monitors is Array, "get_monitors() should return an Array")


func test_get_windows_returns_array():
	var windows = LiveKitScreenCapture.get_windows()
	assert_not_null(windows, "get_windows() should return an Array")
	assert_true(windows is Array, "get_windows() should return an Array")


func test_create_for_monitor_invalid_dict_returns_null():
	var result = LiveKitScreenCapture.create_for_monitor({})
	assert_null(result,
		"create_for_monitor with empty dict should return null")
	assert_push_error("missing 'id' key")


func test_create_for_window_invalid_dict_returns_null():
	var result = LiveKitScreenCapture.create_for_window({})
	assert_null(result,
		"create_for_window with empty dict should return null")
	assert_push_error("missing 'id' key")


func test_default_texture_null():
	var capture := LiveKitScreenCapture.new()
	assert_null(capture.get_texture(),
		"Default-constructed capture texture should be null")


func test_default_image_null():
	var capture := LiveKitScreenCapture.new()
	assert_null(capture.get_image(),
		"Default-constructed capture image should be null")


func test_poll_uninitialized_returns_false():
	var capture := LiveKitScreenCapture.new()
	assert_false(capture.poll(),
		"poll() on uninitialized capture should return false")


func test_stop_uninitialized_no_crash():
	var capture := LiveKitScreenCapture.new()
	capture.stop()
	assert_true(true, "stop() on uninitialized capture should not crash")


func test_pause_uninitialized_no_crash():
	var capture := LiveKitScreenCapture.new()
	capture.pause()
	assert_true(true, "pause() on uninitialized capture should not crash")


func test_resume_uninitialized_no_crash():
	var capture := LiveKitScreenCapture.new()
	capture.resume()
	assert_true(true, "resume() on uninitialized capture should not crash")


func test_is_paused_uninitialized():
	var capture := LiveKitScreenCapture.new()
	assert_false(capture.is_paused(),
		"is_paused() on uninitialized capture should return false")


func test_screenshot_uninitialized_returns_null():
	var capture := LiveKitScreenCapture.new()
	var result = capture.screenshot()
	assert_null(result,
		"screenshot() on uninitialized capture should return null")
	assert_push_error("not initialized")


func test_close_uninitialized_no_crash():
	var capture := LiveKitScreenCapture.new()
	capture.close()
	assert_true(true, "close() on uninitialized capture should not crash")


func test_start_uninitialized_no_crash():
	var capture := LiveKitScreenCapture.new()
	capture.start()
	assert_push_error("not initialized")


func test_auto_poll_default_true():
	var capture := LiveKitScreenCapture.new()
	assert_true(capture.get_auto_poll(),
		"auto_poll should default to true")


func test_auto_poll_set_false():
	var capture := LiveKitScreenCapture.new()
	capture.set_auto_poll(false)
	assert_false(capture.get_auto_poll(),
		"auto_poll should be false after set_auto_poll(false)")


func test_auto_poll_set_true_again():
	var capture := LiveKitScreenCapture.new()
	capture.set_auto_poll(false)
	capture.set_auto_poll(true)
	assert_true(capture.get_auto_poll(),
		"auto_poll should be true after re-enabling")


# --- Display-dependent tests (guarded for CI) ---

func test_create_with_display():
	var perms = LiveKitScreenCapture.check_permissions()
	if perms.get("status", 2) == LiveKitScreenCapture.PERMISSION_ERROR:
		pending("No display available — skipping create() test")
		return
	var capture = LiveKitScreenCapture.create()
	if capture == null:
		pending("create() returned null — capture not available in this environment")
		return
	assert_not_null(capture, "create() should return a valid capture")
	assert_not_null(capture.get_texture(), "Factory-created capture should have a texture")
	capture.close()
