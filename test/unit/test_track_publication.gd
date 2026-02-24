extends GutTest
## Tests LiveKitTrackPublication hierarchy, unbound defaults.


func test_create_track_publication():
	var pub := LiveKitTrackPublication.new()
	assert_not_null(pub, "TrackPublication should be created")


func test_unbound_publication_sid_empty():
	var pub := LiveKitTrackPublication.new()
	assert_eq(pub.get_sid(), "",
		"Unbound publication SID should be empty")


func test_unbound_publication_name_empty():
	var pub := LiveKitTrackPublication.new()
	assert_eq(pub.get_name(), "",
		"Unbound publication name should be empty")


func test_unbound_publication_kind_zero():
	var pub := LiveKitTrackPublication.new()
	assert_eq(pub.get_kind(), 0,
		"Unbound publication kind should be 0")


func test_unbound_publication_source_zero():
	var pub := LiveKitTrackPublication.new()
	assert_eq(pub.get_source(), 0,
		"Unbound publication source should be 0")


func test_unbound_publication_not_muted():
	var pub := LiveKitTrackPublication.new()
	assert_false(pub.get_muted(),
		"Unbound publication should not be muted")


func test_unbound_publication_mime_type_empty():
	var pub := LiveKitTrackPublication.new()
	assert_eq(pub.get_mime_type(), "",
		"Unbound publication mime_type should be empty")


func test_unbound_publication_not_simulcasted():
	var pub := LiveKitTrackPublication.new()
	assert_false(pub.get_simulcasted(),
		"Unbound publication should not be simulcasted")


func test_unbound_publication_track_null():
	var pub := LiveKitTrackPublication.new()
	assert_null(pub.get_track(),
		"Unbound publication track should be null")


func test_local_track_publication_inheritance():
	var pub := LiveKitLocalTrackPublication.new()
	assert_true(pub is LiveKitTrackPublication,
		"LocalTrackPublication should inherit LiveKitTrackPublication")


func test_remote_track_publication_inheritance():
	var pub := LiveKitRemoteTrackPublication.new()
	assert_true(pub is LiveKitTrackPublication,
		"RemoteTrackPublication should inherit LiveKitTrackPublication")


func test_unbound_remote_publication_subscribed():
	var pub := LiveKitRemoteTrackPublication.new()
	assert_false(pub.get_subscribed(),
		"Unbound remote publication subscribed should be false")


func test_unbound_remote_publication_set_subscribed():
	var pub := LiveKitRemoteTrackPublication.new()
	# Should not crash — null guard checks publication_ pointer
	pub.set_subscribed(true)
	assert_true(true, "set_subscribed on unbound publication should not crash")
