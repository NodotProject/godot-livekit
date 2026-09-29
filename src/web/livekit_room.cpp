#include "livekit_room.h"

#include "livekit_track.h"
#include "livekit_track_publication.h"
#include "web_bridge.h"

#include <godot_cpp/classes/marshalls.hpp>

#include <algorithm>
#include <vector>

using namespace godot;

// Rooms with auto_poll enabled.
static std::vector<LiveKitRoom *> polled_rooms;

void LiveKitRoom::_bind_methods() {
    ClassDB::bind_method(D_METHOD("connect_to_room", "url", "token", "options"), &LiveKitRoom::connect_to_room);
    ClassDB::bind_method(D_METHOD("disconnect_from_room"), &LiveKitRoom::disconnect_from_room);
    ClassDB::bind_method(D_METHOD("poll_events"), &LiveKitRoom::poll_events);
    ClassDB::bind_method(D_METHOD("get_local_participant"), &LiveKitRoom::get_local_participant);
    ClassDB::bind_method(D_METHOD("get_remote_participants"), &LiveKitRoom::get_remote_participants);
    ClassDB::bind_method(D_METHOD("get_sid"), &LiveKitRoom::get_sid);
    ClassDB::bind_method(D_METHOD("get_name"), &LiveKitRoom::get_name);
    ClassDB::bind_method(D_METHOD("get_metadata"), &LiveKitRoom::get_metadata);
    ClassDB::bind_method(D_METHOD("get_connection_state"), &LiveKitRoom::get_connection_state);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "local_participant", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitLocalParticipant"), "", "get_local_participant");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "remote_participants"), "", "get_remote_participants");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "sid"), "", "get_sid");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "metadata"), "", "get_metadata");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "connection_state"), "", "get_connection_state");

    ClassDB::bind_method(D_METHOD("set_auto_poll", "enabled"), &LiveKitRoom::set_auto_poll);
    ClassDB::bind_method(D_METHOD("get_auto_poll"), &LiveKitRoom::get_auto_poll);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_poll"), "set_auto_poll", "get_auto_poll");

    // Connection signals
    ADD_SIGNAL(MethodInfo("connected"));
    ADD_SIGNAL(MethodInfo("disconnected"));
    ADD_SIGNAL(MethodInfo("connection_failed",
            PropertyInfo(Variant::STRING, "error")));
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

    BIND_ENUM_CONSTANT(STATE_DISCONNECTED);
    BIND_ENUM_CONSTANT(STATE_CONNECTED);
    BIND_ENUM_CONSTANT(STATE_RECONNECTING);
}

LiveKitRoom::LiveKitRoom() {
    js_id_ = web_call("room_create");
    polled_rooms.push_back(this);
}

LiveKitRoom::~LiveKitRoom() {
    polled_rooms.erase(std::remove(polled_rooms.begin(), polled_rooms.end(), this), polled_rooms.end());
    if (js_id_) {
        web_call("room_destroy", Array::make(js_id_));
    }
}

bool LiveKitRoom::connect_to_room(const String &url, const String &token, const Dictionary &options) {
    _reset_participants();
    connection_state_ = STATE_DISCONNECTED;
    web_call("room_connect", Array::make(js_id_, url, token, options));
    return true; // "connection started" — result arrives via signals
}

void LiveKitRoom::disconnect_from_room() {
    web_call("room_disconnect", Array::make(js_id_));
    _reset_participants();
    connection_state_ = STATE_DISCONNECTED;
}

void LiveKitRoom::poll_events() {
    Array events = web_call("room_take_events", Array::make(js_id_));
    for (int i = 0; i < events.size(); i++) {
        _dispatch(events[i]);
    }
}

void LiveKitRoom::_reset_participants() {
    local_participant_.unref();
    remote_participants_.clear();
}

// Returns the event's participant: the local participant, a known remote participant, or null.
Ref<LiveKitParticipant> LiveKitRoom::_participant(const Dictionary &p_event, bool p_create_remote) {
    if (p_event.get("local", false)) {
        return local_participant_;
    }
    String identity = p_event.get("identity", String());
    if (identity.is_empty()) {
        return Ref<LiveKitParticipant>();
    }
    if (!remote_participants_.has(identity) && p_create_remote) {
        Ref<LiveKitRemoteParticipant> p;
        p.instantiate();
        p->bind_web_participant(js_id_, identity, false);
        remote_participants_[identity] = p;
    }
    return remote_participants_.get(identity, Variant());
}

