
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
//  Раскомментировать, чтобы сразу после загрузки плагина показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
#endif

#define SHOWDEBUGSTR
//#define MCHKHEAP
#define DEBUGSTRMENU(s) //DEBUGSTR(s)
#define DEBUGSTRINPUT(s) //DEBUGSTR(s)
#define DEBUGSTRDLGEVT(s) //DEBUGSTR(s)
#define DEBUGSTRCMD(s) //DEBUGSTR(s)
#define DEBUGSTRACTIVATE(s) //DEBUGSTR(s)
#define DEBUGSTRCURDIR(s) //DEBUGSTR(s)
#define DEBUGSTRTOUCHINPUT(s) //DEBUGSTR(s)


#include "PluginHeader.h"
#include "PluginSrv.h"
#include "ConEmuPluginBase.h"
#include "ConEmuPluginA.h"
#include "ConEmuPlugin995.h"
#include "ConEmuPlugin1900.h"
#include "ConEmuPlugin2800.h"
#include "PluginBackground.h"
#include "../common/ConEmuCheckEx.h"
#include "../common/EmergencyShow.h"
#include "../common/EnvVar.h"
#include "../common/FarVersion.h"
#include "../common/HkFunc.h"
#include "../common/MFileMapping.h"
#include "../common/MProcess.h"
#include "../common/MSection.h"
#include "../common/MSetter.h"
#include "../common/MWow64Disable.h"
#include "../common/SetEnvVar.h"
#include "../common/WFiles.h"
#include "../common/WConsoleEx.h"
#include "../common/WModuleCheck.h"
#include "../common/WThreads.h"
#include "../ConEmuHk/ConEmuHooks.h"
#include "../ConEmu/version.h"


extern MOUSE_EVENT_RECORD gLastMouseReadEvent;
extern LONG gnDummyMouseEventFromMacro;
extern BOOL gbUngetDummyMouseEvent;
extern SetFarHookMode_t SetFarHookMode;

CPluginBase* gpPlugin = nullptr;

// true - if several versions of ConEmu.dll were detected
bool CPluginBase::gb_DllUniqueWarned = false;
// true - if gb_DllUniqueWarned or DllCheckUnique was called from PluginMenu
bool CPluginBase::gb_DllUniqueChecked = false;
// true - if plugin was loaded inside ConEmu tab with different CESERVER_REQ_VER
bool CPluginBase::gb_ProtocolWarned = false;

static BOOL gbTryOpenMapHeader = FALSE;
static BOOL gbStartupHooksAfterMap = FALSE;

BOOL gbWasDetached = FALSE;
BOOL gbFarWndVisible = FALSE;
CONSOLE_SCREEN_BUFFER_INFO gsbiDetached;

DWORD gnPeekReadCount = 0;
LONG  gnInLongOperation = 0;

bool gbExitFarCalled = false;

HMODULE ghPluginModule = nullptr; // ConEmu.dll - сам плагин
HWND ghConEmuWndDC = nullptr; // Содержит хэндл окна отрисовки. Это ДОЧЕРНЕЕ окно.
DWORD gdwPreDetachGuiPID = 0;
DWORD gdwServerPID = 0;
BOOL TerminalMode = FALSE;
HWND FarHwnd = nullptr;
DWORD gnMainThreadId = 0, gnMainThreadIdInitial = 0;
HANDLE ghMonitorThread = nullptr; DWORD gnMonitorThreadId = 0;
HANDLE ghSetWndSendTabsEvent = nullptr;
FarVersion gFarVersion = {};
WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX];
int maxTabCount = 0, lastWindowCount = 0, gnCurTabCount = 0;
CESERVER_REQ* gpTabs = nullptr; //(ConEmuTab*) Alloc(maxTabCount, sizeof(ConEmuTab));
BOOL gbForceSendTabs = FALSE;
int  gnCurrentWindowType = 0; // WTYPE_PANELS / WTYPE_VIEWER / WTYPE_EDITOR
BOOL gbIgnoreUpdateTabs = FALSE; // выставляется на время CMD_SETWINDOW
BOOL gbRequestUpdateTabs = FALSE; // выставляется при получении события FOCUS/KILLFOCUS
BOOL gbClosingModalViewerEditor = FALSE; // выставляется при закрытии модального редактора/вьювера
MOUSE_EVENT_RECORD gLastMouseReadEvent{};
BOOL gbUngetDummyMouseEvent = FALSE;
LONG gnAllowDummyMouseEvent = 0;
LONG gnDummyMouseEventFromMacro = 0;

extern HMODULE ghHooksModule;
extern BOOL gbHooksModuleLoaded; // TRUE, если был вызов LoadLibrary("ConEmuHk.dll"), тогда его нужно FreeLibrary при выходе


MSection *csData = nullptr;
// результат выполнения команды (пишется функциями OutDataAlloc/OutDataWrite)
CESERVER_REQ* gpCmdRet = nullptr;
// инициализируется как "gpData = gpCmdRet->Data;"
LPBYTE gpData = nullptr, gpCursor = nullptr;
DWORD  gnDataSize=0;

int gnPluginOpenFrom = -1;
DWORD gnReqCommand = -1;
LPVOID gpReqCommandData = nullptr;
HANDLE ghReqCommandEvent = nullptr;
BOOL   gbReqCommandWaiting = FALSE;


UINT gnMsgTabChanged = 0;
MSection *csTabs = nullptr;
BOOL  gbPlugKeyChanged=FALSE;
HKEY  ghRegMonitorKey=nullptr; HANDLE ghRegMonitorEvt=nullptr;
HANDLE ghPluginSemaphore = nullptr;
wchar_t gsFarLang[64] = {0};
BOOL gbNeedPostTabSend = FALSE;
BOOL gbNeedPostEditCheck = FALSE; // проверить, может в активном редакторе изменился статус
int lastModifiedStateW = -1;
BOOL gbNeedPostReloadFarInfo = FALSE;
BOOL gbNeedReloadFarDir = FALSE;
DWORD gnNeedPostTabSendTick = 0;
#define NEEDPOSTTABSENDDELTA 100
#define MONITORENVVARDELTA 1000
MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> *gpConMap;
const CESERVER_CONSOLE_MAPPING_HDR *gpConMapInfo = nullptr;
//AnnotationInfo *gpColorerInfo = nullptr;
BOOL gbStartedUnderConsole2 = FALSE;
DWORD gnSelfPID = 0; //GetCurrentProcessId();
HANDLE ghFarInfoMapping = nullptr;
CEFAR_INFO_MAPPING *gpFarInfo = nullptr, *gpFarInfoMapping = nullptr;
HANDLE ghFarAliveEvent = nullptr;
PanelViewRegInfo gPanelRegLeft = {};
PanelViewRegInfo gPanelRegRight = {};
// Для плагинов PicView & MMView нужно знать, нажат ли CtrlShift при F3
HANDLE ghConEmuCtrlPressed = nullptr, ghConEmuShiftPressed = nullptr;
BOOL gbWaitConsoleInputEmpty = FALSE, gbWaitConsoleWrite = FALSE; //, gbWaitConsoleInputPeek = FALSE;
HANDLE ghConsoleInputEmpty = nullptr, ghConsoleWrite = nullptr; //, ghConsoleInputWasPeek = nullptr;
DWORD GetMainThreadId();
int gnSynchroCount = 0;
bool gbSynchroProhibited = false;
bool gbInputSynchroPending = false;
// When we are in Macro - we can't load certain data from Far and Synchro calls should be throttled
DWORD gnSynchroThrottleTick = 0;
const DWORD gnSynchroThrottleDelay = 500;

struct HookModeFar gFarMode = {sizeof(HookModeFar), true/*bFarHookMode*/,
	false, false, false, false, false, {}, nullptr};
// ReSharper disable once CppInconsistentNaming
extern SetFarHookMode_t SetFarHookMode;  // NOLINT(readability-redundant-declaration)


PluginAndMenuCommands gpPluginMenu[menu_Last] =
{
	{CEMenuEditOutput, menu_EditConsoleOutput, pcc_EditConsoleOutput},
	{CEMenuViewOutput, menu_ViewConsoleOutput, pcc_ViewConsoleOutput},
	{0, menu_Separator1}, // Separator
	{CEMenuShowHideTabs, menu_SwitchTabVisible, pcc_SwitchTabVisible},
	{CEMenuNextTab, menu_SwitchTabNext, pcc_SwitchTabNext},
	{CEMenuPrevTab, menu_SwitchTabPrev, pcc_SwitchTabPrev},
	{CEMenuCommitTab, menu_SwitchTabCommit, pcc_SwitchTabCommit},
	{CEMenuShowTabsList, menu_ShowTabsList, pcc_ShowTabsList},
	{CEMenuShowPanelsList, menu_ShowPanelsList, pcc_ShowPanelsList},
	{0, menu_Separator2},
	{CEMenuGuiMacro, menu_ConEmuMacro}, // должен вызываться "по настоящему", а не через callplugin
	{0, menu_Separator3},
	{CEMenuAttach, menu_AttachToConEmu, pcc_AttachToConEmu},
	{0, menu_Separator4},
	{CEMenuDebug, menu_StartDebug, pcc_StartDebug},
	{CEMenuConInfo, menu_ConsoleInfo, pcc_StartDebug},
};

//#define ConEmu_SysID 0x43454D55 // 'CEMU'

CPluginBase* Plugin()
{
	if (!gpPlugin)
	{
		if (!gFarVersion.dwVerMajor)
			CPluginBase::LoadFarVersion();

		if (gFarVersion.dwVerMajor==1)
			gpPlugin = new CPluginAnsi();
		else if (gFarVersion.dwBuild>=2800)
			gpPlugin = new CPluginW2800();
		else if (gFarVersion.dwBuild>=1900)
			gpPlugin = new CPluginW1900();
		else
			gpPlugin = new CPluginW995();
	}

	return gpPlugin;
}

CPluginBase::CPluginBase()
{
	mb_StartupInfoOk = false;

	ee_Read = ee_Save = ee_Redraw = ee_Close = ee_GotFocus = ee_KillFocus = ee_Change = -1;
	ve_Read = ve_Close = ve_GotFocus = ve_KillFocus = -1;
	se_CommonSynchro = -1;
	wt_Desktop = wt_Panels = wt_Viewer = wt_Editor = wt_Dialog = wt_VMenu = wt_Help = -1;
	ma_Other = ma_Shell = ma_Viewer = ma_Editor = ma_Dialog = ma_Search = ma_Disks = ma_MainMenu = ma_Menu = ma_Help = -1;
	ma_InfoPanel = ma_QViewPanel = ma_TreePanel = ma_FindFolder = ma_UserMenu = -1;
	ma_ShellAutoCompletion = ma_DialogAutoCompletion = -1;
	of_LeftDiskMenu = of_PluginsMenu = of_FindList = of_Shortcut = of_CommandLine = of_Editor = of_Viewer = of_FilePanel = of_Dialog = of_Analyse = of_RightDiskMenu = of_FromMacro = -1;
	fctl_GetPanelDirectory = fctl_GetPanelFormat = fctl_GetPanelPrefix = fctl_GetPanelHostFile = -1;
	pt_FilePanel = pt_TreePanel = -1;

	ms_RootRegKey = nullptr;

	InvalidPanelHandle = (gFarVersion.dwVerMajor >= 3) ? nullptr : INVALID_HANDLE_VALUE;
}

CPluginBase::~CPluginBase()
{
}

void CPluginBase::DllMain_ProcessAttach(HMODULE hModule)
{
	ghPluginModule = (HMODULE)hModule;
	ghWorkingModule = hModule;
	gnSelfPID = GetCurrentProcessId();
	HeapInitialize();
	gfnSearchAppPaths = SearchAppPaths;

	hkFunc.Init(WIN3264TEST(L"ConEmu.dll",L"ConEmu.x64.dll"), ghPluginModule);

	#ifdef SHOW_STARTED_MSGBOX
	if (!IsDebuggerPresent())
		MessageBoxA(nullptr, "ConEmu*.dll loaded", "ConEmu plugin", 0);
	#endif

	gpLocalSecurity = LocalSecurity();

	csTabs = new MSection();
	csData = new MSection();

	// It's possible that our module is loaded from background thread,
	// therefore we have to find Main process thread ID
	gnMainThreadId = gnMainThreadIdInitial = GetMainThreadId();

	InitHWND(/*hConWnd*/);

	// Check Terminal mode
	TerminalMode = isTerminalMode();

	// Check if we can continue initialization for ConEmu cooperation
	if (!TerminalMode
		&& DllCheckUnique())
	{
		PlugServerInit();

		if (!StartupHooks(ghPluginModule))
		{
			if (ghConEmuWndDC)
			{
				_ASSERTE(FALSE);
				DEBUGSTR(L"!!! Can't install injects!!!\n");
			}
			else
			{
				DEBUGSTR(L"No GUI, injects was not installed!\n");
			}
		}
	}
}

void CPluginBase::DllMain_ProcessDetach()
{
	ShutdownPluginStep(L"DLL_PROCESS_DETACH");

	if (!gbExitFarCalled)
	{
		_ASSERTE(FALSE && "ExitFar was not called. Unsupported Far<->Plugin builds?");
		Plugin()->ExitFarCommon(true);
	}

	if (gnSynchroCount > 0)
	{
		//if (gFarVersion.dwVerMajor == 2 && gFarVersion.dwBuild < 1735) -- в фаре пока не чинили, поэтому всегда ругаемся, если что...
		BOOL lbSynchroSafe = FALSE;
		if ((gFarVersion.dwVerMajor == 2 && gFarVersion.dwVerMinor >= 1) || (gFarVersion.dwVerMajor >= 3))
			lbSynchroSafe = TRUE;
		if (!lbSynchroSafe)
		{
			ShowMessageBox(L"Syncho events are pending!\nFar may crash after unloading plugin", MB_OK|MB_ICONEXCLAMATION|MB_SETFOREGROUND|MB_SYSTEMMODAL);
		}
	}

	if (csTabs)
	{
		delete csTabs;
		csTabs = nullptr;
	}

	if (csData)
	{
		delete csData;
		csData = nullptr;
	}

	PlugServerStop(true);

	if (gpBgPlugin)
	{
		delete gpBgPlugin;
		gpBgPlugin = nullptr;
	}

	HeapDeinitialize();

	ShutdownPluginStep(L"DLL_PROCESS_DETACH - done");
}

bool CPluginBase::DllCheckUnique(bool bFromMenu)
{
	if (gb_DllUniqueWarned)
		return false;

	int iCount = 0;
	CEStr lsFiles;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(MODULEENTRY32)};

		for (BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
		{
			LPCWSTR pszName = PointToName(module.szModule);
			if (!lstrcmpi(pszName, L"ConEmu.dll")
				|| !lstrcmpi(pszName, L"ConEmu.x64.dll"))
			{
				iCount++;
				lsFiles.Append(module.szExePath[0] ? module.szExePath : module.szModule, L"\n");
			}
		}

		CloseHandle(snapshot);
	}

	if (iCount <= 1)
	{
		if (bFromMenu)
			gb_DllUniqueChecked = true;
		return true;
	}

	gb_DllUniqueWarned = gb_DllUniqueChecked = true;

	wchar_t szExePath[MAX_PATH] = L"";
	GetModuleFileName(nullptr, szExePath, countof(szExePath));

	const CEStr lsMsg(
		L"Several instances of ConEmu plugin were loaded!\n"
		L"Please, either remove old versions,\n"
		L"or change Far.exe arguments (/p switch)!\n\n",
		szExePath, L"\n",
		lsFiles.ms_Val);

	ShowMessageBox(lsMsg, MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_SYSTEMMODAL);

	return false;
}


bool CPluginBase::LoadFarVersion()
{
	wchar_t ErrText[512]; ErrText[0] = 0;
	bool lbRc = ::LoadFarVersion(gFarVersion, ErrText);

	if (ErrText[0])
	{
		ShowMessageBox(ErrText, MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
	}

	if (!lbRc)
	{
		gFarVersion.dwVerMajor = 2;
		gFarVersion.dwVerMinor = 0;
		gFarVersion.dwBuild = MIN_FAR2_BUILD;
		_ASSERTE(gFarVersion.dwBits == WIN3264TEST(32,64));
	}

	return lbRc;
}

int CPluginBase::ShowMessageBox(LPCWSTR asMessage, UINT uType)
{
	wchar_t szTitle[128], szVer4[8] = L""; lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	swprintf_c(szTitle, L"ConEmu plugin %02u%02u%02u%s%s[%u%s] PID=%u",
		(MVV_1 % 100), MVV_2, MVV_3, szVer4[0] && szVer4[1] ? L"-" : L"", szVer4,
		WIN3264TEST(32,64), RELEASEDEBUGTEST(L"",L"D"),
		GetCurrentProcessId());

	int nRc = ::MessageBoxW(nullptr, asMessage, szTitle, uType);
	return nRc;
}

int CPluginBase::ShowMessageGui(int aiMsg, int aiButtons)
{
	wchar_t wszBuf[MAX_PATH];
	LPCWSTR pwszMsg = GetMsg(aiMsg, wszBuf, countof(wszBuf));


	if (!pwszMsg || !*pwszMsg)
	{
		swprintf_c(wszBuf, L"<MsgID=%i>", aiMsg);
		pwszMsg = wszBuf;
	}

	int nRc = ShowMessageBox(pwszMsg, aiButtons);
	return nRc;
}

/* static, WINAPI, Thread */
DWORD CPluginBase::BackgroundMacroError(LPVOID lpParameter)
{
	wchar_t* pszMacroError = (wchar_t*)lpParameter;

	ShowMessageBox(pszMacroError, MB_ICONSTOP|MB_SYSTEMMODAL);

	SafeFree(pszMacroError);

	return 0;
}

void CPluginBase::PostMacro(const wchar_t* asMacro, INPUT_RECORD* apRec, bool abShowParseErrors)
{
	if (!asMacro || !*asMacro)
		return;

	_ASSERTE(GetCurrentThreadId()==gnMainThreadId);

	MOUSE_EVENT_RECORD mre;

	if (apRec && apRec->EventType == MOUSE_EVENT)
	{
		gLastMouseReadEvent = mre = apRec->Event.MouseEvent;
	}
	else
	{
		mre = gLastMouseReadEvent;
	}

	PostMacroApi(asMacro, apRec, abShowParseErrors);

	//FAR BUGBUG: Макрос не запускается на исполнение, пока мышкой не дернем :(
	//  Это чаще всего проявляется при вызове меню по RClick
	//  Если курсор на другой панели, то RClick сразу по пассивной
	//  не вызывает отрисовку :(

#if 0
	//111002 - попробуем просто gbUngetDummyMouseEvent
	//InterlockedIncrement(&gnDummyMouseEventFromMacro);
	gnDummyMouseEventFromMacro = TRUE;
	gbUngetDummyMouseEvent = TRUE;
#endif
#if 0
	//if (!mcr.Param.PlainText.Flags) {
	INPUT_RECORD ir[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};

	if (isPressed(VK_CAPITAL))
		ir[0].Event.MouseEvent.dwControlKeyState |= CAPSLOCK_ON;

	if (isPressed(VK_NUMLOCK))
		ir[0].Event.MouseEvent.dwControlKeyState |= NUMLOCK_ON;

	if (isPressed(VK_SCROLL))
		ir[0].Event.MouseEvent.dwControlKeyState |= SCROLLLOCK_ON;

	ir[0].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	ir[0].Event.MouseEvent.dwMousePosition = mre.dwMousePosition;

	// Вроде одного хватало, правда когда {0,0} посылался
	ir[1].Event.MouseEvent.dwControlKeyState = ir[0].Event.MouseEvent.dwControlKeyState;
	ir[1].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	//ir[1].Event.MouseEvent.dwMousePosition.X = 1;
	//ir[1].Event.MouseEvent.dwMousePosition.Y = 1;
	ir[0].Event.MouseEvent.dwMousePosition = mre.dwMousePosition;
	ir[0].Event.MouseEvent.dwMousePosition.X++;

	//2010-01-29 попробуем STD_OUTPUT
	//if (!ghConIn) {
	//	ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//	if (ghConIn == INVALID_HANDLE_VALUE) {
	//		#ifdef _DEBUG
	//		DWORD dwErr = GetLastError();
	//		_ASSERTE(ghConIn!=INVALID_HANDLE_VALUE);
	//		#endif
	//		ghConIn = nullptr;
	//		return;
	//	}
	//}
	TODO("Необязательно выполнять реальную запись в консольный буфер. Можно обойтись подстановкой в наших функциях перехвата чтения буфера.");
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD cbWritten = 0;

	// Вроде одного хватало, правда когда {0,0} посылался
	#ifdef _DEBUG
	BOOL fSuccess =
	#endif
	WriteConsoleInput(hIn/*ghConIn*/, ir, 1, &cbWritten);
	_ASSERTE(fSuccess && cbWritten==1);
	//}
	//InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_REDRAWALL,nullptr);
#endif
}

bool CPluginBase::isMacroActive(int& iMacroActive)
{
	if (!FarHwnd)
	{
		return false;
	}

	if (!iMacroActive)
	{
		iMacroActive = IsMacroActive() ? 1 : 2;
	}

	return (iMacroActive == 1);
}

bool CPluginBase::CheckBufferEnabled()
{
	if (!mb_StartupInfoOk)
		return false;

	static int siEnabled = 0;

	// Чтобы проверку выполнять только один раз.
	// Т.к. буфер может быть реально сброшен, а фар его все-еще умеет.
	if (siEnabled)
	{
		return (siEnabled == 1);
	}

	SMALL_RECT rcFar = {0};

	if (GetFarRect(rcFar))
	{
		if (rcFar.Top > 0 && rcFar.Bottom > rcFar.Top)
		{
			siEnabled = 1;
			return true;
		}
	}

	siEnabled = -1;
	return false;
}

INT_PTR CPluginBase::PanelControl(HANDLE hPanel, int Command, INT_PTR Param1, void* Param2)
{
	INT_PTR iRc = -1;

	if (mb_StartupInfoOk && (Command != -1))
	{
		iRc = PanelControlApi(hPanel, Command, Param1, Param2);
	}

	if (Command == fctl_GetPanelDirectory || Command == fctl_GetPanelFormat || Command == fctl_GetPanelPrefix || Command == fctl_GetPanelHostFile)
	{
		if (Param2 && (Param1 > 0) && (iRc <= 0 || iRc > Param1))
		{
			wchar_t* psz = (wchar_t*)Param2;
			*psz = 0;
		}
	}

	return iRc;
}

bool CPluginBase::GetPanelInfo(GetPanelDirFlags Flags, BkPanelInfo* pBk)
{
	CEPanelInfo info = {};
	info.szCurDir = pBk->szCurDir;
	info.szFormat = pBk->szFormat;
	info.szHostFile = pBk->szHostFile;

	bool bRc = GetPanelInfo(Flags, &info);
	if (bRc)
	{
		pBk->bFocused = info.bFocused;
		pBk->bPlugin = info.bPlugin;
		pBk->bVisible = info.bVisible;
		pBk->nPanelType = info.nPanelType;
		pBk->rcPanelRect = info.rcPanelRect;
	}

	return bRc;
}

void CPluginBase::FillUpdateBackground(struct PaintBackgroundArg* pFar)
{
	if (!mb_StartupInfoOk)
		return;

	LoadFarColors(pFar->nFarColors);

	LoadFarSettings(&pFar->FarInterfaceSettings, &pFar->FarPanelSettings);

	pFar->bPanelsAllowed = CheckPanelExist();

	if (pFar->bPanelsAllowed)
	{
		GetPanelInfo(gpdf_Left, &pFar->LeftPanel);
		GetPanelInfo(gpdf_Right, &pFar->RightPanel);
	}

	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO scbi = {};
	GetConsoleScreenBufferInfo(hCon, &scbi);

	SMALL_RECT rc = {0};
	if (CheckBufferEnabled() && GetFarRect(rc))
	{
		pFar->rcConWorkspace.left = rc.Left;
		pFar->rcConWorkspace.top = rc.Top;
		pFar->rcConWorkspace.right = rc.Right;
		pFar->rcConWorkspace.bottom = rc.Bottom;
	}
	else
	{
		pFar->rcConWorkspace.left = pFar->rcConWorkspace.top = 0;
		pFar->rcConWorkspace.right = scbi.dwSize.X - 1;
		pFar->rcConWorkspace.bottom = scbi.dwSize.Y - 1;
		//pFar->conSize = scbi.dwSize;
	}

	pFar->conCursor = scbi.dwCursorPosition;
	CONSOLE_CURSOR_INFO crsr = {0};
	GetConsoleCursorInfo(hCon, &crsr);

	if (!crsr.bVisible || crsr.dwSize == 0)
	{
		pFar->conCursor.X = pFar->conCursor.Y = -1;
	}
}

bool CPluginBase::StorePanelDirs(LPCWSTR asActive, LPCWSTR asPassive)
{
	// No one was specified? Exit.
	if (!(asActive && *asActive) && !(asPassive && *asPassive))
	{
		return false;
	}

	if (!gpFarInfo)
	{
		_ASSERTE(gpFarInfo!=nullptr);
		return false;
	}

	bool bChanged = false;

	for (int i = 0; i <= 1; i++)
	{
		LPCWSTR pszSet = i ? asPassive : asActive;
		_ASSERTE(countof(gpFarInfo->sActiveDir) == countof(gpFarInfo->sPassiveDir));
		wchar_t* pDst = i ? gpFarInfo->sPassiveDir : gpFarInfo->sActiveDir;
		wchar_t* pCpy = gpFarInfoMapping ? (i ? gpFarInfoMapping->sPassiveDir : gpFarInfoMapping->sActiveDir) : nullptr;
		int iLen = lstrlen(pszSet);
		if (iLen >= countof(gpFarInfo->sActiveDir))
		{
			_ASSERTE(FALSE && "Far current dir path is too long");
			iLen = countof(gpFarInfo->sActiveDir)-1;
		}

		if (pszSet && (lstrcmp(pszSet, pDst) != 0))
		{
			#ifdef _DEBUG
			const CEStr lsDbg(i ? L"PPanelDir changed -> " : L"APanelDir changed -> ", pszSet);
			DEBUGSTRCURDIR(lsDbg);
			#endif

			pDst[iLen] = 0;
			wmemmove(pDst, pszSet, iLen);

			if (pCpy)
			{
				pCpy[iLen] = 0;
				wmemmove(pCpy, pszSet, iLen);
			}

			bChanged = true;
		}
	}

	if (bChanged && gpFarInfoMapping)
	{
		LONG lIdx = InterlockedIncrement(&gpFarInfo->nPanelDirIdx);
		gpFarInfoMapping->nPanelDirIdx = lIdx;
	}

	return bChanged;
}

void CPluginBase::UpdatePanelDirs()
{
	if (!gpFarInfo)
	{
		_ASSERTE(gpFarInfo);
		return;
	}

	// It is not safe to call GetPanelDir even from MainThread, even if APanel is not plugin
	// FCTL_GETPANELDIRECTORY вызывает GetFindData что в некоторых случаях вызывает глюки в глючных плагинах...
	// Макрос же вроде возвращает "текущую" информацию

	if (gFarVersion.dwVerMajor >= 2)
	{
		wchar_t szMacro[200] = L"";
		if (gFarVersion.dwVerMajor == 2)
		{
			if (isEditorViewer())
			{
				// Don't ping Far when Editor/Viewer are active
				return;
			}
			swprintf_c(szMacro,
				L"$if (!Editor && !Viewer)"
				L" $if (!APanel.Plugin) callplugin(0x%08X,%i) $end "
				L" $if (!PPanel.Plugin) callplugin(0x%08X,%i) $end "
				L"$end",
				ConEmu_SysID, CE_CALLPLUGIN_REQ_DIRA, ConEmu_SysID, CE_CALLPLUGIN_REQ_DIRP);
		}
		else if (!gFarVersion.IsFarLua())
		{
			swprintf_c(szMacro,
				L"$if (APanel.Plugin) %%A=\"\"; $else %%A=APanel.Path0; $end"
				L" $if (PPanel.Plugin) %%P=\"\"; $else %%P=PPanel.Path0; $end"
				L" Plugin.Call(\"%s\",%i,%%A,%%P)",
				ConEmu_GuidS, CE_CALLPLUGIN_REQ_DIRS);
		}
		else
		{
			swprintf_c(szMacro,
				L"Plugin.Call(\"%s\",%i,APanel.Plugin and \"\" or APanel.Path0,PPanel.Plugin and \"\" or PPanel.Path0)",
				ConEmu_GuidS, CE_CALLPLUGIN_REQ_DIRS);
		}

		if (szMacro[0])
		{
			DEBUGSTRCURDIR(szMacro);
			// Issue 1777: Хоть глюки фара и нужно лечить в фаре, но пока ошибки просто пропустим
			PostMacro(szMacro, nullptr, false);
			return; // Async
		}
	}

	// Here we are only for Far 1.7x
	_ASSERTE(gFarVersion.dwVerMajor == 1);

	bool bChanged = false;

	wchar_t szTitle[MAX_PATH*2] = L"";
	DWORD nLen = GetConsoleTitle(szTitle, countof(szTitle));
	if (!nLen || nLen >= countof(szTitle))
		return; // Invalid result?
	if (szTitle[0] != L'{' || !isDriveLetter(szTitle[1]) || szTitle[2] != L':')
		return; // Not a path in the title;

	wchar_t * pszFar = wcsstr(szTitle, L"} - Far");
	if (!pszFar)
		return; // Valid terminator not found

	*pszFar = 0;

	// It will update mapping and increase .nPanelDirIdx
	StorePanelDirs(szTitle+1, nullptr);
}

bool CPluginBase::RunExternalProgram(wchar_t* pszCommand)
{
	wchar_t *pszExpand = nullptr;
	CEStr szTemp, szExpand, szCurDir;

	if (!pszCommand || !*pszCommand)
	{
		if (!InputBox(L"ConEmu", L"Start console program", L"ConEmu.CreateProcess", L"cmd", szTemp.ms_Val))
			return false;

		pszCommand = szTemp.ms_Val;
	}

	if (wcschr(pszCommand, L'%'))
	{
		// ReSharper disable once CppJoinDeclarationAndAssignment
		szExpand = ExpandEnvStr(pszCommand);
		if (szExpand.ms_Val)
			pszCommand = szExpand.ms_Val;
	}

	szCurDir.ms_Val = GetPanelDir(gpdf_Active|gpdf_NoPlugin);
	if (!szCurDir.ms_Val || !*szCurDir.ms_Val)
	{
		szCurDir.Set(L"C:\\");
	}

	bool bSilent = (wcsstr(pszCommand, L"-new_console") != nullptr);

	if (!bSilent)
		ShowUserScreen(true);

	RunExternalProgramW(pszCommand, szCurDir.ms_Val, bSilent);

	if (!bSilent)
		ShowUserScreen(false);
	RedrawAll();

	return TRUE;
}

bool CPluginBase::ProcessCommandLine(wchar_t* pszCommand)
{
	if (!pszCommand)
		return false;

	if (lstrcmpni(pszCommand, L"run:", 4) == 0)
	{
		RunExternalProgram(pszCommand+4); //-V112
		return true;
	}

	return false;
}

void CPluginBase::ShowPluginMenu(PluginCallCommands nCallID /*= pcc_None*/)
{
	int nItem = -1;

	if (!FarHwnd)
	{
		ShowMessage(CEInvalidConHwnd,0); // "ConEmu plugin\nGetConsoleWindow()==FarHwnd is nullptr"
		return;
	}

	if (IsTerminalMode())
	{
		ShowMessage(CEUnavailableInTerminal,0); // "ConEmu plugin\nConEmu is not available in terminal mode\nCheck TERM environment variable"
		return;
	}

	// If our module was loaded before some other old version,
	// we have not warned user about duplicate modules
	DllCheckUnique(true);

	CheckConEmuDetached();

	if (nCallID != pcc_None)
	{
		// Команды CallPlugin
		for (size_t i = 0; i < countof(gpPluginMenu); i++)
		{
			if (gpPluginMenu[i].CallID == nCallID)
			{
				nItem = gpPluginMenu[i].MenuID;
				break;
			}
		}
		_ASSERTE(nItem!=-1);

		SHOWDBGINFO(L"*** ShowPluginMenu used default item\n");
	}
	else
	{
		ConEmuPluginMenuItem items[menu_Last] = {};
		int nCount = menu_Last; //sizeof(items)/sizeof(items[0]);
		_ASSERTE(nCount == countof(gpPluginMenu));
		for (int i = 0; i < nCount; i++)
		{
			if (!gpPluginMenu[i].LangID)
			{
				items[i].Separator = true;
				continue;
			}
			_ASSERTE(i == gpPluginMenu[i].MenuID);
			items[i].Selected = pcc_Selected((PluginMenuCommands)i);
			items[i].Disabled = pcc_Disabled((PluginMenuCommands)i);
			items[i].MsgID = gpPluginMenu[i].LangID;
		}

		SHOWDBGINFO(L"*** calling ShowPluginMenu\n");
		nItem = ShowPluginMenu(items, nCount);
	}

	if (nItem < 0)
	{
		SHOWDBGINFO(L"*** ShowPluginMenu cancelled, nItem < 0\n");
		return;
	}

	#ifdef _DEBUG
	wchar_t szInfo[128]; swprintf_c(szInfo, L"*** ShowPluginMenu done, nItem == %i\n", nItem);
	SHOWDBGINFO(szInfo);
	#endif

	switch (nItem)
	{
		case menu_EditConsoleOutput:
		case menu_ViewConsoleOutput:
		{
			// Открыть в редакторе вывод последней консольной программы
			EditViewConsoleOutput((nItem==1)/*abView*/);
		} break;

		case menu_SwitchTabVisible: // Показать/спрятать табы
		case menu_SwitchTabNext:
		case menu_SwitchTabPrev:
		case menu_SwitchTabCommit:
		{
			SwitchTabCommand((PluginMenuCommands)nItem);
		} break;

		case menu_ShowTabsList:
		{
			ShowTabsList();
		} break;

		case menu_ShowPanelsList:
		{
			ShowPanelsList();
		} break;

		case menu_ConEmuMacro: // Execute GUI macro (gialog)
		{
			GuiMacroDlg();
		} break;

		case menu_AttachToConEmu: // Attach to GUI (если FAR был CtrlAltTab)
		{
			if (TerminalMode) break;  // низзя

			if (ghConEmuWndDC && IsWindow(ghConEmuWndDC)) break;  // Мы и так подключены?

			Attach2Gui();
		} break;

		//#ifdef _DEBUG
		//case 11: // Start "ConEmuC.exe /DEBUGPID="
		//#else
		case menu_StartDebug: // Start "ConEmuC.exe /DEBUGPID="
			//#endif
		{
			if (TerminalMode) break;  // низзя

			StartDebugger();
		} break;

		case menu_ConsoleInfo:
		{
			ShowConsoleInfo();
		} break;
	}
}

void CPluginBase::ShowTabsList()
{
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GETALLTABS, sizeof(CESERVER_REQ_HDR));
	CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
	if (pOut && (pOut->GetAllTabs.Count > 0))
	{
		INT_PTR nMenuRc = -1;

		int Count = pOut->GetAllTabs.Count;
		int AllCount = Count + pOut->GetAllTabs.Tabs[Count-1].ConsoleIdx;
		ConEmuPluginMenuItem* pItems = (ConEmuPluginMenuItem*)calloc(AllCount,sizeof(*pItems));
		if (pItems)
		{
			int nLastConsole = 0;
			for (int i = 0, k = 0; i < Count; i++, k++)
			{
				if (nLastConsole != pOut->GetAllTabs.Tabs[i].ConsoleIdx)
				{
					pItems[k++].Separator = true;
					nLastConsole = pOut->GetAllTabs.Tabs[i].ConsoleIdx;
				}
				_ASSERTE(k < AllCount);
				pItems[k].Selected = (pOut->GetAllTabs.Tabs[i].ActiveConsole && pOut->GetAllTabs.Tabs[i].ActiveTab);
				pItems[k].Checked = pOut->GetAllTabs.Tabs[i].ActiveTab;
				pItems[k].Disabled = pOut->GetAllTabs.Tabs[i].Disabled;
				pItems[k].MsgText = pOut->GetAllTabs.Tabs[i].Title;
				pItems[k].UserData = i;
			}

			nMenuRc = ShowPluginMenu(pItems, AllCount, CEMenuTitleTabs);

			if ((nMenuRc >= 0) && (nMenuRc < AllCount))
			{
				nMenuRc = pItems[nMenuRc].UserData;

				if (pOut->GetAllTabs.Tabs[nMenuRc].ActiveConsole && !pOut->GetAllTabs.Tabs[nMenuRc].ActiveTab)
				{
					DWORD nTab = pOut->GetAllTabs.Tabs[nMenuRc].FarWindow;
					int nOpenFrom = -1;
					int nArea = Plugin()->GetMacroArea();
					if (nArea != -1)
					{
						if (nArea == ma_Shell || nArea == ma_Search || nArea == ma_InfoPanel || nArea == ma_QViewPanel || nArea == ma_TreePanel)
							nOpenFrom = of_FilePanel;
						else if (nArea == ma_Editor)
							nOpenFrom = of_Editor;
						else if (nArea == ma_Viewer)
							nOpenFrom = of_Viewer;
					}
					gnPluginOpenFrom = nOpenFrom;
					ProcessCommand(CMD_SETWINDOW, FALSE, &nTab, nullptr, true/*bForceSendTabs*/);
				}
				else if (!pOut->GetAllTabs.Tabs[nMenuRc].ActiveConsole || !pOut->GetAllTabs.Tabs[nMenuRc].ActiveTab)
				{
					CESERVER_REQ* pActIn = ExecuteNewCmd(CECMD_ACTIVATETAB, sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD));
					pActIn->dwData[0] = pOut->GetAllTabs.Tabs[nMenuRc].ConsoleIdx;
					pActIn->dwData[1] = pOut->GetAllTabs.Tabs[nMenuRc].TabNo;
					CESERVER_REQ* pActOut = ExecuteGuiCmd(FarHwnd, pActIn, FarHwnd);
					ExecuteFreeResult(pActOut);
					ExecuteFreeResult(pActIn);
				}
			}

			SafeFree(pItems);
		}
	}
	else
	{
		ShowMessage(CEGetAllTabsFailed, 0);
	}

	ExecuteFreeResult(pOut);
	ExecuteFreeResult(pIn);
}

