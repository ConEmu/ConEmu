
/*
Copyright (c) 2016-present Maximus5
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
#define SHOWDEBUGSTR

#ifdef _DEBUG
//#define SHOW_ATTACH_MSGBOX
//#define SHOW_OUTPUT_MSGBOX
//#define SHOW_INJECT_MSGBOX
#endif

#include "../common/Common.h"
#include "../common/CmdLine.h"
#include "../common/ConEmuCheck.h"
#include "../common/CEStr.h"
#include "../common/EnvVar.h"
#include "../common/MModule.h"
#include "../common/MProcess.h"
#include "../common/MStrDup.h"
#include "../common/ProcessData.h"
#include "../common/WFiles.h"
#include "../common/WConsole.h"
#include "../common/WUser.h"
#include "../common/MWnd.h"
#include "../ConEmuHk/Injects.h"
#include "../ConEmu/version.h"

#include "Actions.h"
#include "ConsoleArgs.h"
#include "ConsoleState.h"
#include "ConsoleMain.h"
#include "GuiMacro.h"
#include "InjectRemote.h"
#include "MapDump.h"
#include "UnicodeTest.h"
#include "ConPty.h"


// ConEmuC -OsVerInfo
int OsVerInfo()
{
	OSVERSIONINFOEX osv = {};
	osv.dwOSVersionInfoSize = sizeof(osv);
	GetOsVersionInformational(reinterpret_cast<OSVERSIONINFO*>(&osv));

	UINT DBCS = IsWinDBCS();
	UINT HWFS = IsHwFullScreenAvailable();
	UINT W5fam = IsWin5family();
	UINT WXPSP1 = IsWinXP(1);
	UINT W6 = IsWin6();
	UINT W7 = IsWin7();
	UINT W10 = IsWin10();
	UINT Wx64 = IsWindows64();
	UINT WINE = IsWine();
	UINT WPE = IsWinPE();
	UINT TELNET = isTerminalMode();

	wchar_t szInfo[200];
	swprintf_c(szInfo,
		L"OS version information\n"
		L"%u.%u build %u SP%u.%u suite=x%04X type=%u\n"
		L"W5fam=%u WXPSP1=%u W6=%u W7=%u W10=%u Wx64=%u\n"
		L"HWFS=%u DBCS=%u WINE=%u WPE=%u TELNET=%u\n",
		osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber, osv.wServicePackMajor, osv.wServicePackMinor, osv.wSuiteMask, osv.wProductType,
		W5fam, WXPSP1, W6, W7, W10, Wx64, HWFS,
		DBCS, WINE, WPE, TELNET);
	PrintBuffer(szInfo);

	return MAKEWORD(osv.dwMinorVersion, osv.dwMajorVersion);
}

void RegisterConsoleFontHKLM(LPCWSTR pszFontFace)
{
	if (!pszFontFace || !*pszFontFace)
		return;

	HKEY hk;
	DWORD nRights = KEY_ALL_ACCESS|WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0),0);

	if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
	                  0, nRights, &hk))
	{
		wchar_t szId[32] = {0}, szFont[255]; DWORD dwLen, dwType;

		for (DWORD i = 0; i <20; i++)
		{
			szId[i] = L'0'; szId[i+1] = 0; wmemset(szFont, 0, 255);

			if (RegQueryValueExW(hk, szId, nullptr, &dwType, (LPBYTE)szFont, &(dwLen = 255*2)))
			{
				RegSetValueExW(hk, szId, 0, REG_SZ, (LPBYTE)pszFontFace, (lstrlen(pszFontFace)+1)*2);
				break;
			}

			if (lstrcmpi(szFont, pszFontFace) == 0)
			{
				break; // он уже добавлен
			}
		}

		RegCloseKey(hk);
	}
}


bool DoStateCheck(ConEmuStateCheck eStateCheck)
{
	LogFunction(L"DoStateCheck");

	bool bOn = false;
	const char* checkName;

	switch (eStateCheck)
	{
	case ConEmuStateCheck::IsConEmu:
	case ConEmuStateCheck::IsAnsi:
		checkName = (eStateCheck == ConEmuStateCheck::IsConEmu) ? "IsConEmu" : "IsAnsi";
		if (gState.realConWnd_)
		{
			CESERVER_CONSOLE_MAPPING_HDR* pInfo = static_cast<CESERVER_CONSOLE_MAPPING_HDR*>(malloc(sizeof(*pInfo)));
			if (pInfo && LoadSrvMapping(gState.realConWnd_, *pInfo))
			{
				_ASSERTE(pInfo->ComSpec.ConEmuExeDir[0] && pInfo->ComSpec.ConEmuBaseDir[0]);

				const MWnd hWnd = static_cast<HWND>(pInfo->hConEmuWndDc);
				if (hWnd && IsWindow(hWnd))
				{
					if (eStateCheck == ConEmuStateCheck::IsConEmu)
					{
						bOn = true;
					}
					else if (eStateCheck == ConEmuStateCheck::IsAnsi)
					{
						bOn = ((pInfo->Flags & ConEmu::ConsoleFlags::ProcessAnsi) != 0);
					}
				}
			}
			SafeFree(pInfo);
		}
		break;
	case ConEmuStateCheck::IsAdmin:
		checkName = "IsAdmin";
		bOn = IsUserAdmin();
		break;
	case ConEmuStateCheck::IsRedirect:
		checkName = "IsRedirect";
		bOn = IsOutputRedirected();
		break;
	case ConEmuStateCheck::IsTerm:
		checkName = "IsTerm";
		bOn = isTerminalMode();
		break;
	case ConEmuStateCheck::None:
	default:
		_ASSERTE(FALSE && "Unsupported StateCheck code");
		checkName = "UnknownCheck";
	}

	return bOn;
}


int DoInjectHooks(const CEStr& asCmdArg)
{
	gbInShutdown = TRUE; // avoid confirmation on exit
	gState.runMode_ = RunMode::SetHook64;
	LPWSTR pszNext = asCmdArg.data();
	LPWSTR pszEnd = nullptr;
	BOOL lbForceGui = FALSE;
	PROCESS_INFORMATION pi = {};

	auto strToHandle = [](LPWSTR pszNext, LPWSTR* ppszEnd) -> HANDLE
	{
#ifdef _WIN64
		return HANDLE(wcstoull(pszNext, ppszEnd, 16));
#else
		return HANDLE(wcstoul(pszNext, ppszEnd, 16));
#endif
	};

	pi.hProcess = strToHandle(pszNext, &pszEnd);

	if (pi.hProcess && pszEnd && *pszEnd)
	{
		pszNext = pszEnd+1;
		pi.dwProcessId = wcstoul(pszNext, &pszEnd, 10);
	}

	if (pi.dwProcessId && pszEnd && *pszEnd)
	{
		pszNext = pszEnd+1;
		pi.hThread = strToHandle(pszNext, &pszEnd);
	}

	if (pi.hThread && pszEnd && *pszEnd)
	{
		pszNext = pszEnd+1;
		pi.dwThreadId = wcstoul(pszNext, &pszEnd, 10);
	}

	if (pi.dwThreadId && pszEnd && *pszEnd)
	{
		pszNext = pszEnd+1;
		lbForceGui = wcstoul(pszNext, &pszEnd, 10);
	}


	#ifdef SHOW_INJECT_MSGBOX
	{
		wchar_t szDbgMsg[512] = L"", szTitle[128] = L"";
		PROCESSENTRY32W procInfo{};
		GetProcessInfo(pi.dwProcessId, procInfo);
		swprintf_c(szTitle, L"ConEmuCD PID=%u", GetCurrentProcessId());
		swprintf_c(szDbgMsg, L"InjectsTo PID=%s {%s}\nConEmuCD PID=%u", asCmdArg.c_str(L""), procInfo.szExeFile, GetCurrentProcessId());
		if (MessageBoxW(nullptr, szDbgMsg, szTitle, MB_SYSTEMMODAL | MB_OKCANCEL) != IDOK)
		{
			return CERR_HOOKS_FAILED;
		}
	}
	#endif


	if (pi.hProcess && pi.hThread && pi.dwProcessId && pi.dwThreadId)
	{
		const CINJECTHK_EXIT_CODES iHookRc = InjectHooks(pi, 0, gbLogProcess, gsSelfPath, gState.realConWnd_);

		if (iHookRc == CIH_OK/*0*/)
		{
			return CERR_HOOKS_WAS_SET;
		}

		// It's better to show an error message
		const DWORD nErrCode = GetLastError();
		//_ASSERTE(iHookRc == 0); -- ассерт не нужен, есть MsgBox
		wchar_t szDbgMsg[255], szTitle[128];
		swprintf_c(szTitle, L"ConEmuC[%u], PID=%u", WIN3264TEST(32,64), GetCurrentProcessId());
		swprintf_c(szDbgMsg, L"ConEmuC.X, PID=%u\nInjecting hooks into PID=%u\nFAILED, code=%i:0x%08X", GetCurrentProcessId(), pi.dwProcessId, iHookRc, nErrCode);
		MessageBoxW(nullptr, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	}
	else
	{
		//_ASSERTE(pi.hProcess && pi.hThread && pi.dwProcessId && pi.dwThreadId);
		wchar_t szDbgMsg[512] = L"", szTitle[128] = L"";
		swprintf_c(szTitle, L"ConEmuC, PID=%u", GetCurrentProcessId());
		swprintf_c(szDbgMsg, L"ConEmuC.X, PID=%u\nCmdLine parsing FAILED (%u,%u,%u,%u,%u)!\n%s",
			GetCurrentProcessId(), LODWORD(pi.hProcess), LODWORD(pi.hThread), pi.dwProcessId, pi.dwThreadId, lbForceGui, //-V205
			asCmdArg.c_str(L""));
		MessageBoxW(nullptr, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	}

	return CERR_HOOKS_FAILED;
}