void LiveKitRoom::_dispatch(const Dictionary &p_event) {
    String type = p_event.get("type", String());
    if (type == "connected") {
        connection_state_ = STATE_CONNECTED;
        local_participant_.instantiate();
        local_participant_->bind_web_participant(js_id_, String(), true);
        Array identities = web_call("room_remote_identities", Array::make(js_id_));
        for (int i = 0; i < identities.size(); i++) {
            Dictionary event;
            event["identity"] = identities[i];
            _participant(event, true);
        }
        emit_signal("connected");
    } else if (type == "connection_failed") {
        connection_state_ = STATE_DISCONNECTED;
        emit_signal("connection_failed", p_event.get("error", String()));
    } else if (type == "disconnected") {
        connection_state_ = STATE_DISCONNECTED;
        emit_signal("disconnected");
    } else if (type == "reconnecting") {
        connection_state_ = STATE_RECONNECTING;
        emit_signal("reconnecting");
    } else if (type == "reconnected") {
        connection_state_ = STATE_CONNECTED;
        emit_signal("reconnected");
    } else if (type == "participant_connected") {
        emit_signal("participant_connected", _participant(p_event, true));
    } else if (type == "participant_disconnected") {
        Ref<LiveKitParticipant> p = _participant(p_event);
        if (p.is_valid()) {
            remote_participants_.erase(p->get_identity());
            emit_signal("participant_disconnected", p);
        }
    } else if (type == "room_metadata_changed") {
        emit_signal("room_metadata_changed", p_event.get("old", String()), p_event.get("new", String()));
    } else if (type == "connection_quality_changed") {
        Ref<LiveKitParticipant> p = _participant(p_event);
        if (p.is_valid()) {
            emit_signal("connection_quality_changed", p, int(p_event.get("quality", 0)));
        }
    } else if (type == "participant_metadata_changed" || type == "participant_name_changed") {
        Ref<LiveKitParticipant> p = _participant(p_event);
        if (p.is_valid()) {
            emit_signal(type, p, p_event.get("old", String()), p_event.get("new", String()));
        }
    } else if (type == "participant_attributes_changed") {
        Ref<LiveKitParticipant> p = _participant(p_event);
        if (p.is_valid()) {
            emit_signal("participant_attributes_changed", p, p_event.get("changed", Dictionary()));
        }
    } else if (type == "track_published" || type == "track_unpublished") {
        Ref<LiveKitTrackPublication> pub = LiveKitTrackPublication::from_info(p_event.get("publication", Dictionary()), true);
        emit_signal(type, pub, _participant(p_event, true));
    } else if (type == "track_subscribed" || type == "track_unsubscribed") {
        Ref<LiveKitTrack> track = LiveKitTrack::from_info(p_event.get("track", Dictionary()), true);
        Ref<LiveKitTrackPublication> pub = LiveKitTrackPublication::from_info(p_event.get("publication", Dictionary()), true);
        emit_signal(type, track, pub, _participant(p_event, true));
    } else if (type == "track_muted" || type == "track_unmuted") {
        bool local = p_event.get("local", false);
        Ref<LiveKitTrackPublication> pub = LiveKitTrackPublication::from_info(p_event.get("publication", Dictionary()), !local);
        emit_signal(type, _participant(p_event), pub);
    } else if (type == "local_track_published") {
        Dictionary pub_info = p_event.get("publication", Dictionary());
        Ref<LiveKitTrackPublication> pub = LiveKitTrackPublication::from_info(pub_info, false);
        emit_signal("local_track_published", pub, pub->get_track());
    } else if (type == "local_track_unpublished") {
        emit_signal("local_track_unpublished", LiveKitTrackPublication::from_info(p_event.get("publication", Dictionary()), false));
    } else if (type == "data_received") {
        PackedByteArray data = Marshalls::get_singleton()->base64_to_raw(p_event.get("data", String()));
        emit_signal("data_received", data, _participant(p_event), int(p_event.get("kind", 1)), p_event.get("topic", String()));
    }
}

Ref<LiveKitLocalParticipant> LiveKitRoom::get_local_participant() const {
    return local_participant_;
}

Dictionary LiveKitRoom::get_remote_participants() const {
    return remote_participants_.duplicate();
}

String LiveKitRoom::get_sid() const {
    return web_call_dict("room_info", Array::make(js_id_)).get("sid", String());
}

String LiveKitRoom::get_name() const {
    return web_call_dict("room_info", Array::make(js_id_)).get("name", String());
}

String LiveKitRoom::get_metadata() const {
    return web_call_dict("room_info", Array::make(js_id_)).get("metadata", String());
}

int LiveKitRoom::get_connection_state() const {
    return connection_state_;
}

void LiveKitRoom::set_auto_poll(bool enabled) {
    if (auto_poll_ == enabled) {
        return;
    }
    auto_poll_ = enabled;
    if (enabled) {
        polled_rooms.push_back(this);
    } else {
        polled_rooms.erase(std::remove(polled_rooms.begin(), polled_rooms.end(), this), polled_rooms.end());
    }
}

bool LiveKitRoom::get_auto_poll() const {
    return auto_poll_;
}

void LiveKitRoom::poll_all() {
    // Signal handlers may create or free rooms, so poll a snapshot and skip rooms freed since.
    std::vector<LiveKitRoom *> rooms = polled_rooms;
    for (LiveKitRoom *room : rooms) {
        if (std::find(polled_rooms.begin(), polled_rooms.end(), room) != polled_rooms.end()) {
            // Keeps the room alive even if a signal handler drops the last other reference.
            Ref<LiveKitRoom> keep_alive(room);
            room->poll_events();
        }
    }
}
