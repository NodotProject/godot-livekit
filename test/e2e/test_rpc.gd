extends "res://test/e2e/e2e_helper.gd"
## E2E: RPC calls between two participants.


func test_rpc_reaches_a_registered_method_and_the_caller_gets_an_empty_response():
	if _skip_if_no_server():
		return
	assert_true(await _connect_both_rooms(), "Both rooms should connect")
	var invoked := []
	var responses := []
	var errors := []
	_room2.get_local_participant().rpc_method_invoked.connect(
		func(method, request_id, caller_identity, payload, response_timeout):
			invoked.append([method, request_id != "", caller_identity, payload, response_timeout > 0.0])
	)
	_room2.get_local_participant().register_rpc_method("greet")
	var caller := _room.get_local_participant()
	caller.rpc_response_received.connect(func(method, result): responses.append([method, result]))
	caller.rpc_error.connect(func(method, message): errors.append([method, message]))

	caller.perform_rpc("e2e-user-2", "greet", "hello", 5.0)
	await _poll_both(func(): return responses.size() + errors.size() > 0 and invoked.size() > 0, 10.0)

	# The binding can't reply from a handler, so the caller always gets an empty response.
	assert_eq(
		[invoked, responses, errors],
		[[["greet", true, "e2e-user-1", "hello", true]], [["greet", ""]], []],
	)


func test_rpc_to_an_unregistered_method_reports_an_error():
	if _skip_if_no_server():
		return
	assert_true(await _connect_both_rooms(), "Both rooms should connect")
	var callee := _room2.get_local_participant()
	callee.register_rpc_method("greet")
	callee.unregister_rpc_method("greet")
	var caller := _room.get_local_participant()
	var outcomes := []
	caller.rpc_response_received.connect(func(method, result): outcomes.append(["response", method, result]))
	caller.rpc_error.connect(func(method, message): outcomes.append(["error", method, message]))

	caller.perform_rpc("e2e-user-2", "greet", "hello", 5.0)
	await _poll_both(func(): return outcomes.size() > 0, 10.0)

	assert_eq(outcomes, [["error", "greet", "Method not supported at destination"]])
