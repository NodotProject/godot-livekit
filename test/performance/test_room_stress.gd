extends GutTest
## Stress tests for room creation, connection, and destruction.


func test_rapid_room_creation():
	var before := Time.get_ticks_msec()
	for i in range(100):
		var room := LiveKitRoom.new()
		room = null
	var elapsed := Time.get_ticks_msec() - before
	gut.p("100 room create/free cycles in %d ms" % elapsed)
	assert_lt(elapsed, 5000,
		"100 create/free cycles should complete in < 5s (took %d ms)" % elapsed)


func test_rapid_connect_disconnect_stress():
	var room := LiveKitRoom.new()
	var before := Time.get_ticks_msec()
	for i in range(50):
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	gut.p("50 connect/disconnect cycles in %d ms" % elapsed)
	assert_true(true,
		"50 connect/disconnect stress cycles completed without crash (%d ms)" % elapsed)


func test_many_rooms_simultaneous():
	var rooms := []
	var before := Time.get_ticks_msec()
	# Create and connect 20 rooms
	for i in range(20):
		var room := LiveKitRoom.new()
		room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
		rooms.append(room)
	# Disconnect all
	for room in rooms:
		room.disconnect_from_room()
	rooms.clear()
	var elapsed := Time.get_ticks_msec() - before
	gut.p("20 simultaneous rooms connect+disconnect in %d ms" % elapsed)
	assert_true(true,
		"20 simultaneous rooms completed without crash (%d ms)" % elapsed)
