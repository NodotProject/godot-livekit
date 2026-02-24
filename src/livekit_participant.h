#ifndef GODOT_LIVEKIT_PARTICIPANT_H
#define GODOT_LIVEKIT_PARTICIPANT_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <livekit/participant.h>

namespace godot {

class LiveKitParticipant : public RefCounted {
    GDCLASS(LiveKitParticipant, RefCounted)

protected:
    static void _bind_methods();
    livekit::Participant *participant_ = nullptr;

public:
    LiveKitParticipant();
    ~LiveKitParticipant();

    virtual void bind_participant(livekit::Participant *p_participant);

    String get_sid() const;
    String get_name() const;
    String get_identity() const;
    String get_metadata() const;
};

class LiveKitLocalParticipant : public LiveKitParticipant {
    GDCLASS(LiveKitLocalParticipant, LiveKitParticipant)

protected:
    static void _bind_methods();

public:
    LiveKitLocalParticipant();
    ~LiveKitLocalParticipant();
};

class LiveKitRemoteParticipant : public LiveKitParticipant {
    GDCLASS(LiveKitRemoteParticipant, LiveKitParticipant)

protected:
    static void _bind_methods();

public:
    LiveKitRemoteParticipant();
    ~LiveKitRemoteParticipant();
};

}

#endif // GODOT_LIVEKIT_PARTICIPANT_H
