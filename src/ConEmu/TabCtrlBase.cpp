
/*
Copyright (c) 2013-present Maximus5
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

#define DEBUGSTRTABS(s) //DEBUGSTR(s)
#define DEBUGSTRWARN(s) DEBUGSTR(s)
#define DEBUGSTRSEL(s) DEBUGSTR(s)

#include "../common/defines.h"
#include "Header.h"
#include "TabBar.h"
#include "TabCtrlBase.h"
#include "DarkMode.h"
#include "Options.h"
#include "OptionsClass.h"
#include "ConEmu.h"
#include "VirtualConsole.h"
#include "TrayIcon.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "Status.h"
#include "Menu.h"

#define TAB_DRAG_START_DELTA 5
#define MIN_TABCLICK_CHANGE_DELTA 200

CTabPanelBase::CTabPanelBase(CTabBarClass* ap_Owner)
{
	mp_Owner = ap_Owner;
	mh_TabFont = nullptr;
	mh_TabTip = mh_Balloon = nullptr;
	ms_TabErrText[0] = 0;
	memset(&tiBalloon,0,sizeof(tiBalloon));
	mn_prevTab = -1;
	mn_LBtnDrag = 0;
	mh_DragCursor = nullptr;
	ZeroStruct(mpt_DragStart);
	mn_InSelChange = 0;
	mn_LastChangeTick = 0;
}

CTabPanelBase::~CTabPanelBase()
{
	if (mh_TabFont)
	{
		DeleteObject(mh_TabFont);
		mh_TabFont = nullptr;
	}
}

void CTabPanelBase::UpdateTabFontInt()
{
	if (!IsTabbarCreated())
		return;

	LOGFONT lf = {};
	lf.lfWeight = FW_DONTCARE;
	lf.lfCharSet = gpSet->nTabFontCharSet;
	lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
	wcscpy_c(lf.lfFaceName, gpSet->sTabFontFace);
	gpFontMgr->EvalLogfontSizes(lf, gpSet->nTabFontHeight, 0);

	wchar_t szInfo[100];
	swprintf_c(szInfo, L"Creating tab font name='%s' height=%i", lf.lfFaceName, lf.lfHeight);
	LogString(szInfo);

	// CreateFont
	HFONT hFont = CreateFontIndirect(&lf);

	// virtual
	SetTabbarFont(hFont);

	// Store new Font in member variable
	if (mh_TabFont)
		DeleteObject(mh_TabFont);
	mh_TabFont = hFont;
}

void CTabPanelBase::ShowTabErrorInt(LPCTSTR asInfo, int tabIndex)
{
	if (!asInfo)
		ms_TabErrText[0] = 0;
	else if (asInfo != ms_TabErrText)
		lstrcpyn(ms_TabErrText, asInfo, countof(ms_TabErrText));

	tiBalloon.lpszText = ms_TabErrText;

	if (ms_TabErrText[0])
	{
		SendMessage(mh_TabTip, TTM_ACTIVATE, FALSE, 0);
		SendMessage(mh_Balloon, TTM_UPDATETIPTEXT, 0, (LPARAM)&tiBalloon);

		//RECT rcControl; GetWindowRect(GetDlgItem(hMain, tFontFace), &rcControl);
		RECT rcTab = {0}; GetTabRect(tabIndex, &rcTab);

		//int ptx = 0; //rcControl.right - 10;
		//int pty = 0; //(rcControl.top + rcControl.bottom) / 2;
		SendMessage(mh_Balloon, TTM_TRACKPOSITION, 0, MAKELONG((rcTab.left+rcTab.right)>>1,rcTab.bottom));
		SendMessage(mh_Balloon, TTM_TRACKACTIVATE, TRUE, (LPARAM)&tiBalloon);
		gpConEmu->SetKillTimer(true, TIMER_FAILED_TABBAR_ID, TIMER_FAILED_TABBAR_ELAPSE);
	}
	else
	{
		SendMessage(mh_Balloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tiBalloon);
		SendMessage(mh_TabTip, TTM_ACTIVATE, TRUE, 0);
	}
}

//int CTabPanelBase::GetTabIcon(bool bAdmin)
//{
//	return m_TabIcons.GetTabIcon(bAdmin);
//}

bool CTabPanelBase::OnTimerInt(WPARAM wParam)
{
	if (wParam == TIMER_FAILED_TABBAR_ID)
	{
		gpConEmu->SetKillTimer(false, TIMER_FAILED_TABBAR_ID, 0);
		SendMessage(mh_Balloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&tiBalloon);
		SendMessage(mh_TabTip, TTM_ACTIVATE, TRUE, 0);
		return true;
	}
	return false;
}

//void CTabPanelBase::InitIconList()
//{
//	if (!m_TabIcons.IsInitialized())
//	{
//		m_TabIcons.Initialize();
//	}
//}

void CTabPanelBase::InitTooltips(HWND hParent)
{
	if (!mh_TabTip || !IsWindow(mh_TabTip))
	{
		mh_TabTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
									WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
									CW_USEDEFAULT, CW_USEDEFAULT,
									CW_USEDEFAULT, CW_USEDEFAULT,
									hParent, nullptr,
									g_hInstance, nullptr);
		DarkMode::ApplyTooltip(mh_TabTip);
		SetWindowPos(mh_TabTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
	}

	if (!mh_Balloon || !IsWindow(mh_Balloon))
	{
		mh_Balloon = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
		                            gpSetCls->BalloonStyle() | WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
		                            CW_USEDEFAULT, CW_USEDEFAULT,
		                            CW_USEDEFAULT, CW_USEDEFAULT,
		                            hParent, nullptr,
		                            g_hInstance, nullptr);
		DarkMode::ApplyTooltip(mh_Balloon);
		SetWindowPos(mh_Balloon, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
		// Set up tool information.
		// In this case, the "tool" is the entire parent window.
		tiBalloon.cbSize = 44; // был sizeof(TOOLINFO);
		tiBalloon.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
		tiBalloon.hwnd = hParent;
		tiBalloon.hinst = g_hInstance;
		static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
		tiBalloon.lpszText = szAsterisk;
		tiBalloon.uId = (UINT_PTR)TTF_IDISHWND;
		GetTabBarClientRect(&tiBalloon.rect);
		// Associate the ToolTip with the tool window.
		SendMessage(mh_Balloon, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO) &tiBalloon);
		// Allow multiline
		SendMessage(mh_Balloon, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(300));
	}
}

bool CTabPanelBase::FarSendChangeTab(int tabIndex, CVConGuard* rpVCon /*= nullptr*/)
{
	CVConGuard VCon;
	DWORD wndIndex = 0;
	BOOL  bNeedActivate = FALSE, bChangeOk = FALSE;
	ShowTabErrorInt(nullptr, 0);

	bool bVConOk = mp_Owner->GetVConFromTab(tabIndex, &VCon, &wndIndex);

	static DWORD nLastTick = 0; DWORD nCurTick = GetTickCount();
	wchar_t szInfo[120];
	int iTickDelta = nLastTick ? (int)(nCurTick - nLastTick) : -1;
	swprintf_c(szInfo, L"Change tab requested: Tab=%i VCon=%i Wnd=%i Delta=%i",
		tabIndex+1, VCon.VCon() ? VCon->Index() : -1, wndIndex, iTickDelta);
	if (gpSet->isLogging()) { LogString(szInfo); } else { DEBUGSTRSEL(szInfo); }
	// _ASSERTE((iTickDelta==-1 || iTickDelta>=250) && "Suspicious fast tab switching, may be not intended by user"); -- may occurs while closing all tabs or window
	nLastTick = nCurTick;

	if (rpVCon)
		rpVCon->Release();

	if (!bVConOk)
	{
		if (mp_Owner->IsInSwitch())
			mp_Owner->Update();  // показать реальное положение дел

		return false;
	}

	CVirtualConsole *pVCon = VCon.VCon();

	if (!pVCon->isActive(false))
		bNeedActivate = TRUE;

	DWORD nCallStart = TimeGetTime(), nCallEnd = 0;

	CRealConsole *pRCon = pVCon->RCon();
	bChangeOk = pRCon->ActivateFarWindow(wndIndex);

	if (!bChangeOk)
	{
		// Всплыть тултип с руганью - не смогли активировать
		ShowTabErrorInt(pRCon->GetActivateFarWindowError(ms_TabErrText, countof(ms_TabErrText)), tabIndex);
	}
	else
	{
		nCallEnd = TimeGetTime();
		if ((nCallEnd - nCallStart) > ACTIVATE_TAB_CRITICAL)
		{
			DEBUGSTRWARN(L"*** Long Far tab activation duration! ***\n");
			_ASSERTE(((nCallEnd - nCallStart) <= ACTIVATE_TAB_CRITICAL) || IsDebuggerPresent());
		}
	}

	// Чтобы лишнее не мелькало - активируем консоль
	// ТОЛЬКО после смены таба (успешной или неудачной - неважно)
	TODO("Не срабатывает. Новые данные из консоли получаются с задержкой и мелькает старое содержимое активируемой консоли");

	if (bNeedActivate)
	{
		if (!gpConEmu->Activate(pVCon))
		{
			if (mp_Owner->IsInSwitch())
				mp_Owner->Update();  // показать реальное положение дел

			TODO("А текущий таб не слетит, если активировать не удалось?");
			return false;
		}
	}

	if (!bChangeOk)
	{
		pVCon = nullptr;

		if (mp_Owner->IsInSwitch())
			mp_Owner->Update();  // показать реальное положение дел
	}

	UNREFERENCED_PARAMETER(nCallStart);
	UNREFERENCED_PARAMETER(nCallEnd);

	if (rpVCon)
		rpVCon->Attach(pVCon);
	return (pVCon != nullptr);
}

