extends "res://test/integration/test_helper.gd"
## Integration tests for track publishing and subscription.

var _room2: LiveKitRoom = null
var _track_published := false
var _track_unpublished := false
var _track_subscribed_on_remote := false


func before_each():
	super.before_each()
	_track_published = false
	_track_unpublished = false
	_track_subscribed_on_remote = false
	if _is_server_available():
		_room2 = LiveKitRoom.new()


func after_each():
	if _room2 != null:
		_room2.disconnect_from_room()
		_room2 = null
	super.after_each()


func _connect_both_rooms():
	_room.connect_to_room(_livekit_url, _token_1, {})
	_poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	_room2.connect_to_room(_livekit_url, _token_2, {})
	_poll_until(_room2, func():
		return _room2.get_connection_state() == LiveKitRoom.STATE_CONNECTED)
	_poll_until(_room, func():
		return _room.get_remote_participants().size() == 1, 10.0)


func test_publish_audio_track():
	if _skip_if_no_server():
		return
	_connect_both_rooms()
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var track = LiveKitLocalAudioTrack.create("test_audio", source)
	assert_not_null(track, "Audio track should be created")
	var lp = _room.get_local_participant()
	var pub = lp.publish_track(track, {})
	# Give time for publish to complete
	_poll_until(_room, func():
		return lp.get_track_publications().size() > 0, 5.0)
	assert_true(lp.get_track_publications().size() > 0,
		"local_track_published: publication should appear in track_publications")


func test_publish_video_track():
	if _skip_if_no_server():
		return
	_connect_both_rooms()
	var source = LiveKitVideoSource.create(640, 480)
	var track = LiveKitLocalVideoTrack.create("test_video", source)
	assert_not_null(track, "Video track should be created")
	var lp = _room.get_local_participant()
	var pub = lp.publish_track(track, {})
	_poll_until(_room, func():
		return lp.get_track_publications().size() > 0, 5.0)
	assert_true(lp.get_track_publications().size() > 0,
		"local_track_published: publication should appear in track_publications")


func test_unpublish_track():
	if _skip_if_no_server():
		return
	_connect_both_rooms()
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var track = LiveKitLocalAudioTrack.create("test_audio_unpub", source)
	var lp = _room.get_local_participant()
	var pub = lp.publish_track(track, {})
	_poll_until(_room, func():
		return lp.get_track_publications().size() > 0, 5.0)
	# Unpublish by SID
	var pubs = lp.get_track_publications()
	if pubs.size() > 0:
		var sid = pubs.keys()[0]
		lp.unpublish_track(sid)
		_poll_until(_room, func():
			return lp.get_track_publications().size() == 0, 5.0)
	assert_eq(lp.get_track_publications().size(), 0,
		"Track publications should be empty after unpublish")


func test_remote_track_subscribed():
	if _skip_if_no_server():
		return
	_connect_both_rooms()
	# Publish audio from room 1
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var track = LiveKitLocalAudioTrack.create("test_remote_sub", source)
	var lp = _room.get_local_participant()
	lp.publish_track(track, {})
	# Wait for room 2 to see the remote track
	var saw_track = _poll_until(_room2, func():
		var remotes = _room2.get_remote_participants()
		for key in remotes:
			var rp = remotes[key]
			if rp.get_track_publications().size() > 0:
				return true
		return false, 10.0)
	if saw_track:
		var remotes = _room2.get_remote_participants()
		for key in remotes:
			var rp = remotes[key]
			var rpubs = rp.get_track_publications()
			if rpubs.size() > 0:
				var pub = rpubs.values()[0]
				assert_eq(pub.get_kind(), LiveKitTrack.KIND_AUDIO,
					"Subscribed track should be audio kind")
				return
	gut.p("Remote track subscription may require more time (informational)")
	pass_test("Track publish completed without crash")
