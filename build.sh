#!/bin/bash

# Enhanced build script for Godot-LiveKit (livekit GDExtension)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# --- Configuration ---
PLATFORM=""
ARCH=""
SCONS_FLAGS=""
LIVEKIT_VERSION="1.13.1"
# Where the LiveKit SDK goes. iOS has one per slice (device or simulator).
LIVEKIT_SDK_DIR="livekit-sdk"
# LiveKit publishes no Android or iOS build of the C++ SDK, so those build it from the same release with this patch.
LIVEKIT_MOBILE_PATCH="patches/client-sdk-cpp-mobile.patch"
# Cross-compiling the SDK needs a protoc for the build machine that matches its vendored protobuf.
PROTOC_VERSION="25.3"
# protoc runs on the build machine, so there's one download per host.
PROTOC_SHA256_LINUX_X86_64="f853e691868d0557425ea290bf7ba6384eef2fa9b04c323afab49a770ba9da80"
PROTOC_SHA256_LINUX_AARCH64="9eae1f20f70cccc912d1c318c3929b86aebf5afd4b0f32c196ef682c222ed5ae"
PROTOC_SHA256_MACOS="c06b6c5527c9b11a30a9c103400ee59eb6882f1766ea786fe4bdbb7f55ee5e6f"
# webrtc-sys needs NDK r27's libc++.
MIN_ANDROID_NDK_MAJOR=27
# Godot 4.5's iOS export templates need iOS 14.
IOS_MIN_VERSION="14.0"
IOS_SIMULATOR=false
GODOT_CPP_VERSION="godot-4.5-stable"
FRAMETAP_VERSION="0.1.3"
LIVEKIT_JS_VERSION="2.24.0"
LIVEKIT_JS_SHA256="88b41eb0905a66dd3f46009c9a024aa72503b53398ae9a70c1f60258d246d262"
SKIP_FRAMETAP=false
SKIP_LIVEKIT_SDK=false
BUILD_LIVEKIT_SDK=false

# Verify a downloaded file's SHA-256 hash (if a hash is provided).
verify_checksum() {
    local file="$1"
    local expected_hash="$2"
    if [ -z "$expected_hash" ]; then
        return 0 # No hash to verify
    fi
    local actual_hash
    if command -v sha256sum &>/dev/null; then
        actual_hash=$(sha256sum "$file" | awk '{print $1}')
    elif command -v shasum &>/dev/null; then
        actual_hash=$(shasum -a 256 "$file" | awk '{print $1}')
    else
        echo -e "${YELLOW}Warning: no sha256sum or shasum found, skipping checksum verification${NC}"
        return 0
    fi
    if [ "$actual_hash" != "$expected_hash" ]; then
        echo -e "${RED}ERROR: Checksum mismatch for $file${NC}"
        echo -e "${RED}  Expected: $expected_hash${NC}"
        echo -e "${RED}  Actual:   $actual_hash${NC}"
        rm -f "$file"
        exit 1
    fi
    echo -e "${GREEN}Checksum verified for $file${NC}"
}

# --- Helper Functions ---
BUILD_TARGET="template_release"

show_usage() {
    echo -e "${YELLOW}Usage: $0 [linux|macos|windows|android|ios|web] [arm64|x86_64] [--simulator] [--debug]${NC}"
    echo "  linux: Build for Linux (x86_64)"
    echo "  macos [arm64|x86_64]: Build for macOS (defaults to host arch)"
    echo "  windows: Build for Windows (x86_64, cross-compile)"
    echo "  android [arm64|x86_64]: Build for Android (defaults to arm64, requires ANDROID_NDK_ROOT)"
    echo "  ios [--simulator]: Build for iOS devices or the iOS simulator (arm64, requires Xcode)"
    echo "  web: Build for the web (wasm32, single-threaded, requires emcc from emsdk on PATH)"
    echo "  --debug: Build debug variant (template_debug) instead of release"
    exit 1
}

# --- Platform-specific Setup ---
setup_linux() {
    PLATFORM="linux"
    ARCH="x86_64"
    SCONS_FLAGS="platform=linux arch=x86_64"
    LIVEKIT_ARCHIVE="livekit-sdk-ubuntu-22.04-x64-${LIVEKIT_VERSION}.tar.gz"
    LIVEKIT_URL="https://github.com/livekit/client-sdk-cpp/releases/download/v${LIVEKIT_VERSION}/${LIVEKIT_ARCHIVE}"
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (Linux) ===${NC}"
}

