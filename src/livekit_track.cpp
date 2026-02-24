#include "livekit_track.h"
#include "livekit_audio_source.h"
#include "livekit_video_source.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/dictionary.hpp>

#include <livekit/stats.h>

#include <chrono>
#include <variant>

using namespace godot;

namespace {

Dictionary rtc_stats_data_to_dict(const livekit::RtcStatsData &d) {
    Dictionary dict;
    dict["id"] = String(d.id.c_str());
    dict["timestamp_ms"] = d.timestamp_ms;
    return dict;
}

Dictionary inbound_rtp_to_dict(const livekit::RtcInboundRtpStats &s) {
    Dictionary dict;
    dict["type"] = "inbound_rtp";
    dict["id"] = String(s.rtc.id.c_str());
    dict["timestamp_ms"] = s.rtc.timestamp_ms;
    dict["ssrc"] = (int)s.stream.ssrc;
    dict["kind"] = String(s.stream.kind.c_str());
    dict["transport_id"] = String(s.stream.transport_id.c_str());
    dict["codec_id"] = String(s.stream.codec_id.c_str());
    dict["packets_received"] = (int64_t)s.received.packets_received;
    dict["packets_lost"] = (int64_t)s.received.packets_lost;
    dict["jitter"] = s.received.jitter;
    dict["track_identifier"] = String(s.inbound.track_identifier.c_str());
    dict["frames_decoded"] = (int)s.inbound.frames_decoded;
    dict["frames_dropped"] = (int)s.inbound.frames_dropped;
    dict["frame_width"] = (int)s.inbound.frame_width;
    dict["frame_height"] = (int)s.inbound.frame_height;
    dict["frames_per_second"] = s.inbound.frames_per_second;
    dict["bytes_received"] = (int64_t)s.inbound.bytes_received;
    dict["nack_count"] = (int)s.inbound.nack_count;
    dict["pli_count"] = (int)s.inbound.pli_count;
    dict["fir_count"] = (int)s.inbound.fir_count;
    dict["freeze_count"] = (int)s.inbound.freeze_count;
    dict["total_freeze_duration"] = s.inbound.total_freeze_duration;
    dict["audio_level"] = s.inbound.audio_level;
    dict["total_audio_energy"] = s.inbound.total_audio_energy;
    dict["total_samples_duration"] = s.inbound.total_samples_duration;
    dict["concealed_samples"] = (int64_t)s.inbound.concealed_samples;
    dict["jitter_buffer_delay"] = s.inbound.jitter_buffer_delay;
    return dict;
}

Dictionary outbound_rtp_to_dict(const livekit::RtcOutboundRtpStats &s) {
    Dictionary dict;
    dict["type"] = "outbound_rtp";
    dict["id"] = String(s.rtc.id.c_str());
    dict["timestamp_ms"] = s.rtc.timestamp_ms;
    dict["ssrc"] = (int)s.stream.ssrc;
    dict["kind"] = String(s.stream.kind.c_str());
    dict["transport_id"] = String(s.stream.transport_id.c_str());
    dict["codec_id"] = String(s.stream.codec_id.c_str());
    dict["packets_sent"] = (int64_t)s.sent.packets_sent;
    dict["bytes_sent"] = (int64_t)s.sent.bytes_sent;
    dict["frame_width"] = (int)s.outbound.frame_width;
    dict["frame_height"] = (int)s.outbound.frame_height;
    dict["frames_per_second"] = s.outbound.frames_per_second;
    dict["frames_sent"] = (int)s.outbound.frames_sent;
    dict["frames_encoded"] = (int)s.outbound.frames_encoded;
    dict["key_frames_encoded"] = (int)s.outbound.key_frames_encoded;
    dict["target_bitrate"] = s.outbound.target_bitrate;
    dict["total_encode_time"] = s.outbound.total_encode_time;
    dict["total_packet_send_delay"] = s.outbound.total_packet_send_delay;
    dict["quality_limitation_reason"] = (int)s.outbound.quality_limitation_reason;
    dict["nack_count"] = (int)s.outbound.nack_count;
    dict["pli_count"] = (int)s.outbound.pli_count;
    dict["fir_count"] = (int)s.outbound.fir_count;
    dict["active"] = s.outbound.active;
    dict["retransmitted_packets_sent"] = (int64_t)s.outbound.retransmitted_packets_sent;
    dict["retransmitted_bytes_sent"] = (int64_t)s.outbound.retransmitted_bytes_sent;
    return dict;
}

Dictionary remote_inbound_rtp_to_dict(const livekit::RtcRemoteInboundRtpStats &s) {
    Dictionary dict;
    dict["type"] = "remote_inbound_rtp";
    dict["id"] = String(s.rtc.id.c_str());
    dict["timestamp_ms"] = s.rtc.timestamp_ms;
    dict["ssrc"] = (int)s.stream.ssrc;
    dict["kind"] = String(s.stream.kind.c_str());
    dict["packets_received"] = (int64_t)s.received.packets_received;
    dict["packets_lost"] = (int64_t)s.received.packets_lost;
    dict["jitter"] = s.received.jitter;
    dict["round_trip_time"] = s.remote_inbound.round_trip_time;
    dict["total_round_trip_time"] = s.remote_inbound.total_round_trip_time;
    dict["fraction_lost"] = s.remote_inbound.fraction_lost;
    dict["round_trip_time_measurements"] = (int64_t)s.remote_inbound.round_trip_time_measurements;
    return dict;
}

Dictionary remote_outbound_rtp_to_dict(const livekit::RtcRemoteOutboundRtpStats &s) {
    Dictionary dict;
    dict["type"] = "remote_outbound_rtp";
    dict["id"] = String(s.rtc.id.c_str());
    dict["timestamp_ms"] = s.rtc.timestamp_ms;
    dict["ssrc"] = (int)s.stream.ssrc;
    dict["kind"] = String(s.stream.kind.c_str());
    dict["packets_sent"] = (int64_t)s.sent.packets_sent;
    dict["bytes_sent"] = (int64_t)s.sent.bytes_sent;
    dict["remote_timestamp"] = s.remote_outbound.remote_timestamp;
    dict["round_trip_time"] = s.remote_outbound.round_trip_time;
    dict["total_round_trip_time"] = s.remote_outbound.total_round_trip_time;
    return dict;
}

Dictionary codec_to_dict(const livekit::RtcCodecStats &s) {
    Dictionary dict;
    dict["type"] = "codec";
    dict["id"] = String(s.rtc.id.c_str());
    dict["timestamp_ms"] = s.rtc.timestamp_ms;
    dict["payload_type"] = (int)s.codec.payload_type;
    dict["mime_type"] = String(s.codec.mime_type.c_str());
    dict["clock_rate"] = (int)s.codec.clock_rate;
    dict["channels"] = (int)s.codec.channels;
    return dict;
}

Dictionary transport_to_dict(const livekit::RtcTransportStats &s) {
    Dictionary dict;
    dict["type"] = "transport";
    dict["id"] = String(s.rtc.id.c_str());
    dict["timestamp_ms"] = s.rtc.timestamp_ms;
    dict["packets_sent"] = (int64_t)s.transport.packets_sent;
    dict["packets_received"] = (int64_t)s.transport.packets_received;
    dict["bytes_sent"] = (int64_t)s.transport.bytes_sent;
    dict["bytes_received"] = (int64_t)s.transport.bytes_received;
    dict["dtls_cipher"] = String(s.transport.dtls_cipher.c_str());
    dict["srtp_cipher"] = String(s.transport.srtp_cipher.c_str());
    return dict;
}

Dictionary candidate_pair_to_dict(const livekit::RtcCandidatePairStats &s) {
    Dictionary dict;
    dict["type"] = "candidate_pair";
    dict["id"] = String(s.rtc.id.c_str());
    dict["timestamp_ms"] = s.rtc.timestamp_ms;
    dict["bytes_sent"] = (int64_t)s.candidate_pair.bytes_sent;
    dict["bytes_received"] = (int64_t)s.candidate_pair.bytes_received;
    dict["total_round_trip_time"] = s.candidate_pair.total_round_trip_time;
    dict["current_round_trip_time"] = s.candidate_pair.current_round_trip_time;
    dict["available_outgoing_bitrate"] = s.candidate_pair.available_outgoing_bitrate;
    dict["available_incoming_bitrate"] = s.candidate_pair.available_incoming_bitrate;
    dict["nominated"] = s.candidate_pair.nominated;
    return dict;
}

Dictionary stats_variant_to_dict(const livekit::RtcStats &stats) {
    return std::visit([](auto &&s) -> Dictionary {
        using T = std::decay_t<decltype(s)>;
        if constexpr (std::is_same_v<T, livekit::RtcInboundRtpStats>) {
            return inbound_rtp_to_dict(s);
        } else if constexpr (std::is_same_v<T, livekit::RtcOutboundRtpStats>) {
            return outbound_rtp_to_dict(s);
        } else if constexpr (std::is_same_v<T, livekit::RtcRemoteInboundRtpStats>) {
            return remote_inbound_rtp_to_dict(s);
        } else if constexpr (std::is_same_v<T, livekit::RtcRemoteOutboundRtpStats>) {
            return remote_outbound_rtp_to_dict(s);
        } else if constexpr (std::is_same_v<T, livekit::RtcCodecStats>) {
            return codec_to_dict(s);
        } else if constexpr (std::is_same_v<T, livekit::RtcTransportStats>) {
            return transport_to_dict(s);
        } else if constexpr (std::is_same_v<T, livekit::RtcCandidatePairStats>) {
            return candidate_pair_to_dict(s);
        } else if constexpr (std::is_same_v<T, livekit::RtcMediaSourceStats>) {
            Dictionary dict;
            dict["type"] = "media_source";
            dict["id"] = String(s.rtc.id.c_str());
            dict["timestamp_ms"] = s.rtc.timestamp_ms;
            dict["track_identifier"] = String(s.source.track_identifier.c_str());
            dict["kind"] = String(s.source.kind.c_str());
            return dict;
        } else if constexpr (std::is_same_v<T, livekit::RtcMediaPlayoutStats>) {
            Dictionary dict;
            dict["type"] = "media_playout";
            dict["id"] = String(s.rtc.id.c_str());
            dict["timestamp_ms"] = s.rtc.timestamp_ms;
            dict["total_samples_duration"] = s.audio_playout.total_samples_duration;
            dict["total_playout_delay"] = s.audio_playout.total_playout_delay;
            dict["total_samples_count"] = (int64_t)s.audio_playout.total_samples_count;
            return dict;
        } else if constexpr (std::is_same_v<T, livekit::RtcPeerConnectionStats>) {
            Dictionary dict;
            dict["type"] = "peer_connection";
            dict["id"] = String(s.rtc.id.c_str());
            dict["timestamp_ms"] = s.rtc.timestamp_ms;
            dict["data_channels_opened"] = (int)s.pc.data_channels_opened;
            dict["data_channels_closed"] = (int)s.pc.data_channels_closed;
            return dict;
        } else if constexpr (std::is_same_v<T, livekit::RtcDataChannelStats>) {
            Dictionary dict;
            dict["type"] = "data_channel";
            dict["id"] = String(s.rtc.id.c_str());
            dict["timestamp_ms"] = s.rtc.timestamp_ms;
            dict["label"] = String(s.dc.label.c_str());
            dict["messages_sent"] = (int)s.dc.messages_sent;
            dict["bytes_sent"] = (int64_t)s.dc.bytes_sent;
            dict["messages_received"] = (int)s.dc.messages_received;
            dict["bytes_received"] = (int64_t)s.dc.bytes_received;
            return dict;
        } else if constexpr (std::is_same_v<T, livekit::RtcLocalCandidateStats> || std::is_same_v<T, livekit::RtcRemoteCandidateStats>) {
            Dictionary dict;
            if constexpr (std::is_same_v<T, livekit::RtcLocalCandidateStats>) {
                dict["type"] = "local_candidate";
            } else {
                dict["type"] = "remote_candidate";
            }
            dict["id"] = String(s.rtc.id.c_str());
            dict["timestamp_ms"] = s.rtc.timestamp_ms;
            dict["address"] = String(s.candidate.address.c_str());
            dict["port"] = s.candidate.port;
            dict["protocol"] = String(s.candidate.protocol.c_str());
            return dict;
        } else if constexpr (std::is_same_v<T, livekit::RtcCertificateStats>) {
            Dictionary dict;
            dict["type"] = "certificate";
            dict["id"] = String(s.rtc.id.c_str());
            dict["timestamp_ms"] = s.rtc.timestamp_ms;
            dict["fingerprint"] = String(s.certificate.fingerprint.c_str());
            dict["fingerprint_algorithm"] = String(s.certificate.fingerprint_algorithm.c_str());
            return dict;
        } else if constexpr (std::is_same_v<T, livekit::RtcStreamStats>) {
            Dictionary dict;
            dict["type"] = "stream";
            dict["id"] = String(s.rtc.id.c_str());
            dict["timestamp_ms"] = s.rtc.timestamp_ms;
            dict["stream_identifier"] = String(s.stream.stream_identifier.c_str());
            return dict;
        } else {
            Dictionary dict;
            dict["type"] = "unknown";
            return dict;
        }
    }, stats.stats);
}

} // anonymous namespace

