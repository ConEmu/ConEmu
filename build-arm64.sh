#!/bin/sh
set -eu

cd "$(dirname "$0")"
root=$PWD
out=${BUILD_DIR:-"$root/Release-arm64"}
case "$out" in
    /*) ;;
    *) echo "BUILD_DIR must be an absolute path" >&2; exit 1 ;;
esac

target=aarch64-w64-mingw32-
cpp_options="-fdelayed-template-parsing -fms-extensions -fms-compatibility-version=19.20"
cpp_flags="-include crtdbg.h -Wno-c++11-narrowing -Wno-nonportable-include-path"
package="$out/package"
plugin="$package/Plugins/ConEmu"
mkdir -p "$out/obj" "$package/ConEmu" "$plugin"

build_component() {
    make -B -j "${JOBS:-4}" -C "$root/src/$1" -f makefile_gcc \
        ARM64=1 DIRBIT=64 CROSS_HOST="$target" CXX="${target}clang++" CC="${target}clang" \
        FCPPOPT="$cpp_options" USERCPPFLAGS="$cpp_flags" \
        TMPDIR="$out/obj" RELEASEDIR="$2"
}

build_component ConEmu "$package"
build_component ConEmuC "$package/ConEmu"
build_component ConEmuCD "$package/ConEmu"

cd "$root/src/ConEmuPlugin"
# Reuse the existing source list. The additional units are needed by this build.
set -- $(sed -nE 's/^[[:space:]]*([^[:space:]]+\.cpp)[[:space:]]*\\?[[:space:]]*$/\1/p' makefile_gcc) \
    KeyEvents.cpp LogBrowser.cpp ../common/CmdArg.cpp ../common/WRegistry.cpp \
    ../common/EnvVar.cpp ../common/MModule.cpp
"${target}windres" -D UNICODE -D WIN64 -i ConEmu.rc -o "$out/obj/plugin-version.o"
"${target}clang++" -shared -static -std=c++17 -O2 -DUNICODE -D_UNICODE -DWIN64 \
    -DCONEMU_INTERNAL_PLUGIN_EXPORTS -DNDEBUG -DHIDE_TODO -DNOMINMAX -D_WIN32_WINNT=0x0601 \
    $cpp_options $cpp_flags -Wno-pragma-pack "$@" "$out/obj/plugin-version.o" export.def \
    -o "$plugin/ConEmu.arm64.dll" -lversion -lwininet -lgdi32 -ladvapi32 -lshell32 \
    -lshlwapi -lole32 -loleaut32 -lcomctl32 -luuid -lwinmm

cp "$root/Release/ConEmu/License.txt" "$root/Release/ConEmu/ConEmu.l10n" "$package/ConEmu/"
mkdir -p "$package/ConEmu/Scripts"
cp "$root/Release/ConEmu/Scripts/ConEmu.NoHooks.psm1" "$package/ConEmu/Scripts/"
cp "$root/Release/ConEmu/Scripts/ConEmu.NoHooks.bash" "$package/ConEmu/Scripts/"
cp "$root/src/common/Unicode-LICENSE.txt" "$package/ConEmu/"
cp "$root"/Release/plugins/ConEmu/ConEmuPlugin_*.lng "$plugin/"

sh "$root/build-arm64-extra-plugins.sh" "$out"
