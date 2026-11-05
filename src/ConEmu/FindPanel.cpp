
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
#include "ConEmu.h"
#include "FindPanel.h"
#include "LngRc.h"
#include "Menu.h"
#include "Options.h"
#include "OptionsClass.h"
#include "SearchCtrl.h"
#include "VConGroup.h"
#include "../common/CEStr.h"
#include "../common/MMap.h"

ATOM CFindPanel::mh_Class = 0;
#define FindPanelClass L"ConEmuFindPanel"
#define SearchHint CLngRc::getRsrc(lng_Search/*"Search"*/)
#define SearchCtrlId 1
#define SearchCtrlShift 3
#define SearchCtrlWidthMul 5

static MMap<HWND,CFindPanel*> g_FindMap;

CFindPanel::CFindPanel(CConEmuMain* apConEmu)
	: mp_ConEmu(apConEmu)
	, mh_Pane(nullptr)
	, mh_Edit(nullptr)
	, mh_Font(nullptr)
	, mfn_EditProc(nullptr)
	, mn_KeyDown(0)
	, mn_RebarHeight(0)
{
	if (!g_FindMap.Initialized())
		g_FindMap.Init(16);
	ms_PrevSearch = new CEStr();
}

CFindPanel::~CFindPanel()
{
	if (mh_Pane)
		DestroyWindow(mh_Pane);
	OnDestroy();
	SafeDelete(ms_PrevSearch);
}

HWND CFindPanel::GetHWND()
{
	return mh_Pane;
}

int CFindPanel::GetMinWidth()
{
	if (!this || !mh_Pane)
		return 0;
	return std::max(80,(int)(SearchCtrlWidthMul*mn_RebarHeight));
}

void CFindPanel::ResetSearch()
{
	ms_PrevSearch->Clear();
	mn_KeyDown = 0;
}

HWND CFindPanel::CreatePane(HWND hParent, int nBarHeight)
{
	ResetSearch();

	if (mh_Pane && IsWindow(mh_Pane))
		return mh_Pane;

	if (!RegisterPaneClass())
		return nullptr;

	_ASSERTE(mh_Edit==nullptr);
	mh_Edit = nullptr;

	mn_RebarHeight = nBarHeight;

	int nShiftY = SearchCtrlShift; //gpSetCls->EvalSize(SearchCtrlShift, esf_Vertical|esf_CanUseDpi);
	int nHeight = nBarHeight - (2 * nShiftY);
	int nWidth = nBarHeight * SearchCtrlWidthMul;

	mh_Pane = CreateWindowEx(WS_EX_CONTROLPARENT,
		FindPanelClass, L"",
		WS_CHILD|WS_VISIBLE,
		0, nShiftY, nWidth, nHeight, hParent, nullptr, nullptr, this);

	return mh_Pane;
}

bool CFindPanel::OnCreateFinished()
{
	RECT rcClient = {};
	OnSize(&rcClient);

	if (!mh_Edit)
	{
		mh_Edit = CreateWindowEx(WS_EX_CLIENTEDGE,
			L"EDIT", L"",
			WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL|ES_WANTRETURN,
			rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top,
			mh_Pane, (HMENU)SearchCtrlId, nullptr, nullptr);
		if (!mh_Edit)
		{
			return false;
		}

		g_FindMap.Set(mh_Edit, this);

		OnCreateFont();

		mfn_EditProc = (WNDPROC)SetWindowLongPtr(mh_Edit, GWLP_WNDPROC, (LONG_PTR)EditCtrlProc);

		EditIconHint_Set(mh_Pane, mh_Edit, true, SearchHint, false, UM_SEARCH, 0);
		AllowDarkModeForWindow(mh_Edit, global::g_darkModeEnabled);
		gpConEmu->SetWindowTheme(mh_Edit, global::g_darkModeEnabled ? L"DarkMode_CFD" : nullptr, nullptr);
	}

	return true;
}

void CFindPanel::OnCreateFont()
{
	if (!mh_Edit)
		return;

	HFONT hOldFont = mh_Font;
	RECT rcEdit = {};
	GetClientRect(mh_Edit, &rcEdit);

	// CreateFont
	LOGFONT lf = {};
	lf.lfWeight = FW_DONTCARE;
	lf.lfCharSet = gpSet->nTabFontCharSet;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	wcscpy_c(lf.lfFaceName, gpSet->sTabFontFace);
	lf.lfHeight = -std::max(6,(int)(rcEdit.bottom - gpSetCls->EvalSize(3, esf_Vertical|esf_CanUseDpi)));
	mh_Font = CreateFontIndirect(&lf);

	SendMessage(mh_Edit, WM_SETFONT, (WPARAM)mh_Font, FALSE);

	SafeDeleteObject(hOldFont);
}

