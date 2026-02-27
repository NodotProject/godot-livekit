extends GutTest
## Tests LiveKitAudioStream defaults and null-guard factory behavior.


func test_audio_stream_default_sample_rate():
	var stream := LiveKitAudioStream.new()
	assert_eq(stream.get_sample_rate(), 48000,
		"Default AudioStream sample rate should be 48000")
	stream.close()


func test_audio_stream_default_channels():
	var stream := LiveKitAudioStream.new()
	assert_eq(stream.get_num_channels(), 1,
		"Default AudioStream num_channels should be 1")
	stream.close()


func test_audio_stream_from_track_null():
	var result = LiveKitAudioStream.from_track(null)
	assert_null(result,
		"from_track(null) should return null")
	assert_push_error("invalid track")


func test_audio_stream_from_track_unbound():
	var track := LiveKitTrack.new()
	var result = LiveKitAudioStream.from_track(track)
	assert_null(result,
		"from_track with unbound track should return null")
	assert_push_error("invalid track")


func test_audio_stream_close_default():
	var stream := LiveKitAudioStream.new()
	# Should not crash on default-constructed stream
	stream.close()
	assert_true(true, "close on default AudioStream should not crash")


func test_audio_stream_is_ref_counted():
	var stream := LiveKitAudioStream.new()
	assert_true(stream is RefCounted, "AudioStream should be RefCounted")
	stream.close()
