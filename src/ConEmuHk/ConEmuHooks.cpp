
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

#define HOOKS_VIRTUAL_ALLOC
#ifdef _DEBUG
	#define REPORT_VIRTUAL_ALLOC
#else
	#undef REPORT_VIRTUAL_ALLOC
#endif

#ifdef _DEBUG
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#else
	#define DefTermMsg(s) //MessageBox(NULL, s, L"ConEmuHk", MB_SYSTEMMODAL)
#endif

#ifdef _DEBUG
	//#define PRE_PEEK_CONSOLE_INPUT
	#undef PRE_PEEK_CONSOLE_INPUT
#else
	#undef PRE_PEEK_CONSOLE_INPUT
#endif

#define DROP_SETCP_ON_WIN2K3R2
//#define SHOWDEBUGSTR -- специально отключено, CONEMU_MINIMAL, OutputDebugString могут нарушать работу процессов

#undef SHOWCREATEPROCESSTICK
#ifdef _DEBUG
	#define SHOWCREATEPROCESSTICK
	#define DUMP_VIM_SETCURSORPOS
#endif

#ifdef _DEBUG
	//#define TRAP_ON_MOUSE_0x0
	#undef TRAP_ON_MOUSE_0x0
	#undef LOG_GETANCESTOR
#else
	#undef TRAP_ON_MOUSE_0x0
	#undef LOG_GETANCESTOR
#endif

// Иначе не определяется GetConsoleAliases (хотя он должен быть доступен в Win2k)
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0501

// Declare function ID variables
#define DECLARE_CONEMU_HOOK_FUNCTION_ID

#include "../common/defines.h"
#include <winnt.h>
#include <tchar.h>
#include <shlwapi.h>
#include "../common/Common.h"
#include "SetHook.h"
#include "DefTermHk.h"
#include "DllOptions.h"
#include "ShellProcessor.h"
#include "../common/CmdLine.h"
#include "../common/WObjects.h"

#include "hkCmdExe.h"
#include "hkConsole.h"
#include "hkConsoleInput.h"
#include "hkConsoleOutput.h"
#include "hkDialog.h"
#include "hkEnvironment.h"
#include "hkFarExe.h"
#include "hkGDI.h"
#include "hkKernel.h"
#include "hkLibrary.h"
#include "hkMessages.h"
#include "hkProcess.h"
#include "hkStdIO.h"
#include "hkWindow.h"

#include "hlpProcess.h"


#ifdef _DEBUG
	//#define DEBUG_CON_TITLE
	#undef DEBUG_CON_TITLE
	#ifdef DEBUG_CON_TITLE
	CEStr* gpLastSetConTitle = NULL;
	#endif
#else
	#undef DEBUG_CON_TITLE
#endif


/* Forward declarations */
BOOL IsVisibleRectLocked(COORD& crLocked);


#undef isPressed
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)



#ifdef _DEBUG
#define DebugString(x) //OutputDebugString(x)
#else
#define DebugString(x) //OutputDebugString(x)
#endif


// Service functions
//typedef DWORD (WINAPI* GetProcessId_t)(HANDLE Process);
GetProcessId_t gfGetProcessId = NULL;


void __stdcall SetFarHookMode(struct HookModeFar *apFarMode)
{
	if (!apFarMode)
	{
		gFarMode.bFarHookMode = FALSE;
	}
	else if (apFarMode->cbSize != sizeof(HookModeFar))
	{
		_ASSERTE(apFarMode->cbSize == sizeof(HookModeFar));
		/*
		gFarMode.bFarHookMode = FALSE;
		wchar_t szTitle[64], szText[255];
		msprintf(szTitle, SKIPLEN(countof(szTitle)) L"ConEmuHk, PID=%u", GetCurrentProcessId());
		msprintf(szText, SKIPLEN(countof(szText)) L"SetFarHookMode recieved invalid sizeof = %u\nRequired = %u", apFarMode->cbSize, (DWORD)sizeof(HookModeFar));
		Message BoxW(NULL, szText, szTitle, MB_ICONSTOP|MB_SYSTEMMODAL);
		*/
	}
	else
	{
		// При запуске Ctrl+Shift+W - фар как-то криво инитится... Ctrl+O не работает
		#ifdef _DEBUG
		char szInfo[100]; msprintf(szInfo, countof(szInfo), "SetFarHookMode. FarHookMode=%u, LongConsoleOutput=%u\n", apFarMode->bFarHookMode, apFarMode->bLongConsoleOutput);
		OutputDebugStringA(szInfo);
		#endif
		memmove(&gFarMode, apFarMode, sizeof(gFarMode));
	}
}