setup_macos() {
    PLATFORM="macos"
    ARCH="${1:-$(uname -m)}"
    # Normalize arch name
    case "$ARCH" in
        aarch64|arm64) ARCH="arm64" ;;
        x86_64|amd64)  ARCH="x86_64" ;;
        *)
            echo -e "${RED}Unsupported macOS architecture: $ARCH${NC}"
            exit 1
            ;;
    esac
    SCONS_FLAGS="platform=macos arch=${ARCH}"
    local sdk_arch="arm64"
    [ "$ARCH" = "x86_64" ] && sdk_arch="x64"
    LIVEKIT_ARCHIVE="livekit-sdk-macos-${sdk_arch}-${LIVEKIT_VERSION}.tar.gz"
    LIVEKIT_URL="https://github.com/livekit/client-sdk-cpp/releases/download/v${LIVEKIT_VERSION}/${LIVEKIT_ARCHIVE}"
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (macOS ${ARCH}) ===${NC}"
}

setup_windows() {
    PLATFORM="windows"
    ARCH="x86_64"
    SCONS_FLAGS="platform=windows arch=x86_64"
    LIVEKIT_ARCHIVE="livekit-sdk-windows-x64-${LIVEKIT_VERSION}.zip"
    LIVEKIT_URL="https://github.com/livekit/client-sdk-cpp/releases/download/v${LIVEKIT_VERSION}/${LIVEKIT_ARCHIVE}"
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (Windows Native) ===${NC}"
}

setup_android() {
    PLATFORM="android"
    ARCH="${1:-arm64}"
    # Its own folder, so that Android and desktop builds in one checkout don't replace each other's SDK.
    LIVEKIT_SDK_DIR="livekit-sdk-android-${ARCH}"
    SCONS_FLAGS="platform=android arch=${ARCH} livekit_sdk=${LIVEKIT_SDK_DIR} screen_capture=no"
    BUILD_LIVEKIT_SDK=true
    SKIP_FRAMETAP=true
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (Android ${ARCH}) ===${NC}"
}

setup_ios() {
    PLATFORM="ios"
    ARCH="arm64"
    local slice="ios-${ARCH}"
    [ "$IOS_SIMULATOR" = true ] && slice="${slice}-simulator"
    LIVEKIT_SDK_DIR="livekit-sdk-${slice}"
    SCONS_FLAGS="platform=ios arch=${ARCH} ios_simulator=$([ "$IOS_SIMULATOR" = true ] && echo yes || echo no) ios_min_version=${IOS_MIN_VERSION} livekit_sdk=${LIVEKIT_SDK_DIR} screen_capture=no"
    BUILD_LIVEKIT_SDK=true
    SKIP_FRAMETAP=true
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (${slice}) ===${NC}"
}

setup_web() {
    PLATFORM="web"
    ARCH="wasm32"
    # Godot's web export templates default to single-threaded builds since 4.3.
    SCONS_FLAGS="-f SConstruct.web platform=web threads=no"
    SKIP_FRAMETAP=true
    SKIP_LIVEKIT_SDK=true
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (Web) ===${NC}"
}

# --- Build Functions ---

# Function to check if godot-cpp cache is valid
check_godotcpp_cache() {
    echo -e "${YELLOW}Checking godot-cpp cache...${NC}"

    if [ ! -d "godot-cpp/bin" ] || [ ! -d "godot-cpp/include" ] || [ ! -d "godot-cpp/gen" ]; then
        echo -e "${RED}Cache miss: godot-cpp prebuilt not found${NC}"
        return 1
    fi

    # A build for another architecture or target doesn't have the library this build links.
    local gcpp_lib="godot-cpp/bin/*godot-cpp.${PLATFORM}.${BUILD_TARGET}*.${ARCH}.*"
    if [ "$PLATFORM" == "ios" ]; then
        # The device library's name is the simulator library's, minus .simulator.
        gcpp_lib="godot-cpp/bin/libgodot-cpp.ios.${BUILD_TARGET}.${ARCH}$([ "$IOS_SIMULATOR" = true ] && echo .simulator).a"
    fi
    if [ -n "$ARCH" ] && ! ls $gcpp_lib &> /dev/null; then
        echo -e "${RED}Cache miss: godot-cpp not built for ${PLATFORM} ${ARCH} (${BUILD_TARGET})${NC}"
        return 1
    fi

    # Verify version matches
    if [ -f "godot-cpp/.version" ] && [ "$(cat godot-cpp/.version)" = "$GODOT_CPP_VERSION" ]; then
        echo -e "${GREEN}godot-cpp cache is valid!${NC}"
        return 0
    fi

    echo -e "${RED}Cache miss: godot-cpp version mismatch${NC}"
    return 1
}

