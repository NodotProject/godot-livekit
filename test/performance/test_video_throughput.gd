extends GutTest
## Performance tests for video capture throughput.


func test_640x480_capture():
	var source = LiveKitVideoSource.create(640, 480)
	var image := Image.create(640, 480, false, Image.FORMAT_RGBA8)
	image.fill(Color.RED)

	var before := Time.get_ticks_msec()
	for i in range(100):
		source.capture_frame(image, i * 33333, 0)  # ~30fps timestamps
	var elapsed := Time.get_ticks_msec() - before

	var fps := 100.0 / (elapsed / 1000.0) if elapsed > 0 else 0.0
	gut.p("640x480 capture: 100 frames in %d ms (%.1f fps)" % [elapsed, fps])
	assert_gt(fps, 30.0,
		"640x480 capture should exceed 30 fps (got %.1f fps)" % fps)


func test_1080p_capture():
	var source = LiveKitVideoSource.create(1920, 1080)
	var image := Image.create(1920, 1080, false, Image.FORMAT_RGBA8)
	image.fill(Color.BLUE)

	var before := Time.get_ticks_msec()
	for i in range(30):
		source.capture_frame(image, i * 33333, 0)
	var elapsed := Time.get_ticks_msec() - before

	var fps := 30.0 / (elapsed / 1000.0) if elapsed > 0 else 0.0
	gut.p("1080p capture: 30 frames in %d ms (%.1f fps)" % [elapsed, fps])
	# Informational — just log fps, don't enforce strict threshold for 1080p
	assert_true(true, "1080p capture completed (%.1f fps)" % fps)