void CPluginBase::ShowPanelsList()
{
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GETALLPANELS, sizeof(CESERVER_REQ_HDR));
	CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);

	if ((pOut->DataSize() > 0) && (pOut->Panels.iCount > 0))
	{
		INT_PTR nMenuRc = -1;

		int AllCount = pOut->Panels.iCount;
		ConEmuPluginMenuItem* pItems = (ConEmuPluginMenuItem*)calloc(AllCount,sizeof(*pItems));
		if (pItems)
		{
			LPCWSTR pszDir = pOut->Panels.szDirs;

			for (int i = 0; i < AllCount; i++)
			{
				pItems[i].Selected = (pOut->Panels.iCurrent == i);
				//pItems[i].Checked = false;
				//pItems[i].Disabled = false;
				pItems[i].MsgText = pszDir;
				pItems[i].UserData = (INT_PTR)pszDir;
				pszDir += lstrlen(pszDir)+1;
			}

			nMenuRc = ShowPluginMenu(pItems, AllCount, CEMenuTitlePanels);

			if ((nMenuRc >= 0) && (nMenuRc < AllCount))
			{
				pszDir = (LPCWSTR)pItems[nMenuRc].UserData;
				if (pszDir)
				{
					PrintText(pszDir);
				}
			}

			SafeFree(pItems);
		}
	}
	else
	{
		ShowMessage(CEGetAllPanelsFailed, 0);
	}

	ExecuteFreeResult(pOut);
	ExecuteFreeResult(pIn);
}

void CPluginBase::EditViewConsoleOutput(bool abView)
{
	// Открыть в редакторе вывод последней консольной программы
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GETOUTPUTFILE, sizeof(CESERVER_REQ_HDR)+sizeof(pIn->OutputFile.bUnicode));
	if (!pIn) return;

	pIn->OutputFile.bUnicode = (gFarVersion.dwVerMajor>=2);

	CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);

	if (pOut)
	{
		if (pOut->OutputFile.szFilePathName[0])
		{
			bool lbRc = OpenEditor(pOut->OutputFile.szFilePathName, abView, true);

			if (!lbRc)
			{
				DeleteFile(pOut->OutputFile.szFilePathName);
			}
		}

		ExecuteFreeResult(pOut);
	}

	ExecuteFreeResult(pIn);
}

void CPluginBase::SwitchTabCommand(PluginMenuCommands cmd)
{
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_TABSCMD, sizeof(CESERVER_REQ_HDR)+sizeof(pIn->Data));
	if (!pIn) return;

	// Data[0] <== enum ConEmuTabCommand
	switch (cmd)
	{
	case menu_SwitchTabVisible: // Показать/спрятать табы
		pIn->Data[0] = ctc_ShowHide; break;
	case menu_SwitchTabNext:
		pIn->Data[0] = ctc_SwitchNext; break;
	case menu_SwitchTabPrev:
		pIn->Data[0] = ctc_SwitchPrev; break;
	case menu_SwitchTabCommit:
		pIn->Data[0] = ctc_SwitchCommit; break;
	default:
		_ASSERTE(FALSE && "Unsupported command");
		pIn->Data[0] = ctc_ShowHide;
	}

	CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
	if (pOut) ExecuteFreeResult(pOut);
	ExecuteFreeResult(pIn);
}

int CPluginBase::ProcessSynchroEvent(int Event, void *Param)
{
	if (Event != se_CommonSynchro)
		return 0;

	if (gbInputSynchroPending)
		gbInputSynchroPending = false;

	// Некоторые плагины (NetBox) блокируют главный поток, и открывают
	// в своем потоке диалог. Это ThreadSafe. Некорректные открытия
	// отследить не удастся. Поэтому, считаем, если Far дернул наш
	// ProcessSynchroEventW, то это (временно) стала главная нить
	DWORD nPrevID = gnMainThreadId;
	gnMainThreadId = GetCurrentThreadId();

	#ifdef _DEBUG
	{
		static int nLastType = -1;
		int nCurType = GetActiveWindowType();

		if (nCurType != nLastType)
		{
			LPCWSTR pszCurType = GetWindowTypeName(nCurType);

			LPCWSTR pszLastType = GetWindowTypeName(nLastType);

			wchar_t szDbg[255];
			swprintf_c(szDbg, L"FarWindow: %s activated (was %s)\n", pszCurType, pszLastType);
			DEBUGSTR(szDbg);
			nLastType = nCurType;
		}
	}
	#endif

	if (!gbSynchroProhibited)
	{
		OnMainThreadActivated();
	}

	if (gnSynchroCount > 0)
		gnSynchroCount--;

	if (gbSynchroProhibited && (gnSynchroCount == 0))
	{
		Plugin()->StopWaitEndSynchro();
	}

	gnMainThreadId = nPrevID;

	return 0;
}

void CPluginBase::ProcessEditorInputInternal(const INPUT_RECORD& Rec)
{
	// only key events with virtual codes > 0 are likely to cause status change (?)
	if (((Rec.EventType & 0xFF) == KEY_EVENT) && Rec.Event.KeyEvent.bKeyDown
		&& (Rec.Event.KeyEvent.wVirtualKeyCode || Rec.Event.KeyEvent.wVirtualScanCode || Rec.Event.KeyEvent.uChar.UnicodeChar))
    {
		#ifdef SHOW_DEBUG_EVENTS
		char szDbg[255]; wsprintfA(szDbg, "ProcessEditorInput(E=%i, VK=%i, SC=%i, CH=%i, Down=%i)\n", Rec.EventType, Rec.Event.KeyEvent.wVirtualKeyCode, Rec.Event.KeyEvent.wVirtualScanCode, Rec.Event.KeyEvent.uChar.AsciiChar, Rec.Event.KeyEvent.bKeyDown);
		OutputDebugStringA(szDbg);
		#endif

		gbNeedPostEditCheck = TRUE;
	}
}

int CPluginBase::ProcessEditorViewerEvent(int EditorEvent, int ViewerEvent)
{
	if (EditorEvent == ee_Redraw)
	{
#if defined(_M_ARM64) || defined(__aarch64__)
		if (gpBgPlugin)
			gpBgPlugin->OnMainThreadActivated(EditorEvent, ViewerEvent);
#endif
		return 0;
	}

	if (!gbRequestUpdateTabs)
	{
		if ((EditorEvent != -1)
			&& (EditorEvent == ee_Read || EditorEvent == ee_Close || EditorEvent == ee_GotFocus || EditorEvent == ee_KillFocus || EditorEvent == ee_Save))
		{
			gbRequestUpdateTabs = TRUE;
			//} else if (Event == EE_REDRAW && gbHandleOneRedraw) {
			//	gbHandleOneRedraw = false; gbRequestUpdateTabs = TRUE;
		}
		else if ((ViewerEvent != -1)
			&& (ViewerEvent == ve_Close || ViewerEvent == ve_GotFocus || ViewerEvent == ve_KillFocus || ViewerEvent == ve_Read))
		{
			gbRequestUpdateTabs = TRUE;
		}
	}

	if (isModalEditorViewer())
	{
		if ((EditorEvent == ee_Close) || (ViewerEvent == ve_Close))
		{
			gbClosingModalViewerEditor = TRUE;
		}
	}

	if (gpBgPlugin && (EditorEvent != ee_Redraw))
	{
#if defined(_M_ARM64) || defined(__aarch64__)
		if (EditorEvent == ee_Close || ViewerEvent == ve_Close)
		{
			// Far still reports the closing window in this callback.
			gpBgPlugin->SetForceCheck();
			ExecuteSynchroApi();
			return 0;
		}
#endif
		gpBgPlugin->OnMainThreadActivated(EditorEvent, ViewerEvent);
	}

	return 0;
}

bool CPluginBase::isEditorViewer()
{
	if (!gpTabs)
		return false;
	return ((gnCurrentWindowType == wt_Editor) || (gnCurrentWindowType == wt_Viewer));
}

bool CPluginBase::isModalEditorViewer()
{
	if (!gpTabs || !gpTabs->Tabs.nTabCount)
		return false;

	// Если последнее открытое окно - модальное
	if (gpTabs->Tabs.tabs[gpTabs->Tabs.nTabCount-1].Modal)
		return true;

	// Было раньше такое условие, по идее это не правильно
	// if (gpTabs->Tabs.tabs[0].Type != wt_Panels)
	// return true;

	return false;
}

// Плагин может быть вызван в первый раз из фоновой нити.
// Поэтому простой "gnMainThreadId = GetCurrentThreadId();" не прокатит. Нужно искать первую нить процесса!
DWORD CPluginBase::GetMainThreadId()
{
	DWORD nThreadID = 0;
	DWORD nProcID = GetCurrentProcessId();
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 ti = {sizeof(THREADENTRY32)};

		if (Thread32First(h, &ti))
		{
			do
			{
				// Нужно найти ПЕРВУЮ нить процесса
				if (ti.th32OwnerProcessID == nProcID)
				{
					nThreadID = ti.th32ThreadID;
					break;
				}
			}
			while(Thread32Next(h, &ti));
		}

		CloseHandle(h);
	}

	// Нехорошо. Должна быть найдена. Вернем хоть что-то (текущую нить)
	if (!nThreadID)
	{
		_ASSERTE(nThreadID!=0);
		nThreadID = GetCurrentThreadId();
	}

	return nThreadID;
}

void CPluginBase::ShowConsoleInfo()
{
	DWORD nConIn = 0, nConOut = 0;
	HANDLE hConIn = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	GetConsoleMode(hConIn, &nConIn);
	GetConsoleMode(hConOut, &nConOut);

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	GetConsoleScreenBufferInfo(hConOut, &csbi);
	CONSOLE_CURSOR_INFO ci = {};
	GetConsoleCursorInfo(hConOut, &ci);

	wchar_t szInfo[1024];
	swprintf_c(szInfo,
		L"ConEmu Console information\n"
		L"TerminalMode=%s\n"
		L"Console HWND=0x%08X; "
		L"Virtual HWND=0x%08X\n"
		L"ServerPID=%u; CurrentPID=%u\n"
		L"ConInMode=0x%08X; ConOutMode=0x%08X\n"
		L"Buffer size=(%u,%u); Rect=(%u,%u)-(%u,%u)\n"
		L"CursorInfo=(%u,%u,%u%s); MaxWndSize=(%u,%u)\n"
		L"OutputAttr=0x%02X\n"
		,
		TerminalMode ? L"Yes" : L"No",
		LODWORD(FarHwnd), LODWORD(ghConEmuWndDC),
		gdwServerPID, GetCurrentProcessId(),
		nConIn, nConOut,
		csbi.dwSize.X, csbi.dwSize.Y,
		csbi.srWindow.Left, csbi.srWindow.Top, csbi.srWindow.Right, csbi.srWindow.Bottom,
		csbi.dwCursorPosition.X, csbi.dwCursorPosition.Y, ci.dwSize, ci.bVisible ? L"V" : L"H",
		csbi.dwMaximumWindowSize.X, csbi.dwMaximumWindowSize.Y,
		csbi.wAttributes,
		0
	);

	ShowMessage(szInfo, 0, false);
}

bool CPluginBase::RunExternalProgramW(wchar_t* pszCommand, wchar_t* pszCurDir, bool bSilent/*=false*/)
{
	bool lbRc = false;
	_ASSERTE(pszCommand && *pszCommand);

	if (bSilent)
	{
		CEnvStrings strs(GetEnvironmentStringsW());
		DWORD nCmdLen = lstrlen(pszCommand)+1;
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_NEWCMD, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_NEWCMD)+((nCmdLen+strs.cchLength_)*sizeof(wchar_t)));
		if (pIn)
		{
			pIn->NewCmd.hFromConWnd = FarHwnd;
			if (pszCurDir)
				lstrcpyn(pIn->NewCmd.szCurDir, pszCurDir, countof(pIn->NewCmd.szCurDir));

			pIn->NewCmd.SetCommand(pszCommand);
			pIn->NewCmd.SetEnvStrings(strs.strings_, strs.cchLength_);

			HWND hGuiRoot = GetConEmuHWND(ConEmuWndType::GuiMainWindow);
			CESERVER_REQ* pOut = ExecuteGuiCmd(hGuiRoot, pIn, FarHwnd);
			if (pOut)
			{
				if (pOut->hdr.cbSize > sizeof(pOut->hdr) && pOut->Data[0])
				{
					lbRc = true;
				}
				ExecuteFreeResult(pOut);
			}
			else
			{
				_ASSERTE(pOut!=nullptr);
			}
			ExecuteFreeResult(pIn);
		}
	}
	else
	{
		STARTUPINFO cif = {};
		cif.cb = sizeof(cif);
		PROCESS_INFORMATION pri = {};
		HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
		DWORD oldConsoleMode;
		DWORD nErr = 0;
		DWORD nExitCode = 0;
		GetConsoleMode(hStdin, &oldConsoleMode);
		SetConsoleMode(hStdin, ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT); // подбиралось методом тыка

		#ifdef _DEBUG
		if (!bSilent)
		{
			WARNING("Посмотреть, как Update в консоль выводит.");
			wprintf(L"\nCmd: <%s>\nDir: <%s>\n\n", pszCommand, pszCurDir);
		}
		#endif

		MWow64Disable wow; wow.Disable();
		SetLastError(0);
		BOOL lb = CreateProcess(/*strCmd, strArgs,*/ nullptr, pszCommand, nullptr, nullptr, TRUE,
		          NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE, nullptr, pszCurDir, &cif, &pri);
		nErr = GetLastError();
		wow.Restore();

		if (lb)
		{
			WaitForSingleObject(pri.hProcess, INFINITE);
			GetExitCodeProcess(pri.hProcess, &nExitCode);
			CloseHandle(pri.hProcess);
			CloseHandle(pri.hThread);

			#ifdef _DEBUG
			if (!bSilent)
				wprintf(L"\nConEmuC: Process was terminated, ExitCode=%i\n\n", nExitCode);
			#endif

			lbRc = true;
		}
		else
		{
			#ifdef _DEBUG
			if (!bSilent)
				wprintf(L"\nConEmuC: CreateProcess failed, ErrCode=0x%08X\n\n", nErr);
			#endif
		}

		//wprintf(L"Cmd: <%s>\nArg: <%s>\nDir: <%s>\n\n", strCmd, strArgs, pszCurDir);
		SetConsoleMode(hStdin, oldConsoleMode);
	}

	return lbRc;
}

bool CPluginBase::StartDebugger()
{
	if (IsDebuggerPresent())
	{
		ShowMessage(CEAlreadyDebuggerPresent,0); // "ConEmu plugin\nDebugger is already attached to current process"
		return false; // Уже
	}

	if (IsTerminalMode())
	{
		ShowMessage(CECantDebugInTerminal,0); // "ConEmu plugin\nDebugger is not available in terminal mode"
		return false; // Уже
	}

	//DWORD dwServerPID = 0;
	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times
	wchar_t  szExe[MAX_PATH*3] = {0};
	wchar_t  szConEmuC[MAX_PATH];
	bool lbRc = false;
	DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si; memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);
	DWORD dwSelfPID = GetCurrentProcessId();

	if ((nLen = GetEnvironmentVariableW(ENV_CONEMUBASEDIR_VAR_W, szConEmuC, MAX_PATH-16)) < 1)
	{
		ShowMessage(CECantDebugNotEnvVar,0); // "ConEmu plugin\nEnvironment variable 'ConEmuBaseDir' not defined\nDebugger is not available"
		return false; // Облом
	}

	lstrcatW(szConEmuC, L"\\ConEmuC.exe");

	if (!FileExists(szConEmuC))
	{
		wchar_t* pszSlash = nullptr;

		if (((nLen=GetModuleFileName(0, szConEmuC, MAX_PATH-24)) < 1) || ((pszSlash = wcsrchr(szConEmuC, L'\\')) == nullptr))
		{
			ShowMessage(CECantDebugNotEnvVar,0); // "ConEmu plugin\nEnvironment variable 'ConEmuBaseDir' not defined\nDebugger is not available"
			return false; // Облом
		}

		lstrcpyW(pszSlash, L"\\ConEmu\\ConEmuC.exe");

		if (!FileExists(szConEmuC))
		{
			lstrcpyW(pszSlash, L"\\ConEmuC.exe");

			if (!FileExists(szConEmuC))
			{
				ShowMessage(CECantDebugNotEnvVar,0); // "ConEmu plugin\nEnvironment variable 'ConEmuBaseDir' not defined\nDebugger is not available"
				return false; // Облом
			}
		}
	}

	int w = 80, h = 25;
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
	{
		w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
		h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
	}

	if (ghConEmuWndDC)
	{
		DWORD nGuiPID = 0; GetWindowThreadProcessId(ghConEmuWndDC, &nGuiPID);
		// Откроем дебаггер в новой вкладке ConEmu. При желании юзеру проще сделать Detach
		// "/DEBUGPID=" обязательно должен быть первым аргументом

		swprintf_c(szExe, L"\"%s\" /ATTACH /ROOT \"%s\" /DEBUGPID=%i /BW=%i /BH=%i /BZ=%i",
		          szConEmuC, szConEmuC, dwSelfPID, w, h, LONGOUTPUTHEIGHT_MAX);
		//swprintf_c(szExe, L"\"%s\" /ATTACH /GID=%u /GHWND=%08X /ROOT \"%s\" /DEBUGPID=%i /BW=%i /BH=%i /BZ=%i",
		//          szConEmuC, nGuiPID, (DWORD)(DWORD_PTR)ghConEmuWndDC, szConEmuC, dwSelfPID, w, h, LONGOUTPUTHEIGHT_MAX);
	}
	else
	{
		// Запустить дебаггер в новом видимом консольном окне
		swprintf_c(szExe, L"\"%s\" /DEBUGPID=%i /BW=%i /BH=%i /BZ=%i",
		          szConEmuC, dwSelfPID, w, h, LONGOUTPUTHEIGHT_MAX);
	}

	if (ghConEmuWndDC)
	{
		si.dwFlags |= STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE;
	}

	if (!CreateProcess(nullptr, szExe, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE, nullptr,
	                  nullptr, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		#ifdef _DEBUG
		DWORD dwErr = GetLastError();
		#endif
		ShowMessage(CECantStartDebugger,0); // "ConEmu plugin\nНе удалось запустить процесс отладчика"
	}
	else
	{
		lbRc = true;
	}

	return lbRc;
}

