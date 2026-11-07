
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

#include "Header.h"
#include <commctrl.h>
#include "../common/shlobj.h"
#include <exdisp.h>
#if !defined(__GNUC__) || defined(__MINGW32__)
#pragma warning(push)
#pragma warning(disable: 4091)
#include <dbghelp.h>
#pragma warning(pop)
#include <shobjidl.h>
#include <propkey.h>
#else
#include "../common/DbgHlpGcc.h"
#endif
#include "../common/ConEmuCheck.h"
#include "../common/EnvVar.h"
#include "../common/MBSTR.h"
#include "../common/MSetter.h"
#include "../common/WFiles.h"
#include "helper.h"
#include "AboutDlg.h"
#include "ConEmu.h"
#include "ConEmuApp.h"
#include "ConfirmDlg.h"
#include "DefaultTerm.h"
#include "DontEnable.h"
#include "DpiAware.h"
#include "FontMgr.h"
#include "GlobalHotkeys.h"
#include "HooksUnlocker.h"
#include "IconList.h"
#include "Inside.h"
#include "LngRc.h"
#include "MyClipboard.h"
#include "Options.h"
#include "OptionsClass.h"
#include "Recreate.h"
#include "RealConsole.h"
#include "SetCmdTask.h"
#include "TaskBar.h"
#include "UnitTests.h"
#include "Update.h"
#include "version.h"

#include "../common/StartupEnvEx.h"

#include "../common/Monitors.h"

#ifdef _DEBUG
//	#define SHOW_STARTED_MSGBOX
//	#define WAIT_STARTED_DEBUGGER
//	#define DEBUG_MSG_HOOKS
#endif

#ifdef __CYGWIN__
//	#define SHOW_STARTED_MSGBOX
#endif

#define DEBUGSTRMOVE(s) //DEBUGSTR(s)
#define DEBUGSTRTIMER(s) //DEBUGSTR(s)
#define DEBUGSTRSETHOTKEY(s) //DEBUGSTR(s)
#define DEBUGSTRSHUTSTEP(s) DEBUGSTR(s)
#define DEBUGSTRSTARTUP(s) DEBUGSTR(WIN3264TEST(L"ConEmu.exe: ",L"ConEmu64.exe: ") s L"\n")
#define DEBUGSTRSTARTUPLOG(s) {if (gpConEmu) gpConEmu->LogString(s);} DEBUGSTRSTARTUP(s)


#ifdef MSGLOGGER
BOOL bBlockDebugLog=false, bSendToDebugger=true, bSendToFile=false;
WCHAR *LogFilePath=nullptr;
#endif
#ifndef _DEBUG
BOOL gbNoDblBuffer = false;
#else
BOOL gbNoDblBuffer = false;
#endif

bool gbMessagingStarted = false;


#if defined(__CYGWIN__)
const CLSID CLSID_ShellWindows = {0x9BA05972, 0xF6A8, 0x11CF, {0xA4, 0x42, 0x00, 0xA0, 0xC9, 0x0A, 0x8F, 0x39}};
const IID IID_IShellWindows = {0x85CB6900, 0x4D95, 0x11CF, {0x96, 0x0C, 0x00, 0x80, 0xC7, 0xF4, 0xEE, 0x85}};
#endif

// Debugging purposes
DWORD gnLastMsgTick = (DWORD)-1;
SYSTEMTIME gstLastTimer = {};


//externs
HINSTANCE g_hInstance=nullptr;
HWND ghWnd=nullptr, ghWndWork=nullptr, ghWndApp=nullptr, ghWndDrag=nullptr;
#ifdef _DEBUG
HWND gh__Wnd = nullptr; // Informational, to be sure what handle had our window before been destroyd
#endif
// Если для ярлыка назначен shortcut - может случиться, что в главное окно он не дойдет
WPARAM gnWndSetHotkey = 0, gnWndSetHotkeyOk = 0;
#ifdef _DEBUG
HWND ghConWnd=nullptr;
#endif
CConEmuMain *gpConEmu = nullptr;
//CVirtualConsole *pVCon=nullptr;
Settings  *gpSet = nullptr;
CSettings *gpSetCls = nullptr;
CFontMgr* gpFontMgr = nullptr;
ConEmuHotKeyList* gpHotKeys = nullptr;
//TCHAR temp[MAX_PATH]; -- низзя, очень велик шанс нарваться при многопоточности
HICON hClassIcon = nullptr, hClassIconSm = nullptr;
BOOL gbDebugLogStarted = FALSE;
BOOL gbDebugShowRects = FALSE;
CEStartupEnv* gpStartEnv = nullptr;


const TCHAR *const gsClassName = VirtualConsoleClass; // окна отрисовки
const TCHAR *const gsClassNameParent = VirtualConsoleClassMain; // главное окно
const TCHAR *const gsClassNameWork = VirtualConsoleClassWork; // Holder для всех VCon
const TCHAR *const gsClassNameBack = VirtualConsoleClassBack; // Подложка (со скроллерами) для каждого VCon
const TCHAR *const gsClassNameApp = VirtualConsoleClassApp;


MMap<HWND,CVirtualConsole*> gVConDcMap;
MMap<HWND,CVirtualConsole*> gVConBkMap;


OSVERSIONINFO gOSVer = {};
WORD gnOsVer = 0x500;
bool gbIsWine = false;
bool gbIsDBCS = false;
// Drawing console font face name (default)
wchar_t gsDefGuiFont[32] = L"Lucida Console"; // gbIsWine ? L"Liberation Mono" : L"Lucida Console"
wchar_t gsAltGuiFont[32] = L"Courier New"; // "Lucida Console" is not installed?
// Set this font (default) in real console window to enable unicode support
wchar_t gsDefConFont[32] = DEFAULT_CONSOLE_FONT_NAME; // DBCS ? L"Liberation Mono" : L"Lucida Console"
wchar_t gsAltConFont[32] = L"Courier New"; // "Lucida Console" is not installed?
// Use this (default) in ConEmu interface, where allowed (tabs, status, panel views, ...)
wchar_t gsDefMUIFont[32] = L"Tahoma";         // WindowsVista ? L"Segoe UI" : L"Tahoma"

LPCWSTR GetMouseMsgName(UINT msg)
{
	LPCWSTR pszName;
	switch (msg)
	{
	case WM_MOUSEMOVE: pszName = L"WM_MOUSEMOVE"; break;
	case WM_LBUTTONDOWN: pszName = L"WM_LBUTTONDOWN"; break;
	case WM_LBUTTONUP: pszName = L"WM_LBUTTONUP"; break;
	case WM_LBUTTONDBLCLK: pszName = L"WM_LBUTTONDBLCLK"; break;
	case WM_RBUTTONDOWN: pszName = L"WM_RBUTTONDOWN"; break;
	case WM_RBUTTONUP: pszName = L"WM_RBUTTONUP"; break;
	case WM_RBUTTONDBLCLK: pszName = L"WM_RBUTTONDBLCLK"; break;
	case WM_MBUTTONDOWN: pszName = L"WM_MBUTTONDOWN"; break;
	case WM_MBUTTONUP: pszName = L"WM_MBUTTONUP"; break;
	case WM_MBUTTONDBLCLK: pszName = L"WM_MBUTTONDBLCLK"; break;
	case 0x020A: pszName = L"WM_MOUSEWHEEL"; break;
	case 0x020B: pszName = L"WM_XBUTTONDOWN"; break;
	case 0x020C: pszName = L"WM_XBUTTONUP"; break;
	case 0x020D: pszName = L"WM_XBUTTONDBLCLK"; break;
	case 0x020E: pszName = L"WM_MOUSEHWHEEL"; break;
	default:
		{
			static wchar_t szTmp[32] = L"";
			swprintf_c(szTmp, L"0x%X(%u)", msg, msg);
			pszName = szTmp;
		}
	}
	return pszName;
}

LONG gnMessageNestingLevel = 0;

#ifdef MSGLOGGER

#include "DebugMsgLog.h"

BOOL POSTMESSAGE(HWND h,UINT m,WPARAM w,LPARAM l,BOOL extra)
{
	MCHKHEAP;
	DebugLogMessage(h,m,w,l,1,extra);
	return PostMessage(h,m,w,l);
}
LRESULT SENDMESSAGE(HWND h,UINT m,WPARAM w,LPARAM l)
{
	MCHKHEAP;
	DebugLogMessage(h,m,w,l,0,FALSE);
	return SendMessage(h,m,w,l);
}
#endif

#ifdef _DEBUG
char gsz_MDEBUG_TRAP_MSG[3000];
char gsz_MDEBUG_TRAP_MSG_APPEND[2000];
HWND gh_MDEBUG_TRAP_PARENT_WND = nullptr;
int __stdcall _MDEBUG_TRAP(LPCSTR asFile, int anLine)
{
	//__debugbreak();
	_ASSERT(FALSE);
	wsprintfA(gsz_MDEBUG_TRAP_MSG, "MDEBUG_TRAP\r\n%s(%i)\r\n", asFile, anLine);

	if (gsz_MDEBUG_TRAP_MSG_APPEND[0])
		lstrcatA(gsz_MDEBUG_TRAP_MSG,gsz_MDEBUG_TRAP_MSG_APPEND);

	MessageBoxA(ghWnd,gsz_MDEBUG_TRAP_MSG,"MDEBUG_TRAP",MB_OK|MB_ICONSTOP);
	return 0;
}
int MDEBUG_CHK = TRUE;
#endif


bool LogString(LPCWSTR asInfo, bool abWriteTime /*= true*/, bool abWriteLine /*= true*/)
{
	return gpConEmu->LogString(asInfo, abWriteTime, abWriteLine);
}

void ShutdownGuiStep(LPCWSTR asInfo, int nParm1 /*= 0*/, int nParm2 /*= 0*/, int nParm3 /*= 0*/, int nParm4 /*= 0*/)
{
	wchar_t szFull[512];
	msprintf(szFull, countof(szFull), L"%u:ConEmuG:PID=%u:TID=%u: ",
		GetTickCount(), GetCurrentProcessId(), GetCurrentThreadId());
	if (asInfo)
	{
		int nLen = lstrlen(szFull);
		msprintf(szFull+nLen, countof(szFull)-nLen, asInfo, nParm1, nParm2, nParm3, nParm4);
	}

	LogString(szFull);

#ifdef _DEBUG
	static int nDbg = 0;
	if (!nDbg)
		nDbg = IsDebuggerPresent() ? 1 : 2;
	if (nDbg != 1)
		return;
	DEBUGSTRSHUTSTEP(szFull);
#endif
}

bool GetDlgItemSigned(HWND hDlg, WORD nID, int& nValue, int nMin /*= 0*/, int nMax /*= 0*/)
{
	BOOL lbOk = FALSE;
	int n = (int)GetDlgItemInt(hDlg, nID, &lbOk, TRUE);
	if (!lbOk)
		return false;
	if (nMin || nMax)
	{
		if (nValue < nMin)
			return false;
		if (nMax && nValue > nMax)
			return false;
	}
	nValue = n;
	return true;
}

bool GetDlgItemUnsigned(HWND hDlg, WORD nID, DWORD& nValue, DWORD nMin /*= 0*/, DWORD nMax /*= 0*/)
{
	BOOL lbOk = FALSE;
	DWORD n = GetDlgItemInt(hDlg, nID, &lbOk, FALSE);
	if (!lbOk)
		return false;
	if (nMin || nMax)
	{
		if (nValue < nMin)
			return false;
		if (nMax && nValue > nMax)
			return false;
	}
	nValue = n;
	return true;
}

CEStr GetDlgItemTextPtr(HWND hDlg, WORD nID)
{
	CEStr result;
	MyGetDlgItemText(hDlg, nID, result);
	return result;
}

size_t MyGetDlgItemText(HWND hDlg, WORD nID, CEStr& rsText)
{
	HWND hEdit;

	if (nID)
		hEdit = GetDlgItem(hDlg, nID);
	else
		hEdit = hDlg;

	if (!hEdit)
	{
		rsText.Clear();
		return 0;
	}

	int nLen = GetWindowTextLengthW(hEdit);

	if (nLen > 0)
	{
		if (!rsText.GetBuffer(nLen))
		{
			_ASSERTE(rsText.data() != nullptr)
		}
		else
		{
			rsText.SetAt(0, 0);
			GetWindowTextW(hEdit, rsText.data(), rsText.GetMaxCount());
		}
	}
	else
	{
		_ASSERTE(nLen == 0);
		nLen = 0;
		rsText.Clear();
	}

	return nLen;
}

