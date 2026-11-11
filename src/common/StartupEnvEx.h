
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

#pragma once

#include "MModule.h"
#include "MStrDup.h"
#include "StartupEnv.h"
#include "WSession.h"
#include "WUser.h"
#include "../ConEmu/version.h"
#include <tlhelp32.h>

class LoadStartupEnvEx : public LoadStartupEnv
{
protected:
	static BOOL CALLBACK LoadStartupEnv_FillMonitors(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
	{
		CEStartupEnv* p = (CEStartupEnv*)dwData;
		if (p->nMonitorsCount >= countof(p->Monitors))
			return FALSE;

		MONITORINFOEX mi;
		ZeroStruct(mi);
		mi.cbSize = sizeof(mi);
		if (GetMonitorInfo(hMonitor, &mi))
		{
			size_t i = p->nMonitorsCount++;
			p->Monitors[i].hMon = hMonitor;
			p->Monitors[i].rcMonitor = mi.rcMonitor;
			p->Monitors[i].rcWork = mi.rcWork;
			p->Monitors[i].dwFlags = mi.dwFlags;
			wcscpy_c(p->Monitors[i].szDevice, mi.szDevice);

			// If lpszDriver is ... the device name of a specific display device,
			// then lpszDevice must be ... that same device name.
			HDC hdc = CreateDC(mi.szDevice, mi.szDevice, NULL, NULL);  // -V549
			if (hdc)
			{
				p->Monitors[i].dpis[0].x = GetDeviceCaps(hdc, LOGPIXELSX);
				p->Monitors[i].dpis[0].y = GetDeviceCaps(hdc, LOGPIXELSY);
				DeleteDC(hdc);
			}
			else
			{
				p->Monitors[i].dpis[0].x = p->Monitors[i].dpis[0].y = -1;
			}

			if (p->nMonitorsCount >= countof(p->Monitors))
				return FALSE;
		}

		return TRUE;
	}

	static CEStr LoadFonts()
	{
		CEStr result;
		int nLenMax = 2048;
		wchar_t* pszFonts = result.GetBuffer(nLenMax);

		if (pszFonts)
		{
			// ReSharper disable CppTooWideScope
			wchar_t szName[64], szValue[64];
			HKEY hk;
			const DWORD nRights = KEY_READ | WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0), 0);
			if (0 == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont", 0, nRights, &hk))
			{
				const bool bIsDBCS = IsWinDBCS();
				DWORD idx = 0, cchName = countof(szName), cchValue = sizeof(szValue)-2, dwType;
				LONG iRc;
				wchar_t* psz = pszFonts;
				while ((iRc = RegEnumValue(hk, idx++, szName, &cchName, NULL, &dwType, (LPBYTE)szValue, &cchValue)) == 0)
				{
					szName[std::min<size_t>(countof(szName)-1, cchName)] = 0;
					szValue[std::min<size_t>(countof(szValue)-1,cchValue/2)] = 0;
					const int nNameLen = lstrlen(szName);
					const int nValLen = lstrlen(szValue);
					const int nLen = nNameLen+nValLen+3;

					wchar_t* pszEndPtr = NULL;
					int cp = wcstol(szName, &pszEndPtr, 10);
					// Standard console fonts are stored with named "0", "00", "000", etc.
					// But some hieroglyph fonts can be stored for special codepages ("950", "936", etc)
					if (cp)
					{
						// Is codepage supported by our console host?
						// No way to check that...
						// All function reporting "Valid CP"
						// Checked: GetCPInfo, GetCPInfoEx, IsValidCodePage, WideCharToMultiByte
						if (!bIsDBCS)
							cp = -1; // No need to store this font
					}

					if ((cp != -1) && (nLen < nLenMax))
					{
						const wchar_t* from = psz;
						if (*pszFonts) *(psz++) = L'\t';
						lstrcpy(psz, szName); psz += nNameLen;
						*(psz++) = L'\t';
						lstrcpy(psz, szValue); psz += nValLen;
						nLenMax -= static_cast<int>(psz - from);
					}

					cchName = countof(szName); cchValue = sizeof(szValue)-2;
				}
				RegCloseKey(hk);
			}
		}

		return result;
	}