bool CPluginBase::Attach2Gui(bool bLeaveOpened /*= false*/)
{
	bool lbRc = false;
	DWORD dwServerPID = 0;
	BOOL lbFound = FALSE;
	WCHAR  szCmdLine[MAX_PATH+0x100] = {0};
	wchar_t szConEmuBase[MAX_PATH+1], szConEmuGui[MAX_PATH+1];
	//DWORD nLen = 0;
	PROCESS_INFORMATION pi; memset(&pi, 0, sizeof(pi));
	STARTUPINFO si = {};
	si.cb = sizeof(si);
	const DWORD dwSelfPID = GetCurrentProcessId();
	wchar_t* pszSlash = nullptr;
	DWORD nStartWait = 255;

	if (GetConEmuHWND(ConEmuWndType::GuiDcWindow))
	{
		ShowMessageGui(CECantStartServer4, MB_ICONSTOP|MB_SYSTEMMODAL);
		goto wrap;
	}

	gbWasDetached = TRUE;

	if (!FindConEmuBaseDir(szConEmuBase, szConEmuGui, ghPluginModule))
	{
		ShowMessageGui(CECantStartServer2, MB_ICONSTOP|MB_SYSTEMMODAL);
		goto wrap;
	}

	// Нужно загрузить ConEmuHk.dll и выполнить инициализацию хуков. Учесть, что ConEmuHk.dll уже мог быть загружен
	if (!ghHooksModule)
	{
		wchar_t szHookLib[MAX_PATH+16];
		wcscpy_c(szHookLib, szConEmuBase);
		wcscat_c(szHookLib, L"\\" ConEmuHk_DLL_3264);
		ghHooksModule = LoadLibrary(szHookLib);
		if (ghHooksModule)
		{
			gbHooksModuleLoaded = TRUE;
			// После подцепляния к GUI нужно выполнить StartupHooks!
			gbStartupHooksAfterMap = TRUE;
		}
	}


	if (FindServerCmd(CECMD_ATTACH2GUI, dwServerPID, true) && dwServerPID != 0)
	{
		// "Server was already started. PID=%i. Exiting...\n", dwServerPID
		gdwServerPID = dwServerPID;
		_ASSERTE(gdwServerPID!=0);
		gbTryOpenMapHeader = (gpConMapInfo==nullptr);

		if (gpConMapInfo)  // 04.03.2010 Maks - Если мэппинг уже открыт - принудительно передернуть ресурсы и информацию
			CheckResources(TRUE);

		lbRc = true;
		goto wrap;
	}

	gdwServerPID = 0;
	//TODO("У сервера пока не получается менять шрифт в консоли, которую создал FAR");
	//SetConsoleFontSizeTo(GetConEmuHWND(ConEmuWndType::ConsoleWindow), 6, 4, L"Lucida Console");
	// Create process, with flag /Attach GetCurrentProcessId()
	// Sleep for sometimes, try InitHWND(hConWnd); several times

	szCmdLine[0] = L'"';
	wcscat_c(szCmdLine, szConEmuBase);
	wcscat_c(szCmdLine, L"\\");
	//if ((nLen = GetEnvironmentVariableW(ENV_CONEMUBASEDIR_VAR_W, szCmdLine+1, MAX_PATH)) > 0)
	//{
	//	if (szCmdLine[nLen] != L'\\') { szCmdLine[nLen+1] = L'\\'; szCmdLine[nLen+2] = 0; }
	//}
	//else
	//{
	//	if (!GetModuleFileName(0, szCmdLine+1, MAX_PATH) || !(pszSlash = wcsrchr(szCmdLine, L'\\')))
	//	{
	//		ShowMessageGui(CECantStartServer2, MB_ICONSTOP|MB_SYSTEMMODAL);
	//		goto wrap;
	//	}
	//	pszSlash[1] = 0;
	//}

	pszSlash = szCmdLine + lstrlenW(szCmdLine);
	//BOOL lbFound = FALSE;
	// Для фанатов 64-битных версий
#ifdef WIN64

	//if (!lbFound) -- точная папка уже найдена
	//{
	//	lstrcpyW(pszSlash, L"ConEmu\\ConEmuC64.exe");
	//	lbFound = FileExists(szCmdLine+1);
	//}

	if (!lbFound)
	{
		lstrcpyW(pszSlash, L"ConEmuC64.exe");
		lbFound = FileExists(szCmdLine+1);
	}

#endif

	//if (!lbFound) -- точная папка уже найдена
	//{
	//	lstrcpyW(pszSlash, L"ConEmu\\ConEmuC.exe");
	//	lbFound = FileExists(szCmdLine+1);
	//}

	if (!lbFound)
	{
		lstrcpyW(pszSlash, L"ConEmuC.exe");
		lbFound = FileExists(szCmdLine+1);
	}

	if (!lbFound)
	{
		ShowMessageGui(CECantStartServer3, MB_ICONSTOP|MB_SYSTEMMODAL);
		goto wrap;
	}

	//if (IsWindows64())
	//	wsprintf(szCmdLine+lstrlenW(szCmdLine), L"ConEmuC64.exe\" /ATTACH /PID=%i", dwSelfPID);
	//else
	wsprintf(szCmdLine+lstrlenW(szCmdLine), L"\" /ATTACH /FARPID=%i %s", dwSelfPID, bLeaveOpened ? L"/CONFIRM " : L"");
	if (gdwPreDetachGuiPID)
		wsprintf(szCmdLine+lstrlenW(szCmdLine), L"/GID=%i", gdwPreDetachGuiPID);

	if (!CreateProcess(nullptr, szCmdLine, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr,
	                  nullptr, &si, &pi))
	{
		// Хорошо бы ошибку показать?
		ShowMessageGui(CECantStartServer, MB_ICONSTOP|MB_SYSTEMMODAL); // "ConEmu plugin\nCan't start console server process (ConEmuC.exe)\nOK"
	}
	else
	{
		wchar_t szName[64];
		swprintf_c(szName, CESRVSTARTEDEVENT, pi.dwProcessId/*gnSelfPID*/);
		// Event мог быть создан и ранее (в Far-плагине, например)
		HANDLE hServerStartedEvent = CreateEvent(LocalSecurity(), TRUE, FALSE, szName);
		_ASSERTE(hServerStartedEvent!=nullptr);
		HANDLE hWait[] = {pi.hProcess, hServerStartedEvent};
		nStartWait = WaitForMultipleObjects(countof(hWait), hWait, FALSE, ATTACH_START_SERVER_TIMEOUT);

		if (nStartWait == 0)
		{
			// Server was terminated!
			ShowMessageGui(CECantStartServer, MB_ICONSTOP|MB_SYSTEMMODAL); // "ConEmu plugin\nCan't start console server process (ConEmuC.exe)\nOK"
		}
		else
		{
			// Server must be initialized ATM
			_ASSERTE(nStartWait == 1);

			// Recall initialization of ConEmuHk.dll
			if (ghHooksModule)
			{
				RequestLocalServer_t fRequestLocalServer = (RequestLocalServer_t)GetProcAddress(ghHooksModule, "RequestLocalServer");
				// Refresh ConEmu HWND's
				if (fRequestLocalServer)
				{
					RequestLocalServerParm Parm = {sizeof(Parm), slsf_ReinitWindows};
					//if (gFarVersion.dwVerMajor >= 3)
					//	Parm.Flags |=
					fRequestLocalServer(&Parm);
				}
			}

			gdwServerPID = pi.dwProcessId;
			_ASSERTE(gdwServerPID!=0);
			SafeCloseHandle(pi.hProcess);
			SafeCloseHandle(pi.hThread);
			lbRc = true;
			// Чтобы MonitorThread пытался открыть Mapping
			gbTryOpenMapHeader = (gpConMapInfo==nullptr);
		}
	}

wrap:
	return lbRc;
}

bool CPluginBase::FindServerCmd(DWORD nServerCmd, DWORD &dwServerPID, bool bFromAttach /*= false*/)
{
	if (!FarHwnd)
	{
		_ASSERTE(FarHwnd!=nullptr);
		return false;
	}

	bool lbRc = false;

	//111209 - пробуем через мэппинг, там ИД сервера уже должен быть
	CESERVER_CONSOLE_MAPPING_HDR SrvMapping = {};
	if (LoadSrvMapping(FarHwnd, SrvMapping))
	{
		CESERVER_REQ* pIn = ExecuteNewCmd(nServerCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
		pIn->dwData[0] = GetCurrentProcessId();
		CESERVER_REQ* pOut = ExecuteSrvCmd(SrvMapping.nServerPID, pIn, FarHwnd);

		if (pOut)
		{
			_ASSERTE(SrvMapping.nServerPID == pOut->dwData[0]);
			dwServerPID = SrvMapping.nServerPID;
			ExecuteFreeResult(pOut);
			lbRc = true;
		}
		else
		{
			_ASSERTE(pOut!=nullptr);
		}

		ExecuteFreeResult(pIn);

		// Если команда успешно выполнена - выходим
		if (lbRc)
			return true;
	}
	else
	{
		_ASSERTE(bFromAttach && "LoadSrvMapping(FarHwnd, SrvMapping) failed");
		return false;
	}
	return false;

#if 0
	BOOL lbRc = FALSE;
	DWORD nProcessCount = 0, nProcesses[100] = {0};
	dwServerPID = 0;
	typedef DWORD (WINAPI* FGetConsoleProcessList)(LPDWORD lpdwProcessList, DWORD dwProcessCount);
	FGetConsoleProcessList pfnGetConsoleProcessList = nullptr;
	HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");

	if (hKernel)
	{
		pfnGetConsoleProcessList = (FGetConsoleProcessList)GetProcAddress(hKernel, "GetConsoleProcessList");
	}

	BOOL lbWin2kMode = (pfnGetConsoleProcessList == nullptr);

	if (!lbWin2kMode)
	{
		if (pfnGetConsoleProcessList)
		{
			nProcessCount = pfnGetConsoleProcessList(nProcesses, countof(nProcesses));

			if (nProcessCount && nProcessCount > countof(nProcesses))
			{
				_ASSERTE(nProcessCount <= countof(nProcesses));
				nProcessCount = 0;
			}
		}
	}

	if (lbWin2kMode)
	{
		DWORD nSelfPID = GetCurrentProcessId();
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

		if (lbWin2kMode)
		{
			if (hSnap != INVALID_HANDLE_VALUE)
			{
				PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

				if (Process32First(hSnap, &prc))
				{
					do
					{
						if (prc.th32ProcessID == nSelfPID)
						{
							nProcesses[0] = prc.th32ParentProcessID;
							nProcesses[1] = nSelfPID;
							nProcessCount = 2;
							break;
						}
					}
					while(!dwServerPID && Process32Next(hSnap, &prc));
				}

				CloseHandle(hSnap);
			}
		}
	}

	if (nProcessCount >= 2)
	{
		//DWORD nParentPID = 0;
		DWORD nSelfPID = GetCurrentProcessId();
		HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

		if (hSnap != INVALID_HANDLE_VALUE)
		{
			PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

			if (Process32First(hSnap, &prc))
			{
				do
				{
					for(UINT i = 0; i < nProcessCount; i++)
					{
						if (prc.th32ProcessID != nSelfPID
						        && prc.th32ProcessID == nProcesses[i])
						{
							if (lstrcmpiW(prc.szExeFile, L"conemuc.exe")==0
							        /*|| lstrcmpiW(prc.szExeFile, L"conemuc64.exe")==0*/)
							{
								CESERVER_REQ* pIn = ExecuteNewCmd(nServerCmd, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
								pIn->dwData[0] = GetCurrentProcessId();
								CESERVER_REQ* pOut = ExecuteSrvCmd(prc.th32ProcessID, pIn, FarHwnd);

								if (pOut) dwServerPID = prc.th32ProcessID;

								ExecuteFreeResult(pIn); ExecuteFreeResult(pOut);

								// Если команда успешно выполнена - выходим
								if (dwServerPID)
								{
									lbRc = TRUE;
									break;
								}
							}
						}
					}
				}
				while(!dwServerPID && Process32Next(hSnap, &prc));
			}

			CloseHandle(hSnap);
		}
	}

	return lbRc;
#endif
}

int CPluginBase::ShowMessage(int aiMsg, int aiButtons)
{
	wchar_t szMsgText[512] = L"";
	GetMsg(aiMsg, szMsgText, countof(szMsgText));

	return ShowMessage(szMsgText, aiButtons, true);
}

// Если не вызывать - буфер увеличивается автоматически. Размер в БАЙТАХ
// Возвращает FALSE при ошибках выделения памяти
bool CPluginBase::OutDataAlloc(DWORD anSize)
{
	_ASSERTE(gpCmdRet==nullptr);
	// + размер заголовка gpCmdRet
	gpCmdRet = (CESERVER_REQ*)calloc(sizeof(CESERVER_REQ_HDR)+anSize,1);

	if (!gpCmdRet)
		return false;

	// Код команды пока не известен - установит вызывающая функция
	ExecutePrepareCmd(&gpCmdRet->hdr, 0, anSize+sizeof(CESERVER_REQ_HDR));
	gpData = gpCmdRet->Data;
	gnDataSize = anSize;
	gpCursor = gpData;
	return true;
}

// Размер в БАЙТАХ. вызывается автоматически из OutDataWrite
// Возвращает FALSE при ошибках выделения памяти
bool CPluginBase::OutDataRealloc(DWORD anNewSize)
{
	if (!gpCmdRet)
		return OutDataAlloc(anNewSize);

	if (anNewSize < gnDataSize)
		return false; // нельзя выделять меньше памяти, чем уже есть

	// realloc иногда не работает, так что даже и не пытаемся
	CESERVER_REQ* lpNewCmdRet = (CESERVER_REQ*)calloc(sizeof(CESERVER_REQ_HDR)+anNewSize,1);

	if (!lpNewCmdRet)
		return false;

	ExecutePrepareCmd(&lpNewCmdRet->hdr, gpCmdRet->hdr.nCmd, anNewSize+sizeof(CESERVER_REQ_HDR));
	LPBYTE lpNewData = lpNewCmdRet->Data;

	if (!lpNewData)
		return false;

	// скопировать существующие данные
	memcpy(lpNewData, gpData, gnDataSize);
	// запомнить новую позицию курсора
	gpCursor = lpNewData + (gpCursor - gpData);
	// И новый буфер с размером
	free(gpCmdRet);
	gpCmdRet = lpNewCmdRet;
	gpData = lpNewData;
	gnDataSize = anNewSize;
	return true;
}

// Размер в БАЙТАХ
// Возвращает FALSE при ошибках выделения памяти
bool CPluginBase::OutDataWrite(LPVOID apData, DWORD anSize)
{
	if (!gpData)
	{
		if (!OutDataAlloc(std::max<DWORD>(1024, (anSize+128))))
			return false;
	}
	else if (((gpCursor-gpData)+anSize) > gnDataSize)
	{
		if (!OutDataRealloc(gnDataSize + std::max<DWORD>(1024, (anSize+128))))
			return false;
	}

	// Скопировать данные
	memcpy(gpCursor, apData, anSize);
	gpCursor += anSize;
	return true;
}

bool CPluginBase::CreateTabs(int windowCount)
{
	if (gpTabs && maxTabCount > (windowCount + 1))
	{
		// пересоздавать не нужно, секцию не трогаем. только запомним последнее кол-во окон
		lastWindowCount = windowCount;
		return true;
	}

	//Enter CriticalSection(csTabs);

	if ((gpTabs==nullptr) || (maxTabCount <= (windowCount + 1)))
	{
		MSectionLock SC; SC.Lock(csTabs, TRUE);
		maxTabCount = windowCount + 20; // с запасом

		if (gpTabs)
		{
			free(gpTabs); gpTabs = nullptr;
		}

		gpTabs = (CESERVER_REQ*) calloc(sizeof(CESERVER_REQ_HDR) + maxTabCount*sizeof(ConEmuTab), 1);
	}

	lastWindowCount = windowCount;

	return (gpTabs != nullptr);
}

bool CPluginBase::AddTab(int &tabCount, int WindowPos, bool losingFocus, bool editorSave,
			int Type, LPCWSTR Name, LPCWSTR FileName,
			int Current, int Modified, int Modal,
			int EditViewId)
{
	bool lbCh = false;
	DEBUGSTR(L"--AddTab\n");

	if (Type == wt_Panels)
	{
		lbCh = (gpTabs->Tabs.tabs[0].Current != (Current/*losingFocus*/ ? 1 : 0)) ||
		       (gpTabs->Tabs.tabs[0].Type != wt_Panels);
		gpTabs->Tabs.tabs[0].Current = Current/*losingFocus*/ ? 1 : 0;
		//lstrcpyn(gpTabs->Tabs.tabs[0].Name, FUNC_Y(GetMsgW)(0), CONEMUTABMAX-1);
		gpTabs->Tabs.tabs[0].Name[0] = 0;
		gpTabs->Tabs.tabs[0].Pos = (WindowPos >= 0) ? WindowPos : 0;
		gpTabs->Tabs.tabs[0].Type = wt_Panels;
		gpTabs->Tabs.tabs[0].Modified = 0; // Иначе GUI может ошибочно считать, что есть несохраненные редакторы
		gpTabs->Tabs.tabs[0].EditViewId = 0;
		gpTabs->Tabs.tabs[0].Modal = 0;

		if (!tabCount)
			tabCount++;

		if (Current)
		{
			gpTabs->Tabs.CurrentType = gnCurrentWindowType = Type;
			gpTabs->Tabs.CurrentIndex = 0;
		}
	}
	else if (Type == wt_Editor || Type == wt_Viewer)
	{
		// Первое окно - должно быть панели. Если нет - значит фар открыт в режиме редактора
		if (tabCount == 1)
		{
			// 04.06.2009 Maks - Не, чего-то не то... при открытии редактора из панелей - он заменяет панели
			//gpTabs->Tabs.tabs[0].Type = Type;
		}

		// when receiving saving event receiver is still reported as modified
		if (editorSave && lstrcmpi(FileName, Name) == 0)
			Modified = 0;


		// Облагородить заголовок таба с Ctrl-O
		wchar_t szConOut[MAX_PATH];
		LPCWSTR pszName = PointToName(Name);
		if (pszName && (wmemcmp(pszName, L"CEM", 3) == 0))
		{
			LPCWSTR pszExt = PointToExt(pszName);
			if (lstrcmpi(pszExt, L".tmp") == 0)
			{
				GetMsg(CEConsoleOutput, szConOut, countof(szConOut));

				Name = szConOut;
			}
		}


		lbCh = (gpTabs->Tabs.tabs[tabCount].Current != (Current/*losingFocus*/ ? 1 : 0)/*(losingFocus ? 0 : Current)*/)
		    || (gpTabs->Tabs.tabs[tabCount].Type != Type)
		    || (gpTabs->Tabs.tabs[tabCount].Modified != Modified)
			|| (gpTabs->Tabs.tabs[tabCount].Modal != Modal)
		    || (lstrcmp(gpTabs->Tabs.tabs[tabCount].Name, Name) != 0);
		// when receiving losing focus event receiver is still reported as current
		gpTabs->Tabs.tabs[tabCount].Type = Type;
		gpTabs->Tabs.tabs[tabCount].Current = (Current/*losingFocus*/ ? 1 : 0)/*losingFocus ? 0 : Current*/;
		gpTabs->Tabs.tabs[tabCount].Modified = Modified;
		gpTabs->Tabs.tabs[tabCount].Modal = Modal;
		gpTabs->Tabs.tabs[tabCount].EditViewId = EditViewId;

		if (gpTabs->Tabs.tabs[tabCount].Current != 0)
		{
			lastModifiedStateW = Modified != 0 ? 1 : 0;
			gpTabs->Tabs.CurrentType = gnCurrentWindowType = Type;
			gpTabs->Tabs.CurrentIndex = tabCount;
		}

		//else
		//{
		//	lastModifiedStateW = -1; //2009-08-17 при наличии более одного редактора - сносит крышу
		//}
		int nLen = std::min(lstrlen(Name),(CONEMUTABMAX-1));
		lstrcpyn(gpTabs->Tabs.tabs[tabCount].Name, Name, nLen+1);
		gpTabs->Tabs.tabs[tabCount].Name[nLen]=0;
		gpTabs->Tabs.tabs[tabCount].Pos = (WindowPos >= 0) ? WindowPos : tabCount;
		tabCount++;
	}

	return lbCh;
}

void CPluginBase::AddTabFinish(int tabCount)
{
	gpTabs->Tabs.nTabCount = tabCount;
}

void CPluginBase::SendTabs(int tabCount, bool abForceSend/*=false*/)
{
	if (gb_ProtocolWarned)
	{
		DEBUGSTR(L"!!! CPluginBase::SendTabs skipped due to gb_ProtocolWarned !!!\n");
		return;
	}

	MSectionLock SC; SC.Lock(csTabs);

	if (!gpTabs)
	{
		_ASSERTE(gpTabs!=nullptr);
		return;
	}

	gnCurTabCount = tabCount; // сразу запомним!, А то при ретриве табов количество еще старым будет...
	gpTabs->Tabs.nTabCount = tabCount;
	gpTabs->hdr.cbSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB)
	                     + sizeof(ConEmuTab) * ((tabCount > 1) ? (tabCount - 1) : 0);
	// Обновляем структуру сразу, чтобы она была готова к отправке в любой момент
	ExecutePrepareCmd(&gpTabs->hdr, CECMD_TABSCHANGED, gpTabs->hdr.cbSize);

	// Это нужно делать только если инициировано ФАРОМ. Если запрос прислал ConEmu - не посылать...
	if (tabCount && ghConEmuWndDC && IsWindow(ghConEmuWndDC) && abForceSend)
	{
		gpTabs->Tabs.bMacroActive = Plugin()->IsMacroActive();
		gpTabs->Tabs.bMainThread = (GetCurrentThreadId() == gnMainThreadId);

		// Если выполняется макрос и отложенная отсылка (по окончанию) уже запрошена
		if (gpTabs->Tabs.bMacroActive && gbNeedPostTabSend)
		{
			gnNeedPostTabSendTick = GetTickCount(); // Обновить тик
			return;
		}

		gbNeedPostTabSend = FALSE;
		CESERVER_REQ* pOut =
		    ExecuteGuiCmd(FarHwnd, gpTabs, FarHwnd);

		if (pOut)
		{
			if (pOut->hdr.cbSize >= (sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_CONEMUTAB_RET)))
			{
				if (gpTabs->Tabs.bMacroActive && pOut->TabsRet.bNeedPostTabSend)
				{
					// Отослать после того, как макрос завершится
					gbNeedPostTabSend = TRUE;
					gnNeedPostTabSendTick = GetTickCount();
				}
				else if (pOut->TabsRet.bNeedResize)
				{
					// Если это отложенная отсылка табов после выполнения макросов
					if (GetCurrentThreadId() == gnMainThreadId)
					{
						FarSetConsoleSize(pOut->TabsRet.crNewSize.X, pOut->TabsRet.crNewSize.Y);
					}
				}
			}

			ExecuteFreeResult(pOut);
		}
	}

	SC.Unlock();
}

void CPluginBase::CloseTabs()
{
	if (!gb_ProtocolWarned
		&& ghConEmuWndDC && IsWindow(ghConEmuWndDC) && FarHwnd)
	{
		CESERVER_REQ in; // Пустая команда - значит FAR закрывается
		ExecutePrepareCmd(&in, CECMD_TABSCHANGED, sizeof(CESERVER_REQ_HDR));
		CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, &in, FarHwnd);

		if (pOut) ExecuteFreeResult(pOut);
	}
}

bool CPluginBase::UpdateConEmuTabs(bool abSendChanges)
{
	bool lbCh = false;
	// Блокируем сразу, т.к. ниже по коду gpTabs тоже используется
	MSectionLock SC; SC.Lock(csTabs);
	// На случай, если текущее окно заблокировано диалогом - не получится точно узнать
	// какое окно фара активно. Поэтому вернем последнее известное.
	int nLastCurrentTab = -1, nLastCurrentType = -1;

	if (gpTabs && gpTabs->Tabs.nTabCount > 0)
	{
		nLastCurrentTab = gpTabs->Tabs.CurrentIndex;
		nLastCurrentType = gpTabs->Tabs.CurrentType;
	}

	if (gpTabs)
	{
		gpTabs->Tabs.CurrentIndex = -1; // для строгости
	}

	if (!gbIgnoreUpdateTabs)
	{
		if (gbRequestUpdateTabs)
			gbRequestUpdateTabs = FALSE;

		if (ghConEmuWndDC && FarHwnd)
			CheckResources(FALSE);

		bool lbDummy = false;
		int windowCount = GetWindowCount();

		if ((windowCount == 0) && !gpFarInfo->bFarPanelAllowed)
		{
			windowCount = 1; lbDummy = true;
		}

		// lastWindowCount обновляется в CreateTabs
		lbCh = (lastWindowCount != windowCount);

		if (CreateTabs(windowCount))
		{
			if (lbDummy)
			{
				int tabCount = 0;
				lbCh |= AddTab(tabCount, 0, false, false, wt_Panels, nullptr, nullptr, 1, 0, 0, 0);
				gpTabs->Tabs.nTabCount = tabCount;
			}
			else
			{
				lbCh |= UpdateConEmuTabsApi(windowCount);
			}
		}
	}

	if (gpTabs)
	{
		if (gpTabs->Tabs.CurrentIndex == -1 && nLastCurrentTab != -1 && gpTabs->Tabs.nTabCount > 0)
		{
			// Активное окно определить не удалось
			if ((UINT)nLastCurrentTab >= gpTabs->Tabs.nTabCount)
				nLastCurrentTab = (gpTabs->Tabs.nTabCount - 1);

			gpTabs->Tabs.CurrentIndex = nLastCurrentTab;
			gpTabs->Tabs.tabs[nLastCurrentTab].Current = TRUE;
			gpTabs->Tabs.CurrentType = gpTabs->Tabs.tabs[nLastCurrentTab].Type;
		}

		if (gpTabs->Tabs.CurrentType == 0)
		{
			if (gpTabs->Tabs.CurrentIndex >= 0 && gpTabs->Tabs.CurrentIndex < (int)gpTabs->Tabs.nTabCount)
				gpTabs->Tabs.CurrentType = gpTabs->Tabs.tabs[nLastCurrentTab].Type;
			else
				gpTabs->Tabs.CurrentType = wt_Panels;
		}

		gnCurrentWindowType = gpTabs->Tabs.CurrentType;

		if (abSendChanges || gbForceSendTabs)
		{
			bool lbForceSend = (gbForceSendTabs != FALSE) || (lbCh && (gnReqCommand==(DWORD)-1));
			_ASSERTE((gbForceSendTabs==FALSE || IsDebuggerPresent()) && "Async SetWindow was timeouted?");
			gbForceSendTabs = FALSE;
			SendTabs(gpTabs->Tabs.nTabCount, lbForceSend);
		}
	}

	if (lbCh && gpBgPlugin)
	{
		gpBgPlugin->SetForceUpdate();
		gpBgPlugin->OnMainThreadActivated();
		gbNeedBgActivate = FALSE;
	}

	return lbCh;
}

