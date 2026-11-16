
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


#include "ConsoleMain.h"
#include "ConsoleState.h"
#include <winerror.h>
#include <winnt.h>
#include <tchar.h>
#include <chrono>
#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/ConEmuColors3.h"
#include "../common/WConsole.h"
#include "../common/WConsoleInfo.h"
#include "../common/WErrGuard.h"
#include "../common/ConPtyData.h"
#include "../common/ConPtyInput.h"
#include <set>
#include "../ConEmu/version.h"

#include "ConAnsi.h"
#include "ConAnsiImpl.h"
#include "ConEmuSrv.h"
#include "Shutdown.h"

#ifdef _DEBUG
//	#define DUMP_CONSOLE_OUTPUT
#endif


#define ANSI_MAP_CHECK_TIMEOUT std::chrono::seconds(1)

#ifdef _DEBUG
#define DebugString(x) OutputDebugString(x)
#define DebugStringA(x) OutputDebugStringA(x)
#else
#define DebugString(x) //OutputDebugString(x)
#define DebugStringA(x) //OutputDebugStringA(x)
#endif





void SrvAnsi::DisplayParm::Reset(const bool full)
{
	if (full)
		*this = DisplayParm{};

	const WORD nDefColors = SrvAnsi::GetDefaultAnsiAttr();
	_TextColor = CONFORECOLOR(nDefColors);
	_BackColor = CONBACKCOLOR(nDefColors);

	_ASSERTE(!full || _Dirty==false);
}
void SrvAnsi::DisplayParm::setDirty(const bool val)
{
	_Dirty = val;
}
void SrvAnsi::DisplayParm::setBrightOrBold(const bool val)
{
	_BrightOrBold = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setItalic(const bool val)
{
	_Italic = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setUnderline(const bool val)
{
	_Underline = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setBrightFore(const bool val)
{
	_BrightFore = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setBrightBack(const bool val)
{
	_BrightBack = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setTextColor(const int val)
{
	_TextColor = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setText256(const SrvAnsi::cbit val)
{
	_Text256 = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setBackColor(const int val)
{
	_BackColor = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setBack256(const SrvAnsi::cbit val)
{
	_Back256 = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setInverse(const bool val)
{
	_Inverse = val;
	setDirty(true);
}
void SrvAnsi::DisplayParm::setCrossed(const bool val)
{
	_Crossed = val;
	setDirty(true);
}






SrvAnsi* SrvAnsi::object_ = nullptr;

std::mutex& SrvAnsi::GetObjectMutex()
{
	static std::mutex objectMutex;  // NOLINT(clang-diagnostic-exit-time-destructors)
	return objectMutex;
}

//static
SrvAnsi* SrvAnsi::Object()
{
	// #condata Let Object accept requestor (connector) PID as argument
	if (!object_)
	{
		std::unique_lock<std::mutex> lock(GetObjectMutex());
		if (!object_)
		{
			object_ = new SrvAnsi();
			// release() abandons ownership without unlocking; MSVC then throws
			// resource_deadlock_would_occur when shutdown reacquires this mutex.
			lock.unlock();

			Shutdown::RegisterEvent(SrvAnsi::Release, 0);
		}
	}
	return object_;
}

//static
void SrvAnsi::Release(LPARAM lParam)
{
	if (!object_)
		return;

	std::unique_lock<std::mutex> lock(GetObjectMutex());
	delete object_;
	object_ = nullptr;
}

SrvAnsi::SrvAnsi()
{
	const CONSOLE_SCREEN_BUFFER_INFO csbi = GetDefaultCSBI();
	int newWidth = 0, newHeight = 0; DWORD nScroll = 0;
	if (!GetConWindowSize(csbi, 0, 0, 0, &newWidth, &newHeight, &nScroll) || newWidth <= 0 || newHeight <= 0)
		throw condata::console_error("GetConWindowSize failed");

	gDisplayParm.Reset(true);

	const condata::Attribute attr = GetDefaultAttr();
	m_Primary.SetAttr(attr);
	m_Primary.Resize({static_cast<unsigned>(newWidth), newHeight});
	m_Primary.SetBellCallback(BellCallback, this);

	m_Alternative.SetAttr(attr);
	m_Alternative.Resize({static_cast<unsigned>(newWidth), newHeight});
	// Alternative buffer is not expected to have backscroll (intended for Vim and other "fullscreen" applications)
	m_Alternative.SetBackscrollMax(0);
	m_Alternative.SetBellCallback(BellCallback, this);

	// #condata Try to increase the buffer if the original title is too long?
	wchar_t szOrigTitle[2000];
	gsInitConTitle.Set(::GetConsoleTitle(szOrigTitle, static_cast<DWORD>(std::size(szOrigTitle))) ? szOrigTitle : L"ConEmu");

	StartXTermMode(true);
}

SrvAnsi::~SrvAnsi()
{
	std::unique_lock<std::mutex> lock(m_UseMutex);
	DoneAnsiLog(true);
	StartXTermMode(false);
}

condata::Table* SrvAnsi::GetTable(GetTableEnum _table)
{
	switch (_table)
	{
	case GetTableEnum::current:
		break;
	case GetTableEnum::primary:
		m_UsePrimary = true;
		break;
	case GetTableEnum::alternative:
		m_UsePrimary = false;
		break;
	default:
		_ASSERTE(FALSE && "Unexpected GetTableEnum")
		throw condata::console_error("Unexpected GetTableEnum");
	}

	return m_UsePrimary ? &m_Primary : &m_Alternative;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
CESERVER_CONSOLE_MAPPING_HDR* SrvAnsi::GetConMap() const
{
	if (gpSrv && gpSrv->pConsoleMap)
		return gpSrv->pConsoleMap->Ptr();
	return LoadSrvMapping(gState.realConWnd_, m_ClientMap) ? &m_ClientMap : nullptr;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
bool SrvAnsi::IsCmdProcess() const
{
	// #condata This interface is proposed for connector only yet...
	return false;
}

void SrvAnsi::BellCallback(void* param)
{
	SrvAnsi* obj = static_cast<SrvAnsi*>(param);
	// #condata Add option to none/flash/beep?
	if (obj->mb_SuppressBells)
	{
		++obj->m_BellsCounter;
	}
	else
	{
		// #condata Use special sound?
		MessageBeep(MB_ICONINFORMATION);
	}
}

/* ************ Export ANSI printings ************ */

// ReSharper disable once CppMemberFunctionMayBeStatic
void SrvAnsi::DebugStringUtf8(LPCWSTR asMessage)
{
	#ifdef _DEBUG
	if (!asMessage || !*asMessage)
		return;
	// Only ConEmuC debugger is able to show UTF-8 encoded debug strings
	// So, set bUseUtf8 to false if VS debugger is required
	static bool bUseUtf8 = false;
	if (!bUseUtf8)
	{
		DebugString(asMessage);
		return;
	}
	const int iLen = wcslen(asMessage);
	char szUtf8[200];
	char* pszUtf8 = ((iLen * 3 + 5) < static_cast<int>(countof(szUtf8))) ? szUtf8 : static_cast<char*>(malloc(iLen * 3 + 5));
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
	if (pszUtf8 != szUtf8)
		free(pszUtf8);
	#endif
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SrvAnsi::FirstAnsiCall(const BYTE* lpBuf, DWORD nNumberOfBytes)
{
#ifdef SHOW_FIRST_ANSI_CALL
	static bool bTriggered = false;
	if (!bTriggered)
	{
		if (lpBuf && nNumberOfBytes && (*lpBuf == 0x1B || *lpBuf == CTRL('E') || *lpBuf == DSC))
		{
			bTriggered = true;
			ShowStartedMsgBox(L" First ansi call!");
		}
	}
#endif
}

void SrvAnsi::InitAnsiLog(LPCWSTR asFilePath, const bool LogAnsiCodes)
{
	gbAnsiLogCodes = LogAnsiCodes;

	// Already initialized?
	if (ghAnsiLogFile)
		return;

	ScopedObject(CLastErrorGuard);

	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hLog = CreateFile(asFilePath, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hLog && (hLog != INVALID_HANDLE_VALUE))
	{
		// Succeeded
		ghAnsiLogFile = hLog;
	}
}

void SrvAnsi::DoneAnsiLog(const bool bFinal)
{
	if (!ghAnsiLogFile)
		return;

	if (gbAnsiLogNewLine || (gnEnterPressed > 0))
	{
		SrvAnsi::WriteAnsiLogUtf8("\n", 1);
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
	}
}

// ReSharper disable once CppMemberFunctionMayBeStatic
UINT SrvAnsi::GetCodePage()
{
	//UINT cp = gCpConv.nDefaultCP ? gCpConv.nDefaultCP : GetConsoleOutputCP();
	//return cp;

	// #SrvAnsi Expected to be called from connector only
	return CP_UTF8;
}

// Intended to log some WinAPI functions
void SrvAnsi::WriteAnsiLogFormat(const char* format, ...)
{
	if (!ghAnsiLogFile || !format)
		return;
	ScopedObject(CLastErrorGuard);

	WriteAnsiLogTime();

	va_list argList;
	va_start(argList, format);
	char func_buffer[200] = "";
	if (S_OK == StringCchVPrintfA(func_buffer, countof(func_buffer), format, argList))
	{
		// #Connector Detect active executable
		char s_ExeName[80] = "connector";
		//if (!*s_ExeName)
		//	WideCharToMultiByte(CP_UTF8, 0, gsExeName, -1, s_ExeName, countof(s_ExeName)-1, nullptr, nullptr);

		char log_string[300] = "";
		msprintf(log_string, countof(log_string), "\x1B]9;11;\"%s: %s\"\x7", s_ExeName, func_buffer);
		WriteAnsiLogUtf8(log_string, static_cast<DWORD>(strlen(log_string)));
	}
	va_end(argList);
}

void SrvAnsi::WriteAnsiLogTime()
{
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
		WriteAnsiLogUtf8(time_str, strlen(time_str));
	}
}

bool SrvAnsi::WriteAnsiLogUtf8(const char* lpBuffer, DWORD nChars)
{
	if (!lpBuffer || !nChars)
		return FALSE;
	// Handle multi-thread writers
	// But multi-process writers can't be handled correctly
	SetFilePointer(ghAnsiLogFile, 0, nullptr, FILE_END);
	DWORD nWritten = 0;
	const BOOL bWrite = WriteFile(ghAnsiLogFile, lpBuffer, nChars, &nWritten, nullptr);
	UNREFERENCED_PARAMETER(nWritten);
	gnEnterPressed = 0; gbAnsiLogNewLine = false;
	gbAnsiWasNewLine = (lpBuffer[nChars-1] == '\n');
	return bWrite;
}

void SrvAnsi::WriteAnsiLogW(LPCWSTR lpBuffer, DWORD nChars)
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
	CEStrA longBuf;
	BOOL bWrite = FALSE;
	const int nNeed = WideCharToMultiByte(CP_UTF8, 0, lpBuffer, nChars, nullptr, 0, nullptr, nullptr);
	if (nNeed < 1)
		return;
	char* pszBuf = ((nNeed + iEnterShift + 1) <= static_cast<int>(countof(sBuf))) ? sBuf : longBuf.GetBuffer(nNeed+iEnterShift+1);
	if (!pszBuf)
		return;
	if (iEnterShift)
		pszBuf[0] = '\n';
	const int nLen = WideCharToMultiByte(CP_UTF8, 0, lpBuffer, nChars, pszBuf + iEnterShift, nNeed, nullptr, nullptr);
	// Must be OK, but check it
	if (nLen > 0 && nLen <= nNeed)
	{
		pszBuf[iEnterShift+nNeed] = 0;
		bWrite = WriteAnsiLogUtf8(pszBuf, nLen+iEnterShift);
	}
	std::ignore = bWrite;
}

void SrvAnsi::GetFeatures(bool* pbAnsiAllowed, bool* pbSuppressBells) const
{
	static std::chrono::steady_clock::time_point nLastCheck{};
	static bool bAnsiAllowed = true;
	static bool bSuppressBells = true;

	if ((std::chrono::steady_clock::now() - nLastCheck) > ANSI_MAP_CHECK_TIMEOUT)
	{
		CESERVER_CONSOLE_MAPPING_HDR* pMap = GetConMap();
		if (pMap)
		{
			//if (!::LoadSrvMapping(gState.realConWnd, *pMap) || !pMap->bProcessAnsi)
			//	bAnsiAllowed = false;
			//else
			//	bAnsiAllowed = true;

			bAnsiAllowed = ((pMap != nullptr) && ((pMap->Flags & ConEmu::ConsoleFlags::ProcessAnsi) != 0));
			bSuppressBells = ((pMap != nullptr) && ((pMap->Flags & ConEmu::ConsoleFlags::SuppressBells) != 0));

			//free(pMap);
		}
		nLastCheck = std::chrono::steady_clock::now();
	}

	if (pbAnsiAllowed)
		*pbAnsiAllowed = bAnsiAllowed;
	if (pbSuppressBells)
		*pbSuppressBells = bSuppressBells;
}



/*static*/ CONSOLE_SCREEN_BUFFER_INFO SrvAnsi::GetDefaultCSBI()
{
	static CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	if (csbi.dwSize.X > 0 && csbi.dwSize.Y > 0)
		return csbi;

	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!GetConsoleScreenBufferInfo(hOut, &csbi))
		throw condata::console_error("GetConsoleScreenBufferInfo failed");

	return csbi;
}

/*static*/ SHORT SrvAnsi::GetDefaultAnsiAttr()
{
	// Default console colors
	static SHORT clrDefault = 0;
	if (clrDefault)
		return clrDefault;

	const CONSOLE_SCREEN_BUFFER_INFO csbi = GetDefaultCSBI();

	static SHORT con2Ansi[16] = {0,4,2,6,1,5,3,7,8|0,8|4,8|2,8|6,8|1,8|5,8|3,8|7};
	clrDefault = con2Ansi[CONFORECOLOR(csbi.wAttributes)]
		| static_cast<SHORT>(con2Ansi[CONBACKCOLOR(csbi.wAttributes)] << 4);

	return clrDefault;
}

// ReSharper disable once CppMemberFunctionMayBeStatic
condata::Attribute SrvAnsi::GetDefaultAttr() const
{
	const CONSOLE_SCREEN_BUFFER_INFO csbi = GetDefaultCSBI();
	condata::Attribute result{};
	result.attr.attr = csbi.wAttributes;
	return result;
}

void SrvAnsi::ApplyDisplayParm()
{
	if (gDisplayParm.getDirty())
	{
		ReSetDisplayParm(false, true);
	}
}

void SrvAnsi::ReSetDisplayParm(bool bReset, bool bApply)
{
	WARNING("Эту функу нужно дергать при смене буферов, закрытии дескрипторов, и т.п.");

	if (bReset)
	{
		gDisplayParm.Reset(true);
	}

	if (bApply)
	{
		gDisplayParm.setDirty(false);

		condata::Attribute attr = {};

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


		// 30-37,38,39
		const auto TextColor = gDisplayParm.getTextColor();
		// 38
		const auto Text256 = gDisplayParm.getText256();
		// 40-47,48,49
		const auto BackColor = gDisplayParm.getBackColor();
		// 48
		const auto Back256 = gDisplayParm.getBack256();


		if (Text256)
		{
			if (Text256 == clr24b)
			{
				attr.flags |= condata::Attribute::fFore24b;
				attr.crForeColor = TextColor&0xFFFFFF;
			}
			else
			{
				if (TextColor > 15)
					attr.flags |= condata::Attribute::fFore24b;
				else
					attr.attr.foreIdx = RgbMap[TextColor & 0xF];
				attr.crForeColor = RgbMap[TextColor&0xFF];
			}
			if (attr.flags & condata::Attribute::fFore24b)
			{
				WORD n = 7;
				Far3Color::Color2FgIndex(attr.crForeColor & 0xFFFFFF, n);
				attr.attr.foreIdx = n;
			}
		}
		else if (TextColor & 0x8)
		{
			// Comes from CONSOLE_SCREEN_BUFFER_INFO::wAttributes
			attr.attr.foreIdx = ClrMap[TextColor&0x7]
				| 0x08;
		}
		else
		{
			attr.attr.foreIdx = ClrMap[TextColor&0x7]
				| ((gDisplayParm.getBrightFore() || (gDisplayParm.getBrightOrBold() && !gDisplayParm.getBrightBack())) ? 0x08 : 0);
		}

		if (gDisplayParm.getBrightOrBold() && (Text256 || gDisplayParm.getBrightFore() || gDisplayParm.getBrightBack()))
			attr.flags |= condata::Attribute::fBold;
		if (gDisplayParm.getItalic())
			attr.flags |= condata::Attribute::fItalic;
		if (gDisplayParm.getUnderline())
			attr.attr.attr |= COMMON_LVB_UNDERSCORE;
		if (gDisplayParm.getCrossed())
			attr.flags |= condata::Attribute::fCrossed;
		if (gDisplayParm.getInverse())
			attr.attr.attr |= COMMON_LVB_REVERSE_VIDEO;

		if (Back256)
		{
			if (Back256 == clr24b)
			{
				attr.flags |= condata::Attribute::fBack24b;
				attr.crBackColor = BackColor&0xFFFFFF;
			}
			else
			{
				if (BackColor > 15)
					attr.flags |= condata::Attribute::fBack24b;
				else
					attr.attr.backIdx = RgbMap[BackColor & 0xF];
				attr.crBackColor = RgbMap[BackColor&0xFF];
			}
			if (attr.flags & condata::Attribute::fBack24b)
			{
				WORD n = 0;
				Far3Color::Color2BgIndex(attr.crBackColor & 0xFFFFFF, (attr.flags & condata::Attribute::fFore24b) && (attr.crBackColor == attr.crForeColor), n);
				attr.attr.backIdx = n;
			}
		}
		else if (BackColor & 0x8)
		{
			// Comes from CONSOLE_SCREEN_BUFFER_INFO::wAttributes
			attr.attr.backIdx = ClrMap[BackColor&0x7]
				| 0x8;
		}
		else
		{
			attr.attr.backIdx = ClrMap[BackColor&0x7]
				| (gDisplayParm.getBrightBack() ? 0x8 : 0);
		}

		GetTable(GetTableEnum::current)->SetAttr(attr);
	}
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SrvAnsi::DumpEscape(LPCWSTR buf, size_t cchLen, DumpEscapeCodes iUnknown)
{
#if defined(DUMP_CONSOLE_OUTPUT)

	static const wchar_t szAnalogues[32] =
	{
		32, 9786, 9787, 9829, 9830, 9827, 9824, 8226, 9688, 9675, 9689, 9794, 9792, 9834, 9835, 9788,
		9658, 9668, 8597, 8252,  182,  167, 9632, 8616, 8593, 8595, 8594, 8592, 8735, 8596, 9650, 9660
	};

	if (!buf || !cchLen)
	{
		// There are programs which try to write empty strings
		// e.g. clink, perl, ...
		//_ASSERTEX((buf && cchLen) || (gszClinkCmdLine && buf));
	}

	wchar_t szDbg[200];
	size_t nLen = cchLen;
	static int nWriteCallNo = 0;

	switch (iUnknown)
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
		msprintf(szDbg, countof(szDbg), L"[%u] AnsiDump #%u: ", GetCurrentThreadId(), ++nWriteCallNo);
	}

	size_t nStart = lstrlenW(szDbg);
	wchar_t* pszDst = szDbg + nStart;
	wchar_t* pszFrom = szDbg;

	if (buf && cchLen)
	{
		const wchar_t* pszSrc = (wchar_t*)buf;
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
		size_t nMsgLen = lstrlenW(psEmptyMessage);
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
}

// ReSharper disable once CppMemberFunctionMayBeStatic
void SrvAnsi::SetCursorVisibility(bool visible)
{
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO ci = {};
	if (::GetConsoleCursorInfo(hOut, &ci))
	{
		ci.bVisible = visible;
		SetConsoleCursorInfo(hOut, &ci);
	}
}

void SrvAnsi::SetCursorShape(unsigned style)
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
	// ReSharper disable once CppLocalVariableMayBeConst
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO ci = {};
	if (GetConsoleCursorInfo(hOut, &ci))
	{
		// We can't implement all possible styles in RealConsole,
		// but we can use "Block/Underline" shapes...
		ci.dwSize = (style == 1 || style == 2) ? 100 : 15;
		SetConsoleCursorInfo(hOut, &ci);
	}
	ChangeTermMode(tmc_CursorShape, style);
}

// we need to ‘cache’ parts of non-translated MBCS chars (one UTF-8 symbol may be transmitted by up to *three* parts)
bool SrvAnsi::OurWriteConsoleA(const char* lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	bool lbRc = false;

	wchar_t* buf = nullptr;
	wchar_t szTemp[280]; // would be enough in most cases
	CEStr ptrTemp;
	ssize_t bufMax;

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

	SrvAnsiImpl writer(this, GetTable(GetTableEnum::current));

	// Nothing to write? Or flush buffer?
	if (!lpBuffer || !nNumberOfCharsToWrite)
	{
		if (lpNumberOfCharsWritten)
			*lpNumberOfCharsWritten = 0;
		lbRc = true;
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
			lbRc = writer.OurWriteConsole(buf, static_cast<DWORD>(pDst - buf), &nWritten);
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
		lbRc = writer.OurWriteConsole(buf, static_cast<DWORD>(pDst - buf), &nWritten);
		if (lbRc)
			nTotalWritten += nWritten;
	}

	// If succeeded
	if (lbRc)
	{
		// Issue 1291: Python fails to print string sequence with ASCII character followed by Chinese character.
		if (lpNumberOfCharsWritten)
			*lpNumberOfCharsWritten = nNumberOfCharsToWrite;

		// for pty debug purposes
		#ifdef _DEBUG
		static std::atomic_size_t buffered{ 0 };
		static std::atomic<std::chrono::steady_clock::time_point> lastTick{};
		buffered += nNumberOfCharsToWrite;
		if ((std::chrono::steady_clock::now() - lastTick.load()) >= std::chrono::seconds(5))
		{
			// std::unique_lock<std::mutex> lock(m_UseMutex); -- writer already locked the mutex
			GetTable(GetTableEnum::current)->DebugDump(true);
			buffered = 0;
			lastTick = std::chrono::steady_clock::now();
		}
		#endif
	}

fin:
	++m_FrameVersion;
	std::ignore = pTokenStart;
	std::ignore = nTotalWritten;
	return lbRc;
}

bool SrvAnsi::OurWriteConsoleW(const wchar_t* lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten)
{
	bool lbRc = false;

	SrvAnsiImpl writer(this, GetTable(GetTableEnum::current));

	// The output
	lbRc = writer.OurWriteConsole(lpBuffer, nNumberOfCharsToWrite, lpNumberOfCharsWritten);
	++m_FrameVersion;
	return lbRc;
}

void SrvAnsi::SetConPtyInput(HANDLE input)
{
	{
		std::lock_guard<std::mutex> lock(m_InputMutex);
		m_ConPtyInput = input;
		m_ConPtyInputError = ERROR_SUCCESS;
	}
	if (input)
	{
		m_Primary.SetBackscrollMax(ConPtyHistoryLimit);
		m_Primary.SetReflow(true);
		m_Alternative.SetUnicode(true);
		ChangeTermMode(tmc_TerminalType, te_conpty);
	}
	if (!input && gbIsXTermOutput)
		StartXTermMode(false);
}

bool SrvAnsi::WriteTerminalInput(const char* data, DWORD size)
{
	std::lock_guard<std::mutex> lock(m_InputMutex);
	if (!m_ConPtyInput.load() || m_ConPtyInputError.load()) return false;
	if (WriteConPtyInput(m_ConPtyInput.load(), data, size)) return true;
	m_ConPtyInputError = GetLastError() ? GetLastError() : ERROR_WRITE_FAULT;
	return false;
}

bool SrvAnsi::ReportTerminalString(const wchar_t* text)
{
	const int size = WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
	if (!size)
		return false;
	std::vector<char> bytes(size);
	WideCharToMultiByte(CP_UTF8, 0, text, -1, bytes.data(), size, nullptr, nullptr);
	return WriteTerminalInput(bytes.data(), size - 1);
}

bool SrvAnsi::ReadTerminalFrame(COORD size, std::vector<condata::CharInfo>& cells, COORD& cursor, uint64_t& version,
	unsigned& history, std::map<unsigned, std::wstring>& links, std::map<unsigned, std::wstring>& graphemes)
{
	std::lock_guard<std::mutex> lock(m_UseMutex);
	if (size.X < 1 || size.Y < 1 || static_cast<unsigned>(size.Y) > ConPtyCellCapacity / size.X)
		throw condata::console_error("ConPTY viewport exceeds frame capacity");
	m_Primary.SetBackscrollMax(std::min<unsigned>({ConPtyHistoryLimit, ConPtyCellCapacity / size.X - size.Y, static_cast<unsigned>(32767 - size.Y)}));
	const auto previous = m_Primary.GetSize();
	if (previous.x != static_cast<unsigned>(size.X) || previous.y != size.Y)
	{
		m_Primary.Resize({static_cast<unsigned>(size.X), size.Y});
		m_Alternative.Resize({static_cast<unsigned>(size.X), size.Y});
		++m_FrameVersion;
	}
	if (version == m_FrameVersion)
		return false;
	auto* table = GetTable(GetTableEnum::current);
	history = m_UsePrimary ? table->GetBackscroll() : 0;
	cells.resize(static_cast<size_t>(size.X) * (size.Y + history));
	table->GetData(cells.data(), {static_cast<unsigned>(size.X), static_cast<int>(size.Y + history)},
		{0, -static_cast<int>(history), size.X - 1, size.Y - 1});
	links.clear();
	graphemes.clear();
	for (const auto& cell : cells)
	{
		const auto glyph = table->GetGraphemes().find(cell.attr.graphemeId);
		if (glyph != table->GetGraphemes().end()) graphemes.insert(*glyph);
		const auto link = m_Hyperlinks.find(cell.attr.hyperlinkId);
		if (link != m_Hyperlinks.end()) links.insert(*link);
	}
	const auto pos = table->GetCursor();
	cursor = {static_cast<SHORT>(std::min<unsigned>(pos.x, size.X - 1)), static_cast<SHORT>(std::max(0, std::min(pos.y, size.Y - 1)))};
	version = m_FrameVersion;
	return true;
}

void SrvAnsi::SetHyperlink(const std::wstring& uri)
{
	m_ActiveHyperlink = 0;
	if (!IsConPtyUriAllowed(uri) || !m_NextHyperlink)
		return;
	for (const auto& link : m_Hyperlinks)
	{
		if (link.second == uri) { m_ActiveHyperlink = link.first; return; }
	}
	if (m_Hyperlinks.size() >= ConPtyLinkCapacity || m_UriChars + uri.size() > ConPtyUriCapacity)
		PruneHyperlinks();
	if (m_Hyperlinks.size() >= ConPtyLinkCapacity || m_UriChars + uri.size() > ConPtyUriCapacity)
		return;
	m_ActiveHyperlink = m_NextHyperlink++;
	m_Hyperlinks.emplace(m_ActiveHyperlink, uri);
	m_UriChars += uri.size();
}

void SrvAnsi::PruneHyperlinks()
{
	std::set<unsigned> used;
	std::vector<condata::CharInfo> cells;
	for (auto* table : {&m_Primary, &m_Alternative})
	{
		const auto size = table->GetSize();
		const int history = table->GetBackscroll();
		cells.resize(static_cast<size_t>(size.x) * (size.y + history));
		table->GetData(cells.data(), {size.x, size.y + history}, {0, -history, static_cast<int>(size.x) - 1, size.y - 1});
		for (const auto& cell : cells) if (cell.attr.hyperlinkId) used.insert(cell.attr.hyperlinkId);
	}
	for (auto link = m_Hyperlinks.begin(); link != m_Hyperlinks.end();)
	{
		if (!used.count(link->first)) { m_UriChars -= link->second.size(); link = m_Hyperlinks.erase(link); }
		else ++link;
	}
}

std::pair<HANDLE, HANDLE> SrvAnsi::GetClientHandles(DWORD clientPID)
{
	//_ASSERTE(FALSE && "Continue to SrvAnsi::GetClientHandles");
	auto clientHandles = m_pipes.GetClientHandles(clientPID);
	if (!clientHandles.first || !clientHandles.second)
	{
		_ASSERTE(clientHandles.first && clientHandles.second);
		return std::pair<HANDLE, HANDLE>{};
	}

	if (!m_pipe_thread[0])
	{
		m_pipe_thread[0] = MThread(PipeThread, new PipeArg{this, true/*input*/}, "SrvAnsi::In::%u", clientPID);
	}
	if (!m_pipe_thread[1])
	{
		m_pipe_thread[1] = MThread(PipeThread, new PipeArg{this, false/*output*/}, "SrvAnsi::Out::%u", clientPID);
	}

	return clientHandles;
}

DWORD WINAPI SrvAnsi::PipeThread(LPVOID pipe_arg)
{
	PipeArg* p = static_cast<PipeArg*>(pipe_arg);

	if (p->input)
	{
		INPUT_RECORD pir[128];
		// ReSharper disable once CppLocalVariableMayBeConst
		HANDLE hTermInput = GetStdHandle(STD_INPUT_HANDLE);
		while (!p->self->m_pipe_stop)
		{
			// #condata Replace with internal input buffer
			DWORD peek = 0, read = 0;
			const BOOL bRc = (PeekConsoleInputW(hTermInput, pir, static_cast<DWORD>(std::size(pir)), &peek) && (peek != 0))
				? ReadConsoleInputW(hTermInput, pir, peek, &read)
				: FALSE;
			if (bRc && read)
			{
				if (!p->self->m_pipes.Write(pir, read * sizeof(pir[0])))
				{
					_ASSERTE(FALSE && "input pipe was closed?");
					break;
				}
			}
			Sleep(10);
		}
	}
	else
	{
		while (!p->self->m_pipe_stop)
		{
			const auto data = p->self->m_pipes.Read(true);
			if (!data.first)
			{
				_ASSERTE(FALSE && "output pipe was closed?");
				break;
			}
			if (data.second)
			{
				p->self->OurWriteConsoleA(static_cast<const char*>(data.first), data.second, nullptr);
			}
		}
	}

	delete p;
	return 0;
}

void SrvAnsi::ChangeTermMode(TermModeCommand mode, DWORD value, DWORD nPID /*= 0*/)
{
	// ReSharper disable once CppDeclaratorDisambiguatedAsFunction
	extern BOOL cmd_StartXTerm(CESERVER_REQ& in, CESERVER_REQ** out);

	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_STARTXTERM, sizeof(CESERVER_REQ_HDR) + 3 * sizeof(DWORD));
	if (pIn)
	{
		pIn->dwData[0] = mode;
		pIn->dwData[1] = value;  // NOLINT(clang-diagnostic-array-bounds)
		pIn->dwData[2] = nPID ? nPID : GetCurrentProcessId();  // NOLINT(clang-diagnostic-array-bounds)
		CESERVER_REQ* pOut = nullptr;
		if (gState.runMode_ == RunMode::Server || gState.runMode_ == RunMode::AltServer)
			cmd_StartXTerm(*pIn, &pOut);
		else if (auto* mapping = GetConMap())
			pOut = ExecuteSrvCmd(mapping->nServerPID, pIn, gState.realConWnd_);
		ExecuteFreeResult(pIn);
		ExecuteFreeResult(pOut);
	}

	if (mode < countof(gWasXTermModeSet))
	{
		gWasXTermModeSet[mode] = {value, nPID ? nPID : GetCurrentProcessId()};
	}
}

void SrvAnsi::StartXTermMode(const bool bStart)
{
	// May be triggered by ConEmuT, official Vim builds and in some other cases
	//_ASSERTEX((gXTermAltBuffer.Flags & xtb_AltBuffer) && (gbIsXTermOutput!=bStart));
	_ASSERTEX(gbIsXTermOutput!=bStart);

	StartXTermOutput(bStart);

	// Remember last mode and pass to server
	ChangeTermMode(tmc_TerminalType, bStart ? (IsConPty() ? te_conpty : te_xterm) : te_win32);
}

void SrvAnsi::StartXTermOutput(const bool bStart)
{
	// Remember last mode
	gbIsXTermOutput = bStart;
}

void SrvAnsi::RefreshXTermModes()
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