	static void LoadAutorunsKey(HKEY& hk, CEStr& szAutoruns, LPCWSTR pszPrefix)
	{
		if (!hk) return;
		DWORD nSize = 0;
		if ((RegQueryValueEx(hk, L"AutoRun", NULL, NULL, NULL, &nSize) == 0) && nSize)
		{
			CEStr data;
			if (data.GetBuffer(nSize+2)
				&& (RegQueryValueEx(hk, L"AutoRun", NULL, NULL, (LPBYTE)data.data(), &nSize) == 0)
				&& !data.IsEmpty())
			{
				szAutoruns.Append(pszPrefix, data.c_str(), L"\r\n");
			}
		}
		RegCloseKey(hk);
		hk = NULL;
	}

	static CEStr LoadAutoruns()
	{
		CEStr result;
		HKEY hk = NULL;
		if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", 0, KEY_READ, &hk) == 0)
			LoadAutorunsKey(hk, result, L"  HKCU: "); // closes hk
		bool bWin64 = IsWindows64();
		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Command Processor", 0, KEY_READ|(bWin64?KEY_WOW64_32KEY:0), &hk) == 0)
			LoadAutorunsKey(hk, result, L"  HKLM32: "); // closes hk
		if (bWin64 && RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Command Processor", 0, KEY_READ|KEY_WOW64_64KEY, &hk) == 0)
			LoadAutorunsKey(hk, result, L"  HKLM64: "); // closes hk
		return result;
	}

	static CEStr LoadWindowsBuild(const OSVERSIONINFOEXW& osv)
	{
		CEStr result;
		if (IsWin10())
		{
			HKEY hk = NULL;
			if (0 == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hk))
			{
				DWORD dwSize, UBR = 0, ubrType = 0, relType = 0;
				wchar_t ReleaseId[128] = L"", szUBR[32];
				LONG ubr = RegQueryValueEx(hk, L"UBR", NULL, &ubrType, (LPBYTE)&UBR, &(dwSize = sizeof(UBR)));
				LONG rid = RegQueryValueEx(hk, L"ReleaseId", NULL, &relType, (LPBYTE)ReleaseId, &(dwSize = sizeof(ReleaseId)));
				CEStr pszSP = osv.szCSDVersion[0] ? CEStr(L", SP: ", osv.szCSDVersion) : CEStr();
				if (ubr == 0 && ubrType == REG_DWORD && UBR && rid == 0 && relType == REG_SZ && *ReleaseId)
					result = CEStr(ReleaseId, L", UBR: ", ultow_s(UBR, szUBR, 10), pszSP);
				else if (rid == 0 && relType == REG_SZ && *ReleaseId)
					result = CEStr(ReleaseId, pszSP.c_str());
				else if (ubr == 0 && ubrType == REG_DWORD && UBR)
					result = CEStr(ReleaseId, L"UBR(", ultow_s(UBR, szUBR, 10), L")", pszSP);
				RegCloseKey(hk);
			}
		}
		if (result.IsEmpty())
			result.Set(osv.szCSDVersion);
		return result;
	}

