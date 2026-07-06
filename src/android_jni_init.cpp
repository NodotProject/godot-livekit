// Fix for issue #3: SIGSEGV inside connect_to_room on Android arm64.
//
// LiveKit's Rust WebRTC layer (liblivekit_ffi.so) requires JNI initialization
// on Android before it touches AudioRecord/AudioManager/MediaCodec; without it
// the first connect crashes with a null-pointer dereference on a tokio worker
// thread. liblivekit_ffi.so ships WebRTC's Android JNI glue and exports its own
// JNI_OnLoad, but the Java runtime only auto-invokes JNI_OnLoad on the library
// named in System.loadLibrary — dependencies pulled in by the dynamic linker
// (like liblivekit_ffi.so) never get the call. So we forward it ourselves.
//
// Two paths, in order of preference:
//  1. This library's own JNI_OnLoad below fires when the extension is loaded
//     via System.loadLibrary, and forwards the received JavaVM. (Same pattern
//     decentraland/godot-explorer ships on the Play Store.)
//  2. If the engine dlopen()ed us instead (no JNI_OnLoad), we recover the
//     already-created VM via JNI_GetCreatedJavaVMs at module init time.

#ifdef ANDROID_ENABLED

#include "android_jni_init.h"

#include <dlfcn.h>
#include <jni.h>

#include <godot_cpp/variant/utility_functions.hpp>

namespace {

using JniOnLoadFn = jint (*)(JavaVM *, void *);
using GetCreatedVMsFn = jint (*)(JavaVM **, jsize, jsize *);

bool jni_initialized = false;

void forward_to_livekit_ffi(JavaVM *vm) {
    if (jni_initialized || vm == nullptr) {
        return;
    }
    // liblivekit_ffi.so is already loaded as a DT_NEEDED dependency of this
    // library; dlopen just hands us a handle to it.
    void *ffi = dlopen("liblivekit_ffi.so", RTLD_NOW | RTLD_GLOBAL);
    if (ffi == nullptr) {
        godot::UtilityFunctions::push_error(
                "godot-livekit android: dlopen(liblivekit_ffi.so) failed: ",
                dlerror());
        return;
    }
    JniOnLoadFn on_load = reinterpret_cast<JniOnLoadFn>(dlsym(ffi, "JNI_OnLoad"));
    if (on_load == nullptr) {
        godot::UtilityFunctions::push_error(
                "godot-livekit android: liblivekit_ffi.so does not export JNI_OnLoad");
        return;
    }
    on_load(vm, nullptr);
    jni_initialized = true;
}

// JNI_GetCreatedJavaVMs is not part of the stable NDK surface, so probe the
// dynamic linker for it instead of linking against it.
JavaVM *query_created_vm() {
    GetCreatedVMsFn get_vms = reinterpret_cast<GetCreatedVMsFn>(
            dlsym(RTLD_DEFAULT, "JNI_GetCreatedJavaVMs"));
    if (get_vms == nullptr) {
        const char *candidates[] = { "libart.so", "libnativehelper.so" };
        for (const char *lib : candidates) {
            void *handle = dlopen(lib, RTLD_NOW | RTLD_NOLOAD);
            if (handle != nullptr) {
                get_vms = reinterpret_cast<GetCreatedVMsFn>(
                        dlsym(handle, "JNI_GetCreatedJavaVMs"));
                if (get_vms != nullptr) {
                    break;
                }
            }
        }
    }
    if (get_vms == nullptr) {
        return nullptr;
    }
    JavaVM *vm = nullptr;
    jsize count = 0;
    if (get_vms(&vm, 1, &count) != JNI_OK || count == 0) {
        return nullptr;
    }
    return vm;
}

} // namespace

extern "C" {

// Invoked automatically by the Android runtime when this library is loaded
// via System.loadLibrary.
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void * /*reserved*/) {
    forward_to_livekit_ffi(vm);
    return JNI_VERSION_1_6;
}

} // extern "C"

void livekit_android_jni_init() {
    if (jni_initialized) {
        return;
    }
    // JNI_OnLoad never ran, so the library was dlopen()ed. Recover the VM.
    forward_to_livekit_ffi(query_created_vm());
    if (!jni_initialized) {
        godot::UtilityFunctions::push_error(
                "godot-livekit android: could not obtain the JavaVM (JNI_OnLoad "
                "never ran and JNI_GetCreatedJavaVMs is unavailable). LiveKit's "
                "WebRTC layer is uninitialized and connect_to_room will crash.");
    }
}

#endif // ANDROID_ENABLED
