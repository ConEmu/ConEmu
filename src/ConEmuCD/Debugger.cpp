
/*
Copyright (c) 2013-present Maximus5
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

#define SHOWDEBUGSTR
#define DEBUGSTRCMD(x) DEBUGSTR(x)
#define DEBUGSTRFIN(x) DEBUGSTR(x)
#define DEBUGSTRCP(x) DEBUGSTR(x)

#include "ConsoleMain.h"
#include "ConEmuSrv.h"
#include "ConsoleState.h"
#include "ExitCodes.h"

#include "../common/CEStr.h"
#include "../common/MMap.h"
#include "../common/MModule.h"
#include "../common/MProcess.h"
#include "../common/MProcessBits.h"
#include "../common/MStrDup.h"
#include "../common/shlobj.h"
#include "../common/TokenHelper.h"
#include "../common/WFiles.h"
#include "../common/WObjects.h"
#include "../common/WThreads.h"
#include "../ConEmu/version.h"

#if !defined(__GNUC__) || defined(__MINGW32__)
	#pragma warning(disable: 4091)
	#include <dbghelp.h>
	#pragma warning(default: 4091)
#else
	#include "../common/DbgHlpGcc.h"
#endif

#ifndef _T
	#define __T(x) L ## x
	#define _T(x) __T(x)
#endif


#define CE_TREE_TEMPLATE CE_CONEMUC_NAME_W L": DebuggerPID=%u RootPID=%u Count=%i"

DebuggerInfo::DebuggerInfo()
= default;

DebuggerInfo::~DebuggerInfo()
{
	SafeCloseHandle(hDebugReady);
	SafeCloseHandle(hDebugThread);
}

void DebuggerInfo::PrintDebugInfo() const
{
	Printf(CE_CONEMUC_NAME_A ": Debugger successfully attached to PID=%u\n", gpWorker->RootProcessId());
	TODO("Вывести информацию о загруженных модулях, потоках, и стеке потоков");
}

void DebuggerInfo::UpdateDebuggerTitle() const
{
	if (!this->bDebugProcessTree)
		return;

	wchar_t szTitle[100];
	const auto* pszTemplate = CE_TREE_TEMPLATE;
	if (GetConsoleTitle(szTitle, sizeof(szTitle)))
	{
		// Заголовок уже установлен, возможно из другого процесса,
		// при одновременном дампе/отладке нескольких процессов разных битностей
		if ((wcsncmp(szTitle, pszTemplate, 8) == 0) && wcsstr(szTitle, L"DebuggerPID"))
			return;
	}

	swprintf_c(szTitle, pszTemplate,
		GetCurrentProcessId(), gpWorker->RootProcessId(), this->nProcessCount);
	SetTitle(szTitle);
}

DumpProcessType DebuggerInfo::ConfirmDumpType(DWORD dwProcessId, LPCSTR asConfirmText /*= nullptr*/) const
{
	if (this->bAutoDump)
		return DumpProcessType::MiniDump; // Automatic MINI-dumps

	if (this->debugDumpProcess == DumpProcessType::MiniDump || this->debugDumpProcess == DumpProcessType::FullDump)
		return this->debugDumpProcess;

	// ANSI is used because of asConfirmText created as ANSI
	char szTitleA[64];
	sprintf_c(szTitleA, CE_CONEMUC_NAME_A " Debugging PID=%u, Debugger PID=%u", dwProcessId, GetCurrentProcessId());

	const auto nBtn = MessageBoxA(nullptr, asConfirmText ? asConfirmText : "Create minidump (<No> - fulldump)?", szTitleA, MB_YESNOCANCEL|MB_SYSTEMMODAL);

	switch (nBtn)
	{
	case IDYES:
		return DumpProcessType::MiniDump;
	case IDNO:
		return DumpProcessType::FullDump;
	default:
		return DumpProcessType::None;
	}
}

int DebuggerInfo::RunDebugger()
{
	if (!this->pDebugTreeProcesses)
	{
		this->pDebugTreeProcesses = static_cast<MMap<DWORD, CEDebugProcessInfo>*>(calloc(1, sizeof(*this->pDebugTreeProcesses)));
		this->pDebugTreeProcesses->Init(1024);
	}

	UpdateDebuggerTitle();

	// Increase console buffer size
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		HANDLE hCon = ghConOut;
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		GetConsoleScreenBufferInfo(hCon, &csbi);
		if (IsWindowVisible(gState.realConWnd_) && (csbi.dwSize.X < 260))
		{
			// Enlarge both width and height
			const COORD crNewSize = {260, 32000};
			SetConsoleScreenBufferSize(ghConOut, crNewSize);
		}
		else if (csbi.dwSize.Y < 1000)
		{
			// ConEmu do not support horizontal scrolling yet
			const COORD crNewSize = {csbi.dwSize.X, 32000};
			SetConsoleScreenBufferSize(ghConOut, crNewSize);
		}
	}

	// Вывести в консоль информацию о версии.
	PrintVersion();
	#ifdef SHOW_DEBUG_STARTED_MSGBOX
	wchar_t szInfo[128];
	StringCchPrintf(szInfo, countof(szInfo), L"Attaching debugger...\n" CE_CONEMUC_NAME_W " PID = %u\nDebug PID = %u",
	                GetCurrentProcessId(), gpWorker->RootProcessId());
	MessageBox(GetConEmuHWND(ConEmuWndType::ConsoleWindow), szInfo, CE_CONEMUC_NAME_W L".Debugger", 0);
	#endif

	if (this->szDebuggingCmdLine.IsEmpty())
	{
		const int iAttachRc = AttachRootProcessHandle();
		if (iAttachRc != 0)
			return iAttachRc;
	}
	else
	{
		_ASSERTE(!this->bDebuggerActive);
	}

	SafeFree(gpszRunCmd);
	gpszRunCmd = lstrdup(L"").Detach();
	if (!gpszRunCmd)
	{
		PrintBuffer("Can't allocate 1 wchar!\n");
		return CERR_NOTENOUGHMEM1;
	}

	_ASSERTE(((gpWorker->RootProcessHandle()!=nullptr) || (!this->szDebuggingCmdLine.IsEmpty())) && "Process handle must be opened");

	// Если просили сделать дамп нескольких процессов - нужно сразу уточнить его тип
	if (this->bDebugMultiProcess && (this->debugDumpProcess == DumpProcessType::AskUser))
	{
		// 2 - minidump, 3 - fulldump
		const auto nConfirmDumpType = ConfirmDumpType(gpWorker->RootProcessId(), nullptr);
		if (nConfirmDumpType < DumpProcessType::MiniDump)
		{
			// Отмена
			return CERR_CANTSTARTDEBUGGER;
		}
		this->debugDumpProcess = nConfirmDumpType;
	}

	// this->bDebuggerActive must be set in DebugThread

	if (this->bAutoDump)
		this->bDebuggerRequestDump = TRUE;

	// Run DebugThread
	this->hDebugReady = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// All debugger events are processed in special thread
	this->hDebugThread = apiCreateThread(DebugThread, nullptr, &this->dwDebugThreadId, "DebugThread");
	HANDLE hEvents[2] = {this->hDebugReady, this->hDebugThread};

	// First we must wait for debugger thread initialization finish
	const DWORD nReady = WaitForMultipleObjects(countof(hEvents), hEvents, FALSE, INFINITE);
	if (nReady != WAIT_OBJECT_0)
	{
		DWORD nExit = 0;
		GetExitCodeThread(this->hDebugThread, &nExit);
		return nExit;
	}

	// this->bDebuggerActive was set in DebugThread

	// And wait for debugger thread completion
	_ASSERTE(gState.runMode_ == RunMode::Undefined);
	DWORD nDebugThread; // = WaitForSingleObject(this->hDebugThread, INFINITE);
	const DWORD nDbgTimeout = std::min<DWORD>(std::max<DWORD>(25, this->nAutoInterval), 100);

	LONG nLastCounter = this->nDumpsCounter - 1; // First dump create always
	DWORD nLastTick = GetTickCount();
	while ((nDebugThread = WaitForSingleObject(this->hDebugThread, nDbgTimeout)) == WAIT_TIMEOUT)
	{
		TODO("Add console input reader");

		const DWORD nDelta = GetTickCount() - nLastTick;
		if (this->bAutoDump)
		{
			if ((nDelta + 10) >= this->nAutoInterval)
			{
				// Don't request next dump until previous was finished
				if (nLastCounter != this->nDumpsCounter)
				{
					nLastCounter = this->nDumpsCounter;
					// Request mini-dump
					GenerateMiniDumpFromCtrlBreak();
				}
				// Next step
				nLastTick = GetTickCount();
			}
		}
	}
	_ASSERTE(nDebugThread == WAIT_OBJECT_0); UNREFERENCED_PARAMETER(nDebugThread);

	gbInShutdown = TRUE;
	return 0;
}

