
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

#define SHOWDEBUGSTR

#define HIDE_USE_EXCEPTION_INFO
#include "Header.h"
#include "../common/WUser.h"

#include "AboutDlg.h"
#include "ConEmu.h"
#include "ConEmuCtrl.h"

#include "ConfirmDlg.h"
#include "Hotkeys.h"
#include "FindDlg.h"
#include "LngRc.h"
#include "Macro.h"
#include "Menu.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "ScreenDump.h"
#include "SetCmdTask.h"
#include "TabBar.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

#define DEBUGSTRAPPS(s) DEBUGSTR(s)


// Текущая обрабатываемая клавиша
const ConEmuHotKey* gpCurrentHotKey = nullptr;

bool CConEmuCtrl::mb_SkipOneAppsRelease = false;
HHOOK CConEmuCtrl::mh_SkipOneAppsRelease = nullptr;


CConEmuCtrl::CConEmuCtrl()
{
}

CConEmuCtrl::~CConEmuCtrl()
{
}

// pRCon may be nullptr, pszChars may be nullptr
const ConEmuHotKey* CConEmuCtrl::ProcessHotKey(const ConEmuChord& VkState, bool bKeyDown, const wchar_t *pszChars, CRealConsole* pRCon)
{
	// For testing and checking purposes
	// User may disable "GuiMacro" processing with "ConEmu /NoHotkey"
	if (gpConEmu->DisableAllHotkeys)
	{
		return nullptr;
	}

	UINT vk = VkState.Vk;
	if (!(vk >= '0' && vk <= '9'))
		ResetDoubleKeyConsoleNum();

	const ConEmuHotKey* pHotKey = gpHotKeys->GetHotKeyInfo(VkState, bKeyDown, pRCon);
	gpCurrentHotKey = pHotKey;

	if (pHotKey == nullptr && pRCon && pRCon->isSelectionPresent())
	{
		pHotKey = pRCon->ProcessSelectionHotKey(VkState, bKeyDown, pszChars);
	}

	if (pHotKey && (pHotKey != ConEmuSkipHotKey))
	{
		// For testing and checking purposes
		// User may disable "GuiMacro" processing with "ConEmu /NoMacro"
		if (pHotKey && gpConEmu->DisableAllMacro)
		{
			if ((pHotKey->HkType == chk_Macro) || (pHotKey->GuiMacro && *pHotKey->GuiMacro))
			{
				pHotKey = nullptr;
			}
		}

		bool bEnabled = true;
		if (pHotKey && pHotKey->Enabled)
		{
			bEnabled = pHotKey->Enabled();
			if (!bEnabled)
			{
				pHotKey = nullptr;
			}
		}

		if (pHotKey)
		{
			// Чтобы у консоли не сносило крышу (FAR может выполнить макрос на Alt)
			if (((VkState.Mod & cvk_ALLMASK) == cvk_LAlt) || ((VkState.Mod & cvk_ALLMASK) == cvk_RAlt))
			{
				if (pRCon && gpSet->isFixAltOnAltTab)
					pRCon->PostKeyPress(VK_CONTROL, LEFT_ALT_PRESSED, 0);
			}

			// Теперь собственно действие
			if (pHotKey->fkey)
			{
				bool bApps = (VkState.Mod & cvk_Apps) == cvk_Apps;
				if (bApps)
					gpConEmu->SkipOneAppsRelease(true);

				pHotKey->fkey(VkState, false, pHotKey, pRCon);

				if (bApps && !isPressed(VK_APPS))
					gpConEmu->SkipOneAppsRelease(false);
			}
			else
			{
				_ASSERTE(pHotKey->fkey!=nullptr);
			}
		}
	}

	gpCurrentHotKey = nullptr;

	return pHotKey;
}

void CConEmuCtrl::UpdateControlKeyState()
{
	bCaps = (1 & (WORD)GetKeyState(VK_CAPITAL)) == 1;
	bNum = (1 & (WORD)GetKeyState(VK_NUMLOCK)) == 1;
	bScroll = (1 & (WORD)GetKeyState(VK_SCROLL)) == 1;
	bWin = isPressed(VK_LWIN) || isPressed(VK_RWIN);
	bApps = isPressed(VK_APPS);
	bLAlt = isPressed(VK_LMENU);
	bRAlt = isPressed(VK_RMENU);
	bLCtrl = isPressed(VK_LCONTROL);
	bRCtrl = isPressed(VK_RCONTROL);
	bLShift = isPressed(VK_LSHIFT);
	bRShift = isPressed(VK_RSHIFT);

	DWORD ControlKeyState = 0;

	//if (((DWORD)lParam & (DWORD)(1 << 24)) != 0)
	//	r.Event.KeyEvent.dwControlKeyState |= ENHANCED_KEY;

	if (bCaps)
		ControlKeyState |= CAPSLOCK_ON;

	if (bNum)
		ControlKeyState |= NUMLOCK_ON;

	if (bScroll)
		ControlKeyState |= SCROLLLOCK_ON;

	if (bLAlt)
		ControlKeyState |= LEFT_ALT_PRESSED;

	if (bRAlt)
		ControlKeyState |= RIGHT_ALT_PRESSED;

	if (bLCtrl)
		ControlKeyState |= LEFT_CTRL_PRESSED;

	if (bRCtrl)
		ControlKeyState |= RIGHT_CTRL_PRESSED;

	if (bLShift || bRShift)
		ControlKeyState |= SHIFT_PRESSED;

	dwControlKeyState = ControlKeyState;
}

// lParam - из сообщений WM_KEYDOWN/WM_SYSKEYDOWN/...
DWORD CConEmuCtrl::GetControlKeyState(LPARAM lParam)
{
	return dwControlKeyState | ((((DWORD)lParam & (DWORD)(1 << 24)) != 0) ? ENHANCED_KEY : 0);
}

