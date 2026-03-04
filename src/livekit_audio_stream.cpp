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
    ring_.resize(kMaxAudioBufferSamples, 0.0f);
}

LiveKitAudioStream::~LiveKitAudioStream() {
    alive_->store(false);
    close();
}

void LiveKitAudioStream::_reader_loop() {
    // Capture a local copy so the native stream stays alive even if
    // close() calls stream_.reset() on another thread.
    auto stream = stream_;
    if (!stream) return;
    auto alive = alive_;
    while (running_.load() && alive->load()) {
        livekit::AudioFrameEvent event;
        bool ok = stream->read(event);
        if (!ok) {
            break;
        }
        if (!alive->load()) {
            break;
        }

        const auto &frame = event.frame;
        const auto &pcm_data = frame.data();
        int channels = frame.num_channels();

        // Update sample rate/channels from actual frame data
        sample_rate_ = frame.sample_rate();
        num_channels_ = channels;

        // Convert int16 interleaved PCM to float32 using persistent buffer.
        size_t sample_count = pcm_data.size();
        reader_float_samples_.resize(sample_count);
        for (size_t i = 0; i < sample_count; i++) {
            reader_float_samples_[i] = pcm_data[i] / 32768.0f;
        }

        {
            std::lock_guard<std::mutex> lock(audio_mutex_);
            // Write sample-by-sample into ring buffer.
            // If full, overwrite oldest samples (advance head).
            for (size_t i = 0; i < sample_count; i++) {
                ring_[ring_tail_] = reader_float_samples_[i];
                ring_tail_ = (ring_tail_ + 1) % kMaxAudioBufferSamples;
                if (ring_count_ == kMaxAudioBufferSamples) {
                    // Overwrite oldest — advance head.
                    ring_head_ = (ring_head_ + 1) % kMaxAudioBufferSamples;
                } else {
                    ring_count_++;
                }
            }
        }
    }
}

Ref<LiveKitAudioStream> LiveKitAudioStream::from_track(const Ref<LiveKitTrack> &track) {
    if (track.is_null() || !track->get_native_track()) {
        UtilityFunctions::push_error("LiveKitAudioStream::from_track: invalid track");
        return Ref<LiveKitAudioStream>();
    }

    livekit::AudioStream::Options opts;

    auto native_stream = livekit::AudioStream::fromTrack(track->get_native_track(), opts);
    if (!native_stream) {
        UtilityFunctions::push_error("LiveKitAudioStream::from_track: failed to create stream");
        return Ref<LiveKitAudioStream>();
    }

    Ref<LiveKitAudioStream> stream;
    stream.instantiate();
    stream->stream_ = native_stream;
    stream->running_.store(true);
    // Capture the alive sentinel so the thread can detect if the object is freed.
    auto alive = stream->alive_;
    stream->reader_thread_.start([raw = stream.ptr(), alive]() {
        if (alive->load()) { raw->_reader_loop(); }
    });

    return stream;
}

Ref<LiveKitAudioStream> LiveKitAudioStream::from_participant(const Ref<LiveKitRemoteParticipant> &participant, int source) {
    if (participant.is_null() || !participant->get_native_remote_participant()) {
        UtilityFunctions::push_error("LiveKitAudioStream::from_participant: invalid participant");
        return Ref<LiveKitAudioStream>();
    }

    livekit::AudioStream::Options opts;

    auto native_stream = livekit::AudioStream::fromParticipant(
            *participant->get_native_remote_participant(),
            (livekit::TrackSource)source,
            opts);
    if (!native_stream) {
        UtilityFunctions::push_error("LiveKitAudioStream::from_participant: failed to create stream");
        return Ref<LiveKitAudioStream>();
    }

    Ref<LiveKitAudioStream> stream;
    stream.instantiate();
    stream->stream_ = native_stream;
    stream->running_.store(true);
    auto alive = stream->alive_;
    stream->reader_thread_.start([raw = stream.ptr(), alive]() {
        if (alive->load()) { raw->_reader_loop(); }
    });

    return stream;
}

int LiveKitAudioStream::get_sample_rate() const {
    return sample_rate_.load();
}

int LiveKitAudioStream::get_num_channels() const {
    return num_channels_.load();
}

int LiveKitAudioStream::poll(const Ref<AudioStreamGeneratorPlayback> &playback) {
    if (playback.is_null()) {
        return 0;
    }

    std::vector<float> buffer;
    {
        std::unique_lock<std::mutex> lock(audio_mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            uint32_t count = lock_contention_count_.fetch_add(1) + 1;
            if (count == 1 || (count % 100) == 0) {
                UtilityFunctions::push_warning("LiveKitAudioStream::poll: lock contention (", count, " skipped frames)");
            }
            return 0;
        }
        if (ring_count_ == 0) {
            return 0;
        }
        // Drain ring buffer into local buffer.
        buffer.resize(ring_count_);
        for (size_t i = 0; i < ring_count_; i++) {
            buffer[i] = ring_[(ring_head_ + i) % kMaxAudioBufferSamples];
        }
        ring_head_ = 0;
        ring_tail_ = 0;
        ring_count_ = 0;
    }

    int channels = num_channels_.load();
    if (channels <= 0) {
        channels = 1;
    }

    int frames = buffer.size() / channels;
    if (frames == 0) {
        return 0;
    }

    // Push as many frames as the playback buffer can accept
    int can_push = playback->get_frames_available();
    int to_push = (frames < can_push) ? frames : can_push;

    if (to_push > 0) {
        // Convert to Vector2 frames (stereo: L/R, mono: duplicate to both channels)
        PackedVector2Array push_array;
        push_array.resize(to_push);
        for (int i = 0; i < to_push; i++) {
            float left = buffer[i * channels];
            float right = (channels > 1) ? buffer[i * channels + 1] : left;
            push_array.set(i, Vector2(left, right));
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
    reader_thread_.join_or_detach(2000);
    stream_.reset();
}
