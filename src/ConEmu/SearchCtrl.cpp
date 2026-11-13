
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

#include "Header.h"
#include "DarkMode.h"
#include "DwmHelper.h"
#include "resource.h"
#include "ToolImg.h"
#include "../common/MMap.h"
#include "../common/MSetter.h"

#define CE_ICON_SPACING 6
#define CE_ICON_YPAD 2
#define SEARCH_CTRL_TIMERID 1001
#define SEARCH_CTRL_TIMER   1500
#define SEARCH_CTRL_REFRID  1002
#define SEARCH_CTRL_REFR    250

struct CEIconDrawHandles
{
	CToolImg* pIcon;
	HFONT     hFont;

	// !!! Do NOT add destructor here to free objects/handles !!!
	// !!! These life-time objects are returned to callers !!!
};

struct CEIconHintInfo
{
	HWND     hEditCtrl;
	HWND     hRootDlg;
	WNDPROC  lpPrevWndFunc;
	int      iRes; // Index for gpIconHandles
	BOOL     bSearchIcon;
	DWORD    nMargins;
	wchar_t  sHint[80];
	COLORREF clrHint;
	DWORD    nSearchMsg;
	DWORD    nDefBtnID;
};

struct CEIconHintOthers
{
	HWND     hCtrl;
	HWND     hRootDlg;
	WNDPROC  lpPrevWndFunc;
};

static MMap<HWND,CEIconHintInfo>* gpIconHints = nullptr;
static MMap<HWND,CEIconHintOthers>* gpIconOthers = nullptr;
// Map the font height (actually CEIconHintInfo.iRes) to hFont and pIcon
static MMap<int,CEIconDrawHandles>* gpIconHandles = nullptr;

// Returns the index (new or existing) in the gpIconHandles
static int EditIconHintCreateHandles(HWND hEditCtrl, CToolImg** pIcon = nullptr, HFONT* phFont = nullptr)
{
	if (pIcon)
		*pIcon = nullptr;
	if (phFont)
		*phFont = nullptr;

	if (!gpIconHandles)
	{
		_ASSERTE(gpIconHandles!=nullptr && "Must be already initialized!");
		return 0;
	}

	// Current control font
	HFONT hf = (HFONT)SendMessage(hEditCtrl, WM_GETFONT, 0, 0);
	LOGFONT lf = {};
	RECT rcClient = {}; GetClientRect(hEditCtrl, &rcClient);
	//CreateFont(nFontHeight, 0, 0, 0, FW_NORMAL, TRUE/*Italic*/, FALSE, 0,
	//           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS,
	//           L"Tahoma");
	if ((hf == nullptr)
		|| (GetObject(hf, sizeof(lf), &lf) <= 0))
	{
		ZeroStruct(lf);
		lf.lfHeight = rcClient.bottom - 4;
		lf.lfCharSet = DEFAULT_CHARSET;
		lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	}
	// And change other properties to our font style
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = FW_NORMAL;
	lf.lfItalic = TRUE;
	lf.lfUnderline = 0;
	lf.lfStrikeOut = 0;
	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	lstrcpyn(lf.lfFaceName, L"Tahoma", countof(lf.lfFaceName));

	// Check first, if there is already created handles for that resolution?
	CEIconDrawHandles dh = {};
	if (!gpIconHandles->Get(lf.lfHeight, &dh))
	{
		// Need to be created
		dh.pIcon = CToolImg::CreateSearchButton();

		dh.hFont = CreateFontIndirect(&lf);
		if (!dh.hFont)
		{
			_ASSERTE(dh.hFont!=nullptr);
			return 0;
		}

		gpIconHandles->Set(lf.lfHeight, dh);
	}

	if (pIcon)
		*pIcon = dh.pIcon;
	if (phFont)
		*phFont = dh.hFont;

	return lf.lfHeight;
}

// Returns handles from gpIconHandles
static bool EditIconHintGetHandles(int iRes, CEIconDrawHandles& dh)
{
	ZeroStruct(dh);

	if (!gpIconHandles)
	{
		_ASSERTE(gpIconHandles!=nullptr && "Must be already initialized!");
		return false;
	}

	if (iRes && gpIconHandles->Get(iRes, &dh))
	{
		return true;
	}

	return false;
}