bool CFindPanel::RegisterPaneClass()
{
	if (mh_Class)
		return true;

	WNDCLASSEX wcFindPane = {
		sizeof(wcFindPane), 0,
		FindPaneProc, 0, 0,
		g_hInstance, nullptr, LoadCursor(nullptr, IDC_ARROW),
		reinterpret_cast<HBRUSH>((COLOR_BTNFACE + 1)), nullptr,
		FindPanelClass, nullptr
	};

	mh_Class = RegisterClassEx(&wcFindPane);

	return (mh_Class != 0);
}

LRESULT CFindPanel::FindPaneProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR lRc = 0;
	CFindPanel* pPanel = nullptr;
	RECT rcClient = {};

	if (uMsg == WM_CREATE)
	{
		if (lParam && reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams)
		{
			pPanel = static_cast<CFindPanel*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
			pPanel->mh_Pane = hWnd;
		}
	}
	else
	{
		g_FindMap.Get(hWnd, &pPanel);
	}

	if (pPanel && EditIconHint_Process(hWnd, uMsg, wParam, lParam, lRc))
	{
		return lRc;
	}

	switch (uMsg)
	{
	case WM_THEMECHANGED:
		if (pPanel && pPanel->mh_Edit)
		{
			AllowDarkModeForWindow(pPanel->mh_Edit, global::g_darkModeEnabled);
			gpConEmu->SetWindowTheme(pPanel->mh_Edit, global::g_darkModeEnabled ? L"DarkMode_CFD" : nullptr, nullptr);
			pPanel->Invalidate();
		}
		break;
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if (global::g_darkModeEnabled)
		{
			HDC dc = reinterpret_cast<HDC>(wParam);
			SetTextColor(dc, DarkMode::Text);
			SetBkColor(dc, DarkMode::Control);
			return reinterpret_cast<LRESULT>(DarkMode::Brush(dc, DarkMode::Control));
		}
		break;
	case WM_ERASEBKGND:
		if (global::g_darkModeEnabled)
		{
			GetClientRect(hWnd, &rcClient);
			FillRect(reinterpret_cast<HDC>(wParam), &rcClient,
				DarkMode::Brush(reinterpret_cast<HDC>(wParam), DarkMode::Background));
			return 1;
		}
		break;
	case WM_CREATE:
		if (!pPanel || !((CFindPanel*)((CREATESTRUCT*)lParam)->lpCreateParams)->OnCreate((CREATESTRUCT*)lParam))
		{
			lRc = -1;
			goto wrap;
		}
		break;

	case WM_COMMAND:
		switch (HIWORD(wParam))
		{
		case BN_CLICKED:
			goto wrap;
		}
		break;

	case WM_WINDOWPOSCHANGING:
		if (pPanel)
			pPanel->OnWindowPosChanging((WINDOWPOS*)lParam);
		break;

	case WM_SIZE:
	case WM_MOVE:
		if (pPanel)
			pPanel->OnSize(nullptr);
		break;

	case WM_DESTROY:
		if (pPanel)
			pPanel->OnDestroy();
		break;

	case UM_SEARCH:
		if (pPanel)
			pPanel->OnSearch();
		break;
	}

	lRc = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
wrap:
	return lRc;
}

LRESULT CFindPanel::EditCtrlProc(HWND hCtrl, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT lRc = 0;
	CFindPanel* pPanel = nullptr;
	g_FindMap.Get(hCtrl, &pPanel);

	if (pPanel) switch (uMsg)
	{
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_CHAR:
		if (pPanel->OnKeyboard(uMsg, wParam, lParam, lRc))
			goto wrap;
		break;
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
		if (uMsg == WM_RBUTTONUP)
			pPanel->ShowMenu();
		goto wrap;
	}

	if (pPanel && pPanel->mfn_EditProc)
		lRc = ::CallWindowProc(pPanel->mfn_EditProc, hCtrl, uMsg, wParam, lParam);
	else
		lRc = ::DefWindowProc(hCtrl, uMsg, wParam, lParam);
wrap:
	return lRc;
}

bool CFindPanel::OnCreate(CREATESTRUCT* ps)
{
	g_FindMap.Set(mh_Pane, this);

	return true;
}

void CFindPanel::OnDestroy()
{
	g_FindMap.Del(mh_Pane);
	g_FindMap.Del(mh_Edit);
	SafeDeleteObject(mh_Font);
	mh_Pane = mh_Edit = nullptr;
}

