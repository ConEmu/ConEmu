
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

#ifndef COMMON_HEADER_HPP_
#define COMMON_HEADER_HPP_

// Interface version
#define CESERVER_REQ_VER    170

// Max tabs/panes count
#define MAX_CONSOLE_COUNT 30

#include "defines.h"
#include "ConEmuColors.h"
#include "MCircular.h"

#ifndef _DEBUG
//PRAGMA_ERROR("MIN_CON_WIDTH & MIN_CON_HEIGHT - remake for Far Panels")
#endif
//#define MIN_CON_WIDTH 28
//#define MIN_CON_HEIGHT 7
#define MIN_CON_WIDTH 4
#define MIN_CON_HEIGHT 2
#define DEF_CON_WIDTH RELEASEDEBUGTEST(80,110)
#define DEF_CON_HEIGHT RELEASEDEBUGTEST(25,35)
#define MAX_CON_BUFFER_HEIGHT 32766 /* 'magic' 0x7FFE */
#define LONGOUTPUTHEIGHT_MIN 300 /* Used in Settings dialog */
#define LONGOUTPUTHEIGHT_MAX MAX_CON_BUFFER_HEIGHT /* 'magic' 0x7FFE */
#define GUI_ATTACH_TIMEOUT 5000

#define CONSOLE_PROCESSES_MAX 20

#ifndef CONSOLE_NO_SELECTION

typedef struct _CONSOLE_SELECTION_INFO
{
	DWORD dwFlags;
	COORD dwSelectionAnchor;
	SMALL_RECT srSelection;
} CONSOLE_SELECTION_INFO, *PCONSOLE_SELECTION_INFO;

#endif

#ifndef _CRT_WIDE
#define __CRT_WIDE(_String) L ## _String
#define _CRT_WIDE(_String) __CRT_WIDE(_String)
#endif



//#define MAXCONMAPCELLS      (600*400)
#define CES_NTVDM 0x10
#define CEC_INITTITLE       L"ConEmu"

// Our binaries
#define ConEmu_32_EXE L"ConEmu.exe"
#define ConEmu_64_EXE L"ConEmu64.exe"
#define ConEmu_EXE_3264 WIN3264TEST(ConEmu_32_EXE,ConEmu_64_EXE)
#define ConEmuC_32_EXE L"ConEmuC.exe"
#define ConEmuC_64_EXE L"ConEmuC64.exe"
#define ConEmuC_EXE_3264 WIN3264TEST(ConEmuC_32_EXE,ConEmuC_64_EXE)
#define ConEmuCD_32_DLL L"ConEmuCD.dll"
#define ConEmuCD_64_DLL L"ConEmuCD64.dll"
#define ConEmuCD_DLL_3264 WIN3264TEST(ConEmuCD_32_DLL,ConEmuCD_64_DLL)
#define ConEmuHk_32_DLL L"ConEmuHk.dll"
#define ConEmuHk_64_DLL L"ConEmuHk64.dll"
#define ConEmuHk_DLL_3264 WIN3264TEST(ConEmuHk_32_DLL,ConEmuHk_64_DLL)

// Windows and Classes
#define VirtualConsoleClass L"VirtualConsoleClass" // DC Window
#define VirtualConsoleClassMain L"VirtualConsoleClass" // Main window (with title/tabs/etc.)
#define VirtualConsoleClassApp L"VirtualConsoleClassApp" // special Popup (used in some special cases to hide TaskBar icon)
#define VirtualConsoleClassWork L"VirtualConsoleClassWork" // Holder for all VCon-s
#define VirtualConsoleClassBack L"VirtualConsoleClassBack" // Underlay (with scrollers) for each VCon
#define VirtualConsoleClassGhost L"VirtualConsoleClassGhost" // support for Win7 Aero
#define ConEmuPanelViewClass L"ConEmuPanelView" // Used in Far Manager plugin

#define RealConsoleClass L"ConsoleWindowClass"
#define PseudoConsoleClass L"PseudoConsoleWindow"
#define WineConsoleClass L"WineConsoleClass"
// functions IsConsoleClass & IsConsoleWindow are defined in ConEmuCheck.cpp

// GetWindowLong for VirtualConsoleClassBack
#define WindowLongBack_ConWnd 0  // 4 bytes data
#define WindowLongBack_DCWnd  4  // 4 bytes data
// GetWindowLongPtr for VirtualConsoleClass (DC window)
#define WindowLongDCWnd_ConWnd 0 // 4/8 bytes (depends on bitness)

// Some ANSI & Controls
#define DSC 0x90
#define ESC 0x1B
#define ENQ 0x05
#define BEL 0x07
#define CTRL(x) ((x)&0x1F)


#define CECOPYRIGHTSTRING_A "(c) 2009-2020, ConEmu.Maximus5@gmail.com"
#define CECOPYRIGHTSTRING_W L"© 2009-2020 ConEmu.Maximus5@gmail.com"


#define CEHOMEPAGE_A    "https://conemu.github.io/"
#define CEHOMEPAGE     _CRT_WIDE(CEHOMEPAGE_A)
#define CEDOWNLPAGE    L"https://conemu.github.io/en/Downloads.html"
#define CEFLATTRPAGE   L"http://flattr.com/thing/2002773/Maximus5ConEmu-on-GitHub"
#define CEDONATEPAGE   L"https://conemu.github.io/donate.html"
#define CEWIKIBASE     L"https://conemu.github.io/en/"
#define CEGUIARGSPAGE  CEWIKIBASE L"ConEmuArgs.html"
#define CEHOOKSWIKI    CEWIKIBASE L"ConEmuHk.html"
#define CEFIRSTSTART   CEWIKIBASE L"SettingsFast.html"
#define CEMACTYPEWARN  CEWIKIBASE L"Installation.html#mactype"
#define CEMSBUGWIKI    CEWIKIBASE L"MicrosoftBugs.html"
#define CEREPORTBUG    CEWIKIBASE L"Issues.html"
#define CEREPORTCRASH  CEWIKIBASE L"Issues.html"
#define CEWHATSNEW     CEWIKIBASE L"Whats_New.html"
#define CEZONEID       CEWIKIBASE L"ZoneId.html"
#define CECLEARSCREEN  CEWIKIBASE L"ClearScreen.html"


#define DEFAULT_CONSOLE_FONT_NAME L"Lucida Console"


// Tasks related
#define CmdFilePrefix     L'@' // Request to load "task" from the text file
#define DropLnkPrefix     L'?' // The sign we probably got command line by dropping smth on ConEmu's icon
#define TaskBracketLeft   L'{'
#define TaskBracketLeftS  L"{"
#define TaskBracketRight  L'}'
#define TaskBracketRightS L"}"
#define AutoStartTaskName L"<Startup>"
#define AutoStartTaskLeft L'<'


// EnvVars
#define ENV_CONEMUDIR_VAR_A            "ConEmuDir"
#define ENV_CONEMUDIR_VAR_W              _CRT_WIDE(ENV_CONEMUDIR_VAR_A)
#define ENV_CONEMUBASEDIR_VAR_A        "ConEmuBaseDir"
#define ENV_CONEMUBASEDIR_VAR_W          _CRT_WIDE(ENV_CONEMUBASEDIR_VAR_A)
#define ENV_CONEMUBASEDIRSHORT_VAR_A   "ConEmuBaseDirShort"
#define ENV_CONEMUBASEDIRSHORT_VAR_W     _CRT_WIDE(ENV_CONEMUBASEDIRSHORT_VAR_A)
#define ENV_CONEMUWORKDIR_VAR_A        "ConEmuWorkDir"
#define ENV_CONEMUWORKDIR_VAR_W          _CRT_WIDE(ENV_CONEMUWORKDIR_VAR_A)
#define ENV_CONEMUDRIVE_VAR_A          "ConEmuDrive"
#define ENV_CONEMUDRIVE_VAR_W            _CRT_WIDE(ENV_CONEMUDRIVE_VAR_A)
#define ENV_CONEMUWORKDRIVE_VAR_A      "ConEmuWorkDrive"
#define ENV_CONEMUWORKDRIVE_VAR_W        _CRT_WIDE(ENV_CONEMUWORKDRIVE_VAR_A)
#define ENV_CONEMUHWND_VAR_A           "ConEmuHWND"
#define ENV_CONEMUHWND_VAR_W             _CRT_WIDE(ENV_CONEMUHWND_VAR_A)
#define ENV_CONEMUPID_VAR_A            "ConEmuPID"
#define ENV_CONEMUPID_VAR_W              _CRT_WIDE(ENV_CONEMUPID_VAR_A)
#define ENV_CONEMUSERVERPID_VAR_A      "ConEmuServerPID"
#define ENV_CONEMUSERVERPID_VAR_W        _CRT_WIDE(ENV_CONEMUSERVERPID_VAR_A)
#define ENV_CONEMUDRAW_VAR_A           "ConEmuDrawHWND"
#define ENV_CONEMUDRAW_VAR_W             _CRT_WIDE(ENV_CONEMUDRAW_VAR_A)
#define ENV_CONEMUBACK_VAR_A           "ConEmuBackHWND"
#define ENV_CONEMUBACK_VAR_W             _CRT_WIDE(ENV_CONEMUBACK_VAR_A)
#define ENV_CONEMUANSI_VAR_A           "ConEmuANSI"
#define ENV_CONEMUANSI_VAR_W             _CRT_WIDE(ENV_CONEMUANSI_VAR_A)
#define ENV_CONEMUANSILOG_VAR_A        "ConEmuAnsiLog"
#define ENV_CONEMUANSILOG_VAR_W          _CRT_WIDE(ENV_CONEMUANSILOG_VAR_A)
#define ENV_CONEMUFAKEDT_VAR_A         "ConEmuFakeDT"
#define ENV_CONEMUFAKEDT_VAR_W           _CRT_WIDE(ENV_CONEMUFAKEDT_VAR_A)
#define ENV_CONEMU_BUILD_A             "ConEmuBuild"
#define ENV_CONEMU_BUILD_W               _CRT_WIDE(ENV_CONEMU_BUILD_A)
#define ENV_CONEMU_CONFIG_A            "ConEmuConfig"
#define ENV_CONEMU_CONFIG_W              _CRT_WIDE(ENV_CONEMU_CONFIG_A)
#define ENV_CONEMUCFGDIR_VAR_A         "ConEmuCfgDir"
#define ENV_CONEMUCFGDIR_VAR_W           _CRT_WIDE(ENV_CONEMUCFGDIR_VAR_A)
#define ENV_CONEMU_EXEARGS_A           "ConEmuArgs"
#define ENV_CONEMU_EXEARGS_W              _CRT_WIDE(ENV_CONEMU_EXEARGS_A)
#define ENV_CONEMU_EXEARGS2_A          "ConEmuArgs2"
#define ENV_CONEMU_EXEARGS2_W             _CRT_WIDE(ENV_CONEMU_EXEARGS2_A)
#define ENV_CONEMU_TASKNAME_A          "ConEmuTask"
#define ENV_CONEMU_TASKNAME_W            _CRT_WIDE(ENV_CONEMU_TASKNAME_A)
#define ENV_CONEMU_PALETTENAME_A       "ConEmuPalette"
#define ENV_CONEMU_PALETTENAME_W         _CRT_WIDE(ENV_CONEMU_PALETTENAME_A)
#define ENV_CONEMU_ISADMIN_A           "ConEmuIsAdmin"
#define ENV_CONEMU_ISADMIN_W             _CRT_WIDE(ENV_CONEMU_ISADMIN_A)
#define ENV_CONEMU_WAITKEY_A           "ConEmuWaitKey"
#define ENV_CONEMU_WAITKEY_W             _CRT_WIDE(ENV_CONEMU_WAITKEY_A)
#define ENV_CONEMUREPORTEXE_VAR_A      "ConEmuReportExe"
#define ENV_CONEMUREPORTEXE_VAR_W        _CRT_WIDE(ENV_CONEMUREPORTEXE_VAR_A)
// Modifies behavior of starting processes inside ConEmu tab
#define ENV_CONEMU_HOOKS_A             "ConEmuHooks"
#define ENV_CONEMU_HOOKS_W               _CRT_WIDE(ENV_CONEMU_HOOKS_A)
#define ENV_CONEMU_HOOKS_ENABLED      L"Enabled" // Informational
#define ENV_CONEMU_HOOKS_DISABLED     L"OFF" // don't set hooks from "ConEmuHk.dll"
#define ENV_CONEMU_HOOKS_NOARGS       L"NOARG" // don't process -new_console & -cur_console arguments
#define ENV_CONEMU_HOOKS_NOSTART      L"NOSTART" // don't use "start xxx.exe" to start new tab
// This is set to "YES" during AutoUpdate script execution
#define ENV_CONEMU_INUPDATE_A          "ConEmuInUpdate"
#define ENV_CONEMU_INUPDATE_W            _CRT_WIDE(ENV_CONEMU_INUPDATE_A)
#define ENV_CONEMU_INUPDATE_YES       L"YES" // This is set to "YES" during AutoUpdate script execution
// When ConEmuC tries to debug process tree - force disable DEBUG_PROCESS/DEBUG_ONLY_THIS_PROCESS flags when creating subchildren
#define ENV_CONEMU_BLOCKCHILDDEBUGGERS_A    "ConEmuBlockChildDebuggers"
#define ENV_CONEMU_BLOCKCHILDDEBUGGERS_W _CRT_WIDE(ENV_CONEMU_BLOCKCHILDDEBUGGERS_A)
#define ENV_CONEMU_BLOCKCHILDDEBUGGERS_YES L"YES"
// Used internally
#define ENV_CONEMU_MONITOR_INTERNAL_A  "ConEmuMonitorThreadId"
#define ENV_CONEMU_MONITOR_INTERNAL_W    _CRT_WIDE(ENV_CONEMU_MONITOR_INTERNAL_A)
// Don't forget add exclusion to IsExportEnvVarAllowed
#define ENV_CONEMU_SLEEP_INDICATE_A    "ConEmuSleepIndicator"
#define ENV_CONEMU_SLEEP_INDICATE_W      _CRT_WIDE(ENV_CONEMU_SLEEP_INDICATE_A)
#define ENV_CONEMU_SLEEP_INDICATE_NUM L"NUM"
#define ENV_CONEMU_SLEEP_INDICATE_TTL L"TITLE"
// Codepage conversions hacks: ConEmuCpCvt=perl.exe:1252:1251;less.exe:850:866;*:1234:866;...
#define ENV_CONEMU_CPCVT_APP_A         "ConEmuCpCvt"
#define ENV_CONEMU_CPCVT_APP_W           _CRT_WIDE(ENV_CONEMU_CPCVT_APP_A)
// Override default console CP: ConEmuDefCp=65001
#define ENV_CONEMU_DEFAULTCP_A         "ConEmuDefaultCp"
#define ENV_CONEMU_DEFAULTCP_W           _CRT_WIDE(ENV_CONEMU_DEFAULTCP_A)
// Return set of ConEmu::ConsoleFlags flags from ConEmu settings, e.g. "0x000082A6"
#define ENV_CONEMU_FEATURES_A          "ConEmuFeatures"
#define ENV_CONEMU_FEATURES_W            _CRT_WIDE(ENV_CONEMU_FEATURES_A)

#define CONEMU_CONHOST_CREATED_MSG L"ConEmu: ConHost was created PID=" // L"%u\n"

// AnsiCon compatible variables
#define ENV_ANSICON_VAR_A           "ANSICON"      // ex. -> "140x9999 (140x35)"
#define ENV_ANSICON_VAR_W             _CRT_WIDE(ENV_ANSICON_VAR_A)
#define ENV_ANSICON_DEF_VAR_A       "ANSICON_DEF"  // ex. -> "7"
#define ENV_ANSICON_DEF_VAR_W         _CRT_WIDE(ENV_ANSICON_DEF_VAR_A)
#define ENV_ANSICON_VER_VAR_A       "ANSICON_VER"  // "189"
#define ENV_ANSICON_VER_VAR_W         _CRT_WIDE(ENV_ANSICON_VER_VAR_A)
#define ENV_ANSICON_VER_VALUE       "189"