// true - запретить передачу в консоль, сами обработали
// pRCon may be nullptr, pszChars may be nullptr
bool CConEmuCtrl::ProcessHotKeyMsg(UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars, CRealConsole* pRCon)
{
	_ASSERTE((messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN) || (messg == WM_KEYUP || messg == WM_SYSKEYUP));

	WARNING("CConEmuCtrl:: Наверное нужно еще какие-то пляски с бубном при отпускании хоткеев");
	WARNING("CConEmuCtrl:: Ибо в CConEmuMain::OnKeyboard была запутанная логика с sm_SkipSingleHostkey, sw_SkipSingleHostkey, sl_SkipSingleHostkey");

	// Обновить и подготовить "r.Event.KeyEvent.dwControlKeyState"
	UpdateControlKeyState();

	DWORD vk = (DWORD)(wParam & 0xFF);
	bool bKeyDown = (messg == WM_KEYDOWN || messg == WM_SYSKEYDOWN);
	bool bKeyUp = (messg == WM_KEYUP || messg == WM_SYSKEYUP);

	if (mn_DoubleKeyConsoleNum && (!(vk >= '0' && vk <= '9')))
	{
		//if (!(vk >= '0' && vk <= '9'))
		//	ResetDoubleKeyConsoleNum();

		int nNewIdx = -1;
		// попытка активации одной кнопкой
		if (mn_DoubleKeyConsoleNum>='1' && mn_DoubleKeyConsoleNum<='9')
			nNewIdx = mn_DoubleKeyConsoleNum - '1';
		else if (mn_DoubleKeyConsoleNum=='0')
			nNewIdx = 9;

		ResetDoubleKeyConsoleNum();

		if (nNewIdx >= 0)
			gpConEmu->ConActivate(nNewIdx);
	}

	if (bKeyUp)
	{
		if ((mb_InWinTabSwitch && (vk == VK_RWIN || vk == VK_LWIN))
			|| (mb_InCtrlTabSwitch && (vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL)))
		{
			mb_InWinTabSwitch = mb_InCtrlTabSwitch = FALSE;
			gpConEmu->TabCommand(ctc_SwitchCommit);
			WARNING("CConEmuCtrl:: В фар отпускание кнопки таки пропустим?");
		}
	}

	// На сами модификаторы - действий не вешается
	if (vk == VK_LWIN || vk == VK_RWIN /*|| vk == VK_APPS*/
		|| vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT
		|| vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL
		|| vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU)
	{
		if (pRCon)
		{
			// Однако, если это был одиночный обработанный модификатор - его нужно "пофиксить",
			// чтобы на его отпускание не выполнился Far-макрос например
			if (bKeyUp)
			{
				FixSingleModifier(vk, pRCon);
			}
			else
			{
				_ASSERTE(bKeyDown);
				int ModCount = 0;
				if (bLAlt) ModCount++;
				if (bRAlt) ModCount++;
				if (bLCtrl) ModCount++;
				if (bRCtrl) ModCount++;
				if (bLShift || bRShift) ModCount++;

				if ((ModCount == 1) && (vk != VK_APPS) && (vk != VK_LWIN))
				{
					mb_LastSingleModifier = FALSE;
					mn_LastSingleModifier =
						(vk == VK_LMENU || vk == VK_RMENU || vk == VK_MENU) ? VK_MENU :
						(vk == VK_LCONTROL || vk == VK_RCONTROL || vk == VK_CONTROL) ? VK_CONTROL :
						(vk == VK_LSHIFT || vk == VK_RSHIFT || vk == VK_SHIFT) ? VK_SHIFT : 0;

					if (!mn_LastSingleModifier)
					{
						// Win и прочие модификаторы здесь не интересуют
					}
					else
					{
						mn_SingleModifierFixState = dwControlKeyState;
						switch (mn_LastSingleModifier)
						{
						case VK_MENU:
							mn_SingleModifierFixKey = VK_CONTROL;
							mn_SingleModifierFixState |= (LEFT_CTRL_PRESSED);
							break;
						case VK_CONTROL:
							mn_SingleModifierFixKey = VK_MENU;
							mn_SingleModifierFixState |= (RIGHT_ALT_PRESSED);
							break;
						case VK_SHIFT:
							mn_SingleModifierFixKey = VK_MENU;
							mn_SingleModifierFixState |= RIGHT_ALT_PRESSED;
							break;
						}
					}
				}
				else if (ModCount > 1)
				{
					// Больше не нужно
					mb_LastSingleModifier = FALSE;
					mn_LastSingleModifier = mn_SingleModifierFixState = 0;
					mn_SingleModifierFixKey = 0;
				}
			}
		}
		return false;
	}

	ConEmuChord VkState = ChordFromVk(vk);

	if (bKeyDown)
		m_SkippedMsg = 0;

	const ConEmuHotKey* pHotKey = ProcessHotKey(VkState, bKeyDown, pszChars, pRCon);

	// Для "одиночных"
	if (pHotKey && mn_LastSingleModifier)
	{
		if (pHotKey != ConEmuSkipHotKey)
		{
			mb_LastSingleModifier = TRUE;
		}
	}
	else if (!pHotKey && !(VkState.Mod & (cvk_Ctrl|cvk_Alt|cvk_Shift)))
	{
		if (bKeyDown)
		{
			// Раз мы попали сюда - значит сам Apps у нас не хоткей, но может быть модификатором?
			if ((vk == VK_APPS) && gpSet->isModifierExist(LOBYTE(vk)))
			{
				m_SkippedMsg = messg; m_SkippedMsgWParam = wParam; m_SkippedMsgLParam = lParam;
				// Откладываем либо до
				// *) нажатия другой кнопки, не перехватываемой нами (например Apps+U)
				// *) отпускания самого Apps
				return ConEmuSkipHotKey;
			}
		}
		else if ((vk == VK_APPS) && m_SkippedMsg && pRCon)
		{
			// Отпускается Apps. Сначала нужно "дослать" в консоль ее нажатие
			pRCon->ProcessKeyboard(m_SkippedMsg, m_SkippedMsgWParam, m_SkippedMsgLParam, nullptr);
		}
	}

	if (((VkState.Mod & cvk_ALLMASK) == cvk_Win)
		&& (vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT))
	{
		//120821 - в режиме HideCaption почему-то не выходит из Maximized по Win+Down
		if (gpConEmu->isCaptionHidden())
		{
			if (vk == VK_DOWN)
			{
				if (::IsZoomed(ghWnd)/*тут нужен реальный Zoomed*/)
				{
					gpConEmu->SetWindowMode(wmNormal);
				}
			}
		}
	}

	return (pHotKey != nullptr);
}

// Warning! UpdateControlKeyState() must be called already!
ConEmuChord CConEmuCtrl::ChordFromVk(DWORD Vk)
{
	_ASSERTE((Vk & 0xFF) == Vk);

	ConEmuChord chord = {LOBYTE(Vk)};

	if (bWin)
		chord.Mod |= cvk_Win;

	if ((Vk != VK_APPS) && bApps)
		chord.Mod |= cvk_Apps;

	if (bLCtrl)
		chord.Mod |= cvk_LCtrl|cvk_Ctrl;
	if (bRCtrl)
		chord.Mod |= cvk_RCtrl|cvk_Ctrl;

	if (bLAlt)
		chord.Mod |= cvk_LAlt|cvk_Alt;
	if (bRAlt)
		chord.Mod |= cvk_RAlt|cvk_Alt;

	if (bLShift)
		chord.Mod |= cvk_LShift|cvk_Shift;
	if (bRShift)
		chord.Mod |= cvk_RShift|cvk_Shift;

	return chord;
}

