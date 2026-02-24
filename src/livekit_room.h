#ifndef GODOT_LIVEKIT_ROOM_H
#define GODOT_LIVEKIT_ROOM_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include <livekit/room.h>
#include <livekit/room_delegate.h>

#include "livekit_participant.h"

namespace godot {

class LiveKitRoom : public RefCounted {
    GDCLASS(LiveKitRoom, RefCounted)

private:
    class GodotRoomDelegate : public livekit::RoomDelegate {
    private:
        LiveKitRoom *room;

    public:
        GodotRoomDelegate(LiveKitRoom *p_room) :
                room(p_room) {}

        virtual void onParticipantConnected(livekit::Room &r, const livekit::ParticipantConnectedEvent &e) override;
        virtual void onParticipantDisconnected(livekit::Room &r, const livekit::ParticipantDisconnectedEvent &e) override;
        virtual void onConnectionStateChanged(livekit::Room &r, const livekit::ConnectionStateChangedEvent &e) override;
        virtual void onDisconnected(livekit::Room &r, const livekit::DisconnectedEvent &e) override;
    };

    livekit::Room *room = nullptr;
    GodotRoomDelegate *delegate = nullptr;

    Ref<LiveKitParticipant> local_participant;
    Dictionary remote_participants;

protected:
    static void _bind_methods();

public:
    LiveKitRoom();
    ~LiveKitRoom();

    bool connect_to_room(const String &url, const String &token, const Dictionary &options);
    void disconnect_from_room();

    Ref<LiveKitParticipant> get_local_participant() const;
    Dictionary get_remote_participants() const;
};

}

#endif // GODOT_LIVEKIT_ROOM_H