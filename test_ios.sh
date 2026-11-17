#!/bin/bash

# Runs GUT tests in the iOS simulator, or checks that an export builds for iOS devices.
#
# Requires: macOS with Xcode and an iOS simulator runtime, a simulator build
# (./build.sh ios --simulator), and Godot with its iOS export templates installed. Unit tests run by
# default; integration and e2e tests are added when LIVEKIT_TEST_URL / RUN_E2E are set, as in
# test.sh. The simulator shares the host's network, so a local server is at 127.0.0.1.
#
# Godot's official templates have no arm64 simulator code (godotengine/godot#122379), so set
# GODOT_IOS_SIMULATOR_LIB to a libgodot.a built with
# `scons platform=ios target=template_debug arch=arm64 ios_simulator=yes`.
#
# --build-for-device exports the project with a device build (./build.sh ios) and builds it, unsigned,
# for iOS devices instead, then checks that the app bundles every framework its code links.
#
# Usage: ./test_ios.sh [--build-for-device] [res://test/path/to/test.gd ...]

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# iOS builds leave out screen capture, so these scripts don't compile there.
EXCLUDED_TESTS=(
    res://test/unit/test_screen_capture.gd
    res://test/unit/test_screen_capture_threading.gd
    res://test/e2e/test_screen_capture.gd
)
PASSTHROUGH_ENV=(LIVEKIT_TEST_URL LIVEKIT_TOKEN_1 LIVEKIT_TOKEN_2 RUN_E2E LIVEKIT_E2E_URL LIVEKIT_E2E_ROOM LIVEKIT_API_KEY LIVEKIT_API_SECRET)
BUNDLE_ID="org.godotlivekit.tests"
APP_NAME="LiveKitTests"
# Godot's log in the app's user:// folder, which is its Documents folder on iOS.
APP_LOG="test.log"
TIMEOUT_SEC="${IOS_TEST_TIMEOUT:-1800}"
# Homebrew's coreutils names it gtimeout.
TIMEOUT_CMD="timeout"
if ! command -v timeout &>/dev/null && command -v gtimeout &>/dev/null; then
    TIMEOUT_CMD="gtimeout"
fi
GODOT_CMD="${GODOT:-godot}"

BUILD_FOR_DEVICE=false
tests=()
for arg in "$@"; do
    case "$arg" in
        --build-for-device) BUILD_FOR_DEVICE=true ;;
        *) tests+=("$arg") ;;
    esac
done

slice="ios-arm64-simulator"
build_cmd="./build.sh ios --simulator"
if [ "$BUILD_FOR_DEVICE" = true ]; then
    slice="ios-arm64"
    build_cmd="./build.sh ios"
fi
if [ ! -d "addons/godot-livekit/bin/libgodot-livekit.ios.xcframework/${slice}" ]; then
    echo -e "${RED}iOS build (${slice}) not found. Run ${build_cmd} first.${NC}"
    exit 1
fi

godot_version=$($GODOT_CMD --version | sed -E 's/^([0-9]+\.[0-9]+(\.[0-9]+)?\.[a-z0-9_]+).*/\1/')
templates_dir="${GODOT_TEMPLATES_DIR:-$HOME/Library/Application Support/Godot/export_templates/${godot_version}}"
if [ ! -f "$templates_dir/ios.zip" ]; then
    echo -e "${RED}Godot's iOS export templates for ${godot_version} weren't found in ${templates_dir}.${NC}"
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

