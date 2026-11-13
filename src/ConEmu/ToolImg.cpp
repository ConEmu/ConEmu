
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

#define HIDE_USE_EXCEPTION_INFO

#define SHOWDEBUGSTR

#include "../common/defines.h"
#include <commctrl.h>
#include "Header.h"
#include "DwmHelper.h"
#include "ToolImg.h"


CToolImg::CToolImg()
{
	mh_Bmp = nullptr;
	mh_BmpDc = nullptr;
	mh_OldBmp = nullptr;
	mn_BtnWidth = 14; mn_BtnHeight = 14;
	mcr_Background = GetSysColor(COLOR_BTNFACE);
	mn_BtnCount = 0;
	mn_MaxBtnCount = 0;
	mprc_Btns = nullptr;
	mb_HasAlpha = false;

	#ifdef __GNUC__
	HMODULE hGdi32 = GetModuleHandle(L"gdi32.dll");
	GdiAlphaBlend = (AlphaBlend_t)(hGdi32 ? GetProcAddress(hGdi32, "GdiAlphaBlend") : nullptr);
	#endif
}

CToolImg::~CToolImg()
{
	FreeDC();
	FreeBMP();
}

HBITMAP CToolImg::GetBitmap()
{
	// Free memory allocated for temp DC
	FreeDC();
	// Bitmap is ready?
	return mh_Bmp;
}

void CToolImg::FreeDC()
{
	if (mh_BmpDc)
	{
		SelectObject(mh_BmpDc, mh_OldBmp);
		mh_OldBmp = nullptr;
		DeleteDC(mh_BmpDc);
		mh_BmpDc = nullptr;
	}

	SafeFree(mprc_Btns);
}

void CToolImg::FreeBMP()
{
	if (mh_Bmp)
	{
		DeleteObject(mh_Bmp);
		mh_Bmp = nullptr;
	}

	mn_BtnCount = 0;
	SafeFree(mprc_Btns);
}

bool CToolImg::Create(int nBtnWidth, int nBtnHeight, int nMaxCount, COLORREF clrBackground)
{
	// Ensure there is no old handles
	FreeDC();
	FreeBMP();

	if (nBtnWidth <= 0 || nBtnHeight <= 0 || nMaxCount <= 0)
		return false;

	// Init vars
	mn_BtnWidth = nBtnHeight; mn_BtnHeight = nBtnHeight;
	mn_BtnCount = 0; mn_MaxBtnCount = nMaxCount;
	mcr_Background = clrBackground;

	return CreateField(nBtnWidth*nMaxCount, nBtnHeight, clrBackground);
}

bool CToolImg::CreateField(int nImgWidth, int nImgHeight, COLORREF clrBackground)
{
	_ASSERTE(mh_BmpDc == nullptr);

	int nBPP = -1;
	bool bComp = (gnOsVer < 0x600);
	if (!bComp)
	{
		mh_BmpDc = CreateCompatibleDC(nullptr);
		nBPP = GetDeviceCaps(mh_BmpDc, BITSPIXEL);
		if (nBPP < 32)
		{
			bComp = true;
			DeleteDC(mh_BmpDc);
			mh_BmpDc = nullptr;
		}
	}

	HDC hScreen = bComp ? GetDC(nullptr) : nullptr;

	// Create memory DC
	if (!mh_BmpDc)
		mh_BmpDc = CreateCompatibleDC(hScreen);

	if (!mh_BmpDc)
	{
		_ASSERTE(mh_BmpDc!=nullptr);
	}
	else
	{
		// Create memory bitmap (WinXP and lower toolbar has some problem with true-color buttons, so - 8bit/compatible)
		if (bComp)
			mh_Bmp = CreateCompatibleBitmap(hScreen, nImgWidth, nImgHeight);
		else
			mh_Bmp = CreateBitmap(nImgWidth, nImgHeight, 1, 32, nullptr);
	}

	if (hScreen)
		ReleaseDC(nullptr, hScreen);

	if (!mh_Bmp)
	{
		FreeDC();
		return false;
	}

	mh_OldBmp = (HBITMAP)SelectObject(mh_BmpDc, mh_Bmp);

	// Prefill with background
	RECT rcFill = {0, 0, nImgWidth, nImgHeight};
	HBRUSH hbr = CreateSolidBrush(clrBackground);
	FillRect(mh_BmpDc, &rcFill, hbr);
	DeleteObject(hbr);

	return true;
}

