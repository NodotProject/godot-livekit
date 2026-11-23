#include "livekit_audio_source.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <exception>

using namespace godot;

void LiveKitAudioSource::_bind_methods() {
    ClassDB::bind_static_method("LiveKitAudioSource", D_METHOD("create", "sample_rate", "num_channels", "queue_size_ms"), &LiveKitAudioSource::create, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("capture_frame", "data", "sample_rate", "num_channels", "samples_per_channel"), &LiveKitAudioSource::capture_frame);
    ClassDB::bind_method(D_METHOD("clear_queue"), &LiveKitAudioSource::clear_queue);
    ClassDB::bind_method(D_METHOD("get_queued_duration"), &LiveKitAudioSource::get_queued_duration);
    ClassDB::bind_method(D_METHOD("get_sample_rate"), &LiveKitAudioSource::get_sample_rate);
    ClassDB::bind_method(D_METHOD("get_num_channels"), &LiveKitAudioSource::get_num_channels);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "sample_rate"), "", "get_sample_rate");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "num_channels"), "", "get_num_channels");
}

LiveKitAudioSource::LiveKitAudioSource() {
}

LiveKitAudioSource::~LiveKitAudioSource() {
}

Ref<LiveKitAudioSource> LiveKitAudioSource::create(int sample_rate, int num_channels, int queue_size_ms) {
    auto native_source = std::make_shared<livekit::AudioSource>(sample_rate, num_channels, queue_size_ms);

    Ref<LiveKitAudioSource> source;
    source.instantiate();
    source->source_ = native_source;
    source->sample_rate_ = sample_rate;
    source->num_channels_ = num_channels;
    source->direct_capture_ = queue_size_ms == 0;
    return source;
}

void LiveKitAudioSource::capture_frame(const PackedFloat32Array &data, int sample_rate, int num_channels, int samples_per_channel) {
    if (!source_) {
        UtilityFunctions::push_error("LiveKitAudioSource::capture_frame: source not initialized");
        return;
    }
    // LiveKit's SDK reports these errors asynchronously and stops waiting for them after 20ms, so
    // under load they would be dropped silently.
    if (sample_rate != sample_rate_ || num_channels != num_channels_) {
        UtilityFunctions::push_error("LiveKitAudioSource::capture_frame: the frame is ", sample_rate, " Hz with ",
                num_channels, " channel(s), but the source is ", sample_rate_, " Hz with ", num_channels_,
                " channel(s)");
        return;
    }
    if (direct_capture_ && samples_per_channel != sample_rate / 100) {
        UtilityFunctions::push_error("LiveKitAudioSource::capture_frame: a source with no queue needs 10ms frames (",
                sample_rate / 100, " samples per channel), not ", samples_per_channel);
        return;
    }

    // Convert float32 to int16
    std::vector<int16_t> pcm_data(data.size());
    for (int i = 0; i < data.size(); i++) {
        float sample = data[i];
        // Clamp to [-1, 1]
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        pcm_data[i] = static_cast<int16_t>(sample * 32767.0f);
    }

    // Exceptions (e.g. for data that doesn't match samples_per_channel) must not escape into Godot,
    // where they would terminate the process.
    try {
        livekit::AudioFrame frame(std::move(pcm_data), sample_rate, num_channels, samples_per_channel);
        source_->captureFrame(frame);
    } catch (const std::exception &e) {
        UtilityFunctions::push_error("LiveKitAudioSource::capture_frame: ", String(e.what()));
    }
}

void LiveKitAudioSource::clear_queue() {
    if (source_) {
        source_->clearQueue();
    }
}

double LiveKitAudioSource::get_queued_duration() const {
    if (source_) {
        return source_->queuedDuration();
    }
    return 0.0;
}

int LiveKitAudioSource::get_sample_rate() const {
    return sample_rate_;
}

int LiveKitAudioSource::get_num_channels() const {
    return num_channels_;
}
