
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

#define AssertCantActivate(x) //MBoxAssert(x)

#define SHOWDEBUGSTR
//#define ALLOWUSEFARSYNCHRO

// To catch gh#222
#undef USE_SEH

#include <cstdlib>
#include "Header.h"
#include "../common/shlobj.h"

#include "../common/ConEmuCheck.h"
#include "../common/ConEmuPipeMode.h"
#include "../common/EnvVar.h"
#include "../common/execute.h"
#include "../common/MGuiMacro.h"
#include "../common/MFileLog.h"
#include "../common/MModule.h"
#include "../common/MProcessBits.h"
#include "../common/MSectionSimple.h"
#include "../common/MSetter.h"
#include "../common/MStrEsc.h"
#include "../common/MToolHelp.h"
#include "../common/WConsole.h" // 'ENABLE_VIRTUAL_TERMINAL_INPUT'
#include "../common/MWow64Disable.h"
#include "../common/ProcessData.h"
#include "../common/ProcessSetEnv.h"
#include "../common/RgnDetect.h"
#include "../common/SetEnvVar.h"
#include "../common/WFiles.h"
#include "../common/WSession.h"
#include "../common/WThreads.h"
#include "../common/WUser.h"
#include "ConEmu.h"
#include "ConEmuPipe.h"
#include "ConfirmDlg.h"
#include "CreateProcess.h"
#include "DontEnable.h"
#include "DynDialog.h"
#include "Inside.h"
#include "LngRc.h"
#include "Macro.h"
#include "Menu.h"
#include "MyClipboard.h"
#include "OptionsClass.h"
#include "RConFiles.h"
#include "RConPalette.h"
#include "RealBuffer.h"
#include "RealConsole.h"
#include "GlobalHotkeys.h"
#include "RunQueue.h"
#include "SetColorPalette.h"
#include "SetPgDebug.h"
#include "Status.h"
#include "TabBar.h"
#include "TermX.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

#include <unordered_map>
#include <unordered_set>

#include "../common/MWnd.h"


#define DEBUGSTRCMD(s) //DEBUGSTR(s)
#define DEBUGSTRDRAW(s) //DEBUGSTR(s)
#define DEBUGSTRSTATUS(s) //DEBUGSTR(s)
#define DEBUGSTRINPUT(s) //DEBUGSTR(s)
#define DEBUGSTRINPUTMSG(s) //DEBUGSTR(s)
#define DEBUGSTRINPUTLL(s) //DEBUGSTR(s)
#define DEBUGSTRWHEEL(s) //DEBUGSTR(s)
#define DEBUGSTRINPUTPIPE(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRPROC(s) //DEBUGSTR(s)
#define DEBUGSTRCONHOST(s) //DEBUGSTR(s)
#define DEBUGSTRPKT(s) //DEBUGSTR(s)
#define DEBUGSTRCON(s) //DEBUGSTR(s)
#define DEBUGSTRLANG(s) //DEBUGSTR(s)// ; Sleep(2000)
#define DEBUGSTRSENDMSG(s) //DEBUGSTR(s)
#define DEBUGSTRLOGA(s) //OutputDebugStringA(s)
#define DEBUGSTRLOGW(s) //DEBUGSTR(s)
#define DEBUGSTRALIVE(s) //DEBUGSTR(s)
#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRMACRO(s) //DEBUGSTR(s)
#define DEBUGSTRALTSRV(s) //DEBUGSTR(s)
#define DEBUGSTRSTOP(s) //DEBUGSTR(s)
#define DEBUGSTRFOCUS(s) //LogFocusInfo(s)
#define DEBUGSTRGUICHILDPOS(s) //DEBUGSTR(s)
#define DEBUGSTRPROGRESS(s) //DEBUGSTR(s)
#define DEBUGSTRFARPID(s) //DEBUGSTR(s)
#define DEBUGSTRMOUSE(s) //DEBUGSTR(s)
#define DEBUGSTRSEL(s) //DEBUGSTR(s)
#define DEBUGSTRTEXTSEL(s) //DEBUGSTR(s)
#define DEBUGSTRCLICKPOS(s) //DEBUGSTR(s)
#define DEBUGSTRCTRLBS(s) //DEBUGSTR(s)
#define DEBUG_XTERM(s) DEBUGSTR(s)

// Иногда не отрисовывается диалог поиска полностью - только бежит текущая сканируемая директория.
// Иногда диалог отрисовался, но часть до текста "..." отсутствует

WARNING("В каждой VCon создать буфер BYTE[256] для хранения распознанных клавиш (Ctrl,...,Up,PgDn,Add,и пр.");
WARNING("Нераспознанные можно помещать в буфер {VKEY,wchar_t=0}, в котором заполнять последний wchar_t по WM_CHAR/WM_SYSCHAR");
WARNING("При WM_(SYS)CHAR помещать wchar_t в начало, в первый незанятый VKEY");
WARNING("При нераспознанном WM_KEYUP - брать(и убрать) wchar_t из этого буфера, послав в консоль UP");
TODO("А периодически - проводить проверку isKeyDown, и чистить буфер");
WARNING("при переключении на другую консоль (да наверное и в процессе просто нажатий - модификатор может быть изменен в самой программе) требуется проверка caps, scroll, num");
WARNING("а перед пересылкой символа/клавиши проверять нажат ли на клавиатуре Ctrl/Shift/Alt");

WARNING("Часто после разблокирования компьютера размер консоли изменяется (OK), но обновленное содержимое консоли не приходит в GUI - там остaется обрезанная верхняя и нижняя строка");


//Курсор, его положение, размер консоли, измененный текст, и пр...

#define VCURSORWIDTH 2
#define HCURSORHEIGHT 2

#define Free SafeFree
#define Alloc calloc

//#define Assert(V) if ((V)==FALSE) { wchar_t szAMsg[MAX_PATH*2]; swprintf_c(szAMsg, L"Assertion (%s) at\n%s:%i", _T(#V), _T(__FILE__), __LINE__); Box(szAMsg); }

#ifdef _DEBUG
#define HEAPVAL MCHKHEAP
#else
#define HEAPVAL
#endif

#ifdef _DEBUG
#define FORCE_INVALIDATE_TIMEOUT 999999999
#else
#define FORCE_INVALIDATE_TIMEOUT 300
#endif

#define CHECK_CONHWND_TIMEOUT 500

#define WAIT_THREAD_DETACH_TIMEOUT 5'000

#define HIGHLIGHT_RUNTIME_MIN 10'000
#define HIGHLIGHT_INVISIBLE_MIN 2000

static BOOL gbInSendConEvent = FALSE;


#define gsCloseGui CLngRc::getRsrc(lng_ConfirmCloseChildGuiQ/*"Confirm closing active child window?"*/)
#define gsCloseCon CLngRc::getRsrc(lng_ConfirmCloseConsoleQ/*"Confirm closing console?"*/)
#define gsTerminateAllButShell CLngRc::getRsrc(lng_ConfirmKillButShellQ/*"Terminate all but shell processes?"*/)
#define gsCloseEditor CLngRc::getRsrc(lng_ConfirmCloseEditorQ/*"Confirm closing Far editor?"*/)
#define gsCloseViewer CLngRc::getRsrc(lng_ConfirmCloseViewerQ/*"Confirm closing Far viewer?"*/)

#define GUI_MACRO_PREFIX L'#'

#define ALL_MODIFIERS (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)
#define CTRL_MODIFIERS (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED)

namespace {
CRealConsole::PostStringFlags operator|(const CRealConsole::PostStringFlags e1, const CRealConsole::PostStringFlags e2)
{
	return static_cast<CRealConsole::PostStringFlags>(static_cast<uint32_t>(e1) | static_cast<uint32_t>(e2));
}
bool operator&(const CRealConsole::PostStringFlags e1, const CRealConsole::PostStringFlags e2)
{
	return (static_cast<uint32_t>(e1) & static_cast<uint32_t>(e2)) != 0;
}
}

//static bool gbInTransparentAssert = false;

CRealConsole::CRealConsole(CVirtualConsole* pVCon, CConEmuMain* pOwner)
	: mp_VCon(pVCon)
	, mp_ConEmu(pOwner)
	, mb_ConstuctorFinished(false)
{
}

bool CRealConsole::Construct(CVirtualConsole* apVCon, RConStartArgsEx *args)
{
	// Can't use Assert while object was not initialized yet
	#ifdef _DEBUG
	_ASSERTE(apVCon);
	_ASSERTE(args && args->pszSpecialCmd && *args->pszSpecialCmd && !CConEmuMain::IsConsoleBatchOrTask(args->pszSpecialCmd));
	LPCWSTR pszCmdTemp = args ? args->pszSpecialCmd : nullptr;
	CmdArg lsFirstTemp;
	if (pszCmdTemp)
	{
		if ((pszCmdTemp = NextArg(pszCmdTemp, lsFirstTemp)))
		{
			_ASSERTE(!CConEmuMain::IsConsoleBatchOrTask(lsFirstTemp));
		}
	}
	// Add check for un-processed working directory
	_ASSERTE(args && (!args->pszStartupDir || args->pszStartupDir[0]!=L'%'));
	#endif
	MCHKHEAP;

	_ASSERTE(mp_VCon == apVCon);
	mp_VCon = apVCon;

	MCHKHEAP;
	SetConStatus(L"Initializing ConEmu (2)", cso_ResetOnConsoleReady|cso_DontUpdate|cso_Critical);
	//mp_VCon->mp_RCon = this;
	HWND hView = apVCon->GetView();
	if (!hView)
	{
		_ASSERTE(hView!=nullptr);
	}
	else
	{
		PostMessage(apVCon->GetView(), WM_SETCURSOR, -1, -1);
	}

	// Tabs
	tabs.mp_ActiveTab = new CTab("RealConsole:mp_ActiveTab",__LINE__);

	#ifdef TAB_REF_PLACE
	tabs.m_Tabs.SetPlace("RealConsole.cpp:tabs.m_Tabs",0);
	#endif

	// -- т.к. автопоказ табов может вызвать ресайз - то табы в самом конце инициализации!
	//SetTabs(nullptr,1);

	DWORD_PTR nSystemAffinity = (DWORD_PTR)-1, nProcessAffinity = (DWORD_PTR)-1;
	if (GetProcessAffinityMask(GetCurrentProcess(), &nProcessAffinity, &nSystemAffinity))
		mn_ProcessAffinity = nProcessAffinity;
	else
		mn_ProcessAffinity = 1;
	mn_ProcessPriority = GetPriorityClass(GetCurrentProcess());

	//m_DetectedDialogs.Count = 0;
	//mn_DetectCallCount = 0;
	wcscpy_c(Title, mp_ConEmu->GetDefaultTitle());
	wcscpy_c(TitleFull, Title);
	mn_MainSrv_PID = mn_ConHost_PID = 0; mh_MainSrv = nullptr; mb_MainSrv_Ready = false;
	mn_CheckFreqLock = 0;
	mn_ActiveLayout = 0;
	mn_AltSrv_PID = 0;  //mh_AltSrv = nullptr;
	mn_Terminal_PID = 0;
	mb_SwitchActiveServer = false;
	mh_SwitchActiveServer = CreateEvent(nullptr,FALSE,FALSE,nullptr);
	mh_ActiveServerSwitched = CreateEvent(nullptr,FALSE,FALSE,nullptr);
	mh_ConInputPipe = nullptr;
	mb_UseOnlyPipeInput = FALSE;
	//mh_CreateRootEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	mb_InCreateRoot = FALSE;
	mb_NeedStartProcess = FALSE; mb_IgnoreCmdStop = FALSE;
	ms_MainSrv_Pipe[0] = 0; ms_ConEmuC_Pipe[0] = 0; ms_ConEmuCInput_Pipe[0] = 0; ms_VConServer_Pipe[0] = 0;
	mn_TermEventTick = 0;
	mh_TermEvent = CreateEvent(nullptr,TRUE/*MANUAL - используется в нескольких нитях!*/,FALSE,nullptr); ResetEvent(mh_TermEvent);
	mh_StartExecuted = CreateEvent(nullptr,FALSE,FALSE,nullptr); ResetEvent(mh_StartExecuted);
	mb_StartResult = FALSE;
	UpdateStartState(rss_NotStarted, true);
	mh_MonitorThreadEvent = CreateEvent(nullptr,TRUE,FALSE,nullptr); //2009-09-09 Поставил Manual. Нужно для определения, что можно убирать флаг Detached
	mn_ServerActiveTick1 = mn_ServerActiveTick2 = 0;
	mh_UpdateServerActiveEvent = CreateEvent(nullptr,TRUE,FALSE,nullptr);
	//mb_UpdateServerActive = FALSE;
	mh_ApplyFinished.Create(nullptr,TRUE,FALSE,nullptr);
	//mh_EndUpdateEvent = CreateEvent(nullptr,FALSE,FALSE,nullptr);
	//WARNING("mh_Sync2WindowEvent убрать");
	//mh_Sync2WindowEvent = CreateEvent(nullptr,FALSE,FALSE,nullptr);
	//mh_ConChanged = CreateEvent(nullptr,FALSE,FALSE,nullptr);
	//mh_PacketArrived = CreateEvent(nullptr,FALSE,FALSE,nullptr);
	//mh_CursorChanged = nullptr;
	//mb_Detached = FALSE;
	//m_Args.pszSpecialCmd = nullptr; -- не требуется
	//mpsz_CmdBuffer = nullptr;
	mb_FullRetrieveNeeded = FALSE;
	m_StartTime = {};
	//mb_AdminShieldChecked = FALSE;
	mb_DataChanged = FALSE;
	mb_RConStartedSuccess = FALSE;
	m_Term = {};
	m_TermCursor = {};
	mn_ProgramStatus = 0; mn_FarStatus = 0; mn_Comspec4Ntvdm = 0;
	isShowConsole = gpSet->isConVisible;
	//mb_ConsoleSelectMode = false;
	mn_SelectModeSkipVk = 0;
	mn_ProcessCount = mn_ProcessClientCount = 0;
	mn_FarPID = 0;
	m_ActiveProcess = {};
	m_AppDistinctProcess = {};
	mb_ForceRefreshAppId = false;
	mn_FarNoPanelsCheck = 0;
	ms_LastActiveProcess[0] = 0;
	mn_LastAppSettingsId = -2;
	memset(m_FarPlugPIDs, 0, sizeof(m_FarPlugPIDs)); mn_FarPlugPIDsCount = 0;
	memset(m_TerminatedPIDs, 0, sizeof(m_TerminatedPIDs)); mn_TerminatedIdx = 0;
	mb_SkipFarPidChange = FALSE;
	mn_InRecreate = 0; mb_ProcessRestarted = FALSE;
	mb_InDetach = false;
	SetInCloseConsole(false);
	mb_RecreateFailed = FALSE;
	mn_StartTick = mn_RunTime = 0;
	mb_WasVisibleOnce = false;
	mn_DeactivateTick = 0;
	CloseConfirmReset();
	mn_LastSetForegroundPID = 0;
	mb_InPostCloseMacro = false;
	m_Mouse.bWasMouseSelection = false;
	mp_RenameDpiAware = nullptr;

	mpcs_CurWorkDir = new MSectionSimple(true);

	ms_MountRoot = nullptr;

	mn_TextColorIdx = 7; mn_BackColorIdx = 0;
	mn_PopTextColorIdx = 5; mn_PopBackColorIdx = 15;

	m_RConServer.Init(this);

	//mb_ThawRefreshThread = FALSE;
	mn_LastUpdateServerActive = 0;

	//mb_BuferModeChangeLocked = FALSE;

	mn_DefaultBufferHeight = gpSetCls->bForceBufferHeight ? gpSetCls->nForceBufferHeight : gpSet->DefaultBufferHeight;

	mp_Palette = new CRConPalette(this);

	mp_RBuf = new CRealBuffer(this);
	mp_EBuf = nullptr;
	mp_SBuf = nullptr;
	SetActiveBuffer(mp_RBuf, false);
	mb_ABufChaged = false;

	mn_LastInactiveRgnCheck = 0;
	#ifdef _DEBUG
	mb_DebugLocked = FALSE;
	#endif

	m_RootInfo = {};
	//m_RootInfo.nExitCode = STILL_ACTIVE;
	m_ServerClosing = {};
	m_Args.AssignFrom(RConStartArgsEx());
	ms_RootProcessName[0] = 0;
	mn_RootProcessIcon = -1;
	mb_NeedLoadRootProcessIcon = true;
	mn_LastInvalidateTick = 0;

	hConWnd = nullptr;

	m_ChildGui = {};
	setGuiWndPID(nullptr, 0, 0, nullptr); // force set mn_GuiWndPID to 0

	mn_InPostDeadChar = 0;

	mp_TrueColorerData = nullptr;
	mn_TrueColorMaxCells = 0;
	memset(&m_TrueColorerHeader, 0, sizeof(m_TrueColorerHeader));

	//mb_PluginDetected = FALSE;
	mn_FarPID_PluginDetected = 0;
	memset(&m_FarInfo, 0, sizeof(m_FarInfo));
	lstrcpy(ms_Editor, L"edit ");
	lstrcpy(ms_EditorRus, L"редактирование ");
	lstrcpy(ms_Viewer, L"view ");
	lstrcpy(ms_ViewerRus, L"просмотр ");
	lstrcpy(ms_TempPanel, L"{Temporary panel");
	lstrcpy(ms_TempPanelRus, L"{Временная панель");
	//lstrcpy(ms_NameTitle, L"Name");

	if (!mp_RBuf)
	{
		_ASSERTE(mp_RBuf);
		return false;
	}

	// Initialize buffer sizes
	if (!PreInit())
		return false;

	if (!PreCreate(args))
		return false;

	mb_WasStartDetached = (m_Args.Detached == crb_On);
	_ASSERTE(mb_WasStartDetached == (args->Detached == crb_On));

	mst_ServerStartingTime = {};

	/* *** TABS *** */
	// -- т.к. автопоказ табов может вызвать ресайз - то табы в самом конце инициализации!
	_ASSERTE(isMainThread()); // Иначе табы сразу не перетряхнутся
	SetTabs(nullptr, 1, 0); // Для начала - показывать вкладку Console, а там ФАР разберется
	tabs.mb_WasInitialized = true;
	MCHKHEAP;

	/* *** Set start pending *** */
	if (mb_NeedStartProcess)
	{
		// Push request to "startup queue"
		RequestStartup();
	}

	mb_ConstuctorFinished = true;
	return true;
}

CRealConsole::~CRealConsole()
{
	MCHKHEAP;
	DEBUGSTRCON(L"CRealConsole::~CRealConsole()\n");

	if (!isMainThread())
	{
		//_ASSERTE(isMainThread());
		MBoxA(L"~CRealConsole() called from background thread");
	}

	if (gbInSendConEvent)
	{
#ifdef _DEBUG
		_ASSERTE(gbInSendConEvent==FALSE);
#endif
		Sleep(100);
	}

	StopThread();
	MCHKHEAP

	if (mp_RBuf)
		{ delete mp_RBuf; mp_RBuf = nullptr; }
	if (mp_EBuf)
		{ delete mp_EBuf; mp_EBuf = nullptr; }
	if (mp_SBuf)
		{ delete mp_SBuf; mp_SBuf = nullptr; }
	mp_ABuf = nullptr;

	SafeDelete(mp_Palette);

	SafeCloseHandle(mh_StartExecuted);

	SafeCloseHandle(mh_MainSrv); mn_MainSrv_PID = mn_ConHost_PID = 0; mb_MainSrv_Ready = false;
	/*SafeCloseHandle(mh_AltSrv);*/  mn_AltSrv_PID = 0;
	mn_Terminal_PID = 0;
	SafeCloseHandle(mh_SwitchActiveServer); mb_SwitchActiveServer = false;
	SafeCloseHandle(mh_ActiveServerSwitched);
	SafeCloseHandle(mh_ConInputPipe);
	m_ConDataChanged.Close();
	if (m_ConHostSearch)
	{
		m_ConHostSearch.Release();
	}


	tabs.mn_tabsCount = 0;
	SafeDelete(tabs.mp_ActiveTab);
	tabs.m_Tabs.MarkTabsInvalid(CTabStack::MatchAll, 0);
	tabs.m_Tabs.ReleaseTabs(FALSE);
	mp_ConEmu->mp_TabBar->PrintRecentStack();

	//
	CloseLogFiles();

	if (mp_sei)
	{
		SafeCloseHandle(mp_sei->hProcess);
		SafeFree(mp_sei);
	}

	if (mp_sei_dbg)
	{
		SafeCloseHandle(mp_sei_dbg->hProcess);
		SafeFree(mp_sei_dbg);
	}

	m_RConServer.Stop(true);

	//CloseMapping();
	CloseMapHeader(); // CloseMapData() & CloseFarMapData() зовет сам CloseMapHeader
	CloseColorMapping(); // Colorer data

	SafeDelete(mp_RenameDpiAware);

	SafeDelete(mp_PriorityDpiAware);

	SafeDelete(mpcs_CurWorkDir);

	SafeCloseHandle(mh_UpdateServerActiveEvent);
	SafeCloseHandle(mh_MonitorThreadEvent);
	SafeDelete(mp_Files);
	SafeDelete(mp_XTerm);
	MCHKHEAP;
}

CConEmuMain* CRealConsole::Owner()
{
	return this ? mp_ConEmu : nullptr;
}

CVirtualConsole* CRealConsole::VCon()
{
	return this ? mp_VCon : nullptr;
}

bool CRealConsole::PreCreate(RConStartArgsEx *args)
{
	if (!args)
	{
		_ASSERTE(args != nullptr);
		mp_ConEmu->LogString("ERROR: nullptr passed to PreCreate");
		return false;
	}

	// At the moment our real console logs aren't created yet, use global one
	if (gpSet->isLogging())
	{
		wchar_t szPrefix[128];
		swprintf_c(szPrefix, L"CRealConsole::PreCreate, hView=x%08X, hBack=x%08X, Detached=%u, AsAdmin=%u, PTY=x%04X",
			LODWORD(reinterpret_cast<DWORD_PTR>(mp_VCon->GetView())), LODWORD(reinterpret_cast<DWORD_PTR>(mp_VCon->GetBack())),
			static_cast<UINT>(args->Detached), static_cast<UINT>(args->RunAsAdministrator), static_cast<UINT>(args->nPTY));
		const CEStr fullCommand = args->CreateCommandLine(false);
		const CEStr szInfo(szPrefix, L", Cmd=‘", fullCommand.c_str(), L"’");
		mp_ConEmu->LogString(szInfo.c_str(szPrefix));
	}

	bool bCopied = m_Args.AssignFrom(*args);

	if (!gpConEmu->CanUseInjects())
		m_Args.InjectsDisable = crb_On;

	// Don't leave security information (passwords) in memory
	if (bCopied && args->pszUserName)
	{
		_ASSERTE(*args->pszUserName);

		SecureZeroMemory(args->szUserPassword, sizeof(args->szUserPassword));

		// When User name was set, but password - Not...
		if (!*m_Args.szUserPassword && (m_Args.UseEmptyPassword != crb_On))
		{
			int nRc = mp_ConEmu->RecreateDlg(&m_Args);

			if (nRc != IDC_START)
				bCopied = false;
		}
	}

	if (!bCopied)
		return false;

	// 111211 - здесь может быть передан "-new_console:..."
	if (m_Args.pszSpecialCmd)
	{
		// Должен быть обработан в вызывающей функции (CVirtualConsole::CreateVCon?)
		_ASSERTE(wcsstr(args->pszSpecialCmd, L"-new_console")==nullptr);
		m_Args.ProcessNewConArg();
	}
	else
	{
		_ASSERTE(((args->Detached == crb_On) || (args->pszSpecialCmd && *args->pszSpecialCmd)) && "Command line must be specified already!");
	}

	mb_NeedStartProcess = FALSE;

	PrepareNewConArgs();

	PrepareDefaultColors();

	if (!ms_DefTitle.IsEmpty())
	{
		//lstrcpyn(Title, ms_DefTitle, countof(Title));
		//wcscpy_c(TitleFull, Title);
		//wcscpy_c(ms_PanelTitle, Title);
		lstrcpyn(TitleCmp, ms_DefTitle, countof(TitleCmp));
		OnTitleChanged();
	}

	if (args->Detached == crb_On)
	{
		// Пока ничего не делаем - просто создается серверная нить
		if (!PreInit())  //TODO: вообще-то PreInit() уже наверное вызван...
		{
			return false;
		}

		m_Args.Detached = crb_On;
	}
	else
	{
		mb_NeedStartProcess = TRUE;
	}

	if (mp_RBuf)
	{
		mp_RBuf->PreFillBuffers();
	}

	if (!StartMonitorThread())
	{
		return false;
	}

	// В фоновой вкладке?
	args->BackgroundTab = m_Args.BackgroundTab;

	return true;
}

void CRealConsole::RepositionDialogWithTab(HWND hDlg)
{
	// Positioning
	RECT rcDlg = {}; GetWindowRect(hDlg, &rcDlg);
	if (mp_ConEmu->isTabsShown())
	{
		RECT rcTab = {};
		if (mp_ConEmu->mp_TabBar->GetActiveTabRect(&rcTab))
		{
			if (gpSet->nTabsLocation == 1)
				OffsetRect(&rcDlg, rcTab.left - rcDlg.left, rcTab.top - rcDlg.bottom); // bottom
			else
				OffsetRect(&rcDlg, rcTab.left - rcDlg.left, rcTab.bottom - rcDlg.top); // top
		}
	}
	else
	{
		RECT rcCon = {};
		if (GetWindowRect(GetView(), &rcCon))
		{
			OffsetRect(&rcDlg, rcCon.left - rcDlg.left, rcCon.top - rcDlg.top);
		}
	}
	MoveWindowRect(hDlg, rcDlg);
}

INT_PTR CRealConsole::priorityProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	CRealConsole* pRCon = nullptr;
	if (messg == WM_INITDIALOG)
		pRCon = (CRealConsole*)lParam;
	else
		pRCon = (CRealConsole*)GetWindowLongPtr(hDlg, DWLP_USER);

	if (!pRCon)
		return FALSE;

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hClassIcon));
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hClassIconSm));

			pRCon->mp_ConEmu->OnOurDialogOpened();
			_ASSERTE(pRCon!=nullptr);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pRCon);

			CDynDialog::LocalizeDialog(hDlg, lng_DlgAffinity);

			if (pRCon->mp_PriorityDpiAware)
				pRCon->mp_PriorityDpiAware->Attach(hDlg, ghWnd, CDynDialog::GetDlgClass(hDlg));

			// Positioning
			pRCon->RepositionDialogWithTab(hDlg);

			// Show affinity/priority
			DWORD_PTR nSystemAffinity = (DWORD_PTR)-1, nProcessAffinity = (DWORD_PTR)-1;
			uint64_t All = GetProcessAffinityMask(GetCurrentProcess(), &nProcessAffinity, &nSystemAffinity) ? nSystemAffinity : 1;
			uint64_t Current = pRCon->mn_ProcessAffinity;
			for (int i = 0; i < 64; i++)
			{
				HWND hCheck = GetDlgItem(hDlg, cbAffinity0+i);
				if (!hCheck)
				{
					_ASSERTE(hCheck != nullptr);
					continue;
				}
				EnableWindow(hCheck, (All & 1) != 0);
				CheckDlgButton(hDlg, cbAffinity0+i, (Current & 1) ? BST_CHECKED : BST_UNCHECKED);
				All = (All >> 1);
				Current = (Current >> 1);
			}

			UINT rbPriority = rbPriorityNormal;
			switch (pRCon->mn_ProcessPriority)
			{
			case REALTIME_PRIORITY_CLASS:
				rbPriority = rbPriorityRealtime; break;
			case HIGH_PRIORITY_CLASS:
				rbPriority = rbPriorityHigh; break;
			case ABOVE_NORMAL_PRIORITY_CLASS:
				rbPriority = rbPriorityAbove; break;
			case NORMAL_PRIORITY_CLASS:
				rbPriority = rbPriorityNormal; break;
			case BELOW_NORMAL_PRIORITY_CLASS:
				rbPriority = rbPriorityBelow; break;
			case IDLE_PRIORITY_CLASS:
				rbPriority = rbPriorityIdle; break;
			}
			CheckRadioButton(hDlg, rbPriorityRealtime, rbPriorityIdle, rbPriority);

			SetFocus(GetDlgItem(hDlg, IDOK));
			return FALSE;
		}

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
					case IDOK:
						{
							// Get changes
							uint64_t All = 0, Current = 1;
							for (int i = 0; i < 64; i++)
							{
								HWND hCheck = GetDlgItem(hDlg, cbAffinity0+i);
								if (IsDlgButtonChecked(hDlg, cbAffinity0+i))
									All |= Current;
								Current = (Current << 1);
							}

							DWORD nPriority = NORMAL_PRIORITY_CLASS;
							if (IsDlgButtonChecked(hDlg, rbPriorityRealtime))
								nPriority = REALTIME_PRIORITY_CLASS;
							else if (IsDlgButtonChecked(hDlg, rbPriorityHigh))
								nPriority = HIGH_PRIORITY_CLASS;
							else if (IsDlgButtonChecked(hDlg, rbPriorityAbove))
								nPriority = ABOVE_NORMAL_PRIORITY_CLASS;
							else if (IsDlgButtonChecked(hDlg, rbPriorityBelow))
								nPriority = BELOW_NORMAL_PRIORITY_CLASS;
							else if (IsDlgButtonChecked(hDlg, rbPriorityIdle))
								nPriority = IDLE_PRIORITY_CLASS;

							// Return to pRCon
							pRCon->mn_ProcessAffinity = All;
							pRCon->mn_ProcessPriority = nPriority;

							// Done
							EndDialog(hDlg, IDOK);
							return TRUE;
						}
					case IDCANCEL:
					case IDCLOSE:
						priorityProc(hDlg, WM_CLOSE, 0, 0);
						return TRUE;
				}
			}
			break;

		case WM_CLOSE:
			pRCon->mp_ConEmu->OnOurDialogClosed();
			EndDialog(hDlg, IDCANCEL);
			break;

		case WM_DESTROY:
			if (pRCon->mp_PriorityDpiAware)
				pRCon->mp_PriorityDpiAware->Detach();
			break;

		default:
			if (pRCon->mp_PriorityDpiAware && pRCon->mp_PriorityDpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
			{
				return TRUE;
			}
	}

	return FALSE;
}

bool CRealConsole::ChangeAffinityPriority(LPCWSTR asAffinity /*= nullptr*/, LPCWSTR asPriority /*= nullptr*/)
{
	bool lbRc = false;
	INT_PTR iRc = IDCANCEL;

	DWORD dwServerPID = GetServerPID(true);
	if (!dwServerPID)
		return false;

	if ((asAffinity && *asAffinity) || (asPriority && *asPriority))
	{
		wchar_t* pszEnd;
		if (asAffinity && *asAffinity)
		{
			if (asAffinity[0] == L'0' && (asAffinity[1] == L'x' || asAffinity[1] == L'X'))
				mn_ProcessAffinity = _wcstoui64(asAffinity+2, &pszEnd, 16);
			else if (isDigit(asAffinity[0]))
				mn_ProcessAffinity = _wcstoui64(asAffinity, &pszEnd, 10);
		}
		if (asPriority && *asPriority)
		{
			if (asPriority[0] == L'0' && (asPriority[1] == L'x' || asPriority[1] == L'X'))
				mn_ProcessPriority = wcstoul(asPriority+2, &pszEnd, 16);
			else if (isDigit(asPriority[0]))
				mn_ProcessPriority = wcstoul(asPriority, &pszEnd, 10);
		}
	}
	else
	{
		DontEnable de;
		CDpiForDialog::Create(mp_PriorityDpiAware);
		iRc = CDynDialog::ExecuteDialog(IDD_AFFINITY, ghWnd, priorityProc, reinterpret_cast<LPARAM>(this));
		SafeDelete(mp_PriorityDpiAware);
	}

	if (iRc == IDOK)
	{
		// Handles must have PROCESS_SET_INFORMATION access right, so we call MainServer

		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_AFFNTYPRIORITY, sizeof(CESERVER_REQ_HDR)+2*sizeof(uint64_t));
		if (!pIn)
		{
			// Not enough memory? System fails
			return false;
		}

		pIn->qwData[0] = mn_ProcessAffinity;
		pIn->qwData[1] = mn_ProcessPriority;

		DEBUGTEST(DWORD dwTickStart = timeGetTime());

		//Terminate
		CESERVER_REQ *pOut = ExecuteSrvCmd(dwServerPID, pIn, ghWnd);

		lbRc = (pOut && (pOut->DataSize() > 2*sizeof(DWORD)) && (pOut->dwData[0] != 0));

		DEBUGTEST(DWORD dwTickEnd = timeGetTime());
		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
	}

	return lbRc;
}

RealBufferType CRealConsole::GetActiveBufferType()
{
	AssertThisRet(rbt_Undefined);
	if (!mp_ABuf)
		return rbt_Undefined;
	return mp_ABuf->m_Type;
}

void CRealConsole::DumpConsole(HANDLE ahFile)
{
	_ASSERTE(mp_ABuf!=nullptr);

	return mp_ABuf->DumpConsole(ahFile);
}

bool CRealConsole::LoadDumpConsole(LPCWSTR asDumpFile)
{
	AssertThisRet(false);

	if (!mp_SBuf)
	{
		mp_SBuf = new CRealBuffer(this, rbt_DumpScreen);
		if (!mp_SBuf)
		{
			_ASSERTE(mp_SBuf!=nullptr);
			return false;
		}
	}

	if (!mp_SBuf->LoadDumpConsole(asDumpFile))
	{
		SetActiveBuffer(mp_RBuf);
		return false;
	}

	SetActiveBuffer(mp_SBuf);

	return true;
}

bool CRealConsole::LoadAlternativeConsole(LoadAltMode iMode /*= lam_Default*/)
{
	AssertThisRet(false);

	if (!mp_SBuf)
	{
		mp_SBuf = new CRealBuffer(this, rbt_Alternative);
		if (!mp_SBuf)
		{
			_ASSERTE(mp_SBuf!=nullptr);
			return false;
		}
	}

	if (!mp_SBuf->LoadAlternativeConsole(iMode))
	{
		SetActiveBuffer(mp_RBuf);
		return false;
	}

	if (mb_ConPty.load() && mp_ABuf->m_Type == rbt_Primary)
		GetConPtyGlyphs(0, 0, m_FrozenConPtyGlyphs, true);

	SetActiveBuffer(mp_SBuf);

	return true;
}

bool CRealConsole::SetActiveBuffer(RealBufferType aBufferType)
{
	AssertThisRet(false);

	bool lbRc;
	switch (aBufferType)
	{
	case rbt_Primary:
		lbRc = SetActiveBuffer(mp_RBuf);
		break;

	case rbt_Alternative:
		if (GetActiveBufferType() != rbt_Primary)
		{
			lbRc = true; // уже НЕ основной буфер. Не меняем
			break;
		}

		lbRc = LoadAlternativeConsole();
		break;

	default:
		// Другие тут пока не поддерживаются
		_ASSERTE(aBufferType==rbt_Primary);
		lbRc = false;
	}

	//mp_VCon->Invalidate();

	return lbRc;
}

bool CRealConsole::SetActiveBuffer(CRealBuffer* aBuffer, bool abTouchMonitorEvent /*= true*/)
{
	AssertThisRet(false);

	if (!aBuffer || (aBuffer != mp_RBuf && aBuffer != mp_EBuf && aBuffer != mp_SBuf))
	{
		_ASSERTE(aBuffer && (aBuffer == mp_RBuf || aBuffer == mp_EBuf || aBuffer == mp_SBuf));
		return false;
	}

	CRealBuffer* pOldBuffer = mp_ABuf;

	mp_ABuf = aBuffer;
	if (aBuffer == mp_RBuf) m_FrozenConPtyGlyphs.clear();
	mb_ABufChaged = true;

	if (isActive(false))
	{
		// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
		OnBufferHeight();
	}

	if (mh_MonitorThreadEvent && abTouchMonitorEvent)
	{
		// Передернуть нить MonitorThread
		SetMonitorThreadEvent();
	}

	if (pOldBuffer && (pOldBuffer == mp_SBuf))
	{
		mp_SBuf->ReleaseMem();
	}

	return true;
}

void CRealConsole::DoLockUnlock(bool bLock)
{
	DWORD nServerPID = GetServerPID(true);
	if (!nServerPID)
		return;

	wchar_t szInfo[80];
	CESERVER_REQ *pIn = nullptr, *pOut = nullptr;

	if (bLock)
	{
		swprintf_c(szInfo, L"RCon ID=%i is locking", mp_VCon->ID());
		mp_ConEmu->LogString(szInfo);

		pIn = ExecuteNewCmd(CECMD_LOCKSTATION, sizeof(CESERVER_REQ_HDR));
		if (pIn)
		{
			pOut = ExecuteSrvCmd(nServerPID, pIn, ghWnd);
		}
	}
	else
	{
		RECT rcCon = mp_ConEmu->CalcRect(CER_CONSOLE_CUR, mp_VCon);

		swprintf_c(szInfo, L"RCon ID=%i is unlocking, NewSize={%i,%i}", mp_VCon->ID(), rcCon.right, rcCon.bottom);
		mp_ConEmu->LogString(szInfo);

		// Don't call SetConsoleSize to avoid skipping server calls due optimization
		pIn = ExecuteNewCmd(CECMD_UNLOCKSTATION, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETSIZE));
		if (pIn)
		{
			pIn->SetSize.size = MakeCoord(rcCon.right, rcCon.bottom);
			pIn->SetSize.nBufferHeight = mp_RBuf->BufferHeight(0);
			pOut = ExecuteSrvCmd(nServerPID, pIn, ghWnd);
		}
	}

	if (pOut)
	{
		SetConStatus(bLock ? L"LOCKED" : nullptr);
	}

	ExecuteFreeResult(pIn);
	ExecuteFreeResult(pOut);
}

bool CRealConsole::SetConsoleSize(SHORT sizeX, SHORT sizeY, USHORT sizeBuffer, DWORD anCmdID/*=CECMD_SETSIZESYNC*/)
{
	AssertThisRet(false);

	// Всегда меняем _реальный_ буфер консоли.
	return (mp_RBuf->SetConsoleSize(sizeX, sizeY, sizeBuffer, anCmdID) != FALSE);
}

void CRealConsole::EndSizing()
{
	if (m_ConStatus.szText[0])
	{
		SetConStatus(nullptr);
	}
}

void CRealConsole::SyncGui2Window(const RECT rcVConBack)
{
	AssertThis();

	if (m_ChildGui.hGuiWnd && !m_ChildGui.bGuiExternMode)
	{
		if (!IsWindow(m_ChildGui.hGuiWnd))
		{
			// Странно это, по идее, приложение при закрытии окна должно было сообщить,
			// что оно закрылось, m_ChildGui.hGuiWnd больше не валиден...
			_ASSERTE(IsWindow(m_ChildGui.hGuiWnd));
			setGuiWnd(nullptr);
			return;
		}

		#ifdef _DEBUG
		RECT rcTemp = mp_ConEmu->CalcRect(CER_BACK, mp_VCon);
		#endif
		// Окошко гуевого приложения нужно разместить поверх области, отведенной под наш VCon.
		// Но! тут нужна вся область, без отрезания прокруток и округлений размеров под знакоместо
		RECT rcGui = rcVConBack;
		OffsetRect(&rcGui, -rcGui.left, -rcGui.top);

		DWORD dwExStyle = GetWindowLong(m_ChildGui.hGuiWnd, GWL_EXSTYLE);
		DWORD dwStyle = GetWindowLong(m_ChildGui.hGuiWnd, GWL_STYLE);
		#if 0
		HRGN hrgn = CreateRectRgn(0,0,0,0);
		int RgnType = GetWindowRgn(m_ChildGui.hGuiWnd, hrgn);
		#endif
		CorrectGuiChildRect(dwStyle, dwExStyle, rcGui, m_ChildGui.Process.Name);
		#if 0
		if (hrgn) DeleteObject(hrgn);
		#endif
		RECT rcCur = {};
		GetWindowRect(m_ChildGui.hGuiWnd, &rcCur);
		HWND hBack = mp_VCon->GetBack();
		MapWindowPoints(nullptr, hBack, (LPPOINT)&rcCur, 2);
		if (memcmp(&rcCur, &rcGui, sizeof(RECT)) != 0)
		{
			// Через команду пайпа, а то если он "под админом" будет Access denied
			SetOtherWindowPos(m_ChildGui.hGuiWnd, HWND_TOP, rcGui.left,rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top,
				SWP_ASYNCWINDOWPOS|SWP_NOACTIVATE);
		}
		// Запомнить ЭКРАННЫЕ координаты, куда поставили окошко
		MapWindowPoints(hBack, nullptr, (LPPOINT)&rcGui, 2);
		StoreGuiChildRect(&rcGui);
	}
}

// Изменить размер консоли по размеру окна (главного)
// prcNewWnd передается из CConEmuMain::OnSizing(WPARAM wParam, LPARAM lParam)
// для опережающего ресайза консоли (во избежание мелькания отрисовки панелей)
void CRealConsole::SyncConsole2Window(bool abNtvdmOff/*=FALSE*/, LPRECT prcNewWnd/*=nullptr*/)
{
	AssertThis();

	//2009-06-17 Попробуем так. Вроде быстрее и наверное ничего блокироваться не должно
	/*
	if (GetCurrentThreadId() != mn_MonitorThreadID) {
	    RECT rcClient; Get ClientRect(ghWnd, &rcClient);
	    _ASSERTE(rcClient.right>250 && rcClient.bottom>200);

	    // Посчитать нужный размер консоли
	    RECT newCon = mp_ConEmu->CalcRect(CER_CONSOLE, rcClient, CER_MAINCLIENT);

	    if (newCon.right==TextWidth() && newCon.bottom==TextHeight())
	        return; // размеры не менялись

	    SetEvent(mh_Sync2WindowEvent);
	    return;
	}
	*/
	DEBUGLOGFILE("CRealConsole::SyncConsole2Window\n");
	RECT rcClient;

	if (prcNewWnd == nullptr)
	{
		WARNING("DoubleView: переделать для...");
		rcClient = mp_ConEmu->ClientRect();
	}
	else
	{
		rcClient = mp_ConEmu->CalcRect(CER_MAINCLIENT, *prcNewWnd, CER_MAIN);
	}

	//_ASSERTE(rcClient.right>140 && rcClient.bottom>100);
	// Посчитать нужный размер консоли
	mp_ConEmu->AutoSizeFont(rcClient, CER_MAINCLIENT);
	RECT newCon = mp_ConEmu->CalcRect(abNtvdmOff ? CER_CONSOLE_NTVDMOFF : CER_CONSOLE_CUR, rcClient, CER_MAINCLIENT, mp_VCon);
	_ASSERTE(newCon.right>=MIN_CON_WIDTH && newCon.bottom>=MIN_CON_HEIGHT);

	if (abNtvdmOff && isNtvdm())
	{
		LogString(L"NTVDM was stopped (SyncConsole2Window), clearing CES_NTVDM");
		SetProgramStatus(CES_NTVDM, 0);
	}

	#if 0
	if (m_ChildGui.hGuiWnd && !m_ChildGui.bGuiExternMode)
	{
		_ASSERTE(FALSE && "Fails on DoubleView?");
		SyncGui2Window(rcClient);
	}
	#endif

	// Change the size of the buffer (virtuals too)
	// Real console uses SHORT as size, so we are too
	COORD crNewSize = MakeCoord(newCon.right, newCon.bottom);
	mp_ABuf->SyncConsole2Window(crNewSize.X, crNewSize.Y);
}

// Вызывается при аттаче (после детача), или после RunAs?
// sbi передавать не ссылкой, а копией, ибо та же память
bool CRealConsole::AttachConemuC(HWND ahConWnd, DWORD anConemuC_PID, const CESERVER_REQ_STARTSTOP* rStartStop, CESERVER_REQ_SRVSTARTSTOPRET& pRet)
{
	DWORD dwErr = 0;
	HANDLE hProcess = nullptr;
	DEBUGTEST(bool bAdmStateChanged = ((rStartStop->bUserIsAdmin!=FALSE) != (m_Args.RunAsAdministrator==crb_On)));
	m_Args.RunAsAdministrator = rStartStop->bUserIsAdmin ? crb_On : crb_Off;

	// Процесс запущен через ShellExecuteEx под другим пользователем (Administrator)
	if (mp_sei)
	{
		// Issue 791: Console server fails to duplicate self Process handle to GUI
		// _ASSERTE(mp_sei->hProcess==nullptr);

		if (!rStartStop->hServerProcessHandle)
		{
			if (mp_sei->hProcess)
			{
				hProcess = mp_sei->hProcess;
			}
			else
			{
				_ASSERTE(rStartStop->hServerProcessHandle!=0);
				_ASSERTE(mp_sei->hProcess!=nullptr);
				DisplayLastError(L"Server process handle was not received!", -1);
			}
		}
		else
		{
			//_ASSERTE(FALSE && "Continue to AttachConEmuC");

			// Пока не все хорошо.
			// --Переделали. Запуск идет через GUI, чтобы окошко не мелькало.
			// --mp_sei->hProcess не заполняется
			HANDLE hRecv = (HANDLE)(DWORD_PTR)rStartStop->hServerProcessHandle;
			DWORD nWait = WaitForSingleObject(hRecv, 0);

			#ifdef _DEBUG
			DWORD nSrvPID = 0;
			typedef DWORD (WINAPI* GetProcessId_t)(HANDLE);
			GetProcessId_t getProcessId = (GetProcessId_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetProcessId");
			if (getProcessId)
				nSrvPID = getProcessId(hRecv);
			#endif

			if (nWait != WAIT_TIMEOUT)
			{
				DisplayLastError(L"Invalid server process handle received!");
			}
		}
	}

	if (rStartStop->sCmdLine && *rStartStop->sCmdLine)
	{
		SafeFree(m_Args.pszSpecialCmd);
		_ASSERTE(m_Args.Detached == crb_On);
		m_Args.pszSpecialCmd = lstrdup(rStartStop->sCmdLine).Detach();
	}

	_ASSERTE(hProcess==nullptr || (mp_sei && mp_sei->hProcess));
	_ASSERTE(rStartStop->hServerProcessHandle!=nullptr || mh_MainSrv!=nullptr);

	if (rStartStop->hServerProcessHandle && (hProcess != (HANDLE)(DWORD_PTR)rStartStop->hServerProcessHandle))
	{
		if (!hProcess)
		{
			hProcess = (HANDLE)(DWORD_PTR)rStartStop->hServerProcessHandle;
		}
		else
		{
			CloseHandle((HANDLE)(DWORD_PTR)rStartStop->hServerProcessHandle);
		}
	}

	if (mp_sei && mp_sei->hProcess)
	{
		_ASSERTE(mp_sei->hProcess == hProcess || hProcess == (HANDLE)(DWORD_PTR)rStartStop->hServerProcessHandle);
		mp_sei->hProcess = nullptr;
	}

	// Иначе - открываем как обычно
	if (!hProcess)
	{
		hProcess = OpenProcess(MY_PROCESS_ALL_ACCESS, FALSE, anConemuC_PID);
		if (!hProcess || hProcess == INVALID_HANDLE_VALUE)
		{
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, anConemuC_PID); //-V773
		}
	}

	if (!hProcess || hProcess == INVALID_HANDLE_VALUE)
	{
		DisplayLastError(L"Can't open ConEmuC process! Attach is impossible!", dwErr = GetLastError());
		return false;
	}

	WARNING("TODO: Support horizontal scroll");

	// Remove bRootIsCmdExe&isScroll() from expression. Use REAL scrolls from REAL console
	// BufferHeightTurnedOn and SetBufferHeightMode checks moved to InitSBI
	// bool bCurBufHeight = /*rStartStop->bRootIsCmdExe || mp_RBuf->isScroll() ||*/ mp_RBuf->BufferHeightTurnedOn(lsbi);

	RECT rcWnd = mp_ConEmu->ClientRect();
	TODO("DoubleView: ?");
	mp_ConEmu->AutoSizeFont(rcWnd, CER_MAINCLIENT);

	mp_RBuf->InitSBI(rStartStop->sbi);

	//// Событие "изменения" консоли //2009-05-14 Теперь события обрабатываются в GUI, но прийти из консоли может изменение размера курсора
	//swprintf_c(ms_ConEmuC_Pipe, CE_CURSORUPDATE, mn_MainSrv_PID);
	//mh_CursorChanged = CreateEvent ( nullptr, FALSE, FALSE, ms_ConEmuC_Pipe );
	//if (!mh_CursorChanged) {
	//    ms_ConEmuC_Pipe[0] = 0;
	//    DisplayLastError(L"Can't create event!");
	//    return false;
	//}

	//mh_MainSrv = hProcess;
	//mn_MainSrv_PID = anConemuC_PID;
	SetMainSrvPID(anConemuC_PID, hProcess);
	//Current AltServer
	SetAltSrvPID(rStartStop->nAltServerPID);

	SetHwnd(ahConWnd);
	ProcessUpdate(&anConemuC_PID, 1);

	// Инициализировать имена пайпов, событий, мэппингов и т.п.
	InitNames();

	// Открыть/создать map с данными
	OnServerStarted(anConemuC_PID, hProcess, rStartStop->dwKeybLayout, TRUE);

	#ifdef _DEBUG
	_ASSERTE(isServerAlive());
	#endif

	// Prepare result
	QueryStartStopRet(pRet);

	// Передернуть нить MonitorThread
	SetMonitorThreadEvent();

	_ASSERTE((pRet.Info.nBufferHeight == 0) || ((int)pRet.Info.nBufferHeight > (rStartStop->sbi.srWindow.Bottom-rStartStop->sbi.srWindow.Top)));

	return true;
}

void CRealConsole::QueryStartStopRet(CESERVER_REQ_SRVSTARTSTOPRET& pRet)
{
	BOOL bCurBufHeight = isBufferHeight();
	pRet.Info.bWasBufferHeight = bCurBufHeight;
	pRet.Info.hWnd = ghWnd;
	pRet.Info.hWndDc = mp_VCon->GetView();
	pRet.Info.hWndBack = mp_VCon->GetBack();
	pRet.Info.dwPID = GetCurrentProcessId();
	pRet.Info.nBufferHeight = bCurBufHeight ? mp_ABuf->GetBufferHeight() : 0;
	pRet.Info.nWidth = TextWidth();
	pRet.Info.nHeight = TextHeight();
	pRet.Info.dwMainSrvPID = GetServerPID(true);
	pRet.Info.dwAltSrvPID = 0;

	pRet.Info.bNeedLangChange = TRUE;
	TODO("Проверить на x64, не будет ли проблем с 0xFFFFFFFFFFFFFFFFFFFFF");
	pRet.Info.NewConsoleLang = mp_ConEmu->GetActiveKeyboardLayout();

	// Установить шрифт для консоли
	pRet.Font.cbSize = sizeof(pRet.Font);
	pRet.Font.inSizeY = gpSet->ConsoleFont.lfHeight;
	pRet.Font.inSizeX = gpSet->ConsoleFont.lfWidth;
	lstrcpy(pRet.Font.sFontName, gpSet->ConsoleFont.lfFaceName);

	// Limited logging of console contents (same output as processed by ConEmu::ConsoleFlags::ProcessAnsi)
	pRet.AnsiLog = GetAnsiLogInfo();

	// Return GUI info, let it be in one place
	mp_ConEmu->GetGuiInfo(pRet.GuiMapping);

	// Environment strings (inherited from parent console)
	_ASSERTE(pRet.EnvCommands.cchCount == 0 && pRet.EnvCommands.psz == nullptr);
	SetInitEnvCommands(pRet);
}

void CRealConsole::SetInitEnvCommands(CESERVER_REQ_SRVSTARTSTOPRET& pRet)
{
	CProcessEnvCmd env;

	if (m_Args.cchEnvStrings && m_Args.pszEnvStrings)
	{
		env.AddZeroedPairs(m_Args.pszEnvStrings);
	}

	if (gpSet->psEnvironmentSet)
	{
		env.AddLines(gpSet->psEnvironmentSet->c_str(), true);
	}

	size_t cchData = 0;
	CEStr EnvData(env.Allocate(&cchData));
	pRet.EnvCommands.Set(EnvData.Detach(), cchData);

	// Current palette
	_ASSERTE(pRet.PaletteName.psz == nullptr);
	const ColorPalette* pPal = nullptr;
	LPCWSTR pszPalette = m_Args.pszPalette ? m_Args.pszPalette : nullptr;
	if (!pszPalette || !*pszPalette)
	{
		int iActiveIndex = mp_VCon->GetPaletteIndex();
		if (iActiveIndex == -1)
			pPal = gpSet->PaletteFindCurrent(true);
		if (!pPal)
			pPal = gpSet->PaletteGet(iActiveIndex);
		if (pPal)
			pszPalette = pPal->pszName;
	}
	if (pszPalette && *pszPalette)
	{
		pRet.PaletteName.Set(lstrdup(pszPalette).Detach());
	}

	if (pPal)
	{
		_ASSERTE((mn_TextColorIdx || mn_BackColorIdx) && (mn_PopTextColorIdx || mn_PopBackColorIdx));
		_ASSERTE(sizeof(pRet.Palette.ColorTable) == sizeof(pPal->Colors));

		pRet.Palette.bPalletteLoaded = true;
		pRet.Palette.wAttributes = MAKECONCOLOR(mn_TextColorIdx, mn_BackColorIdx);
		pRet.Palette.wPopupAttributes = MAKECONCOLOR(mn_PopTextColorIdx, mn_PopBackColorIdx);
		memmove_s(pRet.Palette.ColorTable, sizeof(pRet.Palette.ColorTable),
			pPal->Colors.data(), pPal->Colors.size() * sizeof(pPal->Colors[0]));
	}

	// Task name
	_ASSERTE(pRet.TaskName.psz == nullptr);
	if (m_Args.pszTaskName && *m_Args.pszTaskName)
	{
		pRet.TaskName.Set(lstrdup(m_Args.pszTaskName).Detach());
	}
}

void CRealConsole::ShowKeyBarHint(WORD nID)
{
	AssertThis();

	if (mp_RBuf)
		mp_RBuf->ShowKeyBarHint(nID);
}

void CRealConsole::PasteExplorerPath(bool bDoCd /*= true*/, bool bSetFocus /*= true*/)
{
	AssertThis();

	CEStr pszPath = getFocusedExplorerWindowPath();

	if (pszPath)
	{
		PostPromptCmd(bDoCd, pszPath);

		if (bSetFocus)
		{
			if (!isActive(false/*abAllowGroup*/))
				gpConEmu->Activate(mp_VCon);

			if (!mp_ConEmu->isMeForeground())
				mp_ConEmu->DoMinimizeRestore(sih_SetForeground);
		}
	}
}

bool CRealConsole::PostPromptCmd(bool CD, LPCWSTR asCmd)
{
	AssertThisRet(false);
	if (!asCmd || !*asCmd)
		return false;

	bool lbRc = false;
	const DWORD nActivePid = GetActivePID();
	if (nActivePid && (mp_ABuf->m_Type == rbt_Primary))
	{
		size_t cchMax = _tcslen(asCmd);

		if (CD && isFar(true))
		{
			// Change folder using Far Macro
			cchMax = cchMax * 2 + 128;
			CEStr pszMacro;
			if (pszMacro.GetBuffer(cchMax))
			{
				_wcscpy_c(pszMacro.data(), cchMax, L"@Panel.SetPath(0,\"");
				wchar_t* pszDst = pszMacro.data() + _tcslen(pszMacro);
				LPCWSTR pszSrc = asCmd;
				while (*pszSrc)
				{
					if (*pszSrc == L'\\')
						*(pszDst++) = L'\\';
					*(pszDst++) = *(pszSrc++);
				}
				*(pszDst++) = L'"';
				*(pszDst++) = L')';
				*(pszDst) = 0;

				PostMacro(pszMacro, TRUE/*async*/);
				lbRc = true;
			}
		}
		else
		{
			LPCWSTR pszFormat = nullptr;

			// \e cd /d "%s" \n
			cchMax += 32;

			if (CD)
			{
				// \ecd /d \1\n - \e - ESC, \b - BS, \n - ENTER, \1 - "dir", \2 - "bash dir"
				pszFormat = mp_ConEmu->mp_Inside ? mp_ConEmu->mp_Inside->GetInsideSynchronizeCurDir() : nullptr;
				if (!pszFormat || !*pszFormat)
				{
					const auto* pszExe = GetActiveProcessName();
					if (pszExe && (lstrcmpi(pszExe, L"powershell.exe") == 0))
					{
						//swprintf_c(psz, cchMax/*#SECURELEN*/, L"%ccd \"%s\"%c", 27, asCmd, L'\n');
						pszFormat = L"\\ecd \\1\\n";
					}
					else if (isUnixFS() || (pszExe && (lstrcmpi(pszExe, L"bash.exe") == 0 || lstrcmpi(pszExe, L"sh.exe") == 0)))
					{
						pszFormat = L"\\e\\bcd \\2\\n";
					}
					else
					{
						//swprintf_c(psz, cchMax/*#SECURELEN*/, L"%ccd /d \"%s\"%c", 27, asCmd, L'\n');
						pszFormat = L"\\ecd /d \\1\\n";
					}
				}

				_ASSERTE(pszFormat != nullptr); // may should not be nullptr
				cchMax += _tcslen(pszFormat);
			}

			CEStr pszData;
			if (pszData.GetBuffer(cchMax))
			{
				if (CD)
				{
					_ASSERTE(pszFormat != nullptr); // may should not be nullptr
					// \ecd /d %1 - \e - ESC, \b - BS, \n - ENTER, \1 - "dir", \2 - "bash dir"

					wchar_t* pszDst = pszData.data();
					wchar_t* pszEnd = pszDst + cchMax - 1;

					while (*pszFormat && (pszDst < pszEnd))
					{
						switch (*pszFormat)
						{
						case L'\\':
							pszFormat++;
							switch (*pszFormat)
							{
							case L'e': case L'E':
								*(pszDst++) = 27;
								break;
							case L'b': case L'B':
								*(pszDst++) = 8;
								break;
							case L'n': case L'N':
								*(pszDst++) = L'\n';
								break;
							case L'1':
								if ((pszDst+3) < pszEnd)
								{
									_ASSERTE(asCmd && (*asCmd != L'"'));
									LPCWSTR pszText = asCmd;

									*(pszDst++) = L'"';

									while (*pszText && (pszDst < pszEnd))
									{
										*(pszDst++) = *(pszText++);
									}

									// Done, quote
									if (pszDst < pszEnd)
									{
										*(pszDst++) = L'"';
									}
								}
								break;
							case L'2':
								// bash style - "/c/user/dir/..."
								if ((pszDst+4) < pszEnd)
								{
									_ASSERTE(asCmd && (*asCmd != L'"') && (*asCmd != L'/'));
									CEStr path;
									DupCygwinPath(asCmd, false, GetMntPrefix(), path);
									LPCWSTR pszText = !path ? asCmd : path.c_str();

									*(pszDst++) = L'"';

									while (*pszText && (pszDst < pszEnd))
									{
										*(pszDst++) = *(pszText++);
									}

									// Done, quote
									if (pszDst < pszEnd)
									{
										*(pszDst++) = L'"';
									}
								}
								break;
							default:
								*(pszDst++) = *pszFormat;
							}
							pszFormat++;
							break;
						default:
							*(pszDst++) = *(pszFormat++);
						}
					}
					*pszDst = 0;
				} // end of "if (CD)"
				else
				{
					const auto* clearPrompt = isUnixFS() ? L"\x1B\x08" : L"\x1B";
					swprintf_c(pszData.data(), pszData.GetMaxCount(), L"%s%s%c", clearPrompt, asCmd, L'\n');
				}

				PostString(pszData.data(), pszData.GetLen(), PostStringFlags::None);
			}
		}
	}

	return lbRc;
}

void CRealConsole::OnKeysSending()
{
	AssertThis();
	if (!mp_RBuf)
		return;
	mp_RBuf->OnKeysSending();
}

// !!! Функция может менять буфер pszChars! !!! Private !!!
bool CRealConsole::PostString(wchar_t* pszChars, size_t cchCount, PostStringFlags flags)
{
	if (!pszChars || !cchCount)
	{
		_ASSERTE(pszChars && cchCount);
		gpConEmu->LogString(L"PostString fails, nothing to send");
		return false;
	}

	if (!m_ChildGui.hGuiWnd && isPaused())
	{
		gpConEmu->LogString(L"PostString skipped, console is paused");
		return false;
	}

	// If user want to type simultaneously into visible or all consoles
	const EnumVConFlags isGrouped = (flags & PostStringFlags::AllowGroup) ? mp_VCon->isGroupedInput() : evf_None;

	// Call OnKeysSending to reset top-left in the buffer if was locked (after scroll)
	if (isGrouped)
	{
		struct implKeys
		{
			static bool DoKeysSending(CVirtualConsole* pVCon, LPARAM lParam)
			{
				pVCon->RCon()->OnKeysSending();
				return true;
			}
		};
		CVConGroup::EnumVCon(isGrouped, implKeys::DoKeysSending, 0);
	}
	else
	{
		OnKeysSending();
	}

	if (CSetPgDebug::GetActivityLoggingType() == glt_Input)
	{
		// "\" --> "\\", all chars [0..31] -> "\xXX"
		CSetPgDebug::LogRConString* pCopy = (CSetPgDebug::LogRConString*)malloc(sizeof(CSetPgDebug::LogRConString) + (cchCount+1)*4*sizeof(wchar_t));
		if (pCopy)
		{
			pCopy->nServerPID = GetServerPID();
			pCopy->pszString = (wchar_t*)(pCopy+1);
			wchar_t* psz = pCopy->pszString;
			for (size_t i = 0; i < cchCount; ++i)
			{
				switch (pszChars[i])
				{
				case 0x1B:
					*(psz++) = L'\\'; *(psz++) = L'e';
					break;
				case L'\r':
					*(psz++) = L'\\'; *(psz++) = L'r';
					break;
				case L'\n':
					*(psz++) = L'\\'; *(psz++) = L'n';
					break;
				case L'\t':
					*(psz++) = L'\\'; *(psz++) = L't';
					break;
				case L'\\':
					*(psz++) = L'\\'; *(psz++) = L'\\';
					break;
				default:
					if ((unsigned)(pszChars[i]) <= 31)
					{
						wchar_t szHex[5];
						swprintf_c(szHex, L"\\x%02X", (unsigned)(pszChars[i]));
						wcscpy_s(psz, 5, szHex);
						psz += 4;
					}
					else
					{
						*(psz++) = pszChars[i];
					}
				}
			}
			*(psz) = 0;
			PostMessage(gpSetCls->GetPage(thi_Debug), DBGMSG_LOG_ID, DBGMSG_LOG_STR_MAGIC, reinterpret_cast<LPARAM>(pCopy));
		}
	}

	// Prepare character buffer to post data
	wchar_t szLog[80];
	wchar_t* pszEnd = pszChars + cchCount;
	INPUT_RECORD r[2];
	MSG64* pirChars = (MSG64*)malloc(sizeof(MSG64)+cchCount*2*sizeof(MSG64::MsgStr));
	if (!pirChars)
	{
		AssertMsg(L"Can't allocate (INPUT_RECORD* pirChars)!");
		gpConEmu->LogString(L"PostString fails, memory allocation failed");
		return false;
	}

	bool lbRc = false;
	const bool lbIsFar = isFar();
	const bool encodeXterm = (flags & PostStringFlags::XTermSequence) && (pszChars[0] == 0x1B) && (cchCount > 1);

	size_t cchSucceeded = 0;
	MSG64::MsgStr* pir = pirChars->msg;
	for (wchar_t* pch = pszChars; pch < pszEnd; pch++, pir+=2)
	{
		_ASSERTE(*pch); // оно ASCIIZ

		if (pch[0] == L'\r' && pch[1] == L'\n')
		{
			pch++; // "\r\n" - слать "\n"
			if (pch[1] == L'\n')
				pch++; // buggy line returns "\r\n\n"
		}

		// "слать" в консоль '\r' а не '\n' чтобы "Enter" нажался.
		if (pch[0] == L'\n')
		{
			*pch = L'\r'; // буфер наш, что хотим - то и делаем
		}

		TranslateKeyPress(0, 0, *pch, -1, r, r+1);

		if (*pch == 0x7F)
		{
			// gh-641: Posting `0x7F` (which is <BS> on xterm) causes LEFT_CTRL_PRESSED in dwControlKeyState
			//   we are sending <BS>, but not a <Ctrl>-<BS>
			r[0].Event.KeyEvent.dwControlKeyState &= ~(LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED|SHIFT_PRESSED);
			r[1].Event.KeyEvent.dwControlKeyState = r[0].Event.KeyEvent.dwControlKeyState;
		}

		// 130822 - Japanese+iPython - (wVirtualKeyCode!=0) fails with pyreadline
		if (lbIsFar && (r->EventType == KEY_EVENT) && !r->Event.KeyEvent.wVirtualKeyCode)
		{
			// Если в RealConsole валится VK=0, но его фар игнорит
			r[0].Event.KeyEvent.wVirtualKeyCode = VK_PROCESSKEY;
			r[1].Event.KeyEvent.wVirtualKeyCode = VK_PROCESSKEY;
		}

		// To simplify buffering of escape sequences on connector side
		if (encodeXterm && (r->EventType == KEY_EVENT))
		{
			const WORD sequenceMark{ 255 };
			if (pch == pszChars)
			{
				r[0].Event.KeyEvent.wVirtualScanCode = sequenceMark;
			}
			else if (pch + 1 == pszEnd)
			{
				r[1].Event.KeyEvent.wVirtualScanCode = sequenceMark;
			}
		}

		PackInputRecord(r, pir);
		PackInputRecord(r+1, pir+1);
		cchSucceeded += 2;

		//// функция сама разберется что посылать
		//while (!PostKeyPress(0, 0, *pch))
		//{
		//	wcscpy_c(szMsg, L"Key press sending failed!\nTry again?");

		//	if (MessageBox(ghWnd, szMsg, GetTitle(), MB_RETRYCANCEL) != IDRETRY)
		//	{
		//		goto wrap;
		//	}

		//	// try again
		//}
	}

	if (cchSucceeded)
	{
		swprintf_c(szLog, L"PostString was prepared %u key events", (DWORD)cchSucceeded);
		gpConEmu->LogString(szLog);

		if (isGrouped)
		{
			struct implPost
			{
				MSG64* pirChars;
				size_t cchSucceeded;

				static bool DoPost(CVirtualConsole* pVCon, LPARAM lParam)
				{
					implPost* p = (implPost*)lParam;
					pVCon->RCon()->PostConsoleEventPipe(p->pirChars, p->cchSucceeded);
					return true;
				}
			} impl = {pirChars, cchSucceeded};
			CVConGroup::EnumVCon(isGrouped, implPost::DoPost, (LPARAM)&impl);
			lbRc = true;
		}
		else
		{
			lbRc = PostConsoleEventPipe(pirChars, cchSucceeded);
		}
	}
	else
	{
		gpConEmu->LogString(L"PostString fails, cchSucceeded is null");
	}

	if (!lbRc)
	{
		MBox(L"Key press sending failed!");
		gpConEmu->LogString(L"PostConsoleEventPipe fails");
	}

	//lbRc = true;
	//wrap:
	free(pirChars);
	return lbRc;
}

bool CRealConsole::PostKeyPress(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode /*= -1*/)
{
	AssertThisRet(false);

	if (!hConWnd)
	{
		_ASSERTE(FALSE && "Must not get here, use mpsz_PostCreateMacro to postpone macroses");
		return false;
	}

	if (!m_ChildGui.hGuiWnd && isPaused())
	{
		gpConEmu->LogString(L"PostKeyPress skipped, console is paused");
		return false;
	}

	INPUT_RECORD r[2] = {{KEY_EVENT},{KEY_EVENT}};
	TranslateKeyPress(vkKey, dwControlState, wch, ScanCode, r, r+1);

	bool lbPress = PostConsoleEvent(r);
	bool lbDepress = lbPress && PostConsoleEvent(r+1);
	return (lbPress && lbDepress);
}

//void CRealConsole::TranslateKeyPress(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode, INPUT_RECORD& rDown, INPUT_RECORD& rUp)
//{
//	// Может приходить запрос на отсылку даже если текущий буфер НЕ rbt_Primary,
//	// например, при начале выделения и автоматическом переключении на альтернативный буфер
//
//	if (!vkKey && !dwControlState && wch)
//	{
//		USHORT vk = VkKeyScan(wch);
//		if (vk && (vk != 0xFFFF))
//		{
//			vkKey = (vk & 0xFF);
//			vk = vk >> 8;
//			if ((vk & 7) == 6)
//			{
//				// For keyboard layouts that use the right-hand ALT key as a shift
//				// key (for example, the French keyboard layout), the shift state is
//				// represented by the value 6, because the right-hand ALT key is
//				// converted internally into CTRL+ALT.
//				dwControlState |= SHIFT_PRESSED;
//			}
//			else
//			{
//				if (vk & 1)
//					dwControlState |= SHIFT_PRESSED;
//				if (vk & 2)
//					dwControlState |= LEFT_CTRL_PRESSED;
//				if (vk & 4)
//					dwControlState |= LEFT_ALT_PRESSED;
//			}
//		}
//	}
//
//	if (ScanCode == -1)
//		ScanCode = MapVirtualKey(vkKey, 0/*MAPVK_VK_TO_VSC*/);
//
//	INPUT_RECORD r = {KEY_EVENT};
//	r.Event.KeyEvent.bKeyDown = TRUE;
//	r.Event.KeyEvent.wRepeatCount = 1;
//	r.Event.KeyEvent.wVirtualKeyCode = vkKey;
//	r.Event.KeyEvent.wVirtualScanCode = ScanCode;
//	r.Event.KeyEvent.uChar.UnicodeChar = wch;
//	r.Event.KeyEvent.dwControlKeyState = dwControlState;
//	rDown = r;
//
//	TODO("Может нужно в dwControlKeyState применять модификатор, если он и есть vkKey?");
//
//	r.Event.KeyEvent.bKeyDown = FALSE;
//	r.Event.KeyEvent.dwControlKeyState = dwControlState;
//	rUp = r;
//}

bool CRealConsole::PostKeyUp(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode /*= -1*/)
{
	AssertThisRet(false);

	if (ScanCode == -1)
		ScanCode = MapVirtualKey(vkKey, 0/*MAPVK_VK_TO_VSC*/);

	if (dwControlState == 0)
	{
		switch (vkKey)
		{
		case VK_RMENU:
			dwControlState = ENHANCED_KEY;  // -V796
		case VK_LMENU:
			vkKey = VK_MENU;
			break;

		case VK_RCONTROL:
			dwControlState = ENHANCED_KEY;  // -V796
		case VK_LCONTROL:
			vkKey = VK_CONTROL;
			break;
		}
	}
	else
	{
		// VK_MENU is expected with Alt+Numbers: char is posted with Alt release
		_ASSERTE(vkKey!=VK_LMENU && vkKey!=VK_RMENU && vkKey!=VK_LCONTROL && vkKey!=VK_RCONTROL);
	}

	INPUT_RECORD r = {KEY_EVENT};
	r.Event.KeyEvent.bKeyDown = FALSE;
	r.Event.KeyEvent.wRepeatCount = 1;
	r.Event.KeyEvent.wVirtualKeyCode = vkKey;
	r.Event.KeyEvent.wVirtualScanCode = ScanCode;
	r.Event.KeyEvent.uChar.UnicodeChar = wch;
	r.Event.KeyEvent.dwControlKeyState = dwControlState;
	bool lbOk = PostConsoleEvent(&r);
	return lbOk;
}

// Used for previously executed by ConEmuHk: CECMD_BSDELETEWORD & case CECMD_MOUSECLICK
bool CRealConsole::IsPromptActionAllowed(bool bFromMouse, const AppSettings* pApp)
{
	AssertThisRet(false);
	if (!mp_RBuf || !pApp)
		return false;

	// Some global checks to prevent translation
	if (m_ChildGui.hGuiWnd || isPaused() || isFar())
		return false;

	// Don't allow Ctrl+BS and prompt-mouse-click features if application
	// has not reported "I'm in the prompt"
	COORD crPrompt = {};
	if (!QueryPromptStart(&crPrompt))
		return false;

	// Change prompt position with mouse click
	if (bFromMouse)
	{
		bool bForce = (pApp->CTSClickPromptPosition() == 1);
		DWORD nConInMode = mp_RBuf->GetConInMode();
		// If application has set ENABLE_MOUSE_INPUT flag, than
		// bypass clicks without changes to its input queue
		if (!bForce && ((nConInMode & ENABLE_MOUSE_INPUT) == ENABLE_MOUSE_INPUT))
			return false;
	}

	// Allow the feature
	return true;
}

int CRealConsole::EvalPromptCtrlBSCount(const AppSettings* pApp)
{
	COORD crCursor = {};
	mp_RBuf->GetCursorInfo(&crCursor, nullptr);
	// Primary buffer works with visual coords
	crCursor = mp_RBuf->BufferToScreen(crCursor);

	CRConDataGuard data;
	ConsoleLinePtr line = {};
	if (!mp_RBuf->GetConsoleLine(crCursor.Y, data, line))
		return 0;

	COORD crPrompt = {};
	if (!QueryPromptStart(&crPrompt))
		return 0;

	// Only RIGHT brackets here to be sure that `(x86)` will be deleted including left bracket
	wchar_t cBreaks[] = L"\x20\xA0>])}$.,/\\\"";
	_ASSERTE(cBreaks[0]==ucSpace && cBreaks[1]==ucNoBreakSpace);

	int iBSCount = 0;
	COORD crFrom = crCursor;
	bool bFirst = true;
	while (crFrom.Y >= 0)
	{
		int i = crFrom.X - 1;

		if (bFirst)
		{
			// Delete all `spaces` first
			while ((i >= 0) && ((line.pChar[i] == ucSpace) || (line.pChar[i] == ucNoBreakSpace)))
				iBSCount++, i--;
			bFirst = false;
		}

		// delimiters
		while ((i >= 0) && wcschr(cBreaks+2, line.pChar[i]))
		{
			iBSCount++; i--;
		}
		// and all `NON-spaces`
		bool prev_line = true;
		while (i >= 0)
		{
			if (wcschr(cBreaks, line.pChar[i]))
			{
				prev_line = false; break;
			}
			iBSCount++; i--;
		}

		// Take line above?
		if (!prev_line || !crFrom.Y || !data->GetConsoleLine(crFrom.Y-1, line))
			break;
		--crFrom.Y;
		crFrom.X = line.nLen;
	}

	return iBSCount;
}

int CRealConsole::EvalPromptLeftRightCount(const AppSettings* pApp, COORD crMouse, WORD& vkKey)
{
	COORD crCursor = {};
	mp_RBuf->GetCursorInfo(&crCursor, nullptr);
	if (crCursor == crMouse)
		return 0; // nothing to do

	// Query cursor's line
	CRConDataGuard data;
	ConsoleLinePtr line = {};
	if (!mp_RBuf->GetConsoleLine(mp_RBuf->BufferToScreen(crCursor).Y, data, line))
		return 0;

	// must be already checked, just query the coords
	COORD crPrompt = {};
	if (!QueryPromptStart(&crPrompt))
		return 0;
	// Don't change cursor position if user clicks line above the prompt
	if (crMouse.Y < crPrompt.Y)
		return 0;

	// get proper coords from crMouse/crPrompt
	bool bForward = true;
	COORD crClick = crMouse;
	if ((CoordCompare(crClick, crCursor) < 0)
		&& (CoordCompare(crClick, crPrompt) < 0))
	{
		// we shall not go beyond the prompt start
		crClick = crPrompt;
	}

	// evaluate and check *from* and *to* coords (min/max actually)
	COORD crMin, crMax;
	switch (CoordCompare(crClick, crCursor))
	{
	case -1:
		crMin = mp_RBuf->BufferToScreen(crClick);
		crMax = mp_RBuf->BufferToScreen(crCursor);
		bForward = false; vkKey = VK_LEFT;
		break;
	case 1:
		crMin = mp_RBuf->BufferToScreen(crCursor);
		crMax = mp_RBuf->BufferToScreen(crClick);
		bForward = true; vkKey = VK_RIGHT;
		break;
	default:
		// Nothing to do
		return 0;
	}
	if (crMin.X < 0 || crMin.Y < 0)
	{
		MBoxAssert(crMin.X >= 0 && crMin.Y >= 0);
		return 0;
	}
	if (crMax.X < 0 || crMax.Y < 0)
	{
		MBoxAssert(crMax.X >= 0 && crMax.Y >= 0);
		return 0;
	}

	int nKeyCount = 0;
	int nBashSpaces = 0;
	bool bBashMargin = pApp->CTSBashMargin();

	/* *** prompt sample *** *
	 | C:\> git log --graph "--date=format:%y%m%d: |
	 | %H%M" "--pretty=format:%C(auto)%h%d %C(bold |
	 | blue)%an %Cgreen%ad  %Creset%s" %*          |
	 * *** end of sample *** */

	auto isDirty = [](const wchar_t* pLine, int minX, int maxX)
	{
		bool dirty = false;
		for (int x = minX; x < maxX; ++x)
		{
			if (!isSpace(pLine[x]))
			{
				dirty = true; break;
			}
		}
		return dirty;
	};

	// When going forward and *clicked* line is empty - do nothing
	// to avoid unexpected cursor movement when user clicks on empty field of terminal
	if (crMax.Y > crMin.Y)
	{
		if (!data->GetConsoleLine(crMax.Y, line) || line.nLen <= 0 || !line.pChar)
			return 0;
		if (!isDirty(line.pChar, 0, line.nLen))
			return 0;
	}

	int prevLineSpaces = 0;

	// Here we need to count characters
	// *from* current text cursor position (crCursor)
	// *to*   clicked position (crClick)
	for (int Y = crMin.Y; Y <= crMax.Y; ++Y)
	{
		// acquire the line
		if (!data->GetConsoleLine(Y, line))
			line.pChar = nullptr;
		// validate line data
		if (line.nLen <= 0 || !line.pChar)
		{
			_ASSERTE(line.nLen>0 && line.pChar);
			break;
		}

		// Line end/start position
		int minX = (Y == crMin.Y) ? crMin.X : 0;
		int maxX = (Y == crMax.Y) ? std::min((int)crMax.X, line.nLen) : line.nLen;
		if (minX < 0 || maxX <= 0 || minX >= maxX || maxX > line.nLen)
		{
			MBoxAssert(minX >= 0 && maxX >= 0 && maxX >= minX && maxX < line.nLen);
			continue; // next line
		}

		// Does line contain any text?
		bool dirty = isDirty(line.pChar, minX, maxX);

		// Don't try do advance cursor forward if there are no text anymore
		if (bForward && !dirty)
		{
			// If prev line ended with spaces, truncate it from our result
			_ASSERTE(prevLineSpaces <= nKeyCount);
			nKeyCount -= prevLineSpaces;
			break;
		}

		// Some shells take into account some terminals inability to set cursor *after* last cell,
		// and able to optionally wrap lines at (width-1), so last cell of the each prompt line is
		// always empty. We shall not count these spaces.
		if (bBashMargin && (maxX == line.nLen))
		{
			if (isSpace(line.pChar[maxX-1]))
			{
				--maxX;
				++nBashSpaces;
			}
			else
			{
				// a char in the last cell means the shell utilizes whole width of the terminal
				bBashMargin = false;
				nKeyCount += nBashSpaces;
				nBashSpaces = 0;
			}
		}

		// Trailing spaces on the line
		prevLineSpaces = 0;
		if (bForward)
		{
			for (int x = maxX - 1; x >= minX && isSpace(line.pChar[x]); --x)
				++prevLineSpaces;
			if (Y == crMax.Y)
				maxX -= prevLineSpaces;
		}

		// The line is dirty?
		if (maxX > minX)
		{
			nKeyCount += (maxX - minX);
		}
	}

	return nKeyCount;
}

bool CRealConsole::ChangePromptPosition(const AppSettings* pApp, COORD crMouse)
{
	// must be already checked, just query the coords
	COORD crPrompt = {};
	if (!QueryPromptStart(&crPrompt))
		return false;

	WORD vkKey = VK_LEFT;
	int nKeyCount = EvalPromptLeftRightCount(pApp, crMouse, vkKey);

	if (nKeyCount > 0)
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"Changing prompt position by LClick: {%i,%i} VK=%u Count=%u", crMouse.X, crMouse.Y, vkKey, nKeyCount);
		DEBUGSTRCLICKPOS(szInfo);
		LogString(szInfo);

		INPUT_RECORD r[2] = {};
		const DWORD dwControlState = 0;
		TranslateKeyPress(vkKey, dwControlState, 0, -1, &r[0], &r[1]);
		for (int repeats = nKeyCount; repeats > 0;)
		{
			INPUT_RECORD rs[2] = {r[0], r[1]};
			rs[0].Event.KeyEvent.wRepeatCount = std::min<int>(255, repeats);
			repeats -= rs[0].Event.KeyEvent.wRepeatCount;
			PostConsoleEvent(&rs[0]);
			PostConsoleEvent(&rs[1]);
		}
	}

	return true;
}

bool CRealConsole::DeleteWordKeyPress(bool bTestOnly /*= false*/)
{
	// cygwin/msys connector - they are configured through .inputrc
	DWORD nTermPID = (GetTermType() == te_xterm) ? GetTerminalPID() : 0;
	if (nTermPID)
	{
		return false;
	}

	DWORD nActivePID = GetActivePID();
	if (!nActivePID || (mp_ABuf->m_Type != rbt_Primary) || isFar() || isNtvdm())
	{
		return false;
	}

	const AppSettings* pApp = gpSet->GetAppSettings(GetActiveAppSettingsId());
	if (!pApp || !pApp->CTSDeleteLeftWord()
		|| !IsPromptActionAllowed(false, pApp))
	{
		return false;
	}

	if (bTestOnly)
	{
		return true;
	}

	int iBSCount = EvalPromptCtrlBSCount(pApp);
	if (iBSCount > 0)
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"Delete word by Ctrl+BS: VK=%u Count=%u", VK_BACK, iBSCount);
		DEBUGSTRCTRLBS(szInfo);
		LogString(szInfo);

		INPUT_RECORD r[2] = {};
		const DWORD dwControlState = 0;
		TranslateKeyPress(VK_BACK, dwControlState, (wchar_t)VK_BACK, -1, &r[0], &r[1]);
		r[0].Event.KeyEvent.wRepeatCount = iBSCount;
		PostConsoleEvent(&r[0]);
		PostConsoleEvent(&r[1]);
	}

	return true;
}

bool CRealConsole::PostLeftClickSync(COORD crDC)
{
	AssertThisRet(false);

	const DWORD nFarPID = GetFarPID();
	if (!nFarPID)
	{
		_ASSERTE(nFarPID != 0);
		return false;
	}

	bool lbOk = false;
	const COORD crMouse = ScreenToBuffer(mp_VCon->ClientToConsole(crDC.X, crDC.Y));
	CConEmuPipe pipe(nFarPID, CONEMUREADYTIMEOUT);

	if (pipe.Init(_T("CConEmuMain::EMenu"), TRUE))
	{
		mp_ConEmu->DebugStep(_T("PostLeftClickSync: Waiting for result (10 sec)"));

		DWORD nClickData[2] = {TRUE, static_cast<DWORD>(MAKELONG(crMouse.X, crMouse.Y))};

		if (!pipe.Execute(CMD_LEFTCLKSYNC, nClickData, sizeof(nClickData)))
		{
			// ReSharper disable once StringLiteralTypo
			LogString("pipe.Execute(CMD_LEFTCLKSYNC) failed");
		}
		else
		{
			lbOk = true;
		}

		mp_ConEmu->DebugStep(nullptr);
	}

	return lbOk;
}

bool CRealConsole::PostCtrlBreakEvent(DWORD nEvent, DWORD nGroupId)
{
	AssertThisRet(false);

	if (mn_MainSrv_PID == 0 || !m_ConsoleMap.IsValid())
		return false; // Сервер еще не стартовал. События будут пропущены...

	bool lbRc = false;

	// #CTRL_BREAK Don't call CECMD_CTRLBREAK if we know that conhost is inside ReadLine

	_ASSERTE(nEvent == CTRL_C_EVENT/*0*/ || nEvent == CTRL_BREAK_EVENT/*1*/);

	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_CTRLBREAK, sizeof(CESERVER_REQ_HDR)+2*sizeof(DWORD));
	if (pIn)
	{
		pIn->dwData[0] = nEvent;
		pIn->dwData[1] = nGroupId;

		CESERVER_REQ* pOut = ExecuteSrvCmd(GetServerPID(false), pIn, ghWnd);
		if (pOut->DataSize() >= sizeof(DWORD))
			lbRc = (pOut->dwData[0] != 0);
		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
	}

	return lbRc;
}

bool CRealConsole::PostConsoleEvent(INPUT_RECORD* piRec, bool bFromIME /*= false*/)
{
	AssertThisRet(false);
	if (!piRec)
		return false;

	if (mn_MainSrv_PID == 0 || !m_ConsoleMap.IsValid())
		return false; // Сервер еще не стартовал. События будут пропущены...

	if (!m_ChildGui.hGuiWnd && isPaused())
	{
		gpConEmu->LogString(L"PostConsoleEvent skipped, console is paused");
		// Stop "Paused" mode by "Esc"
		if (piRec->EventType == KEY_EVENT && !piRec->Event.KeyEvent.bKeyDown
			&& piRec->Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)
			Pause(CEPause_Off);
		return false;
	}

	bool lbRc = false;

	// In ChildGui mode - all events are posted into the window
	if (m_ChildGui.hGuiWnd)
	{
		if (piRec->EventType == KEY_EVENT)
		{
			UINT msg = bFromIME ? WM_IME_CHAR : WM_CHAR;
			WPARAM wParam = 0;
			LPARAM lParam = 0;

			if (piRec->Event.KeyEvent.bKeyDown && piRec->Event.KeyEvent.uChar.UnicodeChar)
				wParam = piRec->Event.KeyEvent.uChar.UnicodeChar;

			if (wParam || lParam)
			{
				lbRc = PostConsoleMessage(m_ChildGui.hGuiWnd, msg, wParam, lParam);
			}
		}

		return lbRc;
	}

	// XTerm keyboard and mouse emulation
	if (m_Term.Term && ProcessXtermSubst(*piRec))
	{
		return true;
	}

	if (piRec->EventType == MOUSE_EVENT)
	{
		#ifdef _DEBUG
		static DWORD nLastBtnState;
		#endif
		//WARNING!!! Тут проблема следующая.
		// Фаровский AltIns требует получения MOUSE_MOVE в той же координате, где прошел клик.
		//  Иначе граббинг начинается не с "кликнутой" а со следующей позиции.
		// В других случаях наблюдаются проблемы с кликами. Например, в диалоге
		//  UCharMap. При его минимизации, если кликнуть по кнопке минимизации
		//  и фар получит MOUSE_MOVE - то диалог закроется при отпускании кнопки мышки.
		//2010-07-12 обработка вынесена в CRealConsole::OnMouse с учетом GUI курсора по пикселам

		#ifdef _DEBUG
		if (piRec->Event.MouseEvent.dwButtonState == RIGHTMOST_BUTTON_PRESSED
			&& ((piRec->Event.MouseEvent.dwControlKeyState & 9) != 9))
		{
			nLastBtnState = piRec->Event.MouseEvent.dwButtonState;
		}
		#endif

		if (((m_Mouse.rLastEvent.dwEventFlags & MOUSE_MOVED) && !(piRec->Event.MouseEvent.dwEventFlags & MOUSE_MOVED))
			|| (!(m_Mouse.rLastEvent.dwEventFlags & MOUSE_MOVED)
					&& (0 != memcmp(&m_Mouse.rLastEvent, &piRec->Event.MouseEvent, sizeof(m_Mouse.rLastEvent))))
			)
		{
			#ifdef _DEBUG
			const MOUSE_EVENT_RECORD& me = piRec->Event.MouseEvent;
			wchar_t szDbg[120];
			swprintf_c(szDbg, L"RCon::Mouse(Send) event at {%i,%i} Btns=x%X Keys=x%X %s",
				me.dwMousePosition.X, me.dwMousePosition.Y, me.dwButtonState, me.dwControlKeyState,
				(me.dwEventFlags & MOUSE_MOVED) ? L"Moved" : L"");
			DEBUGSTRINPUTLL(szDbg);
			#endif
		}

		// Store last mouse event
		m_Mouse.rLastEvent = piRec->Event.MouseEvent;
		//m_Mouse.LastMouse.dwMousePosition   = piRec->Event.MouseEvent.dwMousePosition;
		//m_Mouse.LastMouse.dwEventFlags      = piRec->Event.MouseEvent.dwEventFlags;
		//m_Mouse.LastMouse.dwButtonState     = piRec->Event.MouseEvent.dwButtonState;
		//m_Mouse.LastMouse.dwControlKeyState = piRec->Event.MouseEvent.dwControlKeyState;
		#ifdef _DEBUG
		nLastBtnState = piRec->Event.MouseEvent.dwButtonState;
		#endif
	}
	else if (piRec->EventType == KEY_EVENT)
	{
		// github#19: Code moved to ../ConEmuCD/Queue.cpp

		if (!piRec->Event.KeyEvent.wRepeatCount)
		{
			_ASSERTE(piRec->Event.KeyEvent.wRepeatCount!=0);
			piRec->Event.KeyEvent.wRepeatCount = 1;
		}

		// Keyboard/Output/Delay performance
		if (piRec->Event.KeyEvent.bKeyDown
			&& ((piRec->Event.KeyEvent.uChar.UnicodeChar >= L' ')
				|| (piRec->Event.KeyEvent.wVirtualKeyCode >= VK_PRIOR && piRec->Event.KeyEvent.wVirtualKeyCode <= VK_DOWN)
				)
			)
		{
			gpSetCls->Performance(tPerfKeyboard, FALSE);
		}
	}
	else if (piRec->EventType == 0)
	{
		_ASSERTE(piRec->EventType != 0);
	}

	if (CSetPgDebug::GetActivityLoggingType() == glt_Input)
	{
		//INPUT_RECORD *prCopy = (INPUT_RECORD*)calloc(sizeof(INPUT_RECORD),1);
		CESERVER_REQ_PEEKREADINFO* pCopy = (CESERVER_REQ_PEEKREADINFO*)malloc(sizeof(CESERVER_REQ_PEEKREADINFO));
		if (pCopy)
		{
			pCopy->nCount = 1;
			pCopy->bMainThread = TRUE;
			pCopy->cPeekRead = 'S';
			pCopy->cUnicode = 'W';
			pCopy->Buffer[0] = *piRec;
			PostMessage(gpSetCls->GetPage(thi_Debug), DBGMSG_LOG_ID, DBGMSG_LOG_INPUT_MAGIC, reinterpret_cast<LPARAM>(pCopy));
		}
	}

	// ***
	{
		MSG64 msg = {sizeof(msg), 1};

		if (PackInputRecord(piRec, msg.msg))
		{
			if (isLogging())
				LogInput(piRec);

			_ASSERTE(msg.msg[0].message!=0);
			//if (mb_UseOnlyPipeInput) {
			lbRc = PostConsoleEventPipe(&msg);

			#ifdef _DEBUG
			if (gbInSendConEvent)
			{
				_ASSERTE(!gbInSendConEvent);
			}
			#endif
		}
		else
		{
			mp_ConEmu->DebugStep(L"ConEmu: PackInputRecord failed!");
		}
	}

	return lbRc;
}

// Highlight icon (flashing actually) on modified console contents
bool CRealConsole::isHighlighted()
{
	AssertThisRet(false);
	if (!tabs.bConsoleDataChanged)
		return false;

	if (mp_VCon->isVisible())
	{
		tabs.bConsoleDataChanged = false;
		tabs.nFlashCounter = 0;
		return false;
	}

	if (!gpSet->nTabFlashChanged)
		return false;

	return tabs.bConsoleDataChanged;
}

// Вызывается при изменения текста/атрибутов в реальной консоли (mp_RBuf)
void CRealConsole::OnConsoleDataChanged()
{
	AssertThis();
	// Do not take into account gpSet->nTabFlashChanged
	// because bConsoleDataChanged may be used in tab template
	if (mp_VCon->isVisible())
		return;

	if (!mb_WasVisibleOnce && (GetRunTime() < HIGHLIGHT_RUNTIME_MIN))
		return;
	// Don't annoy with flashing after typing in cmd prompt:
	// cmd -new_console
	// The active console was deactivated just now, no need to inform user about "changes".
	DWORD nInvisibleTime = mn_DeactivateTick ? (GetTickCount() - mn_DeactivateTick) : 0;
	if (nInvisibleTime < HIGHLIGHT_INVISIBLE_MIN)
		return;
	// Don't flash in Far while it is showing progress
	// That is useless because tab/caption/task-progress is changed during operation
	if ((m_Progress.Progress >= 0) && isFar())
		return;

	if (!tabs.bConsoleDataChanged)
	{
		// Highlighting will be updated in ::OnTimerCheck
		tabs.nFlashCounter = 0;
		tabs.bConsoleDataChanged = true;
		// But tab text labels - must be updated specially
		if (gpSet->szTabModifiedSuffix[0])
			mp_ConEmu->RequestPostUpdateTabs();
	}
}

void CRealConsole::OnTimerCheck()
{
	AssertThis();

	if (InCreateRoot() || InRecreate())
		return;

	if (!mb_WasVisibleOnce && mp_VCon->isVisible())
		mb_WasVisibleOnce = true;

	if (mn_StartTick)
		GetRunTime();

	//Highlighting
	if (isHighlighted())
	{
		if ((gpSet->nTabFlashChanged < 0) || (tabs.nFlashCounter < gpSet->nTabFlashChanged))
		{
			InterlockedIncrement(&tabs.nFlashCounter);
			mp_ConEmu->mp_TabBar->HighlightTab(tabs.mp_ActiveTab->Tab(), (tabs.nFlashCounter & 1) != 0);
		}
	}

	//TODO: На проверку пайпов а не хэндла процесса
	if (!mh_MainSrv)
		return;

	if (!isServerAlive())
	{
		_ASSERTE((mn_TermEventTick != 0 && mn_TermEventTick != (DWORD)-1) && "Server was terminated, StopSignal was not called");
		StopSignal();
		return;
	}

	// А это наверное вообще излишне, проверим под дебагом, как жить будет
#ifdef _DEBUG
	if (hConWnd && !IsWindow(hConWnd))
	{
		_ASSERTE((mn_TermEventTick != 0 && mn_TermEventTick != (DWORD)-1) && "Console window was destroyed, StopSignal was not called");
		return;
	}
#endif
}

void CRealConsole::OnSelectionTimerCheck()
{
	AssertThis();

	if (InCreateRoot() || InRecreate() || !mp_ABuf)
		return;

	if (mp_ABuf->isSelectionPresent())
	{
		mp_ABuf->OnTimerCheckSelection();
	}
}

void CRealConsole::MonitorAssertTrap()
{
	mb_MonitorAssertTrap = true;
	SetMonitorThreadEvent();
}

enum
{
	IDEVENT_TERM = 0,           // Завершение нити/консоли/conemu
	IDEVENT_MONITORTHREADEVENT, // Используется, чтобы вызвать Update & Invalidate
	IDEVENT_UPDATESERVERACTIVE, // Дернуть pRCon->UpdateServerActive()
	IDEVENT_SWITCHSRV,          // пришел запрос от альт.сервера переключиться на него
	IDEVENT_SERVERPH,           // ConEmuC.exe process handle (server)
	EVENTS_COUNT
};

DWORD CRealConsole::MonitorThread(LPVOID lpParameter)
{
	DWORD nWait = IDEVENT_TERM;
	CRealConsole* pRCon = (CRealConsole*)lpParameter;
	bool bDetached = (pRCon->m_Args.Detached == crb_On) && !pRCon->mb_ProcessRestarted && !pRCon->mn_InRecreate;
	bool lbChildProcessCreated = FALSE;

	pRCon->UpdateStartState(rss_MonitorStarted);

	pRCon->LogString("MonitorThread started");

	pRCon->SetConStatus(bDetached ? L"Detached" : L"Initializing RealConsole...", cso_ResetOnConsoleReady|cso_Critical);

	if (pRCon->mb_NeedStartProcess)
	{
		#ifdef _DEBUG
		if (pRCon->mh_MainSrv)
		{
			_ASSERTE(pRCon->mh_MainSrv==nullptr);
		}
		#endif

		HANDLE hWait[] = {pRCon->mh_TermEvent, pRCon->mh_StartExecuted};
		DWORD nWait = WaitForMultipleObjects(countof(hWait), hWait, FALSE, INFINITE);
		if ((nWait == WAIT_OBJECT_0) || !pRCon->mb_StartResult)
		{
			//_ASSERTE(FALSE && "Failed to start console?"); -- no need in debug
			goto wrap;
		}

		#ifdef _DEBUG
		int nNumber = pRCon->mp_ConEmu->isVConValid(pRCon->mp_VCon);
		UNREFERENCED_PARAMETER(nNumber);
		#endif
	}

	//_ASSERTE(pRCon->mh_ConChanged!=nullptr);
	// Пока нить запускалась - произошел "аттач" так что все нормально...
	//_ASSERTE(pRCon->mb_Detached || pRCon->mh_MainSrv!=nullptr);

	// а тут мы будем читать консоль...
	nWait = pRCon->MonitorThreadWorker(bDetached, lbChildProcessCreated);

wrap:

	if (nWait == IDEVENT_SERVERPH)
	{
		//ShutdownGuiStep(L"### Server was terminated\n");
		DWORD nExitCode = 999;
		GetExitCodeProcess(pRCon->mh_MainSrv, &nExitCode);
		DWORD nSrvWait = WaitForSingleObject(pRCon->mh_MainSrv, 0);

		wchar_t szErrInfo[255], szCode[30];
		swprintf_c(szCode, (nExitCode > 0 && nExitCode <= 2048) ? L"%i" : L"0x%08X", nExitCode);
		swprintf_c(szErrInfo,
			L"Server process was terminated, ExitCode=%s, Wait=%u",
			szCode, nSrvWait);
		if (nExitCode == 0xC000013A)
			wcscat_c(szErrInfo, L" (by Ctrl+C)");

		ShutdownGuiStep(szErrInfo);

		if (nExitCode == 0)
		{
			pRCon->SetConStatus(nullptr);
		}
		else
		{
			pRCon->SetConStatus(szErrInfo);
		}
	}

	// Только если процесс был успешно запущен
	if (pRCon->mb_StartResult)
	{
		// А это чтобы не осталось висеть окно ConEmu, раз всё уже закрыто
		pRCon->mp_ConEmu->OnRConStartedSuccess(nullptr);
	}

	ShutdownGuiStep(L"StopSignal");

	if (!pRCon->mb_InDetach && (pRCon->mn_InRecreate != (DWORD)-1))
	{
		pRCon->StopSignal();
	}

	ShutdownGuiStep(L"Leaving MonitorThread");
	return 0;
}

int CRealConsole::WorkerExFilter(unsigned int code, struct _EXCEPTION_POINTERS *ep, LPCTSTR szFile, UINT nLine)
{
	wchar_t szInfo[100];
	swprintf_c(szInfo, L"Exception 0x%08X triggered in CRealConsole::MonitorThreadWorker TID=%u", code, GetCurrentThreadId());

	AssertBox(szInfo, szFile, nLine, ep);

	return EXCEPTION_EXECUTE_HANDLER;
}

DWORD CRealConsole::MonitorThreadWorker(bool bDetached, bool& rbChildProcessCreated)
{
	rbChildProcessCreated = false;

	mb_MonitorAssertTrap = false;

	_ASSERTE(IDEVENT_SERVERPH==(EVENTS_COUNT-1)); // Должен быть последним хэндлом!
	HANDLE hEvents[EVENTS_COUNT];
	_ASSERTE(EVENTS_COUNT==countof(hEvents)); // проверить размерность

	hEvents[IDEVENT_TERM] = mh_TermEvent;
	hEvents[IDEVENT_MONITORTHREADEVENT] = mh_MonitorThreadEvent; // Использовать, чтобы вызвать Update & Invalidate
	hEvents[IDEVENT_UPDATESERVERACTIVE] = mh_UpdateServerActiveEvent; // Дернуть UpdateServerActive()
	hEvents[IDEVENT_SWITCHSRV] = mh_SwitchActiveServer;
	hEvents[IDEVENT_SERVERPH] = mh_MainSrv;
	//HANDLE hAltServerHandle = nullptr;

	DWORD  nEvents = countof(hEvents);

	// Скорее всего будет nullptr, если запуск идет через ShellExecuteEx(runas)
	if (hEvents[IDEVENT_SERVERPH] == nullptr)
		nEvents --;

	DWORD  nWait = 0, nSrvWait = -1, nAcvWait = -1;
	BOOL   bException = FALSE, bIconic = FALSE, /*bFirst = TRUE,*/ bGuiVisible = FALSE;
	bool   bActive = false, bVisible = false;
	DWORD nElapse = std::max<UINT>(10, gpSet->nMainTimerElapse);
	DWORD nInactiveElapse = std::max<UINT>(10, gpSet->nMainTimerInactiveElapse);
	DWORD nLastFarPID = 0;
	bool bLastAlive = false, bLastAliveActive = false;
	bool lbForceUpdate = false;
	WARNING("Переделать ожидание на hDataReadyEvent, который выставляется в сервере?");
	TODO("Нить не завершается при F10 в фаре - процессы пока не инициализированы...");
	DWORD nConsoleStartTick = GetTickCount();
	DWORD nTimeout = 0;
	CRealBuffer* pLastBuf = nullptr;
	bool lbActiveBufferChanged = false;
	DWORD nConWndCheckTick = GetTickCount();
	wchar_t szLog[140];

	while (TRUE)
	{
		bActive = isActive(false);
		bVisible = bActive || isVisible();
		bIconic = mp_ConEmu->isIconic();

		// в минимизированном/неактивном режиме - сократить расходы
		nTimeout = bIconic ? std::max<UINT>(1000, nInactiveElapse)
			: !(gpSet->isRetardInactivePanes ? bActive : bVisible) ? nInactiveElapse
			: nElapse;

		if (bActive)
			gpSetCls->Performance(tPerfInterval, TRUE); // считается по своему

		#ifdef _DEBUG
		// 1-based console index
		int nVConNo = mp_ConEmu->isVConValid(mp_VCon);
		UNREFERENCED_PARAMETER(nVConNo);
		#endif

		// Проверка, вдруг осталась висеть "мертвая" консоль?
		if (hEvents[IDEVENT_SERVERPH] == nullptr && mh_MainSrv)
		{
			if (!isServerAlive())
			{
				swprintf_c(szLog, L"Server was terminated (WAIT_OBJECT_0) PID=%u", mn_MainSrv_PID);
				LogString(szLog);
				_ASSERTE(bDetached == false);
				nWait = IDEVENT_SERVERPH;
				break;
			}
		}
		else if (hConWnd)
		{
			DWORD nDelta = (GetTickCount() - nConWndCheckTick);
			if (nDelta >= CHECK_CONHWND_TIMEOUT)
			{
				if (!IsWindow(hConWnd))
				{
					swprintf_c(szLog, L"Console window 0x%08X was abnormally terminated?", LODWORD(hConWnd));
					LogString(szLog);
					_ASSERTE(FALSE && "Console window was abnormally terminated?");
					nWait = IDEVENT_SERVERPH;
					break;
				}
				nConWndCheckTick = GetTickCount();
			}
		}




		nWait = WaitForMultipleObjects(nEvents, hEvents, FALSE, nTimeout);
		if (nWait == (DWORD)-1)
		{
			DWORD nWaitItems[EVENTS_COUNT] = {99,99,99,99,99};

			for (size_t i = 0; i < nEvents; i++)
			{
				nWaitItems[i] = WaitForSingleObject(hEvents[i], 0);
				if (nWaitItems[i] == 0)
				{
					nWait = (DWORD)i;
				}
			}
			_ASSERTE(nWait!=(DWORD)-1);

			#ifdef _DEBUG
			int nDbg = nWaitItems[EVENTS_COUNT-1];
			UNREFERENCED_PARAMETER(nDbg);
			#endif
		}

		// Explicit detach was requested
		if (mb_InDetach)
		{
			break;
		}

		// Обновить флаги после ожидания
		if (!bActive)
		{
			bActive = isActive(false);
			bVisible = bActive || isVisible();
		}

		#ifdef _DEBUG
		if (mb_DebugLocked)
		{
			bActive = bVisible = false;
		}
		#endif

		if (nWait == IDEVENT_TERM || nWait == IDEVENT_SERVERPH)
		{
			//if (nWait == IDEVENT_SERVERPH) -- внизу
			//{
			//	DEBUGSTRPROC(L"### ConEmuC.exe was terminated\n");
			//}
			#ifdef _DEBUG
			DWORD nSrvExitPID = 0, nSrvPID = 0, nFErr;
			BOOL bFRc = GetExitCodeProcess(hEvents[IDEVENT_SERVERPH], &nSrvExitPID);
			nFErr = GetLastError();
			typedef DWORD (WINAPI* GetProcessId_t)(HANDLE);
			GetProcessId_t getProcessId = (GetProcessId_t)GetProcAddress(GetModuleHandle(L"kernel32.dll"), "GetProcessId");
			if (getProcessId)
				nSrvPID = getProcessId(hEvents[IDEVENT_SERVERPH]);
			#endif

			swprintf_c(szLog, L"Terminating MonitorThreadWorker by %s", (nWait == IDEVENT_TERM) ? L"IDEVENT_TERM" : L"IDEVENT_SERVERPH");
			LogString(szLog);

			// Чтобы однозначность кода в MonitorThread была
			nWait = IDEVENT_SERVERPH;
			// требование завершения нити
			break;
		}

		if ((nWait == IDEVENT_SWITCHSRV) || mb_SwitchActiveServer)
		{
			//hAltServerHandle = mh_AltSrv;
			//_ASSERTE(hAltServerHandle!=nullptr);
			//hEvents[IDEVENT_SERVERPH] = hAltServerHandle ? hAltServerHandle : mh_MainSrv;

			if (!ReopenServerPipes())
			{
				// Try again?
				SetSwitchActiveServer(true, eSetEvent, eDontChange);
			}
			else
			{
				// Done
				SetSwitchActiveServer(false, eDontChange, eSetEvent);
			}
		}

		bool bNeedUpdateServerActive = (nWait == IDEVENT_UPDATESERVERACTIVE);
		if (!bNeedUpdateServerActive && mn_ServerActiveTick1)
		{
			nAcvWait = WaitForSingleObject(mh_UpdateServerActiveEvent, 0);
			bNeedUpdateServerActive = (nAcvWait == WAIT_OBJECT_0);
		}
		if (bNeedUpdateServerActive)
		{
			if (isServerCreated(true))
			{
				_ASSERTE(hConWnd!=nullptr && "Console window must be already detected!");
				mn_ServerActiveTick2 = GetTickCount();
				DEBUGTEST(DWORD nDelay = mn_ServerActiveTick2 - mn_ServerActiveTick1);

				UpdateServerActive(TRUE);
			}
			ResetEvent(mh_UpdateServerActiveEvent);
			mn_ServerActiveTick1 = 0;
		}

		// Это событие теперь ManualReset
		if (nWait == IDEVENT_MONITORTHREADEVENT
		        || WaitForSingleObject(hEvents[IDEVENT_MONITORTHREADEVENT],0) == WAIT_OBJECT_0)
		{
			ResetEvent(hEvents[IDEVENT_MONITORTHREADEVENT]);

			// Сюда мы попадаем, например, при запуске новой консоли "Под админом", когда GUI НЕ "Под админом"
			// В начале нити (mh_MainSrv == nullptr), если запуск идет через ShellExecuteEx(runas)
			if (hEvents[IDEVENT_SERVERPH] == nullptr)
			{
				if (mh_MainSrv)
				{
					if (bDetached || (m_Args.RunAsAdministrator == crb_On))
					{
						bDetached = false;
						LogString(L"Updating hEvents[IDEVENT_SERVERPH] with mh_MainSrv");
						hEvents[IDEVENT_SERVERPH] = mh_MainSrv;
						nEvents = countof(hEvents);
					}
					else
					{
						_ASSERTE(bDetached==true);
					}
				}
				else
				{
					_ASSERTE(mh_MainSrv!=nullptr);
				}
			}
		}

		if (!rbChildProcessCreated
			&& ((mn_ProcessClientCount > 0) || (mn_Terminal_PID != 0))
			&& ((GetTickCount() - nConsoleStartTick) > PROCESS_WAIT_START_TIME))
		{
			rbChildProcessCreated = true;
			#ifdef _DEBUG
			if (mp_ConEmu->isScClosing())
			{
				_ASSERTE(FALSE && "Drop of mb_ScClosePending may cause assertion in CVConGroup::isVisible");
			}
			#endif
			OnStartedSuccess();
		}

		// IDEVENT_SERVERPH уже проверен, а код возврата обработается при выходе из цикла
		//// Проверим, что ConEmuC жив
		//if (mh_MainSrv)
		//{
		//	DWORD dwExitCode = 0;
		//	#ifdef _DEBUG
		//	BOOL fSuccess =
		//	#endif
		//	    GetExitCodeProcess(mh_MainSrv, &dwExitCode);
		//	if (dwExitCode!=STILL_ACTIVE)
		//	{
		//		StopSignal();
		//		return 0;
		//	}
		//}

		// If postponed macro was not executed (due to multithreading issues)
		if (mpsz_PostCreateMacro && isConsoleReady())
		{
			// Run it now...
			ProcessPostponedMacro();
		}

		// Если консоль не должна быть показана - но ее кто-то отобразил
		if (!isShowConsole && !gpSet->isConVisible)
		{
			/*if (foreWnd == hConWnd)
			    apiSetForegroundWindow(ghWnd);*/
			bool bMonitorVisibility = true;

			#ifdef _DEBUG
			if ((GetKeyState(VK_SCROLL) & 1))
				bMonitorVisibility = false;

			WARNING("bMonitorVisibility = false - для отлова сброса буфера");
			bMonitorVisibility = false;
			#endif

			if (bMonitorVisibility && IsWindowVisible(hConWnd))
				ShowOtherWindow(hConWnd, SW_HIDE);
		}

		// Ensure that window data match hConWnd - some external app may damage it
		if (hConWnd)
		{
			CheckVConRConPointer(false);
		}

		// Размер консоли меняем в том треде, в котором это требуется. Иначе можем заблокироваться при Update (InitDC)
		// Требуется изменение размеров консоли
		/*if (nWait == (IDEVENT_SYNC2WINDOW)) {
		    SetConsoleSize(m_ReqSetSize);
		    //SetEvent(mh_ReqSetSizeEnd);
		    //continue; -- и сразу обновим информацию о ней
		}*/
		DWORD dwT1 = GetTickCount();
		SAFETRY
		{
			if (mb_MonitorAssertTrap)
			{
				mb_MonitorAssertTrap = false;
				//#ifdef _DEBUG
				//MyAssertTrap();
				//#else
				//DebugBreak();
				//#endif
				RaiseTestException();
			}

			//ResetEvent(mh_EndUpdateEvent);

			// Тут и ApplyConsole вызывается
			//if (mp_ConsoleInfo)

			lbActiveBufferChanged = (mp_ABuf != pLastBuf);
			if (lbActiveBufferChanged)
			{
				mb_ABufChaged = lbForceUpdate = true;
			}

			if (mp_RBuf != mp_ABuf)
			{
				mn_LastFarReadTick = GetTickCount();
				if (lbActiveBufferChanged || mp_ABuf->isConsoleDataChanged())
					lbForceUpdate = true;
			}
			// Если сервер жив - можно проверить наличие фара и его отклик
			else if ((!mb_SkipFarPidChange) && m_ConsoleMap.IsValid())
			{
				bool lbFarChanged = false;
				// Alive?
				DWORD nCurFarPID = GetFarPID(TRUE);

				if (!nCurFarPID)
				{
					// Возможно, сменился FAR (возврат из cmd.exe/tcc.exe, или вложенного фара)
					DWORD nPID = GetFarPID(FALSE);

					if (nPID)
					{
						for (UINT i = 0; i < mn_FarPlugPIDsCount; i++)
						{
							if (m_FarPlugPIDs[i] == nPID)
							{
								nCurFarPID = nPID;
								SetFarPluginPID(nCurFarPID);
								break;
							}
						}
					}
				}

				// Если фара (с плагином) нет, а раньше был
				if (!nCurFarPID && nLastFarPID)
				{
					// Закрыть и сбросить PID
					CloseFarMapData();
					nLastFarPID = 0;
					lbFarChanged = true;
				}

				// Если PID фара (с плагином) сменился
				if (nCurFarPID && nLastFarPID != nCurFarPID)
				{
					//mn_LastFarReadIdx = -1;
					mn_LastFarReadTick = 0;
					mn_LastFarAliveTick = 0;
					nLastFarPID = nCurFarPID;

					// Переоткрывать мэппинг при смене PID фара
					// (из одного фара запустили другой, который закрыли и вернулись в первый)
					if (!OpenFarMapData())
					{
						// Значит его таки еще (или уже) нет?
						if (mn_FarPID_PluginDetected == nCurFarPID)
						{
							for (UINT i = 0; i < mn_FarPlugPIDsCount; i++)  // сбросить ИД списка плагинов
							{
								if (m_FarPlugPIDs[i] == nCurFarPID)
									m_FarPlugPIDs[i] = 0;
							}

							SetFarPluginPID(0);
						}
					}

					lbFarChanged = true;
				}

				bool bAlive = CheckConsoleAppAlive(nCurFarPID, bLastAlive);

				if (bActive)
				{
					if (bLastAlive != bAlive || !bLastAliveActive)
					{
						DEBUGSTRALIVE(bAlive ? L"MonitorThread: Alive changed to TRUE\n" : L"MonitorThread: Alive changed to FALSE\n");
						PostMessage(GetView(), WM_SETCURSOR, -1, -1);
					}

					bLastAliveActive = true;

					if (lbFarChanged)
						mp_ConEmu->UpdateProcessDisplay(FALSE); // refresh PID in the Settings window
				}
				else
				{
					bLastAliveActive = false;
				}

				bLastAlive = bAlive;

				WARNING("!!! Если ожидание m_ConDataChanged будет перенесено выше - то тут нужно пользовать полученное выше значение !!!");

				if (!m_ConDataChanged.Wait(0,TRUE))
				{
					// если сменился статус (Far/не Far) - перерисовать на всякий случай,
					// чтобы после возврата из батника, гарантированно отрисовалось в режиме фара
					_ASSERTE(mp_RBuf==mp_ABuf);
					if (mb_InCloseConsole && mh_MainSrv && !isServerAlive())
					{
						// Чтобы однозначность кода в MonitorThread была
						LogString("Terminating MonitorThreadWorker by mb_InCloseConsole && !isServerAlive");
						nWait = IDEVENT_SERVERPH;
						// Основной сервер закрылся (консоль закрыта), идем на выход
						break;
					}

					if (mb_InCloseConsole && !isServerClosing(true) && m_ServerClosing.bBackActivated)
					{
						// Revive console after switching back to MainServer from AltServer
						// (console was not actually closed on request)
						SetInCloseConsole(false);
						m_ServerClosing.bBackActivated = FALSE;
					}

					if (mp_RBuf->ApplyConsoleInfo())
					{
						lbForceUpdate = true;
					}
				}
			}

			bool bCheckStatesFindPanels = false, lbForceUpdateProgress = false;

			// Если консоль неактивна - CVirtualConsole::Update не вызывается и диалоги не детектятся. А это требуется.
			// Смотрим именно mp_ABuf, т.к. здесь нас интересует то, что нужно показать на экране!
			if ((!bActive || bIconic) && (lbActiveBufferChanged || mp_ABuf->isConsoleDataChanged()))
			{
				DWORD nCurTick = GetTickCount();
				DWORD nDelta = nCurTick - mn_LastInactiveRgnCheck;

				if (nDelta > CONSOLEINACTIVERGNTIMEOUT)
				{
					mn_LastInactiveRgnCheck = nCurTick;

					// Если при старте ConEmu создано несколько консолей через '@'
					// то все кроме активной - не инициализированы (InitDC не вызывался),
					// что нужно делать в главной нити, LoadConsoleData об этом позаботится
					if (mp_VCon->LoadConsoleData())
						bCheckStatesFindPanels = true;
				}
			}


			// обновить статусы, найти панели, и т.п.
			if (mb_DataChanged || tabs.mb_TabsWasChanged)
			{
				lbForceUpdate = true; // чтобы если консоль неактивна - не забыть при ее активации передернуть что нужно...
				tabs.mb_TabsWasChanged = false;
				mb_DataChanged = false;
				// Функция загружает ТОЛЬКО ms_PanelTitle, чтобы показать
				// корректный текст в закладке, соответствующей панелям
				CheckPanelTitle();
				// выполнит ниже CheckFarStates & FindPanels
				bCheckStatesFindPanels = true;
			}

			if (!bCheckStatesFindPanels)
			{
				// Если была обнаружена "ошибка" прогресса - проверить заново.
				// Это может также произойти при извлечении файла из архива через MA.
				// Проценты бегут (панелей нет), проценты исчезают, панели появляются, но
				// пока не произойдет хоть каких-нибудь изменений в консоли - статус не обновлялся.
				if (m_Progress.LastWarnCheckTick || mn_FarStatus & (CES_WASPROGRESS|CES_OPER_ERROR))
					bCheckStatesFindPanels = true;
			}

			if (bCheckStatesFindPanels)
			{
				// Статусы mn_FarStatus & mn_PreWarningProgress
				// Результат зависит от распознанных регионов!
				// В функции есть вложенные вызовы, например,
				// mp_ABuf->GetDetector()->GetFlags()
				CheckFarStates();
				// Если возможны панели - найти их в консоли,
				// заодно оттуда позовется CheckProgressInConsole
				mp_RBuf->FindPanels();
			}

			// Far Manager? Refresh its working directories if they are in tabs
			if (mn_FarPID_PluginDetected)
			{
				// Tab templates are case insensitive yet
				LPCWSTR pszTabTempl = gpSet->szTabPanels;
				if ((wcsstr(pszTabTempl, L"%d") || wcsstr(pszTabTempl, L"%D")
					|| wcsstr(pszTabTempl, L"%f") || wcsstr(pszTabTempl, L"%F"))
					&& ReloadFarWorkDir())
				{
					mp_ConEmu->mp_TabBar->Update();
				}
			}

			if (m_Progress.ConsoleProgress >= 0
				&& m_Progress.LastConsoleProgress == -1
				&& m_Progress.LastConProgrTick != 0)
			{
				// Пока бежит запаковка 7z - иногда попадаем в момент, когда на новой строке процентов еще нет
				DWORD nDelta = GetTickCount() - m_Progress.LastConProgrTick;

				if (nDelta >= CONSOLEPROGRESSTIMEOUT)
				{
					logProgress(L"Clearing console progress due timeout", -1);
					setConsoleProgress(-1);
					setLastConsoleProgress(-1, true);
					lbForceUpdateProgress = true;
				}
			}


			// Видимость гуй-клиента - это кнопка "буферного режима"
			if (m_ChildGui.hGuiWnd && bActive)
			{
				BOOL lbVisible = ::IsWindowVisible(m_ChildGui.hGuiWnd);
				if (lbVisible != bGuiVisible)
				{
					// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
					OnBufferHeight();
					bGuiVisible = lbVisible;
				}
			}

			if (m_ChildGui.hGuiWnd
				|| (isConsoleReady() && !isPaused()) // Don't show "Mark ..." when console is paused
				)
			{
				int iLen = GetWindowText(m_ChildGui.isGuiWnd() ? m_ChildGui.hGuiWnd : hConWnd, TitleCmp, countof(TitleCmp)-2);
				if (iLen <= 0)
				{
					// Validation
					TitleCmp[0] = 0;
				}
				else
				{
					// Some validation
					if (iLen >= countof(TitleCmp))
					{
						iLen = countof(TitleCmp)-2;
						TitleCmp[iLen+1] = 0;
					}
					// Trim trailing spaces
					while ((iLen > 0) && (TitleCmp[iLen-1] == L' '))
					{
						TitleCmp[--iLen] = 0;
					}
				}
			}

			#ifdef _DEBUG
			int iDbg;
			if (TitleCmp[0] == 0)
				iDbg = 0;
			#endif

			DEBUGTEST(BOOL bWasForceTitleChanged = mb_ForceTitleChanged);

			if (mb_ForceTitleChanged
				|| lbForceUpdateProgress
				|| (TitleCmp[0] && wcscmp(Title, TitleCmp)))
			{
				mb_ForceTitleChanged = FALSE;
				OnTitleChanged();
				lbForceUpdateProgress = false; // прогресс обновлен
			}
			else if (bActive)
			{
				// Если в консоли заголовок не менялся, но он отличается от заголовка в ConEmu
				mp_ConEmu->CheckNeedUpdateTitle(GetTitle(true));
			}

			if (!bVisible)
			{
				if (lbForceUpdate)
					mp_VCon->UpdateThumbnail();
			}
			else
			{
				//2009-01-21 сомнительно, что здесь действительно нужно подресайзивать дочерние окна
				//if (lbForceUpdate) // размер текущего консольного окна был изменен
				//	mp_ConEmu->OnSize(false); // послать в главную нить запрос на обновление размера
				bool lbNeedRedraw = false;

				if (lbForceUpdate && gpConEmu->isIconic())
					mp_VCon->UpdateThumbnail();

				if ((nWait == (WAIT_OBJECT_0+1)) || lbForceUpdate)
				{
					//2010-05-18 lbForceUpdate вызывал CVirtualConsole::Update(abForce=true), что приводило к тормозам
					bool bForce = false; //lbForceUpdate;
					lbForceUpdate = false;
					//mp_VCon->Validate(); // сбросить флажок

					if (isLogging(3)) LogString("mp_VCon->Update from CRealConsole::MonitorThread");

					if (mp_VCon->Update(bForce))
					{
						// Invalidate уже вызван!
						lbNeedRedraw = false;
					}
				}
				else if (bVisible // где мигать курсором
					&& gpSet->GetAppSettings(GetActiveAppSettingsId())->CursorBlink(bActive)
					&& mb_RConStartedSuccess)
				{
					// Возможно, настало время мигнуть курсором?
					bool lbNeedBlink = false;
					mp_VCon->UpdateCursor(lbNeedBlink);

					// UpdateCursor Invalidate не зовет
					if (lbNeedBlink)
					{
						DEBUGSTRDRAW(L"+++ Force invalidate by lbNeedBlink\n");

						if (isLogging(3)) LogString("Invalidating from CRealConsole::MonitorThread.1");

						lbNeedRedraw = true;
					}
				}
				else if (((GetTickCount() - mn_LastInvalidateTick) > FORCE_INVALIDATE_TIMEOUT))
				{
					DEBUGSTRDRAW(L"+++ Force invalidate by timeout\n");

					if (isLogging(3)) LogString("Invalidating from CRealConsole::MonitorThread.2");

					lbNeedRedraw = true;
				}

				// Если нужна отрисовка - дернем основную нить
				if (lbNeedRedraw)
				{
					//#ifndef _DEBUG
					//WARNING("******************");
					//TODO("После этого двойная отрисовка не вызывается?");
					//mp_VCon->Redraw();
					//#endif

					if (mp_VCon->GetView())
						mp_VCon->Invalidate();
					mn_LastInvalidateTick = GetTickCount();
				}
			}

			pLastBuf = mp_ABuf;

		} SAFECATCHFILTER(WorkerExFilter(GetExceptionCode(), GetExceptionInformation(), _T(__FILE__), __LINE__))
		{
			bException = TRUE;
			// Assertion is shown in WorkerExFilter
		}
		// Чтобы не было слишком быстрой отрисовки (тогда процессор загружается на 100%)
		// делаем такой расчет задержки
		DWORD dwT2 = GetTickCount();
		DWORD dwD = std::max<DWORD>(10, (dwT2 - dwT1));
		nElapse = (DWORD)(nElapse*0.7 + dwD*0.3);

		if (nElapse > 1000) nElapse = 1000;  // больше секунды - не ждать! иначе курсор мигать не будет

		if (bException)
		{
			bException = FALSE;

			//#ifdef _DEBUG
			//_ASSERTE(FALSE);
			//#endif

			//AssertMsg(L"Exception triggered in CRealConsole::MonitorThread");
		}

		//if (bActive)
		//	gpSetCls->Performance(tPerfInterval, FALSE);

		if (m_ServerClosing.nServerPID
		        && m_ServerClosing.nServerPID == mn_MainSrv_PID
		        && (GetTickCount() - m_ServerClosing.nRecieveTick) >= SERVERCLOSETIMEOUT)
		{
			// Видимо, сервер повис во время выхода?
			LogString("Terminating MonitorThreadWorker by m_ServerClosing.nServerPID");
			isConsoleClosing(); // функция позовет TerminateProcess(mh_MainSrv)
			nWait = IDEVENT_SERVERPH;
			break;
		}

		#ifdef _DEBUG
		UNREFERENCED_PARAMETER(nVConNo);
		#endif
	}

	return nWait;
}

bool CRealConsole::PreInit()
{
	TODO("Инициализация остальных буферов?");

	_ASSERTE(mp_RBuf && mp_RBuf==mp_ABuf);
	MCHKHEAP;

	return mp_RBuf->PreInit();
}

void CRealConsole::SetMonitorThreadEvent()
{
	AssertThis();

	SetEvent(mh_MonitorThreadEvent);
}

// static
bool CRealConsole::RefreshAfterRestore(CVirtualConsole* pVCon, LPARAM lParam)
{
	CVConGuard VCon(pVCon);
	CRealConsole* pRCon = VCon->RCon();
	if (pRCon)
	{
		pRCon->SetMonitorThreadEvent();
		BOOL bRedraw = FALSE;
		HWND hChildGui = pRCon->GuiWnd();
		DWORD dwServerPID = pRCon->GetServerPID(true);
		if (hChildGui && dwServerPID)
		{
			// We need to invalidate client and non-client areas, following lines does the trick
			CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_REDRAWHWND, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
			if (pIn)
			{
				pIn->dwData[0] = (DWORD)(DWORD_PTR)hChildGui;
				CESERVER_REQ *pOut = ExecuteSrvCmd(dwServerPID, pIn, ghWnd);
				if (pOut && (pOut->DataSize() >= sizeof(DWORD)))
				{
					bRedraw = pOut->dwData[0];
				}
				ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
			_ASSERTE(bRedraw);
		}
		UNREFERENCED_PARAMETER(bRedraw);
	}
	return true;
}

bool CRealConsole::StartMonitorThread()
{
	bool lbRc = false;
	_ASSERTE(mh_MonitorThread==nullptr);
	DWORD nCreateBegin = GetTickCount();
	SetConStatus(L"Initializing ConEmu (4)", cso_ResetOnConsoleReady|cso_Critical);
	mh_MonitorThread = apiCreateThread(MonitorThread, (LPVOID)this, &mn_MonitorThreadID, "RCon::MonitorThread ID=%i", mp_VCon->ID());
	SetConStatus(L"Initializing ConEmu (5)", cso_ResetOnConsoleReady|cso_Critical);
	DWORD nCreateEnd = GetTickCount();
	DWORD nThreadCreationTime = nCreateEnd - nCreateBegin;
	if (nThreadCreationTime > 2500)
	{
		wchar_t szInfo[80];
		swprintf_c(szInfo,
			L"[DBG] Very high CPU load? CreateThread takes %u ms", nThreadCreationTime);
		#ifdef _DEBUG
		Icon.ShowTrayIcon(szInfo);
		#endif
		LogString(szInfo);
	}

	if (mh_MonitorThread == nullptr)
	{
		DisplayLastError(_T("Can't create console thread!"));
	}
	else
	{
		//lbRc = SetThreadPriority(mh_MonitorThread, THREAD_PRIORITY_ABOVE_NORMAL);
		lbRc = true;
	}

	return lbRc;
}

HKEY CRealConsole::PrepareConsoleRegistryKey(LPCWSTR asSubKey) const
{
	HKEY hkConsole = nullptr;
	LONG lRegRc;


	const CEStr lsKey(JoinPath(L"Console", asSubKey)); // Default is "Console\\ConEmu"
	if (0 == (lRegRc = RegCreateKeyEx(HKEY_CURRENT_USER, lsKey, 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkConsole, nullptr)))
	{
		DWORD nSize = sizeof(DWORD), nNewValue;
		struct {
			LPCWSTR pszName;
			DWORD nType;
			union {
				DWORD_PTR nDef;
				LPCWSTR pszDef;
			};
			DWORD nMin, nMax;
		} consoleOptions[] = {
			// Issue 700: Default history buffers count too small.
			{ L"HistoryBufferSize", REG_DWORD, {static_cast<DWORD_PTR>(50)}, 16, 999 },
			{ L"NumberOfHistoryBuffers", REG_DWORD, {static_cast<DWORD_PTR>(32)}, 16, 999 },
			// Restrict windows from creating console using undesired font
			{ L"FaceName", REG_SZ, {reinterpret_cast<DWORD_PTR>(gpSet->ConsoleFont.lfFaceName)}, 0, 0 },
			{ L"FontSize", REG_DWORD, {static_cast<DWORD_PTR>((LOWORD(gpSet->ConsoleFont.lfHeight) << 16))}, 0, 0 },
			// Avoid extra conhost resizes on startup
			{ L"ScreenBufferSize", REG_DWORD, {static_cast<DWORD_PTR>((LOWORD(mp_RBuf->GetBufferHeight()) << 16) | LOWORD(mp_RBuf->GetBufferWidth()))}, 0, 0 },
			{ L"WindowSize", REG_DWORD, {static_cast<DWORD_PTR>((LOWORD(mp_RBuf->GetTextHeight()) << 16) | LOWORD(mp_RBuf->GetTextWidth()))}, 0, 0 },
		};
		for (auto& option : consoleOptions)
		{
			const bool bHasMinMax = (option.nType == REG_DWORD) && (option.nMin || option.nMax);

			switch (option.nType)
			{
			case REG_DWORD:
			{
				DWORD nValue = 0, nType = 0;
				lRegRc = RegQueryValueEx(hkConsole, option.pszName, nullptr, &nType, reinterpret_cast<LPBYTE>(&nValue), &nSize);
				if ((lRegRc != 0) || (nType != REG_DWORD) || (nSize != sizeof(DWORD))
					|| (bHasMinMax && ((nValue < option.nMin) || (nValue > option.nMax)))
					|| (!bHasMinMax && (nValue != option.nDef))
					)
				{
					nNewValue = LODWORD(option.nDef);
					if (option.nMin || option.nMax)
					{
						if (!lRegRc && (nValue < option.nMin))
							nNewValue = option.nMin;
						else if (!lRegRc && (nValue > option.nMax))
							nNewValue = option.nMax;
					}

					lRegRc = RegSetValueEx(hkConsole, option.pszName, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&nNewValue), sizeof(nNewValue));
				}
				break;
			} // case REG_DWORD:

			case REG_SZ:
			{
				if (option.pszDef)
				{
					nNewValue = (wcslen(option.pszDef) + 1) * sizeof(option.pszDef[0]);
					lRegRc = RegSetValueEx(hkConsole, option.pszName, 0, REG_SZ, reinterpret_cast<const BYTE*>(option.pszDef), nNewValue);
				}
				break;
			} // case REG_SZ

			} // switch (BufferValues[i].nType)

			std::ignore = lRegRc;
		}
	}
	else
	{
		const CEStr lsMsg(L"Failed to create/open registry key", L"\n[HKCU\\", lsKey, L"]");
		DisplayLastError(lsMsg, lRegRc);
		hkConsole = nullptr;
	}

	return hkConsole;
}

void CRealConsole::PrepareDefaultColors()
{
	BYTE nTextColorIdx /*= 7*/, nBackColorIdx /*= 0*/, nPopTextColorIdx /*= 5*/, nPopBackColorIdx /*= 15*/;
	PrepareDefaultColors(nTextColorIdx, nBackColorIdx, nPopTextColorIdx, nPopBackColorIdx);
}

void CRealConsole::PrepareDefaultColors(BYTE& nTextColorIdx, BYTE& nBackColorIdx, BYTE& nPopTextColorIdx, BYTE& nPopBackColorIdx, bool bUpdateRegistry /*= false*/, HKEY hkConsole /*= nullptr*/)
{
	//nTextColorIdx = 7; nBackColorIdx = 0; nPopTextColorIdx = 5; nPopBackColorIdx = 15;

	// Тут берем именно "GetDefaultAppSettingsId", а не "GetActiveAppSettingsId"
	// т.к. довольно стремно менять АТРИБУТЫ консоли при выполнении пакетников и пр.
	const AppSettings* pApp = gpSet->GetAppSettings(GetDefaultAppSettingsId());
	_ASSERTE(pApp!=nullptr);

	// May be palette was inherited from RealConsole (Win+G attach)
	const ColorPalette* pPal = mp_VCon->m_SelfPalette.bPredefined ? &mp_VCon->m_SelfPalette : nullptr;
	// User's chosen special palette for this console?
	if (!pPal)
	{
		_ASSERTE(countof(pApp->szPaletteName)>0); // must be array, not pointer
		LPCWSTR pszPalette = (m_Args.pszPalette && *m_Args.pszPalette) ? m_Args.pszPalette
			: (pApp->OverridePalette && *pApp->szPaletteName) ? pApp->szPaletteName
			: nullptr;
		if (pszPalette && *pszPalette)
		{
			int iPalIdx = gpSet->PaletteGetIndex(pszPalette);
			if (iPalIdx >= 0)
			{
				pPal = gpSet->PaletteGet(iPalIdx);
			}
		}
	}

	nTextColorIdx = pPal ? pPal->nTextColorIdx : pApp->TextColorIdx(); // 0..15,16
	nBackColorIdx = pPal ? pPal->nBackColorIdx : pApp->BackColorIdx(); // 0..15,16
	if (nTextColorIdx <= 15 || nBackColorIdx <= 15)
	{
		if (nTextColorIdx >= 16) nTextColorIdx = 7;
		if (nBackColorIdx >= 16) nBackColorIdx = 0;
		if ((nTextColorIdx == nBackColorIdx)
			&& (!gpSetCls->IsBackgroundEnabled(mp_VCon)
				|| !(gpSet->nBgImageColors & (1 << nBackColorIdx))))  // bg color is an image
		{
			nTextColorIdx = nBackColorIdx ? 0 : 7;
		}
		//si.dwFlags |= STARTF_USEFILLATTRIBUTE;
		//si.dwFillAttribute = (nBackColorIdx << 4) | nTextColorIdx;
		mn_TextColorIdx = nTextColorIdx;
		mn_BackColorIdx = nBackColorIdx;
	}
	else
	{
		nTextColorIdx = nBackColorIdx = 16;
		//si.dwFlags &= ~STARTF_USEFILLATTRIBUTE;
		mn_TextColorIdx = 7;
		mn_BackColorIdx = 0;
	}

	nPopTextColorIdx = pPal ? pPal->nPopTextColorIdx : pApp->PopTextColorIdx(); // 0..15,16
	nPopBackColorIdx = pPal ? pPal->nPopBackColorIdx : pApp->PopBackColorIdx(); // 0..15,16
	if (nPopTextColorIdx <= 15 || nPopBackColorIdx <= 15)
	{
		if (nPopTextColorIdx >= 16) nPopTextColorIdx = 5;
		if (nPopBackColorIdx >= 16) nPopBackColorIdx = 15;
		if (nPopTextColorIdx == nPopBackColorIdx)
			nPopBackColorIdx = nPopTextColorIdx ? 0 : 15;
		mn_PopTextColorIdx = nPopTextColorIdx;
		mn_PopBackColorIdx = nPopBackColorIdx;
	}
	else
	{
		nPopTextColorIdx = nPopBackColorIdx = 16;
		mn_PopTextColorIdx = 5;
		mn_PopBackColorIdx = 15;
	}


	if (bUpdateRegistry)
	{
		bool bNeedClose = false;
		if (hkConsole == nullptr)
		{
			LONG lRegRc;
			if (0 != (lRegRc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Console\\ConEmu", 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hkConsole, nullptr)))
			{
				DisplayLastError(L"Failed to create/open registry key 'HKCU\\Console\\ConEmu'", lRegRc);
				hkConsole = nullptr;
			}
			else
			{
				bNeedClose = true;
			}
		}

		if (nTextColorIdx > 15)
			nTextColorIdx = GetDefaultTextColorIdx();
		if (nBackColorIdx > 15)
			nBackColorIdx = GetDefaultBackColorIdx();
		DWORD nColors = MAKECONCOLOR(nTextColorIdx, nBackColorIdx);
		if (hkConsole)
		{
			RegSetValueEx(hkConsole, L"ScreenColors", 0, REG_DWORD, (LPBYTE)&nColors, sizeof(nColors));
		}

		if (nPopTextColorIdx <= 15 || nPopBackColorIdx <= 15)
		{
			if (hkConsole)
			{
				DWORD nColors = MAKECONCOLOR(mn_PopTextColorIdx, mn_PopBackColorIdx);
				RegSetValueEx(hkConsole, L"PopupColors", 0, REG_DWORD, (LPBYTE)&nColors, sizeof(nColors));
			}
		}

		if (bNeedClose)
		{
			RegCloseKey(hkConsole);
		}
	}
}

void CRealConsole::OnStartProcessAllowed()
{
	AssertThis();

	if (!mb_NeedStartProcess)
	{
		_ASSERTE(mb_NeedStartProcess);
		mb_StartResult = TRUE;
		SetEvent(mh_StartExecuted);
		return;
	}

	_ASSERTE(mh_MainSrv == nullptr);

	if (!PreInit())
	{
		DEBUGSTRPROC(L"### RCon:PreInit failed\n");
		SetConStatus(L"RCon:PreInit failed", cso_Critical);

		mb_StartResult = FALSE;
		mb_NeedStartProcess = FALSE;
		SetEvent(mh_StartExecuted);

		return;
	}

	BOOL bStartRc = StartProcess();

	if (!bStartRc)
	{
		wchar_t szErrInfo[128];
		swprintf_c(szErrInfo, L"Can't start root process (code %i)", (int)GetLastError());
		DEBUGSTRPROC(L"### Can't start process\n");

		SetConStatus(szErrInfo, cso_Critical);

		WARNING("Need to be checked, what happens on 'Run errors'");
		return;
	}
}

void CRealConsole::ConHostSearchPrepare()
{
	AssertThis();
	CRefGuard<CConHostSearch> search(m_ConHostSearch.Ptr());
	if (!search)
	{
		_ASSERTE(search);
		return;
	}

	LogString(L"Prepare searching for conhost");

	search->data.Reset();

	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (h && (h != INVALID_HANDLE_VALUE))
	{
		PROCESSENTRY32 PI = {sizeof(PI)};
		if (Process32First(h, &PI))
		{
			BOOL bFlag = TRUE;
			do {
				if (lstrcmpi(PI.szExeFile, L"conhost.exe") == 0)
					search->data.Set(PI.th32ProcessID, bFlag);
			} while (Process32Next(h, &PI));
		}
		CloseHandle(h);
	}

	LogString(L"Prepare searching for conhost - Done");
}

DWORD CRealConsole::ConHostSearch(bool bFinal)
{
	AssertThisRet(0);
	CRefGuard<CConHostSearch> search(m_ConHostSearch.Ptr());
	if (!search)
	{
		_ASSERTE(search);
		return 0;
	}

	if (!mn_ConHost_PID)
	{
		_ASSERTE(mn_ConHost_PID==0);
		mn_ConHost_PID = 0;
	}

	LogString(L"Searching for conhost");

	for (int s = 0; s <= 1; s++)
	{
		// Ищем новые процессы "conhost.exe"
		MArray<PROCESSENTRY32W> CreatedHost;
		MToolHelpProcess processes;
		if (processes.Open())
		{
			PROCESSENTRY32W pi{};
			while (processes.Next(pi))
			{
				if (lstrcmpi(pi.szExeFile, L"conhost.exe") == 0)
				{
					if (!search->data.Get(pi.th32ProcessID, nullptr))
					{
						CreatedHost.push_back(pi);
					}
				}
			}
			processes.Close();
		}

		if (CreatedHost.size() <= 0)
		{
			_ASSERTE(!bFinal && "Created conhost.exe was not found!");
			goto wrap;
		}
		else if (CreatedHost.size() > 1)
		{
			//_ASSERTE(FALSE && "More than one created conhost.exe was found!");
			LogString(L"More than one created conhost.exe was found!");
			Sleep(250); // Попробовать еще раз? Может кто-то левый проехал...
			// We may try to determine ‘proper’ conhost via duplicating console process handle,
			// example is here: https://github.com/Maximus5/getconkbl (Elfy's fork)
			// However, the very conhost.exe PID is not required for ConEmu.
			continue;
		}
		else
		{
			// Установить mn_ConHost_PID
			ConHostSetPID(CreatedHost[0].th32ProcessID);
			break;
		}
	}

wrap:

	if (bFinal)
	{
		m_ConHostSearch.Release();
	}

	if (gpSet->isLogging())
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"ConHostPID=%u", mn_ConHost_PID);
		LogString(szInfo);
	}

	if (mn_ConHost_PID)
		mp_ConEmu->ReleaseConhostDelay();
	return mn_ConHost_PID;
}

void CRealConsole::ConHostSetPID(DWORD nConHostPID)
{
	mn_ConHost_PID = nConHostPID;

	// Вывести информацию в отладчик.
	// Надо! Даже в релизе. (хотя, при желании, можно только при mb_BlockChildrenDebuggers)
	if (nConHostPID)
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, CONEMU_CONHOST_CREATED_MSG L"%u\n", nConHostPID);
		DEBUGSTRCONHOST(szInfo);
	}
}

LPCWSTR CRealConsole::GetStartupDir()
{
	LPCWSTR pszStartupDir = m_Args.pszStartupDir;

	if (m_Args.pszUserName != nullptr)
	{
		// When starting under another credentials - try to use %USERPROFILE% instead of "system32"
		if (!pszStartupDir || !*pszStartupDir)
		{
			// Issue 1557: switch -new_console:u:"other_user:password" lock the account of other_user
			// So, just return nullptr to force SetCurrentDirectory("%USERPROFILE%") in the server
			return nullptr;

			#if 0
			HANDLE hLogonToken = m_Args.CheckUserToken();
			if (hLogonToken)
			{
				HRESULT hr = E_FAIL;

				// Windows 2000 - hLogonToken - not supported
				if (gOSVer.dwMajorVersion <= 5 && gOSVer.dwMinorVersion == 0)
				{
					if (ImpersonateLoggedOnUser(hLogonToken))
					{
						memset(ms_ProfilePathTemp, 0, sizeof(ms_ProfilePathTemp));
						hr = SHGetFolderPath(nullptr, CSIDL_PROFILE, nullptr, SHGFP_TYPE_CURRENT, ms_ProfilePathTemp);
						RevertToSelf();
					}
				}
				else
				{
					memset(ms_ProfilePathTemp, 0, sizeof(ms_ProfilePathTemp));
					hr = SHGetFolderPath(nullptr, CSIDL_PROFILE, hLogonToken, SHGFP_TYPE_CURRENT, ms_ProfilePathTemp);
				}

				if (SUCCEEDED(hr) && *ms_ProfilePathTemp)
				{
					pszStartupDir = ms_ProfilePathTemp;
				}

				CloseHandle(hLogonToken);
			}
			#endif
		}
	}

	LPCWSTR lpszWorkDir = mp_ConEmu->WorkDir(pszStartupDir);

	return lpszWorkDir;
}

bool CRealConsole::StartDebugger(StartDebugType sdt)
{
	DWORD dwPID = GetActivePID();
	if (!dwPID)
		dwPID = GetLoadedPID();
	if (!dwPID)
		return false;

	//// Create process, with flag /Attach GetCurrentProcessId()
	//// Sleep for sometimes, try InitHWND(hConWnd); several times

	WCHAR  szExe[MAX_PATH*3] = L"";
	bool lbRc = false;
	PROCESS_INFORMATION pi = {};
	STARTUPINFO si = {};
	si.cb = sizeof(si);

	int nBits = GetProcessBits(dwPID);
	LPCWSTR pszServer = (nBits == 64) ? mp_ConEmu->ms_ConEmuC64Full : mp_ConEmu->ms_ConEmuC32Full;

	switch (sdt)
	{
	case sdt_DumpMemory:
	case sdt_DumpMemoryTree:
		{
			si.dwFlags |= STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_SHOWNORMAL;

			if (sdt == sdt_DumpMemory)
			{
				swprintf_c(szExe, L"\"%s\" /DEBUGPID=%i /DUMP", pszServer, dwPID);
			}
			else
			{
				// В режиме "Дамп дерева процессов" нас интересует и дамп текущего процесса ConEmu.exe
				CEStr lsPID(ultow_s(GetCurrentProcessId(), szExe, 10));
				ConProcess* pPrc = nullptr;
				int nCount = GetProcesses(&pPrc, false/*ClientOnly*/);
				if (!pPrc || (nCount < 1))
				{
					MsgBox(L"GetProcesses fails", MB_OKCANCEL|MB_SYSTEMMODAL, L"StartDebugLogConsole");
					return false;
				}
				for (int i = 0; i < nCount; i++)
				{
					lsPID = CEStr(lsPID.ms_Val ? L"," : nullptr, ultow_s(pPrc[i].ProcessID, szExe, 10));
					if (lsPID.GetLen() > MAX_PATH)
						break;
				}
				swprintf_c(szExe, L"\"%s\" /DEBUGPID=%s /DUMP", pszServer, lsPID.ms_Val);
				free(pPrc);
			}
		} break;
	case sdt_DebugActiveProcess:
		{
			TODO("Может лучше переделать на CreateCon?");
			si.dwFlags |= STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;
			//No need to ‘Attach’ it's better to show ‘New console’ dialog and allow user to change options (splitting for example)
			//DWORD dwSelfPID = GetCurrentProcessId();
			//int W = TextWidth();
			//int H = TextHeight();
			//swprintf_c(szExe, L"\"%s\" /ATTACH /GID=%i /GHWND=%08X /BW=%i /BH=%i /BZ=%u /ROOT \"%s\" /DEBUGPID=%i ",
			//	pszServer, dwSelfPID, LODWORD(ghWnd), W, H, LONGOUTPUTHEIGHT_MAX, pszServer, dwPID);
			swprintf_c(szExe, L"\"%s\" /DEBUGPID=%i ", pszServer, dwPID);
		} break;
	default:
		_ASSERTE(FALSE && "Unsupported debugger mode");
		return false;
	}

	bool bShowStartDlg = true; // isPressed(VK_SHIFT)

	#if 0 //defined(_DEBUG)
	// For mini-dump - immediately, to not to introduce lags
	// If Shift key is pressed - check it below
	if ((sdt != sdt_DumpMemory) && (sdt != sdt_DumpMemoryTree) && !bShowStartDlg)
	{
		if (MsgBox(szExe, MB_OKCANCEL|MB_SYSTEMMODAL, L"StartDebugLogConsole") != IDOK)
			return false;
	}
	#endif

	// CreateOrRunAs needs to know how "original" process was started...
	RConStartArgsEx Args;
	Args.AssignFrom(m_Args);
	SafeFree(Args.pszSpecialCmd);

	// If process was started under different credentials, most probably
	// we need to ask user for password (we don't store passwords in memory for security reason)
	if ((((m_Args.RunAsAdministrator != crb_On) || mp_ConEmu->mb_IsUacAdmin)
			&& (m_Args.pszUserName != nullptr) && (m_Args.UseEmptyPassword != crb_On)
		)
		// If it's not a dump request, and user is pressing Shift key, open NewConsole dialog
		|| (((sdt != sdt_DumpMemory) && (sdt != sdt_DumpMemoryTree)) && bShowStartDlg)
		)
	{
		Args.pszSpecialCmd = lstrdup(szExe).Detach();
		Args.nSplitValue = 700;

		int nRc = mp_ConEmu->RecreateDlg(&Args, true);

		if (nRc != IDC_START)
			return false;

		if (gpSetCls->IsMulti() && (Args.aRecreate != cra_CreateWindow))
			lbRc = (gpConEmu->CreateCon(Args) != nullptr);
		else
			lbRc = gpConEmu->CreateWnd(Args);
	}
	else
	{
		LPCWSTR pszWordDir = nullptr;
		DWORD dwLastErr = 0;

		if (!CreateOrRunAs(this, Args, szExe, pszWordDir, si, pi, mp_sei_dbg, dwLastErr))
		{
			// Хорошо бы ошибку показать?
			DWORD dwErr = GetLastError();
			wchar_t szErr[128]; swprintf_c(szErr, L"Can't create debugger console! ErrCode=0x%08X", dwErr);
			MBoxA(szErr);
		}
		else
		{
			SafeCloseHandle(pi.hProcess);
			SafeCloseHandle(pi.hThread);
			lbRc = true;
		}
	}

	return lbRc;
}

void CRealConsole::SetSwitchActiveServer(bool bSwitch, CRealConsole::SwitchActiveServerEvt eCall, SwitchActiveServerEvt eResult)
{
	if (isLogging())
		LogString(bSwitch ? L"AltServer switch was requested" : L"AltServer switch was set to false");

	if (eResult == eResetEvent)
		ResetEvent(mh_ActiveServerSwitched);

	if (eCall == eResetEvent)
		ResetEvent(mh_SwitchActiveServer);

	#ifdef _DEBUG
	if (bSwitch)
		mb_SwitchActiveServer = true;
	else
	#endif
	mb_SwitchActiveServer = bSwitch;

	if (eResult == eSetEvent)
		SetEvent(mh_ActiveServerSwitched);

	if (eCall == eSetEvent)
	{
		_ASSERTE(bSwitch);
		SetEvent(mh_SwitchActiveServer);
	}
}

void CRealConsole::ResetVarsOnStart()
{
	mp_VCon->ResetOnStart();

	SetInCloseConsole(false);
	mb_RecreateFailed = FALSE;
	SetSwitchActiveServer(false, eResetEvent, eResetEvent);
	//Drop flag after Restart console
	mb_InPostCloseMacro = false;
	//mb_WasStartDetached = FALSE; -- не сбрасывать, на него смотрит и isDetached()
	m_RootInfo = {};
	//m_RootInfo.nExitCode = STILL_ACTIVE;
	m_ServerClosing = {};
	mn_StartTick = mn_RunTime = 0;
	mn_DeactivateTick = 0;
	mb_WasVisibleOnce = mp_VCon->isVisible();
	mb_NeedLoadRootProcessIcon = true;
	m_ScrollStatus = {};
	SafeFree(ms_MountRoot);

	UpdateStartState(rss_StartupRequested);

	hConWnd = nullptr;

	mn_FarNoPanelsCheck = 0;

	// Don't show in VCon invalid data
	if (mp_RBuf)
		mp_RBuf->ResetConData();
	// At the moment of start, Primary buffer is expected to be active
	_ASSERTE(mp_ABuf == mp_RBuf);

	if (mp_XTerm)
		mp_XTerm->Reset();

	// setXXX for the sake of convenience
	setGuiWndPID(nullptr, 0, 0, nullptr); // set m_ChildGui.Process.ProcessID to 0
	m_ChildGui = {};

	StartStopXTerm(0, false/*te_win32*/);

	mb_WasForceTerminated = FALSE;

	// Обновить закладки
	tabs.m_Tabs.MarkTabsInvalid(CTabStack::MatchNonPanel, 0);
	SetTabs(nullptr, 1, 0);
	mp_ConEmu->mp_TabBar->PrintRecentStack();
}

void CRealConsole::SetRootProcessName(LPCWSTR asProcessName)
{
	if (!asProcessName) asProcessName = L"";

	if (lstrcmp(asProcessName, ms_RootProcessName) != 0)
	{
		if (isLogging())
		{
			CEStr lsLog(L"Root process name changed. `", ms_RootProcessName, L"` -> `", asProcessName, L"`");
			LogString(lsLog);
		}
		mn_RootProcessIcon = -1;
		lstrcpyn(ms_RootProcessName, asProcessName, countof(ms_RootProcessName));
		mb_NeedLoadRootProcessIcon = true;
	}
}

void CRealConsole::UpdateRootInfo(const CESERVER_ROOT_INFO& RootInfo)
{
	if (gpSet->isLogging())
	{
		wchar_t szInfo[120];
		swprintf_c(szInfo, L"UpdateRootInfo(Running=%s PID=%u ExitCode=%u UpTime=%u) PTY=x%04X",
			RootInfo.bRunning ? L"Yes" : L"No", RootInfo.nPID, RootInfo.nExitCode, RootInfo.nUpTime,
			m_Args.nPTY);
		LogString(szInfo);
	}

	// Root process was just successfully started?
	if (!m_RootInfo.bRunning && RootInfo.bRunning && RootInfo.nPID)
	{
		if (m_Args.nPTY)
		{
			// xterm keys notation
			StartStopXTerm(RootInfo.nPID, (m_Args.nPTY & pty_XTerm));
			// Bracketed paste
			StartStopBracketedPaste(RootInfo.nPID, (m_Args.nPTY & pty_BrPaste));
			// Application cursor keys
			StartStopAppCursorKeys(RootInfo.nPID, (m_Args.nPTY & pty_AppKeys));
		}
	}

	m_RootInfo = RootInfo;

	if (isActive(false))
	{
		mp_ConEmu->mp_Status->UpdateStatusBar(true);
	}
}

CRealConsole::CConHostSearch::CConHostSearch()
{
	data.Init();
}

void CRealConsole::CConHostSearch::FinalRelease()
{
	data.Release();
}

bool CRealConsole::StartProcess()
{
	AssertThisRet(false);

	// Must be executed in Runner Thread
	_ASSERTE(mp_ConEmu->mp_RunQueue->isRunnerThread());
	// Monitor thread must be started already
	_ASSERTE(mn_MonitorThreadID!=0);

	bool lbRc = false;
	SetConStatus(L"Preparing process startup line...", cso_ResetOnConsoleReady|cso_Critical);

	SetConEmuEnvVarChild(mp_VCon->GetView(), mp_VCon->GetBack());
	SetConEmuEnvVar(ghWnd);

	// Для валидации объектов
	CVirtualConsole* pVCon = mp_VCon;

	//if (!mb_ProcessRestarted)
	//{
	//	if (!PreInit())
	//		goto wrap;
	//}

	HWND hSetForeground = (mp_ConEmu->isIconic() || !IsWindowVisible(ghWnd)) ? GetForegroundWindow() : ghWnd;

	mb_UseOnlyPipeInput = false;

	if (mp_sei)
	{
		SafeCloseHandle(mp_sei->hProcess);
		SafeFree(mp_sei);
	}

	//ResetEvent(mh_CreateRootEvent);
	CloseConfirmReset();

	//Moved to function
	ResetVarsOnStart();

	//Ready!
	mb_InCreateRoot = true;

	//=====================================
	STARTUPINFO si{};
	PROCESS_INFORMATION pi{};
	CEStr szInitConTitle(CEC_INITTITLE);
	MCHKHEAP;

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USECOUNTCHARS | STARTF_USESIZE/*|STARTF_USEPOSITION*/;
	si.lpTitle = szInitConTitle.ms_Val;

	// Buffer size in cells
	si.dwXCountChars = mp_RBuf->GetBufferWidth() /*con.m_sbi.dwSize.X*/;
	si.dwYCountChars = mp_RBuf->GetBufferHeight() /*con.m_sbi.dwSize.Y*/;

	// Window size should be in *pixels*, we could only estimate it, based on the font size 4x6
	// #TODO: Use configured gpSet->ConsoleFont.lfHeight instead of default 4x6, need to evaluate width though
	if (mp_RBuf->isScroll() /*con.bBufferHeight*/)
	{
		si.dwXSize = 4 * mp_RBuf->GetTextWidth()/*con.m_sbi.dwSize.X*/ + 2*GetSystemMetrics(SM_CXFRAME) + GetSystemMetrics(SM_CXVSCROLL);
		si.dwYSize = 6 * mp_RBuf->GetTextHeight()/*con.nTextHeight*/ + 2*GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION);
	}
	else
	{
		si.dwXSize = 4 * mp_RBuf->GetTextWidth()/*con.m_sbi.dwSize.X*/ + 2*GetSystemMetrics(SM_CXFRAME);
		si.dwYSize = 6 * mp_RBuf->GetTextHeight()/*con.m_sbi.dwSize.X*/ + 2*GetSystemMetrics(SM_CYFRAME) + GetSystemMetrics(SM_CYCAPTION);
	}

	si.wShowWindow = gpSet->isConVisible ? SW_SHOWNORMAL : SW_HIDE;
	isShowConsole = gpSet->isConVisible;

	ZeroMemory(&pi, sizeof(pi));
	MCHKHEAP;
	_ASSERTE((m_Args.pszStartupDir == nullptr) || (*m_Args.pszStartupDir != 0));

	// Altered console title was set?
	if (!ms_DefTitle.IsEmpty())
		si.lpTitle = ms_DefTitle.ms_Val;

	// Prepare cmd line
	LPCWSTR lpszRawCmd = (m_Args.pszSpecialCmd && *m_Args.pszSpecialCmd) ? m_Args.pszSpecialCmd : gpConEmu->GetCmd(nullptr, true);
	_ASSERTE(lpszRawCmd && *lpszRawCmd);
	LPCWSTR lpszCmd = lpszRawCmd;
	LPCWSTR lpszWorkDir = nullptr;
	DWORD dwLastError = 0;
	DWORD nCreateBegin, nCreateEnd, nCreateDuration = 0;

	// We need to know full path to ConEmuC
	CEStr lsConsoleKey;
	// If we call ShellExecute("runas", ...) we need to use special console key
	if (m_Args.RunAsAdministrator != crb_On)
	{
		// For CreateProcess - we can specify console window title
		// which is used as registry key to initialize conhost
		// si.lpTitle must be already set either to "ConEmu", or ms_DefTitle.ms_Val
		_ASSERTE(si.lpTitle && *si.lpTitle);
		lsConsoleKey.Set(si.lpTitle);
	}
	else
	{
		// We need, for example
		// [HKEY_CURRENT_USER\Console\C:_Tools_ConEmu_ConEmuC64.exe]
		lsConsoleKey.Set(mp_ConEmu->ConEmuCExeFull(lpszCmd));
		wchar_t* pch = wcschr(lsConsoleKey.ms_Val, L'\\');
		while (pch)
		{
			*(pch++) = L'_';
			pch = wcschr(pch, L'\\');
		}
	}

	// HistoryBufferSize, NumberOfHistoryBuffers, FaceName, FontSize, ...
	HKEY hkConsole = PrepareConsoleRegistryKey(lsConsoleKey);

	BYTE nTextColorIdx /*= 7*/, nBackColorIdx /*= 0*/, nPopTextColorIdx /*= 5*/, nPopBackColorIdx /*= 15*/;
	PrepareDefaultColors(nTextColorIdx, nBackColorIdx, nPopTextColorIdx, nPopBackColorIdx, true, hkConsole);
	si.dwFlags |= STARTF_USEFILLATTRIBUTE;
	si.dwFillAttribute = MAKECONCOLOR(nTextColorIdx, nBackColorIdx);

	if (hkConsole)
	{
		RegCloseKey(hkConsole);
		hkConsole = nullptr;
	}

	// ConHost.exe was introduced in Windows 7.
	// But in that version the process was created "from parent csrss".
	// Windows 8 - is OK, conhost.exe is a child of our Root console process.
	bool bNeedConHostSearch = (gnOsVer == 0x0601);
	bool bConHostLocked = false;
	if (bNeedConHostSearch)
	{
		if (!m_ConHostSearch)
		{
			m_ConHostSearch.Attach(new CConHostSearch());
			bNeedConHostSearch = m_ConHostSearch.IsValid();
		}
		else
		{
			m_ConHostSearch->data.Reset();
		}
	}
	//
	if (!bNeedConHostSearch)
	{
		if (m_ConHostSearch)
			m_ConHostSearch.Release();
	}
	else
	{
		bConHostLocked = mp_ConEmu->LockConhostStart();
	}


	// In case if console was restarted with another icon (shell/task)
	if (isActive(false))
		mp_ConEmu->Taskbar_UpdateOverlay();


	int nStep = 1;

	UpdateStartState(rss_StartingServer);


	// Go
	while (nStep <= 2)
	{
		_ASSERTE(nStep==1 || nStep==2);
		MCHKHEAP;

		CEStr curCmd;

		// Do actual process creation
		lbRc = StartProcessInt(lpszCmd, curCmd, lpszWorkDir, bNeedConHostSearch, hSetForeground,
			nCreateBegin, nCreateEnd, nCreateDuration, nTextColorIdx, nBackColorIdx, nPopTextColorIdx, nPopBackColorIdx, si, pi, dwLastError);

		if (lbRc)
		{
			UpdateStartState(rss_ServerStarted);
			break; // OK, started
		}

		/* End of process start-up */

		// Failure!!
		if (InRecreate())
		{
			m_Args.Detached = crb_On;
			_ASSERTE(mh_MainSrv==nullptr);
			SafeCloseHandle(mh_MainSrv);
			_ASSERTE(isDetached());
			wchar_t szErrInfo[80];
			swprintf_c(szErrInfo, L"Restart console failed (code %i)", static_cast<int>(dwLastError));
			SetConStatus(szErrInfo, cso_Critical);
		}

		//Box("Cannot execute the command.");
		//DWORD dwLastError = GetLastError();
		DEBUGSTRPROC(L"CreateProcess failed\n");

		// Get description of 'dwLastError'
		size_t cchFormatMax = 1024;
		CEStr errDescr;
		if (errDescr.GetBuffer(cchFormatMax))
		{
			if (0 == FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
				nullptr, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				errDescr.data(), cchFormatMax, nullptr))
			{
				swprintf_c(errDescr.data(), cchFormatMax/*#SECURELEN*/, L"Unknown system error: 0x%x", dwLastError);
			}
		}

		wchar_t descrAndCode[128] = L"";
		if (dwLastError == ERROR_ACCESS_DENIED)
			swprintf_c(descrAndCode, std::size(descrAndCode), L"Can't create new console, check your antivirus\r\nError code: %i\r\n", static_cast<int>(dwLastError));
		else
			swprintf_c(descrAndCode, std::size(descrAndCode), L"Can't create new console, command execution failed\r\nError code: %i\r\n", static_cast<int>(dwLastError));

		int nButtons = MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND;
		LPCWSTR actionDescr = nullptr;
		if (nStep == 1)
		{
			actionDescr = L"\r\n\r\n" L"Try again with new console dialog?";
			nButtons |= MB_YESNO;
		}

		const CEStr errorText(descrAndCode, L"\r\n",
			curCmd.c_str(), L"\r\n\r\nWorking folder:\r\n\"",
			lpszWorkDir ? lpszWorkDir : L"%USERPROFILE%", L"\"",
			actionDescr);

		MCHKHEAP
		int nBrc = MsgBox(errorText, nButtons, mp_ConEmu->GetDefaultTitle(), nullptr, false);

		if (nBrc == IDYES)
		{
			RConStartArgsEx args;
			args.AssignFrom(m_Args);
			args.aRecreate = cra_CreateTab; // cra_EditTab; -- button "Start" is expected instead of ambiguous "Save"

			int nCreateRc = mp_ConEmu->RecreateDlg(&args);

			if (nCreateRc != IDC_START)
			{
				nBrc = IDCANCEL;
			}
			else
			{
				SafeFree(m_Args.pszSpecialCmd);
				SafeFree(m_Args.pszStartupDir);
				// Rest of fields will be cleared by AssignFrom
				m_Args.AssignFrom(args);
				lpszCmd = m_Args.pszSpecialCmd;
			}
		}

		if (nBrc != IDYES)
		{
			if (mp_ConEmu->isValid(pVCon))
			{
				mb_InCreateRoot = FALSE;
				if (InRecreate())
				{
					// #TODO: Set Detached flag?
				}
				else
				{
					CloseConsole(false, false);
				}
			}
			else
			{
				_ASSERTE(mp_ConEmu->isValid(pVCon));
			}
			lbRc = FALSE;
			goto wrap;
		}

		nStep ++;
		MCHKHEAP

		// Command was changed, refresh settings
		PrepareDefaultColors(nTextColorIdx, nBackColorIdx, nPopTextColorIdx, nPopBackColorIdx, true);
		if (nTextColorIdx <= 15 || nBackColorIdx <= 15)
		{
			si.dwFlags |= STARTF_USEFILLATTRIBUTE;
			si.dwFillAttribute = MAKECONCOLOR(nTextColorIdx, nBackColorIdx);
		}
	}

	MCHKHEAP

	MCHKHEAP
	SafeCloseHandle(pi.hThread);
	//CloseHandle(pi.hProcess); pi.hProcess = nullptr;
	SetMainSrvPID(pi.dwProcessId, pi.hProcess);
	//mn_MainSrv_PID = pi.dwProcessId;
	//mh_MainSrv = pi.hProcess;
	pi.hProcess = nullptr;

	if (bNeedConHostSearch)
	{
		_ASSERTE(lbRc);
		// Search for new "conhost.exe" processes
		// But it could be not in time, so no warnings yet
		if (ConHostSearch(false))
		{
			m_ConHostSearch.Release();
		}
		// Do Unlock immediately
		if (bConHostLocked)
		{
			mp_ConEmu->UnlockConhostStart();
			bConHostLocked = false;
		}
	}

	if (m_Args.RunAsAdministrator != crb_On)
	{
		// names of pipes, events, mappings, etc.
		InitNames();
	}

wrap:

	if (bConHostLocked)
	{
		mp_ConEmu->UnlockConhostStart();
		bConHostLocked = false;
	}

	// In "As Admin" mode we are in the "CreateRoot" until started server notifies us
	if (!lbRc || mn_MainSrv_PID)
	{
		mb_InCreateRoot = FALSE;
	}
	else
	{
		_ASSERTE(m_Args.RunAsAdministrator == crb_On);
	}

	mb_StartResult = lbRc;
	mb_NeedStartProcess = FALSE;
	SetEvent(mh_StartExecuted);
	#ifdef _DEBUG
	SetEnvironmentVariable(ENV_CONEMU_MONITOR_INTERNAL_W, nullptr);
	#endif
	// Let know calling function about the problem
	SetLastError(dwLastError);
	// Allow to recreate console again
	if (!lbRc && mn_InRecreate)
		mb_RecreateFailed = TRUE;
	return lbRc;
}

bool CRealConsole::StartProcessInt(LPCWSTR& lpszCmd, CEStr& curCmdBuffer, LPCWSTR& lpszWorkDir,
								   bool bNeedConHostSearch, HWND hSetForeground,
								   DWORD& nCreateBegin, DWORD& nCreateEnd, DWORD& nCreateDuration,
								   BYTE nTextColorIdx /*= 7*/, BYTE nBackColorIdx /*= 0*/, BYTE nPopTextColorIdx /*= 5*/, BYTE nPopBackColorIdx /*= 15*/,
								   STARTUPINFO& si, PROCESS_INFORMATION& pi, DWORD& dwLastError)
{
	bool lbRc = false;
	const DWORD nColors = (nTextColorIdx) | (nBackColorIdx << 8) | (nPopTextColorIdx << 16) | (nPopBackColorIdx << 24);

	_ASSERTE(mn_RunTime==0 && mn_StartTick==0); // should not be set yet, or should be already cleared

	const bool bDirChanged = gpConEmu->ChangeWorkDir(gpConEmu->WorkDir(lpszWorkDir));
	const auto* lpszConEmuC = mp_ConEmu->ConEmuCExeFull(lpszCmd);
	if (bDirChanged)
		gpConEmu->ChangeWorkDir(nullptr);

	GetLocalTime(&mst_ServerStartingTime);
	SYSTEMTIME* lpst = (lstrcmpi(PointToName(lpszConEmuC), L"ConEmuC64.exe") == 0)
		? &mp_ConEmu->mst_LastConsole64StartTime
		: &mp_ConEmu->mst_LastConsole32StartTime;
	const bool bIsFirstConsole = ((lpst->wYear != mst_ServerStartingTime.wYear)
		|| (lpst->wMonth != mst_ServerStartingTime.wMonth)
		|| (lpst->wDay != mst_ServerStartingTime.wDay));
	*lpst = mst_ServerStartingTime;

	int nCurLen = 0;
	if (lpszCmd == nullptr)
	{
		_ASSERTE(lpszCmd!=nullptr);
		lpszCmd = L"";
	}
	INT_PTR nLen = _tcslen(lpszCmd);
	nLen += _tcslen(mp_ConEmu->ms_ConEmuExe) + 380 + MAX_PATH * 2;
	MCHKHEAP;

	auto* psCurCmd = curCmdBuffer.GetBuffer(nLen);
	_ASSERTE(psCurCmd);


	// Begin generation of execution command line
	*psCurCmd = 0;

	if (m_Args.RunAsSystem == crb_On)
	{
		_ASSERTE(m_Args.RunAsAdministrator == crb_On);
		_wcscat_c(psCurCmd, nLen, L"\"");
		_wcscat_c(psCurCmd, nLen, mp_ConEmu->ms_ConEmuExe);
		_wcscat_c(psCurCmd, nLen, L"\" ");
		if (gpSet->isLogging())
			_wcscat_c(psCurCmd, nLen, L"-log ");
		DWORD nSessionId = static_cast<DWORD>(-1);
		apiQuerySessionID(GetCurrentProcessId(), nSessionId);
		const INT_PTR curLen = _tcslen(psCurCmd);
		msprintf(psCurCmd + curLen, nLen - curLen, L"-system:%u -cmd ", nSessionId);
	}

	_wcscat_c(psCurCmd, nLen, L"\"");
	// Copy to psCurCmd full path to ConEmuC.exe or ConEmuC64.exe
	_wcscat_c(psCurCmd, nLen, lpszConEmuC);
	//lstrcat(psCurCmd, L"\\");
	//lstrcpy(psCurCmd, mp_ConEmu->ms_ConEmuCExeName);
	_wcscat_c(psCurCmd, nLen, L"\" ");

	const unsigned logLevel = isLogging();
	if (logLevel)
		_wcscat_c(psCurCmd, nLen, (logLevel == 3) ? L" /LOG3" : (logLevel == 2) ? L" /LOG2" : L" /LOG");

	if ((m_Args.RunAsAdministrator == crb_On) && !mp_ConEmu->mb_IsUacAdmin)
	{
		m_Args.Detached = crb_On;
		_wcscat_c(psCurCmd, nLen, L" /ADMIN");
	}

	// Run CheckAndWarnHookers() only for the first console
	// and if the switch `-NoHooksWars` was not specified
	if (!bIsFirstConsole || gpConEmu->opt.NoHooksWarn)
	{
		_wcscat_c(psCurCmd, nLen, L" /OMITHOOKSWARN");
	}

	// Console modes (insert/overwrite)
	{
		DWORD nMode =
			// (gpSet->nConInMode != (DWORD)-1) ? gpSet->nConInMode : 0
			(ENABLE_INSERT_MODE << 16) // Mask for insert/overwrite mode
			;
		if (m_Args.OverwriteMode == crb_On)
			nMode &= ~ENABLE_INSERT_MODE; // Turn bit OFF (Overwrite mode). Yep, it's NOP, but here for compatibility and clearness.
		else
			nMode |= ENABLE_INSERT_MODE; // Turn bit ON (Insert mode)

		// gh-1007: disable ENABLE_QUICK_EDIT_MODE by default
		nMode |= (ENABLE_QUICK_EDIT_MODE << 16);

		nCurLen = _tcslen(psCurCmd);
		swprintf_c(psCurCmd+nCurLen, nLen-nCurLen/*#SECURELEN*/, L" /CINMODE=%X", nMode);
	}

	_ASSERTE(mp_RBuf==mp_ABuf);
	const int nWndWidth = mp_RBuf->GetTextWidth();
	const int nWndHeight = mp_RBuf->GetTextHeight();
	/*было - GetConWindowSize(con.m_sbi, nWndWidth, nWndHeight);*/
	_ASSERTE(nWndWidth>0 && nWndHeight>0);

	const DWORD ActiveId = GetMonitorThreadID();
	_ASSERTE(mp_ConEmu->mp_RunQueue->isRunnerThread());
	_ASSERTE(mn_MonitorThreadID != 0 && mn_MonitorThreadID == ActiveId && ActiveId != GetCurrentThreadId());

	nCurLen = _tcslen(psCurCmd);
	_wsprintf(psCurCmd + nCurLen, SKIPLEN(nLen - nCurLen)
		L" /AID=%u /GID=%u /GHWND=%08X /BW=%i /BH=%i /BZ=%i \"/FN=%s\" /FW=%i /FH=%i /TA=%08X",
		ActiveId, GetCurrentProcessId(), LODWORD(ghWnd), nWndWidth, nWndHeight, mn_DefaultBufferHeight,
		gpSet->ConsoleFont.lfFaceName, gpSet->ConsoleFont.lfWidth, gpSet->ConsoleFont.lfHeight,
		nColors);

	/*if (gpSet->FontFile[0]) { --  РЕГИСТРАЦИЯ ШРИФТА НА КОНСОЛЬ НЕ РАБОТАЕТ!
		wcscat(psCurCmd, L" \"/FF=");
		wcscat(psCurCmd, gpSet->FontFile);
		wcscat(psCurCmd, L"\"");
	}*/

	if (!gpSet->isConVisible)
		_wcscat_c(psCurCmd, nLen, L" /HIDE");

	// /CONFIRM | /NOCONFIRM | /NOINJECT
	m_Args.AppendServerArgs(psCurCmd, nLen);

	_wcscat_c(psCurCmd, nLen, L" /ROOT ");
	_wcscat_c(psCurCmd, nLen, lpszCmd);
	MCHKHEAP;
	dwLastError = 0;
	#ifdef MSGLOGGER
	DEBUGSTRPROC(psCurCmd);
	#endif

	#ifdef _DEBUG
	wchar_t monitorId[20] = L"";
	swprintf_c(monitorId, L"%u", ActiveId);
	SetEnvironmentVariable(ENV_CONEMU_MONITOR_INTERNAL_W, monitorId);
	#endif

	if (bNeedConHostSearch)
		ConHostSearchPrepare();

	nCreateBegin = GetTickCount();

	GetLocalTime(&m_StartTime);
	ms_StartWorkDir.Set(lpszWorkDir);
	ms_CurWorkDir.Set(lpszWorkDir);
	ms_CurPassiveDir.Set(nullptr);

	if (isLogging())
	{
		const CEStr szNewCon = m_Args.CreateCommandLine(false);
		const CEStr lsLog(L"Starting VCon[", mp_VCon->IndexStr(), L"]: Cmd=‘", psCurCmd, L"’, NewCon=‘", szNewCon.ms_Val, L"’");
		LogString(lsLog);
	}

	// Create process or RunAs
	lbRc = CreateOrRunAs(this, m_Args, psCurCmd, lpszWorkDir, si, pi, mp_sei, dwLastError);

	if (isLogging())
	{
		wchar_t szInfo[32] = L"";
		if (!lbRc)
		{
			const CEStr lsLog(L"VCon[", mp_VCon->IndexStr(), L"] failed, code=", ultow_s(dwLastError, szInfo, 10));
			LogString(lsLog);
		}
		else
		{
			if (pi.dwProcessId)
				swprintf_c(szInfo, L", ServerPID=%u", pi.dwProcessId);
			const CEStr lsLog(L"VCon[", mp_VCon->IndexStr(), L"] started", szInfo);
			LogString(lsLog);
		}
	}

	nCreateEnd = GetTickCount();
	nCreateDuration = nCreateEnd - nCreateBegin;

	// If known - save main server PID and HANDLE
	if (lbRc && ((m_Args.RunAsAdministrator != crb_On) || mp_ConEmu->mb_IsUacAdmin))
	{
		SetMainSrvPID(pi.dwProcessId, pi.hProcess);
	}

	if (lbRc)
	{
		mn_StartTick = GetTickCount(); mn_RunTime = 0;

		if (m_Args.RunAsAdministrator != crb_On)
		{
			ProcessUpdate(&pi.dwProcessId, 1);
			AllowSetForegroundWindow(pi.dwProcessId);
		}

		// 131022 If any other process was activated by user during our initialization - don't grab the focus
		HWND hCurFore = nullptr;
		bool bMeFore = mp_ConEmu->isMeForeground(true/*real*/, true/*dialog*/, &hCurFore) || (hCurFore == nullptr);
		DEBUGTEST(wchar_t szForeClass[255] = L"");
		if (!bMeFore && !::IsWindowVisible(hCurFore))
		{
			// But current console window handle may not initialized yet, try to check it
			#ifdef _DEBUG
			GetClassName(hCurFore, szForeClass, countof(szForeClass));
			_ASSERTE(!IsConsoleClass(szForeClass)); // Some other GUI application was started by user?
			#endif
			bMeFore = true;
		}
		if (bMeFore)
		{
			apiSetForegroundWindow(hSetForeground);
		}

		DEBUGSTRPROC(L"CreateProcess OK\n");
	}
	else if (mn_InRecreate)
	{
		m_Args.Detached = crb_On;
		mn_InRecreate = static_cast<DWORD>(-1);
	}

	std::ignore = nCreateDuration;
	/* End of process start-up */
	return lbRc;
}

// (Args may be != pRCon->m_Args)
bool CRealConsole::CreateOrRunAs(CRealConsole* pRCon, RConStartArgsEx& Args,
				   LPWSTR psCurCmd, LPCWSTR& lpszWorkDir,
				   STARTUPINFO& si, PROCESS_INFORMATION& pi, SHELLEXECUTEINFO*& pp_sei,
				   DWORD& dwLastError, bool bExternal /*= false*/)
{
	bool lbRc = false;

	lpszWorkDir = pRCon->GetStartupDir();

	// Function may be used for starting GUI applications (errors & hyperlinks)
	bool bConsoleProcess = true;
	{
		CEStr szExe;
		DWORD nSubSys = 0, nBits = 0, nAttrs = 0;
		if (!IsNeedCmd(TRUE, psCurCmd, szExe))
		{
			if (GetImageSubsystem(szExe, nSubSys, nBits, nAttrs))
				if (nSubSys == IMAGE_SUBSYSTEM_WINDOWS_GUI)
					bConsoleProcess = false;
		}
	}

	SetLastError(0);

	// Если сам ConEmu запущен под админом - нет смысла звать ShellExecuteEx("RunAs")
	if ((Args.RunAsAdministrator != crb_On) || pRCon->mp_ConEmu->mb_IsUacAdmin)
	{
		LockSetForegroundWindow(bExternal ? LSFW_UNLOCK : LSFW_LOCK);
		pRCon->SetConStatus(L"Starting root process...", cso_ResetOnConsoleReady|cso_Critical);

		MWow64Disable wow;
		wow.Disable();

		if (Args.pszUserName != nullptr)
		{
			// When starting under another credentials - try to use %USERPROFILE% instead of "system32"
			HRESULT hr = E_FAIL;
			wchar_t szUserDir[MAX_PATH] = L"";
			LPCWSTR workDirTemp = lpszWorkDir;
			CEStr pszChangedCmd;
			// Issue 1557: switch -new_console:u:"other_user:password" lock the account of other_user
			if (!workDirTemp || !*workDirTemp)
			{
				// We need something existing in both account to run our server process
				hr = SHGetFolderPath(nullptr, CSIDL_SYSTEM, nullptr, SHGFP_TYPE_CURRENT, szUserDir);
				if (FAILED(hr) || !*szUserDir)
					wcscpy_c(szUserDir, L"C:\\");
				workDirTemp = szUserDir;
				// Force SetCurrentDirectory("%USERPROFILE%") in the server
				CmdArg exe;
				LPCWSTR pszTemp = psCurCmd;
				if ((pszTemp = NextArg(pszTemp, exe)))
					pszChangedCmd = CEStr(exe, L" /PROFILECD ", pszTemp);
			}
			DWORD nFlags = (Args.RunAsNetOnly == crb_On) ? LOGON_NETCREDENTIALS_ONLY : LOGON_WITH_PROFILE;
			lbRc = (CreateProcessWithLogonW(Args.pszUserName, Args.pszDomain, Args.szUserPassword,
										nFlags, nullptr, pszChangedCmd ? pszChangedCmd.data() : psCurCmd,
										NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE
										|(bConsoleProcess ? CREATE_NEW_CONSOLE : 0)
										, nullptr, workDirTemp, &si, &pi) != FALSE);
				//if (CreateProcessAsUser(Args.hLogonToken, nullptr, psCurCmd, nullptr, nullptr, FALSE,
				//	NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_CONSOLE
				//	, nullptr, Args.pszStartupDir, &si, &pi))

			dwLastError = GetLastError();

			SecureZeroMemory(Args.szUserPassword, sizeof(Args.szUserPassword));
		}
		else if (Args.RunAsRestricted == crb_On)
		{
			lbRc = (CreateProcessRestricted(nullptr, psCurCmd, nullptr, nullptr, FALSE,
										NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE
										|(bConsoleProcess ? CREATE_NEW_CONSOLE : 0)
										, nullptr, lpszWorkDir, &si, &pi, &dwLastError) != FALSE);

			dwLastError = GetLastError();
		}
		else
		{
			lbRc = (CreateProcess(nullptr, psCurCmd, nullptr, nullptr, FALSE,
										NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE
										|(bConsoleProcess ? CREATE_NEW_CONSOLE : 0)
										//|CREATE_NEW_PROCESS_GROUP - низя! перестает срабатывать Ctrl-C
										, nullptr, lpszWorkDir, &si, &pi) != FALSE);

			dwLastError = GetLastError();
		}

		DEBUGSTRPROC(L"CreateProcess finished\n");
		//if (Args.hLogonToken) { CloseHandle(Args.hLogonToken); Args.hLogonToken = nullptr; }
		if (!bExternal)
			LockSetForegroundWindow(LSFW_UNLOCK);
		else if (lbRc && pi.dwProcessId)
			AllowSetForegroundWindow(pi.dwProcessId);
	}
	else // Args.bRunAsAdministrator
	{
		LPCWSTR pszCmd = psCurCmd;
		CmdArg szExec;

		if (!(pszCmd = NextArg(pszCmd, szExec)))
		{
			lbRc = FALSE;
			dwLastError = -1;
		}
		else
		{
			if (pp_sei)
			{
				SafeCloseHandle(pp_sei->hProcess);
				SafeFree(pp_sei);
			}

			const INT_PTR cchDirLen = lpszWorkDir ? _tcslen(lpszWorkDir) : 0;
			const INT_PTR cchExecLen = _tcslen(szExec);
			const INT_PTR cchCmdLen = pszCmd ? _tcslen(pszCmd) : 0;
			int nWholeSize = sizeof(SHELLEXECUTEINFO)
				                + sizeof(wchar_t) *
				                (10  /* Verb */
				                + cchExecLen + 2
				                + cchCmdLen + 2
				                + cchDirLen + 2
				                );
			pp_sei = (SHELLEXECUTEINFO*)calloc(nWholeSize, 1);
			pp_sei->cbSize = sizeof(SHELLEXECUTEINFO);
			pp_sei->hwnd = ghWnd;
			//pp_sei->hwnd = /*nullptr; */ ghWnd; // почему я тут nullptr ставил?

			// 121025 - remove SEE_MASK_NOCLOSEPROCESS
			pp_sei->fMask = SEE_MASK_NOASYNC | (bConsoleProcess ? SEE_MASK_NO_CONSOLE : 0);
			// Issue 791: Console server fails to duplicate self Process handle to GUI
			pp_sei->fMask |= SEE_MASK_NOCLOSEPROCESS;

			pp_sei->lpVerb = (wchar_t*)(pp_sei+1);
			wcscpy_s((wchar_t*)pp_sei->lpVerb, 6, L"runas");
			pp_sei->lpFile = pp_sei->lpVerb + _tcslen(pp_sei->lpVerb) + 2;
			wcscpy_s((wchar_t*)pp_sei->lpFile, cchExecLen+1, szExec);
			pp_sei->lpParameters = pp_sei->lpFile + _tcslen(pp_sei->lpFile) + 2;

			if (pszCmd)
			{
				*(wchar_t*)pp_sei->lpParameters = L' ';
				wcscpy_s((wchar_t*)(pp_sei->lpParameters+1), cchCmdLen+1, pszCmd);
			}

			pp_sei->lpDirectory = pp_sei->lpParameters + _tcslen(pp_sei->lpParameters) + 2;

			if (lpszWorkDir && *lpszWorkDir)
				_wcscpy_c((wchar_t*)pp_sei->lpDirectory, cchDirLen+1, lpszWorkDir);
			else
				pp_sei->lpDirectory = nullptr;

			//pp_sei->nShow = gpSet->isConVisible ? SW_SHOWNORMAL : SW_HIDE;
			//pp_sei->nShow = SW_SHOWMINIMIZED;
			pp_sei->nShow = SW_HIDE;

			// GuiShellExecuteEx запускается в основном потоке, поэтому nCreateDuration здесь не считаем
			pRCon->SetConStatus((gOSVer.dwMajorVersion>=6) ? L"Starting root process as Administrator..." : L"Starting root process as user...", cso_ResetOnConsoleReady|cso_Critical);
			//lbRc = mp_ConEmu->GuiShellExecuteEx(pp_sei, mp_VCon);

			pRCon->mp_ConEmu->SetIgnoreQuakeActivation(true);

			lbRc = (ShellExecuteEx(pp_sei) != FALSE);

			pRCon->mp_ConEmu->SetIgnoreQuakeActivation(false);

			// ошибку покажем дальше
			dwLastError = GetLastError();
		}
	}

	return lbRc;
}

// Инициализировать/обновить имена пайпов, событий, мэппингов и т.п.
void CRealConsole::InitNames()
{
	DWORD nSrvPID = GetServerPID(false);
	// Имя пайпа для управления ConEmuC
	swprintf_c(ms_ConEmuC_Pipe, CESERVERPIPENAME, L".", nSrvPID);
	swprintf_c(ms_MainSrv_Pipe, CESERVERPIPENAME, L".", mn_MainSrv_PID);
	swprintf_c(ms_ConEmuCInput_Pipe, CESERVERINPUTNAME, L".", mn_MainSrv_PID);
	// Имя событие измененности данных в консоли
	m_ConDataChanged.InitName(CEDATAREADYEVENT, nSrvPID);
	//swprintf_c(ms_ConEmuC_DataReady, CEDATAREADYEVENT, mn_MainSrv_PID);
	MCHKHEAP;
	m_GetDataPipe.InitName(mp_ConEmu->GetDefaultTitle(), CESERVERREADNAME, L".", nSrvPID);
	// Enable overlapped mode and termination by event
	_ASSERTE(mh_TermEvent!=nullptr);
	m_GetDataPipe.SetTermEvent(mh_TermEvent);
}

// Если включена прокрутка - скорректировать индекс ячейки из экранных в буферные
COORD CRealConsole::ScreenToBuffer(const COORD crMouse)
{
	AssertThisRet(crMouse);
	return mp_ABuf->ScreenToBuffer(crMouse);
}

COORD CRealConsole::BufferToScreen(const COORD crMouse, const bool bFixup /*= true*/, const bool bVertOnly /*= false*/)
{
	AssertThisRet(crMouse);
	return mp_ABuf->BufferToScreen(crMouse, bFixup, bVertOnly);
}

// x,y - экранные координаты
void CRealConsole::OnScroll(UINT messg, WPARAM wParam, int x, int y, bool abFromTouch /*= false*/)
{
	#ifdef _DEBUG
	wchar_t szDbgInfo[200]; swprintf_c(szDbgInfo, L"RBuf::OnMouse %s XY={%i,%i}%s\n",
		messg==WM_MOUSEWHEEL?L"WM_MOUSEWHEEL":messg==WM_MOUSEHWHEEL?L"WM_MOUSEHWHEEL":
		L"{OtherMsg}", x,y, (abFromTouch?L" Touch":L""));
	DEBUGSTRMOUSE(szDbgInfo);
	#endif

	switch (messg)
	{
	case WM_MOUSEWHEEL:
	{
		m_Mouse.WheelDirAccum += (SHORT)HIWORD(wParam);
		m_Mouse.WheelAccumulated = (HIWORD(wParam) && std::abs((SHORT)HIWORD(wParam)) < WHEEL_DELTA);
		if (std::abs(m_Mouse.WheelDirAccum) < WHEEL_DELTA)
			break;
		UINT nCount = (abFromTouch || m_Mouse.WheelAccumulated) ? 1 : gpConEmu->mouse.GetWheelScrollLines();
		if (m_Mouse.WheelAccumulated)
			nCount *= (std::abs(m_Mouse.WheelDirAccum) / WHEEL_DELTA);

		BOOL lbCtrl = isPressed(VK_CONTROL);
		int nDirCmd = (m_Mouse.WheelDirAccum > 0)
			? (lbCtrl ? SB_PAGEUP : SB_LINEUP)
			: (lbCtrl ? SB_PAGEDOWN : SB_LINEDOWN);
		// Keep Far's real viewport at its cursor. Only move the GUI viewport,
		// otherwise Far redraws pull the real console straight back to the bottom.
		mp_ABuf->DoScrollBuffer(nDirCmd, -1, nCount,
			gpSet->isFarWheelScrollback && isFar() && isFarBufferSupported());

		m_Mouse.WheelDirAccum %= WHEEL_DELTA;
		break;
	} // WM_MOUSEWHEEL

	case WM_MOUSEHWHEEL:
	{
		TODO("WM_MOUSEHWHEEL - горизонтальная прокрутка");
		_ASSERTE(FALSE && "Horz scrolling! WM_MOUSEHWHEEL");
		//return true; -- когда будет готово - return true;
		break;
	} // WM_MOUSEHWHEEL
	} // switch (messg)
}

bool CRealConsole::OnMouseSelection(UINT messg, WPARAM wParam, int x, int y)
{
	AssertThisRet(false);
	if (!hConWnd)
		return false;

	if (isSelectionPresent()
		&& ((wParam & MK_LBUTTON) || messg == WM_LBUTTONUP))
	{
		return OnMouse(messg, wParam, x, y);
	}

	return false;
}

// x,y - экранные координаты
// Если abForceSend==true - не проверять на "повторность" события, и не проверять "isPressed(VK_?BUTTON)"
bool CRealConsole::OnMouse(UINT messg, WPARAM wParam, int x, int y, bool abForceSend /*= false*/)
{
	AssertThisRet(false);

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL                  0x020E
#endif
#ifdef _DEBUG
	wchar_t szDbg[60];
	swprintf_c(szDbg, L"RCon::Mouse(messg=%s) at DC {%ix%i} %s",
		GetMouseMsgName(messg), x, y, abForceSend ? L"ForceSend" : L"");
	static UINT lastMsg = 0;
	if ((messg != WM_MOUSEMOVE) || (messg != lastMsg))
	{
		DEBUGSTRINPUTMSG(szDbg);
	}
	lastMsg = messg;
#endif

	if (!hConWnd)
	{
		return false;
	}

	// No sense to process mouse events (hyperlinks, selection, etc.)
	// if neither process (console application) was started
	// nor server initialization finishes (data was aquired from console)
	if (m_StartState <= rss_DataAquired)
	{
		return false;
	}

	if (messg != WM_MOUSEMOVE)
	{
		m_Mouse.crLastMouseEventPos.X = m_Mouse.crLastMouseEventPos.Y = -1;
	}

	// Если включен фаровский граббер - то координаты нужны скорректированные, чтобы точно позиции выделять
	TODO("StrictMonospace включать пока нельзя, т.к. сбивается клик в редакторе, например. Да и в диалогах есть текстовые поля!");
	bool bStrictMonospace = false; //!isConSelectMode(); // она реагирует и на фаровский граббер

	// Получить известные координаты символов
	COORD crMouse = ScreenToBuffer(mp_VCon->ClientToConsole(x,y, bStrictMonospace));

	// Do this BEFORE check in ABuf
	if (messg == WM_LBUTTONDOWN)
		m_Mouse.bWasMouseSelection = false;

	// Buffer may process mouse events by itself (selections/copy/paste, scroll, ...)
	if (mp_ABuf->OnMouse(messg, wParam, x, y, crMouse))
	{
		if (isWheelEvent(messg))
			mp_ConEmu->LogString(L"-- skipped (mp_ABuf)\n", true, false);
		return true;
	}

	#ifdef _DEBUG
	if (mp_ABuf->isSelectionPresent())
	{
		_ASSERTE(FALSE && "Buffer must process mouse internally");
	}
	#endif


	if (isFar() && mp_ConEmu->IsGesturesEnabled())
	{
		//120830 - для Far Manager: облегчить "попадание"
		if (messg == WM_LBUTTONDOWN)
		{
			bool bChanged = false;
			m_Mouse.crMouseTapReal = crMouse;

			// Клик мышкой в {0x0} гасит-показывает панели, но на планшете - фиг попадешь.
			// Тап в область часов будет делать то же самое
			// Считаем, что если тап пришелся на 2-ю строку - тоже.
			// В редакторе/вьювере не дергаться - там смысла нет.
			if (!(isEditor() || isViewer()) && (crMouse.Y <= 1) && ((crMouse.X + 5) >= (int)TextWidth()))
			{
				bChanged = true;
				m_Mouse.crMouseTapChanged = MakeCoord(0,0);
			}

			if (bChanged)
			{
				crMouse = m_Mouse.crMouseTapChanged;
				m_Mouse.bMouseTapChanged = TRUE;
			}
			else
			{
				m_Mouse.bMouseTapChanged = FALSE;
			}

		}
		else if (m_Mouse.bMouseTapChanged && (messg == WM_LBUTTONUP || messg == WM_MOUSEMOVE))
		{
			if (m_Mouse.crMouseTapReal.X == crMouse.X && m_Mouse.crMouseTapReal.Y == crMouse.Y)
			{
				crMouse = m_Mouse.crMouseTapChanged;
			}
			else
			{
				m_Mouse.bMouseTapChanged = FALSE;
			}
		}
	}
	else
	{
		m_Mouse.bMouseTapChanged = FALSE;
	}


	const AppSettings* pApp = nullptr;
	if ((messg == WM_LBUTTONUP) && !m_Mouse.bWasMouseSelection
		&& ((pApp = gpSet->GetAppSettings(GetActiveAppSettingsId())) != nullptr)
		&& pApp->CTSClickPromptPosition()
		&& gpSet->IsModifierPressed(vkCTSVkPromptClk, true)
		&& IsPromptActionAllowed(true, pApp))
	{
		if (ChangePromptPosition(pApp, crMouse))
		{
			return true;
		}
	}

	// If user has disabled posting mouse events to console
	if (gpSet->isDisableMouse)
	{
		if (isWheelEvent(messg))
			mp_ConEmu->LogString(L"-- skipped (isDisableMouse)\n", true, false);
		return false;
	}

	// Issue 1165: Don't send "Mouse events" into console input buffer if ENABLE_QUICK_EDIT_MODE
	if (!abForceSend && !isSendMouseAllowed())
	{
		return false;
	}

	DEBUGTEST(bool lbFarBufferSupported = isFarBufferSupported());

	// Если консоль в режиме с прокруткой - не посылать мышь в консоль
	// Иначе получаются казусы. Если во время выполнения команды (например "dir c: /s")
	// успеть дернуть мышкой - то при возврате в ФАР сразу пойдет фаровский драг
	// -- if (isBufferHeight() && !lbFarBufferSupported)
	// -- заменено на сброс мышиных событий в ConEmuHk при завершении консольного приложения
	if (isFarInStack() && !gpSet->isUseInjects)
		return false;

	// Если MouseWheel таки посылается в консоль - сбросить TopLeft чтобы избежать коллизий
	if ((messg == WM_MOUSEWHEEL || messg == WM_MOUSEHWHEEL) && (mp_ABuf->m_Type == rbt_Primary))
	{
		mp_ABuf->ResetTopLeft();
	}

	PostMouseEvent(messg, wParam, crMouse, abForceSend);

	if (messg == WM_MOUSEMOVE)
	{
		m_Mouse.ptLastMouseGuiPos.x = x; m_Mouse.ptLastMouseGuiPos.y = y;
	}

	return true;
}

// Prepare dwControlKeyState for INPUT_RECORD CAPSLOCK_ON, NUMLOCK_ON, SCROLLLOCK_ON
void CRealConsole::AddIndicatorsCtrlState(DWORD& dwControlKeyState)
{
	if (GetKeyState(VK_CAPITAL) & 1)
		dwControlKeyState |= CAPSLOCK_ON;

	if (GetKeyState(VK_NUMLOCK) & 1)
		dwControlKeyState |= NUMLOCK_ON;

	if (GetKeyState(VK_SCROLL) & 1)
		dwControlKeyState |= SCROLLLOCK_ON;
}

// Prepare dwControlKeyState for LEFT_ALT_PRESSED, RIGHT_ALT_PRESSED, LEFT_CTRL_PRESSED, RIGHT_CTRL_PRESSED, SHIFT_PRESSED
void CRealConsole::AddModifiersCtrlState(DWORD& dwControlKeyState)
{
	if (isPressed(VK_LMENU))
		dwControlKeyState |= LEFT_ALT_PRESSED;

	if (isPressed(VK_RMENU))
		dwControlKeyState |= RIGHT_ALT_PRESSED;

	if (isPressed(VK_LCONTROL))
		dwControlKeyState |= LEFT_CTRL_PRESSED;

	if (isPressed(VK_RCONTROL))
		dwControlKeyState |= RIGHT_CTRL_PRESSED;

	if (isPressed(VK_SHIFT))
		dwControlKeyState |= SHIFT_PRESSED;
}

void CRealConsole::PostMouseEvent(UINT messg, WPARAM wParam, COORD crMouse, bool abForceSend /*= false*/)
{
	// По идее, мышь в консоль может пересылаться, только
	// если активный буфер - буфер реальной консоли
	_ASSERTE(mp_ABuf==mp_RBuf);

	INPUT_RECORD r; memset(&r, 0, sizeof(r));
	r.EventType = MOUSE_EVENT;

	// Mouse Buttons
	if (messg != WM_LBUTTONUP && (messg == WM_LBUTTONDOWN || messg == WM_LBUTTONDBLCLK || (!abForceSend && isPressed(VK_LBUTTON))))
		r.Event.MouseEvent.dwButtonState |= FROM_LEFT_1ST_BUTTON_PRESSED;

	if (messg != WM_RBUTTONUP && (messg == WM_RBUTTONDOWN || messg == WM_RBUTTONDBLCLK || (!abForceSend && isPressed(VK_RBUTTON))))
		r.Event.MouseEvent.dwButtonState |= RIGHTMOST_BUTTON_PRESSED;

	if (messg != WM_MBUTTONUP && (messg == WM_MBUTTONDOWN || messg == WM_MBUTTONDBLCLK || (!abForceSend && isPressed(VK_MBUTTON))))
		r.Event.MouseEvent.dwButtonState |= FROM_LEFT_2ND_BUTTON_PRESSED;

	if (messg != WM_XBUTTONUP && (messg == WM_XBUTTONDOWN || messg == WM_XBUTTONDBLCLK))
	{
		if ((HIWORD(wParam) & 0x0001/*XBUTTON1*/))
			r.Event.MouseEvent.dwButtonState |= 0x0008/*FROM_LEFT_3ND_BUTTON_PRESSED*/;
		else if ((HIWORD(wParam) & 0x0002/*XBUTTON2*/))
			r.Event.MouseEvent.dwButtonState |= 0x0010/*FROM_LEFT_4ND_BUTTON_PRESSED*/;
	}

	m_Mouse.bMouseButtonDown = (r.Event.MouseEvent.dwButtonState
	                      & (FROM_LEFT_1ST_BUTTON_PRESSED|FROM_LEFT_2ND_BUTTON_PRESSED|RIGHTMOST_BUTTON_PRESSED)) != 0;

	// Prepare dwControlKeyState for INPUT_RECORD CAPSLOCK_ON, NUMLOCK_ON, SCROLLLOCK_ON
	AddIndicatorsCtrlState(r.Event.MouseEvent.dwControlKeyState);

	// and LEFT_ALT_PRESSED, RIGHT_ALT_PRESSED, LEFT_CTRL_PRESSED, RIGHT_CTRL_PRESSED, SHIFT_PRESSED
	AddModifiersCtrlState(r.Event.MouseEvent.dwControlKeyState);

	// Mouse event flags
	if (messg == WM_LBUTTONDBLCLK || messg == WM_RBUTTONDBLCLK || messg == WM_MBUTTONDBLCLK)
		r.Event.MouseEvent.dwEventFlags = DOUBLE_CLICK;
	else if (messg == WM_MOUSEMOVE)
		r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	else if (messg == WM_MOUSEWHEEL)
	{
		#ifdef SHOWDEBUGSTR
		{
			wchar_t szDbgMsg[128]; swprintf_c(szDbgMsg, L"WM_MOUSEWHEEL(%i, Btns=0x%04X, x=%i, y=%i)\n",
				(int)(short)HIWORD(wParam), (DWORD)LOWORD(wParam), crMouse.X, crMouse.Y);
			DEBUGSTRWHEEL(szDbgMsg);
		}
		#endif
		if (isLogging(2))
		{
			char szDbgMsg[128]; sprintf_c(szDbgMsg, "WM_MOUSEWHEEL(wParam=0x%08X, x=%i, y=%i)", (DWORD)wParam, crMouse.X, crMouse.Y);
			LogString(szDbgMsg);
		}

		WARNING("Если включен режим прокрутки - посылать команду пайпа, а не мышиное событие");
		// Иначе на 2008 server вообще не крутится
		r.Event.MouseEvent.dwEventFlags = MOUSE_WHEELED;
		SHORT nScroll = (SHORT)(((DWORD)wParam & 0xFFFF0000)>>16);

		if (nScroll<0) { if (nScroll > -WHEEL_DELTA) nScroll = -WHEEL_DELTA; }
		else { if (nScroll < WHEEL_DELTA) nScroll = WHEEL_DELTA; }

		if (nScroll < -WHEEL_DELTA || nScroll > WHEEL_DELTA)
			nScroll = ((SHORT)(nScroll / WHEEL_DELTA)) * WHEEL_DELTA;

		r.Event.MouseEvent.dwButtonState |= ((DWORD)(WORD)nScroll) << 16;
		//r.Event.MouseEvent.dwButtonState |= /*(0xFFFF0000 & wParam)*/ (nScroll > 0) ? 0x00780000 : 0xFF880000;
	}
	else if (messg == WM_MOUSEHWHEEL)
	{
		if (isLogging(2))
		{
			char szDbgMsg[128]; sprintf_c(szDbgMsg, "WM_MOUSEHWHEEL(wParam=0x%08X, x=%i, y=%i)", (DWORD)wParam, crMouse.X, crMouse.Y);
			LogString(szDbgMsg);
		}

		r.Event.MouseEvent.dwEventFlags = 8; //MOUSE_HWHEELED
		SHORT nScroll = (SHORT)(((DWORD)wParam & 0xFFFF0000)>>16);

		if (nScroll<0) { if (nScroll > -WHEEL_DELTA) nScroll = -WHEEL_DELTA; }
		else { if (nScroll < WHEEL_DELTA) nScroll = WHEEL_DELTA; }

		if (nScroll < -WHEEL_DELTA || nScroll > WHEEL_DELTA)
			nScroll = ((SHORT)(nScroll / WHEEL_DELTA)) * WHEEL_DELTA;

		r.Event.MouseEvent.dwButtonState |= ((DWORD)(WORD)nScroll) << 16;
	}

	if (messg == WM_LBUTTONDOWN || messg == WM_RBUTTONDOWN || messg == WM_MBUTTONDOWN)
	{
		m_Mouse.bBtnClicked = TRUE; m_Mouse.crBtnClickPos = crMouse;
	}

	// В Far3 поменяли действие ПКМ 0_0
	bool lbRBtnDrag = (r.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) == RIGHTMOST_BUTTON_PRESSED;
	bool lbNormalRBtnMode = false;
	// gpSet->isRSelFix добавлен, чтобы этот fix можно было отключить
	if (lbRBtnDrag && isFar(TRUE) && m_FarInfo.cbSize && gpSet->isRSelFix)
	{
		if ((m_FarInfo.FarVer.dwVerMajor > 3) || (m_FarInfo.FarVer.dwVerMajor == 3 && m_FarInfo.FarVer.dwBuild >= 2381))
		{
			if (gpSet->isRClickSendKey == 0)
			{
				// Если не нажаты LCtrl/RCtrl/LAlt/RAlt - считаем что выделение не идет, отдаем в фар "как есть"
				if (0 == (r.Event.MouseEvent.dwControlKeyState & (RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED|RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED)))
					lbRBtnDrag = false;
			}
			else
			{
				COORD crVisible = mp_ABuf->BufferToScreen(crMouse,false,true);
				// Координата попадает в панель (включая правую/левую рамку)?
				if (CoordInPanel(crVisible, TRUE))
				{
					if (!(r.Event.MouseEvent.dwControlKeyState & (SHIFT_PRESSED|RIGHT_ALT_PRESSED|LEFT_ALT_PRESSED|RIGHT_CTRL_PRESSED|LEFT_CTRL_PRESSED)))
					{
						lbNormalRBtnMode = true;
						// 18.09.2012 Maks - LEFT_CTRL_PRESSED -> RIGHT_CTRL_PRESSED, cause of AltGr
						r.Event.MouseEvent.dwControlKeyState |= RIGHT_ALT_PRESSED|RIGHT_CTRL_PRESSED;
					}
				}
			}
		}
	}
	UNREFERENCED_PARAMETER(lbNormalRBtnMode);

	if (messg == WM_MOUSEMOVE /*&& m_Mouse.bMouseButtonDown*/)
	{
		// Issue 172: проблема с правым кликом на PanelTabs
		//if (m_Mouse.crLastMouseEventPos.X == crMouse.X && m_Mouse.crLastMouseEventPos.Y == crMouse.Y)
		//	return; // не посылать в консоль MouseMove на том же месте
		//m_Mouse.crLastMouseEventPos.X = crMouse.X; m_Mouse.crLastMouseEventPos.Y = crMouse.Y;
		//// Проверять будем по пикселам, иначе AltIns начинает выделять со следующей позиции
		//int nDeltaX = (m_Mouse.ptLastMouseGuiPos.x > x) ? (m_Mouse.ptLastMouseGuiPos.x - x) : (x - m_Mouse.ptLastMouseGuiPos.x);
		//int nDeltaY = (m_Mouse.ptLastMouseGuiPos.y > y) ? (m_Mouse.ptLastMouseGuiPos.y - y) : (y - m_Mouse.ptLastMouseGuiPos.y);
		// Теперь - проверяем по координатам консоли, а не экрана.
		// Этого достаточно, AltIns не глючит, т.к. смена "типа события" (клик/движение) также отслеживается
		int nDeltaX = m_Mouse.rLastEvent.dwMousePosition.X - crMouse.X;
		int nDeltaY = m_Mouse.rLastEvent.dwMousePosition.Y - crMouse.Y;

		// Последний посланный m_Mouse.LastMouse запоминается в PostConsoleEvent
		if (m_Mouse.rLastEvent.dwEventFlags == MOUSE_MOVED // только если последним - был послан НЕ клик
				&& m_Mouse.rLastEvent.dwButtonState     == r.Event.MouseEvent.dwButtonState
				&& m_Mouse.rLastEvent.dwControlKeyState == r.Event.MouseEvent.dwControlKeyState
				//&& (nDeltaX <= 1 && nDeltaY <= 1) // был 1 пиксел
				&& !nDeltaX && !nDeltaY // стал 1 символ
				&& !abForceSend // и если не просили точно послать
				)
			return; // не посылать в консоль MouseMove на том же месте

		if (m_Mouse.bBtnClicked)
		{
			// Если после LBtnDown в ЭТУ же позицию не был послан MOUSE_MOVE - дослать в m_Mouse.crBtnClickPos
			if (m_Mouse.bMouseButtonDown && (m_Mouse.crBtnClickPos.X != crMouse.X || m_Mouse.crBtnClickPos.Y != crMouse.Y))
			{
				r.Event.MouseEvent.dwMousePosition = m_Mouse.crBtnClickPos;
				PostConsoleEvent(&r);
			}

			m_Mouse.bBtnClicked = FALSE;
		}

		//m_Mouse.ptLastMouseGuiPos.x = x; m_Mouse.ptLastMouseGuiPos.y = y;
		m_Mouse.crLastMouseEventPos.X = crMouse.X; m_Mouse.crLastMouseEventPos.Y = crMouse.Y;
	}

	// При БЫСТРОМ драге правой кнопкой мышки выделение в панели получается прерывистым. Исправим это.
	if (gpSet->isRSelFix)
	{
		// Имеет смысл только если в GUI сейчас показывается реальный буфер
		if (mp_ABuf != mp_RBuf)
		{
			mp_RBuf->SetRBtnDrag(FALSE);
		}
		else
		{
			//BOOL lbRBtnDrag = (r.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) == RIGHTMOST_BUTTON_PRESSED;
			COORD con_crRBtnDrag = {};
			BOOL con_bRBtnDrag = mp_RBuf->GetRBtnDrag(&con_crRBtnDrag);

			if (con_bRBtnDrag && !lbRBtnDrag)
			{
				con_bRBtnDrag = FALSE;
				mp_RBuf->SetRBtnDrag(FALSE);
			}
			else if (con_bRBtnDrag)
			{
				#ifdef _DEBUG
				SHORT nXDelta = crMouse.X - con_crRBtnDrag.X;
				#endif
				SHORT nYDelta = crMouse.Y - con_crRBtnDrag.Y;

				if (nYDelta < -1 || nYDelta > 1)
				{
					// Если после предыдущего драга прошло более 1 строки
					SHORT nYstep = (nYDelta < -1) ? -1 : 1;
					SHORT nYend = crMouse.Y; // - nYstep;
					crMouse.Y = con_crRBtnDrag.Y + nYstep;

					// досылаем пропущенные строки
					while (crMouse.Y != nYend)
					{
						#ifdef _DEBUG
						wchar_t szDbg[60]; swprintf_c(szDbg, L"+++ Add right button drag: {%ix%i}\n", crMouse.X, crMouse.Y);
						DEBUGSTRINPUT(szDbg);
						#endif

						r.Event.MouseEvent.dwMousePosition = crMouse;
						PostConsoleEvent(&r);
						crMouse.Y += nYstep;
					}
				}
			}

			if (lbRBtnDrag)
			{
				mp_RBuf->SetRBtnDrag(TRUE, &crMouse);
			}
		}
	}

	r.Event.MouseEvent.dwMousePosition = crMouse;

	if (mn_FarPID && mn_FarPID != mn_LastSetForegroundPID)
	{
		AllowSetForegroundWindow(mn_FarPID);
		mn_LastSetForegroundPID = mn_FarPID;
	}

	// Посылаем событие в консоль через ConEmuC
	PostConsoleEvent(&r);
}

void CRealConsole::StartSelection(bool abTextMode, SHORT anX/*=-1*/, SHORT anY/*=-1*/, bool abByMouse/*=FALSE*/, DWORD anAnchorFlag/*=0*/)
{
	mp_ABuf->StartSelection(abTextMode, anX, anY, abByMouse, 0, nullptr, anAnchorFlag);
}

void CRealConsole::ChangeSelectionByKey(UINT vkKey, bool bCtrl, bool bShift)
{
	mp_ABuf->ChangeSelectionByKey(vkKey, bCtrl, bShift);
}

void CRealConsole::ExpandSelection(SHORT anX, SHORT anY)
{
	mp_ABuf->ExpandSelection(anX, anY, mp_ABuf->isSelectionPresent());
}

void CRealConsole::DoSelectionFinalize()
{
	const bool do_copy = (isMouseSelectionPresent() && gpSet->isCTSAutoCopy);
	mp_ABuf->DoSelectionFinalize(do_copy);
}

void CRealConsole::OnSelectionChanged()
{
	// Show current selection state in the Status bar
	CEStr szSelInfo;
	CONSOLE_SELECTION_INFO sel = {};
	if (mp_ABuf->GetConsoleSelectionInfo(&sel))
	{
		#ifdef _DEBUG
		static CONSOLE_SELECTION_INFO old_sel = {};
		bool bChanged = false; static int iTheSame = 0;
		if (memcmp(&old_sel, &sel, sizeof(sel)) != 0)
		{
			old_sel = sel; iTheSame = 0;
		}
		else
		{
			iTheSame++;
		}
		#endif

		if (sel.dwFlags & CONSOLE_MOUSE_SELECTION)
			m_Mouse.bWasMouseSelection = true;

		const bool bStreamMode = ((sel.dwFlags & CONSOLE_TEXT_SELECTION) != 0);
		const int  nCellsCount = mp_ABuf->GetSelectionCellsCount();

		wchar_t szCoords[128] = L"", szChars[20];
		swprintf_c(szCoords, L"{%i,%i}-{%i,%i}:{%i,%i}",
			sel.srSelection.Left+1, sel.srSelection.Top+1,
			sel.srSelection.Right+1, sel.srSelection.Bottom+1,
			sel.dwSelectionAnchor.X+1, sel.dwSelectionAnchor.Y+1);
		szSelInfo = CEStr(
			ltow_s(nCellsCount, szChars, 10),
			CLngRc::getRsrc(lng_SelChars/*" chars "*/),
			szCoords,
			bStreamMode
				? CLngRc::getRsrc(lng_SelStream/*" stream selection"*/)
				: CLngRc::getRsrc(lng_SelBlock/*" block selection"*/)
			);
	}
	SetConStatus(szSelInfo);
}

bool CRealConsole::DoSelectionCopy(CECopyMode CopyMode /*= cm_CopySel*/, BYTE nFormat /*= CTSFormatDefault*/ /* use gpSet->isCTSHtmlFormat */, LPCWSTR pszDstFile /*= nullptr*/)
{
	bool bCopyRc = false;
	bool bReturnPrimary = false;
	CRealBuffer* pBuf = mp_ABuf;

	if (nFormat == CTSFormatDefault)
		nFormat = gpSet->isCTSHtmlFormat;

	if (CopyMode == cm_CopyAll)
	{
		if (pBuf->m_Type == rbt_Primary)
		{
			COORD crEnd = {0,0};
			mp_ABuf->GetCursorInfo(&crEnd, nullptr);
			crEnd.X = mp_ABuf->GetBufferWidth()-1;
			//crEnd = mp_ABuf->ScreenToBuffer(crEnd);

			if (LoadAlternativeConsole(lam_FullBuffer) && (mp_ABuf->m_Type != rbt_Primary))
			{
				bReturnPrimary = true;
				pBuf = mp_ABuf;
				pBuf->m_Type = rbt_Selection;
				pBuf->StartSelection(TRUE, 0, 0);
				pBuf->ExpandSelection(crEnd.X, crEnd.Y, false);
			}
		}
		else if (pBuf->m_Type == rbt_DumpScreen)
		{
			COORD crEnd = {pBuf->GetBufferWidth() - 1, pBuf->GetBufferHeight() - 1};
			pBuf->StartSelection(TRUE, 0, 0);
			pBuf->ExpandSelection(crEnd.X, crEnd.Y, false);
		}
		else
		{
			MsgBox(L"Return to Primary buffer first!", MB_ICONEXCLAMATION);
			goto wrap;
		}
	}
	else if (CopyMode == cm_CopyVis)
	{
		CONSOLE_SCREEN_BUFFER_INFO sbi = {};
		pBuf = mp_ABuf;
		pBuf->ConsoleScreenBufferInfo(&sbi);
		// Copy the GUI viewport, which may be virtually scrolled away from conhost.
		sbi.srWindow.Top = pBuf->GetBufferPosY();
		sbi.srWindow.Bottom = sbi.srWindow.Top + pBuf->GetTextHeight() - 1;
		// Start Block mode for HTML (nFormat!=0), we need to "represent" screen contents
		// And Stream mode for plain text (nFormat==0)
		pBuf->StartSelection((nFormat==0), sbi.srWindow.Left, sbi.srWindow.Top);
		pBuf = mp_ABuf; // selection may switch to a frozen full-buffer snapshot
		pBuf->ExpandSelection(sbi.srWindow.Right, sbi.srWindow.Bottom, false);
	}

	bCopyRc = pBuf->DoSelectionCopy(CopyMode, nFormat, pszDstFile);
wrap:
	if (bReturnPrimary)
	{
		SetActiveBuffer(rbt_Primary);
	}
	return bCopyRc;
}

void CRealConsole::DoFindText(int nDirection)
{
	AssertThis();

	if (gpSet->FindOptions.bFreezeConsole)
	{
		if (mp_ABuf->m_Type == rbt_Primary) //-V637
		{
			if (LoadAlternativeConsole(lam_FullBuffer) && (mp_ABuf->m_Type != rbt_Primary))
			{
				mp_ABuf->m_Type = rbt_Find;
			}
		}
	}
	else
	{
		if (mp_ABuf && (mp_ABuf->m_Type == rbt_Find))
		{
			SetActiveBuffer(rbt_Primary);
		}
	}
	if (mp_ABuf)
	{
		mp_ABuf->MarkFindText(nDirection, gpSet->FindOptions.text, gpSet->FindOptions.bMatchCase, gpSet->FindOptions.bMatchWholeWords);
	}
}

void CRealConsole::DoEndFindText()
{
	AssertThis();

	if (mp_ABuf && (mp_ABuf->m_Type == rbt_Find))
	{
		// Issue 1911: Do not scroll out of found position after clicking in the console to allow select text there.
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		mp_ABuf->ConsoleScreenBufferInfo(&csbi);
		mp_RBuf->DoScrollBuffer(SB_THUMBPOSITION, csbi.srWindow.Top);

		SetActiveBuffer(rbt_Primary);

		OnSelectionChanged();
	}
}

bool CRealConsole::OpenConsoleEventPipe()
{
	if (mh_ConInputPipe && mh_ConInputPipe!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(mh_ConInputPipe); mh_ConInputPipe = nullptr;
	}

	TODO("Если пайп с таким именем не появится в течении 10 секунд (минуты?) - закрыть VirtualConsole показав ошибку");
	// Try to open a named pipe; wait for it, if necessary.
	int nSteps = 10;
	BOOL fSuccess;
	DWORD dwErr = 0, dwWait = 0;

	DEBUGTEST(DWORD dwTick0 = GetTickCount());

	while ((nSteps--) > 0)
	{
		mh_ConInputPipe = CreateFile(
		                      ms_ConEmuCInput_Pipe,// pipe name
		                      GENERIC_WRITE,
		                      0,              // no sharing
		                      nullptr,           // default security attributes
		                      OPEN_EXISTING,  // opens existing pipe
		                      0,              // default attributes
		                      nullptr);          // no template file

		// Break if the pipe handle is valid.
		if (mh_ConInputPipe != INVALID_HANDLE_VALUE)
		{
			// The pipe connected; change to message-read mode.
			DWORD dwMode = CE_PIPE_READMODE;
			fSuccess = SetNamedPipeHandleState(
			               mh_ConInputPipe,    // pipe handle
			               &dwMode,  // new pipe mode
			               nullptr,     // don't set maximum bytes
			               nullptr);    // don't set maximum time

			if (!fSuccess /*&& !gbIsWine*/)
			{
				DEBUGSTRINPUT(L" - FAILED!\n");
				dwErr = GetLastError();
				//SafeCloseHandle(mh_ConInputPipe);

				//if (!IsDebuggerPresent())
				if (!isConsoleClosing() && !gbIsWine)
					DisplayLastError(L"SetNamedPipeHandleState failed", dwErr);

				//return FALSE;
			}

			return TRUE;
		}

		// Exit if an error other than ERROR_PIPE_BUSY occurs.
		dwErr = GetLastError();

		if (dwErr != ERROR_PIPE_BUSY)
		{
			TODO("Подождать, пока появится пайп с таким именем, но только пока жив mh_MainSrv");
			if (!isServerAlive())
			{
				DEBUGSTRINPUT(L"ConEmuC was closed. OpenPipe FAILED!\n");
				return FALSE;
			}

			if (isConsoleClosing())
				break;

			continue;
			//DisplayLastError(L"Could not open pipe", dwErr);
			//return 0;
		}

		// All pipe instances are busy, so wait for 0.1 second.
		if (!WaitNamedPipe(ms_ConEmuCInput_Pipe, 100))
		{
			dwErr = GetLastError();

			if (!isServerAlive())
			{
				DEBUGSTRINPUT(L"ConEmuC was closed. OpenPipe FAILED!\n");
				return FALSE;
			}

			if (!isConsoleClosing())
			{
				DisplayLastError(L"WaitNamedPipe failed", dwErr);
			}

			return FALSE;
		}
	}

	if (mh_ConInputPipe == nullptr || mh_ConInputPipe == INVALID_HANDLE_VALUE)
	{
		// Не дождались появления пайпа. Возможно, ConEmuC еще не запустился
		//DEBUGSTRINPUT(L" - mh_ConInputPipe not found!\n");
		#ifdef _DEBUG
		DWORD dwTick1 = GetTickCount();
		struct ServerClosing sc1 = m_ServerClosing;
		#endif

		if (!isConsoleClosing())
		{
			#ifdef _DEBUG
			DWORD dwTick2 = GetTickCount();
			struct ServerClosing sc2 = m_ServerClosing;

			if (dwErr == WAIT_TIMEOUT)
			{
				TODO("Иногда трапится. Проверить m_ServerClosing.nServerPID. Может его выставлять при щелчке по крестику?");
				MyAssertTrap();
			}

			DWORD dwTick3 = GetTickCount();
			struct ServerClosing sc3 = m_ServerClosing;
			#endif

			int nLen = _tcslen(ms_ConEmuCInput_Pipe) + 128;
			wchar_t* pszErrMsg = (wchar_t*)malloc(nLen*sizeof(wchar_t));
			swprintf_c(pszErrMsg, nLen/*#SECURELEN*/, L"ConEmuCInput not found, ErrCode=0x%08X\n%s", dwErr, ms_ConEmuCInput_Pipe);
			//DisplayLastError(L"mh_ConInputPipe not found", dwErr);
			// Выполняем Post-ом, т.к. консоль может уже закрываться (по стеку сообщений)? А сервер еще не прочухал...
			mp_ConEmu->PostDisplayRConError(this, pszErrMsg);
		}

		return FALSE;
	}

	return FALSE;
}

bool CRealConsole::PostConsoleEventPipe(MSG64 *pMsg, size_t cchCount /*= 1*/)
{
	if (!pMsg || !cchCount)
	{
		_ASSERTE(pMsg && (cchCount>0));
		return false;
	}

	DWORD dwErr = 0; //, dwMode = 0;
	bool lbOk = false;
	BOOL fSuccess = FALSE;

	#ifdef _DEBUG
	if (gbInSendConEvent)
	{
		_ASSERTE(!gbInSendConEvent);
	}
	#endif

	// Пайп есть. Проверим, что ConEmuC жив
	if (!isServerAlive())
	{
		//DisplayLastError(L"ConEmuC was terminated");
		LogString("PostConsoleEventPipe skipped due to server not alive");
		return false;
	}

	TODO("Если пайп с таким именем не появится в течении 10 секунд (минуты?) - закрыть VirtualConsole показав ошибку");

	if (mh_ConInputPipe==nullptr || mh_ConInputPipe==INVALID_HANDLE_VALUE)
	{
		// Try to open a named pipe; wait for it, if necessary.
		if (!OpenConsoleEventPipe())
		{
			LogString("PostConsoleEventPipe skipped due to OpenConsoleEventPipe failed");
			return false;
		}
	}


	#ifdef _DEBUG
	switch (pMsg->msg[0].message)
	{
		case WM_KEYDOWN: case WM_SYSKEYDOWN:
			DEBUGSTRINPUTPIPE(L"ConEmu: Sending key down\n"); break;
		case WM_KEYUP: case WM_SYSKEYUP:
			DEBUGSTRINPUTPIPE(L"ConEmu: Sending key up\n"); break;
		default:
			DEBUGSTRINPUTPIPE(L"ConEmu: Sending input\n");
	}
	#endif

	gbInSendConEvent = TRUE;
	DWORD dwSize = sizeof(MSG64)+(cchCount-1)*sizeof(MSG64::MsgStr), dwWritten;
	//_ASSERTE(pMsg->cbSize==dwSize);
	pMsg->cbSize = dwSize;
	pMsg->nCount = cchCount;

	fSuccess = WriteFile(mh_ConInputPipe, pMsg, dwSize, &dwWritten, nullptr);

	if (fSuccess)
	{
		lbOk = true;
	}
	else
	{
		dwErr = GetLastError();

		if (!isConsoleClosing())
		{
			if (dwErr == ERROR_NO_DATA/*0x000000E8*//*The pipe is being closed.*/
				|| dwErr == ERROR_PIPE_NOT_CONNECTED/*No process is on the other end of the pipe.*/)
			{
				if (!isServerAlive())
					goto wrap;

				if (OpenConsoleEventPipe())
				{
					fSuccess = WriteFile(mh_ConInputPipe, pMsg, dwSize, &dwWritten, nullptr);

					if (fSuccess)
					{
						lbOk = true;
						goto wrap; // таки ОК
					}
				}
			}

			#ifdef _DEBUG
			//DisplayLastError(L"Can't send console event (pipe)", dwErr);
			wchar_t szDbg[128];
			swprintf_c(szDbg, L"Can't send console event (pipe)", dwErr);
			mp_ConEmu->DebugStep(szDbg);
			#endif
		}

		goto wrap;
	}

wrap:
	gbInSendConEvent = FALSE;

	return lbOk;
}

bool CRealConsole::PostConsoleMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	BOOL lbOk = FALSE;
	bool bNeedCmd = isAdministrator() || (m_Args.pszUserName != nullptr);

	// 120630 - добавил WM_CLOSE, иначе в сервере не успевает выставиться флаг gbInShutdown
	if (nMsg == WM_INPUTLANGCHANGE || nMsg == WM_INPUTLANGCHANGEREQUEST || nMsg == WM_CLOSE)
		bNeedCmd = true;

	#ifdef _DEBUG
	wchar_t szDbg[255];
	if (nMsg == WM_INPUTLANGCHANGE || nMsg == WM_INPUTLANGCHANGEREQUEST)
	{
		const wchar_t* pszMsgID = (nMsg == WM_INPUTLANGCHANGE) ? L"WM_INPUTLANGCHANGE" : L"WM_INPUTLANGCHANGEREQUEST";
		const wchar_t* pszVia = bNeedCmd ? L"CmdExecute" : L"PostThreadMessage";
		swprintf_c(szDbg, L"RealConsole: %s, CP:%i, HKL:0x%08I64X via %s\n",
		          pszMsgID, (DWORD)wParam, (unsigned __int64)(DWORD_PTR)lParam, pszVia);
		DEBUGSTRLANG(szDbg);
	}

	swprintf_c(szDbg,
		L"PostMessage(x%08X, x%04X"
		WIN3264TEST(L", x%08X",L", x%08X%08X")
		WIN3264TEST(L", x%08X",L", x%08X%08X")
		L")\n",
		LODWORD(hWnd), nMsg, WIN3264WSPRINT(wParam), WIN3264WSPRINT(lParam));
	DEBUGSTRSENDMSG(szDbg);
	#endif

	if (!bNeedCmd)
	{
		lbOk = POSTMESSAGE(hWnd/*hConWnd*/, nMsg, wParam, lParam, FALSE);
	}
	else
	{
		CESERVER_REQ in;
		ExecutePrepareCmd(&in, CECMD_POSTCONMSG, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_POSTMSG));
		// Собственно, аргументы
		in.Msg.bPost = TRUE;
		in.Msg.hWnd = hWnd;
		in.Msg.nMsg = nMsg;
		in.Msg.wParam = wParam;
		in.Msg.lParam = lParam;

		DWORD dwTickStart = timeGetTime();

		// сообщения рассылаем только через главный сервер. альтернативный (приложение) может висеть
		CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(true), &in, ghWnd);

		CSetPgDebug::debugLogCommand(&in, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

		if (pOut)
		{
			lbOk = TRUE;
			ExecuteFreeResult(pOut);
		}
	}

	return (lbOk != FALSE);
}

void CRealConsole::StopSignal()
{
	AssertThis();

	DEBUGSTRCON(L"CRealConsole::StopSignal()\n");

	LogString(L"CRealConsole::StopSignal()");

	if (mn_ProcessCount)
	{
		MSectionLock SPRC; SPRC.Lock(&csPRC, TRUE);
		m_Processes.clear();
		SPRC.Unlock();
		mn_ProcessCount = 0;
		mn_ProcessClientCount = 0;
	}

	mn_TermEventTick = GetTickCount();
	SetEvent(mh_TermEvent);

	if (mn_InRecreate == (DWORD)-1)
	{
		mn_InRecreate = 0;
		mb_ProcessRestarted = FALSE;
		m_Args.Detached = crb_Off;
	}

	if (!mn_InRecreate)
	{
		setGuiWnd(nullptr);

		// Чтобы при закрытии не было попытки активировать
		// другую вкладку ЭТОЙ консоли
		#ifdef _DEBUG
		if (IsSwitchFarWindowAllowed())
		{
			_ASSERTE(IsSwitchFarWindowAllowed() == false);
		}
		#endif

		// Очистка массива консолей и обновление вкладок
		CConEmuChild::ProcessVConClosed(mp_VCon);

		// Clear some vars
		mpsz_PostCreateMacro.Release();
	}
}

bool CRealConsole::StartStopTermMode(const DWORD pid, const TermModeCommand mode, const DWORD value)
{
	AssertThisRet(false);

	bool processed = true;
	// _ASSERTE(pid != 0); pid is 0 when it's caused by ConsoleMode change detection in server

	switch (mode)
	{
	case tmc_TerminalType:
		_ASSERTE(value == te_win32 || value == te_xterm || value == te_conpty);
		if (value == te_conpty && !m_ConPtyMap.IsValid())
		{
			m_ConPtyMap.InitName(ConPtyDataName, LODWORD(mp_VCon->GetView()));
			m_ConPtyMap.Open(false);
		}
		mb_ConPty = value == te_conpty;
		m_TrueColorerHeader.flushCounter = 0;
		StartStopXTerm(pid, (value != te_win32));
		mp_VCon->Invalidate();
		break;
	case tmc_MouseMode:
		StartStopXMouse(pid, static_cast<TermMouseMode>(value));
		break;
	case tmc_BracketedPaste:
		StartStopBracketedPaste(pid, (value != 0));
		break;
	case tmc_AppCursorKeys:
		StartStopAppCursorKeys(pid, (value != 0));
		break;
	case tmc_CursorShape:
		SetCursorShape(static_cast<TermCursorShapes>(value));
		break;
	case tmc_ConInMode:
		// Some console application (not hooked?) changes ConInMode flag ENABLE_VIRTUAL_TERMINAL_INPUT
		if ((GetTermType() == te_xterm) != ((value & ENABLE_VIRTUAL_TERMINAL_INPUT) == ENABLE_VIRTUAL_TERMINAL_INPUT))
		{
			_ASSERTEX(m_RootInfo.nPID == pid); // expected PID at the moment
			const DWORD nRootPID = m_RootInfo.nPID ? m_RootInfo.nPID : pid;
			const bool newXTerm = ((value & ENABLE_VIRTUAL_TERMINAL_INPUT) == ENABLE_VIRTUAL_TERMINAL_INPUT);
			StartStopXTerm(nRootPID, newXTerm);
			if (newXTerm)
			{
				StartStopAppCursorKeys(nRootPID, true);
			}
		}
		break;
	case tmc_Last:
	default:
		_ASSERTE(FALSE && "mode is not supported");
		processed = false;
	}

	return processed;
}

bool CRealConsole::GetConPtyGlyphs(int columns, int rows, std::vector<ConPtyGlyph>& glyphs, bool fullFrame)
{
	glyphs.clear();
	const auto* data = m_ConPtyMap.Ptr();
	if (!mb_ConPty.load()) return false;
	if (mp_ABuf->m_Type != rbt_Primary)
	{
		const int top = fullFrame ? 0 : mp_ABuf->GetBufferPosY();
		for (const auto& saved : m_FrozenConPtyGlyphs)
			if (saved.position.Y >= top && saved.position.Y < top + rows && saved.position.X + saved.width <= columns)
			{
				auto copy = saved; copy.position.Y -= top; glyphs.push_back(std::move(copy));
			}
		return true;
	}
	if (!data) return false;
	const int top = fullFrame ? 0 : mp_ABuf->GetBufferPosY();
	if (fullFrame) { columns = data->columns; rows = data->rows; }
	for (unsigned attempt = 0; attempt < 3; ++attempt)
	{
		glyphs.clear();
		const auto counter = data->annotation.flushCounter;
		if (data->annotation.locked || data->version != ConPtyDataVersion || data->columns != columns
			|| !data->columns || data->rows > ConPtyCellCapacity / data->columns || top < 0
			|| data->graphemeCount > ConPtyGraphemeCapacity || data->graphemeChars > ConPtyGraphemeChars) return false;
		for (int y = 0; y < rows && y + top < static_cast<int>(data->rows); ++y)
			for (int x = 0; x < columns; ++x)
			{
				const auto& cell = data->cells[(y + top) * columns + x];
				const unsigned id = cell.raw[6], width = cell.raw[5];
				if (!id || !width || width > 2 || x + width > columns) continue;
				const auto entry = std::lower_bound(data->graphemes, data->graphemes + data->graphemeCount, id,
					[](const ConPtyLinkEntry& item, DWORD value) { return item.id < value; });
				if (entry == data->graphemes + data->graphemeCount || entry->id != id || entry->length > 64
					|| entry->offset > data->graphemeChars || entry->length > data->graphemeChars - entry->offset) continue;
				glyphs.push_back({{static_cast<SHORT>(x), static_cast<SHORT>(y)}, width,
					std::wstring(data->graphemeText + entry->offset, entry->length)});
			}
		MemoryBarrier();
		if (!data->annotation.locked && counter == data->annotation.flushCounter) return true;
	}
	glyphs.clear();
	return false;
}

bool CRealConsole::GetColorData(AnnotationHeader& header, const AnnotationInfo*& cells, bool& absolute)
{
	absolute = false;
	cells = nullptr;
	if (mb_ConPty.load())
	{
		const auto* data = m_ConPtyMap.Ptr();
		if (!data || data->version != ConPtyDataVersion || data->annotation.locked
			|| !data->columns || !data->rows || data->rows > ConPtyCellCapacity / data->columns)
			return false;
		header = data->annotation;
		header.bufferSize = data->rows * data->columns;
		cells = data->cells;
		absolute = true;
		return true;
	}
	if (!m_TrueColorerMap.IsValid() || !mp_TrueColorerData) return false;
	header = *m_TrueColorerMap.Ptr();
	cells = mp_TrueColorerData;
	return !header.locked;
}

bool CRealConsole::GetConPtyLink(COORD position, CEStr& uri, COORD& first, COORD& last)
{
	std::wstring target;
	if (!mb_ConPty.load() || !ResolveConPtyLink(m_ConPtyMap.Ptr(), position, target, first, last))
		return false;
	uri.Set(target.c_str());
	return true;
}

bool CRealConsole::UseTrueColor() const
{
	return gpSet->isTrueColorer || mb_ConPty.load();
}

bool CRealConsole::StartStopTermMode(TermModeCommand mode, ChangeTermAction action)
{
	AssertThisRet(false);

	const DWORD nActivePID = GetActivePID();
	DWORD newValue = 0;

	switch (mode)
	{
	case tmc_TerminalType:
		if (action == cta_Switch)
			newValue = (GetTermType() == te_win32) ? te_xterm : te_win32;
		else if (action == cta_Enable)
			newValue = te_xterm;
		else
			newValue = te_win32;
		break;
	case tmc_AppCursorKeys:
		if (action == cta_Switch)
			newValue = GetAppCursorKeys() ? FALSE : TRUE;
		else
			newValue = (action == cta_Enable);
		break;
	case tmc_BracketedPaste:
		if (action == cta_Switch)
			newValue = GetBracketedPaste() ? FALSE : TRUE;
		else
			newValue = (action == cta_Enable);
		break;
	default:
		return false;
	}

	return StartStopTermMode(nActivePID, mode, newValue);
}

void CRealConsole::StartStopXTerm(const DWORD nPID, const bool xTerm)
{
	wchar_t szInfo[100];
	if (gpSet->isLogging())
	{
		swprintf_c(szInfo, L"StartStopXTerm(nPID=%u, xTerm=%u)", nPID, static_cast<UINT>(xTerm));
		LogString(szInfo);
	}
	else
	{
		DEBUG_XTERM(msprintf(szInfo, countof(szInfo), L"XTerm: %s: %s pid=%u", ConEmu_EXE_3264, xTerm ? L"On" : L"Off", nPID));
	}

	if (!nPID || !xTerm)
	{
		m_Term = {};
		if (mp_XTerm)
			mp_XTerm->AppCursorKeys = false;
	}
	else
	{
		_ASSERTE(xTerm);
		m_Term.Term = te_xterm;
	}

	if (isActive(false) && mp_ConEmu->mp_Status)
		mp_ConEmu->mp_Status->UpdateStatusBar(true);

	if (ghOpWnd && isActive(false))
		gpSetCls->UpdateConsoleMode(this);
}

void CRealConsole::StartStopXMouse(DWORD nPID, TermMouseMode MouseMode)
{
	if (gpSet->isLogging())
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"StartStopXMouse(nPID=%u, Mode=0x%02X)", nPID, (DWORD)MouseMode);
		LogString(szInfo);
	}

	m_Term.nMouseMode = MouseMode;

	if (isActive(false) && mp_ConEmu->mp_Status)
		mp_ConEmu->mp_Status->UpdateStatusBar(true);

	if (ghOpWnd && isActive(false))
		gpSetCls->UpdateConsoleMode(this);
}

void CRealConsole::StartStopBracketedPaste(DWORD nPID, bool bUseBracketedPaste)
{
	if (gpSet->isLogging())
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"StartStopBracketedPaste(nPID=%u, Enabled=%s)", nPID, bUseBracketedPaste ? L"YES" : L"NO");
		LogString(szInfo);
	}

	m_Term.bBracketedPaste = bUseBracketedPaste;

	if (isActive(false) && mp_ConEmu->mp_Status)
		mp_ConEmu->mp_Status->UpdateStatusBar(true);

	if (ghOpWnd && isActive(false))
		gpSetCls->UpdateConsoleMode(this);
}

// Generally for XTerm emulation: ESC [ ? 1 h/l
void CRealConsole::StartStopAppCursorKeys(DWORD nPID, bool bAppCursorKeys)
{
	if (gpSet->isLogging())
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"StartStopAppCursorKeys(nPID=%u, Enabled=%s)", nPID, bAppCursorKeys ? L"YES" : L"NO");
		LogString(szInfo);
	}

	if (!mp_XTerm)
	{
		inew(mp_XTerm, new TermX);
	}
	_ASSERTE(mp_XTerm != nullptr);

	mp_XTerm->AppCursorKeys = bAppCursorKeys;

	if (isActive(false) && mp_ConEmu->mp_Status)
		mp_ConEmu->mp_Status->UpdateStatusBar(true);

	if (ghOpWnd && isActive(false))
		gpSetCls->UpdateConsoleMode(this);
}

bool CRealConsole::GetBracketedPaste()
{
	return (this && (m_Term.bBracketedPaste != FALSE));
}

bool CRealConsole::GetAppCursorKeys()
{
	return (this && mp_XTerm && mp_XTerm->AppCursorKeys);
}

void CRealConsole::PortableStarted(CESERVER_REQ_PORTABLESTARTED* pStarted)
{
	_ASSERTE(pStarted->hProcess == nullptr && pStarted->nPID);
	if (gpSet->isLogging())
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"PortableStarted(nPID=%u, Subsystem=%u)", pStarted->nPID, pStarted->nSubsystem);
		LogString(szInfo);
	}

	_ASSERTE(pStarted->hProcess == nullptr); // Not valid for ConEmu process
	m_ChildGui.paf = *pStarted;
	m_ChildGui.paf.hProcess = nullptr;

	// gh-681: NodeJSPortable.exe just runs "Server.cmd"
	if (pStarted->nSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI || pStarted->nSubsystem == IMAGE_SUBSYSTEM_BATCH_FILE)
	{
		if (!m_ChildGui.hGuiWnd || !IsWindow(m_ChildGui.hGuiWnd))
		{
			m_ChildGui.bChildConAttached = true;
			ShowWindow(GetView(), SW_SHOW);
			mp_VCon->Invalidate();
		}
	}
}

void CRealConsole::StopThread(bool abRecreating)
{
#ifdef _DEBUG
	/*
		HeapValidate(mh_Heap, 0, nullptr);
	*/
#endif
	_ASSERTE(abRecreating==mb_ProcessRestarted);
	DEBUGSTRPROC(L"Entering StopThread\n");

	// выставление флагов и завершение потока
	if (mh_MonitorThread)
	{
		// Сначала выставить флаги закрытия
		StopSignal(); //SetEvent(mh_TermEvent);

		// А теперь можно ждать завершения
		if (WaitForSingleObject(mh_MonitorThread, 30000) != WAIT_OBJECT_0)
		{
			// А это чтобы не осталось висеть окно ConEmu, раз всё уже закрыто
			mp_ConEmu->OnRConStartedSuccess(nullptr);
			LogString(L"### Main Thread waiting timeout, terminating...\n");
			DEBUGSTRPROC(L"### Main Thread waiting timeout, terminating...\n");
			_ASSERTE(FALSE && "Terminating mh_MonitorThread");
			mb_WasForceTerminated = TRUE;
			apiTerminateThread(mh_MonitorThread, 1);
		}
		else
		{
			DEBUGSTRPROC(L"Main Thread closed normally\n");
		}

		SafeCloseHandle(mh_MonitorThread);
	}

	if (mh_PostMacroThread != nullptr)
	{
		DWORD nWait = WaitForSingleObject(mh_PostMacroThread, 0);
		if (nWait == WAIT_OBJECT_0)
		{
			CloseHandle(mh_PostMacroThread);
			mh_PostMacroThread = nullptr;
		}
		else
		{
			// Должен быть nullptr, если нет - значит завис предыдущий макрос
			_ASSERTE(mh_PostMacroThread==nullptr && "Terminating mh_PostMacroThread");
			apiTerminateThread(mh_PostMacroThread, 100);
			CloseHandle(mh_PostMacroThread);
		}
	}


	// Завершение серверных нитей этой консоли
	DEBUGSTRPROC(L"About to terminate main server thread (MonitorThread)\n");

	if (ms_VConServer_Pipe[0])  // значит хотя бы одна нить была запущена
	{
		StopSignal(); // уже должен быть выставлен, но на всякий случай


		// Закрыть серверные потоки (пайпы)
		m_RConServer.Stop();


		ms_VConServer_Pipe[0] = 0;
	}

	if (!abRecreating)
	{
		SafeCloseHandle(mh_TermEvent);
		mn_TermEventTick = -1;
		SafeCloseHandle(mh_MonitorThreadEvent);
	}

	if (abRecreating)
	{
		hConWnd = nullptr;

		// Servers
		_ASSERTE((mn_AltSrv_PID==0) && "AltServer was not terminated?");
		//SafeCloseHandle(mh_AltSrv);
		SetAltSrvPID(0/*, nullptr*/);
		//mn_AltSrv_PID = 0;
		SafeCloseHandle(mh_MainSrv);
		SetMainSrvPID(0, nullptr);
		//mn_MainSrv_PID = 0;

		SetFarPID(0);
		SetFarPluginPID(0);
		SetActivePID(nullptr);
		SetAppDistinctPID(nullptr);

		mn_LastSetForegroundPID = 0;
		SafeCloseHandle(mh_ConInputPipe);
		m_ConDataChanged.Close();
		m_GetDataPipe.Close();
		// Имя пайпа для управления ConEmuC
		ms_ConEmuC_Pipe[0] = 0;
		ms_MainSrv_Pipe[0] = 0;
		ms_ConEmuCInput_Pipe[0] = 0;
		// Закрыть все мэппинги
		CloseMapHeader();
		CloseColorMapping();
		mp_VCon->Invalidate();
	}

#ifdef _DEBUG
	/*
		HeapValidate(mh_Heap, 0, nullptr);
	*/
#endif
	DEBUGSTRPROC(L"Leaving StopThread\n");
}

bool CRealConsole::InScroll()
{
	AssertThisRet(false);
	if (!mp_VCon || !isBufferHeight())
		return false;

	return mp_VCon->InScroll();
}

bool CRealConsole::isGuiVisible()
{
	AssertThisRet(false);

	if (m_ChildGui.isGuiWnd())
	{
		// IsWindowVisible не подходит, т.к. учитывает видимость и mh_WndDC
		DWORD_PTR nStyle = GetWindowLongPtr(m_ChildGui.hGuiWnd, GWL_STYLE);
		if ((nStyle & WS_VISIBLE) != 0)
		{
			if (m_ChildGui.bCreateHidden)
				m_ChildGui.bCreateHidden = false;
			return true;
		}
		// Avoid ScrollBar force show during ChildGui startup
		if (m_ChildGui.bCreateHidden)
			return true;
	}
	return false;
}

bool CRealConsole::isGuiOverCon()
{
	AssertThisRet(false);

	if (m_ChildGui.hGuiWnd && !m_ChildGui.bGuiExternMode)
	{
		return isGuiVisible();
	}

	return false;
}

// Проверить, включен ли буфер (TRUE). Или высота окна равна высоте буфера (FALSE).
bool CRealConsole::isBufferHeight()
{
	AssertThisRet(false);

	if (m_ChildGui.hGuiWnd)
	{
		return !isGuiVisible();
	}

	return (mp_ABuf->isScroll() != 0);
}

// TRUE - если консоль "заморожена" (альтернативный буфер)
bool CRealConsole::isAlternative()
{
	AssertThisRet(false);

	if (m_ChildGui.hGuiWnd)
		return false;

	return (mp_ABuf && (mp_ABuf != mp_RBuf));
}

bool CRealConsole::isConSelectMode()
{
	AssertThisRet(false);
	if (!mp_ABuf)
	{
		return false;
	}

	return mp_ABuf->isConSelectMode();
}

bool CRealConsole::isDetached()
{
	if (this == nullptr)
	{
		return FALSE;
	}

	if ((m_Args.Detached != crb_On) && !mn_InRecreate)
	{
		return FALSE;
	}

	// Флажок ВООБЩЕ не сбрасываем - ориентируемся на хэндлы
	//_ASSERTE(!mb_Detached || (mb_Detached && (hConWnd==nullptr)));
	return (mh_MainSrv == nullptr);
}

bool CRealConsole::isWindowVisible()
{
	AssertThisRet(false);

	if (!hConWnd)
		return false;

	if (!::IsWindowVisible(hConWnd))
		return false;
	return true;
}

LPCTSTR CRealConsole::GetTitle(bool abGetRenamed/*=false*/)
{
	AssertThisRet(nullptr);

	// На старте mn_ProcessCount==0, а кнопку в тулбаре показывать уже нужно

	// Здесь нужно возвращать "переименованным" активный таб, а не только первый
	// Пока abGetRenamed используется только в ConfirmCloseConsoles и CTaskBarGhost::CheckTitle
	if (abGetRenamed && (tabs.nActiveType & fwt_Renamed))
	{
		CTab tab(__FILE__,__LINE__);
		if (GetTab(tabs.nActiveIndex, tab) && (tab->Flags() & fwt_Renamed))
		{
			lstrcpyn(TempTitleRenamed, tab->Renamed.Ptr(), countof(TempTitleRenamed));
			if (*TempTitleRenamed)
			{
				return TempTitleRenamed;
			}
		}
	}

	if (isAdministrator() && gpSet->szAdminTitleSuffix[0])
	{
		if (TitleAdmin[0] == 0)
		{
			TitleAdmin[countof(TitleAdmin)-1] = 0;
			wcscpy_c(TitleAdmin, TitleFull);
			StripWords(TitleAdmin, gpSet->pszTabSkipWords);
			wcscat_c(TitleAdmin, gpSet->szAdminTitleSuffix);
		}
		return TitleAdmin;
	}

	return TitleFull;
}

// В отличие от GetTitle, который возвращает заголовок "вообще", в том числе и для редакторов/вьюверов
// эта функция возвращает заголовок панелей фара, которые сейчас могут быть не активны (т.е. это будет не Title)
LPCWSTR CRealConsole::GetPanelTitle()
{
	if (isFar() && ms_PanelTitle[0])  // скорее всего - это Panels?
		return ms_PanelTitle;
	return TitleFull;
}

LPCWSTR CRealConsole::GetTabTitle(CTab& tab)
{
	LPCWSTR pszName = tab->GetName();

	if (!pszName || !*pszName)
	{
		if (tab->Type() == fwt_Panels)
		{
			pszName = GetPanelTitle();
		}
		// If still not retrieved - show dummy title "ConEmu"
		if (!pszName || !*pszName)
		{
			pszName = mp_ConEmu->GetDefaultTabLabel();
		}
	}

	if (!pszName)
	{
		_ASSERTE(pszName!=nullptr);
		pszName = L"???";
	}

	return pszName;
}

void CRealConsole::ResetTopLeft()
{
	AssertThis();
	if (!mp_RBuf)
		return;
	mp_RBuf->ResetTopLeft();
}

// nDirection is one of the standard SB_xxx constants
LRESULT CRealConsole::DoScroll(int nDirection, UINT nCount /*= 1*/)
{
	AssertThisRet(0);
	if (!mp_ABuf)
		return 0;

	LRESULT lRc = 0;
	short nTrackPos = -1;
	CONSOLE_SCREEN_BUFFER_INFO sbi = {};
	SMALL_RECT srRealWindow = {};
	int nVisible = 0;
	bool bOnlyVirtual = gpSet->isFarWheelScrollback && isFar() && isFarBufferSupported();

	mp_ABuf->ConsoleScreenBufferInfo(&sbi, &srRealWindow);

	switch (nDirection)
	{
	case SB_HALFPAGEUP:
	case SB_HALFPAGEDOWN:
		nCount = MulDiv(nCount, TextHeight(), 2);
		nDirection -= SB_HALFPAGEUP;
		_ASSERTE(nDirection==SB_LINEUP || nDirection==SB_LINEDOWN);
		_ASSERTE(SB_LINEUP==(SB_HALFPAGEUP-SB_HALFPAGEUP) && SB_LINEDOWN==(SB_HALFPAGEDOWN-SB_HALFPAGEUP));
		break;
	case SB_THUMBTRACK:
	case SB_THUMBPOSITION:
		nTrackPos = nCount;
		nCount = 1;
		break;
	case SB_LINEDOWN:
	case SB_LINEUP:
		nCount = 1;
		break;
	case SB_PAGEDOWN:
	case SB_PAGEUP:
		nCount = TextHeight();
		nDirection -= SB_PAGEUP;
		_ASSERTE(nDirection==SB_LINEUP || nDirection==SB_LINEDOWN);
		break;
	case SB_TOP:
		nTrackPos = 0;
		nCount = 1;
		break;
	case SB_BOTTOM:
		nTrackPos = sbi.dwSize.Y - TextHeight() + 1;
		nCount = 1;
		break;
	case SB_GOTOCURSOR:
		nVisible = mp_ABuf->GetTextHeight();
		nDirection = SB_THUMBPOSITION;
		// Курсор выше видимой области?
		if ((sbi.dwCursorPosition.Y < sbi.srWindow.Top)
			// Курсор ниже видимой области?
			|| (sbi.dwCursorPosition.Y > sbi.srWindow.Bottom))
		{
			// Видимая область GUI отличается от области консоли?
			if ((mp_ABuf->m_Type == rbt_Primary)
				&& CoordInSmallRect(sbi.dwCursorPosition, srRealWindow))
			{
				// Просто сбросить
				mp_ABuf->ResetTopLeft();
				goto wrap;
			}
			// Let it set to one from the bottom
			nTrackPos = std::max(0,sbi.dwCursorPosition.Y-nVisible+2);
			nCount = 1;
			// it would be better to scroll console physically, if the cursor
			// is visible in the current region, so when user types new command
			// they would not be surprised by unexpected scroll to the "real" viewport
			// bOnlyVirtual = true;
		}
		else
		{
			// Он и так видим, но сбросим TopLeft
			mp_ABuf->ResetTopLeft();
			goto wrap;
		}
		break;
	case SB_PROMPTUP:
	case SB_PROMPTDOWN:
		if (GetServerPID())
		{
			bool bFound = false;
			CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_FINDNEXTROWID, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)*2);
			if (pIn)
			{
				pIn->dwData[0] = mp_ABuf->GetBufferPosY();
				pIn->dwData[1] = (nDirection == SB_PROMPTUP);
				CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), pIn, ghWnd);
				if (pOut && (pOut->DataSize() >= sizeof(DWORD)*2))
				{
					if (pOut->dwData[1] != DWORD(-1))
					{
						nDirection = SB_THUMBPOSITION;
						nTrackPos = pOut->dwData[0];
						nCount = 1;
						bFound = true;
					}
				}
				ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
			if (!bFound)
				goto wrap;
		}
		break;
	case SB_ENDSCROLL:
		goto wrap;
	}

	lRc = mp_ABuf->DoScrollBuffer(nDirection, nTrackPos, nCount, bOnlyVirtual);
wrap:
	return lRc;
}

const ConEmuHotKey* CRealConsole::ProcessSelectionHotKey(const ConEmuChord& VkState, bool bKeyDown, const wchar_t *pszChars)
{
	if (!isSelectionPresent())
		return nullptr;

	return mp_ABuf->ProcessSelectionHotKey(VkState, bKeyDown, pszChars);
}

void CRealConsole::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars, const MSG* pDeadCharMsg)
{
	AssertThis();

	//LRESULT result = 0;
	_ASSERTE(pszChars!=nullptr);

	if ((messg == WM_CHAR) || (messg == WM_SYSCHAR) || (messg == WM_IME_CHAR))
	{
		_ASSERTE((messg != WM_CHAR) && (messg != WM_SYSCHAR) && (messg != WM_IME_CHAR));
	}
	else
	{
		if ((wParam == VK_KANJI) || (wParam == VK_PROCESSKEY))
		{
			// Don't send to real console
			return;
		}
		// Dead key? debug
		_ASSERTE(wParam != VK_PACKET);
	}

	#ifdef _DEBUG
	if (wParam != VK_LCONTROL && wParam != VK_RCONTROL && wParam != VK_CONTROL &&
	        wParam != VK_LSHIFT && wParam != VK_RSHIFT && wParam != VK_SHIFT &&
	        wParam != VK_LMENU && wParam != VK_RMENU && wParam != VK_MENU &&
	        wParam != VK_LWIN && wParam != VK_RWIN)
	{
		wParam = wParam;
	}

	// (wParam == 'C' || wParam == VK_PAUSE/*Break*/)
	// Ctrl+C & Ctrl+Break are very special signals in Windows
	// After many tries I decided to send 'C'/Break to console window
	// via simple `SendMessage(ghConWnd, WM_KEYDOWN, r.Event.KeyEvent.wVirtualKeyCode, 0)`
	// That is done inside ../ConEmuCD/Queue.cpp::ProcessInputMessage
	// Some restriction applies to it also, look inside `ProcessInputMessage`
	//
	// Yep, there is another nice function - GenerateConsoleCtrlEvent
	// But it has different restrictions and it doesn't break ReadConsole
	// because that function loops deep inside Windows kernel...
	// However, user can call it via GuiMacro:
	// Break() for Ctrl+C or Break(1) for Ctrl+Break
	// if simple Keys("^c") does not work in your case.

	if (wParam == VK_CONTROL || wParam == VK_LCONTROL || wParam == VK_RCONTROL || wParam == 'C')
	{
		if (messg == WM_KEYDOWN || messg == WM_KEYUP /*|| messg == WM_CHAR*/)
		{
			wchar_t szDbg[128];

			if (messg == WM_KEYDOWN)
				swprintf_c(szDbg, L"WM_KEYDOWN(%i,0x%08X)\n", (DWORD)wParam, (DWORD)lParam);
			else //if (messg == WM_KEYUP)
				swprintf_c(szDbg, L"WM_KEYUP(%i,0x%08X)\n", (DWORD)wParam, (DWORD)lParam);

			DEBUGSTRINPUT(szDbg);
		}
	}
	#endif

	// Проверим, может клавишу обработает сам буфер (выделение текста кнопками, если оно уже начато и т.п.)?
	// Сами HotKeys буфер не обрабатывает
	if (mp_ABuf->OnKeyboard(hWnd, messg, wParam, lParam, pszChars))
	{
		return;
	}

	if (((wParam & 0xFF) >= VK_WHEEL_FIRST) && ((wParam & 0xFF) <= VK_WHEEL_LAST))
	{
		// Такие коды с клавиатуры приходить не должны, а то для "мышки" ничего не останется
		_ASSERTE(!(((wParam & 0xFF) >= VK_WHEEL_FIRST) && ((wParam & 0xFF) <= VK_WHEEL_LAST)));
		return;
	}

	// Hotkey не группируются
	// Иначе получается маразм, например, с Ctrl+Shift+O,
	// который по идее должен разбивать АКТИВНУЮ консоль,
	// но пытается разбить все видимые, в итоге срывает крышу
	if (mp_ConEmu->ProcessHotKeyMsg(messg, wParam, lParam, pszChars, this))
	{
		// Yes, Skip
		return;
	}

	// If user want to type simultaneously into visible or all consoles
	EnumVConFlags is_grouped = mp_VCon->isGroupedInput();
	if (is_grouped)
	{
		KeyboardIntArg args = {hWnd, messg, wParam, lParam, pszChars, pDeadCharMsg};
		CVConGroup::EnumVCon(is_grouped, OnKeyboardBackCall, (LPARAM)&args);
		return; // Done
	}

	OnKeyboardInt(hWnd, messg, wParam, lParam, pszChars, pDeadCharMsg);
}

bool CRealConsole::OnKeyboardBackCall(CVirtualConsole* pVCon, LPARAM lParam)
{
	KeyboardIntArg* p = (KeyboardIntArg*)lParam;
	pVCon->RCon()->OnKeyboardInt(p->hWnd, p->messg, p->wParam, p->lParam, p->pszChars, p->pDeadCharMsg);
	return true;
}

void CRealConsole::OnKeyboardInt(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars, const MSG* pDeadCharMsg)
{
	// Основная обработка
	{
		if (wParam == VK_MENU && (messg == WM_KEYUP || messg == WM_SYSKEYUP) && gpSet->isFixAltOnAltTab)
		{
			// При быстром нажатии Alt-Tab (переключение в другое окно)
			// в консоль проваливается {press Alt/release Alt}
			// В результате, может выполниться макрос, повешенный на Alt.
			if (getForegroundWindow()!=ghWnd && GetFarPID())
			{
				if (/*isPressed(VK_MENU) &&*/ !isPressed(VK_CONTROL) && !isPressed(VK_SHIFT))
				{
					PostKeyPress(VK_CONTROL, LEFT_ALT_PRESSED, 0);
				}
			}
			// Continue!
		}

		#if 0
		if (wParam == 189)
		{
			UINT sc = LOBYTE(HIWORD(lParam));
			BYTE pressed[256] = {};
			BOOL b = GetKeyboardState(pressed);
			b = FALSE;
		}
		#endif

		// *** Do not send to console ***
		if (m_ChildGui.isGuiWnd())
		{
			switch (messg)
			{
			case WM_KEYDOWN:
			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
				// Issue 971: Sending dead chars to GUI child applications
				if (pDeadCharMsg)
				{
					if (messg==WM_KEYDOWN || messg==WM_SYSKEYDOWN)
					{
						mn_InPostDeadChar = messg;
						PostConsoleMessage(m_ChildGui.hGuiWnd, pDeadCharMsg->message, pDeadCharMsg->wParam, pDeadCharMsg->lParam);
					}
					else
					{
						//mn_InPostDeadChar = FALSE;
					}
				}
				else if (mn_InPostDeadChar && (pszChars && *pszChars))
				{
					_ASSERTE(pszChars[0] && (pszChars[1]==0 || (pszChars[1]==pszChars[0] && pszChars[2]==0)));
					for (size_t i = 0; pszChars[i]; i++)
					{
						PostConsoleMessage(m_ChildGui.hGuiWnd, (mn_InPostDeadChar==WM_KEYDOWN) ? WM_CHAR : WM_SYSCHAR, (WPARAM)pszChars[i], lParam);
					}
					mn_InPostDeadChar = FALSE;
				}
				else
				{
					if (mn_InPostDeadChar)
					{
						if (messg==WM_KEYUP || messg==WM_SYSKEYUP)
						{
							break;
						}
						else
						{
							// Почему-то не сбросился mn_InPostDeadChar
							_ASSERTE(messg==WM_KEYUP || messg==WM_SYSKEYUP);
							mn_InPostDeadChar = FALSE;
						}
					}

					PostConsoleMessage(m_ChildGui.hGuiWnd, messg, wParam, lParam);
				}
				break;
			case WM_CHAR:
			case WM_SYSCHAR:
			case WM_DEADCHAR:
				// Не должны сюда доходить - обработка через WM_KEYDOWN и т.п.?
				_ASSERTE(messg!=WM_CHAR && messg!=WM_SYSCHAR && messg!=WM_DEADCHAR);
				break;
			default:
				PostConsoleMessage(m_ChildGui.hGuiWnd, messg, wParam, lParam);
			}
		}
		else
		{
			if (mp_ConEmu->isInImeComposition())
			{
				// Сейчас ввод работает на окно IME и не должен попадать в консоль!
				return;
			}

			// Завершение выделения по KeyPress?
			if (mp_ABuf->isSelectionPresent())
			{
				// буквы/цифры/символы/...
				if ((gpSet->isCTSEndOnTyping && (pszChars && *pszChars))
					// +все, что не генерит символы (стрелки, Fn, и т.п.)
					|| (gpSet->isCTSEndOnKeyPress
						&& (messg==WM_KEYDOWN || messg==WM_SYSKEYDOWN)
						&& !(wParam==VK_SHIFT || wParam==VK_CONTROL || wParam==VK_MENU || wParam==VK_LWIN || wParam==VK_RWIN || wParam==VK_APPS)
						&& !gpSet->IsModifierPressed(mp_ABuf->isStreamSelection() ? vkCTSVkText : vkCTSVkBlock, false)
					))
				{
					// 2 - end only, do not copy
					mp_ABuf->DoSelectionFinalize((gpSet->isCTSEndOnTyping == 1));
				}
				else
				{
					return; // В режиме выделения - в консоль ВООБЩЕ клавиатуру не посылать!
				}
			}


			if (GetActiveBufferType() != rbt_Primary)
			{
				// Пропускать кнопки в консоль только если буфер реальный
				return;
			}

			// Если видимый регион заблокирован, то имеет смысл его сбросить
			// если нажата не кнопка-модификатор?
			WORD vk = LOWORD(wParam);
			if ((messg != WM_KEYUP && messg != WM_SYSKEYUP)
				&& ((pszChars && *pszChars)
					|| (vk && vk != VK_LWIN && vk != VK_RWIN
						&& vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL
						&& vk != VK_MENU && vk != VK_LMENU && vk != VK_RMENU
						&& vk != VK_SHIFT && vk != VK_LSHIFT && vk != VK_RSHIFT
						&& !(vk >= VK_BROWSER_BACK/*0xA6*/ && vk < VK_OEM_1/*0xBA*/))))
			{
				mp_RBuf->OnKeysSending();
			}

			// А теперь собственно отсылка в консоль
			ProcessKeyboard(messg, wParam, lParam, pszChars);
		}
	}
	return;
}

TermEmulationType CRealConsole::GetTermType()
{
	_ASSERTE(te_win32 == (TermEmulationType)0);

	// xterm emulation?
	if (!m_Term.Term)
	{
		return te_win32;
	}

	if (m_Term.Term)
	{
		inew(mp_XTerm, new TermX);
	}

	return m_Term.Term;
}

bool CRealConsole::ProcessXtermSubst(const INPUT_RECORD& r)
{
	// xterm emulation?
	if (!GetTermType())
		return false;

	bool bProcessed = false;
	bool bSend = false;
	CEStr szSubstKeys;
	WORD nRepeatCount = 1;

	// Till now, this may be ‘te_xterm’ or ‘te_win32’ only
	_ASSERTE(m_Term.Term == te_xterm);
	_ASSERTE(mp_XTerm != nullptr);

	// Processed xterm keys?
	{
		switch (r.EventType)
		{
		case KEY_EVENT:
			// Key need to be translated?
			{
				bProcessed = mp_XTerm->GetSubstitute(r.Event.KeyEvent, szSubstKeys);
				// But only key presses are sent to terminal
				bSend = (bProcessed && r.Event.KeyEvent.bKeyDown && !szSubstKeys.IsEmpty());
				if (r.Event.KeyEvent.wRepeatCount)
					nRepeatCount = r.Event.KeyEvent.wRepeatCount;
			}
			break; // KEY_EVENT

		case MOUSE_EVENT:
			// Mouse event need to be translated?
			{
				INPUT_RECORD rc = r;
				// xTerm requires "visible" coordinates
				rc.Event.MouseEvent.dwMousePosition = BufferToScreen(r.Event.MouseEvent.dwMousePosition);
				TermMouseMode mode = m_Term.nMouseMode;
				if (!mode)
				{
					//LPCWSTR pszPrcName = GetActiveProcessName();
					//if (pszPrcName && CompareProcessNames(pszPrcName, L"vim"))
					//	mode = tmm_VIM;
					//else
					mode = tmm_SCROLL;
				}
				bProcessed = mp_XTerm->GetSubstitute(rc.Event.MouseEvent, mode, szSubstKeys);
				bSend = (bProcessed && !szSubstKeys.IsEmpty());
			}
			break; // MOUSE_EVENT
		}

		if (bSend)
		{
			for (WORD n = 0; n < nRepeatCount; ++n)
			{
				// Don't use group-input here - it's already processed by ProcessXtermSubst caller
				if (!PostString(szSubstKeys.ms_Val, szSubstKeys.GetLen(), PostStringFlags::XTermSequence))
					break;
			}
		}
	}

	return bProcessed;
}

// pszChars may be nullptr
void CRealConsole::ProcessKeyboard(UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars)
{
	INPUT_RECORD r = {KEY_EVENT};

	//WORD nCaps = 1 & (WORD)GetKeyState(VK_CAPITAL);
	//WORD nNum = 1 & (WORD)GetKeyState(VK_NUMLOCK);
	//WORD nScroll = 1 & (WORD)GetKeyState(VK_SCROLL);
	//WORD nLAlt = 0x8000 & (WORD)GetKeyState(VK_LMENU);
	//WORD nRAlt = 0x8000 & (WORD)GetKeyState(VK_RMENU);
	//WORD nLCtrl = 0x8000 & (WORD)GetKeyState(VK_LCONTROL);
	//WORD nRCtrl = 0x8000 & (WORD)GetKeyState(VK_RCONTROL);
	//WORD nShift = 0x8000 & (WORD)GetKeyState(VK_SHIFT);

	//if (messg == WM_CHAR || messg == WM_SYSCHAR) {
	//    if (((WCHAR)wParam) <= 32 || mn_LastVKeyPressed == 0)
	//        return; // это уже обработано
	//    r.Event.KeyEvent.bKeyDown = TRUE;
	//    r.Event.KeyEvent.uChar.UnicodeChar = (WCHAR)wParam;
	//    r.Event.KeyEvent.wRepeatCount = 1; TODO("0-15 ? Specifies the repeat count for the current message. The value is the number of times the keystroke is autorepeated as a result of the user holding down the key. If the keystroke is held long enough, multiple messages are sent. However, the repeat count is not cumulative.");
	//    r.Event.KeyEvent.wVirtualKeyCode = mn_LastVKeyPressed;
	//} else {

	mn_LastVKeyPressed = wParam & 0xFFFF;

	////PostConsoleMessage(hConWnd, messg, wParam, lParam, FALSE);
	//if ((wParam >= VK_F1 && wParam <= /*VK_F24*/ VK_SCROLL) || wParam <= 32 ||
	//    (wParam >= VK_LSHIFT/*0xA0*/ && wParam <= /*VK_RMENU=0xA5*/ 0xB7 /*=VK_LAUNCH_APP2*/) ||
	//    (wParam >= VK_LWIN/*0x5B*/ && wParam <= VK_APPS/*0x5D*/) ||
	//    /*(wParam >= VK_NUMPAD0 && wParam <= VK_DIVIDE) ||*/ //TODO:
	//    (wParam >= VK_PRIOR/*0x21*/ && wParam <= VK_HELP/*0x2F*/) ||
	//    nLCtrl || nRCtrl ||
	//    ((nLAlt || nRAlt) && !(nLCtrl || nRCtrl || nShift) && (wParam >= VK_NUMPAD0/*0x60*/ && wParam <= VK_NUMPAD9/*0x69*/)) || // Ввод Alt-цифры при включенном NumLock
	//    FALSE)
	//{

	TODO("0-15 ? Specifies the repeat count for the current message. The value is the number of times the keystroke is autorepeated as a result of the user holding down the key. If the keystroke is held long enough, multiple messages are sent. However, the repeat count is not cumulative.");
	r.Event.KeyEvent.wRepeatCount = 1;
	r.Event.KeyEvent.wVirtualKeyCode = mn_LastVKeyPressed;
	r.Event.KeyEvent.uChar.UnicodeChar = pszChars ? pszChars[0] : 0;

	//if (!nLCtrl && !nRCtrl) {
	//    if (wParam == VK_ESCAPE || wParam == VK_RETURN || wParam == VK_BACK || wParam == VK_TAB || wParam == VK_SPACE
	//        || FALSE)
	//        r.Event.KeyEvent.uChar.UnicodeChar = wParam;
	//}
	//    mn_LastVKeyPressed = 0; // чтобы не обрабатывать WM_(SYS)CHAR
	//} else {
	//    return;
	//}
	r.Event.KeyEvent.bKeyDown = (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN);
	//}
	r.Event.KeyEvent.wVirtualScanCode = ((DWORD)lParam & 0xFF0000) >> 16; // 16-23 - Specifies the scan code. The value depends on the OEM.
	// 24 - Specifies whether the key is an extended key, such as the right-hand ALT and CTRL keys that appear on an enhanced 101- or 102-key keyboard. The value is 1 if it is an extended key; otherwise, it is 0.
	// 29 - Specifies the context code. The value is 1 if the ALT key is held down while the key is pressed; otherwise, the value is 0.
	// 30 - Specifies the previous key state. The value is 1 if the key is down before the message is sent, or it is 0 if the key is up.
	// 31 - Specifies the transition state. The value is 1 if the key is being released, or it is 0 if the key is being pressed.

	r.Event.KeyEvent.dwControlKeyState = mp_ConEmu->GetControlKeyState(lParam);

	if (mn_LastVKeyPressed == VK_ESCAPE)
	{
		if ((gpSet->isMapShiftEscToEsc && (gpSet->isMultiMinByEsc == 1))
			&& ((r.Event.KeyEvent.dwControlKeyState & ALL_MODIFIERS) == SHIFT_PRESSED))
		{
			// When enabled feature "Minimize by Esc always"
			// we need easy way to send simple "Esc" to console
			// There is an option for this: Map Shift+Esc to Esc
			r.Event.KeyEvent.dwControlKeyState &= ~ALL_MODIFIERS;
		}
	}

	#ifdef _DEBUG
	if (r.EventType == KEY_EVENT && r.Event.KeyEvent.bKeyDown &&
	        r.Event.KeyEvent.wVirtualKeyCode == VK_F11)
	{
		DEBUGSTRINPUT(L"  ---  F11 sending\n");
	}
	#endif

	// -- заменено на перехват функции ScreenToClient
	//// Сделано (пока) только чтобы текстовое EMenu активировалось по центру консоли,
	//// а не в положении мыши (что смотрится отвратно - оно может сплющиться до 2-3 строк).
	//// Только при скрытой консоли.
	//RemoveFromCursor();

	if (mn_FarPID && mn_FarPID != mn_LastSetForegroundPID)
	{
		//DWORD dwFarPID = GetFarPID();
		//if (dwFarPID)
		AllowSetForegroundWindow(mn_FarPID);
		mn_LastSetForegroundPID = mn_FarPID;
	}

	// Эмуляция терминала?
	if (m_Term.Term && ProcessXtermSubst(r))
	{
		return;
	}

	PostConsoleEvent(&r);

	// нажатие клавиши может трансформироваться в последовательность нескольких символов...
	if (pszChars && pszChars[0] && pszChars[1])
	{
		/*
		The expected behaviour would be (as it is in a cmd.exe session):
		- hit "^" -> see nothing
		- hit "^" again -> see ^^
		- hit "^" again -> see nothing
		- hit "^" again -> see ^^

		Alternatively:
		- hit "^" -> see nothing
		- hit any other alpha-numeric key, e.g. "k" -> see "^k"
		*/
		for (int i = 1; pszChars[i]; i++)
		{
			r.Event.KeyEvent.uChar.UnicodeChar = pszChars[i];
			PostConsoleEvent(&r);
		}
	}
}

void CRealConsole::OnKeyboardIme(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (messg != WM_IME_CHAR)
		return;

	// Do not run excess ops when routing our msg to GUI application
	if (m_ChildGui.isGuiWnd())
	{
		PostConsoleMessage(m_ChildGui.hGuiWnd, messg, wParam, lParam);
		return;
	}


	INPUT_RECORD r = {KEY_EVENT};
	WORD nCaps = 1 & (WORD)GetKeyState(VK_CAPITAL);
	WORD nNum = 1 & (WORD)GetKeyState(VK_NUMLOCK);
	WORD nScroll = 1 & (WORD)GetKeyState(VK_SCROLL);
	WORD nLAlt = 0x8000 & (WORD)GetKeyState(VK_LMENU);
	WORD nRAlt = 0x8000 & (WORD)GetKeyState(VK_RMENU);
	WORD nLCtrl = 0x8000 & (WORD)GetKeyState(VK_LCONTROL);
	WORD nRCtrl = 0x8000 & (WORD)GetKeyState(VK_RCONTROL);
	WORD nShift = 0x8000 & (WORD)GetKeyState(VK_SHIFT);

	r.Event.KeyEvent.wRepeatCount = 1; // Repeat count. Since the first byte and second byte is continuous, this is always 1.
	// 130822 - Japanese+iPython - (wVirtualKeyCode!=0) fails with pyreadline
	r.Event.KeyEvent.wVirtualKeyCode = isFar() ? VK_PROCESSKEY : 0; // В RealConsole валится VK=0, но его фар игнорит
	r.Event.KeyEvent.uChar.UnicodeChar = (wchar_t)wParam;
	r.Event.KeyEvent.bKeyDown = TRUE; //(messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN);
	r.Event.KeyEvent.wVirtualScanCode = ((DWORD)lParam & 0xFF0000) >> 16; // 16-23 - Specifies the scan code. The value depends on the OEM.
	// 24 - Specifies whether the key is an extended key, such as the right-hand ALT and CTRL keys that appear on an enhanced 101- or 102-key keyboard. The value is 1 if it is an extended key; otherwise, it is 0.
	// 29 - Specifies the context code. The value is 1 if the ALT key is held down while the key is pressed; otherwise, the value is 0.
	// 30 - Specifies the previous key state. The value is 1 if the key is down before the message is sent, or it is 0 if the key is up.
	// 31 - Specifies the transition state. The value is 1 if the key is being released, or it is 0 if the key is being pressed.
	r.Event.KeyEvent.dwControlKeyState = 0;

	if (((DWORD)lParam & (DWORD)(1 << 24)) != 0)
		r.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;

	if ((nCaps & 1) == 1)
		r.Event.KeyEvent.dwControlKeyState |= CAPSLOCK_ON;

	if ((nNum & 1) == 1)
		r.Event.KeyEvent.dwControlKeyState |= NUMLOCK_ON;

	if ((nScroll & 1) == 1)
		r.Event.KeyEvent.dwControlKeyState |= SCROLLLOCK_ON;

	if (nLAlt & 0x8000)
		r.Event.KeyEvent.dwControlKeyState |= LEFT_ALT_PRESSED;

	if (nRAlt & 0x8000)
		r.Event.KeyEvent.dwControlKeyState |= RIGHT_ALT_PRESSED;

	if (nLCtrl & 0x8000)
		r.Event.KeyEvent.dwControlKeyState |= LEFT_CTRL_PRESSED;

	if (nRCtrl & 0x8000)
		r.Event.KeyEvent.dwControlKeyState |= RIGHT_CTRL_PRESSED;

	if (nShift & 0x8000)
		r.Event.KeyEvent.dwControlKeyState |= SHIFT_PRESSED;

	PostConsoleEvent(&r, true);
}

void CRealConsole::OnDosAppStartStop(enum StartStopType sst, DWORD anPID)
{
	if (sst == sst_App16Start)
	{
		DEBUGSTRPROC(L"16 bit application STARTED\n");

		if (mn_Comspec4Ntvdm == 0)
		{
			// mn_Comspec4Ntvdm может быть еще не заполнен, если 16бит вызван из батника
			WARNING("###: При запуске vc.com из cmd.exe - ntvdm.exe запускается в обход ConEmuC.exe, что нехорошо наверное");
			_ASSERTE(mn_Comspec4Ntvdm != 0 || !isFarInStack());
		}

		if (!(mn_ProgramStatus & CES_NTVDM))
		{
			LogString(L"NTVDM was detected (sst_App16Start), setting CES_NTVDM");
			mn_ProgramStatus |= CES_NTVDM;
		}

		// -- в cmdStartStop - mb_IgnoreCmdStop устанавливался всегда, поэтому условие убрал
		//if (gOSVer.dwMajorVersion>5 || (gOSVer.dwMajorVersion==5 && gOSVer.dwMinorVersion>=1))
		mb_IgnoreCmdStop = TRUE;

		SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
	}
	else if (sst == sst_App16Stop)
	{
		// This comes from CConEmuMain::WinEventProc(EVENT_CONSOLE_END_APPLICATION)
		DEBUGSTRPROC(L"16 bit application TERMINATED\n");

		// Не сбрасывать CES_NTVDM сразу (при mn_Comspec4Ntvdm!=0).
		// Еще не отработал возврат размера консоли!
		if (mn_Comspec4Ntvdm == 0)
		{
			LogString(L"NTVDM was stopped (sst_App16Stop), clearing CES_NTVDM");
			SetProgramStatus(CES_NTVDM, 0/*Flags*/);
		}
		else
		{
			LogString(L"NTVDM was stopped (sst_App16Stop), CES_NTVDM was left");
		}

		//2010-02-26 убрал. может прийти с задержкой и только создать проблемы
		//SyncConsole2Window(); // После выхода из 16bit режима хорошо бы отресайзить консоль по размеру GUI
		// хорошо бы проверить, что 16бит не осталось в других консолях
		if (!mp_ConEmu->isNtvdm(TRUE))
		{
			SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
		}
	}
}

// Это приходит из ConEmuC.exe::ServerInitGuiTab (CECMD_SRVSTARTSTOP)
// здесь сервер только "запускается" и еще не готов принимать команды
HWND CRealConsole::OnServerStarted(const HWND ahConWnd, const DWORD anServerPID, const DWORD dwKeybLayout, CESERVER_REQ_SRVSTARTSTOPRET& pRet)
{
	AssertThisRet(nullptr);
	if ((ahConWnd == nullptr) || (hConWnd && (ahConWnd != hConWnd)) || (anServerPID != mn_MainSrv_PID))
	{
		MBoxAssert(ahConWnd!=nullptr);
		MBoxAssert((hConWnd==nullptr) || (ahConWnd==hConWnd));
		MBoxAssert(anServerPID==mn_MainSrv_PID);
		return nullptr;
	}

	UpdateStartState(rss_ServerConnected);

	// Окошко консоли скорее всего еще не инициализировано
	if (hConWnd == nullptr)
	{
		SetConStatus(L"Waiting for console server...", cso_ResetOnConsoleReady|cso_Critical);
		SetHwnd(ahConWnd);
	}

	// Само разберется
	OnConsoleKeyboardLayout(dwKeybLayout);

	// Are there postponed macros? Like print(...) for example
	ProcessPostponedMacro();

	// Return required info
	QueryStartStopRet(pRet);

	return mp_VCon->GetView();
}

//void CRealConsole::OnWinEvent(DWORD anEvent, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime)
//{
//	_ASSERTE(hwnd!=nullptr);
//
//	if (hConWnd == nullptr && anEvent == EVENT_CONSOLE_START_APPLICATION && idObject == (LONG)mn_MainSrv_PID)
//	{
//		SetConStatus(L"Waiting for console server...");
//		SetHwnd(hwnd);
//	}
//
//	_ASSERTE(hConWnd!=nullptr && hwnd==hConWnd);
//	//TODO("!!! Сделать обработку событий и население m_Processes");
//	//
//	//AddProcess(idobject), и удаление idObject из списка процессов
//	// Не забыть, про обработку флажка Ntvdm
//	TODO("При отцеплении от консоли NTVDM - можно прибить этот процесс");
//
//	switch(anEvent)
//	{
//		case EVENT_CONSOLE_START_APPLICATION:
//			//A new console process has started.
//			//The idObject parameter contains the process identifier of the newly created process.
//			//If the application is a 16-bit application, the idChild parameter is CONSOLE_APPLICATION_16BIT and idObject is the process identifier of the NTVDM session associated with the console.
//		{
//			if (mn_InRecreate>=1)
//				mn_InRecreate = 0; // корневой процесс успешно пересоздался
//
//			//WARNING("Тут можно повиснуть, если нарваться на блокировку: процесс может быть добавлен и через сервер");
//			//Process Add(idObject);
//			// Если запущено 16битное приложение - необходимо повысить приоритет нашего процесса, иначе будут тормоза
//#ifndef WIN64
//			_ASSERTE(CONSOLE_APPLICATION_16BIT==1);
//
//			if (idChild == CONSOLE_APPLICATION_16BIT)
//			{
//				OnDosAppStartStop(sst_App16Start, idObject);
//
//				//if (mn_Comspec4Ntvdm == 0)
//				//{
//				//	// mn_Comspec4Ntvdm может быть еще не заполнен, если 16бит вызван из батника
//				//	_ASSERTE(mn_Comspec4Ntvdm != 0);
//				//}
//
//				//if (!(mn_ProgramStatus & CES_NTVDM))
//				//	SetProgramStatus(mn_ProgramStatus|CES_NTVDM);
//
//				//if (gOSVer.dwMajorVersion>5 || (gOSVer.dwMajorVersion==5 && gOSVer.dwMinorVersion>=1))
//				//	mb_IgnoreCmdStop = TRUE;
//
//				//SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
//			}
//
//#endif
//		} break;
//		case EVENT_CONSOLE_END_APPLICATION:
//			//A console process has exited.
//			//The idObject parameter contains the process identifier of the terminated process.
//		{
//			//WARNING("Тут можно повиснуть, если нарваться на блокировку: процесс может быть удален и через сервер");
//			//Process Delete(idObject);
//			//
//#ifndef WIN64
//			if (idChild == CONSOLE_APPLICATION_16BIT)
//			{
//				OnDosAppStartStop(sst_App16Stop, idObject);
//			}
//
//#endif
//		} break;
//	}
//}


void CRealConsole::OnServerClosing(DWORD anSrvPID, int* pnShellExitCode)
{
	if (anSrvPID == mn_MainSrv_PID && mh_MainSrv)
	{
		wchar_t szLog[120];
		swprintf_c(szLog, L"OnServerClosing: SrvPID=%u RootExitCode=", anSrvPID);
		if (pnShellExitCode && (*pnShellExitCode >= 0 && *pnShellExitCode <= 2048))
			swprintf_c(szLog+wcslen(szLog), countof(szLog)-wcslen(szLog)/*#SECURELEN*/, L"%u", *pnShellExitCode);
		else if (pnShellExitCode)
			swprintf_c(szLog+wcslen(szLog), countof(szLog)-wcslen(szLog)/*#SECURELEN*/, L"0x%08X", *pnShellExitCode);
		else
			wcscat_c(szLog, L"Unknown");
		LogString(szLog);

		// 131017 set flags to ON
		StopSignal();

		//int nCurProcessCount = m_Processes.size();
		//_ASSERTE(nCurProcessCount <= 1);
		SetInCloseConsole(true);
		m_ServerClosing.nRecieveTick = GetTickCount();
		m_ServerClosing.hServerProcess = mh_MainSrv;
		m_ServerClosing.nServerPID = anSrvPID;

		// Поскольку сервер закрывается, пайп более не доступен
		ms_ConEmuC_Pipe[0] = 0;
		ms_MainSrv_Pipe[0] = 0;
	}
	else
	{
		_ASSERTE(anSrvPID == mn_MainSrv_PID);
	}

	// Shell exit code
	if (pnShellExitCode)
	{
		m_RootInfo.nExitCode = *pnShellExitCode;
		m_RootInfo.bRunning = false;
	}

	// Reset starting state flags
	UpdateStartState(rss_NotStarted, true);
}

bool CRealConsole::isProcessExist(DWORD anPID)
{
	if (mn_InRecreate || isDetached() || !mn_ProcessCount)
		return false;

	bool bExist = false;
	MSectionLock SPRC; SPRC.Lock(&csPRC);
	int dwProcCount = (int)m_Processes.size();

	for (int i = 0; i < dwProcCount; i++)
	{
		ConProcess prc = m_Processes[i];
		if (prc.ProcessID == anPID)
		{
			bExist = true;
			break;
		}
	}

	SPRC.Unlock();

	return bExist;
}

int CRealConsole::GetProcesses(ConProcess** ppPrc, bool ClientOnly /*= false*/)
{
	if (mn_InRecreate && (mn_InRecreate != (DWORD)-1))
	{
		DWORD dwCurTick = GetTickCount();
		DWORD dwDelta = dwCurTick - mn_InRecreate;

		if (dwDelta > CON_RECREATE_TIMEOUT)
		{
			mn_InRecreate = 0;
			m_Args.Detached = crb_On; // Чтобы GUI не захлопнулся
		}
		else if (ppPrc == nullptr)
		{
			if (mn_InRecreate && !mb_ProcessRestarted && mh_MainSrv)
			{
				//DWORD dwExitCode = 0;

				if (!isServerAlive())
				{
					RecreateProcessStart();
				}
			}

			return 1; // Чтобы во время Recreate GUI не захлопнулся
		}
	}


	// The following block protect ConEmu window from unexpected termination
	if (isDetached())
	{
		return 1;
	}
	if (!mn_ProcessCount)
	{
		if (mb_NeedStartProcess || mb_InCreateRoot)
			return 1;
		if (mh_MainSrv && isServerAlive())
			return 1;
	}


	// If the process count was requested only
	if (ppPrc == nullptr || mn_ProcessCount == 0)
	{
		if (ppPrc) *ppPrc = nullptr;

		if (hConWnd && !mh_MainSrv)
		{
			if (!IsWindow(hConWnd))
			{
				_ASSERTE(FALSE && "Console window was abnormally terminated?");
				return 0;
			}
		}

		return ClientOnly ? mn_ProcessClientCount : mn_ProcessCount;
	}

	MSectionLock SPRC; SPRC.Lock(&csPRC);
	int dwProcCount = (int)m_Processes.size();
	int nCount = 0;

	if (dwProcCount > 0)
	{
		*ppPrc = (ConProcess*)calloc(dwProcCount, sizeof(ConProcess));
		if (*ppPrc == nullptr)
		{
			_ASSERTE((*ppPrc)!=nullptr);
			return dwProcCount;
		}

		//std::vector<ConProcess>::iterator end = m_Processes.end();
		//int i = 0;
		//for (std::vector<ConProcess>::iterator iter = m_Processes.begin(); iter != end; ++iter, ++i)
		for (int i = 0; i < dwProcCount; i++)
		{
			ConProcess prc = m_Processes[i];
			if (ClientOnly)
			{
				if (prc.IsConHost)
				{
					continue;
				}
				if (prc.ProcessID == mn_MainSrv_PID)
				{
					_ASSERTE(IsConsoleServer(prc.Name));
					continue;
				}
				_ASSERTE(!IsConsoleService(prc.Name));
				if (IsConsoleHelper(prc.Name))
				{
					continue;
				}
			}
			//(*ppPrc)[i] = *iter;
			(*ppPrc)[nCount++] = prc;
		}
	}
	else
	{
		*ppPrc = nullptr;
	}

	SPRC.Unlock();

	return nCount;
}

DWORD CRealConsole::GetProgramStatus()
{
	AssertThisRet(0);
	return mn_ProgramStatus;
}

DWORD CRealConsole::GetFarStatus()
{
	AssertThisRet(0);

	if ((mn_ProgramStatus & CES_FARACTIVE) == 0)
		return 0;

	return mn_FarStatus;
}

bool CRealConsole::isServerAlive()
{
	AssertThisRet(false);
	if (!mh_MainSrv || mh_MainSrv == INVALID_HANDLE_VALUE)
		return false;

#ifdef _DEBUG
	DWORD dwExitCode = 0, nSrvPID = 0, nErrCode = 0;
	LockFrequentExecute(500,mn_CheckFreqLock)
	{
		SetLastError(0);
		BOOL fSuccess = GetExitCodeProcess(mh_MainSrv, &dwExitCode);
		nErrCode = GetLastError();

		static bool bFnAcquired = false;
		static DWORD (WINAPI* getProcessId)(HANDLE);
		if (!bFnAcquired)
		{
			MModule kernel(GetModuleHandle(L"kernel32.dll"));
			kernel.GetProcAddress("GetProcessId", getProcessId);
			bFnAcquired = true;
		}
		if (getProcessId)
		{
			SetLastError(0);
			nSrvPID = getProcessId(mh_MainSrv);
			nErrCode = GetLastError();
		}
	}
#endif

	DWORD nServerWait = WaitForSingleObject(mh_MainSrv, 0);
	DEBUGTEST(DWORD nWaitErr = GetLastError());

	return (nServerWait == WAIT_TIMEOUT);
}

bool CRealConsole::isServerAvailable()
{
	if (isServerClosing())
		return false;
	TODO("На время переключения на альтернативный сервер - возвращать false");
	return true;
}

bool CRealConsole::isServerClosing(bool bStrict /*= false*/)
{
	AssertThisRet(true);

	if (m_ServerClosing.nServerPID && (m_ServerClosing.nServerPID == mn_MainSrv_PID))
		return true;

	// Otherwise we can get weird errors 'Maximum real console size was reached'
	// when closing a tab with several splits in it...
	if (!bStrict && mb_InCloseConsole)
		return true;

	return false;
}

DWORD CRealConsole::GetMonitorThreadID()
{
	AssertThisRet(0);
	return mn_MonitorThreadID;
}

DWORD CRealConsole::GetServerPID(bool bMainOnly /*= false*/)
{
	AssertThisRet(0);

	#if 0
	// During multiple tabs/splits initialization we may get bunch
	// of RCon-s with mb_InCreateRoot but empty mn_MainSrv_PID
	if (mb_InCreateRoot && !mn_MainSrv_PID)
	{
		_ASSERTE(!mb_InCreateRoot || mn_MainSrv_PID);
		Sleep(500);
		//_ASSERTE(!mb_InCreateRoot || mn_MainSrv_PID);
		//if (GetCurrentThreadId() != mn_MonitorThreadID) {
		//	WaitForSingleObject(mh_CreateRootEvent, 30000); -- низя - DeadLock
		//}
	}
	#endif

	return (mn_AltSrv_PID && !bMainOnly) ? mn_AltSrv_PID : mn_MainSrv_PID;
}

DWORD CRealConsole::GetTerminalPID()
{
	AssertThisRet(0);
	return mn_Terminal_PID;
}

void CRealConsole::SetMainSrvPID(DWORD anMainSrvPID, HANDLE ahMainSrv)
{
	_ASSERTE((mh_MainSrv==nullptr || mh_MainSrv==ahMainSrv) && "mh_MainSrv must be closed before!");
	_ASSERTE((anMainSrvPID!=0 || mn_AltSrv_PID==0) && "AltServer must be closed before!");

	DEBUGTEST(isServerAlive());

	#ifdef _DEBUG
	DWORD nSrvWait = ahMainSrv ? WaitForSingleObject(ahMainSrv, 0) : WAIT_TIMEOUT;
	_ASSERTE(nSrvWait == WAIT_TIMEOUT);
	#endif

	mh_MainSrv = ahMainSrv;
	mn_MainSrv_PID = anMainSrvPID;

	if (!anMainSrvPID)
	{
		// только сброс! установка в OnServerStarted(DWORD anServerPID, HANDLE ahServerHandle)
		mb_MainSrv_Ready = false;
		// Устанавливается в OnConsoleLangChange
		mn_ActiveLayout = 0;
		// Сброс
		ConHostSetPID(0);
		// cygwin/msys connector
		SetTerminalPID(0);
	}
	else if (gpSet->isLogging())
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"ServerPID=%u was set for VCon[%i]", anMainSrvPID, mp_VCon->Index()+1);
		// Add info both to GUI and CON logs
		gpConEmu->LogString(szInfo);
		// RCon logging
		CreateLogFiles();
		LogString(szInfo);
	}

	DEBUGTEST(isServerAlive());

	if (isActive(false))
		mp_ConEmu->mp_Status->OnServerChanged(mn_MainSrv_PID, mn_AltSrv_PID ? mn_AltSrv_PID : mn_Terminal_PID);

	DEBUGTEST(isServerAlive());
}

void CRealConsole::SetTerminalPID(DWORD anTerminalPID)
{
	if (mn_Terminal_PID != anTerminalPID)
	{
		mn_Terminal_PID = anTerminalPID;

		if (isActive(false))
			mp_ConEmu->mp_Status->OnServerChanged(mn_MainSrv_PID, mn_AltSrv_PID ? mn_AltSrv_PID : mn_Terminal_PID);
	}
}

void CRealConsole::SetAltSrvPID(DWORD anAltSrvPID/*, HANDLE ahAltSrv*/)
{
	if (isLogging())
	{
		wchar_t szLog[80];
		swprintf_c(szLog, L"AltServer changed: Old=%u New=%u", mn_AltSrv_PID, anAltSrvPID);
		LogString(szLog);
	}
	//_ASSERTE((mh_AltSrv==nullptr || mh_AltSrv==ahAltSrv) && "mh_AltSrv must be closed before!");
	//mh_AltSrv = ahAltSrv;
	mn_AltSrv_PID = anAltSrvPID;
}

void CRealConsole::SetInCloseConsole(bool InCloseConsole)
{
	if (mb_InCloseConsole != InCloseConsole)
		mb_InCloseConsole = InCloseConsole;
}

bool CRealConsole::InitAltServer(DWORD nAltServerPID/*, HANDLE hAltServer*/)
{
	// nAltServerPID may be 0, hAltServer вообще убрать
	bool bOk = false;

	ResetEvent(mh_ActiveServerSwitched);

	//// mh_AltSrv должен закрыться в MonitorThread!
	//HANDLE hOldAltServer = mh_AltSrv;
	//mh_AltSrv = nullptr;
	////mn_AltSrv_PID = nAltServerPID;
	////mh_AltSrv = hAltServer;
	SetAltSrvPID(nAltServerPID/*, hAltServer*/);

	if (!nAltServerPID && isServerClosing(true))
	{
		// Переоткрывать пайпы смысла нет, консоль закрывается
		SetSwitchActiveServer(false, eResetEvent, eDontChange);
		bOk = true;
	}
	else
	{
		SetSwitchActiveServer(true, eSetEvent, eResetEvent);

		HANDLE hWait[] = {mh_ActiveServerSwitched, mh_MonitorThread, mh_MainSrv, mh_TermEvent};
		DWORD nWait = WAIT_TIMEOUT;
		DEBUGTEST(DWORD nStartWait = GetTickCount());

		// mh_TermEvent выставляется после реального закрытия консоли
		// но если инициировано закрытие консоли - ожидание завершения смены
		// сервера может привести к блокировке, поэтому isServerClosing
		while ((nWait == WAIT_TIMEOUT) && !isServerClosing())
		{
			nWait = WaitForMultipleObjects(countof(hWait), hWait, FALSE, 100);

			#ifdef _DEBUG
			if ((nWait == WAIT_TIMEOUT) && nStartWait && ((GetTickCount() - nStartWait) > 2000))
			{
				_ASSERTE((nWait == WAIT_OBJECT_0) && "Switching Monitor thread to alternative server takes more than 2000ms");
			}
			#endif
		}

		if (nWait == WAIT_OBJECT_0)
		{
			_ASSERTE(mb_SwitchActiveServer==false && "Must be dropped by MonitorThread");
			SetSwitchActiveServer(false, eDontChange, eDontChange);
		}
		else
		{
			_ASSERTE(isServerClosing());
		}

		bOk = (nWait == WAIT_OBJECT_0);
	}

	if (isActive(false))
		mp_ConEmu->mp_Status->OnServerChanged(mn_MainSrv_PID, mn_AltSrv_PID ? mn_AltSrv_PID : mn_Terminal_PID);

	return bOk;
}

bool CRealConsole::ReopenServerPipes()
{
	DWORD nSrvPID = GetServerPID(false);
	HANDLE hSrvHandle = mh_MainSrv;

	if (isServerClosing(true))
	{
		LogString(L"!!! ReopenServerPipes was called in MainServerClosing !!!");
		_ASSERTE(FALSE && "ReopenServerPipes was called in MainServerClosing");
	}
	else if (mb_InCloseConsole)
	{
		LogString(L"Server was reactivated");
		m_ServerClosing.bBackActivated = TRUE;
	}

	wchar_t szDbgInfo[512];
	if (isLogging())
	{
		swprintf_c(szDbgInfo, L"ReopenServerPipes: ServerPID=%u", GetServerPID(false));
		LogString(szDbgInfo);
	}

	InitNames();

	// Reopen console changes notification event
	if (m_ConDataChanged.Open() == nullptr)
	{
		bool bSrvClosed = (WaitForSingleObject(hSrvHandle, 0) == WAIT_OBJECT_0);
		// -- We may not come in time due to multiple AltServers shutdown
		//Assert(mb_InCloseConsole && "m_ConDataChanged.Open() != nullptr"); UNREFERENCED_PARAMETER(bSrvClosed);
		swprintf_c(szDbgInfo, L"ReopenServerPipes: m_ConDataChanged.Open() failed, MainServer (PID=%u) is %s", GetServerPID(true), bSrvClosed ? L"Closed" : L"OK");
		LogString(szDbgInfo);
		return false;
	}

	// Reopen m_GetDataPipe
	if (!m_GetDataPipe.Open())
	{
		bool bSrvClosed = (WaitForSingleObject(hSrvHandle, 0) == WAIT_OBJECT_0);
		//Assert((bOpened || mb_InCloseConsole) && "m_GetDataPipe.Open() failed"); UNREFERENCED_PARAMETER(bSrvClosed);
		swprintf_c(szDbgInfo, L"ReopenServerPipes: m_GetDataPipe.Open() failed, MainServer (PID=%u) is %s", GetServerPID(true), bSrvClosed ? L"Closed" : L"OK");
		LogString(szDbgInfo);
		return false;
	}

	bool bActive = isActive(false);

	if (bActive)
		mp_ConEmu->mp_Status->OnServerChanged(mn_MainSrv_PID, mn_AltSrv_PID ? mn_AltSrv_PID : mn_Terminal_PID);

	UpdateServerActive(TRUE);

	if (RELEASEDEBUGTEST(isLogging(),true))
	{
		szDbgInfo[0] = 0;

		MSectionLock SC; SC.Lock(&csPRC);
		for (INT_PTR ii = 0; ii < m_Processes.size(); ii++)
		{
			ConProcess* i = &(m_Processes[ii]);
			if (i->ProcessID == nSrvPID)
			{
				swprintf_c(szDbgInfo, L"==> Active server changed to '%s' PID=%u", i->Name, nSrvPID);
				break;
			}
		}
		SC.Unlock();

		if (!*szDbgInfo)
			swprintf_c(szDbgInfo, L"==> Active server changed to PID=%u", nSrvPID);

		if (isLogging()) { LogString(szDbgInfo); } else { DEBUGSTRALTSRV(szDbgInfo); }
	}

	return true;
}

// Если bFullRequired - требуется чтобы сервер уже мог принимать команды
bool CRealConsole::isServerCreated(bool bFullRequired /*= false*/)
{
	if (!mn_MainSrv_PID)
		return false;

	if (bFullRequired && !mb_MainSrv_Ready)
		return false;

	return true;
}

DWORD CRealConsole::GetFarPID(bool abPluginRequired/*=false*/)
{
	AssertThisRet(0);

	if (!mn_FarPID  // Должен быть известен код PID
	        || ((mn_ProgramStatus & CES_FARACTIVE) == 0) // выставлен флаг
	        || ((mn_ProgramStatus & CES_NTVDM) == CES_NTVDM)) // Если активна 16бит программа - значит фар точно не доступен
		return 0;

	if (abPluginRequired)
	{
		if (mn_FarPID_PluginDetected && mn_FarPID_PluginDetected == mn_FarPID)
			return mn_FarPID_PluginDetected;
		else
			return 0;
	}

	return mn_FarPID;
}

void CRealConsole::SetProgramStatus(DWORD nDrop, DWORD nSet)
{
	#ifdef _DEBUG
	bool bWasNtvdm = (mn_ProgramStatus & CES_NTVDM)!=0;
	DWORD nPrevStatus = mn_ProgramStatus;
	#endif

	if (nDrop == (DWORD)-1)
	{
		mn_ProgramStatus = nSet;
	}
	else if (nDrop && nSet)
		mn_ProgramStatus = (mn_ProgramStatus & ~nDrop) | nSet;
	else if (nDrop)
		mn_ProgramStatus = (mn_ProgramStatus & ~nDrop);
	else if (nSet)
		mn_ProgramStatus = (mn_ProgramStatus | nSet);

	#ifdef _DEBUG
	if (bWasNtvdm && !(mn_ProgramStatus & CES_NTVDM))
	{
		bWasNtvdm = false; // For debug breakpoint
	}
	#endif
}

void CRealConsole::SetFarStatus(DWORD nNewFarStatus)
{
	mn_FarStatus = nNewFarStatus;
}

// Вызывается при запуске в консоли ComSpec
void CRealConsole::SetFarPID(DWORD nFarPID)
{
	bool bNeedUpdate = (mn_FarPID != nFarPID);

	#ifdef _DEBUG
	wchar_t szDbg[100];
	swprintf_c(szDbg, L"SetFarPID: New=%u, Old=%u\n", nFarPID, mn_FarPID);
	#endif

	if (nFarPID)
	{
		if ((mn_ProgramStatus & (CES_FARACTIVE|CES_FARINSTACK)) != (CES_FARACTIVE|CES_FARINSTACK))
			SetProgramStatus(0, CES_FARACTIVE|CES_FARINSTACK/*Flags*/);
	}
	else
	{
		if (mn_ProgramStatus & CES_FARACTIVE)
			SetProgramStatus(CES_FARACTIVE, 0/*Flags*/);
	}

	mn_FarPID = nFarPID;

	// Для фара могут быть настроены другие параметры фона и прочего...
	if (bNeedUpdate)
	{
		DEBUGSTRFARPID(szDbg);
		if (tabs.RefreshFarPID((mn_ProgramStatus & CES_FARACTIVE) ? nFarPID : 0))
			mp_ConEmu->mp_TabBar->Update();

		if ((GetActiveTabType() & fwt_TypeMask) == fwt_Panels)
		{
			CTab panels(__FILE__,__LINE__);
			if ((tabs.m_Tabs.GetTabByIndex(0, panels)) && (panels->Type() == fwt_Panels))
			{
				panels->SetName(GetPanelTitle());
			}
		}

		mp_VCon->Update(true);
	}
}

void CRealConsole::SetFarPluginPID(DWORD nFarPluginPID)
{
	bool bNeedUpdate = (mn_FarPID_PluginDetected != nFarPluginPID);

	wchar_t szLog[100] = L"";
	swprintf_c(szLog, L"SetFarPluginPID: New=%u, Old=%u", nFarPluginPID, mn_FarPID_PluginDetected);

	mn_FarPID_PluginDetected = nFarPluginPID;

	// Для фара могут быть настроены другие параметры фона и прочего...
	if (bNeedUpdate)
	{
		if (isLogging())
		{
			LogString(szLog);
		}
		#ifdef _DEBUG
		else
		{
			DEBUGSTRFARPID(szLog);
		}
		#endif

		if (tabs.RefreshFarPID(nFarPluginPID))
			mp_ConEmu->mp_TabBar->Update();

		mp_VCon->Update(true);
	}
}

ConEmuAnsiLog CRealConsole::GetAnsiLogInfo()
{
	// #Refactoring Implement contents of GetAnsiLogInfo as ConEmuAnsiLog method

	// Valid only for started server
	const DWORD nSrvPID = GetServerPID(true);
	if (!nSrvPID)
	{
		_ASSERTE(nSrvPID != 0); // ServerPID should be actualized already!
	}

	// Limited logging of console contents (same output as processed by ConEmu::ConsoleFlags::ProcessAnsi)
	ConEmuAnsiLog AnsiLog = {};

	wchar_t dir[MAX_PATH] = L"", name[40] = L"";
	const SYSTEMTIME& st = GetStartTime();
	msprintf(name, countof(name), CEANSILOGNAMEFMT, st.wYear, st.wMonth, st.wDay, nSrvPID);

	// Max path = (MAX_PATH + "\ConEmu-yyyy-mm-dd-p12345.log")
	if (m_Args.pszAnsiLog && *m_Args.pszAnsiLog)
	{
		// #ANSI Add ability to enable/disable codes via `L0` or `L1`; allow pszAnsiLog to be "" to indicate switch
		AnsiLog.Enabled = true;
		AnsiLog.LogAnsiCodes = gpSet->isAnsiLogCodes;
		lstrcpyn(dir, m_Args.pszAnsiLog, static_cast<int>(std::size(dir)));
		// Is it only a directory, without file name?
		const auto dir_tail = dir[wcslen(dir) - 1];
		if (dir_tail != L'\\' && dir_tail != L'/'
			&& !DirectoryExists(CEStr(ExpandEnvStr(dir))))
		{
			// File name may be specified in -new_console:L:"C:\temp\my-file.log"
			const auto ptr_ext = PointToExt(dir);
			// We don't restrict to use only certain extension
			if (ptr_ext && ptr_ext[0] == L'.' && ptr_ext[1])
				name[0] = 0;
		}
	}
	else
	{
		AnsiLog.Enabled = gpSet->isAnsiLog;
		AnsiLog.LogAnsiCodes = gpSet->isAnsiLogCodes;
		lstrcpyn(dir,
			(gpSet->isAnsiLog && gpSet->pszAnsiLog && *gpSet->pszAnsiLog)
				? gpSet->pszAnsiLog : CEANSILOGFOLDER,
			static_cast<int>(std::size(dir)));
	}

	if (name[0])
	{
		const int dir_len_max = static_cast<int>(std::size(dir) - wcslen(name) - 2); // possible slash and '\0'-terminator
		CEStr concat(JoinPath(dir, name));
		lstrcpyn(AnsiLog.Path, concat, static_cast<int>(std::size(AnsiLog.Path)));
	}
	else
	{
		lstrcpyn(AnsiLog.Path, dir, static_cast<int>(std::size(AnsiLog.Path)));
	}

	return AnsiLog;
}

LPCWSTR CRealConsole::GetConsoleInfo(LPCWSTR asWhat, CEStr& rsInfo)
{
	wchar_t szTemp[MAX_PATH*4] = L"";
	LPCWSTR pszVal = szTemp;

	if (lstrcmpi(asWhat, L"ServerPID") == 0)
		ultow_s(GetServerPID(true), szTemp, 10);
	else if (lstrcmpi(asWhat, L"DrawHWND") == 0)
		msprintf(szTemp, countof(szTemp), L"0x%08X", LODWORD(VCon()->GetView()));
	else if (lstrcmpi(asWhat, L"BackHWND") == 0)
		msprintf(szTemp, countof(szTemp), L"0x%08X", LODWORD(VCon()->GetBack()));
	else if (lstrcmpi(asWhat, L"WorkDir") == 0)
		pszVal = GetConsoleStartDir(rsInfo);
	else if (lstrcmpi(asWhat, L"CurDir") == 0)
		pszVal = GetConsoleCurDir(rsInfo, false);
	else if (lstrcmpi(asWhat, L"ActivePID") == 0)
		msprintf(szTemp, countof(szTemp), L"%u", GetActivePID());
	else if (lstrcmpi(asWhat, L"RootName") == 0)
		pszVal = rsInfo.Set(GetRootProcessName());
	else if (lstrcmpi(asWhat, L"Root") == 0 || lstrcmpi(asWhat, L"RootInfo") == 0)
	{
		DWORD dwServerPID = GetServerPID(true);
		CESERVER_REQ *pOut = nullptr, *pIn = ExecuteNewCmd(CECMD_GETROOTINFO, sizeof(CESERVER_REQ_HDR));
		if (dwServerPID)
			pOut = ExecuteSrvCmd(dwServerPID, pIn, ghWnd);

		// Return <Root ... /> xml
		CreateRootInfoXml(GetRootProcessName(),
			(pOut && (pOut->DataSize() >= sizeof(CESERVER_ROOT_INFO))) ? &pOut->RootInfo : nullptr,
			rsInfo);

		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);

		pszVal = rsInfo.ms_Val;
	}
	else if (lstrcmpi(asWhat, L"AnsiLog") == 0)
	{
		const DWORD nSrvPID = GetServerPID(true);
		if (nSrvPID)
		{
			const ConEmuAnsiLog& AnsiLog = GetAnsiLogInfo();
			if (AnsiLog.Enabled)
			{
				pszVal = rsInfo.Set(AnsiLog.Path);
			}
		}
	}

	if (pszVal == szTemp)
		pszVal = rsInfo.Set(szTemp);

	return pszVal;
}

// Used in StatusBar
LPCWSTR CRealConsole::GetActiveProcessInfo(CEStr& rsInfo)
{
	AssertThisRet(nullptr);

	DWORD nPID = 0;
	ConProcess Process = {};

	if ((nPID = GetActivePID(&Process)) == 0)
	{
		if (m_RootInfo.nPID)
		{
			wchar_t szExitInfo[80];
			if (m_RootInfo.bRunning)
				swprintf_c(szExitInfo, L":%u", m_RootInfo.nPID);
			else if (static_cast<int32_t>(m_RootInfo.nExitCode) >= -255)
				swprintf_c(szExitInfo, L":%u %s %i",
					m_RootInfo.nPID,
					CLngRc::getRsrc(lng_ExitCode/*"exit code"*/),
					static_cast<int32_t>(m_RootInfo.nExitCode));
			else
				swprintf_c(szExitInfo, L":%u %s 0x%08X",
					m_RootInfo.nPID,
					CLngRc::getRsrc(lng_ExitCode/*"exit code"*/),
					m_RootInfo.nExitCode);
			rsInfo = CEStr(ms_RootProcessName, szExitInfo);
		}
		else if ((m_StartState < rss_ProcessActive) && ms_RootProcessName[0])
		{
			rsInfo = CEStr(L"Starting: ", ms_RootProcessName);
		}
		else
		{
			rsInfo = L"Unknown state";
		}
	}
	else
	{
		TODO("Show full running process tree?");

		wchar_t szNameTrim[64];
		lstrcpyn(szNameTrim, *Process.Name ? Process.Name : L"???", countof(szNameTrim));

		DWORD nInteractivePID = GetInteractivePID();
		DWORD nFarPluginPID = GetFarPID(true);
		LPCWSTR pszInteractive = (nPID == nFarPluginPID) ? L"#"
			: (nPID == nInteractivePID) ? L"*"
			: L"";

		bool isAdmin = isAdministrator();
		LPCWSTR pszAdmin = isAdmin ? L"*" : L"";

		size_t cchTextMax = 255;
		wchar_t* pszText = rsInfo.GetBuffer(cchTextMax);

		// Issue 1708: show active process bitness and UAC state
		if (IsWindows64())
		{
			wchar_t szBits[8] = L"";
			if (Process.Bits > 0) swprintf_c(szBits, L"%i", Process.Bits);

			swprintf_c(pszText, cchTextMax/*#SECURELEN*/, _T("%s%s[%s%s]:%u"), szNameTrim, pszInteractive, pszAdmin, szBits, nPID);
		}
		else
		{
			swprintf_c(pszText, cchTextMax/*#SECURELEN*/, _T("%s%s%s:%u"), szNameTrim, pszInteractive, pszAdmin, nPID);
		}
	}

	return rsInfo.c_str(L"Unknown state");
}

// Вернуть PID "условно активного" процесса в консоли
DWORD CRealConsole::GetActivePID(ConProcess* rpProcess /*= nullptr*/)
{
	AssertThisRet(0);

	DWORD nPID = 0;

	if (!nPID && m_ChildGui.hGuiWnd)
		nPID = m_ChildGui.Process.ProcessID;
	if (!nPID)
		nPID = GetFarPID();
	if (!nPID)
		nPID = m_ActiveProcess.ProcessID;

	if (nPID && rpProcess)
		GetProcessInformation(nPID, rpProcess);

	return nPID;
}

bool CRealConsole::GetProcessInformation(DWORD nPID, ConProcess* rpProcess /*= nullptr*/)
{
	AssertThisRet(false);
	if (!nPID)
		return false;

	bool bFound = false;

	if (m_ChildGui.hGuiWnd && (nPID == m_ChildGui.Process.ProcessID))
	{
		if (rpProcess)
			*rpProcess = m_ChildGui.Process;
		bFound = true;
	}
	else if (nPID == m_ActiveProcess.ProcessID)
	{
		if (rpProcess)
			*rpProcess = m_ActiveProcess;
		bFound = true;
	}
	else if (nPID == m_AppDistinctProcess.ProcessID)
	{
		if (rpProcess)
			*rpProcess = m_AppDistinctProcess;
		bFound = true;
	}
	else
	{
		ConProcess* pPrcList = nullptr;
		ConProcess* pFound = nullptr;
		int nCount = GetProcesses(&pPrcList, false/*ClientOnly*/);
		if (pPrcList && (nCount > 0))
		{
			for (int i = 0; i < nCount; i++)
			{
				if (pPrcList[i].ProcessID == nPID)
				{
					if (rpProcess)
						*rpProcess = pPrcList[i];
					bFound = true;
					break;
				}
			}
		}
		SafeFree(pPrcList);
	}

	return bFound;
}

DWORD CRealConsole::GetInteractivePID()
{
	AssertThisRet(0);

	DWORD nPID = 0;

	if (!nPID && m_ChildGui.hGuiWnd)
		nPID = m_ChildGui.Process.ProcessID;
	if (!nPID && m_AppMap.IsValid())
		nPID = m_AppMap.Ptr()->nLastReadInputPID;
	if (!nPID)
		nPID = GetFarPID(true);

	return nPID;
}

DWORD CRealConsole::GetLoadedPID()
{
	if (isServerClosing() || !GetServerPID(true))
		return 0;

	// For example, ChildGui started with: set ConEmuReportExe=notepad.exe & notepad.exe
	// Check waiting dialog box with caption "ConEmuHk, PID=%u, ..."
	// The content must be "<ms_RootProcessName> loaded!"

	struct Impl {
		LPCWSTR pszName;
		HWND hFound;
		DWORD nPID;
		wchar_t szText[255];
		static BOOL CALLBACK EnumProc(HWND hWnd, LPARAM lParam)
		{
			Impl* p = (Impl*)lParam;
			if ((GetClassName(hWnd, p->szText, countof(p->szText)) > 0)
				&& (wcscmp(p->szText, L"#32770") == 0))
			{
				wchar_t szCmp[] = L"ConEmuHk, PID=";
				INT_PTR iLen = wcslen(szCmp);
				if ((GetWindowText(hWnd, p->szText, countof(p->szText)) > 0)
					&& (wcsncmp(p->szText, szCmp, iLen) == 0))
				{
					wchar_t* pszEnd = nullptr;
					p->nPID = wcstoul(p->szText+iLen, &pszEnd, 10);
					// TODO: Можно бы еще проверить текст в диалоге...
					return (p->nPID == 0);
				}
			}
			return TRUE; // Continue search
		};
	} impl = {ms_RootProcessName};

	EnumWindows(Impl::EnumProc, (LPARAM)&impl);

	return impl.nPID;
}

// Используется при запросе подтверждения закрытия
// В идеале - должен возвращать PID "работающего" процесса
// то есть процесса, выполняющего в данный момент скрипт,
// или копирующего файлы, или ждущего подтверждения пользователя...
// PID или совпадает с GetActivePID или 0
DWORD CRealConsole::GetRunningPID()
{
	AssertThisRet(0);

	if (m_ChildGui.hGuiWnd)
	{
		return 0; // We can't handle ChildGui
	}

	if (!mn_ProcessCount)
	{
		return 0; // No process was found yet
	}

	DWORD nPID = 0, nCount = 0;

	// `git-cmd.exe` or `git-bash.exe` from Git-for-Windows v2.x
	DWORD nGitHelperPID = 0, nGitHelper = 0;

	MSectionLock SPRC; SPRC.Lock(&csPRC);

	for (int i = 0; i < mn_ProcessCount; i++)
	{
		const ConProcess& prc = m_Processes[i];
		if (prc.IsConHost || prc.IsMainSrv || prc.IsTermSrv
			|| (prc.ProcessID == mn_MainSrv_PID))
		{
			continue;
		}
		nPID = prc.ProcessID; nCount++;
		// `git-cmd.exe` or `git-bash.exe` from Git-for-Windows v2.x
		// They are waiting for actual root process bash.exe or smth...
		if (IsGitBashHelper(prc.Name))
		{
			if (!nGitHelper)
				nGitHelperPID = prc.ProcessID;
			nGitHelper++;
		}
	}

	SPRC.Unlock();

	// If root was `git-bash.exe` or `git-cmd.exe` than correct process counter
	if ((nCount == 2) && (nGitHelper == 1))
	{
		if (nGitHelperPID != nPID)
		{
			nCount = 1;
		}
	}

	// If the only shell is running now - consider it is free
	if (nCount <= 1)
	{
		TODO("Recheck conditions. May be need to check if the shell is in ReadLine?");
		return 0;
	}

	return nPID;
}

LPCWSTR CRealConsole::GetActiveProcessName()
{
	AssertThisRet(nullptr);
	if (!m_ActiveProcess.ProcessID)
		return nullptr;
	return m_ActiveProcess.Name;
}

void CRealConsole::ResetActiveAppSettingsId()
{
	mb_ForceRefreshAppId = true;
}

// Вызывается перед запуском процесса
int CRealConsole::GetDefaultAppSettingsId()
{
	AssertThisRet(-1);

	int iAppId = -1;
	LPCWSTR lpszCmd = nullptr;
	//wchar_t* pszBuffer = nullptr;
	LPCWSTR pszName = nullptr;
	CmdArg szExe;
	LPCWSTR pszTemp = nullptr;
	LPCWSTR pszIconFile = (m_Args.pszIconFile && *m_Args.pszIconFile) ? m_Args.pszIconFile : nullptr;
	bool bAsAdmin = false;
	CStartEnvTitle setTitle(&ms_DefTitle);

	if (m_Args.pszSpecialCmd)
	{
		lpszCmd = m_Args.pszSpecialCmd;
	}
	else if (m_Args.Detached == crb_On)
	{
		goto wrap;
	}
	else
	{
		// Some tricky, but while starting we need to show at least one "empty" tab,
		// otherwise tabbar without tabs looks weird. Therefore on starting stage
		// m_Args.pszSpecialCmd may be nullptr. This is not so good, because GetCmd()
		// may return task name instead of real command.
		_ASSERTE(m_Args.pszSpecialCmd && *m_Args.pszSpecialCmd && "Command line must be specified already!");

		lpszCmd = gpConEmu->GetCmd(nullptr, true);

		//// May be this is batch?
		//pszBuffer = mp_ConEmu->LoadConsoleBatch(lpszCmd);
		//if (pszBuffer && *pszBuffer)
		//	lpszCmd = pszBuffer;
	}

	if (!lpszCmd || !*lpszCmd)
	{
		SetRootProcessName(nullptr);
		goto wrap;
	}

	// Parse command line
	ProcessSetEnvCmd(lpszCmd, nullptr, &setTitle);
	pszTemp = lpszCmd;

	if ((pszTemp = NextArg(pszTemp, szExe)))
	{
		pszName = PointToName(szExe);

		pszTemp = (*lpszCmd == L'"') ? nullptr : PointToName(lpszCmd);
		if (pszTemp && (wcschr(pszName, L'.') == nullptr) && (wcschr(pszTemp, L'.') != nullptr))
		{
			// Если в lpszCmd указан полный путь к запускаемому файлу без параметров и без кавычек
			if (FileExists(lpszCmd))
				pszName = pszTemp;
		}

		if (pszName != pszTemp)
			lpszCmd = szExe;
	}

	if (!pszName)
	{
		SetRootProcessName(nullptr);
		goto wrap;
	}

	if (wcschr(pszName, L'.') == nullptr)
	{
		// If extension was not defined, assume it's an .exe
		szExe = CEStr(szExe.c_str(), L".exe");
		pszName = PointToName(szExe);
	}

	SetRootProcessName(pszName);

	// In fact, m_Args.bRunAsAdministrator may be not true on startup
	bAsAdmin = m_Args.pszSpecialCmd ? (m_Args.RunAsAdministrator == crb_On) : mp_ConEmu->mb_IsUacAdmin;

	// Done. Get AppDistinct ID
	iAppId = gpSet->GetAppSettingsId(pszName, bAsAdmin);

	if (!pszIconFile)
		pszIconFile = szExe;

wrap:
	// Load (or create) icon for new tab
	if (mb_NeedLoadRootProcessIcon)
	{
		mb_NeedLoadRootProcessIcon = false;
		mn_RootProcessIcon = mp_ConEmu->mp_TabBar->CreateTabIcon(pszIconFile, bAsAdmin, GetStartupDir());
	}
	// Fin
	return iAppId;
}

int CRealConsole::GetActiveAppSettingsId(bool bReload /*= false*/)
{
	AssertThisRet(-1);

	const int   iLastId = mn_LastAppSettingsId;
	const bool  isAdmin = isAdministrator();

	if (!m_AppDistinctProcess.ProcessID)
	{
		if ((mn_LastAppSettingsId == -2) || bReload || mb_ForceRefreshAppId)
			mn_LastAppSettingsId = GetDefaultAppSettingsId();
		goto wrap;
	}

	if (bReload || mb_ForceRefreshAppId)
	{
		mb_ForceRefreshAppId = false;

		int iAppId = gpSet->GetAppSettingsId(m_AppDistinctProcess.Name, isAdmin);

		// When explicit AppDistinct not found - take settings for the root process
		// ms_RootProcessName must be processed in prev. GetDefaultAppSettingsId/PrepareDefaultColors
		if ((iAppId == -1) && (*ms_RootProcessName))
		{
			// m_AppDistinctProcess must contain only processes which have AppDistinct,
			// but JIC try with root process
			iAppId = gpSet->GetAppSettingsId(ms_RootProcessName, isAdmin);
		}

		// Cache it
		mn_LastAppSettingsId = iAppId;
	}

wrap:
	// If changed - refresh VCon
	if (iLastId != mn_LastAppSettingsId)
		mp_VCon->OnAppSettingsChanged(mn_LastAppSettingsId);
	return mn_LastAppSettingsId;
}

void CRealConsole::SetActivePID(const ConProcess* apProcess)
{
	bool bChanged = false;

	if (!apProcess || !apProcess->ProcessID)
	{
		if (m_ActiveProcess.ProcessID)
		{
			m_ActiveProcess = {};
			bChanged = true;
		}
	}
	else if (m_ActiveProcess.ProcessID != apProcess->ProcessID)
	{
		UpdateStartState(rss_ProcessActive);
		m_ActiveProcess = *apProcess;
		bChanged = true;
	}

	if (bChanged && isActive(false))
	{
		mp_ConEmu->mp_Status->UpdateStatusBar(true);
	}
}

void CRealConsole::SetAppDistinctPID(const ConProcess* apProcess)
{
	bool bChanged = false;

	if (!apProcess || !apProcess->ProcessID)
	{
		// Supposed to be during startup only
		bChanged = (m_AppDistinctProcess.ProcessID != 0);
		m_AppDistinctProcess = {};
	}
	else if (m_AppDistinctProcess.ProcessID != apProcess->ProcessID)
	{
		_ASSERTE(apProcess!=nullptr);
		m_AppDistinctProcess = *apProcess;
		bChanged = true;
	}

	if (bChanged)
	{
		// Refresh settings
		int iLastId = mn_LastAppSettingsId;
		int iAppId = GetActiveAppSettingsId(true);

		// Update is called by GetActiveAppSettingsId >> CVirtualConsole::OnAppSettingsChanged
		// mp_VCon->Invalidate();

		wchar_t szLog[80];
		if (iLastId != iAppId)
			swprintf_c(szLog, L"AppSettingsID changed %i >> %i for PID=%u", iLastId, iAppId, (apProcess ? apProcess->ProcessID : 0));
		else
			swprintf_c(szLog, L"AppSettingsID %i for PID=%u", iAppId, (apProcess ? apProcess->ProcessID : 0));
		LogString(szLog);
	}
}

// Обновить статус запущенных программ
// Возвращает TRUE если сменился статус (Far/не Far)
bool CRealConsole::ProcessUpdateFlags(bool abProcessChanged)
{
	bool lbChanged = false;
	//Warning: Должен вызываться ТОЛЬКО из ProcessAdd/ProcessDelete, т.к. сам секцию не блокирует
	bool bIsFar = false, bIsTelnet = false, bIsCmd = false;
	DWORD dwFarPID = 0;
	DWORD dwTerminalPID = 0;
	ConProcess ActivePID = {};
	// Наличие 16bit определяем ТОЛЬКО по WinEvent. Иначе не получится отсечь его завершение,
	// т.к. процесс ntvdm.exe не выгружается, а остается в памяти.
	bool bIsNtvdm = (mn_ProgramStatus & CES_NTVDM) == CES_NTVDM;

	if (bIsNtvdm && mn_Comspec4Ntvdm)
		bIsCmd = true;

	int nClientCount = 0;

	DWORD nInteractivePID = m_AppMap.IsValid() ? m_AppMap.Ptr()->nLastReadInputPID : 0;
	ConProcess AppDistinctPID = {};
	bool bAppIdProcessExists = false;
	CESERVER_ROOT_INFO Root = {};

	//std::vector<ConProcess>::reverse_iterator iter = m_Processes.rbegin();
	//std::vector<ConProcess>::reverse_iterator rend = m_Processes.rend();
	//
	//while (iter != rend)
	for (INT_PTR ii = (m_Processes.size() - 1); ii >= 0; ii--)
	{
		ConProcess* iter = &(m_Processes[ii]);

		// Skip ConEmuC Server process
		if (iter->ProcessID == mn_MainSrv_PID)
			continue;
		_ASSERTE(iter->IsMainSrv==false);
		_ASSERTE(iter->ProcessID!=0);

		// Skip Windows console sybsystem process `conhost.exe`
		if (iter->IsConHost || !iter->ProcessID)
			continue;

		if (!IsConsoleHelper(iter->Name))
			nClientCount++;

		// Root process would be the last one in our iteration
		Root.nPID = iter->ProcessID;

		// Far Manager?
		if (!bIsFar)
		{
			// Process ID with ConEmu.dll Far Manager plugin loaded
			if (iter->ProcessID == mn_FarPID_PluginDetected)
			{
				iter->IsFar = true; // In case of the specific name (not a `far.exe`)
				iter->IsFarPlugin = true;
			}

			// It's a Far Manager
			if (iter->IsFar)
			{
				if (!dwFarPID)
					dwFarPID = iter->ProcessID;
				bIsFar = true;
			}
		}

		if (!bIsTelnet && iter->IsTelnet)
			bIsTelnet = true;

		if (!bIsFar && !bIsCmd && iter->IsCmd)
			bIsCmd = true;

		if (iter->IsTermSrv)
			dwTerminalPID = iter->ProcessID;

		// Look for roughly active process
		if (!ActivePID.ProcessID)
			ActivePID = *iter;
		else if (ActivePID.ProcessID == iter->ParentPID)
			ActivePID = *iter;

		// Check for interactive process, if it is noted in AppDistinct
		if (nInteractivePID && (iter->ProcessID == nInteractivePID))
		{
			int iAppId = gpSet->GetAppSettingsId(iter->Name, isAdministrator());
			if (iAppId >= 0)
			{
				AppDistinctPID = *iter;
			}
		}
		// Last detected process for AppID is alive?
		if (m_AppDistinctProcess.ProcessID && (m_AppDistinctProcess.ProcessID == iter->ProcessID))
		{
			bAppIdProcessExists = true;
		}
	}

	TODO("Однако, наверное cmd.exe/tcc.exe может быть запущен и в 'фоне'? Например из Update");

	DWORD nNewProgramStatus = 0;

	if (bIsFar) // сначала - ставим флаг "InStack", т.к. ниже флаг фара может быть сброшен из-за bIsCmd
	{
		nNewProgramStatus |= CES_FARINSTACK;
	}

	if (bIsCmd && bIsFar)  // Если в консоли запущен cmd.exe/tcc.exe - значит (скорее всего?) фар выполняет команду
	{
		bIsFar = false; dwFarPID = 0;
	}

	if (bIsFar)
	{
		nNewProgramStatus |= CES_FARACTIVE;
	}

	if (bIsFar && bIsNtvdm)
	{
		// 100627 -- считаем, что фар не запускает 16бит программные без cmd (%comspec%)
		bIsNtvdm = false;
	}

	if (bIsTelnet)
	{
		nNewProgramStatus |= CES_TELNETACTIVE;
	}

	if (bIsNtvdm)  // определяется выше как "(mn_ProgramStatus & CES_NTVDM) == CES_NTVDM"
	{
		if (!(mn_ProgramStatus & CES_NTVDM))
			LogString(L"NTVDM.EXE was detected, setting CES_NTVDM");
		nNewProgramStatus |= CES_NTVDM;
	}
	else if (mn_ProgramStatus & CES_NTVDM)
	{
		LogString(L"NTVDM.EXE was terminated, clearing CES_NTVDM");
	}

	if (mn_ProgramStatus != nNewProgramStatus)
	{
		SetProgramStatus((DWORD)-1, nNewProgramStatus);
	}

	mn_ProcessCount = (int)m_Processes.size();
	mn_ProcessClientCount = nClientCount;

	if (dwFarPID && mn_FarPID != dwFarPID)
	{
		AllowSetForegroundWindow(dwFarPID);
	}

	if (!mn_FarPID && mn_FarPID != dwFarPID)
	{
		// Если ДО этого был НЕ фар, а сейчас появился фар - перерисовать панели.
		// Это нужно на всякий случай, чтобы гарантированно отрисовались расширения цветов и шрифтов.
		// Если был фар, а стал НЕ фар - перерисовку не делаем, чтобы на экране
		// не мелькали цветные артефакты (пример - вызов ActiveHelp из редактора)
		lbChanged = TRUE;
	}

	SetFarPID(dwFarPID);

	// Let remember Root console process?
	if (Root.nPID && (Root.nPID != m_RootInfo.nPID))
	{
		Root.bRunning = TRUE;
		Root.nExitCode = STILL_ACTIVE;
		UpdateRootInfo(Root);
	}

	if (mn_Terminal_PID != dwTerminalPID)
		SetTerminalPID(dwTerminalPID);

	if (m_ActiveProcess.ProcessID != ActivePID.ProcessID)
		SetActivePID(&ActivePID);

	// if it *found*, *exists*, and *changed*
	if (AppDistinctPID.ProcessID && (AppDistinctPID.ProcessID != m_AppDistinctProcess.ProcessID))
		SetAppDistinctPID(&AppDistinctPID);
	else if (m_AppDistinctProcess.ProcessID && !bAppIdProcessExists)
		SetAppDistinctPID(nullptr);

	// Nothing was found?
	if (mn_ProcessCount == 0)
	{
		if (mn_InRecreate == 0)
		{
			StopSignal();
		}
		else if (mn_InRecreate == 1)
		{
			mn_InRecreate = 2;
		}
	}

	// Refresh list on the Settings/Info page, and tab titles
	if (abProcessChanged)
	{
		mp_ConEmu->UpdateProcessDisplay(abProcessChanged);

		mp_ConEmu->mp_TabBar->Update();
	}

	return lbChanged;
}

// Возвращает TRUE если сменился статус (Far/не Far)
bool CRealConsole::ProcessUpdate(const DWORD *apPID, UINT anCount)
{
	BOOL lbRecreateOk = FALSE;
	bool bIsWin64 = IsWindows64();

	if (mn_InRecreate && mn_ProcessCount == 0)
	{
		// Раз сюда что-то пришло, значит мы получили пакет, значит консоль запустилась
		lbRecreateOk = TRUE;
	}

	if (m_ConHostSearch)
	{
		if (!mn_ConHost_PID)
		{
			// Ищем новые процессы "conhost.exe"
			// Раз наш сервер "проснулся" - значит "conhost.exe" уже должен быть
			// "ProcessUpdate(SrvPID, 1)" вызывается еще из StartProcess
			ConHostSearch(anCount>1);
		}
	}

	// #TODO replace m_TerminatedPIDs with dedicated structure
	std::unordered_map<DWORD, size_t> terminatedMap;
	terminatedMap.reserve(countof(m_TerminatedPIDs));
	for (UINT k = 0; k < countof(m_TerminatedPIDs); k++)
	{
		if (!m_TerminatedPIDs[k])
			continue;
		// PID was marked as "closing", don't add it again
		terminatedMap.insert({ m_TerminatedPIDs[k], k });
	}

	std::unordered_map<DWORD, size_t> pidsMap;
	std::vector<DWORD> pidsList;
	if (apPID && anCount > 0)
	{
		_ASSERTE(mn_ConHost_PID == 0 || (apPID ? *apPID : 0) != mn_ConHost_PID);

		pidsMap.reserve(anCount);
		pidsList.reserve(anCount);
		bool conhostAdded = false;
		auto addPid = [&pidsMap, &pidsList, &terminatedMap](const DWORD pid)
		{
			if (!pid)
				return false;
			if (terminatedMap.count(pid))
				return false;
			const auto inserted = pidsMap.insert({ pid, pidsList.size() });
			if (inserted.second)
				pidsList.push_back(pid);
			return inserted.second;
		};
		for (UINT i = 0; i < anCount; ++i)
		{
			if (!addPid(apPID[i]))
				continue;

			if (mn_ConHost_PID && !conhostAdded)
			{
				// Let's show conhost.exe as second process in a console
				std::ignore = addPid(mn_ConHost_PID);
				conhostAdded = true;
			}
		}

		if (mn_ConHost_PID && !conhostAdded)
		{
			std::ignore = addPid(mn_ConHost_PID);
		}
	}

	MSectionLock SPRC; SPRC.Lock(&csPRC);

	CProcessData* pProcData = nullptr;

	// Drop Far processes, which aren't exist in our console
	for (UINT j = 0; j < mn_FarPlugPIDsCount; j++)
	{
		if (m_FarPlugPIDs[j] == 0)
			continue;

		if (!pidsMap.count(m_FarPlugPIDs[j]))
			m_FarPlugPIDs[j] = 0;
	}

	// Check which processes already/still exists in our console
	for (auto& proc : m_Processes)
	{
		auto found = pidsMap.find(proc.ProcessID);
		if (found != pidsMap.end())
		{
			proc.inConsole = true;
			pidsList[found->second] = 0;
			pidsMap.erase(found);
		}
		else
		{
			proc.inConsole = false;
		}
	}

	// Are there new processes to add?
	const bool bProcessNew = !pidsMap.empty();
	// Some were terminated?
	const bool bProcessDel = std::any_of(m_Processes.begin(), m_Processes.end(),
		[](const ConProcess& proc) { return proc.inConsole == false;  });

	// Are there changes?
	bool bProcessChanged = false;
	if (bProcessNew || bProcessDel)
	{
		MArray<ConProcess> addProcesses;
		addProcesses.reserve(pidsMap.size());
		std::unordered_set<DWORD> notAlive;

		// #PROCESS Mark somehow failed processes to avoid permanent calls to CreateToolhelp32Snapshot
		// Now we assume that PID's came from Server are valid (retrieved from conhost)
		MToolHelpProcess processes;
		if (!processes.Open())
		{
			const DWORD dwErr = GetLastError();
			_ASSERTE(FALSE && "Open processes snapshot failed");
			wchar_t szError[255];
			swprintf_c(szError, L"Can't create process snapshot, ErrCode=0x%08X", dwErr);
			mp_ConEmu->DebugStep(szError);
		}
		else
		{
			notAlive.reserve(m_Processes.size());
			for (const auto& cp : m_Processes)
			{
				notAlive.insert(cp.ProcessID);
			}

			PROCESSENTRY32W pi{};
			while (processes.Next(pi))
			{
				if (!pi.th32ProcessID)
					continue;

				// It was reported, but still exists, waiting...
				terminatedMap.erase(pi.th32ProcessID);

				// Alive processes
				notAlive.erase(pi.th32ProcessID);

				// Is it new process?
				if (!pidsMap.count(pi.th32ProcessID))
					continue;

				ConProcess cp{};
				cp.ProcessID = pi.th32ProcessID;
				cp.ParentPID = pi.th32ParentProcessID;
				ProcessCheckName(cp, pi.szExeFile); //far, telnet, cmd, tcc, conemuc, и пр.
				cp.inConsole = true;

				if (!cp.Bits)
				{
					if (bIsWin64)
					{
						cp.Bits = GetProcessBits(cp.ProcessID);
						// Will fail with elevated consoles/processes
						if (cp.Bits == 0)
						{
							if (!pProcData)
								pProcData = new CProcessData();
							if (pProcData)
								pProcData->GetProcessName(cp.ProcessID, nullptr, 0, nullptr, 0, &cp.Bits);
						}
					}
					else
					{
						cp.Bits = 32;
					}
				}

				addProcesses.push_back(std::move(cp));
			}
		}

		if ((!terminatedMap.empty() || !addProcesses.empty() || !notAlive.empty() || bProcessDel)
			&& SPRC.RelockExclusive(300))
		{
			for (auto& cp : addProcesses)
			{
				m_Processes.push_back(std::move(cp));
				bProcessChanged = true;
			}

			for (const auto& iter : terminatedMap)
			{
				if (m_TerminatedPIDs[iter.second] == iter.first)
					m_TerminatedPIDs[iter.second] = 0;
			}

			// Remove dead processes
			for (ssize_t i = 0; i < m_Processes.size();)
			{
				ConProcess* iter = &(m_Processes[i]);
				if (!iter->inConsole || notAlive.count(iter->ProcessID))
				{
					m_Processes.erase(i);
					bProcessChanged = true;
				}
				else
				{
					++i;
				}
			}
		}
	}

	// Undo exclusive locks, to continue probably long operations
	if (SPRC.isLocked(TRUE))
	{
		SPRC.Unlock();
		SPRC.Lock(&csPRC);
	}

	SafeDelete(pProcData);

	// Refresh running programs status, retrieve FAR PID, count processes
	const auto farStateChanged = ProcessUpdateFlags(bProcessChanged);

	if (lbRecreateOk)
		mn_InRecreate = 0;

	return farStateChanged;
}

void CRealConsole::ProcessCheckName(struct ConProcess &ConPrc, LPWSTR asFullFileName)
{
	const auto* name = PointToName(asFullFileName);
	lstrcpynW(ConPrc.Name, name, countof(ConPrc.Name));

	ConPrc.IsMainSrv = (ConPrc.ProcessID == mn_MainSrv_PID);
	#ifdef _DEBUG
	if (ConPrc.IsMainSrv)
	{
		_ASSERTE(lstrcmpi(ConPrc.Name, _T("conemuc.exe"))==0 || lstrcmpi(ConPrc.Name, _T("conemuc64.exe"))==0);
	}
	else if (lstrcmpi(ConPrc.Name, _T("conemuc.exe"))==0 || lstrcmpi(ConPrc.Name, _T("conemuc64.exe"))==0)
	{
		_ASSERTE(mn_MainSrv_PID!=0 && "Main server PID was not determined?");
	}
	#endif

	// Тут главное не промахнуться, и не посчитать корневой conemuc, из которого запущен сам FAR, или который запустил плагин, чтобы GUI прицепился к этой консоли
	ConPrc.IsCmd = !ConPrc.IsMainSrv
			&& (lstrcmpi(ConPrc.Name, _T("cmd.exe"))==0
				|| lstrcmpi(ConPrc.Name, _T("tcc.exe"))==0
				|| lstrcmpi(ConPrc.Name, _T("conemuc.exe"))==0
				|| lstrcmpi(ConPrc.Name, _T("conemuc64.exe"))==0);

	ConPrc.IsConHost = lstrcmpi(ConPrc.Name, _T("conhost.exe"))==0
				|| lstrcmpi(ConPrc.Name, _T("System"))==0
				|| lstrcmpi(ConPrc.Name, _T("csrss.exe"))==0;
	ConPrc.IsTermSrv = IsTerminalServer(ConPrc.Name);

	ConPrc.IsFar = IsFarExe(ConPrc.Name);
	ConPrc.IsNtvdm = lstrcmpi(ConPrc.Name, _T("ntvdm.exe"))==0;
	ConPrc.IsTelnet = lstrcmpi(ConPrc.Name, _T("telnet.exe"))==0;

	if (ConPrc.IsConHost)
	{
		ConPrc.Bits = IsWindows64() ? 64 : 32;
	}

	if (!mn_ConHost_PID && ConPrc.IsConHost)
	{
		_ASSERTE(ConPrc.ProcessID!=0);
		ConHostSetPID(ConPrc.ProcessID);
	}
}

bool CRealConsole::WaitConsoleSize(int anWaitSize, DWORD nTimeout)
{
	bool lbRc = false;
	//CESERVER_REQ *pIn = nullptr, *pOut = nullptr;
	DWORD nStart = GetTickCount();
	DWORD nDelta = 0;
	//BOOL lbBufferHeight = FALSE;
	//int nNewWidth = 0, nNewHeight = 0;

	if (nTimeout > 10000) nTimeout = 10000;

	if (nTimeout == 0) nTimeout = 100;

	if (GetCurrentThreadId() == mn_MonitorThreadID)
	{
		_ASSERTE(GetCurrentThreadId() != mn_MonitorThreadID);
		return FALSE;
	}

#ifdef _DEBUG
	wchar_t szDbg[128]; swprintf_c(szDbg, L"CRealConsole::WaitConsoleSize(H=%i, Timeout=%i)\n", anWaitSize, nTimeout);
	DEBUGSTRSIZE(szDbg);
#endif
	WARNING("Вообще, команду в сервер может и не посылать? Сам справится? Просто проверять значения из FileMap");

	// Чтобы не обломаться на посылке команды в активный буфер, а ожидания - в реальном
	_ASSERTE(mp_ABuf==mp_RBuf);

	while (nDelta < nTimeout)
	{
		// Было - if (GetConWindowSize(con.m_sbi, nNewWidth, nNewHeight, &lbBufferHeight))
		if (anWaitSize == mp_RBuf->GetWindowHeight())
		{
			lbRc = TRUE;
			break;
		}

		SetMonitorThreadEvent();
		Sleep(10);
		nDelta = GetTickCount() - nStart;
	}

	_ASSERTE(lbRc && "WaitConsoleSize");
	DEBUGSTRSIZE(lbRc ? L"CRealConsole::WaitConsoleSize SUCCEEDED\n" : L"CRealConsole::WaitConsoleSize FAILED!!!\n");
	return lbRc;
}

void CRealConsole::ShowConsoleOrGuiClient(int nMode) // -1 Toggle 0 - Hide 1 - Show
{
	if (this == nullptr) return;

	// В GUI-режиме (putty, notepad, ...) CtrlWinAltSpace "переключает" привязку (делает detach/attach)
	// Но только в том случае, если НЕ включен "буферный" режим (GUI скрыто, показан текст консоли сервера)
	if (m_ChildGui.hGuiWnd && isGuiVisible())
	{
		ShowGuiClientExt(nMode);
	}
	else
	{
		ShowConsole(nMode);
	}
}

// It's called now from AskChangeBufferHeight and AskChangeAlternative
void CRealConsole::ShowGuiClientInt(bool bShow)
{
	if (bShow && m_ChildGui.hGuiWnd && IsWindow(m_ChildGui.hGuiWnd))
	{
		m_ChildGui.bGuiForceConView = false;
		ShowOtherWindow(m_ChildGui.hGuiWnd, SW_SHOW);
		ShowWindow(GetView(), SW_HIDE);
	}
	else
	{
		m_ChildGui.bGuiForceConView = true;
		ShowWindow(GetView(), SW_SHOW);
		if (m_ChildGui.hGuiWnd && IsWindow(m_ChildGui.hGuiWnd))
			ShowOtherWindow(m_ChildGui.hGuiWnd, SW_HIDE);
		mp_VCon->Invalidate();
	}
}

// The only way to show ChildGui's window system menu,
// if user's chosen to hide children window caption
void CRealConsole::ChildSystemMenu()
{
	AssertThis();
	if (!m_ChildGui.hGuiWnd)
		return;

	//Seems like we need to bring focus to ConEmu window before
	SetForegroundWindow(ghWnd);
	mp_ConEmu->setFocus();

	HMENU hSysMenu = GetSystemMenu(m_ChildGui.hGuiWnd, FALSE);
	if (hSysMenu)
	{
		POINT ptCur = {}; MapWindowPoints(mp_VCon->GetBack(), nullptr, &ptCur, 1);
		int nCmd = mp_ConEmu->mp_Menu->trackPopupMenu(tmp_ChildSysMenu, hSysMenu, TPM_LEFTALIGN|TPM_TOPALIGN|TPM_RETURNCMD|TPM_NONOTIFY,
			ptCur.x, ptCur.y, ghWnd, nullptr);
		if (nCmd)
		{
			PostConsoleMessage(m_ChildGui.hGuiWnd, WM_SYSCOMMAND, nCmd, 0);
		}
	}
}

void CRealConsole::ShowGuiClientExt(int nMode, bool bDetach /*= FALSE*/) // -1 Toggle 0 - Hide 1 - Show
{
	if (this == nullptr) return;

	// В GUI-режиме (putty, notepad, ...) CtrlWinAltSpace "переключает" привязку (делает detach/attach)
	// Но только в том случае, если НЕ включен "буферный" режим (GUI скрыто, показан текст консоли сервера)
	if (!m_ChildGui.hGuiWnd)
		return;

	if (nMode == -1)
	{
		nMode = m_ChildGui.bGuiExternMode ? 0 : 1;
	}

	bool bPrev = mp_ConEmu->SetSkipOnFocus(true);

	// Вынести Gui приложение из вкладки ConEmu (но Detach не делать)
	CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SETGUIEXTERN, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETGUIEXTERN));
	if (pIn)
	{
		AllowSetForegroundWindow(m_ChildGui.Process.ProcessID);

		//pIn->dwData[0] = (nMode == 0) ? FALSE : TRUE;
		pIn->SetGuiExtern.bExtern = (nMode == 0) ? FALSE : TRUE;
		pIn->SetGuiExtern.bDetach = bDetach;

		CESERVER_REQ *pOut = ExecuteHkCmd(m_ChildGui.Process.ProcessID, pIn, ghWnd);
		if (pOut && (pOut->DataSize() >= sizeof(DWORD)))
		{
			m_ChildGui.bGuiExternMode = (pOut->dwData[0] != 0);
		}
		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);

		if (gpSet->isLogging())
		{
			wchar_t sInfo[200];
			swprintf_c(sInfo, L"ShowGuiClientExtern: PID=%u, hGuiWnd=x%08X, bExtern=%i, bDetach=%u",
				m_ChildGui.Process.ProcessID, LODWORD(m_ChildGui.hGuiWnd), nMode, bDetach);
			mp_ConEmu->LogString(sInfo);
		}

		SetOtherWindowPos(m_ChildGui.hGuiWnd, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
	}

	mp_ConEmu->SetSkipOnFocus(bPrev);

	mp_VCon->Invalidate();
}

void CRealConsole::ShowConsole(int nMode) // -1 Toggle 0 - Hide 1 - Show
{
	if (this == nullptr) return;

	if (!hConWnd) return;

	if (nMode == -1)
	{
		//nMode = IsWindowVisible(hConWnd) ? 0 : 1;
		nMode = isShowConsole ? 0 : 1;
	}

	if (nMode == 1)
	{
		isShowConsole = true;
		//apiShowWindow(hConWnd, SW_SHOWNORMAL);
		//if (setParent) SetParent(hConWnd, 0);
		RECT rcCon, rcWnd; GetWindowRect(hConWnd, &rcCon);
		GetClientRect(GetView(), &rcWnd);
		MapWindowPoints(GetView(), nullptr, (POINT*)&rcWnd, 2);
		//GetWindowRect(ghWnd, &rcWnd);
		//RECT rcShift = mp_ConEmu->Calc Margins(CEM_STATUS|CEM_SCROLL|CEM_FRAME,mp_VCon);
		//rcWnd.right -= rcShift.right;
		//rcWnd.bottom -= rcShift.bottom;
		TODO("Скорректировать позицию так, чтобы не вылезло за экран");

		HWND hInsertAfter = HWND_TOPMOST;

		#ifdef _DEBUG
		if (gbIsWine)
			hInsertAfter = HWND_TOP;
		#endif

		if (SetOtherWindowPos(hConWnd, hInsertAfter,
			rcWnd.right-rcCon.right+rcCon.left, rcWnd.bottom-rcCon.bottom+rcCon.top,
			0,0, SWP_NOSIZE|SWP_SHOWWINDOW))
		{
			if (!IsWindowEnabled(hConWnd))
				EnableWindow(hConWnd, true); // Для админской консоли - не сработает.

			DWORD dwExStyle = GetWindowLong(hConWnd, GWL_EXSTYLE);

			#if 0
			DWORD dw1, dwErr;

			if ((dwExStyle & WS_EX_TOPMOST) == 0)
			{
				dw1 = SetWindowLong(hConWnd, GWL_EXSTYLE, dwExStyle|WS_EX_TOPMOST);
				dwErr = GetLastError();
				dwExStyle = GetWindowLong(hConWnd, GWL_EXSTYLE);

				if ((dwExStyle & WS_EX_TOPMOST) == 0)
				{
					HWND hInsertAfter = HWND_TOPMOST;

					#ifdef _DEBUG
					if (gbIsWine)
						hInsertAfter = HWND_TOP;
					#endif

					SetOtherWindowPos(hConWnd, hInsertAfter,
						0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
				}
			}
			#endif

			// Issue 246. Возвращать фокус в ConEmu можно только если удалось установить
			// "OnTop" для RealConsole, иначе - RealConsole "всплывет" на заднем плане
			if ((dwExStyle & WS_EX_TOPMOST))
				mp_ConEmu->setFocus();

			//} else { //2010-06-05 Не требуется. SetOtherWindowPos выполнит команду в сервере при необходимости
			//	if (isAdministrator() || (m_Args.pszUserName != nullptr)) {
			//		// Если оно запущено в Win7 as admin
			//        CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SHOWCONSOLE, sizeof(CESERVER_REQ_HDR) + sizeof(DWORD));
			//		if (pIn) {
			//			pIn->dwData[0] = SW_SHOWNORMAL;
			//			CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), pIn, ghWnd);
			//			if (pOut) ExecuteFreeResult(pOut);
			//			ExecuteFreeResult(pIn);
			//		}
			//	}
		}

		//if (setParent) SetParent(hConWnd, 0);
	}
	else
	{
		isShowConsole = false;
		ShowOtherWindow(hConWnd, SW_HIDE);
		////if (!gpSet->isConVisible)
		//if (!apiShowWindow(hConWnd, SW_HIDE)) {
		//	if (isAdministrator() || (m_Args.pszUserName != nullptr)) {
		//		// Если оно запущено в Win7 as admin
		//        CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SHOWCONSOLE, sizeof(CESERVER_REQ_HDR) + sizeof(DWORD));
		//		if (pIn) {
		//			pIn->dwData[0] = SW_HIDE;
		//			CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), pIn, ghWnd);
		//			if (pOut) ExecuteFreeResult(pOut);
		//			ExecuteFreeResult(pIn);
		//		}
		//	}
		//}
		////if (setParent) SetParent(hConWnd, setParent2 ? ghWnd : 'ghWnd DC');
		////if (!gpSet->isConVisible)
		////EnableWindow(hConWnd, false); -- наверное не нужно
		mp_ConEmu->setFocus();
	}
}

//void CRealConsole::CloseMapping()
//{
//	if (pConsoleData) {
//		UnmapViewOfFile(pConsoleData);
//		pConsoleData = nullptr;
//	}
//	if (hFileMapping) {
//		CloseHandle(hFileMapping);
//		hFileMapping = nullptr;
//	}
//}

// Вызывается при окончании инициализации сервера из ConEmuC.exe:SendStarted (CECMD_CMDSTARTSTOP)
void CRealConsole::OnServerStarted(DWORD anServerPID, HANDLE ahServerHandle, DWORD dwKeybLayout, bool abFromAttach /*= FALSE*/)
{
	_ASSERTE(anServerPID && (anServerPID == mn_MainSrv_PID));
	if (ahServerHandle != nullptr)
	{
		if (mh_MainSrv == nullptr)
		{
			// В принципе, это может быть, если сервер запущен "извне"
			SetMainSrvPID(mn_MainSrv_PID, ahServerHandle);
			//mh_MainSrv = ahServerHandle;
		}
		else if (ahServerHandle != mh_MainSrv)
		{
			SafeCloseHandle(ahServerHandle); // не нужен, у нас уже есть дескриптор процесса сервера
		}
		_ASSERTE(mn_MainSrv_PID == anServerPID);
	}

	if (abFromAttach || !m_ConsoleMap.IsValid())
	{
		// Инициализировать имена пайпов, событий, мэппингов и т.п.
		InitNames();
		// Открыть map с данными, теперь он уже должен быть создан
		OpenMapHeader(abFromAttach);
	}

	// И атрибуты Colorer
	// создаются по дескриптору окна отрисовки
	CreateColorMapping();

	// Возвращается через CESERVER_REQ_STARTSTOPRET
	//if ((gpSet->isMonitorConsoleLang & 2) == 2) // Один Layout на все консоли
	//	SwitchKeyboardLayout(INPUTLANGCHANGE_SYSCHARSET,mp_ConEmu->GetActiveKeyboardLayout());

	// Само разберется
	OnConsoleKeyboardLayout(dwKeybLayout);

	// Сервер готов принимать команды
	mb_MainSrv_Ready = true;
}

// Если эта функция вызвана - считаем, что консоль запустилась нормально
// И при ее закрытии не нужно оставлять висящим окно ConEmu
void CRealConsole::OnStartedSuccess()
{
	if (this)
	{
		mb_RConStartedSuccess = TRUE;
		mp_ConEmu->OnRConStartedSuccess(this);
	}
}

void CRealConsole::SetHwnd(HWND ahConWnd, bool abForceApprove /*= FALSE*/)
{
	// Окно разрушено? Пересоздание консоли?
	if (hConWnd && !IsWindow(hConWnd))
	{
		_ASSERTE(IsWindow(hConWnd));
		hConWnd = nullptr;
	}

	// Мог быть уже вызван (AttachGui/ConsoleEvent/CMD_START)
	if (hConWnd != nullptr)
	{
		if (hConWnd != ahConWnd)
		{
			if (m_ConsoleMap.IsValid())
			{
				_ASSERTE(!m_ConsoleMap.IsValid());
				//CloseMapHeader(); // вдруг был подцеплен к другому окну? хотя не должен
				// OpenMapHeader() пока не зовем, т.к. map мог быть еще не создан
			}

			Assert(hConWnd == ahConWnd);
			if (!abForceApprove)
				return;
		}
	}

	if (ahConWnd && mb_InCreateRoot)
	{
		// При запуске "под администратором" mb_InCreateRoot сразу не сбрасывается
		// При обычном запуске тоже иногда не успевает
		// -- _ASSERTE(m_Args.RunAsAdministrator == crb_On);
		mb_InCreateRoot = FALSE;
	}

	hConWnd = ahConWnd;
	CheckVConRConPointer(true);
	//if (mb_Detached && ahConWnd) // Не сбрасываем, а то нить может не успеть!
	//  mb_Detached = FALSE; // Сброс флажка, мы уже подключились
	//OpenColorMapping();
	mb_ProcessRestarted = FALSE; // Консоль запущена
	SetInCloseConsole(false);
	m_Args.Detached = crb_Off;
	m_ServerClosing = {};
	if (mn_InRecreate>=1)
		mn_InRecreate = 0; // корневой процесс успешно пересоздался

	if (ms_VConServer_Pipe[0] == 0)
	{
		// Запустить серверный пайп
		swprintf_c(ms_VConServer_Pipe, CEGUIPIPENAME, L".", LODWORD(hConWnd)); //был mn_MainSrv_PID //-V205

		m_RConServer.Start();
	}

#if 0
	ShowConsole(gpSet->isConVisible ? 1 : 0); // установить консольному окну флаг AlwaysOnTop или спрятать его
#endif

	//else if (isAdministrator())
	//	ShowConsole(0); // В Win7 оно таки появляется видимым - проверка вынесена в ConEmuC

	// Перенесено в OnServerStarted
	//if ((gpSet->isMonitorConsoleLang & 2) == 2) // Один Layout на все консоли
	//    SwitchKeyboardLayout(INPUTLANGCHANGE_SYSCHARSET,mp_ConEmu->GetActiveKeyboardLayout());

	if (isActive(false))
	{
		#ifdef _DEBUG
		ghConWnd = hConWnd; // на удаление
		#endif
		// Чтобы можно было найти хэндл окна по хэндлу консоли
		mp_ConEmu->OnActiveConWndStore(hConWnd);
		// StatusBar
		mp_ConEmu->mp_Status->OnActiveVConChanged(mp_ConEmu->ActiveConNum(), this);
	}
}

void CRealConsole::CheckVConRConPointer(bool bForceSet)
{
	AssertThis();

	_ASSERTE(hConWnd != nullptr);
	HWND hVCon = mp_VCon->GetView();
	HWND hVConBack = mp_VCon->GetBack();

	HWND hCurrent = (HWND)INVALID_HANDLE_VALUE;
	if (!bForceSet)
	{
		hCurrent = (HWND)GetWindowLongPtr(hVCon, WindowLongDCWnd_ConWnd);
		if (hCurrent == hConWnd)
			return; // OK, was not changed externally
		if (isServerClosing())
			return; // Skip - server is already in the shutdown state
		// Don't warn in "Inside" mode
		_ASSERTE(mp_ConEmu->mp_Inside && "WindowLongPtr was changed externally?");
	}

	SetWindowLongPtr(hVCon, WindowLongDCWnd_ConWnd, (LONG_PTR)hConWnd);

	SetWindowLong(hVConBack, WindowLongBack_ConWnd, LODWORD(hConWnd));
	SetWindowLong(hVConBack, WindowLongBack_DCWnd, LODWORD(hVCon));
}

void CRealConsole::OnFocus(bool abFocused)
{
	AssertThis();

	if ((mn_Focused == -1) ||
	        ((mn_Focused == 0) && abFocused) ||
	        ((mn_Focused == 1) && !abFocused))
	{
		#ifdef _DEBUG
		if (abFocused)
		{
			DEBUGSTRFOCUS(L"--Gets focus");
		}
		else
		{
			DEBUGSTRFOCUS(L"--Loses focus");
		}
		#endif

		// Сразу, иначе по окончании PostConsoleEvent RCon может разрушиться?
		mn_Focused = abFocused ? 1 : 0;

		if (m_ServerClosing.nServerPID
			&& m_ServerClosing.nServerPID == mn_MainSrv_PID)
		{
			return;
		}

		INPUT_RECORD r = {FOCUS_EVENT};
		r.Event.FocusEvent.bSetFocus = abFocused;
		PostConsoleEvent(&r);
	}
}

void CRealConsole::CreateLogFiles()
{
	if (!isLogging() || !mn_MainSrv_PID)
	{
		return;
	}

	if (!mp_Log)
	{
		mp_Log = new MFileLog(L"ConEmu-con", mp_ConEmu->ms_ConEmuExeDir, mn_MainSrv_PID);
	}

	wchar_t szInfo[MAX_PATH * 2];

	HRESULT hr = mp_Log ? mp_Log->CreateLogFile(L"ConEmu-con", mn_MainSrv_PID, gpSet->isLogging()) : E_UNEXPECTED;
	if (hr != 0)
	{
		swprintf_c(szInfo, L"Create log file failed! ErrCode=0x%08X\n%s\n", (DWORD)hr, mp_Log->GetLogFileName());
		MBoxA(szInfo);
		SafeDelete(mp_Log);
		return;
	}

	swprintf_c(szInfo, L"RCon ID=%i started", mp_VCon->ID());
	LogString(szInfo);

	LPCWSTR pszCommand = GetCmd(true);
	LogString((pszCommand && *pszCommand) ? pszCommand : L"<No Command>");
}

unsigned CRealConsole::isLogging(unsigned level /*= 1*/)
{
	unsigned logLevel = gpSet->isLogging();
	if (!logLevel)
		return 0;
	return (logLevel && (level <= logLevel)) ? logLevel : 0;
}

bool CRealConsole::LogString(LPCWSTR asText)
{
	AssertThisRet(false);

	if (!asText) return false;

	if (mp_Log)
	{
		wchar_t szTID[32]; swprintf_c(szTID, L"[%u]", GetCurrentThreadId());
		mp_Log->LogString(asText, true, szTID);
		return true;
	}
	else
	{
		return mp_ConEmu->LogString(asText);
	}
}

bool CRealConsole::LogString(LPCSTR asText)
{
	AssertThisRet(false);

	if (!asText) return false;

	if (mp_Log)
	{
		char szTID[32]; sprintf_c(szTID, "[%u]", GetCurrentThreadId());
		mp_Log->LogString(asText, true, szTID);
		return true;
	}
	else
	{
		return mp_ConEmu->LogString(asText);
	}
}

bool CRealConsole::LogInput(UINT uMsg, WPARAM wParam, LPARAM lParam, LPCWSTR pszTranslatedChars /*= nullptr*/)
{
	AssertThisRet(false);
	// Есть еще вообще-то и WM_UNICHAR, но ввод UTF-32 у нас пока не поддерживается
	if (!mp_Log || !isLogging())
		return false;
	if (!(uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR
		|| uMsg == WM_SYSCHAR || uMsg == WM_DEADCHAR || uMsg == WM_SYSDEADCHAR
		|| uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP
		|| (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)))
		return false;
	if ((uMsg == WM_MOUSEMOVE) && !isLogging(2))
		return false;

	char szInfo[192] = {0};
	SYSTEMTIME st; GetLocalTime(&st);
	sprintf_c(szInfo, "%i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	char *pszAdd = szInfo+strlen(szInfo);

	if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR
		|| uMsg == WM_SYSCHAR || uMsg == WM_SYSDEADCHAR  || uMsg == WM_DEADCHAR
		|| uMsg == WM_SYSKEYDOWN || uMsg == WM_SYSKEYUP)
	{
		char chUtf8[64] = "";
		if (pszTranslatedChars && (*pszTranslatedChars >= 32))
		{
			chUtf8[0] = L'"';
			WideCharToMultiByte(CP_UTF8, 0, pszTranslatedChars, -1, chUtf8+1, countof(chUtf8)-3, 0,0);
			lstrcatA(chUtf8, "\"");
		}
		else if (uMsg == WM_CHAR || uMsg == WM_SYSCHAR || uMsg == WM_DEADCHAR)
		{
			chUtf8[0] = L'"';
			switch ((WORD)wParam)
			{
			case L'\r':
				chUtf8[1] = L'\\'; chUtf8[2] = L'r';
				break;
			case L'\n':
				chUtf8[1] = L'\\'; chUtf8[2] = L'n';
				break;
			case L'\t':
				chUtf8[1] = L'\\'; chUtf8[2] = L't';
				break;
			default:
				WideCharToMultiByte(CP_UTF8, 0, (wchar_t*)&wParam, 1, chUtf8+1, countof(chUtf8)-3, 0,0);
			}
			lstrcatA(chUtf8, "\"");
		}
		else
		{
			lstrcpynA(chUtf8, "\"\" ", 5);
		}
		/* */ _wsprintfA(pszAdd, SKIPLEN(countof(szInfo)-(pszAdd-szInfo))
						 "%s %s wParam=x%08X, lParam=x%08X\r\n",
						 (uMsg == WM_KEYDOWN) ? "WM_KEYDOWN" :
						 (uMsg == WM_KEYUP)   ? "WM_KEYUP  " :
						 (uMsg == WM_CHAR) ? "WM_CHAR" :
						 (uMsg == WM_SYSCHAR) ? "WM_SYSCHAR" :
						 (uMsg == WM_DEADCHAR) ? "WM_DEADCHAR" :
						 (uMsg == WM_SYSDEADCHAR) ? "WM_SYSDEADCHAR" :
						 (uMsg == WM_SYSKEYDOWN) ? "WM_SYSKEYDOWN" :
						 (uMsg == WM_SYSKEYUP) ? "WM_SYSKEYUP" : "???",
						 chUtf8,
						 (DWORD)wParam, (DWORD)lParam);
	}
	else if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
	{
		/* */ _wsprintfA(pszAdd, SKIPLEN(countof(szInfo)-(pszAdd-szInfo))
						 "x%04X (%u) wParam=x%08X, lParam=x%08X\r\n",
						 uMsg, uMsg,
						 (DWORD)wParam, (DWORD)lParam);
	}
	else
	{
		_ASSERTE(FALSE && "Unknown message");
		return false;
	}

	if (*pszAdd)
	{
		mp_Log->LogString(szInfo, false, nullptr, false);
		//DWORD dwLen = 0;
		//WriteFile(mh_LogInput, szInfo, strlen(szInfo), &dwLen, 0);
		//FlushFileBuffers(mh_LogInput);
		return true;
	}

	return false;
}

bool CRealConsole::LogInput(INPUT_RECORD* pRec)
{
	AssertThisRet(false);
	if (!mp_Log || !isLogging())
		return false;

	char szInfo[192] = {0};
	SYSTEMTIME st; GetLocalTime(&st);
	sprintf_c(szInfo, "%i:%02i:%02i.%03i ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
	char *pszAdd = szInfo+strlen(szInfo);

	switch (pRec->EventType)
	{
			/*case FOCUS_EVENT: _wsprintfA(pszAdd, countof(szInfo)-(pszAdd-szInfo), "FOCUS_EVENT(%i)\r\n", pRec->Event.FocusEvent.bSetFocus);
				break;*/
		case MOUSE_EVENT: //sprintf_c(pszAdd, countof(szInfo)-(pszAdd-szInfo)/*#SECURELEN*/, "MOUSE_EVENT\r\n");
			{
				if (!isLogging(2) && (pRec->Event.MouseEvent.dwEventFlags == MOUSE_MOVED))
					return false; // Движения мышки логировать только при /log2

				sprintf_s(pszAdd, countof(szInfo)-(pszAdd-szInfo),
				           "Mouse: {%ix%i} Btns:{", pRec->Event.MouseEvent.dwMousePosition.X, pRec->Event.MouseEvent.dwMousePosition.Y);
				pszAdd += strlen(pszAdd);

				if (pRec->Event.MouseEvent.dwButtonState & 1) strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "L");

				if (pRec->Event.MouseEvent.dwButtonState & 2) strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "R");

				if (pRec->Event.MouseEvent.dwButtonState & 4) strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "M1");

				if (pRec->Event.MouseEvent.dwButtonState & 8) strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "M2");

				if (pRec->Event.MouseEvent.dwButtonState & 0x10) strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "M3");

				pszAdd += strlen(pszAdd);

				if (pRec->Event.MouseEvent.dwButtonState & 0xFFFF0000)
				{
					sprintf_s(pszAdd, countof(szInfo)-(pszAdd-szInfo),
					           "x%04X", (pRec->Event.MouseEvent.dwButtonState & 0xFFFF0000)>>16);
				}

				strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "} "); pszAdd += strlen(pszAdd);
				_wsprintfA(pszAdd, SKIPLEN(countof(szInfo)-(pszAdd-szInfo))
				           "KeyState: 0x%08X ", pRec->Event.MouseEvent.dwControlKeyState);

				if (pRec->Event.MouseEvent.dwEventFlags & 0x01) strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "|MOUSE_MOVED");

				if (pRec->Event.MouseEvent.dwEventFlags & 0x02) strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "|DOUBLE_CLICK");

				if (pRec->Event.MouseEvent.dwEventFlags & 0x04) strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "|MOUSE_WHEELED"); //-V112

				if (pRec->Event.MouseEvent.dwEventFlags & 0x08) strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "|MOUSE_HWHEELED");

				strcat_s(pszAdd, countof(szInfo)-(pszAdd-szInfo), "\r\n");
			} break;
		case KEY_EVENT:
		{
			char chUtf8[4] = " ";
			if (pRec->Event.KeyEvent.uChar.UnicodeChar >= 32)
				WideCharToMultiByte(CP_UTF8, 0, &pRec->Event.KeyEvent.uChar.UnicodeChar, 1, chUtf8, countof(chUtf8), 0,0);

			sprintf_s(pszAdd, countof(szInfo)-(pszAdd-szInfo),
			                 "%s (\\x%04X) %s count=%i, VK=%i, SC=%i, CH=%i, State=0x%08x %s\r\n",
			                 chUtf8, pRec->Event.KeyEvent.uChar.UnicodeChar,
			                 pRec->Event.KeyEvent.bKeyDown ? "Down," : "Up,  ",
			                 pRec->Event.KeyEvent.wRepeatCount,
			                 pRec->Event.KeyEvent.wVirtualKeyCode,
			                 pRec->Event.KeyEvent.wVirtualScanCode,
			                 pRec->Event.KeyEvent.uChar.UnicodeChar,
			                 pRec->Event.KeyEvent.dwControlKeyState,
			                 (pRec->Event.KeyEvent.dwControlKeyState & ENHANCED_KEY) ?
			                 "<Enhanced>" : "");
		} break;
	}

	if (*pszAdd)
	{
		mp_Log->LogString(szInfo, false, nullptr, false);
		//DWORD dwLen = 0;
		//WriteFile(mh_LogInput, szInfo, strlen(szInfo), &dwLen, 0);
		//FlushFileBuffers(mh_LogInput);
		return true;
	}

	return false;
}

void CRealConsole::CloseLogFiles()
{
	SafeDelete(mp_Log);
}

void CRealConsole::PrepareNewConArgs()
{
	if (m_Args.BufHeight == crb_On)
	{
		mn_DefaultBufferHeight = m_Args.nBufHeight;
		mp_RBuf->SetBufferHeightMode(mn_DefaultBufferHeight>0);
	}
}

// Послать в консоль запрос на закрытие
bool CRealConsole::RecreateProcess(RConStartArgsEx *args)
{
	AssertThisRet(false);

	_ASSERTE((hConWnd && mh_MainSrv) || isDetached());

	if ((!hConWnd || !mh_MainSrv) && !isDetached())
	{
		AssertMsg(L"Console was not created (CRealConsole::SetConsoleSize)");
		return false; // консоль пока не создана?
	}

	if (mn_InRecreate && !mb_RecreateFailed)
	{
		AssertMsg(L"Console already in recreate...");
		return false;
	}

	if (!args)
	{
		AssertMsg(L"args were not specified!");
		return false;
	}

	if (args->pszSpecialCmd && *args->pszSpecialCmd)
	{
		if (mp_ConEmu->IsConsoleBatchOrTask(args->pszSpecialCmd))
		{
			// Load Task contents
			CEStr pszTaskCommands = mp_ConEmu->LoadConsoleBatch(args->pszSpecialCmd, args);
			if (pszTaskCommands.IsEmpty())
			{
				CEStr lsMsg(L"Can't load task contents!\n", args->pszSpecialCmd);
				MsgBox(lsMsg, MB_ICONSTOP);
				return false;
			}
			// Only one command can started in a console
			wchar_t* pszBreak = wcspbrk(pszTaskCommands.data(), L"\r\n");
			if (pszBreak)
			{
				CEStr lsMsg(L"Task ", args->pszSpecialCmd, L" contains more than a command.\n" L"Only first will be executed.");
				int iBtn = MsgBox(lsMsg, MB_ICONEXCLAMATION|MB_OKCANCEL);
				if (iBtn != IDOK)
					return false;
				*pszBreak = 0;
			}
			// Run contents but not a "task"
			SafeFree(args->pszSpecialCmd);
			args->pszSpecialCmd = pszTaskCommands.Detach();
		}
	}

	_ASSERTE(m_Args.pszStartupDir==nullptr || (m_Args.pszStartupDir && args->pszStartupDir));
	SafeFree(m_Args.pszStartupDir);

	if (isLogging())
	{
		wchar_t szPrefix[128];
		swprintf_c(szPrefix, L"CRealConsole::RecreateProcess, hView=x%08X, Detached=%u, AsAdmin=%u, Cmd=",
			LODWORD(mp_VCon->GetView()), static_cast<UINT>(args->Detached), static_cast<UINT>(args->RunAsAdministrator));
		const CEStr pszInfo(szPrefix, args->pszSpecialCmd ? args->pszSpecialCmd : L"<nullptr>");
		LogString(pszInfo.c_str(szPrefix));
	}

	const bool bCopied = m_Args.AssignFrom(*args, true);

	// Don't leave security information (passwords) in memory
	if (args->pszUserName)
	{
		SecureZeroMemory(args->szUserPassword, sizeof(args->szUserPassword));
	}

	if (!bCopied)
	{
		AssertMsg(L"Failed to copy args (CRealConsole::RecreateProcess)");
		return false;
	}

	m_Args.ProcessNewConArg();

	PrepareNewConArgs();

	// #CLASSES Add OnRecreate event?

	mb_ProcessRestarted = FALSE;
	mn_InRecreate = GetTickCount();
	CloseConfirmReset();

	if (!mn_InRecreate)
	{
		DisplayLastError(L"GetTickCount failed");
		return false;
	}

	if (isDetached())
	{
		_ASSERTE(mn_InRecreate && mn_ProcessCount == 0 && !mb_ProcessRestarted);
		RecreateProcessStart();
	}
	else
	{
		CloseConsole(false, false);
	}
	// mb_InCloseConsole сбросим после того, как появится новое окно!
	//SetInCloseConsole(false);
	//if (con.pConChar && con.pConAttr)
	//{
	//	wmemset((wchar_t*)con.pConAttr, 7, con.nTextWidth * con.nTextHeight);
	//}

	CloseConfirmReset();
	SetConStatus(L"Restarting process...", cso_Critical);
	return true;
}

void CRealConsole::UpdateStartState(RConStartState state, bool force /*= false*/)
{
	MSectionLockSimple lock(m_StartStateCS);
	if (force || (m_StartState < state))
		m_StartState = state;
}

void CRealConsole::RequestStartup(bool bForce)
{
	AssertThis();
	// Created as detached?
	if (bForce)
	{
		mb_NeedStartProcess = true;
		mb_WasStartDetached = false;
		m_Args.Detached = crb_Off;
	}
	// Push request to "startup queue"
	UpdateStartState(rss_StartupRequested);
	mp_ConEmu->mp_RunQueue->RequestRConStartup(this);
}

// И запустить ее заново
bool CRealConsole::RecreateProcessStart()
{
	bool lbRc = false;


	if ((mn_InRecreate == 0) || (mn_ProcessCount != 0) || mb_ProcessRestarted)
	{
		_ASSERTE(FALSE && "Must not be called twice, while Restart is still pending, or was not prepared");
	}
	else
	{
		mb_ProcessRestarted = TRUE;

		bool bWasNTVDM = ((mn_ProgramStatus & CES_NTVDM) == CES_NTVDM);

		SetProgramStatus((DWORD)-1, 0);
		mb_IgnoreCmdStop = FALSE;

		if (bWasNTVDM)
		{
			// При пересоздании сбрасывается 16битный режим, нужно отресайзиться
			if (!PreInit())
				return false;
		}

		StopThread(TRUE/*abRecreate*/);
		ResetEvent(mh_TermEvent);
		mn_TermEventTick = 0;

		//Moved to function
		ResetVarsOnStart();

		ms_VConServer_Pipe[0] = 0;
		m_RConServer.Stop();

		// Superfluous? Or may be changed in other threads?
		ResetEvent(mh_TermEvent);
		mn_TermEventTick = 0;

		ResetEvent(mh_StartExecuted);

		mb_NeedStartProcess = TRUE;
		mb_StartResult = FALSE;

		// Взведем флажочек, т.к. консоль как бы отключилась от нашего VCon
		mb_WasStartDetached = TRUE;
		m_Args.Detached = crb_On;

		// Push request to "startup queue"
		RequestStartup();

		lbRc = StartMonitorThread();

		if (!lbRc)
		{
			mb_NeedStartProcess = FALSE;
			mn_InRecreate = 0;
			mb_ProcessRestarted = FALSE;
		}

	}

	return lbRc;
}

bool CRealConsole::IsConsoleDataChanged()
{
	AssertThisRet(false);

	#ifdef _DEBUG
	if (mb_DebugLocked)
		return false;
	#endif

	WARNING("После смены буфера - тоже вернуть TRUE!");

	return mb_ABufChaged || mp_ABuf->isConsoleDataChanged();
}

bool CRealConsole::IsFarHyperlinkAllowed(bool abFarRequired)
{
	if (!gpSet->isFarGotoEditor)
		return false;
	//if (gpSet->isFarGotoEditorVk && !isPressed(gpSet->isFarGotoEditorVk))
	if (!gpSet->IsModifierPressed(vkFarGotoEditorVk, true))
		return false;

	// Для открытия гиперссылки (http, email) фар не требуется
	if (abFarRequired)
	{
		// А вот чтобы открыть в редакторе файл - нужен фар и плагин
		if (!CVConGroup::isFarExist(fwt_NonModal|fwt_PluginRequired))
		{
			return false;
		}
	}

	// Мышка должна быть в пределах окна, иначе фигня получается
	POINT ptCur = {-1,-1};
	GetCursorPos(&ptCur);
	RECT rcWnd = {};
	GetWindowRect(this->GetView(), &rcWnd);
	if (!PtInRect(&rcWnd, ptCur))
		return false;
	// Можно
	return true;
}

// nWidth и nHeight это размеры, которые хочет получить VCon (оно могло еще не среагировать на изменения?
void CRealConsole::GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, ConEmuTextRange& etr)
{
	AssertThis();

	if (mb_ABufChaged)
		mb_ABufChaged = false; // сбросим

	mp_ABuf->GetConsoleData(pChar, pAttr, nWidth, nHeight, etr);
}

bool CRealConsole::SetFullScreen()
{
	AssertThisRet(false);
	const DWORD nServerPID = GetServerPID();
	if (nServerPID == 0)
		return false;

	COORD crNewSize = {};
	bool lbRc = false;
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SETFULLSCREEN, sizeof(CESERVER_REQ_HDR));
	if (pIn)
	{
		CESERVER_REQ* pOut = ExecuteSrvCmd(nServerPID, pIn, ghWnd);
		if (pOut && pOut->DataSize() >= sizeof(CESERVER_REQ_FULLSCREEN))
		{
			lbRc = (pOut->FullScreenRet.bSucceeded != 0);
			if (lbRc)
				crNewSize = pOut->FullScreenRet.crNewSize;
		}
		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
	}
	TODO("crNewSize");
	return lbRc;
}

//#define PICVIEWMSG_SHOWWINDOW (WM_APP + 6)
//#define PICVIEWMSG_SHOWWINDOW_KEY 0x0101
//#define PICVIEWMSG_SHOWWINDOW_ASC 0x56731469

bool CRealConsole::ShowOtherWindow(HWND hWnd, int swShow, bool abAsync/*=TRUE*/)
{
	if ((IsWindowVisible(hWnd) == FALSE) == (swShow == SW_HIDE))
		return true; // уже все сделано

	bool lbRc = false;

	// Вероятность зависания, поэтому кидаем команду в пайп
	//lbRc = apiShowWindow(hWnd, swShow);
	//
	//lbRc = ((IsWindowVisible(hWnd) == FALSE) == (swShow == SW_HIDE));
	//
	////if (!lbRc)
	//{
	//	DWORD dwErr = GetLastError();
	//
	//	if (dwErr == 0)
	//	{
	//		if ((IsWindowVisible(hWnd) == FALSE) == (swShow == SW_HIDE))
	//			lbRc = TRUE;
	//		else
	//			dwErr = 5; // попробовать через сервер
	//	}
	//
	//	if (dwErr == 5 /*E_access*/)
		{
			//PostConsoleMessage(hWnd, WM_SHOWWINDOW, SW_SHOWNA, 0);
			CESERVER_REQ in;
			ExecutePrepareCmd(&in, CECMD_POSTCONMSG, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_POSTMSG));
			// Собственно, аргументы
			in.Msg.bPost = abAsync;
			in.Msg.hWnd = hWnd;
			in.Msg.nMsg = WM_SHOWWINDOW;
			in.Msg.wParam = swShow; //SW_SHOWNA;
			in.Msg.lParam = 0;

			DWORD dwTickStart = timeGetTime();

			CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), &in, ghWnd);

			CSetPgDebug::debugLogCommand(&in, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

			if (pOut) ExecuteFreeResult(pOut);

			lbRc = TRUE;
		}
		//else if (!lbRc)
		//{
		//	wchar_t szClass[64], szMessage[255];
		//
		//	if (!GetClassName(hWnd, szClass, 63))
		//		swprintf_c(szClass, L"0x%08X", (DWORD)hWnd); else szClass[63] = 0;
		//
		//	swprintf_c(szMessage, L"Can't %s %s window!",
		//	          (swShow == SW_HIDE) ? L"hide" : L"show",
		//	          szClass);
		//	DisplayLastError(szMessage, dwErr);
		//}
	//}

	return lbRc;
}

bool CRealConsole::SetOtherWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
	#ifdef _DEBUG
	if (!hWnd || !IsWindow(hWnd))
	{
		_ASSERTE(FALSE && "SetOtherWindowPos(<InvalidHWND>)");
	}
	#endif

	if (gpSet->isLogging())
	{
		wchar_t sInfo[200];
		swprintf_c(sInfo, L"SetOtherWindowPos: hWnd=x%08X, hInsertAfter=x%08X, X=%i, Y=%i, CX=%i, CY=%i, Flags=x%04X",
			LODWORD(hWnd), LODWORD(hWndInsertAfter), X,Y,cx,cy, uFlags);
		mp_ConEmu->LogString(sInfo);
	}

	bool lbRc = false; DWORD dwErr = ERROR_ACCESS_DENIED/*5*/;
	// It'll be better to show console window from server threads
	if (hWnd == this->hConWnd)
	{
		lbRc = (SetWindowPos(hWnd, hWndInsertAfter, X, Y, cx, cy, uFlags) != FALSE);
		dwErr = GetLastError();
	}

	if (!lbRc)
	{
		if (dwErr == ERROR_ACCESS_DENIED/*5*/)
		{
			CESERVER_REQ in;
			ExecutePrepareCmd(&in, CECMD_SETWINDOWPOS, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETWINDOWPOS));
			// Собственно, аргументы
			in.SetWndPos.hWnd = hWnd;
			in.SetWndPos.hWndInsertAfter = hWndInsertAfter;
			in.SetWndPos.X = X;
			in.SetWndPos.Y = Y;
			in.SetWndPos.cx = cx;
			in.SetWndPos.cy = cy;
			in.SetWndPos.uFlags = uFlags;

			DWORD dwTickStart = timeGetTime();

			CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(true), &in, ghWnd, TRUE/*Async*/);

			CSetPgDebug::debugLogCommand(&in, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

			if (pOut) ExecuteFreeResult(pOut);

			lbRc = TRUE;
		}
		else
		{
			wchar_t szClass[64], szMessage[128];

			if (!GetClassName(hWnd, szClass, 63))
				swprintf_c(szClass, L"0x%08X", LODWORD(hWnd));
			else
				szClass[63] = 0;

			swprintf_c(szMessage, L"SetWindowPos(%s) failed!", szClass);
			DisplayLastError(szMessage, dwErr);
		}
	}

	return lbRc;
}

bool CRealConsole::SetOtherWindowFocus(HWND hWnd, bool abSetForeground)
{
	bool lbRc = false;
	DWORD dwErr = 0;
	HWND hLastFocus = nullptr;

	if (!((m_Args.RunAsAdministrator == crb_On) || m_Args.pszUserName || (m_Args.RunAsRestricted == crb_On)/*?*/))
	{
		if (abSetForeground)
		{
			lbRc = apiSetForegroundWindow(hWnd);
		}
		else
		{
			SetLastError(0);
			hLastFocus = SetFocus(hWnd);
			dwErr = GetLastError();
			lbRc = (dwErr == 0 /* != ERROR_ACCESS_DENIED {5}*/);
		}
	}
	else
	{
		lbRc = apiSetForegroundWindow(hWnd);
	}

	// -- смысла нет, не работает
	//if (!lbRc)
	//{
	//	CESERVER_REQ in;
	//	ExecutePrepareCmd(&in, CECMD_SETFOCUS, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETFOCUS));
	//	// Собственно, аргументы
	//	in.setFocus.bSetForeground = abSetForeground;
	//	in.setFocus.hWindow = hWnd;
	//	CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), &in, ghWnd);
	//	if (pOut) ExecuteFreeResult(pOut);
	//	lbRc = TRUE;
	//}

	UNREFERENCED_PARAMETER(hLastFocus);

	return lbRc;
}

HWND CRealConsole::SetOtherWindowParent(HWND hWnd, HWND hParent)
{
	HWND h = nullptr;
	DWORD dwErr = 0;

	SetLastError(0);
	h = SetParent(hWnd, hParent);
	if (h == nullptr)
		dwErr = GetLastError();

	if (dwErr)
	{
		CESERVER_REQ in;
		ExecutePrepareCmd(&in, CECMD_SETPARENT, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETPARENT));
		// Собственно, аргументы
		in.setParent.hWnd = hWnd;
		in.setParent.hParent = hParent;

		DWORD dwTickStart = timeGetTime();

		CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), &in, ghWnd);

		CSetPgDebug::debugLogCommand(&in, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

		if (pOut)
		{
			h = pOut->setParent.hParent;
			ExecuteFreeResult(pOut);
		}
	}

	return h;
}

bool CRealConsole::SetOtherWindowRgn(HWND hWnd, int nRects, LPRECT prcRects, bool bRedraw)
{
	bool lbRc = false;
	CESERVER_REQ in;
	ExecutePrepareCmd(&in, CECMD_SETWINDOWRGN, sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_SETWINDOWRGN));
	// Собственно, аргументы
	in.SetWndRgn.hWnd = hWnd;

	if (nRects <= 0 || !prcRects)
	{
		_ASSERTE(nRects==0 || nRects==-1); // -1 means reset rgn and hide window
		in.SetWndRgn.nRectCount = nRects;
		in.SetWndRgn.bRedraw = bRedraw;
	}
	else
	{
		in.SetWndRgn.nRectCount = nRects;
		in.SetWndRgn.bRedraw = bRedraw;
		memmove(in.SetWndRgn.rcRects, prcRects, nRects*sizeof(RECT));
	}

	DWORD dwTickStart = timeGetTime();

	CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), &in, ghWnd);

	CSetPgDebug::debugLogCommand(&in, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

	if (pOut) ExecuteFreeResult(pOut);

	lbRc = true;
	return lbRc;
}

void CRealConsole::ShowHideViews(bool abShow)
{
	// Все окна теперь создаются в дочернем окне, и скрывается именно оно, этого достаточно
#if 0
	// т.к. apiShowWindow обломается, если окно создано от имени другого пользователя (или Run as admin)
	// то скрытие и отображение окна делаем другим способом
	HWND hPic = isPictureView();

	if (hPic)
	{
		if (abShow)
		{
			if (mb_PicViewWasHidden && !IsWindowVisible(hPic))
				ShowOtherWindow(hPic, SW_SHOWNA);

			mb_PicViewWasHidden = FALSE;
		}
		else
		{
			mb_PicViewWasHidden = TRUE;
			ShowOtherWindow(hPic, SW_HIDE);
		}
	}

	for(int p = 0; p <= 1; p++)
	{
		const PanelViewInit* pv = mp_VCon->GetPanelView(p==0);

		if (pv)
		{
			if (abShow)
			{
				if (pv->bVisible && !IsWindowVisible(pv->hWnd))
					ShowOtherWindow(pv->hWnd, SW_SHOWNA);
			}
			else
			{
				if (IsWindowVisible(pv->hWnd))
					ShowOtherWindow(pv->hWnd, SW_HIDE);
			}
		}
	}
#endif
}

void CRealConsole::OnActivate(int nNewNum, int nOldNum)
{
	AssertThis();

	wchar_t szInfo[120];
	swprintf_c(szInfo, L"RCon was activated Index=%i OldIndex=%i", nNewNum+1, nOldNum+1);
	if (gpSet->isLogging()) { LogString(szInfo); } else { DEBUGSTRSEL(szInfo); }

	_ASSERTE(isActive(false));
	// Чтобы можно было найти хэндл окна по хэндлу консоли
	mp_ConEmu->OnActiveConWndStore(hConWnd);

	// Чтобы не мигать "измененными" консолями при старте
	mb_WasVisibleOnce = true;
	mn_DeactivateTick = 0;

	// Keyboard reaction counter
	gpSetCls->Performance(tPerfKeyboard, (BOOL)-1);

	// Чтобы корректно таб для группы показывать
	CVConGroup::OnConActivated(mp_VCon);

	#ifdef _DEBUG
	ghConWnd = hConWnd; // на удаление
	#endif
	// Проверить
	mp_VCon->OnAlwaysShowScrollbar();
	// Чтобы все в одном месте было
	OnGuiFocused(TRUE, TRUE);

	// If there are no other splits - nothing to invalidate
	if (CVConGroup::isGroup(mp_VCon))
	{
		mp_ConEmu->InvalidateGaps();
	}

	mp_ConEmu->mp_Status->OnActiveVConChanged(nNewNum, this);

	mp_ConEmu->Taskbar_UpdateOverlay();

	if (m_ChildGui.hGuiWnd && !m_ChildGui.bGuiExternMode)
	{
		// SyncConsole2Window, SyncGui2Window
		mp_ConEmu->OnSize();
	}

	//if (mh_MonitorThread) SetThreadPriority(mh_MonitorThread, THREAD_PRIORITY_ABOVE_NORMAL);

	if ((gpSet->isMonitorConsoleLang & 2) == 2)
	{
		// Один Layout на все консоли
		// Но нет смысла дергать сервер, если в нем и так выбран "правильный" layout
		if (mp_ConEmu->GetActiveKeyboardLayout() != mp_RBuf->GetKeybLayout())
		{
			SwitchKeyboardLayout(INPUTLANGCHANGE_SYSCHARSET,mp_ConEmu->GetActiveKeyboardLayout());
		}
	}
	else if (mp_RBuf->GetKeybLayout() && (gpSet->isMonitorConsoleLang & 1) == 1)
	{
		// Следить за Layout'ом в консоли
		mp_ConEmu->SwitchKeyboardLayout(mp_RBuf->GetKeybLayout());
	}

	WARNING("Не работало обновление заголовка");
	mp_ConEmu->UpdateTitle();
	UpdateScrollInfo();
	mp_ConEmu->mp_TabBar->OnConsoleActivated(nNewNum/*, isBufferHeight()*/);
	mp_ConEmu->mp_TabBar->Update();
	// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
	OnBufferHeight();
	mp_ConEmu->UpdateProcessDisplay(TRUE);
	//gpSet->NeedBackgroundUpdate(); -- 111105 плагиновые подложки теперь в VCon, а файловая - все равно общая, дергать не нужно
	ShowHideViews(TRUE);
	//HWND hPic = isPictureView();
	//if (hPic && mb_PicViewWasHidden) {
	//	if (!IsWindowVisible(hPic)) {
	//		if (!apiShowWindow(hPic, SW_SHOWNA)) {
	//			DisplayLastError(L"Can't show PictireView window!");
	//		}
	//	}
	//}
	//mb_PicViewWasHidden = FALSE;

	if (ghOpWnd && isActive(false))
		gpSetCls->UpdateConsoleMode(this);

	if (isActive(false))
	{
		mp_ConEmu->GetGlobalHotkeys().UpdateActiveGhost(mp_VCon);
		mp_ConEmu->OnSetCursor(-1,-1);
		mp_ConEmu->UpdateWindowRgn();
	}
}

void CRealConsole::OnDeactivate(int nNewNum)
{
	AssertThis();

	const MWnd hFore = GetForegroundWindow();
	const MWnd hGui = mp_VCon->GuiWnd();
	if (hGui)
		GuiWndFocusStore();

	mp_VCon->SavePaneSnapshot();

	ShowHideViews(FALSE);

	OnGuiFocused(FALSE);

	if ((hFore == ghWnd) // фокус был в ConEmu
		|| (m_ChildGui.isGuiWnd() && (hFore == m_ChildGui.hGuiWnd))) // или в дочернем приложении
	{
		mp_ConEmu->setFocus();
	}

	mn_DeactivateTick = GetTickCount();
}

void CRealConsole::OnGuiFocused(bool abFocus, bool abForceChild /*= FALSE*/)
{
	AssertThis();

	if (!abFocus)
		mp_VCon->RestoreChildFocusPending(false);

	if (m_ChildGui.bInSetFocus)
	{
		#ifdef _DEBUG
		wchar_t szInfo[128];
		swprintf_c(szInfo,
			L"CRealConsole::OnGuiFocused(%u) skipped, mb_InSetFocus=1, mb_LastConEmuFocusState=%u)",
			abFocus, mp_ConEmu->mb_LastConEmuFocusState);
		DEBUGSTRFOCUS(szInfo);
		#endif
		return;
	}

	MSetter lSet(&m_ChildGui.bInSetFocus);

	if (abFocus)
	{
		mp_VCon->OnTaskbarFocus();

		if (m_ChildGui.hGuiWnd)
		{
			if (abForceChild)
			{
				#ifdef _DEBUG
				HWND hFore = getForegroundWindow();
				DWORD nForePID = 0;
				if (hFore) GetWindowThreadProcessId(hFore, &nForePID);
				// if (m_ChildGui.Process.ProcessID != nForePID) { }
				#endif

				GuiNotifyChildWindow();

				// Зовем Post-ом, т.к. сейчас таб может быть еще не активирован, а в процессе...
				mp_VCon->PostRestoreChildFocus();
			}

			gpConEmu->SetFrameActiveState(true);
		}
		else
		{
			// -- От этого кода одни проблемы - например, не активируется диалог Settings щелчком по TaskBar-у
			// mp_ConEmu->setFocus();
		}
	}

	if (!abFocus)
	{
		if (mp_ConEmu->isMeForeground(true, true))
		{
			abFocus = TRUE;
			DEBUGSTRFOCUS(L"CRealConsole::OnGuiFocused - checking foreground, ConEmu in front");
		}
		else
		{
			DEBUGSTRFOCUS(L"CRealConsole::OnGuiFocused - checking foreground, ConEmu inactive");
		}
	}

	//// Если FALSE - сервер увеличивает интервал опроса консоли (GUI теряет фокус)
	//mb_ThawRefreshThread = abFocus || !gpSet->isSleepInBackground;

	// 121007 - сервер теперь дергаем только при АКТИВАЦИИ окна
	////BOOL lbNeedChange = FALSE;
	//// Разрешит "заморозку" серверной нити и обновит hdr.bConsoleActive в мэппинге
	//if (m_ConsoleMap.IsValid() && ms_MainSrv_Pipe[0])
	if (abFocus)
	{
		BOOL lbActive = isActive(false);

		// -- Проверки убираем. Overhead небольшой, а проблем огрести можно (например, мэппинг обновиться не успел)
		//if ((BOOL)m_ConsoleMap.Ptr()->bConsoleActive == lbActive
		//     && (BOOL)m_ConsoleMap.Ptr()->bThawRefreshThread == mb_ThawRefreshThread)
		//{
		//	lbNeedChange = FALSE;
		//}
		//else
		//{
		//	lbNeedChange = TRUE;
		//}
		//if (lbNeedChange)

		if (lbActive)
		{
			UpdateServerActive();
		}
	}

#ifdef _DEBUG
	DEBUGSTRALTSRV(L"--> Updating active was skipped\n");
#endif
}

// Обновить в сервере флаги Active & ThawRefreshThread,
// а заодно заставить перечитать содержимое консоли (если abActive == TRUE)
void CRealConsole::UpdateServerActive(bool abImmediate /*= FALSE*/)
{
	AssertThis();

	//mb_UpdateServerActive = abActive;
	bool bActiveNonSleep = false;
	bool bActive = isActive(false);
	bool bVisible = bActive || isVisible();
	if (gpSet->isRetardInactivePanes ? bActive : bVisible)
	{
		bActiveNonSleep = (!gpSet->isSleepInBackground || mp_ConEmu->isMeForeground(true, true));
	}

	if (!bActiveNonSleep)
		return; // команду в сервер посылаем только при активации

	if (!isServerCreated(true))
		return; // сервер еще не завершил инициализацию

	DEBUGTEST(DWORD nTID = GetCurrentThreadId());

	if (!abImmediate && (mn_MonitorThreadID && (GetCurrentThreadId() != mn_MonitorThreadID)))
	{
		#ifdef _DEBUG
		static bool bDebugIgnoreActiveEvent = false;
		if (bDebugIgnoreActiveEvent)
		{
			return;
		}
		#endif

		DEBUGSTRFOCUS(L"UpdateServerActive - delayed, event");
		_ASSERTE(mh_UpdateServerActiveEvent!=nullptr);
		mn_ServerActiveTick1 = GetTickCount();
		SetEvent(mh_UpdateServerActiveEvent);
		return;
	}

	BOOL fSuccess = FALSE;

	// Always and only in the Main server
	if (ms_MainSrv_Pipe[0])
	{
		size_t nInSize = sizeof(CESERVER_REQ_HDR);
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_ONACTIVATION, nInSize);
		CESERVER_REQ* pOut = nullptr;

		if (pIn)
		{
			#if 0
			wchar_t szInfo[255];
			bool bFore = mp_ConEmu->isMeForeground(true, true);
			HWND hFore = GetForegroundWindow(), hFocus = GetFocus();
			wchar_t szForeWnd[1024]; getWindowInfo(hFore, szForeWnd); szForeWnd[128] = 0;
			swprintf_c(szInfo,
				L"UpdateServerActive - called(Active=%u, Speed=%s, mb_LastConEmuFocusState=%u, ConEmu=%s, hFore=%s, hFocus=x%08X)",
				abActive, mb_ThawRefreshThread ? L"high" : L"low", mp_ConEmu->mb_LastConEmuFocusState, bFore ? L"foreground" : L"inactive",
				szForeWnd, (DWORD)hFocus);
			#endif

			DWORD dwTickStart = timeGetTime();
			mn_LastUpdateServerActive = GetTickCount();

			pOut = ExecuteCmd(ms_MainSrv_Pipe, pIn, 500, ghWnd);
			fSuccess = (pOut != nullptr);

			CSetPgDebug::debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime()-dwTickStart, ms_MainSrv_Pipe, pOut);

			#if 0
			DEBUGSTRFOCUS(szInfo);
			#endif

			mn_LastUpdateServerActive = GetTickCount();
		}

		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
	}
	else
	{
		DEBUGSTRFOCUS(L"UpdateServerActive - failed, no Pipe");
	}

#ifdef _DEBUG
	wchar_t szDbgInfo[512];
	swprintf_c(szDbgInfo, L"--> Updating active(%i) %s: %s\n",
		bActiveNonSleep, fSuccess ? L"OK" : L"Failed!", *ms_MainSrv_Pipe ? (ms_MainSrv_Pipe+18) : L"<NoPipe>");
	DEBUGSTRALTSRV(szDbgInfo);
#endif
	UNREFERENCED_PARAMETER(fSuccess);
}

void CRealConsole::UpdateScrollInfo()
{
	if (!isActive(false))
		return;

	if (!isMainThread())
	{
		mp_ConEmu->OnUpdateScrollInfo(FALSE/*abPosted*/);
		return;
	}

	CVConGuard guard(mp_VCon);

	UpdateCursorInfo();


	if (m_ScrollStatus.nLastHeight == mp_ABuf->GetDynamicHeight()
	        && m_ScrollStatus.nLastWndHeight == mp_ABuf->GetTextHeight()/*(con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1)*/
	        && m_ScrollStatus.nLastTop == mp_ABuf->GetBufferPosY()/*con.m_sbi.srWindow.Top*/)
		return; // was not changed

	m_ScrollStatus.nLastHeight = mp_ABuf->GetDynamicHeight();
	m_ScrollStatus.nLastWndHeight = mp_ABuf->GetTextHeight()/*(con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1)*/;
	m_ScrollStatus.nLastTop = mp_ABuf->GetBufferPosY()/*con.m_sbi.srWindow.Top*/;
	mp_VCon->SetScroll(mp_ABuf->isScroll()/*con.bBufferHeight*/, m_ScrollStatus.nLastTop, m_ScrollStatus.nLastWndHeight, m_ScrollStatus.nLastHeight);
}

bool CRealConsole::_TabsInfo::RefreshFarPID(DWORD nNewPID)
{
	bool bChanged = false;
	CTab ActiveTab("RealConsole.cpp:ActiveTab",__LINE__);
	int nActiveTab = -1;
	if (m_Tabs.RefreshFarStatus(nNewPID, ActiveTab, nActiveTab, mn_tabsCount, mb_HasModalWindow))
	{
		StoreActiveTab(nActiveTab, ActiveTab);
		mb_TabsWasChanged = bChanged = true;
	}
	return bChanged;
}

void CRealConsole::_TabsInfo::StoreActiveTab(int anActiveIndex, CTab& ActiveTab)
{
	if (ActiveTab.Tab() && (anActiveIndex >= 0))
	{
		nActiveIndex = anActiveIndex;
		nActiveFarWindow = ActiveTab->Info.nFarWindowID;
		_ASSERTE(ActiveTab->Info.Type & fwt_CurrentFarWnd);
		nActiveType = ActiveTab->Info.Type;
		mp_ActiveTab->Init(ActiveTab);
	}
	else
	{
		_ASSERTE(FALSE && "Active tab must be detected!");
		nActiveIndex = 0;
		nActiveFarWindow = 0;
		nActiveType = fwt_Panels|fwt_CurrentFarWnd;
		mp_ActiveTab->Init(nullptr);
	}
}

void CRealConsole::SetTabs(ConEmuTab* apTabs, int anTabsCount, DWORD anFarPID)
{
#ifdef _DEBUG
	wchar_t szDbg[128];
	swprintf_c(szDbg, L"CRealConsole::SetTabs.  ItemCount=%i, PrevItemCount=%i\n", anTabsCount, tabs.mn_tabsCount);
	DEBUGSTRTABS(szDbg);
#endif

	ConEmuTab lTmpTabs = {0};
	bool bRenameByArgs = false;

	// Табы нужно проверить и подготовить
	if (apTabs && (anTabsCount > 0))
	{
		_ASSERTE(apTabs->Type>1 || !apTabs->Modified);

		// иначе вместо "Panels" будет то что в заголовке консоли. Например "edit - doc1.doc"
		// это например, в процессе переключения закладок
		if (anTabsCount > 1 && apTabs[0].Type == fwt_Panels && apTabs[0].Current)
			apTabs[0].Name[0] = 0;

		// Ensure that "far /e file.txt" will produce "Modal" tab
		if (anTabsCount == 1 && (apTabs[0].Type == fwt_Viewer || apTabs[0].Type == fwt_Editor))
			apTabs[0].Modal = 1;

		// Far may fail sometimes when closing modal editor (when editor can't save changes on ShiftF10)
		int nModalTab = -1;
		for (int i = (anTabsCount-1); i >= 0; i--)
		{
			if (apTabs[i].Modal)
			{
				_ASSERTE((i == (anTabsCount-1)) && "Modal tabs can be at the tail only");
				if (!apTabs[i].Current)
				{
					if (i == (anTabsCount-1))
					{
						DEBUGSTRTABS(L"!!! Last 'Modal' tab was not 'Current'\n");
						apTabs[i].Current = 1;
					}
					else
					{
						_ASSERTE(apTabs[i].Current && "Modal tab can be only current?");
					}
				}
			}
		}

		// Find active tab. Start from the tail (Far can fail during opening new editor/viewer)
		int nActiveTab = -1;
		for (int i = (anTabsCount-1); i >= 0; i--)
		{
			if (apTabs[i].Current)
			{
				// Active tab found
				nActiveTab = i;
				// Avoid possible faults with modal editors/viewers
				for (int j = (i - 1); j >= 0; j--)
				{
					if (apTabs[j].Current)
						apTabs[j].Current = 0;
				}
				break;
			}
		}

		// Ensure, at least one tab is current
		if (nActiveTab < 0)
		{
			apTabs[0].Current = 1;
		}

		#ifdef _DEBUG
		// отладочно: имена закладок (редакторов/вьюверов) должны быть заданы!
		for (int i = 1; i < anTabsCount; i++)
		{
			if (apTabs[i].Name[0] == 0)
			{
				_ASSERTE(apTabs[i].Name[0]!=0);
			}
		}
		#endif
	}
	else if ((anTabsCount == 1 && apTabs == nullptr) || (anTabsCount <= 0 || apTabs == nullptr))
	{
		_ASSERTE(anTabsCount>=1);
		apTabs = &lTmpTabs;
		apTabs->Current = 1;
		apTabs->Type = 1;
		anTabsCount = 1;
		bRenameByArgs = (m_Args.pszRenameTab && *m_Args.pszRenameTab);
		if (!ms_DefTitle.IsEmpty())
		{
			lstrcpyn(apTabs->Name, ms_DefTitle.ms_Val, countof(apTabs->Name));
		}
		else if (ms_RootProcessName[0])
		{
			// Если известно (должно бы уже) имя корневого процесса - показать его
			lstrcpyn(apTabs->Name, ms_RootProcessName, countof(apTabs->Name));
			wchar_t* pszDot = wcsrchr(apTabs->Name, L'.');
			if (pszDot) // Leave file name only, no extension
				*pszDot = 0;
		}
		else
		{
			_ASSERTE(FALSE && "RootProcessName or DefTitle must be known already!")
		}
	}
	else
	{
		_ASSERTE(FALSE && "Must not get here!");
		return;
	}

	// Already must be
	_ASSERTE(anTabsCount>0 && apTabs!=nullptr);

	// Started "as admin"
	if (isAdministrator())
	{
		// Mark tabs as elevated (overlay icon on tab or suffix appended by GetTab)
		for (int i = 0; i < anTabsCount; i++)
		{
			apTabs[i].Type |= fwt_Elevated;
		}
	}

	HANDLE hUpdate = tabs.m_Tabs.UpdateBegin();

	bool bTabsChanged = false;
	bool bHasModal = false;
	CTab ActiveTab("RealConsole.cpp:ActiveTab",__LINE__);
	int  nActiveIndex = -1;

	for (int i = 0; i < anTabsCount; i++)
	{
		CEFarWindowType TypeAndFlags = apTabs[i].Type
			| (apTabs[i].Modified ? fwt_ModifiedFarWnd : fwt_Any)
			| (apTabs[i].Current ? fwt_CurrentFarWnd : fwt_Any)
			| (apTabs[i].Modal ? fwt_ModalFarWnd : fwt_Any);

		bool bModal = ((TypeAndFlags & fwt_ModalFarWnd) == fwt_ModalFarWnd);
		bool bEditorViewer = (((TypeAndFlags & fwt_TypeMask) == fwt_Editor) || ((TypeAndFlags & fwt_TypeMask) == fwt_Viewer));

		// Do not use GetFarPID() here, because Far states may not be updated yet?
		int nPID = (bModal || bEditorViewer) ? anFarPID : 0;
		#ifdef _DEBUG
		_ASSERTE(nPID || ((TypeAndFlags & fwt_TypeMask) == fwt_Panels));
		if (nPID && !GetFarPID())
			int nDbg = 0; // That may happen with "edit:<git log", if processes were not updated yet (in MonitorThread)
		#endif

		bHasModal |= bModal;

		if (tabs.m_Tabs.UpdateFarWindow(hUpdate, mp_VCon, apTabs[i].Name, TypeAndFlags, nPID, apTabs[i].Pos, apTabs[i].EditViewId, ActiveTab))
			bTabsChanged = true;

		if (TypeAndFlags & fwt_CurrentFarWnd)
			nActiveIndex = i;
	}

	_ASSERTE(!bRenameByArgs || (anTabsCount==1));
	if (bRenameByArgs)
	{
		_ASSERTE(ActiveTab.Tab()!=nullptr);
		if (ActiveTab.Tab())
		{
			ActiveTab->Info.Type |= fwt_Renamed;
			ActiveTab->Renamed.Set(m_Args.pszRenameTab);
		}
	}

	tabs.mn_tabsCount = anTabsCount;
	tabs.mb_HasModalWindow = bHasModal;
	tabs.StoreActiveTab(nActiveIndex, ActiveTab);

	if (tabs.m_Tabs.UpdateEnd(hUpdate, GetFarPID(true)))
		bTabsChanged = true;

	if (bTabsChanged)
		tabs.mb_TabsWasChanged = true;

	#ifdef _DEBUG
	GetActiveTabType();
	#endif

	#ifdef _DEBUG
	// Check tabs, flag fwt_CurrentFarWnd must be set at least once
	bool FlagWasSet = false;
	for (int I = tabs.mn_tabsCount-1; I >= 0; I--)
	{
		CTab tab(__FILE__,__LINE__);
		if (tabs.m_Tabs.GetTabByIndex(I, tab))
		{
			if (tab->Flags() & fwt_CurrentFarWnd)
			{
				FlagWasSet = true;
				break;
			}
		}
	}
	_ASSERTE(FlagWasSet);
	#endif

	// Передернуть mp_ConEmu->mp_TabBar->..
	if (mp_ConEmu->isValid(mp_VCon))    // Во время создания консоли она еще не добавлена в список...
	{
		// Если была показана ошибка "This tab can't be activated now"
		if (bTabsChanged && isActive(false))
		{
			// скрыть ее
			mp_ConEmu->mp_TabBar->ShowTabError(nullptr, 0);
		}
		// На время появления автотабов - отключалось
		mp_ConEmu->mp_TabBar->SetRedraw(TRUE);
		mp_ConEmu->mp_TabBar->Update();
	}
	else
	{
		_ASSERTE(FALSE && "Must be added in valid-list for consistence!");
	}
}

INT_PTR CRealConsole::renameProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	CRealConsole* pRCon = nullptr;
	if (messg == WM_INITDIALOG)
		pRCon = (CRealConsole*)lParam;
	else
		pRCon = (CRealConsole*)GetWindowLongPtr(hDlg, DWLP_USER);

	if (!pRCon)
		return FALSE;

	switch (messg)
	{
		case WM_INITDIALOG:
		{
			SendMessage(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hClassIcon));
			SendMessage(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hClassIconSm));

			pRCon->mp_ConEmu->OnOurDialogOpened();
			_ASSERTE(pRCon!=nullptr);
			SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pRCon);

			CDynDialog::LocalizeDialog(hDlg, lng_DlgRenameTab);

			if (pRCon->mp_RenameDpiAware)
				pRCon->mp_RenameDpiAware->Attach(hDlg, ghWnd, CDynDialog::GetDlgClass(hDlg));

			// Positioning
			pRCon->RepositionDialogWithTab(hDlg);

			HWND hEdit = GetDlgItem(hDlg, tNewTabName);

			int nLen = 0;
			CTab tab(__FILE__,__LINE__);
			if (pRCon->GetTab(pRCon->tabs.nActiveIndex, tab))
			{
				nLen = lstrlen(tab->GetName());
				SetWindowText(hEdit, tab->GetName());
			}

			SendMessage(hEdit, EM_SETSEL, 0, nLen);

			SetFocus(hEdit);

			return FALSE;
		}

		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				switch (LOWORD(wParam))
				{
					case IDOK:
						{
							const CEStr pszNew = GetDlgItemTextPtr(hDlg, tNewTabName);
							pRCon->RenameTab(pszNew);
							EndDialog(hDlg, IDOK);
							return TRUE;
						}
					case IDCANCEL:
					case IDCLOSE:
						renameProc(hDlg, WM_CLOSE, 0, 0);
						return TRUE;
				}
			}
			break;

		case WM_CLOSE:
			pRCon->mp_ConEmu->OnOurDialogClosed();
			EndDialog(hDlg, IDCANCEL);
			break;

		case WM_DESTROY:
			if (pRCon->mp_RenameDpiAware)
				pRCon->mp_RenameDpiAware->Detach();
			break;

		default:
			if (pRCon->mp_RenameDpiAware && pRCon->mp_RenameDpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
			{
				return TRUE;
			}
	}

	return FALSE;
}

void CRealConsole::DoRenameTab()
{
	AssertThis();

	DontEnable de;
	CDpiForDialog::Create(mp_RenameDpiAware);
	// Modal dialog (CreateDialog)
	INT_PTR iRc = CDynDialog::ExecuteDialog(IDD_RENAMETAB, ghWnd, renameProc, reinterpret_cast<LPARAM>(this));
	if (iRc == IDOK)
	{
		//mp_ConEmu->mp_TabBar->Update(); -- уже, в RenameTab(...)
		UNREFERENCED_PARAMETER(iRc);
	}
	SafeDelete(mp_RenameDpiAware);
}

// Запустить Elevated копию фара с теми же папками на панелях
void CRealConsole::AdminDuplicate()
{
	AssertThis();

	DuplicateRoot(false, true);
}

bool CRealConsole::DuplicateRoot(bool bSkipMsg /*= false*/, bool bRunAsAdmin /*= false*/, LPCWSTR asNewConsole /*= nullptr*/, LPCWSTR asApp /*= nullptr*/, LPCWSTR asParm /*= nullptr*/)
{
	ConProcess* pProc = nullptr, *p = nullptr;
	int nCount = GetProcesses(&pProc);
	if (nCount < 2)
	{
		SafeFree(pProc);
		if (!bSkipMsg)
		{
			DisplayLastError(L"Nothing to duplicate, root process not found", -1);
		}
		return false;
	}
	bool bOk = false;
	DWORD nServerPID = GetServerPID(true);
	for (int k = 0; k <= 1 && !p; k++)
	{
		for (int i = 0; i < nCount; i++)
		{
			if (pProc[i].ProcessID == nServerPID)
				continue;

			if (IsConsoleServer(pProc[i].Name) || IsConsoleService(pProc[i].Name))
				continue;

			if (!k)
			{
				if (pProc[i].ParentPID != nServerPID)
					continue;
			}
			p = pProc+i;
			break;
		}
	}
	if (!p)
	{
		if (!bSkipMsg)
		{
			DisplayLastError(L"Can't find root process in the active console", -1);
		}
	}
	else
	{
		CEStr szWorkDir; GetConsoleCurDir(szWorkDir, true);
		const CEStr szConfirm(L"Do you want to duplicate tab with root?\n",
			L"Process: ", p->Name, L"\n",
			L"Directory: ", szWorkDir);
		if (bSkipMsg || !gpSet->isMultiDupConfirm
			|| ((MsgBox(szConfirm, MB_OKCANCEL|MB_ICONQUESTION) == IDOK)))
		{
			bool bRootCmdRedefined = false;
			RConStartArgsEx args;
			args.AssignFrom(m_Args);

			// If user want to run anything else, just inheriting current console state
			if (asApp && *asApp)
			{
				bRootCmdRedefined = true;
				SafeFree(args.pszSpecialCmd);
				if (asParm)
					args.pszSpecialCmd = CEStr(asApp, L" ", asParm).Detach();
				else
					args.pszSpecialCmd = CEStr(asApp).Detach();
			}
			else if (asParm && *asParm)
			{
				bRootCmdRedefined = true;
				CEStr(args.pszSpecialCmd, L" ", asParm).Swap(args.pszSpecialCmd);
			}


			if (asNewConsole && *asNewConsole)
			{
				bRootCmdRedefined = true;
				CEStr(args.pszSpecialCmd, L" ", asNewConsole).Swap(args.pszSpecialCmd);
			}

			// Нужно оставить там "new_console", иначе не отключается подтверждение закрытия например
			CEStr szCmdRedefined((const wchar_t*)(bRootCmdRedefined ? args.pszSpecialCmd : nullptr));

			// Explicitly load "detected" working directory
			SafeFree(args.pszStartupDir);
			args.pszStartupDir = lstrdup(szWorkDir).Detach();

			// Mark as detached, because the new console will be started from active shell process,
			// but not from ConEmu (yet, behavior planned to be changed)
			args.Detached = crb_On;

			// Reset "split" settings, the actual must be passed within asNewConsole switches
			// and the will be processed during the following mp_ConEmu->CreateCon(&args) call
			args.eSplit = RConStartArgsEx::eSplitNone;

			// Create (detached) tab ready for attach
			CVirtualConsole *pVCon = mp_ConEmu->CreateCon(args);

			if (pVCon)
			{
				CRealConsole* pRCon = pVCon->RCon();

				size_t cbMaxSize = sizeof(CESERVER_REQ_HDR) + sizeof(CESERVER_REQ_DUPLICATE)
					+ (szCmdRedefined ? (szCmdRedefined.GetLen() + 1) : 0) * sizeof(wchar_t)
					+ (szWorkDir ? (szWorkDir.GetLen() + 1) : 0) * sizeof(wchar_t)
					;

				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_DUPLICATE, cbMaxSize);
				pIn->Duplicate.hGuiWnd = ghWnd;
				pIn->Duplicate.nGuiPID = GetCurrentProcessId();
				pIn->Duplicate.nAID = pRCon->GetMonitorThreadID();
				pIn->Duplicate.bRunAs = bRunAsAdmin;
				pIn->Duplicate.nWidth = pRCon->mp_RBuf->TextWidth();
				pIn->Duplicate.nHeight = pRCon->mp_RBuf->TextHeight();
				pIn->Duplicate.nBufferHeight = pRCon->mp_RBuf->GetBufferHeight();

				BYTE nTextColorIdx /*= 7*/, nBackColorIdx /*= 0*/, nPopTextColorIdx /*= 5*/, nPopBackColorIdx /*= 15*/;
				pRCon->PrepareDefaultColors(nTextColorIdx, nBackColorIdx, nPopTextColorIdx, nPopBackColorIdx, true);
				pIn->Duplicate.nColors = (nTextColorIdx) | (nBackColorIdx << 8) | (nPopTextColorIdx << 16) | (nPopBackColorIdx << 24);

				if (szWorkDir)
					pIn->Duplicate.sDirectory.Set(szWorkDir.Detach());

				if (szCmdRedefined)
					pIn->Duplicate.sCommand.Set(szCmdRedefined.Detach());

				// Variable strings
				LPBYTE ptrDst = ((LPBYTE)&pIn->Duplicate) + sizeof(pIn->Duplicate);
				ptrDst = pIn->Duplicate.sDirectory.Mangle(ptrDst);
				ptrDst = pIn->Duplicate.sCommand.Mangle(ptrDst);

				// Go
				int nFRc = -1;
				CESERVER_REQ* pOut = nullptr;
				if (m_Args.InjectsDisable != crb_On)
				{
					pOut = ExecuteHkCmd(p->ProcessID, pIn, ghWnd);
					nFRc = (pOut->DataSize() >= sizeof(DWORD)) ? (int)(pOut->dwData[0]) : -100;
				}

				// If ConEmuHk was not enabled in the console or due to another reason
				// duplicate root was failed - just start new console with the same options
				if ((nFRc != 0) && args.pszSpecialCmd && *args.pszSpecialCmd)
				{
					pRCon->RequestStartup(true);
					nFRc = 0;
				}

				if (nFRc != 0)
				{
					if (!bSkipMsg)
					{
						wchar_t szRc[16];
						CEStr szError(L"Duplicate tab with root '", p->Name, L"' failed, code=", ltow_s(nFRc, szRc, 10));
						DisplayLastError(szError, -1);
					}
					// Do not leave 'hunging' tab if duplicating was failed
					pRCon->CloseConsole(false, false, false);
				}
				else
				{
					bOk = true;
				}
				ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
		}
	}
	SafeFree(pProc);
	return bOk;
}

void CRealConsole::RenameTab(LPCWSTR asNewTabText /*= nullptr*/)
{
	AssertThis();

	CTab tab(__FILE__,__LINE__);
	if (GetTab(tabs.nActiveIndex, tab))
	{
		if (asNewTabText && *asNewTabText)
		{
			tab->Renamed.Set(asNewTabText);
			tab->Info.Type |= fwt_Renamed;
		}
		else
		{
			tab->Renamed.Set(nullptr);
			tab->Info.Type &= ~fwt_Renamed;
		}
		_ASSERTE(tab->Info.Type & fwt_CurrentFarWnd);
		tabs.nActiveType = tab->Info.Type;
	}

	mp_ConEmu->mp_TabBar->Update();
	mp_VCon->OnTitleChanged();
}

void CRealConsole::RenameWindow(LPCWSTR asNewWindowText /*= nullptr*/)
{
	AssertThis();

	DWORD dwServerPID = GetServerPID(true);
	if (!dwServerPID)
		return;

	if (!asNewWindowText || !*asNewWindowText)
		asNewWindowText = mp_ConEmu->GetDefaultTitle();

	int cchMax = lstrlen(asNewWindowText)+1;
	CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SETCONTITLE, sizeof(CESERVER_REQ_HDR)+sizeof(wchar_t)*cchMax);
	if (pIn)
	{
		_wcscpy_c((wchar_t*)pIn->wData, cchMax, asNewWindowText);

		DWORD dwTickStart = timeGetTime();

		CESERVER_REQ *pOut = ExecuteSrvCmd(dwServerPID, pIn, ghWnd);

		CSetPgDebug::debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
	}
}

int CRealConsole::GetRootProcessIcon()
{
	AssertThisRet(-1);
	return gpSet->isTabIcons ? mn_RootProcessIcon : -1;
}

LPCWSTR CRealConsole::GetRootProcessName()
{
	AssertThisRet(nullptr);
	if (!*ms_RootProcessName)
		return nullptr;
	return ms_RootProcessName;
}

void CRealConsole::NeedRefreshRootProcessIcon()
{
	mb_NeedLoadRootProcessIcon = true;
	GetDefaultAppSettingsId();
}

int CRealConsole::GetTabCount(bool abVisibleOnly /*= FALSE*/)
{
	AssertThisRet(0);

	if (abVisibleOnly)
	{
		// Если не хотят показывать все доступные редакторы/вьювера, а только активное окно
		if (!gpSet->bShowFarWindows)
			return 1;
	}

	if (((mn_ProgramStatus & CES_FARACTIVE) == 0))
		return 1; // На время выполнения команд - ТОЛЬКО одна закладка

	//TODO("Обработать gpSet->isHideDisabledTabs(), вдруг какие-то табы засерены");
	//if (tabs.mn_tabsCount > 1 && gpSet->bHideDisabledConsoleTabs)
	//{
	//	int nCount = 0;
	//	for (int i = 0; i < tabs.m_Tabs.GetCount(); i++)
	//	{
	//		if (CanActivateFarWindow(i))
	//			nCount++;
	//	}
	//	if (nCount == 0)
	//	{
	//		_ASSERTE(nCount>0);
	//		nCount = 1;
	//	}
	//	return nCount;
	//}

	return std::max(tabs.mn_tabsCount,1);
}

int CRealConsole::GetActiveTab()
{
	AssertThisRet(0);

	return tabs.nActiveIndex;
}

// (Panels=1, Viewer=2, Editor=3) |(Elevated=0x100) |(NotElevated=0x200) |(Modal=0x400)
CEFarWindowType CRealConsole::GetActiveTabType()
{
	int nType = fwt_Any/*0*/;

	#ifdef _DEBUG
	TabInfo rInfo;
	int iModal = -1, iTabCount;
	if (tabs.mn_tabsCount < 1)
	{
		// Possible situation if some windows message was processed during RCon construction (ex. status bar re-painting)
		_ASSERTE(tabs.mn_tabsCount>=1 || !mb_ConstuctorFinished);
		nType = fwt_Panels|fwt_CurrentFarWnd;
	}
	else
	{
		nType = fwt_Panels|fwt_CurrentFarWnd;
		MSectionLockSimple SC;
		tabs.m_Tabs.LockTabs(&SC);
		iTabCount = tabs.m_Tabs.GetCount();
		for (int i = 0; i < iTabCount; i++)
		{
			if (tabs.m_Tabs.GetTabInfoByIndex(i, rInfo)
				&& (rInfo.Status == tisValid)
				&& (rInfo.Type & fwt_ModalFarWnd))
			{
				iModal = i;
				_ASSERTE(tabs.nActiveIndex == i);
				_ASSERTE((rInfo.Type & fwt_CurrentFarWnd) == fwt_CurrentFarWnd);
				nType = rInfo.Type;
				break;
			}
		}

		if (iModal == -1)
		{
			for (int i = 0; i < iTabCount; i++)
			{
				if (tabs.m_Tabs.GetTabInfoByIndex(i, rInfo)
					&& (rInfo.Status == tisValid)
					&& (rInfo.Type & fwt_CurrentFarWnd))
				{
					nType = rInfo.Type;
					_ASSERTE(tabs.nActiveIndex == i);
				}
			}
		}
	}

	_ASSERTE(tabs.nActiveType == nType);
	#endif

	// And release code
	nType = tabs.nActiveType;

	#ifdef _DEBUG
	if (isAdministrator() != ((nType&fwt_Elevated)==fwt_Elevated))
	{
		_ASSERTE(isAdministrator() == ((nType&fwt_Elevated)==fwt_Elevated));

		#if 0
		if (isAdministrator() && gpSet->isAdminShield())
		{
			nType |= fwt_Elevated;
		}
		#endif
	}
	#endif

	return nType;
}

#if 0
void CRealConsole::UpdateTabFlags(/*IN|OUT*/ ConEmuTab* pTab)
{
	if (ms_RenameFirstTab[0])
		pTab->Type |= fwt_Renamed;

	if (isAdministrator() && (gpSet->isAdminShield() || gpSet->isAdminSuffix()))
	{
		if (gpSet->isAdminShield())
		{
			pTab->Type |= fwt_Elevated;
		}
	}
}
#endif

// Если такого таба нет - pTab НЕ ОБНУЛЯТЬ!!!
bool CRealConsole::GetTab(int tabIdx, /*OUT*/ CTab& rTab)
{
	AssertThisRet(false);

	#ifdef _DEBUG
	// Должен быть как минимум один (хотя бы пустой) таб
	if (tabs.mn_tabsCount <= 0)
	{
		_ASSERTE(tabs.mn_tabsCount>0 || !tabs.mb_WasInitialized);
	}
	#endif

	// Здесь именно mn_tabsCount, т.к. возвращаются "визуальные" табы, а не "ушедшие в фон"
	if ((tabIdx < 0) || (tabIdx >= tabs.mn_tabsCount))
		return false;

	int iGetTabIdx = tabIdx;

	// На время выполнения DOS-команд - только один таб
	if (mn_ProgramStatus & CES_FARACTIVE)
	{
		// Only Far Manager can get several tab in one console
		#if 0
		// Есть модальный редактор/вьювер?
		if (tabs.mb_HasModalWindow)
		{
			if (tabIdx > 0)
				return false;

			CTab Test(__FILE__,__LINE__);
			for (int i = 0; i < tabs.mn_tabsCount; i++)
			{
				if (tabs.m_Tabs.GetTabByIndex(iGetTabIdx, Test)
					&& (Test->Flags() & fwt_ModalFarWnd))
				{
					iGetTabIdx = i;
					break;
				}
			}
		}
		#endif
	}
	else if (tabIdx > 0)
	{
		// Return all tabs, even if Far is in background
		#ifdef _DEBUG
		//return false;
		int iDbg = 0;
		#endif
	}

	// Go
	if (!tabs.m_Tabs.GetTabByIndex(iGetTabIdx, rTab))
		return false;

	// Refresh its highlighted state
	if ((rTab->Info.Type & fwt_CurrentFarWnd) && isHighlighted() && (tabs.nFlashCounter & 1))
		rTab->Info.Type |= fwt_Highlighted;
	else if (rTab->Info.Type & fwt_Highlighted)
		rTab->Info.Type &= ~fwt_Highlighted;

	// Refresh modified state of simple consoles (not the Editor tabs of Far Manager)
	if ((rTab->Info.Type & fwt_CurrentFarWnd) && (rTab->Type() != fwt_Editor))
	{
		if (tabs.bConsoleDataChanged)
			rTab->Info.Type |= fwt_ModifiedFarWnd;
		else
			rTab->Info.Type &= ~fwt_ModifiedFarWnd;
	}
	else if (rTab->Type() != fwt_Editor)
	{
		rTab->Info.Type &= ~fwt_ModifiedFarWnd;
	}

	// Update active tab info
	if (rTab->Info.Type & fwt_CurrentFarWnd)
		tabs.StoreActiveTab(iGetTabIdx, rTab);

#if 0
	if ((tabIdx == 0) && (*tabs.ms_RenameFirstTab))
	{
		rTab->Info.Type |= fwt_Renamed;
		rTab->Renamed.Set(tabs.ms_RenameFirstTab);
	}
	else
	{
		rTab->Info.Type &= ~fwt_Renamed;
	}
#endif

	return true;
}

int CRealConsole::GetModifiedEditors()
{
	int nEditors = 0;

	if (tabs.mn_tabsCount > 0)
	{
		TabInfo Info;

		for (int j = 0; j < tabs.mn_tabsCount; j++)
		{
			if (tabs.m_Tabs.GetTabInfoByIndex(j, Info))
			{
				if (((Info.Type & fwt_TypeMask) == fwt_Editor) && (Info.Type & fwt_ModifiedFarWnd))
					nEditors++;
			}
		}
	}

	return nEditors;
}

void CRealConsole::CheckPanelTitle()
{
#ifdef _DEBUG

	if (mb_DebugLocked)
		return;

#endif

	if (tabs.mn_tabsCount > 0)
	{
		CEFarWindowType wt = GetActiveTabType();
		if ((wt & fwt_TypeMask) == fwt_Panels)
		{
			WCHAR szPanelTitle[CONEMUTABMAX];

			if (!GetWindowText(hConWnd, szPanelTitle, countof(szPanelTitle)-1))
				ms_PanelTitle[0] = 0;
			else if (szPanelTitle[0] == L'{' || szPanelTitle[0] == L'(')
				lstrcpy(ms_PanelTitle, szPanelTitle);
		}
	}
}

DWORD CRealConsole::CanActivateFarWindow(int anWndIndex)
{
	AssertThisRet(0);

	WARNING("CantActivateInfo: Хорошо бы при отображении хинта 'Can't activate tab' сказать 'почему'");

	DWORD dwPID = GetFarPID();

	if (!dwPID)
	{
		wcscpy_c(tabs.sTabActivationErr, L"Far was not found in console");
		LogString(tabs.sTabActivationErr);
		return -1; // консоль активируется без разбора по вкладкам (фара нет)
	}

	// Если есть меню: ucBoxDblDownRight ucBoxDblHorz ucBoxDblHorz ucBoxDblHorz (первая или вторая строка консоли) - выходим
	// Если идет процесс (в заголовке консоли {n%}) - выходим
	// Если висит диалог - выходим (диалог обработает сам плагин)

	// Far 4040 - new "Desktop" window type has "0" index,
	// so we can't just check (anWndIndex >= tabs.mn_tabsCount)
	if (anWndIndex < 0 /*|| anWndIndex >= tabs.mn_tabsCount*/)
	{
		swprintf_c(tabs.sTabActivationErr, L"Bad anWndIndex=%i was requested", anWndIndex);
		LogString(tabs.sTabActivationErr);
		AssertCantActivate(anWndIndex>=0);
		return 0;
	}

	// Добавил такую проверочку. По идее, у нас всегда должен быть актуальный номер текущего окна.
	if (tabs.nActiveFarWindow == anWndIndex)
	{
		swprintf_c(tabs.sTabActivationErr, L"Far window %i is already active", anWndIndex);
		LogString(tabs.sTabActivationErr);
		return (DWORD)-1; // Нужное окно уже выделено, лучше не дергаться...
	}

	if (isPictureView(TRUE))
	{
		swprintf_c(tabs.sTabActivationErr, L"PicView was found\r\nPID=%u, anWndIndex=%i", dwPID, anWndIndex);
		LogString(tabs.sTabActivationErr);
		AssertCantActivate("isPictureView"==nullptr);
		return 0; // При наличии PictureView переключиться на другой таб этой консоли не получится
	}

	if (!GetWindowText(hConWnd, TitleCmp, countof(TitleCmp)-2))
		TitleCmp[0] = 0;

	// Прогресс уже определился в другом месте
	if (GetProgress(nullptr)>=0)
	{
		swprintf_c(tabs.sTabActivationErr, L"Progress was detected\r\nPID=%u, anWndIndex=%i", dwPID, anWndIndex);
		LogString(tabs.sTabActivationErr);
		AssertCantActivate("GetProgress>0"==nullptr);
		return 0; // Идет копирование или какая-то другая операция
	}

	//// Копирование в FAR: "{33%}..."
	////2009-06-02: PPCBrowser показывает копирование так: "(33% 00:02:20)..."
	//if ((TitleCmp[0] == L'{' || TitleCmp[0] == L'(')
	//    && isDigit(TitleCmp[1]) &&
	//    ((TitleCmp[2] == L'%' /*&& TitleCmp[3] == L'}'*/) ||
	//     (isDigit(TitleCmp[2]) && TitleCmp[3] == L'%' /*&& TitleCmp[4] == L'}'*/) ||
	//     (isDigit(TitleCmp[2]) && isDigit(TitleCmp[3]) && TitleCmp[4] == L'%' /*&& TitleCmp[5] == L'}'*/))
	//   )
	//{
	//    // Идет копирование
	//    return 0;
	//}

	if (!mp_RBuf->isInitialized())
	{
		swprintf_c(tabs.sTabActivationErr, L"Buffer not initialized\r\nPID=%u, anWndIndex=%i", dwPID, anWndIndex);
		LogString(tabs.sTabActivationErr);
		AssertCantActivate("Buf.isInitiazed"==nullptr);
		return 0; // консоль не инициализирована, ловить нечего
	}

	if (mp_RBuf != mp_ABuf)
	{
		swprintf_c(tabs.sTabActivationErr, L"Alternative buffer is active\r\nPID=%u, anWndIndex=%i", dwPID, anWndIndex);
		LogString(tabs.sTabActivationErr);
		AssertCantActivate("mp_RBuf != mp_ABuf"==nullptr);
		return 0; // если активирован доп.буфер - менять окна нельзя
	}

	BOOL lbMenuOrMacro = FALSE;

	// Меню может быть только в панелях
	if ((GetActiveTabType() & fwt_TypeMask) == fwt_Panels)
	{
		lbMenuOrMacro = mp_RBuf->isFarMenuOrMacro();
	}

	// Если строка меню отображается всегда:
	//  0-строка начинается с "  " или с "R   " если идет запись макроса
	//  1-я строка ucBoxDblDownRight ucBoxDblHorz ucBoxDblHorz или "[0+1]" ucBoxDblHorz ucBoxDblHorz
	//  2-я строка ucBoxDblVert
	// Наличие активного меню определяем по количеству цветов в первой строке.
	// Неактивное меню отображается всегда одним цветом - в активном подсвечиваются хоткеи и выбранный пункт

	if (lbMenuOrMacro)
	{
		swprintf_c(tabs.sTabActivationErr, L"Menu or macro was detected\r\nPID=%u, anWndIndex=%i", dwPID, anWndIndex);
		LogString(tabs.sTabActivationErr);
		AssertCantActivate(lbMenuOrMacro==FALSE);
		return 0;
	}

	// Если висит диалог - не даем переключаться по табам
	if (mp_ABuf && (mp_ABuf->GetDetector()->GetFlags() & FR_FREEDLG_MASK))
	{
		swprintf_c(tabs.sTabActivationErr, L"Dialog was detected\r\nPID=%u, anWndIndex=%i", dwPID, anWndIndex);
		LogString(tabs.sTabActivationErr);
		AssertCantActivate("FR_FREEDLG_MASK"==nullptr);
		return 0;
	}

	AssertCantActivate(dwPID!=0);
	return dwPID;
}

bool CRealConsole::IsSwitchFarWindowAllowed()
{
	AssertThisRet(false);

	if (mb_InCloseConsole || mn_TermEventTick)
	{
		wcscpy_c(tabs.sTabActivationErr,
			mb_InCloseConsole
				? L"Console is in closing state"
				: L"Termination event was set");
		return false;
	}

	return true;
}

LPCWSTR CRealConsole::GetActivateFarWindowError(wchar_t* pszBuffer, size_t cchBufferMax)
{
	AssertThisRet(L"this==nullptr");

	if (!pszBuffer || !cchBufferMax)
	{
		return *tabs.sTabActivationErr ? tabs.sTabActivationErr : L"Unknown error";
	}

	_wcscpy_c(pszBuffer, cchBufferMax, L"This tab can't be activated now!");
	if (*tabs.sTabActivationErr)
	{
		_wcscat_c(pszBuffer, cchBufferMax, L"\r\n");
		_wcscat_c(pszBuffer, cchBufferMax, tabs.sTabActivationErr);
	}

	return pszBuffer;
}

bool CRealConsole::ActivateFarWindow(int anWndIndex)
{
	AssertThisRet(false);

	tabs.sTabActivationErr[0] = 0;

	if ((anWndIndex == tabs.nActiveFarWindow) || (!anWndIndex && (tabs.mn_tabsCount <= 1)))
	{
		return true;
	}

	if (!IsSwitchFarWindowAllowed())
	{
		if (!*tabs.sTabActivationErr) wcscpy_c(tabs.sTabActivationErr, L"!IsSwitchFarWindowAllowed()");
		LogString(tabs.sTabActivationErr);
		return false;
	}

	DWORD dwPID = CanActivateFarWindow(anWndIndex);

	if (!dwPID)
	{
		if (!*tabs.sTabActivationErr) wcscpy_c(tabs.sTabActivationErr, L"Far PID not found (0)");
		LogString(tabs.sTabActivationErr);
		return false;
	}
	else if (dwPID == (DWORD)-1)
	{
		_ASSERTE(tabs.sTabActivationErr[0] == 0);
		return true; // Нужное окно уже выделено, лучше не дергаться...
	}

	bool lbRc = false;
	//DWORD nWait = -1;
	CConEmuPipe pipe(dwPID, 100);

	if (!pipe.Init(_T("CRealConsole::ActivateFarWindow")))
	{
		swprintf_c(tabs.sTabActivationErr, L"Pipe initialization failed\r\nPID=%u", dwPID);
		LogString(tabs.sTabActivationErr);
	}
	else
	{
		DWORD nData[2] = {(DWORD)anWndIndex,0};

		// Если в панелях висит QSearch - его нужно предварительно "снять"
		if (((tabs.nActiveType & fwt_TypeMask) == fwt_Panels)
			&& (mp_ABuf && (mp_ABuf->GetDetector()->GetFlags() & FR_QSEARCH)))
		{
			nData[1] = TRUE;
		}

		DEBUGSTRCMD(L"GUI send CMD_SETWINDOW\n");
		if (!pipe.Execute(CMD_SETWINDOW, nData, 8))
		{
			swprintf_c(tabs.sTabActivationErr, L"CMD_SETWINDOW failed\r\nPID=%u, Err=%u", dwPID, GetLastError());
			LogString(tabs.sTabActivationErr);
		}
		else
		{
			DEBUGSTRCMD(L"CMD_SETWINDOW executed\n");

			WARNING("CMD_SETWINDOW по таймауту возвращает последнее считанное положение окон (gpTabs).");
			// То есть если переключение окна выполняется дольше 2х сек - возвратится предыдущее состояние
			DWORD cbBytesRead=0;
			//DWORD tabCount = 0, nInMacro = 0, nTemp = 0, nFromMainThread = 0;
			ConEmuTab* pGetTabs = nullptr;
			CESERVER_REQ_CONEMUTAB TabHdr;
			DWORD nHdrSize = sizeof(CESERVER_REQ_CONEMUTAB) - sizeof(TabHdr.tabs);

			//if (pipe.Read(&tabCount, sizeof(DWORD), &cbBytesRead) &&
			//	pipe.Read(&nInMacro, sizeof(DWORD), &nTemp) &&
			//	pipe.Read(&nFromMainThread, sizeof(DWORD), &nTemp)
			//	)
			if (!pipe.Read(&TabHdr, nHdrSize, &cbBytesRead))
			{
				swprintf_c(tabs.sTabActivationErr, L"CMD_SETWINDOW result read failed\r\nPID=%u, Err=%u", dwPID, GetLastError());
				LogString(tabs.sTabActivationErr);
			}
			else
			{
				pGetTabs = (ConEmuTab*)pipe.GetPtr(&cbBytesRead);
				_ASSERTE(cbBytesRead==(TabHdr.nTabCount*sizeof(ConEmuTab)));

				if (cbBytesRead != (TabHdr.nTabCount*sizeof(ConEmuTab)))
				{
					swprintf_c(tabs.sTabActivationErr,
						L"CMD_SETWINDOW bad result header\r\nPID=%u, (%u != %u*%u)", dwPID, cbBytesRead, TabHdr.nTabCount, (DWORD)sizeof(ConEmuTab));
					LogString(tabs.sTabActivationErr);
				}
				else
				{
					SetTabs(pGetTabs, TabHdr.nTabCount, dwPID);
					int iActive = -1;
					if ((anWndIndex >= 0) && (TabHdr.nTabCount > 0))
					{
						// Последние изменения в фаре привели к невозможности
						// проверки корректности активации таба по его ИД
						// Очередной Far API breaking change
						lbRc = true;

						#if 0
						for (UINT i = 0; i < TabHdr.nTabCount; i++)
						{
							if (pGetTabs[i].Current)
							{
								// Store its index
								iActive = pGetTabs[i].Pos;
								// Same as requested?
								if (pGetTabs[i].Pos == anWndIndex)
								{
									lbRc = true;
									break;
								}
							}
						}
						#endif
					}
					// Error reporting
					if (!lbRc)
					{
						swprintf_c(tabs.sTabActivationErr,
							L"Tabs received but wrong tab is active\r\nPID=%u, Req=%i, Cur=%i", dwPID, anWndIndex, iActive);
						LogString(tabs.sTabActivationErr);
					}
				}

				MCHKHEAP;
			}

			pipe.Close();
			// Теперь нужно передернуть сервер, чтобы он перечитал содержимое консоли
			UpdateServerActive(); // пусть будет асинхронно, задержки раздражают

			// И MonitorThread, чтобы он получил новое содержимое
			mh_ApplyFinished.Reset();
			mn_LastConsolePacketIdx--;
			SetMonitorThreadEvent();

			//120412 - не будем ждать. больше задержки раздражают, нежели возможное отставание в отрисовке окна
			//nWait = WaitForSingleObject(mh_ApplyFinished, SETSYNCSIZEAPPLYTIMEOUT);
		}
	}

	return lbRc;
}

bool CRealConsole::IsConsoleThread()
{
	AssertThisRet(false);

	const DWORD dwCurThreadId = GetCurrentThreadId();
	return dwCurThreadId == mn_MonitorThreadID;
}

void CRealConsole::SetForceRead()
{
	SetMonitorThreadEvent();
}

// Вызывается из TabBar->ConEmu
void CRealConsole::ChangeBufferHeightMode(bool abBufferHeight)
{
	AssertThis();

	TODO("Тут бы не высоту менять, а выполнять подмену буфера на длинный вывод последней команды");
	// Пока, для совместимости, оставим как было
	_ASSERTE(mp_ABuf==mp_RBuf);
	mp_ABuf->ChangeBufferHeightMode(abBufferHeight);
}

HANDLE CRealConsole::PrepareOutputFileCreate(wchar_t* pszFilePathName)
{
	wchar_t szTemp[200];
	HANDLE hFile = nullptr;

	if (GetTempPath(200, szTemp))
	{
		if (GetTempFileName(szTemp, L"CEM", 0, pszFilePathName))
		{
			hFile = CreateFile(pszFilePathName, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

			if (hFile == INVALID_HANDLE_VALUE)
			{
				pszFilePathName[0] = 0;
				hFile = nullptr;
			}
		}
	}

	return hFile;
}

bool CRealConsole::PrepareOutputFile(bool abUnicodeText, wchar_t* pszFilePathName)
{
	bool lbRc = false;
	//CESERVER_REQ_HDR In = {0};
	//const CESERVER_REQ *pOut = nullptr;
	//MPipe<CESERVER_REQ_HDR,CESERVER_REQ> Pipe;
	//_ASSERTE(sizeof(In)==sizeof(CESERVER_REQ_HDR));
	//ExecutePrepareCmd(&In, CECMD_GETOUTPUT, sizeof(CESERVER_REQ_HDR));
	//WARNING("Выполнять в главном сервере? Было в ms_ConEmuC_Pipe");
	//Pipe.InitName(mp_ConEmu->GetDefaultTitle(), L"%s", ms_MainSrv_Pipe, 0);

	//if (!Pipe.Transact(&In, In.cbSize, &pOut))
	//{
	//	MBoxA(Pipe.GetErrorText());
	//	return FALSE;
	//}

	MFileMapping<CESERVER_CONSAVE_MAPHDR> StoredOutputHdr;
	MFileMapping<CESERVER_CONSAVE_MAP> StoredOutputItem;

	CESERVER_CONSAVE_MAPHDR* pHdr = nullptr;
	CESERVER_CONSAVE_MAP* pData = nullptr;

	// #AltBuffer Load alt buffer from Console Server (CECMD_GETOUTPUT / cmd_GetOutput)
	StoredOutputHdr.InitName(CECONOUTPUTNAME, LODWORD(hConWnd)); //-V205
	if (!(pHdr = StoredOutputHdr.Open()) || !pHdr->sCurrentMap[0])
	{
		DisplayLastError(L"Stored output mapping was not created!");
		return FALSE;
	}

	DWORD cchMaxBufferSize = std::min(pHdr->MaxCellCount, (DWORD)(pHdr->info.dwSize.X * pHdr->info.dwSize.Y));

	StoredOutputItem.InitName(pHdr->sCurrentMap); //-V205
	size_t nMaxSize = sizeof(*pData) + cchMaxBufferSize * sizeof(pData->Data[0]);
	if (!(pData = StoredOutputItem.Open(FALSE,nMaxSize)))
	{
		DisplayLastError(L"Stored output data mapping was not created!");
		return FALSE;
	}

	CONSOLE_SCREEN_BUFFER_INFO storedSbi = pData->info;



	HANDLE hFile = PrepareOutputFileCreate(pszFilePathName);
	lbRc = (hFile != nullptr);

	if ((pData->hdr.nVersion == CESERVER_REQ_VER) && (pData->hdr.cbSize > sizeof(CESERVER_CONSAVE_MAP)))
	{
		//const CESERVER_CONSAVE* pSave = (CESERVER_CONSAVE*)pOut;
		UINT nWidth = storedSbi.dwSize.X;
		UINT nHeight = storedSbi.dwSize.Y;
		size_t cchMaxCount = std::min<size_t>((nWidth*nHeight), pData->MaxCellCount);
		//const wchar_t* pwszCur = pSave->Data;
		//const wchar_t* pwszEnd = (const wchar_t*)(((LPBYTE)pOut)+pOut->hdr.cbSize);
		CHAR_INFO* ptrCur = pData->Data;
		CHAR_INFO* ptrEnd = ptrCur + cchMaxCount;

		//if (pOut->hdr.nVersion == CESERVER_REQ_VER && nWidth && nHeight && (pwszCur < pwszEnd))
		if (nWidth && nHeight && pData->Succeeded)
		{
			DWORD dwWritten;
			char *pszAnsi = nullptr;
			const CHAR_INFO* ptrRn = nullptr;
			wchar_t *pszBuf = (wchar_t*)malloc((nWidth+1)*sizeof(*pszBuf));
			pszBuf[nWidth] = 0;

			if (!abUnicodeText)
			{
				pszAnsi = (char*)malloc(nWidth+1);
				pszAnsi[nWidth] = 0;
			}
			else
			{
				WORD dwTag = 0xFEFF; //BOM
				WriteFile(hFile, &dwTag, 2, &dwWritten, 0);
			}

			BOOL lbHeader = TRUE;

			for (UINT y = 0; y < nHeight && ptrCur < ptrEnd; y++)
			{
				UINT nCurLen = 0;
				ptrRn = ptrCur + nWidth - 1;

				while (ptrRn >= ptrCur && ptrRn->Char.UnicodeChar == L' ')
				{
					ptrRn --;
				}

				nCurLen = ptrRn - ptrCur + 1;

				if (nCurLen > 0 || !lbHeader)    // Первые N строк если они пустые - не показывать
				{
					if (lbHeader)
					{
						lbHeader = FALSE;
					}
					else if (nCurLen == 0)
					{
						// Если ниже строк нет - больше ничего не писать
						ptrRn = ptrCur + nWidth;

						while (ptrRn < ptrEnd && ptrRn->Char.UnicodeChar == L' ') ptrRn ++;

						if (ptrRn >= ptrEnd) break;  // Заполненных строк больше нет
					}

					CHAR_INFO* ptrSrc = ptrCur;
					wchar_t* ptrDst = pszBuf;
					for (UINT i = 0; i < nCurLen; i++, ptrSrc++)
						*(ptrDst++) = ptrSrc->Char.UnicodeChar;

					if (abUnicodeText)
					{
						if (nCurLen>0)
							WriteFile(hFile, pszBuf, nCurLen*2, &dwWritten, 0);

						WriteFile(hFile, L"\r\n", 2*sizeof(wchar_t), &dwWritten, 0); //-V112
					}
					else
					{
						nCurLen = WideCharToMultiByte(CP_ACP, 0, pszBuf, nCurLen, pszAnsi, nWidth, 0,0);

						if (nCurLen>0)
							WriteFile(hFile, pszAnsi, nCurLen, &dwWritten, 0);

						WriteFile(hFile, "\r\n", 2, &dwWritten, 0);
					}
				}

				ptrCur += nWidth;
			}
			SafeFree(pszAnsi);
			SafeFree(pszBuf);
		}
	}

	if (hFile != nullptr && hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);

	//if (pOut && (LPVOID)pOut != (LPVOID)cbReadBuf)
	//    free(pOut);
	return lbRc;
}

void CRealConsole::SwitchKeyboardLayout(WPARAM wParam, DWORD_PTR dwNewKeyboardLayout)
{
	AssertThis();

	if (m_ChildGui.hGuiWnd && dwNewKeyboardLayout)
	{
		#ifdef _DEBUG
		WCHAR szMsg[255];
		swprintf_c(szMsg, L"SwitchKeyboardLayout/GUIChild(CP:%i, HKL:0x" WIN3264TEST(L"%08X",L"%X%08X") L")\n",
				  (DWORD)wParam, WIN3264WSPRINT(dwNewKeyboardLayout));
		DEBUGSTRLANG(szMsg);
		#endif

		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_LANGCHANGE, sizeof(CESERVER_REQ_HDR) + sizeof(DWORD));
		if (pIn)
		{
			pIn->dwData[0] = (DWORD)dwNewKeyboardLayout;

			DWORD dwTickStart = timeGetTime();

			CESERVER_REQ *pOut = ExecuteHkCmd(m_ChildGui.Process.ProcessID, pIn, ghWnd);

			CSetPgDebug::debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteHkCmd", pOut);

			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}

		DEBUGSTRLANG(L"SwitchKeyboardLayout/GUIChild - finished\n");
	}

	if (ms_ConEmuC_Pipe[0] == 0) return;

	if (!hConWnd) return;

	//#ifdef _DEBUG
	//	WCHAR szMsg[255];
	//	swprintf_c(szMsg, L"CRealConsole::SwitchKeyboardLayout(CP:%i, HKL:0x" WIN3264TEST(L"%08X",L"%X%08X") L")\n",
	//	          (DWORD)wParam, WIN3264WSPRINT(dwNewKeyboardLayout));
	//	DEBUGSTRLANG(szMsg);
	//#endif

	if (gpSet->isLogging(2))
	{
		WCHAR szInfo[255];
		swprintf_c(szInfo, L"CRealConsole::SwitchKeyboardLayout(CP:%i, HKL:0x%08I64X)",
		          (DWORD)wParam, (unsigned __int64)(DWORD_PTR)dwNewKeyboardLayout);
		LogString(szInfo);
	}

	// Сразу запомнить новое значение, чтобы не циклиться
	mp_RBuf->SetKeybLayout(dwNewKeyboardLayout);

	#ifdef _DEBUG
	WCHAR szMsg[255];
	swprintf_c(szMsg, L"SwitchKeyboardLayout/Console(CP:%i, HKL:0x" WIN3264TEST(L"%08X",L"%X%08X") L")\n",
			  (DWORD)wParam, WIN3264WSPRINT(dwNewKeyboardLayout));
	DEBUGSTRLANG(szMsg);
	#endif

	// В FAR при XLat делается так:
	//PostConsoleMessageW(hFarWnd,WM_INPUTLANGCHANGEREQUEST, INPUTLANGCHANGE_FORWARD, 0);
	PostConsoleMessage(hConWnd, WM_INPUTLANGCHANGEREQUEST, wParam, static_cast<LPARAM>(dwNewKeyboardLayout));
}

void CRealConsole::Paste(CEPasteMode PasteMode /*= pm_Standard*/, LPCWSTR asText /*= nullptr*/, bool abNoConfirm /*= false*/, PosixPasteMode posixMode /*= pxm_Auto*/)
{
	AssertThis();

	if (!hConWnd)
	{
		_ASSERTE(FALSE && "Must not get here, use mpsz_PostCreateMacro to postpone macroses");
		return;
	}

	WARNING("warning: Хорошо бы слямзить из ClipFixer кусочек по корректировке текста. Настройка?");

#if 0
	// Не будем пользоваться встроенным WinConsole функционалом. Мало контроля.
	PostConsoleMessage(hConWnd, WM_COMMAND, SC_PASTE_SECRET, 0);
#else

	// Reset selection if it was present
	if (isSelectionPresent())
		mp_ABuf->DoSelectionFinalize(false);

	CEStr buffer;

	if (asText != nullptr)
	{
		if (!*asText)
			return;
		buffer = lstrdup(asText, 1); // Reserve memory for space-termination
	}
	else
	{
		HGLOBAL hglb = nullptr;
		LPCWSTR lptstr = nullptr;
		wchar_t szErr[256] = {}; DWORD nErrCode = 0;

		if (MyOpenClipboard(L"GetClipboard"))
		{
			buffer = GetClipboardText(nErrCode, szErr, countof(szErr));
			MyCloseClipboard();
		}

		if (buffer.IsEmpty())
		{
			if (*szErr)
				DisplayLastError(szErr, nErrCode);
			return;
		}
	}

	if (buffer.IsNull())
	{
		gpConEmu->LogString(L"pszBuf is nullptr, nothing to paste");
		return;
	}
	else if (buffer.IsEmpty())
	{
		gpConEmu->LogString(L"pszBuf is empty, nothing to paste");
		return;
	}

	wchar_t szMsg[256];
	auto nBufLen = buffer.GetLen();

	// Смотрим первую строку / наличие второй
	wchar_t* pszRN = wcspbrk(buffer.data(), L"\r\n");

	// If user has enabled AutoTrimSingleLine, check if we are pasting a single line string.
	// If so, set PasteMode to pm_OneLine to remove trailing newlines.
	if (pszRN && gpSet->isAutoTrimSingleLine) {
		wchar_t* pszNext = pszRN;
		while ((*pszNext == L'\r') || (*pszNext == L'\n')) {
			pszNext++;
		}
		if (!wcspbrk(pszNext, L"\r\n")) {
			PasteMode = pm_OneLine;
		}
	}

	if (PasteMode == pm_OneLine)
	{
		const AppSettings* pApp = gpSet->GetAppSettings(GetActiveAppSettingsId());
		const auto flags = MakeOneLinerFlags::None
			| (pApp && (pApp->CTSTrimTrailing() != 0) ? MakeOneLinerFlags::TrimTailing : MakeOneLinerFlags::None);

		CEStr oneLinerBuffer = MakeOneLinerString(buffer, flags);
		if (oneLinerBuffer.IsEmpty())
		{
			gpConEmu->LogString(L"oneLinerBuffer is empty, nothing to paste");
			return;
		}

		buffer = std::move(oneLinerBuffer);
		nBufLen = buffer.GetLen();

		// Buffer must not contain any line-feeds now! Safe for paste in command line!
		_ASSERTE(wcspbrk(buffer.c_str(), L"\r\n\t") == nullptr);
		pszRN = nullptr;
	}
	else if (pszRN)
	{
		if (PasteMode == pm_FirstLine)
		{
			_ASSERTE(buffer.c_str() <= pszRN && pszRN < (buffer.c_str() + buffer.GetLen()));
			*pszRN = L'\0'; // this is our own memory block
			nBufLen = pszRN - buffer.c_str();
		}
		else if (gpSet->isPasteConfirmEnter && !abNoConfirm)
		{
			wcscpy_c(szMsg, CLngRc::getRsrc(lng_PasteEnterConfirm/*"Pasting text involves <Enter> keypress!\nContinue?"*/));

			if (MsgBox(szMsg, MB_OKCANCEL|MB_ICONEXCLAMATION, GetTitle()) != IDOK)
			{
				goto wrap;
			}
		}
	}

	// Convert Windows style path from clipboard to cygwin style?
	// If clipboard contains double-quoted-path -> IsFilePath returns false -> paste content unchanged
	if (!buffer.IsEmpty()
			// if POSIX was explicitly requested
		&& ((posixMode == pxm_Convert)
			// or was NOT prohibited and auto conversion is allowed
			|| ((posixMode != pxm_Intact) && isPosixConvertAllowed() && isUnixFS()
				&& ((PasteMode == pm_FirstLine) || (PasteMode == pm_OneLine) || !(wcschr(buffer.c_str(), L'\n') || wcschr(buffer.c_str(), L'\r')))
			))
			// check the path validity at last
		&& IsFilePath(buffer.c_str(), true))
	{
		CEStr szPosix;
		if (DupCygwinPath(buffer.c_str(), true, GetMntPrefix(), szPosix))
		{
			buffer = std::move(szPosix);
			nBufLen = buffer.GetLen();
		}
	}

	if (gpSet->nPasteConfirmLonger && !abNoConfirm && (nBufLen > (ssize_t)gpSet->nPasteConfirmLonger))
	{
		swprintf_c(szMsg,
			CLngRc::getRsrc(lng_PasteLongConfirm/*"Pasting text length is %u chars!\nContinue?"*/),
			LODWORD(nBufLen));

		if (MsgBox(szMsg, MB_OKCANCEL|MB_ICONQUESTION, GetTitle()) != IDOK)
		{
			goto wrap;
		}
	}

	if (nBufLen && m_Term.bBracketedPaste)
	{
		CEStr bracketedText(L"\x1B[200~", buffer.c_str(), L"\x1B[201~");
		if (bracketedText.IsEmpty())
		{
			_ASSERTE(bracketedText.IsEmpty());
		}
		else
		{
			buffer = std::move(bracketedText);
			_ASSERTE(buffer.GetLen() == (nBufLen+12));
			nBufLen += 12;
		}
	}

	// Send to the console all symbols from buffer
	if (nBufLen)
	{
		PostString(buffer.data(), nBufLen, PostStringFlags::AllowGroup);
	}
	else
	{
		_ASSERTE(nBufLen);
		gpConEmu->LogString(L"Paste fails, pszEnd <= pszBuf");
	}

wrap:
	gpConEmu->LogString(L"Paste done");
#endif
}

// !!! USE CAREFULLY !!!
// This function writes directly to console surface!
// So, it may cause a rubbish, if console has running applications!
bool CRealConsole::Write(LPCWSTR pszText, int nLen /*= -1*/, DWORD* pnWritten /*= nullptr*/)
{
	AssertThisRet(false);

	if (nLen == -1)
		nLen = wcslen(pszText);
	if (nLen <= 0)
		return false;

	DWORD dwServerPID = GetServerPID(false);
	if (!dwServerPID)
		return false;

	bool lbRc = false;
	DWORD cbData = (nLen + 1) * sizeof(pszText[0]);

	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_WRITETEXT, sizeof(CESERVER_REQ_HDR)+cbData);
	if (pIn)
	{
		wmemmove((wchar_t*)pIn->wData, pszText, nLen);
		pIn->wData[nLen] = 0;

		CESERVER_REQ* pOut = ExecuteSrvCmd(dwServerPID, pIn, ghWnd);
		if (pOut->DataSize() >= sizeof(DWORD))
		{
			if (pnWritten)
				*pnWritten = pOut->dwData[0];
			lbRc = (pOut->dwData[0] != 0);
		}
		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);
	}

	return lbRc;
}

bool CRealConsole::isConsoleClosing()
{
	if (!mp_ConEmu->isValid(this))
		return true;

	if (m_ServerClosing.nServerPID
	        && m_ServerClosing.nServerPID == mn_MainSrv_PID
	        && (GetTickCount() - m_ServerClosing.nRecieveTick) >= SERVERCLOSETIMEOUT)
	{
		// Видимо, сервер повис во время выхода? Но проверим, вдруг он все-таки успел завершиться?
		if (isServerAlive())
		{
			wchar_t szText[255];
			swprintf_c(szText, L"Server PID=%u hangs during termination. Force kill triggered!", mn_MainSrv_PID);
			LogString(szText);

			#ifdef _DEBUG
			wchar_t szTitle[128];
			swprintf_c(szTitle, L"ConEmu, PID=%u", GetCurrentProcessId());
			swprintf_c(szText,
			          L"This is Debug message.\n\nServer hung. PID=%u\nm_ServerClosing.nServerPID=%u\n\nPress Ok to terminate server",
			          mn_MainSrv_PID, m_ServerClosing.nServerPID);
			MsgBox(szText, MB_ICONSTOP|MB_SYSTEMMODAL, szTitle);
			#endif

			if (mh_MainSrv)
			{
				TerminateProcess(mh_MainSrv, 100);
			}
		}

		return true;
	}

	if ((hConWnd == nullptr) || mb_InCloseConsole)
		return true;

	return false;
}

bool CRealConsole::isConsoleReady()
{
	AssertThisRet(false);
	if (hConWnd && mn_MainSrv_PID && mb_MainSrv_Ready)
		return true;
	return false;
}

void CRealConsole::CloseConfirmReset()
{
	mn_CloseConfirmedTick = 0;
	mb_CloseFarMacroPosted = false;
}

bool CRealConsole::isCloseTabConfirmed(CEFarWindowType TabType, LPCWSTR asConfirmation, bool bForceAsk /*= false*/)
{
	_ASSERTE(TabType && ((TabType & fwt_TypeMask) == TabType));

	// Simple console or Far panels
	if (((TabType == fwt_Panels)
			&& !(mp_ConEmu->CloseConfirmFlags() & Settings::cc_Console)
			&& !((mp_ConEmu->CloseConfirmFlags() & Settings::cc_Running) && GetRunningPID()))
		// Far Manager editors/viewers
		|| (((TabType == fwt_Viewer) || (TabType == fwt_Editor))
			&& !(mp_ConEmu->CloseConfirmFlags() & Settings::cc_FarEV))
		)
	{
		return true;
	}

	if (!bForceAsk && mp_ConEmu->isCloseConfirmed())
	{
		return true;
	}

	int nBtn = 0;
	{
		ConfirmCloseParam Parm;
		Parm.nConsoles = 1;
		Parm.nOperations = ((GetProgress(nullptr,nullptr)>=0)
			|| ((mp_ConEmu->CloseConfirmFlags() & Settings::cc_Running) && GetRunningPID()))
			? 1 : 0;
		Parm.nUnsavedEditors = GetModifiedEditors();
		Parm.asSingleConsole = asConfirmation ? asConfirmation : m_ChildGui.isGuiWnd() ? gsCloseGui : gsCloseCon;
		Parm.asSingleTitle = Title;

		nBtn = ConfirmCloseConsoles(Parm);

		//DontEnable de;
		////nBtn = MessageBox(gbMessagingStarted ? ghWnd : nullptr, szMsg, Title, MB_ICONEXCLAMATION|MB_YESNOCANCEL);
		//nBtn = MessageBox(gbMessagingStarted ? ghWnd : nullptr,
		//	asConfirmation ? asConfirmation : gsCloseAny, Title, MB_OKCANCEL|MB_ICONEXCLAMATION);
	}

	if (nBtn != IDYES)
	{
		CloseConfirmReset();
		return false;
	}

	mn_CloseConfirmedTick = GetTickCount();
	return true;
}

void CRealConsole::CloseConsoleWindow(bool abConfirm)
{
	if (abConfirm)
	{
		if (!isCloseTabConfirmed(fwt_Panels, m_ChildGui.isGuiWnd() ? gsCloseGui : gsCloseCon))
			return;
	}

	SetInCloseConsole(true);

	if (m_ChildGui.isGuiWnd())
	{
		PostConsoleMessage(m_ChildGui.hGuiWnd, WM_CLOSE, 0, 0);
	}
	else
	{
		PostConsoleMessage(hConWnd, WM_CLOSE, 0, 0);
	}
}

bool CRealConsole::TerminateAllButShell(bool abConfirm)
{
	DWORD dwServerPID = GetServerPID(true);
	if (!dwServerPID)
	{
		// No server
		return false;
	}

	const wchar_t sMsgTitle[] = L"Terminate all but shell";

	ConProcess* pPrc = nullptr;
	int nCount = GetProcesses(&pPrc, true/*ClientOnly*/);
	// If console has only shell
	if (!pPrc || (nCount < 1))
	{
		MsgBox(L"GetProcesses fails", MB_OKCANCEL|MB_SYSTEMMODAL, sMsgTitle);
		return false;
	}
	// Some users are starting "far.exe" from batch files
	if ((nCount == 1)
		|| ((nCount == 2) && IsCmdProcessor(pPrc[0].Name) && IsFarExe(pPrc[1].Name))
		)
	{
		// MsgBox(L"Running process was not detected", MB_OKCANCEL|MB_SYSTEMMODAL, sMsgTitle);
		// Let ask user if he wants to kill active process even it is a shell
		CloseConsole(true/*abForceTerminate*/, true/*abConfirm*/, false/*abAllowMacro*/);
		return false;
	}

	if (abConfirm)
	{
		ConfirmCloseParam Parm;
		Parm.nConsoles = 1;
		Parm.nOperations = (GetProgress(nullptr,nullptr)>=0) ? 1 : 0;
		Parm.nUnsavedEditors = GetModifiedEditors();
		Parm.asSingleConsole = gsTerminateAllButShell;
		Parm.asSingleTitle = Title;

		int nBtn = ConfirmCloseConsoles(Parm);

		if (nBtn != IDYES)
		{
			free(pPrc);
			return false;
		}
	}

	// Allocate enough storage
	CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_TERMINATEPID, sizeof(CESERVER_REQ_HDR)+(1+nCount)*sizeof(DWORD));
	if (!pIn)
	{
		// Not enough memory? System fails
		return false;
	}

	int iFrom = 1; // Skip the shell (pPrc[0].Name is cmd.exe, far.exe, ipython, and so on)
	if ((nCount >= 2) && (0==lstrcmpi(pPrc[0].Name, L"cmd.exe")) && IsFarExe(pPrc[1].Name))
		iFrom++;   // the "shell" is "far.exe" started from batch
	if ((nCount > iFrom) && IsFarExe(pPrc[iFrom-1].Name) && IsConsoleServer(pPrc[iFrom].Name))
		iFrom++;   // ConEmuC was started from "far.exe" as "ComSpec"

	int iKillCount = 0;
	for (int i = iFrom; i < nCount; i++)
	{
		pIn->dwData[++iKillCount] = pPrc[i].ProcessID;
	}
	free(pPrc);
	pIn->dwData[0] = iKillCount;

	DEBUGTEST(DWORD dwTickStart = timeGetTime());

	//Terminate
	CESERVER_REQ *pOut = ExecuteSrvCmd(dwServerPID, pIn, ghWnd);

	bool bOk = (pOut && (pOut->DataSize() > 2*sizeof(DWORD)) && (pOut->dwData[0] != 0));

	DEBUGTEST(DWORD dwTickEnd = timeGetTime());
	ExecuteFreeResult(pOut);
	ExecuteFreeResult(pIn);

	return bOk;
}

bool CRealConsole::TerminateActiveProcessConfirm(DWORD nPID)
{
	int nBtn;

	DWORD nActivePID = nPID ? nPID : GetActivePID();

	ConProcess prc = {};
	if (!GetProcessInformation(nActivePID, &prc) || !(prc.Name[0]))
		wcscpy_c(prc.Name, L"<Not found>");

	// Confirm termination
	wchar_t szMsg[255];
	swprintf_c(szMsg,
		L"Kill active process '%s' PID=%u?",
		prc.Name, nActivePID);

	{
		DontEnable de;
		nBtn = MsgBox(szMsg, MB_ICONEXCLAMATION | MB_OKCANCEL, Title, gbMessagingStarted ? ghWnd : nullptr, false);
	}

	return (nBtn == IDOK);
}

bool CRealConsole::TerminateActiveProcess(bool abConfirm, DWORD nPID)
{
	DWORD dwServerPID = GetServerPID(true);
	if (!dwServerPID)
	{
		// No server
		return false;
	}

	DWORD nActivePID = nPID ? nPID : GetActivePID();

	if (abConfirm)
	{
		if (!TerminateActiveProcessConfirm(nActivePID))
		{
			return false;
		}
	}

	bool lbTerminateSucceeded = false;
	wchar_t szMsg[128];

	//Terminate
	CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_TERMINATEPID, sizeof(CESERVER_REQ_HDR) + 2 * sizeof(DWORD));
	if (pIn)
	{
		pIn->dwData[0] = 1; // Count
		pIn->dwData[1] = nActivePID;
		DWORD dwTickStart = timeGetTime();

		CESERVER_REQ *pOut = ExecuteSrvCmd(dwServerPID, pIn, ghWnd);

		CSetPgDebug::debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime() - dwTickStart, L"ExecuteSrvCmd", pOut);

		if (pOut)
		{
			if (pOut->hdr.cbSize == sizeof(CESERVER_REQ_HDR) + 2 * sizeof(DWORD))
			{
				lbTerminateSucceeded = true;

				// Show error message if failed?
				if (pOut->dwData[0] == FALSE)
				{
					swprintf_c(szMsg, L"TerminateProcess(%u) failed", nPID);
					DisplayLastError(szMsg, pOut->dwData[1]);
				}
			}
			ExecuteFreeResult(pOut);
		}
		ExecuteFreeResult(pIn);
	}

	return lbTerminateSucceeded;
}

void CRealConsole::CloseConsole(bool abForceTerminate, bool abConfirm, bool abAllowMacro /*= true*/)
{
	AssertThis();

	_ASSERTE(!mb_ProcessRestarted);

	LogString(L"CRealConsole::CloseConsole");

	// Для Terminate - спрашиваем отдельно
	// Don't show confirmation dialog, if this method was reentered (via GuiMacro call)
	if (abConfirm && !abForceTerminate && !mb_InPostCloseMacro)
	{
		if (!isCloseTabConfirmed(fwt_Panels, nullptr))
			return;
	}
	#ifdef _DEBUG
	else
	{
		// при вызове из RecreateProcess, SC_SYSCLOSE (там уже спросили)
		UNREFERENCED_PARAMETER(abConfirm);
	}
	#endif

	ShutdownGuiStep(L"Closing console window");

	// Сброс background
	CESERVER_REQ_SETBACKGROUND BackClear = {};
	bool lbCleared = false;
	//mp_VCon->SetBackgroundImageData(&BackClear);

	if (abAllowMacro && mb_InPostCloseMacro)
	{
		abAllowMacro = false;
		LogString(L"Post close macro in progress, disabling abAllowMacro");
	}

	if (hConWnd)
	{
		if (!IsWindow(hConWnd))
		{
			_ASSERTE(FALSE && !mb_WasForceTerminated && "Console window was abnormally terminated?");
			return;
		}

		if (gpSet->isSafeFarClose && !abForceTerminate && abAllowMacro)
		{
			bool lbExecuted = false;

			LPCWSTR pszMacro = gpSet->SafeFarCloseMacro(IsFarLua() ? fmv_Lua : fmv_Default);
			_ASSERTE(pszMacro && *pszMacro);

			// GuiMacro выполняется безотносительно "фар/не фар"
			if (*pszMacro == GUI_MACRO_PREFIX/*L'#'*/)
			{
				mb_InPostCloseMacro = true;
				// Для удобства. Она сама позовет CConEmuMacro::ExecuteMacro
				PostMacro(pszMacro, TRUE);
				lbExecuted = true;
			}

			DWORD nFarPID;
			if (!lbExecuted && ((nFarPID = GetFarPID(TRUE/*abPluginRequired*/)) != 0))
			{
				// Set flag immediately
				mb_InPostCloseMacro = true;

				// FarMacro выполняется асинхронно, так что не будем смотреть на "isAlive"
				{
					SetInCloseConsole(true);
					mp_ConEmu->DebugStep(_T("ConEmu: Execute SafeFarCloseMacro"));

					if (!lbCleared)
					{
						lbCleared = true;
						mp_VCon->SetBackgroundImageData(&BackClear);
					}

					// Async, иначе ConEmu зависнуть может
					PostMacro(pszMacro, TRUE);

					lbExecuted = true;

					mp_ConEmu->DebugStep(nullptr);
				}

				// Don't return to false, otherwise it is impossible
				// to close "Hanged Far instance"
				// -- // Post failed? Continue to simple close
				//mb_InPostCloseMacro = false;

				if (lbExecuted)
					return;
			}
		} // if (gpSet->isSafeFarClose && !abForceTerminate && abAllowMacro)

		DWORD nActivePID;
		if (abForceTerminate && !mn_InRecreate
			&& ((nActivePID = GetActivePID()) != 0))
		{
			if (abConfirm)
			{
				if (!TerminateActiveProcessConfirm(nActivePID))
				{
					return;
				}
			}

			// Doesn't matter, if we succeeded with termination,
			// continue to close console *window* to finalize our pane
			TerminateActiveProcess(false, nActivePID);
		}

		if (!lbCleared)
		{
			lbCleared = true;
			mp_VCon->SetBackgroundImageData(&BackClear);
		}

		CloseConsoleWindow(false/*NoConfirm - already confirmed*/);
	}
	else
	{
		m_Args.Detached = crb_Off;

		if (mp_VCon)
			CConEmuChild::ProcessVConClosed(mp_VCon);
	}
}

// Check if tab (active console/Far Editor or Viewer) may be closed with Macro?
bool CRealConsole::CanCloseTab(bool abPluginRequired /*= false*/)
{
	// Far Manager plugin is required?
	if (abPluginRequired)
	{
		// No plugin -> no macro -> we can't close tab
		if (!isFar(true/* abPluginRequired */))
			return false;
	}

	// Tab may be closed as usual (by closing real console window)
	return true;
}

// для фара - Мягко (с подтверждением) закрыть текущий таб.
// для GUI  - WM_CLOSE, пусть само разбирается
// для остальных (cmd.exe, и т.п.) WM_CLOSE в консоль. Скорее всего, она закроется сразу
void CRealConsole::CloseTab()
{
	AssertThis();

	// Если консоль "зависла" после рестарта (или на старте)
	// То нет смысла пытаться что-то послать в сервер, которого нет
	if (!mn_MainSrv_PID && !mh_MainSrv)
	{
		//Sleep(500); // Подождать чуть-чуть, может сервер все-таки появится?

		StopSignal();

		return;
	}

	if (GuiWnd())
	{
		if (!isCloseTabConfirmed(fwt_Panels, gsCloseGui))
			return;
		// It will send WM_CLOSE to ChildGui main window
		CloseConsoleWindow(false/*NoConfirm - already confirmed*/);
	}
	else
	{
		bool bForceTerminate = false;
		CEFarWindowType tabtype = GetActiveTabType();
		// Check, if we may send Macro to close Far (is it Far tab?) with Far macros
		bool bCanCloseMacro = false;
		if (((tabtype & fwt_TypeMask) == fwt_Editor)
			|| ((tabtype & fwt_TypeMask) == fwt_Viewer)
			)
		{
			bCanCloseMacro = CanCloseTab(true);
		}

		if (bCanCloseMacro && !isAlive())
		{
			wchar_t szInfo[255];
			swprintf_c(szInfo,
				L"Far Manager (PID=%u) is not alive.\n"
				L"Close realconsole window instead of posting Macro?",
				GetFarPID(TRUE));
			int nBrc = MsgBox(szInfo, MB_ICONEXCLAMATION|MB_YESNOCANCEL, mp_ConEmu->GetDefaultTitle());
			switch (nBrc)
			{
			case IDCANCEL:
				// Отмена
				return;
			case IDYES:
				bCanCloseMacro = false;
				break;
			}

			if (bCanCloseMacro)
			{
				LPCWSTR pszConfirmText =
					((tabtype & fwt_TypeMask) == fwt_Editor) ? gsCloseEditor :
					((tabtype & fwt_TypeMask) == fwt_Viewer) ? gsCloseViewer :
					gsCloseCon;

				if (!isCloseTabConfirmed((tabtype & fwt_TypeMask), pszConfirmText))
				{
					// Отмена
					return;
				}
			}
		}
		else
		{
			bool bSkipConfirm = false;
			LPCWSTR pszConfirmText = gsCloseCon;
			if (bCanCloseMacro)
			{
				pszConfirmText =
					((tabtype & fwt_TypeMask) == fwt_Editor) ? gsCloseEditor :
					((tabtype & fwt_TypeMask) == fwt_Viewer) ? gsCloseViewer :
					gsCloseCon;
				if (!(mp_ConEmu->CloseConfirmFlags() & Settings::cc_FarEV))
				{
					if (((tabtype & fwt_TypeMask) == fwt_Editor) || ((tabtype & fwt_TypeMask) == fwt_Viewer))
					{
						bSkipConfirm = true;
					}
				}
			}

			if (!bSkipConfirm && !isCloseTabConfirmed((tabtype & fwt_TypeMask), pszConfirmText))
			{
				// Отмена
				return;
			}
		}

		if (bCanCloseMacro)
		{
			// Close Far Manager Editors and Viewers
			PostMacro(gpSet->TabCloseMacro(fmv_Default), TRUE/*async*/);
		}
		else
		{
			// Use common function to terminate console
			CloseConsole(bForceTerminate, false);
		}
	}
}

unsigned CRealConsole::TextWidth()
{
	AssertThisRet(MIN_CON_WIDTH);
	return mp_ABuf->TextWidth();
}

unsigned CRealConsole::TextHeight()
{
	AssertThisRet(MIN_CON_HEIGHT);
	return mp_ABuf->TextHeight();
}

unsigned CRealConsole::BufferWidth()
{
	TODO("CRealConsole::BufferWidth()");
	return TextWidth();
}

unsigned CRealConsole::BufferHeight(unsigned nNewBufferHeight/*=0*/)
{
	return mp_ABuf->BufferHeight(nNewBufferHeight);
}

void CRealConsole::OnBufferHeight()
{
	mp_ABuf->OnBufferHeight();
}

bool CRealConsole::isActive(bool abAllowGroup)
{
	AssertThisRet(false);
	if (!mp_VCon)
		return false;
	return mp_VCon->isActive(abAllowGroup);
}

// Проверяет не только активность, но и "в фокусе ли ConEmu"
bool CRealConsole::isInFocus()
{
	AssertThisRet(false);
	if (!mp_VCon)
		return false;

	TODO("DoubleView: когда будет группировка ввода - чтобы курсором мигать во всех консолях");
	if (!isActive(false/*abAllowGroup*/))
		return false;

	if (!mp_ConEmu->isMeForeground(false, true))
		return false;

	return true;
}

bool CRealConsole::isVisible()
{
	AssertThisRet(false);
	if (!mp_VCon)
		return false;

	return mp_VCon->isVisible();
}

// Результат зависит от распознанных регионов!
// В функции есть вложенные вызовы, например,
// mp_ABuf->GetDetector()->GetFlags()
void CRealConsole::CheckFarStates()
{
#ifdef _DEBUG

	if (mb_DebugLocked)
		return;

#endif
	DWORD nLastState = mn_FarStatus;
	DWORD nNewState = (mn_FarStatus & (~CES_FARFLAGS));

	if (!isFar(true))
	{
		nNewState = 0;
		setPreWarningProgress(-1);
	}
	else
	{
		// Сначала пытаемся проверить статус по закладкам: если к нам из плагина пришло,
		// что активен "viewer/editor" - то однозначно ставим CES_VIEWER/CES_EDITOR
		CEFarWindowType nActiveType = (GetActiveTabType() & fwt_TypeMask);

		if (nActiveType != fwt_Panels)
		{
			if (nActiveType == fwt_Viewer)
				nNewState |= CES_VIEWER;
			else if (nActiveType == fwt_Editor)
				nNewState |= CES_EDITOR;
		}

		// Если по закладкам ничего не определили - значит может быть либо панель, либо
		// "viewer/editor" но при отсутствии плагина в фаре
		if ((nNewState & CES_FARFLAGS) == 0)
		{
			// пытаемся определить "viewer/editor" по заголовку окна консоли
			if (wcsncmp(Title, ms_Editor, _tcslen(ms_Editor))==0 || wcsncmp(Title, ms_EditorRus, _tcslen(ms_EditorRus))==0)
				nNewState |= CES_EDITOR;
			else if (wcsncmp(Title, ms_Viewer, _tcslen(ms_Viewer))==0 || wcsncmp(Title, ms_ViewerRus, _tcslen(ms_ViewerRus))==0)
				nNewState |= CES_VIEWER;
			else if (isFilePanel(true, true))
				nNewState |= CES_FILEPANEL;
		}

		// Смысл в том, чтобы не сбросить флажок CES_MAYBEPANEL если в панелях открыт диалог
		// Флажок сбрасывается только в редактроре/вьювере
		if ((nNewState & (CES_EDITOR | CES_VIEWER)) != 0)
			nNewState &= ~(CES_MAYBEPANEL|CES_WASPROGRESS|CES_OPER_ERROR); // При переключении в редактор/вьювер - сбросить CES_MAYBEPANEL

		if ((nNewState & CES_FILEPANEL) == CES_FILEPANEL)
		{
			nNewState |= CES_MAYBEPANEL; // Попали в панель - поставим флажок
			nNewState &= ~(CES_WASPROGRESS|CES_OPER_ERROR); // Значит СЕЙЧАС процесс копирования не идет
		}

		if (m_Progress.Progress >= 0 && m_Progress.Progress <= 100)
		{
			if (m_Progress.ConsoleProgress == m_Progress.Progress)
			{
				// При извлечении прогресса из текста консоли - Warning ловить смысла нет
				setPreWarningProgress(-1);
				nNewState &= ~CES_OPER_ERROR;
				nNewState |= CES_WASPROGRESS; // Пометить статус, что прогресс был
			}
			else
			{
				setPreWarningProgress(m_Progress.Progress);

				if ((nNewState & CES_MAYBEPANEL) == CES_MAYBEPANEL)
					nNewState |= CES_WASPROGRESS; // Пометить статус, что прогресс был

				nNewState &= ~CES_OPER_ERROR;
			}
		}
		else if ((nNewState & (CES_WASPROGRESS|CES_MAYBEPANEL)) == (CES_WASPROGRESS|CES_MAYBEPANEL)
		        && (m_Progress.PreWarningProgress != -1))
		{
			if (m_Progress.LastWarnCheckTick == 0)
			{
				m_Progress.LastWarnCheckTick = GetTickCount();
			}
			else if ((mn_FarStatus & CES_OPER_ERROR) == CES_OPER_ERROR)
			{
				// для удобства отладки - флаг ошибки уже установлен
				_ASSERTE((nNewState & CES_OPER_ERROR) == CES_OPER_ERROR);
				nNewState |= CES_OPER_ERROR;
			}
			else
			{
				DWORD nDelta = GetTickCount() - m_Progress.LastWarnCheckTick;

				if (nDelta > CONSOLEPROGRESSWARNTIMEOUT)
				{
					nNewState |= CES_OPER_ERROR;
					//mn_LastWarnCheckTick = 0;
				}
			}
		}
	}

	if (m_Progress.Progress == -1 && m_Progress.PreWarningProgress != -1)
	{
		if ((nNewState & CES_WASPROGRESS) == 0)
		{
			setPreWarningProgress(-1); m_Progress.LastWarnCheckTick = 0;
			mp_ConEmu->UpdateProgress();
		}
		else if (/*isFilePanel(true)*/ (nNewState & CES_FILEPANEL) == CES_FILEPANEL)
		{
			nNewState &= ~(CES_OPER_ERROR|CES_WASPROGRESS);
			setPreWarningProgress(-1); m_Progress.LastWarnCheckTick = 0;
			mp_ConEmu->UpdateProgress();
		}
	}

	if (nNewState != nLastState)
	{
		#ifdef _DEBUG
		if ((nNewState & CES_FILEPANEL) == 0)
			nNewState = nNewState;  // -V570 for breakpoint
		#endif

		SetFarStatus(nNewState);
		mp_ConEmu->UpdateProcessDisplay(FALSE);
	}
}

// m_Progress.Progress не меняет, результат возвращает
short CRealConsole::CheckProgressInTitle()
{
	// Обработка прогресса NeroCMD и пр. консольных программ (если курсор находится в видимой области)
	// выполняется в CheckProgressInConsole (-> mn_ConsoleProgress), вызывается из FindPanels
	short nNewProgress = -1;
	int i = 0;
	wchar_t ch;

	// Wget [41%] http://....
	while ((ch = Title[i]) != 0)
	{
		i++;
		if (ch == L'{' || ch == L'(' || ch == L'[')
			break;
	}

	//if (Title[0] == L'{' || Title[0] == L'(' || Title[0] == L'[') {
	if (Title[i])
	{
		// Некоторые плагины/программы форматируют проценты по двум пробелам
		if (Title[i] == L' ')
		{
			i++;

			if (Title[i] == L' ')
				i++;
		}

		// Теперь, если цифра - считываем проценты
		if (isDigit(Title[i]))
		{
			if (isDigit(Title[i+1]) && isDigit(Title[i+2])
			        && (Title[i+3] == L'%' || (isDot(Title[i+3]) && isDigit(Title[i+4]) && Title[i+7] == L'%'))
			  )
			{
				// По идее больше 100% быть не должно :)
				nNewProgress = 100*(Title[i] - L'0') + 10*(Title[i+1] - L'0') + (Title[i+2] - L'0');
			}
			else if (isDigit(Title[i+1])
			        && (Title[i+2] == L'%' || (isDot(Title[i+2]) && isDigit(Title[i+3]) && Title[i+4] == L'%'))
			       )
			{
				// 10 .. 99 %
				nNewProgress = 10*(Title[i] - L'0') + (Title[i+1] - L'0');
			}
			else if (Title[i+1] == L'%' || (isDot(Title[i+1]) && isDigit(Title[i+2]) && Title[i+3] == L'%'))
			{
				// 0 .. 9 %
				nNewProgress = (Title[i] - L'0');
			}

			_ASSERTE(nNewProgress<=100);

			if (nNewProgress > 100)
				nNewProgress = 100;
		}
	}

	if (nNewProgress != m_Progress.Progress)
		logProgress(L"RCon::ProgressInTitle: %i", nNewProgress);

	return nNewProgress;
}

void CRealConsole::OnTitleChanged()
{
	AssertThis();

	#ifdef _DEBUG
	if (mb_DebugLocked)
		return;
	#endif

	wcscpy_c(Title, TitleCmp);
	mp_ConEmu->SetTitle(mp_VCon->GetView(), TitleCmp, true);

	if (tabs.nActiveType & (fwt_Viewer|fwt_Editor))
	{
		// После Ctrl-F10 в редакторе фара меняется текущая панель (директория)
		// В заголовке консоли панель фара?
		if ((TitleCmp[0] == L'{')
			&& ((TitleCmp[1] == L'\\' && TitleCmp[2] == L'\\')
				|| (isDriveLetter(TitleCmp[1]) && (TitleCmp[2] == L':' && TitleCmp[3] == L'\\'))))
		{
			if ((wcsstr(TitleCmp, L"} - ") != nullptr)
				&& (wcscmp(ms_PanelTitle, TitleCmp) != 0))
			{
				wcscpy_c(ms_PanelTitle, TitleCmp);
				mp_ConEmu->mp_TabBar->Update();
			}
		}
	}

	TitleFull[0] = 0;
	short nNewProgress = CheckProgressInTitle();

	if (nNewProgress == -1)
	{
		// mn_ConsoleProgress обновляется в FindPanels, должен быть уже вызван
		// mn_AppProgress обновляется через Esc-коды, GuiMacro или через команду пайпа
		const short nConProgr =
			((m_Progress.AppProgressState == AnsiProgressStatus::Running)
				|| (m_Progress.AppProgressState == AnsiProgressStatus::Error)
				|| (m_Progress.AppProgressState == AnsiProgressStatus::Paused)) ? m_Progress.AppProgress
			: (m_Progress.AppProgressState == AnsiProgressStatus::Indeterminate) ? 0
			: m_Progress.ConsoleProgress;

		if ((nConProgr >= 0) && (nConProgr <= 100))
		{
			// Обработка прогресса NeroCMD и пр. консольных программ
			// Если курсор находится в видимой области
			// Use "m_Progress.ConsoleProgress" because "AppProgress" is not "our" detection
			// thats why we are not storing it in (common) member variable
			nNewProgress = m_Progress.ConsoleProgress;

			// Подготовим строку с процентами
			wchar_t szPercents[5];
			if ((nConProgr == 0) && (m_Progress.AppProgressState == AnsiProgressStatus::Indeterminate))
				wcscpy_c(szPercents, L"%%");
			else
				swprintf_c(szPercents, L"%i%%", nConProgr);

			// И если в заголовке нет процентов, добавить их в наш заголовок
			// (проценты есть только в консоли или заданы через Guimacro)
			if (!wcsstr(TitleCmp, szPercents))
			{
				TitleFull[0] = L'{'; TitleFull[1] = 0;
				wcscat_c(TitleFull, szPercents);
				wcscat_c(TitleFull, L"} ");
			}
		}
	}

	wcscat_c(TitleFull, TitleCmp);

	// Обновляем на что нашли
	setProgress(nNewProgress);

	if ((nNewProgress >= 0 && nNewProgress <= 100) && isFar(true))
		setPreWarningProgress(nNewProgress);

	//SetProgress(nNewProgress);

	TitleAdmin[0] = 0;

	CheckFarStates();
	// иначе может среагировать на изменение заголовка ДО того,
	// как мы узнаем, что активировался редактор...
	TODO("Должно срабатывать и при запуске консольной программы!");

	if (Title[0] == L'{' || Title[0] == L'(')
		CheckPanelTitle();

	CTab panels(__FILE__,__LINE__);
	if (tabs.m_Tabs.GetTabByIndex(0, panels))
	{
		if (panels->Type() == fwt_Panels)
		{
			panels->SetName(GetPanelTitle());
		}
	}

	// заменил на GetProgress, т.к. он еще учитывает mn_PreWarningProgress
	nNewProgress = GetProgress(nullptr);

	if (isActive(false) && wcscmp(GetTitle(), mp_ConEmu->GetLastTitle(false)))
	{
		// Для активной консоли - обновляем заголовок. Прогресс обновится там же
		setLastShownProgress(nNewProgress);
		mp_ConEmu->UpdateTitle();
	}
	else if (m_Progress.LastShownProgress != nNewProgress)
	{
		// Для НЕ активной консоли - уведомить главное окно, что у нас сменились проценты
		setLastShownProgress(nNewProgress);
		mp_ConEmu->UpdateProgress();
	}

	mp_VCon->OnTitleChanged(); // Обновить таб на таскбаре

	// Только если табы уже инициализированы
	if (tabs.mn_tabsCount > 0)
	{
		mp_ConEmu->mp_TabBar->Update(); // сменить заголовок закладки?
	}
}

// Если фар запущен как "far /e ..." то панелей в нем вообще нет
bool CRealConsole::isFarPanelAllowed()
{
	AssertThisRet(false);
	// Если текущий процесс НЕ фар - то и проверять нечего, "консоль" считается за "панель"
	DWORD nActivePID = GetActivePID();
	bool  bRootIsFar = IsFarExe(ms_RootProcessName);
	if (!nActivePID && !bRootIsFar)
		return true;
	// Известен PID фара?
	DWORD nFarPID = GetFarPID();
	if (nFarPID)
	{
		// Проверим, получена ли информация из плагина
		const CEFAR_INFO_MAPPING *pFar = GetFarInfo();
		if (pFar)
		{
			// Если получено из плагина
			return (pFar->bFarPanelAllowed != FALSE);
		}
	}
	// Пытаемся разобрать строку аргументов
	if (bRootIsFar && (!nActivePID || (nActivePID == nFarPID)))
	{
		if (mn_FarNoPanelsCheck)
			return (mn_FarNoPanelsCheck == 1);
		CmdArg szArg;
		LPCWSTR pszCmdLine = GetCmd();
		while ((pszCmdLine = NextArg(pszCmdLine, szArg)))
		{
			LPCWSTR ps = szArg.ms_Val;
			if ((ps[0] == L'-' || ps[0] == L'/')
				&& (ps[1] == L'e' || ps[1] == L'E' || ps[1] == L'v' || ps[1] == L'V')
				&& (ps[2] == 0))
			{
				// Считаем что фар запущен как редактор, панелей нет
				mn_FarNoPanelsCheck = 2;
				return false;
			}
		}
		mn_FarNoPanelsCheck = 1;
	}
	// Во всех остальных случаях, считаем что "панели есть"
	return true;
}

DWORD CRealConsole::GetActiveDlgFlags()
{
	DWORD dwFlags = (this && mp_ABuf) ? mp_ABuf->GetDetector()->GetFlags() : FR_FREEDLG_MASK;
	return dwFlags;
}

bool CRealConsole::isFilePanel(bool abPluginAllowed/*=false*/, bool abSkipEditViewCheck /*= false*/, bool abSkipDialogCheck /*= false*/)
{
	AssertThisRet(false);

	if (Title[0] == 0) return false;

	// Функция используется в процессе проверки флагов фара, а там Viewer/Editor уже проверены
	if (!abSkipEditViewCheck)
	{
		if (isEditor() || isViewer())
			return false;
	}

	// Если висят какие-либо диалоги - считаем что это НЕ панель
	DWORD dwFlags = GetActiveDlgFlags();

	// We need to polish panel frames even if there are F2/F11/anything
	if (!abSkipDialogCheck && ((dwFlags & FR_FREEDLG_MASK) != 0))
		return false;

	// нужно для DragDrop
	if (_tcsncmp(Title, ms_TempPanel, _tcslen(ms_TempPanel)) == 0 || _tcsncmp(Title, ms_TempPanelRus, _tcslen(ms_TempPanelRus)) == 0)
		return true;

	if ((abPluginAllowed && Title[0]==_T('{')) ||
	        (_tcsncmp(Title, _T("{\\\\"), 3)==0) ||
	        (Title[0] == _T('{') && isDriveLetter(Title[1]) && Title[2] == _T(':') && Title[3] == _T('\\')))
	{
		TCHAR *Br = _tcsrchr(Title, _T('}'));

		if (Br && _tcsstr(Br, _T("} - Far")))
		{
			if (mp_RBuf->isLeftPanel() || mp_RBuf->isRightPanel())
				return true;
		}
	}

	//TCHAR *BrF = _tcschr(Title, '{'), *BrS = _tcschr(Title, '}'), *Slash = _tcschr(Title, '\\');
	//if (BrF && BrS && Slash && BrF == Title && (Slash == Title+1 || Slash == Title+3))
	//    return true;
	return false;
}

bool CRealConsole::isEditor()
{
	AssertThisRet(false);

	return ((GetFarStatus() & CES_EDITOR) == CES_EDITOR);
}

bool CRealConsole::isEditorModified()
{
	AssertThisRet(false);

	if (!isEditor()) return false;

	if (tabs.mn_tabsCount)
	{
		TabInfo Info;

		for (int j = 0; j < tabs.mn_tabsCount; j++)
		{
			if (tabs.m_Tabs.GetTabInfoByIndex(j, Info) && (Info.Type & fwt_CurrentFarWnd))
			{
				if ((Info.Type & fwt_TypeMask) == fwt_Editor)
				{
					return ((Info.Type & fwt_ModifiedFarWnd) == fwt_ModifiedFarWnd);
				}

				return false;
			}
		}
	}

	return false;
}

bool CRealConsole::isViewer()
{
	AssertThisRet(false);

	return ((GetFarStatus() & CES_VIEWER) == CES_VIEWER);
}

bool CRealConsole::isNtvdm()
{

	if (mn_ProgramStatus & CES_NTVDM)
	{
		// Наличие 16bit определяем ТОЛЬКО по WinEvent. Иначе не получится отсечь его завершение,
		// т.к. процесс ntvdm.exe не выгружается, а остается в памяти.
		return true;
		//if (mn_ProgramStatus & CES_FARFLAGS) {
		//  //mn_ActiveStatus &= ~CES_NTVDM;
		//} else if (isFilePanel()) {
		//  //mn_ActiveStatus &= ~CES_NTVDM;
		//} else {
		//  return true;
		//}
	}

	return false;
}

bool CRealConsole::isFixAndCenter(COORD* lpcrConSize /*= nullptr*/)
{
	if (!isNtvdm())
		return false;

	if (lpcrConSize)
	{
		*lpcrConSize = mp_RBuf->GetDefaultNtvdmHeight();
	}

	return true;
}

const RConStartArgsEx& CRealConsole::GetArgs()
{
	return m_Args;
}

void CRealConsole::SetPaletteName(LPCWSTR asPaletteName)
{
	CEStr pszNew(asPaletteName);
	pszNew.Swap(m_Args.pszPalette);
	_ASSERTE(!mp_VCon->m_SelfPalette.bPredefined);
	PrepareDefaultColors();
}

LPCWSTR CRealConsole::GetCmd(bool bThisOnly /*= false*/)
{
	AssertThisRet(L"");

	if (m_Args.pszSpecialCmd)
		return m_Args.pszSpecialCmd;
	else if (!bThisOnly)
		return gpConEmu->GetCmd(nullptr, true);
	else
		return L"";
}

CEStr CRealConsole::CreateCommandLine(const bool abForTasks /*= false*/)
{
	AssertThisRet(nullptr);

	// m_Args.pszStartupDir is used in GetStartupDir()
	// thats why we save the value before showing the current one
	wchar_t* pszDirSave = m_Args.pszStartupDir;
	CEStr szCurDir;
	m_Args.pszStartupDir = GetConsoleCurDir(szCurDir, true) ? szCurDir.ms_Val : nullptr;

	SafeFree(m_Args.pszRenameTab);
	CTab tab(__FILE__,__LINE__);
	if (GetTab(0, tab) && (tab->Flags() & fwt_Renamed))
	{
		const auto* pszRenamed = tab->Renamed.Ptr();
		if (pszRenamed && *pszRenamed)
		{
			m_Args.pszRenameTab = lstrdup(pszRenamed).Detach();
		}
	}

	CEStr pszCmd = m_Args.CreateCommandLine(abForTasks);

	m_Args.pszStartupDir = pszDirSave;

	return pszCmd;
}

bool CRealConsole::GetUserPwd(const wchar_t*& rpszUser, const wchar_t*& rpszDomain, bool& rbRestricted) const
{
	if (m_Args.RunAsRestricted == crb_On)
	{
		rbRestricted = true;
		rpszDomain = nullptr;
		rpszUser = nullptr;
		return true;
	}

	if (m_Args.pszUserName /*&& m_Args.pszUserPassword*/)
	{
		rpszUser = m_Args.pszUserName;
		rpszDomain = m_Args.pszDomain;
		rbRestricted = false;
		return true;
	}

	return false;
}

void CRealConsole::logProgress(LPCWSTR asFormat, int V1, int V2)
{
	#ifndef _DEBUG
	if (!isLogging())
		return;
	#endif

	wchar_t szInfo[100];
	swprintf_c(szInfo, countof(szInfo)-1/*#SECURELEN*/, asFormat, V1, V2);

	#ifdef _DEBUG
	if (!mp_Log)
	{
		DEBUGSTRPROGRESS(szInfo);
		return;
	}
	#endif
	LogString(szInfo);
}

void CRealConsole::setProgress(short value)
{
	if (m_Progress.Progress != value)
	{
		logProgress(L"RCon::setProgress(%i)", value);
		m_Progress.Progress = value;

		// Don't flash in Far while it is showing progress
		// That is useless because tab/caption/task-progress is changed during operation
		if (!tabs.bConsoleDataChanged && isFar())
		{
			mn_DeactivateTick = GetTickCount();
		}
	}
}

void CRealConsole::setLastShownProgress(short value)
{
	if (m_Progress.LastShownProgress != value)
	{
		logProgress(L"RCon::setLastShownProgress(%i)", value);
		m_Progress.LastShownProgress = value;
	}
}

void CRealConsole::setPreWarningProgress(short value)
{
	if (m_Progress.PreWarningProgress != value)
	{
		logProgress(L"RCon::setPreWarningProgress(%i)", value);
		m_Progress.PreWarningProgress = value;
	}
}

void CRealConsole::setConsoleProgress(short value)
{
	if (m_Progress.ConsoleProgress != value)
	{
		logProgress(L"RCon::setConsoleProgress(%i)", value);
		m_Progress.ConsoleProgress = value;
	}
}

void CRealConsole::setLastConsoleProgress(short value, bool UpdateTick)
{
	if (m_Progress.LastConsoleProgress != value)
	{
		logProgress(L"RCon::setLastConsoleProgress(%i,%u)", value, UpdateTick);
		m_Progress.LastConsoleProgress = value;
	}

	if (UpdateTick)
	{
		m_Progress.LastConProgrTick = (value >= 0) ? GetTickCount() : 0;
	}
}

void CRealConsole::setAppProgress(const AnsiProgressStatus AppProgressState, const short AppProgress)
{
	logProgress(L"RCon::setAppProgress(%i,%i)", int(AppProgressState), int(AppProgress));

	if (m_Progress.AppProgressState != AppProgressState)
		m_Progress.AppProgressState = AppProgressState;
	if (m_Progress.AppProgress != AppProgress)
		m_Progress.AppProgress = AppProgress;
}

short CRealConsole::GetProgress(AnsiProgressStatus* rpnState/*1-error,2-ind*/, bool* rpbNotFromTitle)
{
	AssertThisRet(-1);

	if (m_Progress.AppProgressState > AnsiProgressStatus::None)
	{
		if (rpnState)
		{
			*rpnState = m_Progress.AppProgressState;
		}
		if (rpbNotFromTitle)
		{
			//-- Если проценты пришли НЕ из заголовка, а из текста
			//-- консоли - то они "дописываются" в текущий заголовок, для таба
			////// Если пришло от установки через ESC-коды или GuiMacro
			//*rpbNotFromTitle = TRUE;
			*rpbNotFromTitle = FALSE;
		}
		return m_Progress.AppProgress;
	}

	if (rpbNotFromTitle)
	{
		//-- Если проценты пришли НЕ из заголовка, а из текста
		//-- консоли - то они "дописываются" в текущий заголовок, для таба
		//*rpbNotFromTitle = (mn_ConsoleProgress != -1);
		*rpbNotFromTitle = FALSE;
	}

	if (m_Progress.Progress >= 0)
	{
		if (rpnState)
			*rpnState = AnsiProgressStatus::Running;
		return m_Progress.Progress;
	}

	if (m_Progress.PreWarningProgress >= 0)
	{
		// mn_PreWarningProgress - это последнее значение прогресса (0..100)
		// по после завершения процесса - он может еще быть не сброшен
		if (rpnState)
		{
			*rpnState = ((mn_FarStatus & CES_OPER_ERROR) == CES_OPER_ERROR)
				? AnsiProgressStatus::Error : AnsiProgressStatus::Running;
		}

		//if (mn_LastProgressTick != 0 && rpbError) {
		//	DWORD nDelta = GetTickCount() - mn_LastProgressTick;
		//	if (nDelta >= 1000) {
		//		if (rpbError) *rpbError = TRUE;
		//	}
		//}
		return m_Progress.PreWarningProgress;
	}

	if (rpnState)
		*rpnState = AnsiProgressStatus::None;
	return -1;
}

bool CRealConsole::SetProgress(const AnsiProgressStatus state, const short value, LPCWSTR pszName /*= nullptr*/)
{
	AssertThisRet(false);

	bool lbOk = false;

	//SetProgress 0
	//  -- Remove progress
	//SetProgress 1 <Value>
	//  -- Set progress value to <Value> (0-100)
	//SetProgress 2 <Value>
	//  -- Set error state in progress
	//SetProgress 3
	//  -- Set indeterminate state in progress
	//SetProgress 4 <Value>
	//  -- Start progress for some long process
	//SetProgress 5 <Name>
	//  -- Stop progress started with "4"

	switch (state)
	{
	case AnsiProgressStatus::None:
		setAppProgress(state, 0);
		lbOk = true;
		break;
	case AnsiProgressStatus::Running:
		setAppProgress(state, std::min<short>(std::max<short>(value,0),100));
		lbOk = true;
		break;
	case AnsiProgressStatus::Error:
	case AnsiProgressStatus::Paused:
		setAppProgress(state, (value > 0) ? std::min<short>(std::max<short>(value,0),100) : m_Progress.AppProgress);
		lbOk = true;
		break;
	case AnsiProgressStatus::Indeterminate:
		setAppProgress(state, m_Progress.AppProgress);
		lbOk = true;
		break;
	default:
		_ASSERTE(FALSE && "TODO: Estimation of process duration");
		break;
	}

	if (lbOk)
	{
		// Показать прогресс в заголовке
		mb_ForceTitleChanged = TRUE;

		if (isActive(false))
			// Для активной консоли - обновляем заголовок. Прогресс обновится там же
			mp_ConEmu->UpdateTitle();
		else
			// Для НЕ активной консоли - уведомить главное окно, что у нас сменились проценты
			mp_ConEmu->UpdateProgress();
	}

	return lbOk;
}

void CRealConsole::UpdateGuiInfoMapping(const ConEmuGuiMapping* apGuiInfo)
{
	DWORD dwServerPID = GetServerPID(true);
	if (dwServerPID)
	{
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GUICHANGED, sizeof(CESERVER_REQ_HDR)+apGuiInfo->cbSize);
		if (pIn)
		{
			memmove(&(pIn->GuiInfo), apGuiInfo, apGuiInfo->cbSize);
			DWORD dwTickStart = timeGetTime();

			CESERVER_REQ *pOut = ExecuteSrvCmd(dwServerPID, pIn, ghWnd);

			CSetPgDebug::debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

			if (pOut)
				ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}
}

// Послать в активный фар CMD_FARSETCHANGED
// Обновляются настройки: gpSet->isFARuseASCIIsort, gpSet->isShellNoZoneCheck;
void CRealConsole::UpdateFarSettings(DWORD anFarPID /*= 0*/, FAR_REQ_FARSETCHANGED* rpSetEnvVar /*= nullptr*/)
{
	AssertThis();

	if (rpSetEnvVar)
		memset(rpSetEnvVar, 0, sizeof(*rpSetEnvVar));

	// [MAX_PATH*4+64]
	FAR_REQ_FARSETCHANGED *pSetEnvVar = rpSetEnvVar ? rpSetEnvVar : (FAR_REQ_FARSETCHANGED*)calloc(sizeof(FAR_REQ_FARSETCHANGED),1); //+2*(MAX_PATH*4+64),1);
	//wchar_t *szData = pSetEnvVar->szEnv;
	pSetEnvVar->bFARuseASCIIsort = gpSet->isFARuseASCIIsort;
	pSetEnvVar->bShellNoZoneCheck = gpSet->isShellNoZoneCheck;
	CSetPgDebug* pDbgPg = (CSetPgDebug*)gpSetCls->GetPageObj(thi_Debug);
	pSetEnvVar->bMonitorConsoleInput = (CSetPgDebug::GetActivityLoggingType() == glt_Input);
	pSetEnvVar->bLongConsoleOutput = gpSet->AutoBufferHeight;

	mp_ConEmu->GetComSpecCopy(pSetEnvVar->ComSpec);

	if (rpSetEnvVar == nullptr)
	{
		// В консоли может быть активна и другая программа
		DWORD dwFarPID = (mn_FarPID_PluginDetected == mn_FarPID) ? mn_FarPID : 0; // anFarPID ? anFarPID : GetFarPID();

		if (!dwFarPID)
		{
			SafeFree(pSetEnvVar);
			return;
		}

		// Выполнить в плагине
		CConEmuPipe pipe(dwFarPID, 300);
		int nSize = sizeof(FAR_REQ_FARSETCHANGED); //+2*(pszName - szData);

		if (pipe.Init(L"FarSetChange", TRUE))
			pipe.Execute(CMD_FARSETCHANGED, pSetEnvVar, nSize);

		//pipe.Execute(CMD_SETENVVAR, szData, 2*(pszName - szData));
		SafeFree(pSetEnvVar);
	}
}

void CRealConsole::UpdateTextColorSettings(bool ChangeTextAttr /*= TRUE*/, bool ChangePopupAttr /*= TRUE*/, const AppSettings* apDistinct /*= nullptr*/)
{
	AssertThis();

	// Обновлять, только если наши настройки сменились
	if (apDistinct)
	{
		const AppSettings* pApp = gpSet->GetAppSettings(GetActiveAppSettingsId());
		if (pApp != apDistinct)
		{
			return;
		}
	}

	PrepareDefaultColors();

	CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SETCONCOLORS, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETCONSOLORS));
	if (!pIn)
		return;

	pIn->SetConColor.ChangeTextAttr = ChangeTextAttr;
	pIn->SetConColor.NewTextAttributes = MAKECONCOLOR(GetDefaultTextColorIdx(),GetDefaultBackColorIdx());
	pIn->SetConColor.ChangePopupAttr = ChangePopupAttr;
	pIn->SetConColor.NewPopupAttributes = MAKECONCOLOR(mn_PopTextColorIdx, mn_PopBackColorIdx);
	pIn->SetConColor.ReFillConsole = !isFar();
	// #Refill Pass current DynamicHeight into

	if (isLogging())
	{
		wchar_t szLog[140];
		swprintf_c(szLog,
			L"Color Palette: CECMD_SETCONCOLORS: Text(%s) {%u|%u} Popup(%s) {%u|%u} Refill=%s",
			ChangeTextAttr ? L"change" : L"leave",
			CONFORECOLOR(pIn->SetConColor.NewTextAttributes), CONBACKCOLOR(pIn->SetConColor.NewTextAttributes),
			ChangePopupAttr ? L"change" : L"leave",
			CONFORECOLOR(pIn->SetConColor.NewPopupAttributes), CONBACKCOLOR(pIn->SetConColor.NewPopupAttributes),
			pIn->SetConColor.ReFillConsole ? L"yes" : L"no");
		LogString(szLog);
	}

	CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), pIn, ghWnd);

	if (isLogging()) LogString(L"Color Palette: CECMD_SETCONCOLORS finished");

	ExecuteFreeResult(pIn);
	ExecuteFreeResult(pOut);
}

HWND CRealConsole::FindPicViewFrom(HWND hFrom)
{
	// !!! PicView может быть несколько, по каждому на открытый ФАР
	HWND hPicView = nullptr;

	//hPictureView = FindWindowEx(ghWnd, nullptr, L"FarPictureViewControlClass", nullptr);
	// Класс может быть как "FarPictureViewControlClass", так и "FarMultiViewControlClass"
	// А вот заголовок у них пока один
	while ((hPicView = FindWindowEx(hFrom, hPicView, nullptr, L"PictureView")) != nullptr)
	{
		// Проверить на принадлежность фару
		DWORD dwPID, dwTID;
		dwTID = GetWindowThreadProcessId(hPicView, &dwPID);

		if (dwPID == mn_FarPID || dwPID == GetActivePID())
			break;

		UNREFERENCED_PARAMETER(dwTID);
	}

	return hPicView;
}

struct FindChildGuiWindowArg
{
	HWND  hDlg;
	DWORD nPID;
};

BOOL CRealConsole::FindChildGuiWindowProc(HWND hwnd, LPARAM lParam)
{
	struct FindChildGuiWindowArg *p = (struct FindChildGuiWindowArg*)lParam;
	DWORD nPID;
	if (IsWindowVisible(hwnd) && GetWindowThreadProcessId(hwnd, &nPID) && (nPID == p->nPID))
	{
		p->hDlg = hwnd;
		return FALSE; // fin
	}
	return TRUE;
}

// Заголовок окна для PictureView вообще может пользователем настраиваться, так что
// рассчитывать на него при определения "Просмотра" - нельзя
HWND CRealConsole::isPictureView(bool abIgnoreNonModal/*=FALSE*/)
{
	AssertThisRet(nullptr);

	if (m_ChildGui.Process.ProcessID && !m_ChildGui.hGuiWnd)
	{
		// Если GUI приложение запущено во вкладке, и аттач выполняется НЕ из ConEmuHk.dll
		HWND hBack = mp_VCon->GetBack();
		HWND hChild = nullptr;
		DEBUGTEST(DWORD nSelf = GetCurrentProcessId());
		#ifdef _DEBUG // due to unittests
		_ASSERTE(nSelf != m_ChildGui.Process.ProcessID);
		#endif

		while ((hChild = FindWindowEx(hBack, hChild, nullptr, nullptr)) != nullptr)
		{
			DWORD nChild, nStyle;

			nStyle = ::GetWindowLong(hChild, GWL_STYLE);
			if (!(nStyle & WS_VISIBLE))
				continue;

			if (GetWindowThreadProcessId(hChild, &nChild))
			{
				if (nChild == m_ChildGui.Process.ProcessID)
				{
					break;
				}
			}
		}

		if ((hChild == nullptr) && gpSet->isAlwaysOnTop)
		{
			// Если создается диалог (подключение PuTTY например)
			// то он тоже должен быть OnTop, иначе вообще виден не будет
			struct FindChildGuiWindowArg arg = {};
			arg.nPID = m_ChildGui.Process.ProcessID;
			EnumWindows(FindChildGuiWindowProc, (LPARAM)&arg);
			if (arg.hDlg)
			{
				DWORD nExStyle = GetWindowLong(arg.hDlg, GWL_EXSTYLE);
				if (!(nExStyle & WS_EX_TOPMOST))
				{
					//TODO: Через Сервер! А то прав может не хватить
					SetWindowPos(arg.hDlg, HWND_TOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
				}
			}
		}

		if (hChild != nullptr)
		{
			DWORD nStyle = ::GetWindowLong(hChild, GWL_STYLE), nStyleEx = ::GetWindowLong(hChild, GWL_EXSTYLE);
			RECT rcChild; GetWindowRect(hChild, &rcChild);
			SetGuiMode(m_ChildGui.nGuiAttachFlags|agaf_Self, hChild, nStyle, nStyleEx, m_ChildGui.Process.Name, m_ChildGui.Process.ProcessID, m_ChildGui.Process.Bits, rcChild);
		}
	}

	if (hPictureView && (!IsWindow(hPictureView) || !isFar()))
	{
		hPictureView = nullptr; mb_PicViewWasHidden = FALSE;
		mp_ConEmu->InvalidateAll();
	}

	// !!! PicView может быть несколько, по каждому на открытый ФАР
	if (!hPictureView)
	{
		// !! Дочерние окна должны быть только в окнах отрисовки. В главном - быть ничего не должно !!
		//hPictureView = FindWindowEx(ghWnd, nullptr, L"FarPictureViewControlClass", nullptr);
		//hPictureView = FindPicViewFrom(ghWnd);
		//if (!hPictureView)
		//hPictureView = FindWindowEx('ghWnd DC', nullptr, L"FarPictureViewControlClass", nullptr);
		hPictureView = FindPicViewFrom(mp_VCon->GetView());

		if (!hPictureView)    // FullScreen?
		{
			//hPictureView = FindWindowEx(nullptr, nullptr, L"FarPictureViewControlClass", nullptr);
			hPictureView = FindPicViewFrom(nullptr);
		}

		// Принадлежность процессу фара уже проверила сама FindPicViewFrom
		//if (hPictureView) {
		//    // Проверить на принадлежность фару
		//    DWORD dwPID, dwTID;
		//    dwTID = GetWindowThreadProcessId ( hPictureView, &dwPID );
		//    if (dwPID != mn_FarPID) {
		//        hPictureView = nullptr; mb_PicViewWasHidden = FALSE;
		//    }
		//}
	}

	if (hPictureView)
	{
		WARNING("PicView теперь дочернее в DC, но может быть FullScreen?")
		if (mb_PicViewWasHidden)
		{
			// Окошко было скрыто при переключении на другую консоль, но картинка еще активна
		}
		else
		if (!IsWindowVisible(hPictureView))
		{
			// Если вызывали Help (F1) - окошко PictureView прячется (самим плагином), считаем что картинки нет
			hPictureView = nullptr; mb_PicViewWasHidden = FALSE;
		}
	}

	if (mb_PicViewWasHidden && !hPictureView) mb_PicViewWasHidden = FALSE;

	if (hPictureView && abIgnoreNonModal)
	{
		wchar_t szClassName[128];

		if (GetClassName(hPictureView, szClassName, countof(szClassName)))
		{
			if (wcscmp(szClassName, L"FarMultiViewControlClass") == 0)
			{
				// Пока оно строго немодальное, но потом может быть по другому
				DWORD_PTR dwValue = GetWindowLongPtr(hPictureView, 0);

				if (dwValue != 0x200)
					return nullptr;
			}
		}
	}

	return hPictureView;
}

HWND CRealConsole::ConWnd()
{
	AssertThisRet(nullptr);

	return hConWnd;
}

HWND CRealConsole::GetView()
{
	AssertThisRet(nullptr);

	return mp_VCon->GetView();
}

// Если работаем в Gui-режиме (Notepad, Putty, ...)
HWND CRealConsole::GuiWnd()
{
	AssertThisRet(nullptr);
	if (!m_ChildGui.isGuiWnd())
		return nullptr;
	return m_ChildGui.hGuiWnd;
}
DWORD CRealConsole::GuiWndPID()
{
	AssertThisRet(0);
	if (!m_ChildGui.hGuiWnd)
		return 0;
	return m_ChildGui.Process.ProcessID;
}
bool CRealConsole::isGuiForceConView()
{
	// user asked to hide ChildGui and show our VirtualConsole (with current console contents)
	if (m_ChildGui.bGuiForceConView
		// gh#94: Putty/plink-proxy. hGuiWnd!=nullptr, but plink was "attached" into our console
		|| (m_ChildGui.bChildConAttached && !m_ChildGui.hGuiWnd))
		return true;
	return false;
}
bool CRealConsole::isGuiExternMode()
{
	return m_ChildGui.bGuiExternMode;
}
bool CRealConsole::isGuiEagerFocus()
{
	if (!GuiWnd())
		return false;
	if (isGuiForceConView() || isGuiExternMode())
		return false;
	return true;
}

// При движении окна ConEmu - нужно подергать дочерние окошки
// Иначе PuTTY глючит с обработкой мышки
void CRealConsole::GuiNotifyChildWindow()
{
	AssertThis();
	if (!m_ChildGui.hGuiWnd || m_ChildGui.bGuiExternMode)
		return;

	DEBUGTEST(BOOL bValid1 = IsWindow(m_ChildGui.hGuiWnd));

	if (!IsWindowVisible(m_ChildGui.hGuiWnd))
		return;

	RECT rcCur = {};
	if (!GetWindowRect(m_ChildGui.hGuiWnd, &rcCur))
		return;

	if (memcmp(&m_ChildGui.rcLastGuiWnd, &rcCur, sizeof(rcCur)) == 0)
		return; // ConEmu не двигалось

	StoreGuiChildRect(&rcCur); // Сразу запомнить

	HWND hBack = mp_VCon->GetBack();
	MapWindowPoints(nullptr, hBack, (LPPOINT)&rcCur, 2);

	DEBUGTEST(BOOL bValid2 = IsWindow(m_ChildGui.hGuiWnd));

	SetOtherWindowPos(m_ChildGui.hGuiWnd, nullptr, rcCur.left, rcCur.top, rcCur.right-rcCur.left+1, rcCur.bottom-rcCur.top+1, SWP_NOZORDER);
	SetOtherWindowPos(m_ChildGui.hGuiWnd, nullptr, rcCur.left, rcCur.top, rcCur.right-rcCur.left, rcCur.bottom-rcCur.top, SWP_NOZORDER);

	DEBUGTEST(BOOL bValid3 = IsWindow(m_ChildGui.hGuiWnd));

	//RECT rcChild = {};
	//GetWindowRect(hGuiWnd, &rcChild);
	////MapWindowPoints(nullptr, VCon->GetBack(), (LPPOINT)&rcChild, 2);

	//WPARAM wParam = 0;
	//LPARAM lParam = MAKELPARAM(rcChild.left, rcChild.top);
	////pRCon->PostConsoleMessage(hGuiWnd, WM_MOVE, wParam, lParam);

	//wParam = ::IsZoomed(hGuiWnd) ? SIZE_MAXIMIZED : SIZE_RESTORED;
	//lParam = MAKELPARAM(rcChild.right-rcChild.left, rcChild.bottom-rcChild.top);
	////pRCon->PostConsoleMessage(hGuiWnd, WM_SIZE, wParam, lParam);
}

void CRealConsole::GuiWndFocusStore()
{
	AssertThis();
	if (!m_ChildGui.hGuiWnd)
		return;

	mp_VCon->RestoreChildFocusPending(false);

	GUITHREADINFO gti = {sizeof(gti)};

	DWORD nPID = 0, nGetPID = 0, nErr = 0;
	BOOL bAttached = FALSE, bAttachCalled = FALSE;
	bool bHwndChanged = false;

	DWORD nTID = GetWindowThreadProcessId(m_ChildGui.hGuiWnd, &nPID);

	DEBUGTEST(BOOL bGuiInfo = )
	GetGUIThreadInfo(nTID, &gti);

	if (gti.hwndFocus && (m_ChildGui.hGuiWndFocusStore != gti.hwndFocus))
	{
		GetWindowThreadProcessId(gti.hwndFocus, &nGetPID);
		if (nGetPID != GetCurrentProcessId())
		{
			bHwndChanged = true;

			m_ChildGui.hGuiWndFocusStore = gti.hwndFocus;

			GuiWndFocusThread(gti.hwndFocus, bAttached, bAttachCalled, nErr);

			_ASSERTE(bAttached);
		}
	}

	if (gpSet->isLogging() && bHwndChanged)
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"GuiWndFocusStore for PID=%u, hWnd=x%08X", nPID, LODWORD(m_ChildGui.hGuiWndFocusStore));
		mp_ConEmu->LogString(szInfo);
	}
}

// Если (bForce == false) - то по "настройкам", юзер мог просить переключиться в ConEmu
void CRealConsole::GuiWndFocusRestore(bool bForce /*= false*/)
{
	AssertThis();
	if (!m_ChildGui.hGuiWnd)
		return;

	// Temp workaround for Issue 876: Ctrl+N and Win-Alt-Delete hotkey randomly break
	if (!gpSet->isFocusInChildWindows)
	{
		mp_ConEmu->setFocus();
		return;
	}

	bool bSkipInvisible = false;
	BOOL bAttached = FALSE;
	DWORD nErr = 0;

	HWND hSetFocus = m_ChildGui.hGuiWndFocusStore ? m_ChildGui.hGuiWndFocusStore : m_ChildGui.hGuiWnd;

	if (hSetFocus && IsWindow(hSetFocus))
	{
		_ASSERTE(isMainThread());

		BOOL bAttachCalled = FALSE;
		GuiWndFocusThread(hSetFocus, bAttached, bAttachCalled, nErr);

		if (IsWindowVisible(hSetFocus))
			SetFocus(hSetFocus);
		else
			bSkipInvisible = true;

		wchar_t sInfo[200];
		swprintf_c(sInfo, L"GuiWndFocusRestore to x%08X, hGuiWnd=x%08X, Attach=%s, Err=%u%s",
			LODWORD(hSetFocus), LODWORD(m_ChildGui.hGuiWnd),
			bAttachCalled ? (bAttached ? L"Called" : L"Failed") : L"Skipped", nErr,
			bSkipInvisible ? L", SkipInvisible" : L"");
		DEBUGSTRFOCUS(sInfo);
		mp_ConEmu->LogString(sInfo);
	}
	else
	{
		DEBUGSTRFOCUS(L"GuiWndFocusRestore skipped");
	}

	mp_VCon->RestoreChildFocusPending(bSkipInvisible);
}

void CRealConsole::GuiWndFocusThread(HWND hSetFocus, BOOL& bAttached, BOOL& bAttachCalled, DWORD& nErr)
{
	AssertThis();
	if (!m_ChildGui.hGuiWnd)
		return;

	bAttached = FALSE;
	nErr = 0;

	DWORD nMainTID = GetWindowThreadProcessId(ghWnd, nullptr);

	bAttachCalled = FALSE;
	DWORD nTID = GetWindowThreadProcessId(hSetFocus, nullptr);
	if (nTID != m_ChildGui.nGuiAttachInputTID)
	{
		// Надо, иначе не получится "передать" фокус в дочернее окно GUI приложения
		bAttached = AttachThreadInput(nTID, nMainTID, TRUE);

		if (!bAttached)
			nErr = GetLastError();
		else
			m_ChildGui.nGuiAttachInputTID = nTID;

		bAttachCalled = TRUE;
	}
	else
	{
		// Уже
		bAttached = TRUE;
	}
}

// Вызывается после завершения вставки дочернего GUI-окна в ConEmu
void CRealConsole::StoreGuiChildRect(LPRECT prcNewPos)
{
	AssertThis();
	if (!m_ChildGui.hGuiWnd)
	{
		_ASSERTE(m_ChildGui.hGuiWnd);
		return;
	}

	RECT rcChild = {};

	if (prcNewPos)
	{
		rcChild = *prcNewPos;
	}
	else
	{
		GetWindowRect(m_ChildGui.hGuiWnd, &rcChild);
	}

	#ifdef _DEBUG
	if (memcmp(&m_ChildGui.rcLastGuiWnd, &rcChild, sizeof(rcChild)) != 0)
	{
		DEBUGSTRGUICHILDPOS(L"CRealConsole::StoreGuiChildRect");
	}
	#endif

	m_ChildGui.rcLastGuiWnd = rcChild;
}

void CRealConsole::UpdateStartArgs(RConStartArgsEx::SplitType aSplitType, UINT aSplitValue, UINT aSplitPane, bool active)
{
	AssertThis();

	m_Args.eSplit = aSplitType;
	m_Args.nSplitValue = aSplitValue;
	m_Args.nSplitPane = aSplitPane;

	// Ensure non-active consoles does not have 'Foreground' flag
	// Ensure than active console does not have 'Background' flag
	if (active && m_Args.BackgroundTab)
		m_Args.BackgroundTab = crb_Undefined;
	if (!active && m_Args.ForegroundTab)
		m_Args.ForegroundTab = crb_Undefined;
}

void CRealConsole::SetGuiMode(DWORD anFlags, HWND ahGuiWnd, DWORD anStyle, DWORD anStyleEx, LPCWSTR asAppFileName, DWORD anAppPID, int anBits, RECT arcPrev)
{
	AssertThis();

	if ((m_ChildGui.hGuiWnd != nullptr) && !IsWindow(m_ChildGui.hGuiWnd))
	{
		setGuiWnd(nullptr); // окно закрылось, открылось другое
	}

	if (m_ChildGui.hGuiWnd != nullptr && m_ChildGui.hGuiWnd != ahGuiWnd)
	{
		Assert(m_ChildGui.hGuiWnd==nullptr);
		return;
	}

	CVConGuard guard(mp_VCon);


	if (gpSet->isLogging())
	{
		wchar_t szInfo[MAX_PATH*4];
		HWND hBack = guard->GetBack();
		swprintf_c(szInfo, L"SetGuiMode: BackWnd=x%08X, Flags=x%04X, PID=%u",
			(DWORD)(DWORD_PTR)hBack, anFlags, anAppPID);

		for (int s = 0; s <= 3; s++)
		{
			LPCWSTR pszLabel = nullptr;
			wchar_t szTemp[MAX_PATH-1] = {0};
			RECT rc;

			switch (s)
			{
			case 0:
				if (!ahGuiWnd) continue;
				GetWindowRect(hBack, &rc);
				swprintf_c(szTemp, L"hGuiWnd=x%08X, Style=x%08X, StyleEx=x%08X, Prev={%i,%i}-{%i,%i}, Back={%i,%i}-{%i,%i}",
					(DWORD)(DWORD_PTR)ahGuiWnd, (DWORD)(DWORD_PTR)hBack, anStyle, anStyleEx,
					LOGRECTCOORDS(arcPrev), LOGRECTCOORDS(rc));
				break;
			case 1:
				pszLabel = L"File";
				lstrcpyn(szTemp, asAppFileName, countof(szTemp));
				break;
			case 2:
				if (!ahGuiWnd) continue;
				pszLabel = L"Class";
				GetClassName(ahGuiWnd, szTemp, countof(szTemp)-1);
				break;
			case 3:
				if (!ahGuiWnd) continue;
				pszLabel = L"Title";
				GetWindowText(ahGuiWnd, szTemp, countof(szTemp)-1);
				break;
			}

			wcscat_c(szInfo, L", ");
			if (pszLabel)
			{
				wcscat_c(szInfo, pszLabel);
				wcscat_c(szInfo, L"=");
			}
			wcscat_c(szInfo, szTemp);
		}

		mp_ConEmu->LogString(szInfo);
	}


	AllowSetForegroundWindow(anAppPID);

	// Вызывается два раза. Первый (при запуске exe) ahGuiWnd==nullptr, второй - после фактического создания окна
	// А может и три, если при вызове ShowWindow оказалось что SetParent пролетел (XmlNotepad)
	if (m_ChildGui.hGuiWnd == nullptr)
	{
		m_ChildGui.rcPreGuiWndRect = arcPrev;
	}
	// ahGuiWnd может быть на первом этапе, когда ConEmuHk уведомляет - запустился GUI процесс
	_ASSERTE((m_ChildGui.hGuiWnd==nullptr && ahGuiWnd==nullptr) || (ahGuiWnd && IsWindow(ahGuiWnd))); // Проверить, чтобы мусор не пришел...
	m_ChildGui.nGuiAttachFlags = anFlags;
	setGuiWndPID(ahGuiWnd, anAppPID, anBits, PointToName(asAppFileName)); // устанавливает mn_GuiWndPID
	m_ChildGui.bCreateHidden = ((anStyle & WS_VISIBLE) == 0);
	m_ChildGui.nGuiWndStyle = anStyle; m_ChildGui.nGuiWndStylEx = anStyleEx;
	m_ChildGui.bGuiExternMode = false;
	if (ahGuiWnd)
		mp_VCon->TrackMouse();
	GuiWndFocusStore();
	ShowWindow(GetView(), SW_HIDE); // Остается видим только Back, в нем создается GuiClient

#ifdef _DEBUG
	mp_VCon->CreateDbgDlg();
#endif

	// Ставим после "m_ChildGui.hGuiWnd = ahGuiWnd", т.к. для гуй-приложений логика кнопки другая.
	if (isActive(false))
	{
		// Обновить на тулбаре статусы Scrolling(BufferHeight) & Alternative
		OnBufferHeight();
	}

	// Уведомить сервер (ConEmuC), что создано GUI подключение
	DWORD nSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ATTACHGUIAPP);
	CESERVER_REQ In;
	ExecutePrepareCmd(&In, CECMD_ATTACHGUIAPP, nSize);

	In.AttachGuiApp.nFlags = anFlags;
	In.AttachGuiApp.hConEmuWnd = ghWnd;
	In.AttachGuiApp.hConEmuDc = GetView();
	In.AttachGuiApp.hConEmuBack = mp_VCon->GetBack();
	In.AttachGuiApp.hAppWindow = ahGuiWnd;
	In.AttachGuiApp.Styles.nStyle = anStyle;
	In.AttachGuiApp.Styles.nStyleEx = anStyleEx;
	In.AttachGuiApp.Styles.Shifts = {};
	CorrectGuiChildRect(In.AttachGuiApp.Styles.nStyle, In.AttachGuiApp.Styles.nStyleEx, In.AttachGuiApp.Styles.Shifts, m_ChildGui.Process.Name);
	In.AttachGuiApp.nPID = anAppPID;
	if (asAppFileName)
		wcscpy_c(In.AttachGuiApp.sAppFilePathName, asAppFileName);

	DWORD dwTickStart = timeGetTime();

	CESERVER_REQ *pOut = ExecuteSrvCmd(GetServerPID(), &In, ghWnd);

	CSetPgDebug::debugLogCommand(&In, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

	if (pOut) ExecuteFreeResult(pOut);

	if (m_ChildGui.hGuiWnd)
	{
		m_ChildGui.bInGuiAttaching = true;
		HWND hDcWnd = mp_VCon->GetView();
		RECT rcDC; ::GetWindowRect(hDcWnd, &rcDC);
		MapWindowPoints(nullptr, hDcWnd, (LPPOINT)&rcDC, 2);
		// Чтобы артефакты не появлялись
		ValidateRect(hDcWnd, &rcDC);

		DWORD nPID = 0;
		DWORD nTID = GetWindowThreadProcessId(m_ChildGui.hGuiWnd, &nPID);
		_ASSERTE(nPID == anAppPID);
		AllowSetForegroundWindow(nPID);
		UNREFERENCED_PARAMETER(nTID);

		/*
		BOOL lbThreadAttach = AttachThreadInput(nTID, GetCurrentThreadId(), TRUE);
		DWORD nMainTID = GetWindowThreadProcessId(ghWnd, nullptr);
		BOOL lbThreadAttach2 = AttachThreadInput(nTID, nMainTID, TRUE);
		DWORD nErr = GetLastError();
		*/

		// Хм. Окошко еще не внедрено в ConEmu, смысл отдавать фокус в другое приложение?
		// SetFocus не сработает - другой процесс
		apiSetForegroundWindow(m_ChildGui.hGuiWnd);


		// Запомнить будущие координаты
		HWND hBack = mp_VCon->GetBack();
		RECT rcChild = {}; GetWindowRect(hBack, &rcChild);
		// AddMargin - не то
		rcChild.left += In.AttachGuiApp.Styles.Shifts.left;
		rcChild.top += In.AttachGuiApp.Styles.Shifts.top;
		rcChild.right += In.AttachGuiApp.Styles.Shifts.right;
		rcChild.bottom += In.AttachGuiApp.Styles.Shifts.bottom;
		StoreGuiChildRect(&rcChild);


		GetWindowText(m_ChildGui.hGuiWnd, Title, countof(Title)-2);
		wcscpy_c(TitleFull, Title);
		TitleAdmin[0] = 0;
		mb_ForceTitleChanged = FALSE;
		OnTitleChanged();

		mp_VCon->UpdateThumbnail(TRUE);
	}
}

bool CRealConsole::CanCutChildFrame(LPCWSTR pszExeName)
{
	if (!pszExeName || !*pszExeName)
		return true;
	if (lstrcmpi(pszExeName, L"chrome.exe") == 0
		|| lstrcmpi(pszExeName, L"firefox.exe") == 0)
	{
		return false;
	}
	return true;
}

void CRealConsole::CorrectGuiChildRect(DWORD anStyle, DWORD anStyleEx, RECT& rcGui, LPCWSTR pszExeName)
{
	if (!CanCutChildFrame(pszExeName))
	{
		return;
	}

	int nX = 0, nY = 0, nY0 = 0;
	// SM_CXSIZEFRAME & SM_CYSIZEFRAME fails in Win7/WDM (smaller than real values)
	int nTestSize = 100;
	RECT rcTest = MakeRect(nTestSize,nTestSize);
	if (AdjustWindowRectEx(&rcTest, anStyle, FALSE, anStyleEx))
	{
		nX = -rcTest.left;
		nY = rcTest.bottom - nTestSize; // only frame size, not caption+frame
	}
	else if (anStyle & WS_THICKFRAME)
	{
		nX = GetSystemMetrics(SM_CXSIZEFRAME);
		nY = GetSystemMetrics(SM_CYSIZEFRAME);
	}
	else if (anStyleEx & WS_EX_WINDOWEDGE)
	{
		nX = GetSystemMetrics(SM_CXFIXEDFRAME);
		nY = GetSystemMetrics(SM_CYFIXEDFRAME);
	}
	else if (anStyle & WS_DLGFRAME)
	{
		nX = GetSystemMetrics(SM_CXEDGE);
		nY = GetSystemMetrics(SM_CYEDGE);
	}
	else if (anStyle & WS_BORDER)
	{
		nX = GetSystemMetrics(SM_CXBORDER);
		nY = GetSystemMetrics(SM_CYBORDER);
	}
	else
	{
		nX = GetSystemMetrics(SM_CXFIXEDFRAME);
		nY = GetSystemMetrics(SM_CYFIXEDFRAME);
	}
	if ((anStyle & WS_CAPTION) && gpSet->isHideChildCaption)
	{
		if (anStyleEx & WS_EX_TOOLWINDOW)
			nY0 += GetSystemMetrics(SM_CYSMCAPTION);
		else
			nY0 += GetSystemMetrics(SM_CYCAPTION);
	}
	rcGui.left -= nX; rcGui.right += nX; rcGui.top -= nY+nY0; rcGui.bottom += nY;
}

int CRealConsole::GetStatusLineCount(int nLeftPanelEdge)
{
	AssertThisRet(0);
	if (!isFar())
		return 0;

	// Должен вызываться только при активном реальном буфере
	_ASSERTE(mp_RBuf==mp_ABuf);
	return mp_RBuf->GetStatusLineCount(nLeftPanelEdge);
}

// abIncludeEdges - включать
int CRealConsole::CoordInPanel(COORD cr, bool abIncludeEdges /*= FALSE*/)
{
	AssertThisRet(0);
	if (!mp_ABuf || (mp_ABuf != mp_RBuf))
		return 0;

#ifdef _DEBUG
	// Для отлова некорректных координат для "far /w"
	int nVisibleHeight = mp_ABuf->GetTextHeight();
	if (cr.Y > (nVisibleHeight+16))
	{
		_ASSERTE(cr.Y <= nVisibleHeight);
	}
#endif

	RECT rcPanel;

	if (GetPanelRect(FALSE, &rcPanel, false, abIncludeEdges) && CoordInRect(cr, rcPanel))
		return 1;

	if (mp_RBuf->GetPanelRect(TRUE, &rcPanel, false, abIncludeEdges) && CoordInRect(cr, rcPanel))
		return 2;

	return 0;
}

bool CRealConsole::GetPanelRect(bool abRight, RECT* prc, bool abFull /*= FALSE*/, bool abIncludeEdges /*= FALSE*/)
{
	if (prc)
		*prc = MakeRect(-1, -1);

	AssertThisRet(false);
	if (mp_ABuf != mp_RBuf)
	{
		if (prc)
			*prc = MakeRect(-1,-1);
		return false;
	}

	return mp_RBuf->GetPanelRect(abRight, prc, abFull, abIncludeEdges);
}

CEActiveAppFlags CRealConsole::GetActiveAppFlags()
{
	AssertThisRet(caf_Standard);
	if (!hConWnd || !m_AppMap.IsValid())
		return caf_Standard;

	#ifdef _DEBUG
	DWORD nPID = GetActivePID();
	#endif

	CEActiveAppFlags nActiveAppFlags = m_AppMap.Ptr()->nActiveAppFlags;
	return nActiveAppFlags;
}

LPCWSTR CRealConsole::GetMntPrefix()
{
	// Prefer RConStartArg parameter. Even if ‘empty’ prefix was specified!
	if (m_Args.pszMntRoot && (wcscmp(m_Args.pszMntRoot, L"-") != 0))
		return m_Args.pszMntRoot;

	// If prefix was acquired from connector
	if (ms_MountRoot)
		return ms_MountRoot;

	WORD conInMode = mp_RBuf ? mp_RBuf->GetConInMode() : 0;
	const CEActiveAppFlags activeAppFlags = GetActiveAppFlags();
	TermEmulationType termMode = GetTermType();

	if (conInMode & ENABLE_VIRTUAL_TERMINAL_INPUT/*0x200*/)
		return L"/mnt";
	if (activeAppFlags & caf_Cygwin1)
		return L"/cygdrive";
	// caf_Msys1|caf_Msys2 - no prefix?
	return nullptr;
}

bool CRealConsole::isCygwinMsys()
{
	CEActiveAppFlags nActiveAppFlags = GetActiveAppFlags();
	if ((nActiveAppFlags & (caf_Cygwin1|caf_Msys1|caf_Msys2)))
		return true;
	return false;
}

bool CRealConsole::isUnixFS()
{
	if (isCygwinMsys())
		return true;
	if (GetTermType() == te_xterm)
		return true;
	return false;
}

// If autoconversion of pasted Windows paths is allowed in console?
bool CRealConsole::isPosixConvertAllowed()
{
	if (!isUnixFS())
		return false;
	// We check here explicitly for m_Args.pszMntRoot!
	// User may disable conversion via "-new_console:p:-" for ssh tasks for example
	if (m_Args.pszMntRoot && (wcscmp(m_Args.pszMntRoot, L"-") == 0))
		return false;
	return true;
}

bool CRealConsole::isFar(bool abPluginRequired/*=false*/)
{
	AssertThisRet(false);

	return GetFarPID(abPluginRequired)!=0;
}

// Должна вернуть true, если из фара что-то было запущено
// Если активный процесс и есть far - то false
bool CRealConsole::isFarInStack()
{
	if (mn_FarPID || mn_FarPID_PluginDetected || (mn_ProgramStatus & CES_FARINSTACK))
	{
		if (m_ActiveProcess.ProcessID
			&& ((m_ActiveProcess.ProcessID == mn_FarPID) || (m_ActiveProcess.ProcessID == mn_FarPID_PluginDetected)))
		{
			return false;
		}

		return true;
	}

	return false;
}

// Проверить, включен ли в фаре режим "far /w".
// В этом случае, буфер есть, но его прокруткой должен заниматься сам фар.
// Комбинации типа CtrlUp, CtrlDown и мышку - тоже передавать в фар.
bool CRealConsole::isFarBufferSupported()
{
	AssertThisRet(false);

	return (m_FarInfo.cbSize && m_FarInfo.bBufferSupport && (m_FarInfo.nFarPID == GetFarPID()));
}

// Проверить, разрешен ли в консоли прием мышиных сообщений
// Программы могут отключать прием через SetConsoleMode
bool CRealConsole::isSendMouseAllowed()
{
	AssertThisRet(false);
	if (mp_ABuf->m_Type != rbt_Primary)
		return false;

	if (mp_ABuf->GetConInMode() & ENABLE_QUICK_EDIT_MODE)
		return false;

	return true;
}

bool CRealConsole::isInternalScroll()
{
	if (gpSet->isDisableMouse || !isSendMouseAllowed())
		return true;

	// Если прокрутки нет - крутить нечего?
	if (!mp_ABuf->isScroll())
		return false;

	if (isFar())
	{
		// Use live Far metadata, not frames detected in historical screen contents.
		const auto* info = GetFarInfo();
		return gpSet->isFarWheelScrollback && isFarBufferSupported() && info
			&& info->bFarPanelInfoFilled && info->nMacroArea == fma_Panels
			&& !info->FarLeftPanel.Visible && !info->FarRightPanel.Visible;
	}

	return true;
}

bool CRealConsole::isFarKeyBarShown()
{
	if (!isFar())
	{
		TODO("KeyBar в других приложениях? hiew?");
		return false;
	}

	bool bKeyBarShown = false;
	const CEFAR_INFO_MAPPING* pInfo = GetFarInfo();
	if (pInfo)
	{
		// Editor/Viewer
		if (isEditor() || isViewer())
		{
			WARNING("Эта настройка пока не отдается!");
			bKeyBarShown = true;
		}
		else
		{
			bKeyBarShown = (pInfo->FarInterfaceSettings.ShowKeyBar != 0);
		}
	}
	return bKeyBarShown;
}

bool CRealConsole::isSelectionAllowed()
{
	AssertThisRet(false);
	return mp_ABuf->isSelectionAllowed();
}

bool CRealConsole::isSelectionPresent()
{
	AssertThisRet(false);
	return mp_ABuf->isSelectionPresent();
}

bool CRealConsole::isMouseSelectionPresent()
{
	AssertThisRet(false);
	return mp_ABuf->isMouseSelectionPresent();
}

bool CRealConsole::GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel)
{
	if (!isSelectionPresent())
	{
		memset(sel, 0, sizeof(*sel));
		return false;
	}
	return mp_ABuf->GetConsoleSelectionInfo(sel);
}

// Unlike `Alternative buffer` this returns `Paused` state of RealConsole
// We can "pause" applications which are using simple WriteFile to output data
bool CRealConsole::isPaused()
{
	AssertThisRet(false);
	return mp_RBuf->isPaused();
}

// Changes `isPaused` state
CEPauseCmd CRealConsole::Pause(CEPauseCmd cmd)
{
	CEPauseCmd result = CEPause_Unknown;
	DWORD dwServerPID = GetServerPID();

	if (dwServerPID)
	{
		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_PAUSE, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD));
		if (pIn)
		{
			// Store before call if pausing
			CEPauseCmd newState = (cmd == CEPause_Switch) ? (isPaused() ? CEPause_Off : CEPause_On) : cmd;
			if (newState == CEPause_On)
				mp_RBuf->StorePausedState(CEPause_On);

			pIn->dwData[0] = cmd;
			CESERVER_REQ *pOut = ExecuteSrvCmd(dwServerPID, pIn, ghWnd);
			if (pOut && (pOut->DataSize() >= sizeof(DWORD)))
			{
				result = (CEPauseCmd)pOut->dwData[0];
				mp_RBuf->StorePausedState(result);
			}
			else
			{
				mp_RBuf->StorePausedState(CEPause_Unknown);
			}

			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}

	return result;
}

bool CRealConsole::QueryPromptStart(COORD *cr)
{
	AssertThisRet(false);
	if (!hConWnd || !m_AppMap.IsValid())
		return false;

	#ifdef _DEBUG
	DWORD nPID = GetActivePID();
	#endif

	// TODO: It would be nice to check m_AppMap.Ptr()->nPreReadRowID?

	CONSOLE_SCREEN_BUFFER_INFO csbiPreRead = m_AppMap.Ptr()->csbiPreRead;
	if (!csbiPreRead.dwCursorPosition.X && !csbiPreRead.dwCursorPosition.Y)
		return false;

	if (cr)
		*cr = csbiPreRead.dwCursorPosition;
	return true;
}

void CRealConsole::QueryTermModes(wchar_t* pszInfo, int cchMax, bool bFull)
{
	if (!pszInfo || cchMax <= 0 || !this || !mp_RBuf)
	{
		if (pszInfo) *pszInfo = 0;
		return;
	}

	TermEmulationType Term = m_Term.Term;;
	BOOL bBracketedPaste = m_Term.bBracketedPaste;
	CEActiveAppFlags appFlags = GetActiveAppFlags();
	BOOL bAppCursorKeys = mp_XTerm ? mp_XTerm->AppCursorKeys : FALSE;
	TermMouseMode MouseFlags = m_Term.nMouseMode;

	wchar_t szFlags[128] = L"";
	switch (Term)
	{
	case te_win32:
		wcscpy_c(szFlags, bFull ? L"win32" : L"W"); break;
	case te_xterm:
		wcscpy_c(szFlags, bFull ? L"xterm" : L"X"); break;
	default:
		msprintf(szFlags, countof(szFlags), bFull ? L"term=%u" : L"t=%u", Term);
	}
	if (bAppCursorKeys)
		wcscat_c(szFlags, bFull ? L"|AppKeys" : L"A");
	if (bBracketedPaste)
		wcscat_c(szFlags, bFull ? L"|BrPaste" : L"B");
	if (appFlags & caf_Cygwin1)
		wcscat_c(szFlags, bFull ? L"|cygwin" : L"C");
	if (appFlags & caf_Msys1)
		wcscat_c(szFlags, bFull ? L"|msys" : L"1");
	if (appFlags & caf_Msys2)
		wcscat_c(szFlags, bFull ? L"|msys2" : L"2");
	if (appFlags & caf_Clink)
		wcscat_c(szFlags, bFull ? L"|clink" : L"K");

	if (MouseFlags && !bFull)
		wcscat_c(szFlags, L"M");
	else if (MouseFlags)
	{
		wchar_t szHex[20];
		msprintf(szHex, countof(szHex), L"|mouse=x%02X", MouseFlags);
		wcscat_c(szFlags, szHex);
	}

	lstrcpyn(pszInfo, szFlags, cchMax);
}

void CRealConsole::QueryRConModes(wchar_t* pszInfo, int cchMax, bool bFull)
{
	if (!pszInfo || cchMax <= 0 || !this || !mp_RBuf)
	{
		if (pszInfo) *pszInfo = 0;
		return;
	}

	// "Console states (In=x98, Out=x03, win32)"
	wchar_t szInfo[80];
	swprintf_c(szInfo,
		bFull ? L"In=x%02X, Out=x%02X" : L"x%02X,x%02X", mp_RBuf->GetConInMode(), mp_RBuf->GetConOutMode());

	lstrcpyn(pszInfo, szInfo, cchMax);
}

void CRealConsole::QueryCellInfo(wchar_t* pszInfo, int cchMax)
{
	if (!pszInfo || cchMax <= 0 || !this || !mp_ABuf)
	{
		if (pszInfo) *pszInfo = 0;
		return;
	}

	mp_ABuf->QueryCellInfo(pszInfo, cchMax);
}

void CRealConsole::GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci, COORD *cr)
{
	AssertThis();

	if (ci)
		mp_ABuf->ConsoleCursorInfo(ci);

	if (cr)
		mp_ABuf->ConsoleCursorPos(cr);
}

void CRealConsole::GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi)
{
	AssertThis();
	mp_ABuf->ConsoleScreenBufferInfo(sbi);
}

// pszSrc содержит либо полный путь, либо часть его
// допускаются как прямые так и обратные слеши
// путь может быть указан в cygwin формате
// также, функция может выполнить автопоиск в 1-м уровне подпапок "текущей" директории
LPCWSTR CRealConsole::GetFileFromConsole(LPCWSTR asSrc, CEStr& szFull)
{
	szFull.Clear();

	AssertThisRet(nullptr);
	if (!asSrc || !*asSrc)
	{
		_ASSERTE(this && asSrc && *asSrc);
		return nullptr;
	}

	MSectionLockSimple CS; CS.Lock(mpcs_CurWorkDir);

	if (!mp_Files)
		mp_Files = new CRConFiles(this);

	// Caching
	return mp_Files->GetFileFromConsole(asSrc, szFull);
}

// Returns true on changes
bool CRealConsole::ReloadFarWorkDir()
{
	const DWORD nFarPID = GetFarPID(true);
	if (!nFarPID)
		return false;

	bool bChanged = false;
	MSectionLock CS; CS.Lock(&ms_FarInfoCS, TRUE);
	const CEFAR_INFO_MAPPING* pInfo = m__FarInfo.Ptr();
	if (pInfo)
	{
		if (pInfo->nPanelDirIdx != m_FarInfo.nPanelDirIdx)
		{
			wcscpy_c(m_FarInfo.sActiveDir, pInfo->sActiveDir);
			wcscpy_c(m_FarInfo.sPassiveDir, pInfo->sPassiveDir);
			m_FarInfo.nPanelDirIdx = pInfo->nPanelDirIdx;
			bChanged = true;
		}
	}

	return bChanged;
}

const SYSTEMTIME& CRealConsole::GetStartTime() const
{
	// TODO: use shared_ptr, drop this
	if (!this) {  // NOLINT(clang-diagnostic-undefined-bool-conversion)
		static const SYSTEMTIME null_time = {};
		return null_time;
	} else {
		return m_StartTime;
	}
}

LPCWSTR CRealConsole::GetConsoleStartDir(CEStr& szDir)
{
	AssertThisRet(nullptr);

	szDir.Set(ms_StartWorkDir);
	return szDir.IsEmpty() ? nullptr : szDir.c_str();
}

LPCWSTR CRealConsole::GetConsoleCurDir(CEStr& szDir, bool NeedRealPath)
{
	AssertThisRet(nullptr);

	// Пути берем из мэппинга текущего плагина
	const DWORD nFarPID = GetFarPID(true);
	if (nFarPID)
	{
		ReloadFarWorkDir();
		if (m_FarInfo.sActiveDir[0])
		{
			szDir.Set(m_FarInfo.sActiveDir);
			goto wrap;
		}
	}

	// If it is not a Far with plugin - try to take the ms_CurWorkDir
	{
		MSectionLockSimple CS; CS.Lock(mpcs_CurWorkDir);
		// Is path (received from console) valid?
		if (!ms_CurWorkDir.IsEmpty()
			&& (!NeedRealPath || (ms_CurWorkDir[0] != L'~')))
		{
			szDir.Set(ms_CurWorkDir);
			goto wrap;
		}
	}

	// Last chance - startup dir of the console
	szDir.Set(mp_ConEmu->WorkDir(m_Args.pszStartupDir));
wrap:
	return szDir.IsEmpty() ? nullptr : szDir.c_str();
}

void CRealConsole::GetPanelDirs(CEStr& szActiveDir, CEStr& szPassive)
{
	szActiveDir.Set(ms_CurWorkDir);
	szPassive.Set(isFar() ? ms_CurPassiveDir.c_str(L"") : L"");
}

void CRealConsole::StoreCurWorkDir(CESERVER_REQ_STORECURDIR* pNewCurDir)
{
	if (!pNewCurDir || ((pNewCurDir->iActiveCch <= 0) && (pNewCurDir->iPassiveCch <= 0)))
		return;
	if (IsBadStringPtr(pNewCurDir->szDir, MAX_PATH))
		return;

	MSectionLockSimple CS; CS.Lock(mpcs_CurWorkDir);

	LPCWSTR pszArg = pNewCurDir->szDir;
	for (int i = 0; i <= 1; i++)
	{
		const int iCch = i ? pNewCurDir->iPassiveCch : pNewCurDir->iActiveCch;

		CEStr szWinPath;
		if (iCch)
		{
			if (*pszArg)
				MakeWinPath(pszArg, GetMntPrefix(), szWinPath);
			pszArg += iCch;
		}

		if (!szWinPath.IsEmpty())
		{
			if (i)
				ms_CurPassiveDir.Set(szWinPath);
			else
				ms_CurWorkDir.Set(szWinPath);
		}
	}

	if (mp_Files)
		mp_Files->ResetCache();

	CS.Unlock();

	// Tab templates are case insensitive yet
	LPCWSTR pszTabTempl = isFar() ? gpSet->szTabPanels : gpSet->szTabConsole;
	if (wcsstr(pszTabTempl, L"%d") || wcsstr(pszTabTempl, L"%D")
		|| wcsstr(pszTabTempl, L"%f") || wcsstr(pszTabTempl, L"%F"))
	{
		mp_ConEmu->mp_TabBar->Update();
	}
}

// CECMD_STARTCONNECTOR
void CRealConsole::SetMountRoot(CESERVER_REQ* pConnectorInfo)
{
	LPCSTR pszMountRootA = pConnectorInfo->DataSize() ? (LPCSTR)pConnectorInfo->Data : nullptr;
	SafeFree(ms_MountRoot);
	if (pszMountRootA)
	{
		int nLen = lstrlenA(pszMountRootA);
		ms_MountRoot = (wchar_t*)calloc(nLen+1,sizeof(*ms_MountRoot));
		MultiByteToWideChar(CP_UTF8, 0, pszMountRootA, -1, ms_MountRoot, nLen+1);
	}
}


// В дальнейшем надо бы возвращать значение для активного приложения
// По крайней мене в фаре мы можем проверить токены.
// В свойствах приложения проводником может быть установлен флажок "Run as administrator"
// Может быть соответствующий манифест...
// Хотя скорее всего это невозможно. В одной консоли не могут крутиться программы
// под разными аккаунтами (точнее elevated/non elevated)
bool CRealConsole::isAdministrator()
{
	AssertThisRet(false);

	if (m_Args.RunAsAdministrator == crb_On)
		return true;

	if (mp_ConEmu->mb_IsUacAdmin && (m_Args.RunAsAdministrator != crb_Off) && (m_Args.RunAsRestricted != crb_On) && !m_Args.pszUserName)
		return true;

	return false;
}

bool CRealConsole::isMouseButtonDown()
{
	AssertThisRet(false);

	return m_Mouse.bMouseButtonDown;
}

// Аргумент - DWORD(!) а не DWORD_PTR. Это приходит из консоли.
void CRealConsole::OnConsoleKeyboardLayout(const DWORD dwNewLayout)
{
	_ASSERTE(dwNewLayout!=0 || gbIsWine);

	// LayoutName: "00000409", "00010409", ...
	// А HKL от него отличается, так что передаем DWORD
	// HKL в x64 выглядит как: "0x0000000000020409", "0xFFFFFFFFF0010409"

	// Информационно?
	mn_ActiveLayout = dwNewLayout;

	if (dwNewLayout)
	{
		mp_ConEmu->OnLangChangeConsole(mp_VCon, dwNewLayout);
	}
}

// Вызывается из CConEmuMain::OnLangChangeConsole в главной нити
void CRealConsole::OnConsoleLangChange(DWORD_PTR dwNewKeybLayout)
{
	if (mp_RBuf->GetKeybLayout() != dwNewKeybLayout)
	{
		if (gpSet->isLogging(2))
		{
			wchar_t szInfo[255];
			swprintf_c(szInfo, L"CRealConsole::OnConsoleLangChange, Old=0x%08X, New=0x%08X",
			          (DWORD)mp_RBuf->GetKeybLayout(), (DWORD)dwNewKeybLayout);
			LogString(szInfo);
		}

		mp_RBuf->SetKeybLayout(dwNewKeybLayout);
		mp_ConEmu->SwitchKeyboardLayout(dwNewKeybLayout);

		#ifdef _DEBUG
		WCHAR szMsg[255];
		HKL hkl = GetKeyboardLayout(0);
		swprintf_c(szMsg, L"ConEmu: GetKeyboardLayout(0) after SwitchKeyboardLayout = 0x%08I64X\n",
		          (unsigned __int64)(DWORD_PTR)hkl);
		DEBUGSTRLANG(szMsg);
		//Sleep(2000);
		#endif
	}
	else
	{
		if (gpSet->isLogging(2))
		{
			wchar_t szInfo[255];
			swprintf_c(szInfo, L"CRealConsole::OnConsoleLangChange skipped, mp_RBuf->GetKeybLayout() already is 0x%08X",
			          (DWORD)dwNewKeybLayout);
			LogString(szInfo);
		}
	}
}

void CRealConsole::CloseColorMapping()
{
	m_TrueColorerMap.CloseMap();
	//if (mp_ColorHdr) {
	//	UnmapViewOfFile(mp_ColorHdr);
	//mp_ColorHdr = nullptr;
	mp_TrueColorerData = nullptr;
	mn_TrueColorMaxCells = 0;
	//}
	//if (mh_ColorMapping) {
	//	CloseHandle(mh_ColorMapping);
	//	mh_ColorMapping = nullptr;
	//}
	mb_DataChanged = true;
	mn_LastColorFarID = 0;
}

//void CRealConsole::CheckColorMapping(DWORD dwPID)
//{
//	if (!dwPID)
//		dwPID = GetFarPID();
//	if ((!dwPID && m_TrueColorerMap.IsValid()) || (dwPID != mn_LastColorFarID)) {
//		//CloseColorMapping();
//		if (!dwPID)
//			return;
//	}
//
//	if (dwPID == mn_LastColorFarID)
//		return; // Для этого фара - наличие уже проверяли!
//
//	mn_LastColorFarID = dwPID; // сразу запомним
//
//}

void CRealConsole::CreateColorMapping()
{
	AssertThis();

	if (mp_TrueColorerData)
	{
		// Mapping was already created
		return;
	}

	BOOL lbResult = FALSE;
	wchar_t szErr[512]; szErr[0] = 0;
	//wchar_t szMapName[512]; szErr[0] = 0;
	AnnotationHeader *pHdr = nullptr;
	_ASSERTE(sizeof(AnnotationInfo) == 8*sizeof(int)/*sizeof(AnnotationInfo::raw)*/);
	_ASSERTE(mp_VCon->GetView()!=nullptr);
	// 111101 - было "hConWnd", но GetConsoleWindow теперь перехватывается.
	m_TrueColorerMap.InitName(AnnotationShareName, (DWORD)sizeof(AnnotationInfo), LODWORD(mp_VCon->GetView())); //-V205

	WARNING("Удалить и переделать!");
	COORD crMaxSize = mp_RBuf->GetMaxSize();
	int nMapCells = std::max<int>(crMaxSize.X,200) * std::max<int>(crMaxSize.Y,200) * 2;
	DWORD nMapSize = nMapCells * sizeof(AnnotationInfo) + sizeof(AnnotationHeader);

	pHdr = m_TrueColorerMap.Create(nMapSize);
	if (!pHdr)
	{
		lstrcpyn(szErr, m_TrueColorerMap.GetErrorText(), countof(szErr));
		goto wrap;
	}
	pHdr->struct_size = sizeof(AnnotationHeader);
	pHdr->bufferSize = nMapCells;
	pHdr->locked = 0;
	pHdr->flushCounter = 0;

	//_ASSERTE(mh_ColorMapping == nullptr);
	//swprintf_c(szMapName, AnnotationShareName, sizeof(AnnotationInfo), (DWORD)hConWnd);
	//mh_ColorMapping = OpenFileMapping(FILE_MAP_READ, FALSE, szMapName);
	//if (!mh_ColorMapping) {
	//	DWORD dwErr = GetLastError();
	//	swprintf_c (szErr, L"ConEmu: Can't open colorer file mapping. ErrCode=0x%08X. %s", dwErr, szMapName);
	//	goto wrap;
	//}
	//
	//mp_ColorHdr = (AnnotationHeader*)MapViewOfFile(mh_ColorMapping, FILE_MAP_READ,0,0,0);
	//if (!mp_ColorHdr) {
	//	DWORD dwErr = GetLastError();
	//	wchar_t szErr[512];
	//	swprintf_c (szErr, L"ConEmu: Can't map colorer info. ErrCode=0x%08X. %s", dwErr, szMapName);
	//	CloseHandle(mh_ColorMapping); mh_ColorMapping = nullptr;
	//	goto wrap;
	//}
	pHdr = m_TrueColorerMap.Ptr();
	mn_TrueColorMaxCells = nMapCells;
	mp_TrueColorerData = (const AnnotationInfo*)(((LPBYTE)pHdr)+pHdr->struct_size);
	lbResult = TRUE;
wrap:

	if (!lbResult && szErr[0])
	{
		mp_ConEmu->DebugStep(szErr);
#ifdef _DEBUG
		MBoxA(szErr);
#endif
	}

	//return lbResult;
}

bool CRealConsole::OpenFarMapData()
{
	bool lbResult = false;
	wchar_t szMapName[128], szErr[512]; szErr[0] = 0;
	DWORD dwErr = 0;
	DWORD nFarPID = GetFarPID(TRUE);

	// !!! обязательно
	MSectionLock CS; CS.Lock(&ms_FarInfoCS, TRUE);

	//CloseFarMapData();
	//_ASSERTE(m_FarInfo.IsValid() == FALSE);

	// Если сервер (консоль) закрывается - нет смысла переоткрывать FAR Mapping!
	if (m_ServerClosing.hServerProcess)
	{
		CloseFarMapData(&CS);
		goto wrap;
	}

	nFarPID = GetFarPID(TRUE);
	if (!nFarPID)
	{
		CloseFarMapData(&CS);
		goto wrap;
	}

	if (m_FarInfo.cbSize && m_FarInfo.nFarPID == nFarPID)
	{
		goto SkipReopen; // уже получен, видимо из другого потока
	}

	// Секция уже заблокирована, можно
	m__FarInfo.InitName(CEFARMAPNAME, nFarPID);
	if (!m__FarInfo.Open())
	{
		lstrcpynW(szErr, m__FarInfo.GetErrorText(), countof(szErr));
		goto wrap;
	}

	if (m__FarInfo.Ptr()->nFarPID != nFarPID)
	{
		_ASSERTE(m__FarInfo.Ptr()->nFarPID != nFarPID);
		CloseFarMapData(&CS);
		swprintf_c(szErr, L"ConEmu: Invalid FAR info format. %s", szMapName);
		goto wrap;
	}

SkipReopen:
	_ASSERTE(m__FarInfo.Ptr()->nProtocolVersion == CESERVER_REQ_VER);
	m__FarInfo.GetTo(&m_FarInfo, sizeof(m_FarInfo));

	m_FarAliveEvent.InitName(CEFARALIVEEVENT, nFarPID);
	if (!m_FarAliveEvent.Open())
	{
		dwErr = GetLastError();

		if (m__FarInfo.IsValid())
		{
			_ASSERTE(m_FarAliveEvent.GetHandle()!=nullptr);
		}
	}

	lbResult = TRUE;
wrap:

	if (!lbResult && szErr[0] && !isServerClosing(true))
	{
		Sleep(250);
		if (!isServerClosing(true))
		{
			mp_ConEmu->DebugStep(szErr);
			LogString(szErr);
		}
	}

	UNREFERENCED_PARAMETER(dwErr);

	return lbResult;
}

bool CRealConsole::OpenMapHeader(bool abFromAttach)
{
	bool lbResult = false;
	wchar_t szErr[512]; szErr[0] = 0;
	//int nConInfoSize = sizeof(CESERVER_CONSOLE_MAPPING_HDR);

	if (m_ConsoleMap.IsValid() && m_AppMap.IsValid())
	{
		if (hConWnd == (HWND)(m_ConsoleMap.Ptr()->hConWnd))
		{
			_ASSERTE(m_ConsoleMap.Ptr() == nullptr);
			return TRUE;
		}
	}

	m_ConsoleMap.InitName(CECONMAPNAME, LODWORD(hConWnd));
	if (!m_ConsoleMap.Open())
	{
		//TODO: Fails, if server was started under "System" account
		//TODO: ..\160110-Run-As-System\ConEmu-System.xml
		lstrcpyn(szErr, m_ConsoleMap.GetErrorText(), countof(szErr));
		goto wrap;
	}

	m_AppMap.InitName(CECONAPPMAPNAME, LODWORD(hConWnd));
	if (!m_AppMap.Open())
	{
		lstrcpyn(szErr, m_AppMap.GetErrorText(), countof(szErr));
		goto wrap;
	}


	if (!abFromAttach)
	{
		if (m_ConsoleMap.Ptr()->nGuiPID != GetCurrentProcessId())
		{
			_ASSERTE(m_ConsoleMap.Ptr()->nGuiPID == GetCurrentProcessId());
			WARNING("Наверное нужно будет передать в сервер код GUI процесса? В каком случае так может получиться?");
			// Передать через команду сервера новый GUI PID. Если пайп не готов сразу выйти
		}
	}

	if (m_ConsoleMap.Ptr()->hConWnd && m_ConsoleMap.Ptr()->bDataReady)
	{
		// Только если MonitorThread еще не был запущен
		if (mn_MonitorThreadID == 0)
		{
			_ASSERTE(mp_RBuf==mp_ABuf);
			mp_RBuf->ApplyConsoleInfo();
		}
	}

	lbResult = TRUE;
wrap:

	if (!lbResult && szErr[0])
	{
		mp_ConEmu->DebugStep(szErr);
		MBoxA(szErr);
	}

	return lbResult;
}

void CRealConsole::CloseFarMapData(MSectionLock* pCS)
{
	MSectionLock CS;
	(pCS ? pCS : &CS)->Lock(&ms_FarInfoCS, TRUE);

	m_FarInfo.cbSize = 0; // сброс
	m__FarInfo.CloseMap();

	m_FarAliveEvent.Close();
}

void CRealConsole::CloseMapHeader()
{
	CloseFarMapData();

	m_GetDataPipe.Close();

	m_ConsoleMap.CloseMap();
	m_AppMap.CloseMap();

	if (mp_RBuf) mp_RBuf->ResetBuffer();
	if (mp_EBuf) mp_EBuf->ResetBuffer();
	if (mp_SBuf) mp_SBuf->ResetBuffer();

	mb_DataChanged = true;
}

bool CRealConsole::isAlive()
{
	AssertThisRet(false);

	bool lbAlive = true;

	if (GetFarPID(TRUE) != 0 && mn_LastFarReadTick)
	{
		const DWORD nLastReadTick = mn_LastFarReadTick;
		const DWORD nCurTick = GetTickCount();
		const DWORD nDelta = nCurTick - nLastReadTick;
		lbAlive = nLastReadTick && (nDelta < gpSet->nFarHourglassDelay);
	}

	return lbAlive;
}

bool CRealConsole::CheckConsoleAppAlive(DWORD const nCurFarPID, bool const bLastAlive)
{
	bool bAlive = false;
	if (nCurFarPID && m_FarInfo.cbSize && m_FarAliveEvent.Open())
	{
		const DWORD aliveCurTick = GetTicks();
		const DWORD aliveReadDelta = aliveCurTick - mn_LastFarReadTick;
		// Don't call Wait too often
		if (mn_LastFarReadTick == 0 || aliveReadDelta >= (gpSet->nFarHourglassDelay / 4))
		{
			const bool aliveEventSet = (m_FarAliveEvent.Wait(0) == WAIT_OBJECT_0);

			bool waitingConIn = false;
			if (!aliveEventSet)
			{
				// Check if Far is inside WaitForMultipleObjects on ConIn
				MSectionLock CS; CS.Lock(&ms_FarInfoCS, TRUE);
				if (m__FarInfo.IsValid() && m__FarInfo.Ptr() && m__FarInfo.Ptr()->nProtocolVersion == CESERVER_REQ_VER)
				{
					const DWORD ttl = m__FarInfo.Ptr()->activeTtl;
					if (ttl)
					{
						const int delta = static_cast<int32_t>(ttl - GetTicks());
						if (delta >= 0)
						{
							waitingConIn = true;
						}
					}
				}
			}

			if (aliveEventSet || waitingConIn)
			{
				mn_LastFarAliveTick = mn_LastFarReadTick = aliveCurTick ? aliveCurTick : 1;
				bAlive = true; // alive
				if (!bLastAlive) { DEBUGSTRALIVE(aliveEventSet ? L"MonitorThread: Alive set because of event" : L"MonitorThread: Alive set because of ConInWait"); }
				_ASSERTE(IsDebuggerPresent() || isAlive() == bAlive);
			}
			else if (aliveReadDelta > gpSet->nFarHourglassDelay)
			{
				mn_LastFarReadTick = aliveCurTick - gpSet->nFarHourglassDelay - 1; // don't get too away
				bAlive = false; // Far is busy
				if (bLastAlive) { DEBUGSTRALIVE(L"MonitorThread: Alive reset because of timeout"); }
			}
		}
		else
		{
			bAlive = (aliveReadDelta < gpSet->nFarHourglassDelay); // Should be yet valid
			if (bAlive != bLastAlive) { DEBUGSTRALIVE(L"MonitorThread: Alive state was changed"); }
			_ASSERTE(IsDebuggerPresent() || isAlive() == bAlive);
		}
	}
	else
	{
		bAlive = true; // ConEmu Far plugin is not installed, or current app is not Far
		_ASSERTE(IsDebuggerPresent() || isAlive() == bAlive);
	}

	return bAlive;
}

LPCWSTR CRealConsole::GetConStatus()
{
	AssertThisRet(nullptr);

	if (m_ChildGui.hGuiWnd)
		return nullptr;
	return m_ConStatus.szText;
}

void CRealConsole::SetConStatus(LPCWSTR asStatus, DWORD/*enum ConStatusOption*/ Options /*= cso_Default*/)
{
	if (!asStatus)
		asStatus = L"";

	wchar_t szPrefix[128];
	swprintf_c(szPrefix, L"CRealConsole::SetConStatus, hView=x%08X: ", (DWORD)(DWORD_PTR)mp_VCon->GetView());
	CEStr pszInfo(szPrefix, *asStatus ? asStatus : L"<Empty>");
	DEBUGSTRSTATUS(pszInfo);

	#ifdef _DEBUG
	if ((m_ConStatus.Options == Options) && (lstrcmp(m_ConStatus.szText, asStatus) == 0))
	{
		int iDbg = 0; // Nothing was changed?
	}
	#endif

	LogString(pszInfo);

	size_t cchMax = countof(m_ConStatus.szText);
	if (asStatus[0])
		lstrcpyn(m_ConStatus.szText, asStatus, cchMax);
	else
		wmemset(m_ConStatus.szText, 0, cchMax);
	m_ConStatus.Options = Options;

	if (!gpSet->isStatusBarShow && !(Options & cso_Critical) && (asStatus && *asStatus))
	{
		// No need to force non-critical status messages to console (upper-left corner)
		return;
	}

	if (!(Options & cso_DontUpdate) && isActive((Options & cso_Critical)==cso_Critical))
	{
		// Обновить статусную строку, если она показана
		if (gpSet->isStatusBarShow && isActive(false))
		{
			// Перерисовать сразу
			mp_ConEmu->mp_Status->UpdateStatusBar(true, true);
		}
		else if (isGuiOverCon())
		{
			// No status bar, ChildGui is over our VCon window, nothing to update
			return;
		}
		else if (!(Options & cso_DontUpdate) && mp_VCon->GetView())
		{
			mp_VCon->Update(true);
		}

		if (mp_VCon->GetView())
		{
			mp_VCon->Invalidate();
		}
	}
}

void CRealConsole::SetCursorShape(TermCursorShapes xtermShape)
{
	AssertThis();
	if (!mp_VCon)
	{
		return; // Exceptional
	}

	m_TermCursor.CursorShape = (xtermShape > tcs_Default && xtermShape < tcs_Last) ? xtermShape : tcs_Default;

	// Trigger force redraw
	mp_VCon->Update(true);
}

TermCursorShapes CRealConsole::GetCursorShape()
{
	AssertThisRet(tcs_Default); // Exceptional

	return m_TermCursor.CursorShape;
}

void CRealConsole::UpdateCursorInfo()
{
	AssertThis();

	if (!isActive(false))
		return;

	ConsoleInfoArg arg = {};
	GetConsoleInfo(&arg);

	mp_ConEmu->UpdateCursorInfo(&arg);
}

void CRealConsole::GetConsoleInfo(ConsoleInfoArg* pInfo)
{
	mp_ABuf->ConsoleCursorInfo(&pInfo->cInfo);
	mp_ABuf->ConsoleScreenBufferInfo(&pInfo->sbi, &pInfo->srRealWindow, &pInfo->TopLeft);
	pInfo->crCursor = pInfo->sbi.dwCursorPosition;
}

bool CRealConsole::isNeedCursorDraw()
{
	AssertThisRet(false);

	if (GuiWnd())
	{
		// В GUI режиме VirtualConsole скрыта под GUI окном и видна только при "включении" BufferHeight
		if (!isBufferHeight())
			return false;
	}
	else
	{
		if (!hConWnd || !mb_RConStartedSuccess)
			return false;

		if (!mp_ABuf->isCursorVisible())
			return false;
	}

	return true;
}

// может отличаться от CVirtualConsole
bool CRealConsole::isCharBorderVertical(WCHAR inChar)
{
	if ((inChar != ucBoxDblHorz && inChar != ucBoxSinglHorz
	        && (inChar >= ucBoxSinglVert && inChar <= ucBoxDblVertHorz))
	        || (inChar >= ucBox25 && inChar <= ucBox75) || inChar == ucBox100
	        || inChar == ucUpScroll || inChar == ucDnScroll)
		return true;
	else
		return false;
}

bool CRealConsole::isCharBorderLeftVertical(WCHAR inChar)
{
	if (inChar < ucBoxSinglHorz || inChar > ucBoxDblVertHorz)
		return false; // чтобы лишних сравнений не делать

	if (inChar == ucBoxDblVert || inChar == ucBoxSinglVert
			|| inChar == ucBoxDblDownRight || inChar == ucBoxSinglDownRight
			|| inChar == ucBoxDblVertRight || inChar == ucBoxDblVertSinglRight
			|| inChar == ucBoxSinglVertRight
			|| (inChar >= ucBox25 && inChar <= ucBox75) || inChar == ucBox100
			|| inChar == ucUpScroll || inChar == ucDnScroll)
		return true;
	else
		return false;
}

// может отличаться от CVirtualConsole
bool CRealConsole::isCharBorderHorizontal(WCHAR inChar)
{
	if (inChar == ucBoxSinglDownDblHorz || inChar == ucBoxSinglUpDblHorz
			|| inChar == ucBoxDblDownDblHorz || inChar == ucBoxDblUpDblHorz
			|| inChar == ucBoxSinglDownHorz || inChar == ucBoxSinglUpHorz || inChar == ucBoxDblUpSinglHorz
			|| inChar == ucBoxDblHorz)
		return true;
	else
		return false;
}

bool CRealConsole::GetMaxConSize(COORD* pcrMaxConSize)
{
	bool bOk = false;

	if (m_ConsoleMap.IsValid())
	{
		if (pcrMaxConSize)
			*pcrMaxConSize = m_ConsoleMap.Ptr()->crMaxConSize;

		bOk = true;
	}

	return bOk;
}

int CRealConsole::GetDetectedDialogs(int anMaxCount, SMALL_RECT* rc, DWORD* rf)
{
	AssertThisRet(0);
	if (!mp_ABuf)
		return 0;

	return mp_ABuf->GetDetector()->GetDetectedDialogs(anMaxCount, rc, rf);
}

const CRgnDetect* CRealConsole::GetDetector()
{
	AssertThisRet(nullptr);
	return mp_ABuf->GetDetector();
}

// Преобразовать абсолютные координаты консоли в координаты нашего буфера
// (вычесть номер верхней видимой строки и скорректировать видимую высоту)
bool CRealConsole::ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr)
{
	AssertThisRet(false);
	return mp_ABuf->ConsoleRect2ScreenRect(rcCon, prcScr);
}

DWORD CRealConsole::PostMacroThread(LPVOID lpParameter)
{
	PostMacroAnyncArg* pArg = (PostMacroAnyncArg*)lpParameter;
	pArg->pRCon->LogString("PostMacroThread started");
	if (pArg->bPipeCommand)
	{
		CConEmuPipe pipe(pArg->pRCon->GetFarPID(TRUE), CONEMUREADYTIMEOUT);
		if (pipe.Init(_T("CRealConsole::PostMacroThread"), TRUE))
		{
			pArg->pRCon->mp_ConEmu->DebugStep(_T("PostMacroThread: Waiting for result (10 sec)"));
			pArg->pRCon->LogString("... executing PipeCommand (thread)");
			BOOL lbSent = pipe.Execute(pArg->nCmdID, pArg->Data, pArg->nCmdSize);
			pArg->pRCon->LogString(lbSent ? L"... PipeCommand was sent (thread)" : L"... PipeCommand sending was failed (thread)");
			pArg->pRCon->mp_ConEmu->DebugStep(nullptr);
		}
		else
		{
			pArg->pRCon->LogString("Far pipe was not opened, Macro was skipped (thread)");
		}
	}
	else
	{
		pArg->pRCon->LogString("... reentering PostMacro (thread)");
		pArg->pRCon->PostMacro(pArg->szMacro, FALSE/*теперь - точно Sync*/);
	}
	free(pArg);
	return 0;
}

void CRealConsole::PostCommand(DWORD anCmdID, DWORD anCmdSize, LPCVOID ptrData)
{
	AssertThis();
	if (mh_PostMacroThread != nullptr)
	{
		DWORD nWait = WaitForSingleObject(mh_PostMacroThread, 0);
		if (nWait == WAIT_OBJECT_0)
		{
			CloseHandle(mh_PostMacroThread);
			mh_PostMacroThread = nullptr;
		}
		else
		{
			// Должен быть nullptr, если нет - значит завис предыдущий макрос
			_ASSERTE(mh_PostMacroThread==nullptr && "Terminating mh_PostMacroThread");
			apiTerminateThread(mh_PostMacroThread, 100);
			CloseHandle(mh_PostMacroThread);
		}
	}

	size_t nArgSize = sizeof(PostMacroAnyncArg) + anCmdSize;
	PostMacroAnyncArg* pArg = (PostMacroAnyncArg*)calloc(1,nArgSize);
	pArg->pRCon = this;
	pArg->bPipeCommand = TRUE;
	pArg->nCmdID = anCmdID;
	pArg->nCmdSize = anCmdSize;
	if (ptrData && anCmdSize)
		memmove(pArg->Data, ptrData, anCmdSize);
	mh_PostMacroThread = apiCreateThread(PostMacroThread, pArg, &mn_PostMacroThreadID, "RCon::PostMacroThread#1 ID=%i", mp_VCon->ID());
	if (mh_PostMacroThread == nullptr)
	{
		// Если не удалось запустить нить
		MBoxAssert(mh_PostMacroThread!=nullptr);
		free(pArg);
	}
	return;
}

void CRealConsole::PostDragCopy(bool abMove)
{
	const CEFAR_INFO_MAPPING* pFarVer = GetFarInfo();

	wchar_t *mcr = (wchar_t*)calloc(128, sizeof(wchar_t));
	//2010-02-18 Не было префикса '@'
	//2010-03-26 префикс '@' ставить нельзя, ибо тогда процесса копирования видно не будет при отсутствии подтверждения

	if (pFarVer && ((pFarVer->FarVer.dwVerMajor > 3) || ((pFarVer->FarVer.dwVerMajor == 3) && (pFarVer->FarVer.dwBuild > 2850))))
	{
		// Если тянули ".." то перед копированием на другую панель сначала необходимо выйти на верхний уровень
		lstrcpyW(mcr, L"if APanel.SelCount==0 and APanel.Current==\"..\" then Keys('CtrlPgUp') end ");

		// Теперь собственно клавиша запуска
		// И если просили копировать сразу без подтверждения
		if (gpSet->isDropEnabled==2)
		{
			lstrcatW(mcr, abMove ? L"Keys('F6 Enter')" : L"Keys('F5 Enter')");
		}
		else
		{
			lstrcatW(mcr, abMove ? L"Keys('F6')" : L"Keys('F5')");
		}
	}
	else
	{
		// Если тянули ".." то перед копированием на другую панель сначала необходимо выйти на верхний уровень
		lstrcpyW(mcr, L"$If (APanel.SelCount==0 && APanel.Current==\"..\") CtrlPgUp $End ");
		// Теперь собственно клавиша запуска
		lstrcatW(mcr, abMove ? L"F6" : L"F5");

		// И если просили копировать сразу без подтверждения
		if (gpSet->isDropEnabled==2)
			lstrcatW(mcr, L" Enter "); //$MMode 1");
	}

	PostMacro(mcr, TRUE/*abAsync*/);
	SafeFree(mcr);
}

bool CRealConsole::GetFarVersion(FarVersion* pfv)
{
	AssertThisRet(false);

	const DWORD nPID = GetFarPID(TRUE/*abPluginRequired*/);

	if (!nPID)
		return false;

	const CEFAR_INFO_MAPPING* pInfo = GetFarInfo();
	if (!pInfo)
	{
		_ASSERTE(pInfo!=nullptr);
		return false;
	}

	if (pfv)
		*pfv = pInfo->FarVer;

	return (pInfo->FarVer.dwVerMajor >= 1);
}

bool CRealConsole::IsFarLua()
{
	FarVersion fv{};
	if (GetFarVersion(&fv))
		return fv.IsFarLua();
	return false;
}

void CRealConsole::PostMacro(LPCWSTR asMacro, bool abAsync /*= FALSE*/)
{
	AssertThis();
	if (!asMacro || !*asMacro)
	{
		mp_ConEmu->LogString(L"Null Far macro was skipped");
		return;
	}

	if (*asMacro == GUI_MACRO_PREFIX/*L'#'*/)
	{
		// This is GuiMacro, but not a Far Manager macros
		if (asMacro[1])
		{
			CEStr pszGui(asMacro + 1);
			CEStr szRc;
			if (isLogging())
			{
				const CEStr lsLog(L"CRealConsole::PostMacro: ", asMacro);
				LogString(lsLog);
			}
			szRc = ConEmuMacro::ExecuteMacro(std::move(pszGui), this);
			LogString(szRc.c_str(L"<nullptr>"));
			TODO("Show result in the status line?");
		}
		return;
	}

	const DWORD nPID = GetFarPID(TRUE/*abPluginRequired*/);

	if (!nPID)
	{
		_ASSERTE(FALSE && "Far with plugin was not found, Macro was skipped");
		LogString("Far with plugin was not found, Macro was skipped");
		return;
	}

	const CEFAR_INFO_MAPPING* pInfo = GetFarInfo();
	if (!pInfo)
	{
		_ASSERTE(pInfo!=nullptr);
		LogString("Far mapping info was not found, Macro was skipped");
		return;
	}

	if (pInfo->FarVer.IsFarLua())
	{
		if (lstrcmpi(asMacro, gpSet->RClickMacroDefault(fmv_Standard)) == 0)
			asMacro = gpSet->RClickMacroDefault(fmv_Lua);
		else if (lstrcmpi(asMacro, gpSet->SafeFarCloseMacroDefault(fmv_Standard)) == 0)
			asMacro = gpSet->SafeFarCloseMacroDefault(fmv_Lua);
		else if (lstrcmpi(asMacro, gpSet->TabCloseMacroDefault(fmv_Standard)) == 0)
			asMacro = gpSet->TabCloseMacroDefault(fmv_Lua);
		else if (lstrcmpi(asMacro, gpSet->SaveAllMacroDefault(fmv_Standard)) == 0)
			asMacro = gpSet->SaveAllMacroDefault(fmv_Lua);
		else
		{
			_ASSERTE(*asMacro && *asMacro != L'$' && "Macro must be adopted to Lua");
		}
	}

	if (isLogging())
	{
		wchar_t szPID[32]; swprintf_c(szPID, L"(FarPID=%u): ", nPID);
		CEStr lsLog(L"CRealConsole::PostMacro", szPID, asMacro);
		LogString(lsLog);
	}

	if (abAsync)
	{
		if (mb_InCloseConsole)
			ShutdownGuiStep(L"PostMacro, creating thread");

		if (mh_PostMacroThread != nullptr)
		{
			DWORD nWait = WaitForSingleObject(mh_PostMacroThread, 0);
			if (nWait == WAIT_OBJECT_0)
			{
				CloseHandle(mh_PostMacroThread);
				mh_PostMacroThread = nullptr;
			}
			else
			{
				// Должен быть nullptr, если нет - значит завис предыдущий макрос
				_ASSERTE(mh_PostMacroThread==nullptr && "Terminating mh_PostMacroThread");
				LogString(L"Terminating mh_PostMacroThread (hung)");
				apiTerminateThread(mh_PostMacroThread, 100);
				CloseHandle(mh_PostMacroThread);
			}
		}

		size_t nLen = _tcslen(asMacro);
		size_t nArgSize = sizeof(PostMacroAnyncArg) + nLen*sizeof(*asMacro);
		PostMacroAnyncArg* pArg = (PostMacroAnyncArg*)calloc(1,nArgSize);
		pArg->pRCon = this;
		pArg->bPipeCommand = FALSE;
		_wcscpy_c(pArg->szMacro, nLen+1, asMacro);
		LogString("... executing macro asynchronously");
		mh_PostMacroThread = apiCreateThread(PostMacroThread, pArg, &mn_PostMacroThreadID, "RCon::PostMacroThread#2 ID=%i", mp_VCon->ID());
		if (mh_PostMacroThread == nullptr)
		{
			// Если не удалось запустить нить
			MBoxAssert(mh_PostMacroThread!=nullptr);
			free(pArg);
		}
		return;
	}

	if (mb_InCloseConsole)
	{
		ShutdownGuiStep(L"PostMacro, calling pipe");
	}

#ifdef _DEBUG
	DEBUGSTRMACRO(asMacro); DEBUGSTRMACRO(L"\n");
#endif
	CConEmuPipe pipe(nPID, CONEMUREADYTIMEOUT);

	if (pipe.Init(_T("CRealConsole::PostMacro"), TRUE))
	{
		LogString("... executing Macro synchronously");
		mp_ConEmu->DebugStep(_T("Macro: Waiting for result (10 sec)"));
		BOOL lbSent = pipe.Execute(CMD_POSTMACRO, asMacro, (_tcslen(asMacro)+1)*2);
		LogString(lbSent ? L"... Macro was sent" : L"... Macro sending was failed");
		mp_ConEmu->DebugStep(nullptr);
	}
	else
	{
		LogString("Far pipe was not opened, Macro was skipped");
	}

	if (mb_InCloseConsole)
	{
		ShutdownGuiStep(L"PostMacro, done");
	}
}

CEStr CRealConsole::PostponeMacro(CEStr&& asMacro)
{
	AssertThisRet(lstrdup(L"InvalidPointer"));
	if (!asMacro)
	{
		_ASSERTE(asMacro);
		return lstrdup(L"InvalidPointer");
	}

	if (!mpsz_PostCreateMacro)
	{
		mpsz_PostCreateMacro = std::move(asMacro);
	}
	else
	{
		ConEmuMacro::ConcatMacro(mpsz_PostCreateMacro, asMacro);
		asMacro.Release();
	}

	return lstrdup(L"PostponedRCon");
}

void CRealConsole::ProcessPostponedMacro()
{
	AssertThis();

	CEStr pszMacro = nullptr;
	CEStr pszResult = nullptr;

	if (mpsz_PostCreateMacro)
	{
		pszMacro = std::move(mpsz_PostCreateMacro);
		mpsz_PostCreateMacro = nullptr;
	}

	if (pszMacro)
	{
		CVConGuard VCon(mp_VCon);
		pszResult = ConEmuMacro::ExecuteMacro(std::move(pszMacro), this);
	}
}

DWORD CRealConsole::InitiateDetach()
{
	DWORD nServerPID = GetServerPID(true);
	mb_InDetach = true;
	wchar_t szLog[80] = L"";
	if (mh_MonitorThread)
	{
		SetMonitorThreadEvent();
		if (GetCurrentThreadId() != mn_MonitorThreadID)
		{
			DWORD nWait = WaitForSingleObject(mh_MonitorThread, WAIT_THREAD_DETACH_TIMEOUT);
			msprintf(szLog, countof(szLog), L"InitiateDetach: WaitResult=%u", nWait);
			LogString(szLog);
		}
	}
	return nServerPID;
}

bool CRealConsole::DetachRCon(bool bPosted /*= false*/, bool bSendCloseConsole /*= false*/, bool bDontConfirm /*= false*/)
{
	AssertThisRet(false);

	bool bDetached = false;

	LogString(L"CRealConsole::Detach");

	SIZE cellSize = {};

	if (InRecreate())
	{
		LogString(L"CRealConsole::Detach - Restricted, InRecreate!");
		goto wrap;
	}

	cellSize = mp_VCon->GetCellSize();

	if (m_ChildGui.hGuiWnd)
	{
		if (!bPosted)
		{
			if (!bDontConfirm && gpSet->isMultiDetachConfirm
				&& MsgBox(CLngRc::getRsrc(lng_DetachGuiConfirm/*"Detach GUI application from ConEmu?"*/),
					MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2, GetTitle()) != IDYES)
			{
				return false;
			}

			RECT rcGui = {};
			GetWindowRect(m_ChildGui.hGuiWnd, &rcGui); // Логичнее все же оставить приложение в том же месте а не ставить в m_ChildGui.rcPreGuiWndRect
			m_ChildGui.rcPreGuiWndRect = rcGui;

			ShowGuiClientExt(1, TRUE);

			ShowOtherWindow(m_ChildGui.hGuiWnd, SW_HIDE, FALSE/*синхронно*/);
			SetOtherWindowParent(m_ChildGui.hGuiWnd, nullptr);
			SetOtherWindowPos(m_ChildGui.hGuiWnd, HWND_NOTOPMOST, rcGui.left, rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top, SWP_SHOWWINDOW);

			mp_VCon->PostDetach(bSendCloseConsole);
			return false; // Not yet
		}

		//#ifdef _DEBUG
		//WINDOWPLACEMENT wpl = {sizeof(wpl)};
		//GetWindowPlacement(m_ChildGui.hGuiWnd, &wpl); // дает клиентские координаты
		//#endif

		HWND lhGuiWnd = m_ChildGui.hGuiWnd;
		//RECT rcGui = m_ChildGui.rcPreGuiWndRect;
		//GetWindowRect(m_ChildGui.hGuiWnd, &rcGui); // Логичнее все же оставить приложение в том же месте

		//ShowGuiClientExt(1, TRUE);

		//ShowOtherWindow(lhGuiWnd, SW_HIDE, FALSE/*синхронно*/);
		//SetOtherWindowParent(lhGuiWnd, nullptr);
		//SetOtherWindowPos(lhGuiWnd, HWND_NOTOPMOST, rcGui.left, rcGui.top, rcGui.right-rcGui.left, rcGui.bottom-rcGui.top, SWP_SHOWWINDOW);

		// Сбросить переменные, чтобы гуй закрыть не пыталось
		setGuiWndPID(nullptr, 0, 0, nullptr);
		//mb_IsGuiApplication = FALSE;

		//// Закрыть консоль
		//CloseConsole(false, false);

		// Inform server about close
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_DETACHCON, sizeof(CESERVER_REQ_HDR)+4*sizeof(DWORD));
		pIn->dwData[0] = LODWORD(lhGuiWnd); // HWND handles can't be larger than DWORD to not harm 32bit apps
		pIn->dwData[1] = bSendCloseConsole;
		pIn->dwData[2] = cellSize.cy;
		pIn->dwData[3] = cellSize.cx;
		DWORD dwTickStart = timeGetTime();

		DWORD nServerPID = InitiateDetach();
		CESERVER_REQ *pOut = ExecuteSrvCmd(nServerPID, pIn, ghWnd);

		CSetPgDebug::debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

		if (pOut)
			bDetached = true;
		ExecuteFreeResult(pOut);
		ExecuteFreeResult(pIn);

		// Поднять отцепленное окно "наверх"
		apiSetForegroundWindow(lhGuiWnd);
	}
	else
	{
		if (!bPosted)
		{
			if (!bDontConfirm && gpSet->isMultiDetachConfirm
				&& (MsgBox(CLngRc::getRsrc(lng_DetachConConfirm/*"Detach console from ConEmu?"*/),
					MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2, GetTitle()) != IDYES))
			{
				return false;
			}

			mp_VCon->PostDetach(bSendCloseConsole);
			return false; // Not yet
		}

		//ShowConsole(1); -- уберем, чтобы не мигало
		isShowConsole = TRUE; // просто флажок взведем, чтобы не пытаться ее спрятать
		RECT rcScreen;
		if (GetWindowRect(mp_VCon->GetView(), &rcScreen) && hConWnd)
			SetOtherWindowPos(hConWnd, HWND_NOTOPMOST, rcScreen.left, rcScreen.top, 0,0, SWP_NOSIZE);

		// Уведомить сервер, что он больше не наш
		CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_DETACHCON, sizeof(CESERVER_REQ_HDR)+4*sizeof(DWORD));
		DWORD dwTickStart = timeGetTime();
		pIn->dwData[0] = 0;
		pIn->dwData[1] = bSendCloseConsole;
		pIn->dwData[2] = cellSize.cy;
		pIn->dwData[3] = cellSize.cx;

		DWORD nServerPID = InitiateDetach();
		CESERVER_REQ *pOut = ExecuteSrvCmd(nServerPID, pIn, ghWnd);

		CSetPgDebug::debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime()-dwTickStart, L"ExecuteSrvCmd", pOut);

		if (pOut)
		{
			bDetached = true;
			ExecuteFreeResult(pOut);
		}
		else
		{
			ShowConsole(1);
		}

		ExecuteFreeResult(pIn);

		//SetLastError(0);
		//BOOL bVisible = IsWindowVisible(hConWnd); -- проверка обламывается. Не успевает...
		//DWORD nErr = GetLastError();
		//if (hConWnd && !bVisible)
		//	ShowConsole(1);
	}

	// Чтобы случайно не закрыть RealConsole?
	m_Args.Detached = crb_On;

	CConEmuChild::ProcessVConClosed(mp_VCon);

wrap:
	return bDetached;
}

void CRealConsole::Unfasten()
{
	AssertThis();

	// Unfasten of ChildGui is not working yet
	if (m_ChildGui.hGuiWnd)
	{
		DisplayLastError(L"ChildGui unfastening is not working yet", -1);
		return;
	}

	DWORD nAttachPID = m_ChildGui.hGuiWnd ? m_ChildGui.Process.ProcessID : GetServerPID(true);
	if (!nAttachPID)
	{
		_ASSERTE(nAttachPID);
		return;
	}

	if (!DetachRCon(true, false))
	{
		return;
	}

	CEStr lsTempBuf;
	LPCWSTR pszConEmuStartArgs = gpConEmu->MakeConEmuStartArgs(lsTempBuf);
	wchar_t szMacro[64];
	swprintf_c(szMacro, L"-GuiMacro \"Attach %u\"", nAttachPID);
	CEStr lsRunArgs(L"\"", gpConEmu->ms_ConEmuExe, L"\" -Detached ",
		pszConEmuStartArgs, (pszConEmuStartArgs ? L" " : nullptr),
		szMacro);

	STARTUPINFO si = {};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};
	BOOL bStarted = CreateProcess(nullptr, lsRunArgs.ms_Val, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi);
	if (!bStarted)
	{
		DisplayLastError(lsRunArgs.ms_Val, GetLastError(), MB_ICONSTOP, L"Failed to start new ConEmu instance", ghWnd);
		return;
	}

	SafeCloseHandle(pi.hProcess);
	SafeCloseHandle(pi.hThread);
}

const CEFAR_INFO_MAPPING* CRealConsole::GetFarInfo()
{
	AssertThisRet(nullptr);

	//return m_FarInfo.Ptr(); -- нельзя, может быть закрыт в другом потоке!

	if (!m_FarInfo.cbSize)
		return nullptr;
	return &m_FarInfo;
}

bool CRealConsole::InCreateRoot()
{
	AssertThisRet(false);
	return (mb_InCreateRoot && !mn_MainSrv_PID);
}

bool CRealConsole::InRecreate()
{
	AssertThisRet(false);
	return (mb_ProcessRestarted);
}

DWORD CRealConsole::GetRunTime()
{
	AssertThisRet(0);
	if (!mn_StartTick)
		return 0;
	mn_RunTime = (GetTickCount() - mn_StartTick);
	return mn_RunTime;
}

// Можно ли к этой консоли прицепить GUI приложение
bool CRealConsole::GuiAppAttachAllowed(DWORD anServerPID, LPCWSTR asAppFileName, DWORD anAppPID)
{
	AssertThisRet(false);
	// Если даже сервер еще не запущен
	if (InCreateRoot())
		return false;

	if (anServerPID && mn_MainSrv_PID && (anServerPID != mn_MainSrv_PID))
		return false;

	// PortableApps launcher?
	if (m_ChildGui.paf.nPID == anAppPID)
		return true;

	// Интересуют только клиентские процессы!
	int nProcCount = GetProcesses(nullptr, true);
	DWORD nActivePID = GetActivePID();

	// Вызывается два раза. Первый (при запуске exe) ahGuiWnd==nullptr, второй - после фактического создания окна

	if (nProcCount > 0 && nActivePID == anAppPID)
		return true; // Уже подцепились именно к этому приложению, а сейчас было создано окно

	// Иначе - проверяем дальше
	if ((nProcCount > 0) || (nActivePID != 0))
		return false;

	// Проверить, подходит ли asAppFileName к ожидаемому
	LPCWSTR pszCmd = GetCmd();
	// Strip from startup command: set, chcp, title, etc.
	if (pszCmd)
		ProcessSetEnvCmd(pszCmd);
	// Now we can compare executable name
	TODO("PortableApps!!! Запускаться может, например, CommandPromptPortable.exe, а стартовать cmd.exe");
	if (pszCmd && *pszCmd && asAppFileName && *asAppFileName)
	{
		wchar_t szApp[MAX_PATH+1];
		CmdArg  szArg;
		LPCWSTR pszArg = nullptr, pszApp = nullptr, pszOnly = nullptr;

		while (pszCmd[0] == L'"' && pszCmd[1] == L'"')
			pszCmd++;

		pszOnly = PointToName(pszCmd);

		// Теперь то, что запущено (пришло из GUI приложения)
		pszApp = PointToName(asAppFileName);
		lstrcpyn(szApp, pszApp, countof(szApp));
		wchar_t* pszDot = wcsrchr(szApp, L'.'); // расширение?
		CharUpperBuff(szApp, lstrlen(szApp));

		if ((pszCmd = NextArg(pszCmd, szArg, &pszApp)))
		{
			// Что пытаемся запустить в консоли
			CharUpperBuff(szArg.ms_Val, lstrlen(szArg));
			pszArg = PointToName(szArg);
			if (lstrcmp(pszArg, szApp) == 0)
				return true;
			if (!wcschr(pszArg, L'.') && pszDot)
			{
				*pszDot = 0;
				if (lstrcmp(pszArg, szApp) == 0)
					return true;
				*pszDot = L'.';
			}
		}

		// Может там кавычек нет, а путь с пробелом
		szArg.Set(pszOnly);
		CharUpperBuff(szArg.ms_Val, lstrlen(szArg));
		if (lstrcmp(szArg, szApp) == 0)
			return true;
		if (pszArg && !wcschr(pszArg, L'.') && pszDot)
		{
			*pszDot = 0;
			if (lstrcmp(pszArg, szApp) == 0)
				return true;
			*pszDot = L'.';
		}

		return false;
	}

	_ASSERTE(pszCmd && *pszCmd && asAppFileName && *asAppFileName);
	return true;
}

void CRealConsole::ShowPropertiesDialog()
{
	AssertThis();

	// Если в RealConsole два раза подряд послать SC_PROPERTIES_SECRET,
	// то при закрытии одного из двух (!) открытых диалогов - ConHost падает!
	// Поэтому, сначала попытаемся найти диалог настроек, и только если нет - посылаем msg
	HWND hConProp = nullptr;
	wchar_t szTitle[255]; int nDefLen = _tcslen(CEC_INITTITLE);
	// К сожалению, так не получится найти окно свойств, если консоль была
	// создана НЕ через ConEmu (например, был запущен Far, а потом сделан Attach).
	while ((hConProp = FindWindowEx(nullptr, hConProp, (LPCWSTR)32770, nullptr)) != nullptr)
	{
		if (GetWindowText(hConProp, szTitle, countof(szTitle))
			&& szTitle[0] == L'"' && szTitle[nDefLen+1] == L'"'
			&& !wmemcmp(szTitle+1, CEC_INITTITLE, nDefLen))
		{
			apiSetForegroundWindow(hConProp);
			return; // нашли, показываем!
		}
	}

	POSTMESSAGE(hConWnd, WM_SYSCOMMAND, SC_PROPERTIES_SECRET/*65527*/, 0, TRUE);
}

DWORD CRealConsole::GetConsoleCP()
{
	return mp_RBuf->GetConsoleCP();
}

DWORD CRealConsole::GetConsoleOutputCP()
{
	return mp_RBuf->GetConsoleOutputCP();
}

void CRealConsole::GetConsoleModes(WORD& nConInMode, WORD& nConOutMode, TermEmulationType& Term, bool& bBracketedPaste)
{
	if (this && mp_RBuf)
	{
		nConInMode = mp_RBuf->GetConInMode();
		nConOutMode = mp_RBuf->GetConOutMode();
		Term = m_Term.Term;
		bBracketedPaste = m_Term.bBracketedPaste;
	}
	else
	{
		nConInMode = nConOutMode = 0;
		Term = te_win32;
		bBracketedPaste = FALSE;
	}
}

void CRealConsole::ResetHighlightHyperlinks()
{
	AssertThis();
	// Reset flags in buffers
	if (mp_RBuf)
		mp_RBuf->ResetHighlightHyperlinks();
	if (mp_EBuf)
		mp_EBuf->ResetHighlightHyperlinks();
	if (mp_SBuf)
		mp_SBuf->ResetHighlightHyperlinks();
	// Following is superfluous, JIC if we create new buffers in future
	if (mp_ABuf)
		mp_ABuf->ResetHighlightHyperlinks();
}

ExpandTextRangeType CRealConsole::GetLastTextRangeType()
{
	return mp_ABuf->GetLastTextRangeType();
}

void CRealConsole::setGuiWnd(HWND ahGuiWnd)
{
	#ifdef _DEBUG
	DWORD nPID = 0;
	_ASSERTE(ahGuiWnd != (HWND)-1);
	#endif

	if (ahGuiWnd)
	{
		m_ChildGui.hGuiWnd = ahGuiWnd;

		#ifdef _DEBUG
		GetWindowThreadProcessId(ahGuiWnd, &nPID);
		_ASSERTE(nPID == m_ChildGui.Process.ProcessID);
		#endif

		//Useless, because ahGuiWnd is invisible yet and scrolling will be revealed in TrackMouse
		//mp_VCon->HideScroll(TRUE);
	}
	else
	{
		m_ChildGui.hGuiWnd = m_ChildGui.hGuiWndFocusStore = nullptr;
	}
}

void CRealConsole::setGuiWndPID(HWND ahGuiWnd, DWORD anPID, int anBits, LPCWSTR asProcessName)
{
	m_ChildGui.Process.ProcessID = anPID;
	m_ChildGui.Process.Bits = anBits;

	if (asProcessName != m_ChildGui.Process.Name)
	{
		lstrcpyn(m_ChildGui.Process.Name, asProcessName ? asProcessName : L"", countof(m_ChildGui.Process.Name));
	}

	setGuiWnd(ahGuiWnd);

	if (gpSet->isLogging())
	{
		wchar_t szInfo[MAX_PATH+100];
		DWORD dwExStyle = ahGuiWnd ? GetWindowLong(ahGuiWnd, GWL_EXSTYLE) : 0;
		DWORD dwStyle = ahGuiWnd ? GetWindowLong(ahGuiWnd, GWL_STYLE) : 0;
		swprintf_c(szInfo, L"setGuiWndPID: PID=%u, HWND=x%08X, Style=x%08X, ExStyle=x%08X, '%s'", anPID, (DWORD)(DWORD_PTR)ahGuiWnd, dwStyle, dwExStyle, m_ChildGui.Process.Name);
		mp_ConEmu->LogString(szInfo);
	}
}

void CRealConsole::CtrlWinAltSpace()
{
	AssertThis();

	static DWORD dwLastSpaceTick = 0;

	if ((dwLastSpaceTick-GetTickCount())<1000)
	{
		//if (hWnd == ghWnd DC) MBoxA(_T("Space bounce received from DC")) else
		//if (hWnd == ghWnd) MBoxA(_T("Space bounce received from MainWindow")) else
		//if (hWnd == mp_ConEmu->m_Back->mh_WndBack) MBoxA(_T("Space bounce received from BackWindow")) else
		//if (hWnd == mp_ConEmu->m_Back->mh_WndScroll) MBoxA(_T("Space bounce received from ScrollBar")) else
		MBoxA(_T("Space bounce received from unknown window"));
		return;
	}

	dwLastSpaceTick = GetTickCount();
	//MBox(L"CtrlWinAltSpace: Toggle");
	ShowConsoleOrGuiClient(-1); // Toggle visibility
}

void CRealConsole::AutoCopyTimer()
{
	if (gpSet->isCTSAutoCopy && isSelectionPresent())
	{
		DEBUGSTRTEXTSEL(L"CRealConsole::AutoCopyTimer() -> DoSelectionFinalize");
		if (gpSet->isCTSResetOnRelease)
			mp_ABuf->DoSelectionFinalize(true);
		else
			DoSelectionCopy();
	}
	else
	{
		DEBUGSTRTEXTSEL(L"CRealConsole::AutoCopyTimer() -> skipped");
	}
}
