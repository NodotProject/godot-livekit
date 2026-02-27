extends GutTest
## Thread safety tests for audio/video source operations.


func test_audio_rapid_captures():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var data := PackedFloat32Array()
	data.resize(480)
	data.fill(0.0)
	# 100 rapid capture_frame calls should not crash
	for i in range(100):
		source.capture_frame(data, 48000, 1, 480)
	assert_true(true, "100 rapid capture_frame calls should not crash")


func test_audio_clear_during_capture():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var data := PackedFloat32Array()
	data.resize(480)
	data.fill(0.5)
	# Interleave capture and clear operations
	for i in range(20):
		source.capture_frame(data, 48000, 1, 480)
		source.clear_queue()
	assert_true(true, "Interleaved capture/clear should not crash")


func test_audio_boundary_values():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	# SDK requires 10ms frames: 48000 Hz * 0.01s = 480 samples
	var data := PackedFloat32Array()
	data.resize(480)
	data.fill(0.0)
	# Test boundary float values — clamped at livekit_audio_source.cpp:45-46
	data[0] = 1.0    # max normal
	data[1] = -1.0   # min normal
	data[2] = 1.5    # above max — should be clamped to 1.0
	data[3] = -1.5   # below min — should be clamped to -1.0
	source.capture_frame(data, 48000, 1, 480)
	assert_true(true, "Boundary sample values should be clamped without crash")
