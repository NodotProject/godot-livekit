#ifndef GODOT_LIVEKIT_VIDEO_STREAM_H
#define GODOT_LIVEKIT_VIDEO_STREAM_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <livekit/video_stream.h>
#include <livekit/video_frame.h>

#include "livekit_track.h"
#include "livekit_participant.h"

#include <memory>
#include <mutex>
#include <atomic>

#include "detachable_thread.h"

namespace godot {

class LiveKitVideoStream : public RefCounted {
    GDCLASS(LiveKitVideoStream, RefCounted)

private:
    std::shared_ptr<livekit::VideoStream> stream_;
    Ref<ImageTexture> texture_;
    std::mutex frame_mutex_;
    std::unique_ptr<livekit::VideoFrame> pending_frame_;
    DetachableThread reader_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> thread_started_{false};
    std::atomic<uint32_t> lock_contention_count_{0};

    // Shared sentinel: stays valid even if `this` is freed after detach.
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);

    // Cached frame buffers — reused across poll() calls to avoid per-frame allocations.
    Ref<Image> cached_image_;
    PackedByteArray cached_pba_;
    int last_width_{0};
    int last_height_{0};

    // Auto-poll: when true, the poller calls poll() every frame automatically.
    bool auto_poll_{true};

    void _reader_loop();
    void _ensure_reader_started();

protected:
    static void _bind_methods();

public:
    LiveKitVideoStream();
    ~LiveKitVideoStream();

    static Ref<LiveKitVideoStream> from_track(const Ref<LiveKitTrack> &track);
    static Ref<LiveKitVideoStream> from_participant(const Ref<LiveKitRemoteParticipant> &participant, int source);

    Ref<ImageTexture> get_texture() const;
    bool poll();
    void close();

    void set_auto_poll(bool enabled);
    bool get_auto_poll() const;
};

}

#endif // GODOT_LIVEKIT_VIDEO_STREAM_H