CToolImg* CToolImg::CreateDonateButton()
{
	ButtonRowInfo Btns[] = {
		{74,  21},
		{92,  26},
		{111, 31},
		{148, 42},
	};
	int nY = 0;
	for (size_t i = 0; i < countof(Btns); i++)
	{
		Btns[i].nY = nY; Btns[i].nCount = 1;
		nY += 1 + Btns[i].nHeight;
	}

	CToolImg* p = new CToolImg();
	if (p && !p->CreateButtonField(MAKEINTRESOURCE(IDB_DONATE), 0, Btns, (int)countof(Btns), true))
	{
		#ifdef _DEBUG
		DWORD nErr = GetLastError();
		_ASSERTE(FALSE && "Resource loading failed?");
		#endif
		SafeDelete(p);
	}
	if (p && CDwmHelper::IsWindowsDarkMode())
	{
		DIBSECTION bitmap = {};
		if (GetObject(p->mh_Bmp, sizeof(bitmap), &bitmap) == sizeof(bitmap)
			&& bitmap.dsBm.bmBitsPixel == 32 && bitmap.dsBm.bmBits)
		{
			// Preserve the original silhouette and shading in the premultiplied sprite.
			auto row = static_cast<BYTE*>(bitmap.dsBm.bmBits);
			for (LONG y = 0; y < bitmap.dsBm.bmHeight; ++y, row += bitmap.dsBm.bmWidthBytes)
			{
				auto pixels = reinterpret_cast<RGBQUAD*>(row);
				for (LONG x = 0; x < bitmap.dsBm.bmWidth; ++x)
				{
					auto& pixel = pixels[x];
					const int alpha = pixel.rgbReserved;
					const int luminance = (pixel.rgbRed * 54 + pixel.rgbGreen * 183 + pixel.rgbBlue * 19) / 256;
					const int shade = (alpha * 36 + std::max(0, alpha - luminance) * 210) / 255;
					pixel.rgbRed = static_cast<BYTE>(shade);
					pixel.rgbGreen = static_cast<BYTE>(shade);
					pixel.rgbBlue = static_cast<BYTE>(shade);
				}
			}
		}
	}
	return p;
}

CToolImg* CToolImg::CreateFlattrButton()
{
	ButtonRowInfo Btns[] = {
		{55,  19},
		{69,  24},
		{82,  29},
		{110, 38},
	};
	int nY = 0;
	for (size_t i = 0; i < countof(Btns); i++)
	{
		Btns[i].nY = nY; Btns[i].nCount = 1;
		nY += 1 + Btns[i].nHeight;
	}

	CToolImg* p = new CToolImg();
	if (p && !p->CreateButtonField(MAKEINTRESOURCE(IDB_FLATTR), 0, Btns, (int)countof(Btns), true))
	{
		#ifdef _DEBUG
		DWORD nErr = GetLastError();
		_ASSERTE(FALSE && "Resource loading failed?");
		#endif
		SafeDelete(p);
	}
	return p;
}

CToolImg* CToolImg::CreateSearchButton()
{
	ButtonRowInfo Btns[] = {
		{11,  11},
		{14,  14},
		{18,  18},
		{20,  20},
		{23,  23},
	};
	int nY = 0;
	for (size_t i = 0; i < countof(Btns); i++)
	{
		Btns[i].nY = nY; Btns[i].nCount = 1;
		nY += 1 + Btns[i].nHeight;
	}

	CToolImg* p = new CToolImg();
	if (p && !p->CreateButtonField(MAKEINTRESOURCE(IDB_SEARCH), 0, Btns, (int)countof(Btns), true))
	{
		#ifdef _DEBUG
		DWORD nErr = GetLastError();
		_ASSERTE(FALSE && "Resource loading failed?");
		#endif
		SafeDelete(p);
	}
	return p;
}

