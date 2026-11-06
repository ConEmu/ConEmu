
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

#ifndef HIDE_USE_EXCEPTION_INFO
#define HIDE_USE_EXCEPTION_INFO
#endif

#ifndef SHOWDEBUGSTR
#define SHOWDEBUGSTR
#endif

#include "../common/defines.h"
//
#include "Header.h"
//
#include "AboutDlg.h"
#include "Attach.h"
#include "ConEmu.h"
#include "FindDlg.h"
#include "LngRc.h"
#include "Macro.h"
#include "Menu.h"
#include "Menu.h"
#include "Options.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "SetColorPalette.h"
#include "SetCmdTask.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "Update.h"
#include "VConChild.h"
#include "VConGroup.h"
#include "VirtualConsole.h"
#include "../common/MSetter.h"
#include "../common/MToolTip.h"
#include "../common/WUser.h"


#define DEBUGSTRSIZE(s) //DEBUGSTR(s)
#define DEBUGSTRSYS(s) //DEBUGSTR(s)

static const wchar_t* sMenuHotkey = L"1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";


CConEmuMenu::CConEmuMenu()
	: mn_MsgOurSysCommand(gpConEmu->GetRegisteredMessage("UM_SYSCOMMAND")),
	m_phMenus({
	// Used in CConEmuMain

	// Popup's for SystemMenu
	&mh_SysDebugPopup,
	&mh_SysEditPopup,
	&mh_ActiveVConPopup,
	&mh_TerminateVConPopup,
	&mh_VConListPopup,
	&mh_HelpPopup,

	&mh_InsideSysMenu,

	// Used in VirtualConsole
	&mh_PopupMenu,
	&mh_TerminatePopup,
	&mh_RestartPopup,
	&mh_VConDebugPopup,
	&mh_VConEditPopup,
	&mh_VConViewPopup,
		})
{
}

CConEmuMenu::~CConEmuMenu()
{
	for (auto& phMenu : m_phMenus)
	{
		if (*phMenu)
		{
			DestroyMenu(*phMenu);
			*phMenu = nullptr;
		}
	}
}

// Returns previous value
bool CConEmuMenu::SetPassSysCommand(bool abPass /*= true*/)
{
	const bool result = mb_PassSysCommand;
	mb_PassSysCommand = abPass;
	return result;
}

bool CConEmuMenu::GetPassSysCommand() const
{
	return mb_PassSysCommand;
}

// Returns previous value
bool CConEmuMenu::SetInScMinimize(bool abInScMinimize)
{
	bool b = mb_InScMinimize;
	mb_InScMinimize = abInScMinimize;
	return b;
}

bool CConEmuMenu::GetInScMinimize() const
{
	return mb_InScMinimize;
}

bool CConEmuMenu::SetRestoreFromMinimized(bool abInRestore)
{
	bool b = mb_InRestoreFromMinimized;
	mb_InRestoreFromMinimized = abInRestore;
	return b;
}

bool CConEmuMenu::GetRestoreFromMinimized() const
{
	return mb_InRestoreFromMinimized;
}

TrackMenuPlace CConEmuMenu::SetTrackMenuPlace(TrackMenuPlace tmpPlace)
{
	TrackMenuPlace t = mn_TrackMenuPlace;
	mn_TrackMenuPlace = tmpPlace;
	return t;
}

TrackMenuPlace CConEmuMenu::GetTrackMenuPlace() const
{
	return mn_TrackMenuPlace;
}

bool CConEmuMenu::CreateOrUpdateMenu(HMENU& hMenu, const MenuItem* items, const size_t itemsCount)
{
	bool bNew = false;
	if (hMenu == nullptr)
	{
		bNew = true;
		hMenu = CreatePopupMenu();
	}

	for (size_t i = 0; i < itemsCount; ++i)
	{
		if (items[i].mit == mit_Separator)
		{
			if (bNew)
				AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
		}
		else if (bNew)
		{
			AppendMenu(hMenu,
				((items[i].mit == mit_Popup) ? MF_POPUP : MF_STRING) | items[i].Flags,
				items[i].MenuId,
				items[i].HotkeyId ? MenuAccel(items[i].HotkeyId, items[i].pszText) : items[i].pszText);
		}
		else
		{
			MENUITEMINFO mi = {sizeof(mi), MIIM_STRING};
			const CEStr lsItemText(items[i].HotkeyId ? MenuAccel(items[i].HotkeyId,items[i].pszText) : items[i].pszText);
			mi.dwTypeData = lsItemText.ms_Val;
			if (items[i].mit == mit_Popup)
				SetMenuItemInfo(hMenu, static_cast<UINT>(i), TRUE, &mi);
			else
				SetMenuItemInfo(hMenu, static_cast<UINT>(items[i].MenuId), FALSE, &mi);

			if (items[i].mit == mit_Command)
				EnableMenuItem(hMenu, static_cast<UINT>(items[i].MenuId), MF_BYCOMMAND|items[i].Flags);
			else if (items[i].mit == mit_Option)
				CheckMenuItem(hMenu, static_cast<UINT>(items[i].MenuId), MF_BYCOMMAND|items[i].Flags);
		}
	}

	return bNew;
}

void CConEmuMenu::OnNewConPopupMenu(POINT* ptWhere /*= nullptr*/, DWORD nFlags /*= 0*/, bool bShowTaskItems /*= false*/)
{
	mb_CmdShowTaskItems = bShowTaskItems;
	HMENU hPopup = CreatePopupMenu();
	LPCWSTR pszCurCmd = nullptr;
	if (!ptWhere && !nFlags)
	{
		if (gpConEmu->isTabsShown() && (gpSet->nTabsLocation == 1))
			nFlags = TPM_BOTTOMALIGN;
	}
	bool lbReverse = (nFlags & TPM_BOTTOMALIGN) == TPM_BOTTOMALIGN;

	CVConGuard VCon;

	if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
		pszCurCmd = VCon->RCon()->GetCmd();

	LPCWSTR pszHistory = gpSet->HistoryGet(0);
	int nInsertPos = lbReverse ? 0 : -1;
	mn_CmdLastID = 0;
	//int nFirstID = 0, nLastID = 0, nFirstGroupID = 0, nLastGroupID = 0;
	//int nCreateID = 0, nSetupID = 0, nResetID = 0;
	//bool bWasHistory = false;

	//memset(m_CmdPopupMenu, 0, sizeof(m_CmdPopupMenu));
	m_CmdTaskPopup.clear();

	bool bSeparator = true;
	CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};

	// "New console dialog..."
	itm.Reset(CmdTaskPopupItem::eNewDlg, ++mn_CmdLastID, MenuAccel(vkMultiNewConfirm,CLngRc::getRsrc(lng_MenuNewConsole/*"New console dialog..."*/)));
	InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
	m_CmdTaskPopup.push_back(itm);

	// Команда активной консоли
	if (pszCurCmd && *pszCurCmd)
	{
		itm.Reset(CmdTaskPopupItem::eCmd, ++mn_CmdLastID, pszCurCmd);
		itm.pszCmd = pszCurCmd;
		InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
		m_CmdTaskPopup.push_back(itm);
	}


	// Tasks begins
	{
		InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_SEPARATOR, 0, 0);

		// -- don't. let show current instance tasks only
		//// Обновить группы команд
		//gpSet->LoadCmdTasks(nullptr);

		int nGroup = 0, nCurGroupCount = 0;
		const CommandTasks* pGrp = nullptr;
		HMENU hCurPopup = hPopup;
		//const wchar_t* sMenuHotkey = L"1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		const int nMenuHotkeyMax = static_cast<int>(wcslen(sMenuHotkey));
		//bool bWasTasks = false;

		struct FolderInfo
		{
			wchar_t szFolderName[64];
			HMENU   hPopup;
			int     nGrpCount;
		};
		MArray<FolderInfo> Folders;
		FolderInfo flNew = {};
		CEStr szTempName;

		while ((pGrp = gpSet->CmdTaskGet(nGroup++)) != nullptr)
		{
			int nGrpCount = -1;
			LPCWSTR pszFolder;
			HMENU hGrpPopup = nullptr;

			LPCWSTR pszTaskName = pGrp->pszName ? pGrp->pszName : L"<nullptr>";

			if ((pszFolder = wcsstr(pszTaskName, L"::")) != nullptr)
			{
				// "Far::Latest", "Far::Far 1.7", "Build::ConEmu GUI", ...
				wchar_t szFolderName[64] = L"";
				const auto cchMaxFolderLen = std::min<size_t>(std::size(szFolderName) - 2, (pszFolder - pszTaskName + 1));
				lstrcpyn(szFolderName, pszTaskName, static_cast<int>(cchMaxFolderLen));
				wcscat_c(szFolderName, L"}");
				for (auto& fl : Folders)
				{
					if (lstrcmp(fl.szFolderName, szFolderName) == 0)
					{
						nGrpCount = (++fl.nGrpCount);
						hGrpPopup = fl.hPopup;
					}
				}

				if (!hGrpPopup)
				{
					// So create new popup "1: {Shells}" for example

					itm.Reset(CmdTaskPopupItem::eMore, -1);

					nCurGroupCount++;
					if (nCurGroupCount >= 1 && nCurGroupCount <= nMenuHotkeyMax)
					{
						swprintf_c(itm.szShort, L"&%c: ", sMenuHotkey[nCurGroupCount-1]);
						int iLen = lstrlen(itm.szShort);
						lstrcpyn(itm.szShort+iLen, szFolderName, countof(itm.szShort)-iLen);
					}
					else
					{
						lstrcpyn(itm.szShort, szFolderName, countof(itm.szShort));
					}

					itm.hPopup = CreatePopupMenu();
					if (!InsertMenu(hCurPopup, nInsertPos, MF_BYPOSITION|MF_POPUP|MF_STRING|MF_ENABLED, (UINT_PTR)itm.hPopup, itm.szShort))
						break;
					hGrpPopup = itm.hPopup;
					m_CmdTaskPopup.push_back(itm);

					ZeroStruct(flNew);
					wcscpy_c(flNew.szFolderName, szFolderName);
					flNew.hPopup = itm.hPopup;
					flNew.nGrpCount = nGrpCount = 1;
					Folders.push_back(flNew);
				}

				szTempName = CEStr(L"{", pszFolder + 2);
				pszTaskName = szTempName;
			}

			if (!hGrpPopup)
			{
				if ((nCurGroupCount++) >= MAX_CMD_GROUP_SHOW)
				{
					itm.Reset(CmdTaskPopupItem::eMore, -1);
					wcscpy_c(itm.szShort, CLngRc::getRsrc(lng_MenuMoreTasks/*"&More tasks"*/)); // separate, because "Reset" - escapes "&"
					itm.hPopup = CreatePopupMenu();
					if (!InsertMenu(hCurPopup, nInsertPos, MF_BYPOSITION|MF_POPUP|MF_STRING|MF_ENABLED, (UINT_PTR)itm.hPopup, itm.szShort))
						break;
					hCurPopup = itm.hPopup;
					m_CmdTaskPopup.push_back(itm);
					nCurGroupCount = 1;
				}
				nGrpCount = nCurGroupCount;
				hGrpPopup = hCurPopup;
			}

			// Next task
			itm.Reset(CmdTaskPopupItem::eTaskPopup, ++mn_CmdLastID);
			itm.pGrp = pGrp;
			//itm.pszCmd = nullptr; // pGrp->pszCommands; - don't show hint, there is SubMenu on RClick

			if (nGrpCount >= 1 && nGrpCount <= nMenuHotkeyMax)
			{
				swprintf_c(itm.szShort, L"&%c: ", sMenuHotkey[nGrpCount-1]);
			}
			else
			{
				itm.szShort[0] = 0;
				_ASSERTE(nGrpCount>=1 && nGrpCount<=nMenuHotkeyMax); // Too many tasks in one submenu?
			}

			wchar_t szHotkey[128];
			itm.SetShortName(pszTaskName, !mb_CmdShowTaskItems, pGrp->HotKey.GetHotkeyName(szHotkey, false));

			if (mb_CmdShowTaskItems)
			{
				itm.hPopup = CreatePopupMenu();
				InsertMenu(hGrpPopup, nInsertPos, MF_BYPOSITION|MF_POPUP|MF_STRING|MF_ENABLED, (UINT_PTR)itm.hPopup, itm.szShort);
			}
			else
			{
				InsertMenu(hGrpPopup, nInsertPos, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
			}
			m_CmdTaskPopup.push_back(itm);

			//bWasTasks = true;
		}
		//// was any task appended?
		//bSeparator = bWasTasks;

		itm.Reset(CmdTaskPopupItem::eSetupTasks, ++mn_CmdLastID, MenuAccel(vkWinAltT, CLngRc::getRsrc(lng_MenuSetupTasks/*"Setup tasks..."*/)));
		InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
		m_CmdTaskPopup.push_back(itm);
		bSeparator = true;
	}
	// Tasks ends

	// Is history present?
	if (pszHistory && *pszHistory)
	{
		if (bSeparator)
		{
			bSeparator = false;
			InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_SEPARATOR, 0, nullptr);
		}

		//bool bSeparator = false;
		int nCount = 0;
		while ((pszHistory = gpSet->HistoryGet(nCount)) && (nCount < MAX_CMD_HISTORY_SHOW))
		{
			// Текущий - будет первым
			if (!pszCurCmd || lstrcmp(pszCurCmd, pszHistory))
			{
				itm.Reset(CmdTaskPopupItem::eCmd, ++mn_CmdLastID, pszHistory);
				itm.pszCmd = pszHistory;

				InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_STRING|MF_ENABLED, itm.nCmd, itm.szShort);
				m_CmdTaskPopup.push_back(itm);
			}

			pszHistory += _tcslen(pszHistory)+1;
			nCount++;
		}

		itm.Reset(CmdTaskPopupItem::eClearHistory, ++mn_CmdLastID, CLngRc::getRsrc(lng_MenuClearHistory/*"Clear history..."*/));
		InsertMenu(hPopup, nInsertPos, MF_BYPOSITION|MF_STRING|MF_ENABLED, itm.nCmd, itm.szShort);
		m_CmdTaskPopup.push_back(itm);
	}


	RECT rcBtnRect = {0};
	LPRECT lpExcl = nullptr;
	DWORD nAlign = TPM_RIGHTALIGN|TPM_TOPALIGN;

	if (ptWhere)
	{
		rcBtnRect.right = ptWhere->x;
		rcBtnRect.bottom = ptWhere->y;
		if (nFlags)
			nAlign = nFlags;
	}
	else if (gpConEmu->mp_TabBar && gpConEmu->isTabsShown())
	{
		gpConEmu->mp_TabBar->Toolbar_GetBtnRect(TID_CREATE_CON, &rcBtnRect);
		lpExcl = &rcBtnRect;
	}
	else
	{
		GetClientRect(ghWnd, &rcBtnRect);
		MapWindowPoints(ghWnd, nullptr, (LPPOINT)&rcBtnRect, 2);
		rcBtnRect.left = rcBtnRect.right;
		rcBtnRect.bottom = rcBtnRect.top;
	}

	//mn_FirstTaskID = nFirstGroupID; mn_LastTaskID = nLastGroupID;

	mp_CmdRClickForce = nullptr;
	mb_InNewConRPopup = false; // JIC

	mb_InNewConPopup = true;
	int nId = trackPopupMenu(tmp_Cmd, hPopup, nAlign|TPM_RETURNCMD/*|TPM_NONOTIFY*/,
	                         rcBtnRect.right,rcBtnRect.bottom, ghWnd, lpExcl);
	mb_InNewConPopup = mb_InNewConRPopup = false;
	//ShowMenuHint(nullptr);

	if ((nId >= 1) || mp_CmdRClickForce)
	{
		if (mp_CmdRClickForce)
		{
			itm = *mp_CmdRClickForce;
		}
		else
		{
			itm.ItemType = CmdTaskPopupItem::eNone;
			for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
			{
				if (m_CmdTaskPopup[i].nCmd == nId)
				{
					itm = m_CmdTaskPopup[i];
					break;
				}
			}
		}

		if (itm.ItemType == CmdTaskPopupItem::eNewDlg)
		{
			gpConEmu->RecreateAction(gpSetCls->GetDefaultCreateAction(), TRUE);
		}
		else if (itm.ItemType == CmdTaskPopupItem::eSetupTasks)
		{
			CSettings::Dialog(IDD_SPG_TASKS);
		}
		else if (itm.ItemType == CmdTaskPopupItem::eClearHistory)
		{
			gpSetCls->ResetCmdHistory();
		}
		else if ((itm.ItemType == CmdTaskPopupItem::eCmd)
			|| (itm.ItemType == CmdTaskPopupItem::eTaskCmd)
			|| (itm.ItemType == CmdTaskPopupItem::eTaskAll)
			|| (itm.ItemType == CmdTaskPopupItem::eTaskPopup))
		{
			RConStartArgsEx con;
			if ((itm.ItemType == CmdTaskPopupItem::eTaskAll) || (itm.ItemType == CmdTaskPopupItem::eTaskPopup))
			{
				const CommandTasks* pGrp = (const CommandTasks*)itm.pGrp;
				con.pszSpecialCmd = lstrdup(pGrp->pszName).Detach();
				pGrp->ParseGuiArgs(&con);
				_ASSERTE(con.pszSpecialCmd && *con.pszSpecialCmd==TaskBracketLeft && con.pszSpecialCmd[lstrlen(con.pszSpecialCmd)-1]==TaskBracketRight);
			}
			else if (itm.ItemType == CmdTaskPopupItem::eTaskCmd)
			{
				const CommandTasks* pGrp = (const CommandTasks*)itm.pGrp;

				// May be directory was set in task properties?
				pGrp->ParseGuiArgs(&con);

				// Task pre-options, for example ">*cmd"
				con.pszSpecialCmd = lstrdup(gpConEmu->ParseScriptLineOptions(itm.pszCmd, nullptr, &con)).Detach();

				con.ProcessNewConArg();

				_ASSERTE(con.pszSpecialCmd && *con.pszSpecialCmd);
			}
			else if (itm.ItemType == CmdTaskPopupItem::eCmd)
			{
				_ASSERTE(itm.pszCmd && (*itm.pszCmd != L'>'));

				con.pszSpecialCmd = lstrdup(itm.pszCmd).Detach();

				_ASSERTE(con.pszSpecialCmd && *con.pszSpecialCmd);
			}

			if (isPressed(VK_SHIFT))
			{
				int nRc = gpConEmu->RecreateDlg(&con);

				if (nRc != IDC_START)
					return;

				CVConGroup::Redraw();
			}
			// Issue 1564: Add tasks to history too
			else if ((itm.ItemType == CmdTaskPopupItem::eCmd) || (itm.ItemType == CmdTaskPopupItem::eTaskPopup))
			{
				gpSet->HistoryAdd(con.pszSpecialCmd);
			}

			//Собственно, запуск
			if ((gpSetCls->IsMulti() || !gpConEmu->isVConExists(0))
					&& (con.aRecreate != cra_CreateWindow))
			{
				gpConEmu->CreateCon(con, true);
			}
			else
			{
				gpConEmu->CreateWnd(con);
			}
		}
	}

	// Release handles
	for (auto& taskMenuItem : m_CmdTaskPopup)
	{
		if (taskMenuItem.hPopup)
			DestroyMenu(taskMenuItem.hPopup);
		taskMenuItem.pszTaskBuf.Release();
	}
	DestroyMenu(hPopup);
}

