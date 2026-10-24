#include "livekit_audio_source.h"

#include "web_audio.h"
#include "web_bridge.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>

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

LiveKitAudioSource::~LiveKitAudioSource() {
    if (js_id_) {
        web_call("source_destroy", Array::make(js_id_));
    }
}

Ref<LiveKitAudioSource> LiveKitAudioSource::create(int sample_rate, int num_channels, int queue_size_ms) {
    Ref<LiveKitAudioSource> source;
    source.instantiate();
    source->sample_rate_ = sample_rate;
    source->num_channels_ = num_channels;
    source->js_id_ = web_call("source_create", Array::make(sample_rate, num_channels, queue_size_ms));
    return source;
}

void LiveKitAudioSource::capture_frame(const PackedFloat32Array &data, int sample_rate, int num_channels, int samples_per_channel) {
    if (!js_id_) {
        UtilityFunctions::push_error("LiveKitAudioSource::capture_frame: source not initialized");
        return;
    }
    if (sample_rate <= 0 || num_channels <= 0) {
        UtilityFunctions::push_error("LiveKitAudioSource::capture_frame: invalid sample rate or channel count");
        return;
    }
    int count = std::min<int>(data.size(), samples_per_channel * num_channels);
    count -= count % num_channels;
    if (count > 0) {
        web_source_capture(js_id_, data.ptr(), count, sample_rate, num_channels);
    }
}

void LiveKitAudioSource::clear_queue() {
    if (js_id_) {
        web_call("source_clear", Array::make(js_id_));
    }
}

double LiveKitAudioSource::get_queued_duration() const {
    return js_id_ ? double(web_call("source_queued_duration", Array::make(js_id_))) : 0.0;
}

int LiveKitAudioSource::get_sample_rate() const {
    return sample_rate_;
}

int LiveKitAudioSource::get_num_channels() const {
    return num_channels_;
}
