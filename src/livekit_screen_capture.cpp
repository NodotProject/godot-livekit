#include "livekit_screen_capture.h"
#include "livekit_poller.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void LiveKitScreenCapture::_bind_methods() {
    // Static query methods
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("get_monitors"), &LiveKitScreenCapture::get_monitors);
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("get_windows"), &LiveKitScreenCapture::get_windows);
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("check_permissions"), &LiveKitScreenCapture::check_permissions);

    // Factory methods
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("create"), &LiveKitScreenCapture::create);
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("create_for_monitor", "monitor"), &LiveKitScreenCapture::create_for_monitor);
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("create_for_window", "window"), &LiveKitScreenCapture::create_for_window);

    // Capture lifecycle
    ClassDB::bind_method(D_METHOD("start"), &LiveKitScreenCapture::start);
    ClassDB::bind_method(D_METHOD("stop"), &LiveKitScreenCapture::stop);
    ClassDB::bind_method(D_METHOD("pause"), &LiveKitScreenCapture::pause);
    ClassDB::bind_method(D_METHOD("resume"), &LiveKitScreenCapture::resume);
    ClassDB::bind_method(D_METHOD("is_paused"), &LiveKitScreenCapture::is_paused);

    // Frame access
    ClassDB::bind_method(D_METHOD("poll"), &LiveKitScreenCapture::poll);
    ClassDB::bind_method(D_METHOD("get_texture"), &LiveKitScreenCapture::get_texture);
    ClassDB::bind_method(D_METHOD("get_image"), &LiveKitScreenCapture::get_image);

    // One-shot
    ClassDB::bind_method(D_METHOD("screenshot"), &LiveKitScreenCapture::screenshot);

    // Cleanup
    ClassDB::bind_method(D_METHOD("close"), &LiveKitScreenCapture::close);

    // Auto-poll
    ClassDB::bind_method(D_METHOD("set_auto_poll", "enabled"), &LiveKitScreenCapture::set_auto_poll);
    ClassDB::bind_method(D_METHOD("get_auto_poll"), &LiveKitScreenCapture::get_auto_poll);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_poll"), "set_auto_poll", "get_auto_poll");

    // Signal
    ADD_SIGNAL(MethodInfo("frame_received"));

    // Enum
    BIND_ENUM_CONSTANT(PERMISSION_OK);
    BIND_ENUM_CONSTANT(PERMISSION_WARNING);
    BIND_ENUM_CONSTANT(PERMISSION_ERROR);
}

LiveKitScreenCapture::LiveKitScreenCapture() {
}

LiveKitScreenCapture::~LiveKitScreenCapture() {
    LiveKitPoller::instance().unregister_screen_capture(this);
    alive_->store(false);
    close();
}

// --- Static query methods ---

Array LiveKitScreenCapture::get_monitors() {
    Array result;
    try {
        auto monitors = frametap::get_monitors();
        for (const auto &m : monitors) {
            Dictionary d;
            d["id"] = m.id;
            d["name"] = String(m.name.c_str());
            d["x"] = m.x;
            d["y"] = m.y;
            d["width"] = m.width;
            d["height"] = m.height;
            d["scale"] = m.scale;
            result.push_back(d);
        }
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::get_monitors: ", e.what());
    }
    return result;
}

Array LiveKitScreenCapture::get_windows() {
    Array result;
    try {
        auto windows = frametap::get_windows();
        for (const auto &w : windows) {
            Dictionary d;
            d["id"] = (int64_t)w.id;
            d["name"] = String(w.name.c_str());
            d["x"] = w.x;
            d["y"] = w.y;
            d["width"] = w.width;
            d["height"] = w.height;
            result.push_back(d);
        }
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::get_windows: ", e.what());
    }
    return result;
}

