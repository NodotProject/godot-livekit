#ifndef GODOT_LIVEKIT_WEB_VIDEO_H
#define GODOT_LIVEKIT_WEB_VIDEO_H

#include <cstdint>

// Video paths that bypass web_call()'s JSON: pixels are copied directly between the wasm heap
// and bridge.js.

namespace godot {

void web_video_source_capture(int p_source_id, const uint8_t *p_rgba, int p_width, int p_height, int p_rotation);

// Captures the stream's newest frame if it hasn't been read yet, returning whether there was one
// and setting its size. web_video_stream_read() then copies its RGBA pixels into r_rgba.
bool web_video_stream_prepare(int p_stream_id, int *r_width, int *r_height);
void web_video_stream_read(int p_stream_id, uint8_t *r_rgba);

}

#endif // GODOT_LIVEKIT_WEB_VIDEO_H
