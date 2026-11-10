
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

#pragma once

#undef SHOW_AUTOSCROLL

#define NEWMOUSESTYLE
#define NEWRUNSTYLE
//#undef NEWRUNSTYLE

#define APPDISTINCTBACKGROUND

#include "../common/defines.h"

#include <shlwapi.h>
//#include <vector>
//#if !defined(__GNUC__)
//#include <crtdbg.h>
//#endif

#undef SetCurrentDirectory
#define SetCurrentDirectory "gpConEmu->ChangeWorkDir() must be used!"

#include "../common/Memory.h"
#include "../common/MAssert.h"
#include "../common/MStrDup.h"
#include "../common/CEStr.h"
#include "../common/Keyboard.h"

#if defined(__GNUC__) && !defined(__MINGW64_VERSION_MAJOR)
#define wmemmove_s(d,ds,s,ss) wmemmove(d,s,ss)
#define SecureZeroMemory(p,s) memset(p,0,s)
#endif

#ifndef TimeGetTime
	#ifdef __GNUC__
		#define TimeGetTime GetTickCount
	#else
		#define TimeGetTime timeGetTime
	#endif
#endif


#include "globals.h"
#include "resource.h"


#define DRAG_L_ALLOWED        0x0001
#define DRAG_R_ALLOWED        0x0002
#define DRAG_L_STARTED        0x0010
#define DRAG_R_STARTED        0x0020
#define MOUSE_R_LOCKED        0x0040
#define MOUSE_SIZING_BEGIN    0x0080
#define MOUSE_SIZING_TODO     0x0100
#define MOUSE_SIZING_DBLCKL   0x0200
#define MOUSE_DRAGPANEL_SPLIT 0x0400
#define MOUSE_DRAGPANEL_LEFT  0x0800
#define MOUSE_DRAGPANEL_RIGHT 0x1000
#define MOUSE_DRAGPANEL_BOTH  (MOUSE_DRAGPANEL_LEFT|MOUSE_DRAGPANEL_RIGHT)
#define MOUSE_WINDOW_DRAG     0x4000

#define MOUSE_DRAGPANEL_ALL (MOUSE_DRAGPANEL_SPLIT|MOUSE_DRAGPANEL_LEFT|MOUSE_DRAGPANEL_RIGHT|MOUSE_DRAGPANEL_BOTH)

#define MAX_TITLE_SIZE 0x400

#define CON_RECREATE_TIMEOUT 60000
#define CON_REDRAW_TIMOUT 5

#ifndef CLEARTYPE_NATURAL_QUALITY
#define CLEARTYPE_QUALITY       5
#define CLEARTYPE_NATURAL_QUALITY       6
#endif

#ifndef EVENT_CONSOLE_START_APPLICATION
#define EVENT_CONSOLE_START_APPLICATION 0x4006
#define EVENT_CONSOLE_END_APPLICATION   0x4007
#endif

#if !defined(CONSOLE_APPLICATION_16BIT)
#define CONSOLE_APPLICATION_16BIT       0x0001
#endif


#define TIMER_MAIN_ID 0
#define TIMER_MAIN_ELAPSE 500
#define TIMER_CONREDRAW_ID 1
#define TIMER_CAPTION_APPEAR_ID 3
#define TIMER_CAPTION_DISAPPEAR_ID 4
#define TIMER_RCLICKPAINT 5
#define TIMER_RCLICKPAINT_ELAPSE 20
#define TIMER_ADMSHIELD_ID 7
#define TIMER_ADMSHIELD_ELAPSE 1000
#define TIMER_QUAKE_AUTOHIDE_ID 9
#define TIMER_QUAKE_AUTOHIDE_ELAPSE 100
#define QUAKE_FOCUS_CHECK_TIMER_DELAY 500
#define TIMER_FAILED_TABBAR_ID 10 // FAILED_TABBAR_TIMERID
#define TIMER_FAILED_TABBAR_ELAPSE 3000 // FAILED_TABBAR_TIMEOUT
#define TIMER_ACTIVATESPLIT_ID 11
#define TIMER_ACTIVATESPLIT_ELAPSE 500
#define TIMER_SELECTION_ID 12
#define TIMER_SELECTION_ELAPSE 20

#define ACTIVATE_TAB_CRITICAL  1000
#define POST_UPDATE_TIMEOUT   2000

// Undocumented messages
#include "../common/ConsoleMessages.h"

