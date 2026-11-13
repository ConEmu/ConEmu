
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

#define SHOWDEBUGSTR

#define HIDE_USE_EXCEPTION_INFO
#include "Header.h"
#include "ConEmu.h"
#include "DwmApi_Part.h"
#include "DwmHelper.h"
#include "FrameHolder.h"
#include "Menu.h"
#include "Options.h"
#include "OptionsClass.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "../common/MHandle.h"
#include "../common/MSetter.h"

#ifdef _DEBUG
static int _nDbgStep = 0;
static void DBGFUNCTION(const char* s)
{
	//char _szDbg[512];
	//msprintf(_szDbg, sizeof(_szDbg), "%i: %s\n", ++_nDbgStep, s);
	//OutputDebugStringA(_szDbg);
	// Sleep(1000);
}
#else
static wchar_t _szDbg[512];
#define DBGFUNCTION(s) //{ wsprintf(_szDbg, L"%i: %s\n", ++_nDbgStep, s); OutputDebugString(_szDbg); /*Sleep(1000);*/ }
#endif

#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRNC(s) DEBUGSTR(s)
#define DEBUGSTRPAINT(s) //DEBUGSTR(s)

#ifdef _DEBUG
	//#define RED_CLIENT_FILL
	#undef RED_CLIENT_FILL
#else
	#undef RED_CLIENT_FILL
#endif

extern HICON hClassIconSm;

CFrameHolder::CFrameHolder()
	: mp_ConEmu(static_cast<CConEmuMain*>(this))
{
}

CFrameHolder::~CFrameHolder()
{
	mb_NcActive = false;
}

void CFrameHolder::InitFrameHolder()
{
	mb_Initialized = true;
	RecalculateFrameSizes();
}

