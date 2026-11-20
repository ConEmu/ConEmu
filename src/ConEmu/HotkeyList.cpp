
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

#include "Header.h"
#include "HotkeyList.h"
#include "ConEmuCtrl.h"
#include "Options.h"
#include "../ConEmuCD/GuiHooks.h"

// ReSharper disable twice CppParameterMayBeConst
ConEmuHotKey& ConEmuHotKeyList::Add(const int descrLangId, const ConEmuHotKeyType hkType, LPCWSTR name, HotkeyFKey_t keyFunc)
{
	static ConEmuHotKey dummy = {};
	ConEmuHotKey* p = &dummy;

	#ifdef _DEBUG
	for (ssize_t i = 0; i < size(); i++)
	{
		if ((*this)[i].DescrLangID == descrLangId)
		{
			_ASSERTE(FALSE && "This item was already added!");
		}
	}
	#endif

	const auto iNew = this->push_back(ConEmuHotKey{});
	if (iNew >= 0 && iNew < this->size())
	{
		p = &((*this)[iNew]);
		memset(p, 0, sizeof(*p));
		p->DescrLangID = descrLangId;
		p->HkType = hkType;
		_ASSERTE(p->Enabled == nullptr);
		lstrcpyn(p->Name, name, countof(p->Name));
		p->fkey = keyFunc;
		_ASSERTE(p->OnKeyUp == false);
		_ASSERTE(p->GuiMacro == nullptr);
	}

	return *p;
}

void ConEmuHotKeyList::UpdateNumberModifier()
{
	const ConEmuModifiers mods = cvk_NumHost | CEVkMatch::GetFlagsFromMod(gpSet->HostkeyNumberModifier());

	for (ssize_t i = this->size() - 1; i >= 0; i--)
	{
		ConEmuHotKey& hk = (*this)[i];
		if (hk.HkType == chk_NumHost)
			hk.Key.Mod = mods;
	}
}

void ConEmuHotKeyList::UpdateArrowModifier()
{
	ConEmuModifiers Mods = cvk_ArrHost|CEVkMatch::GetFlagsFromMod(gpSet->HostkeyArrowModifier());

	for (ssize_t i = this->size() - 1; i >= 0; i--)
	{
		ConEmuHotKey& hk = (*this)[i];
		if (hk.HkType == chk_ArrHost)
			hk.Key.Mod = Mods;
	}
}

void ConEmuHotKeyList::ReleaseHotkeys()
{
	for (ssize_t i = size() - 1; i >= 0; i--)
	{
		SafeFree((*this)[i].GuiMacro);
	}
	clear();
}

const ConEmuHotKey* ConEmuHotKeyList::GetHotKeyPtr(const int idx)
{
	const ConEmuHotKey* pHK = nullptr;

	if (idx >= 0 && this)
	{
		if (idx < size())
		{
			pHK = &((*this)[idx]);
		}
		else
		{
			const int iHotkeys = static_cast<int>(size());
			const CommandTasks* pCmd = gpSet->CmdTaskGet(idx - iHotkeys);
			if (pCmd)
			{
				_ASSERTE(pCmd->HotKey.HkType==chk_Task && pCmd->HotKey.GetTaskIndex()==(idx-iHotkeys));
				pHK = &pCmd->HotKey;
			}
		}
	}

	return pHK;
}

// pRCon may be nullptr
// Returns either nullptr or valid pointer to ConEmuHotKey
const ConEmuHotKey* ConEmuHotKeyList::FindHotKey(const ConEmuChord& VkState, CRealConsole* pRCon)
{
	// На сами модификаторы - действий не вешается
	switch (VkState.Vk)
	{
	case VK_LWIN: case VK_RWIN:
	case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
	case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
	case VK_MENU: case VK_LMENU: case VK_RMENU:
		return nullptr;
	case 0:
		_ASSERTE(VkState.Vk!=0);
		return nullptr;
	}

	const ConEmuHotKey* p = nullptr;

	// Теперь бежим по mp_HotKeys и сравниваем требуемые модификаторы
	for (int i = 0;; i++)
	{
		const ConEmuHotKey* pi = GetHotKeyPtr(i);
		if (!pi)
			break;

		if (pi->HkType == chk_Modifier)
			continue;

		// Hotkey was not set in settings?
		if (!pi->Key.Vk)
			continue;

		// May be disabled by settings or context?
		if (pi->Enabled)
		{
			if (!pi->Enabled())
				continue;
		}

		// Do compare (chord keys are planned)
		if (!pi->Key.IsEqual(VkState))
			continue;

		// The function
		if (pi->fkey)
		{
			// Допускается ли этот хоткей в текущем контексте?
			if (pi->fkey(VkState, true, pi, pRCon))
			{
				p = pi;
				break; // Нашли
			}
		}
		else
		{
			// Хоткей должен знать, что он "делает"
			_ASSERTE(pi->fkey!=nullptr);
		}
	}

	return p;
}

// pRCon may be nullptr
// Returns: nullptr, ConEmuSkipHotKey, or valid pointer to ConEmuHotKey
const ConEmuHotKey* ConEmuHotKeyList::GetHotKeyInfo(const ConEmuChord& VkState, bool bKeyDown, CRealConsole* pRCon)
{
	const ConEmuHotKey* p = FindHotKey(VkState, pRCon);

	// Некоторые комбинации нужно обрабатывать "на отпускание" во избежание глюков с интерфейсом
	if (p)
	{
		// Поэтому проверяем, совпадает ли требование "нажатости"
		if (p->OnKeyUp == bKeyDown)
			p = ConEmuSkipHotKey;
	}

	return p;
}