#if 0
bool CToolImg::CreateButtonField(COLORREF clrBackground, ButtonFieldInfo* pBtns, int nBtnCount)
{
	FreeDC();
	FreeBMP();

	mprc_Btns = (LPRECT)calloc(nBtnCount, sizeof(*mprc_Btns));
	if (!mprc_Btns)
		return false;
	mn_MaxBtnCount = nBtnCount;

	int nFieldWidth = pBtns[0].nWidth;
	int nFieldHeight = pBtns[0].nHeight;
	for (int i = 1; i < nBtnCount; i++)
	{
		nFieldWidth = std::max(nFieldWidth, pBtns[i].nWidth);
		nFieldHeight += 1 + pBtns[i].nHeight;
	}

	if (!CreateField(nFieldWidth, nFieldHeight, clrBackground))
		return false;

	//COLORMAP colorMap = {0xC0C0C0, clrBackground/*GetSysColor(COLOR_BTNFACE)*/};

	bool bRc = true;

	_ASSERTE(mn_BtnCount == 0);

	int nDstY = 0;
	DWORD nErrCode = 0;

	for (int i = 0; i < nBtnCount; i++)
	{
		//HBITMAP hbm = CreateMappedBitmap(g_hInstance, (INT_PTR)pBtns[i].szName, 0, &colorMap, 1);
		HBITMAP hbm = (HBITMAP)LoadImage(g_hInstance, pBtns[i].szName, IMAGE_BITMAP, 0, 0, LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);
		if (hbm == nullptr)
		{
			nErrCode = GetLastError();
			bRc = false;
			continue;
		}
		else
		{
			RECT rc = {0, nDstY, pBtns[i].nWidth, nDstY+pBtns[i].nHeight};
			mprc_Btns[i] = rc;

			if (!PaintBitmap(hbm, pBtns[i].nWidth, pBtns[i].nHeight, mh_BmpDc, 0, nDstY, pBtns[i].nWidth, pBtns[i].nHeight))
			{
				nErrCode = GetLastError();
				bRc = false;
			}

			DeleteObject(hbm);
		}

		nDstY += 1 + pBtns[i].nHeight;
	}

	#ifdef _DEBUG
	//SaveImageEx(L"T:\\BtnField.png", mh_Bmp);
	#endif

	mn_BtnCount = mn_MaxBtnCount;

	return true;
}
#endif

bool CToolImg::CreateButtonField(LPCWSTR szImgRes, const COLORREF clrBackground, ButtonRowInfo* pBtns, const int nRowCount, const bool bHasAlpha)
{
	FreeDC();
	FreeBMP();

	mb_HasAlpha = bHasAlpha;

	mprc_Btns = static_cast<LPRECT>(calloc(nRowCount, sizeof(*mprc_Btns)));
	if (!mprc_Btns)
		return false;
	mn_MaxBtnCount = nRowCount;

	#if defined(_DEBUG)
	int nFieldWidth = pBtns[0].nWidth;
	int nFieldHeight = pBtns[0].nHeight;
	for (int i = 1; i < nRowCount; i++)
	{
		nFieldWidth = std::max(nFieldWidth, pBtns[i].nWidth);
		nFieldHeight += 1 + pBtns[i].nHeight;
	}
	#endif

	bool bRc = true;

	_ASSERTE(mn_BtnCount == 0);
	_ASSERTE(mh_Bmp == nullptr);

	DWORD nErrCode = 0;
	UINT flags = bHasAlpha ? (LR_CREATEDIBSECTION) : (LR_LOADTRANSPARENT|LR_LOADMAP3DCOLORS);
	mh_Bmp = (HBITMAP)LoadImage(g_hInstance, szImgRes, IMAGE_BITMAP, 0, 0, flags);

	if (mh_Bmp == nullptr)
	{
		nErrCode = GetLastError();
		#ifdef _DEBUG
		// ReSharper disable once CppLocalVariableMayBeConst
		HRSRC hFind = FindResource(g_hInstance, szImgRes, RT_BITMAP);
		// ReSharper disable once CppLocalVariableMayBeConst
		HGLOBAL hLoad = hFind ? LoadResource(g_hInstance, hFind) : nullptr;
		// ReSharper disable once CppDeclaratorNeverUsed
		DWORD nSize = hFind ? SizeofResource(g_hInstance, hFind) : 0;
		// ReSharper disable once CppDeclaratorNeverUsed
		BITMAPINFO* ptr = hLoad ? static_cast<BITMAPINFO*>(LockResource(hLoad)) : nullptr;
		#endif
		_ASSERTE(mh_Bmp!=nullptr && "LoadImage from app resources was failed");
		bRc = false;
	}
	else
	{
		bRc = true;
	}

	if (bRc)
	{
		for (int i = 0; i < nRowCount; i++)
		{
			const RECT rc = {0, pBtns[i].nY, pBtns[i].nWidth, pBtns[i].nY + pBtns[i].nHeight};
			mprc_Btns[i] = rc;
		}

		#ifdef _DEBUG
		//SaveImageEx(L"T:\\BtnField.png", mh_Bmp);
		#endif

		mn_BtnCount = mn_MaxBtnCount;
	}

	return true;
}

