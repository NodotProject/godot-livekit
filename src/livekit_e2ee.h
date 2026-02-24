#ifndef GODOT_LIVEKIT_E2EE_H
#define GODOT_LIVEKIT_E2EE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

#include <livekit/e2ee.h>

namespace godot {

class LiveKitE2eeOptions : public RefCounted {
    GDCLASS(LiveKitE2eeOptions, RefCounted)

public:
    enum EncryptionType {
        ENCRYPTION_NONE = 0,
        ENCRYPTION_GCM = 1,
        ENCRYPTION_CUSTOM = 2,
    };

private:
    EncryptionType encryption_type_ = ENCRYPTION_GCM;
    PackedByteArray shared_key_;
    PackedByteArray ratchet_salt_;
    int ratchet_window_size_ = 16;
    int failure_tolerance_ = -1;

protected:
    static void _bind_methods();

public:
    LiveKitE2eeOptions();
    ~LiveKitE2eeOptions();

    void set_encryption_type(EncryptionType type);
    EncryptionType get_encryption_type() const;

    void set_shared_key(const PackedByteArray &key);
    PackedByteArray get_shared_key() const;

    void set_ratchet_salt(const PackedByteArray &salt);
    PackedByteArray get_ratchet_salt() const;

    void set_ratchet_window_size(int size);
    int get_ratchet_window_size() const;

    void set_failure_tolerance(int tolerance);
    int get_failure_tolerance() const;

    livekit::E2EEOptions to_native() const;
};

class LiveKitKeyProvider : public RefCounted {
    GDCLASS(LiveKitKeyProvider, RefCounted)

private:
    livekit::E2EEManager::KeyProvider *key_provider_ = nullptr;

protected:
    static void _bind_methods();

public:
    LiveKitKeyProvider();
    ~LiveKitKeyProvider();

    void bind_key_provider(livekit::E2EEManager::KeyProvider *kp);

    void set_shared_key(const PackedByteArray &key, int key_index);
    PackedByteArray get_shared_key(int key_index) const;
    PackedByteArray ratchet_shared_key(int key_index);

    void set_key(const String &participant_identity, const PackedByteArray &key, int key_index);
    PackedByteArray get_key(const String &participant_identity, int key_index) const;
    PackedByteArray ratchet_key(const String &participant_identity, int key_index);
};

class LiveKitFrameCryptor : public RefCounted {
    GDCLASS(LiveKitFrameCryptor, RefCounted)

private:
    String participant_identity_;
    int key_index_ = 0;
    bool enabled_ = false;

    // We store copies since FrameCryptor objects from frameCryptors() are returned by value
    livekit::E2EEManager::FrameCryptor *frame_cryptor_ = nullptr;

protected:
    static void _bind_methods();

public:
    LiveKitFrameCryptor();
    ~LiveKitFrameCryptor();

    void bind_frame_cryptor(livekit::E2EEManager::FrameCryptor *fc);

    String get_participant_identity() const;
    int get_key_index() const;
    bool get_enabled() const;

    void set_enabled(bool enabled);
    void set_key_index(int key_index);
};

class LiveKitE2eeManager : public RefCounted {
    GDCLASS(LiveKitE2eeManager, RefCounted)

private:
    livekit::E2EEManager *e2ee_manager_ = nullptr;
    Ref<LiveKitKeyProvider> key_provider_;

protected:
    static void _bind_methods();

public:
    LiveKitE2eeManager();
    ~LiveKitE2eeManager();

    void bind_manager(livekit::E2EEManager *mgr);

    bool get_enabled() const;
    void set_enabled(bool enabled);

    Ref<LiveKitKeyProvider> get_key_provider();
    Array get_frame_cryptors() const;
};

}

VARIANT_ENUM_CAST(LiveKitE2eeOptions::EncryptionType);

#endif // GODOT_LIVEKIT_E2EE_H
