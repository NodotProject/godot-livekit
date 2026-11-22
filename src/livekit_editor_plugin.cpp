#include "livekit_editor_plugin.h"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

// Relative to res://addons/, as _get_android_libraries() expects.
static const char *WEBRTC_JAR = "godot-livekit/bin/libwebrtc.jar";

String LiveKitAndroidExportPlugin::_get_name() const {
    return "GodotLiveKitAndroid";
}

bool LiveKitAndroidExportPlugin::_supports_platform(const Ref<EditorExportPlatform> &p_platform) const {
    return p_platform.is_valid() && p_platform->get_os_name() == "Android";
}

PackedStringArray LiveKitAndroidExportPlugin::_get_android_libraries(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const {
    PackedStringArray libraries;
    libraries.push_back(WEBRTC_JAR);
    return libraries;
}

void LiveKitAndroidExportPlugin::_export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) {
    if (!p_features.has("android")) {
        return;
    }
    // Godot only adds export plugins' Android libraries to Gradle builds, so without one the app would
    // be missing WebRTC's Java classes, and LiveKit would fail at runtime rather than here.
    if (!bool(get_option("gradle_build/use_gradle_build"))) {
        UtilityFunctions::push_error("godot-livekit: Android exports need a Gradle build, to add WebRTC's Java "
                                     "classes. Turn on Gradle Build > Use Gradle Build in the export preset.");
    }
    if (!FileAccess::file_exists(String("res://addons/") + WEBRTC_JAR)) {
        UtilityFunctions::push_error("godot-livekit: res://addons/", WEBRTC_JAR, " is missing. Reinstall the addon "
                                     "from a release, or build it with ./build.sh android.");
    }
}

void LiveKitEditorPlugin::_enter_tree() {
    android_export_plugin_.instantiate();
    add_export_plugin(android_export_plugin_);
}

void LiveKitEditorPlugin::_exit_tree() {
    remove_export_plugin(android_export_plugin_);
    android_export_plugin_.unref();
}
