#ifndef GODOT_LIVEKIT_POLLER_H
#define GODOT_LIVEKIT_POLLER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace godot {

class LiveKitRoom;
class LiveKitVideoStream;
#ifdef LIVEKIT_SCREEN_CAPTURE_SUPPORTED
class LiveKitScreenCapture;
#endif

// Singleton that auto-polls all registered LiveKit objects every frame.
// Uses register_frame_callback() — no SceneTree node needed.
//
// Audio streams are NOT auto-polled because poll() requires a
// Ref<AudioStreamGeneratorPlayback> that the poller can't provide.
class LiveKitPoller {
public:
	static LiveKitPoller &instance() {
		static LiveKitPoller poller;
		return poller;
	}

	// --- Registration ---

	void register_room(LiveKitRoom *ptr, std::shared_ptr<std::atomic<bool>> alive);
	void unregister_room(LiveKitRoom *ptr);

	void register_video_stream(LiveKitVideoStream *ptr, std::shared_ptr<std::atomic<bool>> alive);
	void unregister_video_stream(LiveKitVideoStream *ptr);

#ifdef LIVEKIT_SCREEN_CAPTURE_SUPPORTED
	void register_screen_capture(LiveKitScreenCapture *ptr, std::shared_ptr<std::atomic<bool>> alive);
	void unregister_screen_capture(LiveKitScreenCapture *ptr);
#endif

	// Called once per frame from the registered frame callback.
	void poll_all();

private:
	LiveKitPoller() = default;
	LiveKitPoller(const LiveKitPoller &) = delete;
	LiveKitPoller &operator=(const LiveKitPoller &) = delete;

	struct RoomEntry {
		LiveKitRoom *ptr;
		std::shared_ptr<std::atomic<bool>> alive;
	};
	struct VideoEntry {
		LiveKitVideoStream *ptr;
		std::shared_ptr<std::atomic<bool>> alive;
	};
#ifdef LIVEKIT_SCREEN_CAPTURE_SUPPORTED
	struct ScreenEntry {
		LiveKitScreenCapture *ptr;
		std::shared_ptr<std::atomic<bool>> alive;
	};
#endif

	std::mutex mutex_;
	std::vector<RoomEntry> rooms_;
	std::vector<VideoEntry> video_streams_;
#ifdef LIVEKIT_SCREEN_CAPTURE_SUPPORTED
	std::vector<ScreenEntry> screen_captures_;
#endif
};

} // namespace godot

#endif // GODOT_LIVEKIT_POLLER_H
