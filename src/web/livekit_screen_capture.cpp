#include "livekit_screen_capture.h"

#include "web_bridge.h"
#include "web_poll_list.h"
#include "web_video.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// Only captures made by a factory poll automatically, as natively.
static WebPollList<LiveKitScreenCapture> polled_captures;

void LiveKitScreenCapture::_bind_methods() {
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("get_monitors"), &LiveKitScreenCapture::get_monitors);
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("get_windows"), &LiveKitScreenCapture::get_windows);
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("check_permissions"), &LiveKitScreenCapture::check_permissions);

    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("create"), &LiveKitScreenCapture::create);
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("create_for_monitor", "monitor"), &LiveKitScreenCapture::create_for_monitor);
    ClassDB::bind_static_method("LiveKitScreenCapture", D_METHOD("create_for_window", "window"), &LiveKitScreenCapture::create_for_window);

    ClassDB::bind_method(D_METHOD("start"), &LiveKitScreenCapture::start);
    ClassDB::bind_method(D_METHOD("stop"), &LiveKitScreenCapture::stop);
    ClassDB::bind_method(D_METHOD("pause"), &LiveKitScreenCapture::pause);
    ClassDB::bind_method(D_METHOD("resume"), &LiveKitScreenCapture::resume);
    ClassDB::bind_method(D_METHOD("is_paused"), &LiveKitScreenCapture::is_paused);

    ClassDB::bind_method(D_METHOD("poll"), &LiveKitScreenCapture::poll);
    ClassDB::bind_method(D_METHOD("get_texture"), &LiveKitScreenCapture::get_texture);
    ClassDB::bind_method(D_METHOD("get_image"), &LiveKitScreenCapture::get_image);

    ClassDB::bind_method(D_METHOD("screenshot"), &LiveKitScreenCapture::screenshot);

    ClassDB::bind_method(D_METHOD("close"), &LiveKitScreenCapture::close);

    ClassDB::bind_method(D_METHOD("set_auto_poll", "enabled"), &LiveKitScreenCapture::set_auto_poll);
    ClassDB::bind_method(D_METHOD("get_auto_poll"), &LiveKitScreenCapture::get_auto_poll);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_poll"), "set_auto_poll", "get_auto_poll");

    ADD_SIGNAL(MethodInfo("frame_received"));

    BIND_ENUM_CONSTANT(PERMISSION_OK);
    BIND_ENUM_CONSTANT(PERMISSION_WARNING);
    BIND_ENUM_CONSTANT(PERMISSION_ERROR);
}

LiveKitScreenCapture::~LiveKitScreenCapture() {
    polled_captures.remove(this);
    close();
}

Array LiveKitScreenCapture::get_monitors() {
    return Array();
}

Array LiveKitScreenCapture::get_windows() {
    return Array();
}

Dictionary LiveKitScreenCapture::check_permissions() {
    Dictionary result;
    Array details;
    if (bool(web_call("screen_capture_supported"))) {
        // Not OK, since nothing is captured until the user agrees.
        result["status"] = (int)PERMISSION_WARNING;
        result["summary"] = String("The browser asks the user what to share when a capture starts");
        details.push_back(String("Call start() from an input event handler, since browsers may only allow it after a user gesture."));
        details.push_back(String("Browsers can't list or capture a chosen monitor or window."));
    } else {
        result["status"] = (int)PERMISSION_ERROR;
        result["summary"] = String("This browser doesn't support screen capture");
    }
    result["details"] = details;
    return result;
}

Ref<LiveKitScreenCapture> LiveKitScreenCapture::create() {
    if (!bool(web_call("screen_capture_supported"))) {
        UtilityFunctions::push_error("LiveKitScreenCapture::create: this browser doesn't support screen capture");
        return Ref<LiveKitScreenCapture>();
    }
    Variant js_id = web_call("screen_capture_create");
    if (js_id.get_type() == Variant::NIL) {
        return Ref<LiveKitScreenCapture>();
    }
    Ref<LiveKitScreenCapture> capture;
    capture.instantiate();
    capture->js_id_ = js_id;
    capture->texture_.instantiate();
    polled_captures.add(capture.ptr());
    return capture;
}

Ref<LiveKitScreenCapture> LiveKitScreenCapture::create_for_monitor(const Dictionary &monitor_dict) {
    if (!monitor_dict.has("id")) {
        UtilityFunctions::push_error("LiveKitScreenCapture::create_for_monitor: dictionary missing 'id' key");
    } else {
        UtilityFunctions::push_error("LiveKitScreenCapture::create_for_monitor: browsers can't capture a chosen monitor (use create(), which asks the user)");
    }
    return Ref<LiveKitScreenCapture>();
}

Ref<LiveKitScreenCapture> LiveKitScreenCapture::create_for_window(const Dictionary &window_dict) {
    if (!window_dict.has("id")) {
        UtilityFunctions::push_error("LiveKitScreenCapture::create_for_window: dictionary missing 'id' key");
    } else {
        UtilityFunctions::push_error("LiveKitScreenCapture::create_for_window: browsers can't capture a chosen window (use create(), which asks the user)");
    }
    return Ref<LiveKitScreenCapture>();
}

void LiveKitScreenCapture::start() {
    if (!js_id_) {
        UtilityFunctions::push_error("LiveKitScreenCapture::start: not initialized (use create() factory)");
        return;
    }
    web_call("screen_capture_start", Array::make(js_id_));
}

void LiveKitScreenCapture::stop() {
    if (js_id_) {
        web_call("screen_capture_stop", Array::make(js_id_));
    }
}

void LiveKitScreenCapture::pause() {
    if (js_id_) {
        paused_ = true;
        web_call("screen_capture_set_paused", Array::make(js_id_, true));
    }
}

void LiveKitScreenCapture::resume() {
    if (js_id_) {
        paused_ = false;
        web_call("screen_capture_set_paused", Array::make(js_id_, false));
    }
}

bool LiveKitScreenCapture::is_paused() const {
    return js_id_ && paused_;
}

bool LiveKitScreenCapture::poll() {
    if (!js_id_ || !web_video_stream_poll(js_id_, latest_image_, texture_, cached_pba_)) {
        return false;
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

Ref<Image> LiveKitScreenCapture::screenshot() {
    if (!js_id_) {
        UtilityFunctions::push_error("LiveKitScreenCapture::screenshot: not initialized (use create() factory)");
        return Ref<Image>();
    }
    poll();
    if (latest_image_.is_null()) {
        UtilityFunctions::push_error("LiveKitScreenCapture::screenshot: no frame has been captured yet (on the web, start() the capture first)");
        return Ref<Image>();
    }
    return latest_image_->duplicate();
}

void LiveKitScreenCapture::close() {
    if (js_id_) {
        web_call("screen_capture_close", Array::make(js_id_));
        js_id_ = 0;
    }
}

void LiveKitScreenCapture::set_auto_poll(bool enabled) {
    if (auto_poll_ == enabled) {
        return;
    }
    auto_poll_ = enabled;
    if (enabled) {
        polled_captures.add(this);
    } else {
        polled_captures.remove(this);
    }
}

bool LiveKitScreenCapture::get_auto_poll() const {
    return auto_poll_;
}

void LiveKitScreenCapture::poll_all() {
    polled_captures.poll_all(&LiveKitScreenCapture::poll);
}
