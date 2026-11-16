
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

#ifdef _DEBUG
//#define SHOW_INJECTS_MSGBOX
#else
#undef SHOW_INJECTS_MSGBOX
#endif

#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/WModuleCheck.h"
#include "Injects.h"
#include "InjectsBootstrap.h"
#include "hlpProcess.h"
#include "../common/MModule.h"
#include "../common/WObjects.h"
#include <tuple>

#ifdef SHOW_INJECTS_MSGBOX
#include "../common/MToolHelp.h"
#include "../common/ProcessData.h"
#endif

extern HMODULE ghOurModule;

InjectsFnPtr gfLoadLibrary;
InjectsFnPtr gfLdrGetDllHandleByName;

HANDLE ghSkipSetThreadContextForThread = nullptr;

HANDLE ghInjectsInMainThread = nullptr;

// Проверить, что gfLoadLibrary лежит в пределах модуля hKernel!
UINT_PTR GetLoadLibraryAddress()
{
	if (gfLoadLibrary.fnPtr)
		return gfLoadLibrary.fnPtr;

	UINT_PTR fnLoadLibrary = 0;
	HMODULE hKernel32 = ::GetModuleHandle(L"kernel32.dll");
	HMODULE hKernelBase = IsWin8() ? ::GetModuleHandle(L"KernelBase.dll") : nullptr;
	if (!hKernel32 || LDR_IS_RESOURCE(hKernel32))
	{
		_ASSERTE(hKernel32 && !LDR_IS_RESOURCE(hKernel32));
		return 0;
	}

	HMODULE hConEmuHk = ::GetModuleHandle(ConEmuHk_DLL_3264);
	if (hConEmuHk && (hConEmuHk != ghOurModule))
	{
		typedef FARPROC (WINAPI* GetLoadLibraryW_t)();
		GetLoadLibraryW_t GetLoadLibraryW = (GetLoadLibraryW_t)::GetProcAddress(hConEmuHk, "GetLoadLibraryW");
		if (GetLoadLibraryW)
		{
			fnLoadLibrary = (UINT_PTR)GetLoadLibraryW();
		}
	}

	UINT_PTR fnKernel32 = (UINT_PTR)::GetProcAddress(hKernel32, "LoadLibraryW");
	UINT_PTR fnKernelBase = hKernelBase ? (UINT_PTR)::GetProcAddress(hKernelBase, "LoadLibraryW") : 0L;
	HMODULE hKernel = fnKernelBase ? hKernelBase : hKernel32;
	if (!fnLoadLibrary)
	{
		fnLoadLibrary = fnKernelBase ? fnKernelBase : fnKernel32;
	}

	// Функция должна быть именно в Kernel32.dll (а не в какой либо другой библиотеке, мало ли кто захукал...)
	if (!CheckCallbackPtr(hKernel, 1, (FARPROC*)&fnLoadLibrary, TRUE))
	{
		// _ASSERTE уже был
		return 0;
	}

	gfLoadLibrary = InjectsFnPtr(hKernel, fnLoadLibrary, (hKernel==hKernel32) ? L"Kernel32.dll" : L"KernelBase.dll");
	return fnLoadLibrary;
}

UINT_PTR GetLdrGetDllHandleByNameAddress()
{
	if (gfLdrGetDllHandleByName.fnPtr)
		return gfLdrGetDllHandleByName.fnPtr;

	UINT_PTR fnLdrGetDllHandleByName = 0;
	HMODULE hNtDll = ::GetModuleHandle(L"ntdll.dll");
	if (!hNtDll || LDR_IS_RESOURCE(hNtDll))
	{
		_ASSERTE(hNtDll&& !LDR_IS_RESOURCE(hNtDll));
		return 0;
	}

	fnLdrGetDllHandleByName = (UINT_PTR)::GetProcAddress(hNtDll, "LdrGetDllHandleByName");

	// Функция должна быть именно в ntdll.dll (а не в какой либо другой библиотеке, мало ли кто захукал...)
	if (!CheckCallbackPtr(hNtDll, 1, (FARPROC*)&fnLdrGetDllHandleByName, TRUE))
	{
		// _ASSERTE уже был
		return 0;
	}

	gfLdrGetDllHandleByName = InjectsFnPtr(hNtDll, fnLdrGetDllHandleByName, L"ntdll.dll");
	return fnLdrGetDllHandleByName;
}

