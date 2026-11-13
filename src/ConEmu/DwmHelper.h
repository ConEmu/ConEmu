
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

#pragma once

enum FrameDrawStyle
{
	fdt_Win2k = 1,
	fdt_Win2kSelf = 2,
	fdt_Themed = 3,
	fdt_Aero = 4,
	fdt_Win8 = 5,
};

struct PaintDC
{
	bool   bInternal;
	HDC    hDC;
	HANDLE hBuffered;

	// Internal use!
	RECT   rcTarget;
	void*  hInternal1; // HDC hdcTarget
	void*  hInternal2; // HBITMAP hbmp
	void*  hInternal3; // HBITMAP hOldBmp
	void*  hInternal4; // void* pPixels
};

enum IMMERSIVE_HC_CACHE_MODE
{
	IHCM_USE_CACHED_VALUE,
	IHCM_REFRESH
};

// 1903 18362
enum PreferredAppMode
{
	Default,
	AllowDark,
	ForceDark,
	ForceLight,
	Max
};

enum WINDOWCOMPOSITIONATTRIB
{
	WCA_UNDEFINED = 0,
	WCA_NCRENDERING_ENABLED = 1,
	WCA_NCRENDERING_POLICY = 2,
	WCA_TRANSITIONS_FORCEDISABLED = 3,
	WCA_ALLOW_NCPAINT = 4,
	WCA_CAPTION_BUTTON_BOUNDS = 5,
	WCA_NONCLIENT_RTL_LAYOUT = 6,
	WCA_FORCE_ICONIC_REPRESENTATION = 7,
	WCA_EXTENDED_FRAME_BOUNDS = 8,
	WCA_HAS_ICONIC_BITMAP = 9,
	WCA_THEME_ATTRIBUTES = 10,
	WCA_NCRENDERING_EXILED = 11,
	WCA_NCADORNMENTINFO = 12,
	WCA_EXCLUDED_FROM_LIVEPREVIEW = 13,
	WCA_VIDEO_OVERLAY_ACTIVE = 14,
	WCA_FORCE_ACTIVEWINDOW_APPEARANCE = 15,
	WCA_DISALLOW_PEEK = 16,
	WCA_CLOAK = 17,
	WCA_CLOAKED = 18,
	WCA_ACCENT_POLICY = 19,
	WCA_FREEZE_REPRESENTATION = 20,
	WCA_EVER_UNCLOAKED = 21,
	WCA_VISUAL_OWNER = 22,
	WCA_HOLOGRAPHIC = 23,
	WCA_EXCLUDED_FROM_DDA = 24,
	WCA_PASSIVEUPDATEMODE = 25,
	WCA_USEDARKMODECOLORS = 26,
	WCA_LAST = 27
};

struct WINDOWCOMPOSITIONATTRIBDATA
{
	WINDOWCOMPOSITIONATTRIB Attrib;
	PVOID pvData;
	SIZE_T cbData;
};

namespace global {
	extern bool g_darkModeSupported;
	extern bool g_darkModeEnabled;
	extern DWORD g_buildNumber;
}

using fnRtlGetNtVersionNumbers = void (WINAPI*)(LPDWORD major, LPDWORD minor, LPDWORD build);
using fnSetWindowCompositionAttribute = BOOL(WINAPI*)(HWND hWnd, WINDOWCOMPOSITIONATTRIBDATA*);
// 1809 17763
using fnShouldAppsUseDarkMode = bool (WINAPI*)(); // ordinal 132
using fnAllowDarkModeForWindow = bool (WINAPI*)(HWND hWnd, bool allow); // ordinal 133
using fnAllowDarkModeForApp = bool (WINAPI*)(bool allow); // ordinal 135, in 1809
using fnFlushMenuThemes = void (WINAPI*)(); // ordinal 136
using fnRefreshImmersiveColorPolicyState = void (WINAPI*)(); // ordinal 104
using fnIsDarkModeAllowedForWindow = bool (WINAPI*)(HWND hWnd); // ordinal 137
using fnGetIsImmersiveColorUsingHighContrast = bool (WINAPI*)(IMMERSIVE_HC_CACHE_MODE mode); // ordinal 106
using fnOpenNcThemeData = HANDLE/*HTHEME*/(WINAPI*)(HWND hWnd, LPCWSTR pszClassList); // ordinal 49
// 1903 18362
using fnShouldSystemUseDarkMode = bool (WINAPI*)(); // ordinal 138
using fnSetPreferredAppMode = PreferredAppMode(WINAPI*)(PreferredAppMode appMode); // ordinal 135, in 1903
using fnIsDarkModeAllowedForApp = bool (WINAPI*)(); // ordinal 139

