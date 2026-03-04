#ifndef GODOT_LIVEKIT_SCREEN_CAPTURE_H
#define GODOT_LIVEKIT_SCREEN_CAPTURE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <frametap/frametap.h>

#include <memory>
#include <mutex>
#include <atomic>
#include <vector>
#include <cstdint>

namespace godot {

class LiveKitScreenCapture : public RefCounted {
    GDCLASS(LiveKitScreenCapture, RefCounted)

public:
    enum PermissionLevel {
        PERMISSION_OK = 0,
        PERMISSION_WARNING = 1,
        PERMISSION_ERROR = 2,
    };

private:
    struct PendingFrame {
        std::vector<uint8_t> data;
        size_t width = 0;
        size_t height = 0;
    };

    std::unique_ptr<frametap::FrameTap> tap_;
    Ref<ImageTexture> texture_;
    Ref<Image> latest_image_;
    std::mutex frame_mutex_;
    PendingFrame pending_frame_;
    std::atomic<bool> has_pending_{false};
    std::atomic<bool> capturing_{false};

    // Shared sentinel for poller safety — stays valid after destruction.
    std::shared_ptr<std::atomic<bool>> alive_ = std::make_shared<std::atomic<bool>>(true);

    // Cached frame buffers — reused across poll() calls to avoid per-frame allocations.
    PackedByteArray cached_pba_;
    size_t last_width_{0};
    size_t last_height_{0};

    // Auto-poll: when true, the poller calls poll() every frame automatically.
    bool auto_poll_{true};

protected:
    static void _bind_methods();

public:
    LiveKitScreenCapture();
    ~LiveKitScreenCapture();

    // Static query methods
    static Array get_monitors();
    static Array get_windows();
    static Dictionary check_permissions();

    // Factory methods
    static Ref<LiveKitScreenCapture> create();
    static Ref<LiveKitScreenCapture> create_for_monitor(const Dictionary &monitor_dict);
    static Ref<LiveKitScreenCapture> create_for_window(const Dictionary &window_dict);

    // Capture lifecycle
    void start();
    void stop();
    void pause();
    void resume();
    bool is_paused() const;

    // Frame access
    bool poll();
    Ref<ImageTexture> get_texture() const;
    Ref<Image> get_image() const;

    // One-shot capture
    Ref<Image> screenshot();

    // Cleanup
    void close();

    void set_auto_poll(bool enabled);
    bool get_auto_poll() const;
};

}

VARIANT_ENUM_CAST(LiveKitScreenCapture::PermissionLevel)

#endif // GODOT_LIVEKIT_SCREEN_CAPTURE_H
