
/*
Copyright (c) 2015-present Maximus5
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
	#define DebugString(x) //OutputDebugString(x)
#else
	#define DebugString(x) //OutputDebugString(x)
#endif

// SetLastError, GetLastError
#ifdef _DEBUG
	#define HOOK_ERROR_PROC
	//#undef HOOK_ERROR_PROC
	#define HOOK_ERROR_NO ERROR_INVALID_DATA
#else
	#undef HOOK_ERROR_PROC
#endif

#include "../common/Common.h"

#include <tlhelp32.h>

#include "hkKernel.h"

#include "DllOptions.h"

/* **************** */

// Helper function
bool LocalToSystemTime(LPFILETIME lpLocal, LPSYSTEMTIME lpSystem)
{
	FILETIME ftSystem;
	bool bOk = LocalFileTimeToFileTime(lpLocal, &ftSystem)
		&& FileTimeToSystemTime(&ftSystem, lpSystem);
	return bOk;
}

// Helper function
bool GetTime(bool bSystem, LPSYSTEMTIME lpSystemTime)
{
	bool bHacked = false;
	wchar_t szEnvVar[32] = L""; // 2013-01-01T15:16:17.95

	DWORD nCurTick = GetTickCount();
	DWORD nCheckDelta = nCurTick - gnTimeEnvVarLastCheck;
	const DWORD nCheckDeltaMax = 1000;
	if (!gnTimeEnvVarLastCheck || (nCheckDelta >= nCheckDeltaMax))
	{
		gnTimeEnvVarLastCheck = nCurTick;
		GetEnvironmentVariable(ENV_CONEMUFAKEDT_VAR_W, szEnvVar, countof(szEnvVar));
		lstrcpyn(gszTimeEnvVarSave, szEnvVar, countof(gszTimeEnvVarSave));
	}
	else if (*gszTimeEnvVarSave)
	{
		lstrcpyn(szEnvVar, gszTimeEnvVarSave, countof(szEnvVar));
	}
	else
	{
		goto wrap;
	}

	if (*szEnvVar)
	{
		SYSTEMTIME st = {0}; FILETIME ft; wchar_t* p = szEnvVar;

		if (!(st.wYear = LOWORD(wcstol(p, &p, 10))) || !p || (*p != L'-' && *p != L'.'))
			goto wrap;
		if (!(st.wMonth = LOWORD(wcstol(p+1, &p, 10))) || !p || (*p != L'-' && *p != L'.'))
			goto wrap;
		if (!(st.wDay = LOWORD(wcstol(p+1, &p, 10))) || !p || (*p != L'T' && *p != L' ' && *p != 0))
			goto wrap;
		// Possible format 'dd.mm.yyyy'? This is returned by "cmd /k echo %DATE%"
		if (st.wDay >= 1900 && st.wYear <= 31)
		{
			WORD w = st.wDay; st.wDay = st.wYear; st.wYear = w;
		}

		// Time. Optional?
		if (!p || !*p)
		{
			SYSTEMTIME lt; GetLocalTime(&lt);
			st.wHour = lt.wHour;
			st.wMinute = lt.wMinute;
			st.wSecond = lt.wSecond;
			st.wMilliseconds = lt.wMilliseconds;
		}
		else
		{
			if (((st.wHour = LOWORD(wcstol(p+1, &p, 10)))>=24) || !p || (*p != L':' && *p != L'.'))
				goto wrap;
			if (((st.wMinute = LOWORD(wcstol(p+1, &p, 10)))>=60))
				goto wrap;

			// Seconds and MS are optional
			if ((p && (*p == L':' || *p == L'.')) && ((st.wSecond = LOWORD(wcstol(p+1, &p, 10))) >= 60))
				goto wrap;
			// cmd`s %TIME% shows Milliseconds as two digits
			if ((p && (*p == L':' || *p == L'.')) && ((st.wMilliseconds = (10*LOWORD(wcstol(p+1, &p, 10)))) >= 1000))
				goto wrap;
		}

		// Check it
		if (!SystemTimeToFileTime(&st, &ft))
			goto wrap;
		if (bSystem)
		{
			if (!LocalToSystemTime(&ft, lpSystemTime))
				goto wrap;
		}
		else
		{
			*lpSystemTime = st;
		}
		bHacked = true;
	}
wrap:
	return bHacked;
}

