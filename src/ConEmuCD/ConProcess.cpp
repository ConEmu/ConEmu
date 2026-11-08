
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

#define SHOWDEBUGSTR
#define DEBUGSTRPROC(x) DEBUGSTR(x)

#include "ConsoleMain.h"
#include "ConEmuSrv.h"
#include "ConProcess.h"
#include "../common/MProcess.h"
#include "../common/MSection.h"
#include "../common/ProcessData.h"
#include "../common/WUser.h"
#include <tlhelp32.h>


#include "ConsoleArgs.h"
#include "ConsoleState.h"

#define XTERM_PID_TIMEOUT 2500

#define MAX_GET_PROC_COUNT 128

#ifdef _DEBUG
#define WINE_PRINT_PROC_INFO
#define DUMP_PROC_INFO(s,n,p) //DumpProcInfo(s,n,p)
#define DBG_XTERM(x) DebugXtermOutput(x)
#else
#undef WINE_PRINT_PROC_INFO
#define DUMP_PROC_INFO(s,n,p)
#define DBG_XTERM(x)
#endif


ConProcess::ConProcess(const MModule& kernel32)
{
	if (kernel32.GetProcAddress("GetConsoleProcessList", pfnGetConsoleProcessList))
	{
		// Don't try to retrieve processes for service modes (e.g. /SetHook) started detached from conhost
		if (gState.runMode_ == RunMode::Server || gState.runMode_ == RunMode::AltServer)
		{
			SetLastError(0);
			startProcessCount_ = pfnGetConsoleProcessList(startProcessIds_, countof(startProcessIds_));
			// Wine bug validation
			if (!startProcessCount_)
			{
				const DWORD nErr = GetLastError();
				_ASSERTE(startProcessCount_ || gState.isWine_);
				wchar_t szDbgMsg[512], szFile[MAX_PATH] = {};
				GetModuleFileName(nullptr, szFile, countof(szFile));
				msprintf(szDbgMsg, countof(szDbgMsg), L"%s: PID=%u: GetConsoleProcessList failed, code=%u\r\n", PointToName(szFile), gnSelfPID, nErr);
				PrintBuffer(szDbgMsg);
				pfnGetConsoleProcessList = nullptr;
			}
		}
	}

	csProc = new MSection();

	if (pnProcesses.resize(START_MAX_PROCESSES)
		&& pnProcessesGet.resize(START_MAX_PROCESSES)
		&& pnProcessesCopy.resize(START_MAX_PROCESSES))
		nMaxProcesses = START_MAX_PROCESSES;

	// Let use pid==0 for SetConsoleMode requests
	xRequests.push_back(XTermRequest{});
}

ConProcess::~ConProcess()  // NOLINT(modernize-use-equals-default)
{
}

void ConProcess::ProcessCountChanged(BOOL abChanged, UINT anPrevCount, MSectionLock& CS)
{
	int nExitPlaceAdd = 2; // 2,3,4,5,6,7,8,9 +(nExitPlaceStep)
	bool bPrevCount2 = (anPrevCount>1);

	wchar_t szCountInfo[256];
	if (abChanged && RELEASEDEBUGTEST((gpLogSize!=NULL),true))
	{
		DWORD nPID, nLastPID = 0, nFoundPID = 0;
		swprintf_c(szCountInfo, L"Process list was changed: %u -> %i", anPrevCount, nProcessCount);

		wcscat_c(szCountInfo, L"\r\n                        Processes:");
		INT_PTR iLen = lstrlen(szCountInfo);
		wchar_t *psz = (szCountInfo + iLen), *pszEnd = szCountInfo + (countof(szCountInfo) - iLen - 64);
		for (size_t i = 0; (i < nProcessCount) && (psz < pszEnd); i++)
		{
			nPID = pnProcesses[i];
			if (!nPID) continue;
			swprintf_c(psz, 12/*#SECURELEN*/, L" %u", nPID);
			psz += lstrlen(psz);
			nLastPID = nPID;
			if (nPID == nLastFoundPID)
				nFoundPID = nPID;
		}

		if (nFoundPID || nLastPID)
		{
			nPID = nFoundPID ? nFoundPID : nLastPID;
			msprintf(psz, (psz - szCountInfo), L"\r\n                        ActivePID: %u, Name: ", nPID);
			psz += lstrlen(psz);
			CProcessData procName;
			procName.GetProcessName(nPID, psz, LODWORD(psz - szCountInfo), nullptr, 0, nullptr);
		}

		LogString(szCountInfo);
	}


	// Use section, if was not locked before
	CS.Lock(csProc);

#ifdef USE_COMMIT_EVENT
	// Если кто-то регистрировался как "ExtendedConsole.dll"
	// то проверить, а не свалился ли процесс его вызвавший
	if (gpSrv && nExtConsolePID)
	{
		DWORD nExtPID = nExtConsolePID;
		bool bExist = false;
		for (UINT i = 0; i < nProcessCount; i++)
		{
			if (pnProcesses[i] == nExtPID)
			{
				bExist = true; break;
			}
		}
		if (!bExist)
		{
			if (hExtConsoleCommit)
			{
				CloseHandle(hExtConsoleCommit);
				hExtConsoleCommit = NULL;
			}
			nExtConsolePID = 0;
		}
	}
#endif

	// NTVDM block
#ifndef WIN64
	// Найти "ntvdm.exe"
	if (abChanged && !nNtvdmPID && !IsWindows64())
	{
		//BOOL lbFarExists = FALSE, lbTelnetExist = FALSE;

		if (nProcessCount > 1)
		{
			//TODO: Reuse MToolHelp.h
			HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);

			if (hSnap != INVALID_HANDLE_VALUE)
			{
				PROCESSENTRY32 prc = {sizeof(PROCESSENTRY32)};

				if (Process32First(hSnap, &prc))
				{
					do
					{
						for (UINT i = 0; i < nProcessCount; i++)
						{
							if (prc.th32ProcessID != gnSelfPID
							        && prc.th32ProcessID == pnProcesses[i])
							{
								//if (IsFarExe(prc.szExeFile))
								//{
								//	lbFarExists = TRUE;
								//	//if (nProcessCount <= 2) // нужно проверить и ntvdm
								//	//	break; // возможно, в консоли еще есть и telnet?
								//}

								//#ifndef WIN64
								//else
								if (!nNtvdmPID && lstrcmpiW(prc.szExeFile, L"ntvdm.exe")==0)
								{
									nNtvdmPID = prc.th32ProcessID;
									break;
								}
								//#endif

								//// 23.04.2010 Maks - telnet нужно определять, т.к. у него проблемы с Ins и курсором
								//else if (lstrcmpiW(prc.szExeFile, L"telnet.exe")==0)
								//{
								//	lbTelnetExist = TRUE;
								//}
							}
						}

						//if (lbFarExists && lbTelnetExist
						//	#ifndef WIN64
						//        && nNtvdmPID
						//	#endif
						//    )
						//{
						//	break; // чтобы условие выхода внятнее было
						//}
					}
					while (Process32Next(hSnap, &prc));
				}

				CloseHandle(hSnap);
			}
		}

		//bTelnetActive = lbTelnetExist;
	}
