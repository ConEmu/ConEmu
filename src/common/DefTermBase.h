
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


#pragma once

#if !defined(DEBUGSTRDEFTERM)
#define DEBUGSTRDEFTERM(s)
#undef USEDEBUGSTRDEFTERM
#else
#define USEDEBUGSTRDEFTERM
#endif

#include "Common.h"
#include "Memory.h"
#include "CEStr.h"
#include "MArray.h"
#include "MSectionSimple.h"
#include "ConEmuCheck.h"
#include "MModule.h"
#include "MProcess.h"
#include "MProcessBits.h"
#include "WThreads.h"
#include "../ConEmuCD/ExitCodes.h"

#include <cstdint>
#include <chrono>
#include <tuple>
#include <windows.h>


#define DEF_TERM_ALIVE_CHECK_TIMEOUT 750
#define DEF_TERM_ALIVE_RECHECK_TIMEOUT 1500
#define DEF_TERM_INSTALL_TIMEOUT std::chrono::seconds(30)

// Controls if console server shows "Press Enter to close console" after quick termination of started app
enum class TerminalConfirmClose : uint32_t
{
	Auto = 0,
	Always = 1, // implies " /CONFIRM"
	Never = 2, // implies " /NOCINFIRM"
};

/* *********** Default terminal settings *********** */
struct CEDefTermOpt
{
public:
	BOOL     bUseDefaultTerminal = false;
	BOOL     bAggressive = false;
	BOOL     bNoInjects = false; // " /NOINJECT"
	BOOL     bNewWindow = false; // " /GHWND=NEW"
	BOOL     bDebugLog = false;
	TerminalConfirmClose nDefaultTerminalConfirmClose = {};
	ConEmu::ConsoleFlags nConsoleFlags = {}; // Used for populating m_SrvMapping in ShellProcessor
	wchar_t* pszConEmuExe = nullptr; // The full path to the ConEmu.exe
	wchar_t* pszConEmuBaseDir = nullptr; // %ConEmuBaseDir%
	wchar_t* pszCfgFile = nullptr; // " /LoadCfgFile "...""
	wchar_t* pszConfigName = nullptr; // " /CONFIG "...""
	wchar_t* pszzHookedApps = nullptr; // ASCIIZZ
	bool     bExternalPointers = false;
	// Last checking time
	FILETIME ftLastCheck = {};
private:
	// use dynamic link, because code is using in ConEmuHk too
	MModule hAdvApi = {};
public:
	CEDefTermOpt()
	{
	}

	CEDefTermOpt(const CEDefTermOpt&) = delete;
	CEDefTermOpt(CEDefTermOpt&&) = delete;
	CEDefTermOpt& operator=(const CEDefTermOpt&) = delete;
	CEDefTermOpt& operator=(CEDefTermOpt&&) = delete;

	~CEDefTermOpt()
	{
		if (!bExternalPointers)
		{
			if (pszConEmuExe)
				free(pszConEmuExe);
			if (pszConEmuBaseDir)
				free(pszConEmuBaseDir);
			if (pszCfgFile)
				free(pszCfgFile);
			if (pszConfigName)
				free(pszConfigName);
			if (pszzHookedApps)
				free(pszzHookedApps);
		}
	}

public:
	enum class SerializeMode
	{
		Load,
		Save,
	};

