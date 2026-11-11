
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
#include "Common.h"
#include "CEStr.h"
#include "CmdLine.h"
#include "EnvVar.h"
#include "MStrDup.h"
#include "WObjects.h"

#ifndef CONEMU_MINIMAL
// DON'T disable redirector in ConEmuHk, we shall not change the console app behavior
#include "MWow64Disable.h"
#endif

namespace
{
	// These chars could not exist in paths or file names
	const wchar_t ILLEGAL_CHARACTERS[] = L"?*<>|";
	// Cmd special characters - pipelines, redirections, escaping
	const wchar_t SPECIAL_CMD_CHARACTERS[] = L"&|<>^";
	// Cmd special characters - pipelines, redirections
	const wchar_t REDIRECTION_CMD_CHARACTERS[] = L"&|<>";

	// The commands, which we shall not try to expand/convert into "*.exe"
	const wchar_t* const CMD_INTERNAL_COMMANDS[] = {
		L"ACTIVATE", L"ALIAS", L"ASSOC", L"ATTRIB", L"BEEP", L"BREAK", L"CALL", L"CDD", L"CHCP", L"COLOR", L"COPY", L"DATE", L"DEFAULT", L"DEL", L"DELAY", L"DESCRIBE",
		L"DETACH", L"DIR", L"DIRHISTORY", L"DIRS", L"DRAWBOX", L"DRAWHLINE", L"DRAWVLINE", L"ECHO", L"ECHOERR", L"ECHOS", L"ECHOSERR", L"ENDLOCAL", L"ERASE",
		L"ERRORLEVEL", L"ESET", L"EXCEPT", L"EXIST", L"EXIT", L"FFIND", L"FOR", L"FREE", L"FTYPE", L"GLOBAL", L"GOTO", L"HELP", L"HISTORY", L"IF", L"IFF", L"INKEY",
		L"INPUT", L"KEYBD", L"KEYS", L"LABEL", L"LIST", L"LOG", L"MD", L"MEMORY", L"MKDIR", L"MOVE", L"MSGBOX", L"NOT", L"ON", L"OPTION", L"PATH", L"PAUSE", L"POPD",
		L"PROMPT", L"PUSHD", L"RD", L"REBOOT", L"REN", L"RENAME", L"RMDIR", L"SCREEN", L"SCRPUT", L"SELECT", L"SET", L"SETDOS", L"SETLOCAL", L"SHIFT", L"SHRALIAS",
		L"START", L"TEE", L"TIME", L"TIMER", L"TITLE", L"TOUCH", L"TREE", L"TRUENAME", L"TYPE", L"UNALIAS", L"UNSET", L"VER", L"VERIFY", L"VOL", L"VSCRPUT", L"WINDOW",
		L"Y" };
}

#if defined(CE_UNIT_TEST) && CE_UNIT_TEST==1
#include <vector>
#include <string>
std::vector<std::wstring> GetCmdInternalCommands()
{
	std::vector<std::wstring> commands;
	commands.reserve(countof(CMD_INTERNAL_COMMANDS));
	for (const auto* internalCommand: CMD_INTERNAL_COMMANDS)
	{
		commands.push_back(internalCommand);
	}
	return commands;
}
#endif

bool IsCmdInternalCommand(const wchar_t* cmd)
{
	if (!cmd || !*cmd || wcspbrk(cmd, L".\\/"))
		return false;
	auto* found = ::bsearch(cmd, CMD_INTERNAL_COMMANDS, countof(CMD_INTERNAL_COMMANDS), sizeof(wchar_t*),
		[](const void* a, const void* b) -> int
		{
			const auto cmp = lstrcmpiW(static_cast<const wchar_t*>(a), *static_cast<const wchar_t* const*>(b));
			return cmp;
		});
	return found != nullptr;
}

// Returns true on changes
// bDeQuote:  replace two "" with one "
// bDeEscape: process special symbols: ^e^[^r^n^t^b
bool DemangleArg(CmdArg& rsDemangle, bool bDeQuote /*= true*/, bool bDeEscape /*= false*/)
{
	if (rsDemangle.IsEmpty() || !(bDeQuote || bDeEscape))
	{
		return false; // Nothing to do
	}

	const auto* pszDemangles = (bDeQuote && bDeEscape) ? L"^\"" : bDeQuote ? L"\"" : L"^";
	const auto* pchCap = wcspbrk(rsDemangle, pszDemangles);
	if (pchCap == nullptr)
	{
		return false; // Nothing to replace
	}

	wchar_t* pszDst = rsDemangle.ms_Val;
	const wchar_t* pszSrc = rsDemangle.ms_Val;
	const wchar_t* pszEnd = rsDemangle.ms_Val + rsDemangle.GetLen();

	while (pszSrc < pszEnd)
	{
		if (bDeQuote && (*pszSrc == L'"'))
		{
			*(pszDst++) = *(pszSrc++);
			if (*pszSrc == L'"') // Expected, but may be missed by user?
				pszSrc++;
		}
		else if (bDeEscape && (*pszSrc == L'^'))
		{
			switch (*(++pszSrc))
			{
			case L'^': // Demangle cap
				*pszDst = L'^'; break;
			case 0:    // Leave single final cap
				*pszDst = L'^'; continue;
			case L'r': case L'R': // CR
				*pszDst = L'\r'; break;
			case L'n': case L'N': // LF
				*pszDst = L'\n'; break;
			case L't': case L'T': // TAB
				*pszDst = L'\t'; break;
			case L'a': case L'A': // BELL
				*pszDst = 7; break;
			case L'b': case L'B': // BACK
				*pszDst = L'\b'; break;
			case L'e': case L'E': case L'[': // ESC
				*pszDst = 27; break;
			default:
				// Unknown ctrl-sequence, bypass
				*(pszDst++) = *(pszSrc++);
				continue;
			}
			pszDst++; pszSrc++;
		}
		else
		{
			*(pszDst++) = *(pszSrc++);
		}
	}
	// Was processed? Zero terminate it.
	*pszDst = 0;

	return true;
}

