
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

#define DROP_SETCP_ON_WIN2K3R2

//#define USE_ONLY_INT_CHECK_PTR
#undef USE_ONLY_INT_CHECK_PTR

// Otherwise GetConsoleAliases fails to define (but it should be already available in Win2k)
#undef _WIN32_WINNT  // NOLINT(clang-diagnostic-reserved-id-macro)
// ReSharper disable once CppInconsistentNaming
#define _WIN32_WINNT 0x0501  // NOLINT(clang-diagnostic-reserved-id-macro)


//#define USECHECKPROCESSMODULES
#define ASSERT_ON_PROCNOTFOUND

#include "../common/defines.h"
// ReSharper disable once CppUnusedIncludeDirective
#include <intrin.h>
#include "../common/Common.h"
#include "../common/crc32.h"
#include "../common/ConEmuCheck.h"
#include "../common/WErrGuard.h"
#include "../common/WModuleCheck.h"
//#include "../common/MArray.h"
#include "Ansi.h"
#include "DefTermHk.h"
#include "GuiAttach.h"
#include "hkCmdExe.h"
#include "hkConsole.h"
#include "hkLibrary.h"
#include "hkStdIO.h"
#include "MainThread.h"
#include "SetHook.h"
#include "ShellProcessor.h"
#include "DllOptions.h"
#if defined(_M_ARM64) || defined(__aarch64__)
#include <detours.h>
#include <tlhelp32.h>
#include <vector>
#else
#include "../modules/minhook/include/MinHook.h"
#endif
#include "../common/HkFunc.h"
#include "../common/MWnd.h"
#include "../common/WObjects.h"


#ifdef _DEBUG
	//WARNING!!! OutputDebugString must NOT be used from ConEmuHk::DllMain(DLL_PROCESS_DETACH). See Issue 465
	#define DebugString(x) //if ((gnDllState != ds_DllProcessDetach) || gbIsSshProcess) OutputDebugString(x)
	#define DebugStringA(x) //if ((gnDllState != ds_DllProcessDetach) || gbIsSshProcess) OutputDebugStringA(x)
#else
	#define DebugString(x)
	#define DebugStringA(x)
#endif


extern HWND ghConWnd;      // RealConsole  // NOLINT(readability-redundant-declaration)

extern bool gbPrepareDefaultTerminal;  // NOLINT(readability-redundant-declaration)

extern FARPROC CallWriteConsoleW;  // NOLINT(readability-redundant-declaration)

extern GetConsoleWindow_T gfGetRealConsoleWindow; // from ConEmuCheck.cpp

#ifdef _DEBUG
bool gbSuppressShowCall = false;
bool gbSkipSuppressShowCall = false;
bool gbSkipCheckProcessModules = false;
#endif

// Called function indexes logging
namespace HookLogger
{
	// #define HOOK_LOG_MAX 1024 // Must be a power of 2
	FnCall g_calls[HOOK_LOG_MAX];
	LONG   g_callsIdx = -1;
};

#if !defined(_M_ARM64) && !defined(__aarch64__)
MH_STATUS g_mhInit = MH_UNKNOWN;
MH_STATUS g_mhCreate = MH_UNKNOWN;
MH_STATUS g_mhEnable = MH_UNKNOWN;
MH_STATUS g_mhCritical = MH_UNKNOWN;
MH_STATUS g_mhDisableAll = MH_UNKNOWN;
MH_STATUS g_mhDeinit = MH_UNKNOWN;
#endif


//!!!All dll names MUST BE LOWER CASE!!!
//!!!WARNING!!! Modifying this don't forget the GetPreloadModules() !!!
const wchar_t KERNELBASE[] = L"kernelbase.dll";	const wchar_t KERNELBASE_NOEXT[] = L"kernelbase";
const wchar_t KERNEL32[] = L"kernel32.dll";		const wchar_t KERNEL32_NOEXT[] = L"kernel32";
const wchar_t USER32[]   = L"user32.dll";		const wchar_t USER32_NOEXT[]   = L"user32";
const wchar_t GDI32[]    = L"gdi32.dll";		const wchar_t GDI32_NOEXT[]    = L"gdi32";
const wchar_t SHELL32[]  = L"shell32.dll";		const wchar_t SHELL32_NOEXT[]  = L"shell32";
const wchar_t ADVAPI32[] = L"advapi32.dll";		const wchar_t ADVAPI32_NOEXT[] = L"advapi32";
//!!!WARNING!!! Modifying this don't forget the GetPreloadModules() !!!
HMODULE
	ghKernelBase = nullptr,
	ghKernel32 = nullptr,
	ghUser32 = nullptr,
	ghGdi32 = nullptr,
	ghShell32 = nullptr,
	ghAdvapi32 = nullptr;
HMODULE* ghSysDll[] = {
	&ghKernelBase,
	&ghKernel32,
	&ghUser32,
	&ghGdi32,
	&ghShell32,
	&ghAdvapi32};
//!!!WARNING!!! Добавляя в этот список - не забыть добавить и в GetPreloadModules() !!!


// Forwards

void LogModuleLoaded(LPCWSTR pwszModule, HMODULE hModule);
void LogModuleUnloaded(LPCWSTR pwszModule, HMODULE hModule);


struct PreloadFuncs {
	LPCSTR	sFuncName;
	void**	pFuncPtr;
};
struct PreloadModules {
	LPCWSTR      sModule, sModuleNoExt;
	HMODULE     *pModulePtr;
	PreloadFuncs Funcs[5];
};

size_t GetPreloadModules(PreloadModules** ppModules)
{
	static size_t snModulesCount = 0;
	static PreloadModules* spModules = nullptr;
	if (spModules)
	{
		if (ppModules)
			*ppModules = spModules;
		return snModulesCount;
	}

	PreloadModules Modules[] =
	{
		{GDI32,		GDI32_NOEXT,	&ghGdi32, {}},
		{SHELL32,	SHELL32_NOEXT,	&ghShell32, {}},
		{ADVAPI32,	ADVAPI32_NOEXT,	&ghAdvapi32, {}},
	};
	snModulesCount = countof(Modules);
	spModules = new PreloadModules[snModulesCount];
	if (!spModules)
		return 0;
	memmove(spModules, Modules, sizeof(Modules));
	if (ppModules)
		*ppModules = spModules;
	return snModulesCount;
}

void CheckLoadedModule(LPCWSTR asModule)
{
	if (!asModule || !*asModule)
		return;

	PreloadModules* Checks = nullptr;
	const size_t nChecks = GetPreloadModules(&Checks);

	for (size_t m = 0; m < nChecks; m++)
	{
		if ((*Checks[m].pModulePtr) != nullptr)
			continue;

		if (!lstrcmpiW(asModule, Checks[m].sModule) || !lstrcmpiW(asModule, Checks[m].sModuleNoExt))
		{
			*Checks[m].pModulePtr = LoadLibraryW(Checks[m].sModule); // LoadLibrary, increment the counter
			if ((*Checks[m].pModulePtr) != nullptr)
			{
				_ASSERTEX(Checks[m].Funcs[countof(Checks[m].Funcs)-1].sFuncName == nullptr);

				for (size_t f = 0; f < countof(Checks[m].Funcs) && Checks[m].Funcs[f].sFuncName; f++)
				{
					*Checks[m].Funcs[f].pFuncPtr = reinterpret_cast<void*>(GetProcAddress(*Checks[m].pModulePtr, Checks[m].Funcs[f].sFuncName));
				}
			}
		}
	}
}

void FreeLoadedModule(HMODULE hModule)
{
	if (!hModule)
		return;

	PreloadModules* Checks = nullptr;
	const size_t nChecks = GetPreloadModules(&Checks);

	for (size_t m = 0; m < nChecks; m++)
	{
		if ((*Checks[m].pModulePtr) != hModule)
			continue;

		if (GetModuleHandle(Checks[m].sModule) == nullptr)
		{
			// По идее, такого быть не должно, т.к. счетчик мы накрутили, библиотека не должна была выгрузиться
			_ASSERTEX(*Checks[m].pModulePtr == nullptr);

			*Checks[m].pModulePtr = nullptr;

			_ASSERTEX(Checks[m].Funcs[countof(Checks[m].Funcs)-1].sFuncName == nullptr);

			for (size_t f = 0; f < countof(Checks[m].Funcs) && Checks[m].Funcs[f].sFuncName; f++)
			{
				*Checks[m].Funcs[f].pFuncPtr = nullptr;
			}
		}
	}
}


///
/// Let's know all processed modules
///

HkModuleMap* gpHookedModules = nullptr;

bool InitializeHookedModules()
{
	if (!gpHookedModules)
	{
		//MessageBox(nullptr, L"InitializeHookedModules", L"Hooks", MB_SYSTEMMODAL);

		//WARNING: "new" вызывать из DllStart нельзя! DllStart вызывается НЕ из главной нити,
		//WARNING: причем, когда главная нить еще не была запущена. В итоге, если это
		//WARNING: попытаться сделать мы получим:
		//WARNING: runtime error R6030  - CRT not initialized
		// -- gpHookedModules = new MArray<HkModuleInfo>;
		// -- поэтому тупо через массив
		HkModuleMap* pMap = static_cast<HkModuleMap*>(calloc(sizeof(HkModuleMap), 1));
		if (!pMap)
		{
			_ASSERTE(pMap!=nullptr);
		}
		else if (!pMap->Init())
		{
			free(pMap);
		}
		else
		{
			gpHookedModules = pMap;
		}
	}

	return (gpHookedModules != nullptr);
}