HANDLE DebuggerInfo::GetProcessHandleForDebug(DWORD nPID, LPDWORD pnErrCode /*= nullptr*/) const
{
	_ASSERTE(this->bDebugProcess);

	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	HANDLE hProcess = nullptr;
	DWORD  nErrCode = 0;

	if ((nPID == gpWorker->RootProcessId()) && gpWorker->RootProcessHandle())
	{
		hProcess = gpWorker->RootProcessHandle();
	}
	else
	{
		CEDebugProcessInfo pi = {};

		if (this->pDebugTreeProcesses->Get(nPID, &pi) && pi.hProcess)
		{
			// Уже
			hProcess = pi.hProcess;
		}
		else
		{
			// Нужно открыть дескриптор процесса, это запущенный дочерний второго уровня

			DWORD dwFlags = PROCESS_QUERY_INFORMATION|SYNCHRONIZE;

			if (this->bDebuggerActive || this->bDebugProcessTree)
				dwFlags |= PROCESS_VM_READ;

			CAdjustProcessToken token;
			token.Enable(1, SE_DEBUG_NAME);

			// Сначала - пробуем "все права"
			// PROCESS_ALL_ACCESS (defined in new SDK) may fail on WinXP!
			hProcess = OpenProcess(MY_PROCESS_ALL_ACCESS, FALSE, nPID);
			if (!hProcess)
			{
				DEBUGTEST(nErrCode = GetLastError());

				hProcess = OpenProcess(dwFlags, FALSE, nPID);

				if (!hProcess)
				{
					nErrCode = GetLastError();
				}
			}

			token.Release();

			if (nPID != gpWorker->RootProcessId())
			{
				// Запомнить дескриптор
				_ASSERTE(pi.nPID == nPID || pi.nPID == 0); // уже должен быть
				pi.nPID = nPID;
				pi.hProcess = hProcess;
				this->pDebugTreeProcesses->Set(nPID, pi);
			}
		}
	}

	if (pnErrCode)
		*pnErrCode = nErrCode;

	return hProcess;
}

void DebuggerInfo::AttachConHost(DWORD nConHostPID) const
{
	DWORD  nErrCode = 0;
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hConHost = GetProcessHandleForDebug(nConHostPID, &nErrCode);

	if (!hConHost)
	{
		Printf(CE_CONEMUC_NAME_A ": Opening ConHost process handle failed, Code=%u\n", nErrCode);
	}
	else if (!DebugActiveProcess(nConHostPID))
	{
		nErrCode = GetLastError();
		Printf(CE_CONEMUC_NAME_A ": Attaching to ConHost process handle failed, Code=%u\n", nErrCode);
	}

	if (!nErrCode)
	{
		//TODO: Запустить ConEmuC (требуемой битности) под админом
	}
}

