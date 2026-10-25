extends GutTest
## Web-only screen capture tests, for browser behaviors with no native equivalent. Run by
## test_web.sh when RUN_E2E is set (headless Chrome's fake UI shares a fake screen).


func test_restarting_while_the_user_chooses_leaves_nothing_behind():
	var videos_before := _video_elements()
	var capture := LiveKitScreenCapture.create()
	capture.set_auto_poll(false)
	# Each start() asks the user, so the first answer arrives after the restart.
	capture.start()
	capture.stop()
	capture.start()
	var deadline := Time.get_ticks_msec() + 10000
	while not capture.poll() and Time.get_ticks_msec() < deadline:
		await get_tree().create_timer(0.05).timeout
	# Both answers are in by now, the first having arrived before the second.
	var videos_while_capturing := _video_elements()

	capture.close()

	assert_eq([videos_while_capturing - videos_before, _video_elements() - videos_before], [1, 0])


func _video_elements() -> int:
	return JavaScriptBridge.eval("document.querySelectorAll('video').length")