bool CToolImg::GetSizeForHeight(int nPreferHeight, int& nDispW, int& nDispH)
{
	if (nPreferHeight <= 0)
		return false;
	AssertThisRet(false);
	if (mn_BtnCount <= 0 || mprc_Btns == nullptr)
		return false;

	// 100%, 125%, 150%, 200%, and some physical sized for search button
	// _ASSERTE(mn_BtnCount == 4);

	// Find max size (for 200% dpi)
	int nMaxW = 0, nMaxH = 0;
	for (int i = 0; i < mn_BtnCount; i++)
	{
		const int h = (mprc_Btns[i].bottom - mprc_Btns[i].top);
		if (h > nMaxH)
		{
			nMaxH = h;
			nMaxW = (mprc_Btns[i].right - mprc_Btns[i].left);
		}
	}

	if (nMaxW <= 0 || nMaxH <= 0)
		return false;

	const bool byWidth = false; // (nMaxW > nMaxH);

	const int nNeed = nPreferHeight; // (byWidth ? nMaxW : nMaxH) * nDisplayDpi * 100 / (96 * 200);
	nDispW = 0; nDispH = 0;

	for (int i = 0; i < mn_BtnCount; i++)
	{
		int w = (mprc_Btns[i].right - mprc_Btns[i].left);
		int h = (mprc_Btns[i].bottom - mprc_Btns[i].top);
		// ReSharper disable once CppUnreachableCode
		const int n = (byWidth ? w : h);

		// ReSharper disable once CppUnreachableCode
		if ((n <= nNeed) && (n > (byWidth ? nDispW : nDispH)))
		{
			nDispW = w; nDispH = h;
		}
		// Continue enum, may be better choice will be found
	}

	return (nDispH > 0);
}

bool CToolImg::PaintButtonTransparent(int iBtn, HDC dc, int x, int y) const
{
	// Toolbar bitmaps are a uniform horizontal strip. GetBitmap releases the
	// construction DC, so use a temporary source DC for custom drawing.
	if (!mh_Bmp || iBtn < 0 || iBtn >= mn_BtnCount) return false;
	HDC source = CreateCompatibleDC(dc);
	if (!source) return false;
	HGDIOBJ old = SelectObject(source, mh_Bmp);
	const bool result = GdiTransparentBlt(dc, x, y, mn_BtnWidth, mn_BtnHeight,
		source, iBtn * mn_BtnWidth, 0, mn_BtnWidth, mn_BtnHeight, mcr_Background) != FALSE;
	SelectObject(source, old); DeleteDC(source);
	return result;
}

