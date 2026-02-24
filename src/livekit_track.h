#ifndef GODOT_LIVEKIT_TRACK_H
#define GODOT_LIVEKIT_TRACK_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <livekit/track.h>
#include <livekit/local_audio_track.h>
#include <livekit/local_video_track.h>
#include <livekit/remote_audio_track.h>
#include <livekit/remote_video_track.h>

#include <memory>
#include <thread>

namespace godot {

class LiveKitAudioSource;
class LiveKitVideoSource;

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

protected:
    static void _bind_methods();
    std::shared_ptr<livekit::Track> track_;

public:
    LiveKitTrack();
    ~LiveKitTrack();

    void bind_track(const std::shared_ptr<livekit::Track> &track);
    std::shared_ptr<livekit::Track> get_native_track() const { return track_; }

    String get_sid() const;
    String get_name() const;
    int get_kind() const;
    int get_source() const;
    bool get_muted() const;
    int get_stream_state() const;

    void request_stats();
};

class LiveKitLocalAudioTrack : public LiveKitTrack {
    GDCLASS(LiveKitLocalAudioTrack, LiveKitTrack)

protected:
    static void _bind_methods();

public:
    LiveKitLocalAudioTrack();
    ~LiveKitLocalAudioTrack();

    static Ref<LiveKitLocalAudioTrack> create(const String &name, const Ref<LiveKitAudioSource> &source);
    void mute();
    void unmute();
};

class LiveKitLocalVideoTrack : public LiveKitTrack {
    GDCLASS(LiveKitLocalVideoTrack, LiveKitTrack)

protected:
    static void _bind_methods();

public:
    LiveKitLocalVideoTrack();
    ~LiveKitLocalVideoTrack();

    static Ref<LiveKitLocalVideoTrack> create(const String &name, const Ref<LiveKitVideoSource> &source);
    void mute();
    void unmute();
};

class LiveKitRemoteAudioTrack : public LiveKitTrack {
    GDCLASS(LiveKitRemoteAudioTrack, LiveKitTrack)

protected:
    static void _bind_methods();

public:
    LiveKitRemoteAudioTrack();
    ~LiveKitRemoteAudioTrack();
};

class LiveKitRemoteVideoTrack : public LiveKitTrack {
    GDCLASS(LiveKitRemoteVideoTrack, LiveKitTrack)

protected:
    static void _bind_methods();

public:
    LiveKitRemoteVideoTrack();
    ~LiveKitRemoteVideoTrack();
};

}

VARIANT_ENUM_CAST(LiveKitTrack::TrackKind);
VARIANT_ENUM_CAST(LiveKitTrack::TrackSource);
VARIANT_ENUM_CAST(LiveKitTrack::StreamState);

#endif // GODOT_LIVEKIT_TRACK_H
