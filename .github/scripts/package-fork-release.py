"""Package CI binaries for installation into an existing Far directory."""

import hashlib
from pathlib import Path
import struct
import sys
from zipfile import ZIP_DEFLATED, ZipFile


MACHINES = {"arm64": 0xAA64, "x86": 0x014C, "x64": 0x8664}
PLUGINS = {
    "": "ConEmu",
    "Background/": "ConEmuBg",
    "Lines/": "ConEmuLn",
    "Thumbs/": "ConEmuTh",
}


def add_binary(files, root, source, destination, arch):
    path = root / source
    data = path.read_bytes()
    offset = struct.unpack_from("<I", data, 60)[0]
    machine = struct.unpack_from("<H", data, offset + 4)[0]
    if data[:2] != b"MZ" or data[offset:offset + 4] != b"PE\0\0" or machine != MACHINES[arch]:
        raise ValueError(f"{path}: expected {arch} PE, got machine {machine:#06x}")
    files[destination] = path


def package(artifacts, output, sha, arch):
    files = {}
    arm = arch == "arm64"
    build = artifacts / ("arm64/package" if arm else f"binaries-{arch}")
    suffix = "" if arch == "x86" else "64"
    gui = f"ConEmu{suffix}.exe"
    add_binary(files, build, gui, gui, arch)

    for runtime in ([arch] if arm else ["x86", "x64"]):
        root = build if arm else artifacts / f"binaries-{runtime}"
        bits = "" if runtime == "x86" else "64"
        names = [f"ConEmuC{bits}.exe", f"ConEmuCD{bits}.dll"]
        if not arm:
            names.append(f"ConEmuHk{bits}.dll")
        for name in names:
            path = f"ConEmu/{name}"
            add_binary(files, root, path, path, runtime)

    plugin_source = "Plugins/ConEmu" if arm else "plugins/ConEmu"
    extension = {"arm64": ".arm64.dll", "x86": ".dll", "x64": ".x64.dll"}[arch]
    for folder, plugin in PLUGINS.items():
        name = folder + plugin + extension
        add_binary(files, build, f"{plugin_source}/{name}", f"Plugins/ConEmu/{name}", arch)
    for decoder in ("gdi+", "ico", "pe"):
        name = f"Thumbs/{decoder}.t{'32' if arch == 'x86' else '64'}"
        add_binary(files, build, f"{plugin_source}/{name}", f"Plugins/ConEmu/{name}", arch)

    for name in ("ConEmu.l10n", "License.txt", "Scripts/ConEmu.NoHooks.bash", "Scripts/ConEmu.NoHooks.psm1"):
        files[f"ConEmu/{name}"] = Path("Release/ConEmu") / name
    files["ConEmu/Unicode-LICENSE.txt"] = Path("src/common/Unicode-LICENSE.txt")
    resources = Path("Release/plugins/ConEmu")
    for pattern in ("**/*.lng", "Background/Background.xml", "Background/img/*.png"):
        for path in resources.glob(pattern):
            files[f"Plugins/ConEmu/{path.relative_to(resources).as_posix()}"] = path
    for name in ("ARM64-BUILD.md", "CONPTY.md", "FAR-SCROLLBACK.md", "SECURITY-CHANGES.txt"):
        files[name] = Path(name)

    archive = output / f"ConEmu-{sha[:7]}-{arch}.zip"
    with ZipFile(archive, "w", ZIP_DEFLATED) as zip_file:
        for name, path in sorted(files.items()):
            zip_file.write(path, name)
        zip_file.writestr("SOURCE-COMMIT.txt", f"https://github.com/Sim-md/ConEmu/commit/{sha}\n")
    with ZipFile(archive) as zip_file:
        if zip_file.testzip() is not None:
            raise ValueError(f"{archive}: ZIP integrity check failed")
    return archive


def main():
    artifacts, output = map(Path, sys.argv[1:3])
    sha = sys.argv[3]
    output.mkdir()
    archives = [package(artifacts, output, sha, arch) for arch in MACHINES]
    checksums = [f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.name}" for path in archives]
    (output / "SHA256SUMS.txt").write_text("\n".join(checksums) + "\n", encoding="ascii")


if __name__ == "__main__":
    main()