	bool Serialize(const SerializeMode mode = SerializeMode::Load)
	{
		if (!this)
			return false;

		// use dynamic link, because code is using in ConEmuHk too
		typedef LONG (WINAPI* RegCreateKeyEx_t)(HKEY hKey, LPCWSTR lpSubKey, DWORD Reserved, LPTSTR lpClass, DWORD dwOptions, REGSAM samDesired, LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult, LPDWORD lpdwDisposition);
		typedef LONG (WINAPI* RegCloseKey_t)(HKEY hKey);
		typedef LONG (WINAPI* RegQueryValueEx_t)(HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData);
		typedef LONG (WINAPI* RegSetValueEx_t)(HKEY hKey, LPCWSTR lpValueName, DWORD Reserved, DWORD dwType, const BYTE *lpData, DWORD cbData);
		typedef LONG (WINAPI* RegQueryInfoKey_t)(HKEY hKey, LPWSTR lpClass, LPDWORD lpcClass, LPDWORD lpReserved, LPDWORD lpcSubKeys, LPDWORD lpcMaxSubKeyLen, LPDWORD lpcMaxClassLen, LPDWORD lpcValues, LPDWORD lpcMaxValueNameLen, LPDWORD lpcMaxValueLen, LPDWORD lpcbSecurityDescriptor, PFILETIME lpftLastWriteTime);
		static RegCreateKeyEx_t regCreateKeyEx = nullptr;
		static RegCloseKey_t regCloseKey = nullptr;
		static RegQueryValueEx_t regQueryValueEx = nullptr;
		static RegSetValueEx_t regSetValueEx = nullptr;
		static RegQueryInfoKey_t regQueryInfoKey = nullptr;
		if (!hAdvApi)
		{
			if (!hAdvApi.Load(L"advapi32.dll"))
				return false;
		}
		if (!regCreateKeyEx || !regCloseKey || !regQueryValueEx || !regSetValueEx || !regQueryInfoKey)
		{
			if (!hAdvApi.GetProcAddress("RegCreateKeyExW", regCreateKeyEx)
				|| !hAdvApi.GetProcAddress("RegCloseKey", regCloseKey)
				|| !hAdvApi.GetProcAddress("RegQueryValueExW", regQueryValueEx)
				|| !hAdvApi.GetProcAddress("RegSetValueExW", regSetValueEx)
				|| !hAdvApi.GetProcAddress("RegQueryInfoKeyW", regQueryInfoKey))
				return false;
		}

		const LPCWSTR sSubKey = /*[HKCU]*/ L"Software\\ConEmu";
		HKEY hk = nullptr;
		const DWORD samDesired = (mode == SerializeMode::Save) ? KEY_WRITE : KEY_READ;
		DWORD dwValue = 0, dwSize, dwType;
		LONG lRc = regCreateKeyEx(HKEY_CURRENT_USER, sSubKey, 0, nullptr, 0, samDesired, nullptr, &hk, &dwValue);
		if ((lRc != 0) || (hk == nullptr))
		{
			return false;
		}

		if (mode == SerializeMode::Load)
		{
			FILETIME ftKey = {};
			lRc = regQueryInfoKey(hk, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &ftKey);
			if ((lRc == 0)
				&& (ftLastCheck.dwHighDateTime || ftLastCheck.dwLowDateTime)
				&& (memcmp(&ftKey, &ftLastCheck, sizeof(ftLastCheck)) == 0))
			{
				// Nothing was changed, no need to reload
				regCloseKey(hk);
				return (bUseDefaultTerminal != 0);
			}
			ftLastCheck = ftKey;
		}

		_ASSERTE(sizeof(nConsoleFlags) == sizeof(DWORD));
		_ASSERTE(sizeof(bUseDefaultTerminal) == sizeof(DWORD));
		_ASSERTE(sizeof(nDefaultTerminalConfirmClose) == sizeof(DWORD));

		struct strOpt {
			LPCWSTR Name; void* ptr; DWORD Size; DWORD Type;
		} optionList[] = {
			{L"DefTerm-Enabled",   &bUseDefaultTerminal, sizeof(bUseDefaultTerminal), REG_DWORD},
			// ReSharper disable once StringLiteralTypo
			{L"DefTerm-Agressive", &bAggressive, sizeof(bAggressive), REG_DWORD},
			{L"DefTerm-NoInjects", &bNoInjects, sizeof(bNoInjects), REG_DWORD},
			{L"DefTerm-NewWindow", &bNewWindow, sizeof(bNewWindow), REG_DWORD},
			{L"DefTerm-DebugLog",  &bDebugLog,  sizeof(bDebugLog),  REG_DWORD},
			{L"DefTerm-Confirm",   &nDefaultTerminalConfirmClose, sizeof(nDefaultTerminalConfirmClose), REG_DWORD},
			{L"DefTerm-Flags",     &nConsoleFlags, sizeof(nConsoleFlags), REG_DWORD},
			{L"DefTerm-ConEmuExe", &pszConEmuExe, 0, REG_SZ},
			{L"DefTerm-BaseDir",   &pszConEmuBaseDir, 0, REG_SZ},
			{L"DefTerm-CfgFile",   &pszCfgFile, 0, REG_SZ},
			{L"DefTerm-Config",    &pszConfigName, 0, REG_SZ},
			{L"DefTerm-AppList",   &pszzHookedApps, 0, REG_MULTI_SZ},
		};

		bool bRc = false;

		if (mode == SerializeMode::Load)
		{
			const bool wasExternalPointers = this->bExternalPointers;
			// Load
			for (auto& opt : optionList)
			{
				if (opt.Size)
				{
					// BYTE or BOOL
					dwSize = sizeof(dwValue);
					lRc = regQueryValueEx(hk, opt.Name, nullptr, &dwType, reinterpret_cast<LPBYTE>(&dwValue), &dwSize);

					if (lRc != 0)
						dwValue = 0;

					if (opt.Size == sizeof(DWORD))
						*static_cast<LPDWORD>(opt.ptr) = dwValue;
					else if (opt.Size == sizeof(BYTE))
						*static_cast<LPBYTE>(opt.ptr) = LOBYTE(dwValue);
				}
				else // String or MSZ
				{
					wchar_t* pszOld = *static_cast<wchar_t**>(opt.ptr);
					wchar_t* pszNew = nullptr;

					dwSize = 0;
					lRc = regQueryValueEx(hk, opt.Name, nullptr, &dwType, nullptr, &dwSize);
					if ((lRc == 0) && dwSize && (dwType == REG_SZ || dwType == REG_MULTI_SZ))
					{
						pszNew = static_cast<wchar_t*>(calloc(dwSize + 2 * sizeof(wchar_t), 1));
						if (pszNew)
						{
							lRc = regQueryValueEx(hk, opt.Name, nullptr, &dwType, reinterpret_cast<LPBYTE>(pszNew), &dwSize);
							// Do not need to store empty strings
							if (!*pszNew)
							{
								free(pszNew);
								pszNew = nullptr;
							}
						}
					}

					*static_cast<wchar_t**>(opt.ptr) = pszNew;
					if (!wasExternalPointers)
					{
						SafeFree(pszOld);
					}
					std::ignore = lRc;
				}
			}

			this->bExternalPointers = false;
			bRc = (bUseDefaultTerminal != 0);
		}
		#ifndef CONEMU_MINIMAL
		else
		{
			// Save
			bRc = true;

			for (const auto& opt : optionList)
			{
				if (opt.Size)
				{
					// BYTE or BOOL
					dwSize = opt.Size;
					lRc = regSetValueEx(hk, opt.Name, 0, opt.Type, static_cast<const BYTE*>(opt.ptr), dwSize);
				}
				else if (opt.Type == REG_SZ)
				{
					LPCWSTR pSZ = *static_cast<wchar_t**>(opt.ptr);
					if (!pSZ) pSZ = L"";
					dwSize = (lstrlen(pSZ) + 1) * sizeof(wchar_t);
					lRc = regSetValueEx(hk, opt.Name, 0, REG_SZ, reinterpret_cast<const BYTE*>(pSZ), dwSize);
				}
				else if (opt.Type == REG_MULTI_SZ)
				{
					LPCWSTR pMSZ = *static_cast<wchar_t**>(opt.ptr);
					if (!pMSZ) pMSZ = L"\0";
					dwSize = sizeof(wchar_t);
					LPCWSTR ptr = pMSZ;
					while (*ptr)
					{
						const int nLen = lstrlen(ptr);
						if (nLen < 1)
							break;
						dwSize += (nLen + 1) * sizeof(wchar_t);
						ptr += (nLen + 1);
					}
					lRc = regSetValueEx(hk, opt.Name, 0, REG_MULTI_SZ, reinterpret_cast<const BYTE*>(pMSZ), dwSize);
				}
				if (lRc != 0)
					bRc = false;
			}
		}
		#endif

		regCloseKey(hk);
		return bRc;
	}
};
/* *********** Default terminal settings *********** */

