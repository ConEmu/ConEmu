
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

#define HIDE_USE_EXCEPTION_INFO

#ifdef _DEBUG
//  #define SHOW_GUIATTACH_START
	#define TEST_JP_LANS
#endif


#define SHOWDEBUGSTR

#define CHILD_DESK_MODE
#define REGPREPARE_EXTERNAL
//#define CATCH_TOPMOST_SET

#include "Header.h"
#include "AboutDlg.h"

#include "../common/shlobj.h"
#include "ShObjIdl_Part.h"

#include "../common/execute.h"
#include "../common/EnvVar.h"
#include "../common/md5.h"
#include "../common/MArray.h"
#include "../common/MFileLogEx.h"
#include "../common/Monitors.h"
#include "../common/MProcess.h"
#include "../common/MSetter.h"
#include "../common/MToolTip.h"
#include "../common/MWow64Disable.h"
#include "../common/ProcessSetEnv.h"
#include "../common/StartupEnvDef.h"
#include "../common/WFiles.h"
#include "../common/WUser.h"
#include "AltNumpad.h"
#include "Attach.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "ConEmuPipe.h"
#include "DefaultTerm.h"
#include "DontEnable.h"
#include "DpiAware.h"
#include "DragDrop.h"
#include "FindDlg.h"
#include "GdiObjects.h"
#include "GestureEngine.h"
#include "GlobalHotkeys.h"
#include "Inside.h"
#include "LoadImg.h"
#include "Macro.h"
#include "Menu.h"
#include "Options.h"
#include "OptionsClass.h"
#include "PushInfo.h"
#include "Recreate.h"
#include "RunQueue.h"
#include "SetCmdTask.h"
#include "SetPgDebug.h"
#include "SetPgInfo.h"
#include "Status.h"
#include "SystemEnvironment.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "Update.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "version.h"
#include "VirtualConsole.h"


#define DEBUGSTRSYS(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE(s) DEBUGSTR(s)
#define DEBUGSTRSTYLE(s) DEBUGSTR(s)
#define DEBUGSTRCONS(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)// ; Sleep(2000)
#define DEBUGSTRMOUSE(s) //DEBUGSTR(s)
#define DEBUGSTRMOUSEWHEEL(s) //DEBUGSTR(s)
#define DEBUGSTRRCLICK(s) //DEBUGSTR(s)
#define DEBUGSTRKEY(s) //DEBUGSTR(s)
#define DEBUGSTRIME(s) //DEBUGSTR(s)
#define DEBUGSTRCHAR(s) //DEBUGSTR(s)
#define DEBUGSTRSETCURSOR_(s) //DEBUGSTR(s)
#define DEBUGSTRSETCURSOR(s) //OutputDebugString(s)
#define DEBUGSTRCONEVENT(s) //DEBUGSTR(s)
#define DEBUGSTRMACRO(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL(s) //DEBUGSTR(s)
#define DEBUGSTRPANEL2(s) //DEBUGSTR(s)
#define DEBUGSTRFOREGROUND(s) //DEBUGSTR(s)
#define DEBUGSTRLLKB(s) //DEBUGSTR(s)
#define DEBUGSTRTIMER(s) //DEBUGSTR(s)
#define DEBUGSTRMSG(h,m,w,l) //DebugLogMessage(h, m, w, l, -1, FALSE)
#define DEBUGSTRMSG2(s) //DEBUGSTR(s)
#define DEBUGSTRANIMATE(s) //DEBUGSTR(s)
#define DEBUGSTRFOCUS(s) //DEBUGSTR(s)
#define DEBUGSTRSESS(s) DEBUGSTR(s)
#define DEBUGSTRDPI(s) DEBUGSTR(s)
#define DEBUGSTRNOLOG(s) //DEBUGSTR(s)
#define DEBUGSTRDESTROY(s) DEBUGSTR(s)
#define DEBUGSTRSETCHANGE(s) DEBUGSTR(s)
#ifdef _DEBUG
//#define DEBUGSHOWFOCUS(s) DEBUGSTR(s)
#endif

//#define PROCESS_WAIT_START_TIME 1000

#define DRAG_DELTA 5
#define SPLITOVERCHECK_DELTA 5 // gpSet->isActivateSplitMouseOver()

#define PTDIFFTEST(C,D) PtDiffTest(C, ptCur.x, ptCur.y, D)
//(((abs(C.x-(short)LOWORD(lParam)))<D) && ((abs(C.y-(short)HIWORD(lParam)))<D))

#define _ASSERTEL(x) if (!(x)) { LogString(#x); _ASSERTE(x); }


#if defined(__GNUC__)
#define EXT_GNUC_LOG
#endif


#define RCLICKAPPSTIMEOUT 600
#define RCLICKAPPS_START 200 // начало отрисовки кружка вокруг курсора
#define RCLICKAPPSTIMEOUT_MAX 10000
#define RCLICKAPPSDELTA 3

#define TOUCH_DBLCLICK_DELTA 1000 // 1sec

//#define CONEMU_ANIMATE_DURATION 200
//#define CONEMU_ANIMATE_DURATION_MAX 5000

const wchar_t* gsHomePage    = CEHOMEPAGE;    //L"https://conemu.github.io";
const wchar_t* gsDownlPage   = CEDOWNLPAGE;   //L"https://conemu.github.io/en/Downloads.html";
const wchar_t* gsFirstStart  = CEFIRSTSTART;  //L"https://conemu.github.io/en/SettingsFast.html";
const wchar_t* gsReportBug   = CEREPORTBUG;   //L"https://conemu.github.io/en/Issues.html";
const wchar_t* gsReportCrash = CEREPORTCRASH; //L"https://conemu.github.io/en/Issues.html";
const wchar_t* gsWhatsNew    = CEWHATSNEW;    //L"https://conemu.github.io/en/Whats_New.html";

#define gsPortableApps_LauncherIni  L"\\..\\AppInfo\\Launcher\\ConEmuPortable.ini"
#define gsPortableApps_DefaultXml   L"\\..\\DefaultData\\settings\\ConEmu.xml"
#define gsPortableApps_ConEmuXmlDir L"\\..\\..\\Data\\settings"

namespace ConEmuMsgLogger
{
	MSG g_msg[BUFFER_SIZE];
	LONG g_msgidx = -1;

	Event g_pos[BUFFER_POS_SIZE];
	LONG g_posidx = -1;

	void LogPos(const MSG& msg, Source from)
	{
		#ifdef _DEBUG
		if (from != msgCanvas)
			return;
		#endif

		// Get next message index
		LONG i = _InterlockedIncrement(&g_posidx);
		// Write a message at this index
		Event& e = g_pos[i & (BUFFER_POS_SIZE - 1)]; // Wrap to buffer size

		e.hwnd = msg.hwnd;
		e.time = GetTickCount(); // msg.time == 0 скорее всего

		switch (msg.message)
		{
		case WM_WINDOWPOSCHANGING:
			e.msg = Event::WindowPosChanging;
			break;
		case WM_WINDOWPOSCHANGED:
			e.msg = Event::WindowPosChanged;
			break;
		case WM_MOVE:
			e.msg = Event::Move;
			break;
		case WM_SIZE:
			e.msg = Event::Size;
			break;
		case WM_MOVING:
			e.msg = Event::Moving;
			break;
		case WM_SIZING:
			e.msg = Event::Sizing;
			break;
		}

		switch (msg.message)
		{
		case WM_WINDOWPOSCHANGING:
		case WM_WINDOWPOSCHANGED:
			if (((WINDOWPOS*)msg.lParam)->flags & SWP_NOMOVE)
			{
				e.x = e.y = 0;
			}
			else
			{
				e.x = ((WINDOWPOS*)msg.lParam)->x;
				e.y = ((WINDOWPOS*)msg.lParam)->y;
			}
			if (((WINDOWPOS*)msg.lParam)->flags & SWP_NOSIZE)
			{
				e.w = e.h = 0;
			}
			else
			{
				e.w = ((WINDOWPOS*)msg.lParam)->cx;
				e.h = ((WINDOWPOS*)msg.lParam)->cy;
			}
			break;
		case WM_MOVE:
			e.x = (int)(short)LOWORD(msg.lParam);
			e.y = (int)(short)HIWORD(msg.lParam);
			e.w = e.h = 0;
			break;
		case WM_SIZE:
			e.x = e.y = 0;
			e.w = LOWORD(msg.lParam);
			e.h = HIWORD(msg.lParam);
			break;
		case WM_MOVING:
		case WM_SIZING:
			e.msg = Event::Sizing;
			// minimize memory use for lock-free logging
			e.x = LOSHORT(((LPRECT)msg.lParam)->left);
			e.y = LOSHORT(((LPRECT)msg.lParam)->top);
			e.w = LOSHORT(((LPRECT)msg.lParam)->right - e.x);
			e.h = LOSHORT(((LPRECT)msg.lParam)->bottom - e.y);
			break;
		}
	} // void LogPos(const MSG& msg)
}

const wchar_t gsFocusShellActivated[] = L"HSHELL_WINDOWACTIVATED";
const wchar_t gsFocusCheckTimer[] = L"TIMER_MAIN_ID";
const wchar_t gsFocusQuakeCheckTimer[] = L"TIMER_QUAKE_AUTOHIDE_ID";

#pragma warning(disable: 4355)
CConEmuMain::CConEmuMain()
	: CTaskBar(this)
	, CConEmuStart(this)
{
	#pragma warning(default: 4355)
	gpConEmu = this; // immediately! only one CConEmuMain instance per process is allowed now

	CVConGroup::Initialize();

	getForegroundWindow();

	// ConEmu main window was not created yet, so...

	_ASSERTE(gOSVer.dwMajorVersion>=5);

	wchar_t szVer4[8] = L""; lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	// Same as ConsoleMain.cpp::SetWorkEnvVar()
	swprintf_c(ms_ConEmuBuild, L"%02u%02u%02u%s%s",
		(MVV_1 % 100), MVV_2, MVV_3, szVer4[0] && szVer4[1] ? L"-" : L"", szVer4);
	// And title
	swprintf_c(ms_ConEmuDefTitle, L"ConEmu %s [%i%s]",
		ms_ConEmuBuild, WIN3264TEST(32,64), RELEASEDEBUGTEST(L"",L"D"));

	// Dynamic messages
	RegisterMessages();

	// Classes
	m_Hotkeys = std::make_shared<GlobalHotkeys>();
	mp_Menu = new CConEmuMenu;
	mp_Status = new CStatus(this);
	mp_TabBar = new CTabBarClass;
	mp_DefTrm = new CDefaultTerminal;
	mp_Find = new CEFindDlg;
	mp_RunQueue = new CRunQueue;
	mp_AltNumpad = new CAltNumpad(this);
	mp_HandleMonitor = new HandleMonitor();

	mn_MainThreadId = GetCurrentThreadId();

	bool bNeedConHostSearch = (gnOsVer == 0x0601);
	if (bNeedConHostSearch)
	{
		m_LockConhostStart.pcsLock = new MSectionLockSimple();
		m_LockConhostStart.pcs = new MSectionSimple(true);
	}

	m_SshAgentsLock = new MSectionSimple(true);

	mh_CursorWait = LoadCursor(nullptr, IDC_WAIT);
	mh_CursorArrow = LoadCursor(nullptr, IDC_ARROW);
	mh_CursorMove = LoadCursor(nullptr, IDC_SIZEALL);
	mh_CursorIBeam = LoadCursor(nullptr, IDC_IBEAM);
	mh_CursorAppStarting = LoadCursor(nullptr, IDC_APPSTARTING);
	// g_hInstance is not initialized yet?
	mh_SplitV = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_SPLITV));
	mh_SplitV2 = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_SPLITV2));
	mh_SplitH = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_SPLITH));
	GetCursorPos(&mouse.ptLastSplitOverCheck);
	m_ActiveKeybLayout = GetActiveKeyboardLayout();

	#ifdef __GNUC__
	HMODULE hGdi32 = GetModuleHandle(L"gdi32.dll");
	GdiAlphaBlend = (AlphaBlend_t)(hGdi32 ? GetProcAddress(hGdi32, "GdiAlphaBlend") : nullptr);
	#endif

	HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	// GetLayeredWindowAttributes появился только в XP
	_GetLayeredWindowAttributes = (GetLayeredWindowAttributes_t)(hUser32 ? GetProcAddress(hUser32, "GetLayeredWindowAttributes") : nullptr);
	#ifdef __GNUC__
	// SetLayeredWindowAttributes есть в Win2k, только про него не знает GCC
	SetLayeredWindowAttributes = (SetLayeredWindowAttributes_t)(hUser32 ? GetProcAddress(hUser32, "SetLayeredWindowAttributes") : nullptr);
	#endif

	// IME support (WinXP or later)
	if (gnOsVer >= 0x501)
	{
		mh_Imm32 = LoadLibrary(L"Imm32.dll");
		if (mh_Imm32)
		{
			_ImmSetCompositionFont = (ImmSetCompositionFontW_t)GetProcAddress(mh_Imm32, "ImmSetCompositionFontW");
			_ImmSetCompositionWindow = (ImmSetCompositionWindow_t)GetProcAddress(mh_Imm32, "ImmSetCompositionWindow");
			_ImmGetContext = (ImmGetContext_t)GetProcAddress(mh_Imm32, "ImmGetContext");
			if (!_ImmSetCompositionFont || !_ImmSetCompositionWindow || !_ImmGetContext)
			{
				_ASSERTE(_ImmSetCompositionFont && _ImmSetCompositionWindow && _ImmGetContext);
				_ImmSetCompositionFont = nullptr;
				_ImmSetCompositionWindow = nullptr;
				_ImmGetContext = nullptr;
				FreeLibrary(mh_Imm32);
				mh_Imm32 = nullptr;
			}
		}
	}

	saved_environment_ = std::make_shared<SystemEnvironment>();
	saved_environment_->LoadFromRegistry();

	// Load current comspec form registry
	HKEY hk;
	if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment", 0, KEY_READ, &hk))
	{
		DWORD nSize = sizeof(ms_ComSpecInitial)-sizeof(wchar_t);
		if (RegQueryValueEx(hk, L"ComSpec", nullptr, nullptr, (LPBYTE)ms_ComSpecInitial, &nSize))
			ms_ComSpecInitial[0] = 0;
		RegCloseKey(hk);
	}
	if (!*ms_ComSpecInitial)
	{
		GetComspecFromEnvVar(ms_ComSpecInitial, countof(ms_ComSpecInitial));
	}
	_ASSERTE(*ms_ComSpecInitial);

	wchar_t *pszSlash = nullptr;
	CEStr szExePath, szCompact;
	if (!GetModuleFileName(nullptr, szExePath.GetBuffer(MAX_PATH), MAX_PATH) || !(pszSlash = wcsrchr(szExePath.ms_Val, L'\\')))
	{
		DisplayLastError(L"GetModuleFileName failed");
		TerminateProcess(GetCurrentProcess(), 100);
		return;
	}
	szCompact = GetFullPathNameEx(szExePath);
	if (FileExists(szCompact))
		wcscpy_c(ms_ConEmuExe, szCompact);
	else
		wcscpy_c(ms_ConEmuExe, szExePath);

	// Папка программы
	wcscpy_c(ms_ConEmuExeDir, ms_ConEmuExe);
	pszSlash = wcsrchr(ms_ConEmuExeDir, L'\\');
	if (pszSlash) *pszSlash = 0;

	// Store current (at the startup moment) working directory
	StoreWorkDir();

	bool lbBaseFound = false;

	wchar_t szFindFile[MAX_PATH+64];

	// Mingw/msys installation?
	CEStr lsBash;
	if (FindBashLocation(lsBash))
	{
		m_InstallMode |= cm_MSysStartup;
	}

	// Сначала проверяем подпапку
	wcscpy_c(ms_ConEmuBaseDir, ms_ConEmuExeDir);
	wcscat_c(ms_ConEmuBaseDir, L"\\ConEmu");
	lbBaseFound = CheckBaseDir();


	if (!lbBaseFound)
	{
		// Если не нашли - то в той же папке, что и GUI
		wcscpy_c(ms_ConEmuBaseDir, ms_ConEmuExeDir);
		lbBaseFound = CheckBaseDir();
	}

	if (!lbBaseFound)
	{
		// Если все-равно не нашли - проверить, может это mingw?
		pszSlash = wcsrchr(ms_ConEmuExeDir, L'\\');
		if (pszSlash && (lstrcmpi(pszSlash, L"\\bin") == 0))
		{
			// -> $MINGW_ROOT/libexec/ConEmu
			size_t cch = pszSlash - ms_ConEmuExeDir;
			wmemmove(ms_ConEmuBaseDir, ms_ConEmuExeDir, cch);
			ms_ConEmuBaseDir[cch] = 0;
			wcscat_c(ms_ConEmuBaseDir, L"\\libexec\\ConEmu");
			// Last chance
			lbBaseFound = CheckBaseDir();
			if (lbBaseFound)
			{
				m_InstallMode |= cm_MinGW;
			}
			else
			{
				// Fail, revert to ExeDir
				wcscpy_c(ms_ConEmuBaseDir, ms_ConEmuExeDir);
			}
		}
	}

	// Git-For-Windows package?
	if (isMSysStartup() /*&& !isMingwMode()*/)
	{
		// ConEmu.exe and other binaries may be located in /opt/bin
		lsBash = JoinPath(ms_ConEmuExeDir, L"..\\..\\git-bash.exe");
		if (FileExists(lsBash))
		{
			m_InstallMode |= (cm_MinGW|cm_GitForWin);
		}
	}

	if (isMingwMode() && isMSysStartup())
	{
		// This is example. Will be replaced with full path.
		SetDefaultCmd(nullptr /*L"bash.exe --login -i"*/);
	}

	if (!isMingwMode())
	{
		// PortableApps.com?
		LPCWSTR pszCheck[] = {
			gsPortableApps_LauncherIni /* L"\\..\\AppInfo\\Launcher\\ConEmuPortable.ini" */,
			gsPortableApps_DefaultXml /* L"\\..\\DefaultData\\settings\\ConEmu.xml"*/};
		for (size_t c = 0; c < countof(pszCheck); c++)
		{
			wcscpy_c(szFindFile, ms_ConEmuExeDir);
			wcscat_c(szFindFile, pszCheck[c]);
			if (!FileExists(szFindFile))
				goto NonPortable;
		}

		// Ищем где должен лежать файл настроек в структуре PortableApps
		wcscpy_c(ms_ConEmuXml, ms_ConEmuExeDir);
		wchar_t* pszCut = wcsrchr(ms_ConEmuXml, L'\\');
		if (pszCut)
		{
			*pszCut = 0;
			pszCut = wcsrchr(ms_ConEmuXml, L'\\');
			// gsPortableApps_ConEmuXml   L"\\..\\..\\Data\\settings"
			if (pszCut)
			{
				LPCWSTR pszAppendPath = gsPortableApps_ConEmuXmlDir;
				_ASSERTE(wcsncmp(pszAppendPath, L"\\..\\..\\D", 8)==0);
				*pszCut = 0;
				wcscat_c(ms_ConEmuXml, pszAppendPath+6);
				if (!DirectoryExists(ms_ConEmuXml))
					MyCreateDirectory(ms_ConEmuXml);
				wcscat_c(ms_ConEmuXml, L"\\ConEmu.xml");
				if (!FileExists(ms_ConEmuXml))
				{
					// Нужно скопировать дефолтный файл настроек?
					CopyFile(szFindFile, ms_ConEmuXml, FALSE);
				}
				// Turn it on
				m_InstallMode |= cm_PortableApps;
			}

			if (!(m_InstallMode & cm_PortableApps))
			{
				// Fail
				ms_ConEmuXml[0] = 0;
			}
		}


	}
	NonPortable:

	m_DbgInfo = {};
	if (IsDebuggerPresent())
	{
		wchar_t szDebuggers[32];
		if (GetEnvironmentVariable(ENV_CONEMU_BLOCKCHILDDEBUGGERS_W, szDebuggers, countof(szDebuggers)))
		{
			m_DbgInfo.bBlockChildrenDebuggers = (lstrcmp(szDebuggers, ENV_CONEMU_BLOCKCHILDDEBUGGERS_YES) == 0);
		}
	}
	// И сразу сбросить ее, чтобы не было мусора
	SetEnvironmentVariable(ENV_CONEMU_BLOCKCHILDDEBUGGERS_W, nullptr);


	// Чтобы знать, может мы уже запущены под UAC админом?
	mb_IsUacAdmin = IsUserAdmin();


	// Добавить в окружение переменную с папкой к ConEmu.exe
	SetEnvironmentVariable(ENV_CONEMUDIR_VAR_W, ms_ConEmuExeDir);
	SetEnvironmentVariable(ENV_CONEMUBASEDIR_VAR_W, ms_ConEmuBaseDir);
	CEStr BaseShort(GetShortFileNameEx(ms_ConEmuBaseDir, false));
	SetEnvironmentVariable(ENV_CONEMUBASEDIRSHORT_VAR_W, BaseShort.IsEmpty() ? ms_ConEmuBaseDir : BaseShort.ms_Val);
	SetEnvironmentVariable(ENV_CONEMU_BUILD_W, ms_ConEmuBuild);
	SetEnvironmentVariable(ENV_CONEMU_CONFIG_W, L"");
	SetEnvironmentVariable(ENV_CONEMUCFGDIR_VAR_W, L"");
	SetEnvironmentVariable(ENV_CONEMU_EXEARGS_W, L"");
	SetEnvironmentVariable(ENV_CONEMU_EXEARGS2_W, L"");
	SetEnvironmentVariable(ENV_CONEMU_ISADMIN_W, mb_IsUacAdmin ? L"ADMIN" : nullptr);

	// Just reset it here. Variables would be set to real values
	// later in CConEmuStart::ProcessConEmuArgsVar
	SetEnvironmentVariableW(L"ConEmuArgs", L"");   // configuration switches
	SetEnvironmentVariableW(L"ConEmuArgs2", L"");  // the tail with `-cmd ...` or `-cmdlist ...`

	wchar_t szDrive[MAX_PATH];
	SetEnvironmentVariable(ENV_CONEMUDRIVE_VAR_W, GetDrive(ms_ConEmuExeDir, szDrive, countof(szDrive)));


	// Ищем файл портабельных настроек. Сначала пробуем в BaseDir
	ConEmuXml();
	ConEmuIni();

	// Help-файл. Сначала попробуем в BaseDir
	wcscpy_c(ms_ConEmuChm, ms_ConEmuBaseDir); lstrcat(ms_ConEmuChm, L"\\ConEmu.chm");

	if (!FileExists(ms_ConEmuChm))
	{
		if (lstrcmp(ms_ConEmuBaseDir, ms_ConEmuExeDir))
		{
			wcscpy_c(ms_ConEmuChm, ms_ConEmuExeDir); lstrcat(ms_ConEmuChm, L"\\ConEmu.chm");

			if (!FileExists(ms_ConEmuChm))
				ms_ConEmuChm[0] = 0;
		}
		else
		{
			ms_ConEmuChm[0] = 0;
		}
	}

	wcscpy_c(ms_ConEmuC32Full, ms_ConEmuBaseDir);
	wcscat_c(ms_ConEmuC32Full, L"\\ConEmuC.exe");
	if (IsWindows64())
	{
		wcscpy_c(ms_ConEmuC64Full, ms_ConEmuBaseDir);
		wcscat_c(ms_ConEmuC64Full, L"\\ConEmuC64.exe");
		// Проверяем наличие
		if (!FileExists(ms_ConEmuC64Full))
			wcscpy_c(ms_ConEmuC64Full, ms_ConEmuC32Full);
		else if (!FileExists(ms_ConEmuC32Full))
			wcscpy_c(ms_ConEmuC32Full, ms_ConEmuC64Full);
	}
	else
	{
		wcscpy_c(ms_ConEmuC64Full, ms_ConEmuC32Full);
	}


	// DosBox (the only available way to run legacy Dos-applications in 64-bit OS)
	CheckDosBoxExists();

	CFrameHolder::InitFrameHolder();
}

void CConEmuMain::RegisterMessages()
{
	// Dynamic messages
	mn__FirstAppMsg = WM_APP+10;
	m__AppMsgs.Init(128, true);
	// Go
	mn_MsgPostCreate = RegisterMessage("PostCreate");
	mn_MsgPostCopy = RegisterMessage("PostCopy");
	mn_MsgMyDestroy = RegisterMessage("MyDestroy");
	mn_MsgUpdateSizes = RegisterMessage("UpdateSizes");
	mn_MsgUpdateCursorInfo = RegisterMessage("UpdateCursorInfo");
	mn_MsgSetWindowMode = RegisterMessage("SetWindowMode");
	mn_MsgUpdateTitle = RegisterMessage("UpdateTitle");
	mn_MsgUpdateScrollInfo = RegisterMessage("UpdateScrollInfo");
	mn_MsgUpdateTabs = RegisterMessage("UpdateTabs"); mn_ReqMsgUpdateTabs = 0; //RegisterWindowMessage(CONEMUMSG_UPDATETABS);
	mn_MsgOldCmdVer = RegisterMessage("OldCmdVer"); mb_InShowOldCmdVersion = FALSE;
	mn_MsgTabCommand = RegisterMessage("TabCommand");
	mn_MsgTabSwitchFromHook = RegisterMessage("CONEMUMSG_SWITCHCON",CONEMUMSG_SWITCHCON); //mb_InWinTabSwitch = FALSE;
	mn_MsgWinKeyFromHook = RegisterMessage("CONEMUMSG_HOOKEDKEY",CONEMUMSG_HOOKEDKEY);
	mn_PostConsoleResize = RegisterMessage("PostConsoleResize");
	mn_ConsoleLangChanged = RegisterMessage("ConsoleLangChanged");
	mn_MsgPostOnBufferHeight = RegisterMessage("PostOnBufferHeight");
	mn_MsgFlashWindow = RegisterMessage("CONEMUMSG_FLASHWINDOW",CONEMUMSG_FLASHWINDOW);
	mn_MsgPostAltF9 = RegisterMessage("PostAltF9");
	mn_MsgInitInactiveDC = RegisterMessage("InitInactiveDC");
	mn_MsgUpdateProcDisplay = RegisterMessage("UpdateProcDisplay");
	mn_MsgFontSetSize = RegisterMessage("FontSetSize");
	mn_MsgDisplayRConError = RegisterMessage("DisplayRConError");
	mn_MsgMacroFontSetName = RegisterMessage("MacroFontSetName");
	mn_MsgCreateViewWindow = RegisterMessage("CreateViewWindow");
	mn_MsgPostTaskbarActivate = RegisterMessage("PostTaskbarActivate"); mb_PostTaskbarActivate = FALSE;
	mn_MsgInitVConGhost = RegisterMessage("InitVConGhost");
	mn_MsgPanelViewMapCoord = RegisterMessage("CONEMUMSG_PNLVIEWMAPCOORD",CONEMUMSG_PNLVIEWMAPCOORD);
	mn_MsgTaskBarCreated = RegisterMessage("TaskbarCreated",L"TaskbarCreated");
	mn_MsgTaskBarBtnCreated = RegisterMessage("TaskbarButtonCreated",L"TaskbarButtonCreated");
	mn_MsgSheelHook = RegisterMessage("SHELLHOOK",L"SHELLHOOK");
	mn_MsgDeleteVConMainThread = RegisterMessage("DeleteVConMainThread");
	mn_MsgReqChangeCurPalette = RegisterMessage("ChangeCurrentPalette");
	mn_MsgMacroExecSync = RegisterMessage("MacroExecSync");
	mn_MsgActivateVCon = RegisterMessage("ActivateVCon");
	mn_MsgPostScClose = RegisterMessage("ScClose");
	mn_MsgOurSysCommand = RegisterMessage("UM_SYSCOMMAND");
	mn_MsgCallMainThread = RegisterMessage("CallMainThread");
}

bool CConEmuMain::isInputGrouped() const
{
	return mb_GroupInputFlag;
}

bool CConEmuMain::isMingwMode()
{
	return (m_InstallMode & cm_MinGW) == cm_MinGW;
}

bool CConEmuMain::isUpdateAllowed()
{
	return !(m_InstallMode & (cm_MinGW));
}

bool CConEmuMain::isMSysStartup()
{
	return (m_InstallMode & cm_MSysStartup) == cm_MSysStartup;
}

void LogFocusInfo(LPCWSTR asInfo, int Level/*=1*/)
{
	if (!asInfo)
		return;

	DWORD nStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD nStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	wchar_t szStyles[64]; swprintf_c(szStyles, L" Style=x%08X ExStyle=x%08X", nStyle, nStyleEx);

	if (gpSet->isLogging(Level))
	{
		CEStr lsMsg(asInfo, szStyles);
		gpConEmu->LogString(lsMsg, TRUE);
	}

	#ifdef _DEBUG
	if ((Level == 1) || gpSet->isLogging(Level))
	{
		const CEStr lsMsg(asInfo, szStyles);
		DEBUGSTRFOCUS(lsMsg);
	}
	#endif
}

void CConEmuMain::SetAppID(LPCWSTR asExtraArgs)
{
	BYTE md5Ok = 0;
	wchar_t szId[40] = L"";
	const CEStr lsFull(ms_ConEmuExeDir, asExtraArgs);

	if (!lsFull.IsEmpty())
	{
		const auto iLen = lsFull.GetLen();
		CharUpperBuff(lsFull.data(), iLen);

		if (gpSet->isLogging())
		{
			const CEStr lsLog(L"Creating AppID from data: `", lsFull, L"`");
			LogString(lsLog);
		}

		unsigned char data[16] = {};
		MD5_CTX ctx = {};
		MD5_Init(&ctx);
		MD5_Update(&ctx, lsFull.data(), iLen * sizeof(lsFull[0]));
		MD5_Final(data, &ctx);

		for (int i = 0; i < 16; i++)
		{
			const BYTE bt = static_cast<BYTE>(data[i]);
			md5Ok |= bt;
			msprintf(szId+i*2, 3, L"%02x", static_cast<UINT>(bt));
		}
	}

	if (!md5Ok || (szId[0] == 0))
	{
		_ASSERTE(md5Ok && (szId[0] != 0)); // Must be filled already
		lstrcpyn(szId, ms_ConEmuBuild, countof(szId));
	}

	wcscpy_c(ms_AppID, szId);

	// This suffix isn't passed to Win7+ TaskBar AppModelID, but we need to stored it,
	// because it's used in Mappings when ConEmu tries to find existing instance for
	// passing command line in single-instance mode for example...
	wchar_t szSuffix[64] = L"";
	swprintf_c(szSuffix, L"::%u", static_cast<UINT>(CESERVER_REQ_VER));
	// hash - 32char, + 5..6 for Protocol ID, quite be enough
	_ASSERTE((lstrlen(ms_AppID) + lstrlen(szSuffix)) < countof(ms_AppID));
	wcscat_c(ms_AppID, szSuffix);

	if (gpSet->isLogging())
	{
		const CEStr lsLog(L"New AppID: ", ms_AppID);
		LogString(lsLog);
	}
}

void CConEmuMain::StoreWorkDir(LPCWSTR asNewCurDir /*= nullptr*/)
{
	if (asNewCurDir && (asNewCurDir[0] == L':'))
	{
		asNewCurDir = nullptr; // Пути а-ля библиотеки - не поддерживаются при выполнении команд и приложений
	}

	if (asNewCurDir)
	{
		if (*asNewCurDir)
		{
			// Here we are if "/dir" was specified in the app switches
			mb_ConEmuWorkDirArg = true;
			// Save it
			wcscpy_c(ms_ConEmuWorkDir, asNewCurDir);
			// The root of the drive must have trailing '\'
			FixDirEndSlash(ms_ConEmuWorkDir);
		}
	}
	else
	{
		// Запомнить текущую папку (на момент запуска)
		wchar_t szDir[MAX_PATH+1] = L"";
		DWORD nDirLen = GetCurrentDirectory(MAX_PATH, szDir);

		if (!nDirLen || nDirLen>MAX_PATH)
		{
			// Do not force to %ConEmuDir%
			//wcscpy_c(szDir, ms_ConEmuExeDir);
			szDir[0] = 0;
		}
		else
		{
			// If it not a root of the drive - cut end slash, to be the same with ms_ConEmuExeDir
			FixDirEndSlash(szDir);
		}

		if (!mb_ConEmuHere)
		{
			// Prepare WinDir for comparison
			wchar_t szWinDir[MAX_PATH] = L"";
			if (GetWindowsDirectory(szWinDir, MAX_PATH-10))
				FixDirEndSlash(szWinDir);
			wchar_t szSysDir[MAX_PATH];
			wcscpy_c(szSysDir, szWinDir);
			wcscat_c(szSysDir, L"\\system32");

			// If current directory is C:\Windows, or C:\Windows\System32, or just %ConEmuDir%
			// force working directory to the %UserProfile%
			if (!*szDir
				|| (lstrcmpi(szDir, szWinDir) == 0)
				|| (lstrcmpi(szDir, szSysDir) == 0)
				|| (lstrcmpi(szDir, ms_ConEmuExeDir) == 0))
			{
				if (SHGetSpecialFolderPath(nullptr, szDir, CSIDL_PROFILE, FALSE))
				{
					FixDirEndSlash(szDir);
				}
				if (!*szDir)
				{
					// Finally, if all others failed
					wcscpy_c(szDir, ms_ConEmuExeDir);
				}
			}
		}

		wcscpy_c(ms_ConEmuWorkDir, szDir);
	}

	wchar_t szDrive[MAX_PATH];
	SetEnvironmentVariable(ENV_CONEMUWORKDRIVE_VAR_W, GetDrive(ms_ConEmuWorkDir, szDrive, countof(szDrive)));
}

LPCWSTR CConEmuMain::WorkDir(LPCWSTR asOverrideCurDir /*= nullptr*/)
{
	LPCWSTR pszWorkDir;
	if (asOverrideCurDir && *asOverrideCurDir)
	{
		pszWorkDir = asOverrideCurDir;
	}
	else
	{
		_ASSERTE(this && ms_ConEmuWorkDir[0]!=0);
		pszWorkDir = ms_ConEmuWorkDir;
	}

	if (pszWorkDir && (pszWorkDir[0] == L':'))
		pszWorkDir = L""; // Пути а-ля библиотеки - не поддерживаются при выполнении команд и приложений
	return pszWorkDir;
}

bool CConEmuMain::ChangeWorkDir(LPCWSTR asTempCurDir)
{
	bool bChanged = false;
	BOOL bApi = FALSE; DWORD nApiErr = 0;

	if (asTempCurDir)
	{
		bApi = SetCurrentDirectoryW(asTempCurDir);
		if (bApi)
			bChanged = true;
		else
			nApiErr = GetLastError();
	}
	else
	{
		bApi = SetCurrentDirectoryW(ms_ConEmuExeDir);
		nApiErr = bApi ? 0 : GetLastError();
	}

	UNREFERENCED_PARAMETER(bApi);
	UNREFERENCED_PARAMETER(nApiErr);
	return bChanged;
}

void CConEmuMain::AppendExtraArgs(LPCWSTR asSwitch, LPCWSTR asSwitchValue /*= nullptr*/)
{
	if (!asSwitch || !*asSwitch)
		return;

	mps_ConEmuExtraArgs.Append(asSwitch, L" ");

	if (asSwitchValue && *asSwitchValue)
	{
		if (IsQuotationNeeded(asSwitchValue))
			mps_ConEmuExtraArgs.Append(L"\"", asSwitchValue, L"\" ");
		else
			mps_ConEmuExtraArgs.Append(asSwitchValue, L" ");
	}
}

// Returns either nullptr string, or arguments with *trailing space*
LPCWSTR CConEmuMain::MakeConEmuStartArgs(CEStr& rsArgs, LPCWSTR asOtherConfig /*= nullptr*/) const
{
	bool bSpecialXml = false;
	LPCWSTR pszXmlFile = gpConEmu->ConEmuXml(&bSpecialXml);
	if (pszXmlFile && (!bSpecialXml || !*pszXmlFile))
		pszXmlFile = nullptr;

	LPCWSTR pszConfig = (asOtherConfig && *asOtherConfig) ? asOtherConfig : gpSetCls->GetConfigName();
	if (pszConfig && !*pszConfig)
		pszConfig = nullptr;

	const wchar_t* pszAddArgs = gpConEmu->mps_ConEmuExtraArgs.c_str();

	const size_t cchMax = 1 // _wcscat_c requires size including terminating zero
		+ (pszConfig ? (_tcslen(pszConfig) + 16) : 0)
		+ (pszXmlFile ? (_tcslen(pszXmlFile) + 32) : 0)
		+ (gpSetCls->isResetBasicSettings ? 7 : 0)
		+ (pszAddArgs ? (_tcslen(pszAddArgs) + 1) : 0);
	if (!cchMax)
	{
		rsArgs.Clear();
		return nullptr;
	}

	wchar_t* pszBuf = rsArgs.GetBuffer(cchMax);
	if (!pszBuf)
		return nullptr;

	pszBuf[0] = 0;

	if (pszXmlFile && *pszXmlFile)
	{
		_wcscat_c(pszBuf, cchMax, L"-LoadCfgFile \"");
		_wcscat_c(pszBuf, cchMax, pszXmlFile);
		_wcscat_c(pszBuf, cchMax, L"\" ");
	}
	if (pszConfig && *pszConfig)
	{
		_wcscat_c(pszBuf, cchMax, L"-config \"");
		_wcscat_c(pszBuf, cchMax, pszConfig);
		_wcscat_c(pszBuf, cchMax, L"\" ");
	}
	if (pszAddArgs && *pszAddArgs)
	{
		_wcscat_c(pszBuf, cchMax, pszAddArgs);
		if (pszAddArgs[_tcslen(pszAddArgs)-1] != L' ')
			_wcscat_c(pszBuf, cchMax, L" ");
	}
	if (gpSetCls->isResetBasicSettings)
	{
		_wcscat_c(pszBuf, cchMax, L"-basic ");
	}

	return rsArgs.ms_Val;
}

bool CConEmuMain::CheckRequiredFiles()
{
	wchar_t szPath[MAX_PATH+32];
	struct ReqFile {
		int  bits;
		bool isRequired;
		wchar_t name[16];
	} requiredFiles[] = {
		{32, true, ConEmuC_32_EXE},
		{64, true, ConEmuC_64_EXE},
		{32, true, ConEmuCD_32_DLL},
		{64, true, ConEmuCD_64_DLL},
		{32, gpSet->isUseInjects, ConEmuHk_32_DLL},
		{64, gpSet->isUseInjects, ConEmuHk_64_DLL},
	};

	std::wstring szRequired, szRecommended;
	const bool isWin64 = IsWindows64();
	const int  nExeBits = WIN3264TEST(32,64);

	wcscpy_c(szPath, ms_ConEmuBaseDir);
	wcscat_c(szPath, L"\\");
	wchar_t* pszSlash = szPath + _tcslen(szPath);
	uint64_t nFileSize = 0;
	size_t missedRequired = 0;
	// on 32bit os we check only ConEmuHk_32_DLL
	const size_t countToDisableInjects = isWin64 ? 2 : 1;

	for (const auto& requiredFile : requiredFiles)
	{
		if (requiredFile.bits == 64)
		{
			if (!isWin64)
				continue; // 64битные файлы в 32битных ОС не нужны
		}

		_wcscpy_c(pszSlash, 32, requiredFile.name);
		if (!FileExists(szPath, &nFileSize) || !nFileSize)
		{
			if (!requiredFile.isRequired)
			{
				++missedRequired;
				continue;
			}
			auto& pszList = (requiredFile.bits == nExeBits) ? szRequired : szRecommended;
			if (!pszList.empty())
				pszList += L", ";;
			pszList += requiredFile.name;
		}
	}

	// if both ConEmuHk.dll and ConEmuHk64.dll were deleted
	if (missedRequired >= countToDisableInjects)
	{
		LogString(L"Injects are disabled for root processes");
		mb_DontUseInjects = true;
	}

	if (szRequired[0] || szRecommended[0])
	{
		std::wstring szMsg;
		szMsg.reserve(szRequired.length() + szRecommended.length() + wcslen(ms_ConEmuExe) + 255);
		szMsg += !szRequired.empty() ? L"Critical error\n\n" : L"Warning\n\n";
		if (!szRequired.empty())
		{
			szMsg += L"Required files not found!\n";
			szMsg += szRequired;
			szMsg += L"\n\n";
		}
		if (!szRecommended.empty())
		{
			szMsg += L"Recommended files not found!\n";
			szMsg += szRecommended;
			szMsg += L"\n\n";
		}
		szMsg += L"ConEmu was started from:\n";
		szMsg += ms_ConEmuExe;
		szMsg += L"\n";
		if (!szRequired.empty())
		{
			szMsg += L"\nConEmu will exit now";
		}
		MsgBox(szMsg.c_str(), MB_SYSTEMMODAL|(!szRequired.empty() ? MB_ICONSTOP : MB_ICONWARNING), GetDefaultTitle(), nullptr);
	}

	if (!szRequired.empty())
		return false;
	return true; // allowed to continue
}

bool CConEmuMain::CanUseInjects() const
{
	return !mb_DontUseInjects;
}

bool CConEmuMain::CheckBaseDir() const
{
	bool lbBaseFound = false;
	wchar_t szBaseFile[MAX_PATH+12];

	lstrcpyn(szBaseFile, ms_ConEmuBaseDir, countof(szBaseFile));
	wchar_t *pszSlash = szBaseFile + _tcslen(szBaseFile);
	lstrcpy(pszSlash, L"\\ConEmuC.exe");

	if (FileExists(szBaseFile))
	{
		lbBaseFound = true;
	}

	#ifdef WIN64
	if (!lbBaseFound)
	{
		lstrcpy(pszSlash, L"\\" ConEmuC_64_EXE);

		if (FileExists(szBaseFile))
		{
			lbBaseFound = true;
		}
	}
	#endif

	return lbBaseFound;
}

bool CConEmuMain::SetConfigFile(LPCWSTR asFilePath, bool abWriteReq /*= false*/, bool abSpecialPath /*= false*/)
{
	LPCWSTR pszExt = asFilePath ? PointToExt(asFilePath) : nullptr;
	int nLen = asFilePath ? lstrlen(asFilePath) : 0;

	if (!asFilePath || !*asFilePath)
	{
		DisplayLastError(L"Empty file path specified in CConEmuMain::SetConfigFile", -1);
		return false;
	}

	if (!asFilePath || !*asFilePath || !pszExt || !*pszExt || (nLen > MAX_PATH))
	{
		CEStr lsMsg(
			L"Invalid file path specified in CConEmuMain::SetConfigFile",
			L"\n", L"asFilePath=`", asFilePath, L"`",
			L"\n", L"Only *.xml files are allowed");
		DisplayLastError(lsMsg, -1);
		return false;
	}

	CEStr szPath;
	int nFileType = 0;

	if (lstrcmpi(pszExt, L".ini") == 0)
	{
		nFileType = 1;
	}
	else if (lstrcmpi(pszExt, L".xml") == 0)
	{
		nFileType = 0;
	}
	else
	{
		DisplayLastError(L"Unsupported file extension specified in CConEmuMain::SetConfigFile", -1);
		return false;
	}


	//The path must be already in full form, call apiGetFullPathName just for ensure (but it may return wrong path)
	_ASSERTE((asFilePath[1]==L':' && asFilePath[2]==L'\\') || (asFilePath[0]==L'\\' && asFilePath[1]==L'\\'));
	nLen = apiGetFullPathName(asFilePath, szPath);
	if (!nLen || (nLen >= MAX_PATH))
	{
		DisplayLastError(L"Failed to expand specified file path in CConEmuMain::SetConfigFile");
		return false;
	}

	LPCWSTR pszFilePath = PointToName(szPath);
	if (!pszFilePath || (pszFilePath == szPath.ms_Val))
	{
		DisplayLastError(L"Invalid file path specified in CConEmuMain::SetConfigFile (backslash not found)", -1);
		return false;
	}


	// Create the folder if it's absent
	// Function accepts paths with trailing file names
	MyCreateDirectory(szPath.ms_Val);


	// Нужно создать файл, если его нету.
	// Если просили доступ на запись - то однозначно CreateFile
	if (abWriteReq || !FileExists(szPath))
	{
		HANDLE hFile = CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (!hFile || (hFile == INVALID_HANDLE_VALUE))
		{
			DWORD nErrCode = GetLastError();
			wchar_t szErrMsg[MAX_PATH*2];
			wcscpy_c(szErrMsg, L"Failed to create configuration file:\r\n");
			wcscat_c(szErrMsg, szPath);
			DisplayLastError(szErrMsg, nErrCode);
			return false;
		}
		CloseHandle(hFile);
	}

	if (nFileType == 1)
	{
		ms_ConEmuXml[0] = 0;
		wcscpy_c(ms_ConEmuIni, asFilePath);
	}
	else
	{
		ms_ConEmuIni[0] = 0;
		wcscpy_c(ms_ConEmuXml, asFilePath);
	}

	mb_SpecialConfigPath = abSpecialPath;

	return true;
}

void CConEmuMain::SetForceUseRegistry()
{
	mb_ForceUseRegistry = true;
}

LPWSTR CConEmuMain::ConEmuXml(bool* pbSpecialPath /*= nullptr*/)
{
	if (pbSpecialPath) *pbSpecialPath = false;

	if (mb_ForceUseRegistry)
	{
		return L"";
	}

	if (ms_ConEmuXml[0])
	{
		if (FileExists(ms_ConEmuXml))
		{
			if (pbSpecialPath) *pbSpecialPath = mb_SpecialConfigPath;
			return ms_ConEmuXml;
		}
	}

	TODO("Хорошо бы еще дать возможность пользователю использовать два файла - системный (предустановки) и пользовательский (настройки)");

	CEStr szXmlFile;
	if (FindConEmuXml(szXmlFile))
	{
		wcscpy_c(ms_ConEmuXml, szXmlFile);
	}
	else
	{
		// If create new, than create it in the %ConEmuBaseDir%
		// Minimize rubbish in the root folder
		wcscpy_c(ms_ConEmuXml, ms_ConEmuBaseDir); wcscat_c(ms_ConEmuXml, L"\\ConEmu.xml");
	}

	return ms_ConEmuXml;
}

LPCWSTR CConEmuMain::FindConEmuXml(CEStr& szXmlFile)
{
	szXmlFile.Clear();

	// Ищем файл портабельных настроек (возвращаем первый найденный, приоритет...)
	// Search for portable xml file settings. Return first found due to defined priority.
	// We support both "ConEmu.xml" and its ‘dot version’ ".ConEmu.xml"
	MArray<LPCWSTR> pszSearchXml;
	if (isMingwMode())
	{
		// Since Git-For-Windows 2.x
		// * ConEmu.exe is supposed to be in /opt/bin/
		// * ConEmu.xml in /share/conemu/
		pszSearchXml.push_back(L"%ConEmuDir%\\..\\share\\conemu\\ConEmu.xml");
		pszSearchXml.push_back(L"%ConEmuDir%\\..\\share\\conemu\\.ConEmu.xml");
	}
	pszSearchXml.push_back(L"%ConEmuDir%\\ConEmu.xml");
	pszSearchXml.push_back(L"%ConEmuDir%\\.ConEmu.xml");
	pszSearchXml.push_back(L"%ConEmuBaseDir%\\ConEmu.xml");
	pszSearchXml.push_back(L"%ConEmuBaseDir%\\.ConEmu.xml");
	pszSearchXml.push_back(L"%APPDATA%\\ConEmu.xml");
	pszSearchXml.push_back(L"%APPDATA%\\.ConEmu.xml");

	for (INT_PTR i = 0; i < pszSearchXml.size(); i++)
	{
		if (CEStr szExpand = GetFullPathNameEx(pszSearchXml[i]))
		{
			if (FileExists(szExpand))
			{
				szXmlFile = szExpand.Detach();
				break;
			}
		}
	}

	return szXmlFile.IsEmpty() ? nullptr : szXmlFile.c_str();
}

LPWSTR CConEmuMain::ConEmuIni()
{
	if (ms_ConEmuIni[0])
	{
		if (FileExists(ms_ConEmuIni))
			return ms_ConEmuIni;
	}

	TODO("Хорошо бы еще дать возможность пользователю использовать два файла - системный (предустановки) и пользовательский (настройки)");

	// Ищем файл портабельных настроек (возвращаем первый найденный, приоритет...)
	CEStr searchIni[] = {
		ExpandEnvStr(L"%ConEmuDir%\\ConEmu.ini"),
		ExpandEnvStr(L"%ConEmuBaseDir%\\ConEmu.ini"),
		ExpandEnvStr(L"%APPDATA%\\ConEmu.ini"),
	};

	for (const auto& ini : searchIni)
	{
		if (FileExists(ini))
		{
			wcscpy_c(ms_ConEmuIni, ini);
			goto fin;
		}
	}

	// Но если _создавать_ новый, то в BaseDir! Чтобы в корне не мусорить
	wcscpy_c(ms_ConEmuIni, ms_ConEmuBaseDir); wcscat_c(ms_ConEmuIni, L"\\ConEmu.ini");

fin:
	return ms_ConEmuIni;
}

LPCWSTR CConEmuMain::ConEmuCExeFull(LPCWSTR asCmdLine/*=nullptr*/)
{
	// Если OS - 32битная или в папке ConEmu был найден только один из "серверов"
	if (!IsWindows64() || !lstrcmp(ms_ConEmuC32Full, ms_ConEmuC64Full))
	{
		// Сразу вернуть
		return ms_ConEmuC32Full;
	}

	//LPCWSTR pszServer = ms_ConEmuC32Full;
	bool lbCmd = false, lbFound = false;
	int bits = IsWindows64() ? 64 : 32;
	CEStr expanded;

	if (!asCmdLine || !*asCmdLine)
	{
		// Если строка запуска не указана - считаем, что запускается ComSpec
		lbCmd = true;
	}
	else
	{
		// Strip from cmdline some commands, which ConEmuC can process internally
		ProcessSetEnvCmd(asCmdLine);

		// Проверить битность asCmdLine во избежание лишних запусков серверов для Inject
		// и корректной битности запускаемого процессора по настройке
		CmdArg szTemp;
		if (!FileExists(asCmdLine))
		{
			const wchar_t *psz = asCmdLine;
			if ((psz = NextArg(psz, szTemp)))
				asCmdLine = szTemp;
		}

		if (wcschr(asCmdLine, L'%'))
		{
			expanded = ExpandEnvStr(asCmdLine);
			if (expanded)
				asCmdLine = expanded.c_str();
		}

		// Если путь указан полностью - берем битность из него, иначе - проверяем "на cmd"
		if ((lstrcmpi(asCmdLine, L"cmd") == 0) || (lstrcmpi(asCmdLine, L"cmd.exe") == 0))
		{
			lbCmd = true;
		}
		else
		{
			const auto* pszExt = PointToExt(asCmdLine);
			if (pszExt && (lstrcmpi(pszExt, L".exe") != 0) && (lstrcmpi(pszExt, L".com") != 0))
			{
				// Если указано расширение, и это не .exe и не .com - считаем, что запуск через ComProcessor
				lbCmd = true;
			}
			else
			{
				CEStr szFind;
				if (szFind.Set(asCmdLine))
					CharUpperBuff(szFind.ms_Val, lstrlen(szFind));
				// We don't care if "%windir% is actual start
				if (!szFind.IsEmpty() && (wcsstr(szFind, L"\\SYSNATIVE\\") || wcsstr(szFind, L"\\SYSWOW64\\")))
				{
					// For "SysNative" we need 32-bit server, the 64-bit applications just does not see this folder.
					// The "SysWow64" folder contains only 32-bit application.
					bits = 32;
				}
				else
				{
					MWow64Disable wow;
					wow.Disable();
					DWORD imageSubsystem = 0;
					DWORD imageBits = 0;
					DWORD fileAttrs = 0;
					lbFound = GetImageSubsystem(asCmdLine, imageSubsystem, imageBits, fileAttrs);
					// Try to search for the file in any case.
					// Moreover, if we didn't found the file, asCmdLine could miss the file extension, try to add it.
					if (!lbFound)
					{
						const auto* pszSearchFile = asCmdLine;
						const auto* const filePtr = PointToName(asCmdLine);
						CEStr pszSearchPath;
						if (filePtr && (filePtr > asCmdLine))
						{
							const ssize_t fileNameIndex = filePtr - asCmdLine;
							pszSearchPath.Set(asCmdLine);
							if (pszSearchPath)
							{
								pszSearchFile = filePtr;
								pszSearchPath.SetAt(fileNameIndex, 0);
							}
						}

						bool bSearchRc = (apiSearchPath(pszSearchPath, pszSearchFile, pszExt ? nullptr : L".exe", szFind) > 0);
						if (!bSearchRc && !pszSearchPath)
						{
							const CEStr szRoot(ms_ConEmuExeDir);
							// One folder above ConEmu's directory
							wchar_t* pszRootSlash = wcsrchr(szRoot.data(), L'\\');
							if (pszRootSlash)
								*pszRootSlash = 0;
							bSearchRc = (apiSearchPath(szRoot, pszSearchFile, pszExt ? nullptr : L".exe", szFind) > 0);
						}
						if (bSearchRc)
						{
							lbFound = GetImageSubsystem(szFind, imageSubsystem, imageBits, fileAttrs);
						}
					}

					if (lbFound)
						bits = imageBits;
				}
			}
		}
	}

	if (lbCmd)
	{
		if (gpSet->ComSpec.csBits == csb_SameApp)
			bits = WIN3264TEST(32,64);
		else if (gpSet->ComSpec.csBits == csb_x32)
			bits = 32;
	}

	return (bits == 64) ? ms_ConEmuC64Full : ms_ConEmuC32Full;
}

BOOL CConEmuMain::Init()
{
	_ASSERTE(mp_TabBar != nullptr);

	// Чтобы не блокировать папку запуска - CD
	ChangeWorkDir(nullptr);

	// Только по настройке, а то дочерние процессы с тем же Affinity запускаются...
	// На тормоза - не влияет. Но вроде бы на многопроцессорных из-за глюков в железе могут быть ошибки подсчета производительности, если этого не сделать
	if (gpSet->nAffinity)
		SetProcessAffinityMask(GetCurrentProcess(), gpSet->nAffinity);

	ConEmuAbout::InitCommCtrls();

	SetRequestedMonitor(gpStartEnv->hStartMon);

	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	//SetThreadAffinityMask(GetCurrentThread(), 1);
	/*DWORD dwErr = 0;
	HMODULE hInf = LoadLibrary(L"infis.dll");
	if (!hInf)
	dwErr = GetLastError();*/
	//!!!ICON
	LoadIcons();

	mp_Tip = new CToolTip();
	//m_Child = new CConEmuChild();
	//m_Back = new CConEmuBack();
	//m_Macro = new CConEmuMacro();
	//#pragma message("Win2k: EVENT_CONSOLE_START_APPLICATION, EVENT_CONSOLE_END_APPLICATION")
	//Нас интересуют только START и END. Все остальные события приходят от ConEmuC через серверный пайп
	//#if defined(__GNUC__)
	//HMODULE hUser32 = GetModuleHandle(L"user32.dll");
	//FSetWinEventHook SetWinEventHook = (FSetWinEventHook)GetProcAddress(hUser32, "SetWinEventHook");
	//#endif
#ifndef _WIN64
	// Интересует только запуск/останов 16битных приложений
	// NTVDM нету в 64битных версиях Windows
	if (!IsWindows64())
	{
		mh_WinHook = SetWinEventHook(EVENT_CONSOLE_START_APPLICATION/*EVENT_CONSOLE_CARET*/,EVENT_CONSOLE_END_APPLICATION,
	                             nullptr, (WINEVENTPROC)CConEmuMain::WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT);
	}
#endif
	//mh_PopupHook = SetWinEventHook(EVENT_SYSTEM_MENUPOPUPSTART,EVENT_SYSTEM_MENUPOPUPSTART,
	//    nullptr, (WINEVENTPROC)CConEmuMain::WinEventProc, 0,0, WINEVENT_OUTOFCONTEXT);
	/*mh_Psapi = LoadLibrary(_T("psapi.dll"));
	if (mh_Psapi) {
		GetModuleFileNameEx = (FGetModuleFileNameEx)GetProcAddress(mh_Psapi, "GetModuleFileNameExW");
		if (GetModuleFileNameEx)
			return TRUE;
	}*/
	/*DWORD dwErr = GetLastError();
	TCHAR szErr[255];
	_wsprintf(szErr, countof(szErr), _T("Can't initialize psapi!\r\nLastError = 0x%08x"), dwErr);
	MBoxA(szErr);
	return FALSE;*/

	return TRUE;
}

void CConEmuMain::DeinitOnDestroy(HWND hWnd, bool abForce /*= false*/)
{
	// May it hang in some cases?
	session.SetSessionNotification(false);

	// Ensure "RCon starting queue" is terminated
	mp_RunQueue->Terminate();

	// If the window was closed as part of children termination (InsideIntegration)
	// Than, close all spawned consoles
	CVConGroup::OnDestroyConEmu();

	// and destroy all child windows
	if (mn_InOurDestroy == 0)
		DestroyAllChildWindows();

	if (mp_AttachDlg)
	{
		mp_AttachDlg->Close();
	}
	if (mp_RecreateDlg)
	{
		mp_RecreateDlg->Close();
	}

	// Function cares about single execution
	gpSet->SaveSettingsOnExit();

	// Kill all ssh-agents.exe processes started from ConEmu consoles
	TerminateSshAgents();

	// Required before ResetEvent(mh_ConEmuAliveEvent),
	// to avoid problems in another instance with hotkey registering
	m_Hotkeys->RegisterMinRestore(false);

	if (mb_ConEmuAliveOwned && mh_ConEmuAliveEvent)
	{
		ResetEvent(mh_ConEmuAliveEvent); // Дадим другим процессам "завладеть" первенством
		SafeCloseHandle(mh_ConEmuAliveEvent);
		mb_ConEmuAliveOwned = FALSE;
	}

	if (mb_MouseCaptured)
	{
		ReleaseCapture();
		mb_MouseCaptured = FALSE;
	}

	//120122 - Теперь через PipeServer
	m_GuiServer.Stop(true);

	CVConGroup::DestroyAllVCon();

	#ifndef _WIN64
	if (mh_WinHook)
	{
		UnhookWinEvent(mh_WinHook);
		mh_WinHook = nullptr;
	}
	#endif

	if (mb_ShellHookRegistered)
	{
		mb_ShellHookRegistered = false;
		DeregisterShellHookWindow(hWnd);
	}

	if (mp_DragDrop)
	{
		delete mp_DragDrop;
		mp_DragDrop = nullptr;
	}

	Icon.RemoveTrayIcon(true);

	Taskbar_Release();

	GetGlobalHotkeys().UnRegisterHotKeys(TRUE);

	SetKillTimer(false, TIMER_MAIN_ID, 0);
	SetKillTimer(false, TIMER_CONREDRAW_ID, 0);
	SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
	SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
	SetKillTimer(false, TIMER_QUAKE_AUTOHIDE_ID, 0);
	SetKillTimer(false, TIMER_ACTIVATESPLIT_ID, 0);

	CVConGroup::StopSignalAll();
}

// Kill all ssh-agents.exe processes started from ConEmu consoles
void CConEmuMain::TerminateSshAgents()
{
	if (!gpSet->isKillSshAgent)
	{
		return;
	}

	_ASSERTE(isMainThread());
	wchar_t szLog[128];

	MSectionLockSimple lock; lock.Lock(m_SshAgentsLock);
	for (INT_PTR i = 0; i < m_SshAgents.size(); i++)
	{
		if (m_SshAgents[i].hAgent != nullptr)
		{
			DWORD nWait = WaitForSingleObject(m_SshAgents[i].hAgent, 0);
			if (nWait == WAIT_OBJECT_0)
			{
				GetExitCodeProcess(m_SshAgents[i].hAgent, &m_SshAgents[i].nExitCode);
				swprintf_c(szLog, L"ssh-agent.exe with PID %u has been already terminated with code %u", m_SshAgents[i].nAgentPID, m_SshAgents[i].nExitCode);
			}
			else
			{
				TerminateProcess(m_SshAgents[i].hAgent, 1);
				swprintf_c(szLog, L"ssh-agent.exe with PID %u was killed on ConEmu termination", m_SshAgents[i].nAgentPID);
			}
			LogString(szLog);
			SafeCloseHandle(m_SshAgents[i].hAgent);
		}
	}
}

// Comes from one of console processes running in ConEmu tabs.
// It's expected (at the moment at least) that console, spawned ssh-agent.exe,
// must be started under the same credentials as ConEmu.
void CConEmuMain::RegisterSshAgent(DWORD SshAgentPID)
{
	wchar_t szLog[128];

	MSectionLockSimple lock; lock.Lock(m_SshAgentsLock);
	SshAgent agent = {OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, SshAgentPID), SshAgentPID};
	if (agent.hAgent == nullptr)
	{
		DWORD code = GetLastError();
		swprintf_c(szLog, L"Failed to open handle for ssh-agent.exe with PID %u, errorcode 0x%08X (%u)", SshAgentPID, code, code);
	}
	else
	{
		swprintf_c(szLog, L"ssh-agent.exe with PID %u has been started", SshAgentPID);
		m_SshAgents.push_back(agent);
	}
	lock.Unlock();
	LogString(szLog);
}

bool CConEmuMain::IsResetBasicSettings()
{
	return gpSetCls->isResetBasicSettings;
}

bool CConEmuMain::IsFastSetupDisabled()
{
	return (gpSetCls->isResetBasicSettings || gpSetCls->isFastSetupDisabled);
}

bool CConEmuMain::IsAllowSaveSettingsOnExit()
{
	if (gpSetCls->ibDisableSaveSettingsOnExit || IsResetBasicSettings())
		return false;
	if (mn_StartupFinished < ss_VConStarted || mn_StartupFinished >= ss_Destroying)
		return false;
	return true;
}

void CConEmuMain::OnUseGlass(bool abEnableGlass)
{
	//CheckMenuItem(GetSysMenu(false), ID_USEGLASS, MF_BYCOMMAND | (abEnableGlass ? MF_CHECKED : MF_UNCHECKED));
}
void CConEmuMain::OnUseTheming(bool abEnableTheming)
{
	//CheckMenuItem(GetSysMenu(false), ID_USETHEME, MF_BYCOMMAND | (abEnableTheming ? MF_CHECKED : MF_UNCHECKED));
}
void CConEmuMain::OnUseDwm(bool abEnableDwm)
{
	//CheckMenuItem(GetSysMenu(false), ID_ISDWM, MF_BYCOMMAND | (abEnableDwm ? MF_CHECKED : MF_UNCHECKED));
}

bool CConEmuMain::isTabsShown() const
{
	if (gpSet->isTabs == 1)
		return true;
	return (mp_TabBar && mp_TabBar->IsTabsActive());
}

void CConEmuMain::SetWindowStyle(DWORD anStyle)
{
	SetWindowStyle(ghWnd, anStyle);
}

void CConEmuMain::SetWindowStyle(HWND ahWnd, DWORD anStyle)
{
	LONG lRc = SetWindowLong(ahWnd, GWL_STYLE, anStyle);
	UNREFERENCED_PARAMETER(lRc);
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE
DWORD CConEmuMain::GetWindowStyle()
{
	DWORD style = WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

	if (mp_Inside)
	{
		style |= WS_CHILD|WS_SYSMENU;
	}
	else
	{
		//if (gpSet->isShowOnTaskBar) // ghWndApp
		//	style |= WS_POPUPWINDOW | WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
		//else
		style |= WS_OVERLAPPEDWINDOW;
		style = FixWindowStyle(style);
		//if (gpSet->isTabsOnTaskBar() && gOSVer.dwMajorVersion <= 5)
		//{
		//	style |= WS_POPUP;
		//}

		#ifndef CHILD_DESK_MODE
		if (opt.DesktopMode)
			style |= WS_POPUP;
		#endif

		//if (ghWnd) {
		//	if (gpSet->isHideCaptionAlways)
		//		style &= ~(WS_CAPTION/*|WS_THICKFRAME*/);
		//	else
		//		style |= (WS_CAPTION|/*WS_THICKFRAME|*/WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
		//}
	}
	return style;
}

DWORD CConEmuMain::FixWindowStyle(DWORD dwStyle, ConEmuWindowMode wmNewMode /*= wmCurrent*/)
{
	/* WS_CAPTION == (WS_BORDER | WS_DLGFRAME) */

	if ((wmNewMode == wmCurrent) || (wmNewMode == wmNotChanging))
	{
		wmNewMode = WindowMode;
	}

	if (mp_Inside)
	{
		dwStyle &= ~(WS_CAPTION|WS_THICKFRAME);
		dwStyle |= WS_CHILD|WS_SYSMENU;
	}
	else if (isCaptionHidden(wmNewMode))
	{
		dwStyle &= ~WS_CAPTION;

		const bool noThickFrame = isSelfFrame(wmNewMode)
			|| mb_DisableThickFrame  // eliminate glitches during quake animation
			;

		if (noThickFrame)
		{
			dwStyle &= ~(WS_THICKFRAME);
		}

		// m_ForceShowFrame - to bypass strange MS limitation "not resizing borderless windows with WS_SYSMENU"
		if (m_ForceShowFrame == fsf_Show)
		{
			dwStyle &= ~(WS_THICKFRAME|WS_SYSMENU);
		}
		// User decides to disable standard (DWM) frame with a shadow
		// in favor of borderless or self-drawn-border window
		else if (gpSet->HideCaptionAlwaysFrame() >= 0)
		{
			_ASSERTE(!(dwStyle & WS_THICKFRAME)); // should already be cleared, just ensure...
			dwStyle &= ~(WS_THICKFRAME); // remove standard (DWM) frame and shadows
			dwStyle |= (WS_SYSMENU); // but allow system menu!
		}
		// Normal mode for caption-less window, allow resize using standard (DWM) frame
		else if (!noThickFrame)
		{
			dwStyle |= (WS_THICKFRAME|WS_SYSMENU);
		}
	}
	else
	{
		/* WS_CAPTION == WS_BORDER | WS_DLGFRAME  */
		dwStyle |= WS_CAPTION|WS_THICKFRAME|WS_SYSMENU;
	}

	return dwStyle;
}

void CConEmuMain::SetWindowStyleEx(DWORD anStyleEx)
{
	SetWindowStyleEx(ghWnd, anStyleEx);
}

void CConEmuMain::SetWindowStyleEx(HWND ahWnd, DWORD anStyleEx)
{
	if (gpSet->isLogging())
	{
		char szInfo[100];
		RECT rcWnd = {}; GetWindowRect(ghWnd, &rcWnd);
		sprintf_c(szInfo,
			"SetWindowStyleEx(HWND=x%08X, StyleEx=x%08X)",
			(DWORD)(DWORD_PTR)ahWnd, anStyleEx);
		LogString(szInfo);
	}
	LONG lRc = SetWindowLong(ahWnd, GWL_EXSTYLE, anStyleEx);
	UNREFERENCED_PARAMETER(lRc);
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE_EX
DWORD CConEmuMain::GetWindowStyleEx()
{
	DWORD styleEx = 0;

	if (mp_Inside)
	{
		// ничего вроде не надо
	}
	else
	{
		if (gpSet->nTransparent < 255)
			styleEx |= WS_EX_LAYERED;

		if (gpSet->isAlwaysOnTop)
			styleEx |= WS_EX_TOPMOST;

		if (gpSet->isWindowOnTaskBar(true))
			styleEx |= WS_EX_APPWINDOW;

		#ifndef CHILD_DESK_MODE
		if (opt.DesktopMode)
			styleEx |= WS_EX_TOOLWINDOW;
		#endif
	}

	return styleEx;
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE
DWORD CConEmuMain::GetWorkWindowStyle()
{
	DWORD style = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
	return style;
}

// Эта функция расчитывает необходимые стили по текущим настройкам, а не возвращает GWL_STYLE_EX
DWORD CConEmuMain::GetWorkWindowStyleEx()
{
	DWORD styleEx = 0;
	return styleEx;
}

// The Window shown on TaskBar to get Progress (Windows 7 and higher)
// Also used in CVConGroup::OnFlashWindow and during mouse activation
HWND CConEmuMain::GetRootHWND()
{
	HWND hRoot = nullptr;
	if (gpConEmu->mp_Inside)
		hRoot = gpConEmu->mp_Inside->GetParentRoot();
	if (!hRoot)
		hRoot = ghWnd;
	return hRoot;
}

std::shared_ptr<MFileLogEx> CConEmuMain::GetLogger() const
{
	return mp_Log;
}

bool CConEmuMain::CreateLog()
{
	_ASSERTE(gpSet->isLogging());

	if (mp_Log)
	{
		// Logging service is already created
		return true;
	}

	bool bRc = false;
	std::lock_guard<std::mutex> lock(mcs_Log);

	mp_Log = std::make_shared<MFileLogEx>(L"ConEmu-gui", ms_ConEmuExeDir, GetCurrentProcessId());

	const HRESULT hr = mp_Log ? mp_Log->CreateLogFile(L"ConEmu-gui", GetCurrentProcessId(), gpSet->isLogging()) : E_UNEXPECTED;
	if (hr != 0)
	{
		wchar_t szError[MAX_PATH*2];
		swprintf_c(szError, L"Create log file failed! ErrCode=0x%08X\n%s\n", static_cast<DWORD>(hr), mp_Log->GetLogFileName());
		MBoxA(szError);
		mp_Log.reset();
		// Failed to start logging, don't try again
		gpSet->DisableLogging();
	}
	else
	{
		mp_Log->LogStartEnv(gpStartEnv);
		bRc = true;
	}

	return bRc;
}

bool CConEmuMain::LogString(LPCWSTR asInfo, bool abWriteTime /*= true*/, bool abWriteLine /*= true*/)
{
	if (!this || !mp_Log)
	{
		#ifdef _DEBUG
		if (asInfo && *asInfo)
		{
			DEBUGSTRNOLOG(asInfo);
		}
		#endif
		return false;
	}

	mp_Log->LogString(asInfo, abWriteTime, nullptr, abWriteLine);
	return true;
}

bool CConEmuMain::LogString(LPCSTR asInfo, bool abWriteTime /*= true*/, bool abWriteLine /*= true*/)
{
	if (!this || !mp_Log)
	{
		#ifdef _DEBUG
		if (asInfo && *asInfo)
		{
			CEStr lsDump;
			int iLen = lstrlenA(asInfo);
			MultiByteToWideChar(CP_ACP, 0, asInfo, -1, lsDump.GetBuffer(iLen), iLen+1);
			DEBUGSTRNOLOG(lsDump.ms_Val);
		}
		#endif
		return false;
	}

	mp_Log->LogString(asInfo, abWriteTime, nullptr, abWriteLine);
	return true;
}

BOOL CConEmuMain::CreateMainWindow()
{
	//if (!Init()) -- вызывается из WinMain
	//	return FALSE; // Ошибка уже показана

	if (_tcscmp(VirtualConsoleClass,VirtualConsoleClassMain)) //-V549
	{
		MBoxA(_T("Error: class names must be equal!"));
		return FALSE;
	}

	if (monitors.empty())
		ReloadMonitorInfo();

	// First ShowWindow forced to use nCmdShow. This may be weird...
	SkipOneShowWindow();

	// 2009-06-11 Возможно, что CS_SAVEBITS приводит к глюкам отрисовки
	// банально не прорисовываются некоторые части экрана (драйвер видюхи глючит?)
	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC/*|CS_SAVEBITS*/, CConEmuMain::MainWndProc, 0, 16,
	                 g_hInstance, hClassIcon, LoadCursor(nullptr, IDC_ARROW),
	                 nullptr /*(HBRUSH)COLOR_BACKGROUND*/,
	                 nullptr, gsClassNameParent, hClassIconSm
	                };// | CS_DROPSHADOW

	WNDCLASSEX wcWork = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC/*|CS_SAVEBITS*/, CConEmuMain::WorkWndProc, 0, 16,
	                 g_hInstance, hClassIcon, LoadCursor(nullptr, IDC_ARROW),
	                 nullptr /*(HBRUSH)COLOR_BACKGROUND*/,
	                 nullptr, gsClassNameWork, hClassIconSm
	                };// | CS_DROPSHADOW

	WNDCLASSEX wcBack = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC/*|CS_SAVEBITS*/, CConEmuChild::BackWndProc, 0, 16,
	                 g_hInstance, hClassIcon, LoadCursor(nullptr, IDC_ARROW),
	                 nullptr /*(HBRUSH)COLOR_BACKGROUND*/,
	                 nullptr, gsClassNameBack, hClassIconSm
	                };// | CS_DROPSHADOW

	if (!RegisterClassEx(&wc) || !RegisterClassEx(&wcWork) || !RegisterClassEx(&wcBack))
	{
		DisplayLastError(L"Failed to register class names");
		return -1;
	}

	if (mp_Inside)
	{
		if (!mp_Inside->GetParentWnd())
		{
			_ASSERTE(mp_Inside == nullptr); // Must be cleared already!
			_ASSERTE(!mp_Inside->GetInsideIntegration() || mp_Inside->GetParentWnd());
			//m_InsideIntegration = ii_None;
			SafeDelete(mp_Inside);
		}
		else
		{
			DWORD nParentTID, nParentPID;
			nParentTID = GetWindowThreadProcessId(mp_Inside->GetParentWnd(), &nParentPID);
			_ASSERTE(nParentTID && nParentPID);
			BOOL bAttach = AttachThreadInput(GetCurrentThreadId(), nParentTID, TRUE);
			if (!bAttach)
			{
				DisplayLastError(L"Inside: AttachThreadInput() failed!");
			}
		}
	}

	DWORD styleEx = GetWindowStyleEx();
	DWORD style = GetWindowStyle();
	int nWidth=CW_USEDEFAULT, nHeight=CW_USEDEFAULT;

	// WindowMode may be changed in SettingsLoaded

	// If we determine "Startup monitor" by AI or user-specified "-monitor" switch
	bool MonChanged = FixPosByStartupMonitor(gpStartEnv->hStartMon);

	// Evaluate window sized for Normal mode
	if ((this->WndWidth.Value && this->WndHeight.Value) || mp_Inside)
	{
		MBoxAssert(gpFontMgr->FontWidth() && gpFontMgr->FontHeight());
		RECT rcWnd = GetDefaultRect();
		if (gpSetCls->IsConfigNew || MonChanged)
		{
			// We get here on "clean configuration" or if we forced our window to "Current monitor"
			// Correct position to appear inside working area
			const RECT rcDefault = rcWnd;
			if (FixWindowRect(rcWnd, CEB_ALL))
			{
				// We store "visual" position, which may differ from real window placement
				this->WndPos = VisualPosFromReal(rcWnd.left, rcWnd.top);
				// Size may be set in cells, pixels or percents
				RECT rcCon = CalcRect(CER_CONSOLE_ALL, rcWnd, CER_MAIN);
				if ((rcDefault.right - rcDefault.left) > (rcWnd.right - rcWnd.left))
				{
					_ASSERTE(this->WndWidth.Style == ss_Standard);
					this->WndWidth.Set(true, ss_Standard, rcCon.right);
				}
				if ((rcDefault.bottom - rcDefault.top) > (rcWnd.bottom - rcWnd.top))
				{
					_ASSERTE(this->WndHeight.Style == ss_Standard);
					this->WndHeight.Set(false, ss_Standard, rcCon.bottom);
				}
			}
		}
		StoreNormalRect(&rcWnd);
		nWidth = rcWnd.right - rcWnd.left;
		nHeight = rcWnd.bottom - rcWnd.top;
	}
	else
	{
		_ASSERTE(this->WndWidth.Value && this->WndHeight.Value);
	}

	HWND hParent = mp_Inside ? mp_Inside->GetParentWnd() : ghWndApp;

	if (gpSet->isLogging())
	{
		wchar_t szCreate[128];
		swprintf_c(szCreate, L"Current display logical DPI=%u", gpSetCls->QueryDpi());
		LogString(szCreate);
		swprintf_c(szCreate, L"Creating main window.%s%s%s Parent=x%08X X=%i Y=%i W=%i H=%i style=x%08X exStyle=x%08X Mode=%s",
			gpSet->isQuakeStyle ? L" Quake" : L"",
			this->mp_Inside ? L" Inside" : L"",
			opt.DesktopMode ? L" Desktop" : L"",
			LODWORD(hParent),
			this->WndPos.x, this->WndPos.y, nWidth, nHeight, style, styleEx,
			GetWindowModeName(gpSet->isQuakeStyle ? static_cast<ConEmuWindowMode>(gpSet->_WindowMode) : WindowMode));
		LogString(szCreate);
	}

	// Create window intentionally small in size, we adjust it lately
	const int minWidth = 160, minHeight = 16;
	POINT ptCreate = this->RealPosFromVisual(WndPos.x, WndPos.y);
	SizeInfo::RequestDpi({gpSetCls->QueryDpi(), gpSetCls->QueryDpi()});
	SizeInfo::RequestRect({ptCreate.x, ptCreate.y, ptCreate.x + minWidth, ptCreate.y + minHeight});
	ghWnd = CreateWindowEx(styleEx, gsClassNameParent, GetCmd(), style,
	                       ptCreate.x, ptCreate.y, minWidth, minHeight, hParent, nullptr, (HINSTANCE)g_hInstance, nullptr);

	if (!ghWnd)
	{
		DWORD nErrCode = GetLastError();

		// Don't warn, if "Inside" mode was requested and parent was closed
		_ASSERTE(!mp_Inside || (hParent == mp_Inside->GetParentWnd()));

		WarnCreateWindowFail(L"main window", hParent, nErrCode);

		return FALSE;
	}

	// If created style differs from required?
	DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD dwNewStyle = FixWindowStyle(dwStyle, WindowMode);
	DEBUGTEST(WINDOWPLACEMENT wpl1 = {sizeof(wpl1)}; GetWindowPlacement(ghWnd, &wpl1););
	if (dwStyle != dwNewStyle)
	{
		//RefreshWindowStyles();
		SetWindowStyle(dwNewStyle);
	}
	DEBUGTEST(WINDOWPLACEMENT wpl2 = {sizeof(wpl2)}; GetWindowPlacement(ghWnd, &wpl2););

	setWindowPos(nullptr, ptCreate.x, ptCreate.y, nWidth, nHeight, SWP_NOMOVE);

	OnCreateFinished();

	//if (gpSet->isHideCaptionAlways)
	//	OnHideCaption();
#ifdef _DEBUG
	DWORD style2 = GetWindowLongPtr(ghWnd, GWL_STYLE);
	DWORD styleEx2 = GetWindowLongPtr(ghWnd, GWL_EXSTYLE);
#endif
	OnTransparent();
	OnGlobalSettingsChanged();
	return TRUE;
}

BOOL CConEmuMain::CreateWorkWindow()
{
	HWND  hParent = ghWnd;
	if (!SizeInfo::isCalculated())
	{
		RECT rcWnd = GetDefaultRect();
		SizeInfo::RequestRect(rcWnd);
	}
	RECT  rcClient = SizeInfo::WorkspaceRect();
	DWORD styleEx = GetWorkWindowStyleEx();
	DWORD style = GetWorkWindowStyle();

	_ASSERTE(hParent!=nullptr);

	ghWndWork = CreateWindowEx(styleEx, gsClassNameWork, L"ConEmu Workspace", style,
	                       rcClient.left, rcClient.top, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
	                       hParent, nullptr, (HINSTANCE)g_hInstance, nullptr);
	if (!ghWndWork)
	{
		DisplayLastError(L"Failed to create workspace window");
		return FALSE;
	}

	return TRUE;
}

bool CConEmuMain::SetParent(HWND hNewParent)
{
	HWND hCurrent = GetParent(ghWnd);
	if (hCurrent != hNewParent)
	{
		//::SetParent(ghWnd, nullptr);
		::SetParent(ghWnd, hNewParent);
	}
	HWND hSet = GetParent(ghWnd);
	bool lbSuccess = (hSet == hNewParent) || (hNewParent == nullptr && hSet == GetDesktopWindow());
	return lbSuccess;
}

void CConEmuMain::FillConEmuMainFont(ConEmuMainFont* pFont)
{
	// Параметры основного шрифта ConEmu
	lstrcpyn(pFont->sFontName, gpFontMgr->FontFaceName(), countof(pFont->sFontName));
	pFont->nFontHeight = gpFontMgr->FontHeight();
	pFont->nFontWidth = gpFontMgr->FontWidth();
	pFont->nFontCellWidth = gpFontMgr->FontCellWidth();
	pFont->nFontQuality = gpFontMgr->FontQuality();
	pFont->nFontCharSet = gpFontMgr->FontCharSet();
	pFont->Bold = gpFontMgr->FontBold();
	pFont->Italic = gpFontMgr->FontItalic();
	lstrcpyn(pFont->sBorderFontName, gpFontMgr->BorderFontFaceName(), countof(pFont->sBorderFontName));
	pFont->nBorderFontWidth = gpFontMgr->BorderFontWidth();
}


bool CConEmuMain::CheckDosBoxExists()
{
	bool absent = false;

	const wchar_t* reqFiles[] = { L"DosBox.exe", L"DosBox.conf" };
	for (size_t i = 0; i < countof(reqFiles); ++i)
	{
		CEStr file(ms_ConEmuBaseDir, L"\\DosBox\\", reqFiles[i]);
		if (!FileExists(file))
		{
			absent = true; break;
		}
	}

	return (mb_DosBoxExists = !absent);
}

void CConEmuMain::GetComSpecCopy(ConEmuComspec& ComSpec)
{
	_ASSERTE(m_GuiInfo.ComSpec.csType==gpSet->ComSpec.csType);
	_ASSERTE(m_GuiInfo.ComSpec.csBits==(m_DbgInfo.bBlockChildrenDebuggers ? csb_SameApp : gpSet->ComSpec.csBits));
	ComSpec = m_GuiInfo.ComSpec;
}

// Перед аттачем GUI приложения нужно создать мэппинг,
// чтобы оно могло легко узнать, куда нужно подцепляться
void CConEmuMain::CreateGuiAttachMapping(DWORD nGuiAppPID)
{
	m_GuiAttachMapping.CloseMap();
	m_GuiAttachMapping.InitName(CEGUIINFOMAPNAME, nGuiAppPID);

	if (m_GuiAttachMapping.Create())
	{
		m_GuiAttachMapping.SetFrom(&m_GuiInfo);
	}
}

// Установить пути в структуре
void CConEmuMain::InitComSpecStr(ConEmuComspec& ComSpec)
{
	wcscpy_c(ComSpec.ConEmuExeDir, ms_ConEmuExeDir);
	wcscpy_c(ComSpec.ConEmuBaseDir, ms_ConEmuBaseDir);
}

void CConEmuMain::GetGuiInfo(ConEmuGuiMapping& GuiInfo) const
{
	GuiInfo = m_GuiInfo;
}

const ConEmuGuiMapping& CConEmuMain::GetGuiInfo() const
{
	return m_GuiInfo;
}

void CConEmuMain::UpdateGuiInfoMapping()
{
	using ConEmu::ConsoleFlags;

	m_GuiInfo.nProtocolVersion = CESERVER_REQ_VER;

	static DWORD ChangeNum = 0; ChangeNum++; if (!ChangeNum) ChangeNum = 1;
	m_GuiInfo.nChangeNum = ChangeNum;

	m_GuiInfo.hGuiWnd = ghWnd;
	m_GuiInfo.nGuiPID = GetCurrentProcessId();

	m_GuiInfo.nLoggingType = CSetPgDebug::GetActivityLoggingType();
	m_GuiInfo.useInjects = (gpSet->isUseInjects ? ConEmuUseInjects::Use : ConEmuUseInjects::DontUse);
	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::UseTrueColor,(gpSet->isTrueColorer ? ConsoleFlags::UseTrueColor : ConsoleFlags::Empty));
	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::ProcessAnsi,(gpSet->isProcessAnsi ? ConsoleFlags::ProcessAnsi : ConsoleFlags::Empty));
	SetConEmuFlags(m_GuiInfo.Flags, ConsoleFlags::AnsiExecAny | ConsoleFlags::AnsiExecCmd,
		((gpSet->isAnsiExec == ansi_Allowed) ? ConsoleFlags::AnsiExecAny : (gpSet->isAnsiExec == ansi_CmdOnly) ? ConsoleFlags::AnsiExecCmd : ConsoleFlags::Empty));
	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::SuppressBells,(gpSet->isSuppressBells ? ConsoleFlags::SuppressBells : ConsoleFlags::Empty));
	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::ConExcHandler,(gpSet->isConsoleExceptionHandler ? ConsoleFlags::ConExcHandler : ConsoleFlags::Empty));
	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::ProcessNewCon,(gpSet->isProcessNewConArg ? ConsoleFlags::ProcessNewCon : ConsoleFlags::Empty));
	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::ProcessCmdStart,(gpSet->isProcessCmdStart ? ConsoleFlags::ProcessCmdStart : ConsoleFlags::Empty));
	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::ProcessCtrlZ,(gpSet->isProcessCtrlZ ? ConsoleFlags::ProcessCtrlZ : ConsoleFlags::Empty));
	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::RealConVisible,(gpSet->isConVisible ? ConsoleFlags::RealConVisible : ConsoleFlags::Empty));
	// использовать расширение командной строки (ReadConsole). 0 - нет, 1 - старая версия (0.1.1), 2 - новая версия
	switch (gpSet->isUseClink())
	{
	case 1:
		SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::UseClink_Any,ConsoleFlags::UseClink_1);
		break;
	case 2:
		SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::UseClink_Any,ConsoleFlags::UseClink_2);
		break;
	default:
		_ASSERTE(gpSet->isUseClink()==0);
		SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::UseClink_Any,ConsoleFlags::Empty);
	}

	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::BlockChildDbg,(m_DbgInfo.bBlockChildrenDebuggers ? ConsoleFlags::BlockChildDbg : ConsoleFlags::Empty));

	// m_GuiInfo.Flags[ConsoleFlags::SleepInBackground], m_GuiInfo.hActiveCons, m_GuiInfo.dwActiveTick, m_GuiInfo.bGuiActive
	UpdateGuiInfoMappingActive(isMeForeground(true, true), false);

	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::DosBox,(CheckDosBoxExists() ? ConsoleFlags::DosBox : ConsoleFlags::Empty));

	wcscpy_c(m_GuiInfo.sConEmuExe, ms_ConEmuExe);
	//-- переехали в m_GuiInfo.ComSpec
	//wcscpy_c(m_GuiInfo.sConEmuDir, ms_ConEmuExeDir);
	//wcscpy_c(m_GuiInfo.sConEmuBaseDir, ms_ConEmuBaseDir);
	lstrcpyn(m_GuiInfo.sConEmuArgs, opt.cfgSwitches.ms_Val ? opt.cfgSwitches.ms_Val : L"", countof(m_GuiInfo.sConEmuArgs));

	// AppID. It's formed of some critical parameters
	_ASSERTE(lstrcmp(ms_AppID, ms_ConEmuBuild)!=0); // must be populated properly
	lstrcpyn(m_GuiInfo.AppID, ms_AppID, countof(m_GuiInfo.AppID));

	// *********************
	// *** ComSpec begin ***
	// *********************
	{
		// Сначала - обработать что что поменяли (найти tcc и т.п.)
		SetEnvVarExpanded(L"ComSpec", ms_ComSpecInitial); // т.к. функции могут ориентироваться на окружение
		FindComspec(&gpSet->ComSpec);
		InitComSpecStr(gpSet->ComSpec);
		// установит переменную окружения ComSpec, если просили (isAddConEmu2Path)
		UpdateComspec(&gpSet->ComSpec, true/*но не трогать %PATH%*/);

		// Скопируем всю структуру сначала, потом будет "править" то что нужно
		m_GuiInfo.ComSpec = gpSet->ComSpec;

		// Теперь перенести в мэппинг, для информирования других процессов
		ComSpecType csType = gpSet->ComSpec.csType;
		// Если мы в режиме "отладки дерева процессов" - предпочитать ComSpec битности приложения!
		ComSpecBits csBits = m_DbgInfo.bBlockChildrenDebuggers ? csb_SameApp : gpSet->ComSpec.csBits;
		//m_GuiInfo.ComSpec.csType = gpSet->ComSpec.csType;
		//m_GuiInfo.ComSpec.csBits = gpSet->ComSpec.csBits;
		CEStr szExpand, szFull;
		if (csType == cst_Explicit)
		{
			// Expand env.vars
			if (wcschr(gpSet->ComSpec.ComspecExplicit, L'%'))
			{
				szExpand = ExpandEnvStr(gpSet->ComSpec.ComspecExplicit);
				if (szExpand.IsEmpty())
				{
					_ASSERTE(FALSE && "ExpandEnvStr(ComSpec)");
					szExpand.Clear();
				}
			}
			else
			{
				szExpand.Set(gpSet->ComSpec.ComspecExplicit);
			}
			// Expand relative paths. Note. Path MUST be specified with root
			// NOT recommended: "..\tcc\tcc.exe" this may lead to unpredictable results cause of CurrentDirectory
			// Allowed: "%ConEmuDir%\..\tcc\tcc.exe"
			if (!szExpand.IsEmpty())
			{
				if (!apiGetFullPathName(szExpand, szFull))
				{
					_ASSERTE(FALSE && "apiGetFullPathName(ComSpec)");
					szFull.Clear();
				}
				else if (!FileExists(szFull))
				{
					szFull.Clear();
				}
			}
			else
			{
				szFull.Clear();
			}
			// Validate and save in mapping
			if (!szFull.IsEmpty())
			{
				wcscpy_c(m_GuiInfo.ComSpec.ComspecExplicit, szFull);
			}
			else
			{
				_ASSERTE(csType == cst_Explicit);
				csType = cst_AutoTccCmd;
				m_GuiInfo.ComSpec.ComspecExplicit[0] = 0;
			}
		}
		else
		{
			m_GuiInfo.ComSpec.ComspecExplicit[0] = 0; // clean from possible rubbish
		}

		//
		if (csType == cst_Explicit)
		{
			_ASSERTE(*szFull);
			wcscpy_c(m_GuiInfo.ComSpec.Comspec32, szFull);
			wcscpy_c(m_GuiInfo.ComSpec.Comspec64, szFull);
		}
		else
		{
			wcscpy_c(m_GuiInfo.ComSpec.Comspec32, gpSet->ComSpec.Comspec32);
			wcscpy_c(m_GuiInfo.ComSpec.Comspec64, gpSet->ComSpec.Comspec64);
		}

		// finalization
		m_GuiInfo.ComSpec.csType = csType;
		m_GuiInfo.ComSpec.csBits = csBits;
		//m_GuiInfo.ComSpec.isUpdateEnv = gpSet->ComSpec.isUpdateEnv;
		//m_GuiInfo.ComSpec.isAddConEmu2Path = gpSet->ComSpec.isAddConEmu2Path;
	}
	// *******************
	// *** ComSpec end ***
	// *******************


	FillConEmuMainFont(&m_GuiInfo.MainFont);

	TODO("DosBox");
	//BOOL     bDosBox; // наличие DosBox
	//wchar_t  sDosBoxExe[MAX_PATH+1]; // полный путь к DosBox.exe
	//wchar_t  sDosBoxEnv[8192]; // команды загрузки (mount, и пр.)

	//if (mb_CreateProcessLogged)
	//	lstrcpy(m_GuiInfo.sLogCreateProcess, ms_LogCreateProcess);
	// sConEmuArgs уже заполнен в PrepareCommandLine

	BOOL lbMapIsValid = m_GuiInfoMapping.IsValid();

	if (!lbMapIsValid)
	{
		m_GuiInfoMapping.InitName(CEGUIINFOMAPNAME, GetCurrentProcessId());

		if (m_GuiInfoMapping.Create())
			lbMapIsValid = TRUE;
	}

	if (lbMapIsValid)
	{
		m_GuiInfoMapping.SetFrom(&m_GuiInfo);
	}
	#ifdef _DEBUG
	else
	{
		_ASSERTE(lbMapIsValid);
	}
	#endif

	if (mp_DefTrm)
	{
		mp_DefTrm->UpdateDefTermMapping();
	}
}

void CConEmuMain::UpdateGuiInfoMappingActive(bool bActive, bool bUpdatePtr /*= true*/)
{
	using ConEmu::ConsoleFlags;

	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::SleepInBackground,(gpSet->isSleepInBackground ? ConsoleFlags::SleepInBackground : ConsoleFlags::Empty));
	SetConEmuFlags(m_GuiInfo.Flags,ConsoleFlags::RetardInactivePanes,(gpSet->isRetardInactivePanes ? ConsoleFlags::RetardInactivePanes : ConsoleFlags::Empty));

	m_GuiInfo.bGuiActive = bActive;


	// *** ConEmuGuiMapping::Consoles - begin
	{
		struct FillConsoles
		{
			ConEmuGuiMapping* pGuiInfo;
			int nCount;

			static bool Fill(CVirtualConsole* pVCon, LPARAM lParam)
			{
				FillConsoles* p = (FillConsoles*)lParam;
				ConEmuConsoleInfo& ci = p->pGuiInfo->Consoles[p->nCount++];
				CRealConsole* pRCon = pVCon->RCon();
				ci.Console = pRCon->ConWnd();
				ci.DCWindow = pVCon->GetView();
				ci.BackWindow = pVCon->GetBack();
				ci.ChildGui = pVCon->GuiWnd();
				ci.Flags = (pVCon->isActive(false) ? ccf_Active : ccf_None)
					| (pVCon->isVisible() ? ccf_Visible : ccf_None)
					| (ci.ChildGui ? ccf_ChildGui : ccf_None);
				ci.ServerPID = pRCon->GetServerPID(true);
				return true;
			}
		} fill = { &m_GuiInfo, 0};

		CVConGroup::EnumVCon(evf_All, FillConsoles::Fill, reinterpret_cast<LPARAM>(&fill));

		if (fill.nCount < countof(m_GuiInfo.Consoles))
			memset(m_GuiInfo.Consoles + fill.nCount, 0, sizeof(m_GuiInfo.Consoles[0])*(countof(m_GuiInfo.Consoles) - fill.nCount));
	}
	// *** ConEmuGuiMapping::Consoles - end


	// Update finished
	m_GuiInfo.dwActiveTick = GetTickCount();

	if (!bUpdatePtr)
		return;

	ConEmuGuiMapping* pData = m_GuiInfoMapping.Ptr();

	if (pData)
	{
		if ((pData->Flags != m_GuiInfo.Flags)
			|| ((pData->bGuiActive!=FALSE) != (bActive!=FALSE))
			|| (memcmp(pData->Consoles, m_GuiInfo.Consoles, sizeof(m_GuiInfo.Consoles)) != 0))
		{
			pData->Flags = m_GuiInfo.Flags;
			pData->bGuiActive = bActive;
			memmove(pData->Consoles, m_GuiInfo.Consoles, sizeof(m_GuiInfo.Consoles));
			pData->dwActiveTick = m_GuiInfo.dwActiveTick;
		}
	}
}

void CConEmuMain::Destroy(bool abForce)
{
	LogString(L"CConEmuMain::Destroy()");

	// Warning! if (abForce) - we can't call MainThread,
	// there is nobody to process messages (parent was killed)
	if (abForce)
	{
		//TODO: Close all consoles, who've finished their processes
		//TODO: EmergencyShow all consoles, who've running processes
	}

	DeinitOnDestroy(ghWnd, abForce);

	if (gbInDisplayLastError)
	{
		LogString(L"-- Destroy skipped due to gbInDisplayLastError");
		return; // Чтобы не схлопывались окна с сообщениями об ошибках
	}

	#ifdef _DEBUG
	if (gnInMyAssertTrap > 0)
	{
		LogString(L"-- Destroy skipped due to gnInMyAssertTrap");
		mb_DestroySkippedInAssert = true;
		return;
	}
	#endif

	if (abForce)
	{
		// Expected only in Inside mode if parent was abnormally terminated
		_ASSERTE(mp_Inside!=nullptr);
		int iBtn;
		// Don't warn if we have nothing to do
		if (isInsideInvalid() && !isVConExists(0))
			iBtn = IDOK;
		else
			iBtn = MsgBox(L"ConEmu's parent window was terminated abnormally.\n"
				L"Continue to kill ConEmu process?",
				MB_OKCANCEL|MB_ICONEXCLAMATION, GetDefaultTitle(), nullptr, false);
		if (iBtn == IDOK)
		{
			ExitProcess(1);
		}
	}
	else if (ghWnd)
	{
		//HWND hWnd = ghWnd;
		//ghWnd = nullptr;
		//DestroyWindow(hWnd); -- может быть вызвано из другой нити
		PostMessage(ghWnd, mn_MsgMyDestroy, GetCurrentThreadId(), 0);
	}
	else
	{
		LogString(L"-- Destroy skipped due to ghWnd");
	}
}

CConEmuMain::~CConEmuMain()
{
	_ASSERTE(ghWnd==nullptr || !IsWindow(ghWnd));
	MCHKHEAP;
	//ghWnd = nullptr;

	// Ensure "RCon starting queue" is terminated
	if (mp_RunQueue)
		mp_RunQueue->Terminate();

	SafeDelete(mp_DefTrm);

	bool bNeedConHostSearch = (gnOsVer == 0x0601);
	if (bNeedConHostSearch)
	{
		SafeDelete(m_LockConhostStart.pcs);
		SafeDelete(m_LockConhostStart.pcsLock);
	}

	SafeDelete(mp_AttachDlg);
	SafeDelete(mp_RecreateDlg);
	SafeCloseHandle(mh_ConEmuAliveEvent);
	SafeCloseHandle(mh_ConEmuAliveEventNoDir);

	CVConGroup::DestroyAllVCon();

	CVirtualConsole::ClearPartBrushes();

	#ifndef _WIN64
	if (mh_WinHook)
	{
		UnhookWinEvent(mh_WinHook);
		mh_WinHook = nullptr;
	}
	#endif

	//if (mh_PopupHook) {
	//	UnhookWinEvent(mh_PopupHook);
	//	mh_PopupHook = nullptr;
	//}

	SafeDelete(mp_DragDrop);

	//if (ProgressBars) {
	//    delete ProgressBars;
	//    ProgressBars = nullptr;
	//}

	SafeDelete(mp_TabBar);

	SafeDelete(mp_Tip);

	SafeDelete(mp_Status);

	SafeDelete(mp_Menu);

	SafeDelete(mp_Inside);

	SafeDelete(mp_RunQueue);

	SafeDelete(mp_Find);

	SafeDelete(mp_PushInfo);

	SafeDelete(mp_AltNumpad);

	SafeDelete(mp_HandleMonitor);

	//if (m_Child)
	//{
	//	delete m_Child;
	//	m_Child = nullptr;
	//}

	//if (m_Back)
	//{
	//	delete m_Back;
	//	m_Back = nullptr;
	//}

	//if (m_Macro)
	//{
	//	delete m_Macro;
	//	m_Macro = nullptr;
	//}

	//if (mh_RecreatePasswFont)
	//{
	//	DeleteObject(mh_RecreatePasswFont);
	//	mh_RecreatePasswFont = nullptr;
	//}

	m_GuiServer.Stop(true);

	CVConGroup::Deinitialize();

	//Delete Critical Section(&mcs_ShellExecuteEx);

	LoadImageFinalize();

	if (wasTerminateThreadCalled())
	{
		LogString(L"WARNING!!! TerminateThread was called, process would be terminated forcedly");
	}

	SafeDelete(m_SshAgentsLock);

	mp_Log.reset();

	if (m_Hotkeys) m_Hotkeys->OnTerminate();

	CommonShutdown();

	gpConEmu = nullptr;
}




/* ****************************************************** */
/*                                                        */
/*                  Sizing methods                        */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	Sizing_Methods() {};
}
#endif

void CConEmuMain::AskChangeTermMode()
{
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) < 0)
		return;
	CVirtualConsole *pVCon = VCon.VCon();
	CRealConsole *pRCon = pVCon->RCon();
	if (!pRCon) return;

    pRCon ->StartStopTermMode(tmc_TerminalType, cta_Switch);
}

void CConEmuMain::AskChangeBufferHeight()
{
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) < 0)
		return;
	CVirtualConsole *pVCon = VCon.VCon();
	CRealConsole *pRCon = pVCon->RCon();
	if (!pRCon) return;


	HWND hGuiClient = pRCon->GuiWnd();
	if (hGuiClient)
	{
		pRCon->ShowGuiClientInt(pRCon->isGuiForceConView());
		return;
	}


	// Win7 BUGBUG: Issue 192: падение Conhost при turn bufferheight ON
	// http://code.google.com/p/conemu-maximus5/issues/detail?id=192
	if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1)
		return;


	BOOL lbBufferHeight = pRCon->isBufferHeight();

	int nBtn = MsgBox(lbBufferHeight ?
					  L"Do You want to turn bufferheight OFF?" :
					  L"Do You want to turn bufferheight ON?",
					  MB_ICONQUESTION|MB_OKCANCEL,
					  GetDefaultTitle());

	if (nBtn != IDOK) return;

	#ifdef _DEBUG
	HANDLE hFarInExecuteEvent = nullptr;

	if (!lbBufferHeight)
	{
		DWORD dwFarPID = pRCon->GetFarPID();

		if (dwFarPID)
		{
			// Это событие дергается в отладочной (мной поправленной) версии фара
			wchar_t szEvtName[64]; swprintf_c(szEvtName, L"FARconEXEC:%08X", LODWORD(pRCon->ConWnd()));
			hFarInExecuteEvent = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEvtName);

			if (hFarInExecuteEvent)  // Иначе ConEmuC вызовет _ASSERTE в отладочном режиме
				SetEvent(hFarInExecuteEvent);
		}
	}
	#endif


	pRCon->ChangeBufferHeightMode(!lbBufferHeight);


	#ifdef _DEBUG
	if (hFarInExecuteEvent)
		ResetEvent(hFarInExecuteEvent);
	#endif

	// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
	pRCon->OnBufferHeight();
}

void CConEmuMain::AskChangeAlternative()
{
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) < 0)
		return;
	CVirtualConsole *pVCon = VCon.VCon();
	CRealConsole *pRCon = pVCon->RCon();
	if (!pRCon) return;


	HWND hGuiClient = pRCon->GuiWnd();
	if (hGuiClient)
	{
		pRCon->ShowGuiClientInt(!pRCon->isGuiVisible());
		return;
	}


	// Переключиться на альтернативный/основной буфер
	RealBufferType CurBuffer = pRCon->GetActiveBufferType();
	if (CurBuffer == rbt_Primary)
	{
		pRCon->SetActiveBuffer(rbt_Alternative);
	}
	else
	{
		pRCon->SetActiveBuffer(rbt_Primary);
	}

	// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
	pRCon->OnBufferHeight();
}

void CConEmuMain::SyncNtvdm()
{
	OnSize();
}

// Don't change RealConsole height on tabs auto-show/hide,
// resize ConEmu window height instead (Issue 1977, gh#373)
void CConEmuMain::OnTabbarActivated(bool bTabbarVisible, bool bInAutoShowHide)
{
	// If creation was not finished - nothing to do
	if (InCreateWindow())
	{
		return;
	}

	RequestRecalc();

	int iNewWidth, iNewHeight;

	if (bInAutoShowHide && isWindowNormal())
	{
		if (!isIconic())
		{
			RECT rcCur = CalcRect(CER_MAIN, nullptr);
			MONITORINFO mi = {};
			GetNearestMonitorInfo(&mi, nullptr, &rcCur);

			SIZE szCon = GetDefaultSize(true);
			_ASSERTE(szCon.cy!=26);
			RECT rcCon = MakeRect(szCon.cx, szCon.cy);
			ConEmuMargins tMainTabAct = bTabbarVisible ? CEM_TABACTIVATE : CEM_TABDEACTIVATE;
			RECT rcWnd = CalcRect(CER_MAIN, rcCon, CER_CONSOLE_ALL, nullptr, tMainTabAct);

			iNewWidth = rcWnd.right-rcWnd.left; iNewHeight = rcWnd.bottom-rcWnd.top;

			if ((iNewWidth != (rcCur.right - rcCur.left)) || (iNewHeight != (rcCur.bottom - rcCur.top)))
			{
				// #SIZE_TODO When toggle "Show toolbar" and show the TabBar - we get assertion coz band width is changing
				_ASSERTE(iNewWidth == (rcCur.right - rcCur.left)); // Width change is not expected

				// If the window become LARGER than possible (going out of working area)
				// we must decrease console height unfortunately...
				if (bTabbarVisible && ((iNewHeight + rcCur.top) > mi.rcWork.bottom))
				{
					int iAllowedShift = (rcCur.bottom > mi.rcWork.bottom) ? (rcCur.bottom - mi.rcWork.bottom) : 0;
					RECT rcWnd3 = {rcCur.left, rcCur.top, rcCur.right, (mi.rcWork.bottom + iAllowedShift)};
					// Than we shall correct the integral size
					RECT rcCon2 = CalcRect(CER_CONSOLE_ALL, rcWnd3, CER_MAIN, nullptr, bTabbarVisible ? CEM_TABACTIVATE : CEM_TABDEACTIVATE);
					RECT rcWnd2 = CalcRect(CER_MAIN, rcCon2, CER_CONSOLE_ALL, nullptr, bTabbarVisible ? CEM_TABACTIVATE : CEM_TABDEACTIVATE);
					// Final fix
					iNewHeight = (rcWnd2.bottom-rcWnd2.top);
					// Have to update normal rect
					rcWnd3.bottom = rcWnd3.top + iNewHeight;
					StoreNormalRect(&rcWnd3);
				}

				// Now, change the size of our window, but restrict responding
				// to size changes, until window resize finishes
				if (iNewHeight != (rcCur.bottom - rcCur.top))
				{
					MSetter lSet(&mn_IgnoreSizeChange);
					setWindowPos(nullptr, 0, 0, iNewWidth, iNewHeight, SWP_NOZORDER|SWP_NOMOVE);
				}

				// Now we may correct our children layout
				OnSize();
			}
		}
	}
	else
	{
		// Fullscreen or Inside, for example, we can't change window height, so
		// do console resize to match new (lesser or larger) client area size
		CVConGroup::SyncConsoleToWindow();
	}

	StoreNormalRect(nullptr);
}

void CConEmuMain::ForceShowTabs(BOOL abShow)
{
	//if (!mp_ VActive)
	//  return;
	//2009-05-20 Раз это Force - значит на возможность получить табы из фара забиваем! Для консоли показывается "Console"
	BOOL lbTabsAllowed = abShow /*&& this->mp_TabBar->IsAllowed()*/;

	if (abShow && gpSet->isTabs && lbTabsAllowed)
	{
		this->mp_TabBar->Activate(TRUE);
		this->mp_TabBar->Update();
	}
	else if (!abShow)
	{
		this->mp_TabBar->Deactivate(TRUE);
	}

	// Иначе Gaps не отрисуются
	this->InvalidateAll();
	UpdateWindowRgn();
	// При отключенных табах нужно показать "[n/n] " а при выключенных - спрятать
	UpdateTitle(); // сам перечитает
}

// Должна вернуть TRUE, если события мыши не нужно пропускать в консоль
BOOL CConEmuMain::TrackMouse()
{
	BOOL lbCapture = FALSE; // По умолчанию - мышь не перехватывать

	TODO("DoubleView: переделать на обработку в видимых консолях");
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
	{
		if (VCon->TrackMouse())
			lbCapture = TRUE;
	}

	return lbCapture;
}

void CConEmuMain::OnAlwaysShowScrollbar(bool abSync /*= true*/)
{
	CVConGroup::OnAlwaysShowScrollbar(abSync);
}

LRESULT CConEmuMain::OnQueryEndSession(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	wchar_t szSession[128];
	swprintf_c(szSession, L"Session End:%s%s%s\r\n",
		(lParam & ENDSESSION_CLOSEAPP) ? L" ENDSESSION_CLOSEAPP" : L"",
		(lParam & ENDSESSION_CRITICAL) ? L" ENDSESSION_CRITICAL" : L"",
		(lParam & ENDSESSION_LOGOFF)   ? L" ENDSESSION_LOGOFF" : L"");
	LogString(szSession, true, false);
	DEBUGSTRSESS(szSession);

	return ::DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CConEmuMain::SessionInfo::Log(WPARAM State, LPARAM SessionID)
{
	LONG i = _InterlockedIncrement(&g_evtidx);
	EvtLog evt = {GetTickCount(), (DWORD)State, SessionID};
	// Write a message at this index
	g_evt[i & (SESSION_LOG_SIZE - 1)] = evt;
}

bool CConEmuMain::SessionInfo::Connected()
{
	return (wState!=7/*WTS_SESSION_LOCK*/);
}

// Called from: WM_WTSSESSION_CHANGE -> CConEmuMain::OnSessionChanged
void CConEmuMain::SessionInfo::SessionChanged(WPARAM State, LPARAM SessionID)
{
	wState = State;
	lSessionID = SessionID;
	Log(State, SessionID);
}

void CConEmuMain::SessionInfo::SetSessionNotification(bool bSwitch)
{
	if (((hWtsApi!=nullptr) == bSwitch) || !IsWindowsXP)
		return;

	if (bSwitch)
	{
		wState = (WPARAM)-1;
		lSessionID = (LPARAM)-1;

		hWtsApi = LoadLibrary(L"Wtsapi32.dll");

		pfnRegister = hWtsApi ? (WTSRegisterSessionNotification_t)GetProcAddress(hWtsApi, "WTSRegisterSessionNotification") : nullptr;
		pfnUnregister = hWtsApi ? (WTSUnRegisterSessionNotification_t)GetProcAddress(hWtsApi, "WTSUnRegisterSessionNotification") : nullptr;

		// May return RPC_S_INVALID_BINDING error code
		// if "Global\\TermSrvReadyEvent" event was not set
		if (!pfnRegister || !pfnUnregister || !pfnRegister(ghWnd, 0/*NOTIFY_FOR_THIS_SESSION*/))
		{
			if (hWtsApi)
			{
				FreeLibrary(hWtsApi);
				hWtsApi = nullptr;
			}
			return;
		}
	}
	else if (hWtsApi)
	{
		if (pfnUnregister && ghWnd)
		{
			pfnUnregister(ghWnd);
			// Once
			pfnUnregister = nullptr;
		}

		// FreeLibrary(hWtsApi);
		// hWtsApi = nullptr;
	}
}

// WM_WTSSESSION_CHANGE
LRESULT CConEmuMain::OnSessionChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	DEBUGTEST(bool bPrevConnected = session.Connected());

	session.SessionChanged(wParam, lParam);

	wchar_t szInfo[128], szState[32];
	WORD nSessionCode = LOWORD(wParam);
	switch (nSessionCode)
	{
		//0x1 The session identified by lParam was connected to the console terminal or RemoteFX session.
		case WTS_CONSOLE_CONNECT: wcscpy_c(szState, L"WTS_CONSOLE_CONNECT"); break;
		//0x2 The session identified by lParam was disconnected from the console terminal or RemoteFX session.
		case WTS_CONSOLE_DISCONNECT: wcscpy_c(szState, L"WTS_CONSOLE_DISCONNECT"); break;
		//0x3 The session identified by lParam was connected to the remote terminal.
		case WTS_REMOTE_CONNECT: wcscpy_c(szState, L"WTS_REMOTE_CONNECT"); break;
		//0x4 The session identified by lParam was disconnected from the remote terminal.
		case WTS_REMOTE_DISCONNECT: wcscpy_c(szState, L"WTS_REMOTE_DISCONNECT"); break;
		//0x5 A user has logged on to the session identified by lParam.
		case WTS_SESSION_LOGON: wcscpy_c(szState, L"WTS_SESSION_LOGON"); break;
		//0x6 A user has logged off the session identified by lParam.
		case WTS_SESSION_LOGOFF: wcscpy_c(szState, L"WTS_SESSION_LOGOFF"); break;
		//0x7 The session identified by lParam has been locked.
		case WTS_SESSION_LOCK: wcscpy_c(szState, L"WTS_SESSION_LOCK"); break;
		//0x8 The session identified by lParam has been unlocked.
		case WTS_SESSION_UNLOCK: wcscpy_c(szState, L"WTS_SESSION_UNLOCK"); break;
		//0x9 The session identified by lParam has changed its remote controlled status. To determine the status, call GetSystemMetrics and check the SM_REMOTECONTROL metric.
		case WTS_SESSION_REMOTE_CONTROL: wcscpy_c(szState, L"WTS_SESSION_REMOTE_CONTROL"); break;
		//0xA Reserved for future use.
		case 0xA/*WTS_SESSION_CREATE*/: wcscpy_c(szState, L"WTS_SESSION_CREATE"); break;
		//0xB Reserved for future use.
		case 0xB/*WTS_SESSION_TERMINATE*/: wcscpy_c(szState, L"WTS_SESSION_TERMINATE"); break;
		default: swprintf_c(szState, WIN3264TEST(L"x%08X",L"x%0X%08X"), WIN3264WSPRINT(wParam));
	}
	swprintf_c(szInfo, L"Session State (#%i): %s\r\n", (int)lParam, szState);
	LogString(szInfo, true, false);
	DEBUGSTRSESS(szInfo);

	if ((nSessionCode == WTS_SESSION_LOCK)
		|| (nSessionCode == WTS_SESSION_UNLOCK)
		)
	{
		// Ignore window size changes until station will be unlocked
		if (nSessionCode == WTS_SESSION_LOCK)
		{
			#ifdef _DEBUG // due to unittests
			_ASSERTE(bPrevConnected && (mn_IgnoreSizeChange >= 0));
			#endif

			if (!session.bWasLocked)
			{
				session.bWasLocked = TRUE;
				InterlockedIncrement(&mn_IgnoreSizeChange);
			}
		}
		else if (nSessionCode == WTS_SESSION_UNLOCK)
		{
			#ifdef _DEBUG // due to unittests
			_ASSERTE(!bPrevConnected && (mn_IgnoreSizeChange > 0));
			#endif
			if (session.bWasLocked && (mn_IgnoreSizeChange > 0))
			{
				session.bWasLocked = FALSE;
				InterlockedDecrement(&mn_IgnoreSizeChange);
			}
		}

		// Stop all servers from reading console contents
		struct impl {
			static bool DoLockUnlock(CVirtualConsole* pVCon, LPARAM lParam)
			{
				pVCon->RCon()->DoLockUnlock(lParam == WTS_SESSION_LOCK);
				return true; // continue;
			};
		};
		CVConGroup::EnumVCon(evf_All, impl::DoLockUnlock, static_cast<LPARAM>(nSessionCode));

		// Refresh child windows
		OnSize();
	}

	return 0; // Return value ignored
}








/* ****************************************************** */
/*                                                        */
/*                  System Routines                       */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	System_Routines() {};
}
#endif

void CConEmuMain::OnActiveConWndStore(HWND hConWnd)
{
	// Для удобства внешних программ, плагинов и прочая
	// Запоминаем в UserData дескриптор "активного" окна консоли
	SetWindowLongPtr(ghWnd, GWLP_USERDATA, (LONG_PTR)hConWnd);

	// ghWndWork - планируется как Holder для виртуальных консолей,
	// причем он может быть как дочерним для ghWnd, так и отдельно
	// висящим, и делающим вид, что оно дочернее для ghWnd.
	if (ghWndWork && (ghWndWork != ghWnd))
	{
		SetWindowLongPtr(ghWndWork, GWLP_USERDATA, (LONG_PTR)hConWnd);
	}

	UpdateGuiInfoMappingActive(isMeForeground(true,true));
}

BOOL CConEmuMain::Activate(CVirtualConsole* apVCon)
{
	BOOL lbRc;
	if (!isMainThread())
	{
		LRESULT lRc;
		if (apVCon->GuiWnd())
		{
			// We can lock, if Activate VCon is called from ChildGui.
			// That's why - PostMessage instead of SendMessage
			PostMessage(ghWnd, mn_MsgActivateVCon, 0, reinterpret_cast<LPARAM>(apVCon));
			lRc = 0;
		}
		else
		{
			lRc = SendMessage(ghWnd, mn_MsgActivateVCon, 0, reinterpret_cast<LPARAM>(apVCon));
		}
		lbRc = (lRc == (LRESULT)apVCon);
	}
	else
	{
		lbRc = CVConGroup::Activate(apVCon);
	}
	return lbRc;
}

void CConEmuMain::MoveActiveTab(CVirtualConsole* apVCon, bool bLeftward)
{
	CVConGroup::MoveActiveTab(apVCon, bLeftward);
}

// 0 - based
int CConEmuMain::ActiveConNum()
{
	return CVConGroup::ActiveConNum();
}

int CConEmuMain::GetConCount()
{
	return CVConGroup::GetConCount();
}

void CConEmuMain::AttachToDialog()
{
	if (!mp_AttachDlg)
		mp_AttachDlg = new CAttachDlg;
	mp_AttachDlg->AttachDlg();
}

// Вернуть общее количество процессов по всем консолям
DWORD CConEmuMain::CheckProcesses()
{
	return CVConGroup::CheckProcesses();
}

bool CConEmuMain::ConActivateNext(bool abNext)
{
	return CVConGroup::ConActivateNext(abNext);
}

// nCon - zero-based index of console; -1 for last console
bool CConEmuMain::ConActivate(int nCon)
{
	return CVConGroup::ConActivate(nCon);
}

// asName - renamed title, console title, active process name, root process name
bool CConEmuMain::ConActivateByName(LPCWSTR asName)
{
	return CVConGroup::ConActivateByName(asName);
}

bool CConEmuMain::CreateWnd(RConStartArgsEx& args)
{
	if (!args.pszSpecialCmd || !*args.pszSpecialCmd)
	{
		_ASSERTE(args.pszSpecialCmd && *args.pszSpecialCmd);
		return false;
	}

	_ASSERTE(args.aRecreate == cra_CreateWindow);

	BOOL bStart = FALSE;

	CEStr szAddArgs;
	LPCWSTR pszAddArgs = MakeConEmuStartArgs(szAddArgs);

	// Start new ConEmu.exe process with chosen arguments...
	STARTUPINFO si = {};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};
	wchar_t* pszCmdLine = nullptr;
	size_t cchMaxLen = _tcslen(ms_ConEmuExe)
		+ _tcslen(args.pszSpecialCmd)
		+ (pszAddArgs ? (_tcslen(pszAddArgs) + 2) : 0)
		+ (args.pszAddGuiArg ? _tcslen(args.pszAddGuiArg) : 0)
		+ 170; // other flags and `-new_console`
	if ((pszCmdLine = (wchar_t*)malloc(cchMaxLen*sizeof(*pszCmdLine))) == nullptr)
	{
		_ASSERTE(pszCmdLine);
	}
	else
	{
		pszCmdLine[0] = L'"'; pszCmdLine[1] = 0;
		_wcscat_c(pszCmdLine, cchMaxLen, ms_ConEmuExe);
		_wcscat_c(pszCmdLine, cchMaxLen, L"\" ");

		// -config, -loadcfgfile, -fontdir, -basic and others
		if (pszAddArgs)
		{
			_wcscat_c(pszCmdLine, cchMaxLen, pszAddArgs);
			_wcscat_c(pszCmdLine, cchMaxLen, L" ");
		}

		// Avoid reusing existing window
		_wcscat_c(pszCmdLine, cchMaxLen, L"-NoSingle ");
		if (gpSet->isQuakeStyle)
			_wcscat_c(pszCmdLine, cchMaxLen, L"-NoQuake ");

		// Some arguments may be defined in the RConStartArgsEx
		if (args.pszAddGuiArg)
			_wcscat_c(pszCmdLine, cchMaxLen, args.pszAddGuiArg);

		// The starting command
		_wcscat_c(pszCmdLine, cchMaxLen, L"-run ");
		_wcscat_c(pszCmdLine, cchMaxLen, args.pszSpecialCmd);
		if ((args.RunAsAdministrator == crb_On) || (args.RunAsRestricted == crb_On) || args.pszUserName)
		{
			if (args.RunAsAdministrator == crb_On)
			{
				// Create ConEmu.exe with current credentials, implying elevation for the console
				_wcscat_c(pszCmdLine, cchMaxLen, L" -new_console:a");
			}
			else if (args.RunAsRestricted == crb_On)
			{
				_wcscat_c(pszCmdLine, cchMaxLen, L" -new_console:r");
			}
		}

		if ((args.RunAsAdministrator != crb_On) && (args.RunAsRestricted != crb_On) && (args.pszUserName && *args.pszUserName))
		{
			DWORD nFlags = (args.RunAsNetOnly == crb_On) ? LOGON_NETCREDENTIALS_ONLY : LOGON_WITH_PROFILE;
			bStart = CreateProcessWithLogonW(args.pszUserName, args.pszDomain, args.szUserPassword,
		                           nFlags, nullptr, pszCmdLine,
		                           NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE
		                           , nullptr, args.pszStartupDir, &si, &pi);
		}
		else
			bStart = CreateProcess(nullptr, pszCmdLine, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, args.pszStartupDir, &si, &pi);

		if (!bStart)
		{
			DWORD nErr = GetLastError();
			DisplayLastError(pszCmdLine, nErr, MB_ICONSTOP, L"Failed to start new ConEmu window");
		}
		else
		{
			SafeCloseHandle(pi.hProcess);
			SafeCloseHandle(pi.hThread);
		}
	}

	SafeFree(pszCmdLine);
	return (bStart != FALSE);
}

// Also, called from mn_MsgCreateCon
CVirtualConsole* CConEmuMain::CreateCon(RConStartArgsEx& args, bool abAllowScripts /*= false*/, bool abForceCurConsole /*= false*/)
{
	if (!isMainThread())
	{
		// Создание VCon в фоновых потоках не допускается, т.к. здесь создаются HWND
		MBoxAssert(isMainThread());
		return nullptr;
	}

	CVirtualConsole* pVCon = CVConGroup::CreateCon(args, abAllowScripts, abForceCurConsole);

	return pVCon;
}

// args должен быть выделен через "new"
// по завершении - на него будет вызван "delete"
void CConEmuMain::PostCreateCon(const RConStartArgsEx& args)
{
	RConStartArgsEx* pArgs = new RConStartArgsEx;
	if (!pArgs)
	{
		_ASSERTE(pArgs != nullptr);
		return;
	}
	pArgs->AssignFrom(args);

	_ASSERTE((pArgs->pszStartupDir == nullptr) || (*pArgs->pszStartupDir != 0));

	struct impl {
		CConEmuMain* pObj;
		RConStartArgsEx *pArgs;

		static LRESULT CreateConMainThread(LPARAM lParam)
		{
			impl *p = (impl*)lParam;
			CVirtualConsole* pVCon = p->pObj->CreateCon(*p->pArgs, true);
			delete p->pArgs;
			delete p;
			return (LRESULT)pVCon;
		};
	} *Impl = new impl;
	Impl->pObj = this;
	Impl->pArgs = pArgs;

	// Execute asynchronously
	CallMainThread(false, impl::CreateConMainThread, reinterpret_cast<LPARAM>(Impl));
}

LPCWSTR CConEmuMain::ParseScriptLineOptions(LPCWSTR apszLine, bool* rpbSetActive, RConStartArgsEx* pArgs)
{
	if (!apszLine)
	{
		_ASSERTE(apszLine!=nullptr);
		return nullptr;
	}

	// !!! Don't Reset values (rpbSetActive, rpbAsAdmin, pArgs)
	// !!! They may be defined in the other place

	apszLine = SkipNonPrintable(apszLine);

	// Go
	while (*apszLine == L'>' || *apszLine == L'*' /*|| *apszLine == L'?'*/ || *apszLine == L' ' || *apszLine == L'\t')
	{
		if (*apszLine == L'>')
		{
			if (rpbSetActive)
				*rpbSetActive = true;
			if (pArgs && (pArgs->ForegroundTab == crb_Undefined) && (pArgs->BackgroundTab == crb_Undefined))
				pArgs->ForegroundTab = crb_On;
		}

		if (*apszLine == L'*')
		{
			if (pArgs && (pArgs->RunAsAdministrator == crb_Undefined))
				pArgs->RunAsAdministrator = crb_On;
		}

		apszLine++;
	}

	// don't reset one that may come from apDefArgs
	if (pArgs && (pArgs->RunAsAdministrator == crb_Undefined))
		pArgs->RunAsAdministrator = mb_IsUacAdmin ? crb_On : crb_Off;

	// Process some more specials
	if (apszLine && *apszLine)
	{
		LPCWSTR pcszCmd = apszLine;
		CmdArg szArg;
		const int iNewConLen = lstrlen(L"-new_console");
		while ((pcszCmd = NextArg(pcszCmd, szArg)))
		{
			// On first "-new_console" or "-cur_console" stop processing "specials"
			if (wcsncmp(szArg, L"-new_console", iNewConLen) == 0 || wcsncmp(szArg, L"-cur_console", iNewConLen) == 0)
				break;

			// Only /-syntax is allowed
			if (szArg.ms_Val[0] != L'/')
				break;

			if (lstrcmpi(szArg, L"/bufferheight") == 0)
			{
				if ((pcszCmd = NextArg(pcszCmd, szArg)))
				{
					wchar_t* pszEnd = nullptr;
					if (pArgs)
					{
						pArgs->nBufHeight = wcstol(szArg, &pszEnd, 10);
						pArgs->BufHeight = crb_On;
					}
					apszLine = pcszCmd; // OK
					continue;
				}
			}
			else if (lstrcmpi(szArg, L"/dir") == 0)
			{
				if ((pcszCmd = NextArg(pcszCmd, szArg)))
				{
					if (pArgs)
					{
						SafeFree(pArgs->pszStartupDir);
						pArgs->pszStartupDir = lstrdup(szArg).Detach();
					}
					apszLine = pcszCmd; // OK
					continue;
				}
			}
			else if (lstrcmpi(szArg, L"/icon") == 0)
			{
				if ((pcszCmd = NextArg(pcszCmd, szArg)))
				{
					if (pArgs)
					{
						SafeFree(pArgs->pszIconFile);
						pArgs->pszIconFile = lstrdup(szArg).Detach();
					}
					apszLine = pcszCmd; // OK
					continue;
				}
			}
			else if (lstrcmpi(szArg, L"/tab") == 0)
			{
				if ((pcszCmd = NextArg(pcszCmd, szArg)))
				{
					if (pArgs)
					{
						SafeFree(pArgs->pszRenameTab);
						pArgs->pszRenameTab = lstrdup(szArg).Detach();
					}
					apszLine = pcszCmd; // OK
					continue;
				}
			}

			const CEStr pszErr(L"Unsupported switch in task command:", L"\r\n", apszLine);
			const int iBtn = MsgBox(pszErr, MB_ICONSTOP | MB_OKCANCEL, GetDefaultTitle());
			if (iBtn == IDCANCEL)
				return nullptr;
			return L"";
		}
	}

	return SkipNonPrintable(apszLine);
}

// Возвращает указатель на АКТИВНУЮ консоль (при создании группы)
// apszScript содержит строки команд, разделенные \r\n
CVirtualConsole* CConEmuMain::CreateConGroup(LPCWSTR apszScript, const RConStartArgsEx *apDefArgs /*= nullptr*/)
{
	CVirtualConsole* pVConResult = nullptr;
	const CEStr pszDataW(apszScript);
	const wchar_t *pszCursor = pszDataW.c_str();
	CEStr szLine;
	//wchar_t *pszNewLine = wcschr(pszLine, L'\n');
	CVirtualConsole *pSetActive = nullptr, *pVCon = nullptr, *pLastVCon = nullptr;
	bool lbSetActive = false, lbOneCreated = false;

	CVConGroup::OnCreateGroupBegin();

	while ((pszCursor = NextLine(pszCursor, szLine, NLF_SKIP_EMPTY_LINES| NLF_TRIM_SPACES)))
	{
		lbSetActive = false;

		// Prepare arguments
		RConStartArgsEx args;

		if (apDefArgs)
		{
			args.AssignFrom(*apDefArgs);

			// If the caller has specified exact split configuration - use it only for the first creating pane
			if (lbOneCreated && args.eSplit)
			{
				args.eSplit = args.eSplitNone;
				args.nSplitValue = DefaultSplitValue;
				args.nSplitPane = 0;
			}
		}

		if (apDefArgs && apDefArgs->pszStartupDir && *apDefArgs->pszStartupDir)
			args.pszStartupDir = lstrdup(apDefArgs->pszStartupDir).Detach();
		else
			SafeFree(args.pszStartupDir);


		// Task pre-options, for example ">* /dir c:\sources cmd"
		LPCWSTR pszLine = ParseScriptLineOptions(szLine.ms_Val, &lbSetActive, &args);

		// Support limited tasks nesting
		CEStr lsTempTask, lsTempLine;
		while (pszLine && IsConsoleBatchOrTask(pszLine))
		{
			lsTempTask = LoadConsoleBatch(pszLine, &args);
			if (lsTempTask.IsEmpty())
				break;
			// We need only first line from nested task
			if (!NextLine(lsTempTask, lsTempLine))
				break;
			pszLine = ParseScriptLineOptions(lsTempLine.ms_Val, nullptr, &args);
		}

		if (pszLine == nullptr)
		{
			// Stop processing this task, error was displayed
			break;
		}

		if (*pszLine)
		{
			SafeFree(args.pszSpecialCmd);
			args.pszSpecialCmd = lstrdup(pszLine).Detach();

			// If any previous tab was marked as "active"/"foreground" for starting group,
			// we need to run others tabs in "background"
			if (!lbSetActive && pSetActive && CVConGroup::isValid(pSetActive))
				args.BackgroundTab = crb_On;

			pVCon = CreateCon(args, false, true);

			if (!pVCon)
			{
				LPCWSTR pszFailMsg = L"Can't create new virtual console! {CConEmuMain::CreateConGroup}";
				LogString(pszFailMsg);
				if ((mn_StartupFinished == ss_Started) || !isInsideInvalid())
				{
					DisplayLastError(pszFailMsg, -1);
				}

				if (!lbOneCreated)
				{
					//Destroy(); -- must call caller
					goto wrap;
				}
			}
			else
			{
				lbOneCreated = true;
				pLastVCon = pVCon;

				const RConStartArgsEx& modArgs = pVCon->RCon()->GetArgs();
				if (lbSetActive || (!pSetActive && (modArgs.ForegroundTab == crb_On)))
					pSetActive = pVCon;

				if (CVConGroup::isVConExists((int)MAX_CONSOLE_COUNT-1))
					break; // No more consoles are allowed
			}
		}

		// While creating group of consoles we have to
		// process messaged to avoid possible deadlocks
		MSG Msg;
		while (PeekMessage(&Msg,0,0,0,PM_REMOVE))
		{
			if (!ProcessMessage(Msg))
				goto wrap;
		}

		if (!ghWnd || !IsWindow(ghWnd))
			goto wrap;
	}

	if (pSetActive)
	{
		Activate(pSetActive);
	}

	pVConResult = (pSetActive ? pSetActive : pLastVCon);
wrap:
	CVConGroup::OnCreateGroupEnd();
	return pVConResult;
}

void CConEmuMain::CreateGhostVCon(CVirtualConsole* apVCon)
{
	PostMessage(ghWnd, mn_MsgInitVConGhost, 0, reinterpret_cast<LPARAM>(apVCon));
}

void CConEmuMain::DebugStep(LPCWSTR asMsg, BOOL abErrorSeverity/*=FALSE*/)
{
	if (asMsg && *asMsg)
	{
		// Если ведется ЛОГ - выбросить в него
		LogString(asMsg);
	}

	if (ghWnd)
	{
		static bool bWasDbgStep, bWasDbgError;

		if (asMsg && *asMsg)
		{
			if (gpSet->isDebugSteps || abErrorSeverity)
			{
				bWasDbgStep = true;

				if (abErrorSeverity) bWasDbgError = true;

				SetTitle(ghWnd, asMsg);
			}
		}
		else
		{
			// Обновит заголовок в соответствии с возможными процентами в неактивной консоли
			// и выполнит это в главной нити, если необходимо
			// abErrorSeverity проверяем, чтобы можно было насильно "повторить" текущий заголовок
			if (bWasDbgStep || abErrorSeverity)
			{
				bWasDbgStep = false;

				if (bWasDbgError)
				{
					bWasDbgError = false;
					return;
				}

				UpdateTitle();
			}
		}
	}
}

DWORD_PTR CConEmuMain::GetActiveKeyboardLayout()
{
	_ASSERTE(mn_MainThreadId!=0);
	DWORD_PTR dwActive = (DWORD_PTR)GetKeyboardLayout(mn_MainThreadId);
	return dwActive;
}

//LPTSTR CConEmuMain::GetTitleStart()
//{
//	//mn_ActiveStatus &= ~CES_CONALTERNATIVE; // сброс флага альтернативной консоли
//	return Title;
//}

LPCWSTR CConEmuMain::GetDefaultTitle() const
{
	return ms_ConEmuDefTitle;
}

LPCWSTR CConEmuMain::GetDefaultTabLabel() const
{
	return L"ConEmu";
}

void CConEmuMain::SetTitleTemplate(LPCWSTR asTemplate)
{
	lstrcpyn(TitleTemplate, asTemplate ? asTemplate : L"", countof(TitleTemplate));
}

// Thread-safe SetWindowText for VCon
// abTrySync == true is allowed only for asTitle pointing to member variables
void CConEmuMain::SetTitle(HWND ahWnd, LPCWSTR asTitle, bool abTrySync /*= false*/)
{
	if (!this)
	{
		_ASSERTE(this!=nullptr);
		return;
	}
	if (!ahWnd || !asTitle)
	{
		return;
	}

	if (gpSet->isLogging())
	{
		wchar_t szHwnd[20]; ultow_s(LODWORD(ahWnd), szHwnd, 16);
		CEStr lsMsg(L"SetTitle: hWnd=x", szHwnd, L" Title=`", asTitle, L"`");
		LogString(lsMsg);
	}

	if (isMainThread())
	{
		SetWindowText(ahWnd, asTitle);
		return;
	}

	if (!IsWindow(ahWnd))
	{
		return;
	}

	// Try to call with timeout? May be main thread is unlocked now...
	if (abTrySync)
	{
		DWORD_PTR dwResult = 0;
		LRESULT lRc = SendMessageTimeout(ahWnd, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(asTitle), SMTO_ABORTIFHUNG, 500, &dwResult);
		if (lRc != 0)
		{
			return; // OK
		}
		UNREFERENCED_PARAMETER(dwResult);
	}

	// Well, we have to post execution to the main thread...
	struct impl
	{
		HWND hWnd;
		wchar_t* psTitle;
		CConEmuMain* pConEmu;
		impl(HWND ahWnd, LPCWSTR asTitle, CConEmuMain* apConEmu)
		{
			hWnd = ahWnd;
			psTitle = lstrdup(asTitle).Detach();
			pConEmu = apConEmu;
		};
		static LRESULT setTitle(LPARAM lParam)
		{
			impl* p = (impl*)lParam;
			if (gpSet->isLogging())
			{
				wchar_t szHwnd[20]; ultow_s(LODWORD(p->hWnd), szHwnd, 16);
				CEStr lsMsg(L"SetTitle(async): hWnd=x", szHwnd, L" Title=`", p->psTitle, L"`");
				p->pConEmu->LogString(lsMsg);
			}
			if (IsWindow(p->hWnd))
				SetWindowText(p->hWnd, p->psTitle);
			free(p->psTitle);
			SafeDelete(p);
			return TRUE;
		};
	};

	CallMainThread(false, impl::setTitle, reinterpret_cast<LPARAM>(new impl(ahWnd, asTitle, this)));
}

bool CConEmuMain::ExecuteProcessPrepare()
{
	TODO("Disable all global hooks for execution time?");
	return false;
}

void CConEmuMain::ExecuteProcessFinished(bool bOpt)
{
	if (bOpt)
	{
		TODO("Return all global hooks for execution time?");
	}
}

void CConEmuMain::SetPostGuiMacro(LPCWSTR asGuiMacro)
{
	ConEmuMacro::ConcatMacro(ms_PostGuiMacro, asGuiMacro);
}

void CConEmuMain::AddPostGuiRConMacro(LPCWSTR asGuiMacro)
{
	ConEmuMacro::ConcatMacro(ms_PostRConMacro, asGuiMacro);
}

void CConEmuMain::ExecPostGuiMacro()
{
	if (ms_PostGuiMacro.IsEmpty() && ms_PostRConMacro.IsEmpty())
		return;

	CVConGuard VCon;
	GetActiveVCon(&VCon);

	if (!ms_PostGuiMacro.IsEmpty())
	{
		CEStr pszMacro = std::move(ms_PostGuiMacro);
		CEStr pszRc = ConEmuMacro::ExecuteMacro(std::move(pszMacro), VCon.VCon() ? VCon->RCon() : nullptr);
	}

	if (!ms_PostRConMacro.IsEmpty() && VCon.VCon())
	{
		CEStr pszMacro = std::move(ms_PostRConMacro);
		CEStr pszRc = ConEmuMacro::ExecuteMacro(std::move(pszMacro), VCon.VCon() ? VCon->RCon() : nullptr);
	}
}

void CConEmuMain::SetTaskbarIcon(HICON ahNewIcon)
{
	// Change only icon on TaskBar, don't change Alt+Tab or TitleBar icons
	#if 1

	if (mh_TaskbarIcon != ahNewIcon)
	{
		wchar_t szLog[140] = L"";
		swprintf_c(szLog, L"SetTaskbarIcon: NewIcon=x%p OldIcon=x%p", (LPVOID)ahNewIcon, (LPVOID)mh_TaskbarIcon);
		LogString(szLog);

		// mh_TaskbarIcon will be returned for WM_GETICON(ICON_SMALL|ICON_SMALL2)
		std::swap(mh_TaskbarIcon, ahNewIcon);
		if (ahNewIcon)
			DestroyIcon(ahNewIcon);

		// Have to "force refresh" the icon on TaskBar, otherwise
		// icon would not be updated on sequential clicks on cbTaskbarOverlay
		SendMessage(ghWnd, WM_SETICON, ICON_SMALL, 0);
		SendMessage(ghWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hClassIconSm));
	}

	#endif

	// This would change window icon (TaskBar, TitleBar, Alt+Tab)
	#if 0
	HICON hOldIcon = (HICON)SendMessage(ghWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(ahNewIcon));
	if (hOldIcon && (hOldIcon != hClassIconSm))
		DestroyIcon(hOldIcon);
	if (mh_TaskbarIcon && (mh_TaskbarIcon != hClassIconSm) && (mh_TaskbarIcon != hOldIcon))
		DestroyIcon(mh_TaskbarIcon);
	mh_TaskbarIcon = ahNewIcon;
	#endif
}

void CConEmuMain::SetWindowIcon(LPCWSTR asNewIcon)
{
	// Don't change TITLE BAR icon after initialization finished
	if (mn_StartupFinished >= ss_Started)
		return;

	if (!asNewIcon || !*asNewIcon)
		return;

	// Do not apply the icon from {Task} if option "Show overlay icon" is on
	// to avoid two similar icons, one over another...
	if (gpSet->isTaskbarOverlay)
		return;

	auto* hOldClassIcon = hClassIcon;
	auto* hOldClassIconSm = hClassIconSm;
	hClassIcon = nullptr; hClassIconSm = nullptr;

	ms_IconPath = ExpandEnvStr(asNewIcon);

	LoadIcons();

	if (hClassIcon)
	{
		SetClassLongPtr(ghWnd, GCLP_HICON, reinterpret_cast<LONG_PTR>(hClassIcon));
		SetClassLongPtr(ghWnd, GCLP_HICONSM, reinterpret_cast<LONG_PTR>(hClassIconSm));
		SendMessage(ghWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hClassIcon));
		SendMessage(ghWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hClassIconSm));
	}
	else
	{
		hClassIcon = hOldClassIcon;
		hClassIconSm = hOldClassIconSm;
	}
}

// Нужно вызывать после загрузки настроек!
void CConEmuMain::LoadIcons()
{
	if (hClassIcon)
		return; // Уже загружены

	CEStr szIconPath;
	CEStr lsLog;

	if (ms_IconPath)
	{
		if (FileExists(ms_IconPath))
		{
			szIconPath.Set(ms_IconPath);
			lsLog = CEStr(L"Loading icon from '/icon' switch `", ms_IconPath, L"`");
		}
		else
		{
			if (!apiSearchPath(nullptr, ms_IconPath, nullptr, szIconPath))
			{
				szIconPath.Clear();
				lsLog = CEStr(L"Icon specified with '/icon' switch `", ms_IconPath, L"` was not found");
			}
			else
			{
				lsLog = CEStr(L"Icon specified with '/icon' was found at `", szIconPath, L"`");
			}
		}
	}
	else
	{
		szIconPath = JoinPath(ms_ConEmuExeDir, L"ConEmu.ico");
		if (!FileExists(szIconPath))
		{
			szIconPath.Clear();
		}
		else
		{
			lsLog = CEStr(L"Loading icon `", szIconPath, L"`");
		}
	}

	if (!lsLog.IsEmpty())
		LogString(lsLog);

	if (!szIconPath.IsEmpty())
	{
		const auto* lpszExt = PointToExt(szIconPath);

		if (lpszExt && (lstrcmpi(lpszExt, L".ico") == 0))
		{
			hClassIcon = static_cast<HICON>(LoadImage(nullptr, szIconPath, IMAGE_ICON,
				GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR | LR_LOADFROMFILE));
			hClassIconSm = static_cast<HICON>(LoadImage(nullptr, szIconPath, IMAGE_ICON,
				GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR | LR_LOADFROMFILE));
		}
		else
		{
			ExtractIconEx(szIconPath, 0, &hClassIcon, &hClassIconSm, 1);
		}
	}

	if (hClassIcon)
	{
		lsLog = CEStr(L"External icons were loaded, small=", hClassIconSm?L"OK":L"nullptr", L", large=", hClassIcon?L"OK":L"nullptr");
		LogString(lsLog);
	}
	else
	{
		szIconPath.Clear();

		hClassIcon = (HICON)LoadImage(GetModuleHandle(0),
		                              MAKEINTRESOURCE(gpSet->nIconID), IMAGE_ICON,
		                              GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);

		if (hClassIconSm) DestroyIcon(hClassIconSm);

		hClassIconSm = (HICON)LoadImage(GetModuleHandle(0),
		                                MAKEINTRESOURCE(gpSet->nIconID), IMAGE_ICON,
		                                GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);

		LogString(L"Using default ConEmu icon");
	}
}

//bool CConEmuMain::LoadVersionInfo(wchar_t* pFullPath)
//{
//	LPBYTE pBuffer=nullptr;
//	wchar_t* pVersion=nullptr;
//	//wchar_t* pDesc=nullptr;
//	const wchar_t WSFI[] = L"StringFileInfo";
//	DWORD size = GetFileVersionInfoSizeW(pFullPath, &size);
//
//	if (!size) return false;
//
//	MCHKHEAP
//	pBuffer = new BYTE[size];
//	MCHKHEAP
//	GetFileVersionInfoW((wchar_t*)pFullPath, 0, size, pBuffer);
//	//Find StringFileInfo
//	DWORD ofs;
//
//	for(ofs = 92; ofs < size; ofs += *(WORD*)(pBuffer+ofs))
//		if (!lstrcmpiW((wchar_t*)(pBuffer+ofs+6), WSFI))
//			break;
//
//	if (ofs >= size)
//	{
//		delete pBuffer;
//		return false;
//	}
//
//	TCHAR *langcode;
//	langcode = (TCHAR*)(pBuffer + ofs + 42);
//	TCHAR blockname[48];
//	unsigned dsize;
//	swprintf_c(blockname, _T("\\%s\\%s\\FileVersion"), WSFI, langcode);
//
//	if (!VerQueryValue(pBuffer, blockname, (void**)&pVersion, &dsize))
//	{
//		pVersion = 0;
//	}
//	else
//	{
//		if (dsize>=31) pVersion[31]=0;
//
//		wcscpy(szConEmuVersion, pVersion);
//		pVersion = wcsrchr(szConEmuVersion, L',');
//
//		if (pVersion && wcscmp(pVersion, L", 0")==0)
//			*pVersion = 0;
//	}
//
//	delete[] pBuffer;
//	return true;
//}

//void CConEmuMain::OnCopyingState()
//{
//	TODO("CConEmuMain::OnCopyingState()");
//}

void CConEmuMain::PostDragCopy(bool abMove, bool abReceived/*=FALSE*/)
{
	if (!abReceived)
	{
		PostMessage(ghWnd, mn_MsgPostCopy, 0, static_cast<LPARAM>(abMove));
	}
	else
	{
		//PostMacro(apszMacro);
		//free(apszMacro);
		CVConGuard VCon;
		if (GetActiveVCon(&VCon) >= 0)
		{
			VCon->RCon()->PostDragCopy(abMove);
		}
	}
}

void CConEmuMain::PostMacro(LPCWSTR asMacro)
{
	if (!asMacro || !*asMacro)
		return;

	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
	{
		VCon->RCon()->PostMacro(asMacro);
	}
	//#ifdef _DEBUG
	//DEBUGSTRMACRO(asMacro); OutputDebugStringW(L"\n");
	//#endif
	//
	//CConEmuPipe pipe(GetFarPID(), CONEMUREADYTIMEOUT);
	//if (pipe.Init(_T("CConEmuMain::PostMacro"), TRUE))
	//{
	//    //DWORD cbWritten=0;
	//    DebugStep(_T("Macro: Waiting for result (10 sec)"));
	//    pipe.Execute(CMD_POSTMACRO, asMacro, (_tcslen(asMacro)+1)*2);
	//    DebugStep(nullptr);
	//}
}

void CConEmuMain::PostFontSetSize(int nRelative/*0/1/2*/, int nValue/*для nRelative==0 - высота, для ==1 - +-1, +-2,..., для ==2 - zoom в процентах*/)
{
	// It will call `gpSetCls->MacroFontSetSize((int)wParam, (int)lParam)` in the main thread
	PostMessage(ghWnd, mn_MsgFontSetSize, (WPARAM)nRelative, static_cast<LPARAM>(nValue));
}

void CConEmuMain::PostMacroFontSetName(wchar_t* pszFontName, WORD anHeight /*= 0*/, WORD anWidth /*= 0*/, BOOL abPosted)
{
	if (!abPosted)
	{
		wchar_t* pszDup = lstrdup(pszFontName).Detach();
		WPARAM wParam = (((DWORD)anHeight) << 16) | (anWidth);
		PostMessage(ghWnd, mn_MsgMacroFontSetName, wParam, reinterpret_cast<LPARAM>(pszDup));
	}
	else
	{
		if (gpSetCls)
			gpFontMgr->MacroFontSetName(pszFontName, anHeight, anWidth);
		free(pszFontName);
	}
}

void CConEmuMain::PostChangeCurPalette(LPCWSTR pszPalette, bool bChangeDropDown, bool abPosted)
{
	if (!abPosted)
	{
		// Synchronous!
		SendMessage(ghWnd, mn_MsgReqChangeCurPalette, (WPARAM)bChangeDropDown, reinterpret_cast<LPARAM>(pszPalette));
	}
	else
	{
		const ColorPalette* pPal = gpSet->PaletteGetByName(pszPalette);
		gpSetCls->ChangeCurrentPalette(pPal, bChangeDropDown);
	}
}

LRESULT CConEmuMain::SyncExecMacro(WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc;
	lRc = SendMessage(ghWnd, mn_MsgMacroExecSync, wParam, lParam);
	return lRc;
}

void CConEmuMain::PostDisplayRConError(CRealConsole* apRCon, wchar_t* pszErrMsg)
{
	//CVConGuard VCon(apRCon->VCon());
	//SendMessage(ghWnd, mn_MsgDisplayRConError, (WPARAM)apRCon, reinterpret_cast<LPARAM>(pszErrMsg));

	if (RELEASEDEBUGTEST(mb_FindBugMode,true))
	{
		//_ASSERTE(FALSE && "Strange 'ConEmuCInput not found' error");
		RaiseTestException();
	}

	PostMessage(ghWnd, mn_MsgDisplayRConError, (WPARAM)apRCon, reinterpret_cast<LPARAM>(pszErrMsg));
}

GlobalHotkeys& CConEmuMain::GetGlobalHotkeys() const
{
	if (!m_Hotkeys)
		throw std::logic_error("m_Hotkeys was not initialized");
	return *m_Hotkeys;
}

void CConEmuMain::CtrlWinAltSpace()
{
	CVConGuard VCon;
	if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
	{
		VCon->RCon()->CtrlWinAltSpace(); // Toggle visibility
	}
}

void CConEmuMain::DeleteVConMainThread(CVirtualConsole* apVCon)
{
	// We can't use SendMessage because of server thread blocking
	DEBUGTEST(LRESULT lRc =)
		PostMessage(ghWnd, mn_MsgDeleteVConMainThread, 0, reinterpret_cast<LPARAM>(apVCon));
	//if (!lRc)
	//{
	//	if (CVConGroup::isValid(apVCon))
	//	{
	//		apVCon->DeleteFromMainThread();
	//	}
	//}
}

// abRecreate: TRUE - пересоздать текущую, FALSE - создать новую
// abConfirm:  TRUE - показать диалог подтверждения
// abRunAs:    TRUE - под админом
bool CConEmuMain::RecreateAction(RecreateActionParm aRecreate, BOOL abConfirm, RConBoolArg bRunAs /*= crb_Undefined*/)
{
	bool bExecRc = false;
	FLASHWINFO fl = {sizeof(FLASHWINFO)}; fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
	FlashWindowEx(&fl); // При многократных созданиях мигать начинает...
	RConStartArgsEx args;

	if (aRecreate == cra_RecreateTab)
	{
		CVConGuard VCon;
		if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
		{
			const RConStartArgsEx& CurArgs = VCon->RCon()->GetArgs();
			args.AssignFrom(CurArgs);
			//args.pszSpecialCmd = CurArgs.CreateCommandLine();
		}
	}

	args.aRecreate = aRecreate;
	args.RunAsAdministrator = bRunAs;

	WARNING("При переходе на новую обработку кнопок больше не нужно");
	//if (!abConfirm && isPressed(VK_SHIFT))
	//	abConfirm = TRUE;

	if ((args.aRecreate == cra_CreateTab) || (args.aRecreate == cra_CreateWindow))
	{
		// Создать новую консоль
		BOOL lbSlotFound = (args.aRecreate == cra_CreateWindow);

		if (args.aRecreate == cra_CreateTab)
		{
			//for (size_t i = 0; i < countof(mp_VCon); i++)
			//{
			//	if (!mp_VCon[i]) { lbSlotFound = TRUE; break; }
			//}
			// Проверяем по последней, т.к. "дырок" у нас быть не может
			if (!isVConExists(MAX_CONSOLE_COUNT-1))
				lbSlotFound = TRUE;
		}

		if (!lbSlotFound)
		{
			static bool bBoxShowed = false;

			if (!bBoxShowed)
			{
				bBoxShowed = true;
				FlashWindowEx(&fl); // При многократных созданиях мигать начинает...
				MBoxA(L"Maximum number of consoles was reached.");
				bBoxShowed = false;
			}

			FlashWindowEx(&fl); // При многократных созданиях мигать начинает...
			return false;
		}

		if (abConfirm)
		{
			int nRc = RecreateDlg(&args);

			//int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, Recreate DlgProc, (LPARAM)&args);
			if (nRc != IDC_START)
				return false;

			CVConGroup::Redraw();
		}

		if (args.aRecreate == cra_CreateTab)
		{
			//Собственно, запуск
			if (CreateCon(args, true))
				bExecRc = true;
		}
		else
		{
			if (!args.pszSpecialCmd || !*args.pszSpecialCmd)
			{
				_ASSERTE((args.pszSpecialCmd && *args.pszSpecialCmd) || !abConfirm);
				SafeFree(args.pszSpecialCmd);
				args.pszSpecialCmd = lstrdup(GetCmd()).Detach();
			}

			if (!args.pszSpecialCmd || !*args.pszSpecialCmd)
			{
				_ASSERTE(args.pszSpecialCmd && *args.pszSpecialCmd);
			}
			else
			{
				// Start new ConEmu.exe process with chosen arguments...
				if (CreateWnd(args))
					bExecRc = true;
			}
		}
	}
	else
	{
		// Restart or close console
		CVConGuard VCon;
		if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
		{
			args.RunAsAdministrator = ((bRunAs == crb_On) || VCon->RCon()->isAdministrator()) ? crb_On : crb_Undefined;

			if (abConfirm)
			{
				int nRc = RecreateDlg(&args);

				//int nRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RESTART), ghWnd, Recreate DlgProc, (LPARAM)&args);
				if (nRc == IDC_TERMINATE)
				{
					// "Terminate" button in the "Recreate" dialog
					// it intended to close active console forcedly
					VCon->RCon()->CloseConsole(true, false);
					return true;
				}

				if (nRc != IDC_START)
					return false;
			}

			if (VCon.VCon())
			{
				VCon->Redraw();
				// Собственно, Recreate
				VCon->RCon()->RecreateProcess(&args);
				bExecRc = true;
			}
		}
	}

	SafeFree(args.pszSpecialCmd);
	SafeFree(args.pszStartupDir);
	SafeFree(args.pszUserName);
	//SafeFree(args.pszUserPassword);
	return bExecRc;
}

int CConEmuMain::RecreateDlg(RConStartArgsEx* apArg, bool abDontAutoSelCmd /*= false*/)
{
	static LONG lnInCall = 0;
	if (lnInCall > 0)
	{
		LogString(L"RecreateDlg was skipped because it is already shown");
		return IDCANCEL;
	}
	MSetter lSet(&lnInCall);

	if (!mp_RecreateDlg)
		mp_RecreateDlg = new CRecreateDlg();

	int nRc = mp_RecreateDlg->RecreateDlg(apArg, abDontAutoSelCmd);

	return nRc;
}



int CConEmuMain::RunSingleInstance(HWND hConEmuWnd /*= nullptr*/, LPCWSTR apszCmd /*= nullptr*/)
{
	int liAccepted = 0;
	LPCWSTR lpszCmd = apszCmd ? apszCmd
		: (m_StartDetached == crb_On) ? nullptr
		: GetCmd();
	wchar_t szLogPrefix[100] = L"";

	if ((!lpszCmd || !*lpszCmd) && (gpSetCls->SingleInstanceShowHide == sih_None))
	{
		// It's not possible to pass anything to existing instance
		liAccepted = -1;
		goto wrap;
	}
	else
	{
		HWND ConEmuHwnd = hConEmuWnd ? hConEmuWnd : FindWindowExW(nullptr, nullptr, VirtualConsoleClassMain, nullptr);
		MArray<HWND> hConEmuS;

		if (ConEmuHwnd)
		{
			if (hConEmuWnd)
			{
				hConEmuS.push_back(ConEmuHwnd);
			}
			else
			{
				// Сразу найдем список окон
				while (ConEmuHwnd)
				{
					hConEmuS.push_back(ConEmuHwnd);

					ConEmuHwnd = FindWindowExW(nullptr, ConEmuHwnd, VirtualConsoleClassMain, nullptr);
				}
			}
		}

		for (INT_PTR i = 0; i < hConEmuS.size(); i++)
		{
			ConEmuHwnd = hConEmuS[i];

			DWORD dwPID = 0, dwPidError = 0;
			if (!GetWindowThreadProcessId(ConEmuHwnd, &dwPID))
			{
				dwPidError = GetLastError();
				_ASSERTE(FALSE && "GetWindowThreadProcessId failed, Access denied?");
			}

			// If we search for ConEmu window (!hConEmuWnd), than check AppID.
			// It's formed of some critical parameters, to ensure that
			// on ‘single instance’ we would not pass new tab into
			// wrong application instance.
			// For example, running "ConEmu -quake" must not create new tab
			// in the existing ConEmu ‘non-quake’ instance.
			if (!hConEmuWnd && dwPID)
			{
				MFileMapping<ConEmuGuiMapping> GuiTestMapping;
				GuiTestMapping.InitName(CEGUIINFOMAPNAME, dwPID);
				// Try to open it
				const ConEmuGuiMapping* ptrMap = GuiTestMapping.Open();
				if (!ptrMap)
				{
					swprintf_c(szLogPrefix, L"GuiTestMapping failed for PID=%u HWND=x%08X: ", dwPID, LODWORD(ConEmuHwnd));
					CEStr lsLog(szLogPrefix, GuiTestMapping.GetErrorText());
					LogString(lsLog);
					continue;
				}
				// Compare protocol and AppID
				if ((ptrMap->nProtocolVersion != CESERVER_REQ_VER)
					|| (lstrcmpi(ptrMap->AppID, this->ms_AppID) != 0))
				{
					swprintf_c(szLogPrefix, L"Skipped due to different AppID; PID=%u HWND=x%08X: \"", dwPID, LODWORD(ConEmuHwnd));
					CEStr lsSkip(szLogPrefix, ptrMap->AppID, L"\"; Our: \"", this->ms_AppID, L"\"");
					LogString(lsSkip);
					continue;
				}
			}

			CESERVER_REQ *pIn = nullptr, *pOut = nullptr;
			int nCmdLen = lpszCmd ? lstrlenW(lpszCmd) : 1;
			int nSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_NEWCMD) + (nCmdLen*sizeof(wchar_t));

			pIn = ExecuteNewCmd(CECMD_NEWCMD, nSize);

			if (pIn)
			{
				// "ShowHide" argument has priority before szCommand!!!
				// If need to run command in new tab of existing window,
				// "ShowHide" must be "sih_None"

				pIn->NewCmd.isAdvLogging = gpSet->isLogging();

				if (gpConEmu->WindowStartMinimized)
					pIn->NewCmd.ShowHide = sih_Minimize;
				if (pIn->NewCmd.ShowHide == sih_None)
					pIn->NewCmd.ShowHide = gpSetCls->SingleInstanceShowHide;
				if ((pIn->NewCmd.ShowHide == sih_None) && (m_StartDetached == crb_On))
					pIn->NewCmd.ShowHide = sih_StartDetached;
				_ASSERTE(pIn->NewCmd.ShowHide != sih_None || m_StartDetached == crb_Undefined);

				//GetCurrentDirectory(countof(pIn->NewCmd.szCurDir), pIn->NewCmd.szCurDir);
				lstrcpyn(pIn->NewCmd.szCurDir, WorkDir(), countof(pIn->NewCmd.szCurDir));

				//GetModuleFileName(nullptr, pIn->NewCmd.szConEmu, countof(pIn->NewCmd.szConEmu));
				wcscpy_c(pIn->NewCmd.szConEmu, ms_ConEmuExeDir);

				pIn->NewCmd.SetCommand(lpszCmd ? lpszCmd : L"");

				// Task? That may have "/dir" switch in task parameters
				if (lpszCmd && (*lpszCmd == TaskBracketLeft) && !mb_ConEmuWorkDirArg && !mb_ConEmuHere)
				{
					RConStartArgsEx args;
					const CEStr pszDataW = LoadConsoleBatch(lpszCmd, &args);
					if (args.pszStartupDir && *args.pszStartupDir)
					{
						lstrcpyn(pIn->NewCmd.szCurDir, args.pszStartupDir, countof(pIn->NewCmd.szCurDir));
					}
				}

				if (dwPID)
					AllowSetForegroundWindow(dwPID);

				pOut = ExecuteGuiCmd(ConEmuHwnd, pIn, nullptr);

				if (pOut && pOut->Data[0])
					liAccepted = 1;
			}

			if (pIn) ExecuteFreeResult(pIn);

			if (pOut) ExecuteFreeResult(pOut);

			if (liAccepted)
				break;

			UNREFERENCED_PARAMETER(dwPidError);
			// Попытаться со следующим окном (может быть запущено несколько экземпляров из разных путей
		}
	}

wrap:
	return liAccepted;
}

void CConEmuMain::ReportOldCmdVersion(DWORD nCmd, DWORD nVersion, int bFromServer, DWORD nFromProcess, HANDLE hFromModule, DWORD nBits)
{
	if (!isMainThread())
	{
		if (mb_InShowOldCmdVersion)
			return; // уже послано
		mb_InShowOldCmdVersion = TRUE;

		static CESERVER_REQ_HDR info = {};
		info.cbSize = sizeof(info);
		info.nCmd = nCmd;
		info.nVersion = nVersion;
		info.nSrcPID = nFromProcess;
		info.hModule = hFromModule;
		info.nBits = nBits;
		PostMessage(ghWnd, mn_MsgOldCmdVer, bFromServer, (LPARAM)&info);

		return;
	}

	// Один раз за сессию?
	static bool lbErrorShowed = false;
	if (lbErrorShowed) return;

	// Попытаемся получить информацию по процессу и модулю
	HANDLE h = nullptr;
	PROCESSENTRY32 pi = {sizeof(pi)};
	MODULEENTRY32  mi = {sizeof(mi)};
	LPCTSTR pszCallPath = nullptr;
	#ifdef _WIN64
	if (nBits == 64)
		h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nFromProcess);
	else
		h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32, nFromProcess);
	#else
	h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, nFromProcess);
	#endif

	wchar_t szSrcExe[MAX_PATH] = L"";

	if (h && h != INVALID_HANDLE_VALUE)
	{
		if (Module32First(h, &mi))
		{
			do {
				if (mi.th32ProcessID == nFromProcess)
				{
					if (!*szSrcExe)
					{
						LPCWSTR pszExePath = mi.szExePath[0] ? mi.szExePath : mi.szModule;
						LPCWSTR pszExt = PointToExt(pszExePath);
						if (lstrcmpi(pszExt, L".exe") == 0)
						{
							lstrcpyn(szSrcExe, pszExePath, countof(szSrcExe));
						}
					}

					if (!hFromModule || (mi.hModule == (HMODULE)hFromModule))
					{
						pszCallPath = mi.szExePath[0] ? mi.szExePath : mi.szModule;
						break;
					}
				}
				else
				{
					_ASSERTE(mi.th32ProcessID == nFromProcess);
				}
			} while (Module32Next(h, &mi));

			//if (!pszCallPath && pszExePath)
			//	pszCallPath = pszExePath;
		}
		CloseHandle(h);
	}

	if (!*szSrcExe)
	{
		h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (h && h != INVALID_HANDLE_VALUE)
		{
			if (Process32First(h, &pi))
			{
				do {
					if (pi.th32ProcessID == nFromProcess)
					{
						lstrcpyn(szSrcExe, pi.szExeFile, countof(szSrcExe));
						break;
					}
				} while (Process32Next(h, &pi));
			}
			CloseHandle(h);
		}
	}

	if (!pszCallPath && *szSrcExe)
	{
		LPCWSTR pszExeName = PointToName(szSrcExe);
		if (IsFarExe(pszExeName))
		{
			pszCallPath = L"(Check plugins in %FARHOME%\\Plugins folder)";
		}
	}

	lbErrorShowed = true;
	int nMaxLen = 255+(pszCallPath ? _tcslen(pszCallPath) : 0)+_tcslen(ms_ConEmuExe)+_tcslen(szSrcExe)+_tcslen(ms_ConEmuDefTitle);
	wchar_t *pszMsg = (wchar_t*)calloc(nMaxLen,sizeof(wchar_t));
	_wsprintf(pszMsg, SKIPLEN(nMaxLen)
		L"%s received wrong version packet!\n"
		L"%s\n\n"
		L"CommandID: %i, Version: %i, ReqVersion: %i, PID: %u\n"
		L"%s\n%s\n%s"
		L"Please check installation files",
		ms_ConEmuDefTitle, ms_ConEmuExe,
		nCmd, nVersion, CESERVER_REQ_VER, nFromProcess,
		szSrcExe, pszCallPath ? pszCallPath : L"", pszCallPath ? L"\n" : L""
		/*(bFromServer==1) ? L"ConEmuC*.exe" : (bFromServer==0) ? L"ConEmu*.dll and Far plugins" : L"ConEmuC*.exe and ConEmu*.dll"*/
		);
	MBox(pszMsg);
	free(pszMsg);
	mb_InShowOldCmdVersion = FALSE; // теперь можно показать еще одно...
}

bool CConEmuMain::LockConhostStart()
{
	// ConHost.exe появился в Windows 7. Но там он создается "от родительского csrss".
	// А вот в Win8 - уже все хорошо, он создается от корневого консольного процесса.
	bool bNeedConHostSearch = (gnOsVer == 0x0601);
	if (!bNeedConHostSearch)
		return false;
	m_LockConhostStart.pcsLock->Lock(m_LockConhostStart.pcs);
	if (m_LockConhostStart.wait)
	{
		// Если консоли создавать слишком быстро - может возникнуть проблема идентификации принадлежности процессов
		Sleep(250);
	}
	m_LockConhostStart.wait = true;
	return true;
}
void CConEmuMain::UnlockConhostStart()
{
	bool bNeedConHostSearch = (gnOsVer == 0x0601);
	if (!bNeedConHostSearch)
		return;
	m_LockConhostStart.pcsLock->Unlock();
}

void CConEmuMain::ReleaseConhostDelay()
{
	m_LockConhostStart.wait = false;
}

// Запуск отладки текущего GUI
bool CConEmuMain::StartDebugLogConsole()
{
	if (IsDebuggerPresent())
		return false; // УЖЕ!

	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	WCHAR  szExe[MAX_PATH*2] = {0};
	bool lbRc = false;
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi = {};
	STARTUPINFO si = {};
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();
	// "/ATTACH" - низя, а то заблокируемся при попытке подключения к "отлаживаемому" GUI
	swprintf_c(szExe, L"\"%s\" /DEBUGPID=%u /BW=80 /BH=25 /BZ=%u",
	          WIN3264TEST(ms_ConEmuC32Full,ms_ConEmuC64Full), dwSelfPID, LONGOUTPUTHEIGHT_MAX);

	#ifdef _DEBUG
	if (MsgBox(szExe, MB_OKCANCEL|MB_SYSTEMMODAL, L"StartDebugLogConsole", nullptr, false) != IDOK)
		return false;
	#endif

	if (!CreateProcess(nullptr, szExe, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, nullptr,
	                  nullptr, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		DWORD dwErr = GetLastError();
		wchar_t szErr[128]; swprintf_c(szErr, L"Can't create debugger console! ErrCode=0x%08X", dwErr);
		MBoxA(szErr);
	}
	else
	{
		gbDebugLogStarted = TRUE;
		SafeCloseHandle(pi.hProcess);
		SafeCloseHandle(pi.hThread);
		lbRc = true;
	}

	return lbRc;
}

bool CConEmuMain::StartDebugActiveProcess()
{
	CVConGuard VCon;
	CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : nullptr;
	if (!pRCon)
	{
		MsgBox(L"There is no active VCon", MB_ICONSTOP, GetDefaultTitle());
		return false;
	}

	DWORD dwPID = pRCon->GetActivePID();
	if (!dwPID)
	{
		// For example, ChildGui started with: set ConEmuReportExe=notepad.exe & notepad.exe
		// Check waiting dialog box with caption "ConEmuHk, PID=%u, ..."
		// The content must be "<ms_RootProcessName> loaded!"
		dwPID = pRCon->GetLoadedPID();
	}
	if (!dwPID)
	{
		MsgBox(L"There is no active process", MB_ICONSTOP, GetDefaultTitle());
		return false;
	}

	bool lbRc = pRCon->StartDebugger(sdt_DebugActiveProcess);
	return lbRc;
}

bool CConEmuMain::MemoryDumpActiveProcess(bool abProcessTree /*= false*/)
{
	CVConGuard VCon;
	CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : nullptr;
	if (!pRCon)
	{
		MsgBox(L"There is no active VCon", MB_ICONSTOP, GetDefaultTitle());
		return false;
	}

	DWORD dwPID = pRCon->GetActivePID();
	if (!dwPID && abProcessTree)
		dwPID = pRCon->GetServerPID();
	if (!dwPID)
	{
		MsgBox(L"There is no active process", MB_ICONSTOP, GetDefaultTitle());
		return false;
	}

	bool lbRc = pRCon->StartDebugger(abProcessTree ? sdt_DumpMemoryTree : sdt_DumpMemory);
	return lbRc;
}

void CConEmuMain::UpdateProcessDisplay(bool abForce)
{
	TODO("Move to CSetPgInfo");
	if (!ghOpWnd)
		return;

	if (!isMainThread())
	{
		PostMessage(ghWnd, mn_MsgUpdateProcDisplay, abForce, 0);
		return;
	}

	HWND hInfo = gpSetCls->GetPage(thi_Info);

	CVConGuard VCon;
	wchar_t szNo[32], szFlags[255]; szNo[0] = szFlags[0] = 0;

	// Не совсем корректно это в статусе процессов показывать, но пока на вкладке Info больше негде
	wchar_t szTile[32] = L"";
	ConEmuWindowCommand tile = GetTileMode(false);
	if (tile != cwc_Current)
	{
		FormatTileMode(tile, szTile, countof(szTile)-1);
		if (szTile[0]) wcscat_c(szTile, L" ");
	}

	// Статусы активной консоли
	if (GetActiveVCon(&VCon) >= 0)
	{
		DWORD nProgramStatus = VCon->RCon()->GetProgramStatus();
		DWORD nFarStatus = VCon->RCon()->GetFarStatus();
		if (nProgramStatus&CES_TELNETACTIVE) wcscat_c(szFlags, L"Telnet ");
		//CheckDlgButton(hInfo, cbsTelnetActive, (nProgramStatus&CES_TELNETACTIVE) ? BST_CHECKED : BST_UNCHECKED);
		if (nProgramStatus&CES_NTVDM) wcscat_c(szFlags, L"16bit ");
		//CheckDlgButton(hInfo, cbsNtvdmActive, (nProgramStatus&CES_NTVDM) ? BST_CHECKED : BST_UNCHECKED);
		if (nProgramStatus&CES_FARACTIVE) wcscat_c(szFlags, L"Far ");
		//CheckDlgButton(hInfo, cbsFarActive, (nProgramStatus&CES_FARACTIVE) ? BST_CHECKED : BST_UNCHECKED);
		if (nFarStatus&CES_FILEPANEL) wcscat_c(szFlags, L"Panels ");
		//CheckDlgButton(hInfo, cbsFilePanel, (nFarStatus&CES_FILEPANEL) ? BST_CHECKED : BST_UNCHECKED);
		if (nFarStatus&CES_EDITOR) wcscat_c(szFlags, L"Editor ");
		//CheckDlgButton(hInfo, cbsEditor, (nFarStatus&CES_EDITOR) ? BST_CHECKED : BST_UNCHECKED);
		if (nFarStatus&CES_VIEWER) wcscat_c(szFlags, L"Viewer ");
		//CheckDlgButton(hInfo, cbsViewer, (nFarStatus&CES_VIEWER) ? BST_CHECKED : BST_UNCHECKED);
		if (nFarStatus&CES_WASPROGRESS) wcscat_c(szFlags, L"%%Progress ");
		//CheckDlgButton(hInfo, cbsProgress, ((nFarStatus&CES_WASPROGRESS) /*|| VCon->RCon()->GetProgress(nullptr)>=0*/) ? BST_CHECKED : BST_UNCHECKED);
		if (nFarStatus&CES_OPER_ERROR) wcscat_c(szFlags, L"%%Error ");
		//CodePage
		swprintf_c(szNo, L"CP:%u:%u ", VCon->RCon()->GetConsoleCP(), VCon->RCon()->GetConsoleOutputCP());
		wcscat_c(szFlags, szNo);
		//GUI window tile mode
		wcscat_c(szFlags, szTile);

		//CheckDlgButton(hInfo, cbsProgressError, (nFarStatus&CES_OPER_ERROR) ? BST_CHECKED : BST_UNCHECKED);
		swprintf_c(szNo, L"%i/%i", VCon->RCon()->GetFarPID(), VCon->RCon()->GetFarPID(TRUE));
	}
	else
	{
		//GUI window tile mode
		wcscat_c(szFlags, szTile);
	}

	if (hInfo)
	{
		SetDlgItemText(hInfo, tsTopPID, szNo);
		SetDlgItemText(hInfo, tsRConFlags, szFlags);
	}

	if (!abForce)
		return;

	MCHKHEAP;

	if (hInfo)
	{
		CVConGroup::OnUpdateProcessDisplay(hInfo);
	}

	MCHKHEAP
}

// gh-1271: update caret position to help accessibilty tools (e.g. Magnifier)
void CConEmuMain::UpdateCaretPos(CVirtualConsole& vcon, const RECT& rect)
{
	if (!gpConEmu->isMeForeground(false, false))
	{
		return;
	}

	if (!m_Foreground.bCaretCreated)
	{
		if (!CreateCaret(ghWnd, nullptr, RectWidth(rect), RectHeight(rect)))
			return;
		m_Foreground.bCaretCreated = true;
	}

	POINT pt = {rect.left, rect.bottom};
	//MapWindowPoints(vcon.GetView(), ghWnd, &pt, 1);
	SetCaretPos(pt.x, pt.y);
}

void CConEmuMain::UpdateCursorInfo(const ConsoleInfoArg* pInfo)
{
	mp_Status->OnConsoleChanged(&pInfo->sbi, &pInfo->cInfo, &pInfo->TopLeft, false);

	CSetPgInfo* pInfoPg;
	if (!gpSetCls->GetPageObj(pInfoPg))
		return;

	if (!isMainThread())
	{
		ConsoleInfoArg* pDup = (ConsoleInfoArg*)malloc(sizeof(*pDup));
		*pDup = *pInfo;
		PostMessage(ghWnd, mn_MsgUpdateCursorInfo, 0, reinterpret_cast<LPARAM>(pDup));
		return;
	}

	pInfoPg->FillCursorInfo(pInfoPg->Dlg(), pInfo);
}

void CConEmuMain::UpdateSizes()
{
	TODO("Move to CSetPgInfo");
	POINT ptCur = {}; GetCursorPos(&ptCur);
	HWND hPoint = WindowFromPoint(ptCur);

	HWND hInfo = gpSetCls->GetPage(thi_Info);

	if (!ghOpWnd || !hInfo)
	{
		// Может курсор-сплиттер нужно убрать или поставить
		if (hPoint && ((hPoint == ghWnd) || (GetParent(hPoint) == ghWnd)))
		{
			PostMessage(ghWnd, WM_SETCURSOR, -1, -1);
		}
		return;
	}

	if (!isMainThread())
	{
		PostMessage(ghWnd, mn_MsgUpdateSizes, 0, 0);
		return;
	}

	// Может курсор-сплиттер нужно убрать или поставить
	if (hPoint && ((hPoint == ghWnd) || (GetParent(hPoint) == ghWnd)))
	{
		SendMessage(ghWnd, WM_SETCURSOR, -1, -1);
	}

	CVConGuard guard;
	GetActiveVCon(&guard);
	CVirtualConsole* pVCon = guard.VCon();

	if (pVCon)
	{
		pVCon->UpdateInfo();
	}
	else
	{
		SetDlgItemText(hInfo, tConBufferChr, _T("?"));
		SetDlgItemText(hInfo, tConSizeChr, _T("?"));
		SetDlgItemText(hInfo, tConLeftTop, _T("?"));
		SetDlgItemText(hInfo, tDCSize, L"?");
		SetDlgItemText(hInfo, tCursorPos, L"?");
		SetDlgItemText(hInfo, tPanelLeft, _T("?"));
		SetDlgItemText(hInfo, tPanelRight, _T("?"));
	}
}

void CConEmuMain::CheckNeedUpdateTitle(LPCWSTR asRConTitle)
{
	if (!asRConTitle)
		return;

	// Issue 1004: overflow of mn_MsgUpdateTitle when "/title" option was specified
	if (TitleTemplate[0] != 0)
		return;

	// Если в консоли заголовок не менялся, но он отличается от заголовка в ConEmu
	if (wcscmp(asRConTitle, GetLastTitle(false)))
		UpdateTitle();
}

// !!!Warning!!! Никаких return. в конце функции вызывается необходимый CheckProcesses
void CConEmuMain::UpdateTitle()
{
	if (GetCurrentThreadId() != mn_MainThreadId)
	{
		PostMessage(ghWnd, mn_MsgUpdateTitle, 0, 0);
		return;
	}

	LPCTSTR pszNewTitle = nullptr;

	if (TitleTemplate[0] != 0)
	{
		DWORD n = ExpandEnvironmentStrings(TitleTemplate, Title, countof(Title));
		if (n && (n < countof(Title)))
			pszNewTitle = Title;
		else
			pszNewTitle = GetDefaultTitle();
	}

	if (!pszNewTitle)
	{
		CVConGuard VCon;
		if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
			pszNewTitle = VCon->RCon()->GetTitle(true);
	}

	if (!pszNewTitle)
	{
		pszNewTitle = GetDefaultTitle();
	}


	if (pszNewTitle && (pszNewTitle != Title) && lstrcmpi(Title, pszNewTitle))
		lstrcpyn(Title, pszNewTitle, countof(Title));


	// SetTitle(ghWnd, psTitle) вызывается здесь
	// Там же обновится L"[%i/%i] " если несколько консолей а табы отключены
	UpdateProgress(/*TRUE*/);
	Icon.UpdateTitle();
	// Под конец - проверить список процессов консоли
	CheckProcesses();
}

// Если в текущей консоли есть проценты - отображаются они
// Иначе - отображается максимальное значение процентов из всех консолей
void CConEmuMain::UpdateProgress()
{
	if (!this || (ghWnd == nullptr))
	{
		_ASSERTE(this && ghWnd);
		return;
	}

	if (GetCurrentThreadId() != mn_MainThreadId)
	{
		/*чтобы не наколоться на многопоточности */
		PostMessage(ghWnd, mn_MsgUpdateTitle, 0, 0);
		return;
	}

	LPCWSTR psTitle = nullptr;
	LPCWSTR pszFixTitle = GetLastTitle(true);
	wchar_t MultiTitle[MAX_TITLE_SIZE+30];
	MultiTitle[0] = 0;
	short nProgress = -1;
	bool bActiveHasProgress = FALSE;
	bool bNeedAddToTitle = FALSE;
	AnsiProgressStatus progressState = AnsiProgressStatus::None;

	CVConGroup::GetProgressInfo(nProgress, bActiveHasProgress, progressState);

	mn_Progress = std::min<short>(nProgress, 100);

	if (!bActiveHasProgress)
	{
		if (!bNeedAddToTitle && (nProgress >= 0))
			bNeedAddToTitle = TRUE;
	}

	static short nLastProgress = -1;
	static bool bLastProgressError = FALSE;
	static bool bLastProgressIndeterminate = FALSE;
	static bool bLastProgressPaused = FALSE;

	const bool stateChanged = bLastProgressError != (progressState == AnsiProgressStatus::Error)
		|| bLastProgressIndeterminate != (progressState == AnsiProgressStatus::Indeterminate)
		|| bLastProgressPaused != (progressState == AnsiProgressStatus::Paused);

	const UINT newApiState = (progressState == AnsiProgressStatus::Running) ? TBPF_NORMAL
		: (progressState == AnsiProgressStatus::Error) ? TBPF_ERROR
		: (progressState == AnsiProgressStatus::Paused) ? TBPF_PAUSED
		: (progressState == AnsiProgressStatus::Indeterminate) ? TBPF_INDETERMINATE
		: TBPF_NOPROGRESS;

	if (nLastProgress != mn_Progress || stateChanged)
	{
		HRESULT hr = S_OK;

		if ((mn_Progress >= 0) && gpSet->isTaskbarProgress)
		{
			hr = Taskbar_SetProgressValue(mn_Progress);

			if (nLastProgress == -1 || stateChanged)
				hr = Taskbar_SetProgressState(newApiState);
		}
		else
		{
			hr = Taskbar_SetProgressState(TBPF_NOPROGRESS);
		}

		// Update last state
		nLastProgress = mn_Progress;
		bLastProgressError = (progressState == AnsiProgressStatus::Error);
		bLastProgressIndeterminate = (progressState == AnsiProgressStatus::Indeterminate);
		bLastProgressPaused = (progressState == AnsiProgressStatus::Paused);
		UNREFERENCED_PARAMETER(hr);
	}

	if ((mn_Progress >= 0) && bNeedAddToTitle)
	{
		psTitle = MultiTitle;
		const INT_PTR nCurTtlLen = _tcslen(MultiTitle);
		if ((mn_Progress == 0) && (progressState == AnsiProgressStatus::Indeterminate))
			_wcscpy_c(MultiTitle+nCurTtlLen, countof(MultiTitle)-nCurTtlLen, L"{*%%} ");
		else
			swprintf_c(MultiTitle+nCurTtlLen, countof(MultiTitle)-nCurTtlLen/*#SECURELEN*/, L"{*%i%%} ", mn_Progress);
	}

	if (gpSetCls->IsMulti() && (gpSet->isNumberInCaption || !isTabsShown()))
	{
		int nCur = 1, nCount = 0;

		nCur = GetActiveVCon(nullptr, &nCount);
		if (nCur < 0)
			nCur = (nCount > 0) ? 1 : 0;
		else
			nCur++;

		if (gpSet->isNumberInCaption || (nCount > 1))
		{
			psTitle = MultiTitle;
			wsprintf(MultiTitle+_tcslen(MultiTitle), L"[%i/%i] ", nCur, nCount);
		}
	}

	if (psTitle)
		wcscat_c(MultiTitle, pszFixTitle);
	else
		psTitle = pszFixTitle;

	SetTitle(ghWnd, psTitle);

	// Задел на будущее
	if (ghWndApp)
		SetTitle(ghWndApp, psTitle);
}

#ifndef _WIN64
/* static */
VOID CConEmuMain::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
{
	_ASSERTE(hwnd!=nullptr);

	#ifdef _DEBUG
	wchar_t szDbg[128];
	#endif

	#ifdef _DEBUG
	switch (anEvent)
	{
		case EVENT_CONSOLE_START_APPLICATION:
		{
			//A new console process has started.
			//The idObject parameter contains the process identifier of the newly created process.
			//If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.
			if ((idChild == CONSOLE_APPLICATION_16BIT) && gpSet->isLogging())
			{
				char szInfo[64]; wsprintfA(szInfo, "NTVDM started, PID=%i\n", idObject);
				gpConEmu->LogString(szInfo, TRUE);
			}

			#ifdef _DEBUG
			swprintf_c(szDbg, L"EVENT_CONSOLE_START_APPLICATION(HWND=0x%08X, PID=%i%s)\n", (DWORD)hwnd, idObject, (idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
			DEBUGSTRCONEVENT(szDbg);
			#endif

		} break;
		case EVENT_CONSOLE_END_APPLICATION:
		{
			//A console process has exited.
			//The idObject parameter contains the process identifier of the terminated process.
			if ((idChild == CONSOLE_APPLICATION_16BIT) && gpSet->isLogging())
			{
				char szInfo[64]; wsprintfA(szInfo, "NTVDM stopped, PID=%i\n", idObject);
				gpConEmu->LogString(szInfo, TRUE);
			}

			#ifdef _DEBUG
			swprintf_c(szDbg, L"EVENT_CONSOLE_END_APPLICATION(HWND=0x%08X, PID=%i%s)\n", (DWORD)hwnd, idObject,
				(idChild == CONSOLE_APPLICATION_16BIT) ? L" 16bit" : L"");
			DEBUGSTRCONEVENT(szDbg);
			#endif

		} break;
		case EVENT_CONSOLE_UPDATE_REGION: // 0x4002
		{
			//More than one character has changed.
			//The idObject parameter is a COORD structure that specifies the start of the changed region.
			//The idChild parameter is a COORD structure that specifies the end of the changed region.
			#ifdef _DEBUG
			COORD crStart, crEnd; memmove(&crStart, &idObject, sizeof(idObject)); memmove(&crEnd, &idChild, sizeof(idChild));
			swprintf_c(szDbg, L"EVENT_CONSOLE_UPDATE_REGION({%i, %i} - {%i, %i})\n", crStart.X,crStart.Y, crEnd.X,crEnd.Y);
			DEBUGSTRCONEVENT(szDbg);
			#endif
		} break;
		case EVENT_CONSOLE_UPDATE_SCROLL: //0x4004
		{
			//The console has scrolled.
			//The idObject parameter is the horizontal distance the console has scrolled.
			//The idChild parameter is the vertical distance the console has scrolled.
			#ifdef _DEBUG
			swprintf_c(szDbg, L"EVENT_CONSOLE_UPDATE_SCROLL(X=%i, Y=%i)\n", idObject, idChild);
			DEBUGSTRCONEVENT(szDbg);
			#endif
		} break;
		case EVENT_CONSOLE_UPDATE_SIMPLE: //0x4003
		{
			//A single character has changed.
			//The idObject parameter is a COORD structure that specifies the character that has changed.
			//Warning! В писании от  микрософта тут ошибка (после репорта была исправлена)
			//The idChild parameter specifies the character in the low word and the character attributes in the high word.
			#ifdef _DEBUG
			COORD crWhere; memmove(&crWhere, &idObject, sizeof(idObject));
			WCHAR ch = (WCHAR)LOWORD(idChild); WORD wA = HIWORD(idChild);
			swprintf_c(szDbg, L"EVENT_CONSOLE_UPDATE_SIMPLE({%i, %i} '%c'(\\x%04X) A=%i)\n", crWhere.X,crWhere.Y, ch, ch, wA);
			DEBUGSTRCONEVENT(szDbg);
			#endif
		} break;
		case EVENT_CONSOLE_CARET: //0x4001
		{
			//Warning! WinXPSP3. Это событие проходит ТОЛЬКО если консоль в фокусе.
			//         А с ConEmu она НИКОГДА не в фокусе, так что курсор не обновляется.
			//The console caret has moved.
			//The idObject parameter is one or more of the following values:
			//      CONSOLE_CARET_SELECTION or CONSOLE_CARET_VISIBLE.
			//The idChild parameter is a COORD structure that specifies the cursor's current position.
			#ifdef _DEBUG
			COORD crWhere; memmove(&crWhere, &idChild, sizeof(idChild));
			swprintf_c(szDbg, L"EVENT_CONSOLE_CARET({%i, %i} Sel=%c, Vis=%c\n", crWhere.X,crWhere.Y,
			                            ((idObject & CONSOLE_CARET_SELECTION)==CONSOLE_CARET_SELECTION) ? L'Y' : L'N',
			                            ((idObject & CONSOLE_CARET_VISIBLE)==CONSOLE_CARET_VISIBLE) ? L'Y' : L'N');
			DEBUGSTRCONEVENT(szDbg);
			#endif
		} break;
		case EVENT_CONSOLE_LAYOUT: //0x4005
		{
			//The console layout has changed.
			DEBUGSTRCONEVENT(L"EVENT_CONSOLE_LAYOUT\n");
		} break;
	}
	#endif

	// Интересуют только 16битные приложения.
	if (!(((anEvent == EVENT_CONSOLE_START_APPLICATION) || (anEvent == EVENT_CONSOLE_END_APPLICATION))
			&& (idChild == CONSOLE_APPLICATION_16BIT)))
		return;

	StartStopType sst = (anEvent == EVENT_CONSOLE_START_APPLICATION) ? sst_App16Start : sst_App16Stop;

	CVConGroup::OnDosAppStartStop(hwnd, sst, idChild);
}
#endif

/* ****************************************************** */
/*                                                        */
/*                      Painting                          */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	Painting() {};
}
#endif

void CConEmuMain::InitInactiveDC(CVirtualConsole* apVCon)
{
	PostMessage(ghWnd, mn_MsgInitInactiveDC, 0, reinterpret_cast<LPARAM>(apVCon));
}

void CConEmuMain::Invalidate(LPRECT lpRect, BOOL bErase /*= TRUE*/)
{
	#ifdef _DEBUG
	static bool bSkip = false;
	if (bSkip)
		return;
	#endif
	::InvalidateRect(ghWnd, lpRect, bErase);
}

void CConEmuMain::InvalidateFrame()
{
	if (!isSelfFrame())
		return;
	auto hRgn = CreateSelfFrameRgn();
	if (hRgn.HasHandle())
	{
		InvalidateRgn(ghWnd, hRgn, FALSE);
		hRgn.GetHandle();
	}
	// avoid glitches at the top of the window (Win10)
	RECT rc = SizeInfo::RealClientRect();
	int diff = RectHeight(SizeInfo::WindowRect()) - RectHeight(rc);
	rc.top = 0; rc.bottom = diff;
	InvalidateRect(ghWnd, &rc, FALSE);
}

void CConEmuMain::InvalidateAll()
{
	Invalidate(nullptr, TRUE);

	CVConGroup::InvalidateAll();

	CVConGroup::InvalidateGaps();

	if (mp_TabBar)
		mp_TabBar->Invalidate();

	//-- No need to invalidate status due to Invalidate(nullptr) called above
	//if (mp_Status)
	//	mp_Status->InvalidateStatusBar();
}

void CConEmuMain::UpdateWindowChild(CVirtualConsole* apVCon)
{
	CVConGroup::UpdateWindowChild(apVCon);
}

bool CConEmuMain::isRightClickingPaint()
{
	return mb_RightClickingPaint;
}

void CConEmuMain::RightClickingPaint(HDC hdcIntVCon, CVirtualConsole* apVCon)
{
	//TODO: Если меняется PanelView - то "под кружочком" останется старый кусок
	if (hdcIntVCon == (HDC)INVALID_HANDLE_VALUE)
	{
		//if (mh_RightClickingWnd)
		//	apiShowWindow(mh_RightClickingWnd, SW_HIDE);
		return;
	}

	BOOL lbSucceeded = FALSE;

	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
	{
		if (!apVCon)
		{
			apVCon = VCon.VCon();
		}
		// По идее, кружок может появиться только в активной консоли
		_ASSERTE(apVCon == VCon.VCon());
	}


	HWND hView = apVCon ? apVCon->GetView() : nullptr;

	if (!hView || gpSet->isDisableMouse)
	{
		DEBUGSTRRCLICK(L"RightClickingPaint: !hView || gpSet->isDisableMouse\n");
	}
	else
	{
		if (gpSet->isRClickTouchInvert())
		{
			// Длинный клик в режиме инверсии?
			lbSucceeded = FALSE;

			DEBUGSTRRCLICK(L"RightClickingPaint: gpSet->isRClickTouchInvert()\n");

			//if (!mb_RightClickingLSent && apVCon)
			//{
			//	mb_RightClickingLSent = TRUE;
			//	// Чтобы установить курсор в панелях точно под кликом
			//	// иначе получается некрасиво, что курсор прыгает только перед
			//	// появлением EMenu, а до этого (пока крутится "кружок") курсор не двигается.
			//	apVCon->RCon()->PostLeftClickSync(mouse.RClkDC);
			//	//apVCon->RCon()->OnMouse(WM_MOUSEMOVE, 0, mouse.RClkDC.X, mouse.RClkDC.Y, true);
			//	//WARNING("По хорошему, нужно дождаться пока мышь обработается");
			//	//apVCon->RCon()->PostMacro(L"MsLClick");
			//	//WARNING("!!! Заменить на CMD_LEFTCLKSYNC?");
			//}
		}
		else if ((gpSet->isRClickSendKey > 1) && (mouse.state & MOUSE_R_LOCKED)
			&& (m_RightClickingFrames > 0) && mh_RightClickingBmp)
		{
			//WORD nRDown = GetKeyState(VK_RBUTTON);
			//POINT ptCur; GetCursorPos(&ptCur);
			//ScreenToClient(hView, &ptCur);
			bool bRDown = isPressed(VK_RBUTTON);

			if (bRDown)
			{
				DWORD dwCurTick = TimeGetTime(); //GetTickCount();
				DWORD dwDelta = dwCurTick - mouse.RClkTick;

				if (dwDelta < RCLICKAPPS_START)
				{
					// Пока рисовать не начали
					lbSucceeded = TRUE;
				}
				// Если держали дольше 10сек - все назад
				else if (dwDelta > RCLICKAPPSTIMEOUT_MAX/*10сек*/)
				{
					lbSucceeded = FALSE;
				}
				else
				{
					lbSucceeded = TRUE;

					if (!mb_RightClickingLSent && apVCon)
					{
						mb_RightClickingLSent = TRUE;
						// Чтобы установить курсор в панелях точно под кликом
						// иначе получается некрасиво, что курсор прыгает только перед
						// появлением EMenu, а до этого (пока крутится "кружок") курсор не двигается.
						apVCon->RCon()->PostLeftClickSync(mouse.RClkDC);
						//apVCon->RCon()->OnMouse(WM_MOUSEMOVE, 0, mouse.RClkDC.X, mouse.RClkDC.Y, true);
						//WARNING("По хорошему, нужно дождаться пока мышь обработается");
						//apVCon->RCon()->PostMacro(L"MsLClick");
						//WARNING("!!! Заменить на CMD_LEFTCLKSYNC?");
					}

					// Прикинуть индекс фрейма
					int nIndex = (dwDelta - RCLICKAPPS_START) * m_RightClickingFrames / (RCLICKAPPSTIMEOUT - RCLICKAPPS_START);

					if (nIndex >= m_RightClickingFrames)
					{
						nIndex = (m_RightClickingFrames-1); // рисуем последний фрейм, мышку можно отпускать
						//-- SetKillTimer(false, TIMER_RCLICKPAINT, 0); // таймер понадобится для "скрытия" кружочка после RCLICKAPPSTIMEOUT_MAX
					}

					#ifdef _DEBUG
					wchar_t szDbg[128];
					swprintf_c(szDbg, L"RightClickingPaint: Delta=%u, nIndex=%u, {%ix%i}\n", dwDelta, nIndex, Rcursor.x, Rcursor.y);
					DEBUGSTRRCLICK(szDbg);
					#endif

					//if (hdcIntVCon || (m_RightClickingCurrent != nIndex))
					{
						// Рисуем
						HDC hdcSelf = nullptr;
						const wchar_t szRightClickingClass[] = L"ConEmu_RightClicking";
						//BOOL lbSelfDC = FALSE;

						if (!mb_RightClickingRegistered)
						{
							WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_OWNDC, CConEmuMain::RightClickingProc, 0, 0,
							                 g_hInstance, nullptr, LoadCursor(nullptr, IDC_ARROW),
							                 nullptr, nullptr, szRightClickingClass, nullptr};
							if (!RegisterClassEx(&wc))
							{
								DisplayLastError(L"Register class failed");
							}
							else
							{
								mb_RightClickingRegistered = TRUE;
							}
						}

						int nHalf = m_RightClickingSize.y>>1;

						if (mb_RightClickingRegistered && (!mh_RightClickingWnd || !IsWindow(mh_RightClickingWnd)))
						{
							POINT pt = {Rcursor.x-nHalf, Rcursor.y-nHalf};
							MapWindowPoints(hView, ghWnd, &pt, 1);
							mh_RightClickingWnd = CreateWindow(szRightClickingClass, L"",
								WS_VISIBLE|WS_CHILD|WS_DISABLED|WS_CLIPSIBLINGS|WS_CLIPCHILDREN,
								pt.x, pt.y, m_RightClickingSize.y, m_RightClickingSize.y,
								ghWnd, (HMENU)9999, g_hInstance, nullptr);
							SetWindowPos(mh_RightClickingWnd, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
						}
						else
						{
							if (!IsWindowVisible(mh_RightClickingWnd))
								apiShowWindow(mh_RightClickingWnd, SW_SHOWNORMAL);
						}

						if (mh_RightClickingWnd && ((hdcSelf = GetDC(mh_RightClickingWnd)) != nullptr))
						{
							DEBUGSTRRCLICK(L"RightClickingPaint: Painting...\n");

							HDC hCompDC = CreateCompatibleDC(hdcSelf);
							HBITMAP hOld = (HBITMAP)SelectObject(hCompDC, mh_RightClickingBmp);

							BLENDFUNCTION bf = {AC_SRC_OVER,0,255,AC_SRC_ALPHA};

							if (hdcIntVCon)
							{
								// Если меняется содержимое консоли - его нужно "обновить" и в нашем "окошке" с кружочком
								BitBlt(hdcSelf, 0, 0, m_RightClickingSize.y, m_RightClickingSize.y,
									hdcIntVCon, Rcursor.x-nHalf, Rcursor.y-nHalf, SRCCOPY);
							}

							GdiAlphaBlend(hdcSelf, 0, 0, m_RightClickingSize.y, m_RightClickingSize.y,
								  hCompDC, nIndex*m_RightClickingSize.y, 0, m_RightClickingSize.y, m_RightClickingSize.y, bf);

							if (hOld && hCompDC)
								SelectObject(hCompDC, hOld);
							if (hCompDC)
								DeleteDC(hCompDC);

							//if (/*lbSelfDC &&*/ hdcSelf)
							DeleteDC(hdcSelf);
						}
					}

					// Запомним фрейм, что рисовали в последний раз
					m_RightClickingCurrent = nIndex;
				}
			}
		}
		else
		{
			DEBUGSTRRCLICK(L"RightClickingPaint: Condition failed\n");
		}
	}

	if (!lbSucceeded)
	{
		StopRightClickingPaint();
	}
}

void CConEmuMain::StartRightClickingPaint()
{
	if (!mb_RightClickingPaint)
	{
		if (m_RightClickingFrames > 0 && mh_RightClickingBmp)
		{
			m_RightClickingCurrent = -1;
			mb_RightClickingPaint = TRUE;
			mb_RightClickingLSent = FALSE;
			SetKillTimer(true, TIMER_RCLICKPAINT, TIMER_RCLICKPAINT_ELAPSE);
		}
	}
}

void CConEmuMain::StopRightClickingPaint()
{
	DEBUGSTRRCLICK(L"StopRightClickingPaint\n");

	if (mh_RightClickingWnd)
	{
		DestroyWindow(mh_RightClickingWnd);
		mh_RightClickingWnd = nullptr;
	}

	if (mb_RightClickingPaint)
	{
		mb_RightClickingPaint = FALSE;
		mb_RightClickingLSent = FALSE;
		SetKillTimer(false, TIMER_RCLICKPAINT, 0);
		m_RightClickingCurrent = -1;
		CVConGroup::InvalidateAll();
	}
}

/* static */
/* Смысл этого окошка в том, чтобы отрисоваться поверх возможного PanelView */
LRESULT CConEmuMain::RightClickingProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
	case WM_CREATE:
		return 0; // allow
	case WM_PAINT:
		{
			// Если этого не сделать - в очереди "зависнет" WM_PAINT
			PAINTSTRUCT ps = {};
			BeginPaint(hWnd, &ps);
			//RECT rcClient; GetClientRect(hWnd, &rcClient);
			//FillRect(ps.hdc, &rcClient, (HBRUSH)GetStockObject(WHITE_BRUSH));
			EndPaint(hWnd, &ps);
		}
		return 0;
	case WM_ERASEBKGND:
		return 0;
	}

	return ::DefWindowProc(hWnd, messg, wParam, lParam);
}

//void CConEmuMain::OnPaintClient(HDC hdc/*, int width, int height*/)
//{
//	// Если "завис" PostUpdate
//	if (mp_TabBar->NeedPostUpdate())
//		mp_TabBar->Update();
//
//#ifdef _DEBUG
//	RECT rcDbgSize; GetWindowRect(ghWnd, &rcDbgSize);
//	wchar_t szSize[255]; swprintf_c(szSize, L"WM_PAINT -> Window size (X=%i, Y=%i, W=%i, H=%i)\n",
//	                               rcDbgSize.left, rcDbgSize.top, (rcDbgSize.right-rcDbgSize.left), (rcDbgSize.bottom-rcDbgSize.top));
//	DEBUGSTRSIZE(szSize);
//	static RECT rcDbgSize1;
//
//	if (memcmp(&rcDbgSize1, &rcDbgSize, sizeof(rcDbgSize1)))
//	{
//		rcDbgSize1 = rcDbgSize;
//	}
//#endif
//
//	//PaintGaps(hdc);
//}

void CConEmuMain::InvalidateGaps()
{
	if (isIconic())
		return;

	CVConGroup::InvalidateGaps();
}

void CConEmuMain::Update(bool isForce /*= false*/)
{
	CVConGroup::Update(isForce);
}

/* ****************************************************** */
/*                                                        */
/*                 Status functions                       */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	Status_Functions() {};
}
#endif

DWORD CConEmuMain::GetFarPID(bool abPluginRequired/*=false*/)
{
	DWORD dwPID = CVConGroup::GetFarPID(abPluginRequired);
	return dwPID;
}

LPCTSTR CConEmuMain::GetLastTitle(bool abUseDefault/*=true*/)
{
	if (!this)
		return L"ConEmu";

	if (!Title[0] && abUseDefault)
		return GetDefaultTitle();

	return Title;
}

LPCTSTR CConEmuMain::GetVConTitle(int nIdx)
{
	CVConGuard VCon;
	if (!CVConGroup::GetVCon(nIdx, &VCon))
		return nullptr;

	if (!VCon.VCon() || !VCon->RCon())
		return nullptr;

	LPCWSTR pszTitle = VCon->RCon()->GetTitle();
	if (pszTitle == nullptr)
	{
		_ASSERTE(pszTitle!=nullptr);
		pszTitle = GetDefaultTitle();
	}
	return pszTitle;
}

// Returns 0-based index of the active console if succeeded
int CConEmuMain::GetActiveVCon(CVConGuard* pVCon /*= nullptr*/, int* pAllCount /*= nullptr*/)
{
	return CVConGroup::GetActiveVCon(pVCon, pAllCount);
}

HICON CConEmuMain::GetCurrentVConIcon()
{
	HICON hIcon = nullptr;
	int iRConIcon = 0;
	CVConGuard VCon;

	if (mp_TabBar && (GetActiveVCon(&VCon) >= 0))
	{
		iRConIcon = VCon->RCon()->GetRootProcessIcon();
		if (iRConIcon >= 0)
		{
			hIcon = mp_TabBar->GetTabIconByIndex(iRConIcon);
		}
	}

	return hIcon;
}

bool CConEmuMain::IsActiveConAdmin()
{
	CVConGuard VCon;
	bool bAdmin = (GetActiveVCon(&VCon) >= 0)
		? VCon->RCon()->isAdministrator()
		: mb_IsUacAdmin;
	return bAdmin;
}

// 0 - такой консоли нет
// 1..MAX_CONSOLE_COUNT - "номер" консоли (1 based!)
int CConEmuMain::isVConValid(CVirtualConsole* apVCon)
{
	int nIdx = CVConGroup::GetVConIndex(apVCon);
	if (nIdx >= 0)
		return (nIdx+1);
	return 0;
}

bool CConEmuMain::isConSelectMode()
{
	return CVConGroup::isConSelectMode();
}

// Возвращает true если начат ShellDrag
bool CConEmuMain::isDragging()
{
	if ((mouse.state & (DRAG_L_STARTED | DRAG_R_STARTED)) == 0)
		return false;

	if (isConSelectMode())
	{
		mouse.state &= ~(DRAG_L_STARTED | DRAG_L_ALLOWED | DRAG_R_STARTED | DRAG_R_ALLOWED | MOUSE_DRAGPANEL_ALL);
		return false;
	}

	return true;
}

bool CConEmuMain::isFirstInstance(bool bFolderIgnore /*= false*/)
{
	// ms_AppID must be initialized already!
	_ASSERTE(ms_AppID[0] != 0);

	if (!mb_AliveInitialized)
	{
		//создадим событие, чтобы не было проблем с ключом /SINGLE
		mb_AliveInitialized = TRUE;

		wcscpy_c(ms_ConEmuAliveEvent, CEGUI_ALIVE_EVENT);
		wcscat_c(ms_ConEmuAliveEvent, L"_");
		wcscat_c(ms_ConEmuAliveEvent, ms_AppID);

		//Events by default are created in "Local\\" kernel namespace.
		//GetUserName(ms_ConEmuAliveEvent+_tcslen(ms_ConEmuAliveEvent), &nSize);
		//WARNING("Event не работает, если conemu.exe запущен под другим пользователем");

		// Create event distinct by program folder and config name
		SetLastError(0);
		mh_ConEmuAliveEvent = CreateEvent(nullptr, TRUE, TRUE, ms_ConEmuAliveEvent);
		mn_ConEmuAliveEventErr = GetLastError();
		_ASSERTE(mh_ConEmuAliveEvent!=nullptr);

		// -- name can contain any character except the backslash character (\).

		mb_ConEmuAliveOwned = (mh_ConEmuAliveEvent != nullptr) && (mn_ConEmuAliveEventErr != ERROR_ALREADY_EXISTS);

		// Теперь - глобальное
		SetLastError(0);
		mh_ConEmuAliveEventNoDir = CreateEvent(nullptr, TRUE, TRUE, CEGUI_ALIVE_EVENT);
		mn_ConEmuAliveEventErrNoDir = GetLastError();
		mb_ConEmuAliveOwnedNoDir = (mh_ConEmuAliveEventNoDir != nullptr) && (mn_ConEmuAliveEventErrNoDir != ERROR_ALREADY_EXISTS);
		_ASSERTE(mh_ConEmuAliveEventNoDir!=nullptr);
	}


	// Попытка регистрации - по локальному событию. Ошибку показывать - только по глобальному.
	if (mh_ConEmuAliveEvent && !mb_ConEmuAliveOwned)
	{
		if (WaitForSingleObject(mh_ConEmuAliveEvent,0) == WAIT_TIMEOUT)
		{
			SetEvent(mh_ConEmuAliveEvent);
			mb_ConEmuAliveOwned = true;

			// Этот экземпляр становится "Основным" (другой, ранее бывший основным, был закрыт)
			GetGlobalHotkeys().RegisterMinRestore(true);
		}
	}

	if (mh_ConEmuAliveEventNoDir && !mb_ConEmuAliveOwnedNoDir)
	{
		if (WaitForSingleObject(mh_ConEmuAliveEventNoDir,0) == WAIT_TIMEOUT)
		{
			SetEvent(mh_ConEmuAliveEventNoDir);
			mb_ConEmuAliveOwnedNoDir = true;
		}
	}

	//

	// Смотря что просили
	return bFolderIgnore ? mb_ConEmuAliveOwnedNoDir : mb_ConEmuAliveOwned;
}

bool CConEmuMain::isLBDown()
{
	return (mouse.state & DRAG_L_ALLOWED) == DRAG_L_ALLOWED;
}

// hForcedForeground is used during mouse activation
bool CConEmuMain::RecheckForegroundWindow(LPCWSTR asFrom, HWND* phFore /*= nullptr*/, HWND hForcedForeground /*= nullptr*/)
{
	DWORD NewState = fgf_Background;

	// Call this function only
	HWND hForeWnd = hForcedForeground ? hForcedForeground : getForegroundWindow();
	HWND hInsideFocus = nullptr;
	DWORD nForePID = 0;
	bool bChanged = false;
	HWND hOldFore = m_Foreground.hLastFore;
	DWORD nOldState = m_Foreground.ForegroundState;

	if (mp_Inside)
	{
		if (mp_Inside->IsParentProcess(hForeWnd))
		{
			if (hForcedForeground)
			{
				// During WM_MOUSEACTIVATE activation the focus was not ajusted yet
				// So we predict the active status to avoid fade active/inactive flickering
				_ASSERTE(asFrom && 0==wcscmp(asFrom,L"OnActivateByMouse"));
				NewState |= fgf_InsideParent;
			}
			else if ((hInsideFocus = mp_Inside->CheckInsideFocus()))
			{
				DWORD nInsideFocusPID = 0; GetWindowThreadProcessId(hInsideFocus, &nInsideFocusPID);
				if (nInsideFocusPID == GetCurrentProcessId())
				{
					NewState |= fgf_InsideParent;
				}
			}
		}
	}

	if (hForeWnd != m_Foreground.hLastFore)
	{
		bool bNonResponsive = false;

		if (hForeWnd != nullptr)
		{
			if (hForeWnd == ghWnd)
			{
				NewState |= fgf_ConEmuMain;
			}
			else
			{
				GetWindowThreadProcessId(hForeWnd, &nForePID);

				if (nForePID == GetCurrentProcessId())
				{
					NewState |= fgf_ConEmuDialog;
				}
				else if (mp_Inside && mp_Inside->IsParentProcess(hForeWnd))
				{
					// Already processed
				}
				else if (CVConGroup::isOurWindow(hForeWnd))
				{
					NewState |= fgf_RealConsole;
				}
			}

			// DefTerm checks
			// If user want to use ConEmu as default terminal for CUI apps
			// we need to hook GUI applications (e.g. explorer)
			if (!(NewState & fgf_ConEmuAny)
				&& nForePID && mp_DefTrm->IsReady() && gpSet->isSetDefaultTerminal && isMainThread())
			{
				if (mp_DefTrm->IsAppMonitored(hForeWnd, nForePID))
				{
					// If window is non-responsive (application was not loaded yet)
					if (!CDefTermBase::IsWindowResponsive(hForeWnd))
					{
						// DefTerm hooker will skip it
						bNonResponsive = true;
						#ifdef USEDEBUGSTRDEFTERM
						wchar_t szInfo[120];
						swprintf_c(szInfo, L"!!! DefTerm::CheckForeground skipped (timer), x%08X PID=%u is non-responsive !!!", (DWORD)(DWORD_PTR)hForeWnd, nForePID);
						DEBUGSTRDEFTERM(szInfo);
						#endif
					}
					else
					{
						mp_DefTrm->CheckForeground(hForeWnd, nForePID);
					}
				}
			}
		}

		// Logging
		bChanged = true;

		// And remember 'non-responsive' state to be able recheck DefTerm
		if ((m_Foreground.nDefTermNonResponsive && !bNonResponsive) || (m_Foreground.nDefTermNonResponsive != nForePID))
		{
			m_Foreground.nDefTermNonResponsive = bNonResponsive ? nForePID : 0;
			m_Foreground.nDefTermTick = bNonResponsive ? GetTickCount() : 0;
		}
	}

	if ((hForeWnd != m_Foreground.hLastFore)
		|| (m_Foreground.hLastInsideFocus != hInsideFocus))
	{
		// Save new state
		if (m_Foreground.ForegroundState != NewState)
		{
			bChanged = true;
			m_Foreground.ForegroundState = NewState;
		}
		m_Foreground.hLastFore = hForeWnd;
		m_Foreground.hLastInsideFocus = hInsideFocus;
	}

	if (bChanged)
	{
		wchar_t szLog[120];
		swprintf_c(szLog, L"Foreground state changed (%s): State=x%02X HWND=x%08X PID=%u OldHWND=x%08X OldState=x%02X",
			asFrom, NewState, (DWORD)(DWORD_PTR)hForeWnd, nForePID, (DWORD)(DWORD_PTR)hOldFore, nOldState);
		if (gpSet->isLogging()) { LogString(szLog); } else { DEBUGSTRFOCUS(szLog); }
	}

	// DefTerm, Recheck?
	if (m_Foreground.nDefTermNonResponsive
		&& ((GetTickCount() - m_Foreground.nDefTermTick) > DEF_TERM_ALIVE_RECHECK_TIMEOUT)
		)
	{
		// For example, Visual Studio was non-responsive while loading
		GetWindowThreadProcessId(hForeWnd, &nForePID);
		if ((nForePID != m_Foreground.nDefTermNonResponsive)
			|| (CDefTermBase::IsWindowResponsive(hForeWnd)
				&& mp_DefTrm->CheckForeground(hForeWnd, nForePID))
			)
		{
			m_Foreground.nDefTermNonResponsive = m_Foreground.nDefTermTick = 0;
		}
	}

	if (phFore)
		*phFore = hForeWnd;

	return ((m_Foreground.ForegroundState & fgf_ConEmuMain) != fgf_Background);
}

bool CConEmuMain::isInside()
{
	if (!this)
		return false;

	if (!mp_Inside)
		return false;

	_ASSERTE(mp_Inside->GetInsideIntegration() != CConEmuInside::ii_None);
	return (mp_Inside->GetInsideIntegration() != CConEmuInside::ii_None);
}

// Returns true if we were started in "Inside" mode
// *AND* parent window was terminated
bool CConEmuMain::isInsideInvalid()
{
	// Validate pointers and mode
	if (!isInside())
		return false;
	// If we failed to or not yet determined the parent window HWND
	if (!mp_Inside->GetParentWnd())
		return false;
	// Check parent descriptor
	if (::IsWindow(mp_Inside->GetParentWnd()))
		return false;
	// Abnormal termination of parent window?
	return true;
}

bool CConEmuMain::isMeForeground(bool abRealAlso/*=false*/, bool abDialogsAlso/*=true*/, HWND* phFore/*=nullptr*/)
{
	if (!this) return false;

	// Reuse states evaluated in RecheckForegroundWindow
	DWORD nMask = fgf_ConEmuMain
		| (abDialogsAlso ? fgf_ConEmuDialog : fgf_Background)
		| (abRealAlso ? fgf_RealConsole : fgf_Background)
		| (mp_Inside ? fgf_InsideParent : fgf_Background);
	bool isMe = ((m_Foreground.ForegroundState & nMask) != fgf_Background);

	if (phFore)
		*phFore = m_Foreground.hLastFore;

	return isMe;
}

bool CConEmuMain::isMouseOverFrame(bool abReal)
{
	if ((m_ForceShowFrame != fsf_Hide) && isSizing())
	{
		if (!isPressed(VK_LBUTTON))
		{
			EndSizing();
		}
		else
		{
			return true;
		}
	}

	bool bCurForceShow = false;

	if (abReal)
	{
		POINT ptMouse; GetCursorPos(&ptMouse);
		RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);

		const int nAdd = 2;
		const int nSub = 1;

		// чтобы область активации рамки была чуть побольше, увеличим регион
		rcWnd.left -= nAdd; rcWnd.right += nAdd; rcWnd.bottom += nAdd;
		if (gpSet->isQuakeStyle)
			rcWnd.top += nSub; // Don't activate on the TOP edge in Quake mode
		else
			rcWnd.top -= nAdd;

		if (PtInRect(&rcWnd, ptMouse))
		{
			RECT rcClient = ClientRect();

			// чтобы область активации рамки была чуть побольше, уменьшим регион
			rcClient.left += nSub; rcClient.right -= nSub; rcClient.bottom -= nSub;
			if (gpSet->isQuakeStyle)
				rcWnd.top -= nAdd;
			else
				rcClient.top += nSub;

			MapWindowPoints(ghWnd, nullptr, (LPPOINT)&rcClient, 2);

			if (!PtInRect(&rcClient, ptMouse))
			{
				bCurForceShow = true;
			}
			else if (mp_Status)
			{
				POINT ptClient = {ptMouse.x - rcClient.left, ptMouse.y - rcClient.top};
				if (mp_Status->IsCursorOverResizeMark(ptClient))
					bCurForceShow = true;
			}
		}
	}
	else
	{
		bCurForceShow = (m_ForceShowFrame == fsf_Show);
	}

	return bCurForceShow;
}

bool CConEmuMain::isNtvdm(BOOL abCheckAllConsoles/*=FALSE*/)
{
	return CVConGroup::isNtvdm(abCheckAllConsoles);
}

bool CConEmuMain::isOurConsoleWindow(HWND hCon)
{
	return CVConGroup::isOurConsoleWindow(hCon);
}

bool CConEmuMain::isValid(CRealConsole* apRCon)
{
	return CVConGroup::isValid(apRCon);
}

bool CConEmuMain::isValid(CVirtualConsole* apVCon)
{
	return CVConGroup::isValid(apVCon);
}

bool CConEmuMain::isVConExists(int nIdx)
{
	return CVConGroup::isVConExists(nIdx);
}

bool CConEmuMain::isVConHWND(HWND hChild, CVConGuard* pVCon /*= nullptr*/)
{
	return CVConGroup::isVConHWND(hChild, pVCon);
}

bool CConEmuMain::isViewer()
{
	return CVConGroup::isViewer();
}

//bool CConEmuMain::isChildWindowVisible()
//{
//	return CVConGroup::isChildWindowVisible();
//}

// Заголовок окна для PictureView вообще может пользователем настраиваться, так что
// рассчитывать на него при определения "Просмотра" - нельзя
bool CConEmuMain::isPictureView()
{
	return CVConGroup::isPictureView();
}

bool CConEmuMain::isProcessCreated()
{
	return mb_ProcessCreated;
}

// Сюда может придти только LOWORD от HKL
void CConEmuMain::SwitchKeyboardLayout(DWORD_PTR dwNewKeybLayout)
{
	if ((gpSet->isMonitorConsoleLang & 1) == 0)
	{
		if (gpSet->isLogging(2))
			LogString(L"CConEmuMain::SwitchKeyboardLayout skipped, cause of isMonitorConsoleLang==0");

		return;
	}

	#ifdef _DEBUG
	wchar_t szDbg[128]; swprintf_c(szDbg, L"CConEmuMain::SwitchKeyboardLayout(0x%08I64X)\n", (unsigned __int64)dwNewKeybLayout);
	DEBUGSTRLANG(szDbg);
	#endif

	HKL hKeyb[20] = {};
	UINT nCount, i;
	BOOL lbFound = FALSE;

	nCount = GetKeyboardLayoutList(countof(hKeyb), hKeyb);

	for (i = 0; !lbFound && i < nCount; i++)
	{
		if (hKeyb[i] == (HKL)dwNewKeybLayout)
			lbFound = TRUE;
	}

	WARNING("Похоже с другими раскладками будет глючить. US Dvorak?");

	for (i = 0; !lbFound && i < nCount; i++)
	{
		if ((((DWORD_PTR)hKeyb[i]) & 0xFFFF) == (dwNewKeybLayout & 0xFFFF))
		{
			lbFound = TRUE; dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
		}
	}

	// Если не задана раскладка (только язык?) формируем по умолчанию
	if (!lbFound && (dwNewKeybLayout == (dwNewKeybLayout & 0xFFFF)))
		dwNewKeybLayout |= (dwNewKeybLayout << 16);

	// Может она сейчас и активна?
	if (dwNewKeybLayout != GetActiveKeyboardLayout())
	{
#ifdef _DEBUG
		swprintf_c(szDbg, L"CConEmuMain::SwitchKeyboardLayout change to(0x%08I64X)\n", (unsigned __int64)dwNewKeybLayout);
		DEBUGSTRLANG(szDbg);
#endif

		if (gpSet->isLogging(2))
		{
			wchar_t szInfo[255];
			swprintf_c(szInfo, L"CConEmuMain::SwitchKeyboardLayout, posting WM_INPUTLANGCHANGEREQUEST, WM_INPUTLANGCHANGE for 0x%08X",
			          (DWORD)dwNewKeybLayout);
			LogString(szInfo);
		}

		// Теперь переключаем раскладку
		PostMessage(ghWnd, WM_INPUTLANGCHANGEREQUEST, 0, static_cast<LPARAM>(dwNewKeybLayout));
		PostMessage(ghWnd, WM_INPUTLANGCHANGE, 0, static_cast<LPARAM>(dwNewKeybLayout));
	}
	else
	{
		if (gpSet->isLogging(2))
		{
			wchar_t szInfo[255];
			swprintf_c(szInfo, L"CConEmuMain::SwitchKeyboardLayout skipped, cause of GetActiveKeyboardLayout()==0x%08X",
			          (DWORD)dwNewKeybLayout);
			LogString(szInfo);
		}
	}
}





/* ****************************************************** */
/*                                                        */
/*                        EVENTS                          */
/*                                                        */
/* ****************************************************** */

#ifdef colorer_func
{
	EVENTS() {};
}
#endif

// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
void CConEmuMain::OnBufferHeight() //BOOL abBufferHeight)
{
	if (!isMainThread())
	{
		PostMessage(ghWnd, mn_MsgPostOnBufferHeight, 0, 0);
		return;
	}

	CVirtualConsole* pVCon = nullptr;
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
		pVCon = VCon.VCon();


	BOOL lbBufferHeight = FALSE;
	BOOL lbAlternative = FALSE;

	if (pVCon)
	{
		mp_Status->OnConsoleBufferChanged(pVCon->RCon());

		lbBufferHeight = pVCon->RCon()->isBufferHeight();
		lbAlternative = pVCon->RCon()->isAlternative();
	}

	TrackMouse(); // спрятать или показать прокрутку, если над ней мышка

	mp_TabBar->OnBufferHeight(lbBufferHeight);
	mp_TabBar->OnAlternative(lbAlternative);
}


//bool CConEmuMain::DoClose()
//{
//	bool lbProceed = CVConGroup::OnScClose();
//
//	return lbProceed;
//}

BYTE CConEmuMain::CloseConfirmFlags()
{
	if (SilentMacroClose)
		return Settings::cc_None;
	return gpSet->nCloseConfirmFlags;
}

void CConEmuMain::PostScClose()
{
	// Post mn_MsgPostScClose instead of WM_SYSCOMMAND(SC_CLOSE) to ensure that it is our message
	// and it will not be skipped by CConEmuMain::isSkipNcMessage
	PostMessage(ghWnd, mn_MsgPostScClose, 0, 0);
}

// returns true if gpConEmu->Destroy() was called
bool CConEmuMain::OnScClose()
{
	LogString(L"CVConGroup::OnScClose()");

	bool lbMsgConfirmed = false;

	// lbMsgConfirmed - был ли показан диалог подтверждения, или юзер не включил эту опцию
	if (!CVConGroup::OnCloseQuery(&lbMsgConfirmed))
		return false; // не закрывать

	return CVConGroup::DoCloseAllVCon(lbMsgConfirmed);
}

void CConEmuMain::SetScClosePending(bool bFlag)
{
	mb_ScClosePending = bFlag;
}

bool CConEmuMain::isScClosing()
{
	return mb_ScClosePending;
}

bool CConEmuMain::isCloseConfirmed()
{
	if (!(CloseConfirmFlags() & Settings::cc_Window))
		return false;
	return DisableCloseConfirm ? true : mb_ScClosePending;
}

LRESULT CConEmuMain::OnCreate(HWND hWnd, LPCREATESTRUCT lpCreate)
{
	// Must be set already from CConEmuMain::MainWndProc, but just to be sure
	_ASSERTE(ghWnd == hWnd);
	ghWnd = hWnd;

	wchar_t szInfo[200];
	swprintf_c(szInfo,
		L"OnCreate: hWnd=x%08X, x=%i, y=%i, cx=%i, cy=%i, style=x%08X, exStyle=x%08X",
		(DWORD)(DWORD_PTR)hWnd, lpCreate->x, lpCreate->y, lpCreate->cx, lpCreate->cy, lpCreate->style, lpCreate->dwExStyle);
	if (!LogString(szInfo)) { DEBUGSTRSIZE(szInfo); }

	if (const auto theme = gpConEmu->opt.WindowTheme.GetStr())
		gpConEmu->SetWindowTheme(hWnd, theme, nullptr);

	RECT rcWndMin = {WINDOWS_ICONIC_POS, WINDOWS_ICONIC_POS, WINDOWS_ICONIC_POS+160, WINDOWS_ICONIC_POS+16};
	SizeInfo::RequestRect(rcWndMin);

	Icon.LoadIcon(hWnd, gpSet->nIconID/*IDI_ICON1*/);

	// Support FlashWindow requests from Far Manager and application starting up
	mb_ShellHookRegistered = mb_ShellHookRegistered
		|| RegisterShellHookWindow(hWnd);

	#ifdef _DEBUG // due to unittests
	_ASSERTE(ghConWnd==nullptr && "ConWnd must not be created yet");
	#endif
	OnActiveConWndStore(nullptr); // Refresh window data

	// Create or update SystemMenu
	mp_Menu->GetSysMenu(TRUE);

	// Holder
	_ASSERTE(ghWndWork==nullptr); // еще не должен был быть создан
	if (!ghWndWork && !CreateWorkWindow())
	{
		return -1; // The error already must be shown
	}

	_ASSERTE(InCreateWindow() == true);

	// Start pipe server
	if (!m_GuiServer.Start())
	{
		return -1; // The error already must be shown
	}

	mn_StartupFinished = ss_OnCreateCalled;

	return 0;
}

void CConEmuMain::OnCreateFinished()
{
	// Must be set already from CConEmuMain::MainWndProc, but just to be sure
	_ASSERTE(ghWnd != nullptr);
	_ASSERTE(ghWndWork != nullptr);

	RECT rcWndS = {}, rcWndT = {}, rcBeforeResize = {}, rcAfterResize = {};
	GetWindowRect(ghWnd, &rcWndS);
	SizeInfo::RequestRect(rcWndS);

	// #START_NOTE Perhaps this shall be in OnCreate?
	OnTaskbarButtonCreated();

	// Used for restoring from Maximized/Fullscreen/Iconic. Remember current Pos/Size.
	StoreNormalRect(nullptr);

	_ASSERTE(InCreateWindow() == true);

	if (gpSet->isTabs==1)  // TabBar is always visible?
	{
		ForceShowTabs(TRUE); // Show TabBar

		// Without created real child TabBar window,
		// size evaluations were approximate
		if ((WindowMode == wmNormal) && !mp_Inside)
		{
			rcWndT = GetDefaultRect();
			//TODO: Here Quake (if selected) will be activated.
			//      This may be rather "long" operation (animation)
			//      and no RCon-s will be created until it finishes
			setWindowPos(nullptr, rcWndT.left, rcWndT.top, rcWndT.right- rcWndT.left, rcWndT.bottom- rcWndT.top, SWP_NOZORDER);
			StoreNormalRect(&rcWndT);
		}
	}

	GetWindowRect(ghWnd, &rcBeforeResize);

	// Brush up window size
	ReSize(gpSet->isIntegralSize());

	GetWindowRect(ghWnd, &rcAfterResize);

	mn_StartupFinished = ss_OnCreateFinished;

	UNREFERENCED_PARAMETER(rcWndS.left); UNREFERENCED_PARAMETER(rcWndT.left);
	UNREFERENCED_PARAMETER(rcBeforeResize.left); UNREFERENCED_PARAMETER(rcAfterResize.left);
}

// Returns Zero if unsupported, otherwise - first asSource character (type)
wchar_t CConEmuMain::IsConsoleBatchOrTask(LPCWSTR asSource)
{
	wchar_t Supported = 0;

	// If task name is quoted
	CmdArg lsTemp;
	if (asSource && (*asSource == L'"'))
	{
		if (NextArg(asSource, lsTemp))
		{
			asSource = lsTemp.c_str();
		}
	}

	if (asSource && *asSource && (
			(*asSource == CmdFilePrefix)
			|| (*asSource == TaskBracketLeft)
			|| (lstrcmp(asSource, AutoStartTaskName) == 0)
			|| (*asSource == DropLnkPrefix)
		))
	{
		Supported = *asSource;
	}
	else
	{
		_ASSERTE(asSource && *asSource);
	}

	return Supported;
}

CEStr CConEmuMain::LoadConsoleBatch(LPCWSTR asSource, RConStartArgsEx* pArgs /*= nullptr*/)
{
	if (pArgs && pArgs->pszTaskName)
	{
		SafeFree(pArgs->pszTaskName);
	}

	const wchar_t cType = IsConsoleBatchOrTask(asSource);
	if (!cType)
	{
		_ASSERTE(asSource && (*asSource==CmdFilePrefix || *asSource==TaskBracketLeft));
		return {};
	}

	// If task name is quoted
	CmdArg lsTemp;
	if (asSource && (*asSource == L'"'))
	{
		LPCWSTR pszTemp;
		if ((pszTemp = NextArg(asSource, lsTemp)))
		{
			asSource = lsTemp.c_str();

			pszTemp = SkipNonPrintable(pszTemp);
			if (pszTemp && *pszTemp)
			{
				// #Tasks Is it still true?
				_ASSERTE((!pszTemp || !*pszTemp) && "Task arguments are not supported yet");
			}
		}
	}

	CEStr result;

	switch (cType)
	{
	case CmdFilePrefix:
		// it's a "batch" file for simultaneous run of several consoles
		result = LoadConsoleBatch_File(asSource);
		break;

	case TaskBracketLeft:
	case AutoStartTaskLeft: // AutoStartTaskName
		// The Task name
		result = LoadConsoleBatch_Task(asSource, pArgs);
		break;

	case DropLnkPrefix:
		// We get here when on ConEmu (or it's desktop shortcut) user
		// drops (D&D) one or several other items.
		result = LoadConsoleBatch_Drops(asSource);
		break;

	#ifdef _DEBUG
	default:
		_ASSERTE(FALSE && "Unsupported type of Task/Batch");
	#endif
	}

	return result;
}

CEStr CConEmuMain::LoadConsoleBatch_File(LPCWSTR asSource)
{
	CEStr pszDataW;

	if (asSource && (*asSource == CmdFilePrefix))
	{
		// В качестве "команды" указан "пакетный файл" одновременного запуска нескольких консолей
		DWORD nSize = 0, nErrCode = 0;
		const int iRead = ReadTextFile(asSource + 1, (1 << 20), pszDataW, nSize, nErrCode);

		if (iRead == -1)
		{
			const CEStr szCurDir = GetCurDir();
			const CEStr szErrMsg(L"Can't open console batch file:\n" L"\x2018"/*‘*/, asSource + 1, L"\x2019"/*’*/ L"\nCurrent directory:\n" L"\x2018"/*‘*/, szCurDir, L"\x2019"/*’*/);
			DisplayLastError(szErrMsg, nErrCode);
			//Destroy(); -- must do caller
			return nullptr;
		}
		else if (iRead == -2)
		{
			const CEStr szErrMsg(L"Console batch file is too large or empty:\n" L"\x2018"/*‘*/, asSource + 1, L"\x2019"/*’*/);
			DisplayLastError(szErrMsg, nErrCode);
			//Destroy(); -- must caller
			return nullptr;
		}
		else if (iRead < 0)
		{
			const CEStr szErrMsg(L"Reading console batch file failed:\n" L"\x2018"/*‘*/, asSource + 1, L"\x2019"/*’*/);
			DisplayLastError(szErrMsg, nErrCode);
			//Destroy(); -- must caller
			return nullptr;
		}

	}
	else
	{
		_ASSERTE(FALSE && "Invalid CmdFilePrefix");
	}

	return pszDataW;
}

CEStr CConEmuMain::LoadConsoleBatch_Drops(LPCWSTR asSource)
{
	CEStr pszDataW;

	if (asSource && (*asSource == DropLnkPrefix))
	{
		// Сюда мы попадаем, если на ConEmu (или его ярлык)
		// набрасывают (в проводнике?) один или несколько других файлов/программ
		asSource++;
		if (!*asSource)
		{
			DisplayLastError(L"Empty command", (DWORD)-1);
			return nullptr;
		}

		// Считаем, что один файл (*.exe, *.cmd, ...) или ярлык (*.lnk)
		// это одна запускаемая консоль в ConEmu.
		CmdArg szPart;
		wchar_t szExe[MAX_PATH+1], szDir[MAX_PATH+1];
		HRESULT hr = S_OK;
		IShellLinkW* pShellLink = nullptr;
		IPersistFile* pFile = nullptr;
		if (StrStrI(asSource, L".lnk"))
		{
			hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&pShellLink);
			if (FAILED(hr) || !pShellLink)
			{
				DisplayLastError(L"Can't create IID_IShellLinkW", (DWORD)hr);
				return nullptr;
			}
			hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pFile);
			if (FAILED(hr) || !pFile)
			{
				DisplayLastError(L"Can't create IID_IPersistFile", (DWORD)hr);
				pShellLink->Release();
				return nullptr;
			}
		}

		INT_PTR cchArguments = MAX_WIDE_PATH_LENGTH;
		wchar_t* pszArguments = (wchar_t*)calloc(cchArguments,sizeof(wchar_t));
		if (!pszArguments)
		{
			SafeRelease(pShellLink);
			SafeRelease(pFile);
			return nullptr;
		}

		// Поехали
		CEStrConcat pszConsoles;
		pszConsoles.Reserve(MAX_CONSOLE_COUNT * 2);
		const wchar_t lineDelim[] = L"\r\n";
		size_t cchLen, cchAllLen = 0, iCount = 0;
		while ((iCount < MAX_CONSOLE_COUNT) && ((asSource = NextArg(asSource, szPart))))
		{
			if (lstrcmpi(PointToExt(szPart), L".lnk") == 0)
			{
				// Ярлык
				hr = pFile->Load(szPart, STGM_READ);
				if (SUCCEEDED(hr))
				{
					hr = pShellLink->GetPath(szExe, countof(szExe), nullptr, 0);
					if (SUCCEEDED(hr) && *szExe)
					{
						hr = pShellLink->GetArguments(pszArguments, cchArguments);
						if (FAILED(hr))
							pszArguments[0] = 0;
						hr = pShellLink->GetWorkingDirectory(szDir, countof(szDir));
						if (FAILED(hr))
							szDir[0] = 0;

						cchLen = _tcslen(szExe)+3
							+ _tcslen(pszArguments)+1
							+ (*szDir ? (_tcslen(szDir)+32) : 0); // + "-new_console:d<Dir>
						CEStr console;
						if (console.GetBuffer(cchLen))
						{
							swprintf_c(console.data(), console.GetMaxCount(), L"\"%s\"%s%s",
								Unquote(szExe), *pszArguments ? L" " : L"", pszArguments);
							if (*szDir)
							{
								console.Append(L" \"-new_console:d");
								console.Append(Unquote(szDir));
								console.Append(L"\"");
							}
							pszConsoles.Append(std::move(console));
							pszConsoles.Append(lineDelim);
							iCount++;
						}

						cchAllLen += cchLen+3;
					}
				}
			}
			else
			{
				cchLen = _tcslen(szPart) + 3;
				CEStr console;
				if (console.GetBuffer(cchLen))
				{
					swprintf_c(console.data(), console.GetMaxCount(), L"\"%s\"", szPart.c_str());
					pszConsoles.Append(std::move(console));
					pszConsoles.Append(lineDelim);
					iCount++;
				}

				cchAllLen += cchLen+3;
			}
		}

		SafeFree(pszArguments);

		if (pShellLink)
			pShellLink->Release();
		if (pFile)
			pFile->Release();

		if (iCount == 0)
			return nullptr;

		if (iCount == 1)
			pszDataW = std::move(pszConsoles.GetString(0));
		else
			pszDataW = pszConsoles.GetData();
	}
	else
	{
		_ASSERTE(FALSE && "Invalid DropLnkPrefix");
	}

	return pszDataW;
}

CEStr CConEmuMain::LoadConsoleBatch_Task(LPCWSTR asSource, RConStartArgsEx* pArgs /*= nullptr*/)
{
	CEStr pszDataW;

	if (asSource && ((*asSource == TaskBracketLeft) || (lstrcmp(asSource, AutoStartTaskName) == 0)))
	{
		CEStr szName; // Name of the task, we need copy because we trim asSource tail
		CEStr lsTail; // Additional arguments supposed to be appended to each task's command
		szName.Set(asSource);
		// Search for task name end
		wchar_t* psz = wcschr(szName.ms_Val, TaskBracketRight);
		if (psz)
		{
			lsTail.Set(SkipNonPrintable(psz+1));
			psz[1] = 0;
		}

		const CommandTasks* pGrp = gpSet->CmdTaskGetByName(szName);

		if (pGrp)
		{
			if (pArgs)
				pArgs->pszTaskName = lstrdup(szName).Detach();

			// TODO: Supposed to be appended to EACH command (task line),
			// TODO: but now lsTail may be appended to single-command tasks only
			if (pGrp->pszCommands && !wcschr(pGrp->pszCommands, L'\n'))
				pszDataW = CEStr(pGrp->pszCommands, lsTail.IsEmpty() ? nullptr : L" ", lsTail.ms_Val);
			else
				pszDataW.Set(pGrp->pszCommands);

			if (pArgs && pGrp->pszGuiArgs)
			{
				RConStartArgsEx parsedArgs;
				pGrp->ParseGuiArgs(&parsedArgs);

				if (IsStrEmpty(pArgs->pszStartupDir) && IsStrNotEmpty(parsedArgs.pszStartupDir))
					ExchangePtr(pArgs->pszStartupDir, parsedArgs.pszStartupDir);
				if (IsStrEmpty(pArgs->pszIconFile) && IsStrNotEmpty(parsedArgs.pszIconFile))
					ExchangePtr(pArgs->pszIconFile, parsedArgs.pszIconFile);

				if (IsStrNotEmpty(pArgs->pszIconFile))
				{
					// Функция сама проверит - можно или нет менять иконку приложения
					SetWindowIcon(pArgs->pszIconFile);
				}
			}
		}

		if (!pszDataW || !*pszDataW)
		{
			ssize_t cchMax = _tcslen(szName) + 100;
			CEStr pszErrMsg;
			if (pszErrMsg.GetBuffer(cchMax))
			{
				swprintf_c(pszErrMsg.data(), pszErrMsg.GetMaxCount(), L"Command group %s %s!\n"
					L"Choose your shell?",
					szName.c_str(L""), pszDataW ? L"is empty" : L"not found");
			}
			const int nBtn = MsgBox(pszErrMsg.c_str(L"Choose your shell?"), MB_YESNO | MB_ICONEXCLAMATION);

			pszErrMsg.Release();
			pszDataW.Release();

			if (nBtn == IDYES)
			{
				LPCWSTR pszDefCmd = GetDefaultCmd();

				RConStartArgsEx args;
				args.aRecreate = (mn_StartupFinished >= ss_Started) ? cra_EditTab : cra_CreateTab;
				if (pszDefCmd && *pszDefCmd)
				{
					SafeFree(args.pszSpecialCmd);
					args.pszSpecialCmd = lstrdup(pszDefCmd).Detach();
				}

				int nCreateRc = RecreateDlg(&args);

				if ((nCreateRc == IDC_START) && args.pszSpecialCmd && *args.pszSpecialCmd)
				{
					CEStr pszNewCmd;
					if ((*args.pszSpecialCmd == CmdFilePrefix) || (*args.pszSpecialCmd == TaskBracketLeft))
					{
						CEStr pszTaskName; pszTaskName.Attach(std::move(args.pszSpecialCmd));
						args.pszSpecialCmd = nullptr; // reset command as we pass args (as extra options) into LoadConsoleBatch
						pszNewCmd = LoadConsoleBatch(pszTaskName, &args);
					}
					else
					{
						pszNewCmd = args.CreateCommandLine(false);
					}
					return pszNewCmd;
				}
			}
			return nullptr;
		}

	}
	else
	{
		_ASSERTE(FALSE && "Invalid task name");
	}

	return pszDataW;
}

bool CConEmuMain::CreateStartupConsoles()
{
	BOOL lbCreated = FALSE;
	bool isScript = false;
	LPCWSTR pszCmd = GetCmd(&isScript);

	if (!pszCmd || !*pszCmd)
	{
		_ASSERTE(pszCmd != nullptr && *pszCmd != 0); // Must be!
		LogString(L"CreateStartupConsoles failed: pszCmd is null");
		return false;
	}

	if (isScript)
	{
		CEStr szDataW(pszCmd);

		// "Script" is a Task represented as one string with "|||" as command delimiter
		// Replace "|||" to "\r\n" as standard Task expects
		wchar_t* pszNext = szDataW.ms_Val;
		while ((pszNext = wcsstr(pszNext, L"|||")) != nullptr)
		{
			*(pszNext++) = L' ';
			*(pszNext++) = L'\r';
			*(pszNext++) = L'\n';
		}

		LogString(L"Creating console group using `|||` script");

		// GO
		if (!CreateConGroup(szDataW))
		{
			Destroy();
			return false;
		}

		// reset command, came from `-cmdlist` and load settings
		ResetCmdArg();

		lbCreated = TRUE;
	}
	else if ((*pszCmd == CmdFilePrefix || *pszCmd == TaskBracketLeft || lstrcmpi(pszCmd, AutoStartTaskName) == 0) && (m_StartDetached != crb_On))
	{
		RConStartArgsEx args;
		// Was "/dir" specified in the app switches?
		if (mb_ConEmuWorkDirArg)
			args.pszStartupDir = lstrdup(ms_ConEmuWorkDir).Detach();
		CEStr lsLog(L"Creating console group using task ", pszCmd);
		LogString(lsLog);
		// Here are either text file with Task contents, or just a Task name
		CEStr szDataW(LoadConsoleBatch(pszCmd, &args));
		if (szDataW.IsEmpty())
		{
			Destroy();
			return false;
		}

		// GO
		if (!CreateConGroup(szDataW, &args))
		{
			Destroy();
			return false;
		}

		lbCreated = TRUE;
	}

	if (!lbCreated && (m_StartDetached != crb_On))
	{
		RConStartArgsEx args;
		args.Detached = crb_Off;

		SafeFree(args.pszSpecialCmd);
		args.pszSpecialCmd = lstrdup(GetCmd()).Detach();

		CEStr lsLog(L"Creating console using command ", args.pszSpecialCmd);
		LogString(lsLog);

		if (!CreateCon(args, TRUE))
		{
			CEStr szFailMsg(L"Can't create new virtual console!\n"
				, L"{CConEmuMain::CreateStartupConsoles}\n"
				, L"Command: ", GetCmd()
				);
			LogString(szFailMsg);
			if (!isInsideInvalid())
			{
				DisplayLastError(szFailMsg, -1);
			}
			Destroy();
			return false;
		}
	}

	return true;
}

void CConEmuMain::OnMainCreateFinished()
{
	if (mn_StartupFinished < ss_CreateQueueReady)
	{
		_ASSERTE(mn_StartupFinished == ss_PostCreate2Called || mn_StartupFinished == ss_VConAreCreated);

		// Allow to run CRunQueue::AdvanceQueue()
		mn_StartupFinished = ss_CreateQueueReady;
	}

	// This flags has only "startup" effect
	if (m_StartDetached == crb_On)
	{
		m_StartDetached = crb_Off;  // действует только на первую консоль
	}

	if (isWindowNormal() && (gpSet->isTabs == 2) && isTabsShown())
	{
		SizeInfo::RequestRecalc();

		#ifdef _DEBUG
		RECT rcCur = {}; ::GetWindowRect(ghWnd, &rcCur);
		#endif
		RECT rcDef = GetDefaultRect();

		setWindowPos(nullptr, rcDef.left, rcDef.top, RectWidth(rcDef), RectHeight(rcDef), SWP_NOZORDER);
	}

	// Useless. RealConsoles are created from RunQueue, and here
	// ConEmu can't lose focus in in favour of RealConsole
	#if 0
	// Если фокус был у нас - вернуть его (на всякий случай, вдруг RealConsole какая забрала
	if (mp_Inside)
	{
		if ((hCurForeground == ghWnd) || (hCurForeground != mp_Inside->mh_InsideParentRoot))
			apiSetForegroundWindow(mp_Inside->mh_InsideParentRoot);
		setWindowPos(HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	}
	else if ((hCurForeground == ghWnd) && !WindowStartMinimized)
	{
		apiSetForegroundWindow(ghWnd);
	}
	#endif

	if (WindowStartMinimized)
	{
		_ASSERTE(!WindowStartNoClose || (WindowStartTsa && WindowStartNoClose));  // -V728

		if (WindowStartTsa || ForceMinimizeToTray)
		{
			Icon.HideWindowToTray();

			if (WindowStartNoClose)
			{
				LogString(L"Implied {isMultiLeaveOnClose=1 && isMultiHideOnClose=1} due to WindowStartTsa & WindowStartNoClose");
				//_ASSERTE(FALSE && "Set up {isMultiLeaveOnClose=1 && isMultiHideOnClose=1}");
				//gpSet->isMultiLeaveOnClose = 1;
				//gpSet->isMultiHideOnClose = 1;
			}
		}
		else if (IsWindowVisible(ghWnd) && !isIconic())
		{
			PostMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}

		WindowStartMinimized = false;
	}

	UpdateWin7TaskList(mb_UpdateJumpListOnStartup);
	mb_UpdateJumpListOnStartup = false;

	UINT_PTR n = SetKillTimer(true, TIMER_MAIN_ID, TIMER_MAIN_ELAPSE/*gpSet->nMainTimerElapse*/);
	DEBUGTEST(DWORD dw = GetLastError());
	n = SetKillTimer(true, TIMER_CONREDRAW_ID, CON_REDRAW_TIMOUT * 2);
	UNREFERENCED_PARAMETER(n);
	OnActivateSplitChanged();

	if (gpSet->isTaskbarOverlay && gpSet->isWindowOnTaskBar())
	{
		// Bug in Win7? Sometimes after startup ‘Overlay icon’ was not appeared.
		mn_TBOverlayTimerCounter = 0;
		SetKillTimer(true, TIMER_ADMSHIELD_ID, TIMER_ADMSHIELD_ELAPSE);
	}

	// Start RCon Queue
	mp_RunQueue->StartQueue();

	// Check if DefTerm is enabled
	mp_DefTrm->StartGuiDefTerm(false);

	ExecPostGuiMacro();

	mp_PushInfo = new CPushInfo();
	if (mp_PushInfo && !mp_PushInfo->ShowNotification())
	{
		SafeDelete(mp_PushInfo);
	}

	if (mb_SettingsRequested)
	{
		// CSettings::Dialog();
		PostMessage(ghWnd, WM_SYSCOMMAND, ID_SETTINGS, 0);
	}
}

void CConEmuMain::PostCreate(bool abReceived/*=FALSE*/)
{
	if (gpSet->isLogging())
	{
		wchar_t sInfo[100];
		swprintf_c(sInfo,
			L"PostCreate.%u.begin (LastTabClose=%u CrossClick=%u DoMin=%u DoHide=%u)",
			abReceived ? 2 : 1,
			(UINT)gpSet->isCloseOnLastTabClose(), (UINT)gpSet->isCloseOnCrossClick(),
			(UINT)gpSet->isMinOnLastTabClose(), (UINT)gpSet->isHideOnLastTabClose()
			);
		LogString(sInfo);
	}

	mn_StartupFinished = abReceived ? ss_PostCreate2Called : ss_PostCreate1Called;

	if (!abReceived)
	{
		#ifdef MSGLOGGER
		WINDOWPLACEMENT wpl; memset(&wpl, 0, sizeof(wpl)); wpl.length = sizeof(wpl);
		GetWindowPlacement(ghWnd, &wpl);
		#endif

		CheckActiveLayoutName();

		if (gpSet->isLogging())
		{
			session.SetSessionNotification(true);
		}

		if (gpSet->isHideCaptionAlways())
		{
			RefreshWindowStyles();
		}

		if (opt.DesktopMode)
			DoDesktopModeSwitch();

		SetWindowMode((ConEmuWindowMode)gpSet->_WindowMode, FALSE, TRUE);

		PostMessage(ghWnd, mn_MsgPostCreate, 0, 0);

		if (!mh_RightClickingBmp)
		{
			mh_RightClickingBmp = (HBITMAP)LoadImage(g_hInstance, MAKEINTRESOURCE(IDB_RIGHTCLICKING),
			                      IMAGE_BITMAP, 0,0, LR_CREATEDIBSECTION);

			if (mh_RightClickingBmp)
			{
				BITMAP bi = {0};

				if (GetObject(mh_RightClickingBmp, sizeof(bi), &bi)
				        && bi.bmWidth > bi.bmHeight && bi.bmHeight > 0)
				{
					m_RightClickingSize.x = bi.bmWidth; m_RightClickingSize.y = bi.bmHeight;
				}
				else
				{
					m_RightClickingSize.x = 384; m_RightClickingSize.y = 16;
				}

				_ASSERTE(m_RightClickingSize.y>0);
				m_RightClickingFrames = m_RightClickingSize.x / m_RightClickingSize.y;
				// по идее, должно быть ровно
				_ASSERTE(m_RightClickingSize.x == m_RightClickingFrames * m_RightClickingSize.y);
			}
		}
	}
	else // (abReceived == true)
	{
		HWND hCurForeground = GetForegroundWindow();

		Taskbar_Init();

		GetGlobalHotkeys().RegisterMinRestore(true);

		GetGlobalHotkeys().RegisterHotKeys();

		// Если вдруг оказалось, что хоткей (из ярлыка) назначился для
		// окна отличного от главного
		if (!gnWndSetHotkeyOk && gnWndSetHotkey)
		{
			if (ghWndApp)
			{
				SendMessage(ghWndApp, WM_SETHOTKEY, 0, 0);
			}
			SendMessage(ghWnd, WM_SETHOTKEY, gnWndSetHotkey, 0);
		}

		//TODO: Возможно, стоит отложить запуск секунд на 5, чтобы не мешать инициализации?
		if (!opt.DisableAutoUpdate
			&& (gpSet->UpdSet.isUpdateCheckOnStartup || opt.AutoUpdateOnStart)
			)
		{
			CheckUpdates(UpdateCallMode::Automatic); // Don't show "You are using latest available version"
		}

		if (gpFontMgr->ms_FontError && !(gpStartEnv && ((gpStartEnv->bIsWinPE == 1) || (gpStartEnv->bIsWine == 1))))
		{
			MBoxA(gpFontMgr->ms_FontError);
			gpFontMgr->ms_FontError.Release();
		}

		if (!gpSetCls->CheckConsoleFontFast())
		{
			DontEnable de;
			gpSetCls->EditConsoleFont(ghWnd);
		}

		// Если в ключе [HKEY_CURRENT_USER\Console] будут левые значения - то в Win7 могут
		// начаться страшные глюки :-)
		// например, консольное окно будет "дырявое" - рамка есть, а содержимого - нет :-P
		gpSet->CheckConsoleSettings();

		if (gpSet->isShowBgImage)
		{
			gpSetCls->LoadBackgroundFile(gpSet->sBgImage);
		}

		// Перенес перед созданием консоли, чтобы не блокировать SendMessage
		// (mp_DragDrop->Register() может быть относительно длительной операцией)
		if (!mp_DragDrop)
		{
			// было 'ghWnd DC'. Попробуем на главное окно, было бы удобно
			// "бросать" на таб (с автоматической активацией консоли)
			mp_DragDrop = new CDragDrop();

			if (!mp_DragDrop->Register())
			{
				CDragDrop *p = mp_DragDrop; mp_DragDrop = nullptr;
				delete p;
			}
		}

		// Может быть в настройке указано - всегда показывать иконку в TSA
		Icon.SettingsChanged();

		// Проверка уникальности хоткеев
		gpSet->CheckHotkeyUnique();


		if (!isVConExists(0) || (m_StartDetached != crb_On))  // Консоль уже может быть создана, если пришел Attach из ConEmuC
		{
			if (!CreateStartupConsoles())
			{
				return;
			}
		}

		// Finalize window creation
		OnMainCreateFinished();
	}

	if (!abReceived)
		mn_StartupFinished = ss_PostCreate1Finished;
	else if (isVConExists(0) || (mn_StartupFinished == ss_VConAreCreated))
		mn_StartupFinished = ss_VConStarted;
	else
		mn_StartupFinished = ss_PostCreate2Finished /* == ss_Started */;

	if (gpSet->isLogging())
	{
		wchar_t szInfo[64];
		swprintf_c(szInfo, L"PostCreate.%i.end", abReceived ? 2 : 1);
		LogWindowPos(szInfo);
	}
}

// Это не Post, а Send. Синхронное создание окна в главной нити
HWND CConEmuMain::PostCreateView(CConEmuChild* pChild)
{
	HWND hChild = nullptr;
	if (GetCurrentThreadId() == mn_MainThreadId)
	{
		hChild = pChild->CreateView();
	}
	else
	{
		hChild = (HWND)SendMessage(ghWnd, mn_MsgCreateViewWindow, 0, reinterpret_cast<LPARAM>(pChild));
	}
	return hChild;
}

// Processes WM_DESTROY message
LRESULT CConEmuMain::OnDestroy(HWND hWnd)
{
	if (!this)
		return 0;
	wchar_t szLog[80]; swprintf_c(szLog, L"WM_DESTROY: CConEmuMain hWnd=x%08X OurDestroy=%i", LODWORD(hWnd), mn_InOurDestroy);
	if (mp_Log) { LogString(szLog); } else { DEBUGSTRDESTROY(szLog); }

	mn_StartupFinished = ss_Destroying;

	DeinitOnDestroy(hWnd);

	PostQuitMessage(0);

	// After WM_DESTROY (especially with InsideParent mode)
	// ghWnd changes into invalid state
	mn_StartupFinished = ss_Destroyed;
	if (mn_InOurDestroy == 0)
		ghWnd = nullptr;
	return 0;
}

void CConEmuMain::DestroyAllChildWindows()
{
	if (!ghWnd)
		return;
	_ASSERTE(mn_InOurDestroy==0);

	struct Impl {
		MArray<HWND> lhChildren;
		static BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
		{
			Impl* p = (Impl*)lParam;
			p->lhChildren.push_back(hwnd);
			return TRUE;
		}
	} impl;
	EnumChildWindows(ghWnd, Impl::EnumChildProc, (LPARAM)&impl);

	HWND hChild = nullptr;
	while (impl.lhChildren.pop_back(hChild))
	{
		DestroyWindow(hChild);
	}
}

LRESULT CConEmuMain::OnFlashWindow(WPARAM wParam, LPARAM lParam)
{
	DWORD nOpt = ((DWORD)wParam & 0x00F00000) >> 20;
	DWORD nFlags = ((DWORD)wParam & 0xFF000000) >> 24;
	DWORD nCount = (DWORD)wParam & 0xFFFFF;
	CVConGroup::OnFlashWindow(nOpt, nFlags, nCount, (HWND)lParam);

	return 0;
}

void CConEmuMain::DoFlashWindow(CESERVER_REQ_FLASHWINFO* pFlash, bool bFromMacro)
{
	WPARAM wParam = 0;

	if (pFlash->fType == eFlashSimple)
	{
		wParam = 0x00100000 | (pFlash->bInvert ? 0x00200000 : 0) | (bFromMacro ? 0x00400000 : 0);
	}
	else
	{
		wParam = ((pFlash->dwFlags & 0xFF) << 24) | (pFlash->uCount & 0xFFFFF)
			| ((pFlash->fType == eFlashBeep) ? 0x00800000 : 0)
			| (bFromMacro ? 0x00400000 : 0);
	}

	PostMessage(ghWnd, mn_MsgFlashWindow, wParam, static_cast<LPARAM>(pFlash->hWnd.u));
}

void CConEmuMain::setFocus()
{
#ifdef _DEBUG
	DWORD nSelfPID = GetCurrentProcessId();
	wchar_t sFore[1024], sFocus[1024]; sFore[0] = sFocus[0] = 0;
	HWND hFocused = GetFocus();
	DWORD nFocusPID = 0; if (hFocused) { GetWindowThreadProcessId(hFocused, &nFocusPID); getWindowInfo(hFocused, sFocus); }
	HWND hFore = GetForegroundWindow();
	DWORD nForePID = 0; if (hFore) { GetWindowThreadProcessId(hFore, &nForePID); getWindowInfo(hFore, sFore); }
	bool bNeedFocus = false;
	if (hFocused != ghWnd)
		bNeedFocus = true;
	CVConGuard VCon;
	HWND hGuiWnd = (GetActiveVCon(&VCon) >= 0) ? VCon->GuiWnd() : nullptr;

	// -- ассерт убрал, может возникать при переключении на консоль (табы)
	//_ASSERTE(hGuiWnd==nullptr || !IsWindowVisible(hGuiWnd));
#endif

	SetFocus(ghWnd);
}

bool CConEmuMain::SetSkipOnFocus(bool abSkipOnFocus)
{
	bool bPrev = mb_SkipOnFocus;
	mb_SkipOnFocus = abSkipOnFocus;
	return bPrev;
}

void CConEmuMain::OnOurDialogOpened()
{
	mb_AllowAutoChildFocus = false;
}

void CConEmuMain::OnOurDialogClosed()
{
	CheckAllowAutoChildFocus((DWORD)-1);
}

bool CConEmuMain::isMenuActive()
{
	return (mp_Menu->GetTrackMenuPlace() != tmp_None);
}

bool CConEmuMain::CanSetChildFocus()
{
	if (isMenuActive())
		return false;
	return true;
}

void CConEmuMain::CheckAllowAutoChildFocus(DWORD nDeactivatedTID)
{
	if (!gpSet->isFocusInChildWindows)
	{
		mb_AllowAutoChildFocus = false;
		return;
	}

	bool bAllowAutoChildFocus = false;
	CVConGuard VCon;
	HWND hChild = nullptr;

	if (GetActiveVCon(&VCon) >= 0
		&& ((hChild = VCon->GuiWnd()) != nullptr))
	{
		if (nDeactivatedTID == (DWORD)-1)
		{
			bAllowAutoChildFocus = true;
		}
		// просто так фокус в дочернее окно ставить нельзя
		// если переключать фокус в дочернее приложение по любому чиху
		// вообще не получается активировать окно ConEmu, открыть системное меню,
		// клацнуть по крестику в заголовке и т.п.
		else if (nDeactivatedTID)
		{
			DWORD nChildTID = GetWindowThreadProcessId(hChild, nullptr);
			DWORD nConEmuTID = GetWindowThreadProcessId(ghWnd, nullptr);

			if ((nDeactivatedTID != nChildTID)
				&& (nDeactivatedTID != nConEmuTID)) // это на всякий случай...
			{
				// isMeForeground(true, true) -- уже дает неправильный результат, ConEmu считается активным
				bAllowAutoChildFocus = true;
			}
		}
	}

	mb_AllowAutoChildFocus = bAllowAutoChildFocus;
}

bool CConEmuMain::IsChildFocusAllowed(HWND hChild)
{
	DWORD dwStyle = GetWindowLong(hChild, GWL_STYLE);

	if ((dwStyle & (WS_POPUP|WS_OVERLAPPEDWINDOW|WS_DLGFRAME)) != 0)
		return true; // Это диалог, не трогаем

	if (dwStyle & WS_CHILD)
	{
		wchar_t szClass[200] = L"";
		if (GetClassName(hChild, szClass, countof(szClass)))
		{
			if (lstrcmp(szClass, L"EDIT") == 0)
				return true; // Контрол поиска, и пр.
		}
	}

	return false;
}

LRESULT CConEmuMain::OnFocus(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, LPCWSTR asMsgFrom /*= nullptr*/, bool abForceChild /*= FALSE*/)
{
	// Чтобы избежать лишних вызовов по CtrlWinAltSpace при работе с GUI приложением
	if (mb_SkipOnFocus)
	{
		return 0;
	}

	bool lbSetFocus = false;
	#ifdef _DEBUG
	DWORD lnForceTimerCheckLoseFocus = mn_ForceTimerCheckLoseFocus;
	#endif

	WCHAR szDbg[128];

	LPCWSTR pszMsgName = L"Unknown";
	HWND hNewFocus = nullptr;
	HWND hForeground = nullptr;
	bool bSkipQuakeActivation = false;

	if (messg == WM_SETFOCUS)
	{
		lbSetFocus = true;
		swprintf_c(szDbg, L"WM_SETFOCUS(From=0x%08X)", LODWORD(wParam));
		LogFocusInfo(szDbg);
		pszMsgName = L"WM_SETFOCUS";
	}
	else if (messg == WM_ACTIVATE)
	{
		lbSetFocus = (LOWORD(wParam)==WA_ACTIVE) || (LOWORD(wParam)==WA_CLICKACTIVE);

		if (LOWORD(wParam)==WA_INACTIVE)
		{
			hForeground = GetForegroundWindow();
			hNewFocus = lParam ? reinterpret_cast<HWND>(lParam) : hForeground;
			if (CVConGroup::isOurWindow(hNewFocus))
			{
				lbSetFocus = true; // Считать, что фокус мы не теряем!

				// Клик мышкой в области дочернего GUI-приложения, запущенного в ConEmu
				// isPressed - не подходит
				if (!CVConGroup::InCreateGroup())
				{
					POINT ptCur = {}; GetCursorPos(&ptCur);
					for (int i = 0;; i++)
					{
						CVConGuard vCon;
						if (!CVConGroup::GetVCon(i, &vCon))
							break;
						if (!vCon->isVisible())
							continue;

						HWND hGuiWnd = vCon->GuiWnd();
						RECT rcChild = {}; GetWindowRect(hGuiWnd, &rcChild);
						if (PtInRect(&rcChild, ptCur))
						{
							Activate(vCon.VCon());
						}
					}
				}
			}
			else if (hNewFocus)
			{
				// Запускается новая консоль в режиме "администратора"?
				if (CVConGroup::isInCreateRoot())
				{
					if (IsConsoleWindow(hNewFocus))
					{
						lbSetFocus = true;
					}
				}
			}
		}

		if (!lbSetFocus && opt.DesktopMode && mh_ShellWindow)
		{
			if (isPressed(VK_LBUTTON) || isPressed(VK_RBUTTON))
			{
				// При активации _Десктопа_ мышкой - запомнить, что ставить фокус в себя не нужно
				POINT ptCur; GetCursorPos(&ptCur);
				HWND hFromPoint = WindowFromPoint(ptCur);

				//if (hFromPoint == mh_ShellWindow) TODO: если сразу совпало - сразу выходим

				// mh_ShellWindow чисто для информации. Хоть родитель ConEmu и меняется на mh_ShellWindow
				// но проводник может перекинуть наше окно в другое (WorkerW или Progman)
				if (hFromPoint)
				{
					bool lbDesktopActive = false;
					wchar_t szClass[128];

					// Нужно учесть, что еще могут быть всякие бары, панели, и прочее, лежащие на десктопе
					while (hFromPoint)
					{
						if (GetClassName(hFromPoint, szClass, 127))
						{
							if (!wcscmp(szClass, L"WorkerW") || !wcscmp(szClass, L"Progman"))
							{
								DWORD dwPID;
								GetWindowThreadProcessId(hFromPoint, &dwPID);
								lbDesktopActive = (dwPID == mn_ShellWindowPID);
								break;
							}
						}

						// может таки что-то дочернее попалось?
						hFromPoint = GetParent(hFromPoint);
					}

					if (lbDesktopActive)
						mb_FocusOnDesktop = FALSE;
				}
			}
		}
		else if (lbSetFocus && gpSet->isQuakeStyle && isQuakeMinimized)
		{
			bool lbSkipQuakeActivation = false;
			if ((LOWORD(wParam) == WA_ACTIVE) && (HIWORD(wParam) != 0))
			{
				// Сюда мы попадаем если "активируется" кнопка на таскбаре, но без разворачивания окна
				// Пример. Наше окно единственное, жмем кнопку минимизации в тулбаре,
				// Quake сворачивается, но т.к. других окон нет - фокус "остается" в нашем
				// окне (в кнопке на таскбаре) и приходит WM_ACTIVATE.
				// Здесь его нужно игнорировать, иначе Quake нифига не "свернется"
				lbSkipQuakeActivation = true;
			}
			else if (mn_IgnoreQuakeActivation > 0)
			{
				// Правый клик на иконке в TSA, тоже НЕ выезжать
				lbSkipQuakeActivation = true;
			}

			if (!lbSkipQuakeActivation)
			{
				// Обработать клик по кнопке на таскбаре
				DoMinimizeRestore(sih_ShowMinimize);
			}
		}

		if (LOWORD(wParam)==WA_ACTIVE)
			swprintf_c(szDbg, L"WM_ACTIVATE(From=0x%08X)", (DWORD)lParam);
		else
			swprintf_c(szDbg, L"WM_ACTIVATE.WA_INACTIVE(To=0x%08X) OurFocus=%i", (DWORD)lParam, lbSetFocus);
		LogFocusInfo(szDbg);

		pszMsgName = L"WM_ACTIVATE";
	}
	else if (messg == WM_ACTIVATEAPP)
	{
		lbSetFocus = (wParam!=0);
		if (!lbSetFocus)
		{
			hNewFocus = hForeground = GetForegroundWindow();
		}

		if (lbSetFocus)
			swprintf_c(szDbg, L"WM_ACTIVATEAPP.Activate(FromTID=%i)", (DWORD)lParam);
		else
			swprintf_c(szDbg, L"WM_ACTIVATEAPP.Deactivate(ToTID=%i) OurFocus=%i", (DWORD)lParam, lbSetFocus);
		LogFocusInfo(szDbg);

		if (!lbSetFocus)
		{
			if (CVConGroup::isOurWindow(hNewFocus))
			{
				lbSetFocus = true; // Считать, что фокус мы не теряем!
			}
		}
		else if (gpSet->isQuakeStyle)
		{
			if (mn_IgnoreQuakeActivation <= 0)
			{
				bSkipQuakeActivation = true;
			}
			else if (IsWindowVisible(ghWnd))
			{
				// Если в активном табе сидит ChildGui (popup, а-ля notepad) то
				// при попытке активации любого нашего диалога (Settings, NewCon, подтверждения закрытия)
				// начинается зоопарк с активацией/выезжанием окна
				CVConGuard VCon;
				if ((GetActiveVCon(&VCon) >= 0) && (VCon->GuiWnd()))
				{
					bSkipQuakeActivation = true;
				}
			}

			if (!bSkipQuakeActivation)
			{
				DoMinimizeRestore(sih_Show);
			}
		}

		pszMsgName = L"WM_ACTIVATEAPP";
	}
	else if (messg == WM_KILLFOCUS)
	{
		pszMsgName = L"WM_KILLFOCUS";
		hForeground = GetForegroundWindow();
		hNewFocus = wParam ? (HWND)wParam : hForeground;

		if (m_Foreground.bCaretCreated)
		{
			DestroyCaret();
			m_Foreground.bCaretCreated = false;
		}

		if (CVConGroup::isOurWindow(hNewFocus))
		{
			lbSetFocus = true; // Считать, что фокус мы не теряем!
		}

		swprintf_c(szDbg, L"WM_KILLFOCUS(To=0x%08X) OurFocus=%i", (DWORD)wParam, lbSetFocus);
		LogFocusInfo(szDbg);

	}
	else
	{
		_ASSERTE(messg == 0 && asMsgFrom != nullptr);
		if (asMsgFrom)
			pszMsgName = asMsgFrom;

		if (opt.DesktopMode)
		{
			CheckFocus(pszMsgName);
		}

		if (opt.DesktopMode || mp_Inside)
		{
			// В этих режимах игнорировать
			return 0;
		}

		hNewFocus = GetForegroundWindow();

		lbSetFocus = CVConGroup::isOurWindow(hNewFocus);

		if (mb_LastTransparentFocused != lbSetFocus)
		{
			OnTransparentSeparate(lbSetFocus);
		}

		if (!lbSetFocus && (asMsgFrom == gsFocusQuakeCheckTimer)
			&& mn_ForceTimerCheckLoseFocus
			//&& ((GetTickCount() - mn_ForceTimerCheckLoseFocus) >= QUAKE_FOCUS_CHECK_TIMER_DELAY)
			)
		{
			// сброс, однократная проверка для "Hide on focus lose"
			mn_ForceTimerCheckLoseFocus = 0;
		}
		else if (lbSetFocus == mb_LastConEmuFocusState)
		{
			// Logging
			bool bNeedLog = RELEASEDEBUGTEST((gpSet->isLogging(2)!=0),true);
			if (bNeedLog)  // -V575
			{
				HWND hFocus = GetFocus();
				wchar_t szInfo[128];
				swprintf_c(szInfo, L"Focus event skipped, lbSetFocus=%u, mb_LastConEmuFocusState=%u, ghWnd=x%08X, hFore=x%08X, hFocus=x%08X", lbSetFocus, mb_LastConEmuFocusState, LODWORD(ghWnd), LODWORD(hNewFocus), LODWORD(hFocus));
				LogFocusInfo(szInfo, 2);
			}

			// Не дергаться
			return 0;
		}

		// Сюда мы доходим, если по каким-то причинам (ChildGui?) ConEmu не получил
		// информации о том, что его окно было [де]активировано. Нужно уведомить сервер.
	}


	CheckFocus(pszMsgName);

	CVirtualConsole* pVCon = nullptr;
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
		pVCon = VCon.VCon();

	HWND hGuiWnd = pVCon ? pVCon->RCon()->GuiWnd() : nullptr; // GUI режим (дочернее окно)

	// Если фокус "забрало" какое-либо дочернее окно в ConEmu (VideoRenderer - 'ActiveMovie Window')
	if (hNewFocus && hNewFocus != ghWnd && hNewFocus != hGuiWnd)
	{
		HWND hParent = hNewFocus;

		while ((hParent = GetParent(hNewFocus)) != nullptr)
		{
			if (IsChildFocusAllowed(hNewFocus))
				break; // не трогаем
			if (hParent == hGuiWnd)
				break;
			else if (hParent == ghWnd)
			{
				setFocus();
				hNewFocus = GetFocus();

				if (hNewFocus != ghWnd)
				{
					_ASSERTE(hNewFocus == ghWnd);
				}
				else
				{
					LogFocusInfo(L"Focus was returned to ConEmu");
				}

				lbSetFocus = (hNewFocus == ghWnd);
				break;
			}

			hNewFocus = hParent;
		}
	}

	if (lbSetFocus && !hNewFocus && pVCon && mp_TabBar)
	{
		if (mp_TabBar->IsSearchShown(true))
		{
			hNewFocus = mp_TabBar->ActivateSearchPane(true);
		}
	}

	if (!lbSetFocus && !hNewFocus && (isPressed(VK_LBUTTON) || isPressed(VK_RBUTTON)))
	{
		// Клик по таскбару, или еще кому...
		POINT ptCur = {}; GetCursorPos(&ptCur);
		HWND hUnderMouse = WindowFromPoint(ptCur);
		DWORD nPID = 0;
		if (hUnderMouse && GetWindowThreadProcessId(hUnderMouse, &nPID) && (nPID != GetCurrentProcessId()))
		{
			hNewFocus = hUnderMouse;
		}
	}

	// GUI client?
	if (!lbSetFocus && hNewFocus)
	{
		DWORD nNewPID = 0; GetWindowThreadProcessId(hNewFocus, &nNewPID);
		if (nNewPID == GetCurrentProcessId())
		{
			lbSetFocus = true;
		}
		else if (pVCon)
		{
			if (hNewFocus == pVCon->RCon()->ConWnd())
			{
				lbSetFocus = true;
			}
			else
			{

				DWORD nRConPID = pVCon->RCon()->GetActivePID();
				if (nNewPID == nRConPID)
					lbSetFocus = true;
			}
		}
	}



	/****************************************************************/
	/******      Done, Processing Activation/Deactivation      ******/
	/****************************************************************/

	UpdateGuiInfoMappingActive(lbSetFocus);

	mb_LastConEmuFocusState = lbSetFocus;
	LogFocusInfo(lbSetFocus ? L"mb_LastConEmuFocusState set to TRUE" : L"mb_LastConEmuFocusState set to FALSE");


	// Если для активного и НЕактивного окна назначены разные значения
	OnTransparentSeparate(lbSetFocus);

	//
	if (!lbSetFocus)
	{
		if (pVCon)
			FixSingleModifier(0, pVCon->RCon());
		mp_TabBar->SwitchRollback();
		GetGlobalHotkeys().UnRegisterHotKeys();
		// Если работает ChildGui - значит был хак с активностью рамки ConEmu
		if (pVCon && pVCon->GuiWnd())
		{
			SetFrameActiveState(false);
		}
	}
	else
	{
		GetGlobalHotkeys().RegisterHotKeys();
	}

	if (pVCon && pVCon->RCon())
	{
		// Здесь будет вызван CRealConsole::UpdateServerActive

		// -- просто так abForceChild=TRUE здесь ставить нельзя
		// -- если переключать фокус в дочернее приложение по любому чиху
		// -- вообще не получается активировать окно ConEmu, открыть системное меню,
		// -- клацнуть по крестику в заголовке и т.п.
		// -- Поэтому abForceChild передается из вызывающей функции, которая "знает"

		pVCon->RCon()->OnGuiFocused(lbSetFocus, abForceChild);
	}

	if (gpSet->isMinimizeOnLoseFocus()
		&& (!lbSetFocus && !isMeForeground(true,true)) && !isIconic()
		&& IsWindowVisible(ghWnd))
	{
		wchar_t szClassName[255]; szClassName[0] = 0;
		bool bIsTaskbar = false, bIsTrayNotifyWnd = false;
		HWND h = hNewFocus;

		if (messg != 0)
		{
			while (h)
			{
				GetClassName(h, szClassName, countof(szClassName));
				if (lstrcmpi(szClassName, L"Shell_TrayWnd") == 0)
				{
					bIsTaskbar = true;
					break;
				}
				else if (lstrcmpi(szClassName, L"TrayNotifyWnd") == 0)
				{
					bIsTrayNotifyWnd = true;
				}
				h = GetParent(h);
			}
		}

		if (RELEASEDEBUGTEST(gpSet->isLogging(),true))
		{
			wchar_t szInfo[512];
			swprintf_c(szInfo, L"MinimizeOnFocusLose: Class=%s, IsTaskbar=%u, IsTrayNotifyWnd=%u, Timer=%u",
				szClassName, bIsTaskbar, bIsTrayNotifyWnd, mn_ForceTimerCheckLoseFocus);
			LogFocusInfo(szInfo);
		}

		// Если активируется TaskBar - не дергаться.
		// Иначе получается бред при клике на иконку в TSA.
		if (!bIsTaskbar)
		{
			if (mn_IgnoreQuakeActivation <= 0)
			{
				DoMinimizeRestore(sih_AutoHide);
			}
		}
		else if (!mn_ForceTimerCheckLoseFocus)
		{
			mn_ForceTimerCheckLoseFocus = GetTickCount();
			_ASSERTE(mn_ForceTimerCheckLoseFocus!=0);
			SetKillTimer(true, TIMER_QUAKE_AUTOHIDE_ID, TIMER_QUAKE_AUTOHIDE_ELAPSE);
		}
	}

	if (gpSet->isFadeInactive)
	{
		if (pVCon)
		{
			bool bForeground = lbSetFocus || isMeForeground();
			bool bLastFade = (pVCon!=nullptr) ? pVCon->mb_LastFadeFlag : false;
			bool bNewFade = (gpSet->isFadeInactive && !bForeground && !isPictureView());

			if (bLastFade != bNewFade)
			{
				if (pVCon) pVCon->mb_LastFadeFlag = bNewFade;

				pVCon->Invalidate();
			}
		}

		mp_Status->UpdateStatusBar(true);
	}

	// Может настройки ControlPanel изменились?
	mouse.ReloadWheelScroll();

	if (lbSetFocus && pVCon && gpSet->isTabsOnTaskBar())
	{
		HWND hFore = GetForegroundWindow();
		if (hFore == ghWnd)
		{
			if (!mb_PostTaskbarActivate)
			{
				//mp_ VActive->OnTaskbarFocus();
				mb_PostTaskbarActivate = TRUE;
				PostMessage(ghWnd, mn_MsgPostTaskbarActivate, 0, 0);
			}
		}
	}

	if (gpSet->isSkipFocusEvents)
		return 0;

#ifdef MSGLOGGER
	/*if (messg == WM_ACTIVATE && wParam == WA_INACTIVE) {
		WCHAR szMsg[128]; _wsprintf(szMsg, countof(szMsg), L"--Deactivating to 0x%08X\n", lParam);
		DEBUGSTR(szMsg);
	}
	switch (messg) {
		case WM_SETFOCUS:
		{
			DEBUGSTR(L"--Gets focus\n");
			//return 0;
		} break;
		case WM_KILLFOCUS:
		{
			DEBUGSTR(L"--Loses focus\n");
		} break;
	}*/
#endif

	if (pVCon /*&& (messg == WM_SETFOCUS || messg == WM_KILLFOCUS)*/)
	{
		pVCon->RCon()->OnFocus(lbSetFocus);
	}

	return 0;
}

void CConEmuMain::RefreshWindowStyles()
{
	if (m_JumpMonitor.bInJump)
	{
		if (gpSet->isLogging(2)) LogString(L"OnHideCaption skipped due to m_JumpMonitor.bInJump");
		return;
	}

	DEBUGTEST(bool bHideCaption = gpConEmu->isCaptionHidden());

	mp_TabBar->OnCaptionHidden();

	//if (isZoomed())
	//{
	//	SetWindowMode(wmMaximized, TRUE);
	//}

	DWORD nStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD nNewStyle = FixWindowStyle(nStyle, WindowMode);
	DWORD nStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	bool bNeedCorrect = (!isIconic() && (WindowMode != wmFullScreen) && !isInside()) && !m_QuakePrevSize.bWaitReposition;
	bool bChanged = false;

	if (bNeedCorrect || (nNewStyle != nStyle))
	{
		// Make debug output too
		{
			wchar_t szInfo[128];
			swprintf_c(szInfo, L"Changing main window 0x%08X style Cur=x%08X New=x%08X ExStyle=x%08X", LODWORD(ghWnd), nStyle, nNewStyle, nStyleEx);
			if (!LogString(szInfo)) { DEBUGSTRSTYLE(szInfo); }
		}

		MSetter lSet(&mn_IgnoreSizeChange);

		_ASSERTE(!isInside());

		// coordinates relative to upper-left screen corner
		RECT rcClient = {}, rcBefore = {}, rcAfter = {};
		if (bNeedCorrect)
		{
			// AdjustWindowRectEx НЕ должно вызываться в FullScreen. Глючит. А рамки с заголовком нет, расширять нечего.
			_ASSERTE(!isFullScreen() || (WindowMode==wmNormal || WindowMode==wmMaximized));

			RECT rcBefore = SizeInfo::WindowRect();
			RECT rcClient = SizeInfo::ClientRect();
			MapWindowPoints(ghWnd, nullptr, (LPPOINT)&rcClient, 2);
			RECT rcFrame = CalcMargins(CEM_FRAMECAPTION);
			RECT rcAfter = rcClient;
			AddMargins(rcAfter, rcFrame, rcop_Enlarge);
			// When switching from "No caption" to "Caption"
			if (IsSizeFree())
			{
				// we need to check window location, caption must not be "Out-of-screen"
				FixWindowRect(rcAfter, CEB_ALL);
			}

			if (rcBefore != rcAfter)
			{
				SizeInfo::RequestRect(rcAfter);
				m_FixPosAfterStyle = GetTickCount();
				mrc_FixPosAfterStyle = rcAfter;
				bChanged = true;
			}
			else
			{
				bNeedCorrect = false;
			}
		}

		if (!bNeedCorrect)
		{
			m_FixPosAfterStyle = 0;
		}

		if (nNewStyle != nStyle)
		{
			MSetter inChange(&mb_InCaptionChange);
			SetWindowStyle(nNewStyle);
			bChanged = true;
		}
	}

	_ASSERTE(mn_IgnoreSizeChange>=0);

	if (bChanged && IsWindowVisible(ghWnd) && !isIconic())
	{
		// If we are changing window mode (e.g. Max->Normal)
		// user may see drawing artifacts during resize
		// But it's required in some cases, e.g. if "Hide caption when maximized"
		//_ASSERTE(changeFromWindowMode == wmNotChanging);
		MSetter lSet2(&mn_IgnoreSizeChange);
		// Refresh JIC
		RedrawFrame();
		// Status bar and others
		Invalidate(nullptr, TRUE);
	}

	if (changeFromWindowMode == wmNotChanging)
	{
		UpdateWindowRgn();
		ReSize();
	}

	GetGlobalHotkeys().UpdateWinHookSettings();

	//OnTransparent();
}

void CConEmuMain::ReloadEnvironmentVariables()
{
	const wchar_t dbg_msg[] = L"Reloading environment variables from system registry";
	if (!LogString(dbg_msg))
	{
		DEBUGSTRSETCHANGE(dbg_msg);
	}


	auto new_environment = std::make_shared<SystemEnvironment>();
	new_environment->LoadFromRegistry();

	const auto& old_data = saved_environment_->env_data;
	const auto& new_data = new_environment->env_data;
	for (const auto& old_var : old_data)
	{
		if (!new_data.count(old_var.first))
		{
			std::wstring dbg_log(L"  Erased: `" + old_var.second.name
				+ L"`, old value: `" + old_var.second.data + L"`");
			if (!LogString(dbg_log.c_str()))
			{
				DEBUGSTRSETCHANGE(dbg_log.c_str());
			}

			SetEnvironmentVariable(old_var.second.name.c_str(), nullptr);
		}
	}
	for (const auto& new_var : new_data)
	{
		auto old_iter = old_data.find(new_var.first);
		if (old_iter == old_data.end()
			|| (old_iter->second.data != new_var.second.data))
		{
			std::wstring old_value((old_iter == old_data.end())
					? std::wstring(L"Absent")
					: std::wstring(L"`" + old_iter->second.data + L"`"));
			std::wstring dbg_log(L"  Changed: `" + new_var.second.name
				+ L"`, new value: `" + new_var.second.data + L"`, old_value: "
				+ old_value);
			if (!LogString(dbg_log.c_str()))
			{
				DEBUGSTRSETCHANGE(dbg_log.c_str());
			}

			if (new_var.second.expandable)
			{
				CEStr value(ExpandEnvStr(new_var.second.data.c_str()));
				if (!value.IsEmpty())
				{
					SetEnvironmentVariable(new_var.second.name.c_str(), value);
					continue;
				}
			}
			SetEnvironmentVariable(new_var.second.name.c_str(), new_var.second.data.c_str());
		}
	}

	std::lock_guard<std::mutex> lock(saved_environment_mutex_);
	saved_environment_ = new_environment;
}

std::shared_ptr<SystemEnvironment> CConEmuMain::GetEnvironmentVariables() const
{
	std::lock_guard<std::mutex> lock(saved_environment_mutex_);
	std::shared_ptr<SystemEnvironment> data = saved_environment_;
	return data;
}

void CConEmuMain::OnGlobalSettingsChanged()
{
	UpdateGuiInfoMapping();

	// и отослать в серверы
	CVConGroup::OnUpdateGuiInfoMapping(&m_GuiInfo);

	// И фары тоже уведомить
	CVConGroup::OnUpdateFarSettings();
}

void CConEmuMain::OnPanelViewSettingsChanged(BOOL abSendChanges/*=TRUE*/)
{
	// UpdateGuiInfoMapping будет вызван после завершения создания окна
	if (!InCreateWindow())
	{
		UpdateGuiInfoMapping();
	}

	// Заполнить цвета gpSet->ThSet.crPalette[16], gpSet->ThSet.crFadePalette[16]
	const auto& pcrNormal = gpSet->GetColors(-1, FALSE);
	const auto& pcrFade = gpSet->GetColors(-1, TRUE);

	for(int i=0; i<16; i++)
	{
		// через FOR чтобы с BitMask не наколоться
		gpSet->ThSet.crPalette[i] = (pcrNormal[i]) & 0xFFFFFF;
		gpSet->ThSet.crFadePalette[i] = (pcrFade[i]) & 0xFFFFFF;
	}

	gpSet->ThSet.nFontQuality = gpFontMgr->FontQuality();

	// Параметры основного шрифта ConEmu
	FillConEmuMainFont(&gpSet->ThSet.MainFont);
	//lstrcpy(gpSet->ThSet.MainFont.sFontName, gpSet->FontFaceName());
	//gpSet->ThSet.MainFont.nFontHeight = gpSetCls->FontHeight();
	//gpSet->ThSet.MainFont.nFontWidth = gpSetCls->FontWidth();
	//gpSet->ThSet.MainFont.nFontCellWidth = gpSet->FontSizeX3 ? gpSet->FontSizeX3 : gpSetCls->FontWidth();
	//gpSet->ThSet.MainFont.nFontQuality = gpSet->FontQuality();
	//gpSet->ThSet.MainFont.nFontCharSet = gpSet->FontCharSet();
	//gpSet->ThSet.MainFont.Bold = gpSet->FontBold();
	//gpSet->ThSet.MainFont.Italic = gpSet->FontItalic();
	//lstrcpy(gpSet->ThSet.MainFont.sBorderFontName, gpSet->BorderFontFaceName());
	//gpSet->ThSet.MainFont.nBorderFontWidth = gpSet->BorderFontWidth();

	// Применить в мэппинг (используется в "Panel View" и "Background"-плагинах
	bool lbChanged = gpSetCls->m_ThSetMap.SetFrom(&gpSet->ThSet);

	if (abSendChanges && lbChanged)
	{
		// и отослать заинтересованным
		CVConGroup::OnPanelViewSettingsChanged();
	}
}

void CConEmuMain::OnTaskbarCreated()
{
	Icon.OnTaskbarCreated();

	// Т.к. создан TaskBar - нужно (пере)создать интерфейсы
	Taskbar_Release();
	Taskbar_Init();

	// И передернуть все TaskBarGhost
	CVConGroup::OnTaskbarCreated();

	// Refresh DefTerm hooking
	if (mp_DefTrm) mp_DefTrm->OnTaskbarCreated();

	// Dummy
	if (mp_DragDrop) mp_DragDrop->OnTaskbarCreated();
}

void CConEmuMain::OnTaskbarButtonCreated()
{
	// May be not initialized yet
	Taskbar_Init();

	if (!gpSet->isWindowOnTaskBar())
	{
		DWORD styleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
		if (styleEx & WS_EX_APPWINDOW)
		{
			styleEx &= ~WS_EX_APPWINDOW;
			SetWindowStyleEx(styleEx);
		}

		// Если оно вдруг таки показано на таскбаре
		Taskbar_DeleteTabXP(ghWnd);
	}
	else
	{
		if (mb_IsUacAdmin)
		{
			// Bug in Win7? Sometimes after startup ‘Overlay icon’ was not appeared.
			mn_TBOverlayTimerCounter = 0;
			SetKillTimer(true, TIMER_ADMSHIELD_ID, TIMER_ADMSHIELD_ELAPSE);
		}
	}
}

void CConEmuMain::OnDefaultTermChanged()
{
	if (!this || !mp_DefTrm)
		return;

	// Save to registry only from Settings window or ConEmu startup
	mp_DefTrm->ApplyAndSave(true, false);

	mp_DefTrm->OnHookedListChanged();
}

void CConEmuMain::OnTaskbarSettingsChanged()
{
	CVConGroup::OnTaskbarSettingsChanged();

	if (!gpSet->isTabsOnTaskBar())
		OnAllGhostClosed();

	DWORD styleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	if (!gpSet->isWindowOnTaskBar())
	{
		if (styleEx & WS_EX_APPWINDOW)
		{
			styleEx &= ~WS_EX_APPWINDOW;
			SetWindowStyleEx(styleEx);
			// Если оно вдруг таки показано на таскбаре
			Taskbar_DeleteTabXP(ghWnd);
		}
	}
	else
	{
		if (!(styleEx & WS_EX_APPWINDOW))
		{
			styleEx |= WS_EX_APPWINDOW;
			SetWindowStyleEx(styleEx);
			// Показать на таскбаре
			Taskbar_AddTabXP(ghWnd);
		}
	}

	apiSetForegroundWindow(ghOpWnd ? ghOpWnd : ghWnd);
}

CConEmuMain::StartupStage CConEmuMain::GetStartupStage() const
{
	return mn_StartupFinished;
}

bool CConEmuMain::InCreateWindow()
{
	return (mn_StartupFinished < ss_VConAreCreated);
}

bool CConEmuMain::InQuakeAnimation()
{
	return (mn_QuakePercent != 0);
}

/* static */
BOOL CConEmuMain::EnumWindowsOverQuake(HWND hWnd, LPARAM lpData)
{
	DWORD nStyle = GetWindowLong(hWnd, GWL_STYLE);
	if (!(nStyle & WS_VISIBLE))
		return TRUE; // Не видимые окна не интересуют
	if (nStyle & WS_CHILD)
		return TRUE; // Внезапно "дочерние" - тоже
	DWORD nStyleEx = GetWindowLong(hWnd, GWL_EXSTYLE);
	if ((nStyleEx & WS_EX_TOOLWINDOW) != 0)
		return TRUE; // Палитры/тулбары - пропускаем

	DWORD nPID;
	GetWindowThreadProcessId(hWnd, &nPID);
	if (nPID == GetCurrentProcessId())
	{
		// Все, раз дошли до нашего окна - значит всех кто сверху уже перебрали
		// Но hWnd может быть нашим диалогом, так что прекращаем только на ghWnd
		return (hWnd != ghWnd);
	}

	WindowsOverQuake* pOur = (WindowsOverQuake*)lpData;
	RECT rcWnd = {}, rcOver = {};

	if (GetWindowRect(hWnd, &rcWnd))
	{
		if (IntersectRect(&rcOver, &pOur->rcWnd, &rcWnd))
		{
			// Окно лежит поверх нашего
			if (memcmp(&rcOver, pOur, sizeof(rcOver)) == 0)
			{
				pOur->iRc = NULLREGION;
				return FALSE; // Окно полностью скрыто
			}

			// Начинаются расчеты
			HRGN hWndRgn = CreateRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
			pOur->iRc = CombineRgn(pOur->hRgn, pOur->hRgn, hWndRgn, RGN_DIFF);
			DeleteObject(hWndRgn);

			_ASSERTE(pOur->iRc != ERROR);
			if (pOur->iRc == NULLREGION)
			{
				// Окно полностью скрыто
				return FALSE;
			}
		}
	}

	return TRUE;
}

UINT CConEmuMain::IsQuakeVisible()
{
	if (!IsWindowVisible(ghWnd))
		return 0;
	if (isIconic())
		return 0;

	RECT rcWnd = {}; GetWindowRect(ghWnd, &rcWnd);
	if (IsRectEmpty(&rcWnd))
	{
		_ASSERTE(FALSE && "rcWnd must not be empty");
		return 0;
	}

	int nFullSq = (rcWnd.right - rcWnd.left) * (rcWnd.bottom - rcWnd.top);
	if (nFullSq <= 0)
	{
		_ASSERTE(nFullSq > 0);
		return 0;
	}

	WindowsOverQuake rcFree = {rcWnd, nullptr, SIMPLEREGION};
	rcFree.hRgn = CreateRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom);
	EnumWindows(EnumWindowsOverQuake, (LPARAM)&rcFree);

	UINT  nVisiblePart = 100; // в процентах

	// Если "Видимая область" окна стала менее VisibleLimit(%) - считаем что окно стало "не видимым"
	if (rcFree.iRc == NULLREGION)
	{
		nVisiblePart = 0;
	}
	else
	{
		int nLeftSq = 0; //(rcFree.right - rcFree.left) * (rcFree.bottom - rcFree.top);
		DWORD cbSize = GetRegionData(rcFree.hRgn, 0, nullptr);
		if (cbSize > sizeof(PRGNDATA))
		{
			LPRGNDATA lpRgnData = (LPRGNDATA)calloc(cbSize,1);
			lpRgnData->rdh.dwSize = sizeof(lpRgnData->rdh);
			cbSize = GetRegionData(rcFree.hRgn, cbSize, lpRgnData);
			if (cbSize && (lpRgnData->rdh.iType == RDH_RECTANGLES))
			{
				LPRECT pRc = (LPRECT)lpRgnData->Buffer;
				for (DWORD i = 0; i < lpRgnData->rdh.nCount; i++)
				{
					nLeftSq += (pRc[i].right - pRc[i].left) * (pRc[i].bottom - pRc[i].top);
				}
			}
			else
			{
				_ASSERTE(lpRgnData->rdh.iType == RDH_RECTANGLES);
			}
			free(lpRgnData);

			// Видимость в процентах
			nVisiblePart = (nLeftSq * 100 / nFullSq);
			if (nVisiblePart > 100)
			{
				_ASSERTE(nVisiblePart <= 100);
				nVisiblePart = 100;
			}
		}
		else
		{
			_ASSERTE(cbSize > sizeof(PRGNDATA));
		}
	}

	DeleteObject(rcFree.hRgn);

	return nVisiblePart;
}

void CConEmuMain::OnSwitchGuiFocus(SwitchGuiFocusOp FocusOp)
{
	if (!((FocusOp >= static_cast<SwitchGuiFocusOp>(0)) && (FocusOp < SwitchGuiFocusOp::Last)))
	{
		Assert((FocusOp >= static_cast<SwitchGuiFocusOp>(0)) && (FocusOp < SwitchGuiFocusOp::Last));  // -V571
		return;
	}

	CVConGuard VCon;
	//HWND hSet = ghWnd;

	if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon()->GuiWnd())
	{
		bool bSetChild = (FocusOp == SwitchGuiFocusOp::FocusChild);

		if (FocusOp == SwitchGuiFocusOp::FocusSwitch)
		{
			DWORD nFocusPID = 0;
			HWND hGet = GetFocus();
			if (hGet)
			{
				GetWindowThreadProcessId(hGet, &nFocusPID);
			}

			if (nFocusPID == GetCurrentProcessId())
			{
				// Set focus into ChildGui application
				bSetChild = true;
			}
		}

		if (bSetChild)
		{
			// Set focus into ChildGui application
			VCon->RCon()->GuiWndFocusRestore();
			return;
		}
	}

	if (FocusOp == SwitchGuiFocusOp::FocusConEmu)
	{
		setFocus();
	}
	else if (FocusOp == SwitchGuiFocusOp::FocusParent)
	{
		if (mp_Inside)
			mp_Inside->SetFocusParent();
	}
}

// During excessing keyboard activity weird things may happen:
// WM_PAINT do not come into message queue until key is released
void CConEmuMain::CheckNeedRepaint()
{
	CVConGuard VCon;
	if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
	{
		if (VCon->IsInvalidatePending())
		{
			LogString(L"Forcing active VCon redraw");
			VCon->Redraw(true);
		}
	}
}

void CConEmuMain::EnterAltNumpadMode(UINT nBase)
{
	if (!this || !mp_AltNumpad)
	{
		_ASSERTE(this && mp_AltNumpad);
		return;
	}

	if ((nBase != 0) && (nBase != 10) && (nBase != 16))
	{
		_ASSERTE(nBase==0 || nBase==10 || nBase==16);
		return;
	}

	mp_AltNumpad->StartCapture(nBase, 0, true);
}

LRESULT CConEmuMain::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
#ifdef _DEBUG
	#if 0
	if (messg == WM_KEYDOWN || messg == WM_KEYUP)
	{
		if (wParam >= VK_F1 && wParam <= VK_F4)
		{
			if (messg == WM_KEYUP)
			{
				DWORD nFlags = 0;
				switch ((DWORD)wParam)
				{
				case VK_F1:
					nFlags = AW_SLIDE|AW_HIDE|AW_HOR_NEGATIVE|AW_VER_NEGATIVE;
					break;
				case VK_F2:
					nFlags = AW_SLIDE|AW_HOR_POSITIVE|AW_VER_POSITIVE;
					break;
				case VK_F3:
					nFlags = AW_BLEND|AW_HIDE|AW_CENTER;
					break;
				case VK_F4:
					nFlags = AW_BLEND|AW_CENTER;
					break;
				}
				BOOL b = AnimateWindow(ghWnd, 500, nFlags);
				DWORD nErr = GetLastError();
				wchar_t szErr[128];
				swprintf_c(szErr, L"AnimateWindow(%08x)=%u, Err=%u\n", nFlags, b, nErr);
				DEBUGSTRANIMATE(szErr);
			}
			return 0;
		}
	}
	#endif
#endif

	if (!m_GuiInfo.bGuiActive)
	{
		UpdateGuiInfoMappingActive(true);
	}

	if (mb_ImeMethodChanged)
	{
		if (messg == WM_SYSKEYDOWN || messg == WM_SYSKEYUP)
		{
			if (messg == WM_SYSKEYUP)
				mb_ImeMethodChanged = false;
			return 0;
		}
		else
		{
			mb_ImeMethodChanged = false;
		}
	}

	WORD bVK = (WORD)(wParam & 0xFF);
	//-- некорректно. сработало на LCtrl+LAlt
	//bool bAltGr = (messg == WM_KEYDOWN && bVK == VK_MENU);
	//_ASSERTE(!bAltGr);

	#ifdef _DEBUG
	wchar_t szDebug[255];
	swprintf_c(szDebug, L"%s(VK=0x%02X, Scan=%i, lParam=0x%08X)\n",
	          (messg == WM_KEYDOWN) ? L"WM_KEYDOWN" :
	          (messg == WM_KEYUP) ? L"WM_KEYUP" :
	          (messg == WM_SYSKEYDOWN) ? L"WM_SYSKEYDOWN" :
	          (messg == WM_SYSKEYUP) ? L"WM_SYSKEYUP" :
	          L"<Unknown Message> ",
	          bVK, ((DWORD)lParam & 0xFF0000) >> 16, (DWORD)lParam);
	DEBUGSTRKEY(szDebug);
	#endif


	if (gpSet->isLogging())
		CVConGroup::LogInput(messg, wParam, lParam);

	if (mp_AltNumpad->OnKeyboard(hWnd, messg, wParam, lParam))
		return 0;

#if 1
	// Works fine, but need to cache last pressed key, cause of need them in WM_KEYUP (send char to console)
	wchar_t szTranslatedChars[16] = {0};
	int nTranslatedChars = 0;
	MSG msgList[16] = {{hWnd,messg,wParam,lParam}};
	int iAddMsgList = 0, iDeadMsgIndex = 0;
	int iProcessDeadChars = 0;
	//static bool bWasDeadChar = false;
	//static LPARAM nDeadCharLParam = 0;
	MSG DeadCharMsg = {};

	if (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN)
	{
		_ASSERTE(sizeof(szTranslatedChars) == sizeof(m_TranslatedChars[0].szTranslatedChars));
		//BOOL lbDeadChar = FALSE;
		MSG msg, msg1;
		LPARAM lKey1ScanCode = (lParam & 0x00FF0000);
		DEBUGTEST(LPARAM lKey2ScanCode = 0);

		// 120722 - Spanish (for example) layout. Есть несколько клавиш для ввода
		// символов с разными Grave/Acute/Circumflex/Tilde/Diaeresis/Ring Above
		// Например, при вводе "~" и "a" система продуцирует следующую последовательность
		// WM_KEYDOWN, WM_DEADCHAR (~), WM_KEYUP, WM_KEYDOWN, WM_CHAR ("a" с "~"), WM_KEYUP
		// Причем только в том случае, если нет задержек в обработке этих сообщений.
		// При наличии задержки в обработке - получаем просто "a" без тильды.

		TODO("Проверить немецкую раскладку с '^'");

		while (nTranslatedChars < 15  // извлечь из буфера все последующие WM_CHAR & WM_SYSCHAR
		        && PeekMessage(&msg, 0,0,0, PM_NOREMOVE)
		      )
		{
			bool bNeedGet = true;
			DEBUGTEST(MSG firstMsg = msg);

			// Windows иногда умудряется вклинить "левые" сообщения между WM_KEYDOWN & WM_CHAR
			// в результате некоторые буквы могут "проглатываться" или вообще перемешиваться...
			// Обработаем "посторонние" сообщения

			if ((msg.message == WM_CLOSE) || (msg.message == WM_DESTROY)
				|| (msg.message == WM_QUIT) || (msg.message == WM_QUERYENDSESSION))
			{
				#ifdef _DEBUG
				wchar_t szDbg[160]; swprintf_c(szDbg, L"  PeekMessage stops, get WM_%04X(0x%02X, 0x%08X)",
					msg.message, (DWORD)msg.wParam, (DWORD)msg.lParam);
				DEBUGSTRCHAR(szDbg);
				#endif

				// After these messages we certainly must stop trying to process keypress
				break;
			}

			if (!iProcessDeadChars && (msg.message == messg))
			{
				// User keeps pressing key
				_ASSERTE(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN);
				break;
			}

			if (!(msg.message == WM_CHAR || msg.message == WM_SYSCHAR
						|| msg.message == WM_DEADCHAR || msg.message == WM_SYSDEADCHAR
						|| msg.message == WM_KEYUP || msg.message == WM_KEYDOWN
						|| msg.message == WM_SYSKEYUP || msg.message == WM_SYSKEYDOWN
						))
			{
				// Remove from buffer and process
				if (!GetMessage(&msg1, 0,0,0))
					break;
				bNeedGet = false;

				#ifdef _DEBUG
				wchar_t szDbg[160]; swprintf_c(szDbg, L"  Recursion of WM_%04X(0x%02X, 0x%08X)",
					msg1.message, (DWORD)msg1.wParam, (DWORD)msg1.lParam);
				DEBUGSTRCHAR(szDbg);
				#endif

				if (!(msg1.message == WM_CHAR || msg1.message == WM_SYSCHAR
						|| msg1.message == WM_DEADCHAR || msg1.message == WM_SYSDEADCHAR
						|| msg1.message == WM_KEYUP || msg1.message == WM_KEYDOWN
						|| msg1.message == WM_SYSKEYUP || msg1.message == WM_SYSKEYDOWN
						))
				{
					static LONG lCounter = 0;
					MSetter lSetCounter(&lCounter);
					if (lCounter > 5)
					{
						_ASSERTE(FALSE && "Too many nested calls");
						break;
					}

					if (!ProcessMessage(msg1))
						break;

					CheckNeedRepaint();

					// This message skipped
					continue;
				}
				else
				{
					// Unexpected message ID, keyboard input may be garbled?
					// In msg we may peek WM_PAINT, but in msg1 get WM_SYSKEYDOWN...
					// _ASSERTE(msg.message == msg1.message);
					msg = msg1;
				}
			}

			// We may get different message than was peeked
			if (!iProcessDeadChars && (msg.message == messg))
			{
				// User keeps pressing key
				_ASSERTE(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN);
				break;
			}

			if (!(msg.message == WM_CHAR || msg.message == WM_SYSCHAR
					|| msg.message == WM_DEADCHAR || msg.message == WM_SYSDEADCHAR
					|| (iProcessDeadChars == 1 // был WM_DEADCHAR/WM_SYSDEADCHAR, ожидаем WM_KEYUP и WM_KEYDOWN
						&& ((msg.message == WM_KEYUP && (msg.lParam & 0x00FF0000) == lKey1ScanCode)
							|| (msg.message == WM_KEYDOWN /* следующая за DeadChar собственно буква */))
						)
					|| (iProcessDeadChars == 2 // был WM_DEADCHAR/WM_SYSDEADCHAR, теперь ожидаем WM_CHAR
						&& (msg.message == WM_CHAR || msg.message == WM_SYSCHAR)
						)
					))
				//|| (msg.lParam & 0xFF0000) != lKey1ScanCode /* совпадение скан-кода */) // 120722 - убрал
			{
				#ifdef _DEBUG
				wchar_t szDbg[160]; swprintf_c(szDbg, L"  PeekMessage stops, get WM_%04X(0x%02X, 0x%08X)",
					msg.message, (DWORD)msg.wParam, (DWORD)msg.lParam);
				DEBUGSTRCHAR(szDbg);
				#endif
				// Message was already removed from input buffer, but Peek and Get was returned different message IDs?
				_ASSERTE(bNeedGet && "Keyboard message must be processed, but there was unexpected sequence");
				// Stop queue checking
				break;
			}

			// Remove from buffer
			if (bNeedGet && !GetMessage(&msg1, 0,0,0))
			{
				_ASSERTE(FALSE && "GetMessage was failed while PeekMessage was succeeded");
				break;
			}

			// Let process our message
			{
				ConEmuMsgLogger::Log(msg1, ConEmuMsgLogger::msgCommon);
				_ASSERTEL(msg1.message == msg.message && msg1.wParam == msg.wParam && msg1.lParam == msg.lParam);
				msgList[++iAddMsgList] = msg1;

				if (gpSet->isLogging(4))
				{
					LogMessage(msg1.hwnd, msg1.message, msg1.wParam, msg1.lParam);
				}

				#ifdef _DEBUG
				wchar_t szDbg[160]; swprintf_c(szDbg, L"  %s(0x%02X='%c', Scan=%i, lParam=0x%08X)",
					(messg == WM_CHAR) ? L"WM_CHAR" : (messg == WM_SYSCHAR) ? L"WM_SYSCHAR" : (messg == WM_SYSDEADCHAR) ? L"WM_SYSDEADCHAR" : L"WM_DEADCHAR",
					(DWORD)wParam, wParam?(wchar_t)wParam:L' ', ((DWORD)lParam & 0xFF0000) >> 16, (DWORD)lParam);
				DEBUGSTRCHAR(szDbg);
				#endif

				if (msg.message == WM_CHAR || msg.message == WM_SYSCHAR)
				{
					szTranslatedChars[nTranslatedChars ++] = (wchar_t)msg1.wParam;
					if (iProcessDeadChars == 1)
					{
						_ASSERTEL(iProcessDeadChars == 2); // уже должно было быть выставлено в WM_KEYDOWN
						iProcessDeadChars = 2;
					}
				}
				else if (msg.message == WM_DEADCHAR || msg.message == WM_SYSDEADCHAR)
				{
					_ASSERTEL(iProcessDeadChars==0 && "Must be first entrance");
					//lbDeadChar = TRUE;
					iProcessDeadChars = 1;
					DeadCharMsg = msg;
				}
				else if (msg.message == WM_KEYDOWN)
				{
					if (iProcessDeadChars == 1)
					{
						iProcessDeadChars = 2;
						DEBUGTEST(lKey2ScanCode = (msg.lParam & 0x00FF0000));
						iDeadMsgIndex = iAddMsgList;
					}
				}

				// 120722 - Было DispatchMessage вместо TranslateMessage.
				// Требуется обработать сообщение
				TranslateMessage(&msg1);
				//DispatchMessage(&msg1);

				if ((msg.message == WM_KEYDOWN) || (msg.message == WM_SYSKEYDOWN))
				{
					CheckNeedRepaint();
				}
			}
		} // end of "while nTranslatedChars && PeekMessage"

		if (gpSet->isLogging())
		{
			for (int i = 1; i <= iAddMsgList; i++)
			{
				CVConGroup::LogInput(msgList[i].message, msgList[i].wParam, msgList[i].lParam);
			}
		}

		if (iProcessDeadChars == 2)
		{
			_ASSERTEL(iDeadMsgIndex>0);
			_ASSERTEL(msgList[iDeadMsgIndex].message == messg);
			wParam = msgList[iDeadMsgIndex].wParam;
			lParam = msgList[iDeadMsgIndex].lParam;
			bVK = (WORD)(wParam & 0xFF);
		}

		//if (lbDeadChar)
		//{
		//	bWasDeadChar = true;
		//	nDeadCharLParam = lParam;
		//}
		//else
		//{
		//	bWasDeadChar = false;
		//}

		//if (lbDeadChar && nTranslatedChars)
		//{
		//	_ASSERTEL(FALSE && "Dead char does not produces szTranslatedChars");
		//	lbDeadChar = FALSE;
		//}

		memmove(m_TranslatedChars[bVK].szTranslatedChars, szTranslatedChars, sizeof(szTranslatedChars));
	}
	else
	{
		_ASSERTEL((messg == WM_KEYUP || messg == WM_SYSKEYUP) && "Unexpected msg");
		//if (messg == WM_KEYUP || messg == WM_SYSKEYUP)
		//{
		//	if (bWasDeadChar)
		//	{
		//		if ((nDeadCharLParam & 0xFF0000) == (lParam & 0xFF0000))
		//		{
		//			//DefWindowProc(hWnd, messg, wParam, lParam);
		//		}
		//		bWasDeadChar = false;
		//	}
		//}
		//else
		//{
		//	_ASSERTEL((messg == WM_KEYUP || messg == WM_SYSKEYUP) && "Unexpected msg");
		//}

		szTranslatedChars[0] = m_TranslatedChars[bVK].szTranslatedChars[0];
		szTranslatedChars[1] = 0;
		nTranslatedChars = szTranslatedChars[0] ? 1 : 0;
	}

#endif
#if 0
	/* Works, but has problems with dead chars */
	wchar_t szTranslatedChars[11] = {0};
	int nTranslatedChars = 0;

	if (!GetKeyboardState(m_KeybStates))
	{
		#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		_ASSERTEL(FALSE);
		#endif
		static bool sbErrShown = false;

		if (!sbErrShown)
		{
			sbErrShown = true;
			DisplayLastError(L"GetKeyboardState failed!");
		}
	}
	else
	{
		HKL hkl = (HKL)GetActiveKeyboardLayout();
		UINT nVK = wParam & 0xFFFF;
		UINT nSC = ((DWORD)lParam & 0xFF0000) >> 16;
		WARNING("BUGBUG: похоже глючит в x64 на US-Dvorak");
		nTranslatedChars = ToUnicodeEx(nVK, nSC, m_KeybStates, szTranslatedChars, 10, 0, hkl);
		if (nTranslatedChars>0) szTranslatedChars[std::min(10,nTranslatedChars)] = 0; else szTranslatedChars[0] = 0;
	}

#endif
#if 0
	/* Invalid ? */
	wchar_t szTranslatedChars[16] = {0};

	if (!GetKeyboardState(m_KeybStates))
	{
#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		_ASSERTEL(FALSE);
#endif
		static bool sbErrShown = false;

		if (!sbErrShown)
		{
			sbErrShown = true;
			DisplayLastError(L"GetKeyboardState failed!");
		}
	}

	//MSG smsg = {hWnd, messg, wParam, lParam};
	//TranslateMessage(&smsg);

	/*if (bVK == VK_SHIFT || bVK == VK_MENU || bVK == VK_CONTROL || bVK == VK_LWIN || bVK == VK_RWIN) {
		if (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN) {
			if (
		} else if (messg == WM_KEYUP || messg == WM_SYSKEYUP) {
		}
	}*/

	if (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN)
	{
		HKL hkl = (HKL)m_ActiveKeybLayout; //GetActiveKeyboardLayout();
		UINT nVK = wParam & 0xFFFF;
		UINT nSC = ((DWORD)lParam & 0xFF0000) >> 16;
		WARNING("BUGBUG: похоже глючит в x64 на US-Dvorak");
		int nTranslatedChars = ToUnicodeEx(nVK, nSC, m_KeybStates, szTranslatedChars, 15, 0, hkl);

		if (nTranslatedChars >= 0)
		{
			// 2 or more
			// Two or more characters were written to the buffer specified by pwszBuff.
			// The most common cause for this is that a dead-key character (accent or diacritic)
			// stored in the keyboard layout could not be combined with the specified virtual key
			// to form a single character. However, the buffer may contain more characters than the
			// return value specifies. When this happens, any extra characters are invalid and should be ignored.
			szTranslatedChars[std::min(15,nTranslatedChars)] = 0;
		}
		else if (nTranslatedChars == -1)
		{
			// The specified virtual key is a dead-key character (accent or diacritic).
			// This value is returned regardless of the keyboard layout, even if several
			// characters have been typed and are stored in the keyboard state. If possible,
			// even with Unicode keyboard layouts, the function has written a spacing version
			// of the dead-key character to the buffer specified by pwszBuff. For example, the
			// function writes the character SPACING ACUTE (0x00B4),
			// rather than the character NON_SPACING ACUTE (0x0301).
			szTranslatedChars[0] = 0;
		}
		else
		{
			// Invalid
			szTranslatedChars[0] = 0;
		}

		mn_LastPressedVK = bVK;
	}

#endif

	if (messg == WM_KEYDOWN && !GetGlobalHotkeys().IsActiveHotkeyRegistered())
		GetGlobalHotkeys().RegisterHotKeys(); // Win and other special keys

	_ASSERTEL(messg != WM_CHAR && messg != WM_SYSCHAR);

	// Process some general hotkeys

	if (wParam == VK_ESCAPE)
	{
		// The "Esc" should cancel D&D
		// But if drag was initiated outside (dragging smth. from Explorer over ConEmu)
		// and "Esc" cancels it, the "Esc" keypress could go into Far panels.
		// Trying to mitigate that

		if (mp_DragDrop->InDragDrop())
			return 0;

		static bool bEscPressed = false;
		if (messg != WM_KEYUP)
			bEscPressed = true;
		else if (!bEscPressed)
			return 0;
		else
			bEscPressed = false;
	}

	if ((wParam == VK_CAPITAL) || (wParam == VK_NUMLOCK) || (wParam == VK_SCROLL))
		mp_Status->OnKeyboardChanged();

	// Теперь - можно переслать в консоль
	CVConGuard VCon;
	if ((GetActiveVCon(&VCon) >= 0) && VCon->RCon())
	{
		_ASSERTEL(iProcessDeadChars==0 || iProcessDeadChars==1);
		VCon->RCon()->OnKeyboard(hWnd, messg, wParam, lParam, szTranslatedChars, iProcessDeadChars?&DeadCharMsg:nullptr);
	}
	else if (((wParam & 0xFF) >= VK_WHEEL_FIRST) && ((wParam & 0xFF) <= VK_WHEEL_LAST))
	{
		// Такие коды с клавиатуры приходить не должны, а то для "мышки" ничего не останется
		_ASSERTEL(!(((wParam & 0xFF) >= VK_WHEEL_FIRST) && ((wParam & 0xFF) <= VK_WHEEL_LAST)));
	}
	else
	{
		// Если вдруг активной консоли нету (вообще?) но клавиши обработать все-равно надо
		// true - хоткей обработан
		if (!ProcessHotKeyMsg(messg, wParam, lParam, szTranslatedChars, nullptr))
		{
			; // Keypress was not processed

			//// И напоследок
			//if (wParam == VK_ESCAPE)
			//{
			//	if (messg == WM_KEYUP)
			//	{
			//		DoMinimizeRestore(sih_Minimize);
			//	}
			//}
		}
	}

	return 0;
}

LRESULT CConEmuMain::OnKeyboardHook(DWORD VkMod)
{
	if (!VkMod)
		return 0;

	// While debugging - low-level keyboard hooks almost lock DevEnv
	HooksUnlocker;

	CVConGuard VCon;
	CRealConsole* pRCon = (GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : nullptr;

	TODO("Second key of Chord?");
	ConEmuChord HK = {LOBYTE(VkMod), static_cast<ConEmuModifiers>(VkMod & cvk_ALLMASK)};

	const ConEmuHotKey* pHotKey = ProcessHotKey(HK, true/*bKeyDown*/, nullptr, pRCon);

	if (pHotKey == ConEmuSkipHotKey)
	{
		// Если функция срабатывает на отпускание
		pHotKey = ProcessHotKey(HK, false/*bKeyDown*/, nullptr, pRCon);
	}

	return 0;
}

void CConEmuMain::OnConsoleKey(WORD vk, LPARAM Mods)
{
	// Некоторые комбинации обрабатываются в самом ConEmu и сюда приходить по идее не должны
#if 0
	if (vk == VK_SPACE && (Mods == (Mods & (MOD_ALT|MOD_LALT|MOD_RALT))))
	{
		_ASSERTE(vk != VK_SPACE);
		if (!gpSet->isSendAltSpace)
		{
			ShowSysmenu();
			return;
		}
	}
	if (vk == VK_RETURN && (Mods == (Mods & (MOD_ALT|MOD_LALT|MOD_RALT))))
	{
		_ASSERTE(vk != VK_RETURN);
		if (!gpSet->isSendAltEnter)
		{
			DoFullScreen();
			return;
		}
	}
#endif

	// Все остальное - обычным образом
	CVConGuard VCon;
	CRealConsole* pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : nullptr;
	if (pRCon)
	{
		INPUT_RECORD r = {KEY_EVENT};
		// Alt и прочих слать не нужно - он уже послан
		r.Event.KeyEvent.bKeyDown = TRUE;
		r.Event.KeyEvent.wRepeatCount = 1;
		r.Event.KeyEvent.wVirtualKeyCode = vk;
		r.Event.KeyEvent.wVirtualScanCode = /*28 на моей клавиатуре*/MapVirtualKey(vk, 0/*MAPVK_VK_TO_VSC*/);
		if (Mods & MOD_LALT)
			r.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;
		if (Mods & MOD_RALT)
			r.Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;
		if (Mods & MOD_LCONTROL)
			r.Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;
		if (Mods & MOD_RCONTROL)
			r.Event.KeyEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;
		if (Mods & MOD_SHIFT)
			r.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;
		pRCon->PostConsoleEvent(&r);
		//On Keyboard(hConWnd, WM_KEYUP, VK_RETURN, 0);
		r.Event.KeyEvent.bKeyDown = FALSE;
		pRCon->PostConsoleEvent(&r);
	}
}

bool CConEmuMain::isInImeComposition()
{
	return mb_InImeComposition;
}

void CConEmuMain::UpdateImeComposition()
{
	if (!mh_Imm32)
		return;

	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
	{
		HWND hView = VCon->GetView();

		CONSOLE_CURSOR_INFO ci = {};
		CONSOLE_SCREEN_BUFFER_INFO sbi = {};
		CRealConsole* pRCon = VCon->RCon();
		pRCon->GetConsoleCursorInfo(&ci);
		pRCon->GetConsoleScreenBufferInfo(&sbi);
		COORD crVisual = pRCon->BufferToScreen(sbi.dwCursorPosition);
		POINT ptCurPos = VCon->ConsoleToClient(crVisual.X, crVisual.Y);

		MapWindowPoints(hView, ghWnd, &ptCurPos, 1);

		COMPOSITIONFORM cf = {CFS_POINT, ptCurPos};

		HIMC hImc = _ImmGetContext(ghWnd/*hView?*/);
		_ImmSetCompositionWindow(hImc, &cf);

		LOGFONT lf = {};
		gpFontMgr->GetMainLogFont(lf);
		_ImmSetCompositionFont(hImc, &lf);
	}
}

LRESULT CConEmuMain::OnKeyboardIme(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	bool lbProcessed = false;
	wchar_t szDbgMsg[255];
	switch (messg)
	{
		case WM_IME_CHAR:
			{
				// Посылается в RealConsole (это ввод из окошка IME)
				swprintf_c(szDbgMsg, L"WM_IME_CHAR: char=%c, wParam=%u, lParam=0x%08X\n", (wchar_t)wParam, (DWORD)wParam, (DWORD)lParam);
				DEBUGSTRIME(szDbgMsg);
				CVConGuard VCon;
				if (GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->OnKeyboardIme(hWnd, messg, wParam, lParam);
				}
			}
			break;
		case WM_IME_COMPOSITION:
			swprintf_c(szDbgMsg, L"WM_IME_COMPOSITION: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			UpdateImeComposition();
			//if (lParam & GCS_RESULTSTR)
			//{
			//	HIMC hIMC = ImmGetContext(hWnd);
			//	if (hIMC)
			//	{
			//		DWORD dwSize;
			//		wchar_t* lpstr;
			//		// Get the size of the result string.
			//		dwSize = ImmGetCompositionString(hIMC, GCS_RESULTSTR, nullptr, 0);
			//		// increase buffer size for terminating null character,
			//		dwSize += sizeof(wchar_t);
			//		lpstr = (wchar_t*)calloc(dwSize,1);
			//		if (lpstr)
			//		{
			//			// Get the result strings that is generated by IME into lpstr.
			//			ImmGetCompositionString(hIMC, GCS_RESULTSTR, lpstr, dwSize);
			//			DEBUGSTRIME(L"GCS_RESULTSTR: ");
			//			DEBUGSTRIME(lpstr);
			//			// add this string into text buffer of application
			//			free(lpstr);
			//		}
			//		ImmReleaseContext(hWnd, hIMC);
			//	}
			//}
			break;
		case WM_IME_COMPOSITIONFULL:
			swprintf_c(szDbgMsg, L"WM_IME_COMPOSITIONFULL: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_CONTROL:
			swprintf_c(szDbgMsg, L"WM_IME_CONTROL: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_ENDCOMPOSITION:
			swprintf_c(szDbgMsg, L"WM_IME_ENDCOMPOSITION: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			// IME закончен. Обычные нажатия опять можно посылать в консоль
			mb_InImeComposition = false;
			break;
		case WM_IME_KEYDOWN:
			swprintf_c(szDbgMsg, L"WM_IME_KEYDOWN: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_KEYUP:
			swprintf_c(szDbgMsg, L"WM_IME_KEYUP: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_NOTIFY:
			swprintf_c(szDbgMsg, L"WM_IME_NOTIFY: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			switch (wParam)
			{
				case IMN_SETOPENSTATUS:
					// Как-то странно, нажатие Alt` (в японской раскладке) посылает в консоль артефактный символ
					// (пересылает нажатие в консоль)
					mb_ImeMethodChanged = (wParam == IMN_SETOPENSTATUS) && isPressed(VK_MENU);
					break;
				//case 0xF:
				//	{
				//		wchar_t szInfo[1024];
				//		getWindowInfo((HWND)lParam, szInfo);
				//	}
				//	break;
			}
			break;
		case WM_IME_REQUEST:
			swprintf_c(szDbgMsg, L"WM_IME_REQUEST: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			//if (wParam == IMR_QUERYCHARPOSITION)
			//{
			//	IMECHARPOSITION* p = (IMECHARPOSITION*)lParam;
			//	GetWindowRect(ghWnd DC, &p->rcDocument);
			//	p->cLineHeight = gpSetCls->FontHeight();
			//	if (mp_ VActive && mp_ VActive->RCon())
			//	{
			//		COORD crCur = {};
			//		mp_ VActive->RCon()->GetConsoleCursorPos(&crCur);
			//		p->pt = mp_ VActive->ConsoleToClient(crCur.X, crCur.Y);
			//	}
			//	lbProcessed = true;
			//	result = TRUE;
			//}
			break;
		case WM_IME_SELECT:
			swprintf_c(szDbgMsg, L"WM_IME_SELECT: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_SETCONTEXT:
			// If the application draws the composition window,
			// the default IME window does not have to show its composition window.
			// In this case, the application must clear the ISC_SHOWUICOMPOSITIONWINDOW
			// value from the lParam parameter before passing the message to DefWindowProc
			// or ImmIsUIMessage. To display a certain user interface window, an application
			// should remove the corresponding value so that the IME will not display it.
			swprintf_c(szDbgMsg, L"WM_IME_SETCONTEXT: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			break;
		case WM_IME_STARTCOMPOSITION:
			swprintf_c(szDbgMsg, L"WM_IME_STARTCOMPOSITION: wParam=0x%08X, lParam=0x%08X\n", (DWORD)wParam, (DWORD)lParam);
			DEBUGSTRIME(szDbgMsg);
			UpdateImeComposition();
			// Начало IME. Теперь нужно игнорировать обычные нажатия (не посылать в консоль)
			mb_InImeComposition = true;
			break;
		default:
			_ASSERTE(messg==0);
	}
	if (!lbProcessed)
		result = DefWindowProc(hWnd, messg, wParam, lParam);
	return result;
}

void CConEmuMain::AppendHKL(wchar_t* szInfo, size_t cchInfoMax, HKL* hKeyb, int nCount)
{
	INT_PTR nLen = _tcslen(szInfo);
	INT_PTR cchLen = cchInfoMax - nLen - 1;
	int i = 0;
	wchar_t* pszStr = szInfo + nLen;
	while ((i < nCount) && (cchLen > 40))
	{
		swprintf_c(pszStr, cchLen/*#SECURELEN*/, WIN3264TEST(L"0x%08X",L"0x%08X%08X") L" ", WIN3264WSPRINT(hKeyb[i]));
		nLen = _tcslen(pszStr);
		cchLen -= nLen;
		pszStr += nLen;
		i++;
	}
}

void CConEmuMain::AppendRegisteredLayouts(wchar_t* szInfo, size_t cchInfoMax)
{
	INT_PTR nLen = _tcslen(szInfo);
	INT_PTR cchLen = cchInfoMax - nLen - 1;
	wchar_t* pszStr = szInfo + nLen;
	for (size_t i = 0; (i < countof(m_LayoutNames)) && (cchLen > 40); i++)
	{
		if (!m_LayoutNames[i].bUsed)
			continue;
		swprintf_c(pszStr, cchLen/*#SECURELEN*/, L"{0x%08X," WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"} ",
			m_LayoutNames[i].klName, WIN3264WSPRINT(m_LayoutNames[i].hkl));
		nLen = _tcslen(pszStr);
		cchLen -= nLen;
		pszStr += nLen;
	}
}

void CConEmuMain::CheckActiveLayoutName()
{
	wchar_t szLayout[KL_NAMELENGTH] = {0}, *pszEnd = nullptr;
	GetKeyboardLayoutName(szLayout);
	HKL hkl = GetKeyboardLayout(0);
	HKL hKeyb[20] = {};
	UINT nCount = GetKeyboardLayoutList(countof(hKeyb), hKeyb);

	#ifdef TEST_JP_LANS
	wcscpy_c(szLayout, L"00000411");
	hkl = (HKL)0x04110411;
	nCount = 3; hKeyb[0] = (HKL)(DWORD_PTR)0x04110411; hKeyb[0] = (HKL)(DWORD_PTR)0xE0200411; hKeyb[0] = (HKL)(DWORD_PTR)0x04090409;
	#endif

	DWORD dwLayout = wcstoul(szLayout, &pszEnd, 16);

	wchar_t szInfo[200];
	swprintf_c(szInfo, L"Layout=%s, HKL=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L", Count=%u\n", szLayout, WIN3264WSPRINT(hkl), nCount);
	AppendHKL(szInfo, countof(szInfo), hKeyb, nCount);

	int i, iUnused = -1;

	if (dwLayout)
	{
		for (i = 0; i < (int)countof(m_LayoutNames); i++)
		{
			if (!m_LayoutNames[i].bUsed)
			{
				if (iUnused == -1) iUnused = i;
				continue;
			}

			if (m_LayoutNames[i].klName == dwLayout)
			{
				iUnused = -2; // set "-2" to avoid assert lower
				break;
			}
		}
	}

	if (iUnused >= 0)
	{
		// Старшие слова совпадать не будут (если совпадают, то случайно)
		// А вот младшие - должны бы, или хотя бы "Primary Language ID". Но...
		// Issue 1050 #10: Layout=00020405, HKL=0xFFFFFFFFF00A0409. Weird...
		if ((LOWORD(hkl) & 0xFF) != (LOWORD(dwLayout) & 0xFF))
		{
			wcscat_c(szInfo, L"\nLOWORD(hkl)==LOWORD(dwLayout) -- Warning, strange layout ID");
			wchar_t* psz = szInfo; while (psz && (psz = wcschr(psz, L'\n'))) *psz = L' ';
			LogString(szInfo);
		}

		// So, always store "startup" layout and HKL for further reference...
		StoreLayoutName(iUnused, dwLayout, hkl);
	}
	else if (iUnused == -1)
	{
		// Startup keyboard layout must be detected
		wcscat_c(szInfo, L"\niUnused==-1");
		//AssertMsg(szInfo); -- gh-1606
		LogString(szInfo);
	}
}

/// Called from CheckActiveLayoutName, OnLangChange, OnLangeChangeConsole
void CConEmuMain::StoreLayoutName(int iIdx, DWORD dwLayout, HKL hkl)
{
	_ASSERTE(iIdx>=0 && iIdx<countof(m_LayoutNames) && !m_LayoutNames[iIdx].bUsed);
	wchar_t szInfo[100];
	swprintf_c(szInfo, L"StoreLayoutName(%i,x%08X," WIN3264TEST(L"0x%08X",L"0x%08X%08X") L")", iIdx, dwLayout, WIN3264WSPRINT(hkl));
	LogString(szInfo);

	// Check. This layout must not be stored already!
	for (size_t i = 0; i < countof(m_LayoutNames); i++)
	{
		if (m_LayoutNames[i].bUsed
			&& ((m_LayoutNames[i].klName == dwLayout)
				|| (m_LayoutNames[i].hkl == (DWORD_PTR)hkl)))
		{
			wchar_t szAssert[255];
			swprintf_c(szAssert, L"Layout 0x%08X was already registered (hkl=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L")\nRegList: ", dwLayout, WIN3264WSPRINT(hkl));
			AppendRegisteredLayouts(szAssert, countof(szAssert));
			AssertMsg(szAssert);
		}
	}

	m_LayoutNames[iIdx].bUsed = TRUE;
	m_LayoutNames[iIdx].klName = dwLayout;
	m_LayoutNames[iIdx].hkl = (DWORD_PTR)hkl;
}

LRESULT CConEmuMain::OnLangChange(UINT messg, WPARAM wParam, LPARAM lParam)
{
	// In Win7 we may not get WM_INPUTLANGCHANGEREQUEST at all (at least with mouse switching)
	_ASSERTEX(messg == WM_INPUTLANGCHANGEREQUEST || messg == WM_INPUTLANGCHANGE);

	const UINT   nLastMsg = m_KeyboardState.nLastLngMsg;
	const LPARAM lLastPrm = m_KeyboardState.lLastLngPrm;

	/*
	**********
	Вызовы идут в следующем порядке (WinXP SP3)
	**********
	En -> Ru : --> Слетает после этого!
	**********
	19:17:15.043(gui.4720) ConEmu: WM_INPUTLANGCHANGEREQUEST(CP:1, HKL:0x04190419)
	19:17:17.043(gui.4720) ConEmu: WM_INPUTLANGCHANGE(CP:204, HKL:0x04190419)
	19:17:19.043(gui.4720) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGE) = 0x04190419)
	19:17:21.043(gui.4720) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGEREQUEST) = 0x04190419)
	19:17:23.043(gui.4720) CRealConsole::SwitchKeyboardLayout(CP:1, HKL:0x04190419)
	19:17:25.044(gui.4720) RealConsole: WM_INPUTLANGCHANGEREQUEST, CP:1, HKL:0x04190419 via CmdExecute
	19:17:27.044(gui.3072) GUI received CECMD_LANGCHANGE
	19:17:29.044(gui.4720) ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = 0x04190419
	19:17:31.044(gui.4720) ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = 0x04190419
	--> Слетает после этого!
	'ConEmu.exe': Loaded 'C:\WINDOWS\system32\kbdru.dll'
	'ConEmu.exe': Unloaded 'C:\WINDOWS\system32\kbdru.dll'
	19:17:33.075(gui.4720) ConEmu: Calling GetKeyboardLayout(0)
	19:17:35.075(gui.4720) ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x04190419
	19:17:37.075(gui.4720) ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x04190419
	**********
	Ru -> En : --> Слетает после этого!
	**********
	17:23:36.013(gui.3152) ConEmu: WM_INPUTLANGCHANGEREQUEST(CP:1, HKL:0x04090409)
	17:23:36.013(gui.3152) ConEmu: WM_INPUTLANGCHANGE(CP:0, HKL:0x04090409)
	17:23:36.013(gui.3152) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGE) = 0x04090409)
	17:23:36.013(gui.3152) ConEmu: GetKeyboardLayout(0) after DefWindowProc(WM_INPUTLANGCHANGEREQUEST) = 0x04090409)
	17:23:36.013(gui.3152) CRealConsole::SwitchKeyboardLayout(CP:1, HKL:0x04090409)
	17:23:36.013(gui.3152) RealConsole: WM_INPUTLANGCHANGEREQUEST, CP:1, HKL:0x04090409 via CmdExecute
	ConEmuC: PostMessage(WM_INPUTLANGCHANGEREQUEST, CP:1, HKL:0x04090409)
	The thread 'Win32 Thread' (0x3f0) has exited with code 1 (0x1).
	ConEmuC: InputLayoutChanged (GetConsoleKeyboardLayoutName returns) '00000409'
	17:23:38.013(gui.4460) GUI received CECMD_LANGCHANGE
	17:23:40.028(gui.4460) ConEmu: GetKeyboardLayout(0) on CECMD_LANGCHANGE after GetKeyboardLayout(0) = 0x04090409
	--> Слетает после этого!
	'ConEmu.exe': Loaded 'C:\WINDOWS\system32\kbdus.dll'
	'ConEmu.exe': Unloaded 'C:\WINDOWS\system32\kbdus.dll'
	17:23:42.044(gui.4460) ConEmu: Calling GetKeyboardLayout(0)
	17:23:44.044(gui.4460) ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x04090409
	17:23:46.044(gui.4460) ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x04090409
	*/
	LRESULT result = 1;
#ifdef _DEBUG
	WCHAR szMsg[255];
	swprintf_c(szMsg, L"ConEmu: %s(CP:%i, HKL:0x%08I64X)\n",
	          (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
	          (DWORD)wParam, (unsigned __int64)(DWORD_PTR)lParam);
	DEBUGSTRLANG(szMsg);
#endif

	if (gpSet->isLogging(2))
	{
		WCHAR szInfo[255];
		swprintf_c(szInfo, L"CConEmuMain::OnLangChange: %s(CP:%i, HKL:0x%08I64X)",
		          (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
		          (DWORD)wParam, (unsigned __int64)(DWORD_PTR)lParam);
		LogString(szInfo);
	}

	m_KeyboardState.nLastLngMsg = messg;
	m_KeyboardState.lLastLngPrm = lParam;

	/*
	wParam = 204, lParam = 0x04190419 - Russian
	wParam = 0,   lParam = 0x04090409 - US
	wParam = 0,   lParam = 0xfffffffff0020409 - US Dvorak
	wParam = 0,   lParam = 0xfffffffff01a0409 - US Dvorak left hand
	wParam = 0,   lParam = 0xfffffffff01b0409 - US Dvorak
	*/
	//POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
	//mn_CurrentKeybLayout = lParam;
#ifdef _DEBUG
	wchar_t szBeforeChange[KL_NAMELENGTH] = {0}; GetKeyboardLayoutName(szBeforeChange);
#endif
	// Собственно смена
	result = DefWindowProc(ghWnd, messg, wParam, lParam);
	// Проверяем, что получилось
	wchar_t szAfterChange[KL_NAMELENGTH] = {0}; GetKeyboardLayoutName(szAfterChange);
	//if (wcscmp(szAfterChange, szBeforeChange)) -- практика показывает, что уже совпадают.
	//{
	wchar_t *pszEnd = szAfterChange+8;
	DWORD dwLayoutAfterChange = wcstoul(szAfterChange, &pszEnd, 16);
	// Запомнить, что получилось в m_LayoutNames
	int i, iUnused = -1;

	for (i = 0; i < (int)countof(m_LayoutNames); i++)
	{
		if (!m_LayoutNames[i].bUsed)
		{
			if (iUnused == -1) iUnused = i; continue;
		}

		if (m_LayoutNames[i].klName == dwLayoutAfterChange)
		{
			iUnused = -1; break;
		}
	}

	if (iUnused != -1)
	{
		DEBUGTEST(HKL hkl = GetKeyboardLayout(0));
		#ifdef _DEBUG // due to unittests
		_ASSERTE((DWORD_PTR)hkl == (DWORD_PTR)lParam);
		#endif
		StoreLayoutName(iUnused, dwLayoutAfterChange, (HKL)(DWORD_PTR)lParam);
	}

	//}
#ifdef _DEBUG
	HKL hkl = GetKeyboardLayout(0);
	swprintf_c(szMsg, L"ConEmu: GetKeyboardLayout(0) after DefWindowProc(%s) = 0x%08I64X)\n",
	          (messg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST",
	          (unsigned __int64)(DWORD_PTR)hkl);
	DEBUGSTRLANG(szMsg);
#endif

	// в Win7x64 WM_INPUTLANGCHANGEREQUEST вообще не приходит, по крайней мере при переключении мышкой
	if (messg == WM_INPUTLANGCHANGEREQUEST || messg == WM_INPUTLANGCHANGE)
	{
		CVConGuard VCon;
		CRealConsole* pRCon = (GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : nullptr;

		if (pRCon
			&& !(nLastMsg == WM_INPUTLANGCHANGEREQUEST && messg == WM_INPUTLANGCHANGE
				&& ((lLastPrm == lParam) || (lLastPrm == 0))
				))
		{
			pRCon->SwitchKeyboardLayout(
			    (messg == WM_INPUTLANGCHANGEREQUEST) ? wParam : 0, lParam
			);
		}

		// -- Эффекта не имеет
		//if (pRCon)
		//{
		//	HWND hGuiWnd = pRCon->GuiWnd();
		//	if (hGuiWnd)
		//		pRCon->PostConsoleMessage(hGuiWnd, messg, wParam, lParam);
		//}
	}

	m_ActiveKeybLayout = (DWORD_PTR)lParam;

	mp_Status->OnKeyboardChanged();

	//  if (isFar() && gpSet->isLangChangeWsPlugin)
	//  {
	//   //LONG lLastLang = GetWindowLong ( ghWnd DC, GWL_LANGCHANGE );
	//   //SetWindowLong ( ghWnd DC, GWL_LANGCHANGE, lParam );
	//
	//   /*if (lLastLang == lParam)
	//    return result;*/
	//
	//CConEmuPipe pipe(GetFarPID(), 10);
	//if (pipe.Init(_T("CConEmuMain::OnLangChange"), FALSE))
	//{
	//  if (pipe.Execute(CMD_LANGCHANGE, &lParam, sizeof(LPARAM)))
	//  {
	//      //DebugStep(_T("ConEmu: Switching language (1 sec)"));
	//      // Подождем немножко, проверим что плагин живой
	//      /*DWORD dwWait = WaitForSingleObject(pipe.hEventAlive, CONEMUALIVETIMEOUT);
	//      if (dwWait == WAIT_OBJECT_0)*/
	//          return result;
	//  }
	//}
	//  }
	//  //POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
	//  //SENDMESSAGE(ghConWnd, messg, wParam, lParam);
	//  //if (messg == WM_INPUTLANGCHANGEREQUEST)
	//  {
	//      //wParam Specifies the character set of the new locale.
	//      //lParam - HKL
	//      //ActivateKeyboardLayout((HKL)lParam, 0);
	//      //POSTMESSAGE(ghConWnd, messg, wParam, lParam, FALSE);
	//      //SENDMESSAGE(ghConWnd, messg, wParam, lParam);
	//  }
	//  if (messg == WM_INPUTLANGCHANGE)
	//  {
	//      //SENDMESSAGE(ghConWnd, WM_SETFOCUS, 0,0);
	//      //POSTMESSAGE(ghConWnd, WM_SETFOCUS, 0,0, TRUE);
	//      //POSTMESSAGE(ghWnd, WM_SETFOCUS, 0,0, TRUE);
	//  }
	return result;
}

// dwLayoutName содержит не HKL, а "имя" (i.e. "00030409") сконвертированное в DWORD
LRESULT CConEmuMain::OnLangChangeConsole(CVirtualConsole *apVCon, const DWORD adwLayoutName)
{
	if ((gpSet->isMonitorConsoleLang & 1) != 1)
		return 0;

	if (!isValid(apVCon))
		return 0;

	wchar_t szInfo[255];

	// bypass x64 debugger bug
	DWORD dwLayoutName = adwLayoutName;

	if (!dwLayoutName)
	{
		// Ubuntu, Wine. Get 0 here.
		swprintf_c(szInfo, L"CConEmuMain::OnLangChangeConsole (0x%08X), Wine=%i", dwLayoutName, (int)gbIsWine);
		LogString(szInfo);
		return 0;
	}

	if (!isMainThread())
	{
		// --> не "нравится" руслату обращение к раскладкам из фоновых потоков. Поэтому выполняется в основном потоке
		if (gpSet->isLogging(2))
		{
			swprintf_c(szInfo, L"CConEmuMain::OnLangChangeConsole (0x%08X), Posting to main thread", dwLayoutName);
			LogString(szInfo);
		}

		PostMessage(ghWnd, mn_ConsoleLangChanged, dwLayoutName, reinterpret_cast<LPARAM>(apVCon));
		return 0;
	}
	else
	{
		if (gpSet->isLogging(2))
		{
			swprintf_c(szInfo, L"CConEmuMain::OnLangChangeConsole (0x%08X), MainThread", dwLayoutName);
			LogString(szInfo);
		}
	}


	wchar_t szName[10]; swprintf_c(szName, L"%08X", dwLayoutName);
	HKL hkl;

	#ifdef _DEBUG
	//Sleep(2000);
	WCHAR szMsg[255];
	hkl = GetKeyboardLayout(0);
	swprintf_c(szMsg,
		L"ConEmu: GetKeyboardLayout(0) in OnLangChangeConsole after GetKeyboardLayout(0) = "
		WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"\n",
		WIN3264WSPRINT((DWORD_PTR)hkl));
	DEBUGSTRLANG(szMsg);
	//Sleep(2000);
	#endif

	#ifdef _DEBUG
	DEBUGSTRLANG(szName);
	#endif

	DWORD_PTR dwNewKeybLayout = dwLayoutName; //(DWORD_PTR)LoadKeyboardLayout(szName, 0);
	HKL hKeyb[20] = {};
	UINT nCount, i, j, s, nErr;
	SetLastError(0);
	nCount = GetKeyboardLayoutList(countof(hKeyb), hKeyb);
	nErr = GetLastError();
	/*
	HKL:
	0x0000000004090409 - US
	0x0000000004190419 - Russian
	0xfffffffff0010409 - US - International
	0xfffffffff0020409 - US - Dvorak
	0xfffffffff01a0409 - US - Dvorak left hand
	0xfffffffff01b0409 - US - Dvorak right hand
	Layout (dwLayoutName):
	0x00010409 - US - Dvorak
	0x00030409 - US - Dvorak left hand
	0x00040409 - US - Dvorak right hand
	*/
	BOOL lbFound = FALSE;
	int iUnused = -1, iUnknownLeft = -1;

	// Check first, this layout may be found already
	for (i = 0; i < countof(m_LayoutNames); i++)
	{
		if (!m_LayoutNames[i].bUsed)
		{
			if (iUnused == -1) iUnused = i;

			continue;
		}

		if (m_LayoutNames[i].klName == dwLayoutName)
		{
			lbFound = TRUE;
			dwNewKeybLayout = m_LayoutNames[i].hkl;
			iUnused = -1; // запоминать не потребуется
			break;
		}
	}

	// LCIDToLocaleName(dwLayoutName, ...)

	// s - number of items in (switch)
	for (s = 0; !lbFound && (s <= 5); s++)
	{
		for (i = 0; (i < nCount); i++)
		{
			// "классическая" раскладка, в которой ид раскладки совпадает с языком?
			if ((s == 0) && ((dwLayoutName & 0xFFFF) != dwLayoutName))
				continue;

			bool bIgnore = false;
			for (j = 0; j < countof(m_LayoutNames); j++)
			{
				if (!m_LayoutNames[j].bUsed)
					continue;

				if (m_LayoutNames[j].hkl == (DWORD_PTR)hKeyb[i])
				{
					bIgnore = true;
					break;
				}
			}
			if (bIgnore)
				continue;

			switch (s)
			{
			case 0:
				// это "классическая" раскладка, в которой ид раскладки совпадает с языком?
				lbFound = (((DWORD_PTR)hKeyb[i]) == (dwNewKeybLayout | (dwNewKeybLayout << 16)));
				break;
			case 1:
				// Issue 1035: dwLayoutName=0x00020409; hKeyb={0xfffffffff0010414,0xfffffffff0010409}
				// Любая (свободная/необработанная) раскладка для требуемого языка?
				lbFound = (HIWORD(hKeyb[i]) != LOWORD(hKeyb[i])) && (LOWORD(hKeyb[i]) == LOWORD(dwNewKeybLayout));
				//lbFound = ((((DWORD_PTR)hKeyb[i]) & 0xFFFFFFF) == dwNewKeybLayout);
				break;
			case 2:
				// По младшему слову
				lbFound = (LOWORD((DWORD_PTR)hKeyb[i]) == LOWORD(dwNewKeybLayout));
				break;
			case 3:
				// Если опять не нашли - то сравниваем старшее (HIWORD) слово раскладки
				lbFound = (HIWORD((DWORD_PTR)hKeyb[i]) == LOWORD(dwNewKeybLayout));
				break;
			case 4:
				// Last chance - Primary Language ID
				lbFound = ((LOWORD((DWORD_PTR)hKeyb[i]) & 0xFF) == (LOWORD(dwNewKeybLayout) & 0xFF));
				break;
			case 5:
				// If there is only "unlinked" HKL - choose it
				iUnknownLeft = 0;
				for (UINT i1 = 0; i1 < nCount; i1++)
				{
					bool bKnown = false;
					for (size_t i2 = 0; i2 < countof(m_LayoutNames); i2++)
					{
						if (!m_LayoutNames[i2].bUsed)
							continue;

						if (m_LayoutNames[i2].hkl == (DWORD_PTR)hKeyb[i1])
						{
							bKnown = true;
							break;
						}
					}
					if (!bKnown)
					{
						iUnknownLeft++;
					}
				}
				lbFound = (iUnknownLeft == 1);
				wsprintf(szInfo, L"hkl=0x%08X, dwLayout=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L" -- Warning, strange layout ID", dwLayoutName, WIN3264WSPRINT(hKeyb[i]));
				LogString(szInfo);
				break;
			//case 3:
			//	// Last change - check LOWORD of both HKL & Layout
			//	// Issue 1035: dwLayoutName=0x00020409; hKeyb={0xfffffffff0010414,0xfffffffff0010409}
			//	lbFound = (LOWORD((DWORD_PTR)hKeyb[i]) == LOWORD(dwNewKeybLayout));
			//	break;
			default:
				Assert(FALSE && "Invalid index");
			}

			if (lbFound)
			{
				dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
				break;
			}
		}
	}

	//// Если не нашли, и это "классическая" раскладка, в которой ид раскладки совпадает с языком
	//if (!lbFound && ((dwLayoutName & 0xFFFF) == dwLayoutName))
	//{
	//	DWORD_PTR dwTest = dwNewKeybLayout | (dwNewKeybLayout << 16);

	//	for (i = 0; i < nCount; i++)
	//	{
	//		if (((DWORD_PTR)hKeyb[i]) == dwTest)
	//		{
	//			lbFound = TRUE;
	//			dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
	//			break;
	//		}
	//	}

	//	// Если опять не нашли - то сравниваем старшее (HIWORD) слово раскладки
	//	if (!lbFound)
	//	{
	//		WORD wTest = LOWORD(dwNewKeybLayout);

	//		for (i = 0; i < nCount; i++)
	//		{
	//			if (HIWORD((DWORD_PTR)hKeyb[i]) == wTest)
	//			{
	//				lbFound = TRUE;
	//				dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
	//				break;
	//			}
	//		}
	//	}
	//}

	//// Last change - check LOWORD of both HKL & Layout
	//// Issue 1035: dwLayoutName=0x00020409; hKeyb={0xfffffffff0010414,0xfffffffff0010409}
	//if (!lbFound)
	//{
	//	WORD wTest = LOWORD(dwNewKeybLayout);

	//	for (i = 0; i < nCount; i++)
	//	{
	//		if (LOWORD((DWORD_PTR)hKeyb[i]) == wTest)
	//		{
	//			lbFound = TRUE;
	//			dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
	//			break;
	//		}
	//	}
	//}

	if ((!lbFound && gpSet->isLogging()) || gpSet->isLogging(2))
	{
		swprintf_c(szInfo, L"Keyboard layout lookup (0x%08X) %s", dwLayoutName, lbFound ? L"Succeeded" : L"FAILED");
		LogString(szInfo);

		hkl = GetKeyboardLayout(0);
		swprintf_c(szInfo,
			L"  Current keyboard layout:\r\n       " WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"\n",
			WIN3264WSPRINT((DWORD_PTR)hkl));
		LogString(szInfo, false, false);

		LogString(L"  Installed keyboard layouts:\r\n", false, false);
		for (i = 0; i < nCount; i++)
		{
			swprintf_c(szInfo,
				L"    %u: " WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"\n",
				i+1, WIN3264WSPRINT(hKeyb[i]));
			LogString(szInfo, false, false);
		}
	}

	if (!lbFound)
	{
		swprintf_c(szInfo, L"dwLayoutName not found: Count=%u,Left=%i,Err=%u,%08X\r\n", nCount, iUnknownLeft, nErr, dwLayoutName);
		AppendHKL(szInfo, countof(szInfo), hKeyb, nCount);

		// Загружать НОВЫЕ раскладки - некорректно (Issue 944)
		AssertMsg(szInfo);

		//wchar_t szLayoutName[9] = {0};
		//swprintf_c(szLayoutName, L"%08X", dwLayoutName);

		//if (gpSet->isLogging(2))
		//{
		//	WCHAR szInfo[255];
		//	swprintf_c(szInfo, L"CConEmuMain::OnLangChangeConsole -> LoadKeyboardLayout(0x%08X)", dwLayoutName);
		//	LogString(szInfo);
		//}

		//dwNewKeybLayout = (DWORD_PTR)LoadKeyboardLayout(szLayoutName, 0);

		//if (gpSet->isLogging(2))
		//{
		//	WCHAR szInfo[255];
		//	swprintf_c(szInfo, L"CConEmuMain::OnLangChangeConsole -> LoadKeyboardLayout()=0x%08X", (DWORD)dwNewKeybLayout);
		//	LogString(szInfo);
		//}

		//lbFound = TRUE;
	}

	if (lbFound && iUnused != -1)
	{
		StoreLayoutName(iUnused, dwLayoutName, (HKL)dwNewKeybLayout);
	}

	UNREFERENCED_PARAMETER(nCount);

	//dwNewKeybLayout = (DWORD_PTR)hklNew;
	//for (i = 0; !lbFound && i < nCount; i++)
	//{
	//	if (hKeyb[i] == (HKL)dwNewKeybLayout)
	//		lbFound = TRUE;
	//}
	//WARNING("Похоже с другими раскладками будет глючить. US Dvorak?");
	//for (i = 0; !lbFound && i < nCount; i++)
	//{
	//	if ((((DWORD_PTR)hKeyb[i]) & 0xFFFF) == (dwNewKeybLayout & 0xFFFF))
	//	{
	//		lbFound = TRUE; dwNewKeybLayout = (DWORD_PTR)hKeyb[i];
	//	}
	//}
	//// Если не задана раскладка (только язык?) формируем по умолчанию
	//if (!lbFound && (dwNewKeybLayout == (dwNewKeybLayout & 0xFFFF)))
	//{
	//	dwNewKeybLayout |= (dwNewKeybLayout << 16);
	//}
#ifdef _DEBUG
	DEBUGSTRLANG(L"ConEmu: Calling GetKeyboardLayout(0)\n");
	//Sleep(2000);
	hkl = GetKeyboardLayout(0);
	swprintf_c(szMsg, L"ConEmu: GetKeyboardLayout(0) after LoadKeyboardLayout = 0x%08I64X\n",
	          (unsigned __int64)(DWORD_PTR)hkl);
	DEBUGSTRLANG(szMsg);
	//Sleep(2000);
#endif

	if (apVCon->isActive(false))
	{
		apVCon->RCon()->OnConsoleLangChange(dwNewKeybLayout);
	}

	return 0;
}

void CConEmuMain::SetSkipMouseEvent(UINT nMsg1, UINT nMsg2, UINT nReplaceDblClk)
{
	mouse.nSkipEvents[0] = nMsg1 /*WM_LBUTTONDOWN*/;
	mouse.nSkipEvents[1] = nMsg2 /*WM_LBUTTONUP*/;
	mouse.nReplaceDblClk = nReplaceDblClk /*WM_LBUTTONDBLCLK*/;
}

bool CConEmuMain::PatchMouseEvent(UINT messg, POINT& ptCurClient, POINT& ptCurScreen, WPARAM wParam, bool& isPrivate)
{
	isPrivate = false;

	// Для этих сообщений, lParam - relative to the upper-left corner of the screen.
	if (messg == WM_MOUSEWHEEL || messg == WM_MOUSEHWHEEL)
		ScreenToClient(ghWnd, &ptCurClient);
	else // Для остальных lParam содержит клиентские координаты
		ClientToScreen(ghWnd, &ptCurScreen);

	if (mb_MouseCaptured
		|| (messg == WM_LBUTTONDOWN) || (messg == WM_RBUTTONDOWN) || (messg == WM_MBUTTONDOWN)
		)
	{
		HWND hChild = ::ChildWindowFromPointEx(ghWnd, ptCurClient, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);

		CVConGuard VCon;
		if (isVConHWND(hChild, &VCon)/*(hChild != ghWnd)*/) // Это должна быть VCon
		{
			#ifdef _DEBUG
			wchar_t szClass[128]; GetClassName(hChild, szClass, countof(szClass));
			_ASSERTE(lstrcmp(szClass, VirtualConsoleClass)==0 && "This must be VCon DC window");
			#endif

			//bool bSkipThisEvent = false;

			// WARNING! Тут строго, без учета активности группы!
			if (VCon.VCon() && VCon->isVisible() && !VCon->isActive(false))
			{
				// по клику - активировать кликнутый сплит
				if ((messg == WM_LBUTTONDOWN) || (messg == WM_RBUTTONDOWN) || (messg == WM_MBUTTONDOWN))
				{
					Activate(VCon.VCon());
				}

				if (gpSet->isMouseSkipActivation)
				{
					if (messg == WM_LBUTTONDOWN)
					{
						SetSkipMouseEvent(WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK);
						//bSkipThisEvent = true;
					}
					else if (messg == WM_RBUTTONDOWN)
					{
						SetSkipMouseEvent(WM_RBUTTONDOWN, WM_RBUTTONUP, WM_RBUTTONDBLCLK);
						//bSkipThisEvent = true;
					}
					else if (messg == WM_MBUTTONDOWN)
					{
						SetSkipMouseEvent(WM_MBUTTONDOWN, WM_MBUTTONUP, WM_MBUTTONDBLCLK);
						//bSkipThisEvent = true;
					}
				}
			}


			// Если активны PanelView - они могут транслировать координаты
			POINT ptVConCoord = ptCurClient;
			::MapWindowPoints(ghWnd, hChild, &ptVConCoord, 1);
			HWND hPanelView = ::ChildWindowFromPointEx(hChild, ptVConCoord, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);
			if (hPanelView && (hPanelView != hChild))
			{
				// LClick пропускать в консоль нельзя, чтобы не возникало глюков с позиционированием по клику...
				isPrivate = (messg == WM_LBUTTONDOWN);

				if (mb_MouseCaptured)
				{
					#ifdef _DEBUG
					GetClassName(hPanelView, szClass, countof(szClass));
					_ASSERTE(lstrcmp(szClass, ConEmuPanelViewClass)==0 && "This must be Thumbnail or Tile window");
					#endif

					::MapWindowPoints(hChild, hPanelView, &ptVConCoord, 1);

					DWORD_PTR lRc = (DWORD_PTR)-1;
					if (SendMessageTimeout(hPanelView, mn_MsgPanelViewMapCoord, MAKELPARAM(ptVConCoord.x,ptVConCoord.y), 0, SMTO_NORMAL, PNLVIEWMAPCOORD_TIMEOUT, &lRc) && (lRc != (DWORD_PTR)-1))
					{
						ptCurClient.x = LOWORD(lRc);
						ptCurClient.y = HIWORD(lRc);
						ptCurScreen = ptCurClient;
						ClientToScreen(ghWnd, &ptCurScreen);
					}
				}
			}
		}
	}

	if (messg == WM_LBUTTONDBLCLK)
	{
		mouse.LDblClkDC = ptCurClient;
		mouse.LDblClkTick = TimeGetTime();
	}
	else if ((mouse.lastMsg == WM_LBUTTONDBLCLK) && ((messg == WM_MOUSEMOVE) || (messg == WM_LBUTTONUP)))
	{
		// Тачпады и тачскрины.
		// При двойном тапе может получаться следующая фигня:
		//17:40:25.787(gui.4460) GUI::Mouse WM_ MOUSEMOVE at screen {603x239} x00000000
		//17:40:25.787(gui.4460) GUI::Mouse WM_ LBUTTONDOWN at screen {603x239} x00000001
		//17:40:25.787(gui.4460) GUI::Mouse WM_ LBUTTONUP at screen {603x239} x00000000
		//17:40:25.787(gui.4460) GUI::Mouse WM_ MOUSEMOVE at screen {603x239} x00000000
		//17:40:25.880(gui.4460) GUI::Mouse WM_ LBUTTONDBLCLK at screen {603x239} x00000001
		//17:40:25.880(gui.4460) GUI::Mouse WM_ MOUSEMOVE at screen {598x249} x00000001
		//17:40:25.927(gui.4460) GUI::Mouse WM_ LBUTTONUP at screen {598x249} x00000000
		//17:40:25.927(gui.4460) GUI::Mouse WM_ MOUSEMOVE at screen {598x249} x00000000
		// Т.е. место второго тапа немного смещено (95% случаев)
		// В итоге, винда присылает MOUSEMOVE & LBUTTONUP для этого смещения, что нежелательно,
		// т.к. Far, например, после двойного тапа по папке выполняет позиционирование
		// на файл в этой папке. Как если бы после DblClick был еще один лишний Click.
		DWORD dwDelta = TimeGetTime() - mouse.LDblClkTick;
		if (dwDelta <= TOUCH_DBLCLICK_DELTA)
		{
			if (messg == WM_MOUSEMOVE)
			{
				if ((wParam & (MK_LBUTTON|MK_RBUTTON|MK_MBUTTON)) == MK_LBUTTON)
				{
					return false; // Не пропускать это событие в консоль
				}
			}
			else
			{
				_ASSERTE(messg==WM_LBUTTONUP);
				// А тут мы скорректируем позицию, чтобы консоль думала, что "курсор не двигался"
				ptCurClient = mouse.LDblClkDC;
				ptCurScreen = ptCurClient;
				//MapWindowPoints(
				ClientToScreen(ghWnd, &ptCurScreen);
			}
		}
	}

	return true;
}

LRESULT CConEmuMain::OnMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	// кто его знает, в каких координатах оно пришло...
	//short winX = GET_X_LPARAM(lParam);
	//short winY = GET_Y_LPARAM(lParam);

	if (gpSet->isLogging())
		CVConGroup::LogInput(messg, wParam, lParam);

	if (isMenuActive())
	{
		if (mp_Tip)
		{
			mp_Tip->HideTip();
		}

		if (gpSet->isLogging())
			LogString("Mouse event skipped due to (isMenuActive())");

		return 0;
	}

	LRESULT lRc = 0;

	#if defined(CONEMU_TABBAR_EX)
	if (mp_TabBar->OnMouse(hWnd, messg, wParam, lParam, lRc))
		return lRc;
	#endif

	wchar_t szDbg[200];

	//2010-05-20 все-таки будем ориентироваться на lParam, потому что
	//  только так ConEmuTh может передать корректные координаты
	//POINT ptCur = {-1, -1}; GetCursorPos(&ptCur);
	POINT ptCurClient = {(int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam)};
	POINT ptCurScreen = ptCurClient;
	// MouseWheel event must get SCREEN coordinates
	if (messg == WM_MOUSEWHEEL || messg == WM_MOUSEHWHEEL)
	{
		POINT ptRealScreen = {}; GetCursorPos(&ptRealScreen);

		wchar_t szKeys[100] = L"";
		if (wParam & MK_CONTROL)  wcscat_c(szKeys, L" Ctrl");
		if (wParam & MK_LBUTTON)  wcscat_c(szKeys, L" LBtn");
		if (wParam & MK_MBUTTON)  wcscat_c(szKeys, L" MBtn");
		if (wParam & MK_RBUTTON)  wcscat_c(szKeys, L" RBtn");
		if (wParam & MK_XBUTTON1) wcscat_c(szKeys, L" XBtn1");
		if (wParam & MK_XBUTTON2) wcscat_c(szKeys, L" XBtn2");
		swprintf_c(szDbg,
			L"%s Dir:%i%s LParam:{%i,%i} Real:{%i,%i}\n",
			(messg == WM_MOUSEWHEEL) ? L"Wheel" : L"HWheel",
			(int)(short)HIWORD(wParam), szKeys,
			(int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam),
			ptRealScreen.x, ptRealScreen.y);
		if (mp_Log)
		{
			LogString(szDbg, true, false);
		}
		else
		{
			DEBUGSTRMOUSEWHEEL(szDbg);
		}

		// gh#216: Mouse wheel only works on first console in split window
		// https://conemu.github.io/en/MicrosoftBugs.html#WM_MOUSEWHEEL-10
		if (IsWin10())
		{
			ptCurClient = ptCurScreen = ptRealScreen;
		}

		// Now, get VCon under mouse cursor
		CVConGuard VCon;
		if (CVConGroup::GetVConFromPoint(ptCurScreen, &VCon))
		{
			CRealConsole* pRCon = VCon->RCon();
			HWND hGuiChild = pRCon->GuiWnd();
			if (hGuiChild && pRCon->isGuiOverCon())
			{
				// Just resend it to child GUI
				#if 0
				bWasSendToChildGui = true;
				#endif
				LogString(L"-- forwarding to ChildGui\n", true, false);
				VCon->RCon()->PostConsoleMessage(hGuiChild, messg, wParam, lParam);
				return 0;
			}
		}
		else
		{
			swprintf_c(szDbg, L"-- !GetVConFromPoint: {%i,%i}\n", ptCurScreen.x, ptCurScreen.y);
			LogString(szDbg, true, false);
		}
	}

	// Коррекция координат или пропуск сообщений
	bool isPrivate = false;
	bool bContinue = PatchMouseEvent(messg, ptCurClient, ptCurScreen, wParam, isPrivate);

#ifdef _DEBUG
	swprintf_c(szDbg, L"GUI::Mouse %s at screen {%ix%i} x%08X%s\n",
		GetMouseMsgName(messg),
		ptCurScreen.x, ptCurScreen.y, (DWORD)wParam,
		bContinue ? L"" : L" - SKIPPED!");
	DEBUGSTRMOUSE(szDbg);
#endif

	//gpSet->IsModifierPressed(vkWndDragKey, false))
	if ((messg == WM_LBUTTONDOWN)
		&& gpSet->isMouseDragWindow
		// 120820 - двигать можно только в "Normal"
		&& !(isZoomed() || isFullScreen() || gpSet->isQuakeStyle)
		&& ProcessHotKeyMsg(WM_KEYDOWN, VK_LBUTTON, 0, nullptr, nullptr))
	{
		mouse.state |= MOUSE_WINDOW_DRAG;
		GetCursorPos(&mouse.ptWndDragStart);
		GetWindowRect(ghWnd, &mouse.rcWndDragStart);
		SetCapture(ghWnd);
		SetCursor(LoadCursor(nullptr,IDC_SIZEALL));
		return 0;
	}
	else if (mouse.state & MOUSE_WINDOW_DRAG)
	{
		if ((messg == WM_LBUTTONUP) || !isPressed(VK_LBUTTON))
		{
			mouse.state &= ~MOUSE_WINDOW_DRAG;
			ReleaseCapture();
		}
		else
		{
			POINT ptCurPos = {};
			GetCursorPos(&ptCurPos);
			RECT rcNew = {
				mouse.rcWndDragStart.left  + (ptCurPos.x - mouse.ptWndDragStart.x),
				mouse.rcWndDragStart.top   + (ptCurPos.y - mouse.ptWndDragStart.y)
			};
			rcNew.right = rcNew.left + (mouse.rcWndDragStart.right - mouse.rcWndDragStart.left);
			rcNew.bottom = rcNew.top + (mouse.rcWndDragStart.bottom - mouse.rcWndDragStart.top);

			int pre_x = rcNew.left, pre_y = rcNew.top;
			OnMoving(&rcNew, true);
			if (rcNew.left != pre_x)
				mouse.ptWndDragStart.x += (pre_x - rcNew.left);
			if (rcNew.top != pre_y)
				mouse.ptWndDragStart.y += (pre_y - rcNew.top);

			TODO("Desktop mode?");
			MoveWindowRect(ghWnd, rcNew, TRUE);
		}
		return 0;
	}


	// Обработать клики на StatusBar
	if (mp_Status->ProcessStatusMessage(hWnd, messg, wParam, lParam, ptCurClient, lRc))
	{
		LogString(L"-- mouse event skipped (StatusBar event)\n", true, false);
		return lRc;
	}

	if (!bContinue)
	{
		LogString(L"-- mouse event skipped\n", true, false);
		return 0;
	}

	if (mp_Inside && ((messg == WM_LBUTTONDOWN) || (messg == WM_RBUTTONDOWN)))
	{
		#ifdef _DEBUG
		HWND hFocus = GetFocus();
		HWND hFore = GetForegroundWindow();
		#endif
		apiSetForegroundWindow(ghWnd);
		SetFocus(ghWnd);
	}


	TODO("DoubleView. Хорошо бы колесико мышки перенаправлять в консоль под мышиным курсором, а не в активную");
	RECT /*conRect = {0},*/ dcRect = {0};
	//GetWindowRect('ghWnd DC', &dcRect);
	CVirtualConsole* pVCon = nullptr;
	CVConGuard VCon;

	if (CVConGroup::GetVConFromPoint(ptCurScreen, &VCon))
	{
		pVCon = VCon.VCon();
	}
	else if (mb_MouseCaptured && CVConGroup::GetActiveVCon(&VCon) >= 0)
	{
		if (VCon->RCon()->isMouseSelectionPresent())
			pVCon = VCon.VCon();
		else
			VCon.Release();
	}
	CRealConsole *pRCon = pVCon ? pVCon->RCon() : nullptr;
	HWND hView = pVCon ? pVCon->GetView() : nullptr;
	if (hView)
	{
		GetWindowRect(hView, &dcRect);
		MapWindowPoints(nullptr, ghWnd, (LPPOINT)&dcRect, 2);
	}

	HWND hChild = ::ChildWindowFromPointEx(ghWnd, ptCurClient, CWP_SKIPINVISIBLE|CWP_SKIPDISABLED|CWP_SKIPTRANSPARENT);


	//enum DragPanelBorder dpb = DragPanelBorder::None; //CConEmuMain::CheckPanelDrag(COORD crCon)
	if (isWheelEvent(messg) && HIWORD(wParam))
	{
		BYTE vk = 0;
		if (messg == WM_MOUSEWHEEL)
			vk =  (((short)(WORD)HIWORD(wParam)) > 0) ? VK_WHEEL_UP : VK_WHEEL_DOWN;
		else if (messg == 0x020E/*WM_MOUSEHWHEEL*/)
			vk =  (((short)(WORD)HIWORD(wParam)) > 0) ? VK_WHEEL_RIGHT : VK_WHEEL_LEFT; // Если MSDN не врет - проверить не на чем

		// Зовем "виртуальное" кнопочное нажатие (если назначено)
		if (vk && ProcessHotKeyMsg(WM_KEYDOWN, vk, 0, nullptr, pRCon))
		{
			// назначено, в консоль не пропускаем
			swprintf_c(szDbg, L"-- %s skipped (HotKey)\n", (messg == WM_MOUSEWHEEL) ? L"Wheel" : L"HWheel");
			LogString(szDbg, true, false);
			return 0;
		}
	}
	else if (messg >= WM_LBUTTONDOWN && messg <= WM_MBUTTONDBLCLK)
	{
		BYTE vk;
		if (messg >= WM_LBUTTONDOWN && messg <= WM_LBUTTONDBLCLK)
			vk = VK_LBUTTON;
		else if (messg >= WM_RBUTTONDOWN && messg <= WM_RBUTTONDBLCLK)
			vk = VK_RBUTTON;
		else
			vk = VK_MBUTTON;

		UpdateControlKeyState();

		// Selection, copy/pase
		if (pRCon && pRCon->OnMouseSelection(messg, wParam, ptCurClient.x - dcRect.left, ptCurClient.y - dcRect.top))
		{
			return 0;
		}

		bool bDn = (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_MBUTTONDOWN);
		bool bUp = (messg == WM_LBUTTONUP || messg == WM_RBUTTONUP || messg == WM_MBUTTONUP);

		const ConEmuHotKey* pHotKey = gpHotKeys->GetHotKeyInfo(ChordFromVk(vk), bDn, pRCon);

		if (pHotKey)
		{
			if ((pHotKey != ConEmuSkipHotKey) && (bDn || bUp))
			{
				// Зовем "виртуальное" кнопочное нажатие
				ProcessHotKeyMsg(bDn ? WM_KEYDOWN : WM_KEYUP, vk, 0, nullptr, pRCon);
			}

			// назначено, в консоль не пропускаем
			return 0;
		}
	}

	//BOOL lbMouseWasCaptured = mb_MouseCaptured;
	if (!mb_MouseCaptured)
	{
		// Если клик
		if ((hView && hChild == hView) &&
			(messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_MBUTTONDOWN
			|| (isPressed(VK_LBUTTON) || isPressed(VK_RBUTTON) || isPressed(VK_MBUTTON))))
		{
			// В клиентской области (области отрисовки)
			//if (hView /*PtInRect(&dcRect, ptCurClient)*/)
			//{
			mb_MouseCaptured = TRUE;
			//TODO("После скрытия ViewPort наверное SetCapture нужно будет делать на ghWnd");
			SetCapture(ghWnd); // 100208 было 'ghWnd DC'
			//}
		}
	}
	else
	{
		// Все кнопки мышки отпущены - release
		if (!isPressed(VK_LBUTTON) && !isPressed(VK_RBUTTON) && !isPressed(VK_MBUTTON))
		{
			ReleaseCapture();
			mb_MouseCaptured = FALSE;

			if (pRCon && pRCon->isMouseButtonDown())
			{
				if (ptCurClient.x < dcRect.left)
					ptCurClient.x = dcRect.left;
				else if (ptCurClient.x >= dcRect.right)
					ptCurClient.x = dcRect.right-1;

				if (ptCurClient.y < dcRect.top)
					ptCurClient.y = dcRect.top;
				else if (ptCurClient.y >= dcRect.bottom)
					ptCurClient.y = dcRect.bottom-1;
			}
		}
	}

	if (!pVCon)
	{
		if (isWheelEvent(messg))
			LogString(L"-- skipped (!pVCon)\n", true, false);
		return 0;
	}

	if (gpFontMgr->FontWidth()==0 || gpFontMgr->FontHeight()==0)
	{
		if (isWheelEvent(messg))
			LogString(L"-- skipped (bad font size)\n", true, false);
		return 0;
	}

	if ((messg==WM_LBUTTONUP || messg==WM_MOUSEMOVE) && (mouse.state & MOUSE_SIZING_DBLCKL))
	{
		if (messg==WM_LBUTTONUP)
			mouse.state &= ~MOUSE_SIZING_DBLCKL;

		return 0; //2009-04-22 После DblCkl по заголовку в консоль мог проваливаться LBUTTONUP
	}

	//-- Вообще, если курсор над полосой прокрутки - любые события мышки в консоль не нужно слать...
	//-- Но если мышка над скроллом - то обработкой Wheel занимается RealConsole и слать таки нужно...
	if (TrackMouse())
	{
		if (isWheelEvent(messg))
			LogString(L"-- skipped (over ScrollBar)\n", true, false);
		if (pRCon && ((messg == WM_MOUSEWHEEL) || (messg == WM_MOUSEHWHEEL)))
			pRCon->OnScroll(messg, wParam, ptCurClient.x, ptCurClient.y);
		return 0;
	}

	// Selection, copy/pase
	if (pRCon && pRCon->OnMouseSelection(messg, wParam, ptCurClient.x - dcRect.left, ptCurClient.y - dcRect.top))
	{
		if (isWheelEvent(messg))
			LogString(L"-- skipped (over ScrollBar)\n", true, false);
		return 0;
	}

	if (pRCon && isWheelEvent(messg))
	{
		if (pRCon->isInternalScroll())
		{
			LogString(L"-- skipped (isInternalScroll)\n", true, false);
			pRCon->OnScroll(messg, wParam, ptCurClient.x, ptCurClient.y);
			return 0;
		}
	}

	// Иначе в консоль проваливаются щелчки по незанятому полю таба...
	// все, что кроме - считаем кликами и они должны попадать в dcRect
	if (messg != WM_MOUSEMOVE && messg != WM_MOUSEWHEEL && messg != WM_MOUSEHWHEEL)
	{
		if (mouse.state & MOUSE_SIZING_DBLCKL)
			mouse.state &= ~MOUSE_SIZING_DBLCKL;

		if (hView == nullptr /*!PtInRect(&dcRect, ptCur)*/)
		{
			DEBUGLOGFILE("Click outside of DC");
			return 0;
		}
	}

	// Переводим в клиентские (относительно hView) координаты
	ptCurClient.x -= dcRect.left; ptCurClient.y -= dcRect.top;
	//WARNING("Избавляться от ghConWnd. Должно быть обращение через классы");
	//::GetClientRect(ghConWnd, &conRect);
	COORD cr = pVCon->ClientToConsole(ptCurClient.x,ptCurClient.y);
	short conX = cr.X; //winX/gpSet->Log Font.lfWidth;
	short conY = cr.Y; //winY/gpSet->Log Font.lfHeight;

	if ((messg != WM_MOUSEWHEEL && messg != WM_MOUSEHWHEEL) && (conX<0 || conY<0))
	{
		DEBUGLOGFILE("Mouse outside of upper-left");
		return 0;
	}

	//* ****************************
	//* Если окно ConEmu не в фокусе - не слать к консоль движение мышки,
	//* иначе получается неприятная реакция пунктов меню и т.п.
	//* ****************************
	if (gpSet->isMouseSkipMoving && !isMeForeground(false,false))
	{
		DEBUGLOGFILE("ConEmu is not foreground window, mouse event skipped");
		if (isWheelEvent(messg))
			LogString(L"-- skipped (no focus)\n", true, false);
		return 0;
	}

	//* ****************************
	//* После получения WM_MOUSEACTIVATE опционально можно не пропускать
	//* клик в консоль, чтобы при активации ConEmu случайно не задеть
	//* (закрыть или отменить) висящий в FAR диалог
	//* ****************************
	if ((mouse.nSkipEvents[0] && mouse.nSkipEvents[0] == messg)
	        || (mouse.nSkipEvents[1]
	            && (mouse.nSkipEvents[1] == messg || messg == WM_MOUSEMOVE)))
	{
		if (mouse.nSkipEvents[0] == messg)
		{
			mouse.nSkipEvents[0] = 0;
			DEBUGSTRMOUSE(L"Skipping Mouse down\n");
		}
		else if (mouse.nSkipEvents[1] == messg)
		{
			mouse.nSkipEvents[1] = 0;
			DEBUGSTRMOUSE(L"Skipping Mouse up\n");
		}

		#ifdef _DEBUG
		else if (messg == WM_MOUSEMOVE)
		{
			DEBUGSTRMOUSE(L"Skipping Mouse move\n");
		}
		#endif

		DEBUGLOGFILE("ConEmu was not foreground window, mouse activation event skipped");
		LogString(L"-- skipped (activation event skipped)\n", true, false);
		return 0;
	}

	//* ****************************
	//* После получения WM_MOUSEACTIVATE и включенном gpSet->isMouseSkipActivation
	//* двойной клик переслать в консоль как одинарный, иначе после активации
	//* мышкой быстрый клик в том же месте будет обработан неправильно
	//* ****************************
	if (mouse.nReplaceDblClk)
	{
		if (!gpSet->isMouseSkipActivation)
		{
			mouse.nReplaceDblClk = 0;
		}
		else
		{
			if (messg == WM_LBUTTONDOWN && mouse.nReplaceDblClk == WM_LBUTTONDBLCLK)
			{
				mouse.nReplaceDblClk = 0;
			}
			else if (messg == WM_RBUTTONDOWN && mouse.nReplaceDblClk == WM_RBUTTONDBLCLK)
			{
				mouse.nReplaceDblClk = 0;
			}
			else if (messg == WM_MBUTTONDOWN && mouse.nReplaceDblClk == WM_MBUTTONDBLCLK)
			{
				mouse.nReplaceDblClk = 0;
			}
			else if (mouse.nReplaceDblClk == messg)
			{
				switch (mouse.nReplaceDblClk)
				{
					case WM_LBUTTONDBLCLK:
						messg = WM_LBUTTONDOWN;
						break;
					case WM_RBUTTONDBLCLK:
						messg = WM_RBUTTONDOWN;
						break;
					case WM_MBUTTONDBLCLK:
						messg = WM_MBUTTONDOWN;
						break;
				}

				mouse.nReplaceDblClk = 0;
			}
		}
	}

	// Forwarding сообщений в нить драга
	if (mp_DragDrop && mp_DragDrop->IsDragStarting())
	{
		if (mp_DragDrop->ForwardMessage(messg, wParam, lParam))
		{
			if (isWheelEvent(messg))
				LogString(L"-- skipped (Drag&Drop)\n", true, false);
			return 0;
		}
	}

	// -- поскольку выделялка у нас теперь своя - этого похоже не требуется. лишние движения окна
	// -- вызывают side-effects на некоторых системах. Issue 274: Окно реальной консоли позиционируется в неудобном месте
	/////*&& isPressed(VK_LBUTTON)*/) && // Если этого не делать - при выделении мышкой консоль может самопроизвольно прокрутиться
	//// Только при скрытой консоли. Иначе мы ConEmu видеть вообще не будем
	//if (pRCon && (/*pRCon->isBufferHeight() ||*/ !pRCon->isWindowVisible())
	//    && (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_LBUTTONDBLCLK || messg == WM_RBUTTONDBLCLK
	//        || (messg == WM_MOUSEMOVE && isPressed(VK_LBUTTON)))
	//    )
	//{
	//   // buffer mode: cheat the console window: adjust its position exactly to the cursor
	//   RECT win;
	//   GetWindowRect(ghWnd, &win);
	//   short x = win.left + ptCurClient.x - MulDiv(ptCurClient.x, conRect.right, std::max<uint>(1, pVCon->Width));
	//   short y = win.top + ptCurClient.y - MulDiv(ptCurClient.y, conRect.bottom, std::max<uint>(1, pVCon->Height));
	//   RECT con;
	//   GetWindowRect(ghConWnd, &con);
	//   if (con.left != x || con.top != y)
	//       MOVEWINDOW(ghConWnd, x, y, con.right - con.left + 1, con.bottom - con.top + 1, TRUE);
	//}

	if (!CVConGroup::isFar())
	{
		if (messg != WM_MOUSEMOVE) { DEBUGLOGFILE("FAR not active, all clicks forced to console"); }

		goto fin;
	}

	// Для этих сообщений, lParam - relative to the upper-left corner of the screen.
	if (messg == WM_MOUSEWHEEL || messg == WM_MOUSEHWHEEL)
		lParam = MAKELONG((short)ptCurScreen.x, (short)ptCurScreen.y);
	else
		lParam = MAKELONG((short)ptCurClient.x, (short)ptCurClient.y);

	// Запомним последнее мышиное событие (для PatchMouseEvent)
	mouse.lastMsg = messg;

	// Теперь можно обрабатывать мышку, и если нужно - слать ее в консоль
	if (messg == WM_MOUSEMOVE)
	{
		// ptCurClient уже преобразован в клиентские (относительно hView) координаты
		if (!OnMouse_Move(pVCon, hWnd, WM_MOUSEMOVE, wParam, lParam, ptCurClient, cr))
			return 0;
	}
	else
	{
		mouse.lastMMW=-1; mouse.lastMML=-1;

		if (messg == WM_LBUTTONDBLCLK)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_LBtnDblClk(pVCon, hWnd, messg, wParam, lParam, ptCurClient, cr))
				return 0;
		} // !!! Без else, т.к. теоретически функция может заменить клик на одинарный

		if (messg == WM_RBUTTONDBLCLK)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_RBtnDblClk(pVCon, hWnd, messg, wParam, lParam, ptCurClient, cr))
				return 0;
		} // !!! Без else, т.к. функция может заменить клик на одинарный

		// Теперь обрабатываем что осталось
		if (messg == WM_LBUTTONDOWN)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_LBtnDown(pVCon, hWnd, WM_LBUTTONDOWN, wParam, lParam, ptCurClient, cr))
				return 0;
		}
		else if (messg == WM_LBUTTONUP)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_LBtnUp(pVCon, hWnd, WM_LBUTTONUP, wParam, lParam, ptCurClient, cr))
				return 0;
		}
		else if (messg == WM_RBUTTONDOWN)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_RBtnDown(pVCon, hWnd, WM_RBUTTONDOWN, wParam, lParam, ptCurClient, cr))
				return 0;
		}
		else if (messg == WM_RBUTTONUP)
		{
			// ptCurClient уже преобразован в клиентские (относительно hView) координаты
			if (!OnMouse_RBtnUp(pVCon, hWnd, WM_RBUTTONUP, wParam, lParam, ptCurClient, cr))
				return 0;
		}
	}

#ifdef MSGLOGGER

	if (messg == WM_MOUSEMOVE)
		messg = WM_MOUSEMOVE;

#endif
fin:
	// Сюда мы попадаем если ни одна из функций (OnMouse_xxx) не запретила пересылку в консоль
	// иначе после (например) RBtnDown в консоль может СРАЗУ провалиться MOUSEMOVE
	// ptCurClient уже преобразован в клиентские (относительно hView) координаты
	mouse.lastMMW=wParam; mouse.lastMML=MAKELPARAM(ptCurClient.x, ptCurClient.y);

	// LClick из PanelViews пропускать в консоль нельзя, чтобы не возникало глюков с позиционированием по клику...
	if (!isPrivate)
	{
		// Теперь осталось послать событие в консоль
		if (pRCon)
			pRCon->OnMouse(messg, wParam, ptCurClient.x, ptCurClient.y);
	}

	return 0;
}

LRESULT CConEmuMain::OnMouse_Move(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	// WM_MOUSEMOVE вроде бы слишком часто вызывается даже при условии что курсор не двигается...
	if (wParam==mouse.lastMMW && lParam==mouse.lastMML
	        && (mouse.state & MOUSE_DRAGPANEL_ALL) == 0)
	{
		DEBUGSTRPANEL2(L"PanelDrag: Skip of wParam==mouse.lastMMW && lParam==mouse.lastMML\n");
		return 0;
	}

	mouse.lastMMW=wParam; mouse.lastMML=lParam;

	mp_Find->UpdateFindDlgAlpha();

	// мог не сброситься, проверим
	isSizing();

	// 18.03.2009 Maks - Если уже тащим - мышь не слать
	if (isDragging())
	{
		// может флажок случайно остался?
		if ((mouse.state & DRAG_L_STARTED) && ((wParam & MK_LBUTTON)==0))
			mouse.state &= ~(DRAG_L_STARTED | DRAG_L_ALLOWED);

		if ((mouse.state & DRAG_R_STARTED) && ((wParam & MK_RBUTTON)==0))
			mouse.state &= ~(DRAG_R_STARTED | DRAG_R_ALLOWED);

		// Случайно остался?
		mouse.state &= ~MOUSE_DRAGPANEL_ALL;

		if (mouse.state & (DRAG_L_STARTED | DRAG_R_STARTED))
		{
			DEBUGSTRPANEL2(L"PanelDrag: Skip of isDragging\n");
			return 0;
		}
	}
	else if ((mouse.state & (DRAG_L_ALLOWED | DRAG_R_ALLOWED)) != 0)
	{
		if (isConSelectMode())
		{
			mouse.state &= ~(DRAG_L_STARTED | DRAG_L_ALLOWED | DRAG_R_STARTED | DRAG_R_ALLOWED | MOUSE_DRAGPANEL_ALL);
		}
	}

	if (mouse.state & MOUSE_DRAGPANEL_ALL)
	{
		if (!gpSet->isDragPanel)
		{
			mouse.state &= ~MOUSE_DRAGPANEL_ALL;
			DEBUGSTRPANEL2(L"PanelDrag: Skip of isDragPanel==0\n");
		}
		else
		{
			if (!isPressed(VK_LBUTTON))
			{
				mouse.state &= ~MOUSE_DRAGPANEL_ALL;
				DEBUGSTRPANEL2(L"PanelDrag: Skip of LButton not pressed\n");
				return 0;
			}

			if (cr.X == mouse.LClkCon.X && cr.Y == mouse.LClkCon.Y)
			{
				DEBUGSTRPANEL2(L"PanelDrag: Skip of cr.X == mouse.LClkCon.X && cr.Y == mouse.LClkCon.Y\n");
				return 0;
			}

			if (gpSet->isDragPanel == 1)
				OnSizePanels(cr);

			// Мышь не слать...
			return 0;
		}
	}

	//TODO: вроде бы иногда isSizing() не сбрасывается?
	//? может так: if (gpSet->isDragEnabled & mouse.state)
	if (gpSet->isDragEnabled & ((mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED))!=0)
	        && !isPictureView())
	{
		if (mp_DragDrop==nullptr)
		{
			DebugStep(_T("DnD: Drag-n-Drop is null"));
		}
		else
		{
			mouse.bIgnoreMouseMove = true;
			BOOL lbDiffTest = PTDIFFTEST(cursor,DRAG_DELTA); // TRUE - если курсор НЕ двигался (далеко)

			if (!lbDiffTest && !isDragging() && !isSizing())
			{
				// 2009-06-19 Выполнялось сразу после "if (gpSet->isDragEnabled & (mouse.state & (DRAG_L_ALLOWED|DRAG_R_ALLOWED)))"
				if (mouse.state & MOUSE_R_LOCKED)
				{
					// чтобы при RightUp не ушел APPS
					mouse.state &= ~MOUSE_R_LOCKED;
					//PtDiffTest(C, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), D)
					char szLog[255];
					wsprintfA(szLog, "Right drag started, MOUSE_R_LOCKED cleared: cursor={%i,%i}, Rcursor={%i,%i}, Now={%i,%i}, MinDelta=%i",
					          (int)cursor.x, (int)cursor.y,
					          (int)Rcursor.x, (int)Rcursor.y,
					          (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam),
					          (int)DRAG_DELTA);
					LogString(szLog);
				}

				BOOL lbLeftDrag = (mouse.state & DRAG_L_ALLOWED) == DRAG_L_ALLOWED;
				LogString(lbLeftDrag ? "Left drag about to start" : "Right drag about to start");
				// Если сначала фокус был на файловой панели, но после LClick он попал на НЕ файловую - отменить ShellDrag
				bool bFilePanel = CVConGroup::isFilePanel();

				if (!bFilePanel)
				{
					DebugStep(_T("DnD: not file panel"));
					//isLBDown = false;
					mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | DRAG_R_ALLOWED | DRAG_R_STARTED);
					mouse.bIgnoreMouseMove = false;
					//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
					pVCon->RCon()->OnMouse(WM_LBUTTONUP, wParam, ptCur.x, ptCur.x);
					//ReleaseCapture(); --2009-03-14
					return 0;
				}

				// Чтобы сам фар не дергался на MouseMove...
				//isLBDown = false;
				mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | DRAG_R_STARTED); // флажок поставит сам CDragDrop::Drag() по наличию DRAG_R_ALLOWED
				// вроде валится, если перед Drag
				//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание

				//TODO("Тут бы не посылать мышь в очередь, а передавать через аргумент команды GetDragInfo в плагин");
				//if (lbLeftDrag) { // сразу "отпустить" клавишу
				//	//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( mouse.LClkCon.X, mouse.LClkCon.Y ), TRUE);     //посылаем консоли отпускание
				//	pVCon->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.LClkDC.X, mouse.LClkDC.Y);
				//} else {
				//	//POSTMESSAGE(ghConWnd, WM_LBUTTONDOWN, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
				//	pVCon->RCon()->OnMouse(WM_LBUTTONDOWN, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
				//	//POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);     //посылаем консоли отпускание
				//	pVCon->RCon()->OnMouse(WM_LBUTTONUP, wParam, mouse.RClkDC.X, mouse.RClkDC.Y);
				//}
				//pVCon->RCon()->FlushInputQueue();

				// Иначе иногда срабатывает FAR'овский D'n'D
				//SENDMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ));     //посылаем консоли отпускание
				if (mp_DragDrop)
				{
					//COORD crMouse = pVCon->ClientToConsole(
					//	lbLeftDrag ? mouse.LClkDC.X : mouse.RClkDC.X,
					//	lbLeftDrag ? mouse.LClkDC.Y : mouse.RClkDC.Y);
					DebugStep(_T("DnD: Drag-n-Drop starting"));
					mp_DragDrop->Drag(!lbLeftDrag, lbLeftDrag ? mouse.LClkDC : mouse.RClkDC);
					DebugStep(Title); // вернуть заголовок
				}
				else
				{
					_ASSERTE(mp_DragDrop); // должно быть обработано выше
					DebugStep(_T("DnD: Drag-n-Drop is null"));
				}

				mouse.bIgnoreMouseMove = false;
				//#ifdef NEWMOUSESTYLE
				//newX = cursor.x; newY = cursor.y;
				//#else
				//newX = MulDiv(cursor.x, conRect.right, std::max<uint>(1, pVCon->Width));
				//newY = MulDiv(cursor.y, conRect.bottom, std::max<uint>(1, pVCon->Height));
				//#endif
				//if (lbLeftDrag)
				//  POSTMESSAGE(ghConWnd, WM_LBUTTONUP, wParam, MAKELPARAM( newX, newY ), TRUE);     //посылаем консоли отпускание
				//isDragProcessed=false; -- убрал, иначе при бросании в пассивную панель больших файлов дроп может вызваться еще раз???
				return 0;
			}
		}
	}
	else if (!gpSet->isDisableMouse && gpSet->isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
	{
		//Если двинули мышкой, а была включена опция RClick - не вызывать
		//контекстное меню - просто послать правый клик
		if (!PTDIFFTEST(Rcursor, RCLICKAPPSDELTA))
		{
			//isRBDown=false;
			mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);

			if (mb_RightClickingPaint)
			{
				// Если начали рисовать Waiting вокруг курсора
				StopRightClickingPaint();
				_ASSERTE(mb_RightClickingPaint==FALSE);
			}

			char szLog[255];
			wsprintfA(szLog, "Mouse was moved, MOUSE_R_LOCKED cleared: Rcursor={%i,%i}, Now={%i,%i}, MinDelta=%i",
			          (int)Rcursor.x, (int)Rcursor.y,
			          (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam),
			          (int)RCLICKAPPSDELTA);
			LogString(szLog);
			//POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, 0, MAKELPARAM( mouse.RClkCon.X, mouse.RClkCon.Y ), TRUE);
			pVCon->RCon()->OnMouse(WM_RBUTTONDOWN, 0, mouse.RClkDC.X, mouse.RClkDC.Y);
		}

		return 0;
	}

	/*if (!isRBDown && (wParam==MK_RBUTTON)) {
	// Чтобы при выделении правой кнопкой файлы не пропускались
	if ((newY-RBDownNewY)>5) {// пока попробуем для режима сверху вниз
	for (short y=RBDownNewY;y<newY;y+=5)
	POSTMESSAGE(ghConWnd, WM_MOUSEMOVE, wParam, MAKELPARAM( RBDownNewX, y ), TRUE);
	}
	RBDownNewX=newX; RBDownNewY=newY;
	}*/

	if (mouse.bIgnoreMouseMove)
		return 0;

	return TRUE; // переслать в консоль
}

LRESULT CConEmuMain::OnMouse_LBtnDown(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	DragPanelBorder dpb = DragPanelBorder::None;
	//if (isLBDown()) ReleaseCapture(); // Вдруг сглючило? --2009-03-14
	//isLBDown = false;
	mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | MOUSE_DRAGPANEL_ALL);
	mouse.bIgnoreMouseMove = false;
	mouse.LClkCon = cr;
	mouse.LClkDC = MakeCoord(ptCur.x,ptCur.y);
	CRealConsole *pRCon = pVCon ? pVCon->RCon() : nullptr;

	if (!pRCon)  // если консоли нет - то и слать некуда
		return 0;

	dpb = CConEmuMain::CheckPanelDrag(cr);

	if (dpb != DragPanelBorder::None)
	{
		switch (dpb)
		{
		case DragPanelBorder::Split:
			mouse.state |= MOUSE_DRAGPANEL_SPLIT; break;
		case DragPanelBorder::Left:
			mouse.state |= MOUSE_DRAGPANEL_LEFT; break;
		case DragPanelBorder::Right:
			mouse.state |= MOUSE_DRAGPANEL_RIGHT; break;
		case DragPanelBorder::Both:
			mouse.state |= MOUSE_DRAGPANEL_BOTH; break;
		}

		// Change panel height separately
		if (dpb == DragPanelBorder::Left || dpb == DragPanelBorder::Right)
		{
			if (dpb == DragPanelBorder::Left)
			{
				if (pRCon->IsFarLua())
					pRCon->PostMacro(L"if not APanel.Left then Keys(\"Tab\") end");
				else
					pRCon->PostMacro(L"@$If (!APanel.Left) Tab $End");
			}
			else if (dpb == DragPanelBorder::Right)
			{
				if (pRCon->IsFarLua())
					pRCon->PostMacro(L"if APanel.Left then Keys(\"Tab\") end");
				else
					PostMacro(L"@$If (APanel.Left) Tab $End");
			}
		}

		// LBtnDown в консоль не посылаем, но попробуем послать MouseMove?
		// (иначе начинается драка с PanelTabs - он перехватывает и буферизирует всю клавиатуру)
		INPUT_RECORD r = {MOUSE_EVENT};
		r.Event.MouseEvent.dwMousePosition = mouse.LClkCon;
		r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
		if (pVCon)
			pVCon->RCon()->PostConsoleEvent(&r);
		return 0;
	}

	if (!isConSelectMode() && CVConGroup::isFilePanel() && pVCon &&
	        pVCon->RCon()->CoordInPanel(mouse.LClkCon))
	{
		//SetCapture('ghWnd DC'); --2009-03-14
		cursor.x = (int)(short)LOWORD(lParam);
		cursor.y = (int)(short)HIWORD(lParam);
		//isLBDown=true;
		//isDragProcessed=false;
		CONSOLE_CURSOR_INFO ci;
		pVCon->RCon()->GetConsoleCursorInfo(&ci);

		if ((ci.bVisible && ci.dwSize>0)  // курсор должен быть видим, иначе это чье-то меню
		        && gpSet->isDragEnabled & DRAG_L_ALLOWED)
		{
			//if (!gpSet->nLDragKey || isPressed(gpSet->nLDragKey))
			// функция проверит нажат ли nLDragKey (допускает nLDragKey==0), а другие - не нажаты
			// то есть нажат SHIFT(==nLDragKey), а CTRL & ALT - НЕ нажаты
			if (gpSet->IsModifierPressed(vkLDragKey, true))
			{
				mouse.state = DRAG_L_ALLOWED;
			}
		}

		// иначе после LBtnDown в консоль может СРАЗУ провалиться MOUSEMOVE
		mouse.lastMMW=wParam; mouse.lastMML=lParam;
		//if (gpSet->is DnD) mouse.bIgnoreMouseMove = true;
		//POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE); // было SEND
		pVCon->RCon()->OnMouse(messg, wParam, ptCur.x, ptCur.y);
		return 0;
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_LBtnUp(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	BOOL lbLDrag = (mouse.state & DRAG_L_STARTED) == DRAG_L_STARTED;

	if (mouse.state & MOUSE_DRAGPANEL_ALL)
	{
		if (gpSet->isDragPanel == 2)
		{
			OnSizePanels(cr);
		}

		mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | MOUSE_DRAGPANEL_ALL);
		// Мышь не слать...
		return 0;
	}

	mouse.state &= ~(DRAG_L_ALLOWED | DRAG_L_STARTED | MOUSE_DRAGPANEL_ALL);

	if (lbLDrag)
		return 0; // кнопка уже "отпущена"

	if (mouse.bIgnoreMouseMove)
	{
		mouse.bIgnoreMouseMove = false;
		//#ifdef NEWMOUSESTYLE
		//newX = cursor.x; newY = cursor.y;
		//#else
		//newX = MulDiv(cursor.x, conRect.right, std::max<uint>(1, pVCon->Width));
		//newY = MulDiv(cursor.y, conRect.bottom, std::max<uint>(1, pVCon->Height));
		//#endif
		//POSTMESSAGE(ghConWnd, messg, wParam, MAKELPARAM( newX, newY ), FALSE);
		pVCon->RCon()->OnMouse(messg, wParam, cursor.x, cursor.y);
		return 0;
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_LBtnDblClk(CVirtualConsole* pVCon, HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	CRealConsole *pRCon = pVCon->RCon();

	if (!pRCon)  // Если консоли нет - то слать некуда
		return 0;

	enum DragPanelBorder dpb = CConEmuMain::CheckPanelDrag(cr);

	if (dpb != DragPanelBorder::None)
	{
		wchar_t szMacro[128]; szMacro[0] = 0;

		bool lbIsLua = pRCon->IsFarLua();

		if (dpb == DragPanelBorder::Split)
		{
			RECT rcRight; pRCon->GetPanelRect(TRUE, &rcRight, TRUE);
			int  nCenter = pRCon->TextWidth() / 2;

			if (lbIsLua)
			{
				if (nCenter < rcRight.left)
					swprintf_c(szMacro, L"for RCounter= %i,1,-1 do Keys(\"CtrlLeft\") end", rcRight.left - nCenter);
				else if (nCenter > rcRight.left)
					swprintf_c(szMacro, L"for RCounter= %i,1,-1 do Keys(\"CtrlRight\") end", nCenter - rcRight.left);
			}
			else
			{
				if (nCenter < rcRight.left)
					swprintf_c(szMacro, L"@$Rep (%i) CtrlLeft $End", rcRight.left - nCenter);
				else if (nCenter > rcRight.left)
					swprintf_c(szMacro, L"@$Rep (%i) CtrlRight $End", nCenter - rcRight.left);
			}
		}
		else
		{
			wchar_t szKey[32];
			wcscpy_c(szKey, (dpb == DragPanelBorder::Both) ? L"CtrlDown" : L"CtrlShiftDown");

			if (lbIsLua)
			{
				swprintf_c(szMacro, L"for RCounter= %i,1,-1 do Keys(\"%s\") end", pRCon->TextHeight(), szKey);
			}
			else
			{
				swprintf_c(szMacro, L"@$Rep (%i) %s $End", pRCon->TextHeight(), szKey);
			}
		}

		if (szMacro[0])
			pRCon->PostMacro(szMacro);

		return 0;
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_RBtnDown(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	Rcursor = ptCur;
	mouse.RClkCon = cr;
	mouse.RClkDC = MakeCoord(ptCur.x,ptCur.y);
	//isRBDown=false;
	mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);
	mouse.bIgnoreMouseMove = false;

	if (gpSet->isLogging())
	{
		char szLog[100];
		wsprintfA(szLog, "Right button down: Rcursor={%i,%i}", (int)Rcursor.x, (int)Rcursor.y);
		LogString(szLog);
	}

	//if (isFilePanel()) // Maximus5
	bool bSelect = false, bPanel = false, bActive = false, bCoord = false;

	if (!(bSelect = isConSelectMode())
	        && (bPanel = CVConGroup::isFilePanel())
	        && (bActive = (pVCon != nullptr))
	        && (bCoord = (pVCon->RCon()->CoordInPanel(mouse.RClkCon) > 0)))
	{
		if (gpSet->isDragEnabled & DRAG_R_ALLOWED)
		{
			//if (!gpSet->nRDragKey || isPressed(gpSet->nRDragKey)) {
			// функция проверит нажат ли nRDragKey (допускает nRDragKey==0), а другие - не нажаты
			// то есть нажат SHIFT(==nRDragKey), а CTRL & ALT - НЕ нажаты
			if (gpSet->IsModifierPressed(vkRDragKey, true))
			{
				mouse.state = DRAG_R_ALLOWED;

				if (gpSet->isLogging()) LogString("RightClick ignored of gpSet->nRDragKey pressed");

				return 0;
			}
		}

		// Если ничего лишнего не нажато!
		if (!gpSet->isDisableMouse && gpSet->isRClickSendKey && !(wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)))
		{
			//заведем таймер на .3
			//если больше - пошлем apps
			mouse.state |= MOUSE_R_LOCKED; mouse.bSkipRDblClk = false;
			char szLog[100];
			wsprintfA(szLog, "MOUSE_R_LOCKED was set: Rcursor={%i,%i}", (int)Rcursor.x, (int)Rcursor.y);
			LogString(szLog);
			mouse.RClkTick = TimeGetTime(); //GetTickCount();
			StartRightClickingPaint();
			return 0;
		}
		else
		{
			if (gpSet->isLogging())
				LogString(
					gpSet->isDisableMouse ? "RightClick ignored of gpSet->isDisableMouse" :
					!gpSet->isRClickSendKey ? "RightClick ignored of !gpSet->isRClickSendKey" :
					"RightClick ignored of wParam&(MK_CONTROL|MK_LBUTTON|MK_MBUTTON|MK_SHIFT|MK_XBUTTON1|MK_XBUTTON2)"
				);
		}
	}
	else
	{
		if (gpSet->isLogging())
			LogString(
				bSelect ? "RightClick ignored of isConSelectMode" :
				!bPanel ? "RightClick ignored of NOT isFilePanel" :
				!bActive ? "RightClick ignored of NOT isFilePanel" :
				!bCoord ? "RightClick ignored of NOT isFilePanel" :
				"RightClick ignored, unknown cause"
			);
	}

	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_RBtnUp(CVirtualConsole* pVCon, HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	// Сразу сбросить, если выставлялся
	StopRightClickingPaint();

	if (!gpSet->isDisableMouse && gpSet->isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
	{
		//isRBDown=false; // сразу сбросим!
		mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);

		if (PTDIFFTEST(Rcursor,RCLICKAPPSDELTA))
		{
			//держали зажатой <.3
			//убьем таймер, кликнем правой кнопкой
			//KillTimer(hWnd, 1); -- Maximus5, таймер более не используется
			const DWORD dwCurTick = TimeGetTime(); //GetTickCount();
			const DWORD dwDelta = dwCurTick - mouse.RClkTick;

			// Если держали дольше .3с, но не слишком долго :)
			if ((gpSet->isRClickSendKey==1)
				|| (dwDelta>RCLICKAPPSTIMEOUT/*.3сек*/ && dwDelta<RCLICKAPPSTIMEOUT_MAX/*10000*/)
				// Или в режиме тачскрина - длинный _тап_
				|| (gpSet->isRClickTouchInvert() && (dwDelta<RCLICKAPPSTIMEOUT))
				)
			{
				if (!mb_RightClickingLSent && pVCon)
				{
					mb_RightClickingLSent = TRUE;
					// Чтобы установить курсор в панелях точно под кликом
					// иначе получается некрасиво, что курсор прыгает только перед
					// появлением EMenu, а до этого (пока крутится "кружок") курсор не двигается.
					pVCon->RCon()->PostLeftClickSync(mouse.RClkDC);
					//apVCon->RCon()->OnMouse(WM_MOUSEMOVE, 0, mouse.RClkDC.X, mouse.RClkDC.Y, true);
					//WARNING("По хорошему, нужно дождаться пока мышь обработается");
					//apVCon->RCon()->PostMacro(L"MsLClick");
					//WARNING("!!! Заменить на CMD_LEFTCLKSYNC?");
				}

				// А теперь можно и Apps нажать
				mouse.bSkipRDblClk=true; // чтобы пока FAR думает в консоль не проскочило мышиное сообщение
				//POSTMESSAGE(ghConWnd, WM_KEYDOWN, VK_APPS, 0, TRUE);
				const DWORD dwFarPID = pVCon->RCon()->GetFarPID();

				if (dwFarPID)
				{
					AllowSetForegroundWindow(dwFarPID);
					COORD crMouse = pVCon->RCon()->ScreenToBuffer(
										pVCon->ClientToConsole(mouse.RClkDC.X, mouse.RClkDC.Y)
									);
					CConEmuPipe pipe(GetFarPID(), CONEMUREADYTIMEOUT);

					if (pipe.Init(_T("CConEmuMain::EMenu"), TRUE))
					{
						//DWORD cbWritten=0;
						DebugStep(_T("EMenu: Waiting for result (10 sec)"));
						size_t nLen = 0;
						unsigned nSize = sizeof(crMouse) + sizeof(wchar_t);

						if (gpSet->sRClickMacro && gpSet->sRClickMacro[0])
						{
							nLen = wcslen(gpSet->sRClickMacro);
							if (nLen > 0x8000)
							{
								const std::wstring errorInfo = L"EMenu: sRClickMacro is too long (" + std::to_wstring(nLen) + L" chars)";
								LogString(errorInfo.c_str());
							}
							nSize += nLen * sizeof(gpSet->sRClickMacro[0]);
						}

						const std::unique_ptr<BYTE[], std::default_delete<BYTE[]>> pcbData(new BYTE[nSize]);
						_ASSERTE(pcbData.get() != nullptr);
						memmove_s(pcbData.get(), nSize, &crMouse, sizeof(crMouse));

						wchar_t* ptrSendMacro = reinterpret_cast<wchar_t*>(pcbData.get() + sizeof(crMouse));
						if (nLen > 0)
						{
							wcscpy_s(ptrSendMacro, nLen + 1, gpSet->sRClickMacro);
						}
						else
						{
							*ptrSendMacro = 0;
						}

						if (!pipe.Execute(CMD_EMENU, pcbData.get(), nSize))
						{
							LogString("RightClicked, but pipe.Execute(CMD_EMENU) failed");
						}
						else
						{
							// OK
							if (gpSet->isLogging())
							{
								char szInfo[255] = {0};
								lstrcpyA(szInfo, "RightClicked, pipe.Execute(CMD_EMENU) OK");

								if (gpSet->sRClickMacro && *gpSet->sRClickMacro)
								{
									lstrcatA(szInfo, ", Macro: "); nLen = lstrlenA(szInfo);
									WideCharToMultiByte(CP_ACP,0, gpSet->sRClickMacro,-1, szInfo+nLen, countof(szInfo)-nLen-1, 0,0);
								}
								else
								{
									lstrcatA(szInfo, ", NoMacro");
								}

								LogString(szInfo);
							}
						}

						DebugStep(nullptr);
					}
					else
					{
						LogString("RightClicked, but pipe.Init() failed");
					}

					//INPUT_RECORD r = {KEY_EVENT};
					////pVCon->RCon()->On Keyboard(ghConWnd, WM_KEYDOWN, VK_APPS, (VK_APPS << 16) | (1 << 24));
					////pVCon->RCon()->On Keyboard(ghConWnd, WM_KEYUP, VK_APPS, (VK_APPS << 16) | (1 << 24));
					//r.Event.KeyEvent.bKeyDown = TRUE;
					//r.Event.KeyEvent.wVirtualKeyCode = VK_APPS;
					//r.Event.KeyEvent.wVirtualScanCode = /*28 на моей клавиатуре*/MapVirtualKey(VK_APPS, 0/*MAPVK_VK_TO_VSC*/);
					//r.Event.KeyEvent.dwControlKeyState = 0x120;
					//pVCon->RCon()->PostConsoleEvent(&r);
					////On Keyboard(hConWnd, WM_KEYUP, VK_RETURN, 0);
					//r.Event.KeyEvent.bKeyDown = FALSE;
					//r.Event.KeyEvent.dwControlKeyState = 0x120;
					//pVCon->RCon()->PostConsoleEvent(&r);
				}
				else
				{
					LogString("RightClicked, but FAR PID is 0");
				}

				return 0;
			}
			else
			{
				char szLog[255];
				// if ((gpSet->isRClickSendKey==1) || (dwDelta>RCLICKAPPSTIMEOUT && dwDelta<10000))
				lstrcpyA(szLog, "RightClicked, but condition failed: ");

				if (gpSet->isRClickSendKey!=1)
				{
					wsprintfA(szLog+lstrlenA(szLog), "((isRClickSendKey=%i)!=1)", (UINT)gpSet->isRClickSendKey);
				}
				else
				{
					wsprintfA(szLog+lstrlenA(szLog), "(isRClickSendKey==%i)", (UINT)gpSet->isRClickSendKey);
				}

				if (dwDelta <= RCLICKAPPSTIMEOUT)
				{
					wsprintfA(szLog+lstrlenA(szLog), ", ((Delay=%i)<=%i)", dwDelta, (int)RCLICKAPPSTIMEOUT);
				}
				else if (dwDelta >= 10000)
				{
					wsprintfA(szLog+lstrlenA(szLog), ", ((Delay=%i)>=10000)", dwDelta);
				}
				else
				{
					wsprintfA(szLog+lstrlenA(szLog), ", (Delay==%i)", dwDelta);
				}

				LogString(szLog);
			}
		}
		else
		{
			char szLog[100];
			wsprintfA(szLog, "RightClicked, but mouse was moved abs({%i,%i}-{%i,%i})>%i", Rcursor.x, Rcursor.y, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam), (int)RCLICKAPPSDELTA);
			LogString(szLog);
		}

		// иначе после RBtnDown в консоль может СРАЗУ провалиться MOUSEMOVE
		mouse.lastMMW=MK_RBUTTON|wParam; mouse.lastMML=lParam; // добавляем, т.к. мы в RButtonUp
		// Иначе нужно сначала послать WM_RBUTTONDOWN
		//POSTMESSAGE(ghConWnd, WM_RBUTTONDOWN, wParam, MAKELPARAM( newX, newY ), TRUE);
		pVCon->RCon()->OnMouse(WM_RBUTTONDOWN, wParam, ptCur.x, ptCur.y);
	}
	else
	{
		char szLog[255];
		// if (gpSet->isRClickSendKey && (mouse.state & MOUSE_R_LOCKED))
		//wsprintfA(szLog, "RightClicked, but condition failed (RCSK:%i, State:%u)", (int)gpSet->isRClickSendKey, (DWORD)mouse.state);
		lstrcpyA(szLog, "RightClicked, but condition failed: ");

		if (gpSet->isDisableMouse)
		{
			wsprintfA(szLog+lstrlenA(szLog), "((isDisableMouse=%i)==0)", (UINT)gpSet->isDisableMouse);
		}
		if (gpSet->isRClickSendKey==0)
		{
			wsprintfA(szLog+lstrlenA(szLog), "((isRClickSendKey=%i)==0)", (UINT)gpSet->isRClickSendKey);
		}
		else
		{
			wsprintfA(szLog+lstrlenA(szLog), "(isRClickSendKey==%i)", (UINT)gpSet->isRClickSendKey);
		}

		if ((mouse.state & MOUSE_R_LOCKED) == 0)
		{
			wsprintfA(szLog+lstrlenA(szLog), ", (((state=0x%X)&MOUSE_R_LOCKED)==0)", (DWORD)mouse.state);
		}
		else
		{
			wsprintfA(szLog+lstrlenA(szLog), ", (state==0x%X)", (DWORD)mouse.state);
		}

		LogString(szLog);
	}

	//isRBDown=false; // чтобы не осталось случайно
	mouse.state &= ~(DRAG_R_ALLOWED | DRAG_R_STARTED | MOUSE_R_LOCKED);
	return TRUE; // переслать в консоль
}
LRESULT CConEmuMain::OnMouse_RBtnDblClk(CVirtualConsole* pVCon, HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam, POINT ptCur, COORD cr)
{
	if (mouse.bSkipRDblClk)
	{
		mouse.bSkipRDblClk = false;
		return 0; // не обрабатывать, сейчас висит контекстное меню
	}

	//if (gpSet->isRClickSendKey) {
	//	// Заменить на одинарный клик, иначе может не подняться контекстное меню
	//	messg = WM_RBUTTONDOWN;
	//} -- хотя, раньше это всегда и делалось в любом случае...
	messg = WM_RBUTTONDOWN;
	return TRUE; // переслать в консоль
}

BOOL CConEmuMain::OnMouse_NCBtnDblClk(HWND hWnd, UINT& messg, WPARAM wParam, LPARAM lParam)
{
	// По DblClick на рамке - развернуть окно по горизонтали (вертикали) на весь монитор
	if (wParam == HTLEFT || wParam == HTRIGHT || wParam == HTTOP || wParam == HTBOTTOM)
	{
		if (WindowMode != wmNormal)
		{
			// Так быть не должно - рамка не должна быть видна в этих режимах
			// Для Quake - игнорировать
			_ASSERTE((WindowMode == wmNormal) || gpSet->isQuakeStyle);
			return FALSE;
		}

		ChangeTileMode((wParam == HTLEFT || wParam == HTRIGHT) ? cwc_TileWidth : cwc_TileHeight);

		return TRUE;
	}

	return FALSE;
}

void CConEmuMain::SetDragCursor(HCURSOR hCur)
{
	mh_DragCursor = hCur;

	if (mh_DragCursor)
	{
		SetCursor(mh_DragCursor);
	}
	else
	{
		OnSetCursor();
	}
}

void CConEmuMain::SetWaitCursor(BOOL abWait)
{
	mb_WaitCursor = abWait;

	if (mb_WaitCursor)
		SetCursor(mh_CursorWait);
	else
		SetCursor(mh_CursorArrow);
}

void CConEmuMain::CheckFocus(LPCWSTR asFrom)
{
	HWND hCurForeground = GetForegroundWindow();
	static HWND hPrevForeground;

	if (hPrevForeground == hCurForeground)
		return;

	hPrevForeground = hCurForeground;
	DWORD dwPID = 0, dwTID = 0;

	#ifdef _DEBUG
	wchar_t szDbg[255], szClass[128];

	if (!hCurForeground || !GetClassName(hCurForeground, szClass, 127)) lstrcpy(szClass, L"<nullptr>");
	#endif

	BOOL lbConEmuActive = (hCurForeground == ghWnd || (ghOpWnd && hCurForeground == ghOpWnd));
	BOOL lbLDown = isPressed(VK_LBUTTON);

	if (!lbConEmuActive)
	{
		if (!hCurForeground)
		{
			#ifdef _DEBUG
			swprintf_c(szDbg, L"Foreground changed (%s). NewFore=0x00000000, LBtn=%i\n", asFrom, lbLDown);
			#endif
		}
		else
		{
			dwTID = GetWindowThreadProcessId(hCurForeground, &dwPID);
			// Получить информацию об активном треде
			GUITHREADINFO gti = {sizeof(GUITHREADINFO)};
			GetGUIThreadInfo(0/*dwTID*/, &gti);

			#ifdef _DEBUG
			swprintf_c(szDbg, L"Foreground changed (%s). NewFore=0x%08X, Active=0x%08X, Focus=0x%08X, Class=%s, LBtn=%i\n", asFrom, LODWORD(hCurForeground), LODWORD(gti.hwndActive), LODWORD(gti.hwndFocus), szClass, lbLDown);
			#endif

			// mh_ShellWindow чисто для информации. Хоть родитель ConEmu и меняется на mh_ShellWindow
			// но проводник может перекинуть наше окно в другое (WorkerW или Progman)
			if (opt.DesktopMode && mh_ShellWindow)
			{
				//HWND hShell = GetShellWindow(); // Progman
				bool lbDesktopActive = false;

				if (dwPID == mn_ShellWindowPID)  // дальше только для процесса Explorer.exe (который держит Desktop)
				{
					// При WinD активируется (Foreground) не Progman, а WorkerW
					// Тем не менее, фокус - передается в дочернее окно Progman
					if (hCurForeground == mh_ShellWindow)
					{
						lbDesktopActive = true;
					}
					else
					{
						wchar_t szClass[128];

						if (!GetClassName(hCurForeground, szClass, 127)) szClass[0] = 0;

						if (!wcscmp(szClass, L"WorkerW") || !wcscmp(szClass, L"Progman"))
							lbDesktopActive = true;

						//HWND hDesktop = GetDesktopWindow();
						//HWND hParent = GetParent(gti.hwndFocus);
						////	GetWindow(gti.hwndFocus, GW_OWNER);
						//while (hParent) {
						//	if (hParent == mh_ShellWindow) {
						//		lbDesktopActive = true;
						//		break;
						//	}
						//	hParent = GetParent(hParent);
						//	if (hParent == hDesktop) break;
						//}
					}
				}

				if (lbDesktopActive)
				{
					CVConGuard VCon;

					if (lbLDown)
					{
						// Кликнули мышкой для активации рабочего стола с иконками
						mb_FocusOnDesktop = FALSE; // запомним, что ConEmu активировать не нужно
					}
					else if (mb_FocusOnDesktop && (GetActiveVCon(&VCon) >= 0))
					{
						// Чтобы пользователю не приходилось вручную активировать ConEmu после WinD / WinM
						//apiSetForegroundWindow(ghWnd); // это скорее всего не сработает, т.к. фокус сейчас у другого процесса!
						// так что "активируем" мышкой
						COORD crOpaque = VCon->FindOpaqueCell();

						if (crOpaque.X<0 || crOpaque.Y<0)
						{
							DEBUGSTRFOREGROUND(L"Can't activate ConEmu on desktop. No opaque cell was found in VCon\n");
						}
						else
						{
							POINT pt = VCon->ConsoleToClient(crOpaque.X,crOpaque.Y);
							MapWindowPoints(VCon->GetView(), nullptr, &pt, 1);
							HWND hAtPoint = WindowFromPoint(pt);

							if (hAtPoint != ghWnd)
							{
								#ifdef _DEBUG
								wchar_t szDbgInfo[255], szDbgClass[64];

								if (!hAtPoint || !GetClassName(hAtPoint, szDbgClass, 63)) szDbgClass[0] = 0;

								swprintf_c(szDbgInfo, L"Can't activate ConEmu on desktop. Opaque cell={%i,%i} screen={%i,%i}. WindowFromPoint=0x%08X (%s)\n",
								          crOpaque.X, crOpaque.Y, pt.x, pt.y, LODWORD(hAtPoint), szDbgClass);
								DEBUGSTRFOREGROUND(szDbgInfo);
								#endif
							}
							else
							{
								DEBUGSTRFOREGROUND(L"Activating ConEmu on desktop by mouse click\n");
								mouse.bForceSkipActivation = true;  // don't pass this click into the console!
								// Remember, where was the cursor. Revert the position later
								POINT ptCur; GetCursorPos(&ptCur);
								SetCursorPos(pt.x,pt.y);  // Need to "move" the mouse, otherwise mouse_event will not work
								// "clicking"
								mouse_event(MOUSEEVENTF_ABSOLUTE+MOUSEEVENTF_LEFTDOWN, pt.x,pt.y, 0,0);
								mouse_event(MOUSEEVENTF_ABSOLUTE+MOUSEEVENTF_LEFTUP, pt.x,pt.y, 0,0);
								// Revert the cursor position
								SetCursorPos(ptCur.x,ptCur.y);
							}
						}
					}
				}
			}
		}
	}
	else
	{
		#ifdef _DEBUG
		swprintf_c(szDbg, L"Foreground changed (%s). NewFore=0x%08X, ConEmu has focus, LBtn=%i\n", asFrom, LODWORD(hCurForeground), lbLDown);
		#endif
		mb_FocusOnDesktop = TRUE;
	}

	UNREFERENCED_PARAMETER(dwTID);
	DEBUGSTRFOREGROUND(szDbg);
}

bool CConEmuMain::CheckUpdates(UpdateCallMode callMode)
{
	if (!isUpdateAllowed())
		return false;

	_ASSERTE(isMainThread());

	if (!gpUpd)
		gpUpd = new CConEmuUpdate;

	gpUpd->StartCheckProcedure(callMode);

	return true;
}

void CConEmuMain::RequestPostUpdateTabs()
{
	LONG l = InterlockedIncrement(&mn_ReqMsgUpdateTabs);
	if (l == 1)
	{
		PostMessage(ghWnd, mn_MsgUpdateTabs, 0, 0);
	}
}

DWORD CConEmuMain::isSelectionModifierPressed(bool bAllowEmpty)
{
	DWORD nResult;

	if (m_Pressed.bChecked)
	{
		nResult = bAllowEmpty ? m_Pressed.nReadyToSel : m_Pressed.nReadyToSelNoEmpty;
	}
	else
	{
		DWORD nReadyToSel = 0, nReadyToSelNoEmpty = 0;
		bool bNoEmptyPressed = false, bEmptyAllowed = false;

		if (gpSet->isCTSSelectBlock)
		{
			gpSet->IsModifierPressed(vkCTSVkBlock, &bNoEmptyPressed, &bEmptyAllowed);
			if (bNoEmptyPressed)
				nReadyToSelNoEmpty = nReadyToSel = CONSOLE_BLOCK_SELECTION;
			else if (bEmptyAllowed)
				nReadyToSel = CONSOLE_BLOCK_SELECTION;
		}

		if (!nReadyToSelNoEmpty && gpSet->isCTSSelectText)
		{
			gpSet->IsModifierPressed(vkCTSVkText, &bNoEmptyPressed, &bEmptyAllowed);
			if (bNoEmptyPressed)
				nReadyToSelNoEmpty = nReadyToSel = CONSOLE_TEXT_SELECTION;
			else if (bEmptyAllowed && !nReadyToSel)
				nReadyToSel = CONSOLE_TEXT_SELECTION;
			_ASSERTE(nReadyToSel || (nReadyToSelNoEmpty == nReadyToSel));
		}

		// Store it
		m_Pressed.nReadyToSel = nReadyToSel;
		m_Pressed.nReadyToSelNoEmpty = nReadyToSelNoEmpty;
		m_Pressed.bChecked = TRUE;

		nResult = bAllowEmpty ? nReadyToSel : nReadyToSelNoEmpty;
	}

	return nResult;
}

void CConEmuMain::ForceSelectionModifierPressed(DWORD nValue)
{
	m_Pressed.nReadyToSel = nValue;
	m_Pressed.bChecked = TRUE;
}

enum DragPanelBorder CConEmuMain::CheckPanelDrag(COORD crCon)
{
	if (!gpSet->isDragPanel || isPictureView())
		return DragPanelBorder::None;

	CVConGuard VCon;
	CRealConsole* pRCon = (GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : nullptr;

	if (!pRCon)
		return DragPanelBorder::None;

	// Должен быть "Фар", "Панели", "Активен" (нет смысла пытаться дергать панели, если фар "висит")
	if (!pRCon->isFar() || !pRCon->isFilePanel(true) || !pRCon->isAlive())
		return DragPanelBorder::None;

	// Если активен наш или ФАРовский граббер
	if (pRCon->isConSelectMode())
		return DragPanelBorder::None;

	// Если удерживается модификатор запуска выделения текста
	if (isSelectionModifierPressed(false))
		return DragPanelBorder::None;

	//CONSOLE_CURSOR_INFO ci;
	//mp_ VActive->RCon()->GetConsoleCursorInfo(&ci);
	//   if (!ci.bVisible || ci.dwSize>40) // Курсор должен быть видим, и не в режиме граба
	//   	return DragPanelBorder::None;
	// Теперь - можно проверить
	DragPanelBorder dpb = DragPanelBorder::None;

	//TODO("Сделаем все-таки драг влево-вправо хватанием за «промежуток» между рамками");
	//int nSplitWidth = gpSetCls->BorderFontWidth()/5;
	//if (nSplitWidth < 1) nSplitWidth = 1;

	RECT rcRight = {}, rcLeft = {};
	bool hasRight = VCon->RCon()->GetPanelRect(TRUE, &rcRight, TRUE);
	bool hasLeft = VCon->RCon()->GetPanelRect(FALSE, &rcLeft, TRUE);
	const double both_part = 0.15;

	auto over_edge = [](const COORD& crCon, const RECT& rcPanel)
	{
		return (crCon.Y == rcPanel.bottom && (rcPanel.left <= crCon.X && crCon.X <= rcPanel.right));
	};

	if ((dpb == DragPanelBorder::None)
		&& hasRight && hasLeft && rcRight != rcLeft)
	{
		if ((crCon.X >= (rcLeft.right - both_part * RectWidth(rcLeft)))
			&& (crCon.X <= (rcRight.left + both_part * RectWidth(rcRight)))
			&& (over_edge(crCon, rcLeft) || over_edge(crCon, rcRight)))
			dpb = DragPanelBorder::Both;
	}


	if ((dpb == DragPanelBorder::None)
		&& hasRight)
	{
		if (crCon.X == rcRight.left && (rcRight.top <= crCon.Y && crCon.Y <= rcRight.bottom))
			dpb = DragPanelBorder::Split;
		else if (over_edge(crCon, rcRight))
			dpb = DragPanelBorder::Right;
	}

	if ((dpb == DragPanelBorder::None)
		&& hasLeft)
	{
		if (gpSet->isDragPanelBothEdges && crCon.X == rcLeft.right && (rcLeft.top <= crCon.Y && crCon.Y <= rcLeft.bottom))
			dpb = DragPanelBorder::Split;
		if (over_edge(crCon, rcLeft))
			dpb = DragPanelBorder::Left;
	}

	return dpb;
}

LRESULT CConEmuMain::OnSetCursor(WPARAM wParam/*=-1*/, LPARAM lParam/*=-1*/)
{
	DEBUGSTRSETCURSOR_(lParam==-1 ? L"WM_SETCURSOR (int)" : L"WM_SETCURSOR");
#ifdef _DEBUG
	if (isPressed(VK_LBUTTON))
	{
		int nDbg = 0;
	}
#endif

	if (lParam != -1)
	{
		// Process only for client areas
		WORD ht = LOWORD(lParam);
		if (ht != HTCLIENT)
		{
			BOOL rc = FALSE;
			LPCWSTR pszCursor = nullptr;
			if (isMenuActive())
			{
				// Don't change cursor in some cases
				pszCursor = nullptr;
			}
			else if (isSelfFrame())
			{
				switch (ht)
				{
				case HTTOP: case HTBOTTOM:
					DEBUGSTRSETCURSOR(L" ---> IDC_SIZENS\n");
					pszCursor = IDC_SIZENS; break;
				case HTLEFT: case HTRIGHT:
					DEBUGSTRSETCURSOR(L" ---> IDC_SIZEWE\n");
					pszCursor = IDC_SIZEWE; break;
				case HTTOPLEFT: case HTBOTTOMRIGHT:
					DEBUGSTRSETCURSOR(L" ---> IDC_SIZENWSE\n");
					pszCursor = IDC_SIZENWSE; break;
				case HTTOPRIGHT: case HTBOTTOMLEFT:
					DEBUGSTRSETCURSOR(L" ---> IDC_SIZENESW\n");
					pszCursor = IDC_SIZENESW; break;
				}
			}
			else if (ht == HTCAPTION && gpSet->isHideCaptionAlways()
				&& (!isTabsShown()
					|| (IsWin10() && gpSet->nTabsLocation))
				)
			{
				DEBUGSTRSETCURSOR(L" ---> IDC_SIZEALL\n");
				pszCursor = IDC_SIZEALL;
			}

			if (pszCursor)
			{
				HCURSOR sizeCursor = LoadCursor(nullptr, pszCursor);
				if (sizeCursor)
				{
					SetCursor(sizeCursor);
					rc = TRUE;
				}
			}
			return rc;
		}
	}

	RConStartArgsEx::SplitType split = CVConGroup::isSplitterDragging();
	if (split)
	{
		DEBUGSTRSETCURSOR(L" ---> Processed by Splitter\n");
		SetCursor((split == RConStartArgsEx::eSplitVert) ? mh_SplitV : mh_SplitH);
		return TRUE;
	}

	CVConGuard VCon;
	POINT ptCur; GetCursorPos(&ptCur);
	// Если сейчас идет trackPopupMenu - то на выход
	if (!isMenuActive())
		CVConGroup::GetVConFromPoint(ptCur, &VCon);
	CVirtualConsole* pVCon = VCon.VCon();
	if (pVCon && !pVCon->isActive(false))
		pVCon = nullptr;
	CRealConsole *pRCon = pVCon ? pVCon->RCon() : nullptr;

	// Курсор НЕ над консолью?
	if (!pRCon)
	{
		// Status bar "Resize mark"?
		if (gpSet->isStatusBarShow && !gpSet->isStatusColumnHidden[csi_SizeGrip])
		{
			MapWindowPoints(nullptr, ghWnd, &ptCur, 1);

			if (mp_Status->IsCursorOverResizeMark(ptCur))
			{
				LRESULT lResult = 0;
				if (mp_Status->ProcessStatusMessage(ghWnd, WM_SETCURSOR, 0,0, ptCur, lResult))
				{
					DEBUGSTRSETCURSOR(L" ---> Processed by StatusBar\n");
					return TRUE;
				}
			}
		}

		// Если курсор НЕ над консолью - то курсор по умолчанию
		DEBUGSTRSETCURSOR(L" ---> skipped, not over VCon\n");
		return FALSE;
	}

	// В GUI режиме - не ломать курсор, заданный дочерним приложением
	HWND hGuiClient = pRCon->GuiWnd();
	if (pRCon && hGuiClient && pRCon->isGuiVisible())
	{
		DEBUGSTRSETCURSOR(L" ---> skipped (FALSE), GUI App Visible\n");
		return FALSE;
	}

	if (lParam == (LPARAM)-1)
	{
		RECT rcWnd; GetWindowRect(ghWnd, &rcWnd);

		if (!PtInRect(&rcWnd, ptCur))
		{
			if (!isMeForeground())
			{
				DEBUGSTRSETCURSOR(L" ---> skipped, !isMeForeground()\n");
				return FALSE;
			}

			lParam = HTNOWHERE;
		}
		else
		{
			//CVirtualConsole* pVCon = GetVConFromPoint(ptCur); -- уже
			if (pVCon)
				lParam = HTCLIENT;
			else
				lParam = HTCAPTION;
		}

		wParam = pVCon ? (WPARAM)pVCon->GetView() : (WPARAM)ghWnd;
	}

	if (!(((HWND)wParam) == ghWnd || pVCon)
		|| isSizing()
		|| (LOWORD(lParam) != HTCLIENT && LOWORD(lParam) != HTNOWHERE))
	{
		/*if (gpSet->isHideCaptionAlways && !mb_InTrackSysMenu && !isSizing()
			&& (LOWORD(lParam) == HTTOP || LOWORD(lParam) == HTCAPTION))
		{
			SetCursor(mh_CursorMove);
			return TRUE;
		}*/
		DEBUGSTRSETCURSOR(L" ---> skipped, condition\n");
		return FALSE;
	}

	HCURSOR hCur = nullptr;
	//LPCWSTR pszCurName = nullptr;
	BOOL lbMeFore = TRUE;
	ExpandTextRangeType etr = etr_None;

	if (isMenuActive())
	{
		goto DefaultCursor;
	}

	if (LOWORD(lParam) == HTCLIENT && pVCon)
	{
		if (mouse.state & MOUSE_WINDOW_DRAG)
		{
			hCur = LoadCursor(nullptr, IDC_SIZEALL);
		}
		// Если начат ShellDrag
		else if (mh_DragCursor && isDragging())
		{
			hCur = mh_DragCursor;
			DEBUGSTRSETCURSOR(L" ---> DragCursor\n");
		}
		else if (pRCon->isSelectionPresent())
		{
			if (gpSet->isCTSIBeam && (pVCon && !pVCon->CheckMouseOverScroll()))
				hCur = mh_CursorIBeam; // LoadCursor(nullptr, IDC_IBEAM);
		}
		else if ((etr = pRCon->GetLastTextRangeType()) != etr_None)
		{
			hCur = LoadCursor(nullptr, IDC_HAND);
		}
		else if (mouse.state & MOUSE_DRAGPANEL_ALL)
		{
			if (mouse.state & MOUSE_DRAGPANEL_SPLIT)
			{
				hCur = mh_SplitH;
				DEBUGSTRSETCURSOR(L" ---> SplitH (dragging)\n");
			}
			else if ((mouse.state & MOUSE_DRAGPANEL_BOTH) == MOUSE_DRAGPANEL_BOTH)
			{
				hCur = mh_SplitV2;
				DEBUGSTRSETCURSOR(L" ---> SplitV (dragging)\n");
			}
			else
			{
				hCur = mh_SplitV;
				DEBUGSTRSETCURSOR(L" ---> SplitV (dragging)\n");
			}
		}
		else
		{
			lbMeFore = isMeForeground();

			if (lbMeFore)
			{
				HWND hWndDC = pVCon ? pVCon->GetView() : nullptr;
				if (pRCon && pRCon->isFar(FALSE) && hWndDC)  // Плагин не нужен, ФАР сам...
				{
					MapWindowPoints(nullptr, hWndDC, &ptCur, 1);
					COORD crCon = pVCon->ClientToConsole(ptCur.x,ptCur.y);
					enum DragPanelBorder dpb = CheckPanelDrag(crCon);

					if (dpb == DragPanelBorder::Split)
					{
						hCur = mh_SplitH;
						DEBUGSTRSETCURSOR(L" ---> SplitH (allow)\n");
					}
					else if (dpb == DragPanelBorder::Both)
					{
						hCur = mh_SplitV2;
						DEBUGSTRSETCURSOR(L" ---> SplitV2 (allow)\n");
					}
					else if (dpb != DragPanelBorder::None)
					{
						hCur = mh_SplitV;
						DEBUGSTRSETCURSOR(L" ---> SplitV (allow)\n");
					}
				}
			}

			// If mouse is used for selection, or specified modifier is pressed
			if (!hCur && lbMeFore && gpSet->isCTSIBeam)
			{
				// Хм, если выделение настроено "всегда" и без модификатора -
				// на экране всегда будет IBeam курсор, что может раздражать...
				// Поэтому - false
				if (isSelectionModifierPressed(false))
				{
					hCur = mh_CursorIBeam;
				}
			}
		}
	}

	if (!hCur && lbMeFore)
	{
		if (mb_WaitCursor)
		{
			hCur = mh_CursorWait;
			DEBUGSTRSETCURSOR(L" ---> CursorWait (mb_CursorWait)\n");
		}
		else if (gpSet->isFarHourglass)
		{
			if (pRCon)
			{
				const bool lbAlive = pRCon->isAlive();
				if (!lbAlive)
				{
					hCur = mh_CursorAppStarting;
					DEBUGSTRSETCURSOR(L" ---> AppStarting (!pRCon->isAlive())\n");
				}
			}
		}
	}

DefaultCursor:
	if (!hCur)
	{
		hCur = mh_CursorArrow;
		DEBUGSTRSETCURSOR(L" ---> Arrow (default)\n");
	}

	SetCursor(hCur);
	return TRUE;
}

// true - если nPID запущен в одной из консолей
bool CConEmuMain::isConsolePID(DWORD nPID)
{
	return CVConGroup::isConsolePID(nPID);
}

LRESULT CConEmuMain::OnShellHook(WPARAM wParam, LPARAM lParam)
{
	/*
	wParam lParam
	HSHELL_GETMINRECT A pointer to a SHELLHOOKINFO structure.
	HSHELL_WINDOWACTIVATEED The HWND handle of the activated window.
	HSHELL_RUDEAPPACTIVATEED The HWND handle of the activated window.
	HSHELL_WINDOWREPLACING The HWND handle of the window replacing the top-level window.
	HSHELL_WINDOWREPLACED The HWND handle of the window being replaced.
	HSHELL_WINDOWCREATED The HWND handle of the window being created.
	HSHELL_WINDOWDESTROYED The HWND handle of the top-level window being destroyed.
	HSHELL_ACTIVATESHELLWINDOW Not used.
	HSHELL_TASKMAN Can be ignored.
	HSHELL_REDRAW The HWND handle of the window that needs to be redrawn.
	HSHELL_FLASH The HWND handle of the window that needs to be flashed.
	HSHELL_ENDTASK The HWND handle of the window that should be forced to exit.
	HSHELL_APPCOMMAND The APPCOMMAND which has been unhandled by the application or other hooks. See WM_APPCOMMAND and use the GET_APPCOMMAND_LPARAM macro to retrieve this parameter.
	*/
	switch(wParam)
	{
		case HSHELL_FLASH:
		{
			//HWND hCon = (HWND)lParam;
			//if (!hCon) return 0;
			//for (size_t i = 0; i < countof(mp_VCon); i++) {
			//    if (!mp_VCon[i]) continue;
			//    if (mp_VCon[i]->RCon()->ConWnd() == hCon) {
			//        FLASHWINFO fl = {sizeof(FLASHWINFO)};
			//        if (isMeForeground()) {
			//        	if (mp_VCon[i] != mp_ VActive) { // Только для неактивной консоли
			//                fl.dwFlags = FLASHW_STOP; fl.hwnd = ghWnd;
			//                FlashWindowEx(&fl); // Чтобы мигание не накапливалось
			//        		fl.uCount = 4; fl.dwFlags = FLASHW_ALL; fl.hwnd = ghWnd;
			//        		FlashWindowEx(&fl);
			//        	}
			//        } else {
			//        	fl.dwFlags = FLASHW_ALL|FLASHW_TIMERNOFG; fl.hwnd = ghWnd;
			//        	FlashWindowEx(&fl); // Помигать в GUI
			//        }
			//
			//        fl.dwFlags = FLASHW_STOP; fl.hwnd = hCon;
			//        FlashWindowEx(&fl);
			//        break;
			//    }
			//}
		}
		break;

		case HSHELL_WINDOWCREATED:
		{
			if (isMeForeground())
			{
				HWND hWnd = (HWND)lParam;

				if (!hWnd) return 0;

				DWORD dwPID = 0, dwParentPID = 0; // dwFarPID = 0;
				GetWindowThreadProcessId(hWnd, &dwPID);

				if (dwPID && dwPID != GetCurrentProcessId())
				{
					AllowSetForegroundWindow(dwPID);

					WARNING("Проверить, успевает ли диалог показаться, или потом для него ShowWindow зовется?");
					if (IsWindowVisible(hWnd))  // was it in time?
					{
						bool lbConPID = isConsolePID(dwPID);

						PROCESSENTRY32 prc = {};
						if (!lbConPID)
						{
							// Получить PID родительского процесса этого окошка
							if (GetProcessInfo(dwPID, prc))
							{
								dwParentPID = prc.th32ParentProcessID;

								lbConPID = (dwParentPID && isConsolePID(dwParentPID));
							}
						}

						if (lbConPID)
						{
							apiSetForegroundWindow(hWnd);
						}
					}

					//#ifdef _DEBUG
					//wchar_t szTitle[255], szClass[255], szMsg[1024];
					//GetWindowText(hWnd, szTitle, 254); GetClassName(hWnd, szClass, 254);
					//_wsprintf(szMsg, countof(szMsg), L"Window was created:\nTitle: %s\nClass: %s\nPID: %i", szTitle, szClass, dwPID);
					//MBox(szMsg);
					//#endif
				}
			}
		}
		break;

		#ifdef _DEBUG
		case HSHELL_ACTIVATESHELLWINDOW:
		{
			// Is not called
			wchar_t szDbg[128];
			swprintf_c(szDbg, L"HSHELL_ACTIVATESHELLWINDOW(lParam=0x%08X)\n", LODWORD(lParam));
			DEBUGSTRFOREGROUND(szDbg);
		}
		break;
		#endif

		case HSHELL_WINDOWACTIVATED:
		{
			// Приходит позже чем WM_ACTIVATE(WA_INACTIVE), но пригодится, если CE был НЕ в фокусе
			#ifdef _DEBUG
			// Когда активируется Desktop - lParam == 0

			wchar_t szDbg[128], szClass[64];
			if (!lParam || !GetClassName(reinterpret_cast<HWND>(lParam), szClass, 63))
				wcscpy_c(szClass, L"<nullptr>");

			const BOOL lbLBtn = isPressed(VK_LBUTTON);
			swprintf_c(szDbg, L"HSHELL_WINDOWACTIVATED(lParam=0x%08X, %s, %i)\n", LODWORD(lParam), szClass, lbLBtn);
			DEBUGSTRFOREGROUND(szDbg);
			#endif

			//CheckFocus(L"HSHELL_WINDOWACTIVATED");
			OnFocus(nullptr, 0, 0, 0, gsFocusShellActivated);
		}
		break;
	}

	return 0;
}

UINT_PTR CConEmuMain::SetKillTimer(bool bEnable, UINT nTimerID, UINT nTimerElapse)
{
	UINT_PTR nRc;
	if (bEnable)
		nRc = ::SetTimer(ghWnd, nTimerID, nTimerElapse, nullptr);
	else
		nRc = ::KillTimer(ghWnd, nTimerID);
	return nRc;
}

LRESULT CConEmuMain::OnTimer(WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	extern SYSTEMTIME gstLastTimer;
	GetLocalTime(&gstLastTimer);

#ifdef _DEBUG
	wchar_t szDbg[128]; swprintf_c(szDbg, L"ConEmu:MainTimer(%u)\n", wParam);
	DEBUGSTRTIMER(szDbg);
#endif

	//if (mb_InTimer) return 0; // чтобы ненароком два раза в одно событие не вошел (хотя не должен)
	mb_InTimer = TRUE;
	//result = OnTimer(wParam, lParam);

#ifdef DEBUGSHOWFOCUS
	HWND hFocus = GetFocus();
	HWND hFore = GetForegroundWindow();
	static HWND shFocus, shFore;
	static wchar_t szWndInfo[1200];

	if (hFocus != shFocus)
	{
		swprintf_c(szWndInfo, L"(Fore=0x%08X) Focus was changed to ", (DWORD)hFore);
		getWindowInfo(hFocus, szWndInfo+_tcslen(szWndInfo));
		wcscat(szWndInfo, L"\n");
		DEBUGSHOWFOCUS(szWndInfo);
		shFocus = hFocus;
	}

	if (hFore != shFore)
	{
		if (hFore != hFocus)
		{
			wcscpy(szWndInfo, L"Foreground window was changed to ");
			getWindowInfo(hFore, szWndInfo+_tcslen(szWndInfo));
			wcscat(szWndInfo, L"\n");
			DEBUGSHOWFOCUS(szWndInfo);
		}

		shFore = hFore;
	}

#endif

	CVirtualConsole* pVCon = nullptr;
	CVConGuard VCon;
	if (GetActiveVCon(&VCon) >= 0)
		pVCon = VCon.VCon();

	switch (wParam)
	{
		case TIMER_MAIN_ID: // Период: 500 мс
			OnTimer_Main(pVCon);
			break; // case 0:

		case TIMER_CONREDRAW_ID: // Период: CON_REDRAW_TIMOUT*2
			OnTimer_ConRedraw(pVCon);
			break; // case 1:

		case TIMER_CAPTION_APPEAR_ID:
		case TIMER_CAPTION_DISAPPEAR_ID:
			OnTimer_FrameAppearDisappear(wParam);
			break;

		case TIMER_RCLICKPAINT:
			OnTimer_RClickPaint();
			break;

		case TIMER_ADMSHIELD_ID:
			OnTimer_AdmShield();
			break;

		case TIMER_QUAKE_AUTOHIDE_ID:
			OnTimer_QuakeFocus();
			break;

		case TIMER_FAILED_TABBAR_ID:
			mp_TabBar->OnTimer(wParam);
			break;

		case TIMER_ACTIVATESPLIT_ID:
			OnTimer_ActivateSplit();
			break;

		case TIMER_SELECTION_ID: // 20ms TIMER_SELECTION_ELAPSE
			OnTimer_Selection();
			break;
	}

	mb_InTimer = FALSE;
	return result;
}

void CConEmuMain::OnTimer_Background()
{
	// If we run in "Inside" mode, and parent was abnormally terminated, our main thread
	// almost hangs and ConEmu and consoles can't shutdown properly...
	if (!mp_Inside)
		return; // Don't care otherwise

	if (!IsWindow(mp_Inside->GetParentWnd()))
	{
		// Don't kill consoles? Let RealConsoles appear to user?
		// CVConGroup::DoCloseAllVCon(true);

		// We can't do or show anything?
		Destroy(true);
	}
}

void CConEmuMain::OnTimer_Main(CVirtualConsole* pVCon)
{
	#ifdef _DEBUG
	if (mb_DestroySkippedInAssert)
	{
		if (gnInMyAssertTrap <= 0)
		{
			if (CVConGroup::GetConCount() == 0)
			{
				LogString(L"-- Destroy was skipped due to gnInMyAssertTrap, reposting");
				Destroy();
				return;
			}
			else
			{
				mb_DestroySkippedInAssert = false;
			}
		}
	}
	#endif

	//Maximus5. Hack - если какая-то зараза задизейблила окно
	if (!DontEnable::isDontEnable())
	{
		DWORD dwStyle = GetWindowLong(ghWnd, GWL_STYLE);

		if (dwStyle & WS_DISABLED)
			EnableWindow(ghWnd, TRUE);
	}

	HWND hForeWnd = nullptr;
	bool bForeground = RecheckForegroundWindow(L"OnTimer_Main", &hForeWnd);
	if (bForeground && !m_GuiInfo.bGuiActive)
	{
		UpdateGuiInfoMappingActive(true);
	}

	DWORD dwStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	#ifdef CATCH_TOPMOST_SET
	static bool bWasTopMost = false;
	_ASSERTE((bWasTopMost || gpSet->isAlwaysOnTop || ((dwStyleEx & WS_EX_TOPMOST)==0)) && "TopMost mode was set (WM_TIMER)");
	bWasTopMost = ((dwStyleEx & WS_EX_TOPMOST)==WS_EX_TOPMOST);
	#endif
	if (!gpSet->isAlwaysOnTop && ((dwStyleEx & WS_EX_TOPMOST)==WS_EX_TOPMOST))
	{
		CheckTopMostState();
	}

	mp_Status->OnTimer();

	CheckProcesses();
	TODO("Теперь это условие не работает. 1 - раньше это был сам ConEmu.exe");

	if (m_ProcCount == 0)
	{
		// mb_ProcessCreated: Don't close ConEmu if console application exists abnormally,
		//		let user examine its possible console output
		// gnInMsgBox: Don't close ConEmu if there is any MsgBox (config save errors for example)
		if (mb_ProcessCreated && (gnInMsgBox <= 0))
		{
			OnAllVConClosed();
			// Once. Otherwise window we can't show window when "Auto hide" is on...
			mb_ProcessCreated = false;
			return;
		}
	}
	else
	{
		if (!mb_WorkspaceErasedOnClose)
			mb_WorkspaceErasedOnClose = false;
	}

	// Чтобы не возникало "зависаний/блокировок" в потоке чтения консоли - проверяем "живость" сервера
	// Кроме того, здесь проверяется "нужно ли скроллить консоль во время выделения мышкой"
	CVConGroup::OnRConTimerCheck();

	// TODO: поддержку SlideShow повесить на отдельный таймер
	BOOL lbIsPicView = isPictureView();

	if (bPicViewSlideShow)
	{
		DWORD dwTicks = GetTickCount();
		DWORD dwElapse = dwTicks - dwLastSlideShowTick;

		if (dwElapse > gpSet->nSlideShowElapse)
		{
			if (IsWindow(hPictureView) && pVCon)
			{
				//
				bPicViewSlideShow = false;
				SendMessage(pVCon->RCon()->ConWnd(), WM_KEYDOWN, VK_NEXT, 0x01510001);
				SendMessage(pVCon->RCon()->ConWnd(), WM_KEYUP, VK_NEXT, 0xc1510001);
				// Окно могло измениться?
				isPictureView();
				dwLastSlideShowTick = GetTickCount();
				bPicViewSlideShow = true;
			}
			else
			{
				hPictureView = nullptr;
				bPicViewSlideShow = false;
			}
		}
	}

	if (mp_Inside)
	{
		mp_Inside->InsideParentMonitor();
	}

	//2009-04-22 - вроде не требуется
	/*if (lbIsPicView && !isPiewUpdate)
	{
	    // чтобы принудительно обновиться после закрытия PicView
	    isPiewUpdate = true;
	}*/

	if (!lbIsPicView && isPiewUpdate)
	{
		// После скрытия/закрытия PictureView нужно передернуть консоль - ее размер мог измениться
		isPiewUpdate = false;
		CVConGroup::SyncConsoleToWindow();
		InvalidateAll();
	}

	if (!isIconic())
	{
		// Был сдвиг окна? Проверка после отпускания кнопки мышки
		if ((mouse.state & MOUSE_SIZING_BEGIN) || mouse.bCheckNormalRect)
		{
			bool bIsSizing = isSizing();
			if (!bIsSizing)
			{
				StoreNormalRect(nullptr); // Сама разберется, надо/не надо
			}
		}
		//// Было ли реальное изменение размеров?
		//BOOL lbSizingToDo  = (mouse.state & MOUSE_SIZING_TODO) == MOUSE_SIZING_TODO;

		//if (isSizing() && !isPressed(VK_LBUTTON)) {
		//    // Сброс всех флагов ресайза мышкой
		//    ResetSizingFlags(MOUSE_SIZING_BEGIN|MOUSE_SIZING_TODO);
		//}

		//TODO("возможно весь ресайз (кроме SyncNtvdm?) нужно перенести в нить консоли")
		//OnConsoleResize();

		// update scrollbar
		OnUpdateScrollInfo(TRUE);
	}
	else
	{
		if (mouse.bCheckNormalRect)
			mouse.bCheckNormalRect = false;
	}

	// If ThickFrame (resizable) is not used
	if (gpConEmu->isSelfFrame())
	{
		if (!bForeground)
		{
			if (m_ForceShowFrame != fsf_Hide)
			{
				StopForceShowFrame();
			}
		}
		else
		{
			// в Normal режиме при помещении мышки над местом, где должен быть
			// заголовок или рамка - показать их
			if (!isIconic() && !isSizing() && (WindowMode == wmNormal))
			{
				TODO("Не наколоться бы с предыдущим статусом при ресайзе?");
				//static bool bPrevForceShow = false;
				bool bCurForceShow = isMouseOverFrame(true);

				if (bCurForceShow != (m_ForceShowFrame != fsf_Hide))
				{
					m_ForceShowFrame = bCurForceShow ? fsf_WaitShow : fsf_Hide;
					SetKillTimer(false, TIMER_CAPTION_APPEAR_ID, 0);
					SetKillTimer(false, TIMER_CAPTION_DISAPPEAR_ID, 0);
					WORD nID = bCurForceShow ? TIMER_CAPTION_APPEAR_ID : TIMER_CAPTION_DISAPPEAR_ID;
					DWORD nDelay = bCurForceShow ? gpSet->nHideCaptionAlwaysDelay : gpSet->nHideCaptionAlwaysDisappear;

					// Если просили показывать с задержкой - то по таймеру
					if (nDelay)
					{
						LPCWSTR pszLogMsg = bCurForceShow ? L"Timer(TIMER_CAPTION_APPEAR_ID)" : L"Timer(TIMER_CAPTION_DISAPPEAR_ID)";
						if (!LogString(pszLogMsg)) { DEBUGSTRSTYLE(pszLogMsg); }
						SetKillTimer(true, nID, nDelay);
					}
					else if (bCurForceShow)
					{
						StartForceShowFrame();
					}
					else
					{
						StopForceShowFrame();
					}
				}
			}
		}
	}
	else if (m_ForceShowFrame != fsf_Hide)
	{
		StopForceShowFrame();
	}

	if (pVCon)
	{
		bool bLastFade = pVCon->mb_LastFadeFlag;
		bool bNewFade = (gpSet->isFadeInactive && !bForeground && !lbIsPicView);

		// We may get here during "Inside" mode, when "active-ness"
		// is checked by timer via RecheckForegroundWindow
		if (bLastFade != bNewFade)
		{
			pVCon->mb_LastFadeFlag = bNewFade;
			InvalidateAll();
		}

		if (hForeWnd && (hForeWnd == pVCon->GuiWnd()))
		{
			pVCon->RCon()->GuiWndFocusStore();
		}
		else if (pVCon->mb_RestoreChildFocusPending && bForeground)
		{
			pVCon->RCon()->GuiWndFocusRestore();
		}
	}

	if (mh_ConEmuAliveEvent && !mb_ConEmuAliveOwned)
		isFirstInstance(); // Заодно и проверит...

	#ifndef APPDISTINCTBACKGROUND
	// Если был изменен файл background
	if (gpSetCls->PollBackgroundFile())
	{
		Update(true);
	}
	#endif

	if (isMenuActive() && mp_Tip)
	{
		POINT ptCur; GetCursorPos(&ptCur);
		HWND hPoint = WindowFromPoint(ptCur);
		if (hPoint)
		{
			#if 0
			wchar_t szWinInfo[1024];
			swprintf_c(szWinInfo, L"WindowFromPoint(%i,%i) ", ptCur.x, ptCur.y);
			OutputDebugString(szWinInfo);
			getWindowInfo(hPoint, szWinInfo);
			wcscat_c(szWinInfo, L"\n");
			OutputDebugString(szWinInfo);
			#endif

			wchar_t szClass[128];
			if (GetClassName(hPoint, szClass, countof(szClass))
				&& (lstrcmp(szClass, VirtualConsoleClass) == 0 || lstrcmp(szClass, VirtualConsoleClassMain) == 0)) //-V501
			{
				mp_Tip->HideTip();
			}
		}
	}

	// -- Замена на OnFocus
	//CheckFocus(L"TIMER_MAIN_ID");
	// Проверить, может ConEmu был активирован, а сервер нет?
	OnFocus(nullptr, 0, 0, 0, gsFocusCheckTimer);

	if (!lbIsPicView && gpSet->UpdSet.isUpdateCheckHourly)
	{
		gpSet->UpdSet.CheckHourlyUpdate();
	}

	// Size/Pos, Autostart task, etc.
	gpSet->OnAutoSaveTimer();

}

void CConEmuMain::OnActivateSplitChanged()
{
	DWORD nTimeout = TIMER_ACTIVATESPLIT_ELAPSE;
	bool bActive = gpSet->isActivateSplitMouseOver();
	// Try to get system default timeout only if 'system' options is used
	if (gpSet->bActivateSplitMouseOver == 2)
	{
		if (!SystemParametersInfo(SPI_GETACTIVEWNDTRKTIMEOUT, 0, &nTimeout, 0))
			nTimeout = TIMER_ACTIVATESPLIT_ELAPSE;
		else if (nTimeout < 25)
			nTimeout = 25;
		else if (nTimeout > 2500)
			nTimeout = 2500;
	}
	SetKillTimer(bActive, TIMER_ACTIVATESPLIT_ID, nTimeout);
}

void CConEmuMain::OnTimer_ActivateSplit()
{
	if (!gpSet->isActivateSplitMouseOver())
	{
		SetKillTimer(false, TIMER_ACTIVATESPLIT_ID, 0);
		return;
	}

	if (isMenuActive())
	{
		return;
	}

	HWND hForeWnd = nullptr;
	bool bForeground = isMeForeground(false, true, &hForeWnd);

	CVConGuard VConFromPoint;
	// bForeground - does not fit our needs (it ignores GUI children)
	// 130821 - Don't try to change focus during popup menu active!
	POINT ptCur; GetCursorPos(&ptCur);
	if (!PTDIFFTEST(mouse.ptLastSplitOverCheck, SPLITOVERCHECK_DELTA))
	{
		mouse.ptLastSplitOverCheck = ptCur;
		bool isChildGui = false;
		if (!isIconic()
			&& (hForeWnd == ghWnd
				|| (mp_Inside && mp_Inside->IsParentProcess(hForeWnd))
				|| (isChildGui = CVConGroup::isOurGuiChildWindow(hForeWnd))
			))
		{
			// Курсор над ConEmu?
			RECT rcClient = SizeInfo::WindowRect();
			// Проверяем, в какой из VCon попадает курсор?
			if (PtInRect(&rcClient, ptCur))
			{
				if (CVConGroup::GetVConFromPoint(ptCur, &VConFromPoint))
				{
					bool needActivate = !VConFromPoint->isActive(false);
					if (isChildGui)
					{
						HWND hCursorWnd = WindowFromPoint(ptCur);
						if (hCursorWnd && CVConGroup::isOurGuiChildWindow(hCursorWnd))
						{
							RECT rcFore{};
							GetWindowRect(hCursorWnd, &rcFore);
							if (PtInRect(&rcFore, ptCur))
								needActivate = false;
						}
					}
					if (needActivate)
					{
						CVConGuard VCon;
						// Если был активирован GUI CHILD - то фокус нужно сначала поставить в ConEmu
						if (!bForeground && (GetActiveVCon(&VCon) >= 0) && VCon->RCon()->GuiWnd())
						{
							apiSetForegroundWindow(ghWnd);
						}

						if (Activate(VConFromPoint.VCon()))
						{
							// Если был активен GUI CHILD - то ConEmu может НЕ активироваться
							if (!bForeground && !VConFromPoint.VCon()->RCon()->GuiWnd())
							{
								apiSetForegroundWindow(ghWnd);
							}
						}
					}
				}
			}
		}
	}

	UNREFERENCED_PARAMETER(bForeground);
}

void CConEmuMain::OnTimer_ConRedraw(CVirtualConsole* pVCon)
{
	if (pVCon && !isIconic())
	{
		pVCon->CheckPostRedraw();
	}
}

void CConEmuMain::OnTimer_FrameAppearDisappear(WPARAM wParam)
{
	SetKillTimer(false, wParam, 0);

	if (isMouseOverFrame(true))
		StartForceShowFrame();
	else
		StopForceShowFrame();
}

void CConEmuMain::OnTimer_RClickPaint()
{
	RightClickingPaint(nullptr, nullptr);
}

// Bug in Win7? Sometimes after startup ‘Overlay icon’ was not appeared.
void CConEmuMain::OnTimer_AdmShield()
{
	if (gpSet->isTaskbarOverlay)
	{
		LogString(L"Calling Taskbar_UpdateOverlay from timer");
		Taskbar_UpdateOverlay();
		mn_TBOverlayTimerCounter++;
	}
	if ((mn_TBOverlayTimerCounter >= 5) || !gpSet->isTaskbarOverlay)
	{
		SetKillTimer(false, TIMER_ADMSHIELD_ID, 0);
		LogString(L"Timer for Taskbar_UpdateOverlay was stopped");
	}
}

void CConEmuMain::OnTimer_Selection()
{
	CVConGroup::OnRConSelectionTimerCheck();
}

void CConEmuMain::OnTimer_QuakeFocus()
{
	if (mn_ForceTimerCheckLoseFocus)
	{
		if (RELEASEDEBUGTEST(gpSet->isLogging(),true))
		{
			wchar_t szInfo[80];
			DWORD nDelay = GetTickCount() - mn_ForceTimerCheckLoseFocus;
			swprintf_c(szInfo, L"OnTimer_QuakeFocus, delay %u ms", nDelay);
			LogFocusInfo(szInfo);
		}

		OnFocus(nullptr, 0, 0, 0, gsFocusQuakeCheckTimer);
	}
	else
	{
		LogFocusInfo(L"OnTimer_QuakeFocus skipped, already processed (TSA)");
	}

	SetKillTimer(false, TIMER_QUAKE_AUTOHIDE_ID, 0);
}

void CConEmuMain::OnTransparentSeparate(bool bSetFocus)
{
	if (gpSet->isTransparentSeparate
		&& (gpSet->nTransparent != gpSet->nTransparentInactive)
		&& gpSet->isTransparentAllowed())
	{
		// То нужно обновить "прозрачность"
		OnTransparent(true, bSetFocus);
	}
}

void CConEmuMain::OnTransparent()
{
	// Don't touch transparency during Quake-animation
	if (mn_QuakePercent != 0)
		return;

	bool bSetFocus = mp_Menu->GetRestoreFromMinimized();
	bool bFromFocus = bSetFocus;

	OnTransparent(bFromFocus, bSetFocus);
}

void CConEmuMain::OnTransparent(bool abFromFocus /*= false*/, bool bSetFocus /*= false*/)
{
	bool bForceLayered = false;
	bool bActive = abFromFocus ? bSetFocus : isMeForeground();
	bool bColorKey = gpSet->isColorKeyTransparent;
	UINT nAlpha = (bActive || !gpSet->isTransparentSeparate) ? gpSet->nTransparent : gpSet->nTransparentInactive;
	nAlpha = std::max<UINT>((bActive?MIN_ALPHA_VALUE:MIN_INACTIVE_ALPHA_VALUE), std::min<UINT>(nAlpha, 255));

	if (((gpSet->nTransparent < 255)
			|| (gpSet->isTransparentSeparate && (gpSet->nTransparentInactive < 255))
			|| bColorKey)
		&& !gpSet->isTransparentAllowed())
	{
		nAlpha = 255; bColorKey = false;
	}
	else if (gpSet->isTransparentSeparate && (nAlpha == 255)
		&& ((gpSet->nTransparent < 255) || (gpSet->nTransparentInactive < 255)))
	{
		bForceLayered = true;
	}

	mn_LastTransparentValue = nAlpha;

	TODO("EnableBlurBehind: tabs and toolbar must be rewritten, all parts of GUI must be drawn with Alpha channel");
	#ifdef _DEBUG
	//EnableBlurBehind((nAlpha < 255));
	#endif

	// return true - when state was changes
	if (SetTransparent(ghWnd, nAlpha, bColorKey, gpSet->nColorKeyValue, bForceLayered))
	{
		// Запомнить
		mb_LastTransparentFocused = bActive;

		if (!isMenuActive())
		{
			OnSetCursor();
		}
	}
}

// return true - when state was changes
bool CConEmuMain::SetTransparent(HWND ahWnd, UINT anAlpha/*0..255*/, bool abColorKey /*= false*/, COLORREF acrColorKey /*= 0*/, bool abForceLayered /*= false*/)
{
	#ifdef __GNUC__
	if (!SetLayeredWindowAttributes)
	{
		_ASSERTE(SetLayeredWindowAttributes!=nullptr);
		return false;
	}
	#endif

	BOOL bNeedRedrawOp = FALSE;
	// Тут бы ветвиться по Active/Inactive, но это будет избыточно.
	// Проверка уже сделана в OnTransparent
	UINT nTransparent = std::max<UINT>(MIN_INACTIVE_ALPHA_VALUE, std::min<UINT>(anAlpha, 255));
	DWORD dwExStyle = GetWindowLongPtr(ahWnd, GWL_EXSTYLE);
	bool lbChanged = false;

	if (ahWnd == ghWnd)
		mp_Status->OnTransparency();

	if ((nTransparent >= 255) && !abColorKey && !abForceLayered)
	{
		// Прозрачность отключается (полностью непрозрачный)
		//SetLayeredWindowAttributes(ahWnd, 0, 255, LWA_ALPHA);
		if ((dwExStyle & WS_EX_LAYERED) == WS_EX_LAYERED)
		{
			dwExStyle &= ~WS_EX_LAYERED;
			SetLayeredWindowAttributes(ahWnd, 0, 255, LWA_ALPHA);
			SetWindowStyleEx(ahWnd, dwExStyle);
			lbChanged = true;
			LogString("Transparency: Disabling WS_EX_LAYERED");
		}
		else
		{
			LogString("Transparency: WS_EX_LAYERED was already disabled");
		}
	}
	else
	{
		if ((dwExStyle & WS_EX_LAYERED) == 0)
		{
			dwExStyle |= WS_EX_LAYERED;
			SetWindowStyleEx(ahWnd, dwExStyle);
			bNeedRedrawOp = TRUE;
			lbChanged = true;
			LogString("Transparency: Enabling WS_EX_LAYERED");
		}
		else
		{
			LogString("Transparency: WS_EX_LAYERED was already enabled");
		}

		DWORD nNewFlags = (((nTransparent < 255) || abForceLayered) ? LWA_ALPHA : 0) | (abColorKey ? LWA_COLORKEY : 0);

		BYTE nCurAlpha = 0;
		DWORD nCurFlags = 0;
		COLORREF nCurColorKey = 0;
		BOOL bGet = FALSE;
		if (_GetLayeredWindowAttributes)
			bGet = _GetLayeredWindowAttributes(ahWnd, &nCurColorKey, &nCurAlpha, &nCurFlags);

		if (lbChanged
			|| (!bGet)
			|| (nCurAlpha != nTransparent) || (nCurFlags != nNewFlags)
			|| (abColorKey && (nCurColorKey != acrColorKey)))
		{
			lbChanged = true;
			BOOL bSet = SetLayeredWindowAttributes(ahWnd, acrColorKey, nTransparent, nNewFlags);
			if (gpSet->isLogging())
			{
				wchar_t szInfo[128];
				DWORD nErr = bSet ? 0 : GetLastError();
				swprintf_c(szInfo, L"Transparency: Set(0x%08X, 0x%08X, %u, 0x%X) -> %u:%u",
					LODWORD(ahWnd), acrColorKey, nTransparent, nNewFlags, bSet, nErr);
				LogString(szInfo);
			}
		}
		else
		{
			LogString(L"Transparency: Attributes were not changed");
		}

		// После смены стиля (не было альфа - появилась альфа) измененное окно "выносится наверх"
		// и принудительно перерисовывается. Если в этот момент видим диалог настроек - он затирается.
		if (bNeedRedrawOp)
		{
			HWND hWindows[] = {ghOpWnd, (mp_AttachDlg ? mp_AttachDlg->GetHWND() : nullptr), mp_Find->mh_FindDlg};
			for (size_t i = 0; i < countof(hWindows); i++)
			{
				if (hWindows[i] && (hWindows[i] != ahWnd) && IsWindow(hWindows[i]))
				{
					// Ask the window and its children to repaint
					RedrawWindow(hWindows[i], nullptr, nullptr, RDW_ERASE|RDW_INVALIDATE|RDW_FRAME|RDW_ALLCHILDREN);
				}
			}
		}
	}

	return lbChanged;
}

// Вызовем UpdateScrollPos для АКТИВНОЙ консоли
LRESULT CConEmuMain::OnUpdateScrollInfo(BOOL abPosted/* = FALSE*/)
{
	if (!abPosted)
	{
		PostMessage(ghWnd, mn_MsgUpdateScrollInfo, 0, 0);
		return 0;
	}

	CVConGroup::OnUpdateScrollInfo();

	return 0;
}

// Чтобы при создании ПЕРВОЙ консоли на экране сразу можно было что-то нарисовать
void CConEmuMain::OnVConCreated(CVirtualConsole* apVCon, const RConStartArgsEx *args)
{
	if (mn_StartupFinished == ss_PostCreate2Called)
		mn_StartupFinished = ss_VConAreCreated;
	else if (mn_StartupFinished == ss_Started)
		mn_StartupFinished = ss_VConStarted;
	#ifdef _DEBUG
	else // VCon creation is NOT expected at the moment
		_ASSERTE((mn_StartupFinished >= ss_PostCreate2Called) && (mn_StartupFinished <= ss_VConStarted));
	#endif

	SetScClosePending(false); // сброс

	// Если ConEmu не был закрыт после закрытия всех табов, и запущен новый таб...
	gpSet->ResetSavedOnExit();

	CVConGroup::OnVConCreated(apVCon, args);

	ExecPostGuiMacro();
}

// Зависит от настроек и того, как закрывали
bool CConEmuMain::isDestroyOnClose(bool ScCloseOnEmpty /*= false*/)
{
	CConEmuUpdate::UpdateStep step = gpUpd ? gpUpd->InUpdate() : CConEmuUpdate::UpdateStep::NotStarted;

	bool bNeedDestroy = false;

	if ((step == CConEmuUpdate::UpdateStep::ExitAndUpdate) || (step == CConEmuUpdate::UpdateStep::PostponeUpdate))
	{
		bNeedDestroy = true; // Иначе облом при обновлении
	}
	else if (gpSet->isCloseOnLastTabClose() || mb_ForceQuitOnClose)
	{
		bNeedDestroy = true;
	}
	else if (!gpSet->isCloseOnCrossClick())
	{
		bNeedDestroy = ScCloseOnEmpty;
	}
	else
	{
		// Сюда мы попадаем, если просили оставлять ConEmu только если
		// закрыта была вкладка, а не нажат "крестик" в заголовке
		_ASSERTE(gpSet->isCloseOnCrossClick());
		// mb_ScClosePending выставляется в true при закрытии крестиком
		// То есть, если нажали "крестик" - вызываем закрытие окна ConEmu
		bNeedDestroy = (mb_ScClosePending || ScCloseOnEmpty);
	}

	if (gpSet->isLogging())
	{
		wchar_t sInfo[100];
		swprintf_c(sInfo, L"isDestroyOnClose(%u) {%u,%u,%u,%u,%u,%u} -> %u",
			(UINT)ScCloseOnEmpty, (UINT)step,
			(UINT)gpSet->isCloseOnLastTabClose(), (UINT)gpSet->isCloseOnCrossClick(),
			(UINT)gpSet->isMinOnLastTabClose(), (UINT)gpSet->isHideOnLastTabClose(),
			(UINT)mb_ScClosePending,
			bNeedDestroy);
		LogString(sInfo);
	}

	return bNeedDestroy;
}

void CConEmuMain::OnAllVConClosed()
{
	LogString(L"CConEmuMain::OnAllVConClosed()");
	ShutdownGuiStep(L"AllVConClosed");

	OnAllGhostClosed();

	Taskbar_SetOverlay(nullptr);

	if (isDestroyOnClose())
	{
		Destroy();
	}
	else
	{
		if (gpSet->isMultiHideOnClose == 1)
		{
			if (IsWindowVisible(ghWnd))
			{
				Icon.HideWindowToTray();
			}
		}
		else if (gpSet->isMultiHideOnClose == 2)
		{
			if (!isIconic())
				SendMessage(ghWnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
		}

		if (!mb_WorkspaceErasedOnClose)
		{
			mb_WorkspaceErasedOnClose = true;
			UpdateTitle();
			InvalidateAll();
		}
	}
}

void CConEmuMain::OnGhostCreated(CVirtualConsole* apVCon, HWND ahGhost)
{
	// При появлении на таскбаре первой кнопки с консолью (WinXP и ниже)
	// нужно убрать с него кнопку самого приложения
	// 111127 на Vista тоже кнопку "убирать" нужно
	if (gpSet->isTabsOnTaskBar() && !IsWindows7)
	{
		DWORD curStyle = GetWindowLong(ghWnd, GWL_STYLE);
		DWORD curStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
		DWORD style = (curStyle | WS_POPUP);
		DWORD styleEx = (curStyleEx & ~WS_EX_APPWINDOW);
		if (style != curStyle || styleEx != curStyleEx)
		{
			SetWindowStyle(style);
			SetWindowStyleEx(styleEx);
		}
		Taskbar_DeleteTabXP(ghWnd);
	}
}

void CConEmuMain::OnAllGhostClosed()
{
	if (!gpSet->isWindowOnTaskBar(true))
		return;

	DWORD curStyle = GetWindowLong(ghWnd, GWL_STYLE);
	DWORD curStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE);
	DWORD style = (curStyle & ~WS_POPUP);
	DWORD styleEx = (curStyleEx | WS_EX_APPWINDOW);
	if (style != curStyle || styleEx != curStyleEx)
	{
		SetWindowStyle(style);
		SetWindowStyleEx(styleEx);

		Taskbar_AddTabXP(ghWnd);
	}
}

void CConEmuMain::OnRConStartedSuccess(CRealConsole* apRCon)
{
	// Note, apRCon MAY be nullptr
	mb_ProcessCreated = true;

	if (apRCon != nullptr)
	{
		SetScClosePending(false); // сброс
	}
}

bool CConEmuMain::LogWindowPos(LPCWSTR asPrefix, LPRECT prcWnd /*= nullptr*/)
{
	if (!gpSet->isLogging())
		return false;

	wchar_t szInfo[255];
	RECT rcWnd = {};
	if (ghWnd && !prcWnd)
		GetWindowRect(ghWnd, &rcWnd);
	else if (prcWnd)
		rcWnd = *prcWnd;
	const auto mi = GetNearestMonitor(&rcWnd);
	swprintf_c(szInfo, L"%s: %s %s WindowMode=%s Rect={%i,%i}-{%i,%i} Mon(x%08X)=({%i,%i}-{%i,%i}),({%i,%i}-{%i,%i})",
		asPrefix ? asPrefix : L"WindowPos",
		::IsWindowVisible(ghWnd) ? L"Visible" : L"Hidden",
		::IsIconic(ghWnd) ? L"Iconic" : ::IsZoomed(ghWnd) ? L"Maximized" : L"Normal",
		GetWindowModeName(static_cast<ConEmuWindowMode>(gpSet->isQuakeStyle ? gpSet->_WindowMode : WindowMode)),
		LOGRECTCOORDS(rcWnd), LODWORD(mi.hMon), LOGRECTCOORDS(mi.mi.rcMonitor), LOGRECTCOORDS(mi.mi.rcWork));
	return LogString(szInfo);
}

bool CConEmuMain::LogMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!this || !mp_Log || !gpSet->isLogging(4))
		return false;

	char szLog[128];
	#define CASE_MSG(x) case x: lstrcpynA(szLog, #x, 64); break
	#define CASE_MSG2(x,n) case x: lstrcpynA(szLog, n, 64); break
	switch (uMsg)
	{
	CASE_MSG(WM_CREATE);
	CASE_MSG(WM_WTSSESSION_CHANGE);
	CASE_MSG(WM_SHOWWINDOW);
	CASE_MSG(WM_PAINT);
	CASE_MSG(WM_SETHOTKEY);
	CASE_MSG(WM_WINDOWPOSCHANGING);
	CASE_MSG(WM_WINDOWPOSCHANGED);
	CASE_MSG(WM_SIZE);
	CASE_MSG(WM_SIZING);
	CASE_MSG(WM_MOVE);
	CASE_MSG(WM_MOVING);
	CASE_MSG(WM_MOUSEMOVE);
	CASE_MSG(WM_MOUSEWHEEL);
	CASE_MSG(WM_LBUTTONDOWN);
	CASE_MSG(WM_LBUTTONUP);
	CASE_MSG(WM_LBUTTONDBLCLK);
	CASE_MSG(WM_RBUTTONDOWN);
	CASE_MSG(WM_RBUTTONUP);
	CASE_MSG(WM_RBUTTONDBLCLK);
	CASE_MSG(WM_KEYDOWN);
	CASE_MSG(WM_KEYUP);
	CASE_MSG(WM_CHAR);
	CASE_MSG(WM_DEADCHAR);
	CASE_MSG(WM_SYSKEYDOWN);
	CASE_MSG(WM_SYSKEYUP);
	CASE_MSG(WM_SYSCHAR);
	CASE_MSG(WM_SYSDEADCHAR);
	CASE_MSG2(0x0108,"WM_x0108");
	CASE_MSG2(0x0109,"WM_UNICHAR");
	CASE_MSG(WM_COMMAND);
	CASE_MSG(WM_SYSCOMMAND);
	CASE_MSG(WM_TIMER);
	CASE_MSG(WM_VSCROLL);
	CASE_MSG(WM_GETICON);
	CASE_MSG(WM_SETTEXT);
	CASE_MSG(WM_QUERYENDSESSION);
	CASE_MSG(WM_ENDSESSION);
	default:
		{
			LPCSTR pszReg = nullptr;
			if ((uMsg >= WM_APP) && m__AppMsgs.Get(uMsg, &pszReg) && pszReg && *pszReg)
				sprintf_c(szLog, "Msg%04X(%s)", uMsg, pszReg);
			else
				sprintf_c(szLog, "Msg%04X(%u)", uMsg, uMsg);
		}
	}
	#undef CASE_MSG

	size_t cchLen = strlen(szLog);
	_wsprintfA(szLog+cchLen, SKIPLEN(countof(szLog)-cchLen)
		": HWND=x%08X," WIN3264TEST(" W=x%08X, L=x%08X", " W=x%08X%08X, L=x%08X%08X"),
		LODWORD(hWnd), WIN3264WSPRINT(wParam), WIN3264WSPRINT(lParam));

	return LogString(szLog);
}

/* static */
LRESULT CConEmuMain::MainWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	extern LONG gnMessageNestingLevel;
	MSetter nestedLevel(&gnMessageNestingLevel);
	bool bDestroyed = false;


	if (gpConEmu)
	{
		gpConEmu->mp_HandleMonitor->DoCheck();
		gpConEmu->PreWndProc(messg);

		if (gpConEmu->mn_StartupFinished >= CConEmuMain::ss_Destroyed)
			bDestroyed = true;
	}

	if ((ghWnd == nullptr) && gpConEmu && (gpConEmu->mn_StartupFinished < CConEmuMain::ss_Destroying))
		ghWnd = hWnd; // Set it immediately, let functions use it

	#ifdef _DEBUG
	extern HWND gh__Wnd;
	if (!gh__Wnd && ghWnd)
		gh__Wnd = ghWnd;
	#endif

	gpConEmu->mp_HandleMonitor->DoCheck();

	if (hWnd == ghWnd)
		result = gpConEmu->WndProc(hWnd, messg, wParam, lParam);
	else if (bDestroyed)
		// During shutdown few message may pass here... "SHELLHOOK"
		result = ::DefWindowProc(hWnd, messg, wParam, lParam);
	else
		result = CConEmuChild::ChildWndProc(hWnd, messg, wParam, lParam);

	gpConEmu->mp_HandleMonitor->DoCheck();

	return result;
}

BOOL CConEmuMain::isDialogMessage(MSG &Msg)
{
	BOOL lbDlgMsg = FALSE;
	_ASSERTE(this!=nullptr && this==gpConEmu);

	HWND hDlg = nullptr;

	if (ghOpWnd && IsWindow(ghOpWnd))
	{
		lbDlgMsg = gpSetCls->isDialogMessage(Msg);
	}

	if (!lbDlgMsg && mp_AttachDlg && (hDlg = mp_AttachDlg->GetHWND()))
	{
		if (IsWindow(hDlg))
			lbDlgMsg = IsDialogMessage(hDlg, &Msg);
	}

	if (!lbDlgMsg && (hDlg = mp_Find->mh_FindDlg))
	{
		if (IsWindow(hDlg))
			lbDlgMsg = IsDialogMessage(hDlg, &Msg);
	}

	return lbDlgMsg;
}

bool CConEmuMain::isSkipNcMessage(const MSG& Msg)
{
	// When some GuiChild applications has focus (e.g. PuTTY)
	// Pressing Alt+F4 unexpectedly close all ConEmu's tabs instead of active (PuTTY) only
	if ((Msg.message == WM_SYSCOMMAND) && (Msg.wParam == SC_CLOSE))
	{
		// Message was posted by system?
		if (Msg.time)
		{
			CVConGuard VCon; HWND hGuiChild;
			if ((GetActiveVCon(&VCon) >= 0) && ((hGuiChild = VCon->GuiWnd()) != nullptr))
			{
				VCon->RCon()->PostConsoleMessage(hGuiChild, Msg.message, Msg.wParam, Msg.lParam);
				return true;
			}
		}
	}

	return false;
}

/* static */
/* Window procedure for ghWndWork */
LRESULT CConEmuMain::WorkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	_ASSERTE(ghWndWork==nullptr || ghWndWork==hWnd);

	// Logger
	MSG msgStr = {hWnd, uMsg, wParam, lParam};
	ConEmuMsgLogger::Log(msgStr, ConEmuMsgLogger::msgWork);

	if (gpSet->isLogging(4) && gpConEmu)
	{
		gpConEmu->LogMessage(hWnd, uMsg, wParam, lParam);
	}


	if (gpConEmu)
	{
		gpConEmu->mp_HandleMonitor->DoCheck();
		gpConEmu->PreWndProc(uMsg);
	}

	switch (uMsg)
	{
	case WM_CREATE:
		if (!ghWndWork)
			ghWndWork = hWnd;
		_ASSERTE(ghWndWork == hWnd);
		break;

	case WM_PAINT:
		{
			// Сплиттеры
			PAINTSTRUCT ps = {};
			BeginPaint(hWnd, &ps);
			_ASSERTE(ghWndWork == hWnd);

			CVConGroup::OnPaintGaps(ps.hdc);

			EndPaint(hWnd, &ps);
		} // WM_PAINT
		break;
	case WM_PRINTCLIENT:
		if (wParam && (lParam & PRF_CLIENT))
		{
			CVConGroup::OnPaintGaps((HDC)wParam);
		}
		break;

	case WM_SETCURSOR:
	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
		result = CVConGroup::OnMouseEvent(hWnd, uMsg, wParam, lParam);
		break;

#ifdef _DEBUG
	case WM_MOVE:
		break;
	case WM_SIZE:
		break;
#endif

	default:
		result = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	gpConEmu->mp_HandleMonitor->DoCheck();
	return result;
}

UINT CConEmuMain::RegisterMessage(LPCSTR asLocal, LPCWSTR asGlobal)
{
	UINT nMsg;
	if (asGlobal)
		nMsg = RegisterWindowMessage(asGlobal);
	else
		nMsg = ++mn__FirstAppMsg;
	m__AppMsgs.Set(nMsg, asLocal);
	return nMsg;
}

UINT CConEmuMain::GetRegisteredMessage(LPCSTR asLocal, LPCWSTR asGlobal)
{
	struct impl
	{
		LPCSTR pszFind;
		UINT   nMsg;

		static bool EnumMsg(const UINT& anMsg, const LPCSTR& asMsg, LPARAM lParam)
		{
			impl* p = (impl*)lParam;
			if (asMsg && *asMsg && lstrcmpA(asMsg, p->pszFind) == 0)
			{
				p->nMsg = anMsg;
				return true;
			}
			return false; // continue iterations
		};
	} Impl = { asLocal };

	if (m__AppMsgs.EnumKeysValues(impl::EnumMsg, (LPARAM)&Impl))
	{
		_ASSERTE(Impl.nMsg!=0);
		return Impl.nMsg;
	}
	return RegisterMessage(asLocal, asGlobal);
}

struct CallMainThreadArg
{
public:
	static const DWORD magic_value = 0x7847CABF;
public:
	DWORD  magic_ = magic_value;
	bool   needDelete_ = false; // shall main thread call delete on CallMainThreadArg?
	LPARAM lParam_ = 0;
	DWORD  sourceTID_ = 0;
	DWORD  callTick_ = 0;
	DWORD  receiveTick_ = 0;
	std::function<LRESULT(LPARAM)> fnStd_ = nullptr;
public:
	CallMainThreadArg(const LPARAM lParam, const bool needFree, std::function<LRESULT(LPARAM)>&& fn)
		: needDelete_(needFree)
		, lParam_(lParam)
		, sourceTID_(GetCurrentThreadId())
		, callTick_(GetTickCount())
		, fnStd_(std::move(fn))
	{
	}
};

LRESULT CConEmuMain::CallMainThread(bool bSync, std::function<LRESULT(LPARAM)>&& fn, LPARAM lParam)
{
	LRESULT lRc;
	DWORD duration = 0;
	if (isMainThread() && bSync)
	{
		lRc = fn(lParam);
	}
	else if (!bSync)
	{
		CallMainThreadArg* pArg = new CallMainThreadArg(lParam, true, std::move(fn));
		lRc = PostMessage(ghWnd, mn_MsgCallMainThread, CallMainThreadArg::magic_value, reinterpret_cast<LPARAM>(pArg));
	}
	else
	{
		CallMainThreadArg arg(lParam, false, std::move(fn));
		lRc = SendMessage(ghWnd, mn_MsgCallMainThread, CallMainThreadArg::magic_value, reinterpret_cast<LPARAM>(&arg));
		duration = arg.receiveTick_ - arg.callTick_;
	}
	std::ignore = duration;
	return lRc;
}

// Speed up selection modifier checks
void CConEmuMain::PreWndProc(UINT messg)
{
	m_Pressed.bChecked = FALSE;
}

LRESULT CConEmuMain::OnActivateByMouse(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	wchar_t szLog[100];
	swprintf_c(szLog, L"Activation by mouse: HWND=x%08X msg=x%X wParam=x%X lParam=x%X", LODWORD(hWnd), messg, LODWORD(wParam), LODWORD(lParam));
	if (gpSet->isLogging())
		LogString(szLog);
	else
		DEBUGSTRFOCUS(szLog);

	// просто так фокус в дочернее окно ставить нельзя
	// если переключать фокус в дочернее приложение по любому чиху
	// вообще не получается активировать окно ConEmu, открыть системное меню,
	// клацнуть по крестику в заголовке и т.п.
	if (wParam && lParam && gpSet->isFocusInChildWindows)
	{
		CheckAllowAutoChildFocus((DWORD)lParam);
	}

	BOOL bTouchActivation = this->mouse.bTouchActivation;

	//return MA_ACTIVATEANDEAT; -- ест все подряд, а LBUTTONUP пропускает :(
	this->mouse.nSkipEvents[0] = 0;
	this->mouse.nSkipEvents[1] = 0;
	this->mouse.bTouchActivation = false;

	bool bSkipActivation = false;

	if (mp_TabBar && mp_TabBar->IsSearchShown(true))
	{
		RECT rcWork = WorkspaceRect();
		POINT ptCur = {}; GetCursorPos(&ptCur);
		MapWindowPoints(ghWnd, nullptr, (LPPOINT)&rcWork, 2);
		if (PtInRect(&rcWork, ptCur))
		{
			bSkipActivation = true;
			CVConGuard VCon;
			if (CVConGroup::GetVConFromPoint(ptCur, &VCon)
				&& !VCon->CheckMouseOverScroll())
			{
				mp_TabBar->ActivateSearchPane(false);
			}
		}
	}

	if (this->mouse.bForceSkipActivation  // force activation of the Windows which is a child of Desktop
		|| bSkipActivation
		|| (gpSet->isMouseSkipActivation
			&& (LOWORD(lParam) == HTCLIENT)
			&& (bTouchActivation || !(m_Foreground.ForegroundState & (fgf_ConEmuMain|fgf_InsideParent))))
		)
	{
		this->mouse.bForceSkipActivation = false; // Once
		POINT ptMouse = {0}; GetCursorPos(&ptMouse);
		if (IsGesturesEnabled() || CVConGroup::GetVConFromPoint(ptMouse))
		{
			DEBUGSTRFOCUS(L"!Skipping mouse activation message!");
			if (HIWORD(lParam) == WM_LBUTTONDOWN)
			{
				SetSkipMouseEvent(WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK);
			}
			else if (HIWORD(lParam) == WM_RBUTTONDOWN)
			{
				SetSkipMouseEvent(WM_RBUTTONDOWN, WM_RBUTTONUP, WM_RBUTTONDBLCLK);
			}
			else if (HIWORD(lParam) == WM_MBUTTONDOWN)
			{
				SetSkipMouseEvent(WM_MBUTTONDOWN, WM_MBUTTONUP, WM_MBUTTONDBLCLK);
			}
			else if (HIWORD(lParam) == 0x0246/*WM_POINTERDOWN*/)
			{
				// Following real WM_LBUTTONDOWN activation is expected
				this->mouse.bTouchActivation = true;
			}
			else
			{
				_ASSERTE(FALSE && "Unexpected mouse activation message");
			}
		}
		else
		{
			DEBUGSTRFOCUS(L"!Bypassing ClickActivation to console (No Gesture, No VCon)!");
		}
	}
	else
	{
		swprintf_c(szLog, L"!Bypassing ClickActivation to console (Condition, State=x%X)!", m_Foreground.ForegroundState);
		DEBUGSTRFOCUS(szLog);
	}

	if (this->mp_Inside)
	{
		apiSetForegroundWindow(ghWnd);
	}

	RecheckForegroundWindow(L"OnActivateByMouse", nullptr, GetRootHWND());

	return result;
}

// Window procedure for ghWnd
// BUT! It may be called from child windows, e.g. for keyboard processing
LRESULT CConEmuMain::WndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	//MCHKHEAP

	// Logger
	MSG msgStr = {hWnd, messg, wParam, lParam};
	ConEmuMsgLogger::Log(msgStr, ConEmuMsgLogger::msgMain);

	if (gpSet->isLogging(4))
	{
		LogMessage(hWnd, messg, wParam, lParam);
	}

	#ifdef _DEBUG
	wchar_t szDbg[127] = L"";
	if (messg == WM_TIMER || messg == WM_GETICON)
	{
		bool lbDbg1 = false;
	}
	else if (messg >= WM_APP)
	{
		bool lbDbg2 = false;
		wchar_t szName[64];
		LPCSTR pszReg = nullptr;
		if (m__AppMsgs.Get(messg, &pszReg) && pszReg && *pszReg)
			MultiByteToWideChar(CP_ACP, 0, pszReg, -1, szName, countof(szName));
		else
			swprintf_c(szName, L"x%03X", messg);
		 swprintf_c(szDbg, L"WndProc(%i{%s},%i,%i)\n", messg, szName, (DWORD)wParam, (DWORD)lParam);
		DEBUGSTRMSG2(szDbg);
	}
	else
	{
		swprintf_c(szDbg, L"WndProc(%i{x%03X},%i,%i)\n", messg, messg, (DWORD)wParam, (DWORD)lParam);
		DEBUGSTRMSG(hWnd, messg, wParam, lParam);
	}
	#endif

	PatchMsgBoxIcon(hWnd, messg, wParam, lParam);

	if (messg == WM_SYSCHAR)  // Для пересылки в консоль не используется, но чтобы не пищало - необходимо
		return TRUE;

	if (this->ProcessNcMessage(hWnd, messg, wParam, lParam, result))
		return result;

	if (this->ProcessGestureMessage(hWnd, messg, wParam, lParam, result))
		return result;

	//if (messg == WM_CHAR)
	//  return TRUE;

	switch (messg)
	{
		case WM_CREATE:
			result = this->OnCreate(hWnd, (LPCREATESTRUCT)lParam);
			break;

		case WM_SETHOTKEY:
			gnWndSetHotkeyOk = wParam;
			result = ::DefWindowProc(hWnd, messg, wParam, lParam);
			break;

		case WM_WTSSESSION_CHANGE:
			result = OnSessionChanged(hWnd, messg, wParam, lParam);
			break;

		case WM_QUERYENDSESSION:
			result = OnQueryEndSession(hWnd, messg, wParam, lParam);
			break;

		case WM_NOTIFY:
		{
			#if !defined(CONEMU_TABBAR_EX)
			if (this->mp_TabBar)
			{
				if (this->mp_TabBar->OnNotify((LPNMHDR)lParam, result))
					return result;
			}
			#endif

			break;
		}
		case WM_COMMAND:
		{
			if (this->mp_TabBar)
				this->mp_TabBar->OnCommand(wParam, lParam);

			result = 0;
			break;
		}
		case WM_INITMENUPOPUP:
			return this->mp_Menu->OnInitMenuPopup(hWnd, (HMENU)wParam, lParam);
		case WM_MENUSELECT:
			this->mp_Menu->OnMenuSelected((HMENU)lParam, LOWORD(wParam), HIWORD(wParam));
			return 0;
		case WM_MENURBUTTONUP:
			this->mp_Menu->OnMenuRClick((HMENU)lParam, (UINT)wParam);
			return 0;
		case WM_PRINT:
			if (wParam)
			{
				WPARAM lfix = lParam;
				if (IsWin10() && isCaptionHidden())
					lfix &= ~PRF_NONCLIENT;
				DefWindowProc(hWnd, messg, wParam, lParam);
			}
			return 0;
		case WM_PRINTCLIENT:
			if (wParam && (hWnd == ghWnd) /*&& gpSet->isStatusBarShow*/ && (lParam & PRF_CLIENT))
			{
				LRESULT lPaintRc = 0;
				OnPaint(hWnd, (HDC)wParam, WM_PRINTCLIENT, lPaintRc);

				//int nHeight = gpSet->StatusBarHeight();
				//RECT wr = CalcRect(CER_MAINCLIENT);
				//RECT rcStatus = {wr.left, wr.bottom - nHeight, wr.right, wr.bottom};
				//mp_Status->PaintStatus((HDC)wParam, rcStatus);
			}
			break;
		case WM_TIMER:
			result = this->OnTimer(wParam, lParam);
			break;
		case WM_SIZING:
		{
			RECT* pRc = (RECT*)lParam;
			wchar_t szDbg[128]; swprintf_c(szDbg, L"WM_SIZING (Edge%i, {%i,%i}-{%i,%i})\n", (DWORD)wParam, LOGRECTCOORDS(*pRc));

			if (!isIconic())
				result = this->OnSizing(wParam, lParam);

			size_t cchLen = wcslen(szDbg);
			swprintf_c(szDbg+cchLen, countof(szDbg)-cchLen/*#SECURELEN*/, L" -> ({%i,%i}-{%i,%i})\n", LOGRECTCOORDS(*pRc));
			LogString(szDbg, true, false);
			DEBUGSTRSIZE(szDbg);
		} break;

		case WM_MOVING:
		{
			RECT* pRc = (RECT*)lParam;
			if (pRc)
			{
				wchar_t szDbg[128]; swprintf_c(szDbg, L"WM_MOVING ({%i,%i}-{%i,%i})", LOGRECTCOORDS(*pRc));

				result = this->OnMoving(pRc, true);

				size_t cchLen = wcslen(szDbg);
				swprintf_c(szDbg+cchLen, countof(szDbg)-cchLen/*#SECURELEN*/, L" -> ({%i,%i}-{%i,%i})\n", LOGRECTCOORDS(*pRc));
				LogString(szDbg, true, false);
				DEBUGSTRSIZE(szDbg);
			}
		} break;

		case WM_SHOWWINDOW:
		{
			wchar_t szDbg[128]; swprintf_c(szDbg, L"WM_SHOWWINDOW (Show=%i, Status=%i)\r\n", (DWORD)wParam, (DWORD)lParam);
			LogString(szDbg, true, false);
			DEBUGSTRSIZE(szDbg);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			if (wParam == FALSE)
			{
				if (!Icon.isWindowInTray() && !Icon.isHidingToTray() && !this->InQuakeAnimation())
				{
					//_ASSERTE(Icon.isHidingToTray());
					Icon.HideWindowToTray(gpSet->isDownShowHiddenMessage ? nullptr : _T("ConEmu was hidden from some program"));
					this->mb_ExternalHidden = true;
				}
			}
			else if (this->mb_ExternalHidden)
			{
				Icon.RestoreWindowFromTray(TRUE);
				this->mb_ExternalHidden = false;
			}

			//Логирование, что получилось
			if (gpSet->isLogging())
			{
				LogWindowPos(L"WM_SHOWWINDOW.end");
			}
		} break;

		case WM_SETTINGCHANGE:
		{
			const wchar_t* change_verb = nullptr;
			const wchar_t kEnvironmentVerb[] = L"Environment";
			wchar_t szDbg[128]; swprintf_c(szDbg, L"WM_SETTINGCHANGE: W=x%04X, L=x%04X", (DWORD)wParam, (DWORD)lParam);
			if (lParam && !IsBadStringPtr((LPCWSTR)lParam, 64))
			{
				change_verb = (LPCWSTR)lParam;
				wcscat_c(szDbg, L", ");
				size_t cur_len = wcslen(szDbg);
				lstrcpyn(szDbg + cur_len, change_verb, std::size(szDbg) - cur_len);
			}
			if (!LogString(szDbg))
			{
				DEBUGSTRSETCHANGE(szDbg);
			}

			if (wParam == SPI_SETWORKAREA)
			{
				ReloadMonitorInfo();
			}
			result = ::DefWindowProc(hWnd, messg, wParam, lParam);
			if (wParam == SPI_SETWORKAREA)
			{
				auto wm = GetWindowMode();
				if (wm != wmNormal && !isIconic() && isMeForeground())
					SetWindowMode(wm);
			}

			if (change_verb && 0 == wcscmp(change_verb, kEnvironmentVerb)
				&& gpSet->AutoReloadEnvironment && !gpConEmu->opt.NoAutoEnvReload)
			{
				ReloadEnvironmentVariables();
			}
		} break;

		case WM_DISPLAYCHANGE:
		{
			OnDisplayChanged(LOWORD(wParam), LOWORD(lParam), HIWORD(lParam));
			result = ::DefWindowProc(hWnd, messg, wParam, lParam);
			auto wm = GetWindowMode();
			if (wm != wmNormal && !isIconic() && isMeForeground())
				SetWindowMode(wm);
		} break;
		case /*0x02E0*/ WM_DPICHANGED:
		{
			// Update window DPI, recreate fonts and toolbars
			OnDpiChanged(LOWORD(wParam), HIWORD(wParam), (LPRECT)lParam, true, dcs_Api);
			// Call windows defaults?
			result = ::DefWindowProc(hWnd, messg, wParam, lParam);
		} break;

		case WM_SIZE:
		{
			#ifdef _DEBUG
			DWORD_PTR dwStyle = GetWindowLong(ghWnd, GWL_STYLE);
			wchar_t szDbg[128]; swprintf_c(szDbg, L"WM_SIZE (Type:x%X, {%i, %i}) style=0x%08X\n", (DWORD)wParam, LOWORD(lParam), HIWORD(lParam), (DWORD)dwStyle);
			DEBUGSTRSIZE(szDbg);
			#endif

			//if (gpSet->isDontMinimize && wParam == SIZE_MINIMIZED) {
			//	result = 0;
			//	break;
			//}
			if (!isIconic())
			{
				if (lParam)
				{
					WORD newClientWidth = LOWORD(lParam), newClientHeight = HIWORD(lParam);
					RECT real_client = SizeInfo::RealClientRect();
					if (newClientWidth != RectWidth(real_client) || newClientHeight != RectHeight(real_client))
					{
						_ASSERTE(FALSE && "New client size must be already processed in WM_WINDOWPOSCHANGING");
					}
				}
				result = this->OnSize(HIWORD(wParam)!=2, LOWORD(wParam));
			}
		} break;
		case WM_MOVE:
		{
			#ifdef _DEBUG
			wchar_t szDbg[128]; swprintf_c(szDbg, L"WM_MOVE ({%i, %i})\n", (int)(SHORT)LOWORD(lParam), (int)(SHORT)HIWORD(lParam));
			DEBUGSTRSIZE(szDbg);
			#endif
		} break;
		case WM_WINDOWPOSCHANGING:
		{
			result = this->OnWindowPosChanging(hWnd, messg, wParam, lParam);
		} break;
		case WM_WINDOWPOSCHANGED:
		{
			result = this->OnWindowPosChanged(hWnd, messg, wParam, lParam);
		} break;
		//case WM_NCCALCSIZE:
		//	{
		//		NCCALCSIZE_PARAMS *pParms = nullptr;
		//		LPRECT pRect = nullptr;
		//		if (wParam) pParms = (NCCALCSIZE_PARAMS*)lParam; else pRect = (LPRECT)lParam;
		//		result = DefWindowProc(hWnd, messg, wParam, lParam);
		//		break;
		//	}
		case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO pInfo = (LPMINMAXINFO)lParam;
			result = this->OnGetMinMaxInfo(pInfo);
			break;
		}
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			//case WM_CHAR: -- убрал. Теперь пользуем ToUnicodeEx.
			//case WM_SYSCHAR:
			result = this->OnKeyboard(hWnd, messg, wParam, lParam);

			if (messg == WM_SYSKEYUP || messg == WM_SYSKEYDOWN)
				result = TRUE;

			//if (messg == WM_SYSCHAR)
			//    return TRUE;
			break;

		case WM_CHAR:
		case WM_SYSCHAR:
		case WM_DEADCHAR:
		case WM_SYSDEADCHAR:
		{
			wchar_t szLog[160];
			swprintf_c(szLog,
				L"%s(0x%02X='%c', Scan=%i, lParam=0x%08X)",
				(messg == WM_CHAR) ? L"WM_CHAR" : (messg == WM_SYSCHAR) ? L"WM_SYSCHAR" : (messg == WM_SYSDEADCHAR) ? L"WM_SYSDEADCHAR" : L"WM_DEADCHAR",
				(DWORD)wParam, wParam?(wchar_t)wParam:L' ', ((DWORD)lParam & 0xFF0000) >> 16, (DWORD)lParam);
			if (mp_AltNumpad->OnKeyboard(hWnd, messg, wParam, lParam))
			{
				wcscat_c(szLog, L" processed as AltNumpad");
				LogString(szLog);
				break;
			}
			wcscat_c(szLog, L" must be processed internally in CConEmuMain::OnKeyboard");
			DebugStep(szLog, FALSE);
			DEBUGTEST(wcscat_c(szLog, L"\n")); // for breakpoint
		}
		break;

		case WM_ACTIVATE:
			LogString((wParam == WA_CLICKACTIVE) ? L"Window was activated by mouse click" : (wParam == WA_ACTIVE) ? L"Window was activated somehow" : (wParam == WA_INACTIVE) ? L"Window was deactivated" : L"Unknown state in WM_ACTIVATE");
			RecheckForegroundWindow(L"WM_ACTIVATE");
			// gh#139: Quake was hidden when user presses Win key (Task bar menu)
			//   but ConEmu was unexpectedly slided down after second Win press (menu was hidden)
			if ((gpSet->isQuakeStyle == 2)
				&& (LOWORD(wParam) == WA_ACTIVE)
				&& !mn_QuakePercent
				&& !IsWindowVisible(ghWnd))
			{
				HWND hNextApp = FindNextSiblingApp(true);
				wchar_t szInfo[100];
				swprintf_c(szInfo, L"WM_ACTIVATE skipped because Quake is hidden, bypass focus to x%08X", (DWORD)(DWORD_PTR)hNextApp);
				LogFocusInfo(szInfo);
				break;
			}
			result = this->OnFocus(hWnd, messg, wParam, lParam);
			if (this->mb_AllowAutoChildFocus)
			{
				HWND hFore = getForegroundWindow();
				CVConGuard VCon;
				// Если это наш диалог - пропустить возврат фокуса!
				if (hFore && (hFore != ghWnd) && isMeForeground(false, true))
				{
					this->mb_AllowAutoChildFocus = false; // для отладчика
				}
				else if (this->CanSetChildFocus()
					&& (this->GetActiveVCon(&VCon) >= 0)
					&& VCon->RCon()->isGuiEagerFocus())
				{
					VCon->PostRestoreChildFocus();
				}
				this->mb_AllowAutoChildFocus = false; // однократно
			}
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
		case WM_ACTIVATEAPP:
			if (wParam && mp_Menu->GetInScMinimize())
			{
				// Sometimes, during Quake window minimize (generally by clicking on the taskbar button)
				// OS send WM_ACTIVATEAPP(TRUE) from internals of DefWindowProc(WM_SYSCOMMAND,SC_MINIMIZE).
				// We need to treat that message as 'Lose focus'
				LogString(L"Event 'Application activating' received during ScMinimize! Forcing to 'Application deactivating'");
				wParam = FALSE; lParam = 0;
			}
			LogString(wParam ? L"Application activating" : L"Application deactivating");
			RecheckForegroundWindow(L"WM_ACTIVATEAPP");
			// просто так фокус в дочернее окно ставить нельзя
			// если переключать фокус в дочернее приложение по любому чиху
			// вообще не получается активировать окно ConEmu, открыть системное меню,
			// клацнуть по крестику в заголовке и т.п.
			if (wParam && lParam && gpSet->isFocusInChildWindows)
			{
				CheckAllowAutoChildFocus((DWORD)lParam);
			}
			result = this->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
		case WM_KILLFOCUS:
			result = this->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;
		case WM_SETFOCUS:
			result = this->OnFocus(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam); //-V519
			break;
		case WM_MOUSEACTIVATE:
			result = this->OnActivateByMouse(hWnd, messg, wParam, lParam);
			result = DefWindowProc(hWnd, messg, wParam, lParam);
			break;

		case WM_MOUSEMOVE:
		case WM_MOUSEWHEEL:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_XBUTTONDBLCLK:
			result = this->OnMouse(hWnd, messg, wParam, lParam);
			break;
		case WM_CLOSE:
			_ASSERTE(FALSE && "WM_CLOSE is not called in normal behavior");
			this->OnScClose();
			result = 0;
			break;
		case WM_SYSCOMMAND:
			result = this->mp_Menu->OnSysCommand(hWnd, wParam, lParam, WM_SYSCOMMAND);
			break;

		case WM_TRAYNOTIFY:
			result = Icon.OnTryIcon(hWnd, messg, wParam, lParam);
			break;

		case WM_HOTKEY:
			_ASSERTE(messg!=WM_HOTKEY && "Must be processed from ProcessMessage function");
			GetGlobalHotkeys().OnWmHotkey(wParam);
			return 0;

		case WM_SETCURSOR:
			result = this->OnSetCursor(wParam, lParam);

			if (!result && (lParam != -1))
			{
				// Установка resize-курсоров на рамке, и т.п.
				result = DefWindowProc(hWnd, messg, wParam, lParam);
			}

			MCHKHEAP;
			// If an application processes this message, it should return TRUE to halt further processing or FALSE to continue.
			return result;
		case WM_DESTROY:
			result = this->OnDestroy(hWnd);
			break;
		case WM_IME_CHAR:
		case WM_IME_COMPOSITION:
		case WM_IME_COMPOSITIONFULL:
		case WM_IME_CONTROL:
		case WM_IME_ENDCOMPOSITION:
		case WM_IME_KEYDOWN:
		case WM_IME_KEYUP:
		case WM_IME_NOTIFY:
		case WM_IME_REQUEST:
		case WM_IME_SELECT:
		case WM_IME_SETCONTEXT:
		case WM_IME_STARTCOMPOSITION:
			result = this->OnKeyboardIme(hWnd, messg, wParam, lParam);
			break;

		case WM_INPUTLANGCHANGE:
		case WM_INPUTLANGCHANGEREQUEST:
			if (hWnd == ghWnd)
				result = this->OnLangChange(messg, wParam, lParam);
			else
				_ASSERTE(hWnd == ghWnd); // CConEmuMain::WndProc is expected to be called for ghWnd
			break;

		case WM_GETICON:
			switch (wParam)
			{
			case ICON_BIG: // 1
				return (LRESULT)hClassIcon;
			case ICON_SMALL:
			case ICON_SMALL2:
				if (isInNcPaint() || !gpSet->isTaskbarOverlay || !mh_TaskbarIcon)
				{
					if (gpSet->isLogging(2))
						LogString(L"WM_GETICON: returning hClassIconSm");
					return (LRESULT)hClassIconSm;
				}
				if (gpSet->isLogging(2))
					LogString(L"WM_GETICON: returning mh_TaskbarIcon");
				return (LRESULT)mh_TaskbarIcon;
			default:
				return ::DefWindowProc(hWnd, messg, wParam, lParam);
			}
			break;

		default:

			if (messg == this->mn_MsgPostCreate)
			{
				this->PostCreate(TRUE);
				return 0;
			}
			else if (messg == this->mn_MsgPostCopy)
			{
				this->PostDragCopy(lParam!=0, TRUE);
				return 0;
			}
			else if (messg == this->mn_MsgMyDestroy)
			{
				ShutdownGuiStep(L"DestroyWindow");
				DeinitOnDestroy(hWnd);
				_ASSERTE(hWnd == ghWnd);
				mn_InOurDestroy++;
				DestroyWindow(hWnd);
				mn_InOurDestroy--;
				return 0;
			}
			else if (messg == this->mn_MsgUpdateSizes)
			{
				this->UpdateSizes();
				return 0;
			}
			else if (messg == this->mn_MsgUpdateCursorInfo)
			{
				ConsoleInfoArg* pInfo = (ConsoleInfoArg*)lParam;
				this->UpdateCursorInfo(pInfo);
				free(pInfo);
				return 0;
			}
			else if (messg == this->mn_MsgSetWindowMode)
			{
				this->SetWindowMode((ConEmuWindowMode)wParam);
				return 0;
			}
			else if (messg == this->mn_MsgUpdateTitle)
			{
				this->UpdateTitle();
				return 0;
			}
			else if (messg == this->mn_MsgUpdateScrollInfo)
			{
				return OnUpdateScrollInfo(TRUE);
			}
			else if (messg == this->mn_MsgUpdateTabs)
			{
				this->mn_ReqMsgUpdateTabs = 0;
				DEBUGSTRTABS(L"OnUpdateTabs\n");
				this->mp_TabBar->Update(TRUE);
				return 0;
			}
			else if (messg == this->mn_MsgOldCmdVer)
			{
				CESERVER_REQ_HDR* p = (CESERVER_REQ_HDR*)lParam;
				_ASSERTE(p->cbSize == sizeof(CESERVER_REQ_HDR));
				this->ReportOldCmdVersion(p->nCmd, p->nVersion, (int)wParam, p->nSrcPID, p->hModule, p->nBits);
				return 0;
			}
			else if (messg == this->mn_MsgTabCommand)
			{
				this->TabCommand((ConEmuTabCommand)(int)wParam);
				return 0;
			}
			else if (messg == this->mn_MsgTabSwitchFromHook)
			{
				this->TabCommand((wParam & cvk_Shift) ? ctc_SwitchPrev : ctc_SwitchNext);
				if (!gpSet->isTabLazy)
				{
					mb_InWinTabSwitch = !gpSet->isTabRecent;
					this->TabCommand(ctc_SwitchCommit);
				}
				else
				{
					mb_InWinTabSwitch = TRUE;
				}
				return 0;
			}
			else if (messg == this->mn_MsgWinKeyFromHook)
			{
				// Тут должно передавать ВСЕ модификаторы!
				DWORD VkMod = (DWORD)wParam;
				this->OnKeyboardHook(VkMod);
				//BOOL lbShift = (lParam!=0);
				//OnKeyboardHook(vk, lbShift);
				return 0;
			}
			#if 0
			else if (messg == this->mn_MsgConsoleHookedKey)
			{
				WORD vk = LOWORD(wParam);
				this->OnConsoleKey(vk, lParam);
				return 0;
			}
			#endif
			else if (messg == this->mn_MsgSheelHook)
			{
				this->OnShellHook(wParam, lParam);
				return 0;
			}
			//else if (messg == this->mn_ShellExecuteEx)
			//{
			//	this->GuiShellExecuteExQueue();
			//	return 0;
			//	//GuiShellExecuteExArg* pArg = (GuiShellExecuteExArg*)wParam;
			//	//if (pArg && (wParam != TRUE))
			//	//{
			//	//	_ASSERTE(pArg->lpShellExecute == (SHELLEXECUTEINFO*)lParam);
			//	//	result = this->GuiShellExecuteEx(pArg->lpShellExecute, pArg->pVCon);
			//	//	SafeFree(pArg);
			//	//}
			//	//return result;
			//}
			else if (messg == this->mn_PostConsoleResize)
			{
				this->OnConsoleResize();
				return 0;
			}
			else if (messg == this->mn_ConsoleLangChanged)
			{
				this->OnLangChangeConsole((CVirtualConsole*)lParam, (DWORD)wParam);
				return 0;
			}
			else if (messg == this->mn_MsgPostOnBufferHeight)
			{
				CVConGuard VCon;
				if (this->GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->OnBufferHeight();
				}
				return 0;
				//} else if (messg == this->mn_MsgSetForeground) {
				//	apiSetForegroundWindow((HWND)lParam);
				//	return 0;
			}
			else if (messg == this->mn_MsgFlashWindow)
			{
				return OnFlashWindow(wParam, lParam);
			}
			else if (messg == this->mn_MsgPostAltF9)
			{
				DoMaximizeRestore();
				return 0;
			}
			else if (messg == this->mn_MsgInitInactiveDC)
			{
				CVConGuard VCon;
				if (VCon.Attach((CVirtualConsole*)lParam)
					&& !VCon->isVisible())
				{
					VCon->InitDC(true, true, nullptr, nullptr);
					VCon->LoadConsoleData();
				}

				return 0;
			}
			else if (messg == this->mn_MsgUpdateProcDisplay)
			{
				this->UpdateProcessDisplay(wParam!=0);
				return 0;
			}
			else if (messg == this->mn_MsgFontSetSize)
			{
				gpFontMgr->MacroFontSetSize((int)wParam, (int)lParam);
				return 0;
			}
			else if (messg == this->mn_MsgMacroFontSetName)
			{
				this->PostMacroFontSetName((wchar_t*)lParam, (WORD)(((DWORD)wParam & 0xFFFF0000)>>16), (WORD)(wParam & 0xFFFF), TRUE);
				return 0;
			}
			else if (messg == this->mn_MsgReqChangeCurPalette)
			{
				this->PostChangeCurPalette((LPCWSTR)lParam, (wParam!=0), true);
				return 0;
			}
			else if (messg == this->mn_MsgMacroExecSync)
			{
				return ConEmuMacro::ExecuteMacroSync(wParam, lParam);
			}
			else if (messg == this->mn_MsgDisplayRConError)
			{
				CRealConsole *pRCon = (CRealConsole*)wParam;
				wchar_t* pszErrMsg = (wchar_t*)lParam;

				if (this->isValid(pRCon))
				{
					CVConGuard VCon(pRCon->VCon());
					MsgBox(pszErrMsg, MB_ICONSTOP|MB_SYSTEMMODAL, nullptr, ghWnd, false);
				}

				free(pszErrMsg);
				return 0;
			}
			else if (messg == this->mn_MsgCreateViewWindow)
			{
				CConEmuChild* pChild = (CConEmuChild*)lParam;
				HWND hChild = pChild ? pChild->CreateView() : nullptr;
				return (LRESULT)hChild;
			}
			else if (messg == this->mn_MsgPostTaskbarActivate)
			{
				mb_PostTaskbarActivate = FALSE;
				HWND hFore = GetForegroundWindow();
				CVConGuard VCon;
				if ((this->GetActiveVCon(&VCon) >= 0) && (hFore == ghWnd))
					VCon->OnTaskbarFocus();
				return 0;
			}
			else if (messg == this->mn_MsgInitVConGhost)
			{
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;
				if (this->isValid(pVCon))
					pVCon->InitGhost();
				return 0;
			}
			else if (messg == this->mn_MsgTaskBarCreated)
			{
				this->OnTaskbarCreated();
				result = DefWindowProc(hWnd, messg, wParam, lParam);
				return result;
			}
			else if (messg == this->mn_MsgTaskBarBtnCreated)
			{
				LogString(L"Message 'TaskbarButtonCreated' was received, reapplying icon properties");
				this->OnTaskbarButtonCreated();
				result = DefWindowProc(hWnd, messg, wParam, lParam);
				return result;
			}
			else if (messg == this->mn_MsgDeleteVConMainThread)
			{
				// Танцы с бубном, если последний Release() пришелся в фоновом потоке
				LRESULT lDelRc = 0;
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;
				if (CVConGroup::isValid(pVCon))
				{
					pVCon->DeleteFromMainThread();
					lDelRc = 1;
				}
				return lDelRc;
			}
			else if (messg == this->mn_MsgActivateVCon)
			{
				LRESULT lActivateRc = 0;
				CVirtualConsole* pVCon = (CVirtualConsole*)lParam;
				if (CVConGroup::isValid(pVCon))
				{
					BOOL bActivateRc = this->Activate(pVCon);
					if (bActivateRc)
						lActivateRc = (LRESULT)pVCon;
				}
				return lActivateRc;
			}
			else if (messg == this->mn_MsgPostScClose)
			{
				this->OnScClose();
				return 0;
			}
			else if (messg == this->mn_MsgOurSysCommand)
			{
				this->mp_Menu->OnSysCommand(hWnd, wParam, lParam, messg);
				return 0;
			}
			else if (messg == this->mn_MsgCallMainThread)
			{
				LRESULT lFnRc = -1;
				CallMainThreadArg* pArg = reinterpret_cast<CallMainThreadArg*>(lParam);
				if (wParam == CallMainThreadArg::magic_value && pArg && (pArg->magic_ == CallMainThreadArg::magic_value))
				{
					pArg->receiveTick_ = GetTickCount();

					if (pArg->fnStd_)
					{
						lFnRc = pArg->fnStd_(pArg->lParam_);
					}

					if (pArg->needDelete_)
					{
						delete pArg;
					}
				}
				return lFnRc;
			}

			//else if (messg == this->mn_MsgCmdStarted || messg == this->mn_MsgCmdStopped) {
			//  return this->OnConEmuCmd( (messg == this->mn_MsgCmdStarted), (HWND)wParam, (DWORD)lParam);
			//}


			if (messg)
			{
				// 0x0313 - Undocumented. When you right-click on a taskbar button,
				// Windows sends an undocumented message ($0313) to the corresponding
				// application window. The WPARAM is unused (zero) and the
				// LPARAM contains the mouse position in screen coordinates, in the usual
				// format. By default, WindowProc handles this message by popping up the
				// system menu at the given coordinates.
				if ((messg == WM_CONTEXTMENU) || (messg == 0x0313))
				{
					mp_Menu->SetTrackMenuPlace(tmp_System);
					mp_Tip->HideTip();
				}

				result = DefWindowProc(hWnd, messg, wParam, lParam);

				if ((messg == WM_CONTEXTMENU) || (messg == 0x0313))
				{
					mp_Menu->SetTrackMenuPlace(tmp_None);
					mp_Tip->HideTip();
				}
			}
	}

	return result;
}
