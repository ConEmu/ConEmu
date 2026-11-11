
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
#include "Header.h"
#include "DwmApi_Part.h"
#include "ConEmu.h"
#include "Options.h"




/* *************************** */

CDwmHelper::CDwmHelper()
{
	InitDwm();
	DrawType();
}

CDwmHelper::~CDwmHelper()
{
	if (mh_DwmApi)
	{
		FreeLibrary(mh_DwmApi);
		mh_DwmApi = nullptr;

		mb_DwmAllowed = false;
		dwm = {};
	}

	if (mh_UxTheme)
	{
		if (mb_BufferedAllowed && ux._BufferedPaintUnInit)
		{
			ux._BufferedPaintUnInit();
		}

		FreeLibrary(mh_UxTheme);
		mh_UxTheme = nullptr;

		ux = {};
	}
}

FrameDrawStyle CDwmHelper::DrawType()
{
	if (IsWin8())
	{
		m_DrawType = fdt_Win8;
	}
	else if (IsGlass())
	{
		m_DrawType = fdt_Aero;
	}
	else if (IsThemed())
	{
		m_DrawType = fdt_Themed;
	}
	else
	{
		m_DrawType = fdt_Win2k;
	}

	return m_DrawType;
}

void CDwmHelper::InitDwm()
{
	BOOL lbDbg;

	mh_User32 = GetModuleHandle(L"User32.dll");

	//gOSVer.dwOSVersionInfoSize = sizeof(gOSVer);
	//GetVersionEx(&gOSVer);
	if (IsWin6())
	{
		user._ChangeWindowMessageFilter = (USER::ChangeWindowMessageFilter_t)GetProcAddress(mh_User32, "ChangeWindowMessageFilter");
		if (user._ChangeWindowMessageFilter)
		{
			lbDbg = user._ChangeWindowMessageFilter(WM_DWMSENDICONICTHUMBNAIL, MSGFLT_ADD);
			lbDbg = user._ChangeWindowMessageFilter(WM_DWMSENDICONICLIVEPREVIEWBITMAP, MSGFLT_ADD);
			UNREFERENCED_PARAMETER(lbDbg);
		}

		mh_DwmApi = LoadLibrary(_T("dwmapi.dll"));
		if (mh_DwmApi)
		{
			// Vista+
			dwm._DwmIsCompositionEnabled = (DWM::DwmIsCompositionEnabled_t)GetProcAddress(mh_DwmApi, "DwmIsCompositionEnabled");
			dwm._DwmSetWindowAttribute = (DWM::DwmSetWindowAttribute_t)GetProcAddress(mh_DwmApi, "DwmSetWindowAttribute");
			dwm._DwmGetWindowAttribute = (DWM::DwmGetWindowAttribute_t)GetProcAddress(mh_DwmApi, "DwmGetWindowAttribute");
			dwm._DwmExtendFrameIntoClientArea = (DWM::DwmExtendFrameIntoClientArea_t)GetProcAddress(mh_DwmApi, "DwmExtendFrameIntoClientArea");
			dwm._DwmDefWindowProc = (DWM::DwmDefWindowProc_t)GetProcAddress(mh_DwmApi, "DwmDefWindowProc");
			dwm._DwmSetIconicThumbnail = (DWM::DwmSetIconicThumbnail_t)GetProcAddress(mh_DwmApi, "DwmSetIconicThumbnail");
			dwm._DwmSetIconicLivePreviewBitmap = (DWM::DwmSetIconicLivePreviewBitmap_t)GetProcAddress(mh_DwmApi, "DwmSetIconicLivePreviewBitmap");
			dwm._DwmInvalidateIconicBitmaps = (DWM::DwmInvalidateIconicBitmaps_t)GetProcAddress(mh_DwmApi, "DwmInvalidateIconicBitmaps");
			dwm._DwmEnableBlurBehindWindow = (DWM::DwmEnableBlurBehindWindow_t)GetProcAddress(mh_DwmApi, "DwmEnableBlurBehindWindow");

			mb_DwmAllowed = (dwm._DwmIsCompositionEnabled != nullptr)
				&& (dwm._DwmGetWindowAttribute != nullptr) && (dwm._DwmSetWindowAttribute != nullptr)
				&& (dwm._DwmExtendFrameIntoClientArea != nullptr)
				&& (dwm._DwmDefWindowProc != nullptr);
			if (mb_DwmAllowed)
				mb_EnableGlass = true;
		}
	}

	if (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1))
	{
		mh_UxTheme = LoadLibrary(_T("UxTheme.dll"));
		if (mh_UxTheme)
		{
			// XP+
			ux._IsAppThemed = (UX::AppThemed_t)GetProcAddress(mh_UxTheme, "IsAppThemed");
			ux._IsThemeActive = (UX::AppThemed_t)GetProcAddress(mh_UxTheme, "IsThemeActive");
			ux._OpenThemeData = (UX::OpenThemeData_t)GetProcAddress(mh_UxTheme, "OpenThemeData");
			ux._CloseThemeData = (UX::CloseThemeData_t)GetProcAddress(mh_UxTheme, "CloseThemeData");
			ux._DrawThemeBackground = (UX::DrawThemeBackground_t)GetProcAddress(mh_UxTheme, "DrawThemeBackground");
			ux._DrawThemeEdge = (UX::DrawThemeEdge_t)GetProcAddress(mh_UxTheme, "DrawThemeEdge");
			ux._GetThemeMargins = (UX::GetThemeMargins_t)GetProcAddress(mh_UxTheme, "GetThemeMargins");
			ux._GetThemePartSize = (UX::GetThemePartSize_t)GetProcAddress(mh_UxTheme, "GetThemePartSize");
			ux._GetThemePosition = (UX::GetThemePosition_t)GetProcAddress(mh_UxTheme, "GetThemePosition");
			ux._GetThemeSysSize = (UX::GetThemeSysSize_t)GetProcAddress(mh_UxTheme, "GetThemeSysSize");
			ux._GetThemeBackgroundContentRect = (UX::GetThemeBackgroundContentRect_t)GetProcAddress(mh_UxTheme, "GetThemeBackgroundContentRect");
			ux._SetThemeAppProperties = (UX::SetThemeAppProperties_t)GetProcAddress(mh_UxTheme, "SetThemeAppProperties");
			// Vista+
			ux._BufferedPaintInit = (UX::BufferedPaintInit_t)GetProcAddress(mh_UxTheme, "BufferedPaintInit");
			ux._BufferedPaintUnInit = (UX::BufferedPaintInit_t)GetProcAddress(mh_UxTheme, "BufferedPaintUnInit");
			ux._BeginBufferedPaint = (UX::BeginBufferedPaint_t)GetProcAddress(mh_UxTheme, "BeginBufferedPaint");
			ux._BufferedPaintSetAlpha = (UX::BufferedPaintSetAlpha_t)GetProcAddress(mh_UxTheme, "BufferedPaintSetAlpha");
			ux._EndBufferedPaint = (UX::EndBufferedPaint_t)GetProcAddress(mh_UxTheme, "EndBufferedPaint");
			ux._DrawThemeTextEx = (UX::DrawThemeTextEx_t)GetProcAddress(mh_UxTheme, "DrawThemeTextEx");
			ux._SetWindowTheme = (UX::SetWindowTheme_t)GetProcAddress(mh_UxTheme, "SetWindowTheme");

			mb_ThemeAllowed = (ux._IsAppThemed != nullptr) && (ux._IsThemeActive != nullptr);
			if (mb_ThemeAllowed)
			{
				mb_EnableTheming = true;
				if (ux._BufferedPaintInit && ux._BufferedPaintUnInit)
				{
					HRESULT hr = ux._BufferedPaintInit();
					mb_BufferedAllowed = SUCCEEDED(hr);
				}
			}
		}
	}

	if (IsWin10())
	{
		user._AdjustWindowRectExForDpi = (USER::AdjustWindowRectExForDpi_t)GetProcAddress(mh_User32, "AdjustWindowRectExForDpi");
	}
}

