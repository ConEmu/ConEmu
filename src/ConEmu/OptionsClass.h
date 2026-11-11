
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


#pragma once

#include "../common/MArray.h"
#include "../common/MFileMapping.h"
#include <commctrl.h>

#include "CustomFonts.h"
#include "DpiAware.h"
#include "FontMgr.h"
#include "HotkeyList.h"
#include "Options.h"
#include "SetDlgButtons.h"
#include "SetDlgColors.h"
#include "SetDlgFonts.h"
#include "SetPgBase.h"

#define BALLOON_MSG_TIMERID 101

class CBackground;
class CBackgroundInfo;
class CDpiForDialog;
class CDynDialog;
class CImgButtons;
struct CEHelpPopup;
struct DpiValue;

enum SingleInstanceArgEnum
{
	sgl_Default  = 0,
	sgl_Enabled  = 1,
	sgl_Disabled = 2,
};

enum PerformanceCounterTypes
{
	tPerfFPS = 0,
	tPerfData,
	tPerfRender,
	tPerfBlt,
	tPerfInterval,
	tPerfKeyboard,
	tPerfLast
};

typedef DWORD EvalSizeFlags;
const EvalSizeFlags
	// May be used in combination
	esf_CanUseDpi    = 0x0010, // Take into account _dpi
	esf_CanUseZoom   = 0x0020, // Take into account mn_FontZoomValue
	esf_CanUseUnits  = 0x0040, // Make the result negative IF gpSet->FontUseUnits is ON
	// Must not be used together
	esf_Vertical     = 0x0001, // Used to get lfHeight
	esf_Horizontal   = 0x0002  // Used to get lfWidth
;

