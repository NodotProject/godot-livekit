extends GutTest
## Tests all track classes: enum values, unbound defaults, and null-guard safety.


func test_track_kind_enum_values():
	assert_eq(LiveKitTrack.KIND_UNKNOWN, 0, "KIND_UNKNOWN should be 0")
	assert_eq(LiveKitTrack.KIND_AUDIO, 1, "KIND_AUDIO should be 1")
	assert_eq(LiveKitTrack.KIND_VIDEO, 2, "KIND_VIDEO should be 2")


func test_track_source_enum_values():
	assert_eq(LiveKitTrack.SOURCE_UNKNOWN, 0, "SOURCE_UNKNOWN should be 0")
	assert_eq(LiveKitTrack.SOURCE_CAMERA, 1, "SOURCE_CAMERA should be 1")
	assert_eq(LiveKitTrack.SOURCE_MICROPHONE, 2, "SOURCE_MICROPHONE should be 2")
	assert_eq(LiveKitTrack.SOURCE_SCREENSHARE, 3, "SOURCE_SCREENSHARE should be 3")
	assert_eq(LiveKitTrack.SOURCE_SCREENSHARE_AUDIO, 4, "SOURCE_SCREENSHARE_AUDIO should be 4")


func test_track_stream_state_enum_values():
	assert_eq(LiveKitTrack.STATE_UNKNOWN, 0, "STATE_UNKNOWN should be 0")
	assert_eq(LiveKitTrack.STATE_ACTIVE, 1, "STATE_ACTIVE should be 1")
	assert_eq(LiveKitTrack.STATE_PAUSED, 2, "STATE_PAUSED should be 2")


func test_unbound_track_sid_empty():
	var t := LiveKitTrack.new()
	assert_eq(t.get_sid(), "", "Unbound track SID should be empty")


func test_unbound_track_name_empty():
	var t := LiveKitTrack.new()
	assert_eq(t.get_name(), "", "Unbound track name should be empty")


func test_unbound_track_kind_unknown():
	var t := LiveKitTrack.new()
	assert_eq(t.get_kind(), LiveKitTrack.KIND_UNKNOWN,
		"Unbound track kind should be KIND_UNKNOWN")


func test_unbound_track_source_unknown():
	var t := LiveKitTrack.new()
	assert_eq(t.get_source(), LiveKitTrack.SOURCE_UNKNOWN,
		"Unbound track source should be SOURCE_UNKNOWN")


func test_unbound_track_not_muted():
	var t := LiveKitTrack.new()
	assert_false(t.get_muted(), "Unbound track should not be muted")


func test_unbound_track_stream_state_unknown():
	var t := LiveKitTrack.new()
	assert_eq(t.get_stream_state(), LiveKitTrack.STATE_UNKNOWN,
		"Unbound track stream state should be STATE_UNKNOWN")


func test_unbound_track_request_stats_no_crash():
	var t := LiveKitTrack.new()
	# Should not crash — null guard returns early
	t.request_stats()
	assert_true(true, "request_stats on unbound track should not crash")
	assert_push_error("not bound")


func test_local_audio_track_inheritance():
	var t := LiveKitLocalAudioTrack.new()
	assert_true(t is LiveKitTrack,
		"LocalAudioTrack should inherit LiveKitTrack")
	assert_true(t is RefCounted,
		"LocalAudioTrack should be RefCounted")


func test_local_video_track_inheritance():
	var t := LiveKitLocalVideoTrack.new()
	assert_true(t is LiveKitTrack,
		"LocalVideoTrack should inherit LiveKitTrack")


func test_remote_audio_track_inheritance():
	var t := LiveKitRemoteAudioTrack.new()
	assert_true(t is LiveKitTrack,
		"RemoteAudioTrack should inherit LiveKitTrack")


func test_remote_video_track_inheritance():
	var t := LiveKitRemoteVideoTrack.new()
	assert_true(t is LiveKitTrack,
		"RemoteVideoTrack should inherit LiveKitTrack")


func test_local_audio_track_mute_unmute_unbound():
	var t := LiveKitLocalAudioTrack.new()
	# Should not crash — null guard checks track_ pointer
	t.mute()
	t.unmute()
	assert_true(true, "mute/unmute on unbound local audio track should not crash")


func test_local_video_track_mute_unmute_unbound():
	var t := LiveKitLocalVideoTrack.new()
	# Should not crash — null guard checks track_ pointer
	t.mute()
	t.unmute()
	assert_true(true, "mute/unmute on unbound local video track should not crash")


func test_create_local_audio_track_null_source():
	var t = LiveKitLocalAudioTrack.create("test", null)
	assert_null(t, "create with null source should return null")
	assert_push_error("source is null")