#define MAX_CMD_HISTORY 100
#define MAX_CMD_HISTORY_SHOW 16
#define MAX_CMD_GROUP_SHOW 10

#define MAX_RENAME_TAB_LEN 128

#define MBox(rt) MsgBox(rt, /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION, Title)
#define MBoxA(rt) MsgBox(rt, /*MB_SYSTEMMODAL |*/ MB_ICONINFORMATION, nullptr)
#define MBoxError(rt) MsgBox(rt, /*MB_SYSTEMMODAL |*/ MB_ICONSTOP, nullptr)
#define MBoxAssert(V) _ASSERTE(V)
#define AssertThis() if (this == nullptr) { _ASSERT(this!=nullptr); return; }
#define AssertThisRet(dummy) if (this == nullptr) { _ASSERT(this!=nullptr); return (dummy); }

#define isWheelEvent(messg) (((messg) == WM_MOUSEWHEEL) || ((messg) == WM_MOUSEHWHEEL/*0x020E*/))

#ifdef MSGLOGGER
BOOL POSTMESSAGE(HWND h,UINT m,WPARAM w,LPARAM l,BOOL extra);
LRESULT SENDMESSAGE(HWND h,UINT m,WPARAM w,LPARAM l);
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) {MCHKHEAP; DebugLogPos(hw,x,y,w,h,"SetWindowPos"); SetWindowPos(hw,hp,x,y,w,h,f);}
#define MOVEWINDOW(hw,x,y,w,h,r) {MCHKHEAP; DebugLogPos(hw,x,y,w,h,"MoveWindow"); MoveWindow(hw,x,y,w,h,r);}
#define SETCONSOLESCREENBUFFERSIZERET(h,s,r) {MCHKHEAP; DebugLogBufSize(h,s); r=SetConsoleScreenBufferSize(h,s);}
#define SETCONSOLESCREENBUFFERSIZE(h,s) {BOOL lb; SETCONSOLESCREENBUFFERSIZERET(h,s,lb);}
#define DEBUGLOGFILE(m) DebugLogFile(m)
void DebugLogFile(LPCSTR asMessage);
void DebugLogBufSize(HANDLE h, COORD sz);
void DebugLogPos(HWND hw, int x, int y, int w, int h, LPCSTR asFunc);
void DebugLogMessage(HWND h, UINT m, WPARAM w, LPARAM l, int posted, BOOL extra);
#else
#define POSTMESSAGE(h,m,w,l,e) PostMessage(h,m,w,l)
#define SENDMESSAGE(h,m,w,l) SendMessage(h,m,w,l)
#define SETWINDOWPOS(hw,hp,x,y,w,h,f) SetWindowPos(hw,hp,x,y,w,h,f)
#define MOVEWINDOW(hw,x,y,w,h,r) MoveWindow(hw,x,y,w,h,r)
#define SETCONSOLESCREENBUFFERSIZERET(h,s,r) r=SetConsoleScreenBufferSize(h,s)
#define SETCONSOLESCREENBUFFERSIZE(h,s) SetConsoleScreenBufferSize(h,s)
#define DEBUGLOGFILE(m)
#define DebugLogMessage(h,m,w,l,posted,extra)
#endif
LPCWSTR GetMouseMsgName(UINT msg);

bool LogString(LPCWSTR asInfo, bool abWriteTime = true, bool abWriteLine = true);

//#if !defined(__GNUC__)
//#pragma warning (disable : 4005)
//#if !defined(_WIN32_WINNT)
//#define _WIN32_WINNT 0x0502
//#endif
//#endif

#include "helper.h"



//------------------------------------------------------------------------
///| Code optimizing |////////////////////////////////////////////////////
//------------------------------------------------------------------------

//#if !defined(__GNUC__)
//#include <intrin.h>
//#pragma function(memset, memcpy)
//__forceinline void *memset(void *_Dst, int _Val, size_t _Size)
//{
//	__stosb((unsigned char*)_Dst, _Val, _Size);
//	return _Dst;
//}
//__forceinline void *memcpy(void *_Dst, const void *_Src, size_t _Size)
//{
//	__movsb((unsigned char*)_Dst, (unsigned const char*)_Src, _Size);
//	return _Dst;
//}
//#endif

void WarnCreateWindowFail(LPCWSTR pszDescription, HWND hParent, DWORD nErrCode);
RECT CenterInParent(RECT rcDlg, HWND hParent);
BOOL MoveWindowRect(HWND hWnd, const RECT& rcWnd, BOOL bRepaint = FALSE);
HICON CreateNullIcon();