class CDefTermBase
{
protected:
	struct ProcessInfo
	{
		HWND      hWnd;
		HANDLE    hProcess;
		DWORD     nPID;
		DWORD     nHookResult;
		std::chrono::steady_clock::time_point nHookTick; // informational field
	};
	struct ThreadArg
	{
		CDefTermBase* pTerm;
		HWND hFore;
		DWORD nForePID;
	};
	struct ThreadHandle
	{
		HANDLE hThread;
		DWORD nThreadId, nCreatorThreadId;
	};

private:
	bool   mb_ConEmuGui = false;
	bool   mb_ReadyToHook = false;
	bool   mb_PostCreatedThread = false;
	bool   mb_Initialized = false;
	HWND   mh_LastWnd = nullptr;
	HWND   mh_LastIgnoredWnd = nullptr;
	HWND   mh_LastCall = nullptr;
	MArray<ThreadHandle> m_Threads = {};
	MSectionSimple* mcs_Threads = nullptr;
	MArray<ProcessInfo> m_Processed = {};
	MSectionSimple* mcs_Processes = nullptr;
protected:
	HANDLE mh_PostThread = nullptr;
	DWORD  mn_PostThreadId = 0;
	/// @brief Blocks main check routine and settings reloading
	MSectionSimple* mcs = nullptr;
	bool   mb_Termination = false;
protected:
	// Shell may not like injecting immediately after start-up
	// If so, set (mb_ExplorerHookAllowed = false) in constructor
	bool   mb_ExplorerHookAllowed = false;

protected:
	CEDefTermOpt m_Opt;
public:
	const CEDefTermOpt& GetOpt() const
	{
		return m_Opt;
	};

protected:
	virtual CDefTermBase* GetInterface() = 0;
	virtual bool isDefaultTerminalAllowed(bool bDontCheckName = false) = 0; // !(gpConEmu->DisableSetDefTerm || !gpSet->isSetDefaultTerminal)
	virtual int  DisplayLastError(LPCWSTR asLabel, DWORD dwError=0, DWORD dwMsgFlags=0, LPCWSTR asTitle=nullptr, HWND hParent=nullptr) = 0;
	virtual void ShowTrayIconError(LPCWSTR asErrText) = 0; // Icon.ShowTrayIcon(asErrText, tsa_Default_Term);
	virtual void ReloadSettings() = 0; // Copy from gpSet or load from [HKCU]

protected:
	virtual void StopHookers()
	{
		mb_Termination = true;

		ClearThreads(true);

		ClearProcessed(true);

		SafeDelete(mcs);
		SafeDelete(mcs_Threads);
		SafeDelete(mcs_Processes);
	}

public:
	CDefTermBase(bool bConEmuGui)
		: mb_ConEmuGui(bConEmuGui)
	{
		mcs = new MSectionSimple(true);
		mcs_Threads = new MSectionSimple(true);
		mcs_Processes = new MSectionSimple(true);
	}

	CDefTermBase(const CDefTermBase&) = delete;
	CDefTermBase(CDefTermBase&&) = delete;
	CDefTermBase& operator=(const CDefTermBase&) = delete;
	CDefTermBase& operator=(CDefTermBase&&) = delete;

	virtual ~CDefTermBase()
	{
		// StopHookers(); -- Have to be called in the ancestor destructor!
		_ASSERTE(mcs == nullptr);
		_ASSERTE(mcs_Threads == nullptr);
		_ASSERTE(mcs_Processes == nullptr);
	}

public:
	bool IsReady()
	{
		return (this && mb_Initialized && mb_ReadyToHook);
	}

	static bool IsShellTrayWindowClass(LPCWSTR asClassName)
	{
		const bool bShellTrayWnd = (asClassName != nullptr)
			&& ((lstrcmp(asClassName, L"Shell_TrayWnd") == 0)
				);
		return bShellTrayWnd;
	}

	static bool IsExplorerWindowClass(LPCWSTR asClassName)
	{
		const bool bShellWnd = (asClassName != nullptr)
			&& ((lstrcmp(asClassName, L"CabinetWClass") == 0)
				|| (lstrcmp(asClassName, L"ExploreWClass") == 0)
				);
		return bShellWnd;
	}

	static bool IsWindowResponsive(HWND hFore)
	{
		DWORD_PTR dwResult = 0;

		// To be sure that application is responsive (started successfully and is not hung)
		// However, that does not guarantee that explorer.exe will be hooked properly
		// During several restarts of explorer.exe from TaskMgr - sometimes it hungs...
		// So, that is the last change but not sufficient (must be handled in GUI too)
		// -- SMTO_ERRORONEXIT exist in Vista only
		const LRESULT lMsgRc = SendMessageTimeout(hFore, WM_NULL, 0, 0, SMTO_NORMAL, DEF_TERM_ALIVE_CHECK_TIMEOUT, &dwResult);
		return (lMsgRc != 0);
	}