LRESULT CTabPanelBase::OnMouseRebar(UINT uMsg, int x, int y)
{
	if (!gpSet->isTabs)
		return 0;

	if (uMsg == WM_LBUTTONDBLCLK)
	{
		if ((gpSet->nTabBarDblClickAction == TabBarDblClick::MaxRestoreWindow)
			|| ((gpSet->nTabBarDblClickAction == TabBarDblClick::Auto) && gpConEmu->isCaptionHidden()))
		{
			LogString(L"ToolBar: ReBar double click: DoMaximizeRestore");
			// Чтобы клик случайно не провалился в консоль
			gpConEmu->mouse.state |= MOUSE_SIZING_DBLCKL;
			// Аналог AltF9
			gpConEmu->DoMaximizeRestore();
		}
		else if ((gpSet->nTabBarDblClickAction == TabBarDblClick::OpenNewShell)
			|| ((gpSet->nTabBarDblClickAction == TabBarDblClick::Auto) && !gpConEmu->isCaptionHidden()))
		{
			LogString(L"ToolBar: ReBar double click: RecreateAction");
			gpConEmu->RecreateAction(cra_CreateTab/*FALSE*/, gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
		}
		#ifdef _DEBUG
		else
		{
			_ASSERTE(FALSE && "gpSet->nTabBarDblClickAction: Action was not processed");
		}
		#endif
	}
	else if (uMsg == WM_RBUTTONUP)
	{
		POINT ptScr; GetCursorPos(&ptScr);
		LogString(L"ShowSysmenu called from (TabBar)");
		gpConEmu->mp_Menu->ShowSysmenu(ptScr.x, ptScr.y/*-32000*/);
	}
	else if (gpConEmu->GetWindowMode() == wmNormal)
	{
		// WM_NC* messages needs screen coords in lParam
		POINT ptScr; GetCursorPos(&ptScr);
		LPARAM lParamMain = MAKELONG(ptScr.x,ptScr.y);
		gpConEmu->WndProc(ghWnd, uMsg-(WM_MOUSEMOVE-WM_NCMOUSEMOVE), HTCAPTION, lParamMain);
	}
	else if (uMsg == WM_MBUTTONUP)
	{
		LogString(L"ToolBar: ReBar MBtnUp: RecreateAction");
		gpConEmu->RecreateAction(cra_CreateTab/*FALSE*/, gpSet->isMultiNewConfirm || isPressed(VK_SHIFT));
	}

	return 0;
}

void CTabPanelBase::DoTabDrag(UINT uMsg)
{
	if (mn_LBtnDrag && !isPressed(VK_LBUTTON))
	{
		EndTabDrag();
		return;
	}

	POINT ptCur = {}; GetCursorPos(&ptCur);
	if (mn_LBtnDrag == 1)
	{
		if (_abs(ptCur.x - mpt_DragStart.x) <= TAB_DRAG_START_DELTA)
		{
			// Еще не начали
			return;
		}

		mn_LBtnDrag = 2;

		if (!mh_DragCursor)
			mh_DragCursor = LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_MOVE));
		if (mh_DragCursor)
			SetCursor(mh_DragCursor);
	}

	int iHoverTab = GetTabFromPoint(ptCur, true, false);
	int iActiveTab = GetCurSelInt();
	if ((iHoverTab >= 0) && (iActiveTab >= 0) && (iHoverTab != iActiveTab))
	{
		bool bLeftward = (iHoverTab < iActiveTab);
		CVConGuard VHover, VActive;
		DWORD nHoverWnd = 0, nActiveWnd = 0;
		RECT rcHover = {}, rcActive = {};
		RECT rcNew = {}, rcOld = {};
		int iHoverWidth, iActiveWidth;
		int iFindNew, iFindOld;

		if (!mp_Owner->GetVConFromTab(iHoverTab, &VHover, &nHoverWnd))
			goto wrap;
		if (!mp_Owner->GetVConFromTab(iActiveTab, &VActive, &nActiveWnd))
			goto wrap;

		TODO("Драг редакторов/вьюверах в пределах табов одного VCon");
		if (VHover.VCon() == VActive.VCon())
			goto wrap;

		if (!GetTabRect(iActiveTab, &rcActive))
			goto wrap;

		if (!GetTabRect(iHoverTab, &rcHover))
			goto wrap;

		// Если отображаются табы редакторов/вьюверов - нужны доп.телодвижения,
		// иначе rcNew/rcOld будут содержать "крайние/активные" координаты,
		// а после рокировки табов они поменяются.
		// Например, слева от текущего - три таба редактора и только потом панель,
		// и если тащится только один таб с "cmd", то он "прыгнет" не на одну влево
		// а через три, в итоге при движении мышки будет постоянное мельтешение.
		iFindNew = mp_Owner->GetFirstLastVConTab(VHover.VCon(), bLeftward, iHoverTab);
		if ((iFindNew < 0) || !GetTabRect(iFindNew, &rcNew))
			rcNew = rcHover;
		iFindOld = mp_Owner->GetFirstLastVConTab(VActive.VCon(), !bLeftward, iActiveTab);
		if ((iFindOld < 0) || !GetTabRect(iFindOld, &rcOld))
			rcOld = rcActive;

		// Если текущий таб короче нового - то нужны доп.проверки
		// чтобы при драге таб не стал мельтешить вправо/влево
		iHoverWidth = (std::max(rcHover.right,rcNew.right) - std::min(rcHover.left,rcNew.left));
		_ASSERTE(iHoverWidth>0);
		iActiveWidth = (std::max(rcActive.right,rcOld.right) - std::min(rcActive.left,rcOld.left));
		_ASSERTE(iActiveWidth>0);

		if (iActiveWidth < iHoverWidth)
		{
			if (bLeftward)
			{
				// Драг влево
				//if ((rcActive.right - iHoverWidth) > (rcHover.left + iActiveWidth))
				if (ptCur.x > (rcNew.left + iActiveWidth))
					goto wrap;
			}
			else
			{
				// Драг вправо
				//if ((rcActive.left + iHoverWidth) < (rcHover.right - iActiveWidth))
				if (ptCur.x < (rcNew.right - iActiveWidth))
					goto wrap;
			}
		}

		CVConGroup::MoveActiveTab(VActive.VCon(), bLeftward);
	}