// returns false if message not handled
bool CFrameHolder::ProcessNcMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
#ifdef _DEBUG
	if (mb_NcAnimate)
	{
		wchar_t szDbg[1024], szMsg[128], szInfo[255]; szInfo[0] = 0;
		switch (uMsg)
		{
		case WM_ERASEBKGND:
			lstrcpy(szMsg, L"WM_ERASEBKGND"); break;
		case WM_PAINT:
			lstrcpy(szMsg, L"WM_PAINT"); break;
		case WM_NCPAINT:
			lstrcpy(szMsg, L"WM_NCPAINT"); break;
		case WM_NCACTIVATE:
			lstrcpy(szMsg, L"WM_NCACTIVATE"); break;
		case WM_NCCALCSIZE:
			lstrcpy(szMsg, L"WM_NCCALCSIZE"); break;
		case WM_NCHITTEST:
			lstrcpy(szMsg, L"WM_NCHITTEST"); break;
		case WM_NCLBUTTONDOWN:
			lstrcpy(szMsg, L"WM_NCLBUTTONDOWN"); break;
		case WM_NCMOUSEMOVE:
			lstrcpy(szMsg, L"WM_NCMOUSEMOVE"); break;
		case WM_NCMOUSELEAVE:
			lstrcpy(szMsg, L"WM_NCMOUSELEAVE"); break;
		case WM_NCMOUSEHOVER:
			lstrcpy(szMsg, L"WM_NCMOUSEHOVER"); break;
		case WM_NCLBUTTONDBLCLK:
			lstrcpy(szMsg, L"WM_NCLBUTTONDBLCLK"); break;
		case 0xAE: /*WM_NCUAHDRAWCAPTION*/
			lstrcpy(szMsg, L"WM_NCUAHDRAWCAPTION"); break;
		case 0xAF: /*WM_NCUAHDRAWFRAME*/
			lstrcpy(szMsg, L"WM_NCUAHDRAWFRAME"); break;
		case 0x31E: /*WM_DWMCOMPOSITIONCHANGED*/
			lstrcpy(szMsg, L"WM_DWMCOMPOSITIONCHANGED"); break;
		case WM_WINDOWPOSCHANGED:
			lstrcpy(szMsg, L"WM_WINDOWPOSCHANGED"); break;
		case WM_SYSCOMMAND:
			lstrcpy(szMsg, L"WM_SYSCOMMAND"); break;
		case WM_GETTEXT:
			lstrcpy(szMsg, L"WM_GETTEXT"); break;
		case WM_PRINT:
			lstrcpy(szMsg, L"WM_PRINT"); break;
		case WM_PRINTCLIENT:
			lstrcpy(szMsg, L"WM_PRINTCLIENT"); break;
		case WM_GETMINMAXINFO:
			lstrcpy(szMsg, L"WM_GETMINMAXINFO"); break;
		case WM_WINDOWPOSCHANGING:
			lstrcpy(szMsg, L"WM_WINDOWPOSCHANGING"); break;
		case WM_MOVE:
			lstrcpy(szMsg, L"WM_MOVE");
			wsprintf(szInfo, L"{%i,%i}", (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
			break;
		case WM_SIZE:
			lstrcpy(szMsg, L"WM_SIZE");
			wsprintf(szInfo, L"%s {%i,%i}",
				(wParam==SIZE_MAXHIDE) ? L"SIZE_MAXHIDE" :
				(wParam==SIZE_MAXIMIZED) ? L"SIZE_MAXIMIZED" :
				(wParam==SIZE_MAXSHOW) ? L"SIZE_MAXSHOW" :
				(wParam==SIZE_MINIMIZED) ? L"SIZE_MINIMIZED" :
				(wParam==SIZE_RESTORED) ? L"SIZE_RESTORED" : L"???",
				(int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
			break;
		default:
			wsprintf(szMsg, L"%u=x%X", uMsg, uMsg);
		}

		wsprintf(szDbg, L"MsgInAnimage(%s, %u, %u%s%s)\n", szMsg, (DWORD)wParam, (DWORD)lParam,
			szInfo[0] ? L" - " : L"", szInfo);
		OutputDebugString(szDbg);
	}
#endif

	if (ghWnd && hWnd != ghWnd)
	{
		_ASSERTE(hWnd == ghWnd || !ghWnd);
		return false;
	}

	switch (uMsg)
	{
	case WM_NCHITTEST:
		return CFrameHolder::OnNcHitTest(hWnd, uMsg, wParam, lParam, lResult);
	case WM_NCPAINT:
		return CFrameHolder::OnNcPaint(hWnd, uMsg, wParam, lParam, lResult);
	case WM_NCACTIVATE:
		return OnNcActivate(hWnd, uMsg, wParam, lParam, lResult);
	case WM_NCCALCSIZE:
		return OnNcCalcSize(hWnd, uMsg, wParam, lParam, lResult);

	case WM_NCLBUTTONDOWN:
	case WM_NCLBUTTONUP:
	case WM_NCRBUTTONDOWN:
	case WM_NCRBUTTONUP:
	case WM_NCMBUTTONDOWN:
	case WM_NCMBUTTONUP:
	case WM_NCMOUSEMOVE:
	case WM_NCMOUSELEAVE:
	case WM_NCMOUSEHOVER:
		return CFrameHolder::OnNcMouseMessage(hWnd, uMsg, wParam, lParam, lResult);
	case WM_NCLBUTTONDBLCLK:
		return CFrameHolder::OnNcLButtonDblClk(hWnd, uMsg, wParam, lParam, lResult);

	//case WM_NCCREATE:
	//	mp_ConEmu->CheckGlassAttribute(); return false;
	case 0xAE: /*WM_NCUAHDRAWCAPTION*/
		return CFrameHolder::OnNcUahDrawCaption(hWnd, uMsg, wParam, lParam, lResult);
	case 0xAF: /*WM_NCUAHDRAWFRAME*/
		return CFrameHolder::OnNcUahDrawFrame(hWnd, uMsg, wParam, lParam, lResult);
	case 0x31E: /*WM_DWMCOMPOSITIONCHANGED*/
		return CFrameHolder::OnDwmCompositionChanged(hWnd, uMsg, wParam, lParam, lResult);

	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
		return CFrameHolder::OnMouseMessage(hWnd, uMsg, wParam, lParam, lResult);

	case WM_KEYDOWN:
	case WM_KEYUP:
		return CFrameHolder::OnKeyboardMessage(hWnd, uMsg, wParam, lParam, lResult);

	case WM_ERASEBKGND:
		return CFrameHolder::OnEraseBkgnd(hWnd, uMsg, wParam, lParam, lResult);
	case WM_PAINT:
		return OnPaint(hWnd, nullptr/*use BeginPaint,EndPaint*/, WM_PAINT, lResult);

	case WM_SYSCOMMAND:
		return CFrameHolder::OnSysCommand(hWnd, uMsg, wParam, lParam, lResult);
	case WM_GETTEXT:
		return CFrameHolder::OnGetText(hWnd, uMsg, wParam, lParam, lResult);

	default:
		break;
	}

	return false;
}

bool CFrameHolder::OnEraseBkgnd(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_ERASEBKGND");
	lResult = TRUE;
	return true;
}

bool CFrameHolder::OnMouseMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_MOUSExxx");

	if (mp_ConEmu->isCaptionHidden())
	{
		RECT wr = {}; GetWindowRect(hWnd, &wr);
		POINT point; // Coordinates, relative to UpperLeft corner of window
		point.x = (int)(short)LOWORD(lParam);
		point.y = (int)(short)HIWORD(lParam);

		LRESULT ht = DoNcHitTest(point, RectWidth(wr), RectHeight(wr), HTCLIENT);
		if (ht != HTNOWHERE && ht != HTCLIENT)
		{
			POINT ptScr = point;
			MapWindowPoints(hWnd, nullptr, &ptScr, 1);
			LPARAM lParamMain = MAKELONG(ptScr.x, ptScr.y);
			lResult = mp_ConEmu->WndProc(hWnd, uMsg-(WM_MOUSEMOVE-WM_NCMOUSEMOVE), ht, lParamMain);
			return true;
		}
	}
	return false;
}

bool CFrameHolder::OnGetText(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_GETTEXT");
	//TODO: Во время анимации Maximize/Restore/Minimize заголовок отрисовывается
	//TODO: системой, в итоге мелькает текст и срезаются табы
	//TODO: Сделаем, пока, чтобы текст хотя бы не мелькал...
#if 0
	if (mb_NcAnimate && gpSet->isTabsInCaption)
	{
		_ASSERTE(!IsWindows7); // Проверить на XP и ниже
		if (wParam && lParam)
		{
			*(wchar_t*)lParam = 0;
		}
		lResult = 0;
		return true;
	}
#endif
	return false;
}

bool CFrameHolder::OnNcMouseMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	bool lbRc = false;

	switch (uMsg)
	{
	case WM_NCLBUTTONDOWN:
		DBGFUNCTION("WM_NCLBUTTONDOWN");
		if ((wParam >= HTLEFT && wParam <= HTBOTTOMRIGHT) && mp_ConEmu->isSelfFrame())
			mp_ConEmu->StartForceShowFrame();
		mp_ConEmu->BeginSizing();
		break;
	case WM_NCLBUTTONUP:
		DBGFUNCTION("WM_NCLBUTTONUP");
		break;
	case WM_NCRBUTTONDOWN:
		DBGFUNCTION("WM_NCRBUTTONDOWN");
		break;
	case WM_NCRBUTTONUP:
		DBGFUNCTION("WM_NCRBUTTONUP");
		break;
	case WM_NCMBUTTONDOWN:
		DBGFUNCTION("WM_NCMBUTTONDOWN");
		break;
	case WM_NCMBUTTONUP:
		DBGFUNCTION("WM_NCMBUTTONUP");
		break;
	case WM_NCMOUSEMOVE:
		DBGFUNCTION("WM_NCMOUSEMOVE");
		break;
	case WM_NCMOUSELEAVE:
		DBGFUNCTION("WM_NCMOUSELEAVE");
		break;
	case WM_NCMOUSEHOVER:
		DBGFUNCTION("WM_NCMOUSEHOVER");
		break;
	}

	ptLastNcClick = MakePoint(LOWORD(lParam),HIWORD(lParam));

	if ((uMsg == WM_NCMOUSEMOVE) || (uMsg == WM_NCLBUTTONUP))
		mp_ConEmu->isSizing(uMsg); // могло не сброситься, проверим

	if (!lbRc)
	{
		if ((wParam == HTSYSMENU) && (uMsg == WM_NCLBUTTONDOWN))
		{
			mp_ConEmu->mp_Menu->OnNcIconLClick();
			lResult = 0;
			lbRc = true;
		}
		else if ((wParam == HTSYSMENU || wParam == HTCAPTION)
			&& (uMsg == WM_NCRBUTTONDOWN || uMsg == WM_NCRBUTTONUP))
		{
			if (uMsg == WM_NCRBUTTONUP)
			{
				LogString(L"ShowSysmenu called from (WM_NCRBUTTONUP)");
				mp_ConEmu->mp_Menu->ShowSysmenu((short)LOWORD(lParam), (short)HIWORD(lParam));
			}
			lResult = 0;
			lbRc = true;
		}
		else if ((uMsg == WM_NCRBUTTONDOWN || uMsg == WM_NCRBUTTONUP)
			&& (wParam == HTCLOSE || wParam == HTMINBUTTON || wParam == HTMAXBUTTON))
		{
			switch (LOWORD(wParam))
			{
			case HTMINBUTTON:
			case HTCLOSE:
				Icon.HideWindowToTray();
				break;
			case HTMAXBUTTON:
				mp_ConEmu->DoFullScreen();
				break;
			}
			lResult = 0;
			lbRc = true;
		}
	}
	return lbRc;
}

bool CFrameHolder::OnNcLButtonDblClk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_NCLBUTTONDBLCLK");
	bool lbRc = false;

	mp_ConEmu->mouse.state |= MOUSE_SIZING_DBLCKL; // don't forwars LBtnUp in the console

	if (wParam == HTCAPTION)
	{
		mb_NcAnimate = true;
	}

	if (wParam == HT_CONEMUTAB)
	{
		_ASSERTE(FALSE && "There is no tabs in 'Caption'");
	}
	else if (mp_ConEmu->OnMouse_NCBtnDblClk(hWnd, uMsg, wParam, lParam))
	{
		lResult = 0; // DblClick на рамке - ресайз по ширине/высоте рабочей области экрана
	}
	else
	{
		lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	if (wParam == HTCAPTION)
	{
		mb_NcAnimate = false;
	}
	return true;
}

bool CFrameHolder::OnKeyboardMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_KEYUP/WM_KEYDOWN");
	if (gpSet->isTabSelf && (wParam == VK_TAB || mp_ConEmu->mp_TabBar->IsInSwitch()))
	{
		if (isPressed(VK_CONTROL) && !isPressed(VK_MENU) && !isPressed(VK_LWIN) && !isPressed(VK_RWIN))
		{
			if (mp_ConEmu->mp_TabBar->ProcessTabKeyboardEvent(hWnd, uMsg, wParam, lParam, lResult))
			{
				return true;
			}
		}
	}
	return false;
}

