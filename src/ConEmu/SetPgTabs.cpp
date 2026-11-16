
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

#include "ConEmu.h"
#include "OptionsClass.h"
#include "SetDlgLists.h"
#include "SetDlgFonts.h"
#include "SetPgFonts.h"
#include "SetPgTabs.h"
#include "TabBar.h"

CSetPgTabs::CSetPgTabs()
{
}

CSetPgTabs::~CSetPgTabs()
{
}

LRESULT CSetPgTabs::OnInitDialog(HWND hDlg, bool abInitial)
{
	checkRadioButton(hDlg, rbTabsAlways, rbTabsNone, (gpSet->isTabs == 2) ? rbTabsAuto : gpSet->isTabs ? rbTabsAlways : rbTabsNone);

	checkDlgButton(hDlg, cbTabsLocationBottom, (gpSet->nTabsLocation == 1) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbTabsUseWindowsAccent, gpSet->isTabsUseWindowsAccent);

	checkDlgButton(hDlg, cbOneTabPerGroup, gpSet->isOneTabPerGroup);

	checkDlgButton(hDlg, cbTabSelf, gpSet->isTabSelf);

	checkDlgButton(hDlg, cbTabRecent, gpSet->isTabRecent);

	checkDlgButton(hDlg, cbTabLazy, gpSet->isTabLazy);

	checkDlgButton(hDlg, cbHideInactiveConTabs, gpSet->bHideInactiveConsoleTabs);
	//checkDlgButton(hDlg, cbHideDisabledTabs, gpSet->bHideDisabledTabs);
	checkDlgButton(hDlg, cbShowFarWindows, gpSet->bShowFarWindows);

	SetDlgItemText(hDlg, tTabFontFace, gpSet->sTabFontFace);

	if (CSetDlgFonts::EnumFontsFinished())  // Если шрифты уже считаны
	{
		if (abInitial)
		{
			CSetPgFonts* pFonts = nullptr;
			if (gpSetCls->GetPageObj(pFonts))
			{
				pFonts->CopyFontsTo(hDlg, tTabFontFace, 0); // можно скопировать список с вкладки [thi_Fonts]
			}
		}
	}

	UINT nVal = gpSet->nTabFontHeight;
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tTabFontHeight), CSetDlgLists::eFSizesSmall, nVal, true);

	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tTabFontCharset), CSetDlgLists::eCharSets, gpSet->nTabFontCharSet, false);

	checkDlgButton(hDlg, cbMultiIterate, gpSet->isMultiIterate);

	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tTabBarDblClickAction), CSetDlgLists::eTabBarDblClickActions, gpSet->nTabBarDblClickAction);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, tTabBtnDblClickAction), CSetDlgLists::eTabBtnDblClickActions, gpSet->nTabBtnDblClickAction);

	SetDlgItemText(hDlg, tTabConsole, gpSet->szTabConsole);
	SetDlgItemText(hDlg, tTabModifiedSuffix, gpSet->szTabModifiedSuffix);
	SetDlgItemInt(hDlg, tTabFlashCounter, gpSet->nTabFlashChanged, TRUE);

	SetDlgItemText(hDlg, tTabSkipWords, gpSet->pszTabSkipWords ? gpSet->pszTabSkipWords : L"");
	SetDlgItemInt(hDlg, tTabLenMax, gpSet->nTabLenMax, FALSE);

	checkDlgButton(hDlg, cbAdminShield, gpSet->isAdminShield() ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbAdminSuffix, gpSet->isAdminSuffix() ? BST_CHECKED : BST_UNCHECKED);
	SetDlgItemText(hDlg, tAdminSuffix, gpSet->szAdminTitleSuffix);

	return 0;
}

