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
    // Signal the capture thread to stop and wait for it.
    running_.store(false);
    frame_cv_.notify_all();
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
}

Ref<LiveKitVideoSource> LiveKitVideoSource::create(int width, int height) {
    auto native_source = std::make_shared<livekit::VideoSource>(width, height);

    Ref<LiveKitVideoSource> source;
    source.instantiate();
    source->source_ = native_source;
    source->width_ = width;
    source->height_ = height;

    // Start the background capture thread.
    source->running_.store(true);
    source->capture_thread_ = std::thread(&LiveKitVideoSource::capture_loop, source.ptr());

    return source;
}

void LiveKitVideoSource::capture_frame(const Ref<Image> &image, int64_t timestamp_us, int rotation) {
    if (!source_ || image.is_null()) {
        UtilityFunctions::push_error("LiveKitVideoSource::capture_frame: source not initialized or image is null");
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

    livekit::VideoRotation rot = livekit::VideoRotation::VIDEO_ROTATION_0;
    switch (rotation) {
        case 90: rot = livekit::VideoRotation::VIDEO_ROTATION_90; break;
        case 180: rot = livekit::VideoRotation::VIDEO_ROTATION_180; break;
        case 270: rot = livekit::VideoRotation::VIDEO_ROTATION_270; break;
    }

    // Copy pixel data into the pending slot (replaces any stale frame).
    {
        std::lock_guard<std::mutex> lock(frame_mutex_);
        pending_frame_.data.assign(data.ptr(), data.ptr() + data.size());
        pending_frame_.width = w;
        pending_frame_.height = h;
        pending_frame_.timestamp_us = timestamp_us;
        pending_frame_.rotation = rot;
        pending_frame_.valid = true;
    }
    frame_cv_.notify_one();
}

void LiveKitVideoSource::capture_loop() {
    while (running_.load()) {
        PendingFrame frame;

        // Wait for a pending frame or shutdown signal.
        {
            std::unique_lock<std::mutex> lock(frame_mutex_);
            frame_cv_.wait(lock, [this]() {
                return pending_frame_.valid || !running_.load();
            });
            if (!running_.load()) {
                break;
            }
            // Move the frame out of the pending slot.
            frame = std::move(pending_frame_);
            pending_frame_.valid = false;
            pending_frame_.data.clear();
        }

        if (!frame.valid || frame.data.empty() || !source_) {
            continue;
        }

        // This call may block (SDK encoder backpressure) — that's fine,
        // we're on a background thread.  The main thread stays responsive
        // and can overwrite pending_frame_ with newer data.
        livekit::VideoFrame vf(frame.width, frame.height,
                               livekit::VideoBufferType::RGBA,
                               std::move(frame.data));
        source_->captureFrame(vf, frame.timestamp_us, frame.rotation);
    }
}

int LiveKitVideoSource::get_width() const {
    return width_;
}

int LiveKitVideoSource::get_height() const {
    return height_;
}
