// Fix for issue #3: crash inside connect_to_room on Android arm64.
//
// Two Android-specific requirements meet here:
//
// 1. LiveKit's WebRTC layer (liblivekit_ffi.so) needs JNI initialization —
//    its JNI_OnLoad wires the org.webrtc Java glue (on Android, WebRTC does
//    all mic/speaker I/O through Java AudioRecord/AudioTrack). Godot loads
//    GDExtensions with dlopen(), so that JNI_OnLoad never runs by itself.
//
// 2. ART resolves a Java native method ONLY against libraries loaded with
//    System.loadLibrary in that class's classloader. A dlopen()ed library's
//    exported Java_* symbols are invisible to resolution, so merely calling
//    the FFI's JNI_OnLoad ourselves is not enough — org.webrtc.* natives
//    still throw UnsatisfiedLinkError (verified on Quest 3 / Android 14).
//
// Both are solved the same way: at module init, reflectively call
// System.loadLibrary for the FFI library (fires its JNI_OnLoad exactly once
// AND registers its natives with the app classloader) and for this library
// (registers the AV1 stubs below). In app processes a caller-less JNI
// System.loadLibrary resolves to ClassLoader.getSystemClassLoader(), which
// is the application PathClassLoader — the same loader that holds the
// org.webrtc classes from libwebrtc.jar.
//
// Packaging note: the org.webrtc Java classes themselves come from
// libwebrtc.jar (from the same prebuilt the FFI is built against; see the
// rust-sdks webrtc release tag) and must be included in the app's gradle
// build, e.g. android/build/libs/{debug,release}/ in a Godot project.

#ifdef ANDROID_ENABLED

#include "android_jni_init.h"

#include <dlfcn.h>
#include <jni.h>

#include <godot_cpp/variant/utility_functions.hpp>

namespace {

using GetCreatedVMsFn = jint (*)(JavaVM **, jsize, jsize *);

bool jni_initialized = false;

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

bool java_load_library(JNIEnv *env, const char *name) {
    jclass system_cls = env->FindClass("java/lang/System");
    if (system_cls == nullptr) {
        env->ExceptionClear();
        godot::UtilityFunctions::push_error(
                "godot-livekit android: FindClass(java/lang/System) failed");
        return false;
    }
    jmethodID load_library = env->GetStaticMethodID(
            system_cls, "loadLibrary", "(Ljava/lang/String;)V");
    if (load_library == nullptr) {
        env->ExceptionClear();
        env->DeleteLocalRef(system_cls);
        godot::UtilityFunctions::push_error(
                "godot-livekit android: System.loadLibrary method not found");
        return false;
    }
    jstring jname = env->NewStringUTF(name);
    env->CallStaticVoidMethod(system_cls, load_library, jname);
    bool ok = true;
    if (env->ExceptionCheck()) {
        env->ExceptionDescribe();  // details go to logcat
        env->ExceptionClear();
        godot::UtilityFunctions::push_error(
                "godot-livekit android: System.loadLibrary(", name,
                ") threw — org.webrtc natives will not resolve");
        ok = false;
    }
    env->DeleteLocalRef(jname);
    env->DeleteLocalRef(system_cls);
    return ok;
}

} // namespace

extern "C" {

// Fires only if this library itself is loaded via System.loadLibrary (either
// by livekit_android_jni_init below, or by a future Godot that Java-loads
// GDExtensions). Registration side effects are handled by the runtime; there
// is nothing else to do here.
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM * /*vm*/, void * /*reserved*/) {
    return JNI_VERSION_1_6;
}

// libwebrtc.jar declares AV1 codec classes whose native methods
// liblivekit_ffi.so does not export (libaom is not compiled in). Stub them so
// codec enumeration reports AV1 unsupported instead of throwing
// UnsatisfiedLinkError. (decentraland/godot-explorer ships the same stub.)
JNIEXPORT jboolean JNICALL
Java_org_webrtc_LibaomAv1Decoder_nativeIsSupported(JNIEnv *, jclass) {
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_org_webrtc_LibaomAv1Encoder_nativeIsSupported(JNIEnv *, jclass) {
    return JNI_FALSE;
}

} // extern "C"

void livekit_android_jni_init() {
    if (jni_initialized) {
        return;
    }
    JavaVM *vm = query_created_vm();
    if (vm == nullptr) {
        godot::UtilityFunctions::push_error(
                "godot-livekit android: could not obtain the JavaVM "
                "(JNI_GetCreatedJavaVMs unavailable). LiveKit's WebRTC layer "
                "is uninitialized and connect_to_room will crash.");
        return;
    }
    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK
            || env == nullptr) {
        if (vm->AttachCurrentThread(&env, nullptr) != JNI_OK || env == nullptr) {
            godot::UtilityFunctions::push_error(
                    "godot-livekit android: could not attach a JNIEnv");
            return;
        }
    }
    // Order matters: the FFI library first (WebRTC JNI init + native
    // registration), then this library (AV1 stubs). Both calls are idempotent
    // for already-Java-loaded libraries.
    bool ok = java_load_library(env, "livekit_ffi");
    ok = java_load_library(env, "godot-livekit.android.arm64") && ok;
    jni_initialized = ok;
}

#endif // ANDROID_ENABLED
