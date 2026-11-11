
//TODO: Нотация.
//TODO: XXX       - вернуть значение переменной, возможно скорректировав ее
//TODO: GetXXX    - вернуть значение переменной
//TODO: SetXXX    - установить значение переменной, команда в консоль не посылается!
//TODO: ChangeXXX - послать в консоль и установить значение переменной

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
#define SHOWDEBUGSTR
//#define ALLOWUSEFARSYNCHRO

#include "Header.h"
#include <tlhelp32.h>

#include "../common/ConEmuCheck.h"
#include "../common/EnvVar.h"
#include "../common/MGlobal.h"
#include "../common/MSetter.h"
#include "../common/RgnDetect.h"
#include "../common/WConsoleInfo.h"
#include "../common/wcchars.h"
#include "../common/wcwidth.h"
#include "../common/WFiles.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "FindDlg.h"
#include "HtmlCopy.h"
#include "Match.h"
#include "Menu.h"
#include "MyClipboard.h"
#include "OptionsClass.h"
#include "RConPalette.h"
#include "RealBuffer.h"
#include "RealConsole.h"
#include "SetPgDebug.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "VConText.h"
#include "VirtualConsole.h"

#include <cmath>
#include <map>
#include <set>

#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRSIZE2(s) DEBUGSTR(s) // Warning level
#define DEBUGSTRDYNAMIC(s) //DEBUGSTR(s)
#define DEBUGSTRPKT(s) //DEBUGSTR(s)
#define DEBUGSTRKEY(s) //DEBUGSTR(s)
#define DEBUGSTRCURSORPOS(s) //DEBUGSTR(s)
#define DEBUGSTRMOUSE(s) //DEBUGSTR(s)
#define DEBUGSTRTOPLEFT(s) //DEBUGSTR(s)
#define DEBUGSTRTRUEMOD(s) //DEBUGSTR(s)
#define DEBUGSTRLINK(s) //DEBUGSTR(s)
#define DEBUGSTRSEL(s) //DEBUGSTR(s)
#define DEBUGSTRTIMEOUTS(s) //DEBUGSTR(s)

// ANSI, without "\r\n"
#define IFLOGCONSOLECHANGE gpSet->isLogging(2)
#define LOGCONSOLECHANGE(s) if (IFLOGCONSOLECHANGE) mp_RCon->LogString(s)

#ifndef CONSOLE_MOUSE_DOWN
#define CONSOLE_MOUSE_DOWN 8
#endif

#define SEL_MOUSE_AUTOSCROLL_DELTA 15
#define SEL_MOUSE_AUTOSCROLL_PIX   2

#ifdef _DEBUG
#define HEAPVAL MCHKHEAP
#else
#define HEAPVAL
#endif

//#define Assert(V) if ((V)==FALSE) { wchar_t szAMsg[MAX_PATH*2]; swprintf_c(szAMsg, L"Assertion (%s) at\n%s:%i", _T(#V), _T(__FILE__), __LINE__); CRealConsole::Box(szAMsg); }

const DWORD gnKeyBarFlags[] = {0,
	LEFT_CTRL_PRESSED, LEFT_ALT_PRESSED, SHIFT_PRESSED,
	LEFT_CTRL_PRESSED|LEFT_ALT_PRESSED, LEFT_CTRL_PRESSED|SHIFT_PRESSED,
	LEFT_ALT_PRESSED|SHIFT_PRESSED,
	RIGHT_CTRL_PRESSED, RIGHT_ALT_PRESSED,
	RIGHT_CTRL_PRESSED|RIGHT_ALT_PRESSED, RIGHT_CTRL_PRESSED|SHIFT_PRESSED,
	RIGHT_ALT_PRESSED|SHIFT_PRESSED
};




CRealBuffer::CRealBuffer(CRealConsole* apRCon, RealBufferType aType /*= rbt_Primary*/)
{
	mp_RCon = apRCon;
	m_Type = aType;
	mn_LastRgnFlags = -1;

	mcs_Data.Init();

	ZeroStruct(con);
	con.TopLeft.Reset();
	mp_Match = nullptr;

	bufferModeChangeLocked_ = FALSE;
	mcr_LastMousePos = MakeCoord(-1,-1);

	mb_LeftPanel = mb_RightPanel = FALSE;
	mr_LeftPanel = mr_LeftPanelFull = RECT{};
	mr_RightPanel = mr_RightPanelFull = RECT{};

	ZeroStruct(dump);

	m_TrueMode.pcsLock = new MSectionSimple(true);
	m_TrueMode.mp_Cmp = nullptr;
	m_TrueMode.nCmpMax = 0;

	mpp_RunHyperlink = nullptr;
}

CRealBuffer::~CRealBuffer()
{
	// Is not critical anymore, it's locked just for flickering minimization
	_ASSERTE(con.nInGetConsoleData==0);

	m_ConData.Release();

	dump.Close();

	SafeDelete(m_TrueMode.pcsLock);
	SafeFree(m_TrueMode.mp_Cmp);

	SafeDelete(mp_Match);

	if (mpp_RunHyperlink)
	{
		SafeCloseHandle(mpp_RunHyperlink->hProcess);
		SafeFree(mpp_RunHyperlink);
	}
}

void CRealBuffer::ReleaseMem()
{
	TODO("Освободить не используемую память");
	m_Type = rbt_Undefined;
	dump.Close();
}

// Вызывается после CVirtualConsole::Dump и CRealConsole::DumpConsole
// Это последний, третий блок в файле дампа
void CRealBuffer::DumpConsole(HANDLE ahFile)
{
	BOOL lbRc = FALSE;
	DWORD dw = 0;

	CRConDataGuard data;
	if (GetData(data))
	{
		MSectionLock sc; sc.Lock(&csCON, FALSE);
		DWORD cchMax = std::min((DWORD)(con.nTextWidth * con.nTextHeight), LODWORD(data->nMaxCells));
		lbRc = WriteFile(ahFile, data->pConChar, cchMax*sizeof(*data->pConChar), &dw, nullptr);
		lbRc &= WriteFile(ahFile, data->pConAttr, cchMax*sizeof(*data->pConAttr), &dw, nullptr);
	}

	UNREFERENCED_PARAMETER(lbRc);
}


bool CRealBuffer::LoadDumpConsole(LPCWSTR asDumpFile)
{
	bool lbRc = false;
	HANDLE hFile = nullptr;
	LARGE_INTEGER liSize;
	COORD cr = {};
	wchar_t* pszDumpTitle, *pszRN, *pszSize;
	unsigned nX = 0;
	unsigned nY = 0;
	DWORD dwConDataBufSize = 0;
	DWORD dwConDataBufSizeEx = 0;
	CRConDataGuard data;

	SetSelectionFlags(0);
	dump.Close();

	if (!asDumpFile || !*asDumpFile)
	{
		_ASSERTE(asDumpFile!=nullptr && *asDumpFile);
		goto wrap;
	}

	hFile = CreateFile(asDumpFile, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (!hFile || hFile == INVALID_HANDLE_VALUE)
	{
		DisplayLastError(L"Can't open dump file for reading");
		goto wrap;
	}
	if (!GetFileSizeEx(hFile, &liSize) || !liSize.LowPart || liSize.HighPart)
	{
		DisplayLastError(L"Invalid dump file size", -1);
		goto wrap;
	}

	dump.ptrData = (LPBYTE)malloc(liSize.LowPart);
	if (!dump.ptrData)
	{
		_ASSERTE(dump.ptrData!=nullptr);
		goto wrap;
	}
	dump.cbDataSize = liSize.LowPart;

	if (!ReadFile(hFile, dump.ptrData, liSize.LowPart, (LPDWORD)&liSize.HighPart, nullptr) || (liSize.LowPart != (DWORD)liSize.HighPart))
	{
		DisplayLastError(L"Dump file reading failed");
		goto wrap;
	}
	CloseHandle(hFile); hFile = nullptr;

	// Поехали
	pszDumpTitle = (wchar_t*)dump.ptrData;

	pszRN = wcschr(pszDumpTitle, L'\r');
	if (!pszRN)
	{
		DisplayLastError(L"Dump file invalid format (title)", -1);
		goto wrap;
	}
	*pszRN = 0;
	pszSize = pszRN + 2;

	if (wcsncmp(pszSize, L"Size: ", 6))
	{
		DisplayLastError(L"Dump file invalid format (Size start)", -1);
		goto wrap;
	}

	pszRN = wcschr(pszSize, L'\r');

	if (!pszRN)
	{
		DisplayLastError(L"Dump file invalid format (Size line end)", -1);
		goto wrap;
	}

	// Первый блок
	dump.pszBlock1 = pszRN + 2;

	pszSize += 6;
	dump.crSize.X = (SHORT)wcstol(pszSize, &pszRN, 10);

	if (!pszRN || *pszRN!=L'x')
	{
		DisplayLastError(L"Dump file invalid format (Size.X)", -1);
		goto wrap;
	}

	pszSize = pszRN + 1;
	dump.crSize.Y = (SHORT)wcstol(pszSize, &pszRN, 10);

	if (!pszRN || (*pszRN!=L' ' && *pszRN!=L'\r'))
	{
		DisplayLastError(L"Dump file invalid format (Size.Y)", -1);
		goto wrap;
	}

	pszSize = pszRN;
	dump.crCursor.X = 0; dump.crCursor.Y = dump.crSize.Y-1;

	if (*pszSize == L' ')
	{
		while(*pszSize == L' ') pszSize++;

		if (wcsncmp(pszSize, L"Cursor: ", 8)==0)
		{
			pszSize += 8;
			cr.X = (SHORT)wcstol(pszSize, &pszRN, 10);

			if (!pszRN || *pszRN!=L'x')
			{
				DisplayLastError(L"Dump file invalid format (Cursor)", -1);
				goto wrap;
			}

			pszSize = pszRN + 1;
			cr.Y = (SHORT)wcstol(pszSize, &pszRN, 10);

			if (cr.X>=0 && cr.Y>=0)
			{
				dump.crCursor.X = cr.X; dump.crCursor.Y = cr.Y;
			}
		}
	}

	dwConDataBufSize = dump.crSize.X * dump.crSize.Y;
	dwConDataBufSizeEx = dump.crSize.X * dump.crSize.Y * sizeof(CharAttr);

	dump.pcaBlock1 = (CharAttr*)(((WORD*)(dump.pszBlock1)) + dwConDataBufSize);
	dump.Block1 = (((LPBYTE)(((LPBYTE)(dump.pcaBlock1)) + dwConDataBufSizeEx)) - dump.ptrData) <= (INT_PTR)dump.cbDataSize;

	if (!dump.Block1)
	{
		DisplayLastError(L"Dump file invalid format (Block1)", -1);
		goto wrap;
	}

	m_Type = rbt_DumpScreen;


	// При активации дополнительного буфера нельзя выполнять SyncWindow2Console.
	// Привести видимую область буфера к текущей.
	nX = mp_RCon->TextWidth();
	nY = mp_RCon->TextHeight();


	// Почти все готово, осталась инициализация
	ZeroStruct(con.m_sel);
	con.m_ci.dwSize = 15; con.m_ci.bVisible = TRUE;
	ZeroStruct(con.m_sbi);
	con.m_dwConsoleCP = con.m_dwConsoleOutputCP = 866;
	con.m_dwConsoleInMode = 0; con.m_dwConsoleOutMode = 0;
	con.m_sbi.dwSize = dump.crSize;
	con.m_sbi.dwCursorPosition = dump.crCursor;
	con.m_sbi.wAttributes = 7;
	con.m_sbi.srWindow.Left = 0;
	con.m_sbi.srWindow.Top = 0;
	con.m_sbi.srWindow.Right = nX - 1;
	con.m_sbi.srWindow.Bottom = nY - 1;
	con.crMaxSize = mp_RCon->mp_RBuf->con.crMaxSize; //MakeCoord(std::max(dump.crSize.X,nX),std::max(dump.crSize.Y,nY));
	con.m_sbi.dwMaximumWindowSize = con.crMaxSize; //dump.crSize;
	SetTopLeft();
	con.nTextWidth = nX/*dump.crSize.X*/;
	con.nTextHeight = nY/*dump.crSize.Y*/;
	con.nBufferHeight = dump.crSize.Y;
	con.bBufferHeight = TRUE;
	TODO("Горизонтальная прокрутка");

	//dump.NeedApply = TRUE;

	// Создание буферов
	if (!InitBuffers(&data))
	{
		_ASSERTE(FALSE);
		goto wrap;
	}
	else if (!data.isValid(dwConDataBufSize))
	{
		_ASSERTE(FALSE && "Must be initialized!");
		goto wrap;
	}
	else
	// and do the copy
	{
		wchar_t*  pszSrc = dump.pszBlock1;
		CharAttr* pcaSrc = dump.pcaBlock1;
		wchar_t*  pszDst = data->pConChar;
		TODO("Хорошо бы весь расширенный буфер тут хранить, а не только CHAR_ATTR");
		WORD*     pnaDst = data->pConAttr;

		wmemmove(pszDst, pszSrc, dwConDataBufSize);

		WARNING("Похоже, что это сейчас не требуется, все равно будет перечитано");
		// Расфуговка буфера CharAttr на консольные атрибуты
		for (DWORD n = 0; n < dwConDataBufSize; n++, pcaSrc++, pnaDst++)
		{
			*pnaDst = MAKECONCOLOR(pcaSrc->nForeIdx, pcaSrc->nBackIdx);
		}
	}

	con.mb_ConDataValid = true;
	con.bConsoleDataChanged = TRUE;

	lbRc = true; // OK
wrap:
	if (hFile && hFile != INVALID_HANDLE_VALUE)
		CloseHandle(hFile);
	if (!lbRc)
		dump.Close();
	return lbRc;
}

bool CRealBuffer::LoadDataFromDump(const CONSOLE_SCREEN_BUFFER_INFO& storedSbi, const CHAR_INFO* pData, DWORD cchMaxCellCount)
{
	bool lbRc = false;
	LARGE_INTEGER liSize;
	//COORD cr = {};
	unsigned nX = 0;
	unsigned nY = 0;

	dump.Close();

	dump.crSize = storedSbi.dwSize;
	dump.crCursor = storedSbi.dwCursorPosition;

	DWORD cchCellCount = storedSbi.dwSize.X * storedSbi.dwSize.Y;
	// Памяти выделяем "+1", чтобы wchar_t* был ASCIIZ
	liSize.QuadPart = (cchCellCount + 1) * (sizeof(CharAttr) + sizeof(wchar_t));
	if (!cchCellCount || !liSize.LowPart || liSize.HighPart)
	{
		DisplayLastError(L"Invalid dump file size", -1);
		goto wrap;
	}

	if (!cchMaxCellCount || (cchMaxCellCount < cchCellCount))
	{
		DisplayLastError(L"Invalid max cell count", -1);
		goto wrap;
	}

	dump.ptrData = (LPBYTE)malloc(liSize.LowPart);
	if (!dump.ptrData)
	{
		_ASSERTE(dump.ptrData!=nullptr);
		goto wrap;
	}
	dump.cbDataSize = liSize.LowPart;

	// Поехали

	// Первый (и единственный) блок
	dump.pszBlock1 = (wchar_t*)dump.ptrData;
	dump.pszBlock1[cchCellCount] = 0;
	dump.pcaBlock1 = (CharAttr*)(dump.pszBlock1 + cchCellCount + 1);
	dump.Block1 = TRUE;


	m_Type = rbt_Alternative;


	// При активации дополнительного буфера нельзя выполнять SyncWindow2Console.
	// Привести видимую область буфера к текущей.
	nX = mp_RCon->TextWidth();
	nY = mp_RCon->TextHeight();


	// Почти все готово, осталась инициализация
	ZeroStruct(con.m_sel);
	con.m_ci.dwSize = 15; con.m_ci.bVisible = TRUE;
	ZeroStruct(con.m_sbi);
	con.m_dwConsoleCP = con.m_dwConsoleOutputCP = 866;
	con.m_dwConsoleInMode = 0; con.m_dwConsoleOutMode = 0;
	con.m_sbi.dwSize = dump.crSize;
	con.m_sbi.dwCursorPosition = dump.crCursor;
	con.m_sbi.wAttributes = 7;

	TOPLEFTCOORD NewTopLeft; NewTopLeft.Reset();
	if (mp_RCon->mp_ABuf && (mp_RCon->mp_ABuf != this))
		mp_RCon->mp_ABuf->ConsoleScreenBufferInfo(nullptr, nullptr, &NewTopLeft);
	Assert(m_Type != rbt_Primary); // Dumps must NOT be loaded into Primary buffer
	SetTopLeft(NewTopLeft.y, NewTopLeft.x, false);

	if (NewTopLeft.isLocked() && NewTopLeft.x >= 0)
		con.m_sbi.srWindow.Right = std::min((NewTopLeft.x + (int)nX - 1),((int)storedSbi.dwSize.X - 1));
	else
		con.m_sbi.srWindow.Right = std::min(((int)nX - 1),((int)storedSbi.dwSize.X - 1));
	con.m_sbi.srWindow.Left = std::max(0,con.m_sbi.srWindow.Right - (int)nX + 1);

	if (NewTopLeft.isLocked() && NewTopLeft.y >= 0)
		con.m_sbi.srWindow.Bottom = std::min((NewTopLeft.y + (int)nY - 1),(storedSbi.dwSize.Y - 1));
	else
		con.m_sbi.srWindow.Bottom = std::min((storedSbi.srWindow.Top + (int)nY - 1),(storedSbi.dwSize.Y - 1));
	con.m_sbi.srWindow.Top = std::max(0,con.m_sbi.srWindow.Bottom - (int)nY + 1);

	con.crMaxSize = mp_RCon->mp_RBuf->con.crMaxSize; //MakeCoord(std::max(dump.crSize.X,nX),std::max(dump.crSize.Y,nY));
	con.m_sbi.dwMaximumWindowSize = con.crMaxSize; //dump.crSize;
	con.nTextWidth = nX/*dump.crSize.X*/;
	con.nTextHeight = nY/*dump.crSize.Y*/;
	con.bBufferHeight = (dump.crSize.Y > (int)nY);
	con.nBufferHeight = con.bBufferHeight ? dump.crSize.Y : 0;
	TODO("Горизонтальная прокрутка");

	//dump.NeedApply = TRUE;

	// and do the copy
	{
		TODO("Хорошо бы весь расширенный буфер тут хранить, а не только CHAR_INFO");

		const CHAR_INFO* ptrSrc = pData;
		CharAttr* pcaDst = dump.pcaBlock1;
		wchar_t*  pszDst = dump.pszBlock1;
		wchar_t   ch;

		CharAttr *lcaTableOrg = nullptr;
		PrepareColorTable(nullptr, nullptr, &lcaTableOrg);

		std::vector<AnnotationInfo> frozenColors;
		AnnotationHeader frameHeader = {};
		const AnnotationInfo* frameCells = nullptr;
		bool absoluteColors = false;
		if (mp_RCon->IsConPty() && mp_RCon->GetColorData(frameHeader, frameCells, absoluteColors)
			&& absoluteColors && frameHeader.bufferSize >= cchCellCount)
		{
			frozenColors.assign(frameCells, frameCells + cchCellCount);
			AnnotationHeader after = {}; const AnnotationInfo* unused = nullptr; bool absolute = false;
			if (!mp_RCon->GetColorData(after, unused, absolute) || after.flushCounter != frameHeader.flushCounter)
				frozenColors.clear();
		}

		DWORD nMax = std::min(cchCellCount,cchMaxCellCount);
		// Расфуговка буфера на консольные атрибуты
		for (DWORD n = 0; n < nMax; n++, ptrSrc++, pszDst++, pcaDst++)
		{
			ch = ptrSrc->Char.UnicodeChar;
			//2009-09-25. Некоторые (старые?) программы умудряются засунуть в консоль символы (ASC<32)
			//            их нужно заменить на юникодные аналоги
			*pszDst = (ch < 32) ? gszAnalogues[(WORD)ch] : ch;

			*pcaDst = lcaTableOrg[ptrSrc->Attributes & 0xFF];
			if (n < frozenColors.size())
			{
				const auto& color = frozenColors[n];
				if (color.fg_valid) pcaDst->crForeColor = color.fg_color;
				if (color.bk_valid) pcaDst->crBackColor = color.bk_color;
				if (color.style & AI_STYLE_REVERSE) { const unsigned fore = pcaDst->crForeColor; pcaDst->crForeColor = pcaDst->crBackColor; pcaDst->crBackColor = fore; }
				pcaDst->nFontIndex = color.style & fnt_StdFontMask;
			}


			//WARNING("Похоже, что это сейчас не требуется, все равно будет перечитано");
			//*pnaDst = ptrSrc->Attributes;
		}

		if (cchCellCount > cchMaxCellCount)
		{
			_ASSERTE(cchCellCount <= cchMaxCellCount);
			for (DWORD n = nMax; n < cchCellCount; n++, pszDst++, pcaDst++)
			{
				*pszDst = L' ';
				*pcaDst = lcaTableOrg[0];
			}
		}

		//WARNING("Похоже, что это сейчас не требуется, все равно будет перечитано");
		//wmemmove(con.pConChar, dump.pszBlock1, cchCellCount);
	}

	con.mb_ConDataValid = true;
	con.bConsoleDataChanged = TRUE;

	lbRc = true; // OK
wrap:
	if (!lbRc)
		dump.Close();
	return lbRc;
}

// 1 - Last long console output, 2 - current console "image", 3 - copy of rbt_Primary
bool CRealBuffer::LoadAlternativeConsole(LoadAltMode iMode /*= lam_Default*/)
{
	bool lbRc = false;

	SetSelectionFlags(0);
	dump.Close();

	DWORD nStartTick = GetTickCount(), nDurationTick;

	if (iMode == lam_Default)
	{
		if (mp_RCon->isFar() && gpSet->AutoBufferHeight)
		{
			iMode = lam_LastOutput;
		}
		else
		{
			// При открытии Vim (например) буфер отключается и переходим в режим без прокрутки
			// А вот старый буфер как раз и хочется посмотреть
			bool lbAltBufSwitched = false;
			if (!mp_RCon->isBufferHeight())
			{
				mp_RCon->GetServerPID();

				CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_ALTBUFFERSTATE, sizeof(CESERVER_REQ_HDR));
				if (pIn)
				{
					CESERVER_REQ *pOut = ExecuteSrvCmd(mp_RCon->GetServerPID(), pIn, ghWnd);
					if (pOut && (pOut->DataSize() >= sizeof(DWORD)))
					{
						lbAltBufSwitched = (pOut->dwData[0]!=0);
					}
					ExecuteFreeResult(pOut);
					ExecuteFreeResult(pIn);
				}
			}

			iMode = lbAltBufSwitched ? lam_LastOutput : lam_FullBuffer;
		}
	}

	if ((iMode == lam_LastOutput) || (iMode == lam_FullBuffer))
	{
		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_CONSOLEFULL, sizeof(CESERVER_REQ_HDR) + 2 * sizeof(DWORD));
		if (pIn)
		{
			int dynHeight = mp_RCon->mp_RBuf->GetDynamicHeight();
			if (iMode == lam_FullBuffer)
			{
				pIn->dwData[0] = (dynHeight > 0) ? dynHeight : 0;
				pIn->dwData[1] = FALSE;
			}
			else
			{
				pIn->dwData[0] = 0;
				pIn->dwData[1] = TRUE;
			}
			CESERVER_REQ *pOut = ExecuteSrvCmd(mp_RCon->GetServerPID(), pIn, ghWnd);
			if (pOut && (pOut->hdr.cbSize > sizeof(CESERVER_CONSAVE_MAP)))
			{
				const CESERVER_CONSAVE_MAP* pInfo = (const CESERVER_CONSAVE_MAP*)pOut;
				// Go
				lbRc = LoadDataFromDump(pInfo->info, pInfo->Data, pInfo->MaxCellCount);
			}
			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
		}
	}
	else
	{
		// Unsupported yet...
		_ASSERTE(iMode==lam_LastOutput || iMode==lam_FullBuffer);
	}

	// final checks
	if (!lbRc)
		dump.Close();
	// Logging
	nDurationTick = GetTickCount() - nStartTick;
	if (!lbRc)
	{
		mp_RCon->LogString(L"!!! CRealBuffer::LoadAlternativeConsole FAILED !!!");
	}
	else if (nDurationTick > 1000)
	{
		wchar_t szLog[80];
		swprintf_c(szLog, L"!!! CRealBuffer::LoadAlternativeConsole takes %u ms !!!", nDurationTick);
		if (!mp_RCon->LogString(szLog))
		{
			DEBUGSTRTIMEOUTS(szLog);
		}
		_ASSERTE(!lbRc || (nDurationTick < 3000));
	}
	return lbRc;
}

bool CRealBuffer::SetConsoleSizeSrv(USHORT sizeX, USHORT sizeY, USHORT sizeBuffer, DWORD anCmdID /*= CECMD_SETSIZESYNC*/)
{
	if (!this) return FALSE;

	_ASSERTE(m_Type == rbt_Primary);
	#ifdef _DEBUG
	int nVConNo = gpConEmu->isVConValid(mp_RCon->VCon());
	#endif

	if (!mp_RCon->hConWnd || mp_RCon->ms_ConEmuC_Pipe[0] == 0)
	{
		// 19.06.2009 Maks - Она действительно может быть еще не создана
		//Box(_T("Console was not created (CRealConsole::SetConsoleSize)"));
		DEBUGSTRSIZE(L"SetConsoleSize skipped (!mp_RCon->hConWnd || !mp_RCon->ms_ConEmuC_Pipe)\n");

		mp_RCon->LogString("SetConsoleSize skipped (!mp_RCon->hConWnd || !mp_RCon->ms_ConEmuC_Pipe)");

		return FALSE; // консоль пока не создана?
	}

	bool lbRc = false;
	BOOL fSuccess = FALSE;
	//DWORD dwRead = 0;
	DWORD dwTickStart = 0;
	DWORD nCallTimeout = 0;
	DWORD nInSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETSIZE);
	DWORD nOutSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_RETSIZE);
	CESERVER_REQ* pIn = ExecuteNewCmd(anCmdID, nInSize);
	CESERVER_REQ* pOut = nullptr; //ExecuteNewCmd(anCmdID, nOutSize);
	SMALL_RECT rect = {0};
	//bool bLargestReached = false;
	bool bSecondTry = false;

	bool bNeedApplyConsole;

	_ASSERTE(anCmdID==CECMD_SETSIZESYNC || anCmdID==CECMD_SETSIZENOSYNC || anCmdID==CECMD_CMDSTARTED || anCmdID==CECMD_CMDFINISHED);
	//ExecutePrepareCmd(&lIn.hdr, anCmdID, lIn.hdr.cbSize);
	if (!pIn /*|| !pOut*/)
	{
		_ASSERTE(pIn);
		goto wrap;
	}

	// Для режима BufferHeight нужно передать еще и видимый прямоугольник (нужна только верхняя координата?)
	if (con.bBufferHeight)
	{
		// case: buffer mode: change buffer
		CONSOLE_SCREEN_BUFFER_INFO sbi = con.m_sbi;
		sbi.dwSize.X = sizeX; // new
		sizeBuffer = BufferHeight(sizeBuffer); // Если нужно - скорректировать
		_ASSERTE(sizeBuffer > 0);
		sbi.dwSize.Y = sizeBuffer;

		// rect по идее вообще не нужен, за блокировку при прокрутке отвечает nSendTopLine
		rect.Top = sbi.srWindow.Top;
		rect.Left = sbi.srWindow.Left;
		rect.Right = rect.Left + sizeX - 1;
		rect.Bottom = rect.Top + sizeY - 1;

		if (rect.Right >= sbi.dwSize.X)
		{
			int shift = sbi.dwSize.X - 1 - rect.Right;
			rect.Left += shift;
			rect.Right += shift;
		}

		if (rect.Bottom >= sbi.dwSize.Y)
		{
			int shift = sbi.dwSize.Y - 1 - rect.Bottom;
			rect.Top += shift;
			rect.Bottom += shift;
		}

		// В size передаем видимую область
		//sizeY = TextHeight(); -- sizeY уже(!) должен содержать требуемую высоту видимой области!
	}
	else
	{
		_ASSERTE(sizeBuffer == 0);
	}

	_ASSERTE(sizeY>0 && sizeY<200);

	// Устанавливаем параметры для передачи в ConEmuC
	pIn->SetSize.nBufferHeight = sizeBuffer; //con.bBufferHeight ? gpSet->Default BufferHeight : 0;
	pIn->SetSize.size.X = sizeX;
	pIn->SetSize.size.Y = sizeY;

	pIn->SetSize.dwFarPID = (con.bBufferHeight && !mp_RCon->isFarBufferSupported()) ? 0 : mp_RCon->GetFarPID(TRUE);

	// Если заблокирована верхняя видимая строка - выполнять строго
	if (anCmdID != CECMD_CMDFINISHED && con.TopLeft.isLocked())
	{
		// иначе - проверяем текущее соответствие
		//CONSOLE_SCREEN_BUFFER_INFO sbi = mp_ConsoleInfo->sbi;
		bool lbSizeMatch = true;

		if (con.bBufferHeight)
		{
			if (con.m_sbi.dwSize.X != sizeX || con.m_sbi.dwSize.Y != sizeBuffer)
				lbSizeMatch = false;
			else if (con.m_sbi.srWindow.Top != rect.Top || con.m_sbi.srWindow.Bottom != rect.Bottom)
				lbSizeMatch = false;
		}
		else
		{
			if (con.m_sbi.dwSize.X != sizeX || con.m_sbi.dwSize.Y != sizeY)
				lbSizeMatch = false;
		}

		// fin
		if (lbSizeMatch && anCmdID != CECMD_CMDFINISHED)  // -V560
		{
			lbRc = TRUE; // менять ничего не нужно
			goto wrap;
		}

		// При смене размера буфера - сбросить последнее мышиное событие
		ResetLastMousePos();
	}

	char szInfo[128], szSizeCmd[32];

	{
		switch (anCmdID)
		{
		case CECMD_SETSIZESYNC:
			lstrcpynA(szSizeCmd, "CECMD_SETSIZESYNC", countof(szSizeCmd)); break;
		case CECMD_CMDSTARTED:
			lstrcpynA(szSizeCmd, "CECMD_CMDSTARTED", countof(szSizeCmd)); break;
		case CECMD_CMDFINISHED:
			lstrcpynA(szSizeCmd, "CECMD_CMDFINISHED", countof(szSizeCmd)); break;
		case CECMD_SETSIZENOSYNC:
			lstrcpynA(szSizeCmd, "CECMD_SETSIZENOSYNC", countof(szSizeCmd)); break;
		default:
			sprintf_c(szSizeCmd, "SizeCmd=%u", anCmdID);
		}
		sprintf_c(szInfo, "%s(Cols=%i, Rows=%i, Buf=%i, TopLeft={%i,%i})",
		           szSizeCmd, sizeX, sizeY, sizeBuffer, con.TopLeft.y, con.TopLeft.x);
		mp_RCon->LogString(szInfo);
	}

	dwTickStart = timeGetTime();
	// С таймаутом
	nCallTimeout = RELEASEDEBUGTEST(500,30000);

	_ASSERTE(con.bLockChange2Text);
	mp_RCon->mh_ApplyFinished.Reset();

	_ASSERTE(mp_RCon->m_ConsoleMap.IsValid());
	bNeedApplyConsole =
		mp_RCon->m_ConsoleMap.IsValid()
		&& (anCmdID == CECMD_SETSIZESYNC)
		&& (mp_RCon->mn_MonitorThreadID != GetCurrentThreadId());

	_ASSERTE(pOut==nullptr);
	pOut = ExecuteCmd(mp_RCon->ms_ConEmuC_Pipe, pIn, nCallTimeout, ghWnd);
	fSuccess = (pOut != nullptr);

	if (mp_RCon->isServerClosing())
		goto wrap;

	if (fSuccess /*&& (dwRead == (DWORD)nOutSize)*/)
	{
		int nSetWidth = sizeX, nSetHeight = sizeY;
		int nSetWidth2 = -1, nSetHeight2 = -1;
		if (GetConWindowSize(pOut->SetSizeRet.SetSizeRet, &nSetWidth, &nSetHeight, nullptr))
		{
			// If change-size (enlarging) was failed
			if ((sizeX > (UINT)nSetWidth) || (sizeY > (UINT)nSetHeight))
			{
				// Check width
				if ((pOut->SetSizeRet.crMaxSize.X > nSetWidth) && (sizeX > (UINT)nSetWidth))
				{
					bSecondTry = true;
					pIn->SetSize.size.X = pOut->SetSizeRet.crMaxSize.X;
				}
				// And height
				if ((pOut->SetSizeRet.crMaxSize.Y > nSetHeight) && (sizeY > (UINT)nSetHeight))
				{
					bSecondTry = true;
					pIn->SetSize.size.Y = pOut->SetSizeRet.crMaxSize.Y;
				}
				// Try again with lesser size?
				if (bSecondTry)
				{
					CESERVER_REQ* pOut2 = ExecuteCmd(mp_RCon->ms_ConEmuC_Pipe, pIn, nCallTimeout, ghWnd);
					fSuccess = (pOut2 != nullptr);
					if (pOut2)
					{
						ExecuteFreeResult(pOut);
						pOut = pOut2;
						if (GetConWindowSize(pOut->SetSizeRet.SetSizeRet, &nSetWidth2, &nSetHeight2, nullptr))
						{
							nSetWidth = nSetWidth2; nSetHeight = nSetHeight2;
						}
					}
				}
				// Server is closing? Ignore...
				if (mp_RCon->isServerClosing())
				{
					goto wrap;
				}
				else if ((sizeX > (UINT)nSetWidth) || (sizeY > (UINT)nSetHeight))
				{
					_ASSERTE(FALSE && "Maximum real console size was reached");
					// Inform user -- Add exact numbers to log and hint
					wchar_t szInfo[240];
					swprintf_c(szInfo,
						L"Maximum real console size {%i,%i} was reached, lesser size {%i,%i} was applied than requested {%u,%u}",
						pOut ? (int)pOut->SetSizeRet.crMaxSize.X : -1, pOut ? (int)pOut->SetSizeRet.crMaxSize.Y : -1,
						nSetWidth, nSetHeight, (UINT)sizeX, (UINT)sizeY);
					mp_RCon->LogString(szInfo);

					swprintf_c(szInfo,
						L"Maximum real console size {%i,%i} was reached\nDecrease font size in the real console properties",
						nSetWidth, nSetHeight);
					Icon.ShowTrayIcon(szInfo, tsa_Console_Size);
				}
			}
		}
	}

	CSetPgDebug::debugLogCommand(pIn, FALSE, dwTickStart, timeGetTime()-dwTickStart, mp_RCon->ms_ConEmuC_Pipe, pOut);

	if (!fSuccess || (pOut && (pOut->hdr.cbSize < nOutSize)))
	{
		if (gpSet->isLogging())
		{
			DWORD dwErr = GetLastError();
			sprintf_c(szInfo, "SetConsoleSizeSrv.ExecuteCmd FAILED!!! ErrCode=0x%08X, Bytes read=%i", dwErr, pOut ? pOut->hdr.cbSize : 0);
			mp_RCon->LogString(szInfo);
		}

		DEBUGSTRSIZE(L"SetConsoleSize.ExecuteCmd FAILED!!!\n");
	}
	else
	{
		DEBUGSTRSIZE(L"SetConsoleSize.fSuccess == TRUE\n");

		if (pOut->hdr.nCmd != pIn->hdr.nCmd)
		{
			_ASSERTE(pOut->hdr.nCmd == pIn->hdr.nCmd);

			if (gpSet->isLogging())
			{
				char szInfo[128]; sprintf_c(szInfo, "SetConsoleSizeSrv FAILED!!! OutCmd(%i)!=InCmd(%i)", pOut->hdr.nCmd, pIn->hdr.nCmd);
				mp_RCon->LogString(szInfo);
			}
		}
		else
		{
			CONSOLE_SCREEN_BUFFER_INFO sbi = pOut->SetSizeRet.SetSizeRet;
			int nBufHeight;

			if (bNeedApplyConsole)
			{
				// Если Apply еще не прошел - ждем
				DWORD nWait = -1;

				int nNewWidth = 0, nNewHeight = 0;
				DWORD nScroll = 0;
				GetConWindowSize(sbi, &nNewWidth, &nNewHeight, &nScroll);

				// Сравниваем с GetTextWidth/GetTextHeight т.к. именно они отдают
				// размеры консоли отображаемой в VCon (размеры буферов VCon)
				// ((nNewWidth != (int)(UINT)sizeX) || (nNewHeight != (int)(UINT)sizeY))
				if ((GetTextWidth() != nNewWidth) || (GetTextHeight() != nNewHeight))
				{
					mp_RCon->mn_LastConsolePacketIdx--;
					mp_RCon->SetMonitorThreadEvent();
					DWORD nWaitTimeout = SETSYNCSIZEAPPLYTIMEOUT;

					#ifdef _DEBUG
					DWORD nWaitStart = GetTickCount();
					nWaitTimeout = SETSYNCSIZEAPPLYTIMEOUT*4; //30000;
					nWait = mp_RCon->mh_ApplyFinished.Wait(nWaitTimeout);
					if (nWait == WAIT_TIMEOUT)
					{
						_ASSERTE(FALSE && "SETSYNCSIZEAPPLYTIMEOUT");
						//nWait = WaitForSingleObject(mp_RCon->mh_ApplyFinished, nWaitTimeout);
					}
					DWORD nWaitDelay = GetTickCount() - nWaitStart;
					#else
					nWait = mp_RCon->mh_ApplyFinished.Wait(nWaitTimeout);
					#endif

					COORD crDebugCurSize = con.m_sbi.dwSize;

					if (crDebugCurSize.X != sizeX)
					{
						if (gpSet->isLogging())
						{
							char szInfo[128]; sprintf_c(szInfo, "SetConsoleSize FAILED!!! ReqSize={%ix%i}, OutSize={%ix%i}", sizeX, (sizeBuffer ? sizeBuffer : sizeY), crDebugCurSize.X, crDebugCurSize.Y);
							mp_RCon->LogString(szInfo);
						}

						#ifdef _DEBUG
						_ASSERTE(crDebugCurSize.X == sizeX);
						#endif
					}
				}

				if (gpSet->isLogging())
				{
					mp_RCon->LogString(
					    (nWait == (DWORD)-1) ?
					    "SetConsoleSizeSrv: does not need wait" :
					    (nWait != WAIT_OBJECT_0) ?
					    "SetConsoleSizeSrv.WaitForSingleObject(mp_RCon->mh_ApplyFinished) TIMEOUT!!!" :
					    "SetConsoleSizeSrv.WaitForSingleObject(mp_RCon->mh_ApplyFinished) succeeded");
				}

				// If sync resize was abandoned (timeout)
				// let use current values
				sbi = con.m_sbi;
				nBufHeight = con.nBufferHeight;
			}
			else
			{
				nBufHeight = pIn->SetSize.nBufferHeight;

				if (gpSet->isLogging())
					mp_RCon->LogString("SetConsoleSizeSrv.Not waiting for ApplyFinished");
			}

			WARNING("!!! Здесь часто возникают _ASSERTE'ы. Видимо высота буфера меняется в другой нити и con.bBufferHeight просто не успевает?");

			if (con.bBufferHeight)
			{
				_ASSERTE((sbi.srWindow.Bottom-sbi.srWindow.Top)<200);

				if (gpSet->isLogging())
				{
					char szInfo[128]; sprintf_c(szInfo, "Current size: {%i,%i} Buf={%i,%i}", sbi.srWindow.Right-sbi.srWindow.Left+1, sbi.srWindow.Bottom-sbi.srWindow.Top+1, sbi.dwSize.X, sbi.dwSize.Y);
					mp_RCon->LogString(szInfo);
				}

				if (sbi.dwSize.X == sizeX && sbi.dwSize.Y == nBufHeight)
				{
					lbRc = TRUE;
				}
				else
				{
					if (gpSet->isLogging())
					{
						char szInfo[128]; sprintf_c(szInfo, "SetConsoleSizeSrv FAILED! Ask={%ix%i}, Cur={%ix%i}, Ret={%ix%i}",
						                             sizeX, sizeY,
						                             sbi.dwSize.X, sbi.dwSize.Y,
						                             pOut->SetSizeRet.SetSizeRet.dwSize.X, pOut->SetSizeRet.SetSizeRet.dwSize.Y
						                            );
						mp_RCon->LogString(szInfo);
					}

					lbRc = FALSE;
				}
			}
			else
			{
				if (sbi.dwSize.Y > 200)
				{
					_ASSERTE(sbi.dwSize.Y<200);
				}

				if (gpSet->isLogging())
				{
					char szInfo[128]; sprintf_c(szInfo, "Current size: Cols=%i, Rows=%i", sbi.dwSize.X, sbi.dwSize.Y);
					mp_RCon->LogString(szInfo);
				}

				if (sbi.dwSize.X == sizeX && sbi.dwSize.Y == sizeY)
				{
					lbRc = TRUE;
				}
				else
				{
					if (gpSet->isLogging())
					{
						char szInfo[128]; sprintf_c(szInfo, "SetConsoleSizeSrv FAILED! Ask={%ix%i}, Cur={%ix%i}, Ret={%ix%i}",
						                             sizeX, sizeY,
						                             sbi.dwSize.X, sbi.dwSize.Y,
						                             pOut->SetSizeRet.SetSizeRet.dwSize.X, pOut->SetSizeRet.SetSizeRet.dwSize.Y
						                            );
						mp_RCon->LogString(szInfo);
					}

					lbRc = FALSE;
				}
			}

			// If server was changed the size
			if (lbRc)
			{
				DEBUGSTRSIZE(L"SetConsoleSizeSrv.lbRc == TRUE\n");
				int nNewWidth = 0, nNewHeight = 0;
				DWORD nScroll = 0;
				if (GetConWindowSize(con.m_sbi, &nNewWidth, &nNewHeight, &nScroll))
				{
					// Let store new values to be sure our GUI is using them
					if ((GetTextWidth() != nNewWidth) || (GetTextHeight() != nNewHeight))
					{
						SetChange2Size(nNewWidth, nNewHeight);
					}
				}

				#ifdef _DEBUG
				if (con.nChange2TextHeight > 200)
				{
					_ASSERTE(con.nChange2TextHeight<=200);
				}
				if (con.bBufferHeight)
				{
					_ASSERTE(con.nBufferHeight == sbi.dwSize.Y);
				}
				#endif

				con.nBufferHeight = con.bBufferHeight ? sbi.dwSize.Y : 0;
			}
		}
	}

