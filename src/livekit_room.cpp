#include "livekit_room.h"
#include "livekit_track.h"
#include "livekit_track_publication.h"
#include "livekit_e2ee.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <livekit/participant.h>
#include <livekit/local_participant.h>
#include <livekit/remote_participant.h>
#include <livekit/track.h>
#include <livekit/track_publication.h>
#include <livekit/local_track_publication.h>
#include <livekit/remote_track_publication.h>

using namespace godot;

void LiveKitRoom::_bind_methods() {
    ClassDB::bind_method(D_METHOD("connect_to_room", "url", "token", "options"), &LiveKitRoom::connect_to_room);
    ClassDB::bind_method(D_METHOD("disconnect_from_room"), &LiveKitRoom::disconnect_from_room);
    ClassDB::bind_method(D_METHOD("get_local_participant"), &LiveKitRoom::get_local_participant);
    ClassDB::bind_method(D_METHOD("get_remote_participants"), &LiveKitRoom::get_remote_participants);
    ClassDB::bind_method(D_METHOD("get_sid"), &LiveKitRoom::get_sid);
    ClassDB::bind_method(D_METHOD("get_name"), &LiveKitRoom::get_name);
    ClassDB::bind_method(D_METHOD("get_metadata"), &LiveKitRoom::get_metadata);
    ClassDB::bind_method(D_METHOD("get_connection_state"), &LiveKitRoom::get_connection_state);
    ClassDB::bind_method(D_METHOD("get_e2ee_manager"), &LiveKitRoom::get_e2ee_manager);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "local_participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitLocalParticipant"), "", "get_local_participant");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "remote_participants"), "", "get_remote_participants");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "sid"), "", "get_sid");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "metadata"), "", "get_metadata");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_state"), "", "get_connection_state");

    // Connection signals
    ADD_SIGNAL(MethodInfo("connected"));
    ADD_SIGNAL(MethodInfo("disconnected"));
    ADD_SIGNAL(MethodInfo("reconnecting"));
    ADD_SIGNAL(MethodInfo("reconnected"));

    // Participant signals
    ADD_SIGNAL(MethodInfo("participant_connected",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("participant_disconnected",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("participant_metadata_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::STRING, "old_metadata"),
            PropertyInfo(Variant::STRING, "new_metadata")));
    ADD_SIGNAL(MethodInfo("participant_name_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::STRING, "old_name"),
            PropertyInfo(Variant::STRING, "new_name")));
    ADD_SIGNAL(MethodInfo("participant_attributes_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::DICTIONARY, "changed_attributes")));

    // Room metadata
    ADD_SIGNAL(MethodInfo("room_metadata_changed",
            PropertyInfo(Variant::STRING, "old_metadata"),
            PropertyInfo(Variant::STRING, "new_metadata")));

    // Connection quality
    ADD_SIGNAL(MethodInfo("connection_quality_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::INT, "quality")));

    // Track signals
    ADD_SIGNAL(MethodInfo("track_published",
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteTrackPublication"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("track_unpublished",
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteTrackPublication"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("track_subscribed",
            PropertyInfo(Variant::OBJECT, "track", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrack"),
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteTrackPublication"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("track_unsubscribed",
            PropertyInfo(Variant::OBJECT, "track", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrack"),
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteTrackPublication"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("track_muted",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrackPublication")));
    ADD_SIGNAL(MethodInfo("track_unmuted",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrackPublication")));
    ADD_SIGNAL(MethodInfo("local_track_published",
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitLocalTrackPublication"),
            PropertyInfo(Variant::OBJECT, "track", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrack")));
    ADD_SIGNAL(MethodInfo("local_track_unpublished",
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitLocalTrackPublication")));

    // Data
    ADD_SIGNAL(MethodInfo("data_received",
            PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant"),
            PropertyInfo(Variant::INT, "kind"),
            PropertyInfo(Variant::STRING, "topic")));

    // E2EE signals
    ADD_SIGNAL(MethodInfo("e2ee_state_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::INT, "state")));
    ADD_SIGNAL(MethodInfo("participant_encryption_status_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::BOOL, "is_encrypted")));

    // Connection state enum
    BIND_ENUM_CONSTANT(STATE_DISCONNECTED);
    BIND_ENUM_CONSTANT(STATE_CONNECTED);
    BIND_ENUM_CONSTANT(STATE_RECONNECTING);
}

LiveKitRoom::LiveKitRoom() {
    room = new livekit::Room();
    delegate = new GodotRoomDelegate(this);
    room->setDelegate(delegate);
}

LiveKitRoom::~LiveKitRoom() {
    if (room) {
        delete room;
        room = nullptr;
    }
    if (delegate) {
        delete delegate;
        delegate = nullptr;
    }
}

bool LiveKitRoom::connect_to_room(const String &url, const String &token, const Dictionary &options) {
    livekit::RoomOptions room_options;
    room_options.auto_subscribe = options.get("auto_subscribe", true);
    room_options.dynacast = options.get("dynacast", false);

    // Parse E2EE options
    if (options.has("e2ee")) {
        Variant e2ee_var = options["e2ee"];
        Ref<LiveKitE2eeOptions> e2ee_opts = e2ee_var;
        if (e2ee_opts.is_valid()) {
            room_options.encryption = e2ee_opts->to_native();
        }
    }

    bool success = room->Connect(url.utf8().get_data(), token.utf8().get_data(), room_options);
    if (success) {
        connection_state = STATE_CONNECTED;

        // Initialize local participant
        livekit::LocalParticipant *lp = room->localParticipant();
        if (lp) {
            local_participant.instantiate();
            local_participant->bind_participant(lp);
            local_participant->bind_local_participant(lp);
        }

        // Initialize remote participants
        auto remotes = room->remoteParticipants();
        for (auto const &r : remotes) {
            Ref<LiveKitRemoteParticipant> p;
            p.instantiate();
            p->bind_participant(r.get());
            p->bind_remote_participant(static_cast<livekit::RemoteParticipant *>(r.get()));
            remote_participants[p->get_identity()] = p;
        }

        // Initialize E2EE manager if available
        livekit::E2EEManager *mgr = room->e2eeManager();
        if (mgr) {
            e2ee_manager_.instantiate();
            e2ee_manager_->bind_manager(mgr);
        }
    }
    return success;
}

void LiveKitRoom::disconnect_from_room() {
    // SDK has no explicit disconnect; destroying the Room disconnects
    local_participant.unref();
    remote_participants.clear();
    e2ee_manager_.unref();
    connection_state = STATE_DISCONNECTED;

    if (delegate) {
        delete delegate;
        delegate = nullptr;
    }
    if (room) {
        delete room;
        room = nullptr;
    }

    // Recreate room and delegate for potential reconnection
    room = new livekit::Room();
    delegate = new GodotRoomDelegate(this);
    room->setDelegate(delegate);
}

Ref<LiveKitLocalParticipant> LiveKitRoom::get_local_participant() const {
    return local_participant;
}

Dictionary LiveKitRoom::get_remote_participants() const {
    return remote_participants;
}

String LiveKitRoom::get_sid() const {
    if (room) {
        auto info = room->room_info();
        if (info.sid.has_value()) {
            return String(info.sid.value().c_str());
        }
    }
    return String();
}

String LiveKitRoom::get_name() const {
    if (room) {
        auto info = room->room_info();
        return String(info.name.c_str());
    }
    return String();
}

String LiveKitRoom::get_metadata() const {
    if (room) {
        auto info = room->room_info();
        return String(info.metadata.c_str());
    }
    return String();
}

int LiveKitRoom::get_connection_state() const {
    return (int)connection_state;
}

Ref<LiveKitE2eeManager> LiveKitRoom::get_e2ee_manager() const {
    return e2ee_manager_;
}

Ref<LiveKitParticipant> LiveKitRoom::_find_or_create_participant(livekit::Participant *p) {
    if (!p) {
        return Ref<LiveKitParticipant>();
    }

    String identity = String(p->identity().c_str());

    // Check if it's the local participant
    if (local_participant.is_valid() && local_participant->get_identity() == identity) {
        return local_participant;
    }

    // Check remote participants
    if (remote_participants.has(identity)) {
        return remote_participants[identity];
    }

    return Ref<LiveKitParticipant>();
}

// GodotRoomDelegate implementations

void LiveKitRoom::GodotRoomDelegate::onParticipantConnected(livekit::Room &r, const livekit::ParticipantConnectedEvent &e) {
    Ref<LiveKitRemoteParticipant> p;
    p.instantiate();
    p->bind_participant(e.participant);
    p->bind_remote_participant(static_cast<livekit::RemoteParticipant *>(e.participant));

    room->remote_participants[p->get_identity()] = p;
    room->call_deferred("emit_signal", "participant_connected", p);
}

void LiveKitRoom::GodotRoomDelegate::onParticipantDisconnected(livekit::Room &r, const livekit::ParticipantDisconnectedEvent &e) {
    String identity = e.participant->identity().c_str();
    Ref<LiveKitRemoteParticipant> p = room->remote_participants.get(identity, Variant());

    if (p.is_valid()) {
        room->remote_participants.erase(identity);
        room->call_deferred("emit_signal", "participant_disconnected", p);
    }
}

void LiveKitRoom::GodotRoomDelegate::onConnectionStateChanged(livekit::Room &r, const livekit::ConnectionStateChangedEvent &e) {
    if (e.state == livekit::ConnectionState::Connected) {
        room->connection_state = STATE_CONNECTED;
        room->call_deferred("emit_signal", "connected");
    } else if (e.state == livekit::ConnectionState::Disconnected) {
        room->connection_state = STATE_DISCONNECTED;
        room->call_deferred("emit_signal", "disconnected");
    } else if (e.state == livekit::ConnectionState::Reconnecting) {
        room->connection_state = STATE_RECONNECTING;
    }
}

void LiveKitRoom::GodotRoomDelegate::onDisconnected(livekit::Room &r, const livekit::DisconnectedEvent &e) {
    room->connection_state = STATE_DISCONNECTED;
    room->call_deferred("emit_signal", "disconnected");
}

void LiveKitRoom::GodotRoomDelegate::onReconnecting(livekit::Room &r, const livekit::ReconnectingEvent &e) {
    room->connection_state = STATE_RECONNECTING;
    room->call_deferred("emit_signal", "reconnecting");
}

void LiveKitRoom::GodotRoomDelegate::onReconnected(livekit::Room &r, const livekit::ReconnectedEvent &e) {
    room->connection_state = STATE_CONNECTED;
    room->call_deferred("emit_signal", "reconnected");
}

void LiveKitRoom::GodotRoomDelegate::onRoomMetadataChanged(livekit::Room &r, const livekit::RoomMetadataChangedEvent &e) {
    String old_metadata = String(e.old_metadata.c_str());
    String new_metadata = String(e.new_metadata.c_str());
    room->call_deferred("emit_signal", "room_metadata_changed", old_metadata, new_metadata);
}

void LiveKitRoom::GodotRoomDelegate::onConnectionQualityChanged(livekit::Room &r, const livekit::ConnectionQualityChangedEvent &e) {
    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    if (p.is_valid()) {
        room->call_deferred("emit_signal", "connection_quality_changed", p, (int)e.quality);
    }
}

void LiveKitRoom::GodotRoomDelegate::onParticipantMetadataChanged(livekit::Room &r, const livekit::ParticipantMetadataChangedEvent &e) {
    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    if (p.is_valid()) {
        String old_metadata = String(e.old_metadata.c_str());
        String new_metadata = String(e.new_metadata.c_str());
        room->call_deferred("emit_signal", "participant_metadata_changed", p, old_metadata, new_metadata);
    }
}

void LiveKitRoom::GodotRoomDelegate::onParticipantNameChanged(livekit::Room &r, const livekit::ParticipantNameChangedEvent &e) {
    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    if (p.is_valid()) {
        String old_name = String(e.old_name.c_str());
        String new_name = String(e.new_name.c_str());
        room->call_deferred("emit_signal", "participant_name_changed", p, old_name, new_name);
    }
}

void LiveKitRoom::GodotRoomDelegate::onParticipantAttributesChanged(livekit::Room &r, const livekit::ParticipantAttributesChangedEvent &e) {
    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    if (p.is_valid()) {
        Dictionary changed;
        for (const auto &attr : e.changed_attributes) {
            changed[String(attr.key.c_str())] = String(attr.value.c_str());
        }
        room->call_deferred("emit_signal", "participant_attributes_changed", p, changed);
    }
}

// Track delegate callbacks

void LiveKitRoom::GodotRoomDelegate::onTrackPublished(livekit::Room &r, const livekit::TrackPublishedEvent &e) {
    Ref<LiveKitRemoteTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(e.publication);

    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    room->call_deferred("emit_signal", "track_published", pub, p);
}

void LiveKitRoom::GodotRoomDelegate::onTrackUnpublished(livekit::Room &r, const livekit::TrackUnpublishedEvent &e) {
    Ref<LiveKitRemoteTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(e.publication);

    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    room->call_deferred("emit_signal", "track_unpublished", pub, p);
}

void LiveKitRoom::GodotRoomDelegate::onTrackSubscribed(livekit::Room &r, const livekit::TrackSubscribedEvent &e) {
    Ref<LiveKitTrack> track;
    track.instantiate();
    track->bind_track(e.track);

    Ref<LiveKitRemoteTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(e.publication);

    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    room->call_deferred("emit_signal", "track_subscribed", track, pub, p);
}

void LiveKitRoom::GodotRoomDelegate::onTrackUnsubscribed(livekit::Room &r, const livekit::TrackUnsubscribedEvent &e) {
    Ref<LiveKitTrack> track;
    track.instantiate();
    track->bind_track(e.track);

    Ref<LiveKitRemoteTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(e.publication);

    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    room->call_deferred("emit_signal", "track_unsubscribed", track, pub, p);
}

void LiveKitRoom::GodotRoomDelegate::onTrackMuted(livekit::Room &r, const livekit::TrackMutedEvent &e) {
    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);

    Ref<LiveKitTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(e.publication);

    room->call_deferred("emit_signal", "track_muted", p, pub);
}

void LiveKitRoom::GodotRoomDelegate::onTrackUnmuted(livekit::Room &r, const livekit::TrackUnmutedEvent &e) {
    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);

    Ref<LiveKitTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(e.publication);

    room->call_deferred("emit_signal", "track_unmuted", p, pub);
}

void LiveKitRoom::GodotRoomDelegate::onLocalTrackPublished(livekit::Room &r, const livekit::LocalTrackPublishedEvent &e) {
    Ref<LiveKitLocalTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(e.publication);

    Ref<LiveKitTrack> track;
    track.instantiate();
    track->bind_track(e.track);

    room->call_deferred("emit_signal", "local_track_published", pub, track);
}

void LiveKitRoom::GodotRoomDelegate::onLocalTrackUnpublished(livekit::Room &r, const livekit::LocalTrackUnpublishedEvent &e) {
    Ref<LiveKitLocalTrackPublication> pub;
    pub.instantiate();
    pub->bind_publication(e.publication);

    room->call_deferred("emit_signal", "local_track_unpublished", pub);
}

void LiveKitRoom::GodotRoomDelegate::onUserPacketReceived(livekit::Room &r, const livekit::UserDataPacketEvent &e) {
    PackedByteArray data;
    data.resize(e.data.size());
    memcpy(data.ptrw(), e.data.data(), e.data.size());

    Ref<LiveKitParticipant> p;
    if (e.participant) {
        p = room->_find_or_create_participant(e.participant);
    }

    int kind = (int)e.kind;
    String topic = e.topic.empty() ? String() : String(e.topic.c_str());

    room->call_deferred("emit_signal", "data_received", data, p, kind, topic);
}

void LiveKitRoom::GodotRoomDelegate::onE2eeStateChanged(livekit::Room &r, const livekit::E2eeStateChangedEvent &e) {
    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    if (p.is_valid()) {
        room->call_deferred("emit_signal", "e2ee_state_changed", p, (int)e.state);
    }
}

void LiveKitRoom::GodotRoomDelegate::onParticipantEncryptionStatusChanged(livekit::Room &r, const livekit::ParticipantEncryptionStatusChangedEvent &e) {
    Ref<LiveKitParticipant> p = room->_find_or_create_participant(e.participant);
    if (p.is_valid()) {
        room->call_deferred("emit_signal", "participant_encryption_status_changed", p, e.is_encrypted);
    }
}
