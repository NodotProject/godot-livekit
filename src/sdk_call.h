#pragma once

#include <godot_cpp/variant/utility_functions.hpp>

#include <exception>

/// Calls into the LiveKit SDK, reporting its exceptions with push_error() and returning false
/// instead. (It throws when the room has disconnected or is disconnecting.)
template <typename F>
bool call_sdk(const char *method, F &&call) {
    try {
        call();
        return true;
    } catch (const std::exception &e) {
        godot::UtilityFunctions::push_error(method, ": ", godot::String(e.what()));
        return false;
    }
}