void FinalizeHookedModules()
{
	HLOG1("FinalizeHookedModules",0);
	if (gpHookedModules)
	{
		HkModuleInfo** pp = nullptr;
		const INT_PTR iCount = gpHookedModules->GetKeysValues(nullptr, &pp);
		if (iCount > 0)
		{
			for (INT_PTR i = 0; i < iCount; i++)
				free(pp[i]);
			gpHookedModules->ReleasePointer(pp);
		}
		gpHookedModules->Release();
		SafeFree(gpHookedModules);
	}
	HLOGEND1();
}

HkModuleInfo* IsHookedModule(HMODULE hModule, LPWSTR pszName = nullptr, size_t cchNameMax = 0)
{
	// Must be initialized already, but JIC
	if (!gpHookedModules)
		InitializeHookedModules();

	if (!gpHookedModules)
	{
		_ASSERTE(gpHookedModules!=nullptr);
		return nullptr;
	}

	//bool lbHooked = false;

	HkModuleInfo* p = nullptr;
	if (gpHookedModules->Get(hModule, &p))
	{
		//_ASSERTE(p->Hooked == 1 || p->Hooked == 2);
		//lbHooked = true;

		// If we need to retrieve module name by its handle
		if (pszName && cchNameMax)
		{
			lstrcpyn(pszName, p->sModuleName, static_cast<int>(cchNameMax));
		}
	}

	return p;
}

HkModuleInfo* AddHookedModule(HMODULE hModule, LPCWSTR sModuleName)
{
	// Must be initialized already, but JIC
	if (!gpHookedModules)
		InitializeHookedModules();

	_ASSERTE(gpHookedModules);
	if (!gpHookedModules)
	{
		_ASSERTE(gpHookedModules!=nullptr);
		return nullptr;
	}

	HkModuleInfo* p = IsHookedModule(hModule);

	if (!p)
	{
		p = static_cast<HkModuleInfo*>(calloc(sizeof(HkModuleInfo), 1));
		if (!p)
		{
			_ASSERTE(p!=nullptr);
		}
		else
		{
			p->hModule = hModule;

			lstrcpyn(p->sModuleName, sModuleName?sModuleName:L"", countof(p->sModuleName));
			//_ASSERTEX(lstrcmpi(p->sModuleName,L"dsound.dll"));

			gpHookedModules->Set(hModule, p);
		}
	}

	return p;
}

void RemoveHookedModule(HMODULE hModule)
{
	// Must be initialized already, but JIC
	if (!gpHookedModules)
		InitializeHookedModules();

	_ASSERTE(gpHookedModules);
	if (!gpHookedModules)
	{
		_ASSERTE(gpHookedModules!=nullptr);
		return;
	}

	HkModuleInfo* p = nullptr;
	if (gpHookedModules->Get(hModule, &p, true/*Remove*/))
	{
		SafeFree(p);
	}
}



//typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);
HMODULE ghOnLoadLibModule = nullptr;
OnLibraryLoaded_t gfOnLibraryLoaded = nullptr;
OnLibraryLoaded_t gfOnLibraryUnLoaded = nullptr;


HookItem *gpHooks = nullptr; // [MAX_HOOKED_PROCS]
size_t gnHookedFuncs = 0;


BOOL gbLogLibraries = FALSE;
DWORD gnLastLogSetChange = 0;


// Используется в том случае, если требуется выполнить оригинальную функцию, без нашей обертки
// пример в OnPeekConsoleInputW
void* __cdecl GetOriginalAddress(void* OurFunction, DWORD nFnID /*= 0*/, void* ApiFunction/*= nullptr*/, HookItem** ph/*= nullptr*/, bool abAllowNulls /*= false*/)
{
	void* lpfn = nullptr;

	// Initialized?
	if (!gpHooks || !HooksWereSetRaw)
	{
		goto wrap;
	}

	// We must know function index in the gpHooks
	if (nFnID && (nFnID <= gnHookedFuncs))
	{
		const size_t nIdx = nFnID - 1;
		if (gpHooks[nIdx].NewAddress == OurFunction)
		{
			if (ph) *ph = &(gpHooks[nIdx]);
			// Return address where we may call "original" function
			lpfn = gpHooks[nIdx].CallAddress;
			HookLogger::Log(nFnID);
			InterlockedIncrement(&gpHooks[nIdx].Counter);
			goto wrap;
		}
	}

	// That is strange. Try to find via pointer
	// gh-888: Due AllocConsole from DefTerm (GUI app) only limited list of functions may be hooked
	_ASSERTE((abAllowNulls || gbPrepareDefaultTerminal) && "Function index was not detected");
	for (int i = 0; gpHooks[i].NewAddress; i++)
	{
		if (gpHooks[i].NewAddress == OurFunction)
		{
			if (ph) *ph = &(gpHooks[i]);
			// Return address where we may call "original" function
			lpfn = gpHooks[i].CallAddress;
			HookLogger::Log(i+1); // Function ID
			InterlockedIncrement(&gpHooks[i].Counter);
			goto wrap;
		}
	}

wrap:
	// Not yet hooked?
	if (!lpfn && !abAllowNulls && ApiFunction)
	{
		#ifdef _DEBUG
		if (HooksWereSet)
		{
			static bool asserted = false;
			_ASSERT(asserted && "The function was not hooked before");
			if (gbPrepareDefaultTerminal)
				asserted = true;
		}
		#endif
		if (ph) *ph = nullptr;
		lpfn = ApiFunction;
	}

	_ASSERT(lpfn || (!HooksWereSet || abAllowNulls));
	return lpfn;
}

// Used in GetLoadLibraryAddress, however it may be obsolete with minhook
FARPROC WINAPI GetLoadLibraryW()
{
	// KERNEL ADDRESS
	LPVOID fnLoadLibraryW = nullptr; // Can't use (LPVOID)&LoadLibraryW anymore, our imports are changed
	HookItem* p = nullptr;
	if (GetOriginalAddress(reinterpret_cast<LPVOID>(OnLoadLibraryW), HOOK_FN_ID(LoadLibraryW), nullptr, &p, true))
	{
		fnLoadLibraryW = p->HookedAddress;
		if (!fnLoadLibraryW)
		{
			_ASSERTEX(p->HookedAddress && "LoadLibraryW was not hooked?");
			fnLoadLibraryW = reinterpret_cast<LPVOID>(&LoadLibraryW);
		}
	}
	else
	{
		// fnLoadLibraryW could be nullptr if we don't have to hook LoadLibraryW at all
		_ASSERTEX(gnLdrDllNotificationUsed == ldr_FullSupport && "Failed to find function address in Kernel32");
	}
	return reinterpret_cast<FARPROC>(fnLoadLibraryW);
}

FARPROC WINAPI GetWriteConsoleW()
{
	return reinterpret_cast<FARPROC>(GetOriginalAddress(reinterpret_cast<LPVOID>(OnWriteConsoleW), HOOK_FN_ID(WriteConsoleW)));
}

// Our modules (ConEmuCD.dll, ConEmuDW.dll, ConEmu.dll and so on)
// need and ready to call unhooked functions (trampolines)
BOOL WINAPI RequestTrampolines(LPCWSTR asModule, HMODULE hModule)
{
	if (!gpHooks)
		return FALSE;
	const bool bRc = SetImports(asModule, hModule, TRUE);
	return bRc;
}



DWORD CalculateNameCRC32(const char *name)
{
	const DWORD dwRead = lstrlenA(name);
	DWORD nCRC32 = 0xFFFFFFFF;
	if (!CalcCRC(reinterpret_cast<const BYTE*>(name), dwRead, nCRC32))
		return 0;
	// as we don't need a real CRC32, but only a hash for comparison, last XOR is not required
	return nCRC32;
}