void CConEmuMenu::OnNewConPopupMenuRClick(HMENU hMenu, UINT nItemPos)
{
	if (!mb_InNewConPopup || mb_InNewConRPopup)
		return;
	_ASSERTE(mn_TrackMenuPlace == tmp_Cmd);

	MENUITEMINFO mi = {sizeof(mi)};
	mi.fMask = MIIM_ID|MIIM_FTYPE|MIIM_SUBMENU;
	if (!GetMenuItemInfo(hMenu, nItemPos, TRUE, &mi))
		return;

	wchar_t szClass[128] = {};
	POINT ptCur = {}; GetCursorPos(&ptCur);
	HWND hMenuWnd = WindowFromPoint(ptCur);
	GetClassName(hMenuWnd, szClass, countof(szClass));
	if (lstrcmp(szClass, L"#32768") != 0)
		hMenuWnd = nullptr;

	int nId = (int)mi.wID;
	HMENU hPopup = mi.hSubMenu;

	//if (mn_LastTaskID < 0 || nId > (UINT)mn_LastTaskID)
	//	return;
	CmdTaskPopupItem* itm = nullptr;
	for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
	{
		if  ((mb_CmdShowTaskItems && (m_CmdTaskPopup[i].hPopup && m_CmdTaskPopup[i].hPopup == hPopup))
			|| (!mb_CmdShowTaskItems && (m_CmdTaskPopup[i].nCmd == nId)))
		{
			itm = &(m_CmdTaskPopup[i]);
			break;
		}
	}
	// Проверяем, по чему щелкнули
	if (!itm || itm->ItemType != CmdTaskPopupItem::eTaskPopup)
		return;

	// Если таски созданы были как SubMenu?
	if (mb_CmdShowTaskItems)
	{
		// Need to close parentmenu
		if (hMenuWnd)
		{
			HWND hParent = GetParent(hMenuWnd);
			wchar_t szClass[100];
			GetClassName(hParent, szClass, countof(szClass));
			// Если таски были открыты через "More tasks" - нужно закрыть и родительские субменю
			_ASSERTE(hParent == ghWnd);

			mp_CmdRClickForce = itm;
			PostMessage(hMenuWnd, WM_CLOSE, 0, 0);
		}
	}
	else
	{
		const CommandTasks* pGrp = (const CommandTasks*)itm->pGrp;
		if (!pGrp || !pGrp->pszCommands || !*pGrp->pszCommands)
			return;

		// Поехали
		HMENU hPopup = CreatePopupMenu();
		int nLastID = FillTaskPopup(hPopup, itm);

		int nRetID = -1;

		if (nLastID > 0)
		{
			RECT rcMenuItem = {};
			GetMenuItemRect(ghWnd, hMenu, nItemPos, &rcMenuItem);

			mb_InNewConRPopup = true;
			nRetID = trackPopupMenu(tmp_CmdPopup, hPopup, TPM_RETURNCMD|TPM_NONOTIFY|TPM_RECURSE,
								 rcMenuItem.right,rcMenuItem.top, ghWnd, &rcMenuItem);
			mb_InNewConRPopup = false;
		}

		DestroyMenu(hPopup);

		CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};
		if (nRetID > 0)
		{
			for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
			{
				if (m_CmdTaskPopup[i].nCmd == nRetID)
				{
					itm = m_CmdTaskPopup[i];
					break;
				}
			}
		}

		if (itm.ItemType == CmdTaskPopupItem::eTaskCmd)
		{
			// Need to close parentmenu
			if (hMenuWnd)
			{
				PostMessage(hMenuWnd, WM_CLOSE, 0, 0);
			}

			RConStartArgsEx con;

			// May be directory was set in task properties?
			pGrp->ParseGuiArgs(&con);

			LPCWSTR pszCmd = gpConEmu->ParseScriptLineOptions(itm.pszCmd, nullptr, &con);

			// Well, start selected line from Task
			con.pszSpecialCmd = lstrdup(pszCmd).Detach();
			if (!con.pszSpecialCmd)
			{
				_ASSERTE(con.pszSpecialCmd!=nullptr);
			}
			else
			{
				con.ProcessNewConArg();

				if (isPressed(VK_SHIFT))
				{
					int nRc = gpConEmu->RecreateDlg(&con);

					if (nRc != IDC_START)
						return;

					CVConGroup::Redraw();
				}
				//Don't add task subitems to history
				//else
				//{
				//	gpSet->HistoryAdd(con.pszSpecialCmd);
				//}

				//Собственно, запуск
				if (gpSetCls->IsMulti())
					gpConEmu->CreateCon(con, true);
				else
					gpConEmu->CreateWnd(con);
			}
		}

		//SafeFree(pszDataW);
	}
}

bool CConEmuMenu::OnMenuSelected_NewCon(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (!mb_InNewConPopup)
	{
		_ASSERTE(mb_InNewConPopup);
		return false;
	}

	CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};
	for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
	{
		if (m_CmdTaskPopup[i].nCmd == nID)
		{
			itm = m_CmdTaskPopup[i];
			break;
		}
	}

	if (itm.ItemType == CmdTaskPopupItem::eCmd && itm.pszCmd)
	{
		LPCWSTR pszCmd = itm.pszCmd;
		if (itm.szShort[0] && pszCmd && lstrcmp(pszCmd, itm.szShort))
		{
			POINT pt; GetCursorPos(&pt);
			RECT rcMenuItem = {};
			BOOL lbMenuItemPos = FALSE;
			if (nFlags & MF_POPUP)
			{
				lbMenuItemPos = GetMenuItemRect(ghWnd, hMenu, nID, &rcMenuItem);
			}
			else
			{
				for (int i = 0; i < 100; i++)
				{
					if (GetMenuItemID(hMenu, i) == nID)
					{
						lbMenuItemPos = GetMenuItemRect(ghWnd, hMenu, i, &rcMenuItem);
						break;
					}
				}
			}
			if (lbMenuItemPos)
			{
				pt.x = rcMenuItem.left + (rcMenuItem.bottom - rcMenuItem.top)*2; //(rcMenuItem.left + rcMenuItem.right) >> 1;
				pt.y = rcMenuItem.bottom;
			}

			ShowMenuHint(pszCmd, &pt);
		}
		else
		{
			ShowMenuHint(nullptr);
		}
	}
	else
	{
		ShowMenuHint(nullptr);
	}
	return true;
}