#endif

	dwProcessLastCheckTick = GetTickCount();

	// Если корневой процесс проработал достаточно (10 сек), значит он живой и gState.alwaysConfirmExit_ можно сбросить
	// Если gState.autoDisableConfirmExit_==FALSE - сброс подтверждение закрытия консоли не выполняется
	if (gState.alwaysConfirmExit_  // если уже не сброшен
	        && gState.autoDisableConfirmExit_ // требуется ли вообще такая проверка (10 сек)
	        && anPrevCount > 1 // если в консоли был зафиксирован запущенный процесс
	        && gpWorker->RootProcessHandle()) // и корневой процесс был вообще запущен
	{
		if ((dwProcessLastCheckTick - nProcessStartTick) > CHECK_ROOT_OK_TIMEOUT)
		{
			_ASSERTE(gpConsoleArgs->confirmExitParm_ == RConStartArgs::eConfDefault);
			// эта проверка выполняется один раз
			gState.autoDisableConfirmExit_ = false;
			// 10 сек. прошло, теперь необходимо проверить, а жив ли процесс?
			DWORD dwProcWait = WaitForSingleObject(gpWorker->RootProcessHandle(), 0);

			if (dwProcWait == WAIT_OBJECT_0)
			{
				// Корневой процесс завершен, возможно, была какая-то проблема?
				gState.rootAliveLess10sec_ = TRUE; // корневой процесс проработал менее 10 сек
			}
			else
			{
				// Корневой процесс все еще работает, считаем что все ок и подтверждения закрытия консоли не потребуется
				gState.DisableAutoConfirmExit();
				//nProcessStartTick = GetTickCount() - 2*CHECK_ROOTSTART_TIMEOUT; // менять nProcessStartTick не нужно. проверка только по флажкам
			}
		}
	}

	if (gbRootWasFoundInCon == 0 && nProcessCount > 1 && gpWorker->RootProcessHandle() && gpWorker->RootProcessId())
	{
		if (WaitForSingleObject(gpWorker->RootProcessHandle(), 0) == WAIT_OBJECT_0)
		{
			gbRootWasFoundInCon = 2; // в консоль процесс не попал, и уже завершился
		}
		else
		{
			for (UINT n = 0; n < nProcessCount; n++)
			{
				if (gpWorker->RootProcessId() == pnProcesses[n])
				{
					// Процесс попал в консоль
					gbRootWasFoundInCon = 1; break;
				}
			}
		}
	}

	// e.g. if 'bash.exe' was killed in console
	CheckXRequests(CS);

	// Только для x86. На x64 ntvdm.exe не бывает.
	#ifndef WIN64
	WARNING("bNtvdmActive нигде не устанавливается");
	if (nProcessCount == 2 && !bNtvdmActive && nNtvdmPID)
	{
		// Возможно было запущено 16битное приложение, а ntvdm.exe не выгружается при его закрытии
		// gnSelfPID не обязательно будет в pnProcesses[0]
		if ((pnProcesses[0] == gnSelfPID && pnProcesses[1] == nNtvdmPID)
		        || (pnProcesses[1] == gnSelfPID && pnProcesses[0] == nNtvdmPID))
		{
			// Послать в нашу консоль команду закрытия
			PostMessage(gState.realConWnd_, WM_CLOSE, 0, 0);
		}
	}
	#endif

	WARNING("Если в консоли ДО этого были процессы - все условия вида 'nProcessCount == 1' обломаются");

	bool bForcedTo2 = false;
	DWORD nWaitDbg1 = -1, nWaitDbg2 = -1;
	// Похоже "пример" не соответствует условию, оставлю пока, для истории
	// -- Пример - запускаемся из фара. Количество процессов ИЗНАЧАЛЬНО - 5
	// -- cmd вываливается сразу (path not found)
	// -- количество процессов ОСТАЕТСЯ 5 и ни одно из ниже условий не проходит
	if (anPrevCount == 1 && nProcessCount == 1
		&& nProcessStartTick && dwProcessLastCheckTick
		&& ((dwProcessLastCheckTick - nProcessStartTick) > CHECK_ROOT_START_TIMEOUT)
		&& (nWaitDbg1 = WaitForSingleObject(ghExitQueryEvent,0)) == WAIT_TIMEOUT
		// выходить можно только если корневой процесс завершился
		&& gpWorker->RootProcessHandle() && ((nWaitDbg2 = WaitForSingleObject(gpWorker->RootProcessHandle(),0)) != WAIT_TIMEOUT))
	{
		anPrevCount = 2; // чтобы сработало следующее условие
		bForcedTo2 = true;
		//2010-03-06 - установка флажка должна быть при старте сервера
		//if (!gState.alwaysConfirmExit_) gState.alwaysConfirmExit_ = TRUE; // чтобы консоль не схлопнулась
	}

	if (anPrevCount > 1 && nProcessCount == 1)
	{
		if (pnProcesses[0] != gnSelfPID)
		{
			_ASSERTE(pnProcesses[0] == gnSelfPID);
		}
		else
		{
			#ifdef DEBUG_ISSUE_623
			_ASSERTE(FALSE && "Calling SetTerminateEvent");
			#endif

			// !!! Во время сильной загрузки процессора периодически
			// !!! случается, что ConEmu отваливается быстрее, чем в
			// !!! консоли появится фар. Обратить внимание на nPrevProcessedDbg[]
			// !!! в вызывающей функции. Откуда там появилось 2 процесса,
			// !!! и какого фига теперь стал только 1?
			_ASSERTE(gbTerminateOnCtrlBreak==FALSE);
			// !!! ****

			CS.Unlock();

			//2010-03-06 это не нужно, проверки делаются по другому
			//if (!gState.alwaysConfirmExit_ && (dwProcessLastCheckTick - nProcessStartTick) <= CHECK_ROOTSTART_TIMEOUT) {
			//	gState.alwaysConfirmExit_ = TRUE; // чтобы консоль не схлопнулась
			//}
			if (gState.alwaysConfirmExit_ && (dwProcessLastCheckTick - nProcessStartTick) <= CHECK_ROOT_START_TIMEOUT)
				gState.rootAliveLess10sec_ = TRUE; // корневой процесс проработал менее 10 сек

			if (bForcedTo2)
				nExitPlaceAdd = bPrevCount2 ? 3 : 4;
			else if (bPrevCount2)
				nExitPlaceAdd = 5;

			if (!nExitQueryPlace) nExitQueryPlace = nExitPlaceAdd/*2,3,4,5,6,7,8,9*/+(nExitPlaceStep);

			ShutdownSrvStep(L"All processes are terminated, SetEvent(ghExitQueryEvent)");
			SetTerminateEvent(ste_ProcessCountChanged);
		}
	}
	else if (abChanged && gpSrv->pConsole)
	{
		gpSrv->pConsole->bDataChanged = TRUE;
		SetEvent(gpSrv->hRefreshEvent);
	}

	UNREFERENCED_PARAMETER(nWaitDbg1); UNREFERENCED_PARAMETER(nWaitDbg2); UNREFERENCED_PARAMETER(bForcedTo2);
}

