extends Node
## Runs GUT inside a web export, where there's no command line. The tests to run come from the
## page URL (`?tests=res://test/unit/test_room.gd,...`), as can env vars read by the test helpers.
## Prints `GUT_WEB_RESULT tests=N failing=N pending=N` when done, for test_web.sh to parse.

const GutConfig := preload("res://addons/gut/gut_config.gd")
const GutRunner := preload("res://addons/gut/gui/GutRunner.tscn")


func _ready() -> void:
	var tests := str(JavaScriptBridge.eval("new URLSearchParams(location.search).get('tests') || ''")).split(",", false)
	var config := GutConfig.new()
	config.options.tests = Array(tests)
	config.options.should_exit = false
	config.options.log_level = 1
	var runner := GutRunner.instantiate()
	runner.set_gut_config(config)
	add_child(runner)
	var gut = runner.get_gut()
	gut.end_run.connect(func() -> void:
		print("GUT_WEB_RESULT tests=%d failing=%d pending=%d" % [
			gut.get_test_count(), gut.get_fail_count(), gut.get_pending_count()
		])
	)
	runner.run_tests(false)
