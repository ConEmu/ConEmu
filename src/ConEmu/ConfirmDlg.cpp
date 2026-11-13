
/*
Copyright (c) 2012-present Maximus5
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
#include "../common/MModule.h"

#include "ConEmu.h"
#include "DontEnable.h"
#include "RealConsole.h"
#include "VConRelease.h"
#include "VirtualConsole.h"

#include "ConfirmDlg.h"

#include "LngRc.h"

static HRESULT CALLBACK TaskDlgCallback(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData)
{
	switch (uNotification)
	{
		case TDN_CREATED:
		{
			DWORD nStyleEx = GetWindowLong(hwnd, GWL_EXSTYLE);
			if (!(nStyleEx & WS_EX_TOPMOST))
			{
				if ((ghWnd && ((nStyleEx = GetWindowLong(ghWnd, GWL_EXSTYLE)) & WS_EX_TOPMOST))
					|| (ghOpWnd && ((nStyleEx = GetWindowLong(ghOpWnd, GWL_EXSTYLE)) & WS_EX_TOPMOST)))
				{
					SetWindowPos(hwnd, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
				}
			}
			SendMessage(hwnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(CreateNullIcon()));
			//if (!gbAlreadyAdmin)
			//{
			//	SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver86, 1);
			//	SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver64, 1);
			//}
			break;
		}
		case TDN_VERIFICATION_CLICKED:
		{
			//SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver86, wParam);
			//SendMessage(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, Ver64, wParam);
			break;
		}
		case TDN_HYPERLINK_CLICKED:
		{
			PCWSTR pszHREF = (PCWSTR)lParam;
			if (pszHREF && *pszHREF)
				ShellExecute(hwnd, L"open", pszHREF, nullptr, nullptr, SW_SHOWNORMAL);
			break;
		}
	}
	return S_OK;
}

static MModule gComCtl32;
typedef HRESULT (WINAPI* TaskDialogIndirect_t)(const TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, BOOL *pfVerificationFlagChecked);
static TaskDialogIndirect_t TaskDialogIndirect_f = nullptr; //(TaskDialogIndirect_t)(hDll?GetProcAddress(hDll, "TaskDialogIndirect"):nullptr);

struct TaskDialogCaptionContext
{
	PFTASKDIALOGCALLBACK callback;
	LONG_PTR data;
};

static HRESULT CALLBACK TaskDialogCaptionCallback(HWND hwnd, UINT notification, WPARAM wParam, LPARAM lParam, LONG_PTR data)
{
	const auto& context = *reinterpret_cast<const TaskDialogCaptionContext*>(data);
	if (notification == TDN_CREATED)
		gpConEmu->EnableDarkCaption(hwnd);
	return context.callback(hwnd, notification, wParam, lParam, context.data);
}

HRESULT TaskDialog(TASKDIALOGCONFIG *pTaskConfig, int *pnButton, int *pnRadioButton, bool *pfVerificationFlagChecked)
{
	HRESULT hr = E_UNEXPECTED;
	BOOL bCheckBox = FALSE;

	if (!TaskDialogIndirect_f)
	{
		if (gComCtl32.Load(L"comctl32.dll"))
		{
			gComCtl32.GetProcAddress("TaskDialogIndirect", TaskDialogIndirect_f);
		}
	}

	if (TaskDialogIndirect_f)
	{
		ghDlgPendingFrom = nullptr;
		if (!pTaskConfig->pfCallback)
			pTaskConfig->pfCallback = TaskDlgCallback;
		else if (hClassIconSm)
			ghDlgPendingFrom = GetForegroundWindow();

		TASKDIALOGCONFIG config = *pTaskConfig;
		const TaskDialogCaptionContext context{config.pfCallback, config.lpCallbackData};
		if (gpConEmu && CDwmHelper::IsWindowsDarkMode())
		{
			config.pfCallback = TaskDialogCaptionCallback;
			config.lpCallbackData = reinterpret_cast<LONG_PTR>(&context);
		}
		hr = TaskDialogIndirect_f(&config, pnButton, pnRadioButton, &bCheckBox);

		ghDlgPendingFrom = nullptr;
	}

	if (pfVerificationFlagChecked)
		*pfVerificationFlagChecked = (bCheckBox != FALSE);

	return hr;
}

namespace
{
constexpr int CloseDialogLink = 100;
constexpr int CloseDialogIcon = 101;
constexpr int CloseDialogHeading = 102;

struct CloseDialog
{
	const TASKDIALOGCONFIG& config;
	HFONT font = nullptr;
	HFONT headingFont = nullptr;
};

INT_PTR CALLBACK DarkCloseDialogProc(HWND dialog, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_INITDIALOG:
	{
		auto& state = *reinterpret_cast<CloseDialog*>(lParam);
		SetWindowLongPtr(dialog, DWLP_USER, lParam);
		const auto& config = state.config;
		TaskDlgCallback(dialog, TDN_CREATED, 0, 0, 0);
		gpConEmu->ApplyPopupMenuTheme(dialog);
		gpConEmu->EnableDarkCaption(dialog);
		SetWindowText(dialog, config.pszWindowTitle);

		HDC dc = GetDC(dialog);
		const int dpi = GetDeviceCaps(dc, LOGPIXELSX);
		const int margin = MulDiv(10, dpi, 96);
		const int width = MulDiv(410, dpi, 96);
		const int inset = MulDiv(53, dpi, 96);
		NONCLIENTMETRICSW metrics = {sizeof(metrics)};
		if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics), &metrics, 0))
		{
			state.font = CreateFontIndirectW(&metrics.lfMessageFont);
			metrics.lfMessageFont.lfHeight = -MulDiv(16, dpi, 96);
			state.headingFont = CreateFontIndirectW(&metrics.lfMessageFont);
		}
		HFONT font = state.font ? state.font : static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
		HFONT headingFont = state.headingFont ? state.headingFont : font;
		const auto oldFont = SelectObject(dc, font);
		auto add = [&](LPCWSTR cls, LPCWSTR title, DWORD style, int id, int left, int top, int wide, int height)
		{
			HWND child = CreateWindowExW(0, cls, title, WS_CHILD | WS_VISIBLE | style,
				left, top, wide, height, dialog,
				reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), g_hInstance, nullptr);
			if (child)
				SendMessage(child, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
			return child;
		};
		auto textHeight = [&](LPCWSTR text, int available)
		{
			RECT bounds = {0, 0, available, 0};
			DrawTextW(dc, text, -1, &bounds, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
			return bounds.bottom;
		};

		const int textWidth = width - inset - margin;
		const int iconSize = MulDiv(32, dpi, 96);
		add(L"Static", L"", SS_OWNERDRAW, CloseDialogIcon, margin, margin, iconSize, iconSize);
		SelectObject(dc, headingFont);
		const int heading = textHeight(config.pszMainInstruction, textWidth);
		HWND label = add(L"Static", config.pszMainInstruction, SS_NOPREFIX, CloseDialogHeading, inset, margin, textWidth, heading);
		if (label)
			SendMessage(label, WM_SETFONT, reinterpret_cast<WPARAM>(headingFont), FALSE);
		int top = margin * 3 + std::max(heading, iconSize);
		bool ready = label != nullptr;
		for (UINT i = 0; i < config.cButtons; ++i)
		{
			const auto& button = config.pButtons[i];
			const wchar_t* note = wcschr(button.pszButtonText, L'\n');
			const CEStr title = note ? CEStr(button.pszButtonText, note - button.pszButtonText) : CEStr(button.pszButtonText);
			const int available = textWidth - MulDiv(38, dpi, 96);
			SelectObject(dc, headingFont);
			int height = textHeight(title, available) + 2 * margin;
			SelectObject(dc, font);
			if (note)
				height += textHeight(note + 1, available);
			ready = add(L"Button", button.pszButtonText, WS_TABSTOP | BS_OWNERDRAW,
				button.nButtonID, inset, top, textWidth, height) && ready;
			top += height;
		}
		const int linkHeight = textHeight(gsHomePage, width - 4 * margin) + MulDiv(16, dpi, 96);
		top += margin;
		ready = add(L"Button", gsHomePage, WS_TABSTOP | BS_OWNERDRAW, CloseDialogLink, 0, top, width, linkHeight) && ready;
		SelectObject(dc, oldFont);
		ReleaseDC(dialog, dc);
		if (!ready)
		{
			EndDialog(dialog, -1);
			return FALSE;
		}

		RECT frame = {0, 0, width, top + linkHeight};
		AdjustWindowRectEx(&frame, GetWindowLong(dialog, GWL_STYLE), FALSE, GetWindowLong(dialog, GWL_EXSTYLE));
		RECT parent = {};
		GetWindowRect(config.hwndParent, &parent);
		const int frameWidth = frame.right - frame.left, frameHeight = frame.bottom - frame.top;
		MONITORINFO monitor = {sizeof(monitor)};
		int x = parent.left + (parent.right - parent.left - frameWidth) / 2;
		int y = parent.top + (parent.bottom - parent.top - frameHeight) / 2;
		if (GetMonitorInfoW(MonitorFromWindow(config.hwndParent, MONITOR_DEFAULTTONEAREST), &monitor))
		{
			x = std::max(monitor.rcWork.left, std::min<LONG>(x, monitor.rcWork.right - frameWidth));
			y = std::max(monitor.rcWork.top, std::min<LONG>(y, monitor.rcWork.bottom - frameHeight));
		}
		SetWindowPos(dialog, nullptr, x, y, frameWidth, frameHeight, SWP_NOZORDER | SWP_NOACTIVATE);
		SendMessage(dialog, DM_SETDEFID, config.nDefaultButton, 0);
		SetFocus(GetDlgItem(dialog, config.nDefaultButton));
		return FALSE;
	}
	case WM_DRAWITEM:
	{
		const auto& item = *reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
		const auto& state = *reinterpret_cast<const CloseDialog*>(GetWindowLongPtr(dialog, DWLP_USER));
		if (item.CtlID == CloseDialogIcon)
		{
			DrawIconEx(item.hDC, item.rcItem.left, item.rcItem.top, state.config.hMainIcon,
				item.rcItem.right - item.rcItem.left, item.rcItem.bottom - item.rcItem.top, 0, nullptr, DI_NORMAL);
			return TRUE;
		}
		const bool focus = (item.itemState & ODS_FOCUS) != 0;
		const bool pressed = (item.itemState & ODS_SELECTED) != 0;
		const bool link = item.CtlID == CloseDialogLink;
		SetDCBrushColor(item.hDC, pressed ? RGB(43, 74, 101)
			: link ? RGB(41, 41, 41) : RGB(32, 32, 32));
		FillRect(item.hDC, &item.rcItem, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
		if (focus)
		{
			SetDCBrushColor(item.hDC, RGB(93, 131, 160));
			FrameRect(item.hDC, &item.rcItem, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
		}
		wchar_t title[MAX_PATH * 2] = {};
		GetWindowTextW(item.hwndItem, title, countof(title));
		const auto oldFont = SelectObject(item.hDC, reinterpret_cast<HFONT>(SendMessage(item.hwndItem, WM_GETFONT, 0, 0)));
		SetTextColor(item.hDC, link ? RGB(134, 193, 255) : RGB(240, 240, 240));
		SetBkMode(item.hDC, TRANSPARENT);
		RECT text = item.rcItem;
		const int dpi = GetDeviceCaps(item.hDC, LOGPIXELSX);
		const int padding = MulDiv(10, dpi, 96);
		InflateRect(&text, -padding, -(padding / 2));
		if (link)
		{
			DrawTextW(item.hDC, title, -1, &text, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
			SIZE size{};
			if (GetTextExtentPoint32W(item.hDC, title, lstrlenW(title), &size))
			{
				RECT underline = {text.left, (text.top + text.bottom + size.cy) / 2 - 1,
					text.left + size.cx, (text.top + text.bottom + size.cy) / 2};
				SetDCBrushColor(item.hDC, RGB(134, 193, 255));
				FillRect(item.hDC, &underline, static_cast<HBRUSH>(GetStockObject(DC_BRUSH)));
			}
		}
		else
		{
			if (state.headingFont)
				SelectObject(item.hDC, state.headingFont);
			SetTextColor(item.hDC, RGB(134, 193, 255));
			RECT arrow = text;
			DrawTextW(item.hDC, L"\u2192", -1, &arrow, DT_SINGLELINE | DT_NOPREFIX);
			text.left += MulDiv(24, dpi, 96);
			SetTextColor(item.hDC, RGB(240, 240, 240));
			const wchar_t* note = wcschr(title, L'\n');
			const int length = note ? static_cast<int>(note - title) : -1;
			RECT bounds = text;
			DrawTextW(item.hDC, title, length, &bounds, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
			DrawTextW(item.hDC, title, length, &text, DT_WORDBREAK | DT_NOPREFIX);
			if (note)
			{
				SelectObject(item.hDC, reinterpret_cast<HFONT>(SendMessage(item.hwndItem, WM_GETFONT, 0, 0)));
				text.top = bounds.bottom;
				DrawTextW(item.hDC, note + 1, -1, &text, DT_WORDBREAK | DT_NOPREFIX);
			}
		}
		SelectObject(item.hDC, oldFont);
		return TRUE;
	}
	case WM_CTLCOLORDLG:
	case WM_CTLCOLORSTATIC:
	{
		const auto dc = reinterpret_cast<HDC>(wParam);
		SetBkMode(dc, TRANSPARENT);
		const HWND control = reinterpret_cast<HWND>(lParam);
		SetTextColor(dc, msg == WM_CTLCOLORSTATIC && GetDlgCtrlID(control) == CloseDialogHeading
			? RGB(134, 193, 255) : RGB(240, 240, 240));
		SetDCBrushColor(dc, RGB(32, 32, 32));
		return reinterpret_cast<INT_PTR>(GetStockObject(DC_BRUSH));
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case CloseDialogLink:
			ShellExecute(dialog, L"open", gsHomePage, nullptr, nullptr, SW_SHOWNORMAL);
			return TRUE;
		case IDYES:
		case IDNO:
		case IDCANCEL:
			EndDialog(dialog, LOWORD(wParam));
			return TRUE;
		}
		break;
	case WM_CLOSE:
		EndDialog(dialog, IDCANCEL);
		return TRUE;
	}
	return FALSE;
}

HRESULT ShowCloseTaskDialog(TASKDIALOGCONFIG& config, int& button)
{
	if (CDwmHelper::IsWindowsDarkMode())
	{
		const struct
		{
			DLGTEMPLATE dialog;
			WORD menu, windowClass, title;
		} layout = {{WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME, 0, 0, 0, 0, 0, 0}, 0, 0, 0};
		CloseDialog state{config};
		const auto result = DialogBoxIndirectParamW(g_hInstance, &layout.dialog, config.hwndParent,
			DarkCloseDialogProc, reinterpret_cast<LPARAM>(&state));
		if (state.font)
			DeleteObject(state.font);
		if (state.headingFont)
			DeleteObject(state.headingFont);
		if (result != -1)
		{
			button = static_cast<int>(result);
			return S_OK;
		}
	}
	return TaskDialog(&config, &button, nullptr, nullptr);
}
}

// IDYES    - Close All consoles
// IDNO     - Close active console only
// IDCANCEL - As is
int ConfirmCloseConsoles(const ConfirmCloseParam& Parm)
{
	DontEnable de;

	wchar_t szText[512] = L"", *pszText = nullptr;
	int nBtn = IDCANCEL;

	static LONG lCounter = 0;
	LONG l = InterlockedIncrement(&lCounter);
	if (l > 1)
	{
		if (l == 2)
		{
			_ASSERTE(FALSE && "Confirm stack overflow!");
		}
		goto wrap;
	}

	if (Parm.rpLeaveConEmuOpened) *Parm.rpLeaveConEmuOpened = false;

	// Use TaskDialog?
	if (IsWin6())
	{
		// must be already initialized: CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		CEStr btnYes, btnCancel;
		CLngRc::getControl(IDYES, btnYes, L"Yes");
		CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");

		wchar_t szMessage[128];
		if (Parm.asSingleConsole)
			lstrcpyn(szMessage, Parm.asSingleConsole, countof(szMessage));
		else if (Parm.bForceKill)
			wcscpy_c(szMessage, CLngRc::getRsrc(lng_ConfirmKillingQ/*"Confirm killing?"*/));
		else if (Parm.bGroup)
			wcscpy_c(szMessage, CLngRc::getRsrc(lng_ConfirmCloseGroupQ/*"Confirm closing group?"*/));
		else
			wcscpy_c(szMessage, CLngRc::getRsrc(lng_ConfirmClosingQ/*"Confirm closing?"*/));

		wchar_t webLink[MAX_PATH]; swprintf_c(webLink, L"<a href=\"%s\">%s</a>", gsHomePage, gsHomePage);

		wchar_t szCloseAll[MAX_PATH*2];
		if (Parm.asSingleConsole)
		{
			wcscpy_c(szCloseAll, btnYes);
			wcscat_c(szCloseAll, L"\n");
			pszText = szCloseAll + _tcslen(szCloseAll);
			const int iLen = lstrlen(Parm.asSingleTitle);
			const int iCozyLen = 40;
			const int cchMax = static_cast<int>(countof(szCloseAll) - (pszText - szCloseAll));
			if (iLen <= std::min(iCozyLen, cchMax))
			{
				lstrcpyn(pszText, Parm.asSingleTitle, cchMax);
			}
			else
			{
				int iMax = std::min(iCozyLen, cchMax)-3;
				lstrcpyn(pszText, Parm.asSingleTitle, iMax);
				if ((iMax + 4) < cchMax)
					_wcscat_c(pszText, cchMax, L"...");
			}
			pszText += _tcslen(pszText);
		}
		else
		{
			wchar_t szCount[16]; swprintf_c(szCount, L" %u ", Parm.nConsoles);
			const auto* closeGroupOf = CLngRc::getRsrc(lng_ConfirmCloseBtnGroup/*"Close group of"*/);
			const auto* close = CLngRc::getRsrc(lng_ConfirmCloseBtn/*"Close"*/);
			const auto* closeAll = CLngRc::getRsrc(lng_ConfirmCloseBtnAll/*"Close all"*/);
			const auto* consoles = CLngRc::getRsrc(lng_ConfirmCloseBtnConsoles/*"consoles"*/);
			if (Parm.bGroup && (Parm.nConsoles > 1))
			{
				if (Parm.bGroup == ConfirmCloseParam::eGroup)
					wcscpy_c(szCloseAll, CEStr(closeGroupOf, szCount, consoles));
				else
					wcscpy_c(szCloseAll, CEStr(close, szCount, consoles));
			}
			else
			{
				if (Parm.nConsoles > 1)
					wcscpy_c(szCloseAll, CEStr(closeAll, szCount, consoles));
				else
					wcscpy_c(szCloseAll, CLngRc::getRsrc(lng_ConfirmCloseBtnConsole/*"Close 1 console"*/));
			}
			pszText = szCloseAll + _tcslen(szCloseAll);
		}
		if ((Parm.asSingleConsole == nullptr) || (Parm.nOperations || Parm.nUnsavedEditors))
		{
			//if (nOperations)
			{
				swprintf_c(pszText, countof(szCloseAll)-(pszText-szCloseAll)/*#SECURELEN*/, L"\n%s: %i",
					CLngRc::getRsrc(lng_ConfirmIncompleteOperations/*"Incomplete operations"*/), Parm.nOperations);
				pszText += _tcslen(pszText);
			}
			//if (nUnsavedEditors)
			{
				swprintf_c(pszText, countof(szCloseAll)-(pszText-szCloseAll)/*#SECURELEN*/, L"\n%s: %i",
					CLngRc::getRsrc(lng_ConfirmUnsavedEditors/*"Unsaved editor windows"*/), Parm.nUnsavedEditors);
				pszText += _tcslen(pszText);
			}
		}

		wchar_t szCloseOne[MAX_PATH];
		wcscpy_c(szCloseOne, CLngRc::getRsrc(lng_ConfirmCloseActiveOnly/*"Close active console only"*/));
		if (Parm.nConsoles > 1)
		{
			CVConGuard VCon;
			int iCon = gpConEmu->GetActiveVCon(&VCon);
			if (iCon >= 0)
			{
				pszText = szCloseOne + _tcslen(szCloseOne);
				swprintf_c(pszText, countof(szCloseOne)-(pszText-szCloseOne)/*#SECURELEN*/, L"\n#%u: ", (iCon+1));
				pszText += _tcslen(pszText);
				const int cchMax = static_cast<int>(countof(szCloseOne) - (pszText - szCloseOne));
				lstrcpyn(pszText, VCon->RCon()->GetTitle(true), cchMax);
			}
		}

		const CEStr szCancel(btnCancel, L"\n", CLngRc::getRsrc(lng_ConfirmDontCloseAnything/*"Don't close anything"*/));


		int nButtonPressed                  = 0;
		TASKDIALOGCONFIG config             = {sizeof(config)};
		TASKDIALOG_BUTTON buttons[]   = {
			{ IDYES,    szCloseAll },
			{ IDNO,     szCloseOne },
			{ IDCANCEL, szCancel },
		};
		config.cButtons                     = countof(buttons);
		if (Parm.nConsoles <= 1)
		{
			buttons[1] = buttons[2];
			config.cButtons--;
		}

		config.hwndParent                   = ghWnd;
		config.hInstance                    = nullptr /*g_hInstance*/;
		config.dwFlags                      = TDF_USE_HICON_MAIN|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION
		                                      |TDF_ENABLE_HYPERLINKS; //|TDIF_SIZE_TO_CONTENT|TDF_CAN_BE_MINIMIZED;
		//config.pszMainIcon                  = MAKEINTRESOURCE(IDI_ICON1);
		config.hMainIcon                    = hClassIcon;
		config.pszWindowTitle               = gpConEmu->GetDefaultTitle();
		config.pszMainInstruction           = szMessage;
		//config.pszContent                 = L"...";
		config.pButtons                     = buttons;
		config.nDefaultButton               = IDYES;
		config.pszFooter                    = webLink;

		//{
		//	config.dwFlags |= TDF_VERIFICATION_FLAG_CHECKED;
		//	config.pszVerificationText = L"Text on checkbox";
		//}

		const HRESULT hr = ShowCloseTaskDialog(config, nButtonPressed);

		if (hr == S_OK)
		{
			switch (nButtonPressed)
			{
			case IDCANCEL: // user canceled the dialog
			case IDYES:
			case IDNO:
				nBtn = nButtonPressed;
				goto wrap;

			default:
				_ASSERTE(nButtonPressed==IDCANCEL||nButtonPressed==IDYES||nButtonPressed==IDNO);
				break; // should never happen
			}
		}
	}

	// TaskDialogs aren't available in current Windows version, use standart MsgBox instead

	if (Parm.asSingleConsole)
	{
		lstrcpyn(szText,
			Parm.asSingleConsole ? Parm.asSingleConsole : Parm.bForceKill ? L"Confirm killing?" : L"Confirm closing?",
			std::min<int>(128, countof(szText)));
		wcscat_c(szText, L"\r\n\r\n");
		const int nLen = lstrlen(szText);
		lstrcpyn(szText+nLen, Parm.asSingleTitle, countof(szText)-nLen);
	}
	else
	{
		swprintf_c(szText, L"About to close %u console%s.\r\n", Parm.nConsoles, (Parm.nConsoles>1)?L"s":L"");
	}
	// ReSharper disable once CppJoinDeclarationAndAssignment
	pszText = szText + _tcslen(szText);

	if (Parm.nOperations || Parm.nUnsavedEditors)
	{
		*(pszText++) = L'\r'; *(pszText++) = L'\n'; *(pszText) = 0;

		if (Parm.nOperations)
		{
			swprintf_c(pszText, countof(szText)-(pszText-szText)/*#SECURELEN*/, L"Incomplete operations: %i\r\n", Parm.nOperations);
			pszText += _tcslen(pszText);
		}
		if (Parm.nUnsavedEditors)
		{
			swprintf_c(pszText, countof(szText)-(pszText-szText)/*#SECURELEN*/, L"Unsaved editor windows: %i\r\n", Parm.nUnsavedEditors);
			pszText += _tcslen(pszText);
		}
	}

	if (Parm.nConsoles > 1)
	{
		wcscat_c(szText,
			L"\r\nPress button <No> to close active console only\r\n"
			L"\r\nProceed with close ConEmu?");
	}

	nBtn = MsgBox(szText, (/*rpPanes ? MB_OKCANCEL :*/ (Parm.nConsoles>1) ? MB_YESNOCANCEL : MB_OKCANCEL)|MB_ICONEXCLAMATION,
		gpConEmu->GetDefaultTitle(), ghWnd);

	if (nBtn == IDOK)
	{
		nBtn = IDYES; // for unambiguity
	}