bool ConProcess::ProcessAdd(DWORD nPID, MSectionLock& CS)
{
	CS.Lock(csProc);

	UINT nPrevCount = nProcessCount;
	BOOL lbChanged = FALSE;
	_ASSERTE(nPID!=0);

	// Добавить процесс в список
	_ASSERTE(pnProcesses[0] == gnSelfPID);
	BOOL lbFound = FALSE;
	for (DWORD n = 0; n < nPrevCount; n++)
	{
		if (pnProcesses[n] == nPID)
		{
			lbFound = TRUE;
			break;
		}
	}
	if (!lbFound)
	{
		if (nPrevCount < nMaxProcesses)
		{
			CS.RelockExclusive(200);
			pnProcesses[nProcessCount++] = nPID;
			nLastFoundPID = nPID;
			lbChanged = TRUE;
		}
		else
		{
			_ASSERTE(nPrevCount < nMaxProcesses);
		}
	}

	return lbChanged;
}

// We receive this via CECMD_FARDETACHED or sst_AltServerStop/sst_ComspecStop/sst_AppStop
// So, the process leaves the console indeed
bool ConProcess::ProcessRemove(DWORD nPID, UINT nPrevCount, MSectionLock& CS)
{
	bool lbChanged = false;
	_ASSERTE(nPID != 0);

	CS.Lock(csProc);

	// Remove it from our list
	_ASSERTE(pnProcesses[0] == gnSelfPID);
	DWORD nChange = 0;
	for (DWORD n = 0; n < nPrevCount; n++)
	{
		if (pnProcesses[n] == nPID)
		{
			CS.RelockExclusive(200);
			lbChanged = true;
			if (nLastFoundPID == nPID)
				nLastFoundPID = 0;
			nProcessCount--;
			continue;
		}
		if (lbChanged)
		{
			pnProcesses[nChange] = pnProcesses[n];
		}
		nChange++;
	}

	for (INT_PTR i = 0; i < xRequests.size(); ++i)
	{
		if (xRequests[i].pid != nPID)
			continue;
		xRequests.erase(i);
		break;
	}

	return lbChanged;
}

