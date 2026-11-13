
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

// Dark-mode integration adapted from ysc3839/win32-darkmode (via HO-COOH).
// See ../../Release/ConEmu/DarkMode-LICENSE.txt for the MIT license notices.

#define HIDE_USE_EXCEPTION_INFO
#include "Header.h"
#include "DwmApi_Part.h"
#include "DarkMode.h"
#include "ConEmu.h"
#include "Options.h"
#include "IatHook.h"




/* *************************** */

namespace global {
	bool g_darkModeSupported = false;
	bool g_darkModeEnabled = false;
	DWORD g_buildNumber = 0;
}

fnSetWindowCompositionAttribute _SetWindowCompositionAttribute = nullptr;
fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode = nullptr;
fnAllowDarkModeForWindow _AllowDarkModeForWindow = nullptr;
fnAllowDarkModeForApp _AllowDarkModeForApp = nullptr;
fnFlushMenuThemes _FlushMenuThemes = nullptr;
fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState = nullptr;
fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow = nullptr;
fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast = nullptr;
fnOpenNcThemeData _OpenNcThemeData = nullptr;
fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode = nullptr;
fnSetPreferredAppMode _SetPreferredAppMode = nullptr;

bool AllowDarkModeForWindow(HWND hWnd, bool allow)
{
	if (global::g_darkModeSupported)
		return _AllowDarkModeForWindow(hWnd, allow);
	return false;
}

bool IsHighContrast()
{
	return CDwmHelper::IsHighContrast();
}

void CDwmHelper::RefreshTitleBarThemeColor(HWND hWnd)
{
	if (!hWnd || !global::g_darkModeSupported)
		return;

	global::g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();
	BOOL dark = global::g_darkModeEnabled;
	_AllowDarkModeForWindow(hWnd, dark != FALSE);

	// Prefer the DWM attribute on current Windows. Keep the older Windows 10
	// paths, and resolve DWM dynamically as with the other optional APIs.
	const DWORD useImmersiveDarkMode = 20;
	const HRESULT hr = dwm._DwmSetWindowAttribute
		? dwm._DwmSetWindowAttribute(hWnd, useImmersiveDarkMode, &dark, sizeof(dark))
		: E_NOTIMPL;
	if (FAILED(hr) && global::g_buildNumber < 18362)
		SetPropW(hWnd, L"UseImmersiveDarkModeColors", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(dark)));
	else if (FAILED(hr) && _SetWindowCompositionAttribute)
	{
		WINDOWCOMPOSITIONATTRIBDATA data = { WCA_USEDARKMODECOLORS, &dark, sizeof(dark) };
		_SetWindowCompositionAttribute(hWnd, &data);
	}

	RedrawWindow(hWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_FRAME);
	if (hWnd == ghWnd)
	{
		EnumChildWindows(hWnd, [](HWND child, LPARAM) -> BOOL
		{
			SendMessage(child, WM_THEMECHANGED, 0, 0);
			RedrawWindow(child, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_FRAME);
			return TRUE;
		}, 0);
	}
}

bool CDwmHelper::UpdateConsoleCaptionColors(HWND window)
{
	if (!window || !dwm._DwmSetWindowAttribute || global::g_buildNumber < 22000)
		return false;

	COLORREF background = DefaultCaptionColor;
	COLORREF foreground = DefaultCaptionColor;
	DarkMode::GetConsoleChromeColors(background, foreground);
	if (background == captionBackground_ && foreground == captionForeground_)
		return false;

	// DWMWA_CAPTION_COLOR and DWMWA_TEXT_COLOR are Windows 11 attributes.
	constexpr DWORD captionColor = 35;
	constexpr DWORD textColor = 36;
	if (FAILED(dwm._DwmSetWindowAttribute(window, captionColor, &background, sizeof(background))))
		return false;

	dwm._DwmSetWindowAttribute(window, textColor, &foreground, sizeof(foreground));
	captionBackground_ = background;
	captionForeground_ = foreground;
	return true;
}

bool IsColorSchemeChangeMessage(LPARAM lParam)
{
	bool is = false;
	if (!global::g_darkModeSupported)
		return false;
	if (lParam && lstrcmpiW(reinterpret_cast<LPCWSTR>(lParam), L"ImmersiveColorSet") == 0)
	{
		_RefreshImmersiveColorPolicyState();
		is = true;
	}
	if (_GetIsImmersiveColorUsingHighContrast)
		_GetIsImmersiveColorUsingHighContrast(IHCM_REFRESH);
	return is;
}

bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam)
{
	if (message == WM_SETTINGCHANGE)
		return IsColorSchemeChangeMessage(lParam);
	return false;
}

