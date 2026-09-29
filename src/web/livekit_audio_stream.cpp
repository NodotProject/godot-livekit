#include "livekit_audio_stream.h"

#include "web_audio.h"
#include "web_bridge.h"

#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

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

LiveKitAudioStream::~LiveKitAudioStream() {
    close();
}

Ref<LiveKitAudioStream> LiveKitAudioStream::from_track(const Ref<LiveKitTrack> &track) {
    if (track.is_null() || !track->get_js_id()) {
        UtilityFunctions::push_error("LiveKitAudioStream::from_track: invalid track");
        return Ref<LiveKitAudioStream>();
    }
    Variant id = web_call("stream_from_track", Array::make(track->get_js_id()));
    if (id.get_type() == Variant::NIL) {
        return Ref<LiveKitAudioStream>();
    }
    Ref<LiveKitAudioStream> stream;
    stream.instantiate();
    stream->js_id_ = id;
    return stream;
}

Ref<LiveKitAudioStream> LiveKitAudioStream::from_participant(const Ref<LiveKitRemoteParticipant> &participant, int source) {
    if (participant.is_null() || !participant->get_room_js_id()) {
        UtilityFunctions::push_error("LiveKitAudioStream::from_participant: invalid participant");
        return Ref<LiveKitAudioStream>();
    }
    Variant id = web_call("stream_from_participant", Array::make(participant->get_room_js_id(), participant->get_identity(), source));
    if (id.get_type() == Variant::NIL) {
        return Ref<LiveKitAudioStream>();
    }
    Ref<LiveKitAudioStream> stream;
    stream.instantiate();
    stream->js_id_ = id;
    return stream;
}

int LiveKitAudioStream::get_sample_rate() const {
    if (!js_id_) {
        return 48000;
    }
    return web_call_dict("stream_info", Array::make(js_id_)).get("sample_rate", 48000);
}

int LiveKitAudioStream::get_num_channels() const {
    if (!js_id_) {
        return 1;
    }
    return web_call_dict("stream_info", Array::make(js_id_)).get("num_channels", 1);
}

int LiveKitAudioStream::poll(const Ref<AudioStreamGeneratorPlayback> &playback) {
    if (!js_id_ || playback.is_null()) {
        return 0;
    }
    int available = playback->get_frames_available();
    if (available <= 0) {
        return 0;
    }
    buffer_.resize(available * 2);
    int frames = web_stream_read(js_id_, buffer_.data(), available);
    if (frames <= 0) {
        return 0;
    }
    PackedVector2Array push_array;
    push_array.resize(frames);
    Vector2 *out = push_array.ptrw();
    for (int i = 0; i < frames; i++) {
        out[i] = Vector2(buffer_[i * 2], buffer_[i * 2 + 1]);
    }
    playback->push_buffer(push_array);
    return frames;
}

void LiveKitAudioStream::close() {
    if (js_id_) {
        web_call("stream_close", Array::make(js_id_));
        js_id_ = 0;
    }
}