	/*
	static BOOL CALLBACK EnumLocalesProcEx(LPWSTR lpLocaleString, DWORD dwFlags, LPARAM lParam)
	{
		return TRUE;
	}
	static BOOL CALLBACK EnumLocalesProc(LPWSTR lpLocaleString)
	{
		return TRUE;
	}
	*/

public:
	static CEStartupEnv* Create()
	{
		CEStartupEnv* pEnv = NULL;
		LPBYTE ptrEnd = NULL;

		CEStr pszFonts = LoadFonts();
		size_t cchFnt = pszFonts ? (lstrlen(pszFonts)+1) : 0;

		CEStr pszAutoruns = LoadAutoruns();
		size_t cchAut = pszAutoruns ? (lstrlen(pszAutoruns)+1) : 0;

		size_t cchTotal = (cchFnt+cchAut)*sizeof(wchar_t);

		if (Load(cchTotal, pEnv, ptrEnd))
		{
			wchar_t* psz = (wchar_t*)ptrEnd;

			pEnv->nMonitorsCount = 0;
			EnumDisplayMonitors(NULL, NULL, LoadStartupEnv_FillMonitors, reinterpret_cast<LPARAM>(pEnv));

			HDC hDC = CreateCompatibleDC(NULL);
			pEnv->nBPP = GetDeviceCaps(hDC, BITSPIXEL);
			DeleteDC(hDC);

			// These functions call AdvApi32 functions
			// But LoadStartupEnvEx is not used in ConEmuHk, so there will
			// no AdvApi32 loading during ConEmuHk initialization
			pEnv->bIsWine = IsWine() ? 1 : 0;
			pEnv->bIsWinPE = IsWinPE() ? 1 : 0;
			pEnv->bIsAdmin = IsUserAdmin() ? 1 : 0;

			if (!pEnv->bIsReactOS && (pEnv->os.dwMajorVersion == 5))
			{
				LPCWSTR pszReactCompare = GetReactOsName();
				wchar_t szValue[64] = L"";
				HKEY hk;
				if (0 == RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hk))
				{
					DWORD nSize = sizeof(szValue)-sizeof(*szValue);
					if (0 == RegQueryValueEx(hk, L"ProductName", NULL, NULL, (LPBYTE)szValue, &nSize))
					{
						pEnv->bIsReactOS = (lstrcmpi(szValue, pszReactCompare) == 0);
					}
					RegCloseKey(hk);
				}
			}

			if (pszFonts)
			{
				_wcscpy_c(psz, cchFnt, pszFonts);
				pEnv->pszRegConFonts = psz;
				psz += cchFnt;
			}

			if (pszAutoruns)
			{
				_wcscpy_c(psz, cchAut, pszAutoruns);
				pEnv->pszAutoruns = psz;
				psz += cchAut;
			}
		}