static LRESULT EditIconHintPaint(HWND hEditCtrl, CEIconHintInfo* p, UINT Msg = WM_PAINT)
{
	LRESULT lRc = 0;

	_ASSERTE(p!=nullptr);

	CEIconDrawHandles dh = {};
	gpIconHandles->Get(p->iRes, &dh);

	int iLen = GetWindowTextLength(hEditCtrl);
	PAINTSTRUCT ps = {};
	HDC hdc; bool bReleaseDC = false;
	bool bDrawHint = ((*p->sHint) && dh.hFont && (iLen == 0) && (GetFocus() != hEditCtrl));

	if (!bDrawHint || !dh.pIcon || (Msg != WM_PAINT))
	{
		if (Msg == WM_PAINT)
		{
			if (p->lpPrevWndFunc)
				lRc = CallWindowProc(p->lpPrevWndFunc, hEditCtrl, WM_PAINT, 0, 0);
			else
				lRc = DefWindowProc(hEditCtrl, WM_PAINT, 0, 0);
		}
		// Need to draw icon?
		if (!dh.pIcon)
			goto wrap;
		// If drawing after standard procedure
		hdc = GetDC(hEditCtrl);
		bReleaseDC = true;
	}
	else
	{
		hdc = BeginPaint(hEditCtrl, &ps);
	}

	if (hdc)
	{
		RECT rcClient = {};
		if (GetClientRect(hEditCtrl, &rcClient))
		{
			RECT rect = rcClient;
			int ih = rect.bottom+1-CE_ICON_YPAD;
			int iw = ih;

			// Paint background
			// Match the real edit control, including the parent's dark colors.
			HBRUSH hBr = reinterpret_cast<HBRUSH>(SendMessage(GetParent(hEditCtrl),
				WM_CTLCOLOREDIT, reinterpret_cast<WPARAM>(hdc), reinterpret_cast<LPARAM>(hEditCtrl)));
			if (!hBr) hBr = GetSysColorBrush(COLOR_WINDOW);
			const bool darkEdit = global::g_darkModeEnabled && GetBkColor(hdc) == DarkMode::Control;
			RECT rcFill = rect;
			// Draw only icon?
			if (!bDrawHint)
			{
				rcFill.right -= HIWORD(p->nMargins);
				rcFill.left = rcFill.right - (iw + CE_ICON_SPACING);
			}
			FillRect(hdc, &rcFill, hBr);
			SetBkMode(hdc, TRANSPARENT);

			// Cut initial margins
			rect.left  += LOWORD(p->nMargins);
			rect.right -= HIWORD(p->nMargins);

			// Draw "Search" icon
			if (p->bSearchIcon && dh.pIcon)
			{
				dh.pIcon->GetSizeForHeight(ih, iw, ih);
				int X1 = rcClient.right - iw - 1;
				int Y1 = std::max<int>(0,((rcClient.bottom-rcClient.top-ih)/2));

				if (darkEdit)
				{
					HPEN pen = CreatePen(PS_SOLID, std::max(1, ih / 12), DarkMode::Hint);
					HGDIOBJ oldPen = SelectObject(hdc, pen);
					HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
					const int diameter = ih * 2 / 3;
					Ellipse(hdc, X1 + iw - diameter - 1, Y1 + 1, X1 + iw - 1, Y1 + diameter + 1);
					MoveToEx(hdc, X1 + iw - diameter + 1, Y1 + diameter - 1, nullptr);
					LineTo(hdc, X1 + 1, Y1 + ih - 1);
					SelectObject(hdc, oldBrush); SelectObject(hdc, oldPen); DeleteObject(pen);
				}
				else
					dh.pIcon->PaintButton(-1, hdc, X1, Y1, iw, ih);

				rect.right = X1 - CE_ICON_SPACING;
			}

			// Draw "Hint"
			if (bDrawHint)
			{
				rect.bottom--;
				HFONT hOldFont = (HFONT)SelectObject(hdc, dh.hFont);
				COLORREF OldColor = GetTextColor(hdc);
				SetTextColor(hdc, darkEdit ? DarkMode::Hint : p->clrHint);
				const int oldBkMode = SetBkMode(hdc, TRANSPARENT);
				DrawText(hdc, p->sHint, -1, &rect, DT_LEFT|DT_SINGLELINE|DT_EDITCONTROL|DT_VCENTER);
				SetBkMode(hdc, oldBkMode);
				SetTextColor(hdc, OldColor);
				SelectObject(hdc, hOldFont);
			}
		}

		// Finalize DC
		if (bReleaseDC)
			ReleaseDC(hEditCtrl, hdc);
		else
			EndPaint(hEditCtrl, &ps);
	}

wrap:
	return lRc;
}

