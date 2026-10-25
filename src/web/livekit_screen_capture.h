#ifndef GODOT_LIVEKIT_SCREEN_CAPTURE_H
#define GODOT_LIVEKIT_SCREEN_CAPTURE_H

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {

// Web implementation of LiveKitScreenCapture, on top of getDisplayMedia(). Browsers ask the user
// what to share when a capture starts, so monitors and windows can't be listed or chosen, and
// frames only arrive once the user has picked one. Frames are read like LiveKitVideoStream's.
class LiveKitScreenCapture : public RefCounted {
    GDCLASS(LiveKitScreenCapture, RefCounted)

public:
    enum PermissionLevel {
        PERMISSION_OK = 0,
        PERMISSION_WARNING = 1,
        PERMISSION_ERROR = 2,
    };

private:
    int js_id_ = 0;
    Ref<ImageTexture> texture_;
    Ref<Image> latest_image_;
    PackedByteArray cached_pba_;
    bool paused_ = false;
    bool auto_poll_ = true;

    void _register();
    void _unregister();

protected:
    static void _bind_methods();

public:
    ~LiveKitScreenCapture();

    // Browsers can't list these, so both are empty.
    static Array get_monitors();
    static Array get_windows();
    static Dictionary check_permissions();

    static Ref<LiveKitScreenCapture> create();
    // Browsers can't capture a chosen monitor or window, so these fail (after the same
    // validation as natively).
    static Ref<LiveKitScreenCapture> create_for_monitor(const Dictionary &monitor_dict);
    static Ref<LiveKitScreenCapture> create_for_window(const Dictionary &window_dict);

    // Asks the user what to share. Browsers may only allow this after a user gesture, so call it
    // from an input event handler.
    void start();
    void stop();
    void pause();
    void resume();
    bool is_paused() const;

    bool poll();
    Ref<ImageTexture> get_texture() const;
    Ref<Image> get_image() const;

    // A copy of the latest frame, since a browser can't capture without the user's choice.
    Ref<Image> screenshot();

    void close();

    void set_auto_poll(bool enabled);
    bool get_auto_poll() const;

    // Polls every capture with auto_poll enabled. Called once per frame.
    static void poll_all();
};

}

VARIANT_ENUM_CAST(LiveKitScreenCapture::PermissionLevel)

#endif // GODOT_LIVEKIT_SCREEN_CAPTURE_H