	/// @brief Checks if the process described by hFore and ForePID shall be hooked to support DefTerm.
	/// @param hFore HWND of the top window (found by GetForegroundWindow)
	/// @param nForePID PID of the hFore
	/// @param bSkipShell If true than skip Windows Taskbar process
	/// @param pPrc [OUT] May point to a buffer receiving PROCESSENTRY32
	/// @param pszClass [OUT] May point to a buffer receiving the class name of the hFore
	/// @return true if we should inject ConEmuHk into the nForePID process
	bool IsAppMonitored(HWND hFore, const DWORD nForePID, const bool bSkipShell = true, PROCESSENTRY32* pPrc = nullptr, CEStr* pszClass = nullptr)
	{
		// Should process all checks, storing successful/unsuccessful state,
		// to not doing the same checks again on subsequent calls of CheckForeground.
		// It's called from CConEmuMain::RecheckForegroundWindow

		wchar_t szClass[MAX_PATH] = L"";
		PROCESSENTRY32 prc = {};
		bool bMonitored = false;
		bool bShellTrayWnd = false; // task bar
		bool bShellWnd = false; // one of explorer windows (folder browsers)
		wchar_t szInfo[MAX_PATH + 80] = L"";

		if (!IsAppAllowed(hFore, nForePID))
		{
			swprintf_c(szInfo, L"DefTerm::CheckForeground x%08X PID=%u is rejected by ConEmu",
				LODWORD(reinterpret_cast<DWORD_PTR>(hFore)), nForePID);
			LogHookingStatus(nForePID, szInfo);
			goto wrap;
		}

		// Check window class
		if (GetClassName(hFore, szClass, countof(szClass)))
		{
			bShellTrayWnd = IsShellTrayWindowClass(szClass);
			bShellWnd = IsExplorerWindowClass(szClass);

			#ifdef USEDEBUGSTRDEFTERM
			swprintf_c(szInfo, L"DefTerm::CheckForeground x%08X PID=%u %u %u '%s'",
				LODWORD(reinterpret_cast<DWORD_PTR>(hFore)), nForePID, static_cast<UINT>(bShellTrayWnd), static_cast<UINT>(bShellWnd), szClass);
			DEBUGSTRDEFTERM(szInfo);
			#endif

			if ((lstrcmp(szClass, VirtualConsoleClass) == 0)
				//|| (lstrcmp(szClass, L"#32770") == 0) // Ignore dialogs // -- Process dialogs too (Application may be dialog-based)
				|| ((bSkipShell || !mb_ExplorerHookAllowed) && bShellTrayWnd) // Do not hook explorer windows?
				|| IsConsoleClass(szClass))
			{
				#ifdef USEDEBUGSTRDEFTERM
				swprintf_c(szInfo, L"DefTerm::CheckForeground x%08X PID=%u ignored by class",
					LODWORD(reinterpret_cast<DWORD_PTR>(hFore)), nForePID);
				DEBUGSTRDEFTERM(szInfo);
				#endif
				goto wrap;
			}
		}

		// May be hooked by class name? e.g. "TaskManagerWindow"
		bMonitored = IsAppNameMonitored(szClass);
		if (!bMonitored)
		{
			// Then check by process exe name
			if (!GetProcessInfo(nForePID, prc))
			{
				#ifdef USEDEBUGSTRDEFTERM
				swprintf_c(szInfo, L"DefTerm::CheckForeground x%08X PID=%u skipped, can't get process name",
					LODWORD(reinterpret_cast<DWORD_PTR>(hFore)), nForePID);
				DEBUGSTRDEFTERM(szInfo);
				#endif
				goto wrap;
			}

			CharLowerBuff(prc.szExeFile, lstrlen(prc.szExeFile));

			if (lstrcmp(prc.szExeFile, L"csrss.exe") == 0)
			{
				// This is "System" process and may not be hooked
				#ifdef USEDEBUGSTRDEFTERM
				swprintf_c(szInfo, L"DefTerm::CheckForeground x%08X PID=%u skipped, csrss.exe",
					LODWORD(reinterpret_cast<DWORD_PTR>(hFore)), nForePID);
				DEBUGSTRDEFTERM(szInfo);
				#endif
				goto wrap;
			}

			// Is it in monitored applications?
			bMonitored = IsAppNameMonitored(prc.szExeFile);
		}
	wrap:
		// And how it is?
		if (!bMonitored)
		{
			mh_LastIgnoredWnd = hFore;
			#ifdef USEDEBUGSTRDEFTERM
			swprintf_c(szInfo, L"DefTerm::CheckForeground x%08X PID=%u skipped, app is not monitored",
				LODWORD(reinterpret_cast<DWORD_PTR>(hFore)), nForePID);
			DEBUGSTRDEFTERM(szInfo);
			#endif
		}
		else
		{
			// _ASSERTE(FALSE && "Continue to hook DefTerm allowed process");
		}

		if (pPrc && prc.dwSize)
			memmove(pPrc, &prc, sizeof(*pPrc));
		if (pszClass)
			pszClass->Set(szClass);
		std::ignore = bShellWnd;
		return bMonitored;
	}

