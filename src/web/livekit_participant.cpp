#include "livekit_participant.h"

#include "livekit_track.h"
#include "livekit_track_publication.h"
#include "web_bridge.h"

#include <godot_cpp/classes/marshalls.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// LiveKitParticipant

void LiveKitParticipant::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_sid"), &LiveKitParticipant::get_sid);
    ClassDB::bind_method(D_METHOD("get_name"), &LiveKitParticipant::get_name);
    ClassDB::bind_method(D_METHOD("get_identity"), &LiveKitParticipant::get_identity);
    ClassDB::bind_method(D_METHOD("get_metadata"), &LiveKitParticipant::get_metadata);
    ClassDB::bind_method(D_METHOD("get_attributes"), &LiveKitParticipant::get_attributes);
    ClassDB::bind_method(D_METHOD("get_kind"), &LiveKitParticipant::get_kind);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "sid"), "", "get_sid");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "identity"), "", "get_identity");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "metadata"), "", "get_metadata");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "attributes"), "", "get_attributes");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "kind"), "", "get_kind");

    BIND_ENUM_CONSTANT(KIND_STANDARD);
    BIND_ENUM_CONSTANT(KIND_INGRESS);
    BIND_ENUM_CONSTANT(KIND_EGRESS);
    BIND_ENUM_CONSTANT(KIND_SIP);
    BIND_ENUM_CONSTANT(KIND_AGENT);
}

void LiveKitParticipant::bind_web_participant(int p_room_js_id, const String &p_identity, bool p_local) {
    room_js_id_ = p_room_js_id;
    identity_ = p_identity;
    local_ = p_local;
    _info();
}

Dictionary LiveKitParticipant::_info() const {
    if (room_js_id_) {
        Dictionary live = web_call_dict("participant_info", Array::make(room_js_id_, identity_, local_));
        if (!live.is_empty()) {
            info_ = live;
        }
    }
    return info_;
}

// Returns the participant's publications keyed by track sid.
Dictionary LiveKitParticipant::_publications() const {
    Dictionary result;
    if (!room_js_id_) {
        return result;
    }
    Array infos = web_call("participant_publications", Array::make(room_js_id_, identity_, local_));
    for (int i = 0; i < infos.size(); i++) {
        Dictionary info = infos[i];
        result[info.get("sid", String())] = LiveKitTrackPublication::from_info(info, !local_);
    }
    return result;
}

String LiveKitParticipant::get_sid() const {
    return _info().get("sid", String());
}

String LiveKitParticipant::get_name() const {
    return _info().get("name", String());
}

String LiveKitParticipant::get_identity() const {
    // The local participant's identity comes from its token, so it's only known once connected.
    return local_ ? String(_info().get("identity", String())) : identity_;
}

String LiveKitParticipant::get_metadata() const {
    return _info().get("metadata", String());
}

Dictionary LiveKitParticipant::get_attributes() const {
    return _info().get("attributes", Dictionary());
}

int LiveKitParticipant::get_kind() const {
    return _info().get("kind", KIND_STANDARD);
}

// LiveKitLocalParticipant

void LiveKitLocalParticipant::_bind_methods() {
    ClassDB::bind_method(D_METHOD("publish_data", "data", "reliable", "destination_identities", "topic"), &LiveKitLocalParticipant::publish_data, DEFVAL(true), DEFVAL(PackedStringArray()), DEFVAL(String()));
    ClassDB::bind_method(D_METHOD("set_metadata", "metadata"), &LiveKitLocalParticipant::set_metadata);
    ClassDB::bind_method(D_METHOD("set_name", "name"), &LiveKitLocalParticipant::set_name);
    ClassDB::bind_method(D_METHOD("set_attributes", "attributes"), &LiveKitLocalParticipant::set_attributes);
    ClassDB::bind_method(D_METHOD("get_track_publications"), &LiveKitLocalParticipant::get_track_publications);
    ClassDB::bind_method(D_METHOD("publish_track", "track", "options"), &LiveKitLocalParticipant::publish_track, DEFVAL(Dictionary()));
    ClassDB::bind_method(D_METHOD("unpublish_track", "track_sid"), &LiveKitLocalParticipant::unpublish_track);

    ClassDB::bind_method(D_METHOD("perform_rpc", "destination", "method", "payload", "timeout"), &LiveKitLocalParticipant::perform_rpc, DEFVAL(10.0));
    ClassDB::bind_method(D_METHOD("register_rpc_method", "method"), &LiveKitLocalParticipant::register_rpc_method);
    ClassDB::bind_method(D_METHOD("unregister_rpc_method", "method"), &LiveKitLocalParticipant::unregister_rpc_method);

    ADD_SIGNAL(MethodInfo("rpc_method_invoked",
            PropertyInfo(Variant::STRING, "method"),
            PropertyInfo(Variant::STRING, "request_id"),
            PropertyInfo(Variant::STRING, "caller_identity"),
            PropertyInfo(Variant::STRING, "payload"),
            PropertyInfo(Variant::FLOAT, "response_timeout")));

    ADD_SIGNAL(MethodInfo("rpc_response_received",
            PropertyInfo(Variant::STRING, "method"),
            PropertyInfo(Variant::STRING, "result")));

    ADD_SIGNAL(MethodInfo("rpc_error",
            PropertyInfo(Variant::STRING, "method"),
            PropertyInfo(Variant::STRING, "error_message")));
}