void ShutdownGuiStep(LPCWSTR asInfo, int nParm1 = 0, int nParm2 = 0, int nParm3 = 0, int nParm4 = 0);
void LogFocusInfo(LPCWSTR asInfo, int Level=1);

bool PtMouseDblClickTest(const MSG& msg1, const MSG msg2);

bool IntFromString(int& rnValue, LPCWSTR asValue, int anBase = 10, LPCWSTR* rsEnd = nullptr);
bool GetDlgItemSigned(HWND hDlg, WORD nID, int& nValue, int nMin = 0, int nMax = 0);
bool GetDlgItemUnsigned(HWND hDlg, WORD nID, DWORD& nValue, DWORD nMin = 0, DWORD nMax = 0);
CEStr GetDlgItemTextPtr(HWND hDlg, WORD nID);
size_t MyGetDlgItemText(HWND hDlg, WORD nID, CEStr& rsText);
bool GetColorRef(LPCWSTR pszText, COLORREF* pCR);

//#pragma warning(disable: 4311) // 'type cast' : pointer truncation from 'HBRUSH' to 'BOOL'

CEStr getFocusedExplorerWindowPath();
enum CESelectFileFlags
{
	sff_Default      = 0,
	sff_AutoQuote    = 1,
	sff_Cygwin       = 2,
	sff_SaveNewFile  = 4,
};
CEStr SelectFolder(LPCWSTR asTitle, LPCWSTR asDefFolder = nullptr, HWND hParent = ghWnd, DWORD/*CESelectFileFlags*/ nFlags = sff_AutoQuote, CRealConsole* apRCon = nullptr);
CEStr SelectFile(LPCWSTR asTitle, LPCWSTR asDefFile = nullptr, LPCWSTR asDefPath = nullptr, HWND hParent = ghWnd, LPCWSTR asFilter = nullptr, DWORD/*CESelectFileFlags*/ nFlags = sff_AutoQuote, CRealConsole* apRCon = nullptr);

#include "../common/RConStartArgsEx.h"


void RaiseTestException();

#define APP_MODEL_ID_PREFIX L"Maximus5.ConEmu."

#include "SettingsStorage.h"

#include "../common/MRect.h"
#include "../common/UnicodeChars.h"
#include "../common/WObjects.h"
#include "../common/CmdLine.h"

#define IsWindowsXP ((gOSVer.dwMajorVersion >= 6) || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion > 0))
#define IsWindowsVista (gOSVer.dwMajorVersion >= 6)
#define IsWindows7 ((gOSVer.dwMajorVersion > 6) || (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion > 0))
#define IsWindows8 ((gOSVer.dwMajorVersion > 6) || (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion > 1))
#define IsWindows8_1 ((gOSVer.dwMajorVersion > 6) || (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion > 2))

// GNU C HEADER PATCH
#ifdef __GNUC__
typedef BOOL (WINAPI* AlphaBlend_t)(HDC hdcDest, int xoriginDest, int yoriginDest, int wDest, int hDest, HDC hdcSrc, int xoriginSrc, int yoriginSrc, int wSrc, int hSrc, BLENDFUNCTION ftn);
typedef BOOL (WINAPI* SetLayeredWindowAttributes_t)(HWND hwnd, COLORREF crKey, BYTE bAlpha, DWORD dwFlags);
typedef BOOL (WINAPI* CreateRestrictedToken_t)(HANDLE ExistingTokenHandle, DWORD Flags, DWORD DisableSidCount, PSID_AND_ATTRIBUTES SidsToDisable, DWORD DeletePrivilegeCount, PLUID_AND_ATTRIBUTES PrivilegesToDelete, DWORD RestrictedSidCount, PSID_AND_ATTRIBUTES SidsToRestrict, PHANDLE NewTokenHandle);
#endif
// GetLayeredWindowAttributes появилась только в XP
typedef BOOL (WINAPI* GetLayeredWindowAttributes_t)(HWND hwnd, COLORREF *pcrKey, BYTE *pbAlpha, DWORD *pdwFlags);

#if !defined(MAPVK_VSC_TO_VK_EX)
	#define MAPVK_VK_TO_VSC     (0)
	#define MAPVK_VSC_TO_VK     (1)
	#define MAPVK_VK_TO_CHAR    (2)
	#define MAPVK_VSC_TO_VK_EX  (3)
