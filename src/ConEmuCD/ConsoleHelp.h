
/*
Copyright (c) 2009-present Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifdef _DEBUG
#define _DBGHLP(x) x
#else
#define _DBGHLP(x)
#endif

// Used via SetWindowText && PrintBuffer. Need not escape of "%".
#define pConsoleHelp \
		L"This is a console part of ConEmu product.\r\n" \
		L"Usage: ConEmuC [switches] -ROOT <program with arguments, far.exe for example>\r\n" \
		L"   or: ConEmuC [switches] [-STD] [-U | -A] [-Async | -Fork] -C <command line>\r\n" \
		L"   or: ConEmuC -AUTOATTACH [-GHWND=NEW|<HWND>]\r\n" \
		L"   or: ConEmuC -ATTACH -NOCMD\r\n" \
		L"   or: ConEmuC -ATTACH [-GHWND=NEW|<HWND>] -[FAR|CON|TRM]PID=<PID>\r\n" \
		L"   or: ConEmuC [-SILENT] [-USEEXPORT] -GUIMACRO[:PID|0xHWND][:T<tab>][:S<split>] <GuiMacro command>\r\n" \
		L"   or: ConEmuC -IsConEmu | -IsAnsi | -IsAdmin | -IsRedirect | -IsTerm\r\n" \
		L"   or: ConEmuC -DEBUGPID=<PID>[,<PID2>[,...]] [-DUMP | -MINI | -AUTOMINI | -FULL]\r\n" \
		L"   or: ConEmuC -DEBUGEXE | -DEBUGTREE <CommandLine>\r\n" \
		L"   or: ConEmuC [-SILENT] -EXPORT[=CON|GUI|ALL] [Var1 [Var2 [...]]]\r\n" \
		L"   or: ConEmuC -ECHO | -TYPE [...]\r\n" \
		L"   or: ConEmuC -StoreCWD [\"dir\"]\r\n" \
		L"   or: ConEmuC -PTY program [arguments] (Windows 10 1809+)\r\n" \
		L"   or: ConEmuC -download [-login <name> -password <pwd>]\r\n" \
		L"               [-proxy <address:port> [-proxylogin <name> -proxypassword <pwd>]]\r\n" \
		L"               [-async Y|N] [-timeout[1|2] <ms>] [-agent <name>] [-debug] [-nolog]\r\n" \
		L"               \"full_url_to_file\" [\"local_path_name\" | - ]\r\n" \
_DBGHLP(L"   or: ConEmuC -REGCONFONT=<FontName> -> RegisterConsoleFontHKLM\r\n") \
		L"   or: ConEmuC -?\r\n" \
		L"Switches and commands:\r\n" \
		L"     -[NO]CONFIRM    - [don't] confirm closing console on program termination\r\n" \
		L"     -AUTOATTACH     - asynchronous attach to ConEmu GUI (for batches)\r\n" \
		L"                       always returns 0 as errorlevel on exit (Issue 1003)\r\n" \
		L"     -ATTACH         - auto attach to ConEmu GUI\r\n" \
		L"       -GHWND        - you may force new ConEmu window or attach to specific one\r\n" \
		L"       -NOCMD        - attach current (existing) console to GUI\r\n" \
		L"       -PID=<PID>    - use <PID> as root process\r\n" \
		L"       -FARPID=<PID> - for internal use from Far plugin\r\n" \
		L"       -CONPID=<PID> - 'soft' mode, don't inject ConEmuHk into <PID>\r\n" \
		L"       -TRMPID=<PID> - called from *.vshost.exe when 'AllocConsole' just created\r\n" \
		L"     -B{W|H|Z}       - define window width, height and buffer height\r\n" \
		L"     -F{N|W|H}       - define console font name, width, height\r\n" \
		L"     -GuiMacro ...   - https://conemu.github.io/en/GuiMacro.html\r\n" \
		L"     -Export[...]    - https://conemu.github.io/en/ExportEnvVar.html\r\n" \
		L"     -IsAdmin        - returns 1 as errorlevel if current user has elevated privileges, 2 if not\r\n" \
		L"     -IsAnsi         - returns 1 as errorlevel if ANSI are processed, 2 if not\r\n" \
		L"     -IsConEmu       - returns 1 as errorlevel if running in ConEmu tab, 2 if not\r\n" \
		L"     -IsRedirect     - returns 1 as errorlevel if CONOUT is redirected, 2 if not\r\n" \
		L"     -IsTerm         - returns 1 as errorlevel if running in telnet, 2 if not\r\n" \
		L"     -Log[N]         - create (debug) log file, N is number from 0 to 3\r\n" \
		L"     -Echo | -Type   - https://conemu.github.io/en/ConEmuC.html#EchoAndType\r\n" \
		L"     -StoreCWD       - inform ConEmu about current working directory\r\n" \
_DBGHLP(L"     -- following switches are visible in debug builds only but available in release too--\r\n") \
_DBGHLP(L"     -CINMODE==<hex:gnConsoleModeFlags>\r\n") \
_DBGHLP(L"     -CREATECON -> used internally for hidden console creation\r\n") \
_DBGHLP(L"     -DOSBOX -> use DosBox\r\n") \
_DBGHLP(L"     -FN=<ConFontName> -FH=<FontHeight> -FW=<FontWidth>\r\n") \
_DBGHLP(L"     -GHWND=<ConEmu.exe HWND>\r\n") \
_DBGHLP(L"     -GID=<ConEmu.exe PID>\r\n") \
_DBGHLP(L"     -HIDE -> gbForceHideConWnd=TRUE\r\n") \
_DBGHLP(L"     -INJECT=PID{10}\r\n") \
_DBGHLP(L"     -SETHOOKS=HP{16},PID{10},HT{16},TID{10},ForceGui\r\n") \
        L"     -Args <Command line> -> printf NextArg's\r\n" \
        L"     -CheckUnicode -> Some unicode printouts\r\n" \
        L"     -ConInfo -> Current console tech info\r\n" \
        L"     -OsVerInfo -> Show OS information, returns OsVer as errorlevel\r\n" \
        L"     -ErrorLevel <number> -> exit with specified errorlevel\r\n" \
        L"     -Result -> print errorlevel of -C <command>\r\n" \
		L"\r\n"

#define pNewConsoleHelp \
		L"When you run application from ConEmu console, you may use one or more\r\n" \
		L"  Switch: -new_console[:switches]\r\n" \
		L"     a - RunAs shell verb (as Admin on Vista+, user/pwd in Win2k and WinXP)\r\n" \
_DBGHLP(L"     A - run under highest available account\r\n") \
		L"     b - Create background tab\r\n" \
		L"     c - force enable ‘Press Enter or Esc to close console’ confirmation\r\n" \
		L"         c0 - wait for Enter/Esc silently\r\n" \
		L"         c1 - don't close console automatically, even by Enter/Esc\r\n" \
		L"     C:\"<iconfile>\" - specify icon used in tab\r\n" \
		L"     d:\"<dir>\" - specify working directory\r\n" \
		L"     e - use credentials for network resources only\r\n" \
		L"     f - force starting console active, useful when starting several consoles simultaneously\r\n" \
		L"     h<height> - i.e., h0 - turn buffer off, h9999 - switch to 9999 lines\r\n" \
		L"     i - don't inject ConEmuHk into starting process\r\n" \
		L"     I - (GuiMacro only) forces inheriting of root process contents, like \"Duplicate root\"\r\n" \
_DBGHLP(L"     l - lock console size, do not sync it to ConEmu window\r\n") \
        L"     L:\"<dir>\\\" - the directory for ANSI log files\r\n" \
        L"     L:\"<logfile>\" - the full path for ANSI log file\r\n" \
		L"     m:/mnt - defines ‘/mnt’ prefix for Unix-path conversion, m:\"\" - no prefix\r\n" \
		L"     n - disable ‘Press Enter or Esc to close console’ confirmation\r\n" \
		L"     o - don't enable ‘Long console output’ when starting command from Far Manager\r\n" \
		L"     p[N] - pty modes, N - bitmask: 1 - XTermKeys, 2 - BrPaste, 4 - AppCursorKeys; default is 5 (1+4)\r\n" \
		L"     P:\"<palettename>\" - set fixed palette for tab\r\n" \
		L"     r - run as restricted user\r\n" \
		L"     R - force start hooks server in the parent process\r\n" \
		L"     s[<SplitTab>T][<Percents>](H|V)\r\n" \
		L"       - https://conemu.github.io/en/SplitScreen.html\r\n" \
		L"     t:\"<tabname>\" - set title for new created tab\r\n" \
_DBGHLP(L"     x<width>, y<height> - change size of visible area, use with 'l'\r\n") \
		L"     u - ConEmu choose user dialog\r\n" \
		L"     u:\"<user>:<pwd>\" - specify user/pwd in args\r\n" \
		L"     w[0] - Enable [disable] ‘Overwrite’ mode in command prompt by default\r\n" \
		L"     W:\"<tabwallpaper>\" - use specified wallpaper for the tab\r\n" \
		L"     z - Don't use ‘Default terminal’ feature for this command\r\n" \
		L"\r\n" \
		L"  Warning: Option ‘Inject ConEmuHk’ must be enabled in ConEmu settings!\r\n" \
		L"  Example: dir -new_console:bh9999c:d:\"C:\\\":P:\"^<PowerShell^>\" c:\\Users /s\r\n" \
		L"  Note: Some switches may be used in the similar \"-cur_console:...\"\r\n"


#define pDosBoxHelp \
		L"Starting DosBox, You may use following default combinations in DosBox window\r\n" \
		L"ALT-ENTER     Switch to full screen and back.\r\n" \
		L"ALT-PAUSE     Pause emulation (hit ALT-PAUSE again to continue).\r\n" \
		L"CTRL-F1       Start the keymapper.\r\n" \
		L"CTRL-F4       Change between mounted floppy/CD images. Update directory cache \r\n" \
		L"              for all drives.\r\n" \
		L"CTRL-ALT-F5   Start/Stop creating a movie of the screen. (avi video capturing)\r\n" \
		L"CTRL-F5       Save a screenshot. (PNG format)\r\n" \
		L"CTRL-F6       Start/Stop recording sound output to a wave file.\r\n" \
		L"CTRL-ALT-F7   Start/Stop recording of OPL commands. (DRO format)\r\n" \
		L"CTRL-ALT-F8   Start/Stop the recording of raw MIDI commands.\r\n" \
		L"CTRL-F7       Decrease frameskip.\r\n" \
		L"CTRL-F8       Increase frameskip.\r\n" \
		L"CTRL-F9       Kill DOSBox.\r\n" \
		L"CTRL-F10      Capture/Release the mouse.\r\n" \
		L"CTRL-F11      Slow down emulation (Decrease DOSBox Cycles).\r\n" \
		L"CTRL-F12      Speed up emulation (Increase DOSBox Cycles).\r\n" \
		L"ALT-F12       Unlock speed (turbo button/fast forward).\r\n" \
		L"F11, ALT-F11  (machine=cga) change tint in NTSC output modes\r\n" \
		L"F11           (machine=hercules) cycle through amber, green, white colouring\r\n"