int DoInjectRemote(const CEStr& asCmdArg, bool abDefTermOnly)
{
	gbInShutdown = TRUE; // avoid confirmations on exit
	gState.runMode_ = RunMode::SetHook64;
	auto* pszNext = asCmdArg.data();
	LPWSTR pszEnd = nullptr;
	const DWORD nRemotePID = wcstoul(pszNext, &pszEnd, 10);
	wchar_t szStr[16];
	wchar_t szTitle[128];
	wchar_t szInfo[120];
	wchar_t szParentPID[32];


	#ifdef _DEBUG
	extern int ShowInjectRemoteMsg(int nRemotePID, LPCWSTR asCmdArg);
	if (ShowInjectRemoteMsg(nRemotePID, asCmdArg) != IDOK)
	{
		return CERR_HOOKS_FAILED;
	}
	#endif


	if (nRemotePID)
	{
		#if defined(SHOW_ATTACH_MSGBOX)
		if (!IsDebuggerPresent())
		{
			swprintf_c(szTitle, L"%s PID=%u /INJECT", gsModuleName, gnSelfPID);
			const wchar_t* pszCmdLine = GetCommandLineW();
			MessageBox(nullptr,pszCmdLine,szTitle,MB_SYSTEMMODAL);
		}
		#endif

		CEStr lsName, lsPath;
		{
		CProcessData processes;
		processes.GetProcessName(nRemotePID, lsName.GetBuffer(MAX_PATH), MAX_PATH, lsPath.GetBuffer(MAX_PATH * 2), MAX_PATH * 2, nullptr);
		const CEStr lsLog(L"Remote: PID=", ultow_s(nRemotePID, szStr, 10), L" Name=`", lsName, L"` Path=`", lsPath, L"`");
		LogString(lsLog);
		}

		// Go to hook
		// InjectRemote waits for thread termination
		DWORD nErrCode = 0;
		const CINFILTRATE_EXIT_CODES iHookRc = InjectRemote(nRemotePID, abDefTermOnly, &nErrCode);

		swprintf_c(szInfo, L"InjectRemote result: %i (%s)", iHookRc,
			(iHookRc == CIR_OK) ? L"CIR_OK" :
			(iHookRc == CIR_AlreadyInjected) ? L"CIR_AlreadyInjected" :
			L"?");
		LogString(szInfo);

		if (iHookRc == CIR_OK/*0*/ || iHookRc == CIR_AlreadyInjected/*1*/)
		{
			return iHookRc ? CERR_HOOKS_WAS_ALREADY_SET : CERR_HOOKS_WAS_SET;
		}
		else if ((iHookRc == CIR_ProcessWasTerminated) || (iHookRc == CIR_OpenProcess))
		{
			// Don't show error message to user. These codes are logged to file only.
			return CERR_HOOKS_FAILED;
		}

		const DWORD nSelfPid = GetCurrentProcessId();
		PROCESSENTRY32 self = {}, parent = {};
		// Not optimal, needs refactoring
		if (GetProcessInfo(nSelfPid, self))
			GetProcessInfo(self.th32ParentProcessID, parent);

		// Ошибку (пока во всяком случае) лучше показать, для отлова возможных проблем
		//_ASSERTE(iHookRc == 0); -- ассерт не нужен, есть MsgBox

		swprintf_c(szTitle,
			L"%s %s, PID=%u", gsModuleName, gsVersion, nSelfPid);

		swprintf_c(szInfo,
			L"Injecting remote FAILED, code=%i:0x%08X\n"
			L"%s %s, PID=%u\n"
			L"RemotePID=%u ",
			iHookRc, nErrCode, gsModuleName, gsVersion, nSelfPid, nRemotePID);

		swprintf_c(szParentPID,
			L"\n"
			L"ParentPID=%u ",
			self.th32ParentProcessID);

		const CEStr lsError(
			szInfo,
			lsPath.IsEmpty() ? lsName.IsEmpty() ? L"<Unknown>" : lsName.ms_Val : lsPath.ms_Val,
			szParentPID,
			parent.szExeFile);

		LogString(lsError);
		MessageBoxW(nullptr, lsError, szTitle, MB_SYSTEMMODAL);
	}
	else
	{
		//_ASSERTE(pi.hProcess && pi.hThread && pi.dwProcessId && pi.dwThreadId);
		wchar_t szDbgMsg[512];
		swprintf_c(szTitle, L"ConEmuC, PID=%u", GetCurrentProcessId());
		swprintf_c(szDbgMsg, L"ConEmuC.X, PID=%u\nCmdLine parsing FAILED (%u)!\n%s",
			GetCurrentProcessId(), nRemotePID,
			asCmdArg.c_str());
		LogString(szDbgMsg);
		MessageBoxW(nullptr, szDbgMsg, szTitle, MB_SYSTEMMODAL);
	}

	return CERR_HOOKS_FAILED;
}

