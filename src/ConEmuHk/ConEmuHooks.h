
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

// This file is used in ConEmuPlugin and ExtendedConsole modules

#include <windows.h>

#include "../common/Common.h"

// Options are set when the process is Far Manager
struct HookModeFar
{
	// size of the struct
	DWORD cbSize;
	// set to TRUE from ConEmu.dll Far plugin !!! MUST BE FIRST BOOL !!!
	BOOL  bFarHookMode;
	// have to hook: OnCompareStringW
	BOOL  bFarUseAsciiSort;
	// -> OnShellExecuteXXX
	BOOL  bShellNoZoneCheck;
	// on (Read/Peek)ConsoleInput(A/W) send info to GUI/Settings/Debug
	BOOL  bMonitorConsoleInput;
	// on EMenu call show the menu at mouse cursor position
	BOOL  bPopupMenuPos;
	// increase buffer height during console applications execution (ignored in "far.exe /w")
	BOOL  bLongConsoleOutput;
	// Far Manager version info
	FarVersion farVer;
	// Callback to call (in plugin) when Far Manager tries to read console input.
	// Sort of a hack to execute the code when Far is in synchronous api aware mode.
	void  (WINAPI* onCurDirChanged)();
};

extern struct HookModeFar gFarMode;
extern SrvLogString_t gfnSrvLogString;

struct HookCallbackArg
{
	BOOL         bMainThread;
	// pointer to variable with result of original function
	LPVOID       lpResult;
	// arguments (or pointers to them) of original function, casted to DWORD_PTR
	DWORD_PTR    lArguments[10];
};

// PreCallBack may returns (FALSE) to skip original function calling,
//		in that case, caller got pArgs->lResult
// PostCallBack can modify pArgs->lResult only (callback function result is ignored)
// ExceptCallBack can modify pArgs->lResult only (callback function result is ignored)
typedef BOOL (WINAPI* HookItemCallback_t)(HookCallbackArg* pArgs);

typedef bool (__stdcall* SetHookCallbacks_t)(const char* ProcName, const wchar_t* DllName, HMODULE hCallbackModule,
        HookItemCallback_t PreCallBack, HookItemCallback_t PostCallBack,
        HookItemCallback_t ExceptCallBack);
//extern SetHookCallbacks_t SetHookCallbacks; // = NULL;

// Callback definition
typedef VOID (WINAPI* OnLibraryLoaded_t)(HMODULE ahModule);

typedef void (__stdcall* SetLoadLibraryCallback_t)(HMODULE ahCallbackModule,
        OnLibraryLoaded_t afOnLibraryLoaded, OnLibraryLoaded_t afOnLibraryUnLoaded);
//extern SetLoadLibraryCallback_t SetLoadLibraryCallback; // = NULL;

typedef void (__stdcall* SetFarHookMode_t)(struct HookModeFar *apFarMode);
//extern SetFarHookMode_t SetFarHookMode;

#define SETARGS(r) HookCallbackArg args = {(hkIsMainThread)}; args.lpResult = (LPVOID)(r)
#define SETARGS1(r,a1) SETARGS(r); args.lArguments[0] = (DWORD_PTR)(a1)
#define SETARGS2(r,a1,a2) SETARGS1(r,a1); args.lArguments[1] = (DWORD_PTR)(a2)
#define SETARGS3(r,a1,a2,a3) SETARGS2(r,a1,a2); args.lArguments[2] = (DWORD_PTR)(a3)
#define SETARGS4(r,a1,a2,a3,a4) SETARGS3(r,a1,a2,a3); args.lArguments[3] = (DWORD_PTR)(a4)
#define SETARGS5(r,a1,a2,a3,a4,a5) SETARGS4(r,a1,a2,a3,a4); args.lArguments[4] = (DWORD_PTR)(a5)
#define SETARGS6(r,a1,a2,a3,a4,a5,a6) SETARGS5(r,a1,a2,a3,a4,a5); args.lArguments[5] = (DWORD_PTR)(a6)
#define SETARGS7(r,a1,a2,a3,a4,a5,a6,a7) SETARGS6(r,a1,a2,a3,a4,a5,a6); args.lArguments[6] = (DWORD_PTR)(a7)
#define SETARGS8(r,a1,a2,a3,a4,a5,a6,a7,a8) SETARGS7(r,a1,a2,a3,a4,a5,a6,a7); args.lArguments[7] = (DWORD_PTR)(a8)
#define SETARGS9(r,a1,a2,a3,a4,a5,a6,a7,a8,a9) SETARGS8(r,a1,a2,a3,a4,a5,a6,a7,a8); args.lArguments[8] = (DWORD_PTR)(a9)
#define SETARGS10(r,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10) SETARGS9(r,a1,a2,a3,a4,a5,a6,a7,a8,a9); args.lArguments[9] = (DWORD_PTR)(a10)
// !!! WARNING !!! DWORD_PTR lArguments[10]; - maximum - 10 arguments
