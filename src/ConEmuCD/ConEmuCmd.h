
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
#include "WorkerBase.h"

class WorkerComspec final : public WorkerBase
{
public:
	virtual ~WorkerComspec();

	WorkerComspec();

	WorkerComspec(const WorkerComspec&) = delete;
	WorkerComspec(WorkerComspec&&) = delete;
	WorkerComspec& operator=(const WorkerComspec&) = delete;
	WorkerComspec& operator=(WorkerComspec&&) = delete;

	int Init() override;
	void Done(int exitCode, bool reportShutdown = false) override;

	int ProcessCommandLineArgs() override;

	int ProcessNewConsoleArg(LPCWSTR asCmdLine);

	bool IsCmdK() const override;
	void SetCmdK(bool useCmdK) override;

	bool SetConsoleSize(USHORT BufferHeight, COORD crNewSize, SMALL_RECT rNewRect, LPCSTR asLabel = nullptr, bool bForceWriteLog = false) override;

private:
	bool GetAliases(wchar_t* asExeName, wchar_t** rsAliases, LPDWORD rnAliasesSize) const;

	bool bK = false; // when started as "ConEmuC -k"
	bool bNewConsole = false;
	wchar_t szComSpecName[32] = L"";
	wchar_t szSelfName[32] = L"";
	wchar_t *pszPreAliases = nullptr;
	DWORD nPreAliasSize = 0;
};