bool CToolImg::PaintButton(int iBtn, HDC hdcDst, const int nDstX, const int nDstY, const int nDstWidth, const int nDstHeight) const
{
	if (!mh_Bmp || !mprc_Btns || (iBtn >= mn_BtnCount) || (mn_BtnCount <= 0))
		return false;

	if (iBtn < 0)
	{
		int iFoundHeight = (mprc_Btns[0].bottom - mprc_Btns[0].top);
		iBtn = 0;

		for (int i = 1; i < mn_BtnCount; i++)
		{
			const int h = (mprc_Btns[i].bottom - mprc_Btns[i].top);
			if ((h <= nDstHeight) && (h > iFoundHeight))
			{
				iBtn = i;
				iFoundHeight = h;
			}
			// Continue enum, may be better choice will be found
		}
	}

	const RECT rc = mprc_Btns[iBtn];

	int y = nDstY;
	const int w = std::min<int>(nDstWidth, (rc.right - rc.left));
	int h = nDstHeight;

	if (nDstHeight > (rc.bottom - rc.top))
	{
		h = rc.bottom - rc.top;
		y += ((nDstHeight - h) >> 1);
	}

	bool bRc = false;

	if (mh_BmpDc != nullptr)
	{
		// Due to some adjustment errors after dpi downscaling the cropping may be occured, so use StretchBlt
		if ((rc.bottom - rc.top) > h)
			bRc = (StretchBlt(hdcDst, nDstX, y, w, h, mh_BmpDc, rc.left, rc.top, rc.right-rc.left+1, rc.bottom-rc.top+1, SRCCOPY) != FALSE);
		else
			bRc = (BitBlt(hdcDst, nDstX, y, w, h, mh_BmpDc, rc.left, rc.top, SRCCOPY) != FALSE);
	}
	else
	{
		// ReSharper disable once CppLocalVariableMayBeConst
		HDC hCompDC = CreateCompatibleDC(hdcDst);
		if (hCompDC)
		{
			// ReSharper disable once CppLocalVariableMayBeConst
			HBITMAP hOld = static_cast<HBITMAP>(SelectObject(hCompDC, mh_Bmp));

			// ReSharper disable once CppRedundantBooleanExpressionArgument
			if (mb_HasAlpha && (GdiAlphaBlend != nullptr))  // NOLINT(clang-diagnostic-tautological-pointer-compare)
			{
				const BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
				// -- int iPrev = SetStretchBltMode(hdc, STRETCH_HALFTONE);
				bRc = (GdiAlphaBlend(hdcDst, nDstX, y, w, h, hCompDC, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, bf) != FALSE);
				// -- SetStretchBltMode(hdc, iPrev);
			}
			else if ((rc.bottom - rc.top) > h)
			{
				// Due to some adjustment errors after dpi downscaling the cropping may be occurred, so use StretchBlt
				// -- int iPrev = SetStretchBltMode(hdc, STRETCH_HALFTONE);
				bRc = (StretchBlt(hdcDst, nDstX, y, w, h, hCompDC, rc.left, rc.top, rc.right-rc.left+1, rc.bottom-rc.top+1, SRCCOPY) != FALSE);
				// -- SetStretchBltMode(hdc, iPrev);
			}
			else
			{
				bRc = (BitBlt(hdcDst, nDstX, y, w, h, hCompDC, rc.left, rc.top, SRCCOPY) != FALSE);
			}

			// ReSharper disable once CppDeclaratorNeverUsed
			DEBUGTEST(DWORD nErr = GetLastError());

			if (hOld && hCompDC)
				SelectObject(hCompDC, hOld);
			DeleteDC(hCompDC);
		}
	}

	return bRc;
}

int CToolImg::AddBitmap(HBITMAP hbm, const int iNumBtns)
{
	BITMAP bm = {};
	const int ccb = hbm ? GetObject(hbm, sizeof(bm), &bm) : -1;
	if ((ccb <= 0) || (bm.bmWidth <= 0) || (bm.bmHeight == 0))
	{
		_ASSERTE((ccb > 0) && "Invalid bitmap loaded?");
		return 0;
	}

	const int iSrcBtnWidth = bm.bmWidth / iNumBtns;
	int iAdded = iNumBtns;
	if (iAdded > (mn_MaxBtnCount - mn_BtnCount))
	{
		_ASSERTE((iAdded <= (mn_MaxBtnCount - mn_BtnCount)) && "CToolImg was overflowed");
		iAdded = (mn_MaxBtnCount - mn_BtnCount);
		if (iAdded < 1)
		{
			_ASSERTE((mn_MaxBtnCount > mn_BtnCount) && "Button count overflow");
			return 0;
		}
	}
	const int iSrcHeight = _abs(bm.bmHeight);
	const int iSrcWidth = iAdded * iSrcBtnWidth;

	// Where we must paint to
	const int x = mn_BtnCount * mn_BtnWidth;

	#ifdef _DEBUG
	//SaveImageEx(L"C:\\ConEmu\\Pre.png", mh_Bmp);
	//SaveImageEx(L"C:\\ConEmu\\Paint.png", hbm);
	#endif

	// And go
	const bool blitRc = PaintBitmap(hbm, iSrcWidth, iSrcHeight, mh_BmpDc, x, 0, iAdded*mn_BtnWidth, mn_BtnHeight);

	#ifdef _DEBUG
	//SaveImageEx(L"C:\\ConEmu\\Post.png", mh_Bmp);
	#endif

	// Check result
	if (blitRc)
		mn_BtnCount += iAdded;
	else
		iAdded = 0;

	_ASSERTE(mn_BtnCount <= mn_MaxBtnCount);

	// Done
	return iAdded;
}