void CPluginBase::ShowProtocolWarning()
{
	if (gb_ProtocolWarned)
		return;

	gb_ProtocolWarned = true;

	wchar_t szExePath[MAX_PATH + 1] = L"\n";
	GetModuleFileName(nullptr, szExePath + 1, countof(szExePath) - 1);
	wchar_t szSelfPath[MAX_PATH + 1] = L"\n";
	GetModuleFileName(ghPluginModule, szSelfPath + 1, countof(szSelfPath) - 1);

	wchar_t szProtocolWarning[255];
	msprintf(szProtocolWarning, countof(szProtocolWarning),
		L"Wrong plugin version was loaded in ConEmu/Far tab!\n"
		L"Protocol versions: Plugin=%u, ConEmu=%u\n"
		L"Please, either install proper version,\n"
		L"or change Far.exe arguments (/p switch)\n\n",
		CESERVER_REQ_VER, gpConMapInfo->nProtocolVersion);

	LPCWSTR pszConEmuExe = ((gpConMapInfo->cbSize >= (size_t)((LPBYTE)&gpConMapInfo->hConEmuRoot - (LPBYTE)gpConMapInfo))
		&& IsFilePath(gpConMapInfo->sConEmuExe)) ? gpConMapInfo->sConEmuExe : nullptr;
	const CEStr lsMsg(
		szProtocolWarning,
		pszConEmuExe,
		szExePath,
		szSelfPath);
	ShowMessageBox(lsMsg, MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_SYSTEMMODAL);
}

// Вызывается при инициализации из SetStartupInfo[W] и при обновлении табов UpdateConEmuTabs[???]
// То есть по идее, это происходит только когда фар явно вызывает плагин (legal api calls)
void CPluginBase::CheckResources(bool abFromStartup)
{
	if (GetCurrentThreadId() != gnMainThreadId)
	{
		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		return;
	}

	if (gsFarLang[0] && !abFromStartup)
	{
		static DWORD dwLastTickCount = GetTickCount();
		DWORD dwCurTick = GetTickCount();

		if ((dwCurTick - dwLastTickCount) < CHECK_RESOURCES_INTERVAL)
			return;

		dwLastTickCount = dwCurTick;
	}

	//if (abFromStartup) {
	//	_ASSERTE(gpConMapInfo!=nullptr);
	//	if (!gpFarInfo)
	//		gpFarInfo = (CEFAR_INFO_MAPPING*)calloc(sizeof(CEFAR_INFO_MAPPING),1);
	//}
	//if (gpConMapInfo)
	// Теперь он отвязан от gpConMapInfo
	ReloadFarInfo(true);

	// CECMD_FARLOADED has no sense outside of ConEmu,
	// or if ConEmu PROTOCOL version differs (CESERVER_REQ_VER)
	if (!gpConMapInfo)
	{
		return;
	}
	if ((gpConMapInfo->cbSize < sizeof(*gpConMapInfo))
		|| (gpConMapInfo->nProtocolVersion != CESERVER_REQ_VER))
	{
		if (!gb_ProtocolWarned)
		{
			ShowProtocolWarning();
		}
		return;
	}

	// We may be reattached into proper ConEmu window...
	if (gb_ProtocolWarned)
		gb_ProtocolWarned = false;

	wchar_t szLang[64];
	GetEnvironmentVariable(L"FARLANG", szLang, 63);

	if (abFromStartup || lstrcmpW(szLang, gsFarLang) || !gdwServerPID)
	{
		wchar_t szTitle[1024] = {0};
		GetConsoleTitleW(szTitle, 1024);
		SetConsoleTitleW(L"ConEmuC: CheckResources started");
		InitResources();
		DWORD dwServerPID = 0;
		FindServerCmd(CECMD_FARLOADED, dwServerPID);
		_ASSERTE(dwServerPID!=0);
		gdwServerPID = dwServerPID;
		SetConsoleTitleW(szTitle);
	}
	_ASSERTE(gdwServerPID!=0);
}

// Передать в ConEmu строки с ресурсами
void CPluginBase::InitResources()
{
	// В ConEmu нужно передать следущие ресурсы
	struct {
		int MsgId; wchar_t* pszRc; size_t cchMax; LPCWSTR pszDef;
	} OurStr[] = {
		{CELngEdit, gpFarInfo->sLngEdit, countof(gpFarInfo->sLngEdit), L"edit"},
		{CELngView, gpFarInfo->sLngView, countof(gpFarInfo->sLngView), L"view"},
		{CELngTemp, gpFarInfo->sLngTemp, countof(gpFarInfo->sLngTemp), L"{Temporary panel"},
	};

	if (GetCurrentThreadId() == gnMainThreadId)
	{
		for (size_t i = 0; i < countof(OurStr); i++)
		{
			GetMsg(OurStr[i].MsgId, OurStr[i].pszRc, OurStr[i].cchMax);
		}
	}

	if (!ghConEmuWndDC || !FarHwnd)
		return;

	int iAllLen = 0;
	for (size_t i = 0; i < countof(OurStr); i++)
	{
		if (!*OurStr[i].pszRc)
			lstrcpyn(OurStr[i].pszRc, OurStr[i].pszDef, OurStr[i].cchMax);
		iAllLen += lstrlen(OurStr[i].pszRc)+1;
	}

	int nSize = sizeof(CESERVER_REQ) + sizeof(DWORD) + iAllLen*sizeof(OurStr[0].pszRc[0]) + 2;
	CESERVER_REQ *pIn = (CESERVER_REQ*)calloc(nSize,1);

	if (pIn)
	{
		ExecutePrepareCmd(&pIn->hdr, CECMD_RESOURCES, nSize);
		pIn->dwData[0] = GetCurrentProcessId();
		wchar_t* pszRes = (wchar_t*)&(pIn->dwData[1]);

		for (size_t i = 0; i < countof(OurStr); i++)
		{
			lstrcpyW(pszRes, OurStr[i].pszRc); pszRes += lstrlenW(pszRes)+1;
		}

		// Поправить nSize (он должен быть меньше)
		_ASSERTE(pIn->hdr.cbSize >= (DWORD)(((LPBYTE)pszRes) - ((LPBYTE)pIn)));
		pIn->hdr.cbSize = (DWORD)(((LPBYTE)pszRes) - ((LPBYTE)pIn));
		CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);

		if (pOut)
		{
			if (pOut->DataSize() >= sizeof(FAR_REQ_FARSETCHANGED))
			{
				cmd_FarSetChanged(&pOut->FarSetChanged);
			}
			else
			{
				_ASSERTE(FALSE && "CECMD_RESOURCES failed (DataSize)");
			}
			ExecuteFreeResult(pOut);
		}
		else
		{
			_ASSERTE(pOut!=nullptr && "CECMD_RESOURCES failed");
		}

		free(pIn);
		GetEnvironmentVariable(L"FARLANG", gsFarLang, 63);
	}
}

void CPluginBase::CloseMapHeader()
{
	if (gpConMap)
		gpConMap->CloseMap();

	// delete для gpConMap здесь не делаем, может использоваться в других нитях!
	gpConMapInfo = nullptr;
}

int CPluginBase::OpenMapHeader()
{
	int iRc = -1;

	CloseMapHeader();

	if (FarHwnd)
	{
		if (!gpConMap)
			gpConMap = new MFileMapping<CESERVER_CONSOLE_MAPPING_HDR>;

		gpConMap->InitName(CECONMAPNAME, LODWORD(FarHwnd));

		if (gpConMap->Open())
		{
			gpConMapInfo = gpConMap->Ptr();

			if (gpConMapInfo)
			{
				if (gpConMapInfo->hConEmuWndDc)
				{
					SetConEmuEnvVar(gpConMapInfo->hConEmuRoot);
					SetConEmuEnvVarChild(gpConMapInfo->hConEmuWndDc, gpConMapInfo->hConEmuWndBack);
				}
				//if (gpConMapInfo->nLogLevel)
				//	InstallTrapHandler();
				iRc = 0;
			}
		}
		else
		{
			gpConMapInfo = nullptr;
		}

		//swprintf_c(szMapName, CECONMAPNAME, (DWORD)FarHwnd);
		//ghFileMapping = OpenFileMapping(FILE_MAP_READ, FALSE, szMapName);
		//if (ghFileMapping)
		//{
		//	gpConMapInfo = (const CESERVER_CONSOLE_MAPPING_HDR*)MapViewOfFile(ghFileMapping, FILE_MAP_READ,0,0,0);
		//	if (gpConMapInfo)
		//	{
		//		//ReloadFarInfo(); -- смысла нет. SetStartupInfo еще не вызывался
		//		iRc = 0;
		//	}
		//	else
		//	{
		//		#ifdef _DEBUG
		//		dwErr = GetLastError();
		//		#endif
		//		CloseHandle(ghFileMapping);
		//		ghFileMapping = nullptr;
		//	}
		//}
		//else
		//{
		//	#ifdef _DEBUG
		//	dwErr = GetLastError();
		//	#endif
		//}
	}

	return iRc;
}

void CPluginBase::InitRootRegKey()
{
	_ASSERTE(gFarVersion.dwVerMajor==1 || gFarVersion.dwVerMajor==2);
	// начальная инициализация. в SetStartupInfo поправим
	LPCWSTR pszFarName = (gFarVersion.dwVerMajor==3) ? L"Far Manager" :
		(gFarVersion.dwVerMajor==2) ? L"FAR2"
		: L"FAR";

	// Нужно учесть, что FAR мог запуститься с ключом /u (выбор конфигурации)
	wchar_t szFarUser[MAX_PATH];
	if (GetEnvironmentVariable(L"FARUSER", szFarUser, countof(szFarUser)) == 0)
		szFarUser[0] = 0;

	SafeFree(ms_RootRegKey);
	if (szFarUser[0])
		ms_RootRegKey = CEStr(L"Software\\", pszFarName, L"\\Users\\", szFarUser).Detach();
	else
		ms_RootRegKey = CEStr(L"Software\\", pszFarName).Detach();
}

void CPluginBase::SetRootRegKey(wchar_t* asKeyPtr)
{
	SafeFree(ms_RootRegKey);
	ms_RootRegKey = asKeyPtr;

	int nLen = ms_RootRegKey ? lstrlen(ms_RootRegKey) : 0;
	// Тут к нам приходит путь к настройкам НАШЕГО плагина
	// А на нужно получить "общий" ключ (для последующего считывания LoadPanelTabsFromRegistry
	if (nLen > 0)
	{
		if (ms_RootRegKey[nLen-1] == L'\\')
			ms_RootRegKey[nLen-1] = 0;
		wchar_t* pszName = wcsrchr(ms_RootRegKey, L'\\');
		if (pszName)
			*pszName = 0;
		else
			SafeFree(ms_RootRegKey);
	}
}

void CPluginBase::LoadPanelTabsFromRegistry()
{
	if (!ms_RootRegKey || !*ms_RootRegKey)
		return;

	CEStr pszTabsKey(ms_RootRegKey, L"\\Plugins\\PanelTabs");
	if (!pszTabsKey)
		return;

	HKEY hk;
	if (0 == RegOpenKeyExW(HKEY_CURRENT_USER, pszTabsKey, 0, KEY_READ, &hk))
	{
		DWORD dwVal, dwSize;

		if (!RegQueryValueExW(hk, L"SeparateTabs", nullptr, nullptr, (LPBYTE)&dwVal, &(dwSize = sizeof(dwVal))))
			gpFarInfo->PanelTabs.SeparateTabs = dwVal ? 1 : 0;

		if (!RegQueryValueExW(hk, L"ButtonColor", nullptr, nullptr, (LPBYTE)&dwVal, &(dwSize = sizeof(dwVal))))
			gpFarInfo->PanelTabs.ButtonColor = dwVal & 0xFF;

		RegCloseKey(hk);
	}
}

void CPluginBase::InitHWND()
{
	gsFarLang[0] = 0;

	if (!gFarVersion.dwVerMajor)
	{
		LoadFarVersion();  // пригодится уже здесь!

		if (gFarVersion.dwVerMajor == 3)
		{
			SetupExportsFar3();
		}
	}


	// Returns HWND of ...
	//  aiType==0: Gui console DC window
	//        ==1: Gui Main window
	//        ==2: Console window
	FarHwnd = GetConEmuHWND(ConEmuWndType::ConsoleWindow);
	ghConEmuWndDC = GetConEmuHWND(ConEmuWndType::GuiDcWindow);
	gbWasDetached = (ghConEmuWndDC == nullptr);
	gbFarWndVisible = IsWindowVisible(FarHwnd);


	{
		// TrueColor buffer check
		wchar_t szMapName[64];
		swprintf_c(szMapName, L"Console2_consoleBuffer_%d", (DWORD)GetCurrentProcessId());
		HANDLE hConsole2 = OpenFileMapping(FILE_MAP_READ, FALSE, szMapName);
		gbStartedUnderConsole2 = (hConsole2 != nullptr);

		if (hConsole2)
			CloseHandle(hConsole2);
	}

	// CtrlShiftF3 - для MMView & PicView
	if (!ghConEmuCtrlPressed)
	{
		wchar_t szName[64];
		swprintf_c(szName, CEKEYEVENT_CTRL, gnSelfPID);
		ghConEmuCtrlPressed = CreateEvent(nullptr, TRUE, FALSE, szName);
		if (ghConEmuCtrlPressed) ResetEvent(ghConEmuCtrlPressed); else { _ASSERTE(ghConEmuCtrlPressed); }

		swprintf_c(szName, CEKEYEVENT_SHIFT, gnSelfPID);
		ghConEmuShiftPressed = CreateEvent(nullptr, TRUE, FALSE, szName);
		if (ghConEmuShiftPressed) ResetEvent(ghConEmuShiftPressed); else { _ASSERTE(ghConEmuShiftPressed); }
	}

	OpenMapHeader();
	// Проверить, созданы ли буферы для True-Colorer
	// Это для того, чтобы пересоздать их при детаче
	//CheckColorerHeader();
	//memset(hEventCmd, 0, sizeof(HANDLE)*MAXCMDCOUNT);
	//int nChk = 0;
	//ghConEmuWndDC = GetConEmuHWND(FALSE/*abRoot*/  /*, &nChk*/);
	gnMsgTabChanged = RegisterWindowMessage(CONEMUTABCHANGED);

	if (!ghSetWndSendTabsEvent) ghSetWndSendTabsEvent = CreateEvent(0,0,0,0);

	// Даже если мы не в ConEmu - все равно запустить нить, т.к. в ConEmu теперь есть возможность /Attach!
	//WCHAR szEventName[128];
	DWORD dwCurProcId = GetCurrentProcessId();

	if (!ghReqCommandEvent)
	{
		ghReqCommandEvent = CreateEvent(nullptr,FALSE,FALSE,nullptr);
		_ASSERTE(ghReqCommandEvent!=nullptr);
	}

	if (!ghPluginSemaphore)
	{
		ghPluginSemaphore = CreateSemaphore(nullptr, 1, 1, nullptr);
		_ASSERTE(ghPluginSemaphore!=nullptr);
	}

	// Запустить сервер команд
	if (!PlugServerStart())
	{
		TODO("Показать ошибку");
	}

	ghConsoleWrite = CreateEvent(nullptr,FALSE,FALSE,nullptr);
	_ASSERTE(ghConsoleWrite!=nullptr);
	ghConsoleInputEmpty = CreateEvent(nullptr,FALSE,FALSE,nullptr);
	_ASSERTE(ghConsoleInputEmpty!=nullptr);
	ghMonitorThread = apiCreateThread(MonitorThreadProcW, nullptr, &gnMonitorThreadId, "Plugin::MonitorThread");

	//ghInputThread = apiCreateThread(nullptr, 0, InputThreadProcW, 0, 0, &gnInputThreadId);

	// Если мы не под эмулятором - больше ничего делать не нужно
	if (ghConEmuWndDC)
	{
		//
		DWORD dwPID, dwThread;
		dwThread = GetWindowThreadProcessId(ghConEmuWndDC, &dwPID);
		typedef BOOL (WINAPI* AllowSetForegroundWindowT)(DWORD);
		HMODULE hUser32 = GetModuleHandle(L"user32.dll");

		if (hUser32)
		{
			AllowSetForegroundWindowT AllowSetForegroundWindowF = (AllowSetForegroundWindowT)GetProcAddress(hUser32, "AllowSetForegroundWindow");

			if (AllowSetForegroundWindowF) AllowSetForegroundWindowF(dwPID);
		}

		#if 0
		// дернуть табы, если они нужны
		int tabCount = 0;
		MSectionLock SC; SC.Lock(csTabs);
		CreateTabs(1);
		AddTab(tabCount, 0, false, false, WTYPE_PANELS, nullptr, nullptr, 1, 0, 0, 0);
		// Сейчас отсылать не будем - выполним, когда вызовется SetStartupInfo -> CommonStartup
		//SendTabs(tabCount=1, TRUE);
		SC.Unlock();
		#endif
	}
}

void CPluginBase::CheckConEmuDetached()
{
	if (ghConEmuWndDC)
	{
		// ConEmu могло подцепиться
		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> ConMap;
		ConMap.InitName(CECONMAPNAME, LODWORD(FarHwnd));

		if (ConMap.Open())
		{
			if (ConMap.Ptr()->hConEmuWndDc == nullptr)
			{
				ghConEmuWndDC = nullptr;
			}

			ConMap.CloseMap();
		}
		else
		{
			ghConEmuWndDC = nullptr;
		}
	}
}

void CPluginBase::EmergencyShow()
{
	if (IsWindowVisible(FarHwnd))
		return;

	// If there is a ConEmuCD - just skip 'Plugin version'
	HMODULE hSrv = GetModuleHandle(ConEmuCD_DLL_3264);
	if (hSrv)
		return;

	// May be server exists? Wait a little for it
	Sleep(2000);
	if (IsWindowVisible(FarHwnd))
		return;

	// Last chance, lets try to do it here
	::EmergencyShow(FarHwnd);
}

// Эту нить нужно оставить, чтобы была возможность отобразить консоль при падении ConEmu
// static, WINAPI
DWORD CPluginBase::MonitorThreadProcW(LPVOID lpParameter)
{
	//DWORD dwProcId = GetCurrentProcessId();
	DWORD dwStartTick = GetTickCount();
	//DWORD dwMonitorTick = dwStartTick;
	BOOL lbStartedNoConEmu = (ghConEmuWndDC == nullptr) && !gbStartedUnderConsole2;
	//BOOL lbTryOpenMapHeader = FALSE;
	//_ASSERTE(ghConEmuWndDC!=nullptr); -- ConEmu может подцепиться позднее!

	WARNING("В MonitorThread нужно также отслеживать и 'живость' сервера. Иначе приложение останется невидимым (");

	while(true)
	{
		DWORD dwWait = 0;
		DWORD dwTimeout = 500;
		/*#ifdef _DEBUG
		dwTimeout = INFINITE;
		#endif*/
		//dwWait = WaitForMultipleObjects(MAXCMDCOUNT, hEventCmd, FALSE, dwTimeout);
		dwWait = WaitForSingleObject(ghServerTerminateEvent, dwTimeout);

		if (dwWait == WAIT_OBJECT_0)
			break; // завершение плагина

		// No input hook is available to request metadata refreshes in this mode.
		// Schedule Far APIs on its main thread, including when no background plugin is enabled.
		if (!ghHooksModule && ghConEmuWndDC && IS_SYNCHRO_ALLOWED && gnSynchroCount == 0)
		{
			gbNeedPostReloadFarInfo = TRUE;
			Plugin()->ExecuteSynchro();
		}

		// Если FAR запущен в "невидимом" режиме и по истечении таймаута
		// так и не подцепились к ConEmu - всплыть окошко консоли
		if (lbStartedNoConEmu && ghConEmuWndDC == nullptr && FarHwnd != nullptr)
		{
			DWORD dwCurTick = GetTickCount();
			DWORD dwDelta = dwCurTick - dwStartTick;

			if (dwDelta > GUI_ATTACH_TIMEOUT)
			{
				lbStartedNoConEmu = FALSE;

				if (!TerminalMode && !IsWindowVisible(FarHwnd))
				{
					EmergencyShow();
				}
			}
		}

		// Теоретически, нить обработки может запуститься и без ghConEmuWndDC (под телнетом)
		if (ghConEmuWndDC && FarHwnd && (dwWait == WAIT_TIMEOUT))
		{
			// Может быть ConEmu свалилось
			if (!IsWindow(ghConEmuWndDC) && ghConEmuWndDC)
			{
				HWND hConWnd = GetConEmuHWND(ConEmuWndType::ConsoleWindow);

				if ((hConWnd && !IsWindow(hConWnd))
					|| (!gbWasDetached && FarHwnd && !IsWindow(FarHwnd)))
				{
					// hConWnd не валидно
					wchar_t szWarning[255];
					swprintf_c(szWarning, L"Console was abnormally termintated!\r\nExiting from FAR (PID=%u)", GetCurrentProcessId());
					ShowMessageBox(szWarning, MB_OK|MB_ICONSTOP|MB_SETFOREGROUND);
					TerminateProcess(GetCurrentProcess(), 100);
					return 0;
				}

				if (!TerminalMode && !IsWindowVisible(FarHwnd))
				{
					EmergencyShow();
				}
				else if (!gbWasDetached)
				{
					gbWasDetached = TRUE;
					ghConEmuWndDC = nullptr;
				}
			}
		}

		if (gbWasDetached && !ghConEmuWndDC)
		{
			// ConEmu могло подцепиться
			if (gpConMapInfo && gpConMapInfo->hConEmuWndDc && IsWindow(gpConMapInfo->hConEmuWndDc))
			{
				gbWasDetached = FALSE;
				ghConEmuWndDC = (HWND)gpConMapInfo->hConEmuWndDc;

				// Update our in-process env vars
				SetConEmuEnvVar(gpConMapInfo->hConEmuRoot);
				SetConEmuEnvVarChild(gpConMapInfo->hConEmuWndDc, gpConMapInfo->hConEmuWndBack);

				if (gbStartupHooksAfterMap)
				{
					gbStartupHooksAfterMap = FALSE;
					StartupHooks(ghPluginModule);
				}

				CPluginBase* p = Plugin();

				// Передернуть отрисовку, чтобы обновить TrueColor
				p->RedrawAll();

				// Inform GUI about our Far/Plugin
				p->InitResources();

				// Обновить ТАБЫ после реаттача
				if (gnCurTabCount && gpTabs)
				{
					p->SendTabs(gnCurTabCount, TRUE);
				}
			}
			else if (FarHwnd && gbFarWndVisible && !gbTryOpenMapHeader)
			{
				if (!IsWindowVisible(FarHwnd))
				{
					gbFarWndVisible = FALSE;
					gbTryOpenMapHeader = TRUE;
				}
			}
		}

		//if (ghConEmuWndDC && gbMonitorEnvVar && gsMonitorEnvVar[0]
		//        && (GetTickCount() - dwMonitorTick) > MONITORENVVARDELTA)
		//{
		//	UpdateEnvVar(gsMonitorEnvVar);
		//	dwMonitorTick = GetTickCount();
		//}

		if (gbNeedPostTabSend)
		{
			DWORD nDelta = GetTickCount() - gnNeedPostTabSendTick;

			if (nDelta > NEEDPOSTTABSENDDELTA)
			{
				if (Plugin()->IsMacroActive())
				{
					gnNeedPostTabSendTick = GetTickCount();
				}
				else
				{
					// Force Send tabs to ConEmu
					MSectionLock SC; SC.Lock(csTabs, TRUE); // блокируем exclusively, чтобы во время пересылки данные не поменялись из другого потока
					SendTabs(gnCurTabCount, TRUE);
					SC.Unlock();
				}
			}
		}

		if (/*ghConEmuWndDC &&*/ gbTryOpenMapHeader)
		{
			if (gpConMapInfo)
			{
				_ASSERTE(gpConMapInfo == nullptr);
				gbTryOpenMapHeader = FALSE;
			}
			else if (OpenMapHeader() == 0)
			{
				// OK, переподцепились
				gbTryOpenMapHeader = FALSE;
			}

			if (gpConMapInfo)
			{
				// 04.03.2010 Maks - Если мэппинг открыли - принудительно передернуть ресурсы и информацию
				//CheckResources(true); -- должен выполняться в основной нити, поэтому - через Activate
				// 22.09.2010 Maks - вызывать ActivatePlugin - некорректно!
				//ActivatePlugin(CMD_CHKRESOURCES, nullptr);
				Plugin()->ProcessCommand(CMD_CHKRESOURCES, TRUE/*bReqMainThread*/, nullptr);
			}
		}

		if (gbStartupHooksAfterMap && gpConMapInfo && ghConEmuWndDC && IsWindow(ghConEmuWndDC))
		{
			gbStartupHooksAfterMap = FALSE;
			StartupHooks(ghPluginModule);
		}

		if (gpBgPlugin)
		{
			gpBgPlugin->MonitorBackground();
		}

		//if (gpConMapInfo) {
		//	if (gpConMapInfo->nFarPID == 0)
		//		gbNeedReloadFarInfo = TRUE;
		//}
	}

	return 0;
}

