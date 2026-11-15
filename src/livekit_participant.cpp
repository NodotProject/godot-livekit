#include "livekit_participant.h"
#include "livekit_track.h"
#include "livekit_track_publication.h"
#include "sdk_call.h"

#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <livekit/rpc_error.h>

#include <algorithm>

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

void LiveKitParticipant::bind_participant(const std::weak_ptr<livekit::Participant> &p_participant, const std::string &p_identity) {
    participant_ = p_participant;
    identity_ = p_identity;
}

String LiveKitParticipant::get_sid() const {
    if (auto participant = participant_.lock()) {
        return String(participant->sid().c_str());
    }
    return String();
}

String LiveKitParticipant::get_name() const {
    if (auto participant = participant_.lock()) {
        return String(participant->name().c_str());
    }
    return String();
}

String LiveKitParticipant::get_identity() const {
    return String(identity_.c_str());
}

String LiveKitParticipant::get_metadata() const {
    if (auto participant = participant_.lock()) {
        return String(participant->metadata().c_str());
    }
    return String();
}

Dictionary LiveKitParticipant::get_attributes() const {
    Dictionary result;
    if (auto participant = participant_.lock()) {
        const auto &attrs = participant->attributes();
        for (const auto &pair : attrs) {
            result[String(pair.first.c_str())] = String(pair.second.c_str());
        }
    }
    return result;
}

int LiveKitParticipant::get_kind() const {
    if (auto participant = participant_.lock()) {
        return (int)participant->kind();
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
    // respond_to_rpc / respond_to_rpc_error are not exposed: the underlying
    // SDK requires synchronous responses from the handler callback, so async
    // RPC replies are not yet supported.  The handler returns nullopt, which
    // the SDK sends to the caller as an empty response.

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
    // Wait for any in-flight RPC thread.
    rpc_thread_.join_or_detach(2000);
    unregister_rpc_methods();
}

std::shared_ptr<livekit::LocalParticipant> LiveKitLocalParticipant::get_native_local_participant() const {
    return std::static_pointer_cast<livekit::LocalParticipant>(participant_.lock());
}

void LiveKitLocalParticipant::publish_data(const PackedByteArray &data, bool reliable, const PackedStringArray &destination_identities, const String &topic) {
    auto lp = get_native_local_participant();
    if (!lp) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::publish_data: not bound");
        return;
    }

    std::vector<uint8_t> payload(data.ptr(), data.ptr() + data.size());
    std::vector<std::string> identities;
    for (int i = 0; i < destination_identities.size(); i++) {
        identities.push_back(std::string(destination_identities[i].utf8().get_data()));
    }
    std::string topic_str = topic.utf8().get_data();

    call_sdk("LiveKitLocalParticipant::publish_data", [&] { lp->publishData(payload, reliable, identities, topic_str); });
}

void LiveKitLocalParticipant::set_metadata(const String &metadata) {
    auto lp = get_native_local_participant();
    if (!lp) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::set_metadata: not bound");
        return;
    }
    call_sdk("LiveKitLocalParticipant::set_metadata", [&] { lp->setMetadata(std::string(metadata.utf8().get_data())); });
}

void LiveKitLocalParticipant::set_name(const String &name) {
    auto lp = get_native_local_participant();
    if (!lp) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::set_name: not bound");
        return;
    }
    call_sdk("LiveKitLocalParticipant::set_name", [&] { lp->setName(std::string(name.utf8().get_data())); });
}

void LiveKitLocalParticipant::set_attributes(const Dictionary &attributes) {
    auto lp = get_native_local_participant();
    if (!lp) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::set_attributes: not bound");
        return;
    }

    std::unordered_map<std::string, std::string> attrs;
    Array keys = attributes.keys();
    for (int i = 0; i < keys.size(); i++) {
        String key = keys[i];
        String value = attributes[key];
        attrs[std::string(key.utf8().get_data())] = std::string(value.utf8().get_data());
    }
    call_sdk("LiveKitLocalParticipant::set_attributes", [&] { lp->setAttributes(attrs); });
}

