#include "livekit_track.h"

#include "livekit_audio_source.h"
#include "web_bridge.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void LiveKitTrack::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_sid"), &LiveKitTrack::get_sid);
    ClassDB::bind_method(D_METHOD("get_name"), &LiveKitTrack::get_name);
    ClassDB::bind_method(D_METHOD("get_kind"), &LiveKitTrack::get_kind);
    ClassDB::bind_method(D_METHOD("get_source"), &LiveKitTrack::get_source);
    ClassDB::bind_method(D_METHOD("get_muted"), &LiveKitTrack::get_muted);
    ClassDB::bind_method(D_METHOD("get_stream_state"), &LiveKitTrack::get_stream_state);
    ClassDB::bind_method(D_METHOD("request_stats"), &LiveKitTrack::request_stats);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "sid"), "", "get_sid");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "kind"), "", "get_kind");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "source"), "", "get_source");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "muted"), "", "get_muted");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stream_state"), "", "get_stream_state");

    ADD_SIGNAL(MethodInfo("stats_received",
            PropertyInfo(Variant::ARRAY, "stats")));

    BIND_ENUM_CONSTANT(KIND_UNKNOWN);
    BIND_ENUM_CONSTANT(KIND_AUDIO);
    BIND_ENUM_CONSTANT(KIND_VIDEO);

    BIND_ENUM_CONSTANT(SOURCE_UNKNOWN);
    BIND_ENUM_CONSTANT(SOURCE_CAMERA);
    BIND_ENUM_CONSTANT(SOURCE_MICROPHONE);
    BIND_ENUM_CONSTANT(SOURCE_SCREENSHARE);
    BIND_ENUM_CONSTANT(SOURCE_SCREENSHARE_AUDIO);

    BIND_ENUM_CONSTANT(STATE_UNKNOWN);
    BIND_ENUM_CONSTANT(STATE_ACTIVE);
    BIND_ENUM_CONSTANT(STATE_PAUSED);
}

Ref<LiveKitTrack> LiveKitTrack::from_info(const Dictionary &p_info, bool p_remote) {
    Ref<LiveKitTrack> track;
    int kind = p_info.get("kind", KIND_UNKNOWN);
    if (p_remote && kind == KIND_AUDIO) {
        track = Ref<LiveKitTrack>(memnew(LiveKitRemoteAudioTrack));
    } else if (p_remote && kind == KIND_VIDEO) {
        track = Ref<LiveKitTrack>(memnew(LiveKitRemoteVideoTrack));
    } else if (!p_remote && kind == KIND_AUDIO) {
        track = Ref<LiveKitTrack>(memnew(LiveKitLocalAudioTrack));
    } else {
        track.instantiate();
    }
    track->js_id_ = int(p_info.get("id", 0));
    track->info_ = p_info;
    return track;
}

Dictionary LiveKitTrack::_info() const {
    if (js_id_) {
        Dictionary live = web_call_dict("track_info", Array::make(js_id_));
        if (!live.is_empty()) {
            info_ = live;
        }
    }
    return info_;
}

String LiveKitTrack::get_sid() const {
    return _info().get("sid", String());
}

String LiveKitTrack::get_name() const {
    return _info().get("name", String());
}

int LiveKitTrack::get_kind() const {
    return _info().get("kind", KIND_UNKNOWN);
}

int LiveKitTrack::get_source() const {
    return _info().get("source", SOURCE_UNKNOWN);
}

bool LiveKitTrack::get_muted() const {
    return _info().get("muted", false);
}

int LiveKitTrack::get_stream_state() const {
    return _info().get("stream_state", STATE_UNKNOWN);
}

void LiveKitTrack::request_stats() {
    UtilityFunctions::push_error("LiveKitTrack::request_stats: not yet supported on the web");
}

// LiveKitLocalAudioTrack

void LiveKitLocalAudioTrack::_bind_methods() {
    ClassDB::bind_static_method("LiveKitLocalAudioTrack", D_METHOD("create", "name", "source"), &LiveKitLocalAudioTrack::create);
    ClassDB::bind_method(D_METHOD("mute"), &LiveKitLocalAudioTrack::mute);
    ClassDB::bind_method(D_METHOD("unmute"), &LiveKitLocalAudioTrack::unmute);
}

Ref<LiveKitLocalAudioTrack> LiveKitLocalAudioTrack::create(const String &name, const Ref<LiveKitAudioSource> &source) {
    if (source.is_null()) {
        UtilityFunctions::push_error("LiveKitLocalAudioTrack::create: source is null");
        return Ref<LiveKitLocalAudioTrack>();
    }
    Dictionary info = web_call_dict("local_audio_track_create", Array::make(source->get_js_id(), name));
    if (info.is_empty()) {
        return Ref<LiveKitLocalAudioTrack>();
    }
    return LiveKitTrack::from_info(info, false);
}

void LiveKitLocalAudioTrack::mute() {
    if (js_id_) {
        web_call("track_set_muted", Array::make(js_id_, true));
    }
}

void LiveKitLocalAudioTrack::unmute() {
    if (js_id_) {
        web_call("track_set_muted", Array::make(js_id_, false));
    }
}
