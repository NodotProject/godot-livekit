extends GutTest
## Tests E2EE classes. Runtime-gated via ClassDB.class_exists().
## Uses GUT pending() to skip on non-E2EE builds.


func _skip_if_no_e2ee() -> bool:
	if not ClassDB.class_exists("LiveKitE2eeOptions"):
		pending("E2EE not available on this platform/build — skipping")
		return true
	return false


func test_e2ee_encryption_type_enum():
	if _skip_if_no_e2ee():
		return
	var opts = ClassDB.instantiate("LiveKitE2eeOptions")
	assert_eq(LiveKitE2eeOptions.ENCRYPTION_NONE, 0, "ENCRYPTION_NONE should be 0")
	assert_eq(LiveKitE2eeOptions.ENCRYPTION_GCM, 1, "ENCRYPTION_GCM should be 1")
	assert_eq(LiveKitE2eeOptions.ENCRYPTION_CUSTOM, 2, "ENCRYPTION_CUSTOM should be 2")


func test_e2ee_options_defaults():
	if _skip_if_no_e2ee():
		return
	var opts = ClassDB.instantiate("LiveKitE2eeOptions")
	assert_eq(opts.get_encryption_type(), LiveKitE2eeOptions.ENCRYPTION_GCM,
		"Default encryption type should be GCM")
	assert_eq(opts.get_ratchet_window_size(), 16,
		"Default ratchet window size should be 16")
	assert_eq(opts.get_failure_tolerance(), -1,
		"Default failure tolerance should be -1")


func test_e2ee_options_set_shared_key():
	if _skip_if_no_e2ee():
		return
	var opts = ClassDB.instantiate("LiveKitE2eeOptions")
	var key := PackedByteArray([1, 2, 3, 4, 5])
	opts.set_shared_key(key)
	assert_eq(opts.get_shared_key(), key,
		"Shared key should roundtrip through set/get")


func test_e2ee_options_set_ratchet_salt():
	if _skip_if_no_e2ee():
		return
	var opts = ClassDB.instantiate("LiveKitE2eeOptions")
	var salt := PackedByteArray([10, 20, 30])
	opts.set_ratchet_salt(salt)
	assert_eq(opts.get_ratchet_salt(), salt,
		"Ratchet salt should roundtrip through set/get")


func test_e2ee_options_ratchet_window_size():
	if _skip_if_no_e2ee():
		return
	var opts = ClassDB.instantiate("LiveKitE2eeOptions")
	opts.set_ratchet_window_size(32)
	assert_eq(opts.get_ratchet_window_size(), 32,
		"Ratchet window size should roundtrip through set/get")


func test_e2ee_options_failure_tolerance():
	if _skip_if_no_e2ee():
		return
	var opts = ClassDB.instantiate("LiveKitE2eeOptions")
	opts.set_failure_tolerance(5)
	assert_eq(opts.get_failure_tolerance(), 5,
		"Failure tolerance should roundtrip through set/get")


func test_key_provider_unbound():
	if _skip_if_no_e2ee():
		return
	var kp = ClassDB.instantiate("LiveKitKeyProvider")
	# Should not crash on unbound provider
	kp.set_shared_key(PackedByteArray([1, 2, 3]), 0)
	var key = kp.get_shared_key(0)
	assert_eq(key.size(), 0,
		"Unbound key provider should return empty key")
	assert_push_error(2)


func test_frame_cryptor_unbound_defaults():
	if _skip_if_no_e2ee():
		return
	var fc = ClassDB.instantiate("LiveKitFrameCryptor")
	assert_eq(fc.get_participant_identity(), "",
		"Unbound frame cryptor identity should be empty")
	assert_eq(fc.get_key_index(), 0,
		"Unbound frame cryptor key_index should be 0")
	assert_false(fc.get_enabled(),
		"Unbound frame cryptor should not be enabled")


func test_frame_cryptor_set_enabled_unbound():
	if _skip_if_no_e2ee():
		return
	var fc = ClassDB.instantiate("LiveKitFrameCryptor")
	# Should not crash
	fc.set_enabled(true)
	assert_push_error("not bound")


func test_e2ee_manager_unbound_not_enabled():
	if _skip_if_no_e2ee():
		return
	var mgr = ClassDB.instantiate("LiveKitE2eeManager")
	assert_false(mgr.get_enabled(),
		"Unbound E2EE manager should not be enabled")


func test_e2ee_manager_unbound_frame_cryptors_empty():
	if _skip_if_no_e2ee():
		return
	var mgr = ClassDB.instantiate("LiveKitE2eeManager")
	assert_eq(mgr.get_frame_cryptors().size(), 0,
		"Unbound E2EE manager should have no frame cryptors")
