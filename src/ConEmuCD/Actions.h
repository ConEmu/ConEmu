
/*
Copyright (c) 2016-present Maximus5
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
#include "GuiMacro.h"

enum class ConEmuStateCheck
{
	None = 0,
	IsConEmu,
	IsTerm,
	IsAnsi,
	IsAdmin,
	IsRedirect,
};

enum class ConEmuExecAction
{
	None = 0,
	RegConFont, // RegisterConsoleFontHKLM
	InjectHooks,
	InjectRemote,
	InjectDefTrm,
	GuiMacro,
	CheckUnicodeFont,
	PrintConsoleInfo,
	TestUnicodeCvt,
	OsVerInfo,
	ExportCon,  // export env.vars to processes of active console
	ExportTab,  // ea_ExportCon + ConEmu window
	ExportGui,  // export env.vars to ConEmu window
	ExportAll,  // export env.vars to all opened tabs of current ConEmu window
	ParseArgs,  // debug test of NextArg function... print args to STDOUT
	ErrorLevel, // return specified errorlevel
	OutEcho,    // echo "string" with ANSI processing
	OutType,    // print file contents with ANSI processing
	StoreCWD,   // store current console work dir
	DumpStruct, // dump file mapping contents
	ConPty,    // host a child process through Windows pseudoconsole
};

int  DoExecAction(ConEmuExecAction eExecAction, LPCWSTR asCmdArg /* rest of cmdline */, MacroInstance& Inst);
int  DoExportEnv(LPCWSTR asCmdArg, ConEmuExecAction eExecAction, bool bSilent = false);
int  DoOutput(ConEmuExecAction eExecAction, LPCWSTR asCmdArg);
bool DoStateCheck(ConEmuStateCheck eStateCheck);

int  WriteOutput(LPCWSTR pszText, DWORD cchLen = (DWORD)-1, DWORD* pdwWritten = nullptr, bool bProcessed = false, bool bAsciiPrint = false, bool bStreamBy1 = false, bool bToBottom = false);