	bool CheckForeground(HWND hFore, const DWORD nForePid, const bool bRunInThread = true, const bool bSkipShell = true)
	{
		if (mb_Termination)
		{
			return false;
		}

		if (!isDefaultTerminalAllowed())
		{
#ifdef USEDEBUGSTRDEFTERM
			static bool bNotified = false;
			if (!bNotified)
			{
				bNotified = true;
				DEBUGSTRDEFTERM(L"!!! DefTerm::CheckForeground skipped, !isDefaultTerminalAllowed !!!");
			}
#endif
			return false;
		}

		// if we were not yet initialized
		if (!mb_ReadyToHook
			// or arguments error
			|| (!hFore || !nForePid)
			// or it's our process
			|| (nForePid == GetCurrentProcessId()))
		{
			// Do nothing, exit!
			DEBUGSTRDEFTERM(L"!!! DefTerm::CheckForeground skipped, uninitialized (mb_ReadyToHook, hFore, nForePID) !!!");
			_ASSERTE(mb_ReadyToHook && "Must be initialized");
			_ASSERTE(hFore && nForePid);
			return false;
		}

		if (hFore == mh_LastWnd || hFore == mh_LastIgnoredWnd)
		{
			DEBUGSTRDEFTERM(L"DefTerm::CheckForeground skipped, hFore was already checked");
			return (hFore == mh_LastWnd);
		}

		// GO

		bool lbRc = false;
		int  iHookerRc = -1;
		MSectionLockSimple guard;
		bool lbConHostLocked = false;
		DWORD nResult = 0;
		CEStr szClass;
		PROCESSENTRY32 prc = {};
		bool bMonitored = false;
		bool bNotified = false;
		bool alreadyProcessed = false;
		HANDLE hProcess = nullptr;
		DWORD nErrCode = 0;
		wchar_t szInfo[MAX_PATH + 80];


		if (bRunInThread && (hFore == mh_LastCall))
		{
			// Just exit. It's a check for rapid background calls.
			DEBUGSTRDEFTERM(L"DefTerm::CheckForeground skipped, already bRunInThread");
			goto wrap;
		}
		mh_LastCall = hFore;

		if (bRunInThread)
		{
			AutoClearThreads();

			HANDLE hPostThread = nullptr; DWORD nThreadId = 0;
			ThreadArg* pArg = static_cast<ThreadArg*>(malloc(sizeof(ThreadArg)));
			if (!pArg)
			{
				_ASSERTE(pArg);  // -V571
				goto wrap;
			}
			pArg->pTerm = GetInterface();
			pArg->hFore = hFore;
			pArg->nForePID = nForePid;

			DEBUGSTRDEFTERM(L"DefTerm::CheckForeground creating background thread PostCheckThread");

			hPostThread = apiCreateThread(PostCheckThread, pArg, &nThreadId, "DefTerm::PostCheckThread");
			_ASSERTE(hPostThread != nullptr);
			if (hPostThread)
			{
				MSectionLockSimple threadGuard;
				threadGuard.Lock(mcs_Threads);
				const ThreadHandle th = { hPostThread, nThreadId, GetCurrentThreadId() };
				m_Threads.push_back(th);

#ifdef USEDEBUGSTRDEFTERM
				swprintf_c(szInfo, L"DefTerm::CheckForeground created TID=%u", nThreadId);
				DEBUGSTRDEFTERM(szInfo);
#endif
			}

			lbRc = (hPostThread != nullptr); // return OK?
			goto wrap;
		}

		if (mb_Termination)
			goto wrap;

		// To be sure that application is responsive (started successfully and is not hung)
		// However, that does not guarantee that explorer.exe will be hooked properly
		// During several restarts of explorer.exe from TaskMgr - sometimes it hungs...
		// So, that is the last change but not sufficient (must be handled in GUI too)
		// -- SMTO_ERRORONEXIT exist in Vista only
		if (!IsWindowResponsive(hFore))
		{
			// That app is not ready for hooking
			if (mh_LastCall == hFore)
				mh_LastCall = nullptr;
#ifdef USEDEBUGSTRDEFTERM
			swprintf_c(szInfo, L"!!! DefTerm::CheckForeground skipped, x%08X is non-responsive !!!",
				LODWORD(reinterpret_cast<DWORD_PTR>(hFore)));
			DEBUGSTRDEFTERM(szInfo);
#endif
			goto wrap;
		}

		guard.Lock(mcs);

		// Clear dead processes and windows
		ClearProcessed(false);

		if (mb_Termination)
			goto wrap;

		// Check if app is monitored by window class and process name
		bMonitored = IsAppMonitored(hFore, nForePid, bSkipShell, &prc, &szClass);

		if (!bMonitored)
		{
			mh_LastIgnoredWnd = hFore;
			goto wrap;
		}

		// That is hooked executable/classname
		// But may be it was processed already?
		{
			MSectionLockSimple processGuard;
			processGuard.Lock(mcs_Processes);
			for (INT_PTR i = m_Processed.size(); i-- && !mb_Termination;)
			{
				if (m_Processed[i].nPID == nForePid)
				{
					alreadyProcessed = true;
					#ifdef USEDEBUGSTRDEFTERM
					swprintf_c(szInfo, L"DefTerm::CheckForeground x%08X PID=%u skipped, already processed",
						LODWORD(reinterpret_cast<DWORD_PTR>(hFore)), nForePid);
					DEBUGSTRDEFTERM(szInfo);
					#endif
					break; // already hooked/processed
			}
		}
	}

		// May be hooked already?
		if (alreadyProcessed || mb_Termination)
		{
			mh_LastIgnoredWnd = hFore;
			lbRc = false;
			goto wrap;
		}

		_ASSERTE(isDefaultTerminalAllowed());

		swprintf_c(szInfo, L"CheckForeground x%08X <<== trying to set hooks", LODWORD(hFore));
		LogHookingStatus(nForePid, szInfo);

		bNotified = NotifyHookingStatus(nForePid, prc.szExeFile[0] ? prc.szExeFile : szClass);

		ConhostLocker(true, lbConHostLocked);

		_ASSERTE(hProcess == nullptr);
		iHookerRc = StartDefTermHooker(nForePid, hProcess, nResult, m_Opt.pszConEmuBaseDir, nErrCode);

		ConhostLocker(false, lbConHostLocked);

		swprintf_c(szInfo, L"CheckForeground x%08X <<== nResult=%i iHookerRc=%i", LODWORD(hFore), nResult, iHookerRc);
		LogHookingStatus(nForePid, szInfo);

		if (iHookerRc != 0)
		{
			mh_LastIgnoredWnd = hFore;
			if (iHookerRc == -3)
			{
				DisplayLastError(L"Failed to start hooking application!\nDefault terminal feature will not be available!", nErrCode);
			}
			goto wrap;
		}

		// Always store to m_Processed
		{
			ProcessInfo inf = {};
			inf.hWnd = hFore;
			inf.hProcess = hProcess;
			hProcess = nullptr; // no need to close the handle (if any), it's saved in the array
			inf.nPID = nForePid;
			inf.nHookTick = std::chrono::steady_clock::now();
			inf.nHookResult = nResult;

			StoreProcessed(inf);
		}

		// And what?
		if ((nResult == static_cast<UINT>(CERR_HOOKS_WAS_SET)) || (nResult == static_cast<UINT>(CERR_HOOKS_WAS_ALREADY_SET)))
		{
			mh_LastWnd = hFore;
			lbRc = true;
			goto wrap;
		}
		// Failed, remember this
		if (hProcess)
		{
			CloseHandle(hProcess);
		}
		mh_LastIgnoredWnd = hFore;
		_ASSERTE(lbRc == false);
	wrap:
		guard.Unlock();
		if (bNotified)
		{
			NotifyHookingStatus(0, nullptr);
			LogHookingStatus(nForePid, L"CheckForeground finished");
		}
		return lbRc;
	}

public:
	void OnHookedListChanged()
	{
		if (!mb_Initialized)
			return;
		if (!isDefaultTerminalAllowed(true))
			return;

		MSectionLockSimple guard;
		guard.Lock(mcs);
		ClearProcessed(true);
		guard.Unlock();

		// If there were no Explorer in the allowed list, but now it was added
		// than check the Shell process (TaskBar)
		CheckShellWindow();
	}

public:
	int StartDefTermHooker(DWORD nPID, HANDLE hDefProcess = nullptr)
	{
		HANDLE hProcess = nullptr;
		DWORD nResult = 0, nErrCode = 0;
		MSectionLockSimple guard;
		guard.Lock(mcs);

		// We get here after CreateProcess (ConEmuHk) or from VisualStudio for "*.vshost.exe"
		// Here we should be able to open a descriptor and set hooks

		if (hDefProcess)
		{
			// This handle may come from CreateProcess (ConEmuHk), need to make a copy of it
			DuplicateHandle(GetCurrentProcess(), hDefProcess, GetCurrentProcess(), &hProcess, 0, FALSE, DUPLICATE_SAME_ACCESS);
		}

		// Always store to m_Processed
		{
			ProcessInfo inf = {};
			inf.hProcess = hProcess; // no need to close hProcess, handle is stored in the array
			inf.nPID = nPID;
			inf.nHookTick = std::chrono::steady_clock::now();
			inf.nHookResult = STILL_ACTIVE;

			StoreProcessed(inf);
		}

		// Because we must unlock the section before StartDefTermHooker to avoid dead locks in VS
		guard.Unlock();

		// Do the job
		const int iRc = StartDefTermHooker(nPID, hProcess, nResult, m_Opt.pszConEmuBaseDir, nErrCode);

		return iRc;
	}

private:
	int StartDefTermHooker(DWORD nForePID, HANDLE& hProcess, DWORD& nResult, LPCWSTR asConEmuBaseDir, DWORD& nErrCode)
	{
		if (mb_Termination)
			return -1;

		int iRc = -1;
		wchar_t szCmdLine[MAX_PATH * 2] = L"";
		wchar_t szConTitle[32] = L"ConEmu";
		LPCWSTR pszSrvExe = nullptr;
		int nBits = 0;
		PROCESS_INFORMATION pi = {};
		STARTUPINFO si = {};
		si.cb = sizeof(si);
		BOOL bStarted = FALSE;
		HANDLE hMutex = nullptr;
		wchar_t szMutexName[64];
		DWORD nMutexCreated = 0;
		wchar_t szInfo[120];
		std::chrono::steady_clock::time_point nWaitStartTick{};
		std::chrono::steady_clock::duration nWaitDelta{};

		nErrCode = 0;

		msprintf(szMutexName, countof(szMutexName), CEDEFAULTTERMBEGIN, nForePID);
		hMutex = CreateMutex(LocalSecurity(), TRUE, szMutexName);
		nMutexCreated = hMutex ? GetLastError() : 0;

		// If was not opened yet
		if (hProcess == nullptr)
		{
			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION|SYNCHRONIZE, FALSE, nForePID);
		}

