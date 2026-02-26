#include "livekit_video_stream.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void LiveKitVideoStream::_bind_methods() {
    ClassDB::bind_static_method("LiveKitVideoStream", D_METHOD("from_track", "track"), &LiveKitVideoStream::from_track);
    ClassDB::bind_static_method("LiveKitVideoStream", D_METHOD("from_participant", "participant", "source"), &LiveKitVideoStream::from_participant);

    ClassDB::bind_method(D_METHOD("get_texture"), &LiveKitVideoStream::get_texture);
    ClassDB::bind_method(D_METHOD("poll"), &LiveKitVideoStream::poll);
    ClassDB::bind_method(D_METHOD("close"), &LiveKitVideoStream::close);

    ADD_SIGNAL(MethodInfo("frame_received"));
}

LiveKitVideoStream::LiveKitVideoStream() {
}

LiveKitVideoStream::~LiveKitVideoStream() {
    close();
}

void LiveKitVideoStream::_reader_loop() {
    // Capture a local copy so the native stream stays alive even if
    // close() calls stream_.reset() on another thread.
    auto stream = stream_;
    while (running_.load()) {
        livekit::VideoFrameEvent event;
        bool ok = stream->read(event);
        if (!ok) {
            break;
        }

        auto frame = std::make_unique<livekit::VideoFrame>(std::move(event.frame));

        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            pending_frame_ = std::move(frame);
        }
    }
    thread_exited_.store(true);
}

Ref<LiveKitVideoStream> LiveKitVideoStream::from_track(const Ref<LiveKitTrack> &track) {
    if (track.is_null() || !track->get_native_track()) {
        UtilityFunctions::push_error("LiveKitVideoStream::from_track: invalid track");
        return Ref<LiveKitVideoStream>();
    }

    livekit::VideoStream::Options opts;
    opts.format = livekit::VideoBufferType::RGBA;

    auto native_stream = livekit::VideoStream::fromTrack(track->get_native_track(), opts);
    if (!native_stream) {
        UtilityFunctions::push_error("LiveKitVideoStream::from_track: failed to create stream");
        return Ref<LiveKitVideoStream>();
    }

    Ref<LiveKitVideoStream> stream;
    stream.instantiate();
    stream->stream_ = native_stream;
    stream->texture_.instantiate();
    stream->running_.store(true);
    stream->reader_thread_ = std::thread(&LiveKitVideoStream::_reader_loop, stream.ptr());

    return stream;
}

Ref<LiveKitVideoStream> LiveKitVideoStream::from_participant(const Ref<LiveKitRemoteParticipant> &participant, int source) {
    if (participant.is_null() || !participant->get_native_remote_participant()) {
        UtilityFunctions::push_error("LiveKitVideoStream::from_participant: invalid participant");
        return Ref<LiveKitVideoStream>();
    }

    livekit::VideoStream::Options opts;
    opts.format = livekit::VideoBufferType::RGBA;

    auto native_stream = livekit::VideoStream::fromParticipant(
            *participant->get_native_remote_participant(),
            (livekit::TrackSource)source,
            opts);
    if (!native_stream) {
        UtilityFunctions::push_error("LiveKitVideoStream::from_participant: failed to create stream");
        return Ref<LiveKitVideoStream>();
    }

    Ref<LiveKitVideoStream> stream;
    stream.instantiate();
    stream->stream_ = native_stream;
    stream->texture_.instantiate();
    stream->running_.store(true);
    stream->reader_thread_ = std::thread(&LiveKitVideoStream::_reader_loop, stream.ptr());

    return stream;
}

Ref<ImageTexture> LiveKitVideoStream::get_texture() const {
    return texture_;
}

bool LiveKitVideoStream::poll() {
    std::unique_ptr<livekit::VideoFrame> frame;

    {
        std::unique_lock<std::mutex> lock(frame_mutex_, std::try_to_lock);
        if (!lock.owns_lock() || !pending_frame_) {
            return false; // Reader thread holds the lock or no frame; skip
        }
        frame = std::move(pending_frame_);
    }

    int width = frame->width();
    int height = frame->height();
    size_t data_size = frame->dataSize();

    PackedByteArray pba;
    pba.resize(data_size);
    memcpy(pba.ptrw(), frame->data(), data_size);

    Ref<Image> image = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, pba);
    if (image.is_null()) {
        return false;
    }

    if (texture_.is_valid()) {
        texture_->set_image(image);
    }

    emit_signal("frame_received");
    return true;
}

void LiveKitVideoStream::close() {
    running_.store(false);
    if (stream_) {
        stream_->close();
    }
    if (reader_thread_.joinable()) {
        // stream_->close() should make read() return false promptly.
        // Wait up to 2 seconds as a safety net; detach if the thread doesn't exit.
        for (int i = 0; i < 2000 && !thread_exited_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if (thread_exited_.load()) {
            reader_thread_.join();
        } else {
            reader_thread_.detach();
        }
    }
    stream_.reset();
}