// create=false used to erasing on reset
INT_PTR ConProcess::GetXRequestIndex(DWORD pid, bool create)
{
	// csProc must be locked exclusively!

	for (INT_PTR i = 0; i < xRequests.size(); ++i)
	{
		if (xRequests[i].pid == pid)
		{
			// pid==0 is used to
			if (create && pid)
			{
				// Move it to the end of the queue
				while ((i+1) < xRequests.size())
				{
					std::swap(xRequests[i], xRequests[i+1]);
					++i;
				}
				// Update the last alive tick of the process
				if (xRequests[i].tick)
				{
					DWORD t = GetTickCount();
					if (!t) t = 1;
					xRequests[i].tick = t;
				}
			}
			return i;
		}
	}

	if (create)
	{
		XTermRequest x{pid, GetTickCount()};
		if (!x.tick) x.tick++;
		if (pid)
		{
			xRequests.push_back(x);
		}
		else
		{
			_ASSERTE(FALSE && "pid==0 must be created explicitly");
			xRequests.insert(0, x);
		}
		return (xRequests.size() - 1);
	}

	return -1;
}

void ConProcess::RefreshXRequests(MSectionLock& CS)
{
	// Use section, if was not locked before
	CS.Lock(csProc);

	for (int m = 0; m < tmc_Last; ++m)
	{
		// ***console*** life-time
		if (m == tmc_CursorShape)
			continue;

		DWORD value = 0, pid = 0;
		for (INT_PTR i = xRequests.size() - 1; !value && i >= 0; --i)
		{
			const XTermRequest& x = xRequests[i];
			if (x.modes[m])
			{
				value = x.modes[m];
				pid = x.pid;
			}
		}

		if (xFixedRequests[m] != value)
		{
			// Update
			xFixedRequests[m] = value;
			// and inform the GUI
			CESERVER_REQ* in = ExecuteNewCmd(CECMD_STARTXTERM, sizeof(CESERVER_REQ_HDR) + sizeof(DWORD) * 3);
			if (in)
			{
				#ifdef _DEBUG
				wchar_t buffer[80];
				DBG_XTERM(msprintf(buffer, countof(buffer), L"XRequests chage: command=%u value=%u pid=%u", m, value, pid));
				#endif
				in->dwData[0] = static_cast<TermModeCommand>(m);
				in->dwData[1] = value;  // NOLINT(clang-diagnostic-array-bounds)
				in->dwData[2] = pid;  // NOLINT(clang-diagnostic-array-bounds)
				CESERVER_REQ* pGuiOut = ExecuteGuiCmd(gState.realConWnd_, in, gState.realConWnd_);
				ExecuteFreeResult(pGuiOut);
			}
		}
	}
}

void ConProcess::OnAttached()
{
	for (int m = 0; m < tmc_Last; ++m)
	{
		// Only if some mode was turned ON
		if (!xFixedRequests[m])
			continue;
		// Inform the GUI
		CESERVER_REQ* in = ExecuteNewCmd(CECMD_STARTXTERM, sizeof(CESERVER_REQ_HDR)+sizeof(DWORD)*3);
		if (in)
		{
			in->dwData[0] = (TermModeCommand)m;
			in->dwData[1] = xFixedRequests[m];
			in->dwData[2] = gnSelfPID; // doesn't matter
			CESERVER_REQ* pGuiOut = ExecuteGuiCmd(gState.realConWnd_, in, gState.realConWnd_);
			ExecuteFreeResult(pGuiOut);
		}
	}
}

void ConProcess::CheckXRequests(MSectionLock& CS)
{
	bool bWasSet = false;
	for (int m = 0; m < tmc_Last; ++m)
	{
		if (xFixedRequests[m])
		{
			bWasSet = true; break;
		}
	}
	// If modes were not requested, there is nothing to clear
	if (!bWasSet)
	{
		return;
	}

	if (!CS.isLocked())
		CS.Lock(csProc, true);
	else
		CS.RelockExclusive();

	bool bChanged = false;
	// Go down to zero to simplify erase procedure
	for (INT_PTR i = xRequests.size() - 1; i >= 0; --i)
	{
		XTermRequest& x = xRequests[i];
		if (!x.pid)
			continue;
		bool alive = false;
		for (UINT n = 0; n < nProcessCount; ++n)
		{
			if (pnProcesses[n] == x.pid)
			{
				alive = true;
				break;
			}
		}
		if (!alive)
		{
			// Did process appeared in console?
			if (x.tick)
			{
				if (x.pid == gpWorker->RootProcessId() && gbRootWasFoundInCon)
					x.tick = 0;
				else
				{
					DWORD nCurTick = GetTickCount();
					if ((nCurTick - x.tick) > XTERM_PID_TIMEOUT)
						x.tick = 0;
				}
			}
			// Was terminated
			if (!x.tick)
			{
				bChanged = true;
				xRequests.erase(i);
			}
		}
	}

	if (bChanged)
	{
		RefreshXRequests(CS);
	}
}

