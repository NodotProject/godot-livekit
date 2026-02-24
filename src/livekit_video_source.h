#ifndef GODOT_LIVEKIT_VIDEO_SOURCE_H
#define GODOT_LIVEKIT_VIDEO_SOURCE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <livekit/video_source.h>
#include <livekit/video_frame.h>

#include <memory>

namespace godot {

class LiveKitVideoSource : public RefCounted {
    GDCLASS(LiveKitVideoSource, RefCounted)

private:
    std::shared_ptr<livekit::VideoSource> source_;
    int width_ = 0;
    int height_ = 0;

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