// Function checks, if we need drop first and last quotation marks
// Example: ""7z.exe" /?"
// Using cmd.exe rules
bool IsNeedDequote(LPCWSTR asCmdLine, const bool abFromCmdCK, LPCWSTR* rsEndQuote/*=nullptr*/)
{
	if (rsEndQuote)
		*rsEndQuote = nullptr;

	if (!asCmdLine)
		return false;

	bool bDeQu = false;
	LPCWSTR pszQE, pszSP;
	if (asCmdLine[0] == L'"')
	{
		bDeQu = (asCmdLine[1] == L'"');
		// Всегда - нельзя. Иначе парсинг строки запуска некорректно идет
		// L"\"C:\\ConEmu\\ConEmuC64.exe\"  /PARENTFARPID=1 /C \"C:\\GIT\\cmdw\\ad.cmd CE12.sln & ci -m \"Solution debug build properties\"\""
		if (!bDeQu)
		{
			size_t nLen = lstrlen(asCmdLine);
			if (abFromCmdCK)
			{
				bDeQu = ((asCmdLine[nLen-1] == L'"') && (asCmdLine[nLen-2] == L'"'));
			}
			if (!bDeQu && (asCmdLine[nLen-1] == L'"'))
			{
				pszSP = wcschr(asCmdLine+1, L' ');
				pszQE = wcschr(asCmdLine+1, L'"');
				if (pszSP && pszQE && (pszSP < pszQE)
					&& ((pszSP - asCmdLine) < MAX_PATH))
				{
					CEStr lsTmp;
					lsTmp.Set(asCmdLine+1, pszSP-asCmdLine-1);
					bDeQu = (IsFilePath(lsTmp, true) && IsExecutable(lsTmp));
				}
			}
		}
	}
	if (!bDeQu)
		return false;

	// Don't dequote?
	pszQE = wcsrchr(asCmdLine+2, L'"');
	if (!pszQE)
		return false;

#if 0
	LPCWSTR pszQ1 = wcschr(asCmdLine+2, L'"');
	if (!pszQ1)
		return false;
	LPCWSTR pszQE = wcsrchr(pszQ1, L'"');
	// Only TWO quotes in asCmdLine?
	if (pszQE == pszQ1)
	{
		// Doesn't contains special symbols?
		if (!wcspbrk(asCmdLine+1, L"&<>()@^|"))
		{
			// Must contains spaces (doubt?)
			if (wcschr(asCmdLine+1, L' '))
			{
				// Cmd also checks this for executable file name. Skip this check?
				return false;
			}
		}
	}
#endif

	// Well, we get here
	_ASSERTE(asCmdLine[0]==L'"' && pszQE && *pszQE==L'"' && !wcschr(pszQE+1,L'"'));
	// Dequote it!
	if (rsEndQuote)
		*rsEndQuote = pszQE;
	return true;
}

// #CmdArg Eliminate QueryNext*** and make Next** return LPCWSTR

// Returns PTR to next arg or nullptr on error
const wchar_t* NextArg(const wchar_t* asCmdLine, CmdArg &rsArg, const wchar_t** rsArgStart/*=nullptr*/)
{
	if (!asCmdLine || !*asCmdLine)
		return nullptr;

	#ifdef _DEBUG
	if ((rsArg.m_nTokenNo==0) // first token
		|| ((rsArg.m_nTokenNo>0) && (rsArg.m_sLastTokenEnd==asCmdLine)
			&& (wcsncmp(asCmdLine, rsArg.m_sLastTokenSave, countof(rsArg.m_sLastTokenSave)-1))==0))
	{
		// OK, параметры корректны
	}
	else
	{
		_ASSERTE(FALSE && "rsArgs was not resetted before new cycle!");
	}
	#endif

	LPCWSTR psCmdLine = SkipNonPrintable(asCmdLine), pch = nullptr;
	if (!*psCmdLine)
		return nullptr;

	// Remote surrounding quotes, in certain cases
	// Example: ""7z.exe" /?"
	// Example: "C:\Windows\system32\cmd.exe" /C ""C:\Python27\python.EXE""
	if ((rsArg.m_nTokenNo == 0) || (rsArg.m_nCmdCall == CmdArg::CmdCall::CmdK))
	{
		if (IsNeedDequote(psCmdLine, (rsArg.m_nCmdCall == CmdArg::CmdCall::CmdK), &rsArg.m_pszDequoted))
			psCmdLine++;
		if (rsArg.m_nCmdCall == CmdArg::CmdCall::CmdK)
			rsArg.m_nCmdCall = CmdArg::CmdCall::CmdC;
	}

	size_t nArgLen = 0;
	bool lbQMode = false;

	if (*psCmdLine == L'"')  // if starts with `"`
	{
		lbQMode = true;
		psCmdLine++;
		// ... /d "\"C:\ConEmu\ConEmuPortable.exe\" /Dir ...
		bool bQuoteEscaped = (psCmdLine[0] == L'\\' && psCmdLine[1] == L'"');
		pch = wcschr(psCmdLine, L'"');
		if (pch && (pch > psCmdLine))
		{
			// To be correctly parsed something like this:
			// reg.exe add "HKCU\MyCo" /ve /t REG_EXPAND_SZ /d "\"C:\ConEmu\ConEmuPortable.exe\" /Dir \"%V\" /cmd \"cmd.exe\" \"-new_console:nC:cmd.exe\" \"-cur_console:d:%V\"" /f
			// But must not fail with ‘simple’ command like (no escapes in "C:\"):
			// /dir "C:\" /icon "cmd.exe" /single

			// Prev version fails while getting strings for -GuiMacro, example:
			// ConEmu.exe -detached -GuiMacro "print(\" echo abc \"); Context;"

			pch = wcspbrk(psCmdLine, L"\\\"");
			while (pch)
			{
				// Escaped quotation?
				if ((*pch == L'\\') && (*(pch+1) == L'"'))
				{
					// It's allowed when:
					// a) at the beginning of the line (handled above, bQuoteEscaped);
					// b) after space, left bracket or colon (-GuiMacro)
					// c) when already was forced by bQuoteEscaped
					if ((
						((((pch - 1) >= psCmdLine) && wcschr(L" (,", *(pch-1)))
							|| (*(pch+2) && !isSpace(*(pch+2)))
						)) || bQuoteEscaped)
					{
						bQuoteEscaped = true;
						pch++; // Point to "
					}
				}
				else if (*pch == L'"')
					break;
				// Next entry AFTER pch
				pch = wcspbrk(pch+1, L"\\\"");
			}
		}

		if (!pch)
			return nullptr;

		while (pch[1] == L'"' && (!rsArg.m_pszDequoted || ((pch+1) < rsArg.m_pszDequoted)))
		{
			pch += 2;
			pch = wcschr(pch, L'"');

			if (!pch)
				return nullptr;
		}

		// Теперь в pch ссылка на последнюю "
	}
	else
	{
		// До конца строки или до первого пробела
		//pch = wcschr(psCmdLine, L' ');
		// 09.06.2009 Maks - обломался на: cmd /c" echo Y "
		pch = psCmdLine;

		// General: Look for spacing of quote
		while (*pch && *pch!=L'"'
			&& *pch!=L' ' && *pch!=L'\t' && *pch!=L'\r' && *pch!=L'\n')
			pch++;

		//if (!pch) pch = psCmdLine + lstrlenW(psCmdLine); // до конца строки
	}

	_ASSERTE(pch >= psCmdLine);
	nArgLen = pch - psCmdLine;

	// Set result arugment
	// Warning: Don't demangle quotes/escapes here, or we'll fail to
	// concatenate environment or smth, losing quotes and others
	if (!rsArg.Set(psCmdLine, nArgLen))
		return nullptr;
	rsArg.m_bQuoted = lbQMode;
	rsArg.m_nTokenNo++;

	if (rsArgStart) *rsArgStart = psCmdLine;

	psCmdLine = pch;
	// Finalize
	if ((*psCmdLine == L'"') && (lbQMode || (rsArg.m_pszDequoted == psCmdLine)))
		psCmdLine++; // was pointed to closing quotation mark

	psCmdLine = SkipNonPrintable(psCmdLine);
	// When whole line was dequoted
	if ((*psCmdLine == L'"') && (rsArg.m_pszDequoted == psCmdLine))
		psCmdLine++;

	#ifdef _DEBUG
	rsArg.m_sLastTokenEnd = psCmdLine;
	lstrcpyn(rsArg.m_sLastTokenSave, psCmdLine, countof(rsArg.m_sLastTokenSave));
	#endif

	switch (rsArg.m_nCmdCall)
	{
	case CmdArg::CmdCall::Undefined:
		// Если это однозначно "ключ" - то на имя файла не проверяем
		if (*rsArg.ms_Val == L'/' || *rsArg.ms_Val == L'-')
		{
			// Это для парсинга (чтобы ассертов не было) параметров из ShellExecute (там cmd.exe указывается в другом аргументе)
			if ((rsArg.m_nTokenNo == 1) && (lstrcmpi(rsArg.ms_Val, L"/C") == 0 || lstrcmpi(rsArg.ms_Val, L"/K") == 0))
				rsArg.m_nCmdCall = CmdArg::CmdCall::CmdK;
		}
		else
		{
			pch = PointToName(rsArg.ms_Val);
			if (pch)
			{
				if ((lstrcmpi(pch, L"cmd") == 0 || lstrcmpi(pch, L"cmd.exe") == 0)
					|| (lstrcmpi(pch, L"ConEmuC") == 0 || lstrcmpi(pch, L"ConEmuC.exe") == 0)
					|| (lstrcmpi(pch, L"ConEmuC64") == 0 || lstrcmpi(pch, L"ConEmuC64.exe") == 0))
				{
					rsArg.m_nCmdCall = CmdArg::CmdCall::CmdExeFound;
				}
			}
		}
		break;
	case CmdArg::CmdCall::CmdExeFound:
		if (lstrcmpi(rsArg.ms_Val, L"/C") == 0 || lstrcmpi(rsArg.ms_Val, L"/K") == 0)
			rsArg.m_nCmdCall = CmdArg::CmdCall::CmdK;
		else if ((rsArg.ms_Val[0] != L'/') && (rsArg.ms_Val[0] != L'-'))
			rsArg.m_nCmdCall = CmdArg::CmdCall::Undefined;
		break;
	}

	return psCmdLine;
}

