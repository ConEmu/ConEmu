# Building ConEmu for Windows ARM64

Use a POSIX environment such as WSL with GNU Make, standard shell utilities,
and the LLVM-MinGW cross toolchain on `PATH`. Use a checkout and build path
without spaces for the existing GNU Make rules.

```sh
git submodule update --init --recursive
export PATH="/path/to/llvm-mingw/bin:$PATH"
sh build-arm64.sh
```

The script builds the GUI, console executable, server DLL, and the Far plugins.
The package includes ConEmuBg, ConEmuLn, and ConEmuTh with its GDI+, ICO, and
PE thumbnail modules. Keep their subdirectories and resource files together.
The files to install are under `Release-arm64/package`. Set `BUILD_DIR` to an
absolute path to change the build directory or `JOBS` to change the number of
parallel Make jobs.

## Run with Far Manager

1. Copy the contents of `Release-arm64/package` into the ARM64 Far Manager
   directory, preserving the `ConEmu` and `Plugins/ConEmu` subdirectories.
   Use the ARM64 Far distribution with its LuaMacro plugin for panel mouse
   handling.
2. Start `ConEmu64.exe -run Far.exe` from that directory. In ConEmu settings
   under **Far Manager**, enable **LDrag** and **Drop & Confirmation**.
3. To copy from Explorer, hold Ctrl while dropping a file on a Far file panel.
   The copy should appear in that panel's directory.
4. To copy from Far, start with the left mouse button without modifiers when
   using the default left-drag binding. Hold Ctrl after the drag starts and
   drop in an Explorer folder.

The ARM64 build disables injection and uses the Far plugin's IPC for panel
information and drag operations. Features that require `ConEmuHk` are
unavailable in this build.

## Explicit shell integration without ConEmuHk

Import the bundled module from an existing PowerShell tab in ConEmu:

```powershell
Import-Module "$env:ConEmuBaseDir\Scripts\ConEmu.NoHooks.psm1"
New-ConEmuConsole -Command cmd.exe -Arguments '/k echo Hello'
New-ConEmuConsole -Command cmd.exe -Options 'new_console:s50H'
Set-ConEmuTabTitle 'Build'
Set-ConEmuProgress -State Running -Value 50
Set-ConEmuProgress -State None
Sync-ConEmuDirectory
```

These commands use the existing ConEmuC IPC interface. `Sync-ConEmuDirectory`
can be called from a PowerShell profile's existing `prompt` function to report
the directory before each prompt. It skips PowerShell virtual providers.
The module does not change the profile or prompt automatically.

`ConEmuC64 /Echo` and `/Type` use Windows virtual terminal processing
when the hook output functions are unavailable. The console output mode is
restored after each call. Redirected output and `/Echo -r` retain their existing
behavior. This provides native Windows ANSI handling for these explicit output
commands. Full terminal sessions use the separate ConPTY path described below.

## ConPTY sessions

Windows 10 version 1809 or later is required. The ConPTY APIs are loaded at
runtime. Start a shell explicitly through the matching console helper:

```cmd
ConEmu64.exe -NoSingle -run ConEmu\ConEmuC64.exe /PTY powershell.exe -NoLogo
```

From an existing PowerShell console, the bundled module also provides:

```powershell
Start-ConEmuPty -CommandLine 'cmd.exe /d'
```

The child receives a Windows pseudoconsole. A dedicated reader decodes its UTF-8
and terminal sequences with the existing server-side ANSI parser. The renderer
publishes characters through the console buffer and full-history RGB/link
metadata through a versioned per-console shared mapping. The GUI enables RGB annotations for the lifetime of this session even
when the profile's general TrueColor option is disabled. Use the GUI and server
DLL from the same package.

The session supports SGR colors including xterm-256 and 24-bit foreground and
background, cursor movement, erasing, primary/alternate screens, terminal query
responses and window resizing. Supported OSC operations include window titles
(0/2), tab titles (9;3), progress (9;4), and directory reporting (7 and 9;9).
OSC macro execution retains the existing ANSI execution permission checks.
The legacy OSC wait-for-key command (9;5) is ignored in a ConPTY session.

The input and output pipes are serviced independently. The child process tree
belongs to a job that is closed when the session ends. Shutdown drains the final
output, restores the caller's input mode, code page, cursor visibility and title,
and clears the session's RGB annotations.

The primary screen retains up to 2000 history rows, also bounded by a 262144-cell
frame capacity. ConEmu's scrollbar and Scroll macro access these rows with their
RGB attributes. CSI 3 J clears history; the alternate screen has separate contents
and no primary-screen history. Changing the width reflows soft-wrapped primary-screen paragraphs, including retained history, without joining explicit newlines. Alternate-screen grids and restricted scroll regions retain application-controlled layout.

OSC 8 hyperlinks support BEL and ST terminators, fragmented input, and labels
separate from their target URI. Closing the link stops applying it to new text;
SGR reset leaves the active link intact. Links are underlined and use the existing
ConEmu hyperlink setting and modifier-click binding (Far Manager / goto editor).
Targets are limited to HTTP(S), FTP, mailto and local file URIs. Remote file shares
and executable URI schemes are ignored. Resource limits are 2048 UTF-16 units per
URI, 4096 distinct targets and a 1M-unit URI arena; old unreferenced targets are
reclaimed when necessary. Optional OSC 8 id grouping is not interpreted.
Metadata is active only while the ConPTY session is running. HTML copying retains
colors and underline, but does not embed OSC 8 targets as HTML anchors.

