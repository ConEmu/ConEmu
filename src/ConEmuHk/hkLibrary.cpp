
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

#include "../common/Common.h"

#ifdef _DEBUG
#include "../common/WModuleCheck.h"
#endif

#include "hkLibrary.h"
#include "MainThread.h"
#include "DllOptions.h"
#include "../common/ConEmuCheck.h"
#include "../common/MModule.h"
#include "../common/WErrGuard.h"
#include "../common/WObjects.h"

/* **************** */

// ReSharper disable CppInconsistentNaming
struct UNICODE_STRING
{
  USHORT Length;
  USHORT MaximumLength;
  PWSTR  Buffer;
};
enum LDR_DLL_NOTIFICATION_REASON
{
	LDR_DLL_NOTIFICATION_REASON_LOADED = 1,
	LDR_DLL_NOTIFICATION_REASON_UNLOADED = 2,
};
struct LDR_DLL_LOADED_NOTIFICATION_DATA
{
	ULONG Flags;                    //Reserved.
	const UNICODE_STRING* FullDllName;   //The full path name of the DLL module.
	const UNICODE_STRING* BaseDllName;   //The base file name of the DLL module.
	PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
	ULONG SizeOfImage;              //The size of the DLL image, in bytes.
};
struct LDR_DLL_UNLOADED_NOTIFICATION_DATA
{
	ULONG Flags;                    //Reserved.
	const UNICODE_STRING* FullDllName;   //The full path name of the DLL module.
	const UNICODE_STRING* BaseDllName;   //The base file name of the DLL module.
	PVOID DllBase;                  //A pointer to the base address for the DLL in memory.
	ULONG SizeOfImage;              //The size of the DLL image, in bytes.
};
union LDR_DLL_NOTIFICATION_DATA
{
	LDR_DLL_LOADED_NOTIFICATION_DATA Loaded;
	LDR_DLL_UNLOADED_NOTIFICATION_DATA Unloaded;
};
typedef VOID (CALLBACK* PLDR_DLL_NOTIFICATION_FUNCTION)(ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA* NotificationData, PVOID Context);
VOID CALLBACK LdrDllNotification(ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA* NotificationData, PVOID Context);
typedef NTSTATUS (NTAPI* LdrRegisterDllNotification_t)(ULONG Flags, PLDR_DLL_NOTIFICATION_FUNCTION NotificationFunction, PVOID Context, PVOID *Cookie);
typedef NTSTATUS (NTAPI* LdrUnregisterDllNotification_t)(PVOID Cookie);
static LdrRegisterDllNotification_t LdrRegisterDllNotification = nullptr;
static LdrUnregisterDllNotification_t LdrUnregisterDllNotification = nullptr;
static PVOID gpLdrDllNotificationCookie = nullptr;
static NTSTATUS gnLdrDllNotificationState = static_cast<NTSTATUS>(-1);
LdrDllNotificationMode gnLdrDllNotificationUsed = ldr_Unchecked;
// ReSharper restore CppInconsistentNaming


void CheckLdrNotificationAvailable()
{
	static bool bLdrWasChecked = false;
	if (bLdrWasChecked)
		return;

	const bool isAllowed = IsWin6();

	// LdrDllNotification работает так как нам надо начиная с Windows 8
	// В предыдущих версиях Windows нотификатор вызывается из LdrpFindOrMapDll
	// ДО того, как были обработаны импорты функцией LdrpProcessStaticImports (а точнее LdrpSnapThunk)

	if (isAllowed)
	{
		const MModule hNtDll(L"ntdll.dll");
		if (hNtDll.IsValid())
		{
			// Functions are available since Vista
			if (hNtDll.GetProcAddress("LdrRegisterDllNotification", LdrRegisterDllNotification)
				&& hNtDll.GetProcAddress("LdrUnregisterDllNotification", LdrUnregisterDllNotification))
			{
				gnLdrDllNotificationState = LdrRegisterDllNotification(0, LdrDllNotification, nullptr, &gpLdrDllNotificationCookie);
				gnLdrDllNotificationUsed = (gnLdrDllNotificationState != 0/*STATUS_SUCCESS*/) ? ldr_Unavailable
					: IsWin8() ? ldr_FullSupport : ldr_PartialSupport;
			}
		}
	}

	bLdrWasChecked = true;
}

void UnregisterLdrNotification()
{
	if ((gnLdrDllNotificationUsed == ldr_PartialSupport) || (gnLdrDllNotificationUsed == ldr_FullSupport))
	{
		gnLdrDllNotificationUsed = ldr_Unregistered;
		_ASSERTEX(LdrUnregisterDllNotification!=nullptr);
		LdrUnregisterDllNotification(gpLdrDllNotificationCookie);
	}
}


