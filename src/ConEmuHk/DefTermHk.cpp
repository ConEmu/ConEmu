
/*
Copyright (c) 2012-present Maximus5
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

#include "../common/Common.h"
#include "../common/MFileLog.h"
#include "../common/MStrDup.h"
#include "../common/MToolHelp.h"
#include "../common/MFileMapping.h"
#include "hlpConsole.h"
#include "hlpProcess.h"
#include "DefTermHk.h"

#include "DllOptions.h"
#include "SetHook.h"
#include "../common/DefTermChildMap.h"
#include "../common/MWnd.h"
#include "../ConEmu/version.h"

#define FOREGROUND_CHECK_DELAY  1000
#define STOP_HOOKERS_TIMEOUT    30000
#define SERIALIZE_CHECK_TIMEOUT 1000

#ifdef _DEBUG
	// Do not call OutputDebugString more that required, even in _DEBUG
	#define DebugStr(s) //OutputDebugString(s)
#else
	#define DebugStr(s)
#endif

#ifdef _DEBUG
	#define DefTermMsg(s) //MessageBox(nullptr, s, L"ConEmuHk", MB_SYSTEMMODAL)
#else
	#define DefTermMsg(s) //MessageBox(nullptr, s, L"ConEmuHk", MB_SYSTEMMODAL)
#endif

// helper
bool CDefTermHk::IsDefTermEnabled()
{
	if (!gbPrepareDefaultTerminal || !gpDefTerm)
		return false;
	const bool bDontCheckName = (gbIsNetVsHost || gbIsVSDebugConsole || gbIsVSDebugger)
		// Especially for Code, which starts detached "cmd /c start /wait"
		|| ((ghConWnd == nullptr) && (gnImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI))
		;
	if (!gpDefTerm->isDefaultTerminalAllowed(bDontCheckName))
		return false;
	return true;
}

#if defined(__GNUC__)
extern "C"
#endif
void WINAPI RequestDefTermShutdown()
{
	if (!gbPrepareDefaultTerminal)
	{
		_ASSERTEX(FALSE && "Expected in DefTerm only!");
		return;
	}
	gnDllState |= ds_OnDefTermShutdown;
	DoDllStop(true, ds_OnDefTermShutdown);
}

bool CDefTermHk::InitDefTerm()
{
	bool lbRc = true;

	wchar_t szInfo[MAX_PATH*2];
	msprintf(szInfo, countof(szInfo), L"!!! TH32CS_SNAPMODULE, TID=%u, InitDefaultTerm\n", GetCurrentThreadId());
	DebugStr(szInfo); // Don't call DefTermLogString here - gpDefTerm was not initialized yet

	_ASSERTEX(gpDefTerm==nullptr);
	gpDefTerm = new CDefTermHk();
	if (!gpDefTerm)
	{
		_ASSERTEX(gpDefTerm!=nullptr);
		return false;
	}

	//_ASSERTE(FALSE && "InitDefaultTerm");

	// При обновлении ConEmu может обновиться и ConEmuHk.dll
	// Но в процессы с "DefTerm" грузится копия dll-ки, поэтому
	// после обновления в уже хукнутый процесс загружается
	// вторая "ConEmuHk.YYMMDD.dll", а старую при этом нужно
	// выгрузить. Этим и займемся.
	HMODULE hPrevHooks = nullptr;
	_ASSERTEX(gnSelfPID!=0 && ghOurModule!=nullptr);
	MToolHelpModule snapshot(gnSelfPID);
	if (snapshot.Open())
	{
		MODULEENTRY32 mi = {};
		//wchar_t szOurName[MAX_PATH] = L"";
		//GetModuleFileName(ghOurModule, szOurName, MAX_PATH);
		wchar_t szMinor[8] = L"";
		lstrcpyn(szMinor, WSTRING(MVV_4a), countof(szMinor));
		wchar_t szAddName[40];
		msprintf(szAddName, countof(szAddName),
			CEDEFTERMDLLFORMAT /*L"ConEmuHk%s.%02u%02u%02u%s.dll"*/,
			CEDEFTERMHK_SUFFIX, MVV_1, MVV_2, MVV_3, szMinor);
		//LPCWSTR pszOurName = PointToName(szOurName);
		wchar_t* pszDot = wcschr(szAddName, L'.');
		wchar_t szCheckName[MAX_PATH+1];

		if (pszDot)
		{
			pszDot[1] = 0; // Need to check only name, without version number
			const int nCurLen = lstrlen(szAddName);
			while (snapshot.Next(mi))
			{
				if (mi.hModule == ghOurModule)
					continue;
				lstrcpyn(szCheckName, PointToName(mi.szExePath), nCurLen+1);
				if (lstrcmpi(szCheckName, szAddName) == 0)
				{
					msprintf(szInfo, countof(szInfo),
						L"Previous ConEmuHk module found at address " WIN3264TEST(L"0x%08X",L"0x%X%08X") L": %s",
						WIN3264WSPRINT(mi.hModule), mi.szExePath);
					DefTermLogString(szInfo);

					hPrevHooks = mi.hModule;
					break; // Prev (old version) instance found!
				}
			}
		}
	}

	if (hPrevHooks)
	{
		DefTermLogString(L"Trying to unload previous ConEmuHk module");

		// To avoid possible problems with CloseHandle in "unhooking" state
		gpDefTerm->mh_InitDefTermContinueFrom = OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());

		// gh#322: We must not call MH_Uninitialize because it deinitializes HEAP
		// but when we are in DefTerm mode, this OPCODE addresses are used ATM...
		// ReSharper disable once CppLocalVariableMayBeConst
		HANDLE hThread = apiCreateThread(CDefTermHk::InitDefTermContinue, static_cast<LPVOID>(hPrevHooks),
			&gpDefTerm->mn_InitDefTermContinueTID, "InitDefTermContinue");
		if (hThread)
		{
			CloseHandle(hThread);
		}
		else
		{
			lbRc = false;
		}
	}
	else
	{
		CDefTermHk::InitDefTermContinue(nullptr);
	}

	return lbRc;
}