bool CFrameHolder::OnSysCommand(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_SYSCOMMAND");

	if (wParam == SC_MAXIMIZE || wParam == SC_MINIMIZE || wParam == SC_RESTORE)
	{
		// Win 10 build 9926 bug?
		if ((wParam == SC_MAXIMIZE) && IsWin10())
		{
			if (!mp_ConEmu->isMeForeground(false,false))
			{
				return true;
			}
		}
		mb_NcAnimate = true;
		//GetWindowText(hWnd, ms_LastCaption, countof(ms_LastCaption));
		//SetWindowText(hWnd, L"");
	}
	// While dragging by caption we get here wParam==SC_MOVE|HTCAPTION
	lResult = mp_ConEmu->mp_Menu->OnSysCommand(hWnd, wParam, lParam, WM_SYSCOMMAND);
	if (wParam == SC_MAXIMIZE || wParam == SC_MINIMIZE || wParam == SC_RESTORE)
	{
		mb_NcAnimate = false;
		//SetWindowText(hWnd, ms_LastCaption);
	}

	return true;
}

bool CFrameHolder::OnNcUahDrawCaption(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_NCUAHDRAWCAPTION");
	if (gpSet->isLogging(2)) LogString(L"WM_NCUAHDRAWCAPTION - begin");
	MSetter inNcPaint(&mn_InNcPaint);
	lResult = DoDwmMessage(hWnd, uMsg, wParam, lParam);
	gpConEmu->RefreshTitleBarThemeColor(hWnd);
	if (gpSet->isLogging(2)) LogString(L"WM_NCUAHDRAWCAPTION - end");
	return true;
}

bool CFrameHolder::OnNcUahDrawFrame(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_NCUAHDRAWFRAME");
	if (gpSet->isLogging(2)) LogString(L"WM_NCUAHDRAWFRAME - begin");
	MSetter inNcPaint(&mn_InNcPaint);
	lResult = DoDwmMessage(hWnd, uMsg, wParam, lParam);
	if (gpSet->isLogging(2)) LogString(L"WM_NCUAHDRAWFRAME - end");
	return true;
}

bool CFrameHolder::OnDwmCompositionChanged(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_DWMCOMPOSITIONCHANGED");
	if (gpSet->isLogging(2)) LogString(L"WM_DWMCOMPOSITIONCHANGED - begin");
	lResult = DoDwmMessage(hWnd, uMsg, wParam, lParam);
	if (gpSet->isLogging(2)) LogString(L"WM_DWMCOMPOSITIONCHANGED - end");
	return true;
}

LRESULT CFrameHolder::DoDwmMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Unknown!
	_ASSERTE(uMsg == 0x31E || uMsg == 0xAE || uMsg == 0xAF);
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void CFrameHolder::NC_Redraw()
{
	mb_RedrawRequested = false;

	//TODO: При потере фокуса (клик мышкой по другому окну) лезут глюки отрисовки этого нового окна
	SetWindowPos(ghWnd, 0, 0, 0, 0, 0,
		SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE|SWP_DRAWFRAME);

	//SetWindowPos(ghWnd, nullptr, 0, 0, 0, 0,
	//		SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	//RedrawWindow(ghWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
	//RedrawWindow(ghWnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME);

	// В Aero отрисовка идет как бы на клиентской части
	if (mp_ConEmu->DrawType() == fdt_Aero)
	{
		mp_ConEmu->Invalidate(nullptr, FALSE);
	}
}

void CFrameHolder::RedrawLock()
{
	mn_RedrawLockCount++;
}

void CFrameHolder::RedrawUnlock()
{
	mn_RedrawLockCount = 0;
	if (mb_RedrawRequested)
	{
		NC_Redraw();
	}
}

void CFrameHolder::RedrawFrame()
{
	//TODO: табы/кнопки могут быть в клиентской области!
	if (mn_RedrawLockCount > 0)
	{
		mb_RedrawRequested = true;
	}
	else
	{
		NC_Redraw();
	}
}