wrap:
	ExecuteFreeResult(pIn);
	ExecuteFreeResult(pOut);
	return lbRc;
}

bool CRealBuffer::isInResize()
{
	return (this && con.bLockChange2Text);
}

bool CRealBuffer::SetConsoleSize(SHORT sizeX, SHORT sizeY, USHORT sizeBuffer, DWORD anCmdID /*= CECMD_SETSIZESYNC*/)
{
	if (!this) return false;

	if ((sizeX <= 0) || (sizeY <= 0))
	{
		_ASSERTE((sizeX > 0) && (sizeY > 0));
		return FALSE;
	}

	// Если была блокировка DC - сбросим ее
	mp_RCon->mp_VCon->LockDcRect(FALSE, nullptr);

	if (m_Type != rbt_Primary)
	{
		// Все альтернативные буферы "меняются" виртуально
		con.nTextWidth = sizeX;
		con.nTextHeight = sizeY;
		con.nBufferHeight = sizeBuffer;
		con.bBufferHeight = (sizeBuffer != 0);
		ChangeScreenBufferSize(con.m_sbi, sizeX, sizeY, sizeX, sizeBuffer ? sizeBuffer : sizeY);

		if (mp_RCon->isActive(false))
		{
			gpConEmu->mp_Status->OnConsoleChanged(&con.m_sbi, &con.m_ci, nullptr, true);
		}

		return TRUE;
	}

	//_ASSERTE(mp_RCon->hConWnd && mp_RCon->ms_ConEmuC_Pipe[0]);

	if (!mp_RCon->hConWnd || mp_RCon->ms_ConEmuC_Pipe[0] == 0)
	{
		// 19.06.2009 Maks - Она действительно может быть еще не создана
		//Box(_T("Console was not created (CRealConsole::SetConsoleSize)"));
		if (gpSet->isLogging()) mp_RCon->LogString("SetConsoleSize skipped (!mp_RCon->hConWnd || !mp_RCon->ms_ConEmuC_Pipe)");

		// Это должно быть только на этапе создания новой консоли (например, появилась панель табов)
		if (con.nTextWidth != sizeX || con.nTextHeight != sizeY)
		{
			_ASSERTE(mp_RCon->hConWnd==nullptr || mp_RCon->mb_InCloseConsole);
			con.nTextWidth = sizeX;
			con.nTextHeight = sizeY;
			InitBuffers(sizeX*sizeY, sizeX, sizeY);
			mp_RCon->VCon()->OnConsoleSizeReset(sizeX, sizeY);
		}

		DEBUGSTRSIZE(L"SetConsoleSize skipped (!mp_RCon->hConWnd || !mp_RCon->ms_ConEmuC_Pipe)\n");
		return false; // консоль пока не создана?
	}

	HEAPVAL
	_ASSERTE(sizeX>=MIN_CON_WIDTH && sizeY>=MIN_CON_HEIGHT);

	if (sizeX</*4*/MIN_CON_WIDTH)
		sizeX = /*4*/MIN_CON_WIDTH;

	if (sizeY</*2*/MIN_CON_HEIGHT)
		sizeY = /*2*/MIN_CON_HEIGHT;

	_ASSERTE(con.bBufferHeight || (!con.bBufferHeight && !sizeBuffer));  // -V728

	COORD crFixed = {};
	if (mp_RCon->isFixAndCenter(&crFixed))
	{
		if ((crFixed.X != sizeX) || (crFixed.Y != sizeY))
		{
			_ASSERTE(FALSE && "NTVDM may fail if size will be changed");
		}
	}

	if (con.bBufferHeight && !sizeBuffer)
		sizeBuffer = BufferHeight(sizeBuffer);

	_ASSERTE(!con.bBufferHeight || (sizeBuffer >= sizeY));
	bool lbRc = false;
	#ifdef _DEBUG
	DWORD dwPID = mp_RCon->GetFarPID(TRUE);
	#endif
	//if (dwPID)
	//{
	//	// Если это СТАРЫЙ FAR (нет Synchro) - может быть не ресайзить через плагин?
	//	// Хотя тут плюс в том, что хоть активация и идет чуть медленнее, но
	//	// возврат из ресайза получается строго после обновления консоли
	//	if (!mb_PluginDetected || dwPID != mn_FarPID_PluginDetected)
	//		dwPID = 0;
	//}
	_ASSERTE(sizeY <= 300);
	/*if (!con.bBufferHeight && dwPID)
		lbRc = SetConsoleSizePlugin(sizeX, sizeY, sizeBuffer, anCmdID);
	else*/
	HEAPVAL;

	wchar_t szStatus[64];
	int iBufWidth = GetBufferWidth();
	if (iBufWidth < (int)sizeX) iBufWidth = sizeX;
	int iBufHeight = (sizeBuffer > 0) ? sizeBuffer : GetBufferHeight();
	swprintf_c(szStatus, L"{%u,%u} size, {%i,%i} buffer", sizeX, sizeY, iBufWidth, iBufHeight);
	mp_RCon->SetConStatus(szStatus, CRealConsole::cso_Critical);

	// Попробовать для консолей (cmd, и т.п.) делать ресайз после отпускания мышки
	if ((gpConEmu->mouse.state & MOUSE_SIZING_BEGIN)
		&& (!mp_RCon->GuiWnd() && !mp_RCon->GetFarPID()))
	{
		if (anCmdID==CECMD_CMDSTARTED || anCmdID==CECMD_CMDFINISHED)
		{
			_ASSERTE(FALSE && "MOUSE_SIZING_BEGIN was not cleared!");
		}
		else
		{
			if (gpSet->isLogging()) mp_RCon->LogString("SetConsoleSize skipped until LMouseButton not released");
			DEBUGSTRSIZE2(L"!!! SetConsoleSize skipped until LMouseButton not released !!!\n");
			goto wrap;
		}
	}

	// Сразу поставим, чтобы в основной нити при синхронизации размер не слетел
	// Необходимо заблокировать RefreshThread, чтобы не вызывался ApplyConsoleInfo ДО ЗАВЕРШЕНИЯ SetConsoleSize
	{
	MSetter lSet((bool*)&con.bLockChange2Text);
	SetChange2Size(sizeX, sizeY);
	// Call the server
	lbRc = SetConsoleSizeSrv(sizeX, sizeY, sizeBuffer, anCmdID);
	// Done
	lSet.Unlock();
	HEAPVAL;
	}

	mp_RCon->SetConStatus(nullptr);

	goto wrap;
#if 0
	if (lbRc && mp_RCon->isActive(false))
	{
		if (!mp_RCon->isNtvdm())
		{
			// update size info
			if (gpConEmu->isWindowNormal())
			{
				int nHeight = TextHeight();
				gpSetCls->UpdateSize(sizeX, nHeight);
			}
		}

		// -- должно срабатывать при ApplyData
		//gpConEmu->UpdateStatusBar();
	}
#endif
wrap:
	HEAPVAL;
	DEBUGSTRSIZE(L"SetConsoleSize.finalizing\n");
	return lbRc;
}

void CRealBuffer::SyncConsole2Window(SHORT wndSizeX, SHORT wndSizeY)
{
	if ((wndSizeX <= 0) || (wndSizeY <= 0))
	{
		_ASSERTE((wndSizeX > 0) && (wndSizeY > 0));
	}

	// Во избежание лишних движений да и зацикливания...
	if (con.nTextWidth != wndSizeX || con.nTextHeight != wndSizeY)
	{
		if (IFLOGCONSOLECHANGE)
		{
			char szInfo[128]; sprintf_c(szInfo, "CRealBuffer::SyncConsole2Window(Cols=%i, Rows=%i, Current={%i,%i})", wndSizeX, wndSizeY, con.nTextWidth, con.nTextHeight);
			mp_RCon->LogString(szInfo);
		}

		#ifdef _DEBUG
		if (wndSizeX == 80)
		{
			int nDbg = wndSizeX;
		}
		#endif


		// Do resize
		SetConsoleSize(wndSizeX, wndSizeY, 0/*Auto*/);


		if (mp_RCon->isActive(false) && isMainThread())
		{
			// Сразу обновить DC чтобы скорректировать Width & Height
			mp_RCon->mp_VCon->OnConsoleSizeChanged();
		}
	}
}

bool CRealBuffer::isScroll(RealBufferScroll aiScroll /*= rbs_Any*/)
{
	TODO("Horizontal scroll");
	return con.bBufferHeight;
}

// Called during attach (CRealConsole::AttachConemuC)
void CRealBuffer::InitSBI(const CONSOLE_SCREEN_BUFFER_INFO& sbi)
{
	ResetConData();

	bool bCurBufHeight = IsBufferHeightTurnedOn(sbi);

	// #SIZE_TODO Check with split panes?
	RECT rcWnd = mp_RCon->mp_ConEmu->CalcRect(CER_DC, mp_RCon->mp_VCon);
	RECT rcCon = mp_RCon->mp_ConEmu->CalcRect(CER_CONSOLE_CUR, rcWnd, CER_MAINCLIENT, mp_RCon->mp_VCon);

	// Correct heights and dimensions, they would be applied by attaching server
	CONSOLE_SCREEN_BUFFER_INFO lsbi = sbi;
	lsbi.dwSize.X = MakeShort(rcCon.right);
	lsbi.srWindow.Left = 0;
	lsbi.srWindow.Right = MakeShort(rcCon.right-1);

	if (bCurBufHeight)
	{
		// Don't change lsbi.dwSize.Y
		lsbi.srWindow.Bottom = MakeShort(lsbi.srWindow.Top + rcCon.bottom - 1);
	}
	else
	{
		lsbi.dwSize.Y = MakeShort(rcCon.bottom);
		lsbi.srWindow.Top = 0;
		lsbi.srWindow.Bottom = MakeShort(rcCon.bottom - 1);
	}

	con.m_sbi = lsbi;

	TODO("Horizontal scroll");
	con.bBufferHeight = bCurBufHeight;

	// Check real console scrollers - if they were changed
	_ASSERTEX(!bufferModeChangeLocked_);
	CheckBufferSize();

	_ASSERTE(mp_RCon->isBufferHeight() == bCurBufHeight);
}

void CRealBuffer::InitMaxSize(const COORD& crMaxSize)
{
	con.crMaxSize = crMaxSize;
}

COORD CRealBuffer::GetMaxSize()
{
	return con.crMaxSize;
}

bool CRealBuffer::isInitialized()
{
	// Was console intialized properly?
	if (!m_ConData.Ptr() || !con.nTextWidth || con.nTextHeight<2)
		return false;
	return true;
}

bool CRealBuffer::isFarMenuOrMacro()
{
	bool lbMenuActive = false;
	MSectionLock cs; cs.Lock(&csCON);

	WARNING("CantActivateInfo: Хорошо бы при отображении хинта 'Can't activate tab' сказать 'почему'");

	CRConDataGuard data;
	if (GetData(data))
	{
		TODO("Хорошо бы реально у фара узнать, выполняет ли он макрос");
		if (((data->pConChar[0] == L'R') && ((data->pConAttr[0] & 0xFF) == 0x4F))
			|| ((data->pConChar[0] == L'P') && ((data->pConAttr[0] & 0xFF) == 0x2F)))
		{
			// Запись макроса. Запретим наверное переключаться?
			lbMenuActive = true;
		}
		else if (data->pConChar[0] == L' ' && data->pConChar[con.nTextWidth] == ucBoxDblVert)
		{
			lbMenuActive = true;
		}
		else if (data->pConChar[0] == L' ' && (data->pConChar[con.nTextWidth] == ucBoxDblDownRight ||
		                                    (data->pConChar[con.nTextWidth] == '['
		                                     && (data->pConChar[con.nTextWidth+1] >= L'0' && data->pConChar[con.nTextWidth+1] <= L'9'))))
		{
			// Строка меню ВСЕГДА видна. Определим, активно ли меню.
			for(int x=1; !lbMenuActive && x<con.nTextWidth; x++)
			{
				if (data->pConAttr[x] != data->pConAttr[0])  // неактивное меню - не подсвечивается
					lbMenuActive = true;
			}
		}
		else
		{
			// Если строка меню ВСЕГДА не видна, а только всплывает
			wchar_t* pszLine = data->pConChar + con.nTextWidth;

			for(int x=1; !lbMenuActive && x<(con.nTextWidth-10); x++)
			{
				if (pszLine[x] == ucBoxDblDownRight && pszLine[x+1] == ucBoxDblHorz)
					lbMenuActive = true;
			}
		}
	}

	cs.Unlock();

	return lbMenuActive;
}

bool CRealBuffer::isConsoleDataChanged()
{
	if (!this) return false;
	return con.bConsoleDataChanged;
}

bool CRealBuffer::PreInit()
{
	HEAPVAL;
	// Инициализировать переменные m_sbi, m_ci, m_sel
	//RECT rcWnd; Get ClientRect(ghWnd, &rcWnd);

	// mp_RCon->isBufferHeight использовать нельзя, т.к. mp_RBuf.m_sbi еще не инициализирован!
	bool bNeedBufHeight = (gpSetCls->bForceBufferHeight && gpSetCls->nForceBufferHeight>0)
	                      || (!gpSetCls->bForceBufferHeight && mp_RCon->mn_DefaultBufferHeight);

	if (bNeedBufHeight && !isScroll())
	{
		HEAPVAL;
		SetBufferHeightMode(TRUE);
		HEAPVAL;
		_ASSERTE(mp_RCon->mn_DefaultBufferHeight>0);
		BufferHeight(mp_RCon->mn_DefaultBufferHeight);
	}

	HEAPVAL;
	RECT rcCon;

	// Even if our window was minimized, CalcRect will use proper sizes (mrc_StoredNormalRect for example)
	rcCon = gpConEmu->CalcRect(CER_CONSOLE_CUR, mp_RCon->mp_VCon);

	if (IsRectEmpty(&rcCon))
	{
		_ASSERTE(FALSE && "CER_CONSOLE_CUR is empty");
		LogString(L"Evaluated CER_CONSOLE_CUR is empty!");
		return false;
	}

	if (con.bBufferHeight)
	{
		_ASSERTE(mp_RCon->mn_DefaultBufferHeight>0);
		con.m_sbi.dwSize = MakeCoord(rcCon.right,mp_RCon->mn_DefaultBufferHeight);
	}
	else
	{
		con.m_sbi.dwSize = MakeCoord(rcCon.right,rcCon.bottom);
	}
	con.nTextWidth = rcCon.right;
	con.nTextHeight = rcCon.bottom;

	SetTopLeft();

	con.m_sbi.wAttributes = 7;
	con.m_sbi.srWindow = MakeSmallRect(0, 0, rcCon.right-1, rcCon.bottom-1);
	con.m_sbi.dwMaximumWindowSize = con.m_sbi.dwSize;
	con.m_ci.dwSize = 15; con.m_ci.bVisible = TRUE;

	if (!InitBuffers())
		return false;

	return true;
}

bool CRealBuffer::GetData(CRConDataGuard& data)
{
	// m_ConData is not expected to be used in other buffer types
	_ASSERTE(m_Type == rbt_Primary || m_Type == rbt_DumpScreen);

	MSectionLockSimple cs; cs.Lock(&mcs_Data);
	if (m_ConData.isValid())
	{
		data.Attach(m_ConData.Ptr());
	}
	else
	{
		data.Release();
	}
	cs.Unlock();

	return data.isValid();
}

/// Only for the moment, informational
bool CRealBuffer::isDataValid()
{
	bool bValid = false;

	if ((m_Type == rbt_DumpScreen) || (m_Type == rbt_Alternative) || (m_Type == rbt_Selection) || (m_Type == rbt_Find))
	{
		bValid = ((dump.crSize.X > 0) && (dump.crSize.Y > 0) && dump.pszBlock1 && dump.pcaBlock1);
	}
	else
	{
		CRConDataGuard data;
		bValid = GetData(data) && (data->nWidth && data->nHeight);
	}

	return bValid;
}

bool CRealBuffer::InitBuffers(CRConDataGuard* pData)
{
	return InitBuffers(0/*anCellCount*/, 0/*anWidth*/, 0/*anHeight*/, pData);
}

bool CRealBuffer::InitBuffers(DWORD anCellCount /*= 0*/, int anWidth /*= 0*/, int anHeight /*= 0*/, CRConDataGuard* pData /*= nullptr*/)
{
	bool lbRc = false;
	int nNewWidth = 0, nNewHeight = 0;
	size_t nCellCount = 0;
	BYTE nDefTextAttr;
	CRConDataGuard data;

	// Эта функция должна вызываться только в MonitorThread.
	// Тогда блокировка буфера не потребуется
	#ifdef _DEBUG
	DWORD dwCurThId = GetCurrentThreadId();
	_ASSERTE((mp_RCon->mn_MonitorThreadID==0 || dwCurThId==mp_RCon->mn_MonitorThreadID
				|| (mp_RCon->m_StartState <= rss_StartingServer) || mp_RCon->hConWnd==nullptr)
		|| ((m_Type==rbt_DumpScreen || m_Type==rbt_Alternative || m_Type==rbt_Selection || m_Type==rbt_Find)
				&& isMainThread()));
	#endif

	HEAPVAL;

	if (anWidth > 0 && anHeight > 0)
	{
		nNewWidth = anWidth; nNewHeight = anHeight;
	}
	else
	{
		if (!GetConWindowSize(con.m_sbi, &nNewWidth, &nNewHeight, nullptr))
		{
			if (pData)
				pData->Release();
			return FALSE;
		}
	}

	// Функция вызывается с (anCellCount!=0) ТОЛЬКО из ApplyConsoleInfo()
	if (anCellCount)
	{
		nCellCount = anCellCount;

		if (anCellCount < (DWORD)(nNewWidth * nNewHeight))
		{
			_ASSERTE(anCellCount >= (DWORD)(nNewWidth * nNewHeight));
			nCellCount = (nNewWidth * nNewHeight);
		}
		else if ((con.nCreatedBufWidth == nNewWidth && con.nCreatedBufHeight == nNewHeight)
			&& (con.nTextWidth == nNewWidth && con.nTextHeight == nNewHeight))
		{
			// Sizes were not changed
			if (GetData(data)
				&& data.isValid(nNewWidth * nNewHeight))
			{
				if (pData)
					pData->Attach(data.Ptr());
				lbRc = TRUE;
				goto wrap;
			}
		}
	}
	else
	{
		nCellCount = nNewWidth * nNewHeight;
	}

	// Evaluate default back/text color (indexes)
	nDefTextAttr = MAKECONCOLOR(mp_RCon->GetDefaultTextColorIdx(), mp_RCon->GetDefaultBackColorIdx());

	// Если требуется увеличить или создать (первично) буфера
	// Increase buffer sizes if required
	if (!GetData(data) || !data.isValid() || (data->nMaxCells < nCellCount))
	{
		// Exclusive(!) Lock
		MSectionLockSimple sc; sc.Lock(&mcs_Data);
		data.Release();

		wchar_t szLog[80];
		swprintf_c(szLog, L"InitBuffers Width=%u Height=%u Cells=%u (begin)", nNewWidth, nNewHeight, nCellCount);
		mp_RCon->LogString(szLog);


		HEAPVAL;
		con.LastStartInitBuffersTick = GetTickCount();

		// Is not critical anymore, it's locked just for flickering minimization
		_ASSERTE(con.nInGetConsoleData==0);

		// Сначала - сброс
		con.nConBufCells = 0;


		if (CRConData::Allocate(data, mp_RCon, nCellCount))
		{
			HEAPVAL;

			con.nConBufCells = data->nMaxCells;

			HEAPVAL;
			wmemset((wchar_t*)data->pConAttr, nDefTextAttr, data->nMaxCells);

			con.LastEndInitBuffersTick = GetTickCount();

			HEAPVAL;
			m_ConData.Attach(data.Ptr());

			HEAPVAL;
			lbRc = TRUE;
		}
		else
		{
			con.LastEndInitBuffersTick = (DWORD)-1;
		}

		sc.Unlock();

		_ASSERTE(data.isValid(nCellCount) && "Failed to allocate data");

		// Is not critical anymore, it's locked just for flickering minimization
		_ASSERTE(con.nInGetConsoleData==0);

		HEAPVAL

		swprintf_c(szLog, L"InitBuffers Width=%u Height=%u Cells=%u (done)", nNewWidth, nNewHeight, nCellCount);
		mp_RCon->LogString(szLog);
	}
	else if (con.nTextWidth!=nNewWidth || con.nTextHeight!=nNewHeight)
	{
		HEAPVAL
		MSectionLock sc; sc.Lock(&csCON);

		size_t nFillCount = nCellCount ? std::min(nCellCount,con.nConBufCells) : con.nConBufCells;

		if (GetData(data) && data.isValid(nFillCount))
		{
			memset(data->pConChar, 0, nFillCount * sizeof(*data->pConChar));

			wmemset((wchar_t*)data->pConAttr, nDefTextAttr, nFillCount);

			memset(data->pDataCmp, 0, nFillCount * sizeof(*data->pDataCmp));
		}

		sc.Unlock();
		HEAPVAL
		lbRc = TRUE;
	}
	else
	{
		lbRc = (GetData(data) && data.isValid(nCellCount));
	}

wrap:
	HEAPVAL;

	if (lbRc && ((nNewWidth * (size_t)nNewHeight) <= con.nConBufCells))
	{
		if ((con.nCreatedBufWidth != nNewWidth) || (con.nCreatedBufHeight != nNewHeight))
		{
			con.nCreatedBufWidth = nNewWidth;
			con.nCreatedBufHeight = nNewHeight;
		}
	}

	con.nTextWidth = nNewWidth;
	con.nTextHeight = nNewHeight;
	// чтобы передернулись положения панелей и прочие флаги
	if (this == mp_RCon->mp_ABuf)
		mp_RCon->mb_DataChanged = TRUE;

	if (pData)
	{
		if (lbRc)
		{
			_ASSERTE(data.isValid());
			pData->Attach(data.Ptr());
		}
		else
		{
			pData->Release();
		}
	}

	return lbRc;
}

void CRealBuffer::PreFillBuffers()
{
	CRConDataGuard data;
	if (GetData(data))
	{
		WORD nDefTextAttr = MAKECONCOLOR(mp_RCon->GetDefaultTextColorIdx(), mp_RCon->GetDefaultBackColorIdx());
		wmemset((wchar_t*)data->pConAttr, nDefTextAttr, data->nMaxCells);
	}
}

SHORT CRealBuffer::GetBufferWidth() const
{
	return con.m_sbi.dwSize.X;
}

SHORT CRealBuffer::GetBufferHeight() const
{
	return con.m_sbi.dwSize.Y;
}

int CRealBuffer::GetDynamicHeight() const
{
	if (gpSet->isDynamicBufferHeight && con.nDynamicHeight > 0)
		return con.nDynamicHeight;
	return GetBufferHeight();
}

SHORT CRealBuffer::GetBufferPosX() const
{
	_ASSERTE(con.m_sbi.srWindow.Left==0);
	return con.m_sbi.srWindow.Left;
}

SHORT CRealBuffer::GetBufferPosY() const
{
	//-- по факту - не интересно, ассерт часто возникал в процессе обильного скролла-вывода текста
	//-- (con.nTopVisibleLine и csbi.srWindow.Top оказывались рассинхронизированными)
	//#if defined(_DEBUG)
	#if 0
	USHORT nTop = con.nTopVisibleLine;
	CONSOLE_SCREEN_BUFFER_INFO csbi = con.m_sbi;
	bool bInScroll = mp_RCon->InScroll();
	if (nTop != csbi.srWindow.Top)
	{
		TODO("Пока не переделал скролл на пайп - данные могут приходить немного с запаздыванием");
		_ASSERTE(nTop == csbi.srWindow.Top || bInScroll);
		bool bDbgShowConsole = false;
		if (bDbgShowConsole)
		{
			mp_RCon->ShowConsole(1);
			mp_RCon->ShowConsole(0);
		}
	}
	#endif

	// ConPTY grows its real buffer as history arrives. Use the viewport belonging
	// to the received cells, not an older requested TopLeft from before that growth.
	if (m_Type == rbt_Primary && mp_RCon->IsConPty())
		return con.m_sbi.srWindow.Top;
	return (con.TopLeft.y >= 0) ? con.TopLeft.y : con.m_sbi.srWindow.Top;
}

int CRealBuffer::TextWidth() const
{
	AssertThisRet(MIN_CON_WIDTH);

	if (con.nChange2TextWidth > 0)
		return con.nChange2TextWidth;

	_ASSERTE(con.nTextWidth>=MIN_CON_WIDTH);
	return con.nTextWidth;
}

int CRealBuffer::GetTextWidth() const
{
	AssertThisRet(MIN_CON_WIDTH);

	_ASSERTE(con.nTextWidth>=MIN_CON_WIDTH && con.nTextWidth<=400);
	return con.nTextWidth;
}

int CRealBuffer::TextHeight() const
{
	AssertThisRet(MIN_CON_HEIGHT);

	int nRet = 0;

	if (con.nChange2TextHeight > 0)
		nRet = con.nChange2TextHeight;
	else
		nRet = con.nTextHeight;

	#ifdef _DEBUG
	if (nRet <= MIN_CON_HEIGHT || nRet > 200)
	{
		_ASSERTE(nRet>=MIN_CON_HEIGHT && nRet<=200);
	}
	#endif

	return nRet;
}

int CRealBuffer::GetTextHeight() const
{
	AssertThisRet(MIN_CON_HEIGHT);

	_ASSERTE(con.nTextHeight>=MIN_CON_HEIGHT && con.nTextHeight<=200);
	return con.nTextHeight;
}

int CRealBuffer::GetWindowWidth() const
{
	int nWidth = con.m_sbi.srWindow.Right - con.m_sbi.srWindow.Left + 1;
	_ASSERTE(nWidth>=MIN_CON_WIDTH && nWidth <= 400);
	return nWidth;
}

int CRealBuffer::GetWindowHeight() const
{
	int nHeight = con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1;
	_ASSERTE(nHeight>=MIN_CON_HEIGHT && nHeight <= 300);
	return nHeight;
}

// #SIZE_TODO CRealBuffer::BufferHeight needs refactoring
int CRealBuffer::BufferHeight(unsigned nNewBufferHeight /*= 0*/)
{
	int nBufferHeight = 0;

	if (con.bBufferHeight)
	{
		if (nNewBufferHeight)
		{
			nBufferHeight = nNewBufferHeight;
			con.nBufferHeight = nNewBufferHeight;
		}
		else if (con.nBufferHeight)
		{
			nBufferHeight = con.nBufferHeight;
		}
		else if (mp_RCon->mn_DefaultBufferHeight)
		{
			nBufferHeight = mp_RCon->mn_DefaultBufferHeight;
		}
		else
		{
			nBufferHeight = gpSet->DefaultBufferHeight;
		}
	}
	else
	{
		// После выхода из буферного режима сбрасываем запомненную высоту, чтобы
		// в следующий раз установить высоту из настроек (gpSet->DefaultBufferHeight)
		_ASSERTE(nNewBufferHeight == 0);
		con.nBufferHeight = 0;
	}

	return nBufferHeight;
}

