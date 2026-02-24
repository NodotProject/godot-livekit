#include "livekit_audio_stream.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/vector2.hpp>

using namespace godot;

void LiveKitAudioStream::_bind_methods() {
    ClassDB::bind_static_method("LiveKitAudioStream", D_METHOD("from_track", "track"), &LiveKitAudioStream::from_track);
    ClassDB::bind_static_method("LiveKitAudioStream", D_METHOD("from_participant", "participant", "source"), &LiveKitAudioStream::from_participant);

    ClassDB::bind_method(D_METHOD("get_sample_rate"), &LiveKitAudioStream::get_sample_rate);
    ClassDB::bind_method(D_METHOD("get_num_channels"), &LiveKitAudioStream::get_num_channels);
    ClassDB::bind_method(D_METHOD("poll", "playback"), &LiveKitAudioStream::poll);
    ClassDB::bind_method(D_METHOD("close"), &LiveKitAudioStream::close);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "sample_rate"), "", "get_sample_rate");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "num_channels"), "", "get_num_channels");
}

LiveKitAudioStream::LiveKitAudioStream() {
}

LiveKitAudioStream::~LiveKitAudioStream() {
    close();
}

void LiveKitAudioStream::_reader_loop() {
    while (running_.load()) {
        livekit::AudioFrameEvent event;
        bool ok = stream_->read(event);
        if (!ok) {
            break;
        }

        const auto &frame = event.frame;
        const auto &pcm_data = frame.data();
        int channels = frame.num_channels();
        int samples_per_ch = frame.samples_per_channel();

        // Update sample rate/channels from actual frame data
        sample_rate_ = frame.sample_rate();
        num_channels_ = channels;

        // Convert int16 interleaved PCM to float32
        std::vector<float> float_samples(pcm_data.size());
        for (size_t i = 0; i < pcm_data.size(); i++) {
            float_samples[i] = pcm_data[i] / 32768.0f;
        }

        {
            std::lock_guard<std::mutex> lock(audio_mutex_);
            audio_buffer_.insert(audio_buffer_.end(), float_samples.begin(), float_samples.end());
        }
    }
    thread_exited_.store(true);
}

Ref<LiveKitAudioStream> LiveKitAudioStream::from_track(const Ref<LiveKitTrack> &track) {
    if (track.is_null() || !track->get_native_track()) {
        UtilityFunctions::printerr("LiveKitAudioStream::from_track: invalid track");
        return Ref<LiveKitAudioStream>();
    }

    livekit::AudioStream::Options opts;

    auto native_stream = livekit::AudioStream::fromTrack(track->get_native_track(), opts);
    if (!native_stream) {
        UtilityFunctions::printerr("LiveKitAudioStream::from_track: failed to create stream");
        return Ref<LiveKitAudioStream>();
    }

    Ref<LiveKitAudioStream> stream;
    stream.instantiate();
    stream->stream_ = native_stream;
    stream->running_.store(true);
    stream->reader_thread_ = std::thread(&LiveKitAudioStream::_reader_loop, stream.ptr());

    return stream;
}

Ref<LiveKitAudioStream> LiveKitAudioStream::from_participant(const Ref<LiveKitRemoteParticipant> &participant, int source) {
    if (participant.is_null() || !participant->get_native_remote_participant()) {
        UtilityFunctions::printerr("LiveKitAudioStream::from_participant: invalid participant");
        return Ref<LiveKitAudioStream>();
    }

    livekit::AudioStream::Options opts;

    auto native_stream = livekit::AudioStream::fromParticipant(
            *participant->get_native_remote_participant(),
            (livekit::TrackSource)source,
            opts);
    if (!native_stream) {
        UtilityFunctions::printerr("LiveKitAudioStream::from_participant: failed to create stream");
        return Ref<LiveKitAudioStream>();
    }

    Ref<LiveKitAudioStream> stream;
    stream.instantiate();
    stream->stream_ = native_stream;
    stream->running_.store(true);
    stream->reader_thread_ = std::thread(&LiveKitAudioStream::_reader_loop, stream.ptr());

    return stream;
}

int LiveKitAudioStream::get_sample_rate() const {
    return sample_rate_;
}

int LiveKitAudioStream::get_num_channels() const {
    return num_channels_;
}

int LiveKitAudioStream::poll(const Ref<AudioStreamGeneratorPlayback> &playback) {
    if (playback.is_null()) {
        return 0;
    }

    std::vector<float> buffer;
    {
        std::unique_lock<std::mutex> lock(audio_mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            return 0; // Reader thread holds the lock; skip this frame
        }
        buffer.swap(audio_buffer_);
    }

    if (buffer.empty()) {
        return 0;
    }

    int channels = num_channels_;
    if (channels <= 0) {
        channels = 1;
    }

    int frames = buffer.size() / channels;
    if (frames == 0) {
        return 0;
    }

    // Convert to Vector2 frames (stereo: L/R, mono: duplicate to both channels)
    PackedVector2Array frames_array;
    frames_array.resize(frames);

    for (int i = 0; i < frames; i++) {
        float left = buffer[i * channels];
        float right = (channels > 1) ? buffer[i * channels + 1] : left;
        frames_array.set(i, Vector2(left, right));
    }

    // Push as many frames as the playback buffer can accept
    int can_push = playback->get_frames_available();
    int to_push = (frames < can_push) ? frames : can_push;

    if (to_push > 0) {
        // Push frame by frame since push_buffer takes the full array
        PackedVector2Array push_array;
        push_array.resize(to_push);
        for (int i = 0; i < to_push; i++) {
            push_array.set(i, frames_array[i]);
        }
        playback->push_buffer(push_array);
    }

    return to_push;
}

void LiveKitAudioStream::close() {
    running_.store(false);
    if (stream_) {
        stream_->close();
    }
    if (reader_thread_.joinable()) {
        // stream_->close() should make read() return false almost immediately.
        // Brief poll as a safety net; detach if the thread doesn't exit.
        for (int i = 0; i < 20 && !thread_exited_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (thread_exited_.load()) {
            reader_thread_.join();
        } else {
            reader_thread_.detach();
        }
    }
    stream_.reset();
}