DWORD DebuggerInfo::DebugThread(LPVOID lpvParam)
{
	DWORD nWait = WAIT_TIMEOUT;
	wchar_t szInfo[1024] = L"";
	wchar_t szPID[20] = L"";
	int iAttachedCount = 0;
	// ReSharper disable once IdentifierTypo
	CEStr szOtherBitPids, szOtherDebugCmd;

	auto& dbgInfo = gpWorker->DbgInfo();

	// Avoid killing debugging program on when the debugger (our process) exits
	const auto& kernel = gpWorker->KernelModule();
	kernel.GetProcAddress("DebugActiveProcessStop", dbgInfo.pfnDebugActiveProcessStop);
	kernel.GetProcAddress("DebugSetProcessKillOnExit", dbgInfo.pfnDebugSetProcessKillOnExit);

	// Affect GetProcessHandleForDebug
	dbgInfo.bDebuggerActive = TRUE;

	// If dump was requested
	if (dbgInfo.debugDumpProcess != DumpProcessType::None)
	{
		dbgInfo.bUserRequestDump = TRUE;
		dbgInfo.bDebuggerRequestDump = TRUE;
	}

	// "/DEBUGEXE" or "/DEBUGTREE"
	if (!dbgInfo.szDebuggingCmdLine.IsEmpty())
	{
		STARTUPINFO si = {};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi = {};

		if (dbgInfo.bDebugProcessTree)
		{
			SetEnvironmentVariable(ENV_CONEMU_BLOCKCHILDDEBUGGERS_W, ENV_CONEMU_BLOCKCHILDDEBUGGERS_YES);
		}

		if (!CreateProcess(nullptr, dbgInfo.szDebuggingCmdLine.ms_Val, nullptr, nullptr, FALSE,
			NORMAL_PRIORITY_CLASS|CREATE_NEW_CONSOLE|
			DEBUG_PROCESS | (dbgInfo.bDebugProcessTree ? 0 : DEBUG_ONLY_THIS_PROCESS),
			nullptr, nullptr, &si, &pi))
		{
			const DWORD dwErr = GetLastError();

			wchar_t szProc[64] = L"";
			PROCESSENTRY32 info = {};
			if (GetProcessInfo(gpWorker->RootProcessId(), info))
				_wcscpyn_c(szProc, countof(szProc), info.szExeFile, countof(szProc));

			swprintf_c(szInfo, L"Can't start debugging process. ErrCode=0x%08X\n", dwErr);
			const CEStr lsInfo(szInfo, dbgInfo.szDebuggingCmdLine, L"\n");
			PrintBuffer(lsInfo);
			return CERR_CANTSTARTDEBUGGER;
		}

		gpWorker->SetRootProcessHandle(pi.hProcess);
		gpWorker->SetRootThreadHandle(pi.hThread);
		gpWorker->SetRootProcessId(pi.dwProcessId);
		gpWorker->SetRootThreadId(pi.dwThreadId);
		gpWorker->SetRootStartTime(GetTickCount());

		// Let's know that at least one process is debugging
		iAttachedCount++;
	}


	/* ************************* */
	int iDbgIdx = 0;
	bool bSetKillOnExit = true;

	while (true)
	{
		HANDLE hDbgProcess = nullptr;
		DWORD  nDbgProcessID = 0;

		if ((iDbgIdx++) == 0)
		{
			hDbgProcess = gpWorker->RootProcessHandle();
			nDbgProcessID = gpWorker->RootProcessId();
		}
		else
		{
			// Взять из pDebugAttachProcesses
			if (!dbgInfo.pDebugAttachProcesses)
				break;
			if (!dbgInfo.pDebugAttachProcesses->pop_back(nDbgProcessID))
				break;
			hDbgProcess = dbgInfo.GetProcessHandleForDebug(nDbgProcessID);
			if (!hDbgProcess)
			{
				_ASSERTE(hDbgProcess!=nullptr && "Can't open debugging process handle");
				continue;
			}
		}


		_ASSERTE(hDbgProcess!=nullptr && "Process handle must be opened");


		// Битность отладчика должна соответствовать битности приложения!
		if (IsWindows64())
		{
			int nBits = GetProcessBits(nDbgProcessID, hDbgProcess);
			if ((nBits == 32 || nBits == 64) && (nBits != WIN3264TEST(32,64)))
			{
				// If /DEBUGEXE or /DEBUGTREE was used
				if (!dbgInfo.szDebuggingCmdLine.IsEmpty())
				{
					PrintBuffer("Bitness of ConEmuC and debugging program does not match\n");
					continue;
				}

				// Добавить процесс в список для запуска альтернативного дебаггера соотвествующей битности
				// Force trailing "," even if only one PID specified ( --> bDebugMultiProcess = TRUE)
				szOtherBitPids.Append(ltow_s(nDbgProcessID, szPID, 10), L",");

				// Может там еще процессы в списке на дамп?
				continue;
			}
		}

		if (dbgInfo.szDebuggingCmdLine.IsEmpty())
		{
			if (DebugActiveProcess(nDbgProcessID))
			{
				iAttachedCount++;
			}
			else
			{
				DWORD dwErr = GetLastError();

				wchar_t szProc[64]; szProc[0] = 0;
				PROCESSENTRY32 info = {};
				if (GetProcessInfo(nDbgProcessID, info))
					_wcscpyn_c(szProc, countof(szProc), info.szExeFile, countof(szProc));

				swprintf_c(szInfo, L"Can't attach debugger to '%s' PID=%i. ErrCode=0x%08X\n",
					szProc[0] ? szProc : L"not found", nDbgProcessID, dwErr);
				PrintBuffer(szInfo);

				// Может другие подцепить получится?
				continue;
			}
		}

		// To avoid debugged processes killing
		if (bSetKillOnExit && dbgInfo.pfnDebugSetProcessKillOnExit)
		{
			// affects all current and future debuggees connected to the calling thread
			if (dbgInfo.pfnDebugSetProcessKillOnExit(FALSE/*KillOnExit*/))
			{
				bSetKillOnExit = false;
			}
		}
	}

	// Different bitness, need to start appropriate debugger
	if (szOtherBitPids.ms_Val && *szOtherBitPids.ms_Val)
	{
		wchar_t szExe[MAX_PATH+5], *pszName = nullptr;
		if (!GetModuleFileName(nullptr, szExe, MAX_PATH))
		{
			swprintf_c(szInfo, L"GetModuleFileName(nullptr) failed. ErrCode=0x%08X\n", GetLastError());
			PrintBuffer(szInfo);
		}
		else if (!((pszName = const_cast<wchar_t*>(PointToName(szExe)))))
		{
			swprintf_c(szInfo, L"GetModuleFileName(nullptr) returns invalid path\n%s\n", szExe);
			PrintBuffer(szInfo);
		}
		else
		{
			*pszName = 0;
			// Reverted to current bitness
			wcscat_c(szExe, WIN3264TEST(L"ConEmuC64.exe", L"ConEmuC.exe"));

			szOtherDebugCmd = CEStr(L"\"", szExe, L"\" "
				L"/DEBUGPID=", szOtherBitPids.ms_Val,
				(dbgInfo.debugDumpProcess == DumpProcessType::AskUser) ? L" /DUMP" :
				(dbgInfo.debugDumpProcess == DumpProcessType::MiniDump) ? L" /MINIDUMP" :
				(dbgInfo.debugDumpProcess == DumpProcessType::FullDump) ? L" /FULLDUMP" : L"");

			STARTUPINFO si = {};
			si.cb = sizeof(si);
			PROCESS_INFORMATION pi = {};
			if (CreateProcessW(nullptr, szOtherDebugCmd.ms_Val, nullptr, nullptr, TRUE,
				NORMAL_PRIORITY_CLASS, nullptr, nullptr, &si, &pi))
			{
				// Don't wait
				SafeCloseHandle(pi.hProcess);
				SafeCloseHandle(pi.hThread);
			}
			else
			{
				DWORD dwErr = GetLastError();
				swprintf_c(szInfo, L"Can't start external debugger, ErrCode=0x%08X\n", dwErr);
				const CEStr lsInfo(szInfo, szOtherDebugCmd, L"\n");
				PrintBuffer(lsInfo);
			}
		}
	}

	//_ASSERTE(FALSE && "Continue to dump");

	// If neither /DEBUG[EXE|TREE] nor /DEBUGPID was not succeeded
	if (iAttachedCount == 0)
	{
		dbgInfo.bDebuggerActive = FALSE;
		return CERR_CANTSTARTDEBUGGER;
	}
	else if (iAttachedCount > 1)
	{
		_ASSERTE(dbgInfo.bDebugMultiProcess && "Already must be set from arguments parser");
		dbgInfo.bDebugMultiProcess = TRUE;
	}

	if (dbgInfo.bUserRequestDump)
	{
		dbgInfo.nWaitTreeBreaks = iAttachedCount;
	}

	/* **************** */

	// To avoid debugged processes killing (JIC, must be called already)
	if (bSetKillOnExit && dbgInfo.pfnDebugSetProcessKillOnExit)
	{
		// affects all current and future debuggees connected to the calling thread
		if (dbgInfo.pfnDebugSetProcessKillOnExit(FALSE/*KillOnExit*/))
		{
			bSetKillOnExit = false;
		}
	}

	dbgInfo.PrintDebugInfo();
	SetEvent(dbgInfo.hDebugReady);


	while (nWait == WAIT_TIMEOUT)
	{
		dbgInfo.ProcessDebugEvent();

		if (ghExitQueryEvent)
			nWait = WaitForSingleObject(ghExitQueryEvent, 0);
	}

//done:
	gState.rootAliveLess10sec_ = FALSE;
	gbInShutdown = TRUE;
	gState.alwaysConfirmExit_ = FALSE;

	_ASSERTE(gbTerminateOnCtrlBreak==FALSE);

	if (!nExitQueryPlace) nExitQueryPlace = 12+(nExitPlaceStep);

	SetTerminateEvent(ste_DebugThread);
	return 0;
}