bool CRealBuffer::GetConWindowSize(const CONSOLE_SCREEN_BUFFER_INFO& sbi, int* pnNewWidth, int* pnNewHeight, DWORD* pnScroll) const
{
	TODO("Заменить на вызов ::GetConWindowSize из WObjects.cpp");
	DWORD nScroll = 0; // enum RealBufferScroll
	int nNewWidth = 0, nNewHeight = 0;

	// Функция возвращает размер ОКНА (видимой области), то есть буфер может быть больше

	int nCurWidth = TextWidth();
	if (sbi.dwSize.X == nCurWidth)
	{
		nNewWidth = sbi.dwSize.X;
	}
	else
	{
		TODO("Добавить в con флажок горизонтальной прокрутки");
		TODO("и вообще, заменить на вызов ::GetConWindowSize из WObjects.cpp");
		if (sbi.dwSize.X <= EvalBufferTurnOnSize(std::max<int>(nCurWidth, con.crMaxSize.X)))
		{
			nNewWidth = sbi.dwSize.X;
		}
		else
		{
			// 111125 было "nNewWidth  = sbi.dwSize.X;", но так игнорируется горизонтальная прокрутка
			nNewWidth = sbi.srWindow.Right - sbi.srWindow.Left + 1;
			_ASSERTE(nNewWidth <= sbi.dwSize.X);
		}
	}
	// Флаги
	if (/*(sbi.dwSize.X > sbi.dwMaximumWindowSize.X) ||*/ (nNewWidth < sbi.dwSize.X))
	{
		// для проверки условий
		//_ASSERTE((sbi.dwSize.X > sbi.dwMaximumWindowSize.X) && (nNewWidth < sbi.dwSize.X));
		nScroll |= rbs_Horz;
	}


	int nCurHeight = TextHeight();
	if (sbi.dwSize.Y == nCurHeight)
	{
		nNewHeight = sbi.dwSize.Y;
	}
	else
	{
		if ((con.bBufferHeight && (sbi.dwSize.Y > nCurHeight))
			|| (sbi.dwSize.Y > EvalBufferTurnOnSize(nCurHeight)))
		{
			nNewHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
		}
		else
		{
			nNewHeight = sbi.dwSize.Y;
		}
	}
	// Флаги
	if (/*(sbi.dwSize.Y > sbi.dwMaximumWindowSize.Y) ||*/ (nNewHeight < sbi.dwSize.Y))
	{
		// для проверки условий
		//_ASSERTE((sbi.dwSize.Y >= sbi.dwMaximumWindowSize.Y) && (nNewHeight < sbi.dwSize.Y));
		nScroll |= rbs_Vert;
	}

	// Validation
	if ((nNewWidth <= 0) || (nNewHeight <= 0))
	{
		Assert(nNewWidth>0 && nNewHeight>0);
		return FALSE;
	}

	// Result
	if (pnNewWidth)
		*pnNewWidth = nNewWidth;
	if (pnNewHeight)
		*pnNewHeight = nNewHeight;
	if (pnScroll)
		*pnScroll = nScroll;

	return TRUE;

	//BOOL lbBufferHeight = this->isScroll();

	//// Проверка режимов прокрутки
	//if (!lbBufferHeight)
	//{
	//	if (sbi.dwSize.Y > sbi.dwMaximumWindowSize.Y)
	//	{
	//		lbBufferHeight = TRUE; // однозначное включение прокрутки
	//	}
	//}

	//if (lbBufferHeight)
	//{
	//	if (sbi.srWindow.Top == 0
	//	        && sbi.dwSize.Y == (sbi.srWindow.Bottom + 1)
	//	  )
	//	{
	//		lbBufferHeight = FALSE; // однозначное вЫключение прокрутки
	//	}
	//}

	//// Теперь собственно размеры
	//if (!lbBufferHeight)
	//{
	//	nNewHeight =  sbi.dwSize.Y;
	//}
	//else
	//{
	//	// Это может прийти во время смены размера
	//	if ((sbi.srWindow.Bottom - sbi.srWindow.Top + 1) < MIN_CON_HEIGHT)
	//		nNewHeight = con.nTextHeight;
	//	else
	//		nNewHeight = sbi.srWindow.Bottom - sbi.srWindow.Top + 1;
	//}

	//WARNING("Здесь нужно выполнить коррекцию, если nNewHeight велико - включить режим BufferHeight");

	//if (pbBufferHeight)
	//	*pbBufferHeight = lbBufferHeight;

	//_ASSERTE(nNewWidth>=MIN_CON_WIDTH && nNewHeight>=MIN_CON_HEIGHT);

	//if (!nNewWidth || !nNewHeight)
	//{
	//	Assert(nNewWidth && nNewHeight);
	//	return FALSE;
	//}

	////if (nNewWidth < sbi.dwSize.X)
	////    nNewWidth = sbi.dwSize.X;
	//return TRUE;
}

// Изменение значений переменной (флаг включенного скролла)
void CRealBuffer::SetBufferHeightMode(bool abBufferHeight, bool abIgnoreLock /*= false*/)
{
	if (bufferModeChangeLocked_)
	{
		if (!abIgnoreLock)
		{
			//_ASSERTE(mb_BuferModeChangeLocked==FALSE || abIgnoreLock);
			return;
		}
	}

	con.bBufferHeight = abBufferHeight;

	if (mp_RCon->isActive(false))
		OnBufferHeight();
}

// Вызывается из TabBar->ConEmu
void CRealBuffer::ChangeBufferHeightMode(bool abBufferHeight)
{
	if (abBufferHeight && gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 1)
	{
		// Win7 BUGBUG: Issue 192: падение Conhost при turn bufferheight ON
		// http://code.google.com/p/conemu-maximus5/issues/detail?id=192
		return;
		//const SHORT nMaxBuf = 600;
		//if (nNewBufHeightSize > nMaxBuf && mp_RCon->isFar())
		//	nNewBufHeightSize = nMaxBuf;
	}

	_ASSERTE(!bufferModeChangeLocked_);
	MSetter lSetter(&bufferModeChangeLocked_);
	con.bBufferHeight = abBufferHeight;

	// Если при запуске было "conemu.exe /bufferheight 0 ..."
	if (abBufferHeight /*&& !con.nBufferHeight*/)
	{
		// Если пользователь меняет высоту буфера в диалоге настроек
		con.nBufferHeight = gpSet->DefaultBufferHeight;

		if (con.nBufferHeight<300)
		{
			_ASSERTE(con.nBufferHeight>=300);
			con.nBufferHeight = std::max(300,con.nTextHeight*2);
		}
	}

	ResetLastMousePos();

	USHORT nNewBufHeightSize = abBufferHeight ? con.nBufferHeight : 0;
	SetConsoleSize(TextWidth(), TextHeight(), nNewBufHeightSize, CECMD_SETSIZESYNC);
}

void CRealBuffer::SetChange2Size(int anChange2TextWidth, int anChange2TextHeight)
{
	#ifdef _DEBUG
	if (anChange2TextHeight > 200)
	{
		_ASSERTE(anChange2TextHeight <= 200);
	}
	#endif

	con.nChange2TextWidth = anChange2TextWidth;
	con.nChange2TextHeight = anChange2TextHeight;
}

bool CRealBuffer::isBuferModeChangeLocked()
{
	return bufferModeChangeLocked_;
}

// Utilized in CRealServer::cmdStartStop
bool CRealBuffer::BuferModeChangeLock()
{
	bool lbNeedUnlock = !bufferModeChangeLocked_;
	bufferModeChangeLocked_ = true;
	return lbNeedUnlock;
}

// Utilized in CRealServer::cmdStartStop
void CRealBuffer::BuferModeChangeUnlock()
{
	bufferModeChangeLocked_ = false;
}

// По con.m_sbi проверяет, включена ли прокрутка
bool CRealBuffer::CheckBufferSize()
{
	bool lbForceUpdate = false;

	if (!this)
		return false;

	if (bufferModeChangeLocked_)
		return false;

	//if (con.m_sbi.dwSize.X>(con.m_sbi.srWindow.Right-con.m_sbi.srWindow.Left+1)) {
	//  DEBUGLOGFILE("Wrong screen buffer width\n");
	//  // Окошко консоли почему-то схлопнулось по горизонтали
	//  WARNING("пока убрал для теста");
	//  //MOVEWINDOW(mp_RCon->hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
	//} else {
	// BufferHeight может меняться и из плагина (например, DVDPanel), во время работы фара, или в других приложения (wmic)
	BOOL lbTurnedOn = IsBufferHeightTurnedOn(con.m_sbi);

	if (!lbTurnedOn && con.bBufferHeight)
	{
		// может быть консольная программа увеличила буфер самостоятельно?
		// TODO: отключить прокрутку!!!
		SetBufferHeightMode(FALSE);
		//UpdateScrollInfo(); -- зовется в ApplyConsoleInfo()
		lbForceUpdate = true;
	}
	else if (lbTurnedOn && !con.bBufferHeight)
	{
		SetBufferHeightMode(TRUE);
		//UpdateScrollInfo(); -- зовется в ApplyConsoleInfo()
		lbForceUpdate = true;
	}

	//TODO: А если высота буфера вдруг сменилась из самой консольной программы?
	//if ((BufferHeight == 0) && (con.m_sbi.dwSize.Y>(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+1))) {
	//  TODO("это может быть консольная программа увеличила буфер самостоятельно!")
	//      DEBUGLOGFILE("Wrong screen buffer height\n");
	//  // Окошко консоли почему-то схлопнулось по вертикали
	//  WARNING("пока убрал для теста");
	//  //MOVEWINDOW(mp_RCon->hConWnd, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 1);
	//}
	//TODO: Можно бы перенести в ConEmuC, если нужно будет
	//// При выходе из FAR -> CMD с BufferHeight - смена QuickEdit режима
	//DWORD mode = 0;
	//BOOL lb = FALSE;
	//if (BufferHeight) {
	//  //TODO: похоже, что для BufferHeight это вызывается постоянно?
	//  //lb = GetConsoleMode(hConIn(), &mode);
	//  mode = GetConsoleMode();
	//  if (con.m_sbi.dwSize.Y>(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+1)) {
	//      // Буфер больше высоты окна
	//      mode |= ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE|ENABLE_EXTENDED_FLAGS;
	//  } else {
	//      // Буфер равен высоте окна (значит ФАР запустился)
	//      mode &= ~(ENABLE_QUICK_EDIT_MODE|ENABLE_INSERT_MODE);
	//      mode |= ENABLE_EXTENDED_FLAGS;
	//  }
	//  TODO("SetConsoleMode");
	//  //lb = SetConsoleMode(hConIn(), mode);
	//}
	return lbForceUpdate;
}

bool CRealBuffer::LoadDataFromSrv(CRConDataGuard& data, DWORD CharCount, CHAR_INFO* pData)
{
	if (m_Type != rbt_Primary)
	{
		_ASSERTE(m_Type == rbt_Primary);
		return FALSE; // Изменений нет
	}

	size_t nCharCmp = std::min<size_t>(CharCount, con.nConBufCells);

	if (!nCharCmp || !data.isValid(CharCount))
	{
		Assert(nCharCmp && data.isValid(CharCount));
		return FALSE;
	}

	_ASSERTE(nCharCmp <= data->nMaxCells);

	HEAPVAL;
	bool lbScreenChanged = false;
	bool bTopChanged = false;
	bool bRestChanged = false;
	wchar_t* lpChar = data->pConChar;
	WORD* lpAttr = data->pConAttr;

	_ASSERTE(sizeof(*data->pDataCmp) == sizeof(*pData));

	// Tricky... Ignore top line (with clocks) in Far manager to skip gpSet->nTabFlashChanged
	// Do not take into account gpSet->nTabFlashChanged
	// because bConsoleDataChanged may be used in tab template
	if (mp_RCon->isFar())
	{
		size_t nTopCmp = std::min(nCharCmp, size_t(con.nTextWidth));
		size_t nRestCmp = nCharCmp - nTopCmp;
		bTopChanged = (memcmp(data->pDataCmp, pData, nTopCmp*sizeof(*pData)) != 0);
		bRestChanged = nRestCmp ? (memcmp(data->pDataCmp+nTopCmp, pData+nTopCmp, nRestCmp*sizeof(*pData)) != 0) : false;
	}
	else
	{
		bRestChanged = (memcmp(data->pDataCmp, pData, nCharCmp*sizeof(*pData)) != 0);
	}
	lbScreenChanged = (bRestChanged || bTopChanged);

	if (lbScreenChanged)
	{
		// Extended logging?
		if (IFLOGCONSOLECHANGE)
		{
			char sInfo[128];
			sprintf_c(sInfo, "DataCmp was changed, width=%u, height=%u, count=%u", con.nTextWidth, con.nTextHeight, CharCount);

			const CHAR_INFO* lp1 = data->pDataCmp;
			const CHAR_INFO* lp2 = pData;
			INT_PTR idx = -1;

			for (DWORD n = 0; n < nCharCmp; n++, lp1++, lp2++)
			{
				if (memcmp(lp1, lp2, sizeof(*lp2)) != 0)
				{
					idx = (lp1 - data->pDataCmp);
					int y = con.nTextWidth ? (idx / con.nTextWidth) : 0;
					int x = con.nTextWidth ? (idx - y * con.nTextWidth) : idx;

					sprintf_c(sInfo+strlen(sInfo), 32/*#SECURELEN*/, ", posY=%i, posX=%i", y, x);

					break;
				}
			}

			if (idx == -1)
			{
				lstrcatA(sInfo, ", failed to find change idx");
			}

			LOGCONSOLECHANGE(sInfo);
		}


		memmove(data->pDataCmp, pData, nCharCmp*sizeof(CHAR_INFO));
		HEAPVAL;

		CHAR_INFO* lpCur = data->pDataCmp;

		// Strip CHAR_INFO into text and attributes
		for (DWORD n = 0; n < nCharCmp; n++, lpCur++)
		{
			*(lpAttr++) = lpCur->Attributes;
			// If some tool write the '\0' to the console we would not be able to copy the console contents to clipboard
			*(lpChar++) = lpCur->Char.UnicodeChar ? lpCur->Char.UnicodeChar : L' ';
		}

		// ASCIIZ guarantee to use string functions
		if (lpChar < (data->pConChar + con.nConBufCells))
		{
			*lpChar = 0;
		}
		else
		{
			_ASSERTE(lpChar < (data->pConChar + con.nConBufCells));
		}

		con.mb_ConDataValid = true;

		HEAPVAL
	}

	if (bRestChanged)
	{
		mp_RCon->OnConsoleDataChanged();
	}

	return lbScreenChanged;
}

bool CRealBuffer::IsTrueColorerBufferChanged()
{
	AnnotationHeader frameHeader = {};
	const AnnotationInfo* frameCells = nullptr;
	bool absolute = false;
	if (mp_RCon->GetColorData(frameHeader, frameCells, absolute) && absolute)
	{
		const bool changed = mp_RCon->m_TrueColorerHeader.flushCounter != frameHeader.flushCounter;
		mp_RCon->m_TrueColorerHeader = frameHeader;
		return changed;
	}
	bool lbChanged = false;
	AnnotationHeader aHdr;
	int nCmp = 0;
	int nCurMax;
	MSectionLockSimple CS;

	if (!mp_RCon->UseTrueColor() || !mp_RCon->mp_TrueColorerData)
		goto wrap;

	// Проверка буфера TrueColor
	if (!mp_RCon->m_TrueColorerMap.GetTo(&aHdr, sizeof(aHdr)))
		goto wrap;

	// Check counter
	if (mp_RCon->m_TrueColorerHeader.flushCounter == aHdr.flushCounter)
		goto wrap;

	nCurMax = std::min(con.nTextWidth*con.nTextHeight,mp_RCon->mn_TrueColorMaxCells);
	if (nCurMax <= 0)
		goto wrap;

	// Compare data
	CS.Lock(m_TrueMode.pcsLock);
	{
		size_t cbCurSize = nCurMax*sizeof(*m_TrueMode.mp_Cmp);

		if (!m_TrueMode.mp_Cmp || (nCurMax > m_TrueMode.nCmpMax))
		{
			SafeFree(m_TrueMode.mp_Cmp);
			m_TrueMode.nCmpMax = nCurMax;
			m_TrueMode.mp_Cmp = (AnnotationInfo*)malloc(cbCurSize);
			nCmp = 2;
		}
		else
		{
			nCmp = memcmp(m_TrueMode.mp_Cmp, mp_RCon->mp_TrueColorerData, cbCurSize);
		}

		if ((nCmp != 0) && m_TrueMode.mp_Cmp)
		{
			memcpy(m_TrueMode.mp_Cmp, mp_RCon->mp_TrueColorerData, cbCurSize);
		}
	}
	CS.Unlock();


	if (nCmp != 0)
	{
		lbChanged = true;

		if (IFLOGCONSOLECHANGE)
		{
			char szDbgSize[128]; sprintf_c(szDbgSize, "ApplyConsoleInfo: TrueColorer.flushCounter(%u) -> changed(%u)", mp_RCon->m_TrueColorerHeader.flushCounter, aHdr.flushCounter);
			LOGCONSOLECHANGE(szDbgSize);
		}
	}

	// Save counter to avoid redundant checks
	mp_RCon->m_TrueColorerHeader = aHdr;
wrap:
	return lbChanged;
}

// Unlike `Alternative buffer` this returns `Paused` state of RealConsole
// We can "pause" applications which are using simple WriteFile to output data
bool CRealBuffer::isPaused()
{
	if (!this)
		return false;
	return ((con.Flags & CECI_Paused) == CECI_Paused);
}

void CRealBuffer::StorePausedState(CEPauseCmd state)
{
	if (!this)
		return;

	con.FlagsUpdateTick = GetTickCount();

	switch (state)
	{
	case CEPause_On:
		SetConEmuFlags(con.Flags, CECI_Paused, CECI_Paused);
		break;
	case CEPause_Off:
		SetConEmuFlags(con.Flags, CECI_Paused, CECI_None);
		break;
	default:
		con.FlagsUpdateTick = 0;
	}
}

bool CRealBuffer::ApplyConsoleInfo()
{
	bool bBufRecreate = false;
	bool lbChanged = false;

	#ifdef _DEBUG
	if (mp_RCon->mb_DebugLocked)
		return FALSE;
	int nConNo = gpConEmu->isVConValid(mp_RCon->mp_VCon);
	nConNo = nConNo;  // -V570
	#endif

	if (!mp_RCon->isServerAvailable())
	{
		// Сервер уже закрывается. попытка считать данные из консоли может привести к зависанию!
		mp_RCon->mh_ApplyFinished.Set();
		return FALSE;
	}

	// mh_ApplyFinished must be set when the expected data (or size) will be ready
	bool bSetApplyFinished = !con.bLockChange2Text;
	mp_RCon->mh_ApplyFinished.Reset();

	const CESERVER_REQ* pInfo = nullptr;
	CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_CONSOLEDATA, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_CONINFO));

	// Request exact TopLeft position if it is locked in GUI
	pIn->ReqConInfo.TopLeft = con.TopLeft;

	if (mp_RCon->mb_SwitchActiveServer)
	{
		// Skip this step. Waiting for new console server.
	}
	else if (!mp_RCon->m_ConsoleMap.IsValid())
	{
		_ASSERTE(mp_RCon->m_ConsoleMap.IsValid());
	}
	else if (!mp_RCon->m_GetDataPipe.Transact((CESERVER_REQ_HDR*)pIn, pIn->hdr.cbSize, (const CESERVER_REQ_HDR**)&pInfo) || !pInfo)
	{
		#ifdef _DEBUG
		if (mp_RCon->m_GetDataPipe.GetErrorCode() == ERROR_PIPE_NOT_CONNECTED)
		{
			int nDbg = ERROR_PIPE_NOT_CONNECTED;
		}
		else if (mp_RCon->m_GetDataPipe.GetErrorCode() == ERROR_BAD_PIPE)
		{
			int nDbg = ERROR_BAD_PIPE;
		}
		else
		{
			wchar_t szInfo[128];
			swprintf_c(szInfo, L"mp_RCon->m_GetDataPipe.Transact failed, code=%i\r\n", (int)mp_RCon->m_GetDataPipe.GetErrorCode());
			const CEStr pszFull(szInfo, mp_RCon->m_GetDataPipe.GetErrorText());
			//MBoxA(pszFull);
			LogString(pszFull);
		}
		#endif
	}
	else if (pInfo->hdr.cbSize < sizeof(CESERVER_REQ_CONSOLE_STATE))
	{
		_ASSERTE(pInfo->hdr.cbSize >= sizeof(CESERVER_REQ_CONSOLE_STATE));
	}
	else
	{
		ApplyConsoleInfo(pInfo, bSetApplyFinished, lbChanged, bBufRecreate);
	}

	if (bSetApplyFinished)
	{
		mp_RCon->mh_ApplyFinished.Set();
	}

	if (lbChanged)
	{
		mp_RCon->mb_DataChanged = TRUE; // Переменная используется внутри класса
		con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole

		mp_RCon->UpdateScrollInfo();
	}

	ExecuteFreeResult(pIn);

	return lbChanged;
}

void CRealBuffer::ApplyConsoleInfo(const CESERVER_REQ* pInfo, bool& bSetApplyFinished, bool& lbChanged, bool& bBufRecreate)
{
	bool bNeedLoadData = false;
	bool lbBufferChecked = false;

		//if (mn_LastConsoleDataIdx != mp_ConsoleInfo->nCurDataMapIdx) {
		//	ReopenMapData();
		//}
		DWORD nPID = GetCurrentProcessId();
		DWORD nMapGuiPID = mp_RCon->m_ConsoleMap.Ptr()->nGuiPID;

		if (nPID != nMapGuiPID)
		{
			// Если консоль запускалась как "-new_console" то nMapGuiPID может быть еще 0?
			// Хотя, это может случиться только если батник запущен из консоли НЕ прицепленной к GUI ConEmu.
			if (nMapGuiPID != 0)
			{
				_ASSERTE(nMapGuiPID == nPID);
			}
		}

		#ifdef _DEBUG
		HWND hWnd = pInfo->ConState.hConWnd;
		if (!hWnd || (hWnd != mp_RCon->hConWnd))
		{
			// Wine bug ? Incomplete packet?
			_ASSERTE(hWnd!=nullptr);
			_ASSERTE(hWnd==mp_RCon->hConWnd);
		}
		#endif

		if (con.Flags != pInfo->ConState.Flags)
		{
			if (!con.FlagsUpdateTick || (pInfo->ConState.nSrvUpdateTick > con.FlagsUpdateTick))
			{
				con.Flags = pInfo->ConState.Flags;
			}
		}

		//if (mp_RCon->hConWnd != hWnd) {
		//    SetHwnd ( hWnd ); -- низя. Maps уже созданы!
		//}
		// 3
		// Здесь у нас реальные процессы консоли, надо обновиться
		if (mp_RCon->ProcessUpdate(pInfo->ConState.nProcesses, countof(pInfo->ConState.nProcesses)))
		{
			//120325 - нет смысла перерисовывать консоль, если данные в ней не менялись.
			//  это приводит 1) к лишнему мельканию; 2) глюкам отрисовки в запущенных консольных приложениях
			//lbChanged = true; // если сменился статус (Far/не Far) - перерисовать на всякий случай
		}
		// Теперь нужно открыть секцию - начинаем изменение переменных класса
		MSectionLock sc;
		// 4
		DWORD dwCiSize = pInfo->ConState.dwCiSize;

		if (dwCiSize != 0)
		{
			_ASSERTE(dwCiSize == sizeof(con.m_ci));

			if (memcmp(&con.m_ci, &pInfo->ConState.ci, sizeof(con.m_ci))!=0)
			{
				LOGCONSOLECHANGE("ApplyConsoleInfo: CursorInfo changed");
				lbChanged = true;
			}

			con.m_ci = pInfo->ConState.ci;
		}

		// Show changes on the Settings/Info page
		bool bConCP = (con.m_dwConsoleCP != pInfo->ConState.dwConsoleCP);
		bool bConOutCP = (con.m_dwConsoleOutputCP != pInfo->ConState.dwConsoleOutputCP);
		bool bConMode = (con.m_dwConsoleInMode != pInfo->ConState.dwConsoleInMode) || (con.m_dwConsoleOutMode != pInfo->ConState.dwConsoleOutMode);

		// 5, 6, 7
		con.m_dwConsoleCP = pInfo->ConState.dwConsoleCP;
		con.m_dwConsoleOutputCP = pInfo->ConState.dwConsoleOutputCP;
		con.m_dwConsoleInMode = pInfo->ConState.dwConsoleInMode;
		con.m_dwConsoleOutMode = pInfo->ConState.dwConsoleOutMode;

		if (ghOpWnd
			&& (bConMode || bConCP || bConOutCP)
			&& mp_RCon->isActive(false))
		{
			if (bConMode)
				gpSetCls->UpdateConsoleMode(mp_RCon);
			if (bConCP || bConOutCP)
				gpConEmu->UpdateProcessDisplay(false);
		}

		// 8
		DWORD dwSbiSize = pInfo->ConState.dwSbiSize;
		int nNewWidth = 0, nNewHeight = 0;

		if (dwSbiSize != 0)
		{
			HEAPVAL
			_ASSERTE(dwSbiSize == sizeof(con.m_sbi));

			CONSOLE_SCREEN_BUFFER_INFO sbi = pInfo->ConState.sbi;
			_ASSERTE(sizeof(sbi.srWindow.Bottom)==sizeof(SHORT) && sizeof(sbi.srWindow)==4*sizeof(SHORT));
			if (sbi.srWindow.Left < 0 || sbi.srWindow.Top < 0 || sbi.srWindow.Right < 0 || sbi.srWindow.Bottom < 0)
			{
				_ASSERTE(sbi.srWindow.Left >= 0 && sbi.srWindow.Top >= 0 && sbi.srWindow.Right >= 0 && sbi.srWindow.Bottom >= 0);
				for (size_t i = 0; i < 4; ++i)
				{
					SHORT& n = ((SHORT*)&sbi.srWindow)[i];
					if (n < 0)
						n = std::numeric_limits<SHORT>::max();
				}
			}

			if (memcmp(&con.m_sbi, &sbi, sizeof(con.m_sbi))!=0)
			{
				LOGCONSOLECHANGE("ApplyConsoleInfo: ScreenBufferInfo changed");
				lbChanged = true;

				//if (mp_RCon->isActive(false))
				//	gpConEmu->UpdateCursorInfo(&pInfo->sbi, pInfo->sbi.dwCursorPosition, pInfo->ci);
			}

			#ifdef _DEBUG
			wchar_t szCursorDbg[255]; szCursorDbg[0] = 0;
			if (sbi.dwCursorPosition.X != con.m_sbi.dwCursorPosition.X || sbi.dwCursorPosition.Y != con.m_sbi.dwCursorPosition.Y)
				swprintf_c(szCursorDbg, L"CursorPos changed to %ux%u. ", sbi.dwCursorPosition.X, sbi.dwCursorPosition.Y);
			else
				swprintf_c(szCursorDbg, L"CursorPos is %ux%u. ", sbi.dwCursorPosition.X, sbi.dwCursorPosition.Y);
			#endif


			const LONG nLastConsoleRow = mp_RCon->m_AppMap.IsValid() ? mp_RCon->m_AppMap.Ptr()->nLastConsoleRow : -1;

			int newDynamicHeight = 0;
			bool lbRealTurnedOn = IsBufferHeightTurnedOn(sbi);
			con.srRealWindow = pInfo->ConState.srRealWindow;
			// if (nLastConsoleRow > 0) -- even if nLastConsoleRow is zero (no hooks in console application), try to deal with srWindow
			{
				const LONG maxRow = std::max<LONG>(nLastConsoleRow,
					std::max<LONG>(sbi.srWindow.Bottom,
						std::max<LONG>(con.srRealWindow.Bottom, sbi.dwCursorPosition.Y)));
				if (maxRow > 0 && maxRow < sbi.dwSize.Y)
					newDynamicHeight = maxRow + 1;
			}
			#ifdef _DEBUG
			if (con.nDynamicHeight != newDynamicHeight)
			{
				msprintf(szCursorDbg, countof(szCursorDbg), L"Dynamic height changed: %i -> %i (Known=%i, Bottom=%i, Real=%i, Cursor=%i)\n",
					con.nDynamicHeight, newDynamicHeight, nLastConsoleRow, sbi.srWindow.Bottom, con.srRealWindow.Bottom, sbi.dwCursorPosition.Y);
				DEBUGSTRDYNAMIC(szCursorDbg);
			}
			#endif
			con.m_sbi = sbi;
			con.nDynamicHeight = newDynamicHeight;
			con.nLastReportedConsoleRow = nLastConsoleRow;

			// Если мышкой тащат ползунок скроллера - не менять TopVisible
			if (!mp_RCon->InScroll()
				&& con.TopLeft.isLocked()
				&& !pInfo->ConState.TopLeft.isLocked())
			{
				// Сброс позиции прокрутки, она поменялась
				// в результате действий пользователя в консоли
				DEBUGTEST(DWORD nCurTick = GetTickCount());
				if (!con.InTopLeftSet
					&& (!con.TopLeftTick
						|| ((int)(pInfo->ConState.nSrvUpdateTick - con.TopLeftTick) > 0))
					)
				{
					// Сбрасывать не будем, просто откорректируем текущее положение
					SetTopLeft(pInfo->ConState.srRealWindow.Top, -1, false);
				}
			}


			DWORD nScroll;
			if (GetConWindowSize(sbi, &nNewWidth, &nNewHeight, &nScroll))
			{
				// Far sync resize (avoid panel flickering)
				// refresh VCon only when server return new size/data
				if (con.bLockChange2Text)
				{
					if ((con.nChange2TextWidth == nNewWidth) && (con.nChange2TextHeight == nNewHeight))
					{
						bSetApplyFinished = true;
					}
				}

				// #SIZE_TODO buffer mode may be changing at the moment by cmdStartStop, use mutex?
				if (!bufferModeChangeLocked_)
				{
					CheckBufferSize();
					lbBufferChecked = true;
				}

				//  TODO("Включить прокрутку? или оно само?");
				if (nNewWidth != con.nTextWidth || nNewHeight != con.nTextHeight)
				{
					if (IFLOGCONSOLECHANGE)
					{
						char szDbgSize[128]; sprintf_c(szDbgSize, "ApplyConsoleInfo: SizeWasChanged(cx=%i, cy=%i)", nNewWidth, nNewHeight);
						LOGCONSOLECHANGE(szDbgSize);
					}

					#ifdef _DEBUG
					wchar_t szDbgSize[128]; swprintf_c(szDbgSize, L"ApplyConsoleInfo.SizeWasChanged(cx=%i, cy=%i)", nNewWidth, nNewHeight);
					DEBUGSTRSIZE(szDbgSize);
					#endif

					//sc.Lock(&csCON, TRUE);
					//WARNING("может не заблокировалось?");

					// Смена размера, буфер пересоздается
					bBufRecreate = true;
				}
			}


			#if 0
			#ifdef SHOW_AUTOSCROLL
			if (gpSetCls->AutoScroll)
				con.nTopVisibleLine = con.m_sbi.srWindow.Top;
			#else
			// Если мышкой тащат ползунок скроллера - не менять TopVisible
			if (!mp_RCon->InScroll())
				con.nTopVisibleLine = con.m_sbi.srWindow.Top;
			#endif
			#endif

			HEAPVAL
		}


		if (nNewWidth && nNewHeight)
		{
			// Это может случиться во время пересоздания консоли (когда фар падал)
			// или при изменении параметров экрана (Aero->Standard)
			// или при закрытии фара (он "восстанавливает" размер консоли)
			//_ASSERTE((nNewWidth == pInfo->crWindow.X && nNewHeight == pInfo->crWindow.Y) || con.bLockChange2Text);

			DWORD CharCount = nNewWidth * nNewHeight;
			DWORD nCalcCount = 0;
			CHAR_INFO *pData = nullptr;

			// Если вместе с заголовком пришли измененные данные
			if (pInfo->ConState.nDataCount)
			{
				LOGCONSOLECHANGE("ApplyConsoleInfo: Console contents received");

				mp_RCon->mn_LastConsolePacketIdx = pInfo->ConState.nPacketId;

				if (CharCount < pInfo->ConState.nDataCount)
					CharCount = pInfo->ConState.nDataCount;

				#ifdef _DEBUG
				if (pInfo->ConState.nDataCount != (nNewWidth * nNewHeight))
				{
					// Это может случиться во время пересоздания консоли (когда фар падал)
					_ASSERTE(pInfo->ConState.nDataCount == (nNewWidth * nNewHeight));
				}
				#endif


				pData = (CHAR_INFO*)((&pInfo->ConState)+1);

				// Проверка размера!
				nCalcCount = (pInfo->hdr.cbSize - sizeof(pInfo->hdr) - sizeof(pInfo->ConState)) / sizeof(CHAR_INFO);
				#ifdef _DEBUG
				if (nCalcCount != CharCount)
				{
					_ASSERTE(nCalcCount == CharCount);
				}
				#endif
				if (nCalcCount > CharCount)
					nCalcCount = CharCount;
				HEAPVAL;

				bNeedLoadData = true;
			}

			if (bBufRecreate || bNeedLoadData)
			{
				CRConDataGuard data;
				sc.Lock(&csCON);

				if (InitBuffers(CharCount, nNewWidth, nNewHeight, &data))
				{
					if (bNeedLoadData && nCalcCount && pData)
					{
						data->m_sbi = con.m_sbi;
						data->nWidth = nNewWidth;
						data->nHeight = nNewHeight;

						if (LoadDataFromSrv(data, nCalcCount, pData))
						{
							LOGCONSOLECHANGE("ApplyConsoleInfo: InitBuffers&LoadDataFromSrv -> changed");
							lbChanged = true;

							mp_RCon->UpdateStartState(rss_DataAquired);

							// Update keyboard performance, do that here
							// but not in the CRealConsole::OnConsoleDataChanged()
							// because we are interested in cursor moving too!
							if (mp_RCon->isActive(false))
							{
								gpSetCls->Performance(tPerfKeyboard, TRUE);
							}
						}
					}
				}
			}

			if (mp_RCon->m_ConStatus.Options & CRealConsole::cso_ResetOnConsoleReady)
			{
				mp_RCon->SetConStatus(nullptr);
				//_ASSERTE(mp_RCon->m_ConStatus.Options==0);
			}
		}

		// #SIZE_TODO Во время ресайза консоль может подглючивать - отдает не то что нужно...
		//_ASSERTE(*con.pConChar!=ucBoxDblVert);

		// пока выполняется SetConsoleSizeSrv в другой нити Нельзя сбрасывать эти переменные!
		if (!con.bLockChange2Text
			&& ((con.nChange2TextWidth >= 0) || (con.nChange2TextHeight >= 0)))
		{
			SetChange2Size(-1, -1);
		}

		#ifdef _DEBUG
		{
			wchar_t szCursorInfo[60];
			swprintf_c(szCursorInfo, L"Cursor (X=%i, Y=%i, Vis:%i, H:%i)\n",
						con.m_sbi.dwCursorPosition.X, con.m_sbi.dwCursorPosition.Y,
						con.m_ci.bVisible, con.m_ci.dwSize);
			DEBUGSTRPKT(szCursorInfo);

			unsigned TextLen = con.nTextWidth * con.nTextHeight;

			// Must be filled!
			CRConDataGuard data;
			if (GetData(data) && data.isValid(TextLen))
			{
				BOOL lbDataValid = TRUE; unsigned n = 0;
				_ASSERTE(con.nTextWidth == con.m_sbi.dwSize.X);

				wchar_t* pConChar = data->pConChar;
				WORD* pConAttr = data->pConAttr;
				while (n < TextLen)
				{
					if (pConChar[n] == 0)
					{
						lbDataValid = FALSE; break;
					}
					else if (pConChar[n] != L' ')
					{
						// 0 - может быть только для пробела. Иначе символ будет скрытым, чего по идее, быть не должно
						if (pConAttr[n] == 0)
						{
							lbDataValid = FALSE; break;
						}
					}

					n++;
				}
			}

			//_ASSERTE(lbDataValid);
			HEAPVAL;
		}
		#endif

		// Проверка буфера TrueColor
		if (!lbChanged && mp_RCon->UseTrueColor() && mp_RCon->mp_TrueColorerData)
		{
			if (IsTrueColorerBufferChanged())
			{
				lbChanged = true;
			}
		}

		if (lbChanged && !lbBufferChecked)
		{
			// По con.m_sbi проверяет, включена ли прокрутка
			CheckBufferSize();
			HEAPVAL;
		}
}