bool InitHooksLibrary()
{
	#ifdef _DEBUG
	extern HookItem *gpHooks;
	extern size_t gnHookedFuncs;
	// Must be placed on top
	_ASSERTE(gpHooks && gnHookedFuncs==0 && gpHooks[0].NewAddress==NULL);
	#endif

	HLOG1("InitHooksLibrary",0);
	bool lbRc = false;

	HookItem HooksLib1[] =
	{
		/* ************************ */
		HOOK_ITEM_BY_NAME(LoadLibraryA,			KERNEL32),
		HOOK_ITEM_BY_NAME(LoadLibraryExA,		KERNEL32),
		HOOK_ITEM_BY_NAME(LoadLibraryExW,		KERNEL32),
		HOOK_ITEM_BY_NAME(FreeLibrary,			KERNEL32),
		/* ************************ */
		{0}
	};

	HookItem HooksLib2[] =
	{
		/* ************************ */
		HOOK_ITEM_BY_NAME(LoadLibraryW,			KERNEL32),
		/* ************************ */
		{0}
	};


	// Need to hook All LoadLibrary### if the work isn't done by LdrRegisterDllNotification
	if (gnLdrDllNotificationUsed != ldr_FullSupport)
	{
		if (InitHooks(HooksLib1) < 0)
			goto wrap;
	}

	// Also LoadLibraryW, it handles "ExtendedConsole.dll" loading in Far 32/64
	if (gbIsFarProcess || (gnLdrDllNotificationUsed != ldr_FullSupport))
	{
		if (InitHooks(HooksLib2) < 0)
			goto wrap;
	}

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}


