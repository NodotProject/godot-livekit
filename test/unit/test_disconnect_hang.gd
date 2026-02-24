extends GutTest
## Tests that disconnect and room destruction never block the main thread.
##
## The key hang scenarios:
##   1. disconnect_from_room() while Connect() is still in progress
##      (old code joined connect_thread_ on the main thread)
##   2. Room freed shortly after disconnect_from_room()
##      (old code joined disconnect_thread_ in the destructor)
##   3. Double disconnect_from_room() call
##      (old code joined the first disconnect_thread_ on the main thread)

const FRAME_BUDGET_MS := 500  # generous — any of these should take < 1 frame


func test_disconnect_unconnected_room_does_not_block():
	var before := Time.get_ticks_msec()
	var room := LiveKitRoom.new()
	room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"disconnect on an unconnected room should return instantly (took %d ms)" % elapsed)


func test_disconnect_then_free_does_not_block():
	var room := LiveKitRoom.new()
	room.disconnect_from_room()
	# Dropping the last Ref triggers ~LiveKitRoom.  With the old code this
	# joined disconnect_thread_ and blocked for seconds.
	var before := Time.get_ticks_msec()
	room = null  # prevent GUT from holding onto the Ref
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"freeing the room after disconnect should not block (took %d ms)" % elapsed)


func test_double_disconnect_does_not_block():
	var room := LiveKitRoom.new()
	room.disconnect_from_room()
	var before := Time.get_ticks_msec()
	room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"second disconnect should not block on the first (took %d ms)" % elapsed)


func test_disconnect_while_connecting_does_not_block():
	var room := LiveKitRoom.new()
	# Kick off a connection to an unreachable address.  Connect() runs on a
	# background thread and may take several seconds to time out.
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})

	# With the old code this joined the connect thread on the main thread,
	# blocking until Connect() returned or timed out.
	var before := Time.get_ticks_msec()
	room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"disconnect while connecting should not block the main thread (took %d ms)" % elapsed)


func test_free_while_connecting_does_not_block():
	var room := LiveKitRoom.new()
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})

	# Drop the Ref immediately.  The destructor must not block waiting for
	# the connect thread.
	var before := Time.get_ticks_msec()
	room = null
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"freeing a room mid-connect should not block (took %d ms)" % elapsed)


func test_disconnect_connect_disconnect_does_not_block():
	var room := LiveKitRoom.new()
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})
	room.disconnect_from_room()
	room.connect_to_room("ws://127.0.0.1:1", "invalid-token", {})

	var before := Time.get_ticks_msec()
	room.disconnect_from_room()
	var elapsed := Time.get_ticks_msec() - before
	assert_lt(elapsed, FRAME_BUDGET_MS,
		"disconnect-connect-disconnect cycle should not block (took %d ms)" % elapsed)
