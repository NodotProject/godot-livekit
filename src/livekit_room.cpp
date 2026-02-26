#include "livekit_room.h"
#include "livekit_track.h"
#include "livekit_track_publication.h"
#ifdef LIVEKIT_E2EE_SUPPORTED
#include "livekit_e2ee.h"
#endif

#include <godot_cpp/variant/utility_functions.hpp>

#include <exception>
#include <livekit/participant.h>
#include <livekit/local_participant.h>
#include <livekit/remote_participant.h>
#include <livekit/track.h>
#include <livekit/track_publication.h>
#include <livekit/local_track_publication.h>
#include <livekit/remote_track_publication.h>

using namespace godot;

void LiveKitRoom::_bind_methods() {
    ClassDB::bind_method(D_METHOD("connect_to_room", "url", "token", "options"), &LiveKitRoom::connect_to_room);
    ClassDB::bind_method(D_METHOD("disconnect_from_room"), &LiveKitRoom::disconnect_from_room);
    ClassDB::bind_method(D_METHOD("_finalize_connection", "success"), &LiveKitRoom::_finalize_connection);
    ClassDB::bind_method(D_METHOD("poll_events"), &LiveKitRoom::poll_events);
    ClassDB::bind_method(D_METHOD("get_local_participant"), &LiveKitRoom::get_local_participant);
    ClassDB::bind_method(D_METHOD("get_remote_participants"), &LiveKitRoom::get_remote_participants);
    ClassDB::bind_method(D_METHOD("get_sid"), &LiveKitRoom::get_sid);
    ClassDB::bind_method(D_METHOD("get_name"), &LiveKitRoom::get_name);
    ClassDB::bind_method(D_METHOD("get_metadata"), &LiveKitRoom::get_metadata);
    ClassDB::bind_method(D_METHOD("get_connection_state"), &LiveKitRoom::get_connection_state);
#ifdef LIVEKIT_E2EE_SUPPORTED
    ClassDB::bind_method(D_METHOD("get_e2ee_manager"), &LiveKitRoom::get_e2ee_manager);
#endif

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "local_participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitLocalParticipant"), "", "get_local_participant");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "remote_participants"), "", "get_remote_participants");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "sid"), "", "get_sid");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "metadata"), "", "get_metadata");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_state"), "", "get_connection_state");

    // Connection signals
    ADD_SIGNAL(MethodInfo("connected"));
    ADD_SIGNAL(MethodInfo("disconnected"));
    ADD_SIGNAL(MethodInfo("reconnecting"));
    ADD_SIGNAL(MethodInfo("reconnected"));

    // Participant signals
    ADD_SIGNAL(MethodInfo("participant_connected",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("participant_disconnected",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("participant_metadata_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::STRING, "old_metadata"),
            PropertyInfo(Variant::STRING, "new_metadata")));
    ADD_SIGNAL(MethodInfo("participant_name_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::STRING, "old_name"),
            PropertyInfo(Variant::STRING, "new_name")));
    ADD_SIGNAL(MethodInfo("participant_attributes_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::DICTIONARY, "changed_attributes")));

    // Room metadata
    ADD_SIGNAL(MethodInfo("room_metadata_changed",
            PropertyInfo(Variant::STRING, "old_metadata"),
            PropertyInfo(Variant::STRING, "new_metadata")));

    // Connection quality
    ADD_SIGNAL(MethodInfo("connection_quality_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::INT, "quality")));

    // Track signals
    ADD_SIGNAL(MethodInfo("track_published",
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteTrackPublication"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("track_unpublished",
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteTrackPublication"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("track_subscribed",
            PropertyInfo(Variant::OBJECT, "track", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrack"),
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteTrackPublication"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("track_unsubscribed",
            PropertyInfo(Variant::OBJECT, "track", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrack"),
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteTrackPublication"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant")));
    ADD_SIGNAL(MethodInfo("track_muted",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrackPublication")));
    ADD_SIGNAL(MethodInfo("track_unmuted",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrackPublication")));
    ADD_SIGNAL(MethodInfo("local_track_published",
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitLocalTrackPublication"),
            PropertyInfo(Variant::OBJECT, "track", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitTrack")));
    ADD_SIGNAL(MethodInfo("local_track_unpublished",
            PropertyInfo(Variant::OBJECT, "publication", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitLocalTrackPublication")));

    // Data
    ADD_SIGNAL(MethodInfo("data_received",
            PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data"),
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitRemoteParticipant"),
            PropertyInfo(Variant::INT, "kind"),
            PropertyInfo(Variant::STRING, "topic")));

#ifdef LIVEKIT_E2EE_SUPPORTED
    // E2EE signals
    ADD_SIGNAL(MethodInfo("e2ee_state_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::INT, "state")));
    ADD_SIGNAL(MethodInfo("participant_encryption_status_changed",
            PropertyInfo(Variant::OBJECT, "participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitParticipant"),
            PropertyInfo(Variant::BOOL, "is_encrypted")));
#endif

    // Connection state enum
    BIND_ENUM_CONSTANT(STATE_DISCONNECTED);
    BIND_ENUM_CONSTANT(STATE_CONNECTED);
    BIND_ENUM_CONSTANT(STATE_RECONNECTING);
}

LiveKitRoom::LiveKitRoom() {
    room = new livekit::Room();
    delegate = new GodotRoomDelegate(this);
    room->setDelegate(delegate);
}

LiveKitRoom::~LiveKitRoom() {
    // The disconnect thread joins the connect thread, which dereferences
    // `this` (event_mutex_, pending_events_).  We must join both before
    // destroying the object.  Join disconnect first (it transitively waits
    // on connect), then connect (in case disconnect was never started).
    if (disconnect_thread_.joinable()) {
        disconnect_thread_.join();
    }
    if (connect_thread_.joinable()) {
        connect_thread_.join();
    }
    if (room) {
        room->setDelegate(nullptr);
        delete room;
        room = nullptr;
    }
    if (delegate) {
        delete delegate;
        delegate = nullptr;
    }
}

bool LiveKitRoom::connect_to_room(const String &url, const String &token, const Dictionary &options) {
    auto_reconnect_ = options.get("auto_reconnect", true);

    livekit::RoomOptions room_options;
    room_options.auto_subscribe = options.get("auto_subscribe", true);
    room_options.dynacast = options.get("dynacast", false);

#ifdef LIVEKIT_E2EE_SUPPORTED
    // Parse E2EE options
    if (options.has("e2ee")) {
        Variant e2ee_var = options["e2ee"];
        Ref<LiveKitE2eeOptions> e2ee_opts = e2ee_var;
        if (e2ee_opts.is_valid()) {
            room_options.encryption = e2ee_opts->to_native();
        }
    }
#endif

    // Join any previous connect thread before starting a new one.
    if (connect_thread_.joinable()) {
        connect_thread_.join();
    }
    // Previous disconnect thread never dereferences `this`; safe to detach.
    if (disconnect_thread_.joinable()) {
        disconnect_thread_.detach();
    }

    // Suppress the delegate's "connected" signal during async connect;
    // _finalize_connection will emit it after participant init.
    connecting_async_ = true;

    std::string url_str(url.utf8().get_data());
    std::string token_str(token.utf8().get_data());

    // Run the blocking Connect() on a background thread so the main
    // thread (and Godot's rendering/input loop) stays responsive.
    connect_thread_ = std::thread([this, url_str, token_str, room_options]() {
        bool success = false;
        try {
            success = room->Connect(url_str.c_str(), token_str.c_str(), room_options);
        } catch (const std::exception &e) {
            UtilityFunctions::push_error("LiveKitRoom::connect_to_room: connection failed: ", String(e.what()));
        } catch (...) {
            UtilityFunctions::push_error("LiveKitRoom::connect_to_room: connection failed with unknown error");
        }
        std::lock_guard<std::mutex> lock(event_mutex_);
        pending_events_.push_back([this, success]() {
            _finalize_connection(success);
        });
    });

    return true; // "connection started" — result arrives via signals
}

void LiveKitRoom::_finalize_connection(bool success) {
    connecting_async_ = false;

    if (connect_thread_.joinable()) {
        connect_thread_.join();
    }

    if (success) {
        connection_state = STATE_CONNECTED;

        // Initialize local participant
        livekit::LocalParticipant *lp = room->localParticipant();
        if (lp) {
            local_participant.instantiate();
            local_participant->bind_participant(lp);
            local_participant->bind_local_participant(lp);
        }

        // Initialize remote participants
        auto remotes = room->remoteParticipants();
        for (auto const &r : remotes) {
            Ref<LiveKitRemoteParticipant> p;
            p.instantiate();
            p->bind_participant(r.get());
            p->bind_remote_participant(static_cast<livekit::RemoteParticipant *>(r.get()));
            std::lock_guard<std::mutex> plock(participants_mutex_);
            remote_participants[p->get_identity()] = p;
        }

#ifdef LIVEKIT_E2EE_SUPPORTED
        // Initialize E2EE manager if available
        livekit::E2EEManager *mgr = room->e2eeManager();
        if (mgr) {
            e2ee_manager_.instantiate();
            e2ee_manager_->bind_manager(mgr);
        }
#endif

        emit_signal("connected");
    } else {
        connection_state = STATE_DISCONNECTED;
        emit_signal("disconnected");
    }
}

void LiveKitRoom::poll_events() {
    std::vector<std::function<void()>> events;
    {
        std::lock_guard<std::mutex> lock(event_mutex_);
        events.swap(pending_events_);
    }
    for (auto &fn : events) {
        fn();
    }
}

void LiveKitRoom::disconnect_from_room() {
    connecting_async_ = false;

    // Discard pending events from the old connection.
    {
        std::lock_guard<std::mutex> lock(event_mutex_);
        pending_events_.clear();
    }

    // Previous disconnect thread only touches its captured old_room /
    // old_delegate — never `this`.  Safe to detach if still running.
    if (disconnect_thread_.joinable()) {
        disconnect_thread_.detach();
    }

    local_participant.unref();
    {
        std::lock_guard<std::mutex> plock(participants_mutex_);
        remote_participants.clear();
    }
#ifdef LIVEKIT_E2EE_SUPPORTED
    e2ee_manager_.unref();
#endif
    connection_state.store(STATE_DISCONNECTED);

    // Detach delegate before destroying the room so the Room destructor
    // does not fire callbacks into a freed delegate (use-after-free).
    livekit::Room *old_room = room;
    GodotRoomDelegate *old_delegate = delegate;
    if (old_room) {
        old_room->setDelegate(nullptr);
    }

    // Recreate room and delegate immediately so the object is ready for reuse.
    room = new livekit::Room();
    delegate = new GodotRoomDelegate(this);
    room->setDelegate(delegate);

    // Move the connect thread into the lambda so *it* waits for Connect()
    // to finish before deleting the room — without blocking the main thread.
    std::thread ct = std::move(connect_thread_);

    // SDK has no explicit disconnect; destroying the Room disconnects.
    // The Room destructor tears down networking, which can block for
    // several seconds.  Run it on a background thread so the main thread
    // (and Godot's rendering/input loop) stays responsive.
    disconnect_thread_ = std::thread([old_room, old_delegate, ct = std::move(ct)]() mutable {
        // The connect thread dereferences old_room; wait for it to finish
        // before destroying the room.
        if (ct.joinable()) {
            ct.join();
        }
        delete old_room;
        delete old_delegate;
    });
}

Ref<LiveKitLocalParticipant> LiveKitRoom::get_local_participant() const {
    return local_participant;
}

Dictionary LiveKitRoom::get_remote_participants() const {
    std::lock_guard<std::mutex> plock(participants_mutex_);
    return remote_participants.duplicate();
}

String LiveKitRoom::get_sid() const {
    if (room) {
        auto info = room->room_info();
        if (info.sid.has_value()) {
            return String(info.sid.value().c_str());
        }
    }
    return String();
}

String LiveKitRoom::get_name() const {
    if (room) {
        auto info = room->room_info();
        return String(info.name.c_str());
    }
    return String();
}

String LiveKitRoom::get_metadata() const {
    if (room) {
        auto info = room->room_info();
        return String(info.metadata.c_str());
    }
    return String();
}

int LiveKitRoom::get_connection_state() const {
    return connection_state.load();
}

#ifdef LIVEKIT_E2EE_SUPPORTED
Ref<LiveKitE2eeManager> LiveKitRoom::get_e2ee_manager() const {
    return e2ee_manager_;
}
#endif

Ref<LiveKitParticipant> LiveKitRoom::_find_or_create_participant(livekit::Participant *p) {
    if (!p) {
        return Ref<LiveKitParticipant>();
    }

    String identity = String(p->identity().c_str());

    // Check if it's the local participant
    if (local_participant.is_valid() && local_participant->get_identity() == identity) {
        return local_participant;
    }

    // Check remote participants
    std::lock_guard<std::mutex> plock(participants_mutex_);
    if (remote_participants.has(identity)) {
        return remote_participants[identity];
    }

    return Ref<LiveKitParticipant>();
}

Ref<LiveKitParticipant> LiveKitRoom::_find_or_create_participant_by_identity(const std::string &identity) {
    String gd_identity = String(identity.c_str());

    if (local_participant.is_valid() && local_participant->get_identity() == gd_identity) {
        return local_participant;
    }
    std::lock_guard<std::mutex> plock(participants_mutex_);
    if (remote_participants.has(gd_identity)) {
        return remote_participants[gd_identity];
    }
    return Ref<LiveKitParticipant>();
}

// GodotRoomDelegate implementations
//
// All callbacks fire on LiveKit SDK background threads.  We MUST NOT
// call any Godot API (Object::call_deferred, Ref::instantiate, Dictionary
// access, etc.) from these threads — doing so crashes the engine.
//
// Instead, each callback captures lightweight C++ data under the event
// mutex and pushes a lambda.  poll_events() (called from GDScript's
// _process on the main thread) drains the queue and safely creates
// Godot wrappers / emits signals.

void LiveKitRoom::GodotRoomDelegate::onParticipantConnected(livekit::Room &r, const livekit::ParticipantConnectedEvent &e) {
    auto *rp = static_cast<livekit::RemoteParticipant *>(e.participant);
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, rp]() {
        Ref<LiveKitRemoteParticipant> p;
        p.instantiate();
        p->bind_participant(rp);
        p->bind_remote_participant(rp);
        {
            std::lock_guard<std::mutex> plock(room->participants_mutex_);
            room->remote_participants[p->get_identity()] = p;
        }
        room->emit_signal("participant_connected", p);
    });
}

void LiveKitRoom::GodotRoomDelegate::onParticipantDisconnected(livekit::Room &r, const livekit::ParticipantDisconnectedEvent &e) {
    std::string identity(e.participant->identity());
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, identity]() {
        String gd_identity = String(identity.c_str());
        Ref<LiveKitRemoteParticipant> p;
        {
            std::lock_guard<std::mutex> plock(room->participants_mutex_);
            p = room->remote_participants.get(gd_identity, Variant());
            if (p.is_valid()) {
                room->remote_participants.erase(gd_identity);
            }
        }
        if (p.is_valid()) {
            room->emit_signal("participant_disconnected", p);
        }
    });
}

void LiveKitRoom::GodotRoomDelegate::onConnectionStateChanged(livekit::Room &r, const livekit::ConnectionStateChangedEvent &e) {
    auto state = e.state;
    bool connecting = room->connecting_async_.load();
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, state, connecting]() {
        if (state == livekit::ConnectionState::Connected) {
            room->connection_state = STATE_CONNECTED;
            if (!connecting) {
                room->emit_signal("connected");
            }
        } else if (state == livekit::ConnectionState::Disconnected) {
            room->connection_state = STATE_DISCONNECTED;
            if (!connecting) {
                room->emit_signal("disconnected");
            }
        } else if (state == livekit::ConnectionState::Reconnecting) {
            room->connection_state = STATE_RECONNECTING;
        }
    });
}

void LiveKitRoom::GodotRoomDelegate::onDisconnected(livekit::Room &r, const livekit::DisconnectedEvent &e) {
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this]() {
        room->connection_state = STATE_DISCONNECTED;
        room->emit_signal("disconnected");
    });
}

void LiveKitRoom::GodotRoomDelegate::onReconnecting(livekit::Room &r, const livekit::ReconnectingEvent &e) {
    bool auto_reconnect = room->auto_reconnect_;
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, auto_reconnect]() {
        if (!auto_reconnect) {
            room->connection_state = STATE_DISCONNECTED;
            room->emit_signal("disconnected");
            return;
        }
        room->connection_state = STATE_RECONNECTING;
        room->emit_signal("reconnecting");
    });
}

void LiveKitRoom::GodotRoomDelegate::onReconnected(livekit::Room &r, const livekit::ReconnectedEvent &e) {
    bool auto_reconnect = room->auto_reconnect_;
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, auto_reconnect]() {
        if (!auto_reconnect) {
            return;
        }
        room->connection_state = STATE_CONNECTED;
        room->emit_signal("reconnected");
    });
}

void LiveKitRoom::GodotRoomDelegate::onRoomMetadataChanged(livekit::Room &r, const livekit::RoomMetadataChangedEvent &e) {
    std::string old_meta(e.old_metadata);
    std::string new_meta(e.new_metadata);
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, old_meta, new_meta]() {
        room->emit_signal("room_metadata_changed",
                String(old_meta.c_str()), String(new_meta.c_str()));
    });
}

void LiveKitRoom::GodotRoomDelegate::onConnectionQualityChanged(livekit::Room &r, const livekit::ConnectionQualityChangedEvent &e) {
    std::string identity(e.participant ? e.participant->identity() : "");
    int quality = (int)e.quality;
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, identity, quality]() {
        String gd_identity = String(identity.c_str());
        Ref<LiveKitParticipant> p;
        if (room->local_participant.is_valid() &&
                room->local_participant->get_identity() == gd_identity) {
            p = room->local_participant;
        } else if (room->remote_participants.has(gd_identity)) {
            p = room->remote_participants[gd_identity];
        }
        if (p.is_valid()) {
            room->emit_signal("connection_quality_changed", p, quality);
        }
    });
}

void LiveKitRoom::GodotRoomDelegate::onParticipantMetadataChanged(livekit::Room &r, const livekit::ParticipantMetadataChangedEvent &e) {
    std::string identity(e.participant ? e.participant->identity() : "");
    std::string old_meta(e.old_metadata);
    std::string new_meta(e.new_metadata);
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, identity, old_meta, new_meta]() {
        String gd_identity = String(identity.c_str());
        Ref<LiveKitParticipant> p;
        if (room->local_participant.is_valid() &&
                room->local_participant->get_identity() == gd_identity) {
            p = room->local_participant;
        } else if (room->remote_participants.has(gd_identity)) {
            p = room->remote_participants[gd_identity];
        }
        if (p.is_valid()) {
            room->emit_signal("participant_metadata_changed", p,
                    String(old_meta.c_str()), String(new_meta.c_str()));
        }
    });
}