		if (!hProcess)
		{
			// Failed to hook
			nErrCode = GetLastError();
			swprintf_c(szInfo, L"OpenProcess fails, code=%u", nErrCode);
			LogHookingStatus(nForePID, szInfo);

			if (nErrCode == ERROR_ACCESS_DENIED)
			{
				TODO("Let's try to run ConEmuC[64] as admin");
			}
			goto wrap;
		}

		if (nMutexCreated == ERROR_ALREADY_EXISTS)
		{
			// Hooking was started from another process (most probably in aggressive mode)
			iRc = 0;
			nResult = CERR_HOOKS_WAS_ALREADY_SET;
			swprintf_c(szInfo, L"Mutex already exists");
			LogHookingStatus(nForePID, szInfo);
			goto wrap;
		}

		// Need to be hooked
		nBits = GetProcessBits(nForePID, hProcess);
		switch (nBits)
		{
		case 32:
			pszSrvExe = L"ConEmuC.exe";
			break;
		case 64:
			pszSrvExe = L"ConEmuC64.exe";
			break;
		default:
			pszSrvExe = nullptr;
		}
		if (!pszSrvExe)
		{
			// Unsupported bitness?
			CloseHandle(hProcess);
			iRc = -2;
			swprintf_c(szInfo, L"Unsupported process bitness (%i)", nBits);
			LogHookingStatus(nForePID, szInfo);
			goto wrap;
		}
		msprintf(szCmdLine, countof(szCmdLine), L"\"%s\\%s\"%s /DEFTRM=%u",
			asConEmuBaseDir, pszSrvExe,
			isLogging() ? L" /LOG" : L"",
			nForePID);

		if (mb_Termination)
		{
			iRc = -1;
			goto wrap;
		}

		// Run hooker
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.lpTitle = szConTitle;
		LogHookingStatus(nForePID, szCmdLine);
		bStarted = CreateProcess(nullptr, szCmdLine, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi);
		if (!bStarted)
		{
			//DisplayLastError(L"Failed to start hooking application!\nDefault terminal feature will not be available!");
			nErrCode = GetLastError();
			CloseHandle(hProcess);
			iRc = -3;
			swprintf_c(szInfo, L"Fails to start service process, code=%u", nErrCode);
			LogHookingStatus(nForePID, szInfo);
			goto wrap;
		}
		CloseHandle(pi.hThread);

		swprintf_c(szInfo, L"Service process PID=%u started", pi.dwProcessId);
		LogHookingStatus(nForePID, szInfo);

		// Waiting for result, to avoid multiple hooks processed (due to closed mutex)
		// Within VisualStudio and others don't do INFINITE wait to avoid dead locks
		nWaitStartTick = std::chrono::steady_clock::now();
		while (!mb_Termination)
		{
			if (WaitForSingleObject(pi.hProcess, 100) == WAIT_OBJECT_0)
				break;
			nWaitDelta = std::chrono::steady_clock::now() - nWaitStartTick;
			if (!mb_ConEmuGui && (nWaitDelta >= DEF_TERM_INSTALL_TIMEOUT))
				break;
		}
		if (!GetExitCodeProcess(pi.hProcess, &nResult))
			nResult = static_cast<DWORD>(-1);
		swprintf_c(szInfo, L"Service process PID=%u finished with code=%u", pi.dwProcessId, nResult);
		LogHookingStatus(nForePID, szInfo);

		CloseHandle(pi.hProcess);

		iRc = 0;
	wrap:
		if (hMutex)
		{
			CloseHandle(hMutex);
		}
		return iRc;
	}

