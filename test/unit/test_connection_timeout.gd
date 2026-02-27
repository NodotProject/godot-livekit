extends GutTest
## Tests for the connection timeout and connection_failed signal.

const FRAME_BUDGET_MS := 500

# Use 192.0.2.1 (TEST-NET, RFC 5737) — guaranteed unroutable, so Connect()
# will hang until the timeout fires rather than failing instantly.
const UNROUTABLE := "ws://192.0.2.1:1"


func test_connection_failed_signal_exists():
	var room := LiveKitRoom.new()
	assert_true(room.has_signal("connection_failed"),
		"LiveKitRoom should have a connection_failed signal")


func test_connect_timeout_option_accepted():
	var room := LiveKitRoom.new()
	var result = room.connect_to_room("ws://127.0.0.1:1", "t", {"connect_timeout": 5.0})
	assert_true(result, "connect with connect_timeout option should return true")
	room.disconnect_from_room()


func test_connect_timeout_zero_disables():
	var room := LiveKitRoom.new()
	var result = room.connect_to_room("ws://127.0.0.1:1", "t", {"connect_timeout": 0.0})
	assert_true(result, "connect with zero timeout (disabled) should return true")
	room.disconnect_from_room()


func test_timeout_emits_connection_failed():
	var room := LiveKitRoom.new()
	# Use an Array so the lambda can mutate shared state.
	var state := [false, ""]  # [signal_received, error_msg]
	room.connection_failed.connect(func(err: String):
		state[0] = true
		state[1] = err
	)
	room.connect_to_room(UNROUTABLE, "invalid-token", {"connect_timeout": 1.0})

	# Poll until the timeout fires or we give up after 3 seconds.
	var deadline := Time.get_ticks_msec() + 3000
	while Time.get_ticks_msec() < deadline and not state[0]:
		room.poll_events()
		OS.delay_msec(50)

	assert_true(state[0], "connection_failed should fire after timeout")
	assert_string_contains(state[1], "timed out",
		"Error message should mention 'timed out'")
	assert_eq(room.get_connection_state(), LiveKitRoom.STATE_DISCONNECTED,
		"State should be DISCONNECTED after timeout")
	room.disconnect_from_room()


func test_timeout_does_not_fire_when_disconnected_first():
	var room := LiveKitRoom.new()
	var state := [false]  # [signal_received]
	room.connection_failed.connect(func(_err: String):
		state[0] = true
	)
	room.connect_to_room(UNROUTABLE, "invalid-token", {"connect_timeout": 1.0})
	# Disconnect immediately before the timeout fires.
	room.disconnect_from_room()

	# Poll for a bit past the original timeout window.
	var deadline := Time.get_ticks_msec() + 2000
	while Time.get_ticks_msec() < deadline:
		room.poll_events()
		OS.delay_msec(50)

	assert_false(state[0],
		"connection_failed should not fire after explicit disconnect")


func test_poll_events_after_timeout_does_not_double_emit():
	var room := LiveKitRoom.new()
	var state := [0]  # [emit_count]
	room.connection_failed.connect(func(_err: String):
		state[0] += 1
	)
	room.connect_to_room(UNROUTABLE, "invalid-token", {"connect_timeout": 1.0})

	# Wait for the timeout to fire.
	var deadline := Time.get_ticks_msec() + 3000
	while Time.get_ticks_msec() < deadline and state[0] == 0:
		room.poll_events()
		OS.delay_msec(50)
	assert_eq(state[0], 1, "Should have received exactly one connection_failed")

	# Keep polling — the late _finalize_connection should be suppressed.
	for i in range(20):
		room.poll_events()
		OS.delay_msec(50)

	assert_eq(state[0], 1,
		"connection_failed should not fire again after timeout already handled it")
	room.disconnect_from_room()


func test_disconnect_does_not_block_after_timeout():
	var room := LiveKitRoom.new()
	var state := [false]  # [timed_out]
	room.connection_failed.connect(func(_err: String):
		state[0] = true
	)
	room.connect_to_room(UNROUTABLE, "invalid-token", {"connect_timeout": 1.0})

	# Wait for the timeout.
	var deadline := Time.get_ticks_msec() + 3000
	while Time.get_ticks_msec() < deadline and not state[0]:
		room.poll_events()
		OS.delay_msec(50)
	assert_true(state[0], "Should have timed out")

	var before := Time.get_ticks_msec()
	room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"disconnect after timeout should not block (took %d ms)" % elapsed)
