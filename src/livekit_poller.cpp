#include "livekit_poller.h"
#include "livekit_room.h"
#include "livekit_video_stream.h"
#include "livekit_screen_capture.h"

#include <algorithm>

using namespace godot;

// --- Registration ---

void LiveKitPoller::register_room(LiveKitRoom *ptr, std::shared_ptr<std::atomic<bool>> alive) {
	std::lock_guard<std::mutex> lock(mutex_);
	// Avoid duplicate registration.
	for (const auto &e : rooms_) {
		if (e.ptr == ptr) return;
	}
	rooms_.push_back({ptr, std::move(alive)});
}

void LiveKitPoller::unregister_room(LiveKitRoom *ptr) {
	std::lock_guard<std::mutex> lock(mutex_);
	rooms_.erase(
		std::remove_if(rooms_.begin(), rooms_.end(),
			[ptr](const RoomEntry &e) { return e.ptr == ptr; }),
		rooms_.end());
}

void LiveKitPoller::register_video_stream(LiveKitVideoStream *ptr, std::shared_ptr<std::atomic<bool>> alive) {
	std::lock_guard<std::mutex> lock(mutex_);
	for (const auto &e : video_streams_) {
		if (e.ptr == ptr) return;
	}
	video_streams_.push_back({ptr, std::move(alive)});
}

void LiveKitPoller::unregister_video_stream(LiveKitVideoStream *ptr) {
	std::lock_guard<std::mutex> lock(mutex_);
	video_streams_.erase(
		std::remove_if(video_streams_.begin(), video_streams_.end(),
			[ptr](const VideoEntry &e) { return e.ptr == ptr; }),
		video_streams_.end());
}

void LiveKitPoller::register_screen_capture(LiveKitScreenCapture *ptr, std::shared_ptr<std::atomic<bool>> alive) {
	std::lock_guard<std::mutex> lock(mutex_);
	for (const auto &e : screen_captures_) {
		if (e.ptr == ptr) return;
	}
	screen_captures_.push_back({ptr, std::move(alive)});
}

void LiveKitPoller::unregister_screen_capture(LiveKitScreenCapture *ptr) {
	std::lock_guard<std::mutex> lock(mutex_);
	screen_captures_.erase(
		std::remove_if(screen_captures_.begin(), screen_captures_.end(),
			[ptr](const ScreenEntry &e) { return e.ptr == ptr; }),
		screen_captures_.end());
}

// --- Polling ---

void LiveKitPoller::poll_all() {
	// Snapshot entries under lock so signal handlers triggered by
	// poll_events()/poll() can register/unregister freely without
	// deadlocking (std::mutex is not re-entrant).
	std::vector<RoomEntry> rooms_snap;
	std::vector<VideoEntry> video_snap;
	std::vector<ScreenEntry> screen_snap;
	{
		std::lock_guard<std::mutex> lock(mutex_);
		rooms_snap = rooms_;
		video_snap = video_streams_;
		screen_snap = screen_captures_;
	}

	// Poll without holding the lock.
	for (auto &e : rooms_snap) {
		if (e.alive->load()) e.ptr->poll_events();
	}
	for (auto &e : video_snap) {
		if (e.alive->load()) e.ptr->poll();
	}
	for (auto &e : screen_snap) {
		if (e.alive->load()) e.ptr->poll();
	}

	// GC dead entries under a separate lock acquisition.
	{
		std::lock_guard<std::mutex> lock(mutex_);
		rooms_.erase(
			std::remove_if(rooms_.begin(), rooms_.end(),
				[](const RoomEntry &e) { return !e.alive->load(); }),
			rooms_.end());
		video_streams_.erase(
			std::remove_if(video_streams_.begin(), video_streams_.end(),
				[](const VideoEntry &e) { return !e.alive->load(); }),
			video_streams_.end());
		screen_captures_.erase(
			std::remove_if(screen_captures_.begin(), screen_captures_.end(),
				[](const ScreenEntry &e) { return !e.alive->load(); }),
			screen_captures_.end());
	}
}