void LiveKitRoom::GodotRoomDelegate::onParticipantNameChanged(livekit::Room &r, const livekit::ParticipantNameChangedEvent &e) {
    std::string identity(e.participant ? e.participant->identity() : "");
    std::string old_name(e.old_name);
    std::string new_name(e.new_name);
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, identity, old_name, new_name]() {
        String gd_identity = String(identity.c_str());
        Ref<LiveKitParticipant> p;
        if (room->local_participant.is_valid() &&
                room->local_participant->get_identity() == gd_identity) {
            p = room->local_participant;
        } else if (room->remote_participants.has(gd_identity)) {
            p = room->remote_participants[gd_identity];
        }
        if (p.is_valid()) {
            room->emit_signal("participant_name_changed", p,
                    String(old_name.c_str()), String(new_name.c_str()));
        }
    });
}

void LiveKitRoom::GodotRoomDelegate::onParticipantAttributesChanged(livekit::Room &r, const livekit::ParticipantAttributesChangedEvent &e) {
    std::string identity(e.participant ? e.participant->identity() : "");
    std::vector<std::pair<std::string, std::string>> attrs;
    for (const auto &attr : e.changed_attributes) {
        attrs.emplace_back(attr.key, attr.value);
    }
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, identity, attrs]() {
        String gd_identity = String(identity.c_str());
        Ref<LiveKitParticipant> p;
        if (room->local_participant.is_valid() &&
                room->local_participant->get_identity() == gd_identity) {
            p = room->local_participant;
        } else if (room->remote_participants.has(gd_identity)) {
            p = room->remote_participants[gd_identity];
        }
        if (p.is_valid()) {
            Dictionary changed;
            for (const auto &a : attrs) {
                changed[String(a.first.c_str())] = String(a.second.c_str());
            }
            room->emit_signal("participant_attributes_changed", p, changed);
        }
    });
}

