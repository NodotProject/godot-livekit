#ifndef GODOT_LIVEKIT_AUDIO_PROCESSING_MODULE_H
#define GODOT_LIVEKIT_AUDIO_PROCESSING_MODULE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

#include <livekit/audio_processing_module.h>

#include <memory>

namespace godot {

// Exposes WebRTC's audio processing (echo cancellation, noise suppression,
// gain control, high-pass filter) for audio captured and played by Godot.
// Audio must be a multiple of 10ms long.
class LiveKitAudioProcessingModule : public RefCounted {
    GDCLASS(LiveKitAudioProcessingModule, RefCounted)

private:
    std::unique_ptr<livekit::AudioProcessingModule> apm_;

    bool _process(const PackedFloat32Array &data, int sample_rate, int num_channels, bool reverse, PackedFloat32Array &out);

protected:
    static void _bind_methods();

public:
    LiveKitAudioProcessingModule();
    ~LiveKitAudioProcessingModule();

    static Ref<LiveKitAudioProcessingModule> create(const Dictionary &options);

    PackedFloat32Array process_stream(const PackedFloat32Array &data, int sample_rate, int num_channels);
    bool process_reverse_stream(const PackedFloat32Array &data, int sample_rate, int num_channels);
    bool set_stream_delay_ms(int delay_ms);
};

}

#endif // GODOT_LIVEKIT_AUDIO_PROCESSING_MODULE_H