void ConProcess::StartStopXTermMode(const TermModeCommand cmd, const DWORD value, const DWORD pid)
{
	if ((unsigned)cmd >= tmc_Last)
	{
		_ASSERTE((unsigned)cmd < tmc_Last);
		return;
	}

	// some modes has console life time
	if (cmd == tmc_CursorShape)
	{
		xFixedRequests[cmd] = value;
		return;
	}

	// If some process requests the mode, force check process list first
	// Due to race, the pid *may* not be yet retrieved from conhost
	// by CheckProcessCount, but we try to do our best )
	if (value && pid)
	{
		CheckProcessCount(true);
	}

	// others are linked to the calling process life
	MSectionLock CS; CS.Lock(csProc, true);
	INT_PTR idx = GetXRequestIndex(pid, (value != 0));
	if (idx >= 0)
	{
		xRequests[idx].modes[cmd] = value;
		// Modified, update xFixedRequests
		if (value)
		{
			xFixedRequests[cmd] = value;
		}
		else
		{
			DWORD new_value = 0;
			for (INT_PTR i = xRequests.size() - 1; i >= 0; --i)
			{
				const XTermRequest& x = xRequests[i];
				if (x.modes[cmd])
				{
					new_value = x.modes[cmd];
					break;
				}
			}
			xFixedRequests[cmd] = new_value;
		}
	}

	if (value)
	{
		ProcessAdd(pid, CS);
	}
}

void ConProcess::DumpProcInfo(LPCWSTR sLabel, DWORD nCount, DWORD* pPID) const
{
#ifdef WINE_PRINT_PROC_INFO
	const DWORD nErr = GetLastError();
	wchar_t szDbgMsg[255];
	msprintf(szDbgMsg, countof(szDbgMsg), L"%s: Err=%u, Count=%u:", sLabel ? sLabel : L"", nErr, nCount);
	nCount = std::min<DWORD>(nCount,10);
	for (DWORD i = 0; pPID && (i < nCount); i++)
	{
		const int nLen = lstrlen(szDbgMsg);
		msprintf(szDbgMsg+nLen, countof(szDbgMsg)-nLen, L" %u", pPID[i]);
	}
	wcscat_c(szDbgMsg, L"\r\n");
	PrintBuffer(szDbgMsg);
#endif
}