// GCC error: template argument for 'template<class _Ty> class MArray' uses local type
struct ProcInfo {
	DWORD nPID, nParentPID;
	DWORD_PTR Flags;
	WCHAR szExeFile[64];
};

int DoExportEnv(LPCWSTR asCmdArg, ConEmuExecAction eExecAction, bool bSilent /*= false*/)
{
	int iRc = CERR_CARGUMENT;
	//ProcInfo* pList = nullptr;
	MArray<ProcInfo> List;
	LPWSTR pszAllVars = nullptr, pszSrc;
	int iVarCount = 0;
	CESERVER_REQ *pIn = nullptr;
	const DWORD nSelfPID = GetCurrentProcessId();
	DWORD nTestPID, nParentPID;
	DWORD nSrvPID = 0;
	CESERVER_CONSOLE_MAPPING_HDR test = {};
	BOOL lbMapExist = false;
	HANDLE h;
	size_t cchMaxEnvLen = 0;
	wchar_t* pszBuffer;
	CmdArg szTmpPart;
	LPCWSTR pszTmpPartStart;
	LPCWSTR pszCmdArg;
	wchar_t szInfo[200];

	//_ASSERTE(FALSE && "Continue with exporting environment");

	#define ExpFailedPref CE_CONEMUC_NAME_A ": can't export environment"

	if (!gState.realConWnd_)
	{
		_ASSERTE(gState.realConWnd_);
		if (!bSilent)
			PrintBuffer(ExpFailedPref ", gState.realConWnd was not set\n");
		goto wrap;
	}



	// Query current environment
	pszAllVars = GetEnvironmentStringsW();
	if (!pszAllVars || !*pszAllVars)
	{
		if (!bSilent)
			Printf(ExpFailedPref ", GetEnvironmentStringsW failed, code=%u\n", GetLastError());
		goto wrap;
	}

	// Parse variables block, determining MAX length
	pszSrc = pszAllVars;
	while (*pszSrc)
	{
		LPWSTR pszName = pszSrc;
		LPWSTR pszVal = pszName + lstrlen(pszName) + 1;
		pszSrc = pszVal;
	}
	cchMaxEnvLen = pszSrc - pszAllVars + 1;

	// Preparing buffer
	pIn = ExecuteNewCmd(CECMD_EXPORTVARS, sizeof(CESERVER_REQ_HDR)+cchMaxEnvLen*sizeof(wchar_t));
	if (!pIn)
	{
		if (!bSilent)
			PrintBuffer(ExpFailedPref ", pIn allocation failed\n");
		goto wrap;
	}
	pszBuffer = (wchar_t*)pIn->wData;

	pszCmdArg = SkipNonPrintable(asCmdArg);

	//_ASSERTE(FALSE && "Continue to export");

	LogString(L"DoExportEnv: Loading variables to export");

	// Copy variables to buffer
	if (!pszCmdArg || !*pszCmdArg || (lstrcmp(pszCmdArg, L"*")==0) || (lstrcmp(pszCmdArg, L"\"*\"")==0)
		|| (wcsncmp(pszCmdArg, L"* ", 2)==0) || (wcsncmp(pszCmdArg, L"\"*\" ", 4)==0))
	{
		// transfer ALL variables, except of ConEmu's internals
		pszSrc = pszAllVars;
		while (*pszSrc)
		{
			// may be "=C:=C:\Program Files\..."
			LPWSTR pszEq = wcschr(pszSrc+1, L'=');
			if (!pszEq)
			{
				pszSrc = pszSrc + lstrlen(pszSrc) + 1;
				continue;
			}
			*pszEq = 0;
			LPWSTR pszName = pszSrc;
			LPWSTR pszVal = pszName + lstrlen(pszName) + 1;
			LPWSTR pszNext = pszVal + lstrlen(pszVal) + 1;
			if (IsExportEnvVarAllowed(pszName))
			{
				// Non ConEmu's internals, transfer it
				size_t cchAdd = pszNext - pszName;
				wmemmove(pszBuffer, pszName, cchAdd);
				pszBuffer += cchAdd;
				_ASSERTE(*pszBuffer == 0);
				iVarCount++;
				if (gpLogSize)
				{
					*pszEq = L'=';
					LogString(pszName);
				}
			}
			*pszEq = L'=';
			pszSrc = pszNext;
		}
	}
	else while ((pszCmdArg = NextArg(pszCmdArg, szTmpPart, &pszTmpPartStart)))
	{
		if ((pszTmpPartStart > asCmdArg) && (*(pszTmpPartStart-1) != L'"'))
		{
			// Unless the argument name was surrounded by double-quotes
			// replace commas with spaces, this allows more intuitive
			// way to run something like this:
			// ConEmuC -export=ALL SSH_AGENT_PID,SSH_AUTH_SOCK
			wchar_t* pszComma = szTmpPart.ms_Val;
			while ((pszComma = (wchar_t*)wcspbrk(pszComma, L",;")) != nullptr)
			{
				*pszComma = L' ';
			}
		}
		LPCWSTR pszPart = szTmpPart;
		CmdArg szTest;
		while ((pszPart = NextArg(pszPart, szTest)))
		{
			if (!*szTest || *szTest == L'*')
			{
				if (!bSilent)
					PrintBuffer(ExpFailedPref ", name masks can't be quoted\n");
				goto wrap;
			}

			bool bFound = false;
			size_t cchLeft = cchMaxEnvLen - 1;
			// Loop through variable names
			pszSrc = pszAllVars;
			while (*pszSrc)
			{
				// may be "=C:=C:\Program Files\..."
				LPWSTR pszEq = wcschr(pszSrc+1, L'=');
				if (!pszEq)
				{
					pszSrc = pszSrc + lstrlen(pszSrc) + 1;
					continue;
				}
				*pszEq = 0;
				LPWSTR pszName = pszSrc;
				LPWSTR pszVal = pszName + lstrlen(pszName) + 1;
				LPWSTR pszNext = pszVal + lstrlen(pszVal) + 1;
				if (IsExportEnvVarAllowed(pszName)
					&& CompareFileMask(pszName, szTest)) // limited
				{
					// Non ConEmu's internals, transfer it
					size_t cchAdd = pszNext - pszName;
					if (cchAdd >= cchLeft)
					{
						*pszEq = L'=';
						if (!bSilent)
							PrintBuffer(ExpFailedPref ", too many variables\n");
						goto wrap;
					}
					wmemmove(pszBuffer, pszName, cchAdd);
					iVarCount++;
					pszBuffer += cchAdd;
					_ASSERTE(*pszBuffer == 0);
					cchLeft -= cchAdd;
					bFound = true;
					if (gpLogSize)
					{
						*pszEq = L'=';
						LogString(pszName);
					}
				}
				*pszEq = L'=';
				pszSrc = pszNext;

				// continue scan, only if it is a mask
				if (bFound && !wcschr(szTest, L'*'))
					break;
			} // end of 'while (*pszSrc)'

			if (!bFound && !wcschr(szTest, L'*'))
			{
				// To avoid "nothing to export"
				size_t cchAdd = lstrlen(szTest) + 1;
				if (cchAdd < cchLeft)
				{
					// Copy "Name\0\0"
					wmemmove(pszBuffer, szTest, cchAdd);
					iVarCount++;
					cchAdd++; // We need second zero after a Name
					pszBuffer += cchAdd;
					cchLeft -= cchAdd;
					_ASSERTE(*pszBuffer == 0);
					if (gpLogSize)
					{
						CEStr lsLog(L"Variable `", szTest, L"` was not found");
						LogString(lsLog);
					}
				}
			}
		}
	}

	if (pszBuffer == (wchar_t*)pIn->wData)
	{
		if (!bSilent)
			PrintBuffer(ExpFailedPref ", nothing to export\n");
		goto wrap;
	}
	_ASSERTE(*pszBuffer==0 && *(pszBuffer-1)==0); // Must be ASCIIZZ
	_ASSERTE((INT_PTR)cchMaxEnvLen >= (pszBuffer - (wchar_t*)pIn->wData + 1));

	// Precise buffer size
	cchMaxEnvLen = pszBuffer - (wchar_t*)pIn->wData + 1;
	_ASSERTE(!(cchMaxEnvLen>>20));
	pIn->hdr.cbSize = sizeof(CESERVER_REQ_HDR)+(DWORD)cchMaxEnvLen*sizeof(wchar_t);

	// Find current server (even if no server or no GUI - apply environment to parent tree)
	lbMapExist = LoadSrvMapping(gState.realConWnd_, test);
	if (lbMapExist)
	{
		_ASSERTE(test.ComSpec.ConEmuExeDir[0] && test.ComSpec.ConEmuBaseDir[0]);
		nSrvPID = test.nServerPID;
	}

	// Allocate memory for process tree
	if (!List.reserve(4096))
	{
		if (!bSilent)
			PrintBuffer(ExpFailedPref ", List allocation failed\n");
		goto wrap;
	}

	// Go, build tree (first step - query all running PIDs in the system)
	nParentPID = nSelfPID;
	// Don't do snapshot if only GUI was requested
	h = (eExecAction != ConEmuExecAction::ExportGui) ? CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) : nullptr;
	// Snapshot opened?
	if (h && (h != INVALID_HANDLE_VALUE))
	{
		PROCESSENTRY32 PI = {sizeof(PI)};
		if (Process32First(h, &PI))
		{
			do {
				if (PI.th32ProcessID == nSelfPID)
				{
					// On the first step we'll get our parent process
					nParentPID = PI.th32ParentProcessID;
				}
				LPCWSTR pszName = PointToName(PI.szExeFile);
				ProcInfo pi = {
					PI.th32ProcessID,
					PI.th32ParentProcessID,
					IsConsoleServer(pszName) ? 1UL : 0UL
				};
				lstrcpyn(pi.szExeFile, pszName, countof(pi.szExeFile));
				List.push_back(pi);
			} while (Process32Next(h, &PI));
		}

		CloseHandle(h);
	}

	// Send to parents tree
	if (!List.empty() && (nParentPID != nSelfPID))
	{
		DWORD nOurParentPID = nParentPID;
		// Loop while parent is found
		bool bParentFound = true;
		while (nParentPID != nSrvPID)
		{
			wchar_t szName[64] = L"???";
			nTestPID = nParentPID;
			bParentFound = false;
			// find next parent
			INT_PTR i = 0;
			INT_PTR nCount = List.size();
			while (i < nCount)
			{
				const ProcInfo& pi = List[i];
				if (pi.nPID == nTestPID)
				{
					nParentPID = pi.nParentPID;
					if (pi.Flags & 1)
					{
						// ConEmuC - пропустить
						i = 0;
						nTestPID = nParentPID;
						continue;
					}
					lstrcpyn(szName, pi.szExeFile, countof(szName));
					// nTestPID is already pi.nPID
					bParentFound = true;
					break;
				}
				i++;
			}

			if (nTestPID == nSrvPID)
				break; // Fin, this is root server
			if (!bParentFound)
				break; // Fin, no more parents
			if (nTestPID == nOurParentPID)
				continue; // Useless, we just inherited ALL those variables from our parent

			swprintf_c(szInfo, L"DoExportEnv: PID=%u `%s`", nTestPID, szName);
			LogString(szInfo);

			// Apply environment
			CESERVER_REQ *pOut = ExecuteHkCmd(nTestPID, pIn, gState.realConWnd_, FALSE, TRUE);

			if (!pOut && !bSilent)
			{
				swprintf_c(szInfo,
					WIN3264TEST(L"ConEmuC",L"ConEmuC64")
					L": process %s PID=%u was skipped: noninteractive or lack of ConEmuHk\n",
					szName, nTestPID);
				PrintBuffer(szInfo);
			}

			ExecuteFreeResult(pOut);
		}
	}

	// We are trying to apply environment to parent tree even if NO server or GUI was found
	if (nSrvPID)
	{
		swprintf_c(szInfo, L"DoExportEnv: PID=%u (server)", nSrvPID);
		LogString(szInfo);

		// Server found? Try to apply environment
		CESERVER_REQ *pOut = ExecuteSrvCmd(nSrvPID, pIn, gState.realConWnd_);

		if (!pOut)
		{
			if (!bSilent)
				Printf(ExpFailedPref " to PID=%u, root server was terminated?\n", nSrvPID);
		}
		else
		{
			ExecuteFreeResult(pOut);
		}
	}

	// Если просили во все табы - тогда досылаем и в GUI
	if ((eExecAction != ConEmuExecAction::ExportCon) && lbMapExist && test.hConEmuRoot && IsWindow((HWND)test.hConEmuRoot))
	{
		if (eExecAction == ConEmuExecAction::ExportAll)
		{
			pIn->hdr.nCmd = CECMD_EXPORTVARSALL;
		}
		else
		{
			_ASSERTE(pIn->hdr.nCmd == CECMD_EXPORTVARS);
		}

		LogString(L"DoExportEnv: ConEmu GUI");

		// ea_ExportTab, ea_ExportGui, ea_ExportAll -> export to ConEmu window
		ExecuteGuiCmd(gState.realConWnd_, pIn, gState.realConWnd_, TRUE);
	}

	swprintf_c(szInfo, WIN3264TEST(L"ConEmuC",L"ConEmuC64") L": %i %s processed", iVarCount, (iVarCount == 1) ? L"variable was" : L"variables were");
	LogString(szInfo);
	if (!bSilent)
	{
		wcscat_c(szInfo, L"\n");
		PrintBuffer(szInfo);
	}

	iRc = 0;
