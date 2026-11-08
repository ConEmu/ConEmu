# Hook-free ConPTY sessions

This implementation is shared source for Windows x86, x64 and ARM64. It lives in
the same pull-request branch as the Windows theme and ARM64 Far changes. The
branch name `conemu-arm64-nohooks` is historical; it is not an architecture guard.
ConPTY runtime validation and ready-to-run packages are available for ARM64,
x86 and x64. Intel binaries were tested under Windows ARM64 emulation; physical
x86/x64 Windows systems were not available.

## Starting a session

Windows 10 version 1809 or newer is required. From a matching ConEmu package:

```cmd
ConEmu64.exe -NoSingle -run ConEmu\ConEmuC64.exe /PTY powershell.exe -NoLogo
```

Use ConEmu.exe and ConEmuC.exe for x86 builds. The new session is opt-in; existing
Far tasks and shell profiles are not rewritten. Keep GUI and server DLL from the
same package: the Unicode/history shared-frame protocol is version 2.

## Terminal behavior

- ANSI, xterm-256 and TrueColor foreground/background, cursor movement, erase,
  terminal reports, resize and primary/alternate screens.
- Up to 2000 primary-screen history rows, also bounded by 262144 cells. Width
  changes reflow soft-wrapped paragraphs without joining explicit newlines.
  Alternate-screen grids and restricted scroll regions remain app-controlled.
- Unicode 17 grapheme boundaries, combining marks, supplementary characters,
  ZWJ emoji, skin modifiers and regional-indicator pairs. Whole-cluster drawing
  uses Uniscribe and font fallback; copying preserves original strings.
- DEC origin mode, insert mode, autowrap, scroll margins and combined private-mode
  sequences for fullscreen applications.
- OSC titles, directory and progress reporting; OSC 8 hyperlinks with BEL/ST
  terminators, separate labels and an allowed-URI policy. The existing hyperlink
  modifier-click setting is used. HTML copying preserves color/underline, not
  hyperlink targets as anchors.

No ConEmuHk is injected into these ConPTY sessions. This does not reproduce all
legacy hook-based interception features, and Far plugins should use a direct Far
launch rather than a nested pseudoconsole.

## Explicit shell integration

Inside a ConPTY PowerShell 5.1 or 7 session:

```powershell
Import-Module C:\ConEmu\ConEmu\Scripts\ConEmu.NoHooks.psm1
Enable-ConEmuShellIntegration -UpdateTitle
# Restore the original prompt and PSReadLine history handler:
Disable-ConEmuShellIntegration
```

The module also exposes `Start-ConEmuIntegratedPowerShell` and IPC helpers for
console creation, title, progress and CWD. Integration reports OSC 7 CWD,
OSC 133 prompt/command boundaries and completion, and OSC 9;4 progress. Removing
the module restores the saved callbacks. No profile files are modified.

For interactive Bash, source `ConEmu.NoHooks.bash`, then call
`conemu_shell_enable`; undo with `conemu_shell_disable`. Existing PS1 and string
or array PROMPT_COMMAND are restored. Bash reports prompt/completion/CWD; it does
not install a DEBUG trap for command-start progress.

## Validation and limits

The native ARM64 build was checked on Windows ARM64 without loaded ConEmuHk.
History: 21 assertions passed in three consecutive runs, plus an installed-copy
verification. The fixture explicitly starts a hidden classic conhost so the
outer terminal cannot substitute a viewport-only pseudoconsole. It reads actual
console text and checks physical scrollback; it does not replace text assertions
with metadata. A separate result file propagates the hosted test's exit code,
with an absent-helper negative control.

Other checks cover GUI scrolling and RGB, Unicode plain/HTML copying and frozen
selection, resize/reflow, shutdown and child jobs, OSC 8, PowerShell 5.1/7, Bash,
and real Git-for-Windows Vim/less. The Git applications themselves are x64
binaries under ARM64 emulation. Unicode boundary conformance passes all 766
Unicode 17 test cases (1990 boundaries). These checks do not establish universal
fullscreen-application or font compatibility.

Emoji are monochrome; Windows fonts can show flags as letter pairs. Limits are
8192 distinct clusters and 64 UTF-16 units per cluster. Applications that alter
Unicode before writing cannot have their original text reconstructed. Unicode
data is distributed with `Unicode-LICENSE.txt` (Unicode License V3).

See [ARM64-BUILD.md](ARM64-BUILD.md) for the build procedure and standalone test
sources in `src/UnitTests/ConPty`. The ConPTY sources are included by the common
GNU Make and MSVC server projects. Intel builds use MSVC v142. The x86/x64 ZIPs
include both helper/server/hook bitnesses for mixed-bitness Windows. The
PowerShell module chooses the helper appropriate to the operating system.

## Ordinary launches and hook compatibility

`/PTY` is opt-in. Without it, ordinary console launches retain the existing
ConEmuHk path when injection is enabled in the user's profile. The Intel ZIPs
include ConEmuHk.dll and ConEmuHk64.dll and do not ship a profile that disables
injection. In both x86 and x64 GUI tests, the same profile kept injection enabled:
the normal-launch probe found its hook loaded, and the `/PTY` probe found neither
hook DLL in its process. Thus the new mode does not globally turn legacy hooks
off. Hooks can only be injected into a supported matching x86/x64 target; they do
not turn into ARM64 hooks when running Intel builds under emulation.

Both Intel builds passed standalone ANSI/input/lifecycle, history/OSC8,
Unicode/reflow, real Vim/less, GUI RGB/history/frozen Unicode copy and PowerShell
5.1/7 checks. The standard history fixture also inspects loaded server modules to
verify ConEmuHk is absent. Unicode conformance passes 766 cases / 1990 boundaries
on each architecture. The tests revealed and fixed an existing factory mutex
ownership bug (`unique_lock::release` abandoned a locked mutex), which MSVC
reported as a shutdown exception. The shared fix was rechecked on ARM64.

To reproduce the core Intel build after `git submodule update --init --recursive`,
run from a Visual Studio developer PowerShell with the v142 C++ tools and a
Windows 10 SDK installed:

```powershell
foreach ($platform in 'Win32', 'x64') {
  MSBuild.exe src/CE.sln '/t:ConEmu;ConEmuC;ConEmuCD;ConEmuHk' /m:4 `
    /p:Configuration=Release /p:Platform=$platform `
    /p:PlatformToolset=v142 /p:WindowsTargetPlatformVersion=10.0
  if ($LASTEXITCODE) { throw "Build failed: $platform" }
}
```

The optional Far plugins/thumbnail modules in these ZIPs retain the previously
validated builds; their source did not change in this update. Settings, profile
files and the user's default launch mode are not replaced by the archives.
