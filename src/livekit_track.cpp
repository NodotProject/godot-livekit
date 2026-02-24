#include "livekit_track.h"
#include "livekit_audio_source.h"
#include "livekit_video_source.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// LiveKitTrack

void LiveKitTrack::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_sid"), &LiveKitTrack::get_sid);
    ClassDB::bind_method(D_METHOD("get_name"), &LiveKitTrack::get_name);
    ClassDB::bind_method(D_METHOD("get_kind"), &LiveKitTrack::get_kind);
    ClassDB::bind_method(D_METHOD("get_source"), &LiveKitTrack::get_source);
    ClassDB::bind_method(D_METHOD("get_muted"), &LiveKitTrack::get_muted);
    ClassDB::bind_method(D_METHOD("get_stream_state"), &LiveKitTrack::get_stream_state);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "sid"), "", "get_sid");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "kind"), "", "get_kind");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "source"), "", "get_source");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "muted"), "", "get_muted");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stream_state"), "", "get_stream_state");

    BIND_ENUM_CONSTANT(KIND_UNKNOWN);
    BIND_ENUM_CONSTANT(KIND_AUDIO);
    BIND_ENUM_CONSTANT(KIND_VIDEO);

    BIND_ENUM_CONSTANT(SOURCE_UNKNOWN);
    BIND_ENUM_CONSTANT(SOURCE_CAMERA);
    BIND_ENUM_CONSTANT(SOURCE_MICROPHONE);
    BIND_ENUM_CONSTANT(SOURCE_SCREENSHARE);
    BIND_ENUM_CONSTANT(SOURCE_SCREENSHARE_AUDIO);

    BIND_ENUM_CONSTANT(STATE_UNKNOWN);
    BIND_ENUM_CONSTANT(STATE_ACTIVE);
    BIND_ENUM_CONSTANT(STATE_PAUSED);
}

LiveKitTrack::LiveKitTrack() {
}

LiveKitTrack::~LiveKitTrack() {
}

void LiveKitTrack::bind_track(const std::shared_ptr<livekit::Track> &track) {
    track_ = track;
}

String LiveKitTrack::get_sid() const {
    if (track_) {
        return String(track_->sid().c_str());
    }
    return String();
}

String LiveKitTrack::get_name() const {
    if (track_) {
        return String(track_->name().c_str());
    }
    return String();
}

int LiveKitTrack::get_kind() const {
    if (track_) {
        return (int)track_->kind();
    }
    return KIND_UNKNOWN;
}

int LiveKitTrack::get_source() const {
    if (track_) {
        auto src = track_->source();
        if (src.has_value()) {
            return (int)src.value();
        }
    }
    return SOURCE_UNKNOWN;
}

bool LiveKitTrack::get_muted() const {
    if (track_) {
        return track_->muted();
    }
    return false;
}

int LiveKitTrack::get_stream_state() const {
    if (track_) {
        return (int)track_->stream_state();
    }
    return STATE_UNKNOWN;
}

// LiveKitLocalAudioTrack

void LiveKitLocalAudioTrack::_bind_methods() {
    ClassDB::bind_static_method("LiveKitLocalAudioTrack", D_METHOD("create", "name", "source"), &LiveKitLocalAudioTrack::create);
    ClassDB::bind_method(D_METHOD("mute"), &LiveKitLocalAudioTrack::mute);
    ClassDB::bind_method(D_METHOD("unmute"), &LiveKitLocalAudioTrack::unmute);
}

LiveKitLocalAudioTrack::LiveKitLocalAudioTrack() {
}

LiveKitLocalAudioTrack::~LiveKitLocalAudioTrack() {
}