# Function to build godot-cpp from source.
# (Previously fetched a prebuilt from NodotProject/godot-cpp-builds, but the
# godot-4.5-stable release asset was re-uploaded 2026-05-28 with an empty bin/
# directory — no static libraries — which broke every fresh build. Building
# from the official godotengine/godot-cpp tag is deterministic; CI caches the
# result so the cost is paid once per platform.)
fetch_godotcpp() {
    echo -e "${YELLOW}Building godot-cpp from source (${GODOT_CPP_VERSION}, target=${BUILD_TARGET})...${NC}"

    # Each platform's library has its own name, so builds for several platforms can share a clone. But
    # the generated bindings depend on pointer size, so web (32-bit) builds can't share with the others.
    if [ ! -f godot-cpp/.version ] || [ "$(cat godot-cpp/.version)" != "$GODOT_CPP_VERSION" ] \
        || [ "$PLATFORM" == "web" ] || ls godot-cpp/bin/*.web.* &> /dev/null; then
        rm -rf godot-cpp
        git clone --depth 1 --branch "${GODOT_CPP_VERSION}" \
            https://github.com/godotengine/godot-cpp.git godot-cpp
    fi

    local gcpp_flags="platform=${PLATFORM} target=${BUILD_TARGET}"
    if [ -n "$ARCH" ]; then
        gcpp_flags="$gcpp_flags arch=${ARCH}"
    fi
    if [ "$PLATFORM" == "web" ]; then
        gcpp_flags="$gcpp_flags threads=no"
    fi
    if [ "$PLATFORM" == "ios" ]; then
        gcpp_flags="$gcpp_flags ios_min_version=${IOS_MIN_VERSION}"
        [ "$IOS_SIMULATOR" = true ] && gcpp_flags="$gcpp_flags ios_simulator=yes"
    fi
    if [ "$PLATFORM" == "android" ]; then
        export ANDROID_NDK_ROOT="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
        # godot-cpp resolves $ANDROID_HOME/ndk/<its pinned version> BEFORE the
        # ANDROID_NDK_ROOT fallback (and KeyErrors if ANDROID_HOME is entirely
        # unset). Force ANDROID_HOME empty on the scons command line so the
        # NDK that install_dependencies validated is the one actually used.
        gcpp_flags="$gcpp_flags ANDROID_HOME="
    fi
    (cd godot-cpp && scons $gcpp_flags)

    echo "$GODOT_CPP_VERSION" > godot-cpp/.version
    echo -e "${GREEN}godot-cpp built successfully!${NC}"
}

# Function to download livekit
fetch_livekit() {
    echo -e "${YELLOW}Fetching LiveKit C++ SDK for ${PLATFORM} (${ARCH})...${NC}"

    rm -rf ${LIVEKIT_SDK_DIR}
    mkdir -p ${LIVEKIT_SDK_DIR}
    
    echo -e "${YELLOW}Downloading ${LIVEKIT_URL}...${NC}"
    curl -sL "${LIVEKIT_URL}" -o "${LIVEKIT_ARCHIVE}"
    
    echo -e "${YELLOW}Extracting...${NC}"
    if [[ "$LIVEKIT_ARCHIVE" == *.zip ]]; then
        unzip -q "${LIVEKIT_ARCHIVE}" -d ${LIVEKIT_SDK_DIR}
        local inner_dir=$(ls livekit-sdk)
        if [ -d "${LIVEKIT_SDK_DIR}/${inner_dir}/include" ]; then
            mv ${LIVEKIT_SDK_DIR}/${inner_dir}/* ${LIVEKIT_SDK_DIR}/
            rm -rf ${LIVEKIT_SDK_DIR}/${inner_dir}
        fi
    else
        tar -xzf "${LIVEKIT_ARCHIVE}" -C ${LIVEKIT_SDK_DIR} --strip-components=1
    fi
    
    rm "${LIVEKIT_ARCHIVE}"

    # Patch: local_participant.h uses std::vector<ParticipantTrackPermission> in a default
    # parameter but only forward-declares the type. Include track.h for the full definition.
    local lp_header="${LIVEKIT_SDK_DIR}/include/livekit/local_participant.h"
    if [ -f "$lp_header" ] && ! grep -q '#include "livekit/track.h"' "$lp_header"; then
        echo -e "${YELLOW}Patching local_participant.h (incomplete type fix)...${NC}"
        sed -i.bak 's|#include "livekit/participant.h"|#include "livekit/participant.h"\
#include "livekit/track.h"|' "$lp_header"
        rm -f "${lp_header}.bak"

        # Verify the patch applied correctly
        if ! grep -q '#include "livekit/track.h"' "$lp_header"; then
            echo -e "${RED}ERROR: Failed to patch local_participant.h — the expected #include line was not found.${NC}"
            echo -e "${RED}The LiveKit SDK header format may have changed. Please patch manually.${NC}"
            exit 1
        fi
    fi

    livekit_version_key > ${LIVEKIT_SDK_DIR}/.version
    echo -e "${GREEN}LiveKit SDK downloaded and extracted successfully!${NC}"
}

# Function to build the LiveKit C++ SDK for Android or iOS from its release, with our patch
build_livekit_sdk() {
    echo -e "${YELLOW}Building LiveKit C++ SDK ${LIVEKIT_VERSION} for ${PLATFORM} (${ARCH})...${NC}"
    local work_dir="$(pwd)/livekit-sdk-build"
    local src_dir="${work_dir}/client-sdk-cpp"
    local rust_target
    local platform_args=()
    if [ "$PLATFORM" == "android" ]; then
        local ndk_root="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
        local android_abi="arm64-v8a"
        rust_target="aarch64-linux-android"
        if [ "$ARCH" = "x86_64" ]; then
            android_abi="x86_64"
            rust_target="x86_64-linux-android"
        fi
        # The SDK reports errors by throwing, and the extension can only catch them if both share one C++ runtime.
        # With the NDK's default static runtime, each library gets its own exception types, and the app aborts instead.
        platform_args=(
            -DCMAKE_TOOLCHAIN_FILE="${ndk_root}/build/cmake/android.toolchain.cmake"
            -DANDROID_ABI="$android_abi"
            -DANDROID_PLATFORM=android-24
            -DANDROID_STL=c++_shared
        )
    else
        local ios_sdk="iphoneos"
        rust_target="aarch64-apple-ios"
        if [ "$IOS_SIMULATOR" = true ]; then
            ios_sdk="iphonesimulator"
            rust_target="aarch64-apple-ios-sim"
        fi
        platform_args=(
            -DCMAKE_SYSTEM_NAME=iOS
            # Cross-compiling leaves it unset, and abseil then picks x86 compiler flags.
            -DCMAKE_SYSTEM_PROCESSOR="$ARCH"
            -DCMAKE_OSX_SYSROOT="$ios_sdk"
            -DCMAKE_OSX_ARCHITECTURES="$ARCH"
            -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_MIN_VERSION"
        )
        # Cargo builds the FFI, and Rust takes the minimum iOS version from the environment.
        export IPHONEOS_DEPLOYMENT_TARGET="$IOS_MIN_VERSION"
    fi

    rm -rf "${LIVEKIT_SDK_DIR}" "$work_dir"
    mkdir -p "$work_dir"
    git clone --quiet --depth 1 --branch "v${LIVEKIT_VERSION}" --recurse-submodules --shallow-submodules \
        https://github.com/livekit/client-sdk-cpp.git "$src_dir"
    git -C "$src_dir" apply "$(pwd)/${LIVEKIT_MOBILE_PATCH}"

    local protoc_host protoc_sha256
    case "$(uname -s)-$(uname -m)" in
        Linux-x86_64) protoc_host="linux-x86_64"; protoc_sha256="$PROTOC_SHA256_LINUX_X86_64" ;;
        Linux-aarch64 | Linux-arm64) protoc_host="linux-aarch_64"; protoc_sha256="$PROTOC_SHA256_LINUX_AARCH64" ;;
        Darwin-*) protoc_host="osx-universal_binary"; protoc_sha256="$PROTOC_SHA256_MACOS" ;;
        *)
            echo -e "${RED}Building the LiveKit SDK for ${PLATFORM} isn't supported on $(uname -s) $(uname -m)${NC}"
            exit 1
            ;;
    esac
    local protoc_zip="${work_dir}/protoc.zip"
    curl -sL "https://github.com/protocolbuffers/protobuf/releases/download/v${PROTOC_VERSION}/protoc-${PROTOC_VERSION}-${protoc_host}.zip" \
        -o "$protoc_zip"
    verify_checksum "$protoc_zip" "$protoc_sha256"
    unzip -q "$protoc_zip" -d "${work_dir}/protoc"

    # The SDK pins its Rust release in client-sdk-rust, so add the target to that toolchain.
    (cd "${src_dir}/client-sdk-rust" && rustup target add "$rust_target")

    cmake -S "$src_dir" -B "${work_dir}/build" -G Ninja \
        "${platform_args[@]}" \
        -DCMAKE_BUILD_TYPE=Release \
        -DLIVEKIT_VERSION="$LIVEKIT_VERSION" \
        -DProtobuf_PROTOC_EXECUTABLE="${work_dir}/protoc/bin/protoc"
    cmake --build "${work_dir}/build" -j "$(getconf _NPROCESSORS_ONLN)"
    cmake --install "${work_dir}/build" --prefix "$(pwd)/${LIVEKIT_SDK_DIR}"
    if [ "$PLATFORM" == "android" ]; then
        # Apps also need WebRTC's Java classes, which come with the WebRTC build that the SDK's build downloads.
        local webrtc_jar
        # The SDK's build puts Cargo's output in CARGO_TARGET_DIR when it's set, relative to client-sdk-rust.
        webrtc_jar=$(cd "${src_dir}/client-sdk-rust" && find "$(cd "${CARGO_TARGET_DIR:-target}" && pwd)" -path "*/out/livekit_webrtc/*" -name libwebrtc.jar | head -1)
        if [ -z "$webrtc_jar" ]; then
            echo -e "${RED}libwebrtc.jar not found in the LiveKit SDK build${NC}"
            exit 1
        fi
        mkdir -p "${LIVEKIT_SDK_DIR}/java"
        cp "$webrtc_jar" "${LIVEKIT_SDK_DIR}/java/"
    fi

    livekit_version_key > "${LIVEKIT_SDK_DIR}/.version"
    rm -rf "$work_dir"
    echo -e "${GREEN}LiveKit SDK built successfully!${NC}"
}

# Function to download livekit-client, which the web build embeds
fetch_livekit_js() {
    if [ -f "livekit-js/.version" ] && [ "$(cat livekit-js/.version)" = "$LIVEKIT_JS_VERSION" ] && [ -f "livekit-js/livekit-client.e2ee.worker.js" ]; then
        echo -e "${GREEN}Using cached livekit-client ${LIVEKIT_JS_VERSION}${NC}"
        return 0
    fi
    echo -e "${YELLOW}Fetching livekit-client ${LIVEKIT_JS_VERSION}...${NC}"
    local archive="livekit-client-${LIVEKIT_JS_VERSION}.tgz"
    curl -sL "https://registry.npmjs.org/livekit-client/-/${archive}" -o "${archive}"
    verify_checksum "${archive}" "${LIVEKIT_JS_SHA256}"
    rm -rf livekit-js
    mkdir -p livekit-js
    # npm gives every file the same 1985 timestamp, which SCons would take to mean that a
    # different version's files are unchanged, and keep embedding the old version. (-m stamps
    # them with the current time instead.)
    tar -xmzf "${archive}" -C livekit-js --strip-components=2 package/dist/livekit-client.umd.js package/dist/livekit-client.e2ee.worker.js
    tar -xmzf "${archive}" -C livekit-js --strip-components=1 package/LICENSE
    rm "${archive}"
    echo "$LIVEKIT_JS_VERSION" > livekit-js/.version
    echo -e "${GREEN}livekit-client downloaded successfully!${NC}"
}

# Function to check if frametap cache is valid
check_frametap_cache() {
    echo -e "${YELLOW}Checking frametap cache...${NC}"

    if [ ! -d "frametap/include/frametap" ] || [ ! -d "frametap/lib" ]; then
        echo -e "${RED}Cache miss: frametap not found${NC}"
        return 1
    fi

    local version_key="$FRAMETAP_VERSION"
    [ "$PLATFORM" == "macos" ] && version_key="${FRAMETAP_VERSION}-${ARCH}"
    if [ -f "frametap/.version" ] && [ "$(cat frametap/.version)" = "$version_key" ]; then
        echo -e "${GREEN}frametap cache is valid!${NC}"
        return 0
    fi

    echo -e "${RED}Cache miss: frametap version mismatch${NC}"
    return 1
}

# Function to download frametap
fetch_frametap() {
    echo -e "${YELLOW}Fetching frametap for ${PLATFORM}...${NC}"

    local archive
    if [ "$PLATFORM" == "macos" ]; then
        archive="frametap-lib-${PLATFORM}-${ARCH}.zip"
    else
        archive="frametap-lib-${PLATFORM}.zip"
    fi
    local url="https://github.com/krazyjakee/frametap/releases/download/v${FRAMETAP_VERSION}/${archive}"

    rm -rf frametap
    mkdir -p frametap

    echo -e "${YELLOW}Downloading ${url}...${NC}"
    curl -sL "${url}" -o "${archive}"

    echo -e "${YELLOW}Extracting...${NC}"
    unzip -q "${archive}" -d frametap

    # If the zip has a nested directory, flatten it
    local inner_dir=$(ls frametap)
    if [ -d "frametap/${inner_dir}/include" ]; then
        mv frametap/${inner_dir}/* frametap/
        rm -rf "frametap/${inner_dir}"
    fi

    rm "${archive}"
    local version_key="$FRAMETAP_VERSION"
    [ "$PLATFORM" == "macos" ] && version_key="${FRAMETAP_VERSION}-${ARCH}"
    echo "$version_key" > frametap/.version
    echo -e "${GREEN}frametap downloaded and extracted successfully!${NC}"
}

# Function to check if livekit cache is valid
check_livekit_cache() {
    echo -e "${YELLOW}Checking livekit SDK cache...${NC}"

    if [ ! -d "${LIVEKIT_SDK_DIR}/include/livekit" ] || [ ! -d "${LIVEKIT_SDK_DIR}/lib" ]; then
        echo -e "${RED}Cache miss: livekit SDK not found${NC}"
        return 1
    fi

    if [ -f "${LIVEKIT_SDK_DIR}/.version" ] && [ "$(cat ${LIVEKIT_SDK_DIR}/.version)" = "$(livekit_version_key)" ]; then
        echo -e "${GREEN}livekit SDK cache is valid!${NC}"
        return 0
    fi

    echo -e "${RED}Cache miss: livekit SDK version mismatch${NC}"
    return 1
}

# Function to install dependencies
install_dependencies() {
    echo -e "${YELLOW}Checking dependencies for ${PLATFORM}...${NC}"
    
    if [ "$PLATFORM" == "linux" ]; then
        if [[ "$OSTYPE" != "linux-gnu"* ]]; then
            echo -e "${RED}Linux build requires a Linux environment. Current OS: $OSTYPE${NC}"
            exit 1
        fi
        
        local required_tools=("scons" "g++" "curl" "tar" "unzip")
        local missing_tools=()
        
        for tool in "${required_tools[@]}"; do
            if ! command -v "$tool" &> /dev/null; then
                missing_tools+=("$tool")
            fi
        done
        
        if [ ${#missing_tools[@]} -ne 0 ]; then
            echo -e "${RED}Missing required tools: ${missing_tools[*]}${NC}"
            echo -e "${YELLOW}Please install them with:${NC}"
            echo "sudo apt-get update && sudo apt-get install -y build-essential scons curl tar unzip"
            exit 1
        fi
        
    elif [ "$PLATFORM" == "windows" ]; then
        local required_tools=("scons" "curl" "unzip")
        local missing_tools=()
        
        for tool in "${required_tools[@]}"; do
            if ! command -v "$tool" &> /dev/null; then
                missing_tools+=("$tool")
            fi
        done
        
        if [ ${#missing_tools[@]} -ne 0 ]; then
            echo -e "${RED}Missing required tools for Windows native build: ${missing_tools[*]}${NC}"
            exit 1
        fi
    elif [ "$PLATFORM" == "macos" ]; then
        if [[ "$OSTYPE" != "darwin"* ]]; then
            echo -e "${RED}macOS build requires a macOS environment. Current OS: $OSTYPE${NC}"
            exit 1
        fi

        local required_tools=("scons" "curl" "tar" "unzip")
        local missing_tools=()

        for tool in "${required_tools[@]}"; do
            if ! command -v "$tool" &> /dev/null; then
                missing_tools+=("$tool")
            fi
        done

        if [ ${#missing_tools[@]} -ne 0 ]; then
            echo -e "${RED}Missing required tools: ${missing_tools[*]}${NC}"
            exit 1
        fi
    elif [ "$PLATFORM" == "ios" ]; then
        if [[ "$OSTYPE" != "darwin"* ]]; then
            echo -e "${RED}iOS builds require macOS with Xcode. Current OS: $OSTYPE${NC}"
            exit 1
        fi

        # Building the LiveKit SDK also needs git, CMake, Ninja and Rust (rustup); packaging needs Xcode's tools.
        local required_tools=("scons" "curl" "tar" "unzip" "git" "cmake" "ninja" "rustup" "cargo" "xcodebuild" "xcrun" "codesign" "install_name_tool" "otool")
        local missing_tools=()

        for tool in "${required_tools[@]}"; do
            if ! command -v "$tool" &> /dev/null; then
                missing_tools+=("$tool")
            fi
        done

        if [ ${#missing_tools[@]} -ne 0 ]; then
            echo -e "${RED}Missing required tools: ${missing_tools[*]}${NC}"
            exit 1
        fi
    elif [ "$PLATFORM" == "web" ]; then
        local required_tools=("scons" "curl" "tar" "emcc")
        local missing_tools=()

        for tool in "${required_tools[@]}"; do
            if ! command -v "$tool" &> /dev/null; then
                missing_tools+=("$tool")
            fi
        done

        if [ ${#missing_tools[@]} -ne 0 ]; then
            echo -e "${RED}Missing required tools: ${missing_tools[*]}${NC}"
            echo -e "${YELLOW}emcc comes from emsdk (https://emscripten.org), using the version Godot's web templates were built with.${NC}"
            exit 1
        fi
    elif [ "$PLATFORM" == "android" ]; then
        local ndk_root="${ANDROID_NDK_ROOT:-$ANDROID_NDK_HOME}"
        if [ -z "$ndk_root" ] || [ ! -d "$ndk_root" ]; then
            echo -e "${RED}ANDROID_NDK_ROOT or ANDROID_NDK_HOME must be set and point to a valid NDK installation${NC}"
            exit 1
        fi
        local ndk_major
        ndk_major=$(sed -n 's/^Pkg.Revision *= *\([0-9]*\).*/\1/p' "${ndk_root}/source.properties" 2>/dev/null)
        if [ -z "$ndk_major" ] || [ "$ndk_major" -lt "$MIN_ANDROID_NDK_MAJOR" ]; then
            echo -e "${RED}Android builds need NDK r${MIN_ANDROID_NDK_MAJOR} or newer, but ${ndk_root} is ${ndk_major:-unknown}${NC}"
            exit 1
        fi

        # Building the LiveKit SDK also needs git, CMake, Ninja and Rust (rustup).
        local required_tools=("scons" "curl" "tar" "unzip" "git" "cmake" "ninja" "rustup" "cargo")
        local missing_tools=()

        for tool in "${required_tools[@]}"; do
            if ! command -v "$tool" &> /dev/null; then
                missing_tools+=("$tool")
            fi
        done

        if [ ${#missing_tools[@]} -ne 0 ]; then
            echo -e "${RED}Missing required tools: ${missing_tools[*]}${NC}"
            exit 1
        fi
    fi
    
    echo -e "${GREEN}All required dependencies are available${NC}"
}

# Function to build the main project
build_main_project() {
    echo -e "${YELLOW}Building main project (${BUILD_TARGET})...${NC}"
    scons $SCONS_FLAGS target=$BUILD_TARGET
    echo -e "${GREEN}Main project build completed!${NC}"
    
    # Copy shared libraries next to the Godot GDExtension library so they can be loaded
    echo -e "${YELLOW}Copying LiveKit shared libraries to bin...${NC}"
    if [ "$PLATFORM" == "android" ]; then
        local dep_dir="addons/godot-livekit/bin/android-${ARCH}"
        mkdir -p "$dep_dir"
        cp ${LIVEKIT_SDK_DIR}/lib/*.so "$dep_dir/" || true
        # The editor adds it to Android exports (src/livekit_editor_plugin.cpp). It's the same for every ABI.
        cp "${LIVEKIT_SDK_DIR}/java/libwebrtc.jar" addons/godot-livekit/bin/
    elif [ "$PLATFORM" == "web" ]; then
        # livekit-client is embedded in the wasm, so its license ships alongside it.
        cp livekit-js/LICENSE addons/godot-livekit/bin/livekit-client-LICENSE.txt
    elif [ "$PLATFORM" == "linux" ]; then
        cp ${LIVEKIT_SDK_DIR}/lib/*.so* addons/godot-livekit/bin/ || true
    elif [ "$PLATFORM" == "windows" ]; then
        cp ${LIVEKIT_SDK_DIR}/bin/*.dll addons/godot-livekit/bin/ || true
    elif [ "$PLATFORM" == "ios" ]; then
        local slice_dir="build/ios-${ARCH}"
        [ "$IOS_SIMULATOR" = true ] && slice_dir="${slice_dir}-simulator"
        cp "${LIVEKIT_SDK_DIR}"/lib/*.dylib "$slice_dir/"
        prepare_ios_slice "$slice_dir"
        package_ios_xcframeworks
    elif [ "$PLATFORM" == "macos" ]; then
        local dep_dir="addons/godot-livekit/bin/macos-${ARCH}"
        mkdir -p "$dep_dir"
        cp ${LIVEKIT_SDK_DIR}/lib/*.dylib "$dep_dir/" || true

        # Fix hardcoded absolute paths in dylibs from upstream CI builds
        echo -e "${YELLOW}Fixing dylib install names...${NC}"
        for dylib in "$dep_dir"/*.dylib; do
            # Find any absolute path references to liblivekit_ffi.dylib and rewrite to @rpath
            otool -L "$dylib" | grep liblivekit_ffi.dylib | grep -v @rpath | awk '{print $1}' | while read -r bad_path; do
                echo "  Fixing $dylib: $bad_path -> @rpath/liblivekit_ffi.dylib"
                install_name_tool -change "$bad_path" "@rpath/liblivekit_ffi.dylib" "$dylib"
            done
        done
    fi
}

# Gives each library in an iOS slice the name it will have once Godot's iOS export turns it into a framework,
# @rpath/<name>.framework/<name>, and points the libraries' references to each other at those names.
# (The export renames each library, but not the references.)
prepare_ios_slice() {
    local slice_dir="$1"
    local dylib name dependency dependency_name
    for dylib in "$slice_dir"/*.dylib; do
        name=$(basename "$dylib" .dylib)
        install_name_tool -id "@rpath/${name}.framework/${name}" "$dylib"
        otool -L "$dylib" | tail -n +2 | awk '{print $1}' | while read -r dependency; do
            dependency_name=$(basename "$dependency" .dylib)
            if [ -f "$slice_dir/${dependency_name}.dylib" ] && [ "$dependency_name" != "$name" ]; then
                install_name_tool -change "$dependency" "@rpath/${dependency_name}.framework/${dependency_name}" "$dylib"
            fi
        done
        # Each framework finds the others beside its own, in the app's Frameworks directory.
        if ! otool -l "$dylib" | grep -A2 LC_RPATH | grep -q "@loader_path/.."; then
            install_name_tool -add_rpath "@loader_path/.." "$dylib"
        fi
        # Changing a library invalidates its signature, and arm64 code (even in the simulator) must be signed.
        codesign --force --sign - "$dylib"
    done
}

# Combines the iOS slices built so far (device, simulator, or both) into one .xcframework per library.
package_ios_xcframeworks() {
    local bin_dir="addons/godot-livekit/bin"
    local name slice_dir args
    for name in "libgodot-livekit.ios.${ARCH}" liblivekit liblivekit_ffi; do
        args=()
        for slice_dir in "build/ios-${ARCH}" "build/ios-${ARCH}-simulator"; do
            if [ -f "$slice_dir/${name}.dylib" ]; then
                args+=(-library "$slice_dir/${name}.dylib")
            fi
        done
        local xcframework="$bin_dir/${name%.ios.${ARCH}}.ios.xcframework"
        rm -rf "$xcframework"
        xcodebuild -create-xcframework "${args[@]}" -output "$xcframework"
    done
}

# Prints the key that identifies the LiveKit SDK build in its .version file.
livekit_version_key() {
    if [ "$PLATFORM" == "macos" ]; then
        echo "${LIVEKIT_VERSION}-${ARCH}"
    elif [ "$BUILD_LIVEKIT_SDK" = true ]; then
        # A changed patch needs a rebuild.
        echo "${LIVEKIT_VERSION}-${PLATFORM}-${ARCH}$([ "$IOS_SIMULATOR" = true ] && echo -simulator)-$(git hash-object "$LIVEKIT_MOBILE_PATCH" | cut -c1-12)"
    else
        echo "$LIVEKIT_VERSION"
    fi
}

# --- Main Execution ---
main() {
    # Parse command-line arguments
    if [ -z "$1" ]; then
        show_usage
    fi

    local platform_arg=""
    local arch_arg=""
    for arg in "$@"; do
        case "$arg" in
            --debug)
                BUILD_TARGET="template_debug"
                ;;
            linux|macos|windows|android|ios|web)
                platform_arg="$arg"
                ;;
            --simulator)
                IOS_SIMULATOR=true
                ;;
            arm64|x86_64)
                arch_arg="$arg"
                ;;
            *)
                show_usage
                ;;
        esac
    done

    if [ -z "$platform_arg" ]; then
        show_usage
    fi

    case "$platform_arg" in
        linux)
            setup_linux
            ;;
        macos)
            setup_macos "$arch_arg"
            ;;
        windows)
            setup_windows
            ;;
        android)
            setup_android "$arch_arg"
            ;;
        ios)
            setup_ios
            ;;
        web)
            setup_web
            ;;
    esac

    echo -e "${BLUE}Platform: ${PLATFORM}, Architecture: ${ARCH}${NC}"
    echo ""

    install_dependencies

    if [ "$SKIP_FRAMETAP" = true ]; then
        echo -e "${YELLOW}Skipping frametap (not supported on ${PLATFORM})${NC}"
    elif ! check_frametap_cache; then
        fetch_frametap
    else
        echo -e "${GREEN}Using cached frametap${NC}"
    fi

    if [ "$SKIP_LIVEKIT_SDK" = true ]; then
        fetch_livekit_js
    elif check_livekit_cache; then
        echo -e "${GREEN}Using cached livekit SDK${NC}"
    elif [ "$BUILD_LIVEKIT_SDK" = true ]; then
        build_livekit_sdk
    else
        fetch_livekit
    fi

    if ! check_godotcpp_cache; then
        fetch_godotcpp
    else
        echo -e "${GREEN}Using cached godot-cpp prebuilt${NC}"
    fi
    
    echo ""
    build_main_project
    
    echo ""
    echo -e "${GREEN}=== Build Complete! ===${NC}"
    echo -e "${BLUE}Built for: ${PLATFORM} (${ARCH})${NC}"
}

main "$@"