DWORD CDefTermHk::InitDefTermContinue(LPVOID ahPrevHooks)
{
	// ReSharper disable once CppLocalVariableMayBeConst
	HMODULE hPrevHooks = static_cast<HMODULE>(ahPrevHooks);
	DWORD nFromWait = 1;

	// Old library was found, unload it before continue
	if (hPrevHooks)
	{
		DefTermLogString(L"Trying to unload previous ConEmuHk module (thread)");

		// To avoid possible problems with CloseHandle in "unhooking" state
		nFromWait = WaitForSingleObject(gpDefTerm->mh_InitDefTermContinueFrom, 30000);
		if (!gpDefTerm)
		{
			DefTermLogString(L"Unloading failed, gpDefTerm destroyed");
			return 1;
		}
		_ASSERTEX(nFromWait == WAIT_OBJECT_0);
		// Wait a little more
		Sleep(100);
		SafeCloseHandle(gpDefTerm->mh_InitDefTermContinueFrom);

		typedef void (WINAPI* RequestDefTermShutdown_t)();
		const auto fnShutdown = reinterpret_cast<RequestDefTermShutdown_t>(GetProcAddress(hPrevHooks, "RequestDefTermShutdown"));
		if (fnShutdown)
		{
			fnShutdown();
		}
		else
		{
			_ASSERTE(FALSE && "RequestDefTermShutdown not found in ConEmuHk.dll");
		}

		SetLastError(0);
		const BOOL bFree = FreeLibrary(hPrevHooks);
		const DWORD nFreeRc = GetLastError();
		if (!bFree)
		{
			gpDefTerm->DisplayLastError(L"Unloading failed", nFreeRc);
			return 2;
		}
		DefTermLogString(L"Unloading succeeded");
	}

	// For Visual Studio check all spawned processes (children of gnSelfPID), find *.vshost.exe
	if (gbIsVStudio)
	{
		//_ASSERTEX(FALSE && "Continue to find existing *.vshost.exe");
		MToolHelpProcess snapshot;
		if (snapshot.Open())
		{
			PROCESSENTRY32 pe = {};
			while (snapshot.Next(pe))
			{
				if (pe.th32ParentProcessID == gnSelfPID)
				{
					if (IsVsNetHostExe(pe.szExeFile)) // *.vshost.exe
					{
						// Found! Hook it!
						DefTermLogString(L"Child VsNetHost found, hooking");
						gpDefTerm->StartDefTermHooker(pe.th32ProcessID);
						break;
					}
				}
			}
		}
	}
	else
	{
		MToolHelpProcess findForks;
		MArray<PROCESSENTRY32> forks = findForks.FindForks(GetCurrentProcessId());
		TODO("OPTIMIZE: Try to find and process children from all levels, BUT ONLY from ROOT process?");
		for (const auto& p : forks)
		{
			DefTermLogString(L"Forked process found, hooking");
			gpDefTerm->StartDefTermHooker(p.th32ProcessID);
		}
	}

	DefTermLogString(L"InitDefaultTerm finished, calling StartDefTerm");

	gpDefTerm->StartDefTerm();

	// Continue to hooks
	DllStart_Continue();

	return 0;
}

void CDefTermHk::DefTermLogString(LPCSTR asMessage, LPCWSTR asLabel /*= nullptr*/)
{
	if (!gpDefTerm || !asMessage || !*asMessage)
		return;
	const auto iLen = lstrlenA(asMessage);
	CEStr lsMsg;
	MultiByteToWideChar(CP_ACP, 0, asMessage, iLen, lsMsg.GetBuffer(iLen), iLen);
	DefTermLogString(lsMsg.ms_Val, asLabel);
}

