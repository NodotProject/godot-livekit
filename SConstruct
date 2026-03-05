from SCons.Script import ARGUMENTS, Environment, Mkdir, Default, File, CacheDir
import os
import sys

# Resolve platform/target/arch from args
platform = ARGUMENTS.get('platform')
if not platform:
    p = sys.platform
    if p.startswith('win'):
        platform = 'windows'
    elif p == 'darwin':
        platform = 'macos'
    else:
        platform = 'linux'

target = ARGUMENTS.get('target', 'template_release')
if platform == 'android':
    arch = ARGUMENTS.get('arch', 'arm64')
elif platform == 'macos':
    arch = ARGUMENTS.get('arch', 'arm64')
else:
    arch = ARGUMENTS.get('arch', 'x86_64')

is_windows = platform == 'windows'
is_android = platform == 'android'

# Screen capture support (disabled for Android)
enable_screen_capture = ARGUMENTS.get('screen_capture', 'no' if is_android else 'yes').lower() not in ['no', 'false', '0']

# Set up the environment based on the platform
use_mingw_arg = ARGUMENTS.get('use_mingw', 'no')
use_mingw = use_mingw_arg.lower() in ['yes', 'true', '1']

if is_android:
    ndk_root = os.environ.get('ANDROID_NDK_ROOT', os.environ.get('ANDROID_NDK_HOME', ''))
    if not ndk_root:
        print("ERROR: ANDROID_NDK_ROOT or ANDROID_NDK_HOME must be set for Android builds")
        Exit(1)
    # Find the NDK toolchain
    import glob
    toolchain_bins = glob.glob(os.path.join(ndk_root, 'toolchains', 'llvm', 'prebuilt', '*', 'bin'))
    if not toolchain_bins:
        print("ERROR: Could not find NDK toolchain in " + ndk_root)
        Exit(1)
    ndk_bin = toolchain_bins[0]
    ndk_sysroot = os.path.join(os.path.dirname(ndk_bin), 'sysroot')
    android_target = 'aarch64-linux-android24'
    env = Environment()
    env.Replace(CC=os.path.join(ndk_bin, 'clang'))
    env.Replace(CXX=os.path.join(ndk_bin, 'clang++'))
    env.Replace(LINK=os.path.join(ndk_bin, 'clang++'))
    env.Replace(AR=os.path.join(ndk_bin, 'llvm-ar'))
    env.Replace(RANLIB=os.path.join(ndk_bin, 'llvm-ranlib'))
elif is_windows and not use_mingw:
    env = Environment(tools=['default', 'msvc'])
elif is_windows and use_mingw:
    env = Environment(tools=['gcc', 'g++', 'gnulink', 'ar', 'gas'])
    cc_cmd = os.environ.get('CC', 'x86_64-w64-mingw32-gcc')
    cxx_cmd = os.environ.get('CXX', 'x86_64-w64-mingw32-g++')
    env.Replace(CC=cc_cmd)
    env.Replace(CXX=cxx_cmd)
    env.Replace(LINK=cxx_cmd)
else:
    env = Environment()

cache_dir = os.environ.get('SCONS_CACHE_DIR')
if cache_dir:
    CacheDir(cache_dir)

# Add include paths for godot-cpp and LiveKit
cpppath = [
    'src',
    '.',
    'godot-cpp/include',
    'godot-cpp/gen/include',
    'godot-cpp/gdextension',
    'livekit-sdk/include',
]
if enable_screen_capture:
    cpppath.append('frametap/include')
env.Append(CPPPATH=cpppath)

# For Windows cross-compilation with MinGW, point to standard paths
if use_mingw and is_windows:
    env.Append(CCFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])
    env.Append(LINKFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])

libpath = ['godot-cpp/bin', 'livekit-sdk/lib']
if enable_screen_capture:
    libpath.append('frametap/lib')
env.Append(LIBPATH=libpath)

if is_android:
    env.Append(CCFLAGS=[f'--target={android_target}', f'--sysroot={ndk_sysroot}', '-fPIC', '-march=armv8-a'])
    env.Append(CXXFLAGS=['-std=c++20'])
    env.Append(LINKFLAGS=[f'--target={android_target}', f'--sysroot={ndk_sysroot}'])
    env.Append(CPPDEFINES=['ANDROID_ENABLED', 'UNIX_ENABLED'])
elif is_windows and not use_mingw:
    env.Append(CXXFLAGS=['/std:c++20', '/EHsc', '/MT'])
    env.Append(CPPDEFINES=['WIN32', '_WIN32', 'WINDOWS_ENABLED', 'TYPED_METHOD_BIND', 'NOMINMAX'])
elif is_windows and use_mingw:
    env.Append(CCFLAGS=['-fPIC'])
    env.Append(CXXFLAGS=['-std=c++20'])
    env.Append(CPPDEFINES=['WIN32', '_WIN32', 'WINDOWS_ENABLED'])
    env.Append(CCFLAGS=['-Wwrite-strings'])
    env.Append(LINKFLAGS=['-Wl,--no-undefined'])
    env.Append(LINKFLAGS=['-static-libgcc', '-static-libstdc++'])
