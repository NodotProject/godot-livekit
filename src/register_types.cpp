#include "register_types.h"
#include "livekit_room.h"
#include "livekit_participant.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include <livekit/livekit.h>

using namespace godot;

void initialize_livekit_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    livekit::initialize();

    ClassDB::register_class<LiveKitRoom>();
    ClassDB::register_class<LiveKitParticipant>();
    ClassDB::register_class<LiveKitLocalParticipant>();
    ClassDB::register_class<LiveKitRemoteParticipant>();
}

void uninitialize_livekit_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    livekit::shutdown();
}

extern "C" {
GDExtensionBool GDE_EXPORT livekit_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_livekit_module);
    init_obj.register_terminator(uninitialize_livekit_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

    return init_obj.init();
}
}