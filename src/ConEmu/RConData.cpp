
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#include "Options.h"
#include "RConData.h"
#include "RConPalette.h"
#include "RealConsole.h"
#include "../common/ConsoleMixAttr.h"
#include "../common/wcchars.h"

#define DEBUGSTRTRUEMOD(s) //DEBUGSTR(s)



void ConsoleLinePtr::clear()
{
	pChar = nullptr;
	pAttr = nullptr;
	pAttrEx = nullptr;
	nLen = 0;
}

bool ConsoleLinePtr::get(int index, wchar_t& chr, CharAttr& atr)
{
	if (index < 0 || index > nLen)
		return false;
	if (!pChar || !(pAttr || pAttrEx))
		return false;

	chr = pChar[index];

	if (pAttrEx != nullptr)
	{
		atr = pAttrEx[index];
	}
	else if (pAttr)
	{
	}

	_ASSERTE(FALSE && "Complete ConsoleLinePtr::get"); // #TODO
	return false;
}




bool CRConDataGuard::isValid(size_t anCellsRequired /*= 0*/)
{
	if (!mp_Ref || !mp_Ref->isValid(false, anCellsRequired))
		return false;
	return true;
}



/////////////////////////////////////////////////

bool CRConData::Allocate(CRConDataGuard& data, CRealConsole* apRCon, size_t anMaxCells)
{
	if (!anMaxCells)
	{
		_ASSERTE(anMaxCells > 0);
		data.Release();
		return false;
	}

	// Allocate slightly more than requested, to avoid
	// spare data reallocation on minor console resize
	size_t cchNewCharMaxPlus = anMaxCells * 3 / 2;

	CRConData* p = new CRConData(apRCon);
	if (!p
		|| !(p->pConChar = (wchar_t*)calloc(cchNewCharMaxPlus, sizeof(*p->pConChar)))
		|| !(p->pConAttr = (WORD*)calloc(cchNewCharMaxPlus, sizeof(*p->pConAttr)))
		|| !(p->pDataCmp = (CHAR_INFO*)calloc(cchNewCharMaxPlus, sizeof(*p->pDataCmp))))
	{
		if (p)
			delete p;
		p = nullptr;
		data.Release();
		return false;
	}
	else
	{
		p->nMaxCells = cchNewCharMaxPlus;
	}

	data.Attach(p);
	p->Release();
	return true;
}

bool CRConData::Allocate(CRConDataGuard& data, CRealConsole* apRCon, wchar_t* apszBlock1, CharAttr* apcaBlock1, const CONSOLE_SCREEN_BUFFER_INFO& asbi, const COORD& acrSize)
{
	if (!apszBlock1 || acrSize.X <= 0 || acrSize.Y <= 0)
	{
		_ASSERTE(apszBlock1 && acrSize.X > 0 && acrSize.Y > 0);
		data.Release();
		return false;
	}
	CRConData* p = new CRConData(apRCon, apszBlock1, apcaBlock1, asbi, acrSize);
	data.Attach(p);
	p->Release();
	return true;
}

CRConData::CRConData(CRealConsole* apRCon)
	: mp_RCon(apRCon)
	, nMaxCells(0)
	, nWidth(0), nHeight(0)
	, pConChar(nullptr)
	, pConAttr(nullptr)
	, pDataCmp(nullptr)
	, m_sbi()
	, bExternal(false)
	, pszBlock1(nullptr)
	, pcaBlock1(nullptr)
{
}

CRConData::CRConData(CRealConsole* apRCon, wchar_t* apszBlock1, CharAttr* apcaBlock1, const CONSOLE_SCREEN_BUFFER_INFO& sbi, const COORD& acrSize)
	: mp_RCon(apRCon)
	, nMaxCells(acrSize.X * acrSize.Y)
	, nWidth(acrSize.X), nHeight(acrSize.Y)
	, pConChar(nullptr)
	, pConAttr(nullptr)
	, pDataCmp(nullptr)
	, m_sbi(sbi)
	, bExternal(true)
	, pszBlock1(apszBlock1)
	, pcaBlock1(apcaBlock1)
{
}

CRConData::~CRConData()
{
	SafeFree(pConChar);
	SafeFree(pConAttr);
	SafeFree(pDataCmp);
	// pszBlock1 & pcaBlock1 are external pointers (loading console data from dumps)
}

