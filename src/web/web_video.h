#ifndef GODOT_LIVEKIT_WEB_VIDEO_H
#define GODOT_LIVEKIT_WEB_VIDEO_H

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>

#include <cstdint>

// Video paths that bypass web_call()'s JSON: pixels are copied directly between the wasm heap
// and bridge.js.

namespace godot {

void web_video_source_capture(int p_source_id, const uint8_t *p_rgba, int p_width, int p_height, int p_rotation);

// Captures the stream's newest frame if it hasn't been read yet, returning whether there was one
// and setting its size. web_video_stream_read() then copies its RGBA pixels into r_rgba.
bool web_video_stream_prepare(int p_stream_id, int *r_width, int *r_height);
void web_video_stream_read(int p_stream_id, uint8_t *r_rgba);

// Reads the stream's newest frame into r_image and r_texture, as prepare and read above do,
// returning whether there was one. r_pixels and the image are reused while the size is unchanged.
bool web_video_stream_poll(int p_stream_id, Ref<Image> &r_image, const Ref<ImageTexture> &r_texture, PackedByteArray &r_pixels);

}

#endif // GODOT_LIVEKIT_WEB_VIDEO_H
