
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

#pragma once

#include "../ConEmuCD/ConsoleHelp.h"

#define pCmdLine \
	L"Command line examples\r\n" \
	VCGCCTEST(L"––––––––––––––––––––––\r\n",L"----------------------\r\n") \
	L"ConEmu.exe -run Far.exe -w\r\n" \
	L"ConEmu.exe -font \"Consolas\" -FontSize 16 -bufferheight 32766 -run powershell\r\n" \
	L"ConEmu.exe -config \"Hiew\" -run \"C:\\Tools\\HIEW32.EXE\"\r\n" \
	L"ConEmu.exe -run {Shells}\r\n" \
	L"ConEmu.exe -nosingle -runlist cmd ||| cmd -new_console:sV ||| cmd -new_console:sH\r\n" \
	L"ConEmu.exe -noupdate -tsa -min -icon \"cmd.exe\" -run cmd /c dir c:\\ /s\r\n" \
	L"\r\n"\
	L"Command line switches are case insensitive.\r\n" \
	L"You may use either ‘/style’ or ‘-style’ switches.\r\n" \
	L"\r\n"\
	L"-? - This help screen.\r\n" \
	L"-Config <configname> - Use alternative named configuration.\r\n" \
	L"-Dir <workdir> - Set startup directory for ConEmu and consoles.\r\n" \
	L"-Here - Force using of ‘inherited’ startup directory. An alternative to ‘-Dir’ switch.\r\n" \
	L"-FS | -Max | -Nor | -Min - (Full screen), (Maximized), (Normal) or (Minimized) mode.\r\n" \
	L"-TSA - Override (enable) minimize to taskbar status area.\r\n" \
	L"-MinTSA - start minimized in taskbar status area, hide to TSA after console close.\r\n" \
	L"-StartTSA - start minimized in taskbar status area, exit after console close.\r\n" \
	L"-Detached - start ConEmu without consoles.\r\n" \
	L"-NoAutoClose - don't close ConEmu window automatically with last tab.\r\n" \
	L"-Icon <file> - Take icon from file (exe, dll, ico).\r\n" \
	L"-Title <title> - Set a fixed ConEmu window title. Environment variables are supported.\r\n" \
	L"   Use %s for the current title of the active console, e.g. -Title \"Cmder: %s\".\r\n" \
	L"-Multi | -NoMulti - Enable or disable multiconsole features.\r\n" \
	L"-Single - New console will be started in new tab of existing ConEmu.\r\n" \
	L"-NoSingle - Force new ConEmu window even if single mode is selected in the Settings.\r\n" \
	L"-ShowHide | -ShowHideTSA - Works like \"Minimize/Restore\" global hotkey.\r\n" \
	L"-NoAutoEnvReload - Disable auto reload of environment variables from registry.\r\n" \
	L"-NoCascade - Disable ‘Cascade’ option may be set in the Settings.\r\n" \
	L"-NoDefTerm - Don't start initialization procedure for setting up ConEmu as default terminal.\r\n" \
	L"-NoHooksWarn - Don't show ‘hooks are detected’ warning in console.\r\n" \
	L"-NoKeyHooks - Disable SetWindowsHookEx and global hotkeys.\r\n" \
	L"-NoMacro - Disable GuiMacro hotkeys.\r\n" \
	L"-NoHotkey - Disable all hotkeys.\r\n" \
	L"-NoRegFonts - Disable auto register fonts (font files from ConEmu folder).\r\n" \
	L"-NoUpdate - Disable automatic checking for updates on startup\r\n" \
	L"-NoCloseConfirm - Disable confirmation of ConEmu's window closing\r\n" \
	L"-CT[0|1] - Anti-aliasing: -ct0 - off, -ct1 - standard, -ct - cleartype.\r\n" \
	L"-Font <fontname> - Specify the font name.\r\n" \
	L"-FontSize <fontsize> - Specify the font size.\r\n" \
	L"-FontFile <fontfilename> - Loads font from file (multiple pairs allowed).\r\n" \
	L"-FontDir <fontfolder> - Loads all fonts from folder (multiple pairs allowed).\r\n" \
	L"-BufferHeight <lines> - Set console buffer height.\r\n" \
	L"-Wnd{X|Y|W|H} <value> - Set window position and size.\r\n" \
	L"-Quake | -QuakeAuto | -NoQuake - Change quake style.\r\n" \
	L"-Frame <value> - Set custom ‘Frame width’.\r\n" \
	L"-Monitor <1 | x10001 | \"\\\\.\\DISPLAY1\"> - Place window on the specified monitor.\r\n" \
	L"-Palette <name> - Choose named color palette.\r\n" \
	L"-Theme [<name>] - Use specified Windows theme or ‘DarkMode_Explorer’.\r\n" \
	L"-Log[1|2] - Used to create debug log files.\r\n" \
	L"-Demote -run <command> - Run command de-elevated.\r\n" \
	L"-Bypass -run <command> - Just execute the command detached.\r\n" \
	L"-Reset - Don't load settings from registry/xml.\r\n" \
	L"-UpdateJumpList - Update Windows 7 taskbar jump list.\r\n" \
	L"-LngFile <file> - Use specified l10n file as localization storage.\r\n" \
	L"-Lng <en|ru|de|...> - Use specified language id.\r\n" \
	L"-LoadCfgFile <file> - Use specified xml file as configuration storage.\r\n" \
	L"-SaveCfgFile <file> - Save configuration to the specified xml file.\r\n" \
	L"-LoadRegistry - Use Windows registry as configuration storage.\r\n" \
	L"-Settings - Open settings dialog after ConEmu startup.\r\n" \
	L"-SetDefTerm - Set ConEmu as default terminal, use with \"-Exit\" switch.\r\n" \
	L"-UpdateSrcSet <url> - Force to check version.ini by another url.\r\n" \
	L"-AnsiLog <folder> - Force console output logging into the folder.\r\n" \