static void ShowInjectsMsgBox(PROCESS_INFORMATION pi, const DWORD imageBits)
{
#ifdef SHOW_INJECTS_MSGBOX
	wchar_t szTitle[64] = L"", pidStr[16] = L"", pidBits[16] = L"";
	swprintf_c(szTitle, L"ConEmu [%u] Injects (PID=%i)", WIN3264TEST(32, 64), GetCurrentProcessId());

	wchar_t targetProcessName[MAX_PATH] = L"";
	if (!targetProcessName[0])
	{
		MToolHelpModule modules(pi.dwProcessId);
		MODULEENTRY32W exeInfo{};
		if (modules.Next(exeInfo) && exeInfo.szExePath[0])
			std::ignore = lstrcpyn(targetProcessName, exeInfo.szExePath, countof(targetProcessName));
	}
	if (!targetProcessName[0])
	{
		CProcessData process;
		wchar_t name[MAX_PATH];
		process.GetProcessName(pi.dwProcessId, name, countof(name), targetProcessName, countof(targetProcessName), nullptr);
	}

	CEStr moduleName;
	GetCurrentModulePathName(moduleName);
	CEStr exeName;
	GetModulePathName(nullptr, exeName);

	DWORD uType = MB_SYSTEMMODAL;
	const auto* targetExeName = PointToName(targetProcessName);
	if (targetExeName && (IsConEmuGui(targetExeName) || IsConsoleServer(targetExeName)))
		uType |= MB_ICONSTOP;

	const CEStr pidInfo(L"\n\n" L"Target PID: ", ltow_s(pi.dwProcessId, pidStr, 10),
		L", Bitness: ", ltow_s(imageBits, pidBits, 10));
	const CEStr msg(L"Source exe:\n", exeName, L"\n\n" L"Source module:\n", moduleName,
		pidInfo, L"\n", targetProcessName);
	MessageBox(nullptr, msg, szTitle, uType);
#endif
}