int ConEmuHotKeyList::AllocateHotkeys()
{
	// All hotkeys list

	// Ранее у nLDragKey,nRDragKey был тип DWORD

	_ASSERTE(this->empty());

	/*
		*** User (Keys, Global) -- New chk_Global should be added to gRegisteredHotKeys
	*/
	Add(vkMinimizeRestore,chk_Global, L"MinimizeRestore",       CConEmuCtrl::key_MinimizeRestore)
		.SetHotKey(VK_OEM_3/*~*/,VK_CONTROL);
	Add(vkMinimizeRestor2,chk_Global, L"MinimizeRestore2",      CConEmuCtrl::key_MinimizeRestore)
		;
	Add(vkGlobalRestore,  chk_Global, L"GlobalRestore",         CConEmuCtrl::key_GlobalRestore)
		;
	Add(vkCdExplorerPath, chk_Global, L"CdExplorerPath"       ).SetMacro(L"PasteExplorerPath(1,1)")
		;
	Add(vkForceFullScreen,chk_Global, L"ForcedFullScreen",      CConEmuCtrl::key_ForcedFullScreen)
		.SetHotKey(VK_RETURN,VK_CONTROL,VK_LWIN,VK_MENU);
	/*
		-- New chk_Local should be added to gActiveOnlyHotKeys
	*/
	Add(vkSetFocusSwitch, chk_Local,  L"SwitchGuiFocus"       ).SetMacro(L"SetFocus(1)")
		.SetHotKey('Z',VK_LWIN);
	Add(vkSetFocusGui,    chk_Local,  L"SetFocusGui"          ).SetMacro(L"SetFocus(0)")
		;
	Add(vkSetFocusChild,  chk_Local,  L"SetFocusChild"        ).SetMacro(L"SetFocus(2)")
		;
	Add(vkChildSystemMenu,chk_Local,  L"ChildSystemMenu",       CConEmuCtrl::key_ChildSystemMenu);
	/*
		*** User (Keys)
	*/
	Add(vkSetFocusParent, chk_User,   L"SetFocusParent",        CConEmuCtrl::key_InsideSetFocusParent)
		.SetHotKey(VK_ESCAPE,VK_LSHIFT);
	Add(vkCheckUpdates,    chk_User,  L"CheckUpdates"         ).SetMacro(L"Update()")
		.SetHotKey('U',VK_LWIN,VK_SHIFT);
	Add(vkMultiNew,        chk_User,  L"Multi.NewConsole"     ).SetMacro(L"Create()") // it can be used to create multiple consoles by holding Win+W
		.SetHotKey('W',VK_LWIN);
	Add(vkMultiNewConfirm, chk_User,  L"Multi.NewConsoleShift").SetMacro(L"Create(0,1)")
		.SetHotKey('W',VK_LWIN,VK_SHIFT);
	Add(vkMultiCmd,        chk_User,  L"Multi.CmdKey",          CConEmuCtrl::key_MultiCmd)
		.SetHotKey('X',VK_LWIN);
	Add(vkMultiWnd,        chk_User,  L"Multi.NewWndConfirm"  ).SetMacro(L"Create(2)")
		;
	Add(vkMultiWndConfirm, chk_User,  L"Multi.NewWndConfirm"  ).SetMacro(L"Create(2,1)")
		;
	Add(vkMultiNewPopup,   chk_User,  L"Multi.NewConsolePopup", CConEmuCtrl::key_MultiNewPopupMenu)
		.SetHotKey('N',VK_LWIN);
	Add(vkMultiNewPopup2,  chk_User,  L"Multi.NewConsolePopup2",CConEmuCtrl::key_MultiNewPopupMenu2)
		;
	Add(vkMultiNewAttach,  chk_User,  L"Multi.NewAttach",       CConEmuCtrl::key_MultiNewAttach)
		.SetHotKey('G',VK_LWIN,VK_SHIFT).SetOnKeyUp();
	/*
			Splitters
	*/
	Add(vkSplitNewConV,    chk_User,  L"Multi.NewSplitV"      ).SetMacro(L"Split(0,0,50)")
		.SetHotKey('O',VK_CONTROL,VK_SHIFT);
	Add(vkSplitNewConH,    chk_User,  L"Multi.NewSplitH"      ).SetMacro(L"Split(0,50,0)")
		.SetHotKey('E',VK_CONTROL,VK_SHIFT);
	Add(vkMaximizePane,   chk_User,  L"Multi.SplitMaximize"   ).SetMacro(L"Split(3)") // Maximize/restore active split
		.SetHotKey(VK_RETURN, VK_APPS);
	Add(vkSplitSizeVup,    chk_User,  L"Multi.SplitSizeVU"    ).SetMacro(L"Split(1,0,-1)")
		.SetHotKey(VK_UP,VK_APPS,VK_SHIFT);
	Add(vkSplitSizeVdown , chk_User,  L"Multi.SplitSizeVD"    ).SetMacro(L"Split(1,0,1)")
		.SetHotKey(VK_DOWN,VK_APPS,VK_SHIFT);
	Add(vkSplitSizeHleft,  chk_User,  L"Multi.SplitSizeHL"    ).SetMacro(L"Split(1,-1,0)")
		.SetHotKey(VK_LEFT,VK_APPS,VK_SHIFT);
	Add(vkSplitSizeHright, chk_User,  L"Multi.SplitSizeHR"    ).SetMacro(L"Split(1,1,0)")
		.SetHotKey(VK_RIGHT,VK_APPS,VK_SHIFT);
	Add(vkTabPane,         chk_User,  L"Key.TabPane1"         ).SetMacro(L"Tab(10,1)") // Next visible pane
		.SetHotKey(VK_TAB,VK_APPS);
	Add(vkTabPaneShift,    chk_User,  L"Key.TabPane2"         ).SetMacro(L"Tab(10,-1)") // Prev visible pane
		.SetHotKey(VK_TAB,VK_APPS,VK_SHIFT);
	Add(vkSplitFocusUp,    chk_User,  L"Multi.SplitFocusU"    ).SetMacro(L"Split(2,0,-1)")
		.SetHotKey(VK_UP,VK_APPS);
	Add(vkSplitFocusDown,  chk_User,  L"Multi.SplitFocusD"    ).SetMacro(L"Split(2,0,1)")
		.SetHotKey(VK_DOWN,VK_APPS);
	Add(vkSplitFocusLeft,  chk_User,  L"Multi.SplitFocusL"    ).SetMacro(L"Split(2,-1,0)")
		.SetHotKey(VK_LEFT,VK_APPS);
	Add(vkSplitFocusRight, chk_User,  L"Multi.SplitFocusR"    ).SetMacro(L"Split(2,1,0)")
		.SetHotKey(VK_RIGHT,VK_APPS);
	Add(vkSplitSwap,       chk_User,  L"Multi.SplitSwap"      ).SetMacro(L"Split(4)")
		.SetHotKey('X',VK_APPS,VK_MENU);
	Add(vkSplitSwapUp,     chk_User,  L"Multi.SplitSwapU"     ).SetMacro(L"Split(4,0,-1)")
		.SetHotKey(VK_UP,VK_APPS,VK_MENU);
	Add(vkSplitSwapDown,   chk_User,  L"Multi.SplitSwapD"     ).SetMacro(L"Split(4,0,1)")
		.SetHotKey(VK_DOWN,VK_APPS,VK_MENU);
	Add(vkSplitSwapLeft,   chk_User,  L"Multi.SplitSwapL"     ).SetMacro(L"Split(4,-1,0)")
		.SetHotKey(VK_LEFT,VK_APPS,VK_MENU);
	Add(vkSplitSwapRight,  chk_User,  L"Multi.SplitSwapR"     ).SetMacro(L"Split(4,1,0)")
		.SetHotKey(VK_RIGHT,VK_APPS,VK_MENU);
	Add(vkMultiNext,       chk_User,  L"Multi.Next",            CConEmuCtrl::key_MultiNext)
		.SetHotKey('Q',VK_LWIN,VK_SHIFT);
	Add(vkMultiNextShift,  chk_User,  L"Multi.NextShift",       CConEmuCtrl::key_MultiPrev)
		;
	Add(vkMultiRecreate,   chk_User,  L"Multi.Recreate",        CConEmuCtrl::key_MultiRecreate)
		.SetHotKey(192/*VK_tilde*/,VK_LWIN);
	Add(vkMultiAltCon,     chk_User,  L"Multi.AltCon",          CConEmuCtrl::key_AlternativeBuffer)
		.SetHotKey('A',VK_LWIN);
	Add(vkMultiPause,      chk_User,  L"Multi.Pause"          ).SetMacro(L"Pause")
		.SetHotKey(VK_PAUSE);
	Add(vkMultiBuffer,     chk_User,  L"Multi.Scroll",          CConEmuCtrl::key_MultiBuffer)
		;
    Add(vkSwitchTermMode,  chk_User,  L"Multi.SwitchTermMode", CConEmuCtrl::key_SwitchTermMode)
        .SetHotKey('M',VK_CONTROL,VK_SHIFT);
	Add(vkMultiGroup,      chk_User,  L"Multi.GroupInput"     ).SetMacro(L"GroupInput(0)")
		.SetHotKey('G', VK_APPS);
	Add(vkMultiGroupAll,   chk_User,  L"Multi.GroupInputAll"  ).SetMacro(L"GroupInput(3)")
		.SetHotKey('G', VK_APPS, VK_SHIFT);
	Add(vkMultiGroupKey,   chk_User,  L"Multi.GroupInputKey"  ).SetMacro(L"GroupInput(6)")
		.SetHotKey('G', VK_APPS, VK_MENU);
	Add(vkConDetach,       chk_User,  L"Multi.Detach"         ).SetMacro(L"Detach")
		;
	Add(vkConUnfasten,     chk_User,  L"Multi.Unfasten"       ).SetMacro(L"Unfasten")
		;
	Add(vkMultiClose,      chk_User,  L"Multi.Close"          ).SetMacro(L"Close(0)")
		.SetHotKey(VK_DELETE,VK_LWIN); // Close active console
	Add(vkCloseTab,        chk_User,  L"CloseTabKey"          ).SetMacro(L"Close(6)")
		.SetHotKey(VK_DELETE,VK_LWIN,VK_MENU); // Close tab (may be editor/viewer too)
	Add(vkCloseGroup,      chk_User,  L"CloseGroupKey"        ).SetMacro(L"Close(4)")
		.SetHotKey(VK_DELETE,VK_LWIN,VK_CONTROL); // Close active group
	Add(vkCloseGroupPrc,   chk_User,  L"CloseGroupPrcKey"     ).SetMacro(L"Close(7)")
		;
	Add(vkCloseAllCon,     chk_User,  L"CloseAllConKey"       ).SetMacro(L"Close(8)")
		;
	Add(vkCloseZombies,    chk_User,  L"CloseZombiesKey"      ).SetMacro(L"Close(9)")
		;
	Add(vkCloseExceptCon,  chk_User,  L"CloseExceptConKey"    ).SetMacro(L"Close(5)")
		;
	Add(vkClose2Right,     chk_User,  L"CloseToRightKey"      ).SetMacro(L"Close(11)")
		;
	Add(vkTerminateApp,    chk_User,  L"KillProcessKey"       ).SetMacro(L"Close(1)")
		.SetHotKey(VK_CANCEL,VK_CONTROL,VK_MENU); // Ctrl+Alt+Break
	Add(vkTermButShell,    chk_User,  L"KillAllButShellKey"   ).SetMacro(L"Close(10,1)")
		.SetHotKey(VK_PAUSE,VK_LWIN,VK_MENU); // Win+Alt+Break
	Add(vkDuplicateRoot,   chk_User,  L"DuplicateRootKey",      CConEmuCtrl::key_DuplicateRoot)
		.SetHotKey('S',VK_LWIN);
	Add(vkCloseConEmu,     chk_User,  L"CloseConEmuKey"       ).SetMacro(L"Close(2)")
		.SetHotKey(VK_F4,VK_LWIN); // sort of AltF4 for GUI apps
	Add(vkRenameTab,       chk_User,  L"Multi.Rename",          CConEmuCtrl::key_RenameTab)
		.SetHotKey('R',VK_APPS).SetOnKeyUp();
	Add(vkAffinity,        chk_User,  L"AffinityPriorityKey"  ).SetMacro(L"AffinityPriority")
		.SetHotKey('A',VK_APPS).SetOnKeyUp();
	Add(vkMoveTabLeft,     chk_User,  L"Multi.MoveLeft",        CConEmuCtrl::key_MoveTabLeft)
		.SetHotKey(VK_LEFT,VK_LWIN,VK_MENU);
	Add(vkMoveTabRight,    chk_User,  L"Multi.MoveRight",       CConEmuCtrl::key_MoveTabRight)
		.SetHotKey(VK_RIGHT,VK_LWIN,VK_MENU);
	Add(vkCTSVkBlockStart, chk_User,  L"CTS.VkBlockStart",      CConEmuCtrl::key_CTSVkBlockStart) // запуск выделения блока
		;
	Add(vkCTSVkTextStart,  chk_User,  L"CTS.VkTextStart",       CConEmuCtrl::key_CTSVkTextStart)  // запуск выделения текста
		;
	Add(vkCTSCopyHtml0,    chk_User,  L"CTS.VkCopyFmt0"       ).SetMacro(L"Copy(0,0)")
		.SetHotKey('C',VK_CONTROL).SetEnabled(ConEmuHotKey::InSelection);
	Add(vkCTSCopyHtml1,    chk_User,  L"CTS.VkCopyFmt1"       ).SetMacro(L"Copy(0,1)")
		.SetHotKey('C',VK_CONTROL,VK_SHIFT).SetEnabled(ConEmuHotKey::InSelection);
	Add(vkCTSCopyHtml2,    chk_User,  L"CTS.VkCopyFmt2"       ).SetMacro(L"Copy(0,2)")
		.SetEnabled(ConEmuHotKey::InSelection);
	Add(vkCTSVkCopyAll,    chk_User,  L"CTS.VkCopyAll"        ).SetMacro(L"Copy(1)")
		;
	Add(vkHighlightMouse,  chk_User,  L"HighlightMouseSwitch" ).SetMacro(L"HighlightMouse(1)")
		.SetHotKey('L',VK_APPS);
	Add(vkHighlightMouseX, chk_User,  L"HighlightMouseSwitchX").SetMacro(L"HighlightMouse(3)")
		.SetHotKey('X',VK_APPS);
	Add(vkShowTabsList,    chk_User,  L"Multi.ShowTabsList",    CConEmuCtrl::key_ShowTabsList)
		;
	Add(vkShowTabsList2,   chk_User,  L"Multi.ShowTabsList2"  ).SetMacro(L"Tabs(8)")
		.SetHotKey(VK_F12,VK_APPS);
	Add(vkPasteText,       chk_User,  L"ClipboardVkAllLines",   CConEmuCtrl::key_PasteText)
		.SetHotKey('V',VK_CONTROL);
	Add(vkPasteFirstLine,  chk_User,  L"ClipboardVkFirstLine",  CConEmuCtrl::key_PasteFirstLine)
		.SetHotKey(VK_INSERT,VK_SHIFT);
	Add(vkAltNumpad,       chk_User,  L"Key.AltNumpad"        ).SetMacro(L"AltNumber(16)")
		;
	Add(vkDeleteLeftWord,  chk_User,  L"DeleteWordToLeft",      CConEmuCtrl::key_DeleteWordToLeft)
		.SetHotKey(VK_BACK,VK_CONTROL).SetEnabled(ConEmuHotKey::UseCtrlBS);
	Add(vkFindTextDlg,     chk_User,  L"FindTextKey",           CConEmuCtrl::key_FindTextDlg)
		.SetHotKey('F',VK_APPS);
	Add(vkScreenshot,      chk_User,  L"ScreenshotKey",         CConEmuCtrl::key_Screenshot/*, true/ *OnKeyUp*/)
		.SetHotKey('H',VK_LWIN).SetOnKeyUp();
	Add(vkScreenshotFull,  chk_User,  L"ScreenshotFullKey",     CConEmuCtrl::key_ScreenshotFull/*, true/ *OnKeyUp*/)
		.SetHotKey('H',VK_LWIN,VK_SHIFT).SetOnKeyUp();
	Add(vkShowStatusBar,   chk_User,  L"ShowStatusBarKey",      CConEmuCtrl::key_ShowStatusBar)
		.SetHotKey('S',VK_APPS);
	Add(vkShowTabBar,      chk_User,  L"ShowTabBarKey",         CConEmuCtrl::key_ShowTabBar)
		.SetHotKey('T',VK_APPS);
	Add(vkShowCaption,     chk_User,  L"ShowCaptionKey",        CConEmuCtrl::key_ShowCaption)
		.SetHotKey('C',VK_APPS);
	Add(vkAlwaysOnTop,     chk_User,  L"AlwaysOnTopKey",        CConEmuCtrl::key_AlwaysOnTop)
		;
	Add(vkTransparencyInc, chk_User,  L"TransparencyInc"      ).SetMacro(L"Transparency(1,-20)")
		;
	Add(vkTransparencyDec, chk_User,  L"TransparencyDec"      ).SetMacro(L"Transparency(1,+20)")
		;
	Add(vkEditMenu,        chk_User,  L"Key.EditMenu",          CConEmuCtrl::key_EditMenu) // Edit menu
		.SetOnKeyUp();
	Add(vkEditMenu2,       chk_User,  L"Key.EditMenu2",         CConEmuCtrl::key_EditMenu) // Edit menu
		.SetOnKeyUp();
	Add(vkTabMenu,         chk_User,  L"Key.TabMenu",           CConEmuCtrl::key_TabMenu) // Tab menu
		.SetHotKey(VK_SPACE,VK_APPS).SetOnKeyUp();
	Add(vkTabMenu2,         chk_User, L"Key.TabMenu2",          CConEmuCtrl::key_TabMenu) // Tab menu
		.SetOnKeyUp();
	Add(vkMaximize,        chk_User,  L"Key.Maximize"         ).SetMacro(L"WindowMaximize()") // Maximize window
		.SetHotKey(VK_F9,VK_MENU);
	Add(vkMaximizeWidth,   chk_User,  L"Key.MaximizeWidth"    ).SetMacro(L"WindowMode(TWidth)") // Maximize window width
		.SetHotKey(VK_DOWN,VK_LWIN,VK_SHIFT);
	Add(vkMaximizeHeight,  chk_User,  L"Key.MaximizeHeight"   ).SetMacro(L"WindowMode(THeight)") // Maximize window height
		.SetHotKey(VK_UP,VK_LWIN,VK_SHIFT);
	Add(vkTileToLeft,      chk_User,  L"Key.TileToLeft"       ).SetMacro(L"WindowMode(TLeft)")
		.SetHotKey(VK_LEFT,VK_LWIN).SetEnabled(ConEmuHotKey::UseWinMove);
	Add(vkTileToRight,     chk_User,  L"Key.TileToRight"      ).SetMacro(L"WindowMode(TRight)")
		.SetHotKey(VK_RIGHT,VK_LWIN).SetEnabled(ConEmuHotKey::UseWinMove);
	Add(vkJumpActiveMonitor,chk_User, L"Key.JumpActiveMonitor").SetMacro(L"WindowMode(Here)")
		;
	Add(vkJumpPrevMonitor,chk_User,   L"Key.JumpPrevMonitor"  ).SetMacro(L"WindowMode(MPrev)")
		.SetHotKey(VK_LEFT,VK_LWIN,VK_SHIFT).SetEnabled(ConEmuHotKey::UseWinMove);
	Add(vkJumpNextMonitor, chk_User,  L"Key.JumpNextMonitor"  ).SetMacro(L"WindowMode(MNext)")
		.SetHotKey(VK_RIGHT,VK_LWIN,VK_SHIFT).SetEnabled(ConEmuHotKey::UseWinMove);
	Add(vkAltEnter,        chk_User,  L"Key.FullScreen"       ).SetMacro(L"WindowFullscreen()") // Full screen
		.SetHotKey(VK_RETURN,VK_MENU);
	Add(vkSystemMenu,      chk_User,  L"Key.SysMenu",           CConEmuCtrl::key_SystemMenu) // System menu
		.SetHotKey(VK_SPACE,VK_MENU);
	Add(vkSystemMenu2 ,    chk_User,  L"Key.SysMenu2",          CConEmuCtrl::key_SystemMenu) // System menu
		.SetHotKey(VK_RBUTTON,VK_CONTROL).SetOnKeyUp();
	Add(vkDebugProcess,    chk_User,  L"Key.DebugProcess"     ).SetMacro(L"Debug(0)")
		.SetHotKey('D',VK_LWIN,VK_SHIFT).SetOnKeyUp();
	Add(vkDumpProcess,     chk_User,  L"Key.DumpProcess"      ).SetMacro(L"Debug(1)")
		.SetOnKeyUp();
	Add(vkDumpTree,        chk_User,  L"Key.DumpTree"         ).SetMacro(L"Debug(2)")
		.SetOnKeyUp();
	/*
			Scrolling
	*/
	Add(vkCtrlUp,          chk_User,  L"Key.BufUp",             CConEmuCtrl::key_BufferScrollUp) // Buffer scroll
		.SetHotKey(VK_UP,VK_CONTROL);
	Add(vkCtrlDown,        chk_User,  L"Key.BufDn",             CConEmuCtrl::key_BufferScrollDown) // Buffer scroll
		.SetHotKey(VK_DOWN,VK_CONTROL);
	Add(vkCtrlPgUp,        chk_User,  L"Key.BufPgUp",           CConEmuCtrl::key_BufferScrollPgUp) // Buffer scroll
		.SetHotKey(VK_PRIOR,VK_CONTROL);
	Add(vkCtrlPgDn,        chk_User,  L"Key.BufPgDn",           CConEmuCtrl::key_BufferScrollPgDn) // Buffer scroll
		.SetHotKey(VK_NEXT,VK_CONTROL);
	Add(vkAppsPgUp,        chk_User,  L"Key.BufHfPgUp"        ).SetMacro(L"Scroll(2,-1)") // Buffer scroll
		.SetHotKey(VK_PRIOR,VK_APPS);
	Add(vkAppsPgDn,        chk_User,  L"Key.BufHfPgDn"        ).SetMacro(L"Scroll(2,+1)") // Buffer scroll
		.SetHotKey(VK_NEXT,VK_APPS);
	Add(vkAppsHome,        chk_User,  L"Key.BufTop"           ).SetMacro(L"Scroll(3,-1)") // Buffer scroll
		.SetHotKey(VK_HOME,VK_APPS);
	Add(vkAppsEnd,         chk_User,  L"Key.BufBottom"        ).SetMacro(L"Scroll(3,+1)") // Buffer scroll
		.SetHotKey(VK_END,VK_APPS);
	Add(vkFindPrevPrompt,  chk_User, L"Key.BufPrUp"           ).SetMacro(L"Scroll(5,-1)") // Buffer scroll: Find prompt upward
		.SetHotKey(VK_PRIOR,VK_CONTROL,VK_MENU).SetEnabled(ConEmuHotKey::UsePromptFind);
	Add(vkFindNextPrompt,  chk_User, L"Key.BufPrDn"           ).SetMacro(L"Scroll(5,+1)") // Buffer scroll: Find prompt downward
		.SetHotKey(VK_NEXT,VK_CONTROL,VK_MENU).SetEnabled(ConEmuHotKey::UsePromptFind);
	Add(vkAppsBS,          chk_User,  L"Key.BufCursor"        ).SetMacro(L"Scroll(4)") // Buffer scroll
		.SetHotKey(VK_BACK,VK_APPS);
	Add(vkResetTerminal,   chk_User,  L"Key.ResetTerm",         CConEmuCtrl::key_ResetTerminal) // Reset terminal
		;
	/*
			Usability
	*/
	Add(vkFontLarger,      chk_User,  L"FontLargerKey"        ).SetMacro(L"FontSetSize(1,2)")
		.SetHotKey(VK_WHEEL_UP,VK_CONTROL);
	Add(vkFontSmaller,     chk_User,  L"FontSmallerKey"       ).SetMacro(L"FontSetSize(1,-2)")
		.SetHotKey(VK_WHEEL_DOWN,VK_CONTROL);
	Add(vkFontOriginal,    chk_User,  L"FontOriginalKey"      ).SetMacro(L"Zoom(100)")
		.SetHotKey(VK_MBUTTON,VK_CONTROL);
	Add(vkPasteFilePath,   chk_User,  L"PasteFileKey"         ).SetMacro(L"Paste(4)")
		.SetHotKey('F',VK_CONTROL,VK_SHIFT);
	Add(vkPasteDirectory,  chk_User,  L"PastePathKey"         ).SetMacro(L"Paste(5)")
		.SetHotKey('D',VK_CONTROL,VK_SHIFT);
	Add(vkPasteCygwin,     chk_User,  L"PasteCygwinKey"       ).SetMacro(L"Paste(8)")
		.SetHotKey(VK_INSERT,VK_APPS);
	/*
		*** GUI Macros
	*/
	Add(vkGuiMacro01,      chk_Macro, L"KeyMacro01",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro02,      chk_Macro, L"KeyMacro02",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro03,      chk_Macro, L"KeyMacro03",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro04,      chk_Macro, L"KeyMacro04",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro05,      chk_Macro, L"KeyMacro05",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro06,      chk_Macro, L"KeyMacro06",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro07,      chk_Macro, L"KeyMacro07",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro08,      chk_Macro, L"KeyMacro08",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro09,      chk_Macro, L"KeyMacro09",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro10,      chk_Macro, L"KeyMacro10",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro11,      chk_Macro, L"KeyMacro11",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro12,      chk_Macro, L"KeyMacro12",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro13,      chk_Macro, L"KeyMacro13",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro14,      chk_Macro, L"KeyMacro14",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro15,      chk_Macro, L"KeyMacro15",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro16,      chk_Macro, L"KeyMacro16",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro17,      chk_Macro, L"KeyMacro17",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro18,      chk_Macro, L"KeyMacro18",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro19,      chk_Macro, L"KeyMacro19",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro20,      chk_Macro, L"KeyMacro20",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro21,      chk_Macro, L"KeyMacro21",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro22,      chk_Macro, L"KeyMacro22",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro23,      chk_Macro, L"KeyMacro23",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro24,      chk_Macro, L"KeyMacro24",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro25,      chk_Macro, L"KeyMacro25",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro26,      chk_Macro, L"KeyMacro26",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro27,      chk_Macro, L"KeyMacro27",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro28,      chk_Macro, L"KeyMacro28",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro29,      chk_Macro, L"KeyMacro29",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro30,      chk_Macro, L"KeyMacro30",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro31,      chk_Macro, L"KeyMacro31",            CConEmuCtrl::key_GuiMacro);
	Add(vkGuiMacro32,      chk_Macro, L"KeyMacro32",            CConEmuCtrl::key_GuiMacro);
	/*
		*** User (Modifiers)
	*/
	Add(vkCTSVkBlock,      chk_Modifier, L"CTS.VkBlock")     // modifier to start mouse selection
		.SetHotKey(VK_LMENU);
	Add(vkCTSVkText,       chk_Modifier, L"CTS.VkText")      // modifier to start mouse selection
		.SetHotKey(VK_LSHIFT);
	Add(vkCTSVkAct,        chk_Modifier, L"CTS.VkAct")       // modifier to allow actions by right/middle mouse button
		;
	Add(vkCTSVkPromptClk,  chk_Modifier, L"CTS.VkPrompt")    // modifier to position text cursor by mouse click (cmd.exe prompt)
		;
	Add(vkFarGotoEditorVk, chk_Modifier, L"FarGotoEditorVk") // modifier for isFarGotoEditor
		.SetHotKey(VK_LCONTROL);
	Add(vkLDragKey,        chk_Modifier, L"DndLKey")         // modifier to drag by left mouse button
		.SetEnabled(ConEmuHotKey::UseDndLKey);
	Add(vkRDragKey,        chk_Modifier, L"DndRKey")         // modifier to drag by right mouse button
		.SetHotKey(VK_LCONTROL).SetEnabled(ConEmuHotKey::UseDndLKey);
	Add(vkWndDragKey,      chk_Modifier2,L"WndDragKey",  CConEmuCtrl::key_WinDragStart) // modifier to drag ConEmu by any place in the window
		.SetHotKey(VK_LBUTTON,VK_CONTROL,VK_MENU).SetEnabled(ConEmuHotKey::UseWndDragKey);
	/*
		*** System (predefined, fixed)
	*/
	Add(vkWinAltA,         chk_System, L""             ).SetMacro(L"About()")               // Show ‘About’ dialog
		.SetHotKey('A',VK_LWIN,VK_MENU).SetOnKeyUp();
	Add(vkWinAltP,         chk_System, L""             ).SetMacro(L"Settings()")            // Settings dialog
		.SetHotKey('P',VK_LWIN,VK_MENU).SetOnKeyUp();
	Add(vkWinAltK,         chk_System, L""             ).SetMacro(L"Settings(171)")         // Setup ‘Hotkeys’
		.SetHotKey('K',VK_LWIN,VK_MENU).SetOnKeyUp();
	Add(vkWinAltT,         chk_System, L""             ).SetMacro(L"Settings(157)")         // Setup ‘Tasks’
		.SetHotKey('T',VK_LWIN,VK_MENU).SetOnKeyUp();
	Add(vkWinAltH,         chk_System, L""             ).SetMacro(L"Wiki()")                // Show online wiki
		.SetHotKey('H',VK_LWIN,VK_MENU).SetOnKeyUp();
	Add(vkWinAltSpace,     chk_System, L"",              CConEmuCtrl::key_SystemMenu)       // Show ConEmu menu
		.SetHotKey(VK_SPACE,VK_LWIN,VK_MENU).SetOnKeyUp();
	Add(vkCtrlWinAltSpace, chk_System, L"",              CConEmuCtrl::key_ShowRealConsole)  // Show real console
		.SetHotKey(VK_SPACE, VK_CONTROL,VK_LWIN,VK_MENU);
	Add(vkCtrlWinEnter,    chk_System, L""             ).SetMacro(L"WindowFullscreen()")
		.SetHotKey(VK_RETURN,VK_LWIN,VK_CONTROL);
	Add(vkCtrlTab,         chk_System, L"",              CConEmuCtrl::key_CtrlTab)          // Tab switch
		.SetHotKey(VK_TAB,VK_CONTROL).SetEnabled(ConEmuHotKey::UseCtrlTab);
	Add(vkCtrlShiftTab,    chk_System, L"",              CConEmuCtrl::key_CtrlShiftTab)     // Tab switch
		.SetHotKey(VK_TAB,VK_CONTROL,VK_SHIFT).SetEnabled(ConEmuHotKey::UseCtrlTab);
	Add(vkCtrlTab_Left,    chk_System, L"",              CConEmuCtrl::key_CtrlTab_Prev)     // Tab switch
		.SetHotKey(VK_LEFT,VK_CONTROL);
	Add(vkCtrlTab_Up,      chk_System, L"",              CConEmuCtrl::key_CtrlTab_Prev)     // Tab switch
		.SetHotKey(VK_UP,VK_CONTROL);
	Add(vkCtrlTab_Right,   chk_System, L"",              CConEmuCtrl::key_CtrlTab_Next)     // Tab switch
		.SetHotKey(VK_RIGHT,VK_CONTROL);
	Add(vkCtrlTab_Down,    chk_System, L"",              CConEmuCtrl::key_CtrlTab_Next)     // Tab switch
		.SetHotKey(VK_DOWN,VK_CONTROL);
	Add(vkEscNoConsoles,   chk_System, L"",              CConEmuCtrl::key_MinimizeByEsc)    // Minimize ConEmu by Esc when no open consoles left
		.SetHotKey(VK_ESCAPE);
	Add(vkCTSShiftLeft,    chk_System, L""             ).SetMacro(L"Select(0,-1)")
		.SetHotKey(VK_LEFT,VK_SHIFT).SetEnabled(ConEmuHotKey::UseCTSShiftArrow);
	Add(vkCTSShiftRight,   chk_System, L""             ).SetMacro(L"Select(0,1)")
		.SetHotKey(VK_RIGHT,VK_SHIFT).SetEnabled(ConEmuHotKey::UseCTSShiftArrow);
	Add(vkCTSCtrlShiftLeft,chk_System, L""             ).SetMacro(L"Select(0,-1,0,-2)")
		.SetHotKey(VK_LEFT,VK_CONTROL,VK_SHIFT).SetEnabled(ConEmuHotKey::UseCTSShiftArrow);
	Add(vkCTSCtrlShiftRight,chk_System, L""            ).SetMacro(L"Select(0,1,0,2)")
		.SetHotKey(VK_RIGHT,VK_CONTROL,VK_SHIFT).SetEnabled(ConEmuHotKey::UseCTSShiftArrow);
	Add(vkCTSShiftHome,    chk_System, L""             ).SetMacro(L"Select(0,-1,0,-1)")
		.SetHotKey(VK_HOME,VK_SHIFT).SetEnabled(ConEmuHotKey::UseCTSShiftArrow);
	Add(vkCTSShiftEnd,     chk_System, L""             ).SetMacro(L"Select(0,1,0,1)")
		.SetHotKey(VK_END,VK_SHIFT).SetEnabled(ConEmuHotKey::UseCTSShiftArrow);
	Add(vkCTSShiftUp,      chk_System, L""             ).SetMacro(L"Select(1,0,-1)")
		.SetHotKey(VK_UP,VK_SHIFT).SetEnabled(ConEmuHotKey::UseCTSShiftArrow);
	Add(vkCTSShiftDown,    chk_System, L""             ).SetMacro(L"Select(1,0,1)")
		.SetHotKey(VK_DOWN,VK_SHIFT).SetEnabled(ConEmuHotKey::UseCTSShiftArrow);
	/*
		*** [HostKey]+Arrows. Only [HostKey] (modifiers actually) can be configured
			Resize ConEmu window with Win+Arrows
	*/
	Add(vkWinLeft,         chk_ArrHost, L"",             CConEmuCtrl::key_WinWidthDec)  // Decrease window width
		.SetVkMod(VK_LEFT|CEHOTKEY_ARRHOSTKEY).SetEnabled(ConEmuHotKey::UseWinArrows);
	Add(vkWinRight,        chk_ArrHost, L"",             CConEmuCtrl::key_WinWidthInc)  // Increase window width
		.SetVkMod(VK_RIGHT|CEHOTKEY_ARRHOSTKEY).SetEnabled(ConEmuHotKey::UseWinArrows);
	Add(vkWinUp,           chk_ArrHost, L"",             CConEmuCtrl::key_WinHeightDec) // Decrease window height
		.SetVkMod(VK_UP|CEHOTKEY_ARRHOSTKEY).SetEnabled(ConEmuHotKey::UseWinArrows);
	Add(vkWinDown,         chk_ArrHost, L"",             CConEmuCtrl::key_WinHeightInc) // Increase window height
		.SetVkMod(VK_DOWN|CEHOTKEY_ARRHOSTKEY).SetEnabled(ConEmuHotKey::UseWinArrows);
	/*
		*** [HostKey]+Number. Only [HostKey] (modifiers actually) can be configured
			Console activate by number
	*/
	Add(vkConsole_1,       chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('1'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);
	Add(vkConsole_2,       chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('2'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);
	Add(vkConsole_3,       chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('3'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);
	Add(vkConsole_4,       chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('4'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);
	Add(vkConsole_5,       chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('5'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);
	Add(vkConsole_6,       chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('6'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);
	Add(vkConsole_7,       chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('7'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);
	Add(vkConsole_8,       chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('8'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);
	Add(vkConsole_9,       chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('9'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);
	Add(vkConsole_10,      chk_NumHost, L"",             CConEmuCtrl::key_ConsoleNum)
		.SetVkMod('0'|CEHOTKEY_NUMHOSTKEY).SetEnabled(ConEmuHotKey::UseWinNumber);

	// Reapply actual cvk_XXX flags to Win+Arrows
	UpdateNumberModifier();
	// and Win+Numbers
	UpdateArrowModifier();

	// these constant is used to allocate array of keys in keyhook proc
	// Settings -> Controls -> Install keyboard hooks
	const int nHotKeyCount = static_cast<int>(this->size());
	_ASSERTE(nHotKeyCount < (HookedKeysMaxCount - 1));

	// Informational
	return nHotKeyCount;
}
