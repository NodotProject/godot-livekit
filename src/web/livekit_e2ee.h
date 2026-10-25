#ifndef GODOT_LIVEKIT_E2EE_H
#define GODOT_LIVEKIT_E2EE_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

// Web implementation of LiveKitE2eeOptions. Same properties and defaults as the native binding.
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

    // The options as bridge.js's room_connect takes them.
    Dictionary to_web() const;
};

// Web implementation of LiveKitKeyProvider. Keys are applied asynchronously, in order. Ratcheting
// is asynchronous too, so ratchet_shared_key() and ratchet_key() return an empty array, and the
// new key is available from get_shared_key() and get_key() once the ratchet is done.
class LiveKitKeyProvider : public RefCounted {
    GDCLASS(LiveKitKeyProvider, RefCounted)

private:
    int e2ee_js_id_ = 0;

protected:
    static void _bind_methods();

public:
    void bind_web_e2ee(int p_e2ee_js_id) { e2ee_js_id_ = p_e2ee_js_id; }

    void set_shared_key(const PackedByteArray &key, int key_index);
    PackedByteArray get_shared_key(int key_index) const;
    PackedByteArray ratchet_shared_key(int key_index);

    void set_key(const String &participant_identity, const PackedByteArray &key, int key_index);
    PackedByteArray get_key(const String &participant_identity, int key_index) const;
    PackedByteArray ratchet_key(const String &participant_identity, int key_index);
};

// Web implementation of LiveKitFrameCryptor: a snapshot of one track's encryption settings.
class LiveKitFrameCryptor : public RefCounted {
    GDCLASS(LiveKitFrameCryptor, RefCounted)

private:
    int e2ee_js_id_ = 0;
    String participant_identity_;
    int key_index_ = 0;
    bool enabled_ = false;

protected:
    static void _bind_methods();

public:
    // Binds to a cryptor described by bridge.js's e2ee_frame_cryptors.
    void bind_web_cryptor(int p_e2ee_js_id, const Dictionary &p_info);

    String get_participant_identity() const;
    int get_key_index() const;
    bool get_enabled() const;

    void set_enabled(bool enabled);
    void set_key_index(int key_index);
};

// Web implementation of LiveKitE2eeManager, for one connection.
class LiveKitE2eeManager : public RefCounted {
    GDCLASS(LiveKitE2eeManager, RefCounted)

private:
    int e2ee_js_id_ = 0;
    Ref<LiveKitKeyProvider> key_provider_;

protected:
    static void _bind_methods();

public:
    void bind_web_e2ee(int p_e2ee_js_id);

    bool get_enabled() const;
    void set_enabled(bool enabled);

    Ref<LiveKitKeyProvider> get_key_provider();
    Array get_frame_cryptors() const;
};

}

VARIANT_ENUM_CAST(LiveKitE2eeOptions::EncryptionType);

#endif // GODOT_LIVEKIT_E2EE_H