// По переданному CONSOLE_SCREEN_BUFFER_INFO определяет, включена ли прокрутка
// static
bool CRealBuffer::IsBufferHeightTurnedOn(const CONSOLE_SCREEN_BUFFER_INFO& psbi)
{
	bool lbTurnedOn = false;
	TODO("!!! Скорректировать");

	if (psbi.dwSize.Y <= (psbi.srWindow.Bottom - psbi.srWindow.Top + 1))
	{
		_ASSERTE(psbi.dwSize.Y == (psbi.srWindow.Bottom - psbi.srWindow.Top + 1))
		// высота окна == высоте буфера,
		lbTurnedOn = false;
	}
	else
	{
		//Issue 509: Могут быть различные варианты, когда меняется ВИДИМАЯ область,
		//           но не меняется высота буфера. В этом случае, буфер включать нельзя!
		TODO("Тут нужно бы сравнивать не с TextHeight(), а с высотой буфера. Но она инициализируется пока только для режима с прокруткой.");
		int nHeight = TextHeight();
		// Высота буфера 'намного' больше высоты НАШЕГО окна
		if (psbi.dwSize.Y > EvalBufferTurnOnSize(nHeight))
			lbTurnedOn = true;
	}

	return lbTurnedOn;
}

void CRealBuffer::OnBufferHeight()
{
	AssertThis();

	// При смене высоты буфера - сбросить последнее мышиное событие
	ResetLastMousePos();

	gpConEmu->OnBufferHeight();
}

// Если включена прокрутка - скорректировать индекс ячейки из экранных в буферные
COORD CRealBuffer::ScreenToBuffer(COORD crMouse)
{
	if (!this)
		return crMouse;

	if (isScroll())
	{
		#if 0
		// Прокрутка может быть заблокирована?
		_ASSERTE((con.nTopVisibleLine == con.m_sbi.srWindow.Top) || (con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION));
		#endif

		crMouse.X += con.m_sbi.srWindow.Left;
		crMouse.Y += con.m_sbi.srWindow.Top;
	}

	return crMouse;
}

COORD CRealBuffer::BufferToScreen(COORD crMouse, bool bFixup /*= true*/, bool bVertOnly /*= false*/)
{
	if (!this)
		return crMouse;

	if (isScroll())
	{
		if (!bVertOnly)
			crMouse.X = crMouse.X-con.m_sbi.srWindow.Left;
		crMouse.Y = crMouse.Y-con.m_sbi.srWindow.Top;
	}

	if (bFixup)
	{
		crMouse.X = std::max(0, std::min<int>(crMouse.X, GetTextWidth()-1));
		crMouse.Y = std::max(0, std::min<int>(crMouse.Y, GetTextHeight()-1));
	}

	return crMouse;
}

ExpandTextRangeType CRealBuffer::GetLastTextRangeType()
{
	if (!this)
		return etr_None;
	return con.etr.etrLast;
}

bool CRealBuffer::ResetLastMousePos()
{
	mcr_LastMousePos = MakeCoord(-1,-1);

	bool bChanged = (con.etr.etrLast != etr_None);

	if (bChanged)
		StoreLastTextRange(etr_None);

	return bChanged;
}

void CRealBuffer::ResetHighlightHyperlinks()
{
	con.etr.etrLast = etr_None;
}

bool CRealBuffer::ProcessFarHyperlink(bool bUpdateScreen)
{
	bool bChanged = false;

	// Console may scroll after last check time
	if (bUpdateScreen)
	{
		POINT ptCur = {}; GetCursorPos(&ptCur);
		RECT rcClient = {}; GetWindowRect(mp_RCon->VCon()->GetView(), &rcClient);
		if (!PtInRect(&rcClient, ptCur))
		{
			if (mcr_LastMousePos.X != -1)
				bChanged |= ResetLastMousePos();
		}
		else
		{
			ptCur.x -= rcClient.left; ptCur.y -= rcClient.top;
			COORD crMouse = ScreenToBuffer(mp_RCon->VCon()->ClientToConsole(ptCur.x, ptCur.y));
			mcr_LastMousePos = crMouse;
		}
	}

	if ((mcr_LastMousePos.X != -1) && ((mcr_LastMousePos.Y < con.m_sbi.srWindow.Top) || (mcr_LastMousePos.Y >= GetBufferHeight())))
	{
		bChanged |= ResetLastMousePos();
	}
	else if ((mcr_LastMousePos.X == -1) && (con.etr.etrLast != etr_None))
	{
		bChanged |= ResetLastMousePos();
	}
	else
	{
		bChanged |= ProcessFarHyperlink(WM_MOUSEMOVE, mcr_LastMousePos, bUpdateScreen);
	}

	return bChanged;
}

bool CRealBuffer::CanProcessHyperlink(const COORD& crMouse)
{
	if (!mp_RCon->isActive(false))
		return false;

	// Disallow hyperlinks on Far panels
	if (mp_RCon->isFar())
	{
		RECT rc;
		COORD crMap = BufferToScreen(crMouse, false, false);
		if (isLeftPanel() && GetPanelRect(FALSE, &rc, TRUE, TRUE) && CoordInRect(crMap, rc))
			return false;
		if (isRightPanel() && GetPanelRect(TRUE, &rc, TRUE, TRUE) && CoordInRect(crMap, rc))
			return false;
	}

	// Allow
	return true;
}

bool CRealBuffer::ProcessFarHyperlink(UINT messg, COORD crFrom, bool bUpdateScreen)
{
	if (!mp_RCon->IsFarHyperlinkAllowed(false))
		return false;

	if (!isDataValid())
		return false;

	bool lbProcessed = false;

	// переходим на абсолютные координаты
	COORD crStart = crFrom; // MakeCoord(crFrom.X - con.m_sbi.srWindow.Left, crFrom.Y - con.m_sbi.srWindow.Top);

	// Во время скролла координата может быстро улететь
	if ((crStart.Y < 0 || crStart.Y >= GetBufferHeight())
		|| (crStart.X < 0 || crStart.X >= GetBufferWidth()))
	{
		_ASSERTE((crStart.X==-1 && crStart.Y==-1) && "Must not get here");

		bool bChanged = ResetLastMousePos();
		return bChanged;
	}

	HooksUnlocker;

	COORD crEnd = crStart;
	CEStr szText;
	ExpandTextRangeType rc = etr_None;
	if (CanProcessHyperlink(crStart))
	{
		if (m_Type == rbt_Primary && mp_RCon->GetConPtyLink(crStart, szText, crStart, crEnd))
		{
			rc = etr_Url;
			StoreLastTextRange(rc);
		}
		else
			rc = ExpandTextRange(crStart, crStart, crEnd, etr_AnyClickable, &szText);
	}
	bool bChanged = con.etrWasChanged || (con.etr.etrLast != rc);
	if (memcmp(&crStart, &con.etr.mcr_FileLineStart, sizeof(crStart)) != 0
		|| memcmp(&crEnd, &con.etr.mcr_FileLineEnd, sizeof(crStart)) != 0)
	{
		con.etr.mcr_FileLineStart = crStart;
		con.etr.mcr_FileLineEnd = crEnd;
		// bUpdateScreen если вызов идет из GetConsoleData для коррекции отдаваемых координат
		bChanged = true;
	}

	if (bChanged && bUpdateScreen)
	{
		// Refresh on-screen
		UpdateHyperlink();
	}

	if ((rc & etr_File) || (rc & etr_Url))
	{
		if ((messg == WM_LBUTTONUP || messg == WM_LBUTTONDBLCLK) && !szText.IsEmpty())
		{
			if (rc & etr_Url)
			{
				INT_PTR iRc = (INT_PTR)ShellExecute(ghWnd, L"open", szText, nullptr, nullptr, SW_SHOWNORMAL);
				if (iRc <= 32)
				{
					DisplayLastError(szText, iRc, MB_ICONSTOP, L"URL open failed");
				}
			}
			else if (rc & etr_File)
			{
				// Найти номер строки
				CESERVER_REQ_FAREDITOR cmd = {sizeof(cmd)};
				wchar_t* pszText = szText.ms_Val;
				int nLen = lstrlen(pszText)-1;

				if (rc & etr_Row)
				{
					if (pszText[nLen] == L')')
					{
						pszText[nLen] = 0;
						nLen--;
					}
					while ((nLen > 0)
						&& (((pszText[nLen-1] >= L'0') && (pszText[nLen-1] <= L'9'))
							|| ((pszText[nLen-1] == L',') && ((pszText[nLen] >= L'0') && (pszText[nLen] <= L'9')))))
					{
						nLen--;
					}
				}

				if (nLen < 3)
				{
					_ASSERTE(nLen >= 3);
				}
				else
				{ // 1.c:3:
					wchar_t* pszEnd;
					cmd.nLine = wcstol(pszText+nLen, &pszEnd, 10);
					if (pszEnd && (*pszEnd == L',') && isDigit(*(pszEnd+1)))
						cmd.nColon = wcstol(pszEnd+1, &pszEnd, 10);
					if (cmd.nColon < 1)
						cmd.nColon = 1;
					if (rc & etr_Row)
						pszText[nLen-1] = 0;
					while ((pszEnd = wcschr(pszText, L'/')) != nullptr)
						*pszEnd = L'\\'; // заменить прямые слеши на обратные

					CEStr szWinPath;
					LPCWSTR pszWinPath = mp_RCon->GetFileFromConsole(pszText, szWinPath);
					if (pszWinPath)
					{
						// May be too long?
						lstrcpyn(cmd.szFile, pszWinPath, countof(cmd.szFile));
						// Must be available
						if (!FileExists(cmd.szFile))
						{
							// Not found
							pszWinPath = nullptr;
						}
					}
					// Not found?
					if (!pszWinPath)
					{
						lstrcpyn(cmd.szFile, pszText, countof(cmd.szFile));
					}

					TODO("Для удобства, если лог открыт в редакторе, или пропустить мышь, или позвать макрос");
					// Только нужно учесть, что текущий таб может быть редактором, но открыт UserScreen (CtrlO)

					// Проверить, может уже открыт таб с этим файлом?
					LPCWSTR pszFileName = wcsrchr(cmd.szFile, L'\\');
					if (!pszFileName) pszFileName = cmd.szFile; else pszFileName++;
					CVConGuard VCon;

					//// Сброс подчерка, а то при возврате в консоль,
					//// когда модификатор уже отпущен, остается артефакт...
					//StoreLastTextRange(etr_None);
					//UpdateSelection();

					int liActivated = gpConEmu->mp_TabBar->ActiveTabByName(fwt_Editor|fwt_ActivateFound|fwt_PluginRequired, cmd.szFile/*pszFileName*/, &VCon);

					if (liActivated == -2)
					{
						// Нашли, но активировать нельзя, TabBar должен был показать всплывающую подсказку с ошибкой
						_ASSERTE(FALSE);
					}
					else
					{
						if (liActivated >= 0)
						{
							// Нашли, активировали, нужно только на строку перейти
							if (cmd.nLine > 0)
							{
								wchar_t szMacro[96];
								_ASSERTE(VCon.VCon()!=nullptr);
								CRealConsole* pRCon = VCon->RCon();

								if (pRCon->m_FarInfo.FarVer.dwVerMajor == 1)
									swprintf_c(szMacro, L"@$if(Editor) AltF8 \"%i:%i\" Enter $end", cmd.nLine, cmd.nColon);
								else if (pRCon->m_FarInfo.FarVer.IsFarLua())
									swprintf_c(szMacro, L"@if Area.Editor then Keys(\"AltF8\") print(\"%i:%i\") Keys(\"Enter\") end", cmd.nLine, cmd.nColon);
								else
									swprintf_c(szMacro, L"@$if(Editor) AltF8 print(\"%i:%i\") Enter $end", cmd.nLine, cmd.nColon);

								// -- Послать что-нибудь в консоль, чтобы фар ушел из UserScreen открытого через редактор?
								//PostMouseEvent(WM_LBUTTONUP, 0, crFrom);

								// Ок, переход на строку (макрос)
								pRCon->PostMacro(szMacro, TRUE);
							}
						}
						else
						{
							// Если явно указан другой внешний редактор - всегда использовать его
							bool bUseExtEditor = false;
							LPCWSTR pszTemp = gpSet->sFarGotoEditor;
							CmdArg szExe;
							if ((pszTemp = NextArg(pszTemp, szExe)))
							{
								if (!IsFarExe(PointToName(szExe)))
									bUseExtEditor = true;
							}

							CVConGuard VCon;
							if (!pszWinPath || !*pszWinPath)
							{
								//_ASSERTE(pszWinPath!=nullptr); // must not be here!
								//pszWinPath = cmd.szFile; -- file not found, do not open absent files!
								CEStr szDir;
								const CEStr pszErrMsg(L"File '", cmd.szFile, L"' not found!\nDirectory: ", mp_RCon->GetConsoleCurDir(szDir, false));
								if (pszErrMsg)
								{
									MsgBox(pszErrMsg, MB_ICONSTOP);
								}
							}
							else if (bUseExtEditor || !CVConGroup::isFarExist(fwt_NonModal|fwt_PluginRequired, nullptr, &VCon))
							{
								if (gpSet->sFarGotoEditor && *gpSet->sFarGotoEditor)
								{
									wchar_t szRow[32], szCol[32];
									swprintf_c(szRow, L"%u", cmd.nLine);
									swprintf_c(szCol, L"%u", cmd.nColon);
									//LPCWSTR pszVar[] = {L"%1", L"%2", L"%3", ...};
									//%3’ - C:\\Path\\File, ‘%4’ - C:/Path/File, ‘%5’ - /C/Path/File

									CEStr szSlashed(MakeStraightSlashPath(pszWinPath));
									CEStr szCygwin;  DupCygwinPath(pszWinPath, false, mp_RCon->GetMntPrefix(), szCygwin);
									LPCWSTR pszVal[] = {szRow, szCol, pszWinPath, (LPCWSTR)szSlashed, (LPCWSTR)szCygwin};
									//_ASSERTE(countof(pszVar)==countof(pszVal));
									CEStr pszCmd = ExpandMacroValues(gpSet->sFarGotoEditor, pszVal, countof(pszVal));
									if (!pszCmd)
									{
										DisplayLastError(L"Invalid command specified in \"External editor\"", -1);
									}
									else
									{
										RConStartArgsEx args;
										bool bRunOutside = (pszCmd[0] == L'#');
										if (bRunOutside)
										{
											args.pszSpecialCmd = lstrdup(pszCmd.c_str() + 1).Detach();
											pszCmd.Release();
										}
										else
										{
											args.pszSpecialCmd = pszCmd.Detach();
										}

										WARNING("Здесь нужно бы попытаться взять текущую директорию из шелла. Точнее, из консоли НА МОМЕНТ выдачи этой строки.");
										args.pszStartupDir = CEStr(mp_RCon->m_Args.pszStartupDir).Detach();
										args.RunAsAdministrator = mp_RCon->m_Args.RunAsAdministrator;
										args.ForceUserDialog = (
												(mp_RCon->m_Args.ForceUserDialog == crb_On)
												|| (mp_RCon->m_Args.RunAsRestricted == crb_On)
												|| (mp_RCon->m_Args.pszUserName != nullptr))
											? crb_On : crb_Off;
										args.BufHeight = crb_On;
										//args.eConfirmation = RConStartArgsEx::eConfNever;

										if (bRunOutside)
										{
											// Need to check registry for 'App Paths' and set up '%PATH%'
											pszTemp = args.pszSpecialCmd;
											szExe.Release();
											CEnvRestorer szPrevPath;
											if ((pszTemp = NextArg(pszTemp, szExe)))
											{
												if (SearchAppPaths(szExe.c_str(), szExe, true, &szPrevPath))
												{
													CEStr pszChanged(MergeCmdLine(szExe, pszTemp));
													if (pszChanged)
													{
														SafeFree(args.pszSpecialCmd);
														args.pszSpecialCmd = pszChanged.Detach();
													}
												}
											}

											DWORD dwLastError = 0;
											LPCWSTR pszDir = args.pszStartupDir;
											STARTUPINFO si = {};
											si.cb = sizeof(si);
											si.dwFlags = STARTF_USESHOWWINDOW; si.wShowWindow = SW_SHOWNORMAL;
											PROCESS_INFORMATION pi = {};

											if (CRealConsole::CreateOrRunAs(mp_RCon, args, args.pszSpecialCmd, pszDir, si, pi, mpp_RunHyperlink, dwLastError, true))
											{
												HANDLE hProcess = nullptr;
												if (mpp_RunHyperlink)
												{
													hProcess = mpp_RunHyperlink->hProcess;
													SafeFree(mpp_RunHyperlink);
												}
												else
												{
													hProcess = pi.hProcess;
												}
												SafeCloseHandle(pi.hThread);

												// If Ctrl (or any other key) is still pressed during external editor startup,
												// started application may fail to get the focus - it starts below ConEmu window
												if (hProcess && pi.dwProcessId)
												{
													HWND hFore;
													DWORD nStart = GetTickCount(), nMax = 3000, nDelay = 0;
													// So, wait a little for 3sec, while ConEmu has focus
													while (((hFore = GetForegroundWindow()) == ghWnd)
														&& (WaitForSingleObject(hProcess, 100) == WAIT_TIMEOUT)
														&& (nDelay <= nMax))
													{
														HWND hApp = FindProcessWindow(pi.dwProcessId);
														if (hApp)
														{
															SetForegroundWindow(hApp);
															break;
														}

														nDelay = (GetTickCount() - nStart);
													}
												}

												SafeCloseHandle(hProcess);
											}
											else
											{
												DisplayLastError(args.pszSpecialCmd, dwLastError);
											}
										}
										else
										{
											gpConEmu->CreateCon(args, true);
										}
									}
								}
								else
								{
									DisplayLastError(L"Available Far Manager was not found in open tabs", -1);
								}
							}
							else
							{
								// -- Послать что-нибудь в консоль, чтобы фар ушел из UserScreen открытого через редактор?
								//PostMouseEvent(WM_LBUTTONUP, 0, crFrom);

								// Prepared, можно звать плагин
								VCon->RCon()->PostCommand(CMD_OPENEDITORLINE, sizeof(cmd), &cmd);
								if (!VCon->isActive(false))
								{
									gpConEmu->Activate(VCon.VCon());
								}
							}
						}
					}
				}
			}
		}
		lbProcessed = true;
	}

	return lbProcessed;
}

void CRealBuffer::ShowKeyBarHint(WORD nID)
{
	if ((nID > 0) && (nID <= countof(gnKeyBarFlags)))
	{
		// Нужен какой-то безопасный способ "обновить" кейбар, но так,
		// чтобы не сработали макросы, назначенные на одиночные модификаторы!
		//INPUT_RECORD r = {KEY_EVENT};
		//r.Event.MouseEvent.dwMousePosition.X = nID;
		//r.Event.MouseEvent.dwControlKeyState = gnKeyBarFlags[nID - 1];
		//r.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
		//mp_RCon->PostConsoleEvent(&r);
		mp_RCon->PostKeyPress(VK_RWIN, gnKeyBarFlags[nID - 1], 0);
	}
}

// If console IN selection - do autoscroll when mouse if outside or near VCon edges
// If NO selection present - ensure that coordinates are inside our VCon (otherwise - exit)
bool CRealBuffer::PatchMouseCoords(int& x, int& y, COORD& crMouse)
{
	const bool mouseSelection = ((con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION) != 0);

	if ((crMouse.X >= con.m_sbi.srWindow.Left) && (crMouse.X <= con.m_sbi.srWindow.Right)
		&& ((!mouseSelection && (crMouse.Y >= con.m_sbi.srWindow.Top) && (crMouse.Y <= con.m_sbi.srWindow.Bottom))
		|| (mouseSelection && (crMouse.Y > con.m_sbi.srWindow.Top) && (crMouse.Y < con.m_sbi.srWindow.Bottom))))
	{
		DEBUGSTRMOUSE(L"Nothing need to be patched, coordinates are OK (1)\n");
		return true;
	}

	const int nVConHeight = mp_RCon->VCon()->GetVConHeight();

	if (mouseSelection
		&& ((crMouse.Y == con.m_sbi.srWindow.Top) && (y >= SEL_MOUSE_AUTOSCROLL_PIX))
			|| ((crMouse.Y == con.m_sbi.srWindow.Bottom) && (y <= (nVConHeight-SEL_MOUSE_AUTOSCROLL_PIX))))
	{
		DEBUGSTRMOUSE(L"Nothing need to be patched, coordinates are OK (2)\n");
		return true;
	}

	// In mouse selection only
	if (!mouseSelection
		// And mouse button must be still pressed
		|| !(con.m_sel.dwFlags & CONSOLE_MOUSE_DOWN)
		// And VCon must be active
		|| !mp_RCon->VCon()->isActive(false))
	{
		// No selection, mouse outside of VCon, skip this message!
		DEBUGSTRMOUSE(L"!!! No mouse selection or mouse outside of VCon, message skipped !!!\n");
		return false;
	}

	wchar_t dbgBuf[80];

	// Avoid too fast scrolling
	const DWORD nCurTick = GetTickCount();
	const DWORD nDelta = (nCurTick - con.m_SelLastScrollCheck);
	if (nDelta < SEL_MOUSE_AUTOSCROLL_DELTA)
	{
		msprintf(dbgBuf, countof(dbgBuf), L"Mouse selection autoscroll skipped (waiting %ums)\n", SEL_MOUSE_AUTOSCROLL_DELTA);
		DEBUGSTRMOUSE(dbgBuf);
		return false;
	}
	con.m_SelLastScrollCheck = nCurTick;

	// Lets scroll window content
	if ((crMouse.Y < con.m_sbi.srWindow.Top) || (y < SEL_MOUSE_AUTOSCROLL_PIX))
	{
		const auto durationMs = nCurTick - con.m_SelClickTick;
		const auto scrollLines = GetAutoscrollSelectionLines(-y);
		msprintf(dbgBuf, countof(dbgBuf), L"Mouse selection autoscroll buffer up, duration=%u, delta=%i, lines=%u\n", durationMs, y, scrollLines);
		DEBUGSTRMOUSE(dbgBuf);
		crMouse.Y = std::max<SHORT>(0, con.m_sbi.srWindow.Top - 1);
		DoScrollBuffer(SB_LINEUP, -1, scrollLines);
		y = 0;
	}
	else if ((crMouse.Y > con.m_sbi.srWindow.Bottom) || (y > (nVConHeight - SEL_MOUSE_AUTOSCROLL_PIX)))
	{
		const auto durationMs = nCurTick - con.m_SelClickTick;
		const auto scrollLines = GetAutoscrollSelectionLines(y - nVConHeight);
		msprintf(dbgBuf, countof(dbgBuf), L"Mouse selection autoscroll buffer down, duration=%u, delta=%i, lines=%u\n", durationMs, y - nVConHeight, scrollLines);
		DEBUGSTRMOUSE(dbgBuf);
		crMouse.Y = std::min<SHORT>(con.m_sbi.srWindow.Bottom + 1, con.m_sbi.dwSize.Y - 1);
		DoScrollBuffer(SB_LINEDOWN, -1, scrollLines);
		y = (nVConHeight - 1);
	}

	// And patch X coords (while we are not support X scrolling yet)
	if (crMouse.X < con.m_sbi.srWindow.Left)
	{
		crMouse.X = con.m_sbi.srWindow.Left;
		x = 0;
	}
	else if (crMouse.X > con.m_sbi.srWindow.Right)
	{
		crMouse.X = con.m_sbi.srWindow.Right;
		x = (mp_RCon->VCon()->GetVConWidth() - 1);
	}

	return true;
}

void CRealBuffer::OnTimerCheckSelection()
{
	// In mouse selection only
	if (!(con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION))
		return;

	POINT ptCur = {}; GetCursorPos(&ptCur);
	MapWindowPoints(nullptr, mp_RCon->VCon()->GetView(), &ptCur, 1);
	int nVConHeight = mp_RCon->VCon()->GetVConHeight();

	if ((ptCur.y < SEL_MOUSE_AUTOSCROLL_PIX) || (ptCur.y > (nVConHeight-SEL_MOUSE_AUTOSCROLL_PIX)))
	{
		COORD crMouse = ScreenToBuffer(mp_RCon->VCon()->ClientToConsole(ptCur.x, ptCur.y));
		int x = ptCur.x, y = ptCur.y;
		WPARAM wParam = (isPressed(VK_LBUTTON) ? MK_LBUTTON : 0)
			| (isPressed(VK_RBUTTON) ? MK_RBUTTON : 0)
			| (isPressed(VK_MBUTTON) ? MK_MBUTTON : 0);
		OnMouse(WM_MOUSEMOVE, wParam, x, y, crMouse);
	}
}

// x,y - экранные координаты
// crMouse - ScreenToBuffer
// Возвращает true, если мышку обработал "сам буфер"
bool CRealBuffer::OnMouse(UINT messg, WPARAM wParam, int x, int y, COORD crMouse)
{
	#ifdef _DEBUG
	wchar_t szDbgInfo[200]; swprintf_c(szDbgInfo, L"RBuf::OnMouse %s XY={%i,%i} CR={%i,%i} SelFlags=x%08X\n",
		messg==WM_MOUSEMOVE?L"WM_MOUSEMOVE":
		messg==WM_LBUTTONDOWN?L"WM_LBUTTONDOWN":messg==WM_LBUTTONUP?L"WM_LBUTTONUP":messg==WM_LBUTTONDBLCLK?L"WM_LBUTTONDBLCLK":
		messg==WM_RBUTTONDOWN?L"WM_RBUTTONDOWN":messg==WM_RBUTTONUP?L"WM_RBUTTONUP":messg==WM_RBUTTONDBLCLK?L"WM_RBUTTONDBLCLK":
		messg==WM_MBUTTONDOWN?L"WM_MBUTTONDOWN":messg==WM_MBUTTONUP?L"WM_MBUTTONUP":messg==WM_MBUTTONDBLCLK?L"WM_MBUTTONDBLCLK":
		messg==WM_MOUSEWHEEL?L"WM_MOUSEWHEEL":messg==WM_MOUSEHWHEEL?L"WM_MOUSEHWHEEL":
		L"{OtherMsg}", x,y, crMouse.X,crMouse.Y, con.m_sel.dwFlags);
	DEBUGSTRMOUSE(szDbgInfo);
	#endif

	bool lbSkip = true;
	bool lbFarBufferSupported = false;
	bool lbMouseSendAllowed = false;
	bool lbMouseOverScroll = false;
	bool bSelAllowed = false;
	DWORD nModifierPressed = 0;
	DWORD nModifierNoEmptyPressed = 0;

	// Ensure that coordinates are correct
	if (!PatchMouseCoords(x, y, crMouse))
	{
		if (isSelectionPresent())
			goto wrap; // even if outside - don't send to console!
		lbSkip = false;
		goto wrap;
	}

	mcr_LastMousePos = crMouse;

	bSelAllowed = isSelectionPresent();
	// No sense to query isSelectionAllowed on any MouseMove
	if (!bSelAllowed)
	{
		if ((con.ISel.State != IS_None)
			|| (messg == WM_LBUTTONDOWN || messg == WM_LBUTTONUP || messg == WM_LBUTTONDBLCLK)
			|| (messg == WM_RBUTTONDOWN || messg == WM_RBUTTONUP)
			|| (messg == WM_MBUTTONDOWN || messg == WM_MBUTTONUP)
			)
		{
			bSelAllowed = isSelectionAllowed();
			#ifdef _DEBUG
			if (bSelAllowed)
			{
				DEBUGSTRSEL(L"Selection: isSelectionAllowed()==true");
			}
			#endif
		}
	}

	// We have to do these checks always, otherwise MouseMove events would be
	// bypassed to Far Menus, History etc. regardless Alt/Shift are pressed.
	nModifierPressed = gpConEmu->isSelectionModifierPressed(true);
	nModifierNoEmptyPressed = gpConEmu->isSelectionModifierPressed(false);

	if (bSelAllowed && gpSet->isCTSIntelligent
		&& !mp_RCon->m_Term.nMouseMode
		&& !isSelectionPresent()
		&& !nModifierPressed)
	{
		if (messg == WM_LBUTTONDOWN)
		{
			DEBUGSTRSEL(L"Selection: set IS_LBtnDown on WM_LBUTTONDOWN");
			con.ISel.LClickPt = MakePoint(x,y);
			con.ISel.ClickTick = GetTickCount();
			con.ISel.State = IS_LBtnDown;
		}
		else if (con.ISel.State)
		{
			if (!isPressed(VK_LBUTTON))
			{
				DEBUGSTRSEL(L"Selection: set IS_LBtnReleased");
				con.ISel.State = IS_LBtnReleased;
			}
			else
			{
				int nMinX = gpFontMgr->FontWidth();
				int nMinY = gpFontMgr->FontHeight();
				int nMinDiff = std::max(nMinX, nMinY);
				int nXdiff = x - con.ISel.LClickPt.x;
				int nYdiff = y - con.ISel.LClickPt.y;

				if ((messg == WM_MOUSEMOVE) && (con.ISel.State == IS_LBtnDown))
				{
					// Check first for Text selection
					DWORD nForce = 0;
					if (_abs(nXdiff) >= nMinDiff)
						nForce = CONSOLE_TEXT_SELECTION;
					else if (_abs(nYdiff) >= nMinDiff)
						nForce = CONSOLE_BLOCK_SELECTION;
					// GO!
					if (nForce)
					{
						DEBUGSTRSEL(L"Selection: Starting due to MouseMove and IntelliSel");
						gpConEmu->ForceSelectionModifierPressed(nForce);
						_ASSERTE((nForce & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION)) == nForce);
						SetSelectionFlags(con.m_sel.dwFlags | (nForce & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION)));
						OnMouseSelection(WM_LBUTTONDOWN, wParam, con.ISel.LClickPt.x, con.ISel.LClickPt.y);
						goto wrap;
					}
				}
				else if ((messg == WM_LBUTTONDBLCLK) && (con.ISel.State == IS_LBtnReleased))
				{
					if ((_abs(nXdiff) < nMinDiff) && (_abs(nYdiff) < nMinDiff))
					{
						// Simulate started stream selection
						if (!(con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION)))
						{
							DEBUGSTRSEL(L"Selection: Simulating CONSOLE_TEXT_SELECTION started");
							SetSelectionFlags(CONSOLE_TEXT_SELECTION | CONSOLE_MOUSE_SELECTION);
						}

						// And expand it to word boundary
						DEBUGSTRSEL(L"Selection: Trigger WM_LBUTTONDBLCLK on IntelliSel");
						OnMouseSelection(WM_LBUTTONDBLCLK, wParam, x, y);

						// To be sure Triple-Click would be OK
						if (con.m_sel.dwFlags)
							SetSelectionFlags(con.m_sel.dwFlags | CONSOLE_DBLCLICK_SELECTION);

						DEBUGSTRSEL(L"Selection: set IS_None (auto)");
						con.ISel.State = IS_None;

						goto wrap;
					}
				}
			}
		}
	}
	else if (con.ISel.State)
	{
		if ((GetTickCount() - con.ISel.ClickTick) <= GetDoubleClickTime())
		{
			DEBUGSTRSEL(L"Selection: set IS_None (timeout)");
			con.ISel.State = IS_None;
		}
	}

	if (bSelAllowed)
	{
		const bool tripleClick = (messg == WM_LBUTTONDOWN) && (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION)
			&& ((GetTickCount() - con.m_SelDblClickTick) <= GetDoubleClickTime());
		// Click outside selection region - would reset active selection
		if ((((messg == WM_LBUTTONDOWN) && !tripleClick)
			|| ((messg == WM_LBUTTONUP) && !(con.m_sel.dwFlags & (CONSOLE_MOUSE_DOWN | CONSOLE_DBLCLICK_SELECTION | CONSOLE_TRIPLE_CLICK_SELECTION))))
			&& gpSet->isCTSIntelligent // Only intelligent mode?
			&& isSelectionPresent()
			&& !isMouseClickExtension()
			)
		{
			const bool bInside = isMouseInsideSelection(x, y);
			if (!bInside)
			{
				DEBUGSTRSEL(L"Selection: DoSelectionFinalize#L");
				DoSelectionFinalize(false);
			}

			if (messg == WM_LBUTTONDOWN)
			{
				DEBUGSTRSEL(L"Selection: set IS_LBtnDown on WM_LBUTTONDOWN after reset");
				con.ISel.LClickPt = MakePoint(x,y);
				con.ISel.ClickTick = GetTickCount();
				con.ISel.State = IS_LBtnDown;
			}

			// Skip LBtnUp
			if ((messg == WM_LBUTTONUP)
				// but allow LBtnDown if it's inside selection (double and triple clicks)
				|| (!bInside && (messg == WM_LBUTTONDOWN) && !nModifierNoEmptyPressed))  // -V560
			{
				// Anyway, clicks would be ignored
				#ifdef _DEBUG
				swprintf_c(szDbgInfo, L"Selection: %s %s ignored", GetMouseMsgName(messg), bInside ? L"Inside" : L"Outside");
				DEBUGSTRSEL(szDbgInfo);
				#endif
				con.mn_SkipNextMouseEvent = (messg == WM_LBUTTONDOWN) ? WM_LBUTTONUP : 0;
				goto wrap;
			}
		}

		if (messg == WM_LBUTTONDOWN)
		{
			// Selection mode would be started here
			if (OnMouseSelection(WM_LBUTTONDOWN, wParam, x, y))
				goto wrap;
		}

		// Если выделение еще не начато, но удерживается модификатор - игнорировать WM_MOUSEMOVE
		if (messg == WM_MOUSEMOVE && !con.m_sel.dwFlags)
		{
			if (nModifierNoEmptyPressed)
			{
				// Пропустить, пользователь собирается начать выделение, не посылать движение мыши в консоль
				goto wrap;
			}
		}

		if (((gpSet->isCTSRBtnAction == MouseButtonAction::Paste) || ((gpSet->isCTSRBtnAction == MouseButtonAction::Auto) && !isSelectionPresent()))
				&& (messg == WM_RBUTTONDOWN || messg == WM_RBUTTONUP))
		{
			const bool bAllowAutoPaste = !mp_RCon->isFar() && !mp_RCon->m_Term.nMouseMode;
			if (gpSet->IsModifierPressed(vkCTSVkAct, bAllowAutoPaste))
			{
				if (messg == WM_RBUTTONUP)
				{
					// If Paste was requested - no need to use Windows clipboard,
					// and if selection exists copy it first and paste internally
					if (isSelectionPresent() && gpSet->isCTSIntelligent)
					{
						DEBUGSTRSEL(L"Selection: DoCopyPaste#R");
						DoCopyPaste(true, true);
					}
					else
					{
						// Paste is useless in "Alternative mode" or while selection is present...
						DEBUGSTRSEL(L"Selection: DoSelectionFinalize#R");
						DoSelectionFinalize(false);
						// And Paste itself
						DEBUGSTRSEL(L"Selection: Paste#R");
						mp_RCon->Paste();
					}
				}

				goto wrap;
			}
		}

		if (((gpSet->isCTSMBtnAction == MouseButtonAction::Paste) || ((gpSet->isCTSMBtnAction == MouseButtonAction::Auto) && !isSelectionPresent()))
				&& (messg == WM_MBUTTONDOWN || messg == WM_MBUTTONUP))
		{
			if (gpSet->IsModifierPressed(vkCTSVkAct, !mp_RCon->isFar()))
			{
				if (messg == WM_MBUTTONUP)
				{
					DEBUGSTRSEL(L"Selection: Paste#M");
					mp_RCon->Paste();
				}

				goto wrap;
			}
		}
	}

	if ((gpSet->isCTSMBtnAction == MouseButtonAction::Menu) || (gpSet->isCTSRBtnAction == MouseButtonAction::Menu))
	{
		if (((messg == WM_MBUTTONDOWN || messg == WM_MBUTTONDBLCLK) && (gpSet->isCTSMBtnAction == MouseButtonAction::Menu))
			|| ((messg == WM_RBUTTONDOWN || messg == WM_RBUTTONDBLCLK) && (gpSet->isCTSRBtnAction == MouseButtonAction::Menu)))
		{
			goto wrap; // show menu on Up
		}
		if ((messg == WM_MBUTTONUP && gpSet->isCTSMBtnAction == MouseButtonAction::Menu)
			|| (messg == WM_RBUTTONUP && gpSet->isCTSRBtnAction == MouseButtonAction::Menu))
		{
			POINT ptCur = {-32000,-32000};
			GetCursorPos(&ptCur);
			LogString(L"EditMenu called by mouse click");
			gpConEmu->mp_Menu->ShowEditMenu(mp_RCon->VCon(), ptCur);
			goto wrap;
		}
	}

	// Поиск и подсветка файлов с ошибками типа
	// .\realconsole.cpp(8104) : error ...
	if ((con.m_sel.dwFlags == 0) && mp_RCon->IsFarHyperlinkAllowed(false))
	{
		if (messg == WM_MOUSEMOVE || messg == WM_LBUTTONDOWN || messg == WM_LBUTTONUP || messg == WM_LBUTTONDBLCLK)
		{
			if (ProcessFarHyperlink(messg, crMouse, true))
			{
				// Пускать или нет событие мыши в консоль?
				// Лучше наверное не пускать, а то вьювер может заклинить на прокрутке, например
				goto wrap;
			}
		}
	}

	// Init variables
	lbFarBufferSupported = mp_RCon->isFarBufferSupported();
	lbMouseSendAllowed = !gpSet->isDisableMouse && mp_RCon->isSendMouseAllowed();
	lbMouseOverScroll = false;

	// Проверять мышку имеет смысл только если она пересылается в фар, а не работает на прокрутку
	if ((messg == WM_MOUSEWHEEL) || (messg == WM_MOUSEHWHEEL))
	{
		if (con.bBufferHeight && (m_Type == rbt_Primary) && lbFarBufferSupported && lbMouseSendAllowed)
		{
			lbMouseOverScroll = mp_RCon->mp_VCon->CheckMouseOverScroll();
		}
	}

	if (con.bBufferHeight
		&& ((m_Type != rbt_Primary) || !lbFarBufferSupported || !lbMouseSendAllowed || lbMouseOverScroll
			|| (((messg == WM_MOUSEWHEEL) || (messg == WM_MOUSEHWHEEL)) && isSelectionPresent())
			))
	{
		if ((messg == WM_MOUSEWHEEL) || (messg == WM_MOUSEHWHEEL))
		{
			_ASSERTE(FALSE && "Must be processed in CRealConsole::OnScroll");
			goto wrap; // обработано
		}

		if (!isConSelectMode())
		{
			// Allow posting mouse event to console if this is NOT a Far.exe
			lbSkip = (m_Type != rbt_Primary);
			goto wrap;
		}
	}

	//if (isConSelectMode()) -- это неправильно. она реагирует и на фаровский граббер (чтобы D&D не взлетал)
	if (con.m_sel.dwFlags != 0)
	{
		// Ручная обработка выделения, на консоль полагаться не следует...
		#ifdef _DEBUG
		if (messg != WM_MOUSEMOVE)
		{
			DEBUGSTRSEL(L"Seletion: (con.m_sel.dwFlags != 0)");
		}
		#endif
		OnMouseSelection(messg, wParam, x, y);
		goto wrap;
	}

	// При правом клике на KeyBar'е - показать PopupMenu с вариантами модификаторов F-клавиш
	TODO("Пока только для Far Manager?");
	if ((m_Type == rbt_Primary) && (gpSet->isKeyBarRClick)
		&& ((messg == WM_RBUTTONDOWN && (crMouse.Y == (GetBufferHeight() - 1)) && mp_RCon->isFarKeyBarShown())
			|| ((messg == WM_MOUSEMOVE || messg == WM_RBUTTONUP) && con.bRClick4KeyBar)))
	{
		if (messg == WM_RBUTTONDOWN)
		{
			MSectionLock csData;
			const wchar_t* pChar = nullptr;
			int nLen = 0;
			COORD lcrScr = BufferToScreen(crMouse);
			CRConDataGuard data;
			if (GetConsoleLine(data, lcrScr.Y, pChar, nLen, &csData) && (*pChar == L'1'))
			{
				// Т.к. ширина баров переменная, ищем
				int x, k, px = 0, vk = 0;
				for (x = 1, k = 2; x < nLen; x++)
				{
					if (pChar[x] < L'0' || pChar[x] > L'9')
						continue;
					if (k <= 9)
					{
						if ((((int)pChar[x] - L'0') == k) && (pChar[x-1] == L' ')
							&& (pChar[x+1] < L'0' || pChar[x+1] > L'9'))
						{
							if ((lcrScr.X <= (x - 1)) && (lcrScr.X >= px))
							{
								vk = VK_F1 + (k - 2);
								break;
							}
							px = x - 1;
							k++;
						}
					}
					else if (k <= 12)
					{
						if ((pChar[x] == L'1') && (((int)pChar[x+1] - L'0') == (k-10)) && (pChar[x-1] == L' ')
							&& (pChar[x+2] < L'0' || pChar[x+2] > L'9'))
						{
							if ((lcrScr.X <= (x - 1)) && (lcrScr.X >= px))
							{
								px++;
								vk = VK_F1 + (k - 2);
								break;
							}
							px = x - 1;
							k++;
						}
					}
					else
					{
						px++;
						vk = VK_F12;
						break;
					}
				}

				if (vk)
				{
					con.bRClick4KeyBar = TRUE;
					con.crRClick4KeyBar = lcrScr;
					con.ptRClick4KeyBar = mp_RCon->mp_VCon->ConsoleToClient((vk==VK_F1)?(px+1):(px+2), lcrScr.Y);
					ClientToScreen(mp_RCon->GetView(), &con.ptRClick4KeyBar);
					con.nRClickVK = vk;
					goto wrap;
				}
			}
		}
		else if (messg == WM_RBUTTONUP)
		{
			_ASSERTE(con.bRClick4KeyBar);
			//Run!
			HMENU h = CreatePopupMenu();
			wchar_t szText[128];
			for (size_t i = 0; i < countof(gnKeyBarFlags); i++)
			{
				*szText = 0;
				if (gnKeyBarFlags[i] & LEFT_CTRL_PRESSED)
					wcscat_c(szText, L"Ctrl-");
				if (gnKeyBarFlags[i] & LEFT_ALT_PRESSED)
					wcscat_c(szText, L"Alt-");
				if (gnKeyBarFlags[i] & RIGHT_CTRL_PRESSED)
					wcscat_c(szText, L"RCtrl-");
				if (gnKeyBarFlags[i] & RIGHT_ALT_PRESSED)
					wcscat_c(szText, L"RAlt-");
				if (gnKeyBarFlags[i] & SHIFT_PRESSED)
					wcscat_c(szText, L"Shift-");
				swprintf_c(szText+lstrlen(szText), 8/*#SECURELEN*/, L"F%i", (con.nRClickVK - VK_F1 + 1));

				AppendMenu(h, MF_STRING|((!(i % 4)) ? MF_MENUBREAK : 0), i+1, szText);
			}

			RECT rcExcl = {con.ptRClick4KeyBar.x-1,con.ptRClick4KeyBar.y-1,con.ptRClick4KeyBar.x+1,con.ptRClick4KeyBar.y+1};

			int i = gpConEmu->mp_Menu->trackPopupMenu(tmp_KeyBar, h, TPM_LEFTALIGN|TPM_BOTTOMALIGN|/*TPM_NONOTIFY|*/TPM_RETURNCMD|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,
						con.ptRClick4KeyBar.x, con.ptRClick4KeyBar.y, ghWnd, &rcExcl);
			DestroyMenu(h);

			if ((i > 0) && (i <= (int)countof(gnKeyBarFlags)))
			{
				i--;
				mp_RCon->PostKeyPress(con.nRClickVK, gnKeyBarFlags[i], 0);
			}

			//Done
			con.bRClick4KeyBar = FALSE;
			goto wrap;
		}
		else if (messg == WM_MOUSEMOVE)
		{
			_ASSERTE(con.bRClick4KeyBar);
			TODO("«Отпустить» если был сдвиг?");
			goto wrap; // не пропускать в консоль
		}
	}
	else if (con.bRClick4KeyBar)
	{
		con.bRClick4KeyBar = FALSE;
	}

	// Don't bypass to Far Menus, History etc. when Alt/Shift are pressed.
	if (nModifierPressed && (messg == WM_MOUSEMOVE))
	{
		lbSkip = true;
		goto wrap;
	}

	// Allow posting mouse event to console only for rbt_Primary buffer
	lbSkip = (m_Type != rbt_Primary);
