
#pragma once

struct InjectsFnPtr
{
	HMODULE  module;
	UINT_PTR fnPtr;
	wchar_t  szModule[32]{};

	InjectsFnPtr() : module(nullptr), fnPtr(0) { szModule[0] = 0; };
	InjectsFnPtr(HMODULE m, UINT_PTR f, LPCWSTR n) : module(m), fnPtr(f) { lstrcpyn(szModule, n, countof(szModule)); };
};

extern InjectsFnPtr gfLoadLibrary;
extern InjectsFnPtr gfLdrGetDllHandleByName;

#include "../ConEmuCD/ExitCodes.h"

CINJECTHK_EXIT_CODES InjectHooks(PROCESS_INFORMATION pi, DWORD imageBits, BOOL abLogProcess, LPCWSTR asConEmuHkDir, HWND hConWnd);
UINT_PTR GetLoadLibraryAddress();
UINT_PTR GetLdrGetDllHandleByNameAddress();
#if defined(CONEMU_ARM64_HOOKS)
bool IsNativeArm64Target(HANDLE process);
#endif