bool CConEmuMenu::OnMenuSelected(HMENU hMenu, WORD nID, WORD nFlags)
{
	bool bRc = true;

	wchar_t szInfo[80];
	swprintf_c(szInfo, L"OnMenuSelected selected, ID=%u, Flags=x%04X", nID, nFlags);
	LogString(szInfo);

	switch (mn_TrackMenuPlace)
	{
	case tmp_Cmd:
		bRc = OnMenuSelected_NewCon(hMenu, nID, nFlags);
		break;
	case tmp_System:
	case tmp_VCon:
		ShowMenuHint(hMenu, nID, nFlags);
		break;
	case tmp_KeyBar:
		ShowKeyBarHint(hMenu, nID, nFlags);
		break;
	case tmp_StatusBarCols:
		gpConEmu->mp_Status->ProcessMenuHighlight(hMenu, nID, nFlags);
		break;
	case tmp_None:
		bRc = false;
		break;
	default:
		// Unknown menu type?
		bRc = false; // Take no action
	}

	return bRc;
}


void CConEmuMenu::OnMenuRClick(HMENU hMenu, UINT nItemPos)
{
	switch (mn_TrackMenuPlace)
	{
	case tmp_Cmd:
		OnNewConPopupMenuRClick(hMenu, nItemPos);
		break;
	default:
		; // Unsupported in this menu type?
	}
}

void CConEmuMenu::ShowEditMenu(CVirtualConsole* apVCon, POINT ptCur, DWORD Align)
{
	if (!apVCon)
		return;

	const auto hVConEditPopup = CreateEditMenuPopup(apVCon);

	int nCmd = trackPopupMenu(tmp_VCon, hVConEditPopup,
	                          Align | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
	                          ptCur.x, ptCur.y, ghWnd);

	if (nCmd)
	{
		ExecPopupMenuCmd(tmp_VCon, apVCon, nCmd);
	}

	DestroyMenu(hVConEditPopup);
}

// Показать контекстное меню для ТЕКУЩЕЙ закладки консоли
// ptCur - экранные координаты
void CConEmuMenu::ShowPopupMenu(CVirtualConsole* apVCon, POINT ptCur, DWORD Align /* = TPM_LEFTALIGN */, bool abFromConsole /*= false*/)
{
	CVConGuard guard(apVCon);
	BOOL lbNeedCreate = (mh_PopupMenu == nullptr);

	#if 0
	// -- dummy code, TPM_LEFTALIGN==0
	if (!Align)
		Align = TPM_LEFTALIGN;
	#endif

	// Создать или обновить enable/disable
	mh_PopupMenu = CreateVConPopupMenu(apVCon, mh_PopupMenu, true, abFromConsole);
	if (!mh_PopupMenu)
	{
		MBoxAssert(mh_PopupMenu!=nullptr);
		return;
	}

	if (lbNeedCreate)
	{
		AppendMenu(mh_PopupMenu, MF_SEPARATOR, 0, 0);

		_ASSERTE(mh_VConEditPopup == nullptr);
		mh_VConEditPopup = CreateEditMenuPopup(apVCon);
		AppendMenu(mh_PopupMenu, MF_POPUP|MF_ENABLED, (UINT_PTR)mh_VConEditPopup, CLngRc::getRsrc(lng_MenuPopupEdit/*"Ed&it"*/));

		_ASSERTE(mh_VConViewPopup == nullptr);
		mh_VConViewPopup = CreateViewMenuPopup(apVCon);
		AppendMenu(mh_PopupMenu, MF_POPUP|MF_ENABLED, (UINT_PTR)mh_VConViewPopup, CLngRc::getRsrc(lng_MenuPopupPalettes/*"&View (palettes)"*/));

		_ASSERTE(mh_VConDebugPopup == nullptr);
		mh_VConDebugPopup = CreateDebugMenuPopup();
		AppendMenu(mh_PopupMenu, MF_POPUP|MF_ENABLED, (UINT_PTR)mh_VConDebugPopup, CLngRc::getRsrc(lng_MenuPopupDebug/*"&Debug"*/));
	}
	else
	{
		// обновить enable/disable пунктов меню
		CreateEditMenuPopup(apVCon, mh_VConEditPopup);
		// имена палитр и флажок текущей
		CreateViewMenuPopup(apVCon, mh_VConViewPopup);
	}

	// Некузяво. Может вслыть тултип под меню
	//ptCur.x++; ptCur.y++; // чтобы меню можно было сразу закрыть левым кликом.

	mb_FromConsoleMenu = abFromConsole;

	int nCmd = trackPopupMenu(tmp_VCon, mh_PopupMenu,
	                          Align | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
	                          ptCur.x, ptCur.y, ghWnd);

	if (!nCmd)
		return; // отмена

	ExecPopupMenuCmd(tmp_VCon, apVCon, nCmd);

	mb_FromConsoleMenu = false;
}

void CConEmuMenu::ExecPopupMenuCmd(TrackMenuPlace place, CVirtualConsole* apVCon, int nCmd)
{
	CVConGuard guard;
	if (!guard.Attach(apVCon))
		return;

	switch (nCmd)
	{
		case IDM_CLOSE:
			if (!mb_FromConsoleMenu && gpSet->isOneTabPerGroup && CVConGroup::isGroup(apVCon))
				CVConGroup::CloseGroup(apVCon);
			else
				apVCon->RCon()->CloseTab();
			break;
		case IDM_DETACH:
			apVCon->RCon()->DetachRCon();
			break;
		case IDM_UNFASTEN:
			apVCon->RCon()->Unfasten();
			break;
		case IDM_RENAMETAB:
			apVCon->RCon()->DoRenameTab();
			break;
		case IDM_CHANGEAFFINITY:
			apVCon->RCon()->ChangeAffinityPriority();
			break;
		case IDM_DUPLICATE:
		case IDM_ADMIN_DUPLICATE:
			if ((nCmd == IDM_ADMIN_DUPLICATE) || isPressed(VK_SHIFT))
				apVCon->RCon()->AdminDuplicate();
			else
				apVCon->RCon()->DuplicateRoot();
			break;
		case IDM_SPLIT2RIGHT:
		case IDM_SPLIT2BOTTOM:
		{
			CEStr pszMacro = lstrdup((nCmd == IDM_SPLIT2RIGHT) ? L"Split(0,50,0)" : L"Split(0,0,50)");
			CEStr pszRc = ConEmuMacro::ExecuteMacro(std::move(pszMacro), apVCon->RCon());
			break;
		}
		case IDM_CHILDSYSMENU:
			apVCon->RCon()->ChildSystemMenu();
			break;

		case IDM_TERMINATEPRC:
			// Active console/pane: Terminate active process
			apVCon->RCon()->TerminateActiveProcess(isPressed(VK_SHIFT), 0);
			break;
		case IDM_TERMINATEBUTSHELL:
			// Active console/pane: Terminate all except root process
			apVCon->RCon()->TerminateAllButShell(isPressed(VK_SHIFT));
			break;
		case IDM_TERMINATECON:
			// Active console/pane: Do normal close
			apVCon->RCon()->CloseConsole(false, true/*confimation may be disabled in settings*/);
			break;

		case IDM_TERMINATEGROUP:
			// Active group: Do normal close of each pane
		case IDM_TERMINATEPRCGROUP:
			// Active group: Terminate active process in each pane
			CVConGroup::CloseGroup(apVCon, (nCmd==IDM_TERMINATEPRCGROUP));
			break;

		case IDM_TERMINATEALLCON:
			// Do normal close of all consoles (tabs and panes)
			CVConGroup::CloseAllButActive(nullptr, CVConGroup::CloseSimple, false);
			break;
		case IDM_TERMINATECONEXPT:
			// Close all tabs and panes except active console/pane
			CVConGroup::CloseAllButActive(apVCon, CVConGroup::CloseSimple, false);
			break;
		case IDM_TERMINATEZOMBIES:
			// Close ‘zombies’ (where ‘Press Esc to close console’ is displayed)
			CVConGroup::CloseAllButActive(nullptr, CVConGroup::CloseZombie, false);
			break;
		case IDM_TERMINATE2RIGHT:
			// Terminate -> Close to the right
			CVConGroup::CloseAllButActive(apVCon, CVConGroup::Close2Right, false);
			break;

		case IDM_RESTART:
		case IDM_RESTARTAS:
		case IDM_RESTARTDLG:
			if (apVCon->isActive(false))
			{
				gpConEmu->RecreateAction(cra_RecreateTab/*TRUE*/, (nCmd==IDM_RESTARTDLG) || isPressed(VK_SHIFT), (nCmd==IDM_RESTARTAS) ? crb_On : crb_Undefined);
			}
			else
			{
				MBoxAssert(apVCon->isActive(false));
			}

			break;

		case ID_NEWCONSOLE:
			gpConEmu->RecreateAction(gpSetCls->GetDefaultCreateAction(), true);
			break;
		case IDM_ATTACHTO:
			OnSysCommand(ghWnd, IDM_ATTACHTO, 0);
			break;

		//case IDM_SAVE:
		//	apVCon->RCon()->PostMacro(L"F2");
		//	break;
		//case IDM_SAVEALL:
		//	apVCon->RCon()->PostMacro(gpSet->sSaveAllMacro);
		//	break;

		default:
			if ((place == tmp_VCon) && (nCmd >= ID_CON_SETPALETTE_FIRST) && (nCmd <= ID_CON_SETPALETTE_LAST))
			{
				apVCon->ChangePalette(nCmd - ID_CON_SETPALETTE_FIRST);
			}
			else if (nCmd >= IDM_VCONCMD_FIRST && nCmd <= IDM_VCONCMD_LAST)
			{
				_ASSERTE(FALSE && "Unhandled command!");
			}
			else if (nCmd >= 0xAB00)
			{
				// "Системные" команды, обрабатываемые в CConEmu
				OnSysCommand(ghWnd, nCmd, 0);
			}
	}
}

HMENU CConEmuMenu::GetSysMenu(BOOL abInitial /*= FALSE*/)
{
	HMENU hwndMain = nullptr;
	//MENUITEMINFO mi = {sizeof(mi)};
	//wchar_t szText[255];


	if (gpConEmu->mp_Inside || gpSet->isQuakeStyle)
	{
		if (!mh_InsideSysMenu || abInitial)
		{
			if (mh_InsideSysMenu)
				DestroyMenu(mh_InsideSysMenu);

			mh_InsideSysMenu = CreatePopupMenu();
			AppendMenu(mh_InsideSysMenu, MF_STRING|MF_ENABLED, SC_CLOSE, MenuAccel(vkCloseConEmu,L"&Close ConEmu"));
		}
		hwndMain = mh_InsideSysMenu;
	}
	else
	{
		hwndMain = ::GetSystemMenu(ghWnd, FALSE);

		//// "Alt+F4" для пункта "Close" смысла не имеет
		//mi.fMask = MIIM_STRING; mi.dwTypeData = szText; mi.cch = countof(szText);
		//if (GetMenuItemInfo(hwndMain, SC_CLOSE, FALSE, &mi))
		//{
		//	wchar_t* psz = wcschr(szText, L'\t');
		//	if (psz)
		//	{
		//		*psz = 0;
		//		SetMenuItemInfo(hwndMain, SC_CLOSE, FALSE, &mi);
		//	}
		//}
	}

	UpdateSysMenu(hwndMain);

	return hwndMain;
}

