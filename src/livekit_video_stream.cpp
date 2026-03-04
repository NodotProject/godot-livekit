#include "livekit_video_stream.h"
#include "livekit_poller.h"

#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

void LiveKitVideoStream::_bind_methods() {
    ClassDB::bind_static_method("LiveKitVideoStream", D_METHOD("from_track", "track"), &LiveKitVideoStream::from_track);
    ClassDB::bind_static_method("LiveKitVideoStream", D_METHOD("from_participant", "participant", "source"), &LiveKitVideoStream::from_participant);

    ClassDB::bind_method(D_METHOD("get_texture"), &LiveKitVideoStream::get_texture);
    ClassDB::bind_method(D_METHOD("poll"), &LiveKitVideoStream::poll);
    ClassDB::bind_method(D_METHOD("close"), &LiveKitVideoStream::close);

    ClassDB::bind_method(D_METHOD("set_auto_poll", "enabled"), &LiveKitVideoStream::set_auto_poll);
    ClassDB::bind_method(D_METHOD("get_auto_poll"), &LiveKitVideoStream::get_auto_poll);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_poll"), "set_auto_poll", "get_auto_poll");

    ADD_SIGNAL(MethodInfo("frame_received"));
}

LiveKitVideoStream::LiveKitVideoStream() {
}

LiveKitVideoStream::~LiveKitVideoStream() {
    LiveKitPoller::instance().unregister_video_stream(this);
    alive_->store(false);
    close();
}

void LiveKitVideoStream::_reader_loop() {
    // Capture a local copy so the native stream stays alive even if
    // close() calls stream_.reset() on another thread.
    auto stream = stream_;
    if (!stream) return;
    auto alive = alive_;
    while (running_.load() && alive->load()) {
        livekit::VideoFrameEvent event;
        bool ok = stream->read(event);
        if (!ok || !alive->load()) {
            break;
        }

        auto frame = std::make_unique<livekit::VideoFrame>(std::move(event.frame));

        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            pending_frame_ = std::move(frame);
        }
    }
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

    LiveKitPoller::instance().register_video_stream(stream.ptr(), stream->alive_);

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

    LiveKitPoller::instance().register_video_stream(stream.ptr(), stream->alive_);

    return stream;
}

Ref<ImageTexture> LiveKitVideoStream::get_texture() const {
    return texture_;
}

void LiveKitVideoStream::_ensure_reader_started() {
    if (thread_started_.exchange(true)) {
        return; // Already started
    }
    running_.store(true);
    auto alive = alive_;
    reader_thread_.start([this, alive]() {
        if (alive->load()) { _reader_loop(); }
    });
}

bool LiveKitVideoStream::poll() {
    if (!stream_) {
        return false;
    }
    _ensure_reader_started();

    std::unique_ptr<livekit::VideoFrame> frame;

    {
        std::unique_lock<std::mutex> lock(frame_mutex_, std::try_to_lock);
        if (!lock.owns_lock()) {
            uint32_t count = lock_contention_count_.fetch_add(1) + 1;
            if (count == 1 || (count % 100) == 0) {
                UtilityFunctions::push_warning("LiveKitVideoStream::poll: lock contention (", count, " skipped frames)");
            }
            return false;
        }
        if (!pending_frame_) {
            return false;
        }
        frame = std::move(pending_frame_);
    }

    int width = frame->width();
    int height = frame->height();
    size_t data_size = frame->dataSize();

    if (width == last_width_ && height == last_height_ && cached_image_.is_valid()) {
        // Fast path: reuse existing buffers — no allocation.
        memcpy(cached_pba_.ptrw(), frame->data(), data_size);
        cached_image_->set_data(width, height, false, Image::FORMAT_RGBA8, cached_pba_);
        if (texture_.is_valid()) {
            texture_->update(cached_image_);
        }
    } else {
        // Slow path: resolution changed or first frame — allocate.
        cached_pba_.resize(data_size);
        memcpy(cached_pba_.ptrw(), frame->data(), data_size);

        cached_image_ = Image::create_from_data(width, height, false, Image::FORMAT_RGBA8, cached_pba_);
        if (cached_image_.is_null()) {
            return false;
        }

        if (texture_.is_valid()) {
            texture_->set_image(cached_image_);
        }

        last_width_ = width;
        last_height_ = height;
    }

    emit_signal("frame_received");
    return true;
}

void LiveKitVideoStream::close() {
    running_.store(false);
    if (stream_) {
        stream_->close();
    }
    reader_thread_.join_or_detach(2000);
    stream_.reset();
    thread_started_.store(false);
}

void LiveKitVideoStream::set_auto_poll(bool enabled) {
    if (auto_poll_ == enabled) {
        return;
    }
    auto_poll_ = enabled;
    if (enabled) {
        LiveKitPoller::instance().register_video_stream(this, alive_);
    } else {
        LiveKitPoller::instance().unregister_video_stream(this);
    }
}

bool LiveKitVideoStream::get_auto_poll() const {
    return auto_poll_;
}
