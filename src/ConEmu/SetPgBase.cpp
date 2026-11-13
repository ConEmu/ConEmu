
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
#include "DarkMode.h"
#include "../common/MSetter.h"

#include "ConEmu.h"
#include "DlgItemHelper.h"
#include "DynDialog.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"
#include "OptionsHelp.h"
#include "RealConsole.h"
#include "SearchCtrl.h"
#include "SetDlgColors.h"
#include "SetPgApps.h"
#include "SetPgBase.h"
#include "SetPgDebug.h"
#include "SetPgFonts.h"
#include "SetPgKeys.h"

bool CSetPgBase::mb_IgnoreEditChanged = false;

CSetPgBase::CSetPgBase()
{
	mb_IgnoreEditChanged = false;
}

CSetPgBase::~CSetPgBase()
{
	if (mp_DpiAware)
	{
		mp_DpiAware->Detach();
		SafeDelete(mp_DpiAware);
	}
	SafeDelete(mp_DynDialog);
}

HWND CSetPgBase::CreatePage(ConEmuSetupPages* p, HWND ahParent, UINT anActivateTabMsg, const CDpiForDialog* apParentDpi)
{
	wchar_t szLog[80]; swprintf_c(szLog, L"Creating child dialog ID=%u", p->DialogID);
	LogString(szLog);

	SafeDelete(p->pPage);

	p->pPage = p->CreateObj();
	p->pPage->InitObject(ahParent, anActivateTabMsg, apParentDpi, p);

	p->pPage->mp_DynDialog = CDynDialog::ShowDialog(p->DialogID, ahParent, pageOpProc, reinterpret_cast<LPARAM>(p->pPage));
	p->hPage = p->pPage->Dlg();

	return p->hPage;
}

void CSetPgBase::InitObject(HWND ahParent, UINT anActivateTabMsg, const CDpiForDialog* apParentDpi, ConEmuSetupPages* apInfoPtr)
{
	mh_Parent = ahParent;
	mn_ActivateTabMsg = anActivateTabMsg;
	mp_ParentDpi = apParentDpi;
	mp_InfoPtr = apInfoPtr;

	if (apParentDpi)
	{
		CDpiForDialog::Create(mp_DpiAware);
		mb_DpiChanged = false;
	}
	else
	{
		SafeDelete(mp_DpiAware);
	}
}

void CSetPgBase::OnDpiChanged(const CDpiForDialog* apDpi)
{
	if (!this || !mh_Dlg || !mp_DpiAware || !apDpi)
		return;

	mb_DpiChanged = true;

	if (IsWindowVisible(mh_Dlg))
	{
		ProcessDpiChange(apDpi);
	}
}

void CSetPgBase::ProcessDpiChange(const CDpiForDialog* apDpi)
{
	if (!this || !mh_Dlg || !mp_DpiAware || !apDpi)
		return;

	HWND hPlace = GetDlgItem(mh_Parent, tSetupPagePlace);
	RECT rcClient; GetWindowRect(hPlace, &rcClient);
	MapWindowPoints(nullptr, mh_Parent, (LPPOINT)&rcClient, 2);

	mb_DpiChanged = false;
	mp_DpiAware->SetDialogDPI(apDpi->GetCurDpi(), &rcClient);

}

INT_PTR CSetPgBase::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	_ASSERTE(FALSE && "ComboBox was not processed!");
	return 0;
}

INT_PTR CSetPgBase::OnCtlColorStatic(HWND hDlg, HDC hdc, HWND hCtrl, WORD nCtrlId)
{
	if ((nCtrlId >= c0 && nCtrlId <= CSetDlgColors::MAX_COLOR_EDT_ID) || (nCtrlId >= c32 && nCtrlId <= c38))
	{
		return CSetDlgColors::ColorCtlStatic(hDlg, nCtrlId, hCtrl);
	}
	else if (CDlgItemHelper::isHyperlinkCtrl(nCtrlId))
	{
		_ASSERTE(hCtrl!=nullptr);
		// Check appropriate flags
		DWORD nStyle = GetWindowLong(hCtrl, GWL_STYLE);
		if (!(nStyle & SS_NOTIFY))
			SetWindowLong(hCtrl, GWL_STYLE, nStyle|SS_NOTIFY);
		// And the colors
		if (const auto brush = CSettings::SettingsThemeBrush(WM_CTLCOLORSTATIC, hdc, hCtrl, true))
			return brush;
		SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
		SetBkMode(hdc, TRANSPARENT);
		HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
		return (INT_PTR)hBrush;
	}

	/*
	SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
	HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
	SetBkMode((HDC)wParam, TRANSPARENT);
	return (INT_PTR)hBrush;
	*/

	return reinterpret_cast<INT_PTR>(DarkMode::ControlColor(hCtrl, hdc, WM_CTLCOLORSTATIC));
}

