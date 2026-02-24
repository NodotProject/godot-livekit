extends GutTest
## Tests LiveKitParticipant, LiveKitLocalParticipant, LiveKitRemoteParticipant
## property defaults and null-guard safety on unbound objects.


func test_participant_kind_enum_values():
	assert_eq(LiveKitParticipant.KIND_STANDARD, 0, "KIND_STANDARD should be 0")
	assert_eq(LiveKitParticipant.KIND_INGRESS, 1, "KIND_INGRESS should be 1")
	assert_eq(LiveKitParticipant.KIND_EGRESS, 2, "KIND_EGRESS should be 2")
	assert_eq(LiveKitParticipant.KIND_SIP, 3, "KIND_SIP should be 3")
	assert_eq(LiveKitParticipant.KIND_AGENT, 4, "KIND_AGENT should be 4")


func test_unbound_participant_identity_empty():
	var p := LiveKitParticipant.new()
	assert_eq(p.get_identity(), "",
		"Unbound participant identity should be empty")


func test_unbound_participant_name_empty():
	var p := LiveKitParticipant.new()
	assert_eq(p.get_name(), "",
		"Unbound participant name should be empty")


func test_unbound_participant_metadata_empty():
	var p := LiveKitParticipant.new()
	assert_eq(p.get_metadata(), "",
		"Unbound participant metadata should be empty")


func test_unbound_participant_attributes_empty():
	var p := LiveKitParticipant.new()
	assert_eq(p.get_attributes().size(), 0,
		"Unbound participant attributes should be empty")


func test_unbound_participant_kind_is_standard():
	var p := LiveKitParticipant.new()
	assert_eq(p.get_kind(), LiveKitParticipant.KIND_STANDARD,
		"Unbound participant kind should be KIND_STANDARD")


func test_local_participant_inheritance():
	var lp := LiveKitLocalParticipant.new()
	assert_true(lp is LiveKitParticipant,
		"LocalParticipant should inherit LiveKitParticipant")
	assert_true(lp is RefCounted,
		"LocalParticipant should be RefCounted")


func test_remote_participant_inheritance():
	var rp := LiveKitRemoteParticipant.new()
	assert_true(rp is LiveKitParticipant,
		"RemoteParticipant should inherit LiveKitParticipant")
	assert_true(rp is RefCounted,
		"RemoteParticipant should be RefCounted")


func test_local_participant_track_publications_empty():
	var lp := LiveKitLocalParticipant.new()
	assert_eq(lp.get_track_publications().size(), 0,
		"Unbound local participant track publications should be empty")


func test_remote_participant_track_publications_empty():
	var rp := LiveKitRemoteParticipant.new()
	assert_eq(rp.get_track_publications().size(), 0,
		"Unbound remote participant track publications should be empty")


func test_unbound_local_publish_data_no_crash():
	var lp := LiveKitLocalParticipant.new()
	var data := PackedByteArray([1, 2, 3])
	# Should not crash — null guard prints error and returns
	lp.publish_data(data, true, PackedStringArray(), "")
	assert_true(true, "publish_data on unbound participant should not crash")


func test_unbound_local_set_metadata_no_crash():
	var lp := LiveKitLocalParticipant.new()
	# Should not crash — null guard returns early
	lp.set_metadata("test")
	assert_true(true, "set_metadata on unbound participant should not crash")
