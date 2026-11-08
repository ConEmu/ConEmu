# Hook-free ConPTY sessions

This implementation is shared source for Windows x86, x64 and ARM64. It lives in
the same pull-request branch as the Windows theme and ARM64 Far changes. The
branch name `conemu-arm64-nohooks` is historical; it is not an architecture guard.
The latest ConPTY runtime validation and ready-to-run package are ARM64 only.
The earlier x86/x64 ZIPs contain the shared theme changes but predate ConPTY.

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
GNU Make and MSVC server projects. New ConPTY builds for x86/x64 have not yet been
built or runtime-validated; the downloadable Intel packages are explicitly
labelled as earlier, without ConPTY.