protected:
	void Initialize(bool bWaitForReady /*= false*/, bool bShowErrors /*= false*/, bool bNoThreading /*= false*/)
	{
		// Once
		if (!mb_Initialized)
		{
			mb_Initialized = true;
			ReloadSettings();
		}

		if (!isDefaultTerminalAllowed())
		{
			_ASSERTE(bWaitForReady == false);
			if (bShowErrors && m_Opt.bUseDefaultTerminal)
			{
				DisplayLastError(L"Default terminal feature was blocked\n"
					L"with '/NoDefTerm' ConEmu command line argument!", -1);
			}
			return;
		}

#ifdef _DEBUG
		const DWORD nDbgWait = mh_PostThread ? WaitForSingleObject(mh_PostThread, 0) : WAIT_OBJECT_0;
		DWORD nDbgExitCode = static_cast<DWORD>(-1);
		if (mh_PostThread)
			GetExitCodeThread(mh_PostThread, &nDbgExitCode);
		_ASSERTE(nDbgWait == WAIT_OBJECT_0);
#endif

		if (mb_PostCreatedThread)
		{
			if (!bShowErrors)
				return;
			ClearThreads(false);
			if (m_Threads.size() > 0)
			{
				ShowTrayIconError(L"Previous Default Terminal setup cycle was not finished yet");
				return;
			}
		}

		PreCreateThread();

		mb_ReadyToHook = true;

		// Even if it is not a ConEmu.exe we need to start thread,
		// aggressive mode may be turned on by user in any moment
		// We need just slow down our thread as possible...

		// The process takes time, to not block the main thread we start it in background
		mb_PostCreatedThread = true;
		DWORD  nWait = WAIT_FAILED;
		// ReSharper disable once CppLocalVariableMayBeConst
		HANDLE hPostThread = apiCreateThread(PostCreatedThread, GetInterface(), &mn_PostThreadId, "DefTerm::PostCreatedThread");

		if (hPostThread)
		{
			if (bWaitForReady)
			{
				// Wait for 30 seconds or infinite if bNoThreading specified (used for ConEmu.exe /SetDefTerm /Exit)
				const DWORD nWaitTimeout = bNoThreading
					? INFINITE
					: LODWORD(std::chrono::duration_cast<std::chrono::milliseconds>(DEF_TERM_INSTALL_TIMEOUT).count());
				// Do wait
				SetCursor(LoadCursor(nullptr, IDC_WAIT));
				const auto nStart = std::chrono::steady_clock::now();
				nWait = WaitForSingleObject(hPostThread, nWaitTimeout);
				SetCursor(LoadCursor(nullptr, IDC_ARROW));
				const auto nDuration = std::chrono::steady_clock::now() - nStart;
				if (nWait == WAIT_OBJECT_0)
				{
					CloseHandle(hPostThread);
					return;
				}
				//_ASSERTE(nWait == WAIT_OBJECT_0);
				DisplayLastError(L"PostCreatedThread was not finished in 30 seconds");
				std::ignore = nDuration;
			}
			else
			{
				mh_PostThread = hPostThread;
			}

			MSectionLockSimple threadGuard;
			threadGuard.Lock(mcs_Threads);
			const ThreadHandle th = { hPostThread, mn_PostThreadId, GetCurrentThreadId() };
			m_Threads.push_back(th);
		}
		else
		{
			if (bShowErrors)
			{
				DisplayLastError(L"Failed to start PostCreatedThread");
			}
			_ASSERTE(hPostThread != nullptr);
			mb_PostCreatedThread = false;
		}
	}

protected:
	virtual void PreCreateThread()
	{
	}

protected:
	virtual void PostCreateThreadFinished()
	{
	}

protected:
	/// @brief Overrided by ConEmu GUI descendant to show action in the StatusBar
	/// @param processId 0 when hooking is done (remove status bar notification)
	/// @param sName is executable name or window class name
	/// @return true if StatusBar was updated (so need to reset it later)
	virtual bool NotifyHookingStatus(const DWORD processId, LPCWSTR sName)
	{
		// descendant must return true if status bar was changed
		return false;
	}
public:
	// Messages to be placed in log
	virtual void LogHookingStatus(DWORD nForePID, LPCWSTR sMessage)
	{
		DEBUGSTRDEFTERM(sMessage);
	}
	virtual bool isLogging()
	{
		return false;
	}

protected:
	virtual void AutoClearThreads()
	{
		//if (!isMainThread())
		//	return;

		// Clear finished threads
		ClearThreads(false);
	}

protected:
	virtual void ConhostLocker(bool bLock, bool& bWasLocked)
	{
	}

protected:
	void StoreProcessed(const ProcessInfo& info)
	{
		MSectionLockSimple processGuard;
		processGuard.Lock(mcs_Processes);

		bool found = false;

		for (auto& exist : m_Processed)
		{
			if (exist.nPID == info.nPID)
			{
				if (exist.hProcess && exist.hProcess != info.hProcess)
					SafeCloseHandle(exist.hProcess);
				exist = info;
				found = true;
				break;
			}
		}

		if (!found)
		{
			m_Processed.push_back(info);
		}
	}

	void ClearProcessed(bool bForceAll)
	{
		//_ASSERTE(isMainThread());

		MSectionLockSimple processGuard;
		processGuard.Lock(mcs_Processes);

		for (INT_PTR i = m_Processed.size(); i--;)
		{
			bool bTerm = false;
			// ReSharper disable once CppLocalVariableMayBeConst
			HANDLE hProcess = m_Processed[i].hProcess;
			// ReSharper disable once CppLocalVariableMayBeConst
			HWND hWnd = m_Processed[i].hWnd;

			if (bForceAll)
			{
				bTerm = true;
			}
			else if (hWnd)
			{
				if (!IsWindow(hWnd))
				{
					bTerm = true;
				}
			}
			else if (hProcess)
			{
				const DWORD nWait = WaitForSingleObject(hProcess, 0);
				if (nWait == WAIT_OBJECT_0)
				{
					bTerm = true;
				}
			}

			if (bTerm)
			{
				if (hProcess)
					CloseHandle(hProcess);
				m_Processed.erase(i);
			}
		}

		if (mh_LastWnd && (bForceAll || !IsWindow(mh_LastWnd)))
		{
			if (mh_LastIgnoredWnd == mh_LastWnd)
				mh_LastIgnoredWnd = nullptr;
			mh_LastWnd = nullptr;
		}
		if (mh_LastIgnoredWnd && (bForceAll || !IsWindow(mh_LastIgnoredWnd)))
		{
			mh_LastIgnoredWnd = nullptr;
		}
	}