INT_PTR CSetPgBase::OnSetCursor(HWND hDlg, HWND hCtrl, WORD nCtrlId, WORD nHitTest, WORD nMouseMsg)
{
	if (nHitTest != HTCLIENT)
		return FALSE;

	#ifdef _DEBUG
	LONG_PTR wId = GetWindowLongPtr(hCtrl, GWLP_ID);
	#endif
	if (CDlgItemHelper::isHyperlinkCtrl(nCtrlId))
	{
		SetCursor(LoadCursor(nullptr, IDC_HAND));
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
		return TRUE;
	}

	return FALSE;
}

INT_PTR CSetPgBase::OnButtonClicked(HWND hDlg, HWND hBtn, WORD nCtrlId)
{
	return gpSetCls->OnButtonClicked(hDlg, MAKELONG(nCtrlId, BN_CLICKED), reinterpret_cast<LPARAM>(hBtn));
}

// Общая DlgProc на _все_ вкладки
INT_PTR CSetPgBase::pageOpProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR darkResult = 0;
	if (DarkMode::ProcessDialogMessage(hDlg, messg, wParam, lParam, darkResult)) return darkResult;
	TabHwndIndex pgId = thi_Last;
	CSetPgBase* pObj = nullptr;

	if (messg != WM_INITDIALOG)
	{
		ConEmuSetupPages* pPage = nullptr;
		pgId = gpSetCls->GetPageId(hDlg, &pPage);

		if ((pgId == thi_Last) || !pPage || !pPage->pPage)
		{
			_ASSERTE(FALSE && "Page was not created properly yet");
			return TRUE;
		}

		pObj = pPage->pPage;
		_ASSERTE(pObj && (pObj->mn_ActivateTabMsg != WM_APP));
		if (!pObj)
		{
			return TRUE;
		}

		if (pObj && pObj->mp_DpiAware && pObj->mp_DpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
		{
			return TRUE;
		}
	}

	if ((messg == WM_INITDIALOG) || (pObj && (messg == pObj->mn_ActivateTabMsg)))
	{
		bool bInitial = (messg == WM_INITDIALOG);

		if (bInitial)
		{
			if (!lParam)
			{
				_ASSERTE(lParam != 0);
				return 0;
			}
			pObj = (CSetPgBase*)lParam;
		}

		if (!pObj || (pObj->GetPageType() < thi_General) || (pObj->GetPageType() >= thi_Last))
		{
			_ASSERTE(pObj && (pObj->GetPageType() >= thi_General && pObj->GetPageType() < thi_Last));
			return 0;
		}
		_ASSERTE(pObj->Dlg() == nullptr || pObj->Dlg() == hDlg);

		pgId = pObj->GetPageType();

		if (bInitial)
		{
			_ASSERTE(pObj->mp_InfoPtr && pObj->mp_InfoPtr->PageIndex >= 0 && pObj->mp_InfoPtr->hPage == nullptr);
			pObj->mp_InfoPtr->hPage = hDlg;
			pObj->mh_Dlg = hDlg;

			CDynDialog* pDynDialog = CDynDialog::GetDlgClass(hDlg);
			_ASSERTE(pObj->mp_DynDialog==nullptr || pObj->mp_DynDialog==pDynDialog);

			#ifdef _DEBUG
			// pObj->mp_DynDialog is nullptr on first WM_INIT
			if (pObj->mp_DynDialog)
			{
				_ASSERTE(pObj->mp_DynDialog->mh_Dlg == hDlg);
			}
			#endif

			HWND hPlace = GetDlgItem(pObj->mh_Parent, tSetupPagePlace);
			RECT rcClient; GetWindowRect(hPlace, &rcClient);
			MapWindowPoints(nullptr, pObj->mh_Parent, (LPPOINT)&rcClient, 2);
			if (pObj->mp_DpiAware)
				pObj->mp_DpiAware->Attach(hDlg, pObj->mh_Parent, pDynDialog);
			MoveWindowRect(hDlg, rcClient);
		}
		else
		{
			_ASSERTE(pObj->mp_InfoPtr->PageIndex >= 0 && pObj->mp_InfoPtr->hPage == hDlg);
		}

		MSetter lockSelChange(&pObj->mb_SkipSelChange);

		pObj->OnInitDialog(hDlg, bInitial);

		if (bInitial)
		{
			EditIconHint_Subclass(hDlg, pObj->mh_Parent);
			gpSetCls->RegisterTipsFor(hDlg);
		}

		pObj->OnPostLocalize(hDlg);
		CSettings::ApplySettingsTheme(hDlg);

	}
	else if ((messg == WM_HELP) || (messg == HELP_WM_HELP))
	{
		_ASSERTE(messg == WM_HELP);
		if ((wParam == 0) && (lParam != 0))
		{
			// Open wiki page
			HELPINFO* hi = (HELPINFO*)lParam;
			if (hi->cbSize >= sizeof(HELPINFO))
			{
				CEHelpPopup::OpenSettingsWiki(hDlg, hi->iCtrlId);
			}
		}
		return TRUE;
	}
	else if (messg == WM_CTLCOLORSTATIC || messg == WM_CTLCOLORDLG || messg == WM_CTLCOLOREDIT
		|| messg == WM_CTLCOLORLISTBOX || messg == WM_CTLCOLORBTN || messg == WM_CTLCOLORSCROLLBAR)
	{
		const auto dc = reinterpret_cast<HDC>(wParam);
		const auto control = reinterpret_cast<HWND>(lParam);
		if (messg == WM_CTLCOLORSTATIC)
		{
			if (const auto brush = pObj->OnCtlColorStatic(hDlg, dc, control, GetDlgCtrlID(control)))
				return brush;
		}
		return CSettings::SettingsThemeBrush(messg, dc, control);
	}
	else if (pgId == thi_Apps)
	{
		// Страничка "App distinct" в некотором смысле особенная.
		// У многих контролов ИД дублируются с другими вкладками.
		CSetPgApps* pAppsPg;
		if (gpSetCls->GetPageObj(pAppsPg))
		{
			return pAppsPg->PageDlgProc(hDlg, messg, wParam, lParam);
		}
		else
		{
			_ASSERTE(pAppsPg!=nullptr);
			return 0;
		}
	}
	else if (pgId == thi_Integr)
	{
		return pObj->PageDlgProc(hDlg, messg, wParam, lParam);
	}
	else if (pgId == thi_Startup)
	{
		return pObj->PageDlgProc(hDlg, messg, wParam, lParam);
	}
	else
	// All other messages
	switch (messg)
	{
		#ifdef _DEBUG
		case WM_INITDIALOG:
			// Должно быть обработано выше
			_ASSERTE(messg != WM_INITDIALOG);
			break;
		#endif

		case WM_COMMAND:
		{
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				return CSetDlgButtons::OnButtonClicked(hDlg, wParam, lParam);

			case EN_CHANGE:
				// TODO: Remove duplicate condition!
				if (!pObj->mb_SkipSelChange && !pObj->mb_IgnoreEditChanged)
					pObj->OnEditChanged(hDlg, LOWORD(wParam));
				return 0;

			case CBN_EDITCHANGE:
			case CBN_SELCHANGE/*LBN_SELCHANGE*/:
				if (!pObj->mb_SkipSelChange)
					pObj->OnComboBox(hDlg, LOWORD(wParam), HIWORD(wParam));
				return 0;

			case LBN_DBLCLK:
				gpSetCls->OnListBoxDblClk(hDlg, wParam, lParam);
				return 0;

			case CBN_KILLFOCUS:
				if (gpSetCls->mn_LastChangingFontCtrlId && (LOWORD(wParam) == gpSetCls->mn_LastChangingFontCtrlId))
				{
					_ASSERTE(pgId == thi_Fonts);
					PostMessage(hDlg, gpSetCls->mn_MsgRecreateFont, gpSetCls->mn_LastChangingFontCtrlId, 0);
					gpSetCls->mn_LastChangingFontCtrlId = 0;
					return 0;
				}
				break;

			default:
				if (HIWORD(wParam) == 0xFFFF && LOWORD(wParam) == lbConEmuHotKeys)
				{
					dynamic_cast<CSetPgKeys*>(pObj)->OnHotkeysNotify(hDlg, wParam, 0);
				}
			} // switch (HIWORD(wParam))
		} // WM_COMMAND
		break;

		case WM_MEASUREITEM:
			return gpSetCls->OnMeasureFontItem(hDlg, messg, wParam, lParam);
		case WM_DRAWITEM:
			return gpSetCls->OnDrawFontItem(hDlg, messg, wParam, lParam);

		case WM_SETCURSOR:
			return pObj->OnSetCursor(hDlg, (HWND)wParam, GetDlgCtrlID((HWND)wParam), LOWORD(lParam), HIWORD(lParam));

		case WM_HSCROLL:
		{
			if ((pgId == thi_Backgr) && (HWND)lParam == GetDlgItem(hDlg, slDarker))
			{
				int newV = SendDlgItemMessage(hDlg, slDarker, TBM_GETPOS, 0, 0);

				if (newV != gpSet->bgImageDarker)
				{
					gpSetCls->SetBgImageDarker(newV, true);

					//gpSet->bgImageDarker = newV;
					//TCHAR tmp[10];
					//swprintf_c(tmp, L"%i", gpSet->bgImageDarker);
					//SetDlgItemText(hDlg, tDarker, tmp);

					//// Картинку может установить и плагин
					//if (gpSet->isShowBgImage && gpSet->sBgImage[0])
					//	gpSetCls->LoadBackgroundFile(gpSet->sBgImage);
					//else
					//	gpSetCls->NeedBackgroundUpdate();

					//gpConEmu->Update(true);
				}
			}
			else if ((pgId == thi_Transparent) && (HWND)lParam == GetDlgItem(hDlg, slTransparent))
			{
				int newV = SendDlgItemMessage(hDlg, slTransparent, TBM_GETPOS, 0, 0);

				if (newV != gpSet->nTransparent)
				{
					CSettings::checkDlgButton(hDlg, cbTransparent, (newV != MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
					gpSet->nTransparent = newV;
					if (!gpSet->isTransparentSeparate)
						SendDlgItemMessage(hDlg, slTransparentInactive, TBM_SETPOS, (WPARAM)true, static_cast<LPARAM>(gpSet->nTransparent));
					gpConEmu->OnTransparent();
				}
			}
			else if ((pgId == thi_Transparent) && (HWND)lParam == GetDlgItem(hDlg, slTransparentInactive))
			{
				int newV = SendDlgItemMessage(hDlg, slTransparentInactive, TBM_GETPOS, 0, 0);

				if (gpSet->isTransparentSeparate && (newV != gpSet->nTransparentInactive))
				{
					//checkDlgButton(hDlg, cbTransparentInactive, (newV!=MAX_ALPHA_VALUE) ? BST_CHECKED : BST_UNCHECKED);
					gpSet->nTransparentInactive = newV;
					gpConEmu->OnTransparent();
				}
			}
		} // WM_HSCROLL
		break;

		case WM_NOTIFY:
		{
			if (((NMHDR*)lParam)->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hDlg, messg, wParam, lParam);
			}
			else switch (((NMHDR*)lParam)->idFrom)
			{
			case lbActivityLog:
				if (!pObj->mb_SkipSelChange)
					return ((CSetPgDebug*)pObj)->OnActivityLogNotify(wParam, lParam);
				break;
			case lbConEmuHotKeys:
				if (!pObj->mb_SkipSelChange)
					return dynamic_cast<CSetPgKeys*>(pObj)->OnHotkeysNotify(hDlg, wParam, lParam);
				break;
			}
			return 0;
		} // WM_NOTIFY
		break;

		case WM_TIMER:

			if (wParam == BALLOON_MSG_TIMERID)
			{
				KillTimer(hDlg, BALLOON_MSG_TIMERID);
				SendMessage(gpSetCls->hwndBalloon, TTM_TRACKACTIVATE, FALSE, (LPARAM)&gpSetCls->tiBalloon);
				SendMessage(gpSetCls->hwndTip, TTM_ACTIVATE, TRUE, 0);
			}
			break;

		default:
		{
			if (messg == gpSetCls->mn_MsgRecreateFont)
			{
				gpSetCls->RecreateFont(LOWORD(wParam));
			}
			else if (messg == gpSetCls->mn_MsgLoadFontFromMain)
			{
				CSetPgFonts* pFonts = nullptr;
				if (gpSetCls->GetPageObj(pFonts))
				{
					if (pgId == thi_Views)
						pFonts->CopyFontsTo(hDlg, tThumbsFontName, tTilesFontName, 0);
					else if (pgId == thi_Tabs)
						pFonts->CopyFontsTo(hDlg, tTabFontFace, 0);
					else if (pgId == thi_Status)
						pFonts->CopyFontsTo(hDlg, tStatusFontFace, 0);
				}
			}
			else if (messg == gpSetCls->mn_MsgUpdateCounter)
			{
				gpSetCls->PostUpdateCounters(true);
			}
			else if (messg == DBGMSG_LOG_ID && pgId == thi_Debug)
			{
				MSetter lockSelChange(&pObj->mb_SkipSelChange);
				((CSetPgDebug*)pObj)->debugLog(wParam, lParam);
			} // if (messg == DBGMSG_LOG_ID && hDlg == gpSetCls->hDebug)
			else
			{
				pObj->PageDlgProc(hDlg, messg, wParam, lParam);
			}
		} // default:
	} //switch (messg)

	return 0;
}

