#!/bin/bash

# Runs GUT tests against the web build in headless Chrome.
#
# Requires: a web build (./build.sh web), Godot with its web export templates installed,
# google-chrome (or CHROME=...), and python3. Tests that apply to the web run by default;
# integration and e2e tests are added when LIVEKIT_TEST_URL / RUN_E2E are set, as in test.sh.
#
# Usage: ./test_web.sh [res://test/path/to/test.gd ...]

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Unit tests for classes the web build implements.
WEB_UNIT_TESTS=(
    test_audio_source
    test_audio_stream
    test_connection_timeout
    test_disconnect_hang
    test_participant
    test_participant_properties
    test_platform
    test_room
    test_room_properties
    test_track
    test_track_publication
    test_video_source
    test_video_stream
)
# Tests that need a server. Their environment variables are passed through the page URL.
WEB_INTEGRATION_TESTS=(test_connection test_data_channel test_participants test_tracks)
WEB_E2E_TESTS=(test_audio_flow test_data_messaging test_full_session test_multi_participant test_track_lifecycle test_video_flow)
# Web-only e2e tests (see test/web/test_room_lifecycle.gd for why they don't run natively).
WEB_ONLY_E2E_TESTS=(test_room_lifecycle)
PASSTHROUGH_ENV=(LIVEKIT_TEST_URL LIVEKIT_TOKEN_1 LIVEKIT_TOKEN_2 RUN_E2E LIVEKIT_E2E_URL LIVEKIT_E2E_ROOM LIVEKIT_API_KEY LIVEKIT_API_SECRET)
TIMEOUT_SEC="${WEB_TEST_TIMEOUT:-300}"
PORT="${WEB_TEST_PORT:-8060}"
CHROME="${CHROME:-google-chrome}"
EXPORT_DIR="build/web-test"

if [ -f "./godot" ]; then
    GODOT_CMD="./godot"
else
    GODOT_CMD="${GODOT:-godot}"
fi

if [ ! -f addons/godot-livekit/bin/libgodot-livekit.web.wasm32.nothreads.wasm ]; then
    echo -e "${RED}Web build not found. Run ./build.sh web first.${NC}"
    exit 1
fi

if command -v timeout &> /dev/null; then
    TIMEOUT_CMD="timeout"
elif command -v gtimeout &> /dev/null; then
    TIMEOUT_CMD="gtimeout"
else
    echo -e "${RED}timeout not found (on macOS: brew install coreutils).${NC}"
    exit 1
fi

