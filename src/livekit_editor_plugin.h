#ifndef GODOT_LIVEKIT_EDITOR_PLUGIN_H
#define GODOT_LIVEKIT_EDITOR_PLUGIN_H

#include <godot_cpp/classes/editor_export_platform.hpp>
#include <godot_cpp/classes/editor_export_plugin.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>

namespace godot {

// Adds WebRTC's Java classes, which LiveKit's Android libraries call into, to Android exports.
class LiveKitAndroidExportPlugin : public EditorExportPlugin {
    GDCLASS(LiveKitAndroidExportPlugin, EditorExportPlugin)

protected:
    static void _bind_methods() {}

public:
    String _get_name() const override;
    bool _supports_platform(const Ref<EditorExportPlatform> &p_platform) const override;
    PackedStringArray _get_android_libraries(const Ref<EditorExportPlatform> &p_platform, bool p_debug) const override;
    void _export_begin(const PackedStringArray &p_features, bool p_is_debug, const String &p_path, uint32_t p_flags) override;
};

// Adds the export plugin in the editor. The extension registers it, so there's no plugin to enable.
class LiveKitEditorPlugin : public EditorPlugin {
    GDCLASS(LiveKitEditorPlugin, EditorPlugin)

    Ref<LiveKitAndroidExportPlugin> android_export_plugin_;

protected:
    static void _bind_methods() {}

public:
    void _enter_tree() override;
    void _exit_tree() override;
};

} // namespace godot

#endif // GODOT_LIVEKIT_EDITOR_PLUGIN_H
