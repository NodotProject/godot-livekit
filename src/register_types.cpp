#include "register_types.h"
#include "livekit_room.h"
#include "livekit_participant.h"
#include "livekit_track.h"
#include "livekit_track_publication.h"
#include "livekit_video_stream.h"
#include "livekit_audio_stream.h"
#include "livekit_video_source.h"
#include "livekit_audio_source.h"
#ifdef LIVEKIT_E2EE_SUPPORTED
#include "livekit_e2ee.h"
#endif

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

    // Room
    ClassDB::register_class<LiveKitRoom>();

    // Participants
    ClassDB::register_class<LiveKitParticipant>();
    ClassDB::register_class<LiveKitLocalParticipant>();
    ClassDB::register_class<LiveKitRemoteParticipant>();

    // Tracks
    ClassDB::register_class<LiveKitTrack>();
    ClassDB::register_class<LiveKitLocalAudioTrack>();
    ClassDB::register_class<LiveKitLocalVideoTrack>();
    ClassDB::register_class<LiveKitRemoteAudioTrack>();
    ClassDB::register_class<LiveKitRemoteVideoTrack>();

    // Track Publications
    ClassDB::register_class<LiveKitTrackPublication>();
    ClassDB::register_class<LiveKitLocalTrackPublication>();
    ClassDB::register_class<LiveKitRemoteTrackPublication>();

    // Streams
    ClassDB::register_class<LiveKitVideoStream>();
    ClassDB::register_class<LiveKitAudioStream>();

    // Sources
    ClassDB::register_class<LiveKitVideoSource>();
    ClassDB::register_class<LiveKitAudioSource>();

#ifdef LIVEKIT_E2EE_SUPPORTED
    // E2EE
    ClassDB::register_class<LiveKitE2eeOptions>();
    ClassDB::register_class<LiveKitKeyProvider>();
    ClassDB::register_class<LiveKitFrameCryptor>();
    ClassDB::register_class<LiveKitE2eeManager>();
#endif
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
