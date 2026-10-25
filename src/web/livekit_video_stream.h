#ifndef GODOT_LIVEKIT_VIDEO_STREAM_H
#define GODOT_LIVEKIT_VIDEO_STREAM_H

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "livekit_participant.h"
#include "livekit_track.h"

namespace godot {

// Web implementation of LiveKitVideoStream: a remote track plays in a hidden video element, and
// poll() copies its newest frame (via a canvas) into the texture.
class LiveKitVideoStream : public RefCounted {
    GDCLASS(LiveKitVideoStream, RefCounted)

private:
    int js_id_ = 0;
    Ref<ImageTexture> texture_;
    Ref<Image> cached_image_;
    PackedByteArray cached_pba_;
    int last_width_ = 0;
    int last_height_ = 0;
    bool auto_poll_ = true;

    static Ref<LiveKitVideoStream> _create(const Variant &p_js_id);
    void _register();
    void _unregister();

protected:
    static void _bind_methods();

public:
    ~LiveKitVideoStream();

    static Ref<LiveKitVideoStream> from_track(const Ref<LiveKitTrack> &track);
    static Ref<LiveKitVideoStream> from_participant(const Ref<LiveKitRemoteParticipant> &participant, int source);

    Ref<ImageTexture> get_texture() const;
    bool poll();
    void close();

    void set_auto_poll(bool enabled);
    bool get_auto_poll() const;

    // Polls every stream with auto_poll enabled. Called once per frame.
    static void poll_all();
};

}

#endif // GODOT_LIVEKIT_VIDEO_STREAM_H