This is an opt-in preview. Unicode clusters use Unicode 17 UAX #29 boundaries,
including combining marks, supplementary characters, ZWJ emoji, skin modifiers
and regional-indicator pairs. The shared protocol is version 2; GUI and server
must match. Up to 8192 distinct clusters of 64 UTF-16 units are retained. The GUI
uses Uniscribe with font fallback and monochrome Segoe UI Emoji; color emoji and
universal font coverage are not provided. Some Windows fonts show flags as
letter pairs. Copying plain text and HTML preserves original cluster strings.
Applications that replace or split characters before writing them cannot have
their original strings reconstructed by the terminal.

DEC origin mode, insert mode, autowrap, scroll margins and combined private mode
sequences are supported. Real Git-for-Windows Vim and less were exercised on
Windows ARM64 (the Git tools themselves run under x64 emulation). Full
compatibility with every fullscreen application is not guaranteed.
Far plugins that use console mappings should be exercised through the direct
Far launch path rather than inside a nested pseudoconsole.

### Regression checks on Windows ARM64

Build `src/UnitTests/ConPty/ConPtyChild.cpp` and
`src/UnitTests/ConPty/ConPtySession_test.cpp` as standalone Unicode console
executables. With LLVM-MinGW, use `aarch64-w64-mingw32-clang++ -static -municode`
for each source. Run:

```cmd
ConPtySession_test.exe C:\test\ConEmu\ConEmuC64.exe C:\test\ConPtyChild.exe C:\test\result
```

The last argument is a disposable result-file prefix. The test creates a hidden
console and checks ANSI, fragmented UTF-8, CJK width, alternate-screen return,
OSC titles, keyboard input, resize, terminal reports, failed startup, final-output
draining, exit codes and restoration of console state. Its report is written to
`C:\test\result.report`.

Build `ConPtyHistoryChild.cpp` and `ConPtyHistory_test.cpp` in the same directory
as standalone Unicode console executables and run:

```cmd
ConPtyHistory_test.exe C:\test\ConEmu\ConEmuC64.exe C:\test\ConPtyHistoryChild.exe C:\test\history
```

Use a fresh result prefix for each history test run. It checks old/new output,
TrueColor in history, OSC 8 ST/BEL parsing, labels, closing, SGR reset, URI policy,
bounded history, alternate-screen isolation and CSI 3 J. The native ARM64 GUI was
also checked through IPC Scroll and formatted Copy exports at the top and bottom,
with Hooks=OFF and no ConEmuHk in the test server's loaded modules. Link resolution
was tested without opening the browser or following external URLs.

### Explicit shell integration (no profile changes)

For PowerShell 5.1 or 7, inside a `/PTY` session:

```powershell
Import-Module C:\ConEmu\ConEmu\Scripts\ConEmu.NoHooks.psm1
Enable-ConEmuShellIntegration -UpdateTitle
# To undo:
Disable-ConEmuShellIntegration
```

`Start-ConEmuIntegratedPowerShell -UpdateTitle` starts a new integrated ConPTY
PowerShell from the module. Integration preserves the original prompt and
PSReadLine history callback, reports CWD with OSC 7, prompt boundaries with
OSC 133, command completion/status and progress with OSC 9;4. Disabling or
unloading the module restores the original callbacks. No injection is involved.

For an interactive Bash session, explicitly source `ConEmu.NoHooks.bash` and run
`conemu_shell_enable`; undo with `conemu_shell_disable`. Existing PS1 and
PROMPT_COMMAND values (string or array) are restored. Bash reports prompt,
completion status and CWD; it does not install a DEBUG trap or report command-start
progress. Git Bash paths use cygpath; WSL paths use file://wsl.localhost URIs.

### Advanced validation

`src/UnitTests/ConPty` contains standalone model, Unicode conformance,
reflow-bridge and real fullscreen fixtures. Model tests link ConData.cpp,
CEStr.cpp, wcwidth.cpp and Memory.cpp with function/data section garbage
collection; other native fixtures use `-static -municode`. Fullscreen fixtures
expect Git for Windows in `C:\Program Files\Git` and operate only on disposable
files. Launch native integration tests with permission to create pipes and their
own hidden console. A restricted sandbox may reject ConPTY CreatePipe.

Unicode data is pinned to 17.0.0 and licensed under Unicode License V3
(`ConEmu/Unicode-LICENSE.txt`). `generate_grapheme_data.py` regenerates the interval
header from the official Unicode data files. `UnicodeGrapheme_test.cpp` checks
all 766 official test cases (1990 boundaries). Rendering and GUI-copy checks are
separate from boundary conformance; successful parsing alone does not establish
font rendering quality.

### Standalone history test host

The history fixture explicitly launches a hidden classic `conhost.exe` and then
runs its checks in that console. A plain `AllocConsole` from an outer terminal or
pseudoconsole is insufficient on this Windows host: resize requests can succeed
while the exposed buffer remains viewport-sized (25 rows). Reading it produced
empty initial history and false failures for old/new rows, the BEL hyperlink
label and the unsafe-URI label, despite valid ConPTY frame metadata.

The test still reads actual console characters and verifies physical scrollback;
it does not substitute metadata for text or skip these assertions. It also checks
the loaded server modules for absence of ConEmuHk. Because conhost's exit code is
not necessarily the hosted program's exit code, the fixture returns its result
through a fresh `<prefix>.exit` file. Missing results fail the launcher. A negative
control with an absent helper verifies that exit code 4 is propagated.

On Windows ARM64 the corrected fixture passed all 21 checks in three consecutive
runs. Actual ConEmu GUI checks separately passed scrolling, exact RGB, Unicode
plain/HTML copy, alternate/main restoration, history clearing, Hooks OFF and OSC7
working-directory reporting. No terminal parser or history data-model change was
needed to resolve the three discrepancies.