extern fnSetWindowCompositionAttribute _SetWindowCompositionAttribute;
extern fnShouldAppsUseDarkMode _ShouldAppsUseDarkMode;
extern fnAllowDarkModeForWindow _AllowDarkModeForWindow;
extern fnAllowDarkModeForApp _AllowDarkModeForApp;
extern fnFlushMenuThemes _FlushMenuThemes;
extern fnRefreshImmersiveColorPolicyState _RefreshImmersiveColorPolicyState;
extern fnIsDarkModeAllowedForWindow _IsDarkModeAllowedForWindow;
extern fnGetIsImmersiveColorUsingHighContrast _GetIsImmersiveColorUsingHighContrast;
extern fnOpenNcThemeData _OpenNcThemeData;
// 1903 18362
extern fnShouldSystemUseDarkMode _ShouldSystemUseDarkMode;
extern fnSetPreferredAppMode _SetPreferredAppMode;

bool AllowDarkModeForWindow(HWND hWnd, bool allow);
bool IsHighContrast();
bool IsColorSchemeChangeMessage(LPARAM lParam);
bool IsColorSchemeChangeMessage(UINT message, LPARAM lParam);
void AllowDarkModeForApp(bool allow);
void FixDarkScrollBar();

class CDwmHelper
{
protected:
	bool    mb_DwmAllowed = false, mb_ThemeAllowed = false, mb_BufferedAllowed = false;
	bool    mb_EnableGlass = false, mb_EnableTheming = false;
	int     mn_DwmClientRectTopOffset = 0;
	FrameDrawStyle m_DrawType = fdt_Win2k;
	virtual void OnUseGlass(bool abEnableGlass) = 0;
	virtual void OnUseTheming(bool abEnableTheming) = 0;
	virtual void OnUseDwm(bool abEnableDwm) = 0;
public:
	CDwmHelper();
	virtual ~CDwmHelper();
public:
	bool IsDwm();
	bool IsDwmAllowed();
	bool IsGlass();
	bool IsThemed();
	static bool IsHighContrast();
	static bool IsWindowsDarkMode();
	static COLORREF WindowsAccentColor();
	static COLORREF BlendColors(COLORREF base, COLORREF tint, unsigned tintPercent);
	void ApplyPopupMenuTheme(HWND owner);
	void EnableDarkCaption(HWND window);
	void EnableGlass(bool abGlass);
	void EnableTheming(bool abTheme);
	void SetWindowTheme(HWND hWnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
	void RefreshTitleBarThemeColor(HWND hWnd);
	void EnableBlurBehind(bool abBlurBehindClient);
	bool ExtendWindowFrame();
	bool ExtendWindowFrame(HWND hWnd, const RECT& rcMargins);
	void CheckGlassAttribute();
	int GetDwmClientRectTopOffset();
	BOOL DwmDefWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
	HRESULT DwmGetWindowAttribute(HWND hwnd, DWORD dwAttribute, PVOID pvAttribute, DWORD cbAttribute);
	HANDLE/*HTHEME*/ OpenThemeData(HWND hwnd, LPCWSTR pszClassList);
	HRESULT CloseThemeData(HANDLE/*HTHEME*/ hTheme);
	HANDLE/*HPAINTBUFFER*/ BeginBufferedPaint(HDC hdcTarget, const RECT& rcTarget, PaintDC& dc);
	HRESULT BufferedPaintSetAlpha(const PaintDC& dc, const RECT *prc, BYTE alpha);
	HRESULT EndBufferedPaint(PaintDC& dc, BOOL fUpdateTarget);
	HRESULT DrawThemeTextEx(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwFlags, LPRECT pRect, const void/*DTTOPTS*/ *pOptions);
	HRESULT DrawThemeBackground(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect);
	HRESULT DrawThemeEdge(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pDestRect, UINT uEdge, UINT uFlags, LPRECT pContentRect);
	HRESULT GetThemeMargins(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, LPRECT prc, RECT *pMargins);
	HRESULT GetThemePartSize(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT prc, int/*THEMESIZE*/ eSize, SIZE *psz);
	HRESULT GetThemePosition(HANDLE/*HTHEME*/ hTheme, int iPartId, int iStateId, int iPropId, POINT *pPoint);
	int GetThemeSysSize(HANDLE/*HTHEME*/ hTheme, int iSizeID);
	HRESULT GetThemeBackgroundContentRect(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pBoundingRect, LPRECT pContentRect);
	// Determine and store current draw type
	FrameDrawStyle DrawType();
	// Aero support
	void ForceSetIconic(HWND hWnd);
	HRESULT DwmSetIconicThumbnail(HWND hwnd, HBITMAP hbmp);
	HRESULT DwmSetIconicLivePreviewBitmap(HWND hwnd, HBITMAP hbmp, POINT *pptClient);
	HRESULT DwmInvalidateIconicBitmaps(HWND hwnd);
	// Per-monitor DPI support
	BOOL AdjustWindowRectExForDpi(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
private:
	HMODULE mh_User32 = nullptr;
	HMODULE mh_DwmApi = nullptr;
	HMODULE mh_UxTheme = nullptr;
private:
	// Initialize dlls and functions
	void InitDwm();

	struct USER {
	// Win10
	typedef BOOL (WINAPI* AdjustWindowRectExForDpi_t)(LPRECT lpRect, DWORD dwStyle, BOOL bMenu, DWORD dwExStyle, UINT dpi);
	AdjustWindowRectExForDpi_t _AdjustWindowRectExForDpi = nullptr;
	// Vista+
	typedef BOOL (WINAPI* ChangeWindowMessageFilter_t)(UINT message, DWORD dwFlag);
	ChangeWindowMessageFilter_t _ChangeWindowMessageFilter = nullptr;
	} user;

	struct DWM {
	// Vista+ Aero
	typedef HRESULT (WINAPI* DwmIsCompositionEnabled_t)(BOOL *pfEnabled);
	DwmIsCompositionEnabled_t _DwmIsCompositionEnabled = nullptr;
	typedef HRESULT (WINAPI* DwmSetWindowAttribute_t)(HWND hwnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);
	DwmSetWindowAttribute_t _DwmSetWindowAttribute = nullptr;
	typedef HRESULT (WINAPI* DwmExtendFrameIntoClientArea_t)(HWND hWnd, void* pMarInset);
	DwmExtendFrameIntoClientArea_t _DwmExtendFrameIntoClientArea = nullptr;
	typedef BOOL (WINAPI* DwmDefWindowProc_t)(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *plResult);
	DwmDefWindowProc_t _DwmDefWindowProc = nullptr;
	typedef HRESULT (WINAPI* DwmGetWindowAttribute_t)(HWND hwnd, DWORD dwAttribute, PVOID pvAttribute, DWORD cbAttribute);
	DwmGetWindowAttribute_t _DwmGetWindowAttribute = nullptr;
	typedef HRESULT (WINAPI* DwmSetIconicThumbnail_t)(HWND hwnd, HBITMAP hbmp, DWORD dwSITFlags);
	DwmSetIconicThumbnail_t _DwmSetIconicThumbnail = nullptr;
	typedef HRESULT (WINAPI* DwmSetIconicLivePreviewBitmap_t)(HWND hwnd, HBITMAP hbmp, POINT *pptClient, DWORD dwSITFlags);
	DwmSetIconicLivePreviewBitmap_t _DwmSetIconicLivePreviewBitmap = nullptr;
	typedef HRESULT (WINAPI* DwmInvalidateIconicBitmaps_t)(HWND hwnd);
	DwmInvalidateIconicBitmaps_t _DwmInvalidateIconicBitmaps = nullptr;
	typedef HRESULT (WINAPI* DwmEnableBlurBehindWindow_t)(HWND hWnd, const void/*DWM_BLURBEHIND*/ *pBlurBehind);
	DwmEnableBlurBehindWindow_t _DwmEnableBlurBehindWindow = nullptr;
	} dwm;

	struct UX {
	// XP+ Theming
	typedef BOOL (WINAPI* AppThemed_t)();
	AppThemed_t _IsAppThemed = nullptr; // XP
	AppThemed_t _IsThemeActive = nullptr; // XP
	typedef HANDLE/*HTHEME*/ (WINAPI* OpenThemeData_t)(HWND hwnd, LPCWSTR pszClassList);
	OpenThemeData_t _OpenThemeData = nullptr; // XP
	typedef HRESULT (WINAPI* CloseThemeData_t)(HANDLE/*HTHEME*/ hTheme);
	CloseThemeData_t _CloseThemeData = nullptr; // XP
	typedef HRESULT (WINAPI* BufferedPaintInit_t)();
	BufferedPaintInit_t _BufferedPaintInit = nullptr; // Vista
	BufferedPaintInit_t _BufferedPaintUnInit = nullptr; // Vista
	typedef HANDLE/*HPAINTBUFFER*/ (WINAPI* BeginBufferedPaint_t)(HDC hdcTarget, const RECT *prcTarget, int/*BP_BUFFERFORMAT*/ dwFormat, void/*BP_PAINTPARAMS*/ *pPaintParams, HDC *phdc);
	BeginBufferedPaint_t _BeginBufferedPaint = nullptr; // Vista
	typedef HRESULT (WINAPI* BufferedPaintSetAlpha_t)(HANDLE/*HPAINTBUFFER*/ hBufferedPaint, const RECT *prc, BYTE alpha);
	BufferedPaintSetAlpha_t _BufferedPaintSetAlpha = nullptr; // Vista
	typedef HRESULT (WINAPI* EndBufferedPaint_t)(HANDLE/*HPAINTBUFFER*/ hBufferedPaint, BOOL fUpdateTarget);
	EndBufferedPaint_t _EndBufferedPaint = nullptr; // Vista
	typedef HRESULT (WINAPI* DrawThemeTextEx_t)(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwFlags, LPRECT pRect, const void/*DTTOPTS*/ *pOptions);
	DrawThemeTextEx_t _DrawThemeTextEx = nullptr; // Vista
	typedef HRESULT (WINAPI* DrawThemeBackground_t)(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect);
	DrawThemeBackground_t _DrawThemeBackground = nullptr; // XP
	typedef HRESULT (WINAPI* DrawThemeEdge_t)(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pDestRect, UINT uEdge, UINT uFlags, LPRECT pContentRect);
	DrawThemeEdge_t _DrawThemeEdge = nullptr; // XP
	typedef HRESULT (WINAPI* GetThemeMargins_t)(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, int iPropId, LPRECT prc, RECT *pMargins);
	GetThemeMargins_t _GetThemeMargins = nullptr; // XP
	typedef HRESULT (WINAPI* GetThemePartSize_t)(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT prc, int/*THEMESIZE*/ eSize, SIZE *psz);
	GetThemePartSize_t _GetThemePartSize = nullptr; // XP
	typedef HRESULT (WINAPI* GetThemePosition_t)(HANDLE/*HTHEME*/ hTheme, int iPartId, int iStateId, int iPropId, POINT *pPoint);
	GetThemePosition_t _GetThemePosition = nullptr; // XP
	typedef int (WINAPI* GetThemeSysSize_t)(HANDLE/*HTHEME*/ hTheme, int iSizeID);
	GetThemeSysSize_t _GetThemeSysSize = nullptr; // XP
	typedef HRESULT (WINAPI* GetThemeBackgroundContentRect_t)(HANDLE/*HTHEME*/ hTheme, HDC hdc, int iPartId, int iStateId, LPCRECT pBoundingRect, LPRECT pContentRect);
	GetThemeBackgroundContentRect_t _GetThemeBackgroundContentRect = nullptr; // XP
	typedef void (WINAPI* SetThemeAppProperties_t)(DWORD dwFlags);
	SetThemeAppProperties_t _SetThemeAppProperties = nullptr; // XP
	typedef void (WINAPI* SetWindowThemeNonClientAttributes_t)(DWORD dwFlags);
	SetWindowThemeNonClientAttributes_t _SetWindowThemeNonClientAttributes = nullptr; // Vista
	typedef HRESULT (WINAPI* SetWindowTheme_t)(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList);
	SetWindowTheme_t _SetWindowTheme = nullptr; // Vista
	} ux;
};