void CFindPanel::OnWindowPosChanging(WINDOWPOS* p)
{
	_ASSERTE(p->hwnd == mh_Pane);
	RECT rcRebar = {}; GetWindowRect(GetParent(mh_Pane), &rcRebar);
	int iOptimal = SearchCtrlShift; //gpSetCls->EvalSize(SearchCtrlShift, esf_Vertical|esf_CanUseDpi);
	p->y = iOptimal;
	p->cy = (rcRebar.bottom - rcRebar.top) - 2*iOptimal;
}

void CFindPanel::OnSize(LPRECT prcEdit)
{
	if (!mh_Pane || (!mh_Edit && !prcEdit))
		return; // Nothing to do

	RECT rcRebar = {}; GetWindowRect(GetParent(mh_Pane), &rcRebar);
	RECT rcClient = {}; GetClientRect(mh_Pane, &rcClient);
	_ASSERTE(rcClient.top==0 && rcClient.left==0 && rcClient.bottom>0 && rcClient.right>0);

	// Prettify it, let set small gap before right window frame edge
	if (!gpSet->isMultiShowButtons)
	{
		POINT pt = {};
		if (MapWindowPoints(mh_Pane, ghWnd, &pt, 1) && (pt.y > 0) && (pt.y < (rcClient.bottom/3)))
		{
			rcClient.right -= pt.y;
		}
	}

	// Если контрол уже создан - двигаем
	if (mh_Edit)
	{
		mn_RebarHeight = (rcRebar.bottom - rcRebar.top);
		MoveWindow(mh_Edit, rcClient.left, rcClient.top, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top, TRUE);
	}

	if (prcEdit)
	{
		*prcEdit = rcClient;
	}
}

void CFindPanel::OnSearch()
{
	if (!CVConGroup::isVConExists(0))
		return;

	MyGetDlgItemText(mh_Edit, 0, gpSet->FindOptions.text);
	if (!gpSet->FindOptions.text.IsEmpty())
	{
		int nDirection = 0;
		if (ms_PrevSearch->ms_Val && (lstrcmp(ms_PrevSearch->ms_Val, gpSet->FindOptions.text) == 0))
			nDirection = isPressed(VK_SHIFT) ? -1 : 1;
		gpConEmu->DoFindText(nDirection);
		ms_PrevSearch->Set(gpSet->FindOptions.text);
	}
}

void CFindPanel::StopSearch()
{
	if (mh_Edit)
		SetWindowText(mh_Edit, L"");
	ms_PrevSearch->Clear();
	gpSet->SaveFindOptions();
	gpConEmu->setFocus();
	gpConEmu->DoEndFindText();
}

void CFindPanel::ShowMenu()
{
	mn_KeyDown = 0;

	enum MenuCommands
	{
		idFindNext = 1,
		idFindPrev,
		idMatchCase,
		idMatchWhole,
		idFreezeCon,
		idHilightAll,
	};

	HMENU hPopup = CreatePopupMenu();
	AppendMenu(hPopup, MF_STRING, idFindNext, L"Find next" L"\t" L"F3");
	AppendMenu(hPopup, MF_STRING, idFindPrev, L"Find prev" L"\t" L"Shift+F3");
	AppendMenu(hPopup, MF_SEPARATOR, 0, L"");
	AppendMenu(hPopup, MF_STRING | (gpSet->FindOptions.bMatchCase?MF_CHECKED:0), idMatchCase, L"Match case" L"\t" L"Alt+C");
	AppendMenu(hPopup, MF_STRING | (gpSet->FindOptions.bMatchWholeWords?MF_CHECKED:0), idMatchWhole, L"Match whole words" L"\t" L"Alt+W");
	AppendMenu(hPopup, MF_STRING | (gpSet->FindOptions.bFreezeConsole?MF_CHECKED:0), idFreezeCon, L"Freeze console" L"\t" L"Alt+F");
	#if 0
	AppendMenu(hPopup, MF_STRING | (gpSet->FindOptions.bHighlightAll?MF_CHECKED:0), idHilightAll, L"Highlight all matches");
	#endif

	bool bTabsAtBottom = (gpSet->nTabsLocation == 1);
	DWORD Align = TPM_LEFTALIGN | (bTabsAtBottom ? TPM_BOTTOMALIGN : TPM_TOPALIGN);
	RECT rcEdit = {}; GetWindowRect(mh_Edit, &rcEdit);
	POINT pt = {rcEdit.left, bTabsAtBottom ? rcEdit.top : rcEdit.bottom};

	int nCmd = gpConEmu->mp_Menu->trackPopupMenu(tmp_SearchPopup, hPopup, Align|TPM_RETURNCMD,
		pt.x, pt.y, ghWnd);

	LRESULT lRc = 0;
	switch (nCmd)
	{
	case idFindNext:
		gpConEmu->DoFindText(1); break;
	case idFindPrev:
		gpConEmu->DoFindText(-1); break;
	case idMatchCase:
		OnKeyboard(WM_SYSKEYDOWN, 'C', 0, lRc); break;
	case idMatchWhole:
		OnKeyboard(WM_SYSKEYDOWN, 'W', 0, lRc); break;
	case idFreezeCon:
		OnKeyboard(WM_SYSKEYDOWN, 'F', 0, lRc); break;
	case idHilightAll:
		OnKeyboard(WM_SYSKEYDOWN, 'A', 0, lRc); break;
	}

	DestroyMenu(hPopup);
}