bool GetColorRef(LPCWSTR pszText, COLORREF* pCR)
{
	if (!pszText || !*pszText)
		return false;

	bool result = false;
	int r = 0, g = 0, b = 0;
	const wchar_t *pch;
	wchar_t *pchEnd = nullptr;
	COLORREF clr = 0;
	bool bHex = false;

	if ((pszText[0] == L'#') // #RRGGBB
		|| (pszText[0] == L'x' || pszText[0] == L'X') // xBBGGRR (COLORREF)
		|| (pszText[0] == L'0' && (pszText[1] == L'x' || pszText[1] == L'X'))) // 0xBBGGRR (COLORREF)
	{
		// Считаем значение 16-ричным rgb кодом
		pch = (pszText[0] == L'0') ? (pszText+2) : (pszText+1);
		clr = wcstoul(pch, &pchEnd, 16);
		bHex = true;
	}
	else if ((pszText[0] == L'0') && (pszText[1] == L'0')) // 00BBGGRR (COLORREF, copy from *.reg)
	{
		// Это может быть 8 цифр (тоже hex) скопированных из reg-файла
		pch = (pszText + 2);
		clr = wcstoul(pch, &pchEnd, 16);
		bHex = (pchEnd && ((pchEnd - pch) == 6));
	}

	if (bHex)
	{
		// Считаем значение 16-ричным rgb кодом
		if (clr && (pszText[0] == L'#'))
		{
			// "#rrggbb", обменять местами rr и gg, нам нужен COLORREF (bbggrr)
			clr = ((clr & 0xFF)<<16) | ((clr & 0xFF00)) | ((clr & 0xFF0000)>>16);
		}
		// Done
		if (pchEnd && (pchEnd > (pszText+1)) && (clr <= 0xFFFFFF) && (*pCR != clr))
		{
			*pCR = clr;
			result = true;
		}
	}
	else
	{
		pch = (wchar_t*)wcspbrk(pszText, L"0123456789");
		pchEnd = nullptr;
		r = pch ? wcstol(pch, &pchEnd, 10) : 0;
		if (pchEnd && (pchEnd > pch))
		{
			pch = (wchar_t*)wcspbrk(pchEnd, L"0123456789");
			pchEnd = nullptr;
			g = pch ? wcstol(pch, &pchEnd, 10) : 0;

			if (pchEnd && (pchEnd > pch))
			{
				pch = (wchar_t*)wcspbrk(pchEnd, L"0123456789");
				pchEnd = nullptr;
				b = pch ? wcstol(pch, &pchEnd, 10) : 0;
			}

			// decimal format of UltraEdit?
			if ((r > 255) && !g && !b)
			{
				g = (r & 0xFF00) >> 8;
				b = (r & 0xFF0000) >> 16;
				r &= 0xFF;
			}

			// Достаточно ввода одной компоненты
			if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255 && *pCR != RGB(r, g, b))
			{
				*pCR = RGB(r, g, b);
				result = true;
			}
		}
	}

	return result;
}


CEStr SelectFolder(LPCWSTR asTitle, LPCWSTR asDefFolder /*= nullptr*/, HWND hParent /*= ghWnd*/, DWORD/*CESelectFileFlags*/ nFlags /*= sff_AutoQuote*/, CRealConsole* apRCon /*= nullptr*/)
{
	CEStr pszResult = nullptr;

	BROWSEINFO bi = {hParent};
	wchar_t szFolder[MAX_PATH+1] = {0};
	if (asDefFolder)
		wcscpy_c(szFolder, asDefFolder);
	bi.pszDisplayName = szFolder;
	wchar_t szTitle[100];
	bi.lpszTitle = lstrcpyn(szTitle, asTitle ? asTitle : L"Choose folder", countof(szTitle));
	bi.ulFlags = BIF_EDITBOX | BIF_RETURNONLYFSDIRS | BIF_VALIDATE;
	bi.lpfn = CRecreateDlg::BrowseCallbackProc;
	bi.lParam = reinterpret_cast<LPARAM>(szFolder);
	LPITEMIDLIST pRc = SHBrowseForFolder(&bi);

	if (pRc)
	{
		if (SHGetPathFromIDList(pRc, szFolder))
		{
			if (nFlags & sff_Cygwin)
			{
				CEStr path;
				if (DupCygwinPath(szFolder, (nFlags & sff_AutoQuote), apRCon ? apRCon->GetMntPrefix() : nullptr, path))
					pszResult = std::move(path);
			}
			else if ((nFlags & sff_AutoQuote) && (wcschr(szFolder, L' ') != nullptr))
			{
				pszResult = CEStr(L"\"", szFolder, L"\"");
			}
			else
			{
				pszResult.Set(szFolder);
			}
		}

		CoTaskMemFree(pRc);
	}

	return pszResult;
}

CEStr SelectFile(LPCWSTR asTitle, LPCWSTR asDefFile /*= nullptr*/, LPCWSTR asDefPath /*= nullptr*/, HWND hParent /*= ghWnd*/, LPCWSTR asFilter /*= nullptr*/, DWORD/*CESelectFileFlags*/ nFlags /*= sff_AutoQuote*/, CRealConsole* apRCon /*= nullptr*/)
{
	CEStr pszResult;

	wchar_t temp[MAX_PATH] = {};
	if (asDefFile)
		_wcscpy_c(temp, countof(temp), asDefFile);

	OPENFILENAME ofn{};
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hParent;
	ofn.lpstrFilter = asFilter ? asFilter : L"All files (*.*)\0*.*\0Text files (*.txt,*.ini,*.log)\0*.txt;*.ini;*.log\0Executables (*.exe,*.com,*.bat,*.cmd)\0*.exe;*.com;*.bat;*.cmd\0Scripts (*.vbs,*.vbe,*.js,*.jse)\0*.vbs;*.vbe;*.js;*.jse\0\0";
	//ofn.lpstrFilter = L"All files (*.*)\0*.*\0\0";
	ofn.lpstrFile = temp;
	ofn.lpstrInitialDir = asDefPath;
	ofn.nMaxFile = countof(temp);
	ofn.lpstrTitle = asTitle ? asTitle : L"Choose file";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
		| OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|((nFlags & sff_SaveNewFile) ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
	// Append extension if user fails to type it
	if (asDefFile && (asDefFile[0] == L'*' && asDefFile[1] == L'.' && asDefFile[2]))
		ofn.lpstrDefExt = (asDefFile+2);

	const BOOL bRc = (nFlags & sff_SaveNewFile)
		? GetSaveFileName(&ofn)
		: GetOpenFileName(&ofn);

	if (bRc)
	{
		if (nFlags & sff_Cygwin)
		{
			CEStr path;
			if (DupCygwinPath(temp, (nFlags & sff_AutoQuote), apRCon ? apRCon->GetMntPrefix() : nullptr, path))
				pszResult = std::move(path);
		}
		else
		{
			if ((nFlags & sff_AutoQuote) && (wcschr(temp, L' ') != nullptr))
			{
				pszResult = CEStr(L"\"", temp, L"\"");
			}
			else
			{
				pszResult.Set(temp);
			}
		}
	}

	return pszResult;
}


// Defined by user char range for using alternative font
bool isCharAltFont(ucs32 inChar)
{
	return gpSet->CheckCharAltFont(inChar);
}



#ifdef DEBUG_MSG_HOOKS
HHOOK ghDbgHook = nullptr;
LRESULT CALLBACK DbgCallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		CWPSTRUCT* p = (CWPSTRUCT*)lParam;
		if (p->message == WM_SETHOTKEY)
		{
			DEBUGSTRSETHOTKEY(L"WM_SETHOTKEY triggered");
		}
	}
	return CallNextHookEx(ghDbgHook, nCode, wParam, lParam);
}
#endif


LRESULT CALLBACK AppWndProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (gpSet->isLogging(4))
	{
		gpConEmu->LogMessage(hWnd, messg, wParam, lParam);
	}

	if (messg == WM_SETHOTKEY)
	{
		gnWndSetHotkey = wParam;
	}

	if (messg == WM_CREATE)
	{
		if (ghWndApp == nullptr)
			ghWndApp = hWnd;
	}
	else if (messg == WM_ACTIVATEAPP)
	{
		if (wParam && ghWnd)
			gpConEmu->setFocus();
	}

	// gh-1306, gh-460
	switch (messg)
	{
	case WM_WINDOWPOSCHANGING:
		// AppWindow must be visible but always out-of-screen
		if (lParam)
		{
			LPWINDOWPOS p = (LPWINDOWPOS)lParam;
			p->flags &= ~SWP_NOMOVE;
			p->x = p->y = WINDOWS_ICONIC_POS;
		}
		break;

	default:
		result = DefWindowProc(hWnd, messg, wParam, lParam);
	}
	return result;
}

// z120713 - В потоке CRealConsole::MonitorThread возвращаются
// отличные от основного потока HWND. В результате, а также из-за
// отложенного выполнения, UpdateServerActive передавал Thaw==FALSE
HWND ghLastForegroundWindow = nullptr;
HWND getForegroundWindow()
{
	HWND h = nullptr;
	if (!ghWnd || isMainThread())
	{
		ghLastForegroundWindow = h = ::GetForegroundWindow();
	}
	else
	{
		h = ghLastForegroundWindow;
		if (h && !IsWindow(h))
			h = nullptr;
	}
	return h;
}

BOOL CheckCreateAppWindow()
{
	if (!gpSet->NeedCreateAppWindow())
	{
		// Если окно не требуется
		if (ghWndApp)
		{
			// Вызов DestroyWindow(ghWndApp); закроет и "дочернее" ghWnd
			_ASSERTE(ghWnd==nullptr);
			if (ghWnd)
				gpConEmu->SetParent(nullptr);
			DestroyWindow(ghWndApp);
			ghWndApp = nullptr;
		}
		return TRUE;
	}

	WNDCLASSEX wc = {sizeof(WNDCLASSEX), CS_DBLCLKS|CS_OWNDC, AppWndProc, 0, 0,
	                 g_hInstance, hClassIcon, LoadCursor(nullptr, IDC_ARROW),
	                 nullptr /*(HBRUSH)COLOR_BACKGROUND*/,
	                 nullptr, gsClassNameApp, hClassIconSm
	                };// | CS_DROPSHADOW

	if (!RegisterClassEx(&wc))
		return FALSE;


	gpConEmu->LogString(L"Creating app window", false);


	//ghWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, gpSet->wndX, gpSet->wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, nullptr, nullptr, (HINSTANCE)g_hInstance, nullptr);
	DWORD style = WS_OVERLAPPEDWINDOW | WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE;
	int nWidth = 100, nHeight = 100, nX = WINDOWS_ICONIC_POS, nY = WINDOWS_ICONIC_POS;
	DWORD exStyle = WS_EX_TOOLWINDOW|WS_EX_ACCEPTFILES;
	// cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4; -- все равно это было не правильно
	ghWndApp = CreateWindowEx(exStyle, gsClassNameApp, gpConEmu->GetDefaultTitle(), style, nX, nY, nWidth, nHeight, nullptr, nullptr, (HINSTANCE)g_hInstance, nullptr);

	if (!ghWndApp)
	{
		WarnCreateWindowFail(L"application window", nullptr, GetLastError());
		return FALSE;
	}

	if (gpSet->isLogging())
	{
		wchar_t szCreated[128];
		swprintf_c(szCreated, L"App window created, HWND=0x%08X\r\n", LODWORD(ghWndApp));
		gpConEmu->LogString(szCreated, false, false);
	}

	return TRUE;
}

LRESULT CALLBACK SkipShowWindowProc(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	if (!gnWndSetHotkey && (messg == WM_SETHOTKEY))
	{
		gnWndSetHotkey = wParam;
	}

	switch (messg)
	{
	case WM_PAINT:
		{
			PAINTSTRUCT ps = {};
			BeginPaint(hWnd, &ps);
			EndPaint(hWnd, &ps);
		}
		result = 0;
		break;

	case WM_ERASEBKGND:
		result = TRUE;
		break;

	default:
		result = DefWindowProc(hWnd, messg, wParam, lParam);
	}

	return result;
}

void SkipOneShowWindow()
{
	static bool bProcessed = false;
	if (bProcessed)
		return; // уже
	bProcessed = true;

	wchar_t szInfo[128];
	STARTUPINFO si = {}; si.cb = sizeof(si);
	GetStartupInfo(&si);
	swprintf_c(szInfo, L"StartupInfo: flags=0x%04X showWindow=%u", si.dwFlags, static_cast<uint32_t>(si.wShowWindow));

	if (!(si.dwFlags & STARTF_USESHOWWINDOW) || (si.wShowWindow == SW_SHOWNORMAL))
	{
		wcscat_c(szInfo, L", SkipOneShowWindow is not required");
		gpConEmu->LogString(szInfo);
		return; // финты не требуются
	}

	const wchar_t szSkipClass[] = L"ConEmuSkipShowWindow";
	WNDCLASSEX wc = {sizeof(WNDCLASSEX), 0, SkipShowWindowProc, 0, 0,
	                 g_hInstance, hClassIcon, LoadCursor(nullptr, IDC_ARROW),
	                 nullptr /*(HBRUSH)COLOR_BACKGROUND*/,
	                 nullptr, szSkipClass, hClassIconSm
	                };// | CS_DROPSHADOW

	if (!RegisterClassEx(&wc))
		return;


	wcscat_c(szInfo, L", processing SkipOneShowWindow");
	gpConEmu->LogString(szInfo);


	gpConEmu->Taskbar_Init();

	//ghWnd = CreateWindow(szClassName, 0, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, gpSet->wndX, gpSet->wndY, cRect.right - cRect.left - 4, cRect.bottom - cRect.top - 4, nullptr, nullptr, (HINSTANCE)g_hInstance, nullptr);
	DWORD style = WS_OVERLAPPED | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	int nWidth=100, nHeight=100, nX = -32000, nY = -32000;
	DWORD exStyle = WS_EX_TOOLWINDOW;
	HWND hSkip = CreateWindowEx(exStyle, szSkipClass, L"", style, nX, nY, nWidth, nHeight, nullptr, nullptr, (HINSTANCE)g_hInstance, nullptr);

	if (hSkip)
	{
		HRGN hRgn = CreateRectRgn(0,0,1,1);
		SetWindowRgn(hSkip, hRgn, FALSE);

		ShowWindow(hSkip, SW_SHOWNORMAL);
		gpConEmu->Taskbar_DeleteTabXP(hSkip);
		DestroyWindow(hSkip);

		if (gpSet->isLogging())
		{
			swprintf_c(szInfo, L"Skip window 0x%08X was created and destroyed", LODWORD(hSkip));
			gpConEmu->LogString(szInfo);
		}
	}

	// Класс более не нужен
	UnregisterClass(szSkipClass, g_hInstance);

	return;
}

struct FindProcessWindowArg
{
	HWND  hwnd;
	DWORD nPID;
};

static BOOL CALLBACK FindProcessWindowEnum(HWND hwnd, LPARAM lParam)
{
	FindProcessWindowArg* pArg = (FindProcessWindowArg*)lParam;

	if (!IsWindowVisible(hwnd))
		return TRUE; // next window

	DWORD nPID = 0;
	if (!GetWindowThreadProcessId(hwnd, &nPID) || (nPID != pArg->nPID))
		return TRUE; // next window

	pArg->hwnd = hwnd;
	return FALSE; // found
}