VOID CALLBACK LdrDllNotification(ULONG NotificationReason, const LDR_DLL_NOTIFICATION_DATA* NotificationData, PVOID Context)
{
	ScopedObject(CLastErrorGuard);
	wchar_t szModule[MAX_PATH*2] = L"";
	HMODULE hModule;

	const UNICODE_STRING* FullDllName;   //The full path name of the DLL module.
	const UNICODE_STRING* BaseDllName;   //The base file name of the DLL module.

	switch (NotificationReason)
	{
	case LDR_DLL_NOTIFICATION_REASON_LOADED:
		FullDllName = NotificationData->Loaded.FullDllName;
		BaseDllName = NotificationData->Loaded.BaseDllName;
		hModule = static_cast<HMODULE>(NotificationData->Loaded.DllBase);
		break;
	case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
		FullDllName = NotificationData->Unloaded.FullDllName;
		BaseDllName = NotificationData->Unloaded.BaseDllName;
		hModule = static_cast<HMODULE>(NotificationData->Unloaded.DllBase);
		break;
	default:
		return;
	}

	if (FullDllName && FullDllName->Buffer)
		memmove(szModule, FullDllName->Buffer, std::min<size_t>(sizeof(szModule)-2, FullDllName->Length));
	else if (BaseDllName && BaseDllName->Buffer)
		memmove(szModule, BaseDllName->Buffer, std::min<size_t>(sizeof(szModule)-2, BaseDllName->Length));

	#ifdef _DEBUG
	wchar_t szDbgInfo[MAX_PATH*3];
	swprintf_c(szDbgInfo, L"ConEmuHk: Ldr(%s) " WIN3264TEST(L"0x%08X", L"0x%08X%08X") L" '%s'\n",
		(NotificationReason == LDR_DLL_NOTIFICATION_REASON_LOADED) ? L"Loaded" : L"Unload",
		WIN3264WSPRINT(hModule),
		szModule);
	DebugString(szDbgInfo);
	#endif

	switch (NotificationReason)  // NOLINT(hicpp-multiway-paths-covered)
	{
	case LDR_DLL_NOTIFICATION_REASON_LOADED:
		if (gnLdrDllNotificationUsed != ldr_FullSupport)
		{
			//TODO: Dll logging?
			break;
		}

		if (PrepareNewModule(hModule, nullptr, szModule, TRUE, TRUE))
		{
			HookItem* ph = nullptr;
			GetOriginalAddress(reinterpret_cast<LPVOID>(OnLoadLibraryW), HOOK_FN_ID(LoadLibraryW), nullptr, &ph, true);
			if (ph && ph->PostCallBack)
			{
				SETARGS1(&hModule,szModule);
				ph->PostCallBack(&args);
			}
		}
		break;

	case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
		if (gnLdrDllNotificationUsed != ldr_FullSupport)
		{
			//TODO: Dll logging?
			break;
		}

		UnprepareModule(hModule, szModule, 0);
		UnprepareModule(hModule, szModule, 2);
		break;
	}
}

/* **************** */

#ifdef _DEBUG
void OnLoadLibraryLog(LPCSTR lpLibraryA, LPCWSTR lpLibraryW)
{
	#if 0
	if ((lpLibraryA && strncmp(lpLibraryA, "advapi32", 8)==0)
		|| (lpLibraryW && wcsncmp(lpLibraryW, L"advapi32", 8)==0))
	{
		extern HANDLE ghDebugSshLibs, ghDebugSshLibsRc;
		if (ghDebugSshLibs)
		{
			SetEvent(ghDebugSshLibs);
			WaitForSingleObject(ghDebugSshLibsRc, 1000);
		}
	}
	#endif
}
#else
#define OnLoadLibraryLog(lpLibraryA,lpLibraryW)
#endif

/* ************** */
HMODULE WINAPI OnLoadLibraryA(const char* lpFileName)
{
	//typedef HMODULE(WINAPI* OnLoadLibraryA_t)(const char* lpFileName);
	ORIGINAL_KRNL(LoadLibraryA);

	OnLoadLibraryLog(lpFileName,nullptr);

	HMODULE module = nullptr;
	if (F(LoadLibraryA))
		module = F(LoadLibraryA)(lpFileName);
	ScopedObject(CLastErrorGuard);

	// Issue 1079: Almost hangs with PHP
	if (lstrcmpiA(lpFileName, "kernel32.dll") == 0)
		return module;

	if (PrepareNewModule(module, lpFileName, nullptr))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS1(&module,lpFileName);
			ph->PostCallBack(&args);
		}
	}

	return module;
}