// Returns PTR to next line or nullptr on error
const wchar_t* NextLine(const wchar_t* asLines, CEStr &rsLine, NEXTLINEFLAGS Flags /*= NLF_TRIM_SPACES|NLF_SKIP_EMPTY_LINES*/)
{
	if (!asLines || !*asLines)
		return nullptr;

	const wchar_t* psz = asLines;
	//const wchar_t szSpaces[] = L" \t";
	//const wchar_t szLines[] = L"\r\n";
	//const wchar_t szSpacesLines[] = L" \t\r\n";

	if ((Flags & (NLF_TRIM_SPACES|NLF_SKIP_EMPTY_LINES)) == (NLF_TRIM_SPACES|NLF_SKIP_EMPTY_LINES))
	{
		psz = SkipNonPrintable(psz);
	}
	else if (Flags & NLF_TRIM_SPACES)
	{
		while (*psz == L' ' || *psz == L'\t') psz++;
	}
	else if (Flags & NLF_SKIP_EMPTY_LINES)
	{
		while (*psz == L'\r' || *psz == L'\n') psz++;
	}

	if (!*psz)
	{
		return nullptr;
	}

	const wchar_t* pszEnd = wcspbrk(psz, L"\r\n");
	if (!pszEnd)
	{
		pszEnd = psz + lstrlen(psz);
	}

	const wchar_t* pszTrim = pszEnd;
	if (*pszEnd == L'\r') pszEnd++;
	if (*pszEnd == L'\n') pszEnd++;

	if (Flags & NLF_TRIM_SPACES)
	{
		while ((pszTrim > psz) && ((*(pszTrim-1) == L' ') || (*(pszTrim-1) == L'\t')))
			pszTrim--;
	}

	_ASSERTE(pszTrim >= psz);
	rsLine.Set(psz, pszTrim-psz);
	psz = pszEnd;

	return psz;
}

int AddEndSlash(wchar_t* rsPath, int cchMax)
{
	if (!rsPath || !*rsPath)
		return 0;
	int nLen = lstrlen(rsPath);
	if (rsPath[nLen-1] != L'\\')
	{
		if (cchMax >= (nLen+2))
		{
			rsPath[nLen++] = L'\\'; rsPath[nLen] = 0;
		}
	}
	return nLen;
}

bool IsNonPrintable(const wchar_t chr)
{
	// Don't add 0xA0 here as it's not a "real" spacing character
	return (chr == L' ' || chr == L'\t' || chr == L'\r' || chr == L'\n');
}

const wchar_t* SkipNonPrintable(const wchar_t* asParams)
{
	if (!asParams)
		return nullptr;
	const wchar_t* psz = asParams;
	while (IsNonPrintable(*psz))
		psz++;
	return psz;
}

// One trailing (or middle) asterisk allowed
bool CompareFileMask(const wchar_t* asFileName, const wchar_t* asMask)
{
	if (!asFileName || !*asFileName || !asMask || !*asMask)
		return false;
	// Any file?
	if (*asMask == L'*' && *(asMask+1) == 0)
		return true;

	int iCmp = -1;

	wchar_t sz1[MAX_PATH+1], sz2[MAX_PATH+1];
	lstrcpyn(sz1, asFileName, countof(sz1));
	size_t nLen1 = lstrlen(sz1);
	CharUpperBuffW(sz1, (DWORD)nLen1);
	lstrcpyn(sz2, asMask, countof(sz2));
	size_t nLen2 = lstrlen(sz2);
	CharUpperBuffW(sz2, (DWORD)nLen2);

	wchar_t* pszAst = wcschr(sz2, L'*');
	if (!pszAst)
	{
		iCmp = lstrcmp(sz1, sz2);
	}
	else
	{
		*pszAst = 0;
		size_t nLen = pszAst - sz2;
		size_t nRight = lstrlen(pszAst+1);
		if (wcsncmp(sz1, sz2, nLen) == 0)
		{
			if (!nRight)
				iCmp = 0;
			else if (nLen1 >= (nRight + nLen))
				iCmp = lstrcmp(sz1+nLen1-nRight, pszAst+1);
		}
	}

	return (iCmp == 0);
}

