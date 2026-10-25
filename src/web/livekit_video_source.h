#ifndef GODOT_LIVEKIT_VIDEO_SOURCE_H
#define GODOT_LIVEKIT_VIDEO_SOURCE_H

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {

// Web implementation of LiveKitVideoSource: captured frames are drawn onto a canvas whose
// captureStream() track is published. Rotation is applied to the pixels, since browsers can't
// send it as metadata, and timestamps are the browser's.
class LiveKitVideoSource : public RefCounted {
    GDCLASS(LiveKitVideoSource, RefCounted)

private:
    int js_id_ = 0;
    int width_ = 0;
    int height_ = 0;

protected:
    static void _bind_methods();

public:
    ~LiveKitVideoSource();

    static Ref<LiveKitVideoSource> create(int width, int height);

    void capture_frame(const Ref<Image> &image, int64_t timestamp_us, int rotation);

    int get_width() const;
    int get_height() const;

    int get_js_id() const { return js_id_; }
};

}

#endif // GODOT_LIVEKIT_VIDEO_SOURCE_H
