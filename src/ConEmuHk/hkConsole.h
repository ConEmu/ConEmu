
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

#pragma once

#include "../common/defines.h"
#include "SetHook.h"

#include <windows.h>

struct MY_CONSOLE_SCREEN_BUFFER_INFOEX;
struct MY_CONSOLE_FONT_INFOEX;

/* *************************** */

#if __GNUC__
extern "C" {
#ifndef GetConsoleAliases
	DWORD WINAPI GetConsoleAliasesA(LPSTR AliasBuffer, DWORD AliasBufferLength, LPSTR ExeName);
	DWORD WINAPI GetConsoleAliasesW(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName);
#define GetConsoleAliases  GetConsoleAliasesW
#endif
}
#endif

/* *************************** */

HOOK_PROTOTYPE(AllocConsole,BOOL,WINAPI,(void));
HOOK_PROTOTYPE(CreateConsoleScreenBuffer,HANDLE,WINAPI,(DWORD dwDesiredAccess, DWORD dwShareMode, const SECURITY_ATTRIBUTES *lpSecurityAttributes, DWORD dwFlags, LPVOID lpScreenBufferData));
HOOK_PROTOTYPE(FreeConsole,BOOL,WINAPI,(void));
HOOK_PROTOTYPE(GetConsoleAliasesW,DWORD,WINAPI,(LPWSTR AliasBuffer, DWORD AliasBufferLength, LPWSTR ExeName));
HOOK_PROTOTYPE(GetConsoleFontSize,COORD,WINAPI,(HANDLE hConsoleOutput, DWORD nFont));
HOOK_PROTOTYPE(GetConsoleWindow,HWND,WINAPI,(void));
HOOK_PROTOTYPE(GetConsoleProcessList,DWORD,WINAPI,(LPDWORD lpdwProcessList, DWORD dwProcessCount));
HOOK_PROTOTYPE(GetCurrentConsoleFont,BOOL,WINAPI,(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont));
HOOK_PROTOTYPE(GetLargestConsoleWindowSize,COORD,WINAPI,(HANDLE hConsoleOutput));
HOOK_PROTOTYPE(SetConsoleActiveScreenBuffer,BOOL,WINAPI,(HANDLE hConsoleOutput));
HOOK_PROTOTYPE(SetConsoleCP,BOOL,WINAPI,(UINT wCodePageID));
HOOK_PROTOTYPE(SetConsoleCursorInfo,BOOL,WINAPI,(HANDLE hConsoleOutput, const CONSOLE_CURSOR_INFO *lpConsoleCursorInfo));
HOOK_PROTOTYPE(SetConsoleCursorPosition,BOOL,WINAPI,(HANDLE hConsoleOutput, COORD dwCursorPosition));
HOOK_PROTOTYPE(SetConsoleKeyShortcuts,BOOL,WINAPI,(BOOL bSet, BYTE bReserveKeys, LPVOID p1, DWORD n1));
HOOK_PROTOTYPE(SetConsoleOutputCP,BOOL,WINAPI,(UINT wCodePageID));
HOOK_PROTOTYPE(SetConsoleScreenBufferInfoEx,BOOL,WINAPI,(HANDLE hConsoleOutput, MY_CONSOLE_SCREEN_BUFFER_INFOEX* lpConsoleScreenBufferInfoEx));
HOOK_PROTOTYPE(SetConsoleScreenBufferSize,BOOL,WINAPI,(HANDLE hConsoleOutput, COORD dwSize));
HOOK_PROTOTYPE(SetConsoleTitleA,BOOL,WINAPI,(LPCSTR lpConsoleTitle));
HOOK_PROTOTYPE(SetConsoleTitleW,BOOL,WINAPI,(LPCWSTR lpConsoleTitle));
HOOK_PROTOTYPE(SetConsoleWindowInfo,BOOL,WINAPI,(HANDLE hConsoleOutput, BOOL bAbsolute, const SMALL_RECT *lpConsoleWindow));
HOOK_PROTOTYPE(SetCurrentConsoleFontEx,BOOL,WINAPI,(HANDLE hConsoleOutput, BOOL bMaximumWindow, MY_CONSOLE_FONT_INFOEX* lpConsoleCurrentFontEx));