bool CRConData::isValid(bool bCheckSizes, size_t anCellsRequired)
{
	if (!this)
		return false;
	if (!nMaxCells || !pConChar || !pConAttr)
		return false;
	if (!pDataCmp)
		return false;

	if (bCheckSizes && (!nWidth || !nHeight))
		return false;

	if (anCellsRequired && (anCellsRequired > nMaxCells))
	{
		_ASSERTE(anCellsRequired <= nMaxCells);
		return false;
	}

	return true;
}

void CRConData::FinalRelease()
{
	CRConData* p = this;
	delete p;
}

// returns count of *lines* were copied to pChar/pAttr
// nWidth & nHeight - dimension which VCon want to display
UINT CRConData::GetConsoleData(wchar_t* rpChar, CharAttr* rpAttr, UINT anWidth, UINT anHeight,
	wchar_t wSetChar, CharAttr lcaDef, CharAttr *lcaTable, CharAttr *lcaTableExt,
	bool bFade, bool bExtendFonts)
{
	TODO("Во время ресайза консоль может подглючивать - отдает не то что нужно...");
	//_ASSERTE(*con.pConChar!=ucBoxDblVert);
	UINT nYMax = std::min(anHeight,nHeight);

	AnnotationHeader colorHeader = {};
	const AnnotationInfo* pTrueData = nullptr;
	bool absoluteColors = false;
	const bool haveColors = mp_RCon->GetColorData(colorHeader, pTrueData, absoluteColors);

	wchar_t  *pszDst = rpChar;
	CharAttr *pcaDst = rpAttr;

	wchar_t  *pszSrc = pConChar;
	WORD     *pnSrc = pConAttr;

	bool lbIsFar = mp_RCon->isFar();

	// Т.к. есть блокировка (csData), то con.pConChar и con.pConAttr
	// не должны меняться во время выполнения этой функции
	const wchar_t* const pszSrcStart = pConChar;
	WORD* const pnSrcStart = pConAttr;
	size_t nSrcCells = (nWidth * nHeight);

	Assert(pszSrcStart==pszSrc && pnSrcStart==pnSrc);

	const AnnotationInfo *pcolSrc = nullptr;
	const AnnotationInfo *pcolEnd = nullptr;
	BOOL bUseColorData = FALSE, bStartUseColorData = FALSE;

	if (mp_RCon->UseTrueColor() && haveColors)
	{
		pcolSrc = pTrueData;
		pcolEnd = pTrueData + colorHeader.bufferSize;
		if (absoluteColors) pcolSrc += std::min<size_t>(colorHeader.bufferSize, static_cast<size_t>(m_sbi.srWindow.Top) * nWidth);
		bUseColorData = TRUE;
		WARNING("Если far/w - pcolSrc нужно поднять вверх, bStartUseColorData=TRUE, bUseColorData=FALSE");
		if (!absoluteColors && m_sbi.dwSize.Y > (int)nHeight) // con.bBufferHeight
		{
			int lnShiftRows = (m_sbi.dwSize.Y - anHeight) - m_sbi.srWindow.Top;

			if (lnShiftRows > 0)
			{
				#ifdef _DEBUG
				if (nWidth != (m_sbi.srWindow.Right - m_sbi.srWindow.Left + 1))
				{
					_ASSERTE(nWidth == (m_sbi.srWindow.Right - m_sbi.srWindow.Left + 1));
				}
				#endif

				pcolSrc -= (lnShiftRows * nWidth);
				DEBUGSTRTRUEMOD(L"TrueMod skipped due to nShiftRows, bStartUseColorData was set");
				bUseColorData = FALSE;
				bStartUseColorData = TRUE;
			}
			#ifdef _DEBUG
			else if (lnShiftRows < 0)
			{
				//_ASSERTE(nShiftRows>=0);
				wchar_t szLog[200];
				swprintf_c(szLog, L"!!! CRealBuffer::GetConsoleData !!! "
					L"nShiftRows=%i nWidth=%i nHeight=%i Rect={%i,%i}-{%i,%i} Buf={%i,%i}",
					lnShiftRows, anWidth, anHeight,
					m_sbi.srWindow.Left, m_sbi.srWindow.Top, m_sbi.srWindow.Right, m_sbi.srWindow.Bottom,
					m_sbi.dwSize.X, m_sbi.dwSize.Y);
				mp_RCon->LogString(szLog);
			}
			#endif
		}
	}
	else
	{
		DEBUGSTRTRUEMOD(L"TrueMod is not allowed here");
	}

	DWORD cbDstLineSize = anWidth * 2;
	DWORD cnSrcLineLen = nWidth;
	DWORD cbSrcLineSize = cnSrcLineLen * 2;

	#ifdef _DEBUG
	if (nWidth != m_sbi.dwSize.X)
	{
		_ASSERTE(nWidth == m_sbi.dwSize.X); // Scrolling?
	}
	#endif

	DWORD cbLineSize = std::min(cbDstLineSize,cbSrcLineSize);
	int nCharsLeft = (anWidth > nWidth) ? (anWidth - nWidth) : 0;
	//int nY, nX;
	UINT nExtendStartsY = 0;
	//int nPrevDlgBorder = -1;

	// Собственно данные
	for (UINT nY = 0; nY < nYMax; nY++)
	{
		if (nY == nExtendStartsY)
			lcaTable = lcaTableExt;

		// Текст
		memmove(pszDst, pszSrc, cbLineSize);

		if (nCharsLeft > 0)
			wmemset(pszDst+cnSrcLineLen, wSetChar, nCharsLeft);

		// Console text colors (Fg,Bg)
		BYTE PalIndex = MAKECONCOLOR(7,0);

		// While console is in recreate (shutdown console, startup new root)
		// it's shown using monochrome (gray on black)
		bool bForceMono = (mp_RCon->mn_InRecreate != 0);

		bool has_rowid = (cnSrcLineLen >= ROWID_USED_CELLS) && (GetRowIdFromAttrs(pnSrc) != 0);
		#ifdef _DEBUG
		if (!has_rowid && cnSrcLineLen >= ROWID_USED_CELLS) {
			_ASSERTE(!(pnSrc[0] & (CHANGED_CONATTR & ~LEGACY_CONATTR)));
			_ASSERTE(!(pnSrc[1] & (CHANGED_CONATTR & ~LEGACY_CONATTR)));
			_ASSERTE(!(pnSrc[2] & (CHANGED_CONATTR & ~LEGACY_CONATTR)));
			_ASSERTE(!(pnSrc[3] & (CHANGED_CONATTR & ~LEGACY_CONATTR)));
		}
		#endif

		int iTail = cnSrcLineLen;
		wchar_t* pch = pszDst;
		for (UINT nX = 0;
			// Don't forget to advance same pointers at the and if bPair
			iTail-- > 0; nX++, pnSrc++, pcolSrc++, pch++)
		{
			CharAttr& lca = pcaDst[nX];
			bool hasTrueColor = false;
			bool hasFont = false;
			bool inversed = false;
			bool is_rowid = has_rowid && (nX < ROWID_USED_CELLS);

			// If not "mono" we need only lower byte with color indexes
			if (!bForceMono)
			{
				inversed = !is_rowid && ((*pnSrc) & COMMON_LVB_REVERSE_VIDEO);
				if (inversed)
					PalIndex = MAKECONCOLOR(CONBACKCOLOR(*pnSrc), CONFORECOLOR(*pnSrc)); // Inverse
				else
					PalIndex = ((*pnSrc) & 0xFF);
			}
			TODO("OPTIMIZE: lca = lcaTable[PalIndex];");
			lca = lcaTable[PalIndex];
			TODO("OPTIMIZE: вынести проверку bExtendColors за циклы");

			lca.ConAttr = *pnSrc;

			bool bPair = (iTail > 0);
			ucs32 wwch = ucs32_from_wchar(pch, bPair);
			_ASSERTE(wwch >= 0);

			// Colorer & Far - TrueMod
			TODO("OPTIMIZE: вынести проверку bUseColorData за циклы");

			if (bStartUseColorData && !bUseColorData)
			{
				// В случае "far /w" буфер цвета может начаться НИЖЕ верхней видимой границы,
				// если буфер немного прокручен вверх
				if (pcolSrc >= mp_RCon->mp_TrueColorerData)
				{
					DEBUGSTRTRUEMOD(L"TrueMod forced back due bStartUseColorData");
					bUseColorData = TRUE;
				}
			}

			if (bUseColorData)
			{
				if (pcolSrc >= pcolEnd)
				{
					DEBUGSTRTRUEMOD(L"TrueMod stopped - out of buffer");
					bUseColorData = FALSE;
				}
				else
				{
					if (pcolSrc->style & AI_STYLE_REVERSE)
						inversed = true;

					if (pcolSrc->fg_valid)
					{
						hasTrueColor = true;
						hasFont = true;
						lca.nFontIndex = fnt_Normal; //bold/italic/underline will be set below

						unsigned fore = bFade ? gpSet->GetFadeColor(pcolSrc->fg_color) : pcolSrc->fg_color;
						unsigned back = pcolSrc->bk_valid
							? (bFade ? gpSet->GetFadeColor(pcolSrc->bk_color) : pcolSrc->bk_color)
							: (lca.crBackColor);
						lca.crForeColor = inversed ? back : fore;
						lca.crBackColor = inversed ? fore : back;
					}
					else if (pcolSrc->bk_valid)
					{
						hasTrueColor = true;
						hasFont = true;
						lca.nFontIndex = fnt_Normal; //bold/italic/underline will be set below
						unsigned back = bFade ? gpSet->GetFadeColor(pcolSrc->bk_color) : pcolSrc->bk_color;;
						if (inversed)
							lca.crForeColor = back;
						else
							lca.crBackColor = back;
					}

					// nFontIndex: 0 - normal, 1 - bold, 2 - italic, 3 - bold&italic,..., 4 - underline, ...
					if (pcolSrc->style)
					{
						hasFont = true;
						lca.nFontIndex = pcolSrc->style & fnt_StdFontMask;
					}
				}
			}

			if (!hasFont && !is_rowid && ((*pnSrc) & COMMON_LVB_UNDERSCORE))
			{
				lca.nFontIndex = fnt_Underline;
			}

			switch (get_wcwidth(wwch))
			{
			case 0:
				lca.Flags2 |= CharAttr2_Combining;
				break;
			case 2:
				lca.Flags2 |= CharAttr2_DoubleSpaced;
				break;
			}

			if (bPair)
			{
				lca.Flags2 |= CharAttr2_Surrogate;
				CharAttr& lca2 = pcaDst[nX+1];
				lca2 = lca;
				lca2.Flags2 = (lca.Flags2 & ~(CharAttr2_Combining)) | CharAttr2_NonSpacing;
				// advance +1 character
				nX++; pnSrc++; pcolSrc++; pch++; iTail--;
			}
		}

		// Залить остаток (если запрошен больший участок, чем есть консоль
		for (UINT nX = cnSrcLineLen; nX < anWidth; nX++)
		{
			pcaDst[nX] = lcaDef;
		}

		// Far2 показывает красный 'A' в правом нижнем углу консоли
		// Этот ярко красный цвет фона может попасть в Extend Font Colors
		if (bExtendFonts && ((nY+1) == nYMax) && lbIsFar
			&& (pszDst[anWidth-1] == L'A') && (PalIndex == 0xCF))
		{
			// Вернуть "родной" цвет и шрифт
			pcaDst[anWidth-1] = lcaTable[PalIndex];
		}

		// Next line
		pszDst += anWidth;
		pcaDst += anWidth;
		pszSrc += cnSrcLineLen;
	}

	#ifndef __GNUC__
	UNREFERENCED_PARAMETER(pszSrcStart);
	UNREFERENCED_PARAMETER(pnSrcStart);
	#endif
	UNREFERENCED_PARAMETER(nSrcCells);

	return nYMax;
}