void CSetPgBase::setCtrlTitleByHotkey(HWND hDlg, const WORD nCtrlId, const int iHotkeyId,
	const LPCWSTR pszFrom, const LPCWSTR pszTo, const LPCWSTR pszGroup)
{
	if (!hDlg || !nCtrlId || (nCtrlId == static_cast<WORD>(IDC_STATIC)) || (iHotkeyId <= 0))
	{
		_ASSERTE(FALSE && "Invalid identifiers");
		return;
	}

	wchar_t szKeyFull[128] = L"";
	gpSet->GetHotkeyNameById(iHotkeyId, szKeyFull, true);
	if (szKeyFull[0] == 0)
	{
		_ASSERTE(FALSE && "Failed to acquire HotKey");
		wcscpy_c(szKeyFull, CLngRc::getRsrc(lng_KeyNone/*"<None>"*/));

		EnableWindow(GetDlgItem(hDlg, nCtrlId), FALSE);
	}
	else if (pszGroup)
	{
		// "Ctrl+1" -> "Ctrl+Numbers"
		ApplyHotkeyGroupName(szKeyFull, pszGroup);
	}

	CEStr lsLoc;
	if (!CLngRc::getControl(nCtrlId, lsLoc))
	{
		if (GetString(hDlg, nCtrlId, &lsLoc.ms_Val) <= 0)
		{
			_ASSERTE(FALSE && "No title?");
			return;
		}
	}

	auto newLabel = ApplyHotkeyToTitle(lsLoc, pszFrom, pszTo, szKeyFull);
	// Update control text
	SetDlgItemText(hDlg, nCtrlId, newLabel);
}