Dictionary LiveKitLocalParticipant::get_track_publications() const {
    Dictionary result;
    auto lp = get_native_local_participant();
    if (!lp) {
        return result;
    }

    const auto &pubs = lp->trackPublications();
    for (const auto &pair : pubs) {
        Ref<LiveKitLocalTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pair.second);
        result[String(pair.first.c_str())] = pub;
    }
    return result;
}

Ref<LiveKitLocalTrackPublication> LiveKitLocalParticipant::publish_track(const Ref<LiveKitTrack> &track, const Dictionary &options) {
    auto lp = get_native_local_participant();
    if (!lp || track.is_null()) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::publish_track: invalid arguments");
        return Ref<LiveKitLocalTrackPublication>();
    }

    auto native_track = track->get_native_track();
    if (!native_track) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::publish_track: track has no native handle");
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

    if (!call_sdk("LiveKitLocalParticipant::publish_track", [&] { lp->publishTrack(native_track, pub_options); })) {
        return Ref<LiveKitLocalTrackPublication>();
    }

    std::shared_ptr<livekit::LocalTrackPublication> native_pub;
    if (auto audio = std::dynamic_pointer_cast<livekit::LocalAudioTrack>(native_track)) {
        native_pub = audio->publication();
    } else if (auto video = std::dynamic_pointer_cast<livekit::LocalVideoTrack>(native_track)) {
        native_pub = video->publication();
    }
    if (!native_pub) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::publish_track: publish failed");
        return Ref<LiveKitLocalTrackPublication>();
    }

    if (std::find(published_tracks_.begin(), published_tracks_.end(), native_track) == published_tracks_.end()) {
        published_tracks_.push_back(native_track);
    }

    Ref<LiveKitLocalTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(native_pub);
    return pub;
}

void LiveKitLocalParticipant::unpublish_track(const String &track_sid) {
    auto lp = get_native_local_participant();
    if (!lp) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::unpublish_track: not bound");
        return;
    }
    std::string sid = track_sid.utf8().get_data();
    if (!call_sdk("LiveKitLocalParticipant::unpublish_track", [&] { lp->unpublishTrack(sid); })) {
        return;
    }
    published_tracks_.erase(
            std::remove_if(published_tracks_.begin(), published_tracks_.end(),
                    [&sid](const std::shared_ptr<livekit::Track> &track) { return track->sid() == sid; }),
            published_tracks_.end());
}

void LiveKitLocalParticipant::unbind() {
    // Unbinding stops the destructor from reaching the SDK to unregister these.
    unregister_rpc_methods();
    // The SDK's local participant methods wait for a reply without a timeout, and a room being torn
    // down may never send one, so calls made after this fail rather than risk blocking forever.
    participant_.reset();
    published_tracks_.clear();
}

// RPC

void LiveKitLocalParticipant::perform_rpc(const String &destination, const String &method, const String &payload, double timeout) {
    auto lp = get_native_local_participant();
    if (!lp) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::perform_rpc: not bound");
        return;
    }

    // Capture copies for the background thread
    std::string dest_str(destination.utf8().get_data());
    std::string method_str(method.utf8().get_data());
    std::string payload_str(payload.utf8().get_data());
    String method_for_signal = method;

    // Prevent the Godot ref-counted object from being freed while the
    // background thread is running.
    Ref<LiveKitLocalParticipant> prevent_free(this);

    // Run the blocking SDK call on a background thread; deliver the result
    // back to the main thread via signal (call_deferred is thread-safe).
    // Wait for any previous RPC thread to finish before starting a new one.
    rpc_thread_.join_or_detach(2000);
    rpc_thread_.start([prevent_free, lp, dest_str, method_str, payload_str, timeout, method_for_signal]() {
        try {
            std::string result = lp->performRpc(dest_str, method_str, payload_str, timeout);
            prevent_free->call_deferred("emit_signal", "rpc_response_received",
                    method_for_signal, String(result.c_str()));
        } catch (const livekit::RpcError &e) {
            prevent_free->call_deferred("emit_signal", "rpc_error",
                    method_for_signal, String(e.message().c_str()));
        } catch (const std::exception &e) {
            // E.g. the room disconnected, invalidating the participant's handle.
            prevent_free->call_deferred("emit_signal", "rpc_error",
                    method_for_signal, String(e.what()));
        }
    });
}