#endif

#if !defined(WM_XBUTTONDBLCLK)
	#define WM_XBUTTONDOWN       0x020B
	#define WM_XBUTTONUP         0x020C
	#define WM_XBUTTONDBLCLK     0x020D
#endif

#if !defined(SPI_GETCLEARTYPE)
	#define SPI_GETCLEARTYPE     0x1048
#endif

#ifndef SEE_MASK_NOASYNC
	#define SEE_MASK_NOASYNC    0x00000100   // block on the call until the invoke has completed, use for callers that exit after calling ShellExecuteEx()
#endif

#ifndef TTM_SETTITLEW
	#define TTM_SETTITLEW           (WM_USER + 33)  // wParam = TTI_*, lParam = wchar* szTitle
#endif
#ifndef TTM_SETTITLE
	#define TTM_SETTITLE            TTM_SETTITLEW
#endif
#ifndef TTI_WARNING
	#define TTI_WARNING             2
#endif

#ifndef INPUTLANGCHANGE_SYSCHARSET
#define INPUTLANGCHANGE_SYSCHARSET 0x0001
#endif

#ifndef DISABLE_MAX_PRIVILEGE
#define DISABLE_MAX_PRIVILEGE   0x1
#endif


enum ConEmuMargins
{
	// Разница между размером всего окна и клиентской области окна (рамка + заголовок)
	CEM_FRAMEONLY = 0x0001,
	CEM_CAPTION = 0x0002,
	CEM_FRAMECAPTION = (CEM_FRAMEONLY|CEM_CAPTION),
	#if 0
	CEM_CLIENTSHIFT = 0x0004, // Если клиентская часть "расширена" на рамку
	#endif
	// Высота таба (пока только .top)
	CEM_TAB = 0x0010,
	CEM_TABACTIVATE = 0x1010,   // Принудительно считать, что таб есть (при включении таба)
	CEM_TABDEACTIVATE = 0x2010, // Принудительно считать, что таба нет (при отключении таба)
	CEM_TAB_MASK = (CEM_TAB|CEM_TABACTIVATE|CEM_TABDEACTIVATE),
	CEM_SCROLL = 0x0020, // Если полоса прокрутки всегда (!!!) видна - то ее ширина/высота
	CEM_STATUS = 0x0040, // Высота строки статуса
	CEM_PAD = 0x0080, // Ширина "отступа" от краев
	CEM_WIN10FRAME = 0x0100, // Specially created by MS frame margins in Win10
	CEM_VISIBLE_FRAME = 0x0200, // Takes into account HideCaptionAlwaysFrame() and CEM_WIN10FRAME
	CEM_INVISIBLE_FRAME = 0x400, // Takes into account HideCaptionAlwaysFrame() and CEM_WIN10FRAME
	// Маска для получения всех отступов
	CEM_ALL_MARGINS = CEM_FRAMECAPTION|CEM_TAB|/*CEM_SCROLL|*/CEM_STATUS/*|CEM_PAD*/,
	CEM_CLIENT_MARGINS = CEM_TAB|/*CEM_SCROLL|*/CEM_STATUS/*|CEM_PAD*/,
};

enum ConEmuRect
{
	CER_MAIN = 0,   // Полный размер окна
	// Далее все координаты считаются относительно клиентской области {0,0}
	CER_MAINCLIENT, // клиентская область главного окна (БЕЗ отрезания табов, прокруток, DoubleView и прочего. Целиком)
	CER_TAB,        // положение контрола с закладками (всего)
	CER_WORKSPACE,  // рабочая область ConEmu. В ней располагаются все видимые VCon/GUI apps. (БОЛЬШЕ чем CER_BACK при SplitScreen/DoubleView).
	CER_BACK,       // область, отведенная под VCon. Тут нужна вся область, без отрезания прокруток и округлений размеров под знакоместо
	CER_SCROLL,     // положение полосы прокрутки
	CER_DC,         // положение окна отрисовки
	CER_CONSOLE_ALL,// !!! _ размер в символах _ !!! размер всего поля (всех видимых сплитов)
	CER_CONSOLE_CUR,// !!! _ размер в символах _ !!! размер активной консоли (активный сплит)
	CER_CONSOLE_NTVDMOFF, // same as CER_CONSOLE, но во время отключения режима 16бит
	CER_FULLSCREEN, // полный размер в pix текущего монитора (содержащего ghWnd)
	CER_MAXIMIZED,  // размер максимизированного окна на текущем мониторе (содержащего ghWnd)
	CER_RESTORE,    // размер "восстановленного" окна после максимизации (коррекция по размеру монитора?)
	CER_MONITOR,    // полный размер в pix рабочей области текущего монитора (содержащего ghWnd)
//	CER_CORRECTED   // скорректированное положение (чтобы окно было видно на текущем мониторе)
};

