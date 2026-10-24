#include "web_audio.h"

#include <emscripten.h>

EM_JS(void, godot_livekit_js_source_capture, (int p_source_id, const float *p_samples, int p_count, int p_sample_rate, int p_channels), {
    GodotLiveKit.sourceCapture(p_source_id, HEAPF32.slice(p_samples >> 2, (p_samples >> 2) + p_count), p_sample_rate, p_channels);
});

EM_JS(int, godot_livekit_js_stream_read, (int p_stream_id, float *r_frames, int p_max_frames), {
    return GodotLiveKit.streamRead(p_stream_id, HEAPF32, r_frames >> 2, p_max_frames);
});

void godot::web_source_capture(int p_source_id, const float *p_samples, int p_count, int p_sample_rate, int p_channels) {
    godot_livekit_js_source_capture(p_source_id, p_samples, p_count, p_sample_rate, p_channels);
}

int godot::web_stream_read(int p_stream_id, float *r_frames, int p_max_frames) {
    return godot_livekit_js_stream_read(p_stream_id, r_frames, p_max_frames);
}
