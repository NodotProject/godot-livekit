extends GutTest
## Verifies all registered classes are loaded via ClassDB.class_exists().


func test_all_classes_registered():
	# All 17 classes registered in register_types.cpp
	var classes := [
		# Room
		"LiveKitRoom",
		# Participants
		"LiveKitParticipant",
		"LiveKitLocalParticipant",
		"LiveKitRemoteParticipant",
		# Tracks
		"LiveKitTrack",
		"LiveKitLocalAudioTrack",
		"LiveKitLocalVideoTrack",
		"LiveKitRemoteAudioTrack",
		"LiveKitRemoteVideoTrack",
		# Track Publications
		"LiveKitTrackPublication",
		"LiveKitLocalTrackPublication",
		"LiveKitRemoteTrackPublication",
		# Streams
		"LiveKitVideoStream",
		"LiveKitAudioStream",
		# Sources
		"LiveKitVideoSource",
		"LiveKitAudioSource",
		# Screen Capture
		"LiveKitScreenCapture",
	]

	for cls in classes:
		assert_true(ClassDB.class_exists(cls),
			"Class '%s' should be registered in ClassDB" % cls)


func test_e2ee_classes_platform_info():
	# E2EE classes are conditionally compiled (LIVEKIT_E2EE_SUPPORTED)
	var e2ee_classes := [
		"LiveKitE2eeOptions",
		"LiveKitKeyProvider",
		"LiveKitFrameCryptor",
		"LiveKitE2eeManager",
	]

	var any_found := false
	for cls in e2ee_classes:
		if ClassDB.class_exists(cls):
			any_found = true
			break

	if any_found:
		gut.p("E2EE classes are available on this platform/build")
		for cls in e2ee_classes:
			assert_true(ClassDB.class_exists(cls),
				"E2EE class '%s' should be registered when E2EE is supported" % cls)
	else:
		gut.p("E2EE classes are NOT available on this platform/build (informational)")
		pass_test("E2EE not available — skipped")
