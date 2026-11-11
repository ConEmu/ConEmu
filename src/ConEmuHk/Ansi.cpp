
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

// #ANSI This file is expected to be moved almost completely to ConEmuCD

#include "../common/defines.h"
#include <winerror.h>
#include <winnt.h>
#include <tchar.h>
#include <limits>
#include <chrono>
#include <tuple>
#include <atomic>
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/ConsoleMixAttr.h"
#include "../common/CmdLine.h"
#include "../common/HandleKeeper.h"
#include "../common/MConHandle.h"
#include "../common/MRect.h"
#include "../common/MSectionSimple.h"
#include "../common/UnicodeChars.h"
#include "../common/WConsole.h"
#include "../common/WErrGuard.h"
#include "../common/MAtomic.h"

#include "Connector.h"
#include "ExtConsole.h"
#include "hlpConsole.h"
#include "hlpProcess.h"
#include "SetHook.h"

#include "hkConsole.h"
#include "hkStdIO.h"
#include "hkWindow.h"

///* ***************** */
#include "Ansi.h"

#include "../common/MHandle.h"
static DWORD gAnsiTlsIndex = 0;

#include "DllOptions.h"
#include "../common/WObjects.h"
///* ***************** */

#ifdef _DEBUG
#define GH_1402
#endif

#undef isPressed
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)

#define ANSI_MAP_CHECK_TIMEOUT 1000 // 1sec

#ifdef _DEBUG
#define DebugString(x) OutputDebugString(x)
#define DebugStringA(x) OutputDebugStringA(x)
#define DBG_XTERM(x) //CEAnsi::DebugXtermOutput(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#define DBG_XTERM(x) //CEAnsi::DebugXtermOutput(x)
#endif

/* ************ Globals ************ */
// extern HMODULE ghOurModule; // Хэндл нашей dll'ки (здесь хуки не ставятся)
#include "MainThread.h"

/* ************ Globals for SetHook ************ */
extern HWND    ghConWnd;      // RealConsole  // NOLINT(readability-redundant-declaration)
extern HWND    ghConEmuWnd;   // Root! ConEmu window  // NOLINT(readability-redundant-declaration)
// ReSharper disable once CppInconsistentNaming
extern HWND    ghConEmuWndDC; // ConEmu DC window  // NOLINT(readability-redundant-declaration)
// ReSharper disable once CppInconsistentNaming
extern DWORD   gnGuiPID;  // NOLINT(readability-redundant-declaration)
// extern wchar_t gsInitConTitle[512];
/* ************ Globals for SetHook ************ */

/* ************ Globals for xTerm/ViM ************ */
TODO("BufferWidth")
typedef DWORD XTermAltBufferFlags;
const XTermAltBufferFlags
	xtb_AltBuffer          = 0x0001,
	xtb_StoredCursor       = 0x0002,
	xtb_StoredScrollRegion = 0x0004,
	xtb_None               = 0;
static struct XTermAltBufferData
{
	XTermAltBufferFlags Flags;
	int    BufferSize;
	COORD  CursorPos;
	SHORT  ScrollStart, ScrollEnd;
} gXTermAltBuffer = {};
/* ************ Globals for xTerm/ViM ************ */

BOOL WINAPI OnCreateProcessW(LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

// These handles must be registered and released in OnCloseHandle.
HANDLE CEAnsi::ghAnsiLogFile = nullptr;
bool CEAnsi::gbAnsiLogCodes = false;
LONG CEAnsi::gnEnterPressed = 0;
bool CEAnsi::gbAnsiLogNewLine = false;
bool CEAnsi::gbAnsiWasNewLine = false;
MSectionSimple* CEAnsi::gcsAnsiLogFile = nullptr;

// VIM, etc. Some programs waiting control keys as xterm sequences. Need to inform ConEmu GUI.
bool CEAnsi::gbIsXTermOutput = false;
// On Windows 10 we have ENABLE_VIRTUAL_TERMINAL_PROCESSING which implies XTerm mode
DWORD CEAnsi::gPrevConOutMode = 0;
// Let RefreshXTermModes() know what to restore
CEAnsi::TermModeSet CEAnsi::gWasXTermModeSet[tmc_Last] = {};

static MConHandle ghConOut(L"CONOUT$"), ghStdOut(L""), ghStdErr(L"");  // NOLINT(clang-diagnostic-exit-time-destructors)

/* ************ Export ANSI printings ************ */
LONG gnWriteProcessed = 0;
FARPROC CallWriteConsoleW = nullptr;
BOOL WINAPI WriteProcessed3(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, HANDLE hConsoleOutput)
{
	InterlockedIncrement(&gnWriteProcessed);
	DWORD nNumberOfCharsWritten = 0;
	if ((nNumberOfCharsToWrite == static_cast<DWORD>(-1)) && lpBuffer)
	{
		nNumberOfCharsToWrite = lstrlen(lpBuffer);
	}
	const BOOL bRc = CEAnsi::OurWriteConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, &nNumberOfCharsWritten, nullptr);
	if (lpNumberOfCharsWritten) *lpNumberOfCharsWritten = nNumberOfCharsWritten;
	InterlockedDecrement(&gnWriteProcessed);
	return bRc;
}

HANDLE GetStreamHandle(WriteProcessedStream Stream)
{
	HANDLE hConsoleOutput;
	switch (Stream)  // NOLINT(clang-diagnostic-switch-enum)
	{
	case wps_Output:
		hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE); break;
	case wps_Error:
		hConsoleOutput = GetStdHandle(STD_ERROR_HANDLE); break;
	default:
		SetLastError(ERROR_INVALID_PARAMETER);
		return nullptr;
	}
	return hConsoleOutput;
}
BOOL WINAPI WriteProcessed2(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, WriteProcessedStream Stream)
{
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hConsoleOutput = GetStreamHandle(Stream);
	if (!hConsoleOutput || (hConsoleOutput == INVALID_HANDLE_VALUE))
		return FALSE;
	return WriteProcessed3(lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, hConsoleOutput);
}
BOOL WINAPI WriteProcessed(LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	return WriteProcessed2(lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, wps_Output);
}
BOOL WINAPI WriteProcessedA(LPCSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, WriteProcessedStream Stream)
{
	BOOL lbRc = FALSE;
	CEAnsi* pObj = nullptr;

	ORIGINAL_KRNL(WriteConsoleA);

	if ((nNumberOfCharsToWrite == static_cast<DWORD>(-1)) && lpBuffer)
	{
		nNumberOfCharsToWrite = lstrlenA(lpBuffer);
	}

	// Nothing to write? Or flush buffer?
	if (!lpBuffer || !nNumberOfCharsToWrite || Stream == wps_None)
	{
		if (lpNumberOfCharsWritten)
			*lpNumberOfCharsWritten = 0;
		lbRc = TRUE;
		goto fin;
	}

	pObj = CEAnsi::Object();

	if (pObj)
		lbRc = pObj->OurWriteConsoleA(GetStreamHandle(Stream), lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten);
	else
		lbRc = F(WriteConsoleA)(GetStreamHandle(Stream), lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, nullptr);

fin:
	return lbRc;
}
/* ************ Export ANSI printings ************ */

void DebugStringUtf8(LPCWSTR asMessage)
{
	#ifdef _DEBUG
	if (!asMessage || !*asMessage)
		return;
	// Only ConEmuC debugger is able to show Utf-8 encoded debug strings
	// So, set bUseUtf8 to false if VS debugger is required
	static bool bUseUtf8 = false;
	if (!bUseUtf8)
	{
		DebugString(asMessage);
		return;
	}
	const int iLen = lstrlen(asMessage);
	char szUtf8[200];
	CEStrA buffer;
	char* pszUtf8 = ((iLen * 3 + 5) < static_cast<int>(countof(szUtf8))) ? szUtf8 : buffer.GetBuffer(iLen * 3 + 5);
	if (!pszUtf8)
		return;
	pszUtf8[0] = '\xEF'; pszUtf8[1] = '\xBB'; pszUtf8[2] = '\xBF';
	const int iCvt = WideCharToMultiByte(CP_UTF8, 0, asMessage, iLen, pszUtf8+3, iLen*3+1, nullptr, nullptr);
	if (iCvt > 0)
	{
		_ASSERTE(iCvt < (iLen*3+2));
		pszUtf8[iCvt+3] = 0;
		DebugStringA(pszUtf8);
	}
	#endif
}

void CEAnsi::InitAnsiLog(LPCWSTR asFilePath, const bool LogAnsiCodes)
{
	gbAnsiLogCodes = LogAnsiCodes;

	// Already initialized?
	if (ghAnsiLogFile)
		return;

	ScopedObject(CLastErrorGuard);

	gcsAnsiLogFile = new MSectionSimple(true);
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hLog = CreateFile(asFilePath, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hLog && (hLog != INVALID_HANDLE_VALUE))
	{
		// Succeeded
		ghAnsiLogFile = hLog;
	}
	else
	{
		SafeDelete(gcsAnsiLogFile);
	}
}

void CEAnsi::DoneAnsiLog(bool bFinal)
{
	if (!ghAnsiLogFile)
		return;

	if (gbAnsiLogNewLine || (gnEnterPressed > 0))
	{
		CEAnsi::WriteAnsiLogUtf8("\n", 1);
	}

	if (!bFinal)
	{
		FlushFileBuffers(ghAnsiLogFile);
	}
	else
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		HANDLE h = ghAnsiLogFile;
		ghAnsiLogFile = nullptr;
		CloseHandle(h);
		SafeDelete(gcsAnsiLogFile);
	}
}

UINT CEAnsi::GetCodePage()
{
	const UINT cp = gCpConv.nDefaultCP ? gCpConv.nDefaultCP : GetConsoleOutputCP();
	return cp;
}

// Intended to log some WinAPI functions
void CEAnsi::WriteAnsiLogFormat(const char* format, ...)
{
	if (!ghAnsiLogFile || !gbAnsiLogCodes || !format)
		return;
	ScopedObject(CLastErrorGuard);

	WriteAnsiLogTime();

	va_list argList;
	va_start(argList, format);
	char func_buffer[200] = "";
	if (S_OK == StringCchVPrintfA(func_buffer, countof(func_buffer), format, argList))
	{
		static char s_ExeName[80] = "";
		if (!*s_ExeName)
			WideCharToMultiByte(CP_UTF8, 0, gsExeName, -1, s_ExeName, countof(s_ExeName)-1, nullptr, nullptr);

		char log_string[300] = "";
		msprintf(log_string, countof(log_string), "\x1B]9;11;\"%s: %s\"\x7", s_ExeName, func_buffer);
		WriteAnsiLogUtf8(log_string, static_cast<DWORD>(strlen(log_string)));
	}
	va_end(argList);
}

void CEAnsi::WriteAnsiLogTime()
{
	if (!ghAnsiLogFile || !gbAnsiLogCodes)
		return;
	static DWORD last_write_tick_ = 0;

	const DWORD min_diff = 500;
	const DWORD cur_tick = GetTickCount();
	const DWORD cur_diff = cur_tick - last_write_tick_;

	if (!last_write_tick_ || (cur_diff >= min_diff))
	{
		last_write_tick_ = cur_tick;
		SYSTEMTIME st = {};
		GetLocalTime(&st);
		char time_str[40];
		// We should NOT use WriteAnsiLogFormat here!
		msprintf(time_str, std::size(time_str), "\x1B]9;11;\"%02u:%02u:%02u.%03u\"\x7",
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		WriteAnsiLogUtf8(time_str, static_cast<DWORD>(strlen(time_str)));
	}
}

BOOL CEAnsi::WriteAnsiLogUtf8(const char* lpBuffer, DWORD nChars)
{
	if (!ghAnsiLogFile || !lpBuffer || !nChars)
		return FALSE;
	// Handle multi-thread writers
	// But multi-process writers can't be handled correctly
	MSectionLockSimple lock; lock.Lock(gcsAnsiLogFile, 500);
	SetFilePointer(ghAnsiLogFile, 0, nullptr, FILE_END);
	ORIGINAL_KRNL(WriteFile);
	DWORD nWritten = 0;
	const BOOL bWrite = F(WriteFile)(ghAnsiLogFile, lpBuffer, nChars, &nWritten, nullptr);
	UNREFERENCED_PARAMETER(nWritten);
	gnEnterPressed = 0; gbAnsiLogNewLine = false;
	gbAnsiWasNewLine = (lpBuffer[nChars-1] == '\n');
	return bWrite;
}

// This may be called to log ReadConsoleA result
void CEAnsi::WriteAnsiLogA(LPCSTR lpBuffer, DWORD nChars)
{
	if (!ghAnsiLogFile || !lpBuffer || !nChars)
		return;

	ScopedObject(CLastErrorGuard);

	const UINT cp = GetCodePage();
	if (cp == CP_UTF8)
	{
		bool writeLineFeed = false;
		if (gbAnsiLogNewLine)
		{
			if ((lpBuffer[0] == '\n') || ((nChars > 1) && (lpBuffer[0] == '\r') && (lpBuffer[1] == '\n')))
			{
				gbAnsiLogNewLine = false;
			}
			else
			{
				writeLineFeed = true;
			}
		}

		WriteAnsiLogTime();
		if (writeLineFeed)
			WriteAnsiLogUtf8("\n", 1);
		WriteAnsiLogUtf8(lpBuffer, nChars);
	}
	else
	{
		// We don't check here for gbAnsiLogNewLine, because some codepages may even has different codes for CR+LF
		wchar_t sBuf[80 * 3];
		CEStr buffer;
		const int nNeed = MultiByteToWideChar(cp, 0, lpBuffer, nChars, nullptr, 0);
		if (nNeed < 1)
			return;
		wchar_t* pszBuf = (nNeed <= static_cast<int>(countof(sBuf))) ? sBuf : buffer.GetBuffer(nNeed);
		if (!pszBuf)
			return;
		const int nLen = MultiByteToWideChar(cp, 0, lpBuffer, nChars, pszBuf, nNeed);
		// Must be OK, but check it
		if (nLen > 0 && nLen <= nNeed)
		{
			WriteAnsiLogW(pszBuf, nLen);
		}
	}
}

void CEAnsi::WriteAnsiLogW(LPCWSTR lpBuffer, DWORD nChars)
{
	if (!ghAnsiLogFile || !lpBuffer || !nChars)
		return;

	ScopedObject(CLastErrorGuard);

	WriteAnsiLogTime();

	// Cygwin (in RealConsole mode, not connector) don't write CR+LF to screen,
	// it uses SetConsoleCursorPosition instead after receiving '\n' from readline
	int iEnterShift = 0;
	if (gbAnsiLogNewLine)
	{
		if ((lpBuffer[0] == '\n') || ((nChars > 1) && (lpBuffer[0] == '\r') && (lpBuffer[1] == '\n')))
			gbAnsiLogNewLine = false;
		else
			iEnterShift = 1;
	}

	char sBuf[80 * 3]; // Will be enough for most cases
	CEStrA buffer;
	BOOL bWrite = FALSE;
	const int nNeed = WideCharToMultiByte(CP_UTF8, 0, lpBuffer, nChars, nullptr, 0, nullptr, nullptr);
	if (nNeed < 1)
		return;
	char* pszBuf = ((nNeed + iEnterShift + 1) <= static_cast<int>(countof(sBuf))) ? sBuf : buffer.GetBuffer(nNeed + iEnterShift + 1);
	if (!pszBuf)
		return;
	if (iEnterShift)
		pszBuf[0] = '\n';
	const int nLen = WideCharToMultiByte(CP_UTF8, 0, lpBuffer, nChars, pszBuf+iEnterShift, nNeed, nullptr, nullptr);
	// Must be OK, but check it
	if (nLen > 0 && nLen <= nNeed)
	{
		pszBuf[iEnterShift+nNeed] = 0;
		bWrite = WriteAnsiLogUtf8(pszBuf, nLen+iEnterShift);
	}
	std::ignore = bWrite;
}

void CEAnsi::WriteAnsiLogFarPrompt()
{
	_ASSERTE(ghAnsiLogFile!=nullptr && "Caller must check this");
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hCon = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfo(hCon, &csbi))
		return;
	CEStr buffer;
	DWORD nChars = csbi.dwSize.X;
	if (!buffer.GetBuffer(csbi.dwSize.X + 2))
		return;
	// We expect Far has already put "black user screen" and do CR/LF
	// if Far's keybar is hidden, we are on (csbi.dwSize.Y-1), otherwise - (csbi.dwSize.Y-2)
	_ASSERTE(csbi.dwCursorPosition.Y >= (csbi.dwSize.Y-2) && csbi.dwCursorPosition.Y <= (csbi.dwSize.Y-1));
	const COORD crFrom = {0, static_cast<SHORT>(csbi.dwCursorPosition.Y - 1)};
	if (ReadConsoleOutputCharacterW(hCon, buffer.data(), nChars, crFrom, &nChars)
		&& nChars)
	{
		wchar_t* pszBuf = buffer.data();
		// Do RTrim first
		while (nChars && (pszBuf[nChars - 1] == L' '))
			nChars--;
		// Add CR+LF
		pszBuf[nChars++] = L'\r'; pszBuf[nChars++] = L'\n';
		// And do the logging
		WriteAnsiLogW(pszBuf, nChars);
	}
}

void CEAnsi::AnsiLogEnterPressed()
{
	if (!ghAnsiLogFile)
		return;
	InterlockedIncrement(&gnEnterPressed);
	gbAnsiLogNewLine = true;
}

bool CEAnsi::GetFeatures(ConEmu::ConsoleFlags& features)
{
	struct FeaturesCache
	{
		ConEmu::ConsoleFlags features;
		BOOL result;
	};
	static FeaturesCache featuresCache{};
	// Don't use system_clock here, it fails in some cases during DllLoad (segment is not ready somehow)
	static DWORD nLastCheck{ 0 };

	if (!nLastCheck || (GetTickCount() - nLastCheck) > ANSI_MAP_CHECK_TIMEOUT)
	{
		CESERVER_CONSOLE_MAPPING_HDR* pMap = GetConMap();
		//	(CESERVER_CONSOLE_MAPPING_HDR*)malloc(sizeof(CESERVER_CONSOLE_MAPPING_HDR));
		if (pMap)
		{
			// bAnsiAllowed = ((pMap != nullptr) && (pMap->Flags & ConEmu::ConsoleFlags::ProcessAnsi));
			// bSuppressBells = ((pMap != nullptr) && (pMap->Flags & ConEmu::ConsoleFlags::SuppressBells));
			// Well, it's not so atomic as could be, but here we care only on proper features value
			// and we can't use std::atomic here because function is called during DllLoad
			InterlockedExchange(reinterpret_cast<LONG*>(&featuresCache.features), static_cast<LONG>(pMap->Flags));
			InterlockedExchange(reinterpret_cast<LONG*>(&featuresCache.result), static_cast<LONG>(true));
		}
		else
		{
			InterlockedExchange(reinterpret_cast<LONG*>(&featuresCache.features), 0);
			InterlockedExchange(reinterpret_cast<LONG*>(&featuresCache.result), 0);
		}
		nLastCheck = GetTickCount();
	}

	features = static_cast<ConEmu::ConsoleFlags>(InterlockedCompareExchange(reinterpret_cast<LONG*>(&featuresCache.features), 0, 0));
	const bool result = InterlockedCompareExchange(reinterpret_cast<LONG*>(&featuresCache.result), 0, 0);
	return result;
}

void CEAnsi::ReloadFeatures()
{
	ConEmu::ConsoleFlags features = ConEmu::ConsoleFlags::Empty;
	GetFeatures(features);
	mb_SuppressBells = (features & ConEmu::ConsoleFlags::SuppressBells);
}




//struct DisplayParm
//{
//	BOOL WasSet;
//	BOOL BrightOrBold;     // 1
//	BOOL Italic;           // 3
//	BOOL Underline;        // 4
//	BOOL BrightFore;       // 90-97
//	BOOL BrightBack;       // 100-107
//	int  TextColor;        // 30-37,38,39
//	BOOL Text256;          // 38
//    int  BackColor;        // 40-47,48,49
//    BOOL Back256;          // 48
//	// xterm
//	BOOL Inverse;
//} gDisplayParm = {};

CEAnsi::DisplayParm CEAnsi::gDisplayParm = {};

// void CEAnsi::DisplayParm::setWasSet(...) -- intentionally is not defined

