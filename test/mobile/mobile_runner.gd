extends Node
## Runs GUT inside an Android or iOS export, where GUT's command line runner isn't available.
## test_android.sh and test_ios.sh pass the tests to run, and env vars read by the test helpers, as
## user arguments (`tests=res://test/unit/test_room.gd,... RUN_E2E=1 ...`).
## Prints `GUT_MOBILE_STARTED` once it's running and, when done, `GUT_MOBILE_FAILED_SCRIPT <path>` for
## each script with failures and then `GUT_MOBILE_RESULT tests=N failing=N pending=N`, for the scripts
## to parse, and quits (which iOS ignores).

const GutConfig := preload("res://addons/gut/gut_config.gd")
const GutRunner := preload("res://addons/gut/gui/GutRunner.tscn")


func _ready() -> void:
	print("GUT_MOBILE_STARTED")
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
		# Lets the scripts tell which tests failed, e.g. to treat some failures as advisory.
		for script in gut.get_test_collector().scripts:
			if script.get_fail_count() > 0:
				print("GUT_MOBILE_FAILED_SCRIPT %s" % script.path)
		print("GUT_MOBILE_RESULT tests=%d failing=%d pending=%d" % [
			gut.get_test_count(), gut.get_fail_count(), gut.get_pending_count()
		])
		# On Android, quitting flushes the log file and tells test_android.sh the run is over.
		get_tree().quit()
	)
	runner.run_tests(false)