// Fill the HookItem.OldAddress (real procedures from external libs)
// apHooks->Name && apHooks->DllName MUST be for a lifetime
int InitHooks(HookItem* apHooks)
{
	int iFunc = 0;
	DWORD i, j;
	bool skip;

	// Init gnLdrDllNotificationUsed. Supported only in Win8 and higher.
	if (!gnLdrDllNotificationUsed)
	{
		CheckLdrNotificationAvailable();
	}

	if (!InitializeHookedModules())
	{
		return -3;
	}

	if (gpHooks == nullptr)
	{
#if !defined(_M_ARM64) && !defined(__aarch64__)
		// gh#250: Fight with CreateToolhelp32Snapshot lags
		MH_INITIALIZE mhInit = {sizeof(mhInit), 0, nullptr, nullptr, nullptr, nullptr};
		mhInit.Flags = MH_FLAGS_SKIP_EXEC_CHECK;
		if (gbPrepareDefaultTerminal)
		{
			// Let use ‘standard’ enumerations in DefTerm mode
			mhInit.ThreadListCreate = CreateToolhelp32Snapshot;
			mhInit.ThreadListFirst = Thread32First;
			mhInit.ThreadListNext = Thread32Next;
			mhInit.ThreadListClose = CloseHandle;
		}
		else
		{
			// Use our internal enumerator
			mhInit.ThreadListCreate = HookThreadListCreate;
			mhInit.ThreadListFirst = HookThreadListNext;
			mhInit.ThreadListNext = HookThreadListNext;
			mhInit.ThreadListClose = HookThreadListClose;
		}

		g_mhInit = MH_InitializeEx(&mhInit);
		if (g_mhInit != MH_OK)
		{
			_ASSERTE(g_mhInit == MH_OK);
			return -1;
		}
#endif

		gpHooks = static_cast<HookItem*>(calloc(sizeof(HookItem), MAX_HOOKED_PROCS));
		if (!gpHooks)
		{
			return -2;
		}

		// Load kernelbase
		// cmd.exe calls functions from KernelBase.dll but not from kernel32.dll
		// Actually, cmd.exe links to set of api-ms-win-core-*.dll libraries
		// But that is the behavior from Windows 8.0 and higher only!
		static bool KernelHooked = false;
		if (!KernelHooked)
		{
			KernelHooked = true;

			_ASSERTEX(ghKernel32 != nullptr);
			// Windows 7 still uses kernel32.dll,
			// 64-bit version of MultiRun was printed unprocessed ANSI
			if (IsWin8())
			{
				ghKernelBase = LoadLibrary(KERNELBASE);
			}
		}
	}

	if (apHooks && gpHooks)
	{
		for (i = 0; apHooks[i].NewAddress; i++)
		{
			const DWORD NameCRC = CalculateNameCRC32(apHooks[i].Name);

			if (apHooks[i].Name==nullptr || apHooks[i].DllName==nullptr)
			{
				_ASSERTE(apHooks[i].Name!=nullptr && apHooks[i].DllName!=nullptr);
				break;
			}

			// ReSharper disable once CppJoinDeclarationAndAssignment
			skip = false;

			// ReSharper disable once CppJoinDeclarationAndAssignment
			j = 0; // using while, because of j

			while (gpHooks[j].NewAddress)
			{
				if (gpHooks[j].NewAddress == apHooks[i].NewAddress)
				{
					_ASSERTEX((gnDllState & ds_HooksStarted) && "Function NewAddress is not unique! Skipped!");
					skip = true; break;
				}

				if (gpHooks[j].NameCRC == NameCRC
					&& strcmp(gpHooks[j].Name, apHooks[i].Name) == 0
					&& wcscmp(gpHooks[j].DllName, apHooks[i].DllName) == 0)
				{
					_ASSERTEX(FALSE && "Function name/library is not unique! Skipped!");
					skip = true;
					break;
				}

				j++;
			}

			if (skip) continue;


			if ((j+1) >= MAX_HOOKED_PROCS)
			{
				// Maximum possible hooked function count was reached
				_ASSERTE((j+1) < MAX_HOOKED_PROCS);
				break;
			}

			// Store new function
			gpHooks[j] = apHooks[i];
			// Set function ID and NameCRC
			if (apHooks[i].pnFnID)
				*apHooks[i].pnFnID = j+1; // 1-based ID
			gpHooks[j].NameCRC = NameCRC;
			_ASSERTEX(gpHooks[j].Counter == 0);

			// Increase total hooked count
			_ASSERTEX(j >= gnHookedFuncs);
			gnHookedFuncs = j+1;

			// Clear next item just in case
			gpHooks[j+1].Name = nullptr;
			gpHooks[j+1].NewAddress = nullptr;
		}
	}

	// Local variables to speed up GetModuleHandle calls
	HMODULE hLastModule = nullptr;
	LPCWSTR pszLastModule = nullptr;

	// Determine original function addresses
	for (i = 0; gpHooks[i].NewAddress; i++)
	{
		if (gpHooks[i].DllNameA[0] == 0)
		{
			const int nLen = WideCharToMultiByte(CP_ACP, 0, gpHooks[i].DllName, -1, gpHooks[i].DllNameA,
				static_cast<int>(countof(gpHooks[i].DllNameA)), nullptr,nullptr);
			if (nLen > 0)
				CharLowerBuffA(gpHooks[i].DllNameA, nLen);
		}

		if (!gpHooks[i].HookedAddress)
		{
			// If we need to hook exact library (kernel32) instead of KernelBase
			const HMODULE hRequiredMod = gpHooks[i].hDll;

			// Don't load them now with LoadLibrary, process only already loaded modules
			HMODULE mod = hRequiredMod;
			if (!hRequiredMod)
			{
				// Speed up GetModuleHandle calls
				if (pszLastModule == gpHooks[i].DllName)
				{
					mod = hLastModule;
				}
				else
				{
					pszLastModule = gpHooks[i].DllName;
					hLastModule = mod = GetModuleHandle(pszLastModule);
				}
			}

			if (mod == nullptr)
			{
				_ASSERTE(mod != nullptr
					// Библиотеки, которые могут быть НЕ подлинкованы на старте
					|| (gpHooks[i].DllName == SHELL32
						|| gpHooks[i].DllName == USER32
						|| gpHooks[i].DllName == GDI32
						|| gpHooks[i].DllName == ADVAPI32
						));
			}
			else
			{
				// NB, we often get XXXStub instead of the function itself
				const char* exportName = gpHooks[i].NameOrdinal
					? reinterpret_cast<const char*>(static_cast<DWORD_PTR>(gpHooks[i].NameOrdinal))
					: gpHooks[i].Name;

				//TODO: In fact, we need to hook BOTH kernel32.dll and Kernelbase.dll   *
				//TODO: But that is subject to change our code... otherwise we may get  *
				//TODO: problems with double hooked calls                               *

				if (ghKernelBase           // If Kernelbase.dll is loaded
					&& (mod == ghKernel32) // Than, for kernel32.dll we'll try to hook them in Kernelbase.dll
					&& !hRequiredMod       // But, some kernel function must be hooked in the kernel32.dll itself (ExitProcess)
					)
				{
					if (!((gpHooks[i].HookedAddress = reinterpret_cast<void*>(GetProcAddress(ghKernelBase, exportName)))))
					{
						// Strange, most kernel functions are expected to be in KernelBase now
						gpHooks[i].HookedAddress = reinterpret_cast<void*>(GetProcAddress(mod, exportName));
					}
					else
					{
						mod = ghKernelBase;
					}
				}
				else
				{
					gpHooks[i].HookedAddress = reinterpret_cast<void*>(GetProcAddress(mod, exportName));
				}

				if (gpHooks[i].HookedAddress != nullptr)
				{
					iFunc++;
				}
				#ifdef _DEBUG
				else
				{
					// WinXP does not have many hooked functions, don't show dozens of asserts
					_ASSERTE(!IsWin7() || (gpHooks[i].HookedAddress != nullptr));
				}
				#endif

				gpHooks[i].hDll = mod;
			}
		}
	}

	return iFunc;
}

HookItem* FindFunction(const char* pszFuncName)
{
	const DWORD nameCrc = CalculateNameCRC32(pszFuncName);

	for (HookItem* p = gpHooks; p->NewAddress; ++p)
	{
		if (p->NameCRC == nameCrc)
		{
			if (strcmp(p->Name, pszFuncName) == 0)
				return p;
		}
	}

	//HookItemNode* pc = FindFunctionNode(gpHooksRoot, pszFuncName);
	//if (pc)
	//	return pc->p;

	return nullptr;
}


// Main initialization routine
bool StartupHooks()
{
	_ASSERTE(!IsConsoleServer(gsExeName));
	if (gbConEmuCProcess)
	{
		// ConEmuC.exe and ConEmuC64.exe must not be "hooked"
		return false;
	}

	//HLOG0("StartupHooks",0);
	gnDllState |= ds_HooksStarting;

	#ifdef _DEBUG
	// real console window handle should be already initialized in DllMain
	const MWnd hRealConsole(GetRealConsoleWindow());
	_ASSERTE(gbAttachGuiClient || gbDosBoxProcess || gbPrepareDefaultTerminal || (hRealConsole == nullptr) || (ghConWnd != nullptr && ghConWnd == hRealConsole));
	wchar_t sClass[128];
	if (ghConWnd)
	{
		GetClassName(ghConWnd, sClass, countof(sClass));
		_ASSERTE(IsConsoleClass(sClass));
	}

	prepare_timings;
	#endif

	// -- ghConEmuWnd уже должен быть установлен в DllMain!!!
	//gbInShellExecuteEx = FALSE;

	WARNING("Получить из мэппинга gdwServerPID");

	//TODO: Change GetModuleHandle to GetModuleHandleEx? Does it exist in Win2k?

	// Зовем LoadLibrary. Kernel-то должен был сразу загрузиться (static link) в любой
	// windows приложении, но вот shell32 - не обязательно, а нам нужно хуки проинициализировать
	ghKernel32 = LoadLibrary(KERNEL32);
	// user32/shell32/advapi32 тянут за собой много других библиотек, НЕ загружаем, если они еще не подлинкованы
	if (!ghUser32)
	{
		ghUser32 = GetModuleHandle(USER32);
		if (ghUser32) ghUser32 = LoadLibrary(USER32); // если подлинкован - увеличить счетчик
	}
	ghShell32 = GetModuleHandle(SHELL32);
	if (ghShell32)
		ghShell32 = LoadLibrary(SHELL32); // если подлинкован - увеличить счетчик
	ghAdvapi32 = GetModuleHandle(ADVAPI32);
	if (ghAdvapi32)
		ghAdvapi32 = LoadLibrary(ADVAPI32); // если подлинкован - увеличить счетчик

	if (ghKernel32)
		gfGetProcessId = reinterpret_cast<GetProcessId_t>(GetProcAddress(ghKernel32, "GetProcessId"));

	// Prepare array and check basic requirements (LdrNotification, LoadLibrary, etc.)
	InitHooks(nullptr);


	// Populate hooked function list
	if (!gbPrepareDefaultTerminal)
	{
		// General (console & ChildGui)
		HLOG1("StartupHooks.InitHooks",0);
		InitHooksCommon();
		HLOGEND1();
	}
	else
	{
		// Default Terminal?
		HLOG1("StartupHooks.InitHooks",0);
		InitHooksDefTerm();
		HLOGEND1();
	}

	// List of functions is prepared

	// Now we call minhook engine to ‘detour’ the API
	print_timings(L"SetAllHooks");
	HLOG1("SetAllHooks",0);
	const bool lbRc = SetAllHooks();
	if (!lbRc)
	{
		gnDllState &= ~ds_HooksStarted;
		gnDllState |= ds_HooksStartFailed;
	}
	HLOGEND1();

	print_timings(L"SetAllHooks - done");

	return lbRc;
}

