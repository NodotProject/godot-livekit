extends GutTest
## Tests LiveKitRoom property defaults, enum stability, and safe operations
## on unconnected rooms.


func test_connection_state_initially_disconnected():
	var room := LiveKitRoom.new()
	assert_eq(room.get_connection_state(), LiveKitRoom.STATE_DISCONNECTED,
		"New room should be in STATE_DISCONNECTED")


func test_sid_empty_before_connection():
	var room := LiveKitRoom.new()
	assert_eq(room.get_sid(), "",
		"SID should be empty before connection")


func test_name_empty_before_connection():
	var room := LiveKitRoom.new()
	assert_eq(room.get_name(), "",
		"Room name should be empty before connection")


func test_metadata_empty_before_connection():
	var room := LiveKitRoom.new()
	assert_eq(room.get_metadata(), "",
		"Metadata should be empty before connection")


func test_connection_state_enum_values():
	assert_eq(LiveKitRoom.STATE_DISCONNECTED, 0,
		"STATE_DISCONNECTED should be 0")
	assert_eq(LiveKitRoom.STATE_CONNECTED, 1,
		"STATE_CONNECTED should be 1")
	assert_eq(LiveKitRoom.STATE_RECONNECTING, 2,
		"STATE_RECONNECTING should be 2")


func test_poll_events_on_unconnected_room():
	var room := LiveKitRoom.new()
	# Should not crash or error
	room.poll_events()
	assert_true(true, "poll_events on unconnected room should not crash")


func test_connect_returns_true():
	var room := LiveKitRoom.new()
	var result = room.connect_to_room("ws://127.0.0.1:1", "t", {})
	assert_true(result, "connect_to_room should return true (async connect started)")
	room.disconnect_from_room()
	# Let background threads finish so the destructor doesn't block the
	# main thread during GUT's paint cycle (causes segfault).
	await wait_seconds(0.5)


func test_connect_with_empty_options():
	var room := LiveKitRoom.new()
	var result = room.connect_to_room("ws://127.0.0.1:1", "t", {})
	assert_true(result, "connect with empty options should return true")
	room.disconnect_from_room()
	await wait_seconds(0.5)


func test_connect_with_auto_subscribe_option():
	var room := LiveKitRoom.new()
	var result = room.connect_to_room("ws://127.0.0.1:1", "t", {"auto_subscribe": false})
	assert_true(result, "connect with auto_subscribe option should be accepted")
	room.disconnect_from_room()
	await wait_seconds(0.5)


func test_connect_with_auto_reconnect_false():
	var room := LiveKitRoom.new()
	var result = room.connect_to_room("ws://127.0.0.1:1", "t", {"auto_reconnect": false})
	assert_true(result, "connect with auto_reconnect false should be accepted")
	room.disconnect_from_room()
	await wait_seconds(0.5)


func test_room_is_ref_counted():
	var room := LiveKitRoom.new()
	assert_true(room is RefCounted, "LiveKitRoom should be RefCounted")