wrap:
	if (messg == con.mn_SkipNextMouseEvent)
	{
		con.mn_SkipNextMouseEvent = 0;
		#ifdef _DEBUG
		swprintf_c(szDbgInfo, L"Selection: %s is skipped", GetMouseMsgName(messg));
		DEBUGSTRSEL(szDbgInfo);
		#endif
		lbSkip = true;
	}
	return lbSkip;
}

bool CRealBuffer::GetRBtnDrag(COORD* pcrMouse) const
{
	if (pcrMouse)
		*pcrMouse = con.crRBtnDrag;
	return con.bRBtnDrag;
}

void CRealBuffer::SetRBtnDrag(bool abRBtnDrag, const COORD* pcrMouse)
{
	con.bRBtnDrag = abRBtnDrag;

	if (pcrMouse)
		con.crRBtnDrag = *pcrMouse;
}

void CRealBuffer::OnMouseSelectionStarted()
{
	con.m_sel.dwFlags |= CONSOLE_MOUSE_DOWN;
	con.m_SelScrollBurst = 0;
	gpConEmu->SetKillTimer(true, TIMER_SELECTION_ID, TIMER_SELECTION_ELAPSE);
}

void CRealBuffer::OnMouseSelectionStopped()
{
	con.m_sel.dwFlags &= ~CONSOLE_MOUSE_DOWN;
	con.m_SelScrollBurst = 0;
	gpConEmu->SetKillTimer(false, TIMER_SELECTION_ID, 0);
}

uint32_t CRealBuffer::GetAutoscrollSelectionLines(const int yDelta)
{
	uint32_t result = 1;
	const uint32_t maxScrollLines = std::min<uint32_t>(20, GetTextHeight() / 4);
	const auto cellSize = mp_RCon->VCon()->GetCellSize();

	if (yDelta >= 0)
	{
		const auto curTick = GetTickCount();
		if (!con.m_SelScrollBurst)
			con.m_SelScrollBurst = curTick;
		const auto durationSec = (curTick - con.m_SelScrollBurst) / 1000;
		if (durationSec > 0)
			result += static_cast<uint32_t>(std::log2(durationSec));
	}
	else
	{
		// if user returns mouse cursor to work area - reset time acceleration
		con.m_SelScrollBurst = 0;
	}

	if (yDelta > cellSize.cy && cellSize.cy > 0)
	{
		const auto yResult = std::min<uint32_t>(maxScrollLines, yDelta / cellSize.cy);
		if (yResult > result)
			result = yResult;
	}
	
	return result;
}

void CRealBuffer::SetSelectionFlags(const DWORD flags)
{
	if (flags != con.m_sel.dwFlags)
	{
		if ((con.m_sel.dwFlags & CONSOLE_MOUSE_DOWN) != (flags & CONSOLE_MOUSE_DOWN))
		{
			if (flags & CONSOLE_MOUSE_DOWN)
				OnMouseSelectionStarted();
			else
				OnMouseSelectionStopped();
		}

		if (!(con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION) && (flags & CONSOLE_DBLCLICK_SELECTION))
		{
			con.m_SelDblClickRightAnchor = COORD{ con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom };
		}

		con.m_sel.dwFlags = flags;
	}
}

void CRealBuffer::SetSelectionAnchor(int x, int y)
{
	const auto cr = MakeCoord(x, y);
	if (con.m_sel.dwSelectionAnchor != cr)
	{
		con.m_sel.dwSelectionAnchor = cr;
	}
}

// x,y - экранные координаты
bool CRealBuffer::OnMouseSelection(UINT messg, WPARAM wParam, int x, int y)
{
	if (TextWidth()<=1 || TextHeight()<=1)
	{
		_ASSERTE(TextWidth()>1 && TextHeight()>1);
		return false;
	}

	const bool wasSelection = isSelectionPresent();

	// Получить известные координаты символов
	COORD crScreen = mp_RCon->mp_VCon->ClientToConsole(x,y);
	MinMax(crScreen.X, 0, TextWidth()-1);
	MinMax(crScreen.Y, 0, TextHeight()-1);

	COORD cr = ScreenToBuffer(crScreen);

	// Shift+LClick to mark selection start (StartSelection)
	// and use Shift+LClick to ExpandSelection
	const bool extendSelection = (messg == WM_LBUTTONDOWN || messg == WM_LBUTTONUP) && isMouseClickExtension();

	if ((messg == WM_LBUTTONDOWN && !extendSelection)
		|| ((messg == WM_LBUTTONDBLCLK) && (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION)) && (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION))
		)
	{
		bool lbStreamSelection = false;
		BYTE vk = 0; // If selection was started with "modifier" we need to "release" it in the console
		const bool tripleClick = (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION) && ((GetTickCount() - con.m_SelDblClickTick) <= GetDoubleClickTime());

		if (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION))
		{
			// Selection was started from the Menu
			lbStreamSelection = (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION)) == CONSOLE_TEXT_SELECTION;
		}
		else
		{
			const DWORD nPressed = gpConEmu->isSelectionModifierPressed(true);
			if (nPressed)
			{
				// OK
				lbStreamSelection = ((nPressed & CONSOLE_TEXT_SELECTION) != 0);
				vk = LOBYTE(gpSet->GetHotkeyById(lbStreamSelection ? vkCTSVkText : vkCTSVkBlock));
			}
			else
			{
				return false; // modifier is not allowed
			}
		}

		SetSelectionFlags((con.m_sel.dwFlags & ~CONSOLE_KEYMOD_MASK) | (static_cast<DWORD>(vk) << 24));

		COORD crTo = cr;
		if (tripleClick)
		{
			cr.X = 0;
			crTo.X = GetBufferWidth() - 1;
			CRConDataGuard data;
			if (GetData(data))
			{
				ConsoleLinePtr prev{};
				ConsoleLinePtr next{};
				const AppSettings* pApp = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());
				const bool bBash = pApp ? pApp->CTSBashMargin() : false;
				const bool bTrimTrailingSpaces = pApp ? pApp->CTSTrimTrailing() : true;
				auto isOneLine = [&prev, &next, this, bBash]()
				{
					// some heuristics - no more than one space is allowed at the beginning of the next line
					if (next.pChar[0] == L' ' && next.pChar[1] == L' ')
						return false;
					// If right or left edge of screen is "Frame" - force to line break!
					if (IsForceLineBreakChar(prev.pChar[prev.nLen - 1]) || IsForceLineBreakChar(next.pChar[0]))
						return false;
					// no more than one-two spaces at the end of current line
					if (prev.pChar[prev.nLen - 1] == L' '
						&& ((prev.pChar[prev.nLen - 2] == L' ')
							|| (bBash && (prev.pChar[prev.nLen - 2] == L' ' && prev.pChar[prev.nLen - 3] == L' '))))
						return false;
					return true;
				};

				// #TODO check with alternative buffer (full backscroll)
				const int shiftY = data->m_sbi.srWindow.Top;
				// go down
				if (data->GetConsoleLine(crTo.Y - shiftY, prev) && prev.nLen > 4)
				{
					for (int checkY = static_cast<int>(crTo.Y); checkY < static_cast<int>(con.m_sbi.srWindow.Bottom);)
					{
						if (!data->GetConsoleLine((++checkY) - shiftY, next) || next.nLen <= 4)
							break;
						if (!isOneLine())
							break;
						// Ok, let's expand selection
						crTo.Y = static_cast<SHORT>(checkY);
						prev = next;
					}
				}
				// go up
				if (data->GetConsoleLine(cr.Y - shiftY, next) && next.nLen > 4)
				{
					for (int checkY = static_cast<int>(cr.Y); checkY > static_cast<int>(con.m_sbi.srWindow.Top);)
					{
						if (!data->GetConsoleLine((--checkY) - shiftY, prev) || prev.nLen <= 4)
							break;
						if (!isOneLine())
							break;
						// Ok, let's expand selection
						cr.Y = static_cast<SHORT>(checkY);
						next = prev;
					}
				}
				// Trip trailing spaces
				if (bTrimTrailingSpaces && data->GetConsoleLine(crTo.Y - shiftY, next) && next.nLen > 2)
				{
					int lastPos = std::max<int>(0, std::min<int>(next.nLen - 1, crTo.X));
					while (lastPos > 0 && next.pChar[lastPos] == L' ')
					{
						--lastPos;
					}
					crTo.X = static_cast<SHORT>(lastPos);
				}
			} // if (GetData(data))
			// unexpected selection change on LBtnUp
			SetSelectionFlags(con.m_sel.dwFlags | CONSOLE_TRIPLE_CLICK_SELECTION);
		} // if (tripleClick)

		#ifdef _DEBUG
		wchar_t szLog[200]; swprintf_c(szLog, L"Selection: %s %s",
			(messg == WM_LBUTTONDOWN) ? L"WM_LBUTTONDOWN" : L"WM_LBUTTONDBLCLK",
			tripleClick ? L"bTripleClick" : L"");
		DEBUGSTRSEL(szLog);
		#endif

		// Если дошли сюда - значит или модификатор нажат, или из меню выделение запустили
		StartSelection(lbStreamSelection, cr.X, cr.Y, TRUE, tripleClick ? WM_LBUTTONDBLCLK : WM_LBUTTONDOWN, tripleClick ? &crTo : nullptr);

		//WARNING!!! После StartSelection - ничего не делать! Мог смениться буфер!

		return true;
	}
	else if ((messg == WM_LBUTTONDBLCLK) && (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION)))
	{
		// Выделить слово под курсором (как в обычной консоли)
		const bool streamSelection = (con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) == CONSOLE_TEXT_SELECTION;

		DEBUGSTRSEL(L"Selection: WM_LBUTTONDBLCLK - expanding etr_Word");

		// Нужно получить координаты слова
		COORD crFrom{}, crTo{};
		ExpandTextRange(cr, crFrom/*[Out]*/, crTo/*[Out]*/, etr_Word);

		StartSelection(streamSelection, crFrom.X, crFrom.Y, true, WM_LBUTTONDBLCLK, &crTo);

		// mouse button is still pressed

		// WARNING!!! After StartSelection the active buffer could change, stop processing.
		return true;
	}
	else if (
		((messg == WM_MOUSEMOVE) && (con.m_sel.dwFlags & CONSOLE_MOUSE_DOWN))
		|| ((messg == WM_LBUTTONUP) && ((con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION) || extendSelection))
		)
	{
		// При LBtnUp может быть несколько вариантов
		// 1. Cick. После WM_LBUTTONUP нужно дождаться таймера и позвать DoSelectionCopy.
		// 2. DblClick. Генерятся WM_LBUTTONUP WM_LBUTTONDBLCLK. Выделение "Слова".
		// 3. TripleCLick. Генерятся WM_LBUTTONUP WM_LBUTTONDBLCLK WM_LBUTTONDOWN WM_LBUTTONUP. Выделение "Строки".

		//TODO("Горизонтальная прокрутка?");
		//// Если мыша за пределами окна консоли - скорректировать координаты (MinMax)
		//if (cr.X<0 || cr.X>=(int)TextWidth())
		//	cr.X = GetMinMax(cr.X, 0, TextWidth());
		//if (cr.Y<0 || cr.Y>=(int)TextHeight())
		//	cr.Y = GetMinMax(cr.Y, 0, TextHeight());

		if ((messg == WM_MOUSEMOVE) && (con.m_sel.dwFlags & CONSOLE_MOUSE_DOWN) && (con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION))
		{
			// If we hasn't received the WM_LBUTTONUP event somehow, emulate it now
			if (!isPressed(VK_LBUTTON))
				messg = WM_LBUTTONUP;
		}

		if ((messg == WM_LBUTTONUP) && !(con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION) && extendSelection)
		{
			con.m_SelClickTick = GetTickCount();
		}

		// Теперь проверки Double/Triple.
		if ((messg == WM_LBUTTONUP)
			&& ((((con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION)
					&& ((GetTickCount() - con.m_SelClickTick) <= GetDoubleClickTime()))
				|| ((con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION)
					&& ((GetTickCount() - con.m_SelDblClickTick) <= GetDoubleClickTime()))
				|| ((con.m_sel.dwFlags & CONSOLE_TRIPLE_CLICK_SELECTION))
				))
			)
			//&& ((messg == WM_LBUTTONUP)
			//	|| ((messg == WM_MOUSEMOVE)
			//		&& (memcmp(&cr,&con.m_sel.srSelection.Left,sizeof(cr)) || memcmp(&cr,&con.m_sel.srSelection.Right,sizeof(cr)))
			//		)
			//	)
			//)
		{
			// Ignoring due DoubleClickTime
			std::ignore = 0;
		}
		else
		{
			ExpandSelection(cr.X, cr.Y, wasSelection);
		}

		if (messg == WM_LBUTTONUP)
		{
			OnMouseSelectionStopped();

			if (gpSet->isCTSAutoCopy)
			{
				//if ((con.m_sel.srSelection.Left != con.m_sel.srSelection.Right) || (con.m_sel.srSelection.Top != con.m_sel.srSelection.Bottom))
				const DWORD nPrevTick = (con.m_sel.dwFlags & (CONSOLE_DBLCLICK_SELECTION | CONSOLE_TRIPLE_CLICK_SELECTION))
					? con.m_SelDblClickTick
					: con.m_SelClickTick;
				if ((GetTickCount() - nPrevTick) > GetDoubleClickTime())
				{
					// If duration of dragging/marking with mouse key pressed
					// exceeds DblClickTime we may (and must) do copy immediately
					_ASSERTE(nPrevTick != 0);
					_ASSERTE(gpSet->isCTSAutoCopy && mp_RCon && mp_RCon->isSelectionPresent());
					mp_RCon->AutoCopyTimer();
				}
				else
				{
					// Иначе - таймер, чтобы не перекрыть возможность DblClick/TripleClick
					// Скорректировать отсечку таймера на отпускание
					mp_RCon->VCon()->SetAutoCopyTimer(true);
				}
			}
		}

		return true;
	}
	else if ((messg == WM_RBUTTONUP || messg == WM_MBUTTONUP) && (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION)))
	{
		const auto bAction = (messg == WM_RBUTTONUP) ? gpSet->isCTSRBtnAction : gpSet->isCTSMBtnAction; // enum: 0-off, 1-copy, 2-paste, 3-auto

		// On mouse selection, when LBtn is still down, and RBtn is clicked - Do "Internal Copy & Paste"

		const bool bDoCopyWin = (bAction == MouseButtonAction::Copy);
		bool bDoPaste = false;

		if (bAction == MouseButtonAction::Auto)
		{
			if (// LBtn is pressed now
				((con.m_sel.dwFlags & CONSOLE_MOUSE_DOWN))
				// If LBtn was released, but RBtn was pressed **over** selection
				// That allows DblClick on file and RClick on it to paste selection into CmdLine
				|| (gpSet->isCTSIntelligent && isMouseInsideSelection(x, y))
				)
			{
				bDoPaste = true;
			}
		}


		if (!bDoPaste)
		{
			// While "Paste" was not requested - that means "Copy to Windows clipboard"
			const bool bDoCopy = bDoCopyWin || (bAction == MouseButtonAction::Auto);
			DoSelectionFinalize(bDoCopy);
		}
		else
		{
			// If Paste was requested - no need to use Windows clipboard, copy/paste internally
			DoCopyPaste(true, true);
		}

		return true;
	}

	return false;
}

bool CRealBuffer::IsForceLineBreakChar(const wchar_t c) const
{
	static wchar_t sPreLineBreak[] =
	{
		ucBox25,ucBox50,ucBox75,ucBox100,ucUpScroll,ucDnScroll,ucLeftScroll,ucRightScroll,ucArrowUp,ucArrowDown,
		//ucNoBreakSpace, -- this is space, why it was blocked?
		ucBoxDblVert,ucBoxSinglVert,ucBoxDblDownRight,ucBoxDblDownLeft,ucBoxDblUpRight,ucBoxDblUpLeft,ucBoxSinglDownRight,
		ucBoxSinglDownLeft,ucBoxSinglUpRight,ucBoxSinglUpLeft,ucBoxSinglDownDblHorz,ucBoxSinglUpDblHorz,ucBoxDblDownDblHorz,
		ucBoxDblUpDblHorz,ucBoxSinglDownHorz,ucBoxSinglUpHorz,ucBoxDblDownSinglHorz,ucBoxDblUpSinglHorz,ucBoxDblVertRight,
		ucBoxDblVertLeft,ucBoxDblVertSinglRight,ucBoxDblVertSinglLeft,ucBoxSinglVertRight,ucBoxSinglVertLeft,
		ucBoxDblHorz,ucBoxSinglHorz,ucBoxDblVertHorz,
		// End
		0 /*ASCIIZ!!!*/
	};
	const auto* found = wcschr(sPreLineBreak, c);
	return (found != nullptr);
}

void CRealBuffer::DoCopyPaste(bool abCopy, bool abPaste)
{
	const bool bDoCopyWin = abCopy && !abPaste;
	bool bDoPaste = abPaste;
	const bool bClipOpen = bDoCopyWin ? MyOpenClipboard(L"Copy&Paste") : false;
	HGLOBAL hUnicode = nullptr;
	bool bCopyOk = DoSelectionFinalize(abCopy, bDoCopyWin ? cm_CopySel : cm_CopyInt, 0, abPaste ? &hUnicode : nullptr);

	if (bCopyOk && bDoPaste)
	{
		LPCWSTR pszText = nullptr;
		if (hUnicode)
		{
			pszText = (LPCWSTR)GlobalLock(hUnicode);
			if (!pszText)
			{
				DisplayLastError(L"GlobalLock failed, paste is impossible!");
				bDoPaste = false;
			}
		}

		// Immediately paste into console ('Auto' mode)?
		if (bDoPaste)
		{
			mp_RCon->Paste(pm_OneLine, pszText);
		}

		if (hUnicode)
		{
			if (pszText)
				GlobalUnlock(hUnicode);
			GlobalFree(hUnicode);
		}
	}

	if (bClipOpen)
	{
		MyCloseClipboard();
	}  // -V1020
}

void CRealBuffer::MarkFindText(int nDirection, LPCWSTR asText, bool abCaseSensitive, bool abWholeWords)
{
	bool bFound = false;
	COORD crStart = {}, crEnd = {};
	LPCWSTR pszFrom = nullptr, pszDataStart = nullptr, pszEnd = nullptr;
	LPCWSTR pszFrom1 = nullptr, pszEnd1 = nullptr;
	size_t nWidth = 0, nHeight = 0;
	CRConDataGuard data;

	if (m_Type == rbt_Primary)
	{
		nWidth = this->GetTextWidth();
		nHeight = this->GetTextHeight();
		if (!GetData(data) || !data.isValid(nWidth*nHeight))
			goto done;
		pszDataStart = pszFrom = pszFrom1 = data->pConChar;
		_ASSERTE(pszFrom[nWidth*nHeight] == 0); // Должно быть ASCIIZ
		pszEnd = pszEnd1 = pszFrom + (nWidth * nHeight);
	}
	else if (dump.pszBlock1)
	{
		//WARNING("Доработать для режима с прокруткой");
		nWidth = dump.crSize.X;
		nHeight = dump.crSize.Y;
		_ASSERTE(dump.pszBlock1[nWidth*nHeight] == 0); // Должно быть ASCIIZ

		pszDataStart = dump.pszBlock1;
		pszEnd = pszDataStart + (nWidth * nHeight);

		pszFrom = pszFrom1 = dump.pszBlock1 + con.m_sbi.srWindow.Top * nWidth;

		// Search in whole buffer
		//nHeight = std::min((dump.crSize.Y-con.m_sbi.srWindow.Top),(con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1));
		// But remember visible area
		pszEnd1 = pszFrom + (nWidth * std::min((dump.crSize.Y-con.m_sbi.srWindow.Top),(con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1)));
	}

	WARNING("TODO: bFindNext");

	if (pszFrom && asText && *asText && nWidth && nHeight)
	{
		int nFindLen = lstrlen(asText);
		const wchar_t* szWordDelim = L"~!%^&*()+|{}:\"<>?`-=\\[];',./";
		LPCWSTR pszFound = nullptr;
		int nStepMax = 0;
		INT_PTR nFrom = -1;

		if (nDirection > 1)
			nDirection = 1;
		else if (nDirection < -1)
			nDirection = -1;

		if (isSelectionPresent())
		{
			COORD crSelScrn = BufferToScreen(MakeCoord(con.m_sel.srSelection.Left, con.m_sel.srSelection.Top));
			nFrom = crSelScrn.X + (crSelScrn.Y * nWidth);
			_ASSERTE(nFrom>=0);

			if (nDirection >= 0)
			{
				if ((nFrom + nDirection) >= (INT_PTR)(nWidth * nHeight))
					goto done; // считаем, что не нашли
				pszFrom += (nFrom + nDirection);
			}
			else if (nDirection < 0)
			{
				pszEnd = pszFrom + nFrom;
			}
			nStepMax = ((m_Type == rbt_Primary) || (nDirection >= 0)) ? 1 : 2;
		}
		else if (m_Type == rbt_Find)
		{
			nStepMax = (nDirection >= 0) ? 1 : 2;
		}

		for (int i = 0; i <= nStepMax; i++)
		{
			while (pszFrom && (pszFrom < pszEnd) && *pszFrom)
			{
				if (abCaseSensitive)
					pszFrom = StrStr(pszFrom, asText);
				else
					pszFrom = StrStrI(pszFrom, asText);

				if (pszFrom)
				{
					if (abWholeWords)
					{
						#define isWordDelim(ch) (!ch || (wcschr(szWordDelim,ch)!=nullptr) || (ch>=0x2100 && ch<0x2800) || (ch<=32))
						if (pszFrom > pszDataStart)
						{
							if (!isWordDelim(*(pszFrom-1)))
							{
								pszFrom++;
								continue;
							}
						}
						if (!isWordDelim(pszFrom[nFindLen]))
						{
							pszFrom++;
							continue;
						}
					}

					if (nDirection < 0)
					{
						if (pszFrom < pszEnd)
						{
							pszFound = pszFrom;
							pszFrom++;
							bFound = true;
							continue;
						}
						else
						{
							pszFrom = nullptr;
						}
					}
					bFound = true;
					break; // OK, подходит
				}
			} // end of `while (pszFrom && (pszFrom < pszEnd) && *pszFrom)`

			if ((nDirection < 0) && bFound && pszFound)
			{
				pszFrom = pszFound;
			}
			if (pszFrom && bFound)
				break;

			if (!nStepMax)
				break;


			pszFrom = pszDataStart;
			pszEnd = pszDataStart + (nWidth * nHeight);
			if ((nDirection < 0) && (nFrom >= 0))
			{
				if (i == 0)
				{
					pszEnd = pszFrom1 + nFrom;
				}
				else
				{
					pszFrom = pszFrom1 + nFrom + 1;
				}
			}
			else if (nDirection == 0)
			{
				// First 'auto' seek
				pszEnd = pszFrom1;
				nDirection = -1;
			}
		} // end of `for (int i = 0; i <= nStepMax; i++)`


		if (pszFrom && bFound)
		{
			// Нашли
			size_t nCharIdx = (pszFrom - pszDataStart);

			// А пока - возможная коррекция видимой области, если нашли "за пределами"
			if (m_Type != rbt_Primary)
			{
				int nRows = con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top; // Для удобства

				if (pszFrom < pszFrom1)
				{
					// Прокрутить буфер вверх
					con.m_sbi.srWindow.Top = std::max(0,((int)(nCharIdx / nWidth))-1);
					con.m_sbi.srWindow.Bottom = std::min((con.m_sbi.srWindow.Top + nRows), con.m_sbi.dwSize.Y-1);
					SetTopLeft(con.m_sbi.srWindow.Top, con.TopLeft.x);
				}
				else if (pszFrom >= pszEnd1)
				{
					// Прокрутить буфер вниз
					con.m_sbi.srWindow.Bottom = MakeShort(std::min((nCharIdx / nWidth)+1, size_t(con.m_sbi.dwSize.Y-1)));
					con.m_sbi.srWindow.Top = MakeShort(std::max(0, con.m_sbi.srWindow.Bottom-nRows));
					SetTopLeft(con.m_sbi.srWindow.Top, con.TopLeft.x);
				}

				if (nCharIdx >= (size_t)(con.m_sbi.srWindow.Top*nWidth))
				{
					nCharIdx -= con.m_sbi.srWindow.Top*nWidth;
				}
			}
			else
			{
				// Для rbt_Primary - прокрутки быть не должно (не рассчитано здесь на это)
				_ASSERTE(pszDataStart == pszFrom1);
			}

			WARNING("тут бы на ширину буфера ориентироваться, а не видимой области...");
			if (nCharIdx < (nWidth*nHeight))
			{
				bFound = true;
				crStart.Y = MakeShort(nCharIdx / nWidth);
				crStart.X = MakeShort(nCharIdx - (nWidth * crStart.Y));
				crEnd.Y = MakeShort((nCharIdx + nFindLen - 1) / nWidth);
				crEnd.X = MakeShort((nCharIdx + nFindLen - 1) - (nWidth * crEnd.Y));
			}
		}

	} // if (pszFrom && asText && *asText && nWidth && nHeight)

done:
	if (!bFound)
	{
		DoSelectionStop();
	}
	else
	{
		SetSelectionFlags(CONSOLE_SELECTION_IN_PROGRESS | CONSOLE_TEXT_SELECTION | CONSOLE_LEFT_ANCHOR);
		const COORD crStartBuf = ScreenToBuffer(crStart);
		const COORD crEndBuf = ScreenToBuffer(crEnd);
		SetSelectionAnchor(crStartBuf.X, crStartBuf.Y);
		con.m_sel.srSelection.Left = crStartBuf.X;
		con.m_sel.srSelection.Top = crStartBuf.Y;
		con.m_sel.srSelection.Right = crEndBuf.X;
		con.m_sel.srSelection.Bottom = crEndBuf.Y;
	}

	UpdateSelection();
}

