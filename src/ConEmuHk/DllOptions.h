
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

#include <windows.h>

#include "../common/Common.h"

// some forward declarations
struct CEStartupEnv;
struct HookModeFar;
struct InQueue;
class CDefTermHk;
class AsyncCmdQueue;

extern HMODULE ghOurModule;

extern wchar_t gsConEmuBaseDir[MAX_PATH + 1];

// Only exe name of current process
extern wchar_t gsExeName[80];
// cygwin/msys/clink and so on...
extern CEActiveAppFlags gnExeFlags; // = caf_Standard

extern CEStartupEnv* gpStartEnv;
extern bool    gbConEmuCProcess;
extern bool    gbConEmuConnector;
extern DWORD   gnSelfPID;
extern BOOL    gbSelfIsRootConsoleProcess;
extern BOOL    gbForceStartPipeServer;
extern DWORD   gnServerPID;
extern DWORD   gnPrevAltServerPID;
extern DWORD   gnGuiPID;
extern HWND    ghConWnd; // Console window
extern HWND    ghConEmuWnd; // Root! window
extern HWND    ghConEmuWndDC; // ConEmu DC window
extern HWND    ghConEmuWndBack; // ConEmu Back window - holder for GUI client
extern BOOL    gbWasBufferHeight;
extern BOOL    gbNonGuiMode;
extern DWORD   gnImageSubsystem;
extern DWORD   gnImageBits;
extern wchar_t gsInitConTitle[512];
extern BOOL    gbTrueColorServerRequested;


/* ************ Globals for Far Hooks ************ */
extern HookModeFar gFarMode;
extern bool    gbIsFarProcess;
extern InQueue gInQueue;
extern HANDLE  ghConsoleCursorChanged;
extern SrvLogString_t gfnSrvLogString;
/* ************ Globals for Far Hooks ************ */

/* ************ Globals for cmd.exe/clink ************ */
extern bool     gbIsCmdProcess;
extern int      gnCmdInitialized; // 0 - Not already, 1 - OK, -1 - Fail
extern bool     gbAllowClinkUsage;
extern bool     gbClinkInjectRequested;
extern bool     gbAllowUncPaths;
extern bool     gbCurDirChanged;
/* ************ Globals for cmd.exe/clink ************ */

/* ************ Globals for powershell ************ */
extern bool gbIsPowerShellProcess;
extern bool gbPowerShellMonitorProgress;
extern WORD gnConsolePopupColors;
extern int  gnPowerShellProgressValue;
/* ************ Globals for powershell ************ */

/* ************ Globals for Node.JS ************ */
extern bool gbIsNodeJsProcess;
/* ************ Globals for Node.JS ************ */

/* ************ Globals for cygwin/msys ************ */
extern bool gbIsBashProcess;
extern bool gbIsSshProcess;
extern bool gbIsLessProcess;
/* ************ Globals for cygwin/msys ************ */

/* ************ Globals for ViM ************ */
extern bool gbIsVimProcess;
extern bool gbIsVimAnsi;
/* ************ Globals for ViM ************ */

/* ************ Globals for Plink ************ */
extern bool gbIsPlinkProcess;
/* ************ Globals for ViM ************ */

/* ************ Globals for MinTTY ************ */
extern bool gbIsMinTtyProcess;
/* ************ Globals for ViM ************ */

/* ************ Globals for HIEW32.EXE ************ */
extern bool gbIsHiewProcess;
/* ************ Globals for HIEW32.EXE ************ */

/* ************ Globals for DosBox.EXE ************ */
extern bool gbDosBoxProcess;
/* ************ Globals for DosBox.EXE ************ */

/* ************ Don't show VirtualAlloc errors ************ */
extern bool gbSkipVirtualAllocErr;
/* ************ Don't show VirtualAlloc errors ************ */

/* ************ Hooking time functions ************ */
extern DWORD gnTimeEnvVarLastCheck;
extern wchar_t gszTimeEnvVarSave[32];
/* ************ Hooking time functions ************ */

/* ************ Globals for "Default terminal ************ */
extern bool gbPrepareDefaultTerminal;
extern bool gbIsNetVsHost; // *.vshost.exe
extern bool gbIsVStudio; // devenv.exe or WDExpress.exe
extern bool gbIsVSDebugger; // msvsmon.exe
extern bool gbIsVSDebugConsole; // VsDebugConsole.exe
extern bool gbIsVsCode; // code.exe
extern int  gnVsHostStartConsole;
extern bool gbIsGdbHost; // gdb.exe
extern CDefTermHk* gpDefTerm;
extern AsyncCmdQueue* gpAsyncCmdQueue;
/* ************ Globals for "Default terminal ************ */


void SetConEmuHkWindows(HWND hDcWnd, HWND hBackWnd);
void SetServerPID(DWORD anMainSrvPid);

bool isProcessAnsi();
bool isSuppressBells();
#define LogBeepSkip(x) OutputDebugString(x)