bool DebuggerInfo::IsDumpMulti() const
{
	if (this->bDebugProcessTree || this->bDebugMultiProcess)
		return true;
	return false;
}

void DebuggerInfo::FormatDumpName(wchar_t* DmpFile, size_t cchDmpMax, DWORD dwProcessId, bool bTrap, bool bFull) const
{
	//TODO: Добавить в DmpFile имя без пути? <exename>-<ver>-<pid>-<yymmddhhmmss>.[m]dmp
	wchar_t szMinor[8] = L""; lstrcpyn(szMinor, _T(MVV_4a), countof(szMinor));
	SYSTEMTIME lt = {}; GetLocalTime(&lt);
	swprintf_c(DmpFile, cchDmpMax/*#SECURELEN*/, L"%s-%02u%02u%02u%s-%u-%02u%02u%02u%02u%02u%02u%03u.%s",
		bTrap ? L"Trap" : L"CEDump",
		MVV_1, MVV_2, MVV_3, szMinor, dwProcessId,
		lt.wYear%100, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds,
		bFull ? L"dmp" : L"mdmp");
}

bool DebuggerInfo::GetSaveDumpName(DWORD dwProcessId, bool bFull, wchar_t* dmpFile, const DWORD cchMaxDmpFile) const
{
	bool bRc = false;

	MModule hComdlg32;
	// ReSharper disable once IdentifierTypo
	typedef BOOL (WINAPI* GetSaveFileName_t)(LPOPENFILENAMEW lpofn);
	GetSaveFileName_t getSaveFileName = nullptr;
	const bool bDumpMulti = IsDumpMulti() || this->bAutoDump;

	if (!bDumpMulti)
	{
		// ReSharper disable once StringLiteralTypo
		if (hComdlg32.Load(L"COMDLG32.dll"))
			hComdlg32.GetProcAddress("GetSaveFileNameW", getSaveFileName);

		if (getSaveFileName)
		{
			OPENFILENAMEW ofn; memset(&ofn,0,sizeof(ofn));
			ofn.lStructSize=sizeof(ofn);
			ofn.hwndOwner = nullptr;
			// ReSharper disable once StringLiteralTypo
			ofn.lpstrFilter = L"Debug dumps (*.mdmp)\0*.mdmp;*.dmp\0Debug dumps (*.dmp)\0*.dmp;*.mdmp\0\0";
			ofn.nFilterIndex = bFull ? 2 : 1;
			ofn.lpstrFile = dmpFile;
			ofn.nMaxFile = cchMaxDmpFile;
			ofn.lpstrTitle = bFull ? L"Save debug full-dump" : L"Save debug mini-dump";
			ofn.lpstrDefExt = bFull ? L"dmp" : L"mdmp";
			ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
			            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

			if (getSaveFileName(&ofn))
			{
				bRc = true;
			}
		}

		hComdlg32.Free();
	}

	if (bDumpMulti || !getSaveFileName)
	{
		HRESULT dwErr = SHGetFolderPath(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0/*SHGFP_TYPE_CURRENT*/, dmpFile);
		if (FAILED(dwErr))
		{
			memset(dmpFile, 0, cchMaxDmpFile*sizeof(*dmpFile));
			if (GetTempPath(cchMaxDmpFile-32, dmpFile) && *dmpFile)
				dwErr = S_OK;
		}

		if (FAILED(dwErr))
		{
			Printf("\n" CE_CONEMUC_NAME_A ": GetSaveDumpName called, get desktop folder failed, code=%u\n", DWORD(dwErr));
		}
		else
		{
			if (*dmpFile && dmpFile[lstrlen(dmpFile)-1] != L'\\')
				_wcscat_c(dmpFile, cchMaxDmpFile, L"\\");

			_wcscat_c(dmpFile, cchMaxDmpFile, L"ConEmuTrap");
			CreateDirectory(dmpFile, nullptr);

			INT_PTR nLen = lstrlen(dmpFile);
			dmpFile[nLen++] = L'\\'; dmpFile[nLen] = 0;
			FormatDumpName(dmpFile+nLen, cchMaxDmpFile-nLen, dwProcessId, (this->bDebuggerRequestDump!=FALSE), bFull);

			bRc = true;
		}
	}

	return bRc;
}