//#define CE_CURSORUPDATE     L"ConEmuCursorUpdate%u" // ConEmuC_PID - изменился курсор (размер или выделение). положение курсора отслеживает GUI

// Pipe name formats
#define CESERVERPIPENAME    L"\\\\%s\\pipe\\ConEmuSrv%u"      // ConEmuC_PID
#define CESERVERINPUTNAME   L"\\\\%s\\pipe\\ConEmuSrvInput%u" // ConEmuC_PID
#define CESERVERQUERYNAME   L"\\\\%s\\pipe\\ConEmuSrvQuery%u" // ConEmuC_PID
#define CESERVERWRITENAME   L"\\\\%s\\pipe\\ConEmuSrvWrite%u" // ConEmuC_PID
#define CESERVERREADNAME    L"\\\\%s\\pipe\\ConEmuSrvRead%u"  // ConEmuC_PID
#define CEGUIPIPENAME       L"\\\\%s\\pipe\\ConEmuGui.%08X"   // myGetConsoleWindow() // необходимо, чтобы плагин мог общаться с GUI
															  // ghConEmuWndRoot --> CConEmuMain::GuiServerThreadCommand
#define CEPLUGINPIPENAME    L"\\\\%s\\pipe\\ConEmuPlugin%u"   // Far_PID
#define CEHOOKSPIPENAME     L"\\\\%s\\pipe\\ConEmuHk%u"       // PID процесса, в котором крутится Pipe

#define CEINPUTSEMAPHORE    L"ConEmuInputSemaphore.%08X"      // myGetConsoleWindow()

// Mapping name formats
#define CEGUIINFOMAPNAME    L"ConEmuGuiInfoMapping.%u" // --> ConEmuGuiMapping            ( % == dwGuiProcessId )
#define CECONMAPNAME        L"ConEmuFileMapping.%08X"  // --> CESERVER_CONSOLE_MAPPING_HDR ( % == (DWORD)ghConWnd )
#define CECONMAPNAME_A       "ConEmuFileMapping.%08X"  // --> CESERVER_CONSOLE_MAPPING_HDR ( % == (DWORD)ghConWnd ) simplifying ansi
#define CECONAPPMAPNAME     L"ConEmuAppMapping.%08X"   // --> CESERVER_CONSOLE_APP_MAPPING ( % == (DWORD)ghConWnd )
#define CEDEFTERMMAPNAME    L"ConEmuDefTermInside.%u"  // --> CONEMU_INSIDE_DEFTERM_MAPPING ( % == PID of ConEmu process which is integrated into some app )
#define CEINSIDEMAPNAMEP    L"ConEmuDefTermInside.%u"  // --> CONEMU_INSIDE_MAPPING ( % == PID of the root window, where ConEmu is integrated into )
#define CEINSIDEMAPNAMEW    L"ConEmuDefTermInside.%08X"// --> CONEMU_INSIDE_MAPPING ( % == the root window, where ConEmu is integrated into )
#define CEFARMAPNAME        L"ConEmuFarMapping.%u"     // --> CEFAR_INFO_MAPPING               ( % == nFarPID )
#define CECONVIEWSETNAME    L"ConEmuViewSetMapping.%u" // --> PanelViewSetMapping
//#ifdef _DEBUG
#define CEPANELDLGMAPNAME   L"ConEmuPanelViewDlgsMapping.%u" // -> DetectedDialogs     ( % == nFarPID )
//#endif
#define CECONOUTPUTNAME     L"ConEmuLastOutputMapping.%08X"    // --> CESERVER_CONSAVE_MAPHDR ( %X == (DWORD)ghConWnd )
#define CECONOUTPUTITEMNAME L"ConEmuLastOutputMapping.%08X.%u" // --> CESERVER_CONSAVE_MAP ( %X == (DWORD)ghConWnd, %u = CESERVER_CONSAVE_MAPHDR.nCurrentIndex )

// Default terminal hook module name
#define CEDEFTERMDLLFORMAT  L"ConEmuHk%s.%02u%02u%02u%s.dll"
#define CEDEFAULTTERMHOOK   L"ConEmuDefaultTerm.%u" // Если Event взведен - нужно загрузить хуки в процесс только для перехвата запуска консольных приложений
#define CEDEFAULTTERMHOOKOK L"ConEmuDefaultTermOK.%u" // Взводится в ConEmuHk когда инициализация DefTerm началась (CEDEFAULTTERMHOOK больше не нужен)
#define CEDEFAULTTERMBEGIN  L"ConEmuDefTermProc.%u" // Мутекс создается перед началом установки хуков, чтобы не дрались разные процессы-установщики
#define CEDEFAULTTERMHOOKWAIT 0 // Don't need timeout, because we wait for remote thread - WaitForSingleObject(hThread, INFINITE);
//#define CEDEFAULTTERMMUTEX  L"IsConEmuDefaultTerm.%u" // Если Mutex есть - значит какая-то версия длл-ки уже была загружена в обрабатываемый процесс

// Default folder for AnsiLog files
#define CEANSILOGFOLDER     L"%UserProfile%\\Desktop\\ConEmuLogs\\"
// File name format for AnsiLog
#define CEANSILOGNAMEFMT    L"ConEmu-%u-%02u-%02u-p%u.log"

// Events
#define CEDATAREADYEVENT    L"ConEmuSrvDataReady.%u"
#define CEFARWRITECMTEVENT  L"ConEmuSrvFarWriteCommit.%u"
#define CECURSORCHANGEEVENT L"ConEmuSrvCursorChanged.%u"
#define CEFARALIVEEVENT     L"ConEmuFarAliveEvent.%u"
//#define CECONMAPNAMESIZE    (sizeof(CESERVER_REQ_CONINFO)+(MAXCONMAPCELLS*sizeof(CHAR_INFO)))
//#define CEGUIATTACHED       L"ConEmuGuiAttached.%u"
#define CEGUIRCONSTARTED    L"ConEmuGuiRConStarted.%u"
#define CEGUI_ALIVE_EVENT   L"ConEmuGuiStarted"
#define CESRVSTARTEDEVENT   L"ConEmuSrvStarted.%u" // %u==ServerPID
#define CEKEYEVENT_CTRL     L"ConEmuCtrlPressed.%u"
#define CEKEYEVENT_SHIFT    L"ConEmuShiftPressed.%u"
#define CEHOOKLOCKMUTEX     L"ConEmuHookMutex.%u"
//#define CEHOOKDISABLEEVENT  L"ConEmuSkipHooks.%u"
#define CEGHOSTSKIPACTIVATE L"ConEmuGhostActivate.%u"
#define CECONEMUROOTPROCESS L"ConEmuRootProcess.%u" // Если Event взведен - значит это корневой процесс, облегченную версию хуков не использовать!
#define CECONEMUROOTTHREAD  L"ConEmuRootThread.%u"  // Если Event взведен - ConEmuHk загружен в главной нити, гонять snapshot в GetMainThreadId не требуется

#define CESECURITYNAME       "ConEmuLocalData"

//#define CONEMUMSG_ATTACH L"ConEmuMain::Attach"            // wParam == hConWnd, lParam == ConEmuC_PID
//WARNING("CONEMUMSG_SRVSTARTED нужно переделать в команду пайпа для GUI");
//#define CONEMUMSG_SRVSTARTED L"ConEmuMain::SrvStarted"    // wParam == hConWnd, lParam == ConEmuC_PID
//#define CONEMUMSG_SETFOREGROUND L"ConEmuMain::SetForeground"            // wParam == hConWnd, lParam == ConEmuC_PID
#define CONEMUMSG_FLASHWINDOW L"ConEmuMain::FlashWindow"
//#define CONEMUCMDSTARTED L"ConEmuMain::CmdStarted"    // wParam == hConWnd, lParam == ConEmuC_PID (as ComSpec)
//#define CONEMUCMDSTOPPED L"ConEmuMain::CmdTerminated" // wParam == hConWnd, lParam == ConEmuC_PID (as ComSpec)
//#define CONEMUMSG_LLKEYHOOK L"ConEmuMain::LLKeyHook"    // wParam == hConWnd, lParam == ConEmuC_PID
#define CONEMUMSG_ACTIVATECON L"ConEmuMain::ActivateCon"  // wParam == ConNumber (1..12)
#define CONEMUMSG_SWITCHCON L"ConEmuMain::SwitchCon"
#define CONEMUMSG_HOOKEDKEY L"ConEmuMain::HookedKey"
#define CONEMUMSG_CONSOLEHOOKEDKEY L"ConEmuMain::ConsoleHookedKey"
#define CONEMUMSG_PNLVIEWFADE L"ConEmuTh::Fade"
#define CONEMUMSG_PNLVIEWSETTINGS L"ConEmuTh::Settings"
#define PNLVIEWMAPCOORD_TIMEOUT 1000
#define CONEMUMSG_PNLVIEWMAPCOORD L"ConEmuTh::MapCoords"
//#define CONEMUMSG_PNLVIEWLBTNDOWN L"ConEmuTh::LBtnDown"
#define CONEMUMSG_RESTORECHILDFOCUS L"ConEmu::RestoreChildFocus"


constexpr int PANEL_VIEW_WINDOW_LONG_FADE = static_cast<int>(16 * sizeof(DWORD));

enum class ConEmuPanelViewFade
{
	Normal = 1, // normal, bright colors
	Fade = 2,   // fade colors, when ConEmu is out of focus and fade option is enabled
};

// Команды из плагина ConEmu и для GUI Macro
enum ConEmuTabCommand
{
	// These commands go from ConEmu Far plugin and GuiMacro
	ctc_ShowHide = 0,
	ctc_SwitchCommit = 1,
	ctc_SwitchNext = 2,
	ctc_SwitchPrev = 3,
	// GuiMacro commands only
	ctc_SwitchDirect = 4,
	ctc_SwitchRecent = 5,
	ctc_SwitchConsoleDirect = 6,
	ctc_ActivateConsole = 7,
	ctc_ShowTabsList = 8,
	ctc_CloseTab = 9,
	ctc_SwitchPaneDirect = 10,
	ctc_ActivateByName = 11,
	ctc_GetTabsList = 12,
};

enum ConEmuStatusCommand
{
	csc_ShowHide = 0,
	csc_SetStatusText = 1,
};

enum ConEmuWindowCommand
{
	cwc_Current     = 0,
	cwc_Restore     = 1,  // wmNormal
	cwc_Minimize    = 2,  // SC_MINIMIZE
	cwc_MinimizeTSA = 3,  // HideWindowToTray
	cwc_Maximize    = 4,  // wmMaximized
	cwc_FullScreen  = 5,  // wmFullScreen
	cwc_TileLeft    = 6,  // Same as Win+Left in Win7
	cwc_TileRight   = 7,  // Same as Win+Right in Win7
	cwc_TileHeight  = 8,  // Same as Win+Shift+Up in Win7
	cwc_PrevMonitor = 9,  // Same as Win+Shift+Left in Win7
	cwc_NextMonitor = 10, // Same as Win+Shift+Right in Win7
	cwc_TileWidth   = 11, // Maximize width (no analogue yet)
	cwc_BringHere   = 12, // Jump to active monitor (Bring here)
	// for comparison
	cwc_LastCmd
};

enum CONSOLE_KEY_ID
{
	ID_ALTTAB,
	ID_ALTESC,
	ID_ALTSPACE,
	ID_ALTENTER,
	ID_ALTPRTSC,
	ID_PRTSC,
	ID_CTRLESC,
};

#define EvalBufferTurnOnSize(Now) (2 * (Now) + 32)

enum RealBufferScroll
{
	rbs_None = 0,
	rbs_Vert = 1,
	rbs_Horz = 2,
	rbs_Any  = 3,
};

// CECMD_STARTXTERM: what we want to change
// lifetime:
//   `process` - mode would be reset on called process termination
//   `console` - survive on process termination
enum TermModeCommand
{
	tmc_TerminalType   = 0, // TermEmulationType    - process life-time
	tmc_BracketedPaste = 1, // true/false           - process life-time
	tmc_AppCursorKeys  = 2, // true/false           - process life-time
	tmc_CursorShape    = 3, // TermCursorShapes     - ***console*** life-time
	tmc_MouseMode      = 4, // set of TermMouseMode - process life-time
	tmc_ConInMode      = 5, // server detected ENABLE_VIRTUAL_TERMINAL_INPUT change -- root process life-time
	// for array purposes
	tmc_Last
};

// tmc_TerminalType: used for control keys (arrows e.g.) translation
enum TermEmulationType
{
	te_win32 = 0,
	te_xterm = 1,
	te_conpty = 2, // xterm input and session-owned RGB annotation data
};

enum TermCursorShapes
{
	tcs_Default = 0,
	tcs_BlockBlink = 1,
	tcs_BlockSteady = 2,
	tcs_UnderlineBlink = 3,
	tcs_UnderlineSteady = 4,
	tcs_VBarBlink = 5,
	tcs_VBarSteady = 6,
	tcs_Last
};

enum TermMouseMode
{
	tmm_None    = 0,
	tmm_X10     = 0x0001,
	tmm_VT200   = 0x0002,
	tmm_BTN     = 0x0004, /* BTN_EVENT_MOUSE */
	tmm_ANY     = 0x0008, /* ANY_EVENT_MOUSE */
	tmm_FOCUS   = 0x0010, /* FOCUS_EVENT_MOUSE */
	tmm_UTF8    = 0x0020, /* Xterm's UTF8 encoding for mouse positions */
	tmm_XTERM   = 0x0040, /* Xterm's CSI-style mouse encoding */
	tmm_URXVT   = 0x0080, /* Urxvt's CSI-style mouse encoding */
	/* **** ConEmu internals *** */
	tmm_VIM     = 0x1000, /* DEPRECATED: Used to emulate wheel via \033[62~ ... \033[65~ */
	tmm_SCROLL  = 0x2000, /* Send Up/Down/PgUp/PgDn instead of wheel events */
};

// Used with CECMD_SETPROGRESS and "ESC ] 9 ; 4 ; st ; pr ST" (st value)
enum class AnsiProgressStatus : uint16_t
{
	// No progress indicator
	None = 0,
	// Process is running normally, progress = 0..100, green indicator
	Running = 1,
	// An error occurred, progress = 0..100, red indicator
	Error = 2,
	// Process is running, progress is uknown, green indeterminate indicator
	Indeterminate = 3,
	// Process is paused, progress = 0..100, yellow indicator
	Paused = 4,

	// Reserved for future use
	LongRunStart,
	// Reserved for future use
	LongRunStop,
};

//#define CONEMUMAPPING    L"ConEmuPluginData%u"
//#define CONEMUDRAGFROM   L"ConEmuDragFrom%u"
//#define CONEMUDRAGTO     L"ConEmuDragTo%u"
//#define CONEMUREQTABS    L"ConEmuReqTabs%u"
//#define CONEMUSETWINDOW  L"ConEmuSetWindow%u"
//#define CONEMUPOSTMACRO  L"ConEmuPostMacro%u"
//#define CONEMUDEFFONT    L"ConEmuDefFont%u"
//#define CONEMULANGCHANGE L"ConEmuLangChange%u"
//#define CONEMUEXIT       L"ConEmuExit%u"
//#define CONEMUALIVE      L"ConEmuAlive%u"
//#define CONEMUREADY      L"ConEmuReady%u"
#define CONEMUTABCHANGED L"ConEmuTabsChanged"

#define VIRTUAL_REGISTRY_GUID   L"{16B56CA5-F8D2-4EEA-93DC-32403C7355E1}"
#define VIRTUAL_REGISTRY_GUID_A  "{16B56CA5-F8D2-4EEA-93DC-32403C7355E1}"
#define VIRTUAL_REGISTRY_ROOT   L"Software\\ConEmu Virtual Registry"

