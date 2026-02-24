#ifndef GODOT_LIVEKIT_AUDIO_SOURCE_H
#define GODOT_LIVEKIT_AUDIO_SOURCE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

#include <livekit/audio_source.h>
#include <livekit/audio_frame.h>

#include <memory>

namespace godot {

class LiveKitAudioSource : public RefCounted {
    GDCLASS(LiveKitAudioSource, RefCounted)

private:
    std::shared_ptr<livekit::AudioSource> source_;
    int sample_rate_ = 48000;
    int num_channels_ = 1;

protected:
    static void _bind_methods();

public:
    LiveKitAudioSource();
    ~LiveKitAudioSource();

    static Ref<LiveKitAudioSource> create(int sample_rate, int num_channels, int queue_size_ms);

    void capture_frame(const PackedFloat32Array &data, int sample_rate, int num_channels, int samples_per_channel);
    void clear_queue();
    double get_queued_duration() const;

    int get_sample_rate() const;
    int get_num_channels() const;

    std::shared_ptr<livekit::AudioSource> get_native_source() const { return source_; }
};

}

#endif // GODOT_LIVEKIT_AUDIO_SOURCE_H
