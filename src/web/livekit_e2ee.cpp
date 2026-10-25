#include "livekit_e2ee.h"

#include "web_bridge.h"

#include <godot_cpp/classes/marshalls.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// The native SDK's default (livekit::kDefaultRatchetSalt).
static const char *DEFAULT_RATCHET_SALT = "LKFrameEncryptionKey";

// --- LiveKitE2eeOptions ---

void LiveKitE2eeOptions::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_encryption_type", "type"), &LiveKitE2eeOptions::set_encryption_type);
    ClassDB::bind_method(D_METHOD("get_encryption_type"), &LiveKitE2eeOptions::get_encryption_type);
    ClassDB::bind_method(D_METHOD("set_shared_key", "key"), &LiveKitE2eeOptions::set_shared_key);
    ClassDB::bind_method(D_METHOD("get_shared_key"), &LiveKitE2eeOptions::get_shared_key);
    ClassDB::bind_method(D_METHOD("set_ratchet_salt", "salt"), &LiveKitE2eeOptions::set_ratchet_salt);
    ClassDB::bind_method(D_METHOD("get_ratchet_salt"), &LiveKitE2eeOptions::get_ratchet_salt);
    ClassDB::bind_method(D_METHOD("set_ratchet_window_size", "size"), &LiveKitE2eeOptions::set_ratchet_window_size);
    ClassDB::bind_method(D_METHOD("get_ratchet_window_size"), &LiveKitE2eeOptions::get_ratchet_window_size);
    ClassDB::bind_method(D_METHOD("set_failure_tolerance", "tolerance"), &LiveKitE2eeOptions::set_failure_tolerance);
    ClassDB::bind_method(D_METHOD("get_failure_tolerance"), &LiveKitE2eeOptions::get_failure_tolerance);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "encryption_type"), "set_encryption_type", "get_encryption_type");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "shared_key"), "set_shared_key", "get_shared_key");
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "ratchet_salt"), "set_ratchet_salt", "get_ratchet_salt");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "ratchet_window_size"), "set_ratchet_window_size", "get_ratchet_window_size");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "failure_tolerance"), "set_failure_tolerance", "get_failure_tolerance");

    BIND_ENUM_CONSTANT(ENCRYPTION_NONE);
    BIND_ENUM_CONSTANT(ENCRYPTION_GCM);
    BIND_ENUM_CONSTANT(ENCRYPTION_CUSTOM);
}

LiveKitE2eeOptions::LiveKitE2eeOptions() {
    ratchet_salt_ = String(DEFAULT_RATCHET_SALT).to_utf8_buffer();
}

void LiveKitE2eeOptions::set_encryption_type(EncryptionType type) {
    encryption_type_ = type;
}

LiveKitE2eeOptions::EncryptionType LiveKitE2eeOptions::get_encryption_type() const {
    return encryption_type_;
}

void LiveKitE2eeOptions::set_shared_key(const PackedByteArray &key) {
    shared_key_ = key;
}

PackedByteArray LiveKitE2eeOptions::get_shared_key() const {
    return shared_key_;
}

void LiveKitE2eeOptions::set_ratchet_salt(const PackedByteArray &salt) {
    ratchet_salt_ = salt;
}

PackedByteArray LiveKitE2eeOptions::get_ratchet_salt() const {
    return ratchet_salt_;
}

void LiveKitE2eeOptions::set_ratchet_window_size(int size) {
    ratchet_window_size_ = size;
}

int LiveKitE2eeOptions::get_ratchet_window_size() const {
    return ratchet_window_size_;
}

void LiveKitE2eeOptions::set_failure_tolerance(int tolerance) {
    failure_tolerance_ = tolerance;
}

int LiveKitE2eeOptions::get_failure_tolerance() const {
    return failure_tolerance_;
}

Dictionary LiveKitE2eeOptions::to_web() const {
    Marshalls *marshalls = Marshalls::get_singleton();
    Dictionary options;
    options["encryption_type"] = encryption_type_;
    options["shared_key"] = shared_key_.is_empty() ? Variant() : Variant(marshalls->raw_to_base64(shared_key_));
    // As in the native SDK, an empty salt means the default.
    options["ratchet_salt"] = marshalls->raw_to_base64(ratchet_salt_.is_empty() ? String(DEFAULT_RATCHET_SALT).to_utf8_buffer() : ratchet_salt_);
    options["ratchet_window_size"] = ratchet_window_size_;
    options["failure_tolerance"] = failure_tolerance_;
    return options;
}

// --- LiveKitKeyProvider ---

void LiveKitKeyProvider::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_shared_key", "key", "key_index"), &LiveKitKeyProvider::set_shared_key, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_shared_key", "key_index"), &LiveKitKeyProvider::get_shared_key, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("ratchet_shared_key", "key_index"), &LiveKitKeyProvider::ratchet_shared_key, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("set_key", "participant_identity", "key", "key_index"), &LiveKitKeyProvider::set_key, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("get_key", "participant_identity", "key_index"), &LiveKitKeyProvider::get_key, DEFVAL(0));
    ClassDB::bind_method(D_METHOD("ratchet_key", "participant_identity", "key_index"), &LiveKitKeyProvider::ratchet_key, DEFVAL(0));
}

void LiveKitKeyProvider::set_shared_key(const PackedByteArray &key, int key_index) {
    set_key(String(), key, key_index);
}

PackedByteArray LiveKitKeyProvider::get_shared_key(int key_index) const {
    return get_key(String(), key_index);
}

PackedByteArray LiveKitKeyProvider::ratchet_shared_key(int key_index) {
    return ratchet_key(String(), key_index);
}

