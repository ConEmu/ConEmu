
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
#include "SetPgFar.h"
#include "TabBar.h"

CSetPgFar::CSetPgFar()
{
}

CSetPgFar::~CSetPgFar()
{
}

LRESULT CSetPgFar::OnInitDialog(HWND hDlg, bool abInitial)
{
	// Сначала - то что обновляется при активации вкладки

	// Списки
	UINT VkMod = gpSet->GetHotkeyById(vkLDragKey);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbLDragKey), CSetDlgLists::eKeys, VkMod, false);
	VkMod = gpSet->GetHotkeyById(vkRDragKey);
	CSetDlgLists::FillListBoxItems(GetDlgItem(hDlg, lbRDragKey), CSetDlgLists::eKeys, VkMod, false);

	if (!abInitial)
		return 0;

	checkDlgButton(hDlg, cbFARuseASCIIsort, gpSet->isFARuseASCIIsort);

	checkDlgButton(hDlg, cbShellNoZoneCheck, gpSet->isShellNoZoneCheck);

	checkDlgButton(hDlg, cbKeyBarRClick, gpSet->isKeyBarRClick);

	checkDlgButton(hDlg, cbFarWheelScrollback, gpSet->isFarWheelScrollback);
	checkDlgButton(hDlg, cbFarHourglass, gpSet->isFarHourglass);
	SetDlgItemInt(hDlg, tFarHourglass, gpSet->nFarHourglassDelay, false);

	checkDlgButton(hDlg, cbExtendUCharMap, gpSet->isExtendUCharMap);

	checkDlgButton(hDlg, cbDragL, (gpSet->isDragEnabled & DRAG_L_ALLOWED) ? BST_CHECKED : BST_UNCHECKED);
	checkDlgButton(hDlg, cbDragR, (gpSet->isDragEnabled & DRAG_R_ALLOWED) ? BST_CHECKED : BST_UNCHECKED);

	_ASSERTE(gpSet->isDropEnabled==0 || gpSet->isDropEnabled==1 || gpSet->isDropEnabled==2);
	checkDlgButton(hDlg, cbDropEnabled, gpSet->isDropEnabled);

	checkDlgButton(hDlg, cbDnDCopy, gpSet->isDefCopy);

	checkDlgButton(hDlg, cbDropUseMenu, gpSet->isDropUseMenu);

	// Overlay
	checkDlgButton(hDlg, cbDragImage, gpSet->isDragOverlay);

	checkDlgButton(hDlg, cbDragIcons, gpSet->isDragShowIcons);

	checkDlgButton(hDlg, cbRSelectionFix, gpSet->isRSelFix);

	checkDlgButton(hDlg, cbDragPanel, gpSet->isDragPanel);
	checkDlgButton(hDlg, cbDragPanelBothEdges, gpSet->isDragPanelBothEdges);

	_ASSERTE(gpSet->isDisableFarFlashing==0 || gpSet->isDisableFarFlashing==1 || gpSet->isDisableFarFlashing==2);
	checkDlgButton(hDlg, cbDisableFarFlashing, gpSet->isDisableFarFlashing);

	SetDlgItemText(hDlg, tTabPanels, gpSet->szTabPanels);
	SetDlgItemText(hDlg, tTabViewer, gpSet->szTabViewer);
	SetDlgItemText(hDlg, tTabEditor, gpSet->szTabEditor);
	SetDlgItemText(hDlg, tTabEditorMod, gpSet->szTabEditorModified);

	gpSetCls->CheckSelectionModifiers(hDlg);

	return 0;
}

INT_PTR CSetPgFar::OnComboBox(HWND hDlg, WORD nCtrlId, WORD code)
{
	switch (nCtrlId)
	{
	case lbLDragKey:
	{
		BYTE VkMod = 0;
		CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eKeys, VkMod);
		gpSet->SetHotkeyById(vkLDragKey, VkMod);
		break;
	}

	case lbRDragKey:
	{
		BYTE VkMod = 0;
		CSetDlgLists::GetListBoxItem(hDlg, nCtrlId, CSetDlgLists::eKeys, VkMod);
		gpSet->SetHotkeyById(vkLDragKey, VkMod);
		break;
	}

	default:
		_ASSERTE(FALSE && "ListBox was not processed");
	}

	return 0;
}

LRESULT CSetPgFar::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tTabPanels:
	case tTabViewer:
	case tTabEditor:
	case tTabEditorMod:
	{
		wchar_t temp[MAX_PATH] = L"";

		if (GetDlgItemText(hDlg, nCtrlId, temp, countof(temp)) && temp[0])
		{
			temp[31] = 0; // JIC

						  //03.04.2013, via gmail, просили не добавлять автоматом %s
						  //if (wcsstr(temp, L"%s") || wcsstr(temp, L"%n"))
			switch (nCtrlId)
			{
			case tTabPanels:
				wcscpy_c(gpSet->szTabPanels, temp);
				break;
			case tTabViewer:
				wcscpy_c(gpSet->szTabViewer, temp);
				break;
			case tTabEditor:
				wcscpy_c(gpSet->szTabEditor, temp);
				break;
			case tTabEditorMod:
				wcscpy_c(gpSet->szTabEditorModified, temp);
				break;
			}

			gpConEmu->mp_TabBar->Update(TRUE);
		}
		break;
	} // case tTabConsole: case tTabViewer: case tTabEditor: case tTabEditorMod:

	case tFarHourglass:
	{
		BOOL ok = FALSE;
		const UINT value = GetDlgItemInt(hDlg, tFarHourglass, &ok, FALSE);
		gpSet->nFarHourglassDelay = (ok && value) ? value : 2000;
		break;
	} // case tFarHourglass:

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