else:
    env.Append(CCFLAGS=['-fPIC'])
    env.Append(CXXFLAGS=['-std=c++20'])
    if platform == 'macos':
        env.Append(CCFLAGS=['-arch', arch])
        env.Append(LINKFLAGS=['-arch', arch])
        env.Append(LINKFLAGS=['-Wl,-rpath,@loader_path', f'-Wl,-rpath,@loader_path/macos-{arch}'])
    elif platform == 'linux':
        env.Append(LINKFLAGS=['-Wl,-rpath,\\$$ORIGIN'])

if is_windows and not use_mingw:
    lib_ext = '.lib'
    lib_prefix = 'lib' # Prebuilt zip has 'lib' prefix even for MSVC .lib file
else:
    lib_ext = '.a'
    lib_prefix = 'lib'

# Add godot-cpp library
if platform == 'macos':
    # On macOS, if we build for arm64 or x86_64, we might still want to use the universal godot-cpp lib if specific arch isn't there
    godot_cpp_lib_specific = f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}"
    godot_cpp_lib_uni = f"{lib_prefix}godot-cpp.{platform}.{target}.universal{lib_ext}"
    if os.path.exists(os.path.join('godot-cpp', 'bin', godot_cpp_lib_specific)):
        godot_cpp_lib = godot_cpp_lib_specific
    else:
        godot_cpp_lib = godot_cpp_lib_uni
else:
    godot_cpp_lib = f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}"

env.Append(LIBS=[File(os.path.join('godot-cpp', 'bin', godot_cpp_lib))])

# Add LiveKit libraries (linking dynamically since they are pre-built shared libs)
if is_windows:
    # Livekit client-sdk-cpp on Windows provides .lib import files inside lib/
    env.Append(LIBS=[File('livekit-sdk/lib/livekit_ffi.dll.lib'), File('livekit-sdk/lib/livekit.lib')])
elif is_android:
    env.Append(LIBS=['livekit_ffi', 'livekit', 'log'])
elif platform == 'macos':
    env.Append(LIBS=['livekit_ffi', 'livekit'])
else: # linux
    env.Append(LIBS=['livekit_ffi', 'livekit'])

# Add frametap static library (only when screen capture is enabled)
if enable_screen_capture:
    if is_windows and not use_mingw:
        env.Append(LIBS=[File('frametap/lib/frametap.lib')])
    else:
        env.Append(LIBS=[File('frametap/lib/libframetap.a')])

# Add platform-specific system libraries
if is_windows:
    env.Append(LIBS=['ws2_32', 'wsock32', 'iphlpapi', 'crypt32', 'advapi32', 'userenv', 'bcrypt',
                      'dxgi', 'd3d11', 'dwmapi', 'user32', 'gdi32', 'ole32'])
elif is_android:
    pass  # Android system libs (log) already added above
elif platform == 'linux':
    env.Append(LIBS=['pthread', 'dl', 'm'])
    if enable_screen_capture:
        # frametap requires X11 libs and PipeWire/Wayland (discovered via pkg-config)
        env.Append(LIBS=['X11', 'Xext', 'Xfixes', 'Xinerama'])
        env.ParseConfig('pkg-config --cflags --libs libpipewire-0.3 libsystemd wayland-client 2>/dev/null || true')
elif platform == 'macos':
    env.Append(LIBS=['pthread'])
    frameworks = ['CoreFoundation', 'Foundation', 'Security', 'AudioToolbox', 'CoreAudio', 'AppKit']
    if enable_screen_capture:
        frameworks.extend(['ScreenCaptureKit', 'CoreGraphics', 'CoreMedia', 'CoreVideo'])
    env.Append(FRAMEWORKS=frameworks)

# Screen capture support
if enable_screen_capture:
    env.Append(CPPDEFINES=['LIVEKIT_SCREEN_CAPTURE_SUPPORTED'])

# E2EE support - enabled by default; disable with e2ee=no if your SDK lacks E2EE symbols
enable_e2ee = ARGUMENTS.get('e2ee', 'yes').lower() not in ['no', 'false', '0']
if enable_e2ee:
    env.Append(CPPDEFINES=['LIVEKIT_E2EE_SUPPORTED'])

src_files = [
    'src/register_types.cpp',
    'src/livekit_room.cpp',
    'src/livekit_participant.cpp',
    'src/livekit_track.cpp',
    'src/livekit_track_publication.cpp',
    'src/livekit_video_stream.cpp',
    'src/livekit_audio_stream.cpp',
    'src/livekit_video_source.cpp',
    'src/livekit_audio_source.cpp',
    'src/livekit_poller.cpp',
]

if enable_screen_capture:
    src_files.append('src/livekit_screen_capture.cpp')

if enable_e2ee:
    src_files.append('src/livekit_e2ee.cpp')

env.Execute(Mkdir('addons/godot-livekit/bin'))

if is_windows:
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.dll'
elif platform == 'macos':
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.dylib'
else:  # linux, android
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.so'

# Build the full target name explicitly so that MSVC's SharedLibrary
# builder does not misparse the dots in the platform/arch portion as a
# file-extension suffix (which causes "should have exactly one target
# with the suffix: .dll").
lib_name = f"{env['SHLIBPREFIX']}godot-livekit.{platform}.{arch}{env['SHLIBSUFFIX']}"
library = env.SharedLibrary(target=lib_name, source=src_files)
installed_library = env.Install('addons/godot-livekit/bin', library)
Default(installed_library)