void CConEmuCtrl::FixSingleModifier(DWORD Vk, CRealConsole* pRCon)
{
	if (pRCon && gpSet->isFixAltOnAltTab)
	{
		if (mn_LastSingleModifier && ((Vk == 0) || (mb_LastSingleModifier && (Vk == mn_LastSingleModifier))))
			pRCon->PostKeyPress(mn_SingleModifierFixKey, mn_SingleModifierFixState, 0);
	}
	// Больше не нужно
	mb_LastSingleModifier = FALSE;
	mn_LastSingleModifier = mn_SingleModifierFixState = 0;
	mn_SingleModifierFixKey = 0;
}

// User (Keys)
// pRCon may be nullptr
bool CConEmuCtrl::key_MinimizeRestore(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	// Должно обрабатываться через WM_HOTKEY
	_ASSERTE(FALSE && "CConEmuCtrl::key_MinimizeRestore");
	gpConEmu->DoMinimizeRestore();
	return true;
}

// When depressing Esc in ConEmu without consoles (Don't close ConEmu on last console close)
bool CConEmuCtrl::key_MinimizeByEsc(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (gpSet->isMultiMinByEsc == 0)
		return false;

	if (gpSet->isMultiMinByEsc == 2)
	{
		if (CVConGroup::GetConCount(true) > 0)
			return false;
	}

	if (TestOnly)
		return true;

	gpConEmu->DoMinimizeRestore(((gpSet->isMultiHideOnClose == 1) || gpSet->isMinToTray()) ? sih_HideTSA : sih_Minimize);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_GlobalRestore(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	// Должно обрабатываться через WM_HOTKEY
	_ASSERTE(FALSE && "CConEmuCtrl::key_GlobalRestore");
	gpConEmu->DoMinimizeRestore(sih_Show);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_MultiNewPopupMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	// Создать новую консоль
	gpConEmu->mp_Menu->OnNewConPopupMenu(nullptr, 0, false);
	return true;
}
bool CConEmuCtrl::key_MultiNewPopupMenu2(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	// Создать новую консоль
	gpConEmu->mp_Menu->OnNewConPopupMenu(nullptr, 0, true);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_MultiNewAttach(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	// Создать новую консоль
	gpConEmu->mp_Menu->OnSysCommand(ghWnd, IDM_ATTACHTO, 0);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_MultiNext(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	gpConEmu->ConActivateNext(FALSE/*abReverse*/);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_MultiPrev(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	gpConEmu->ConActivateNext(TRUE/*abReverse*/);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_MultiRecreate(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	// Подтверждение на закрытие/пересоздание консоли
	gpConEmu->RecreateAction(cra_RecreateTab/*TRUE*/, TRUE);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_AlternativeBuffer(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	gpConEmu->AskChangeAlternative();
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_MultiBuffer(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	gpConEmu->AskChangeBufferHeight();
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_SwitchTermMode(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
    if (TestOnly)
        return true;
    gpConEmu->AskChangeTermMode();
    return true;
}
// pRCon may be nullptr
bool CConEmuCtrl::key_DuplicateRoot(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	if (TestOnly)
		return true;

	// Almost the same as GuiMacro Shell("new_console:I") except of duplicate confirmation setting
	pRCon->DuplicateRoot();
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_DuplicateRootAs(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	if (TestOnly)
		return true;

	pRCon->AdminDuplicate();
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_MultiCmd(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	RConStartArgsEx args;
	CEStr lsTitle;
	gpSet->CmdTaskGetDefaultShell(args, lsTitle);

	gpConEmu->CreateCon(args, true);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_CTSVkBlockStart(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	if (TestOnly)
		return true;

	// Начать текстовое или блоковое выделение
	pRCon->StartSelection(FALSE/*abTextMode*/);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_CTSVkTextStart(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return FALSE;
	if (TestOnly)
		return true;

	// Начать текстовое или блоковое выделение
	pRCon->StartSelection(TRUE/*abTextMode*/);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_SystemMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* /*hk*/, CRealConsole* /*pRCon*/)
{
	if (TestOnly)
		return true;

	POINT ptCur = {-32000,-32000}; // Default pos of Alt+Space
	if (gpCurrentHotKey)
	{
		DWORD vk = gpCurrentHotKey->Key.Vk;
		if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON)
		{
			GetCursorPos(&ptCur);
		}
	}

	//Win-Alt-Space
	LogString(L"ShowSysmenu called from (key_SystemMenu)");
	gpConEmu->mp_Menu->ShowSysmenu(ptCur.x, ptCur.y);

	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_TabMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	if (TestOnly)
		return true;

	POINT ptCur = {-32000,-32000};
	if (gpCurrentHotKey)
	{
		DWORD vk = gpCurrentHotKey->Key.Vk;
		if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON)
		{
			GetCursorPos(&ptCur);
		}
	}
	if (ptCur.x == -32000)
		ptCur = gpConEmu->mp_Menu->CalcTabMenuPos(pRCon->VCon());

	//Win-Apps
	LogString(L"TabMenu called by hotkey");
	gpConEmu->mp_Menu->ShowPopupMenu(pRCon->VCon(), ptCur, TPM_LEFTALIGN, true);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_EditMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	if (TestOnly)
		return true;

	POINT ptCur = {-32000,-32000};
	if (gpCurrentHotKey)
	{
		DWORD vk = gpCurrentHotKey->Key.Vk;
		if (vk == VK_LBUTTON || vk == VK_RBUTTON || vk == VK_MBUTTON)
		{
			GetCursorPos(&ptCur);
		}
	}
	if (ptCur.x == -32000)
		ptCur = gpConEmu->mp_Menu->CalcTabMenuPos(pRCon->VCon());

	LogString(L"EditMenu called by hotkey");
	gpConEmu->mp_Menu->ShowEditMenu(pRCon->VCon(), ptCur);
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_ShowRealConsole(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	if (TestOnly)
		return true;
	// Должно обрабатываться через WM_HOTKEY?
	pRCon->CtrlWinAltSpace();
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_ForcedFullScreen(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	// Должно обрабатываться через WM_HOTKEY
	gpConEmu->DoForcedFullScreen();
	return true;
}

bool CConEmuCtrl::key_ChildSystemMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	// Должно обрабатываться через WM_HOTKEY
	CVConGuard VCon;
	if (gpConEmu->GetActiveVCon(&VCon) >= 0)
	{
		VCon->RCon()->ChildSystemMenu();
	}
	return true;
}

bool CConEmuCtrl::key_InsideSetFocusParent(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (gpConEmu->mp_Inside == nullptr)
		return false;
	if (TestOnly)
		return true;

	gpConEmu->OnSwitchGuiFocus(SwitchGuiFocusOp::FocusParent);
	return true;
}

// Общая для key_BufferScrollUp/key_BufferScrollDown/key_BufferScrollPgUp/key_BufferScrollPgDn
// pRCon may be nullptr
bool CConEmuCtrl::key_BufferScroll(bool TestOnly, BYTE vk, CRealConsole* pRCon)
{
	if (!pRCon
		|| !pRCon->isBufferHeight()
		|| ((pRCon->GetActiveBufferType() == rbt_Primary) && pRCon->isFarBufferSupported()))
	{
		return false;
	}

	if (TestOnly)
		return true;

	_ASSERTE(pRCon!=nullptr);
	switch (vk)
	{
		case VK_DOWN:
			pRCon->DoScroll(SB_LINEDOWN);
			break;
		case VK_UP:
			pRCon->DoScroll(SB_LINEUP);
			break;
		case VK_NEXT:
			pRCon->DoScroll(SB_PAGEDOWN);
			break;
		case VK_PRIOR:
			pRCon->DoScroll(SB_PAGEUP);
			break;
	}
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_BufferScrollUp(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	return key_BufferScroll(TestOnly, VK_UP, pRCon);
}

// pRCon may be nullptr
bool CConEmuCtrl::key_BufferScrollDown(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	return key_BufferScroll(TestOnly, VK_DOWN, pRCon);
}

// pRCon may be nullptr
bool CConEmuCtrl::key_BufferScrollPgUp(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	return key_BufferScroll(TestOnly, VK_PRIOR, pRCon);
}

// pRCon may be nullptr
bool CConEmuCtrl::key_BufferScrollPgDn(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	return key_BufferScroll(TestOnly, VK_NEXT, pRCon);
}

// pRCon may be nullptr
bool CConEmuCtrl::key_CtrlTab(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!gpSet->isTabSelf)
		return false;

	if (TestOnly)
		return true;

	//WARNING! Must be System Unchanged HOTKEY
	//WARNING! Функция зовет isPressed(VK_SHIFT)
	WARNING("CConEmuCtrl:: Переделать на явный вызов SwitchNext, учесть, что он может быть альтернативным!");
	gpConEmu->mp_TabBar->OnKeyboard(WM_KEYDOWN, VK_TAB, 0);
	if (gpSet->isTabLazy || gpSet->isTabRecent)
		gpConEmu->mb_InCtrlTabSwitch = TRUE;
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_CtrlShiftTab(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!gpSet->isTabSelf)
		return false;

	if (TestOnly)
		return true;

	//WARNING! Must be System Unchanged HOTKEY
	//WARNING! Функция зовет isPressed(VK_SHIFT)
	WARNING("CConEmuCtrl:: Переделать на явный вызов SwitchPrev, учесть, что он может быть альтернативным!");
	gpConEmu->mp_TabBar->OnKeyboard(WM_KEYDOWN, VK_TAB, 0);
	if (gpSet->isTabLazy || gpSet->isTabRecent)
		gpConEmu->mb_InCtrlTabSwitch = TRUE;
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_CtrlTab_Prev(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!gpSet->isTabSelf || !gpConEmu->mp_TabBar->IsInSwitch())
		return false;

	if (TestOnly)
		return true;

	//WARNING! Must be System Unchanged HOTKEY
	//WARNING! Функция зовет isPressed(VK_SHIFT)
	WARNING("CConEmuCtrl:: Переделать на явный вызов Switch, учесть, что он может быть альтернативным!");
	if (!gpConEmu->mp_TabBar->OnKeyboard(WM_KEYDOWN, VkState.Vk, 0))
		return false;
	return true;
}

// pRCon may be nullptr
bool CConEmuCtrl::key_CtrlTab_Next(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!gpSet->isTabSelf || !gpConEmu->mp_TabBar->IsInSwitch())
		return false;

	if (TestOnly)
		return true;

	//WARNING! Must be System Unchanged HOTKEY
	//WARNING! Функция зовет isPressed(VK_SHIFT)
	WARNING("CConEmuCtrl:: Переделать на явный вызов Switch, учесть, что он может быть альтернативным!");
	if (!gpConEmu->mp_TabBar->OnKeyboard(WM_KEYDOWN, VkState.Vk, 0))
		return false;
	return true;
}

// Общая для key_WinWidthDec/key_WinWidthInc/key_WinHeightDec/key_WinHeightInc
// pRCon may be nullptr
bool CConEmuCtrl::key_WinSize(BYTE vk)
{
	if (gpConEmu->isFullScreen() || gpConEmu->isZoomed() || gpConEmu->isIconic())
	{
		// ничего не делать
	}
	else
	{
		CVConGuard VCon;
		CVirtualConsole* pVCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon.VCon() : nullptr;
		RECT rcWindow = {};
		if (GetWindowRect(ghWnd, &rcWindow))
		{
			RECT rcMon = gpConEmu->CalcRect(CER_MONITOR, rcWindow, CER_MONITOR, pVCon);
			int nX = gpFontMgr->FontWidth();
			int nY = gpFontMgr->FontHeight();
			if (vk == VK_LEFT)
			{
				rcWindow.right = rcWindow.right - nX;
			}
			else if (vk == VK_RIGHT)
			{
				if ((rcWindow.right + nX) < rcMon.right)
					rcWindow.right = rcWindow.right + nX;
			}
			else if (vk == VK_UP)
			{
				rcWindow.bottom = rcWindow.bottom - nY;
			}
			else if (vk == VK_DOWN)
			{
				if ((rcWindow.bottom + nY) < rcMon.bottom)
					rcWindow.bottom = rcWindow.bottom + nY;
			}

			if (rcWindow.right > rcWindow.left && rcWindow.bottom > rcWindow.top)
			{
				MoveWindowRect(ghWnd, rcWindow, TRUE);
			}
		}
		//
		//CRealConsole* pRCon = pVCon ? pVCon->RCon() : nullptr;
		//if (pRCon)
		//{
		//
		//	//if (!pRCon->GuiWnd())
		//	//{
		//	//
		//	//}
		//	//else
		//	//{
		//	//	// Ресайз в ГУИ режиме
		//	//}
		//}
	}
	return true;
}

// Все что ниже - было привязано к "HostKey"
// pRCon may be nullptr
bool CConEmuCtrl::key_WinWidthDec(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	return key_WinSize(VK_LEFT);
}

// pRCon may be nullptr
bool CConEmuCtrl::key_WinWidthInc(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	return key_WinSize(VK_RIGHT);
}

// pRCon may be nullptr
bool CConEmuCtrl::key_WinHeightDec(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	return key_WinSize(VK_UP);
}

// pRCon may be nullptr
bool CConEmuCtrl::key_WinHeightInc(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	return key_WinSize(VK_DOWN);
}

// pRCon ignored
bool CConEmuCtrl::key_WinDragStart(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	// Dummy
	return true;
}


void CConEmuCtrl::ResetDoubleKeyConsoleNum(CRealConsole* pRCon)
{
	if (mn_DoubleKeyConsoleNum)
		mn_DoubleKeyConsoleNum = 0;
}

// Console activate by number
// pRCon may be nullptr
bool CConEmuCtrl::key_ConsoleNum(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	int nNewIdx = std::numeric_limits<int>::min();

	// If there are more than 9 consoles, use two-digit activation
	if (CVConGroup::isVConExists(10))
	{
		if (gpConEmu->mn_DoubleKeyConsoleNum)
		{
			if ((VkState.Vk >= '0') && (VkState.Vk <= '9'))
			{
				nNewIdx = ((VkState.Vk - '0') + ((gpConEmu->mn_DoubleKeyConsoleNum - '0')*10)) - 1; // 0-based
			}

			gpConEmu->ResetDoubleKeyConsoleNum(pRCon);
		}
		else
		{
			// Activate Last console at once
			if (VkState.Vk == '0')
			{
				nNewIdx = -1;
				_ASSERTE(gpConEmu->mn_DoubleKeyConsoleNum == 0);
			}
			// Store first digit for 2-digit 0-based console number
			else if ((VkState.Vk > '0') && (VkState.Vk <='9' ))
			{
				gpConEmu->mn_DoubleKeyConsoleNum = VkState.Vk;
			}
			else
			{
				_ASSERTE((VkState.Vk >= '0') && (VkState.Vk <='9' ));
			}
		}
	}
	else
	{
		// Let get 0-based index
		if ((VkState.Vk >= '1') && (VkState.Vk <= '9'))
		{
			nNewIdx = VkState.Vk - '1';
		}
		// Activate Last console at once
		else if (VkState.Vk == '0')
		{
			nNewIdx = -1;
		}

		gpConEmu->ResetDoubleKeyConsoleNum(pRCon);
	}

	if (nNewIdx >= -1)
		gpConEmu->ConActivate(nNewIdx);

	return true;
}

#if 0
bool CConEmuCtrl::key_PicViewSlideshow(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon)
		return false;
	//if (TestOnly) -- ниже, доп.проверки
	//	return true;

	if (VkState.Vk == VK_PAUSE)
	{
		// SlideShow в PicView2 подзадержался
		if (gpConEmu->isPictureView() /*&& !IsWindowUnicode(hPictureView)*/)
		{
			bool lbAllowed = true;
			if (IsWindowUnicode(gpConEmu->hPictureView))
			{
				// На будущее, если будет "встроенный" SlideShow - вернуть на это сообщение TRUE
				UINT nMsg = RegisterWindowMessage(L"PicView:HasSlideShow");
				DWORD_PTR nRc = 0;
				LRESULT lRc = SendMessageTimeout(gpConEmu->hPictureView, nMsg, 0,0, SMTO_NORMAL, 1000, &nRc);
				if (!lRc || nRc == TRUE)
					lbAllowed = false;
			}

			if (!lbAllowed)
				return false;
			if (TestOnly)
				return true;

			gpConEmu->bPicViewSlideShow = !gpConEmu->bPicViewSlideShow;

			if (gpConEmu->bPicViewSlideShow)
			{
				if (gpSet->nSlideShowElapse<=500) gpSet->nSlideShowElapse=500;

				gpConEmu->dwLastSlideShowTick = GetTickCount() - gpSet->nSlideShowElapse;
			}

			return true;
		}
	}
	else if (gpConEmu->bPicViewSlideShow)
	{
		//KillTimer(hWnd, 3);
		if ((VkState.Vk == 0xbd/* -_ */) || (VkState.Vk == 0xbb/* =+ */))
		{
			if (TestOnly)
				return true;

			if (VkState.Vk == 0xbb)
			{
				gpSet->nSlideShowElapse = 1.2 * gpSet->nSlideShowElapse;
			}
			else
			{
				gpSet->nSlideShowElapse = gpSet->nSlideShowElapse / 1.2;

				if (gpSet->nSlideShowElapse<=500) gpSet->nSlideShowElapse=500;
			}

			return true;
		}
		//else
		//{
		//	//bPicViewSlideShow = false; // отмена слайдшоу
		//	return false;
		//}
	}

	return false;
}
#endif

bool CConEmuCtrl::key_GuiMacro(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;
	// Выполнить макрос
	if (hk->GuiMacro && *hk->GuiMacro)
	{
		// Т.к. ExecuteMacro издевается над строкой
		CEStr pszCopy(hk->GuiMacro);
		if (!pszCopy)
		{
			_ASSERTE(pszCopy);  // -V571
		}
		else
		{
			CEStr pszResult = ConEmuMacro::ExecuteMacro(std::move(pszCopy), pRCon);
			TODO("Когда появится StatusLine - хорошо бы в ней результат показать");
		}
	}
	return true;
}

void CConEmuCtrl::ChooseTabFromMenu(BOOL abFirstTabOnly, POINT pt, DWORD Align /*= TPM_CENTERALIGN|TPM_VCENTERALIGN*/)
{
	HMENU hPopup = gpConEmu->mp_Menu->CreateVConListPopupMenu(nullptr, abFirstTabOnly);

	if (!Align)
		Align = TPM_LEFTALIGN|TPM_TOPALIGN;

	const int nTab = gpConEmu->mp_Menu->trackPopupMenu(tmp_TabsList, hPopup, Align|TPM_RETURNCMD,
		pt.x, pt.y, ghWnd);

	if (nTab >= IDM_VCON_FIRST && nTab <= IDM_VCON_LAST)
	{
		const int nNewV = ((int)HIWORD(nTab)) - 1;
		const int nNewR = ((int)LOWORD(nTab)) - 1;

		CVConGuard VCon;
		if (CVConGroup::GetVCon(nNewV, &VCon))
		{
			CRealConsole* pRCon = VCon->RCon();
			if (pRCon)
			{
				CTab tab(__FILE__,__LINE__);
				if (pRCon->GetTab(nNewR, tab))
					pRCon->ActivateFarWindow(tab->Info.nFarWindowID);
			}
			if (!VCon->isActive(false))
				gpConEmu->Activate(VCon.VCon());
		}
	}

	DestroyMenu(hPopup);
}

// Все параметры могут быть nullptr - вызов из GuiMacro
bool CConEmuCtrl::key_ShowTabsList(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	// 120820 - не будем требовать наличия плагина для F12 в Far
	if (pRCon && pRCon->GetFarPID(false))
	{
		// Для Far Manager - не обрабатывать.
		// Если юзеру нужно ConEmu-шное меню для списка табов - есть vkShowTabsList2 и GuiMacro "Tabs(8)"
		return false;
	}
	else
	{
		if (TestOnly)
			return true;

		RECT rcWnd = {};
		if (pRCon)
		{
			GetWindowRect(pRCon->GetView(), &rcWnd);
		}
		else
		{
			GetClientRect(ghWnd, &rcWnd);
			MapWindowPoints(ghWnd, nullptr, (LPPOINT)&rcWnd, 2);
		}

		POINT pt = {(rcWnd.left+rcWnd.right)/2, (rcWnd.top+rcWnd.bottom)/2};

		ChooseTabFromMenu(FALSE, pt, TPM_CENTERALIGN|TPM_VCENTERALIGN);
	}

	return true;
}

bool CConEmuCtrl::key_RenameTab(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	if (pRCon)
	{
		pRCon->DoRenameTab();
	}

	return true;
}

bool CConEmuCtrl::key_MoveTabLeft(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	gpConEmu->MoveActiveTab((pRCon ? pRCon->VCon() : nullptr), true);

	return true;
}

bool CConEmuCtrl::key_MoveTabRight(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	gpConEmu->MoveActiveTab((pRCon ? pRCon->VCon() : nullptr), false);

	return true;
}

bool CConEmuCtrl::key_ShowStatusBar(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	gpConEmu->StatusCommand(csc_ShowHide);

	return true;
}

bool CConEmuCtrl::key_ShowTabBar(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	gpConEmu->TabCommand(ctc_ShowHide);

	return true;
}

bool CConEmuCtrl::key_ShowCaption(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	gpSet->SwitchHideCaptionAlways();
	gpConEmu->RefreshWindowStyles();

	if (ghOpWnd)
	{
		if (gpSetCls->GetPage(thi_Appear))
			CheckDlgButton(gpSetCls->GetPage(thi_Appear), cbHideCaptionAlways, gpSet->isHideCaptionAlways());
		apiSetForegroundWindow(ghOpWnd);
	}

	return true;
}

bool CConEmuCtrl::key_AlwaysOnTop(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	gpSet->isAlwaysOnTop = !gpSet->isAlwaysOnTop;
	gpConEmu->DoAlwaysOnTopSwitch();

	return true;
}

bool CConEmuCtrl::key_ResetTerminal(const ConEmuChord& /*VkState*/, bool TestOnly, const ConEmuHotKey* /*hk*/, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	if (gpSet->isResetTerminalConfirm)
	{
		const auto confirmRc = ConfirmDialog(
			CLngRc::getRsrc(lng_ResetTerminalWarning) /*
			L"Warning!\nThis operation could harm further output of applications.\n"
			L"It's better to execute a dedicated command as `clear` or `cls`."*/,
			CLngRc::getRsrc(lng_ResetTerminalConfirm/*"Do you want to reset terminal contents?"*/),
			CLngRc::getRsrc(lng_ResetTerminalTitle/*"Reset terminal confirmation"*/),
			CECLEARSCREEN, MB_OKCANCEL, ghWnd,
			nullptr, CLngRc::getRsrc(lng_ResetTerminalOk/*"Reset terminal contents and properties"*/),
			nullptr, CLngRc::getRsrc(lng_ResetTerminalCancel/*"Cancel operation"*/));
		if (confirmRc != IDOK)
			return false;
	}

	CEStr szMacro(L"Write(\"\\ec\")");
	const CEStr szResult = ConEmuMacro::ExecuteMacro(std::move(szMacro), pRCon);
	LogString(CEStr(L"Reset terminal macro result: ", szResult.c_str()));

	return true;
}

bool CConEmuCtrl::key_PasteText(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	// Если это GUI App in Tab - пусть само
	if (!pRCon || pRCon->GuiWnd())
		return false;
	// Let Far Manager process Ctrl+V himself too
	if (pRCon->isFar())
		return false;

	const AppSettings* pApp = gpSet->GetAppSettings(pRCon->GetActiveAppSettingsId());
	PasteLinesMode mode = pApp->PasteAllLines();
	if (!mode)
		return false;

	if (TestOnly)
		return true;

	CEPasteMode pasteMode;
	switch (mode)
	{
	case plm_Default:
	case plm_MultiLine:
		pasteMode = pm_Standard; break;
	case plm_SingleLine:
		pasteMode = pm_OneLine; break;
	case plm_FirstLine:
		pasteMode = pm_FirstLine; break;
	default:
		_ASSERTE(FALSE && "Unsupported PasteLinesMode");
		pasteMode = pm_Standard;
	}
	pRCon->Paste(pasteMode, nullptr, false/*default*/, pApp->PosixAllLines());
	return true;
}

bool CConEmuCtrl::key_PasteFirstLine(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	// Если это GUI App in Tab - пусть само
	if (!pRCon || pRCon->GuiWnd())
		return false;
	// Let Far Manager process Ctrl+V himself too
	if (pRCon->isFar())
		return false;

	const AppSettings* pApp = gpSet->GetAppSettings(pRCon->GetActiveAppSettingsId());
	PasteLinesMode mode = pApp->PasteFirstLine();
	if (!mode)
		return false;

	if (TestOnly)
		return true;

	CEPasteMode pasteMode;
	switch (mode)
	{
	case plm_MultiLine:
		pasteMode = pm_Standard; break;
	case plm_Default:
	case plm_SingleLine:
		pasteMode = pm_OneLine; break;
	case plm_FirstLine:
		pasteMode = pm_FirstLine; break;
	default:
		_ASSERTE(FALSE && "Unsupported PasteLinesMode");
		pasteMode = pm_Standard;
	}
	pRCon->Paste(pasteMode, nullptr, false/*default*/, pApp->PosixFirstLine());
	return true;
}


/* ************* Service functions ************* */

void CConEmuCtrl::StatusCommand(ConEmuStatusCommand nStatusCmd, int IntParm, LPCWSTR StrParm, CRealConsole* pRCon)
{
	switch (nStatusCmd)
	{
	case csc_ShowHide:
		{
			// Retrieve IdealRect BEFORE changing of StatusBar visibility
			RECT rcIdeal = gpConEmu->GetIdealRect();

			if (IntParm == 1)
				gpSet->isStatusBarShow = true;
			else if (IntParm == 2)
				gpSet->isStatusBarShow = false;
			else
				gpSet->isStatusBarShow = !gpSet->isStatusBarShow;

			gpConEmu->RequestRecalc();

			if (!gpConEmu->isZoomed() && !gpConEmu->isFullScreen())
			{
				CVConGroup::SyncConsoleToWindow(&rcIdeal, true);
			}

			gpConEmu->ReSize(true);
			gpConEmu->InvalidateGaps();
		}
		break;

	case csc_SetStatusText:
		if (pRCon)
		{
			pRCon->SetConStatus(StrParm ? StrParm : L"");
		}
		break;
	}
}

void CConEmuCtrl::TabCommand(ConEmuTabCommand nTabCmd)
{
	if ((int)nTabCmd < ctc_ShowHide || (int)nTabCmd > ctc_SwitchPrev)
	{
		_ASSERTE((int)nTabCmd >= ctc_ShowHide && (int)nTabCmd <= ctc_SwitchPrev);
		return;
	}

	if (!isMainThread())
	{
		PostMessage(ghWnd, gpConEmu->mn_MsgTabCommand, nTabCmd, 0);
		return;
	}

	switch (nTabCmd)
	{
		case ctc_ShowHide:
		{
			if (gpConEmu->isTabsShown())
				gpSet->isTabs = 0;
			else
				gpSet->isTabs = 1;

			gpConEmu->ForceShowTabs(gpSet->isTabs == 1);
		} break;
		case ctc_SwitchNext:
		{
			gpConEmu->mp_TabBar->SwitchNext();
		} break;
		case ctc_SwitchPrev:
		{
			gpConEmu->mp_TabBar->SwitchPrev();
		} break;
		case ctc_SwitchCommit:
		{
			gpConEmu->mp_TabBar->SwitchCommit();
		} break;

		default:
		{
			_ASSERTE(FALSE && "Not handled command");
		}
	};
}

size_t CConEmuCtrl::GetOpenedTabs(CESERVER_REQ_GETALLTABS::TabInfo*& pTabs)
{
	_ASSERTE(pTabs==nullptr);
	int nConCount = gpConEmu->GetConCount();
	int nActiveCon = gpConEmu->ActiveConNum();
	size_t cchMax = nConCount*16;
	size_t cchCount = 0;
	CVConGuard VCon;

	pTabs = (CESERVER_REQ_GETALLTABS::TabInfo*)calloc(cchMax, sizeof(*pTabs));

	for (int V = 0; CVConGroup::GetVCon(V, &VCon, true); V++)
	{
		if (!pTabs)
		{
			_ASSERTE(pTabs!=nullptr);
			break;
		}

		CRealConsole* pRCon = VCon->RCon();
		if (!pRCon)
			continue;

		CTab tab(__FILE__,__LINE__);
		wchar_t szMark[6];
		for (int t = 0; pRCon->GetTab(t, tab); t++)
		{
			int T = t; //tab->Info.nFarWindowID; -- В Far3 номер из nFarWindowID стал бесполезным для пользователя

			if (cchCount >= cchMax)
			{
				pTabs = (CESERVER_REQ_GETALLTABS::TabInfo*)realloc(pTabs, (cchMax+32)*sizeof(*pTabs));
				if (!pTabs)
				{
					_ASSERTE(pTabs!=nullptr);
					break;
				}
				cchMax += 32;
				_ASSERTE(cchCount<cchMax);
			}

			pTabs[cchCount].ActiveConsole = (V == nActiveCon);
			pTabs[cchCount].ActiveTab = ((tab->Flags() & fwt_CurrentFarWnd) == fwt_CurrentFarWnd);
			pTabs[cchCount].Disabled = ((tab->Flags() & fwt_Disabled) == fwt_Disabled);
			pTabs[cchCount].ConsoleIdx = V;
			pTabs[cchCount].TabNo = T;
			pTabs[cchCount].FarWindow = tab->Info.nFarWindowID;

			// Text
			//wcscpy_c(szMark, tab.Modified ? L" * " : L"   ");
			switch (tab->Type())
			{
				case fwt_Editor:
					wcscpy_c(szMark, (tab->Flags() & fwt_ModifiedFarWnd) ? L" * " : L" E "); break;
				case fwt_Viewer:
					wcscpy_c(szMark, L" V "); break;
				default:
					wcscpy_c(szMark, L"   ");
			}

			if ((V == nActiveCon) && (T <= 9))
				swprintf_c(pTabs[cchCount].Title, L"[%i/&%i]%s", V+1, T, szMark);
			else
				swprintf_c(pTabs[cchCount].Title, L"[%i/%i]%s", V+1, T, szMark);

			#ifdef _DEBUG
			if (pRCon->IsFarLua())
				swprintf_c(pTabs[cchCount].Title+lstrlen(pTabs[cchCount].Title), 30/*#SECURELEN*/, L"{%i} ", tab->Info.nFarWindowID);
			#endif

			int nCurLen = lstrlen(pTabs[cchCount].Title);
			lstrcpyn(pTabs[cchCount].Title+nCurLen, pRCon->GetTabTitle(tab), countof(pTabs[cchCount].Title)-nCurLen);

			cchCount++;
		}
	}
	return cchCount;
}

size_t CConEmuCtrl::GetOpenedPanels(wchar_t*& pszDirs, int& iCount, int& iCurrent)
{
	CEStr szActiveDir, szPassive;
	CVConGuard VCon;
	MArray<wchar_t*> Dirs;
	size_t cchAllLen = 1;
	iCount = iCurrent = 0;

	for (int V = 0; CVConGroup::GetVCon(V, &VCon, true); V++)
	{
		VCon->RCon()->GetPanelDirs(szActiveDir, szPassive);
		if (VCon->isActive(false))
			iCurrent = iCount;
		LPCWSTR psz[] = {szActiveDir.ms_Val, szPassive.ms_Val};
		for (int i = 0; i <= 1; i++)
		{
			if (psz[i] && psz[i][0])
			{
				const int iLen = lstrlen(psz[i]);
				cchAllLen += (iLen + 1);
				Dirs.push_back(lstrdup(psz[i]).Detach());
				iCount++;
			}
		}
	}

	_ASSERTE(pszDirs == nullptr);
	pszDirs = (wchar_t*)malloc(cchAllLen*sizeof(*pszDirs));
	if (!pszDirs)
		return 0;

	wchar_t* psz = pszDirs;
	for (int i = 0; i < Dirs.size(); i++)
	{
		wchar_t* p = Dirs[i];
		_wcscpy_c(psz, cchAllLen, p);
		psz += lstrlen(psz)+1;
		free(p);
	}

	return cchAllLen;
}

bool CConEmuCtrl::key_DeleteWordToLeft(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (!pRCon || pRCon->GuiWnd())
		return false;

	if (TestOnly)
		return pRCon->DeleteWordKeyPress(true);

	pRCon->DeleteWordKeyPress(false);

	return true;
}

bool CConEmuCtrl::key_RunTask(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	_ASSERTE(hk->HkType==chk_Task);

	if (gpSet->CmdTaskGet(hk->GetTaskIndex()))
	{
		wchar_t szMacro[64];
		swprintf_c(szMacro, L"Task(%i)", hk->GetTaskIndex()+1); //1-based
		CEStr pszResult = ConEmuMacro::ExecuteMacro(szMacro, pRCon);
	}
	return true;
}

bool CConEmuCtrl::key_FindTextDlg(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	// Если это GUI App in Tab - само
	if (!pRCon || (pRCon->GuiWnd() && !pRCon->isBufferHeight()))
		return false;
	if (TestOnly)
		return true;

	gpConEmu->mp_Find->FindTextDialog();
	return true;
}

void CConEmuCtrl::DoFindText(int nDirection, CRealConsole* pRCon /*= nullptr*/)
{
	CVConGuard VCon;
	if (!pRCon)
	{
		pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : nullptr;
		if (!pRCon)
			return;
	}

	pRCon->DoFindText(nDirection);
}

void CConEmuCtrl::DoEndFindText(CRealConsole* pRCon /*= nullptr*/)
{
	CVConGuard VCon;
	if (!pRCon)
	{
		pRCon = (CVConGroup::GetActiveVCon(&VCon) >= 0) ? VCon->RCon() : nullptr;
		if (!pRCon)
			return;
	}

	pRCon->DoEndFindText();
}

void CConEmuCtrl::SkipOneAppsRelease(bool abSkip)
{
	if (abSkip)
	{
		if (isPressed(VK_APPS))
		{
			// Игнорировать одно следующее VK_APPS
			mb_SkipOneAppsRelease = true;
			if (!mh_SkipOneAppsRelease)
				mh_SkipOneAppsRelease = SetWindowsHookEx(WH_GETMESSAGE, SkipOneAppsReleaseHook, nullptr, GetCurrentThreadId());
			DEBUGSTRAPPS(mh_SkipOneAppsRelease ? L"CConEmuCtrl::SkipOneAppsRelease set was succeeded\n" : L"CConEmuCtrl::SkipOneAppsRelease FAILED\n");
		}
		else
		{
			abSkip = false;
			DEBUGSTRAPPS(L"CConEmuCtrl::SkipOneAppsRelease ignored, VK_APPS not pressed\n");
		}
	}

	if (!abSkip)
	{
		if (mh_SkipOneAppsRelease)
		{
			UnhookWindowsHookEx(mh_SkipOneAppsRelease);
			mh_SkipOneAppsRelease = nullptr;
			mb_SkipOneAppsRelease = false;
			DEBUGSTRAPPS(L"CConEmuCtrl::SkipOneAppsRelease was unhooked\n");
		}
	}
}

LRESULT CConEmuCtrl::SkipOneAppsReleaseHook(int code, WPARAM wParam, LPARAM lParam)
{
	if (code >= 0)
	{
		if (mb_SkipOneAppsRelease && lParam)
		{
			LPMSG pMsg = (LPMSG)lParam;

			if (pMsg->message == WM_CONTEXTMENU)
			{
				DEBUGSTRAPPS(L"SkipOneAppsReleaseHook: WM_CONTEXTMENU was received and blocked\n");
				pMsg->message = WM_NULL;
				mb_SkipOneAppsRelease = false;
				return FALSE; // Skip one Apps
			}
			#ifdef _DEBUG
			else if (pMsg->message == WM_KEYUP)
			{
				if (pMsg->wParam == VK_APPS)
				{
					DEBUGSTRAPPS(L"SkipOneAppsReleaseHook: WM_KEYUP(VK_APPS) received\n");
				}
			}
			#endif
		}
	}

	return CallNextHookEx(mh_SkipOneAppsRelease, code, wParam, lParam);
}

bool CConEmuCtrl::key_Screenshot(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	MakeScreenshot();
	return true;
}

bool CConEmuCtrl::key_ScreenshotFull(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon)
{
	if (TestOnly)
		return true;

	MakeScreenshot(true);
	return true;
}

void CConEmuCtrl::MakeScreenshot(bool abFullscreen /*= false*/)
{
	BOOL lbRc = FALSE;
	HDC hScreen = nullptr;
	RECT rcWnd = {};
	HWND hWnd = GetForegroundWindow();

	if (!hWnd)
	{
		DisplayLastError(L"GetForegroundWindow() == nullptr");
		return;
	}

	HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	MONITORINFO mi = {sizeof(mi)};
	if (!GetMonitorInfo(hMon, &mi))
	{
		DisplayLastError(L"GetMonitorInfo() failed");
		return;
	}

	if (!abFullscreen)
	{
		GetWindowRect(hWnd, &rcWnd);
		bool bDlg = ((GetWindowLongPtr(hWnd, GWL_STYLE) & DS_MODALFRAME) == DS_MODALFRAME);
		if (gpConEmu->IsGlass())
		{
			if (gnOsVer >= 0x602)
			{
				if (bDlg)
				{
					rcWnd.top -= 5;
					rcWnd.left -= 5;
					rcWnd.right += 3;
					rcWnd.bottom += 5;
				}
				else
				{
					rcWnd.left += 2;
					rcWnd.right -= 2;
					rcWnd.bottom -= 2;
				}
			}
			else if (bDlg && (gnOsVer == 0x601))
			{
				rcWnd.top -= 5;
				rcWnd.left -= 5;
				rcWnd.right += 3;
				rcWnd.bottom += 5;
			}
		}

		// Отрезать края при Zoomed/Fullscreen
		RECT rcVisible = {};
		if (!IntersectRect(&rcVisible, &rcWnd, gpConEmu->isFullScreen() ? &mi.rcMonitor : &mi.rcWork))
		{
			DisplayLastError(L"Window out of monitor rectangle");
			return;
		}
		rcWnd = rcVisible;
	}
	else
	{
		rcWnd = mi.rcMonitor;
	}

	hScreen = GetDC(nullptr);

	lbRc = DumpImage(hScreen, nullptr, rcWnd.left, rcWnd.top, rcWnd.right-rcWnd.left, rcWnd.bottom-rcWnd.top, nullptr, FALSE/*NoTransparency, 24bit*/);

	if (!lbRc)
	{
		DisplayLastError(L"Creating screenshot failed!");
	}

	ReleaseDC(nullptr, hScreen);
}