void ShutdownHooks()
{
	gnDllState |= ds_HooksStopping;

	HLOG1("ShutdownHooks.UnsetImports", 0);
	UnsetImports();
	HLOGEND1();

	HLOG1_("ShutdownHooks.UnsetAllHooks",0);
	UnsetAllHooks();
	HLOGEND1();

	HLOG1_("ShutdownHooks.FreeLibrary",1);
	for (auto& sysDll : ghSysDll)
	{
		if (sysDll && *sysDll)
		{
			FreeLibrary(*sysDll);
			*sysDll = nullptr;
		}
	}
	HLOGEND1();

	FinalizeHookedModules();

	gnDllState |= ds_HooksStopped;
}

void __stdcall SetLoadLibraryCallback(HMODULE ahCallbackModule, OnLibraryLoaded_t afOnLibraryLoaded, OnLibraryLoaded_t afOnLibraryUnLoaded)
{
	ghOnLoadLibModule = ahCallbackModule;
	gfOnLibraryLoaded = afOnLibraryLoaded;
	gfOnLibraryUnLoaded = afOnLibraryUnLoaded;
}

bool __stdcall SetHookCallbacks(const char* ProcName, const wchar_t* DllName, HMODULE hCallbackModule,
                                HookItemCallback_t PreCallBack, HookItemCallback_t PostCallBack,
                                HookItemCallback_t ExceptCallBack)
{
	if (!ProcName|| !DllName)
	{
		_ASSERTE(ProcName!=nullptr && DllName!=nullptr);
		return false;
	}
	_ASSERTE(ProcName[0]!=0 && DllName[0]!=0);

	bool bFound = false;

	if (gpHooks)
	{
		for (int i = 0; i<MAX_HOOKED_PROCS && gpHooks[i].NewAddress; i++)
		{
			if (!strcmp(gpHooks[i].Name, ProcName) && !lstrcmpW(gpHooks[i].DllName,DllName))
			{
				gpHooks[i].hCallbackModule = hCallbackModule;
				gpHooks[i].PreCallBack = PreCallBack;
				gpHooks[i].PostCallBack = PostCallBack;
				gpHooks[i].ExceptCallBack = ExceptCallBack;
				bFound = true;
				//break; // перехватов может быть более одного (деление хуков на exe/dll)
			}
		}
	}

	return bFound;
}

bool FindModuleFileName(HMODULE ahModule, LPWSTR pszName, size_t cchNameMax)
{
	bool lbFound = false;

	//TH32CS_SNAPMODULE - may hang during LoadLibrary/FreeLibrary.
	lbFound = (IsHookedModule(ahModule, pszName, cchNameMax) != nullptr);

	return lbFound;
}

LPCWSTR FormatModuleHandle(HMODULE ahModule, LPCWSTR asFmt32, LPCWSTR asFmt64, LPWSTR pszName, size_t cchNameMax)
{
	#ifdef _WIN64
	if (reinterpret_cast<DWORD_PTR>(ahModule) > 0xFFFFFFFFULL)
	{
		msprintf(pszName, cchNameMax, asFmt64 ? asFmt64 : L"Module=0x%08X%08X", WIN3264WSPRINT(ahModule));
	}
	else
	#endif
	{
		msprintf(pszName, cchNameMax, asFmt32 ? asFmt32 : L"Module=0x%08X", LODWORD(ahModule));
	}
	return pszName;
}




// Let our modules use trampolined (original) versions without superfluous steps
bool SetImportsPrep(LPCWSTR asModule, HMODULE Module, IMAGE_NT_HEADERS* nt_header, BOOL abForceHooks, IMAGE_IMPORT_DESCRIPTOR* Import, size_t ImportCount, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p);
bool SetImportsChange(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p);

// Replace *imported* functions in the *Module*
//	if (abForceHooks == FALSE) than hooks aren't set if we found an import mismatched the original address
//	that's to avoid multiple hook set with multiple LoadLibrary calls
bool SetImports(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks)
{
	IMAGE_IMPORT_DESCRIPTOR* Import = nullptr;
	DWORD Size = 0;
	#ifdef _DEBUG
	const HMODULE hExecutable = GetModuleHandle(nullptr);
	std::ignore = hExecutable;
	#endif

	if (!gpHooks)
		return false;

	if (!Module)
		return false;

	// If our module is already processed - there's nothing to do
	HkModuleInfo* p = IsHookedModule(Module);
	/// TODO: On startup only imports from kernel32 and user32 modules are processed
	/// TODO: Other imports (gdi32?) may be left unprocessed and our modules will use hooked variants
	/// TODO: Not a big problem though
	if (p && p->Hooked != HkModuleState::NotProcessed)
		return true;

	/* No need to do superfluous checks on our modules
	if (!IsModuleValid(Module))
		return false;
	*/

	IMAGE_DOS_HEADER* dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(Module);
	IMAGE_NT_HEADERS* nt_header = nullptr;

	HLOG0("SetImports.Init",(DWORD)Module);

	if (dos_header->e_magic == IMAGE_DOS_SIGNATURE /*'ZM'*/)
	{
		nt_header = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<char*>(Module) + dos_header->e_lfanew);
		if (IsBadReadPtr(nt_header, sizeof(IMAGE_NT_HEADERS)))
			return false;

		if (nt_header->Signature != 0x004550)
			return false;
		else
		{
			Import = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(reinterpret_cast<char*>(Module) +
				static_cast<DWORD>(nt_header->OptionalHeader.
					DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress));
			Size = nt_header->OptionalHeader.
			       DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;
		}
		HLOGEND();
	}
	else
		return false;

	// if wrong module or no import table
	if (!Import)
		return false;


	#ifdef _DEBUG
	const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt_header); //-V220
	std::ignore = section;
	#endif
	_ASSERTE(sizeof(DWORD_PTR) == WIN3264TEST(4, 8));

	// Module is valid, but was not processed yet
	if (!p)
	{
		HLOG("SetImports.Add",(DWORD)Module);
		p = AddHookedModule(Module, asModule);
		HLOGEND();
		if (!p)
			return false;
	}
	// Set it now, to be sure
	p->Hooked = HkModuleState::ImportsChanged;

	HLOG("SetImports.Prepare",(DWORD)Module);
	bool res = false, bHooked = false;
	const INT_PTR nCount = Size / sizeof(IMAGE_IMPORT_DESCRIPTOR);
	bool bFnNeedHook[MAX_HOOKED_PROCS] = {};
	// Separate function to allow exception handlers
	res = SetImportsPrep(asModule, Module, nt_header, abForceHooks, Import, nCount, bFnNeedHook, p);
	HLOGEND();

	HLOG("SetImports.Change",(DWORD)Module);
	// Separate function to allow exception handlers
	bHooked = SetImportsChange(asModule, Module, abForceHooks, bFnNeedHook, p);
	HLOGEND();

	#ifdef _DEBUG
	if (bHooked)
	{
		HLOG("SetImports.FindModuleFileName",(DWORD)Module);
		CEStr szDbg, szModPath;
		const size_t cchDbgMax = MAX_PATH * 3;
		const size_t cchPathMax = MAX_PATH * 2;
		if (szModPath.GetBuffer(cchPathMax))
			FindModuleFileName(Module, szModPath.data(), cchPathMax);
		if (szDbg.GetBuffer(cchDbgMax))
		{
			_wcscpy_c(szDbg.data(), cchDbgMax, L"  ## Imports were changed by conemu: ");
			_wcscat_c(szDbg.data(), cchDbgMax, szModPath);
			_wcscat_c(szDbg.data(), cchDbgMax, L"\n");
			DebugString(szDbg.c_str(L""));
		}
		HLOGEND();
	}
	#endif

	return (res && bHooked);
}

bool isBadModulePtr(const void *lp, UINT_PTR ucb, HMODULE Module, const IMAGE_NT_HEADERS* nt_header)
{
	const bool bTestValid = (static_cast<LPCBYTE>(lp) >= reinterpret_cast<LPCBYTE>(Module))
		&& ((static_cast<LPCBYTE>(lp) + ucb) <= (reinterpret_cast<LPCBYTE>(Module) + nt_header->OptionalHeader.SizeOfImage));

#ifdef USE_ONLY_INT_CHECK_PTR
	bool bApiValid = bTestValid;
#else
	const bool bApiValid = !IsBadReadPtr(lp, ucb);

	#ifdef _DEBUG
	static bool bFirstAssert = false;
	if (bTestValid != bApiValid)
	{
		if (!bFirstAssert)
		{
			bFirstAssert = true;
			_ASSERTE(bTestValid != bApiValid);
		}
	}
	#endif
#endif

	return !bApiValid;
}