bool CRConData::GetConsoleLine(int nLine, ConsoleLinePtr& rpLine) const
{
	if (!bExternal)
	{
		if ((nLine < 0) || (static_cast<UINT>(nLine) >= nHeight)
			|| ((size_t)((nLine + 1) * nWidth) > nMaxCells))
		{
			// _ASSERTE((nLine >= 0) && (nLine < nHeight) && ((size_t)((nLine + 1) * nWidth) > nMaxCells));
			return false;
		}

		size_t lineShift = nLine * nWidth;
		rpLine.pChar = pConChar ? (pConChar + lineShift) : nullptr;
		rpLine.pAttr = pConAttr ? (pConAttr + lineShift) : nullptr;
		rpLine.nLen = nWidth;
	}
	else
	{
		if ((nLine < 0) || (static_cast<UINT>(nLine) >= nHeight))
		{
			// _ASSERTE((nLine >= 0) && (nLine < nHeight));
			return false;
		}

		size_t lineShift = ((m_sbi.srWindow.Top + nLine) * nWidth) + m_sbi.srWindow.Left;
		rpLine.pChar = pszBlock1 ? (pszBlock1 + lineShift) : nullptr;
		rpLine.pAttrEx = pcaBlock1 ? (pcaBlock1 + lineShift) : nullptr;
		rpLine.nLen = nWidth;
	}

	return (rpLine.pChar != nullptr);
}

