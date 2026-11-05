
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

#pragma once

#include "TabCtrlBase.h"

class CToolImg;
class CFindPanel;

class CTabPanelWin : public CTabPanelBase
{
private:
	HWND mh_Tabbar, mh_Toolbar, mh_Rebar;
	CFindPanel* mp_Find;
	bool mb_ChangeAllowed;
	int  mn_LastToolbarWidth;
	int  mn_TabHeight;
	bool mb_DarkMode = false;
	int mn_ToolbarIconSize = 14;

private:
	CToolImg* mp_ToolImg;

public:
	CTabPanelWin(CTabBarClass* ap_Owner);
	virtual ~CTabPanelWin();

public:
	virtual HWND ActivateSearchPaneInt(bool bActivate) override;
	virtual void AddTabInt(LPCWSTR text, int i, CEFarWindowType Flags, int iTabIcon) override;
	virtual void CreateRebar() override;
	virtual void DestroyRebar() override;
	virtual void DeleteItemInt(int I) override;
	virtual bool IsCreated() override;
	virtual bool IsSearchShownInt(bool bFilled) override;
	virtual bool IsTabbarCreated() override;
	virtual bool IsTabbarNotify(LPNMHDR nmhdr) override;
	virtual bool IsToolbarCreated() override;
	virtual bool IsToolbarCommand(WPARAM wParam, LPARAM lParam) override;
	virtual bool IsToolbarNotify(LPNMHDR nmhdr) override;
	virtual int  GetCurSelInt() override;
	virtual int  GetItemCountInt() override;
	virtual bool GetTabBarClientRect(RECT* rcTab) override;
	virtual int  GetTabFromPoint(POINT ptCur, bool bScreen = true, bool bOverTabHitTest = true) override;
	virtual bool GetTabRect(int nTabIdx, RECT* rcTab) override; // Screen-coordinates!
	virtual bool GetTabText(int nTabIdx, wchar_t* pszText, int cchTextMax) override;
	virtual bool GetToolBtnChecked(ToolbarCommandIdx iCmd) override;
	virtual bool GetToolBtnRect(int nCmd, RECT* rcBtnRect) override;
	virtual bool GetRebarClientRect(RECT* rc) override;
	virtual void HighlightTab(int iTab, bool bHighlight) override;
	virtual void InvalidateBar() override;
	virtual void OnCaptionHiddenChanged(bool bCaptionHidden) override;
	virtual void OnConsoleActivatedInt(int nConNumber) override;
	virtual bool OnNotifyInt(LPNMHDR nmhdr, LRESULT& lResult) override;
	virtual void OnWindowStateChanged(ConEmuWindowMode newMode) override;
	virtual int  QueryTabbarHeight() override;
	virtual void RePaintInt() override;
	virtual void RepositionInt() override;
	virtual int  SelectTabInt(int i) override;
	virtual void SetTabbarFont(HFONT hFont) override;
	virtual void SetToolBtnChecked(ToolbarCommandIdx iCmd, bool bChecked) override;
	virtual void ShowBar(bool bShow) override;
	virtual bool ShowSearchPane(bool bShow, bool bCtrlOnly = false) override;
	virtual void ShowTabsPane(bool bShow) override;
	virtual void ShowToolsPane(bool bShow) override;
private:
	static LRESULT CALLBACK _TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT TabProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc);
	//static WNDPROC _defaultTabProc;
	static LRESULT CALLBACK _ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ToolProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc);
	//static WNDPROC _defaultToolProc;
	static LRESULT CALLBACK _ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT ReBarProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, WNDPROC defaultProc);
	//static WNDPROC _defaultReBarProc;

private:
	HWND CreateTabbar();
	HWND CreateToolbar();
	void ApplyTheme();
	void PaintDarkTabs(HDC dc);
	void PaintDarkToolbarButton(LPNMTBCUSTOMDRAW draw);
	void UpdateToolbarPos();
	LRESULT TabHitTest(bool abForce = false, int* pnOverTabHit = nullptr);
	RECT GetRect();
};