//#define CESIGNAL_C          L"ConEmuC_C_Signal.%u"
//#define CESIGNAL_BREAK      L"ConEmuC_Break_Signal.%u"

typedef DWORD CECMD;
const CECMD
//	CECMD_CONSOLEINFO    = 1,
	CECMD_CONSOLEDATA    = 2,
	CECMD_CONSOLEFULL    = 3,
	CECMD_CMDSTARTSTOP   = 4, // CESERVER_REQ_STARTSTOP //-V112
//	CECMD_GETGUIHWND     = 5, // GUI_CMD, In = {}, Out = {dwData[0]==ghWnd, dwData[1]==ConEmuDcWindow
//	CECMD_RECREATE       = 6,
	CECMD_TABSCHANGED    = 7,
	CECMD_CMDSTARTED     = 8, // == CECMD_SETSIZE + восстановить содержимое консоли (запустился comspec)
	CECMD_CMDFINISHED    = 9, // == CECMD_SETSIZE + сохранить содержимое консоли (завершился comspec)
	CECMD_GETOUTPUTFILE  = 10, // Записать вывод последней консольной программы во временный файл и вернуть его имя
//	CECMD_GETOUTPUT      = 11,
	CECMD_LANGCHANGE     = 12,
	CECMD_NEWCMD         = 13, // CESERVER_REQ_NEWCMD, Запустить в этом экземпляре новую консоль с переданной командой (используется при SingleInstance и -new_console). Выполняется в GUI!
	CECMD_TABSCMD        = 14, // enum ConEmuTabCommand: 0: спрятать/показать табы, 1: перейти на следующую, 2: перейти на предыдущую, 3: commit switch
	CECMD_RESOURCES      = 15, // Посылается плагином при инициализации (установка ресурсов)
//	CECMD_GETNEWCONPARM  = 16, // Доп.аргументы для создания новой консоли (шрифт, размер,...)
	CECMD_SETSIZESYNC    = 17, // Синхронно, ждет (но недолго), пока FAR обработает изменение размера (то есть отрисуется)
	CECMD_ATTACH2GUI     = 18, // (a. GUI|Plugin->SRV) Выполнить подключение видимой (отключенной) консоли к GUI. Без аргументов.
	                           // (b. SRV->GUI) Запрос GUI на вкладку для сервера [IN] CESERVER_REQ_STARTSTOP, [OUT] CESERVER_REQ_STARTSTOPRET.
	CECMD_FARLOADED      = 19, // Посылается плагином в сервер
//	CECMD_SHOWCONSOLE    = 20, // В Win7 релизе нельзя скрывать окно консоли, запущенной в режиме администратора -- заменено на CECMD_POSTCONMSG & CECMD_SETWINDOWPOS
	CECMD_POSTCONMSG     = 21, // В Win7 релизе нельзя посылать сообщения окну консоли, запущенной в режиме администратора
//	CECMD_REQUESTCONSOLEINFO  = 22, // было CECMD_REQUESTFULLINFO
	CECMD_SETFOREGROUND  = 23,
	CECMD_FLASHWINDOW    = 24, // CESERVER_REQ_FLASHWINFO
//	CECMD_SETCONSOLECP   = 25,
	CECMD_SAVEALIASES    = 26,
	CECMD_GETALIASES     = 27,
	CECMD_SETSIZENOSYNC  = 28, // Почти CECMD_SETSIZE. Вызывается из плагина.
	CECMD_FARDETACHED    = 29,
	CECMD_REGPANELVIEW   = 30,
	CECMD_ONACTIVATION   = 31, // Для установки флажка ConsoleInfo->bConsoleActive
	CECMD_SETWINDOWPOS   = 32, // CESERVER_REQ_SETWINDOWPOS. //-V112
	CECMD_SETWINDOWRGN   = 33, // CESERVER_REQ_SETWINDOWRGN.
	CECMD_SETBACKGROUND  = 34, // CESERVER_REQ_SETBACKGROUND
	CECMD_ACTIVATECON    = 35, // CESERVER_REQ_ACTIVATECONSOLE
//	CECMD_ONSERVERCLOSE  = 35, // Посылается из ConEmuC*.exe перед закрытием в режиме сервера
	CECMD_DETACHCON      = 36,
	CECMD_GUIMACRO       = 38, // CESERVER_REQ_GUIMACRO. Найти в других консолях редактор/вьювер
	CECMD_ONCREATEPROC   = 39, // CESERVER_REQ_ONCREATEPROCESS
	CECMD_SRVSTARTSTOP   = 40, // {DWORD(1/101); DWORD(ghConWnd);}
	CECMD_SETFARPID      = 41, // Посылается в сервер, чтобы он сменил (CESERVER_CONSOLE_MAPPING_HDR.nFarPID)
	CECMD_ASSERT         = 42, // Отобразить Assert в ConEmu. In=wchar_t[], Out=DWORD.
	CECMD_GUICHANGED     = 43, // посылается в сервер, чтобы он обновил у себя ConEmuGuiMapping->CESERVER_CONSOLE_MAPPING_HDR
	CECMD_PEEKREADINFO   = 44, // CESERVER_REQ_PEEKREADINFO: посылается в GUI на вкладку Debug
	CECMD_ATTACHGUIAPP   = 46, // CESERVER_REQ_ATTACHGUIAPP
	CECMD_KEYSHORTCUTS   = 47, // BYTE Data[2]; SetConsoleKeyShortcuts from kernel32.dll
	CECMD_SETFOCUS       = 48, // CESERVER_REQ_SETFOCUS
	CECMD_SETPARENT      = 49, // CESERVER_REQ_SETPARENT
	CECMD_CTRLBREAK      = 50, // GenerateConsoleCtrlEvent(dwData[0], dwData[1])
//	CECMD_GETCGUINFO     = 51, // ConEmuGuiMapping
	CECMD_SETGUIEXTERN   = 52, // dwData[0]==TRUE - вынести приложение наружу из вкладки ConEmu, dwData[0]==FALSE - вернуть во вкладку
	CECMD_ALIVE          = 53, // просто проверка
	CECMD_STARTSERVER    = 54, // CESERVER_REQ_START. Запустить в консоли ConEmuC.exe в режиме сервера
	CECMD_LOCKDC         = 55, // CESERVER_REQ_LOCKDC
	#ifdef USE_COMMIT_EVENT
	CECMD_REGEXTCONSOLE  = 56, // CESERVER_REQ_REGEXTCON. Регистрация процесса, использующего ExtendedConsole.dll
	#endif
	CECMD_GETALLTABS     = 57, // CESERVER_REQ_GETALLTABS. Вернуть список всех табов, для показа в Far и использовании в макросах.
	CECMD_ACTIVATETAB    = 58, // dwData[0]=0-based console, dwData[1]=0-based tab number
	CECMD_FREEZEALTSRV   = 59, // dwData[0]=1-Freeze, 0-Thaw; dwData[1]=New Alt server PID
	CECMD_SETFULLSCREEN  = 60, // SetConsoleDisplayMode(CONSOLE_FULLSCREEN_MODE) -> CESERVER_REQ_FULLSCREEN
	CECMD_SETTABTITLE    = 63, // wData - это LPCWSTR, посылается в GUI
	CECMD_SETPROGRESS    = 64, // wData[0]: 0 - remove, 1 - set, 2 - error. Для "1": wData[1] - 0..100%.
	CECMD_SETCONCOLORS   = 65, // CESERVER_REQ_SETCONSOLORS
	CECMD_SETCONTITLE    = 66, // wData - это LPCWSTR, посылается в Server
	CECMD_EXPORTVARS     = 67, // wData - same as GetEnvironmentStringsW returns, but may be less (selected vars only)
	CECMD_EXPORTVARSALL  = 68, // same as CECMD_EXPORTVARS, but apply environment to all tabs
	CECMD_DUPLICATE      = 69, // CESERVER_REQ_DUPLICATE. sent to root console process (cmd, far, powershell), processed with ConEmuHk - Create new tab reproducing current state.
	CECMD_GUICLIENTSHIFT = 71, // GuiStylesAndShifts
	CECMD_ALTBUFFER      = 72, // CESERVER_REQ_ALTBUFFER: CmdOutputStore/Restore
	CECMD_ALTBUFFERSTATE = 73, // Проверить, разрешен ли Alt.Buffer?
	CECMD_STARTXTERM     = 74, // dwData[0]=TermModeCommand, dwData[1]=value (TermEmulationType for tmc_TerminalType, and so on), dwData[2]=the PID (may differ from hdr's one).
	//CECMD_DEFTERMSTARTED = 75, // Уведомить GUI, что инициализация хуков для Default Terminal была завершена -- не требуется, ConEmuC ждет успеха
	CECMD_UPDCONMAPHDR   = 76, // AltServer не может менять CESERVER_CONSOLE_MAPPING_HDR во избежание конфликтов. Это делает только RM_MAINSERVER (req.ConInfo)
	CECMD_SETCONSCRBUF   = 77, // CESERVER_REQ_SETCONSCRBUF - temporarily block active server reading thread to change console buffer size
	CECMD_PORTABLESTART  = 78, // CESERVER_REQ_PORTABLESTARTED - used when XxxPortable.exe starts Xxx.exe (paf - kitty, tcc, etc.)
	CECMD_STORECURDIR    = 79, // CESERVER_REQ_STORECURDIR <== GetCurrentDirectory()
	CECMD_GETALLPANELS   = 80, // Result ==> CESERVER_REQ_GETALLPANELS
	CECMD_SETTOPLEFT     = 81, // CESERVER_REQ_CONINFO
	CECMD_PROMPTSTARTED  = 82, // CESERVER_PROMPTSTARTED (ConEmuHk -> Server)
	CECMD_QUERYPALETTE   = 83, // CESERVER_PALETTE
	CECMD_TERMINATEPID   = 84, // DWORD[0]=Count, DWORD[1..]=PIDs
	CECMD_AFFNTYPRIORITY = 85, // QDWORD[0]=Affinity, QDWORD[1]=PRIORITY
	CECMD_PAUSE          = 86, // DWORD[0]: CEPauseCmd; Returns CEPauseCmd
	CECMD_REDRAWHWND     = 87, // DWORD[0]: HWND
	CECMD_LOCKSTATION    = 88, // When user press Win+L
	CECMD_UNLOCKSTATION  = 89, // CESERVER_REQ_SETSIZE
	CECMD_GETROOTINFO    = 90, // CESERVER_ROOT_INFO
	CECMD_WRITETEXT      = 91, // wData[] == LPCWSTR
	CECMD_GETTASKCMD     = 92, // CESERVER_REQ_TASK
	CECMD_ALLOWANSIEXEC  = 93, // IN: wData[] - GuiMacro or CmdLine; OUT: TRUE/FALSE
	CECMD_SSHAGENTSTART  = 94, // dwData[0] - PID of ssh-agent.exe started from one of the ConEmu console processes
	CECMD_FINDNEXTROWID  = 95, // IN: dwData[0] - from row, dwData[1] - search upward (TRUE/FALSE); OUT: dwData[0] - found row or DWORD(-1), dwData[1] - rowid
	CECMD_STARTCONNECTOR = 96, // IN: Data[0] - ASCIIZ string for mount prefix, e.g. "/cygdrive" or just empty "".
	CECMD_STARTPTYSRV    = 97, // IN: Data[0] - start=TRUE, stop=FALSE; OUT: handles for connector qwData[0] - read_input, qwData[1] - write_output
/** Команды FAR плагина **/
	CMD_FIRST_FAR_CMD    = 200,
	CMD_DRAGFROM         = 200,
	CMD_DRAGTO           = 201,
	CMD_REQTABS          = 202,
	CMD_SETWINDOW        = 203,
	CMD_POSTMACRO        = 204, // Если первый символ макроса '@' и после него НЕ пробел - макрос выполняется в DisabledOutput
//	CMD_DEFFONT          = 205,
	CMD_LANGCHANGE       = 206,
	CMD_FARSETCHANGED    = 207, // Изменились настройки для фара (isFARuseASCIIsort, isShellNoZoneCheck, ...)
//	CMD_SETSIZE          = 208,
	CMD_EMENU            = 209,
	CMD_LEFTCLKSYNC      = 210,
	CMD_REDRAWFAR        = 211,
	CMD_FARPOST          = 212,
	CMD_CHKRESOURCES     = 213,
//	CMD_QUITFAR          = 214, // Дернуть завершение консоли (фара?)
	CMD_CLOSEQSEARCH     = 215,
//	CMD_LOG_SHELL        = 216,
	CMD_SET_CON_FONT     = 217, // CESERVER_REQ_SETFONT
	CMD_GUICHANGED       = 218, // CESERVER_REQ_GUICHANGED. изменились настройки GUI (шрифт), размер окна ConEmu, или еще что-то
	CMD_ACTIVEWNDTYPE    = 219, // ThreadSafe - получить информацию об активном окне (его типе) в Far
	CMD_OPENEDITORLINE   = 220, // CESERVER_REQ_FAREDITOR. Открыть в редакторе файл и перейти на строку (обработка ошибок компилятора)
	CMD_LAST_FAR_CMD     = CMD_OPENEDITORLINE;


#define PIPEBUFSIZE 4096
#define DATAPIPEBUFSIZE 40000

// Console Undocumented
#define MOD_LALT         0x0010
#define MOD_RALT         0x0020
#define MOD_LCONTROL     0x0040
#define MOD_RCONTROL     0x0080

typedef DWORD ConEmuModifiers;
const ConEmuModifiers
	// Для удобства, в младшем байте VkMod хранится VK кнопки
	cvk_VK_MASK  = 0x000000FF,

	// Модификаторы, которые юзер просил различать, правый или левый
	cvk_LCtrl    = 0x00000100,
	cvk_RCtrl    = 0x00000200,
	cvk_LAlt     = 0x00000400,
	cvk_RAlt     = 0x00000800,
	cvk_LShift   = 0x00001000,
	cvk_RShift   = 0x00002000,

	// Если без разницы, правый или левый
	cvk_Ctrl     = 0x00010000,
	cvk_Alt      = 0x00020000,
	cvk_Shift    = 0x00040000,
	cvk_Win      = 0x00080000,
	cvk_Apps     = 0x00100000,

	// Без модификатора
	cvk_Naked    = 0x10000000,
	// Спец.модификатор
	cvk_NumHost  = 0x20000000,
	cvk_ArrHost  = 0x40000000,

	// Маска всех с учетом правый/левый
	cvk_CtrlAny  = cvk_Ctrl|cvk_LCtrl|cvk_RCtrl,
	cvk_AltAny   = cvk_Alt|cvk_LAlt|cvk_RAlt,
	cvk_ShiftAny = cvk_Shift|cvk_LShift|cvk_RShift,
	// Маска вообще всех допустимых кнопок
	cvk_ALLMASK  = cvk_CtrlAny|cvk_AltAny|cvk_ShiftAny|cvk_Win|cvk_Apps,
	// Empty value
	cvk_NULL     = 0;

// ConEmu internal virtual key codes
#define VK_WHEEL_UP    0xD0
#define VK_WHEEL_DOWN  0xD1
#define VK_WHEEL_LEFT  0xD2
#define VK_WHEEL_RIGHT 0xD3
//
#define VK_WHEEL_FIRST VK_WHEEL_UP
#define VK_WHEEL_LAST  VK_WHEEL_RIGHT


