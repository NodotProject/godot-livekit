extends GutTest
## Tests all RPC methods on unbound LiveKitLocalParticipant don't crash.


func test_perform_rpc_unbound():
	var lp := LiveKitLocalParticipant.new()
	# Should not crash — null guard at livekit_participant.cpp prints error and returns
	lp.perform_rpc("dest", "method", "payload", 10.0)
	assert_push_error("not bound")


func test_register_rpc_method_unbound():
	var lp := LiveKitLocalParticipant.new()
	# Should not crash — null guard returns early
	lp.register_rpc_method("test_method")
	assert_push_error("not bound")


func test_unregister_rpc_method_unbound():
	var lp := LiveKitLocalParticipant.new()
	# Should not crash — null guard returns early
	lp.unregister_rpc_method("test_method")
	assert_push_error("not bound")


func test_publish_track_unbound():
	var lp := LiveKitLocalParticipant.new()
	var track := LiveKitTrack.new()
	# Should return null — null guard checks local_participant_ pointer
	var result = lp.publish_track(track, {})
	assert_null(result, "publish_track on unbound participant should return null")
	assert_push_error("invalid arguments")