wrap:
	if (uMsg == WM_LBUTTONUP)
	{
		EndTabDrag();
	}
}

void CTabPanelBase::EndTabDrag()
{
	if (!mn_LBtnDrag)
		return;

	mn_LBtnDrag = 0;
	SetCursor(LoadCursor(nullptr, IDC_ARROW));
}

LRESULT CTabPanelBase::OnMouseTabbar(UINT uMsg, int nTabIdx, int x, int y)
{
	if (nTabIdx == -1)
		nTabIdx = GetTabFromPoint(MakePoint(x,y), false);

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
		{
			wchar_t szInfo[120];
			if (mn_LastChangeTick && ((GetTickCount() - mn_LastChangeTick) <= MIN_TABCLICK_CHANGE_DELTA))
			{
				wcscpy_c(szInfo, L"Tab LeftClick was skipped (mn_LastChangeTick)");
				if (gpSet->isLogging()) { LogString(szInfo); } else { DEBUGSTRSEL(szInfo); }
				break;
			}
			// Может приходить и из ReBar по клику НАД табами
			mn_LBtnDrag = 1;
			GetCursorPos(&mpt_DragStart);
			int lnCurTab = GetCurSelInt();
			if (lnCurTab != nTabIdx)
			{
				swprintf_c(szInfo, L"Tab was LeftClicked Tab=%i OldTab=%i InSelChange=%i", nTabIdx+1, lnCurTab+1, mn_InSelChange);
				if (gpSet->isLogging()) { LogString(szInfo); } else { DEBUGSTRSEL(szInfo); }

				if (mn_InSelChange == 0)
				{
					FarSendChangeTab(nTabIdx);
				}
				else
				{
					wcscpy_c(szInfo, L"Tab LeftClick was skipped (mn_InSelChange)");
					if (gpSet->isLogging()) { LogString(szInfo); } else { DEBUGSTRSEL(szInfo); }
				}
			}
			mn_LastChangeTick = GetTickCount();
			break;
		}
	case WM_LBUTTONUP:
		{
			if (mn_LBtnDrag)
			{
				DoTabDrag(uMsg);
			}
			break;
		}
	case WM_MOUSEMOVE:
		{
			if (mn_LBtnDrag)
				DoTabDrag(uMsg);
			break;
		}
	case WM_SETCURSOR:
		{
			if (mn_LBtnDrag)
			{
				if (isPressed(VK_LBUTTON))
				{
					if (mh_DragCursor)
					{
						POINT ptCur = {}; GetCursorPos(&ptCur);
						if (_abs(ptCur.x - mpt_DragStart.x) > TAB_DRAG_START_DELTA)
						{
							SetCursor(mh_DragCursor);
							if (mn_LBtnDrag == 1)
								mn_LBtnDrag = 2;
							return TRUE;
						}
					}
				}
				else
				{
					EndTabDrag();
				}
			}
			return FALSE;
		}
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONDBLCLK:
		{
			if (((uMsg == WM_LBUTTONDBLCLK) && !gpSet->nTabBtnDblClickAction))
				break;

			if (nTabIdx >= 0)
			{
				CVConGuard VCon;
				// для меню нужны экранные координаты, получим их сразу, чтобы менюшка вплывала на клике
				// а то вдруг мышка уедет, во время активации таба...
				POINT ptCur = {0,0}; GetCursorPos(&ptCur);

				if (FarSendChangeTab(nTabIdx, &VCon))
				{
					BOOL lbCtrlPressed = isPressed(VK_CONTROL);

					if (uMsg == WM_LBUTTONDBLCLK)
					{
						switch (gpSet->nTabBtnDblClickAction)
						{
						case TabBtnDblClick::MaxRestoreWindow:
							// Чтобы клик случайно не провалился в консоль
							gpConEmu->mouse.state |= MOUSE_SIZING_DBLCKL;
							// Аналог AltF9
							gpConEmu->DoMaximizeRestore();
							break;
						case TabBtnDblClick::CloseTab:
							VCon->RCon()->CloseTab();
							break;
						case TabBtnDblClick::RestartTab:
							gpConEmu->mp_Menu->ExecPopupMenuCmd(tmp_None, VCon.VCon(), IDM_RESTART);
							break;
						case TabBtnDblClick::DuplicateTab:
							gpConEmu->mp_Menu->ExecPopupMenuCmd(tmp_None, VCon.VCon(), IDM_DUPLICATE);
							break;
						case TabBtnDblClick::MaxRestorePane:
							CVConGroup::PaneMaximizeRestore(VCon.VCon());
							break;
						case TabBtnDblClick::RenameTab:
							VCon->RCon()->DoRenameTab();
							break;
						#ifdef _DEBUG
						default:
							_ASSERTE(FALSE && "gpSet->nTabBtnDblClickAction: action was not processed");
						#endif
						}
					}
					else if (uMsg == WM_RBUTTONUP && !lbCtrlPressed)
					{
						gpConEmu->mp_Menu->ShowPopupMenu(VCon.VCon(), ptCur);
					}
					else
					{
						VCon->RCon()->CloseTab();
					}
				}
			}
			break;
		} // WM_MBUTTONUP, WM_RBUTTONUP, WM_LBUTTONDBLCLK
	}

	if (mn_LBtnDrag && !isPressed(VK_LBUTTON))
		EndTabDrag();

	return 0;
}