WORK_DIR=$(mktemp -d)
DEVICE=""
CREATED_DEVICE=false
HAD_PRESETS=false
WROTE_PRESETS=false
cleanup() {
    if [ -n "$DEVICE" ]; then
        xcrun simctl terminate "$DEVICE" "$BUNDLE_ID" >/dev/null 2>&1 || true
        if [ "$CREATED_DEVICE" = true ]; then
            xcrun simctl shutdown "$DEVICE" >/dev/null 2>&1 || true
            xcrun simctl delete "$DEVICE" >/dev/null 2>&1 || true
        fi
    fi
    # The test preset temporarily replaces the developer's own export presets.
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

# A framework the app links but doesn't bundle only shows up as a crash at launch on a device.
check_bundled_frameworks() {
    local app="$1" missing=() binaries=("$1/$APP_NAME")
    for framework in "$app"/Frameworks/*.framework; do
        [ -d "$framework" ] && binaries+=("$framework/$(basename "$framework" .framework)")
    done
    for binary in "${binaries[@]}"; do
        echo "$(basename "$binary"):"
        while read -r dep; do
            echo "    $dep"
            [ -e "$app/Frameworks/${dep#@rpath/}" ] || missing+=("$(basename "$binary") -> $dep")
        done < <(otool -L "$binary" | awk 'NR > 1 && $1 ~ /^@rpath\// {print $1}')
    done
    if [ ${#missing[@]} -gt 0 ]; then
        printf '  %s\n' "${missing[@]}"
        echo -e "${RED}The app doesn't bundle frameworks that its code links.${NC}"
        exit 1
    fi
}

# The simulator's background work after booting can take most of the host's CPU, enough to time
# out e2e tests, so wait (within a limit) for it to settle before running them.
wait_for_idle_cpu() {
    local deadline=$((SECONDS + ${IOS_SETTLE_TIMEOUT_SEC:-300})) idle
    while [ $SECONDS -lt $deadline ]; do
        # top's first sample is an average since boot, so read its second.
        idle=$(top -l 2 -s 5 -n 0 | awk '/^CPU usage/ {v = $(NF - 1)} END {sub(/%/, "", v); print int(v)}')
        [ "${idle:-0}" -ge 50 ] && return 0
        echo "Waiting for the host's CPU to settle (${idle}% idle)..."
    done
    echo -e "${YELLOW}The host's CPU is still busy. Running the tests anyway.${NC}"
}

# A newly booted simulator spends a minute or two on background work, so boot it before exporting.
if [ "$BUILD_FOR_DEVICE" = false ]; then
    DEVICE="${IOS_SIMULATOR_UDID:-}"
    if [ -z "$DEVICE" ]; then
        read -r runtime device_type < <(xcrun simctl list runtimes -j | python3 -c 'import json,sys
r = [x for x in json.load(sys.stdin)["runtimes"] if x["isAvailable"] and x["platform"] == "iOS"]
t = [d["identifier"] for d in r[-1]["supportedDeviceTypes"] if d["name"].startswith("iPhone")] if r else []
print(r[-1]["identifier"], t[-1]) if t else print()') || true
        if [ -z "${runtime:-}" ] || [ -z "${device_type:-}" ]; then
            echo -e "${RED}No iOS simulator runtime found. Install one with Xcode.${NC}"
            exit 1
        fi
        DEVICE=$(xcrun simctl create "LiveKit Tests" "$device_type" "$runtime")
        CREATED_DEVICE=true
    fi
    xcrun simctl boot "$DEVICE" >/dev/null 2>&1 || true
fi

if [ -f export_presets.cfg ]; then
    cp export_presets.cfg "$WORK_DIR/export_presets.cfg"
    HAD_PRESETS=true
fi
WROTE_PRESETS=true
# Godot needs a team ID and an app icon to write the Xcode project. The simulator build is signed
# ad hoc instead, and the project sets no icon of its own.
cat > export_presets.cfg <<EOF
[preset.0]

name="iOS Test"
platform="iOS"
runnable=true
export_filter="all_resources"
include_filter="*.json,*.txt"
exclude_filter="build/*"
export_path=""
script_export_mode=0

[preset.0.options]

application/app_store_team_id="AAAAAAAAAA"
application/bundle_identifier="$BUNDLE_ID"
application/export_project_only=true
application/min_ios_version="14.0"
icons/icon_1024x1024="res://icon.png"
privacy/microphone_usage_description="The tests capture and play audio."
EOF

echo -e "${YELLOW}Exporting the iOS test project...${NC}"
# Importing fails harmlessly on errors the export reports too (e.g. a missing native library).
$TIMEOUT_CMD 300 $GODOT_CMD --headless --import >/dev/null 2>&1 || true
mkdir -p "$WORK_DIR/xcode"
if ! $TIMEOUT_CMD 1200 $GODOT_CMD --headless --export-debug "iOS Test" "$WORK_DIR/xcode/${APP_NAME}.ipa" \
    >"$WORK_DIR/export.log" 2>&1 || [ ! -d "$WORK_DIR/xcode/${APP_NAME}.xcodeproj" ]; then
    tail -60 "$WORK_DIR/export.log"
    echo -e "${RED}iOS export failed.${NC}"
    exit 1
fi

if [ "$BUILD_FOR_DEVICE" = true ]; then
    echo -e "${YELLOW}Building the app for iOS devices...${NC}"
    if ! xcodebuild -project "$WORK_DIR/xcode/${APP_NAME}.xcodeproj" -scheme "$APP_NAME" -configuration Debug \
        -sdk iphoneos -destination "generic/platform=iOS" -derivedDataPath "$WORK_DIR/derived" \
        CODE_SIGNING_ALLOWED=NO build >"$WORK_DIR/xcodebuild.log" 2>&1; then
        grep -E "error:|warning: .*(dylib|framework)" "$WORK_DIR/xcodebuild.log" | head -40 || true
        tail -40 "$WORK_DIR/xcodebuild.log"
        echo -e "${RED}Building the app for iOS devices failed.${NC}"
        exit 1
    fi
    check_bundled_frameworks "$WORK_DIR/derived/Build/Products/Debug-iphoneos/${APP_NAME}.app"
    echo -e "${GREEN}The export builds for iOS devices.${NC}"
    exit 0
fi

sim_lib=$(find "$WORK_DIR/xcode/${APP_NAME}.xcframework" -path '*simulator*' -name libgodot.a | head -1)
if [ -n "${GODOT_IOS_SIMULATOR_LIB:-}" ]; then
    cp "$GODOT_IOS_SIMULATOR_LIB" "$sim_lib"
elif ! lipo -archs "$sim_lib" | grep -qw arm64; then
    echo -e "${RED}Godot's simulator library has no arm64 code. Set GODOT_IOS_SIMULATOR_LIB, as described at the top of this script.${NC}"
    exit 1
fi

echo -e "${YELLOW}Building the test app for the simulator...${NC}"
# Our libraries' simulator slices are arm64 only.
if ! xcodebuild -project "$WORK_DIR/xcode/${APP_NAME}.xcodeproj" -scheme "$APP_NAME" -configuration Debug \
    -sdk iphonesimulator -destination "generic/platform=iOS Simulator" -derivedDataPath "$WORK_DIR/derived" \
    ARCHS=arm64 ONLY_ACTIVE_ARCH=NO CODE_SIGN_IDENTITY=- CODE_SIGN_STYLE=Manual DEVELOPMENT_TEAM= \
    PROVISIONING_PROFILE_SPECIFIER= build >"$WORK_DIR/xcodebuild.log" 2>&1; then
    grep -E "error:|warning: .*(dylib|framework)" "$WORK_DIR/xcodebuild.log" | head -40 || true
    tail -40 "$WORK_DIR/xcodebuild.log"
    echo -e "${RED}Building the test app failed.${NC}"
    exit 1
fi
app="$WORK_DIR/derived/Build/Products/Debug-iphonesimulator/${APP_NAME}.app"

$TIMEOUT_CMD 600 xcrun simctl bootstatus "$DEVICE" -b >/dev/null
wait_for_idle_cpu

xcrun simctl install "$DEVICE" "$app"
xcrun simctl privacy "$DEVICE" grant microphone "$BUNDLE_ID" || true

args=(res://test/mobile/mobile_runner.tscn --log-file "user://$APP_LOG" ++ "tests=$(IFS=,; echo "${tests[*]}")")
for key in "${PASSTHROUGH_ENV[@]}"; do
    [ -n "${!key:-}" ] && args+=("$key=${!key}")
done

echo -e "${YELLOW}Running the tests...${NC}"
container=$(xcrun simctl get_app_container "$DEVICE" "$BUNDLE_ID" data)
app_logs=("$container/Documents/logs/$APP_LOG" "$container/Documents/$APP_LOG")
xcrun simctl launch --console "$DEVICE" "$BUNDLE_ID" "${args[@]}" >"$WORK_DIR/console.log" 2>&1 &
launch_pid=$!
# Godot on iOS ignores quit requests, so the app is stopped once the runner reports its result.
deadline=$((SECONDS + TIMEOUT_SEC))
while [ $SECONDS -lt $deadline ]; do
    grep -qa 'GUT_MOBILE_RESULT' "$WORK_DIR/console.log" "${app_logs[@]}" 2>/dev/null && break
    kill -0 "$launch_pid" 2>/dev/null || break
    sleep 5
done
xcrun simctl terminate "$DEVICE" "$BUNDLE_ID" >/dev/null 2>&1 || true
wait "$launch_pid" 2>/dev/null || true
LOG_FILE="$WORK_DIR/console.log"
for log in "${app_logs[@]}"; do
    if [ -f "$log" ]; then
        LOG_FILE="$log"
        break
    fi
done
cat "$LOG_FILE"
result=$(grep -aho 'GUT_MOBILE_RESULT.*' "$LOG_FILE" "$WORK_DIR/console.log" | head -1 || true)

if [ -z "$result" ]; then
    echo "--- Console ---"
    tail -100 "$WORK_DIR/console.log"
    echo "--- Crash reports ---"
    find "$HOME/Library/Logs/DiagnosticReports" -name "${APP_NAME}*" -newer "$WORK_DIR/export.log" \
        -exec head -120 {} \; 2>/dev/null || true
    echo -e "${RED}iOS tests did not finish (the app stopped early or took over ${TIMEOUT_SEC}s).${NC}"
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
    echo -e "${RED}iOS tests failed.${NC}"
    exit 1
fi
echo -e "${GREEN}iOS tests passed!${NC}"