Dictionary LiveKitScreenCapture::check_permissions() {
    Dictionary result;
    try {
        auto check = frametap::check_permissions();
        int status = 0;
        switch (check.status) {
            case frametap::PermissionStatus::ok:      status = PERMISSION_OK; break;
            case frametap::PermissionStatus::warning:  status = PERMISSION_WARNING; break;
            case frametap::PermissionStatus::error:    status = PERMISSION_ERROR; break;
        }
        result["status"] = status;
        result["summary"] = String(check.summary.c_str());
        Array details;
        for (const auto &d : check.details) {
            details.push_back(String(d.c_str()));
        }
        result["details"] = details;
    } catch (const frametap::CaptureError &e) {
        result["status"] = (int)PERMISSION_ERROR;
        result["summary"] = String(e.what());
        result["details"] = Array();
    }
    return result;
}

// --- Factory methods ---

Ref<LiveKitScreenCapture> LiveKitScreenCapture::create() {
    try {
        Ref<LiveKitScreenCapture> capture;
        capture.instantiate();
        capture->tap_ = std::make_unique<frametap::FrameTap>();
        capture->texture_.instantiate();
        LiveKitPoller::instance().register_screen_capture(capture.ptr(), capture->alive_);
        return capture;
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::create: ", e.what());
        return Ref<LiveKitScreenCapture>();
    }
}

Ref<LiveKitScreenCapture> LiveKitScreenCapture::create_for_monitor(const Dictionary &monitor_dict) {
    if (!monitor_dict.has("id")) {
        UtilityFunctions::push_error("LiveKitScreenCapture::create_for_monitor: dictionary missing 'id' key");
        return Ref<LiveKitScreenCapture>();
    }

    try {
        frametap::Monitor monitor;
        monitor.id = (int)monitor_dict.get("id", 0);
        if (monitor_dict.has("name")) {
            monitor.name = String(monitor_dict.get("name", "")).utf8().get_data();
        }
        monitor.x = (int)monitor_dict.get("x", 0);
        monitor.y = (int)monitor_dict.get("y", 0);
        monitor.width = (int)monitor_dict.get("width", 0);
        monitor.height = (int)monitor_dict.get("height", 0);
        monitor.scale = (float)monitor_dict.get("scale", 1.0f);

        Ref<LiveKitScreenCapture> capture;
        capture.instantiate();
        capture->tap_ = std::make_unique<frametap::FrameTap>(monitor);
        capture->texture_.instantiate();
        LiveKitPoller::instance().register_screen_capture(capture.ptr(), capture->alive_);
        return capture;
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::create_for_monitor: ", e.what());
        return Ref<LiveKitScreenCapture>();
    }
}

Ref<LiveKitScreenCapture> LiveKitScreenCapture::create_for_window(const Dictionary &window_dict) {
    if (!window_dict.has("id")) {
        UtilityFunctions::push_error("LiveKitScreenCapture::create_for_window: dictionary missing 'id' key");
        return Ref<LiveKitScreenCapture>();
    }

    try {
        frametap::Window window;
        window.id = (uint64_t)(int64_t)window_dict.get("id", 0);
        if (window_dict.has("name")) {
            window.name = String(window_dict.get("name", "")).utf8().get_data();
        }
        window.x = (int)window_dict.get("x", 0);
        window.y = (int)window_dict.get("y", 0);
        window.width = (int)window_dict.get("width", 0);
        window.height = (int)window_dict.get("height", 0);

        Ref<LiveKitScreenCapture> capture;
        capture.instantiate();
        capture->tap_ = std::make_unique<frametap::FrameTap>(window);
        capture->texture_.instantiate();
        LiveKitPoller::instance().register_screen_capture(capture.ptr(), capture->alive_);
        return capture;
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::create_for_window: ", e.what());
        return Ref<LiveKitScreenCapture>();
    }
}

// --- Capture lifecycle ---

void LiveKitScreenCapture::start() {
    if (!tap_) {
        UtilityFunctions::push_error("LiveKitScreenCapture::start: not initialized (use create() factory)");
        return;
    }
    if (capturing_.load()) {
        return;
    }

    try {
        tap_->on_frame([this](const frametap::Frame &frame) {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            const auto &img = frame.image;
            pending_frame_.data.assign(img.data.begin(), img.data.end());
            pending_frame_.width = img.width;
            pending_frame_.height = img.height;
            has_pending_.store(true);
        });
        tap_->start_async();
        capturing_.store(true);
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::start: ", e.what());
    }
}

