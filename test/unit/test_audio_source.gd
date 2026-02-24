extends GutTest
## Tests LiveKitAudioSource.create() factory and operations.


func test_create_audio_source():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	assert_not_null(source, "AudioSource.create should return non-null")


func test_audio_source_sample_rate():
	var source = LiveKitAudioSource.create(44100, 2, 100)
	assert_eq(source.get_sample_rate(), 44100,
		"AudioSource sample rate should match creation parameter")


func test_audio_source_num_channels():
	var source = LiveKitAudioSource.create(48000, 2, 0)
	assert_eq(source.get_num_channels(), 2,
		"AudioSource num_channels should match creation parameter")


func test_audio_source_mono():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	assert_eq(source.get_num_channels(), 1,
		"Mono AudioSource should have 1 channel")


func test_audio_source_queued_duration_zero():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	assert_eq(source.get_queued_duration(), 0.0,
		"New AudioSource should have zero queued duration")


func test_audio_source_capture_frame():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	# 480 samples = 10ms at 48kHz
	var data := PackedFloat32Array()
	data.resize(480)
	data.fill(0.0)
	# Should not crash or error
	source.capture_frame(data, 48000, 1, 480)
	assert_true(true, "capture_frame with valid data should not crash")


func test_audio_source_clear_queue():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	# Should not crash on new source
	source.clear_queue()
	assert_true(true, "clear_queue on new source should not crash")


func test_audio_source_is_ref_counted():
	var source = LiveKitAudioSource.create(48000, 1, 0)
	assert_true(source is RefCounted, "AudioSource should be RefCounted")