class CSettings
	: public CSetDlgButtons
	, public CSetDlgFonts
{
	public:
		CSettings();
		virtual ~CSettings();
	private:
		Settings m_Settings;
		void ReleaseHandles();
		void InitVars_Pages();
		void InitPageNames();
	private:
		friend class CSetDlgButtons;
		friend class CSetDlgColors;
		friend class CSetDlgFonts;
		friend class CFontMgr;
		friend class CSetPgFonts;
		friend class CSetPgBase;
	public:

		private:
			wchar_t ConfigPath[MAX_PATH];
			wchar_t ConfigName[MAX_CONFIG_NAME];
		public:
		LPCWSTR GetConfigPath();
		LPCWSTR GetConfigName();
		void SetConfigName(LPCWSTR asConfigName);

		// Informational only, what Storage was used during settings loading
		StorageType Type = StorageType::BASIC;

		bool IsConfigNew = false; // true, если конфигурация новая
		bool IsConfigPartial = false; // true, if config has no task or start command

		// === Аргументы из командной строки ===
		// Для отладочных целей.
		bool isResetBasicSettings;
		bool isFastSetupDisabled;
		bool isDontCascade;
		// === Запрет авто сохранения опций и при выходе ===
		bool ibDisableSaveSettingsOnExit;

	public:

		bool IsMulti();
		RecreateActionParm GetDefaultCreateAction();

		SingleInstanceArgEnum SingleInstanceArg; // по умолчанию = sgl_Default, но для Quake переключается на = sgl_Enabled
		bool IsSingleInstanceArg() const;
		SingleInstanceShowHideType SingleInstanceShowHide; // по умолчанию = sih_None
		bool ResetCmdHistory(HWND hParent = nullptr);
		void SetSaveCmdHistory(bool bSaveHistory);

		//int DefaultBufferHeight;
		bool bForceBufferHeight; int nForceBufferHeight;

		#ifdef SHOW_AUTOSCROLL
		bool AutoScroll;
		#endif

		LONG EvalSize(LONG nSize, EvalSizeFlags Flags, const DpiValue* apDpi = nullptr) const;

	public:
		char isAllowDetach;

		// Thumbnails and Tiles
		//PanelViewSetMapping ThSet;
		MFileMapping<PanelViewSetMapping> m_ThSetMap;

		// Working variables...
	private:
		#ifndef APPDISTINCTBACKGROUND
		CBackground* mp_Bg = nullptr;
		BITMAPFILEHEADER* mp_BgImgData = nullptr;
		BOOL mb_NeedBgUpdate; //, mb_WasVConBgImage;
		FILETIME ftBgModified;
		DWORD nBgModifiedTick;
		bool isBackgroundImageValid;
		bool mb_BgLastFade;
		#else
		CBackgroundInfo* mp_BgInfo = nullptr;
		#endif
		CDpiForDialog* mp_DpiAware = nullptr;
		CImgButtons* mp_ImgBtn = nullptr;
	public:
		enum ColorShowFormat { eRgbDec = 0, eRgbHex, eBgrHex } m_ColorFormat;
	public:
		void SetBgImageDarker(uint8_t newValue, bool bUpdate);
		#ifndef APPDISTINCTBACKGROUND
		bool PrepareBackground(CVirtualConsole* apVCon, HDC* phBgDc, COORD* pbgBmpSize);
		bool PollBackgroundFile(); // true, если файл изменен
		#else
		CBackgroundInfo* GetBackgroundObject() const;
		#endif
		bool LoadBackgroundFile(LPCWSTR inPath, bool abShowErrors=false);
		bool IsBackgroundEnabled(CVirtualConsole* apVCon) const;
		void NeedBackgroundUpdate();
		//CBackground* CreateBackgroundImage(const BITMAPFILEHEADER* apBkImgData);
	public:

		HWND GetActivePage();
		CSetPgBase* GetActivePageObj();
		HWND GetPage(TabHwndIndex nPage);
		CSetPgBase* GetPageObj(TabHwndIndex nPage);
		template <class T>
		bool GetPageObj(T*& pObject)
		{
			pObject = dynamic_cast<T*>(GetPageObj(T::PageType()));
			return (pObject != nullptr);
		};
		TabHwndIndex GetPageId(HWND hPage);
		LPCWSTR GetActivePageWiki(CEStr& lsWiki);
		bool ActivatePage(TabHwndIndex showPage);

	private:
		TabHwndIndex m_LastActivePageId;

	public:
		//static void CenterDialog(HWND hWnd2);
		void OnSettingsClosed();
		DWORD BalloonStyle();
		// IDD_SETTINGS
		static INT_PTR CALLBACK wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		//
		bool SetOption(LPCWSTR asName, int anValue);
		bool SetOption(LPCWSTR asName, LPCWSTR asValue);
		void SettingsLoaded(SettingsLoadedFlags slfFlags, LPCWSTR pszCmdLine = nullptr);
		void SettingsPreSave();
		static void Dialog(int IdShowPage = 0);
		void UpdateWindowMode(ConEmuWindowMode WndMode);
		void UpdatePos(int x, int y, bool bGetRect = false);
		void UpdateSize(const CESize w, const CESize h);
		void UpdateFontInfo();
		void Performance(UINT nID, BOOL bEnd);
		void PostUpdateCounters(bool bPosted);
		void SetArgBufferHeight(int anBufferHeight);
	public:
		void UpdateConsoleMode(CRealConsole* pRCon);
		bool CheckTheming();
		void OnPanelViewAppeared(BOOL abAppear);
		bool EditConsoleFont(HWND hParent);
		static INT_PTR CALLBACK EditConsoleFontProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		static int CALLBACK EnumConFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount);
		bool CheckConsoleFontFast(LPCWSTR asCheckName = nullptr);
		enum
		{
			ConFontErr_NonSystem   = 0x01,
			ConFontErr_NonRegistry = 0x02,
			ConFontErr_InvalidName = 0x04,
		};
	protected:
		bool bShowConFontError, bConsoleFontChecked;
		wchar_t sDefaultConFontName[32]; // "последний шанс", если юзер отказался выбрать нормальный шрифт
		HWND hConFontDlg;
		DWORD nConFontError; // 0x01 - шрифт не зарегистрирован в системе, 0x02 - не указан в реестре для консоли
		HWND hwndConFontBalloon;
		static bool CheckConsoleFontRegistry(LPCWSTR asFaceName);
		static bool CheckConsoleFont(HWND ahDlg);
		LPCWSTR CreateConFontError(LPCWSTR asReqFont=nullptr, LPCWSTR asGotFont=nullptr);
		TOOLINFO tiConFontBalloon;
		// #DPI Move DPI members to CConEmuMain
		DpiValue _dpi;
		DpiValue _dpi_all;
		void GetOverallDpi();
	public:
		int QueryDpi() const;
		void SetRequestedDpi(const DpiValue& dpi);
	private:
		CEStr ms_BalloonErrTip;
		CEStr ms_ConFontError;
		void ShowErrorTip(wchar_t* asInfo, HWND hDlg, int nCtrlID, HWND hBall, TOOLINFO *pti, HWND hTip, DWORD nTimeout, bool bLeftAligh = false);
	public:
		void ShowFontErrorTip(LPCTSTR asInfo);
		void ShowModifierErrorTip(LPCTSTR asInfo, HWND hDlg, WORD nID);
		void ShowConFontErrorTip();
		static void ApplySettingsTheme(HWND dialog);
		static INT_PTR SettingsThemeBrush(UINT message, HDC dc, HWND control, bool hyperlink = false);
	protected:
		INT_PTR OnCtlColorStatic(HWND hDlg, HDC hdc, HWND hCtrl, WORD nCtrlId);
		void OnResetOrReload(bool abResetOnly, SettingsStorage* pXmlStorage = nullptr);
		void ExportSettings();
		void ImportSettings();
		void SearchForControls(); // Find setting by typed name in the "Search" box
		// IDD_SETTINGS
		LRESULT OnInitDialog();
		//
		LRESULT OnListBoxDblClk(HWND hWnd2, WPARAM wParam, LPARAM lParam);
		LRESULT OnPage(LPNMHDR phdr);
		LRESULT OnNextPage();
		LRESULT OnPrevPage();
		INT_PTR OnMeasureFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		INT_PTR OnDrawFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
		UINT mn_ActivateTabMsg;
		bool mb_IgnoreSelPage;
	public:
		static void UpdateTextColorSettings(BOOL ChangeTextAttr = TRUE, BOOL ChangePopupAttr = TRUE, const AppSettings* apDistinct = nullptr);
		void CheckSelectionModifiers(HWND hWnd2);
		void ChangeCurrentPalette(const ColorPalette* pPal, bool bChangeDropDown);
		void RegisterTipsFor(HWND hChildDlg);
	private:
		//BOOL mb_CharSetWasSet;
		int64_t mn_Freq;
		int64_t mn_FPS[256]; int mn_FPS_CUR_FRAME;
		int64_t mn_RFPS[128]; int mn_RFPS_CUR_FRAME;
		int64_t mn_Counter[tPerfLast];
		int64_t mn_CounterMax[tPerfLast];
		DWORD mn_CounterTick[tPerfLast];
		int64_t mn_KbdDelayCounter, mn_KbdDelays[32/*must be power of 2*/]; LONG mn_KBD_CUR_FRAME;
		HWND hwndTip, hwndBalloon;
		TOOLINFO tiBalloon;
		static BOOL CALLBACK RegisterTipsForChild(HWND hChild, LPARAM lParam);
		void RecreateFont(WORD wFromID) const;
