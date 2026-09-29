#ifndef GODOT_LIVEKIT_ROOM_H
#define GODOT_LIVEKIT_ROOM_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>

#include "livekit_participant.h"

namespace godot {

// Web implementation of LiveKitRoom, backed by a livekit-client Room. Events are delivered by the
// browser between frames, so waiting for them requires yielding (e.g. `await`), not blocking.
class LiveKitRoom : public RefCounted {
    GDCLASS(LiveKitRoom, RefCounted)

public:
    enum ConnectionState {
        STATE_DISCONNECTED = 0,
        STATE_CONNECTED = 1,
        STATE_RECONNECTING = 2,
    };

private:
    int js_id_ = 0;
    int connection_state_ = STATE_DISCONNECTED;
    bool auto_poll_ = true;
    Ref<LiveKitLocalParticipant> local_participant_;
    Dictionary remote_participants_;

    void _dispatch(const Dictionary &p_event);
    Ref<LiveKitParticipant> _participant(const Dictionary &p_event, bool p_create_remote = false);
    void _reset_participants();

protected:
    static void _bind_methods();

public:
    LiveKitRoom();
    ~LiveKitRoom();

    bool connect_to_room(const String &url, const String &token, const Dictionary &options);
    void disconnect_from_room();
    void poll_events();

    Ref<LiveKitLocalParticipant> get_local_participant() const;
    Dictionary get_remote_participants() const;

    String get_sid() const;
    String get_name() const;
    String get_metadata() const;
    int get_connection_state() const;

    void set_auto_poll(bool enabled);
    bool get_auto_poll() const;

    // Polls every room with auto_poll enabled. Called once per frame.
    static void poll_all();
};

}

VARIANT_ENUM_CAST(LiveKitRoom::ConnectionState);

#endif // GODOT_LIVEKIT_ROOM_H
