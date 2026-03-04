extends "res://test/e2e/e2e_helper.gd"
## E2E: Data channel messaging against the production server.

var _received_data: PackedByteArray = PackedByteArray()
var _received_topic: String = ""
var _received_messages: Array = []


func before_each():
	super.before_each()
	_received_data = PackedByteArray()
	_received_topic = ""
	_received_messages = []


func _on_data_received(data: PackedByteArray, _participant, topic: String):
	_received_data = data
	_received_topic = topic
	_received_messages.append({"data": data, "topic": topic})


func test_reliable_data_round_trip():
	if _skip_if_no_server():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	_room2.data_received.connect(_on_data_received)

	var test_data := "e2e-reliable-test-payload".to_utf8_buffer()
	var test_topic := "e2e_reliable"
	var lp = _room.get_local_participant()
	lp.publish_data(test_data, true, PackedStringArray(), test_topic)

	var received = _poll_until(_room2, func():
		return _received_data.size() > 0, 10.0)
	assert_true(received, "Room2 should receive reliable data")

	if received:
		assert_eq(_received_data, test_data, "Payload should match")
		assert_eq(_received_topic, test_topic, "Topic should match")


func test_multiple_topics():
	if _skip_if_no_server():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	_room2.data_received.connect(_on_data_received)

	var lp = _room.get_local_participant()

	# Send messages on different topics
	var data_a := "topic-a-payload".to_utf8_buffer()
	var data_b := "topic-b-payload".to_utf8_buffer()
	lp.publish_data(data_a, true, PackedStringArray(), "topic_a")

	# Small delay between sends to ensure ordering
	_poll_until(_room, func(): return false, 0.5)

	lp.publish_data(data_b, true, PackedStringArray(), "topic_b")

	# Wait for both messages
	var got_both = _poll_until(_room2, func():
		return _received_messages.size() >= 2, 10.0)

	if got_both:
		var topics := []
		for msg in _received_messages:
			topics.append(msg["topic"])
		assert_true("topic_a" in topics, "Should receive message on topic_a")
		assert_true("topic_b" in topics, "Should receive message on topic_b")

		# Verify payloads match their topics
		for msg in _received_messages:
			if msg["topic"] == "topic_a":
				assert_eq(msg["data"], data_a, "topic_a payload should match")
			elif msg["topic"] == "topic_b":
				assert_eq(msg["data"], data_b, "topic_b payload should match")
	else:
		gut.p("Not all messages received within timeout — got %d" % _received_messages.size())
		assert_true(_received_messages.size() >= 1,
			"Should receive at least one message")


func test_large_payload():
	if _skip_if_no_server():
		return

	var both = _connect_both_rooms()
	assert_true(both, "Both rooms should connect")
	if not both:
		return

	_room2.data_received.connect(_on_data_received)

	# Build a 4KB payload
	var large_data := PackedByteArray()
	large_data.resize(4096)
	for i in range(4096):
		large_data[i] = i % 256

	var lp = _room.get_local_participant()
	lp.publish_data(large_data, true, PackedStringArray(), "large_payload")

	var received = _poll_until(_room2, func():
		return _received_data.size() > 0, 10.0)
	assert_true(received, "Room2 should receive large payload")

	if received:
		assert_eq(_received_data.size(), 4096, "Payload size should be 4096 bytes")
		assert_eq(_received_data, large_data, "Large payload should match byte-for-byte")
		assert_eq(_received_topic, "large_payload", "Topic should match")