void CEAnsi::DisplayParm::Reset(const bool full)
{
	if (full)
		*this = DisplayParm{};

	const WORD nDefColors = CEAnsi::GetDefaultTextAttr();
	_TextColor = CONFORECOLOR(nDefColors);
	_BackColor = CONBACKCOLOR(nDefColors);
	_WasSet = TRUE;
}
void CEAnsi::DisplayParm::setBrightOrBold(const bool val)
{
	if (!_WasSet)
		Reset(false);
	_BrightOrBold = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setItalic(const bool val)
{
	if (!_WasSet)
		Reset(false);
	_Italic = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setUnderline(const bool val)
{
	if (!_WasSet)
		Reset(false);
	_Underline = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setBrightFore(const bool val)
{
	if (!_WasSet)
		Reset(false);
	_BrightFore = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setBrightBack(const bool val)
{
	if (!_WasSet)
		Reset(false);
	_BrightBack = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setTextColor(const int val)
{
	if (!_WasSet)
		Reset(false);
	_TextColor = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setText256(const cbit val)
{
	if (!_WasSet)
		Reset(false);
	_Text256 = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setBackColor(const int val)
{
	if (!_WasSet)
		Reset(false);
	_BackColor = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setBack256(const cbit val)
{
	if (!_WasSet)
		Reset(false);
	_Back256 = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setInverse(const bool val)
{
	if (!_WasSet)
		Reset(false);
	_Inverse = val;
	_ASSERTE(_WasSet==TRUE);
}
void CEAnsi::DisplayParm::setCrossed(const bool val)
{
	if (!_WasSet)
		Reset(false);
	_Crossed = val;
	_ASSERTE(_WasSet==TRUE);
}

//struct DisplayCursorPos
//{
//    // Internal
//    COORD StoredCursorPos;
//	// Esc[?1h 	Set cursor key to application 	DECCKM
//	// Esc[?1l 	Set cursor key to cursor 	DECCKM
//	BOOL CursorKeysApp; // "1h"
//} gDisplayCursor = {};

CEAnsi::DisplayCursorPos CEAnsi::gDisplayCursor = {};

//struct DisplayOpt
//{
//	BOOL  WrapWasSet;
//	SHORT WrapAt; // Rightmost X coord (1-based)
//	//
//	BOOL  AutoLfNl; // LF/NL (default off): Automatically follow echo of LF, VT or FF with CR.
//	//
//	BOOL  ScrollRegion;
//	SHORT ScrollStart, ScrollEnd; // 0-based absolute line indexes
//	//
//	BOOL  ShowRawAnsi; // \e[3h display ANSI control characters (TRUE), \e[3l process ANSI (FALSE, normal mode)
//} gDisplayOpt;

CEAnsi::DisplayOpt CEAnsi::gDisplayOpt = {};

//const size_t cchMaxPrevPart = 160;
//static wchar_t gsPrevAnsiPart[cchMaxPrevPart] = {};
//static INT_PTR gnPrevAnsiPart = 0;
//static wchar_t gsPrevAnsiPart2[cchMaxPrevPart] = {};
//static INT_PTR gnPrevAnsiPart2 = 0;
//const  INT_PTR MaxPrevAnsiPart = 80;

#ifdef _DEBUG
static const wchar_t szAnalogues[32] =
{
	32, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788,
	9658, 9668, 8597, 8252,  182,  167, 9632, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660
};
#endif

/*static*/ SHORT CEAnsi::GetDefaultTextAttr()
{
	// Default console colors
	static SHORT clrDefault = 0;
	if (clrDefault)
		return clrDefault;

	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hIn = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfo(hIn, &csbi))
		return (clrDefault = 7);

	static SHORT Con2Ansi[16] = {0,4,2,6,1,5,3,7,8|0,8|4,8|2,8|6,8|1,8|5,8|3,8|7};
	clrDefault = Con2Ansi[CONFORECOLOR(csbi.wAttributes)]
		| static_cast<SHORT>(Con2Ansi[CONBACKCOLOR(csbi.wAttributes)] << 4);

	return clrDefault;
}


const CEAnsi::DisplayParm& CEAnsi::getDisplayParm()
{
	return gDisplayParm;
}

// static
void CEAnsi::ReSetDisplayParm(HANDLE hConsoleOutput, BOOL bReset, BOOL bApply)
{
	WARNING("Эту функу нужно дергать при смене буферов, закрытии дескрипторов, и т.п.");

	if (bReset || !gDisplayParm.getWasSet())
	{
		gDisplayParm.Reset(bReset);
	}

	if (bApply)
	{
		// to display
		ExtAttributesParm attr{};
		attr.StructSize = sizeof(attr);
		attr.ConsoleOutput = hConsoleOutput;
		//DWORD wAttrs = 0;

		// Ansi colors
		static DWORD ClrMap[8] = {0,4,2,6,1,5,3,7};
		// XTerm-256 colors
		static DWORD RgbMap[256] = {0,4,2,6,1,5,3,7, 8+0,8+4,8+2,8+6,8+1,8+5,8+3,8+7, // System Ansi colors
			/*16*/0x000000, /*17*/0x5f0000, /*18*/0x870000, /*19*/0xaf0000, /*20*/0xd70000, /*21*/0xff0000, /*22*/0x005f00, /*23*/0x5f5f00, /*24*/0x875f00, /*25*/0xaf5f00, /*26*/0xd75f00, /*27*/0xff5f00,
			/*28*/0x008700, /*29*/0x5f8700, /*30*/0x878700, /*31*/0xaf8700, /*32*/0xd78700, /*33*/0xff8700, /*34*/0x00af00, /*35*/0x5faf00, /*36*/0x87af00, /*37*/0xafaf00, /*38*/0xd7af00, /*39*/0xffaf00,
			/*40*/0x00d700, /*41*/0x5fd700, /*42*/0x87d700, /*43*/0xafd700, /*44*/0xd7d700, /*45*/0xffd700, /*46*/0x00ff00, /*47*/0x5fff00, /*48*/0x87ff00, /*49*/0xafff00, /*50*/0xd7ff00, /*51*/0xffff00,
			/*52*/0x00005f, /*53*/0x5f005f, /*54*/0x87005f, /*55*/0xaf005f, /*56*/0xd7005f, /*57*/0xff005f, /*58*/0x005f5f, /*59*/0x5f5f5f, /*60*/0x875f5f, /*61*/0xaf5f5f, /*62*/0xd75f5f, /*63*/0xff5f5f,
			/*64*/0x00875f, /*65*/0x5f875f, /*66*/0x87875f, /*67*/0xaf875f, /*68*/0xd7875f, /*69*/0xff875f, /*70*/0x00af5f, /*71*/0x5faf5f, /*72*/0x87af5f, /*73*/0xafaf5f, /*74*/0xd7af5f, /*75*/0xffaf5f,
			/*76*/0x00d75f, /*77*/0x5fd75f, /*78*/0x87d75f, /*79*/0xafd75f, /*80*/0xd7d75f, /*81*/0xffd75f, /*82*/0x00ff5f, /*83*/0x5fff5f, /*84*/0x87ff5f, /*85*/0xafff5f, /*86*/0xd7ff5f, /*87*/0xffff5f,
			/*88*/0x000087, /*89*/0x5f0087, /*90*/0x870087, /*91*/0xaf0087, /*92*/0xd70087, /*93*/0xff0087, /*94*/0x005f87, /*95*/0x5f5f87, /*96*/0x875f87, /*97*/0xaf5f87, /*98*/0xd75f87, /*99*/0xff5f87,
			/*100*/0x008787, /*101*/0x5f8787, /*102*/0x878787, /*103*/0xaf8787, /*104*/0xd78787, /*105*/0xff8787, /*106*/0x00af87, /*107*/0x5faf87, /*108*/0x87af87, /*109*/0xafaf87, /*110*/0xd7af87, /*111*/0xffaf87,
			/*112*/0x00d787, /*113*/0x5fd787, /*114*/0x87d787, /*115*/0xafd787, /*116*/0xd7d787, /*117*/0xffd787, /*118*/0x00ff87, /*119*/0x5fff87, /*120*/0x87ff87, /*121*/0xafff87, /*122*/0xd7ff87, /*123*/0xffff87,
			/*124*/0x0000af, /*125*/0x5f00af, /*126*/0x8700af, /*127*/0xaf00af, /*128*/0xd700af, /*129*/0xff00af, /*130*/0x005faf, /*131*/0x5f5faf, /*132*/0x875faf, /*133*/0xaf5faf, /*134*/0xd75faf, /*135*/0xff5faf,
			/*136*/0x0087af, /*137*/0x5f87af, /*138*/0x8787af, /*139*/0xaf87af, /*140*/0xd787af, /*141*/0xff87af, /*142*/0x00afaf, /*143*/0x5fafaf, /*144*/0x87afaf, /*145*/0xafafaf, /*146*/0xd7afaf, /*147*/0xffafaf,
			/*148*/0x00d7af, /*149*/0x5fd7af, /*150*/0x87d7af, /*151*/0xafd7af, /*152*/0xd7d7af, /*153*/0xffd7af, /*154*/0x00ffaf, /*155*/0x5fffaf, /*156*/0x87ffaf, /*157*/0xafffaf, /*158*/0xd7ffaf, /*159*/0xffffaf,
			/*160*/0x0000d7, /*161*/0x5f00d7, /*162*/0x8700d7, /*163*/0xaf00d7, /*164*/0xd700d7, /*165*/0xff00d7, /*166*/0x005fd7, /*167*/0x5f5fd7, /*168*/0x875fd7, /*169*/0xaf5fd7, /*170*/0xd75fd7, /*171*/0xff5fd7,
			/*172*/0x0087d7, /*173*/0x5f87d7, /*174*/0x8787d7, /*175*/0xaf87d7, /*176*/0xd787d7, /*177*/0xff87d7, /*178*/0x00afd7, /*179*/0x5fafd7, /*180*/0x87afd7, /*181*/0xafafd7, /*182*/0xd7afd7, /*183*/0xffafd7,
			/*184*/0x00d7d7, /*185*/0x5fd7d7, /*186*/0x87d7d7, /*187*/0xafd7d7, /*188*/0xd7d7d7, /*189*/0xffd7d7, /*190*/0x00ffd7, /*191*/0x5fffd7, /*192*/0x87ffd7, /*193*/0xafffd7, /*194*/0xd7ffd7, /*195*/0xffffd7,
			/*196*/0x0000ff, /*197*/0x5f00ff, /*198*/0x8700ff, /*199*/0xaf00ff, /*200*/0xd700ff, /*201*/0xff00ff, /*202*/0x005fff, /*203*/0x5f5fff, /*204*/0x875fff, /*205*/0xaf5fff, /*206*/0xd75fff, /*207*/0xff5fff,
			/*208*/0x0087ff, /*209*/0x5f87ff, /*210*/0x8787ff, /*211*/0xaf87ff, /*212*/0xd787ff, /*213*/0xff87ff, /*214*/0x00afff, /*215*/0x5fafff, /*216*/0x87afff, /*217*/0xafafff, /*218*/0xd7afff, /*219*/0xffafff,
			/*220*/0x00d7ff, /*221*/0x5fd7ff, /*222*/0x87d7ff, /*223*/0xafd7ff, /*224*/0xd7d7ff, /*225*/0xffd7ff, /*226*/0x00ffff, /*227*/0x5fffff, /*228*/0x87ffff, /*229*/0xafffff, /*230*/0xd7ffff, /*231*/0xffffff,
			/*232*/0x080808, /*233*/0x121212, /*234*/0x1c1c1c, /*235*/0x262626, /*236*/0x303030, /*237*/0x3a3a3a, /*238*/0x444444, /*239*/0x4e4e4e, /*240*/0x585858, /*241*/0x626262, /*242*/0x6c6c6c, /*243*/0x767676,
			/*244*/0x808080, /*245*/0x8a8a8a, /*246*/0x949494, /*247*/0x9e9e9e, /*248*/0xa8a8a8, /*249*/0xb2b2b2, /*250*/0xbcbcbc, /*251*/0xc6c6c6, /*252*/0xd0d0d0, /*253*/0xdadada, /*254*/0xe4e4e4, /*255*/0xeeeeee
		};

		const auto& TextColor = gDisplayParm.getTextColor();   // 30-37,38,39
		const auto& Text256 = gDisplayParm.getText256();       // 38
		const auto& BackColor = gDisplayParm.getBackColor();   // 40-47,48,49
		const auto& Back256 = gDisplayParm.getBack256();       // 48

		if (Text256)
		{
			if (Text256 == clr24b)
			{
				attr.Attributes.Flags |= ConEmu::ColorFlags::Fg24Bit;
				attr.Attributes.ForegroundColor = TextColor&0xFFFFFF;
			}
			else
			{
				if (TextColor > 15)
					attr.Attributes.Flags |= ConEmu::ColorFlags::Fg24Bit;
				attr.Attributes.ForegroundColor = RgbMap[TextColor&0xFF];
			}
		}
		else if (TextColor & 0x8)
		{
			// Comes from CONSOLE_SCREEN_BUFFER_INFO::wAttributes
			attr.Attributes.ForegroundColor |= ClrMap[TextColor&0x7]
				| 0x08;
		}
		else
		{
			attr.Attributes.ForegroundColor |= ClrMap[TextColor&0x7]
				| ((gDisplayParm.getBrightFore() || (gDisplayParm.getBrightOrBold() && !gDisplayParm.getBrightBack())) ? 0x08 : 0);
		}

		if (gDisplayParm.getBrightOrBold() && (Text256 || gDisplayParm.getBrightFore() || gDisplayParm.getBrightBack()))
			attr.Attributes.Flags |= ConEmu::ColorFlags::Bold;
		if (gDisplayParm.getItalic())
			attr.Attributes.Flags |= ConEmu::ColorFlags::Italic;
		if (gDisplayParm.getUnderline())
			attr.Attributes.Flags |= ConEmu::ColorFlags::Underline;
		if (gDisplayParm.getCrossed())
			attr.Attributes.Flags |= ConEmu::ColorFlags::Crossed;
		if (gDisplayParm.getInverse())
			attr.Attributes.Flags |= ConEmu::ColorFlags::Reverse;

		if (Back256)
		{
			if (Back256 == clr24b)
			{
				attr.Attributes.Flags |= ConEmu::ColorFlags::Bg24Bit;
				attr.Attributes.BackgroundColor = BackColor&0xFFFFFF;
			}
			else
			{
				if (BackColor > 15)
					attr.Attributes.Flags |= ConEmu::ColorFlags::Bg24Bit;
				attr.Attributes.BackgroundColor = RgbMap[BackColor&0xFF];
			}
		}
		else if (BackColor & 0x8)
		{
			// Comes from CONSOLE_SCREEN_BUFFER_INFO::wAttributes
			attr.Attributes.BackgroundColor |= ClrMap[BackColor&0x7]
				| 0x8;
		}
		else
		{
			attr.Attributes.BackgroundColor |= ClrMap[BackColor&0x7]
				| (gDisplayParm.getBrightBack() ? 0x8 : 0);
		}


		//SetConsoleTextAttribute(hConsoleOutput, (WORD)wAttrs);
		ExtSetAttributes(&attr);
	}
}


#if defined(DUMP_UNKNOWN_ESCAPES) || defined(DUMP_WRITECONSOLE_LINES)
static MAtomic<int32_t> nWriteCallNo{ 0 };
#endif

int CEAnsi::DumpEscape(LPCWSTR buf, size_t cchLen, DumpEscapeCodes iUnknown)
{
	int result = 0;
#if defined(DUMP_UNKNOWN_ESCAPES) || defined(DUMP_WRITECONSOLE_LINES)
	if (!buf || !cchLen)
	{
		// There are programs which try to write empty strings
		// e.g. clink, perl, ...
		//_ASSERTEX((buf && cchLen) || (gszClinkCmdLine && buf));
	}

	wchar_t szDbg[200];
	size_t nLen = cchLen;

	switch (iUnknown)  // NOLINT(clang-diagnostic-switch-enum)
	{
	case de_Unknown/*1*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Unknown Esc Sequence: ", GetCurrentThreadId());
		break;
	case de_BadUnicode/*2*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Bad Unicode Translation: ", GetCurrentThreadId());
		break;
	case de_Ignored/*3*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Ignored Esc Sequence: ", GetCurrentThreadId());
		break;
	case de_UnkControl/*4*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Unknown Esc Control: ", GetCurrentThreadId());
		break;
	case de_Report/*5*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Back Report: ", GetCurrentThreadId());
		break;
	case de_ScrollRegion/*6*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Scroll region: ", GetCurrentThreadId());
		break;
	case de_Comment/*7*/:
		msprintf(szDbg, countof(szDbg), L"[%u] ###Internal comment: ", GetCurrentThreadId());
		break;
	default:
		result = nWriteCallNo.inc();
		msprintf(szDbg, countof(szDbg), L"[%u] AnsiDump #%u: ", GetCurrentThreadId(), result);
	}

	const size_t nStart = lstrlenW(szDbg);
	wchar_t* pszDst = szDbg + nStart;
	wchar_t* pszFrom = szDbg;

	if (buf && cchLen)
	{
		const wchar_t* pszSrc = static_cast<const wchar_t*>(buf);
		size_t nCur = 0;
		while (nLen)
		{
			switch (*pszSrc)
			{
			case L'\r':
				*(pszDst++) = L'\\'; *(pszDst++) = L'r';
				break;
			case L'\n':
				*(pszDst++) = L'\\'; *(pszDst++) = L'n';
				break;
			case L'\t':
				*(pszDst++) = L'\\'; *(pszDst++) = L't';
				break;
			case L'\x1B':
				*(pszDst++) = szAnalogues[0x1B];
				break;
			case 0:
				*(pszDst++) = L'\\'; *(pszDst++) = L'0';
				break;
			case 7:
				*(pszDst++) = L'\\'; *(pszDst++) = L'a';
				break;
			case 8:
				*(pszDst++) = L'\\'; *(pszDst++) = L'b';
				break;
			case 0x7F:
				*(pszDst++) = '\\'; *(pszDst++) = 'x'; *(pszDst++) = '7'; *(pszDst++) = 'F';
				break;
			case L'\\':
				*(pszDst++) = L'\\'; *(pszDst++) = L'\\';
				break;
			default:
				*(pszDst++) = *pszSrc;
			}
			pszSrc++;
			nLen--;
			nCur++;

			if (nCur >= 80)
			{
				*(pszDst++) = 0xB6; // L'¶';
				*(pszDst++) = L'\n';
				*pszDst = 0;
				// Try to pass UTF-8 encoded strings to debugger
				DebugStringUtf8(szDbg);
				wmemset(szDbg, L' ', nStart);
				nCur = 0;
				pszFrom = pszDst = szDbg + nStart;
			}
		}
	}
	else
	{
		pszDst -= 2;
		const wchar_t* psEmptyMessage = L" - <empty sequence>";
		const size_t nMsgLen = lstrlenW(psEmptyMessage);
		wmemcpy(pszDst, psEmptyMessage, nMsgLen);
		pszDst += nMsgLen;
	}

	if (pszDst > pszFrom)
	{
		*(pszDst++) = 0xB6; // L'¶';
		*(pszDst++) = L'\n';
		*pszDst = 0;
		// Try to pass UTF-8 encoded strings to debugger
		DebugStringUtf8(szDbg);
	}

	if (iUnknown == 1)
	{
		_ASSERTEX(FALSE && "Unknown Esc Sequence!");
	}
#endif
	return result;
}

#ifdef DUMP_UNKNOWN_ESCAPES
#define DumpUnknownEscape(buf, cchLen) DumpEscape(buf, cchLen, de_Unknown)
#define DumpKnownEscape(buf, cchLen, eType) DumpEscape(buf, cchLen, eType)
#else
#define DumpUnknownEscape(buf,cchLen) (0)
#define DumpKnownEscape(buf, cchLen, eType) (0)
#endif

static MAtomic<uint32_t> gnLastReadId{ 0 };

// When user type something in the prompt, screen buffer may be scrolled
// It would be nice to do the same in "ConEmuC -StoreCWD"
void CEAnsi::OnReadConsoleBefore(HANDLE hConOut, const CONSOLE_SCREEN_BUFFER_INFO& csbi)
{
	CEAnsi* pObj = CEAnsi::Object();
	if (!pObj)
		return;

	if (!gnLastReadId.load())
		gnLastReadId.store(GetCurrentProcessId());

	WORD newRowId = 0;
	CEConsoleMark Test = {};

	COORD crPos[] = { {4,static_cast<SHORT>(csbi.dwCursorPosition.Y - 1)}, csbi.dwCursorPosition };
	_ASSERTEX(countof(crPos) == countof(pObj->m_RowMarks.SaveRow) && countof(crPos) == countof(pObj->m_RowMarks.RowId));

	pObj->m_RowMarks.csbi = csbi;

	for (int i = 0; i < 2; i++)
	{
		pObj->m_RowMarks.SaveRow[i] = -1;
		pObj->m_RowMarks.RowId[i] = 0;

		if (crPos[i].X < 4 || crPos[i].Y < 0)
			continue;

		if (ReadConsoleRowId(hConOut, crPos[i].Y, &Test))
		{
			pObj->m_RowMarks.SaveRow[i] = crPos[i].Y;
			pObj->m_RowMarks.RowId[i] = Test.RowId;
		}
		else
		{
			// ReSharper disable once CppJoinDeclarationAndAssignment
			newRowId = LOWORD(gnLastReadId.inc());
			if (!newRowId)
				newRowId = LOWORD(gnLastReadId.inc());

			if (WriteConsoleRowId(hConOut, crPos[i].Y, newRowId))
			{
				pObj->m_RowMarks.SaveRow[i] = crPos[i].Y;
				pObj->m_RowMarks.RowId[i] = newRowId;
			}
		}
	}

	// Successful mark?
	_ASSERTEX(((pObj->m_RowMarks.RowId[0] || pObj->m_RowMarks.RowId[1]) && (pObj->m_RowMarks.RowId[0] != pObj->m_RowMarks.RowId[1])) || (!csbi.dwCursorPosition.X && !csbi.dwCursorPosition.Y));

	// Store info in MAPPING
	CESERVER_CONSOLE_APP_MAPPING* pAppMap = gpAppMap ? gpAppMap->Ptr() : nullptr;
	if (pAppMap)
	{
		pAppMap->csbiPreRead = csbi;
		pAppMap->nPreReadRowID[0] = pObj->m_RowMarks.RowId[0];
		pAppMap->nPreReadRowID[1] = pObj->m_RowMarks.RowId[1];
	}
}
void CEAnsi::OnReadConsoleAfter(bool bFinal, bool bNoLineFeed)
{
	CEAnsi* pObj = CEAnsi::Object();
	if (!pObj)
		return;

	if (pObj->m_RowMarks.SaveRow[0] < 0 && pObj->m_RowMarks.SaveRow[1] < 0)
		return;

	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	SHORT nMarkedRow = -1;
	CEConsoleMark Test = {};

	if (!GetConsoleScreenBufferInfo(hConOut, &csbi))
		goto wrap;

	if (!FindConsoleRowId(hConOut, csbi.dwCursorPosition.Y, true, &nMarkedRow, &Test))
		goto wrap;

	for (int i = 1; i >= 0; i--)
	{
		if ((pObj->m_RowMarks.SaveRow[i] >= 0) && (pObj->m_RowMarks.RowId[i] == Test.RowId))
		{
			if (pObj->m_RowMarks.SaveRow[i] == nMarkedRow)
			{
				_ASSERTEX(
					((pObj->m_RowMarks.csbi.dwCursorPosition.Y < (pObj->m_RowMarks.csbi.dwSize.Y-1))
						|| (bNoLineFeed && (pObj->m_RowMarks.csbi.dwCursorPosition.Y == (pObj->m_RowMarks.csbi.dwSize.Y-1))))
					&& "Nothing was changed? Strange, scrolling was expected");
				goto wrap;
			}
			// Well, we get scroll distance
			_ASSERTEX(nMarkedRow < pObj->m_RowMarks.SaveRow[i]); // Upside scroll expected
			ExtScrollScreenParm scrl = {sizeof(scrl), essf_ExtOnly, hConOut, nMarkedRow - pObj->m_RowMarks.SaveRow[i], {}, 0, {}};
			ExtScrollScreen(&scrl);
			goto wrap;
		}
	}

wrap:
	// Clear it
	for (int i = 0; i < 2; i++)
	{
		pObj->m_RowMarks.SaveRow[i] = -1;
		pObj->m_RowMarks.RowId[i] = 0;
	}
}


BOOL CEAnsi::WriteText(OnWriteConsoleW_t writeConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, BOOL abCommit /*= FALSE*/, EXTREADWRITEFLAGS AddFlags /*= ewtf_None*/)
{
	BOOL lbRc = FALSE;
	DWORD /*nWritten = 0,*/ nTotalWritten = 0;

	#ifdef _DEBUG
	ORIGINAL_KRNL(WriteConsoleW);
	OnWriteConsoleW_t pfnDbgWriteConsoleW = F(WriteConsoleW);
	_ASSERTE(((writeConsoleW == pfnDbgWriteConsoleW) || !HooksWereSet) && "It must point to CallPointer for 'unhooked' call");
	#endif

	if (lpBuffer && nNumberOfCharsToWrite)
		m_LastWrittenChar = lpBuffer[nNumberOfCharsToWrite-1];

	ExtWriteTextParm write{};
	write.StructSize = sizeof(write);
	write.Flags = ewtf_Current | AddFlags;
	write.ConsoleOutput = hConsoleOutput;
	write.Private = reinterpret_cast<void*>(writeConsoleW);

	LPCWSTR pszSrcBuffer = lpBuffer;
	wchar_t cvtBuf[80];
	wchar_t* pCvtBuf = nullptr;
	CEStr buffer;
	if (mCharSet && lpBuffer && nNumberOfCharsToWrite)
	{
		static wchar_t G0_DRAWING[31] = {
			0x2666 /*♦*/, 0x2592 /*▒*/, 0x2192 /*→*/, 0x21A8 /*↨*/, 0x2190 /*←*/, 0x2193 /*↓*/, 0x00B0 /*°*/, 0x00B1 /*±*/,
			0x00B6 /*¶*/, 0x2195 /*↕*/, 0x2518 /*┘*/, 0x2510 /*┐*/, 0x250C /*┌*/, 0x2514 /*└*/, 0x253C /*┼*/, 0x203E /*‾*/,
			0x207B /*⁻*/, 0x2500 /*─*/, 0x208B /*₋*/, 0x005F /*_*/, 0x251C /*├*/, 0x2524 /*┤*/, 0x2534 /*┴*/, 0x252C /*┬*/,
			0x2502 /*│*/, 0x2264 /*≤*/, 0x2265 /*≥*/, 0x03C0 /*π*/, 0x2260 /*≠*/, 0x00A3 /*£*/, 0x00B7 /*·*/
		};
		LPCWSTR pszMap = nullptr;
		// ReSharper disable once CppIncompleteSwitchStatement
		switch (mCharSet)  // NOLINT(clang-diagnostic-switch)
		{
		case VTCS_DRAWING:
			pszMap = G0_DRAWING;
			break;
		}
		if (pszMap)
		{
			wchar_t* dst = nullptr;
			for (DWORD i = 0; i < nNumberOfCharsToWrite; ++i)
			{
				if (pszSrcBuffer[i] >= 0x60 && pszSrcBuffer[i] < 0x7F)
				{
					if (!pCvtBuf)
					{
						if (nNumberOfCharsToWrite <= countof(cvtBuf))
						{
							pCvtBuf = cvtBuf;
						}
						else
						{
							if (!((pCvtBuf = buffer.GetBuffer(nNumberOfCharsToWrite))))
								break;
						}
						lpBuffer = pCvtBuf;
						dst = pCvtBuf;
						if (i)
							memmove(dst, pszSrcBuffer, i * sizeof(*dst));
					}
					dst[i] = pszMap[pszSrcBuffer[i] - 0x60];
				}
				else if (dst)
				{
					dst[i] = pszSrcBuffer[i];
				}
			}
		}
	}

	ReloadFeatures();
	if (mb_SuppressBells)
	{
		write.Flags |= ewtf_NoBells;
	}

	if (gDisplayOpt.WrapWasSet && (gDisplayOpt.WrapAt > 0))
	{
		write.Flags |= ewtf_WrapAt;
		write.WrapAtCol = gDisplayOpt.WrapAt;
	}

	if (gDisplayOpt.ScrollRegion)
	{
		write.Flags |= ewtf_Region;
		_ASSERTEX(gDisplayOpt.ScrollStart >= 0 && gDisplayOpt.ScrollEnd >= gDisplayOpt.ScrollStart);
		write.Region.top = gDisplayOpt.ScrollStart;
		write.Region.bottom = gDisplayOpt.ScrollEnd;
		write.Region.left = write.Region.right = -1; // not used yet
	}

	if (!IsAutoLfNl())
		write.Flags |= ewtf_NoLfNl;

	DWORD nWriteFrom = 0, nWriteTo = nNumberOfCharsToWrite;

	#ifdef GH_1402
	static bool fishLineFeed = false;
	struct ExtWriteTextCalls { const wchar_t* buffer; size_t count; };
	const size_t ext_calls_max = 16;
	static ExtWriteTextCalls ext_calls[ext_calls_max] = {};
	static size_t ext_calls_count = 0;

	auto count_chars = [&](const wchar_t test) -> unsigned {
		unsigned count = 0;
		for (DWORD n = nWriteFrom; n < nWriteTo; ++n)
		{
			if (lpBuffer[n] == test)
				++count;
		}
		return count;
	};
	#endif

	if ((nNumberOfCharsToWrite == 1) && isConsoleBadDBCS())
	{
		if (lpBuffer[0] == ucLineFeed)
		{
			static wchar_t dummy_buf[2] = L"%";
			lpBuffer = dummy_buf;
		}
	}

	while (nWriteTo > nWriteFrom && nWriteFrom < nNumberOfCharsToWrite)
	{
		#ifdef GH_1402
		bool curFishLineFeed = false;
		if (count_chars(ucLineFeed) > 0)
		{
			if (!gbIsXTermOutput)
			{
				_ASSERTE(FALSE && "XTerm mode was not enabled!");
				DBG_XTERM(L"xTermOutput=ON due gh-1402 LineFeed");
				SetIsXTermOutput(true);
			}
			curFishLineFeed = true;
		}
		#endif

		// for debug purposes insert "\x1B]9;10\x1B\" at the beginning of connector-*-out.log
		if (gbIsXTermOutput)
		{
			// On Win10 we may utilize DISABLE_NEWLINE_AUTO_RETURN flag, but it would
			// complicate our code, because ConEmu support older Windows versions
			write.Flags &= ~ewtf_DontWrap;
			// top:  writes all lines using full console width
			//       and thereafter some ANSI-s and "\r\n"
			// fish: writes in one call "(Width-1)*Spaces,\r,Space,\r"
			DWORD ChrSet = (lpBuffer[nWriteFrom] == L'\r' || lpBuffer[nWriteFrom] == L'\n') ? 1 : 2;
			for (DWORD n = nWriteFrom+1; (n < nWriteTo); n++)
			{
				DWORD AddChrSet = (lpBuffer[n] == L'\r' || lpBuffer[n] == L'\n') ? 1 : 2;
				if (ChrSet != AddChrSet)
				{
					// If only printable chars are written
					// ExtWriteText will check (AI) if it must not wrap&scroll
					if (ChrSet == 2)
						write.Flags |= ewtf_DontWrap;
					nWriteTo = n;
					break;
				}
				// Expected to be the same
				_ASSERTE(ChrSet == AddChrSet);
				ChrSet |= AddChrSet;
			}
			// Perhaps, we shall do that always
			if (ChrSet == 2)
				write.Flags |= ewtf_DontWrap;
		}
		_ASSERTE(nWriteTo<=nNumberOfCharsToWrite);

		//lbRc = writeConsoleW(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, &nTotalWritten, nullptr);
		write.Buffer = lpBuffer + nWriteFrom;
		write.NumberOfCharsToWrite = nWriteTo - nWriteFrom;

		#ifdef GH_1402
		// For debugging purposes, we store ext_calls_max latest outputs
		auto& ext_call = ext_calls[(ext_calls_count++) % ext_calls_max];
		ext_call.buffer = write.Buffer;
		ext_call.count = write.NumberOfCharsToWrite;
		// Ensure we don't write more than a one LF character
		if (curFishLineFeed)
		{
			unsigned ulf_chars = count_chars(ucLineFeed);
			_ASSERTE(ulf_chars <= 1);
			unsigned cr_chars = count_chars(L'\r');
			unsigned lf_chars = count_chars(L'\n');
			if (ulf_chars)
				_ASSERTE((ulf_chars > 0) != ((cr_chars+lf_chars) > 0));
			if (fishLineFeed && (count_chars(L' ') > 0))
				_ASSERTE((write.Flags & ewtf_DontWrap) != 0);
			if (ulf_chars > 0)
				fishLineFeed = true;
			if (cr_chars > 0)
				fishLineFeed = false;
		}
		#endif

		lbRc = ExtWriteText(&write);
		if (lbRc)
		{
			if (write.NumberOfCharsWritten)
			{
				nTotalWritten += write.NumberOfCharsWritten;
			}
			if (write.ScrolledRowsUp > 0)
			{
				const int right = static_cast<int>(gDisplayCursor.StoredCursorPos.Y) - static_cast<int>(write.ScrolledRowsUp);
				gDisplayCursor.StoredCursorPos.Y = static_cast<SHORT>(std::max(0, right));
			}
		}

		nWriteFrom = nWriteTo; nWriteTo = nNumberOfCharsToWrite;
	}

	if (lpNumberOfCharsWritten)
		*lpNumberOfCharsWritten = nTotalWritten;

	return lbRc;
}

// NON-static, because we need to ‘cache’ parts of non-translated MBCS chars (one UTF-8 symbol may be transmitted by up to *three* parts)
BOOL CEAnsi::OurWriteConsoleA(HANDLE hConsoleOutput, const char *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	_ASSERTE(this != nullptr);
	BOOL lbRc = FALSE;
	wchar_t* buf = nullptr;
	wchar_t szTemp[280]; // would be enough in most cases
	CEStr ptrTemp;
	INT_PTR bufMax;
	// ReSharper disable once CppJoinDeclarationAndAssignment
	DWORD cp;
	// ReSharper disable once CppJoinDeclarationAndAssignment
	CpCvtResult cvt{};
	const char* pSrc = nullptr;
	const char* pTokenStart = nullptr;
	wchar_t* pDst = nullptr;
	wchar_t* pDstEnd = nullptr;
	DWORD nWritten = 0;
	DWORD nTotalWritten = 0;

	ORIGINAL_KRNL(WriteConsoleA);

	// Nothing to write? Or flush buffer?
	if (!lpBuffer || !nNumberOfCharsToWrite || !hConsoleOutput || (hConsoleOutput == INVALID_HANDLE_VALUE))
	{
		if (lpNumberOfCharsWritten)
			*lpNumberOfCharsWritten = 0;
		lbRc = TRUE;
		goto fin;
	}

	if ((nNumberOfCharsToWrite + 3) >= countof(szTemp))
	{
		bufMax = nNumberOfCharsToWrite + 3;
		buf = ptrTemp.GetBuffer(bufMax);
	}
	else
	{
		buf = szTemp;
		bufMax = countof(szTemp);
	}
	if (!buf)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		goto fin;
	}

	cp = GetCodePage();
	m_Cvt.SetCP(cp);

	lbRc = TRUE;
	pSrc = pTokenStart = lpBuffer;
	pDst = buf; pDstEnd = buf + bufMax - 3;
	for (DWORD n = 0; n < nNumberOfCharsToWrite; n++, pSrc++)
	{
		if (pDst >= pDstEnd)
		{
			_ASSERTE((pDst < (buf+bufMax)) && "wchar_t buffer overflow while converting");
			buf[(pDst - buf)] = 0; // It's not required, just to easify debugging
			lbRc = OurWriteConsoleW(hConsoleOutput, buf, static_cast<DWORD>(pDst - buf), &nWritten, nullptr);
			if (lbRc) nTotalWritten += nWritten;
			pDst = buf;
		}
		cvt = m_Cvt.Convert(*pSrc, *pDst);
		switch (cvt)  // NOLINT(clang-diagnostic-switch-enum)
		{
		case ccr_OK:
		case ccr_BadUnicode:
			pDst++;
			break;
		case ccr_Surrogate:
		case ccr_BadTail:
		case ccr_DoubleBad:
			m_Cvt.GetTail(*(++pDst));
			pDst++;
			break;
		default:
			break;
		}
	}

	if (pDst > buf)
	{
		_ASSERTE((pDst < (buf+bufMax)) && "wchar_t buffer overflow while converting");
		buf[(pDst - buf)] = 0; // It's not required, just to easify debugging
		lbRc = OurWriteConsoleW(hConsoleOutput, buf, static_cast<DWORD>(pDst - buf), &nWritten, nullptr);
		if (lbRc)
			nTotalWritten += nWritten;
	}

	// Issue 1291:	Python fails to print string sequence with ASCII character followed by Chinese character.
	if (lpNumberOfCharsWritten && lbRc)
	{
		*lpNumberOfCharsWritten = nNumberOfCharsToWrite;
	}

fin:
	std::ignore = pTokenStart;
	std::ignore = nTotalWritten;
	return lbRc;
}

BOOL CEAnsi::OurWriteConsoleW(HANDLE hConsoleOutput, const VOID *lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten, LPVOID lpReserved, bool bInternal /*= false*/)
{
	ORIGINAL_KRNL(WriteConsoleW);
	BOOL lbRc = FALSE;
	//ExtWriteTextParm wrt = {sizeof(wrt), ewtf_None, hConsoleOutput};
	bool bIsConOut = false;
	bool bIsAnsi = false;

	FIRST_ANSI_CALL(static_cast<const BYTE*>(lpBuffer), nNumberOfCharsToWrite);

#if 0
	// Store prompt(?) for clink 0.1.1
	if ((gnAllowClinkUsage == 1) && nNumberOfCharsToWrite && lpBuffer && gpszLastWriteConsole && gcchLastWriteConsoleMax)
	{
		size_t cchMax = std::min(gcchLastWriteConsoleMax-1,nNumberOfCharsToWrite);
		gpszLastWriteConsole[cchMax] = 0;
		wmemmove(gpszLastWriteConsole, (const wchar_t*)lpBuffer, cchMax);
	}
#endif

	// In debug builds: Write to debug console all console Output
	const auto ansiIndex = DumpKnownEscape(static_cast<const wchar_t*>(lpBuffer), nNumberOfCharsToWrite, de_Normal);

#ifdef _DEBUG
	struct AnsiDuration  // NOLINT(cppcoreguidelines-special-member-functions)
	{
		const int ansiIndex_;
		const std::chrono::steady_clock::time_point startTime_;

		AnsiDuration(const int ansiIndex)
			: ansiIndex_(ansiIndex), startTime_(std::chrono::steady_clock::now())
		{
		}

		~AnsiDuration()
		{
			const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime_);
			wchar_t info[80] = L"";
			msprintf(info, countof(info), L"[%u] AnsiDump #%u duration(ms): %u\n", GetCurrentThreadId(), ansiIndex_, duration.count());
			OutputDebugStringW(info);
		}
	};
	AnsiDuration duration(ansiIndex);
#endif

	CEAnsi* pObj = nullptr;
	CEStr cpCvtBuffer;

	if (lpBuffer && nNumberOfCharsToWrite && hConsoleOutput)
	{
		bIsAnsi = HandleKeeper::IsAnsiCapable(hConsoleOutput, &bIsConOut);

		if (ghAnsiLogFile && bIsConOut)
		{
			CEAnsi::WriteAnsiLogW(static_cast<LPCWSTR>(lpBuffer), nNumberOfCharsToWrite);
		}
	}

	if (lpBuffer && nNumberOfCharsToWrite && hConsoleOutput && bIsAnsi)
	{
		// if that was API call of WriteConsoleW
		if (!bInternal && gCpConv.nFromCP && gCpConv.nToCP)
		{
			// Convert from unicode to MBCS
			CEStrA pszTemp;
			int iMbcsLen = WideCharToMultiByte(gCpConv.nFromCP, 0, static_cast<LPCWSTR>(lpBuffer), nNumberOfCharsToWrite, nullptr, 0, nullptr, nullptr);
			if ((iMbcsLen > 0) && ((pszTemp.GetBuffer(iMbcsLen)) != nullptr))
			{
				BOOL bFailed = FALSE; // Do not do conversion if some chars can't be mapped
				iMbcsLen = WideCharToMultiByte(gCpConv.nFromCP, 0, static_cast<LPCWSTR>(lpBuffer), nNumberOfCharsToWrite, pszTemp.data(), iMbcsLen, nullptr, &bFailed);
				if ((iMbcsLen > 0) && !bFailed)
				{
					int iWideLen = MultiByteToWideChar(gCpConv.nToCP, 0, pszTemp, iMbcsLen, nullptr, 0);
					if (iWideLen > 0)
					{
						wchar_t* ptrBuf = cpCvtBuffer.GetBuffer(iWideLen);
						if (ptrBuf)
						{
							iWideLen = MultiByteToWideChar(gCpConv.nToCP, 0, pszTemp, iMbcsLen, ptrBuf, iWideLen);
							if (iWideLen > 0)
							{
								lpBuffer = ptrBuf;
								nNumberOfCharsToWrite = iWideLen;
							}
						}
					}
				}
			}
		}

		pObj = CEAnsi::Object();
		if (pObj)
		{
			if (pObj->gnPrevAnsiPart || gDisplayOpt.WrapWasSet)
			{
				// Если остался "хвост" от предущей записи - сразу, без проверок
				lbRc = pObj->WriteAnsiCodes(F(WriteConsoleW), hConsoleOutput, static_cast<const wchar_t*>(lpBuffer), nNumberOfCharsToWrite, lpNumberOfCharsWritten);
				goto ansidone;
			}
			else
			{
				_ASSERTEX(ESC==27 && BEL==7 && DSC==0x90);
				const wchar_t* pch = static_cast<const wchar_t*>(lpBuffer);
				for (size_t i = nNumberOfCharsToWrite; i--; pch++)
				{
					// Если в выводимой строке встречается "Ansi ESC Code" - выводим сами
					TODO("Non-CSI codes, like as BEL, BS, CR, LF, FF, TAB, VT, SO, SI");
					if (*pch == ESC /*|| *pch == BEL*/ /*|| *pch == ENQ*/)
					{
						lbRc = pObj->WriteAnsiCodes(F(WriteConsoleW), hConsoleOutput, static_cast<const wchar_t*>(lpBuffer), nNumberOfCharsToWrite, lpNumberOfCharsWritten);
						goto ansidone;
					}
				}
			}
		}
	}

	if (!bIsAnsi || ((pObj = CEAnsi::Object()) == nullptr))
	{
		lbRc = F(WriteConsoleW)(hConsoleOutput, lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten, lpReserved);
	}
	else
	{
		lbRc = pObj->WriteText(F(WriteConsoleW), hConsoleOutput, static_cast<LPCWSTR>(lpBuffer), nNumberOfCharsToWrite, lpNumberOfCharsWritten, TRUE);
		//wrt.Flags = ewtf_Current|ewtf_Commit;
		//wrt.Buffer = (const wchar_t*)lpBuffer;
		//wrt.NumberOfCharsToWrite = nNumberOfCharsToWrite;
		//wrt.Private = F(WriteConsoleW);
		//lbRc = ExtWriteText(&wrt);
		//if (lbRc)
		//{
		//	if (lpNumberOfCharsWritten)
		//		*lpNumberOfCharsWritten = wrt.NumberOfCharsWritten;
		//	if (wrt.ScrolledRowsUp > 0)
		//		gDisplayCursor.StoredCursorPos.Y = std::max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)wrt.ScrolledRowsUp));
		//}
	}
	goto wrap;

ansidone:
	{
		ExtCommitParm cmt = {sizeof(cmt), hConsoleOutput};
		ExtCommit(&cmt);
	}
wrap:
	return lbRc;
}

//struct AnsiEscCode
//{
//	wchar_t  First;  // ESC (27)
//	wchar_t  Second; // any of 64 to 95 ('@' to '_')
//	wchar_t  Action; // any of 64 to 126 (@ to ~). this is terminator
//	wchar_t  Skip;   // Если !=0 - то эту последовательность нужно пропустить
//	int      ArgC;
//	int      ArgV[16];
//	LPCWSTR  ArgSZ; // Reserved for key mapping
//	size_t   cchArgSZ;
//
//#ifdef _DEBUG
//	LPCWSTR  pszEscStart;
//	size_t   nTotalLen;
//#endif
//
//	int      PvtLen;
//	wchar_t  Pvt[16];
//};


// 0 - нет (в lpBuffer только текст)
// 1 - в Code помещена Esc последовательность (может быть простой текст ДО нее)
// 2 - нет, но кусок последовательности сохранен в gsPrevAnsiPart
int CEAnsi::NextEscCode(LPCWSTR lpBuffer, LPCWSTR lpEnd, wchar_t (&szPreDump)[CEAnsi_MaxPrevPart], DWORD& cchPrevPart, LPCWSTR& lpStart, LPCWSTR& lpNext, CEAnsi::AnsiEscCode& Code, BOOL ReEntrance /*= FALSE*/)
{
	int iRc = 0;
	wchar_t wc;

	LPCWSTR lpSaveStart = lpBuffer;
	lpStart = lpBuffer;

	_ASSERTEX(cchPrevPart==0);

	if (gnPrevAnsiPart && !ReEntrance)
	{
		if (*gsPrevAnsiPart == 27)
		{
			_ASSERTEX(gnPrevAnsiPart < 79);
			const INT_PTR nCurPrevLen = gnPrevAnsiPart;
			const INT_PTR nAdd = std::min((lpEnd-lpBuffer),static_cast<INT_PTR>(countof(gsPrevAnsiPart))-nCurPrevLen-1);
			// Need to check buffer overflow!!!
			_ASSERTEX(static_cast<INT_PTR>(countof(gsPrevAnsiPart)) > (nCurPrevLen + nAdd));
			wmemcpy(gsPrevAnsiPart+nCurPrevLen, lpBuffer, nAdd);
			gsPrevAnsiPart[nCurPrevLen+nAdd] = 0;

			WARNING("Проверить!!!");
			LPCWSTR lpReStart, lpReNext;
			const int iCall = NextEscCode(gsPrevAnsiPart, gsPrevAnsiPart + nAdd + gnPrevAnsiPart, szPreDump, cchPrevPart, lpReStart, lpReNext, Code, TRUE);
			if (iCall == 1)
			{
				if ((lpReNext - gsPrevAnsiPart) >= gnPrevAnsiPart)
				{
					// Bypass unrecognized ESC sequences to screen?
					if (lpReStart > gsPrevAnsiPart)
					{
						const INT_PTR nSkipLen = (lpReStart - gsPrevAnsiPart); //DWORD nWritten;
						_ASSERTEX(nSkipLen > 0 && nSkipLen <= static_cast<INT_PTR>(countof(gsPrevAnsiPart)) && nSkipLen <= gnPrevAnsiPart);
						DumpUnknownEscape(gsPrevAnsiPart, nSkipLen);

						//WriteText(writeConsoleW, hConsoleOutput, gsPrevAnsiPart, nSkipLen, &nWritten);
						_ASSERTEX(nSkipLen <= (static_cast<int>(CEAnsi_MaxPrevPart) - static_cast<int>(cchPrevPart)));
						memmove(szPreDump, gsPrevAnsiPart, nSkipLen);
						cchPrevPart += static_cast<int>(nSkipLen);

						if (nSkipLen < gnPrevAnsiPart)
						{
							memmove(gsPrevAnsiPart, lpReStart, (gnPrevAnsiPart - nSkipLen)*sizeof(*gsPrevAnsiPart));
							gnPrevAnsiPart -= nSkipLen;
						}
						else
						{
							_ASSERTEX(nSkipLen == gnPrevAnsiPart);
							*gsPrevAnsiPart = 0;
							gnPrevAnsiPart = 0;
						}
						lpReStart = gsPrevAnsiPart;
					}
					_ASSERTEX(lpReStart == gsPrevAnsiPart);
					lpStart = lpBuffer; // nothing to dump before Esc-sequence
					_ASSERTEX((lpReNext - gsPrevAnsiPart) >= gnPrevAnsiPart);
					WARNING("Проверить!!!");
					lpNext = lpBuffer + (lpReNext - gsPrevAnsiPart - gnPrevAnsiPart);
				}
				else
				{
					_ASSERTEX((lpReNext - gsPrevAnsiPart) >= gnPrevAnsiPart);
					lpStart = lpNext = lpBuffer;
				}
				gnPrevAnsiPart = 0;
				gsPrevAnsiPart[0] = 0;
				iRc = 1;
				goto wrap2;
			}
			else if (iCall == 2)
			{
				gnPrevAnsiPart = nCurPrevLen+nAdd;
				_ASSERTEX(gsPrevAnsiPart[nCurPrevLen+nAdd] == 0);
				iRc = 2;
				goto wrap;
			}

			_ASSERTEX((iCall == 1) && "Invalid esc sequence, need dump to screen?");
		}
		else
		{
			_ASSERTEX(*gsPrevAnsiPart == 27);
		}
	}


	while (lpBuffer < lpEnd)
	{
		switch (*lpBuffer)
		{
		case 27:
			{
				INT_PTR nLeft;
				// ReSharper disable once CppLocalVariableMayBeConst
				LPCWSTR lpEscStart = lpBuffer;

				#ifdef _DEBUG
				Code.pszEscStart = lpBuffer;
				Code.nTotalLen = 0;
				#endif

				// Special one char codes? Like "ESC 7" and so on...
				if ((lpBuffer + 1) < lpEnd)
				{
					// But it may be some "special" codes
					switch (lpBuffer[1])
					{
					case L'7': // Save xterm cursor
					case L'8': // Restore xterm cursor
					case L'c': // Full reset
					case L'g': // Visual Bell
					case L'=':
					case L'>':
					case L'H': // Horizontal Tab Set
					case L'M': // Reverse LF
					case L'E': // CR-LF
					case L'D': // LF
						// xterm?
						lpStart = lpEscStart;
						Code.First = 27;
						Code.Second = *(++lpBuffer);
						Code.ArgC = 0;
						Code.PvtLen = 0;
						Code.Pvt[0] = 0;
						lpEnd = (++lpBuffer);
						iRc = 1;
						goto wrap;
					}
				}

				// If tail is larger than 2 chars, continue
				if ((lpBuffer + 2) < lpEnd)
				{
					// Set lpSaveStart to current start of Esc sequence, it was set to beginning of buffer
					_ASSERTEX(lpSaveStart <= lpBuffer);
					lpSaveStart = lpBuffer;
					_ASSERTEX(lpSaveStart == lpEscStart);

					Code.First = 27;
					Code.Second = *(++lpBuffer);
					Code.ArgC = 0;
					Code.PvtLen = 0;
					Code.Pvt[0] = 0;

					TODO("Bypass unrecognized ESC sequences to screen? Don't try to eliminate 'Possible' sequences?");
					//if (((Code.Second < 64) || (Code.Second > 95)) && (Code.Second != 124/* '|' - vim-xterm-emulation */))
					if (!wcschr(L"[]|()%", Code.Second))
					{
						// Don't assert on rawdump of KeyEvents.exe Esc key presses
						// 10:00:00 KEY_EVENT_RECORD: Dn, 1, Vk="VK_ESCAPE" [27/0x001B], Scan=0x0001 uChar=[U='\x1b' (0x001B): A='\x1b' (0x1B)]
						const bool bStandaloneEscChar = (lpStart < lpSaveStart) && ((*(lpSaveStart - 1) == L'\'' && Code.Second == L'\'') || (*(lpSaveStart - 1) == L' ' && Code.Second == L' '));
						//_ASSERTEX(bStandaloneEscChar && "Unsupported control sequence?");
						if (!bStandaloneEscChar)
						{
							DumpKnownEscape(Code.pszEscStart, std::min<size_t>(Code.nTotalLen, 32), de_UnkControl);
						}
						continue; // invalid code
					}

					// Now parameters go
					++lpBuffer; // move pointer to the first char beyond CSI (after '[')

					auto parseNumArgs = [&Code](const wchar_t* &lpBufferParam, const wchar_t* lpSeqEnd, bool saveAction) -> bool
					{
						wchar_t wcSave;
						int nValue = 0, nDigits = 0;
						Code.ArgC = 0;

						while (lpBufferParam < lpSeqEnd)
						{
							switch (*lpBufferParam)
							{
							case L'0': case L'1': case L'2': case L'3': case L'4':
							case L'5': case L'6': case L'7': case L'8': case L'9':
								nValue = (nValue * 10) + (static_cast<int>(*lpBufferParam) - L'0');
								++nDigits;
								break;

							case L';':
								// Even if there were no digits - default is "0"
								if (Code.ArgC < static_cast<int>(countof(Code.ArgV)))
									Code.ArgV[Code.ArgC++] = nValue; // save argument
								nDigits = nValue = 0;
								break;

							default:
								if (Code.Second == L']')
								{
									// OSC specific, stop on first non-digit/non-semicolon
									if (nDigits && (Code.ArgC < static_cast<int>(countof(Code.ArgV))))
										Code.ArgV[Code.ArgC++] = nValue;
									return (Code.ArgC > 0);
								}
								if (((wcSave = *lpBufferParam) >= 64) && (wcSave <= 126))
								{
									// Fin
									if (saveAction)
										Code.Action = wcSave;
									if (nDigits && (Code.ArgC < static_cast<int>(countof(Code.ArgV))))
										Code.ArgV[Code.ArgC++] = nValue;
									return true;
								}
								if ((static_cast<size_t>(Code.PvtLen) + 2) < countof(Code.Pvt))
								{
									Code.Pvt[Code.PvtLen++] = wcSave; // Skip private symbols
									Code.Pvt[Code.PvtLen] = 0;
								}
							}
							++lpBufferParam;
						}

						if (nDigits && (Code.ArgC < static_cast<int>(countof(Code.ArgV))))
							Code.ArgV[Code.ArgC++] = nValue;
						return (Code.Second == L']');
					};

					switch (Code.Second)
					{
					case L'(':
					case L')':
					case L'%':
					//case L'#':
					//case L'*':
					//case L'+':
					//case L'-':
					//case L'.':
					//case L'/':
						// VT G0/G1/G2/G3 character sets
						lpStart = lpSaveStart;
						Code.Action = *(lpBuffer++);
						Code.Skip = 0;
						Code.ArgSZ = nullptr;
						Code.cchArgSZ = 0;
						lpEnd = lpBuffer;
						iRc = 1;
						goto wrap;
					case L'|':
						// vim-xterm-emulation
					case L'[':
						// Standard
						Code.Skip = 0;
						Code.ArgSZ = nullptr;
						Code.cchArgSZ = 0;
						{
							#ifdef _DEBUG
							// ReSharper disable once CppDeclaratorNeverUsed
							LPCWSTR pszSaveStart = lpBuffer;
							#endif

							if (parseNumArgs(lpBuffer, lpEnd, true))
							{
								lpStart = lpSaveStart;
								lpEnd = lpBuffer+1;
								iRc = 1;
								goto wrap;
							}
						}
						// В данном запросе (на запись) конца последовательности нет,
						// оставшийся хвост нужно сохранить в буфере, для следующего запроса
						// Ниже
						break;

					case L']':
						// Finalizing (ST) with "\x1B\\" or "\x07"
						// "%]4;16;rgb:00/00/00%\" - "%" is ESC
						// "%]0;this is the window titleBEL"
						// ESC ] 0 ; txt ST        Set icon name and window title to txt.
						// ESC ] 1 ; txt ST        Set icon name to txt.
						// ESC ] 2 ; txt ST        Set window title to txt.
						// ESC ] 4 ; num; txt ST   Set ANSI color num to txt.
						// ESC ] 10 ; txt ST       Set dynamic text color to txt.
						// ESC ] 4 6 ; name ST     Change log file to name (normally disabled
						//					       by a compile-time option)
						// ESC ] 5 0 ; fn ST       Set font to fn.
						//Following 2 codes - from linux terminal
						// ESC ] P nrrggbb         Set palette, with parameter given in 7
                        //                         hexadecimal digits after the final P :-(.
						//                         Here n is the color (0-15), and rrggbb indicates
						//                         the red/green/blue values (0-255).
						// ESC ] R                 reset palette

						// ConEmu specific
						// ESC ] 9 ; 1 ; ms ST           Sleep. ms - milliseconds
						// ESC ] 9 ; 2 ; "txt" ST        Show GUI MessageBox ( txt ) for dubug purposes
						// ESC ] 9 ; 3 ; "txt" ST        Set TAB text
						// ESC ] 9 ; 4 ; st ; pr ST      When _st_ is 0: remove progress. When _st_ is 1: set progress value to _pr_ (number, 0-100). When _st_ is 2: set error state in progress on Windows 7 taskbar
						// ESC ] 9 ; 5 ST                Wait for ENTER/SPACE/ESC. Set EnvVar "ConEmuWaitKey" to ENTER/SPACE/ESC on exit.
						// ESC ] 9 ; 6 ; "txt" ST        Execute GuiMacro. Set EnvVar "ConEmuMacroResult" on exit.
						// and others... look at CEAnsi::WriteAnsiCode_OSC

						Code.ArgSZ = lpBuffer;
						Code.cchArgSZ = 0;
						//Code.Skip = Code.Second;

						while (lpBuffer < lpEnd)
						{
							if ((lpBuffer[0] == 7) ||
								(lpBuffer[0] == 27) /* we'll check the proper terminator below */)
							{
								Code.Action = *Code.ArgSZ; // первый символ последовательности
								Code.cchArgSZ = (lpBuffer - Code.ArgSZ);
								lpStart = lpSaveStart;
								const wchar_t* lpBufferPtr = Code.ArgSZ;
								if (lpBuffer[0] == 27)
								{
									if ((lpBuffer + 1) >= lpEnd)
									{
										// Sequence is not complete yet!
										break;
									}
									else if (lpBuffer[1] == L'\\')
									{
										lpEnd = lpBuffer + 2;
									}
									else
									{
										lpEnd = lpBuffer - 1;
										_ASSERTE(*(lpEnd+1) == 27);
										DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
										iRc = 0;
										goto wrap;
									}
								}
								else
								{
									lpEnd = lpBuffer + 1;
								}
								parseNumArgs(lpBufferPtr, lpBuffer, false);
								iRc = 1;
								goto wrap;
							}
							++lpBuffer;
						}
						// Sequence is not complete, we have to store it to concatenate
						// and check on future write call. Below.
						break;

					default:
						// Unknown sequence, use common termination rules
						Code.Skip = Code.Second;
						Code.ArgSZ = lpBuffer;
						Code.cchArgSZ = 0;
						while (lpBuffer < lpEnd)
						{
							// Terminator ASCII symbol: from `@` to `~`
							if (((wc = *lpBuffer) >= 64) && (wc <= 126))
							{
								Code.Action = wc;
								lpStart = lpSaveStart;
								lpEnd = lpBuffer+1;
								iRc = 1;
								goto wrap;
							}
							++lpBuffer;
						}

					} // end of "switch (Code.Second)"
				} // end of minimal length check

				if ((nLeft = (lpEnd - lpEscStart)) <= CEAnsi_MaxPrevAnsiPart)
				{
					if (ReEntrance)
					{
						//_ASSERTEX(!ReEntrance && "Need to be checked!"); -- seems to be OK

						// gsPrevAnsiPart2 stored for debug purposes only (fully excess)
						wmemmove(gsPrevAnsiPart2, lpEscStart, nLeft);
						gsPrevAnsiPart2[nLeft] = 0;
						gnPrevAnsiPart2 = nLeft;
					}
					else
					{
						wmemmove(gsPrevAnsiPart, lpEscStart, nLeft);
						gsPrevAnsiPart[nLeft] = 0;
						gnPrevAnsiPart = nLeft;
					}
				}
				else
				{
					_ASSERTEX(FALSE && "Too long Esc-sequence part, Need to be checked!");
				}

				lpStart = lpEscStart;

				iRc = 2;
				goto wrap;
			} // end of "case 27:"
		} // end of "switch (*lpBuffer)"

		++lpBuffer;
	} // end of "while (lpBuffer < lpEnd)"

wrap:
	lpNext = lpEnd;

	#ifdef _DEBUG
	if (iRc == 1)
		Code.nTotalLen = (lpEnd - Code.pszEscStart);
	#endif
wrap2:
	_ASSERTEX((iRc==0) || (lpStart>=lpSaveStart && lpStart<lpEnd));
	return iRc;
}

// From the cursor position!
BOOL CEAnsi::ScrollLine(HANDLE hConsoleOutput, int nDir)
{
	ExtScrollScreenParm scroll = {sizeof(scroll), essf_Current|essf_Commit, hConsoleOutput, nDir, {}, L' ', {}};
	const BOOL lbRc = ExtScrollLine(&scroll);
	return lbRc;
}

BOOL CEAnsi::ScrollScreen(HANDLE hConsoleOutput, const int nDir) const
{
	auto srView = GetWorkingRegion(hConsoleOutput, true);

	if (gDisplayOpt.ScrollRegion)
	{
		_ASSERTEX(gDisplayOpt.ScrollStart >= 0 && gDisplayOpt.ScrollEnd >= gDisplayOpt.ScrollStart);
		srView.Top = gDisplayOpt.ScrollStart;
		srView.Bottom = gDisplayOpt.ScrollEnd;
	}

	return ScrollScreen(hConsoleOutput, nDir, false , srView);
}

BOOL CEAnsi::ScrollScreen(HANDLE hConsoleOutput, const int nDir, bool global, const SMALL_RECT& scrollRect) const
{
	ExtScrollScreenParm scroll = {
		sizeof(scroll),
		essf_Current | essf_Commit | essf_Region | (global ? essf_Global : essf_None),
		hConsoleOutput, nDir, {}, L' ',
		RECT{ scrollRect.Left, scrollRect.Top, scrollRect.Right, scrollRect.Bottom } };

	const BOOL lbRc = ExtScrollScreen(&scroll);

	return lbRc;
}

BOOL CEAnsi::FullReset(HANDLE hConsoleOutput) const
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		return FALSE;

	ReSetDisplayParm(hConsoleOutput, TRUE, TRUE);

	// Easy way to drop all lines
	ScrollScreen(hConsoleOutput, -csbi.dwSize.Y);

	// Reset cursor
	SetConsoleCursorPosition(hConsoleOutput, {});

	//TODO? Saved cursor position?

	return TRUE;
}

BOOL CEAnsi::ForwardLF(HANDLE hConsoleOutput, BOOL& bApply)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		return FALSE;

	if (bApply)
	{
		ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
		bApply = FALSE;
	}

	if (csbi.dwCursorPosition.Y == (csbi.dwSize.Y - 1))
	{
		WriteText(pfnWriteConsoleW, hConsoleOutput, L"\n", 1, nullptr);
		SetConsoleCursorPosition(hConsoleOutput, csbi.dwCursorPosition);
	}
	else if (csbi.dwCursorPosition.Y < (csbi.dwSize.Y - 1))
	{
		const COORD cr = {csbi.dwCursorPosition.X, static_cast<SHORT>(csbi.dwCursorPosition.Y + 1)};
		SetConsoleCursorPosition(hConsoleOutput, cr);
		if (cr.Y > csbi.srWindow.Bottom)
		{
			SMALL_RECT rcNew = csbi.srWindow;
			rcNew.Bottom = cr.Y;
			rcNew.Top = cr.Y - (csbi.srWindow.Bottom - csbi.srWindow.Top);
			_ASSERTE(rcNew.Top >= 0);
			SetConsoleWindowInfo(hConsoleOutput, TRUE, &rcNew);
		}
	}
	else
	{
		_ASSERTE(csbi.dwCursorPosition.Y > 0);
	}

	return TRUE;
}

BOOL CEAnsi::ReverseLF(HANDLE hConsoleOutput, BOOL& bApply) const
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		return FALSE;

	if (bApply)
	{
		ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
		bApply = FALSE;
	}

	if ((csbi.dwCursorPosition.Y == csbi.srWindow.Top)
		|| (gDisplayOpt.ScrollRegion && csbi.dwCursorPosition.Y == gDisplayOpt.ScrollStart))
	{
		LinesInsert(hConsoleOutput, 1);
	}
	else if (csbi.dwCursorPosition.Y > 0)
	{
		const COORD cr = {csbi.dwCursorPosition.X, static_cast<SHORT>(csbi.dwCursorPosition.Y - 1)};
		SetConsoleCursorPosition(hConsoleOutput, cr);
	}
	else
	{
		_ASSERTE(csbi.dwCursorPosition.Y > 0);
	}

	return TRUE;
}

BOOL CEAnsi::LinesInsert(HANDLE hConsoleOutput, const unsigned linesCount) const
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
	{
		_ASSERTEX(FALSE && "GetConsoleScreenBufferInfoCached failed");
		return FALSE;
	}

	// Apply default color before scrolling!
	ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);

	if (static_cast<int>(linesCount) <= 0)
	{
		_ASSERTEX(static_cast<int>(linesCount) >= 0);
		return FALSE;
	}

	BOOL lbRc = FALSE;

	int TopLine, BottomLine;
	if (gDisplayOpt.ScrollRegion)
	{
		_ASSERTEX(gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		if (csbi.dwCursorPosition.Y < gDisplayOpt.ScrollStart || csbi.dwCursorPosition.Y > gDisplayOpt.ScrollEnd)
			return TRUE;
		TopLine = csbi.dwCursorPosition.Y;
		BottomLine = std::max<int>(gDisplayOpt.ScrollEnd, 0);

		if (static_cast<int>(linesCount) <= (BottomLine - TopLine))
		{
			ExtScrollScreenParm scroll = {
				sizeof(scroll), essf_Current|essf_Commit|essf_Region, hConsoleOutput,
				static_cast<int>(linesCount), {}, L' ',
				// region to be scrolled (that is not a clipping region)
				{0, TopLine, csbi.dwSize.X - 1, BottomLine}};
			lbRc |= ExtScrollScreen(&scroll);
		}
		else
		{
			ExtFillOutputParm fill = {
				sizeof(fill), efof_Attribute|efof_Character, hConsoleOutput,
				{}, L' ', {0, MakeShort(TopLine)}, csbi.dwSize.X * linesCount};
			lbRc |= ExtFillOutput(&fill);
		}
	}
	else
	{
		// What we need to scroll? Buffer or visible rect?
		TopLine = csbi.dwCursorPosition.Y;
		BottomLine = (csbi.dwCursorPosition.Y <= csbi.srWindow.Bottom)
			? csbi.srWindow.Bottom
			: csbi.dwSize.Y - 1;

		ExtScrollScreenParm scroll = {
			sizeof(scroll), essf_Current|essf_Commit|essf_Region, hConsoleOutput,
			static_cast<int>(linesCount), {}, L' ', {0, TopLine, csbi.dwSize.X-1, BottomLine}};
		lbRc |= ExtScrollScreen(&scroll);
	}

	return lbRc;
}

BOOL CEAnsi::LinesDelete(HANDLE hConsoleOutput, const unsigned linesCount)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (!GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
	{
		_ASSERTEX(FALSE && "GetConsoleScreenBufferInfoCached failed");
		return FALSE;
	}

	// Apply default color before scrolling!
	ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);

	BOOL lbRc = FALSE;

	int TopLine, BottomLine;
	if (gDisplayOpt.ScrollRegion)
	{
		_ASSERTEX(gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>gDisplayOpt.ScrollStart);
		// ScrollStart & ScrollEnd are 0-based absolute line indexes
		// relative to VISIBLE area, these are not absolute buffer coords
		if (((csbi.dwCursorPosition.Y + static_cast<int>(linesCount)) <= gDisplayOpt.ScrollStart)
			|| (csbi.dwCursorPosition.Y > gDisplayOpt.ScrollEnd))
		{
			return TRUE; // Nothing to scroll
		}
		TopLine = csbi.dwCursorPosition.Y;
		BottomLine = gDisplayOpt.ScrollEnd;

		const int negateLinesCount = -static_cast<int>(linesCount);
		ExtScrollScreenParm scrl = {
			sizeof(scrl), essf_Current | essf_Commit | essf_Region, hConsoleOutput,
			negateLinesCount, {}, L' ', {0, TopLine, csbi.dwSize.X - 1, BottomLine} };
		if (scrl.Region.top < gDisplayOpt.ScrollStart)
		{
			scrl.Region.top = gDisplayOpt.ScrollStart;
			scrl.Dir += (gDisplayOpt.ScrollStart - TopLine);
		}
		if ((scrl.Dir < 0) && (scrl.Region.top <= scrl.Region.bottom))
		{
			lbRc |= ExtScrollScreen(&scrl);
		}
	}
	else
	{
		// What we need to scroll? Buffer or visible rect?
		TopLine = csbi.dwCursorPosition.Y;
		BottomLine = (csbi.dwCursorPosition.Y <= csbi.srWindow.Bottom)
			? csbi.srWindow.Bottom
			: csbi.dwSize.Y - 1;

		if (BottomLine < TopLine)
		{
			_ASSERTEX(FALSE && "Invalid (empty) scroll region");
			return FALSE;
		}

		const int negateLinesCount = -static_cast<int>(linesCount);
		ExtScrollScreenParm scrl = {
			sizeof(scrl), essf_Current | essf_Commit | essf_Region, hConsoleOutput,
			negateLinesCount, {}, L' ', {0, TopLine, csbi.dwSize.X - 1, BottomLine} };
		lbRc |= ExtScrollScreen(&scrl);
	}

	return lbRc;
}

int CEAnsi::NextNumber(LPCWSTR& asMS)
{
	wchar_t wc;
	int ms = 0;
	while (((wc = *(asMS++)) >= L'0') && (wc <= L'9'))
		ms = (ms * 10) + static_cast<int>(wc - L'0');
	return ms;
}

// ESC ] 9 ; 1 ; ms ST           Sleep. ms - milliseconds
// ReSharper disable once CppMemberFunctionMayBeStatic
void CEAnsi::DoSleep(LPCWSTR asMS)
{
	int ms = NextNumber(asMS);
	if (!ms)
		ms = 100;
	else if (ms > 10000)
		ms = 10000;
	// Delay
	Sleep(ms);
}

void CEAnsi::EscCopyCtrlString(wchar_t* pszDst, LPCWSTR asMsg, INT_PTR cchMaxLen) const
{
	if (!pszDst)
	{
		_ASSERTEX(pszDst!=nullptr);
		return;
	}

	if (cchMaxLen < 0)
	{
		_ASSERTEX(cchMaxLen >= 0);
		cchMaxLen = 0;
	}
	if (cchMaxLen > 1)
	{
		if ((asMsg[0] == L'"') && (asMsg[cchMaxLen-1] == L'"'))
		{
			asMsg++;
			cchMaxLen -= 2;
		}
	}

	if (cchMaxLen > 0)
		wmemmove(pszDst, asMsg, cchMaxLen);
	pszDst[std::max<ssize_t>(cchMaxLen, 0)] = 0;
}

// ESC ] 9 ; 2 ; "txt" ST          Show GUI MessageBox ( txt ) for dubug purposes
void CEAnsi::DoMessage(LPCWSTR asMsg, INT_PTR cchLen) const
{
	CEStr pszText;

	if (pszText.GetBuffer(cchLen))
	{
		EscCopyCtrlString(pszText.data(), asMsg, cchLen);
		//if (cchLen > 0)
		//	wmemmove(pszText, asMsg, cchLen);
		//pszText[cchLen] = 0;

		wchar_t szExe[MAX_PATH] = {};
		GetModuleFileName(nullptr, szExe, countof(szExe));
		wchar_t szTitle[MAX_PATH+64];
		msprintf(szTitle, countof(szTitle), L"PID=%u, %s", GetCurrentProcessId(), PointToName(szExe));

		GuiMessageBox(ghConEmuWnd, pszText, szTitle, MB_ICONINFORMATION|MB_SYSTEMMODAL);
	}
}

bool CEAnsi::IsAnsiExecAllowed(LPCWSTR asCmd) const
{
	// Invalid command or macro?
	if (!asCmd || !*asCmd)
		return false;

	// We need to check settings
	CESERVER_CONSOLE_MAPPING_HDR* pMap = GetConMap();
	if (!pMap)
		return false;

	if ((pMap->Flags & ConEmu::ConsoleFlags::AnsiExecAny))
	{
		// Allowed in any process
	}
	else if ((pMap->Flags & ConEmu::ConsoleFlags::AnsiExecCmd))
	{
		// Allowed in Cmd.exe only
		if (!gbIsCmdProcess)
			return false;
	}
	else
	{
		// Disallowed everywhere
		return false;
	}

	// Now we need to ask GUI, if the command (asCmd) is allowed
	bool bAllowed = false;
	const INT_PTR cchLen = wcslen(asCmd) + 1;
	CESERVER_REQ* pOut = nullptr;
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_ALLOWANSIEXEC, sizeof(CESERVER_REQ_HDR)+sizeof(wchar_t)*cchLen);

	if (pIn)
	{
		_ASSERTE(sizeof(pIn->wData[0])==sizeof(*asCmd));
		memmove(pIn->wData, asCmd, cchLen*sizeof(pIn->wData[0]));

		pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		if (pOut && (pOut->DataSize() == sizeof(pOut->dwData[0])))
		{
			bAllowed = (pOut->dwData[0] == TRUE);
		}
	}

	ExecuteFreeResult(pOut);
	ExecuteFreeResult(pIn);

	return bAllowed;
}