//
//	This is a generic message handler used by WM_SETTEXT and WM_NCACTIVATE.
//	It works by turning off the WS_VISIBLE style, calling
//	the original window procedure, then turning WS_VISIBLE back on.
//
//	This prevents the original wndproc from redrawing the caption.
//	Last of all, we paint the caption ourselves with the inserted buttons
//
LRESULT CFrameHolder::NC_Wrapper(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT ret;
	DWORD dwStyle;

	dwStyle = GetWindowLong(hWnd, GWL_STYLE);

	//Turn OFF WS_VISIBLE, so that WM_NCACTIVATE does not
	//paint our window caption...
	mp_ConEmu->SetWindowStyle(hWnd, dwStyle & ~WS_VISIBLE);

	//Do the default thing..
	ret = DefWindowProc(hWnd, uMsg, wParam, lParam);

	//Restore the original style
	mp_ConEmu->SetWindowStyle(hWnd, dwStyle);

	//Paint the whole window frame + caption
	//Caption_NCPaint(ctp, hWnd, (HRGN)1);
	//if (msg == WM_NCPAINT)
	//{
	//	RECT tr; GetWindowRect(hWnd, &tr);
	//	OffsetRect(&tr, -tr.left, -tr.top);
	//
	//	tr.right -= 120; //TODO:
	//	tr.left += 26; //TODO:
	//	if (IsZoomed(hWnd))
	//		tr.top += GetSystemMetrics(SM_CYFRAME)+1;
	//	else
	//		tr.top += 3;
	//	tr.bottom = tr.top+GetSystemMetrics(SM_CYFRAME)+GetSystemMetrics(SM_CYCAPTION);
	//
	//	HDC hdc = GetWindowDC(hWnd);
	//	mp_Tabs->PaintHeader(hdc, tr);
	//	ReleaseDC(hWnd, hdc);
	//}
	return ret;
}

#if 0
void CFrameHolder::PaintFrame2k(HWND hWnd, HDC hdc, RECT &cr)
{
	//RECT cr;
	//GetWindowRect(hWnd, &cr);
	//OffsetRect(&cr, -cr.left, -cr.top);

	//TODO: Обработка XP
	HPEN hOldPen = (HPEN)SelectObject(hdc, (HPEN)GetStockObject(BLACK_PEN));
	HBRUSH hOldBr = (HBRUSH)SelectObject(hdc, (HBRUSH)GetStockObject(BLACK_BRUSH));

	HPEN hPenHilight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
	HPEN hPenLight = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DLIGHT));
	HPEN hPenShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
	HPEN hPenDkShadow = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
	HPEN hPenFace = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DFACE));
	//TODO: разобраться с шириной рамки, активной/неактивной
	HPEN hPenBorder = CreatePen(PS_SOLID, 1, GetSysColor(mb_NcActive ? COLOR_ACTIVEBORDER : COLOR_INACTIVEBORDER));
	//TODO: градиент
	HBRUSH hBrCaption = GetSysColorBrush(mb_NcActive ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);
	//
	int nCaptionY = GetSystemMetrics(SM_CYCAPTION);

	SelectObject(hdc, hPenLight);
	MoveToEx(hdc, cr.left, cr.bottom-1, nullptr); LineTo(hdc, cr.left, 0); LineTo(hdc, cr.right-1, 0);
	SelectObject(hdc, hPenHilight);
	MoveToEx(hdc, cr.left+1, cr.bottom-2, nullptr); LineTo(hdc, cr.left+1, 1); LineTo(hdc, cr.right-2, 1);
	SelectObject(hdc, hPenDkShadow);
	MoveToEx(hdc, cr.left, cr.bottom-1, nullptr); LineTo(hdc, cr.right-1, cr.bottom-1); LineTo(hdc, cr.right-1, -1);
	SelectObject(hdc, hPenShadow);
	MoveToEx(hdc, cr.left+1, cr.bottom-2, nullptr); LineTo(hdc, cr.right-2, cr.bottom-2); LineTo(hdc, cr.right-2, 0);
	// рамка. обычно это 1 пиксел цвета кнопки
	SelectObject(hdc, hPenBorder); //TODO: но может быть и более одного пиксела
	Rectangle(hdc, cr.left+2, cr.top+2, cr.right-2, cr.bottom-2);

	//TODO: Заливка заголовка через GradientFill
	SelectObject(hdc, hPenFace); SelectObject(hdc, hBrCaption);
	Rectangle(hdc, cr.left+3, cr.top+3, cr.right-3, cr.top+nCaptionY+4);
	//что осталось
	SelectObject(hdc, hPenFace);
	//Rectangle(hdc, cr.left+3, cr.top+3+nCaptionY, cr.right-3, cr.bottom-3);
	MoveToEx(hdc, cr.left+3, cr.top+3+nCaptionY, 0);
	LineTo(hdc, cr.left+3, cr.bottom-3);
	LineTo(hdc, cr.right-3, cr.bottom-3);
	LineTo(hdc, cr.right-3, cr.top+3+nCaptionY);


	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldBr);

	DeleteObject(hPenHilight); DeleteObject(hPenLight); DeleteObject(hPenShadow); DeleteObject(hPenDkShadow);
	DeleteObject(hPenFace); DeleteObject(hPenBorder);

	cr.left += 4; cr.right -= 4; cr.bottom -= 4; cr.top += 4+nCaptionY;
}
#endif