//// Команды FAR плагина
//typedef DWORD FARCMD;
//const FARCMD
//	CMD_DRAGFROM       = 0,
//	CMD_DRAGTO         = 1,
//	CMD_REQTABS        = 2,
//	CMD_SETWINDOW      = 3,
//	CMD_POSTMACRO      = 4, // Если первый символ макроса '@' и после него НЕ пробел - макрос выполняется в DisabledOutput
////	CMD_DEFFONT        = 5,
//	CMD_LANGCHANGE     = 6,
//	CMD_FARSETCHANGED  = 7, // Изменились настройки для фара (isFARuseASCIIsort, isShellNoZoneCheck, ...)
////	CMD_SETSIZE        = 8,
//	CMD_EMENU          = 9,
//	CMD_LEFTCLKSYNC    = 10,
//	CMD_REDRAWFAR      = 11,
//	CMD_FARPOST        = 12,
//	CMD_CHKRESOURCES   = 13,
////	CMD_QUITFAR        = 14, // Дернуть завершение консоли (фара?)
//	CMD_CLOSEQSEARCH   = 15,
////	CMD_LOG_SHELL      = 16,
//	CMD_SET_CON_FONT   = 17, // CESERVER_REQ_SETFONT
//	CMD_GUICHANGED     = 18, // CESERVER_REQ_GUICHANGED. изменились настройки GUI (шрифт), размер окна ConEmu, или еще что-то
//	CMD_ACTIVEWNDTYPE  = 19; // ThreadSafe - получить информацию об активном окне (его типе) в Far
//// +2
//	//MAXCMDCOUNT        = 21;
//    //CMD_EXIT           = MAXCMDCOUNT-1;



//#pragma pack(push, 1)
#include <pshpack1.h>  // NOLINT(clang-diagnostic-pragma-pack)

struct HWND2
{
	DWORD u;
	operator HWND() const
	{
		return reinterpret_cast<HWND>(static_cast<DWORD_PTR>(u)); //-V204
	}
	operator DWORD() const
	{
		return u;
	}
	DWORD GetPortableHandle() const
	{
		return u;
	}
	HWND GetHandle() const
	{
		return reinterpret_cast<HWND>(static_cast<DWORD_PTR>(u));
	}
	struct HWND2& operator=(HWND h)
	{
		u = static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(h)); //-V205
		_ASSERTE(GetHandle() == h);
		return *this;
	}
};

// Для унификации x86/x64. Хранить здесь реальный HKEY нельзя.
// Используется только для "предопределенных" ключей типа HKEY_USERS
struct HKEY2
{
	DWORD u;
	operator HKEY() const
	{
		return (HKEY)(DWORD_PTR)(LONG)u;
	};
	operator DWORD() const
	{
		return u;
	};
	struct HKEY2& operator=(HKEY h)
	{
		//_ASSERTE(((DWORD_PTR)h) < 0x100000000LL);
		u = (DWORD)(LONG)(LONG_PTR)h;
		return *this;
	};
};

struct HANDLE2
{
	uint64_t u;
	operator HANDLE() const
	{
		return (HANDLE)(DWORD_PTR)u;
	};
	operator DWORD_PTR() const
	{
		return (DWORD_PTR)u;
	};
	struct HANDLE2& operator=(HANDLE h)
	{
		u = (DWORD_PTR)h;
		return *this;
	};
};

struct STRPTR2
{
	// Number of CHARS, including terminator zero(s)
	DWORD cchCount;
	// FALSE - psz is valid, TRUE - offset is valid
	BOOL  bMangled;
	// The string itself
	union
	{
		// Used internally, invalid during call
		// Mangled/Demangled before and after call
		wchar_t* psz;
		// Must be 64bit to maintain struct size
		// During call this is an offset in BYTES from STRPTR
		uint64_t offset;
	};

	void   Set(wchar_t*&& ptrSrc, int cch = -1);
	LPBYTE Mangle(LPBYTE ptrDst);
	LPWSTR Demangle();
	operator LPCWSTR();
};

struct MSG64
{
	DWORD cbSize;
	DWORD nCount;
	struct MsgStr
	{
		UINT     message;
		HWND2    hwnd;
		uint64_t wParam;
		uint64_t lParam;
	} msg[1];
};

struct ThumbColor
{
	union
	{
		struct
		{
			unsigned int   ColorRGB : 24;
			unsigned int   ColorIdx : 5; // 5 bits, to support value '16' - automatic use of panel color
			unsigned int   UseIndex : 1; // TRUE - Use ColorRef, FALSE - ColorIdx
		};
		DWORD RawColor;
	};
};

struct ThumbSizes
{
	// сторона превьюшки или иконки
	int nImgSize; // Thumbs: 96, Tiles: 48
	// Сдвиг превьюшки/иконки вправо&вниз относительно полного поля файла
	int nSpaceX1, nSpaceY1; // Thumbs: 1x1, Tiles: 4x4
	// Размер "остатка" вправо&вниз после превьюшки/иконки. Здесь рисуется текст.
	int nSpaceX2, nSpaceY2; // Thumbs: 5x25, Tiles: 172x4
	// Расстояние между превьюшкой/иконкой и текстом
	int nLabelSpacing; // Thumbs: 0, Tiles: 4
	// Отступ текста от краев прямоугольника метки
	int nLabelPadding; // Thumbs: 0, Tiles: 1
	// Шрифт
	wchar_t sFontName[36]; // Tahoma
	int nFontHeight; // 14
};


enum PanelViewMode
{
	pvm_None = 0,
	pvm_Thumbnails = 1,
	pvm_Tiles = 2,
	pvm_Icons = 4,
	// следующий режим (если он будет) делать 4! (это bitmask)
	pvm_TurnOff = 256,
};

/* Основной шрифт в GUI */
struct ConEmuMainFont
{
	wchar_t sFontName[32];
	DWORD nFontHeight, nFontWidth, nFontCellWidth;
	DWORD nFontQuality, nFontCharSet;
	BOOL Bold, Italic;
	wchar_t sBorderFontName[32];
	DWORD nBorderFontWidth;
};


//DWORD nFarInterfaceSettings;
struct CEFarInterfaceSettings
{
	union
	{
		struct
		{
			int Reserved1         : 3; // FIS_CLOCKINPANELS и т.п.
			int ShowKeyBar        : 1; // FIS_SHOWKEYBAR
			int AlwaysShowMenuBar : 1; // FIS_ALWAYSSHOWMENUBAR
		};
		DWORD Raw;
	};
};
//DWORD nFarPanelSettings;
struct CEFarPanelSettings
{
	union
	{
		struct
		{
			int Reserved1          : 5; // FPS_SHOWHIDDENANDSYSTEMFILES и т.п
			int ShowColumnTitles   : 1; // FPS_SHOWCOLUMNTITLES
			int ShowStatusLine     : 1; // FPS_SHOWSTATUSLINE
			int Reserved2          : 4; // FPS_SHOWFILESTOTALINFORMATION и т.п.
			int ShowSortModeLetter : 1; // FPS_SHOWSORTMODELETTER
		};
		DWORD Raw;
	};
};


struct PanelViewSetMapping
{
	DWORD cbSize; // Struct size, на всякий случай

	/* Цвета и рамки */
	ThumbColor crBackground; // Фон превьюшки: RGB или Index

	int nPreviewFrame; // 1 (серая рамка вокруг превьюшки
	ThumbColor crPreviewFrame; // RGB или Index

	int nSelectFrame; // 1 (рамка вокруг текущего элемента)
	ThumbColor crSelectFrame; // RGB или Index

	/* антиалиасинг */
	UINT nFontQuality;

	/* Теперь разнообразные размеры */
	ThumbSizes Thumbs;
	ThumbSizes Tiles;

	/* Основной шрифт в GUI */
	struct ConEmuMainFont MainFont;

	// Прочие параметры загрузки
	BYTE  bLoadPreviews; // bitmask of PanelViewMode {1=Thumbs, 2=Tiles}
	bool  bLoadFolders;  // true - load infolder previews (only for Thumbs)
	UINT  nLoadTimeout;  // 15 sec

	UINT  nMaxZoom; // 600%
	bool  bUsePicView2; // true
	bool  bRestoreOnStartup;


	// AppDistinct палитру нужно получить отдельно
	// Здесь хранятся только "общие" цвета
	COLORREF crPalette[16], crFadePalette[16];


	//// Пока не используется
	//DWORD nCacheFolderType; // юзер/программа/temp/и т.п.
	//wchar_t sCacheFolder[MAX_PATH];
};


typedef BOOL (WINAPI* PanelViewInputCallback)(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult);
typedef union uPanelViewInputCallback
{
	uint64_t Reserved; // необходимо для выравнивания структур при x64 <--> x86
	PanelViewInputCallback f; //-V117
} PanelViewInputCallback_t;
typedef BOOL (WINAPI* PanelViewOutputCallback)(HANDLE hOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);
typedef union uPanelViewOutputCallback
{
	uint64_t Reserved; // необходимо для выравнивания структур при x64 <--> x86
	PanelViewOutputCallback f; //-V117
} PanelViewOutputCallback_t;
struct PanelViewText
{
	// Флаг используемости, выравнивание текста, и др.
#define PVI_TEXT_NOTUSED 0
#define PVI_TEXT_LEFT    1
#define PVI_TEXT_CENTER  2
#define PVI_TEXT_RIGHT   4
#define PVI_TEXT_SKIPSORTMODE 0x100 // только для имени колонки (оставить ФАРовскую буковку сортировки с ^)
	DWORD nFlags;
	// используется только младший байт - индексы консольных цветов
	DWORD bConAttr;
	// собственно текст
	wchar_t sText[128];
};
struct PanelViewInit
{
	DWORD cbSize;
	BOOL  bRegister, bVisible;
	HWND2 hWnd;
	BOOL  bLeftPanel;
	// Flags
#define PVI_COVER_NORMAL         0x000 // как обычно, располагаем прямоугольником, в который ФАР выводит файлы
#define PVI_COVER_SCROLL_CLEAR   0x001 // при отрисовке GUI очистит полосу прокрутки (заменит на вертикальную-двойную)
#define PVI_COVER_SCROLL_OVER    0x002 // PVI_COVER_NORMAL + правая рамка (где полоса прокрутки). Плагин должен отрисовать прокрутку сам
#define PVI_COVER_COLTITLE_OVER  0x004 // PVI_COVER_NORMAL + строка с заголовками колонок (если она есть)
#define PVI_COVER_FULL_OVER      0x008 // Вся РАБОЧАЯ область панели (то есть панель - рамка)
#define PVI_COVER_FRAME_OVER     0x010 // Вся область панели (включая рамку)
#define PVI_COVER_2PANEL_OVER    0x020 // Обе панели (полноэкранная)
#define PVI_COVER_CONSOLE_OVER   0x040 // Консоль целиком
	DWORD nCoverFlags;
	// FAR settings
	CEFarInterfaceSettings FarInterfaceSettings;
	CEFarPanelSettings FarPanelSettings;
	// Координаты всей панели (левой, правой, или fullscreen)
	RECT  PanelRect;
	// Разнообразные текстовые затиралки
	PanelViewText tPanelTitle, tColumnTitle, tInfoLine[3];
	// Это координаты прямоугольника, в котором реально располагается View
	// Координаты определяются в GUI. Единицы - консольные.
	RECT  WorkRect;
	// Callbacks, используются только в плагинах
	PanelViewInputCallback_t pfnPeekPreCall, pfnPeekPostCall, pfnReadPreCall, pfnReadPostCall;
	PanelViewOutputCallback_t pfnWriteCall;
	/* out */
	//PanelViewSetMapping ThSet;
	BOOL bFadeColors;
};



// События, при которых вызывается функция субплагина для перерисовки фона
enum PaintBackgroundEvents
{
	pbe_Common = 0,              // при изменении размера окна ConEmu/размера шрифта/палитры ConEmu/...
	pbe_PanelDirectory = 1,      // при смене папки
	//TODO, by request
	//bue_LeftDirChanged = 1,      // на левой панели изменена текущая папка
	//bue_RightDirChanged = 2,     // на правой панели изменена текущая папка
	//bue_FocusChanged = 4,        // фокус переключился между левой/правой панелью
};

enum PaintBackgroundPlaces
{
	// Используются при регистрации И отрисовке
	pbp_Panels = 1,
	pbp_Editor = 2,
	pbp_Viewer = 4,
	// Используются ТОЛЬКО как [OUT] при отрисовке // Reserved
	pbp_UserScreen  = 8,
	pbp_CommandLine = 16,
	pbp_KeyBar      = 32,
	pbp_MenuBar     = 64, // Верхняя строка - падающее меню
	pbp_StatusLine  = 128, // Статусная строка - редактор/вьювер
	// Этот код используется для завершения нити отрисовки
	pbp_Finalize    = 0x80000000,
	// Undefined
	pbp_None = 0,
};

#define BkPanelInfo_CurDirMax MAX_WIDE_PATH_LENGTH
#define BkPanelInfo_FormatMax MAX_PATH
#define BkPanelInfo_HostFileMax MAX_WIDE_PATH_LENGTH

struct BkPanelInfo
{
	BOOL bVisible;   // Наличие панели
	BOOL bFocused;   // В фокусе
	BOOL bPlugin;    // Плагиновая панель
	int  nPanelType; // enum PANELINFOTYPE
	wchar_t *szCurDir/*[MAX_WIDE_PATH_LENGTH]*/;    // Текущая папка на панели
	wchar_t *szFormat/*[MAX_PATH]*/; // Доступно только в FAR2, в FAR3 это может быть префикс, если "формат" плагином не определен
	wchar_t *szHostFile/*[MAX_WIDE_PATH_LENGTH]*/;  // Доступно только в FAR2
	RECT rcPanelRect; // Консольные координаты панели. В FAR 2+ с ключом /w верх может быть != {0,0}
};

struct PaintBackgroundArg
{
	DWORD cbSize;

	// указан при вызове RegisterBackground(rbc_Register)
	LPARAM lParam;

	// панели/редактор/вьювер
	enum PaintBackgroundPlaces Place;

	// Слой, в котором вызван плагин. По сути, это порядковый номер, 0-based
	// если (dwLevel > 0) плагин НЕ ДОЛЖЕН затирать фон целиком.
	DWORD dwLevel;
	// [Reserved] комбинация из enum PaintBackgroundEvents
	DWORD dwEventFlags;

	// Основной шрифт в ConEmu GUI
	struct ConEmuMainFont MainFont;
	// Палитра в ConEmu GUI
	COLORREF crPalette[16];
	// Reserved
	DWORD dwReserved[20];


	// DC для отрисовки фона. Изначально (для dwLevel==0) DC залит цветом фона crPalette[0]
	HDC hdc;
	// размер DC в пикселях
	int dcSizeX, dcSizeY;
	// Координаты панелей в DC (base 0x0)
	RECT rcDcLeft, rcDcRight;


	// Для облегчения жизни плагинам - текущие параметры FAR
	RECT rcConWorkspace; // Координаты рабочей области FAR. В FAR 2 с ключом /w верх может быть != {0,0}
	COORD conCursor; // положение курсора, или {-1,-1} если он не видим
	CEFarInterfaceSettings FarInterfaceSettings; // ACTL_GETINTERFACESETTINGS
	CEFarPanelSettings FarPanelSettings; // ACTL_GETPANELSETTINGS
	BYTE nFarColors[col_LastIndex]; // Массив цветов фара

	// Информация о панелях
	BOOL bPanelsAllowed;
	BkPanelInfo LeftPanel;
	BkPanelInfo RightPanel;

	// [OUT] Плагин должен указать, какие части консоли он "раскрасил" - enum PaintBackgroundPlaces
	DWORD dwDrawnPlaces;
};

typedef int (WINAPI* PaintConEmuBackground_t)(struct PaintBackgroundArg* pBk);

// Если функция вернет 0 - обновление фона пока не требуется
typedef int (WINAPI* BackgroundTimerProc_t)(LPARAM lParam);

enum RegisterBackgroundCmd
{
	rbc_Register   = 1, // Первичная регистрации "фонового" плагина
	rbc_Unregister = 2, // Убрать плагин из списка "фоновых"
	rbc_Redraw     = 3, // Если плагину требуется обновить фон - он зовет эту команду
};

