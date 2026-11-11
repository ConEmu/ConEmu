
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

#include "ConsoleMain.h"

#include "Actions.h"
#include "ExitCodes.h"
#include "ExportedFunctions.h"
#include "GuiMacro.h"

#include "ConsoleState.h"
#include "RunMode.h"

#include "../common/CEStr.h"
#include "../common/CmdLine.h"
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/MFileMapping.h"
#include "../common/MStrDup.h"
#include "../common/ProcessData.h"

bool operator&(GuiMacroFlags f1, GuiMacroFlags f2)
{
	const auto intersect = static_cast<GuiMacroFlags>(static_cast<int>(f1) & static_cast<int>(f2));
	return intersect == f2;
}

GuiMacroFlags operator|(GuiMacroFlags f1, GuiMacroFlags f2)
{
	return static_cast<GuiMacroFlags>(static_cast<int>(f1) | static_cast<int>(f2));
}


// ReSharper disable twice CppParameterMayBeConst
static BOOL CALLBACK FindTopGuiOrConsole(HWND hWnd, LPARAM lParam)
{
	auto* p = reinterpret_cast<MacroInstance*>(lParam);
	wchar_t szClass[MAX_PATH];
	if (GetClassName(hWnd, szClass, countof(szClass)) < 1)
		return TRUE; // continue search

	// If called "-GuiMacro:0" we need to find first GUI window!
	const bool bConClass = IsConsoleClass(szClass);
	if (!p->nPID && bConClass)
		return TRUE; // continue search

	const bool bGuiClass = (lstrcmp(szClass, VirtualConsoleClassMain) == 0);
	if (!bGuiClass && !bConClass)
		return TRUE; // continue search

	DWORD nTestPID = 0; GetWindowThreadProcessId(hWnd, &nTestPID);
	if (nTestPID == p->nPID || !p->nPID)
	{
		p->hConEmuWnd = hWnd;
		return FALSE; // Found! stop search
	}

	return TRUE; // continue search
}

int GuiMacroCommandLine(LPCWSTR asCmdLine)
{
	int iRc = CERR_CMDLINEEMPTY;
	CmdArg szArg;
	LPCWSTR pszArgStarts = nullptr;
	LPCWSTR lsCmdLine = asCmdLine;
	MacroInstance MacroInst = {}; // Special ConEmu instance for GUIMACRO and other options

	while ((lsCmdLine = NextArg(lsCmdLine, szArg, &pszArgStarts)))
	{
		// Following code wants '/'style arguments
		// Change '-'style to '/'style
		if (szArg[0] == L'-')
			szArg.SetAt(0, L'/');
		else if (szArg[0] != L'/')
			continue;

		// -GUIMACRO[:PID|0xHWND][:T<tab>][:S<split>] <Macro string>
		if (szArg.OneOfSwitches(L"/GuiMacro", L"/GuiMacro="))
		{
			ArgGuiMacro(szArg, MacroInst);

			_ASSERTE(GetRunMode() == RunMode::GuiMacro);
			iRc = DoGuiMacro(lsCmdLine, MacroInst, GuiMacroFlags::SetEnvVar | GuiMacroFlags::PreferSilentMode);

			// We've done
			gbInShutdown = TRUE;
			break;
		}
	}

	return iRc;
}

static HWND GetConEmuExeHWND(DWORD nConEmuPid)
{
	if (!nConEmuPid)
		return nullptr;

	HWND hConEmu = nullptr;

	// EnumThreadWindows will not find child window,
	// so we use map (but it may be not created yet)

	MFileMapping<ConEmuGuiMapping> guiMap;
	guiMap.InitName(CEGUIINFOMAPNAME, nConEmuPid);
	const ConEmuGuiMapping* pMap = guiMap.Open(FALSE);
	if (pMap)
	{
		if ((pMap->cbSize >= (5*sizeof(DWORD)))
			&& (pMap->nGuiPID == nConEmuPid)
			&& IsWindow(pMap->hGuiWnd))
		{
			hConEmu = pMap->hGuiWnd;
		}
	}

	return hConEmu;
}

