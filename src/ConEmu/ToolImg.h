
/*
Copyright (c) 2014-present Maximus5
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

class CToolImg
{
protected:
	HBITMAP  mh_Bmp;
	HDC      mh_BmpDc;
	HBITMAP  mh_OldBmp;
	int      mn_BtnWidth, mn_BtnHeight;
	COLORREF mcr_Background;
	int      mn_BtnCount, mn_MaxBtnCount;
	LPRECT   mprc_Btns;
	bool     mb_HasAlpha;

	#if 0
	struct ButtonFieldInfo
	{
		LPCWSTR szName; int nWidth, nHeight;
	};
	#endif

	struct ButtonRowInfo
	{
		// Size of one button in a row
		int nWidth, nHeight;
		// Start Y coord
		int nY;
		// Count of buttons in a row (==1?)
		int nCount;
	};

	void FreeDC();
	void FreeBMP();

	int  AddBitmap(HBITMAP hbm, int iNumBtns);
	bool CreateField(int nImgWidth, int nImgHeight, COLORREF clrBackground);
	static bool PaintBitmap(HBITMAP hbmSrc, int nSrcWidth, int nSrcHeight, HDC hdcDst, int nDstX, int nDstY, int nDstWidth, int nDstHeight);

	bool CreateButtonField(LPCWSTR szImgRes, COLORREF clrBackground, ButtonRowInfo* pBtns, int nRowCount, bool bHasAlpha);

	#ifdef __GNUC__
	AlphaBlend_t GdiAlphaBlend;
	#endif

	#if 0
	bool CreateButtonField(COLORREF clrBackground, ButtonFieldInfo* pBtns, int nBtnCount);
	#endif

public:
	CToolImg();
	~CToolImg();

	HBITMAP GetBitmap();

	bool PaintButton(int iBtn, HDC hdcDst, int nDstX, int nDstY, int nDstWidth, int nDstHeight) const;
	bool PaintButtonTransparent(int iBtn, HDC dc, int x, int y) const;
	bool GetSizeForHeight(int nPreferHeight, int& nDispW, int& nDispH);

	/*
	HBITMAP LoadImageForWindow(HWND hwnd, HINSTANCE hinst, INT_PTR resId, int iStdWidth, int iStdHeight, COLORREF clrBackground, int iNumMaps, COLORREF from, COLORREF to, ...);
	*/

public:
	bool Create(int nBtnWidth, int nBtnHeight, int nMaxCount, COLORREF clrBackground);
	int  AddButtons(HINSTANCE hinst, INT_PTR resId, int iNumBtns);
	int  AddButtonsMapped(HINSTANCE hinst, INT_PTR resId, int iNumBtns, int iNumMaps, COLORREF from, COLORREF to, ...);

public:
	static CToolImg* CreateDonateButton();
	static CToolImg* CreateFlattrButton();
	static CToolImg* CreateSearchButton();
};
