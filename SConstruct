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
arch = ARGUMENTS.get('arch', 'universal' if platform == 'macos' else 'x86_64')

# Set up the environment based on the platform
use_mingw_arg = ARGUMENTS.get('use_mingw', 'no')
use_mingw = use_mingw_arg.lower() in ['yes', 'true', '1']

if platform == 'windows' and not use_mingw:
    env = Environment(tools=['default', 'msvc'])
elif platform == 'windows' and use_mingw:
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
env.Append(CPPPATH=[
    'src',
    '.',
    'godot-cpp/include', 
    'godot-cpp/gen/include', 
    'godot-cpp/gdextension',
    'livekit-sdk/include',
])

# For Windows cross-compilation with MinGW, point to standard paths
if use_mingw or platform == 'windows':
    env.Append(CCFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])
    env.Append(LINKFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])

env.Append(LIBPATH=['godot-cpp/bin', 'livekit-sdk/lib'])

is_windows = platform == 'windows'
if is_windows and not use_mingw:
    env.Append(CXXFLAGS=['/std:c++17'])
elif is_windows and use_mingw:
    env.Append(CCFLAGS=['-fPIC'])
    env.Append(CXXFLAGS=['-std=c++17'])
    env.Append(CPPDEFINES=['WIN32', '_WIN32', 'WINDOWS_ENABLED'])
    env.Append(CCFLAGS=['-Wwrite-strings'])
    env.Append(LINKFLAGS=['-Wl,--no-undefined'])
    env.Append(LINKFLAGS=['-static-libgcc', '-static-libstdc++'])
else:
    env.Append(CCFLAGS=['-fPIC'])
    env.Append(CXXFLAGS=['-std=c++17'])
    if platform == 'macos':
        if arch == 'universal':
            env.Append(CCFLAGS=['-arch', 'x86_64', '-arch', 'arm64'])
            env.Append(LINKFLAGS=['-arch', 'x86_64', '-arch', 'arm64'])
        else:
            env.Append(CCFLAGS=['-arch', arch])
            env.Append(LINKFLAGS=['-arch', arch])

if is_windows and not use_mingw:
    lib_ext = '.lib'
    lib_prefix = ''
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
    # So we'll link against 'livekit_ffi' and 'livekit'
    env.Append(LIBS=['livekit_ffi', 'livekit'])
elif platform == 'macos':
    env.Append(LIBS=['livekit_ffi', 'livekit'])
else: # linux
    env.Append(LIBS=['livekit_ffi', 'livekit'])

# Add platform-specific system libraries
if is_windows:
    env.Append(LIBS=['ws2_32', 'wsock32', 'iphlpapi', 'crypt32', 'advapi32', 'userenv', 'bcrypt'])
elif platform == 'linux':
    env.Append(LIBS=['pthread', 'dl', 'm'])
elif platform == 'macos':
    env.Append(LIBS=['pthread'])
    env.Append(FRAMEWORKS=['CoreFoundation', 'Foundation', 'Security', 'AudioToolbox', 'CoreAudio'])

src_files = [
    'src/register_types.cpp',
    'src/livekit_room.cpp',
    'src/livekit_participant.cpp',
]

env.Execute(Mkdir('addons/godot-livekit/bin'))

if is_windows:
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.dll'
elif platform == 'macos':
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.dylib'
else:
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.so'

lib_name = f"godot-livekit.{platform}.{target}.{arch}"
if platform == 'macos' and not lib_name.endswith('.dylib'):
    lib_name += '.dylib'
library = env.SharedLibrary(target=lib_name, source=src_files)
installed_library = env.Install('addons/godot-livekit/bin', library)
Default(installed_library)