LPCWSTR GetDrive(LPCWSTR pszPath, wchar_t* szDrive, int/*countof(szDrive)*/ cchDriveMax)
{
	if (!szDrive || cchDriveMax < 16)
		return nullptr;

	if (pszPath[0] != L'\\' && pszPath[1] == L':')
	{
		lstrcpyn(szDrive, pszPath, 3);
	}
	else if (lstrcmpni(pszPath, L"\\\\?\\UNC\\", 8) == 0)
	{
		// UNC format? "\\?\UNC\Server\Share"
		LPCWSTR pszSlash = wcschr(pszPath+8, L'\\'); // point to end of server name
		pszSlash = pszSlash ? wcschr(pszSlash+1, L'\\') : nullptr; // point to end of share name
		lstrcpyn(szDrive, pszPath, pszSlash ? (int)std::min((INT_PTR)cchDriveMax,pszSlash-pszPath+1) : cchDriveMax);
	}
	else if (lstrcmpni(pszPath, L"\\\\?\\", 4) == 0 && pszPath[4] && pszPath[5] == L':')
	{
		lstrcpyn(szDrive, pszPath, 7);
	}
	else if (pszPath[0] == L'\\' && pszPath[1] == L'\\')
	{
		// "\\Server\Share"
		LPCWSTR pszSlash = wcschr(pszPath+2, L'\\'); // point to end of server name
		pszSlash = pszSlash ? wcschr(pszSlash+1, L'\\') : nullptr; // point to end of share name
		lstrcpyn(szDrive, pszPath, pszSlash ? (int)std::min((INT_PTR)cchDriveMax,pszSlash-pszPath+1) : cchDriveMax);
	}
	else
	{
		lstrcpyn(szDrive, L"", cchDriveMax);
	}
	return szDrive;
}

/// <summary>
/// Returns current directory using buffer in szDir
/// </summary>
/// <param name="szDir">Buffer where current directory is set</param>
/// <returns>(LPCWSTR)szDir</returns>
LPCWSTR GetDirectory(CEStr& szDir)
{
	// ReSharper disable twice CppJoinDeclarationAndAssignment
	DWORD nLen, nMax;

	nMax = GetCurrentDirectoryW(MAX_PATH, szDir.GetBuffer(MAX_PATH));
	if (!nMax)
	{
		szDir.Clear();
		goto wrap;
	}
	else if (nMax > MAX_PATH)
	{
		nLen = GetCurrentDirectoryW(nMax, szDir.GetBuffer(nMax));
		if (!nLen || (nLen > nMax))
		{
			szDir.Clear();
			goto wrap;
		}
	}

wrap:
	return szDir.IsEmpty() ? nullptr : szDir.c_str();
}

bool GetFilePathFromSpaceDelimitedString(const wchar_t* commandLine, CEStr& szExe, const wchar_t*& rsArguments)
{
	szExe.Clear();
	rsArguments = nullptr;

	// 17.10.2010 - support executable file path without parameters, but with spaces in its path
	// 22.11.2015 - or some weirdness, like `C:\Program Files\CodeBlocks/cb_console_runner.exe "C:\sources\app.exe"`

	if (!commandLine)
		return false;

	bool result = false;
	const auto* command = commandLine;
	// Skip quotation marks for now, we process here commands like
	// `"C:\Program Files\Far\far.exe /p:path /some-switch"`
	// `"C:\arc\7z.exe a test.7z *.*"`
	size_t wasQuoted = 0;
	while (*command == L'"')
	{
		++command; ++wasQuoted;
	}

	// Empty string - nothing to do
	if (!*command)
		return false;

	const auto* validEnd = wcspbrk(command, ILLEGAL_CHARACTERS);
	if (!validEnd)
		validEnd = command + wcslen(command);

	const wchar_t breakChars[] = L" \"\t\r\n";
	LPCWSTR nextBreak = wcspbrk(command, breakChars);
	if (!nextBreak)
		nextBreak = validEnd;

	CEStr szTemp;
	uint64_t nTempSize;
	bool repeat = true; // at least once, even if *nextBreak == 0
	while (repeat)
	{
		// ReSharper disable once CppAssignedValueIsNeverUsed
		repeat = false;
		_ASSERTE(nextBreak > command);
		if (*nextBreak == L'"')
		{
			// If we reached the '"' - we don't care about this string,
			// reason is that executable is already properly quoted.
			// Or the string is completely malformed.
			break;
		}

		_ASSERTE(command && *command != L'"');
		szTemp.Set(command, (nextBreak - command));
		_ASSERTE(szTemp[(nextBreak - command)] == 0);

		// one of cmd internal commands?
		if (!szTemp.IsEmpty() && !IsFilePath(szTemp, true) && !wcspbrk(szTemp.c_str(), L":\\/")
			&& !wcspbrk(szTemp.c_str(), ILLEGAL_CHARACTERS) && !wcspbrk(szTemp.c_str(), SPECIAL_CMD_CHARACTERS)
			&& IsCmdInternalCommand(szTemp))
		{
			rsArguments = SkipNonPrintable(nextBreak);
			szExe.Set(szTemp);
			result = true;
			break;
		}

		// If this is a full path without environment variables
		if (!szTemp.IsEmpty()
			&& ((IsFilePath(szTemp, true) && !wcschr(szTemp, L'%'))
				// or file/dir may be found via env.var. substitution or searching in %PATH%
				|| FileExistsSearch(szTemp.c_str(), szTemp, true))
			// Than check if it is a FILE (not a directory)
			&& FileExists(szTemp, &nTempSize) && nTempSize)
		{
			// OK, it an our executable
			rsArguments = SkipNonPrintable(nextBreak);
			szExe.Set(szTemp);
			result = !szExe.IsEmpty();
			break;
		}

		_ASSERTE(*nextBreak == 0 || wcschr(breakChars, *nextBreak));
		if (!*nextBreak)
			break;
		// Find next non-space
		while (*nextBreak && (*nextBreak != L'"') && wcschr(breakChars, *nextBreak))
			nextBreak++;
		// If quoted string starts from here - it's supposed to be an argument, no valid exe found.
		if (*nextBreak == L'"')
			break;

		if (nextBreak >= validEnd)
			break;
		nextBreak = wcspbrk(nextBreak, breakChars);
		if (!nextBreak)
			nextBreak = validEnd;
		repeat = true;
	}

	return result;
}

bool IsCmdRedirection(LPCWSTR asParameters)
{
	if (IsStrEmpty(asParameters))
		return false;
	if (wcspbrk(asParameters, REDIRECTION_CMD_CHARACTERS) == nullptr)
		return false;

	CmdArg next;
	const wchar_t* nextStart = nullptr;
	const wchar_t* param = asParameters;
	while ((param = NextArg(param, next, &nextStart)) != nullptr)
	{
		if (wcspbrk(next.c_str(L""), REDIRECTION_CMD_CHARACTERS) == nullptr)
			continue;
		if (!next.m_bQuoted)
			return true;
	}

	return false;
}