bool isBadModuleStringA(LPCSTR lpsz, const UINT_PTR ucchMax, HMODULE Module, IMAGE_NT_HEADERS* nt_header)
{
	const bool bTestStrValid = (reinterpret_cast<LPCBYTE>(lpsz) >= reinterpret_cast<LPCBYTE>(Module))
		&& ((reinterpret_cast<LPCBYTE>(lpsz) + ucchMax) <= (reinterpret_cast<LPCBYTE>(Module) + nt_header->OptionalHeader.SizeOfImage));

#ifdef USE_ONLY_INT_CHECK_PTR
	bool bApiStrValid = bTestStrValid;
#else
	const bool bApiStrValid = !IsBadStringPtrA(lpsz, ucchMax);

	#ifdef _DEBUG
	static bool bFirstAssert = false;
	if (bTestStrValid != bApiStrValid)
	{
		if (!bFirstAssert)
		{
			bFirstAssert = true;
			_ASSERTE(bTestStrValid != bApiStrValid);
		}
	}
	#endif
#endif

	return !bApiStrValid;
}

bool SetImportsPrep(LPCWSTR asModule, HMODULE Module, IMAGE_NT_HEADERS* nt_header, BOOL abForceHooks, IMAGE_IMPORT_DESCRIPTOR* Import, size_t ImportCount, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p)
{
	bool res = false;
	// ReSharper disable once CppJoinDeclarationAndAssignment
	size_t i;

	#define GetPtrFromRVA(rva,pNTHeader,imageBase) (PVOID)((imageBase)+(rva))

	//api-ms-win-core-libraryloader-l1-1-1.dll
	//api-ms-win-core-console-l1-1-0.dll
	//...
	char szCore[18];
	const char szCorePrefix[] = "api-ms-win-core-"; // MUST BE LOWER CASE!
	const int nCorePrefLen = lstrlenA(szCorePrefix);
	_ASSERTE(nCorePrefLen < static_cast<ssize_t>(countof(szCore) - 1));
	bool lbIsCoreModule = false;
	char mod_name[MAX_PATH];

	//_ASSERTEX(lstrcmpi(asModule, L"dsound.dll"));

	SAFETRY
	{
		HLOG0("SetImportsPrep.Begin",ImportCount);

		//_ASSERTE(Size == (nCount * sizeof(IMAGE_IMPORT_DESCRIPTOR))); -- may not be

		for (i = 0; i < ImportCount; i++)
		{
			if (Import[i].Name == 0)
				break;

			HLOG0("SetImportsPrep.Import",i);

			HLOG1("SetImportsPrep.CheckModuleName",i);
			//DebugString( ToTchar( (char*)Module + Import[i].Name ) );
			char* mod_name_ptr = reinterpret_cast<char*>(Module) + Import[i].Name;
			DWORD_PTR rvaINT = Import[i].OriginalFirstThunk;
			const DWORD_PTR rvaIAT = Import[i].FirstThunk; //-V101
			lstrcpynA(mod_name, mod_name_ptr, countof(mod_name));
			CharLowerBuffA(mod_name, lstrlenA(mod_name)); // MUST BE LOWER CASE!
			lstrcpynA(szCore, mod_name, nCorePrefLen+1);
			lbIsCoreModule = (strcmp(szCore, szCorePrefix) == 0);

			// Process only imports from hooked modules
			bool bHookExists = false;
			for (size_t j = 0; gpHooks[j].Name; j++)
			{
				if ((strcmp(mod_name, gpHooks[j].DllNameA) != 0)
					&& !(lbIsCoreModule && (gpHooks[j].DllName == KERNEL32)))
					continue;
				bHookExists = true;
				break;
			}
			// That imported module is not hooked (strange)
			if (!bHookExists)
			{
				HLOGEND1();
				HLOGEND();
				continue;
			}

			if (rvaINT == 0)      // No Characteristics field?
			{
				// Yes! Gotta have a non-zero FirstThunk field then.
				rvaINT = rvaIAT;

				if (rvaINT == 0)       // No FirstThunk field?  Ooops!!!
				{
					_ASSERTE(rvaINT!=0);
					HLOGEND1();
					HLOGEND();
					break;
				}
			}

			PIMAGE_IMPORT_BY_NAME pOrdinalNameO = nullptr;
			IMAGE_THUNK_DATA* thunk = static_cast<IMAGE_THUNK_DATA*>(GetPtrFromRVA(rvaIAT, nt_header, reinterpret_cast<LPCBYTE>(Module)));
			IMAGE_THUNK_DATA* thunkO = static_cast<IMAGE_THUNK_DATA*>(GetPtrFromRVA(rvaINT, nt_header, reinterpret_cast<LPCBYTE>(Module)));

			if (!thunk ||  !thunkO)
			{
				_ASSERTE(thunk && thunkO);
				HLOGEND1();
				HLOGEND();
				continue;
			}
			HLOGEND1();

			// ***** >>>>>> go

			HLOG1_("SetImportsPrep.ImportThunksSteps",i);
			// ReSharper disable twice CppJoinDeclarationAndAssignment
			size_t f, s;
			for (s = 0; s <= 1; s++)
			{
				if (s)
				{
					thunk = static_cast<IMAGE_THUNK_DATA*>(GetPtrFromRVA(rvaIAT, nt_header, reinterpret_cast<LPCBYTE>(Module)));
					thunkO = static_cast<IMAGE_THUNK_DATA*>(GetPtrFromRVA(rvaINT, nt_header, reinterpret_cast<LPCBYTE>(Module)));
				}

				for (f = 0;; thunk++, thunkO++, f++)
				{
					//111127 - ..\GIT\lib\perl5\site_perl\5.8.8\msys\auto\SVN\_Core\_Core.dll
					//         some dlls have bad import tables
					#ifndef USE_SEH
					HLOG("SetImportsPrep.lbBadThunk",f);
					bool lbBadThunk = isBadModulePtr(thunk, sizeof(*thunk), Module, nt_header);
					if (lbBadThunk)
					{
						_ASSERTEX(!lbBadThunk);
						break;
					}
					#endif

					if (!thunk->u1.Function)
						break;

					#ifndef USE_SEH
					HLOG("SetImportsPrep.lbBadThunkO",f);
					bool lbBadThunkO = isBadModulePtr(thunkO, sizeof(*thunkO), Module, nt_header);
					if (lbBadThunkO)
					{
						_ASSERTEX(!lbBadThunkO);
						break;
					}
					#endif

					const char* pszFuncName = nullptr;


					// We get function address and (if s==1) function name
					// Now we need to find trampoline address
					HookItem* ph = gpHooks;
					INT_PTR jj = -1;

					if (!s)
					{
						HLOG1("SetImportsPrep.ImportThunks0",s);

						for (size_t j = 0; ph->Name; ++j, ++ph)
						{
							_ASSERTEX(j<gnHookedFuncs && gnHookedFuncs<=MAX_HOOKED_PROCS);

							// If we failed to determine original address of the procedure (kernel32/WriteConsoleOutputW, etc.)
							if (ph->HookedAddress == nullptr)
							{
								continue;
							}

							// if the import address in the module already matches the address of one of our functions
							if (ph->NewAddress == reinterpret_cast<void*>(thunk->u1.Function))
							{
								res = true; // hook was already set
								break;
							}

							// Check the address of hooked function
							if (reinterpret_cast<void*>(thunk->u1.Function) == ph->HookedAddress)
							{
								jj = j;
								break; // OK, Hook it!
							}
						} // for (size_t j = 0; ph->Name; ++j, ++ph), (s==0)

						HLOGEND1();
					}
					else
					{
						HLOG1("SetImportsPrep.ImportThunks1",s);

						// If changes are restricted if addresses do not match
						if (!abForceHooks)
						{
							//_ASSERTEX(abForceHooks);
							HLOG2("!!!Function skipped of (!abForceHooks)",f);
							continue;
						}

						// Find function name
						if ((thunk->u1.Function != thunkO->u1.Function)
							&& !IMAGE_SNAP_BY_ORDINAL(thunkO->u1.Ordinal))
						{
							pOrdinalNameO = static_cast<PIMAGE_IMPORT_BY_NAME>(GetPtrFromRVA(thunkO->u1.AddressOfData, nt_header, reinterpret_cast<LPCBYTE>(Module)));

							#ifdef USE_SEH
								pszFuncName = reinterpret_cast<const char*>(pOrdinalNameO->Name);
							#else
								HLOG("SetImportsPrep.pOrdinalNameO",f);
								// WARNING: Numerous IsBad???Ptr calls may introduce lags and bugs
								bool lbValidPtr = !isBadModulePtr(pOrdinalNameO, sizeof(IMAGE_IMPORT_BY_NAME), Module, nt_header);
								_ASSERTE(lbValidPtr);

								if (lbValidPtr)
								{
									lbValidPtr = !isBadModuleStringA((LPCSTR)pOrdinalNameO->Name, 10, Module, nt_header);
									_ASSERTE(lbValidPtr);

									if (lbValidPtr)
										pszFuncName = (LPCSTR)pOrdinalNameO->Name;
								}
							#endif
						}

						if (!pszFuncName || !*pszFuncName)
						{
							continue; // This import does not have "Function name"
						}

						HLOG2("SetImportsPrep.FindFunction",f);
						ph = FindFunction(pszFuncName);
						HLOGEND2();
						if (!ph)
						{
							HLOGEND1();
							continue;
						}

						// Module name
						HLOG2_("SetImportsPrep.Module",f);
						if ((strcmp(mod_name, ph->DllNameA) != 0)
							&& !(lbIsCoreModule && (ph->DllName == KERNEL32)))
						{
							HLOGEND2();
							HLOGEND1();
							// Names are different and no duplicates are allowed
							continue;
						}
						HLOGEND2();

						jj = (ph - gpHooks);

						HLOGEND1();
					}


					if (jj >= 0)
					{
						HLOG1("SetImportsPrep.WorkExport",f);

						// When we get here - jj matches pszFuncName or FuncPtr
						if (p->Addresses[jj].ppAdr != nullptr)
						{
							HLOGEND1();
							continue; // уже обработали, следующий импорт
						}

						//#ifdef _DEBUG
						//// Это НЕ ORDINAL, это Hint!!!
						//if (ph->nOrdinal == 0 && ordinalO != (ULONGLONG)-1)
						//	ph->nOrdinal = (DWORD)ordinalO;
						//#endif


						_ASSERTE(sizeof(thunk->u1.Function)==sizeof(DWORD_PTR));

						if (thunk->u1.Function == reinterpret_cast<DWORD_PTR>(ph->NewAddress))
						{
							// Hooks was set in another thread? should not be, blocked by critical section.
							// Could be processed during the previous attempt, if not all modules were loaded on startup
							_ASSERTE(thunk->u1.Function != reinterpret_cast<DWORD_PTR>(ph->NewAddress));
						}
						else
						{
							bFnNeedHook[jj] = true;
							p->Addresses[jj].ppAdr = &thunk->u1.Function;
							#ifdef _DEBUG
							p->Addresses[jj].ppAdrCopy1 = reinterpret_cast<DWORD_PTR>(p->Addresses[jj].ppAdr);
							p->Addresses[jj].ppAdrCopy2 = static_cast<DWORD_PTR>(*p->Addresses[jj].ppAdr);
							p->Addresses[jj].pModulePtr = reinterpret_cast<DWORD_PTR>(p->hModule);
							IMAGE_NT_HEADERS* moduleNtHeader = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<char*>(p->hModule) + reinterpret_cast<IMAGE_DOS_HEADER*>(p->hModule)->e_lfanew);
							p->Addresses[jj].nModuleSize = moduleNtHeader->OptionalHeader.SizeOfImage;
							#endif
							// During UnsetHook("cscapi.dll") and error appeared ERROR_INVALID_PARAMETER in VirtualProtect
							_ASSERTEX(p->hModule == Module);
							HLOG2("SetImportsPrep.CheckCallbackPtr.1",f);
							_ASSERTEX(CheckCallbackPtr(p->hModule, 1, reinterpret_cast<FARPROC*>(&p->Addresses[jj].ppAdr), TRUE));
							HLOGEND2();
							p->Addresses[jj].pOld = thunk->u1.Function;
							p->Addresses[jj].pOur = reinterpret_cast<DWORD_PTR>(ph->CallAddress);
							#ifdef _DEBUG
							lstrcpynA(p->Addresses[jj].sName, ph->Name, countof(p->Addresses[jj].sName));
							#endif

							_ASSERTEX(p->nAdrUsed < countof(p->Addresses));
							p->nAdrUsed++; //информационно
						}

						//DebugString( ToTchar( ph->Name ) );
						res = true;
						HLOGEND1();
					} // if (jj >= 0)
					HLOGEND1();

				} // for (f = 0;; thunk++, thunkO++, f++)

			} // for (s = 0; s <= 1; s++)
			HLOGEND1();

			HLOGEND();
		} // for (i = 0; i < nCount; i++)
		HLOGEND();
	} SAFECATCH {
	}

	return res;
}

