extends GutTest
## Base class for end-to-end tests targeting the production LiveKit server.
## Uses the LiveKit devkey (devkey/secret) to generate join tokens.
## URL defaults to wss://livekit.daccord.gg (overridable via LIVEKIT_E2E_URL).
## Tests are skipped when RUN_E2E env var is not set.

var _livekit_url: String = ""
var _api_key: String = ""
var _api_secret: String = ""
var _room_name: String = ""
var _token_1: String = ""
var _token_2: String = ""
var _room: LiveKitRoom = null
var _room2: LiveKitRoom = null
var _e2e_enabled: bool = false


func _get_env(key: String, default_value: String = "") -> String:
	var value = OS.get_environment(key)
	if value == null or value == "":
		return default_value
	return value


func _is_server_available() -> bool:
	return _e2e_enabled


func _skip_if_no_server() -> bool:
	if not _is_server_available():
		pending("RUN_E2E not set — skipping e2e test")
		return true
	return false


# -- JWT generation (HS256) --------------------------------------------------

func _base64url_encode(data: PackedByteArray) -> String:
	var b64 = Marshalls.raw_to_base64(data)
	b64 = b64.replace("+", "-").replace("/", "_")
	while b64.ends_with("="):
		b64 = b64.left(-1)
	return b64


func _generate_token(identity: String, room: String) -> String:
	var header := {"alg": "HS256", "typ": "JWT"}
	var now := int(Time.get_unix_time_from_system())
	var payload := {
		"iss": _api_key,
		"sub": identity,
		"iat": now,
		"nbf": now,
		"exp": now + 3600,
		"video": {
			"room": room,
			"roomJoin": true,
			"canPublish": true,
			"canSubscribe": true,
			"canPublishData": true,
		},
	}

	var header_b64 := _base64url_encode(JSON.stringify(header).to_utf8_buffer())
	var payload_b64 := _base64url_encode(JSON.stringify(payload).to_utf8_buffer())
	var signing_input := header_b64 + "." + payload_b64

	var crypto := Crypto.new()
	var signature := crypto.hmac_digest(
		HashingContext.HASH_SHA256,
		_api_secret.to_utf8_buffer(),
		signing_input.to_utf8_buffer(),
	)

	return signing_input + "." + _base64url_encode(signature)


# -- Lifecycle ----------------------------------------------------------------

func before_all():
	_e2e_enabled = _get_env("RUN_E2E") != ""
	_livekit_url = _get_env("LIVEKIT_E2E_URL", "wss://livekit.daccord.gg")
	_api_key = _get_env("LIVEKIT_API_KEY", "devkey")
	_api_secret = _get_env("LIVEKIT_API_SECRET", "secret")
	_room_name = _get_env("LIVEKIT_E2E_ROOM", "e2e-test-room")

	if not _is_server_available():
		gut.p("RUN_E2E not set — e2e tests will be skipped")
	else:
		_token_1 = _generate_token("e2e-user-1", _room_name)
		_token_2 = _generate_token("e2e-user-2", _room_name)
		gut.p("E2E LiveKit server: %s  room: %s" % [_livekit_url, _room_name])


func before_each():
	if _is_server_available():
		_room = LiveKitRoom.new()
		_room2 = LiveKitRoom.new()


func after_each():
	if _room2 != null:
		_room2.disconnect_from_room()
		_room2 = null
	if _room != null:
		_room.disconnect_from_room()
		_room = null


# -- Polling helpers ----------------------------------------------------------

## Poll a single room until a condition is met or timeout is reached.
## Returns true if condition was met, false if timed out.
func _poll_until(room: LiveKitRoom, condition: Callable, timeout_sec: float = 10.0) -> bool:
	var deadline := Time.get_ticks_msec() + int(timeout_sec * 1000)
	while Time.get_ticks_msec() < deadline:
		room.poll_events()
		if condition.call():
			return true
		OS.delay_msec(50)
	return false


## Poll both rooms until a condition is met or timeout is reached.
## Useful when both rooms need to process events for the condition to be satisfied.
func _poll_both(condition: Callable, timeout_sec: float = 10.0) -> bool:
	var deadline := Time.get_ticks_msec() + int(timeout_sec * 1000)
	while Time.get_ticks_msec() < deadline:
		_room.poll_events()
		_room2.poll_events()
		if condition.call():
			return true
		OS.delay_msec(50)
	return false


## Connect both rooms and wait for them to see each other.
## Returns true if both connected and discovered each other.
func _connect_both_rooms() -> bool:
	_room.connect_to_room(_livekit_url, _token_1, {})
	var r1 = _poll_until(_room, func():
		return _room.get_connection_state() == LiveKitRoom.STATE_CONNECTED, 15.0)
	if not r1:
		return false

	_room2.connect_to_room(_livekit_url, _token_2, {})
	var r2 = _poll_until(_room2, func():
		return _room2.get_connection_state() == LiveKitRoom.STATE_CONNECTED, 15.0)
	if not r2:
		return false

	# Wait for both to see each other
	var mutual = _poll_both(func():
		return _room.get_remote_participants().size() >= 1 \
			and _room2.get_remote_participants().size() >= 1, 15.0)
	return mutual