bool IsNeedCmd(bool bRootCmd, LPCWSTR asCmdLine, CEStr &szExe, NeedCmdOptions* options /*= nullptr*/)
{
	// do we need to add leading "cmd.exe /c "
	bool isNeedCmd = false;
	// the command is already or should be started via "cmd.exe"
	bool rootIsCmdExe = false;
	// do we need to change quotation?
	StartEndQuot startEndQuot = StartEndQuot::DontChange;
	// if it's suggested to confirm console termination
	bool alwaysConfirmExit = false;
	// pointer to rest of the commands line
	LPCWSTR argumentsPtr = nullptr;

	wchar_t *pwszEndSpace;

	size_t leadingQuotes = 0;
	bool exeIsQuoted = false;
	bool firstObtained = false;
	LPCWSTR pwszCopy;
	int nLastChar;
	#ifdef _DEBUG
	CmdArg szDbgFirst;
	bool bIsBatch = false;
	#endif

	if (!asCmdLine || !*asCmdLine)
	{
		_ASSERTE(asCmdLine && *asCmdLine);
		szExe.Clear();
		alwaysConfirmExit = true;
		isNeedCmd = true;
		goto wrap;
	}

	#ifdef _DEBUG
	// Это минимальные проверки, собственно к коду - не относятся
	{
		NextArg(asCmdLine, szDbgFirst);
		const auto* psz = PointToExt(szDbgFirst);
		if (lstrcmpi(psz, L".cmd")==0 || lstrcmpi(psz, L".bat")==0)
			bIsBatch = true;
	}
	#endif

	if (!szExe.GetBuffer(MAX_PATH))
	{
		_ASSERTE(FALSE && "Failed to allocate MAX_PATH");
		isNeedCmd = true;
		goto wrap;
	}
	szExe.Clear();


	pwszCopy = asCmdLine;
	// cmd /c ""c:\program files\arc\7z.exe" -?"   // да еще и внутри могут быть двойными...
	// cmd /c "dir c:\"
	nLastChar = lstrlenW(pwszCopy) - 1;
	while (nLastChar > 0 && IsNonPrintable(pwszCopy[nLastChar]))
	{
		--nLastChar;
	}
	while (pwszCopy[leadingQuotes] == L'"')
	{
		++leadingQuotes;
	}

	if (pwszCopy[0] == L'"' && pwszCopy[nLastChar] == L'"')
	{
		// #IsNeedCmd try to cut the quotes and process the modified string

		// Examples
		// `""c:\program files\arc\7z.exe" -?"`
		// `""F:\VCProject\FarPlugin\#FAR180\far.exe  -new_console""`
		// `"c:\arc\7z.exe -?"`
		// `"C:\GCC\msys\bin\make.EXE -f "makefile" COMMON="../../../plugins/common""`
		// `""F:\VCProject\FarPlugin\#FAR180\far.exe  -new_console""`
		// `""cmd""`
		// `cmd /c ""c:\program files\arc\7z.exe" -?"` // could be also double-double-quoted inside
		// `cmd /c "dir c:\"`

		// Get the first argument (the executable?)
		CmdArg arg;
		const auto* nextArg = NextArg(pwszCopy, arg);
		if (!nextArg)
		{
			//Parsing command line failed
			#ifdef WARN_NEED_CMD
			_ASSERTE(FALSE);
			#endif
			alwaysConfirmExit = true;
			isNeedCmd = true;
			goto wrap;
		}
		szExe.Set(arg);
		exeIsQuoted = arg.m_bQuoted;

		if (lstrcmpiW(szExe, L"start") == 0)
		{
			// The "start" command could be executed only by command processor
			#ifdef WARN_NEED_CMD
			_ASSERTE(FALSE);
			#endif
			isNeedCmd = true;
			goto wrap;
		}

		if (arg.m_pszDequoted)
		{
			uint64_t nTempSize = 0;
			CEStr expanded;
			const wchar_t* exeToCheck = nullptr;
			// file/dir may be found via env.var. substitution or searching in %PATH%
			if (FileExistsSearch(szExe.c_str(), expanded, true))
				exeToCheck = expanded.IsEmpty() ? szExe.c_str() : expanded.c_str();
			// or it's already a full specified file path
			else if (IsFilePath(szExe, true))
				exeToCheck = szExe.c_str();;

			// Than check if it is a FILE (not a directory)
			if (exeToCheck && FileExists(exeToCheck, &nTempSize) && nTempSize)
			{
				_ASSERTE(*pwszCopy == L'"' && pwszCopy == asCmdLine); // should be still
				++pwszCopy; // skip first double quote
				firstObtained = true;

				if (!expanded.IsEmpty())
					szExe = std::move(expanded);
				// #TODO remove ending quote
				argumentsPtr = SkipNonPrintable(nextArg);

				startEndQuot = StartEndQuot::NeedCut;
			}
		}
	}

	// Get the first argument (the executable?)
	if (!firstObtained)
	{
		szExe.Clear();

		// `start` command must be processed by processor itself
		if ((lstrcmpni(pwszCopy, L"start", 5) == 0)
			&& (!pwszCopy[5] || IsNonPrintable(pwszCopy[5])))
		{
			#ifdef WARN_NEED_CMD
			_ASSERTE(FALSE);
			#endif
			isNeedCmd = true;
			goto wrap;
		}

		// will return true if we found a real existing file path
		if (GetFilePathFromSpaceDelimitedString(pwszCopy, szExe, argumentsPtr))
		{
			// e.g. "C:\1\d.cmd 2 test", now szExe=="C:\1\d.cmd" and it's not quoted.
			exeIsQuoted = false;
		}
		else
		{
			CmdArg arg;
			if (!((pwszCopy = NextArg(pwszCopy, arg))))
			{
				// Parsing command line failed
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				alwaysConfirmExit = true;
				isNeedCmd = true;
				goto wrap;
			}
			szExe.Set(arg);
			exeIsQuoted = arg.m_bQuoted;

			_ASSERTE(lstrcmpiW(szExe, L"start") != 0);

			// Expand environment variables and search in the %PATH%
			FileExistsSearch(szExe.c_str(), szExe, true);

			argumentsPtr = SkipNonPrintable(pwszCopy);
		}
	}

	if (!*szExe)
	{
		_ASSERTE(szExe[0] != 0);
	}
	else
	{
		// Illegal characters in the executable we parse from the command line.
		// We can't run the program as it's path is invalid, so let's try it to "cmd.exe /c ..."
		if (wcspbrk(szExe, ILLEGAL_CHARACTERS))
		{
			#ifdef _DEBUG
			CmdArg testCmd;
			bool isInternalCmd = false;
			if (NextArg(pwszCopy, testCmd))
				isInternalCmd = IsCmdInternalCommand(testCmd);
			#endif
			szExe.Clear();
			rootIsCmdExe = true;
			isNeedCmd = true;
			goto wrap;
		}

		// If there is no "path"
		if (wcschr(szExe, L'\\') == nullptr)
		{
			const bool bHasExt = (wcschr(szExe, L'.') != nullptr);
			// Let's check if it's a processor command, e.g. "DIR"
			if (IsCmdInternalCommand(szExe))
			{
				#ifdef WARN_NEED_CMD
				_ASSERTE(FALSE);
				#endif
				rootIsCmdExe = true;
				isNeedCmd = true;
				goto wrap;
		}

			// Try to find executable in %PATH%
			{
				#ifndef CONEMU_MINIMAL
				MWow64Disable wow; wow.Disable(); // Disable Wow64 file redirector
				#endif
				// #TODO isn't apiSearchPath already called?
				apiSearchPath(nullptr, szExe, bHasExt ? nullptr : L".exe", szExe);
			}
		} // end: if (wcschr(szExe, L'\\') == nullptr)
	}

	// If szExe does not contain the path to the file - try to run via cmd
	// "start "" C:\Utils\Files\Hiew32\hiew32.exe C:\00\Far.exe"
	if (!IsFilePath(szExe))
	{
		#ifdef WARN_NEED_CMD
		_ASSERTE(FALSE);
		#endif
		rootIsCmdExe = true;
		isNeedCmd = true;
		goto wrap;
	}

	//pwszCopy = wcsrchr(szArg, L'\\'); if (!pwszCopy) pwszCopy = szArg; else pwszCopy ++;
	pwszCopy = PointToName(szExe);

	pwszEndSpace = szExe.ms_Val + lstrlenW(szExe) - 1;

	while ((*pwszEndSpace == L' ') && (pwszEndSpace > szExe))
	{
		*(pwszEndSpace--) = 0;
	}

#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif

	if (lstrcmpiW(pwszCopy, L"cmd")==0 || lstrcmpiW(pwszCopy, L"cmd.exe")==0
		|| lstrcmpiW(pwszCopy, L"tcc")==0 || lstrcmpiW(pwszCopy, L"tcc.exe")==0)
	{
		rootIsCmdExe = true; // it IS cmd.exe
		alwaysConfirmExit = true;
		#ifdef _DEBUG // due to unittests
		_ASSERTE(!bIsBatch);
		#endif
		isNeedCmd = false; // as it's already cmd.exe, we don't need to add "cmd /c"
		goto wrap;
	}


	// GoogleIssue 1211: Decide not to do weird heuristic.
	//   If user REALLY needs redirection for root command (huh?)
	//   - they must call "cmd /c ..." directly
	if (!bRootCmd)
	{
		// If we are in comspec mode (run child process from existing shell) and command line contains
		// any of pipelining/redirection characters, than run via "cmd /c ..."
		if (wcspbrk(asCmdLine, SPECIAL_CMD_CHARACTERS) != nullptr)
		{
			#ifdef WARN_NEED_CMD
			_ASSERTE(FALSE);
			#endif
			isNeedCmd = true; goto wrap;  // add cmd.exe
		}
	}

	if (IsFarExe(pwszCopy))
	{
		bool bFound = (wcschr(pwszCopy, L'.') != nullptr);
		// If just "far" was started, it could be a batch, located in the %PATH%
		if (!bFound)
		{
			CEStr szSearch;
			if (apiSearchPath(nullptr, pwszCopy, L".exe", szSearch))
			{
				if (lstrcmpi(PointToExt(szSearch), L".exe") == 0)
					bFound = true;
			}
		}

		if (bFound)
		{
			rootIsCmdExe = false; // FAR!
			#ifdef _DEBUG // due to unittests
			_ASSERTE(!bIsBatch);
			#endif
			isNeedCmd = false; goto wrap; // already executable, no need to add leading cmd.exe
		}
	}

	if (IsExecutable(szExe))
	{
		rootIsCmdExe = false;
		#ifdef _DEBUG // due to unittests
		_ASSERTE(!bIsBatch);
		#endif
		isNeedCmd = false; goto wrap; // already executable, no need to add leading cmd.exe
	}

	//Можно еще Доделать поиски с: SearchPath, GetFullPathName, добавив расширения .exe & .com
	//хотя фар сам формирует полные пути к командам, так что можно не заморачиваться
	#ifdef WARN_NEED_CMD
	_ASSERTE(FALSE);
	#endif
	rootIsCmdExe = true;
#ifndef __GNUC__
#pragma warning( pop )
#endif

	isNeedCmd = true;
wrap:
	if (options)
	{
		// validate cmd quotation rules
		if (!szExe.IsEmpty())
		{
			if (isNeedCmd)
			{
				if (leadingQuotes == 1)
				{
					const bool quotationNeeded = IsQuotationNeeded(szExe);
					if (quotationNeeded || (exeIsQuoted && !IsCmdRedirection(argumentsPtr)))
						startEndQuot = StartEndQuot::NeedAdd;
					else if (!exeIsQuoted)
						startEndQuot = StartEndQuot::NeedCut;
					else
						startEndQuot = StartEndQuot::DontChange;
				}
				else if (leadingQuotes >= 2)
				{
					startEndQuot = StartEndQuot::DontChange;
				}
			}
			else if (leadingQuotes == 1 && !exeIsQuoted)
			{
				startEndQuot = StartEndQuot::NeedCut;
			}
		}
		// result
		options->isNeedCmd = isNeedCmd;
		options->startEndQuot = startEndQuot;
		options->rootIsCmdExe = rootIsCmdExe || isNeedCmd;
		options->alwaysConfirmExit = alwaysConfirmExit;
	}
	return isNeedCmd;
}

