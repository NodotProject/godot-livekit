#ifndef LIVEKIT_ROOM_H
#define LIVEKIT_ROOM_H

#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {

class LiveKitRoom : public RefCounted {
    GDCLASS(LiveKitRoom, RefCounted)

protected:
    static void _bind_methods();

public:
    LiveKitRoom();
    ~LiveKitRoom();
};

}

#endif // LIVEKIT_ROOM_H