void CConEmuMenu::UpdateSysMenu(HMENU hSysMenu)
{
	MENUITEMINFO mi = {sizeof(mi)};
	wchar_t szText[255];

	// "Alt+F4" для пункта "Close" смысла не имеет
	mi.fMask = MIIM_STRING; mi.dwTypeData = szText; mi.cch = countof(szText);
	if (GetMenuItemInfo(hSysMenu, SC_CLOSE, FALSE, &mi))
	{
		wchar_t* psz = wcschr(szText, L'\t');
		if (psz)
		{
			*psz = 0;
			//SetMenuItemInfo(hSysMenu, SC_CLOSE, FALSE, &mi);
		}
		mi.dwTypeData = (LPWSTR)MenuAccel(vkCloseConEmu,szText);
		if (lstrcmp(mi.dwTypeData, szText) != 0)
		{
			SetMenuItemInfo(hSysMenu, SC_CLOSE, FALSE, &mi);
		}
	}

	// В результате работы некоторых недобросовествных программ может сбиваться настроенное системное меню
	mi.fMask = MIIM_STRING; mi.dwTypeData = szText; mi.cch = countof(szText);
	if (!GetMenuItemInfo(hSysMenu, ID_NEWCONSOLE, FALSE, &mi))
	{
		if (!gpConEmu->mp_Inside)
		{
			if (!gpSet->isQuakeStyle)
				InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_TOMONITOR, MenuAccel(vkJumpActiveMonitor, CLngRc::getRsrc(lng_MenuBringHere/*"Bring &here"*/)));
			InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_TOTRAY, TRAY_ITEM_HIDE_NAME/* L"Hide to &TSA" */);
		}
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0);

		//InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_ABOUT, _T("&About / Help"));
		if (mh_HelpPopup) DestroyMenu(mh_HelpPopup);
		mh_HelpPopup = CreateHelpMenuPopup();
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_HelpPopup, CLngRc::getRsrc(lng_MenuPopupHelp/*"Hel&p"*/));
		//if (ms_ConEmuChm[0])  //Показывать пункт только если есть conemu.chm
		//	InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_HELP, _T("&Help"));

		// --------------------
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0);

		if (mh_SysDebugPopup) DestroyMenu(mh_SysDebugPopup);
		mh_SysDebugPopup = CreateDebugMenuPopup();
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_SysDebugPopup, CLngRc::getRsrc(lng_MenuPopupSysDebug/*"&Debug"*/));

		if (mh_SysEditPopup) DestroyMenu(mh_SysEditPopup);
		mh_SysEditPopup = CreateEditMenuPopup(nullptr);
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_SysEditPopup, CLngRc::getRsrc(lng_MenuPopupSysEdit/*"&Edit"*/));

		// --------------------
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0);

		if (mh_VConListPopup) DestroyMenu(mh_VConListPopup);
		mh_VConListPopup = CreateVConListPopupMenu(mh_VConListPopup, TRUE/*abFirstTabOnly*/);
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_VConListPopup, CLngRc::getRsrc(lng_MenuPopupConList/*"Console &list"*/));

		if (mh_ActiveVConPopup) DestroyMenu(mh_ActiveVConPopup);
		if (mh_TerminateVConPopup) { DestroyMenu(mh_TerminateVConPopup); mh_TerminateVConPopup = nullptr; }
		mh_ActiveVConPopup = CreateVConPopupMenu(nullptr, nullptr, false, false);
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_POPUP|MF_ENABLED, (UINT_PTR)mh_ActiveVConPopup, CLngRc::getRsrc(lng_MenuPopupActiveCon/*"Acti&ve console"*/));

		// --------------------
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, 0);
		if (!gpConEmu->mp_Inside)
		{
			InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED|(gpSet->isAlwaysOnTop ? MF_CHECKED : 0),
				ID_ALWAYSONTOP, MenuAccel(vkAlwaysOnTop, CLngRc::getRsrc(lng_MenuAlwaysOnTop/*"Al&ways on top"*/)));
		}
		#ifdef SHOW_AUTOSCROLL
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED|(gpSetCls->AutoScroll ? MF_CHECKED : 0),
			ID_AUTOSCROLL, CLngRc::getRsrc(lng_MenuAutoScroll/*"Auto scro&ll"*/));
		#endif
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_SETTINGS,   MenuAccel(vkWinAltP, CLngRc::getRsrc(lng_MenuSettings/*"Sett&ings..."*/)));
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, IDM_ATTACHTO,  MenuAccel(vkMultiNewAttach, CLngRc::getRsrc(lng_MenuSysAttachTo/*"Attach t&o..."*/)));
		InsertMenu(hSysMenu, 0, MF_BYPOSITION|MF_STRING|MF_ENABLED, ID_NEWCONSOLE, MenuAccel(vkMultiNewConfirm, CLngRc::getRsrc(lng_MenuSysNewConsole/*"New console dialog..."*/)));
	}
}

POINT CConEmuMenu::CalcTabMenuPos(CVirtualConsole* apVCon)
{
	POINT ptCur = {};
	if (apVCon)
	{
		RECT rcWnd;
		if (gpConEmu->mp_TabBar && gpConEmu->isTabsShown())
		{
			gpConEmu->mp_TabBar->GetActiveTabRect(&rcWnd);
			ptCur.x = rcWnd.left;
			ptCur.y = rcWnd.bottom;
		}
		else
		{
			CVConGuard VCon;
			if (gpConEmu->GetActiveVCon(&VCon) >= 0)
			{
				GetWindowRect(VCon->GetView(), &rcWnd);
			}
			else
			{
				_ASSERTE(FALSE && "No Active VCon");
				GetWindowRect(ghWnd, &rcWnd);
			}

			ptCur.x = rcWnd.left;
			ptCur.y = rcWnd.top;
		}
	}
	return ptCur;
}

int CConEmuMenu::FillTaskPopup(HMENU hMenu, CmdTaskPopupItem* pParent)
{
	const CommandTasks* pGrp = static_cast<const CommandTasks*>(pParent->pGrp);

	CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};

	//itm.Reset(CmdTaskPopupItem::eTask, ++mn_CmdLastID, L"Run all commands");
	//itm.pGrp = pGrp;

	int nCount = 0;
	const ssize_t nMenuHotkeyMax = _tcslen(sMenuHotkey);

	if (pGrp && pGrp->pszCommands && *pGrp->pszCommands)
	{
		pParent->pszTaskBuf = lstrdup(pGrp->pszCommands);

		wchar_t *pszDataW = pParent->pszTaskBuf.data();
		wchar_t *pszLine = pszDataW;
		
		while (*pszLine && (nCount < MAX_CONSOLE_COUNT))
		{
			pszLine = const_cast<wchar_t*>(SkipNonPrintable(pszLine));
			wchar_t* pszNewLine = wcschr(pszLine, L'\n');
			if (pszNewLine)
			{
				*pszNewLine = 0;
				if ((pszNewLine > pszDataW) && (*(pszNewLine-1) == L'\r'))
					*(pszNewLine-1) = 0;
			}

			if (*pszLine)
			{
				itm.Reset(CmdTaskPopupItem::eTaskCmd, ++mn_CmdLastID);
				itm.pGrp = pGrp;
				itm.pszCmd = pszLine;
				if (nCount <= nMenuHotkeyMax)
					swprintf_c(itm.szShort, L"&%c: ", sMenuHotkey[nCount]);
				else
					itm.szShort[0] = 0;

				// Escape possible "&" in the command line, let them visible in the menu
				const size_t cchItemMax = _tcslen(itm.szShort) + (2 * _tcslen(pszLine)) + 1;
				if (itm.pszTaskBuf.GetBuffer(cchItemMax))
				{
					itm.pszTaskBuf.Set(itm.szShort);
					CmdTaskPopupItem::SetMenuName(itm.pszTaskBuf.data(), cchItemMax, pszLine, true);

					InsertMenu(hMenu, -1, MF_BYPOSITION | MF_ENABLED | MF_STRING, itm.nCmd, itm.pszTaskBuf.c_str());
					m_CmdTaskPopup.push_back(itm);

					nCount++;
				}
			}

			if (!pszNewLine) break;
			pszLine = pszNewLine + 1;
		}
	}

	return nCount;
}

LRESULT CConEmuMenu::OnInitMenuPopup(HWND hWnd, HMENU hMenu, LPARAM lParam)
{
	// Уже должен быть выставлен тип меню, иначе не будут всплывать подсказки для пунктов меню
	_ASSERTE(mn_TrackMenuPlace != tmp_None);
	if (!hMenu)
	{
		_ASSERTE(hMenu!=nullptr);
		return 0;
	}

	DefWindowProc(hWnd, WM_INITMENUPOPUP, (WPARAM)hMenu, lParam);

	MENUITEMINFO mi = {sizeof(mi)};
	wchar_t szText[255];
	mi.fMask = MIIM_STRING; mi.dwTypeData = szText; mi.cch = countof(szText);
	BOOL bIsSysMenu = GetMenuItemInfo(hMenu, SC_CLOSE, FALSE, &mi);


	if (HIWORD(lParam))
	{
		if (mn_TrackMenuPlace != tmp_System)
		{
			_ASSERTE(mn_TrackMenuPlace == tmp_System);
			return 0;
		}

		// при всплытии "Help/Debug/..." submenu сюда мы тоже попадаем

		if (bIsSysMenu)
		{
			UpdateSysMenu(hMenu);

			//BOOL bSelectionExist = FALSE;

			CVConGuard VCon;
			CVirtualConsole* pVCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : nullptr;
			//if (pVCon && pVCon->RCon())
			//	bSelectionExist = pVCon->RCon()->isSelectionPresent();

			//EnableMenuItem(hMenu, ID_CON_COPY, MF_BYCOMMAND|(bSelectionExist?MF_ENABLED:MF_GRAYED));
			if (mh_SysEditPopup)
			{
				TODO("Проверить, сработает ли, если mh_EditPopup уже был вставлен в SystemMenu?");
				CreateEditMenuPopup(pVCon, mh_SysEditPopup);
			}
			else
			{
				_ASSERTE(mh_SysEditPopup!=nullptr);
			}

			if (mh_VConListPopup)
			{
				CreateVConListPopupMenu(mh_VConListPopup, TRUE/*abFirstTabOnly*/);
			}
			else
			{
				_ASSERTE(mh_VConListPopup!=nullptr);
			}

			if (mh_ActiveVConPopup)
			{
				CreateVConPopupMenu(nullptr, mh_ActiveVConPopup, false, false);
			}
			else
			{
				_ASSERTE(mh_ActiveVConPopup!=nullptr);
			}


			CheckMenuItem(hMenu, ID_DEBUG_SHOWRECTS, MF_BYCOMMAND|(gbDebugShowRects ? MF_CHECKED : MF_UNCHECKED));
			//#ifdef _DEBUG
			//		wchar_t szText[128];
			//		MENUITEMINFO mi = {sizeof(MENUITEMINFO)};
			//		mi.fMask = MIIM_STRING|MIIM_STATE;
			//		bool bLogged = false, bAllowed = false;
			//		CRealConsole* pRCon = mp_ VActive ? mp_ VActive->RCon() : nullptr;
			//
			//		if (pRCon)
			//		{
			//			bLogged = pRCon->IsLogShellStarted();
			//			bAllowed = (pRCon->GetFarPID(TRUE) != 0);
			//		}
			//
			//		lstrcpy(szText, bLogged ? _T("Disable &shell log") : _T("Enable &shell log..."));
			//		mi.dwTypeData = szText;
			//		mi.fState = bAllowed ? MFS_ENABLED : MFS_GRAYED;
			//		SetMenuItemInfo(hMenu, ID_MONITOR_SHELLACTIVITY, FALSE, &mi);
			//#endif
		}
	}

	if (mn_TrackMenuPlace == tmp_Cmd)
	{
		CmdTaskPopupItem* p = nullptr;
		for (INT_PTR i = 0; i < m_CmdTaskPopup.size(); i++)
		{
			if (m_CmdTaskPopup[i].hPopup == hMenu)
			{
				p = &(m_CmdTaskPopup[i]);
				break;
			}
		}

		if (mb_CmdShowTaskItems && p && (p->ItemType == CmdTaskPopupItem::eTaskPopup) && !p->bPopupInitialized)
		{
			p->bPopupInitialized = TRUE;
			const CommandTasks* pGrp = static_cast<const CommandTasks*>(p->pGrp);

			if (pGrp && pGrp->pszCommands && *pGrp->pszCommands)
			{
				int nCount = FillTaskPopup(hMenu, p);

				if (nCount > 1)
				{
					CmdTaskPopupItem itm = {CmdTaskPopupItem::eNone};
					InsertMenu(hMenu, 0, MF_BYPOSITION|MF_SEPARATOR, 0, nullptr);

					itm.Reset(CmdTaskPopupItem::eTaskAll, ++mn_CmdLastID, CLngRc::getRsrc(lng_MenuAllTaskTabs/*"All task tabs"*/));
					itm.pGrp = pGrp;
					InsertMenu(hMenu, 0, MF_BYPOSITION|MF_ENABLED|MF_STRING, itm.nCmd, itm.szShort);
					m_CmdTaskPopup.push_back(itm);

					nCount++;
				}
			}
		}
	}

	return 0;
}

int CConEmuMenu::trackPopupMenu(TrackMenuPlace place, HMENU hMenu, UINT uFlags, int x, int y, HWND hWnd, RECT *prcRect /* = nullptr*/)
{
	ShowMenuHint(nullptr);
	TrackMenuPlace prevPlace = mn_TrackMenuPlace;
	if (prevPlace == place)
	{
		_ASSERTE(prevPlace==tmp_System);
		prevPlace = tmp_None;
	}
	_ASSERTE(prevPlace==tmp_None || prevPlace==tmp_Cmd);

	mn_TrackMenuPlace = place;

	TPMPARAMS ex = {sizeof(ex)};
	if (prcRect)
		ex.rcExclude = *prcRect;
	else
		ex.rcExclude = MakeRect(x-1,y-1,x+1,y+1);

	if (!(uFlags & (TPM_HORIZONTAL|TPM_VERTICAL)))
		uFlags |= TPM_HORIZONTAL;

	wchar_t szInfo[100];
	swprintf_c(szInfo, L"TrackPopupMenuEx started, place=%i, flags=x%08X, {%i,%i}", place, uFlags, x, y);
	LogString(szInfo);

	gpConEmu->ApplyPopupMenuTheme(hWnd);
	SetLastError(0);
	int cmd = TrackPopupMenuEx(hMenu, uFlags, x, y, hWnd, &ex);

	swprintf_c(szInfo, L"TrackPopupMenuEx done, command=%i, code=%u", cmd, GetLastError());
	LogString(szInfo);

	mn_TrackMenuPlace = prevPlace;

	ShowMenuHint(nullptr);

	return cmd;
}