bool InitHooksKernel()
{
	HLOG1("InitHooksKernel",0);
	bool lbRc = false;

	HookItem HooksKernel[] =
	{
		/* ************************ */
		HOOK_ITEM_BY_NAME(OpenFileMappingW,		KERNEL32),
		HOOK_ITEM_BY_NAME(MapViewOfFile,		KERNEL32),
		HOOK_ITEM_BY_NAME(UnmapViewOfFile,		KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(SetEnvironmentVariableA,	KERNEL32),
		HOOK_ITEM_BY_NAME(SetEnvironmentVariableW,	KERNEL32),
		HOOK_ITEM_BY_NAME(GetEnvironmentVariableA,	KERNEL32),
		HOOK_ITEM_BY_NAME(GetEnvironmentVariableW,	KERNEL32),
		#if 0
		HOOK_ITEM_BY_NAME(GetEnvironmentStringsA,	kernel32),
		#endif
		HOOK_ITEM_BY_NAME(GetEnvironmentStringsW,	KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(GetSystemTime,			KERNEL32),
		HOOK_ITEM_BY_NAME(GetLocalTime,				KERNEL32),
		HOOK_ITEM_BY_NAME(GetSystemTimeAsFileTime,	KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(Beep,						KERNEL32),
		/* ************************ */
		{0}
	};

	if (InitHooks(HooksKernel) < 0)
		goto wrap;

	#if 0
	// Проверка, как реагирует на дубли
	HooksCommon[1].NewAddress = NULL;
	_ASSERTEX(FALSE && "Testing");
	InitHooks(HooksCommon);
	#endif

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

bool InitHooksDebugging()
{
#if !defined(_DEBUG)

	return true;

#else

	HLOG1("InitHooksDebugging",0);
	bool lbRc = false;

	HookItem HooksDbg[] =
	{
		/* ************************ */

		HOOK_ITEM_BY_NAME(CreateNamedPipeW,		KERNEL32),

		HOOK_ITEM_BY_NAME(VirtualAlloc,			KERNEL32),
		#if 0
		HOOK_ITEM_BY_NAME(VirtualProtect,		kernel32),
		#endif
		HOOK_ITEM_BY_NAME(SetUnhandledExceptionFilter, KERNEL32),

		/* ************************ */

		#ifdef HOOK_ERROR_PROC
		HOOK_ITEM_BY_NAME(GetLastError,			KERNEL32),
		HOOK_ITEM_BY_NAME(SetLastError,			KERNEL32), // eSetLastError
		#endif

		/* ************************ */
		{0}
	};

	if (InitHooks(HooksDbg) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
#endif // _DEBUG
}

// Console, ANSI, Read/Write, etc.
bool InitHooksConsole()
{
	HLOG1("InitHooksConsole",0);
	bool lbRc = false;

	HookItem HooksConsole[] =
	{
		/* ************************ */
		HOOK_ITEM_BY_NAME(CreateFileW,  		KERNEL32),
		HOOK_ITEM_BY_NAME(CreateFileA,  		KERNEL32),
		HOOK_ITEM_BY_NAME(WriteFile,  			KERNEL32),
		HOOK_ITEM_BY_NAME(ReadFile,				KERNEL32),
		HOOK_ITEM_BY_NAME(CloseHandle,			KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(SetStdHandle,			KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(GetConsoleWindow,     KERNEL32),
		HOOK_ITEM_BY_NAME(GetConsoleMode,		KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleMode,  		KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleTitleA,		KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleTitleW,		KERNEL32),
		HOOK_ITEM_BY_NAME(GetConsoleAliasesW,	KERNEL32),
		HOOK_ITEM_BY_NAME(AllocConsole,			KERNEL32),
		HOOK_ITEM_BY_NAME(FreeConsole,			KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleKeyShortcuts, KERNEL32),
		HOOK_ITEM_BY_NAME(GetConsoleProcessList,KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(SetConsoleTextAttribute, KERNEL32),
		HOOK_ITEM_BY_NAME(WriteConsoleOutputW,  KERNEL32),
		HOOK_ITEM_BY_NAME(WriteConsoleOutputA,  KERNEL32),
		HOOK_ITEM_BY_NAME(ReadConsoleW,			KERNEL32),
		HOOK_ITEM_BY_NAME(ReadConsoleA,			KERNEL32),
		HOOK_ITEM_BY_NAME(PeekConsoleInputW,	KERNEL32),
		HOOK_ITEM_BY_NAME(PeekConsoleInputA,	KERNEL32),
		HOOK_ITEM_BY_NAME(ReadConsoleInputW,	KERNEL32),
		HOOK_ITEM_BY_NAME(ReadConsoleInputA,	KERNEL32),
		HOOK_ITEM_BY_NAME(WriteConsoleInputA,	KERNEL32),
		HOOK_ITEM_BY_NAME(WriteConsoleInputW,	KERNEL32),
		HOOK_ITEM_BY_NAME(WriteConsoleA,  		KERNEL32),
		HOOK_ITEM_BY_NAME(WriteConsoleW,  		KERNEL32),
		HOOK_ITEM_BY_NAME(ScrollConsoleScreenBufferA, KERNEL32),
		HOOK_ITEM_BY_NAME(ScrollConsoleScreenBufferW, KERNEL32),
		HOOK_ITEM_BY_NAME(WriteConsoleOutputCharacterA, KERNEL32),
		HOOK_ITEM_BY_NAME(WriteConsoleOutputCharacterW, KERNEL32),
		HOOK_ITEM_BY_NAME(FillConsoleOutputCharacterA, KERNEL32),
		HOOK_ITEM_BY_NAME(FillConsoleOutputCharacterW, KERNEL32),
		HOOK_ITEM_BY_NAME(FillConsoleOutputAttribute, KERNEL32),
		/* Others console functions */
		HOOK_ITEM_BY_NAME(GetNumberOfConsoleInputEvents, KERNEL32),
		HOOK_ITEM_BY_NAME(FlushConsoleInputBuffer, KERNEL32),
		HOOK_ITEM_BY_NAME(CreateConsoleScreenBuffer, KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleActiveScreenBuffer, KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleWindowInfo, KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleScreenBufferSize, KERNEL32),
		HOOK_ITEM_BY_NAME(SetCurrentConsoleFontEx, KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleScreenBufferInfoEx, KERNEL32),
		HOOK_ITEM_BY_NAME(GetLargestConsoleWindowSize, KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleCursorPosition, KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleCursorInfo, KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(GetCurrentConsoleFont,		KERNEL32),
		HOOK_ITEM_BY_NAME(GetConsoleFontSize,			KERNEL32),
		/* ************************ */

		// https://conemu.github.io/en/MicrosoftBugs.html#chcp_hung
		HOOK_ITEM_BY_NAME(SetConsoleCP,			KERNEL32),
		HOOK_ITEM_BY_NAME(SetConsoleOutputCP,	KERNEL32),
		/* ************************ */
		{0}
	};

	if (InitHooks(HooksConsole) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

bool InitHooksExecutor()
{
	HLOG1("InitHooksExecutor",0);
	bool lbRc = false;

	HookItem HooksCommon[] =
	{
		/* ************************ */
		HOOK_ITEM_BY_LIBR(ExitProcess,			KERNEL32, 0, ghKernel32), // Hook the kernel32.dll, not kernelbase.dll
		HOOK_ITEM_BY_NAME(TerminateProcess,		KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(CreateThread,			KERNEL32),
		HOOK_ITEM_BY_NAME(SetThreadContext,		KERNEL32),
		HOOK_ITEM_BY_NAME(TerminateThread,		KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(CreateProcessA,		KERNEL32),
		HOOK_ITEM_BY_NAME(CreateProcessW,		KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(SetCurrentDirectoryA, KERNEL32),
		HOOK_ITEM_BY_NAME(SetCurrentDirectoryW, KERNEL32),
		/* ************************ */
		HOOK_ITEM_BY_NAME(ShellExecuteExA,		SHELL32),
		HOOK_ITEM_BY_NAME(ShellExecuteExW,		SHELL32),
		HOOK_ITEM_BY_NAME(ShellExecuteA,		SHELL32),
		HOOK_ITEM_BY_NAME(ShellExecuteW,		SHELL32),
		/* ************************ */
		// OnWinExec/WinExec is used in DefTerm only
		/* ************************ */
		{0}
	};

	if (InitHooks(HooksCommon) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

// user32 & gdi32
bool InitHooksUser32()
{
	HLOG1("InitHooksUser32",0);
	bool lbRc = false;

	HookItem HooksUserGdi[] =
	{
		/* ************************ */
		HOOK_ITEM_BY_NAME(TrackPopupMenu,		USER32),
		HOOK_ITEM_BY_NAME(TrackPopupMenuEx,		USER32),
		HOOK_ITEM_BY_NAME(FlashWindow,			USER32),
		HOOK_ITEM_BY_NAME(FlashWindowEx,		USER32),
		HOOK_ITEM_BY_NAME(SetForegroundWindow,	USER32),
		HOOK_ITEM_BY_NAME(GetForegroundWindow,	USER32),
		HOOK_ITEM_BY_NAME(GetWindowRect,		USER32),
		HOOK_ITEM_BY_NAME(ScreenToClient,		USER32),
		/* ************************ */
		//HOOK_ITEM_BY_NAME(CreateWindowA,		user32), -- there is not such export
		//HOOK_ITEM_BY_NAME(CreateWindowW,		user32), -- there is not such export
		HOOK_ITEM_BY_NAME(CreateWindowExA,		USER32),
		HOOK_ITEM_BY_NAME(CreateWindowExW,		USER32),
		HOOK_ITEM_BY_NAME(ShowCursor,			USER32),
		HOOK_ITEM_BY_NAME(ShowWindow,			USER32),
		HOOK_ITEM_BY_NAME(SetFocus,				USER32),
		HOOK_ITEM_BY_NAME(SetParent,			USER32),
		HOOK_ITEM_BY_NAME(GetParent,			USER32),
		HOOK_ITEM_BY_NAME(GetWindow,			USER32),
		HOOK_ITEM_BY_NAME(GetAncestor,			USER32),
		HOOK_ITEM_BY_NAME(GetClassNameA,		USER32),
		HOOK_ITEM_BY_NAME(GetClassNameW,		USER32),
		HOOK_ITEM_BY_NAME(GetActiveWindow,		USER32),
		HOOK_ITEM_BY_NAME(MoveWindow,			USER32),
		HOOK_ITEM_BY_NAME(SetWindowPos,			USER32),
		HOOK_ITEM_BY_NAME(SetWindowLongA,		USER32),
		HOOK_ITEM_BY_NAME(SetWindowLongW,		USER32),
		#ifdef WIN64
		HOOK_ITEM_BY_NAME(SetWindowLongPtrA,	USER32),
		HOOK_ITEM_BY_NAME(SetWindowLongPtrW,	USER32),
		#endif
		HOOK_ITEM_BY_NAME(GetWindowLongA,		USER32),
		HOOK_ITEM_BY_NAME(GetWindowLongW,		USER32),
		#ifdef WIN64
		HOOK_ITEM_BY_NAME(GetWindowLongPtrA,	USER32),
		HOOK_ITEM_BY_NAME(GetWindowLongPtrW,	USER32),
		#endif
		HOOK_ITEM_BY_NAME(GetWindowTextLengthA,	USER32),
		HOOK_ITEM_BY_NAME(GetWindowTextLengthW,	USER32),
		HOOK_ITEM_BY_NAME(GetWindowTextA,		USER32),
		HOOK_ITEM_BY_NAME(GetWindowTextW,		USER32),
		//
		HOOK_ITEM_BY_NAME(GetWindowPlacement,	USER32),
		HOOK_ITEM_BY_NAME(SetWindowPlacement,	USER32),
		HOOK_ITEM_BY_NAME(mouse_event,			USER32),
		HOOK_ITEM_BY_NAME(SendInput,			USER32),
		HOOK_ITEM_BY_NAME(PostMessageA,			USER32),
		HOOK_ITEM_BY_NAME(PostMessageW,			USER32),
		HOOK_ITEM_BY_NAME(SendMessageA,			USER32),
		HOOK_ITEM_BY_NAME(SendMessageW,			USER32),
		HOOK_ITEM_BY_NAME(GetMessageA,			USER32),
		HOOK_ITEM_BY_NAME(GetMessageW,			USER32),
		HOOK_ITEM_BY_NAME(PeekMessageA,			USER32),
		HOOK_ITEM_BY_NAME(PeekMessageW,			USER32),
		HOOK_ITEM_BY_NAME(MessageBeep,			USER32),
		HOOK_ITEM_BY_NAME(CreateDialogParamA,	USER32),
		HOOK_ITEM_BY_NAME(CreateDialogParamW,	USER32),
		HOOK_ITEM_BY_NAME(CreateDialogIndirectParamA, USER32),
		HOOK_ITEM_BY_NAME(CreateDialogIndirectParamW, USER32),
		HOOK_ITEM_BY_NAME(DialogBoxIndirectParamAorW, USER32),
		HOOK_ITEM_BY_NAME(SetMenu,				USER32),
		HOOK_ITEM_BY_NAME(GetDC,				USER32),
		HOOK_ITEM_BY_NAME(GetDCEx,				USER32),
		HOOK_ITEM_BY_NAME(ReleaseDC,			USER32),
		/* ************************ */

		/* ************************ */
		HOOK_ITEM_BY_NAME(StretchDIBits,		GDI32),
		HOOK_ITEM_BY_NAME(BitBlt,				GDI32),
		HOOK_ITEM_BY_NAME(StretchBlt,			GDI32),
		/* ************************ */
		{0}
	};

	if (InitHooks(HooksUserGdi) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

bool InitHooksFarExe()
{
	HLOG1("InitHooksFarExe",0);
	bool lbRc = false;

	// Проверить, фар ли это? Если нет - можно не ставить HooksFarOnly
	bool lbIsFar = false;
	wchar_t* pszExe = (wchar_t*)calloc(MAX_PATH+1,sizeof(wchar_t));
	if (pszExe)
	{
		if (GetModuleFileName(NULL, pszExe, MAX_PATH))
		{
			if (IsFarExe(pszExe))
				lbIsFar = true;
		}
		free(pszExe);
	}

	HookItem HooksFarOnly[] =
	{
		/* ************************ */
		HOOK_ITEM_BY_NAME(CompareStringW, KERNEL32),
		HOOK_ITEM_BY_NAME(WaitForMultipleObjects, KERNEL32),
		/* ************************ */
		{}
	};

	if (lbIsFar)
	{
		if (InitHooks(HooksFarOnly) < 0)
			goto wrap;
	}

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}

bool InitHooksCmdExe()
{
	if (!gbIsCmdProcess)
		return true;

	HLOG1("InitHooksCmdExe",0);
	bool lbRc = false;

	HookItem HooksCmdOnly[] =
	{
		// Vista and below: AdvApi32.dll
		// **NB** In WinXP this module is not linked statically
		HOOK_ITEM_BY_NAME(RegQueryValueExW, IsWin7() ? KERNEL32 : ADVAPI32),
		{}
	};

	if (InitHooks(HooksCmdOnly) < 0)
		goto wrap;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}




/* ************************ */
/* Default Terminal support */
/* ************************ */
bool InitHooksDefTerm()
{
	HLOG1("InitHooksDefTerm",0);
	bool lbRc = false;

	// These functions are required for seizing console and behave as Default Terminal

	HookItem HooksCommon[] =
	{
		HOOK_ITEM_BY_NAME(CreateProcessW,		KERNEL32),
		// Need to "hook" OnCreateProcessA because it is used in "OnWinExec"
		HOOK_ITEM_BY_NAME(CreateProcessA,		KERNEL32),
		// Used in some programs, Issue 853
		HOOK_ITEM_BY_NAME(WinExec,				KERNEL32),
		// Need for hook "Run as administrator"
		HOOK_ITEM_BY_NAME(ShellExecuteExW,		SHELL32),
		{}
	};
	HookItem HooksAllocConsole[] =
	{
		// gh-888, gh-55: Allow to use ConEmu as default console in third-party applications
		HOOK_ITEM_BY_NAME(AllocConsole,			KERNEL32), // Only for "*.vshost.exe"?
		{}
	};
	HookItem HooksCmdLine[] =
	{
		// Issue 1125: "Run as administrator" too. Must be last export
		HOOK_ITEM_BY_ORDN(ShellExecCmdLine,		SHELL32,   265),
		{}
	};
	HookItem HooksVshost[] =
	{
		// Issue 1312: .Net applications runs in "*.vshost.exe" helper GUI application when debugging
		// AllocConsole moved to HooksCommon
		HOOK_ITEM_BY_NAME(ShowWindow,			USER32),
		/* ************************ */
		{}
	};
	// Required in VisualStudio and CodeBlocks (gdb) debuggers
	// Don't restrict to them, other Dev Envs may behave in similar way
	HookItem HooksDevStudio[] =
	{
		HOOK_ITEM_BY_NAME(ResumeThread,			KERNEL32),
		HOOK_ITEM_BY_NAME(GetConsoleProcessList,KERNEL32),
		/* ************************ */
		{}
	};

	// Required for hooking in OS
	// bool check
	if (!InitHooksLibrary())
		goto wrap;

	// Start our functions
	if (InitHooks(HooksCommon) < 0)
		goto wrap;

	if (gbIsNetVsHost || gbPrepareDefaultTerminal)
	{
		if (InitHooks(HooksAllocConsole) < 0)
			goto wrap;
	}

	// Windows 7. There is new undocumented function "ShellExecCmdLine" used by Explorer
	if (IsWin7())
	{
		if (InitHooks(HooksCmdLine) < 0)
			goto wrap;
	}

	// "*.vshost.exe" uses special helper
	if (gbIsNetVsHost)
	{
		if (InitHooks(HooksVshost) < 0)
			goto wrap;
	}

	// Required in VisualStudio and CodeBlocks (gdb) debuggers
	// Don't restrict to them, other Dev Envs may behave in similar way
	{
		if (InitHooks(HooksDevStudio) < 0)
			goto wrap;
	}

	gnDllState |= ds_HooksDefTerm;

	lbRc = true;
wrap:
	HLOGEND1();

	return lbRc;
}



/* ******************* */
/* Console or ChildGui */
/* ******************* */

bool InitHooksCommon()
{
	if (!(gnDllState & ds_HooksDefTerm))
	{
		if (!InitHooksLibrary())
			return false;
	}

	if (!InitHooksKernel())
		return false;

	if (!InitHooksExecutor())
		return false;

	if (!InitHooksDebugging())
		return false;

	if (!InitHooksConsole())
		return false;

	if (!InitHooksUser32())
		return false;

	if (!InitHooksFarExe())
		return false;

	if (!InitHooksCmdExe())
		return false;

	gnDllState |= ds_HooksCommon;

	return true;
}
