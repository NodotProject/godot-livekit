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

	var state := {"stream": null}
	_room2.track_subscribed.connect(func(remote_track, _publication, _participant):
		if remote_track.get_kind() == LiveKitTrack.KIND_AUDIO:
			state.stream = LiveKitAudioStream.from_track(remote_track)
	)

	var generator := AudioStreamGenerator.new()
	generator.mix_rate = SAMPLE_RATE
	generator.buffer_length = 0.5
	var player := AudioStreamPlayer.new()
	player.stream = generator
	player.bus = BUS
	# Web exports default to sample playback, which generators don't support.
	player.playback_type = AudioServer.PLAYBACK_TYPE_STREAM
	add_child_autofree(player)
	player.play()
	var playback: AudioStreamGeneratorPlayback = player.get_stream_playback()

	# Feed a 440Hz tone in real time while measuring what plays on the receiving side.
	var tone := PackedFloat32Array()
	tone.resize(FRAME)
	var sent := 0
	var received := 0
	var played_energy := 0.0
	var played_frames := 0
	var start := Time.get_ticks_msec()
	while Time.get_ticks_msec() - start < 10000 and received < SAMPLE_RATE:
		var due := int((Time.get_ticks_msec() - start) * SAMPLE_RATE / 1000.0)
		while sent + FRAME <= due:
			for i in FRAME:
				tone[i] = 0.5 * sin(TAU * 440.0 * (sent + i) / SAMPLE_RATE)
			source.capture_frame(tone, SAMPLE_RATE, 1, FRAME)
			sent += FRAME
		if state.stream:
			received += state.stream.poll(playback)
		for frame in _capture.get_buffer(_capture.get_frames_available()):
			played_energy += frame.x * frame.x
			played_frames += 1
		await get_tree().process_frame

	var played_rms := sqrt(played_energy / max(played_frames, 1))
	assert_not_null(state.stream, "Room2 should subscribe to the audio track")
	assert_gte(received, SAMPLE_RATE, "Room2 should receive a second of audio")
	assert_gt(played_rms, 0.05, "Received audio should play (a 0.5 amplitude tone, not silence)")
	if state.stream:
		state.stream.close()