#if 0
		// Theming
		HMODULE mh_Uxtheme;
		typedef HRESULT(STDAPICALLTYPE *SetWindowThemeT)(HWND hwnd,LPCWSTR pszSubAppName,LPCWSTR pszSubIdList);
		SetWindowThemeT SetWindowThemeF;
		typedef HRESULT(STDAPICALLTYPE *EnableThemeDialogTextureT)(HWND hwnd,DWORD dwFlags);
		EnableThemeDialogTextureT EnableThemeDialogTextureF;
#endif
		UINT mn_MsgUpdateCounter; BOOL mb_MsgUpdateCounter;
		//wchar_t temp[MAX_PATH];
		UINT mn_MsgRecreateFont;
		UINT mn_MsgLoadFontFromMain;
	private:
		BOOL mb_TabHotKeyRegistered;
		void RegisterTabs();
		void UnregisterTabs();

		WORD mn_LastChangingFontCtrlId;

		bool mb_ThemingEnabled;

	private:
		static void CenterMoreDlg(HWND hWnd2);
	private:
		friend struct Settings;
	public:
		void UpdateWinHookSettings(HMODULE hLLKeyHookDll) const;
	public:
		bool isDialogMessage(MSG &Msg);
	public:
		INT_PTR ProcessTipHelp(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam);
	private:

		CDynDialog *mp_Dialog;
		ConEmuSetupPages *m_Pages;
		int mn_PagesCount;
		static void SelectTreeItem(HWND hTree, HTREEITEM hItem, bool bPost = false);
		void ClearPages();
		TabHwndIndex GetPageId(HWND hPage, ConEmuSetupPages** pp);
		const ConEmuSetupPages* GetPageData(TabHwndIndex nPage);
		TabHwndIndex GetPageIdByDialogId(UINT DialogID);
		int GetDialogDpi();
};