INT_PTR CSetPgTabs::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case tTabFontFace:
	case tTabFontHeight:
	case tTabFontCharset:
	{
		if (code == CBN_EDITCHANGE)
		{
			switch (nCtrlId)
			{
			case tTabFontFace:
				GetDlgItemText(hDlg, nCtrlId, gpSet->sTabFontFace, countof(gpSet->sTabFontFace)); break;
			case tTabFontHeight:
				gpSet->nTabFontHeight = GetNumber(hDlg, nCtrlId); break;
			}
		}
		else if (code == CBN_SELCHANGE)
		{
			UINT val;
			INT_PTR nSel = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);

			switch (nCtrlId)
			{
			case tTabFontFace:
				SendDlgItemMessage(hDlg, nCtrlId, CB_GETLBTEXT, nSel, reinterpret_cast<LPARAM>(gpSet->sTabFontFace));
				break;
			case tTabFontHeight:
				if (CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eFSizesSmall, val))
					gpSet->nTabFontHeight = val;
				break;
			case tTabFontCharset:
				if (CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eCharSets, val))
					gpSet->nTabFontCharSet = val;
				else
					gpSet->nTabFontCharSet = DEFAULT_CHARSET;
			}
		}
		gpConEmu->RecreateControls(true, false, true);
		break;
	} // tTabFontFace, tTabFontHeight, tTabFontCharset

	case tTabBarDblClickAction:
	case tTabBtnDblClickAction:
	{
		if (code == CBN_SELCHANGE)
		{
			UINT val;
			INT_PTR nSel = SendDlgItemMessage(hDlg, nCtrlId, CB_GETCURSEL, 0, 0);

			switch(nCtrlId)
			{
			case tTabBarDblClickAction:
				if (CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eTabBarDblClickActions, val))
					gpSet->nTabBarDblClickAction = val;
				else
					gpSet->nTabBarDblClickAction = TABBAR_DEFAULT_CLICK_ACTION;
				break;
			case tTabBtnDblClickAction:
				if (CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eTabBtnDblClickActions, val))
					gpSet->nTabBtnDblClickAction = val;
				else
					gpSet->nTabBtnDblClickAction = TABBTN_DEFAULT_CLICK_ACTION;
				break;
			}
		}
		break;
	} // tTabBarDblClickAction, tTabBtnDblClickAction

	default:
		_ASSERTE(FALSE && "ListBox was not processed");
	}

	return 0;
}

LRESULT CSetPgTabs::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tTabSkipWords:
	{
		SafeFree(gpSet->pszTabSkipWords);
		gpSet->pszTabSkipWords = GetDlgItemTextPtr(hDlg, nCtrlId).Detach();
		gpConEmu->mp_TabBar->Update(TRUE);
		break;
	}

	case tTabConsole:
	case tTabModifiedSuffix:
	{
		wchar_t temp[MAX_PATH] = L"";

		if (GetDlgItemText(hDlg, nCtrlId, temp, countof(temp)) && temp[0])
		{
			temp[31] = 0; // JIC

			//03.04.2013, via gmail, просили не добавлять автоматом %s
			//if (wcsstr(temp, L"%s") || wcsstr(temp, L"%n"))
			switch (nCtrlId)
			{
			case tTabConsole:
				wcscpy_c(gpSet->szTabConsole, temp);
				break;
			case tTabModifiedSuffix:
				lstrcpyn(gpSet->szTabModifiedSuffix, temp, countof(gpSet->szTabModifiedSuffix));
				break;
			}

			gpConEmu->mp_TabBar->Update(TRUE);
		}
		break;
	} // case tTabConsole: case tTabViewer: case tTabEditor: case tTabEditorMod:

	case tTabLenMax:
	{
		BOOL lbOk = FALSE;
		DWORD n = GetDlgItemInt(hDlg, tTabLenMax, &lbOk, FALSE);

		if (n > 10 && n < CONEMUTABMAX)
		{
			gpSet->nTabLenMax = n;
			gpConEmu->mp_TabBar->Update(TRUE);
		}
		break;
	} // case tTabLenMax:

	case tTabFlashCounter:
	{
		GetDlgItemSigned(hDlg, tTabFlashCounter, gpSet->nTabFlashChanged, -1, 0);
		break;
	} // case tTabFlashCounter:

	case tAdminSuffix:
	{
		wchar_t szNew[64];
		GetDlgItemText(hDlg, tAdminSuffix, szNew, countof(szNew));
		if (lstrcmp(szNew, gpSet->szAdminTitleSuffix) != 0)
		{
			wcscpy_c(gpSet->szAdminTitleSuffix, szNew);
			gpConEmu->mp_TabBar->Update(TRUE);
		}
		break;
	} // case tAdminSuffix:

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