// The handle must have the PROCESS_CREATE_THREAD, PROCESS_QUERY_INFORMATION, PROCESS_VM_OPERATION, PROCESS_VM_WRITE, and PROCESS_VM_READ
// The function may start appropriate bitness of ConEmuC.exe with "/SETHOOKS=..." switch
// If bitness matches, use WriteProcessMemory and SetThreadContext immediately
#if defined(CONEMU_ARM64_HOOKS)
bool IsNativeArm64Target(HANDLE process)
{
	using QueryProcessInfo = BOOL (WINAPI*)(HANDLE, PROCESS_INFORMATION_CLASS, void*, DWORD);
	const auto query = reinterpret_cast<QueryProcessInfo>(
		GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetProcessInformation"));
	PROCESS_MACHINE_INFORMATION machine = {};
	// x64 emulation on Windows on Arm does not use the WOW64 layer.
	if (query && query(process, ProcessMachineTypeInfo, &machine, sizeof(machine))
		&& machine.ProcessMachine == IMAGE_FILE_MACHINE_ARM64)
		return true;
	SetLastError(ERROR_NOT_SUPPORTED);
	return false;
}
#endif

CINJECTHK_EXIT_CODES InjectHooks(PROCESS_INFORMATION pi, const DWORD imageBits, BOOL abLogProcess, LPCWSTR asConEmuHkDir, HWND hConWnd)
{
#if defined(CONEMU_ARM64_HOOKS)
	if (!IsNativeArm64Target(pi.hProcess))
		return CIH_GeneralError;
#endif
	CINJECTHK_EXIT_CODES iRc = CIH_OK/*0*/;
	wchar_t szDllDir[MAX_PATH*2];
	_ASSERTE(ghOurModule!=nullptr);
	BOOL is64bitOs = FALSE;
	DWORD loadedImageBits = (imageBits == 32 || imageBits == 64) ? imageBits : 32; //-V112
	DWORD imageBitsLastError = 0;
#ifdef WIN64
	is64bitOs = TRUE;
#endif
	// to check IsWow64Process
	const MModule hKernel(GetModuleHandle(L"kernel32.dll"));
	MModule hNtDll{};
	DEBUGTEST(int iFindAddress = 0);
	DWORD nErrCode = 0, nWait = 0;
	const int selfImageBits = WIN3264TEST(32,64);

	if (!hKernel)
	{
		iRc = CIH_KernelNotLoaded/*-510*/;
		goto wrap;
	}
	if (IsWin7())
	{
		hNtDll.SetHandle(GetModuleHandle(L"ntdll.dll"));
		// Windows7 +
		if (!hNtDll)
		{
			iRc = CIH_NtdllNotLoaded/*-512*/;
			goto wrap;
		}
	}

	// Процесс не был стартован, или уже завершился
	nWait = WaitForSingleObject(pi.hProcess, 0);
	if (nWait == WAIT_OBJECT_0)
	{
		iRc = CIH_ProcessWasTerminated/*-506*/;
		goto wrap;
	}

	if (asConEmuHkDir && *asConEmuHkDir)
	{
		lstrcpyn(szDllDir, asConEmuHkDir, countof(szDllDir));
	}
	else
	{
		#ifdef _DEBUG
		//_CrtDbgBreak();
		_ASSERTE(FALSE && "asConEmuHkDir is empty, can't set hooks");
		#endif
		//_printf("GetModuleFileName failed! ErrCode=0x%08X\n", dwErr);
		iRc = CIH_GetModuleFileName/*-501*/;
		goto wrap;
	}

	if (hKernel.IsValid())
	{
		typedef BOOL (WINAPI* IsWow64Process_t)(HANDLE, PBOOL);
		IsWow64Process_t isWow64ProcessFunc = nullptr;

		if (hKernel.GetProcAddress("IsWow64Process", isWow64ProcessFunc))
		{
			BOOL bWow64;
			#ifndef WIN64
			// current process is 32-bit, (bWow64==TRUE) means the OS is 64-bit
			bWow64 = FALSE;
			if (isWow64ProcessFunc(GetCurrentProcess(), &bWow64) && bWow64)
				is64bitOs = TRUE;
			#else
			_ASSERTE(is64bitOs);
			#endif
			
			// Check the started process now
			bWow64 = FALSE;
			if (is64bitOs)
			{
				if (isWow64ProcessFunc(pi.hProcess, &bWow64))
					loadedImageBits = bWow64 ? 32 : 64;
				else
					imageBitsLastError = GetLastError();
			}
		}
	}

	ShowInjectsMsgBox(pi, loadedImageBits);

	//int iFindAddress = 0;
	//bool lbInj = false;
	////UINT_PTR fnLoadLibrary = nullptr;
	////DWORD fLoadLibrary = 0;
	//DWORD nErrCode = 0;
	//int SelfImageBits;
	//#ifdef WIN64
	//SelfImageBits = 64;
	//#else
	//SelfImageBits = 32;
	//#endif

	//#ifdef WIN64
	//	fnLoadLibrary = (UINT_PTR)::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");

	//	// 64битный conemuc сможет найти процедуру и в 32битном процессе
	//	iFindAddress = FindKernelAddress(pi.hProcess, pi.dwProcessId, &fLoadLibrary);

	//#else
	// Если битность не совпадает - нужен helper
	if (loadedImageBits != selfImageBits)
	{
		const DWORD dwPidWait = WaitForSingleObject(pi.hProcess, 0);
		if (dwPidWait == WAIT_OBJECT_0)
		{
			_ASSERTE(dwPidWait != WAIT_OBJECT_0);
		}
		// Требуется 64битный(32битный?) comspec для установки хука
		DEBUGTEST(iFindAddress = -1);
		HANDLE hProcess = nullptr, hThread = nullptr;
		DuplicateHandle(GetCurrentProcess(), pi.hProcess, GetCurrentProcess(), &hProcess, 0, TRUE, DUPLICATE_SAME_ACCESS);
		DuplicateHandle(GetCurrentProcess(), pi.hThread, GetCurrentProcess(), &hThread, 0, TRUE, DUPLICATE_SAME_ACCESS);
		_ASSERTE(hProcess && hThread);
		#ifdef _WIN64
		// Если превышение DWORD в Handle - то запускаемый 32битный обломится. Но вызывается ли он вообще?
		if ((LODWORD(hProcess) != (DWORD_PTR)hProcess) || (LODWORD(hThread) != (DWORD_PTR)hThread))
		{
			_ASSERTE((LODWORD(hProcess) == (DWORD_PTR)hProcess) && (LODWORD(hThread) == (DWORD_PTR)hThread));
			iRc = CIH_WrongHandleBitness/*-509*/;
			goto wrap;
		}
		#endif
		wchar_t sz64helper[MAX_PATH*2];
		msprintf(sz64helper, countof(sz64helper),
		          L"\"%s\\ConEmuC%s.exe\" /SETHOOKS=%X,%u,%X,%u",
		          szDllDir, (loadedImageBits==64) ? L"64" : L"",
		          LODWORD(hProcess), pi.dwProcessId, LODWORD(hThread), pi.dwThreadId);
		STARTUPINFO si = {};
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi64 = {};
		LPSECURITY_ATTRIBUTES lpNotInh = LocalSecurity();
		SECURITY_ATTRIBUTES secInh = {};
		secInh.nLength = sizeof(secInh);
		secInh.lpSecurityDescriptor = lpNotInh->lpSecurityDescriptor;
		secInh.bInheritHandle = TRUE;

		// Добавил DETACHED_PROCESS, чтобы helper не появлялся в списке процессов консоли,
		// а то у сервера может крышу сорвать, когда helper исчезнет, а приложение еще не появится.
		BOOL lbHelper = CreateProcess(nullptr, sz64helper, &secInh, &secInh,
			TRUE, HIGH_PRIORITY_CLASS|DETACHED_PROCESS, nullptr, nullptr, &si, &pi64);

		if (!lbHelper)
		{
			nErrCode = GetLastError();
			// Ошибки показывает вызывающая функция/процесс
			iRc = CIH_CreateProcess/*-502*/;

			CloseHandle(hProcess); CloseHandle(hThread);
			goto wrap;
		}
		else
		{
			WaitForSingleObject(pi64.hProcess, INFINITE);

			if (!GetExitCodeProcess(pi64.hProcess, &nErrCode))
				nErrCode = -1;

			CloseHandle(pi64.hProcess); CloseHandle(pi64.hThread);
			CloseHandle(hProcess); CloseHandle(hThread);

			if (((int)nErrCode == CERR_HOOKS_WAS_SET) || ((int)nErrCode == CERR_HOOKS_WAS_ALREADY_SET))
			{
				iRc = CIH_OK/*0*/;
				goto wrap;
			}
			else if ((int)nErrCode == CERR_HOOKS_FAILED)
			{
				iRc = CIH_WrapperFailed/*-505*/;
				goto wrap;
			}

			// Ошибки показывает вызывающая функция/процесс
		}

		// Уже все ветки должны были быть обработаны!
		_ASSERTE(FALSE);
		iRc = CIH_WrapperGeneral/*-504*/;
		goto wrap;

	}
	else
	{
		//iFindAddress = FindKernelAddress(pi.hProcess, pi.dwProcessId, &fLoadLibrary);
		//fnLoadLibrary = (UINT_PTR)::GetProcAddress(::GetModuleHandle(L"kernel32.dll"), "LoadLibraryW");
		if (!GetLoadLibraryAddress())
		{
			_ASSERTE(gfLoadLibrary.fnPtr != 0);
			iRc = CIH_GetLoadLibraryAddress/*-503*/;
			goto wrap;
		}
		else if (IsWin7() && !GetLdrGetDllHandleByNameAddress() && !IsWine())
		{
			_ASSERTE(gfLdrGetDllHandleByName.fnPtr != 0);
			iRc = CIH_GetLdrHandleAddress/*-514*/;
			goto wrap;
		}
		else
		{
			// -- не имеет смысла. процесс еще "не отпущен", поэтому CreateToolhelp32Snapshot(TH32CS_SNAPMODULE) обламывается
			//// Проверить, а не Гуй ли это?
			//BOOL lbIsGui = FALSE;
			//if (!abForceGui)
			//{
			//	DWORD nBits = 0;
			//	if (GetImageSubsystem(pi, ImageSubsystem, nBits))
			//		lbIsGui = (ImageSubsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI);
			//	_ASSERTE(nBits == ImageBits);
			//	if (lbIsGui)
			//	{
			//		iRc = CIH_OK/*0*/;
			//		goto wrap;
			//	}
			//}

			// ??? Сначала нужно проверить, может есть проблема с адресами (ASLR) ???
			//-- ReadProcessMemory возвращает ошибку 299, и cch_dos_read==0, так что не катит...
			//IMAGE_DOS_HEADER dos_hdr = {}; SIZE_T cch_dos_read = 0;
			//BOOL bRead = ::ReadProcessMemory(pi.hProcess, (LPVOID)(DWORD_PTR)hKernel, &dos_hdr, sizeof(dos_hdr), &cch_dos_read);

			DWORD_PTR ptrAllocated = 0; DWORD nAllocated = 0;
			InjectHookFunctions fnArg = {gfLoadLibrary.module, gfLoadLibrary.fnPtr, gfLoadLibrary.szModule, gfLdrGetDllHandleByName.module, gfLdrGetDllHandleByName.fnPtr};
			iRc = InjectHookDLL(pi, &fnArg, loadedImageBits, szDllDir, &ptrAllocated, &nAllocated);

			if (abLogProcess || (iRc != CIH_OK/*0*/))
			{
				const int imageSubsystem = 0; // is not loaded
				wchar_t szInfo[128];
				if (iRc != CIH_OK/*0*/)
				{
					const DWORD nErr = GetLastError();
					msprintf(szInfo, countof(szInfo), L"InjectHookDLL failed, code=%i:0x%08X", iRc, nErr);
				}
				#ifdef _WIN64
				_ASSERTE(selfImageBits == 64);
				if (iRc == CIH_OK/*0*/)
				{
					if ((DWORD)(ptrAllocated >> 32)) //-V112
					{
						msprintf(szInfo, countof(szInfo), L"Alloc: 0x%08X%08X, Size: %u",
							(DWORD)(ptrAllocated >> 32), (DWORD)(ptrAllocated & 0xFFFFFFFF), nAllocated); //-V112
					}
					else
					{
						msprintf(szInfo, countof(szInfo), L"Alloc: 0x%08X, Size: %u",
							(DWORD)(ptrAllocated & 0xFFFFFFFF), nAllocated); //-V112
					}
				}
				#else
				_ASSERTE(selfImageBits == 32);
				if (iRc == CIH_OK/*0*/)
				{
					msprintf(szInfo, countof(szInfo), L"Alloc: 0x" WIN3264TEST(L"%08X",L"%X%08X") L", Size: %u", WIN3264WSPRINT(ptrAllocated), nAllocated);
				}
				#endif

				CESERVER_REQ* pIn = ExecuteNewCmdOnCreate(
					nullptr, hConWnd, eSrvLoaded,
					L"", szInfo, L"", L"", nullptr, nullptr, nullptr, nullptr,
					selfImageBits, imageSubsystem, nullptr, nullptr, nullptr);
				if (pIn)
				{
					CESERVER_REQ* pOut = ExecuteGuiCmd(hConWnd, pIn, hConWnd);
					ExecuteFreeResult(pIn);
					if (pOut) ExecuteFreeResult(pOut);
				}
			}
		}
	}

wrap:
	if (iRc == CIH_OK/*0*/)
	{
		wchar_t szEvtName[64];
		SafeCloseHandle(ghInjectsInMainThread);

		// ResumeThread was not called yet, set event
		msprintf(szEvtName, countof(szEvtName), CECONEMUROOTTHREAD, pi.dwProcessId);
		ghInjectsInMainThread = CreateEvent(LocalSecurity(), TRUE, TRUE, szEvtName);
		if (ghInjectsInMainThread)
		{
			SetEvent(ghInjectsInMainThread);
		}
		else
		{
			_ASSERTEX(ghInjectsInMainThread!=nullptr);
		}

		// ReSharper disable once CppDeclaratorDisambiguatedAsFunction
		extern CESERVER_CONSOLE_APP_MAPPING* GetAppMapPtr();
		CESERVER_CONSOLE_APP_MAPPING* pAppMap = GetAppMapPtr();
		if (pAppMap)
		{
			pAppMap->HookedPids.AddValue(pi.dwProcessId);
		}
	}
	std::ignore = imageBitsLastError;
	return iRc;
}
