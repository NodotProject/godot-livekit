#ifndef GODOT_LIVEKIT_TRACK_PUBLICATION_H
#define GODOT_LIVEKIT_TRACK_PUBLICATION_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include "livekit_track.h"

namespace godot {

// Web implementation of LiveKitTrackPublication, backed by a livekit-client publication.
class LiveKitTrackPublication : public RefCounted {
    GDCLASS(LiveKitTrackPublication, RefCounted)

protected:
    static void _bind_methods();

    int js_id_ = 0;
    bool remote_ = false;
    // The last known state, used once livekit-client has released the publication.
    mutable Dictionary info_;

    Dictionary _info() const;

public:
    // Returns a publication of the matching subclass for p_info (a bridge.js publicationInfo).
    static Ref<LiveKitTrackPublication> from_info(const Dictionary &p_info, bool p_remote);

    String get_sid() const;
    String get_name() const;
    int get_kind() const;
    int get_source() const;
    bool get_muted() const;
    String get_mime_type() const;
    bool get_simulcasted() const;

    Ref<LiveKitTrack> get_track() const;
};

class LiveKitLocalTrackPublication : public LiveKitTrackPublication {
    GDCLASS(LiveKitLocalTrackPublication, LiveKitTrackPublication)

protected:
    static void _bind_methods() {}
};

class LiveKitRemoteTrackPublication : public LiveKitTrackPublication {
    GDCLASS(LiveKitRemoteTrackPublication, LiveKitTrackPublication)

protected:
    static void _bind_methods();

public:
    bool get_subscribed() const;
    void set_subscribed(bool p_subscribed);
};

}

#endif // GODOT_LIVEKIT_TRACK_PUBLICATION_H