#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif
bool IsExecutable(LPCWSTR aszFilePathName, wchar_t** rsExpandedVars /*= nullptr*/)
{
#ifndef __GNUC__
#pragma warning( push )
#pragma warning(disable : 6400)
#endif
	bool result = false;
	CEStr expanded;

	for (int i = 0; i <= 1; i++)
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		LPCWSTR extension = PointToExt(aszFilePathName);

		if (extension)  // if .exe or .com was specified
		{
			if (lstrcmpiW(extension, L".exe")==0 || lstrcmpiW(extension, L".com")==0)
			{
				if (FileExists(aszFilePathName))
				{
					result = true;
					break;
				}
			}
		}

		if (!i && wcschr(aszFilePathName, L'%'))
		{
			expanded = ExpandEnvStr(aszFilePathName);
			if (!expanded)
				break;
			aszFilePathName = expanded.c_str();
			continue;
		}
		break;
	}

	if (rsExpandedVars)
	{
		SafeFree(*rsExpandedVars)
		*rsExpandedVars = expanded.Detach();
	}

	return result;
}
#ifndef __GNUC__
#pragma warning( pop )
#endif

bool CompareProcessNames(LPCWSTR pszProcess1, LPCWSTR pszProcess2)
{
	LPCWSTR pszName1 = PointToName(pszProcess1);
	LPCWSTR pszName2 = PointToName(pszProcess2);
	if (!pszName1 || !*pszName1 || !pszName2 || !*pszName2)
		return false;

	const auto* pszExt1 = wcsrchr(pszName1, L'.');
	const auto* pszExt2 = wcsrchr(pszName2, L'.');

	CEStr lsName1, lsName2;
	if (!pszExt1)
	{
		lsName1 = CEStr(pszName1, L".exe");
		pszName1 = lsName1;
		if (!pszName1)
			return false;
	}
	if (!pszExt2)
	{
		lsName2 = CEStr(pszName2, L".exe");
		pszName2 = lsName2;
		if (!pszName2)
			return false;
	}

	const int iCmp = lstrcmpi(pszName1, pszName2);
	return (iCmp == 0);
}

