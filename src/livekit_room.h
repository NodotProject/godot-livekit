#ifndef GODOT_LIVEKIT_ROOM_H
#define GODOT_LIVEKIT_ROOM_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <livekit/room.h>
#include <livekit/room_delegate.h>

#include "livekit_participant.h"
#include "livekit_e2ee.h"

namespace godot {

class LiveKitTrack;
class LiveKitTrackPublication;
class LiveKitLocalTrackPublication;
class LiveKitRemoteTrackPublication;

class LiveKitRoom : public RefCounted {
    GDCLASS(LiveKitRoom, RefCounted)

public:
    enum ConnectionState {
        STATE_DISCONNECTED = 0,
        STATE_CONNECTED = 1,
        STATE_RECONNECTING = 2,
    };

private:
    class GodotRoomDelegate : public livekit::RoomDelegate {
    private:
        LiveKitRoom *room;

    public:
        GodotRoomDelegate(LiveKitRoom *p_room) :
                room(p_room) {}

        void onParticipantConnected(livekit::Room &r, const livekit::ParticipantConnectedEvent &e) override;
        void onParticipantDisconnected(livekit::Room &r, const livekit::ParticipantDisconnectedEvent &e) override;
        void onConnectionStateChanged(livekit::Room &r, const livekit::ConnectionStateChangedEvent &e) override;
        void onDisconnected(livekit::Room &r, const livekit::DisconnectedEvent &e) override;
        void onReconnecting(livekit::Room &r, const livekit::ReconnectingEvent &e) override;
        void onReconnected(livekit::Room &r, const livekit::ReconnectedEvent &e) override;
        void onRoomMetadataChanged(livekit::Room &r, const livekit::RoomMetadataChangedEvent &e) override;
        void onConnectionQualityChanged(livekit::Room &r, const livekit::ConnectionQualityChangedEvent &e) override;
        void onParticipantMetadataChanged(livekit::Room &r, const livekit::ParticipantMetadataChangedEvent &e) override;
        void onParticipantNameChanged(livekit::Room &r, const livekit::ParticipantNameChangedEvent &e) override;
        void onParticipantAttributesChanged(livekit::Room &r, const livekit::ParticipantAttributesChangedEvent &e) override;
        void onTrackPublished(livekit::Room &r, const livekit::TrackPublishedEvent &e) override;
        void onTrackUnpublished(livekit::Room &r, const livekit::TrackUnpublishedEvent &e) override;
        void onTrackSubscribed(livekit::Room &r, const livekit::TrackSubscribedEvent &e) override;
        void onTrackUnsubscribed(livekit::Room &r, const livekit::TrackUnsubscribedEvent &e) override;
        void onTrackMuted(livekit::Room &r, const livekit::TrackMutedEvent &e) override;
        void onTrackUnmuted(livekit::Room &r, const livekit::TrackUnmutedEvent &e) override;
        void onLocalTrackPublished(livekit::Room &r, const livekit::LocalTrackPublishedEvent &e) override;
        void onLocalTrackUnpublished(livekit::Room &r, const livekit::LocalTrackUnpublishedEvent &e) override;
        void onUserPacketReceived(livekit::Room &r, const livekit::UserDataPacketEvent &e) override;
        void onE2eeStateChanged(livekit::Room &r, const livekit::E2eeStateChangedEvent &e) override;
        void onParticipantEncryptionStatusChanged(livekit::Room &r, const livekit::ParticipantEncryptionStatusChangedEvent &e) override;
    };

    livekit::Room *room = nullptr;
    GodotRoomDelegate *delegate = nullptr;

    Ref<LiveKitLocalParticipant> local_participant;
    Dictionary remote_participants;
    ConnectionState connection_state = STATE_DISCONNECTED;
    Ref<LiveKitE2eeManager> e2ee_manager_;

    Ref<LiveKitParticipant> _find_or_create_participant(livekit::Participant *p);

protected:
    static void _bind_methods();

public:
    LiveKitRoom();
    ~LiveKitRoom();

    bool connect_to_room(const String &url, const String &token, const Dictionary &options);
    void disconnect_from_room();

    Ref<LiveKitLocalParticipant> get_local_participant() const;
    Dictionary get_remote_participants() const;

    String get_sid() const;
    String get_name() const;
    String get_metadata() const;
    int get_connection_state() const;

    Ref<LiveKitE2eeManager> get_e2ee_manager() const;

    livekit::Room *get_native_room() const { return room; }
};

}

VARIANT_ENUM_CAST(LiveKitRoom::ConnectionState);

#endif // GODOT_LIVEKIT_ROOM_H