#ifdef _DEBUG
// Helper function
bool FindModuleByAddress(const BYTE* lpAddress, LPWSTR pszModule, int cchMax)
{
	bool bFound = false;
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (hSnap != INVALID_HANDLE_VALUE)
	{
		MODULEENTRY32 mi = {sizeof(mi)};
		if (Module32First(hSnap, &mi))
		{
			do {
				if ((lpAddress >= mi.modBaseAddr) && (lpAddress < (mi.modBaseAddr + mi.modBaseSize)))
				{
					bFound = true;
					if (pszModule)
						lstrcpyn(pszModule, mi.szExePath, cchMax);
					break;
				}
			} while (Module32Next(hSnap, &mi));
		}
		CloseHandle(hSnap);
	}

	if (!bFound && pszModule)
		*pszModule = 0;
	return bFound;
}
#endif


/* **************** */

#ifdef _DEBUG
LPVOID WINAPI OnVirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
	//typedef LPVOID(WINAPI* OnVirtualAlloc_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);
	ORIGINAL_KRNL(VirtualAlloc);

	LPVOID lpResult = F(VirtualAlloc)(lpAddress, dwSize, flAllocationType, flProtect);
	DWORD dwErr = GetLastError();

	wchar_t szText[MAX_PATH*4];
	if (lpResult == NULL)
	{
		wchar_t szTitle[64];
		msprintf(szTitle, countof(szTitle), L"ConEmuHk, PID=%u, TID=%u", GetCurrentProcessId(), GetCurrentThreadId());

		MEMORY_BASIC_INFORMATION mbi = {};
		SIZE_T mbiSize = VirtualQuery(lpAddress, &mbi, sizeof(mbi));
		wchar_t szOwnedModule[MAX_PATH] = L"";
		FindModuleByAddress((const BYTE*)lpAddress, szOwnedModule, countof(szOwnedModule));

		msprintf(szText, countof(szText)-MAX_PATH,
			L"VirtualAlloc " WIN3264TEST(L"%u",L"x%X%08X") L" bytes failed (%08X..%08X)\n"
			L"ErrorCode=%u %s\n\n"
			L"Allocated information (" WIN3264TEST(L"x%08X",L"x%X%08X") L".." WIN3264TEST(L"x%08X",L"x%X%08X") L")\n"
			L"Size " WIN3264TEST(L"%u",L"x%X%08X") L" bytes State x%X Type x%X Protect x%X\n"
			L"Module: %s\n\n"
			L"Warning! This may cause an errors in Release!\n"
			L"Press <OK> to VirtualAlloc at the default address\n\n",
			WIN3264WSPRINT(dwSize), LODWORD(lpAddress), LODWORD((LPBYTE)lpAddress+dwSize),
			dwErr, (dwErr == 487) ? L"(ERROR_INVALID_ADDRESS)" : L"",
			WIN3264WSPRINT(mbi.BaseAddress), WIN3264WSPRINT(((LPBYTE)mbi.BaseAddress+mbi.RegionSize)),
			WIN3264WSPRINT(mbi.RegionSize), mbi.State, mbi.Type, mbi.Protect,
			szOwnedModule[0] ? szOwnedModule : L"<Unknown>",
			0);

		GetModuleFileName(NULL, szText+lstrlen(szText), MAX_PATH);

		DebugString(szText);

	#if defined(REPORT_VIRTUAL_ALLOC)
		// clink use bunch of VirtualAlloc to try to find suitable memory location
		// Some processes raises that error too often (in debug)
		bool bReport = (!gbIsCmdProcess || (dwSize != 0x1000)) && !gbSkipVirtualAllocErr && !gbIsNodeJsProcess;
		if (bReport)
		{
			// Do not report for .Net application
			static int iNetChecked = 0;
			if (!iNetChecked)
			{
				HMODULE hClr = GetModuleHandle(L"mscoree.dll");
				iNetChecked = hClr ? 2 : 1;
			}
			bReport = (iNetChecked == 1);
		}
		int iBtn = bReport ? GuiMessageBox(NULL, szText, szTitle, MB_SYSTEMMODAL|MB_OKCANCEL|MB_ICONSTOP) : IDCANCEL;
		if (iBtn == IDOK)
		{
			lpResult = F(VirtualAlloc)(NULL, dwSize, flAllocationType, flProtect);
			dwErr = GetLastError();
		}
	#endif
	}
	msprintf(szText, countof(szText),
		L"VirtualAlloc(" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"," WIN3264TEST(L"0x%08X",L"0x%08X%08X") L",%u,%u)=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"\n",
		WIN3264WSPRINT(lpAddress), WIN3264WSPRINT(dwSize), flAllocationType, flProtect, WIN3264WSPRINT(lpResult));
	DebugString(szText);

	SetLastError(dwErr);
	return lpResult;
}
#endif