wrap:
	// Fin
	if (pszAllVars)
		FreeEnvironmentStringsW((LPWCH)pszAllVars);
	if (pIn)
		ExecuteFreeResult(pIn);

	return iRc;
}


// The function exists in both "ConEmuC/ConEmuC.cpp" and "ConEmuCD/Actions.cpp"
// Version in "ConEmuC/ConEmuC.cpp" shows arguments from main(int argc, char** argv)
// Version in "ConEmuCD/Actions.cpp" perhaps would not be ever called
int DoParseArgs(LPCWSTR asCmdLine)
{
	PrintBuffer("Parsing command\n  `");
	PrintBuffer(asCmdLine);
	PrintBuffer("`\n");

	int iShellCount = 0;
	LPWSTR* ppszShl = CommandLineToArgvW(asCmdLine, &iShellCount);

	int i = 0;
	CmdArg szArg;
	PrintBuffer("ConEmu `NextArg` splitter\n");
	while ((asCmdLine = NextArg(asCmdLine, szArg)))
	{
		if (szArg.m_bQuoted)
			DemangleArg(szArg, true);
		Printf("  %u: `", ++i);
		PrintBuffer(szArg);
		PrintBuffer("`\n");
	}
	Printf("  Total arguments parsed: %u\n", i);

	PrintBuffer("Standard shell splitter\n");
	for (int j = 0; j < iShellCount; j++)
	{
		Printf("  %u: `", j);
		PrintBuffer(ppszShl[j]);
		PrintBuffer("`\n");
	}
	Printf("  Total arguments parsed: %u\n", iShellCount);
	LocalFree(ppszShl);

	return i;
}

