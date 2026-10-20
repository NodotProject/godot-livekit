#ifndef GODOT_LIVEKIT_WEB_BRIDGE_H
#define GODOT_LIVEKIT_WEB_BRIDGE_H

#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

// Calls a function in bridge.js's `api` with JSON-serializable arguments, returning its result.
// Loads livekit-client and the bridge on first use. Errors are reported with push_error and
// return null.
Variant web_call(const char *p_function, const Array &p_args = Array());

// Returns a JSON object result as a Dictionary (empty if the call failed or returned null).
Dictionary web_call_dict(const char *p_function, const Array &p_args = Array());

}

#endif // GODOT_LIVEKIT_WEB_BRIDGE_H
