#!/bin/bash

# Runs GUT tests on an Android device or emulator.
#
# Requires: an Android build for the device's architecture (./build.sh android [arm64|x86_64]),
# Godot with its Android export templates installed, a device or emulator connected to adb, the
# Android SDK (ANDROID_HOME) and a JDK (JAVA_HOME) for Godot's Gradle build. Unit tests run by
# default; integration and e2e tests are added when LIVEKIT_TEST_URL / RUN_E2E are set, as in
# test.sh. On an emulator, the host's localhost is 10.0.2.2.
#
# Exporting and running can also be done separately, e.g. to export before starting an emulator,
# since both need a lot of CPU. The tests to run and the env vars are fixed when exporting.
#
# Usage: ./test_android.sh [--arch arm64|x86_64] [res://test/path/to/test.gd ...]
#        ./test_android.sh --export-only tests.apk --arch arm64|x86_64 [res://test/path/to/test.gd ...]
#        ./test_android.sh --apk tests.apk

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Android builds leave out screen capture, so these scripts don't compile there.
EXCLUDED_TESTS=(
    res://test/unit/test_screen_capture.gd
    res://test/unit/test_screen_capture_threading.gd
    res://test/e2e/test_screen_capture.gd
)
PASSTHROUGH_ENV=(LIVEKIT_TEST_URL LIVEKIT_TOKEN_1 LIVEKIT_TOKEN_2 RUN_E2E LIVEKIT_E2E_URL LIVEKIT_E2E_ROOM LIVEKIT_API_KEY LIVEKIT_API_SECRET)
PACKAGE="org.godotlivekit.tests"
# Godot's log in the app's user:// folder.
APP_LOG="test.log"
TIMEOUT_SEC="${ANDROID_TEST_TIMEOUT:-1800}"
# Homebrew's coreutils names it gtimeout on macOS.
TIMEOUT_CMD="timeout"
if ! command -v timeout &>/dev/null && command -v gtimeout &>/dev/null; then
    TIMEOUT_CMD="gtimeout"
fi

if [ -f "./godot" ]; then
    GODOT_CMD="./godot"
else
    GODOT_CMD="${GODOT:-godot}"
fi
if command -v adb &> /dev/null; then
    ADB="adb"
else
    ADB="${ANDROID_HOME:-}/platform-tools/adb"
fi

arch=""
export_only=""
apk=""
tests=()
while [ $# -gt 0 ]; do
    case "$1" in
        --arch) arch="$2"; shift 2 ;;
        --export-only) export_only="$2"; shift 2 ;;
        --apk) apk="$2"; shift 2 ;;
        *) tests+=("$1"); shift ;;
    esac
done
if [ -n "$apk" ] && { [ -n "$export_only" ] || [ ${#tests[@]} -gt 0 ]; }; then
    echo -e "${RED}--apk runs an exported build as is, so it can't be combined with --export-only or tests.${NC}"
    exit 1
fi

if [ -z "$apk" ] && [ -z "$arch" ]; then
    if ! abi=$("$ADB" shell getprop ro.product.cpu.abi 2>/dev/null | tr -d '\r'); then
        echo -e "${RED}No Android device found. Start an emulator or connect a device, or pass --arch.${NC}"
        exit 1
    fi
    case "$abi" in
        arm64-v8a) arch="arm64" ;;
        x86_64) arch="x86_64" ;;
        *)
            echo -e "${RED}Unsupported device ABI: ${abi}${NC}"
            exit 1
            ;;
    esac
fi

WORK_DIR=$(mktemp -d)
LOG_FILE="$WORK_DIR/test.log"
STARTED_APP=false
HAD_PRESETS=false
WROTE_PRESETS=false
cleanup() {
    [ "$STARTED_APP" = true ] && "$ADB" shell am force-stop "$PACKAGE" >/dev/null 2>&1 || true
    # The test preset temporarily replaces the developer's own export presets. (Nothing to undo if
    # it was never written, e.g. because backing up the original failed.)
    if [ "$WROTE_PRESETS" = true ]; then
        if [ "$HAD_PRESETS" = true ]; then
            mv -f "$WORK_DIR/export_presets.cfg" export_presets.cfg
        else
            rm -f export_presets.cfg
        fi
    fi
    rm -rf "$WORK_DIR"
}
trap cleanup EXIT
trap 'exit 130' INT TERM

