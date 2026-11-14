#ifndef GODOT_LIVEKIT_PARTICIPANT_H
#define GODOT_LIVEKIT_PARTICIPANT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

#include <livekit/participant.h>
#include <livekit/local_participant.h>
#include <livekit/remote_participant.h>

#include <memory>
#include <string>
#include <vector>

#include "detachable_thread.h"

namespace godot {

class LiveKitTrack;
class LiveKitLocalTrackPublication;

class LiveKitParticipant : public RefCounted {
    GDCLASS(LiveKitParticipant, RefCounted)

public:
    enum ParticipantKind {
        KIND_STANDARD = 0,
        KIND_INGRESS = 1,
        KIND_EGRESS = 2,
        KIND_SIP = 3,
        KIND_AGENT = 4,
    };

protected:
    static void _bind_methods();
    // Weak, since the room owns its participants and frees them once they disconnect.
    std::weak_ptr<livekit::Participant> participant_;

public:
    LiveKitParticipant();
    ~LiveKitParticipant();

    void bind_participant(const std::weak_ptr<livekit::Participant> &p_participant);

    String get_sid() const;
    String get_name() const;
    String get_identity() const;
    String get_metadata() const;
    Dictionary get_attributes() const;
    int get_kind() const;
};

class LiveKitLocalParticipant : public LiveKitParticipant {
    GDCLASS(LiveKitLocalParticipant, LiveKitParticipant)

private:
    // The SDK only holds published tracks weakly, and forgets their publications and events once
    // they're freed, so these keep them alive until they're unpublished.
    std::vector<std::shared_ptr<livekit::Track>> published_tracks_;
    std::vector<std::string> registered_rpc_methods_;
    DetachableThread rpc_thread_;

protected:
    static void _bind_methods();

public:
    LiveKitLocalParticipant();
    ~LiveKitLocalParticipant();

    std::shared_ptr<livekit::LocalParticipant> get_native_local_participant() const;

    void publish_data(const PackedByteArray &data, bool reliable, const PackedStringArray &destination_identities, const String &topic);
    void set_metadata(const String &metadata);
    void set_name(const String &name);
    void set_attributes(const Dictionary &attributes);

    Dictionary get_track_publications() const;
    Ref<LiveKitLocalTrackPublication> publish_track(const Ref<LiveKitTrack> &track, const Dictionary &options);
    void unpublish_track(const String &track_sid);

    // RPC (async — result delivered via rpc_response_received / rpc_error signals)
    void perform_rpc(const String &destination, const String &method, const String &payload, double timeout);
    void register_rpc_method(const String &method);
    void unregister_rpc_method(const String &method);
    void respond_to_rpc(const String &request_id, const String &payload);
    void respond_to_rpc_error(const String &request_id, int code, const String &message);
};

class LiveKitRemoteParticipant : public LiveKitParticipant {
    GDCLASS(LiveKitRemoteParticipant, LiveKitParticipant)

protected:
    static void _bind_methods();

public:
    LiveKitRemoteParticipant();
    ~LiveKitRemoteParticipant();

    std::shared_ptr<livekit::RemoteParticipant> get_native_remote_participant() const;

    Dictionary get_track_publications() const;
};

}

VARIANT_ENUM_CAST(LiveKitParticipant::ParticipantKind);

#endif // GODOT_LIVEKIT_PARTICIPANT_H