void ArgGuiMacro(const CEStr& szArg, MacroInstance& inst)
{
	wchar_t szLog[200];
	if (gpLogSize) gpLogSize->LogString(szArg);

	ZeroStruct(inst);

	if (szArg.IsEmpty())
	{
		_ASSERTE(!szArg.IsEmpty());
		return;
	}

	// Caller may specify PID/HWND of desired ConEmu instance,
	// or some other switches, like Tab or Split index
	_ASSERTE((lstrcmpni(szArg.ms_Val, L"/GuiMacro", 9) == 0) || (lstrcmpni(szArg.ms_Val, L"-GuiMacro", 9) == 0));
	if (szArg[9] == L':' || szArg[9] == L'=')
	{
		wchar_t* pszEnd = nullptr;
		wchar_t* pszID = szArg.ms_Val+10;
		// Loop through GuiMacro options
		while (pszID && *pszID)
		{
			// HWND of main ConEmu (GUI) window
			if ((pszID[0] == L'0' && (pszID[1] == L'x' || pszID[1] == L'X'))
				|| (pszID[0] == L'x' || pszID[0] == L'X'))
			{
				inst.hConEmuWnd = (HWND)(DWORD_PTR)wcstoul(pszID[0] == L'0' ? (pszID+2) : (pszID+1), &pszEnd, 16);
				pszID = pszEnd;

				if (gpLogSize)
				{
					swprintf_c(szLog, L"Exact instance requested, HWND=x%08X", (DWORD)(DWORD_PTR)inst.hConEmuWnd);
					gpLogSize->LogString(szLog);
				}
			}
			else if (isDigit(pszID[0]))
			{
				// Если тут передать "0" - то выполняем в первом попавшемся (по Z-order) окне ConEmu
				// То есть даже если ConEmuC вызван во вкладке, то
				// а) макро может выполниться в другом окне ConEmu (если наше свернуто)
				// б) макро может выполниться в другой вкладке (если наша не активна)
				inst.nPID = wcstoul(pszID, &pszEnd, 10);

				wchar_t szExe[MAX_PATH] = L"";
				if (inst.nPID != 0)
				{
					CProcessData proc;
					if (!proc.GetProcessName(inst.nPID, szExe, countof(szExe), NULL, 0, NULL))
						szExe[0] = 0;

					// Using mapping information may be preferable due to speed
					// and more, ConEmu may be started as Child window
					if (!((inst.hConEmuWnd = GetConEmuExeHWND(inst.nPID)))
						&& IsConEmuGui(szExe))
					{
						// If ConEmu.exe has just been started, mapping may be not ready yet
						HANDLE hWaitProcess = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, inst.nPID);
						if (!hWaitProcess && IsWin6())
							hWaitProcess = OpenProcess(SYNCHRONIZE | 0x1000/*PROCESS_QUERY_LIMITED_INFORMATION*/, FALSE, inst.nPID);
						const DWORD nStartTick = GetTickCount();
						const DWORD nDelay = hWaitProcess ? 60000 : 10000;
						DWORD nWait = -1;
						// Until succeeded?
						do
						{
							// Wait a little while ConEmu is initializing
							if (hWaitProcess)
							{
								// ReSharper disable once CppJoinDeclarationAndAssignment
								nWait = WaitForSingleObject(hWaitProcess, 100);
								if (nWait == WAIT_OBJECT_0)
									break; // ConEmu.exe was terminated
							}
							else
							{
								Sleep(100);
							}
							// Recheck
							if ((inst.hConEmuWnd = GetConEmuExeHWND(inst.nPID)) != NULL)
								break; // Found
						} while ((GetTickCount() - nStartTick) <= nDelay);
						// Stop checking
						SafeCloseHandle(hWaitProcess);
					}
				}

				// If mapping with GUI PID does not exist - try to enum all Top level windows
				// We are searching for RealConsole or ConEmu window
				if (!inst.hConEmuWnd)
				{
					EnumWindows(FindTopGuiOrConsole, reinterpret_cast<LPARAM>(&inst));
				}

				if (gpLogSize)
				{
					if (inst.hConEmuWnd && inst.nPID)
						swprintf_c(szLog, L"Exact PID=%u requested, instance found HWND=x%08X", inst.nPID, (DWORD)(DWORD_PTR)inst.hConEmuWnd);
					else if (inst.hConEmuWnd && !inst.nPID)
						swprintf_c(szLog, L"First found requested, instance found HWND=x%08X", (DWORD)(DWORD_PTR)inst.hConEmuWnd);
					else
						swprintf_c(szLog, L"No instances was found (requested PID=%u) GuiMacro will fail", inst.nPID);
					gpLogSize->LogString(szLog);
				}

				if (pszID == pszEnd)
					break;
				pszID = pszEnd;
			}
			else if (wcschr(L"SsTt", pszID[0]) && (isDigit(pszID[1]) || (pszID[1] == L'-' && isDigit(pszID[2]))))
			{
				switch (pszID[0])
				{
				case L'S': case L's':
					inst.nSplitIndex = wcstoul(pszID+1, &pszEnd, 10);
					swprintf_c(szLog, L"Split was requested: %u", inst.nSplitIndex);
					break;
				case L'T': case L't':
					inst.nTabIndex = wcstol(pszID+1, &pszEnd, 10);
					swprintf_c(szLog, L"Tab was requested: %i", inst.nTabIndex);
					break;
				default:
					_ASSERTE(FALSE && "Should not get here");
				}
				if (gpLogSize)
					gpLogSize->LogString(szLog);
				if (pszID == pszEnd)
					break;
				pszID = pszEnd;
			}
			else if (*pszID == L':')
			{
				pszID++;
			}
			else
			{
				_ASSERTE(FALSE && "Unsupported GuiMacro option");
				if (gpLogSize)
				{
					const CEStr strErr(L"Unsupported GuiMacro option: ", szArg.ms_Val);
					gpLogSize->LogString(strErr);
				}
				break;
			}
		}

		// This may be VirtualConsoleClassMain or RealConsoleClass...
		if (inst.hConEmuWnd)
		{
			// Has no effect, if hMacroInstance == RealConsoleClass
			wchar_t szClass[MAX_PATH] = L"";
			if ((GetClassName(inst.hConEmuWnd, szClass, countof(szClass)) > 0) && (lstrcmp(szClass, VirtualConsoleClassMain) == 0))
			{
				DWORD nGuiPID = 0; GetWindowThreadProcessId(inst.hConEmuWnd, &nGuiPID);
				AllowSetForegroundWindow(nGuiPID);
			}
			if (gpLogSize)
			{
				gpLogSize->LogString(L"Found instance class name: ", true, NULL, false);
				gpLogSize->LogString(szClass, false, NULL, true);
			}
		}
	}
}