bool CheckProcessName(LPCWSTR pszProcessName, LPCWSTR* lsNames)
{
	LPCWSTR pszName1 = PointToName(pszProcessName);
	if (!pszName1 || !*pszName1 || !lsNames)
		return false;

	const auto* pszExt1 = wcsrchr(pszName1, L'.');

	CEStr lsName1;
	if (!pszExt1)
	{
		lsName1 = CEStr(pszName1, L".exe");
		pszName1 = lsName1;
		if (!pszName1)
			return false;
	}

	for (size_t i = 0; lsNames[i]; i++)
	{
		const auto* pszName2 = lsNames[i];

		_ASSERTE(wcsrchr(pszName2, L'.') != nullptr);
		#if 0
		CEStr lsName2;
		LPCWSTR pszExt2 = wcsrchr(pszName2, L'.');
		if (!pszExt2)
		{
			lsName2 = CEStr(pszName2, L".exe");
			pszName2 = lsName2;
			if (!pszName2)
				return false;
		}
		#endif

		if (lstrcmpi(pszName1, pszName2) == 0)
			return true;
	}

	return false;
}

bool IsConsoleService(LPCWSTR pszProcessName)
{
	LPCWSTR lsNameExt[] = {L"csrss.exe", L"conhost.exe", nullptr};
	return CheckProcessName(pszProcessName, lsNameExt);
}

bool IsConEmuGui(LPCWSTR pszProcessName)
{
	LPCWSTR lsNameExt[] = {L"ConEmu.exe", L"ConEmu64.exe", nullptr};
	return CheckProcessName(pszProcessName, lsNameExt);
}

bool IsConsoleServer(LPCWSTR pszProcessName)
{
	LPCWSTR lsNameExt[] = {L"ConEmuC.exe", L"ConEmuC64.exe", nullptr};
	return CheckProcessName(pszProcessName, lsNameExt);
}

bool IsTerminalServer(LPCWSTR pszProcessName)
{
	LPCWSTR lsNames[] = {
		L"conemu-cyg-32.exe", L"conemu-cyg-64.exe",
		L"conemu-msys-32.exe",
		L"conemu-msys2-32.exe", L"conemu-msys2-64.exe",
		nullptr};
	return CheckProcessName(pszProcessName, lsNames);
}

bool IsGitBashHelper(LPCWSTR pszProcessName)
{
	LPCWSTR lsNameExt[] = { L"git-bash.exe", L"git-cmd.exe", nullptr };
	return CheckProcessName(pszProcessName, lsNameExt);
}

bool IsSshAgentHelper(LPCWSTR pszProcessName)
{
	LPCWSTR lsNameExt[] = { L"ssh-agent.exe", nullptr };
	return CheckProcessName(pszProcessName, lsNameExt);
}

bool IsConsoleHelper(LPCWSTR pszProcessName)
{
	if (IsTerminalServer(pszProcessName)
		|| IsGitBashHelper(pszProcessName))
		return true;
	return false;
}

bool IsFarExe(LPCWSTR asModuleName)
{
	LPCWSTR lsNameExt[] = {L"far.exe", L"far64.exe", nullptr};
	return CheckProcessName(asModuleName, lsNameExt);
}

bool IsCmdProcessor(LPCWSTR asModuleName)
{
	LPCWSTR lsNameExt[] = {L"cmd.exe", L"tcc.exe", nullptr};
	return CheckProcessName(asModuleName, lsNameExt);
}

bool IsVsNetHostExe(LPCWSTR processName)
{
	bool bVsNetHostRequested = false;
	const int iNameLen = processName ? lstrlen(processName) : 0;
	const wchar_t pszVsHostSuffix[] = L".vshost.exe";
	const int iVsHostSuffix = lstrlen(pszVsHostSuffix);
	if ((iNameLen >= iVsHostSuffix) && (lstrcmpi(processName+iNameLen-iVsHostSuffix, pszVsHostSuffix) == 0))
	{
		bVsNetHostRequested = true;
	}
	return bVsNetHostRequested;
}

bool IsVsDebugConsoleExe(LPCWSTR processName)
{
	return CompareProcessNames(L"VsDebugConsole.exe", processName);
}

bool IsVsDebugger(LPCWSTR processName)
{
	return CompareProcessNames(L"msvsmon.exe", processName);
}

bool IsGDB(LPCWSTR processName)
{
	return CompareProcessNames(L"gdb.exe", processName);
}

bool IsQuotationNeeded(LPCWSTR pszPath)
{
	bool bNeeded = false;
	if (pszPath)
	{
		bNeeded = (wcspbrk(pszPath, QuotationNeededChars) != nullptr);
	}
	return bNeeded;
}

CEStr MergeCmdLine(LPCWSTR asExe, LPCWSTR asParams)
{
	const bool bNeedQuot = IsQuotationNeeded(asExe);
	if (asParams && !*asParams)
		asParams = nullptr;

	CEStr result = bNeedQuot
		? CEStr(L"\"", asExe, asParams ? L"\" " : L"\"", asParams)
		: CEStr(asExe, asParams ? L" " : nullptr, asParams);
	return result;
}