void LiveKitLocalParticipant::publish_data(const PackedByteArray &data, bool reliable, const PackedStringArray &destination_identities, const String &topic) {
    if (!room_js_id_) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::publish_data: not bound");
        return;
    }
    String payload = Marshalls::get_singleton()->raw_to_base64(data);
    web_call("local_publish_data", Array::make(room_js_id_, payload, reliable, destination_identities, topic));
}

void LiveKitLocalParticipant::set_metadata(const String &metadata) {
    if (!room_js_id_) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::set_metadata: not bound");
        return;
    }
    web_call("local_set_metadata", Array::make(room_js_id_, metadata));
}

void LiveKitLocalParticipant::set_name(const String &name) {
    if (!room_js_id_) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::set_name: not bound");
        return;
    }
    web_call("local_set_name", Array::make(room_js_id_, name));
}

void LiveKitLocalParticipant::set_attributes(const Dictionary &attributes) {
    if (!room_js_id_) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::set_attributes: not bound");
        return;
    }
    Dictionary strings;
    Array keys = attributes.keys();
    for (int i = 0; i < keys.size(); i++) {
        strings[String(keys[i])] = String(attributes[keys[i]]);
    }
    web_call("local_set_attributes", Array::make(room_js_id_, strings));
}

Dictionary LiveKitLocalParticipant::get_track_publications() const {
    return _publications();
}

// livekit-client publishes asynchronously, so the returned publication's sid stays empty until
// publishing completes (signaled by the room's local_track_published).
Ref<LiveKitLocalTrackPublication> LiveKitLocalParticipant::publish_track(const Ref<LiveKitTrack> &track, const Dictionary &options) {
    if (!room_js_id_ || track.is_null() || !track->get_js_id()) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::publish_track: invalid arguments");
        return Ref<LiveKitLocalTrackPublication>();
    }
    Dictionary info = web_call_dict("local_publish_track", Array::make(room_js_id_, track->get_js_id(), options));
    if (info.is_empty()) {
        return Ref<LiveKitLocalTrackPublication>();
    }
    return LiveKitTrackPublication::from_info(info, false);
}

void LiveKitLocalParticipant::unpublish_track(const String &track_sid) {
    if (!room_js_id_) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::unpublish_track: not bound");
        return;
    }
    web_call("local_unpublish_track", Array::make(room_js_id_, track_sid));
}

void LiveKitLocalParticipant::perform_rpc(const String &destination, const String &method, const String &payload, double timeout) {
    UtilityFunctions::push_error("LiveKitLocalParticipant::perform_rpc: not yet supported on the web");
}

void LiveKitLocalParticipant::register_rpc_method(const String &method) {
    UtilityFunctions::push_error("LiveKitLocalParticipant::register_rpc_method: not yet supported on the web");
}

void LiveKitLocalParticipant::unregister_rpc_method(const String &method) {
    UtilityFunctions::push_error("LiveKitLocalParticipant::unregister_rpc_method: not yet supported on the web");
}

// LiveKitRemoteParticipant

void LiveKitRemoteParticipant::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_track_publications"), &LiveKitRemoteParticipant::get_track_publications);
}

Dictionary LiveKitRemoteParticipant::get_track_publications() const {
    return _publications();
}