HANDLE CPluginBase::OpenPluginCommon(int OpenFrom, INT_PTR Item, bool FromMacro)
{
	if (!mb_StartupInfoOk)
		return InvalidPanelHandle;

	HANDLE hResult = InvalidPanelHandle;
	INT_PTR nID = pcc_None; // выбор из меню

	#ifdef _DEBUG
	if (gFarVersion.dwVerMajor==1)
	{
		wchar_t szInfo[128]; swprintf_c(szInfo, L"OpenPlugin[Ansi] (%i%s, Item=0x%X, gnReqCmd=%i%s)\n",
		                               OpenFrom, (OpenFrom==of_CommandLine) ? L"[OPEN_COMMANDLINE]" :
		                               (OpenFrom==of_PluginsMenu) ? L"[OPEN_PLUGINSMENU]" : L"",
		                               (DWORD)Item,
		                               (int)gnReqCommand,
		                               (gnReqCommand == (DWORD)-1) ? L"" :
		                               (gnReqCommand == CMD_REDRAWFAR) ? L"[CMD_REDRAWFAR]" :
		                               (gnReqCommand == CMD_EMENU) ? L"[CMD_EMENU]" :
		                               (gnReqCommand == CMD_SETWINDOW) ? L"[CMD_SETWINDOW]" :
		                               (gnReqCommand == CMD_POSTMACRO) ? L"[CMD_POSTMACRO]" :
		                               L"");
		OutputDebugStringW(szInfo);
	}
	#endif

	if ((OpenFrom == of_CommandLine) && Item)
	{
		if (gFarVersion.dwVerMajor==1)
		{
			wchar_t* pszUnicode = ToUnicode((char*)Item);
			ProcessCommandLine(pszUnicode);
			SafeFree(pszUnicode);
		}
		else
		{
			ProcessCommandLine((wchar_t*)Item);
		}
		goto wrap;
	}

	// Asynchronous update of current panel directory
	if (FromMacro && ((Item == CE_CALLPLUGIN_REQ_DIRA) || (Item == CE_CALLPLUGIN_REQ_DIRP)))
	{
		wchar_t* pszActive = nullptr;
		wchar_t* pszPassive = nullptr;
		if (Item == CE_CALLPLUGIN_REQ_DIRA)
			pszActive = GetPanelDir(gpdf_Active|gpdf_NoPlugin);
		else if (Item == CE_CALLPLUGIN_REQ_DIRP)
			pszPassive = GetPanelDir(gpdf_Passive|gpdf_NoPlugin);
		// Succeeded? Due to Far2 callplugin realization we can check/store only one panel at a time
		if (pszActive || pszPassive)
			StorePanelDirs(pszActive, pszPassive);
		// Free mem
		SafeFree(pszActive);
		SafeFree(pszPassive);
		goto wrap;
	}

	if (gnReqCommand != (DWORD)-1)
	{
		gnPluginOpenFrom = (OpenFrom & 0xFFFF);
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData);
		goto wrap;
	}

	if (FromMacro)
	{
		if (Item >= 0x4000)
		{
			// Хорошо бы, конечно точнее определять, строка это, или нет...
			LPCWSTR pszCallCmd = (LPCWSTR)Item;

			if (!IsBadStringPtrW(pszCallCmd, 255) && *pszCallCmd)
			{
				if (!ghConEmuWndDC)
				{
					SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);
				}
				else
				{
					int nLen = lstrlenW(pszCallCmd);
					CESERVER_REQ *pIn = nullptr, *pOut = nullptr;
					pIn = ExecuteNewCmd(CECMD_GUIMACRO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GUIMACRO)+nLen*sizeof(wchar_t));
					lstrcpyW(pIn->GuiMacro.sMacro, pszCallCmd);
					pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);

					if (pOut)
					{
						SetEnvironmentVariable(CEGUIMACRORETENVVAR,
						                       pOut->GuiMacro.nSucceeded ? pOut->GuiMacro.sMacro : nullptr);
						ExecuteFreeResult(pOut);
						// 130708 -- Otherwise Far Macro "Plugin.Call" returns "0" always...
						hResult = (HANDLE)TRUE;
					}
					else
					{
						SetEnvironmentVariable(CEGUIMACRORETENVVAR, nullptr);
					}

					ExecuteFreeResult(pIn);
				}
			}

			goto wrap;
		}

		if (Item >= pcc_First && Item <= pcc_Last)
		{
			nID = Item; // Будет сразу выполнена команда
		}
		else if (Item >= SETWND_CALLPLUGIN_BASE)
		{
			// Переключение табов выполняется макросом, чтобы "убрать" QSearch и выполнить проверки
			// (посылается из OnMainThreadActivated: gnReqCommand == CMD_SETWINDOW)
			DEBUGSTRCMD(L"Plugin: SETWND_CALLPLUGIN_BASE\n");
			gnPluginOpenFrom = of_PluginsMenu;
			DWORD nTab = (DWORD)(Item - SETWND_CALLPLUGIN_BASE);
			ProcessCommand(CMD_SETWINDOW, FALSE, &nTab);
			SetEvent(ghSetWndSendTabsEvent);
			goto wrap;
		}
		else if (Item == CE_CALLPLUGIN_SENDTABS)
		{
			DEBUGSTRCMD(L"Plugin: CE_CALLPLUGIN_SENDTABS\n");
			// Force Send tabs to ConEmu
			//MSectionLock SC; SC.Lock(csTabs, TRUE);
			//SendTabs(gnCurTabCount, TRUE);
			//SC.Unlock();
			gbForceSendTabs = TRUE;
			UpdateConEmuTabs(true);
			SetEvent(ghSetWndSendTabsEvent);
			goto wrap;
		}
		else if (Item == CE_CALLPLUGIN_UPDATEBG)
		{
			if (gpBgPlugin)
			{
				gpBgPlugin->SetForceUpdate(true);
				gpBgPlugin->OnMainThreadActivated();
			}
			goto wrap;
		}
	}

	ShowPluginMenu((PluginCallCommands)nID);

wrap:
	#ifdef _DEBUG
	if ((gFarVersion.dwVerMajor==1) && (gnReqCommand != (DWORD)-1))
	{
		wchar_t szInfo[128]; swprintf_c(szInfo, L"*** OpenPlugin[Ansi] post gnReqCmd=%i%s\n",
		                               (int)gnReqCommand,
		                               (gnReqCommand == (DWORD)-1) ? L"" :
		                               (gnReqCommand == CMD_REDRAWFAR) ? L"CMD_REDRAWFAR" :
		                               (gnReqCommand == CMD_EMENU) ? L"CMD_EMENU" :
		                               (gnReqCommand == CMD_SETWINDOW) ? L"CMD_SETWINDOW" :
		                               (gnReqCommand == CMD_POSTMACRO) ? L"CMD_POSTMACRO" :
		                               L"");
		OutputDebugStringW(szInfo);
	}
	#endif
	return hResult;
}

void CPluginBase::ExitFarCommon(bool bFromDllMain /*= false*/)
{
	ShutdownPluginStep(L"ExitFarCmn");

	gbExitFarCalled = true;

	// Плагин выгружается, Вызывать Syncho больше нельзя
	gbSynchroProhibited = true;
	ShutdownHooks();
	StopThread();

	ShutdownPluginStep(L"ExitFarCmn - done");
}

// Вызывается из ACTL_SYNCHRO для FAR2
// или при ConsoleReadInput(1) в FAR1
void CPluginBase::OnMainThreadActivated()
{
	// Теоретически, в FAR2 мы сюда можем попасть и не из основной нити,
	// если таки будет переделана "thread-safe" активация.
	if (gbNeedPostEditCheck)
	{
		DWORD currentModifiedState = Plugin()->GetEditorModifiedState();

		if (lastModifiedStateW != (int)currentModifiedState)
		{
			lastModifiedStateW = (int)currentModifiedState;
			gbRequestUpdateTabs = TRUE;
		}

		// 100909 - не было
		gbNeedPostEditCheck = FALSE;
	}

	// To avoid spare API calls
	int iMacroActive = 0;

	if (!gbRequestUpdateTabs && gbNeedPostTabSend)
	{
		if (!Plugin()->isMacroActive(iMacroActive))
		{
			gbRequestUpdateTabs = TRUE; gbNeedPostTabSend = FALSE;
		}
	}

	if (gbRequestUpdateTabs && !Plugin()->isMacroActive(iMacroActive))
	{
		gbRequestUpdateTabs = gbNeedPostTabSend = FALSE;
		Plugin()->UpdateConEmuTabs(true);

		if (gbClosingModalViewerEditor)
		{
			gbClosingModalViewerEditor = FALSE;
			gbRequestUpdateTabs = TRUE;
		}
	}

	// Retrieve current panel CD's
	// Remove (gnCurrentWindowType == WTYPE_PANELS) restriction,
	// panel paths may be changed even from editor
	if (!Plugin()->isMacroActive(iMacroActive))
	{
		DEBUGSTRCURDIR(L"UpdatePanelDirs()");
		gbNeedReloadFarDir = FALSE;
		Plugin()->UpdatePanelDirs();

		gnSynchroThrottleTick = 0;
	}

	// !!! Это только чисто в OnConsolePeekReadInput, т.к. FAR Api тут не используется
	//if (gpConMapInfo && gpFarInfo && gpFarInfoMapping)
	//	TouchReadPeekConsoleInputs(abPeek ? 1 : 0);

	if (gbNeedPostReloadFarInfo)
	{
		DEBUGSTRCURDIR(L"ReloadFarInfo()");
		gbNeedPostReloadFarInfo = FALSE;
		ReloadFarInfo(false);
	}

	// !!! Это только чисто в OnConsolePeekReadInput, т.к. FAR Api тут не используется
	//// В некоторых случаях (CMD_LEFTCLKSYNC,CMD_CLOSEQSEARCH,...) нужно дождаться, пока очередь опустеет
	//if (gbWaitConsoleInputEmpty)
	//{
	//	DWORD nTestEvents = 0;
	//	HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
	//	if (GetNumberOfConsoleInputEvents(h, &nTestEvents))
	//	{
	//		if (nTestEvents == 0)
	//		{
	//			gbWaitConsoleInputEmpty = FALSE;
	//			SetEvent(ghConsoleInputEmpty);
	//		}
	//	}
	//}

	// Если был запрос на обновление Background
	if (gbNeedBgActivate)  // выставляется в gpBgPlugin->SetForceCheck() или SetForceUpdate()
	{
		gbNeedBgActivate = FALSE;

		if (gpBgPlugin)
			gpBgPlugin->OnMainThreadActivated();
	}

	// Проверяем, надо ли "активировать" плагин?
	if (!gbReqCommandWaiting || gnReqCommand == (DWORD)-1)
	{
		return; // активация в данный момент не требуется
	}

	gbReqCommandWaiting = FALSE; // чтобы ожидающая нить случайно не удалила параметры, когда мы работаем
	TODO("Определить текущую область... (panel/editor/viewer/menu/...");
	gnPluginOpenFrom = 0;

	// Обработка CtrlTab из ConEmu
	if (gnReqCommand == CMD_SETWINDOW)
	{
		ProcessSetWindowCommand();
	}
	else
	{
		// Результата ожидает вызывающая нить, поэтому передаем параметр
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData, &gpCmdRet);
		// Но не освобождаем его (pCmdRet) - это сделает ожидающая нить
	}

	// Мы закончили
	SetEvent(ghReqCommandEvent);
}

void CPluginBase::ProcessSetWindowCommand()
{
	// Обработка CtrlTab из ConEmu
	_ASSERTE(gnReqCommand == CMD_SETWINDOW);
	DEBUGSTRCMD(L"Plugin: OnMainThreadActivated: CMD_SETWINDOW\n");

	if (gFarVersion.dwVerMajor==1)
	{
		gnPluginOpenFrom = of_PluginsMenu;
		// Результата ожидает вызывающая нить, поэтому передаем параметр
		ProcessCommand(gnReqCommand, FALSE/*bReqMainThread*/, gpReqCommandData, &gpCmdRet);
	}
	else
	{
		// Необходимо быть в panel/editor/viewer
		wchar_t szMacro[255];
		DWORD nTabShift = SETWND_CALLPLUGIN_BASE + *((DWORD*)gpReqCommandData);
		// Если панели-редактор-вьювер - сменить окно. Иначе - отослать в GUI табы
		if (gFarVersion.dwVerMajor == 2)
		{
			swprintf_c(szMacro, L"$if (Search) Esc $end $if (Shell||Viewer||Editor) callplugin(0x%08X,%i) $else callplugin(0x%08X,%i) $end",
				  ConEmu_SysID, nTabShift, ConEmu_SysID, CE_CALLPLUGIN_SENDTABS);
		}
		else if (!gFarVersion.IsFarLua())
		{
			swprintf_c(szMacro, L"$if (Search) Esc $end $if (Shell||Viewer||Editor) callplugin(\"%s\",%i) $else callplugin(\"%s\",%i) $end",
				  ConEmu_GuidS, nTabShift, ConEmu_GuidS, CE_CALLPLUGIN_SENDTABS);
		}
		else
		{
			swprintf_c(szMacro, L"if Area.Search then Keys(\"Esc\") end if Area.Shell or Area.Viewer or Area.Editor then Plugin.Call(\"%s\",%i) else Plugin.Call(\"%s\",%i) end",
				  ConEmu_GuidS, nTabShift, ConEmu_GuidS, CE_CALLPLUGIN_SENDTABS);
		}
		gnReqCommand = -1;
		gpReqCommandData = nullptr;
		PostMacro(szMacro, nullptr);
	}
	// Done
}

void CPluginBase::CommonPluginStartup()
{
	gbBgPluginsAllowed = TRUE;

	//111209 - CheckResources зовем перед UpdateConEmuTabs, т.к. иначе CheckResources вызывается дважды
	//2010-12-13 информацию (начальную) о фаре грузим всегда, а отсылаем в GUI только если в ConEmu
	// здесь же и ReloadFarInfo() позовется
	CheckResources(true);

	// Надо табы загрузить
	Plugin()->UpdateConEmuTabs(true);


	// Пробежаться по всем загруженным в данный момент плагинам и дернуть в них "OnConEmuLoaded"
	// А все из за того, что при запуске "Far.exe /co" - порядок загрузки плагинов МЕНЯЕТСЯ
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
	if (snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(MODULEENTRY32)};

		for (BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
		{
			OnConEmuLoaded_t fnOnConEmuLoaded;

			if (((fnOnConEmuLoaded = (OnConEmuLoaded_t)GetProcAddress(module.hModule, "OnConEmuLoaded")) != nullptr)
				&& /* Наверное, только для плагинов фара */
				((GetProcAddress(module.hModule, "SetStartupInfoW") || GetProcAddress(module.hModule, "SetStartupInfo"))))
			{
				OnLibraryLoaded(module.hModule);
			}
		}

		CloseHandle(snapshot);
	}


	//if (gpConMapInfo)  //2010-03-04 Имеет смысл только при запуске из-под ConEmu
	//{
	//	//CheckResources(true);
	//	LogCreateProcessCheck((LPCWSTR)-1);
	//}

	TODO("перенести инициализацию фаровских callback'ов в SetStartupInfo, т.к. будет грузиться как Inject!");

	if (!StartupHooks(ghPluginModule))
	{
		if (ghConEmuWndDC)
		{
			_ASSERTE(FALSE);
			DEBUGSTR(L"!!! Can't install injects!!!\n");
		}
		else
		{
			DEBUGSTR(L"No GUI, injects was not installed!\n");
		}
	}
}

void CPluginBase::StopThread(bool bFromDllMain /*= false*/)
{
	ShutdownPluginStep(L"StopThread");
	#ifdef _DEBUG
	LPCVOID lpPtrConInfo = gpConMapInfo;
	#endif
	gpConMapInfo = nullptr;
	//LPVOID lpPtrColorInfo = gpColorerInfo; gpColorerInfo = nullptr;
	gbBgPluginsAllowed = FALSE;
	NotifyConEmuUnloaded();

	ShutdownPluginStep(L"...ClosingTabs");
	CloseTabs();

	//if (hEventCmd[CMD_EXIT])
	//	SetEvent(hEventCmd[CMD_EXIT]); // Завершить нить

	if (ghServerTerminateEvent)
	{
		SetEvent(ghServerTerminateEvent);
	}

	//if (gnInputThreadId) {
	//	PostThreadMessage(gnInputThreadId, WM_QUIT, 0, 0);
	//}

	ShutdownPluginStep(L"...Stopping server");
	PlugServerStop(bFromDllMain);

	ShutdownPluginStep(L"...Finalizing");

	SafeCloseHandle(ghPluginSemaphore);

	if (ghMonitorThread)  // подождем чуть-чуть, или принудительно прибъем нить ожидания
	{
		if (WaitForSingleObject(ghMonitorThread,1000))
		{
#if !defined(__GNUC__)
#pragma warning (disable : 6258)
#endif
			apiTerminateThread(ghMonitorThread, 100);
		}

		SafeCloseHandle(ghMonitorThread);
	}

	//if (ghInputThread) { // подождем чуть-чуть, или принудительно прибъем нить ожидания
	//	if (WaitForSingleObject(ghInputThread,1000)) {
	//		#if !defined(__GNUC__)
	//		#pragma warning (disable : 6258)
	//		#endif
	//		apiTerminateThread(ghInputThread, 100);
	//	}
	//	SafeCloseHandle(ghInputThread);
	//}

	if (gpTabs)
	{
		free(gpTabs);
		gpTabs = nullptr;
	}

	if (ghReqCommandEvent)
	{
		CloseHandle(ghReqCommandEvent); ghReqCommandEvent = nullptr;
	}

	if (gpFarInfo)
	{
		LPVOID ptr = gpFarInfo; gpFarInfo = nullptr;
		free(ptr);
	}

	if (gpFarInfoMapping)
	{
		UnmapViewOfFile(gpFarInfoMapping);
		CloseHandle(ghFarInfoMapping);
		ghFarInfoMapping = nullptr;
	}

	if (ghFarAliveEvent)
	{
		CloseHandle(ghFarAliveEvent);
		ghFarAliveEvent = nullptr;
	}

	if (ghRegMonitorKey) { RegCloseKey(ghRegMonitorKey); ghRegMonitorKey = nullptr; }

	SafeCloseHandle(ghRegMonitorEvt);
	SafeCloseHandle(ghServerTerminateEvent);
	//WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
	//SafeCloseHandle(ghConIn);
	//SafeCloseHandle(ghInputSynchroExecuted);
	SafeCloseHandle(ghSetWndSendTabsEvent);
	SafeCloseHandle(ghConsoleInputEmpty);
	SafeCloseHandle(ghConsoleWrite);

	if (gpConMap)
	{
		gpConMap->CloseMap();
		delete gpConMap;
		gpConMap = nullptr;
	}

	//if (lpPtrConInfo)
	//{
	//	UnmapViewOfFile(lpPtrConInfo);
	//}
	//if (ghFileMapping)
	//{
	//	CloseHandle(ghFileMapping);
	//	ghFileMapping = nullptr;
	//}
	// -- теперь мэппинги создает GUI
	//CloseColorerHeader();

	CommonShutdown();
	ShutdownPluginStep(L"StopThread - done");
}

void CPluginBase::ShutdownPluginStep(LPCWSTR asInfo, int nParm1 /*= 0*/, int nParm2 /*= 0*/, int nParm3 /*= 0*/, int nParm4 /*= 0*/)
{
#ifdef _DEBUG
	static int nDbg = 0;
	if (!nDbg)
		nDbg = IsDebuggerPresent() ? 1 : 2;
	if (nDbg != 1)
		return;
	wchar_t szFull[512];
	msprintf(szFull, countof(szFull), L"%u:ConEmuP:PID=%u:TID=%u: ",
		GetTickCount(), GetCurrentProcessId(), GetCurrentThreadId());
	if (asInfo)
	{
		int nLen = lstrlen(szFull);
		msprintf(szFull+nLen, countof(szFull)-nLen, asInfo, nParm1, nParm2, nParm3, nParm4);
	}
	lstrcat(szFull, L"\n");
	OutputDebugString(szFull);
#endif
}

// Теоретически, из этой функции Far2+ может сразу вызвать ProcessSynchroEventW.
// Но в текущей версии Far 2/3 она работает асинхронно и сразу выходит, а сама
// ProcessSynchroEventW зовется потом в главной нити (где-то при чтении буфера консоли)
void CPluginBase::ExecuteSynchro()
{
	WARNING("Нет способа определить, будет ли фар вызывать наш ProcessSynchroEventW и в какой момент");
	// Например, если в фаре выставлен ProcessException - то никакие плагины больше не зовутся

	if (IS_SYNCHRO_ALLOWED)
	{
		if (gbSynchroProhibited)
		{
			_ASSERTE(gbSynchroProhibited==false);
			return;
		}

		//Чтобы не было зависаний при попытке активации плагина во время прокрутки
		//редактора, в плагине мониторить нажатие мыши. Если последнее МЫШИНОЕ событие
		//было с нажатой кнопкой - сначала пульнуть в консоль команду "отпускания" кнопки,
		//и только после этого - пытаться активироваться.
		if ((gnAllowDummyMouseEvent > 0) && (gLastMouseReadEvent.dwButtonState & (RIGHTMOST_BUTTON_PRESSED|FROM_LEFT_1ST_BUTTON_PRESSED)))
		{
			//_ASSERTE(!(gLastMouseReadEvent.dwButtonState & (RIGHTMOST_BUTTON_PRESSED|FROM_LEFT_1ST_BUTTON_PRESSED)));
			int nWindowType = Plugin()->GetActiveWindowType();
			// "Зависания" возможны (вроде) только при прокрутке зажатой кнопкой мышки
			// редактора или вьювера. Так что в других областях - не дергаться.
			if (nWindowType == wt_Editor || nWindowType == wt_Viewer)
			{
				gbUngetDummyMouseEvent = TRUE;
			}
		}

		Plugin()->ExecuteSynchroApi();
	}
}

DWORD CPluginBase::WaitPluginActivation(DWORD nCount, HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds)
{
	DWORD nWait = WAIT_TIMEOUT;
	if (IS_SYNCHRO_ALLOWED)
	{
		DWORD nStepWait = 1000;
		DWORD nPrevCount = gnPeekReadCount;

		#ifdef _DEBUG
		if (IsDebuggerPresent())
		{
			nStepWait = 30000;
			if (dwMilliseconds && (dwMilliseconds < 60000))
				dwMilliseconds = 60000;
		}
		#endif

		DWORD nStartTick = GetTickCount(), nCurrentTick = 0;
		DWORD nTimeout = nStartTick + dwMilliseconds;
		bool lbLongOperationWasStarted = false;
		do {
			nWait = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, std::min(dwMilliseconds,nStepWait));
			if (((nWait >= WAIT_OBJECT_0) && (nWait < (WAIT_OBJECT_0+nCount))) || (nWait != WAIT_TIMEOUT))
			{
				_ASSERTE((nWait >= WAIT_OBJECT_0) && (nWait < (WAIT_OBJECT_0+nCount)));
				break; // Succeeded
			}

			nCurrentTick = GetTickCount();

			_ASSERTE(nWait == WAIT_TIMEOUT);

			if (gnInLongOperation > 0)
			{
				// if long operation was pended (like opening new editor window)
				nStartTick = nCurrentTick;
				lbLongOperationWasStarted = true;
				continue; // No need to call ExecuteSynchro repeatedly
			}
			else if ((nPrevCount == gnPeekReadCount) && (dwMilliseconds > 1000)
				&& !lbLongOperationWasStarted
				#ifdef _DEBUG
				&& (!IsDebuggerPresent() || (nCurrentTick > (nStartTick + nStepWait)))
				#endif
				)
			{
				// Ждать дальше смысла видимо нет, фар не дергает (Peek/Read)Input
				break;
			}

			// Если вдруг произошел облом с Syncho (почему?), дернем еще раз
			Plugin()->ExecuteSynchro();
		} while (dwMilliseconds && ((dwMilliseconds == INFINITE) || (nCurrentTick <= nTimeout)));

		#ifdef _DEBUG
		if (nWait == WAIT_TIMEOUT)
		{
			DEBUGSTRACTIVATE(L"ConEmu plugin activation failed");
		}
		#endif
	}
	else
	{
		nWait = WaitForMultipleObjects(nCount, lpHandles, bWaitAll, dwMilliseconds);
	}
	return nWait;
}

// Должна вызываться ТОЛЬКО из нитей уже заблокированных семафором ghPluginSemaphore
bool CPluginBase::ActivatePlugin(DWORD nCmd, LPVOID pCommandData, DWORD nTimeout /*= CONEMUFARTIMEOUT*/) // Release=10сек, Debug=2мин.
{
	bool lbRc = false;
	ResetEvent(ghReqCommandEvent);
	//gbCmdCallObsolete = FALSE;
	gnReqCommand = nCmd; gpReqCommandData = pCommandData;
	gnPluginOpenFrom = -1;
	// Нужен вызов плагина в остновной нити
	gbReqCommandWaiting = TRUE;
	DWORD nWait = 100; // если тут останется (!=0) - функция вернут ошибку
	HANDLE hEvents[] = {ghServerTerminateEvent, ghReqCommandEvent};
	int nCount = countof(hEvents);
	DEBUGSTRMENU(L"*** Waiting for plugin activation\n");

	if (nCmd == CMD_REDRAWFAR || nCmd == CMD_FARPOST)
	{
		WARNING("Оптимизировать!");
		nTimeout = std::min<DWORD>(1000, nTimeout); // чтобы не зависало при попытке ресайза, если фар не отзывается.
	}

	if (gbSynchroProhibited)
	{
		nWait = WAIT_TIMEOUT;
	}
	// Если есть ACTL_SYNCHRO - позвать его, иначе - "активация" в главной нити
	// выполняется тогда, когда фар зовет ReadConsoleInput(1).
	//if (gFarVersion.dwVerMajor = 2 && gFarVersion.dwBuild >= 1006)
	else if (IS_SYNCHRO_ALLOWED)
	{
		#ifdef _DEBUG
		int iArea = Plugin()->GetMacroArea();
		#endif

		InterlockedIncrement(&gnAllowDummyMouseEvent);
		ExecuteSynchro();

		if (!gbUngetDummyMouseEvent && (gLastMouseReadEvent.dwButtonState & (RIGHTMOST_BUTTON_PRESSED|FROM_LEFT_1ST_BUTTON_PRESSED)))
		{
			// Страховка от зависаний
			nWait = WaitForMultipleObjects(nCount, hEvents, FALSE, std::min(1000UL, std::max(250UL, nTimeout)));
			if (nWait == WAIT_TIMEOUT)
			{
				if (!gbUngetDummyMouseEvent && (gLastMouseReadEvent.dwButtonState & (RIGHTMOST_BUTTON_PRESSED|FROM_LEFT_1ST_BUTTON_PRESSED)))
				{
					gbUngetDummyMouseEvent = TRUE;
					// попытаться еще раз
					nWait = WaitPluginActivation(nCount, hEvents, FALSE, nTimeout);
				}
			}
		}
		else
		{
			// Подождать активации. Сколько ждать - может указать вызывающая функция
			nWait = WaitPluginActivation(nCount, hEvents, FALSE, nTimeout);
		}

		if (gnAllowDummyMouseEvent > 0)
		{
			InterlockedDecrement(&gnAllowDummyMouseEvent);
		}
		else
		{
			_ASSERTE(gnAllowDummyMouseEvent >= 0);
			if (gnAllowDummyMouseEvent < 0)
				gnAllowDummyMouseEvent = 0;
		}

	}
	else
	{
		// Подождать активации. Сколько ждать - может указать вызывающая функция
		nWait = WaitPluginActivation(nCount, hEvents, FALSE, nTimeout);
	}


	if (nWait != WAIT_OBJECT_0 && nWait != (WAIT_OBJECT_0+1))
	{
		//110712 - если CMD_REDRAWFAR, то показывать Assert смысла мало, фар может быть занят
		//  например чтением панелей?
		//На CMD_SETWINDOW тоже ругаться не будем - окошко может быть заблокировано, или фар занят.
		_ASSERTE(nWait==WAIT_OBJECT_0 || (nCmd==CMD_REDRAWFAR) || (nCmd==CMD_SETWINDOW));

		if (nWait == (WAIT_OBJECT_0+1))
		{
			if (!gbReqCommandWaiting)
			{
				// Значит плагин в основной нити все-таки активировался, подождем еще?
				DEBUGSTR(L"!!! Plugin execute timeout !!!\n");
				nWait = WaitForMultipleObjects(nCount, hEvents, FALSE, nTimeout);
			}

			//// Таймаут, эту команду плагин должен пропустить, когда фар таки соберется ее выполнить
			//Param->Obsolete = TRUE;
		}
	}
	else
	{
		DEBUGSTRMENU(L"*** DONE\n");
	}

	lbRc = (nWait == (WAIT_OBJECT_0+1));

	if (!lbRc)
	{
		// Сразу сбросим, вдруг не дождались?
		gbReqCommandWaiting = FALSE;
		ResetEvent(ghReqCommandEvent);
	}

	gpReqCommandData = nullptr;
	gnReqCommand = -1; gnPluginOpenFrom = -1;
	return lbRc;
}

