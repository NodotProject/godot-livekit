#ifndef GODOT_LIVEKIT_VIDEO_SOURCE_H
#define GODOT_LIVEKIT_VIDEO_SOURCE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <livekit/video_source.h>
#include <livekit/video_frame.h>

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace godot {

class LiveKitVideoSource : public RefCounted {
    GDCLASS(LiveKitVideoSource, RefCounted)

private:
    std::shared_ptr<livekit::VideoSource> source_;
    int width_ = 0;
    int height_ = 0;

    // --- Async capture thread ---
    // capture_frame() copies pixel data into pending_frame_ and wakes the
    // worker.  The worker calls the blocking SDK captureFrame() off the
    // main thread, so Godot never stalls.
    struct PendingFrame {
        std::vector<uint8_t> data;
        int width = 0;
        int height = 0;
        int64_t timestamp_us = 0;
        livekit::VideoRotation rotation = livekit::VideoRotation::VIDEO_ROTATION_0;
        bool valid = false;
    };

    std::mutex frame_mutex_;
    std::condition_variable frame_cv_;
    PendingFrame pending_frame_;
    std::atomic<bool> running_{false};
    std::thread capture_thread_;

    void capture_loop();

protected:
    static void _bind_methods();

public:
    LiveKitVideoSource();
    ~LiveKitVideoSource();

    static Ref<LiveKitVideoSource> create(int width, int height);

    void capture_frame(const Ref<Image> &image, int64_t timestamp_us, int rotation);

    int get_width() const;
    int get_height() const;

    std::shared_ptr<livekit::VideoSource> get_native_source() const { return source_; }
};

}

#endif // GODOT_LIVEKIT_VIDEO_SOURCE_H