// ESC ] 9 ; 6 ; "macro" ST        Execute some GuiMacro
void CEAnsi::DoGuiMacro(LPCWSTR asCmd, INT_PTR cchLen) const
{
	CESERVER_REQ* pOut = nullptr;
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_GUIMACRO, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_GUIMACRO)+sizeof(wchar_t)*(cchLen + 1));

	if (pIn)
	{
		EscCopyCtrlString(pIn->GuiMacro.sMacro, asCmd, cchLen);

		if (IsAnsiExecAllowed(pIn->GuiMacro.sMacro))
		{
			pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
		}
	}

	// EnvVar "ConEmuMacroResult"
	SetEnvironmentVariable(CEGUIMACRORETENVVAR, pOut && pOut->GuiMacro.nSucceeded ? pOut->GuiMacro.sMacro : nullptr);

	ExecuteFreeResult(pOut);
	ExecuteFreeResult(pIn);
}

// ESC ] 9 ; 7 ; "cmd" ST        Run some process with arguments
void CEAnsi::DoProcess(LPCWSTR asCmd, INT_PTR cchLen) const
{
	// We need zero-terminated string
	CEStr pszCmdLine;

	if (pszCmdLine.GetBuffer(cchLen))
	{
		EscCopyCtrlString(pszCmdLine.data(), asCmd, cchLen);

		if (IsAnsiExecAllowed(pszCmdLine))
		{
			STARTUPINFO si{};
			si.cb = sizeof(si);
			PROCESS_INFORMATION pi{};

			const BOOL bCreated = OnCreateProcessW(
				nullptr, pszCmdLine.data(), nullptr, nullptr,
				FALSE, 0, nullptr, nullptr, &si, &pi);
			if (bCreated)
			{
				if (pi.hProcess)
				{
					WaitForSingleObject(pi.hProcess, INFINITE);
					CloseHandle(pi.hProcess);
				}
				if (pi.hThread)
				{
					CloseHandle(pi.hThread);
				}
			}
		}
	}
}