bool CPluginBase::cmd_OpenEditorLine(CESERVER_REQ_FAREDITOR *pCmd)
{
	bool lbRc = false;

	if (!IsCurrentTabModal() && CheckPanelExist())
	{
		int nWindowType = GetActiveWindowType();
		if ((nWindowType == wt_Panels) || (nWindowType == wt_Editor) || (nWindowType == wt_Viewer)
			|| (wt_Desktop != -1 && nWindowType == wt_Desktop))
		{
			MSetter lSet(&gnInLongOperation);
			if (pCmd->nLine == 0)
				pCmd->nLine = pCmd->nColon = -1;
			lbRc = OpenEditor(pCmd->szFile, false/*abView*/, false/*abDeleteTempFile*/, true/*abDetectCP*/, pCmd->nLine, pCmd->nColon);
		}
	}

	#if 0
	LPCWSTR pSrc = pCmd->szFile;
	INT_PTR cchMax = MAX_PATH*4 + lstrlenW(pSrc); //-V112
	wchar_t* pszMacro = (wchar_t*)malloc(cchMax*sizeof(*pszMacro));
	if (!pszMacro)
	{
		_ASSERTE(pszMacro!=nullptr)
	}
	else
	{
		// Добавим префикс "^", чтобы не вообще посылать "нажатия кнопок" в плагины
		// Иначе, если например активна панель с RegEditor'ом, то результат будет неожиданным )
		if (gFarVersion.dwVerMajor==1)
			_wcscpy_c(pszMacro, cchMax, L"@^$if(Viewer || Editor) F12 0 $end $if(Shell) ShiftF4 \"");
		else if (!gFarVersion.IsFarLua())
			_wcscpy_c(pszMacro, cchMax, L"@^$if(Viewer || Editor) F12 0 $end $if(Shell) ShiftF4 print(\"");
		else if (gFarVersion.IsDesktop()) // '0' is 'Desktop' now
			_wcscpy_c(pszMacro, cchMax, L"@^if Area.Viewer or Area.Editor then Keys(\"F12 1\") end if Area.Shell then Keys(\"ShiftF4\") print(\"");
		else
			_wcscpy_c(pszMacro, cchMax, L"@^if Area.Viewer or Area.Editor then Keys(\"F12 0\") end if Area.Shell then Keys(\"ShiftF4\") print(\"");
		wchar_t* pDst = pszMacro + lstrlen(pszMacro);
		while (*pSrc)
		{
			*(pDst++) = *pSrc;
			if (*pSrc == L'\\')
				*(pDst++) = L'\\';
			pSrc++;
		}
		*pDst = 0;
		if (gFarVersion.dwVerMajor==1)
			_wcscat_c(pszMacro, cchMax, L"\" Enter ");
		else if (!gFarVersion.IsFarLua())
			_wcscat_c(pszMacro, cchMax, L"\") Enter ");
		else
			_wcscat_c(pszMacro, cchMax, L"\") Keys(\"Enter\") ");

		if (pCmd->nLine > 0)
		{
			int nCurLen = lstrlen(pszMacro);
			if (gFarVersion.dwVerMajor==1)
				swprintf_c(pszMacro+nCurLen, cchMax-nCurLen/*#SECURELEN*/, L" $if(Editor) AltF8 \"%i:%i\" Enter $end", pCmd->nLine, pCmd->nColon);
			else if (!gFarVersion.IsFarLua())
				swprintf_c(pszMacro+nCurLen, cchMax-nCurLen/*#SECURELEN*/, L" $if(Editor) AltF8 print(\"%i:%i\") Enter $end", pCmd->nLine, pCmd->nColon);
			else
				swprintf_c(pszMacro+nCurLen, cchMax-nCurLen/*#SECURELEN*/, L" if Area.Editor then Keys(\"AltF8\") print(\"%i:%i\") Keys(\"Enter\") end", pCmd->nLine, pCmd->nColon);
		}

		_wcscat_c(pszMacro, cchMax, (!gFarVersion.IsFarLua()) ? L" $end" : L" end");
		PostMacro(pszMacro, nullptr);
		free(pszMacro);

		lbRc = true;
	}
	#endif

	return lbRc;
}

bool CPluginBase::cmd_RedrawFarCall(CESERVER_REQ*& pCmdRet, CESERVER_REQ** ppResult)
{
	HANDLE hEvents[2] = {ghServerTerminateEvent, ghPluginSemaphore};
	DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

	if (dwWait == WAIT_OBJECT_0)
	{
		// Плагин завершается
		return false;
	}

	// Передернуть Background плагины
	if (gpBgPlugin) gpBgPlugin->SetForceUpdate();

	WARNING("После перехода на Synchro для FAR2 есть опасение, что следующий вызов может произойти до окончания предыдущего цикла обработки Synchro в Far");
	bool lbSucceeded = ActivatePlugin(CMD_FARPOST, nullptr);

	if (lbSucceeded && /*pOldCmdRet !=*/ gpCmdRet)
	{
		pCmdRet = gpCmdRet; // запомнить результат!

		if (ppResult != &gpCmdRet)
			gpCmdRet = nullptr;
	}

	ReleaseSemaphore(ghPluginSemaphore, 1, nullptr);

	return true;
}

bool CPluginBase::cmd_SetWindow(LPVOID pCommandData, bool bForceSendTabs)
{
	bool lbRc = false;
	int nTab = 0;

	// Для Far1 мы сюда попадаем обычным образом, при обработке команды пайпом
	// Для Far2 и выше - через макрос (проверяющий допустимость смены) и callplugin
	DEBUGSTRCMD(L"Plugin: ACTL_SETCURRENTWINDOW\n");

	// Окно мы можем сменить только если:
	if (gnPluginOpenFrom == of_Viewer || gnPluginOpenFrom == of_Editor
	        || gnPluginOpenFrom == of_PluginsMenu
			|| gnPluginOpenFrom == of_FilePanel)
	{
		_ASSERTE(pCommandData!=nullptr);

		if (pCommandData!=nullptr)
			nTab = *((DWORD*)pCommandData);

		gbIgnoreUpdateTabs = TRUE;

		Plugin()->SetWindow(nTab);

		DEBUGSTRCMD(L"Plugin: ACTL_COMMIT finished\n");

		gbIgnoreUpdateTabs = FALSE;
		Plugin()->UpdateConEmuTabs(bForceSendTabs);

		DEBUGSTRCMD(L"Plugin: Tabs updated\n");

		lbRc = true;
	}

	return lbRc;
}

void CPluginBase::cmd_LeftClickSync(LPVOID pCommandData)
{
	BOOL  *pbClickNeed = (BOOL*)pCommandData;
	COORD *crMouse = (COORD *)(pbClickNeed+1);

	// Для Far3 - координаты вроде можно сразу в макрос кинуть
	if (gFarVersion.dwVer >= 3)
	{
		INPUT_RECORD r = {MOUSE_EVENT};
		r.Event.MouseEvent.dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
		r.Event.MouseEvent.dwMousePosition = *crMouse;
		#ifdef _DEBUG
		//r.Event.MouseEvent.dwMousePosition.X = 5;
		#endif

		PostMacro((gFarVersion.dwBuild <= 2850) ? L"MsLClick" : L"Keys('MsLClick')", &r);
	}
	else
	{
		INPUT_RECORD clk[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};
		int i = 0;

		if (*pbClickNeed)
		{
			clk[i].Event.MouseEvent.dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
			clk[i].Event.MouseEvent.dwMousePosition = *crMouse;
			i++;
		}

		clk[i].Event.MouseEvent.dwMousePosition = *crMouse;
		i++;
		DWORD cbWritten = 0;
		HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
		_ASSERTE(h!=INVALID_HANDLE_VALUE && h!=nullptr);
		BOOL fSuccess = WriteConsoleInput(h, clk, 2, &cbWritten);

		if (!fSuccess || cbWritten != 2)
		{
			_ASSERTE(fSuccess && cbWritten==2);
		}
	}
}

void CPluginBase::cmd_CloseQSearch()
{
	if (!gFarVersion.IsFarLua())
		PostMacro(L"$if (Search) Esc $end", nullptr);
	else
		PostMacro(L"if Area.Search Keys(\"Esc\") end", nullptr);
}

void CPluginBase::cmd_EMenu(LPVOID pCommandData)
{
	COORD *crMouse = (COORD *)pCommandData;
	const wchar_t *pszUserMacro = (wchar_t*)(crMouse+1);

	// Т.к. вызов идет через макрос и "rclk_gui:", то настройки emenu трогать нельзя!

	// Иначе в некоторых случаях (Win7 & FAR2x64) не отрисовывается сменившийся курсор
	// В FAR 1.7x это приводит к зачернению экрана??? Решается посылкой
	// "пустого" события движения мышки в консоль сразу после ACTL_KEYMACRO
	Plugin()->RedrawAll();

	const wchar_t* pszMacro = nullptr;

	if (pszUserMacro && *pszUserMacro)
		pszMacro = pszUserMacro;
	else
		pszMacro = gFarVersion.IsFarLua() ? FarRClickMacroDefault3 : FarRClickMacroDefault2; //L"@$If (!CmdLine.Empty) %Flg_Cmd=1; %CmdCurPos=CmdLine.ItemCount-CmdLine.CurPos+1; %CmdVal=CmdLine.Value; Esc $Else %Flg_Cmd=0; $End $Text \"rclk_gui:\" Enter $If (%Flg_Cmd==1) $Text %CmdVal %Flg_Cmd=0; %Num=%CmdCurPos; $While (%Num!=0) %Num=%Num-1; CtrlS $End $End";

	INPUT_RECORD r = {MOUSE_EVENT};
	r.Event.MouseEvent.dwButtonState = FROM_LEFT_1ST_BUTTON_PRESSED;
	r.Event.MouseEvent.dwMousePosition = *crMouse;
	#ifdef _DEBUG
	//r.Event.MouseEvent.dwMousePosition.X = 5;
	#endif

	if (SetFarHookMode)
	{
		// Сказать библиотеке хуков (ConEmuHk.dll), что меню нужно показать в позиции курсора мыши
		gFarMode.bPopupMenuPos = TRUE;
		SetFarHookMode(&gFarMode);
	}

	PostMacro((wchar_t*)pszMacro, &r);
}

bool CPluginBase::cmd_ExternalCallback(LPVOID pCommandData)
{
	bool lbRc = false;

	if (pCommandData
	        && ((SyncExecuteArg*)pCommandData)->nCmd == CMD__EXTERNAL_CALLBACK
	        && ((SyncExecuteArg*)pCommandData)->CallBack != nullptr)
	{
		SyncExecuteArg* pExec = (SyncExecuteArg*)pCommandData;
		BOOL lbCallbackValid = CheckCallbackPtr(pExec->hModule, 1, (FARPROC*)&pExec->CallBack, FALSE, FALSE, FALSE);

		if (lbCallbackValid)
		{
			pExec->CallBack(pExec->lParam);
			lbRc = true;
		}
	}

	return lbRc;
}

void CPluginBase::cmd_ConSetFont(LPVOID pCommandData)
{
	CESERVER_REQ_SETFONT* pFont = (CESERVER_REQ_SETFONT*)pCommandData;

	if (pFont && pFont->cbSize == sizeof(CESERVER_REQ_SETFONT))
	{
		SetConsoleFontSizeTo(GetConEmuHWND(ConEmuWndType::ConsoleWindow), pFont->inSizeY, pFont->inSizeX, pFont->sFontName);
	}
}

void CPluginBase::cmd_GuiChanged(LPVOID pCommandData)
{
	CESERVER_REQ_GUICHANGED *pWindows = (CESERVER_REQ_GUICHANGED*)pCommandData;

	if (gpBgPlugin)
		gpBgPlugin->SetForceThLoad();

	if (pWindows && pWindows->cbSize == sizeof(CESERVER_REQ_GUICHANGED))
	{
		UINT nConEmuSettingsMsg = RegisterWindowMessage(CONEMUMSG_PNLVIEWSETTINGS);

		if (pWindows->hLeftView && IsWindow(pWindows->hLeftView))
		{
			PostMessage(pWindows->hLeftView, nConEmuSettingsMsg, pWindows->nGuiPID, 0);
		}

		if (pWindows->hRightView && IsWindow(pWindows->hRightView))
		{
			PostMessage(pWindows->hRightView, nConEmuSettingsMsg, pWindows->nGuiPID, 0);
		}
	}
}

WARNING("Обязательно сделать возможность отваливаться по таймауту, если плагин не удалось активировать");
// Проверку можно сделать чтением буфера ввода - если там еще есть событие отпускания F11 - значит
// меню плагинов еще загружается. Иначе можно еще чуть-чуть подождать, и отваливаться - активироваться не получится
bool CPluginBase::ProcessCommand(DWORD nCmd, BOOL bReqMainThread, LPVOID pCommandData, CESERVER_REQ** ppResult /*= nullptr*/, bool bForceSendTabs /*= false*/)
{
	bool lbSucceeded = false;
	CESERVER_REQ* pCmdRet = nullptr;

	if (ppResult)  // сначала - сбросить
		*ppResult = nullptr;

	// Некоторые команды можно выполнять в любой нити
	if (nCmd == CMD_SET_CON_FONT || nCmd == CMD_GUICHANGED)
	{
		bReqMainThread = FALSE;
	}

	//Это нужно делать только тогда, когда семафор уже заблокирован!
	//if (gpCmdRet) { free(gpCmdRet); gpCmdRet = nullptr; }
	//gpData = nullptr; gpCursor = nullptr;
	WARNING("Тут нужно сделать проверку содержимого консоли");
	// Если отображено меню - плагин не запустится
	// Не перепутать меню с пустым экраном (Ctrl-O)

	if (bReqMainThread && (gnMainThreadId != GetCurrentThreadId()))
	{
		_ASSERTE(ghPluginSemaphore!=nullptr);
		_ASSERTE(ghServerTerminateEvent!=nullptr);

		// Issue 198: Redraw вызывает отрисовку фаром (1.7x) UserScreen-a (причем без кейбара)
		if (gFarVersion.dwVerMajor < 2 && nCmd == CMD_REDRAWFAR)
		{
			return FALSE; // лучше его просто пропустить
		}

		if (nCmd == CMD_FARPOST)
		{
			return FALSE; // Это просто проверка, что фар отработал цикл
		}

		// Запомним, чтобы знать, были ли созданы данные?
		#ifdef _DEBUG
		CESERVER_REQ* pOldCmdRet = gpCmdRet;
		#endif

		if (/*nCmd == CMD_LEFTCLKSYNC ||*/ nCmd == CMD_CLOSEQSEARCH)
		{
			ResetEvent(ghConsoleWrite);
			gbWaitConsoleWrite = TRUE;
		}

		// Засемафорить, чтобы несколько команд одновременно не пошли...
		{
			HANDLE hEvents[2] = {ghServerTerminateEvent, ghPluginSemaphore};
			DWORD dwWait = WaitForMultipleObjects(2, hEvents, FALSE, INFINITE);

			if (dwWait == WAIT_OBJECT_0)
			{
				// Плагин завершается
				return FALSE;
			}

			if (nCmd == CMD_REDRAWFAR)
				gbNeedBgActivate = TRUE;

			lbSucceeded = ActivatePlugin(nCmd, pCommandData);

			if (lbSucceeded && /*pOldCmdRet !=*/ gpCmdRet)
			{
				pCmdRet = gpCmdRet; // запомнить результат!

				if (ppResult != &gpCmdRet)
					gpCmdRet = nullptr;
			}

			ReleaseSemaphore(ghPluginSemaphore, 1, nullptr);
		}
		// конец семафора

		// The console input hook signals ghConsoleInputEmpty. Without it,
		// waiting here only delays the drag request.
		if ((nCmd == CMD_LEFTCLKSYNC && ghHooksModule) || nCmd == CMD_CLOSEQSEARCH)
		{
			ResetEvent(ghConsoleInputEmpty);
			gbWaitConsoleInputEmpty = TRUE;
			DWORD nWait = WaitForSingleObject(ghConsoleInputEmpty, 2000);

			if (nWait == WAIT_OBJECT_0)
			{
				if (nCmd == CMD_CLOSEQSEARCH)
				{
					// И подождать, пока Фар обработает это событие (то есть до следующего чтения [Peek])
					nWait = WaitForSingleObject(ghConsoleWrite, 1000);
					lbSucceeded = (nWait == WAIT_OBJECT_0);
				}
			}
			else
			{
				#ifdef _DEBUG
				DEBUGSTRMENU((nWait != 0) ? L"*** QUEUE IS NOT EMPTY\n" : L"*** QUEUE IS EMPTY\n");
				#endif
				gbWaitConsoleInputEmpty = FALSE;
				lbSucceeded = (nWait == WAIT_OBJECT_0);
			}
		}

		// Собственно Redraw фар выполнит не тогда, когда его функцию позвали,
		// а когда к нему управление вернется
		if (nCmd == CMD_REDRAWFAR)
		{
			if (!cmd_RedrawFarCall(pCmdRet, ppResult))
			{
				// Плагин завершается
				return false;
			}
		}

		if (ppResult)
		{
			if (ppResult != &gpCmdRet)
			{
				*ppResult = pCmdRet;
			}
		}
		else
		{
			if (pCmdRet && pCmdRet != gpTabs && pCmdRet != gpCmdRet)
			{
				free(pCmdRet);
			}
		}

		//gpReqCommandData = nullptr;
		//gnReqCommand = -1; gnPluginOpenFrom = -1;
		return lbSucceeded; // Результат выполнения команды
	}

	if (gnPluginOpenFrom == 0)
	{
		int iWndType = GetActiveWindowType();
		if (iWndType == wt_Panels)
			gnPluginOpenFrom = of_FilePanel;
		else if (iWndType == wt_Editor)
			gnPluginOpenFrom = of_Editor;
		else if (iWndType == wt_Viewer)
			gnPluginOpenFrom = of_Viewer;
	}

	// Некоторые команды "асинхронные", блокировки не нужны
	if (nCmd == CMD_SET_CON_FONT
		|| nCmd == CMD_GUICHANGED
		)
	{
		switch (nCmd)
		{
		case CMD_SET_CON_FONT:
			cmd_ConSetFont(pCommandData);
			break;

		case CMD_GUICHANGED:
			cmd_GuiChanged(pCommandData);
			break;
		}

		// Ставим и выходим
		if (ghReqCommandEvent)
			SetEvent(ghReqCommandEvent);

		return TRUE;
	}

	MSectionLock CSD; CSD.Lock(csData, TRUE);

	//if (gpCmdRet) { free(gpCmdRet); gpCmdRet = nullptr; } // !!! Освобождается ТОЛЬКО вызывающей функцией!
	gpCmdRet = nullptr; gpData = nullptr; gpCursor = nullptr;

	// Раз дошли сюда - считаем что OK
	lbSucceeded = true;

	switch (nCmd)
	{
	case CMD__EXTERNAL_CALLBACK:
		lbSucceeded = cmd_ExternalCallback(pCommandData);
		break;

	case CMD_DRAGFROM:
		//BOOL  *pbClickNeed = (BOOL*)pCommandData;
		//COORD *crMouse = (COORD *)(pbClickNeed+1);
		//ProcessCommand(CMD_LEFTCLKSYNC, TRUE/*bReqMainThread*/, pCommandData);
		ProcessDragFrom();
		ProcessDragTo();
		break;

	case CMD_DRAGTO:
		ProcessDragTo();
		break;

	case CMD_SETWINDOW:
		cmd_SetWindow(pCommandData, bForceSendTabs);
		pCmdRet = gpTabs;
		break;

	case CMD_POSTMACRO:
		_ASSERTE(pCommandData!=nullptr);
		if (pCommandData!=nullptr)
			PostMacro((wchar_t*)pCommandData, nullptr);
		break;

	case CMD_CLOSEQSEARCH:
		cmd_CloseQSearch();
		break;

	case CMD_LEFTCLKSYNC:
		cmd_LeftClickSync(pCommandData);
		break;
	case CMD_EMENU:  //RMENU
		cmd_EMenu(pCommandData);
		break;

	case CMD_REDRAWFAR:
		// В Far 1.7x были глюки с отрисовкой?
		if (gFarVersion.dwVerMajor>=2)
			RedrawAll();
		break;

	case CMD_CHKRESOURCES:
		CheckResources(true);
		break;

	case CMD_FARPOST:
		// просто сигнализация о том, что фар получил управление.
		lbSucceeded = true;
		break;

	case CMD_OPENEDITORLINE:
		lbSucceeded = true;
		cmd_OpenEditorLine((CESERVER_REQ_FAREDITOR*)pCommandData);
		break;

	default:
		// Неизвестная команда!
		_ASSERTE(FALSE && "Command was not handled!");
		lbSucceeded = false;
	}

	// Функция выполняется в том время, пока заблокирован ghPluginSemaphore,
	// поэтому gpCmdRet можно пользовать
	if (lbSucceeded && !pCmdRet)  // pCmdRet может уже содержать gpTabs
	{
		pCmdRet = gpCmdRet;
		gpCmdRet = nullptr;
	}

	if (ppResult)
	{
		*ppResult = pCmdRet;
	}
	else if (pCmdRet && pCmdRet != gpTabs)
	{
		free(pCmdRet);
	}

	CSD.Unlock();

	_ASSERTE(_CrtCheckMemory());

	if (ghReqCommandEvent)
	{
		SetEvent(ghReqCommandEvent);
	}

	return lbSucceeded;
}

// Изменить размер консоли. Собственно сам ресайз - выполняется сервером!
bool CPluginBase::FarSetConsoleSize(SHORT nNewWidth, SHORT nNewHeight)
{
	bool lbRc = false;

	if (!gdwServerPID)
	{
		_ASSERTE(gdwServerPID!=0);
	}
	else
	{
		CESERVER_REQ In;
		ExecutePrepareCmd(&In, CECMD_SETSIZENOSYNC, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETSIZE));
		memset(&In.SetSize, 0, sizeof(In.SetSize));
		// Для 'far /w' нужно оставить высоту буфера!
		In.SetSize.nBufferHeight = gpFarInfo->bBufferSupport ? -1 : 0;
		In.SetSize.size.X = nNewWidth; In.SetSize.size.Y = nNewHeight;
		DWORD nSrvPID = (gpConMapInfo && gpConMapInfo->nAltServerPID) ? gpConMapInfo->nAltServerPID : gdwServerPID;
		CESERVER_REQ* pOut = ExecuteSrvCmd(nSrvPID, &In, GetConEmuHWND(ConEmuWndType::ConsoleWindow));

		if (pOut)
		{
			ExecuteFreeResult(pOut);
			lbRc = true;
		}

		Plugin()->RedrawAll();
	}

	return lbRc;
}

bool CPluginBase::ReloadFarInfoApi()
{
	if (!mb_StartupInfoOk) return false;

	if (!gpFarInfo)
	{
		_ASSERTE(gpFarInfo!=nullptr);
		return false;
	}

	// Заполнить gpFarInfo->
	//BYTE nFarColors[0x100]; // Массив цветов фара
	//DWORD nFarInterfaceSettings;
	//DWORD nFarPanelSettings;
	//DWORD nFarConfirmationSettings;
	//BOOL  bFarPanelAllowed, bFarLeftPanel, bFarRightPanel;   // FCTL_CHECKPANELSEXIST, FCTL_GETPANELSHORTINFO,...
	//CEFAR_SHORT_PANEL_INFO FarLeftPanel, FarRightPanel;

	DWORD ldwConsoleMode = 0;
	GetConsoleMode(/*ghConIn*/GetStdHandle(STD_INPUT_HANDLE), &ldwConsoleMode);

	#ifdef _DEBUG
	static DWORD ldwDbgMode = 0;
	if (IsDebuggerPresent())
	{
		if (ldwDbgMode != ldwConsoleMode)
		{
			wchar_t szDbg[128]; swprintf_c(szDbg, L"Far.ConEmuW: ConsoleMode(STD_INPUT_HANDLE)=0x%08X\n", ldwConsoleMode);
			OutputDebugStringW(szDbg);
			ldwDbgMode = ldwConsoleMode;
		}
	}
	#endif

	gpFarInfo->nFarConsoleMode = ldwConsoleMode;

	LoadFarColors(gpFarInfo->nFarColors);

	//_ASSERTE(FPS_SHOWCOLUMNTITLES==0x20 && FPS_SHOWSTATUSLINE==0x40); //-V112
	//gpFarInfo->FarInterfaceSettings =
	//    (DWORD)InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETINTERFACESETTINGS, 0, 0);
	//gpFarInfo->nFarPanelSettings =
	//    (DWORD)InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETPANELSETTINGS, 0, 0);
	//gpFarInfo->nFarConfirmationSettings =
	//    (DWORD)InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETCONFIRMATIONS, 0, 0);

	LoadFarSettings(&gpFarInfo->FarInterfaceSettings, &gpFarInfo->FarPanelSettings);

	gpFarInfo->bMacroActive = IsMacroActive();
	int nArea = GetMacroArea();
	if (nArea >= 0)
	{
		if (nArea == ma_Shell || nArea == ma_InfoPanel || nArea == ma_QViewPanel || nArea == ma_TreePanel || nArea == ma_Search)
			gpFarInfo->nMacroArea = fma_Panels;
		else if (nArea == ma_Viewer)
			gpFarInfo->nMacroArea = fma_Viewer;
		else if (nArea == ma_Editor)
			gpFarInfo->nMacroArea = fma_Editor;
		else if (nArea == ma_Dialog || nArea == ma_Disks || nArea == ma_FindFolder || nArea == ma_ShellAutoCompletion
				|| nArea == ma_DialogAutoCompletion || nArea == ma_MainMenu || nArea == ma_Menu || nArea == ma_UserMenu)
			gpFarInfo->nMacroArea = fma_Dialog;
		else
			gpFarInfo->nMacroArea = fma_Unknown;
	}
	else
	{
		gpFarInfo->nMacroArea = fma_Unknown;
	}

	gpFarInfo->bFarPanelAllowed = CheckPanelExist();
	// Far panel APIs must run on its main thread. Keep the last valid snapshot
	// on background refreshes; an old plugin leaves Filled false (safe fallback).
	if (GetCurrentThreadId() == gnMainThreadId)
	{
		CEPanelInfo left = {}, right = {};
		const bool valid = gpFarInfo->bFarPanelAllowed
			&& GetPanelInfo(gpdf_Left, &left) && GetPanelInfo(gpdf_Right, &right);
		gpFarInfo->bFarPanelInfoFilled = valid;
		gpFarInfo->bFarLeftPanel = valid && left.bVisible;
		gpFarInfo->bFarRightPanel = valid && right.bVisible;
		gpFarInfo->FarLeftPanel.Visible = valid && left.bVisible;
		gpFarInfo->FarRightPanel.Visible = valid && right.bVisible;
	}

	// -- пока, во избежание глюков в FAR при неожиданных запросах информации о панелях
	//if (FALSE == (gpFarInfo->bFarPanelAllowed)) {
	//	gpConMapInfo->bFarLeftPanel = FALSE;
	//	gpConMapInfo->bFarRightPanel = FALSE;
	//} else {
	//	PanelInfo piA = {}, piP = {};
	//	BOOL lbActive  = InfoW2800->PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &piA);
	//	BOOL lbPassive = InfoW2800->PanelControl(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, &piP);
	//	if (!lbActive && !lbPassive)
	//	{
	//		gpConMapInfo->bFarLeftPanel = FALSE;
	//		gpConMapInfo->bFarRightPanel = FALSE;
	//	} else {
	//		PanelInfo *ppiL = nullptr;
	//		PanelInfo *ppiR = nullptr;
	//		if (lbActive) {
	//			if (piA.Flags & PFLAGS_PANELLEFT) ppiL = &piA; else ppiR = &piA;
	//		}
	//		if (lbPassive) {
	//			if (piP.Flags & PFLAGS_PANELLEFT) ppiL = &piP; else ppiR = &piP;
	//		}
	//		gpConMapInfo->bFarLeftPanel = ppiL!=nullptr;
	//		gpConMapInfo->bFarRightPanel = ppiR!=nullptr;
	//		if (ppiL) FarPanel2CePanel(ppiL, &(gpConMapInfo->FarLeftPanel));
	//		if (ppiR) FarPanel2CePanel(ppiR, &(gpConMapInfo->FarRightPanel));
	//	}
	//}
	return true;
}

