#include "livekit_track_publication.h"

#include "web_bridge.h"

using namespace godot;

void LiveKitTrackPublication::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_sid"), &LiveKitTrackPublication::get_sid);
    ClassDB::bind_method(D_METHOD("get_name"), &LiveKitTrackPublication::get_name);
    ClassDB::bind_method(D_METHOD("get_kind"), &LiveKitTrackPublication::get_kind);
    ClassDB::bind_method(D_METHOD("get_source"), &LiveKitTrackPublication::get_source);
    ClassDB::bind_method(D_METHOD("get_muted"), &LiveKitTrackPublication::get_muted);
    ClassDB::bind_method(D_METHOD("get_mime_type"), &LiveKitTrackPublication::get_mime_type);
    ClassDB::bind_method(D_METHOD("get_simulcasted"), &LiveKitTrackPublication::get_simulcasted);
    ClassDB::bind_method(D_METHOD("get_track"), &LiveKitTrackPublication::get_track);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "sid"), "", "get_sid");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "kind"), "", "get_kind");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "source"), "", "get_source");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "muted"), "", "get_muted");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "mime_type"), "", "get_mime_type");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "simulcasted"), "", "get_simulcasted");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "track", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrack"), "", "get_track");
}

Ref<LiveKitTrackPublication> LiveKitTrackPublication::from_info(const Dictionary &p_info, bool p_remote) {
    Ref<LiveKitTrackPublication> pub;
    if (p_remote) {
        pub = Ref<LiveKitTrackPublication>(memnew(LiveKitRemoteTrackPublication));
    } else {
        pub = Ref<LiveKitTrackPublication>(memnew(LiveKitLocalTrackPublication));
    }
    pub->js_id_ = int(p_info.get("id", 0));
    pub->remote_ = p_remote;
    pub->info_ = p_info;
    return pub;
}

Dictionary LiveKitTrackPublication::_info() const {
    if (js_id_) {
        Dictionary live = web_call_dict("publication_info", Array::make(js_id_));
        if (!live.is_empty()) {
            info_ = live;
        }
    }
    return info_;
}

String LiveKitTrackPublication::get_sid() const {
    return _info().get("sid", String());
}

String LiveKitTrackPublication::get_name() const {
    return _info().get("name", String());
}

int LiveKitTrackPublication::get_kind() const {
    return _info().get("kind", LiveKitTrack::KIND_UNKNOWN);
}

int LiveKitTrackPublication::get_source() const {
    return _info().get("source", LiveKitTrack::SOURCE_UNKNOWN);
}

bool LiveKitTrackPublication::get_muted() const {
    return _info().get("muted", false);
}

String LiveKitTrackPublication::get_mime_type() const {
    return _info().get("mime_type", String());
}

bool LiveKitTrackPublication::get_simulcasted() const {
    return _info().get("simulcasted", false);
}

Ref<LiveKitTrack> LiveKitTrackPublication::get_track() const {
    Variant track = _info().get("track", Variant());
    if (track.get_type() != Variant::DICTIONARY) {
        return Ref<LiveKitTrack>();
    }
    return LiveKitTrack::from_info(track, remote_);
}

// LiveKitRemoteTrackPublication

void LiveKitRemoteTrackPublication::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_subscribed"), &LiveKitRemoteTrackPublication::get_subscribed);
    ClassDB::bind_method(D_METHOD("set_subscribed", "subscribed"), &LiveKitRemoteTrackPublication::set_subscribed);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "subscribed"), "set_subscribed", "get_subscribed");
}

bool LiveKitRemoteTrackPublication::get_subscribed() const {
    return _info().get("subscribed", false);
}

void LiveKitRemoteTrackPublication::set_subscribed(bool p_subscribed) {
    if (js_id_) {
        web_call("publication_set_subscribed", Array::make(js_id_, p_subscribed));
    }
}
