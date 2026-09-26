#include "livekit_audio_processing_module.h"

#include <godot_cpp/variant/utility_functions.hpp>

#include <exception>
#include <vector>

using namespace godot;

void LiveKitAudioProcessingModule::_bind_methods() {
    ClassDB::bind_static_method("LiveKitAudioProcessingModule", D_METHOD("create", "options"), &LiveKitAudioProcessingModule::create, DEFVAL(Dictionary()));
    ClassDB::bind_method(D_METHOD("process_stream", "data", "sample_rate", "num_channels"), &LiveKitAudioProcessingModule::process_stream);
    ClassDB::bind_method(D_METHOD("process_reverse_stream", "data", "sample_rate", "num_channels"), &LiveKitAudioProcessingModule::process_reverse_stream);
    ClassDB::bind_method(D_METHOD("set_stream_delay_ms", "delay_ms"), &LiveKitAudioProcessingModule::set_stream_delay_ms);
}

LiveKitAudioProcessingModule::LiveKitAudioProcessingModule() {
}

LiveKitAudioProcessingModule::~LiveKitAudioProcessingModule() {
}

// Options (all default to false): echo_cancellation, noise_suppression,
// high_pass_filter, auto_gain_control.
Ref<LiveKitAudioProcessingModule> LiveKitAudioProcessingModule::create(const Dictionary &options) {
    livekit::AudioProcessingModule::Options native_options;
    native_options.echo_cancellation = options.get("echo_cancellation", false);
    native_options.noise_suppression = options.get("noise_suppression", false);
    native_options.high_pass_filter = options.get("high_pass_filter", false);
    native_options.auto_gain_control = options.get("auto_gain_control", false);

    Ref<LiveKitAudioProcessingModule> apm;
    try {
        auto native_apm = std::make_unique<livekit::AudioProcessingModule>(native_options);
        apm.instantiate();
        apm->apm_ = std::move(native_apm);
    } catch (const std::exception &e) {
        UtilityFunctions::push_error("LiveKitAudioProcessingModule::create: ", String(e.what()));
    }
    return apm;
}

// Processes near-end (microphone) audio, returning the processed samples, or
// an empty array on failure.
PackedFloat32Array LiveKitAudioProcessingModule::process_stream(const PackedFloat32Array &data, int sample_rate, int num_channels) {
    PackedFloat32Array out;
    _process(data, sample_rate, num_channels, false, out);
    return out;
}

// Provides far-end (speaker) audio as the echo cancellation reference.
bool LiveKitAudioProcessingModule::process_reverse_stream(const PackedFloat32Array &data, int sample_rate, int num_channels) {
    PackedFloat32Array unused;
    return _process(data, sample_rate, num_channels, true, unused);
}

bool LiveKitAudioProcessingModule::_process(const PackedFloat32Array &data, int sample_rate, int num_channels, bool reverse, PackedFloat32Array &out) {
    if (!apm_ || num_channels <= 0) {
        UtilityFunctions::push_error("LiveKitAudioProcessingModule: not initialized or invalid channel count");
        return false;
    }
    // The native APM panics (aborting the process) unless frames are exactly 10ms.
    if (sample_rate <= 0 || sample_rate % 100 != 0 || data.size() != (sample_rate / 100) * num_channels) {
        UtilityFunctions::push_error("LiveKitAudioProcessingModule: frames must contain exactly 10ms of audio (",
                (sample_rate / 100) * num_channels, " samples at ", sample_rate, "Hz), got ", data.size());
        return false;
    }
    std::vector<int16_t> pcm(data.size());
    for (int i = 0; i < data.size(); i++) {
        float sample = CLAMP(data[i], -1.0f, 1.0f);
        pcm[i] = static_cast<int16_t>(sample * 32767.0f);
    }
    // Exceptions must not escape into Godot, where they would terminate the process.
    try {
        livekit::AudioFrame frame(std::move(pcm), sample_rate, num_channels, data.size() / num_channels);
        if (reverse) {
            apm_->processReverseStream(frame);
            return true;
        }
        apm_->processStream(frame);
        const auto &processed = frame.data();
        out.resize(processed.size());
        float *out_ptr = out.ptrw();
        for (size_t i = 0; i < processed.size(); i++) {
            out_ptr[i] = processed[i] / 32768.0f;
        }
        return true;
    } catch (const std::exception &e) {
        UtilityFunctions::push_error("LiveKitAudioProcessingModule: ", String(e.what()));
        return false;
    }
}

// Sets the estimated delay between a far-end frame being provided and its echo
// arriving via process_stream. Required when echo cancellation is enabled.
bool LiveKitAudioProcessingModule::set_stream_delay_ms(int delay_ms) {
    if (!apm_) {
        return false;
    }
    try {
        apm_->setStreamDelayMs(delay_ms);
        return true;
    } catch (const std::exception &e) {
        UtilityFunctions::push_error("LiveKitAudioProcessingModule::set_stream_delay_ms: ", String(e.what()));
        return false;
    }
}