void CDefTermHk::DefTermLogString(LPCWSTR asMessage, LPCWSTR asLabel /*= nullptr*/)
{
	if (!asMessage || !*asMessage)
	{
		return;
	}

	// To ensure that we may force debug output for troubleshooting
	// even from non-def-term-ed applications
	if (!gpDefTerm)
	{
		DebugStr(asMessage);
		if (asMessage[lstrlen(asMessage) - 1] != L'\n')
		{
			DebugStr(L"\n");
		}
		return;
	}

	
	CEStr lsBuf;
	if (asLabel && *asLabel)
	{
		lsBuf = CEStr(asLabel, asMessage);
	}
	const wchar_t* pszReady = lsBuf.IsEmpty() ? asMessage : lsBuf.c_str();

	gpDefTerm->LogHookingStatus(GetCurrentProcessId(), pszReady);
}

bool CDefTermHk::LoadDefTermSrvMapping(CESERVER_CONSOLE_MAPPING_HDR& srvMapping)
{
	srvMapping = CESERVER_CONSOLE_MAPPING_HDR{};

	if (!gbPrepareDefaultTerminal || !gpDefTerm)
		return false;

	const auto insideSettings = gpDefTerm->LoadInsideSettings();
	const auto& opt = gpDefTerm->m_Opt;

	srvMapping.cbSize = sizeof(srvMapping);

	srvMapping.nProtocolVersion = CESERVER_REQ_VER;
	srvMapping.nGuiPID = gpDefTerm->insideConEmuPid_.load();
	srvMapping.hConEmuRoot = gpDefTerm->insideConEmuWnd_.load();
	srvMapping.hConEmuWndDc = nullptr;
	srvMapping.hConEmuWndBack = nullptr;
	srvMapping.Flags = opt.nConsoleFlags;
	srvMapping.useInjects = opt.bNoInjects ? ConEmuUseInjects::DontUse : ConEmuUseInjects::Use;

	lstrcpy(srvMapping.sConEmuExe, opt.pszConEmuExe ? opt.pszConEmuExe : L"");
	lstrcpy(srvMapping.ComSpec.ConEmuBaseDir, opt.pszConEmuBaseDir ? opt.pszConEmuBaseDir : L"");
	lstrcpy(srvMapping.ComSpec.ConEmuExeDir, opt.pszConEmuExe ? opt.pszConEmuExe : L"");
	wchar_t* pszName = const_cast<wchar_t*>(PointToName(srvMapping.ComSpec.ConEmuExeDir));
	if (pszName && pszName > srvMapping.ComSpec.ConEmuExeDir)
		*(pszName - 1) = L'\0';

	if (srvMapping.ComSpec.ConEmuExeDir[0] == L'\0' || srvMapping.ComSpec.ConEmuBaseDir[0] == L'\0')
	{
		_ASSERTE(srvMapping.ComSpec.ConEmuExeDir[0] && srvMapping.ComSpec.ConEmuBaseDir[0]);
		return false;
	}

	return true;
}


/* ************ Globals for "Default terminal ************ */


CDefTermHk::CDefTermHk()
	: CDefTermBase(false)
{
	mh_StopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	wchar_t szSelfName[MAX_PATH+1] = L"";
	GetModuleFileName(nullptr, szSelfName, countof(szSelfName));
	lstrcpyn(ms_ExeName, PointToName(szSelfName), countof(ms_ExeName));

	mp_FileLog = nullptr;

	insideLock_ = new MSectionSimple(true);

	mn_LastCheck = 0;
	ReloadSettings();

	mn_InitDefTermContinueTID = 0;
	mh_InitDefTermContinueFrom = nullptr;
}

CDefTermHk::~CDefTermHk()
{
	SafeDelete(mp_FileLog);
	CDefTermHk::StopHookers();
	insideMapInfo_.reset();
}

void CDefTermHk::StartDefTerm()
{
	LoadInsideSettings();

	Initialize(false, false, false);
}

void CDefTermHk::StopHookers()
{
	if (!gbPrepareDefaultTerminal)
		return;
	gbPrepareDefaultTerminal = false;
	mb_Termination = true;

	SetEvent(mh_StopEvent);

	WaitForSingleObject(mh_PostThread, STOP_HOOKERS_TIMEOUT);

	SafeDelete(insideLock_);

	CDefTermBase::StopHookers();
}