wrap:
	InterlockedDecrement(&lCounter);
	return nBtn;
}

// uType - flags from MsgBox, i.e. MB_YESNOCANCEL
int ConfirmDialog(LPCWSTR asMessage,
	LPCWSTR asMainLabel, LPCWSTR asCaption, LPCWSTR asUrl, UINT uType, HWND ahParent,
	LPCWSTR asBtn1Name /*= nullptr*/, LPCWSTR asBtn1Hint /*= nullptr*/,
	LPCWSTR asBtn2Name /*= nullptr*/, LPCWSTR asBtn2Hint /*= nullptr*/,
	LPCWSTR asBtn3Name /*= nullptr*/, LPCWSTR asBtn3Hint /*= nullptr*/)
{
	DontEnable de;

	int nBtn = IDCANCEL, nBtnCount = 0;
	CEStr szText, szWebLink, lsBtn1, lsBtn2, lsBtn3;
	LPCWSTR pszName1 = nullptr, pszName2 = nullptr, pszName3 = nullptr;
	TASKDIALOG_BUTTON buttons[3] = {};

	CEStr btnOk, btnCancel, btnAbort, btnRetry, btnIgnore, btnTry, btnYes, btnNo, btnContinue;

	static LONG lCounter = 0;
	LONG l = InterlockedIncrement(&lCounter);
	if (l > 1)
	{
		if (l == 2)
		{
			_ASSERTE(FALSE && "Confirm stack overflow!");
		}
		goto wrap;
	}

	if (!asUrl || !*asUrl)
		asUrl = gsHomePage;
	szWebLink = CEStr(L"<a href=\"", asUrl, L"\">", asUrl, L"</a>");

	switch (uType & 0xF)
	{
	case MB_OK:
		pszName1 = CLngRc::getControl(IDOK, btnOk, L"OK");
		buttons[0].nButtonID = IDOK;
		nBtnCount = 1; break;
	case MB_OKCANCEL:
		pszName1 = CLngRc::getControl(IDOK, btnOk, L"OK");
		buttons[0].nButtonID = IDOK;
		pszName2 = CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");
		buttons[1].nButtonID = IDCANCEL;
		nBtnCount = 2; break;
	case MB_ABORTRETRYIGNORE:
		pszName1 = CLngRc::getControl(IDABORT, btnAbort, L"Abort");
		buttons[0].nButtonID = IDABORT;
		pszName2 = CLngRc::getControl(IDRETRY, btnRetry, L"Retry");
		buttons[1].nButtonID = IDRETRY;
		pszName3 = CLngRc::getControl(IDIGNORE, btnIgnore, L"Ignore");
		buttons[2].nButtonID = IDIGNORE;
		nBtnCount = 3; break;
	case MB_YESNOCANCEL:
		pszName1 = CLngRc::getControl(IDYES, btnYes, L"Yes");
		buttons[0].nButtonID = IDYES;
		pszName2 = CLngRc::getControl(IDNO, btnNo, L"No");
		buttons[1].nButtonID = IDNO;
		pszName3 = CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");
		buttons[2].nButtonID = IDCANCEL;
		nBtnCount = 3; break;
	case MB_YESNO:
		pszName1 = CLngRc::getControl(IDYES, btnYes, L"Yes");
		buttons[0].nButtonID = IDYES;
		pszName2 = CLngRc::getControl(IDNO, btnNo, L"No");
		buttons[1].nButtonID = IDNO;
		nBtnCount = 2; break;
	case MB_RETRYCANCEL:
		pszName1 = CLngRc::getControl(IDRETRY, btnRetry, L"Retry");
		buttons[0].nButtonID = IDRETRY;
		pszName2 = CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");
		buttons[1].nButtonID = IDCANCEL;
		nBtnCount = 2; break;
	case MB_CANCELTRYCONTINUE:
		pszName1 = CLngRc::getControl(IDCANCEL, btnCancel, L"Cancel");
		buttons[0].nButtonID = IDCANCEL;
		pszName2 = CLngRc::getControl(IDTRYAGAIN, btnTry, L"Try");
		buttons[1].nButtonID = IDTRYAGAIN;
		pszName3 = CLngRc::getControl(IDCONTINUE, btnContinue, L"Continue");
		buttons[2].nButtonID = IDCONTINUE;
		nBtnCount = 3; break;
	default:
		_ASSERTE(FALSE && "Flag not supported");
		goto wrap;
	}

	// Use TaskDialog?
	if (gOSVer.dwMajorVersion >= 6)
	{
		HRESULT hr = E_FAIL;
		int nButtonPressed = 0;
		TASKDIALOGCONFIG config = {sizeof(config)};

		config.cButtons = nBtnCount;

		// must be already initialized: CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

		switch (uType & 0xF00)
		{
		case MB_DEFBUTTON3:
			config.nDefaultButton = buttons[2].nButtonID; break;
		case MB_DEFBUTTON2:
			config.nDefaultButton = buttons[1].nButtonID; break;
		default:
			config.nDefaultButton = buttons[0].nButtonID;
		}

		if (config.cButtons >= 1)
		{
			lsBtn1 = CEStr(asBtn1Name ? asBtn1Name : pszName1, asBtn1Hint ? L"\n" : nullptr, asBtn1Hint);
			buttons[0].pszButtonText = lsBtn1.ms_Val;
		}
		if (config.cButtons >= 2)
		{
			lsBtn2 = CEStr(asBtn2Name ? asBtn2Name : pszName2, asBtn2Hint ? L"\n" : nullptr, asBtn2Hint);
			buttons[1].pszButtonText = lsBtn2.ms_Val;
		}
		if (config.cButtons >= 3)
		{
			lsBtn3 = CEStr(asBtn3Name ? asBtn3Name : pszName3, asBtn3Hint ? L"\n" : nullptr, asBtn3Hint);
			buttons[2].pszButtonText = lsBtn3.ms_Val;
		}

		config.hwndParent                   = ahParent;
		config.dwFlags                      = /*TDF_USE_HICON_MAIN|*/TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION
		                                      |TDF_ENABLE_HYPERLINKS; //|TDIF_SIZE_TO_CONTENT|TDF_CAN_BE_MINIMIZED;
		config.pszWindowTitle               = asCaption ? asCaption : gpConEmu->GetDefaultTitle();
		config.pszMainInstruction           = asMainLabel;
		config.pszContent                   = asMessage;
		config.pButtons                     = buttons;
		config.pszFooter                    = szWebLink;

		config.hInstance = g_hInstance;
		config.pszMainIcon = MAKEINTRESOURCE(IDI_ICON1);


		// Show dialog
		hr = TaskDialog(&config, &nButtonPressed, nullptr, nullptr);

		if (hr == S_OK)
		{
			switch (nButtonPressed)
			{
			case IDOK:
			case IDCANCEL:
			case IDABORT:
			case IDRETRY:
			case IDIGNORE:
			case IDYES:
			case IDNO:
			case IDTRYAGAIN:
			case IDCONTINUE:
				nBtn = nButtonPressed;
				goto wrap;

			default:
				_ASSERTE(FALSE && "Unsupported result value");
				break; // should never happen
			}
		}
	}


	// **********************************
	// Use standard message box otherwise
	// **********************************

	szText = CEStr(
		asMainLabel, asMainLabel ? L"\r\n" : nullptr,
		asMessage);

	// URL hint?
	if (asUrl)
	{
		szText.Append(L"\r\n\r\n", asUrl);
	}

	// Button hints?
	if (asBtn1Hint || asBtn2Hint || asBtn3Hint)
	{
		szText.Append(L"\r\n\r\n");
		if (asBtn1Hint)
			szText.Append(L"\r\n<", pszName1, L"> - ", asBtn1Hint);
		if (asBtn2Hint)
			szText.Append(L"\r\n<", pszName2, L"> - ", asBtn2Hint);
		if (asBtn3Hint)
			szText.Append(L"\r\n<", pszName3, L"> - ", asBtn3Hint);
	}

	// Show dialog
	nBtn = MsgBox(szText, uType, asCaption, ahParent);
wrap:
	InterlockedDecrement(&lCounter);
	return nBtn;
}