void AllowDarkModeForApp(bool allow)
{
	if (_AllowDarkModeForApp)
		_AllowDarkModeForApp(allow);
	else if (_SetPreferredAppMode)
		_SetPreferredAppMode(allow ? AllowDark : Default);
}

static HANDLE WINAPI MyOpenThemeData(HWND hWnd, LPCWSTR classList)
{
	if (classList && wcscmp(classList, L"ScrollBar") == 0
		&& global::g_darkModeSupported && _ShouldAppsUseDarkMode()
		&& !IsHighContrast())
	{
		hWnd = nullptr;
		classList = L"Explorer::ScrollBar";
	}
	return _OpenNcThemeData(hWnd, classList);
}

void FixDarkScrollBar()
{
	// Ensure _OpenNcThemeData is initialized before attempting to hook
	if (!global::g_darkModeSupported || !_OpenNcThemeData)
		return;

	HMODULE hComctl = LoadLibraryExW(L"comctl32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
	if (hComctl)
	{
		auto addr = FindDelayLoadThunkInModule(hComctl, "uxtheme.dll", static_cast<uint16_t>(49)); // OpenNcThemeData
		if (addr)
		{
			DWORD oldProtect;
			if (VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), PAGE_READWRITE, &oldProtect))
			{
				addr->u1.Function = reinterpret_cast<ULONG_PTR>(static_cast<fnOpenNcThemeData>(MyOpenThemeData));
				VirtualProtect(addr, sizeof(IMAGE_THUNK_DATA), oldProtect, &oldProtect);
			}
		}
		FreeLibrary(hComctl);
	}
}



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
		mh_UxTheme = LoadLibraryExW(_T("UxTheme.dll"), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
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
			// Win10+


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

	auto RtlGetNtVersionNumbers = reinterpret_cast<fnRtlGetNtVersionNumbers>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetNtVersionNumbers"));
	if (RtlGetNtVersionNumbers)
	{
		DWORD major, minor;
		RtlGetNtVersionNumbers(&major, &minor, &global::g_buildNumber);
		global::g_buildNumber &= ~0xF0000000;
		// These ordinals have different meanings on older Windows 10 builds.
		if (major == 10 && minor == 0 && global::g_buildNumber >= 17763)
		{
			HMODULE hUxtheme = mh_UxTheme;
			if (hUxtheme)
			{
				_OpenNcThemeData = reinterpret_cast<fnOpenNcThemeData>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(49)));
				_RefreshImmersiveColorPolicyState = reinterpret_cast<fnRefreshImmersiveColorPolicyState>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
				_GetIsImmersiveColorUsingHighContrast = reinterpret_cast<fnGetIsImmersiveColorUsingHighContrast>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(106)));
				_ShouldAppsUseDarkMode = reinterpret_cast<fnShouldAppsUseDarkMode>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
				_AllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));

				auto ord135 = GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
				if (global::g_buildNumber < 18362)
					_AllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(ord135);
				else
					_SetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(ord135);

				_FlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
				_IsDarkModeAllowedForWindow = reinterpret_cast<fnIsDarkModeAllowedForWindow>(GetProcAddress(hUxtheme, MAKEINTRESOURCEA(137)));

				_SetWindowCompositionAttribute = reinterpret_cast<fnSetWindowCompositionAttribute>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));

				if (_RefreshImmersiveColorPolicyState &&
					_ShouldAppsUseDarkMode &&
					_AllowDarkModeForWindow &&
					(_AllowDarkModeForApp || _SetPreferredAppMode) &&
					_FlushMenuThemes &&
					_IsDarkModeAllowedForWindow)
				{
					global::g_darkModeSupported = true;

					AllowDarkModeForApp(true);
					_RefreshImmersiveColorPolicyState();

					global::g_darkModeEnabled = _ShouldAppsUseDarkMode() && !IsHighContrast();

					FixDarkScrollBar();
				}
			}
		}
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
	if (global::g_darkModeSupported)
		return _ShouldAppsUseDarkMode() && !IsHighContrast();

	DWORD light = 1, size = sizeof(light);
	return !IsHighContrast()
		&& RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
			L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size) == ERROR_SUCCESS
		&& light == 0;
}

void CDwmHelper::EnableDarkCaption(HWND window)
{
	RefreshTitleBarThemeColor(window);
}

void CDwmHelper::ApplyPopupMenuTheme(HWND owner)
{
	if (!global::g_darkModeSupported)
		return;

	const bool dark = _ShouldAppsUseDarkMode() && !IsHighContrast();
	if (owner)
		AllowDarkModeForWindow(owner, dark);

	static int lastMode = -1;
	if (lastMode != static_cast<int>(dark))
	{
		lastMode = static_cast<int>(dark);
		_FlushMenuThemes();
	}
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
