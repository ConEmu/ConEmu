
/*
Copyright (c) 2016-present Maximus5
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

#include "AboutDlg.h"
#include "DpiAware.h"
#include "ImgButton.h"
#include "ToolImg.h"

wchar_t gsDonatePage[] = CEDONATEPAGE;
wchar_t gsFlattrPage[] = CEFLATTRPAGE;

CImgButtons::CImgButtons(HWND ahDlg, int aAlignLeftId, int aAlignVCenterId)
	: hDlg(ahDlg)
	, hwndTip(nullptr)
	, AlignLeftId(aAlignLeftId)
	, AlignVCenterId(aAlignVCenterId)
{
}

CImgButtons::~CImgButtons()
{
	for (INT_PTR i = 0; i < m_Btns.size(); i++)
	{
		SafeDelete(m_Btns[i].pImg);
		SafeFree(m_Btns[i].pszHint);
	}

	if (hwndTip)
	{
		DestroyWindow(hwndTip);
		hwndTip = nullptr;
	}
}

void CImgButtons::Add(LPCWSTR asText, ToolImgCreate_t fnCreate, UINT nCtrlId, LPCWSTR asHint, OnImgBtnClick_t fnClick)
{
	BtnInfo btn = {};

	lstrcpyn(btn.ResId, asText ? asText : L"", countof(btn.ResId));

	btn.nCtrlId = nCtrlId;
	btn.pszHint = lstrdup(asHint).Detach();
	btn.OnClick = fnClick;

	btn.pImg = fnCreate();

	m_Btns.push_back(btn);
}

bool CImgButtons::GetBtnInfo(UINT nCtrlId, BtnInfo** ppBtn)
{
	if (!nCtrlId || (nCtrlId > 0xFFFF))
	{
		_ASSERTE(nCtrlId <= 0xFFFF);
		return false;
	}

	for (INT_PTR i = m_Btns.size(); (i--) > 0;)
	{
		if (m_Btns[i].nCtrlId == nCtrlId)
		{
			if (ppBtn)
				*ppBtn = &(m_Btns[i]);
			return true;
		}
	}

	return false;
}

void CImgButtons::AddDonateButtons()
{
	Add(L"DONATE", CToolImg::CreateDonateButton, pLinkDonate, gsDonatePage, ConEmuAbout::OnInfo_DonateLink);
	/*
	Add(L"FLATTR", CToolImg::CreateFlattrButton, pLinkFlattr, gsFlattrPage, ConEmuAbout::OnInfo_FlattrLink);
	*/

	ImplementButtons();
}

void CImgButtons::ImplementButtons()
{
	RECT rcLeft = {}, rcTop = {};
	HWND hCtrl;

	hCtrl = GetDlgItem(hDlg, AlignLeftId);
	GetWindowRect(hCtrl, &rcLeft);
	MapWindowPoints(nullptr, hDlg, (LPPOINT)&rcLeft, 2);
	hCtrl = GetDlgItem(hDlg, AlignVCenterId);
	GetWindowRect(hCtrl, &rcTop);
	int nPreferHeight = rcTop.bottom - rcTop.top;
	MapWindowPoints(nullptr, hDlg, (LPPOINT)&rcTop, 2);

	#ifdef _DEBUG
	DpiValue dpi;
	CDpiAware::QueryDpiForWindow(hDlg, &dpi);
	#endif

	int X = rcLeft.left;

	for (INT_PTR i = 0; i < m_Btns.size(); i++)
	{
		if (!m_Btns[i].pImg)
		{
			continue; // Image was failed
		}

		TODO("Вертикальное центрирование по объекту AlignVCenterId");

		int nDispW = 0, nDispH = 0;
		if (!m_Btns[i].pImg->GetSizeForHeight(nPreferHeight, nDispW, nDispH))
		{
			_ASSERTE(FALSE && "Image not available for dpi?");
			continue; // Image was failed?
		}
		_ASSERTE(nDispW>0 && nDispH>0);
		int nY = rcTop.top + ((rcTop.bottom - rcTop.top - nDispH + 1) / 2);

		hCtrl = CreateWindow(L"STATIC", m_Btns[i].ResId,
			WS_CHILD|WS_VISIBLE|SS_NOTIFY|SS_OWNERDRAW,
			X, nY, nDispW, nDispH,
			hDlg, (HMENU)(DWORD_PTR)m_Btns[i].nCtrlId, g_hInstance, nullptr);

		#ifdef _DEBUG
		if (!hCtrl)
			DisplayLastError(L"Failed to create image button control");
		#endif

		//X += nDispW + (10 * dpi.Ydpi / 96);
		X += nDispW + (nDispH / 3);
	}

	RegisterTip();

	UNREFERENCED_PARAMETER(hCtrl);
}

bool CImgButtons::Process(HWND ahDlg, UINT messg, WPARAM wParam, LPARAM lParam, INT_PTR& lResult)
{
	switch (messg)
	{
	case WM_SETCURSOR:
		{
			LONG_PTR lID = GetWindowLongPtr((HWND)wParam, GWLP_ID);
			BtnInfo* pBtn = nullptr;
			if (GetBtnInfo(lID, &pBtn))
			{
				SetCursor(LoadCursor(nullptr, IDC_HAND));
				lResult = TRUE;
				return true;
			}
			return false;
		}
		break;

	case WM_COMMAND:
		if (HIWORD(wParam) == BN_CLICKED)
		{
			BtnInfo* pBtn = nullptr;
			if (GetBtnInfo(LOWORD(wParam), &pBtn))
			{
				pBtn->OnClick();
				return true;
			}
		}
		break;

	case WM_DRAWITEM:
		{
			BtnInfo* pBtn = nullptr;
			if (GetBtnInfo(LOWORD(wParam), &pBtn))
			{
				if (pBtn->pImg)
				{
					DRAWITEMSTRUCT* p = (DRAWITEMSTRUCT*)lParam;
					bool bRc = pBtn->pImg->PaintButton(-1, p->hDC, p->rcItem.left, p->rcItem.top, p->rcItem.right-p->rcItem.left, p->rcItem.bottom-p->rcItem.top);
					if (bRc)
					{
						lResult = TRUE;
						return true;
					}
				}
			}
		}
		break;
	}

	return false;
}

void CImgButtons::RegisterTip()
{
	// Create the ToolTip.
	if (!hwndTip || !IsWindow(hwndTip))
	{
		hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
		                         /* BalloonStyle() | */ WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		                         CW_USEDEFAULT, CW_USEDEFAULT,
		                         CW_USEDEFAULT, CW_USEDEFAULT,
		                         ghOpWnd, nullptr,
		                         g_hInstance, nullptr);
		if (!hwndTip)
			return;
		SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		//SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
	}

	for (INT_PTR i = 0; i < m_Btns.size(); i++)
	{
		BtnInfo& btn = m_Btns[i];

		HWND hChild = GetDlgItem(hDlg, btn.nCtrlId);
		if (!hChild)
			continue;

		TOOLINFO toolInfo = {44}; //sizeof(toolInfo); -- need to work on Win2k and compile with Vista+

		toolInfo.hwnd = hChild;
		toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		toolInfo.uId = (UINT_PTR)hChild;
		toolInfo.lpszText = btn.pszHint;

		SendMessage(hwndTip, TTM_ADDTOOL, 0, (LPARAM)&toolInfo);
	}
}
