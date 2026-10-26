#include "livekit_video_stream.h"

#include "web_bridge.h"
#include "web_poll_list.h"
#include "web_video.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// Only streams made by a factory poll automatically, as natively.
static WebPollList<LiveKitVideoStream> polled_streams;

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
    polled_streams.remove(this);
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
    polled_streams.add(stream.ptr());
    return stream;
}

Ref<ImageTexture> LiveKitVideoStream::get_texture() const {
    return texture_;
}

bool LiveKitVideoStream::poll() {
    if (!web_video_stream_poll(js_id_, cached_image_, texture_, cached_pba_)) {
        return false;
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
        polled_streams.add(this);
    } else {
        polled_streams.remove(this);
    }
}

bool LiveKitVideoStream::get_auto_poll() const {
    return auto_poll_;
}

void LiveKitVideoStream::poll_all() {
    polled_streams.poll_all(&LiveKitVideoStream::poll);
}