bool SetImportsChange(LPCWSTR asModule, HMODULE Module, BOOL abForceHooks, bool (&bFnNeedHook)[MAX_HOOKED_PROCS], HkModuleInfo* p)
{
	bool bHooked = false;
	size_t j = 0;
	DWORD dwErr = static_cast<DWORD>(-1);
	_ASSERTEX(j<gnHookedFuncs && gnHookedFuncs<=MAX_HOOKED_PROCS);

	SAFETRY
	{
		while (j < gnHookedFuncs)
		{
			// Может быть nullptr, если импортируются не все функции
			if (p->Addresses[j].ppAdr && bFnNeedHook[j])
			{
				if (*p->Addresses[j].ppAdr == p->Addresses[j].pOur)
				{
					// оказалось захучено в другой нити или раньше
					_ASSERTEX(*p->Addresses[j].ppAdr != p->Addresses[j].pOur);
				}
				else
				{
					DWORD old_protect = 0xCDCDCDCD;
					if (!VirtualProtect(p->Addresses[j].ppAdr, sizeof(*p->Addresses[j].ppAdr),
					                   PAGE_READWRITE, &old_protect))
					{
						dwErr = GetLastError();
						_ASSERTEX(FALSE);
					}
					else
					{
						bHooked = true;

						*p->Addresses[j].ppAdr = p->Addresses[j].pOur;
						p->Addresses[j].bHooked = TRUE;

						VirtualProtect(p->Addresses[j].ppAdr, sizeof(*p->Addresses[j].ppAdr), old_protect, &old_protect);
					}

				}
			}

			j++;
		}
	} SAFECATCH {
		// Ошибка назначения
		p->Addresses[j].pOur = 0;
	}

	std::ignore = dwErr;
	return bHooked;
}

// Return original imports to our modules, changed by SetImports
bool UnsetImports(HkModuleInfo* p)
{
	if (!gpHooks)
		return false;

	bool bUnhooked = false;
	DWORD dwErr = static_cast<DWORD>(-1);

	if (p && (p->Hooked == HkModuleState::ImportsChanged))
	{
		HLOG1("UnsetHook.Var",0);
		// Change state immediately
		p->Hooked = HkModuleState::ImportsReverted;
		for (auto& address : p->Addresses)
		{
			if (address.pOur == 0)
				continue; // Failed to change that address

			#ifdef _DEBUG
			// During UnsetHook("cscapi.dll") and error appeared ERROR_INVALID_PARAMETER in VirtualProtect
			CheckCallbackPtr(p->hModule, 1, reinterpret_cast<FARPROC*>(&address.ppAdr), TRUE);
			#endif

			DWORD old_protect = 0xCDCDCDCD;
			if (!VirtualProtect(
				address.ppAdr, sizeof(*address.ppAdr),
							   PAGE_READWRITE, &old_protect))
			{
				dwErr = GetLastError();
				//Один раз выскочило ERROR_INVALID_PARAMETER
				// При этом, (p->Addresses[i].ppAdr==0x04cde0e0), (p->Addresses[i].ppAdr==0x912edebf)
				// что было полной пургой. Ни одного модуля в этих адресах не было
				_ASSERTEX(dwErr==ERROR_INVALID_ADDRESS);
			}
			else
			{
				bUnhooked = true;
				// BugBug: ExeOldAddress может отличаться от оригинального, если функция была перехвачена без нас
				//if (abExecutable && gpHooks[j].ExeOldAddress)
				//	thunk->u1.Function = (DWORD_PTR)gpHooks[j].ExeOldAddress;
				//else
				*address.ppAdr = address.pOld;
				address.bHooked = FALSE;
				VirtualProtect(address.ppAdr, sizeof(*address.ppAdr), old_protect, &old_protect);
			}
		}
		// Hooke were unset
		p->Hooked = HkModuleState::ImportsReverted;
		HLOGEND1();
	}


	#ifdef _DEBUG
	if (bUnhooked && p)
	{
		wchar_t* szDbg = static_cast<wchar_t*>(calloc(MAX_PATH*3, 2));
		lstrcpy(szDbg, L"  ## Hooks was UnSet by conemu: ");
		lstrcat(szDbg, p->sModuleName);
		lstrcat(szDbg, L"\n");
		DebugString(szDbg);
		free(szDbg);
	}
	#endif

	std::ignore = dwErr;
	return bUnhooked;
}

void UnsetImports()
{
	if (!gpHooks || !gpHookedModules)
		return;

	HkModuleInfo** pp = nullptr;
	const INT_PTR iCount = gpHookedModules->GetKeysValues(nullptr, &pp);
	if (iCount > 0)
	{
		for (INT_PTR i = 0; i < iCount; i++)
		{
			UnsetImports(pp[i]);
		}
		gpHookedModules->ReleasePointer(pp);
	}
}


/// Set hooks using trampolines
#if defined(_M_ARM64) || defined(__aarch64__)
static bool UpdateArm64Hooks(bool attach)
{
	std::vector<HANDLE> threads;
	const DWORD processId = GetCurrentProcessId();
	const DWORD threadId = GetCurrentThreadId();
	const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
		return false;

	THREADENTRY32 entry = {};
	entry.dwSize = sizeof(entry);
	bool complete = Thread32First(snapshot, &entry) != FALSE;
	if (complete)
	{
		do
		{
			if (entry.th32OwnerProcessID != processId || entry.th32ThreadID == threadId)
				continue;
			const HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
				THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, entry.th32ThreadID);
			if (thread)
				threads.push_back(thread);
			else if (GetLastError() != ERROR_INVALID_PARAMETER)
			{
				complete = false;
				break;
			}
		} while (Thread32Next(snapshot, &entry));
		complete = complete && GetLastError() == ERROR_NO_MORE_FILES;
	}
	CloseHandle(snapshot);

	bool success = false;
	if (complete && DetourTransactionBegin() == NO_ERROR)
	{
		LONG status = NO_ERROR;
		for (int i = 0; status == NO_ERROR && gpHooks[i].NewAddress; ++i)
		{
			auto& hook = gpHooks[i];
			if (attach && hook.HookedAddress && !hook.CallAddress)
			{
				hook.CallAddress = hook.HookedAddress;
				status = DetourAttach(&hook.CallAddress, const_cast<void*>(hook.NewAddress));
			}
			else if (!attach && hook.CallAddress)
				status = DetourDetach(&hook.CallAddress, const_cast<void*>(hook.NewAddress));
		}

		for (const HANDLE thread : threads)
		{
			if (status != NO_ERROR)
				break;
			status = DetourUpdateThread(thread);
		}
		if (status == NO_ERROR)
			success = DetourTransactionCommit() == NO_ERROR;
		else
			DetourTransactionAbort();

		for (int i = 0; gpHooks[i].NewAddress; ++i)
		{
			auto& hook = gpHooks[i];
			if (hook.CallAddress == hook.HookedAddress)
				hook.CallAddress = nullptr;
		}
	}
	for (const HANDLE thread : threads)
		CloseHandle(thread);
	return success;
}
#endif

