extends "res://test/e2e/e2e_helper.gd"
## E2E: Track publishing lifecycle against the production server.


func _get_remote_track_publications(room: LiveKitRoom) -> Array:
	var pubs := []
	var remotes = room.get_remote_participants()
	for key in remotes:
		var rp = remotes[key]
		var rpubs = rp.get_track_publications()
		for pub_key in rpubs:
			pubs.append(rpubs[pub_key])
	return pubs


func test_audio_publish_and_remote_subscribe():
	if _skip_if_no_server():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	# Room1 publishes audio
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var track = LiveKitLocalAudioTrack.create("e2e_audio", source)
	assert_not_null(track, "Audio track should be created")
	var lp = _room.get_local_participant()
	lp.publish_track(track, {})

	# Room2 sees remote track publication with KIND_AUDIO
	var saw_audio = _poll_until(_room2, func():
		var pubs = _get_remote_track_publications(_room2)
		for pub in pubs:
			if pub.get_kind() == LiveKitTrack.KIND_AUDIO:
				return true
		return false, 15.0)
	assert_true(saw_audio, "Room2 should see audio track from Room1")

	if saw_audio:
		var pubs = _get_remote_track_publications(_room2)
		var found_audio := false
		for pub in pubs:
			if pub.get_kind() == LiveKitTrack.KIND_AUDIO:
				found_audio = true
				assert_ne(pub.get_sid(), "", "Track publication SID should not be empty")
		assert_true(found_audio, "Should find KIND_AUDIO publication")


func test_video_publish_and_remote_subscribe():
	if _skip_if_no_server():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	# Room1 publishes video
	var source = LiveKitVideoSource.create(640, 480)
	var track = LiveKitLocalVideoTrack.create("e2e_video", source)
	assert_not_null(track, "Video track should be created")
	var lp = _room.get_local_participant()
	lp.publish_track(track, {})

	# Room2 sees remote track publication
	var saw_video = _poll_until(_room2, func():
		var pubs = _get_remote_track_publications(_room2)
		for pub in pubs:
			if pub.get_kind() == LiveKitTrack.KIND_VIDEO:
				return true
		return false, 15.0)
	assert_true(saw_video, "Room2 should see video track from Room1")


func test_publish_unpublish_cycle():
	if _skip_if_no_server():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	# Publish audio track
	var source = LiveKitAudioSource.create(48000, 1, 0)
	var track = LiveKitLocalAudioTrack.create("e2e_unpublish", source)
	var lp = _room.get_local_participant()
	lp.publish_track(track, {})

	var published = _poll_until(_room, func():
		return lp.get_track_publications().size() > 0, 10.0)
	assert_true(published, "Track should be published")

	# Wait for remote to see it
	var remote_saw = _poll_until(_room2, func():
		return _get_remote_track_publications(_room2).size() > 0, 15.0)
	assert_true(remote_saw, "Room2 should see the track")

	# Unpublish
	var pubs = lp.get_track_publications()
	if pubs.size() > 0:
		var sid = pubs.keys()[0]
		lp.unpublish_track(sid)

	var unpublished = _poll_until(_room, func():
		return lp.get_track_publications().size() == 0, 10.0)
	assert_true(unpublished, "Track should be unpublished locally")

	# Verify remote sees removal
	var remote_gone = _poll_until(_room2, func():
		return _get_remote_track_publications(_room2).size() == 0, 15.0)
	assert_true(remote_gone, "Room2 should see track removed")


func test_multiple_tracks():
	if _skip_if_no_server():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	var lp = _room.get_local_participant()

	# Publish audio
	var audio_source = LiveKitAudioSource.create(48000, 1, 0)
	var audio_track = LiveKitLocalAudioTrack.create("e2e_multi_audio", audio_source)
	lp.publish_track(audio_track, {})

	# Publish video
	var video_source = LiveKitVideoSource.create(640, 480)
	var video_track = LiveKitLocalVideoTrack.create("e2e_multi_video", video_source)
	lp.publish_track(video_track, {})

	# Wait for both publications locally
	var both_local = _poll_until(_room, func():
		return lp.get_track_publications().size() >= 2, 10.0)
	assert_true(both_local, "Should have 2 local publications")

	# Wait for Room2 to see both
	var both_remote = _poll_until(_room2, func():
		return _get_remote_track_publications(_room2).size() >= 2, 15.0)
	assert_true(both_remote, "Room2 should see both tracks from Room1")

	if both_remote:
		var pubs = _get_remote_track_publications(_room2)
		var has_audio := false
		var has_video := false
		for pub in pubs:
			if pub.get_kind() == LiveKitTrack.KIND_AUDIO:
				has_audio = true
			elif pub.get_kind() == LiveKitTrack.KIND_VIDEO:
				has_video = true
		assert_true(has_audio, "Should see audio track remotely")
		assert_true(has_video, "Should see video track remotely")