bool CDwmHelper::IsDwm()
{
	if (!mb_DwmAllowed)
		return false;
	BOOL composition_enabled = FALSE;
	bool isDwm = (dwm._DwmIsCompositionEnabled(&composition_enabled) == S_OK) &&
		composition_enabled;
	return isDwm;
}

bool CDwmHelper::IsDwmAllowed()
{
	return mb_DwmAllowed;
}

bool CDwmHelper::IsGlass()
{
	if (IsWindows8)
		return false;
	if (!mb_DwmAllowed || !mb_EnableGlass)
		return false;
	return IsDwm();
}

bool CDwmHelper::IsHighContrast()
{
	HIGHCONTRASTW contrast = {sizeof(contrast)};
	return SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0)
		&& (contrast.dwFlags & HCF_HIGHCONTRASTON);
}

bool CDwmHelper::IsWindowsDarkMode()
{
	DWORD light = 1, size = sizeof(light);
	return !IsHighContrast()
		&& RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
			L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size) == ERROR_SUCCESS
		&& light == 0;
}

void CDwmHelper::EnableDarkCaption(HWND window)
{
	if (!dwm._DwmSetWindowAttribute || gOSVer.dwBuildNumber < 22000)
		return;

	// DWMWA_USE_IMMERSIVE_DARK_MODE is absent from older Windows SDKs.
	constexpr DWORD immersiveDarkMode = 20;
	const BOOL enabled = TRUE;
	dwm._DwmSetWindowAttribute(window, immersiveDarkMode, &enabled, sizeof(enabled));
}