#if 0
BOOL WINAPI OnVirtualProtect(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect)
{
	//typedef BOOL(WINAPI* OnVirtualProtect_t)(LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
	ORIGINAL_KRNL(VirtualProtect);
	BOOL bResult = FALSE;

	if (F(VirtualProtect))
		bResult = F(VirtualProtect)(lpAddress, dwSize, flNewProtect, lpflOldProtect);

	#ifdef _DEBUG
	DWORD dwErr = GetLastError();
	wchar_t szDbgInfo[100];
	msprintf(szDbgInfo, countof(szDbgInfo),
		L"VirtualProtect(" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L"," WIN3264TEST(L"0x%08X",L"0x%08X%08X") L",%u,%u)=%u, code=%u\n",
		WIN3264WSPRINT(lpAddress), WIN3264WSPRINT(dwSize), flNewProtect, lpflOldProtect ? *lpflOldProtect : 0, bResult, dwErr);
	DebugString(szDbgInfo);
	SetLastError(dwErr);
	#endif

	return bResult;
}
#endif


#ifdef _DEBUG
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI OnSetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter)
{
	//typedef LPTOP_LEVEL_EXCEPTION_FILTER(WINAPI* OnSetUnhandledExceptionFilter_t)(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter);
	ORIGINAL_KRNL(SetUnhandledExceptionFilter);

	LPTOP_LEVEL_EXCEPTION_FILTER lpRc = F(SetUnhandledExceptionFilter)(lpTopLevelExceptionFilter);

	#ifdef _DEBUG
	DWORD dwErr = GetLastError();
	wchar_t szDbgInfo[100];
	msprintf(szDbgInfo, countof(szDbgInfo),
		L"SetUnhandledExceptionFilter(" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L")=" WIN3264TEST(L"0x%08X",L"0x%08X%08X") L", code=%u\n",
		WIN3264WSPRINT(lpTopLevelExceptionFilter), WIN3264WSPRINT(lpRc), dwErr);
	DebugString(szDbgInfo);
	SetLastError(dwErr);
	#endif

	return lpRc;
}
#endif


// TODO: GetSystemTimeAsFileTime??
void WINAPI OnGetSystemTime(LPSYSTEMTIME lpSystemTime)
{
	//typedef void (WINAPI* OnGetSystemTime_t)(LPSYSTEMTIME);
	ORIGINAL_KRNL(GetSystemTime);

	if (!GetTime(true, lpSystemTime))
		F(GetSystemTime)(lpSystemTime);
}


void WINAPI OnGetLocalTime(LPSYSTEMTIME lpSystemTime)
{
	//typedef void (WINAPI* OnGetLocalTime_t)(LPSYSTEMTIME);
	ORIGINAL_KRNL(GetLocalTime);

	if (!GetTime(false, lpSystemTime))
		F(GetLocalTime)(lpSystemTime);
}


void WINAPI OnGetSystemTimeAsFileTime(LPFILETIME lpSystemTimeAsFileTime)
{
	//typedef void (WINAPI* OnGetSystemTimeAsFileTime_t)(LPFILETIME);
	ORIGINAL_KRNL(GetSystemTimeAsFileTime);

	SYSTEMTIME st;
	if (GetTime(true, &st))
	{
		SystemTimeToFileTime(&st, lpSystemTimeAsFileTime);
	}
	else
	{
		F(GetSystemTimeAsFileTime)(lpSystemTimeAsFileTime);
	}
}


#ifdef HOOK_ERROR_PROC
DWORD WINAPI OnGetLastError()
{
	//typedef DWORD (WINAPI* OnGetLastError_t)();
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(GetLastError);
	DWORD nErr = 0;

	if (F(GetLastError))
		nErr = F(GetLastError)();

	if (nErr == HOOK_ERROR_NO)
	{
		nErr = HOOK_ERROR_NO;
	}
	return nErr;
}
#endif


#ifdef HOOK_ERROR_PROC
VOID WINAPI OnSetLastError(DWORD dwErrCode)
{
	//typedef DWORD (WINAPI* OnSetLastError_t)(DWORD dwErrCode);
	SUPPRESSORIGINALSHOWCALL;
	ORIGINAL_KRNL(SetLastError);

	if (dwErrCode == HOOK_ERROR_NO)
	{
		dwErrCode = HOOK_ERROR_NO;
	}

	if (F(SetLastError))
		F(SetLastError)(dwErrCode);
}
#endif