_DBGHLP(L"-ZoneId - Try to drop :Zone.Identifier without confirmation.\r\n") \
	L"-Exit - Don't create ConEmu window, exit after actions.\r\n" \
	L"-QuitOnClose - Forces ConEmu window closing with last tab.\r\n" \
	L"-GuiMacro - Execute some GuiMacro after ConEmu window creation.\r\n" \
	/* L"-Attach [PID] - intercept console of specified process\n" */ \
	L"-run <commandline>|@<taskfile>|{taskname} - Command line to start. This must be the last used switch.\r\n" \
	L"\r\n" \
	L"Read more online: https://conemu.github.io/en/CommandLine.html\r\n"

#define pAboutTasks \
	L"You may set up most used shells as ConEmu's ‘Tasks’ (‘Settings’ dialog ‘Tasks’ page).\r\n" \
	L"\r\n" \
	L"Each Task may contain one or more commands, " \
	L"each command will be started in separate ConEmu tab or pane.\r\n" \
	L"Delimit Task Commands with empty lines.\r\n" \
	L"\r\n" \
	L"Mark ‘Run as Admin’ tabs/consoles with '*' prefix.\r\n" \
	L"When Task contains more then one command, you may mark active console tab/pane with '>' prefix.\r\n" \
	L"Example:\r\n" \
	L"   *cmd\r\n" \
	L"   *>cmd -new_console:sV\r\n" \
	L"   cmd -new_console:s1TH\r\n" \
	L"\r\n" \
	L"You may use \"-new_console\" switches to set up advanced properties of each command:\r\n" \
	L"startup directory, palette, wallpaper, tab title, split configuration and so on.\r\n" \
	L"More information here under the ‘-new_console’ label.\r\n" \
	L"\r\n" \
	L"Also, ConEmu can process some directives internally before executing your shell:\r\n" \
	L"   \"set name=long value\"\r\n" \
	L"   set name=value\r\n" \
	L"   chcp <utf8|ansi|oem|#>\r\n" \
	L"   title \"Your shell\"\r\n" \
	L"Example:\r\n" \
	L"   chcp 1251 & set TERM=MSYS & cmd\r\n" \
	L"\r\n" \
	L"Note! ‘title’ is useless with most of shells like cmd, powershell or Far! You need to change title within your shell:\r\n" \
	L"   cmd /k title Your title\r\n" \
	L"   powershell -noexit -command \"$host.UI.RawUI.WindowTitle='Your title'\"\r\n"

