#ifndef GODOT_LIVEKIT_TRACK_PUBLICATION_H
#define GODOT_LIVEKIT_TRACK_PUBLICATION_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

#include <livekit/track_publication.h>
#include <livekit/local_track_publication.h>
#include <livekit/remote_track_publication.h>

#include "livekit_track.h"

#include <memory>

namespace godot {

class LiveKitTrackPublication : public RefCounted {
    GDCLASS(LiveKitTrackPublication, RefCounted)

protected:
    static void _bind_methods();
    std::shared_ptr<livekit::TrackPublication> publication_;

public:
    LiveKitTrackPublication();
    ~LiveKitTrackPublication();

    void bind_publication(const std::shared_ptr<livekit::TrackPublication> &pub);
    std::shared_ptr<livekit::TrackPublication> get_native_publication() const { return publication_; }

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
    static void _bind_methods();

public:
    LiveKitLocalTrackPublication();
    ~LiveKitLocalTrackPublication();
};

class LiveKitRemoteTrackPublication : public LiveKitTrackPublication {
    GDCLASS(LiveKitRemoteTrackPublication, LiveKitTrackPublication)

protected:
    static void _bind_methods();

public:
    LiveKitRemoteTrackPublication();
    ~LiveKitRemoteTrackPublication();

    bool get_subscribed() const;
    void set_subscribed(bool subscribed);
};

}

#endif // GODOT_LIVEKIT_TRACK_PUBLICATION_H