int DoOutput(ConEmuExecAction eExecAction, LPCWSTR asCmdArg)
{
	int iRc = 0;
	CEStr    szTemp;
	LPCWSTR  pszText = nullptr;
	DWORD    cchLen = 0, dwWritten = 0;
	bool     bAddNewLine = true;
	bool     bProcessed = true;
	bool     bToBottom = false;
	bool     bAsciiPrint = false;
	bool     bExpandEnvVar = false;
	bool     bStreamBy1 = false;
	CmdArg   szArg;
	HANDLE   hFile = nullptr;
	DWORD    DefaultCP = 0;

	#ifdef SHOW_OUTPUT_MSGBOX
	_ASSERTE(FALSE && "Continue to DoOutput");
	#endif

	_ASSERTE(asCmdArg && (*asCmdArg != L' '));
	asCmdArg = SkipNonPrintable(asCmdArg);

	while ((*asCmdArg == L'-' || *asCmdArg == L'/') && (asCmdArg = NextArg(asCmdArg, szArg)))
	{
		// Make uniform
		if (szArg.ms_Val[0] == L'/')
			szArg.ms_Val[0] = L'-';

		// Do not CRLF after printing
		if (lstrcmpi(szArg, L"-n") == 0)
			bAddNewLine = false;
		// ‘RAW’: Do not process ANSI and specials (^e^[^r^n^t^b...)
		else if (lstrcmpi(szArg, L"-r") == 0)
			bProcessed = false;
		// Expand environment variables before echo
		else if (lstrcmpi(szArg, L"-x") == 0)
			bExpandEnvVar = true;
		// Scroll to bottom of screen buffer (ConEmu TrueColor buffer compatible)
		else if (lstrcmpi(szArg, L"-b") == 0)
			bToBottom = true;
		// Use Ascii functions to print text (RAW data, don't convert to unicode)
		else if (lstrcmpi(szArg, L"-a") == 0)
		{
			if (eExecAction == ConEmuExecAction::OutType) bAsciiPrint = true;
		}
		// For testing purposes, stream characters one-by-one
		else if (lstrcmpi(szArg, L"-s") == 0)
		{
			if (eExecAction == ConEmuExecAction::OutType) bStreamBy1 = true;
		}
		// Forced codepage of typed text file
		else // `-65001`, `-utf8`, `-oemcp`, etc.
		{
			UINT nCP = GetCpFromString(szArg.ms_Val+1);
			if (nCP) DefaultCP = nCP;
		}
	}

	_ASSERTE(asCmdArg && (*asCmdArg != L' '));
	asCmdArg = SkipNonPrintable(asCmdArg);

	if (eExecAction == ConEmuExecAction::OutType)
	{
		if ((asCmdArg = NextArg(asCmdArg, szArg)))
		{
			_ASSERTE(!bAsciiPrint || !DefaultCP);
			DWORD nSize = 0, nErrCode = 0;
			const int iRead = ReadTextFile(szArg, (1<<24), szTemp, cchLen, nErrCode, bAsciiPrint ? static_cast<DWORD>(-1) : DefaultCP);
			if (iRead < 0)
			{
				wchar_t szInfo[100];
				swprintf_c(szInfo, L"\r\nCode=%i, Error=%u\r\n", iRead, nErrCode);
				szTemp = CEStr(L"Reading source file failed!\r\n", szArg, szInfo);
				cchLen = static_cast<DWORD>(szTemp.GetLen());
				bAsciiPrint = false;
				iRc = 4;
			}
			pszText = szTemp.ms_Val;
		}
	}
	else if (eExecAction == ConEmuExecAction::OutEcho)
	{
		_ASSERTE(szTemp.ms_Val == nullptr);

		while ((asCmdArg = NextArg(asCmdArg, szArg)))
		{
			LPCWSTR pszAdd = szArg.ms_Val;
			_ASSERTE(pszAdd!=nullptr);

			CEStr lsExpand;
			CmdArg lsDemangle;

			// Expand environment variables
			// TODO: Expand !Variables! too
			if (bExpandEnvVar && wcschr(pszAdd, L'%'))
			{
				lsExpand = ExpandEnvStr(pszAdd);
				if (!lsExpand.IsEmpty())
					pszAdd = lsExpand.ms_Val;
			}

			// Replace two double-quotes with one double-quotes
			if (szArg.m_bQuoted
				// Process special symbols: ^e^[^r^n^t^b
				|| (bProcessed && wcschr(pszAdd, L'^'))
				)
			{
				lsDemangle.Set(pszAdd);
				if (DemangleArg(lsDemangle, szArg.m_bQuoted, bProcessed))
					pszAdd = lsDemangle.ms_Val;
			}

			// Concatenate result text
			szTemp.Append(szTemp.IsEmpty() ? nullptr : L" ", pszAdd);
		}

		if (bAddNewLine)
			szTemp.Append(L"\r\n");
		pszText = szTemp.ms_Val;
		cchLen = pszText ? lstrlen(pszText) : 0;
	}

	#ifdef _DEBUG
	if (bAsciiPrint)
	{
		_ASSERTE(eExecAction == ConEmuExecAction::OutType);
	}
	#endif

	iRc = WriteOutput(pszText, cchLen, &dwWritten, bProcessed, bAsciiPrint, bStreamBy1, bToBottom);

	return iRc;
}

