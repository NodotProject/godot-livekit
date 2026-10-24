#ifndef GODOT_LIVEKIT_AUDIO_STREAM_H
#define GODOT_LIVEKIT_AUDIO_STREAM_H

#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>

#include "livekit_participant.h"
#include "livekit_track.h"

#include <vector>

namespace godot {

// Web implementation of LiveKitAudioStream: an AudioWorklet copies a remote track's audio (at the
// browser's AudioContext rate) into a buffer that poll() drains into Godot.
class LiveKitAudioStream : public RefCounted {
    GDCLASS(LiveKitAudioStream, RefCounted)

private:
    int js_id_ = 0;
    std::vector<float> buffer_;

protected:
    static void _bind_methods();

public:
    ~LiveKitAudioStream();

    static Ref<LiveKitAudioStream> from_track(const Ref<LiveKitTrack> &track);
    static Ref<LiveKitAudioStream> from_participant(const Ref<LiveKitRemoteParticipant> &participant, int source);

    int get_sample_rate() const;
    int get_num_channels() const;
    int poll(const Ref<AudioStreamGeneratorPlayback> &playback);
    void close();
};

}

#endif // GODOT_LIVEKIT_AUDIO_STREAM_H