bool CRConData::FindPanels(bool& bLeftPanel, RECT& rLeftPanel, RECT& rLeftPanelFull, bool& bRightPanel, RECT& rRightPanel, RECT& rRightPanelFull)
{
	rLeftPanel = rLeftPanelFull = rRightPanel = rRightPanelFull = MakeRect(-1,-1);
	bLeftPanel = bRightPanel = false;

	if (!isValid(true, nWidth*nHeight))
		return false;

	unsigned nY = 0;
	BOOL lbIsMenu = FALSE;

	if (pConChar[0] == L' ')
	{
		lbIsMenu = TRUE;

		for (UINT i=0; i<nWidth; i++)
		{
			if (pConChar[i]==ucBoxDblHorz || pConChar[i]==ucBoxDblDownRight || pConChar[i]==ucBoxDblDownLeft)
			{
				lbIsMenu = FALSE; break;
			}
		}

		if (lbIsMenu)
			nY ++; // скорее всего, первая строка - меню
	}
	else if ((((pConChar[0] == L'R') && ((pConAttr[0] & 0xFF) == 0x4F))
		|| ((pConChar[0] == L'P') && ((pConAttr[0] & 0xFF) == 0x2F)))
		&& pConChar[1] == L' ')
	{
		for (UINT i=1; i<nWidth; i++)
		{
			if (pConChar[i]==ucBoxDblHorz || pConChar[i]==ucBoxDblDownRight || pConChar[i]==ucBoxDblDownLeft)
			{
				lbIsMenu = FALSE; break;
			}
		}

		if (lbIsMenu)
			nY ++; // скорее всего, первая строка - меню, при включенной записи макроса
	}

	unsigned nIdx = nY*nWidth;
	// Левая панель
	BOOL bFirstCharOk = (nY == 0)
		&& (
		(pConChar[0] == L'R' && (pConAttr[0] & 0xFF) == 0x4F) // символ записи макроса
			|| (pConChar[0] == L'P' && (pConAttr[0] & 0xFF) == 0x2F) // символ воспроизведения макроса
			);

	bool bFarShowColNames = true;
	bool bFarShowSortLetter = true;
	bool bFarShowStatus = true;
	const CEFAR_INFO_MAPPING *pFar = nullptr;
	if (mp_RCon->m_FarInfo.cbSize)
	{
		pFar = &mp_RCon->m_FarInfo;
		if (pFar)
		{
			if ((pFar->FarPanelSettings.ShowColumnTitles) == 0) //-V112
				bFarShowColNames = false;
			if ((pFar->FarPanelSettings.ShowSortModeLetter) == 0)
				bFarShowSortLetter = false;
			if ((pFar->FarPanelSettings.ShowStatusLine) == 0)
				bFarShowStatus = false;
		}
	}

	// Проверяем левую панель
	bool bContinue = false;
	if (pConChar[nIdx+nWidth] == ucBoxDblVert) // двойная рамка продолжается вниз
	{
		if ((bFirstCharOk || pConChar[nIdx] != ucBoxDblDownRight)
			&& (pConChar[nIdx+1]>=L'0' && pConChar[nIdx+1]<=L'9')) // открыто несколько редакторов/вьюверов
			bContinue = true;
		else if (((bFirstCharOk || pConChar[nIdx] == ucBoxDblDownRight)
			&& (((pConChar[nIdx+1] == ucBoxDblHorz || pConChar[nIdx+1] == L' ') && (bFarShowColNames || !bFarShowSortLetter))
				|| pConChar[nIdx+1] == ucBoxSinglDownDblHorz // доп.окон нет, только рамка
				|| pConChar[nIdx+1] == ucBoxDblDownDblHorz
				|| (pConChar[nIdx+1] == L'[' && pConChar[nIdx+2] == ucLeftScroll) // ScreenGadgets, default
				|| (!bFarShowColNames && !(pConChar[nIdx+1] == ucBoxDblHorz || pConChar[nIdx+1] == L' ')
					&& pConChar[nIdx+1] != ucBoxSinglDownDblHorz && pConChar[nIdx+1] != ucBoxDblDownDblHorz)
				)))
			bContinue = true;
	}

	if (bContinue)
	{
		for (UINT i=2; !bLeftPanel && i<nWidth; i++)
		{
			// Найти правый край левой панели
			if (pConChar[nIdx+i] == ucBoxDblDownLeft
				&& ((pConChar[nIdx+i-1] == ucBoxDblHorz || pConChar[nIdx+i-1] == L' ')
					|| pConChar[nIdx+i-1] == ucBoxSinglDownDblHorz // правый угол панели
					|| pConChar[nIdx+i-1] == ucBoxDblDownDblHorz
					|| (pConChar[nIdx+i-1] == L']' && pConChar[nIdx+i-2] == L'\\') // ScreenGadgets, default
					)
				// МОЖЕТ быть закрыто AltHistory
				/*&& pConChar[nIdx+i+nWidth] == ucBoxDblVert*/)
			{
				unsigned nBottom = nHeight - 1;

				while(nBottom > 4) //-V112
				{
					if (pConChar[nWidth*nBottom] == ucBoxDblUpRight
						/*&& pConChar[nWidth*nBottom+i] == ucBoxDblUpLeft*/)
					{
						rLeftPanel.left = 1;
						rLeftPanel.top = nY + (bFarShowColNames ? 2 : 1);
						rLeftPanel.right = i-1;
						if (bFarShowStatus)
						{
							rLeftPanel.bottom = nBottom - 3;
							for (int j = (nBottom - 3); j > (int)(nBottom - 13) && j > rLeftPanel.top; j--)
							{
								if (pConChar[nWidth*j] == ucBoxDblVertSinglRight)
								{
									rLeftPanel.bottom = j - 1; break;
								}
							}
						}
						else
						{
							rLeftPanel.bottom = nBottom - 1;
						}
						rLeftPanelFull.left = 0;
						rLeftPanelFull.top = nY;
						rLeftPanelFull.right = i;
						rLeftPanelFull.bottom = nBottom;
						bLeftPanel = TRUE;
						break;
					}

					nBottom --;
				}
			}
		}
	}

	// (Если есть левая панель и она не FullScreen) или левой панели нет вообще
	if ((bLeftPanel && rLeftPanelFull.right < (int)nWidth) || !bLeftPanel)
	{
		if (bLeftPanel)
		{
			// Положение известно, нужно только проверить наличие
			if (pConChar[nIdx+rLeftPanelFull.right+1] == ucBoxDblDownRight
				/*&& pConChar[nIdx+rLeftPanelFull.right+1+nWidth] == ucBoxDblVert*/
				/*&& pConChar[nIdx+nWidth*2] == ucBoxDblVert*/
				/*&& pConChar[(rLeftPanelFull.bottom+3)*nWidth+rLeftPanelFull.right+1] == ucBoxDblUpRight*/
				&& pConChar[(rLeftPanelFull.bottom+1)*nWidth-1] == ucBoxDblUpLeft
				)
			{
				rRightPanel = rLeftPanel; // bottom & top берем из rLeftPanel
				rRightPanel.left = rLeftPanelFull.right+2;
				rRightPanel.right = nWidth-2;
				rRightPanelFull = rLeftPanelFull;
				rRightPanelFull.left = rLeftPanelFull.right+1;
				rRightPanelFull.right = nWidth-1;
				bRightPanel = TRUE;
			}
		}

		// Начиная с FAR2 build 1295 панели могут быть разной высоты
		// или левой панели нет
		// или активная панель в FullScreen режиме
		if (!bRightPanel)
		{
			// нужно определить положение панели
			if (((pConChar[nIdx+nWidth-1]>=L'0' && pConChar[nIdx+nWidth-1]<=L'9')  // справа часы
				|| pConChar[nIdx+nWidth-1] == ucBoxDblDownLeft) // или рамка
				&& (pConChar[nIdx+nWidth*2-1] == ucBoxDblVert // ну и правая граница панели
					|| pConChar[nIdx+nWidth*2-1] == ucUpScroll) // или стрелка скроллбара
				)
			{
				int iMinFindX = bLeftPanel ? (rLeftPanelFull.right+1) : 0;
				for(int i=nWidth-3; !bRightPanel && i>=iMinFindX; i--)
				{
					// we look for LEFT edge of the RIGHT panel
					if (pConChar[nIdx+i] == ucBoxDblDownRight
						&& (((pConChar[nIdx+i+1] == ucBoxDblHorz || pConChar[nIdx+i+1] == L' ') && bFarShowColNames)
							|| pConChar[nIdx+i+1] == ucBoxSinglDownDblHorz // right corner
							|| pConChar[nIdx+i+1] == ucBoxDblDownDblHorz
							|| ((i > 10) && (pConChar[nIdx+i-1] == L']' && pConChar[nIdx+i-2] == L'\\')) // ScreenGadgets, default: "╔[◄|►]...[…][\]╗╔[◄|►]═══"
							|| (!bFarShowColNames && !(pConChar[nIdx+i+1] == ucBoxDblHorz || pConChar[nIdx+i+1] == L' ')
								&& pConChar[nIdx+i+1] != ucBoxSinglDownDblHorz && pConChar[nIdx+i+1] != ucBoxDblDownDblHorz)
							)
						// May be covered by AltHistory
						/*&& pConChar[nIdx+i+nWidth] == ucBoxDblVert*/)
					{
						unsigned nBottom = nHeight - 1;

						while(nBottom > 4) //-V112
						{
							if (/*pConChar[nWidth*nBottom+i] == ucBoxDblUpRight
								&&*/ pConChar[nWidth*(nBottom+1)-1] == ucBoxDblUpLeft)
							{
								rRightPanel.left = i+1;
								rRightPanel.top = nY + (bFarShowColNames ? 2 : 1);
								rRightPanel.right = nWidth-2;
								//rRightPanel.bottom = nBottom - 3;
								if (bFarShowStatus)
								{
									rRightPanel.bottom = nBottom - 3;
									for (int j = (nBottom - 3); j > (int)(nBottom - 13) && j > rRightPanel.top; j--)
									{
										if (pConChar[nWidth*j+i] == ucBoxDblVertSinglRight)
										{
											rRightPanel.bottom = j - 1; break;
										}
									}
								}
								else
								{
									rRightPanel.bottom = nBottom - 1;
								}
								rRightPanelFull.left = i;
								rRightPanelFull.top = nY;
								rRightPanelFull.right = nWidth-1;
								rRightPanelFull.bottom = nBottom;
								bRightPanel = TRUE;
								break;
							}

							nBottom --;
						}
					}
				}
			}
		}
	}

	return (bLeftPanel || bRightPanel);
}