namespace {
	/// ConEmuHk is used for preprocessed ANSI output
	static MModule ConEmuHk;

	// Restore the shared console mode even if output fails.
	class NativeAnsiOutput
	{
	public:
		explicit NativeAnsiOutput(HANDLE output) : output_(output) {}
		~NativeAnsiOutput()
		{
			if (changed_)
				SetConsoleMode(output_, mode_);
		}

		void Enable()
		{
			if (GetConsoleMode(output_, &mode_))
			{
				const DWORD vtMode = mode_ | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
				changed_ = vtMode != mode_ && SetConsoleMode(output_, vtMode);
			}
		}

	private:
		HANDLE output_;
		DWORD mode_ = 0;
		bool changed_ = false;
	};
};

/// Write text to STD_OUT
/// @param pszText     text to write, must be '\0'-terminated if #cchLen == -1
/// @param cchLen      [default -1] length of text in characters or -1 to use len(pszText)
/// @param pdwWritten  [default null, out] written count
/// @param bProcessed  [default false] true if pszText contains ANSI sequences (colors, etc.)
/// @param bAsciiPrint [default false] true if pszText is ASCII (not wchar_t)
/// @param bStreamBy1  [default false] true to Write 1 character per function call (debug purposes)
/// @param bToBottom   [default false] true to set text cursor position at the bottom of scroll buffer (force use true-color area)
/// @result            0 on success, otherwise our internal error code
int WriteOutput(LPCWSTR pszText, DWORD cchLen /*= -1*/, DWORD* pdwWritten /*= nullptr*/, bool bProcessed /*= false*/, bool bAsciiPrint /*= false*/, bool bStreamBy1 /*= false*/, bool bToBottom /*= false*/)
{
	int    iRc = 0;
	BOOL   bRc = FALSE;
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD  dwWritten = 0;

	if (bToBottom)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi = {};
		if (GetConsoleScreenBufferInfo(hOut, &csbi))
		{
			COORD crBottom = {0, csbi.dwSize.Y-1};
			SetConsoleCursorPosition(hOut, crBottom);
			SHORT Y1 = crBottom.Y - (csbi.srWindow.Bottom-csbi.srWindow.Top);
			SMALL_RECT srWindow = {0, Y1, csbi.srWindow.Right-csbi.srWindow.Left, crBottom.Y};
			SetConsoleWindowInfo(hOut, TRUE, &srWindow);
		}
	}

	if (pszText && (cchLen == DWORD(-1)))
		cchLen = bAsciiPrint
			? (DWORD)strlen((const char*)pszText)
			: (DWORD)wcslen(pszText);

	if (!pszText || !cchLen)
	{
		iRc = 1;
		goto wrap;
	}

	if (!IsOutputRedirected())
	{
		typedef BOOL (WINAPI* WriteProcessed_t)(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
		typedef BOOL (WINAPI* WriteProcessedA_t)(LPCSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, UINT Stream);
		static WriteProcessed_t WriteProcessed = nullptr;
		static WriteProcessedA_t WriteProcessedA = nullptr;
#if !defined(__aarch64__) && !defined(_M_ARM64)
		if (bProcessed && (!WriteProcessed || !WriteProcessedA))
		{
			//_ASSERTE(FALSE && "Continue to load ConEmuHk");
			// ConEmuHk.dll / ConEmuHk64.dll
			if (ConEmuHk.Load(ConEmuHk_DLL_3264))
			{
				ConEmuHk.GetProcAddress("WriteProcessed", WriteProcessed);
				ConEmuHk.GetProcAddress("WriteProcessedA", WriteProcessedA);
				if (!WriteProcessed || !WriteProcessedA)
				{
					const DWORD errCode = GetLastError();
					wchar_t buffer[120];
					msprintf(buffer, countof(buffer), L"WriteOutput: GetProcAddress(%s, WriteProcessed) failed, code=%u", ConEmuHk_DLL_3264, errCode);
					LogString(buffer);
				}
				else
				{
					LogString(L"WriteOutput: using WriteProcessed from " ConEmuHk_DLL_3264);
				}
			}
			else
			{
				const DWORD errCode = GetLastError();
				wchar_t buffer[120];
				msprintf(buffer, countof(buffer), L"WriteOutput: LoadLibrary(%s) failed, code=%u", ConEmuHk_DLL_3264, errCode);
				LogString(buffer);
			}
		}
		else
		{
			LogString(L"WriteOutput: using WriteConsole because no escape sequences were found");
		}
#endif

		NativeAnsiOutput nativeAnsi(hOut);
		if (bProcessed && !(bAsciiPrint ? WriteProcessedA != nullptr : WriteProcessed != nullptr))
			nativeAnsi.Enable();

		// If possible - use processed (with ANSI support) output via WriteProcessed[A] function

		if (bAsciiPrint)
		{
			if (!bStreamBy1)
			{
				if (WriteProcessedA)
					bRc = WriteProcessedA((char*)pszText, cchLen, &dwWritten, 1);
				else
					bRc = WriteConsoleA(hOut, (char*)pszText, cchLen, &dwWritten, 0);
			}
			else
			{
				if (WriteProcessedA)
				{
					for (DWORD i = 0; i < cchLen; i++)
						bRc = WriteProcessedA(((char*)pszText)+i, 1, &dwWritten, 1);
				}
				else
				{
					for (DWORD i = 0; i < cchLen; i++)
						bRc = WriteConsoleA(hOut, ((char*)pszText)+i, 1, &dwWritten, 0);
				}
			}
		}
		else
		{
			if (WriteProcessed)
				bRc = WriteProcessed(pszText, cchLen, &dwWritten);
			else
				bRc = WriteConsoleW(hOut, pszText, cchLen, &dwWritten, 0);
		}
	}
	else if (bAsciiPrint)
	{
		LogString(L"WriteOutput: using WriteFile because output is redirected");
		bRc = WriteFile(hOut, pszText, cchLen, &dwWritten, 0);
	}
	else
	{
		// Current process output was redirected to file!
		LogString(L"WriteOutput: using WriteFile on unicode string because output is redirected");

		char* pszOem = nullptr;
		UINT  cp = GetConsoleOutputCP();
		int   nDstLen = WideCharToMultiByte(cp, 0, pszText, cchLen, nullptr, 0, nullptr, nullptr);
		if (nDstLen < 1)
		{
			iRc = 2;
			goto wrap;
		}

		pszOem = (char*)malloc(nDstLen);
		if (pszOem)
		{
			int nWrite = WideCharToMultiByte(cp, 0, pszText, cchLen, pszOem, nDstLen, nullptr, nullptr);
			if (nWrite > 1)
			{
				bRc = WriteFile(hOut, pszOem, nWrite, &dwWritten, 0);
			}
			free(pszOem);
		}
	}

	if (!bRc && !iRc)
		iRc = 3;

wrap:
	if (pdwWritten)
		*pdwWritten = dwWritten;
	return iRc;
}

