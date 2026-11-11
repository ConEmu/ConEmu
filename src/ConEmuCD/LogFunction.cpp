
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

#include "ConsoleMain.h"
#include "LogFunction.h"
#include "ConEmuSrv.h"
#include "ConsoleArgs.h"
#include <algorithm>
#include <windows.h>

namespace {
const wchar_t kFnBegin[] = L" [begin]";
const wchar_t kFnEnd[] = L" [end]";
}

std::atomic_int32_t CLogFunction::m_FnLevel{0}; // Simple, without per-thread devision

CLogFunction::CLogFunction(const char* asFnName)
{
	const int nLen = MultiByteToWideChar(CP_ACP, 0, asFnName, -1, NULL, 0);

	const int cchBuf = 80;
	wchar_t sBuf[cchBuf] = L"";
	CEStr szTemp;
	wchar_t *pszBuf = (nLen >= cchBuf)
		? szTemp.GetBuffer(nLen + 1)
		: sBuf;

	MultiByteToWideChar(CP_ACP, 0, asFnName, -1, pszBuf, nLen+1);

	DoLogFunction(pszBuf);
}

CLogFunction::CLogFunction(const wchar_t* asFnName)
{
	DoLogFunction(asFnName);
}

void CLogFunction::DoLogFunction(const wchar_t* asFnName)
{
	if (mb_Logged)
		return;

	const auto lLevel = std::min<int32_t>(++m_FnLevel, 20);
	mb_Logged = true;

	if (!gpLogSize && !(gpConsoleArgs && gpConsoleArgs->debugVerbose_.GetBool()))
		return;

	const int cchFnInfo = std::size(mc_FnInfo);
	wchar_t* pc = mc_FnInfo;
	for (int32_t l = 3 * (lLevel - 1); l > 0; --l)
		*(pc++) = L' ';
	*pc = 0;

	const int nPrefix = static_cast<int>(pc - mc_FnInfo);
	const int cchSpaceLeft = cchFnInfo - nPrefix;
	const int nFnLen = lstrlen(asFnName);
	const int nSufLen = lstrlen(kFnBegin);
	_ASSERTE(lstrlen(kFnBegin) > lstrlen(kFnEnd));
	_ASSERTE(cchSpaceLeft > 48);

	if ((nFnLen + nSufLen) < cchSpaceLeft)
	{
		lstrcpyn(pc, asFnName, cchSpaceLeft);
		mn_FnSuffix = nPrefix + nFnLen;
		pc += nFnLen;
		lstrcpyn(pc, kFnBegin, cchSpaceLeft - nFnLen);
		LogString(mc_FnInfo);
	}
	else
	{
		LogString(CEStr(mc_FnInfo, asFnName, kFnBegin));
		lstrcpyn(pc, asFnName, cchSpaceLeft);
	}
}

CLogFunction::~CLogFunction()
{
	if (!mb_Logged)
		return;
	--m_FnLevel;

	if (!gpLogSize || !(mc_FnInfo[0])) return;

	if (mn_FnSuffix)
	{
		const int cchSpaceLeft = static_cast<int>(std::size(mc_FnInfo) - mn_FnSuffix);
		lstrcpyn(mc_FnInfo + mn_FnSuffix, kFnEnd, cchSpaceLeft);
		LogString(mc_FnInfo);
	}
	else
	{
		LogString(CEStr(mc_FnInfo, kFnEnd));
	}
}
