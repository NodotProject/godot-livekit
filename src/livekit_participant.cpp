#include "livekit_participant.h"

using namespace godot;

void LiveKitParticipant::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_sid"), &LiveKitParticipant::get_sid);
    ClassDB::bind_method(D_METHOD("get_name"), &LiveKitParticipant::get_name);
    ClassDB::bind_method(D_METHOD("get_identity"), &LiveKitParticipant::get_identity);
    ClassDB::bind_method(D_METHOD("get_metadata"), &LiveKitParticipant::get_metadata);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "sid"), "", "get_sid");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "identity"), "", "get_identity");
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "metadata"), "", "get_metadata");
}

LiveKitParticipant::LiveKitParticipant() {
}

LiveKitParticipant::~LiveKitParticipant() {
}

void LiveKitParticipant::bind_participant(livekit::Participant *p_participant) {
    participant_ = p_participant;
}

String LiveKitParticipant::get_sid() const {
    if (participant_) {
        return String(participant_->sid().c_str());
    }
    return String();
}

String LiveKitParticipant::get_name() const {
    if (participant_) {
        return String(participant_->name().c_str());
    }
    return String();
}

String LiveKitParticipant::get_identity() const {
    if (participant_) {
        return String(participant_->identity().c_str());
    }
    return String();
}

String LiveKitParticipant::get_metadata() const {
    if (participant_) {
        return String(participant_->metadata().c_str());
    }
    return String();
}

// LiveKitLocalParticipant

void LiveKitLocalParticipant::_bind_methods() {
}

LiveKitLocalParticipant::LiveKitLocalParticipant() {
}

LiveKitLocalParticipant::~LiveKitLocalParticipant() {
}

// LiveKitRemoteParticipant

void LiveKitRemoteParticipant::_bind_methods() {
}

LiveKitRemoteParticipant::LiveKitRemoteParticipant() {
}

LiveKitRemoteParticipant::~LiveKitRemoteParticipant() {
}