int DoStoreCWD(LPCWSTR asCmdArg)
{
	int iRc = 1;
	CmdArg szDir;

	if (!NextArg(asCmdArg, szDir) || szDir.IsEmpty())
	{
		if (GetDirectory(szDir) == nullptr)
			goto wrap;
	}

	// Sends CECMD_STORECURDIR into RConServer
	SendCurrentDirectory(gState.realConWnd_, szDir);
	iRc = 0;
wrap:
	return iRc;
}

int DoExecAction(const ConEmuExecAction eExecAction, LPCWSTR asCmdArg /* rest of cmdline */, MacroInstance& Inst)
{
	int iRc = CERR_CARGUMENT;

	if (gpConsoleArgs->isLogging_.exists)
	{
		CreateLogSizeFile(0);
	}

	switch (eExecAction)
	{
	case ConEmuExecAction::ConPty:
		return RunConPty(asCmdArg);
	case ConEmuExecAction::RegConFont:
		{
			LogString(L"DoExecAction: ea_RegConFont");
			RegisterConsoleFontHKLM(asCmdArg);
			iRc = CERR_EMPTY_COMSPEC_CMDLINE;
			break;
		}
	case ConEmuExecAction::InjectHooks:
		{
			LogString(L"DoExecAction: DoInjectHooks");
			iRc = DoInjectHooks(asCmdArg);
			break;
		}
	case ConEmuExecAction::InjectRemote:
	case ConEmuExecAction::InjectDefTrm:
		{
			LogString(L"DoExecAction: DoInjectRemote");
			iRc = DoInjectRemote(asCmdArg, (eExecAction == ConEmuExecAction::InjectDefTrm));
			break;
		}
	case ConEmuExecAction::GuiMacro: // ConEmuC -GuiMacro
		{
			LogString(L"DoExecAction: DoGuiMacro");
			const GuiMacroFlags flags = GuiMacroFlags::SetEnvVar
				// If current RealConsole was already started in ConEmu, try to export variable
				| ((gpConsoleArgs->macroExportResult_ && (gState.runMode_ != RunMode::GuiMacro) && (gState.conemuWnd_ != nullptr)) ? GuiMacroFlags::ExportEnvVar : GuiMacroFlags::None)
				// Interactive mode, print output to console
				| ((!gpConsoleArgs->preferSilentMode_ && (gState.runMode_ != RunMode::GuiMacro)) ? GuiMacroFlags::PrintResult : GuiMacroFlags::None);
			iRc = DoGuiMacro(asCmdArg, Inst, flags);
			break;
		}
	case ConEmuExecAction::CheckUnicodeFont: // ConEmuC -CheckUnicode
		{
			LogString(L"DoExecAction: ea_CheckUnicodeFont");
			iRc = CheckUnicodeFont();
			break;
		}
	case ConEmuExecAction::PrintConsoleInfo: // ConEmuC -ConInfo
		{
			LogString(L"DoExecAction: ea_PrintConsoleInfo");
			PrintConsoleInfo();
			iRc = 0;
			break;
		}
	case ConEmuExecAction::TestUnicodeCvt: // ConEmuC -TestUnicode
		{
			LogString(L"DoExecAction: ea_TestUnicodeCvt");
			iRc = TestUnicodeCvt();
			break;
		}
	case ConEmuExecAction::OsVerInfo: // ConEmuC -OsVerInfo
		{
			LogString(L"DoExecAction: ea_OsVerInfo");
			iRc = OsVerInfo();
			break;
		}
	case ConEmuExecAction::ExportCon: // ConEmuC -Export ...
	case ConEmuExecAction::ExportTab:
	case ConEmuExecAction::ExportGui:
	case ConEmuExecAction::ExportAll:
		{
			LogString(L"DoExecAction: DoExportEnv");
			iRc = DoExportEnv(asCmdArg, eExecAction, gpConsoleArgs->preferSilentMode_.GetBool());
			break;
		}
	case ConEmuExecAction::ParseArgs: // ConEmuC -Args ... | ConEmuC -ParseArgs
		{
			LogString(L"DoExecAction: DoParseArgs");
			iRc = DoParseArgs(asCmdArg);
			break;
		}
	case ConEmuExecAction::ErrorLevel: // ConEmuC -ErrorLevel
		{
			LogString(L"DoExecAction: ea_ErrorLevel");
			wchar_t* pszEnd = nullptr;
			iRc = wcstol(asCmdArg, &pszEnd, 10);
			break;
		}
	case ConEmuExecAction::OutEcho: // ConEmuC -e ...
	case ConEmuExecAction::OutType: // ConEmuC -t
		{
			LogString(L"DoExecAction: DoOutput");
			iRc = DoOutput(eExecAction, asCmdArg);
			break;
		}
	case ConEmuExecAction::StoreCWD: // ConEmuC -StoreCWD
		{
			LogString(L"DoExecAction: DoStoreCWD");
			iRc = DoStoreCWD(asCmdArg);
			break;
		}
	case ConEmuExecAction::DumpStruct: // ConEmuC -STRUCT
		{
			LogString(L"DoExecAction: DoDumpStruct");
			iRc = DoDumpStruct(asCmdArg);
			break;
		}
	default:
		_ASSERTE(FALSE && "Unsupported ExecAction code");
	}

	return iRc;
}
