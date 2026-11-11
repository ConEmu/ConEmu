
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


#include <windows.h>

#include "../common/Common.h"
#include "../common/MFileMapping.h"

#include "DbgHooks.h"

extern MFileMapping<CESERVER_CONSOLE_APP_MAPPING> *gpAppMap;
CESERVER_CONSOLE_MAPPING_HDR* GetConMap(BOOL abForceRecreate=FALSE);
CESERVER_CONSOLE_APP_MAPPING* GetAppMapPtr();
CESERVER_CONSOLE_APP_MAPPING* UpdateAppMapFlags(DWORD nFlags/*enum CEReadConsoleInputFlags*/);
CESERVER_CONSOLE_APP_MAPPING* UpdateAppMapRows(LONG anLastConsoleRow, bool abForce);
void OnConWndChanged(HWND ahNewConWnd);
void CheckAnsiConVar(LPCWSTR asName);

enum CEReadConsoleInputFlags
{
	rcif_Ansi      = 1,
	rcif_Unicode   = 2,
	rcif_Peek      = 4,
	rcif_LLInput   = 8, // [Read|Peek]ConsoleInput[A|W]
};

DWORD DllStart_Continue();

typedef DWORD ConEmuHkDllState;
const ConEmuHkDllState
	ds_DllProcessAttach        = 0x00000001,
	ds_DllProcessDetach        = 0x00000002,
	ds_DllProcessDetachBlocked = 0x00000004,
	ds_DllMainThreadDetach     = 0x00000010,
	ds_DllDeinitializing       = 0x00000020,
	ds_HeapInitialized         = 0x00000040,
	ds_HeapDeinitialized       = 0x00000080,
	ds_DllStopping             = 0x00000100,
	ds_DllStopped              = 0x00000200,
	ds_HooksStarting           = 0x00001000,
	ds_HooksStopping           = 0x00002000,
	ds_HooksCommon             = 0x00004000, // InitHooksCommon() was called
	ds_HooksDefTerm            = 0x00008000, // InitHooksDefTerm() was called
	ds_HooksStarted            = 0x00010000,
	ds_HooksStartFailed        = 0x00020000,
	ds_HooksStopped            = 0x00040000,
	ds_DllStopNonFinal         = 0x00100000,
	ds_DllStopFinal            = 0x00200000,
	ds_OnTerminateThread       = 0x00400000,
	ds_OnTerminateProcess      = 0x00800000,
	ds_OnExitProcess           = 0x01000000,
	ds_OnDefTermShutdown       = 0x02000000,
	ds_DllStopSteps            = 0xF0000000,
	ds_Undefined = 0
;
extern ConEmuHkDllState gnDllState;

// Progress of DoDllStop, last one is ds_DllStopped
#define DLL_STOP_STEP(n) { _ASSERTEX((n)>0 && (n)<=15); gnDllState &= ~ds_DllStopSteps; gnDllState |= (((n) & 0xF) << 28); }

// xxxRaw is used internally, during hooks initialization and shutdown
#define HooksWereSetRaw ((gnDllState & ds_HooksStarted) && !(gnDllState & ds_HooksStopped))
// To be sure that hooks are active at the moment
#define HooksWereSet (HooksWereSetRaw && !(gnDllState & ds_HooksStopping))

//int WINAPI RequestLocalServer(/*[IN/OUT]*/RequestLocalServerParm* Parm);
struct AnnotationHeader;
extern AnnotationHeader* gpAnnotationHeader;
extern HANDLE ghCurrentOutBuffer;

void CheckHookServer();
extern bool gbHookServerForcedTermination;

struct CpConv
{
	// for example, "git add -p" uses codepage 1252 while printing thunks to be staged
	// that forces the printed text to be converted to nToCP before printing (OnWriteConsoleW)
	DWORD nFromCP, nToCP;
	// that parm may be used for overriding default console CP
	DWORD nDefaultCP;
};
extern struct CpConv gCpConv;


void GuiSetProgress(AnsiProgressStatus st, WORD pr, LPCWSTR pszName = nullptr);

#if defined(__GNUC__)
extern "C" {
#endif
	HWND WINAPI GetRealConsoleWindow();
	FARPROC WINAPI GetWriteConsoleW();
	int WINAPI RequestLocalServer(/*[IN/OUT]*/RequestLocalServerParm* Parm);
	FARPROC WINAPI GetLoadLibraryW();
	BOOL WINAPI RequestTrampolines(LPCWSTR asModule, HMODULE hModule);
#if defined(__GNUC__)
};
#endif

void DoDllStop(bool bFinal, ConEmuHkDllState bFromTerminate = ds_Undefined);

