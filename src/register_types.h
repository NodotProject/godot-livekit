#ifndef LIVEKIT_REGISTER_TYPES_H
#define LIVEKIT_REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

using namespace godot;

void initialize_livekit_module(ModuleInitializationLevel p_level);
void uninitialize_livekit_module(ModuleInitializationLevel p_level);

#endif // LIVEKIT_REGISTER_TYPES_H