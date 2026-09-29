#ifndef GODOT_LIVEKIT_TRACK_H
#define GODOT_LIVEKIT_TRACK_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

// Web implementation of LiveKitTrack, backed by a livekit-client track.
class LiveKitTrack : public RefCounted {
    GDCLASS(LiveKitTrack, RefCounted)

public:
    enum TrackKind {
        KIND_UNKNOWN = 0,
        KIND_AUDIO = 1,
        KIND_VIDEO = 2,
    };

    enum TrackSource {
        SOURCE_UNKNOWN = 0,
        SOURCE_CAMERA = 1,
        SOURCE_MICROPHONE = 2,
        SOURCE_SCREENSHARE = 3,
        SOURCE_SCREENSHARE_AUDIO = 4,
    };

    enum StreamState {
        STATE_UNKNOWN = 0,
        STATE_ACTIVE = 1,
        STATE_PAUSED = 2,
    };

    // Returns a track of the subclass matching p_info's kind (a bridge.js trackInfo).
    static Ref<LiveKitTrack> from_info(const Dictionary &p_info, bool p_remote);

protected:
    static void _bind_methods();

    int js_id_ = 0;
    // The last known state, used once livekit-client has released the track.
    mutable Dictionary info_;

    Dictionary _info() const;

public:
    int get_js_id() const { return js_id_; }

    String get_sid() const;
    String get_name() const;
    int get_kind() const;
    int get_source() const;
    bool get_muted() const;
    int get_stream_state() const;

    void request_stats();
};

class LiveKitRemoteAudioTrack : public LiveKitTrack {
    GDCLASS(LiveKitRemoteAudioTrack, LiveKitTrack)

protected:
    static void _bind_methods() {}
};

class LiveKitRemoteVideoTrack : public LiveKitTrack {
    GDCLASS(LiveKitRemoteVideoTrack, LiveKitTrack)

protected:
    static void _bind_methods() {}
};

}

VARIANT_ENUM_CAST(LiveKitTrack::TrackKind);
VARIANT_ENUM_CAST(LiveKitTrack::TrackSource);
VARIANT_ENUM_CAST(LiveKitTrack::StreamState);

#endif // GODOT_LIVEKIT_TRACK_H