LRESULT CTabPanelBase::OnMouseToolbar(UINT uMsg, int nCmdId, int x, int y)
{
	switch (uMsg)
	{
		case WM_RBUTTONUP:
		{
			switch (nCmdId)
			{
				case TID_ACTIVE_NUMBER:
				{
					CVConGuard VCon;
					CVirtualConsole* pVCon = (gpConEmu->GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : nullptr;
					if (pVCon)
					{
						RECT rcBtnRect = {0};
						GetToolBtnRect(nCmdId, &rcBtnRect);
						POINT pt = {rcBtnRect.right,rcBtnRect.bottom};

						gpConEmu->mp_Menu->ShowPopupMenu(pVCon, pt, TPM_RIGHTALIGN|TPM_TOPALIGN);
					}
					break;
				}

				case TID_ALTERNATIVE:
				{
					break;
				}

				case TID_SCROLL:
				{
					break;
				}

				case TID_MINIMIZE:
				case TID_APPCLOSE:
				{
					Icon.HideWindowToTray();
					break;
				}

				case TID_MAXIMIZE:
				{
					gpConEmu->DoFullScreen();
					break;
				}

				case TID_CREATE_CON:
				{
					gpConEmu->RecreateAction(cra_CreateTab/*FALSE*/, TRUE);
					break;
				}

				case TID_SYSMENU:
				{
					CSettings::Dialog();
					break;
				}

				#ifdef _DEBUG
				default:
					_ASSERTE(FALSE && "Toolbar RClick was not processed");
				#endif
			}

			break;
		} // WM_RBUTTONUP
	}

	return 0;
}

bool CTabPanelBase::OnSetCursorRebar()
{
	TODO("Сделать выход из Maximized/Fullscreen драгом за таббар");
	if (gpSet->isTabs
		&& !(gpSet->isQuakeStyle && gpSet->wndCascade) // Quake можно двигать по горизонтали
		&& (gpConEmu->GetWindowMode() == wmNormal)
		&& gpConEmu->isCaptionHidden())
	{
		POINT ptCur = {};
		GetCursorPos(&ptCur);
		int nHitTest = GetTabFromPoint(ptCur);

		if (nHitTest == -1)
		{
			SetCursor(/*gpSet->isQuakeStyle ? gpConEmu->mh_CursorArrow :*/ gpConEmu->mh_CursorMove);
			return true;
		}
	}

	return false;
}