bool ConProcess::CheckProcessCount(const bool abForce/*=FALSE*/)
{
	if (!(abForce || nProcessCount <= 0))
	{
		const DWORD dwCurTick = GetTickCount();

		if ((dwCurTick - dwProcessLastCheckTick) < static_cast<DWORD>(CHECK_PROCESSES_TIMEOUT))
			return FALSE;
	}

	BOOL lbChanged = FALSE;
	MSectionLock CS; CS.Lock(csProc);

	const UINT nPrevCount = nProcessCount;
#ifdef _DEBUG
	// for debugging, get current console state
	DWORD nCurProcessesDbg[MAX_GET_PROC_COUNT] = {};
	// for debugging, save previous console state
	DWORD nPrevProcessedDbg[MAX_GET_PROC_COUNT] = {};
	//
	if (!pnProcesses.empty() && nProcessCount)
	{
		memmove_s(nPrevProcessedDbg, sizeof(nPrevProcessedDbg), &pnProcesses[0],
			std::min<DWORD>(countof(nPrevProcessedDbg), nProcessCount) * sizeof(pnProcesses[0]));
	}
#endif

	if (nProcessCount == 0)
	{
		pnProcesses[0] = gnSelfPID;
		nProcessCount = 1;
	}

	if (gpWorker->IsDebuggerActive())
	{
		//if (gpWorker->RootProcessHandle()) {
		//	if (WaitForSingleObject(gpWorker->RootProcessHandle(), 0) == WAIT_OBJECT_0) {
		//		nProcessCount = 1;
		//		return TRUE;
		//	}
		//}
		//pnProcesses[1] = gpSrv->dwRootProcess;
		//nProcessCount = 2;
		return FALSE;
	}

	#ifdef _DEBUG
	bool bDumpProcInfo = gState.isWine_;
	#endif

	bool bProcFound = false;
	bool bConsoleOnly = (gpWorker->RootProcessGui() == NULL);
	bool bMayBeConsolePaf = gpSrv->Portable.nPID && (gpSrv->Portable.nSubsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);

	if (pfnGetConsoleProcessList && (bConsoleOnly || bMayBeConsolePaf))
	{
		WARNING("Переделать, как-то слишком сложно получается");
		DWORD nCurCount;
		nCurCount = pfnGetConsoleProcessList(&pnProcessesGet[0], nMaxProcesses);

		#ifdef _DEBUG
		SetLastError(0);
		int nCurCountDbg = pfnGetConsoleProcessList(nCurProcessesDbg, countof(nCurProcessesDbg));
		DUMP_PROC_INFO(L"WinXP mode", nCurCountDbg, nCurProcessesDbg);
		#endif

		lbChanged = (nProcessCount != nCurCount);

		if (nCurCount && (nLastFoundPID != pnProcessesGet[0]))
			nLastFoundPID = pnProcessesGet[0];
		else if (!nCurCount)
			nLastFoundPID = 0;

		bProcFound = bConsoleOnly && (nCurCount > 0);

		if (nCurCount == 0)
		{
			_ASSERTE(gbTerminateOnCtrlBreak==FALSE);

			// Похоже что сюда мы попадаем также при ошибке 0xC0000142 запуска root-process
			#ifdef _DEBUG
			_ASSERTE(FALSE && "(nCurCount==0)?");
			// Some diagnostics for debugger
			DWORD nErr;
			HWND hConWnd = GetConsoleWindow(); nErr = GetLastError();
			HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE); nErr = GetLastError();
			CONSOLE_SCREEN_BUFFER_INFO sbi = {}; BOOL bSbi = GetConsoleScreenBufferInfo(hOut, &sbi); nErr = GetLastError();
			DWORD nWait = WaitForSingleObject(gpWorker->RootProcessHandle(), 0);
			GetExitCodeProcess(gpWorker->RootProcessHandle(), &nErr);
			#endif

			// Это значит в Win7 свалился conhost.exe
			//if ((gnOsVer >= 0x601) && !gState.isWine_)
			{
				#ifdef _DEBUG
				DWORD dwErr = GetLastError();
				#endif
				nProcessCount = 1;
				SetEvent(ghQuitEvent);

				if (!nExitQueryPlace) nExitQueryPlace = 1+(nExitPlaceStep);

				SetTerminateEvent(ste_CheckProcessCount);
				return TRUE;
			}
		}
		else
		{
			if (nCurCount > nMaxProcesses)
			{
				DWORD nSize = nCurCount + 100;
				MArray<DWORD> pnPID;
				if (pnPID.resize(nSize))
				{
					CS.RelockExclusive(200);
					nCurCount = pfnGetConsoleProcessList(&pnPID[0], nSize);

					if (nCurCount > 0 && nCurCount <= nSize)
					{
						if (!pnProcesses.resize(nSize) || !pnProcessesCopy.resize(nSize))
						{
							return false;
						}
						pnProcessesGet.swap(pnPID);
						_ASSERTE(nExitQueryPlace == 0 || nCurCount == 1);
						nProcessCount = nCurCount;
						nMaxProcesses = nSize;
					}
				}
			}

			// PID-ы процессов в консоли могут оказаться перемешаны. Нас же интересует gnSelfPID на первом месте
			pnProcesses[0] = gnSelfPID;
			DWORD nCorrect = 1, nMax = nMaxProcesses, nDosBox = 0;
			if (gState.dosbox_.use_)
			{
				if (gState.dosbox_.handle_ && WaitForSingleObject(gState.dosbox_.handle_, 0) == WAIT_TIMEOUT)
				{
					pnProcesses[nCorrect++] = gState.dosbox_.pid_;
					nDosBox = 1;
				}
				else if (gState.dosbox_.handle_)
				{
					gState.dosbox_.handle_ = NULL;
				}
			}
			for (DWORD n = 0; n < nCurCount && nCorrect < nMax; n++)
			{
				if (pnProcessesGet[n] != gnSelfPID)
				{
					if (pnProcesses[nCorrect] != pnProcessesGet[n])
					{
						pnProcesses[nCorrect] = pnProcessesGet[n];
						lbChanged = TRUE;
					}
					nCorrect++;
				}
			}
			nCurCount += nDosBox;


			if (nCurCount < nMaxProcesses)
			{
				// Сбросить в 0 ячейки со старыми процессами
				_ASSERTE(nProcessCount < nMaxProcesses);

				if (nCurCount < nProcessCount)
				{
					size_t nSize = sizeof(DWORD) * (nProcessCount - nCurCount);
					memset(&pnProcesses[nCurCount], 0, nSize);
				}

				_ASSERTE(nCurCount>0);
				_ASSERTE(nExitQueryPlace == 0 || nCurCount == 1);
				nProcessCount = nCurCount;
			}

			UINT nSize = sizeof(DWORD) * std::min<DWORD>(nMaxProcesses, START_MAX_PROCESSES);
			#ifdef _DEBUG
			_ASSERTE(!IsBadWritePtr(&pnProcessesCopy[0],nSize));
			_ASSERTE(!IsBadWritePtr(&pnProcesses[0],nSize));
			#endif

			if (!lbChanged)
			{
				lbChanged = memcmp(&pnProcessesCopy[0], &pnProcesses[0], nSize) != 0;
				MCHKHEAP;
			}

			if (lbChanged)
			{
				memmove(&pnProcessesCopy[0], &pnProcesses[0], nSize);

				MCHKHEAP;
			}
		}

		if (!bProcFound && bMayBeConsolePaf && (nCurCount > 0) && pnProcesses.size())
		{
			for (DWORD i = 0; i < nCurCount; i++)
			{
				if (pnProcesses[i] == gpSrv->Portable.nPID)
				{
					// В консоли обнаружен процесс запущенный из ChildGui (e.g. CommandPromptPortable.exe -> cmd.exe)
                    bProcFound = true;
				}
			}
		}
	}

	// Wine related: GetConsoleProcessList may fail
	// or ChildGui related: GUI app started in tab and (optionally) it starts another app (console or gui)
	if (!bProcFound)
	{
		_ASSERTE(pnProcesses[0] == gnSelfPID);
		pnProcesses[0] = gnSelfPID;

		HANDLE hRootProcess = gpWorker->RootProcessHandle();
		DWORD  nRootPID = gpWorker->RootProcessId();

		if (hRootProcess || gpSrv->Portable.hProcess)
		{
			DWORD nRootWait = hRootProcess ? WaitForSingleObject(hRootProcess, 0) : WAIT_OBJECT_0;
			// PortableApps?
			if (nRootWait == WAIT_OBJECT_0)
			{
				if (gpSrv->Portable.hProcess)
				{
					nRootWait = WaitForSingleObject(gpSrv->Portable.hProcess, 0);
					if (nRootWait == WAIT_TIMEOUT)
						nRootPID = gpSrv->Portable.nPID;
				}
			}
			// Ok, Check it
			if (nRootWait == WAIT_OBJECT_0)
			{
				pnProcesses[1] = 0;
				lbChanged = nProcessCount != 1;
				nProcessCount = 1;
			}
			else
			{
				pnProcesses[1] = nRootPID;
				lbChanged = nProcessCount != 2;
				_ASSERTE(nExitQueryPlace == 0);
				nProcessCount = 2;
			}
		}
		else if (gpWorker->RootProcessGui())
		{
			if (!IsWindow(gpWorker->RootProcessGui()))
			{
				// Process handle must be opened!
				_ASSERTE(gpWorker->RootProcessHandle() != NULL);
				// Fin
				pnProcesses[1] = 0;
				lbChanged = nProcessCount != 1;
				nProcessCount = 1;
			}
		}

		DUMP_PROC_INFO(L"Win2k mode", nProcessCount, pnProcesses);
	}

	dwProcessLastCheckTick = GetTickCount();

	ProcessCountChanged(lbChanged, nPrevCount, CS);


	return lbChanged;
}