// ESC ] 9 ; 8 ; "env" ST        Output value of environment variable
void CEAnsi::DoPrintEnv(LPCWSTR asCmd, INT_PTR cchLen)
{
	if (!pfnWriteConsoleW)
		return;

	// We need zero-terminated string
	CEStr pszVarName;

	if (pszVarName.GetBuffer(cchLen))
	{
		EscCopyCtrlString(pszVarName.data(), asCmd, cchLen);

		wchar_t szValue[MAX_PATH];
		wchar_t* pszValue = szValue;
		CEStr valueBuffer;
		DWORD cchMax = countof(szValue);
		DWORD nMax = GetEnvironmentVariable(pszVarName, pszValue, cchMax);

		// Some predefined as `time`, `date`, `cd`, ...
		if (!nMax)
		{
			if ((lstrcmpi(pszVarName, L"date") == 0)
				|| (lstrcmpi(pszVarName, L"time") == 0))
			{
				SYSTEMTIME st = {}; GetLocalTime(&st);
				if (lstrcmpi(pszVarName, L"date") == 0)
					swprintf_c(szValue, L"%u-%02u-%02u", st.wYear, st.wMonth, st.wDay);
				else
					swprintf_c(szValue, L"%u:%02u:%02u.%03u", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
				nMax = lstrlen(szValue);
			}
			#if 0
			else if (lstrcmpi(pszVarName, L"cd") == 0)
			{
				//TODO: If possible
			}
			#endif
		}

		if (nMax >= cchMax)
		{
			cchMax = nMax+1;
			pszValue = valueBuffer.GetBuffer(cchMax);
			nMax = pszValue ? GetEnvironmentVariable(pszVarName, szValue, countof(szValue)) : 0;
		}

		if (nMax)
		{
			TODO("Process here ANSI colors TOO! But now it will be 'reentrance'?");
			WriteText(pfnWriteConsoleW, mh_WriteOutput, pszValue, nMax, &cchMax);
		}
	}
}

// ESC ] 9 ; 9 ; "cwd" ST        Inform ConEmu about shell current working directory
void CEAnsi::DoSendCWD(LPCWSTR asCmd, INT_PTR cchLen) const
{
	// We need zero-terminated string
	CEStr pszCWD;

	if (pszCWD.GetBuffer(cchLen))
	{
		EscCopyCtrlString(pszCWD.data(), asCmd, cchLen);

		// Sends CECMD_STORECURDIR into RConServer
		SendCurrentDirectory(ghConWnd, pszCWD);
	}
}


// ReSharper disable once CppMemberFunctionMayBeStatic
BOOL CEAnsi::ReportString(LPCWSTR asRet)
{
	if (!asRet || !*asRet)
		return FALSE;
	INPUT_RECORD ir[16] = {};
	const size_t nLen = wcslen(asRet);
	if (nLen > std::numeric_limits<DWORD>::max())
		return false;
	INPUT_RECORD* pir = (nLen <= static_cast<int>(countof(ir))) ? ir : static_cast<INPUT_RECORD*>(calloc(nLen, sizeof(INPUT_RECORD)));
	if (!pir)
		return FALSE;

	INPUT_RECORD* p = pir;
	LPCWSTR pc = asRet;
	for (size_t i = 0; i < nLen; i++, p++, pc++)
	{
		const wchar_t ch = (*pc == 0x1B || (*pc >= 0x20 && *pc != 0x7F)) ? *pc : L' ';
		p->EventType = KEY_EVENT;
		p->Event.KeyEvent.bKeyDown = TRUE;
		p->Event.KeyEvent.wRepeatCount = 1;
		p->Event.KeyEvent.uChar.UnicodeChar = ch;
	}

	DumpKnownEscape(asRet, nLen, de_Report);

	DWORD nWritten = 0;
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	const BOOL bSuccess = WriteConsoleInput(hIn, pir, static_cast<DWORD>(nLen), &nWritten) && (nWritten == nLen);

	if (pir != ir)
		free(pir);
	return bSuccess;
}

void CEAnsi::ReportConsoleTitle()
{
	// Titles are untrusted output. Never echo them into input, even after
	// filtering (CVE-2022-46387 / CVE-2023-39150). Applies to ConPTY too.
}

void CEAnsi::ReportTerminalPixelSize()
{
	// `CSI 4 ; height ; width t`
	wchar_t szReport[64];
	int width = 0, height = 0;
	RECT rcWnd = {};
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};

	if (ghConEmuWndDC && GetClientRect(ghConEmuWndDC, &rcWnd))
	{
		width = RectWidth(rcWnd);
		height = RectHeight(rcWnd);
	}

	if ((width <= 0 || height <= 0) && ghConWnd && GetClientRect(ghConWnd, &rcWnd))
	{
		width = RectWidth(rcWnd);
		height = RectHeight(rcWnd);
	}

	if (width <= 0 || height <= 0)
	{
		_ASSERTE(width > 0 && height > 0);
		// Both DC and RealConsole windows were failed?
		if (GetConsoleScreenBufferInfoCached(GetStdHandle(STD_OUTPUT_HANDLE), &csbi))
		{
			const int defCharWidth = 8, defCharHeight = 14;
			width = (csbi.srWindow.Right - csbi.srWindow.Left + 1) * defCharWidth;
			height = (csbi.srWindow.Bottom - csbi.srWindow.Top + 1) * defCharHeight;
		}
	}

	if (width > 0 && height > 0)
	{
		swprintf_c(szReport, L"\x1B[4;%u;%ut", static_cast<uint32_t>(height), static_cast<uint32_t>(width));
		ReportString(szReport);
	}
}