int DoGuiMacro(LPCWSTR asCmdArg, MacroInstance& inst, GuiMacroFlags flags, BSTR* bsResult /*= NULL*/)
{
	// If neither hMacroInstance nor gState.conemuWnd_ was set - Macro will fail most likely
	// Skip assertion show for IsConEmu, it's used in tests
	_ASSERTE(inst.hConEmuWnd!=NULL || gState.realConWnd_!=NULL || gState.conemuWnd_!=NULL || (wcscmp(asCmdArg, L"IsConEmu") == 0));

	wchar_t szErrInst[80] = L"FAILED:Specified ConEmu instance is not found";
	// ReSharper disable once CppVariableCanBeMadeConstexpr
	const wchar_t szErrExec[80] = L"FAILED:Unknown GuiMacro execution error";

	// Don't allow to execute on wrong instance
	if ((inst.nPID && !inst.hConEmuWnd)
		|| (inst.hConEmuWnd && !IsWindow(inst.hConEmuWnd))
		)
	{
		if (bsResult)
		{
			*bsResult = ::SysAllocString(szErrInst);
		}

		bool bRedirect = false;
		const bool bPrintError = (flags & GuiMacroFlags::PrintResult)
			&& (((bRedirect = IsOutputRedirected())) || !(flags & GuiMacroFlags::PreferSilentMode));
		if (bPrintError)
		{
			if (bRedirect) wcscat_c(szErrInst, L"\n"); // PowerShell... it does not insert linefeed
			PrintBuffer(szErrInst);
		}

		return CERR_GUIMACRO_FAILED;
	}

	auto* const hCallWnd =
			(inst.nTabIndex == -2) ? gState.realConWnd_
			: inst.hConEmuWnd ? inst.hConEmuWnd
			: GetConEmuWindows(gState.realConWnd_).ConEmuRoot;
	_ASSERTE(hCallWnd != nullptr);

	// Все что в asCmdArg - выполнить в Gui
	int iRc = CERR_GUIMACRO_FAILED;
	const int nLen = lstrlen(asCmdArg);
	//SetEnvironmentVariable(CEGUIMACRORETENVVAR, NULL);
	CESERVER_REQ *pIn = nullptr, *pOut = nullptr;
	pIn = ExecuteNewCmd(CECMD_GUIMACRO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GUIMACRO)+nLen*sizeof(wchar_t));
	pIn->GuiMacro.nTabIndex = inst.nTabIndex;
	pIn->GuiMacro.nSplitIndex = inst.nSplitIndex;
	lstrcpyW(pIn->GuiMacro.sMacro, asCmdArg);

	LogString(L"DoGuiMacro: executing CECMD_GUIMACRO");

	pOut = ConEmuGuiRpc(hCallWnd).SetOwner(gState.realConWnd_).SetIgnoreAbsence().Execute(pIn);

	LogString(L"DoGuiMacro: CECMD_GUIMACRO finished");

	_ASSERTE(pOut != nullptr);

	if (pOut)
	{
		if (pOut->GuiMacro.nSucceeded)
		{
			LPCWSTR pszResult = (pOut->DataSize() >= sizeof(pOut->GuiMacro)) ? pOut->GuiMacro.sMacro : L"";

			iRc = CERR_GUIMACRO_SUCCEEDED; // OK

			if (bsResult)
			{
				*bsResult = ::SysAllocString(pszResult);
			}

			if ((flags & GuiMacroFlags::SetEnvVar) | (flags & GuiMacroFlags::ExportEnvVar))
			{
				if (gpLogSize)
				{
					const CEStr lsLog(L"DoGuiMacro: set ", CEGUIMACRORETENVVAR, L"=", pszResult);
					LogString(lsLog);
				}
				SetEnvironmentVariable(CEGUIMACRORETENVVAR, pszResult);
			}

			// If current RealConsole was already started in ConEmu
			if (flags & GuiMacroFlags::ExportEnvVar)
			{
				_ASSERTE((flags & GuiMacroFlags::SetEnvVar));
				// Transfer EnvVar to parent console processes
				// This would work only if ‘Inject ConEmuHk’ is enabled
				// However, it's ignored by some shells
				LogString(L"DoGuiMacro: exporting result");
				DoExportEnv(CEGUIMACRORETENVVAR, ConEmuExecAction::ExportCon, true/*bSilent*/);
				LogString(L"DoGuiMacro: DoExportEnv finished");
			}

			// Let reuse `-Silent` switch
			if ((flags & GuiMacroFlags::PrintResult)
				&& (!(flags & GuiMacroFlags::PreferSilentMode) || IsOutputRedirected()))
			{
				LogString(L"DoGuiMacro: printing result");

				// Show macro result in StdOutput
				PrintBuffer(pszResult);

				// PowerShell... it does not insert linefeed
				if (!IsOutputRedirected())
					PrintBuffer(L"\n");
			}
		}

		ExecuteFreeResult(pOut);
	}
	ExecuteFreeResult(pIn);

	if ((iRc != CERR_GUIMACRO_SUCCEEDED) && bsResult)
	{
		*bsResult = ::SysAllocString(szErrExec);
	}

	return iRc;
}