// Defined in "DbgHooks.h"
#ifdef USEHOOKLOG
	#include <intrin.h>

	#define getThreadId() WIN3264TEST(((DWORD*) __readfsdword(24))[9],GetCurrentThreadId())

	#define getTime GetTickCount

	// Originally from http://preshing.com/20120522/lightweight-in-memory-logging
	namespace HookLogger
	{
		struct Event
		{
			#ifdef _DEBUG
			DWORD tid;       // Thread ID
			DWORD dur;       // Step duration
			#endif
			const char* msg; // Info
			DWORD param;
			#ifdef _DEBUG
			LARGE_INTEGER cntr, cntr1; // PerformanceCounters
			#endif
		};
	 
		static const int BUFFER_SIZE = RELEASEDEBUGTEST(256,1024);   // Must be a power of 2
		extern Event g_events[BUFFER_SIZE];
		extern LONG g_pos;
		extern LARGE_INTEGER g_freq;

		static const int CRITICAL_BUFFER_SIZE = 256;
		struct CritInfo
		{
			uint64_t total;
			DWORD count;
			
			#ifdef _WIN64
			DWORD pad;
			#endif

			const char* msg;
		};
		extern CritInfo g_crit[CRITICAL_BUFFER_SIZE];
	 
		inline Event* Log(const char* msg, DWORD param)
		{
			// Get next event index
			LONG i = _InterlockedIncrement(&g_pos);
			// Write an event at this index
			Event* e = g_events + (i & (BUFFER_SIZE - 1));  // Wrap to buffer size
			#ifdef _DEBUG
			e->tid = getThreadId();
			//e->tick = getTime();
			QueryPerformanceCounter(&e->cntr);
			//e->tick = e->cntr.LowPart;
			//Event* ep = g_events + ((i-1) & (BUFFER_SIZE - 1));  // Wrap to buffer size
			//ep->dur = (DWORD)(e->cntr.QuadPart - ep->cntr.QuadPart);
			e->cntr1.QuadPart = 0;
			#endif
			e->msg = msg;
			e->param = param;
			return e;
		}

		extern void RunAnalyzer();
	}

	#undef getThreadId
	#undef getTime
#endif

// USEHOOKLOG may be defined in DbgHooks.h
// SKIPHOOKLOG may be used to disable logging in the exact *.cpp file
#if defined(USEHOOKLOG) && !defined(SKIPHOOKLOG)
	#define HLOG0(m,p) HookLogger::Event* es = HookLogger::Log(m,p);
	#define HLOG(m,p) es = HookLogger::Log(m,p);
	#define HLOGEND() QueryPerformanceCounter(&es->cntr1);
	#define HLOG1(m,p) HookLogger::Event* es1 = HookLogger::Log(m,p);
	#define HLOG1_(m,p) es1 = HookLogger::Log(m,p);
	#define HLOGEND1() QueryPerformanceCounter(&es1->cntr1);
	#define HLOG2(m,p) HookLogger::Event* es2 = HookLogger::Log(m,p);
	#define HLOG2_(m,p) es2 = HookLogger::Log(m,p);
	#define HLOGEND2() QueryPerformanceCounter(&es2->cntr1);
#else
	#define HLOG0(m,p)
	#define HLOG(m,p)
	#define HLOGEND()
	#define HLOG1(m,p)
	#define HLOG1_(m,p)
	#define HLOGEND1()
	#define HLOG2(m,p)
	#define HLOG2_(m,p)
	#define HLOGEND2()
#endif

#if defined(USEHOOKLOG) && !defined(SKIPDLLLOG)
	#define DLOG0(m,p) HookLogger::Event* es = HookLogger::Log(m,p);
	#define DLOG(m,p) es = HookLogger::Log(m,p);
	#define DLOGEND() QueryPerformanceCounter(&es->cntr1);
	#define DLOG1(m,p) HookLogger::Event* es1 = HookLogger::Log(m,p);
	#define DLOG1_(m,p) es1 = HookLogger::Log(m,p);
	#define DLOGEND1() QueryPerformanceCounter(&es1->cntr1);
#else
	#define DLOG0(m,p)
	#define DLOG(m,p)
	#define DLOGEND()
	#define DLOG1(m,p)
	#define DLOG1_(m,p)
	#define DLOGEND1()
#endif

enum ShowExeMsgBox
{
	smb_None = 0,
	smb_HardCoded,
	smb_Environment,
};
extern ShowExeMsgBox gbShowExeMsgBox;

#if defined(SHOW_EXE_TIMINGS) || defined(SHOW_EXE_MSGBOX) || defined(SHOWCREATEPROCESSTICK)
	extern DWORD gnLastShowExeTick;

	void force_print_timings(LPCWSTR s, HANDLE hTimingHandle, wchar_t (&szTimingMsg)[512]);

	#define prepare_timings  wchar_t szTimingMsg[512]; HANDLE hTimingHandle = GetStdHandle(STD_OUTPUT_HANDLE)
	#define print_timings(s) if (gbShowExeMsgBox == smb_HardCoded) { force_print_timings(s,hTimingHandle,szTimingMsg); }
#else
	#define prepare_timings
	#define print_timings(s)
#endif