/* ************** */
HMODULE WINAPI OnLoadLibraryW(const wchar_t* lpFileName)
{
	//typedef HMODULE(WINAPI* OnLoadLibraryW_t)(const wchar_t* lpFileName);
	ORIGINAL_KRNL(LoadLibraryW);
	HMODULE module = nullptr;

	OnLoadLibraryLog(nullptr,lpFileName);

	// ExtendedConsole.dll was moved to %ConEmuBaseDir%
	// Also, there are two versions, ExtendedConsole64.dll for 64-bit Far
	// So we need to patch Far load attempt, because it tries to load "%FARHOME%\ExtendedConsole.dll"
	// Don't rely here on isFar or Far mappings - they may be not initialized yet...
	if (lpFileName && F(LoadLibraryW)
		&& ((lstrcmpiW(lpFileName, L"ExtendedConsole.dll") == 0)
			|| lstrcmpiW(lpFileName, L"ExtendedConsole64.dll") == 0))
	{
		CESERVER_CONSOLE_MAPPING_HDR* info = static_cast<CESERVER_CONSOLE_MAPPING_HDR*>(calloc(1, sizeof(*info)));
		if (info && ::LoadSrvMapping(ghConWnd, *info))
		{
			const size_t cchMax = countof(info->ComSpec.ConEmuBaseDir)+64;
			wchar_t* pszFullPath = static_cast<wchar_t*>(calloc(cchMax, sizeof(*pszFullPath)));
			if (pszFullPath)
			{
				_wcscpy_c(pszFullPath, cchMax, info->ComSpec.ConEmuBaseDir);
				_wcscat_c(pszFullPath, cchMax, WIN3264TEST(L"\\ExtendedConsole.dll",L"\\ExtendedConsole64.dll"));

				module = F(LoadLibraryW)(pszFullPath);

				SafeFree(pszFullPath);
			}
		}
		SafeFree(info);
	}

	if (!module && F(LoadLibraryW))
		module = F(LoadLibraryW)(lpFileName);
	ScopedObject(CLastErrorGuard);

	if ((gnLdrDllNotificationUsed == ldr_PartialSupport) || (gnLdrDllNotificationUsed == ldr_FullSupport))
	{
		// Far Manager ConEmu plugin may do some additional operations:
		// such as initialization of background plugins...
		ProcessOnLibraryLoadedW(module);
		return module;
	}

	// Issue 1079: Almost hangs with PHP
	if (lstrcmpi(lpFileName, L"kernel32.dll") == 0)
		return module;

	if (PrepareNewModule(module, nullptr, lpFileName))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS1(&module,lpFileName);
			ph->PostCallBack(&args);
		}
	}

	return module;
}

/* ************** */
HMODULE WINAPI OnLoadLibraryExA(const char* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	//typedef HMODULE(WINAPI* OnLoadLibraryExA_t)(const char* lpFileName, HANDLE hFile, DWORD dwFlags);
	ORIGINAL_KRNL(LoadLibraryExA);

	OnLoadLibraryLog(lpFileName,nullptr);

	HMODULE module = nullptr;
	if (F(LoadLibraryExA))
		module = F(LoadLibraryExA)(lpFileName, hFile, dwFlags);
	ScopedObject(CLastErrorGuard);

	if (PrepareNewModule(module, lpFileName, nullptr))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS3(&module,lpFileName,hFile,dwFlags);
			ph->PostCallBack(&args);
		}
	}

	return module;
}

/* ************** */
HMODULE WINAPI OnLoadLibraryExW(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags)
{
	//typedef HMODULE(WINAPI* OnLoadLibraryExW_t)(const wchar_t* lpFileName, HANDLE hFile, DWORD dwFlags);
	ORIGINAL_KRNL(LoadLibraryExW);

	OnLoadLibraryLog(nullptr,lpFileName);

	HMODULE module = nullptr;
	if (F(LoadLibraryExW))
		module = F(LoadLibraryExW)(lpFileName, hFile, dwFlags);
	ScopedObject(CLastErrorGuard);

	if (PrepareNewModule(module, nullptr, lpFileName))
	{
		if (ph && ph->PostCallBack)
		{
			SETARGS3(&module,lpFileName,hFile,dwFlags);
			ph->PostCallBack(&args);
		}
	}

	return module;
}

/* ************** */
BOOL WINAPI OnFreeLibrary(HMODULE hModule)
{
	//typedef BOOL (WINAPI* OnFreeLibrary_t)(HMODULE hModule);
	ORIGINAL_KRNL(FreeLibrary);
	BOOL lbRc = FALSE;
	const BOOL lbResource = LDR_IS_RESOURCE(hModule);
	// lbResource получается TRUE например при вызовах из version.dll

	UnprepareModule(hModule, nullptr, 0);

#ifdef _DEBUG
	const BOOL lbModulePre = IsModuleValid(hModule); // GetModuleFileName(hModule, szModule, countof(szModule));
#endif

	// Section locking is inadmissible. One FreeLibrary may cause another FreeLibrary in _different_ thread.
	if (F(FreeLibrary))
		lbRc = F(FreeLibrary)(hModule);
	ScopedObject(CLastErrorGuard);

	// Далее только если !LDR_IS_RESOURCE
	if (lbRc && !lbResource)
		UnprepareModule(hModule, nullptr, 1);

#ifdef _DEBUG
	std::ignore = lbModulePre;
#endif
	return lbRc;
}