HWND FindProcessWindow(DWORD nPID)
{
	FindProcessWindowArg args = {nullptr, nPID};
	EnumWindows(FindProcessWindowEnum, (LPARAM)&args);
	return args.hwnd;
}

HWND ghDlgPendingFrom = nullptr;
void PatchMsgBoxIcon(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam)
{
	if (!ghDlgPendingFrom)
		return;

	HWND hFore = GetForegroundWindow();
	HWND hActive = GetActiveWindow();
	if (hFore && (hFore != ghDlgPendingFrom))
	{
		DWORD nPID = 0;
		GetWindowThreadProcessId(hFore, &nPID);
		if (nPID == GetCurrentProcessId())
		{
			wchar_t szClass[32] = L""; GetClassName(hFore, szClass, countof(szClass));
			if (lstrcmp(szClass, L"#32770") == 0)
			{
				// Reset immediately, to avoid stack overflow
				ghDlgPendingFrom = nullptr;
				if (gpConEmu && CDwmHelper::IsWindowsDarkMode())
					gpConEmu->EnableDarkCaption(hFore);
				// And patch the icon
				SendMessage(hFore, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hClassIcon));
				SendMessage(hFore, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hClassIconSm));
			}
		}
	}
}

LONG gnInMsgBox = 0;
int MsgBox(LPCTSTR lpText, UINT uType, LPCTSTR lpCaption /*= nullptr*/, HWND ahParent /*= (HWND)-1*/, bool abModal /*= true*/)
{
	DontEnable de(abModal);

	ghDlgPendingFrom = GetForegroundWindow();

	HWND hParent = gbMessagingStarted
		? ((ahParent == (HWND)-1) ? ghWnd :ahParent)
		: nullptr;

	HooksUnlocker;
	MSetter lInCall(&gnInMsgBox);

	if (gpSet && gpSet->isLogging())
	{
		CEStr lsLog(lpCaption, lpCaption ? L":: " : nullptr, lpText);
		LogString(lsLog);
	}

	// If there were problems with displaying error box, MessageBox will return default button
	// This may cause infinite loops in some cases
	SetLastError(0);
	int nBtn = MessageBox(hParent, lpText ? lpText : L"<nullptr>", lpCaption ? lpCaption : gpConEmu->GetLastTitle(), uType);
	DWORD nErr = GetLastError();

	ghDlgPendingFrom = nullptr;

	UNREFERENCED_PARAMETER(nErr);
	return nBtn;
}

void WarnCreateWindowFail(LPCWSTR pszDescription, HWND hParent, DWORD nErrCode)
{
	wchar_t szCreateFail[256];

	if (gpConEmu && gpConEmu->mp_Inside)
	{
		swprintf_c(szCreateFail,
			L"Inside mode: Parent (%s): PID=%u ParentPID=%u HWND=x%p EXE=",
			(::IsWindow(gpConEmu->mp_Inside->GetParentWnd()) ? L"Valid" : L"Invalid"),
			gpConEmu->mp_Inside->GetParentInfo().ParentPID,
			gpConEmu->mp_Inside->GetParentInfo().ParentParentPID,
			static_cast<LPVOID>(gpConEmu->mp_Inside->GetParentWnd()));
		const CEStr lsLog(szCreateFail, gpConEmu->mp_Inside->GetParentInfo().ExeName);
		LogString(lsLog);
	}

	swprintf_c(szCreateFail,
		L"Create %s FAILED (code=%u)! Parent=x%p%s%s",
		pszDescription ? pszDescription : L"window", nErrCode, static_cast<LPVOID>(hParent),
		(hParent ? (::IsWindow(hParent) ? L" Valid" : L" Invalid") : L""),
		(hParent ? (::IsWindowVisible(hParent) ? L" Visible" : L" Hidden") : L"")
		);
	LogString(szCreateFail);

	// Don't warn, if "Inside" mode was requested and parent was closed
	if (!gpConEmu || !gpConEmu->isInsideInvalid())
	{
		DisplayLastError(szCreateFail, nErrCode);
	}
}

RECT CenterInParent(RECT rcDlg, HWND hParent)
{
	RECT rcParent; GetWindowRect(hParent, &rcParent);

	const int nWidth  = (rcDlg.right - rcDlg.left);
	const int nHeight = (rcDlg.bottom - rcDlg.top);

	MONITORINFO mi = {sizeof(mi)};
	GetNearestMonitorInfo(&mi, nullptr, &rcParent);

	RECT rcCenter = {
		std::max(mi.rcWork.left, rcParent.left + (rcParent.right - rcParent.left - nWidth) / 2),
		std::max(mi.rcWork.top, rcParent.top + (rcParent.bottom - rcParent.top - nHeight) / 2)
	};

	if (((rcCenter.left + nWidth) > mi.rcWork.right)
		&& (rcCenter.left > mi.rcWork.left))
	{
		rcCenter.left = std::max(mi.rcWork.left, (mi.rcWork.right - nWidth));
	}

	if (((rcCenter.top + nHeight) > mi.rcWork.bottom)
		&& (rcCenter.top > mi.rcWork.top))
	{
		rcCenter.top = std::max(mi.rcWork.top, (mi.rcWork.bottom - nHeight));
	}

	rcCenter.right = rcCenter.left + nWidth;
	rcCenter.bottom = rcCenter.top + nHeight;

	return rcCenter;
}

BOOL MoveWindowRect(HWND hWnd, const RECT& rcWnd, BOOL bRepaint)
{
	BOOL lbRc;

	if (gpConEmu && (ghWnd == hWnd))
		lbRc = gpConEmu->setWindowPos(nullptr, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top, SWP_NOZORDER|(bRepaint?0:SWP_NOREDRAW));
	else
		lbRc = MoveWindow(hWnd, rcWnd.left, rcWnd.top, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top, bRepaint);

	return lbRc;
}

