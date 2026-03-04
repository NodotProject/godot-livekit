#ifndef GODOT_LIVEKIT_DETACHABLE_THREAD_H
#define GODOT_LIVEKIT_DETACHABLE_THREAD_H

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "thread_pool.h"

// Move-only wrapper around std::thread that tracks completion via a
// shared state.  The state is a shared_ptr so it remains valid even
// after the DetachableThread instance is destroyed (when a stuck
// thread is moved to the zombie pool during shutdown).
class DetachableThread {
	struct State {
		mutable std::mutex mutex;
		std::condition_variable cv;
		std::shared_ptr<std::atomic<bool>> done = std::make_shared<std::atomic<bool>>(false);
	};

	std::thread thread_;
	std::shared_ptr<State> state_ = std::make_shared<State>();

public:
	DetachableThread() = default;
	DetachableThread(DetachableThread &&) = default;
	DetachableThread &operator=(DetachableThread &&) = default;

	// Start a new thread.  The done flag is automatically set when fn returns.
	template <typename F>
	void start(F &&fn) {
		assert(!thread_.joinable());
		state_ = std::make_shared<State>();
		auto state = state_; // captured by value so it outlives `this`
		thread_ = std::thread([state, f = std::forward<F>(fn)]() mutable {
			f();
			{
				std::lock_guard<std::mutex> lock(state->mutex);
				state->done->store(true);
			}
			state->cv.notify_all();
		});
	}

	// Wait up to timeout_ms using a condition variable, then join if
	// done or move to the zombie pool if the thread is still stuck.
	void join_or_detach(int timeout_ms = 2000) {
		if (!thread_.joinable()) {
			return;
		}
		{
			std::unique_lock<std::mutex> lock(state_->mutex);
			state_->cv.wait_for(lock, std::chrono::milliseconds(timeout_ms), [this]() {
				return state_->done->load();
			});
		}
		if (state_->done->load()) {
			thread_.join();
		} else {
			// Move to zombie pool instead of bare detach — the pool will
			// attempt a clean join at shutdown.
			ZombieThreadPool::Entry entry;
			entry.thread = std::move(thread_);
			entry.done = state_->done;
			ZombieThreadPool::instance().add(std::move(entry));
		}
	}

	bool finished() const {
		return state_->done->load();
	}
	bool joinable() const { return thread_.joinable(); }
	void join() { thread_.join(); }
	void detach() { thread_.detach(); }

	// Extract the raw std::thread and reset the wrapper.  Needed when
	// Room's disconnect thread moves the connect thread into its lambda.
	std::thread release() {
		auto t = std::move(thread_);
		state_ = std::make_shared<State>();
		return t;
	}
};

#endif // GODOT_LIVEKIT_DETACHABLE_THREAD_H