void CDwmHelper::ApplyPopupMenuTheme(HWND owner)
{
	// Ordinal 135 had a different signature before Windows 10 version 1903.
	if (!mh_UxTheme || gOSVer.dwMajorVersion < 10 || gOSVer.dwBuildNumber < 18362)
		return;

	enum class AppMode { Default, AllowDark, ForceDark };
	using SetAppMode = AppMode (WINAPI*)(AppMode);
	using AllowForWindow = bool (WINAPI*)(HWND, bool);
	using FlushMenus = void (WINAPI*)();
	const auto setMode = reinterpret_cast<SetAppMode>(GetProcAddress(mh_UxTheme, MAKEINTRESOURCEA(135)));
	const auto allow = reinterpret_cast<AllowForWindow>(GetProcAddress(mh_UxTheme, MAKEINTRESOURCEA(133)));
	const auto flush = reinterpret_cast<FlushMenus>(GetProcAddress(mh_UxTheme, MAKEINTRESOURCEA(136)));
	if (!setMode || !flush)
		return;

	const bool dark = IsWindowsDarkMode();
	static int lastMode = -1;
	const bool changed = (lastMode != int(dark));
	if (changed)
	{
		setMode(dark ? AppMode::ForceDark : AppMode::Default);
		lastMode = int(dark);
	}
	if (owner && allow)
		allow(owner, dark);
	if (changed)
		flush();
}

COLORREF CDwmHelper::WindowsAccentColor()
{
	DWORD color = 0, size = sizeof(color);
	if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\DWM",
			L"AccentColor", RRF_RT_REG_DWORD, nullptr, &color, &size) == ERROR_SUCCESS)
		return color & 0x00FFFFFF;
	return GetSysColor(COLOR_HIGHLIGHT);
}

COLORREF CDwmHelper::BlendColors(COLORREF base, COLORREF tint, unsigned tintPercent)
{
	const auto blend = [tintPercent](unsigned a, unsigned b) {
		return (a * (100 - tintPercent) + b * tintPercent) / 100;
	};
	return RGB(blend(GetRValue(base), GetRValue(tint)), blend(GetGValue(base), GetGValue(tint)),
		blend(GetBValue(base), GetBValue(tint)));
}