void CConEmuMenu::ShowMenuHint(LPCWSTR pszText, POINT* ppt)
{
	enum LastHintStyle { eNone, eStatus, eTip };
	static LastHintStyle last = eNone;

	if (pszText && *pszText)
	{
		// Hide last hint
		if (last != eNone)
			ShowMenuHint(nullptr);

		POINT pt = {};
		if (ppt)
			pt = *ppt;
		else
			GetCursorPos(&pt);

		// if status bar exists - show hint there
		if (gpSet->isStatusBarShow)
		{
			if (gpConEmu->mp_Status)
			{
				gpConEmu->mp_Status->SetStatus(pszText);
				last = eStatus;
			}
		}
		else
		{
			if (gpConEmu->mp_Tip)
			{
				gpConEmu->mp_Tip->ShowTip(ghWnd, ghWnd, pszText, TRUE, pt, g_hInstance);
				last = eTip;
			}
		}
	}
	else
	{
		switch (last)
		{
		case eStatus:
			if (gpConEmu->mp_Status)
				gpConEmu->mp_Status->SetStatus(nullptr);
			break;
		case eTip:
			if (gpConEmu->mp_Tip)
				gpConEmu->mp_Tip->HideTip();
			break;
		}
		last = eNone;
	}
}

void CConEmuMenu::ShowMenuHint(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (nID && !(nFlags & MF_POPUP))
	{
		//POINT pt; GetCursorPos(&pt);
		RECT rcMenuItem = {};
		BOOL lbMenuItemPos = FALSE;
		UINT nMenuID = 0;
		for (int i = 0; i < 100; i++)
		{
			nMenuID = GetMenuItemID(hMenu, i);
			if (nMenuID == nID)
			{
				lbMenuItemPos = GetMenuItemRect(ghWnd, hMenu, i, &rcMenuItem);
				break;
			}
		}
		if (lbMenuItemPos)
		{
			POINT pt = {rcMenuItem.left + (rcMenuItem.bottom - rcMenuItem.top)*2, rcMenuItem.bottom};
			//pt.x = rcMenuItem.left; //(rcMenuItem.left + rcMenuItem.right) >> 1;
			//pt.y = rcMenuItem.bottom;
			TCHAR szText[0x200];
			if (CLngRc::getHint(nMenuID, szText, countof(szText)))
			{
				ShowMenuHint(szText, &pt);
				return;
			}
		}
	}

	ShowMenuHint(nullptr);
}

void CConEmuMenu::ShowKeyBarHint(HMENU hMenu, WORD nID, WORD nFlags)
{
	if (nID && !(nFlags & MF_POPUP))
	{
		CVConGuard VCon;
		CVirtualConsole* pVCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : nullptr;
		if (pVCon && pVCon->RCon())
			pVCon->RCon()->ShowKeyBarHint(nID);
	}
}

void CConEmuMenu::OnNcIconLClick()
{
	DWORD nCurTick = GetTickCount();
	DWORD nOpenDelay = nCurTick - mn_SysMenuOpenTick;
	DWORD nCloseDelay = nCurTick - mn_SysMenuCloseTick;
	DWORD nDoubleTime = GetDoubleClickTime();

	if (mn_SysMenuOpenTick && (nOpenDelay < nDoubleTime))
	{
		gpConEmu->PostScClose();
	}
	else if (mn_SysMenuCloseTick && (nCloseDelay < (nDoubleTime/2)))
	{
		// Пропустить - кликом закрыли меню
		#ifdef _DEBUG
		int nDbg = 0;
		#endif
	}
	// When Alt is hold down - sysmenu will be immediately closed
	else if (!isPressed(VK_MENU))
	{
		LogString(L"ShowSysmenu called from (OnNcIconLClick)");
		ShowSysmenu();
	}
}

void CConEmuMenu::ShowSysmenu(int x, int y, DWORD nFlags /*= 0*/)
{
	//if (!Wnd)
	//	Wnd = ghWnd;

	WARNING("SysMenu: Обработать DblClick по иконке!");

	if ((x == -32000) || (y == -32000))
	{
		RECT rect = {}, cRect = {};
		GetWindowRect(ghWnd, &rect);
		cRect = gpConEmu->ClientRect();
		WINDOWINFO wInfo = {sizeof(wInfo)};
		GetWindowInfo(ghWnd, &wInfo);
		int nTabShift =
		    ((gpConEmu->isCaptionHidden()) && gpConEmu->isTabsShown() && (gpSet->nTabsLocation != 1))
		    ? gpConEmu->GetDefaultTabbarHeight() : 0;

		if (x == -32000)
			x = rect.right - cRect.right - wInfo.cxWindowBorders;

		if (y == -32000)
			y = rect.bottom - cRect.bottom - wInfo.cyWindowBorders + nTabShift;
	}

	bool iconic = gpConEmu->isIconic();
	bool zoomed = gpConEmu->isZoomed();
	bool visible = _bool(::IsWindowVisible(ghWnd));
	int style = GetWindowLong(ghWnd, GWL_STYLE);
	HMENU systemMenu = GetSysMenu();

	if (!systemMenu)
		return;

	if (!gpConEmu->mp_Inside)
	{
		EnableMenuItem(systemMenu, SC_RESTORE,
		               MF_BYCOMMAND|((visible && (iconic || zoomed)) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_MOVE,
		               MF_BYCOMMAND|((visible && !(iconic || zoomed)) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_SIZE,
		               MF_BYCOMMAND|((visible && (!(iconic || zoomed) && (style & WS_SIZEBOX))) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_MINIMIZE,
		               MF_BYCOMMAND|((visible && (!iconic && (style & WS_MINIMIZEBOX))) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, SC_MAXIMIZE,
		               MF_BYCOMMAND|((visible && (!zoomed && (style & WS_MAXIMIZEBOX))) ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(systemMenu, ID_TOTRAY, MF_BYCOMMAND|MF_ENABLED);
	}

	mn_TrackMenuPlace = tmp_System;
	SendMessage(ghWnd, WM_INITMENU, (WPARAM)systemMenu, 0);
	SendMessage(ghWnd, WM_INITMENUPOPUP, (WPARAM)systemMenu, MAKELPARAM(0, true));

	SetActiveWindow(ghWnd);

	mn_SysMenuOpenTick = GetTickCount();
	POINT ptCurBefore = {}; GetCursorPos(&ptCurBefore);

	int command = trackPopupMenu(tmp_System, systemMenu,
		 TPM_RETURNCMD | TPM_LEFTBUTTON | TPM_RIGHTBUTTON | nFlags,
		 x, y, ghWnd);

	if (command == 0)
	{
		bool bLbnPressed = isPressed(VK_LBUTTON);
		mn_SysMenuCloseTick = GetTickCount();

		if (bLbnPressed && ((mn_SysMenuCloseTick - mn_SysMenuOpenTick) < GetDoubleClickTime()))
		{
			POINT ptCur = {}; GetCursorPos(&ptCur);
			if (PtDiffTest(ptCur, ptCurBefore.x, ptCurBefore.y, 8))
			{
				LRESULT lHitTest = SendMessage(ghWnd, WM_NCHITTEST, 0, MAKELONG(ptCur.x,ptCur.y));
				if (lHitTest == HTSYSMENU)
				{
					command = SC_CLOSE;
				}
			}
		}
	}
	else
	{
		mn_SysMenuCloseTick = 0;
	}

	wchar_t szInfo[64]; swprintf_c(szInfo, L"ShowSysmenu result: %i", command);
	LogString(szInfo);

	if (command && Icon.isWindowInTray() && gpConEmu->isIconic())
	{
		_ASSERTE(!gpConEmu->mp_Inside);

		switch (command)
		{
			case SC_CLOSE:
				break;

			//case SC_RESTORE:
			//case SC_MOVE:
			//case SC_SIZE:
			//case SC_MINIMIZE:
			//case SC_MAXIMIZE:
			default:
				//SendMessage(ghWnd, WM_TRAYNOTIFY, 0, WM_LBUTTONDOWN);
				//Icon.OnTryIcon(ghWnd, WM_TRAYNOTIFY, 0, WM_LBUTTONDOWN);
				gpConEmu->DoMinimizeRestore(sih_Show);
				break;
		}
	}

	if (command)
	{
		// gpConEmu will trigger our OnSysCommand method on post received
		PostMessage(ghWnd, mn_MsgOurSysCommand/*WM_SYSCOMMAND*/, (WPARAM)command, 0);
	}
}

HMENU CConEmuMenu::CreateDebugMenuPopup()
{
	const HMENU hDebug = CreatePopupMenu();
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_CON_TOGGLE_VISIBLE, MenuAccel(vkCtrlWinAltSpace, CLngRc::getRsrc(lng_MenuConToggleVisible/*"&Real console"*/)));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_CONPROP, CLngRc::getRsrc(lng_MenuConProperties/*"&Properties..."*/));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_SCREENSHOT, MenuAccel(vkScreenshot, CLngRc::getRsrc(lng_MenuScreenshot/*"Make &screenshot..."*/)));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DUMPCONSOLE, CLngRc::getRsrc(lng_MenuDumpConsole/*""&Dump screen..."*/));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_LOADDUMPCONSOLE, CLngRc::getRsrc(lng_MenuLoadDumpConsole/*"&Load screen dump..."*/));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUGGUI, CLngRc::getRsrc(lng_MenuDebugGui/*"Debug &log (GUI)"*/));
//#ifdef _DEBUG
//	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_MONITOR_SHELLACTIVITY, _T("Enable &shell log..."));
//#endif
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUG_SHOWRECTS, CLngRc::getRsrc(lng_MenuDebugShowRects/*"Show debug rec&ts"*/));
	AppendMenu(hDebug, MF_SEPARATOR, 0, nullptr);
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUG_TRAP, CLngRc::getRsrc(lng_MenuDebugTrap/*"Raise exception (Main thread)"*/));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUG_TRAP2, CLngRc::getRsrc(lng_MenuDebugTrap2/*"Raise exception (Monitor thread)"*/));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUG_ASSERT, CLngRc::getRsrc(lng_MenuDebugAssert/*"Show assertion"*/));
	#ifdef TRACK_MEMORY_ALLOCATIONS
	AppendMenu(hDebug, MF_SEPARATOR, 0, nullptr);
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DUMP_MEM_BLK, CLngRc::getRsrc(lng_MenuDumpMemBlk/*"Dump used memory blocks"*/));
	#endif
	AppendMenu(hDebug, MF_SEPARATOR, 0, nullptr);
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_DEBUGCON, MenuAccel(vkDebugProcess, CLngRc::getRsrc(lng_MenuDebugActive/*"Debug &active process..."*/)));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_MINIDUMP, MenuAccel(vkDumpProcess, CLngRc::getRsrc(lng_MenuMiniDump/*"Active process &memory dump..."*/)));
	AppendMenu(hDebug, MF_STRING|MF_ENABLED, ID_MINIDUMPTREE, MenuAccel(vkDumpTree, CLngRc::getRsrc(lng_MenuMiniDumpTree/*"Active &tree memory dump..."*/)));
	return hDebug;
}