// Аргумент для функции: int WINAPI RegisterBackground(RegisterBackgroundArg *pbk)
struct RegisterBackgroundArg
{ //-V802
	DWORD cbSize;
	enum RegisterBackgroundCmd Cmd; // DWORD
	HMODULE hPlugin; // Instance плагина, содержащего PaintConEmuBackground

	// Для дерегистрации всех калбэков достаточно вызвать {sizeof(RegisterBackgroundArg), rbc_Unregister, hPlugin}

	// Что вызывать для обновления фона.
	// Требуется заполнять только для Cmd==rbc_Register,
	// в остальных случаях команды игнорируются

	// Плагин может зарегистрировать несколько различных пар {PaintConEmuBackground,lParam}
	PaintConEmuBackground_t PaintConEmuBackground; // Собственно калбэк
	LPARAM lParam; // lParam будет передан в PaintConEmuBackground

	DWORD  dwPlaces;     // bitmask of PaintBackgroundPlaces
	DWORD  dwEventFlags; // bitmask of PaintBackgroundEvents

	// 0 - плагин предпочитает отрисовывать фон первым. Чем больше nSuggestedLevel
	// тем позднее может быть вызван плагин при обновлении фона
	DWORD  dwSuggestedLevel;

	// Необязательный калбэк таймера
	BackgroundTimerProc_t BackgroundTimerProc;
	// Рекомендованная частота опроса (ms)
	DWORD nBackgroundTimerElapse;
};

typedef int (WINAPI* RegisterBackground_t)(RegisterBackgroundArg *pbk);

typedef void (WINAPI* SyncExecuteCallback_t)(LONG_PTR lParam);


struct FarVersion
{
	union
	{
		DWORD dwVer;
		struct  // NOLINT(clang-diagnostic-nested-anon-types)
		{
			uint dwVerMinor : 15;
			uint Bis : 1;
			uint dwVerMajor : 16;
		};
	};
	DWORD dwBuild;

	DWORD dwBits; // 32/64

	// Macro language changed to Lua
	bool IsFarLua() const
	{
		return ((dwVerMajor > 3) || ((dwVerMajor == 3) && (dwBuild >= 2851)));
	};

	// Userscreen is now 'Desktop' window with index '0'
	bool IsDesktop() const
	{
		return ((dwVerMajor > 3) || ((dwVerMajor == 3) && (dwBuild >= 4039)));
	};
};


// Аргумент для функции OnConEmuLoaded
struct ConEmuLoadedArg
{
	DWORD cbSize;    // размер структуры в байтах
	DWORD nBuildNo;  // {Номер сборки ConEmu*10} (i.e. 1012070). Это версия плагина. В принципе, версия GUI может отличаться
	                 // YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	HMODULE hConEmu; // conemu.dll / conemu.x64.dll
	HMODULE hPlugin; // для информации - Instance этого плагина, в котором вызывается OnConEmuLoaded
	BOOL bLoaded;    // TRUE - при загрузке conemu.dll, FALSE - при выгрузке
	BOOL bGuiActive; // TRUE - консоль запущена из-под ConEmu, FALSE - standalone

	/* ***************** */
	/* Сервисные функции */
	/* ***************** */
	HWND (WINAPI *GetFarHWND)();
	HWND (WINAPI *GetFarHWND2)(BOOL abConEmuOnly);
	void (WINAPI *GetFarVersion)(FarVersion* pfv);
	BOOL (WINAPI *IsTerminalMode)();
	BOOL (WINAPI *IsConsoleActive)();
	int (WINAPI *RegisterPanelView)(PanelViewInit *ppvi);
	int (WINAPI *RegisterBackground)(RegisterBackgroundArg *pbk);
	// Activate console tab in ConEmu
	int (WINAPI *ActivateConsole)();
	// Synchronous execute CallBack in Far main thread (FAR2 use ACTL_SYNCHRO).
	// SyncExecute does not returns, until CallBack finished.
	// ahModule must be module handle, which contains CallBack
	int (WINAPI *SyncExecute)(HMODULE ahModule, SyncExecuteCallback_t CallBack, LONG_PTR lParam);
};

// другие плагины могут экспортировать функцию "OnConEmuLoaded"
// при активации плагина ConEmu (при запуске фара из-под ConEmu)
// эта функция ("OnConEmuLoaded") будет вызвана, в ней плагин может
// зарегистрировать калбэки для обновления Background-ов
// Внимание!!! Эта же функция вызывается при ВЫГРУЗКЕ conemu.dll,
// при выгрузке hConEmu и все функции == NULL
typedef void (WINAPI* OnConEmuLoaded_t)(struct ConEmuLoadedArg* pConEmuInfo);

enum GuiLoggingType
{
	glt_None = 0,
	glt_Processes = 1, // CreateProcess in consoles
	glt_Input = 2,     // Keyboard/Mouse input
	glt_Commands = 3,  // Pipe communications
	glt_Shell = 4,     // glt_Processes + injects/modules actions
	// glt_Debugger,
	// glt_Ansi
	// glt_Files,
};

enum ComSpecType
{
	cst_AutoTccCmd = 0,
	cst_EnvVar     = 1,
	cst_Cmd        = 2,
	cst_Explicit   = 3,
	//
	cst_Last = cst_Explicit
};

enum ComSpecBits
{
	csb_SameOS     = 0,
	csb_SameApp    = 1,
	csb_x32        = 2,
	csb_x64        = 3, // только для аргументов функций
	//
	csb_Last = csb_x32,
};

typedef DWORD CEAdd2Path;
const CEAdd2Path
	CEAP_AddConEmuBaseDir      = 0x00000001,
	CEAP_AddConEmuExeDir       = 0x00000002,
	CEAP_AddAll                = CEAP_AddConEmuBaseDir|CEAP_AddConEmuExeDir,
	CEAP_None                  = 0;

struct ConEmuComspec
{
	ComSpecType  csType;
	ComSpecBits  csBits;
	BOOL         isUpdateEnv;
	CEAdd2Path   AddConEmu2Path;
	BOOL         isAllowUncPaths;
	wchar_t      ComspecExplicit[MAX_PATH]; // этот - хранится в настройке
	wchar_t      Comspec32[MAX_PATH]; // развернутые, готовые к использованию
	wchar_t      Comspec64[MAX_PATH]; // развернутые, готовые к использованию
	wchar_t      ConEmuBaseDir[MAX_PATH]; // БЕЗ завершающего слеша. Папка содержит ConEmuC.exe, ConEmuHk.dll, ConEmu.xml
	wchar_t      ConEmuExeDir[MAX_PATH]; // БЕЗ завершающего слеша. Папка содержит ConEmu.exe, Far.exe (optional)
};


enum class ConEmuUseInjects : DWORD
{
	DontUse = 0,
	Use = 1,
};

namespace ConEmu {

enum class ConsoleFlags : uint32_t
{
	Empty = 0,

	// DosBox is installed, ready to use
	DosBox = 0x00000001,

	// enabled "TrueMod support"
	UseTrueColor = 0x00000002,
	// enabled ANSI sequences processing
	ProcessAnsi = 0x00000004,

	// use clink command line extension (ReadConsole). 1 - old version (0.1.1)
	UseClink_1 = 0x00000008,
	// use clink command line extension (ReadConsole) - 2 - new version
	UseClink_2 = 0x00000010,
	UseClink_Any = UseClink_1 | UseClink_2,

	// Consume lesser resources when ConEmu or the console is not focused/active (see also RetardInactivePanes)
	SleepInBackground = 0x00000020,

	// When ConEmuC tries to debug process tree - force disable DEBUG_PROCESS/DEBUG_ONLY_THIS_PROCESS flags when creating sub children
	BlockChildDbg = 0x00000040,

	// Suppress output of char(7) to console (which produces annoying bell sound)
	SuppressBells = 0x00000080,

	// Set up "SetUnhandledExceptionFilter(CreateDumpOnException)" in alternative servers too
	ConExcHandler = 0x00000100,

	// Enable processing of '-new_console' and '-cur_console' switches in your shell prompt, scripts etc. started in ConEmu tabs
	ProcessNewCon = 0x00000200,
	// Use "start xxx.exe" to start new tab
	ProcessCmdStart = 0x00000400,

	// Show real console
	RealConVisible = 0x00000800,

	// Return 0 bytes from ReadConsole if Ctrl-Z was only entered on the line
	ProcessCtrlZ = 0x00001000,

	// Retard inactive panes (see also SleepInBackground)
	RetardInactivePanes = 0x00002000,

	// AnsiExecutionPerm::ansi_Allowed
	AnsiExecAny = 0x00004000,
	// AnsiExecutionPerm::ansi_CmdOnly
	AnsiExecCmd = 0x00008000,
};

inline ConsoleFlags operator|(const ConsoleFlags e1, const ConsoleFlags e2)
{
	return static_cast<ConsoleFlags>(static_cast<uint32_t>(e1) | static_cast<uint32_t>(e2));
}

inline bool operator&(const ConsoleFlags e1, const ConsoleFlags e2)
{
	return (static_cast<uint32_t>(e1) & static_cast<uint32_t>(e2)) != static_cast<uint32_t>(ConsoleFlags::Empty);
}

}

template <typename T>
void SetConEmuFlags(T& v, const T mask, const T flag)
{
	v = static_cast<T>((static_cast<uint64_t>(v) & ~static_cast<uint64_t>(mask)) | static_cast<uint64_t>(flag));
}

typedef DWORD ConEmuConsoleStateFlags;
const ConEmuConsoleStateFlags
	ccf_Active   = 1,
	ccf_Visible  = 2,
	ccf_ChildGui = 4,
	ccf_None     = 0;

struct ConEmuConsoleInfo
{
	ConEmuConsoleStateFlags Flags;
	HWND2 Console;
	HWND2 DCWindow;
	HWND2 ChildGui;
	DWORD ServerPID;
	HWND2 BackWindow;
};

// CEGUIINFOMAPNAME L"ConEmuGuiInfoMapping.%u" ( % == dwGuiProcessId )
struct ConEmuGuiMapping
{
	DWORD    cbSize;
	DWORD    nProtocolVersion; // == CESERVER_REQ_VER
	DWORD    nChangeNum; // incremental change number of structure
	HWND2    hGuiWnd; // основное (корневое) окно ConEmu
	DWORD    nGuiPID; // PID ConEmu.exe

	DWORD    nLoggingType; // enum GuiLoggingType

	wchar_t  sConEmuExe[MAX_PATH+1]; // полный путь к ConEmu.exe (GUI)
	// --> ComSpec.ConEmuBaseDir:  wchar_t  sConEmuDir[MAX_PATH+1]; // БЕЗ завершающего слеша. Папка содержит ConEmu.exe
	// --> ComSpec.ConEmuBaseDir:  wchar_t  sConEmuBaseDir[MAX_PATH+1]; // БЕЗ завершающего слеша. Папка содержит ConEmuC.exe, ConEmuHk.dll, ConEmu.xml
	wchar_t  sConEmuArgs[MAX_PATH*2];


	ConEmuUseInjects   useInjects;

	ConEmu::ConsoleFlags Flags;
	//BOOL     bUseTrueColor; // включен флажок "TrueMod support"
	//BOOL     bProcessAnsi;  // ANSI X3.64 & XTerm-256-colors Support
	//DWORD    bUseClink;     // использовать расширение командной строки (ReadConsole). 0 - нет, 1 - старая версия (0.1.1), 2 - новая версия

	//BOOL     bSleepInBackg; // Sleep in background

	// Gui is In focus or Not
	BOOL  bGuiActive;
	// Tick, when hActiveCons/bGuiActive was changed
	DWORD dwActiveTick;
	// Information about open consoles
	ConEmuConsoleInfo Consoles[MAX_CONSOLE_COUNT];

	/* Main font in GUI */
	struct ConEmuMainFont MainFont;

	// DosBox
	//BOOL     bDosBox; // наличие DosBox
	//wchar_t  sDosBoxExe[MAX_PATH+1]; // полный путь к DosBox.exe
	//wchar_t  sDosBoxEnv[8192]; // команды загрузки (mount, и пр.)

	ConEmuComspec ComSpec;

	// AppID. It's formed of some critical parameters, to ensure
	// that on ‘single instance’ we would not pass new tab into
	// wrong application instance.
	// For example, running "ConEmu -quake" must not create new tab
	// in the existing ConEmu ‘non-quake’ instance.
	wchar_t AppID[40];
};

typedef unsigned int CEFarWindowType;
static const CEFarWindowType
	fwt_Any            = 0,
	fwt_Panels         = 1,       // _ASSERTE(fwt_Panels == WTYPE_PANELS)
	fwt_Viewer         = 2,       // _ASSERTE(fwt_Viewer == WTYPE_VIEWER)
	fwt_Editor         = 3,       // _ASSERTE(fwt_Viewer == WTYPE_EDITOR)
	fwt_TypeMask       = 0x00FF,

	fwt_Elevated       = 0x00100,
	fwt_NonElevated    = 0x00200, // Аргумент для поиска окна
	fwt_ModalFarWnd    = 0x00400, // Бывший ConEmuTab.Modal
	fwt_NonModal       = 0x00800, // Аргумент для поиска окна
	fwt_PluginRequired = 0x01000, // Аргумент для поиска окна
	fwt_ActivateFound  = 0x02000, // Активировать найденный таб. Аргумент для поиска окна
	fwt_Disabled       = 0x04000, // Таб заблокирован другим модальным табом (или диалогом?)
	fwt_Renamed        = 0x08000, // Таб был принудительно переименован пользователем (?)
	fwt_ActivateOther  = 0x10000, // Аргумент для поиска окна. Активировать найденный таб если он НЕ в активной консоли
	fwt_CurrentFarWnd  = 0x20000, // Бывший ConEmuTab.Current
	fwt_ModifiedFarWnd = 0x40000, // Бывший ConEmuTab.Modified
	fwt_FarFullPathReq = 0x80000, // Аргумент для поиска окна
	fwt_Highlighted   = 0x100000, // Подсветить таб

	fwt_UpdateFlags    = fwt_Elevated|fwt_ModalFarWnd|fwt_Disabled|fwt_CurrentFarWnd|fwt_ModifiedFarWnd|fwt_Highlighted;
	;

//TODO("Restrict CONEMUTABMAX to 128 chars. Only filename, and may be ellipsed...");
#define CONEMUTABMAX 0x400
struct ConEmuTab
{
	int  Pos;
	int  Current;
	CEFarWindowType Type; // (Panels=1, Viewer=2, Editor=3) |(Elevated=0x100) |(NotElevated=0x200) |(Modal=0x400)
	int  Modified;
	int  Modal;
	int  EditViewId;
	wchar_t Name[CONEMUTABMAX];
	//  int  Modified;
	//  int isEditor;
};

struct CESERVER_REQ_CONEMUTAB
{
	DWORD nTabCount;
	BOOL  bMacroActive;
	BOOL  bMainThread;
	int   CurrentType; // WTYPE_PANELS / WTYPE_VIEWER / WTYPE_EDITOR
	int   CurrentIndex; // для удобства, индекс текущего окна в tabs
	ConEmuTab tabs[1];
};

struct CESERVER_REQ_CONEMUTAB_RET
{
	BOOL  bNeedPostTabSend;
	BOOL  bNeedResize;
	COORD crNewSize;
};

struct ForwardedPanelInfo
{
	RECT ActiveRect;
	RECT PassiveRect;
	int ActivePathShift; // сдвиг в этой структуре в байтах
	int PassivePathShift; // сдвиг в этой структуре в байтах
	BOOL NoFarConsole;
	wchar_t szDummy[2];
	union //x64 ready
	{
		WCHAR* pszActivePath/*[MAX_PATH+1]*/; //-V117
		uint64_t Reserved1;
	};
	union //x64 ready
	{
		WCHAR* pszPassivePath/*[MAX_PATH+1]*/; //-V117
		uint64_t Reserved2;
	};
};

