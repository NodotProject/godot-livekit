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
LIVEKIT_VERSION="0.3.1"
GODOT_CPP_VERSION="godot-4.5-stable"
FRAMETAP_VERSION="0.1.3"

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
    echo -e "${YELLOW}Usage: $0 [linux|macos|windows] [arm64|x86_64] [--debug]${NC}"
    echo "  linux: Build for Linux (x86_64)"
    echo "  macos [arm64|x86_64]: Build for macOS (defaults to host arch)"
    echo "  windows: Build for Windows (x86_64, cross-compile)"
    echo "  --debug: Build debug variant (template_debug) instead of release"
    exit 1
}

# --- Platform-specific Setup ---
setup_linux() {
    PLATFORM="linux"
    ARCH="x86_64"
    SCONS_FLAGS="platform=linux arch=x86_64"
    LIVEKIT_ARCHIVE="livekit-sdk-linux-x64-${LIVEKIT_VERSION}.tar.gz"
    LIVEKIT_URL="https://github.com/krazyjakee/client-sdk-cpp/releases/download/v${LIVEKIT_VERSION}/${LIVEKIT_ARCHIVE}"
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
    LIVEKIT_URL="https://github.com/krazyjakee/client-sdk-cpp/releases/download/v${LIVEKIT_VERSION}/${LIVEKIT_ARCHIVE}"
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (macOS ${ARCH}) ===${NC}"
}

setup_windows() {
    PLATFORM="windows"
    ARCH="x86_64"
    SCONS_FLAGS="platform=windows arch=x86_64"
    LIVEKIT_ARCHIVE="livekit-sdk-windows-x64-${LIVEKIT_VERSION}.zip"
    LIVEKIT_URL="https://github.com/krazyjakee/client-sdk-cpp/releases/download/v${LIVEKIT_VERSION}/${LIVEKIT_ARCHIVE}"
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (Windows Native) ===${NC}"
}

# --- Build Functions ---

# Function to check if godot-cpp cache is valid
check_godotcpp_cache() {
    echo -e "${YELLOW}Checking godot-cpp cache...${NC}"

    if [ ! -d "godot-cpp/bin" ] || [ ! -d "godot-cpp/include" ] || [ ! -d "godot-cpp/gen" ]; then
        echo -e "${RED}Cache miss: godot-cpp prebuilt not found${NC}"
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

# Function to fetch godot-cpp prebuilt
fetch_godotcpp() {
    echo -e "${YELLOW}Fetching godot-cpp prebuilt (${GODOT_CPP_VERSION})...${NC}"
    
    local archive="godot-cpp-prebuilt-${GODOT_CPP_VERSION}.zip"
    local url="https://github.com/NodotProject/godot-cpp-builds/releases/download/${GODOT_CPP_VERSION}/${archive}"
    
    rm -rf godot-cpp
    mkdir -p godot-cpp
    
    echo -e "${YELLOW}Downloading ${url}...${NC}"
    curl -sL "${url}" -o "${archive}"
    
    echo -e "${YELLOW}Extracting...${NC}"
    mkdir -p godot-cpp-temp
    unzip -q "${archive}" -d godot-cpp-temp
    
    mv godot-cpp-temp/godot-cpp-prebuilt/* godot-cpp/
    rm -rf godot-cpp-temp "${archive}"
    
    echo "$GODOT_CPP_VERSION" > godot-cpp/.version
    echo -e "${GREEN}godot-cpp prebuilt downloaded and extracted successfully!${NC}"
}

# Function to download livekit
fetch_livekit() {
    echo -e "${YELLOW}Fetching LiveKit C++ SDK for ${PLATFORM} (${ARCH})...${NC}"

    rm -rf livekit-sdk
    mkdir -p livekit-sdk
    
    echo -e "${YELLOW}Downloading ${LIVEKIT_URL}...${NC}"
    curl -sL "${LIVEKIT_URL}" -o "${LIVEKIT_ARCHIVE}"
    
    echo -e "${YELLOW}Extracting...${NC}"
    if [[ "$LIVEKIT_ARCHIVE" == *.zip ]]; then
        unzip -q "${LIVEKIT_ARCHIVE}" -d livekit-sdk
        local inner_dir=$(ls livekit-sdk)
        if [ -d "livekit-sdk/${inner_dir}/include" ]; then
            mv livekit-sdk/${inner_dir}/* livekit-sdk/
            rm -rf livekit-sdk/${inner_dir}
        fi
    else
        tar -xzf "${LIVEKIT_ARCHIVE}" -C livekit-sdk --strip-components=1
    fi
    
    rm "${LIVEKIT_ARCHIVE}"

    # Patch: local_participant.h uses std::vector<ParticipantTrackPermission> in a default
    # parameter but only forward-declares the type. Include track.h for the full definition.
    local lp_header="livekit-sdk/include/livekit/local_participant.h"
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

    local version_key="$LIVEKIT_VERSION"
    [ "$PLATFORM" == "macos" ] && version_key="${LIVEKIT_VERSION}-${ARCH}"
    echo "$version_key" > livekit-sdk/.version
    echo -e "${GREEN}LiveKit SDK downloaded and extracted successfully!${NC}"
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

    if [ ! -d "livekit-sdk/include/livekit" ] || [ ! -d "livekit-sdk/lib" ]; then
        echo -e "${RED}Cache miss: livekit SDK not found${NC}"
        return 1
    fi

    local version_key="$LIVEKIT_VERSION"
    [ "$PLATFORM" == "macos" ] && version_key="${LIVEKIT_VERSION}-${ARCH}"
    if [ -f "livekit-sdk/.version" ] && [ "$(cat livekit-sdk/.version)" = "$version_key" ]; then
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
    if [ "$PLATFORM" == "linux" ]; then
        cp livekit-sdk/lib/*.so* addons/godot-livekit/bin/ || true
    elif [ "$PLATFORM" == "windows" ]; then
        cp livekit-sdk/bin/*.dll addons/godot-livekit/bin/ || true
    elif [ "$PLATFORM" == "macos" ]; then
        local dep_dir="addons/godot-livekit/bin/macos-${ARCH}"
        mkdir -p "$dep_dir"
        cp livekit-sdk/lib/*.dylib "$dep_dir/" || true

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
            linux|macos|windows)
                platform_arg="$arg"
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
    esac

    echo -e "${BLUE}Platform: ${PLATFORM}, Architecture: ${ARCH}${NC}"
    echo ""

    install_dependencies
    
    if ! check_frametap_cache; then
        fetch_frametap
    else
        echo -e "${GREEN}Using cached frametap${NC}"
    fi

    if ! check_livekit_cache; then
        fetch_livekit
    else
        echo -e "${GREEN}Using cached livekit SDK${NC}"
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