HMENU CConEmuMenu::CreateVConListPopupMenu(HMENU ahExist, BOOL abFirstTabOnly)
{
	HMENU h = ahExist ? ahExist : CreatePopupMenu();
	wchar_t szText[128];
	const int nMaxStrLen = 32;

	BOOL lbActiveVCon = FALSE;
	int nActiveCmd = -1; // DWORD MAKELONG(WORD wLow,WORD wHigh);
	DWORD nAddFlags = 0;

	if (ahExist)
	{
		while (DeleteMenu(ahExist, 0, MF_BYPOSITION))
			;
	}

	CVConGuard VCon;
	for (int V = 0; CVConGroup::GetVCon(V, &VCon, true); V++)
	{
		if ((lbActiveVCon = VCon->isActive(false)))
			nActiveCmd = MAKELONG(1, V+1);
		nAddFlags = 0; //(lbActiveVCon ? MF_DEFAULT : 0);
		CRealConsole* pRCon = VCon->RCon();
		if (!pRCon)
		{
			wsprintf(szText, L"%i: VConsole", V + 1);
			AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(1, V+1), szText);
		}
		else
		{
			CTab tab(__FILE__,__LINE__);
			int R = 0;
			if (!pRCon->GetTab(R, tab))
			{
				wsprintf(szText, L"%i: RConsole", V+1);
				AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(1, V+1), szText);
			}
			else
			{
				do
				{
					const bool bActive = (pRCon->GetActiveTab() == R);
					_ASSERTE(bActive == ((tab->Flags() & fwt_CurrentFarWnd) == fwt_CurrentFarWnd));

					nAddFlags = 0/*((lbActiveVCon && (R==0)) ? MF_DEFAULT : 0)*/
						| ((lbActiveVCon && (abFirstTabOnly || bActive)) ? MF_CHECKED : MF_UNCHECKED)
						#if 0
						| ((tab->Flags() & etfDisabled) ? (MF_DISABLED|MF_GRAYED) : 0)
						#endif
						;

					LPCWSTR pszName = pRCon->GetTabTitle(tab);
					int nLen = lstrlen(pszName);
					if (!R)
						wsprintf(szText, L"%i: ", V+1);
					else
						wcscpy_c(szText, L"      ");
					if (nLen <= nMaxStrLen)
					{
						wcscat_c(szText, pszName);
					}
					else
					{
						int nCurLen = lstrlen(szText);
						_ASSERTE((nCurLen+10)<nMaxStrLen);
						if (tab->Type() == fwt_Panels)
						{
							lstrcpyn(szText+nCurLen, pszName, nMaxStrLen-1-nCurLen);
						}
						else
						{
							szText[nCurLen++] = L'\x2026'; szText[nCurLen] = 0;
							lstrcpyn(szText+nCurLen, pszName+nLen-nMaxStrLen, nMaxStrLen-1-nCurLen);
						}
						wcscat_c(szText, L"\x2026"); //...
					}
					AppendMenu(h, MF_STRING|nAddFlags, MAKELONG(R+1, V+1), szText);
				} while (!abFirstTabOnly && pRCon->GetTab(++R, tab));
			}
		}
	}

	if (nActiveCmd != -1 && !abFirstTabOnly)
	{
		MENUITEMINFO mi = {sizeof(mi), MIIM_STATE|MIIM_ID};
		mi.wID = nActiveCmd;
		GetMenuItemInfo(h, nActiveCmd, FALSE, &mi);
		mi.fState |= MF_DEFAULT;
		SetMenuItemInfo(h, nActiveCmd, FALSE, &mi);
	}

	return h;
}

// abAddNew - if "false" the result is inserted as PopupMenu into top-level menu (SysMenu->Active Console)
// abFromConsole - menu was triggered by either mouse click on the *console surface*, or hotkey presses in the console
HMENU CConEmuMenu::CreateVConPopupMenu(CVirtualConsole* apVCon, HMENU ahExist, bool abAddNew, bool abFromConsole)
{
	//BOOL lbEnabled = TRUE;
	HMENU hMenu = ahExist;

	CVConGuard VCon;
	if (!apVCon && (CVConGroup::GetActiveVCon(&VCon) >= 0))
		apVCon = VCon.VCon();

	UINT con_flags = (apVCon ? MF_ENABLED : MF_GRAYED);
	UINT close_flags = ((apVCon && apVCon->RCon()->CanCloseTab()) ? MF_ENABLED : MF_GRAYED);
	UINT gui_flags = ((apVCon && apVCon->GuiWnd()) ? MF_ENABLED : MF_GRAYED);

	// "Close or &kill"
	HMENU& hTerminate = mh_TerminatePopup;
	{
		const MenuItem terminateItems[] = {
			{ mit_Command,   IDM_TERMINATECON,      vkMultiClose,     con_flags,  CLngRc::getRsrc(lng_MenuTerminateCon/*"Close active &console"*/) },
			{ mit_Command,   IDM_TERMINATEPRC,      vkTerminateApp,   con_flags,  CLngRc::getRsrc(lng_MenuTerminatePrc/*"Kill &active process"*/) },
			{ mit_Command,   IDM_TERMINATEBUTSHELL, vkTermButShell,   con_flags,  CLngRc::getRsrc(lng_MenuTerminateButShell/*"Kill all but &shell"*/) },
			{ mit_Separator, 0, 0, 0, nullptr},
			{ mit_Command,   IDM_TERMINATEGROUP,    vkCloseGroup,     con_flags,  CLngRc::getRsrc(lng_MenuTerminateGroup/*"Close active &group"*/) },
			{ mit_Command,   IDM_TERMINATEPRCGROUP, vkCloseGroupPrc,  con_flags,  CLngRc::getRsrc(lng_MenuTerminatePrcGroup/*"Kill active &processes"*/) },
			{ mit_Separator, 0, 0, 0, nullptr},
			{ mit_Command,   IDM_TERMINATEALLCON,   vkCloseAllCon,    MF_ENABLED, CLngRc::getRsrc(lng_MenuTerminateAllCon/*"Close &all consoles"*/) },
			{ mit_Command,   IDM_TERMINATEZOMBIES,  vkCloseZombies,   MF_ENABLED, CLngRc::getRsrc(lng_MenuTerminateZombies/*"Close all &zombies"*/) },
			{ mit_Command,   IDM_TERMINATECONEXPT,  vkCloseExceptCon, MF_ENABLED, CLngRc::getRsrc(lng_MenuTerminateConExpt/*"Close e&xcept active"*/) },
			{ mit_Command,   IDM_TERMINATE2RIGHT,   vkClose2Right,    con_flags,  CLngRc::getRsrc(lng_MenuTerminate2Right/*"Close to the &right"*/) },
			{ mit_Separator, 0, 0, 0, nullptr},
			{ mit_Command,   IDM_DETACH,            vkConDetach,      con_flags,  CLngRc::getRsrc(lng_MenuDetach/*"Detach"*/) },
			{ mit_Command,   IDM_UNFASTEN,          vkConUnfasten,    con_flags,  CLngRc::getRsrc(lng_MenuUnfasten/*"Unfasten"*/) }
		};
		CreateOrUpdateMenu(hTerminate, terminateItems, countof(terminateItems));
	}

	// "&Restart or duplicate"
	HMENU& hRestart = mh_RestartPopup;
	{
		const MenuItem restartItems[] = {
			{ mit_Command,   IDM_DUPLICATE,        vkDuplicateRoot,   con_flags,  CLngRc::getRsrc(lng_MenuDuplicate/*"Duplica&te root..."*/) },
			//{ mit_Command, IDM_ADMIN_DUPLICATE,  vkDuplicateRootAs, con_flags,  L"Duplica&te as Admin..." },
			{ mit_Separator, 0, 0, 0, nullptr},
			{ mit_Command,   IDM_SPLIT2RIGHT,      vkSplitNewConH,    con_flags,  CLngRc::getRsrc(lng_MenuSplit2Right/*"Split to ri&ght"*/) },
			{ mit_Command,   IDM_SPLIT2BOTTOM,     vkSplitNewConV,    con_flags,  CLngRc::getRsrc(lng_MenuSplit2Bottom/*"Split to &bottom"*/) },
			{ mit_Separator, 0, 0, 0, nullptr},
			{ mit_Command,   IDM_RESTARTDLG,       vkMultiRecreate,   con_flags,  CLngRc::getRsrc(lng_MenuRestartDlg/*"&Restart..."*/) },
			{ mit_Command,   IDM_RESTART,          0,                 con_flags,  CLngRc::getRsrc(lng_MenuRestart/*"&Restart"*/) },
			{ mit_Command,   IDM_RESTARTAS,        0,                 con_flags,  CLngRc::getRsrc(lng_MenuRestartAs/*"Restart as Admin"*/) }
		};
		CreateOrUpdateMenu(hRestart, restartItems, countof(restartItems));
	}

	CEStr lsCloseText;
	if (abFromConsole)
		lsCloseText.Set(CLngRc::getRsrc(lng_MenuCloseConsole/*"&Close console"*/));
	else if (gpSet->isOneTabPerGroup && CVConGroup::isGroup(apVCon))
		lsCloseText.Set(CLngRc::getRsrc(lng_MenuCloseGroup/*"&Close tab group"*/));
	else
		lsCloseText.Set(CLngRc::getRsrc(lng_MenuCloseTab/*"&Close tab"*/));

	// Root menu if (abAddNew == true) or (SysMenu->Active Console) if (abAddNew == false)
	{
		const MenuItem vConItems[] = {
			{ mit_Command,   IDM_CLOSE,            vkCloseTab,        close_flags, lsCloseText },
			{ mit_Command,   IDM_RENAMETAB,        vkRenameTab,       con_flags,   CLngRc::getRsrc(lng_MenuRenameTab/*"Rena&me tab..."*/) },
			{ mit_Command,   IDM_CHANGEAFFINITY,   vkAffinity,        con_flags,   CLngRc::getRsrc(lng_MenuChangeAffinity/*"A&ffinity/priority..."*/) },
			// ReSharper disable once CppCStyleCast
			{ mit_Popup,     (UINT_PTR)hRestart,   0,                 MF_ENABLED,  CLngRc::getRsrc(lng_MenuPopupRestart/*"&Restart or duplicate"*/) },
			// ReSharper disable once CppCStyleCast
			{ mit_Popup,     (UINT_PTR)hTerminate, 0,                 MF_ENABLED,  CLngRc::getRsrc(lng_MenuPopupTerminate/*"Close or &kill"*/) },
			{ mit_Command,   IDM_CHILDSYSMENU,     vkChildSystemMenu, gui_flags,   CLngRc::getRsrc(lng_MenuChildSysMenu/*"Child system menu..."*/) },
			// abAddNew - if "false" the result is inserted as PopupMenu into top-level menu
			// These three items are visible in the popup menu (Shift+RClick on VCon)
			{ mit_Separator, 0, 0, 0, nullptr},
			{ mit_Command,   ID_NEWCONSOLE,        vkMultiNewConfirm, MF_ENABLED,  CLngRc::getRsrc(lng_MenuNewConsole/*"New console dialog..."*/) },
			{ mit_Command,   IDM_ATTACHTO,         vkMultiNewAttach,  MF_ENABLED,  CLngRc::getRsrc(lng_MenuAttachTo/*"Attach to..."*/) }
		};

		CreateOrUpdateMenu(hMenu, vConItems, countof(vConItems) - (abAddNew ? 0 : 3));
	}

	return hMenu;
}

HMENU CConEmuMenu::CreateEditMenuPopup(CVirtualConsole* apVCon, HMENU ahExist /*= nullptr*/)
{
	CVConGuard VCon;
	if (!apVCon && (CVConGroup::GetActiveVCon(&VCon) >= 0))
		apVCon = VCon.VCon();

	BOOL lbEnabled = TRUE;
	BOOL lbSelectionExist = FALSE;
	if (apVCon && apVCon->RCon())
	{
		if (apVCon->RCon()->GuiWnd() && !apVCon->RCon()->isBufferHeight())
			lbEnabled = FALSE; // Если видимо дочернее графическое окно - выделение смысла не имеет
		// Нужно ли серить пункт "Copy"
		lbSelectionExist = lbEnabled && apVCon->RCon()->isSelectionPresent();
	}

	HMENU hMenu = ahExist;

	const UINT fEnabled = MF_ENABLED;
	const UINT fGrayed = MF_GRAYED;
	const UINT fChecked = MF_CHECKED;
	const UINT fUnchecked = MF_UNCHECKED;

	const BYTE fmt = gpSet->isCTSHtmlFormat;
	// MenuItemType mit; UINT MenuId; UINT HotkeyId; UINT Flags; LPCWSTR pszText;
	const MenuItem items[] = {
		{ mit_Command,   ID_CON_MARKBLOCK,  vkCTSVkBlockStart, (lbEnabled?fEnabled:fGrayed), CLngRc::getRsrc(lng_MenuConMarkBlock/*"Mark &block"*/) },
		{ mit_Command,   ID_CON_MARKTEXT,   vkCTSVkTextStart,  (lbEnabled?fEnabled:fGrayed), CLngRc::getRsrc(lng_MenuConMarkText/*"Mar&k text"*/) },
		{ mit_Separator, 0, 0, 0, nullptr},
		{ mit_Command,   ID_CON_COPY,       0,          (lbSelectionExist?fEnabled:fGrayed), CLngRc::getRsrc(lng_MenuConCopy/*"Cop&y"*/) },
		{ mit_Command,   ID_CON_COPY_ALL,   vkCTSVkCopyAll,    (lbEnabled?fEnabled:fGrayed), CLngRc::getRsrc(lng_MenuConCopyAll/*"Copy &all"*/) },
		{ mit_Command,   ID_CON_PASTE,      vkPasteText,       (lbEnabled?fEnabled:fGrayed), CLngRc::getRsrc(lng_MenuConPaste/*"&Paste"*/) },
		{ mit_Separator, 0, 0, 0, nullptr},
		{ mit_Command,   ID_RESET_TERMINAL, vkResetTerminal,   (lbEnabled?fEnabled:fGrayed), CLngRc::getRsrc(lng_MenuResetTerminal/*"&Reset terminal"*/) },
		{ mit_Separator, 0, 0, 0, nullptr},
		{ mit_Option,    ID_CON_COPY_HTML0, 0,             ((fmt == 0)?fChecked:fUnchecked), CLngRc::getRsrc(lng_MenuConCopyFmt0/*"Plain &text only"*/) },
		{ mit_Option,    ID_CON_COPY_HTML1, 0,             ((fmt == 1)?fChecked:fUnchecked), CLngRc::getRsrc(lng_MenuConCopyFmt1/*"Copy &HTML format"*/) },
		{ mit_Option,    ID_CON_COPY_HTML2, 0,             ((fmt == 2)?fChecked:fUnchecked), CLngRc::getRsrc(lng_MenuConCopyFmt2/*"Copy a&s HTML"*/) },
		{ mit_Option,    ID_CON_COPY_HTML3, 0,             ((fmt == 3)?fChecked:fUnchecked), CLngRc::getRsrc(lng_MenuConCopyFmt3/*"A&NSI sequences"*/) },
		{ mit_Separator, 0, 0, 0, nullptr},
		{ mit_Command,   ID_CON_FIND,       vkFindTextDlg,     (lbEnabled?fEnabled:fGrayed), CLngRc::getRsrc(lng_MenuConFind/*"&Find text..."*/) },
	};

	CreateOrUpdateMenu(hMenu, items, countof(items));

	return hMenu;
}