protected:
	void ClearThreads(const bool bForceTerminate)
	{
		_ASSERTE((!bForceTerminate || mb_Termination) && "Termination state is expected here!");

		MSectionLockSimple threadGuard;
		threadGuard.Lock(mcs_Threads);

		for (INT_PTR i = m_Threads.size(); i--;)
		{
			const ThreadHandle& th = m_Threads[i];
			HANDLE hThread = th.hThread;
			// mb_Termination: The thread may be was not properly started yet, what for a while...
			if (WaitForSingleObject(hThread, bForceTerminate ? 2500 : 0) != WAIT_OBJECT_0)
			{
				if (bForceTerminate)
				{
					#ifdef _DEBUG
					SuspendThread(hThread);  // -V720
					_ASSERTE(FALSE && "Terminating DefTermBase hooker thread");
					ResumeThread(hThread); // superfluous?
					#endif
					apiTerminateThread(hThread, 100);
				}
				else
				{
					continue; // This thread is yet alive
				}
			}
			SafeCloseHandle(hThread);

			m_Threads.erase(i);
		}
	}

protected:
	static DWORD WINAPI PostCreatedThread(LPVOID lpParameter)
	{
		CDefTermBase *pTerm = static_cast<CDefTermBase*>(lpParameter);

		if (pTerm->mb_ConEmuGui || pTerm->m_Opt.bAggressive)
		{
			// Tests Shell (Taskbar) and active window (GetForegroundWindow)
			pTerm->CheckShellWindow();
		}

		pTerm->PostCreateThreadFinished();

		// Done
		pTerm->mb_PostCreatedThread = false;

		return 0;
	}

protected:
	// To avoid possible ConEmu.exe hang in hook installation into shell process (explorer.exe)
	// during ConEmu startup and ConEmuHk loading
	static DWORD WINAPI PostCheckThread(LPVOID lpParameter)
	{
		bool bRc = false;
		ThreadArg* pArg = static_cast<ThreadArg*>(lpParameter);
		if (pArg && !pArg->pTerm->mb_Termination)
		{
			bRc = pArg->pTerm->CheckForeground(pArg->hFore, pArg->nForePID, false, !pArg->pTerm->mb_ExplorerHookAllowed);
			free(pArg);
		}
		return bRc;
	}

	/// @brief In the Inside mode we set hooks only to our parent window which we have integrated in
	/// @param hFore GetForegroundWindow
	/// @param processId PID
	/// @return true if we may proceed with the process
	virtual bool IsAppAllowed(HWND hFore, DWORD processId)
	{
		return true;
	}

public:
	/// @brief Is exe-name or application window class name in user-defined list?
	/// @param name File name (no path) or class name to check
	/// @return true if name exists in hooked apps list
	bool IsAppNameMonitored(const wchar_t* name) const
	{
		bool bMonitored = false;
		// ReSharper disable once CppLocalVariableMayBeConst
		LPCWSTR pszMonitored = m_Opt.pszzHookedApps;

		if (pszMonitored)
		{
			// All strings are lower case
			const wchar_t* psz = pszMonitored;
			while (*psz)
			{
				if (lstrcmpi(psz, name) == 0)
				{
					bMonitored = true;
					break;
				}
				psz += wcslen(psz) + 1;
			}
		}

		return bMonitored;
	}

protected:
	// Поставить хук в процесс шелла (explorer.exe)
	bool CheckShellWindow()
	{
		if (mb_Termination)
			return false;

		bool bHooked = false;
		// ReSharper disable once CppLocalVariableMayBeConst
		HWND hFore = GetForegroundWindow();
		// ReSharper disable once CppLocalVariableMayBeConst
		HWND hDesktop = GetDesktopWindow(); //csrss.exe on Windows 8
		// ReSharper disable once CppLocalVariableMayBeConst
		HWND hShell = GetShellWindow();
		// ReSharper disable once CppLocalVariableMayBeConst
		HWND hTrayWnd = FindWindowEx(nullptr, nullptr, L"Shell_TrayWnd", nullptr);
		DWORD nDesktopPID = 0, nShellPID = 0, nTrayPID = 0, nForePID = 0;

		if (!bHooked && hShell)
		{
			if (GetWindowThreadProcessId(hShell, &nShellPID) && nShellPID)
			{
				bHooked = CheckForeground(hShell, nShellPID, false, !mb_ExplorerHookAllowed);
			}
		}

		if (!bHooked && hTrayWnd)
		{
			if (GetWindowThreadProcessId(hTrayWnd, &nTrayPID) && nTrayPID
				&& (nTrayPID != nShellPID))
			{
				bHooked = CheckForeground(hTrayWnd, nTrayPID, false, !mb_ExplorerHookAllowed);
			}
		}

		if (!bHooked && hDesktop)
		{
			if (GetWindowThreadProcessId(hDesktop, &nDesktopPID) && nDesktopPID
				&& (nDesktopPID != nTrayPID) && (nDesktopPID != nShellPID))
			{
				bHooked = CheckForeground(hDesktop, nDesktopPID, false, !mb_ExplorerHookAllowed);
			}
		}

		// Поскольку это выполняется на старте, то ConEmu могли запустить специально
		// для установки перехвата терминала. Поэтому нужно проверить и ForegroundWindow!
		if (hFore)
		{
			if (GetWindowThreadProcessId(hFore, &nForePID)
				&& (nForePID != GetCurrentProcessId())
				&& (nForePID != nShellPID) && (nForePID != nDesktopPID) && (nForePID != nTrayPID))
			{
				CheckForeground(hFore, nForePID, false);
			}
		}

		return bHooked;
	}
};