HICON CreateNullIcon()
{
	static HICON hNullIcon = nullptr;

	if (!hNullIcon)
	{
		BYTE NilBits[16*16/8] = {};
		BYTE SetBits[16*16/8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
								0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
		hNullIcon = CreateIcon(nullptr, 16, 16, 1, 1, SetBits, NilBits);
	}

	return hNullIcon;
}

void MessageLoop()
{
	MSG Msg = {nullptr};
	gbMessagingStarted = true;

	#ifdef _DEBUG
	wchar_t szDbg[128];
	#endif

	while (GetMessage(&Msg, nullptr, 0, 0))
	{
		#ifdef _DEBUG
		if (Msg.message == WM_TIMER)
		{
			swprintf_c(szDbg, L"WM_TIMER(0x%08X,%u)\n", LODWORD(Msg.hwnd), Msg.wParam);
			DEBUGSTRTIMER(szDbg);
		}
		#endif

		if (!ProcessMessage(Msg))
			break;
	}

	gbMessagingStarted = false;
}

bool PtMouseDblClickTest(const MSG& msg1, const MSG msg2)
{
	if (msg1.message != msg2.message)
		return false;

	// Maximum DblClick time is 5 sec
	UINT nCurTimeDiff = msg2.time - msg1.time;
	if (nCurTimeDiff > 5000)
	{
		return false;
	}
	else if (nCurTimeDiff)
	{
		UINT nTimeDiff = GetDoubleClickTime();
		if (nCurTimeDiff > nTimeDiff)
			return false;
	}

	// Check coord diff
	POINT pt1 = {(int16_t)LOWORD(msg1.lParam), (int16_t)HIWORD(msg1.lParam)};
	POINT pt2 = {(int16_t)LOWORD(msg2.lParam), (int16_t)HIWORD(msg2.lParam)};
	// Due to mouse captures hwnd may differ
	if (msg1.hwnd != msg2.hwnd)
	{
		ClientToScreen(msg1.hwnd, &pt1);
		ClientToScreen(msg2.hwnd, &pt2);
	}

	if ((pt1.x != pt2.x) || (pt1.y != pt2.y))
	{
		bool bDiffOk;
		int dx1 = GetSystemMetrics(SM_CXDOUBLECLK), dx2 = GetSystemMetrics(SM_CYDOUBLECLK);
		bDiffOk = PtDiffTest(pt1.x, pt1.y, pt2.x, pt2.y, dx1, dx2);
		if (!bDiffOk)
		{
			// May be fin in dpi*multiplied?
			int dpiDX = gpSetCls->EvalSize(dx1, esf_Horizontal|esf_CanUseDpi);
			int dpiDY = gpSetCls->EvalSize(dx2, esf_Vertical|esf_CanUseDpi);
			if (PtDiffTest(pt1.x, pt1.y, pt2.x, pt2.y, dpiDX, dpiDY))
				bDiffOk = true;
		}
		if (!bDiffOk)
			return false;
	}

	return true;
}

bool ProcessMessage(MSG& Msg)
{
	bool bRc = true;
	static bool bQuitMsg = false;
	static MSG MouseClickPatch = {};

	#ifdef _DEBUG
	static DWORD LastInsideCheck = 0;
	const  DWORD DeltaInsideCheck = 1000;
	#endif

	MSetter nestedLevel(&gnMessageNestingLevel);

	gnLastMsgTick = GetTickCount();

	if (Msg.message == WM_QUIT)
	{
		bQuitMsg = true;
		bRc = false;
		goto wrap;
	}

	// Do minimal in-memory logging (small circular buffer)
	ConEmuMsgLogger::Log(Msg, ConEmuMsgLogger::msgCommon);

	if (gpConEmu)
	{
		#ifdef _DEBUG
		if (gpConEmu->mp_Inside)
		{
			DWORD nCurTick = GetTickCount();
			if (!LastInsideCheck)
			{
				LastInsideCheck = nCurTick;
			}
			else if ((LastInsideCheck - nCurTick) >= DeltaInsideCheck)
			{
				if (gpConEmu->isInsideInvalid())
				{
					// Show assertion once
					static bool bWarned = false;
					if (!bWarned)
					{
						bWarned = true;
						_ASSERTE(FALSE && "Parent was terminated, but ConEmu wasn't");
					}
				}
				LastInsideCheck = nCurTick;
			}
		}
		#endif

		if (gpConEmu->isDialogMessage(Msg))
			goto wrap;

		switch (Msg.message)
		{
		case WM_SYSCOMMAND:
			if (gpConEmu->isSkipNcMessage(Msg))
				goto wrap;
			break;
		case WM_HOTKEY:
			gpConEmu->GetGlobalHotkeys().OnWmHotkey(Msg.wParam, Msg.time);
			goto wrap;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
			// gh#470: If user holds Alt key, than we will not receive DblClick messages, only single clicks...
			if (PtMouseDblClickTest(MouseClickPatch, Msg))
			{
				// Convert Click to DblClick message
				_ASSERTE((WM_LBUTTONDBLCLK-WM_LBUTTONDOWN)==2 && (WM_RBUTTONDBLCLK-WM_RBUTTONDOWN)==2 && (WM_MBUTTONDBLCLK-WM_MBUTTONDOWN)==2);
				_ASSERTE((WM_XBUTTONDBLCLK-WM_XBUTTONDOWN)==2);
				Msg.message += (WM_LBUTTONDBLCLK - WM_LBUTTONDOWN);
			}
			else
			{
				// Diffent mouse button was pressed, or not saved yet
				// Store mouse message
				MouseClickPatch = Msg;
			}
			break;
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
			ZeroStruct(MouseClickPatch);
			break;
		}
	}

	TranslateMessage(&Msg);
	DispatchMessage(&Msg);

wrap:
	return bRc;
}

HWND FindTopExplorerWindow()
{
	wchar_t szClass[MAX_PATH] = L"";
	HWND hwndFind = nullptr;

	while ((hwndFind = FindWindowEx(nullptr, hwndFind, nullptr, nullptr)) != nullptr)
	{
		if ((GetClassName(hwndFind, szClass, countof(szClass)) > 0)
			&& CDefTermBase::IsExplorerWindowClass(szClass))
			break;
	}

	return hwndFind;
}

CEStr getFocusedExplorerWindowPath()
{
#define FE_CHECK_OUTER_FAIL(statement) \
	if (!SUCCEEDED(statement)) goto outer_fail;

#define FE_CHECK_FAIL(statement) \
	if (!SUCCEEDED(statement)) goto fail;

#define FE_RELEASE(hnd) \
	if (hnd) { hnd->Release(); hnd = nullptr; }

	CEStr ret;
	wchar_t szPath[MAX_PATH] = L"";

	IShellBrowser *psb = nullptr;
	IShellView *psv = nullptr;
	IFolderView *pfv = nullptr;
	IPersistFolder2 *ppf2 = nullptr;
	IDispatch  *pdisp = nullptr;
	IWebBrowserApp *pwba = nullptr;
	IServiceProvider *psp = nullptr;
	IShellWindows *psw = nullptr;

	VARIANT v;
	HWND hwndWBA;
	LPITEMIDLIST pidlFolder;

	BOOL fFound = FALSE;
	HWND hwndFind = FindTopExplorerWindow();

	FE_CHECK_OUTER_FAIL(CoCreateInstance(CLSID_ShellWindows, nullptr, CLSCTX_ALL,
		IID_IShellWindows, (void**)&psw))

	V_VT(&v) = VT_I4;
	for (V_I4(&v) = 0; !fFound && psw->Item(v, &pdisp) == S_OK; V_I4(&v)++)
	{
		FE_CHECK_FAIL(pdisp->QueryInterface(IID_IWebBrowserApp, (void**)&pwba))
		FE_CHECK_FAIL(pwba->get_HWND((LONG_PTR*)&hwndWBA))

		if (hwndWBA != hwndFind)
			goto fail;

		fFound = TRUE;
		FE_CHECK_FAIL(pwba->QueryInterface(IID_IServiceProvider, (void**)&psp))
		FE_CHECK_FAIL(psp->QueryService(SID_STopLevelBrowser, IID_IShellBrowser, (void**)&psb))
		FE_CHECK_FAIL(psb->QueryActiveShellView(&psv))
		FE_CHECK_FAIL(psv->QueryInterface(IID_IFolderView, (void**)&pfv))
		FE_CHECK_FAIL(pfv->GetFolder(IID_IPersistFolder2, (void**)&ppf2))
		FE_CHECK_FAIL(ppf2->GetCurFolder(&pidlFolder))

		if (!SHGetPathFromIDList(pidlFolder, szPath) || !*szPath)
			goto fail;

		ret.Set(szPath);

		CoTaskMemFree(pidlFolder);

		fail:
		FE_RELEASE(ppf2)
		FE_RELEASE(pfv)
		FE_RELEASE(psv)
		FE_RELEASE(psb)
		FE_RELEASE(psp)
		FE_RELEASE(pwba)
		FE_RELEASE(pdisp)
	}

	outer_fail:
	FE_RELEASE(psw)

	return ret;

#undef FE_CHECK_OUTER_FAIL
#undef FE_CHECK_FAIL
#undef FE_RELEASE
}

#ifndef __GNUC__
// Creates a CLSID_ShellLink to insert into the Tasks section of the Jump List.  This type of Jump
// List item allows the specification of an explicit command line to execute the task.
// Used only for JumpList creation...
static HRESULT _CreateShellLink(PCWSTR pszArguments, PCWSTR pszPrefix, PCWSTR pszTitle, IShellLink **ppsl)
{
	if ((!pszArguments || !*pszArguments) && (!pszTitle || !*pszTitle))
	{
		return E_INVALIDARG;
	}

	LPCWSTR pszConfig = gpSetCls->GetConfigName();
	if (pszConfig && !*pszConfig)
		pszConfig = nullptr;

	CEStr lsTempBuf;
	LPCWSTR pszConEmuStartArgs = gpConEmu->MakeConEmuStartArgs(lsTempBuf);
	_ASSERTE(!pszConEmuStartArgs || pszConEmuStartArgs[_tcslen(pszConEmuStartArgs)-1]==L' ');

	wchar_t* pszBuf = nullptr;
	if (!pszArguments || !*pszArguments)
	{
		size_t cchMax = _tcslen(pszTitle)
			+ (pszPrefix ? _tcslen(pszPrefix) : 0)
			+ (pszConEmuStartArgs ? _tcslen(pszConEmuStartArgs) : 0)
			+ 32;

		pszBuf = (wchar_t*)malloc(cchMax*sizeof(*pszBuf));
		if (!pszBuf)
			return E_UNEXPECTED;

		pszBuf[0] = 0;
		if (pszPrefix && *pszPrefix)
		{
			_wcscat_c(pszBuf, cchMax, pszPrefix);
			_wcscat_c(pszBuf, cchMax, L" ");
		}
		if (pszConEmuStartArgs && *pszConEmuStartArgs)
		{
			_wcscat_c(pszBuf, cchMax, pszConEmuStartArgs);
		}
		_wcscat_c(pszBuf, cchMax, L"-run ");
		_wcscat_c(pszBuf, cchMax, pszTitle);
		pszArguments = pszBuf;
	}
	else if (pszPrefix)
	{
		size_t cchMax = _tcslen(pszArguments)
			+ _tcslen(pszPrefix)
			+ (pszConfig ? _tcslen(pszConfig) : 0)
			+ 32;
		pszBuf = (wchar_t*)malloc(cchMax*sizeof(*pszBuf));
		if (!pszBuf)
			return E_UNEXPECTED;

		pszBuf[0] = 0;
		_wcscat_c(pszBuf, cchMax, pszPrefix);
		_wcscat_c(pszBuf, cchMax, L" ");
		if (pszConfig)
		{
			_wcscat_c(pszBuf, cchMax, L"-config \"");
			_wcscat_c(pszBuf, cchMax, pszConfig);
			_wcscat_c(pszBuf, cchMax, L"\" ");
		}
		_wcscat_c(pszBuf, cchMax, L"-run ");
		_wcscat_c(pszBuf, cchMax, pszArguments);
		pszArguments = pszBuf;
	}

	IShellLink *psl;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&psl);
	if (SUCCEEDED(hr))
	{
		// Determine our executable's file path so the task will execute this application
		WCHAR szAppPath[MAX_PATH];
		if (GetModuleFileName(nullptr, szAppPath, ARRAYSIZE(szAppPath)))
		{
			hr = psl->SetPath(szAppPath);

			// Иконка
			CmdArg szTmp;
			CEStr szIcon; int iIcon = 0;
			CEStr szBatch;
			LPCWSTR pszTemp = pszArguments;
			LPCWSTR pszIcon = nullptr;
			RConStartArgsEx args;

			while ((pszTemp = NextArg(pszTemp, szTmp)))
			{
				if (szTmp.ms_Val[0] == L'/')
					szTmp.ms_Val[0] = L'-';

				if (szTmp.IsSwitch(L"-icon"))
				{
					if ((pszTemp = NextArg(pszTemp, szTmp)))
						pszIcon = szTmp;
					break;
				}
				else if (szTmp.OneOfSwitches(L"-run",L"-cmd"))
				{
					if ((*pszTemp == CmdFilePrefix)
						|| (*pszTemp == TaskBracketLeft) || (lstrcmp(pszTemp, AutoStartTaskName) == 0))
					{
						szBatch = gpConEmu->LoadConsoleBatch(pszTemp);
					}

					if (!szBatch.IsEmpty())
					{
						pszTemp = gpConEmu->ParseScriptLineOptions(szBatch, nullptr, &args);

						// Icon may be defined in -new_console:C:...
						if (!pszIcon)
						{
							if (!args.pszIconFile)
							{
								_ASSERTE(args.pszSpecialCmd == nullptr);
								args.pszSpecialCmd = lstrdup(pszTemp).Detach();
								args.ProcessNewConArg();
							}
							if (args.pszIconFile)
								pszIcon = args.pszIconFile;
						}
					}

					if (!pszIcon)
					{
						szTmp.Clear();
						if ((pszTemp = NextArg(pszTemp, szTmp)))
							pszIcon = szTmp;
					}
					break;
				}
			}

			szIcon.Clear();
			if (pszIcon && *pszIcon)
			{
				CEStr lsTempIcon;
				lsTempIcon.Set(pszIcon);
				CIconList::ParseIconFileIndex(lsTempIcon, iIcon);

				CEStr szIconExp = ExpandEnvStr(lsTempIcon);
				LPCWSTR pszSearch = szIconExp.IsEmpty() ? lsTempIcon.ms_Val : szIconExp.ms_Val;

				if ((!apiGetFullPathName(pszSearch, szIcon)
						|| !FileExists(szIcon))
					&& !apiSearchPath(nullptr, pszSearch, nullptr, szIcon)
					&& !apiSearchPath(nullptr, pszSearch, L".exe", szIcon))
				{
					szIcon.Clear();
					iIcon = 0;
				}
			}

			psl->SetIconLocation(szIcon.IsEmpty() ? szAppPath : szIcon.ms_Val, iIcon);

			DWORD n = GetCurrentDirectory(countof(szAppPath), szAppPath);
			if (n && (n < countof(szAppPath)))
				psl->SetWorkingDirectory(szAppPath);

			if (SUCCEEDED(hr))
			{
				hr = psl->SetArguments(pszArguments);
				if (SUCCEEDED(hr))
				{
					// The title property is required on Jump List items provided as an IShellLink
					// instance.  This value is used as the display name in the Jump List.
					IPropertyStore *pps;
					hr = psl->QueryInterface(IID_PPV_ARGS(&pps));
					if (SUCCEEDED(hr))
					{
						PROPVARIANT propvar = {VT_BSTR};
						//hr = InitPropVariantFromString(pszTitle, &propvar);
						propvar.bstrVal = ::SysAllocString(pszTitle);
						hr = pps->SetValue(PKEY_Title, propvar);
						if (SUCCEEDED(hr))
						{
							hr = pps->Commit();
							if (SUCCEEDED(hr))
							{
								hr = psl->QueryInterface(IID_PPV_ARGS(ppsl));
							}
						}
						//PropVariantClear(&propvar);
						::SysFreeString(propvar.bstrVal);
						pps->Release();
					}
				}
			}
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		psl->Release();
	}

	if (pszBuf)
		free(pszBuf);
	return hr;
}

// The Tasks category of Jump Lists supports separator items.  These are simply IShellLink instances
// that have the PKEY_AppUserModel_IsDestListSeparator property set to TRUE.  All other values are
// ignored when this property is set.
HRESULT _CreateSeparatorLink(IShellLink **ppsl)
{
	IPropertyStore *pps;
	HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IPropertyStore, (void**)&pps);
	if (SUCCEEDED(hr))
	{
		PROPVARIANT propvar = {VT_BOOL};
		//hr = InitPropVariantFromBoolean(TRUE, &propvar);
		propvar.boolVal = VARIANT_TRUE;
		hr = pps->SetValue(PKEY_AppUserModel_IsDestListSeparator, propvar);
		if (SUCCEEDED(hr))
		{
			hr = pps->Commit();
			if (SUCCEEDED(hr))
			{
				hr = pps->QueryInterface(IID_PPV_ARGS(ppsl));
			}
		}
		//PropVariantClear(&propvar);
		pps->Release();
	}
	return hr;
}
#endif

