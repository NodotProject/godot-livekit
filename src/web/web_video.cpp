#include "web_video.h"

#include <emscripten.h>

EM_JS(void, godot_livekit_js_video_source_capture, (int p_source_id, const uint8_t *p_rgba, int p_width, int p_height, int p_rotation), {
    GodotLiveKit.videoSourceCapture(p_source_id, HEAPU8.subarray(p_rgba, p_rgba + p_width * p_height * 4), p_width, p_height, p_rotation);
});

EM_JS(int, godot_livekit_js_video_stream_prepare, (int p_stream_id, int *r_width, int *r_height), {
    const size = GodotLiveKit.videoStreamPrepare(p_stream_id);
    if (!size) {
        return 0;
    }
    HEAP32[r_width >> 2] = size[0];
    HEAP32[r_height >> 2] = size[1];
    return 1;
});

EM_JS(void, godot_livekit_js_video_stream_read, (int p_stream_id, uint8_t *r_rgba), {
    GodotLiveKit.videoStreamRead(p_stream_id, HEAPU8, r_rgba);
});

void godot::web_video_source_capture(int p_source_id, const uint8_t *p_rgba, int p_width, int p_height, int p_rotation) {
    godot_livekit_js_video_source_capture(p_source_id, p_rgba, p_width, p_height, p_rotation);
}

bool godot::web_video_stream_prepare(int p_stream_id, int *r_width, int *r_height) {
    return godot_livekit_js_video_stream_prepare(p_stream_id, r_width, r_height);
}

void godot::web_video_stream_read(int p_stream_id, uint8_t *r_rgba) {
    godot_livekit_js_video_stream_read(p_stream_id, r_rgba);
}
