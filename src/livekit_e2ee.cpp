#include "livekit_e2ee.h"

#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/array.hpp>

using namespace godot;

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
    // Set default ratchet salt to match SDK default
    const char *default_salt = livekit::kDefaultRatchetSalt;
    size_t salt_len = std::char_traits<char>::length(default_salt);
    ratchet_salt_.resize(salt_len);
    memcpy(ratchet_salt_.ptrw(), default_salt, salt_len);
}

LiveKitE2eeOptions::~LiveKitE2eeOptions() {
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

livekit::E2EEOptions LiveKitE2eeOptions::to_native() const {
    livekit::E2EEOptions opts;
    opts.encryption_type = static_cast<livekit::EncryptionType>(encryption_type_);

    livekit::KeyProviderOptions &kpo = opts.key_provider_options;

    if (shared_key_.size() > 0) {
        kpo.shared_key = std::vector<uint8_t>(shared_key_.ptr(), shared_key_.ptr() + shared_key_.size());
    }

    if (ratchet_salt_.size() > 0) {
        kpo.ratchet_salt = std::vector<uint8_t>(ratchet_salt_.ptr(), ratchet_salt_.ptr() + ratchet_salt_.size());
    }

    kpo.ratchet_window_size = ratchet_window_size_;
    kpo.failure_tolerance = failure_tolerance_;

    return opts;
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

LiveKitKeyProvider::LiveKitKeyProvider() {
}

LiveKitKeyProvider::~LiveKitKeyProvider() {
}

void LiveKitKeyProvider::bind_key_provider(livekit::E2EEManager::KeyProvider *kp) {
    key_provider_ = kp;
}

void LiveKitKeyProvider::set_shared_key(const PackedByteArray &key, int key_index) {
    if (!key_provider_) {
        UtilityFunctions::push_error("LiveKitKeyProvider::set_shared_key: not bound");
        return;
    }
    std::vector<uint8_t> k(key.ptr(), key.ptr() + key.size());
    key_provider_->setSharedKey(k, key_index);
}

PackedByteArray LiveKitKeyProvider::get_shared_key(int key_index) const {
    PackedByteArray result;
    if (!key_provider_) {
        UtilityFunctions::push_error("LiveKitKeyProvider::get_shared_key: not bound");
        return result;
    }
    auto k = key_provider_->exportSharedKey(key_index);
    result.resize(k.size());
    memcpy(result.ptrw(), k.data(), k.size());
    return result;
}

PackedByteArray LiveKitKeyProvider::ratchet_shared_key(int key_index) {
    PackedByteArray result;
    if (!key_provider_) {
        UtilityFunctions::push_error("LiveKitKeyProvider::ratchet_shared_key: not bound");
        return result;
    }
    auto k = key_provider_->ratchetSharedKey(key_index);
    result.resize(k.size());
    memcpy(result.ptrw(), k.data(), k.size());
    return result;
}

void LiveKitKeyProvider::set_key(const String &participant_identity, const PackedByteArray &key, int key_index) {
    if (!key_provider_) {
        UtilityFunctions::push_error("LiveKitKeyProvider::set_key: not bound");
        return;
    }
    std::string identity = participant_identity.utf8().get_data();
    std::vector<uint8_t> k(key.ptr(), key.ptr() + key.size());
    key_provider_->setKey(identity, k, key_index);
}

PackedByteArray LiveKitKeyProvider::get_key(const String &participant_identity, int key_index) const {
    PackedByteArray result;
    if (!key_provider_) {
        UtilityFunctions::push_error("LiveKitKeyProvider::get_key: not bound");
        return result;
    }
    std::string identity = participant_identity.utf8().get_data();
    auto k = key_provider_->exportKey(identity, key_index);
    result.resize(k.size());
    memcpy(result.ptrw(), k.data(), k.size());
    return result;
}

PackedByteArray LiveKitKeyProvider::ratchet_key(const String &participant_identity, int key_index) {
    PackedByteArray result;
    if (!key_provider_) {
        UtilityFunctions::push_error("LiveKitKeyProvider::ratchet_key: not bound");
        return result;
    }
    std::string identity = participant_identity.utf8().get_data();
    auto k = key_provider_->ratchetKey(identity, key_index);
    result.resize(k.size());
    memcpy(result.ptrw(), k.data(), k.size());
    return result;
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

LiveKitFrameCryptor::LiveKitFrameCryptor() {
}

LiveKitFrameCryptor::~LiveKitFrameCryptor() {
}

void LiveKitFrameCryptor::bind_frame_cryptor(std::unique_ptr<livekit::E2EEManager::FrameCryptor> fc) {
    frame_cryptor_ = std::move(fc);
    if (frame_cryptor_) {
        participant_identity_ = String(frame_cryptor_->participantIdentity().c_str());
        key_index_ = frame_cryptor_->keyIndex();
        enabled_ = frame_cryptor_->enabled();
    }
}

String LiveKitFrameCryptor::get_participant_identity() const {
    return participant_identity_;
}

int LiveKitFrameCryptor::get_key_index() const {
    if (frame_cryptor_) {
        return frame_cryptor_->keyIndex();
    }
    return key_index_;
}

bool LiveKitFrameCryptor::get_enabled() const {
    if (frame_cryptor_) {
        return frame_cryptor_->enabled();
    }
    return enabled_;
}

void LiveKitFrameCryptor::set_enabled(bool enabled) {
    if (!frame_cryptor_) {
        UtilityFunctions::push_error("LiveKitFrameCryptor::set_enabled: not bound");
        return;
    }
    frame_cryptor_->setEnabled(enabled);
    enabled_ = enabled;
}

void LiveKitFrameCryptor::set_key_index(int key_index) {
    if (!frame_cryptor_) {
        UtilityFunctions::push_error("LiveKitFrameCryptor::set_key_index: not bound");
        return;
    }
    frame_cryptor_->setKeyIndex(key_index);
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

LiveKitE2eeManager::LiveKitE2eeManager() {
}

LiveKitE2eeManager::~LiveKitE2eeManager() {
}

void LiveKitE2eeManager::bind_manager(livekit::E2EEManager *mgr) {
    e2ee_manager_ = mgr;
    key_provider_.unref();
}

bool LiveKitE2eeManager::get_enabled() const {
    if (!e2ee_manager_) {
        return false;
    }
    return e2ee_manager_->enabled();
}

void LiveKitE2eeManager::set_enabled(bool enabled) {
    if (!e2ee_manager_) {
        UtilityFunctions::push_error("LiveKitE2eeManager::set_enabled: not bound");
        return;
    }
    e2ee_manager_->setEnabled(enabled);
}

Ref<LiveKitKeyProvider> LiveKitE2eeManager::get_key_provider() {
    if (!e2ee_manager_) {
        return Ref<LiveKitKeyProvider>();
    }

    livekit::E2EEManager::KeyProvider *kp = e2ee_manager_->keyProvider();
    if (!kp) {
        return Ref<LiveKitKeyProvider>();
    }

    if (!key_provider_.is_valid()) {
        key_provider_.instantiate();
        key_provider_->bind_key_provider(kp);
    }
    return key_provider_;
}

Array LiveKitE2eeManager::get_frame_cryptors() const {
    Array result;
    if (!e2ee_manager_) {
        return result;
    }

    auto cryptors = e2ee_manager_->frameCryptors();
    for (auto &fc : cryptors) {
        Ref<LiveKitFrameCryptor> godot_fc;
        godot_fc.instantiate();
        godot_fc->bind_frame_cryptor(std::make_unique<livekit::E2EEManager::FrameCryptor>(std::move(fc)));
        result.push_back(godot_fc);
    }
    return result;
}
