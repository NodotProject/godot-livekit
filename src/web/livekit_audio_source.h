#ifndef GODOT_LIVEKIT_AUDIO_SOURCE_H
#define GODOT_LIVEKIT_AUDIO_SOURCE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

namespace godot {

// Web implementation of LiveKitAudioSource: captured frames are played into a MediaStreamTrack
// by an AudioWorklet. Unlike the native binding, frames needn't be exactly 10ms; audio at other
// sample rates is resampled to the browser's AudioContext rate.
class LiveKitAudioSource : public RefCounted {
    GDCLASS(LiveKitAudioSource, RefCounted)

private:
    int js_id_ = 0;
    int sample_rate_ = 48000;
    int num_channels_ = 1;

protected:
    static void _bind_methods();

public:
    ~LiveKitAudioSource();

    static Ref<LiveKitAudioSource> create(int sample_rate, int num_channels, int queue_size_ms);

    void capture_frame(const PackedFloat32Array &data, int sample_rate, int num_channels, int samples_per_channel);
    void clear_queue();
    double get_queued_duration() const;

    int get_sample_rate() const;
    int get_num_channels() const;

    int get_js_id() const { return js_id_; }
};

}

#endif // GODOT_LIVEKIT_AUDIO_SOURCE_H
