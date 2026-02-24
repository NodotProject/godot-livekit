#ifndef GODOT_LIVEKIT_AUDIO_STREAM_H
#define GODOT_LIVEKIT_AUDIO_STREAM_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <livekit/audio_stream.h>
#include <livekit/audio_frame.h>

#include "livekit_track.h"
#include "livekit_participant.h"

#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>

namespace godot {

class LiveKitAudioStream : public RefCounted {
    GDCLASS(LiveKitAudioStream, RefCounted)

private:
    std::shared_ptr<livekit::AudioStream> stream_;
    std::mutex audio_mutex_;
    std::vector<float> audio_buffer_;
    std::thread reader_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> thread_exited_{false};
    std::atomic<int> sample_rate_{48000};
    std::atomic<int> num_channels_{1};

    void _reader_loop();

protected:
    static void _bind_methods();

public:
    LiveKitAudioStream();
    ~LiveKitAudioStream();

    static Ref<LiveKitAudioStream> from_track(const Ref<LiveKitTrack> &track);
    static Ref<LiveKitAudioStream> from_participant(const Ref<LiveKitRemoteParticipant> &participant, int source);

    int get_sample_rate() const;
    int get_num_channels() const;
    int poll(const Ref<AudioStreamGeneratorPlayback> &playback);
    void close();
};

}

#endif // GODOT_LIVEKIT_AUDIO_STREAM_H