bool CDwmHelper::IsThemed()
{
	if (!mb_ThemeAllowed) // || !mb_EnableTheming)
		return false;
	return ux._IsAppThemed() && ux._IsThemeActive();
}

void CDwmHelper::EnableGlass(bool abGlass)
{
	mb_EnableGlass = abGlass && mb_DwmAllowed;
	OnUseGlass(mb_EnableGlass);
	CheckGlassAttribute();
	if (mb_DwmAllowed && !abGlass)
	{
		//TODO: При отключении Aero в окне остается "мусор" (что-то из альфа-канала)
	}
}

void CDwmHelper::EnableTheming(bool abTheme)
{
	mb_EnableTheming = abTheme && mb_ThemeAllowed;
	OnUseTheming(mb_EnableTheming);
	CheckGlassAttribute();
}

void CDwmHelper::SetWindowTheme(HWND hWnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList)
{
	if (!ux._SetWindowTheme)
		return;
	HRESULT hr = ux._SetWindowTheme(hWnd, pszSubAppName, pszSubIdList);
	UNREFERENCED_PARAMETER(hr);
}

void CDwmHelper::EnableBlurBehind(bool abBlurBehindClient)
{
	HRESULT hr = E_NOINTERFACE;
	if (dwm._DwmEnableBlurBehindWindow)
	{
		// Возможно, ghWndWork впоследствии будет отдельным окном (WS_POPUP?)
		// чтобы реализовать прозрачность только клиентской части
		HWND hWnd = ghWnd;
		_ASSERTE(ghWndWork!=nullptr);
		if (ghWndWork && !(GetWindowLong(ghWndWork, GWL_STYLE) & WS_CHILD))
			hWnd = ghWndWork;

		RECT rcWnd = {};
		if (abBlurBehindClient)
		{
			GetWindowRect(ghWndWork, &rcWnd);
			MapWindowPoints(nullptr, ghWnd, (LPPOINT)&rcWnd, 2);
		}

		// Create and populate the blur-behind structure.
		struct
		{
			DWORD dwFlags;
			BOOL fEnable;
			HRGN hRgnBlur;
			BOOL fTransitionOnMaximized;
		} bb = {0};

		// Specify blur-behind and blur region.
		bb.fEnable = abBlurBehindClient;
		//bb.hRgnBlur = abBlurBehindClient ? CreateRectRgn(rcWnd.left, rcWnd.top, rcWnd.right, rcWnd.bottom) : nullptr;
		//bb.fTransitionOnMaximized = true;
		bb.dwFlags = 1/*DWM_BB_ENABLE*/; // | (bb.hRgnBlur ? 2/*DWM_BB_BLURREGION*/ : 0); // | 4/*DWM_BB_TRANSITIONONMAXIMIZED*/;


		// Enable blur-behind.
		hr = dwm._DwmEnableBlurBehindWindow(hWnd, &bb);

		if (bb.hRgnBlur)
			DeleteObject(bb.hRgnBlur);
	}
	UNREFERENCED_PARAMETER(hr);
}