static bool EditIconHintOverIcon(HWND hEditCtrl, CEIconHintInfo* p, LPARAM* lpParam)
{
	RECT rcClient = {}; GetClientRect(hEditCtrl, &rcClient);
	rcClient.right -= HIWORD(p->nMargins);
	rcClient.left = rcClient.right - GetSystemMetrics(SM_CXSMICON);

	POINT ptCur = {};
	if (lpParam)
	{
		ptCur = MakePoint((short)LOWORD(*lpParam),(short)HIWORD(*lpParam));
	}
	else
	{
		GetCursorPos(&ptCur);
		MapWindowPoints(nullptr, hEditCtrl, &ptCur, 1);
	}

	bool bOverIcon = (PtInRect(&rcClient, ptCur) != 0);
	return bOverIcon;
}

// Find child with VScroll in hwnd
static BOOL CALLBACK EditIconHint_FindScrollCtrl(HWND hwnd, LPARAM lParam)
{
	if (!IsWindowVisible(hwnd))
		return TRUE;
	DWORD_PTR dwStyles = GetWindowLongPtr(hwnd, GWL_STYLE);
	if (!(dwStyles & WS_VSCROLL))
		return TRUE;

	RECT rcItem = {}; GetWindowRect(hwnd, &rcItem);
	POINT ptCur = {}; GetCursorPos(&ptCur);
	if (!PtInRect(&rcItem, ptCur))
		return TRUE;

	*((HWND*)lParam) = hwnd;

	wchar_t szClass[64] = L""; GetClassName(hwnd, szClass, countof(szClass));
	if (lstrcmp(szClass, L"#32770") != 0)
		return FALSE; // Нашли

	return TRUE;
}