// LiveKitTrack

void LiveKitTrack::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_sid"), &LiveKitTrack::get_sid);
    ClassDB::bind_method(D_METHOD("get_name"), &LiveKitTrack::get_name);
    ClassDB::bind_method(D_METHOD("get_kind"), &LiveKitTrack::get_kind);
    ClassDB::bind_method(D_METHOD("get_source"), &LiveKitTrack::get_source);
    ClassDB::bind_method(D_METHOD("get_muted"), &LiveKitTrack::get_muted);
    ClassDB::bind_method(D_METHOD("get_stream_state"), &LiveKitTrack::get_stream_state);
    ClassDB::bind_method(D_METHOD("request_stats"), &LiveKitTrack::request_stats);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "sid"), "", "get_sid");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "kind"), "", "get_kind");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "source"), "", "get_source");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "muted"), "", "get_muted");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stream_state"), "", "get_stream_state");

    ADD_SIGNAL(MethodInfo("stats_received",
            PropertyInfo(Variant::ARRAY, "stats")));

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

void LiveKitTrack::request_stats() {
    if (!track_) {
        return;
    }

    // Capture shared_ptr so the native track stays alive for the duration
    std::shared_ptr<livekit::Track> track_copy = track_;

    std::thread([this, track_copy]() {
        try {
            auto future = track_copy->getStats();
            auto status = future.wait_for(std::chrono::seconds(5));
            if (status == std::future_status::ready) {
                auto stats_vec = future.get();
                Array result;
                for (const auto &stats : stats_vec) {
                    result.push_back(stats_variant_to_dict(stats));
                }
                call_deferred("emit_signal", "stats_received", result);
            } else {
                UtilityFunctions::printerr("LiveKitTrack::request_stats: timed out waiting for stats");
            }
        } catch (const std::exception &e) {
            UtilityFunctions::printerr("LiveKitTrack::request_stats: error: ", String(e.what()));
        }
    }).detach();
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