void CRealBuffer::StartSelection(bool abTextMode, SHORT anX /*= -1*/, SHORT anY /*= -1*/, bool abByMouse /*= false*/, UINT anFromMsg /*= 0*/, COORD *pcrTo /*= nullptr*/, DWORD anAnchorFlag /*= 0*/)
{
	_ASSERTE(anY==-1 || anY>=GetBufferPosY());
	_ASSERTE(anAnchorFlag==0 || anAnchorFlag==CONSOLE_LEFT_ANCHOR || anAnchorFlag==CONSOLE_RIGHT_ANCHOR);

	// Если начинается выделение - запретить фару начинать драг, а то подеремся
	if (abByMouse)
	{
		Assert(!(gpConEmu->mouse.state & (DRAG_L_STARTED|DRAG_R_STARTED)));
		gpConEmu->mouse.state &= ~(DRAG_L_STARTED|DRAG_L_ALLOWED|DRAG_R_STARTED|DRAG_R_ALLOWED);
	}

	if (!(con.m_sel.dwFlags & (CONSOLE_BLOCK_SELECTION|CONSOLE_TEXT_SELECTION)) && gpSet->isCTSFreezeBeforeSelect)
	{
		if (m_Type == rbt_Primary)
		{
			if (mp_RCon->LoadAlternativeConsole(lam_FullBuffer) && (mp_RCon->mp_ABuf != this))
			{
				// Сменился буфер (переключились на альтернативную консоль)
				// Поэтому дальнейшие действия - в этом буфере

				DoSelectionStop();

				_ASSERTE(mp_RCon->mp_ABuf->m_Type==rbt_Alternative);
				mp_RCon->mp_ABuf->m_Type = rbt_Selection; // Изменить, чтобы по завершении выделения - буфер закрыть

				mp_RCon->mp_ABuf->StartSelection(abTextMode, anX, anY, abByMouse, anFromMsg, nullptr, anAnchorFlag);
				return;
			}
		}
	}

	mp_RCon->VCon()->SetAutoCopyTimer(false);

	WARNING("Доработать для режима с прокруткой - выделение протяжкой, как в обычной консоли");
	if (anX == -1 && anY == -1)
	{
		// Absolute coordinates now!
		anX = con.m_sbi.dwCursorPosition.X; // - con.m_sbi.srWindow.Left;
		anY = con.m_sbi.dwCursorPosition.Y; // - con.m_sbi.srWindow.Top;
	}

	// Если начало выделение не видимо - ставим ближайший угол
	if (anX < 0)
	{
		_ASSERTE(FALSE && "Started (x) in not visible area?");
		anX = 0;
	}
	else if (anX >= GetBufferWidth())
	{
		_ASSERTE(FALSE && "Started beyond right margin");
		anX = GetBufferWidth()-1;
	}

	if (anY < 0)
	{
		_ASSERTE(FALSE && "Started (y) in not visible area?");
		anY = anX = 0;
	}
	else if (anY >= GetBufferHeight())
	{
		_ASSERTE(FALSE && "Started beyond bottom margin");
		anX = 0;
		anY = GetBufferHeight()-1;
	}

	const COORD cr = {anX,anY};

	if (cr.X < 0 || cr.X >= GetBufferWidth() || cr.Y < 0 || cr.Y >= GetBufferHeight())
	{
		_ASSERTE(cr.X >= 0 && cr.X < GetBufferWidth());
		_ASSERTE(cr.Y >= 0 && cr.Y < GetBufferHeight());
		return; // Ошибка в координатах
	}

	const DWORD vkMod = con.m_sel.dwFlags & CONSOLE_KEYMOD_MASK;
	if (vkMod && !abByMouse)
	{
		DoSelectionStop(); // Чтобы Фар не думал, что все еще нажат модификатор
	}

	SetSelectionFlags(CONSOLE_SELECTION_IN_PROGRESS
		| (abByMouse ? (CONSOLE_MOUSE_SELECTION | CONSOLE_MOUSE_DOWN) : 0)
		| (abTextMode ? CONSOLE_TEXT_SELECTION : CONSOLE_BLOCK_SELECTION)
		| (anAnchorFlag & (CONSOLE_LEFT_ANCHOR | CONSOLE_RIGHT_ANCHOR))
		| (abByMouse ? vkMod : 0));
	SetSelectionAnchor(cr.X, cr.Y);
	con.m_SelDblClickRightAnchor = cr;
	con.m_sel.srSelection.Left = con.m_sel.srSelection.Right = cr.X;
	con.m_sel.srSelection.Top = con.m_sel.srSelection.Bottom = cr.Y;
	_ASSERTE(cr.Y>=GetBufferPosY() && cr.Y<GetBufferHeight());

	con.mn_UpdateSelectionCalled = 0;

	if ((anFromMsg == WM_LBUTTONDBLCLK) || (pcrTo && (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION)))
	{
		if (pcrTo)
			ExpandSelection(pcrTo->X, pcrTo->Y, false);

		SetSelectionFlags(con.m_sel.dwFlags | CONSOLE_DBLCLICK_SELECTION);

		_ASSERTE(anFromMsg == WM_LBUTTONDBLCLK);
		//if (anFromMsg == WM_LBUTTONDBLCLK)
		con.m_SelDblClickTick = GetTickCount();

		if (gpSet->isCTSAutoCopy)
		{
			mp_RCon->VCon()->SetAutoCopyTimer(true);
		}
	}
	else if (abByMouse)
	{
		con.m_SelClickTick = GetTickCount();

		if (gpSet->isCTSAutoCopy)
		{
			mp_RCon->VCon()->SetAutoCopyTimer(true);
		}
	}

	// Refresh on-screen and status
	if (!con.mn_UpdateSelectionCalled)
	{
		UpdateSelection();
	}
}

void CRealBuffer::ChangeSelectionByKey(UINT vkKey, bool bCtrl, bool bShift)
{
	COORD cr; ConsoleCursorPos(&cr);
	COORD crPromptBegin = {};

	// -- координаты нужны абсолютные
	//// Поправить
	//cr.Y -= con.nTopVisibleLine;

	bool bJump = bCtrl;
	short iDiff = 1;

	switch (vkKey)
	{
	case VK_LEFT:
	case VK_RIGHT:
	{
		ExpandTextRangeType etr;
		bool bLeftward = (vkKey == VK_LEFT);
		if (bLeftward)
			cr.X = std::max(0, (cr.X - iDiff));
		else
			cr.X = std::min((GetBufferWidth() - 1), (cr.X + iDiff));
		// If `Ctrl` is pressed - jump `by word`
		if (bJump
			&& ((bLeftward && (cr.X > 1))
				|| (!bLeftward && ((cr.X + 1) < GetBufferWidth()))
				))
		{
			COORD crFrom{}, crTo{};
			// Either by `word`
			if ((etr = ExpandTextRange(cr, crFrom, crTo, etr_Word)) != etr_None)
			{
				COORD& crNew = (bLeftward ? crFrom : crTo);
				if (crNew.X != cr.X)
					cr = crNew;
				else
					etr = etr_None;
			}
			// or by 10 chars (we add/sub 9 more chars)
			if (etr == etr_None)
			{
				if (bLeftward)
					cr.X = std::max(0, (cr.X - 9));
				else
					cr.X = std::min((GetBufferWidth() - 1), (cr.X + 9));
			}
		}
		// Do pos change
		break;
	}
	case VK_UP:
	{
		// Half screen if Ctrl is pressed
		if (bJump)
			iDiff = (GetWindowHeight() >> 1);
		cr.Y = std::max(0, (cr.Y - iDiff));
		break;
	}
	case VK_DOWN:
	{
		// Half screen if Ctrl is pressed
		if (bJump)
			iDiff = (GetWindowHeight() >> 1);
		cr.Y = std::min((GetBufferHeight() - 1), (cr.Y + iDiff));
		break;
	}
	case VK_HOME:
	{
		// Extend to prompt starting position first
		// C:\Users> dir c:\...
		// Sequential keypresses Shift-Home, Shift-Home, Shift-Home
		// must select/unselect prompt part ("C:\Users> ") in read line...
		SHORT X = 0;
		if (mp_RCon->QueryPromptStart(&crPromptBegin)
			&& (crPromptBegin.Y >= 0) && (crPromptBegin.X >= 0) // validation
			&& (cr.Y >= crPromptBegin.Y)) // compare with current row
		{
			if (cr.Y > crPromptBegin.Y)
			{
				X = crPromptBegin.X;
				cr.Y = crPromptBegin.Y;
			}
			else if (cr.X > crPromptBegin.X)
			{
				X = crPromptBegin.X;
			}
			else if (cr.X == 0)
			{
				X = crPromptBegin.X;
			}
		}
		cr.X = X;
		break;
	}
	case VK_END:
	{
		//TODO: Extend to text line ending first
		SHORT X = (GetBufferWidth() - 1);
		const wchar_t* pszLine = nullptr; int nLineLen = 0;
		COORD lcrScr = BufferToScreen(cr);
		// Find last non-spacing character
		MSectionLock csData; csData.Lock(&csCON);
		CRConDataGuard data;
		if (GetConsoleLine(data, lcrScr.Y, pszLine, nLineLen, &csData))
		{
			SHORT newX = X;
			while ((newX > 0) && (pszLine[newX] == 0 || pszLine[newX] == L' ' || pszLine[newX] == L'\t'))
				newX--;
			_ASSERTE(con.m_sel.dwFlags!=0);
			if (newX > cr.X)
			{
				X = newX;
			}
			else if (cr.X == X)
			{
				if (((cr.Y != con.m_sel.srSelection.Top) || ((newX+1) != con.m_sel.srSelection.Left))
					&& ((cr.Y != con.m_sel.srSelection.Bottom) || ((newX+1) != con.m_sel.srSelection.Right)))
				{
					X = newX;
				}
			}
		}
		cr.X = X;
		break;
	}
	}

	// What shall we do?
	bool bResetSel = !bShift;
	if (bResetSel)
	{
		// If `Shift` key was released, we reset selection to new position
		bool lbStreamSelection = (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION)) == CONSOLE_TEXT_SELECTION;
		StartSelection(lbStreamSelection, cr.X, cr.Y);
	}
	else
	{
		// `Shift` is holded, extend selection range
		ExpandSelection(cr.X, cr.Y, true);
	}
}

UINT CRealBuffer::CorrectSelectionAnchor()
{
	// Active selection is expected here
	if (!con.m_sel.dwFlags)
	{
		_ASSERTE(con.m_sel.dwFlags!=0);
		return 0;
	}
	// If selection is greater than one char
	if ((con.m_sel.srSelection.Top < con.m_sel.srSelection.Bottom)
		|| (con.m_sel.srSelection.Left < con.m_sel.srSelection.Right))
	{
		if ((con.m_sel.srSelection.Top == con.m_sel.dwSelectionAnchor.Y)
			&& (con.m_sel.srSelection.Left == con.m_sel.dwSelectionAnchor.X))
		{
			SetSelectionFlags((con.m_sel.dwFlags & ~(CONSOLE_LEFT_ANCHOR | CONSOLE_RIGHT_ANCHOR)) | CONSOLE_LEFT_ANCHOR);
		}
		else if ((con.m_sel.srSelection.Bottom == con.m_sel.dwSelectionAnchor.Y)
			&& (con.m_sel.srSelection.Right == con.m_sel.dwSelectionAnchor.X))
		{
			SetSelectionFlags((con.m_sel.dwFlags & ~(CONSOLE_LEFT_ANCHOR | CONSOLE_RIGHT_ANCHOR)) | CONSOLE_RIGHT_ANCHOR);
		}
	}
	// Result
	return (con.m_sel.dwFlags & (CONSOLE_LEFT_ANCHOR|CONSOLE_RIGHT_ANCHOR));
}

void CRealBuffer::ExpandSelection(SHORT anX, SHORT anY, bool bWasSelection)
{
	_ASSERTE(con.m_sel.dwFlags!=0);
	int iCurTop = GetBufferPosY();
	_ASSERTE(anY>=0 && anY<con.m_sbi.dwSize.Y);

	CONSOLE_SELECTION_INFO cur_sel = con.m_sel;

	SetSelectionFlags(con.m_sel.dwFlags | CONSOLE_EXPANDED);

	// 131017 Scroll content if selection cursor goes out of visible screen
	if (anY < iCurTop)
	{
		DoScrollBuffer(SB_LINEUP, -1, (iCurTop - anY));
	}
	else if (anY >= (iCurTop + con.nTextHeight))
	{
		DoScrollBuffer(SB_LINEDOWN, -1, (anY - iCurTop - con.nTextHeight + 1));
	}

	COORD cr = {anX,anY};

	if (cr.X<0 || cr.X>=GetBufferWidth() || cr.Y<0 || cr.Y>=GetBufferHeight())
	{
		_ASSERTE(cr.X>=0 && cr.X<GetBufferWidth());
		_ASSERTE(cr.Y>=0 && cr.Y<GetBufferHeight());
		return; // Ошибка в координатах
	}

	BOOL lbStreamSelection = (con.m_sel.dwFlags & (CONSOLE_TEXT_SELECTION)) == CONSOLE_TEXT_SELECTION;
	DWORD nAnchorFlags = CorrectSelectionAnchor();

	if (!lbStreamSelection)
	{
		if (cr.X < con.m_sel.dwSelectionAnchor.X)
		{
			con.m_sel.srSelection.Left = cr.X;
			con.m_sel.srSelection.Right = con.m_sel.dwSelectionAnchor.X;
		}
		else
		{
			con.m_sel.srSelection.Left = con.m_sel.dwSelectionAnchor.X;
			con.m_sel.srSelection.Right = cr.X;
		}
	}
	else
	{
		const COORD anchor = con.m_sel.dwSelectionAnchor;
		const SMALL_RECT srSel = con.m_sel.srSelection;
		// Switch Left/Right members
		if ((cr.Y > anchor.Y)
		        || ((cr.Y == anchor.Y) && (cr.X > anchor.X)))
		{
			// Extending selection rightward, if it was leftward selection?
			if (con.m_sel.dwFlags & CONSOLE_RIGHT_ANCHOR)
			{
				if (cr.Y > anchor.Y)
				{
					if ((anchor.X + 1) < GetBufferWidth())
					{
						SetSelectionAnchor(con.m_sel.dwSelectionAnchor.X + 1, con.m_sel.dwSelectionAnchor.Y);
					}
					else
					{
						if ((con.m_sel.dwSelectionAnchor.Y + 1) < GetBufferHeight())
						{
							SetSelectionAnchor(0, con.m_sel.dwSelectionAnchor.Y + 1);
						}
						else
						{
							_ASSERTE((con.m_sel.dwSelectionAnchor.Y + 1) < GetBufferHeight());
						}
					}
					SetSelectionFlags((con.m_sel.dwFlags & ~CONSOLE_RIGHT_ANCHOR) | CONSOLE_LEFT_ANCHOR);
				}
				else if (((anchor.X + 1) < GetBufferWidth())
					&& (cr.X >= (anchor.X /*+ ((con.m_sel.dwFlags & CONSOLE_MOUSE_DOWN) ? 1 : 0)*/)))
				{
					SetSelectionAnchor(con.m_sel.dwSelectionAnchor.X + 1, con.m_sel.dwSelectionAnchor.Y);
					cr.X = std::max(cr.X, con.m_sel.dwSelectionAnchor.X);
					SetSelectionFlags((con.m_sel.dwFlags & ~CONSOLE_RIGHT_ANCHOR) | CONSOLE_LEFT_ANCHOR);
				}
			}
			con.m_sel.srSelection.Left = con.m_sel.dwSelectionAnchor.X;
			if (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION)
			{
				COORD crFrom{}, crTo{};
				ExpandTextRange(cr, crFrom, crTo, etr_Word);
				cr = crTo;
			}
			con.m_sel.srSelection.Right = cr.X;
		}
		else
		{
			if (!(con.m_sel.dwFlags & CONSOLE_RIGHT_ANCHOR))
			{
				_ASSERTE(con.m_sel.dwSelectionAnchor.Y == con.m_SelDblClickRightAnchor.Y);
				if (con.m_sel.dwSelectionAnchor.Y == con.m_SelDblClickRightAnchor.Y)
				{
					SetSelectionAnchor(con.m_SelDblClickRightAnchor.X, con.m_SelDblClickRightAnchor.Y);
				}
			}
			// Extending selection leftward, if it was rightward selection?
			if (con.m_sel.dwFlags & CONSOLE_LEFT_ANCHOR)
			{
				if (cr.Y < anchor.Y)
				{
					if (con.m_sel.dwSelectionAnchor.X > 0)
					{
						SetSelectionAnchor(con.m_sel.dwSelectionAnchor.X - 1, con.m_sel.dwSelectionAnchor.Y);
					}
					else
					{
						SetSelectionAnchor(GetBufferWidth() - 1, con.m_sel.dwSelectionAnchor.Y - 1);
					}
					SetSelectionFlags((con.m_sel.dwFlags & ~CONSOLE_LEFT_ANCHOR) | CONSOLE_RIGHT_ANCHOR);
				}
				else if ((anchor.X > 1)
					&& ((cr.X /*+ ((con.m_sel.dwFlags & CONSOLE_MOUSE_DOWN) ? 1 : 0)*/) < anchor.X))
				{
					SetSelectionAnchor(con.m_sel.dwSelectionAnchor.X - 1, con.m_sel.dwSelectionAnchor.Y);
					cr.X = std::min(cr.X, con.m_sel.dwSelectionAnchor.X);
					SetSelectionFlags((con.m_sel.dwFlags & ~CONSOLE_LEFT_ANCHOR) | CONSOLE_RIGHT_ANCHOR);
				}
			}
			if (con.m_sel.dwFlags & CONSOLE_DBLCLICK_SELECTION)
			{
				COORD crFrom{}, crTo{};
				ExpandTextRange(cr, crFrom, crTo, etr_Word);
				cr = crFrom;
			}
			con.m_sel.srSelection.Left = cr.X;
			con.m_sel.srSelection.Right = con.m_sel.dwSelectionAnchor.X;
		}

		if (anchor != con.m_sel.dwSelectionAnchor)
		{
			wchar_t szLog[140];
			swprintf_c(szLog,
				L"Selection: %s: Coord={%i,%i} Old={%i,%i}:{%i,%i}-{%i,%i} New={%i,%i}:{%i,%i}-{%i,%i}",
				(con.m_sel.dwFlags & CONSOLE_LEFT_ANCHOR) ? L"<<LeftAnchor" :
				(con.m_sel.dwFlags & CONSOLE_RIGHT_ANCHOR) ? L"RightAnchor>>" : L"???",
				anX, anY, anchor.X, anchor.Y, LogSRectCoords(srSel),
				con.m_sel.dwSelectionAnchor.X, con.m_sel.dwSelectionAnchor.Y, LogSRectCoords(con.m_sel.srSelection)
				);
			mp_RCon->LogString(szLog);
		}
	}

	if (cr.Y < con.m_sel.dwSelectionAnchor.Y)
	{
		con.m_sel.srSelection.Top = cr.Y;
		con.m_sel.srSelection.Bottom = con.m_sel.dwSelectionAnchor.Y;
	}
	else
	{
		con.m_sel.srSelection.Top = con.m_sel.dwSelectionAnchor.Y;
		con.m_sel.srSelection.Bottom = cr.Y;
	}

	const bool bChanged = (memcmp(&cur_sel, &con.m_sel, sizeof(cur_sel)) != 0);

	if (!bWasSelection || bChanged)
	{
		UpdateSelection();
	}
}

void CRealBuffer::DoSelectionStop()
{
	const BYTE vkMod = HIBYTE(HIWORD(con.m_sel.dwFlags));

	if (vkMod)
	{
		// Но чтобы ФАР не запустил макрос (если есть макро на RAlt например...)
		if (vkMod == VK_CONTROL || vkMod == VK_LCONTROL || vkMod == VK_RCONTROL)
			mp_RCon->PostKeyPress(VK_SHIFT, LEFT_CTRL_PRESSED, 0);
		else if (vkMod == VK_MENU || vkMod == VK_LMENU || vkMod == VK_RMENU)
			mp_RCon->PostKeyPress(VK_SHIFT, LEFT_ALT_PRESSED, 0);
		else
			mp_RCon->PostKeyPress(VK_CONTROL, SHIFT_PRESSED, 0);

		// "Отпустить" в консоли модификатор
		mp_RCon->PostKeyUp(vkMod, 0, 0);
	}

	SetSelectionFlags(0);
}

bool CRealBuffer::DoSelectionCopy(CECopyMode CopyMode /*= cm_CopySel*/, BYTE nFormat /*= CTSFormatDefault*/ /* use gpSet->isCTSHtmlFormat */, LPCWSTR pszDstFile /*= nullptr*/, HGLOBAL* phUnicode /*= nullptr*/)
{
	bool bRc = false;

	RealBufferType bufType = m_Type;

	bool bResetSelection = false;
	if (con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION)
		bResetSelection = gpSet->isCTSResetOnRelease;
	else if (con.m_sel.dwFlags)
		bResetSelection = (gpSet->isCTSEndOnTyping != 0);

	if (!con.m_sel.dwFlags)
	{
		MBoxAssert(con.m_sel.dwFlags != 0);
	}
	else
	{
		const bool lbStreamMode = (con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) == CONSOLE_TEXT_SELECTION;
		bool  lbProcessed = false;

		// Сначала проверим, помещается ли "выделенная область" в "ВИДИМУЮ область"
		if (m_Type == rbt_Primary)
		{
			const COORD crStart = BufferToScreen(MakeCoord(con.m_sel.srSelection.Left, con.m_sel.srSelection.Top), false);
			const COORD crEnd = BufferToScreen(MakeCoord(con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom), false);

			const int nTextWidth = this->GetTextWidth();
			const int nTextHeight = this->GetTextHeight();

			if ((crStart.X < 0) || (crStart.X >= nTextWidth)
				|| (crStart.Y < 0) || (crStart.Y >= nTextHeight)
				|| (crEnd.X < 0) || (crEnd.X >= nTextWidth)
				|| (crEnd.Y < 0) || (crEnd.Y >= nTextHeight))
			{
				if (mp_RCon->LoadAlternativeConsole(lam_FullBuffer) && (mp_RCon->mp_ABuf != this))
				{
					CONSOLE_SELECTION_INFO sel = con.m_sel;
					DoSelectionStop();

					_ASSERTE(mp_RCon->mp_ABuf->m_Type==rbt_Alternative);
					mp_RCon->mp_ABuf->m_Type = rbt_Selection; // Изменить, чтобы по завершении выделения - буфер закрыть
					mp_RCon->mp_ABuf->con.m_sel = sel;

					bRc = mp_RCon->mp_ABuf->DoSelectionCopyInt(CopyMode, lbStreamMode, sel.srSelection.Left, sel.srSelection.Top, sel.srSelection.Right, sel.srSelection.Bottom, nFormat, pszDstFile);

					lbProcessed = true;
					bufType = rbt_Selection;
				}
				else
				{
					_ASSERTE(FALSE && "Failed to load full console data?");
				}
			}
		}

		if (!lbProcessed)
		{
			bRc = DoSelectionCopyInt(CopyMode, lbStreamMode, con.m_sel.srSelection.Left, con.m_sel.srSelection.Top, con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom, nFormat, pszDstFile, phUnicode);
		}
	}

	// Fin, Reset selection region
	if (bResetSelection && bRc)
	{
		DoSelectionStop(); // con.m_sel.dwFlags = 0;

		if (bufType == rbt_Selection)
		{
			mp_RCon->SetActiveBuffer(rbt_Primary);
			// Сразу на выход!
		}

		if (m_Type == rbt_Primary)
		{
			UpdateSelection(); // обновить на экране
		}
	}

	return bRc;
}

int CRealBuffer::GetSelectionCharCount(bool bStreamMode, int srSelection_X1, int srSelection_Y1, int srSelection_X2, int srSelection_Y2, int* pnSelWidth, int* pnSelHeight, int nNewLineLen)
{
	int nCharCount = 0;
	const int nSelWidth = srSelection_X2 - srSelection_X1 + 1;
	const int nSelHeight = srSelection_Y2 - srSelection_Y1 + 1;

	_ASSERTE(nNewLineLen==0 || nNewLineLen==1 || nNewLineLen==2);

	if (!bStreamMode)
	{
		nCharCount = ((nSelWidth+nNewLineLen/* "\r\n" */) * nSelHeight) - nNewLineLen; // после последней строки "\r\n" не ставится
	}
	else
	{
		if (nSelHeight == 1)
		{
			nCharCount = nSelWidth;
		}
		else if (nSelHeight == 2)
		{
			// На первой строке - до конца строки, вторая строка - до окончания блока, + "\r\n"
			nCharCount = (con.nTextWidth - srSelection_X1) + (srSelection_X2 + 1) + nNewLineLen;
		}
		else
		{
			Assert(nSelHeight>2);
			// На первой строке - до конца строки, последняя строка - до окончания блока, + "\r\n"
			nCharCount = (con.nTextWidth - srSelection_X1) + (srSelection_X2 + 1) + nNewLineLen
			             + ((nSelHeight - 2) * (con.nTextWidth + nNewLineLen)); // + серединка * (длину консоли + "\r\n")
		}
	}

	if (pnSelWidth)
		*pnSelWidth = nSelWidth;
	if (pnSelHeight)
		*pnSelHeight = nSelHeight;

	return nCharCount;
}

// Здесь CopyMode уже не используется, передается для информации
bool CRealBuffer::DoSelectionCopyInt(CECopyMode CopyMode, bool bStreamMode, int srSelection_X1, int srSelection_Y1, int srSelection_X2, int srSelection_Y2, BYTE nFormat /*= CTSFormatDefault*/, LPCWSTR pszDstFile /*= nullptr*/, HGLOBAL* phUnicode /*= nullptr*/)
{
	// Warning!!! Здесь уже нельзя ориентироваться на con.m_sel !!!

	LPCWSTR pszDataStart = nullptr;
	WORD* pAttrStart = nullptr;
	CharAttr* pAttrStartEx = nullptr;
	int nTextWidth = 0, nTextHeight = 0;
	CRConDataGuard data;

	if ((CopyMode == cm_CopyInt) && phUnicode)
		nFormat = 0; // Выделен текст для того чтобы сразу его вставить, не трогая Clipboard
	if (nFormat == CTSFormatDefault)
		nFormat = gpSet->isCTSHtmlFormat;

	if (m_Type == rbt_Primary)
	{
		nTextWidth = this->GetTextWidth();
		nTextHeight = this->GetTextHeight();
		if (!GetData(data) || !data.isValid(nTextWidth*nTextHeight))
			return false;
		pszDataStart = data->pConChar;
		pAttrStart = data->pConAttr;
		_ASSERTE(pszDataStart[nTextWidth*nTextHeight] == 0); // Должно быть ASCIIZ

		// Выделение "с прокруткой" обрабатываем только в альтернативных буферах,
		// а здесь нам нужны только "экранные" координаты

		COORD cr = BufferToScreen(MakeCoord(srSelection_X1, srSelection_Y1));
		srSelection_X1 = cr.X; srSelection_Y1 = cr.Y;
		cr = BufferToScreen(MakeCoord(srSelection_X2, srSelection_Y2));
		srSelection_X2 = cr.X; srSelection_Y2 = cr.Y;
	}
	else if (dump.pszBlock1)
	{
		WARNING("Доработать для режима с прокруткой");
		nTextWidth = dump.crSize.X;
		nTextHeight = dump.crSize.Y;
		//nTextWidth = this->GetTextWidth();
		//nTextHeight = this->GetTextHeight();
		_ASSERTE(dump.pszBlock1[nTextWidth*nTextHeight] == 0); // Должно быть ASCIIZ

		pszDataStart = dump.pszBlock1; // + con.m_sbi.srWindow.Top * nTextWidth; -- работаем с полным буфером
		pAttrStartEx = dump.pcaBlock1;
		//nTextHeight = std::min((dump.crSize.Y-con.m_sbi.srWindow.Top),(con.m_sbi.srWindow.Bottom - con.m_sbi.srWindow.Top + 1));
	}

	if (!pszDataStart || !nTextWidth || !nTextHeight)
	{
		Assert(pszDataStart != nullptr);
		return false;
	}

	const AppSettings* pApp = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());
	if (pApp == nullptr)
	{
		Assert(pApp!=nullptr);
		return false;
	}
	BYTE nEOL = pApp->CTSEOL();
	BYTE nTrimTailing = pApp->CTSTrimTrailing();
	bool bDetectLines = pApp->CTSDetectLineEnd();
	bool bBash = pApp->CTSBashMargin();

	// Pre validations
	if (srSelection_X1 > (srSelection_X2+(srSelection_Y2-srSelection_Y1)*nTextWidth))
	{
		Assert(srSelection_X1 <= (srSelection_X2+(srSelection_Y2-srSelection_Y1)*nTextWidth));
		return false;
	}

	DWORD dwErr = 0;
	BOOL  lbRc = FALSE;
	bool  Result = false;
	int   nSelWidth = 0, nSelHeight = 0;
	int   nNewLineLen = 2; // max "\r\n"

	int   nCharCount = GetSelectionCharCount(bStreamMode, srSelection_X1, srSelection_Y1, srSelection_X2, srSelection_Y2, &nSelWidth, &nSelHeight, nNewLineLen);
	_ASSERTE((nSelWidth>-(srSelection_X1-srSelection_X2)) && (nSelHeight>0) && (nCharCount>0));

	std::vector<ConPtyGlyph> copiedGlyphs;
	std::map<size_t, ConPtyGlyph> glyphAt;
	std::set<size_t> glyphTails;
	if (mp_RCon->GetConPtyGlyphs(nTextWidth, nTextHeight, copiedGlyphs, m_Type != rbt_Primary))
		for (const auto& glyph : copiedGlyphs)
		{
			const size_t index = glyph.position.Y * nTextWidth + glyph.position.X;
			glyphAt.emplace(index, glyph);
			for (unsigned n = 1; n < glyph.width; ++n) glyphTails.insert(index + n);
			nCharCount += static_cast<int>(glyph.text.size());
		}

	HGLOBAL hUnicode = nullptr;
	hUnicode = GlobalAlloc(GMEM_MOVEABLE|GMEM_ZEROINIT, (nCharCount+1)*sizeof(wchar_t));

	if (hUnicode == nullptr)
	{
		Assert(hUnicode != nullptr);
		return false;
	}

	wchar_t *pch = (wchar_t*)GlobalLock(hUnicode);

	if (!pch)
	{
		Assert(pch != nullptr);
		GlobalFree(hUnicode);
		return false;
	}

	auto appendCell = [&](LPCWSTR& source) {
		const size_t index = source - pszDataStart;
		const auto glyph = glyphAt.find(index);
		if (glyph != glyphAt.end())
		{
			wmemcpy(pch, glyph->second.text.data(), glyph->second.text.size());
			pch += glyph->second.text.size();
		}
		else if (!glyphTails.count(index)) *(pch++) = *source;
		++source;
	};

	// Заполнить данными
	if ((srSelection_X1 + nSelWidth) > nTextWidth)
	{
		Assert((srSelection_X1 + nSelWidth) <= nTextWidth);
		nSelWidth = nTextWidth - srSelection_X1;
	}

	if ((srSelection_Y1 + nSelHeight) > nTextHeight)
	{
		Assert((srSelection_Y1 + nSelHeight) <= nTextHeight);
		nSelHeight = nTextHeight - srSelection_Y1;
	}


	nSelHeight--;

	const auto& pPal = mp_RCon->VCon()->GetColors();

	bool bUseHtml = (nFormat != 0);
	std::vector<wchar_t> formattedChars;
	std::vector<CharAttr> formattedAttrs;
	if (bUseHtml && m_Type == rbt_Primary && mp_RCon->UseTrueColor())
	{
		// Use the same RGB conversion as the viewport renderer for formatted copies.
		const size_t cells = static_cast<size_t>(nTextWidth) * nTextHeight;
		formattedChars.resize(cells);
		formattedAttrs.resize(cells);
		CharAttr* normal = nullptr;
		CharAttr* extended = nullptr;
		PrepareColorTable(pApp, &extended, &normal);
		data->GetConsoleData(formattedChars.data(), formattedAttrs.data(), nTextWidth, nTextHeight,
			L' ', normal[7], normal, extended, false, pApp->ExtendFonts());
		pAttrStart = nullptr;
		pAttrStartEx = formattedAttrs.data();
	}

	CFormatCopy* html = nullptr;

	if (bUseHtml)
	{
		COLORREF crFore = 0xC0C0C0, crBack = 0;
		if ((m_Type == rbt_Primary) && pAttrStart)
		{
			WORD *pAttr = pAttrStart + con.nTextWidth*srSelection_Y1 + srSelection_X1;
			crFore = pPal[CONFORECOLOR(*pAttr)]; crBack = pPal[CONBACKCOLOR(*pAttr)];
		}
		else if (pAttrStartEx)
		{
			CharAttr* pAttr = pAttrStartEx + con.nTextWidth*srSelection_Y1 + srSelection_X1;
			crFore = pAttr->crForeColor; crBack = pAttr->crBackColor;
		}

		if (nFormat == 3)
		{
			html = new CAnsiCopy(pPal, crFore, crBack);
		}
		else
		{
			//wchar_t szClass[64]; swprintf_c(szClass, L"ConEmu%s%s", gpConEmu->ms_ConEmuBuild, WIN3264TEST(L"x32",L"x64"));
			html = new CHtmlCopy((nFormat == 2), gpConEmu->ms_ConEmuBuild, gpFontMgr->FontFaceName(), gpFontMgr->FontHeightHtml(), crFore, crBack);
		}
	}


	auto addGraphemeHtml = [&](LPCWSTR source, int count) {
		if (glyphAt.empty() || !pAttrStartEx) return false;
		std::wstring text;
		std::vector<CharAttr> attrs;
		const size_t first = source - pszDataStart;
		for (int n = 0; n < count; ++n)
		{
			const size_t index = first + n;
			const auto glyph = glyphAt.find(index);
			if (glyph != glyphAt.end())
			{
				text += glyph->second.text;
				attrs.insert(attrs.end(), glyph->second.text.size(), pAttrStartEx[index]);
			}
			else if (!glyphTails.count(index)) { text += source[n]; attrs.push_back(pAttrStartEx[index]); }
		}
		if (!text.empty()) html->LineAdd(text.data(), attrs.data(), text.size());
		return true;
	};

	if (!bStreamMode)
	{
		// Блоковое выделение
		for (int Y = 0; Y <= nSelHeight; Y++)
		{
			LPCWSTR pszCon = nullptr;

			if (m_Type == rbt_Primary)
			{
				pszCon = pszDataStart + con.nTextWidth * (Y + srSelection_Y1) + srSelection_X1;
			}
			else if (pszDataStart && (Y < nTextHeight))  // -V560
			{
				WARNING("Проверить для режима с прокруткой!");
				pszCon = pszDataStart + dump.crSize.X * (Y + srSelection_Y1) + srSelection_X1;
			}

			//LPCWSTR pszDstStart = pch;
			LPCWSTR pszSrcStart = pszCon;

			int nMaxX = nSelWidth - 1;

			if (pszCon)
			{
				wchar_t* pchStart = pch;

				for (int X = 0; X <= nMaxX; X++)
				{
					appendCell(pszCon);
				}

				if (nTrimTailing == 1)
				{
					while ((pch > pchStart) && (*(pch-1) == L' '))
					{
						*(--pch) = 0;
					}
				}

				if (bUseHtml)
				{
					INT_PTR nLineStart = pszSrcStart - pszDataStart;
					if (addGraphemeHtml(pszSrcStart, nSelWidth)) {}
					else if ((m_Type == rbt_Primary) && pAttrStart)
						html->LineAdd(pszSrcStart, pAttrStart+nLineStart, pPal, nSelWidth);
					else if (pAttrStartEx)
						html->LineAdd(pszSrcStart, pAttrStartEx+nLineStart, nSelWidth);
				}

			}
			else if (nTrimTailing != 1)
			{
				wmemset(pch, L' ', nSelWidth);
				pch += nSelWidth;
			}

			// Добавить перевод строки
			if (Y < nSelHeight)
			{
				switch (nEOL)
				{
				case 1:
					*(pch++) = L'\n'; break;
				case 2:
					*(pch++) = L'\r'; break;
				default:
					*(pch++) = L'\r'; *(pch++) = L'\n';
				}
			}
		}
	}
	else
	{
		// Потоковое (текстовое) выделение
		int nX1, nX2;

		for (int Y = 0; Y <= nSelHeight; Y++)
		{
			nX1 = (Y == 0) ? srSelection_X1 : 0;
			nX2 = (Y == nSelHeight) ? srSelection_X2 : (con.nTextWidth-1);
			LPCWSTR pszCon = nullptr;
			LPCWSTR pszNextLine = nullptr;

			if (m_Type == rbt_Primary)
			{
				pszCon = pszDataStart + con.nTextWidth*(Y+srSelection_Y1) + nX1;
				pszNextLine = ((Y + 1) <= nSelHeight) ? (pszDataStart + con.nTextWidth*(Y+1+srSelection_Y1)) : nullptr;
			}
			else if (pszDataStart && (Y < nTextHeight))  // -V560
			{
				WARNING("Проверить для режима с прокруткой!");
				pszCon = pszDataStart + dump.crSize.X*(Y+srSelection_Y1) + nX1;
				pszNextLine = ((Y + 1) <= nSelHeight) ? (pszDataStart + dump.crSize.X*(Y+1+srSelection_Y1)) : nullptr;
			}

			LPCWSTR pszDstStart = pch;
			LPCWSTR pszSrcStart = pszCon;

			wchar_t* pchStart = pch;

			if (pszCon)
			{
				for (int X = nX1; X <= nX2; X++)
				{
					appendCell(pszCon);
				}

				if (bUseHtml)
				{
					INT_PTR nLineStart = pszSrcStart - pszDataStart;
					INT_PTR nLineLen = nX2 - nX1 + 1;
					if (addGraphemeHtml(pszSrcStart, nLineLen)) {}
					else if ((m_Type == rbt_Primary) && pAttrStart)
						html->LineAdd(pszSrcStart, pAttrStart+nLineStart, pPal, nLineLen);
					else if (pAttrStartEx)
						html->LineAdd(pszSrcStart, pAttrStartEx+nLineStart, nLineLen);
				}

			}
			else
			{
				wmemset(pch, L' ', nSelWidth);
				pch += nSelWidth;
			}

			// Добавить перевод строки
			if (Y < nSelHeight)
			{
				bool bContinue = false;

				if (bDetectLines && pszNextLine
					// Allow maximum one space on the next line
					&& ((pszNextLine[0] != L' ') || (pszNextLine[0] == L' ' && pszNextLine[1] != L' '))  // -V728
					// If right or left edge of screen is "Frame" - force to line break!
					&& !IsForceLineBreakChar(*(pch - 1))
					&& !IsForceLineBreakChar(*pszNextLine))
				{
					// Пытаемся определить, новая это строка или просто перенос в Prompt?
					if ((*(pch - 1) != L' ')
						|| (((pch - 1) > pchStart) && (*(pch - 2) != L' '))
						// bash.exe - one cell space pad on right edge
						|| (bBash && ((pch - 2) > pchStart) && (*(pch - 2) == L' ') && (*(pch - 3) != L' ')))
					{
						bContinue = true;
					}

					if (bBash && (*(pch - 1) == L' '))
					{
						*(--pch) = 0;
					}
				}

				if (!bContinue)
				{
					if (nTrimTailing)
					{
						while ((pch > pchStart) && (*(pch-1) == L' '))
						{
							*(--pch) = 0;
						}
					}

					switch (nEOL)
					{
					case 1:
						*(pch++) = L'\n'; break;
					case 2:
						*(pch++) = L'\r'; break;
					default:
						*(pch++) = L'\r'; *(pch++) = L'\n';
					}
				}
			}
			else
			{
				if (nTrimTailing)
				{
					while ((pch > pchStart) && (*(pch-1) == L' '))
					{
						*(--pch) = 0;
					}
				}
			}
		}
	}

	// Ready
	GlobalUnlock(hUnicode);

	if ((CopyMode == cm_CopyInt) && phUnicode)
	{
		// Был выделен текст для того чтобы сразу его вставить, не трогая Clipboard
		*phUnicode = hUnicode;
		SafeDelete(html);
		return true;
	}

	// HTML?
	HGLOBAL hHtml = nullptr;
	if (bUseHtml)
	{
		hHtml = html->CreateResult();
		if (!hHtml)
		{
			dwErr = GetLastError();
			DisplayLastError(L"Creating HTML format failed!", dwErr, MB_ICONSTOP);
			GlobalFree(hUnicode);
			SafeDelete(html);
			return false;
		}
	}

	// User asked to copy HTML instead of HTML formatted (put HTML in CF_UNICODE)
	if ((nFormat == 2 || nFormat == 3) && hHtml)
	{
		WARNING("hUnicode Overhead...");
		GlobalFree(hUnicode);
		hUnicode = hHtml;
		hHtml = nullptr;
		bUseHtml = false;
	}

	SafeDelete(html);

	// Asked to save it to file instead of clipboard?
	if (pszDstFile)
	{
		int iWriteRc = -1; DWORD nErrCode = 0;
		LPCWSTR pszSrc = (LPCWSTR)GlobalLock(hUnicode);
		if (pszSrc)
		{
			iWriteRc = WriteTextFile(pszDstFile, pszSrc, -1, CP_UTF8, false/*WriteBOM*/, &nErrCode);
			GlobalUnlock(hUnicode);
		}
		GlobalFree(hUnicode);
		if (iWriteRc < 0)
		{
			const CEStr pszErr(L"Failed to create file\n", pszDstFile);
			DisplayLastError(pszErr, nErrCode);
			return false;
		}
		return true;
	}

	// Открыть буфер обмена
	if (!(lbRc = MyOpenClipboard(L"SetClipboard")))
	{
		GlobalFree(hUnicode);
		if (hHtml) GlobalFree(hHtml);
		return false;
	}

	UINT i_CF_HTML = bUseHtml ? RegisterClipboardFormat(L"HTML Format") : 0;

	lbRc = EmptyClipboard();
	// Установить данные
	Result = MySetClipboardData(CF_UNICODETEXT, hUnicode)
		&& (!i_CF_HTML || MySetClipboardData(i_CF_HTML, hHtml));

	while (!Result)
	{
		dwErr = GetLastError();

		if (IDRETRY != DisplayLastError(L"SetClipboardData(CF_UNICODETEXT, ...) failed!", dwErr, MB_RETRYCANCEL|MB_ICONSTOP))
		{
			GlobalFree(hUnicode); hUnicode = nullptr;
			if (hHtml) GlobalFree(hHtml); hHtml = nullptr;
			break;
		}

		Result = MySetClipboardData(CF_UNICODETEXT, hUnicode)
			&& (!i_CF_HTML || MySetClipboardData(i_CF_HTML, hHtml));
	}

	if (Result)
	{
		if (!bStreamMode)
		{
			UINT i_CF_BORLAND_IDE = RegisterClipboardFormat(L"Borland IDE Block Type");
			UINT i_CF_MICROSOFT_IDE = RegisterClipboardFormat(L"MSDEVColumnSelect");

			if (i_CF_BORLAND_IDE)
			{
				MGlobal borland(GMEM_MOVEABLE,1);
				if (borland.Lock())
				{
					*(LPBYTE)borland.Lock() = 2;
					if (MySetClipboardData(i_CF_BORLAND_IDE, borland))
						borland.Detach();
				}
			}
			if (i_CF_MICROSOFT_IDE)
			{
				MySetClipboardData(i_CF_MICROSOFT_IDE, nullptr);
			}
		}

		if (gpSet->isCTSForceLocale)
		{
			MGlobal Lcl;
			if (Lcl.Alloc(GMEM_MOVEABLE, sizeof(DWORD))
				&& Lcl.Lock())
			{
				*(LPDWORD)Lcl.Lock() = gpSet->isCTSForceLocale;
				if (MySetClipboardData(CF_LOCALE, Lcl))
					Lcl.Detach();
			}
		}
	}

	MyCloseClipboard();

	return Result;
}

