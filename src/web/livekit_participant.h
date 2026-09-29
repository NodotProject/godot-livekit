#ifndef GODOT_LIVEKIT_PARTICIPANT_H
#define GODOT_LIVEKIT_PARTICIPANT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {

class LiveKitTrack;
class LiveKitLocalTrackPublication;

// Web implementation of LiveKitParticipant, identified by its room and identity.
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

    int room_js_id_ = 0;
    String identity_;
    bool local_ = false;
    // The last known state, used once the participant has left the room.
    mutable Dictionary info_;

    Dictionary _info() const;
    Dictionary _publications() const;

public:
    void bind_web_participant(int p_room_js_id, const String &p_identity, bool p_local);

    String get_sid() const;
    String get_name() const;
    String get_identity() const;
    String get_metadata() const;
    Dictionary get_attributes() const;
    int get_kind() const;
};

class LiveKitLocalParticipant : public LiveKitParticipant {
    GDCLASS(LiveKitLocalParticipant, LiveKitParticipant)

protected:
    static void _bind_methods();

public:
    void publish_data(const PackedByteArray &data, bool reliable, const PackedStringArray &destination_identities, const String &topic);
    void set_metadata(const String &metadata);
    void set_name(const String &name);
    void set_attributes(const Dictionary &attributes);

    Dictionary get_track_publications() const;
    Ref<LiveKitLocalTrackPublication> publish_track(const Ref<LiveKitTrack> &track, const Dictionary &options);
    void unpublish_track(const String &track_sid);

    void perform_rpc(const String &destination, const String &method, const String &payload, double timeout);
    void register_rpc_method(const String &method);
    void unregister_rpc_method(const String &method);
};

class LiveKitRemoteParticipant : public LiveKitParticipant {
    GDCLASS(LiveKitRemoteParticipant, LiveKitParticipant)

protected:
    static void _bind_methods();

public:
    Dictionary get_track_publications() const;
};

}

VARIANT_ENUM_CAST(LiveKitParticipant::ParticipantKind);

#endif // GODOT_LIVEKIT_PARTICIPANT_H