void LiveKitKeyProvider::set_key(const String &participant_identity, const PackedByteArray &key, int key_index) {
    if (!e2ee_js_id_) {
        UtilityFunctions::push_error("LiveKitKeyProvider::set_key: not bound");
        return;
    }
    web_call("e2ee_set_key", Array::make(e2ee_js_id_, participant_identity, Marshalls::get_singleton()->raw_to_base64(key), key_index));
}

PackedByteArray LiveKitKeyProvider::get_key(const String &participant_identity, int key_index) const {
    if (!e2ee_js_id_) {
        UtilityFunctions::push_error("LiveKitKeyProvider::get_key: not bound");
        return PackedByteArray();
    }
    Variant key = web_call("e2ee_get_key", Array::make(e2ee_js_id_, participant_identity, key_index));
    return key.get_type() == Variant::STRING ? Marshalls::get_singleton()->base64_to_raw(key) : PackedByteArray();
}

PackedByteArray LiveKitKeyProvider::ratchet_key(const String &participant_identity, int key_index) {
    if (!e2ee_js_id_) {
        UtilityFunctions::push_error("LiveKitKeyProvider::ratchet_key: not bound");
        return PackedByteArray();
    }
    web_call("e2ee_ratchet_key", Array::make(e2ee_js_id_, participant_identity, key_index));
    return PackedByteArray();
}

// --- LiveKitFrameCryptor ---

void LiveKitFrameCryptor::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_participant_identity"), &LiveKitFrameCryptor::get_participant_identity);
    ClassDB::bind_method(D_METHOD("get_key_index"), &LiveKitFrameCryptor::get_key_index);
    ClassDB::bind_method(D_METHOD("get_enabled"), &LiveKitFrameCryptor::get_enabled);
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &LiveKitFrameCryptor::set_enabled);
    ClassDB::bind_method(D_METHOD("set_key_index", "key_index"), &LiveKitFrameCryptor::set_key_index);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "participant_identity"), "", "get_participant_identity");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "key_index"), "set_key_index", "get_key_index");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "get_enabled");
}

void LiveKitFrameCryptor::bind_web_cryptor(int p_e2ee_js_id, const Dictionary &p_info) {
    e2ee_js_id_ = p_e2ee_js_id;
    participant_identity_ = p_info.get("identity", String());
    key_index_ = p_info.get("key_index", 0);
    enabled_ = p_info.get("enabled", false);
}

String LiveKitFrameCryptor::get_participant_identity() const {
    return participant_identity_;
}

int LiveKitFrameCryptor::get_key_index() const {
    return key_index_;
}

bool LiveKitFrameCryptor::get_enabled() const {
    return enabled_;
}

void LiveKitFrameCryptor::set_enabled(bool enabled) {
    if (!e2ee_js_id_) {
        UtilityFunctions::push_error("LiveKitFrameCryptor::set_enabled: not bound");
        return;
    }
    web_call("e2ee_cryptor_set_enabled", Array::make(e2ee_js_id_, participant_identity_, enabled));
    enabled_ = enabled;
}

void LiveKitFrameCryptor::set_key_index(int key_index) {
    if (!e2ee_js_id_) {
        UtilityFunctions::push_error("LiveKitFrameCryptor::set_key_index: not bound");
        return;
    }
    web_call("e2ee_cryptor_set_key_index", Array::make(e2ee_js_id_, participant_identity_, key_index));
    key_index_ = key_index;
}

// --- LiveKitE2eeManager ---

void LiveKitE2eeManager::_bind_methods() {
    ClassDB::bind_method(D_METHOD("get_enabled"), &LiveKitE2eeManager::get_enabled);
    ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &LiveKitE2eeManager::set_enabled);
    ClassDB::bind_method(D_METHOD("get_key_provider"), &LiveKitE2eeManager::get_key_provider);
    ClassDB::bind_method(D_METHOD("get_frame_cryptors"), &LiveKitE2eeManager::get_frame_cryptors);

    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "get_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "key_provider", PROPERTY_HINT_RESOURCE_TYPE, "LiveKitKeyProvider"), "", "get_key_provider");
}

void LiveKitE2eeManager::bind_web_e2ee(int p_e2ee_js_id) {
    e2ee_js_id_ = p_e2ee_js_id;
    key_provider_.unref();
}

bool LiveKitE2eeManager::get_enabled() const {
    return e2ee_js_id_ && bool(web_call("e2ee_get_enabled", Array::make(e2ee_js_id_)));
}

void LiveKitE2eeManager::set_enabled(bool enabled) {
    if (!e2ee_js_id_) {
        UtilityFunctions::push_error("LiveKitE2eeManager::set_enabled: not bound");
        return;
    }
    web_call("e2ee_set_enabled", Array::make(e2ee_js_id_, enabled));
}

Ref<LiveKitKeyProvider> LiveKitE2eeManager::get_key_provider() {
    if (!e2ee_js_id_) {
        return Ref<LiveKitKeyProvider>();
    }
    if (key_provider_.is_null()) {
        key_provider_.instantiate();
        key_provider_->bind_web_e2ee(e2ee_js_id_);
    }
    return key_provider_;
}

Array LiveKitE2eeManager::get_frame_cryptors() const {
    Array result;
    if (!e2ee_js_id_) {
        return result;
    }
    Variant infos = web_call("e2ee_frame_cryptors", Array::make(e2ee_js_id_));
    if (infos.get_type() != Variant::ARRAY) {
        return result;
    }
    for (const Variant &info : Array(infos)) {
        Ref<LiveKitFrameCryptor> cryptor;
        cryptor.instantiate();
        cryptor->bind_web_cryptor(e2ee_js_id_, info);
        result.push_back(cryptor);
    }
    return result;
}
