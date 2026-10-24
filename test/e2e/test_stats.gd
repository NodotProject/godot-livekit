extends "res://test/e2e/e2e_helper.gd"
## E2E: WebRTC stats for published and subscribed tracks.

const SAMPLE_RATE := 48000
const FRAME := 480  # 10ms


func test_stats_report_audio_sent_and_received():
	if _skip_if_no_server():
		return
	assert_true(await _connect_both_rooms(), "Both rooms should connect")

	var source = LiveKitAudioSource.create(SAMPLE_RATE, 1, 0)
	var local_track = LiveKitLocalAudioTrack.create("e2e_stats", source)
	_room.get_local_participant().publish_track(local_track, {"source": LiveKitTrack.SOURCE_MICROPHONE})
	var state := {"remote_track": null}
	_room2.track_subscribed.connect(func(track, _publication, _participant):
		if track.get_kind() == LiveKitTrack.KIND_AUDIO:
			state.remote_track = track
	)

	# Send a couple of seconds of tone so that both sides have packets to count.
	var tone := PackedFloat32Array()
	tone.resize(FRAME)
	for i in FRAME:
		tone[i] = 0.5 * sin(TAU * 440.0 * i / SAMPLE_RATE)
	var sent := 0
	var start := Time.get_ticks_msec()
	while Time.get_ticks_msec() - start < 2000 or (state.remote_track == null and Time.get_ticks_msec() - start < 10000):
		while sent < (Time.get_ticks_msec() - start) * SAMPLE_RATE / 1000:
			source.capture_frame(tone, SAMPLE_RATE, 1, FRAME)
			sent += FRAME
		_room.poll_events()
		_room2.poll_events()
		await get_tree().process_frame
	assert_not_null(state.remote_track, "Room2 should subscribe to the audio track")
	if state.remote_track == null:
		return

	var outbound := _find(await _stats_of(local_track), "outbound_rtp")
	var inbound := _find(await _stats_of(state.remote_track), "inbound_rtp")
	assert_eq(
		[outbound.get("kind"), typeof(outbound.get("packets_sent")), outbound.get("packets_sent", 0) > 0,
			inbound.get("kind"), typeof(inbound.get("packets_received")), inbound.get("packets_received", 0) > 0],
		["audio", TYPE_INT, true, "audio", TYPE_INT, true],
	)


## Requests track's stats and waits for them, returning [] if they don't arrive.
func _stats_of(track: LiveKitTrack) -> Array:
	var received := []
	track.stats_received.connect(func(stats): received.append(stats), CONNECT_ONE_SHOT)
	track.request_stats()
	var deadline := Time.get_ticks_msec() + 5000
	while received.is_empty() and Time.get_ticks_msec() < deadline:
		await get_tree().process_frame
	return received[0] if received else []


## Returns the first stats entry of the given type, or {}.
func _find(stats: Array, type: String) -> Dictionary:
	for entry in stats:
		if entry.get("type") == type:
			return entry
	return {}
