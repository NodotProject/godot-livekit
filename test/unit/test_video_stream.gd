extends GutTest
## Tests LiveKitVideoStream defaults and null-guard factory behavior.


func test_create_video_stream():
	var stream := LiveKitVideoStream.new()
	assert_not_null(stream, "VideoStream.new() should return non-null")
	stream.close()


func test_video_stream_from_track_null():
	var result = LiveKitVideoStream.from_track(null)
	assert_null(result,
		"from_track(null) should return null")


func test_video_stream_from_track_unbound():
	var track := LiveKitTrack.new()
	var result = LiveKitVideoStream.from_track(track)
	assert_null(result,
		"from_track with unbound track should return null")


func test_video_stream_poll_default_false():
	var stream := LiveKitVideoStream.new()
	var result = stream.poll()
	assert_false(result,
		"poll on default VideoStream should return false")
	stream.close()


func test_video_stream_close_default():
	var stream := LiveKitVideoStream.new()
	# Should not crash on default-constructed stream
	stream.close()
	assert_true(true, "close on default VideoStream should not crash")


func test_video_stream_texture_null_default():
	var stream := LiveKitVideoStream.new()
	assert_null(stream.get_texture(),
		"Default VideoStream texture should be null")
	stream.close()


func test_video_stream_is_ref_counted():
	var stream := LiveKitVideoStream.new()
	assert_true(stream is RefCounted, "VideoStream should be RefCounted")
	stream.close()
