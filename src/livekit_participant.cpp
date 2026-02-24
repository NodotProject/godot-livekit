#include "livekit_participant.h"
#include "livekit_track.h"
#include "livekit_track_publication.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <livekit/rpc_error.h>

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

LiveKitParticipant::LiveKitParticipant() {
}

LiveKitParticipant::~LiveKitParticipant() {
}

void LiveKitParticipant::bind_participant(livekit::Participant *p_participant) {
    participant_ = p_participant;
}

String LiveKitParticipant::get_sid() const {
    if (participant_) {
        return String(participant_->sid().c_str());
    }
    return String();
}

String LiveKitParticipant::get_name() const {
    if (participant_) {
        return String(participant_->name().c_str());
    }
    return String();
}

String LiveKitParticipant::get_identity() const {
    if (participant_) {
        return String(participant_->identity().c_str());
    }
    return String();
}

String LiveKitParticipant::get_metadata() const {
    if (participant_) {
        return String(participant_->metadata().c_str());
    }
    return String();
}

Dictionary LiveKitParticipant::get_attributes() const {
    Dictionary result;
    if (participant_) {
        const auto &attrs = participant_->attributes();
        for (const auto &pair : attrs) {
            result[String(pair.first.c_str())] = String(pair.second.c_str());
        }
    }
    return result;
}

int LiveKitParticipant::get_kind() const {
    if (participant_) {
        return (int)participant_->kind();
    }
    return KIND_STANDARD;
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

    // RPC
    ClassDB::bind_method(D_METHOD("perform_rpc", "destination", "method", "payload", "timeout"), &LiveKitLocalParticipant::perform_rpc, DEFVAL(10.0));
    ClassDB::bind_method(D_METHOD("register_rpc_method", "method"), &LiveKitLocalParticipant::register_rpc_method);
    ClassDB::bind_method(D_METHOD("unregister_rpc_method", "method"), &LiveKitLocalParticipant::unregister_rpc_method);
    ClassDB::bind_method(D_METHOD("respond_to_rpc", "request_id", "payload"), &LiveKitLocalParticipant::respond_to_rpc);
    ClassDB::bind_method(D_METHOD("respond_to_rpc_error", "request_id", "code", "message"), &LiveKitLocalParticipant::respond_to_rpc_error);

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

LiveKitLocalParticipant::LiveKitLocalParticipant() {
}

LiveKitLocalParticipant::~LiveKitLocalParticipant() {
}

void LiveKitLocalParticipant::bind_local_participant(livekit::LocalParticipant *p) {
    local_participant_ = p;
}

void LiveKitLocalParticipant::publish_data(const PackedByteArray &data, bool reliable, const PackedStringArray &destination_identities, const String &topic) {
    if (!local_participant_) {
        UtilityFunctions::printerr("LiveKitLocalParticipant::publish_data: not bound");
        return;
    }

    std::vector<uint8_t> payload(data.ptr(), data.ptr() + data.size());
    std::vector<std::string> identities;
    for (int i = 0; i < destination_identities.size(); i++) {
        identities.push_back(std::string(destination_identities[i].utf8().get_data()));
    }
    std::string topic_str = topic.utf8().get_data();

    local_participant_->publishData(payload, reliable, identities, topic_str);
}

void LiveKitLocalParticipant::set_metadata(const String &metadata) {
    if (!local_participant_) {
        return;
    }
    local_participant_->setMetadata(std::string(metadata.utf8().get_data()));
}

void LiveKitLocalParticipant::set_name(const String &name) {
    if (!local_participant_) {
        return;
    }
    local_participant_->setName(std::string(name.utf8().get_data()));
}

void LiveKitLocalParticipant::set_attributes(const Dictionary &attributes) {
    if (!local_participant_) {
        return;
    }

    std::unordered_map<std::string, std::string> attrs;
    Array keys = attributes.keys();
    for (int i = 0; i < keys.size(); i++) {
        String key = keys[i];
        String value = attributes[key];
        attrs[std::string(key.utf8().get_data())] = std::string(value.utf8().get_data());
    }
    local_participant_->setAttributes(attrs);
}

Dictionary LiveKitLocalParticipant::get_track_publications() const {
    Dictionary result;
    if (!local_participant_) {
        return result;
    }

    const auto &pubs = local_participant_->trackPublications();
    for (const auto &pair : pubs) {
        Ref<LiveKitLocalTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pair.second);
        result[String(pair.first.c_str())] = pub;
    }
    return result;
}