#define pAboutContributors \
	L"Thanks to all testers and reporters! You help to make the ConEmu better.\r\n" \
	L"\r\n" \
	L"Code signing certificate\r\n" \
	VCGCCTEST(L"––––––––––––––––––––––\r\n",L"----------------------\r\n") \
	L"Thanks to Certum for Open Source Developer certificate\r\n" \
	L"\r\n" \
	L"Special Thanks\r\n" \
	VCGCCTEST(L"––––––––––––––––––––––\r\n",L"----------------------\r\n") \
	L"Michael Lukashov: bug- and warning-fixes\r\n" \
	L"@ulin0208: Chinese translation\r\n" \
	L"Rick Sladkey: bugfixes\r\n" \
	L"ForNeVeR: some ConEmuBg improvements\r\n" \
	L"thecybershadow: bdf support, Documentation\r\n" \
	L"NightRoman: drawing optimization, BufferHeight and other fixes\r\n" \
	L"dolzenko_: windows switching via GUI tabs\r\n" \
	L"alex_itd: Drag'n'Drop, RightClick, AltEnter\r\n" \
	L"Mors: loading font from file\r\n" \
	L"Grzegorz Kozub: new toolbar images"

// --> lng_AboutTitle
//#define pAboutTitle \
//	L"Console Emulation program (local terminal)"

#define pAboutLicense \
	L"\x00A9 2006-2008 Zoin (based on console emulator by SEt)\r\n" \
	CECOPYRIGHTSTRING_W /*© 2009-2014 ConEmu.Maximus5@gmail.com*/ L"\r\n" \
	L"\r\n" \
	L"Permission to use, copy, modify, and distribute this software for any\r\n" \
	L"purpose with or without fee is hereby granted, provided that the above\r\n" \
	L"copyright notice and this permission notice appear in all copies.\r\n" \
	L"\r\n" \
	L"THE SOFTWARE IS PROVIDED 'AS IS' AND THE AUTHOR DISCLAIMS ALL WARRANTIES\r\n" \
	L"WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF\r\n" \
	L"MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR\r\n" \
	L"ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES\r\n" \
	L"WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN\r\n" \
	L"ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF\r\n" \
	L"OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.\r\n"


#define pAbout \
	L"ConEmu-Maximus5 is a Windows console emulator with tabs, which represents " \
	L"multiple consoles and simple GUI applications as one customizable GUI window " \
	L"with various features.\r\n" \
	L"\r\n" \
	L"You may run any shell of your chose (Tasks are recommended) on ConEmu startup.\r\n" \
	L"\r\n" \
	L"Far Manager (a console program for managing files and archives in Windows)" \
	L"gains a lot of improvements in ConEmu: Drag and Drop, Tabs for editors and so on.\r\n" \
	L"\r\n" \
	L"\x00A9 2006-2008 Zoin (based on console emulator by SEt)\r\n" \
	CECOPYRIGHTSTRING_W /*\x00A9 2009-present ConEmu.Maximus5@gmail.com*/ L"\r\n" \
	L"\r\n" \
	L"Online documentation: " CEWIKIBASE L"TableOfContents.html\r\n" \
	L"\r\n" \
	L"You can show your appreciation and support future development by donating."


#define pConsoleHelpFull \
	pConsoleHelp \
	L"When you run application from ConEmu console, you may use one or more -new_console[:switches]\r\n" \
	L"Read about them on next page\r\n"

#define pNewConsoleHelpFull \
	pNewConsoleHelp


#define pDosBoxHelpFull \
	L"ConEmu supports DosBox, read more online:\r\n" \
	CEWIKIBASE L"DosBox.html\r\n" \
	VCGCCTEST(L"––––––––––––––––––––––\r\n",L"----------------------\r\n") \
	pDosBoxHelp

