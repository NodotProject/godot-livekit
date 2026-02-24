extends GutTest
## Tests LiveKitVideoSource.create() factory and capture.


func test_create_video_source():
	var source = LiveKitVideoSource.create(640, 480)
	assert_not_null(source, "VideoSource.create should return non-null")


func test_video_source_width():
	var source = LiveKitVideoSource.create(1920, 1080)
	assert_eq(source.get_width(), 1920,
		"VideoSource width should match creation parameter")


func test_video_source_height():
	var source = LiveKitVideoSource.create(1920, 1080)
	assert_eq(source.get_height(), 1080,
		"VideoSource height should match creation parameter")


func test_video_source_is_ref_counted():
	var source = LiveKitVideoSource.create(640, 480)
	assert_true(source is RefCounted, "VideoSource should be RefCounted")


func test_video_source_capture_rgba():
	var source = LiveKitVideoSource.create(4, 4)
	var image := Image.create(4, 4, false, Image.FORMAT_RGBA8)
	image.fill(Color.RED)
	# Should not crash
	source.capture_frame(image, 0, 0)
	assert_true(true, "capture_frame with RGBA8 image should not crash")


func test_video_source_capture_rgb_converts():
	var source = LiveKitVideoSource.create(4, 4)
	# RGB8 format — should be auto-converted to RGBA8 internally
	var image := Image.create(4, 4, false, Image.FORMAT_RGB8)
	image.fill(Color.BLUE)
	source.capture_frame(image, 0, 0)
	assert_true(true, "capture_frame with RGB8 image should auto-convert without crash")


func test_video_source_capture_with_rotation():
	var source = LiveKitVideoSource.create(4, 4)
	var image := Image.create(4, 4, false, Image.FORMAT_RGBA8)
	image.fill(Color.GREEN)
	# Test all valid rotation values
	for rot in [0, 90, 180, 270]:
		source.capture_frame(image, 0, rot)
	assert_true(true, "capture_frame with rotations 0/90/180/270 should be accepted")