// gpWorker->RootProcessPid()
void DebuggerInfo::WriteMiniDump(DWORD dwProcessId, DWORD dwThreadId, EXCEPTION_RECORD *pExceptionRecord, LPCSTR asConfirmText /*= nullptr*/, BOOL bTreeBreak /*= FALSE*/)
{
	// 2 - minidump, 3 - fulldump
	const auto nConfirmDumpType = ConfirmDumpType(dwProcessId, asConfirmText);
	if (nConfirmDumpType < DumpProcessType::MiniDump)
	{
		// Отмена
		return;
	}

	const MINIDUMP_TYPE dumpType = (nConfirmDumpType == DumpProcessType::MiniDump) ? MiniDumpNormal : MiniDumpWithFullMemory;

	// Т.к. в режиме "ProcessTree" мы пишем пачку дампов - спрашивать тип дампа будем один раз.
	if (IsDumpMulti() // several processes were attached
		&& (this->debugDumpProcess <= DumpProcessType::AskUser)) // 2 - minidump, 3 - fulldump
	{
		this->debugDumpProcess = nConfirmDumpType;
	}

	if (bTreeBreak)
	{
		GenerateTreeDebugBreak(dwProcessId);
	}

	bool bDumpSucceeded = false;
	HANDLE hDmpFile = nullptr;
	//HMODULE hDbghelp = nullptr;
	wchar_t szErrInfo[MAX_PATH*2];

	wchar_t szTitle[64];
	swprintf_c(szTitle, CE_CONEMUC_NAME_W L" Debugging PID=%u, Debugger PID=%u", dwProcessId, GetCurrentProcessId());

	wchar_t dmpFile[MAX_PATH] = L"";
	FormatDumpName(dmpFile, countof(dmpFile), dwProcessId, false, (dumpType == MiniDumpWithFullMemory));

	typedef BOOL (WINAPI* MiniDumpWriteDump_t)(HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
	        PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	        PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
	MiniDumpWriteDump_t MiniDumpWriteDump_f = nullptr;

	while (GetSaveDumpName(dwProcessId, (dumpType == MiniDumpWithFullMemory), dmpFile, countof(dmpFile)))
	{
		if (hDmpFile != INVALID_HANDLE_VALUE && hDmpFile != nullptr)
		{
			CloseHandle(hDmpFile); hDmpFile = nullptr;
		}

		hDmpFile = CreateFileW(dmpFile, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_FLAG_WRITE_THROUGH, nullptr);

		if (hDmpFile == INVALID_HANDLE_VALUE)
		{
			const DWORD nErr = GetLastError();
			swprintf_c(szErrInfo, L"Can't create debug dump file\n%s\nErrCode=0x%08X\n\nChoose another name?", dmpFile, nErr);

			if (MessageBoxW(nullptr, szErrInfo, szTitle, MB_YESNO|MB_SYSTEMMODAL|MB_ICONSTOP)!=IDYES)
				break;

			continue; // try to select file name again
		}

		if (!this->dbgHelpDll)
		{
			// ReSharper disable once StringLiteralTypo
			if (!this->dbgHelpDll.Load(L"Dbghelp.dll"))
			{
				const DWORD nErr = GetLastError();
				swprintf_c(szErrInfo, L"Can't load debug library 'Dbghelp.dll'\nErrCode=0x%08X\n\nTry again?", nErr);

				if (MessageBoxW(nullptr, szErrInfo, szTitle, MB_YESNO|MB_SYSTEMMODAL|MB_ICONSTOP)!=IDYES)
					break;

				continue; // try to select file name again
			}
		}

		if (this->MiniDumpWriteDump_f)
		{
			MiniDumpWriteDump_f = reinterpret_cast<MiniDumpWriteDump_t>(this->MiniDumpWriteDump_f);
		}
		else if (!MiniDumpWriteDump_f)
		{
			this->dbgHelpDll.GetProcAddress("MiniDumpWriteDump", MiniDumpWriteDump_f);

			if (!MiniDumpWriteDump_f)
			{
				const DWORD nErr = GetLastError();
				swprintf_c(szErrInfo, L"Can't locate 'MiniDumpWriteDump' in library 'Dbghelp.dll', ErrCode=%u", nErr);
				MessageBoxW(nullptr, szErrInfo, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
				break;
			}

			this->MiniDumpWriteDump_f = reinterpret_cast<FARPROC>(MiniDumpWriteDump_f);
		}

		if (MiniDumpWriteDump_f)
		{
			#if 0
			MINIDUMP_EXCEPTION_INFORMATION mei = {};
			mei.ThreadId = dwThreadId;
			EXCEPTION_POINTERS ep = {};
			ep.ExceptionRecord = pExceptionRecord;
			ep.ContextRecord = nullptr; // no way to get it here
			// ReSharper disable once CppAssignedValueIsNeverUsed
			mei.ExceptionPointers = &ep;
			// ReSharper disable once CppAssignedValueIsNeverUsed
			mei.ClientPointers = FALSE;
			// ReSharper disable once CppLocalVariableMayBeConst
			// ReSharper disable once IdentifierTypo
			#endif
			MINIDUMP_EXCEPTION_INFORMATION* pmei = nullptr; // #TODO use mei properly?
			PrintBuffer("Creating minidump: ");
			PrintBuffer(dmpFile);
			PrintBuffer("...");

			// ReSharper disable once CppLocalVariableMayBeConst
			HANDLE hProcess = GetProcessHandleForDebug(dwProcessId);

			const BOOL lbDumpRc = MiniDumpWriteDump_f(
				hProcess, dwProcessId,
				hDmpFile,
				dumpType,
				pmei,
				nullptr, nullptr);

			if (!lbDumpRc)
			{
				const DWORD nErr = GetLastError();
				swprintf_c(szErrInfo, L"MiniDumpWriteDump failed.\nErrorCode=0x%08X", nErr);
				Printf("\n" CE_CONEMUC_NAME_A ": Failed, ErrorCode=0x%08X\n", nErr);
				MessageBoxW(nullptr, szErrInfo, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
			}
			else
			{
				const int iLeft = (this->nWaitTreeBreaks > 0) ? (this->nWaitTreeBreaks - 1) : 0;
				Printf("\nMiniDumpWriteDump succeeded, %i left\n", iLeft);
				bDumpSucceeded = true;
			}

			break;
		}

	} // end while (GetSaveDumpName(dwProcessId, (dumpType == MiniDumpWithFullMemory), dmpfile, countof(dmpfile)))

	if (hDmpFile != INVALID_HANDLE_VALUE && hDmpFile != nullptr)
	{
		CloseHandle(hDmpFile);
	}


	if (IsWinXP() // Win2k had no function to detach from process
		&& bDumpSucceeded && this->bUserRequestDump
		// check if all dumps were created
		&& (this->nWaitTreeBreaks <= 1)
		// check if that's not a /DEBUGEXE or /DEBUGTREE
		&& !this->szDebuggingCmdLine
		)
	{
		// After completing all dumps - exit
		SetTerminateEvent(ste_WriteMiniDump);
	}
}

void DebuggerInfo::ProcessDebugEvent()
{
	static wchar_t wszDbgText[1024];
	static char szDbgText[1024];
	BOOL lbNonContinuable = FALSE;
	DEBUG_EVENT evt = {};
	BOOL lbEvent = WaitForDebugEvent(&evt,10);
	#ifdef _DEBUG
	const DWORD dwErr = GetLastError();
	#endif
	static bool bFirstExitThreadEvent = false; // Чтобы вывести на экран подсказку по возможностям "дебаггера"
	//HMODULE hCOMDLG32 = nullptr;
	//typedef BOOL (WINAPI* GetSaveFileName_t)(LPOPENFILENAMEW lpofn);
	//GetSaveFileName_t _GetSaveFileName = nullptr;
	DWORD dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;


	if (lbEvent)
	{
		lbNonContinuable = FALSE;

		switch (evt.dwDebugEventCode)
		{
			case CREATE_PROCESS_DEBUG_EVENT:
			case CREATE_THREAD_DEBUG_EVENT:
			case EXIT_PROCESS_DEBUG_EVENT:
			case EXIT_THREAD_DEBUG_EVENT:
			case RIP_EVENT:
			{
				LPCSTR pszName = "Unknown";

				switch (evt.dwDebugEventCode)
				{
					case CREATE_PROCESS_DEBUG_EVENT: pszName = "CREATE_PROCESS_DEBUG_EVENT"; break;
					case CREATE_THREAD_DEBUG_EVENT: pszName = "CREATE_THREAD_DEBUG_EVENT"; break;
					case EXIT_PROCESS_DEBUG_EVENT: pszName = "EXIT_PROCESS_DEBUG_EVENT"; break;
					case EXIT_THREAD_DEBUG_EVENT: pszName = "EXIT_THREAD_DEBUG_EVENT"; break;
					case RIP_EVENT: pszName = "RIP_EVENT"; break;
				}

				sprintf_c(szDbgText, "{%i.%i} %s\n", evt.dwProcessId,evt.dwThreadId, pszName);
				PrintBuffer(szDbgText);
				if (!bFirstExitThreadEvent && evt.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
				{
					bFirstExitThreadEvent = true;
					if (this->debugDumpProcess == DumpProcessType::None)
					{
						PrintBuffer(CE_CONEMUC_NAME_A ": Press Ctrl+Break to create minidump of debugging process\n");
					}
					//else // -- wrong way, HandlerRoutine will/may fail on first attach
					//{
					//	// Сразу сделать дамп и выйти
					//	HandlerRoutine(CTRL_BREAK_EVENT);
					//}
				}

				if (evt.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
				{
					this->nProcessCount++;

					_ASSERTE(this->pDebugTreeProcesses!=nullptr);
					CEDebugProcessInfo pi = {evt.dwProcessId};
					this->pDebugTreeProcesses->Set(evt.dwProcessId, pi);

					UpdateDebuggerTitle();
				}
				else if (evt.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
				{
					CEDebugProcessInfo pi = {};
					if (this->pDebugTreeProcesses
						&& this->pDebugTreeProcesses->Get(evt.dwProcessId, &pi, true)
						&& pi.hProcess)
					{
						CloseHandle(pi.hProcess);
					}

					if (this->nProcessCount > 0)
						this->nProcessCount--;

					UpdateDebuggerTitle();
				}

				break;
			}

			case LOAD_DLL_DEBUG_EVENT:
			case UNLOAD_DLL_DEBUG_EVENT:
			{
				LPCSTR pszName = "Unknown";
				char szBase[32] = {};
				char szFile[MAX_PATH+128] = {};

				struct MY_FILE_NAME_INFO
				{
					DWORD FileNameLength;
					WCHAR FileName[1];
				};
				typedef BOOL (WINAPI* GetFileInformationByHandleEx_t)(HANDLE hFile, int FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);
				static GetFileInformationByHandleEx_t _GetFileInformationByHandleEx = nullptr;

				switch (evt.dwDebugEventCode)
				{
					case LOAD_DLL_DEBUG_EVENT:
						//6 Reports a load-dynamic-link-library (DLL) debugging event. The value of u.LoadDll specifies a LOAD_DLL_DEBUG_INFO structure.
						pszName = "LOAD_DLL_DEBUG_EVENT";

						if (evt.u.LoadDll.hFile)
						{
							if (IsWin6())
							{
								if (!_GetFileInformationByHandleEx)
									MModule(GetModuleHandle(L"kernel32.dll")).GetProcAddress("GetFileInformationByHandleEx", _GetFileInformationByHandleEx);

								if (_GetFileInformationByHandleEx)
								{
									DWORD nSize = sizeof(MY_FILE_NAME_INFO) + MAX_PATH * sizeof(wchar_t);
									auto* pfi = static_cast<MY_FILE_NAME_INFO*>(calloc(nSize+2, 1));
									if (pfi)
									{
										pfi->FileNameLength = MAX_PATH;
										if (_GetFileInformationByHandleEx(evt.u.LoadDll.hFile, 2/*FileNameInfo*/, pfi, nSize)
											&& pfi->FileName[0])
										{
											wchar_t szFullPath[MAX_PATH+1] = {}, *pszFile;
											DWORD n = GetFullPathName(pfi->FileName, countof(szFullPath), szFullPath, &pszFile);
											if (!n || (n >= countof(szFullPath)))
											{
												lstrcpyn(szFullPath, pfi->FileName, countof(szFullPath));
												pszFile = const_cast<wchar_t*>(PointToName(pfi->FileName));
											}
											else if (!pszFile)
											{
												pszFile = const_cast<wchar_t*>(PointToName(szFullPath));
											}
											lstrcpyA(szFile, ", ");
											WideCharToMultiByte(CP_OEMCP, 0, pszFile, -1, szFile+lstrlenA(szFile), 80, 0,0);
											lstrcatA(szFile, "\n\t");
											WideCharToMultiByte(CP_OEMCP, 0, szFullPath, -1, szFile+lstrlenA(szFile), MAX_PATH, 0,0);
										}
										free(pfi);
									}
								}
							}
							CloseHandle(evt.u.LoadDll.hFile);
						}
						sprintf_c(szBase,
						           " at " WIN3264TEST("0x%08X","0x%08X%08X"),
						           WIN3264WSPRINT((DWORD_PTR)evt.u.LoadDll.lpBaseOfDll));

						break;
					case UNLOAD_DLL_DEBUG_EVENT:
						//7 Reports an unload-DLL debugging event. The value of u.UnloadDll specifies an UNLOAD_DLL_DEBUG_INFO structure.
						pszName = "UNLOAD_DLL_DEBUG_EVENT";
						sprintf_c(szBase,
						           " at " WIN3264TEST("0x%08X","0x%08X%08X"),
						           WIN3264WSPRINT((DWORD_PTR)evt.u.UnloadDll.lpBaseOfDll));
						break;
				}

				sprintf_c(szDbgText, "{%i.%i} %s%s%s\n", evt.dwProcessId,evt.dwThreadId, pszName, szBase, szFile);
				PrintBuffer(szDbgText);
				break;
			} // LOAD_DLL_DEBUG_EVENT, UNLOAD_DLL_DEBUG_EVENT

			case EXCEPTION_DEBUG_EVENT:
				//1 Reports an exception debugging event. The value of u.Exception specifies an EXCEPTION_DEBUG_INFO structure.
			{
				lbNonContinuable = (evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE)==EXCEPTION_NONCONTINUABLE;

				switch (evt.u.Exception.ExceptionRecord.ExceptionCode)
				{
					case EXCEPTION_ACCESS_VIOLATION: // The thread tried to read from or write to a virtual address for which it does not have the appropriate access.
					{
						if (evt.u.Exception.ExceptionRecord.NumberParameters>=2)
						{
							sprintf_c(szDbgText,
							           "{%i.%i} EXCEPTION_ACCESS_VIOLATION at " WIN3264TEST("0x%08X","0x%08X%08X") " flags 0x%08X%s %s of " WIN3264TEST("0x%08X","0x%08X%08X") " FC=%u\n", evt.dwProcessId,evt.dwThreadId,
							           WIN3264WSPRINT((DWORD_PTR)evt.u.Exception.ExceptionRecord.ExceptionAddress),
							           evt.u.Exception.ExceptionRecord.ExceptionFlags,
							           ((evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE) ? "(EXCEPTION_NONCONTINUABLE)" : ""),
							           ((evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==0) ? "Read" :
							            (evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==1) ? "Write" :
							            (evt.u.Exception.ExceptionRecord.ExceptionInformation[0]==8) ? "DEP" : "???"),
							           WIN3264WSPRINT(evt.u.Exception.ExceptionRecord.ExceptionInformation[1]),
							           evt.u.Exception.dwFirstChance
							          );
						}
						else
						{
							sprintf_c(szDbgText,
							           "{%i.%i} EXCEPTION_ACCESS_VIOLATION at " WIN3264TEST("0x%08X","0x%08X%08X") " flags 0x%08X%s FC=%u\n", evt.dwProcessId,evt.dwThreadId,
							           WIN3264WSPRINT((DWORD_PTR)evt.u.Exception.ExceptionRecord.ExceptionAddress),
							           evt.u.Exception.ExceptionRecord.ExceptionFlags,
							           (evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE) ? "(EXCEPTION_NONCONTINUABLE)" : "",
							           evt.u.Exception.dwFirstChance);
						}

						PrintBuffer(szDbgText);
					} // EXCEPTION_ACCESS_VIOLATION
					break;
					case MS_VC_THREADNAME_EXCEPTION:
					{
						sprintf_c(szDbgText,
							"{%i.%i} %s at " WIN3264TEST("0x%08X", "0x%08X%08X") " flags 0x%08X%s FC=%u\n",
							evt.dwProcessId, evt.dwThreadId,
							"MS_VC_THREADNAME_EXCEPTION",
							WIN3264WSPRINT((DWORD_PTR)evt.u.Exception.ExceptionRecord.ExceptionAddress),
							evt.u.Exception.ExceptionRecord.ExceptionFlags,
							(evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE)
							? "(EXCEPTION_NONCONTINUABLE)" : "",
							evt.u.Exception.dwFirstChance);
						PrintBuffer(szDbgText);
					} // MS_VC_THREADNAME_EXCEPTION
					break;
					default:
					{
						char szName[32]; LPCSTR pszName; pszName = szName;
						#define EXCASE(s) case s: pszName = #s; break

						switch(evt.u.Exception.ExceptionRecord.ExceptionCode)
						{
								EXCASE(EXCEPTION_ARRAY_BOUNDS_EXCEEDED); // The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.
								EXCASE(EXCEPTION_BREAKPOINT); // A breakpoint was encountered.
								EXCASE(EXCEPTION_DATATYPE_MISALIGNMENT); // The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.
								EXCASE(EXCEPTION_FLT_DENORMAL_OPERAND); // One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.
								EXCASE(EXCEPTION_FLT_DIVIDE_BY_ZERO); // The thread tried to divide a floating-point value by a floating-point divisor of zero.
								EXCASE(EXCEPTION_FLT_INEXACT_RESULT); // The result of a floating-point operation cannot be represented exactly as a decimal fraction.
								EXCASE(EXCEPTION_FLT_INVALID_OPERATION); // This exception represents any floating-point exception not included in this list.
								EXCASE(EXCEPTION_FLT_OVERFLOW); // The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.
								EXCASE(EXCEPTION_FLT_STACK_CHECK); // The stack overflowed or underflowed as the result of a floating-point operation.
								EXCASE(EXCEPTION_FLT_UNDERFLOW); // The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.
								EXCASE(EXCEPTION_ILLEGAL_INSTRUCTION); // The thread tried to execute an invalid instruction.
								EXCASE(EXCEPTION_IN_PAGE_ERROR); // The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.
								EXCASE(EXCEPTION_INT_DIVIDE_BY_ZERO); // The thread tried to divide an integer value by an integer divisor of zero.
								EXCASE(EXCEPTION_INT_OVERFLOW); // The result of an integer operation caused a carry out of the most significant bit of the result.
								EXCASE(EXCEPTION_INVALID_DISPOSITION); // An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.
								EXCASE(EXCEPTION_NONCONTINUABLE_EXCEPTION); // The thread tried to continue execution after a noncontinuable exception occurred.
								EXCASE(EXCEPTION_PRIV_INSTRUCTION); // The thread tried to execute an instruction whose operation is not allowed in the current machine mode.
								EXCASE(EXCEPTION_SINGLE_STEP); // A trace trap or other single-instruction mechanism signaled that one instruction has been executed.
								EXCASE(EXCEPTION_STACK_OVERFLOW); // The thread used up its stack.
							default:
								sprintf_c(szName,
									        "Exception 0x%08X", evt.u.Exception.ExceptionRecord.ExceptionCode);
						}

						sprintf_c(szDbgText,
							        "{%i.%i} %s at " WIN3264TEST("0x%08X","0x%08X%08X") " flags 0x%08X%s FC=%u\n",
							        evt.dwProcessId,evt.dwThreadId,
							        pszName,
							        WIN3264WSPRINT((DWORD_PTR)evt.u.Exception.ExceptionRecord.ExceptionAddress),
							        evt.u.Exception.ExceptionRecord.ExceptionFlags,
							        (evt.u.Exception.ExceptionRecord.ExceptionFlags&EXCEPTION_NONCONTINUABLE)
							        ? "(EXCEPTION_NONCONTINUABLE)" : "",
							        evt.u.Exception.dwFirstChance);
						PrintBuffer(szDbgText);
					}
				}

				BOOL bDumpOnBreakPoint = this->bDebuggerRequestDump;

				// Если просили цеплять к отладчику создаваемые дочерние процессы
				// или сделать дампы пачки процессов сразу
				if (IsDumpMulti()
					&& (!lbNonContinuable && (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT)))
				{
					// Когда отладчик цепляется к процессу в первый раз - возникает EXCEPTION_BREAKPOINT
					CEDebugProcessInfo pi = {};
					if (this->pDebugTreeProcesses
						&& this->pDebugTreeProcesses->Get(evt.dwProcessId, &pi))
					{
						if (!pi.bWasBreak)
						{
							pi.bWasBreak = TRUE;
							this->pDebugTreeProcesses->Set(evt.dwProcessId, pi);
							if (this->bUserRequestDump)
								bDumpOnBreakPoint = TRUE;
						}
						else
						{
							bDumpOnBreakPoint = TRUE;
						}
					}
				}

				if (this->bDebuggerRequestDump
					|| (!lbNonContinuable && !this->bDebugProcessTree
						&& (evt.u.Exception.ExceptionRecord.ExceptionCode != MS_VC_THREADNAME_EXCEPTION)
						&& (evt.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT))
					|| (IsDumpMulti()
						&& ((evt.u.Exception.ExceptionRecord.ExceptionCode>=0xC0000000)
							|| (bDumpOnBreakPoint && (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT))))
					)
				{
					BOOL bGenerateTreeBreak = this->bDebugProcessTree
						&& (this->bDebuggerRequestDump || lbNonContinuable);

					if (this->bDebugProcessTree
						&& !bGenerateTreeBreak
						&& (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT))
					{
						if (this->nWaitTreeBreaks == 0)
						{
							bGenerateTreeBreak = TRUE;
							this->nWaitTreeBreaks++;
						}
					}

					this->bDebuggerRequestDump = FALSE; // один раз

					char szConfirm[2048];

					if (evt.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
					{
						if (this->debugDumpProcess >= DumpProcessType::AskUser || this->bAutoDump)
							szConfirm[0] = 0;
						else
							lstrcpynA(szConfirm, szDbgText, countof(szConfirm));
					}
					else
					{
						sprintf_c(szConfirm, "%s exception (FC=%u)\n",
							lbNonContinuable ? "Non continuable" : "Continuable",
							evt.u.Exception.dwFirstChance);
						StringCchCatA(szConfirm, countof(szConfirm), szDbgText);
					}
					StringCchCatA(szConfirm, countof(szConfirm), "\nCreate minidump (<No> - fulldump)?");

					//bGenerateTreeBreak -> GenerateTreeDebugBreak(evt.dwProcessId)

					WriteMiniDump(evt.dwProcessId, evt.dwThreadId, &evt.u.Exception.ExceptionRecord, szConfirm, bGenerateTreeBreak);

					if (IsDumpMulti() && (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT))
					{
						if (this->nWaitTreeBreaks > 0)
							this->nWaitTreeBreaks--;
					}

					this->nDumpsCounter++;
				}

				if (!lbNonContinuable /*|| (evt.u.Exception.ExceptionRecord.ExceptionCode==EXCEPTION_BREAKPOINT)*/)
				{
					dwContinueStatus = DBG_CONTINUE;
				}

				break;
			} // EXCEPTION_DEBUG_EVENT

			case OUTPUT_DEBUG_STRING_EVENT:
				//8 Reports an output-debugging-string debugging event. The value of u.DebugString specifies an OUTPUT_DEBUG_STRING_INFO structure.
			{
				wszDbgText[0] = 0;
				msprintf(wszDbgText, countof(wszDbgText), L"{PID=%i.TID=%i} ", evt.dwProcessId, evt.dwThreadId);
				int iPidTidLen = lstrlen(wszDbgText);
				DWORD_PTR iReadMax = countof(wszDbgText) - iPidTidLen - 2;
				wchar_t* pwszDbg = wszDbgText + iPidTidLen;

				if (evt.u.DebugString.nDebugStringLength > iReadMax)
					evt.u.DebugString.nDebugStringLength = LOWORD(iReadMax);

				DWORD_PTR nRead = 0;

				HANDLE hProcess = GetProcessHandleForDebug(evt.dwProcessId);

				if (evt.u.DebugString.fUnicode)
				{
					if (!ReadProcessMemory(hProcess, evt.u.DebugString.lpDebugStringData, pwszDbg, 2*evt.u.DebugString.nDebugStringLength, &nRead))
					{
						_wcscpy_c(pwszDbg, iReadMax, L"???");
					}
					else
					{
						pwszDbg[std::min<size_t>(iReadMax, nRead+1)] = 0;
					}

					static int nPrefixLen = lstrlen(CONEMU_CONHOST_CREATED_MSG);
					if (memcmp(pwszDbg, CONEMU_CONHOST_CREATED_MSG, nPrefixLen*sizeof(pwszDbg[0])) == 0)
					{
						LPWSTR pszEnd = nullptr;
						DWORD nConHostPID = wcstoul(pwszDbg+nPrefixLen, &pszEnd, 10);
						if (nConHostPID && !this->pDebugTreeProcesses->Get(nConHostPID, nullptr))
						{
							AttachConHost(nConHostPID);
						}
					}
				}
				else
				{
					if (!ReadProcessMemory(hProcess, evt.u.DebugString.lpDebugStringData, szDbgText, evt.u.DebugString.nDebugStringLength, &nRead))
					{
						_wcscpy_c(pwszDbg, iReadMax, L"???");
					}
					else
					{
						szDbgText[std::min<size_t>(iReadMax, nRead + 1)] = 0;
						if ((memcmp(szDbgText, "\xEF\xBB\xBF", 3) != 0)
							|| !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, szDbgText + 3, -1, pwszDbg, LODWORD(iReadMax + 1)))
							MultiByteToWideChar(CP_ACP, 0, szDbgText, -1, pwszDbg, LODWORD(iReadMax + 1));
					}
				}

				int iWrite = lstrlen(pwszDbg);
				if ((iWrite > 0) && (pwszDbg[iWrite-1] != L'\n'))
				{
					pwszDbg[iWrite - 1] = L'\n'; pwszDbg[iWrite] = 0;
				}

				PrintBuffer(wszDbgText);

				dwContinueStatus = DBG_CONTINUE;

				break;
			} // OUTPUT_DEBUG_STRING_EVENT
		} //switch (evt.dwDebugEventCode)

		// Продолжить отлаживаемый процесс
		ContinueDebugEvent(evt.dwProcessId, evt.dwThreadId, dwContinueStatus);
	}

	//if (hCOMDLG32)
	//	FreeLibrary(hCOMDLG32);
}

void DebuggerInfo::GenerateMiniDumpFromCtrlBreak()
{
	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	DWORD dwErr = 0;
	const MModule hKernel(GetModuleHandle(L"kernel32.dll"));
	typedef BOOL (WINAPI* DebugBreakProcess_t)(HANDLE process);
	DebugBreakProcess_t debugBreakProcess = nullptr;
	if (hKernel.GetProcAddress("DebugBreakProcess", debugBreakProcess))
	{
		PrintBuffer(CE_CONEMUC_NAME_A ": Sending DebugBreak event to process\n");
		this->bDebuggerRequestDump = TRUE;
		_ASSERTE(gpWorker->RootProcessHandle()!=nullptr);
		if (!debugBreakProcess(gpWorker->RootProcessHandle()))
		{
			dwErr = GetLastError();
			//_ASSERTE(FALSE && dwErr==0);
			Printf(CE_CONEMUC_NAME_A ": Sending DebugBreak event failed, Code=x%X, WriteMiniDump on the fly\n", dwErr);
			this->bDebuggerRequestDump = FALSE;
			WriteMiniDump(gpWorker->RootProcessId(), gpWorker->RootThreadId(), nullptr);
		}
	}
	else
	{
		PrintBuffer(CE_CONEMUC_NAME_A ": DebugBreakProcess not found in kernel32.dll\n");
	}
}

void DebuggerInfo::GenerateTreeDebugBreak(DWORD nExcludePID)
{
	_ASSERTE(this->bDebugProcessTree);

	// ReSharper disable once CppInitializedValueIsAlwaysRewritten
	DWORD dwErr = 0;
	const MModule hKernel(GetModuleHandle(L"kernel32.dll"));
	typedef BOOL (WINAPI* DebugBreakProcess_t)(HANDLE process);
	DebugBreakProcess_t debugBreakProcess = nullptr;
	if (hKernel.GetProcAddress("DebugBreakProcess", debugBreakProcess))
	{
		PrintBuffer(CE_CONEMUC_NAME_A ": Sending DebugBreak event to processes:");

		DWORD pid = 0;

		MMap<DWORD,CEDebugProcessInfo>* pDebugTreeProcesses = this->pDebugTreeProcesses;
		CEDebugProcessInfo pi = {};

		if (pDebugTreeProcesses->GetNext(nullptr, &pid, &pi))
		{
			while (pid)
			{
				if (pid != nExcludePID)
				{
					Printf(" %u", pid);

					if (!pi.hProcess)
					{
						pi.hProcess = GetProcessHandleForDebug(pid);
					}

					if (debugBreakProcess(pi.hProcess))
					{
						this->nWaitTreeBreaks++;
					}
					else
					{
						dwErr = GetLastError();
						Printf("\n" CE_CONEMUC_NAME_A ": Sending DebugBreak event failed, Code=x%X\n", dwErr);
					}
				}

				if (!pDebugTreeProcesses->GetNext(&pid, &pid, &pi))
					break;
			}
			PrintBuffer("\n");
		}
	}
	else
	{
		PrintBuffer(CE_CONEMUC_NAME_A ": DebugBreakProcess not found in kernel32.dll\n");
	}
}