bool UpdateWin7TaskList(bool bForce, bool bNoSuccMsg /*= false*/)
{
	// Jump Lists appears in Windows 7
	if (!IsWin7())
	{
		LogString(L"Jump Lists: Are supported only in Windows 7 or higher");
		return false;
	}

	// Updating takes some time, do it only when really required (requested)
	TODO("Option for automatic update of Jump Lists");
	if (!bForce)
	{
		LogString(L"Jump Lists: Update skipped");
		return false;
	}

	bool bSucceeded = false;

#ifdef __GNUC__
	// MsgBox logs the text itself
	MBoxA(L"Sorry, UpdateWin7TaskList is not available in GCC!");
#else
	SetCursor(LoadCursor(nullptr, IDC_WAIT));

	LPCWSTR pszTasks[32] = {};
	LPCWSTR pszTasksPrefix[32] = {};
	LPCWSTR pszHistory[32] = {};
	LPCWSTR pszCurCmd = nullptr;
	size_t nTasksCount = 0, nHistoryCount = 0;

	// Add commands from history
	if (gpSet->isStoreTaskbarCommands)
	{
		pszCurCmd = SkipNonPrintable(gpConEmu->opt.runCommand);
		if (!pszCurCmd || !*pszCurCmd)
		{
			pszCurCmd = nullptr;
		}

		// Commands form history
		LPCWSTR pszCommand;
		while (((pszCommand = gpSet->HistoryGet(static_cast<int>(nHistoryCount)))) && (nHistoryCount < countof(pszHistory)))
		{
			// Don't add current command to pszCommand, it goes to the end
			if (!pszCurCmd || (lstrcmpi(pszCurCmd, pszCommand) != 0))
			{
				pszHistory[nHistoryCount++] = pszCommand;
			}
			pszCommand += _tcslen(pszCommand)+1;
		}

		if (pszCurCmd)
			nHistoryCount++;
	}

	// Add ConEmu Tasks to TaskBar
	if (gpSet->isStoreTaskbarkTasks)
	{
		int nGroup = 0;
		const CommandTasks* pGrp = nullptr;
		while ((pGrp = gpSet->CmdTaskGet(nGroup++)) && (nTasksCount < countof(pszTasks)))
		{
			if (pGrp->pszName && *pGrp->pszName
				&& !(pGrp->Flags & CETF_NO_TASKBAR))
			{
				pszTasksPrefix[nTasksCount] = pGrp->pszGuiArgs;
				pszTasks[nTasksCount++] = pGrp->pszName;
			}
		}
	}


	// The visible categories are controlled via the ICustomDestinationList interface.  If not customized,
	// applications will get the Recent category by default.
	ICustomDestinationList *pcdl = nullptr;
	HRESULT hr = CoCreateInstance(CLSID_DestinationList, nullptr, CLSCTX_INPROC_SERVER, IID_ICustomDestinationList, (void**)&pcdl);
	if (FAILED(hr) || !pcdl)
	{
		DisplayLastError(L"ICustomDestinationList create failed", (DWORD)hr);
	}
	else
	{
		UINT cMinSlots = 0;
		IObjectArray *poaRemoved = nullptr;
		hr = pcdl->BeginList(&cMinSlots, IID_PPV_ARGS(&poaRemoved));
		if (FAILED(hr))
		{
			DisplayLastError(L"pcdl->BeginList failed", (DWORD)hr);
		}
		else
		{
			// cMinSlots actually must control only ‘last items’ in jump lists
			if ((nHistoryCount > 0) && (nHistoryCount > std::max(cMinSlots,(UINT)3)))
				nHistoryCount = std::max(cMinSlots,(UINT)3);

			// And we tries to add all Tasks with flag ‘Taskbar jump lists’

			wchar_t szInfo[128];
			msprintf(szInfo, countof(szInfo), L"Jump Lists update started, Tasks: %u, History: %u", nTasksCount, nHistoryCount);
			LogString(szInfo);

			IObjectCollection *poc = nullptr;
			hr = CoCreateInstance(CLSID_EnumerableObjectCollection, nullptr, CLSCTX_INPROC, IID_PPV_ARGS(&poc));
			if (FAILED(hr) || !poc)
			{
				DisplayLastError(L"IObjectCollection create failed", (DWORD)hr);
			}
			else
			{
				IShellLink * psl = nullptr;
				bool bNeedSeparator = false;
				bool bEmpty = true;


				// Add ConEmu's Tasks
				if (SUCCEEDED(hr) && gpSet->isStoreTaskbarkTasks && nTasksCount)
				{
					LogString(L"Jump Lists: Tasks");
					for (size_t i = 0; (i < countof(pszTasks)) && pszTasks[i]; i++)
					{
						hr = _CreateShellLink(nullptr, pszTasksPrefix[i], pszTasks[i], &psl);

						if (SUCCEEDED(hr))
						{
							hr = poc->AddObject(psl);
							psl->Release();
							if (SUCCEEDED(hr))
							{
								LogString(pszTasks[i]);
								bNeedSeparator = true;
								bEmpty = false;
							}
						}

						if (FAILED(hr))
						{
							DisplayLastError(L"Add task (CmdGroup) failed", (DWORD)hr);
							break;
						}
					}
				} // if (SUCCEEDED(hr) && gpSet->isStoreTaskbarkTasks && nTasksCount)


				// Add commands from History
				if (SUCCEEDED(hr) && gpSet->isStoreTaskbarCommands && (nHistoryCount || pszCurCmd))
				{
					LogString(L"Jump Lists: History");

					if (bNeedSeparator)
					{
						bNeedSeparator = false; // один раз
						hr = _CreateSeparatorLink(&psl);
						if (SUCCEEDED(hr))
						{
							hr = poc->AddObject(psl);
							psl->Release();
							if (SUCCEEDED(hr))
								bEmpty = false;
						}
					}

					if (SUCCEEDED(hr) && pszCurCmd)
					{
						hr = _CreateShellLink(pszCurCmd, nullptr, pszCurCmd, &psl);

						if (SUCCEEDED(hr))
						{
							hr = poc->AddObject(psl);
							psl->Release();
							if (SUCCEEDED(hr))
							{
								LogString(pszCurCmd);
								bEmpty = false;
							}
						}

						if (FAILED(hr))
						{
							DisplayLastError(L"Add task (pszCurCmd) failed", (DWORD)hr);
						}
					}

					for (size_t i = 0; SUCCEEDED(hr) && (i < countof(pszHistory)) && pszHistory[i]; i++)
					{
						hr = _CreateShellLink(nullptr, nullptr, pszHistory[i], &psl);

						if (SUCCEEDED(hr))
						{
							hr = poc->AddObject(psl);
							psl->Release();
							if (SUCCEEDED(hr))
							{
								LogString(pszHistory[i]);
								bEmpty = false;
							}
						}

						if (FAILED(hr))
						{
							DisplayLastError(L"Add task (pszHistory) failed", (DWORD)hr);
							break;
						}
					}
				} // if (SUCCEEDED(hr) && gpSet->isStoreTaskbarCommands && (nHistoryCount || pszCurCmd))


				// Now we are ready to put items to Jump List
				if (SUCCEEDED(hr))
				{
					IObjectArray * poa = nullptr;
					hr = poc->QueryInterface(IID_PPV_ARGS(&poa));
					if (FAILED(hr) || !poa)
					{
						DisplayLastError(L"poc->QueryInterface(IID_PPV_ARGS(&poa)) failed", (DWORD)hr);
					}
					else
					{
						// Add the tasks to the Jump List. Tasks always appear in the canonical "Tasks"
						// category that is displayed at the bottom of the Jump List, after all other
						// categories.
						hr = bEmpty ? S_OK : pcdl->AddUserTasks(poa);
						if (FAILED(hr))
						{
							DisplayLastError(L"pcdl->AddUserTasks(poa) failed", (DWORD)hr);
						}
						else
						{
							// Commit the list-building transaction.
							hr = pcdl->CommitList();
							if (FAILED(hr))
							{
								DisplayLastError(L"pcdl->CommitList() failed", (DWORD)hr);
							}
							else
							{
								LogString(L"Jump Lists: Updated successfully");

								if (!bNoSuccMsg)
								{
									MsgBox(CLngRc::getRsrc(lng_JumpListUpdated/*"Taskbar jump list was updated successfully"*/),
										MB_ICONINFORMATION, gpConEmu->GetDefaultTitle(), ghOpWnd, true);
								}

								bSucceeded = true;
							}
						}
						poa->Release();
					}
				} // End of apply
				poc->Release();
			}

			if (poaRemoved)
				poaRemoved->Release();
		}

		pcdl->Release();
	}



	// В Win7 можно также показывать в JumpList "документы" (ярлыки, пути, и т.п.)
	// Но это не то... Похоже, чтобы добавить такой "путь" в Recent/Frequent list
	// нужно создавать физический файл (например, с расширением ".conemu"),
	// и (!) регистрировать для него обработчиком conemu.exe

	SetCursor(LoadCursor(nullptr, IDC_ARROW));
#endif // __GNUC__

	return bSucceeded;
}

// Set our unique application-defined Application User Model ID for Windows 7 TaskBar
// The function also fills the gpConEmu->AppID variable, so it's called regardless of OS ver
HRESULT UpdateAppUserModelID()
{
	bool bSpecialXmlFile = false;
	LPCWSTR pszConfigFile = gpConEmu->ConEmuXml(&bSpecialXmlFile);
	LPCWSTR pszConfigName = gpSetCls->GetConfigName();

	if (bSpecialXmlFile && pszConfigFile && *pszConfigFile)
	{
		CEStr szXmlFile;
		if (gpConEmu->FindConEmuXml(szXmlFile)
			&& (0 == szXmlFile.Compare(pszConfigFile, false)))
		{
			pszConfigFile = nullptr; // -loadcfgfile is the same as used by default, don't add path file to AppID
			bSpecialXmlFile = false;
		}
	}

	wchar_t szSuffix[64] = L"";

	// Don't change the ID if application was started without arguments changing:
	// ‘config-name’, ‘config-file’, ‘registry-use’, ‘basic-settings’, ‘quake/noquake’
	if (!gpSetCls->isResetBasicSettings
		&& !gpConEmu->opt.ForceUseRegistryPrm
		&& !bSpecialXmlFile
		&& !(pszConfigName && *pszConfigName)
		&& !gpConEmu->opt.QuakeMode.Exists
		)
	{
		if (IsWindows7)
			LogString(L"AppUserModelID was not changed due to special switches absence");
		gpConEmu->SetAppID(szSuffix);
		return S_FALSE;
	}

	// The MSDN says: An application must provide its AppUserModelID in the following form.
	// It can have no more than 128 characters and cannot contain spaces. Each section should be camel-cased.
	//    CompanyName.ProductName.SubProduct.VersionInformation
	// CompanyName and ProductName should always be used, while the SubProduct and VersionInformation portions are optional

	CEStr lsTempBuf;

	if (gpConEmu->opt.QuakeMode.Exists)
	{
		switch (gpConEmu->opt.QuakeMode.GetInt())
		{
		case 1: case 2:
			wcscat_c(szSuffix, L"::Quake"); break;
		default:
			wcscat_c(szSuffix, L"::NoQuake");
		}
	}

	// Config type/file + [.[No]Quake]
	if (gpSetCls->isResetBasicSettings)
	{
		lsTempBuf.Append(L"::Basic", szSuffix);
	}
	else if (gpConEmu->opt.ForceUseRegistryPrm)
	{
		lsTempBuf.Append(L"::Registry", szSuffix);
	}
	else if (bSpecialXmlFile && pszConfigFile && *pszConfigFile)
	{
		lsTempBuf.Append(L"::", pszConfigFile, szSuffix);
	}
	else
	{
		lsTempBuf.Append(L"::Xml", szSuffix);
	}

	// Named configuration?
	if (!gpSetCls->isResetBasicSettings
		&& (pszConfigName && *pszConfigName))
	{
		lsTempBuf.Append(L"::", pszConfigName);
	}

	// Create hash - AppID (will go to mapping)
	gpConEmu->SetAppID(lsTempBuf);

	// Further steps are required in Windows7+ only
	if (!IsWindows7)
	{
		return S_FALSE;
	}

	// Prepare the string
	lsTempBuf.Set(gpConEmu->ms_AppID);
	wchar_t* pszColon = wcschr(lsTempBuf.ms_Val, L':');
	if (pszColon)
	{
		_ASSERTE(pszColon[0]==L':' && pszColon[1]==L':' && isDigit(pszColon[2]) && "::<CESERVER_REQ_VER> is expected at the tail!");
		*pszColon = 0;
	}
	else
	{
		_ASSERTE(pszColon!=nullptr && "::<CESERVER_REQ_VER> is expected at the tail!");
	}
	CEStr AppID(APP_MODEL_ID_PREFIX/*L"Maximus5.ConEmu."*/, lsTempBuf.ms_Val);

	// And update it
	HRESULT hr = E_NOTIMPL;
	typedef HRESULT (WINAPI* SetCurrentProcessExplicitAppUserModelID_t)(PCWSTR AppID);
	HMODULE hShell = GetModuleHandle(L"Shell32.dll");
	SetCurrentProcessExplicitAppUserModelID_t fnSetAppUserModelID = hShell
		? (SetCurrentProcessExplicitAppUserModelID_t)GetProcAddress(hShell, "SetCurrentProcessExplicitAppUserModelID")
		: nullptr;
	if (fnSetAppUserModelID)
	{
		hr = fnSetAppUserModelID(AppID);
		_ASSERTE(hr == S_OK);
	}

	// Log the change
	wchar_t szLog[200];
	swprintf_c(szLog, L"AppUserModelID was changed to `%s` Result=x%08X", AppID.c_str(L""), static_cast<DWORD>(hr));
	LogString(szLog);

	return hr;
}

bool CheckLockFrequentExecute(DWORD& Tick, DWORD Interval)
{
	DWORD CurTick = GetTickCount();
	bool bUnlock = false;
	if ((CurTick - Tick) >= Interval)
	{
		Tick = CurTick;
		bUnlock = true;
	}
	return bUnlock;
}

void RaiseTestException()
{
	DebugBreak();
}

#include "../common/Dump.h"

LONG WINAPI CreateDumpOnException(LPEXCEPTION_POINTERS ExceptionInfo)
{
	const bool inTestException = ExceptionInfo && ExceptionInfo->ExceptionRecord
		&& (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_CONEMU_MEMORY_DUMP);

	ConEmuDumpInfo dumpInfo{};
	const DWORD dwErr = CreateDumpForReport(ExceptionInfo, dumpInfo);
	wchar_t szAdd[1500];
	wcscpy_c(szAdd, dumpInfo.fullInfo);
	if (dumpInfo.dumpFile[0])
	{
		wcscat_c(szAdd, L"\r\n\r\n" L"Memory dump was saved to\r\n");
		wcscat_c(szAdd, dumpInfo.dumpFile);
		wcscat_c(szAdd, L"\r\n\r\n" L"Please Zip it and send to developer (via DropBox etc.)\r\n");
		wcscat_c(szAdd, CEREPORTCRASH /* https://conemu.github.io/en/Issues.html... */);
	}
	wcscat_c(szAdd, L"\r\n\r\nPress <Yes> to copy this text to clipboard\r\nand open project web page");

	const int nBtn = DisplayLastError(szAdd, dwErr ? dwErr : -1, MB_YESNO | MB_ICONSTOP | MB_SYSTEMMODAL);
	if (nBtn == IDYES)
	{
		CopyToClipboard(dumpInfo.fullInfo);
		ConEmuAbout::OnInfo_ReportCrash(nullptr);
	}

	//if (inTestException)
	//	return EXCEPTION_CONTINUE_EXECUTION;
	return EXCEPTION_EXECUTE_HANDLER;
}

namespace {
	LONG DumpCurrentProcess(LPEXCEPTION_POINTERS ExceptionInfo)
	{
		if (!ExceptionInfo || !ExceptionInfo->ExceptionRecord
			|| ExceptionInfo->ExceptionRecord->ExceptionCode != EXCEPTION_CONEMU_MEMORY_DUMP
			|| ExceptionInfo->ExceptionRecord->NumberParameters != 1
			|| ExceptionInfo->ExceptionRecord->ExceptionInformation[0] == 0)
			return EXCEPTION_EXECUTE_HANDLER;
		ConEmuDumpInfo* dumpInfo = reinterpret_cast<ConEmuDumpInfo*>(ExceptionInfo->ExceptionRecord->ExceptionInformation[0]);

		const DWORD dwErr = CreateDumpForReport(ExceptionInfo, *dumpInfo);
		if (dwErr != 0)
		{
			wchar_t szErrInfo[120] = L"";
			swprintf_c(szErrInfo, L"\n\nCreateDumpForReport failed with code %u\n", dwErr);
			wcscat_c(dumpInfo->fullInfo, szErrInfo);
		}

		return EXCEPTION_EXECUTE_HANDLER;
	}
}

bool DumpCurrentProcess(ConEmuDumpInfo& dumpInfo)
{
	int step = 0;
	dumpInfo.fullInfo[0] = 0;
	dumpInfo.dumpFile[0] = 0;
	__try
	{
		ULONG_PTR arguments[1] = { reinterpret_cast<ULONG_PTR>(&dumpInfo) };
		RaiseException(EXCEPTION_CONEMU_MEMORY_DUMP, EXCEPTION_NONCONTINUABLE_EXCEPTION, 1, arguments);
		step = 1;
	}
	__except (DumpCurrentProcess(GetExceptionInformation()))
	{
		step = 2;
	}
	std::ignore = step;
	return step == 2 && dumpInfo.fullInfo[0] != 0 && dumpInfo.result == 0;
}

