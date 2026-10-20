#include "livekit_participant.h"
#include "livekit_room.h"
#include "livekit_track.h"
#include "livekit_track_publication.h"

#include <gdextension_interface.h>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

// Web builds implement the classes available in browsers on top of livekit-client. The rest
// (e.g. screen capture) aren't registered, so ClassDB.class_exists() reports them as missing.

static void livekit_frame_callback() {
    LiveKitRoom::poll_all();
}

void initialize_livekit_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    // Room
    ClassDB::register_class<LiveKitRoom>();

    // Participants
    ClassDB::register_class<LiveKitParticipant>();
    ClassDB::register_class<LiveKitLocalParticipant>();
    ClassDB::register_class<LiveKitRemoteParticipant>();

    // Tracks
    ClassDB::register_class<LiveKitTrack>();
    ClassDB::register_class<LiveKitRemoteAudioTrack>();
    ClassDB::register_class<LiveKitRemoteVideoTrack>();

    // Track Publications
    ClassDB::register_class<LiveKitTrackPublication>();
    ClassDB::register_class<LiveKitLocalTrackPublication>();
    ClassDB::register_class<LiveKitRemoteTrackPublication>();
}

void uninitialize_livekit_module(ModuleInitializationLevel p_level) {
}

extern "C" {
GDExtensionBool GDE_EXPORT livekit_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

    init_obj.register_initializer(initialize_livekit_module);
    init_obj.register_terminator(uninitialize_livekit_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    init_obj.register_frame_callback(livekit_frame_callback);

    return init_obj.init();
}
}
