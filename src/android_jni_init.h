#ifndef ANDROID_JNI_INIT_H
#define ANDROID_JNI_INIT_H

#ifdef ANDROID_ENABLED

// Ensures LiveKit's WebRTC layer has its JNI globals initialized before the
// SDK is used. Safe to call multiple times; must run before livekit::initialize().
// See android_jni_init.cpp for details (fix for issue #3).
void livekit_android_jni_init();

#endif // ANDROID_ENABLED

#endif // ANDROID_JNI_INIT_H