struct ForwardedFileInfo
{
	WCHAR Path[MAX_PATH+1];
};


struct CESERVER_REQ_HDR
{
	uint32_t cbSize;     // Не size_t(!), а именно DWORD, т.к. пакетами обмениваются и 32<->64 бит между собой.
	uint32_t nVersion;   // CESERVER_REQ_VER
	BOOL     bAsync;     // не посылать "ответ", сразу закрыть пайп
	CECMD    nCmd;       // DWORD
	uint32_t nSrcThreadId;
	uint32_t nSrcPID;
	uint32_t nCreateTick;
	uint32_t nBits;      // битность вызывающего процесса
	uint32_t nLastError; // последний GetLastError() при отправке пакета
	uint32_t IsDebugging;
	HANDLE2  hModule;
};

#define CHECK_CMD_SIZE(pCmd,data_size) ((pCmd)->hdr.cbSize >= (sizeof(CESERVER_REQ_HDR) + (data_size)))


struct CESERVER_CHAR_HDR
{
	int   nSize;    // размер структуры динамический. Если 0 - значит прямоугольник is NULL
	COORD cr1, cr2; // WARNING: Это АБСОЛЮТНЫЕ координаты (без учета прокрутки), а не экранные.
};

struct CESERVER_CHAR
{
	CESERVER_CHAR_HDR hdr; // фиксированная часть
	WORD  data[2];  // variable(!) length
};

//CECONOUTPUTNAME     L"ConEmuLastOutputMapping.%08X"    // --> CESERVER_CONSAVE_MAPHDR ( %X == (DWORD)ghConWnd )
struct CESERVER_CONSAVE_MAPHDR
{
	CESERVER_REQ_HDR hdr;    // Для унификации
	CONSOLE_SCREEN_BUFFER_INFO info; // чтобы знать текущий размер текущего буфера
	DWORD   MaxCellCount;
	int     CurrentIndex;
	wchar_t sCurrentMap[64]; // CECONOUTPUTITEMNAME
};

//CECMD_CONSOLEFULL
//CECONOUTPUTITEMNAME L"ConEmuLastOutputMapping.%08X.%u" // --> CESERVER_CONSAVE_MAP ( %X == (DWORD)ghConWnd, %u = CESERVER_CONSAVE_MAPHDR.nCurrentIndex )
struct CESERVER_CONSAVE_MAP
{
	CESERVER_REQ_HDR hdr; // Для унификации
	CONSOLE_SCREEN_BUFFER_INFO info;

	DWORD MaxCellCount;
	int   CurrentIndex;
	BOOL  Succeeded;

	//TODO: Store TrueColor buffer?

	CHAR_INFO Data[1];
};



struct CESERVER_REQ_RGNINFO
{
	DWORD dwRgnInfoSize;
	CESERVER_CHAR RgnInfo;
};

struct CESERVER_REQ_FULLCONDATA
{
	DWORD dwRgnInfoSize_MustBe0; // must be 0
	DWORD dwOneBufferSize; // may be 0
	wchar_t Data[300]; // Variable length!!!
};

struct CEFAR_SHORT_PANEL_INFO
{
	int   PanelType;
	int   Plugin;
	RECT  PanelRect;
	//int   ItemsNumber;
	//int   SelectedItemsNumber;
	//int   CurrentItem;
	//int   TopPanelItem;
	int   Visible;
	int   Focus;
	//int   ViewMode;
	//int   ShortNames;
	//int   SortMode;
	int   ColumnNames;
	int   StatusLines;
	unsigned __int64 Flags;
};

struct CEFAR_PANELTABS
{
	int   SeparateTabs; // если -1 - то умолчание
	int   ButtonColor;  // если -1 - то умолчание
};

typedef unsigned int CEFAR_MACRO_AREA;
static const CEFAR_MACRO_AREA
	fma_Unknown  = 0x00000000,
	fma_Panels   = 0x00000001,
	fma_Viewer   = 0x00000002,
	fma_Editor   = 0x00000003,
	fma_Dialog   = 0x00000004 //-V112
	;


//TODO: CEFAR_INFO_MAPPING.nFarColors - subject to change!!!
struct CEFAR_INFO_MAPPING
{
	DWORD cbSize;
	DWORD nFarInfoIdx; // Incremented when ReloadFarInfo updates the mapping
	FarVersion FarVer;
	DWORD nProtocolVersion; // == CESERVER_REQ_VER
	DWORD nFarPID, nFarTID;
	BYTE nFarColors[col_LastIndex]; // array with Far colors
	CEFarInterfaceSettings FarInterfaceSettings;
	CEFarPanelSettings FarPanelSettings;
	BOOL  bFarPanelAllowed; // FCTL_CHECKPANELSEXIST
	// Current Area in Far Manager
	CEFAR_MACRO_AREA nMacroArea;
	BOOL bMacroActive;
	// Information about Panels, may be absent
	BOOL bFarPanelInfoFilled;
	BOOL bFarLeftPanel, bFarRightPanel;
	CEFAR_SHORT_PANEL_INFO FarLeftPanel, FarRightPanel; // FCTL_GETPANELSHORTINFO,...
	BOOL bViewerOrEditor; // used in RgnDetect
	DWORD nFarConsoleMode;
	BOOL bBufferSupport; // FAR2+ with "/w" key
	CEFAR_PANELTABS PanelTabs; // PanelTabs plugin settings
	// Some string resources which GUI uses to parse the console contents
	wchar_t sLngEdit[64]; // "edit"
	wchar_t sLngView[64]; // "view"
	wchar_t sLngTemp[64]; // "{Temporary panel"

	// TODO: replace with atomic?
	LONG nPanelDirIdx; // Separately of nFarInfoIdx. Incremented when StorePanelDirs is called

	DWORD compareSentinel; // Below this variable ReloadFarInfo does not compare for changes

	DWORD activeTtl; // GetTickCount when Far should be considered as "Inactive"

	// Far current panel directories
	wchar_t sActiveDir[MAX_WIDE_PATH_LENGTH], sPassiveDir[MAX_WIDE_PATH_LENGTH];
};


// Limited logging of console contents (same output as processed by ConEmu::ConsoleFlags::ProcessAnsi)
// Initialized during CECMD_SRVSTARTSTOP
struct ConEmuAnsiLog
{
	BOOL    Enabled;
	BOOL    LogAnsiCodes;
	// Full path with name of log-file
	wchar_t Path[MAX_PATH];
};


// CECONMAPNAME L"ConEmuFileMapping.%08X" ( % == (DWORD)ghConWnd )
struct CESERVER_CONSOLE_MAPPING_HDR
{
	DWORD cbSize;
	DWORD nLogLevel;
	COORD crMaxConSize;
	DWORD bDataReady;  // TRUE, после того как сервер успешно запустился и готов отдавать данные
	HWND2 hConWnd;     // !! положение hConWnd и nGuiPID не менять !!
	DWORD nServerPID;  //
	DWORD nGuiPID;     // !! на них ориентируется PicViewWrapper   !!
	//
	DWORD Reserved1; //bConsoleActive;
	DWORD nProtocolVersion; // == CESERVER_REQ_VER
	DWORD Reserved2; //bThawRefreshThread; // FALSE - увеличивает интервал опроса консоли (GUI теряет фокус)
	//
	DWORD nActiveFarPID; // PID последнего активного фара
	//
	DWORD nServerInShutdown; // GetTickCount() начала закрытия сервера
	//
	wchar_t  sConEmuExe[MAX_PATH+1]; // полный путь к ConEmu.exe (GUI)
	// --> ComSpec.ConEmuBaseDir:  wchar_t  sConEmuBaseDir[MAX_PATH+1]; // БЕЗ завершающего слеша. Папка содержит ConEmuC.exe, ConEmuHk.dll, ConEmu.xml
	//
	DWORD nAltServerPID;  //
	DWORD ActiveServerPID() const { return nAltServerPID ? nAltServerPID : nServerPID; };

	/// When started "ConEmuC -std -c far.exe" connector shall pause reading input
	DWORD stdConBlockingPID;

	// Root(!) ConEmu window
	HWND2 hConEmuRoot;
	// DC ConEmu window
	HWND2 hConEmuWndDc;
	// Back ConEmu window
	HWND2 hConEmuWndBack;

	DWORD nLoggingType;  // enum GuiLoggingType
	ConEmuUseInjects useInjects;

	ConEmu::ConsoleFlags Flags;
	//BOOL  bDosBox;       // DosBox установлен, можно пользоваться
	//BOOL  bUseTrueColor; // включен флажок "TrueMod support"
	//BOOL  bProcessAnsi;  // ANSI X3.64 & XTerm-256-colors Support
	//DWORD bUseClink;     // использовать расширение командной строки (ReadConsole). 0 - нет, 1 - старая версия (0.1.1), 2 - новая версия

	// Limited logging of console contents (same output as processed by ConEmu::ConsoleFlags::ProcessAnsi)
	ConEmuAnsiLog AnsiLog;

	// Разрешенный размер видимой области
	BOOL  bLockVisibleArea;
	COORD crLockedVisible;
	// И какая прокрутка допустима
	RealBufferScroll rbsAllowed; // пока любая - rbs_Any

	// Specified Tab Icon (full path)
	wchar_t sIcon[MAX_PATH+1];

	ConEmuComspec ComSpec;
};

enum class TerminalConfirmClose : uint32_t;

// CEINSIDEMAPNAMEP L"ConEmuDefTermInside.%u"   ( % == PID of the root window, where ConEmu is integrated into )
// CEINSIDEMAPNAMEW L"ConEmuDefTermInside.%08X" ( % == the root window, where ConEmu is integrated into )
struct CONEMU_INSIDE_MAPPING
{
	/// CONEMU_INSIDE_MAPPING size
	DWORD cbSize;
	/// CESERVER_REQ_VER, changed on each struct modification
	DWORD nProtocolVersion;
	/// PID of ConEmu.exe
	DWORD nGuiPID;
};

// CEDEFTERMMAPNAME L"ConEmuDefTermInside.%u" ( % == PID of ConEmu process which is integrated into some app )
struct CONEMU_INSIDE_DEFTERM_MAPPING
{
	/// CONEMU_INSIDE_DEFTERM_MAPPING size
	DWORD cbSize;
	/// CESERVER_REQ_VER, changed on each struct modification
	DWORD nProtocolVersion;
	/// Full path to ConEmu.exe (GUI)
	wchar_t  sConEmuExe[MAX_PATH+1];
	/// NO trailing slash. Contains ConEmuC.exe, ConEmuHk.dll, ConEmu.xml
	wchar_t  sConEmuBaseDir[MAX_PATH];
	/// PID of ConEmu.exe (same as used in CEDEFTERMMAPNAME)
	DWORD nGuiPID;
	/// Root(!) ConEmu window
	HWND2 hConEmuRoot;

	/// Set of in-console flags
	ConEmu::ConsoleFlags flags;

	/// Default terminal is enabled
	BOOL bUseDefaultTerminal;
	/// Soft mode, no injects
	BOOL isDefaultTerminalNoInjects;
	/// Debug purposes, enable logging in DefTerm
	BOOL isDefaultTerminalDebugLog;
	/// "Press Enter to close console". 0 - Auto, 1 - Always, 2 - Never
	TerminalConfirmClose nDefaultTerminalConfirmClose;
	// "|" delimited
	wchar_t defaultTerminalApps[MAX_PATH];
};


typedef DWORD CEActiveAppFlags;
const CEActiveAppFlags
	caf_Cygwin1      = 0x00000001,
	caf_Msys1        = 0x00000004,
	caf_Msys2        = 0x00000008,
	caf_Clink        = 0x00000010,
	caf_Standard     = 0;


// CECONAPPMAPNAME L"ConEmuAppMapping.%08X" ( % == (DWORD)ghConWnd )
struct CESERVER_CONSOLE_APP_MAPPING
{
	DWORD cbSize;
	DWORD nProtocolVersion; // == CESERVER_REQ_VER

	// Under construction, some fields may be void

	// Updated *before* and *after* ReadConsoleInput (powershell especially)
	DWORD nReadConsoleInputPID;

	// Updated *before* and *after* ReadConsole/ReadFile
	DWORD nReadConsolePID;

	// Updated *before* ReadConsoleInput/ReadConsole/ReadFile
	// It may be zeroed only on normal process exit
	DWORD nLastReadInputPID;

	// It's rather problematic to find where input prompt is started
	// Especially because console contents may be scrolled on typing long commands
	// So, we set RowID (ConsoleMixAttr.h) for cursor row and its predecessor
	WORD  nPreReadRowID[2];
	// Where cursor was located when we set RowID marks
	CONSOLE_SCREEN_BUFFER_INFO csbiPreRead;

	// Active application flags (shell or other current interactive process)
	CEActiveAppFlags nActiveAppFlags;

	// Let know GUI how many rows were processed by console applications
	// This is actually the maximum row number where any write operation was executed
	LONG nLastConsoleRow;

	// CECONEMUROOTTHREAD often fails on cygwin/msys fork
	// This will be more proper way to detect if ConEmuHk
	// is loaded in main thread and snapshotting in the
	// GetMainThreadId is not required
	MCircular<DWORD,256> HookedPids;
};

struct TOPLEFTCOORD
{
	int y;
	int x;

	bool isLocked() const { return (y >= 0 || x >= 0); };
	void Reset() { y = x = -1; };
	bool Equal(const TOPLEFTCOORD& tl) const { return (tl.x==x && tl.y==y); };
};

struct CESERVER_REQ_CONINFO
{
	TOPLEFTCOORD TopLeft;
	BOOL VirtualOnly; // if true - don't call SetConsoleWindowInfo
};

enum CEPauseCmd
{
	CEPause_Switch  = 0,
	CEPause_On      = 1,
	CEPause_Off     = 2,
	CEPause_Unknown = 3,
};

typedef DWORD CECI_FLAGS;
const CECI_FLAGS
	CECI_Paused = 1,
	CECI_None   = 0;

// Assigned on Startup or by SetConsoleScreenBufferInfoEx
struct CESERVER_CONSOLE_PALETTE
{
	BOOL bPalletteLoaded;
	WORD wAttributes, wPopupAttributes;
	COLORREF ColorTable[16];
};

struct CESERVER_REQ_CONSOLE_STATE
{
	HWND2 hConWnd;
	DWORD nPacketId;
	DWORD nSrvUpdateTick; // GetTickCount(), когда консоль была считана в последний раз в сервере
	DWORD nReserved0; //DWORD nInputTID;
	DWORD nProcesses[CONSOLE_PROCESSES_MAX/*20*/];
	DWORD dwCiSize;
	CONSOLE_CURSOR_INFO ci;
	DWORD dwConsoleCP;
	DWORD dwConsoleOutputCP;
	WORD  dwConsoleInMode;
	WORD  dwConsoleOutMode;
	DWORD dwSbiSize;
	CONSOLE_SCREEN_BUFFER_INFO sbi; // srWindow - "видимое" в GUI окно
	TOPLEFTCOORD TopLeft;
	COORD crWindow;
	SMALL_RECT srRealWindow; // Те реальные координаты, которые видимы в RealConsole (а не то, что видимо в GUI окне)
	LONG lastConsoleRow; // Last observed CESERVER_CONSOLE_APP_MAPPING::nLastConsoleRow, may have lags
	COORD crMaxSize; // Максимальный размер консоли в символах (для текущего выбранного шрифта)
	CECI_FLAGS Flags;
	DWORD nDataCount; // Для удобства - количество ячеек (data)
	CESERVER_CONSOLE_PALETTE ConsolePalette;

	// Usually followed by
	// CHAR_INFO  data[N];
};

//typedef struct tag_CESERVER_REQ_CONINFO_DATA {
//	CESERVER_REQ_HDR cmd;
//	COORD      crMaxSize;
//	CHAR_INFO  Buf[1];
//} CESERVER_REQ_CONINFO_DATA;