static LRESULT WINAPI EditIconHintProc(HWND hEditCtrl, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;
	CEIconHintInfo hi = {};
	CEIconDrawHandles dh = {};
	bool bKnown = gpIconHints->Get(hEditCtrl, &hi, (Msg == WM_DESTROY));

	switch (Msg)
	{
	case WM_PAINT:
		lRc = EditIconHintPaint(hEditCtrl, &hi);
		goto wrap;
	case WM_ERASEBKGND:
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_SETCURSOR:
		if (gpIconHandles->Get(hi.iRes, &dh) && dh.pIcon)
		{
			// Cursor over icon?
			if (EditIconHintOverIcon(hEditCtrl, &hi, (Msg==WM_SETCURSOR)?nullptr:&lParam))
			{
				switch (Msg)
				{
				case WM_SETCURSOR:
					SetCursor(LoadCursor(nullptr, IDC_ARROW));
					lRc = TRUE;
					break;
				default:
					if ((Msg == WM_LBUTTONDOWN || Msg == WM_LBUTTONDBLCLK)
						&& (GetWindowTextLength(hEditCtrl) > 0))
					{
						SendMessage(GetParent(hEditCtrl), hi.nSearchMsg, GetWindowLongPtr(hEditCtrl, GWLP_ID), reinterpret_cast<LPARAM>(hEditCtrl));
					}
				}
				goto wrap;
			}
			else if (Msg != WM_SETCURSOR)
			{
				// Redraw search icon
				SetTimer(hEditCtrl, SEARCH_CTRL_REFRID, SEARCH_CTRL_REFR, nullptr);
			}
		}
		break;

	case WM_MOUSEWHEEL:
		// If control under mouse cursor has VScroll - forward message to it
		{
			// Add protection
			static LONG lInCall = 0;
			if (lInCall > 0)
				goto wrap;
			MSetter lSet(&lInCall);
			// Find control under mouse cursor
			POINT ptCur = {}; GetCursorPos(&ptCur);
			HWND hPrevParent = nullptr;
			HWND hParent = hi.hRootDlg;
			if (hParent)
			{
				MapWindowPoints(hPrevParent, hParent, &ptCur, 1);
				HWND hCtrlOver = ChildWindowFromPointEx(hParent, ptCur, CWP_SKIPDISABLED|CWP_SKIPINVISIBLE|CWP_SKIPTRANSPARENT);
				if (!hCtrlOver || (hParent == hCtrlOver))
					break;
				wchar_t szClass[64] = L""; GetClassName(hCtrlOver, szClass, countof(szClass));
				// Child dialog?
				if (lstrcmp(szClass, L"#32770") == 0)
				{
					// Find first control with VScroll
					HWND hSubItem = nullptr;
					EnumChildWindows(hCtrlOver, EditIconHint_FindScrollCtrl, (LPARAM)&hSubItem);
					hCtrlOver = hSubItem;
				}
				if (!hCtrlOver || (hCtrlOver == hEditCtrl))
					break;
				// Informational
				GetClassName(hCtrlOver, szClass, countof(szClass));
				// Must have VScroll
				DWORD_PTR dwStyles = GetWindowLongPtr(hCtrlOver, GWL_STYLE);
				if (dwStyles & WS_VSCROLL)
				{
					// Forward message to proper control
					SendMessage(hCtrlOver, Msg, wParam, lParam);
					// Done, don't pass WM_MOUSEWHEEL to DefWndProc
					goto wrap;
				}
			}
		}
		break;

	case WM_TIMER:
		switch ((DWORD)wParam)
		{
		case SEARCH_CTRL_TIMERID:
			{
				int iLen = GetWindowTextLength(hEditCtrl);
				KillTimer(hEditCtrl, SEARCH_CTRL_TIMERID);
				if ((iLen > 0) || (!hi.bSearchIcon))
				{
					SendMessage(GetParent(hEditCtrl), hi.nSearchMsg, GetWindowLongPtr(hEditCtrl, GWLP_ID), reinterpret_cast<LPARAM>(hEditCtrl));
				}
				goto wrap;
			} break;

		case SEARCH_CTRL_REFRID:
			{
				KillTimer(hEditCtrl, SEARCH_CTRL_REFRID);
				EditIconHintPaint(hEditCtrl, &hi, 0);
				goto wrap;
			} break;
		}
		break; // WM_TIMER

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		if (hi.nDefBtnID)
		{
			DWORD dwStyle;
			HWND hDefBtn = GetDlgItem(hi.hRootDlg, hi.nDefBtnID);
			if (Msg == WM_SETFOCUS)
			{
				dwStyle = GetWindowLong(hDefBtn, GWL_STYLE);
				SetWindowLong(hDefBtn, GWL_STYLE, dwStyle & ~BS_DEFPUSHBUTTON);
			}
			else
			{
				dwStyle = GetWindowLong(hDefBtn, GWL_STYLE);
				SetWindowLong(hDefBtn, GWL_STYLE, dwStyle | BS_DEFPUSHBUTTON);
			}
			::InvalidateRect(hDefBtn, nullptr, FALSE);
		}
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_CHAR:
		if (wParam == VK_RETURN)
		{
			if (Msg == WM_KEYUP)
			{
				PostMessage(hEditCtrl, WM_TIMER, SEARCH_CTRL_TIMERID, 0);
			}
			goto wrap;
		}
		//else if (hi.lpPrevWndFunc)
		//{
		//	// Try to fix ugly icon flickering?
		//	lRc = CallWindowProc(hi.lpPrevWndFunc, hEditCtrl, Msg, wParam, lParam);
		//	KillTimer(hEditCtrl, SEARCH_CTRL_REFRID);
		//	EditIconHintPaint(hEditCtrl, &hi, 0);
		//	goto wrap;
		//}
		break;
	}

	// Not handled, default window procedure
	if (hi.lpPrevWndFunc)
		lRc = CallWindowProc(hi.lpPrevWndFunc, hEditCtrl, Msg, wParam, lParam);
	else
		lRc = DefWindowProc(hEditCtrl, Msg, wParam, lParam);
wrap:
	// Done
	return lRc;
}

// Windows 8.1
// when the dialog is dragged to the another monitor with different dpi value
void EditIconHint_ResChanged(HWND hEditCtrl)
{
	if (!gpIconHints || !gpIconHandles)
		return;
	CEIconHintInfo hi = {};
	if (!gpIconHints->Get(hEditCtrl, &hi))
		return;
	// Refresh hFont and hIcon
	hi.iRes = EditIconHintCreateHandles(hEditCtrl);
	gpIconHints->Set(hEditCtrl, hi);
}

void EditIconHint_SetTimer(HWND hEditCtrl)
{
	SetTimer(hEditCtrl, SEARCH_CTRL_TIMERID, SEARCH_CTRL_TIMER, 0);
}