bool CDefTermHk::isDefaultTerminalAllowed(bool bDontCheckName /*= false*/)
{
	_ASSERTEX((gbPrepareDefaultTerminal==false) || ((gpDefTerm!=nullptr) && (this==gpDefTerm)));

	if (!gbPrepareDefaultTerminal || !this)
	{
		_ASSERTEX(gbPrepareDefaultTerminal && this);
		return false;
	}

	const DWORD nDelta = (GetTickCount() - mn_LastCheck);
	if (nDelta > SERIALIZE_CHECK_TIMEOUT)
	{
		ReloadSettings();
	}

	// Is it even allowed?
	if (!m_Opt.bUseDefaultTerminal)
		return false;
	// All paths are known?
	if (!m_Opt.pszConEmuExe || !*m_Opt.pszConEmuExe || !m_Opt.pszConEmuBaseDir || !*m_Opt.pszConEmuBaseDir)
		return false;
	// Check current executable in m_Opt.pszzHookedApps
	if (!bDontCheckName && !IsAppNameMonitored(ms_ExeName))
		return false;
	// Okay
	return true;
}

// Start checking loop, required for Aggressive mode
void CDefTermHk::PostCreateThreadFinished()
{
	// ReSharper disable once CppEntityAssignedButNoRead
	DWORD dwWait = WAIT_TIMEOUT;
	DWORD nForePid = 0;
	HWND  hFore = nullptr;
	while ((dwWait = WaitForSingleObject(mh_StopEvent, FOREGROUND_CHECK_DELAY)) == WAIT_TIMEOUT)
	{
		// If non-aggressive - don't do anything here...
		if (!m_Opt.bAggressive || !isDefaultTerminalAllowed(true))
			continue;

		// Aggressive mode
		hFore = GetForegroundWindow();
		if (!hFore)
			continue;
		if (!GetWindowThreadProcessId(hFore, &nForePid))
			continue;
		CheckForeground(hFore, nForePid, false);
	}
}

void CDefTermHk::ReloadSettings()
{
	MSectionLockSimple cs;
	cs.Lock(mcs);

	if (!LoadInsideSettings())
		m_Opt.Serialize();
	mn_LastCheck = GetTickCount();

	if (m_Opt.bDebugLog)
	{
		LogInit();
	}
	else
	{
		SafeDelete(mp_FileLog);
	}
}

DWORD CDefTermHk::GetConEmuInsidePid()
{
	if (!gpDefTerm)
		return 0;
	const auto insideOpt = gpDefTerm->LoadInsideSettings();
	if (!insideOpt)
		return 0;
	if (!insideOpt->bUseDefaultTerminal)
		return 0;
	if (!insideOpt->hConEmuRoot || !IsWindow(insideOpt->hConEmuRoot))
		return 0;
	// ConEmu inside is ready for DefTerm
	return insideOpt->nGuiPID;
}

void CDefTermHk::CreateChildMapping(const DWORD childPid, const MHandle& childHandle, const DWORD conemuInsidePid)
{
	if (!gpDefTerm)
		return;

	MSectionLockSimple cs; cs.Lock(&gpDefTerm->childDataLock_);

	if (!gpDefTerm->childData_)
		gpDefTerm->childData_ = std::make_shared<CDefTermChildMap>();

	gpDefTerm->childData_->CreateChildMapping(childPid, childHandle, conemuInsidePid);
}