// Track delegate callbacks

void LiveKitRoom::GodotRoomDelegate::onTrackPublished(livekit::Room &r, const livekit::TrackPublishedEvent &e) {
    auto pub_sp = e.publication;
    std::string identity(e.participant ? e.participant->identity() : "");
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, pub_sp, identity]() {
        Ref<LiveKitRemoteTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pub_sp);
        Ref<LiveKitParticipant> p = room->_find_or_create_participant_by_identity(identity);
        room->emit_signal("track_published", pub, p);
    });
}

void LiveKitRoom::GodotRoomDelegate::onTrackUnpublished(livekit::Room &r, const livekit::TrackUnpublishedEvent &e) {
    auto pub_sp = e.publication;
    std::string identity(e.participant ? e.participant->identity() : "");
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, pub_sp, identity]() {
        Ref<LiveKitRemoteTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pub_sp);
        Ref<LiveKitParticipant> p = room->_find_or_create_participant_by_identity(identity);
        room->emit_signal("track_unpublished", pub, p);
    });
}

void LiveKitRoom::GodotRoomDelegate::onTrackSubscribed(livekit::Room &r, const livekit::TrackSubscribedEvent &e) {
    auto track_sp = e.track;
    auto pub_sp = e.publication;
    std::string identity(e.participant ? e.participant->identity() : "");
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, track_sp, pub_sp, identity]() {
        Ref<LiveKitTrack> track;
        track.instantiate();
        track->bind_track(track_sp);
        Ref<LiveKitRemoteTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pub_sp);
        Ref<LiveKitParticipant> p = room->_find_or_create_participant_by_identity(identity);
        room->emit_signal("track_subscribed", track, pub, p);
    });
}

