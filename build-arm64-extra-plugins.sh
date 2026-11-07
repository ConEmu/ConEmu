#!/bin/sh
set -eu
cd "$(dirname "$0")"
root=$PWD
out=${1:-"$root/Release-arm64"}
obj="$out/obj/extra-plugins"
plugin="$out/package/Plugins/ConEmu"
target=aarch64-w64-mingw32
flags="-std=c++17 -O2 -DUNICODE -D_UNICODE -DWIN64 -D_export= -DNDEBUG -DHIDE_TODO -DNOMINMAX -D_WIN32_WINNT=0x0601 -fdelayed-template-parsing -fms-extensions -fms-compatibility-version=19.20 -include crtdbg.h -Wno-c++11-narrowing -Wno-nonportable-include-path -Wno-pragma-pack"
mkdir -p "$obj" "$plugin/Background" "$plugin/Lines" "$plugin/Thumbs"
cd "$root/src"

project_sources() {
    sed -n 's/.*<ClCompile Include="\([^"]*\)".*/\1/p' "$1.vcxproj" | tr '\\' /
}

for project in common-kernel common-user; do
    for source in $(project_sources "$project"); do
        object="$obj/${source##*/}.o"
        "$target-clang++" $flags -c "$source" -o "$object"
    done
done
llvm-ar rcs "$obj/common.a" "$obj"/*.cpp.o

build_plugin() {
    project=$1
    resource=$2
    definition=$3
    output=$4
    "$target-windres" -D UNICODE -D WIN64 -I "$(dirname "$resource")" -i "$resource" -o "$obj/$project.res.o"
    "$target-clang++" -shared -static $flags $(project_sources "$project") "$obj/$project.res.o" \
        "$definition" "$obj/common.a" -o "$output" -lversion -lwininet -lgdi32 -ladvapi32 \
        -lshell32 -lshlwapi -lole32 -loleaut32 -lcomctl32 -luuid -lwinmm -lmsimg32
}

build_plugin Far.ConEmuBg ConEmuBg/ConEmuBg.rc ConEmuBg/export.def "$plugin/Background/ConEmuBg.arm64.dll"
build_plugin Far.ConEmuLn ConEmuLn/ConEmuLn.rc ConEmuLn/export.def "$plugin/Lines/ConEmuLn.arm64.dll"
build_plugin Far.ConEmuTh ConEmuTh/ConEmuTh.rc ConEmuTh/export.def "$plugin/Thumbs/ConEmuTh.arm64.dll"
for module in gdip ico pe; do
    name=$module
    if [ "$module" = gdip ]; then name=gdi+; fi
    source="ConEmuTh/Modules/$module/$module"
    build_plugin "Far.ConEmuTh.$name" "$source.rc" "$source.def" "$plugin/Thumbs/$name.t64"
done

resources="$root/Release/plugins/ConEmu"
for folder in Background Lines Thumbs; do
    cp "$resources/$folder/"*.lng "$plugin/$folder/"
done
cp "$resources/Background/Background.xml" "$plugin/Background/"
cp -R "$resources/Background/img" "$plugin/Background/"
