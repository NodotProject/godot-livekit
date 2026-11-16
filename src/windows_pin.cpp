#include "windows_pin.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// LiveKit's runtime threads (behind subscriptions) can outlive livekit::shutdown(). When Godot
// unloads the extension at exit, Windows unmaps livekit_ffi.dll under them and the process
// crashes on its way out. Pinned modules are never unloaded, so those threads just end with the
// process. (Linux keeps such libraries mapped anyway.)
void livekit_pin_modules() {
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"livekit_ffi.dll", &module);
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, L"livekit.dll", &module);
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
            reinterpret_cast<LPCWSTR>(&livekit_pin_modules), &module);
}