bool ConProcess::GetRootInfo(CESERVER_REQ* pReq)
{
	if (!pReq)
		return false;
	if (!gpSrv || !gpWorker->RootProcessId() || !gpWorker->RootProcessHandle())
		return false;

	DWORD nWait = WaitForSingleObject(gpWorker->RootProcessHandle(), 0);
	pReq->RootInfo.bRunning = (nWait == WAIT_TIMEOUT);
	pReq->RootInfo.nPID = gpWorker->RootProcessId();
	pReq->RootInfo.nExitCode = (nWait == WAIT_TIMEOUT) ? STILL_ACTIVE : 999;
	GetExitCodeProcess(gpWorker->RootProcessHandle(), &pReq->RootInfo.nExitCode);
	if (nWait == WAIT_TIMEOUT)
	{
		pReq->RootInfo.nUpTime = (GetTickCount() - gpWorker->RootProcessStartTime());
	}
	else
	{
		FILETIME ftCreation = {}, ftExit = {}, ftKernel = {}, ftUser = {};
		GetProcessTimes(gpWorker->RootProcessHandle(), &ftCreation, &ftExit, &ftKernel, &ftUser);
		__int64 upTime = ((*(uint64_t*)&ftExit) - (*(uint64_t*)&ftCreation)) / 10000LL;
		pReq->RootInfo.nUpTime = (LODWORD(upTime) == upTime) ? LODWORD(upTime) : (DWORD)-1;
	}

	return true;
}

// returns true if process list was changed since last query
bool ConProcess::GetProcesses(DWORD* processes, UINT count, DWORD dwMainServerPid)
{
	bool lbChanged = false;
	_ASSERTE(count*sizeof(*processes) == sizeof(nLastRetProcesses));
	// #CONPROCESS Use current process list!
	const DWORD nCurProcCount = GetProcessCount(processes, count, dwMainServerPid);
	_ASSERTE(nCurProcCount && processes[0]);
	const size_t cmpSize = std::min<UINT>(sizeof(nLastRetProcesses), (count * sizeof(*processes)));
	if (nCurProcCount
		&& (memcmp(nLastRetProcesses, processes, cmpSize) != 0))
	{
		// Process list was changed
		lbChanged = true;
		// remember it
		memmove(nLastRetProcesses, gpSrv->pConsole->ConState.nProcesses, cmpSize);
	}
	return lbChanged;
}

// When DefTerm debug console is started for Win32 app
// we need to allocate hidden console, and there is no
// active process, until parent DevEnv successfully starts
// new debugging process session
DWORD ConProcess::WaitForRootConsoleProcess(DWORD nTimeout) const
{
	if (pfnGetConsoleProcessList == nullptr)
	{
		_ASSERTE(FALSE && "Attach to console app was requested, but required WinXP or higher!");
		return 0;
	}

	_ASSERTE(gpConsoleArgs->creatingHiddenConsole_);
	_ASSERTE(gState.realConWnd_!=NULL);

	const DWORD nStart = GetTickCount();
	DWORD nFoundPID = 0;
	DWORD nDelta = 0;
	DWORD nProcesses[20] = {};
	// ReSharper disable once CppJoinDeclarationAndAssignment
	DWORD nProcCount;
	// ReSharper disable once CppJoinDeclarationAndAssignment
	DWORD i;

	PROCESSENTRY32 pi = {};
	GetProcessInfo(gnSelfPID, pi);

	while (!nFoundPID && (nDelta < nTimeout))
	{
		Sleep(50);
		// ReSharper disable once CppJoinDeclarationAndAssignment
		nProcCount = pfnGetConsoleProcessList(nProcesses, countof(nProcesses));

		for (i = 0; i < nProcCount; i++)
		{
			const DWORD nPID = nProcesses[i];
			if (nPID && (nPID != gnSelfPID) && (nPID != pi.th32ParentProcessID))
			{
				nFoundPID = nPID;
				break;
			}
		}

		nDelta = (GetTickCount() - nStart);
	}

	if (!nFoundPID)
	{
		apiShowWindow(gState.realConWnd_, SW_SHOWNORMAL);
		_ASSERTE(FALSE && "Was unable to find starting process");
	}

	return nFoundPID;
}