bool CFrameHolder::OnPaint(HWND hWnd, HDC hdc, UINT uMsg, LRESULT& lResult)
{
	if (hdc == nullptr)
	{
		DBGFUNCTION("WM_PAINT (nullptr)");
		LRESULT& lRc = lResult;
		PAINTSTRUCT ps = {0};
		hdc = BeginPaint(hWnd, &ps);

		if (hdc != nullptr)
		{
			if (!mp_ConEmu->isIconic() && !mp_ConEmu->InMinimizing())
			{
				OnPaint(hWnd, hdc, uMsg, lResult);
			}

			EndPaint(hWnd, &ps);
		}
		else
		{
			_ASSERTE(hdc != nullptr);
		}

		return true;
	}

	DBGFUNCTION("WM_PAINT (frame)");

	RECT rcWndReal = {}; GetWindowRect(hWnd, &rcWndReal);
	RECT rcClientReal = {}; GetClientRect(hWnd, &rcClientReal);
	#ifdef _DEBUG
	RECT rcClientMapped = rcClientReal;
	MapWindowPoints(hWnd, nullptr, (LPPOINT)&rcClientMapped, 2);
	#endif

	// Если "завис" PostUpdate
	if (mp_ConEmu->mp_TabBar->NeedPostUpdate())
		mp_ConEmu->mp_TabBar->Update();

	// Go

	RECT cr{};

	RecalculateFrameSizes();

	RECT wr = mp_ConEmu->ClientRect();
	rcClientReal = mp_ConEmu->RealClientRect();

	if (mp_ConEmu->isSelfFrame())
	{
		const auto hRgn = mp_ConEmu->CreateSelfFrameRgn();
		if (hRgn.HasHandle())
		{
			const int idx = mb_NcActive ? COLOR_ACTIVEBORDER : COLOR_INACTIVEBORDER;
			const MBrush hbr{ GetSysColorBrush(idx), nullptr };
			FillRgn(hdc, hRgn, hbr);
		}
	}

	#ifdef _DEBUG
	wchar_t szPaint[140];
	swprintf_c(szPaint, L"MainClient %s at {%i,%i}-{%i,%i} screen coords, size (%ix%i) calc (%ix%i)",
		(uMsg == WM_PAINT) ? L"WM_PAINT" : (uMsg == WM_PRINTCLIENT) ? L"WM_PRINTCLIENT" : L"UnknownMsg",
		LOGRECTCOORDS(rcClientMapped), LOGRECTSIZE(rcClientMapped), LOGRECTSIZE(wr));
	DEBUGSTRPAINT(szPaint);
	#endif

#if defined(CONEMU_TABBAR_EX)
#ifdef RED_CLIENT_FILL
	HBRUSH h = CreateSolidBrush(RGB(255,0,0));
	FillRect(hdc, &wr, h);
	DeleteObject(h);
	return 0;
#endif
#endif

	if (gpSet->isStatusBarShow)
	{
		const int nHeight = gpSet->StatusBarHeight();
		if (nHeight < (wr.bottom - wr.top))
		{
			RECT rcStatus =
				mp_ConEmu->StatusRect();
				//{wr.left, wr.bottom - nHeight, wr.right, wr.bottom};
			mp_ConEmu->mp_Status->PaintStatus(hdc, &rcStatus);
			wr.bottom = rcStatus.top;
		}
	}

	cr = wr;

	DEBUGTEST(FrameDrawStyle dt = mp_ConEmu->DrawType());


	#ifdef _DEBUG
	int nWidth = (cr.right-cr.left);
	int nHeight = (cr.bottom-cr.top);
	#endif

	lResult = 0;
	return true;
}

bool CFrameHolder::isInNcPaint()
{
	_ASSERTE(mn_InNcPaint>=0);
	return (mn_InNcPaint > 0);
}

bool CFrameHolder::OnNcPaint(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_NCPAINT");
	if (gpSet->isLogging(2)) LogString(L"WM_NCPAINT - begin");
	MSetter inNcPaint(&mn_InNcPaint);

	RecalculateFrameSizes();

	// #SIZE_TODO During restore minimized window we get artifacts - 2018-01-28_20-11-29.png

	// #NC_WARNING If the window is not themed WM_NCCALCSIZE must be passed to DefWindowProc
	//             otherwise titlebar would not be updated propertly

	//if (!IsWin10() || !mp_ConEmu->isCaptionHidden())
	{
		DBGFUNCTION("WM_NCPAINT(default1)");
		lResult = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	//else
	//{
	//	HDC hdc = GetDCEx(hWnd, (HRGN)wParam, DCX_WINDOW|DCX_INTERSECTRGN);
	//	if (hdc)
	//	{
	//		#ifdef _DEBUG
	//		RECT rcWnd{}; GetWindowRect(hWnd, &rcWnd);
	//		OffsetRect(&rcWnd, -rcWnd.left, -rcWnd.top);
	//		FillRgn(hdc, (HRGN)wParam, (HBRUSH)GetStockObject(WHITE_BRUSH));
	//		#endif
	//		ReleaseDC(hWnd, hdc);
	//	}
	//	mp_ConEmu->InvalidateAll();
	//	if (mp_ConEmu->mp_TabBar)
	//	{
	//		mp_ConEmu->mp_TabBar->Invalidate();
	//		mp_ConEmu->mp_TabBar->RePaint();
	//	}
	//}

	if (gpSet->isLogging(2)) LogString(L"WM_NCPAINT - end");
	return true;
}

bool CFrameHolder::OnNcActivate(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION((wParam != 0) ? "WM_NCACTIVATE(active)" : "WM_NCACTIVATE(inactive)");
	_ASSERTE(hWnd == ghWnd);

	bool state_changed = (mb_NcActive != (wParam != 0));
	mb_NcActive = (wParam != 0);

	if (gpSet->isLogging())
	{
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"CFrameHolder::OnNcActivate(%u,x%X)", (DWORD)wParam, (DWORD)(DWORD_PTR)lParam);
		mp_ConEmu->LogString(szInfo);
	}


	lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);


	// We can get WM_NCACTIVATE before we're actually visible. If we're not
	// visible, no need to paint.
	if (!IsWindowVisible(hWnd))
	{
		return true;
	}


	LRESULT lPaintRc = 0;
	if (mp_ConEmu->isCaptionHidden())
	{
		if (state_changed)
		{
			mp_ConEmu->InvalidateFrame();
			//CFrameHolder::OnPaint(hWnd, nullptr, WM_PAINT, lPaintRc);
		}
	}
	return true;
}

bool CFrameHolder::SetDontPreserveClient(bool abSwitch)
{
	bool b = mb_DontPreserveClient;
	mb_DontPreserveClient = abSwitch;
	return b;
}

bool CFrameHolder::SetAllowPreserveClient(bool abSwitch)
{
	bool b = mb_AllowPreserveClient;
	mb_AllowPreserveClient = abSwitch;
	return b;
}