int CRealBuffer::GetSelectionCellsCount()
{
	int nCharCount = 0;
	if (con.m_sel.dwFlags)
	{
		nCharCount = GetSelectionCharCount(isStreamSelection(),
			con.m_sel.srSelection.Left, con.m_sel.srSelection.Top,
			con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom, nullptr, nullptr, 0);
	}
	return nCharCount;
}

// обновить на экране
void CRealBuffer::UpdateSelection()
{
	InterlockedIncrement(&con.mn_UpdateSelectionCalled);

	// Show current selection state in the Status bar
	mp_RCon->OnSelectionChanged();

	TODO("Это корректно? Нужно обновить VCon");
	con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole
	//mp_RCon->mp_VCon->Update(true); -- Update() и так вызывается в PaintVConNormal
	mp_RCon->mp_VCon->Redraw(true);
}

void CRealBuffer::UpdateHyperlink()
{
	DEBUGSTRLINK(con.etr.etrLast ? L"Highligting hyperlink" : L"Drop hyperlink highlighting");
	con.etrWasChanged = false;
	con.bConsoleDataChanged = TRUE;
	mp_RCon->mp_VCon->Redraw(true);
}

bool CRealBuffer::isConSelectMode()
{
	if (!this) return false;

	if (con.m_sel.dwFlags != 0)
		return true;

	if ((this == mp_RCon->mp_RBuf) && mp_RCon->isFar())
	{
		// В Фаре может быть активен граббер (AltIns)
		if (con.m_ci.dwSize >= 40)  // Попробуем его определять по высоте курсора.
			return true;
	}

	return false;
}

bool CRealBuffer::isStreamSelection()
{
	if (!this) return false;

	return ((con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) != 0);
}

// true - if clipboard was set successfully
bool CRealBuffer::DoSelectionFinalize(bool abCopy, CECopyMode CopyMode, WPARAM wParam, HGLOBAL* phUnicode)
{
	bool bCopied = false;

	if (abCopy)
	{
		bCopied = DoSelectionCopy(CopyMode, CTSFormatDefault, nullptr, phUnicode);
	}

	mp_RCon->mn_SelectModeSkipVk = wParam;

	// Spare if abCopy is true
	DoSelectionStop(); // con.m_sel.dwFlags = 0;

	if (m_Type == rbt_Selection)
	{
		mp_RCon->SetActiveBuffer(rbt_Primary);
		// Сразу на выход!
		return bCopied;
	}
	else
	{
		//mb_ConsoleSelectMode = false;
		UpdateSelection(); // обновить на экране
	}

	return bCopied;
}

// pszChars may be nullptr
const ConEmuHotKey* CRealBuffer::ProcessSelectionHotKey(const ConEmuChord& VkState, bool bKeyDown, const wchar_t *pszChars)
{
	if (!this || !con.m_sel.dwFlags)
		return nullptr;

	// If these was not processed by user HotKeys, lets do it...
	// Ctrl+C or Ctrl+Ins must copy current selection to clipboard
	// By default, default vkCTSCopyHtml0 is called with GuiMacro "Copy(0,0)" on Ctrl+C
	if (VkState.IsEqual('C', cvk_Ctrl) || VkState.IsEqual(VK_INSERT, cvk_Ctrl))
	{
		if (bKeyDown)
		{
			DoSelectionFinalize(true, cm_CopySel, VkState.Vk);
		}
		return ConEmuSkipHotKey;
	}

	// Let Far process Paste by Ctrl+V / Shift+Ins
	if (mp_RCon->isFar()
		&& (VkState.IsEqual('V', cvk_Ctrl) || VkState.IsEqual(VK_INSERT, cvk_Shift)))
	{
		if (bKeyDown)
		{
			mp_RCon->DoSelectionFinalize();
		}
		return nullptr;
	}

	// Del/Shift-Del/BS/Ctrl-X - try to "edit" prompt
	bool bDel = false, bShiftDel = false, bBS = false, bCtrlX = false;
	if (gpSet->isCTSEraseBeforeReset &&
		((bDel = VkState.IsEqual(VK_DELETE, cvk_Naked))
		|| (bShiftDel = VkState.IsEqual(VK_DELETE, cvk_Shift))
		|| (bCtrlX = VkState.IsEqual('X', cvk_Ctrl))
		|| (bBS = VkState.IsEqual(VK_BACK, cvk_Naked)))
		)
	{
		const bool bCopyBeforeErase = (bShiftDel || bCtrlX);
		const CONSOLE_SELECTION_INFO sel = con.m_sel;
		const COORD cur = con.m_sbi.dwCursorPosition;
		const COORD anchor = sel.dwSelectionAnchor;
		if (bKeyDown
			// If this was one-line selection
			&& (sel.srSelection.Top == sel.srSelection.Bottom)
			// And cursor position matches anchor position
			&& (cur.Y == anchor.Y)
			&& (((cur.X == anchor.X) && (anchor.X == sel.srSelection.Left) && (sel.srSelection.Right >= cur.X))
				|| ((cur.X == (anchor.X+1)) && (anchor.X == sel.srSelection.Right) && (sel.srSelection.Left <= cur.X))
				)
			)
		{
			DoSelectionFinalize(bCopyBeforeErase, cm_CopySel, VkState.Vk);
			// Now post sequence of keys
			UINT vkPostKey = 0; LPCWSTR pszKey = nullptr;
			if ((anchor.X == sel.srSelection.Left) && (cur.X == anchor.X)
				&& (sel.srSelection.Right >= sel.srSelection.Left))
			{
				vkPostKey = VK_DELETE; pszKey = L"Delete";
			}
			else if ((anchor.X == sel.srSelection.Right)
				&& (sel.srSelection.Right >= sel.srSelection.Left))
			{
				vkPostKey = VK_BACK; pszKey = L"Backspace";
			}
			if (vkPostKey)
			{
				int iScanCode = 0; DWORD dwControlState = 0;
				UINT VK = ConEmuChord::GetVkByKeyName(pszKey, &iScanCode, &dwControlState);
				INPUT_RECORD r[2] = { { KEY_EVENT },{ KEY_EVENT } };
				TranslateKeyPress(VK, dwControlState, (VK == VK_BACK) ? (wchar_t)VK_BACK : 0, iScanCode, r, r + 1);

				bool lbPress = false;
				r[0].Event.KeyEvent.wRepeatCount = (sel.srSelection.Right - sel.srSelection.Left + 1);
				if (r[0].Event.KeyEvent.wRepeatCount <= GetTextWidth())
					lbPress = mp_RCon->PostConsoleEvent(r);
				if (lbPress)
					mp_RCon->PostConsoleEvent(r + 1);
			}
		}
		return ConEmuSkipHotKey;
	}

	return nullptr;
}

// pszChars may be nullptr
bool CRealBuffer::OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars)
{
	if (!mp_RCon->isConsoleReady())
	{
		DEBUGSTRKEY(L"CRealBuffer::OnKeyboard skipped - RCon is not ready yet");
		return false;
	}

	// Обработка Left/Right/Up/Down при выделении

	if (con.m_sel.dwFlags && messg == WM_KEYDOWN
			&& ((wParam == VK_ESCAPE) || (wParam == VK_RETURN)
				/*|| ((wParam == 'C' || wParam == VK_INSERT) && isPressed(VK_CONTROL)) -- moved to ProcessSelectionHotKey */
				|| (wParam == VK_LEFT) || (wParam == VK_RIGHT) || (wParam == VK_UP) || (wParam == VK_DOWN)
				|| (wParam == VK_HOME) || (wParam == VK_END))
	  )
	{
		if ((wParam == VK_ESCAPE) || (wParam == VK_RETURN) /*|| (wParam == 'C' || wParam == VK_INSERT)*/)
		{
			DoSelectionFinalize(wParam != VK_ESCAPE, cm_CopySel, wParam);
		}
		else
		{
			ChangeSelectionByKey(LODWORD(wParam), isPressed(VK_CONTROL), isPressed(VK_SHIFT));
		}

		return true;
	}

	if (messg == WM_KEYUP)
	{
		if (wParam == mp_RCon->mn_SelectModeSkipVk)
		{
			mp_RCon->mn_SelectModeSkipVk = 0; // игнорируем отпускание, поскольку нажатие было на копирование/отмену
			return true;
		}
		else if (gpSet->isFarGotoEditor && isKey(wParam, gpSet->GetHotkeyById(vkFarGotoEditorVk)))
		{
			if (GetLastTextRangeType() != etr_None)
			{
				if (StoreLastTextRange(etr_None))
					UpdateHyperlink();
			}
		}
	}

	if (messg == WM_KEYDOWN)
	{
		if (gpSet->isFarGotoEditor && isKey(wParam, gpSet->GetHotkeyById(vkFarGotoEditorVk)))
		{
			ProcessFarHyperlink(true);
		}

		if (mp_RCon->mn_SelectModeSkipVk)
			mp_RCon->mn_SelectModeSkipVk = 0; // при _нажатии_ любой другой клавиши - сбросить флажок (во избежание)
	}

	switch (m_Type)
	{
	case rbt_DumpScreen:
	case rbt_Alternative:
	case rbt_Selection:
	case rbt_Find:
		if (messg == WM_KEYUP)
		{
			if (wParam == VK_ESCAPE)
			{
				if ((m_Type == rbt_Find) && gpConEmu->mp_Find->mh_FindDlg && IsWindow(gpConEmu->mp_Find->mh_FindDlg))
				{
					break; // Пока висит диалог поиска - буфер не закрывать!
				}
				mp_RCon->SetActiveBuffer(rbt_Primary);
				return true;
			}
		}
		else if (messg == WM_KEYDOWN)
		{
			if ((wParam == VK_NEXT) || (wParam == VK_PRIOR))
			{
				gpConEmu->key_BufferScroll(false, wParam, mp_RCon);
				return true;
			}
			else if (((m_Type != rbt_Selection) && (m_Type != rbt_Find)) // в режиме выделения стрелки трогать не будем
					&& ((wParam == VK_UP) || (wParam == VK_DOWN)))
			{
				gpConEmu->key_BufferScroll(false, wParam, mp_RCon);
				return true;
			}
		}
		break;
	default:
		;
	}

	// Разрешить дальнейшую обработки (хоткеи и прочее)
	return false;
}

void CRealBuffer::OnKeysSending()
{
	_ASSERTE(m_Type == rbt_Primary);
	if (!con.TopLeft.isLocked())
		return;

	bool bReset = false;
	// Console API uses SHORT
	_ASSERTE((con.TopLeft.x==-1) || !HIWORD(con.TopLeft.x));
	_ASSERTE((con.TopLeft.y==-1) || !HIWORD(con.TopLeft.y));
	COORD crTopLeft = {(SHORT)(con.TopLeft.x>=0 ? con.TopLeft.x : 0), (SHORT)(con.TopLeft.y>=0 ? con.TopLeft.y : 0)};

	if (con.TopLeft.y == con.m_sbi.srWindow.Top)
		bReset = true;
	else if (!CoordInSmallRect(con.m_sbi.dwCursorPosition, con.srRealWindow))
		bReset = true;
	else if (mp_RCon->isFar())
		bReset = true;

	if (bReset)
		ResetTopLeft();
}

COORD CRealBuffer::GetDefaultNtvdmHeight()
{
	// 100627 - con.m_sbi.dwSize.Y более использовать некорректно ввиду "far/w"
	COORD cr16bit = {80,(SHORT)(con.m_sbi.srWindow.Bottom-con.m_sbi.srWindow.Top+1)};

	if (gpSet->ntvdmHeight && cr16bit.Y >= (int)gpSet->ntvdmHeight) cr16bit.Y = gpSet->ntvdmHeight;
	else if (cr16bit.Y>=50) cr16bit.Y = 50;
	else if (cr16bit.Y>=43) cr16bit.Y = 43;
	else if (cr16bit.Y>=28) cr16bit.Y = 28;
	else if (cr16bit.Y>=25) cr16bit.Y = 25;

	return cr16bit;
}

const CONSOLE_SCREEN_BUFFER_INFO* CRealBuffer::GetSBI()
{
	return &con.m_sbi;
}

//BOOL CRealBuffer::IsConsoleDataChanged()
//{
//	if (!this) return FALSE;
//
//	return con.bConsoleDataChanged;
//}

bool CRealBuffer::GetConsoleLine(/*[OUT]*/CRConDataGuard& data, int nLine, const wchar_t*& rpChar, int& rnLen, MSectionLock* pcsData /*= nullptr*/)
{
	ConsoleLinePtr line = {};
	if (!GetConsoleLine(nLine, data, line, pcsData))
		return false;
	rpChar = line.pChar;
	rnLen = line.nLen;
	return true;
}

bool CRealBuffer::GetConsoleLine(int nLine, /*[OUT]*/CRConDataGuard& data, ConsoleLinePtr& rpLine, MSectionLock* pcsData /*= nullptr*/)
{
	// Может быть уже заблокировано
	MSectionLock csData;
	if (pcsData)
	{
		if (!pcsData->isLocked())
			pcsData->Lock(&csCON);
	}
	else
	{
		csData.Lock(&csCON);
	}

	rpLine.clear();

	if (nLine < 0 || nLine >= GetWindowHeight())
	{
		_ASSERTE(nLine>=0 && nLine<GetWindowHeight());
		return FALSE;
	}

	if ((m_Type == rbt_DumpScreen) || (m_Type == rbt_Alternative) || (m_Type == rbt_Selection) || (m_Type == rbt_Find))
	{
		if (!dump.pszBlock1)
		{
			data.Release();
			return false;
		}

		if ((nLine < 0) || (nLine >= dump.crSize.Y))  // -V560
		{
			_ASSERTE((nLine >= 0) && (nLine < dump.crSize.Y));
			return false;
		}

		if (!CRConData::Allocate(data, mp_RCon, dump.pszBlock1, dump.pcaBlock1, con.m_sbi, dump.crSize))
			return false;
	}
	else
	{
		if (!GetData(data))
			return false;

		if ((nLine < 0) || (static_cast<UINT>(nLine) >= data->nHeight)
			|| ((size_t)((nLine + 1) * data->nWidth) > data->nMaxCells))
		{
			_ASSERTE((nLine >= 0) && (static_cast<UINT>(nLine) < data->nHeight) && ((size_t)((nLine + 1) * data->nWidth) > data->nMaxCells));
			return FALSE;
		}
	}

	if (!data->GetConsoleLine(nLine, rpLine))
		return false;

	return true;
}

void CRealBuffer::PrepareColorTable(const AppSettings* pApp, CharAttr** pcaTableExt, CharAttr** pcaTableOrg)
{
	bool bExtendFonts = false;
	BYTE nFontNormalColor = 0;
	BYTE nFontBoldColor = 0;
	BYTE nFontItalicColor = 0;

	if (pApp)
	{
		// Don't query application, use passed by argument only
		// pApp = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());

		bExtendFonts = pApp->ExtendFonts();
		nFontNormalColor = pApp->FontNormalColor();
		nFontBoldColor = pApp->FontBoldColor();
		nFontItalicColor = pApp->FontItalicColor();
	}

	// Optimized, palettes are recalculated only when changes are detected
	mp_RCon->mp_Palette->UpdateColorTable(mp_RCon->mp_VCon->GetColors()/*[16+]*/,
		gpSet->isVividColors,
		bExtendFonts, nFontNormalColor, nFontBoldColor, nFontItalicColor);

	if (pcaTableOrg)
		*pcaTableOrg = mp_RCon->mp_Palette->m_TableOrg;
	if (pcaTableExt)
		*pcaTableExt = bExtendFonts ? mp_RCon->mp_Palette->m_TableExt : mp_RCon->mp_Palette->m_TableOrg;
}

void CRealBuffer::ResetConData()
{
	con.mb_ConDataValid = false;
	con.nDynamicHeight = 0;
}

// nWidth и nHeight это размеры, которые хочет получить VCon (оно могло еще не среагировать на изменения?
void CRealBuffer::GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, ConEmuTextRange& etr)
{
	if (!this) return;

	//DWORD cbDstBufSize = nWidth * nHeight * 2;
	DWORD cwDstBufSize = nWidth * nHeight;
	_ASSERTE(nWidth != 0 && nHeight != 0);
	bool bDataValid = false;

	if (nWidth == 0 || nHeight == 0)
		return;

	#ifdef _DEBUG
	if (mp_RCon->mb_DebugLocked)
	{
		return;
	}
	#endif

	con.bConsoleDataChanged = FALSE;

	// формирование умолчательных цветов, по атрибутам консоли
	//TODO("В принципе, это можно делать не всегда, а только при изменениях");
	bool lbIsFar = (mp_RCon->GetFarPID() != 0);
	// Don't highlight while selection is present
	bool lbAllowHilightFileLine = (con.m_sel.dwFlags == 0) && mp_RCon->IsFarHyperlinkAllowed(false);
	if (!lbAllowHilightFileLine && (con.etr.etrLast != etr_None))
		StoreLastTextRange(etr_None);
	WARNING("lbIsFar - хорошо бы заменить на привязку к конкретным приложениям?");
	const AppSettings* pApp = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());
	_ASSERTE(pApp!=nullptr);
	// 120331 - зачем ограничивать настройку доп.цветов?
	bool bExtendFonts = pApp->ExtendFonts();
	bool lbFade = mp_RCon->mp_VCon->isFade;
	CharAttr *lcaTableExt, *lcaTableOrg, *lcaTable;

	PrepareColorTable(pApp, &lcaTableExt, &lcaTableOrg);

	lcaTable = lcaTableOrg;

	// NonExclusive lock (need to ensure that buffers will not be recreated during processing)
	MSectionLock csData; csData.Lock(&csCON);
	MSetter lInGetData(&con.nInGetConsoleData);

	con.LastStartReadBufferTick = GetTickCount();

	HEAPVAL
	wchar_t wSetChar = L' ';
	CharAttr lcaDef;
	BYTE nDefTextAttr = MAKECONCOLOR(mp_RCon->GetDefaultTextColorIdx(), mp_RCon->GetDefaultBackColorIdx());
	_ASSERTE(nDefTextAttr<countof(mp_RCon->mp_Palette->m_TableOrg));
	lcaDef = lcaTable[nDefTextAttr]; // LtGray on Black

	#ifdef _DEBUG
	// Fill with Red '≈' on ‘errors’
	wSetChar = (wchar_t)0x2248;
	lcaDef = lcaTable[MAKECONCOLOR(12,CONBACKCOLOR(nDefTextAttr))];
	#endif

	int nXMax = 0, nYMax = 0;
	int nX = 0, nY = 0;
	wchar_t  *pszDst = pChar;
	CharAttr *pcaDst = pAttr;

	if ((m_Type == rbt_DumpScreen) || (m_Type == rbt_Alternative) || (m_Type == rbt_Selection) || (m_Type == rbt_Find))
	{
		bDataValid = true;
		nXMax = std::min<int>(nWidth, dump.crSize.X);
		int nFirstLine = con.m_sbi.srWindow.Top;
		nYMax = std::min<int>(nHeight, (dump.crSize.Y - nFirstLine));
		wchar_t* pszSrc = dump.pszBlock1 + (nFirstLine * dump.crSize.X) + con.m_sbi.srWindow.Left;
		CharAttr* pcaSrc = dump.pcaBlock1 + (nFirstLine * dump.crSize.X) + con.m_sbi.srWindow.Left;
		for (int Y = 0; Y < nYMax; Y++)
		{
			wmemmove(pszDst, pszSrc, nXMax);
			memmove(pcaDst, pcaSrc, nXMax*sizeof(*pcaDst));

			// Иначе детектор глючит
			TODO("Избавиться от поля Flags. Вообще.");
			int iTail = nXMax;
			for (int i = 0; (i < nXMax) && (iTail-- > 0); i++)
			{
				pcaDst[i].Flags = 0;
				bool bPair = (iTail > 0);
				int wwch = ucs32_from_wchar(pszDst + i, bPair);
				_ASSERTE(wwch >= 0);

				switch (get_wcwidth(wwch))
				{
				case 0:
					pcaDst[i].Flags2 |= CharAttr2_Combining;
					break;
				case 2:
					pcaDst[i].Flags2 |= CharAttr2_DoubleSpaced;
					break;
				}
			}

			if (nWidth > nXMax)
			{
				wmemset(pszDst+nXMax, wSetChar, nWidth-nXMax);
				int iTail = nWidth;
				for (int i = nXMax; (i < nWidth) && (iTail-- > 0); i++)
				{
					pcaDst[i] = lcaDef;
					bool bPair = (iTail > 0);
					int wwch = ucs32_from_wchar(pszDst + i, bPair);
					_ASSERTE(wwch >= 0);

					switch (get_wcwidth(wwch))
					{
					case 0:
						pcaDst[i].Flags2 |= CharAttr2_Combining;
						break;
					case 2:
						pcaDst[i].Flags2 |= CharAttr2_DoubleSpaced;
						break;
					}
				}
			}

			pszDst += nWidth;
			pcaDst += nWidth;
			pszSrc += dump.crSize.X;
			pcaSrc += dump.crSize.X;
		}
		#ifdef _DEBUG
		*pszDst = 0; // для отладчика
		#endif
		// Очистка мусора за пределами идет ниже
	}
	else
	{
		CRConDataGuard data;

		if (!GetData(data)
			|| !con.mb_ConDataValid
			|| !data.isValid())
		{
			// Return totally clear buffer, no need to erase "bottom"
			nYMax = nHeight;
			// Under debug, show "warning" char if only buffer was not initialized
			wchar_t wc = (data.isValid()) ? L' ' : wSetChar;
			wmemset(pChar, wc, cwDstBufSize);
			// And Attributes
			for (DWORD i = 0; i < cwDstBufSize; i++)
				pAttr[i] = lcaDef;
			// Advance pointer to the end of the buffer
			_ASSERTE(con.nConBufCells >= ((size_t)nWidth*(size_t)nHeight + 1)); // It's expected to be ASCIIZ
			pszDst = pChar + nWidth*nHeight;
		}
		else
		{
			bDataValid = true;

			if (lbAllowHilightFileLine)
			{
				// Если мышь сместилась - нужно посчитать
				// Даже если мышь не двигалась - текст мог измениться.
				/*if ((con.etr.mcr_FileLineStart.X == con.etr.mcr_FileLineEnd.X)
				|| (con.etr.mcr_FileLineStart.Y != mcr_LastMousePos.Y)
				|| (con.etr.mcr_FileLineStart.X > mcr_LastMousePos.X || con.etr.mcr_FileLineEnd.X < mcr_LastMousePos.X))*/
				if ((mp_RCon->mp_ABuf == this) && gpConEmu->isMeForeground())
				{
					ProcessFarHyperlink(false);
				}
			}

			// And the data
			nYMax = data->GetConsoleData(pChar, pAttr, nWidth, nHeight, wSetChar, lcaDef,
				lcaTable, lcaTableExt, lbFade, bExtendFonts);
			pszDst = pChar + nWidth*nYMax;
		}
	} // rbt_Primary

	// Apply control characters substitutes for drawing only
	{
		const size_t checkCharsCount = nYMax * nWidth;
		for (size_t i = 0; i < checkCharsCount; ++i)
		{
			const auto charIndex = static_cast<WORD>(pChar[i]);
			if (charIndex < countof(gszAnalogues))
			{
				pChar[i] = gszAnalogues[charIndex];
			}
		}
	}

	// Clean the bottom, if requested height is greater than real one
	for (nY = nYMax; nY < nHeight; nY++)
	{
		wmemset(pszDst, wSetChar, nWidth);
		pszDst += nWidth;

		//wmemset((wchar_t*)pcaDst, wSetAttr, nWidth);
		for (nX = 0; nX < nWidth; nX++)
		{
			pcaDst[nX] = lcaDef;
		}

		pcaDst += nWidth;
	}

	// Чтобы безопасно использовать строковые функции - гарантированно делаем ASCIIZ. Хотя pChars может и \0 содержать?
	*pszDst = 0;

	// Update hyperlinks and other underlines
	if (lbAllowHilightFileLine && (con.etr.etrLast != etr_None))
	{
		etr.mcr_FileLineStart = BufferToScreen(con.etr.mcr_FileLineStart);
		etr.mcr_FileLineEnd = BufferToScreen(con.etr.mcr_FileLineEnd);
		etr.etrLast = con.etr.etrLast;
	}
	else
	{
		etr.etrLast = etr_None;
	}

	if (bDataValid)
	{
		// Подготовить данные для Transparent
		// обнаружение диалогов нужно только при включенной прозрачности,
		// или при пропорциональном шрифте
		// Даже если НЕ (gpSet->NeedDialogDetect()) - нужно сбросить количество прямоугольников.
		PrepareTransparent(pChar, pAttr, nWidth, nHeight);

		if (mn_LastRgnFlags != m_Rgn.GetFlags())
		{
			// Попытаться найти панели и обновить флаги
			if (m_Type == rbt_Primary)
				FindPanels();

			WARNING("Не думаю, что это хорошее место для обновления мышиного курсора");
			// Обновить мышиный курсор
			if (mp_RCon->isActive(false) && (this == mp_RCon->mp_ABuf))
			{
				PostMessage(mp_RCon->GetView(), WM_SETCURSOR, -1, -1);
			}

			mn_LastRgnFlags = m_Rgn.GetFlags();
		}

		if (con.m_sel.dwFlags)
		{
			bool lbStreamMode = (con.m_sel.dwFlags & CONSOLE_TEXT_SELECTION) == CONSOLE_TEXT_SELECTION;
			// srSelection in Absolute coordinates now!
			// Поскольку здесь нас интересует только отображение - можно поступить просто
			COORD crStart = BufferToScreen(MakeCoord(con.m_sel.srSelection.Left, con.m_sel.srSelection.Top), false);
			COORD crEnd = BufferToScreen(MakeCoord(con.m_sel.srSelection.Right, con.m_sel.srSelection.Bottom), false);

			bool bAboveScreen = (con.m_sel.srSelection.Top < con.m_sbi.srWindow.Top);
			bool bBelowScreen = (con.m_sel.srSelection.Bottom > con.m_sbi.srWindow.Bottom);

			SMALL_RECT rc = {crStart.X, crStart.Y, crEnd.X, crEnd.Y};
			_ASSERTE(rc.Left >= 0 && rc.Right < nWidth);
			MinMax(rc.Left, 0, nWidth-1); MinMax(rc.Right, 0, nWidth-1);

			// для прямоугольника выделения сбрасываем прозрачность и ставим стандартный цвет выделения (lcaSel)
			//CharAttr lcaSel = lcaTable[gpSet->isCTSColorIndex]; // Black on LtGray
			_ASSERTE((gpSet->isCTSColorIndex >= 0x00) && (gpSet->isCTSColorIndex <= 0xFF));
			BYTE nForeIdx = CONFORECOLOR(gpSet->isCTSColorIndex);
			BYTE nBackIdx = CONBACKCOLOR(gpSet->isCTSColorIndex);
			const CharAttr& lcaSel = mp_RCon->mp_Palette->m_TableOrg[gpSet->isCTSColorIndex];
			COLORREF crForeColor = lcaSel.crForeColor;
			COLORREF crBackColor = lcaSel.crBackColor;
			int nX1, nX2;


			for (nY = std::max<int>(rc.Top, 0); nY <= std::min<int>(rc.Bottom, nHeight-1); nY++)
			{
				if (!lbStreamMode)
				{
					// Block mode
					nX1 = rc.Left; nX2 = rc.Right;
				}
				else
				{
					nX1 = (nY == rc.Top && !bAboveScreen) ? rc.Left : 0;
					nX2 = (nY == rc.Bottom && !bBelowScreen) ? rc.Right : (nWidth-1);
				}

				pcaDst = pAttr + nWidth*nY + nX1;

				for (nX = nX1; nX <= nX2; nX++, pcaDst++)
				{
					//pcaDst[nX] = lcaSel; -- чтобы не сбрасывать флаги рамок и диалогов - ставим по полям
					pcaDst->crForeColor = pcaDst->crOrigForeColor = crForeColor;
					pcaDst->crBackColor = pcaDst->crOrigBackColor = crBackColor;
					pcaDst->nForeIdx = nForeIdx;
					pcaDst->nBackIdx = nBackIdx;
					#if 0
					pcaDst->bTransparent = FALSE;
					#else
					pcaDst->Flags &= ~CharAttr_Transparent;
					#endif
					pcaDst->nFontIndex = 0;
				}
			}

			//} else {
			//	int nX1, nX2;
			//	for (nY = rc.Top; nY <= rc.Bottom; nY++) {
			//		pnDst = pAttr + nWidth*nY;
			//		nX1 = (nY == rc.Top) ? rc.Left : 0;
			//		nX2 = (nY == rc.Bottom) ? rc.Right : (nWidth-1);
			//		for (nX = nX1; nX <= nX2; nX++) {
			//			pnDst[nX] = lcaSel;
			//		}
			//	}
			//}
		}
	}

	// Если требуется показать "статус" - принудительно перебиваем первую видимую строку возвращаемого буфера
	if (mp_RCon->m_ConStatus.szText[0] && (mp_RCon->m_ConStatus.Options & CRealConsole::cso_Critical)
		&& (!gpSet->isStatusBarShow
			|| (mp_RCon->isActive(true) && !mp_RCon->isActive(false))))
	{
		int nLen = _tcslen(mp_RCon->m_ConStatus.szText);
		wmemcpy(pChar, mp_RCon->m_ConStatus.szText, nLen);

		if (nWidth > nLen)
			wmemset(pChar+nLen, L' ', nWidth-nLen);

		//wmemset((wchar_t*)pAttr, 0x47, nWidth);
		CharAttr lca = lcaTableExt[7];

		for(int i = 0; i < nWidth; i++)
			pAttr[i] = lca;
	}

	con.LastEndReadBufferTick = GetTickCount();

	//FIN
	HEAPVAL
	csData.Unlock();
	return;
}

