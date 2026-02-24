extends GutTest
## Performance tests for audio capture throughput.


func test_sustained_capture():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var data := PackedFloat32Array()
	data.resize(480)  # 10ms at 48kHz
	data.fill(0.1)

	var before := Time.get_ticks_msec()
	for i in range(1000):
		source.capture_frame(data, 48000, 1, 480)
	var elapsed := Time.get_ticks_msec() - before

	# 1000 frames * 480 samples = 480000 samples at 48kHz = 10 seconds of audio
	# Effective rate = 480000 / (elapsed_sec)
	var elapsed_sec := elapsed / 1000.0
	var effective_rate := 480000.0 / elapsed_sec if elapsed_sec > 0 else 0.0
	gut.p("Sustained capture: 1000 frames in %d ms (effective rate: %.0f Hz)" % [elapsed, effective_rate])
	assert_gt(effective_rate, 40000.0,
		"Effective capture rate should exceed 40kHz (got %.0f Hz)" % effective_rate)


func test_burst_capture():
	var source = LiveKitAudioSource.create(48000, 1, 500)  # 500ms queue
	var data := PackedFloat32Array()
	data.resize(480)  # 10ms at 48kHz
	data.fill(0.5)

	for i in range(50):
		source.capture_frame(data, 48000, 1, 480)

	var duration = source.get_queued_duration()
	gut.p("Burst capture: 50 frames queued, duration = %.3f s" % duration)
	assert_gt(duration, 0.0,
		"Queued duration should be > 0 after burst capture")