Ref<LiveKitLocalTrackPublication> LiveKitLocalParticipant::publish_track(const Ref<LiveKitTrack> &track, const Dictionary &options) {
    if (!local_participant_ || track.is_null()) {
        UtilityFunctions::printerr("LiveKitLocalParticipant::publish_track: invalid arguments");
        return Ref<LiveKitLocalTrackPublication>();
    }

    auto native_track = track->get_native_track();
    if (!native_track) {
        UtilityFunctions::printerr("LiveKitLocalParticipant::publish_track: track has no native handle");
        return Ref<LiveKitLocalTrackPublication>();
    }

    livekit::TrackPublishOptions pub_options;
    // Parse options dictionary
    if (options.has("dtx")) {
        pub_options.dtx = options["dtx"];
    }
    if (options.has("red")) {
        pub_options.red = options["red"];
    }
    if (options.has("simulcast")) {
        pub_options.simulcast = options["simulcast"];
    }
    if (options.has("source")) {
        pub_options.source = (livekit::TrackSource)(int)options["source"];
    }
    // video_codec is a VideoCodec enum in the SDK - not easily set from string

    auto native_pub = local_participant_->publishTrack(native_track, pub_options);
    if (!native_pub) {
        UtilityFunctions::printerr("LiveKitLocalParticipant::publish_track: publish failed");
        return Ref<LiveKitLocalTrackPublication>();
    }

    Ref<LiveKitLocalTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(native_pub);
    return pub;
}

void LiveKitLocalParticipant::unpublish_track(const String &track_sid) {
    if (!local_participant_) {
        return;
    }
    local_participant_->unpublishTrack(std::string(track_sid.utf8().get_data()));
}

// RPC

void LiveKitLocalParticipant::perform_rpc(const String &destination, const String &method, const String &payload, double timeout) {
    if (!local_participant_) {
        UtilityFunctions::printerr("LiveKitLocalParticipant::perform_rpc: not bound");
        return;
    }

    // Capture copies for the background thread
    std::string dest_str(destination.utf8().get_data());
    std::string method_str(method.utf8().get_data());
    std::string payload_str(payload.utf8().get_data());
    String method_for_signal = method;

    // Run the blocking SDK call on a background thread; deliver the result
    // back to the main thread via signal (call_deferred is thread-safe).
    std::thread([this, dest_str, method_str, payload_str, timeout, method_for_signal]() {
        try {
            std::string result = local_participant_->performRpc(
                    dest_str, method_str, payload_str, timeout);
            call_deferred("emit_signal", "rpc_response_received",
                    method_for_signal, String(result.c_str()));
        } catch (const livekit::RpcError &e) {
            call_deferred("emit_signal", "rpc_error",
                    method_for_signal, String(e.message().c_str()));
        }
    }).detach();
}

void LiveKitLocalParticipant::register_rpc_method(const String &method) {
    if (!local_participant_) {
        return;
    }

    std::string method_name = method.utf8().get_data();
    local_participant_->registerRpcMethod(method_name,
            [this, method](const livekit::RpcInvocationData &data) -> std::optional<std::string> {
                call_deferred("emit_signal", "rpc_method_invoked",
                        method,
                        String(data.request_id.c_str()),
                        String(data.caller_identity.c_str()),
                        String(data.payload.c_str()),
                        data.response_timeout_sec);
                // Return nullopt to indicate async handling - user calls respond_to_rpc
                return std::nullopt;
            });
}

void LiveKitLocalParticipant::unregister_rpc_method(const String &method) {
    if (!local_participant_) {
        return;
    }
    local_participant_->unregisterRpcMethod(std::string(method.utf8().get_data()));
}

void LiveKitLocalParticipant::respond_to_rpc(const String &request_id, const String &payload) {
    // RPC responses are handled through the handler return value
    // Since we're using async (nullopt return), this is a no-op placeholder
    // The SDK's RPC model requires synchronous response from the handler
    UtilityFunctions::printerr("LiveKitLocalParticipant::respond_to_rpc: async RPC responses not yet supported by SDK");
}

void LiveKitLocalParticipant::respond_to_rpc_error(const String &request_id, int code, const String &message) {
    UtilityFunctions::printerr("LiveKitLocalParticipant::respond_to_rpc_error: async RPC responses not yet supported by SDK");
}

// LiveKitRemoteParticipant

void LiveKitRemoteParticipant::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_track_publications"), &LiveKitRemoteParticipant::get_track_publications);
}

LiveKitRemoteParticipant::LiveKitRemoteParticipant() {
}

LiveKitRemoteParticipant::~LiveKitRemoteParticipant() {
}

void LiveKitRemoteParticipant::bind_remote_participant(livekit::RemoteParticipant *p) {
    remote_participant_ = p;
}

Dictionary LiveKitRemoteParticipant::get_track_publications() const {
    Dictionary result;
    if (!remote_participant_) {
        return result;
    }

    const auto &pubs = remote_participant_->trackPublications();
    for (const auto &pair : pubs) {
        Ref<LiveKitRemoteTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pair.second);
        result[String(pair.first.c_str())] = pub;
    }
    return result;
}