struct CESERVER_REQ_CONINFO_FULL
{
	DWORD cbMaxSize;    // размер всего буфера CESERVER_REQ_CONINFO_FULL (скорее всего будет меньше реальных данных)
	BOOL  bDataChanged; // Выставляется в TRUE, при изменениях содержимого консоли (а не только положение курсора...)
	CESERVER_CONSOLE_MAPPING_HDR  hdr;
	CESERVER_REQ_CONSOLE_STATE ConState;
	CHAR_INFO  data[1];
};

//typedef struct tag_CESERVER_REQ_CONINFO {
//	CESERVER_CONSOLE_MAPPING_HDR inf;
//    union {
//	/* 9*/DWORD dwRgnInfoSize;
//	      CESERVER_REQ_RGNINFO RgnInfo;
//    /*10*/CESERVER_REQ_FULLCONDATA FullData;
//	};
//} CESERVER_REQ_CONINFO;

struct CESERVER_REQ_SETSIZE
{
	USHORT nBufferHeight; // 0 или высота буфера (режим с прокруткой)
	COORD  size;
	//TOPLEFTCOORD TopLeft;
	//SHORT  nSendTopLine;  // -1 или 0based номер строки зафиксированной в GUI (только для режима с прокруткой)
	//SMALL_RECT rcWindow;  // координаты видимой области для режима с прокруткой
	DWORD  dwFarPID;      // Если передано - сервер должен сам достучаться до FAR'а и обновить его размер через плагин ПЕРЕД возвратом
};

struct CESERVER_REQ_OUTPUTFILE
{
	BOOL  bUnicode;
	WCHAR szFilePathName[MAX_PATH+1];
};

struct CESERVER_REQ_RETSIZE
{
	DWORD nNextPacketId;
	COORD crMaxSize; // Current maximum possible size of console window
	CONSOLE_SCREEN_BUFFER_INFO SetSizeRet;
};

enum SingleInstanceShowHideType
{
	sih_None = 0,
	sih_ShowMinimize = 1,
	sih_ShowHideTSA = 2,
	sih_Show = 3,
	sih_SetForeground = 4,
	sih_HideTSA = 5,
	sih_Minimize = 6,
	sih_AutoHide = 7,
	sih_QuakeShowHide = 8,
	sih_StartDetached = 9,
};

struct CESERVER_REQ_NEWCMD // CECMD_NEWCMD
{
	HWND2   hFromConWnd;
	HWND2   hFromDcWnd;  // ConWnd may differ from parent process, but ENV_CONEMUDRAW_VAR_W would be inherited
	SingleInstanceShowHideType ShowHide;
	BYTE isAdvLogging;
	BYTE Reserved;
	wchar_t szConEmu[MAX_PATH]; // Для идентификации, чтобы можно было выполнять команду в instance по тому же пути (путь к папке с ConEmu.exe без слеша)
	wchar_t szCurDir[MAX_PATH];
	// Внимание! Может содержать параметр -new_console. GUI его должен вырезать перед запуском сервера!
	//wchar_t szCommand[1]; // На самом деле - variable_size !!!
	DWORD cchCommand;
	DWORD cchEnvStrings;
	DWORD ptrDataStart;
	wchar_t* GetCommand();
	wchar_t* GetEnvStrings();
	void SetCommand(LPCWSTR asCommand);
	void SetEnvStrings(LPCWSTR asStrings, DWORD cchLenZZ);
};

struct CESERVER_REQ_SETFONT
{
	DWORD cbSize; // страховка
	int inSizeY;
	int inSizeX;
	wchar_t sFontName[LF_FACESIZE];
};

// изменились настройки GUI (шрифт), размер окна ConEmu, или еще что-то
struct CESERVER_REQ_GUICHANGED
{
	DWORD cbSize; // страховка
	DWORD nGuiPID;
	HWND2 hLeftView, hRightView;
};

// CMD_OPENEDITORLINE. Открыть в редакторе файл и перейти на строку (обработка ошибок компилятора)
struct CESERVER_REQ_FAREDITOR
{
	DWORD cbSize; // страховка
	int nLine;    // 1-based
	int nColon;   // 1-based
	wchar_t szFile[MAX_PATH+1];
};

// CECMD_GETALLTABS. Вернуть список всех открытых табов
struct CESERVER_REQ_GETALLTABS
{
	int Count;
	struct TabInfo
	{
		bool    ActiveConsole;
		bool    ActiveTab;
		bool    Disabled;
		int     ConsoleIdx; // 0-based
		int     TabNo;      // 0-based
		int     FarWindow;  // 0-based
		wchar_t Title[MAX_PATH]; // Если не влезет - то и фиг с ним
	} Tabs[1]; // Variable length
};

//CECMD_SETGUIEXTERN
struct CESERVER_REQ_SETGUIEXTERN
{
	BOOL bExtern;  // TRUE - вынести приложение наружу из вкладки ConEmu, FALSE - вернуть во вкладку
	BOOL bDetach;  // Отцепиться и забыть про ConEmu
	//RECT rcOldPos; // Если bDetach - здесь передается "старое" положение окна
};

enum SrvStartStopType
{
	srv_Started = 1,
	srv_Stopped = 101,
};

// CECMD_SRVSTARTSTOP
struct CESERVER_REQ_SRVSTARTSTOP
{
	SrvStartStopType Started;
	HWND2 hConWnd;
	DWORD dwKeybLayout;
	int nShellExitCode;
};

enum StartStopType
{
	sst_ServerStart    = 0,
	sst_AltServerStart = 1,
	sst_ServerStop     = 2,
	sst_AltServerStop  = 3,
	sst_ComspecStart   = 4,
	sst_ComspecStop    = 5,
	sst_AppStart       = 6,
	sst_AppStop        = 7,
	sst_App16Start     = 8,
	sst_App16Stop      = 9,
};
struct CESERVER_REQ_STARTSTOP
{
	StartStopType nStarted;
	HWND2 hWnd; // при передаче В GUI - консоль, при возврате в консоль - GUI
	DWORD dwAID; // ConEmu internal ID of started CRealConsole
	DWORD dwPID; //, dwInputTID;
	DWORD nSubSystem; // 255 для DOS программ, 0x100 - аттач из FAR плагина
	DWORD nImageBits; // 16/32/64
	BOOL  bRootIsCmdExe;
	BOOL  bUserIsAdmin;
	BOOL  bMainServerClosing; // if (nStarted == sst_AltServerStop)
	DWORD dwKeybLayout; // Только при запуске сервера
	// А это приходит из консоли, вдруг консольная программа успела поменять размер буфера
	CONSOLE_SCREEN_BUFFER_INFO sbi;
	// Максимальный размер консоли на текущем шрифте
	COORD crMaxSize;
	// Только ComSpec
	BOOL  bWasBufferHeight;
	// "-cur_console:h<N>" для ComSpec
	BOOL  bForceBufferHeight;
	DWORD nForceBufferHeight;
	// Только при аттаче. Может быть NULL-ом
	HANDLE2 hServerProcessHandle;
	// При завершении
	DWORD nOtherPID; // Для RunMode::RM_COMSPEC - PID завершенного процесса (при sst_ComspecStop)
	// Для информации и удобства (GetModuleFileName(0))
	wchar_t sModuleName[MAX_PATH+1];
	// Reserved
	DWORD nReserved0[17];
	// Create background tab, when attaching new console
	BOOL bRunInBackgroundTab;
	// При запуске в режиме RunMode::RM_COMSPEC, сохранение "длинного вывода"
	DWORD nParentFarPID;
	// После детача/аттача мог остаться "альтернативный" сервер
	DWORD nAltServerPID;
	// Если был успешный вызов функций типа ReadConsole/ReadConsoleInput
	BOOL bWasSucceededInRead;
	// Self console palette? Useful after Win+G
	CESERVER_CONSOLE_PALETTE Palette;
	// Specified Tab Icon (full path)
	wchar_t sIcon[MAX_PATH+1];
	// CmdLine
	wchar_t sCmdLine[1]; // variable length
};

// CECMD_ONCREATEPROC
struct CESERVER_REQ_ONCREATEPROCESS
{
	DWORD   cbStructSize;
	int     nSourceBits;
	wchar_t sFunction[32];
	int     nImageSubsystem;
	int     nImageBits;
	DWORD   nShellFlags; // Флаги ShellExecuteEx
	DWORD   nCreateFlags, nStartFlags; // Флаги CreateProcess
	DWORD   nShowCmd;
	int     nActionLen;
	int     nFileLen;
	int     nParamLen;
	int     nDirLen;
	unsigned __int64 hStdIn, hStdOut, hStdErr;
	// Variable length tail
	wchar_t wsValue[1];
};
struct CESERVER_REQ_ONCREATEPROCESSRET
{
	BOOL  bContinue;
	//BOOL  bUnicode;
	BOOL  bForceBufferHeight;
	BOOL  bAllowDosbox;
	//int   nFileLen;
	//int   nBaseLen;
	//union
	//{
	//	wchar_t wsValue[1];
	//	char sValue[1];
	//};
};

// _ASSERTE(sizeof(CESERVER_REQ_STARTSTOPRET) <= sizeof(CESERVER_REQ_STARTSTOP));
struct CESERVER_REQ_STARTSTOPRET
{
	BOOL     bWasBufferHeight;
	HWND2    hWnd; // при возврате в консоль - GUI (главное окно)
	HWND2    hWndDc;
	HWND2    hWndBack;
	uint32_t dwPID; // при возврате в консоль - PID ConEmu.exe
	uint32_t nBufferHeight, nWidth, nHeight;
	uint32_t dwMainSrvPID;
	uint32_t dwAltSrvPID;
	uint32_t dwPrevAltServerPID;
	BOOL     bNeedLangChange;
	uint64_t NewConsoleLang;
};

// Result of CESERVER_REQ_SRVSTARTSTOP
struct CESERVER_REQ_SRVSTARTSTOPRET
{
	// Main values
	CESERVER_REQ_STARTSTOPRET Info;
	// Используется при CECMD_ATTACH2GUI
	CESERVER_REQ_SETFONT Font;
	// Limited logging of console contents (same output as processed by ConEmu::ConsoleFlags::ProcessAnsi)
	ConEmuAnsiLog AnsiLog;
	// Avoid spare calls, let do all in one place
	ConEmuGuiMapping GuiMapping;
	// Let server know current GUI palette
	CESERVER_CONSOLE_PALETTE Palette;
	// Initialization block of commands:
	// # environment variables may be inherited from parent console,
	// # environment, codepage, aliases and so on may come from ConEmu settings
	// ...
	// "set" \0 "VarName" \0 "VarValue" \0
	// ...
	// "chcp utf-8" \0
	// ...
	// Some more commands may be implemented later, "alias" for example
	STRPTR2 EnvCommands;
	// void SetEnvironmentCommands(LPCWSTR aszzEnvVars, LPCWSTR aszSettingCommands);

	// Store Task and Palette names in EnvVars?
	STRPTR2 TaskName;
	STRPTR2 PaletteName;
};

struct CESERVER_REQ_POSTMSG
{
	BOOL     bPost;
	HWND2    hWnd;
	uint32_t nMsg;
	uint64_t wParam, lParam;
};

enum CEFlashType
{
	eFlashNormal = 0,
	eFlashSimple = 1,
	eFlashBeep   = 2,
};

struct CESERVER_REQ_FLASHWINFO
{
	CEFlashType fType;
	HWND2       hWnd;
	// for (fType == eFlashSimple)
	BOOL        bInvert;
	// this and further for (fType != eFlashSimple)
	uint32_t    dwFlags;
	uint32_t    uCount;
	uint32_t    dwTimeout;
};

// CMD_FARCHANGED - FAR plugin
struct FAR_REQ_FARSETCHANGED
{
	BOOL    bFARuseASCIIsort;
	BOOL    bShellNoZoneCheck; // Затычка для SEE_MASK_NOZONECHECKS
	BOOL    bMonitorConsoleInput; // при (Read/Peek)ConsoleInput(A/W) послать инфу в GUI/Settings/Debug
	BOOL    bLongConsoleOutput; // при выполнении консольных программ из Far - увеличивать высоту буфера
	//wchar_t szEnv[1]; // Variable length: <Name>\0<Value>\0<Name2>\0<Value2>\0\0
	ConEmuComspec ComSpec;
};

struct CESERVER_REQ_SETCONCP
{
	BOOL    bSetOutputCP; // [IN], [Out]=result
	DWORD   nCP;          // [IN], [Out]=LastError
};

// CECMD_SETWINDOWPOS
struct CESERVER_REQ_SETWINDOWPOS
{
	HWND2    hWnd;
	HWND2    hWndInsertAfter;
	int32_t  X;
	int32_t  Y;
	int32_t  cx;
	int32_t  cy;
	uint32_t uFlags;
};

// CECMD_SETWINDOWRGN
struct CESERVER_REQ_SETWINDOWRGN
{
	HWND2 hWnd;
	int   nRectCount;  // если 0 - сбросить WindowRgn, иначе - обсчитать.
	BOOL  bRedraw;
	RECT  rcRects[20]; // [0] - основной окна, [
};

// CECMD_SETBACKGROUND - приходит из плагина "PanelColorer.dll"
// Warning! Structure has variable length. "bmp" field must be followed by bitmap data (same as in *.bmp files)
struct CESERVER_REQ_SETBACKGROUND
{
	int               nType;    // Reserved for future use. Must be 1
	BOOL              bEnabled; // TRUE - ConEmu use this image, FALSE - ConEmu use self background settings

	// какие части консоли раскрашены - enum PaintBackgroundPlaces
	DWORD dwDrawnPlaces;

	//int               nReserved1; // Must by 0. reserved for alpha
	//DWORD             nReserved2; // Must by 0. reserved for replaced colors
	//int               nReserved3; // Must by 0. reserved for background op
	//DWORD nReserved4; // Must by 0. reserved for flags (BmpIsTransparent, RectangleSpecified)
	//DWORD nReserved5; // Must by 0. reserved for level (Some plugins may want to draw small parts over common background)
	//RECT  rcReserved5; // Must by 0. reserved for filled rect (plugin may cover only one panel, or part of it)

	BITMAPFILEHEADER  bmp;
	BITMAPINFOHEADER  bi;
};

// ConEmu respond for CESERVER_REQ_SETBACKGROUND
typedef int SetBackgroundResult;
const SetBackgroundResult
	esbr_OK = 0,               // All OK
	esbr_InvalidArg = 1,       // Invalid *RegisterBackgroundArg
	esbr_PluginForbidden = 2,  // "Allow plugins" unchecked in ConEmu settings ("Background" page)
	esbr_ConEmuInShutdown = 3, // Console is closing. This is not an error, just information
	esbr_Unexpected = 4,       // Unexpected error in ConEmu //-V112
	esbr_InvalidArgSize = 5,   // Invalid RegisterBackgroundArg.cbSize
	esbr_InvalidArgProc = 6,   // Invalid RegisterBackgroundArg.PaintConEmuBackground
	esbr_LastErrorNo = esbr_InvalidArgProc;

struct CESERVER_REQ_SETBACKGROUNDRET
{
	SetBackgroundResult  nResult;
};

// CECMD_ACTIVATECON.
struct CESERVER_REQ_ACTIVATECONSOLE
{
	HWND2 hConWnd;
};

// CECMD_GUIMACRO
#define CEGUIMACRORETENVVAR L"ConEmuMacroResult"
struct CESERVER_REQ_GUIMACRO
{
	DWORD   nSucceeded;
	int     nTabIndex;    // Specially selected tab, 1-based; -1 means last tab; -2 means tab of RCon
	DWORD   nSplitIndex;  // Specially selected split, 1-based
	wchar_t sMacro[1];    // Variable length
};

