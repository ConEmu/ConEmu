
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

#include "defines.h"
#include "MAssert.h"
#include "MStrSafe.h"
#include "MToolTip.h"

#ifndef TTS_BALLOON
#define TTS_BALLOON             0x40
#define TTF_TRACK               0x0020
#define TTF_ABSOLUTE            0x0080
#define TTM_SETMAXTIPWIDTH      (WM_USER + 24)
#define TTM_TRACKPOSITION       (WM_USER + 18)  // lParam = dwPos
#define TTM_TRACKACTIVATE       (WM_USER + 17)  // wParam = TRUE/FALSE start end  lparam = LPTOOLINFO
#endif

/* *********************************** */
CToolTip::CToolTip()
{
	mh_Tip = mh_Ball = NULL;
	mpsz_LastTip = NULL;
	mn_LastTipCchMax = 0;
	memset(&mti_Tip,  0, sizeof(mti_Tip));
	memset(&mti_Ball, 0, sizeof(mti_Ball));
	mb_LastTipBalloon = -1;
}
CToolTip::~CToolTip()
{
	if (mh_Tip)
	{
		if (IsWindow(mh_Tip))
			DestroyWindow(mh_Tip);
		mh_Tip = NULL;
	}
	if (mh_Ball)
	{
		if (IsWindow(mh_Ball))
			DestroyWindow(mh_Ball);
		mh_Ball = NULL;
	}
	if (mpsz_LastTip)
	{
		free(mpsz_LastTip);
		mpsz_LastTip = NULL;
	}
}
void CToolTip::ShowTip(HWND ahOwner, HWND ahControl, LPCWSTR asText, BOOL abBalloon, POINT pt, HINSTANCE hInstance, void (*applyTheme)(HWND))
{
	HideTip();

	if (!asText || !*asText)
		return;


	int nTipLen = lstrlen(asText);
	if (!mpsz_LastTip || (nTipLen >= mn_LastTipCchMax))
	{
		if (mpsz_LastTip)
			free(mpsz_LastTip);
		mn_LastTipCchMax = nTipLen + 1;
		mpsz_LastTip = (wchar_t*)malloc(mn_LastTipCchMax*sizeof(wchar_t));
	}
	_wcscpy_c(mpsz_LastTip, mn_LastTipCchMax, asText);

	TOOLINFO *pti = abBalloon ? (&mti_Ball) : (&mti_Tip);

	if (abBalloon)
	{
		if (!mh_Ball || !IsWindow(mh_Ball))
		{
			mh_Ball = CreateWindowEx ( WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			                           WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX,
			                           CW_USEDEFAULT, CW_USEDEFAULT,
			                           CW_USEDEFAULT, CW_USEDEFAULT,
			                           ahOwner, NULL,
			                           hInstance/*g_hInstance*/, NULL);
			SetWindowPos(mh_Ball, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			// Set up tool information.
			// In this case, the "tool" is the entire parent window.
			pti->cbSize = 44; // был sizeof(TOOLINFO);
			pti->uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
			pti->hwnd = ahControl;
			pti->hinst = hInstance;
			static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
			pti->lpszText = szAsterisk;
			pti->uId = (UINT_PTR)ahControl;
			GetClientRect(ahControl, &(pti->rect));
			// Associate the ToolTip with the tool window.
			SendMessage(mh_Ball, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(pti));
			// Allow multiline
			SendMessage(mh_Ball, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(300));
		}
	}
	else
	{
		if (!mh_Tip || !IsWindow(mh_Tip))
		{
			mh_Tip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
			                         WS_POPUP | TTS_ALWAYSTIP | TTS_BALLOON | TTS_NOPREFIX,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         ahOwner, NULL,
			                         hInstance, NULL);
			SetWindowPos(mh_Tip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			SendMessage(mh_Tip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
		}
	}

	mb_LastTipBalloon = abBalloon;

	HWND hTip = abBalloon ? (mh_Ball) : (mh_Tip);
	if (!hTip)
	{
		_ASSERTE(hTip != NULL);
		return;
	}

	if (applyTheme) applyTheme(hTip);
	pti->lpszText = mpsz_LastTip;

	SendMessage(hTip, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(pti));
	//RECT rcControl; GetWindowRect(GetDlgItem(hDlg, nCtrlID), &rcControl);
	//int ptx = rcControl.right - 10;
	//int pty = (rcControl.top + rcControl.bottom) / 2;
	SendMessage(hTip, TTM_TRACKPOSITION, 0, MAKELONG(pt.x,pt.y));
	SendMessage(hTip, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(pti));

	//SetTimer(hDlg, FAILED_FONT_TIMERID, nTimeout/*FAILED_FONT_TIMEOUT*/, 0);
}
void CToolTip::HideTip()
{
	HWND hTip = (mb_LastTipBalloon == 0) ? mh_Tip : mh_Ball;
	TOOLINFO *pti = (mb_LastTipBalloon == 0) ? (&mti_Tip) : (&mti_Ball);

	if (hTip)
		SendMessage(hTip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(pti));
}