/// @brief Try to find appropriate mapping with DefTerm options (Inside mode)
/// @return returns smart pointer to loaded DefTerm settings from ConEmu's mappings (even if DefTerm is disabled there)
std::shared_ptr<CONEMU_INSIDE_DEFTERM_MAPPING> CDefTermHk::LoadInsideSettings()
{
	MSectionLockSimple cs;
	cs.Lock(insideLock_);

	std::shared_ptr<CONEMU_INSIDE_DEFTERM_MAPPING> insideMapInfo = insideMapInfo_;

	const auto now = std::chrono::steady_clock::now();
	const auto delay = now - insideMapLastCheck_;
	if (delay < insideMapCheckDelay_)
	{
		return insideMapInfo;
	}
	insideMapLastCheck_ = now;

	DWORD conemuGuiPid = FindInsideParentConEmuPid();
	if (conemuGuiPid == 0)
	{
		conemuGuiPid = LoadInsideConEmuPid(CEINSIDEMAPNAMEP, GetCurrentProcessId());
	}

	if (conemuGuiPid == 0)
	{
		if (insideMapInfo_)
			m_Opt.bUseDefaultTerminal = false;
		insideMapInfo_.reset();
		return {};
	}

	MFileMapping<CONEMU_INSIDE_DEFTERM_MAPPING> mapping;
	mapping.InitName(CEDEFTERMMAPNAME, conemuGuiPid);
	if (mapping.Open())
	{
		if (!insideMapInfo)
		{
			insideMapInfo = std::shared_ptr<CONEMU_INSIDE_DEFTERM_MAPPING>(
				static_cast<CONEMU_INSIDE_DEFTERM_MAPPING*>(calloc(1, sizeof(CONEMU_INSIDE_DEFTERM_MAPPING))),
				StructDeleter<CONEMU_INSIDE_DEFTERM_MAPPING>{});
		}
		if (insideMapInfo)
		{
			if (!mapping.GetTo(insideMapInfo.get()))
				insideMapInfo.reset();
		}
	}

	if (!insideMapInfo)
	{
		if (insideMapInfo_)
			m_Opt.bUseDefaultTerminal = false;
		insideMapInfo_.reset();
		insideConEmuPid_.store(0);
		insideConEmuWnd_.store(nullptr);
		return {};
	}

	auto setString = [this](wchar_t*& var, wchar_t* str)
	{
		if (var && var != str && !m_Opt.bExternalPointers)
			SafeFree(var);
		var = str;
	};

	auto setMszString = [this](wchar_t*& var, wchar_t* str, const size_t cchMax)
	{
		if (var && var != str && !m_Opt.bExternalPointers)
			SafeFree(var);
		const size_t cchLen = std::min(wcslen(str), cchMax - 2);
		str[cchLen + 1] = L'\0';
		wchar_t* bar = wcschr(str, L'|');
		while (bar)
		{
			*bar = L'\0';
			bar = wcschr(bar + 1, L'|');
		}
		var = str;
	};

	static wchar_t emptyStr[2] = L"";

	auto& info = *insideMapInfo;
	m_Opt.bUseDefaultTerminal = info.bUseDefaultTerminal;
	m_Opt.bAggressive = false;
	m_Opt.bNoInjects = info.isDefaultTerminalNoInjects;
	m_Opt.bNewWindow = false;
	m_Opt.bDebugLog = info.isDefaultTerminalDebugLog;
	m_Opt.nDefaultTerminalConfirmClose = info.nDefaultTerminalConfirmClose;
	m_Opt.nConsoleFlags = info.flags;
	setString(m_Opt.pszConEmuExe, info.sConEmuExe);
	setString(m_Opt.pszConEmuBaseDir, info.sConEmuBaseDir);
	setString(m_Opt.pszCfgFile, emptyStr);
	setString(m_Opt.pszConfigName, emptyStr);
	setMszString(m_Opt.pszzHookedApps, info.defaultTerminalApps, countof(info.defaultTerminalApps));
	m_Opt.bExternalPointers = true;

	insideConEmuPid_.store(info.nGuiPID);
	insideConEmuWnd_.store(info.hConEmuRoot);

	insideMapInfo_ = insideMapInfo;
	return insideMapInfo;
}

/// @brief Try to detect root window, where ConEmu could be integrated into (Inside mode)
/// @return PID of the ConEmu instance, integrated into our window (Inside mode)
DWORD CDefTermHk::FindInsideParentConEmuPid()
{
	const DWORD curPid = GetCurrentProcessId();

	const MWnd hFore = GetForegroundWindow();

	if (hFore)
	{
		DWORD pid = 0;
		if (GetWindowThreadProcessId(hFore, &pid) && pid == curPid)
		{
			const DWORD conemuGuiPid = LoadInsideConEmuPid(CEINSIDEMAPNAMEW, hFore.GetPortableHandle());
			if (conemuGuiPid)
				return conemuGuiPid;
		}
	}

	MWnd hTop = FindWindowExW(nullptr, nullptr, nullptr, nullptr);
	while (hTop)
	{
		if (hTop != hFore && IsWindowVisible(hTop))
		{
			DWORD pid = 0;
			if (GetWindowThreadProcessId(hTop, &pid) && pid == curPid)
			{
				const DWORD conemuGuiPid = LoadInsideConEmuPid(CEINSIDEMAPNAMEW, hTop.GetPortableHandle());
				if (conemuGuiPid)
					return conemuGuiPid;
			}
		}
		hTop = FindWindowExW(nullptr, hTop, nullptr, nullptr);
	}

	return 0;
}

DWORD CDefTermHk::LoadInsideConEmuPid(const wchar_t* mapNameFormat, DWORD param)
{
	MFileMapping<CONEMU_INSIDE_MAPPING> mapping;
	mapping.InitName(mapNameFormat, param);
	if (!mapping.Open())
		return 0;
	CONEMU_INSIDE_MAPPING info{};
	if (!mapping.GetTo(&info))
		return 0;
	return info.nGuiPID;
}

void CDefTermHk::LogInit()
{
	if (mp_FileLog && mp_FileLog->IsLogOpened())
		return;

	if (!mp_FileLog)
	{
		mp_FileLog = new MFileLog(ms_ExeName);
		if (!mp_FileLog)
			return;
	}

	const HRESULT hr = mp_FileLog->CreateLogFile();
	if (hr != 0)
	{
		SafeDelete(mp_FileLog);
		return;
	}
	mp_FileLog->LogString(GetCommandLineW());
}