// That helper function called from root dialog
bool EditIconHint_Process(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam, INT_PTR& lResult)
{
	bool bProcessed = false;

	switch (messg)
	{
	case WM_COMMAND:
		{
			WORD wCode = HIWORD(wParam);
			WORD wID = LOWORD(wParam);

			switch (wCode)
			{
			case BN_CLICKED:
				{
					HWND hEdit = GetFocus();
					CEIconHintInfo hi = {};
					if (!gpIconHints->Get(hEdit, &hi))
						break;

					KillTimer(hEdit, SEARCH_CTRL_TIMERID);

					switch (wID)
					{
					case IDOK:
						//It will be processed in WM_KEYUP
						//PostMessage(hEdit, WM_TIMER, SEARCH_CTRL_TIMERID, 0);
						break;
					case IDCANCEL:
						if (GetWindowTextLength(hEdit) > 0)
							SetWindowText(hEdit, L"");
						else
							SetFocus(GetNextDlgTabItem(hDlg, hEdit, TRUE));
						break;
					}

					lResult = 0;
					bProcessed = true;
				}
				// BN_CLICKED
				break;

			case EN_CHANGE:
			case EN_UPDATE:
				{
					HWND hEdit = GetDlgItem(hDlg, wID);

					CEIconHintInfo hi = {};
					if (!gpIconHints->Get(hEdit, &hi))
						break;

					switch (wCode)
					{
					case EN_UPDATE:
						// Redraw search icon
						SetTimer(hEdit, SEARCH_CTRL_REFRID, SEARCH_CTRL_REFR, nullptr);
						break;
					case EN_CHANGE:
						// Start search delay on typing
						if (GetWindowTextLength(hEdit) > 0)
							EditIconHint_SetTimer(hEdit);
						else
							KillTimer(hEdit, SEARCH_CTRL_TIMERID);
						break;
					}

					lResult = 0;
					bProcessed = true;
					break;
				} // EN_CHANGE, EN_UPDATE

			case EN_SETFOCUS:
				{
					// Autosel text in search control
					HWND hEdit = (HWND)lParam;

					CEIconHintInfo hi = {};
					if (!gpIconHints->Get(hEdit, &hi))
						break;

					SendMessage(hEdit, EM_SETSEL, 0, -1);

					lResult = 0;
					bProcessed = true;
					break;
				} // EN_SETFOCUS

			} // switch (wCode) in WM_COMMAND
		} break; // WM_COMMAND

	case UM_SEARCH_FOCUS:
	case UM_FILTER_FOCUS:
		{
			CEIconHintInfo hi = {};
			HWND h = nullptr;
			BOOL bSearch = (messg == UM_SEARCH_FOCUS);
			while (gpIconHints->GetNext(h ? &h : nullptr, &h, &hi))
			{
				if ((hi.bSearchIcon == bSearch)
					&& (hi.hRootDlg == hDlg)
					&& IsWindowVisible(hi.hEditCtrl))
				{
					SetFocus(hi.hEditCtrl);
				}
			}
		} break;

	}

	return bProcessed;
}

static bool EditIconHint_ProcessKeys(HWND hCtrl, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT& lRc)
{
	bool bProcessed = false;

	switch (Msg)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		// Use Ctrl+F to set focus to the search field
		if (wParam == 'F')
		{
			bool bSearch = (isPressed(VK_CONTROL) && !isPressed(VK_MENU) && !isPressed(VK_SHIFT));
			bool bFilter = !bSearch && (isPressed(VK_MENU) && !isPressed(VK_CONTROL) && !isPressed(VK_SHIFT));
			if (bSearch || bFilter)
			{
				if ((Msg == WM_KEYDOWN) || (Msg == WM_SYSKEYDOWN))
				{
					CEIconHintOthers iho = {};
					if (!gpIconOthers->Get(hCtrl, &iho))
						break;
					PostMessage(iho.hRootDlg, bSearch ? UM_SEARCH_FOCUS : UM_FILTER_FOCUS, 0, 0);
				}
				lRc = 0;
				bProcessed = true;
			}
		}
		break;
	}

	return bProcessed;
}