void LiveKitLocalParticipant::register_rpc_method(const String &method) {
    auto lp = get_native_local_participant();
    if (!lp) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::register_rpc_method: not bound");
        return;
    }

    std::string method_name = method.utf8().get_data();
    // The SDK calls handlers on its own thread, and may still be running one when this participant is
    // freed, so the handler only holds its ID and emits from the main thread if it's still around.
    const uint64_t self_id = get_instance_id();
    bool registered = call_sdk("LiveKitLocalParticipant::register_rpc_method", [&] {
        lp->registerRpcMethod(method_name,
                [self_id, method](const livekit::RpcInvocationData &data) -> std::optional<std::string> {
                    callable_mp_static(&LiveKitLocalParticipant::emit_rpc_method_invoked)
                            .call_deferred(self_id, method,
                                    String(data.request_id.c_str()),
                                    String(data.caller_identity.c_str()),
                                    String(data.payload.c_str()),
                                    data.response_timeout_sec);
                    // Replies aren't supported yet (see _bind_methods), so the caller gets an empty response.
                    return std::nullopt;
                });
    });
    if (registered) {
        registered_rpc_methods_.push_back(method_name);
    }
}

void LiveKitLocalParticipant::emit_rpc_method_invoked(uint64_t p_participant_id, const String &p_method,
        const String &p_request_id, const String &p_caller_identity, const String &p_payload, double p_response_timeout) {
    if (auto *participant = Object::cast_to<LiveKitLocalParticipant>(ObjectDB::get_instance(p_participant_id))) {
        participant->emit_signal("rpc_method_invoked", p_method, p_request_id, p_caller_identity, p_payload,
                p_response_timeout);
    }
}

void LiveKitLocalParticipant::unregister_rpc_method(const String &method) {
    auto lp = get_native_local_participant();
    if (!lp) {
        UtilityFunctions::push_error("LiveKitLocalParticipant::unregister_rpc_method: not bound");
        return;
    }
    std::string method_name = method.utf8().get_data();
    call_sdk("LiveKitLocalParticipant::unregister_rpc_method", [&] { lp->unregisterRpcMethod(method_name); });
    registered_rpc_methods_.erase(
            std::remove(registered_rpc_methods_.begin(), registered_rpc_methods_.end(), method_name),
            registered_rpc_methods_.end());
}

void LiveKitLocalParticipant::respond_to_rpc(const String &request_id, const String &payload) {
    // RPC responses are handled through the handler return value
    // Since we're using async (nullopt return), this is a no-op placeholder
    // The SDK's RPC model requires synchronous response from the handler
    UtilityFunctions::push_error("LiveKitLocalParticipant::respond_to_rpc: async RPC responses not yet supported by SDK");
}

void LiveKitLocalParticipant::respond_to_rpc_error(const String &request_id, int code, const String &message) {
    UtilityFunctions::push_error("LiveKitLocalParticipant::respond_to_rpc_error: async RPC responses not yet supported by SDK");
}

// LiveKitRemoteParticipant

void LiveKitLocalParticipant::unregister_rpc_methods() {
    if (auto lp = get_native_local_participant()) {
        for (const auto &method : registered_rpc_methods_) {
            try {
                lp->unregisterRpcMethod(method);
            } catch (const std::exception &) {
                // The room is going away, so it won't invoke the method anyway.
            }
        }
    }
    registered_rpc_methods_.clear();
}

void LiveKitRemoteParticipant::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_track_publications"), &LiveKitRemoteParticipant::get_track_publications);
}

LiveKitRemoteParticipant::LiveKitRemoteParticipant() {
}

LiveKitRemoteParticipant::~LiveKitRemoteParticipant() {
}

std::shared_ptr<livekit::RemoteParticipant> LiveKitRemoteParticipant::get_native_remote_participant() const {
    return std::static_pointer_cast<livekit::RemoteParticipant>(participant_.lock());
}

Dictionary LiveKitRemoteParticipant::get_track_publications() const {
    Dictionary result;
    auto rp = get_native_remote_participant();
    if (!rp) {
        return result;
    }

    const auto &pubs = rp->trackPublications();
    for (const auto &pair : pubs) {
        Ref<LiveKitRemoteTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pair.second);
        result[String(pair.first.c_str())] = pub;
    }
    return result;
}