void CDwmHelper::CheckGlassAttribute()
{
	if (mb_DwmAllowed)
	{
		DWMNCRENDERINGPOLICY policy = mb_EnableGlass ? DWMNCRP_ENABLED : DWMNCRP_DISABLED;
		dwm._DwmSetWindowAttribute(ghWnd, DWMWA_NCRENDERING_POLICY,
			&policy, sizeof(DWMNCRENDERINGPOLICY));

		TODO("Replace IsDwm with IsGlass?");
		OnUseDwm(IsDwm());

		ExtendWindowFrame();
	}

	if (mb_ThemeAllowed)
	{
		//TODO: Включить или отключить темы XP+
		//_SetThemeAppProperties(mb_EnableTheming ? STAP_VALIDBITS : 0);
		//if (_SetWindowThemeNonClientAttributes)
		//	_SetWindowThemeNonClientAttributes(ghWnd, STAP_VALIDBITS, mb_EnableTheming ? STAP_VALIDBITS : 0);
		//static bool bRgnWasSet = false;
		//if (mb_EnableTheming)
		//{
		//	if (bRgnWasSet)
		//	{
		//		bRgnWasSet = false;
		//		SetWindowRgn(ghWnd, nullptr, TRUE);
		//	}
		//}
		//else
		//{
		//	if (!bRgnWasSet)
		//	{
		//		RECT wr; GetWindowRect(ghWnd, &wr); OffsetRect(&wr, -wr.left, -wr.top);
		//		HRGN rgn = CreateRectRgnIndirect(&wr);
		//		SetWindowRgn(ghWnd, rgn, TRUE);
		//	}
		//}
	}

	SetWindowPos(ghWnd, nullptr, 0, 0, 0, 0,
			SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
	RedrawWindow(ghWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

bool CDwmHelper::ExtendWindowFrame()
{
	if (!mb_DwmAllowed)
		return false;

	////dwmm.cxLeftWidth = 2;
	////dwmm.cxRightWidth = 2;
	//dwmm.cyTopHeight = GetDwmClientRectTopOffset();
	////dwmm.cyBottomHeight = 0;

	RECT rcMargins = {0, GetDwmClientRectTopOffset()};

	return ExtendWindowFrame(ghWnd, rcMargins);
}

bool CDwmHelper::ExtendWindowFrame(HWND hWnd, const RECT& rcMargins)
{
	if (!mb_DwmAllowed)
		return false;

	MARGINS dwmm = {0};
	dwmm.cxLeftWidth = rcMargins.left;
	dwmm.cxRightWidth = rcMargins.right;
	dwmm.cyTopHeight = rcMargins.top;
	dwmm.cyBottomHeight = rcMargins.bottom;

	HRESULT hr = dwm._DwmExtendFrameIntoClientArea(hWnd, &dwmm);

	//SetWindowThemeNonClientAttributes(hWnd, WTNCA_VALIDBITS, WTNCA_NODRAWCAPTION);

	return SUCCEEDED(hr);
}

BOOL CDwmHelper::DwmDefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult)
{
	if (!mb_DwmAllowed)
		return FALSE;
	return dwm._DwmDefWindowProc(hwnd, msg, wParam, lParam, plResult);
}

HRESULT CDwmHelper::DwmGetWindowAttribute(HWND hwnd, DWORD dwAttribute, PVOID pvAttribute, DWORD cbAttribute)
{
	if (!dwm._DwmGetWindowAttribute)
		return E_NOINTERFACE;
	return dwm._DwmGetWindowAttribute(hwnd, dwAttribute, pvAttribute, cbAttribute);
}

int CDwmHelper::GetDwmClientRectTopOffset()
{
	int nOffset = 0;
	DEBUGTEST(FrameDrawStyle dt = gpConEmu->DrawType());

	if (!gpSet->isTabs)
		goto wrap;

wrap:
	mn_DwmClientRectTopOffset = nOffset;
	return nOffset;
}

HANDLE/*HTHEME*/ CDwmHelper::OpenThemeData(HWND hwnd, LPCWSTR pszClassList)
{
	if (!ux._OpenThemeData)
		return nullptr;
	return ux._OpenThemeData(hwnd, pszClassList);
}

HRESULT CDwmHelper::CloseThemeData(HANDLE/*HTHEME*/ hTheme)
{
	if (!ux._CloseThemeData)
		return E_NOINTERFACE;
	return ux._CloseThemeData(hTheme);
}

HANDLE/*HPAINTBUFFER*/ CDwmHelper::BeginBufferedPaint(HDC hdcTarget, const RECT& rcTarget, PaintDC& dc)
//(HDC hdcTarget, const RECT *prcTarget, int/*BP_BUFFERFORMAT*/ dwFormat, void/*BP_PAINTPARAMS*/ *pPaintParams, HDC *phdc)
{
	HANDLE hResult = nullptr;

	if (ux._BeginBufferedPaint && !dc.bInternal)
	{
		dc.bInternal = false;

		BP_PAINTPARAMS args = {sizeof(args)};
		args.dwFlags = 1/*BPPF_ERASE*/;

		hResult = ux._BeginBufferedPaint(hdcTarget, &rcTarget, 1/*BPBF_DIB*//*dwFormat*/, &args, &dc.hDC);
	}

	if (!hResult)
	{
		BITMAPINFOHEADER bi = {sizeof(bi)};
		bi.biWidth = rcTarget.right;
		bi.biHeight = rcTarget.bottom;
		bi.biPlanes = 1;
		bi.biBitCount = 32;
		void* pPixels = nullptr;
		HDC hdcPaint = CreateCompatibleDC(hdcTarget);
		HBITMAP hbmp = CreateDIBSection(hdcPaint, (BITMAPINFO*)&bi, DIB_RGB_COLORS, &pPixels, nullptr, 0);
		if (hbmp)
		{
			HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcPaint, hbmp);

			dc.hDC = hdcPaint;
			dc.rcTarget = rcTarget;
			dc.hInternal1 = hdcTarget;
			dc.hInternal2 = hbmp;
			dc.hInternal3 = hOldBmp;
			dc.hInternal4 = pPixels;
			dc.bInternal = true;

			hResult = (HANDLE)TRUE;
		}
	}

	dc.hBuffered = hResult;
	return hResult;
}

HRESULT CDwmHelper::BufferedPaintSetAlpha(const PaintDC& dc, const RECT *prc, BYTE alpha)
{
	if (!ux._BufferedPaintSetAlpha || dc.bInternal)
		return E_NOINTERFACE;
	return ux._BufferedPaintSetAlpha(dc.hBuffered, prc, alpha);
}

HRESULT CDwmHelper::EndBufferedPaint(PaintDC& dc, BOOL fUpdateTarget)
//(HANDLE/*HPAINTBUFFER*/ hBufferedPaint, BOOL fUpdateTarget)
{
	HRESULT hr = E_NOINTERFACE;
	if (dc.bInternal)
	{
		if (fUpdateTarget)
		{
			RECT rc = dc.rcTarget;
			BitBlt(
				(HDC)dc.hInternal1, rc.left, rc.top, rc.right-rc.left+1, rc.bottom-rc.top+1,
				dc.hDC, rc.left, rc.top, SRCCOPY);
		}

		SelectObject(dc.hDC, (HBITMAP)dc.hInternal3);
		DeleteObject((HBITMAP)dc.hInternal2);
		DeleteDC(dc.hDC);

		hr = S_OK;
	}
	else
	{
		if (ux._EndBufferedPaint)
		{
			hr = ux._EndBufferedPaint(dc.hBuffered, fUpdateTarget);
		}
	}

	return hr;
}

HRESULT CDwmHelper::DrawThemeTextEx(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwFlags, LPRECT pRect, const void/*DTTOPTS*/ *pOptions)
{
	if (!ux._DrawThemeTextEx)
		return E_NOINTERFACE;
	return ux._DrawThemeTextEx(hTheme, hdc, iPartId, iStateId, pszText, iCharCount, dwFlags, pRect, (const DTTOPTS*)pOptions);
}

HRESULT CDwmHelper::DrawThemeBackground(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect)
{
	if (!ux._DrawThemeBackground)
		return E_NOINTERFACE;
	return ux._DrawThemeBackground(hTheme, hdc, iPartId, iStateId, pRect, pClipRect);
}

HRESULT CDwmHelper::DrawThemeEdge(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pDestRect, UINT uEdge, UINT uFlags, LPRECT pContentRect)
{
	if (!ux._DrawThemeEdge)
		return E_NOINTERFACE;
	return ux._DrawThemeEdge(hTheme, hdc, iPartId, iStateId, pDestRect, uEdge, uFlags, pContentRect);
}

HRESULT CDwmHelper::GetThemeMargins(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, LPRECT prc, RECT *pMargins)
{
	if (!ux._GetThemeMargins)
		return E_NOINTERFACE;
	return ux._GetThemeMargins(hTheme, hdc, iPartId, iStateId, iPropId, prc, pMargins);
}

HRESULT CDwmHelper::GetThemePartSize(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT prc, int/*THEMESIZE*/ eSize, SIZE *psz)
{
	if (!ux._GetThemePartSize)
		return E_NOINTERFACE;
	return ux._GetThemePartSize(hTheme, hdc, iPartId, iStateId, prc, eSize, psz);
}

HRESULT CDwmHelper::GetThemePosition(HANDLE/*HTHEME*/ hTheme, int iPartId, int iStateId, int iPropId, POINT *pPoint)
{
	if (!ux._GetThemePosition)
		return E_NOINTERFACE;
	return ux._GetThemePosition(hTheme, iPartId, iStateId, iPropId, pPoint);
}

int CDwmHelper::GetThemeSysSize(HANDLE/*HTHEME*/ hTheme, int iSizeID)
{
	if (!ux._GetThemeSysSize)
		return -1;
	return ux._GetThemeSysSize(hTheme, iSizeID);
}

HRESULT CDwmHelper::GetThemeBackgroundContentRect(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pBoundingRect, LPRECT pContentRect)
{
	if (!ux._GetThemeBackgroundContentRect)
		return E_NOINTERFACE;
	return ux._GetThemeBackgroundContentRect(hTheme, hdc, iPartId, iStateId, pBoundingRect, pContentRect);
}

void CDwmHelper::ForceSetIconic(HWND hWnd)
{
	BOOL bValue;

	if (dwm._DwmSetWindowAttribute)
	{
		bValue = TRUE;
		dwm._DwmSetWindowAttribute(hWnd, DWMWA_FORCE_ICONIC_REPRESENTATION, &bValue, sizeof(bValue));
	}

	if (dwm._DwmSetWindowAttribute)
	{
		bValue = TRUE;
		dwm._DwmSetWindowAttribute(hWnd, DWMWA_HAS_ICONIC_BITMAP, &bValue, sizeof(bValue));
	}
}

HRESULT CDwmHelper::DwmSetIconicThumbnail(HWND hwnd, HBITMAP hbmp)
{
	HRESULT hr = E_NOINTERFACE;
	if (dwm._DwmSetIconicThumbnail)
		hr = dwm._DwmSetIconicThumbnail(hwnd, hbmp, 0);
	return hr;
}

HRESULT CDwmHelper::DwmSetIconicLivePreviewBitmap(HWND hwnd, HBITMAP hbmp, POINT *pptClient)
{
	HRESULT hr = E_NOINTERFACE;
	if (dwm._DwmSetIconicLivePreviewBitmap)
		hr = dwm._DwmSetIconicLivePreviewBitmap(hwnd, hbmp, pptClient, 0);
	return hr;
}

HRESULT CDwmHelper::DwmInvalidateIconicBitmaps(HWND hwnd)
{
	HRESULT hr = E_NOINTERFACE;
	if (dwm._DwmInvalidateIconicBitmaps)
		hr = dwm._DwmInvalidateIconicBitmaps(hwnd);
	return hr;
}

BOOL CDwmHelper::AdjustWindowRectExForDpi(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi)
{
	BOOL rc = FALSE;
	if (user._AdjustWindowRectExForDpi)
		rc = user._AdjustWindowRectExForDpi(lpRect, dwStyle, bMenu, dwExStyle, dpi);
	else
		rc = AdjustWindowRectEx(lpRect, dwStyle, bMenu, dwExStyle);
	return rc;
}