bool CFrameHolder::OnNcCalcSize(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_NCCALCSIZE");

	RecalculateFrameSizes();

	LRESULT lRcDef = 0;

	// В Aero (Glass) важно, чтобы клиентская область начиналась с верхнего края окна,
	// иначе не получится "рисовать по стеклу"
	FrameDrawStyle fdt = mp_ConEmu->DrawType();

	// Если nCaption == 0, то при fdt_Aero текст в заголовке окна не отрисовывается
	DEBUGTEST(int nCaption = GetCaptionHeight());

	bool bCallDefProc = true; // change to (!mp_ConEmu->IsThemed())?
	bool bCallOurProc = true;

	RECT rcWnd = {}, rcClient = {};

	// We take here the part with TabBar+StatusBar+Workspace
	SizeInfo::WindowRectangles oldRect = mp_ConEmu->GetRectState();

	if (wParam)
	{
		// lParam points to an NCCALCSIZE_PARAMS structure that contains information
		// an application can use to calculate the new size and position of the client rectangle.
		// The application should indicate which part of the client area contains valid information!
		NCCALCSIZE_PARAMS* pParm = (NCCALCSIZE_PARAMS*)lParam;
		if (!pParm)
		{
			_ASSERTE(pParm!=nullptr);
			lResult = 0;
			return true;
		}
		// r[0] contains the new coordinates of a window that has been moved or resized,
		//      that is, it is the proposed new window coordinates
		// r[1] contains the coordinates of the window before it was moved or resized
		// r[2] contains the coordinates of the window's client area before the window was moved or resized
		RECT r[3] = {pParm->rgrc[0], pParm->rgrc[1], pParm->rgrc[2]};
		bool bAllowPreserveClient = mb_AllowPreserveClient
			&& (RectWidth(r[0]) == RectWidth(r[1]) && RectHeight(r[0]) == RectHeight(r[1]));

		rcWnd = r[0];
		mp_ConEmu->RequestRect(rcWnd);

		// We need to call this, otherwise some parts of window may be broken
		// If don't - system will not draw window caption when theming is off
		// #NC_WARNING If the window is not themed WM_NCCALCSIZE must be passed to DefWindowProc
		//             otherwise titlebar would not be updated propertly
		if (bCallDefProc)
		{
			lRcDef = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
			rcClient = pParm->rgrc[0];
		}

		if (bCallOurProc)
		{
			// pParm->rgrc[0] contains the coordinates of the new client rectangle resulting from the move or resize
			// pParm->rgrc[1] rectangle contains the valid destination rectangle
			// pParm->rgrc[2] rectangle contains the valid source rectangle

			if (SizeInfo::IsRectMinimized(rcWnd))
			{
				rcClient = RECT{rcWnd.left, rcWnd.top, rcWnd.left, rcWnd.top};
				pParm->rgrc[0] = rcClient;
			}
			else
			{
				// Need screen coordinates!
				const RECT rcRealClient = mp_ConEmu->RealClientRect();
				rcClient = rcRealClient;
				OffsetRect(&rcClient, rcWnd.left, rcWnd.top);

				pParm->rgrc[0] = rcClient;

				// When restoring minimized window?
				if (!SizeInfo::IsRectMinimized(r[1]))
				{
					// Mark as valid - only client area. Let system to redraw the frame and caption.
					pParm->rgrc[1] = mp_ConEmu->ClientRect();
					OffsetRect(&pParm->rgrc[1], rcWnd.left + rcRealClient.left, rcWnd.top + rcRealClient.top);
					#if 0 // leave source valid rectangle unchanged
					// Source valid rectangle
					pParm->rgrc[2] = oldRect.client;
					OffsetRect(&pParm->rgrc[2], oldRect.window.left + oldRect.real_client.left, oldRect.window.top + oldRect.real_client.top);
					#endif
				}
			}
		}

		if (bAllowPreserveClient)
		{
			lResult = WVR_VALIDRECTS;
		}
		// При смене режимов (особенно при смене HideCaption/NotHideCaption)
		// требовать полную перерисовку клиентской области
		else //if (mb_DontPreserveClient || (mp_ConEmu->GetChangeFromWindowMode() != wmNotChanging))
		{
			lResult = WVR_REDRAW;
		}

		// #NC_WARNING DefWindowProc does not return non-zero result, so we are too?
		lResult = 0;
	}
	else
	{
		// lParam points to a RECT structure. On entry, the structure contains the proposed window
		// rectangle for the window. On exit, the structure should contain the screen coordinates
		// of the corresponding window client area.
		LPRECT nccr = (LPRECT)lParam;
		if (!nccr)
		{
			_ASSERTE(nccr!=nullptr);
			lResult = 0;
			return true;
		}

		rcWnd = *nccr;
		mp_ConEmu->RequestRect(rcWnd);

		if (bCallDefProc)
		{
			// Call default function JIC
			lRcDef = ::DefWindowProc(hWnd, uMsg, wParam, lParam);
			rcClient = *nccr;
		}

		if (bCallOurProc)
		{
			// Need screen coordinates!
			rcClient = mp_ConEmu->RealClientRect();
			OffsetRect(&rcClient, rcWnd.left, rcWnd.top);

			*nccr = rcClient;
		}
	}

	wchar_t szInfo[200];
	swprintf_s(szInfo, countof(szInfo), L"WM_NCCALCSIZE(%u): Wnd={%i,%i}-{%i,%i} {%i*%i} -> Client={%i,%i}-{%i,%i} {%i*%i}",
		LODWORD(wParam), LOGRECTCOORDS(rcWnd), LOGRECTSIZE(rcWnd), LOGRECTCOORDS(rcClient), LOGRECTSIZE(rcWnd));
	DEBUGSTRNC(szInfo);

	UNREFERENCED_PARAMETER(lRcDef);
	UNREFERENCED_PARAMETER(fdt);
	return true;
}

bool CFrameHolder::OnNcHitTest(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT& lResult)
{
	DBGFUNCTION("WM_NCHITTEST");

	// Process clicks on system Min/Max/Close buttons
	if (mp_ConEmu->DrawType() >= fdt_Aero)
	{
		//TODO: Проверить, чтобы за табы окошко НЕ таскалось!
		if (mp_ConEmu->DwmDefWindowProc(hWnd, WM_NCHITTEST, 0, lParam, &lResult))
		{
			return true;
		}
	}


	lResult = DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);

	RECT wr = {}; GetWindowRect(hWnd, &wr);
	POINT point; // Coordinates, relative to UpperLeft corner of window
	point.x = (int)(short)LOWORD(lParam) - wr.left;
	point.y = (int)(short)HIWORD(lParam) - wr.top;

	lResult = DoNcHitTest(point, RectWidth(wr), RectHeight(wr), lResult);

	return true;
}

