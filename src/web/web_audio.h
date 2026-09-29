#ifndef GODOT_LIVEKIT_WEB_AUDIO_H
#define GODOT_LIVEKIT_WEB_AUDIO_H

// Audio paths that bypass web_call()'s JSON: samples are copied directly between the wasm heap
// and bridge.js.

namespace godot {

void web_source_capture(int p_source_id, const float *p_samples, int p_count, int p_sample_rate, int p_channels);

// Copies up to p_max_frames of interleaved stereo audio into r_frames, returning the frame count.
int web_stream_read(int p_stream_id, float *r_frames, int p_max_frames);

}

#endif // GODOT_LIVEKIT_WEB_AUDIO_H