		return pEnv;
	}

	typedef void (*DumpEnvStr_t)(LPCWSTR asText, LPARAM lParam, bool bFirst, bool bNewLine);

	static void ToString(CEStartupEnv* apStartEnv, DumpEnvStr_t DumpEnvStr, LPARAM lParam)
	{
		if (!apStartEnv || (apStartEnv->cbSize < sizeof(*apStartEnv)))
		{
			DumpEnvStr(L"LogStartEnv failed, invalid apStartEnv", lParam, true, true);
			return;
		}

		#define dumpEnvStr(t,n) DumpEnvStr(t, lParam, false, n)

		// Пишем инфу
		wchar_t szSI[MAX_PATH*4], szDesktop[128] = L"", szTitle[128] = L"";

		BOOL bWin64 = IsWindows64();

		#pragma warning(push)
		#pragma warning(disable: 4996)
		OSVERSIONINFOEXW osv = {};
		osv.dwOSVersionInfoSize = sizeof(osv);
		GetOsVersionInformational(reinterpret_cast<OSVERSIONINFOW*>(&osv));
		#pragma warning(pop)

		// if you're running on ReactOS, Version.szCSDVersion will contain two strings.
		// The first string is the Windows compatible service pack number, "Service Pack 6".
		// At the end of that string is a null byte. Following that null byte is the ReactOS
		// version string (something like "ReactOS 0.2.5-RC1 (Build 20041227)".
		// When running on Windows, there will be only one string in Version.szCSDVersion.
		LPCWSTR pszReactOS = osv.szCSDVersion + lstrlen(osv.szCSDVersion); // points to '\0'
		if ((pszReactOS + 10) < (osv.szCSDVersion + countof(osv.szCSDVersion)))
		{
			if (*(pszReactOS+1))
				++pszReactOS;
		}

		HWND hConWnd = GetConsoleWindow();

		FILETIME CreationTime = {}, ExitTime = {}, KernelTime = {}, UserTime = {};
		wchar_t szStartTime[32] = L"";
		if (GetProcessTimes(GetCurrentProcess(), &CreationTime, &ExitTime, &KernelTime, &UserTime))
		{
			FILETIME lft = {};
			SYSTEMTIME st = {};
			if (FileTimeToLocalFileTime(&CreationTime, &lft)
				&& FileTimeToSystemTime(&lft, &st))
					swprintf_c(szStartTime,
						L"%u-%02u-%02u %02u:%02u:%02u.%03u",
						st.wYear, st.wMonth, st.wDay,
						st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		}

		swprintf_c(szTitle, L"%u.%u.%u.x%u", osv.dwMajorVersion, osv.dwMinorVersion, osv.dwBuildNumber, bWin64 ? 64 : 32);
		if ((osv.dwMajorVersion == 6) && (osv.dwMinorVersion == 3))
		{
			if (IsWin10())
				wcscat_c(szTitle, L" !Win10!");
		}

		wchar_t szBuild[24], szVer4[8] = L"", szProdType[20];
		lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
		// Same as ConsoleMain.cpp::SetWorkEnvVar()
		swprintf_c(szBuild, L"%02u%02u%02u%s%s",
			(MVV_1%100), MVV_2, MVV_3, szVer4[0]&&szVer4[1]?L"-":L"", szVer4);
		if (osv.wProductType == VER_NT_WORKSTATION)
			wcscpy_c(szProdType, L"workstation");
		else if (osv.wProductType == VER_NT_SERVER)
			wcscpy_c(szProdType, L"server");
		else if (osv.wProductType == VER_NT_DOMAIN_CONTROLLER)
			wcscpy_c(szProdType, L"domain");
		else
			swprintf_c(szProdType, L"%u", osv.wProductType);

		CEStr pszWinBuild = LoadWindowsBuild(osv);
		swprintf_c(szSI, L"ConEmu %s [%u] Startup Info\r\n"
			L"  OsVer: %s, Product: %s, SP: %u.%u, Suite: 0x%X\r\n"
			L"  Build: %s, ReactOS: %u%s%s%s, Rsrv: %u, WINE: %u, PE: %u, R2: %u\r\n"
			L"  DBCS: %u, IMM: %u, Remote: %u, ACP: %u, OEMCP: %u, Admin: %u\r\n"
			L"  StartTime: %s\r\n"
			, szBuild, WIN3264TEST(32, 64),
			szTitle,
			szProdType, osv.wServicePackMajor, osv.wServicePackMinor, osv.wSuiteMask,
			pszWinBuild.c_str(L""), apStartEnv->bIsReactOS, *pszReactOS ? L" (" : L"", pszReactOS, *pszReactOS ? L")" : L"",
			osv.wReserved, apStartEnv->bIsWine, apStartEnv->bIsWinPE, GetSystemMetrics(89/*SM_SERVERR2*/),
			apStartEnv->bIsDbcs, apStartEnv->bIsImm, apStartEnv->bIsRemote,
			apStartEnv->nAnsiCP, apStartEnv->nOEMCP, apStartEnv->bIsAdmin,
			szStartTime);
		DumpEnvStr(szSI, lParam, true, false);
		pszWinBuild.Release();

		bool is_themed = false, is_dwm = false;
		if (IsWinXP())
		{
			MModule uxtheme(L"UxTheme.dll");
			BOOL (WINAPI* _IsAppThemed)();
			BOOL (WINAPI* _IsThemeActive)();
			if (uxtheme.GetProcAddress("IsAppThemed", _IsAppThemed) && uxtheme.GetProcAddress("IsThemeActive", _IsThemeActive))
				is_themed = _IsAppThemed() && _IsThemeActive();
			if (IsWin7())
			{
				MModule dwm(L"dwmapi.dll");
				HRESULT (WINAPI* _DwmIsCompositionEnabled)(BOOL *pfEnabled);
				if (dwm.GetProcAddress("DwmIsCompositionEnabled", _DwmIsCompositionEnabled))
				{
					BOOL composition_enabled = FALSE;
					is_dwm = (_DwmIsCompositionEnabled(&composition_enabled) == S_OK) && composition_enabled;
				}
			}
		}

		lstrcpyn(szDesktop, apStartEnv->si.lpDesktop ? apStartEnv->si.lpDesktop : L"<NULL>", countof(szDesktop));
		lstrcpyn(szTitle, apStartEnv->si.lpTitle ? apStartEnv->si.lpTitle : L"<NULL>", countof(szTitle));

		swprintf_c(szSI,
			L"  Desktop: %s%s%s, SessionId: %s, ConsoleSessionId: %u, Theming: %u, DWM: %u\r\n"
			L"  Title: %s%s%s\r\n  Size: {%u,%u},{%u,%u}\r\n"
			L"  Flags: 0x%08X, ShowWindow: %u, ConHWnd: 0x%08X\r\n"
			L"  char: %u, short: %u, int: %u, long: %u, u64: %u\r\n"
			L"  Handles: 0x%08X, 0x%08X, 0x%08X\r\n"
			,
			apStartEnv->si.lpDesktop ? L"`" : L"", szDesktop, apStartEnv->si.lpDesktop ? L"`" : L"",
			apiQuerySessionID(), apiGetConsoleSessionID(),
			is_themed ? 1 : 0, is_dwm ? 1 : 0,
			apStartEnv->si.lpTitle ? L"`" : L"", szTitle, apStartEnv->si.lpTitle ? L"`" : L"",
			apStartEnv->si.dwX, apStartEnv->si.dwY, apStartEnv->si.dwXSize, apStartEnv->si.dwYSize,
			apStartEnv->si.dwFlags, static_cast<DWORD>(apStartEnv->si.wShowWindow), LODWORD(hConWnd),
			LODWORD(sizeof(CHAR)), LODWORD(sizeof(short)), LODWORD(sizeof(int)), LODWORD(sizeof(long)), LODWORD(sizeof(uint64_t)),
			LODWORD(apStartEnv->si.hStdInput), LODWORD(apStartEnv->si.hStdOutput), LODWORD(apStartEnv->si.hStdError)
			);
		dumpEnvStr(szSI, false);

		{
		HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
		HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
		DWORD nInMode = 0, nOutMode = 0, nErrMode = 0;
		BOOL bIn = GetConsoleMode(hIn, &nInMode);
		BOOL bOut = GetConsoleMode(hOut, &nOutMode);
		BOOL bErr = GetConsoleMode(hErr, &nErrMode);
		wchar_t szIn[20], szOut[20], szErr[20];
		msprintf(szIn, countof(szIn), L"x%X", nInMode);
		msprintf(szOut, countof(szOut), L"x%X", nOutMode);
		msprintf(szErr, countof(szErr), L"x%X", nErrMode);
		msprintf(szSI, countof(szSI), L"  StdFlags: In=x%X (Mode=%s) Out=x%X (Mode=%s) Err=x%X (Mode=%s)\r\n",
			LODWORD(hIn), bIn?szIn:L"-1", LODWORD(hOut), bOut?szOut:L"-1", LODWORD(hErr), bErr?szErr:L"-1");
		dumpEnvStr(szSI, false);
		}

		if (hConWnd)
		{
			szSI[0] = 0;
			HMODULE hKernel = GetModuleHandleW(L"kernel32.dll");
			typedef BOOL (__stdcall *FGetConsoleKeyboardLayoutName)(wchar_t*);
			FGetConsoleKeyboardLayoutName pfnGetConsoleKeyboardLayoutName = hKernel ? (FGetConsoleKeyboardLayoutName)GetProcAddress(hKernel, "GetConsoleKeyboardLayoutNameW") : NULL;
			if (pfnGetConsoleKeyboardLayoutName)
			{
				ZeroStruct(szTitle);
				if (pfnGetConsoleKeyboardLayoutName(szTitle))
					swprintf_c(szSI, L"  Active console layout name: '%s'", szTitle);
			}
			if (!*szSI)
				swprintf_c(szSI, L"  Active console layout: Unknown, code=%u", GetLastError());
			dumpEnvStr(szSI, true);
		}

		// PID & TID
		swprintf_c(szSI, L"  Current PID: %u, TID: %u", GetCurrentProcessId(), GetCurrentThreadId());
		dumpEnvStr(szSI, true);

		// Текущий HKL (он может отличаться от GetConsoleKeyboardLayoutNameW
		HKL hkl[32] = {NULL};
		hkl[0] = GetKeyboardLayout(0);
		swprintf_c(szSI, L"  Active HKL: " WIN3264TEST(L"0x%08X",L"0x%08X%08X"), WIN3264WSPRINT((DWORD_PTR)hkl[0]));
		dumpEnvStr(szSI, true);
		// Установленные в системе HKL
		UINT nHkl = GetKeyboardLayoutList(countof(hkl), hkl);
		if (!nHkl || (nHkl > countof(hkl)))
		{
			swprintf_c(szSI, L"  GetKeyboardLayoutList failed, code=%u", GetLastError());
			dumpEnvStr(szSI, true);
		}
		else
		{
			wcscpy_c(szSI, L"  GetKeyboardLayoutList:");
			size_t iLen = lstrlen(szSI);
			_ASSERTE((iLen + 1 + nHkl*17)<countof(szSI));

			for (UINT i = 0; i < nHkl; i++)
			{
				swprintf_c(szSI+iLen, 18/*#SECURELEN*/, WIN3264TEST(L" 0x%08X",L" 0x%08X%08X"), WIN3264WSPRINT((DWORD_PTR)hkl[i]));
				iLen += lstrlen(szSI+iLen);
			}
			dumpEnvStr(szSI, true);
		}

		/*
		EnumSystemLocales(EnumLocalesProc, LCID_INSTALLED);
		EnumSystemLocalesEx(EnumLocalesProcEx, LOCALE_ALL, 0, NULL);
		*/

		dumpEnvStr(L"CmdLine: ", false);
		DumpEnvStr(apStartEnv->pszCmdLine ? apStartEnv->pszCmdLine : L"<NULL>", lParam, false, true);
		dumpEnvStr(L"ExecMod: ", false);
		DumpEnvStr(apStartEnv->pszExecMod ? apStartEnv->pszExecMod : L"<NULL>", lParam, false, true);
		dumpEnvStr(L"WorkDir: ", false);
		DumpEnvStr(apStartEnv->pszWorkDir ? apStartEnv->pszWorkDir : L"<NULL>", lParam, false, true);
		dumpEnvStr(L"PathEnv: ", false);
		DumpEnvStr(apStartEnv->pszPathEnv ? apStartEnv->pszPathEnv : L"<NULL>", lParam, false, true);
		dumpEnvStr(L"ConFont: ", false);
		DumpEnvStr(apStartEnv->pszRegConFonts ? apStartEnv->pszRegConFonts : L"<NULL>", lParam, false, true);

		if (!apStartEnv->pszAutoruns)
		{
			DumpEnvStr(L"CMD's AutoRuns: {not defined}", lParam, false, true);
		}
		else
		{
			DumpEnvStr(L"CMD's AutoRuns:", lParam, false, true);
			DumpEnvStr(apStartEnv->pszAutoruns, lParam, false, false); // Alredy "\r\n" terminated
		}

		// szSI уже не используется, можно

		HWND hFore = GetForegroundWindow();
		RECT rcFore = {0}; if (hFore) GetWindowRect(hFore, &rcFore);
		if (hFore) GetClassName(hFore, szDesktop, countof(szDesktop)-1); else szDesktop[0] = 0;
		if (hFore) GetWindowText(hFore, szTitle, countof(szTitle)-1); else szTitle[0] = 0;
		swprintf_c(szSI, L"Foreground: x%08X {%i,%i}-{%i,%i} '%s' - %s", (DWORD)(DWORD_PTR)hFore, LOGRECTCOORDS(rcFore), szDesktop, szTitle);
		dumpEnvStr(szSI, true);

		POINT ptCur = {0}; GetCursorPos(&ptCur);
		HMONITOR hMonFromMouse = MonitorFromPoint(ptCur, MONITOR_DEFAULTTONULL);
		MONITORINFOEX mon; ZeroStruct(mon); mon.cbSize = sizeof(mon);
		HMONITOR hMonFromStart = apStartEnv->hStartMon;
		if (!hMonFromStart || !GetMonitorInfo(hMonFromStart, &mon))
			hMonFromStart = NULL;
		swprintf_c(szSI,
			L"MouseCursor: {%i,%i} MouseMonitor: %08X StartMonitor: %08X",
			ptCur.x, ptCur.y, LODWORD(hMonFromMouse), LODWORD(hMonFromStart));
		dumpEnvStr(szSI, true);

		HDC hdcScreen = GetDC(NULL);
		apStartEnv->nBPP = GetDeviceCaps(hdcScreen,BITSPIXEL);
		int nPlanes = GetDeviceCaps(hdcScreen,PLANES);
		int nAlignment = GetDeviceCaps(hdcScreen,BLTALIGNMENT);
		int nVRefr = GetDeviceCaps(hdcScreen,VREFRESH);
		int nShadeCaps = GetDeviceCaps(hdcScreen,SHADEBLENDCAPS);
		int nDevCaps = GetDeviceCaps(hdcScreen,RASTERCAPS);
		int nDpiX = GetDeviceCaps(hdcScreen, LOGPIXELSX);
		int nDpiY = GetDeviceCaps(hdcScreen, LOGPIXELSY);
		swprintf_c(szSI,
			L"Display: bpp=%i, planes=%i, align=%i, vrefr=%i, shade=x%08X, rast=x%08X, dpi=%ix%i, per-mon-dpi=%u",
			apStartEnv->nBPP, nPlanes, nAlignment, nVRefr, nShadeCaps, nDevCaps, nDpiX, nDpiY, apStartEnv->bIsPerMonitorDpi);
		ReleaseDC(NULL, hdcScreen);
		dumpEnvStr(szSI, true);

		dumpEnvStr(L"Monitors (dpi: WholeDesktop, Effective, Angular, RAW):", true);
		for (size_t i = 0; i < apStartEnv->nMonitorsCount; i++)
		{
			CEStartupEnv::MyMonitorInfo* p = (apStartEnv->Monitors + i);
			szDesktop[0] = 0;
			for (size_t j = 0; j < countof(p->dpis); j++)
			{
				if (p->dpis[j].x || p->dpis[j].y)
				{
					wchar_t szDpi[32];
					swprintf_c(szDpi,
						szDesktop[0] ? L";{%i,%i}" : L"{%i,%i}",
						p->dpis[j].x, p->dpis[j].y);
					wcscat_c(szDesktop, szDpi);
				}
			}
			swprintf_c(szSI,
				L"  %08X: {%i,%i}-{%i,%i} (%ix%i), Working: {%i,%i}-{%i,%i} (%ix%i), dpi: %s `%s`%s",
				(DWORD)(DWORD_PTR)p->hMon,
				LOGRECTCOORDS(p->rcMonitor), LOGRECTSIZE(p->rcMonitor),
				LOGRECTCOORDS(p->rcWork), LOGRECTSIZE(p->rcWork),
				szDesktop, p->szDevice,
				(p->dwFlags & MONITORINFOF_PRIMARY) ? L" <<== Primary" : L"");
			dumpEnvStr(szSI, true);
		}

		dumpEnvStr(L"Modules:", true);
		HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
		MODULEENTRY32W mi = {sizeof(mi)};
		if (h && (h != INVALID_HANDLE_VALUE))
		{
			if (Module32First(h, &mi))
			{
				do
				{
					DWORD_PTR ptrStart = (DWORD_PTR)mi.modBaseAddr;
					DWORD_PTR ptrEnd = (DWORD_PTR)mi.modBaseAddr + (DWORD_PTR)(mi.modBaseSize ? (mi.modBaseSize-1) : 0);
					swprintf_c(szSI,
						L"  " WIN3264TEST(L"%08X-%08X",L"%08X%08X-%08X%08X") L" %8X %s",
						WIN3264WSPRINT(ptrStart), WIN3264WSPRINT(ptrEnd), mi.modBaseSize, mi.szExePath);
					dumpEnvStr(szSI, true);
				} while (Module32Next(h, &mi));
			}
			CloseHandle(h);
		}

		#undef dumpEnvStr
	}
};