bool SetAllHooks()
{
	if (!gpHooks)
	{
		// Functions were not defined?
		_ASSERTE(gpHooks!=nullptr);

		HLOG1("SetAllHooks.InitHooks",0);
		InitHooks(nullptr);
		if (!gpHooks)
			return false;
		HLOGEND1();
	}

	#ifdef _DEBUG
	wchar_t szHookProc[128];
	for (int i = 0; gpHooks[i].NewAddress; i++)
	{
		if (!gpHooks[i].HookedAddress || gpHooks[i].CallAddress)
			continue; // Already processed
		msprintf(szHookProc, countof(szHookProc),
			L"## %S :: " WIN3264TEST(L"x%08X", L"x%X%08X") L" ==>> " WIN3264TEST(L"x%08X", L"x%X%08X") L"\n",
			gpHooks[i].Name,
			WIN3264WSPRINT(gpHooks[i].HookedAddress),
			WIN3264WSPRINT(gpHooks[i].NewAddress)
			);
		DebugString(szHookProc);
	}
	#endif

	#if 0
	// Для исполняемого файла могут быть заданы дополнительные inject-ы (сравнение в FAR)
	HMODULE hExecutable = GetModuleHandle(0);
	#endif

	HLOG0("SetAllHooks.GetMainThreadId",0);
	// Найти ID основной нити
	GetMainThreadId(false);
	_ASSERTE(gnHookMainThreadId!=0);
	HLOGEND();

#if defined(_M_ARM64) || defined(__aarch64__)
	gnDllState |= ds_HooksStarted;
	if (!UpdateArm64Hooks(true))
		return false;
#else
	MH_STATUS status;
	HLOG("SetAllHooks.MH_CreateHook", LODWORD(gnHookedFuncs));
	for (int i = 0; gpHooks[i].NewAddress; i++)
	{
		if (gpHooks[i].HookedAddress && !gpHooks[i].CallAddress)
		{
			g_mhCreate = status = MH_CreateHook(static_cast<LPVOID>(gpHooks[i].HookedAddress), const_cast<LPVOID>(gpHooks[i].NewAddress), &gpHooks[i].CallAddress);
			_ASSERTE(status == MH_OK);
		}
	}
	HLOGEND();

	// Let GetOriginalAddress return trampolines
	gnDllState |= ds_HooksStarted;

	HLOG("SetAllHooks.MH_EnableHook", 0);
	// ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
	g_mhEnable = status = MH_EnableHook(MH_ALL_HOOKS);
	HLOGEND();
	_ASSERTE(status == MH_OK);
#endif

	DebugString(L"SetAllHooks finished\n");


	SetImports(WIN3264TEST(L"ConEmuHk",L"ConEmuHk64"), ghOurModule, TRUE);

	DebugString(L"SetImports finished\n");


	//extern FARPROC CallWriteConsoleW;
	CallWriteConsoleW = reinterpret_cast<FARPROC>(GetOriginalAddress(
		reinterpret_cast<LPVOID>(OnWriteConsoleW), HOOK_FN_ID(WriteConsoleW), nullptr, nullptr, gbPrepareDefaultTerminal));

	// extern GetConsoleWindow_T gfGetRealConsoleWindow; // from ConEmuCheck.cpp
	gfGetRealConsoleWindow = reinterpret_cast<GetConsoleWindow_T>(GetOriginalAddress(
		reinterpret_cast<LPVOID>(OnGetConsoleWindow), HOOK_FN_ID(GetConsoleWindow), nullptr, nullptr, gbPrepareDefaultTerminal));

	DebugString(L"Functions prepared\n");

	return true;
}

void UnsetAllHooks()
{
	#ifdef _DEBUG
	const MModule hExecutable(GetModuleHandle(nullptr));
	#endif

	if (gnLdrDllNotificationUsed)
	{
		HLOG1("UnregisterLdrNotification", 0);
		UnregisterLdrNotification();
		HLOGEND1();
	}

	// Set all "original" function calls to nullptr
	{
	// extern FARPROC CallWriteConsoleW;
	CallWriteConsoleW = nullptr;
	// extern GetConsoleWindow_T gfGetRealConsoleWindow; // from ConEmuCheck.cpp
	gfGetRealConsoleWindow = nullptr;

	HLOG1("hkFunc.OnHooksUnloaded", 0);
	OnHooksUnloaded();
	HLOGEND1();
	}

#if defined(_M_ARM64) || defined(__aarch64__)
	if (gpHooks)
		UpdateArm64Hooks(false);
#else
	// Some functions must be unhooked first
	MH_STATUS status;
	if (gpHooks)
	{
		HLOG1("MH_DisableHook(Specials)", 0);
		DWORD nSpecialID[] = {
			HOOK_FN_ID(CloseHandle),
			0};
		for (unsigned long long nFuncID : nSpecialID)
		{
			if (nFuncID && (nFuncID <= gnHookedFuncs))
			{
				nFuncID--;
				g_mhCritical = status = MH_DisableHook(gpHooks[nFuncID].HookedAddress);
				if (status == MH_OK)
				{
					gpHooks[nFuncID].CallAddress = nullptr;
				}
			}
		}
		HLOGEND1();
	}

	// Don't release minhook buffers in DefTerm mode,
	// keep these instructions just in case...
	if (gbPrepareDefaultTerminal)
	{
		HLOG1("MH_DisableHook(MH_ALL_HOOKS)", 0);
		// ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
		g_mhDisableAll = MH_DisableHook(MH_ALL_HOOKS);
		HLOGEND1();
	}
	else
	{
		HLOG1("MH_Uninitialize", 0);
		g_mhDeinit = MH_Uninitialize();
		HLOGEND1();
	}
#endif

	#ifdef _DEBUG
	DebugStringA("UnsetAllHooks finished\n");
	#endif
}



void LoadModuleFailed(LPCSTR asModuleA, LPCWSTR asModuleW)
{
	const DWORD dwErrCode = GetLastError();

	if (!gnLastLogSetChange)
	{
		CShellProc* sp = new CShellProc();
		if (sp)
		{
			gnLastLogSetChange = GetTickCount();
			gbLogLibraries = sp->LoadSrvMapping() && sp->GetLogLibraries();
			delete sp;
		}
		SetLastError(dwErrCode);
	}

	if (!gbLogLibraries)
		return;


	CESERVER_REQ* pIn = nullptr;
	wchar_t szModule[MAX_PATH+1]; szModule[0] = 0;
	wchar_t szErrCode[64]; szErrCode[0] = 0;
	msprintf(szErrCode, countof(szErrCode), L"ErrCode=0x%08X", dwErrCode);
	if (!asModuleA && !asModuleW)
	{
		wcscpy_c(szModule, L"<nullptr>");
		asModuleW = szModule;
	}
	else if (asModuleA)
	{
		MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0, asModuleA, -1, szModule, countof(szModule));
		szModule[countof(szModule)-1] = 0;
		asModuleW = szModule;
	}
	pIn = ExecuteNewCmdOnCreate(nullptr, ghConWnd, eLoadLibrary, L"Fail", asModuleW, szErrCode, L"", nullptr, nullptr, nullptr, nullptr,
		#ifdef _WIN64
		64
		#else
		32
		#endif
		, 0, nullptr, nullptr, nullptr);
	if (pIn)
	{
		const MWnd hConWnd(GetRealConsoleWindow());
		CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
		ExecuteFreeResult(pIn);
		if (pOut) ExecuteFreeResult(pOut);
	}
	SetLastError(dwErrCode);
}


void ProcessOnLibraryLoadedW(HMODULE module)
{
	// Far Manager ConEmu plugin may do some additional operations:
	// such as initialization of background plugins...
	if (gfOnLibraryLoaded && module)
	{
		ScopedObject(CLastErrorGuard);
		HLOG1_("PrepareNewModule.gfOnLibraryLoaded", LODWORD(module));
		gfOnLibraryLoaded(module);
		HLOGEND1();
	}
}