bool CPluginBase::ReloadFarInfo(bool abForce)
{
	if (!gpFarInfoMapping)
	{
		DWORD dwErr = 0;
		// Создать мэппинг для gpFarInfoMapping
		wchar_t szMapName[MAX_PATH];
		swprintf_c(szMapName, CEFARMAPNAME, gnSelfPID);
		DWORD nMapSize = sizeof(CEFAR_INFO_MAPPING);
		TODO("Заменить на MFileMapping");
		ghFarInfoMapping = CreateFileMapping(INVALID_HANDLE_VALUE,
		                                     gpLocalSecurity, PAGE_READWRITE, 0, nMapSize, szMapName);

		if (!ghFarInfoMapping)
		{
			dwErr = GetLastError();
			//TODO("Показать ошибку создания MAP для ghFarInfoMapping");
			_ASSERTE(ghFarInfoMapping!=nullptr);
		}
		else
		{
			gpFarInfoMapping = (CEFAR_INFO_MAPPING*)MapViewOfFile(ghFarInfoMapping, FILE_MAP_ALL_ACCESS,0,0,0);

			if (!gpFarInfoMapping)
			{
				dwErr = GetLastError();
				CloseHandle(ghFarInfoMapping); ghFarInfoMapping = nullptr;
				//TODO("Показать ошибку создания MAP для ghFarInfoMapping");
				_ASSERTE(gpFarInfoMapping!=nullptr);
			}
			else
			{
				gpFarInfoMapping->cbSize = 0;
			}
		}
	}

	if (!ghFarAliveEvent)
	{
		wchar_t szEventName[64] = L"";
		swprintf_c(szEventName, CEFARALIVEEVENT, gnSelfPID);
		ghFarAliveEvent = CreateEvent(gpLocalSecurity, FALSE, FALSE, szEventName);
	}

	if (!gpFarInfo)
	{
		gpFarInfo = (CEFAR_INFO_MAPPING*)calloc(sizeof(CEFAR_INFO_MAPPING),1);

		if (!gpFarInfo)
		{
			_ASSERTE(gpFarInfo!=nullptr);
			return FALSE;
		}

		gpFarInfo->cbSize = sizeof(CEFAR_INFO_MAPPING);
		gpFarInfo->nFarInfoIdx = 0;
		gpFarInfo->FarVer = gFarVersion;
		gpFarInfo->nFarPID = gnSelfPID;
		gpFarInfo->nFarTID = gnMainThreadId;
		gpFarInfo->nProtocolVersion = CESERVER_REQ_VER;

		if (gFarVersion.dwVerMajor < 2 || (gFarVersion.dwVerMajor == 2 && gFarVersion.dwBuild < 1564))
		{
			gpFarInfo->bBufferSupport = FALSE;
		}
		else
		{
			// Нужно проверить
			gpFarInfo->bBufferSupport = Plugin()->CheckBufferEnabled();
		}

		// Загрузить из реестра настройки PanelTabs
		gpFarInfo->PanelTabs.SeparateTabs = gpFarInfo->PanelTabs.ButtonColor = -1;
		LoadPanelTabsFromRegistry();
	}

	bool lbChanged = false, lbSucceeded = false;

	lbSucceeded = ReloadFarInfoApi();

	if (lbSucceeded)
	{
		// Don't compare sActiveDir/sPassiveDir, only nPanelDirIdx take into account
		INT_PTR iCmpLen = ((LPBYTE)gpFarInfoMapping->sActiveDir) - ((LPBYTE)gpFarInfoMapping);
		if (abForce || (memcmp(gpFarInfoMapping, gpFarInfo, iCmpLen) != 0))
		{
			lbChanged = true;
			gpFarInfo->nFarInfoIdx++;
			*gpFarInfoMapping = *gpFarInfo;
		}
	}

	return lbChanged;
}

bool CPluginBase::pcc_Selected(PluginMenuCommands nMenuID)
{
	bool bSelected = false;

	switch (nMenuID)
	{
	case menu_EditConsoleOutput:
		if (ghConEmuWndDC && IsWindow(ghConEmuWndDC))
			bSelected = true;
		break;
	case menu_AttachToConEmu:
		if (!((ghConEmuWndDC && IsWindow(ghConEmuWndDC)) || IsTerminalMode()))
			bSelected = true;
		break;
	case menu_ViewConsoleOutput:
	case menu_SwitchTabVisible:
	case menu_SwitchTabNext:
	case menu_SwitchTabPrev:
	case menu_SwitchTabCommit:
	case menu_ConEmuMacro:
	case menu_StartDebug:
	case menu_ConsoleInfo:
		break;
	}

	return bSelected;
}

bool CPluginBase::pcc_Disabled(PluginMenuCommands nMenuID)
{
	bool bDisabled = false;

	switch (nMenuID)
	{
	case menu_AttachToConEmu:
		if ((ghConEmuWndDC && IsWindow(ghConEmuWndDC)) || IsTerminalMode())
			bDisabled = true;
		break;
	case menu_StartDebug:
		if (IsDebuggerPresent() || IsTerminalMode())
			bDisabled = true;
		break;
	case menu_EditConsoleOutput:
	case menu_ViewConsoleOutput:
	case menu_SwitchTabVisible:
	case menu_SwitchTabNext:
	case menu_SwitchTabPrev:
	case menu_SwitchTabCommit:
	case menu_ConEmuMacro:
		if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
			bDisabled = true;
		break;
	case menu_ConsoleInfo:
		break;
	}

	return bDisabled;
}

void CPluginBase::TouchReadPeekConsoleInputs(int Peek /*= -1*/)
{
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);

	if (!gpFarInfo || !gpFarInfoMapping || !gpConMapInfo)
	{
		_ASSERTE(gpFarInfo);
		return;
	}

	#ifdef _DEBUG
	static DWORD nLastTick;
	const DWORD nCurTick = GetTickCount();
	const DWORD nDelta = nCurTick - nLastTick;
	wchar_t debugInfo[100];
	#endif

	// Во время макросов - считаем, что Фар "думает"
	if (!Plugin()->IsMacroActive())
	{
		SetEvent(ghFarAliveEvent);
		#ifdef _DEBUG
		msprintf(debugInfo, countof(debugInfo), L"FarAlive: true, delay was %u ms", nDelta);
		DEBUGSTRTOUCHINPUT(debugInfo);
		nLastTick = nCurTick;
		#endif
	}
	else
	{
		#ifdef _DEBUG
		msprintf(debugInfo, countof(debugInfo), L"FarAlive: false due to isMacro, delay was %u ms", nDelta);
		DEBUGSTRTOUCHINPUT(debugInfo);
		#endif
	}

	//gpFarInfo->nFarReadIdx++;
	//gpFarInfoMapping->nFarReadIdx = gpFarInfo->nFarReadIdx;

	TouchReadPeekConsoleInputsMark(Peek);
}

void CPluginBase::TouchReadPeekConsoleInputsMark(int Peek /*= -1*/)
{
#ifdef _DEBUG
	if (Peek == -1)
		return;

	if ((GetKeyState(VK_SCROLL)&1) == 0)
		return;

	static DWORD nLastTick;
	DWORD nCurTick = GetTickCount();
	DWORD nDelta = nCurTick - nLastTick;
	static CONSOLE_SCREEN_BUFFER_INFO sbi;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	if (nDelta > 1000)
	{
		GetConsoleScreenBufferInfo(hOut, &sbi);
		nCurTick = nCurTick;
	}

	static wchar_t Chars[] = L"-\\|/-\\|/";
	int nNextChar = 0;

	if (Peek)
	{
		static int nPeekChar = 0;
		nNextChar = nPeekChar++;

		if (nPeekChar >= 8) nPeekChar = 0;
	}
	else
	{
		static int nReadChar = 0;
		nNextChar = nReadChar++;

		if (nReadChar >= 8) nReadChar = 0;
	}

	CHAR_INFO chi;
	chi.Char.UnicodeChar = Chars[nNextChar];
	chi.Attributes = 15;
	COORD crBufSize = {1,1};
	COORD crBufCoord = {0,0};
	// Cell[0] лучше не трогать - GUI ориентируется на наличие "1" в этой ячейке при проверке активности фара
	SHORT nShift = (Peek?1:2);
	SMALL_RECT rc = {sbi.srWindow.Left+nShift,sbi.srWindow.Bottom,sbi.srWindow.Left+nShift,sbi.srWindow.Bottom};
	WriteConsoleOutputW(hOut, &chi, crBufSize, crBufCoord, &rc);
#endif
}

// Вызывается только в основной нити
// и ТОЛЬКО если фар считывает один (1) INPUT_RECORD
void CPluginBase::OnConsolePeekReadInput(bool abPeek)
{
	#ifdef _DEBUG
	DWORD nCurTID = GetCurrentThreadId();
	DWORD nCurMainTID = gnMainThreadId;
	if (nCurTID != nCurMainTID)
	{
		HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, nCurMainTID);
		if (hThread) SuspendThread(hThread);  // -V720
		_ASSERTE(nCurTID == nCurMainTID);
		if (hThread) { ResumeThread(hThread); CloseHandle(hThread); }
	}
	#endif

	bool lbNeedSynchro = false;

	// Для того, чтобы WaitPluginActivation знал, живой фар, или не очень...
	gnPeekReadCount++;

	if (gpConMapInfo && gpFarInfo && gpFarInfoMapping)
		TouchReadPeekConsoleInputs(abPeek ? 1 : 0);

	if (/*gbNeedReloadFarInfo &&*/ abPeek == FALSE)
	{
		//gbNeedReloadFarInfo = FALSE;
		bool bNeedReload = false;

		if (gpConMapInfo)
		{
			DWORD nMapPID = gpConMapInfo->nActiveFarPID;
			static DWORD dwLastTickCount = 0;

			if (nMapPID == 0 || nMapPID != gnSelfPID)
			{
				//Выполнить команду в главном сервере, альтернативный не имеет права писать в мэппинг
				bNeedReload = true;
				dwLastTickCount = GetTickCount();
				CESERVER_REQ_HDR in;
				ExecutePrepareCmd(&in, CECMD_SETFARPID, sizeof(CESERVER_REQ_HDR));
				WARNING("Overhead and hung possibility");
				// Если ActiveServerPID() возвращает PID самого фара (current AlternativeServer) - то это overhead,
				// т.к. альт.сервер крутится в ЭТОМ же потоке, и его можно "позвать" напрямую.
				// Но дергать здесь ExecuteSrvCmd(gpConMapInfo->nServerPID) - НЕЛЬЗЯ, т.к.
				// в этом случае будет рассинхронизация серверных потоков этого процесса,
				// в итоге nActiveFarPID может никогда не обновиться...
				// Возможность подвисания - это если в nAltServerPID будет "зависший" или закрывающийся процесс (не мы).
				CESERVER_REQ *pOut = ExecuteSrvCmd(gpConMapInfo->ActiveServerPID(), (CESERVER_REQ*)&in, FarHwnd);
				if (pOut)
					ExecuteFreeResult(pOut);
			}
			else
			{
				DWORD dwCurTick = GetTickCount();

				if ((dwCurTick - dwLastTickCount) >= CHECK_FARINFO_INTERVAL)
				{
					bNeedReload = true;
					dwLastTickCount = dwCurTick;
				}
			}
		}

		if (bNeedReload)
		{
			//ReloadFarInfo(FALSE);
			gbNeedPostReloadFarInfo = TRUE;
		}
	}

	if (gbNeedPostReloadFarInfo || gbNeedReloadFarDir || gbNeedPostEditCheck
		|| gbRequestUpdateTabs || gbNeedPostTabSend || gbNeedBgActivate)
	{
		const DWORD delay = (gnSynchroThrottleTick == 0) ? gnSynchroThrottleDelay
			: (GetTickCount() - gnSynchroThrottleTick);
		lbNeedSynchro = (delay >= gnSynchroThrottleDelay);
	}

	// В некоторых случаях (CMD_LEFTCLKSYNC,CMD_CLOSEQSEARCH,...) нужно дождаться, пока очередь опустеет
	if (gbWaitConsoleInputEmpty)
	{
		DWORD nTestEvents = 0;
		HANDLE h = GetStdHandle(STD_INPUT_HANDLE);

		if (GetNumberOfConsoleInputEvents(h, &nTestEvents))
		{
			if (nTestEvents == 0)
			{
				gbWaitConsoleInputEmpty = FALSE;
				SetEvent(ghConsoleInputEmpty);
			}
		}
	}

	if (IS_SYNCHRO_ALLOWED)
	{
		// Требуется дернуть Synchro, чтобы корректно активироваться
		if (lbNeedSynchro && !gbInputSynchroPending)
		{
			gbInputSynchroPending = true;
			gnSynchroThrottleTick = GetTickCount();
			Plugin()->ExecuteSynchro();
		}
	}
	else
	{
		// Для Far1 зовем сразу
		_ASSERTE(gFarVersion.dwVerMajor == 1);
		Plugin()->OnMainThreadActivated();
	}
}

#ifdef _DEBUG
bool CPluginBase::DebugGetKeyboardState(LPBYTE pKeyStates)
{
	short v = 0;
	BYTE b = 0;
	int nKeys[] = {VK_SHIFT,VK_LSHIFT,VK_RSHIFT,
	               VK_MENU,VK_LMENU,VK_RMENU,
	               VK_CONTROL,VK_LCONTROL,VK_RCONTROL,
	               VK_LWIN,VK_RWIN,
	               VK_CAPITAL,VK_NUMLOCK,VK_SCROLL
	              };
	int nKeyCount = sizeof(nKeys)/sizeof(nKeys[0]);

	for (int i=0; i<nKeyCount; i++)
	{
		v = GetAsyncKeyState(nKeys[i]);
		b = v & 1;

		if ((v & 0x8000) == 0x8000)
			b |= 0x80;

		pKeyStates[nKeys[i]] = b;
	}

	return true;
}
#endif

#ifdef _DEBUG
typedef BOOL (__stdcall *FGetConsoleKeyboardLayoutName)(wchar_t*);
FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = nullptr;
DWORD CPluginBase::DebugCheckKeyboardLayout()
{
	DWORD dwLayout = 0x04090409;

	if (!pfnGetConsoleKeyboardLayoutName)
		pfnGetConsoleKeyboardLayoutName = (FGetConsoleKeyboardLayoutName)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetConsoleKeyboardLayoutNameW");

	if (pfnGetConsoleKeyboardLayoutName)
	{
		wchar_t szCurKeybLayout[KL_NAMELENGTH+1];

		if (pfnGetConsoleKeyboardLayoutName(szCurKeybLayout))
		{
			wchar_t *pszEnd = szCurKeybLayout+8;
			dwLayout = wcstoul(szCurKeybLayout, &pszEnd, 16);
		}
	}

	return dwLayout;
}
#endif

#ifdef _DEBUG
void __INPUT_RECORD_Dump(INPUT_RECORD *rec, wchar_t* pszRecord);
void CPluginBase::DebugInputPrint(INPUT_RECORD r)
{
	static wchar_t szDbg[1100]; szDbg[0] = 0;
	SYSTEMTIME st; GetLocalTime(&st);
	swprintf_c(szDbg, L"%02i:%02i:%02i.%03i ", st.wHour,st.wMinute,st.wSecond,st.wMilliseconds);
	__INPUT_RECORD_Dump(&r, szDbg+13);
	lstrcatW(szDbg, L"\n");
	DEBUGSTR(szDbg);
}
#endif

bool CPluginBase::UngetDummyMouseEvent(bool abRead, HookCallbackArg* pArgs)
{
	if (!(pArgs->lArguments[1] && pArgs->lArguments[2] && pArgs->lArguments[3]))
	{
		_ASSERTE(pArgs->lArguments[1] && pArgs->lArguments[2] && pArgs->lArguments[3]);
	}
	else if ((gLastMouseReadEvent.dwButtonState & (RIGHTMOST_BUTTON_PRESSED|FROM_LEFT_1ST_BUTTON_PRESSED))
			|| (gnDummyMouseEventFromMacro > 0))
	{
		// Такой финт нужен только в случае:
		// в редакторе идет скролл мышкой (скролл - зажатой кнопкой на заголовке/кейбаре)
		// нужно заставить фар остановить скролл, иначе активация Synchro невозможна

		// Или второй случай
		//FAR BUGBUG: Макрос не запускается на исполнение, пока мышкой не дернем :(
		//  Это чаще всего проявляется при вызове меню по RClick
		//  Если курсор на другой панели, то RClick сразу по пассивной
		//  не вызывает отрисовку :(

		if ((gnAllowDummyMouseEvent < 1) && (gnDummyMouseEventFromMacro < 1))
		{
			_ASSERTE(gnAllowDummyMouseEvent >= 1);
			if (gnAllowDummyMouseEvent < 0)
				gnAllowDummyMouseEvent = 0;
			gbUngetDummyMouseEvent = FALSE;
			return false;
		}

		// Сообщить в GUI что мы "пустое" сообщение фару кидаем
		if (gFarMode.bFarHookMode && gFarMode.bMonitorConsoleInput)
		{
			CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_PEEKREADINFO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_PEEKREADINFO));
			if (pIn)
			{
				pIn->PeekReadInfo.nCount = (WORD)1;
				pIn->PeekReadInfo.cPeekRead = '*';
				pIn->PeekReadInfo.cUnicode = 'U';
				pIn->PeekReadInfo.h = (HANDLE)pArgs->lArguments[1];
				pIn->PeekReadInfo.nTID = GetCurrentThreadId();
				pIn->PeekReadInfo.nPID = GetCurrentProcessId();
				pIn->PeekReadInfo.bMainThread = (pIn->PeekReadInfo.nTID == gnMainThreadId);

				pIn->PeekReadInfo.Buffer->EventType = MOUSE_EVENT;
				pIn->PeekReadInfo.Buffer->Event.MouseEvent = gLastMouseReadEvent;
				pIn->PeekReadInfo.Buffer->Event.MouseEvent.dwButtonState = 0;
				pIn->PeekReadInfo.Buffer->Event.MouseEvent.dwEventFlags = MOUSE_MOVED;

				CESERVER_REQ* pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
				if (pOut) ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
		}

		PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
		LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
		*pCount = 1;
		p->EventType = MOUSE_EVENT;
		p->Event.MouseEvent = gLastMouseReadEvent;
		p->Event.MouseEvent.dwButtonState = 0;
		p->Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
		*((LPBOOL)pArgs->lpResult) = TRUE;

		if ((gnDummyMouseEventFromMacro > 0) && abRead)
		{
			TODO("А если в очередь фара закинуто несколько макросов? По одному мышиному события выполнится только один, или все?");
			//InterlockedDecrement(&gnDummyMouseEventFromMacro);
			gnDummyMouseEventFromMacro = 0;
		}

		return true;
	}
	else
	{
		gbUngetDummyMouseEvent = FALSE; // Не требуется, фар сам кнопку "отпустил"
	}

	return false;
}

void CPluginBase::FillLoadedParm(struct ConEmuLoadedArg* pArg, HMODULE hSubPlugin, BOOL abLoaded)
{
	memset(pArg, 0, sizeof(struct ConEmuLoadedArg));
	pArg->cbSize = (DWORD)sizeof(struct ConEmuLoadedArg);
	//#define D(N) (1##N-100)
	// nBuildNo в формате YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	pArg->nBuildNo = ((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10);
	pArg->hConEmu = ghPluginModule;
	pArg->hPlugin = hSubPlugin;
	pArg->bLoaded = abLoaded;
	pArg->bGuiActive = abLoaded && (ghConEmuWndDC != nullptr);

	// Сервисные функции
	if (abLoaded)
	{
		pArg->GetFarHWND = GetFarHWND;
		pArg->GetFarHWND2 = GetFarHWND2;
		pArg->GetFarVersion = GetFarVersion;
		pArg->IsTerminalMode = IsTerminalMode;
		pArg->IsConsoleActive = IsConsoleActive;
		pArg->RegisterPanelView = RegisterPanelView;
		pArg->RegisterBackground = RegisterBackground;
		pArg->ActivateConsole = ActivateConsole;
		pArg->SyncExecute = SyncExecute;
	}
}

void CPluginBase::NotifyConEmuUnloaded()
{
	OnConEmuLoaded_t fnOnConEmuLoaded = nullptr;
	BOOL lbSucceeded = FALSE;
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);

	if (snapshot != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 module = {sizeof(MODULEENTRY32)};

		for (BOOL res = Module32First(snapshot, &module); res; res = Module32Next(snapshot, &module))
		{
			if ((fnOnConEmuLoaded = (OnConEmuLoaded_t)GetProcAddress(module.hModule, "OnConEmuLoaded")) != nullptr)
			{
				// Наверное, только для плагинов фара
				if (GetProcAddress(module.hModule, "SetStartupInfoW") || GetProcAddress(module.hModule, "SetStartupInfo"))
				{
					struct ConEmuLoadedArg arg = {sizeof(struct ConEmuLoadedArg)};
					FillLoadedParm(&arg, module.hModule, FALSE); // плагин conemu.dll выгружается!
					//arg.hPlugin = module.hModule;
					//arg.nBuildNo = ((MVV_1 % 100)*10000) + (MVV_2*100) + (MVV_3);
					//arg.hConEmu = ghPluginModule;
					//arg.bLoaded = FALSE;
					lbSucceeded = FALSE;
					SAFETRY
					{
						fnOnConEmuLoaded(&arg);
						lbSucceeded = TRUE;
					} SAFECATCH
					{
						// Failed
						_ASSERTE(lbSucceeded == TRUE);
					}
				}
			}
		}

		CloseHandle(snapshot);
	}
}

bool CPluginBase::OnPanelViewCallbacks(HookCallbackArg* pArgs, PanelViewInputCallback pfnLeft, PanelViewInputCallback pfnRight)
{
	if (!pArgs->bMainThread || !(pfnLeft || pfnRight))
	{
		_ASSERTE(pArgs->bMainThread && (pfnLeft || pfnRight));
		return true; // перехват делаем только в основной нити
	}

	BOOL lbNewResult = FALSE, lbContinue = TRUE;
	HANDLE hInput = (HANDLE)(pArgs->lArguments[0]);
	PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
	DWORD nBufSize = (DWORD)(pArgs->lArguments[2]);
	LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);

	if (lbContinue && pfnLeft)
	{
		_ASSERTE(gPanelRegLeft.bRegister);
		lbContinue = pfnLeft(hInput,p,nBufSize,pCount,&lbNewResult);

		if (!lbContinue)
			*((BOOL*)pArgs->lpResult) = lbNewResult;
	}

	// Если есть только правая панель, или на правой панели задана другая функция
	if (lbContinue && pfnRight && pfnRight != pfnLeft)
	{
		_ASSERTE(gPanelRegRight.bRegister);
		lbContinue = pfnRight(hInput,p,nBufSize,pCount,&lbNewResult);

		if (!lbContinue)
			*((BOOL*)pArgs->lpResult) = lbNewResult;
	}

	return (lbContinue != FALSE);
}


BOOL /*WINAPI*/ CPluginBase::OnShellExecuteExW_Except(HookCallbackArg* pArgs)
{
	if (pArgs->bMainThread)
	{
		Plugin()->ShowMessage(CEShellExecuteException,0);
	}

	*((LPBOOL*)pArgs->lpResult) = FALSE;
	SetLastError(E_UNEXPECTED);

	return TRUE; // Result is ignored in this callback type
}

BOOL CPluginBase::OnWaitForMultipleObjects(HookCallbackArg* pArgs)
{
	// Far 3 waits for (conIn, event) up to end of the minute
	if (pArgs->bMainThread && pArgs->lArguments[0] == 2)
	{
		const DWORD ticks = GetTicks();
		const DWORD ttl = ticks + static_cast<DWORD>(pArgs->lArguments[3]);
		if (gpFarInfo && gpFarInfoMapping)
		{
			gpFarInfo->activeTtl = ttl;
			gpFarInfoMapping->activeTtl = ttl;
		}
	}

	return TRUE;
}

BOOL CPluginBase::OnWaitForMultipleObjectsPost(HookCallbackArg* pArgs)
{
	if (pArgs->bMainThread && pArgs->lArguments[0] == 2)
	{
		if (gpFarInfo && gpFarInfoMapping)
		{
			gpFarInfo->activeTtl = 0;
			gpFarInfoMapping->activeTtl = 0;
		}
	}

	return TRUE;
}

// Used for detection of Far "aliveness"
BOOL /*WINAPI*/ CPluginBase::OnGetNumberOfConsoleInputEventsPost(HookCallbackArg* pArgs)
{
	if (pArgs->bMainThread && gpConMapInfo && gpFarInfo && gpFarInfoMapping)
	{
		TouchReadPeekConsoleInputs(-1);
	}

	return FALSE; // Result is ignored in this callback type
}

// Если функция возвращает FALSE - реальный ReadConsoleInput вызван не будет,
// и в вызывающую функцию (ФАРа?) вернется то, что проставлено в pArgs->lpResult & pArgs->lArguments[...]
BOOL /*WINAPI*/ CPluginBase::OnConsolePeekInput(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread)
		return TRUE;  // обработку делаем только в основной нити

	if (gbUngetDummyMouseEvent)
	{
		if (CPluginBase::UngetDummyMouseEvent(FALSE, pArgs))
			return FALSE; // реальный ReadConsoleInput вызван не будет
	}

	//// Выставить флажок "Жив" можно и при вызове из плагина
	//if (gpConMapInfo && gpFarInfo && gpFarInfoMapping)
	//	TouchReadPeekConsoleInputs(1);

	//if (pArgs->IsExecutable != HEO_Executable)
	//	return TRUE;  // и только при вызове из far.exe

	if (pArgs->lArguments[2] == 1)
	{
		OnConsolePeekReadInput(TRUE/*abPeek*/);
	}

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnPeekPreCall || gPanelRegRight.pfnPeekPreCall)
	{
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnPeekPreCall, gPanelRegRight.pfnPeekPreCall))
			return FALSE;
	}

	return TRUE; // продолжить
}

BOOL /*WINAPI*/ CPluginBase::OnConsolePeekInputPost(HookCallbackArg* pArgs)
{
	// Do work in main thread only!
	if (!pArgs->bMainThread)
		return FALSE; // Result is ignored in this callback type

#ifdef _DEBUG

	if (*(LPDWORD)(pArgs->lArguments[3]))
	{
		wchar_t szDbg[255];
		PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
		LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
		DWORD nLeft = 0; GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &nLeft);
		swprintf_c(szDbg, L"*** OnConsolePeekInputPost(Events=%i, KeyCount=%i, LeftInConBuffer=%i)\n",
		          *pCount, (p->EventType==KEY_EVENT) ? p->Event.KeyEvent.wRepeatCount : 0, nLeft);
		DEBUGSTRINPUT(szDbg);

		// Если под дебагом включен ScrollLock - вывести информацию о считанных событиях
		if (GetKeyState(VK_SCROLL) & 1)
		{
			PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
			LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
			_ASSERTE(*pCount <= pArgs->lArguments[2]);
			UINT nCount = *pCount;

			for(UINT i = 0; i < nCount; i++)
				DebugInputPrint(p[i]);
		}
	}

#endif

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnPeekPostCall || gPanelRegRight.pfnPeekPostCall)
	{
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnPeekPostCall, gPanelRegRight.pfnPeekPostCall))
			return FALSE; // Result is ignored in this callback type
	}

	return FALSE; // Result is ignored in this callback type
}

// Если функция возвращает FALSE - реальный ReadConsoleInput вызван не будет,
// и в вызывающую функцию (ФАРа?) вернется то, что проставлено в pArgs->lpResult & pArgs->lArguments[...]
BOOL CPluginBase::OnConsoleReadInputWork(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread)
		return TRUE;  // обработку делаем только в основной нити

	if (gbUngetDummyMouseEvent)
	{
		if (CPluginBase::UngetDummyMouseEvent(TRUE, pArgs))
		{
			gbUngetDummyMouseEvent = FALSE;
			gLastMouseReadEvent.dwButtonState = 0; // будем считать, что "мышиную блокировку" успешно сняли
			return FALSE; // реальный ReadConsoleInput вызван не будет
		}
		_ASSERTE(gbUngetDummyMouseEvent == FALSE);
	}

	//// Выставить флажок "Жив" можно и при вызове из плагина
	//if (gpConMapInfo && gpFarInfo && gpFarInfoMapping)
	//	TouchReadPeekConsoleInputs(0);
	//
	//if (pArgs->IsExecutable != HEO_Executable)
	//	return TRUE;  // и только при вызове из far.exe

	if (pArgs->lArguments[2] == 1)
	{
		OnConsolePeekReadInput(FALSE/*abPeek*/);
	}

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnReadPreCall || gPanelRegRight.pfnReadPreCall)
	{
		// Если функция возвращает FALSE - реальное чтение не будет вызвано
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnReadPreCall, gPanelRegRight.pfnReadPreCall))
		{
			// это вызвается перед реальным чтением - информация может быть разве что от "PanelViews"
			// Если под дебагом включен ScrollLock - вывести информацию о считанных событиях
			#ifdef _DEBUG
			if (GetKeyState(VK_SCROLL) & 1)
			{
				PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
				LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
				_ASSERTE(*pCount <= pArgs->lArguments[2]);
				UINT nCount = *pCount;

				for (UINT i = 0; i < nCount; i++)
					DebugInputPrint(p[i]);
			}
			#endif

			return FALSE;
		}
	}

	return TRUE; // продолжить
}