## Exports the test app to the given path.
export_apk() {
    local apk_path="$1"
    if [ ! -f "addons/godot-livekit/bin/libgodot-livekit.android.${arch}.so" ]; then
        echo -e "${RED}Android ${arch} build not found. Run ./build.sh android ${arch} first.${NC}"
        exit 1
    fi
    if [ ! -f livekit-sdk/java/libwebrtc.jar ]; then
        echo -e "${RED}livekit-sdk/java/libwebrtc.jar not found. Rebuild with ./build.sh android ${arch}.${NC}"
        exit 1
    fi

    godot_version=$($GODOT_CMD --version | sed -E 's/^([0-9]+\.[0-9]+(\.[0-9]+)?\.[a-z0-9_]+).*/\1/')
    templates_dir="${GODOT_TEMPLATES_DIR:-${XDG_DATA_HOME:-$HOME/.local/share}/godot/export_templates/${godot_version}}"
    if [ ! -f "$templates_dir/android_source.zip" ]; then
        echo -e "${RED}Godot's Android export templates for ${godot_version} weren't found in ${templates_dir}.${NC}"
        exit 1
    fi

    if [ ${#tests[@]} -eq 0 ]; then
        dirs=(test/unit)
        [ -n "${LIVEKIT_TEST_URL:-}" ] && dirs+=(test/integration)
        [ -n "${RUN_E2E:-}" ] && dirs+=(test/e2e)
        for path in $(find "${dirs[@]}" -maxdepth 1 -name 'test_*.gd' | sort); do
            t="res://$path"
            [[ " ${EXCLUDED_TESTS[*]} " == *" $t "* ]] || tests+=("$t")
        done
    fi

    # Exporting with Gradle needs Godot's Android build template in the project. The plugin doesn't add
    # WebRTC's Java classes to exports yet, so the test build adds them to the template.
    if [ "$(cat android/.build_version 2>/dev/null)" != "$godot_version" ]; then
        # A template this script didn't install may have the project's own changes, so leave it alone.
        if [ -d android ] && [ ! -f android/.livekit_test_template ]; then
            echo -e "${RED}android/ has an Android build template this script didn't install. Move it aside to run the tests.${NC}"
            exit 1
        fi
        echo -e "${YELLOW}Installing Godot's Android build template...${NC}"
        rm -rf android
        mkdir -p android/build
        unzip -q "$templates_dir/android_source.zip" -d android/build
        echo "$godot_version" > android/.build_version
        touch android/.livekit_test_template
        touch android/build/.gdignore
    fi
    mkdir -p android/build/libs/debug
    cp livekit-sdk/java/libwebrtc.jar android/build/libs/debug/

    # The keystore that signs debug exports. Godot creates one for the editor, but not when headless.
    if [ -z "${GODOT_ANDROID_KEYSTORE_DEBUG_PATH:-}" ]; then
        export GODOT_ANDROID_KEYSTORE_DEBUG_PATH="$WORK_DIR/debug.keystore"
        export GODOT_ANDROID_KEYSTORE_DEBUG_USER="androiddebugkey"
        export GODOT_ANDROID_KEYSTORE_DEBUG_PASSWORD="android"
        keytool -genkeypair -keystore "$GODOT_ANDROID_KEYSTORE_DEBUG_PATH" -alias androiddebugkey \
            -storepass android -keypass android -keyalg RSA -validity 365 -dname "CN=Android Debug,O=Android,C=US" \
            >/dev/null 2>&1
    fi

    # Android has no command line, so the tests and env vars become user arguments in the export.
    # The emulator's Vulkan driver crashes Godot, so tests render with OpenGL.
    user_args="tests=$(IFS=,; echo "${tests[*]}")"
    for key in "${PASSTHROUGH_ENV[@]}"; do
        [ -n "${!key:-}" ] && user_args="$user_args $key=${!key}"
    done
    extra_args="res://test/mobile/mobile_runner.tscn --rendering-method gl_compatibility --rendering-driver opengl3 --log-file user://$APP_LOG ++ $user_args"

    if [ -f export_presets.cfg ]; then
        cp export_presets.cfg "$WORK_DIR/export_presets.cfg"
        HAD_PRESETS=true
    fi
    WROTE_PRESETS=true
    cat > export_presets.cfg <<EOF
[preset.0]

name="Android Test"
platform="Android"
runnable=true
export_filter="all_resources"
include_filter="*.json,*.txt"
exclude_filter="build/*"
export_path=""
script_export_mode=0

[preset.0.options]

gradle_build/use_gradle_build=true
gradle_build/export_format=0
architectures/armeabi-v7a=false
architectures/arm64-v8a=$([ "$arch" = arm64 ] && echo true || echo false)
architectures/x86=false
architectures/x86_64=$([ "$arch" = x86_64 ] && echo true || echo false)
package/unique_name="$PACKAGE"
package/name="LiveKit Tests"
command_line/extra_args="$extra_args"
permissions/access_network_state=true
permissions/internet=true
permissions/modify_audio_settings=true
permissions/record_audio=true
EOF

    echo -e "${YELLOW}Exporting Android test build (${arch})...${NC}"
    # Importing fails harmlessly on errors the export reports too (e.g. a missing native library).
    $TIMEOUT_CMD 300 $GODOT_CMD --headless --import >/dev/null 2>&1 || true
    rm -f "$apk_path"
    mkdir -p "$(dirname "$apk_path")"
    if ! $TIMEOUT_CMD 1200 $GODOT_CMD --headless --export-debug "Android Test" "$apk_path" >"$WORK_DIR/export.log" 2>&1 \
        || [ ! -f "$apk_path" ]; then
        tail -40 "$WORK_DIR/export.log"
        echo -e "${RED}Android export failed.${NC}"
        exit 1
    fi
    # Gradle leaves a daemon running, which would compete with an emulator for memory.
    (cd android/build && ./gradlew --stop >/dev/null 2>&1) || true
    echo -e "${GREEN}Exported ${#tests[@]} test script(s) to ${apk_path}${NC}"
}

if [ -z "$apk" ]; then
    if [ -n "$export_only" ]; then
        export_apk "$export_only"
        exit 0
    fi
    apk="$WORK_DIR/tests.apk"
    export_apk "$apk"
fi

# Launches the app and waits for it to exit, then reads its log. Sets app_exited and result.
run_app() {
    "$ADB" shell run-as "$PACKAGE" rm -f "files/$APP_LOG" >/dev/null 2>&1 || true
    "$ADB" shell am start -W -n "$PACKAGE/com.godot.game.GodotApp" >/dev/null
    # Emulators can drop off adb for a moment, so a failed adb command doesn't mean the app has
    # stopped. Only adb answering that the process is gone does.
    app_exited=false
    while [ $SECONDS -lt $deadline ]; do
        sleep 5
        running=$("$ADB" shell "pidof $PACKAGE >/dev/null && echo yes || echo no" 2>/dev/null | tr -d '\r' || true)
        if [ "$running" = "no" ]; then
            app_exited=true
            break
        fi
    done
    # The app writes Godot's log to a file (see extra_args), which survives adb dropping out.
    : >"$LOG_FILE"
    for _ in 1 2 3 4 5; do
        "$ADB" shell run-as "$PACKAGE" cat "files/$APP_LOG" >"$LOG_FILE" 2>/dev/null && [ -s "$LOG_FILE" ] && break
        sleep 2
    done
    cat "$LOG_FILE"
    result=$(grep -ao 'GUT_MOBILE_RESULT.*' "$LOG_FILE" | head -1 || true)
}

# Prints what Android logged about the app stopping (a crash, the system killing it, or its activity
# going away), and Godot's own output, which reaches logcat even when the log file doesn't.
print_stop_diagnostics() {
    echo "App exited: $app_exited"
    "$ADB" logcat -d -v tag AndroidRuntime:E DEBUG:V libc:F ActivityManager:I ActivityTaskManager:I lmkd:I \
        Godot:V GodotActivity:V godot:V '*:S' 2>/dev/null | tail -300 || true
}

# Prints every thread's stack, to show where an app that's still running is stuck. debuggerd needs
# root, which emulator images (but not most devices) allow.
print_app_stacks() {
    local pid
    if ! "$ADB" root >/dev/null 2>&1; then
        echo "Can't print the app's stacks without root."
        return
    fi
    "$ADB" wait-for-device
    pid=$("$ADB" shell pidof "$PACKAGE" 2>/dev/null | tr -d '\r' || true)
    [ -n "$pid" ] && "$ADB" shell debuggerd -b "$pid" 2>&1 || true
    "$ADB" unroot >/dev/null 2>&1 || true
}

echo -e "${YELLOW}Running the tests...${NC}"
# A fresh install, since each export may be signed with a new key. -g grants the microphone
# permission.
$TIMEOUT_CMD 300 "$ADB" wait-for-device
"$ADB" uninstall "$PACKAGE" >/dev/null 2>&1 || true
"$ADB" install -g "$apk" >/dev/null
# Includes the crash buffer, so only this run's crashes fail it.
"$ADB" logcat -b main,system,crash -c
STARTED_APP=true
deadline=$((SECONDS + TIMEOUT_SEC))
# On CI's emulator, the app occasionally exits before Godot finishes starting, so before any of the
# plugin's tests run. That's retried, while a crash or a failure once the tests start isn't.
for attempt in 1 2 3; do
    exited_early=false
    run_app
    crash_log=$("$ADB" logcat -d -b crash -v tag 2>/dev/null | tr -d '\r' || true)
    if [ -n "$result" ] || [ "$app_exited" != true ] || grep -aq 'GUT_MOBILE_STARTED' "$LOG_FILE" \
        || echo "$crash_log" | grep -q "$PACKAGE"; then
        break
    fi
    exited_early=true
    print_stop_diagnostics
    echo -e "${YELLOW}The app exited before the tests started (attempt ${attempt}).${NC}"
done
if [ -z "$result" ] && [ "$exited_early" != true ]; then
    print_stop_diagnostics
fi
if [ "$app_exited" != true ]; then
    print_app_stacks
fi

if [ -z "$result" ]; then
    echo -e "${RED}Android tests did not finish (the app stopped early or took over ${TIMEOUT_SEC}s).${NC}"
    exit 1
fi
# The runner reports before quitting, so a hang or crash while shutting down would otherwise go unnoticed.
if [ "$app_exited" != true ]; then
    echo -e "${RED}The app didn't exit within ${TIMEOUT_SEC}s after running the tests.${NC}"
    exit 1
fi
crash_log=$("$ADB" logcat -d -b crash -v tag 2>/dev/null | tr -d '\r' || true)
if echo "$crash_log" | grep -q "$PACKAGE"; then
    echo "$crash_log"
    echo -e "${RED}The app crashed after running the tests.${NC}"
    exit 1
fi
failing=$(echo "$result" | sed -E 's/.*failing=([0-9]+).*/\1/')
ran=$(echo "$result" | sed -E 's/.*tests=([0-9]+).*/\1/')
echo -e "${BLUE}${result}${NC}"
# GUT skips scripts that fail to compile, rather than failing them.
if grep -q 'SCRIPT ERROR: Parse Error' "$LOG_FILE"; then
    echo -e "${RED}Some test scripts failed to compile.${NC}"
    exit 1
fi
if [ "$failing" != "0" ] || [ "$ran" == "0" ]; then
    echo -e "${RED}Android tests failed.${NC}"
    exit 1
fi
echo -e "${GREEN}Android tests passed!${NC}"