Ref<LiveKitLocalAudioTrack> LiveKitLocalAudioTrack::create(const String &name, const Ref<LiveKitAudioSource> &source) {
    if (source.is_null()) {
        UtilityFunctions::printerr("LiveKitLocalAudioTrack::create: source is null");
        return Ref<LiveKitLocalAudioTrack>();
    }

    auto native_source = source->get_native_source();
    if (!native_source) {
        UtilityFunctions::printerr("LiveKitLocalAudioTrack::create: native source is null");
        return Ref<LiveKitLocalAudioTrack>();
    }

    auto native_track = livekit::LocalAudioTrack::createLocalAudioTrack(
            std::string(name.utf8().get_data()), native_source);

    if (!native_track) {
        UtilityFunctions::printerr("LiveKitLocalAudioTrack::create: failed to create native track");
        return Ref<LiveKitLocalAudioTrack>();
    }

    Ref<LiveKitLocalAudioTrack> track;
    track.instantiate();
    track->bind_track(native_track);
    return track;
}

void LiveKitLocalAudioTrack::mute() {
    if (track_) {
        auto local = std::dynamic_pointer_cast<livekit::LocalAudioTrack>(track_);
        if (local) {
            local->mute();
        }
    }
}

void LiveKitLocalAudioTrack::unmute() {
    if (track_) {
        auto local = std::dynamic_pointer_cast<livekit::LocalAudioTrack>(track_);
        if (local) {
            local->unmute();
        }
    }
}

// LiveKitLocalVideoTrack

void LiveKitLocalVideoTrack::_bind_methods() {
    ClassDB::bind_static_method("LiveKitLocalVideoTrack", D_METHOD("create", "name", "source"), &LiveKitLocalVideoTrack::create);
    ClassDB::bind_method(D_METHOD("mute"), &LiveKitLocalVideoTrack::mute);
    ClassDB::bind_method(D_METHOD("unmute"), &LiveKitLocalVideoTrack::unmute);
}

LiveKitLocalVideoTrack::LiveKitLocalVideoTrack() {
}

LiveKitLocalVideoTrack::~LiveKitLocalVideoTrack() {
}

Ref<LiveKitLocalVideoTrack> LiveKitLocalVideoTrack::create(const String &name, const Ref<LiveKitVideoSource> &source) {
    if (source.is_null()) {
        UtilityFunctions::printerr("LiveKitLocalVideoTrack::create: source is null");
        return Ref<LiveKitLocalVideoTrack>();
    }

    auto native_source = source->get_native_source();
    if (!native_source) {
        UtilityFunctions::printerr("LiveKitLocalVideoTrack::create: native source is null");
        return Ref<LiveKitLocalVideoTrack>();
    }

    auto native_track = livekit::LocalVideoTrack::createLocalVideoTrack(
            std::string(name.utf8().get_data()), native_source);

    if (!native_track) {
        UtilityFunctions::printerr("LiveKitLocalVideoTrack::create: failed to create native track");
        return Ref<LiveKitLocalVideoTrack>();
    }

    Ref<LiveKitLocalVideoTrack> track;
    track.instantiate();
    track->bind_track(native_track);
    return track;
}

void LiveKitLocalVideoTrack::mute() {
    if (track_) {
        auto local = std::dynamic_pointer_cast<livekit::LocalVideoTrack>(track_);
        if (local) {
            local->mute();
        }
    }
}

void LiveKitLocalVideoTrack::unmute() {
    if (track_) {
        auto local = std::dynamic_pointer_cast<livekit::LocalVideoTrack>(track_);
        if (local) {
            local->unmute();
        }
    }
}

// LiveKitRemoteAudioTrack

void LiveKitRemoteAudioTrack::_bind_methods() {
}

LiveKitRemoteAudioTrack::LiveKitRemoteAudioTrack() {
}

LiveKitRemoteAudioTrack::~LiveKitRemoteAudioTrack() {
}

// LiveKitRemoteVideoTrack

void LiveKitRemoteVideoTrack::_bind_methods() {
}

LiveKitRemoteVideoTrack::LiveKitRemoteVideoTrack() {
}

LiveKitRemoteVideoTrack::~LiveKitRemoteVideoTrack() {
}