HMENU CConEmuMenu::CreateViewMenuPopup(CVirtualConsole* apVCon, HMENU ahExist /*= nullptr*/)
{
	CVConGuard VCon;
	if (!apVCon && (CVConGroup::GetActiveVCon(&VCon) >= 0))
		apVCon = VCon.VCon();

	bool  bNew = (ahExist == nullptr);
	HMENU hMenu = bNew ? CreatePopupMenu() : ahExist;

	const ColorPalette* pPal = nullptr;

	int iActiveIndex = apVCon->GetPaletteIndex();

	int i = 0;
	int iBreak = 0;
	while ((i < (ID_CON_SETPALETTE_LAST-ID_CON_SETPALETTE_FIRST)) && (pPal = gpSet->PaletteGet(i)))
	{
		if (!pPal->pszName)
		{
			_ASSERTE(pPal->pszName);
			break;
		}
		wchar_t szItem[128] = L"";
		CmdTaskPopupItem::SetMenuName(szItem, countof(szItem), pPal->pszName, true);

		if (!iBreak && i && (pPal->pszName[0] != L'<'))
			iBreak = i;

		MENUITEMINFO mi = {sizeof(mi)};
		mi.fMask = MIIM_STRING|MIIM_STATE|MIIM_FTYPE;
		mi.dwTypeData = szItem; mi.cch = countof(szItem);
		mi.fState =
			// Add 'CheckMark' to the current palette (if it differs from ConEmu global one)
			((i==iActiveIndex) ? MFS_CHECKED : MFS_UNCHECKED)
			;
		mi.fType = MFT_STRING
			// Ensure palettes list will not be too long, ATM there are 25 predefined palettes
			| ((iBreak && !(i % iBreak)) ? MFT_MENUBREAK : 0)
			;
		if (bNew || !SetMenuItemInfo(hMenu, ID_CON_SETPALETTE_FIRST+i, FALSE, &mi))
		{
			_ASSERTE(MFS_CHECKED == MF_CHECKED && MFT_MENUBREAK == MF_MENUBREAK && MFT_STRING == 0);
			AppendMenu(hMenu, MF_STRING | mi.fState | mi.fType, ID_CON_SETPALETTE_FIRST+i, szItem);
		}

		i++;
	}

	for (int j = i; j <= ID_CON_SETPALETTE_LAST; j++)
	{
		DeleteMenu(hMenu, ID_CON_SETPALETTE_FIRST+j, MF_BYCOMMAND);
	}

	return hMenu;
}

HMENU CConEmuMenu::CreateHelpMenuPopup()
{
	HMENU hHelp = CreatePopupMenu();

	if (gpConEmu->isUpdateAllowed())
	{
		if (gpUpd && (gpUpd->InUpdate() != CConEmuUpdate::UpdateStep::NotStarted))
			AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_STOPUPDATE, CLngRc::getRsrc(lng_MenuStopUpdate/*"&Stop updates checking"*/));
		else
			AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_CHECKUPDATE, MenuAccel(vkCheckUpdates, CLngRc::getRsrc(lng_MenuCheckUpdate/*"&Check for updates"*/)));

		AppendMenu(hHelp, MF_SEPARATOR, 0, nullptr);
	}

	AppendMenu(hHelp, MF_STRING|MF_ENABLED, IDM_HOMEPAGE, CLngRc::getRsrc(lng_MenuHomePage/*"&Visit home page"*/));
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, IDM_DONATE_LINK, CLngRc::getRsrc(lng_MenuDonateLink/*"&Donate ConEmu"*/));

	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_WHATS_NEW_FILE, CLngRc::getRsrc(lng_MenuWhatsNewFile/*"Whats &new (local)"*/));
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_WHATS_NEW_WWW, CLngRc::getRsrc(lng_MenuWhatsNewWeb/*"Whats new (&web)"*/));
	AppendMenu(hHelp, MF_SEPARATOR, 0, nullptr);

	AppendMenu(hHelp, MF_STRING|MF_ENABLED, IDM_REPORTBUG, CLngRc::getRsrc(lng_MenuReportBug/*"&Report a bug..."*/));

	if (gpConEmu->ms_ConEmuChm[0])  //Показывать пункт только если есть conemu.chm
		AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_HELP, CLngRc::getRsrc(lng_MenuHelp/*"&Help"*/));

	AppendMenu(hHelp, MF_SEPARATOR, 0, nullptr);
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_HOTKEYS, MenuAccel(vkWinAltK, CLngRc::getRsrc(lng_MenuHotkeys/*"Hot&keys"*/)));
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_ONLINEHELP, MenuAccel(vkWinAltH, CLngRc::getRsrc(lng_MenuOnlineHelp/*"Online &Help"*/)));
	AppendMenu(hHelp, MF_STRING|MF_ENABLED, ID_ABOUT, MenuAccel(vkWinAltA, CLngRc::getRsrc(lng_MenuAbout/*"&About / Help"*/)));

	return hHelp;
}

LPCWSTR CConEmuMenu::MenuAccel(int descrId, LPCWSTR asText)
{
	if (!asText || !*asText)
	{
		_ASSERTE(asText != nullptr);
		return L"";
	}

	static wchar_t szTemp[255];
	wchar_t szKey[128] = {};

	const ConEmuHotKey* pHK = nullptr;
	const DWORD vkMod = gpSet->GetHotkeyById(descrId, &pHK);
	if (!vkMod || !ConEmuChord::GetHotkey(vkMod) || !pHK)
		return asText;

	pHK->GetHotkeyName(szKey);
	if (!*szKey)
		return asText;
	int nLen = lstrlen(szKey);
	lstrcpyn(szTemp, asText, countof(szTemp) - nLen - 4);
	wcscat_c(szTemp, L"\t");
	wcscat_c(szTemp, szKey);

	return szTemp;
}