void LiveKitRoom::GodotRoomDelegate::onTrackUnsubscribed(livekit::Room &r, const livekit::TrackUnsubscribedEvent &e) {
    auto track_sp = e.track;
    auto pub_sp = e.publication;
    std::string identity(e.participant ? e.participant->identity() : "");
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, track_sp, pub_sp, identity]() {
        Ref<LiveKitTrack> track;
        track.instantiate();
        track->bind_track(track_sp);
        Ref<LiveKitRemoteTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pub_sp);
        Ref<LiveKitParticipant> p = room->_find_or_create_participant_by_identity(identity);
        room->emit_signal("track_unsubscribed", track, pub, p);
    });
}

void LiveKitRoom::GodotRoomDelegate::onTrackMuted(livekit::Room &r, const livekit::TrackMutedEvent &e) {
    auto pub_sp = e.publication;
    std::string identity(e.participant ? e.participant->identity() : "");
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, pub_sp, identity]() {
        Ref<LiveKitParticipant> p = room->_find_or_create_participant_by_identity(identity);
        Ref<LiveKitTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pub_sp);
        room->emit_signal("track_muted", p, pub);
    });
}

void LiveKitRoom::GodotRoomDelegate::onTrackUnmuted(livekit::Room &r, const livekit::TrackUnmutedEvent &e) {
    auto pub_sp = e.publication;
    std::string identity(e.participant ? e.participant->identity() : "");
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, pub_sp, identity]() {
        Ref<LiveKitParticipant> p = room->_find_or_create_participant_by_identity(identity);
        Ref<LiveKitTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pub_sp);
        room->emit_signal("track_unmuted", p, pub);
    });
}