DWORD_PTR CRealBuffer::GetKeybLayout()
{
	return con.dwKeybLayout;
}

void CRealBuffer::SetKeybLayout(DWORD_PTR anNewKeyboardLayout)
{
	con.dwKeybLayout = anNewKeyboardLayout;
}

int CRealBuffer::GetStatusLineCount(int nLeftPanelEdge)
{
	if (!this)
		return 0;

	if (!mp_RCon->isFar())
		return 0;

	CRConDataGuard data;
	if (!GetData(data) || !data->nWidth)
		return 0;

	int nBottom, nLeft;
	if (nLeftPanelEdge > mr_LeftPanelFull.left)
	{
		nBottom = mr_RightPanelFull.bottom;
		nLeft = mr_RightPanelFull.left;
	}
	else
	{
		nBottom = mr_LeftPanelFull.bottom;
		nLeft = mr_LeftPanelFull.left;
	}
	if (nBottom < 5)
		return 0; // минимальная высота панели

	size_t maxCells = data->nWidth * data->nHeight;

	for (int i = 2; i <= 11 && i < nBottom; i++)
	{
		size_t idx = data->nWidth*(nBottom-i)+nLeft;
		if (idx >= maxCells)
		{
			break;
		}
		if (data->pConChar[idx] == ucBoxDblVertSinglRight)
		{
			return (i - 1);
		}
	}

	return 0;
}

void CRealBuffer::PrepareTransparent(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight)
{
	//if (!mp_ConsoleInfo)
	if (!pChar || !pAttr)
		return;

	TODO("Хорошо бы m_FarInfo тоже в дамп скидывать.");
	CEFAR_INFO_MAPPING* pFI = (CEFAR_INFO_MAPPING*)malloc(sizeof(CEFAR_INFO_MAPPING));
	if (!pFI)
		return;
	SMALL_RECT rcFarRect = {};
	const CONSOLE_SCREEN_BUFFER_INFO* pSbi = GetSBI();

	if ((m_Type == rbt_Primary) && mp_RCon->GetFarPID())
	{
		*pFI = mp_RCon->m_FarInfo;

		// На (mp_RCon->isViewer() || mp_RCon->isEditor()) ориентироваться
		// нельзя, т.к. CheckFarStates еще не был вызван
		bool bViewerOrEditor = false;
		if (mp_RCon->GetFarPID(true))
		{
			int nTabType = mp_RCon->GetActiveTabType();
			bViewerOrEditor = ((nTabType & 0xFF) == 2 || (nTabType & 0xFF) == 3);

			if (pSbi)
			{
				if (pFI->bBufferSupport)
				{
					rcFarRect.Left = 0;
					rcFarRect.Right = GetTextWidth() - 1;
					rcFarRect.Bottom = pSbi->dwSize.Y - 1;
					rcFarRect.Top =	rcFarRect.Bottom - GetTextHeight() + 1;
				}
			}
		}

		if (!bViewerOrEditor)
		{
			WARNING("Неправильно это, ведь FindPanels еще не был вызван");

			if (mb_LeftPanel)
			{
				pFI->bFarLeftPanel = true;
				pFI->FarLeftPanel.PanelRect = mr_LeftPanelFull;
			}
			else
			{
				pFI->bFarLeftPanel = false;
			}

			if (mb_RightPanel)
			{
				pFI->bFarRightPanel = true;
				pFI->FarRightPanel.PanelRect = mr_RightPanelFull;
			}
			else
			{
				pFI->bFarRightPanel = false;
			}

			pFI->bViewerOrEditor = false;
		}
		else
		{
			pFI->bViewerOrEditor = true;
			pFI->bFarLeftPanel = false;
			pFI->bFarRightPanel = false;
		}

		//if (!FI.bFarLeftPanel && !FI.bFarRightPanel)
		//{
		//	// Если нет панелей - это может быть вьювер/редактор
		//	FI.bViewerOrEditor = (mp_RCon->isViewer() || mp_RCon->isEditor());
		//}
	}
	else
	{
		memset(pFI, 0, sizeof(*pFI));
		TODO("Загружать CEFAR_INFO_MAPPING из дампа");
	}

	m_Rgn.SetNeedTransparency(gpSet->isUserScreenTransparent);
	m_Rgn.SetFarRect(&rcFarRect);
	TODO("При загрузке дампа хорошо бы из него и палитру фара доставать/отдавать");
	const bool bFarUserScreen = mp_RCon->isFar() && (isPressed(VK_CONTROL) && isPressed(VK_SHIFT) && isPressed(VK_MENU));
	m_Rgn.PrepareTransparent(pFI, mp_RCon->mp_VCon->GetColors(), pSbi, pChar, pAttr, nWidth, nHeight, bFarUserScreen);

	free(pFI);

	#ifdef _DEBUG
	int nCount = m_Rgn.GetDetectedDialogs(0,nullptr,nullptr);

	if (nCount == 1)
	{
		nCount = 1;
	}
	#endif
}

// Найти панели, обновить mp_RCon->mn_ConsoleProgress
void CRealBuffer::FindPanels()
{
	TODO("Положение панелей можно бы узнавать из плагина");
	WARNING("!!! Нужно еще сохранять флажок 'Меню сейчас показано'");

	#ifdef _DEBUG
	if (mp_RCon->mb_DebugLocked)
		return;
	WARNING("this==mp_RCon->mp_RBuf ?");
	#endif

	RECT rLeftPanel = MakeRect(-1,-1);
	RECT rLeftPanelFull = rLeftPanel;
	RECT rRightPanel = rLeftPanel;
	RECT rRightPanelFull = rLeftPanel;
	bool bLeftPanel = false;
	bool bRightPanel = false;
	bool bMayBePanels = false;
	bool lbNeedUpdateSizes = false;
	bool lbPanelsChanged = false;
	short nLastProgress = mp_RCon->m_Progress.ConsoleProgress;
	short nNewProgress = -1;
	/*
	Имеем облом. При ресайзе панелей CtrlLeft/CtrlRight иногда сервер считывает
	содержимое консоли ДО окончания вывода в нее новой информации. В итоге верхняя
	часть считанного не соответствует нижней, что влечет ошибку
	определения панелей в CRealConsole::FindPanels - ошибочно считает, что
	Левая панель - полноэкранная :(
	*/
	WARNING("Добавить проверки по всем граням панелей, чтобы на них не было некорректных символов");
	// То есть на грани панели не было других диалогов (вертикальных/угловых бордюров поверх горизонтальной части панели)

	// функция проверяет (mn_ProgramStatus & CES_FARACTIVE) и т.п.
	if (mp_RCon->isFar())
	{
		if (mp_RCon->isFar(TRUE) && mp_RCon->m_FarInfo.cbSize)
		{
			if (mp_RCon->m__FarInfo.Ptr() && mp_RCon->m__FarInfo.Ptr()->nFarInfoIdx != mp_RCon->m_FarInfo.nFarInfoIdx)
			{
				mp_RCon->m__FarInfo.GetTo(&mp_RCon->m_FarInfo, sizeof(mp_RCon->m_FarInfo));
			}
		}

		// Если активен редактор или вьювер (или диалоги, копирование, и т.п.) - искать бессмысленно
		if ((mp_RCon->mn_FarStatus & CES_NOTPANELFLAGS) == 0)
			bMayBePanels = TRUE; // только если нет
	}

	if (bMayBePanels && con.nTextHeight >= MIN_CON_HEIGHT && con.nTextWidth >= MIN_CON_WIDTH)
	{
		CRConDataGuard data;
		if (GetData(data))
		{
			data->FindPanels(bLeftPanel, rLeftPanel, rLeftPanelFull, bRightPanel, rRightPanel, rRightPanelFull);
		}
	}

	#ifdef _DEBUG
	if (bLeftPanel && !bRightPanel && rLeftPanelFull.right > 120)
	{
		bLeftPanel = bLeftPanel;  // -V570
	}
	#endif

	if (mp_RCon->isActive(false))
		lbNeedUpdateSizes = (memcmp(&mr_LeftPanel,&rLeftPanel,sizeof(mr_LeftPanel)) || memcmp(&mr_RightPanel,&rRightPanel,sizeof(mr_RightPanel)));

	lbPanelsChanged = lbNeedUpdateSizes || (mb_LeftPanel != bLeftPanel) || (mb_RightPanel != bRightPanel)
	                  || ((bLeftPanel || bRightPanel) && ((mp_RCon->mn_FarStatus & CES_FILEPANEL) == 0));
	mr_LeftPanel = rLeftPanel; mr_LeftPanelFull = rLeftPanelFull; mb_LeftPanel = bLeftPanel;
	mr_RightPanel = rRightPanel; mr_RightPanelFull = rRightPanelFull; mb_RightPanel = bRightPanel;

	if (bRightPanel || bLeftPanel)
		bMayBePanels = TRUE;
	else
		bMayBePanels = FALSE;

	nNewProgress = -1;

	// mn_ProgramStatus не катит. Хочется подхватывать прогресс из плагина Update, а там как раз CES_FARACTIVE
	//if (!abResetOnly && (mn_ProgramStatus & CES_FARACTIVE) == 0) {
	if (!bMayBePanels && (mp_RCon->mn_FarStatus & CES_NOTPANELFLAGS) == 0)
	{
		// Обработка прогресса NeroCMD и пр. консольных программ
		// Если курсор находится в видимой области
		int nY = con.m_sbi.dwCursorPosition.Y - con.m_sbi.srWindow.Top;

		if (/*con.m_ci.bVisible && con.m_ci.dwSize -- Update прячет курсор?
			&&*/ con.m_sbi.dwCursorPosition.X >= 0 && con.m_sbi.dwCursorPosition.X < con.nTextWidth
		    && nY >= 0 && nY < con.nTextHeight)
		{
			// NeroCMD, wget, chkdsk and other utilities printing operation progress (percentage) in the console output
			CRConDataGuard data;
			if (GetData(data))
			{
				_ASSERTE(nY >= 0);
				nNewProgress = data->CheckProgressInConsole((UINT)nY);
			}
		}
	}

	if (mp_RCon->m_Progress.ConsoleProgress != nNewProgress || nLastProgress != nNewProgress)
	{
		// Запомнить, что получили из консоли
		mp_RCon->setConsoleProgress(nNewProgress);
		// Показать прогресс в заголовке
		mp_RCon->mb_ForceTitleChanged = TRUE;
	}

	if (lbPanelsChanged)
	{
		// Нужно вызвать (возможно повторно), чтобы обновить флаги состояний
		mp_RCon->CheckFarStates();
	}

	if (lbNeedUpdateSizes)
		gpConEmu->UpdateSizes();
}

bool CRealBuffer::isSelectionAllowed()
{
	if (!this)
		return false;

	#ifdef _DEBUG
	// Зачем звать эту функцию из фоновых потоков?
	if (!isMainThread())
	{
		int nDbg = -1;
	}
	#endif

	// gh-871
	if (!isDataValid())
		return false; // if there are not data in console?

	if (con.m_sel.dwFlags != 0)
		return true; // Если выделение было запущено через меню

	// Если УЖЕ удерживается модификатор - то не требуется проверять "Intelligent selection"
	if (gpConEmu->isSelectionModifierPressed(true))
		return true; // force selection mode

	if (!gpSet->isCTSIntelligent)
		return false; // Mouse selection was disabled
	LPCWSTR pszPrcName = mp_RCon->GetActiveProcessName();
	if (!pszPrcName)
		return true; // Starting or terminated state? Allow selection
	LPCWSTR pszExcl = gpSet->GetIntelligentExceptionsMSZ();
	// Check exclusions
	if (pszExcl)
	{
		while (*pszExcl)
		{
			LPCWSTR pszCompare = pszExcl;
			pszExcl += _tcslen(pszExcl)+1;

			if (!CompareProcessNames(pszCompare, pszPrcName))
			{
				continue;
			}

			if (CompareProcessNames(pszCompare, L"far"))
			{
				// Tricky a little
				// * in Far Viewer - click&drag is used for content scrolling
				// * in Far Editor - used for text selection
				// * in Far Panels - used for file drag&drop (ConEmu's or Far internal)
				// * UserScreen (panels are hidden) - use mouse for selection
				// To send mouse to console always - set exceptions to "far.exe" instead of "far"
				if (mp_RCon->isFar())
				{
					if (mp_RCon->isViewer() || mp_RCon->isEditor() || mp_RCon->isFilePanel(true, true))
					{
						return false;
					}
					else
					{
						DWORD nDlgFlags = m_Rgn.GetFlags();
						int nDialogs = m_Rgn.GetDetectedDialogs(3,nullptr,nullptr);
						if ((nDialogs > 0) && !((nDialogs == 1) && (nDlgFlags == FR_MENUBAR)))
							return false; // Any dialog on screen? Don't select
						return true;
					}
				}
				else
				{
					// GetActiveProcessName() may have lags due to complexity
					// But isFar() always returns actual state
					// So we may get here if 'Far' has just become inactive (command execution)
					continue;
				}
			}
			else if (CompareProcessNames(pszCompare, L"vim"))
			{
				// gh#399: Only official builds support our xterm mode
				TermEmulationType term = mp_RCon->GetTermType();
				// So, use mouse for our self selection, if vim did not request xterm
				return (term == te_win32);
			}

			// This app in the restricted list
			// Seems like it uses mouse for internal selection, dragging and so on...
			return false;
		}
	}

	return true;
}

bool CRealBuffer::isSelectionPresent()
{
	if (!this)
		return false;

	return (con.m_sel.dwFlags != 0);
}

bool CRealBuffer::isMouseSelectionPresent()
{
	if (!this)
		return false;

	return ((con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION) != 0);
}

// Allows to start selection (by Shift+LClick or Shift+Arrow)
// and after that extend it by Shift+LClick to clicked postion
// Inspired by https://superuser.com/q/1218047/139371
bool CRealBuffer::isMouseClickExtension()
{
	if (!this || !con.m_sel.dwFlags)
		return false;

	// The end of the block already marked
	if ((con.m_sel.dwFlags & CONSOLE_EXPANDED))
		return false;

	DWORD nPressed = gpConEmu->isSelectionModifierPressed(false);
	DWORD nMask = (CONSOLE_TEXT_SELECTION|CONSOLE_BLOCK_SELECTION);
	if (!(nPressed & nMask) || ((nPressed & nMask) != (con.m_sel.dwFlags & nMask)))
		return false;

	return true;
}

bool CRealBuffer::isMouseInsideSelection(int x, int y)
{
	if (!this || !isSelectionPresent())
		return false;

	COORD crScreen = mp_RCon->mp_VCon->ClientToConsole(x, y);
	MinMax(crScreen.X, 0, TextWidth() - 1);
	MinMax(crScreen.Y, 0, TextHeight() - 1);
	COORD cr = ScreenToBuffer(crScreen);

	bool bInside = false;
	if (isStreamSelection())
	{
		// Complicated rules
		const SMALL_RECT& sr = con.m_sel.srSelection;
		if ((((cr.Y == sr.Top) && (cr.X >= sr.Left))
			|| (cr.Y > sr.Top))
			&& (((cr.Y == sr.Bottom) && (cr.X <= sr.Right))
				|| (cr.Y < sr.Bottom))
			)
		{
			bInside = true;
		}
	}
	else // block selection
	{
		bInside = CoordInSmallRect(cr, con.m_sel.srSelection);
	}

	return bInside;
}

bool CRealBuffer::GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel)
{
	if (!this)
		return false;

	#if 0
	// If it was already started - let it be... Don't look at settings
	if (!isSelectionAllowed())
		return false;
	#endif

	if (sel)
	{
		*sel = con.m_sel;
	}

	return isSelectionPresent();
}

void CRealBuffer::ConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci)
{
	if (!this || !ci) return;

	*ci = con.m_ci;

	if (mp_RCon->m_TermCursor.CursorShape != tcs_Default)
	{
		if (mp_RCon->m_TermCursor.CursorShape == tcs_BlockBlink
			|| mp_RCon->m_TermCursor.CursorShape == tcs_BlockSteady)
			ci->dwSize = 100;
		else
			ci->dwSize = 15;
		// ci->bVisible = TRUE; // Use visibility from RealConsole?
	}

	// Если сейчас выделяется текст мышкой (ConEmu internal)
	// то курсор нужно переключить в половину знакоместа!
	if (isSelectionPresent())
	{
		TODO("А может ну его, эту половину знакоместа? Или по настройке?");
		if (ci->dwSize < 50)
			ci->dwSize = 50;
	}
	else
	{
		const AppSettings* pApp = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());
		bool bActive = mp_RCon->isInFocus();
		if (pApp->CursorIgnoreSize(bActive))
			ci->dwSize = pApp->CursorFixedSize(bActive);
	}
}

void CRealBuffer::GetCursorInfo(COORD* pcr, CONSOLE_CURSOR_INFO* pci)
{
	if (pcr)
	{
		*pcr = con.m_sbi.dwCursorPosition;
	}

	if (pci)
	{
		//*pci = con.m_ci;
		ConsoleCursorInfo(pci);
	}
}

bool CRealBuffer::isCursorVisible()
{
	// Remove cursor from screen while selecting text with mouse
	if (isSelectionPresent() && (con.m_sel.dwFlags & CONSOLE_MOUSE_SELECTION))
		return false;

	// Console cursor is hidden?
	if (!con.m_ci.bVisible && !mp_RCon->mp_VCon->GetCursor(false).Invisible)
		return false;

	return true;
}

void CRealBuffer::ConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* psbi, SMALL_RECT* psrRealWindow /*= nullptr*/, TOPLEFTCOORD* pTopLeft /*= nullptr*/)
{
	if (!this) return;

	if (psbi)
	{
		*psbi = con.m_sbi;
	}

	if (psrRealWindow)
	{
		*psrRealWindow = con.srRealWindow;
	}

	if (pTopLeft)
	{
		*pTopLeft = con.TopLeft;
	}

	// В режиме выделения - положение курсора ставим сами
	if (con.m_sel.dwFlags && psbi)
	{
		ConsoleCursorPos(&(psbi->dwCursorPosition));
	}
}

void CRealBuffer::QueryCellInfo(wchar_t* pszInfo, int cchMax)
{
	if (!pszInfo || cchMax <= 0 || !this || !isDataValid())
	{
		if (pszInfo) *pszInfo = 0;
		return;
	}

	COORD crPos = {};
	CRConDataGuard data;
	ConsoleLinePtr line;
	MSectionLock csLock;
	ConsoleCursorPos(&crPos);
	COORD lcrScr = BufferToScreen(crPos);
	if (!GetConsoleLine(lcrScr.Y, data, line, &csLock))
	{
		if (pszInfo) *pszInfo = 0;
		return;
	}

	wchar_t szInfo[256];
	wchar_t wch = (line.pChar && line.nLen > lcrScr.X && lcrScr.X >= 0) ? line.pChar[lcrScr.X] : 0xFFFF;
	WORD attr = (line.pAttr && line.nLen > lcrScr.X && lcrScr.X >= 0) ? line.pAttr[lcrScr.X] : 0;
	// #TODO Attributes from RealConsole (WORD)
	msprintf(szInfo, countof(szInfo), L"U+%02X A:%02X", wch, attr);
	lstrcpyn(pszInfo, szInfo, cchMax);
}

void CRealBuffer::ConsoleCursorPos(COORD *pcr)
{
	if (!this || !pcr) return;

	if (con.m_sel.dwFlags)
	{
		// В режиме выделения - положение курсора ставим сами
		// con.m_sel.srSelection уже содержит "абсолютные" координаты

		if (con.m_sel.dwSelectionAnchor.X == con.m_sel.srSelection.Left)
			pcr->X = con.m_sel.srSelection.Right;
		else
			pcr->X = con.m_sel.srSelection.Left;

		if (con.m_sel.dwSelectionAnchor.Y == con.m_sel.srSelection.Top)
			pcr->Y = con.m_sel.srSelection.Bottom; // + con.nTopVisibleLine;
		else
			pcr->Y = con.m_sel.srSelection.Top; // + con.nTopVisibleLine;
	}
	else
	{
		*pcr = con.m_sbi.dwCursorPosition;
	}
}

void CRealBuffer::ResetBuffer()
{
	AssertThis();
	con.m_ci.bVisible = TRUE;
	con.m_ci.dwSize = 15;
	con.m_sbi.dwCursorPosition = MakeCoord(0,con.m_sbi.srWindow.Top);

	CRConDataGuard data;
	if (GetData(data))
	{
		MSectionLock sc; sc.Lock(&csCON);
		DWORD OneBufferSize = data->nWidth * data->nHeight;
		memset(data->pConChar, 0, OneBufferSize * sizeof(*data->pConChar));
		memset(data->pConAttr, 0, OneBufferSize * sizeof(*data->pConAttr));
	}
}

// Преобразовать абсолютные координаты консоли в координаты нашего буфера
// (вычесть номер верхней видимой строки и скорректировать видимую высоту)
bool CRealBuffer::ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr)
{
	if (!this) return false;

	*prcScr = rcCon;

	int iTop = GetBufferPosY();
	if (con.bBufferHeight && iTop)
	{
		prcScr->top -= iTop;
		prcScr->bottom -= iTop;
	}

	bool lbRectOK = true;

	if (prcScr->left == 0 && prcScr->right >= con.nTextWidth)
		prcScr->right = con.nTextWidth - 1;

	if (prcScr->left)
	{
		if (prcScr->left >= con.nTextWidth)
			return false;

		if (prcScr->right >= con.nTextWidth)
			prcScr->right = con.nTextWidth - 1;
	}

	if (prcScr->bottom < 0)
	{
		lbRectOK = false; // полностью уехал за границу вверх
	}
	else if (prcScr->top >= con.nTextHeight)
	{
		lbRectOK = false; // полностью уехал за границу вниз
	}
	else
	{
		// Скорректировать по видимому прямоугольнику
		if (prcScr->top < 0)
			prcScr->top = 0;

		if (prcScr->bottom >= con.nTextHeight)
			prcScr->bottom = con.nTextHeight - 1;

		lbRectOK = (prcScr->bottom > prcScr->top);
	}

	return lbRectOK;
}

DWORD CRealBuffer::GetConsoleCP()
{
	return con.m_dwConsoleCP;
}

DWORD CRealBuffer::GetConsoleOutputCP()
{
	return con.m_dwConsoleOutputCP;
}

WORD CRealBuffer::GetConInMode()
{
	return con.m_dwConsoleInMode;
}

WORD CRealBuffer::GetConOutMode()
{
	return con.m_dwConsoleOutMode;
}

ExpandTextRangeType CRealBuffer::ExpandTextRange(const COORD crClick, COORD& crFrom/*[In/Out]*/, COORD& crTo/*[Out]*/, ExpandTextRangeType etr, CEStr* psText /*= nullptr*/)
{
	ExpandTextRangeType result = etr_None;
	crFrom = crClick; crTo = crClick;

	// crFrom/crTo must be absolute coordinates
	_ASSERTE(crFrom.Y>=con.m_sbi.srWindow.Top && crFrom.Y<GetBufferHeight());

	COORD lcrFrom = BufferToScreen(crFrom);
	COORD lcrTo = lcrFrom;

	// Lock buffer to acquire line from screen
	{
	MSectionLock csData; csData.Lock(&csCON);
	CRConDataGuard data;
	const wchar_t* pChar = nullptr;
	int nLen = 0;

	// GetConsoleLine requires visual-screen-buffer coordinates (but not absolute ones, coming from crFrom/crTo)
	if (mp_RCon->mp_VCon && GetConsoleLine(data, lcrFrom.Y, pChar, /*nullptr,*/ nLen, &csData) && pChar)
	{
		_ASSERTE(lcrFrom.Y>=0 && lcrFrom.Y<GetTextHeight());
		_ASSERTE(lcrTo.Y>=0 && lcrTo.Y<GetTextHeight());

		if (!mp_Match)
			mp_Match = new CMatch([this](LPCWSTR asSrc, CEStr& szFull) -> bool {
				return mp_RCon->GetFileFromConsole(asSrc, szFull);
			});

		if (mp_Match && (mp_Match->Match(etr, pChar, nLen, lcrFrom.X, data, lcrFrom.Y) > 0))
		{
			lcrFrom.X = mp_Match->mn_MatchLeft;
			lcrTo.X = mp_Match->mn_MatchRight % data->nWidth;
			lcrTo.Y += (mp_Match->mn_MatchRight - lcrTo.X) / data->nWidth;
			if (psText)
				psText->Set(mp_Match->ms_Match.ms_Val);
			result = mp_Match->m_Type;
		}
	}
	// We do not need line pointer anymore
	}

	// Fail?
	if (result == etr_None)
	{
		// Reset coordinates
		crFrom = MakeCoord(0,0);
		crTo = MakeCoord(0,0);
	}
	else
	{
		crFrom = ScreenToBuffer(lcrFrom);
		crTo = ScreenToBuffer(lcrTo);
	}
	StoreLastTextRange(result);
	return result;
}

bool CRealBuffer::StoreLastTextRange(ExpandTextRangeType etr)
{
	bool bChanged = false;

	if (con.etr.etrLast != etr)
	{
		con.etrWasChanged = bChanged = true;

		con.etr.etrLast = etr;
		//if (etr == etr_None)
		//{
		//	con.etr.mcr_FileLineStart = con.etr.mcr_FileLineEnd = MakeCoord(0,0);
		//}

		if ((mp_RCon->mp_ABuf == this) && mp_RCon->isVisible())
			gpConEmu->OnSetCursor();
	}

	return bChanged;
}

bool CRealBuffer::GetPanelRect(bool abRight, RECT* prc, bool abFull /*= false*/, bool abIncludeEdges /*= false*/)
{
	if (!this)
	{
		if (prc)
			*prc = MakeRect(-1,-1);

		return FALSE;
	}

	if (abRight)
	{
		if (prc)
			*prc = abFull ? mr_RightPanelFull : mr_RightPanel;

		if (mr_RightPanel.right <= mr_RightPanel.left)
			return FALSE;
	}
	else
	{
		if (prc)
			*prc = abFull ? mr_LeftPanelFull : mr_LeftPanel;

		if (mr_LeftPanel.right <= mr_LeftPanel.left)
			return FALSE;
	}

	if (prc && !abFull && abIncludeEdges)
	{
		prc->left  = abRight ? mr_RightPanelFull.left  : mr_LeftPanelFull.left;
		prc->right = abRight ? mr_RightPanelFull.right : mr_LeftPanelFull.right;
	}

	return TRUE;
}

bool CRealBuffer::isLeftPanel()
{
	return mb_LeftPanel;
}

bool CRealBuffer::isRightPanel()
{
	return mb_RightPanel;
}

void CRealBuffer::ResetTopLeft()
{
	if (con.TopLeft.isLocked())
	{
		SetTopLeft(-1, -1, true);
		mp_RCon->UpdateScrollInfo();
	}
}

LRESULT CRealBuffer::DoScrollBuffer(int nDirection, short nTrackPos /*= -1*/, UINT nCount /*= 1*/, bool abOnlyVirtual /*= false*/)
{
	if (!this) return 0;

	int nVisible = GetTextHeight();

	if (!nCount)
	{
		_ASSERTE(nCount >= 1);
		nCount = 1;
	}
	// returns "-1" when user choose "one screen per time"
	else if ((UINT)nCount >= (UINT)nVisible)
	{
		nCount = std::max(1,(nVisible-1));
	}

	// SB_LINEDOWN / SB_LINEUP / SB_PAGEDOWN / SB_PAGEUP
	#if 0
	if (m_Type == rbt_Primary)
	{
		if ((nDirection != SB_THUMBTRACK) && (nDirection != SB_THUMBPOSITION))
			nTrackPos = 0;
		if (nTrackPos < 0)
			nTrackPos = con.m_sbi.srWindow.Top;

		WPARAM wParm = MAKELONG(nDirection,nTrackPos);
		while (true)
		{
			WARNING("Переделать в команду пайпа");
			mp_RCon->PostConsoleMessage(mp_RCon->hConWnd, WM_VSCROLL, wParm, 0);

			if ((nCount <= 1) || (nDirection != SB_LINEUP && nDirection != SB_LINEDOWN) /*|| mp_RCon->isFar()*/)
				break;
			nCount--;
		}

		if ((nDirection == SB_THUMBTRACK) || (nDirection == SB_THUMBPOSITION))
		{
			_ASSERTE(nTrackPos>=0);
			int nVisible = GetTextHeight();

			if (nTrackPos < 0)
				nTrackPos = 0;
			else if ((nTrackPos + nVisible) >= con.m_sbi.dwSize.Y)
				nTrackPos = con.m_sbi.dwSize.Y - nVisible;

			// Обновим Top, иначе курсор отрисовывается в неправильном месте
			con.m_sbi.srWindow.Top = nTrackPos;
			con.m_sbi.srWindow.Bottom = nTrackPos + nVisible - 1;
			con.nTopVisibleLine = nTrackPos;

			////mp_RCon->mp_VCon->Invalidate();
			//mp_RCon->mb_DataChanged = TRUE; // Переменная используется внутри класса
			//con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole
		}
	}
	else
	#endif
	{
		int nNewTop = con.m_sbi.srWindow.Top;
		int nMaxHeight = GetDynamicHeight();

		switch (nDirection)
		{
		case SB_LINEDOWN:
			if ((nNewTop + nVisible) < nMaxHeight)
				nNewTop = std::min<int>((nNewTop+(SHORT)nCount), (nMaxHeight-nVisible));
			break;
		case SB_LINEUP:
			if (nNewTop > 0)
				nNewTop = std::max<int>(0,(nNewTop-(SHORT)nCount));
			break;
		case SB_PAGEDOWN:
			nNewTop = std::min<int>((nMaxHeight - nVisible), (con.m_sbi.srWindow.Top + nVisible - 1));
			break;
		case SB_PAGEUP:
			nNewTop = std::max<int>(0, (con.m_sbi.srWindow.Top - nVisible + 1));
			break;
		case SB_TOP:
			nNewTop = 0;
			break;
		case SB_BOTTOM:
			nNewTop = (nMaxHeight - nVisible);
			break;
		case SB_THUMBTRACK:
		case SB_THUMBPOSITION:
			if (nTrackPos < 0)
			{
				_ASSERTE(nTrackPos>=0);
				nTrackPos = 0;
			}
			else if ((nTrackPos + nVisible) >= nMaxHeight)
			{
				nTrackPos = nMaxHeight - nVisible;
			}
			nNewTop = nTrackPos;
			break;
		case SB_ENDSCROLL:
			break;
		default:
			_ASSERTE(FALSE && "Unexpected scroll command");
		}


		if (nNewTop != GetBufferPosY())
		{
			con.m_sbi.srWindow.Top = SHORT(nNewTop);
			_ASSERTE(con.m_sbi.srWindow.Top == nNewTop);
			con.m_sbi.srWindow.Bottom = SHORT(con.m_sbi.srWindow.Top + nVisible - 1);

			SetTopLeft(nNewTop, con.TopLeft.x, true, abOnlyVirtual);

			// in rbt_Primary the server must reload new viewport and sent console data back to us
			// otherwise we would not know what we need to show to user
			if (m_Type != rbt_Primary)
			{
				mp_RCon->mb_DataChanged = TRUE; // Переменная используется внутри класса
				con.bConsoleDataChanged = TRUE; // А эта - при вызовах из CVirtualConsole
			}
		}
	}
	return 0;
}

// abServerCall - pass new TopLeft into the server to let it reload viewport and send it back to GUI
// abOnlyVirtual - if true - don't call SetConsoleWindowInfo
bool CRealBuffer::SetTopLeft(int ay /*= -1*/, int ax /*= -1*/, bool abServerCall /*= false*/, bool abOnlyVirtual /*= false*/)
{
	bool bChanged = false;
	bool bLockChanged = false;

	if (con.TopLeft.y != ay || con.TopLeft.x != ax)
	{
		bChanged = true;
		bLockChanged = !con.TopLeft.isLocked();

		#ifdef _DEBUG
		wchar_t szDbg[120]; DWORD nSrvPID = mp_RCon->GetServerPID();
		swprintf_c(szDbg, L"TopLeft changed to {%i,%i} from {%i,%i} SrvPID=%u\n", ay,ax, con.TopLeft.y, con.TopLeft.x, nSrvPID);
		DEBUGSTRTOPLEFT(szDbg);
		#endif
	}

	if (abServerCall)
	{
		con.InTopLeftSet = TRUE;
		con.TopLeftTick = GetTickCount();
	}

	con.TopLeft.y = ay;
	con.TopLeft.x = ax;

	if (abServerCall && (m_Type == rbt_Primary))
	{
		DWORD nSrvPID = mp_RCon->GetServerPID();
		if (nSrvPID)
		{
			CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_SETTOPLEFT, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_CONINFO));
			pIn->ReqConInfo.TopLeft = con.TopLeft;
			pIn->ReqConInfo.VirtualOnly = abOnlyVirtual;
			CESERVER_REQ *pOut = ExecuteSrvCmd(nSrvPID, pIn, ghWnd);
			ExecuteFreeResult(pOut);
			ExecuteFreeResult(pIn);
			con.TopLeftTick = GetTickCount();
		}
	}

	if (bLockChanged)
	{
		mp_RCon->UpdateScrollInfo();
	}

	if (abServerCall)
	{
		con.InTopLeftSet = FALSE;
	}

	return bChanged;
}

const CRgnDetect* CRealBuffer::GetDetector()
{
	if (this)
		return &m_Rgn;
	return nullptr;
}
