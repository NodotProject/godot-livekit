#ifndef GODOT_LIVEKIT_POLLER_H
#define GODOT_LIVEKIT_POLLER_H

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

namespace godot {

class LiveKitRoom;
class LiveKitVideoStream;
class LiveKitScreenCapture;

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

	void register_screen_capture(LiveKitScreenCapture *ptr, std::shared_ptr<std::atomic<bool>> alive);
	void unregister_screen_capture(LiveKitScreenCapture *ptr);

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
	struct ScreenEntry {
		LiveKitScreenCapture *ptr;
		std::shared_ptr<std::atomic<bool>> alive;
	};

	std::mutex mutex_;
	std::vector<RoomEntry> rooms_;
	std::vector<VideoEntry> video_streams_;
	std::vector<ScreenEntry> screen_captures_;
};

} // namespace godot

#endif // GODOT_LIVEKIT_POLLER_H