#define pGuiMacro \
	L"Sort of simple macro language, use them with hotkeys or from your shell CLI.\r\n" \
	L"CLI example: ConEmuC -GuiMacro Rename 0 PoSh\r\n" \
	L"Read more online: " CEWIKIBASE L"GuiMacro.html\r\n" \
	VCGCCTEST(L"––––––––––––––––––––––\r\n",L"----------------------\r\n") \
	L"About([\"<Page>\"])\r\n" \
	L"  - Show ‘About’ dialog with page activated, e.g. ‘About(\"-new_console\")’\r\n" \
	L"AffinityPriority([Affinity,Priority])\r\n" \
	L"  - Change affinity and priority of active console\r\n" \
	L"AltNumber([Base])\r\n" \
	L"  - Base==0: Start Alt+Numbers in ANSI decimal mode\r\n" \
	L"    Base==10: Start Alt+Numbers in OEM decimal mode\r\n" \
	L"    Base==16: Start Alt+Numbers in hexadecimal mode\r\n" \
	L"Attach([<PID>[,<Alternative>]])\r\n" \
	L"  - Attach console or ChildGui by PID\r\n" \
	L"Break([<Event>[,<ProcessGroupId>]])\r\n" \
	L"  - Event==0: Generates a CTRL+C signal\r\n" \
	L"    Event==1: Generates a CTRL+BREAK signal\r\n" \
	L"    More info in GenerateConsoleCtrlEvent WinAPI function\r\n" \
	L"Close(<What>[,<Flags>])\r\n" \
	L"  - close current console (0), without confirmation (0,1)\r\n" \
	L"    terminate active process (1), no confirm (1,1)\r\n" \
	L"    close ConEmu window (2), no confirm (2,1)\r\n" \
	L"    close active tab (3)\r\n" \
	L"    close active group (4)\r\n" \
	L"    close all tabs but active (5), no confirm (5,1)\r\n" \
	L"    close active tab or group (6)\r\n" \
	L"    close all active processes of the active group (7)\r\n" \
	L"    close all tabs (8), no confirm (8,1)\r\n" \
	L"    close all zombies (9), no confirm (9,1)\r\n" \
	L"    terminate all but shell process (10), no confirm (10,1)\r\n" \
	L"    close consoles to the right (11), no confirm (11,1)\r\n" \
	L"Context([<Tab>[,<Split>]])\r\n" \
	L"  - Change macro execution context\r\n" \
	L"    Tab: 1-based tab index\r\n" \
	L"    Split: 1-based split index\r\n" \
	L"Copy(<What>[,<Format>[,\"<File>\"]])\r\n" \
	L"  - Copy active console contents\r\n" \
	L"    What==0: current selection\r\n" \
	L"    What==1: all buffer contents\r\n" \
	L"    What==2: visible area contents\r\n" \
	L"    Format==0: plain text, not formatting\r\n" \
	L"    Format==1: copy HTML format\r\n" \
	L"    Format==2: copy as HTML\r\n" \
	L"    Format==3: copy ANSI sequences\r\n" \
	L"    File: if specified - save to file instead of clipboard\r\n" \
	L"Debug(<Action>)\r\n" \
	L"  - Create debugger console or memory dumps\r\n" \
	L"    Action==0: Debug active process\r\n" \
	L"    Action==1: Active process memory dump\r\n" \
	L"    Action==2: Active process tree memory dump\r\n" \
	L"Detach([<Flags>])\r\n" \
	L"  - Detach active RealConsole from ConEmu\r\n" \
	L"    Flags=1: don't show confirm message\r\n" \
	L"EnvironmentReload\r\n" \
	L"  - Reload environment variables from system registry\r\n" \
	L"EnvironmentList\r\n" \
	L"  - Print actual `name=value` environment pairs\r\n" \
	L"    Variables from ConEmu settings go after system ones\r\n" \
	L"FindEditor(\"<FullEditFileName>\")\r\n" \
	L"FindViewer(\"<FullViewerFileName>\")\r\n" \
	L"FindFarWindow(<WindowType>,\"<WindowTitle>\")\r\n" \
	L"  - Returns \"Found:<index of RealConsole>\", \"Active:<Far window number>\",  \"Blocked\", or \"NotFound\"\r\n" \
	L"     <WindowType> is number from Far Manager enum WINDOWINFO_TYPE\r\n" \
	L"Flash(<Cmd>[,<Flags>[,<Count>]])\r\n" \
	L"  - Allows to flash taskbar icon and/or window caption\r\n" \
	L"    Flash(0) - Stop all flashing\r\n" \
	L"    Flash(0,1) - Simple flashing (see MSDN FlashWindow)\r\n" \
	L"Flash(1,<Flags>,<Count>)\r\n" \
	L"  - Special flashing (see MSDN FlashWindowEx)\r\n" \
	L"    Flags: 0 - stop, 1 - caption, 2 - taskbar, 3 - caption+taskbar, etc.\r\n" \
	L"    Count: the number of times to flash the window\r\n" \
	L"FontSetName(\"<FontFaceName>\"[,FontHeight[,FontWidth]])\r\n" \
	L"  - Change font face name and (optionally) height/width.\r\n" \
	L"FontSetSize(Relative,N)\r\n" \
	L"  - Change font height in the ConEmu window\r\n" \
	L"     Relative==0: N - required font height in points\r\n" \
	L"     Relative==1: N (+-1, +-2) - increase/decrease font height\r\n" \
	L"     Relative==2: N (per cents) - alias for Zoom(N)\r\n" \
	L"     returns - \"OK\", or \"InvalidArg\"\r\n" \
	L"GetInfo(\"<Opt1>\"[,\"<Opt2>\"[,...]])\r\n" \
	L"  - Returns values of some ConEmu environment variables\r\n" \
	L"    Processed in GUI so the result may differ from RealConsole env.vars\r\n" \
	L"    \"PID\": returns %ConEmuPID% and so on\r\n" \
	L"  - Returns additional information about RealConsole\r\n" \
	L"    \"Root\": XML with RootProcess info\r\n" \
	L"    \"ActivePID\",\"CurDir\": as is\r\n" \
	L"GetOption(\"<Name>\")\r\n" \
	L"  - Returns value of some ConEmu options (the set is limited)\r\n" \
	L"GroupInput([<Cmd>])\r\n" \
	L"  - Group keyboard input for visible splits\r\n" \
	L"     Cmd==0: switch mode (default)\r\n" \
	L"     Cmd==1: group\r\n" \
	L"     Cmd==2: un-group\r\n" \
	L"  - Group keyboard input for all consoles\r\n" \
	L"     Cmd==3: switch mode\r\n" \
	L"     Cmd==4: group\r\n" \
	L"     Cmd==5: un-group\r\n" \
	L"  - Group keyboard input for selected consoles\r\n" \
	L"     Cmd==6: toggle group mode on active console\r\n" \
	L"HighlightMouse(<What>[,<Act>])\r\n" \
	L"  - change highlighting in the ACTIVE console only\r\n" \
	L"    What==0: switch off/row/col/row+col/off/...\r\n" \
	L"    What==1: row, What==2: col, What==3: row+col\r\n" \
	L"      Act==0: off, Act==1: on, Act==2: switch (default)\r\n" \
	L"IsConEmu\r\n" \
	L"  - Returns \"Yes\" when console was started under !ConEmu\r\n" \
	L"    Alternative is \"ConEmuC.exe -IsConEmu\" and checking errorlevel\r\n" \
	L"IsConsoleActive\r\n" \
	L"  - Check, is RealConsole active or not, \"Yes\"/\"No\"\r\n" \
	L"IsRealVisible\r\n" \
	L"  - Check, is RealConsole visible or not, \"Yes\"/\"No\"\r\n" \
	L"Keys(\"<Combo1>\"[,\"<Combo2>\"[,...]])\r\n" \
	L"     - Post special keystrokes to the console (AutoHotKey syntax)\r\n" \
	L"       \"[Mod1[Mod2[Mod3]]]Key\"\r\n" \
	L"       Mod: ^ - LCtrl, >^ - RCtrl, ! - LAlt, >! - RAlt, + - Shift\r\n" \
	L"       Example: Keys(\"^_\") will post ‘Ctrl+_’\r\n" \
	L"MsgBox(\"<Text>\",\"<Title>\",<ButtonType>)\r\n" \
	L"  - Show modal GUI MessageBox, returns clicked button name (\"Ok\", \"Cancel\", \"Retry\", etc.),\r\n" \
	L"     ButtonType (number) is from Windows SDK.\r\n" \
	L"Palette([<Cmd>[,\"<NewPalette>\"|<NewPaletteIndex>]])\r\n" \
	L"  - Returns current palette name, change if <Cmd> specified\r\n" \
	L"     Cmd==0: return palette from ConEmu settings\r\n" \
	L"     Cmd==1: change palette in ConEmu settings, returns prev palette\r\n" \
	L"     Cmd==2: return palette from current console\r\n" \
	L"     Cmd==3: change palette in current console, returns prev palette\r\n" \
	L"     NewPalette: palette name (string)\r\n" \
	L"     NewPaletteIndex: 0-based index (number)\r\n" \
	L"Paste(<Cmd>[,\"<Text>\"[,\"<Text2>\"[...]]])\r\n" \
	L"  - When <Text> is omitted - paste from Windows clipboard, otherwise - paste <Text>\r\n" \
	L"     Cmd==0: paste all lines\r\n" \
	L"     Cmd==1: paste first line\r\n" \
	L"     Cmd==2: paste all lines, without confirmations\r\n" \
	L"     Cmd==3: paste first line, without confirmations\r\n" \
	L"     Cmd==4: select and parse file pathname, Text - default\r\n" \
	L"     Cmd==5: select and parse folder pathname, Text - default\r\n" \
	L"     Cmd==6: select and parse cygwin file pathname, Text - default\r\n" \
	L"     Cmd==7: select and parse cygwin folder pathname, Text - default\r\n" \
	L"     Cmd==8: paste path from clipboard converted to POSIX style\r\n" \
	L"     Cmd==9: paste all lines space-separated\r\n" \
	L"     Cmd==10: paste all lines space-separated, without confirmations\r\n" \
	L"PasteExplorerPath (<DoCd>,<SetFocus>)\r\n" \
	L"  - Activate ConEmu and ‘CD’ to last (top in Z-order) Explorer window path\r\n" \
	L"     DoCd: 1 - ‘CD’, 0 - paste path only\r\n" \
	L"     SetFocus: 1 - bring ConEmu to the top, 0 - don't change active window\r\n" \
	L"PasteFile(<Cmd>[,\"<File>\"[,\"<CommentMark>\"]])\r\n" \
	L"  - Paste <File> contents, omit <File> to show selection dialog\r\n" \
	L"     Cmd==0: paste all lines\r\n" \
	L"     Cmd==1: paste first line\r\n" \
	L"     Cmd==2: paste all lines, without confirmations\r\n" \
	L"     Cmd==3: paste first line, without confirmations\r\n" \
	L"     Cmd==9: paste all lines space-separated\r\n" \
	L"     Cmd==10: paste all lines space-separated, without confirmations\r\n" \
	L"     If \"<CommentMark>\" specified - skip all lines starting with prefix\r\n" \
	L"Pause([<Cmd>])\r\n" \
	L"  - Pause current console\r\n" \
	L"     Cmd==0: switch mode (default)\r\n" \
	L"     Cmd==1: pause\r\n" \
	L"     Cmd==2: un-pause\r\n" \
	L"Print([\"<Text>\"[,\"<Text2>\"[...]]])\r\n" \
	L"  - Alias for Paste(2,\"<Text>\")\r\n" \
	L"Progress(<Type>[,<Value>])\r\n" \
	L"  - Set progress state on taskbar and ConEmu title\r\n" \
	L"     Type=0: remove progress\r\n" \
	L"     Type=1: set progress value to <Value> (0-100)\r\n" \
	L"     Type=2: set error state in progress with <Value>\r\n" \
	L"     Type=3: set indeterminate state in progress\r\n" \
	L"     Type=4: set paused state in progress with <Value>\r\n" \
	L"Recreate(<Action>[,<Confirm>[,<AsAdmin>]]), alias \"Create\"\r\n" \
	L"  - Create new tab or restart existing one\r\n" \
	L"     Action: 0 - create tab, 1 - restart tab, 2 - create window\r\n" \
	L"     Confirm: 1 - show ‘Create new console’ dialog, 0 - don't show\r\n" \
	L"     AsAdmin: 1 - start elevated tab\r\n" \
	L"Rename(<Type>,[\"<Title>\"])\r\n" \
	L"  - Rename tab (Type=0) or console title (Type=1)\r\n" \
	L"Scroll(<Type>,<Direction>,<Count=1>)\r\n" \
	L"  - Do buffer scrolling actions\r\n" \
	L"     Type: 0; Value: ‘-1’=Up, ‘+1’=Down\r\n" \
	L"     Type: 1; Value: ‘-1’=PgUp, ‘+1’=PgDown\r\n" \
	L"     Type: 2; Value: ‘-1’=HalfPgUp, ‘+1’=HalfPgDown\r\n" \
	L"     Type: 3; Value: ‘-1’=Top, ‘+1’=Bottom\r\n" \
	L"     Type: 4; No arguments; Go to cursor line\r\n" \
	L"Select(<Type>,<DX>,<DY>,<HE>)\r\n" \
	L"  - Used internally for text selection\r\n" \
	L"     Type: 0 - Text, 1 - Block, 2 - Stop\r\n" \
	L"     DX: select text horizontally: -1/+1\r\n" \
	L"     DY: select text vertically: -1/+1\r\n" \
	L"     HE: to-home(-1)/to-end(+1) with text selection\r\n" \
	L"     HE: word-left(-2)/word-right(+2) with text selection\r\n" \
	L"Select(2)\r\n" \
	L"  - Use to stop selection\r\n" \
	L"SetDpi(<DPI>)\r\n" \
	L"  - Change effective dpi for ConEmu window: 96, 120, 144, 192\r\n" \
	L"SetFocus(<Type>)\r\n" \
	L"  - Try to change active window\r\n" \
	L"     Type: 0 - Set focus into ConEmu window/process (default)\r\n" \
	L"     Type: 1 - Switch focus between ConEmu and ChildGui\r\n" \
	L"     Type: 2 - Set focus into ChildGui window of the active console\r\n" \
	L"     Type: 3 - Set focus into the parent of the ConEmu window (ConEmu Inside mode)\r\n" \
	L"SetOption('\"Check\",<ID>,<Value>)\r\n" \
	L"  - Set one of checkbox/radio ConEmu's options\r\n" \
	L"    ID: numeric identifier of checkbox (ConEmu.rc, resource.h)\r\n" \
	L"    Value: 0 - off, 1 - on, 2 - third state\r\n" \
	L"SetOption(\"<Name>\",<Value>[,<IsRelative>])\r\n" \
	L"  - Name=one of allowed for changing ConEmu options\r\n" \
	L"    IsRelative=1 to use relative instead of absolute for some options\r\n" \
	L"    \"AlphaValue\": set transparency level of active window\r\n" \
	L"      Value: 40 .. 255 (255==Opaque)\r\n" \
	L"    \"AlphaValueInactive\": set transparency level of inactive window\r\n" \
	L"      Value: 0 .. 255 (255==Opaque)\r\n" \
	L"    \"AlphaValueSeparate\": enable separate active/inactive transparency level\r\n" \
	L"      Value: 0(disable) .. 1(enable)\r\n" \
	L"    \"AlwaysOnTop\": place ConEmu window above all non-topmost windows\r\n" \
	L"      Value: 2 - toggle, 1 - enable, 0 - disable\r\n" \
	L"    \"bgImage\": change background image\r\n" \
	L"      Value: file or color, e.g. \"C:\\image.jpg\" or \"#3182a4\"\r\n" \
	L"    \"bgImageDarker\": darkening of background image\r\n" \
	L"      Value: 0 .. 255\r\n" \
	L"    \"FarGotoEditorPath\": path to the highlight/error editor \r\n" \
	L"      Value: path to the executable with arguments\r\n" \
	L"    \"QuakeAutoHide\": auto hide on focus lose, Quake mode\r\n" \
	L"      Value: 2 - switch auto-hide, 1 - enable, 0 - disable\r\n" \
	L"    \"Scheme\": switch color scheme for the whole ConEmu window\r\n" \
	L"    \"VConScheme\": switch color scheme for the active console\r\n" \
	L"      Value: color palette name, e.g. \"<Solarized>\"\r\n" \
	L"SetParentHWND(<HWND>)\r\n" \
	L"  - Change ConEmu parent window during Inside mode\r\n" \
	L"Settings([<PageResourceId>])\r\n" \
	L"  - Show ‘Settings’ dialog with specified page activated (optionally)\r\n" \
	L"      PageResourceId: integer DialogID from ‘resource.h’\r\n" \
	L"Shell(\"<Verb>\",\"<File>\"[,\"<Parms>\"[,\"<Dir>\"[,<ShowCmd>]]])\r\n" \
	L"  - Verb can be \"open\", \"print\" and so on, or special value \"new_console\",\r\n" \
	L"     which starts File in the new tab of ConEmu window. Examples:\r\n" \
	L"        Shell(\"open\",\"cmd.exe\",\"/k\")\r\n" \
	L"        Shell(\"\",\"\",\"cmd.exe /k -new_console:b\"),\r\n" \
	L"        Shell(\"new_console:b\",\"\",\"cmd.exe /k\")\r\n" \
	L"        Shell(\"new_console:sV\")\r\n" \
	L"Sleep(<Milliseconds>)\r\n" \
	L"  - Milliseconds: max 10000 (10 seconds)\r\n" \
	L"Split(<Cmd>,<Horz>,<Vert>)\r\n" \
	L"  - Cmd=0, Horz=1..99, Vert=0: Duplicate active \"shell\" split to right\r\n" \
	L"    Cmd=0, Horz=0, Vert=1..99: Duplicate active \"shell\" split to bottom\r\n" \
	L"    Cmd=1, Horz=-1..1, Vert=-1..1: Move splitter between panes (aka resize panes)\r\n" \
	L"    Cmd=2, Horz=-1..1, Vert=-1..1: Put cursor to the nearest pane\r\n" \
	L"    Cmd=3, Maximize/restore active pane\r\n" \
	L"    Cmd=4, Horz=-1..1, Vert=-1..1: Swap nearest panes\r\n" \
	L"Status(0[,<Parm>])\r\n" \
	L"  - Show/Hide status bar, Parm=1 - Show, Parm=2 - Hide\r\n" \
	L"Status(1[,\"<Text>\"])\r\n" \
	L"  - Set status bar text\r\n" \
	L"Tab(<Cmd>[,<Parm>])\r\n" \
	L"  - Control ConEmu tabs\r\n" \
	L"     Cmd==0: show/hide tabs\r\n" \
	L"     Cmd==1: commit lazy changes\r\n" \
	L"     Cmd==2: switch next (eq. CtrlTab)\r\n" \
	L"     Cmd==3: switch prev (eq. CtrlShiftTab)\r\n" \
	L"     Cmd==4: switch tab direct (no recent mode), Parm=(1,-1)\r\n" \
	L"     Cmd==5: switch tab recent, Parm=(1,-1)\r\n" \
	L"     Cmd==6: switch console direct (no recent mode), Parm=(1,-1)\r\n" \
	L"     Cmd==7: activate console, Parm=(1-based console index, -1 for the last tab, -2 for the tab of execution)\r\n" \
	L"     Cmd==8: show tabs list menu (indiffirent Far/Not Far)\r\n" \
	L"     Cmd==9: close active tab, same as Close(3)\r\n" \
	L"     Cmd==10: switches visible split-panes, Parm=(1,-1)\r\n" \
	L"     Cmd==11: activates tab by name, title or process name\r\n" \
	L"     Cmd==12: return tabs list \"1: tab1\\n2: tab2\\n...\"\r\n" \
	L"Task(Index[,\"Dir\"])\r\n" \
	L"  - start task with 1-based index\r\n" \
	L"Task(\"Name\"[,\"Dir\"])\r\n" \
	L"  - start task with specified name\r\n" \
	L"TaskAdd(\"Name\",\"Commands\"[,\"GuiArgs\"[,Flags]])\r\n" \
	L"  - create new task and save it to settings\r\n" \
	L"TermMode(<Mode>[,<Action>])\r\n" \
	L"  - changes active terminal modes\r\n" \
	L"    Mode==0: Keyboard emulation (Xterm/Windows)\r\n" \
	L"    Mode==1: Bracketed paste\r\n" \
	L"    Mode==2: Application cursor keys (DECCKM)\r\n" \
	L"    Action==0: Disable mode\r\n" \
	L"    Action==1: Enable mode\r\n" \
	L"    Action==2: Switch mode (default)\r\n" \
	L"Transparency(<Cmd>,<Value>)\r\n" \
	L"  - change ConEmu transparency level absolutely or relatively\r\n" \
	L"    Cmd==0, Value=40..255 (255==Opaque)\r\n" \
	L"    Cmd==1, Value=<relative inc/dec>\r\n" \
	L"    Cmd==2, Value=0..255 (255==Opaque) - for inactive window\r\n" \
	L"    Cmd==3, Value=<relative inc/dec> - for inactive window\r\n" \
	L"    Cmd==4, Value=<0/1> - AlphaValueSeparate\r\n" \
	L"Unfasten\r\n" \
	L"  - Unfasten active RealConsole from active ConEmu window\r\n" \
	L"Wiki([\"PageName\"])\r\n" \
	L"  - Open online documentation\r\n" \
	L"WindowFullscreen()\r\n" \
	L"  - Switch fullscreen window mode\r\n" \
	L"     returns previous window mode\r\n" \
	L"WindowMaximize([<Cmd>])\r\n" \
	L"  - Switch maximized window mode (Cmd==0)\r\n" \
	L"  - Maximize window by width (Cmd==1) or height (Cmd==2)\r\n" \
	L"     returns previous window mode\r\n" \
	L"WindowMinimize() or WindowMinimize(1)\r\n" \
	L"  - Minimize ConEmu window (\"1\" means \"minimize to the TSA\")\r\n" \
	L"     returns previous window mode\r\n" \
	L"WindowMode([\"<Mode>\"])\r\n" \
	L"  - Returns or set current window mode\r\n" \
	L"     \"NOR\", \"MAX\", \"FS\" (fullscreen), \"MIN\", \"TSA\",\r\n" \
	L"     \"TLEFT\", \"TRIGHT\" (tile to left/right), \"THEIGHT\", \"TWIDTH\"\r\n" \
	L"     \"MPREV\", \"MNEXT\" (move ConEmu to prev/next monitor)\r\n" \
	L"     \"HERE\" (move ConEmu to monitor with mouse cursor)\r\n" \
	L"WindowPosSize(\"<X>\",\"<Y>\",\"<W>\",\"<H>\")\r\n" \
	L"  - Change ConEmu window geometry\r\n" \
	L"Write(\"<Text>\")\r\n" \
	L"  - Write text to console OUTPUT\r\n" \
	L"Zoom(N)\r\n" \
	L"  - Set zoom value, 100 - original font size\r\n" \
	L""