void LiveKitScreenCapture::stop() {
    if (!tap_ || !capturing_.load()) {
        return;
    }

    try {
        tap_->stop();
        capturing_.store(false);
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::stop: ", e.what());
    }
}

void LiveKitScreenCapture::pause() {
    if (!tap_) {
        return;
    }
    try {
        tap_->pause();
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::pause: ", e.what());
    }
}

void LiveKitScreenCapture::resume() {
    if (!tap_) {
        return;
    }
    try {
        tap_->resume();
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::resume: ", e.what());
    }
}

bool LiveKitScreenCapture::is_paused() const {
    if (!tap_) {
        return false;
    }
    try {
        return tap_->is_paused();
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::is_paused: ", e.what());
        return false;
    }
}

// --- Frame access ---

bool LiveKitScreenCapture::poll() {
    if (!has_pending_.load()) {
        return false;
    }

    PendingFrame frame;

    {
        std::unique_lock<std::mutex> lock(frame_mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return false;
        }
        frame = std::move(pending_frame_);
        pending_frame_.data.clear();
        pending_frame_.width = 0;
        pending_frame_.height = 0;
        has_pending_.store(false);
    }

    if (frame.data.empty() || frame.width == 0 || frame.height == 0) {
        return false;
    }

    int width = (int)frame.width;
    int height = (int)frame.height;

    if (frame.width == last_width_ && frame.height == last_height_ && latest_image_.is_valid()) {
        // Fast path: reuse existing buffers — no allocation.
        memcpy(cached_pba_.ptrw(), frame.data.data(), frame.data.size());
        latest_image_->set_data(width, height, false, Image::FORMAT_RGBA8, cached_pba_);
        if (texture_.is_valid()) {
            texture_->update(latest_image_);
        }
    } else {
        // Slow path: resolution changed or first frame — allocate.
        cached_pba_.resize(frame.data.size());
        memcpy(cached_pba_.ptrw(), frame.data.data(), frame.data.size());

        latest_image_ = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, cached_pba_);
        if (latest_image_.is_null()) {
            return false;
        }

        if (texture_.is_valid()) {
            texture_->set_image(latest_image_);
        }

        last_width_ = frame.width;
        last_height_ = frame.height;
    }

    emit_signal("frame_received");
    return true;
}

Ref<ImageTexture> LiveKitScreenCapture::get_texture() const {
    return texture_;
}

Ref<Image> LiveKitScreenCapture::get_image() const {
    return latest_image_;
}

// --- One-shot capture ---

Ref<Image> LiveKitScreenCapture::screenshot() {
    if (!tap_) {
        UtilityFunctions::push_error("LiveKitScreenCapture::screenshot: not initialized (use create() factory)");
        return Ref<Image>();
    }

    try {
        auto img_data = tap_->screenshot();
        if (img_data.data.empty() || img_data.width == 0 || img_data.height == 0) {
            return Ref<Image>();
        }

        PackedByteArray pba;
        pba.resize(img_data.data.size());
        memcpy(pba.ptrw(), img_data.data.data(), img_data.data.size());

        return Image::create_from_data(
            (int)img_data.width, (int)img_data.height, false, Image::FORMAT_RGBA8, pba);
    } catch (const frametap::CaptureError &e) {
        UtilityFunctions::push_error("LiveKitScreenCapture::screenshot: ", e.what());
        return Ref<Image>();
    }
}

// --- Cleanup ---

void LiveKitScreenCapture::close() {
    stop();
    tap_.reset();
}

void LiveKitScreenCapture::set_auto_poll(bool enabled) {
    if (auto_poll_ == enabled) {
        return;
    }
    auto_poll_ = enabled;
    if (enabled) {
        LiveKitPoller::instance().register_screen_capture(this, alive_);
    } else {
        LiveKitPoller::instance().unregister_screen_capture(this);
    }
}

bool LiveKitScreenCapture::get_auto_poll() const {
    return auto_poll_;
}