// point - coordinates relative to UpperLeft corner of window
LRESULT CFrameHolder::DoNcHitTest(const POINT& point, int width, int height, LPARAM def_hit_test /*= 0*/)
{
	LRESULT l_result = def_hit_test;

	// При скрытии окна заголовка убирается стиль WS_CAPTION,
	// но чтобы можно было оставить возможность ресайза за рамку -
	// ставится стиль WS_DLGFRAME а не WS_THICKFRAME
	// Ресайз "сам" не заработает, коррекция l_result в Win8 не помогает
	#if 0
	if (l_result == HTBORDER)
	{
		int nFrame = GetSystemMetrics(SM_CYDLGFRAME);
		int nShift = GetSystemMetrics(SM_CXSMICON);
		int nWidth = wr.right - wr.left;
		int nHeight = wr.bottom - wr.top;

		if (point.x <= nFrame && point.y <= nShift)
			l_result = HTTOPLEFT;
		else if (point.y <= nFrame && point.x <= nShift)
			l_result = HTTOPLEFT;
		else if (point.x >= (nWidth-nFrame) && point.y <= nShift)
			l_result = HTTOPRIGHT;
		else if (point.x >= (nWidth-nFrame-nShift) && point.y <= nFrame)
			l_result = HTTOPRIGHT;
		else if (point.x <= nFrame)
			l_result = HTLEFT;
		else if (point.x >= (nWidth-nFrame))
			l_result = HTRIGHT;
		else if (point.y <= nFrame)
			l_result = HTTOP;
		else if (point.y >= (nHeight-nFrame))
			l_result = HTBOTTOM;

	}
	else
	#endif
	if (mp_ConEmu->DrawType() >= fdt_Aero)
	{
		if (point.y < mp_ConEmu->GetDwmClientRectTopOffset())
		{
			int nShift = GetSystemMetrics(SM_CXSMICON);
			int nFrame = 2;
			//RECT wr; GetWindowRect(hWnd, &wr);
			int nWidth = width;
			if (point.x <= nFrame && point.y <= nShift)
				l_result = HTTOPLEFT;
			else if (point.y <= nFrame && point.x <= nShift)
				l_result = HTTOPLEFT;
			else if (point.x >= (nWidth-nFrame) && point.y <= nShift)
				l_result = HTTOPRIGHT;
			else if (point.x >= (nWidth-nFrame-nShift) && point.y <= nFrame)
				l_result = HTTOPRIGHT;
			else if (point.x > nFrame && point.x <= (nFrame+GetSystemMetrics(SM_CXSMICON)))
				l_result = HTSYSMENU;
			else if (point.x <= nFrame)
				l_result = HTLEFT;
			else if (point.x >= (nWidth-nFrame))
				l_result = HTRIGHT;
			else if (point.y <= nFrame)
				l_result = HTTOP;
			else
				l_result = HTCAPTION;
		}
	}

	// The window should not be draggable by Tabs
	if (l_result == HTCAPTION && gpSet->isTabs && mp_ConEmu->mp_TabBar)
	{
		if (mp_ConEmu->mp_TabBar->TabFromCursor(point) >= 0 || mp_ConEmu->mp_TabBar->TabBtnFromCursor(point) >= 0)
		{
			l_result = HT_CONEMUTAB;
		}
	}

	if (l_result == HTTOP && gpSet->isHideCaptionAlways()
		&& (!mp_ConEmu->mp_TabBar || !mp_ConEmu->isTabsShown()
			|| (IsWin10() && gpSet->nTabsLocation))
		)
	{
		l_result = HTCAPTION;
	}

	if (l_result == HTCLIENT
		&& mp_ConEmu->isSelfFrame()
		&& !mp_ConEmu->isInside()
		&& mp_ConEmu->isWindowNormal())
	{
		int nFrame = mp_ConEmu->GetSelfFrameWidth();
		if (nFrame > 0)
		{
			int nShift = GetSystemMetrics(SM_CXSMICON);
			int nWidth = width;
			int nHeight = height;
			enum Frames { left = 1, top = 2, right = 4, bottom = 8 };
			int frames = 0;
			if (point.x <= nFrame)
				frames |= Frames::left;
			if (point.y <= nFrame)
				frames |= Frames::top;
			if (point.x >= (nWidth-nFrame))
				frames |= Frames::right;
			if (point.y >= (nHeight-nFrame))
				frames |= Frames::bottom;

			if (((frames & Frames::left) && point.y <= nShift) || ((frames & Frames::top) && point.x <= nShift))
				l_result = HTTOPLEFT;
			else if (((frames & Frames::right) && point.y <= nShift) || ((frames & Frames::top) && point.x >= (nWidth-nFrame-nShift)))
				l_result = HTTOPRIGHT;
			else if (((frames & Frames::left) && point.y >= (nHeight-nFrame-nShift)) || ((frames & Frames::bottom) && point.x <= nShift))
				l_result = HTBOTTOMLEFT;
			else if (((frames & Frames::right) && point.y >= (nHeight-nFrame-nShift)) || ((frames & Frames::bottom) && point.x >= (nWidth-nFrame-nShift)))
				l_result = HTBOTTOMRIGHT;
			else if (frames & Frames::left)
				l_result = HTLEFT;
			else if (frames & Frames::right)
				l_result = HTRIGHT;
			else if (frames & Frames::top)
				l_result = HTTOP;
			else if (frames & Frames::bottom)
				l_result = HTBOTTOM;
		}
	}

	if (l_result == HTCLIENT && mp_ConEmu->mp_Status)
	{
		if (mp_ConEmu->mp_Status->IsCursorOverResizeMark(point))
			l_result = HTBOTTOMRIGHT;
	}


	//if ((l_result == HTCLIENT) && gpSet->isStatusBarShow)
	//{
	//	RECT rcStatus = {};
	//	if (mp_ConEmu->mp_Status->GetStatusBarItemRect(csi_SizeGrip, &rcStatus))
	//	{
	//		RECT rcFrame = mp_ConEmu->CalcMargins(CEM_FRAMECAPTION);
	//		POINT ptOver = {point.x - rcFrame.left, point.y - rcFrame.top};
	//		if (PtInRect(&rcStatus, ptOver))
	//		{
	//			l_result = HTBOTTOMRIGHT;
	//		}
	//	}
	//}

	return l_result;
}

//LRESULT CFrameHolder::OnNcLButtonDown(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//{
//	LRESULT l_result = 0;
//
//	if (wParam == HTBORDER /*HTCAPTION*/)
//	{
//		POINT point;
//		RECT wr; GetWindowRect(hWnd, &wr);
//		point.x = (int)(short)LOWORD(lParam) - wr.left;
//		point.y = (int)(short)HIWORD(lParam) - wr.top;
//		//MapWindowPoints(nullptr, hWnd, &point, 1);
//
//		int nTab = mp_ConEmu->TabFromCursor(point);
//		if (nTab >= 0)
//		{
//			mp_ConEmu->SelectTab(nTab);
//		}
//	}
//
//	l_result = DefWindowProc(hWnd, uMsg, wParam, lParam);
//
//	return l_result;
//}