void LiveKitRoom::GodotRoomDelegate::onLocalTrackPublished(livekit::Room &r, const livekit::LocalTrackPublishedEvent &e) {
    auto pub_sp = e.publication;
    auto track_sp = e.track;
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, pub_sp, track_sp]() {
        Ref<LiveKitLocalTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pub_sp);
        Ref<LiveKitTrack> track;
        track.instantiate();
        track->bind_track(track_sp);
        room->emit_signal("local_track_published", pub, track);
    });
}

void LiveKitRoom::GodotRoomDelegate::onLocalTrackUnpublished(livekit::Room &r, const livekit::LocalTrackUnpublishedEvent &e) {
    auto pub_sp = e.publication;
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, pub_sp]() {
        Ref<LiveKitLocalTrackPublication> pub;
        pub.instantiate();
        pub->bind_publication(pub_sp);
        room->emit_signal("local_track_unpublished", pub);
    });
}

void LiveKitRoom::GodotRoomDelegate::onUserPacketReceived(livekit::Room &r, const livekit::UserDataPacketEvent &e) {
    std::vector<uint8_t> data_copy(e.data.begin(), e.data.end());
    std::string identity(e.participant ? e.participant->identity() : "");
    int kind = (int)e.kind;
    std::string topic(e.topic);
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, data_copy, identity, kind, topic]() {
        PackedByteArray data;
        data.resize(data_copy.size());
        memcpy(data.ptrw(), data_copy.data(), data_copy.size());
        Ref<LiveKitParticipant> p;
        if (!identity.empty()) {
            p = room->_find_or_create_participant_by_identity(identity);
        }
        String gd_topic = topic.empty() ? String() : String(topic.c_str());
        room->emit_signal("data_received", data, p, kind, gd_topic);
    });
}

#ifdef LIVEKIT_E2EE_SUPPORTED
void LiveKitRoom::GodotRoomDelegate::onE2eeStateChanged(livekit::Room &r, const livekit::E2eeStateChangedEvent &e) {
    std::string identity(e.participant ? e.participant->identity() : "");
    int state = (int)e.state;
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, identity, state]() {
        Ref<LiveKitParticipant> p = room->_find_or_create_participant_by_identity(identity);
        if (p.is_valid()) {
            room->emit_signal("e2ee_state_changed", p, state);
        }
    });
}

void LiveKitRoom::GodotRoomDelegate::onParticipantEncryptionStatusChanged(livekit::Room &r, const livekit::ParticipantEncryptionStatusChangedEvent &e) {
    std::string identity(e.participant ? e.participant->identity() : "");
    bool encrypted = e.is_encrypted;
    std::lock_guard<std::mutex> lock(room->event_mutex_);
    room->pending_events_.push_back([this, identity, encrypted]() {
        Ref<LiveKitParticipant> p = room->_find_or_create_participant_by_identity(identity);
        if (p.is_valid()) {
            room->emit_signal("participant_encryption_status_changed", p, encrypted);
        }
    });
}
#endif
