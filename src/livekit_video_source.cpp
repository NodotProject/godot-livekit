#include "livekit_video_source.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void LiveKitVideoSource::_bind_methods() {
    ClassDB::bind_static_method("LiveKitVideoSource", D_METHOD("create", "width", "height"), &LiveKitVideoSource::create);
    ClassDB::bind_method(D_METHOD("capture_frame", "image", "timestamp_us", "rotation"), &LiveKitVideoSource::capture_frame, DEFVAL(0), DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_width"), &LiveKitVideoSource::get_width);
    ClassDB::bind_method(D_METHOD("get_height"), &LiveKitVideoSource::get_height);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "width"), "", "get_width");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "height"), "", "get_height");
}

LiveKitVideoSource::LiveKitVideoSource() {
}

LiveKitVideoSource::~LiveKitVideoSource() {
}

Ref<LiveKitVideoSource> LiveKitVideoSource::create(int width, int height) {
    auto native_source = std::make_shared<livekit::VideoSource>(width, height);

    Ref<LiveKitVideoSource> source;
    source.instantiate();
    source->source_ = native_source;
    source->width_ = width;
    source->height_ = height;
    return source;
}

void LiveKitVideoSource::capture_frame(const Ref<Image> &image, int64_t timestamp_us, int rotation) {
    if (!source_ || image.is_null()) {
        return;
    }

    // Ensure image is in RGBA8 format
    Ref<Image> rgba_image = image;
    if (image->get_format() != Image::FORMAT_RGBA8) {
        rgba_image = image->duplicate();
        rgba_image->convert(Image::FORMAT_RGBA8);
    }

    int w = rgba_image->get_width();
    int h = rgba_image->get_height();
    PackedByteArray data = rgba_image->get_data();

    std::vector<uint8_t> frame_data(data.ptr(), data.ptr() + data.size());
    livekit::VideoFrame frame(w, h, livekit::VideoBufferType::RGBA, std::move(frame_data));

    livekit::VideoRotation rot = livekit::VideoRotation::VIDEO_ROTATION_0;
    switch (rotation) {
        case 90: rot = livekit::VideoRotation::VIDEO_ROTATION_90; break;
        case 180: rot = livekit::VideoRotation::VIDEO_ROTATION_180; break;
        case 270: rot = livekit::VideoRotation::VIDEO_ROTATION_270; break;
    }

    source_->captureFrame(frame, timestamp_us, rot);
}

int LiveKitVideoSource::get_width() const {
    return width_;
}

int LiveKitVideoSource::get_height() const {
    return height_;
}