void CFrameHolder::RecalculateFrameSizes()
{
	_ASSERTE(mb_Initialized); // CConEmuMain должен позвать из своего конструктора InitFrameHolder()

	//if (!IsWin10() || (!mrc_NcClientMargins.left))
	{
	//mn_WinCaptionHeight, mn_FrameWidth, mn_FrameHeight, mn_OurCaptionHeight, mn_TabsHeight;
		mn_WinCaptionHeight = GetSystemMetrics(SM_CYCAPTION);
		mn_FrameWidth = GetSystemMetrics(SM_CXFRAME);
		mn_FrameHeight = GetSystemMetrics(SM_CYFRAME);
	}
	/*
	else
	{
		RECT rcWin10 = mp_ConEmu->CalcMargins(CEM_WIN10FRAME);
		mn_FrameWidth = mrc_NcClientMargins.left;
		mn_FrameHeight = mrc_NcClientMargins.bottom;
		if (mrc_NcClientMargins.top > mrc_NcClientMargins.bottom)
			mn_WinCaptionHeight = mrc_NcClientMargins.top - mrc_NcClientMargins.bottom;
	}
	*/

	//int nCaptionDragHeight = 10;
	int nCaptionDragHeight = GetSystemMetrics(SM_CYSIZE); // - mn_FrameHeight;
	if (mp_ConEmu->DrawType() >= fdt_Themed)
	{
	//	//SIZE sz = {}; RECT tmpRc = MakeRect(600,400);
	//	//HANDLE hTheme = mp_ConEmu->OpenThemeData(nullptr, L"WINDOW");
	//	//HRESULT hr = mp_ConEmu->GetThemePartSize(hTheme, nullptr/*dc*/, 18/*WP_CLOSEBUTTON*/, 1/*CBS_NORMAL*/, &tmpRc, 2/*TS_DRAW*/, &sz);
	//	//if (SUCCEEDED(hr))
	//	//	nCaptionDragHeight = std::max(sz.cy - mn_FrameHeight,10);
	//	//mp_ConEmu->CloseThemeData(hTheme);
		nCaptionDragHeight -= mn_FrameHeight;
	}
	mn_CaptionDragHeight = std::max(10,nCaptionDragHeight);

	if (mp_ConEmu->isCaptionHidden())
	{
		mn_OurCaptionHeight = 0;
	}
	else
	{
		mn_OurCaptionHeight = mn_WinCaptionHeight;
	}


	//if (mp_ConEmu->DrawType() == fdt_Aero)
	//{
	//	rcCaption->left = 2; //GetSystemMetrics(SM_CXFRAME);
	//	rcCaption->right = (rcWindow.right - rcWindow.left) - GetSystemMetrics(SM_CXFRAME);
	//	rcCaption->top = 6; //GetSystemMetrics(SM_CYFRAME);
	//	rcCaption->bottom = mp_ConEmu->GetDwmClientRectTopOffset() - 1;
	//}
	//else if (mp_ConEmu->DrawType() == fdt_Themed)
	//{
	//	rcCaption->left = GetSystemMetrics(SM_CXFRAME);
	//	rcCaption->right = (rcWindow.right - rcWindow.left) - GetSystemMetrics(SM_CXFRAME) - 1;
	//	rcCaption->top = GetSystemMetrics(SM_CYFRAME);
	//	rcCaption->bottom = rcCaption->top + GetSystemMetrics(SM_CYCAPTION) + (gpSet->isTabs ? (GetSystemMetrics(SM_CYCAPTION)/2) : 0);
	//}
	//else
	//{
	//	rcCaption->left = GetSystemMetrics(SM_CXFRAME);
	//	rcCaption->right = (rcWindow.right - rcWindow.left) - GetSystemMetrics(SM_CXFRAME) - 1;
	//	rcCaption->top = GetSystemMetrics(SM_CYFRAME);
	//	rcCaption->bottom = rcCaption->top + GetSystemMetrics(SM_CYCAPTION) - 1 + (gpSet->isTabs ? (GetSystemMetrics(SM_CYCAPTION)/2) : 0);
	//}
}

// системный размер рамки окна
UINT CFrameHolder::GetWinFrameWidth()
{
	_ASSERTE(mb_Initialized && mn_FrameWidth > 0 && mn_FrameHeight > 0);
	return static_cast<UINT>(std::max<int>(std::max<int>(mn_FrameWidth, mn_FrameHeight), 4));
}

// высота НАШЕГО заголовка (с учетом табов)
int CFrameHolder::GetCaptionHeight()
{
	_ASSERTE(mb_Initialized && ((mn_OurCaptionHeight > 0) || mp_ConEmu->isCaptionHidden()));
	return mn_OurCaptionHeight;
}

// высота части заголовка, который оставляем для "таскания" окна
int CFrameHolder::GetCaptionDragHeight()
{
	_ASSERTE(mb_Initialized && (mn_CaptionDragHeight >= 0));
	////TODO: Хорошо бы учитывать текущее выставленное dpi для монитора?
	//if (gpSet->ilDragHeight < 0)
	//{
	//	_ASSERTE(gpSet->ilDragHeight>=0);
	//	gpSet->ilDragHeight = 0;
	//}
	//return gpSet->ilDragHeight;
	return mn_CaptionDragHeight;
}

// высота заголовка в окнах Windows по умолчанию (без учета табов)
UINT CFrameHolder::GetWinCaptionHeight()
{
	_ASSERTE(mb_Initialized && mn_WinCaptionHeight > 0);
	return static_cast<UINT>(std::max<int>(mn_WinCaptionHeight, 16));
}

void CFrameHolder::GetIconShift(POINT& IconShift)
{
	switch (mp_ConEmu->DrawType())
	{
	case fdt_Win8:
		{
			IconShift.x = 2;
			IconShift.y = 0;
		}
		break;

	case fdt_Aero:
		{
			IconShift.x = 3;
			//IconShift.y = (mp_ConEmu->isZoomed()?4:0)+(gpSet->ilDragHeight ? 4 : 1);
			IconShift.y = mp_ConEmu->GetCaptionHeight() - 16;
			if (IconShift.y < 0)
				IconShift.y = 0;
			else if (IconShift.y > 4)
				IconShift.y = 4;
			if (mp_ConEmu->isZoomed())
				IconShift.y += 4;
		}
		break;

	case fdt_Themed:
		{
			IconShift.x = 3;
			IconShift.y = mp_ConEmu->GetCaptionHeight() - 16;
			if (IconShift.y < 0)
				IconShift.y = 0;
			else if (IconShift.y > 4)
				IconShift.y = 4;
		}
		break;

	default:
		// Win2k
		{
			IconShift.x = 2;
			IconShift.y = 1; //(gpSet->ilDragHeight ? 1 : 1);
		}
	}
}

void CFrameHolder::SetFrameActiveState(bool bActive)
{
	SendMessage(ghWnd, WM_NCACTIVATE, bActive, 0);
}