static LRESULT WINAPI EditIconHintOtherProc(HWND hCtrl, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	#ifdef _DEBUG
	int iDbgId = GetDlgCtrlID(hCtrl);
	if (iDbgId == tvSetupCategories)
	{
		iDbgId = iDbgId;
	}
	#endif

	LRESULT lRc = 0;
	if (EditIconHint_ProcessKeys(hCtrl, Msg, wParam, lParam, lRc))
		return lRc;
	CEIconHintOthers iho = {};
	if (!gpIconOthers->Get(hCtrl, &iho, (Msg==WM_DESTROY)))
		return DefWindowProc(hCtrl, Msg, wParam, lParam);

	WORD wID = 0;

	switch (Msg)
	{
	case WM_KILLFOCUS:
		// Problem with tAboutText is that it autoselect whole contents
		// when focused via Tab keypress (can't be disabled with styles?)
		wID = LOWORD(GetWindowLong(hCtrl, GWL_ID));
		if (wID == tAboutText)
		{
			SendMessage(iho.hRootDlg, UM_EDIT_KILL_FOCUS, 0, reinterpret_cast<LPARAM>(hCtrl));
		}
		break;
	}

	return CallWindowProc(iho.lpPrevWndFunc, hCtrl, Msg, wParam, lParam);
}

static BOOL CALLBACK EditIconHint_Enum(HWND hCtrl, LPARAM lParam)
{
	if (!gpIconHints || gpIconHints->Get(hCtrl, nullptr))
		return TRUE;
	if (!gpIconOthers || gpIconOthers->Get(hCtrl, nullptr))
		return TRUE;
	CEIconHintOthers add = {hCtrl, (HWND)lParam};
	if ((add.lpPrevWndFunc = (WNDPROC)SetWindowLongPtr(hCtrl, GWLP_WNDPROC, (LONG_PTR)EditIconHintOtherProc)) != nullptr)
	{
		gpIconOthers->Set(hCtrl, add);
	}
	return TRUE;
}

void EditIconHint_Subclass(HWND hDlg, HWND hRootDlg /*= nullptr*/)
{
	if (!hRootDlg)
		hRootDlg = hDlg;

	EnumChildWindows(hDlg, EditIconHint_Enum, reinterpret_cast<LPARAM>(hRootDlg));
}

void EditIconHint_Set(HWND hRootDlg, HWND hEditCtrl, bool bSearchIcon, LPCWSTR sHint, bool bRedraw, UINT nSearchMsg, WORD nDefBtnID)
{
	if (!hEditCtrl)
		return;

	// First init
	if (!gpIconHandles)
	{
		gpIconHandles = (MMap<int,CEIconDrawHandles>*)calloc(sizeof(*gpIconHandles),1);
		gpIconHandles->Init(32, true);
	}
	if (!gpIconHints)
	{
		gpIconHints = (MMap<HWND,CEIconHintInfo>*)calloc(sizeof(*gpIconHints),1);
		gpIconHints->Init(32, true);
	}
	if (!gpIconOthers)
	{
		gpIconOthers = (MMap<HWND,CEIconHintOthers>*)calloc(sizeof(*gpIconHints),1);
		gpIconOthers->Init(2048, true);
	}

	CEIconHintInfo hi = {hEditCtrl, hRootDlg};
	hi.lpPrevWndFunc = (WNDPROC)GetWindowLongPtr(hEditCtrl, GWLP_WNDPROC);
	_ASSERTE(hi.lpPrevWndFunc!=nullptr);
	if (sHint)
	{
		lstrcpyn(hi.sHint, sHint, countof(hi.sHint));
		hi.clrHint = GetSysColor(COLOR_GRAYTEXT);
	}
	CToolImg* pIcon = nullptr;
	hi.iRes = EditIconHintCreateHandles(hEditCtrl, bSearchIcon ? &pIcon : nullptr);
	hi.bSearchIcon = (pIcon != nullptr);
	DWORD_PTR nMargins = SendMessage(hEditCtrl, EM_GETMARGINS, 0, 0);
	hi.nMargins = (DWORD)nMargins;
	hi.nSearchMsg = nSearchMsg;
	hi.nDefBtnID = nDefBtnID;

	gpIconHints->Set(hEditCtrl, hi);

	// Subclass control
	SetWindowLongPtr(hEditCtrl, GWLP_WNDPROC, (LONG_PTR)EditIconHintProc);

	// Margins
	RECT rect = {};
	if (pIcon && GetClientRect(hEditCtrl, &rect))
	{
		int IconWidth = /* square, same as height */ rect.bottom+1-CE_ICON_YPAD;
		SendMessage(hEditCtrl, EM_SETMARGINS, EC_RIGHTMARGIN, MAKELPARAM(LOWORD(nMargins),HIWORD(nMargins)+IconWidth+CE_ICON_SPACING));
	}

	// Done
	if (bRedraw)
		InvalidateRect(hEditCtrl, nullptr, TRUE);
}
