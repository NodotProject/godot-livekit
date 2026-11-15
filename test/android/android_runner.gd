extends Node
## Runs GUT inside an Android export, where tests can't be passed on a command line. test_android.sh
## bakes the tests to run, and env vars read by the test helpers, into the export as user arguments
## (`tests=res://test/unit/test_room.gd,... RUN_E2E=1 ...`).
## Prints `GUT_ANDROID_STARTED` once it's running and `GUT_ANDROID_RESULT tests=N failing=N pending=N`
## when done, for test_android.sh to parse, and quits.

const GutConfig := preload("res://addons/gut/gut_config.gd")
const GutRunner := preload("res://addons/gut/gui/GutRunner.tscn")


func _ready() -> void:
	print("GUT_ANDROID_STARTED")
	var tests := PackedStringArray()
	for arg in OS.get_cmdline_user_args():
		var parts := arg.split("=", true, 1)
		if parts.size() != 2:
			continue
		if parts[0] == "tests":
			tests = parts[1].split(",", false)
		else:
			OS.set_environment(parts[0], parts[1])
	var config := GutConfig.new()
	config.options.tests = Array(tests)
	config.options.should_exit = false
	config.options.log_level = 1
	var runner := GutRunner.instantiate()
	runner.set_gut_config(config)
	add_child(runner)
	var gut = runner.get_gut()
	gut.end_run.connect(func() -> void:
		print("GUT_ANDROID_RESULT tests=%d failing=%d pending=%d" % [
			gut.get_test_count(), gut.get_fail_count(), gut.get_pending_count()
		])
		# Quitting flushes the log file, and tells test_android.sh the run is over.
		get_tree().quit()
	)
	runner.run_tests(false)