void CDefTermHk::LogHookingStatus(DWORD nForePID, LPCWSTR sMessage)
{
	if (!mp_FileLog || !mp_FileLog->IsLogOpened())
	{
		if (sMessage && *sMessage)
		{
			DebugStr(sMessage);
			if (sMessage[lstrlen(sMessage)-1] != L'\n')
			{
				DebugStr(L"\n");
			}
		}
		return;
	}
	wchar_t szPID[16];
	const CEStr lsLog(L"DefTerm[", ultow_s(nForePID, szPID, 10), L"]: ", sMessage);
	mp_FileLog->LogString(lsLog);
}

CDefTermBase* CDefTermHk::GetInterface()
{
	return this;
}

int CDefTermHk::DisplayLastError(LPCWSTR asLabel, DWORD dwError/*=0*/, DWORD dwMsgFlags/*=0*/, LPCWSTR asTitle/*=nullptr*/, HWND hParent/*=nullptr*/)
{
	if (dwError)
	{
		wchar_t szInfo[64];
		msprintf(szInfo, countof(szInfo), L", ErrCode=x%X(%i)", dwError, static_cast<int>(dwError));
		const CEStr lsMsg(asLabel, szInfo);
		LogHookingStatus(GetCurrentProcessId(), lsMsg);
	}
	else
	{
		LogHookingStatus(GetCurrentProcessId(), asLabel);
	}
	return 0;
}

void CDefTermHk::ShowTrayIconError(LPCWSTR asErrText)
{
	LogHookingStatus(GetCurrentProcessId(), asErrText);
	DefTermMsg(asErrText);
}

HWND CDefTermHk::AllocHiddenConsole(const bool bTempForVS)
{
	if (!gpDefTerm)
	{
		_ASSERTE(gpDefTerm != nullptr);
		return nullptr;
	}

	// Function AttachConsole exists in WinXP and above, need dynamic link
	const auto attachConsole = GetAttachConsoleProc();
	if (!attachConsole)
	{
		gpDefTerm->LogHookingStatus(GetCurrentProcessId(), L"Can't create hidden console, function does not exist");
		return nullptr;
	}

	gpDefTerm->LogHookingStatus(GetCurrentProcessId(), L"AllocHiddenConsole");

	gpDefTerm->ReloadSettings();
	_ASSERTEX(IsDefTermEnabled() && (gbIsNetVsHost || bTempForVS || gbPrepareDefaultTerminal));

	if (!IsDefTermEnabled())
	{
		// Disabled in settings or registry
		gpDefTerm->LogHookingStatus(GetCurrentProcessId(), L"Application skipped by settings");
		return nullptr;
	}

	HANDLE hSrvProcess = nullptr;
	const DWORD nAttachPid = bTempForVS ? 0 : gnSelfPID;
	const DWORD nSrvPid = gpDefTerm->StartConsoleServer(nAttachPid, true, &hSrvProcess);
	if (!nSrvPid)
	{
		// Failed to start process?
		return nullptr;
	}
	_ASSERTEX(hSrvProcess!=nullptr);

	HWND hCreatedCon = nullptr;

	// Do while server process is alive
	const DWORD nStart = GetTickCount();
	const DWORD nMaxDelta = 30000;
	DWORD nDelta = 0;
	DWORD nWait = WaitForSingleObject(hSrvProcess, 0);
	while (nWait != WAIT_OBJECT_0)
	{
		if (attachConsole(nSrvPid))
		{
			hCreatedCon = GetRealConsoleWindow();
			if (hCreatedCon)
				break;
		}

		nWait = WaitForSingleObject(hSrvProcess, 150);

		nDelta = (GetTickCount() - nStart);
		if (nDelta > nMaxDelta)
			break;
	}

	return hCreatedCon;
}