BOOL /*WINAPI*/ CPluginBase::OnConsoleReadInput(HookCallbackArg* pArgs)
{
	return OnConsoleReadInputWork(pArgs);
}

BOOL /*WINAPI*/ CPluginBase::OnConsoleReadInputPost(HookCallbackArg* pArgs)
{
	// Do work in main thread only!
	if (!pArgs->bMainThread)
		return FALSE; // Result is ignored in this callback type

#ifdef _DEBUG
	{
		wchar_t szDbg[255];
		PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
		LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
		DWORD nLeft = 0; GetNumberOfConsoleInputEvents(GetStdHandle(STD_INPUT_HANDLE), &nLeft);
		swprintf_c(szDbg, L"*** OnConsoleReadInputPost(Events=%i, KeyCount=%i, LeftInConBuffer=%i)\n",
		          *pCount, (p->EventType==KEY_EVENT) ? p->Event.KeyEvent.wRepeatCount : 0, nLeft);
		//if (*pCount) {
		//	wsprintfW(szDbg+lstrlen(szDbg), L", type=%i", p->EventType);
		//	if (p->EventType == MOUSE_EVENT) {
		//		wsprintfW(L", {%ix%i} BtnState:0x%08X, CtrlState:0x%08X, Flags:0x%08X",
		//			p->Event.MouseEvent.dwMousePosition.X, p->Event.MouseEvent.dwMousePosition.Y,
		//			p->Event.MouseEvent.dwButtonState, p->Event.MouseEvent.dwControlKeyState,
		//			p->Event.MouseEvent.dwEventFlags);
		//	} else if (p->EventType == KEY_EVENT) {
		//		wsprintfW(L", '%c' %s count=%i, VK=%i, SC=%i, CH=\\x%X, State=0x%08x %s",
		//			(p->Event.KeyEvent.uChar.UnicodeChar > 0x100) ? L'?' :
		//			(p->Event.KeyEvent.uChar.UnicodeChar
		//			? p->Event.KeyEvent.uChar.UnicodeChar : L' '),
		//			p->Event.KeyEvent.bKeyDown ? L"Down," : L"Up,  ",
		//			p->Event.KeyEvent.wRepeatCount,
		//			p->Event.KeyEvent.wVirtualKeyCode,
		//			p->Event.KeyEvent.wVirtualScanCode,
		//			p->Event.KeyEvent.uChar.UnicodeChar,
		//			p->Event.KeyEvent.dwControlKeyState,
		//			(p->Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) ?
		//			L"<Enhanced>" : L"");
		//	}
		//}
		//lstrcatW(szDbg, L")\n");
		DEBUGSTRINPUT(szDbg);

		// Если под дебагом включен ScrollLock - вывести информацию о считанных событиях
		if (GetKeyState(VK_SCROLL) & 1)
		{
			PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
			LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);
			_ASSERTE(*pCount <= pArgs->lArguments[2]);
			UINT nCount = *pCount;

			for(UINT i = 0; i < nCount; i++)
				DebugInputPrint(p[i]);
		}
	}
#endif

	HANDLE h = (HANDLE)(pArgs->lArguments[0]);
	PINPUT_RECORD p = (PINPUT_RECORD)(pArgs->lArguments[1]);
	LPDWORD pCount = (LPDWORD)(pArgs->lArguments[3]);

	//Чтобы не было зависаний при попытке активации плагина во время прокрутки
	//редактора, в плагине мониторить нажатие мыши. Если последнее МЫШИНОЕ событие
	//было с нажатой кнопкой - сначала пульнуть в консоль команду "отпускания" кнопки,
	//и только после этого - пытаться активироваться.
	if (pCount && *pCount)
	{
		for (int i = (*pCount) - 1; i >= 0; i--)
		{
			if (p[i].EventType == MOUSE_EVENT)
			{
				gLastMouseReadEvent = p[i].Event.MouseEvent;
				break;
			}
		}
	}

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnReadPostCall || gPanelRegRight.pfnReadPostCall)
	{
		if (!OnPanelViewCallbacks(pArgs, gPanelRegLeft.pfnReadPostCall, gPanelRegRight.pfnReadPostCall))
			return FALSE; // Result is ignored in this callback type
	}

	// Чтобы ФАР сразу прекратил ходить по каталогам при отпускании Enter
	if (h != nullptr)
	{
		if (*pCount == 1 && p->EventType == KEY_EVENT && p->Event.KeyEvent.bKeyDown
		        && (p->Event.KeyEvent.wVirtualKeyCode == VK_RETURN
		            || p->Event.KeyEvent.wVirtualKeyCode == VK_NEXT
		            || p->Event.KeyEvent.wVirtualKeyCode == VK_PRIOR)
		  )
		{
			INPUT_RECORD ir[10]; DWORD nRead = 0, nInc = 0;

			if (PeekConsoleInputW(h, ir, countof(ir), &nRead) && nRead)
			{
				for(DWORD n = 0; n < nRead; n++)
				{
					if (ir[n].EventType == KEY_EVENT && ir[n].Event.KeyEvent.bKeyDown
					        && ir[n].Event.KeyEvent.wVirtualKeyCode == p->Event.KeyEvent.wVirtualKeyCode
					        && ir[n].Event.KeyEvent.dwControlKeyState == p->Event.KeyEvent.dwControlKeyState)
					{
						nInc++;
					}
					else
					{
						break; // дубли в буфере кончились
					}
				}

				if (nInc > 0)
				{
					if (ReadConsoleInputW(h, ir, nInc, &nRead) && nRead)
					{
						p->Event.KeyEvent.wRepeatCount += (WORD)nRead;
					}
				}
			}
		}
	}

	return FALSE; // Result is ignored in this callback type
}

BOOL /*WINAPI*/ CPluginBase::OnWriteConsoleOutput(HookCallbackArg* pArgs)
{
	if (!pArgs->bMainThread)
		return TRUE;  // обработку делаем только в основной нити
	//if (pArgs->IsExecutable != HEO_Executable)
	//	return TRUE;  // и только при вызове из far.exe

	// Если зарегистрирован callback для графической панели
	if (gPanelRegLeft.pfnWriteCall || gPanelRegRight.pfnWriteCall)
	{
		HANDLE hOutput = (HANDLE)(pArgs->lArguments[0]);
		const CHAR_INFO *lpBuffer = (const CHAR_INFO *)(pArgs->lArguments[1]);
		COORD dwBufferSize = *(COORD*)(pArgs->lArguments[2]);
		COORD dwBufferCoord = *(COORD*)(pArgs->lArguments[3]);
		PSMALL_RECT lpWriteRegion = (PSMALL_RECT)(pArgs->lArguments[4]);

		if (gPanelRegLeft.pfnWriteCall)
		{
			_ASSERTE(gPanelRegLeft.bRegister);
			gPanelRegLeft.pfnWriteCall(hOutput,lpBuffer,dwBufferSize,dwBufferCoord,lpWriteRegion);
		}

		// Если есть только правая панель, или на правой панели задана другая функция
		if (gPanelRegRight.pfnWriteCall && gPanelRegRight.pfnWriteCall != gPanelRegLeft.pfnWriteCall)
		{
			_ASSERTE(gPanelRegRight.bRegister);
			gPanelRegRight.pfnWriteCall(hOutput,lpBuffer,dwBufferSize,dwBufferCoord,lpWriteRegion);
		}
	}

	//if (gpBgPlugin)
	//	gpBgPlugin->SetForceCheck();

	if (gbWaitConsoleWrite)
	{
		gbWaitConsoleWrite = FALSE;
		SetEvent(ghConsoleWrite);
	}

	return TRUE;
}

BOOL /*WINAPI*/ CPluginBase::OnPostWriteConsoleOutput(HookCallbackArg* pArgs)
{
	// After output to the console (Commit call) trigger reloading of Far information
	// Especially the Panel Directories

	gbNeedReloadFarDir = TRUE;

	return TRUE;
}

BOOL /*WINAPI*/ CPluginBase::OnConsoleDetaching(HookCallbackArg* pArgs)
{
	if (ghMonitorThread)
	{
		SuspendThread(ghMonitorThread);  // -V720
		// ResumeThread выполняется в конце OnConsoleWasAttached
	}

	// Выполним сразу после SuspendThread, чтобы нить не посчитала, что мы подцепились обратно
	gbWasDetached = (ghConEmuWndDC!=nullptr && IsWindow(ghConEmuWndDC));

	if (ghConEmuWndDC)
	{
		// Запомним, для удобства аттача
		if (!GetWindowThreadProcessId(ghConEmuWndDC, &gdwPreDetachGuiPID))
			gdwPreDetachGuiPID = 0;
	}

	if (gbWasDetached)
	{
		HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(hOutput, &gsbiDetached);

		// Нужно уведомить ТЕКУЩИЙ сервер, что закрываться по окончании команды не нужно
		if (gdwServerPID == 0)
		{
			_ASSERTE(gdwServerPID != 0);
		}
		else
		{
			CESERVER_REQ In = {};
			CESERVER_REQ* pOut = nullptr;
			ExecutePrepareCmd(&In, CECMD_FARDETACHED, sizeof(CESERVER_REQ_HDR));
			pOut = ExecuteSrvCmd(gdwServerPID, &In, FarHwnd);

			if (pOut) ExecuteFreeResult(pOut);
		}
	}

	// -- теперь мэппинги создает GUI
	//CloseColorerHeader(); // Если было

	CloseMapHeader();
	ghConEmuWndDC = nullptr;
	SetConEmuEnvVar(nullptr);
	SetConEmuEnvVarChild(nullptr,nullptr);
	// Потом еще и FarHwnd сбросить нужно будет... Ну этим MonitorThreadProcW займется
	return TRUE; // продолжить выполнение функции
}

// Функции вызываются в основной нити, вполне можно дергать FAR-API
BOOL /*WINAPI*/ CPluginBase::OnConsoleWasAttached(HookCallbackArg* pArgs)
{
	FarHwnd = GetConEmuHWND(ConEmuWndType::ConsoleWindow);
	gbFarWndVisible = IsWindowVisible(FarHwnd);

	if (gbWasDetached)
	{
		// Сразу спрятать окошко
		//apiShowWindow(FarHwnd, SW_HIDE);
	}

	// -- теперь мэппинги создает GUI
	//// Если ранее были созданы мэппинги для цвета - пересоздать
	//CreateColorerHeader();

	if (gbWasDetached)
	{
		// сразу переподцепимся к GUI
		if (!Plugin()->Attach2Gui())
		{
			EmergencyShow();
		}

		// Сбрасываем после Attach2Gui, чтобы MonitorThreadProcW случайно
		// не среагировал раньше времени
		gbWasDetached = FALSE;
	}

	if (ghMonitorThread)
		ResumeThread(ghMonitorThread);

	return TRUE; // Result is ignored in this callback type
}


//#define CREATEEVENT(fmt,h)
//		swprintf_c(szEventName, fmt, dwCurProcId );
//		h = CreateEvent(nullptr, FALSE, FALSE, szEventName);
//		if (h==INVALID_HANDLE_VALUE) h=nullptr;

VOID /*WINAPI*/ CPluginBase::OnCurDirChanged()
{
	CPluginBase* p = Plugin();
	if (p && (gnCurrentWindowType == p->wt_Panels) && (IS_SYNCHRO_ALLOWED))
	{
		// Требуется дернуть Synchro, чтобы корректно активироваться
		if (!gbInputSynchroPending)
		{
			gbInputSynchroPending = true;
			p->ExecuteSynchro();
		}
	}
}

// Определены в SetHook.h
//typedef void (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
//extern OnLibraryLoaded_t gfOnLibraryLoaded;

// Вызывается при загрузке dll
void /*WINAPI*/ CPluginBase::OnLibraryLoaded(HMODULE ahModule)
{
	WARNING("Проверить, чтобы после новых хуков это два раза на один модуль не вызывалось");

	//#ifdef _DEBUG
	//wchar_t szModulePath[MAX_PATH]; szModulePath[0] = 0;
	//GetModuleFileName(ahModule, szModulePath, MAX_PATH);
	//#endif

	//// Если GUI неактивно (запущен standalone FAR) - сразу выйти
	//if (ghConEmuWndDC == nullptr)
	//{
	//	return;
	//}
	WARNING("Нужно специально вызвать OnLibraryLoaded при аттаче к GUI");
	// Если определен калбэк инициализации ConEmu
	OnConEmuLoaded_t fnOnConEmuLoaded = nullptr;
	BOOL lbSucceeded = FALSE;

	if ((fnOnConEmuLoaded = (OnConEmuLoaded_t)GetProcAddress(ahModule, "OnConEmuLoaded")) != nullptr)
	{
		// Наверное, только для плагинов фара
		if (GetProcAddress(ahModule, "SetStartupInfoW") || GetProcAddress(ahModule, "SetStartupInfo"))
		{
			struct ConEmuLoadedArg arg; // = {sizeof(struct ConEmuLoadedArg)};
			FillLoadedParm(&arg, ahModule, TRUE);
			//arg.hPlugin = ahModule;
			//arg.hConEmu = ghPluginModule;
			//arg.hPlugin = ahModule;
			//arg.bLoaded = TRUE;
			//arg.bGuiActive = (ghConEmuWndDC != nullptr);
			//// Сервисные функции
			//arg.GetFarHWND = GetFarHWND;
			//arg.GetFarHWND2 = GetFarHWND2;
			//arg.GetFarVersion = GetFarVersion;
			//arg.IsTerminalMode = IsTerminalMode;
			//arg.IsConsoleActive = IsConsoleActive;
			//arg.RegisterPanelView = RegisterPanelView;
			//arg.RegisterBackground = RegisterBackground;
			//arg.ActivateConsole = ActivateConsole;
			//arg.SyncExecute = SyncExecute;
			SAFETRY
			{
				fnOnConEmuLoaded(&arg);
				lbSucceeded = TRUE;
			} SAFECATCH
			{
				// Failed
				_ASSERTE(lbSucceeded == TRUE);
			}
		}
	}

	if (GetProcAddress(ahModule, "SetConsoleCallbacks"))
	{
		if (!ghExtConModule)
		{
			StartupConsoleHooks();
		}
	}
}

LPWSTR CPluginBase::ToUnicode(LPCSTR asOemStr)
{
	if (!asOemStr)
		return nullptr;
	if (!*asOemStr)
		return lstrdup(L"").Detach();

	return lstrdupW(asOemStr, CP_OEMCP).Detach();
}

void CPluginBase::ToOem(LPCWSTR asUnicode, char* rsOem, INT_PTR cchOemMax)
{
	WideCharToMultiByte(CP_OEMCP, 0, asUnicode?asUnicode:L"", -1, rsOem, (int)cchOemMax, nullptr, nullptr);
}

LPSTR CPluginBase::ToOem(LPCWSTR asUnicode)
{
	if (!asUnicode)
		return nullptr;
	int nLen = lstrlen(asUnicode);
	char* pszOem = (char*)calloc(nLen+1,sizeof(*pszOem));
	if (!pszOem)
		return nullptr;
	ToOem(asUnicode, pszOem, nLen+1);
	return pszOem;
}

void CPluginBase::ProcessDragFrom()
{
	if (!mb_StartupInfoOk)
		return;

	//только для отладки, в релизе не нужен!
	//#ifdef _DEBUG
	//MSetter lSet(&gnInLongOperation);
	//#endif

	//WindowInfo WInfo = {sizeof(WindowInfo)};
	//WInfo.Pos = -1;
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	int ActiveWT = GetActiveWindowType();
	if (ActiveWT != wt_Panels)
	{
		int ItemsCount=0;
		OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	CEPanelInfo PInfo = {sizeof(PInfo)};
	PInfo.szCurDir = (wchar_t*)calloc(BkPanelInfo_CurDirMax, sizeof(*PInfo.szCurDir));

	// Plugins (for example Temp panel) are allowed here
	if (GetPanelInfo(gpdf_Active|gpdf_NoHidden|ppdf_GetItems, &PInfo)
		&& (PInfo.nPanelType == pt_FilePanel || PInfo.nPanelType == pt_TreePanel))
	{
		if (!PInfo.szCurDir)
		{
			_ASSERTE(PInfo.szCurDir != nullptr);
			int ItemsCount=0;
			OutDataWrite(&ItemsCount, sizeof(int));
			OutDataWrite(&ItemsCount, sizeof(int)); // смена формата
			return;
		}

		int nDirLen = 0, nDirNoSlash = 0;

		if (PInfo.szCurDir[0])
		{
			nDirLen = lstrlen(PInfo.szCurDir);

			if (nDirLen > 0)
			{
				if (PInfo.szCurDir[nDirLen-1] != L'\\')
				{
					nDirNoSlash = 1;
				}
			}
		}

		// Это только предполагаемый размер, при необходимости он будет увеличен
		OutDataAlloc(sizeof(int)+PInfo.SelectedItemsNumber*((MAX_PATH+2)+sizeof(int)));
		//Maximus5 - новый формат передачи
		int nNull=0; // ItemsCount
		//WriteFile(hPipe, &nNull, sizeof(int), &cout, nullptr);
		OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));


		if (PInfo.SelectedItemsNumber<=0)
		{
			// Проверка того, что мы стоим на ".."
			if (PInfo.CurrentItem == 0 && PInfo.ItemsNumber > 0)
			{
				if (!nDirNoSlash)
					PInfo.szCurDir[nDirLen-1] = 0;
				else
					nDirLen++;

				int nWholeLen = nDirLen + 1;
				OutDataWrite(&nWholeLen, sizeof(int));
				OutDataWrite(&nDirLen, sizeof(int));
				OutDataWrite(PInfo.szCurDir, sizeof(WCHAR)*nDirLen);
			}

			// Fin
			OutDataWrite(&nNull/*ItemsCount*/, sizeof(int));
		}
		else
		{
			WIN32_FIND_DATAW FileInfo = {};
			wchar_t** piNames = new wchar_t*[PInfo.SelectedItemsNumber];
			bool* bIsFull = new bool[PInfo.SelectedItemsNumber];
			INT_PTR i, ItemsCount = PInfo.SelectedItemsNumber;
			int nMaxLen = MAX_PATH+1, nWholeLen = 1;

			// сначала посчитать максимальную длину буфера
			for (i = 0; i < ItemsCount; i++)
			{
				piNames[i] = nullptr; bIsFull[i] = false; // 'new' does not initilize memory

				if (!GetPanelItemInfo(PInfo, true, i, FileInfo, piNames+i))
					continue;

				if (!piNames[i] || !piNames[i][0])
				{
					_ASSERTE(!piNames[i] || !piNames[i][0]);
					SafeFree(piNames[i]);
					continue;
				}

				if (i == 0
				        && ((FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
				        && IsDotsName(piNames[i]))
				{
					SafeFree(piNames[i]);
					continue;
				}

				int nLen = nDirLen + nDirNoSlash;

				// May be this is already full path (temp panel for example)?
		        if (IsFilePath(piNames[i], true))
				{
					nLen = 0;
					bIsFull[i] = true;
				}

				nLen += lstrlenW(piNames[i]);

				if (nLen>nMaxLen)
					nMaxLen = nLen;

				nWholeLen += (nLen+1);
			}

			nMaxLen += nDirLen;

			OutDataWrite(&nWholeLen, sizeof(int));
			WCHAR* Path = new WCHAR[nMaxLen+1];

			for (i = 0; i < ItemsCount; i++)
			{
				//WCHAR Path[MAX_PATH+1];
				//ZeroMemory(Path, MAX_PATH+1);
				//Maximus5 - засада с корнем диска и возможностью overflow
				//StringCchPrintf(Path, countof(Path), L"%s\\%s", PInfo.szCurDir, PInfo.SelectedItems[i]->FileName);
				Path[0] = 0;

				if (!piNames[i])
					continue; //этот элемент получить не удалось

				int nLen = 0;

				if ((nDirLen > 0) && !bIsFull[i])
				{
					lstrcpy(Path, PInfo.szCurDir);

					if (nDirNoSlash)
					{
						Path[nDirLen]=L'\\';
						Path[nDirLen+1]=0;
					}

					nLen = nDirLen + nDirNoSlash;
				}

				lstrcpy(Path+nLen, piNames[i]);
				nLen += lstrlen(piNames[i]);
				nLen++;

				OutDataWrite(&nLen, sizeof(int));
				OutDataWrite(Path, sizeof(WCHAR)*nLen);

				SafeFree(piNames[i]);
			}

			delete [] piNames;
			delete [] bIsFull;
			delete [] Path;

			// Конец списка
			OutDataWrite(&nNull, sizeof(int));
		}
	}
	else
	{
		int ItemsCount=0;
		OutDataWrite(&ItemsCount, sizeof(int));
		OutDataWrite(&ItemsCount, sizeof(int)); // смена формата
	}

	SafeFree(PInfo.szCurDir);
}

void CPluginBase::ProcessDragTo()
{
	if (!mb_StartupInfoOk)
		return;

	//только для отладки, в релизе не нужен!
	//#ifdef _DEBUG
	//MSetter lSet(&gnInLongOperation);
	//#endif

	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);

	// попробуем работать в диалогах и редакторе
	int ActiveWT = GetActiveWindowType();
	if (ActiveWT == -1)
	{
		//InfoW2800->AdvControl(&guid_ConEmu, ACTL_FREEWINDOWINFO, 0, &WInfo);
		int ItemsCount=0;
		if (gpCmdRet==nullptr)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	int nStructSize;

	if ((ActiveWT == wt_Dialog) || (ActiveWT == wt_Editor))
	{
		// разрешить дроп в виде текста
		ForwardedPanelInfo DlgInfo = {};
		DlgInfo.NoFarConsole = TRUE;
		nStructSize = sizeof(DlgInfo);
		if (gpCmdRet==nullptr)
			OutDataAlloc(nStructSize+sizeof(nStructSize));
		OutDataWrite(&nStructSize, sizeof(nStructSize));
		OutDataWrite(&DlgInfo, nStructSize);
		return;
	}
	else if (ActiveWT != wt_Panels)
	{
		// Иначе - дроп не разрешен
		int ItemsCount=0;
		if (gpCmdRet==nullptr)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
		return;
	}

	nStructSize = sizeof(ForwardedPanelInfo)+2*sizeof(wchar_t); // потом увеличим на длину строк

	CEPanelInfo piActive = {}, piPassive = {};
	piActive.szCurDir = (wchar_t*)malloc(BkPanelInfo_CurDirMax*sizeof(wchar_t));
	piPassive.szCurDir = (wchar_t*)malloc(BkPanelInfo_CurDirMax*sizeof(wchar_t));
	bool lbAOK = GetPanelInfo(gpdf_Active, &piActive);
	bool lbPOK = GetPanelInfo(gpdf_Passive, &piPassive);

	if (lbAOK && piActive.szCurDir)
		nStructSize += (lstrlen(piActive.szCurDir))*sizeof(WCHAR);

	if (lbPOK && piPassive.szCurDir)
		nStructSize += (lstrlen(piPassive.szCurDir))*sizeof(WCHAR); // Именно WCHAR! не TCHAR

	ForwardedPanelInfo* pfpi = (ForwardedPanelInfo*)calloc(nStructSize,1);

	if (!pfpi)
	{
		int ItemsCount=0;
		if (gpCmdRet==nullptr)
			OutDataAlloc(sizeof(ItemsCount));
		OutDataWrite(&ItemsCount,sizeof(ItemsCount));
	}
	else
	{
		pfpi->ActivePathShift = sizeof(ForwardedPanelInfo);
		pfpi->pszActivePath = (WCHAR*)(((char*)pfpi)+pfpi->ActivePathShift);
		pfpi->PassivePathShift = pfpi->ActivePathShift+2; // если ActivePath заполнится - увеличим

		if (lbAOK)
		{
			pfpi->ActiveRect = piActive.rcPanelRect;

			if (!piActive.bPlugin && piActive.bVisible
				&& (piActive.nPanelType == pt_FilePanel || piActive.nPanelType == pt_TreePanel)
				&& (piActive.szCurDir && *piActive.szCurDir))
			{
				lstrcpyW(pfpi->pszActivePath, piActive.szCurDir);
				pfpi->PassivePathShift += lstrlenW(pfpi->pszActivePath)*2;
			}
		}

		pfpi->pszPassivePath = (WCHAR*)(((char*)pfpi)+pfpi->PassivePathShift);

		if (lbPOK)
		{
			pfpi->PassiveRect = piPassive.rcPanelRect;

			if (!piPassive.bPlugin && piPassive.bVisible
				&& (piPassive.nPanelType == pt_FilePanel || piPassive.nPanelType == pt_TreePanel)
				&& (piPassive.szCurDir && *piPassive.szCurDir))
			{
				lstrcpyW(pfpi->pszPassivePath, piPassive.szCurDir);
			}
		}

		if (gpCmdRet==nullptr)
			OutDataAlloc(nStructSize+sizeof(nStructSize));

		OutDataWrite(&nStructSize, sizeof(nStructSize));
		OutDataWrite(pfpi, nStructSize);
		SafeFree(pfpi);
	}

	SafeFree(piActive.szCurDir);
	SafeFree(piPassive.szCurDir);
}

bool CPluginBase::IsCurrentTabModal()
{
	if (!gpTabs)
		return false;

	MSectionLock SC; SC.Lock(csTabs);

	if ((gnCurTabCount > 0) && (gnCurTabCount == gpTabs->Tabs.nTabCount))
	{
		for (int i = gnCurTabCount-1; i >= 0; i--)
		{
			if (gpTabs->Tabs.tabs[i].Current)
			{
				return (gpTabs->Tabs.tabs[i].Modal != 0);
			}
		}
	}

	return false;
}

bool CPluginBase::PrintText(LPCWSTR pszText)
{
	bool lbRc = false;
	LPCWSTR pSrc = pszText;
	// Резервирование места под экранированные символы + макрос
	INT_PTR cchMax = 20 + 2*lstrlen(pszText); //-V112
	wchar_t* pszMacro = (wchar_t*)malloc(cchMax*sizeof(*pszMacro));
	if (!pszMacro)
	{
		_ASSERTE(pszMacro!=nullptr)
	}
	else
	{
		// Добавим префикс "^", чтобы не вообще посылать "нажатия кнопок" в плагины
		if (gFarVersion.dwVerMajor==1)
			_wcscpy_c(pszMacro, cchMax, L"@^$TEXT \"");
		else
			_wcscpy_c(pszMacro, cchMax, L"@^print(\"");

		wchar_t* pDst = pszMacro + lstrlen(pszMacro);
		while (*pSrc)
		{
			// Экранирование слешей и кавычек
			switch (*pSrc)
			{
			case L'\\':
			case L'"':
				*(pDst++) = L'\\';
				break;
			}
			*(pDst++) = *(pSrc++);
		}
		*pDst = 0;

		if (gFarVersion.dwVerMajor==1)
			_wcscat_c(pszMacro, cchMax, L"\"");
		else
			_wcscat_c(pszMacro, cchMax, L"\")");

		PostMacro(pszMacro, nullptr);
		free(pszMacro);

		lbRc = true;
	}

	return lbRc;
}