/// <summary>
/// Execute GuiMacro in (optionally) specified instance of ConEmu console/tab/split
/// </summary>
/// <param name="asInstance">Colon-delimited arguments for instance selection [PID|0xHWND]:[T<tab>]:[S<split>]</param>
/// <param name="asMacro">The macro command(s)</param>
/// <param name="bsResult">nullptr - to return result via environment variable "ConEmuMacroResult",
/// INVALID_HANDLE_VALUE</param>
/// <returns></returns>
int __stdcall GuiMacro(LPCWSTR asInstance, LPCWSTR asMacro, BSTR* bsResult /*= NULL*/)
{
	LogString(L"GuiMacro function called");

	MacroInstance inst = {};

	if (asInstance && *asInstance)
	{
		_ASSERTE((lstrcmpni(asInstance, L"/GuiMacro", 9) != 0) && (lstrcmpni(asInstance, L"-GuiMacro", 9) != 0));

		const CEStr lsArg(L"/GuiMacro:", asInstance);
		ArgGuiMacro(lsArg, inst);
	}

	GuiMacroFlags flags = GuiMacroFlags::None;
	if (bsResult == nullptr)
		flags = GuiMacroFlags::SetEnvVar;
	else if (bsResult == static_cast<BSTR*>(INVALID_HANDLE_VALUE))
		bsResult = nullptr;

	const int iRc = DoGuiMacro(asMacro, inst, flags, bsResult);
	return iRc;
}