bool CToolImg::PaintBitmap(HBITMAP hbmSrc, const int nSrcWidth, const int nSrcHeight,
	HDC hdcDst, const int nDstX, const int nDstY, const int nDstWidth, const int nDstHeight)
{
	// ReSharper disable once CppLocalVariableMayBeConst
	HDC hdc = CreateCompatibleDC(nullptr);
	if (!hdc)
		return false;
	// ReSharper disable once CppLocalVariableMayBeConst
	HBITMAP hOld = static_cast<HBITMAP>(SelectObject(hdc, hbmSrc));

	// And go
	BOOL blitRc;
	if (nSrcHeight != nDstHeight)
	{
		// Need stretch
		blitRc = StretchBlt(hdcDst, nDstX, nDstY, nDstWidth, nDstHeight, hdc, 0, 0, nSrcWidth, nSrcHeight, SRCCOPY);
	}
	else
	{
		// Just blit
		blitRc = BitBlt(hdcDst, nDstX, nDstY, nDstWidth, nDstHeight, hdc, 0, 0, SRCCOPY);
	}

	SelectObject(hdc, hOld);
	DeleteDC(hdc);

	return (blitRc != FALSE);
}

int CToolImg::AddButtons(HINSTANCE hInst, INT_PTR resId, const int iNumBtns)
{
	int iAdded = 0;
	// ReSharper disable once CppLocalVariableMayBeConst
	HBITMAP hbm = static_cast<HBITMAP>(LoadImage(hInst, reinterpret_cast<LPCWSTR>(resId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
	if (hbm)
	{
		#ifdef _DEBUG
		//SaveImageEx(L"T:\\AddButtons.png", hbm);
		#endif
		iAdded = AddBitmap(hbm, iNumBtns);
		DeleteObject(hbm);
	}
	else
	{
		_ASSERTE(FALSE && "LoadImage failed");
	}
	return iAdded;
}

int CToolImg::AddButtonsMapped(HINSTANCE hInst, INT_PTR resId, const int iNumBtns, const int iNumMaps, const COLORREF from, const COLORREF to, ...)
{
	int iAdded = 0;
	_ASSERTE(iNumMaps <= 1);
	COLORMAP colorMap = {from, to};
	// ReSharper disable once CppLocalVariableMayBeConst
	HBITMAP hbm = CreateMappedBitmap(hInst, resId, 0, &colorMap, 1);
	if (hbm)
	{
		iAdded = AddBitmap(hbm, iNumBtns);
		DeleteObject(hbm);
	}
	else
	{
		_ASSERTE(FALSE && "CreateMappedBitmap failed");
	}
	return iAdded;
}

/*
HBITMAP CToolImg::LoadImageForWindow(HWND hwnd, HINSTANCE hinst, INT_PTR resId, int iStdWidth, int iStdHeight, COLORREF clrBackground, int iNumMaps, COLORREF from, COLORREF to, ...)
{
	FreeDC();
	FreeBMP();

	TODO("hwnd must be used to determine monitor DPI (per/mon dpi)");
	int nDisplayDpi = gpSetCls->QueryDpi();

	if (!Create(iStdWidth * nDisplayDpi / 96, iStdHeight * nDisplayDpi / 96, 1, clrBackground))
		return nullptr;

	int iAdded = 0;
	_ASSERTE(iNumMaps <= 1);
	COLORMAP colorMap = {from, to};
	HBITMAP hbm = CreateMappedBitmap(hinst, resId, 0, &colorMap, 1);
	if (hbm)
	{
		iAdded = AddBitmap(hbm, 1);
		DeleteObject(hbm);
	}
	else
	{
		_ASSERTE(FALSE && "CreateMappedBitmap failed");
	}

	return mh_Bmp;
}
*/