typedef DWORD ConEmuBorders;
const ConEmuBorders
	CEB_TOP = 1,
	CEB_LEFT = 2,
	CEB_BOTTOM = 4,
	CEB_RIGHT = 8,
	CEB_ALL = CEB_TOP|CEB_LEFT|CEB_BOTTOM|CEB_RIGHT,
	// Next means "place window OnScreen when it out of screen totally"
	CEB_ALLOW_PARTIAL = 16,
	// None of above
	CEB_NONE = 0;

enum class DragPanelBorder : int
{
	None,
	Split,    // Change width of left/right panels
	Left,     // Left panel height
	Right,    // Right panel height
	Both,     // Change height of both panels
};

enum TrackMenuPlace
{
	tmp_None = 0,
	tmp_System,
	tmp_VCon,
	tmp_Cmd, // Tasks & Last commands
	tmp_CmdPopup, // Task contents (RClick, when menu was created without cmd-submenus)
	tmp_KeyBar,
	tmp_TabsList,
	tmp_PasteCmdLine,
	tmp_StatusBarCols,
	tmp_ChildSysMenu,
	tmp_SearchPopup,
};

enum ConEmuWindowMode
{
	wmCurrent = 0,
	wmNotChanging = -1,
	wmNormal = rNormal,
	wmMaximized = rMaximized,
	wmFullScreen = rFullScreen,
};

LPCWSTR GetWindowModeName(ConEmuWindowMode wm);

enum ExpandTextRangeType
{
	// Used for DblClick word selection, for example
	etr_Word  = 0x0001,
	// Internet/intranet URL's
	etr_Url   = 0x0002,
	// Highlight and goto compiler errors
	etr_File  = 0x0004,
	etr_Row   = 0x1000,
	etr_Col   = 0x2000,
	etr_FileRow = (etr_File|etr_Row),
	etr_FileRowCol = (etr_File|etr_Row|etr_Col),
	// IP v4 or v6
	etr_IP    = 0x0008,
	// PID
	etr_PID   = 0x0010,
	// Find somth suitable for clicking
	etr_AnyClickable = (etr_Url|etr_File),
	// Nothing was found
	etr_None = 0
};

// Подсветка URL's и строк-ошибок-компиляторов
struct ConEmuTextRange
{
	COORD mcr_FileLineStart, mcr_FileLineEnd;
	ExpandTextRangeType etrLast;
};

enum BackgroundOp
{
	eUpLeft = 0,
	eStretch = 1,
	eTile = 2,
	eUpRight = 3,
	eDownLeft = 4,
	eDownRight = 5,
	eFit = 6, // Stretch aspect ratio (Fit)
	eFill = 7, // Stretch aspect ratio (Fill)
	eCenter = 8,
	//
	eOpLast = eCenter,
};

enum AnsiExecutionPerm
{
	ansi_Allowed  = 0,
	ansi_CmdOnly  = 1, // Default
	ansi_Disabled = 2,
};

enum ToolbarMainBitmapIdx
{
	BID_FIST_CON = 0,
	BID_LAST_CON = (MAX_CONSOLE_COUNT-1),
	BID_NEWCON_IDX,
	BID_ALTERNATIVE_IDX,
	BID_MINIMIZE_IDX,
	BID_MAXIMIZE_IDX,
	BID_RESTORE_IDX,
	BID_APPCLOSE_IDX,
	BID_SYSMENU_IDX,
	BID_DUMMYBTN_IDX,
	BID_TOOLBAR_LAST_IDX,
};

enum ToolbarCommandIdx
{
	TID_ACTIVE_NUMBER = 1,
	TID_CREATE_CON,
	TID_ALTERNATIVE,
	TID_SCROLL,
	TID_MINIMIZE,
	TID_MAXIMIZE,
	TID_APPCLOSE,
	//TID_COPYING,
	TID_SYSMENU,
	TID_MINIMIZE_SEP = 110,
};