bool PrepareNewModule(HMODULE module, LPCSTR asModuleA, LPCWSTR asModuleW, BOOL abNoSnapshot /*= FALSE*/, BOOL abForceHooks /*= FALSE*/)
{
	bool lbAllSysLoaded = true;
	for (auto& sysDll : ghSysDll)
	{
		if (sysDll && (*sysDll == nullptr))
		{
			lbAllSysLoaded = false;
			break;
		}
	}

	int iFunc = 0;
	if (!lbAllSysLoaded)
	{
		HLOG1("PrepareNewModule.InitHooks",0);
		// Некоторые перехватываемые библиотеки могли быть
		// не загружены во время первичной инициализации
		// Соответственно для них (если они появились) нужно
		// получить "оригинальные" адреса процедур
		iFunc = InitHooks(nullptr);
		HLOGEND1();
	}


	if (!module)
	{
		LoadModuleFailed(asModuleA, asModuleW);
		return false;
	}

	// Ensure we have unicode module name
	LPCWSTR pszModuleW = asModuleW;
	wchar_t szModule[MAX_PATH+1]; szModule[0] = 0;
	if (!asModuleA && !asModuleW)
	{
		wcscpy_c(szModule, L"<nullptr>");
		pszModuleW = szModule;
	}
	else if (asModuleA)
	{
		MultiByteToWideChar(AreFileApisANSI() ? CP_ACP : CP_OEMCP, 0, asModuleA, -1, szModule, countof(szModule));
		szModule[countof(szModule)-1] = 0;
		pszModuleW = szModule;
	}

	// Проверить по gpHookedModules а не был ли модуль уже обработан?
	HLOG1("PrepareNewModule.IsHookedModule",LODWORD(module));
	HkModuleInfo* p = IsHookedModule(module);
	HLOGEND1();
	if (p)
	{
		// Этот модуль уже обработан!
		return false;
	}

	bool lbModuleOk = false;

	#ifdef _DEBUG
	const BOOL lbResource = LDR_IS_RESOURCE(module);
	std::ignore = lbResource;
	#endif

	HLOG1_("PrepareNewModule.CShellProc",0);
	LogModuleLoaded(pszModuleW, module);
	HLOGEND1();

	// Remember, it is processed already
	HLOG1_("AddHookedModule",LODWORD(module));
	p = AddHookedModule(module, pszModuleW);
	HLOGEND1();
	if (!p)
	{
		// Exit on critical failures
		return false;
	}

	// Refresh hooked exports in the loaded library
	if (iFunc > 0)
	{
		HLOG1_("PrepareNewModule.SetAllHooks",iFunc);
		SetAllHooks();
		HLOGEND1();
	}

	// Far Manager ConEmu plugin may do some additional operations:
	// such as initialization of background plugins...
	ProcessOnLibraryLoadedW(module);

	lbModuleOk = true;
	UNREFERENCED_PARAMETER(p);
	return lbModuleOk;
}


/* ************** */
void UnprepareModule(HMODULE hModule, LPCWSTR pszModule, int iStep)
{
	const BOOL lbResource = LDR_IS_RESOURCE(hModule);
	// lbResource is TRUE e.g. during calls from version.dll

	if ((iStep == 0) && gbLogLibraries && !(gnDllState & ds_DllStopping))
	{
		LogModuleUnloaded(pszModule, hModule);
	}


	// Than only real dlls (!LDR_IS_RESOURCE)
	if ((iStep > 0) && !lbResource && !(gnDllState & ds_DllStopping))
	{
		// Let's try to detect, if the module was indeed unloaded, or just the counter was decreased
		// iStep == 2 comes from LdrDllNotification(Unload)
		// Looks like if the lib was unloaded, the FreeLibrary sets SetLastError(ERROR_GEN_FAILURE)
		// Makes sense only for Win2k/XP, so don't rely on that
		const BOOL lbModulePost = (iStep == 2) ? FALSE : IsModuleValid(hModule); // GetModuleFileName(hModule, szModule, countof(szModule));
		#ifdef _DEBUG
		const DWORD dwErr = lbModulePost ? 0 : GetLastError();
		std::ignore = dwErr;
		#endif

		if (!lbModulePost)
		{
			RemoveHookedModule(hModule);

			if (ghOnLoadLibModule == hModule)
			{
				ghOnLoadLibModule = nullptr;
				gfOnLibraryLoaded = nullptr;
				gfOnLibraryUnLoaded = nullptr;
			}

			if (gpHooks)
			{
				for (int i = 0; i<MAX_HOOKED_PROCS && gpHooks[i].NewAddress; i++)
				{
					if (gpHooks[i].hCallbackModule == hModule)
					{
						gpHooks[i].hCallbackModule = nullptr;
						gpHooks[i].PreCallBack = nullptr;
						gpHooks[i].PostCallBack = nullptr;
						gpHooks[i].ExceptCallBack = nullptr;
					}
				}
			}

			TODO("Тоже на цикл переделать, как в CheckLoadedModule");

			if (gfOnLibraryUnLoaded)
			{
				gfOnLibraryUnLoaded(hModule);
			}

			// Если выгружена библиотека ghUser32/ghAdvapi32...
			// проверить, может какие наши импорты стали невалидными
			FreeLoadedModule(hModule);
		}
	}
}







/* ***************** Logging ****************** */
#ifdef LOG_ORIGINAL_CALL
void LogFunctionCall(LPCSTR asFunc, LPCSTR asFile, int anLine)
{
	if (!gbSuppressShowCall || gbSkipSuppressShowCall)
	{
		const DWORD nErr = GetLastError();
		char sFunc[128]; msprintf(sFunc, countof(sFunc), "Hook[%u:%u]: %s\n", GetCurrentProcessId(), GetCurrentThreadId(), asFunc);
		DebugStringA(sFunc);
		SetLastError(nErr);
	}
	else
	{
		gbSuppressShowCall = false;
	}
}
#endif


void LogModuleLoaded(LPCWSTR pwszModule, HMODULE hModule)
{
	// Update gnExeFlags
	// For cmd.exe
	if (gbIsCmdProcess || gbIsPowerShellProcess)
	{
		if (lstrcmpi(PointToName(pwszModule), CLINK_DLL_NAME_v1) == 0
			|| lstrcmpi(PointToName(pwszModule), CLINK_DLL_NAME_v0) == 0)
			gnExeFlags |= caf_Clink;
	}

	CShellProc* sp = nullptr;

	if (!gnLastLogSetChange || ((GetTickCount() - gnLastLogSetChange) > 2000))
	{
		sp = new CShellProc();
		if (sp)
		{
			gnLastLogSetChange = GetTickCount();
			gbLogLibraries = sp->LoadSrvMapping(TRUE) && sp->GetLogLibraries();
		}
	}
	else if (gbLogLibraries)
	{
		sp = new CShellProc();
		if (!sp)
		{
			gbLogLibraries = FALSE;
		}
	}

	if (gbLogLibraries)
	{
		CESERVER_REQ* pIn = nullptr;

		wchar_t szInfo[64] = L"";
		FormatModuleHandle(hModule, L"Module=0x%08X", L"Module=0x%08X%08X", szInfo, countof(szInfo));

		pIn = sp->NewCmdOnCreate(eLoadLibrary, nullptr, pwszModule, szInfo, nullptr, nullptr, nullptr, nullptr, nullptr, WIN3264TEST(32,64), 0, nullptr, nullptr, nullptr);
		if (pIn)
		{
			const MWnd hConWnd(GetRealConsoleWindow());
			CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
			ExecuteFreeResult(pIn);
			if (pOut) ExecuteFreeResult(pOut);
		}
	}

	SafeDelete(sp);
}

void LogModuleUnloaded(LPCWSTR pwszModule, HMODULE hModule)
{
	CShellProc* sp = new CShellProc();
	if (sp->LoadSrvMapping())
	{
		wchar_t szModule[MAX_PATH*2]; szModule[0] = 0;

		CESERVER_REQ* pIn = nullptr;
		if (pwszModule && *pwszModule)
		{
			lstrcpyn(szModule, pwszModule, countof(szModule));
		}
		else
		{
			wchar_t szHandle[64] = L"";
			FormatModuleHandle(hModule, L", <HMODULE=0x%08X>",L", <HMODULE=0x%08X%08X>", szHandle, countof(szHandle));

			// GetModuleFileName may hang in some cases, so we use our internal modules array
			if (FindModuleFileName(hModule, szModule, countof(szModule)-lstrlen(szHandle)-1))
				wcscat_c(szModule, szHandle);
			else
				wcscpy_c(szModule, szHandle+2);
		}

		pIn = sp->NewCmdOnCreate(eFreeLibrary, nullptr, szModule, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
			#ifdef _WIN64
			64
			#else
			32
			#endif
			, 0, nullptr, nullptr, nullptr);
		if (pIn)
		{
			const MWnd hConWnd(GetRealConsoleWindow());
			CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
			ExecuteFreeResult(pIn);
			if (pOut) ExecuteFreeResult(pOut);
		}
	}
	delete sp;
}

#ifdef _DEBUG
LONG COriginalCallCount::mn_LastTID = 0;
LONG COriginalCallCount::mn_LastFnID = 0;

COriginalCallCount::COriginalCallCount(LONG* pThreadId, LONG* pCount, LONG nFnID, LPCSTR asFnName)
	: mp_ThreadId(pThreadId)
	, mp_Count(pCount)
{
	const LONG nTID = GetCurrentThreadId();
	const LONG lOldTid = InterlockedExchange(mp_ThreadId, nTID);
	LONG nNewCount = 1;
	if (lOldTid != nTID)
		InterlockedExchange(mp_Count, nNewCount);
	else
		nNewCount = InterlockedIncrement(mp_Count);

	InterlockedExchange(&mn_LastTID, nTID);
	InterlockedExchange(&mn_LastFnID, nFnID);

	if (nNewCount == 2)
	{
		// Don't use ASSERT or any GUI function here to avoid infinite recursion!
		char szMsg[120];
		msprintf(szMsg, countof(szMsg), "!!! Hook !!! %s Count=%u\n", asFnName, nNewCount);
		OutputDebugStringA(szMsg);
		std::ignore = 0; // place for breakpoint
	}
};

COriginalCallCount::~COriginalCallCount()
{
	LONG nLeft;
	const LONG nTID = GetCurrentThreadId();
	if (*mp_ThreadId == nTID)
	{
		nLeft = InterlockedDecrement(mp_Count);
		if (nLeft <= 0)
			InterlockedExchange(mp_ThreadId, 0);
	}
}
#endif // #ifdef _DEBUG