CEStr JoinPath(LPCWSTR asPath, LPCWSTR asPart1, LPCWSTR asPart2 /*= nullptr*/)
{
	LPCWSTR psz1 = asPath, psz2 = nullptr, psz3 = asPart1, psz4 = nullptr, psz5 = asPart2;

	// Добавить слеши если их нет на гранях
	// удалить лишние, если они указаны в обеих частях

	if (asPart1)
	{
		const bool bDirSlash1  = (psz1 && *psz1) ? (psz1[lstrlen(psz1)-1] == L'\\') : false;
		const bool bFileSlash1 = (asPart1[0] == L'\\');

		if (bDirSlash1 && bFileSlash1)
			psz3++;
		else if (!bDirSlash1 && !bFileSlash1 && asPath && *asPath)
			psz2 = L"\\";

		if (asPart2)
		{
			bool bDirSlash2  = (psz3 && *psz3) ? (psz3[lstrlen(psz3)-1] == L'\\') : false;
			bool bFileSlash2 = (asPart2[0] == L'\\');

			if (bDirSlash2 && bFileSlash2)
				psz5++;
			else if (!bDirSlash2 && !bFileSlash2)
				psz4 = L"\\";
		}
	}

	return CEStr(psz1, psz2, psz3, psz4, psz5);
}

CEStr GetParentPath(LPCWSTR asPath)
{
	if (!asPath || !*asPath)
		return {};
	LPCWSTR pszName = PointToName(asPath);
	if (!pszName)
		return {};
	while ((pszName > asPath) && (*(pszName-1) == L'\\' || *(pszName-1) == L'/'))
		--pszName;
	if (pszName <= asPath)
		return {};

	const size_t cch = pszName - asPath;
	CEStr parent;
	if (parent.GetBuffer(cch + 1))
	{
		wcsncpy_s(parent.data(), parent.GetMaxCount(), asPath, cch);
		parent.SetAt(cch, 0);
	}
	return parent;
}

bool IsFilePath(LPCWSTR asFilePath, const bool abFullRequired /*= false*/)
{
	if (!asFilePath || !*asFilePath)
		return false;

	// Если в пути встречаются недопустимые символы
	// If contains some illegal characters
	if (wcschr(asFilePath, L'"') ||
		wcschr(asFilePath, L'>') ||
		wcschr(asFilePath, L'<') ||
		wcschr(asFilePath, L'|')
		// '/' don't restrict - it's allowed both in Windows and in cygwin
		)
	{
		return false;
	}

	// skip UNC prefix "\\?\" in paths like "\\?\C:\Tools" or "\\?\UNC\Server\Share"
	bool isUncPath = false;
	if (asFilePath[0] == L'\\' && asFilePath[1] == L'\\' && asFilePath[2] == L'?' && asFilePath[3] == L'\\')
	{
		asFilePath += 4; //-V112
		isUncPath = true;
	}

	// Don't allow two (and more) ":\"
	const auto* pszColon = wcschr(asFilePath, L':');
	if (pszColon)
	{
		// If the ":" exists, that it should be the path like "X:\xxx", i.e. ":" should be second character
		if (pszColon != (asFilePath + 1))
			return false;

		if (!isDriveLetter(asFilePath[0]))
			return false;

		if (wcschr(pszColon + 1, L':'))
			return false;
	}

	if (abFullRequired)
	{
		if (isUncPath)
		{
			// For UNC network paths here should be "UNC\server\..."
			const auto* unc = asFilePath;
			if (unc[0] == L'U' && unc[1] == L'N' && unc[2] == L'C'
				&& unc[3] == L'\\' && unc[4] && unc[4] != L'\\' && wcschr(unc + 5, L'\\'))
				return true;
		}
		else
		{
			const auto* srv = asFilePath;
			if (srv[0] == L'\\' && srv[1] == L'\\' && srv[2] && srv[2] != L'\\' && wcschr(srv + 3, L'\\'))
				return true;
		}

		// And old good driver letter paths
		if (isDriveLetter(asFilePath[0]) && asFilePath[1] == L':' && asFilePath[2])
			return true;
		return false;
	}

	// May be file path
	return true;
}

const wchar_t* PointToName(const wchar_t* asFileOrPath)
{
	if (!asFileOrPath)
	{
		_ASSERTE(asFileOrPath!=nullptr);
		return nullptr;
	}

	// Utilize both type of slashes
	const wchar_t* pszBSlash = wcsrchr(asFileOrPath, L'\\');
	const wchar_t* pszFSlash = wcsrchr(pszBSlash ? pszBSlash : asFileOrPath, L'/');

	const wchar_t* pszFile = pszFSlash ? pszFSlash : pszBSlash;
	if (!pszFile)
		pszFile = asFileOrPath;
	else
		pszFile++;

	return pszFile;
}

const char* PointToName(const char* asFileOrPath)
{
	if (!asFileOrPath)
	{
		_ASSERTE(asFileOrPath!=nullptr);
		return nullptr;
	}

	// Utilize both type of slashes
	const char* pszBSlash = strrchr(asFileOrPath, '\\');
	const char* pszFSlash = strrchr(pszBSlash ? pszBSlash : asFileOrPath, '/');

	const char* pszSlash = pszFSlash ? pszFSlash : pszBSlash;;

	if (pszSlash)
		return pszSlash+1;

	return asFileOrPath;
}

// Возвращает ".ext" или nullptr в случае ошибки
const wchar_t* PointToExt(const wchar_t* asFullPath)
{
	const wchar_t* pszName = PointToName(asFullPath);
	if (!pszName)
		return nullptr; // _ASSERTE уже был
	const wchar_t* pszExt = wcsrchr(pszName, L'.');
	return pszExt;
}

// !!! Меняет asParm !!!
// Cut leading and trailing quotas
const wchar_t* Unquote(wchar_t* asParm, bool abFirstQuote /*= false*/)
{
	if (!asParm)
		return nullptr;
	if (*asParm != L'"')
		return asParm;
	wchar_t* pszEndQ = abFirstQuote ? wcschr(asParm+1, L'"') : wcsrchr(asParm+1, L'"');
	if (!pszEndQ)
	{
		*asParm = 0;
		return asParm;
	}
	*pszEndQ = 0;
	return (asParm+1);
}

// Does proper "-new_console" switch exist?
bool IsNewConsoleArg(LPCWSTR lsCmdLine, LPCWSTR pszArg /*= L"-new_console"*/)
{
	if (!lsCmdLine || !*lsCmdLine || !pszArg || !*pszArg)
		return false;

	int nArgLen = lstrlen(pszArg);
	LPCWSTR pwszCopy = (wchar_t*)wcsstr(lsCmdLine, pszArg);

	// Если после -new_console идет пробел, или это вообще конец строки
	// 111211 - после -new_console: допускаются параметры
	bool bFound = (pwszCopy
		// Must be started with space or double-quote or be the start of the string
		&& ((pwszCopy == lsCmdLine) || ((*(pwszCopy-1) == L' ') || (*(pwszCopy-1) == L'"')))
		// And check the end of parameter
		&& ((pwszCopy[nArgLen] == 0) || wcschr(L" :", pwszCopy[nArgLen])
			|| ((pwszCopy[nArgLen] == L'"') || (pwszCopy[nArgLen+1] == 0))));

	return bFound;
}
