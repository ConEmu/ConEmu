
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

#pragma once

struct ConEmuHotKey;
struct ConEmuChord;

// Некоторые комбинации нужно обрабатывать "на отпускание" во избежание глюков с интерфейсом
extern const ConEmuHotKey* ConEmuSkipHotKey; // = ((ConEmuHotKey*)INVALID_HANDLE_VALUE)

// Текущая обрабатываемая клавиша
extern const ConEmuHotKey* gpCurrentHotKey;

class CConEmuCtrl
{
public:
	CConEmuCtrl();
	virtual ~CConEmuCtrl();

	bool ProcessHotKeyMsg(UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars, CRealConsole* pRCon);
	const ConEmuHotKey* ProcessHotKey(const ConEmuChord& VkState, bool bKeyDown, const wchar_t *pszChars, CRealConsole* pRCon);
	ConEmuChord ChordFromVk(DWORD Vk);

	void UpdateControlKeyState();
	DWORD GetControlKeyState(LPARAM lParam);

	void FixSingleModifier(DWORD Vk, CRealConsole* pRCon);

	static void StatusCommand(ConEmuStatusCommand nStatusCmd, int IntParm = 0, LPCWSTR StrParm = nullptr, CRealConsole* pRCon = nullptr); // csc_ShowHide, csc_SetStatusText

	static void TabCommand(ConEmuTabCommand nTabCmd); // ctc_ShowHide, ctc_SwitchNext, и т.п.
	static size_t GetOpenedTabs(CESERVER_REQ_GETALLTABS::TabInfo*& pTabs);
	static size_t GetOpenedPanels(wchar_t*& pszDirs, int& iCount, int& iCurrent);

	static void ChooseTabFromMenu(BOOL abFirstTabOnly, POINT pt, DWORD Align /*= TPM_CENTERALIGN|TPM_VCENTERALIGN*/);

	static void DoFindText(int nDirection, CRealConsole* pRCon = nullptr);
	static void DoEndFindText(CRealConsole* pRCon = nullptr);

	void SkipOneAppsRelease(bool abSkip);

	static void MakeScreenshot(bool abFullscreen = false);

protected:
	BOOL mb_InWinTabSwitch = FALSE;
	BOOL mb_InCtrlTabSwitch = FALSE;

	UINT mn_DoubleKeyConsoleNum = 0; // Previous VK
	void ResetDoubleKeyConsoleNum(CRealConsole* pRCon = nullptr);

private:
	DWORD dwControlKeyState = 0;
	bool bWin = false, bApps = false;
	bool bCaps = false, bNum = false, bScroll = false;
	bool bLAlt = false, bRAlt = false;
	bool bLCtrl = false, bRCtrl = false;
	bool bLShift = false, bRShift = false;
	DWORD mn_LastSingleModifier = 0, mn_SingleModifierFixState = 0;
	BYTE mn_SingleModifierFixKey = 0;
	BOOL mb_LastSingleModifier = FALSE;

	UINT m_SkippedMsg = 0; WPARAM m_SkippedMsgWParam = 0; LPARAM m_SkippedMsgLParam = 0;

	static bool mb_SkipOneAppsRelease;
	static HHOOK mh_SkipOneAppsRelease;
	static LRESULT CALLBACK SkipOneAppsReleaseHook(int code, WPARAM wParam, LPARAM lParam);

public:
	// true-processed, false-pass to console

	// User (Keys)
	static bool WINAPI key_MinimizeRestore(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MinimizeByEsc(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_GlobalRestore(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MultiNewPopupMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MultiNewPopupMenu2(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MultiNewAttach(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MultiNext(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MultiPrev(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MultiRecreate(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_AlternativeBuffer(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MultiBuffer(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
    static bool WINAPI key_SwitchTermMode(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_DuplicateRoot(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_DuplicateRootAs(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MultiCmd(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_CTSVkBlockStart(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_CTSVkTextStart(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_Screenshot(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_ScreenshotFull(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_ShowStatusBar(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_ShowTabBar(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_ShowCaption(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_AlwaysOnTop(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_ResetTerminal(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	// System (predefined, fixed)
	static bool WINAPI key_SystemMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_TabMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_EditMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_ShowRealConsole(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_ForcedFullScreen(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_ChildSystemMenu(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_InsideSetFocusParent(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_BufferScrollUp(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_BufferScrollDown(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_BufferScrollPgUp(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_BufferScrollPgDn(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_BufferScroll(bool TestOnly, BYTE vk, CRealConsole* pRCon);
	static bool WINAPI key_CtrlTab(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_CtrlShiftTab(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_CtrlTab_Prev(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_CtrlTab_Next(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	#if 0
	// PicView2 slideshow
	static bool WINAPI key_PicViewSlideshow(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	#endif
	// GuiMacro
	static bool WINAPI key_GuiMacro(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	// Tabs list
	static bool WINAPI key_ShowTabsList(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_RenameTab(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MoveTabLeft(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_MoveTabRight(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	// Clipboard
	static bool WINAPI key_PasteText(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_PasteFirstLine(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	// Text editing
	static bool WINAPI key_DeleteWordToLeft(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	// Find text
	static bool WINAPI key_FindTextDlg(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	// Run task (From hotkey specified in task properties)
	static bool WINAPI key_RunTask(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);

public:
	// Все что ниже - было привязано к "HostKey"
	static bool WINAPI key_WinWidthDec(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_WinWidthInc(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_WinHeightDec(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_WinHeightInc(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	static bool WINAPI key_WinSize(BYTE vk);
	static bool WINAPI key_WinDragStart(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
	// Console activate by number
	static bool WINAPI key_ConsoleNum(const ConEmuChord& VkState, bool TestOnly, const ConEmuHotKey* hk, CRealConsole* pRCon);
};
