extends "res://test/e2e/e2e_helper.gd"
## E2E: audio captured by one room is received, and playable, in another.

const SAMPLE_RATE := 48000
const FRAME := 480  # 10ms
const BUS := "E2EAudioFlow"

var _capture: AudioEffectCapture


func before_each():
	super.before_each()
	AudioServer.add_bus()
	AudioServer.set_bus_name(AudioServer.bus_count - 1, BUS)
	_capture = AudioEffectCapture.new()
	AudioServer.add_bus_effect(AudioServer.bus_count - 1, _capture)


func after_each():
	super.after_each()
	AudioServer.remove_bus(AudioServer.get_bus_index(BUS))


func test_published_audio_reaches_remote_room():
	if _skip_if_no_server():
		return

	var both = await _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	var source = LiveKitAudioSource.create(SAMPLE_RATE, 1, 0)
	var track = LiveKitLocalAudioTrack.create("e2e_tone", source)
	_room.get_local_participant().publish_track(track, {"source": LiveKitTrack.SOURCE_MICROPHONE})
	await _assert_tone_reaches_room2(source, "e2e_tone")


func test_audio_flows_after_another_track_from_its_source_is_unpublished():
	if _skip_if_no_server():
		return

	var both = await _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	var source = LiveKitAudioSource.create(SAMPLE_RATE, 1, 0)
	var local := _room.get_local_participant()
	var published := []
	_room.local_track_published.connect(func(publication, _track): published.append(publication))
	local.publish_track(LiveKitLocalAudioTrack.create("first", source), {"source": LiveKitTrack.SOURCE_MICROPHONE})
	assert_true(await _poll_until(_room, func(): return published.size() == 1, 10.0), "first track published")
	local.unpublish_track(published[0].get_sid())
	assert_true(
		await _poll_until(_room, func(): return local.get_track_publications().is_empty(), 10.0),
		"first track unpublished",
	)

	var second = LiveKitLocalAudioTrack.create("second", source)
	local.publish_track(second, {"source": LiveKitTrack.SOURCE_MICROPHONE})
	await _assert_tone_reaches_room2(source, "second")


func test_audio_flows_after_its_track_is_unpublished_and_published_again():
	if _skip_if_no_server():
		return

	var both = await _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	var source = LiveKitAudioSource.create(SAMPLE_RATE, 1, 0)
	var track = LiveKitLocalAudioTrack.create("again", source)
	var local := _room.get_local_participant()
	var published := []
	_room.local_track_published.connect(func(publication, _track): published.append(publication))
	local.publish_track(track, {"source": LiveKitTrack.SOURCE_MICROPHONE})
	assert_true(await _poll_until(_room, func(): return published.size() == 1, 10.0), "track published")
	local.unpublish_track(published[0].get_sid())
	assert_true(
		await _poll_until(_room, func(): return local.get_track_publications().is_empty(), 10.0),
		"track unpublished",
	)

	local.publish_track(track, {"source": LiveKitTrack.SOURCE_MICROPHONE})
	await _assert_tone_reaches_room2(source, "again")


func test_audio_left_unpolled_does_not_delay_later_audio():
	if _skip_if_no_server():
		return

	var both = await _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	var source = LiveKitAudioSource.create(SAMPLE_RATE, 1, 0)
	_room.get_local_participant().publish_track(LiveKitLocalAudioTrack.create("e2e_unpolled", source), {"source": LiveKitTrack.SOURCE_MICROPHONE})
	var state := {"stream": null}
	_room2.track_subscribed.connect(func(remote_track, _publication, _participant):
		if remote_track.get_kind() == LiveKitTrack.KIND_AUDIO:
			state.stream = LiveKitAudioStream.from_track(remote_track)
	)
	var playback := _playback(0.1)
	var receiving := await _feed_tone_until(source, func(): return state.stream and state.stream.poll(playback) > 0, 10.0)
	assert_true(receiving, "Room2 should receive audio")
	if not receiving:
		return

	# As when a game stalls, or its browser tab is in the background.
	await _feed_tone_until(source, func(): return false, 2.0)
	state.stream.poll(playback)

	# What didn't fit would otherwise play late.
	assert_lt(state.stream.poll(_playback(5.0)), SAMPLE_RATE / 5, "Audio that didn't fit should be dropped")
	state.stream.close()


## Captures a tone into source in real time and checks that room2 receives and plays it from
## the remote track named track_name.
func _assert_tone_reaches_room2(source: LiveKitAudioSource, track_name: String) -> void:
	var state := {"stream": null}
	_room2.track_subscribed.connect(func(remote_track, _publication, _participant):
		if remote_track.get_kind() == LiveKitTrack.KIND_AUDIO and remote_track.get_name() == track_name:
			state.stream = LiveKitAudioStream.from_track(remote_track)
	)

	var playback := _playback(0.5)

	# Feed a tone in real time while measuring what plays on the receiving side. (Lambdas capture
	# values, so the counts are kept in a Dictionary.)
	var counts := {"received": 0, "played_energy": 0.0, "played_frames": 0}
	await _feed_tone_until(source, func():
		if state.stream:
			counts.received += state.stream.poll(playback)
		for frame in _capture.get_buffer(_capture.get_frames_available()):
			counts.played_energy += frame.x * frame.x
			counts.played_frames += 1
		return counts.received >= SAMPLE_RATE, 10.0)

	var played_rms := sqrt(counts.played_energy / max(counts.played_frames, 1))
	assert_not_null(state.stream, "Room2 should subscribe to the audio track")
	assert_gte(counts.received, SAMPLE_RATE, "Room2 should receive a second of audio")
	assert_gt(played_rms, 0.05, "Received audio should play (a 0.5 amplitude tone, not silence)")
	if state.stream:
		state.stream.close()


## Returns the playback of a generator playing on BUS.
func _playback(buffer_length: float) -> AudioStreamGeneratorPlayback:
	var generator := AudioStreamGenerator.new()
	generator.mix_rate = SAMPLE_RATE
	generator.buffer_length = buffer_length
	var player := AudioStreamPlayer.new()
	player.stream = generator
	player.bus = BUS
	# Web exports default to sample playback, which generators don't support.
	player.playback_type = AudioServer.PLAYBACK_TYPE_STREAM
	add_child_autofree(player)
	player.play()
	return player.get_stream_playback()


## Captures a 440Hz tone into source in real time until condition (checked each frame) returns
## true, or timeout_sec passes. Returns whether condition did.
func _feed_tone_until(source: LiveKitAudioSource, condition: Callable, timeout_sec: float) -> bool:
	var tone := PackedFloat32Array()
	tone.resize(FRAME)
	var sent := 0
	var start := Time.get_ticks_msec()
	while Time.get_ticks_msec() - start < timeout_sec * 1000:
		var due := int((Time.get_ticks_msec() - start) * SAMPLE_RATE / 1000.0)
		while sent + FRAME <= due:
			for i in FRAME:
				tone[i] = 0.5 * sin(TAU * 440.0 * (sent + i) / SAMPLE_RATE)
			source.capture_frame(tone, SAMPLE_RATE, 1, FRAME)
			sent += FRAME
		if condition.call():
			return true
		await get_tree().process_frame
	return false