LRESULT CConEmuMenu::OnSysCommand(HWND hWnd, WPARAM wParam, LPARAM lParam, UINT Msg /*= 0*/)
{
	static LONG InCall = 0;
	MSetter inCall(&InCall);
	wchar_t szDbg[128], szName[32];
	wcscpy_c(szName, Msg == WM_SYSCOMMAND ? L" - WM_SYSCOMMAND" : Msg == mn_MsgOurSysCommand ? L" - UM_SYSCOMMAND" : L"");
	swprintf_c(szDbg, L"OnSysCommand (%i(0x%X), %i, %i)%s", (DWORD)wParam, (DWORD)wParam, (DWORD)lParam, InCall, szName);
	LogString(szDbg);

	#ifdef _DEBUG
	wcscat_c(szDbg, L"\n");
	DEBUGSTRSIZE(szDbg);
	if (wParam == SC_HOTKEY)
	{
		_ASSERTE(wParam!=SC_HOTKEY);
	}
	#endif

	LRESULT result = 0;

	if (wParam >= IDM_VCON_FIRST && wParam <= IDM_VCON_LAST)
	{
		int nNewV = ((int)HIWORD(wParam))-1;
		#ifdef _DEBUG
		int nNewR = ((int)LOWORD(wParam))-1; UNREFERENCED_PARAMETER(nNewR);
		#endif

		CVConGuard VCon;
		if (CVConGroup::GetVCon(nNewV, &VCon))
		{
			// -- в SysMenu показываются только консоли (редакторов/вьюверов там нет)
			if (!VCon->isActive(false))
			{
				gpConEmu->Activate(VCon.VCon());
			}
		}
		return 0;
	}

	//switch(LOWORD(wParam))
	switch (wParam)
	{
		case ID_NEWCONSOLE:
			// Создать новую консоль
			gpConEmu->RecreateAction(gpSetCls->GetDefaultCreateAction(), true);
			return 0;

		case IDM_ATTACHTO:
			gpConEmu->AttachToDialog();
			return 0;

		case ID_SETTINGS:
			CSettings::Dialog();
			return 0;

		case ID_CON_PASTE:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->Paste();
				}
			}
			return 0;

		case ID_RESET_TERMINAL:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					gpConEmu->key_ResetTerminal(ConEmuChord(), false, nullptr, VCon->RCon());
				}
			}
			return 0;

		case ID_CON_FIND:
			gpConEmu->mp_Find->FindTextDialog();
			return 0;

		case ID_CON_COPY:
		case ID_CON_COPY_ALL:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					CECopyMode CopyMode = (wParam == ID_CON_COPY_ALL) ? cm_CopyAll : cm_CopySel;
					VCon->RCon()->DoSelectionCopy(CopyMode);
				}
			}
			return 0;

		case ID_CON_MARKBLOCK:
		case ID_CON_MARKTEXT:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->StartSelection(LOWORD(wParam) == ID_CON_MARKTEXT);
				}
			}
			return 0;

		case ID_CON_COPY_HTML0:
			gpSet->isCTSHtmlFormat = 0;
			return 0;
		case ID_CON_COPY_HTML1:
			gpSet->isCTSHtmlFormat = 1;
			return 0;
		case ID_CON_COPY_HTML2:
			gpSet->isCTSHtmlFormat = 2;
			return 0;
		case ID_CON_COPY_HTML3:
			gpSet->isCTSHtmlFormat = 3;
			return 0;

		#ifdef SHOW_AUTOSCROLL
		case ID_AUTOSCROLL:
			gpSetCls->AutoScroll = !gpSetCls->AutoScroll;
			CheckMenuItem(gpConEmu->mp_Menu->GetSysMenu(), ID_AUTOSCROLL, MF_BYCOMMAND |
			              (gpSetCls->AutoScroll ? MF_CHECKED : MF_UNCHECKED));
			return 0;
		#endif

		case ID_ALWAYSONTOP:
			{
				gpSet->isAlwaysOnTop = !gpSet->isAlwaysOnTop;
				gpConEmu->DoAlwaysOnTopSwitch();

				HWND hExt = gpSetCls->GetPage(thi_Features);

				if (ghOpWnd && hExt)
				{
					CheckDlgButton(hExt, cbAlwaysOnTop, gpSet->isAlwaysOnTop ? BST_CHECKED : BST_UNCHECKED);
				}
			}
			return 0;

		case ID_DUMPCONSOLE:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->DumpConsole();
				}
			 }
			return 0;

		case ID_SCREENSHOT:
			CConEmuCtrl::MakeScreenshot();

			return 0;

		case ID_LOADDUMPCONSOLE:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->LoadDumpConsole();
				}
			}
			return 0;

		case ID_DEBUGGUI:
			gpConEmu->StartDebugLogConsole();
			return 0;

		case ID_DEBUGCON:
			gpConEmu->StartDebugActiveProcess();
			return 0;
		case ID_MINIDUMP:
		case ID_MINIDUMPTREE:
			gpConEmu->MemoryDumpActiveProcess(wParam==ID_MINIDUMPTREE);
			return 0;

		//case ID_MONITOR_SHELLACTIVITY:
		//{
		//	CRealConsole* pRCon = mp_ VActive->RCon();

		//	if (pRCon)
		//		pRCon->LogShellStartStop();

		//	//if (!mb_CreateProcessLogged)
		//	//	StartLogCreateProcess();
		//	//else
		//	//	StopLogCreateProcess();
		//}
		//return 0;

		case ID_DEBUG_SHOWRECTS:
			gbDebugShowRects = !gbDebugShowRects;
			gpConEmu->InvalidateAll();
			return 0;

		case ID_DEBUG_TRAP:
			ConEmuAbout::OnInfo_ThrowTrapException(true);
			return 0;
		case ID_DEBUG_TRAP2:
			ConEmuAbout::OnInfo_ThrowTrapException(false);
			return 0;
		case ID_DEBUG_ASSERT:
			_ASSERT(FALSE && "This is DEBUG test assertion");
			AssertBox(_T("FALSE && \"This is RELEASE test assertion\""), _T(__FILE__), __LINE__);
			return 0;

		case ID_DUMP_MEM_BLK:
			#ifdef USE_XF_DUMP
			xf_dump();
			#else
			_ASSERTE(FALSE && "TRACK_MEMORY_ALLOCATIONS not defined");
			#endif
			return 0;

		case ID_CON_TOGGLE_VISIBLE:
			{
				CVConGuard VCon;
				if (gpConEmu->GetActiveVCon(&VCon) >= 0)
				{
					VCon->RCon()->ShowConsoleOrGuiClient(-1); // Toggle visibility
				}
			}
			return 0;

		case ID_HELP:
		{
			ConEmuAbout::OnInfo_Help();
			return 0;
		} // case ID_HELP:

		case IDM_HOMEPAGE:
		{
			ConEmuAbout::OnInfo_HomePage();
			return 0;
		}

		case IDM_REPORTBUG:
		{
			ConEmuAbout::OnInfo_ReportBug();
			return 0;
		}

		case ID_CHECKUPDATE:
			gpConEmu->CheckUpdates(UpdateCallMode::Manual);
			return 0;

		case ID_STOPUPDATE:
			if (gpUpd)
				gpUpd->StopChecking();
			return 0;

		case ID_HOTKEYS:
		{
			CSettings::Dialog(IDD_SPG_KEYS);
			return 0;
		}

		case IDM_DONATE_LINK:
		{
			ConEmuAbout::OnInfo_Donate();
			return 0;
		}

		case ID_ONLINEHELP:
		{
			ConEmuAbout::OnInfo_OnlineWiki();
			return 0;
		}

		case ID_ABOUT:
		{
			ConEmuAbout::OnInfo_About();
			return 0;
		}

		case ID_WHATS_NEW_FILE:
		case ID_WHATS_NEW_WWW:
		{
			ConEmuAbout::OnInfo_WhatsNew((wParam == ID_WHATS_NEW_FILE));
			return 0;
		}

		case ID_TOMONITOR:
		{
			gpConEmu->DoBringHere();
			return 0;
		}

		case ID_TOTRAY:
			if (IsWindowVisible(ghWnd))
				Icon.HideWindowToTray();
			else
				Icon.RestoreWindowFromTray();

			return 0;

		case ID_CONPROP:
		{
			CVConGuard VCon;
			if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
			{
				#ifdef MSGLOGGER
				{
					// Для отладки, посмотреть, какие пункты меню есть в RealConsole
					HMENU hMenu = ::GetSystemMenu(VCon->RCon()->ConWnd(), FALSE);
					MENUITEMINFO mii; TCHAR szText[255];

					for(int i=0; i<15; i++)
					{
						memset(&mii, 0, sizeof(mii));
						mii.cbSize = sizeof(mii); mii.dwTypeData=szText; mii.cch=255;
						mii.fMask = MIIM_ID|MIIM_STRING|MIIM_SUBMENU;

						if (GetMenuItemInfo(hMenu, i, TRUE, &mii))
						{
							mii.cbSize = sizeof(mii);

							if (mii.hSubMenu)
							{
								MENUITEMINFO mic;

								for(int i=0; i<15; i++)
								{
									memset(&mic, 0, sizeof(mic));
									mic.cbSize = sizeof(mic); mic.dwTypeData=szText; mic.cch=255;
									mic.fMask = MIIM_ID|MIIM_STRING;

									if (GetMenuItemInfo(mii.hSubMenu, i, TRUE, &mic))
									{
										mic.cbSize = sizeof(mic);
									}
									else
									{
										break;
									}
								}
							}
						}
						else
							break;
					}
				}
				#endif

				// Go!
				VCon->RCon()->ShowPropertiesDialog();
			}
			return 0;
		} // case ID_CONPROP:

		case SC_MAXIMIZE_SECRET:
			gpConEmu->SetWindowMode(wmMaximized);
			break;

		case SC_RESTORE_SECRET:
			gpConEmu->SetWindowMode(wmNormal);
			break;

		case SC_CLOSE:
			gpConEmu->OnScClose();
			break;

		case SC_MAXIMIZE:
		{
			DEBUGSTRSYS(L"OnSysCommand(SC_MAXIMIZE)\n");

			if (!mb_PassSysCommand)
			{
				#ifndef _DEBUG
				if (gpConEmu->isPictureView())
					break;
				#endif

				gpConEmu->SetWindowMode(wmMaximized);
			}
			else
			{
				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);
			}

			break;
		} // case SC_MAXIMIZE:

		case SC_RESTORE:
		{
			DEBUGSTRSYS(L"OnSysCommand(SC_RESTORE)\n");

			if (!mb_PassSysCommand)
			{
				#ifndef _DEBUG
				if (!gpConEmu->isIconic() && gpConEmu->isPictureView())
					break;
				#endif

				if (gpSet->isQuakeStyle)
				{
					gpConEmu->DoMinimizeRestore(sih_Show/*sih_HideTSA*/);
					break;
				}

				if (gpConEmu->SetWindowMode(gpConEmu->isIconic() ? gpConEmu->GetWindowMode() : wmNormal))
				{
					// abForceChild=TRUE, если в табе запущено GUI приложение - можно передать в него фокус
					gpConEmu->OnFocus(ghWnd, WM_ACTIVATEAPP, TRUE, 0, L"After SC_RESTORE", TRUE);
					break;
				}
			}

			// ***
			{
				bool bIconic = _bool(::IsIconic(hWnd));
				bool bPrev = bIconic ? SetRestoreFromMinimized(true) : mb_InRestoreFromMinimized;
				DEBUGTEST(WINDOWPLACEMENT wpl = {sizeof(wpl)}; GetWindowPlacement(ghWnd, &wpl););

				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);

				SetRestoreFromMinimized(bPrev);

				// abForceChild=TRUE, если в табе запущено GUI приложение - можно передать в него фокус
				gpConEmu->OnFocus(hWnd, WM_ACTIVATEAPP, TRUE, 0, L"From SC_RESTORE", TRUE);
			}

			break;
		} // case SC_RESTORE:

		case SC_MINIMIZE:
		{
			DEBUGSTRSYS(L"OnSysCommand(SC_MINIMIZE)\n");

			bool bMin2TSA = gpSet->isMinToTray();

			if (!mb_InScMinimize)
			{
				mb_InScMinimize = true;

				// Запомним, на каком мониторе мы были до минимзации
				gpConEmu->StorePreMinimizeMonitor();

				// Если "фокус" в дочернем Gui приложении - нужно перед скрытием ConEmu "поднять" его
				CVConGuard VCon;
				if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon()->GuiWnd())
				{
					apiSetForegroundWindow(ghWnd);
				}

				if (gpSet->isQuakeStyle)
				{
					gpConEmu->DoMinimizeRestore(sih_AutoHide);
					// We still need to initiate 'Minimize' behavior
					// to make OS move the focus to the previous window
					// So, continue to the WM_SYSCOMMAND
				}
				else if (bMin2TSA)
				{
					Icon.HideWindowToTray();
				}

				if (bMin2TSA)
				{
					// Окошко уже "спрятано", минимизировать не нужно
					mb_InScMinimize = false;
					break;
				}

				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);

				mb_InScMinimize = false;
			}
			else
			{
				DEBUGSTRSYS(L"--OnSysCommand(SC_MINIMIZE) skipped, already in cycle\n");
			}

			break;
		} // case SC_MINIMIZE:

		default:
		{
			if (wParam >= IDM_VCONCMD_FIRST && wParam <= IDM_VCONCMD_LAST)
			{
				if (InCall > 2)
				{
					_ASSERTE(InCall == 1); // Infinite loop?
					break;
				}
				CVConGuard VCon;
				if (CVConGroup::GetActiveVCon(&VCon) >= 0)
					ExecPopupMenuCmd(tmp_System, VCon.VCon(), (int)(DWORD)wParam);
				result = 0;
			}
			else if (wParam != 0xF100)
			{
				#ifdef _DEBUG
				wchar_t szDbg[64]; swprintf_c(szDbg, L"OnSysCommand(%i)\n", (DWORD)wParam);
				DEBUGSTRSYS(szDbg);
				#endif

				// Зачем вообще SysCommand, полученный в ConEmu, перенаправлять в RealConsole?
				#if 0
				// иначе это приводит к потере фокуса и активации невидимой консоли,
				// перехвате стрелок клавиатуры, и прочей фигни...
				if (wParam<0xF000)
				{
					POSTMESSAGE(ghConWnd, WM_SYSCOMMAND, wParam, lParam, FALSE);
				}
				#endif

				if (wParam == SC_SYSMENUPOPUP_SECRET)
				{
					mn_TrackMenuPlace = tmp_System;
					ShowMenuHint(nullptr);
				}

				result = DefWindowProc(hWnd, WM_SYSCOMMAND, wParam, lParam);

				if (wParam == SC_SYSMENUPOPUP_SECRET)
				{
					mn_TrackMenuPlace = tmp_None;
					ShowMenuHint(nullptr);
				}
			}
		} // default:
	}

	return result;
}



void CConEmuMenu::CmdTaskPopupItem::Reset(CmdTaskPopupItemType newItemType, int newCmdId, LPCWSTR asName)
{
	AssertThis();
	*this = CmdTaskPopupItem{};

	this->ItemType = newItemType;
	this->nCmd = newCmdId;

	if (asName)
	{
		SetShortName(asName);
	}
}

void CConEmuMenu::CmdTaskPopupItem::SetShortName(LPCWSTR asName, const bool bRightQuote /*= false*/, LPCWSTR asHotKey /*= nullptr*/)
{
	SetMenuName(this->szShort, countof(this->szShort), asName, (ItemType == eTaskPopup), bRightQuote, asHotKey);
}

void CConEmuMenu::CmdTaskPopupItem::SetMenuName(
	wchar_t* pszDisplay, const size_t cchDisplayMax, LPCWSTR asName, const bool bTrailingPeriod,
	const bool bRightQuote /*= false*/, LPCWSTR asHotKey /*= nullptr*/)
{
	const size_t nCurLen = _tcslen(pszDisplay);
	const size_t nLen = _tcslen(asName);

	const wchar_t* pszSrc = asName;
	wchar_t* pszDst = pszDisplay + nCurLen;
	wchar_t* pszEnd = pszDisplay + cchDisplayMax - 1;
	_ASSERTE((pszDst + std::min<size_t>(8U, 2 * nLen)) <= pszEnd); // Should fit

	if (!bTrailingPeriod)
	{
		if (nLen >= cchDisplayMax)  // NOLINT(clang-diagnostic-sign-compare)
		{
			*(pszDst++) = /*…*/L'\x2026';
			pszSrc = asName + nLen - cchDisplayMax + nCurLen + 2;
			_ASSERTE((pszSrc >= asName) && (pszSrc < (asName + nLen)));
		}
	}

	while (*pszSrc && (pszDst < pszEnd))
	{
		if (*pszSrc == L'&')
		{
			if ((pszDst + 2) >= pszEnd)
				break;
			*(pszDst++) = L'&';
		}

		*(pszDst++) = *(pszSrc++);
	}

	if (bTrailingPeriod && *pszSrc)
	{
		if ((pszDst + 1) >= pszEnd)
			pszDst = pszDisplay + cchDisplayMax - 2;

		*(pszDst++) = /*…*/L'\x2026';
	}

	// Для тасков, показать "»" когда они (сейчас) не разворачиваются в SubMenu
	wchar_t szRight[36] = L"";
	if (bRightQuote)
	{
		if (asHotKey && *asHotKey)
		{
			szRight[0] = L'\t';
			lstrcpyn(szRight + 1, asHotKey, 32);
			wcscat_c(szRight, L" \xBB");
		}
		else
		{
			wcscpy_c(szRight, L"\t\xBB");
		}
	}
	else if (asHotKey && *asHotKey)
	{
		szRight[0] = L'\t';
		lstrcpyn(szRight + 1, asHotKey, 34);
	}

	if (szRight[0])
	{
		const size_t nRight = _tcslen(szRight);
		_ASSERTE((nRight + 10) < cchDisplayMax);

		if ((pszDst + nRight) >= pszEnd)
			pszDst = pszDisplay + std::max<ssize_t>(0, (cchDisplayMax - (nRight + 1)));

		_wcscpy_c(pszDst, cchDisplayMax - (pszDst - pszDisplay), szRight);
		pszDst += _tcslen(pszDst);
		//*(pszDst++) = L'\t';
		//*(pszDst++) = 0xBB /* RightArrow/Quotes */;
	}

	// Terminate with '\0'
	if (pszDst <= pszEnd)
	{
		*pszDst = 0;
	}
	else
	{
		_ASSERTE(pszDst <= pszEnd)
			pszDisplay[cchDisplayMax - 1] = 0;
	}
}
