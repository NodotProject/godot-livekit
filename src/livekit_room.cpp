#include "livekit_room.h"
#include <godot_cpp/variant/utility_functions.hpp>

#include <livekit/participant.h>
#include <livekit/local_participant.h>
#include <livekit/remote_participant.h>

using namespace godot;

void LiveKitRoom::_bind_methods() {
    ClassDB::bind_method(D_METHOD("connect_to_room", "url", "token", "options"), &LiveKitRoom::connect_to_room);
    ClassDB::bind_method(D_METHOD("disconnect_from_room"), &LiveKitRoom::disconnect_from_room);
    ClassDB::bind_method(D_METHOD("get_local_participant"), &LiveKitRoom::get_local_participant);
    ClassDB::bind_method(D_METHOD("get_remote_participants"), &LiveKitRoom::get_remote_participants);

    ADD_SIGNAL(MethodInfo("connected"));
    ADD_SIGNAL(MethodInfo("disconnected"));
    ADD_SIGNAL(MethodInfo("participant_connected", PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant")));
    ADD_SIGNAL(MethodInfo("participant_disconnected", PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant")));
}

LiveKitRoom::LiveKitRoom() {
    room = new livekit::Room();
    delegate = new GodotRoomDelegate(this);
    room->setDelegate(delegate);
}

LiveKitRoom::~LiveKitRoom() {
    if (room) {
        delete room;
    }
    if (delegate) {
        delete delegate;
    }
}

bool LiveKitRoom::connect_to_room(const String &url, const String &token, const Dictionary &options) {
    livekit::RoomOptions room_options;
    room_options.auto_subscribe = options.get("auto_subscribe", true);

    bool success = room->Connect(url.utf8().get_data(), token.utf8().get_data(), room_options);
    if (success) {
        // Initialize local participant
        livekit::LocalParticipant *lp = room->localParticipant();
        if (lp) {
            local_participant.instantiate();
            local_participant->bind_participant(lp);
        }

        // Initialize remote participants
        auto remotes = room->remoteParticipants();
        for (auto const &r : remotes) {
            Ref<LiveKitRemoteParticipant> p;
            p.instantiate();
            p->bind_participant(r.get());
            remote_participants[p->get_identity()] = p;
        }
    }
    return success;
}

void LiveKitRoom::disconnect_from_room() {
    // If the SDK doesn't have a disconnect() method, we can destroy the room object
    // and recreate it, or just let the destructor handle it.
    // However, destroying 'room' here might be dangerous if events are still firing.
}

Ref<LiveKitParticipant> LiveKitRoom::get_local_participant() const {
    return local_participant;
}

Dictionary LiveKitRoom::get_remote_participants() const {
    return remote_participants;
}

// GodotRoomDelegate implementations

void LiveKitRoom::GodotRoomDelegate::onParticipantConnected(livekit::Room &r, const livekit::ParticipantConnectedEvent &e) {
    Ref<LiveKitRemoteParticipant> p;
    p.instantiate();
    p->bind_participant(e.participant);

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
        room->call_deferred("emit_signal", "connected");
    } else if (e.state == livekit::ConnectionState::Disconnected) {
        room->call_deferred("emit_signal", "disconnected");
    }
}

void LiveKitRoom::GodotRoomDelegate::onDisconnected(livekit::Room &r, const livekit::DisconnectedEvent &e) {
    room->call_deferred("emit_signal", "disconnected");
}