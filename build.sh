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

# --- Helper Functions ---
show_usage() {
    echo -e "${YELLOW}Usage: $0 [linux|macos|windows]${NC}"
    echo "  linux: Build for Linux (x86_64)"
    echo "  macos: Build for macOS (universal)"
    echo "  windows: Build for Windows (x86_64, cross-compile)"
    exit 1
}

# --- Platform-specific Setup ---
setup_linux() {
    PLATFORM="linux"
    ARCH="x86_64"
    SCONS_FLAGS="platform=linux arch=x86_64"
    LIVEKIT_ARCHIVE="livekit-sdk-linux-x64-${LIVEKIT_VERSION}.tar.gz"
    LIVEKIT_URL="https://github.com/livekit/client-sdk-cpp/releases/download/v${LIVEKIT_VERSION}/${LIVEKIT_ARCHIVE}"
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (Linux) ===${NC}"
}

setup_macos() {
    PLATFORM="macos"
    ARCH="universal"
    SCONS_FLAGS="platform=macos arch=universal"
    LIVEKIT_ARCHIVE="livekit-sdk-macos-universal-${LIVEKIT_VERSION}.tar.gz"
    LIVEKIT_URL="https://github.com/krazyjakee/client-sdk-cpp/releases/download/v${LIVEKIT_VERSION}/${LIVEKIT_ARCHIVE}"
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (macOS) ===${NC}"
}

setup_windows() {
    PLATFORM="windows"
    ARCH="x86_64"
    SCONS_FLAGS="platform=windows use_mingw=yes arch=x86_64"
    LIVEKIT_ARCHIVE="livekit-sdk-windows-x64-${LIVEKIT_VERSION}.zip"
    LIVEKIT_URL="https://github.com/livekit/client-sdk-cpp/releases/download/v${LIVEKIT_VERSION}/${LIVEKIT_ARCHIVE}"
    echo -e "${BLUE}=== Godot-LiveKit Local Build Script (Windows Cross-Compile) ===${NC}"
}

# --- Build Functions ---

# Function to check if godot-cpp cache is valid
check_godotcpp_cache() {
    echo -e "${YELLOW}Checking godot-cpp cache...${NC}"
    
    if [ ! -d "godot-cpp/bin" ] || [ ! -d "godot-cpp/include" ] || [ ! -d "godot-cpp/gen" ]; then
        echo -e "${RED}Cache miss: godot-cpp prebuilt not found${NC}"
        return 1
    fi
    
    echo -e "${GREEN}godot-cpp cache is valid!${NC}"
    return 0
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
    
    echo -e "${GREEN}godot-cpp prebuilt downloaded and extracted successfully!${NC}"
}

# Function to download livekit
fetch_livekit() {
    if [ "$PLATFORM" == "macos" ] && [ -f "livekit-sdk/lib/liblivekit.dylib" ]; then
        if lipo -info livekit-sdk/lib/liblivekit.dylib | grep -q "x86_64 arm64"; then
            echo -e "${GREEN}Detected local universal LiveKit SDK, skipping download.${NC}"
            return 0
        fi
    fi

    echo -e "${YELLOW}Fetching LiveKit C++ SDK for ${PLATFORM}...${NC}"

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
    echo -e "${GREEN}LiveKit SDK downloaded and extracted successfully!${NC}"
}

# Function to check if livekit cache is valid
check_livekit_cache() {
    echo -e "${YELLOW}Checking livekit SDK cache...${NC}"

    if [ ! -d "livekit-sdk/include/livekit" ] || [ ! -d "livekit-sdk/lib" ]; then
        echo -e "${RED}Cache miss: livekit SDK not found${NC}"
        return 1
    fi

    echo -e "${GREEN}livekit SDK cache is valid!${NC}"
    return 0
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
            echo -e "${RED}Missing required tools for Windows cross-compilation: ${missing_tools[*]}${NC}"
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
    echo -e "${YELLOW}Building main project...${NC}"
    scons $SCONS_FLAGS target=template_release
    echo -e "${GREEN}Main project build completed!${NC}"
    
    # Copy shared libraries next to the Godot GDExtension library so they can be loaded
    echo -e "${YELLOW}Copying LiveKit shared libraries to bin...${NC}"
    if [ "$PLATFORM" == "linux" ]; then
        cp livekit-sdk/lib/*.so* addons/godot-livekit/bin/ || true
    elif [ "$PLATFORM" == "windows" ]; then
        cp livekit-sdk/bin/*.dll addons/godot-livekit/bin/ || true
    elif [ "$PLATFORM" == "macos" ]; then
        cp livekit-sdk/lib/*.dylib addons/godot-livekit/bin/ || true
    fi
}

# --- Main Execution ---
main() {
    # Parse command-line arguments
    if [ -z "$1" ]; then
        show_usage
    fi

    case "$1" in
        linux)
            setup_linux
            ;;
        macos)
            setup_macos
            ;;
        windows)
            export CC=x86_64-w64-mingw32-gcc
            export CXX=x86_64-w64-mingw32-g++
            setup_windows
            ;;
        *)
            show_usage
            ;;
    esac

    echo -e "${BLUE}Platform: ${PLATFORM}, Architecture: ${ARCH}${NC}"
    echo ""

    install_dependencies
    
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