DWORD CDefTermHk::StartConsoleServer(DWORD nAttachPid, bool bNewConWnd, PHANDLE phSrvProcess)
{
	if (!IsDefTermEnabled())
	{
		LogHookingStatus(GetCurrentProcessId(), L"Application skipped by settings");
		return 0;
	}
	// Options must be loaded already
	const CEDefTermOpt& opt = GetOpt();

	// ReSharper disable once CppEntityAssignedButNoRead
	bool bAttachCreated = false;

	CEStr szAddArgs, szAddArgs2;
	CEStr pszCmdLine;
	DWORD nErr = 0;
	STARTUPINFO si = {};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};

	size_t cchAddArgLen = GetSrvAddArgs(false, false, szAddArgs, szAddArgs2);

	wchar_t szExeName[MAX_PATH] = L"";
	if (GetModuleFileName(nullptr, szExeName, countof(szExeName)) && szExeName[0])
		cchAddArgLen += 20 + wcslen(szExeName);

	const size_t cchMax = MAX_PATH + 100 + cchAddArgLen;
	if (pszCmdLine.GetBuffer(cchMax))
	{
		_ASSERTE(nAttachPid || bNewConWnd);

		msprintf(pszCmdLine.data(), cchMax, L"\"%s\\%s\" /ATTACH %s/TRMPID=%u",
			opt.pszConEmuBaseDir,
			ConEmuC_EXE_3264,
			bNewConWnd ? L"/CREATECON " : L"",
			nAttachPid);

		if (szExeName[0])
		{
			_wcscat_c(pszCmdLine.data(), cchMax, L" /ROOTEXE \"");
			_wcscat_c(pszCmdLine.data(), cchMax, szExeName);
			_wcscat_c(pszCmdLine.data(), cchMax, L"\"");
		}

		if (!szAddArgs.IsEmpty())
			_wcscat_c(pszCmdLine.data(), cchMax, szAddArgs);

		_ASSERTEX(szAddArgs2.IsEmpty()); // No "-new_console" switches are expected here!

		const DWORD nCreateFlags = NORMAL_PRIORITY_CLASS | (bNewConWnd ? CREATE_NEW_CONSOLE : 0);
		if (bNewConWnd)
		{
			si.wShowWindow = SW_HIDE;
			si.dwFlags = STARTF_USESHOWWINDOW;
		}

		LogHookingStatus(GetCurrentProcessId(), pszCmdLine);

		if (CreateProcess(nullptr, pszCmdLine.data(), nullptr, nullptr,
			FALSE, nCreateFlags, nullptr, opt.pszConEmuBaseDir, &si, &pi))
		{
			LogHookingStatus(GetCurrentProcessId(), L"Console server was successfully created");
			bAttachCreated = true;
			if (phSrvProcess)
				*phSrvProcess = pi.hProcess;
			else
				CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
			SetServerPID(pi.dwProcessId);
		}
		else
		{
			nErr = GetLastError();
			CEStr pszErrMsg;
			if (pszErrMsg.GetBuffer(MAX_PATH * 3))
			{
				msprintf(pszErrMsg.data(), MAX_PATH * 3, L"ConEmuHk: Failed to start attach server, Err=%u! %s\n", nErr, pszCmdLine.c_str());
				LogHookingStatus(GetCurrentProcessId(), pszErrMsg);
				wchar_t szTitle[64];
				msprintf(szTitle, countof(szTitle), WIN3264TEST(L"ConEmuHk",L"ConEmuHk64") L", PID=%u", GetCurrentProcessId());
				//OutputDebugString(pszErrMsg);
				MessageBox(nullptr, pszErrMsg, szTitle, MB_ICONSTOP | MB_SYSTEMMODAL);
			}
			// Failed to start, pi.dwProcessId must be 0
			_ASSERTEX(pi.dwProcessId == 0);
			pi.dwProcessId = 0;
		}
	}

	std::ignore = bAttachCreated;
	return pi.dwProcessId;
}

bool CDefTermHk::FindConEmuInside(DWORD& guiPid, HWND& guiHwnd)
{
	const auto insideMapping = LoadInsideSettings();
	if (!insideMapping)
		return false;
	if (!insideMapping->nGuiPID || !insideMapping->hConEmuRoot || !IsWindow(insideMapping->hConEmuRoot))
		return false;

	guiPid = insideMapping->nGuiPID;
	guiHwnd = insideMapping->hConEmuRoot;
	return true;
}

void CDefTermHk::OnAllocConsoleFinished(HWND hNewConWnd)
{
	if (!gbPrepareDefaultTerminal || !this)
	{
		_ASSERTEX((gbPrepareDefaultTerminal && gpDefTerm) && "Must be called in DefTerm mode only");
		return;
	}

	if (!hNewConWnd || !IsWindow(hNewConWnd))
	{
		LogHookingStatus(GetCurrentProcessId(), L"OnAllocConsoleFinished: hNewConWnd must be initialized already!");
		_ASSERTEX(FALSE && "hNewConWnd must be initialized already!");
		return;
	}

	ReloadSettings();

	if (!IsDefTermEnabled())
	{
		_ASSERTEX(IsDefTermEnabled());
		// Disabled in settings or registry
		LogHookingStatus(GetCurrentProcessId(), L"OnAllocConsoleFinished: !isDefTermEnabled()");
		return;
	}

	// VsConsoleHost has some specifics (debugging managed apps)
	if (gbIsNetVsHost)
	{
		// It's expected, that after AllocConsole the RealConsole window is always visible
		const BOOL bConWasVisible = IsWindowVisible(hNewConWnd);
		_ASSERTEX(bConWasVisible);
		// To minimize blinking hide it immediately
		ShowWindow(hNewConWnd, SW_HIDE);
		LogHookingStatus(GetCurrentProcessId(), L"Console window was hidden");

		if (!StartConsoleServer(gnSelfPID, false, nullptr))
		{
			if (bConWasVisible)
				ShowWindow(hNewConWnd, SW_SHOW);
			LogHookingStatus(GetCurrentProcessId(), L"Starting attach server failed?");
		}
	}

	// If required - hook full set of console functions
	if (!(gnDllState & ds_HooksCommon))
	{
		// Add to list all **absent** functions
		InitHooksCommon();
		// Set hooks on all functions weren't hooked before
		SetAllHooks();
	}

	// Refresh ghConWnd, mapping, ServerPID, etc.
	OnConWndChanged(hNewConWnd);
}