tests=("$@")
if [ ${#tests[@]} -eq 0 ]; then
    for t in "${WEB_UNIT_TESTS[@]}"; do tests+=("res://test/unit/$t.gd"); done
    if [ -n "${LIVEKIT_TEST_URL:-}" ]; then
        for t in "${WEB_INTEGRATION_TESTS[@]}"; do tests+=("res://test/integration/$t.gd"); done
    fi
    if [ -n "${RUN_E2E:-}" ]; then
        for t in "${WEB_E2E_TESTS[@]}"; do tests+=("res://test/e2e/$t.gd"); done
        for t in "${WEB_ONLY_E2E_TESTS[@]}"; do tests+=("res://test/web/$t.gd"); done
    fi
fi

WORK_DIR=$(mktemp -d)
LOG_FILE="$WORK_DIR/chrome.log"
SERVER_PID=""
CHROME_PID=""
HAD_PRESETS=false
WROTE_PRESETS=false
cleanup() {
    [ -n "$SERVER_PID" ] && kill "$SERVER_PID" 2>/dev/null || true
    if [ -n "$CHROME_PID" ]; then
        kill "$CHROME_PID" 2>/dev/null || true
        wait "$CHROME_PID" 2>/dev/null || true
    fi
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

if [ -f export_presets.cfg ]; then
    cp export_presets.cfg "$WORK_DIR/export_presets.cfg"
    HAD_PRESETS=true
fi
WROTE_PRESETS=true
cat > export_presets.cfg <<'EOF'
[preset.0]

name="Web Test"
platform="Web"
runnable=true
export_filter="all_resources"
include_filter="*.json,*.txt"
exclude_filter="build/*"
export_path=""
script_export_mode=0

[preset.0.options]

variant/extensions_support=true
variant/thread_support=false
vram_texture_compression/for_desktop=true
html/export_icon=false
progressive_web_app/enabled=false
EOF

echo -e "${YELLOW}Exporting web test build...${NC}"
# Cleared first so that a failed export can't leave an earlier build to be tested.
rm -rf "$EXPORT_DIR"
mkdir -p "$EXPORT_DIR"
# Importing fails harmlessly on errors the export reports too (e.g. a missing native library).
"$TIMEOUT_CMD" 120 $GODOT_CMD --headless --import >/dev/null 2>&1 || true
if ! "$TIMEOUT_CMD" 300 $GODOT_CMD --headless --export-release "Web Test" "$EXPORT_DIR/index.html" >"$WORK_DIR/export.log" 2>&1; then
    tail -20 "$WORK_DIR/export.log"
    echo -e "${RED}Web export failed. Are Godot's web export templates installed?${NC}"
    exit 1
fi
# GUT finds tests by path, so scripts are exported as text (script_export_mode=0) rather than
# tokenized. The project has no main scene; Godot runs a scene passed as an argument instead.
sed -i.bak 's|"args":\[\]|"args":["res://test/web/web_runner.tscn"]|' "$EXPORT_DIR/index.html"
rm -f "$EXPORT_DIR/index.html.bak"

query=$(PASSTHROUGH="${PASSTHROUGH_ENV[*]}" python3 - "${tests[@]}" <<'PY'
import os, sys, urllib.parse
params = {"tests": ",".join(sys.argv[1:])}
for key in os.environ.get("PASSTHROUGH", "").split():
    if os.environ.get(key):
        params[key] = os.environ[key]
print(urllib.parse.urlencode(params))
PY
)

python3 -m http.server "$PORT" --bind 127.0.0.1 --directory "$EXPORT_DIR" >/dev/null 2>&1 &
SERVER_PID=$!
sleep 1

echo -e "${YELLOW}Running ${#tests[@]} test script(s) in headless Chrome...${NC}"
# Software WebGL (Godot requires WebGL 2) and fake media devices; DISPLAY is unset so Chrome
# doesn't try to use an X server that may not be reachable.
env -u DISPLAY -u WAYLAND_DISPLAY "$CHROME" --headless=new --no-sandbox --user-data-dir="$WORK_DIR/profile" \
    --enable-logging=stderr --v=0 --ozone-platform=headless --use-angle=swiftshader --enable-unsafe-swiftshader \
    --autoplay-policy=no-user-gesture-required --use-fake-ui-for-media-stream --use-fake-device-for-media-stream \
    "http://127.0.0.1:$PORT/index.html?$query" >"$LOG_FILE" 2>&1 &
CHROME_PID=$!

deadline=$((SECONDS + TIMEOUT_SEC))
result=""
while [ $SECONDS -lt $deadline ]; do
    result=$(grep -ao 'GUT_WEB_RESULT[^"]*' "$LOG_FILE" | head -1 || true)
    [ -n "$result" ] && break
    if ! kill -0 "$CHROME_PID" 2>/dev/null; then
        break
    fi
    sleep 1
done

# Show the page's console output (GUT's report), without Chrome's own logging.
grep -a 'CONSOLE' "$LOG_FILE" | sed -E 's/^.*CONSOLE[^]]*\] "//; s/", source: .*$//' || true

if [ -z "$result" ]; then
    echo -e "${RED}Web tests did not finish within ${TIMEOUT_SEC}s.${NC}"
    exit 1
fi
failing=$(echo "$result" | sed -E 's/.*failing=([0-9]+).*/\1/')
ran=$(echo "$result" | sed -E 's/.*tests=([0-9]+).*/\1/')
echo -e "${BLUE}${result}${NC}"
if [ "$failing" != "0" ] || [ "$ran" == "0" ]; then
    echo -e "${RED}Web tests failed.${NC}"
    exit 1
fi
echo -e "${GREEN}Web tests passed!${NC}"