// Change focus commands, used from hotkeys and GuiMacro
enum class SwitchGuiFocusOp : int
{
	// Set focus into ConEmu window/process
	FocusConEmu = 0,
	// Switch focus between ConEmu and ChildGui
	FocusSwitch = 1,
	// Set focus into ChildGui window of the active console
	FocusChild = 2,
	// Set focus into the parent of the ConEmu window (ConEmu Inside mode)
	FocusParent = 3,
	// Max possible ID to validate input args
	Last
};

enum CEStatusFlags
{
	csf_VertDelim           = 0x00000001,
	csf_HorzDelim           = 0x00000002,
	csf_SystemColors        = 0x00000004,
	csf_NoVerticalPad       = 0x00000008,
};

enum CECopyMode
{
	cm_CopySel = 0, // Copy current selection (old bCopyAll==false)
	cm_CopyAll = 1, // Copy full buffer (old bCopyAll==true)
	cm_CopyVis = 2, // Copy visible screen area
	cm_CopyInt = 3, // Copy current selection into internal CEStr
};

#define CTSFormatDefault 0xFF /* use gpSet->isCTSHtmlFormat */

enum CEPasteMode
{
	pm_Standard  = 0, // Paste with possible "Return" keypresses
	pm_FirstLine = 1, // Paste only first line from the clipboard
	pm_OneLine   = 2, // Paste all lines from the clipboard, but delimit them with SPACES (cmd-line safe!)
};


// this is NOT a bitmask field!
// only exact values are allowed!
enum EnumVConFlags
{
	evf_None     = 0,
	evf_Active   = 1,
	evf_Visible  = 2,
	evf_GroupSet = 3, // Consoles with flag vf_GroupSet
	evf_All      = 4,
};

enum GroupInputCmd
{
	gic_Switch  = 0,
	gic_Enable  = 1,
	gic_Disable = 2,
};


bool CheckLockFrequentExecute(DWORD& Tick, DWORD Interval);
#define LockFrequentExecute(Interval,LastExecuteTick) if (CheckLockFrequentExecute(LastExecuteTick,Interval))
#define LockFrequentExecuteStatic(Interval) static DWORD LastExecuteTick; if (CheckLockFrequentExecute(LastExecuteTick,Interval))

extern const wchar_t* gsHomePage;    // = L"https://conemu.github.io";
extern const wchar_t* gsDownlPage;   // = L"https://conemu.github.io/en/Downloads.html";
extern const wchar_t* gsFirstStart;  // = L"https://conemu.github.io/en/SettingsFast.html";
extern const wchar_t* gsReportBug;   // = L"https://conemu.github.io/en/Issues.html";
extern const wchar_t* gsReportCrash; // = L"https://conemu.github.io/en/Issues.html";
extern const wchar_t* gsWhatsNew;    // = L"https://conemu.github.io/en/Whats_New.html";

template <class T>
void ExchangePtr(T& a, T& b)
{
	T c = a;
	a = b;
	b = c;
}

template <class T>
T GetMinMax(T a, int v1, int v2)
{
	if (a < (T)v1)
		a = (T)v1;
	else if (a > (T)v2)
		a = (T)v2;
	return a;
}

template <class T>
void MinMax(T &a, int v1, int v2)
{
	if (a < (T)v1)
		a = (T)v1;
	else if (a > (T)v2)
		a = (T)v2;
}

template <class T>
void MinMax(T &a, int v2)
{
	if (a > (T)v2)
		a = (T)v2;
}


#include <pshpack1.h>
typedef struct tagMYRGB
{
	union
	{
		COLORREF color;
		struct
		{
			BYTE    rgbBlue;
			BYTE    rgbGreen;
			BYTE    rgbRed;
			BYTE    rgbReserved;
		};
	};
} MYRGB, MYCOLORREF;
#include <poppack.h>

// One message cycle step
bool ProcessMessage(MSG& Msg);

// Predefined user messages
#define UM_USER_CONTROLS    (WM_APP+33)
#define UM_FILL_CMDLIST     (WM_APP+34)
#define UM_SEARCH           (WM_APP+35)
#define UM_SEARCH_FOCUS     (WM_APP+36)
#define UM_EDIT_KILL_FOCUS  (WM_APP+37)
#define UM_PALETTE_FAST_CHG (WM_APP+38)
#define UM_HOTKEY_FILTER    (WM_APP+39)
#define UM_FILTER_FOCUS     (WM_APP+40)