size_t CDefTermHk::GetSrvAddArgs(const bool bGuiArgs, const bool forceInjects, CEStr& rsArgs, CEStr& rsNewCon)
{
	if (!gpDefTerm)
	{
		_ASSERTE(gpDefTerm != nullptr);
		return 0;
	}

	rsArgs.Clear();
	rsNewCon.Clear();

	// #TODO Replace with fast string buffer to optimize appends

	const auto& opt = gpDefTerm->m_Opt;

	const size_t cchMax = 128
		+ ((opt.pszCfgFile && *opt.pszCfgFile) ? (20 + wcslen(opt.pszCfgFile)) : 0)
		+ ((opt.pszConfigName && *opt.pszConfigName) ? (12 + wcslen(opt.pszConfigName)) : 0)
		;
	wchar_t* psz = rsArgs.GetBuffer(cchMax);
	const size_t cchNew = 32; // "-new_console:ni"
	wchar_t* pszNew = rsNewCon.GetBuffer(cchNew);

	if (!psz || !pszNew)
		return 0;

	*psz = 0;
	*pszNew = 0;

	wchar_t szNewConSw[10] = L"";

	DWORD guiPid = 0;
	HWND guiHwnd = nullptr;

	// Do not inject ConEmuHk in the target process?
	if (!forceInjects)
	{
		if (opt.bNoInjects && !bGuiArgs)
			_wcscat_c(psz, cchMax, L" /NOINJECT");
		else if (opt.bNoInjects)
			wcscat_c(szNewConSw, L"i");
	}

	// New or existing window we shall use?
	if (opt.bNewWindow && !bGuiArgs)
		_wcscat_c(psz, cchMax, L" /GHWND=NEW");
	else if (opt.bNewWindow)
		_wcscat_c(psz, cchMax, L" /NOSINGLE");
	else if (!opt.bNewWindow && !bGuiArgs && gpDefTerm->FindConEmuInside(guiPid, guiHwnd))
		msprintf(psz + wcslen(psz), cchMax - wcslen(psz), L" /GID=%u /GHWND=%08X", guiPid, LODWORD(guiHwnd));
	else if (bGuiArgs)
		_wcscat_c(psz, cchMax, L" /REUSE");

	// Confirmations
	if (opt.nDefaultTerminalConfirmClose == TerminalConfirmClose::Always)
	{
		if (!bGuiArgs)
			_wcscat_c(psz, cchMax, L" /CONFIRM");
		else
			wcscat_c(szNewConSw, L"c");
	}
	else if (opt.nDefaultTerminalConfirmClose == TerminalConfirmClose::Never)
	{
		if (!bGuiArgs)
			_wcscat_c(psz, cchMax, L" /NOCONFIRM");
		else
			wcscat_c(szNewConSw, L"n");
	}

	// That switches must be processed in server too!
	if (opt.pszCfgFile && *opt.pszCfgFile)
	{
		_wcscat_c(psz, cchMax, L" /LoadCfgFile \"");
		_wcscat_c(psz, cchMax, opt.pszCfgFile);
		_wcscat_c(psz, cchMax, L"\"");
	}
	if (opt.pszConfigName && *opt.pszConfigName)
	{
		_wcscat_c(psz, cchMax, L" /CONFIG \"");
		_wcscat_c(psz, cchMax, opt.pszConfigName);
		_wcscat_c(psz, cchMax, L"\"");
	}

	if (*psz)
	{
		_wcscat_c(psz, cchMax, L" ");
	}

	if (szNewConSw[0])
	{
		_wcscpy_c(pszNew, cchNew, L" -new_console:");
		_wcscat_c(pszNew, cchNew, szNewConSw);
		_wcscat_c(pszNew, cchNew, L" ");
	}

	const size_t cchLen = wcslen(psz) + wcslen(pszNew);
	return cchLen;
}