void CEAnsi::ReportTerminalCharSize(HANDLE hConsoleOutput, int code)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
	{
		wchar_t sCurInfo[64];
		msprintf(sCurInfo, countof(sCurInfo),
			L"\x1B[%u;%u;%ut",
			code == 18 ? 8 : 9,
			csbi.srWindow.Bottom-csbi.srWindow.Top+1, csbi.srWindow.Right-csbi.srWindow.Left+1);
		ReportString(sCurInfo);
	}
}

void CEAnsi::ReportCursorPosition(HANDLE hConsoleOutput)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
	{
		wchar_t sCurInfo[32];
		msprintf(sCurInfo, countof(sCurInfo),
			L"\x1B[%u;%uR",
			csbi.dwCursorPosition.Y-csbi.srWindow.Top+1, csbi.dwCursorPosition.X-csbi.srWindow.Left+1);
		ReportString(sCurInfo);
	}
}

BOOL CEAnsi::WriteAnsiCodes(OnWriteConsoleW_t writeConsoleW, HANDLE hConsoleOutput, LPCWSTR lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	BOOL lbRc = TRUE, lbApply = FALSE;
	// ReSharper disable once CppLocalVariableMayBeConst
	LPCWSTR lpEnd = (lpBuffer + nNumberOfCharsToWrite);
	AnsiEscCode Code = {};
	wchar_t szPreDump[CEAnsi_MaxPrevPart];
	// ReSharper disable once CppJoinDeclarationAndAssignment
	DWORD cchPrevPart;

	ReloadFeatures();

	// Store this pointer
	pfnWriteConsoleW = writeConsoleW;
	// Ans current output handle
	mh_WriteOutput = hConsoleOutput;

	//ExtWriteTextParm write = {sizeof(write), ewtf_Current, hConsoleOutput};
	//write.Private = writeConsoleW;

	while (lpBuffer < lpEnd)
	{
		LPCWSTR lpStart = nullptr, lpNext = nullptr; // Required to be nullptr-initialized

		// '^' is ESC
		// ^[0;31;47m   $E[31;47m   ^[0m ^[0;1;31;47m  $E[1;31;47m  ^[0m

		cchPrevPart = 0;

		const int iEsc = NextEscCode(lpBuffer, lpEnd, szPreDump, cchPrevPart, lpStart, lpNext, Code);

		if (cchPrevPart)
		{
			if (lbApply)
			{
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			lbRc = WriteText(writeConsoleW, hConsoleOutput, szPreDump, cchPrevPart, lpNumberOfCharsWritten);
			if (!lbRc)
				goto wrap;
		}

		if (iEsc != 0)
		{
			if (lpStart > lpBuffer)
			{
				_ASSERTEX((lpStart-lpBuffer) < static_cast<INT_PTR>(nNumberOfCharsToWrite));

				if (lbApply)
				{
					ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
					lbApply = FALSE;
				}

				const DWORD nWrite = static_cast<DWORD>(lpStart - lpBuffer);
				//lbRc = WriteText(writeConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				lbRc = WriteText(writeConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten, FALSE);
				if (!lbRc)
					goto wrap;
				//write.Buffer = lpBuffer;
				//write.NumberOfCharsToWrite = nWrite;
				//lbRc = ExtWriteText(&write);
				//if (!lbRc)
				//	goto wrap;
				//else
				//{
				//	if (lpNumberOfCharsWritten)
				//		*lpNumberOfCharsWritten = write.NumberOfCharsWritten;
				//	if (write.ScrolledRowsUp > 0)
				//		gDisplayCursor.StoredCursorPos.Y = std::max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)write.ScrolledRowsUp));
				//}
			}

			if (iEsc == 1)
			{
				if (Code.Skip)
				{
					DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
				}
				else
				{
					switch (Code.Second)
					{
					case L'[':
						{
							lbApply = TRUE;
							WriteAnsiCode_CSI(writeConsoleW, hConsoleOutput, Code, lbApply);

						} // case L'[':
						break;

					case L']':
						{
							lbApply = TRUE;
							WriteAnsiCode_OSC(writeConsoleW, hConsoleOutput, Code, lbApply);

						} // case L']':
						break;

					case L'|':
						{
							// vim-xterm-emulation
							lbApply = TRUE;
							WriteAnsiCode_VIM(writeConsoleW, hConsoleOutput, Code, lbApply);
						} // case L'|':
						break;

					case L'7':
					case L'8':
						//TODO: 7 - Save Cursor and _Attributes_
						//TODO: 8 - Restore Cursor and _Attributes_
						XTermSaveRestoreCursor((Code.Second == L'7'), hConsoleOutput);
						break;
					case L'c':
						// Full reset
						FullReset(hConsoleOutput);
						lbApply = FALSE;
						break;
					case L'g':
						// User may disable flashing in ConEmu settings
						GuiFlashWindow(eFlashBeep, ghConWnd, FALSE, FLASHW_ALL, 1, 0);
						break;
					case L'H':
						// #ANSI gh-1827: support 'H' to set tab stops
						DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
						break;
					case L'M':
						ReverseLF(hConsoleOutput, lbApply);
						break;
					case L'E':
						WriteText(writeConsoleW, hConsoleOutput, L"\r\n", 2, nullptr);
						break;
					case L'D':
						ForwardLF(hConsoleOutput, lbApply);
						break;
					case L'=':
					case L'>':
						// xterm "ESC =" - Application Keypad (DECKPAM)
						// xterm "ESC >" - Normal Keypad (DECKPNM)
						DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
						break;
					case L'(':
						// xterm G0..G3?
						switch (Code.Action)
						{
						case L'0':
							mCharSet = VTCS_DRAWING;
							//DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Comment);
							break;
						case L'B':
							mCharSet = VTCS_DEFAULT;
							//DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Comment);
							break;
						default:
							mCharSet = VTCS_DEFAULT;
							DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
						}
						break;

					default:
						DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
					}
				}
			}
		}
		else //if (iEsc != 2) // 2 - means "Esc part stored in buffer"
		{
			_ASSERTEX(iEsc == 0);
			if (lpNext > lpBuffer)
			{
				if (lbApply)
				{
					ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
					lbApply = FALSE;
				}

				const DWORD nWrite = static_cast<DWORD>(lpNext - lpBuffer);
				//lbRc = WriteText(writeConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				lbRc = WriteText(writeConsoleW, hConsoleOutput, lpBuffer, nWrite, lpNumberOfCharsWritten);
				if (!lbRc)
					goto wrap;
				//write.Buffer = lpBuffer;
				//write.NumberOfCharsToWrite = nWrite;
				//lbRc = ExtWriteText(&write);
				//if (!lbRc)
				//	goto wrap;
				//else
				//{
				//	if (lpNumberOfCharsWritten)
				//		*lpNumberOfCharsWritten = write.NumberOfCharsWritten;
				//	if (write.ScrolledRowsUp > 0)
				//		gDisplayCursor.StoredCursorPos.Y = std::max(0,((int)gDisplayCursor.StoredCursorPos.Y - (int)write.ScrolledRowsUp));
				//}
			}
		}

		if (lpNext > lpBuffer)
		{
			lpBuffer = lpNext;
		}
		else
		{
			_ASSERTEX(lpNext > lpBuffer || lpNext == nullptr);
			++lpBuffer;
		}
	}

	if (lbRc && lpNumberOfCharsWritten)
		*lpNumberOfCharsWritten = nNumberOfCharsToWrite;

wrap:
	if (lbApply)
	{
		ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
		lbApply = FALSE;
	}
	return lbRc;
}

void CEAnsi::WriteAnsiCode_CSI(OnWriteConsoleW_t writeConsoleW, HANDLE& hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply)
{
	/*

CSI ? P m h			DEC Private Mode Set (DECSET)
	P s = 4 7 → Use Alternate Screen Buffer (unless disabled by the titeInhibit resource)
	P s = 1 0 4 7 → Use Alternate Screen Buffer (unless disabled by the titeInhibit resource)
	P s = 1 0 4 8 → Save cursor as in DECSC (unless disabled by the titeInhibit resource)
	P s = 1 0 4 9 → Save cursor as in DECSC and use Alternate Screen Buffer, clearing it first (unless disabled by the titeInhibit resource). This combines the effects of the 1 0 4 7 and 1 0 4 8 modes. Use this with terminfo-based applications rather than the 4 7 mode.

CSI ? P m l			DEC Private Mode Reset (DECRST)
	P s = 4 7 → Use Normal Screen Buffer
	P s = 1 0 4 7 → Use Normal Screen Buffer, clearing screen first if in the Alternate Screen (unless disabled by the titeInhibit resource)
	P s = 1 0 4 8 → Restore cursor as in DECRC (unless disabled by the titeInhibit resource)
	P s = 1 0 4 9 → Use Normal Screen Buffer and restore cursor as in DECRC (unless disabled by the titeInhibit resource). This combines the effects of the 1 0 4 7 and 1 0 4 8 modes. Use this with terminfo-based applications rather than the 4 7 mode.


CSI P s @			Insert P s (Blank) Character(s) (default = 1) (ICH)

	*/
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};

	switch (Code.Action) // case sensitive
	{
	case L's':
		// Save cursor position (can not be nested)
		XTermSaveRestoreCursor(true, hConsoleOutput);
		break;

	case L'u':
		// Restore cursor position
		XTermSaveRestoreCursor(false, hConsoleOutput);
		break;

	case L'H': // Set cursor position (1-based)
	case L'f': // Same as 'H'
	case L'A': // Cursor up by N rows
	case L'B': // Cursor down by N rows
	case L'C': // Cursor right by N cols
	case L'D': // Cursor left by N cols
	case L'E': // Moves cursor to beginning of the line n (default 1) lines down.
	case L'F': // Moves cursor to beginning of the line n (default 1) lines up.
	case L'G': // Moves the cursor to column n.
	case L'd': // Moves the cursor to line n.
		// Change cursor position
		if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		{
			struct PointXY {int X,Y;};
			auto get_scroll_region = [&csbi]()
			{
				const auto& srw = csbi.srWindow;
				SMALL_RECT clipRgn = MakeSmallRect(0, 0, srw.Right - srw.Left, srw.Bottom - srw.Top);
				if (gDisplayOpt.ScrollRegion)
				{
					clipRgn.Top = std::max<SHORT>(0, std::min<SHORT>(gDisplayOpt.ScrollStart, csbi.dwSize.Y-1));
					clipRgn.Bottom = std::max<SHORT>(0, std::min<SHORT>(gDisplayOpt.ScrollEnd, csbi.dwSize.Y-1));
				}
				return clipRgn;
			};
			auto get_cursor = [&csbi]()
			{
				const auto& srw = csbi.srWindow;
				const int visible_rows = srw.Bottom - srw.Top;
				return PointXY{
					csbi.dwCursorPosition.X,
					std::max(0, std::min(visible_rows, csbi.dwCursorPosition.Y - srw.Top))
				};
			};
			const struct {int left, top, right, bottom;} workRgn = {0, 0, csbi.dwSize.X - 1, csbi.dwSize.Y - 1};
			_ASSERTEX(workRgn.left <= workRgn.right && workRgn.top <= workRgn.bottom);
			const auto& clipRgn = get_scroll_region();
			const auto cur = get_cursor();
			PointXY crNewPos = cur;

			enum class Direction { kAbsolute, kCompatible, kRelative };

			auto set_y = [&crNewPos, &workRgn, &clipRgn, &cur](Direction direction, int value)
			{
				const int kLegacyY = 9999;
				if (direction == Direction::kCompatible && value >= std::min(kLegacyY, workRgn.bottom))
					// #XTERM_256 Allow to put cursor into the legacy true-color area
					crNewPos.Y = workRgn.bottom;
				else if (direction == Direction::kAbsolute || direction == Direction::kCompatible)
					crNewPos.Y = std::max(workRgn.top, std::min(workRgn.bottom, value));
				else if (value < 0 && cur.Y >= clipRgn.Top)
					crNewPos.Y = std::max<int>(clipRgn.Top, std::min(workRgn.bottom, cur.Y + value));
				else if (value > 0 && cur.Y <= clipRgn.Bottom)
					crNewPos.Y = std::max(workRgn.top, std::min<int>(clipRgn.Bottom, cur.Y + value));
				else
					crNewPos.Y = std::max(workRgn.top, std::min(workRgn.bottom, cur.Y + value));
			};
			auto set_x = [&crNewPos, &workRgn, &cur](Direction direction, int value)
			{
				if (direction == Direction::kAbsolute || direction == Direction::kCompatible)
					crNewPos.X = std::max(workRgn.left, std::min(workRgn.right, value));
				else
					crNewPos.X = std::max(workRgn.left, std::min(workRgn.right, cur.X + value));
			};

			switch (Code.Action)
			{
			case L'H':
				// Set cursor position (ANSI values are 1-based)
				set_y(Direction::kCompatible, (Code.ArgC > 0 && Code.ArgV[0]) ? (Code.ArgV[0] - 1) : 0);
				set_x(Direction::kAbsolute, (Code.ArgC > 1 && Code.ArgV[1]) ? (Code.ArgV[1] - 1) : 0);
				break;
			case L'f':
				// Set cursor position (ANSI values are 1-based)
				set_y(Direction::kAbsolute, (Code.ArgC > 0 && Code.ArgV[0]) ? (Code.ArgV[0] - 1) : 0);
				set_x(Direction::kAbsolute, (Code.ArgC > 1 && Code.ArgV[1]) ? (Code.ArgV[1] - 1) : 0);
				break;
			case L'A':
				// Cursor up by N rows
				set_y(Direction::kRelative, -((Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1));
				break;
			case L'B':
				// Cursor down by N rows
				set_y(Direction::kRelative, +((Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1));
				break;
			case L'C':
				// Cursor right by N cols
				set_x(Direction::kRelative, +((Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1));
				break;
			case L'D':
				// Cursor left by N cols
				set_x(Direction::kRelative, -((Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1));
				break;
			case L'E':
				// Moves cursor to beginning of the line n (default 1) lines down.
				set_y(Direction::kRelative, +((Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1));
				set_x(Direction::kAbsolute, 0);
				break;
			case L'F':
				// Moves cursor to beginning of the line n (default 1) lines up.
				set_y(Direction::kRelative, -((Code.ArgC > 0 && Code.ArgV[0]) ? Code.ArgV[0] : 1));
				set_x(Direction::kAbsolute, 0);
				break;
			case L'G':
				// Moves the cursor to column n.
				set_x(Direction::kAbsolute, (Code.ArgC > 0) ? (Code.ArgV[0] - 1) : 0);
				break;
			case L'd':
				// Moves the cursor to line n (almost the same as 'H', but leave X unchanged).
				set_y(Direction::kAbsolute, (Code.ArgC > 0) ? (Code.ArgV[0] - 1) : 0);
				break;
			#ifdef _DEBUG
			default:
				_ASSERTEX(FALSE && "Missed (sub)case value!");
			#endif
			}

			// Goto
			ORIGINAL_KRNL(SetConsoleCursorPosition);
			{
			const auto& srw = csbi.srWindow;
			COORD crNewPosAPI = {
				static_cast<SHORT>(crNewPos.X),
				static_cast<SHORT>(std::min<int>(csbi.dwSize.Y - 1, srw.Top + crNewPos.Y))
			};
			_ASSERTE(crNewPosAPI.X == crNewPos.X);
			F(SetConsoleCursorPosition)(hConsoleOutput, crNewPosAPI);
			}

			if (gbIsVimProcess)
				gbIsVimAnsi = true;
		} // case 'H', 'f', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'd'
		break;

	case L'J': // Clears part of the screen
		// Clears the screen and moves the cursor to the home position (line 0, column 0).
		if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		{
			if (lbApply)
			{
				// Apply default color before scrolling!
				// ViM: need to fill whole screen with selected background color, so Apply attributes
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			int nCmd = (Code.ArgC > 0) ? Code.ArgV[0] : 0;
			bool resetCursor = false;
			COORD cr0 = {0, csbi.srWindow.Top};
			int nChars = 0;

			switch (nCmd)
			{
			case 0:
				// clear from cursor to end of screen
				cr0 = csbi.dwCursorPosition;
				nChars = (csbi.dwSize.X - csbi.dwCursorPosition.X)
					+ csbi.dwSize.X * (csbi.srWindow.Bottom - csbi.dwCursorPosition.Y);
				break;
			case 1:
				// clear from cursor to beginning of the screen
				nChars = csbi.dwCursorPosition.X + 1
					+ csbi.dwSize.X * (csbi.dwCursorPosition.Y - csbi.srWindow.Top);
				break;
			case 2:
				// clear viewport and moves cursor to viewport's upper left
				nChars = csbi.dwSize.X * (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
				resetCursor = true;
				break;
			case 3:
				// xterm: clear scrollback buffer entirely
				if (csbi.srWindow.Top > 0)
				{
					cr0.X = csbi.dwCursorPosition.X;
					cr0.Y = static_cast<SHORT>(std::max(0, (csbi.dwCursorPosition.Y - csbi.srWindow.Top)));
					resetCursor = true;

					SMALL_RECT scroll{ 0, 0, static_cast<SHORT>(csbi.dwSize.X - 1),
						static_cast<SHORT>(csbi.dwSize.Y - 1) };
					ScrollScreen(hConsoleOutput, -csbi.srWindow.Top, true, scroll);

					SMALL_RECT topLeft = { 0, 0, scroll.Right,
						static_cast<SHORT>(csbi.srWindow.Bottom - csbi.srWindow.Top) };
					SetConsoleWindowInfo(hConsoleOutput, TRUE, &topLeft);

					UpdateAppMapRows(0, true);
				}
				break;
			default:
				DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
			}

			if (nChars > 0)
			{
				ExtFillOutputParm fill = {sizeof(fill), efof_Current|efof_Attribute|efof_Character,
					hConsoleOutput, {}, L' ', cr0, static_cast<DWORD>(nChars)};
				ExtFillOutput(&fill);
			}

			if (resetCursor)
			{
				SetConsoleCursorPosition(hConsoleOutput, cr0);
			}

		} // case L'J':
		break;

	case L'b':
		if (!Code.PvtLen)
		{
			int repeat = (Code.ArgC > 0) ? Code.ArgV[0] : 1;
			if (m_LastWrittenChar && repeat > 0)
			{
				CEStr buffer;
				if (wchar_t* ptr = buffer.GetBuffer(repeat))
				{
					for (int i = 0; i < repeat; ++i)
						ptr[i] = m_LastWrittenChar;
					WriteText(writeConsoleW, hConsoleOutput, ptr, repeat, nullptr);
				}
			}
		}
		else
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		break; // case L'b'

	case L'K': // Erases part of the line
		// Clears all characters from the cursor position to the end of the line
		// (including the character at the cursor position).
		if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		{
			if (lbApply)
			{
				// Apply default color before scrolling!
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			TODO("Need to clear attributes?");
			int nChars = 0;
			int nCmd = (Code.ArgC > 0) ? Code.ArgV[0] : 0;
			COORD cr0 = csbi.dwCursorPosition;

			switch (nCmd)
			{
			case 0: // clear from cursor to the end of the line
				nChars = csbi.dwSize.X - csbi.dwCursorPosition.X;
				break;
			case 1: // clear from cursor to beginning of the line
				cr0.X = 0;
				nChars = csbi.dwCursorPosition.X + 1;
				break;
			case 2: // clear entire line
				cr0.X = 0;
				nChars = csbi.dwSize.X;
				break;
			default:
				DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
			}


			if (nChars > 0)
			{
				ExtFillOutputParm fill = { sizeof(fill), efof_Current | efof_Attribute | efof_Character,
					hConsoleOutput, {}, L' ', cr0, static_cast<DWORD>(nChars) };
				ExtFillOutput(&fill);
			}
		} // case L'K':
		break;

	case L'r':
		//\027[Pt;Pbr
		//
		//Pt is the number of the top line of the scrolling region;
		//Pb is the number of the bottom line of the scrolling region
		// and must be greater than Pt.
		//(The default for Pt is line 1, the default for Pb is the end
		// of the screen)
		//
		if ((Code.ArgC >= 2) && (Code.ArgV[0] >= 0) && (Code.ArgV[1] >= Code.ArgV[0]))
		{
			// Values are 1-based
			SetScrollRegion(true, true, Code.ArgV[0], Code.ArgV[1], hConsoleOutput);
		}
		else
		{
			SetScrollRegion(false);
		}
		break;

	case L'S':
		// Scroll whole page up by n (default 1) lines. New lines are added at the bottom.
		if (lbApply)
		{
			// Apply default color before scrolling!
			ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
			lbApply = FALSE;
		}
		ScrollScreen(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? -Code.ArgV[0] : -1);
		break;

	case L'L':
		// Insert P s Line(s) (default = 1) (IL).
		LinesInsert(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1);
		break;
	case L'M':
		// Delete N Line(s) (default = 1) (DL).
		// This is actually "Scroll UP N line(s) inside defined scrolling region"
		LinesDelete(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1);
		break;

	case L'@':
		// Insert P s (Blank) Character(s) (default = 1) (ICH).
		ScrollLine(hConsoleOutput, ((Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1));
		break;
	case L'P':
		// Delete P s Character(s) (default = 1) (DCH).
		ScrollLine(hConsoleOutput, -((Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1));
		break;

	case L'T':
		// Scroll whole page down by n (default 1) lines. New lines are added at the top.
		if (lbApply)
		{
			// Apply default color before scrolling!
			ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
		}
		TODO("Define scrolling region");
		ScrollScreen(hConsoleOutput, (Code.ArgC > 0 && Code.ArgV[0] > 0) ? Code.ArgV[0] : 1);
		break;

	case L'h':
	case L'l':
		// Set/ReSet Mode
		if (Code.ArgC > 0)
		{
			//ESC [ 3 h
			//       DECCRM (default off): Display control chars.

			//ESC [ 4 h
			//       DECIM (default off): Set insert mode.

			//ESC [ 20 h
			//       LF/NL (default off): Automatically follow echo of LF, VT or FF with CR.

			//ESC [ ? 1 h
			//	  DECCKM (default off): When set, the cursor keys send an ESC O prefix,
			//	  rather than ESC [.

			//ESC [ ? 3 h
			//	  DECCOLM (default off = 80 columns): 80/132 col mode switch.  The driver
			//	  sources note that this alone does not suffice; some user-mode utility
			//	  such as resizecons(8) has to change the hardware registers on the
			//	  console video card.

			//ESC [ ? 5 h
			//	  DECSCNM (default off): Set reverse-video mode.

			//ESC [ ? 6 h
			//	  DECOM (default off): When set, cursor addressing is relative to the
			//	  upper left corner of the scrolling region.


			//ESC [ ? 8 h
			//	  DECARM (default on): Set keyboard autorepeat on.

			//ESC [ ? 9 h
			//	  X10 Mouse Reporting (default off): Set reporting mode to 1 (or reset to
			//	  0) -- see below.

			//ESC [ ? 1000 h
			//	  X11 Mouse Reporting (default off): Set reporting mode to 2 (or reset to
			//	  0) -- see below.

			//ESC [ ? 7711 h
			//    mimic mintty code, same as "ESC ] 9 ; 12 ST"

			switch (Code.ArgV[0])
			{
			case 1:
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
				{
					gDisplayCursor.CursorKeysApp = (Code.Action == L'h');

					if (gbIsVimProcess)
					{
						TODO("Need to find proper way for activation alternative buffer from ViM?");
						if (Code.Action == L'h')
						{
							hConsoleOutput = StartVimTerm(false);
						}
						else
						{
							hConsoleOutput = StopVimTerm();
						}
					}

					ChangeTermMode(tmc_AppCursorKeys, (Code.Action == L'h'));
				}
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				break;
			case 3:
				gDisplayOpt.ShowRawAnsi = (Code.Action == L'h');
				break;
			case 7:
				//ESC [ ? 7 h
				//	  DECAWM (default off): Set autowrap on.  In this mode, a graphic
				//	  character emitted after column 80 (or column 132 of DECCOLM is on)
				//	  forces a wrap to the beginning of the following line first.
				//ESC [ = 7 h
				//    Enables line wrapping
				//ESC [ 7 ; _col_ h
				//    Our extension. _col_ - wrap at column (1-based), default = 80
				if ((gDisplayOpt.WrapWasSet = (Code.Action == L'h')))
				{
					gDisplayOpt.WrapAt = ((Code.ArgC > 1) && (Code.ArgV[1] > 0)) ? static_cast<SHORT>(Code.ArgV[1]) : 80;
				}
				break;
			case 20:
				if (Code.PvtLen == 0)
				{
					SetAutoLfNl(Code.Action == L'h');
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
				}
				else
				{
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				}
				break;
			//ESC [ ? 12 h
			//	  Start Blinking Cursor (att610)
			case 12:
			//ESC [ ? 25 h
			//	  DECTECM (default on): Make cursor visible.
			case 25:
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
				{
					for (int i = 0; i < Code.ArgC; ++i)
					{
						if (Code.ArgV[i] == 25)
						{
							CONSOLE_CURSOR_INFO ci = {};
							if (GetConsoleCursorInfo(hConsoleOutput, &ci))
							{
								ci.bVisible = (Code.Action == L'h');
								SetConsoleCursorInfo(hConsoleOutput, &ci);
							}
						}
						else
						{
							// DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
						}
					}
				}
				else
				{
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				}
				break;
			case 4:
				if (Code.PvtLen == 0)
				{  // NOLINT(bugprone-branch-clone)
					/* h=Insert Mode (IRM), l=Replace Mode (IRM) */
					// Nano posts the `ESC [ 4 l` on start, but do not post `ESC [ 4 h` on exit, that is strange...
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored); // ignored for now
				}
				else if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
				{
					/* h=Smooth (slow) scroll, l=Jump (fast) scroll */
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored); // ignored for now
				}
				else
				{
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				}
				break;
			case 9:    /* X10_MOUSE */
			case 1000: /* VT200_MOUSE */
			case 1002: /* BTN_EVENT_MOUSE */
			case 1003: /* ANY_EVENT_MOUSE */
			case 1004: /* FOCUS_EVENT_MOUSE */
			case 1005: /* Xterm's UTF8 encoding for mouse positions */
			case 1006: /* Xterm's CSI-style mouse encoding */
			case 1015: /* Urxvt's CSI-style mouse encoding */
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
				{
					static DWORD LastMode = 0;
					TermMouseMode ModeMask = (Code.ArgV[0] == 9) ? tmm_X10
						: (Code.ArgV[0] == 1000) ? tmm_VT200
						: (Code.ArgV[0] == 1002) ? tmm_BTN
						: (Code.ArgV[0] == 1003) ? tmm_ANY
						: (Code.ArgV[0] == 1004) ? tmm_FOCUS
						: (Code.ArgV[0] == 1005) ? tmm_UTF8
						: (Code.ArgV[0] == 1006) ? tmm_XTERM
						: (Code.ArgV[0] == 1015) ? tmm_URXVT
						: tmm_None;
					DWORD Mode = (Code.Action == L'h')
						? (LastMode | ModeMask)
						: (LastMode & ~ModeMask);
					LastMode = Mode;
					ChangeTermMode(tmc_MouseMode, Mode);
				}
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				break;
			case 7786: /* 'V': Mouse wheel reporting */
			case 7787: /* 'W': Application mouse wheel mode */
			case 1034: // Interpret "meta" key, sets eighth bit. (enables/disables the eightBitInput resource).
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				break;
			case 47:   /* alternate screen */
			case 1047: /* alternate screen */
			case 1049: /* cursor & alternate screen */
				// xmux/screen: Alternate screen
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
				{
					if (lbApply)
					{
						ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
						lbApply = FALSE;
					}

					// \e[?1049h: save cursor pos
					if ((Code.ArgV[0] == 1049) && (Code.Action == L'h'))
						XTermSaveRestoreCursor(true, hConsoleOutput);
					// h: switch to alternative buffer without backscroll
					// l: restore saved scrollback buffer
					hConsoleOutput = XTermAltBuffer((Code.Action == L'h'), Code.ArgV[0]);
					// \e[?1049l - restore cursor pos
					if ((Code.ArgV[0] == 1049) && (Code.Action == L'l'))
						XTermSaveRestoreCursor(false, hConsoleOutput);
				}
				else
				{
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				}
				break;
			case 1048: /* save/restore cursor */
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
					XTermSaveRestoreCursor((Code.Action == L'h'), hConsoleOutput);
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				break;
			case 2004: /* bracketed paste */
				/* All "pasted" text will be wrapped in `\e[200~ ... \e[201~` */
				if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
					ChangeTermMode(tmc_BracketedPaste, (Code.Action == L'h'));
				else
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				break;
			case 7711:
				if ((Code.Action == L'h') && (Code.PvtLen == 1) && (Code.Pvt[0] == L'?'))
				{
					StorePromptBegin();
				}
				else
				{
					DumpUnknownEscape(Code.pszEscStart, Code.nTotalLen);
				}
				break;
			default:
				DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
			}

			//switch (Code.ArgV[0])
			//{
			//case 0: case 1:
			//	// 40x25
			//	if ((gDisplayOpt.WrapWasSet = (Code.Action == L'h')))
			//	{
			//		gDisplayOpt.WrapAt = 40;
			//	}
			//	break;
			//case 2: case 3:
			//	// 80x25
			//	if ((gDisplayOpt.WrapWasSet = (Code.Action == L'h')))
			//	{
			//		gDisplayOpt.WrapAt = 80;
			//	}
			//	break;
			//case 7:
			//	{
			//		DWORD Mode = 0;
			//		GetConsoleMode(hConsoleOutput, &Mode);
			//		if (Code.Action == L'h')
			//			Mode |= ENABLE_WRAP_AT_EOL_OUTPUT;
			//		else
			//			Mode &= ~ENABLE_WRAP_AT_EOL_OUTPUT;
			//		SetConsoleMode(hConsoleOutput, Mode);
			//	} // enable/disable line wrapping
			//	break;
			//}
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break; // case L'h': case L'l':

	case L'n':
		if (Code.ArgC > 0)
		{
			switch (*Code.ArgV)
			{
			case 5:
				//ESC [ 5 n
				//      Device status report (DSR): Answer is ESC [ 0 n (Terminal OK).
				//
				ReportString(L"\x1B[0n");
				break;
			case 6:
				//ESC [ 6 n
				//      Cursor position report (CPR): Answer is ESC [ y ; x R, where x,y is the
				//      cursor location.
				ReportCursorPosition(hConsoleOutput);
				break;
			default:
				DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
			}
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break;

	case L'm':
		if (Code.PvtLen > 0)
		{
			DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
		}
		// Set display mode (colors, fonts, etc.)
		else if (!Code.ArgC)
		{
			ReSetDisplayParm(hConsoleOutput, TRUE, FALSE);
		}
		else
		{
			for (int i = 0; i < Code.ArgC; i++)
			{
				switch (Code.ArgV[i])
				{
				case 0:
					ReSetDisplayParm(hConsoleOutput, TRUE, FALSE);
					break;
				case 1:
					// Bold
					gDisplayParm.setBrightOrBold(TRUE);
					break;
				case 2:
					// Faint, decreased intensity (ISO 6429)
				case 22:
					// Normal (neither bold nor faint).
					gDisplayParm.setBrightOrBold(FALSE);
					break;
				case 3:
					// Italic
					gDisplayParm.setItalic(TRUE);
					break;
				case 23:
					// Not italic
					gDisplayParm.setItalic(FALSE);
					break;
				case 5: // #TODO ANSI Slow Blink (less than 150 per minute)
				case 6: // #TODO ANSI Rapid Blink (150+ per minute)
				case 25: // #TODO ANSI Blink Off
					DumpKnownEscape(Code.pszEscStart,Code.nTotalLen,de_Ignored);
					break;
				case 4: // Underlined
					gDisplayParm.setUnderline(true);
					break;
				case 24:
					// Not underlined
					gDisplayParm.setUnderline(false);
					break;
				case 7:
					// Reverse video
					gDisplayParm.setInverse(TRUE);
					break;
				case 27:
					// Positive (not inverse)
					gDisplayParm.setInverse(FALSE);
					break;
				case 9:
					// Crossed-out / strikethrough
					gDisplayParm.setCrossed(true);
					break;
				case 29:
					// Not Crossed-out / Not strikethrough
					gDisplayParm.setCrossed(false);
					break;
				case 30: case 31: case 32: case 33: case 34: case 35: case 36: case 37:
					gDisplayParm.setTextColor(Code.ArgV[i] - 30);
					gDisplayParm.setBrightFore(FALSE);
					gDisplayParm.setText256(clr4b);
					break;
				case 38:
					// xterm-256 colors
					// ESC [ 38 ; 5 ; I m -- set foreground to I (0..255) color from xterm palette
					if (((i+2) < Code.ArgC) && (Code.ArgV[i+1] == 5))
					{
						gDisplayParm.setTextColor(Code.ArgV[i+2] & 0xFF);
						gDisplayParm.setText256(clr8b);
						i += 2;
					}
					// xterm-256 colors
					// ESC [ 38 ; 2 ; R ; G ; B m -- set foreground to RGB(R,G,B) 24-bit color
					else if (((i+4) < Code.ArgC) && (Code.ArgV[i+1] == 2))
					{
						gDisplayParm.setTextColor(RGB((Code.ArgV[i+2] & 0xFF),(Code.ArgV[i+3] & 0xFF),(Code.ArgV[i+4] & 0xFF)));
						gDisplayParm.setText256(clr24b);
						i += 4;
					}
					break;
				case 39:
					// Reset
					gDisplayParm.setTextColor(CONFORECOLOR(GetDefaultTextAttr()));
					gDisplayParm.setBrightFore(FALSE);
					gDisplayParm.setText256(clr4b);
					break;
				case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47:
					gDisplayParm.setBackColor(Code.ArgV[i] - 40);
					gDisplayParm.setBrightBack(FALSE);
					gDisplayParm.setBack256(clr4b);
					break;
				case 48:
					// xterm-256 colors
					// ESC [ 48 ; 5 ; I m -- set background to I (0..255) color from xterm palette
					if (((i+2) < Code.ArgC) && (Code.ArgV[i+1] == 5))
					{
						gDisplayParm.setBackColor(Code.ArgV[i+2] & 0xFF);
						gDisplayParm.setBack256(clr8b);
						i += 2;
					}
					// xterm-256 colors
					// ESC [ 48 ; 2 ; R ; G ; B m -- set background to RGB(R,G,B) 24-bit color
					else if (((i+4) < Code.ArgC) && (Code.ArgV[i+1] == 2))
					{
						gDisplayParm.setBackColor(RGB((Code.ArgV[i+2] & 0xFF),(Code.ArgV[i+3] & 0xFF),(Code.ArgV[i+4] & 0xFF)));
						gDisplayParm.setBack256(clr24b);
						i += 4;
					}
					break;
				case 49:
					// Reset
					gDisplayParm.setBackColor(CONBACKCOLOR(GetDefaultTextAttr()));
					gDisplayParm.setBrightBack(FALSE);
					gDisplayParm.setBack256(clr4b);
					break;
				case 90: case 91: case 92: case 93: case 94: case 95: case 96: case 97:
					gDisplayParm.setTextColor((Code.ArgV[i] - 90) | 0x8);
					gDisplayParm.setText256(clr4b);
					gDisplayParm.setBrightFore(TRUE);
					break;
				case 100: case 101: case 102: case 103: case 104: case 105: case 106: case 107:
					gDisplayParm.setBackColor((Code.ArgV[i] - 100) | 0x8);
					gDisplayParm.setBack256(clr4b);
					gDisplayParm.setBrightBack(TRUE);
					break;
				case 10:  // NOLINT(bugprone-branch-clone)
					// Something strange and unknown... (received from ssh)
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
					break;
				case 312:
				case 315:
				case 414:
				case 3130:
					// Something strange and unknown... (received from vim on WSL)
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
					break;
				default:
					DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
				}
			}
		}
		break; // "[...m"

	case L'p':
		if (Code.ArgC == 0 && Code.PvtLen == 1 && Code.Pvt[0] == L'!')
		{
			FullReset(hConsoleOutput);
			lbApply = FALSE;
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break; // "[!p"

	case L'q':
		if ((Code.PvtLen == 1) && (Code.Pvt[0] == L' '))
		{
			/*
			CSI Ps SP q
				Set cursor style (DECSCUSR, VT520).
					Ps = 0  -> ConEmu's default.
					Ps = 1  -> blinking block.
					Ps = 2  -> steady block.
					Ps = 3  -> blinking underline.
					Ps = 4  -> steady underline.
					Ps = 5  -> blinking bar (xterm).
					Ps = 6  -> steady bar (xterm).
			*/
			DWORD nStyle = ((Code.ArgC == 0) || (Code.ArgV[0] < 0) || (Code.ArgV[0] > 6))
				? 0 : Code.ArgV[0];
			CONSOLE_CURSOR_INFO ci = {};
			if (GetConsoleCursorInfo(hConsoleOutput, &ci))
			{
				// We can't implement all possible styles in RealConsole,
				// but we can use "Block/Underline" shapes...
				ci.dwSize = (nStyle == 1 || nStyle == 2) ? 100 : 15;
				SetConsoleCursorInfo(hConsoleOutput, &ci);
			}
			ChangeTermMode(tmc_CursorShape, nStyle);
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break; // "[...q"

	case L't':
		if (Code.ArgC > 0 && Code.ArgC <= 3)
		{
			for (int i = 0; i < Code.ArgC; i++)
			{
				switch (Code.ArgV[i])
				{
				case 8:
					// `ESC [ 8 ; height ; width t` --> Resize the text area to [height;width] in characters.
					{
						int height = -1, width = -1;
						if (i < Code.ArgC)
							height = Code.ArgV[++i];
						if (i < Code.ArgC)
							width = Code.ArgV[++i];
						DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
						std::ignore = height;
						std::ignore = width;
					}
					break;
				case 14:
					// `ESC [ 1 4 t` --> Reports terminal window size in pixels as `CSI 4 ; height ; width t`.
					ReportTerminalPixelSize();
					break;
				case 18:
				case 19:
					// `ESC [ 1 8 t` --> Report the size of the text area in characters as `CSI 8 ; height ; width t`
					// `ESC [ 1 9 t` --> Report the size of the screen in characters as `CSI 9 ; height ; width t`
					ReportTerminalCharSize(hConsoleOutput, Code.ArgV[i]);
					break;
				case 21:
					// `ESC [ 2 1 t` --> Report terminal window title as `OSC l title ST`
					ReportConsoleTitle();
					break;
				case 22:
				case 23:
					// `ESC [ 2 2 ; 0 t` --> Save xterm icon and window title on stack.
					// `ESC [ 2 2 ; 1 t` --> Save xterm icon title on stack.
					// `ESC [ 2 2 ; 2 t` --> Save xterm window title on stack.
					// `ESC [ 2 3 ; 0 t` --> Restore xterm icon and window title from stack.
					// `ESC [ 2 3 ; 1 t` --> Restore xterm icon title from stack.
					// `ESC [ 2 3 ; 2 t` --> Restore xterm window title from stack.
					if (i < Code.ArgC)
						++i; // subcommand
					if (i < Code.ArgC && !Code.ArgV[i])
						++i; // strange sequence 22;0;0t
					DumpKnownEscape(Code.pszEscStart, Code.nTotalLen, de_Ignored);
					break;
				default:
					TODO("ANSI: xterm window manipulation");
					//Window manipulation (from dtterm, as well as extensions). These controls may be disabled using the allowWindowOps resource. Valid values for the first (and any additional parameters) are:
					// 1 --> De-iconify window.
					// 2 --> Iconify window.
					// 3 ; x ; y --> Move window to [x, y].
					// 4 ; height ; width --> Resize the xterm window to height and width in pixels.
					// 5 --> Raise the xterm window to the front of the stacking order.
					// 6 --> Lower the xterm window to the bottom of the stacking order.
					// 7 --> Refresh the xterm window.
					// 8 ; height ; width --> Resize the text area to [height;width] in characters.
					// 9 ; 0 --> Restore maximized window.
					// 9 ; 1 --> Maximize window (i.e., resize to screen size).
					// 1 1 --> Report xterm window state. If the xterm window is open (non-iconified), it returns CSI 1 t . If the xterm window is iconified, it returns CSI 2 t .
					// 1 3 --> Report xterm window position as CSI 3 ; x; y t
					// 1 4 --> Report xterm window in pixels as CSI 4 ; height ; width t
					// 2 0 --> Report xterm window�s icon label as OSC L label ST
					// >= 2 4 --> Resize to P s lines (DECSLPP)
					DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
				}
			}
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break;

	case L'c':
		// echo -e "\e[>c"
		if ((Code.PvtLen == 1) && (Code.Pvt[0] == L'>')
			&& ((Code.ArgC < 1) || (Code.ArgV[0] == 0)))
		{
			// P s = 0 or omitted -> request the terminal's identification code.
			wchar_t szVerInfo[64];
			// this will be "ESC > 67 ; build ; 0 c"
			// 67 is ASCII code of 'C' (ConEmu, yeah)
			// Other terminals report examples: MinTTY -> 77, rxvt -> 82, screen -> 83
			// msprintf(szVerInfo, countof(szVerInfo), L"\x1B>%u;%u;0c", (int)'C', MVV_1*10000+MVV_2*100+MVV_3);
			// Emulate xterm version 136?
			wcscpy_c(szVerInfo, L"\x1B[>0;136;0c");
			ReportString(szVerInfo);
		}
		// echo -e "\e[c"
		else if ((Code.ArgC < 1) || (Code.ArgV[0] == 0))
		{
			// Report "VT100 with Advanced Video Option"
			ReportString(L"\x1B[?1;2c");
		}
		else
		{
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		break;

	case L'X':
		// CSI P s X:  Erase P s Character(s) (default = 1) (ECH)
		if (GetConsoleScreenBufferInfoCached(hConsoleOutput, &csbi))
		{
			if (lbApply)
			{
				// Apply default color before scrolling!
				ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
				lbApply = FALSE;
			}

			int nCount = (Code.ArgC > 0) ? Code.ArgV[0] : 1;
			int nScreenLeft = csbi.dwSize.X - csbi.dwCursorPosition.X - 1 + (csbi.dwSize.X * (csbi.dwSize.Y - csbi.dwCursorPosition.Y - 1));
			int nChars = std::min(nCount,nScreenLeft);
			COORD cr0 = csbi.dwCursorPosition;

			if (nChars > 0)
			{
				ExtFillOutputParm fill = { sizeof(fill), efof_Current | efof_Attribute | efof_Character,
					hConsoleOutput, {}, L' ', cr0, static_cast<DWORD>(nChars) };
				ExtFillOutput(&fill);
			}
		} // case L'X':
		break;

	default:
		DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
	} // switch (Code.Action)
}

void CEAnsi::WriteAnsiCode_OSC(OnWriteConsoleW_t writeConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply)
{
	if (!Code.ArgSZ)
	{
		DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		return;
	}

	// Finalizing (ST) with "\x1B\\" or "\x07"
	//ESC ] 0 ; txt ST        Set icon name and window title to txt.
	//ESC ] 1 ; txt ST        Set icon name to txt.
	//ESC ] 2 ; txt ST        Set window title to txt.
	//ESC ] 4 ; num; txt ST   Set ANSI color num to txt.
	//ESC ] 9 ... ST          ConEmu specific
	//ESC ] 10 ; txt ST       Set dynamic text color to txt.
	//ESC ] 4 6 ; name ST     Change log file to name (normally disabled
	//					      by a compile-time option)
	//ESC ] 5 0 ; fn ST       Set font to fn.

	switch (*Code.ArgSZ)
	{
	case L'0':
	case L'1':
	case L'2':
		if (Code.ArgSZ[1] == L';' && Code.ArgSZ[2])
		{
			CEStr pszNewTitle;
			if (pszNewTitle.GetBuffer(Code.cchArgSZ))
			{
				EscCopyCtrlString(pszNewTitle.data(), Code.ArgSZ + 2, Code.cchArgSZ - 2);
				SetConsoleTitle(pszNewTitle.c_str(gsInitConTitle));
			}
		}
		break;

	case L'4':
		// TODO: the following is suggestion for exact palette colors
		// TODO: but we are using standard xterm palette or truecolor 24bit palette
		_ASSERTEX(Code.ArgSZ[1] == L';');
		break;

	case L'9':
		// ConEmu specific
		// ESC ] 9 ; 1 ; ms ST           Sleep. ms - milliseconds
		// ESC ] 9 ; 2 ; "txt" ST        Show GUI MessageBox ( txt ) for dubug purposes
		// ESC ] 9 ; 3 ; "txt" ST        Set TAB text
		// ESC ] 9 ; 4 ; st ; pr ST      When _st_ is 0: remove progress. When _st_ is 1: set progress value to _pr_ (number, 0-100).
		//                               When _st_ is 2: set error state in progress on Windows 7 taskbar, _pr_ is optional.
		//                               When _st_ is 3: set indeterminate state. When _st_ is 4: set paused state, _pr_ is optional.
		// ESC ] 9 ; 5 ST                Wait for ENTER/SPACE/ESC. Set EnvVar "ConEmuWaitKey" to ENTER/SPACE/ESC on exit.
		// ESC ] 9 ; 6 ; "txt" ST        Execute GuiMacro. Set EnvVar "ConEmuMacroResult" on exit.
		// ESC ] 9 ; 7 ; "cmd" ST        Run some process with arguments
		// ESC ] 9 ; 8 ; "env" ST        Output value of environment variable
		// ESC ] 9 ; 9 ; "cwd" ST        Inform ConEmu about shell current working directory
		// ESC ] 9 ; 10 ; p ST           Request xterm keyboard/output emulation
		// ESC ] 9 ; 11; "*txt*" ST      Just a ‘comment’, skip it.
		// ESC ] 9 ; 12 ST               Let ConEmu treat current cursor position as prompt start. Useful with `PS1`.
		if (Code.ArgSZ[1] == L';')
		{
			if (Code.ArgSZ[2] == L'1')
			{
				if (Code.ArgSZ[3] == L';')
				{
					// ESC ] 9 ; 1 ; ms ST
					DoSleep(Code.ArgSZ+4);
				}
				else if (Code.ArgC >= 2 && Code.ArgV[1] == 10)
				{
					// ESC ] 9 ; 10 ST
					// ESC ] 9 ; 10 ; 1 ST
					if (!gbIsXTermOutput && (Code.ArgC == 2 || Code.ArgV[2] == 1))
					{
						DBG_XTERM(L"xTermOutput=ON due ESC]9;10;1ST");
						DBG_XTERM(L"AutoLfNl=OFF due ESC]9;10;1ST");
						DBG_XTERM(L"term=XTerm due ESC]9;10;1ST");
						CEAnsi::StartXTermMode(true);
					}
					// ESC ] 9 ; 10 ; 0 ST
					else if (Code.ArgC >= 3 || Code.ArgV[2] == 0)
					{
						DBG_XTERM(L"xTermOutput=OFF due ESC]9;10;0ST");
						DBG_XTERM(L"AutoLfNl=ON due ESC]9;10;0ST");
						DBG_XTERM(L"term=Win32 due ESC]9;10;0ST");
						CEAnsi::StartXTermMode(false);
					}
					// ESC ] 9 ; 10 ; 3 ST
					else if (Code.ArgC >= 3 || Code.ArgV[2] == 3)
					{
						DBG_XTERM(L"xTermOutput=ON due ESC]9;10;3ST");
						DBG_XTERM(L"AutoLfNl=OFF due ESC]9;10;3ST");
						CEAnsi::StartXTermOutput(true);
					}
					// ESC ] 9 ; 10 ; 2 ST
					else if (Code.ArgC >= 3 || Code.ArgV[2] == 2)
					{
						DBG_XTERM(L"xTermOutput=OFF due ESC]9;10;2ST");
						DBG_XTERM(L"AutoLfNl=ON due ESC]9;10;2ST");
						CEAnsi::StartXTermOutput(false);
					}
				}
				else if (Code.ArgSZ[3] == L'1' && Code.ArgSZ[4] == L';')
				{
					// ESC ] 9 ; 11; "*txt*" ST - Just a ‘comment’, skip it.
					DumpKnownEscape(Code.ArgSZ+5, lstrlen(Code.ArgSZ+5), de_Comment);
				}
				else if (Code.ArgSZ[3] == L'2')
				{
					// ESC ] 9 ; 12 ST
					StorePromptBegin();
				}
			}
			else if (Code.ArgSZ[2] == L'2' && Code.ArgSZ[3] == L';')
			{
				DoMessage(Code.ArgSZ+4, Code.cchArgSZ - 4);
			}
			else if (Code.ArgSZ[2] == L'3' && Code.ArgSZ[3] == L';')
			{
				CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SETTABTITLE, sizeof(CESERVER_REQ_HDR)+sizeof(wchar_t)*(Code.cchArgSZ));
				if (pIn)
				{
					EscCopyCtrlString(reinterpret_cast<wchar_t*>(pIn->wData), Code.ArgSZ+4, Code.cchArgSZ-4);
					CESERVER_REQ* pOut = ExecuteGuiCmd(ghConWnd, pIn, ghConWnd);
					ExecuteFreeResult(pIn);
					ExecuteFreeResult(pOut);
				}
			}
			else if (Code.ArgSZ[2] == L'4')
			{
				AnsiProgressStatus st = AnsiProgressStatus::None;
				WORD pr = 0;
				const wchar_t* pszName = nullptr;
				if (Code.ArgSZ[3] == L';')
				{
					switch (Code.ArgSZ[4])
					{
					case L'0':
						break;
					case L'1':
						st = AnsiProgressStatus::Running; break;
					case L'2':
						st = AnsiProgressStatus::Error; break;
					case L'3':
						st = AnsiProgressStatus::Indeterminate; break;
					case L'4':
						st = AnsiProgressStatus::Paused; break;
					case L'5': // reserved for future use
						st = AnsiProgressStatus::LongRunStart; break;
					case L'6': // reserved for future use
						st = AnsiProgressStatus::LongRunStop; break;
					}
					if (st == AnsiProgressStatus::Running || st == AnsiProgressStatus::Error || st == AnsiProgressStatus::Paused)
					{
						if (Code.ArgSZ[5] == L';')
						{
							LPCWSTR pszValue = Code.ArgSZ + 6;
							pr = NextNumber(pszValue);
						}
					}
					if (st == AnsiProgressStatus::LongRunStart || st == AnsiProgressStatus::LongRunStop)
					{
						pszName = (Code.ArgSZ[5] == L';') ? (Code.ArgSZ + 6) : nullptr;
					}
				}
				GuiSetProgress(st, pr, pszName);
			}
			else if (Code.ArgSZ[2] == L'5')
			{
				//int s = 0;
				//if (Code.ArgSZ[3] == L';')
				//	s = NextNumber(Code.ArgSZ+4);
				BOOL bSucceeded = FALSE;
				DWORD nRead = 0;
				INPUT_RECORD r = {};
				// ReSharper disable once CppLocalVariableMayBeConst
				HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
				//DWORD nStartTick = GetTickCount();
				while (((bSucceeded = ReadConsoleInput(hIn, &r, 1, &nRead))) && nRead)
				{
					if ((r.EventType == KEY_EVENT) && r.Event.KeyEvent.bKeyDown)
					{
						if ((r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
							|| (r.Event.KeyEvent.wVirtualKeyCode == VK_SPACE)
							|| (r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE))
						{
							break;
						}
					}
				}
				if (bSucceeded && ((r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN)
							|| (r.Event.KeyEvent.wVirtualKeyCode == VK_SPACE)
							|| (r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE)))
				{
					SetEnvironmentVariable(ENV_CONEMU_WAITKEY_W,
						(r.Event.KeyEvent.wVirtualKeyCode == VK_RETURN) ? L"RETURN" :
						(r.Event.KeyEvent.wVirtualKeyCode == VK_SPACE)  ? L"SPACE" :
						(r.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE) ? L"ESC" :
						L"???");
				}
				else
				{
					SetEnvironmentVariable(ENV_CONEMU_WAITKEY_W, L"");
				}
			}
			else if (Code.ArgSZ[2] == L'6' && Code.ArgSZ[3] == L';')
			{
				DoGuiMacro(Code.ArgSZ+4, Code.cchArgSZ - 4);
			}
			else if (Code.ArgSZ[2] == L'7' && Code.ArgSZ[3] == L';')
			{
				DoProcess(Code.ArgSZ+4, Code.cchArgSZ - 4);
			}
			else if (Code.ArgSZ[2] == L'8' && Code.ArgSZ[3] == L';')
			{
				if (lbApply)
				{
					ReSetDisplayParm(hConsoleOutput, FALSE, TRUE);
					lbApply = FALSE;
				}
				DoPrintEnv(Code.ArgSZ+4, Code.cchArgSZ - 4);
			}
			else if (Code.ArgSZ[2] == L'9' && Code.ArgSZ[3] == L';')
			{
				DoSendCWD(Code.ArgSZ+4, Code.cchArgSZ - 4);
			}
		}
		break;

	default:
		DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
	}
}

void CEAnsi::WriteAnsiCode_VIM(OnWriteConsoleW_t writeConsoleW, HANDLE hConsoleOutput, AnsiEscCode& Code, BOOL& lbApply)
{
	if (!gbIsXTermOutput && !gnWriteProcessed)
	{
		DBG_XTERM(L"xTermOutput=ON due Vim start");
		DBG_XTERM(L"AutoLfNl=OFF due Vim start");
		DBG_XTERM(L"term=XTerm due Vim start");
		CEAnsi::StartXTermMode(true);
	}

	switch (Code.Action)
	{
	case L'm':
		// Set xterm display modes (colors, fonts, etc.)
		if (!Code.ArgC)
		{
			//ReSetDisplayParm(hConsoleOutput, TRUE, FALSE);
			DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
		}
		else
		{
			for (int i = 0; i < Code.ArgC; i++)
			{
				switch (Code.ArgV[i])
				{
				case 7:
					gDisplayParm.setBrightOrBold(FALSE);
					gDisplayParm.setItalic(FALSE);
					gDisplayParm.setUnderline(FALSE);
					gDisplayParm.setBrightFore(FALSE);
					gDisplayParm.setBrightBack(FALSE);
					gDisplayParm.setInverse(FALSE);
					break;
				case 15:
					gDisplayParm.setBrightOrBold(TRUE);
					break;
				case 112:
					gDisplayParm.setInverse(TRUE);
					break;
				case 143:
					// What is this?
					break;
				default:
					DumpUnknownEscape(Code.pszEscStart,Code.nTotalLen);
				}
			}
		}
		break; // "|...m"
	}
}

/// Returns coordinates of either working area (viewPort) or full backscroll buffer
/// @param hConsoleOutput   if nullptr we process STD_OUTPUT_HANDLE
/// @param viewPort         if true - returns srWindow, false - full backscroll buffer
/// @result valid SMALL_RECT
SMALL_RECT CEAnsi::GetWorkingRegion(HANDLE hConsoleOutput, bool viewPort) const
{
	const short kFallback = 9999;
	SMALL_RECT region = {0, 0, kFallback, kFallback};
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hConOut = hConsoleOutput ? hConsoleOutput : GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	// If function fails, we return {kFallback, kFallback}
	if (GetConsoleScreenBufferInfoCached(hConOut, &csbi, TRUE))
	{
		if (viewPort)
		{
			// Trick to avoid overflow of SHORT
			region = MakeSmallRect(
				std::min<WORD>(static_cast<WORD>(csbi.srWindow.Left), std::numeric_limits<SHORT>::max()),
				std::min<WORD>(static_cast<WORD>(csbi.srWindow.Top), std::numeric_limits<SHORT>::max()),
				std::min<WORD>(static_cast<WORD>(csbi.srWindow.Right), std::numeric_limits<SHORT>::max()),
				std::min<WORD>(static_cast<WORD>(csbi.srWindow.Bottom), std::numeric_limits<SHORT>::max())
			);
		}
		else
		{
			region = MakeSmallRect(
				0,
				0,
				std::min<WORD>(static_cast<WORD>(csbi.dwSize.X - 1), std::numeric_limits<SHORT>::max()),
				std::min<WORD>(static_cast<WORD>(csbi.dwSize.Y - 1), std::numeric_limits<SHORT>::max())
			);
		}
		// Last checks
		if (region.Right < region.Left)
			region.Right = region.Left;
		if (region.Bottom < region.Top)
			region.Bottom = region.Top;
	}
	return region;
}

void CEAnsi::SetScrollRegion(bool bRegion, bool bRelative, int nStart, int nEnd, HANDLE hConsoleOutput) const
{
	if (bRegion && (nStart >= 0) && (nStart <= nEnd))
	{
		// note: the '\e[0;35r' shall be treated as '\e[1;35r'
		_ASSERTE(nStart >= 0 && nEnd >= nStart);
		const SMALL_RECT working = GetWorkingRegion(hConsoleOutput, bRelative);
		_ASSERTE(working.Top>=0 && working.Left>=0 && working.Bottom>=working.Top && working.Right>=working.Left);
		if (bRelative)
		{
			nStart += working.Top;
			nEnd += working.Top;
		}
		// We need 0-based absolute values
		gDisplayOpt.ScrollStart = static_cast<SHORT>(std::max<int>(working.Top, std::min<int>(nStart - 1, working.Bottom)));
		gDisplayOpt.ScrollEnd = static_cast<SHORT>(std::max<int>(working.Top, std::min<int>(nEnd - 1, working.Bottom)));
		// Validate
		if (!((gDisplayOpt.ScrollRegion = (gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart))))
		{
			_ASSERTEX(gDisplayOpt.ScrollStart>=0 && gDisplayOpt.ScrollEnd>=gDisplayOpt.ScrollStart);
		}
		wchar_t szLog[40]; msprintf(szLog, countof(szLog), L"{%i,%i}", gDisplayOpt.ScrollStart, gDisplayOpt.ScrollEnd);
	}
	else
	{
		gDisplayOpt.ScrollRegion = FALSE;
		DumpKnownEscape(L"<Disabled>", 10, de_ScrollRegion);
	}
}

void CEAnsi::XTermSaveRestoreCursor(bool bSaveCursor, HANDLE hConsoleOutput /*= nullptr*/)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hConOut = hConsoleOutput ? hConsoleOutput : GetStdHandle(STD_OUTPUT_HANDLE);

	if (bSaveCursor)
	{
		// Save cursor position (can not be nested)
		if (GetConsoleScreenBufferInfoCached(hConOut, &csbi))
		{
			gDisplayCursor.StoredCursorPos = csbi.dwCursorPosition;
			gDisplayCursor.bCursorPosStored = TRUE;
		}
	}
	else
	{
		// Restore cursor position
		SetConsoleCursorPosition(hConOut, gDisplayCursor.StoredCursorPos);
	}
}

HANDLE CEAnsi::XTermBufferConEmuAlternative()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi1 = {}, csbi2 = {};
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
	ghStdOut.SetHandle(hOut, MConHandle::StdMode::Output);
	ghStdErr.SetHandle(hErr, MConHandle::StdMode::Output);
	if (GetConsoleScreenBufferInfoCached(hOut, &csbi1, TRUE))
	{
		// -- Turn on "alternative" buffer even if not scrolling exist now
		//if (csbi.dwSize.Y > (csbi.srWindow.Bottom - csbi.srWindow.Top + 1))
		{
			CESERVER_REQ *pIn = nullptr, *pOut = nullptr;
			pIn = ExecuteNewCmd(CECMD_ALTBUFFER,sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ALTBUFFER));
			if (pIn)
			{
				pIn->AltBuf.AbFlags = abf_BufferOff | abf_SaveContents;
				if (isConnectorStarted())
					pIn->AltBuf.AbFlags |= abf_Connector;
				// support "virtual" dynamic console buffer height
				if (CESERVER_CONSOLE_APP_MAPPING* pAppMap = gpAppMap ? gpAppMap->Ptr() : nullptr)
					pIn->AltBuf.BufferHeight = std::max(static_cast<SHORT>(pAppMap->nLastConsoleRow), csbi1.srWindow.Bottom);
				else
					pIn->AltBuf.BufferHeight = csbi1.srWindow.Bottom;
				pOut = ExecuteSrvCmd(gnServerPID, pIn, ghConWnd);
				if (pOut)
				{
					if (!IsWin7Eql())
					{
						ghConOut.Close();
						// ReSharper disable once CppLocalVariableMayBeConst
						HANDLE hNewOut = ghConOut.GetHandle();
						if (hNewOut && hNewOut != INVALID_HANDLE_VALUE)
						{
							hOut = hNewOut;
							SetStdHandle(STD_OUTPUT_HANDLE, hNewOut);
							SetStdHandle(STD_ERROR_HANDLE, hNewOut);
						}
					}

					// Ensure we have fresh information (size was changed)
					GetConsoleScreenBufferInfoCached(hOut, &csbi2, TRUE);

					// Clear current screen contents, don't move cursor position
					const DWORD nChars = csbi2.dwSize.X * csbi2.dwSize.Y;
					ExtFillOutputParm fill = {sizeof(fill), efof_Current|efof_Attribute|efof_Character,
						hOut, {}, L' ', {}, static_cast<DWORD>(nChars) };
					ExtFillOutput(&fill);

					TODO("BufferWidth");
					if (!(gXTermAltBuffer.Flags & xtb_AltBuffer))
					{
						// Backscroll length
						gXTermAltBuffer.BufferSize = (csbi1.dwSize.Y > (csbi1.srWindow.Bottom - csbi1.srWindow.Top + 1))
							? csbi1.dwSize.Y : 0;
						gXTermAltBuffer.Flags = xtb_AltBuffer;
						// Stored cursor pos
						if (gDisplayCursor.bCursorPosStored)
						{
							gXTermAltBuffer.CursorPos = gDisplayCursor.StoredCursorPos;
							gXTermAltBuffer.Flags |= xtb_StoredCursor;
						}
						// Stored scroll region
						if (gDisplayOpt.ScrollRegion)
						{
							gXTermAltBuffer.ScrollStart = gDisplayOpt.ScrollStart;
							gXTermAltBuffer.ScrollEnd = gDisplayOpt.ScrollEnd;
							gXTermAltBuffer.Flags |= xtb_StoredScrollRegion;
						}
					}
				}
				ExecuteFreeResult(pIn);
				ExecuteFreeResult(pOut);
			}
		}
	}
	return hOut;
}

HANDLE CEAnsi::XTermBufferConEmuPrimary()
{
	// Сброс расширенных атрибутов!
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleScreenBufferInfoCached(hOut, &csbi, TRUE))
	{
		const WORD nDefAttr = GetDefaultTextAttr();
		// Сброс только расширенных атрибутов
		ExtFillOutputParm fill = {sizeof(fill), /*efof_ResetExt|*/efof_Attribute/*|efof_Character*/, hOut,
			{ConEmu::ColorFlags::None, CONFORECOLOR(nDefAttr), CONBACKCOLOR(nDefAttr)},
			L' ', {0,0}, static_cast<DWORD>(csbi.dwSize.X * csbi.dwSize.Y)};
		ExtFillOutput(&fill);
		CEAnsi* pObj = CEAnsi::Object();
		if (pObj)
			pObj->ReSetDisplayParm(hOut, TRUE, TRUE);
		else
			SetConsoleTextAttribute(hOut, nDefAttr);
	}

	if (!IsWin7Eql() && ghStdOut.HasHandle())
	{
		SetStdHandle(STD_OUTPUT_HANDLE, ghStdOut);
		SetStdHandle(STD_ERROR_HANDLE, ghStdErr);
		hOut = ghStdOut.Release();
		ghConOut.Close();
	}

	// restore backscroll size and data
	CESERVER_REQ *pIn = nullptr, *pOut = nullptr;
	pIn = ExecuteNewCmd(CECMD_ALTBUFFER,sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_ALTBUFFER));
	if (pIn)
	{
		TODO("BufferWidth");
		pIn->AltBuf.AbFlags = abf_BufferOn | abf_RestoreContents;
		if (!gXTermAltBuffer.BufferSize)
			pIn->AltBuf.AbFlags |= abf_BufferOff;
		if (isConnectorStarted())
			pIn->AltBuf.AbFlags |= abf_Connector;
		pIn->AltBuf.BufferHeight = static_cast<USHORT>(gXTermAltBuffer.BufferSize);
		// Async - is not allowed. Otherwise current application (cmd.exe for example)
		// may start printing before server finishes buffer restoration
		pOut = ExecuteSrvCmd(gnServerPID, pIn, ghConWnd);
		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
		// Restore saved states
		if ((gDisplayCursor.bCursorPosStored = !!(gXTermAltBuffer.Flags & xtb_StoredCursor)))
		{
			gDisplayCursor.StoredCursorPos = gXTermAltBuffer.CursorPos;
		}
		if ((gDisplayOpt.ScrollRegion = !!(gXTermAltBuffer.Flags & xtb_StoredScrollRegion)))
		{
			gDisplayOpt.ScrollStart = gXTermAltBuffer.ScrollStart;
			gDisplayOpt.ScrollEnd = gXTermAltBuffer.ScrollEnd;
		}
	}
	return hOut;
}

#if 0
HANDLE CEAnsi::XTermBufferWin10(const int mode, const bool bSetAltBuffer)
{
	ORIGINAL_KRNL(WriteFile);
	const HANDLE std_out = GetStdHandle(STD_OUTPUT_HANDLE);
	char ansi_seq[32];
	sprintf_c(ansi_seq, "\x1b[?%i%c", mode, bSetAltBuffer ? 'h' : 'l');
	const DWORD write_len = static_cast<DWORD>(strlen(ansi_seq));
	DWORD written = 0;
	const auto rc = F(WriteFile)(std_out, ansi_seq, write_len, &written, nullptr);
	_ASSERTEX(rc && written == write_len);
	return std_out;
}
#endif

/// Change current buffer
/// Alternative buffer in XTerm is used to "fullscreen"
/// applications like Vim. There is no scrolling and this
/// mode is used to save current backscroll contents and
/// restore it when application exits
HANDLE CEAnsi::XTermAltBuffer(const bool bSetAltBuffer, const int mode)
{
	if (bSetAltBuffer)
	{
		// Once!
		if ((gXTermAltBuffer.Flags & xtb_AltBuffer))
			return GetStdHandle(STD_OUTPUT_HANDLE);

		#if 0
		// Can utilize Conhost V2 ANSI?
		if (gbIsSshProcess && IsWin10())
		{
			gXTermAltBuffer.Flags |= xtb_AltBuffer;
			return XTermBufferWin10(mode, bSetAltBuffer);
		}
		#endif

		return XTermBufferConEmuAlternative();
	}
	else
	{
		if (!(gXTermAltBuffer.Flags & xtb_AltBuffer))
			return GetStdHandle(STD_OUTPUT_HANDLE);

		// Once!
		gXTermAltBuffer.Flags &= ~xtb_AltBuffer;

		#if 0
		// Can utilize Conhost V2 ANSI?
		if (gbIsSshProcess && IsWin10())
			return XTermBufferWin10(mode, bSetAltBuffer);
		#endif

		return XTermBufferConEmuPrimary();
	}
}

/*
ViM need some hacks in current ConEmu versions
This is because
1) 256 colors mode requires NO scroll buffer
2) can't find ATM legal way to switch Alternative/Primary buffer by request from ViM
*/

HANDLE CEAnsi::StartVimTerm(bool bFromDllStart)
{
	// Only certain versions of Vim able to use xterm-256 in ConEmu
	if (gnExeFlags & (caf_Cygwin1|caf_Msys1|caf_Msys2))
		return GetStdHandle(STD_OUTPUT_HANDLE);

	// For native vim - don't handle "--help" and "--version" switches
	// Has no sense for cygwin/msys, but they are skipped above
	CmdArg lsArg;
	LPCWSTR pszCmdLine = GetCommandLineW();
	LPCWSTR pszCompare[] = {L"--help", L"-h", L"--version", nullptr};
	while ((pszCmdLine = NextArg(pszCmdLine, lsArg)))
	{
		for (INT_PTR i = 0; pszCompare[i]; i++)
		{
			if (wcscmp(lsArg, pszCompare[i]) == 0)
				return GetStdHandle(STD_OUTPUT_HANDLE);
		}
	}

	return XTermAltBuffer(true);
}

HANDLE CEAnsi::StopVimTerm()
{
	if (gbIsXTermOutput)
	{
		DBG_XTERM(L"xTermOutput=OFF due Vim stop");
		DBG_XTERM(L"AutoLfNl=ON due Vim stop");
		DBG_XTERM(L"term=Win32 due Vim stop");
		CEAnsi::StartXTermMode(false);
	}

	return XTermAltBuffer(false);
}

void CEAnsi::InitTermMode()
{
	bool needSetXterm = false;

	if (IsWin10())
	{
		const MHandle hOut{ GetStdHandle(STD_OUTPUT_HANDLE) };
		gPrevConOutMode = 0;
		if (GetConsoleMode(hOut, &gPrevConOutMode))
		{
			if (gPrevConOutMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING)
			{
				needSetXterm = true;
			}
		}
	}

	if (gbIsVimProcess)
	{
		StartVimTerm(true);
		_ASSERTE(CEAnsi::gbIsXTermOutput == true);
	}

	if (needSetXterm)
	{
		DBG_XTERM(L"xTermOutput=ON due ENABLE_VIRTUAL_TERMINAL_PROCESSING in InitTermMode");
		DBG_XTERM(L"AutoLfNl=OFF due ENABLE_VIRTUAL_TERMINAL_PROCESSING in InitTermMode");
		DBG_XTERM(L"term=XTerm due ENABLE_VIRTUAL_TERMINAL_PROCESSING in InitTermMode");
		StartXTermMode(true);
	}

	if (CEAnsi::gbIsXTermOutput && gPrevConOutMode)
	{
		const bool autoLfNl = (gPrevConOutMode & DISABLE_NEWLINE_AUTO_RETURN) == 0;
		if (CEAnsi::IsAutoLfNl() != autoLfNl)
		{
			CEAnsi::SetAutoLfNl(autoLfNl);
		}
	}
}

void CEAnsi::DoneTermMode()
{
	if (gbIsXTermOutput && (gPrevConOutMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING))
	{
		// If XTerm was enabled already before process start, no need to disable it
		DBG_XTERM(L"xTermOutput=OFF due previous !ENABLE_VIRTUAL_TERMINAL_PROCESSING in DoneTermMode");
		SetIsXTermOutput(false); // just reset
	}

	if (gbIsVimProcess)
	{
		DBG_XTERM(L"StopVimTerm in DoneTermMode");
		StopVimTerm();
	}
	else if (CEAnsi::gbIsXTermOutput)
	{
		DBG_XTERM(L"xTermOutput=OFF in DoneTermMode");
		DBG_XTERM(L"AutoLfNl=ON in DoneTermMode");
		DBG_XTERM(L"term=Win32 in DoneTermMode");
		StartXTermMode(false);
	}
}

void CEAnsi::ChangeTermMode(TermModeCommand mode, DWORD value, DWORD nPID /*= 0*/)
{
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_STARTXTERM, sizeof(CESERVER_REQ_HDR) + 3 * sizeof(DWORD));
	if (pIn)
	{
		pIn->dwData[0] = mode;
		pIn->dwData[1] = value;  // NOLINT(clang-diagnostic-array-bounds)
		pIn->dwData[2] = nPID ? nPID : GetCurrentProcessId();  // NOLINT(clang-diagnostic-array-bounds)
		CESERVER_REQ* pOut = ExecuteSrvCmd(gnServerPID, pIn, ghConWnd);
		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
	}

	if (mode < countof(gWasXTermModeSet))
	{
		gWasXTermModeSet[mode] = { value, nPID ? nPID : GetCurrentProcessId() };
	}
}

void CEAnsi::StartXTermMode(const bool bStart)
{
	// May be triggered by connector, official Vim builds, ENABLE_VIRTUAL_TERMINAL_INPUT, "ESC ] 9 ; 10 ; 1 ST"
	_ASSERTEX(gbIsXTermOutput != bStart);

	StartXTermOutput(bStart);

	// Remember last mode and pass to server
	ChangeTermMode(tmc_TerminalType, bStart ? te_xterm : te_win32);
}

void CEAnsi::SetIsXTermOutput(const bool value)
{
	if (gbIsXTermOutput != value)
	{
		gbIsXTermOutput = value;
	}
}

void CEAnsi::DebugXtermOutput(const wchar_t* message)
{
#ifdef _DEBUG
	wchar_t dbgOut[512];
	msprintf(dbgOut, countof(dbgOut), L"XTerm: %s PID=%u TID=%u: %s\n",
		gsExeName, GetCurrentProcessId(), GetCurrentThreadId(), message);
	OutputDebugStringW(dbgOut);
#endif
}

void CEAnsi::StartXTermOutput(const bool bStart)
{
	// Remember last mode
	SetIsXTermOutput(bStart);
	// Set AutoLfNl according to mode
	SetAutoLfNl(bStart ? false : true);
}

void CEAnsi::RefreshXTermModes()
{
	if (!gbIsXTermOutput)
		return;
	for (int i = 0; i < static_cast<int>(countof(gWasXTermModeSet)); ++i)
	{
		if (!gWasXTermModeSet[i].pid)
			continue;
		_ASSERTE(i != tmc_ConInMode);
		ChangeTermMode(static_cast<TermModeCommand>(i), gWasXTermModeSet[i].value, gWasXTermModeSet[i].pid);
	}
}

void CEAnsi::SetAutoLfNl(const bool autoLfNl)
{
	if (static_cast<bool>(gDisplayOpt.AutoLfNl) != autoLfNl)
	{
		gDisplayOpt.AutoLfNl = autoLfNl;
	}
}

bool CEAnsi::IsAutoLfNl()
{
	// #TODO check for gbIsXTermOutput?
	return gDisplayOpt.AutoLfNl;
}

// This is useful when user press Shift+Home,
// we'll select only "typed command" part, without "prompt"
void CEAnsi::StorePromptBegin()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleScreenBufferInfo(hConOut, &csbi))
	{
		OnReadConsoleBefore(hConOut, csbi);
	}
}

void CEAnsi::StorePromptReset()
{
	CESERVER_CONSOLE_APP_MAPPING* pAppMap = gpAppMap ? gpAppMap->Ptr() : nullptr;
	if (pAppMap)
	{
		pAppMap->csbiPreRead.dwCursorPosition = COORD{0, 0};
	}
}


//static, thread local singleton
CEAnsi* CEAnsi::Object()
{
	CLastErrorGuard errGuard;

	if (!gAnsiTlsIndex)
	{
		gAnsiTlsIndex = TlsAlloc();
	}

	if ((!gAnsiTlsIndex) || (gAnsiTlsIndex == TLS_OUT_OF_INDEXES))
	{
		_ASSERTEX(gAnsiTlsIndex && gAnsiTlsIndex != TLS_OUT_OF_INDEXES);
		return nullptr;
	}

	// Don't use thread_local, it doesn't work in WinXP in dynamically loaded dlls
	CEAnsi* obj = static_cast<CEAnsi*>(TlsGetValue(gAnsiTlsIndex));
	if (!obj)
	{
		obj = new CEAnsi;
		if (obj)
			obj->GetDefaultTextAttr(); // Initialize "default attributes"
		TlsSetValue(gAnsiTlsIndex, obj);
	}

	return obj;
}

//static
void CEAnsi::Release()
{
	if (gAnsiTlsIndex && (gAnsiTlsIndex != TLS_OUT_OF_INDEXES))
	{
		CEAnsi* p = static_cast<CEAnsi*>(TlsGetValue(gAnsiTlsIndex));
		if (p)
		{
			if (IsHeapInitialized())
			{
				delete p;
			}
			TlsSetValue(gAnsiTlsIndex, nullptr);
		}
	}
}