bool ConProcess::IsConsoleProcessCountSupported() const
{
	return pfnGetConsoleProcessList != nullptr;
}

MArray<DWORD> ConProcess::GetSpawnedProcesses() const
{
	MArray<DWORD> result;
	for (const DWORD pid : GetAllProcesses())
	{
		if (!pid || pid == gpWorker->RootProcessId() || pid == gnSelfPID
#ifndef WIN64
			|| pid == nNtvdmPID
#endif
			) continue;
		result.push_back(pid);
	}
	return result;
}

MArray<DWORD> ConProcess::GetAllProcesses() const
{
	if (!pfnGetConsoleProcessList) return {};
	MArray<DWORD> result;
	DWORD capacity = MAX_GET_PROC_COUNT;
	for (unsigned retry = 0; retry < 4; ++retry)
	{
		if (capacity > 65536 || !result.resize(capacity)) return {};
		const DWORD count = pfnGetConsoleProcessList(&result[0], capacity);
		if (!count) return {};
		if (count <= capacity)
		{
			result.resize(count);
			return result;
		}
		// The API returns required size, not number of initialized entries,
		// when the supplied buffer is too small. Retry instead of reading past it.
		capacity = count;
	}
	return {};
}

int ConProcess::GetProcessCount(DWORD *rpdwPID, const UINT nMaxCount, const DWORD dwMainServerPid)
{
	if (!rpdwPID || (nMaxCount < 3))
	{
		_ASSERTE(rpdwPID && (nMaxCount >= 3));
		return this->nProcessCount;
	}


	UINT nRetCount = 0;
	const UINT absentPid = static_cast<UINT>(-1);

	rpdwPID[nRetCount++] = gnSelfPID;

	// Windows 7 and higher: there is "conhost.exe"
	if (IsWin7())
	{
		#if 0
		typedef BOOL (WINAPI* GetNamedPipeServerProcessId_t)(HANDLE Pipe,PULONG ServerProcessId);
		HMODULE hKernel = GetModuleHandle(L"kernel32.dll");
		GetNamedPipeServerProcessId_t GetNamedPipeServerProcessId_f = hKernel
			? (GetNamedPipeServerProcessId_t)GetProcAddress(hKernel, "GetNamedPipeServerProcessId") : nullptr;
		HANDLE hOut; BOOL bSrv = FALSE; ULONG nSrvPid = 0;
		_ASSERTE(FALSE && "calling GetNamedPipeServerProcessId_f");
		if (GetNamedPipeServerProcessId_f)
		{
			hOut = (HANDLE)ghConOut;
			if (hOut)
			{
				bSrv = GetNamedPipeServerProcessId_f(hOut, &nSrvPid);
			}
		}
		#endif

		if (!this->nConhostPID)
		{
			// Найти порожденный conhost.exe
			// #TODO: Reuse MToolHelp.h
			HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (h && (h != INVALID_HANDLE_VALUE))
			{
				// Учтем альтернативные серверы (Far/Telnet/...)
				const DWORD nSrvPid = dwMainServerPid ? dwMainServerPid : gnSelfPID;
				PROCESSENTRY32 processInfo = {};
				processInfo.dwSize = sizeof(processInfo);
				if (Process32First(h, &processInfo))
				{
					do {
						if ((processInfo.th32ParentProcessID == nSrvPid)
							&& (lstrcmpi(processInfo.szExeFile, L"conhost.exe") == 0))
						{
							this->nConhostPID = processInfo.th32ProcessID;
							break;
						}
					} while (Process32Next(h, &processInfo));
				}

				CloseHandle(h);
			}

			if (!this->nConhostPID)
				this->nConhostPID = absentPid;
		}

		// #PROCESSES At the moment we assume nConhostPID is alive because console WinAPI still works
		if (this->nConhostPID && (this->nConhostPID != absentPid))
		{
			rpdwPID[nRetCount++] = this->nConhostPID;
		}
	}


	MSectionLock cs;
	UINT nCurCount = 0;
	if (cs.Lock(this->csProc, TRUE/*abExclusive*/, 200))
	{
		nCurCount = this->nProcessCount;

		for (INT_PTR i1 = (nCurCount-1); (i1 >= 0) && (nRetCount < nMaxCount); i1--)
		{
			const DWORD pid = this->pnProcesses[i1];
			if (pid && pid != gnSelfPID)
			{
				rpdwPID[nRetCount++] = pid;
			}
		}
	}

	for (size_t i = nRetCount; i < nMaxCount; i++)
	{
		rpdwPID[i] = 0;
	}

	//if (nSize > nMaxCount)
	//{
	//	memset(rpdwPID, 0, sizeof(DWORD)*nMaxCount);
	//	rpdwPID[0] = gnSelfPID;

	//	for(int i1=0, i2=(nMaxCount-1); i1<(int)nSize && i2>0; i1++, i2--)
	//		rpdwPID[i2] = this->pnProcesses[i1]; //-V108

	//	nSize = nMaxCount;
	//}
	//else
	//{
	//	memmove(rpdwPID, this->pnProcesses, sizeof(DWORD)*nSize);

	//	for (UINT i=nSize; i<nMaxCount; i++)
	//		rpdwPID[i] = 0; //-V108
	//}

	_ASSERTE(rpdwPID[0]);
	return nRetCount;
}
