#ifndef GODOT_LIVEKIT_THREAD_POOL_H
#define GODOT_LIVEKIT_THREAD_POOL_H

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

// Singleton pool for threads that outlive their owning object.
// Instead of calling std::thread::detach() (which leaks), callers
// move their thread + a shared done-flag into the pool.  At shutdown
// the pool tries to join every thread within a timeout.
class ZombieThreadPool {
public:
	struct Entry {
		std::thread thread;
		std::shared_ptr<std::atomic<bool>> done;
	};

	static ZombieThreadPool &instance() {
		static ZombieThreadPool pool;
		return pool;
	}

	// Move a stuck thread into the pool for later cleanup.
	void add(Entry &&entry) {
		std::lock_guard<std::mutex> lock(mutex_);
		entries_.push_back(std::move(entry));
	}

	// Try to join all pooled threads within timeout_ms.
	// Threads that finish in time are joined; those still stuck are
	// detached as an absolute last resort (process is shutting down).
	void join_all(int timeout_ms = 3000) {
		std::lock_guard<std::mutex> lock(mutex_);
		auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
		for (auto &e : entries_) {
			if (!e.thread.joinable()) {
				continue;
			}
			// Poll until done or deadline reached.
			while (!e.done->load()) {
				if (std::chrono::steady_clock::now() >= deadline) {
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			if (e.done->load()) {
				e.thread.join();
			} else {
				e.thread.detach();
			}
		}
		entries_.clear();
	}

private:
	ZombieThreadPool() = default;
	ZombieThreadPool(const ZombieThreadPool &) = delete;
	ZombieThreadPool &operator=(const ZombieThreadPool &) = delete;

	std::mutex mutex_;
	std::vector<Entry> entries_;
};

#endif // GODOT_LIVEKIT_THREAD_POOL_H