// CECMD_PEEKREADINFO: посылается в GUI на вкладку Debug
struct CESERVER_REQ_PEEKREADINFO
{
	uint16_t     nCount;
	uint8_t      bMainThread;
	uint8_t      bReserved;
	wchar_t      cPeekRead/*'P'/'R' или 'W' в GUI*/;
	wchar_t      cUnicode/*'A'/'W'*/;
	HANDLE2      h;
	DWORD        nPID;
	DWORD        nTID;
	INPUT_RECORD Buffer[1];
};

enum ATTACHGUIAPP_FLAGS
{
	agaf_Fail     = 0,
	agaf_Success  = 1,
	agaf_DotNet   = 2,
	agaf_NoMenu   = 4,
	agaf_WS_CHILD = 8,
	agaf_Self     = 16, // GUI приложение само создало дочернее окно в ConEmu
	agaf_Inactive = 32, // GUI приложение создается в НЕ активной вкладке! Фокус нужно вернуть в ConEmu!
	agaf_QtWindow = 64, // Qt
};

struct GuiStylesAndShifts
{
	DWORD nStyle, nStyleEx;
	RECT  Shifts;
	WCHAR szExeName[MAX_PATH]; // name only, not path
};

// CECMD_ATTACHGUIAPP
struct CESERVER_REQ_ATTACHGUIAPP
{
	DWORD nFlags;
	DWORD nPID;
	DWORD nServerPID;
	DWORD hkl;
	HWND2 hConEmuWnd;   // Root
	HWND2 hConEmuDc;    // DC Window
	HWND2 hConEmuBack;  // Back - holder for GUI
	HWND2 hAppWindow;   // NULL - проверка можно ли, HWND - когда создан
	HWND2 hSrvConWnd;
	RECT  rcWindow;     // координаты
	DWORD Reserved;     // зарезервировано под флаги, вроде "Показывать заголовок"
	//DWORD nStyle, nStyleEx;
	//BOOL  bHideCaption; //
	struct GuiStylesAndShifts Styles;
	wchar_t sAppFilePathName[MAX_PATH*2];
};

// CECMD_PORTABLESTART
struct CESERVER_REQ_PORTABLESTARTED
{
	DWORD   nSubsystem;
	DWORD   nImageBits;
	HANDLE2 hProcess;
	DWORD   nPID;
	wchar_t sAppFilePathName[MAX_PATH*2];
};

// CECMD_SETFOCUS
struct CESERVER_REQ_SETFOCUS
{
	BOOL  bSetForeground;
	HWND2 hWindow;
};

// CECMD_SETPARENT
struct CESERVER_REQ_SETPARENT
{
	HWND2 hWnd;
	HWND2 hParent;
};

struct MyAssertInfo
{
	int nBtns;
	BOOL bNoPipe;
	wchar_t szTitle[255];
	wchar_t szDebugInfo[4096];
};

// CECMD_STARTSERVER
struct CESERVER_REQ_START
{
	DWORD nGuiPID; // In-ConEmuGuiPID
	HWND2 hGuiWnd; // In-ghWnd
	HWND2 hAppWnd; // Hooked application window (для GUI режима)
	DWORD nValue;  // Error codes
	BOOL  bLeave;  // Leave tab opened after process exit
};

// CECMD_LOCKDC
struct CESERVER_REQ_LOCKDC
{
	HWND2 hDcWnd;
	BOOL  bLock;
	RECT  Rect; // Пока просто
};

// CECMD_REGEXTCONSOLE
struct CESERVER_REQ_REGEXTCON
{
	HANDLE2 hCommitEvent;
};

struct CESERVER_REQ_FULLSCREEN
{
	BOOL  bSucceeded;
	DWORD nErrCode;
	COORD crNewSize;
};

struct CESERVER_REQ_SETCONSOLORS
{
	BOOL  ChangeTextAttr;
	BOOL  ChangePopupAttr;
	WORD  NewTextAttributes;
	WORD  NewPopupAttributes;
	BOOL  ReFillConsole;
};

struct CESERVER_REQ_DUPLICATE
{
	HWND2   hGuiWnd;
	DWORD   nGuiPID;
	DWORD   nAID; // Internal ConEmu ID
	BOOL    bRunAs;
	DWORD   nWidth, nHeight;
	DWORD   nBufferHeight;
	DWORD   nColors;
	STRPTR2 sDirectory;
	STRPTR2 sCommand;
};

enum ALTBUFFER_FLAGS
{
	abf_SaveContents    = 0x0001,
	abf_RestoreContents = 0x0002,
	abf_BufferOn        = 0x0004,
	abf_BufferOff       = 0x0008,
	abf_Connector       = 0x0010,
};

struct CESERVER_REQ_ALTBUFFER
{
	DWORD  AbFlags; // ALTBUFFER_FLAGS
	USHORT BufferHeight; // In/Out
};

// CECMD_SETCONSCRBUF
struct CESERVER_REQ_SETCONSCRBUF
{
	HANDLE2 hRequestor; // HANDLE of requesting thread
	HANDLE2 hTemp;      // Used internally, set to NULL when bLock and don't change
	BOOL    bLock;      // Lock or release
	COORD   dwSize;     // Informational, new size of the buffer
};

// CECMD_STORECURDIR
struct CESERVER_REQ_STORECURDIR
{
	int iActiveCch;     // Including terminating zero
	int iPassiveCch;    // Passive panel, Far only. Including terminating zero
	wchar_t szDir[1];   // Variable length;
};

// CECMD_GETALLPANELS
struct CESERVER_REQ_GETALLPANELS
{
	int iCount;         // Count of 0-term strings
	int iCurrent;       // Index of the current panel
	wchar_t szDirs[1]; // Variable length;
};

struct CESERVER_PROMPTSTARTED
{
	// If new fields will be required - add them before szExeName
	wchar_t szExeName[1]; // because of variable length
};

struct CESERVER_PALETTE
{
	COLORREF crPalette[16];
	COLORREF crFadePalette[16];
};

struct CESERVER_ROOT_INFO
{
	BOOL  bRunning;
	DWORD nPID;
	DWORD nExitCode;
	DWORD nUpTime; // millisec
};

// CECMD_GETTASKCMD
struct CESERVER_REQ_TASK
{
	BOOL    found;
	wchar_t data[1]; // Variable length
};

struct CESERVER_REQ
{
	CESERVER_REQ_HDR hdr;
	union
	{
		uint8_t  Data[1]; // variable(!) length
		uint16_t wData[1];
		uint32_t dwData[1];
		uint64_t qwData[1];
		ConEmuGuiMapping GuiInfo;
		CESERVER_REQ_CONINFO ReqConInfo;
		CESERVER_CONSOLE_MAPPING_HDR ConInfo;
		CESERVER_REQ_CONSOLE_STATE ConState;
		CESERVER_REQ_SETSIZE SetSize;
		CESERVER_REQ_RETSIZE SetSizeRet;
		CESERVER_REQ_OUTPUTFILE OutputFile;
		CESERVER_REQ_NEWCMD NewCmd;
		CESERVER_REQ_SRVSTARTSTOP SrvStartStop;
		CESERVER_REQ_SRVSTARTSTOPRET SrvStartStopRet;
		CESERVER_REQ_STARTSTOP StartStop;
		CESERVER_REQ_STARTSTOPRET StartStopRet;
		CESERVER_REQ_CONEMUTAB Tabs;
		CESERVER_REQ_CONEMUTAB_RET TabsRet;
		CESERVER_REQ_POSTMSG Msg;
		CESERVER_REQ_FLASHWINFO Flash;
		FAR_REQ_FARSETCHANGED FarSetChanged;
		CESERVER_REQ_SETCONCP SetConCP;
		CESERVER_REQ_SETWINDOWPOS SetWndPos;
		CESERVER_REQ_SETWINDOWRGN SetWndRgn;
		CESERVER_REQ_SETBACKGROUND Background;
		CESERVER_REQ_SETBACKGROUNDRET BackgroundRet;
		CESERVER_REQ_ACTIVATECONSOLE ActivateCon;
		PanelViewInit PVI;
		CESERVER_REQ_SETFONT Font;
		CESERVER_REQ_GUIMACRO GuiMacro;
		CESERVER_REQ_ONCREATEPROCESS OnCreateProc;
		CESERVER_REQ_ONCREATEPROCESSRET OnCreateProcRet;
		CESERVER_REQ_PEEKREADINFO PeekReadInfo;
		MyAssertInfo AssertInfo;
		CESERVER_REQ_ATTACHGUIAPP AttachGuiApp;
		CESERVER_REQ_PORTABLESTARTED PortableStarted;
		GuiStylesAndShifts GuiAppShifts;
		CESERVER_REQ_SETFOCUS setFocus;
		CESERVER_REQ_SETPARENT setParent;
		CESERVER_REQ_SETGUIEXTERN SetGuiExtern;
		CESERVER_REQ_START NewServer;
		CESERVER_REQ_LOCKDC LockDc;
		CESERVER_REQ_REGEXTCON RegExtCon;
		CESERVER_REQ_GETALLTABS GetAllTabs;
		CESERVER_REQ_FULLSCREEN FullScreenRet;
		CESERVER_REQ_SETCONSOLORS SetConColor;
		CESERVER_REQ_DUPLICATE Duplicate;
		CESERVER_REQ_ALTBUFFER AltBuf;
		CESERVER_REQ_SETCONSCRBUF SetConScrBuf;
		CESERVER_REQ_STORECURDIR CurDir;
		CESERVER_REQ_GETALLPANELS Panels;
		CESERVER_PROMPTSTARTED PromptStarted;
		CESERVER_PALETTE Palette;
		CESERVER_ROOT_INFO RootInfo;
		CESERVER_REQ_TASK GetTask;
	};

	DWORD DataSize() { return (this && (hdr.cbSize >= sizeof(hdr))) ? (hdr.cbSize - sizeof(hdr)) : 0; };
};



//#pragma pack(pop)
#include <poppack.h>  // NOLINT(clang-diagnostic-pragma-pack)



//#define GWL_TABINDEX     0
//#define GWL_LANGCHANGE   4

#ifdef _DEBUG
#define CONEMUALIVETIMEOUT INFINITE
#define CONEMUREADYTIMEOUT INFINITE
#define CONEMUFARTIMEOUT   120000 // Сколько ожидать, пока ФАР среагирует на вызов плагина
#else
#define CONEMUALIVETIMEOUT 1000  // Живость плагина ждем секунду
#define CONEMUREADYTIMEOUT 10000 // А на выполнение команды - 10s max
#define CONEMUFARTIMEOUT   10000 // Сколько ожидать, пока ФАР среагирует на вызов плагина
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif



/*enum PipeCmd
{
    SetTabs=0,
    DragFrom,
    DragTo
};*/

// ConEmu.dll экспортирует следующие функции
//HWND WINAPI GetFarHWND();
//void WINAPI _export GetFarVersion ( FarVersion* pfv );

//#if defined(__GNUC__)
////typedef DWORD   HWINEVENTHOOK;
//#define WINEVENT_OUTOFCONTEXT   0x0000  // Events are ASYNC
//// User32.dll
//typedef HWINEVENTHOOK (WINAPI* FSetWinEventHook)(DWORD eventMin, DWORD eventMax, HMODULE hmodWinEventProc, WINEVENTPROC pfnWinEventProc, DWORD idProcess, DWORD idThread, DWORD dwFlags);
//typedef BOOL (WINAPI* FUnhookWinEvent)(HWINEVENTHOOK hWinEventHook);
//#endif


#ifdef __cplusplus


#define CERR_CMDLINEEMPTY 200
#define CERR_CMDLINE      201

#define MOUSE_EVENT_MOVE      (WM_APP+10)
#define MOUSE_EVENT_CLICK     (WM_APP+11)
#define MOUSE_EVENT_DBLCLICK  (WM_APP+12)
#define MOUSE_EVENT_WHEELED   (WM_APP+13)
#define MOUSE_EVENT_HWHEELED  (WM_APP+14)
#define MOUSE_EVENT_FIRST MOUSE_EVENT_MOVE
#define MOUSE_EVENT_LAST MOUSE_EVENT_HWHEELED

//#define INPUT_THREAD_ALIVE_MSG (WM_APP+100)

//#define MAX_INPUT_QUEUE_EMPTY_WAIT 1000

BOOL PackInputRecord(const INPUT_RECORD* piRec, MSG64::MsgStr* pMsg);
BOOL UnpackInputRecord(const MSG64::MsgStr* piMsg, INPUT_RECORD* pRec);
void TranslateKeyPress(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode, INPUT_RECORD* rDown, INPUT_RECORD* rUp);
void CommonShutdown();

bool IsEnvBlockVariableValid(LPCWSTR asName, LPCWSTR& rszEqnSign);
bool IsEnvBlockVariableValid(LPCWSTR asName);

struct RequestLocalServerParm;
struct AnnotationHeader;
struct AnnotationInfo;
typedef int (WINAPI* RequestLocalServer_t)(/*[IN/OUT]*/RequestLocalServerParm* Parm);
typedef BOOL (WINAPI* ExtendedConsoleWriteText_t)(HANDLE hConsoleOutput, const AnnotationInfo* Attributes, const wchar_t* Buffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
typedef void (WINAPI* ExtendedConsoleCommit_t)();
typedef int (WINAPI* SrvLogString_t)(const wchar_t* str);
typedef DWORD RequestLocalServerFlags;
const RequestLocalServerFlags
	slsf_SetOutHandle      = 1,
	slsf_RequestTrueColor  = 2,
	slsf_PrevAltServerPID  = 4,
	slsf_AltServerStopped  = 8,
	slsf_GetFarCommitEvent = 16,
	slsf_FarCommitForce    = 32,
	slsf_GetCursorEvent    = 64,
	slsf_ReinitWindows     = 128,
	slsf_OnFreeConsole     = 256,
	slsf_OnAllocConsole    = 512,
	slsf__EventsOnly       = (slsf_GetFarCommitEvent|slsf_FarCommitForce|slsf_GetCursorEvent|slsf_OnFreeConsole|slsf_OnAllocConsole),
	slsf_None              = 0;
struct RequestLocalServerParm
{
	DWORD     StructSize;

	RequestLocalServerFlags Flags;

	/*[IN]*/  HANDLE* ppConOutBuffer;  // if Parm->ppConOutBuffer is set, it HAVE TO BE GLOBAL SCOPE variable!
	/*[OUT]*/ AnnotationHeader* pAnnotation;
	/*[OUT]*/ RequestLocalServer_t fRequestLocalServer;
	/*[OUT]*/ ExtendedConsoleWriteText_t fExtendedConsoleWriteText;
	/*[OUT]*/ ExtendedConsoleCommit_t fExtendedConsoleCommit;
	/*[OUT]*/ DWORD_PTR nPrevAltServerPID; // alignment
	/*[OUT]*/ HANDLE hFarCommitEvent;
	/*[OUT]*/ HANDLE hCursorChangeEvent;
	/*[OUT]*/ SrvLogString_t fSrvLogString;
};


#include "MAssert.h"
#include "MSecurity.h"

template <typename T> bool _bool(const T V) { _ASSERTE((V)==FALSE || (V)==TRUE); return ((V) != 0); };

#endif // __cplusplus

// The size of the PROCESS_ALL_ACCESS flag increased on Windows Server 2008 and Windows Vista.
// If an application compiled for Windows Server 2008 and Windows Vista is run on Windows Server 2003
// or Windows XP/2000, the PROCESS_ALL_ACCESS flag is too large and the function specifying this flag
// fails with ERROR_ACCESS_DENIED. To avoid this problem, specify the minimum set of access rights required
// for the operation. If PROCESS_ALL_ACCESS must be used, set _WIN32_WINNT to the minimum operating system
// targeted by your application (for example, #define _WIN32_WINNT _WIN32_WINNT_WINXP).
#undef PROCESS_ALL_ACCESS
#define PROCESS_ALL_ACCESS "PROCESS_ALL_ACCESS"
#define MY_PROCESS_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xFFF)

// This must be the end line
#endif // COMMON_HEADER_HPP_