void CSetPgBase::ApplyHotkeyGroupName(wchar_t(& szFull)[128], LPCWSTR pszGroup)
{
	if (szFull[0] == L'\0')
	{
		wcscpy_c(szFull, CLngRc::getRsrc(lng_KeyNone/*"<None>"*/));
		return;
	}

	_ASSERTE(szFull[wcslen(szFull) - 1] != L'+');
	_ASSERTE(pszGroup != nullptr);

	// Find the key ("Left") in "Ctrl+Alt+Left"
	auto* ptr = static_cast<wchar_t*>(wcsrchr(szFull, L'+'));

	if (ptr)
		*(ptr + 1) = L'\0';
	else
		szFull[0] = L'\0';

	if (pszGroup)
	{
		wcscat_c(szFull, pszGroup);
	}
}

CEStr CSetPgBase::ApplyHotkeyToTitle(CEStr& label, LPCWSTR pszFrom, LPCWSTR pszTo, LPCWSTR pszHotkey)
{
	if (pszFrom && *pszFrom)
	{
		// "Paste mode #1 (Shift+Ins)"
		LPCWSTR ptr2 = nullptr;
		const auto* ptr1 = wcsstr(label, pszFrom);
		if (!ptr1)
		{
			_ASSERTE(ptr1 && "pszFrom not found");
			return std::move(label);
		}
		ptr1 += wcslen(pszFrom);
		if (pszTo && *pszTo)
		{
			ptr2 = wcsstr(ptr1, pszTo);
		}
		if (!ptr2 || (ptr2 == ptr1))
		{
			_ASSERTE(ptr2 && (ptr2 != ptr1) && "Invalid source string");
			return std::move(label);
		}

		// Trim to hotkey beginning
		label.ms_Val[(ptr1 - label.ms_Val)] = 0;
		// And create new title
		return CEStr(label.ms_Val, pszHotkey, ptr2);
	}
	// ReSharper disable once CppRedundantElseKeywordInsideCompoundStatement
	else if (pszTo && *pszTo)
	{
		const auto* ptr = wcsstr(label, pszTo);
		if (!ptr)
		{
			_ASSERTE(ptr && "pszTo not found");
			return std::move(label);
		}

		// Create new title
		return CEStr(pszHotkey, ptr);
	}
	// ReSharper disable once CppRedundantElseKeyword
	else
	{
		_ASSERTE(FALSE && "Invalid anchors");
		return std::move(label);
	}
}