bool CFindPanel::OnKeyboard(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lRc)
{
	bool bSkip = false;

	switch (uMsg)
	{
	case WM_CHAR:
		switch (LOWORD(wParam))
		{
		case VK_RETURN:
		case VK_ESCAPE:
			bSkip = true;
		}
		break;
	case WM_SYSCHAR:
		// Чтобы не пищало
		lRc = bSkip = true;
		break;
	case WM_KEYDOWN:
	case WM_KEYUP:
		switch (LOWORD(wParam))
		{
		case VK_ESCAPE:
		case VK_F10:
			if (uMsg == WM_KEYDOWN)
				mn_KeyDown = LOWORD(wParam);
			else if (mn_KeyDown == LOWORD(wParam))
				StopSearch();
			bSkip = true;
			break;

		case VK_RETURN:
		case VK_F3:
			if (uMsg == WM_KEYDOWN)
				gpConEmu->DoFindText(isPressed(VK_SHIFT) ? -1 : 1);
			bSkip = true;
			break;

		case VK_APPS:
			if (uMsg == WM_KEYDOWN)
				mn_KeyDown = VK_APPS;
			else if (mn_KeyDown == VK_APPS)
				ShowMenu();
			bSkip = true;
			break;
		}
		break;

	case WM_SYSKEYDOWN:
		bSkip = true;
		lRc = TRUE;
		switch (LOWORD(wParam))
		{
		case 'C':
			gpSet->FindOptions.bMatchCase = !gpSet->FindOptions.bMatchCase;
			break;
		case 'W':
			gpSet->FindOptions.bMatchWholeWords = !gpSet->FindOptions.bMatchWholeWords;
			break;
		case 'F':
			gpSet->FindOptions.bFreezeConsole = !gpSet->FindOptions.bFreezeConsole;
			break;
		case 'A':
			gpSet->FindOptions.bHighlightAll = !gpSet->FindOptions.bHighlightAll;
			break;
		case VK_F10:
			mn_KeyDown = LOWORD(wParam);
			bSkip = true;
			goto wrap;
		default:
			goto wrap;
		}
		gpConEmu->DoFindText(isPressed(VK_SHIFT) ? -1 : 1);
		break;
	case WM_SYSKEYUP:
		if (mn_KeyDown == LOWORD(wParam))
			StopSearch();
		bSkip = true;
		lRc = TRUE;
		goto wrap;
	}

wrap:
	return bSkip;
}

bool CFindPanel::IsAvailable(bool bFilled)
{
	if (!mh_Edit || !IsWindowEnabled(mh_Edit))
		return false;

	// Если не нужно знать, заполнено ли поле (то есть мы в процессе поиска)
	if (!bFilled)
		return true;

	// Фокус в поле - мы в поиске
	if (mh_Edit && (GetFocus() == mh_Edit))
		return true;
	// Или уже введен текст
	if (GetWindowTextLength(mh_Edit) > 0)
		return true;

	// Поиск не был запущен
	return false;
}

void CFindPanel::Invalidate()
{
	// #SIZE_TODO When caption is hidden, the search field is not painted properly on startup when `-detached`
	if (mh_Pane)
		::InvalidateRect(mh_Pane, nullptr, FALSE);
	if (mh_Edit)
		::InvalidateRect(mh_Edit, nullptr, FALSE);
}

HWND CFindPanel::Activate(bool bActivate)
{
	if (bActivate)
	{
		if (!IsAvailable(false))
			return nullptr;
		if (!IsWindowVisible(mh_Pane))
			::ShowWindow(mh_Pane, SW_SHOWNORMAL);
		SetFocus(mh_Edit);
	}
	else
	{
		StopSearch();
	}
	return mh_Edit;
}
