extends GutTest
## Base class for integration tests that require a LiveKit server.
## Reads LIVEKIT_TEST_URL and LIVEKIT_TOKEN_* from environment variables.
## Tests are skipped gracefully when no server is available.

var _livekit_url: String = ""
var _token_1: String = ""
var _token_2: String = ""
var _room: LiveKitRoom = null


func _get_env(key: String, default_value: String = "") -> String:
	var value = OS.get_environment(key)
	if value == null or value == "":
		return default_value
	return value


func _is_server_available() -> bool:
	return _livekit_url != ""


func _skip_if_no_server() -> bool:
	if not _is_server_available():
		pending("LIVEKIT_TEST_URL not set — skipping integration test")
		return true
	return false


func before_all():
	_livekit_url = _get_env("LIVEKIT_TEST_URL")
	_token_1 = _get_env("LIVEKIT_TOKEN_1")
	_token_2 = _get_env("LIVEKIT_TOKEN_2")

	if _livekit_url == "":
		gut.p("LIVEKIT_TEST_URL not set — integration tests will be skipped")
	else:
		gut.p("LiveKit server: %s" % _livekit_url)


func before_each():
	if _is_server_available():
		_room = LiveKitRoom.new()


func after_each():
	if _room != null:
		_room.disconnect_from_room()
		_room = null


## Poll the room until a condition is met or timeout is reached.
## Returns true if condition was met, false if timed out.
func _poll_until(room: LiveKitRoom, condition: Callable, timeout_sec: float = 5.0) -> bool:
	var deadline := Time.get_ticks_msec() + int(timeout_sec * 1000)
	while Time.get_ticks_msec() < deadline:
		room.poll_events()
		if condition.call():
			return true
		OS.delay_msec(50)
	return false
