#include "livekit_video_stream.h"

#include "web_bridge.h"
#include "web_video.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <vector>

using namespace godot;

// Streams that poll automatically: those made by a factory, as natively, with auto_poll enabled.
static std::vector<LiveKitVideoStream *> polled_streams;

void LiveKitVideoStream::_bind_methods() {
    ClassDB::bind_static_method("LiveKitVideoStream", D_METHOD("from_track", "track"), &LiveKitVideoStream::from_track);
    ClassDB::bind_static_method("LiveKitVideoStream", D_METHOD("from_participant", "participant", "source"), &LiveKitVideoStream::from_participant);

    ClassDB::bind_method(D_METHOD("get_texture"), &LiveKitVideoStream::get_texture);
    ClassDB::bind_method(D_METHOD("poll"), &LiveKitVideoStream::poll);
    ClassDB::bind_method(D_METHOD("close"), &LiveKitVideoStream::close);

    ClassDB::bind_method(D_METHOD("set_auto_poll", "enabled"), &LiveKitVideoStream::set_auto_poll);
    ClassDB::bind_method(D_METHOD("get_auto_poll"), &LiveKitVideoStream::get_auto_poll);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_poll"), "set_auto_poll", "get_auto_poll");

    ADD_SIGNAL(MethodInfo("frame_received"));
}

LiveKitVideoStream::~LiveKitVideoStream() {
    _unregister();
    close();
}

Ref<LiveKitVideoStream> LiveKitVideoStream::from_track(const Ref<LiveKitTrack> &track) {
    if (track.is_null() || !track->get_js_id()) {
        UtilityFunctions::push_error("LiveKitVideoStream::from_track: invalid track");
        return Ref<LiveKitVideoStream>();
    }
    return _create(web_call("video_stream_from_track", Array::make(track->get_js_id())));
}

Ref<LiveKitVideoStream> LiveKitVideoStream::from_participant(const Ref<LiveKitRemoteParticipant> &participant, int source) {
    if (participant.is_null() || !participant->get_js_id()) {
        UtilityFunctions::push_error("LiveKitVideoStream::from_participant: invalid participant");
        return Ref<LiveKitVideoStream>();
    }
    return _create(web_call("video_stream_from_participant", Array::make(participant->get_room_js_id(), participant->get_js_id(), source)));
}

Ref<LiveKitVideoStream> LiveKitVideoStream::_create(const Variant &p_js_id) {
    if (p_js_id.get_type() == Variant::NIL) {
        return Ref<LiveKitVideoStream>();
    }
    Ref<LiveKitVideoStream> stream;
    stream.instantiate();
    stream->js_id_ = p_js_id;
    stream->texture_.instantiate();
    stream->_register();
    return stream;
}

Ref<ImageTexture> LiveKitVideoStream::get_texture() const {
    return texture_;
}

bool LiveKitVideoStream::poll() {
    int width = 0;
    int height = 0;
    if (!js_id_ || !web_video_stream_prepare(js_id_, &width, &height)) {
        return false;
    }
    cached_pba_.resize(int64_t(width) * height * 4);
    web_video_stream_read(js_id_, cached_pba_.ptrw());
    if (width == last_width_ && height == last_height_ && cached_image_.is_valid()) {
        cached_image_->set_data(width, height, false, Image::FORMAT_RGBA8, cached_pba_);
        texture_->update(cached_image_);
    } else {
        cached_image_ = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, cached_pba_);
        if (cached_image_.is_null()) {
            return false;
        }
        texture_->set_image(cached_image_);
        last_width_ = width;
        last_height_ = height;
    }
    emit_signal("frame_received");
    return true;
}

void LiveKitVideoStream::close() {
    if (js_id_) {
        web_call("video_stream_close", Array::make(js_id_));
        js_id_ = 0;
    }
}

void LiveKitVideoStream::set_auto_poll(bool enabled) {
    if (auto_poll_ == enabled) {
        return;
    }
    auto_poll_ = enabled;
    if (enabled) {
        _register();
    } else {
        _unregister();
    }
}

bool LiveKitVideoStream::get_auto_poll() const {
    return auto_poll_;
}

void LiveKitVideoStream::_register() {
    if (auto_poll_ && std::find(polled_streams.begin(), polled_streams.end(), this) == polled_streams.end()) {
        polled_streams.push_back(this);
    }
}

void LiveKitVideoStream::_unregister() {
    polled_streams.erase(std::remove(polled_streams.begin(), polled_streams.end(), this), polled_streams.end());
}

void LiveKitVideoStream::poll_all() {
    // frame_received handlers may create or free streams, so poll a snapshot and skip freed ones.
    std::vector<LiveKitVideoStream *> streams = polled_streams;
    for (LiveKitVideoStream *stream : streams) {
        if (std::find(polled_streams.begin(), polled_streams.end(), stream) != polled_streams.end()) {
            Ref<LiveKitVideoStream> keep_alive(stream);
            stream->poll();
        }
    }
}