// ReSharper disable twice CppParameterMayBeConst
void AssertBox(LPCTSTR szText, LPCTSTR szFile, const UINT nLine, LPEXCEPTION_POINTERS exceptionInfo /*= nullptr*/)
{
	#ifdef _DEBUG
	//_ASSERTE(FALSE);
	#endif

	static bool bInAssert = false;

	int nRet = IDRETRY;

	const DWORD nPreCode = GetLastError();
	wchar_t szLine[16], szCodes[128];
	ConEmuDumpInfo dumpInfo{};
	CEStr szFull, dumpMessage;

	#ifdef _DEBUG
	MyAssertDumpToFile(szFile, nLine, szText);
	#endif

	LPCWSTR  pszTitle = gpConEmu ? gpConEmu->GetDefaultTitle() : nullptr;
	if (!pszTitle || !*pszTitle) { pszTitle = L"?ConEmu?"; }

	// Prepare assertion message
	{
		SYSTEMTIME st{}; GetSystemTime(&st);
		wchar_t szDashes[] = L"-----------------------\r\n", szPID[120];
		swprintf_c(szPID, L"PID=%u TID=%u at %02u:%02u:%02u.%03u",
			GetCurrentProcessId(), GetCurrentThreadId(), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
		const CEStr lsBuild(L"ConEmu ", (gpConEmu && gpConEmu->ms_ConEmuBuild[0]) ? gpConEmu->ms_ConEmuBuild : L"<UnknownBuild>",
							L" [", WIN3264TEST(L"32",L"64"), RELEASEDEBUGTEST(nullptr,L"D"), L"] ");
		const CEStr lsAssertion(L"Assertion:\r\n", lsBuild, szPID, L"\r\n");
		const CEStr lsWhere(L"\r\n", StripSourceRoot(szFile), L":", ultow_s(nLine, szLine, 10), L"\r\n", szDashes);
		const CEStr lsHeader(lsAssertion,
			(gpConEmu && gpConEmu->ms_ConEmuExe[0]) ? gpConEmu->ms_ConEmuExe : L"<nullptr>",
			lsWhere, szText, L"\r\n", szDashes, L"\r\n");

		szFull = CEStr(
			lsHeader,
			CLngRc::getRsrc(lng_AssertIgnoreDescr/*"Press <Cancel> to continue your work"*/), L"\r\n\r\n",
			CLngRc::getRsrc(lng_AssertRetryDescr/*"Press <Retry> to copy text information to clipboard\r\nand report a bug (open project web page)."*/),
			nullptr);

		DWORD nPostCode = static_cast<DWORD>(-1);

		if (bInAssert)
		{
			nPostCode = static_cast<DWORD>(-2);
			nRet = IDCANCEL;
		}
		else
		{
			bInAssert = true;
			nRet = MsgBox(szFull, MB_RETRYCANCEL | MB_ICONSTOP | MB_SYSTEMMODAL | MB_DEFBUTTON2, pszTitle, nullptr);
			bInAssert = false;
			nPostCode = GetLastError();
		}

		swprintf_c(szCodes, L"\r\nPreError=%i, PostError=%i, Result=%i", nPreCode, nPostCode, nRet);
		szFull.Append(szCodes);
	}

	if (nRet == IDRETRY)
	{
		dumpInfo.comment = szFull.ms_Val;

		const bool bProcessed = DumpCurrentProcess(dumpInfo);

		if (!bProcessed)
		{
			// This dump is created improperly, but that is last resort
			CreateDumpForReport(exceptionInfo, dumpInfo);
		}

		if (dumpInfo.fullInfo[0])
		{
			const CEStr fileMsg = dumpInfo.dumpFile[0]
				? CEStr(L"\r\n\r\n" L"Memory dump was saved to\r\n", dumpInfo.dumpFile,
					L"\r\n\r\n" L"Assertion report was copied to clipboard"
					L"\r\n" L"Please Zip it and send to developer (via DropBox etc.)\r\n",
					CEREPORTCRASH /* https://conemu.github.io/en/Issues.html... */)
				: CEStr();
			dumpMessage = CEStr(dumpInfo.fullInfo, fileMsg.c_str());
			const CEStr clipMessage(L"```\r\n", szFull, L"\r\n```\r\n\r\n", dumpMessage);
			CopyToClipboard(
				!clipMessage.IsEmpty() ? clipMessage.c_str() :
				!dumpMessage.IsEmpty() ? dumpMessage.c_str() :
				dumpInfo.fullInfo);
		}
		else if (szFull)
		{
			CopyToClipboard(szFull);
		}

		ConEmuAbout::OnInfo_ReportCrash(!dumpMessage.IsEmpty() ? dumpMessage.c_str() : szFull.c_str(L""));
	}
}


// Clear some rubbish in the environment
void ResetEnvironmentVariables()
{
	SetEnvironmentVariable(ENV_CONEMUFAKEDT_VAR_W, nullptr);
	SetEnvironmentVariable(ENV_CONEMU_HOOKS_W, nullptr);
}

int CheckZoneIdentifiers(bool abAutoUnblock)
{
	if (!gpConEmu)
	{
		_ASSERTE(gpConEmu!=nullptr);
		return 0;
	}

	CEStr szZonedFiles;

	LPCWSTR pszDirs[] = {
		gpConEmu->ms_ConEmuExeDir,
		gpConEmu->ms_ConEmuBaseDir,
		nullptr};
	LPCWSTR pszFiles[] = {
		L"ConEmu.exe", L"ConEmu64.exe",
		ConEmuC_32_EXE, ConEmuC_64_EXE,
		ConEmuCD_32_DLL, ConEmuCD_64_DLL,
		ConEmuHk_32_DLL, ConEmuHk_64_DLL,
		nullptr};

	for (int i = 0; i <= 1; i++)
	{
		if (i && (lstrcmpi(pszDirs[0], pszDirs[1]) == 0))
			break; // ms_ConEmuExeDir & ms_ConEmuBaseDir

		for (int j = 0; pszFiles[j]; j++)
		{
			CEStr lsFile(JoinPath(pszDirs[i], pszFiles[j]));
			int nZone = 0;
			if (HasZoneIdentifier(lsFile, nZone)
				&& (nZone != 0 /*LocalComputer*/))
			{
				szZonedFiles.Append(szZonedFiles.ms_Val ? L"\r\n" : nullptr, lsFile.ms_Val);
			}
		}
	}

	if (!szZonedFiles.ms_Val)
	{
		return 0; // All files are OK
	}

	CEStr lsMsg(
		L"ConEmu binaries were marked as ‘Downloaded from internet’:\r\n",
		szZonedFiles.ms_Val, L"\r\n\r\n"
		L"This may cause blocking or access denied errors!");

	int iBtn = abAutoUnblock ? IDYES
		: ConfirmDialog(lsMsg, L"Warning!", nullptr, nullptr, MB_YESNOCANCEL, ghWnd,
			L"Unblock and Continue", L"Let ConEmu try to unblock these files" L"\r\n" L"You may see SmartScreen and UAC confirmations",
			L"Visit home page and Exit", CEZONEID /* https://conemu.github.io/en/ZoneId.html */,
			L"Ignore and Continue", L"You may face further warnings");

	switch (iBtn)
	{
	case IDNO:
		ConEmuAbout::OnInfo_OnlineWiki(L"ZoneId");
		// Exit
		return -1;
	case IDYES:
		break; // Try to unblock
	default:
		// Ignore and continue;
		return 0;
	}

	DWORD nErrCode;
	LPCWSTR pszFrom = szZonedFiles.ms_Val;
	CEStr lsFile;
	bool bFirstRunAs = true;
	while ((pszFrom = NextLine(pszFrom, lsFile)))
	{
		if (!DropZoneIdentifier(lsFile, nErrCode))
		{
			if ((nErrCode == ERROR_ACCESS_DENIED)
				&& bFirstRunAs
				&& IsWin6() // UAC available?
				&& !IsUserAdmin()
				)
			{
				bFirstRunAs = false;

				// Let's try to rerun as Administrator
				SHELLEXECUTEINFO sei = {sizeof(sei)};
				sei.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
				sei.lpVerb = L"runas";
				sei.lpFile = gpConEmu->ms_ConEmuExe;
				sei.lpParameters = L" -ZoneId -Exit";
				sei.lpDirectory = gpConEmu->ms_ConEmuExeDir;
				sei.nShow = SW_SHOWNORMAL;

				if (ShellExecuteEx(&sei))
				{
					if (!sei.hProcess)
					{
						Sleep(500);
						_ASSERTE(sei.hProcess!=nullptr);
					}
					if (sei.hProcess)
					{
						WaitForSingleObject(sei.hProcess, INFINITE);
					}

					int nZone = 0;
					if (!HasZoneIdentifier(lsFile, nZone)
						|| (nZone != 0 /*LocalComputer*/))
					{
						// Assuming that elevated copy has fixed all zone problems
						break;
					}
				}
			}

			lsMsg = CEStr(L"Failed to drop ZoneId in file:\r\n", lsFile, L"\r\n\r\n" L"Ignore error and continue?" L"\r\n");
			if (DisplayLastError(lsMsg, nErrCode, MB_ICONSTOP|MB_YESNO) != IDYES)
			{
				return -1; // Fails to change
			}
		}
	}

	return 0;
}

// 0 - Succeeded, otherwise - exit code
// isScript - several tabs or splits were requested via "-cmdlist ..."
// isBare - true if there was no switches, for example "ConEmu.exe c:\tools\far.exe". That is not correct command line actually
// ReSharper disable once CppParameterMayBeConst
int ProcessCmdArg(LPCWSTR cmdNew, const bool isScript, const bool isBare, CEStr& szReady, bool& rbSaveHistory)
{
	rbSaveHistory = false;

	if (!cmdNew || !*cmdNew)
	{
		return 0; // Nothing to do
	}

	// Command line was specified by "-cmd ..." or "-cmdlist ..."
	DEBUGSTRSTARTUP(L"Preparing command line");

	MCHKHEAP
	const wchar_t* pszDefCmd = nullptr;

	if (isScript)
	{
		szReady.Set(cmdNew);
		if (szReady.IsEmpty())
		{
			MBoxAssert(FALSE && "Memory allocation failed");
			return 100;
		}
	}
	else
	{
		ssize_t nLen = _tcslen(cmdNew) + 8;

		// For example "ConEmu.exe c:\tools\far.exe"
		// That is not 'proper' command actually, but we may support this by courtesy
		if (isBare /*(params == (uint)-1)*/
			&& (gpSet->nStartType == 0)
			&& (gpSet->psStartSingleApp && *gpSet->psStartSingleApp))
		{
			// psStartSingleApp may have path to "far.exe" defined...
			// Then if user drops, for example, txt file on the ConEmu's icon,
			// we may concatenate this argument with Far command line.
			pszDefCmd = gpSet->psStartSingleApp;
			CmdArg szExe;
			if (!NextArg(pszDefCmd, szExe))
			{
				_ASSERTE(FALSE && "NextArg failed");
			}
			else
			{
				// only for Far Manager exe
				if (IsFarExe(szExe))
					pszDefCmd = gpSet->psStartSingleApp;
				else
					pszDefCmd = nullptr; // Run only the "dropped" command
			}

			if (pszDefCmd)
			{
				nLen += 3 + _tcslen(pszDefCmd);
			}
		}

		wchar_t* pszReady = szReady.GetBuffer(nLen + 1);
		if (!pszReady)
		{
			MBoxAssert(FALSE && "Memory allocation failed");
			return 100;
		}


		if (pszDefCmd)
		{
			lstrcpy(pszReady, pszDefCmd);
			lstrcat(pszReady, L" ");
			lstrcat(pszReady, SkipNonPrintable(cmdNew));

			if (pszReady[0] != L'/' && pszReady[0] != L'-')
			{
				// gpSet->HistoryAdd(pszReady);
				rbSaveHistory = true;
			}
		}
		// There was no switches
		else if (isBare)
		{
			*pszReady = DropLnkPrefix; // The sign we probably got command line by dropping smth on ConEmu's icon
			lstrcpy(pszReady+1, SkipNonPrintable(cmdNew));

			if (pszReady[1] != L'/' && pszReady[1] != L'-')
			{
				// gpSet->HistoryAdd(pszReady+1);
				rbSaveHistory = true;
			}
		}
		else
		{
			lstrcpy(pszReady, SkipNonPrintable(cmdNew));

			if (pszReady[0] != L'/' && pszReady[0] != L'-')
			{
				// gpSet->HistoryAdd(pszReady);
				rbSaveHistory = true;
			}
		}
	}

	MCHKHEAP

	// Store it
	gpConEmu->SetCurCmd(szReady, isScript);

	return 0;
}

// -debug, -debugi, -debugw
int CheckForDebugArgs(LPCWSTR asCmdLine)
{
	if (IsDebuggerPresent())
		return 0;

	BOOL nDbg = FALSE;
	bool debug = false;  // Just show a MessageBox with command line and PID
	bool debugw = false; // Silently wait until debugger is connected
	bool debugi = false; // _ASSERT(FALSE)
	UINT iSleep = 0;

	#if defined(SHOW_STARTED_MSGBOX)
	debug = true;
	#elif defined(WAIT_STARTED_DEBUGGER)
	debugw = true;
	#endif

	LPCWSTR pszCmd = asCmdLine;
	CmdArg lsArg;
	// First argument (actually, first would be our executable in most cases)
	for (int i = 0; i <= 1; i++)
	{
		if (!(pszCmd = NextArg(pszCmd, lsArg)))
			break;
		// Support both notations
		if (lsArg.ms_Val[0] == L'/') lsArg.ms_Val[0] = L'-';

		if (lstrcmpi(lsArg, L"-debug") == 0)
		{
			debug = true; break;
		}
		if (lstrcmpi(lsArg, L"-debugi") == 0)
		{
			debugi = true; break;
		}
		if (lstrcmpi(lsArg, L"-debugw") == 0)
		{
			debugw = true; break;
		}
	}

	if (debug)
	{
		wchar_t szTitle[128]; swprintf_c(szTitle, L"Conemu started, PID=%i", GetCurrentProcessId());
		CEStr lsText(L"GetCommandLineW()\n", GetCommandLineW(), L"\n\n\n" L"lpCmdLine\n", asCmdLine);
		MessageBox(nullptr, lsText, szTitle, MB_OK|MB_ICONINFORMATION|MB_SETFOREGROUND|MB_SYSTEMMODAL);
		nDbg = IsDebuggerPresent();
	}
	else if (debugw)
	{
		while (!IsDebuggerPresent())
			Sleep(250);
		nDbg = IsDebuggerPresent();
	}
	else if (debugi)
	{
		#ifdef _DEBUG
		if (!IsDebuggerPresent()) _ASSERT(FALSE);
		#endif
		nDbg = IsDebuggerPresent();
	}

	// To be able to do some actions (focus window?) before continue
	if (nDbg)
	{
		iSleep = 5000;
		Sleep(iSleep);
	}

	return nDbg;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	DEBUGSTRSTARTUP(L"WinMain entered");
	int iMainRc = 0;

	g_hInstance = hInstance;
	ghWorkingModule = hInstance;

#ifdef _DEBUG
	gbAllowChkHeap = true;
#endif

	if (!IsDebuggerPresent())
	{
		SetUnhandledExceptionFilter(CreateDumpOnException);
	}

	_ASSERTE(sizeof(CESERVER_REQ_STARTSTOPRET) <= sizeof(CESERVER_REQ_STARTSTOP));
	gOSVer = {};
	gOSVer.dwOSVersionInfoSize = sizeof(gOSVer);
	GetOsVersionInformational(&gOSVer);
	gnOsVer = ((gOSVer.dwMajorVersion & 0xFF) << 8) | (gOSVer.dwMinorVersion & 0xFF);
	HeapInitialize();
	AssertMsgBox = MsgBox;
	gfnHooksUnlockerProc = HooksUnlockerProc;
	initMainThread();
	gfnSearchAppPaths = SearchAppPaths;

	srand(GetTickCount() + GetCurrentProcessId());

	#ifdef _DEBUG
	HMODULE hConEmuHk = GetModuleHandle(ConEmuHk_DLL_3264);
	_ASSERTE(hConEmuHk==nullptr && "Hooks must not be loaded into ConEmu[64].exe!");
	#endif

	// On Vista and higher ensure our process will be
	// marked as fully dpi-aware, regardless of manifest
	if (IsWin6())
	{
		CDpiAware::SetProcessDpiAwareness();
	}


	// lpCmdLine is not a UNICODE string, that's why we have to use GetCommandLineW()
	// However, cygwin breaks normal way of creating Windows' processes,
	// and GetCommandLineW will be useless in cygwin's builds (returns only exe full path)
	CEStr lsCvtCmdLine;
	if (lpCmdLine && *lpCmdLine)
	{
		const int iLen = lstrlenA(lpCmdLine);
		MultiByteToWideChar(CP_ACP, 0, lpCmdLine, -1, lsCvtCmdLine.GetBuffer(iLen), iLen+1);
	}
	// Prepared command line
	CEStr lsCommandLine;
	#if !defined(__CYGWIN__)
	lsCommandLine.Set(GetCommandLineW());
	#else
	lsCommandLine.Set(lsCvtCmdLine.ms_Val);
	#endif
	if (lsCommandLine.IsEmpty())
	{
		lsCommandLine.Set(L"");
	}

	// -debug, -debugi, -debugw
	CheckForDebugArgs(lsCommandLine);


	/* *** DEBUG PURPOSES */
	gpStartEnv = LoadStartupEnvEx::Create();
	if (gnOsVer >= 0x600)
	{
		CDpiAware::UpdateStartupInfo(gpStartEnv);
	}
	/* *** DEBUG PURPOSES */

	gbIsWine = IsWine(); // В общем случае, на флажок ориентироваться нельзя. Это для информации.
	if (gbIsWine)
	{
		wcscpy_c(gsDefGuiFont, L"Liberation Mono");
	}
	else if (IsWindowsVista)
	{
		// Vista+ and ClearType? May be "Consolas" font need to be default in Console.
		BOOL bClearType = FALSE;
		if (SystemParametersInfo(SPI_GETCLEARTYPE, 0, &bClearType, 0) && bClearType)
		{
			wcscpy_c(gsDefGuiFont, L"Consolas");
		}
		// Default UI?
		wcscpy_c(gsDefMUIFont, L"Segoe UI");
	}


	gbIsDBCS = IsWinDBCS();
	if (gbIsDBCS)
	{
		HKEY hk = nullptr;
		DWORD nOemCP = GetOEMCP();
		DWORD nRights = KEY_READ|WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0),0);
		if (nOemCP && !RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont", 0, nRights, &hk))
		{
			wchar_t szName[64]; swprintf_c(szName, L"%u", nOemCP);
			wchar_t szVal[64] = {}; DWORD cbSize = sizeof(szVal)-2;
			if (!RegQueryValueEx(hk, szName, nullptr, nullptr, (LPBYTE)szVal, &cbSize) && *szVal)
			{
				if (wcschr(szVal, L'?'))
				{
					// logging was not started yet... so we cant write to the log
					// Just leave default 'Lucida Console'
					// LogString(L"Invalid console font was registered in registry");
				}
				else
				{
					lstrcpyn(gsDefConFont, (*szVal == L'*') ? (szVal+1) : szVal, countof(gsDefConFont));
				}
			}
			RegCloseKey(hk);
		}
	}

	ResetEnvironmentVariables();

	DEBUGSTRSTARTUP(L"Environment checked");

	gpHotKeys = new ConEmuHotKeyList;
	gpFontMgr = new CFontMgr;
	gpSetCls = new CSettings;
	gpConEmu = new CConEmuMain;
	CLngRc::Initialize();
	gVConDcMap.Init(MAX_CONSOLE_COUNT,true);
	gVConBkMap.Init(MAX_CONSOLE_COUNT,true);
	gpLocalSecurity = LocalSecurity();

	#ifdef _DEBUG
	gAllowAssertThread = am_Thread;
	#endif

	RunDebugTests();

	// If possible, open our windows on the monitor where user have clicked
	// our icon (shortcut on the desktop or TaskBar)
	gpStartEnv->hStartMon = GetStartupMonitor();



#ifdef DEBUG_MSG_HOOKS
	ghDbgHook = SetWindowsHookEx(WH_CALLWNDPROC, DbgCallWndProc, nullptr, GetCurrentThreadId());
#endif

	_ASSERTE(gpSetCls->SingleInstanceArg == sgl_Default);
	gpSetCls->SingleInstanceArg = sgl_Default;
	gpSetCls->SingleInstanceShowHide = sih_None;

	CEStr szReady;
	bool  ReqSaveHistory = false;

	//gpConEmu->cBlinkShift = GetCaretBlinkTime()/15;
	//memset(&gOSVer, 0, sizeof(gOSVer));
	//gOSVer.dwOSVersionInfoSize = sizeof(gOSVer);
	//GetVersionEx(&gOSVer);
	//DisableIME();
	//Windows7 - SetParent для консоли валится
	//gpConEmu->setParent = false; // PictureView теперь идет через Wrapper
	//if ((gOSVer.dwMajorVersion>6) || (gOSVer.dwMajorVersion==6 && gOSVer.dwMinorVersion>=1))
	//{
	//	setParentDisabled = true;
	//}
	//if (gOSVer.dwMajorVersion>=6)
	//{
	//	CheckConIme();
	//}
	//gpSet->InitSettings();

	int iParseRc = 0;
	if (!gpConEmu->ParseCommandLine(lsCommandLine, iParseRc))
	{
		return iParseRc;
	}

	/* ******************************** */
	int iZoneCheck = CheckZoneIdentifiers(gpConEmu->opt.FixZoneId.GetBool());
	if (iZoneCheck < 0)
	{
		return CERR_ZONE_CHECK_ERROR;
	}
	if (gpConEmu->opt.FixZoneId.GetBool() && gpConEmu->opt.ExitAfterActionPrm.GetBool())
	{
		_ASSERTE(gpConEmu->opt.runCommand.IsEmpty());
		return 0;
	}
	/* ******************************** */

//------------------------------------------------------------------------
///| load settings and apply parameters |/////////////////////////////////
//------------------------------------------------------------------------

	// set config name before settings (i.e. where to load from)
	if (gpConEmu->opt.ConfigVal.Exists)
	{
		DEBUGSTRSTARTUP(L"Initializing configuration name");
		gpSetCls->SetConfigName(gpConEmu->opt.ConfigVal.GetStr());
	}

	// xml-using disabled? Forced to registry?
	if (gpConEmu->opt.ForceUseRegistryPrm)
	{
		gpConEmu->SetForceUseRegistry();
	}
	// special config file
	else if (gpConEmu->opt.LoadCfgFile.Exists)
	{
		DEBUGSTRSTARTUP(L"Exact cfg file was specified");
		// При ошибке - не выходим, просто покажем ее пользователю
		gpConEmu->SetConfigFile(gpConEmu->opt.LoadCfgFile.GetStr(), false/*abWriteReq*/, true/*abSpecialPath*/);
	}

	//------------------------------------------------------------------------
	///| Set our own AppUserModelID for Win7 TaskBar |////////////////////////
	///| Call it always to store in gpConEmu->AppID  |////////////////////////
	//------------------------------------------------------------------------
	DEBUGSTRSTARTUP(L"UpdateAppUserModelID");
	UpdateAppUserModelID();

	//------------------------------------------------------------------------
	///| Set up ‘First instance’ event |//////////////////////////////////////
	//------------------------------------------------------------------------
	DEBUGSTRSTARTUP(L"Checking for first instance");
	gpConEmu->isFirstInstance();

	//------------------------------------------------------------------------
	///| Preparing settings |/////////////////////////////////////////////////
	//------------------------------------------------------------------------
	bool bNeedCreateVanilla = false;
	SettingsLoadedFlags slfFlags = slf_None;
	if (gpConEmu->opt.ResetSettings)
	{
		// force this config as "new"
		DEBUGSTRSTARTUP(L"Clear config was requested");
		gpSetCls->IsConfigNew = true;
		gpSet->InitVanilla();
	}
	else
	{
		// load settings from registry
		DEBUGSTRSTARTUP(L"Loading config from settings storage");
		gpSet->LoadSettings(bNeedCreateVanilla);
	}

	// Update package was dropped on ConEmu icon?
	// params == (uint)-1, если первый аргумент не начинается с '/'
	if (!gpConEmu->opt.runCommand.IsEmpty() && (gpConEmu->opt.params == -1))
	{
		CmdArg szPath;
		LPCWSTR pszCmdLine = gpConEmu->opt.runCommand;
		if ((pszCmdLine = NextArg(pszCmdLine, szPath)))
		{
			if (CConEmuUpdate::IsUpdatePackage(szPath))
			{
				DEBUGSTRSTARTUP(L"Update package was dropped on ConEmu, updating");

				// Чтобы при запуске НОВОЙ версии опять не пошло обновление - грохнуть ком-строку
				gpConEmu->opt.cmdRunCommand.Clear();

				// Создание скрипта обновления, запуск будет выполнен в деструкторе gpUpd
				CConEmuUpdate::LocalUpdate(szPath);

				// Перейти к завершению процесса и запуску обновления
				goto done;
			}
		}
	}

	// Store command line in our class variables to be able show it in "Fast Configuration" dialog
	if (gpConEmu->opt.runCommand)
	{
		int iArgRc = ProcessCmdArg(gpConEmu->opt.runCommand, gpConEmu->opt.isScript, (gpConEmu->opt.params == -1), szReady, ReqSaveHistory);
		if (iArgRc != 0)
		{
			return iArgRc;
		}
	}

	// Settings are loaded, fixup
	slfFlags |= slf_OnStartupLoad | slf_AllowFastConfig
		| (bNeedCreateVanilla ? slf_NeedCreateVanilla : slf_None)
		| (gpSetCls->IsConfigPartial ? slf_DefaultTasks : slf_None)
		| ((gpConEmu->opt.ResetSettings || gpSetCls->IsConfigNew) ? slf_DefaultSettings : slf_None);
	// выполнить дополнительные действия в классе настроек здесь
	DEBUGSTRSTARTUP(L"Config loaded, post checks");
	gpSetCls->SettingsLoaded(slfFlags, gpConEmu->opt.runCommand);

	// Для gpSet->isQuakeStyle - принудительно включается gpSetCls->SingleInstanceArg

	// When "/Palette <name>" is specified
	if (gpConEmu->opt.PaletteVal.Exists)
	{
		gpSet->PaletteSetActive(gpConEmu->opt.PaletteVal.GetStr());
	}

	// Set another update location (-UpdateSrcSet <URL>)
	if (gpConEmu->opt.UpdateSrcSet.Exists)
	{
		gpSet->UpdSet.SetUpdateVerLocation(gpConEmu->opt.UpdateSrcSet.GetStr());
	}

	// Force "AnsiLog" feature
	if (gpConEmu->opt.AnsiLogPath.GetStr())
	{
		gpSet->isAnsiLog = true;
		gpSet->isAnsiLogCodes = true;
		SafeFree(gpSet->pszAnsiLog);
		gpSet->pszAnsiLog = lstrdup(gpConEmu->opt.AnsiLogPath.GetStr()).Detach();
	}

	DEBUGSTRSTARTUPLOG(L"SettingsLoaded");


//------------------------------------------------------------------------
///| Processing actions |/////////////////////////////////////////////////
//------------------------------------------------------------------------
	// gpConEmu->mb_UpdateJumpListOnStartup - Обновить JumpLists
	// SaveCfgFilePrm, SaveCfgFile - сохранить настройки в xml-файл (можно использовать вместе с ResetSettings)
	// ExitAfterActionPrm - сразу выйти после выполнения действий
	// не наколоться бы с isAutoSaveSizePos

	if (gpConEmu->mb_UpdateJumpListOnStartup && gpConEmu->opt.ExitAfterActionPrm)
	{
		DEBUGSTRSTARTUP(L"Updating Win7 task list");
		OleInitializer ole(true); // gh-1478
		if (!UpdateWin7TaskList(true/*bForce*/, true/*bNoSuccMsg*/))
		{
			if (!iMainRc) iMainRc = 10;
		}
	}

	// special config file
	if (gpConEmu->opt.SaveCfgFile.Exists)
	{
		// Сохранять конфиг только если получилось сменить путь (создать файл)
		DEBUGSTRSTARTUP(L"Force write current config to settings storage");
		if (!gpConEmu->SetConfigFile(gpConEmu->opt.SaveCfgFile.GetStr(), true/*abWriteReq*/, true/*abSpecialPath*/))
		{
			if (!iMainRc) iMainRc = 11;
		}
		else
		{
			if (!gpSet->SaveSettings())
			{
				if (!iMainRc) iMainRc = 12;
			}
		}
	}

	// Only when ExitAfterActionPrm, otherwise - it will be called from ConEmu's PostCreate
	if (gpConEmu->opt.SetUpDefaultTerminal)
	{
		_ASSERTE(!gpConEmu->DisableSetDefTerm);

		gpSet->isSetDefaultTerminal = true;
		gpSet->isRegisterOnOsStartup = true;

		if (gpConEmu->opt.ExitAfterActionPrm)
		{
			if (gpConEmu->mp_DefTrm)
			{
				// Update registry with ‘DefTerm-...’ settings
				gpConEmu->mp_DefTrm->ApplyAndSave(true, true);
				// Hook all required processes and exit
				gpConEmu->mp_DefTrm->StartGuiDefTerm(true, true);
			}
			else
			{
				_ASSERTE(gpConEmu->mp_DefTrm);
			}

			// Exit now
			gpSetCls->ibDisableSaveSettingsOnExit = true;
		}
	}

	// Actions done
	if (gpConEmu->opt.ExitAfterActionPrm)
	{
		DEBUGSTRSTARTUP(L"Exit was requested");
		goto wrap;
	}

	if (gpConEmu->opt.ExecGuiMacro.GetStr())
	{
		gpConEmu->SetPostGuiMacro(gpConEmu->opt.ExecGuiMacro.GetStr());
	}

//------------------------------------------------------------------------
///| Continue normal work mode  |/////////////////////////////////////////
//------------------------------------------------------------------------

	// Если в режиме "Inside" подходящего окна не нашли и юзер отказался от "обычного" режима
	// mh_InsideParentWND инициализируется вызовом InsideFindParent из Settings::LoadSettings()
	if (gpConEmu->mp_Inside && (gpConEmu->mp_Inside->GetParentWnd() == INSIDE_PARENT_NOT_FOUND))
	{
		DEBUGSTRSTARTUP(L"Bad InsideParentHWND, exiting");
		return 100;
	}


	// Проверить наличие необходимых файлов (перенес сверху, чтобы учитывался флажок "Inject ConEmuHk")
	if (!gpConEmu->CheckRequiredFiles())
	{
		DEBUGSTRSTARTUP(L"Required files were not found, exiting");
		return 100;
	}


	//#pragma message("Win2k: CLEARTYPE_NATURAL_QUALITY")
	//if (ClearTypePrm)
	//	gpSet->LogFont.lfQuality = CLEARTYPE_NATURAL_QUALITY;
	//if (FontPrm)
	//	_tcscpy(gpSet->LogFont.lfFaceName, FontVal);
	//if (SizePrm)
	//	gpSet->LogFont.lfHeight = SizeVal;
	if (gpConEmu->opt.BufferHeightVal.Exists)
	{
		gpSetCls->SetArgBufferHeight(gpConEmu->opt.BufferHeightVal.GetInt());
	}

	if (!gpConEmu->opt.WindowModeVal.Exists)
	{
		if (nCmdShow == SW_SHOWMAXIMIZED)
			gpSet->_WindowMode = wmMaximized;
		else if (nCmdShow == SW_SHOWMINIMIZED || nCmdShow == SW_SHOWMINNOACTIVE)
			gpConEmu->WindowStartMinimized = true;
	}
	else
	{
		gpSet->_WindowMode = (ConEmuWindowMode)gpConEmu->opt.WindowModeVal.GetInt();
	}

	if (gpConEmu->opt.MultiConValue.Exists)
		gpSet->mb_isMulti = gpConEmu->opt.MultiConValue;

	if (gpConEmu->opt.VisValue.Exists)
		gpSet->isConVisible = gpConEmu->opt.VisValue;

	// Если запускается conman (нафига?) - принудительно включить флажок "Обновлять handle"
	//TODO("Deprecated: isUpdConHandle использоваться не должен");

	if (gpSetCls->IsMulti() || StrStrI(gpConEmu->GetCmd(), L"conman.exe"))
	{
		//gpSet->isUpdConHandle = TRUE;

		// сбросить CreateInNewEnvironment для ConMan
		gpConEmu->ResetConman();
	}

	// Need to add to the history?
	if (ReqSaveHistory && !szReady.IsEmpty())
	{
		LPCWSTR pszCommand = szReady;
		if (*pszCommand == DropLnkPrefix)
			pszCommand++;
		gpSet->HistoryAdd(pszCommand);
	}

	//if (FontFilePrm) {
	//	if (!AddFontResourceEx(FontFile, FR_PRIVATE, nullptr)) //ADD fontname; by Mors
	//	{
	//		TCHAR* psz=(TCHAR*)calloc(_tcslen(FontFile)+100,sizeof(TCHAR));
	//		lstrcpyW(psz, L"Can't register font:\n");
	//		lstrcatW(psz, FontFile);
	//		MessageBox(nullptr, psz, gpConEmu->GetDefaultTitle(), MB_OK|MB_ICONSTOP);
	//		free(psz);
	//		return 100;
	//	}
	//	lstrcpynW(gpSet->FontFile, FontFile, countof(gpSet->FontFile));
	//}
	// else if (gpSet->isSearchForFont && gpConEmu->ms_ConEmuExe[0]) {
	//	if (FindFontInFolder(szTempFontFam)) {
	//		// Шрифт уже зарегистрирован
	//		FontFilePrm = true;
	//		FontPrm = true;
	//		FontVal = szTempFontFam;
	//		FontFile = gpSet->FontFile;
	//	}
	//}

	gpConEmu->ReloadMonitorInfo();
	if (gpStartEnv->hStartMon)
		gpConEmu->SetRequestedMonitor(gpStartEnv->hStartMon);


	// Forced window size or pos
	// Call this AFTER SettingsLoaded because we (may be)
	// don't want to change ‘xml-stored’ values
	if (gpConEmu->opt.SizePosPrm)
	{
		if (gpConEmu->opt.sWndX.Exists)
			gpConEmu->SetWindowPosSizeParam(L'X', gpConEmu->opt.sWndX.GetStr());
		if (gpConEmu->opt.sWndY.Exists)
			gpConEmu->SetWindowPosSizeParam(L'Y', gpConEmu->opt.sWndY.GetStr());
		if (gpConEmu->opt.sWndW.Exists)
			gpConEmu->SetWindowPosSizeParam(L'W', gpConEmu->opt.sWndW.GetStr());
		if (gpConEmu->opt.sWndH.Exists)
			gpConEmu->SetWindowPosSizeParam(L'H', gpConEmu->opt.sWndH.GetStr());
	}

	// Quake/NoQuake?
	if (gpConEmu->opt.QuakeMode.Exists)
	{
		gpConEmu->SetQuakeMode(gpConEmu->opt.QuakeMode.GetInt());
		gpSet->isRestore2ActiveMon = true;
	}

	if (gpSet->wndCascade)
	{
		// #SIZE_TODO the CSettings::GetOverallDpi() must be already called
		gpConEmu->CascadedPosFix();
	}

///////////////////////////////////

	// Нет смысла проверять и искать, если наш экземпляр - первый.
	if (gpSetCls->IsSingleInstanceArg() && !gpConEmu->isFirstInstance())
	{
		DEBUGSTRSTARTUPLOG(L"Checking for existing instance");

		HWND hConEmuHwnd = FindWindowExW(nullptr, nullptr, VirtualConsoleClassMain, nullptr);
		// При запуске серии закладок из cmd файла второму экземпляру лучше чуть-чуть подождать
		// чтобы успело "появиться" главное окно ConEmu
		if ((hConEmuHwnd == nullptr) && (gpSetCls->SingleInstanceShowHide == sih_None))
		{
			// Если окна нет, и других процессов (ConEmu.exe, ConEmu64.exe) нет
			// то ждать смысла нет
			bool bOtherExists = false;

			gpConEmu->LogString(L"TH32CS_SNAPPROCESS");

			HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
			if (h && (h != INVALID_HANDLE_VALUE))
			{
				PROCESSENTRY32 PI = {sizeof(PI)};
				DWORD nSelfPID = GetCurrentProcessId();
				if (Process32First(h, &PI))
				{
					do {
						if (PI.th32ProcessID != nSelfPID)
						{
							LPCWSTR pszName = PointToName(PI.szExeFile);
							if (pszName
								&& ((lstrcmpi(pszName, L"ConEmu.exe") == 0)
									|| (lstrcmpi(pszName, L"ConEmu64.exe") == 0)))
							{
								bOtherExists = true;
								break;
							}
						}
					} while (Process32Next(h, &PI));
				}

				CloseHandle(h);
			}

			// Ждать имеет смысл только если есть другие процессы "ConEmu.exe"/"ConEmu64.exe"
			if (bOtherExists)
			{
				Sleep(1000); // чтобы успело "появиться" главное окно ConEmu
			}
		}

		gpConEmu->LogString(L"isFirstInstance");

		// Поехали
		DWORD dwStart = GetTickCount();

		while (!gpConEmu->isFirstInstance())
		{
			DEBUGSTRSTARTUPLOG(L"Waiting for RunSingleInstance");

			int iRunRC = gpConEmu->RunSingleInstance();
			if (iRunRC > 0)
			{
				gpConEmu->LogString(L"Passed to first instance, exiting");
				return 0;
			}
			else if (iRunRC < 0)
			{
				gpConEmu->LogString(L"Reusing running instance is not allowed here");
				break;
			}

			// Если передать не удалось (может первый экземпляр еще в процессе инициализации?)
			Sleep(250);

			// Если ожидание длится более 10 секунд - запускаемся самостоятельно
			if ((GetTickCount() - dwStart) > 10*1000)
				break;
		}

		DEBUGSTRSTARTUPLOG(L"Existing instance was terminated, continue as first instance");
	}

//------------------------------------------------------------------------
///| Allocating console |/////////////////////////////////////////////////
//------------------------------------------------------------------------

#if 0
	//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
	if (AttachPrm)
	{
		if (!AttachVal)
		{
			MBoxA(_T("Invalid <process id> specified in the /Attach argument"));
			//delete pVCon;
			return 100;
		}

		gpSetCls->nAttachPID = AttachVal;
	}
#endif

//------------------------------------------------------------------------
///| Initializing |///////////////////////////////////////////////////////
//------------------------------------------------------------------------

	DEBUGSTRSTARTUPLOG(L"gpConEmu->Init");

	// Тут загружаются иконки, Affinity, SetCurrentDirectory и т.п.
	if (!gpConEmu->Init())
	{
		return 100;
	}

//------------------------------------------------------------------------
///| Create taskbar window |//////////////////////////////////////////////
//------------------------------------------------------------------------

	// Тут создается окошко чтобы не показывать кнопку на таскбаре
	if (!CheckCreateAppWindow())
	{
		return 100;
	}

//------------------------------------------------------------------------
///| Creating window |////////////////////////////////////////////////////
//------------------------------------------------------------------------

	DEBUGSTRSTARTUPLOG(L"gpConEmu->CreateMainWindow");

	if (!gpConEmu->CreateMainWindow())
	{
		return 100;
	}

//------------------------------------------------------------------------
///| Misc |///////////////////////////////////////////////////////////////
//------------------------------------------------------------------------
	DEBUGSTRSTARTUP(L"gpConEmu->PostCreate");
	gpConEmu->PostCreate();
//------------------------------------------------------------------------
///| Main message loop |//////////////////////////////////////////////////
//------------------------------------------------------------------------
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
	MessageLoop();

	iMainRc = gpConEmu->mn_ShellExitCode;
	switch (iMainRc)
	{
	case STATUS_CONTROL_C_EXIT:
		DEBUGSTRSTARTUP(L"Shell has reported close by Ctrl+C. ConEmu is going to exit with code 0xC000013A."); break;
	case STILL_ACTIVE:
		DEBUGSTRSTARTUP(L"Shell has not reported its exit code. Abnormal termination or detached window? ConEmu is going to exit with code 259."); break;
	case 0:
		break;
	default:
		DEBUGSTRSTARTUP(L"ConEmu is going to exit with shell's exit code.");
	}

done:
	DEBUGSTRSTARTUP(L"Terminating");
	ShutdownGuiStep(L"MessageLoop terminated");
//------------------------------------------------------------------------
///| Deinitialization |///////////////////////////////////////////////////
//------------------------------------------------------------------------
	//KillTimer(ghWnd, 0);
	//delete pVCon;
	//CloseHandle(hChildProcess); -- он более не требуется
	//if (FontFilePrm) RemoveFontResourceEx(FontFile, FR_PRIVATE, nullptr); //ADD fontname; by Mors
	gpFontMgr->UnregisterFonts();

	//CoUninitialize();
	OleUninitialize();

	SafeDelete(gpConEmu);
	SafeDelete(gpSetCls);
	SafeDelete(gpFontMgr);
	SafeDelete(gpHotKeys);
	SafeDelete(gpUpd);
	SafeDelete(gpLng);

	ShutdownGuiStep(L"Gui terminated");

wrap:
	HeapDeinitialize();
	DEBUGSTRSTARTUP(L"WinMain exit");
	// If TerminateThread was called at least once,
	// normal process shutdown may hang
	if (wasTerminateThreadCalled())
	{
		TerminateProcess(GetCurrentProcess(), iMainRc);
	}
	return iMainRc;
}
