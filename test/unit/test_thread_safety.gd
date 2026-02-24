extends GutTest
## Thread safety tests extending the pattern from test_disconnect_hang.gd.

const FRAME_BUDGET_MS := 500


func test_rapid_connect_disconnect_cycles():
	var room := LiveKitRoom.new()
	var before := Time.get_ticks_msec()
	for i in range(10):
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"10 connect/disconnect cycles should complete within budget (took %d ms)" % elapsed)


func test_poll_events_during_connect():
	var room := LiveKitRoom.new()
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	var before := Time.get_ticks_msec()
	for i in range(10):
		room.poll_events()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"10 rapid poll_events during connect should not deadlock (took %d ms)" % elapsed)
	room.disconnect_from_room()


func test_multiple_rooms_parallel():
	var rooms := []
	var before := Time.get_ticks_msec()
	for i in range(5):
		var room := LiveKitRoom.new()
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		rooms.append(room)
	for room in rooms:
		room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"5 rooms connect+disconnect should complete within budget (took %d ms)" % elapsed)
	rooms.clear()


func test_disconnect_during_poll():
	var room := LiveKitRoom.new()
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	var before := Time.get_ticks_msec()
	room.poll_events()
	room.disconnect_from_room()
	room.poll_events()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"poll -> disconnect -> poll should be safe (took %d ms)" % elapsed)


func test_free_room_during_connect_no_leak():
	var before := Time.get_ticks_msec()
	for i in range(20):
		var room := LiveKitRoom.new()
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		room = null
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"20 create/connect/null cycles should not crash (took %d ms)" % elapsed)


func test_connect_after_disconnect_reuses_room():
	var room := LiveKitRoom.new()
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	room.disconnect_from_room()
	# Room should be reusable after disconnect
	var result = room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	assert_true(result, "Room should accept a new connection after disconnect")
	room.disconnect_from_room()
