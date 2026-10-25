#include "livekit_video_source.h"

#include "web_bridge.h"
#include "web_video.h"

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

LiveKitVideoSource::~LiveKitVideoSource() {
    if (js_id_) {
        web_call("video_source_destroy", Array::make(js_id_));
    }
}

Ref<LiveKitVideoSource> LiveKitVideoSource::create(int width, int height) {
    Ref<LiveKitVideoSource> source;
    source.instantiate();
    source->width_ = width;
    source->height_ = height;
    source->js_id_ = web_call("video_source_create", Array::make(width, height));
    return source;
}

void LiveKitVideoSource::capture_frame(const Ref<Image> &image, int64_t timestamp_us, int rotation) {
    if (!js_id_ || image.is_null()) {
        UtilityFunctions::push_error("LiveKitVideoSource::capture_frame: source not initialized or image is null");
        return;
    }
    Ref<Image> rgba_image = image;
    if (image->get_format() != Image::FORMAT_RGBA8) {
        rgba_image = image->duplicate();
        // convert() can't change a compressed format, so it would leave the buffer as it was.
        if (rgba_image->is_compressed()) {
            rgba_image->decompress();
        }
        rgba_image->convert(Image::FORMAT_RGBA8);
    }
    int w = rgba_image->get_width();
    int h = rgba_image->get_height();
    if (w <= 0 || h <= 0) {
        return;
    }
    PackedByteArray data = rgba_image->get_data();
    // The bridge reads w * h * 4 bytes, so anything shorter would expose other heap memory.
    if (rgba_image->get_format() != Image::FORMAT_RGBA8 || data.size() < int64_t(w) * h * 4) {
        UtilityFunctions::push_error("LiveKitVideoSource::capture_frame: image can't be converted to RGBA8");
        return;
    }
    web_video_source_capture(js_id_, data.ptr(), w, h, rotation);
}

int LiveKitVideoSource::get_width() const {
    return width_;
}

int LiveKitVideoSource::get_height() const {
    return height_;
}
