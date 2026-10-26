#ifndef GODOT_LIVEKIT_WEB_POLL_LIST_H
#define GODOT_LIVEKIT_WEB_POLL_LIST_H

#include <godot_cpp/classes/ref.hpp>

#include <algorithm>
#include <vector>

namespace godot {

// The objects of a class that poll once per frame (those with auto_poll enabled).
template <typename T>
class WebPollList {
    std::vector<T *> objects_;

    bool _has(T *p_object) const {
        return std::find(objects_.begin(), objects_.end(), p_object) != objects_.end();
    }

public:
    void add(T *p_object) {
        if (!_has(p_object)) {
            objects_.push_back(p_object);
        }
    }

    void remove(T *p_object) {
        objects_.erase(std::remove(objects_.begin(), objects_.end(), p_object), objects_.end());
    }

    // Calls p_poll on each object. Signal handlers may add or free objects meanwhile, so this polls
    // a snapshot, skipping objects freed since, and keeps each object alive while it polls.
    template <typename R>
    void poll_all(R (T::*p_poll)()) {
        std::vector<T *> objects = objects_;
        for (T *object : objects) {
            if (_has(object)) {
                Ref<T> keep_alive(object);
                (object->*p_poll)();
            }
        }
    }
};

}

#endif // GODOT_LIVEKIT_WEB_POLL_LIST_H