short CRConData::CheckProgressInConsole(UINT nCursorLine)
{
	if (!isValid(true, nWidth*(nCursorLine+1)))
		return -1;

	const wchar_t* pszCurLine = pConChar + (nWidth * nCursorLine);

	// Обработка прогресса NeroCMD и пр. консольных программ (если курсор находится в видимой области)
	//Плагин Update
	//"Downloading Far                                               99%"
	//NeroCMD
	//"012% ########.................................................................."
	//ChkDsk
	//"Completed: 25%"
	//Rar
	// ...       Vista x86\Vista x86.7z         6%
	//aria2c
	//[#1 SIZE:0B/9.1MiB(0%) CN:1 SPD:1.2KiBs ETA:2h1m11s]
	ssize_t nIdx = 0;
	bool bAllowDot = false;
	short nProgress = -1;

	const wchar_t *szPercentEng = L" percent";
	const wchar_t *szComplEng  = L"Completed:";
	static wchar_t szPercentRus[16] = {}, szComplRus[16] = {};
	static int nPercentEngLen = lstrlen(szPercentEng), nComplEngLen = lstrlen(szComplEng);
	static int nPercentRusLen, nComplRusLen;

	if (szPercentRus[0] == 0)
	{
		szPercentRus[0] = L' ';
		TODO("Хорошо бы и другие национальные названия обрабатывать, брать из настройки");
		lstrcpy(szPercentRus,L"процент");
		lstrcpy(szComplRus,L"Завершено:");

		nPercentRusLen = lstrlen(szPercentRus);
		nComplRusLen = lstrlen(szComplEng);
	}

	// Сначала проверим, может цифры идут в начале строки (лидирующие пробелы)?
	if (pszCurLine[nIdx] == L' ' && isDigit(pszCurLine[nIdx+1]))
		nIdx++; // один лидирующий пробел перед цифрой
	else if (pszCurLine[nIdx] == L' ' && pszCurLine[nIdx+1] == L' ' && isDigit(pszCurLine[nIdx+2]))
		nIdx += 2; // два лидирующих пробела перед цифрой
	else if (!isDigit(pszCurLine[nIdx]))
	{
		// Строка начинается НЕ с цифры. Может начинается одним из известных префиксов (ChkDsk)?

		if (!wcsncmp(pszCurLine, szComplRus, nComplRusLen))
		{
			nIdx += nComplRusLen;

			if (pszCurLine[nIdx] == L' ') nIdx++;

			if (pszCurLine[nIdx] == L' ') nIdx++;

			bAllowDot = true;
		}
		else if (!wcsncmp(pszCurLine, szComplEng, nComplEngLen))
		{
			nIdx += nComplEngLen;

			if (pszCurLine[nIdx] == L' ') nIdx++;

			if (pszCurLine[nIdx] == L' ') nIdx++;

			bAllowDot = true;
		}
		else if (!wcsncmp(pszCurLine, L"[#", 2))
		{
			const wchar_t* p = wcsstr(pszCurLine, L"%) ");
			while ((p > pszCurLine) && (p[-1] != L'('))
				p--;
			if (p > pszCurLine)
				nIdx = p - pszCurLine;
		}

		// Известных префиксов не найдено, проверяем, может процент есть в конце строки?
		if (!nIdx)
		{
			//TODO("Не работает с одной цифрой");
			// Creating archive T:\From_Work\VMWare\VMWare.part006.rar
			// ...       Vista x86\Vista x86.7z         6%
			int i = nWidth - 1;

			// Откусить trailing spaces
			while ((i > 3) && (pszCurLine[i] == L' '))
				i--;

			// Теперь, если дошли до '%' и перед ним - цифра
			if (i >= 3 && pszCurLine[i] == L'%' && isDigit(pszCurLine[i-1]))
			{
				//i -= 2;
				i--;

				int j = i, k = -1;
				while (j > 0 && isDigit(pszCurLine[j-1]))
					j--;

				// Может быть что-то типа "Progress 25.15%"
				if (((i - j) <= 2) && (j >= 2) && isDot(pszCurLine[j-1]))
				{
					k = j - 1;
					while (k > 0 && isDigit(pszCurLine[k-1]))
						k--;
				}

				if (k >= 0)
				{
					if (((j - k) <= 3) // 2 цифры + точка
						|| (((j - k) <= 4) && (pszCurLine[k] == L'1'))) // "100.0%"
					{
						nIdx = i = k;
						bAllowDot = true;
					}
				}
				else
				{
					if (((j - i) <= 2) // 2 цифры + точка
						|| (((j - i) <= 3) && (pszCurLine[j] == L'1'))) // "100%"
					{
						nIdx = i = j;
					}
				}

				#if 0
				// Две цифры перед '%'?
				if (isDigit(pszCurLine[i-1]))
					i--;

				// Три цифры допускается только для '100%'
				if (pszCurLine[i-1] == L'1' && !isDigit(pszCurLine[i-2]))
				{
					nIdx = i - 1;
				}
				// final check
				else if (!isDigit(pszCurLine[i-1]))
				{
					nIdx = i;
				}
				#endif

				// Может ошибочно детектировать прогресс, если его ввести в prompt
				// Допустим, что если в строке есть символ '>' - то это не прогресс
				while (i>=0)
				{
					if (pszCurLine[i] == L'>')
					{
						nIdx = 0;
						break;
					}

					i--;
				}
			}
		}
	}

	// Менять nProgress только если нашли проценты в строке с курсором
	if (isDigit(pszCurLine[nIdx]))
	{
		if (isDigit(pszCurLine[nIdx+1]) && isDigit(pszCurLine[nIdx+2])
			&& (pszCurLine[nIdx+3]==L'%' || (bAllowDot && isDot(pszCurLine[nIdx+3]))
				|| !wcsncmp(pszCurLine+nIdx+3,szPercentEng,nPercentEngLen)
				|| !wcsncmp(pszCurLine+nIdx+3,szPercentRus,nPercentRusLen)))
		{
			nProgress = 100*(pszCurLine[nIdx] - L'0') + 10*(pszCurLine[nIdx+1] - L'0') + (pszCurLine[nIdx+2] - L'0');
		}
		else if (isDigit(pszCurLine[nIdx+1])
			&& (pszCurLine[nIdx+2]==L'%' || (bAllowDot && isDot(pszCurLine[nIdx+2]))
				|| !wcsncmp(pszCurLine+nIdx+2,szPercentEng,nPercentEngLen)
				|| !wcsncmp(pszCurLine+nIdx+2,szPercentRus,nPercentRusLen)))
		{
			nProgress = 10*(pszCurLine[nIdx] - L'0') + (pszCurLine[nIdx+1] - L'0');
		}
		else if (pszCurLine[nIdx+1]==L'%' || (bAllowDot && isDot(pszCurLine[nIdx+1]))
			|| !wcsncmp(pszCurLine+nIdx+1,szPercentEng,nPercentEngLen)
			|| !wcsncmp(pszCurLine+nIdx+1,szPercentRus,nPercentRusLen))
		{
			nProgress = (pszCurLine[nIdx] - L'0');
		}
	}

	if (nProgress != -1)
	{
		mp_RCon->setLastConsoleProgress(nProgress, true); // его обновляем всегда
	}
	else
	{
		DWORD nDelta = GetTickCount() - mp_RCon->m_Progress.LastConProgrTick;
		if (nDelta < CONSOLEPROGRESSTIMEOUT) // Если таймаут предыдущего значения еще не наступил
			nProgress = mp_RCon->m_Progress.ConsoleProgress; // возъмем предыдущее значение
		mp_RCon->setLastConsoleProgress(-1, false); // его обновляем всегда
	}

	return nProgress;
}
