
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


// cbFARuseASCIIsort, cbFixAltOnAltTab

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#include "DarkMode.h"
#include "version.h"

#include <commctrl.h>
#include "../common/shlobj.h"


#ifdef __GNUC__
#include "ShObjIdl_Part.h"
#endif // __GNUC__

#include "../common/MSetter.h"
#include "../common/StartupEnvDef.h"
#include "../common/WUser.h"
#include "../ConEmuCD/ExitCodes.h"
#include "../ConEmuCD/GuiHooks.h"

#include "Background.h"
#include "CmdHistory.h"
#include "ConEmu.h"
#include "ConfirmDlg.h"
#include "DpiAware.h"
#include "DynDialog.h"
#include "FontMgr.h"
#include "HotkeyDlg.h"
#include "ImgButton.h"
#include "LngRc.h"
#include "Options.h"
#include "OptionsClass.h"

#include "GlobalHotkeys.h"
#include "OptionsFast.h"
#include "OptionsHelp.h"
#include "RealConsole.h"
#include "SearchCtrl.h"
#include "SetColorPalette.h"
#include "SetDlgColors.h"
#include "SetDlgLists.h"
#include "SetPgANSI.h"
#include "SetPgAppear.h"
#include "SetPgApps.h"
#include "SetPgBackgr.h"
#include "SetPgBase.h"
#include "SetPgChildGui.h"
#include "SetPgColors.h"
#include "SetPgComspec.h"
#include "SetPgConfirm.h"
#include "SetPgCursor.h"
#include "SetPgDebug.h"
#include "SetPgDefTerm.h"
#include "SetPgEnvironment.h"
#include "SetPgFar.h"
#include "SetPgFarMacro.h"
#include "SetPgFeatures.h"
#include "SetPgFonts.h"
#include "SetPgHilight.h"
#include "SetPgInfo.h"
#include "SetPgIntegr.h"
#include "SetPgKeyboard.h"
#include "SetPgKeys.h"
#include "SetPgGeneral.h"
#include "SetPgMarkCopy.h"
#include "SetPgMouse.h"
#include "SetPgPaste.h"
#include "SetPgQuake.h"
#include "SetPgSizePos.h"
#include "SetPgStartup.h"
#include "SetPgStatus.h"
#include "SetPgTabs.h"
#include "SetPgTaskbar.h"
#include "SetPgTasks.h"
#include "SetPgTransparent.h"
#include "SetPgUpdate.h"
#include "SetPgViews.h"
#include "TabBar.h"
#include "VConGroup.h"
#include "VirtualConsole.h"

//#define CONEMU_ROOT_KEY L"Software\\ConEmu"

#define DEBUGSTRDPI(s) DEBUGSTR(s)

#define COUNTER_REFRESH 5000


#define FAILED_FONT_TIMEOUT 3000
#define FAILED_CONFONT_TIMEOUT 30000
#define FAILED_SELMOD_TIMEOUT 5000
#define CONTROL_FOUND_TIMEOUT 3000

#define HOTKEY_ID_TAB_NEXT 0x101
#define HOTKEY_ID_TAB_PREV 0x102

//const WORD HostkeyCtrlIds[] = {cbHostWin, cbHostApps, cbHostLCtrl, cbHostRCtrl, cbHostLAlt, cbHostRAlt, cbHostLShift, cbHostRShift};
//const BYTE HostkeyVkIds[]   = {VK_LWIN,   VK_APPS,    VK_LCONTROL, VK_RCONTROL, VK_LMENU,   VK_RMENU,   VK_LSHIFT,    VK_RSHIFT};
//int upToFontHeight=0;
HWND ghOpWnd=nullptr;

#ifdef _DEBUG
#define HEAPVAL HeapValidate(GetProcessHeap(), 0, nullptr);
#else
#define HEAPVAL
#endif

//#define SetThumbColor(s,rgb,idx,us) { (s).RawColor = 0; (s).ColorRGB = rgb; (s).ColorIdx = idx; (s).UseIndex = us; }
//#define SetThumbSize(s,sz,x1,y1,x2,y2,ls,lp,fn,fs) {
//		(s).nImgSize = sz; (s).nSpaceX1 = x1; (s).nSpaceY1 = y1; (s).nSpaceX2 = x2; (s).nSpaceY2 = y2;
//		(s).nLabelSpacing = ls; (s).nLabelPadding = lp; wcscpy_c((s).sFontName,fn); (s).nFontHeight=fs; }
//#define ThumbLoadSet(s,n) {
//		reg->Load(L"PanView." s L".ImgSize", n.nImgSize);
//		reg->Load(L"PanView." s L".SpaceX1", n.nSpaceX1);
//		reg->Load(L"PanView." s L".SpaceY1", n.nSpaceY1);
//		reg->Load(L"PanView." s L".SpaceX2", n.nSpaceX2);
//		reg->Load(L"PanView." s L".SpaceY2", n.nSpaceY2);
//		reg->Load(L"PanView." s L".LabelSpacing", n.nLabelSpacing);
//		reg->Load(L"PanView." s L".LabelPadding", n.nLabelPadding);
//		reg->Load(L"PanView." s L".FontName", n.sFontName, countof(n.sFontName));
//		reg->Load(L"PanView." s L".FontHeight", n.nFontHeight); }
//#define ThumbSaveSet(s,n) {
//		reg->Save(L"PanView." s L".ImgSize", n.nImgSize);
//		reg->Save(L"PanView." s L".SpaceX1", n.nSpaceX1);
//		reg->Save(L"PanView." s L".SpaceY1", n.nSpaceY1);
//		reg->Save(L"PanView." s L".SpaceX2", n.nSpaceX2);
//		reg->Save(L"PanView." s L".SpaceY2", n.nSpaceY2);
//		reg->Save(L"PanView." s L".LabelSpacing", n.nLabelSpacing);
//		reg->Save(L"PanView." s L".LabelPadding", n.nLabelPadding);
//		reg->Save(L"PanView." s L".FontName", n.sFontName);
//		reg->Save(L"PanView." s L".FontHeight", n.nFontHeight); }


CSettings::CSettings()
	: mp_Dialog()
	, m_Pages()
	, mn_PagesCount()
	, mp_BgInfo()
	, m_ColorFormat(eRgbDec) // RRR GGG BBB (как показывать цвета на вкладке Colors)
	, mp_DpiAware()
	, mp_ImgBtn()
	, mn_LastChangingFontCtrlId()
{
	// Prepare global pointers
	gpSetCls = this;
	gpSet = &m_Settings;

	GetOverallDpi();

	// Go
	isResetBasicSettings = false;
	isFastSetupDisabled = false;
	isDontCascade = false; // -nocascade or -dontcascade switch
	ibDisableSaveSettingsOnExit = false;
	bForceBufferHeight = false; nForceBufferHeight = 1000; /* устанавливается в true, из ком.строки /BufferHeight */

	#ifdef SHOW_AUTOSCROLL
	AutoScroll = true;
	#endif

	isAllowDetach = 0;
	mb_ThemingEnabled = (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1));
	//isFullScreen = false;
	//ZeroStruct(m_QuakePrevSize);

	// Некоторые вещи нужно делать вне InitSettings, т.к. она может быть
	// вызвана потом из интерфейса диалога настроек
	wcscpy_c(ConfigPath, CONEMU_ROOT_KEY L"\\.Vanilla");
	ConfigName[0] = 0;

	m_ThSetMap.InitName(CECONVIEWSETNAME, GetCurrentProcessId());
	if (!m_ThSetMap.Create())
	{
		MBoxA(m_ThSetMap.GetErrorText());
	}
	else
	{
		// Применить в Mapping (там заодно и палитра копируется)
		//m_ThSetMap.Set(&gpSet->ThSet);
		//!! Это нужно делать после создания основного шрифта
		//gpConEmu->OnPanelViewSettingsChanged(FALSE);
	}

	// Теперь установим умолчания настроек
	gpSet->InitSettings();

	SingleInstanceArg = sgl_Default;
	SingleInstanceShowHide = sih_None;
	gpSet->mb_CharSetWasSet = FALSE;
	mb_TabHotKeyRegistered = FALSE;
	//hMain = hExt = hFar = hTabs = hKeys = hColors = hCmdTasks = hViews = hInfo = hDebug = hUpdate = hSelection = nullptr;
	m_LastActivePageId = thi_Last;
	hwndTip = nullptr; hwndBalloon = nullptr;
	hConFontDlg = nullptr; hwndConFontBalloon = nullptr; bShowConFontError = FALSE; bConsoleFontChecked = FALSE;
	if (gbIsDBCS)
	{
		wcscpy_c(sDefaultConFontName, gsDefConFont);
	}
	else
	{
		sDefaultConFontName[0] = 0;
	}
	QueryPerformanceFrequency((LARGE_INTEGER *)&mn_Freq);
	memset(mn_Counter, 0, sizeof(mn_Counter));
	memset(mn_CounterMax, 0, sizeof(mn_CounterMax));
	memset(mn_FPS, 0, sizeof(mn_FPS)); mn_FPS_CUR_FRAME = 0;
	memset(mn_RFPS, 0, sizeof(mn_RFPS)); mn_RFPS_CUR_FRAME = 0;
	memset(mn_CounterTick, 0, sizeof(mn_CounterTick));
	memset(mn_KbdDelays, 0, sizeof(mn_KbdDelays)); mn_KbdDelayCounter = 0; mn_KBD_CUR_FRAME = -1;
	//hBgBitmap = nullptr; bgBmp = MakeCoord(0,0); hBgDc = nullptr;
	#ifndef APPDISTINCTBACKGROUND
	mb_BgLastFade = false;
	mp_Bg = nullptr;
	mp_BgImgData = nullptr;
	isBackgroundImageValid = false;
	mb_NeedBgUpdate = FALSE; //mb_WasVConBgImage = FALSE;
	ftBgModified.dwHighDateTime = ftBgModified.dwLowDateTime = nBgModifiedTick = 0;
	#else
	#endif
#if 0
	//120714 - аналогичные параметры работают в ConEmuC.exe, а в GUI они и не работали. убрал пока
	nAttachPID = 0; hAttachConWnd = nullptr;
#endif
	nConFontError = 0;
	memset(&tiBalloon, 0, sizeof(tiBalloon));
	//mn_FadeMul = gpSet->mn_FadeHigh - gpSet->mn_FadeLow;
	//gpSet->mn_LastFadeSrc = gpSet->mn_LastFadeDst = -1;

	#if 0
	SetWindowThemeF = nullptr;
	mh_Uxtheme = LoadLibrary(L"UxTheme.dll");

	if (mh_Uxtheme)
	{
		SetWindowThemeF = (SetWindowThemeT)GetProcAddress(mh_Uxtheme, "SetWindowTheme");
		EnableThemeDialogTextureF = (EnableThemeDialogTextureT)GetProcAddress(mh_Uxtheme, "EnableThemeDialogTexture");
		//if (SetWindowThemeF) { SetWindowThemeF(Progressbar1, L" ", L" "); }
	}
	#endif

	mn_MsgUpdateCounter = RegisterWindowMessage(L"ConEmuSettings::Counter"); mb_MsgUpdateCounter = FALSE;
	mn_MsgRecreateFont = RegisterWindowMessage(L"Settings::RecreateFont");
	mn_MsgLoadFontFromMain = RegisterWindowMessage(L"Settings::LoadFontNames");
	mn_ActivateTabMsg = RegisterWindowMessage(L"Settings::ActivateTab");

	// Settings pages definitions
	InitVars_Pages();

	// Hotkeys list
	CSetPgKeys::ReInitHotkeys();

	memset(&tiConFontBalloon, 0, sizeof(tiConFontBalloon));
	mb_IgnoreSelPage = false;
}

void CSettings::GetOverallDpi()
{
	// Must be called during initialization only
	_ASSERTEX(!gpConEmu || gpConEmu->GetStartupStage() < CConEmuMain::ss_PostCreate2Called);
	_dpi_all = CDpiAware::QueryDpiForMonitor(nullptr);
	_ASSERTE(_dpi_all.Xdpi >= 96 && _dpi_all.Ydpi >= 96);
	_dpi.SetDpi(_dpi_all);
}

// This returns current dpi for main window
int CSettings::QueryDpi() const
{
	return _dpi.Ydpi;
}

// Called during jump to monitor with different dpi
void CSettings::SetRequestedDpi(const DpiValue& dpi)
{
	if (dpi.Xdpi <= 0 || dpi.Ydpi <= 0)
	{
		_ASSERTE(dpi.Xdpi > 0 && dpi.Ydpi > 0);
		return;
	}
	_dpi.SetDpi(dpi);
}

void CSettings::UpdateWinHookSettings(HMODULE hLLKeyHookDll) const
{
	BOOL *pbWinTabHook = reinterpret_cast<BOOL*>(GetProcAddress(hLLKeyHookDll, "gbWinTabHook"));
	BYTE *pnConsoleKeyShortcuts = reinterpret_cast<BYTE*>(GetProcAddress(hLLKeyHookDll, "gnConsoleKeyShortcuts"));

	if (pbWinTabHook)
		*pbWinTabHook = gpSet->isUseWinTab;

	if (pnConsoleKeyShortcuts)
	{
		BYTE nNewValue = 0;

		if (gpSet->isSendAltTab) nNewValue |= 1<<ID_ALTTAB;
		if (gpSet->isSendAltEsc) nNewValue |= 1<<ID_ALTESC;
		if (gpSet->isSendAltPrintScrn) nNewValue |= 1<<ID_ALTPRTSC;
		if (gpSet->isSendPrintScrn) nNewValue |= 1<<ID_PRTSC;
		if (gpSet->isSendCtrlEsc) nNewValue |= 1<<ID_CTRLESC;

		CVConGuard VCon;
		for (int i = 0; CVConGroup::GetVCon(i, &VCon, true); i++)
		{
			nNewValue |= VCon->RCon()->GetConsoleKeyShortcuts();
		}

		*pnConsoleKeyShortcuts = nNewValue;
	}

	// __declspec(dllexport) DWORD gnHookedKeys[HookedKeysMaxCount] = {};
	DWORD *pnHookedKeys = reinterpret_cast<DWORD*>(GetProcAddress(hLLKeyHookDll, "gnHookedKeys"));
	if (pnHookedKeys)
	{
		DWORD *pn = pnHookedKeys;

		//WARNING("CConEmuCtrl:: Тут вообще наверное нужно по всем HotKeys проехаться, а не только по «избранным»");
		for (int i = 0;; i++)
		{
			const ConEmuHotKey* pHK = gpHotKeys->GetHotKeyPtr(i);
			if (!pHK)
				break;

			// chk_Local may fail to be registered by RegisterHotKey
			// so it's better to try both ways if possible
			if ((pHK->HkType == chk_Modifier) || (pHK->HkType == chk_Global))
				continue;

			const DWORD vkMod = pHK->GetVkMod();

			if (!ConEmuChord::HasModifier(vkMod, VK_LWIN))
				continue;

			if (pHK->Enabled && !pHK->Enabled())
				continue;

			DWORD nFlags = ConEmuChord::GetHotkey(vkMod);
			for (int modIndex = 1; modIndex <= 3; modIndex++)
			{
				switch (ConEmuChord::GetModifier(vkMod, modIndex))
				{
				case 0:
					break;
				case VK_LWIN: case VK_RWIN:
					nFlags |= cvk_Win; break;
				case VK_APPS:
					nFlags |= cvk_Apps; break;
				case VK_CONTROL:
					nFlags |= cvk_Ctrl; break;
				case VK_LCONTROL:
					nFlags |= cvk_LCtrl | cvk_Ctrl; break;
				case VK_RCONTROL:
					nFlags |= cvk_RCtrl | cvk_Ctrl; break;
				case VK_MENU:
					nFlags |= cvk_Alt; break;
				case VK_LMENU:
					nFlags |= cvk_LAlt | cvk_Alt; break;
				case VK_RMENU:
					nFlags |= cvk_RAlt | cvk_Alt; break;
				case VK_SHIFT:
					nFlags |= cvk_Shift; break;
				case VK_LSHIFT:
					nFlags |= cvk_LShift | cvk_Shift; break;
				case VK_RSHIFT:
					nFlags |= cvk_RShift | cvk_Shift; break;
				}
			}

			*(pn++) = nFlags;
		}

		*pn = 0;
		_ASSERTE((pn - pnHookedKeys) < (HookedKeysMaxCount-1));
	}
}

void CSettings::InitVars_Pages()
{
	ConEmuSetupPages Pages[] =
	{
		// При добавлении вкладки нужно добавить OnInitDialog_XXX в pageOpProc
		{IDD_SPG_GENERAL,     0, lng_SpgGeneral,      thi_General,      L"SettingsFast.html",         CSetPgGeneral::Create},
		{IDD_SPG_FONTS,       1, lng_SpgFonts,        thi_Fonts,        L"SettingsFonts.html",        CSetPgFonts::Create},
		{IDD_SPG_SIZEPOS,     1, lng_SpgSizePos,      thi_SizePos,      L"SettingsSizePos.html",      CSetPgSizePos::Create},
		{IDD_SPG_APPEAR,      1, lng_SpgAppear,       thi_Appear,       L"SettingsAppearance.html",   CSetPgAppear::Create},
		{IDD_SPG_QUAKE,       1, lng_SpgQuake,        thi_Quake,        L"SettingsQuake.html",        CSetPgQuake::Create},
		{IDD_SPG_BACKGR,      1, lng_SpgBackgr,       thi_Backgr,       L"SettingsBackground.html",   CSetPgBackgr::Create},
		{IDD_SPG_TABS,        1, lng_SpgTabBar,       thi_Tabs,         L"SettingsTabBar.html",       CSetPgTabs::Create},
		{IDD_SPG_CONFIRM,     1, lng_SpgConfirm,      thi_Confirm,      L"SettingsConfirm.html",      CSetPgConfirm::Create},
		{IDD_SPG_TASKBAR,     1, lng_SpgTaskBar,      thi_Taskbar,      L"SettingsTaskBar.html",      CSetPgTaskbar::Create},
		{IDD_SPG_UPDATE,      1, lng_SpgUpdate,       thi_Update,       L"SettingsUpdate.html",       CSetPgUpdate::Create},
		{IDD_SPG_STARTUP,     0, lng_SpgStartup,      thi_Startup,      L"SettingsStartup.html",      CSetPgStartup::Create},
		{IDD_SPG_TASKS,       1, lng_SpgTasks,        thi_Tasks,        L"SettingsTasks.html",        CSetPgTasks::Create},
		{IDD_SPG_ENVIRONMENT, 1, lng_SpgEnvironment,  thi_Environment,  L"SettingsEnvironment.html",  CSetPgEnvironment::Create},
		{IDD_SPG_FEATURES,    0, lng_SpgFeatures,     thi_Features,     L"SettingsFeatures.html",     CSetPgFeatures::Create},
		{IDD_SPG_CURSOR,      1, lng_SpgCursor,       thi_Cursor,       L"SettingsTextCursor.html",   CSetPgCursor::Create},
		{IDD_SPG_COLORS,      1, lng_SpgColors,       thi_Colors,       L"SettingsColors.html",       CSetPgColors::Create},
		{IDD_SPG_TRANSPARENT, 1, lng_SpgTransparent,  thi_Transparent,  L"SettingsTransparency.html", CSetPgTransparent::Create},
		{IDD_SPG_STATUSBAR,   1, lng_SpgStatusBar,    thi_Status,       L"SettingsStatusBar.html",    CSetPgStatus::Create},
		{IDD_SPG_APPDISTINCT, 1, lng_SpgAppDistinct,  thi_Apps,         L"SettingsAppDistinct.html",  CSetPgApps::Create},
		{IDD_SPG_INTEGRATION, 0, lng_SpgIntegration,  thi_Integr,       L"SettingsIntegration.html",  CSetPgIntegr::Create},
		{IDD_SPG_DEFTERM,     1, lng_SpgDefTerm,      thi_DefTerm,      L"SettingsDefTerm.html",      CSetPgDefTerm::Create},
		{IDD_SPG_COMSPEC,     1, lng_SpgComSpec,      thi_Comspec,      L"SettingsComspec.html",      CSetPgComspec::Create},
		{IDD_SPG_CHILDGUI,    1, lng_SpgChildGui,     thi_ChildGui,     L"SettingsChildGui.html",     CSetPgChildGui::Create},
		{IDD_SPG_ANSI,        1, lng_SpgANSI,         thi_ANSI,         L"SettingsANSI.html",         CSetPgANSI::Create},
		{IDD_SPG_KEYS,        0, lng_SpgKeys,         thi_Keys,         L"SettingsHotkeys.html",      CSetPgKeys::Create},
		{IDD_SPG_KEYBOARD,    1, lng_SpgKeyboard,     thi_Keyboard,     L"SettingsKeyboard.html",     CSetPgKeyboard::Create},
		{IDD_SPG_MOUSE,       1, lng_SpgMouse,        thi_Mouse,        L"SettingsMouse.html",        CSetPgMouse::Create},
		{IDD_SPG_MARKCOPY,    1, lng_SpgMarkCopy,     thi_MarkCopy,     L"SettingsMarkCopy.html",     CSetPgMarkCopy::Create},
		{IDD_SPG_PASTE,       1, lng_SpgPaste,        thi_Paste,        L"SettingsPaste.html",        CSetPgPaste::Create},
		{IDD_SPG_HIGHLIGHT,   1, lng_SpgHighlight,    thi_Hilight,      L"SettingsHighlight.html",    CSetPgHilight::Create},
		{IDD_SPG_FEATURE_FAR, 0, lng_SpgFarManager,   thi_Far,          L"SettingsFar.html",          CSetPgFar::Create,       true/*Collapsed*/},
		{IDD_SPG_FARMACRO,    1, lng_SpgFarMacros,    thi_FarMacro,     L"SettingsFarMacros.html",    CSetPgFarMacro::Create},
		{IDD_SPG_VIEWS,       1, lng_SpgFarViews,     thi_Views,        L"SettingsFarView.html",      CSetPgViews::Create},
		{IDD_SPG_INFO,        0, lng_SpgInfo,         thi_Info,         L"SettingsInfo.html",         CSetPgInfo::Create,      RELEASEDEBUGTEST(true,false)/*Collapsed in Release*/},
		{IDD_SPG_DEBUG,       1, lng_SpgDebug,        thi_Debug,        L"SettingsDebug.html",        CSetPgDebug::Create},
		// End
		{},
	};

	#ifdef _DEBUG
	for (size_t i = 0; Pages[i].DialogID; i++)
	{
		_ASSERTE(Pages[i].PageIndex == (TabHwndIndex)i);
	}
	#endif

	mn_PagesCount = ((int)countof(Pages) - 1);
	_ASSERTE(mn_PagesCount>0 && Pages[mn_PagesCount].DialogID==0);
	_ASSERTE(mn_PagesCount == thi_Last);

	m_Pages = (ConEmuSetupPages*)malloc(sizeof(Pages));
	memmove(m_Pages, Pages, sizeof(Pages));
}

void CSettings::InitPageNames()
{
	if (!m_Pages)
	{
		_ASSERTE(m_Pages && (mn_PagesCount > 0));
		return;
	}

	for (int i = 0; i < mn_PagesCount; i++)
	{
		lstrcpyn(m_Pages[i].PageName, CLngRc::getRsrc(m_Pages[i].PageNameRsrc), countof(m_Pages[i].PageName));
	}
}

void CSettings::ReleaseHandles()
{
	//if (sTabCloseMacro) {free(sTabCloseMacro); sTabCloseMacro = nullptr;}
	//if (sSafeFarCloseMacro) {free(sSafeFarCloseMacro); sSafeFarCloseMacro = nullptr;}
	//if (sSaveAllMacro) {free(sSaveAllMacro); sSaveAllMacro = nullptr;}
	//if (sRClickMacro) {free(sRClickMacro); sRClickMacro = nullptr;}

	#ifndef APPDISTINCTBACKGROUND
	SafeDelete(mp_Bg);
	SafeFree(mp_BgImgData);
	#else
	SafeRelease(mp_BgInfo);
	#endif
}

CSettings::~CSettings()
{
	// CSettings must be deleted AFTER ConEmu dtor
	_ASSERTE(gpConEmu == nullptr);

	ReleaseHandles();

	//if (gpSet->psCmd) {free(gpSet->psCmd); gpSet->psCmd = nullptr;}

	//if (gpSet->psCmdHistory) {free(gpSet->psCmdHistory); gpSet->psCmdHistory = nullptr;}

	//if (gpSet->psCurCmd) {free(gpSet->psCurCmd); gpSet->psCurCmd = nullptr;}

#if 0
	if (mh_Uxtheme!=nullptr) { FreeLibrary(mh_Uxtheme); mh_Uxtheme = nullptr; }
#endif

	CSetDlgColors::ReleaseHandles();

	//if (gpSet)
	//{
	//	delete gpSet;
	//	gpSet = nullptr;
	//}

	gpHotKeys->ReleaseHotkeys();

	SafeFree(m_Pages);

	SafeDelete(mp_Dialog);
	SafeDelete(mp_DpiAware);

	gpSet = nullptr;
}

LPCWSTR CSettings::GetConfigPath()
{
	return ConfigPath;
}

LPCWSTR CSettings::GetConfigName()
{
	return ConfigName;
}

void CSettings::SetConfigName(LPCWSTR asConfigName)
{
	if (IsStrNotEmpty(asConfigName))
	{
		_wcscpyn_c(ConfigName, countof(ConfigName), asConfigName, wcslen(asConfigName));
		ConfigName[countof(ConfigName)-1] = 0; //checkpoint
		wcscpy_c(ConfigPath, CONEMU_ROOT_KEY L"\\");
		wcscat_c(ConfigPath, ConfigName);
	}
	else
	{
		wcscpy_c(ConfigPath, CONEMU_ROOT_KEY L"\\.Vanilla");
		ConfigName[0] = 0;
	}
	SetEnvironmentVariable(ENV_CONEMU_CONFIG_W, ConfigName);
}

bool CSettings::SetOption(LPCWSTR asName, int anValue)
{
	bool lbRc = true;

	if (!lstrcmpi(asName, L"CursorBlink"))
	{
		gpSet->AppStd.CursorActive.isBlinking = (anValue != 0);
		HWND hCursorPg = GetPage(thi_Cursor);
		if (hCursorPg)
			checkDlgButton(hCursorPg, cbCursorBlink, (anValue != 0));
		// Может выполняться в фоновом потоке!
		// При таком (ниже) запуске после старта курсор не отрисовывается в промпте
		// -basic -cmd cmd /k conemuc -guimacro status 0 2 -guimacro setoption cursorblink 0 & dir
	}
	else
	{
		lbRc = false;
	}

	return lbRc;
}

bool CSettings::SetOption(LPCWSTR asName, LPCWSTR asValue)
{
	bool lbRc = false;

	if (!lstrcmpi(asName, L"FarGotoEditorPath"))
	{
		wchar_t* pszNewVal = lstrdup(asValue).Detach();
		if (pszNewVal && *pszNewVal)
		{
			wchar_t* pszOld = static_cast<wchar_t*>(InterlockedExchangePointer(reinterpret_cast<PVOID*>(&gpSet->sFarGotoEditor), pszNewVal));
			SafeFree(pszOld);
			HWND hHilightPg = GetPage(thi_Hilight);
			if (hHilightPg)
				SetDlgItemText(hHilightPg, lbGotoEditorCmd, gpSet->sFarGotoEditor);
			lbRc = true;
		}
	}
	else if (!lstrcmpi(asName, L"BackGround Image") || !lstrcmpi(asName, L"bgImage"))
	{
		auto reloadBgImage = [](LPARAM lParam) -> LRESULT
		{
			if (gpSetCls->LoadBackgroundFile((LPCWSTR)lParam, true))
			{
				wcscpy_c(gpSet->sBgImage, (LPCWSTR)lParam);
				HWND hBgPg = gpSetCls->GetPage(thi_Hilight);
				if (hBgPg)
					SetDlgItemText(hBgPg, tBgImage, gpSet->sBgImage);
				gpSetCls->NeedBackgroundUpdate();
				gpConEmu->Update(true);
				return 1;
			}
			return 0;
		};

		lbRc = (gpConEmu->CallMainThread(true, reloadBgImage, reinterpret_cast<LPARAM>(asValue)) != 0);
	}
	else if (!lstrcmpi(asName, L"Scheme"))
	{
		const ColorPalette* pPal = gpSet->PaletteGetByName(asValue);
		if (pPal)
		{
			ChangeCurrentPalette(pPal, true);
			lbRc = true;
		}
	}
	else
	{
		_ASSERTE(FALSE && "Unsupported parameter name");
	}

	return lbRc;
}

void CSettings::SettingsLoaded(SettingsLoadedFlags slfFlags, LPCWSTR pszCmdLine /*= nullptr*/)
{
	// Logging may be enabled in Settings permanently
	if (gpSet->isLogging())
	{
		gpConEmu->CreateLog();
	}

	_ASSERTE(gpLng != nullptr);
	if (gpLng)
		gpLng->Reload();

	// Expose the directory where xml file was loaded from
	SettingsStorage Storage = gpSet->GetSettingsType();

	// gh-1082: "ConEmuCfgDir" << folder where our xml-file is located
	CEStr CfgFilePath(GetParentPath(Storage.File));
	SetEnvironmentVariable(ENV_CONEMUCFGDIR_VAR_W, CfgFilePath.c_str(L""));

	if ((ghWnd == nullptr) || (slfFlags & slf_OnResetReload))
	{
		gpConEmu->WndPos.x = gpSet->_wndX;
		gpConEmu->WndPos.y = gpSet->_wndY;
		gpConEmu->WndWidth.Raw = gpSet->wndWidth.Raw;
		gpConEmu->WndHeight.Raw = gpSet->wndHeight.Raw;
	}

	// Recheck dpi settings?
	if (ghWnd == nullptr)
	{
		_dpi = gpConEmu->GetInitialDpi();
		wchar_t szInfo[100];
		swprintf_c(szInfo, L"DPI initialized to {%i,%i}\r\n", _dpi.Xdpi, _dpi.Ydpi);
		DEBUGSTRDPI(szInfo);
		LogString(szInfo, true, false);
	}

	#if 0
	// Не требуется, размер будет скорректирован в CreateMainWindow()
	if (slfFlags & (slf_NeedCreateVanilla|slf_DefaultSettings) && (gpSet->FontSizeY > 1))
	{
		// Under debug the default console width is set to 110x35,
		// that may be too large on small displays
		MONITORINFO mi = {};
		if (gpConEmu->FindInitialMonitor(&mi))
		{
			// Примерно, лишь бы сильно за рабочую область не вылезало...
			RECT rcCon = gpConEmu->CalcRect(CER_CONSOLE_ALL, mi.rcWork, CER_MONITOR);
			//RECT rcDC = gpConEmu->CalcRect(CER_BACK, mi.rcWork, CER_MONITOR);
			//RECT rcCon = MakeRect((rcDC.right - rcDC.left) / (gpSet->FontSizeX ? gpSet->FontSizeX : (gpSet->FontSizeY / 2)), (rcDC.bottom - rcDC.top) / gpSet->FontSizeY);
			if ((rcCon.right > 0) && (rcCon.bottom > 0)
				&& ((rcCon.right < gpSet->wndWidth.Value) || (rcCon.bottom < gpSet->wndHeight.Value)))
			{
				gpSet->wndWidth.Set(true, ss_Standard, std::min(gpSet->wndWidth.Value, rcCon.right));
				gpSet->wndHeight.Set(false, ss_Standard, std::min(gpSet->wndHeight.Value, rcCon.bottom));
				gpConEmu->WndWidth.Raw = gpSet->wndWidth.Raw;
				gpConEmu->WndHeight.Raw = gpSet->wndHeight.Raw;
			}
		}
	}
	#endif

	// Обработать 32/64 (найти tcc.exe и т.п.)
	FindComspec(&gpSet->ComSpec);

	// Зовем "FastConfiguration" (перед созданием новой/чистой конфигурации)
	if (slfFlags & slf_AllowFastConfig)
	{
		LPCWSTR pszDef = gpConEmu->GetDefaultTitle();
		LPCWSTR pszConfig = gpSetCls->GetConfigName();

		CEStr szTitle;
		LPCWSTR pszFastCfgTitle = CLngRc::getRsrc(lng_DlgFastCfg/*"fast configuration"*/);
		if (pszConfig && *pszConfig)
			szTitle = CEStr(pszDef, L" ", pszFastCfgTitle, L" (", pszConfig, L")");
		else
			szTitle = CEStr(pszDef, L" ", pszFastCfgTitle);

		// Run "Fast configuration dialog" and apply some final defaults (if was Reset of new settings)
		FastConfig::CheckOptionsFast(szTitle, slfFlags);

		// Single instance?
		if (gpSet->isSingleInstance && (gpSetCls->SingleInstanceArg == sgl_Default))
		{
			if ((pszCmdLine && *pszCmdLine) || (gpConEmu->m_StartDetached == crb_On))
			{
				// Должен быть "sih_None" иначе существующая копия не запустит команду
				_ASSERTE(SingleInstanceShowHide == sih_None);
			}
			else
			{
				SingleInstanceShowHide = sih_ShowMinimize;
			}
		}
	}


	if (slfFlags & slf_NeedCreateVanilla)
	{
		if (!gpConEmu->IsResetBasicSettings())
		{
			gpSet->SaveSettings(TRUE/*abSilent*/);
		}
		else
		{
			// Must not be here when using "/basic" switch!
			_ASSERTE(!gpConEmu->IsResetBasicSettings());
		}
	}

	// Issue 1191: ConEmu was launched instead of explorer from taskbar pinned library icon
	bool bHasLibraries = IsWindows7;
	if (bHasLibraries)
	{
		CSetPgIntegr::UnregisterShellInvalids();
	}

	// Передернуть палитру затенения
	gpSet->ResetFadeColors();
	gpSet->GetColors(-1, TRUE);


	// Назначить корректные флаги для кнопок Win+Number и Win+Arrow
	gpHotKeys->UpdateNumberModifier();
	gpHotKeys->UpdateArrowModifier();


	// Проверить необходимость установки хуков
	//-- isKeyboardHooks();
	// При первом запуске - проверить, хотят ли включить автообновление?
	//-- CheckUpdatesWanted();



	// Стили окна
	if ((gpSet->_WindowMode != wmNormal)
		&& (gpSet->_WindowMode != wmMaximized)
		&& (gpSet->_WindowMode != wmFullScreen))
	{
		// Иначе окно вообще не отображается
		_ASSERTE(gpSet->_WindowMode!=0);
		gpSet->_WindowMode = wmNormal;
	}

	//if (rcTabMargins.top > 100) rcTabMargins.top = 100;
	//_ASSERTE((rcTabMargins.bottom == 0 || rcTabMargins.top == 0) && !rcTabMargins.left && !rcTabMargins.right);
	//rcTabMargins.left = rcTabMargins.right = 0;
	//int nTabHeight = rcTabMargins.top ? rcTabMargins.top : rcTabMargins.bottom;
	//if (nTabsLocation == 1)
	//{
	//	rcTabMargins.top = 0; rcTabMargins.bottom = nTabHeight;
	//}
	//else
	//{
	//	rcTabMargins.top = nTabHeight; rcTabMargins.bottom = 0;
	//}

	if (!gpSet->pHistory)
	{
		gpSet->pHistory = new CommandHistory(MAX_CMD_HISTORY);
	}

	MCHKHEAP

	if ((SingleInstanceArg == sgl_Default) && gpSet->isQuakeStyle)
	{
		_ASSERTE((SingleInstanceShowHide == sih_None) || (gpSet->isSingleInstance && (SingleInstanceShowHide == sih_ShowMinimize)));
		SingleInstanceArg = sgl_Enabled;
	}

	//wcscpy_c(gpSet->ComSpec.ConEmuDir, gpConEmu->ms_ConEmuDir);
	gpConEmu->InitComSpecStr(gpSet->ComSpec);
	// -- должно вообще вызываться в UpdateGuiInfoMapping
	//UpdateComspec(&gpSet->ComSpec);

	// Init custom palette for ColorSelection dialog
	CSetDlgColors::OnSettingsLoaded(gpSet->Colors);

	gpFontMgr->SettingsLoaded(slfFlags);

	// Стили окна
	// Т.к. вызывается из Settings::LoadSettings() то проверка на валидность уже не нужно, оставим только ассерт
	_ASSERTE(gpSet->_WindowMode == wmNormal || gpSet->_WindowMode == wmMaximized || gpSet->_WindowMode == wmFullScreen);

	if (ghWnd == nullptr)
	{
		gpConEmu->WindowMode = (ConEmuWindowMode)gpSet->_WindowMode;
	}
	else
	{
		gpConEmu->SetWindowMode((ConEmuWindowMode)gpSet->_WindowMode);
	}

	if (!gpConEmu->InCreateWindow())
	{
		gpConEmu->OnGlobalSettingsChanged();
	}

	MCHKHEAP
}

void CSettings::SettingsPreSave()
{
	// Do not get data from LogFont if it was not created yet
	if (gpConEmu->GetStartupStage() >= CConEmuMain::ss_Started)
	{
		gpFontMgr->SettingsPreSave();
	}

	// Макросы, совпадающие с "умолчательными" - не пишем
	if (gpSet->sRClickMacro && (lstrcmp(gpSet->sRClickMacro, gpSet->RClickMacroDefault(fmv_Default)) == 0 || lstrcmp(gpSet->sRClickMacro, gpSet->RClickMacroDefault(fmv_Lua)) == 0))
		SafeFree(gpSet->sRClickMacro);
	if (gpSet->sSafeFarCloseMacro && (lstrcmp(gpSet->sSafeFarCloseMacro, gpSet->SafeFarCloseMacroDefault(fmv_Default)) == 0 || lstrcmp(gpSet->sSafeFarCloseMacro, gpSet->SafeFarCloseMacroDefault(fmv_Lua)) == 0))
		SafeFree(gpSet->sSafeFarCloseMacro);
	if (gpSet->sTabCloseMacro && (lstrcmp(gpSet->sTabCloseMacro, gpSet->TabCloseMacroDefault(fmv_Default)) == 0 || lstrcmp(gpSet->sTabCloseMacro, gpSet->TabCloseMacroDefault(fmv_Lua)) == 0))
		SafeFree(gpSet->sTabCloseMacro);
	if (gpSet->sSaveAllMacro && (lstrcmp(gpSet->sSaveAllMacro, gpSet->SaveAllMacroDefault(fmv_Default)) == 0 || lstrcmp(gpSet->sSaveAllMacro, gpSet->SaveAllMacroDefault(fmv_Lua)) == 0))
		SafeFree(gpSet->sSaveAllMacro);
}



// Помножить размер на масштаб * dpi * юниты(-1)
LONG CSettings::EvalSize(LONG nSize, EvalSizeFlags Flags, const DpiValue* apDpi /*= nullptr*/) const
{
	if (nSize <= 0)
	{
		// Must not be used for "units"!
		// This must be used for evaluate "pixels" or sort of.
		// Positive values only...
		_ASSERTE(nSize >= 0);
		return 0;
	}

	if (!apDpi)
		apDpi = &_dpi;

	LONG iMul = 1, iDiv = 1;

	// DPI текущего(!) монитора
	if ((Flags & esf_CanUseDpi) && gpSet->FontUseDpi)
	{
		const int iDpi = (Flags & esf_Horizontal) ? apDpi->Xdpi : apDpi->Ydpi;
		if (iDpi > 0)
		{
			iMul *= iDpi;
			iDiv *= 96;
		}
		else
		{
			_ASSERTE(iDpi > 0);
		}
	}

	// Zooming, current, is not stored in the settings
	if (Flags & esf_CanUseZoom)
	{
		if (gpFontMgr->mn_FontZoomValue && (gpFontMgr->mn_FontZoomValue != CFontMgr::FontZoom100))
		{
			iMul *= gpFontMgr->mn_FontZoomValue;
			iDiv *= CFontMgr::FontZoom100;
		}
	}

	// Сейчас множитель-делитель должны быть >0
	_ASSERTE(iMul>0 && iDiv>0);

	// Units (char height) or pixels (cell height)?
	if ((Flags & esf_CanUseUnits) && (Flags & esf_Vertical)
		&& gpSet->FontUseUnits && (nSize > 0))
	{
		iMul = -iMul;
	}

	const LONG iResult = MulDiv(nSize, iMul, iDiv);
	return iResult;
}




// Find setting by typed name in the "Search" box
void CSettings::SearchForControls()
{
	HWND hSearchEdit = GetDlgItem(ghOpWnd, tOptionSearch);
	CEStr pszPart = GetDlgItemTextPtr(hSearchEdit, 0);
	if (pszPart.IsEmpty())
	{
		return;
	}

	TOOLINFO toolInfo{};
	toolInfo.cbSize = 44; //sizeof(toolInfo); -- need to work on Win2k and compile with Vista+
	toolInfo.uFlags = TTF_IDISHWND;
	toolInfo.hwnd = ghOpWnd;
	toolInfo.uId = reinterpret_cast<UINT_PTR>(hSearchEdit);
	SendMessage(hwndTip, TTM_DELTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));

	size_t i, s, iTab = 0, iCurTab = 0;
	INT_PTR lFind = -1;
	HWND hSelTab = nullptr, hCurTab = nullptr, hCtrl = nullptr;
	static HWND hLastTab, hLastCtrl;
	INT_PTR lLastListFind = -1;
	wchar_t szText[255] = L"", szClass[80];
	static wchar_t szLastText[255];

	#define ResetLastList() { lLastListFind = -1; szLastText[0] = 0; }
	#define ResetLastCtrl() { hLastCtrl = nullptr; ResetLastList(); }

	SetCursor(LoadCursor(nullptr,IDC_WAIT));

	for (i = 0; m_Pages[i].DialogID; i++)
	{
		if (m_Pages[i].hPage && IsWindowVisible(m_Pages[i].hPage))
		{
			hSelTab = m_Pages[i].hPage;
			iTab = i;
			break;
		}
	}

	if (!hSelTab)
	{
		_ASSERTE(hSelTab!=nullptr);
		goto wrap;
	}

	if (hLastTab != hSelTab)
	{
		ResetLastCtrl();
	}

	for (s = 0; s <= 2; s++)
	{
		size_t iFrom = iTab, iTo = iTab + 1;

		if (s)
		{
			ResetLastCtrl();
		}

		if (s == 1)
		{
			iFrom = iTab+1; iTo = thi_Last;
		}
		else if (s == 2)
		{
			iFrom = 0; iTo = iTab;
		}

		for (i = iFrom; (i < iTo) && m_Pages[i].DialogID; i++)
		{
			if (m_Pages[i].hPage == nullptr)
			{
				CSetPgBase::CreatePage(&(m_Pages[i]), ghOpWnd, mn_ActivateTabMsg, mp_DpiAware);
			}

			iCurTab = i;
			hCurTab = m_Pages[i].hPage;

			if (hCurTab == nullptr)
			{
				_ASSERTE(hCurTab != nullptr);
				continue;
			}

			_ASSERTE(hCtrl==nullptr);

			if (hCurTab == hLastTab)
			{
				if (hLastCtrl && GetClassName(hLastCtrl, szClass, countof(szClass)))
				{
					if (lstrcmpi(szClass, L"ListBox") == 0)
					{
						INT_PTR iCount = SendMessage(hLastCtrl, LB_GETCOUNT, 0, 0);
						lLastListFind = SendMessage(hLastCtrl, LB_GETCURSEL, 0, 0);
						if ((iCount > 0) && (lLastListFind + 1) < iCount)
							hCtrl = hLastCtrl;
					}
					else if (lstrcmpi(szClass, L"SysListView32") == 0)
					{
						INT_PTR iCount = ListView_GetItemCount(hLastCtrl);
						lLastListFind = ListView_GetNextItem(hLastCtrl, -1, LVNI_SELECTED);
						if ((iCount > 0) && (lLastListFind + 1) < iCount)
							hCtrl = hLastCtrl;
					}
					else
					{
						ResetLastList();
					}
				}
				else
				{
					ResetLastCtrl();
				}
			}

			// If next item not available in the list
			if (!hCtrl)
			{
				hCtrl = FindWindowEx(hCurTab, hLastCtrl, nullptr, nullptr);
			}

			if (hCtrl == hLastCtrl)
				wcscpy_c(szText, szLastText);
			else if (!hCtrl || (hCtrl != hLastCtrl))
				lLastListFind = -1;

			while (hCtrl != nullptr)
			{
				if ((GetWindowLong(hCtrl, GWL_STYLE) & WS_VISIBLE)
					&& GetClassName(hCtrl, szClass, countof(szClass)))
				{
					if (lstrcmpi(szClass, L"ListBox") == 0)
					{
						lFind = SendMessage(hCtrl, LB_FINDSTRING, lLastListFind, reinterpret_cast<LPARAM>(pszPart.c_str()));
						// LB_FINDSTRING search from begin of string, but may be "In string"?
						if (lFind < 0)
						{
							INT_PTR iCount = SendMessage(hCtrl, LB_GETCOUNT, 0, 0);
							for (INT_PTR j = lLastListFind + 1; j < iCount; j++)
							{
								INT_PTR iLen = SendMessage(hCtrl, LB_GETTEXTLEN, j, 0);
								if (iLen >= static_cast<INT_PTR>(countof(szText)))
								{
									_ASSERTE(iLen < static_cast<INT_PTR>(countof(szText)));
								}
								else
								{
									SendMessage(hCtrl, LB_GETTEXT, j, reinterpret_cast<LPARAM>(szText));
									if (StrStrI(szText, pszPart) != nullptr)
									{
										lFind = j;
										break;
									}
								}
							}
						}

						if ((lFind >= 0) && (lFind > lLastListFind))
						{
							lLastListFind = lFind;
							SendMessage(hCtrl, LB_SETCURSEL, lFind, 0);
							INT_PTR iLen = SendMessage(hCtrl, LB_GETTEXTLEN, lFind, 0);
							if (iLen >= static_cast<INT_PTR>(countof(szText)))
							{
								_ASSERTE(iLen < static_cast<INT_PTR>(countof(szText)));
								_wcscpyn_c(szText, countof(szText), pszPart, countof(szText));
							}
							else
							{
								SendMessage(hCtrl, LB_GETTEXT, lFind, reinterpret_cast<LPARAM>(szText));
							}
							break; // Found
						}
					} // End of "ListBox" processing
					else if (lstrcmpi(szClass, L"SysListView32") == 0)
					{
						LVITEM lvi{};
						lvi.mask = LVIF_TEXT | LVIF_STATE | LVIF_PARAM;
						lvi.pszText = szText;
						INT_PTR iCount = ListView_GetItemCount(hCtrl);
						lFind = -1;

						for (INT_PTR j = lLastListFind+1; (j < iCount) && (lFind == -1); j++)
						{
							for (lvi.iSubItem = 0; lvi.iSubItem <= 32; lvi.iSubItem++)
							{
								lvi.cchTextMax = countof(szText);
								if (SendMessage(hCtrl, LVM_GETITEMTEXT, j, reinterpret_cast<LPARAM>(&lvi)) > 0)
								{
									if (StrStrI(szText, pszPart) != nullptr)
									{
										lFind = j;
										break;
									}
								}
							}
						}

						if ((lFind >= 0) && (lFind > lLastListFind))
						{
							lLastListFind = lFind;
							ListView_SetItemState(hCtrl, lFind, LVIS_SELECTED, LVIS_SELECTED);
							ListView_SetSelectionMark(hCtrl, lFind);
							ListView_EnsureVisible(hCtrl, lFind, FALSE);
							break; // Found
						}
					} // End of "SysListView32" processing
					else if (((lstrcmpi(szClass, L"Button") == 0)
								|| (lstrcmpi(szClass, L"Static") == 0))
						&& GetWindowText(hCtrl, szText, countof(szText)) && *szText)
					{
						// The control's text may have (&) accelerator confusing the search
						wchar_t* p = wcschr(szText, L'&');
						while (p)
						{
							wmemmove(p, p+1, wcslen(p));
							p = wcschr(p+1, L'&');
						}

						if (StrStrI(szText, pszPart) != nullptr)
							break; // Found
					} // End of "Button" and "Static" processing
					else if ((lstrcmpi(szClass, L"Edit") == 0)
						&& GetWindowText(hCtrl, szText, countof(szText)) && *szText)
					{
						// Process "Edit" fields too (search for user-typed string)
						if (StrStrI(szText, pszPart) != nullptr)
							break; // Found
					} // End of "Edit" processing
				}

				hCtrl = FindWindowEx(hCurTab, hCtrl, nullptr, nullptr);
			}

			if (hCtrl)
				break;
		}

		if (hCtrl)
			break;
	}

	if (hCtrl != nullptr)
	{
		// Активировать нужный таб
		if (hSelTab != hCurTab)
		{
			SelectTreeItem(GetDlgItem(ghOpWnd, tvSetupCategories), gpSetCls->m_Pages[iCurTab].hTI, true);
			//SendMessage(hCurTab, mn_ActivateTabMsg, 1, (LPARAM)&(m_Pages[i]));
			//ShowWindow(hCurTab, SW_SHOW);
			//ShowWindow(hSelTab, SW_HIDE);
		}

		// Показать хинт?
		static wchar_t szHint[2000];

		LONG wID = GetWindowLong(hCtrl, GWL_ID);

		if ((wID == -1) && (lstrcmpi(szClass, L"Static") == 0))
		{
			// If it is STATIC with IDC_STATIC - get next ctrl (edit/combo/so on)
			wID = GetWindowLong(FindWindowEx(hCurTab, hCtrl, nullptr, nullptr), GWL_ID);
		}

		szHint[0] = 0;

		if (wID != -1)
		{
			// Is there hint for this control?
			if (!CLngRc::getHint(wID, szHint, countof(szHint)))
				szHint[0] = 0;
		}

		if (!*szHint)
		{
			// Show text of original control.
			// szText contains text from previous label ("Static")
			wcscpy_c(szHint, szText);
		}

		wcscpy_c(szLastText, szText);

		HWND hBall = gpSetCls->hwndBalloon;
		TOOLINFO *pti = &gpSetCls->tiBalloon;

		pti->lpszText = szHint;

		if (gpSetCls->hwndTip) SendMessage(gpSetCls->hwndTip, TTM_ACTIVATE, FALSE, 0);

		SendMessage(hBall, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(pti));
		RECT rcControl; GetWindowRect(hCtrl, &rcControl);
		int ptx = /*bLeftAligh ?*/ (rcControl.left + 10) /*: (rcControl.right - 10)*/;
		int pty = rcControl.top + 10; //bLeftAligh ? rcControl.bottom : (rcControl.top + rcControl.bottom) / 2;
		if ((lstrcmpi(szClass, L"ListBox") == 0) && (lFind >= 0))
		{
			RECT rcItem = {};
			if (SendMessage(hCtrl, LB_GETITEMRECT, lFind, reinterpret_cast<LPARAM>(&rcItem)) != LB_ERR)
			{
				pty += rcItem.top;
			}
		}
		else if ((lstrcmpi(szClass, L"SysListView32") == 0) && (lFind >= 0))
		{
			RECT rcList = {};
			if (ListView_GetItemRect(hCtrl, lFind, &rcList, LVIR_LABEL))
			{
				pty += rcList.top;
			}
		}
		SendMessage(hBall, TTM_TRACKPOSITION, 0, MAKELONG(ptx,pty));
		SendMessage(hBall, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(pti));
		SetTimer(hCurTab, BALLOON_MSG_TIMERID, CONTROL_FOUND_TIMEOUT, nullptr);
	}

wrap:
	// Запомнить
	hLastTab = hCurTab;
	hLastCtrl = hCtrl;

	SetCursor(LoadCursor(nullptr,IDC_ARROW));
}

LRESULT CSettings::OnInitDialog()
{
	//_ASSERTE(!hMain && !hColors && !hCmdTasks && !hViews && !hExt && !hFar && !hInfo && !hDebug && !hUpdate && !hSelection);
	//hMain = hExt = hFar = hTabs = hKeys = hViews = hColors = hCmdTasks = hInfo = hDebug = hUpdate = hSelection = nullptr;
	_ASSERTE(m_Pages && (m_Pages[0].PageIndex==thi_General) && !m_Pages[0].hPage /*...*/);
	ClearPages();

	CSetDlgColors::ReleaseHandles();

	_ASSERTE(mp_ImgBtn==nullptr);
	SafeDelete(mp_ImgBtn);
	mp_ImgBtn = new CImgButtons(ghOpWnd, tOptionSearch, bSaveSettings);
	mp_ImgBtn->AddDonateButtons();

	if (mp_DpiAware)
	{
		mp_DpiAware->Attach(ghOpWnd, ghWnd, mp_Dialog);
	}

	EditIconHint_Set(ghOpWnd, GetDlgItem(ghOpWnd, tOptionSearch), true,
		CLngRc::getRsrc(lng_SearchCtrlF/*"Search (Ctrl+F)"*/),
		false, UM_SEARCH, bSaveSettings);
	EditIconHint_Subclass(ghOpWnd);

	RECT rcEdt = {}, rcBtn = {};
	if (GetWindowRect(GetDlgItem(ghOpWnd, tOptionSearch), &rcEdt))
	{
		MapWindowPoints(nullptr, ghOpWnd, reinterpret_cast<LPPOINT>(&rcEdt), 2);

		// Hate non-strict alignment...
		WORD nCtrls[] = {cbExportConfig};
		for (unsigned short nCtrl : nCtrls)
		{
			// ReSharper disable once CppLocalVariableMayBeConst
			HWND hBtn = GetDlgItem(ghOpWnd, nCtrl);
			GetWindowRect(hBtn, &rcBtn);
			MapWindowPoints(nullptr, ghOpWnd, reinterpret_cast<LPPOINT>(&rcBtn), 2);
			SetWindowPos(hBtn, nullptr, rcBtn.left, rcEdt.top-1, rcBtn.right-rcBtn.left, rcBtn.bottom-rcBtn.top, SWP_NOZORDER);
		}

		#if 0
		RECT rcMargins = {0, rcBtn.bottom+2};
		if (gpConEmu->ExtendWindowFrame(ghOpWnd, rcMargins))
		{
			//HDC hDc = GetDC(ghOpWnd);
			//RECT rcClient; GetClientRect(ghOpWnd, &rcClient);
			//rcClient.bottom = rcMargins.top;
			//FillRect(hDc, &rcClient, (HBRUSH)GetStockObject(BLACK_BRUSH));
			//ReleaseDC(ghOpWnd, hDc);
		}
		#endif
	}

	RegisterTipsFor(ghOpWnd);

	// ReSharper disable once CppLocalVariableMayBeConst
	HMENU hSysMenu = GetSystemMenu(ghOpWnd, FALSE);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_SEPARATOR, 0, nullptr);
	InsertMenu(hSysMenu, 0, MF_BYPOSITION | MF_STRING | MF_ENABLED
	           | ((GetWindowLong(ghOpWnd,GWL_EXSTYLE)&WS_EX_TOPMOST) ? MF_CHECKED : 0),  // NOLINT(misc-redundant-expression)
	           ID_ALWAYSONTOP, _T("Al&ways on top..."));
	RegisterTabs();
	mn_LastChangingFontCtrlId = 0;

	//wchar_t szType[8];
	const SettingsStorage Storage = gpSet->GetSettingsType();
	if (Storage.ReadOnly || isResetBasicSettings)
	{
		EnableWindow(GetDlgItem(ghOpWnd, bSaveSettings), FALSE); // Сохранение запрещено
		if (isResetBasicSettings)
		{
			SetDlgItemText(ghOpWnd, bSaveSettings, CLngRc::getRsrc(lng_BasicSettings/*"Настройки по умолчанию"*/));
		}
	}

	if (isResetBasicSettings)
	{
		const CEStr lsBracketed(L"<", CLngRc::getRsrc(lng_BasicSettings/*"Настройки по умолчанию"*/), L">");
		SetDlgItemText(ghOpWnd, tStorage, lsBracketed);
	}
	else if (Storage.Type == StorageType::REG)
	{
		const CEStr szStorage(L"HKEY_CURRENT_USER\\", ConfigPath);
		SetDlgItemText(ghOpWnd, tStorage, szStorage);
	}
	else
	{
		SetDlgItemText(ghOpWnd, tStorage, Storage.File);
	}

	CEStr lsCfgAdd;
	if (ConfigName[0])
		lsCfgAdd = CEStr(L" (", ConfigName, L")");
	const wchar_t* psStage = (ConEmuVersionStage == CEVS_STABLE) ? L"{Stable}"
					: (ConEmuVersionStage == CEVS_PREVIEW) ? L"{Preview}"
					: L"{Alpha}";
	const CEStr lsDlgTitle(
		CLngRc::getRsrc(lng_DlgSettings/*"Settings"*/),
		lsCfgAdd,
		L" ",
		Storage.getTypeName(),
		L" ",
		gpConEmu->GetDefaultTitle(),
		L" ",
		psStage,
		nullptr);

	SetWindowText(ghOpWnd, lsDlgTitle);

	MCHKHEAP
	{
		mb_IgnoreSelPage = true;
		TVINSERTSTRUCT ti{};
		ti.hParent = TVI_ROOT;
		ti.hInsertAfter = TVI_LAST;
		ti.itemex.mask = TVIF_TEXT;
		// ReSharper disable once CppLocalVariableMayBeConst
		HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
		HTREEITEM hLastRoot = TVI_ROOT;
		bool bNeedExpand = true;
		for (size_t i = 0; m_Pages[i].DialogID; i++)
		{
			if ((m_Pages[i].DialogID == IDD_SPG_UPDATE) && !gpConEmu->isUpdateAllowed())
			{
				m_Pages[i].hTI = nullptr;
				continue;
			}

			ti.hParent = m_Pages[i].Level ? hLastRoot : TVI_ROOT;
			ti.item.pszText = m_Pages[i].PageName;

			m_Pages[i].hTI = TreeView_InsertItem(hTree, &ti);

			_ASSERTE(m_Pages[i].hPage==nullptr);
			m_Pages[i].hPage = nullptr;

			if (m_Pages[i].Level == 0)
			{
				hLastRoot = m_Pages[i].hTI;
				bNeedExpand = !m_Pages[i].Collapsed;
			}
			else
			{
				if (bNeedExpand)
				{
					TreeView_Expand(hTree, hLastRoot, TVE_EXPAND);
				}
				// Only two levels in tree
				bNeedExpand = false;
			}
		}

		TreeView_SelectItem(GetDlgItem(ghOpWnd, tvSetupCategories), gpSetCls->m_Pages[0].hTI);

		// ReSharper disable once CppLocalVariableMayBeConst
		HWND hPlace = GetDlgItem(ghOpWnd, tSetupPagePlace);
		apiShowWindow(hPlace, SW_HIDE);

		mb_IgnoreSelPage = false;

		CSetPgBase::CreatePage(&(m_Pages[0]), ghOpWnd, mn_ActivateTabMsg, mp_DpiAware);

		const CEStr lsUrl(CEWIKIBASE, m_Pages[0].wikiPage);
		SetDlgItemText(ghOpWnd, stSetPgWikiLink, lsUrl);

		apiShowWindow(m_Pages[0].hPage, SW_SHOW);
		m_LastActivePageId = gpSetCls->m_Pages[0].PageIndex;
	}
	MCHKHEAP
	{
		CDpiAware::CenterDialog(ghOpWnd);
	}
	return 0;
}














void CSettings::CheckSelectionModifiers(HWND hWnd2)
{
	struct {
		WORD nCtrlID = 0;
		BYTE nMouseBtn = 0;
		LPCWSTR Descr = nullptr;
		TabHwndIndex nDlgID = TabHwndIndex::thi_General;
		bool bEnabled = false;
		int nVkIdx = 0;
		bool bIgnoreInFar = false;
		// Service
		BYTE Vk = 0;
		HWND hPage = nullptr;
	} Keys[] = {
		{lbCTSBlockSelection, VK_LBUTTON, L"Block selection", thi_MarkCopy, gpSet->isCTSSelectBlock, vkCTSVkBlock},
		{lbCTSTextSelection, VK_LBUTTON, L"Text selection", thi_MarkCopy, gpSet->isCTSSelectText, vkCTSVkText},
		{lbCTSClickPromptPosition, VK_LBUTTON, L"Prompt position", thi_Mouse, gpSet->AppStd.isCTSClickPromptPosition!=0, vkCTSVkPromptClk, true},
		{lbCTSActAlways, VK_MBUTTON, L"Mouse button actions", thi_Mouse, true/*gpSet->isCTSActMode!=2*/, vkCTSVkAct},
		{lbCTSActAlways, VK_RBUTTON, L"Mouse button actions", thi_Mouse, true/*gpSet->isCTSActMode!=2*/, vkCTSVkAct},

		// Don't check it?
		// -- {lbFarGotoEditorVk, L"Highlight and goto", ..., gpSet->isFarGotoEditor},

		// Far manager only
		{lbLDragKey, VK_LBUTTON, L"Far Manager LDrag", thi_Far, (gpSet->isDragEnabled & DRAG_L_ALLOWED)!=0, vkLDragKey},
		{0},
	};

	const bool bIsFar = CVConGroup::isFar(true);

	for (size_t i = 0; Keys[i].nCtrlID; i++)
	{
		Keys[i].hPage = GetPage(Keys[i].nDlgID);

		if (!bIsFar && (Keys[i].nCtrlID == lbLDragKey))
		{
			Keys[i].bEnabled = false;
			continue;
		}

		//GetListBoxByte(hDlg, nCtrlID[i], SettingsNS::szKeysAct, SettingsNS::nKeysAct, Vk[i]);
		DWORD VkMod = gpSet->GetHotkeyById(Keys[i].nVkIdx);
		_ASSERTE((VkMod & 0xFF) == VkMod); // One modifier only?
		Keys[i].Vk = static_cast<BYTE>(VkMod & 0xFF);
	}

	for (size_t i = 0; Keys[i+1].nCtrlID; i++)
	{
		if (!Keys[i].bEnabled)
			continue;

		for (size_t j = i+1; Keys[j].nCtrlID; j++)
		{
			if (!Keys[j].bEnabled)
				continue;

			if (((Keys[i].nDlgID == thi_Far) && Keys[j].bIgnoreInFar)
				|| ((Keys[j].nDlgID == thi_Far) && Keys[i].bIgnoreInFar))
				continue;

			if ((Keys[i].Vk == Keys[j].Vk)
				&& (Keys[i].nMouseBtn == Keys[j].nMouseBtn)
				&& ((Keys[i].hPage == hWnd2) || (Keys[j].hPage == hWnd2))
				)
			{
				wchar_t szMod[32] = L"";
				ConEmuChord::GetVkKeyName(Keys[i].Vk, szMod, true);
				CEStr szInfo;
				ConEmuHotKey::CreateNotUniqueWarning(szMod, Keys[i].Descr, Keys[j].Descr, szInfo);
				// ReSharper disable once CppLocalVariableMayBeConst
				HWND hDlg = hWnd2;
				const WORD nID = (Keys[j].hPage == hWnd2) ? Keys[j].nCtrlID : Keys[i].nCtrlID;
				ShowModifierErrorTip(szInfo, hDlg, nID);
				return;
			}
		}
	}

	// At last, compare with explicitly set hotkeys
	for (size_t i = 0; Keys[i+1].nCtrlID; i++)
	{
		if (!Keys[i].bEnabled)
			continue;
		//if (!Keys[i].Vk) // Check or skip?
		//	continue;
		ConEmuHotKey check;
		check.SetVkMod(ConEmuChord::MakeHotKey(Keys[i].nMouseBtn, Keys[i].Vk));
		const ConEmuHotKey* pFound = gpHotKeys->FindHotKey(check.Key, nullptr);
		if (pFound)
		{
			wchar_t szMod[32] = L"", szDescr2[512];
			ConEmuChord::GetVkKeyName(Keys[i].Vk, szMod, true);
			pFound->GetDescription(szDescr2, countof(szDescr2));
			CEStr szFailMsg;
			ConEmuHotKey::CreateNotUniqueWarning(szMod, Keys[i].Descr, szDescr2, szFailMsg);
			// ReSharper disable once CppLocalVariableMayBeConst
			HWND hDlg = hWnd2;
			const WORD nID = Keys[i].nCtrlID;
			ShowModifierErrorTip(szFailMsg, hDlg, nID);
			return;
		}
	}
}

void CSettings::UpdateTextColorSettings(BOOL ChangeTextAttr /*= TRUE*/, BOOL ChangePopupAttr /*= TRUE*/, const AppSettings* apDistinct /*= nullptr*/)
{
	// Обновить палитры
	gpSet->PaletteSetStdIndexes();

	// Обновить консоли
	if (ghWnd)
	{
		CVConGroup::OnUpdateTextColorSettings(ChangeTextAttr, ChangePopupAttr, apDistinct);
	}
}

// This is used if user choose palette from dropdown in the Settings dialog
// OR when GuiMacro Palette was called.
void CSettings::ChangeCurrentPalette(const ColorPalette* pPal, bool bChangeDropDown)
{
	if (!pPal)
	{
		_ASSERTE(pPal!=nullptr);
		return;
	}

	if (!isMainThread())
	{
		gpConEmu->PostChangeCurPalette(pPal->pszName, bChangeDropDown, false);
		return;
	}

	if (gpSet->isLogging())
	{
		const CEStr lsLog(L"Color Palette: `", pPal->pszName, L"` ChangeDropDown=", bChangeDropDown ? L"yes" : L"no");
		LogString(lsLog);
	}

	// ReSharper disable once CppLocalVariableMayBeConst
	HWND hDlg = GetPage(thi_Colors);

	if (bChangeDropDown && hDlg)
	{
		CSetDlgLists::SelectStringExact(hDlg, lbDefaultColors, pPal->pszName);
	}

	// ReSharper disable once CppVariableCanBeMadeConstexpr
	const size_t nCount = pPal->Colors.size();

	for (size_t i = 0; i < nCount; i++)
	{
		gpSet->Colors[i] = pPal->Colors[i]; //-V108
	}

	const BOOL bTextChanged = (gpSet->AppStd.nTextColorIdx != pPal->nTextColorIdx) || (gpSet->AppStd.nBackColorIdx != pPal->nBackColorIdx);
	const BOOL bPopupChanged = (gpSet->AppStd.nPopTextColorIdx != pPal->nPopTextColorIdx) || (gpSet->AppStd.nPopBackColorIdx != pPal->nPopBackColorIdx);

	// We need to change consoles contents if TEXT attributes was changed
	if (bTextChanged || bPopupChanged)
	{
		wchar_t szLog[128];
		swprintf_c(szLog,
			L"Color Palette: Text {%u|%u}->{%u|%u} Popup {%u|%u}->{%u|%u}",
			gpSet->AppStd.nTextColorIdx, gpSet->AppStd.nBackColorIdx, pPal->nTextColorIdx, pPal->nBackColorIdx,
			gpSet->AppStd.nPopTextColorIdx, gpSet->AppStd.nPopBackColorIdx, pPal->nPopTextColorIdx, pPal->nPopBackColorIdx);
		LogString(szLog);

		gpSet->AppStd.nTextColorIdx = pPal->nTextColorIdx;
		gpSet->AppStd.nBackColorIdx = pPal->nBackColorIdx;
		gpSet->AppStd.nPopTextColorIdx = pPal->nPopTextColorIdx;
		gpSet->AppStd.nPopBackColorIdx = pPal->nPopBackColorIdx;

		UpdateTextColorSettings(bTextChanged, bPopupChanged);

		LogString(L"Color Palette: UpdateTextColorSettings finished");
	}

	LogString(L"Color Palette: Refreshing");

	if (hDlg && (hDlg == GetActivePage()))
	{
		CSetPgColors* pColorsPg;
		if (GetPageObj(pColorsPg))
			pColorsPg->OnInitDialog(hDlg, false);
	}

	if (ghWnd)
	{
		gpConEmu->InvalidateAll();
		gpConEmu->Update(true);
	}

	LogString(L"Color Palette: Finished");
}

// TODO: Move to CSetPgBase!!!
LRESULT CSettings::OnListBoxDblClk(HWND hWnd2, WPARAM wParam, LPARAM lParam)
{
	WORD wId = LOWORD(wParam);

	switch (wId)
	{
	case lbStatusAvailable:
		OnButtonClicked(hWnd2, cbStatusAddSelected, 0);
		break;
	case lbStatusSelected:
		OnButtonClicked(hWnd2, cbStatusDelSelected, 0);
		break;
	}

	return 0;
}

void CSettings::SelectTreeItem(HWND hTree, HTREEITEM hItem, bool bPost /*= false*/)
{
	HTREEITEM hParent = TreeView_GetParent(hTree, hItem);
	if (hParent)
	{
		TreeView_Expand(hTree, hParent, TVE_EXPAND);
	}

	if (!bPost)
		TreeView_SelectItem(hTree, hItem);
	else
		PostMessage(hTree, TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(hItem));
}

LRESULT CSettings::OnNextPage()
{
	HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
	NMTREEVIEW nm = { {hTree, tvSetupCategories, TVN_SELCHANGED} };
	nm.itemOld.hItem = TreeView_GetSelection(hTree);
	if (!nm.itemOld.hItem)
		nm.itemOld.hItem = TreeView_GetRoot(hTree);
	nm.itemNew.hItem = TreeView_GetNextSibling(hTree, nm.itemOld.hItem);
	if (!nm.itemNew.hItem)
		nm.itemNew.hItem = TreeView_GetRoot(hTree);
	//return OnPage((LPNMHDR)&nm);
	if (nm.itemNew.hItem)
	{
		SelectTreeItem(hTree, nm.itemNew.hItem);
	}
	return 0;
}

LRESULT CSettings::OnPrevPage()
{
	HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
	NMTREEVIEW nm = { {hTree, tvSetupCategories, TVN_SELCHANGED} };
	nm.itemOld.hItem = TreeView_GetSelection(hTree);
	if (!nm.itemOld.hItem)
		nm.itemOld.hItem = TreeView_GetLastVisible(hTree);
	nm.itemNew.hItem = TreeView_GetPrevSibling(hTree, nm.itemOld.hItem);
	if (!nm.itemNew.hItem)
		nm.itemNew.hItem = TreeView_GetRoot(hTree);
	//return OnPage((LPNMHDR)&nm);
	if (nm.itemNew.hItem)
	{
		SelectTreeItem(hTree, nm.itemNew.hItem);
	}
	return 0;
}

LRESULT CSettings::OnPage(LPNMHDR phdr)
{
	if (ms_BalloonErrTip)
	{
		ms_BalloonErrTip.Release();
		SendMessage(hwndBalloon, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&tiBalloon));
		SendMessage(hwndTip, TTM_ACTIVATE, TRUE, 0);
	}

	if (reinterpret_cast<LONG_PTR>(phdr) == HOTKEY_ID_TAB_NEXT)
	{
		_ASSERTE(FALSE && "Should not get here");
		return OnNextPage();
	}
	else if (reinterpret_cast<LONG_PTR>(phdr) == HOTKEY_ID_TAB_PREV)
	{
		_ASSERTE(FALSE && "Should not get here");
	}


	switch (phdr->code)
	{
		case TVN_SELCHANGED:
		{
			LPNMTREEVIEW p = (LPNMTREEVIEW)phdr;
			HWND hTree = GetDlgItem(ghOpWnd, tvSetupCategories);
			if (p->itemOld.hItem && p->itemOld.hItem != p->itemNew.hItem)
				TreeView_SetItemState(hTree, p->itemOld.hItem, 0, TVIS_BOLD);
			if (p->itemNew.hItem)
				TreeView_SetItemState(hTree, p->itemNew.hItem, TVIS_BOLD, TVIS_BOLD);

			if (mb_IgnoreSelPage)
				return 0;
			HWND hCurrent = nullptr;
			for (size_t i = 0; m_Pages[i].DialogID; i++)
			{
				if (p->itemNew.hItem == m_Pages[i].hTI)
				{
					if (m_Pages[i].hPage == nullptr)
					{
						SetCursor(LoadCursor(nullptr,IDC_WAIT));
						CSetPgBase::CreatePage(&(m_Pages[i]), ghOpWnd, mn_ActivateTabMsg, mp_DpiAware);
					}
					else
					{
						SendMessage(m_Pages[i].hPage, mn_ActivateTabMsg, 1, (LPARAM)&(m_Pages[i]));
						m_Pages[i].pPage->ProcessDpiChange(gpSetCls->mp_DpiAware);
					}

					CEStr lsUrl(CEWIKIBASE, m_Pages[i].wikiPage);
					SetDlgItemText(ghOpWnd, stSetPgWikiLink, lsUrl);

					apiShowWindow(m_Pages[i].hPage, SW_SHOW);
					m_LastActivePageId = gpSetCls->m_Pages[i].PageIndex;
				}
				else if (p->itemOld.hItem == m_Pages[i].hTI)
				{
					hCurrent = m_Pages[i].hPage;
				}
			}
			if (hCurrent)
				apiShowWindow(hCurrent, SW_HIDE);
		} // TVN_SELCHANGED
		break;
	}

	return 0;
}

/// IdShowPage is DialogID (IDD_SPG_FONTS, etc.)
void CSettings::Dialog(int IdShowPage /*= 0*/)
{
	const TabHwndIndex lastPageId = gpSetCls->m_LastActivePageId;
	TabHwndIndex showPage = thi_Last;

	if (!ghOpWnd || !IsWindow(ghOpWnd))
	{
		_ASSERTE(isMainThread());

		SetCursor(LoadCursor(nullptr,IDC_WAIT));

		// Сначала обновить DC, чтобы некрасивостей не было
		gpConEmu->UpdateWindowChild(nullptr);

		CDpiForDialog::Create(gpSetCls->mp_DpiAware);

		wchar_t szLog[80]; swprintf_c(szLog, L"Creating settings dialog, IdPage=%u", IdShowPage);
		LogString(szLog);

		gpSetCls->InitPageNames();

		SafeDelete(gpSetCls->mp_Dialog);
		//2009-05-03. Нам нужен НЕмодальный диалог
		gpSetCls->mp_Dialog = CDynDialog::ShowDialog(IDD_SETTINGS, nullptr, wndOpProc, 0/*dwInitParam*/);

		if (!gpSetCls->mp_Dialog)
		{
			DisplayLastError(L"Can't create settings dialog!");
			goto wrap;
		}
		else
		{
			_ASSERTE(ghOpWnd == gpSetCls->mp_Dialog->mh_Dlg);
			ghOpWnd = gpSetCls->mp_Dialog->mh_Dlg;
		}
	}

	apiShowWindow(ghOpWnd, SW_SHOWNORMAL);

	if (IdShowPage)
		showPage = gpSetCls->GetPageIdByDialogId(IdShowPage);
	if ((showPage == thi_Last) && (lastPageId != thi_Last))
		showPage = lastPageId;

	if (showPage != thi_Last)
		gpSetCls->ActivatePage(showPage);

	SetFocus(ghOpWnd);
wrap:
	return;
}

void CSettings::OnSettingsClosed()
{
	if (!ghOpWnd)
		return;

	bool bWasDebugLogging = (CSetPgDebug::GetActivityLoggingType() != glt_None);

	gpConEmu->OnPanelViewSettingsChanged();

	gpConEmu->GetGlobalHotkeys().RegisterMinRestore(gpSet->IsHotkey(vkMinimizeRestore) || gpSet->IsHotkey(vkMinimizeRestor2));

	if (gpSet->m_isKeyboardHooks == 1)
		gpConEmu->GetGlobalHotkeys().RegisterHooks();
	else if (gpSet->m_isKeyboardHooks == 2)
		gpConEmu->GetGlobalHotkeys().UnRegisterHooks();

	UnregisterTabs();

	CSetDlgColors::ReleaseHandles();

	if (hwndTip)
	{
		DestroyWindow(hwndTip);
		hwndTip = nullptr;
	}

	if (hwndBalloon)
	{
		DestroyWindow(hwndBalloon);
		hwndBalloon = nullptr;
	}

	// mp_DpiAware and others are cleared in ClearPages()
	ClearPages();

	if (bWasDebugLogging)
	{
		_ASSERTE(gpSetCls->GetPageObj(thi_Debug) == nullptr);
		gpConEmu->OnGlobalSettingsChanged();
	}

	gpConEmu->OnOurDialogClosed();

	ghOpWnd = nullptr;
}

void CSettings::OnResetOrReload(bool abResetOnly, SettingsStorage* pXmlStorage /*= nullptr*/)
{
	bool lbWasPos = false;
	RECT rcWnd = {};

	bool bImportOnly = !abResetOnly && pXmlStorage;

	CEStr szLabel, szMessage, szOkDescr;
	if (bImportOnly)
	{
		szLabel = L"Confirm import settings from file";
		szMessage = pXmlStorage->File ? pXmlStorage->File : L"???";
		szOkDescr = L"Import settings via merging";
	}
	else if (abResetOnly)
	{
		szLabel = L"Confirm reset settings to defaults";
		szMessage = L"Warning!!!\nAll your current settings will be lost!";
		szOkDescr = L"Reset settings to defaults!";
	}
	else
	{
		szLabel = CEStr(L"Confirm reload settings from ", SettingsStorage::getTypeName(gpSetCls->Type));
		szMessage = L"Warning!!!\nAll your current settings will be lost!";
		szOkDescr = CEStr(L"Reset to defaults and import settings from ", SettingsStorage::getTypeName(gpSetCls->Type));
	}

	int nBtn = ConfirmDialog(szMessage, szLabel, gpConEmu->GetDefaultTitle(),
		nullptr /* URL */, MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2, ghOpWnd,
		L"Confirm", szOkDescr,
		L"Cancel", nullptr);
	// int nBtn = MsgBox(pszMsg, MB_YESNO|MB_ICONEXCLAMATION|MB_DEFBUTTON2, gpConEmu->GetDefaultTitle(), ghOpWnd);
	if (nBtn != IDYES)
		return;

	SetCursor(LoadCursor(nullptr,IDC_WAIT));
	gpConEmu->Taskbar_SetProgressState(TBPF_INDETERMINATE);

	if (ghOpWnd && IsWindow(ghOpWnd))
	{
		lbWasPos = true;
		GetWindowRect(ghOpWnd, &rcWnd);
		DestroyWindow(ghOpWnd);
	}
	_ASSERTE(ghOpWnd == nullptr);

	if (!bImportOnly)
	{
		// Сброс настроек на умолчания
		gpSet->InitSettings();

		// Почистить макросы и сбросить на умолчания
		CSetPgKeys::ReInitHotkeys();
	}

	if (bImportOnly)
	{
		bool bNeedCreateVanilla = false;
		gpSet->LoadSettings(bNeedCreateVanilla, pXmlStorage);
	}
	else if (!abResetOnly && !gpConEmu->IsResetBasicSettings())
	{
		// Reload from xml/reg
		bool bNeedCreateVanilla = false;
		gpSet->LoadSettings(bNeedCreateVanilla);
	}
	else
	{
		// Reset to defaults
		gpSetCls->IsConfigNew = true; // otherwise some options may be modified, as for "new config"
		gpSet->InitVanilla();
	}


	SettingsLoadedFlags slfFlags = slf_OnResetReload
		| ((abResetOnly && !bImportOnly) ? (slf_DefaultSettings/*|slf_AllowFastConfig*/) : slf_None);

	SettingsLoaded(slfFlags, nullptr);

	if (lbWasPos && !ghOpWnd)
	{
		Dialog();
		if (ghOpWnd)
			SetWindowPos(ghOpWnd, nullptr, rcWnd.left, rcWnd.top, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
	}

	SetCursor(LoadCursor(nullptr,IDC_ARROW));
	gpConEmu->Taskbar_SetProgressState(TBPF_NOPROGRESS);

	MsgBox(L"Don't forget to Save your new settings", MB_ICONINFORMATION, gpConEmu->GetDefaultTitle(), ghOpWnd);
}

void CSettings::ExportSettings()
{
	const CEStr pszDefPath(gpConEmu->ConEmuXml());
	wchar_t *pszSlash = pszDefPath ? wcsrchr(pszDefPath.data(), L'\\') : nullptr;
	if (pszSlash)
		*(pszSlash+1) = 0;

	const CEStr pszFile = SelectFile(L"Export settings", L"*.xml", pszDefPath, ghOpWnd, L"XML files (*.xml)\0*.xml\0", sff_SaveNewFile);
	if (pszFile)
	{
		SetCursor(LoadCursor(nullptr,IDC_WAIT));
		gpConEmu->Taskbar_SetProgressState(TBPF_INDETERMINATE);

		// Export using ".Vanilla" configuration!
		wchar_t* pszSaveName = nullptr;
		if (ConfigName[0])
		{
			pszSaveName = lstrdup(ConfigName).Detach();
			SetConfigName(L"");
		}

		SettingsStorage XmlStorage = {StorageType::XML};
		XmlStorage.File = pszFile;
		const bool bOld = isResetBasicSettings; isResetBasicSettings = false;
		gpSet->SaveSettings(FALSE, &XmlStorage);
		isResetBasicSettings = bOld;

		// Restore configuration name if any
		if (pszSaveName)
		{
			SetConfigName(pszSaveName);
			SafeFree(pszSaveName);
		}

		SetCursor(LoadCursor(nullptr,IDC_ARROW));
		gpConEmu->Taskbar_SetProgressState(TBPF_NOPROGRESS);
	}
}

void CSettings::ImportSettings()
{
	// #SETTINGS Support INI files?
	const CEStr pszFile = SelectFile(L"Import settings", L"*.xml", nullptr, ghOpWnd, L"XML files (*.xml)\0*.xml\0", sff_Default);
	if (pszFile)
	{
		// Import using ".Vanilla" configuration!
		CEStr pszSaveName;
		if (ConfigName[0])
		{
			pszSaveName = lstrdup(ConfigName);
			SetConfigName(L"");
		}

		SettingsStorage XmlStorage = {StorageType::XML};
		XmlStorage.File = pszFile;
		XmlStorage.ReadOnly = true;

		OnResetOrReload(false, &XmlStorage);

		// Restore configuration name if any
		if (pszSaveName)
		{
			SetConfigName(pszSaveName);
		}
	}
}

INT_PTR CSettings::ProcessTipHelp(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	LPNMTTDISPINFO lpnmtdi = reinterpret_cast<LPNMTTDISPINFOW>(lParam);

	if (!lpnmtdi)
	{
		_ASSERTE(lpnmtdi);
		return 0;
	}

	// If your message handler sets the uFlags field of the NMTTDISPINFO structure to TTF_DI_SETITEM,
	// the ToolTip control stores the information and will not request it again.
	static wchar_t szHint[2000];

	_ASSERTE((lpnmtdi->uFlags & TTF_IDISHWND) == TTF_IDISHWND);

	{
		if (gpSet->isShowHelpTooltips)
		{
			CEHelpPopup::GetItemHelp(0, reinterpret_cast<HWND>(lpnmtdi->hdr.idFrom), szHint, countof(szHint));

			lpnmtdi->lpszText = szHint;
		}
		else
		{
			szHint[0] = 0;
		}
	}

	lpnmtdi->szText[0] = 0;

	return 0;
}

static COLORREF SettingsBackground(bool field)
{
	return field ? DarkMode::Control : DarkMode::Background;
}

void CSettings::ApplySettingsTheme(HWND dialog)
{
	gpConEmu->ApplyPopupMenuTheme(dialog);
	DarkMode::ApplyDialog(dialog);
}

INT_PTR CSettings::SettingsThemeBrush(UINT message, HDC dc, HWND control, bool hyperlink)
{
	if (!CDwmHelper::IsWindowsDarkMode())
		return 0;

	bool field = (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX);
	if (message == WM_CTLCOLORSTATIC && control)
	{
		wchar_t cls[16] = {};
		GetClassNameW(control, cls, countof(cls));
		field = _wcsicmp(cls, L"Edit") == 0;
	}
	const auto bg = SettingsBackground(field);
	SetTextColor(dc, hyperlink ? RGB(134, 193, 255)
		: control && !IsWindowEnabled(control) ? RGB(157, 157, 157) : RGB(240, 240, 240));
	SetBkColor(dc, bg);
	SetBkMode(dc, field ? OPAQUE : TRANSPARENT);
	SetDCBrushColor(dc, bg);
	return reinterpret_cast<INT_PTR>(GetStockObject(DC_BRUSH));
}

// DlgProc для окна настроек (IDD_SETTINGS)
INT_PTR CSettings::wndOpProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	_ASSERTE(ghOpWnd == hWnd2 || ghOpWnd == nullptr);

	INT_PTR lRc = 0;
	if ((gpSetCls->mp_ImgBtn && gpSetCls->mp_ImgBtn->Process(hWnd2, messg, wParam, lParam, lRc))
		|| EditIconHint_Process(hWnd2, messg, wParam, lParam, lRc))
	{
		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, lRc);
		return TRUE;
	}

	PatchMsgBoxIcon(hWnd2, messg, wParam, lParam);
	if (DarkMode::ProcessDialogMessage(hWnd2, messg, wParam, lParam, lRc)) return lRc;

	switch (messg)
	{
		case WM_INITDIALOG:
			gpConEmu->OnOurDialogOpened();
			ghOpWnd = hWnd2;
			SendMessage(hWnd2, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hClassIcon));
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hClassIconSm));

			#ifdef _DEBUG
			//if (IsDebuggerPresent())
			if (!gpSet->isAlwaysOnTop)
				SetWindowPos(ghOpWnd, HWND_NOTOPMOST, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE);
			#endif

			SetClassLongPtr(hWnd2, GCLP_HICON, reinterpret_cast<LONG_PTR>(hClassIcon));
			gpSetCls->OnInitDialog();
			ApplySettingsTheme(hWnd2);
			break;

		case WM_SETTINGCHANGE:
			ApplySettingsTheme(hWnd2);
			break;

		case WM_SYSCOMMAND:

			if (LOWORD(wParam) == ID_ALWAYSONTOP)
			{
				const bool lbOnTopNow = GetWindowLong(ghOpWnd, GWL_EXSTYLE) & WS_EX_TOPMOST;
				SetWindowPos(ghOpWnd, lbOnTopNow ? HWND_NOTOPMOST : HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
				CheckMenuItem(GetSystemMenu(ghOpWnd, FALSE), ID_ALWAYSONTOP, MF_BYCOMMAND |
				              (lbOnTopNow ? MF_UNCHECKED : MF_CHECKED));
				SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, 0);
				return 1;
			}

			break;

		case WM_CTLCOLORSTATIC:
		{
			const auto dc = reinterpret_cast<HDC>(wParam);
			const auto control = reinterpret_cast<HWND>(lParam);
			const auto id = LOWORD(GetDlgCtrlID(control));
			if (const auto brush = gpSetCls->OnCtlColorStatic(hWnd2, dc, control, id))
				return brush;
			return SettingsThemeBrush(messg, dc, control);
		}
		case WM_CTLCOLORDLG:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
			return SettingsThemeBrush(messg, reinterpret_cast<HDC>(wParam), reinterpret_cast<HWND>(lParam));

		case WM_SETCURSOR:
			if (CDlgItemHelper::isHyperlinkCtrl(LOWORD(GetDlgCtrlID(reinterpret_cast<HWND>(wParam)))))
			{
				SetCursor(LoadCursor(nullptr, IDC_HAND));
				SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
			break;

		//case WM_GETICON:

		//	if (wParam==ICON_BIG)
		//	{
		//		#ifdef _DEBUG
		//		ICONINFO inf = {0}; BITMAP bi = {0};
		//		if (GetIconInfo(hClassIcon, &inf))
		//		{
		//			GetObject(inf.hbmColor, sizeof(bi), &bi);
		//			if (inf.hbmMask) DeleteObject(inf.hbmMask);
		//			if (inf.hbmColor) DeleteObject(inf.hbmColor);
		//		}
		//		#endif
		//		SetWindowLongPtr(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
		//		return 1;
		//	}
		//	else
		//	{
		//		#ifdef _DEBUG
		//		ICONINFO inf = {0}; BITMAP bi = {0};
		//		if (GetIconInfo(hClassIconSm, &inf))
		//		{
		//			GetObject(inf.hbmColor, sizeof(bi), &bi);
		//			if (inf.hbmMask) DeleteObject(inf.hbmMask);
		//			if (inf.hbmColor) DeleteObject(inf.hbmColor);
		//		}
		//		#endif
		//		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
		//		return 1;
		//	}

		//	return 0;
		case WM_COMMAND:

			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
				{
					switch (LOWORD(wParam))
					{
					case bSaveSettings:
						{
							if (/*ReadOnly || */gpSetCls->isResetBasicSettings)
							{
								//DisplayLastError(isResetBasicSettings ? L"Not allowed in '/Basic' mode" : L"Settings storage is read only", -1);
								DisplayLastError(L"Not allowed in '/Basic' mode", -1);
								return 0;
							}

							const bool isShiftPressed = isPressed(VK_SHIFT);

							// были изменения в полях размера/положения?
							// ReSharper disable once CppLocalVariableMayBeConst
							HWND hSizePosPg = gpSetCls->GetPage(thi_SizePos);
							if (hSizePosPg
								&& IsWindowEnabled(GetDlgItem(hSizePosPg, cbApplyPos)))
							{
								gpSetCls->OnButtonClicked(hSizePosPg, cbApplyPos, 0);
							}

							if (gpSet->SaveSettings())
							{
								if (!isShiftPressed)
									SendMessage(ghOpWnd,WM_COMMAND,IDOK,0);
							}
						}
						break;

					case bResetSettings:
					case bReloadSettings:
						gpSetCls->OnResetOrReload(LOWORD(wParam) == bResetSettings);
						break;
					case cbExportConfig:
						gpSetCls->ExportSettings();
						break;
					case bImportSettings:
						gpSetCls->ImportSettings();
						break;

					case IDCANCEL:
						{
							// Clear the ‘Filter’ field on the Keys&Macro page, for example
							CSetPgBase* pg = gpSetCls->GetActivePageObj();
							if (pg && !pg->QueryDialogCancel())
								break;
						}  // -V796
					case IDOK:
					case IDCLOSE:
						// -- перенесено в WM_CLOSE
						//if (gpSet->isTabs==1) gpConEmu->ForceShowTabs(TRUE); else
						//if (gpSet->isTabs==0) gpConEmu->ForceShowTabs(FALSE); else
						//	gpConEmu->mp_TabBar->Update();
						//gpConEmu->OnPanelViewSettingsChanged();
						SendMessage(ghOpWnd, WM_CLOSE, 0, 0);
						break;

					default:
						gpSetCls->OnButtonClicked(hWnd2, wParam, lParam);
					}
				}
				break;

				case EN_CHANGE:
				{
					_ASSERTE(LOWORD(wParam)==tStorage && "EN_CHANGE is ignored in the ghOpWnd!");
					//gpSetCls->OnEditChanged(hWnd2, wParam, lParam);
				}
				break;

				case CBN_EDITCHANGE:
				case CBN_SELCHANGE:
				{
					_ASSERTE(HIWORD(wParam)!=CBN_EDITCHANGE && "CBN_EDITCHANGE is NOT expected in ghOpWnd!");
					_ASSERTE(HIWORD(wParam)!=CBN_SELCHANGE && "CBN_SELCHANGE is NOT expected in ghOpWnd!");
					//gpSetCls->OnComboBox(hWnd2, wParam, lParam);
				}
				break;
			} // switch (HIWORD(wParam))
			break; // case WM_COMMAND

		case UM_SEARCH:
			gpSetCls->SearchForControls();
			break;

		case WM_NOTIFY:
		{
			// ReSharper disable once CppLocalVariableMayBeConst
			LPNMHDR phdr = reinterpret_cast<LPNMHDR>(lParam);

			if (phdr->code == TTN_GETDISPINFO)
			{
				return gpSetCls->ProcessTipHelp(hWnd2, messg, wParam, lParam);
			}
			else switch (phdr->idFrom)
			{
			case tvSetupCategories:
				gpSetCls->OnPage(phdr);
				break;
			}
		} break;
		case WM_CLOSE:
		{
			gpSetCls->OnSettingsClosed();
			DestroyWindow(hWnd2);
		} break;
		case WM_DESTROY:
		{
			gpSetCls->OnSettingsClosed();
		} break;
		case WM_HOTKEY:

			if (wParam == HOTKEY_ID_TAB_NEXT)
			{
				gpSetCls->OnNextPage();
			}
			else if (wParam == HOTKEY_ID_TAB_PREV)
			{
				gpSetCls->OnPrevPage();
			}

			break;

		case WM_ACTIVATE:

			if (LOWORD(wParam) != 0)
				gpSetCls->RegisterTabs();
			else
				gpSetCls->UnregisterTabs();

			break;

		case HELP_WM_HELP:
			break;
		case WM_HELP:
			if ((wParam == 0) && (lParam != 0))
			{
				// Open wiki page
				HELPINFO* hi = reinterpret_cast<HELPINFO*>(lParam);
				if (hi->cbSize >= sizeof(HELPINFO))
				{
					CEHelpPopup::OpenSettingsWiki(hWnd2, LOWORD(hi->iCtrlId));
				}
			}
			return TRUE;

		default:
			if (gpSetCls->mp_DpiAware && gpSetCls->mp_DpiAware->ProcessDpiMessages(hWnd2, messg, wParam, lParam))
			{
				// Refresh the visible page and mark 'to be changed' others
				for (ConEmuSetupPages* p = gpSetCls->m_Pages; p->DialogID; p++)
				{
					if (p->pPage)
					{
						p->pPage->OnDpiChanged(gpSetCls->mp_DpiAware);
					}
				}

				return TRUE;
			}
	}

	return 0;
}

INT_PTR CSettings::OnCtlColorStatic(HWND hDlg, HDC hdc, HWND hCtrl, WORD nCtrlId)
{
	if (CDlgItemHelper::isHyperlinkCtrl(nCtrlId))
	{
		_ASSERTE(hCtrl!=nullptr);
		// Check appropriate flags
		const DWORD nStyle = GetWindowLong(hCtrl, GWL_STYLE);
		if (!(nStyle & SS_NOTIFY))
			SetWindowLong(hCtrl, GWL_STYLE, nStyle|SS_NOTIFY);
		// And the colors
		if (const auto brush = SettingsThemeBrush(WM_CTLCOLORSTATIC, hdc, hCtrl, true))
			return brush;
		SetTextColor(hdc, GetSysColor(COLOR_HOTLIGHT));
		SetBkMode(hdc, TRANSPARENT);
		HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
		return reinterpret_cast<INT_PTR>(hBrush);
	}

	return reinterpret_cast<INT_PTR>(DarkMode::ControlColor(hCtrl, hdc, WM_CTLCOLORSTATIC));
}

INT_PTR CSettings::OnMeasureFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	const WORD wID = LOWORD(wParam);

	if (wID == tFontFace || wID == tFontFace2 || wID == tThumbsFontName || wID == tTilesFontName
		|| wID == tTabFontFace || wID == tStatusFontFace)
	{
		MEASUREITEMSTRUCT *pItem = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
		const int nDpi = GetDialogDpi();
		if (nDpi >= 96 && _dpi_all.Ydpi >= 96)
		{
			pItem->itemHeight = 15 * nDpi / 96;
		}
		else
		{
			_ASSERTE(nDpi >= 96 && _dpi_all.Ydpi >= 96);
			pItem->itemHeight = 15;
		}
	}

	return TRUE;
}

INT_PTR CSettings::OnDrawFontItem(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	const WORD wID = LOWORD(wParam);

	if (wID == tFontFace || wID == tFontFace2 || wID == tThumbsFontName || wID == tTilesFontName
		|| wID == tTabFontFace || wID == tStatusFontFace)
	{
		DRAWITEMSTRUCT* pItem = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
		wchar_t szText[128]; szText[0] = 0;
		SendDlgItemMessage(hWnd2, wID, CB_GETLBTEXT, pItem->itemID, reinterpret_cast<LPARAM>(szText));
		const DWORD bAlmostMonospace = static_cast<DWORD>(SendDlgItemMessage(hWnd2, wID, CB_GETITEMDATA, pItem->itemID, 0));
		COLORREF crText, crBack;

		if (!(pItem->itemState & ODS_SELECTED))
		{
			const bool dark = CDwmHelper::IsWindowsDarkMode();
			crText = dark ? RGB(240, 240, 240) : GetSysColor(COLOR_WINDOWTEXT);
			crBack = dark ? SettingsBackground(true) : GetSysColor(COLOR_WINDOW);
		}
		else
		{
			crText = GetSysColor(COLOR_HIGHLIGHTTEXT);
			crBack = GetSysColor(COLOR_HIGHLIGHT);
		}

		SetTextColor(pItem->hDC, crText);
		SetBkColor(pItem->hDC, crBack);
		RECT rc = pItem->rcItem;
		HBRUSH hBr = CreateSolidBrush(crBack);
		FillRect(pItem->hDC, &rc, hBr);
		DeleteObject(hBr);
		rc.left++;
		const int nDpi = GetDialogDpi();
		HFONT hFont = CreateFont(-8 * nDpi / 72, 0, 0, 0, (bAlmostMonospace == 1) ? FW_BOLD : FW_NORMAL, 0, 0, 0,
			ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH,
			L"MS Shell Dlg");
		HFONT hOldF = static_cast<HFONT>(SelectObject(pItem->hDC, hFont));
		DrawText(pItem->hDC, szText, _tcslen(szText), &rc, DT_LEFT | DT_VCENTER | DT_NOPREFIX);
		SelectObject(pItem->hDC, hOldF);
		DeleteObject(hFont);
	}

	return TRUE;
}

void CSettings::UpdateWindowMode(ConEmuWindowMode WndMode)
{
	_ASSERTE(WndMode==wmNormal || WndMode==wmMaximized || WndMode==wmFullScreen);

	if (gpSet->isQuakeStyle)
	{
		return;
	}

	if (gpSet->isUseCurrentSizePos)
	{
		gpSet->_WindowMode = WndMode;
	}

	HWND hSizePosPg = GetPage(thi_SizePos);
	if (hSizePosPg && gpSet->isUseCurrentSizePos)
	{
		checkRadioButton(hSizePosPg, rNormal, rFullScreen,
			(WndMode == wmFullScreen) ? rFullScreen
			: (WndMode == wmMaximized) ? rMaximized
			: rNormal);

	}
}

void CSettings::UpdatePos(int ax, int ay, bool bGetRect)
{
	int x = ax, y = ay;

	if (bGetRect
		&& !gpConEmu->isFullScreen()
		&& !gpConEmu->isZoomed()
		&& !gpConEmu->isIconic()
		&& (gpConEmu->GetTileMode(false) == cwc_Current))
	{
		RECT rc; GetWindowRect(ghWnd, &rc);
		const POINT newPos = gpConEmu->VisualPosFromReal(rc.left, rc.top);
		//_ASSERTE(x==newPos.x && y==newPos.y); // May occur during Reset/Reload from Settings dialog
		x = newPos.x; y = newPos.y;
	}
	else
	{
		const POINT newPos = gpConEmu->VisualPosFromReal(ax, ay);
		x = newPos.x; y = newPos.y;
	}

	if ((gpConEmu->WndPos.x != x) || (gpConEmu->WndPos.y != y))
	{
		if (gpConEmu->WndPos.x != x)
			gpConEmu->WndPos.x = x;
		if (gpConEmu->WndPos.y != y)
			gpConEmu->WndPos.y = y;
	}

	if (gpSet->isUseCurrentSizePos)
	{
		if (gpSet->_wndX != x)
			gpSet->_wndX = x;
		if (gpSet->_wndY != y)
			gpSet->_wndY = y;
	}

	HWND hSizePosPg = GetPage(thi_SizePos);
	if (hSizePosPg)
	{
		MSetter lIgnoreEdit(&CSetPgBase::mb_IgnoreEditChanged);
		SetDlgItemInt(hSizePosPg, tWndX, gpSet->isUseCurrentSizePos ? x : gpSet->_wndX, TRUE);
		SetDlgItemInt(hSizePosPg, tWndY, gpSet->isUseCurrentSizePos ? y : gpSet->_wndY, TRUE);
	}

	wchar_t szLabel[128];
	swprintf_c(szLabel, L"UpdatePos A={%i,%i} C={%i,%i} S={%i,%i}", ax,ay, x, y, gpSet->_wndX, gpSet->_wndY);
	gpConEmu->LogWindowPos(szLabel);
}

void CSettings::UpdateSize(const CESize w, const CESize h)
{
	//Issue ???: Сохранять размер Quake?
	const bool bIgnoreWidth = (gpSet->isQuakeStyle != 0) && (gpSet->_WindowMode != wmNormal);

	if (w.IsValid(true) && h.IsValid(false))
	{
		if ((w.Raw != gpConEmu->WndWidth.Raw) || (h.Raw != gpConEmu->WndHeight.Raw))
		{
			gpConEmu->WndWidth.Set(true, w.Style, w.Value);
			gpConEmu->WndHeight.Set(false, h.Style, h.Value);
		}

		if (gpSet->isUseCurrentSizePos && ((w.Raw != gpSet->wndWidth.Raw) || (h.Raw != gpSet->wndHeight.Raw)))
		{
			if (!bIgnoreWidth)
				gpSet->wndWidth.Set(true, w.Style, w.Value);
			gpSet->wndHeight.Set(false, h.Style, h.Value);
		}
	}

	CSetPgSizePos* pSizePosPg;
	if (GetPageObj(pSizePosPg))
		pSizePosPg->FillSizeControls(pSizePosPg->Dlg());

	wchar_t szLabel[128];
	CESize ws = {w.Raw};
	CESize hs = {h.Raw};
	swprintf_c(szLabel, L"UpdateSize A={%s,%s} C={%s,%s} S={%s,%s}", ws.AsString(), hs.AsString(), gpConEmu->WndWidth.AsString(), gpConEmu->WndHeight.AsString(), gpSet->wndWidth.AsString(), gpSet->wndHeight.AsString());
	gpConEmu->LogWindowPos(szLabel);
}

void CSettings::UpdateFontInfo()
{
	CSetPgInfo* pPage;
	if (GetPageObj(pPage))
		pPage->FillFontInfo(pPage->Dlg());
}

void CSettings::PostUpdateCounters(bool bPosted)
{
	if (!bPosted)
	{
		if (!mb_MsgUpdateCounter)
		{
			mb_MsgUpdateCounter = TRUE;
			PostMessage(GetPage(thi_Info), mn_MsgUpdateCounter, 0, 0);
		}
		return;
	}

	wchar_t szTotal[256] = L"";

	if (mn_Freq!=0)
	{
		_ASSERTE(tPerfFPS == 0);

		for (INT_PTR nID = tPerfFPS; nID < tPerfLast; nID++)
		{
			wchar_t sTemp[64];

			int64_t v = 0, v2 = 0, v3 = 0;

			if (nID == tPerfFPS || nID == tPerfInterval)
			{
				int64_t *pFPS = nullptr;
				UINT nCount = 0;

				if (nID == tPerfFPS)
				{
					pFPS = mn_FPS; nCount = countof(mn_FPS);
				}
				else
				{
					pFPS = mn_RFPS; nCount = countof(mn_RFPS);
				}

				int64_t tmin, tmax;
				int64_t imin = 0, imax = 0;
				tmax = tmin = pFPS[0];

				for (UINT i = 0; i < nCount; i++)
				{
					const int64_t vi = pFPS[i]; //-V108
					if (!vi) continue;

					if (vi < tmin) { tmin = vi; imin = i; }
					if (vi > tmax) { tmax = vi; imax = i; }
				}

				if ((tmax > tmin) && mn_Freq > 0)
				{
					_ASSERTE(imin!=imax);
					int64_t iSamples = imax - imin;
					if (iSamples < 0)
						iSamples += nCount;
					v = iSamples * 10 * mn_Freq / (tmax - tmin);
				}
			}
			else if (nID == tPerfKeyboard)
			{
				v = v2 = mn_KbdDelays[0]; v3 = 0;

				const size_t nCount = std::max<int>(0, std::min<int>(mn_KBD_CUR_FRAME, static_cast<int>(countof(mn_KbdDelays))));
				for (size_t i = 0; i < nCount; i++)
				{
					const int64_t vi = mn_KbdDelays[i];
					// Skip too large values, they may be false detected
					if (vi <= 0 || vi >= 5000) continue;

					if (vi < v)
						v = vi;
					if (vi > v2 && vi < 5000)
						v2 = vi;
					v3 += vi; // avg
				}

				if (nCount > 0)
					v3 = v3 / nCount;
			}
			else
			{
				v = (10000 * static_cast<long long>(mn_CounterMax[nID])) / mn_Freq;
			}

			// WinApi's wsprintf can't do float/double, so we use integer arithmetics for FPS and others

			if (nID == tPerfKeyboard)
				swprintf_c(sTemp, L"%u/%u/%u", static_cast<int>(v), static_cast<int>(v3), static_cast<int>(v2));
			else
				swprintf_c(sTemp, L"%u.%u", static_cast<int>(v / 10), static_cast<int>(v % 10));

			switch (nID)
			{
			case tPerfFPS:
				wcscat_c(szTotal, L"   FPS:"); break;
			case tPerfData:
				wcscat_c(szTotal, L"   Data:"); break;
			case tPerfRender:
				wcscat_c(szTotal, L"   Render:"); break;
			case tPerfBlt:
				wcscat_c(szTotal, L"   Blt:"); break;
			case tPerfInterval:
				wcscat_c(szTotal, L"   RPS:"); break;
			case tPerfKeyboard:
				wcscat_c(szTotal, L"   KBD:"); break;
			default:
				wcscat_c(szTotal, L"   ???:");
			}
			wcscat_c(szTotal, sTemp);
		} // if (mn_Freq!=0)
	}

	SetDlgItemText(GetPage(thi_Info), tPerfCounters, SkipNonPrintable(szTotal)); //-V107

	// Done, allow next show cycle
	mb_MsgUpdateCounter = FALSE;
}

void CSettings::Performance(UINT nID, BOOL bEnd)
{
	_ASSERTE(gbPerformance > tPerfLast);

	if (nID == gbPerformance)  //groupbox ctrl id
	{
		if (!isMainThread())
			return;

		if (ghOpWnd)
		{
			// Performance
			wchar_t sTemp[32];
			// These are not MHz. E.g. on "AMD Athlon 64 X2 1999 MHz" we get "0.004 GHz"
			swprintf_c(sTemp, L" (%I64i)", static_cast<int64_t>(mn_Freq / 1000));
			CEStr lsTemp;
			const CEStr lsInfo(gpLng->getControl(gbPerformance, lsTemp, L"Performance counters"), sTemp);
			SetDlgItemText(GetPage(thi_Info), nID, lsInfo);
			// Update immediately
			PostUpdateCounters(true);
		}

		return;
	}

	if (nID >= tPerfLast)
		return;

	int64_t tick2 = 0, t;

	if (nID == tPerfFPS)
	{
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&tick2));
		mn_FPS[mn_FPS_CUR_FRAME] = tick2;
		mn_FPS_CUR_FRAME++;

		if (mn_FPS_CUR_FRAME >= static_cast<int>(countof(mn_FPS))) mn_FPS_CUR_FRAME = 0;

		if (ghOpWnd)
			PostUpdateCounters(false);

		return;
	}

	if (nID == tPerfInterval)
	{
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&tick2));
		mn_RFPS[mn_RFPS_CUR_FRAME] = tick2;
		mn_RFPS_CUR_FRAME++;

		if (mn_RFPS_CUR_FRAME >= static_cast<int>(countof(mn_RFPS)))
			mn_RFPS_CUR_FRAME = 0;

		if (ghOpWnd)
			PostUpdateCounters(false);

		return;
	}

	if (nID == tPerfKeyboard)
	{
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&tick2));

		if (bEnd == (BOOL)-1)
		{
			// After tab switch for example (RCon changed);
			mn_KbdDelayCounter = 0;
		}
		else if (!bEnd)
		{
			// Must be called when user press something on the keyboard
			if (!mn_KbdDelayCounter)
				mn_KbdDelayCounter = tick2;
		}
		else if (mn_KbdDelayCounter && mn_Freq)
		{
			const int64_t iPrev = mn_KbdDelayCounter; mn_KbdDelayCounter = 0;
			// let eval ms the delay of console output is refreshed
			t = (tick2 - iPrev) * 1000 / mn_Freq;
			const int idx = static_cast<int>(InterlockedIncrement(&mn_KBD_CUR_FRAME) & (countof(mn_KbdDelays) - 1));
			mn_KbdDelays[idx] = t;
			if (mn_KBD_CUR_FRAME < 0) mn_KBD_CUR_FRAME = countof(mn_KbdDelays)-1;
		}

		return;
	}

	if (!bEnd)
	{
		QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&(mn_Counter[nID])));
		return;
	}
	else if (!mn_Counter[nID] || !mn_Freq)
	{
		return;
	}

	QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&tick2));
	t = (tick2-mn_Counter[nID]);

	if ((t >= 0)
		&& ((mn_CounterMax[nID] < t)
	        || ((GetTickCount() - mn_CounterTick[nID]) > COUNTER_REFRESH)))
	{
		mn_CounterMax[nID] = t;
		mn_CounterTick[nID] = GetTickCount();

		if (ghOpWnd)
			PostUpdateCounters(false);
	}
}

DWORD CSettings::BalloonStyle()
{
	//грр, Issue 886, и подсказки нифига не видны
	//[HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced]
	//"EnableBalloonTips"=0

	DWORD nBalloonStyle = TTS_BALLOON;

	HKEY hk;
	if (0 == RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced", 0, KEY_READ, &hk))
	{
		DWORD nEnabled = 1;
		DWORD nSize = sizeof(nEnabled);
		if ((0 == RegQueryValueEx(hk, L"EnableBalloonTips", nullptr, nullptr, reinterpret_cast<LPBYTE>(&nEnabled), &nSize)) && (nEnabled == 0))
		{
			nBalloonStyle = 0;
		}
		RegCloseKey(hk);
	}

	return nBalloonStyle;
}

void CSettings::RegisterTipsFor(HWND hChildDlg)
{
	//TODO: Actually, all dialogs have to be localized

	if (gpSetCls->hConFontDlg == hChildDlg)
	{
		if (!hwndConFontBalloon || !IsWindow(hwndConFontBalloon))
		{
			hwndConFontBalloon = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
			                                    BalloonStyle() | WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX | TTS_CLOSE,
			                                    CW_USEDEFAULT, CW_USEDEFAULT,
			                                    CW_USEDEFAULT, CW_USEDEFAULT,
			                                    ghOpWnd, nullptr,
			                                    g_hInstance, nullptr);
			SetWindowPos(hwndConFontBalloon, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			// Set up tool information.
			// In this case, the "tool" is the entire parent window.
			tiConFontBalloon.cbSize = 44; // был sizeof(TOOLINFO);
			tiConFontBalloon.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
			tiConFontBalloon.hwnd = hChildDlg;
			tiConFontBalloon.hinst = g_hInstance;
			static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
			tiConFontBalloon.lpszText = szAsterisk;
			tiConFontBalloon.uId = reinterpret_cast<UINT_PTR>(hChildDlg);
			GetClientRect(ghOpWnd, &tiConFontBalloon.rect);
			// Associate the ToolTip with the tool window.
			SendMessage(hwndConFontBalloon, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(static_cast<LPTTTOOLINFOW>(&tiConFontBalloon)));
			// Allow multiline
			SendMessage(hwndConFontBalloon, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(300));
		}

		CDynDialog::LocalizeDialog(hChildDlg);

	}
	else
	{
		// Used for highlight found control, for example
		if (!hwndBalloon || !IsWindow(hwndBalloon))
		{
			hwndBalloon = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
			                             BalloonStyle() | WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
			                             CW_USEDEFAULT, CW_USEDEFAULT,
			                             CW_USEDEFAULT, CW_USEDEFAULT,
			                             ghOpWnd, nullptr,
			                             g_hInstance, nullptr);
			SetWindowPos(hwndBalloon, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			// Set up tool information.
			// In this case, the "tool" is the entire parent window.
			tiBalloon.cbSize = 44; // был sizeof(TOOLINFO);
			tiBalloon.uFlags = TTF_IDISHWND | TTF_TRACK | TTF_ABSOLUTE;
			tiBalloon.hwnd = GetPage(thi_General);
			tiBalloon.hinst = g_hInstance;
			static wchar_t szAsterisk[] = L"*"; // eliminate GCC warning
			tiBalloon.lpszText = szAsterisk;
			tiBalloon.uId = reinterpret_cast<UINT_PTR>(tiBalloon.hwnd);
			GetClientRect(ghOpWnd, &tiBalloon.rect);
			// Associate the ToolTip with the tool window.
			SendMessage(hwndBalloon, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(static_cast<LPTTTOOLINFOW>(&tiBalloon)));
			// Allow multiline
			SendMessage(hwndBalloon, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(300));
		}

		// Create the ToolTip.
		if (!hwndTip || !IsWindow(hwndTip))
		{
			hwndTip = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, nullptr,
			                         BalloonStyle() | WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         CW_USEDEFAULT, CW_USEDEFAULT,
			                         ghOpWnd, nullptr,
			                         g_hInstance, nullptr);
			SetWindowPos(hwndTip, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
			SendMessage(hwndTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
		}

		// Register tips, if succeeded
		if (hwndTip)
			EnumChildWindows(hChildDlg, RegisterTipsForChild, reinterpret_cast<LPARAM>(hChildDlg));
		else
			CDynDialog::LocalizeDialog(hChildDlg);

		// Final tooltip polishing
		if (hwndTip)
			SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0, static_cast<LPARAM>(EvalSize(300, esf_CanUseDpi | esf_Horizontal)));
	}
}

BOOL CSettings::RegisterTipsForChild(HWND hChild, LPARAM lParam)
{
	HWND hChildDlg = reinterpret_cast<HWND>(lParam);

	#if defined(_DEBUG)
	const LONG wID = GetWindowLong(hChild, GWL_ID);
	if (wID == stCmdGroupCommands)
		std::ignore = wID;
	#endif

	// Localize Control text
	CDynDialog::LocalizeControl(hChild, lParam);

	// Register tooltip by child HWND
	if (gpSetCls->hwndTip)
	{
		bool lbRc = false;
		HWND hEdit = nullptr;

		// Associate the ToolTip with the tool.
		TOOLINFO toolInfo{};
		toolInfo.cbSize = 44; //sizeof(toolInfo); -- need to work on Win2k and compile with Vista+
		GetWindowRect(hChild, &toolInfo.rect);
		MapWindowPoints(nullptr, hChildDlg, reinterpret_cast<LPPOINT>(&toolInfo.rect), 2);
		toolInfo.hwnd = hChildDlg;
		toolInfo.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		toolInfo.uId = reinterpret_cast<UINT_PTR>(hChild);

		// Use CSettings::ProcessTipHelp dynamically
		toolInfo.lpszText = LPSTR_TEXTCALLBACK;

		lbRc = SendMessage(gpSetCls->hwndTip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
		hEdit = FindWindowEx(hChild, nullptr, L"Edit", nullptr);

		if (hEdit)
		{
			toolInfo.lpszText = LPSTR_TEXTCALLBACK;
			toolInfo.uId = reinterpret_cast<UINT_PTR>(hEdit);

			GetWindowRect(hEdit, &toolInfo.rect);
			MapWindowPoints(nullptr, hChildDlg, reinterpret_cast<LPPOINT>(&toolInfo.rect), 2);
			toolInfo.hwnd = hChildDlg;

			lbRc = SendMessage(gpSetCls->hwndTip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&toolInfo));
		}

		UNREFERENCED_PARAMETER(lbRc);
	}

	return TRUE; // Continue enumeration
}


// Вызывается из диалога настроек
void CSettings::RecreateFont(WORD wFromID) const
{
	_ASSERTE(wFromID != static_cast<WORD>(-1));
	gpFontMgr->RecreateFont(false, (wFromID == cbFontAsDeviceUnits || wFromID == cbFontMonitorDpi));

	if (ghOpWnd && (wFromID == tFontFace))
	{
		HWND hMainPg = gpSetCls->GetPage(thi_Fonts);
		if (hMainPg)
		{
			wchar_t szSize[10];
			swprintf_c(szSize, L"%i", gpSet->FontSizeY);
			SetDlgItemText(hMainPg, tFontSizeY, szSize);
		}
	}
}

// "Умолчательная" высота буфера.
// + ConEmu стартует в буферном режиме
// + команда по умолчанию (если не задана в реестре или ком.строке) будет "cmd", а не "far"
void CSettings::SetArgBufferHeight(int anBufferHeight)
{
	_ASSERTE(anBufferHeight>=0);
	//if (anBufferHeight>=0) gpSet->DefaultBufferHeight = anBufferHeight;
	//ForceBufferHeight = (gpSet->DefaultBufferHeight != 0);
	bForceBufferHeight = true;
	nForceBufferHeight = anBufferHeight;
	if (anBufferHeight==0)
	{
		gpConEmu->SetDefaultCmd(L"far");
	}
	else
	{
		const CEStr pszComspec = GetComspec(&gpSet->ComSpec);
		_ASSERTE(!pszComspec.IsEmpty());
		gpConEmu->SetDefaultCmd(pszComspec ? pszComspec : L"cmd");
	}
}

RecreateActionParm CSettings::GetDefaultCreateAction()
{
	if (IsMulti() || !gpConEmu->isVConExists(0))
		return cra_CreateTab;
	return cra_CreateWindow;
}

bool CSettings::IsMulti()
{
	if (!gpSet->mb_isMulti)
	{
		// "SingleInstance" has more weight
		if (!IsSingleInstanceArg())
			return false;
		// Otherwise we'll get infinite loop
	}
	return true;
}

bool CSettings::IsSingleInstanceArg() const
{
	if (SingleInstanceArg == sgl_Enabled)
		return true;

	if ((SingleInstanceArg == sgl_Default) && (gpSet->isSingleInstance || gpSet->isQuakeStyle))
		return true;

	return false;
}

bool CSettings::ResetCmdHistory(HWND hParent)
{
	if (IDYES != MsgBox(L"Do you want to clear current history?\nThis can not be undone!",
			MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2, gpConEmu->GetDefaultTitle(), hParent ? hParent : ghWnd))
	{
		return false;
	}

	gpSet->HistoryReset();

	return true;
}

void CSettings::SetSaveCmdHistory(bool bSaveHistory)
{
	gpSet->isSaveCmdHistory = bSaveHistory;

	// И сразу сохранить в настройках
	SettingsBase* reg = gpSet->CreateSettings(nullptr);
	if (!reg)
	{
		_ASSERTE(reg!=nullptr);
		return;
	}

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		reg->Save(L"SaveCmdHistory", gpSet->isSaveCmdHistory);
		reg->CloseKey();
	}

	delete reg;
}













void CSettings::RegisterTabs()
{
	if (!mb_TabHotKeyRegistered)
	{
		if (RegisterHotKey(ghOpWnd, HOTKEY_ID_TAB_NEXT, MOD_CONTROL, VK_TAB))
			mb_TabHotKeyRegistered = TRUE;

		RegisterHotKey(ghOpWnd, HOTKEY_ID_TAB_PREV, MOD_CONTROL|MOD_SHIFT, VK_TAB);
	}
}

void CSettings::UnregisterTabs()
{
	if (mb_TabHotKeyRegistered)
	{
		UnregisterHotKey(ghOpWnd, HOTKEY_ID_TAB_NEXT);
		UnregisterHotKey(ghOpWnd, HOTKEY_ID_TAB_PREV);
	}

	mb_TabHotKeyRegistered = FALSE;
}





// Показать в "Инфо" текущий режим консоли
void CSettings::UpdateConsoleMode(CRealConsole* pRCon)
{
	CSetPgInfo* pInfoPg;
	HWND hInfoPg = GetPage(thi_Info);
	if (GetPageObj(pInfoPg) && IsWindow(hInfoPg))
		pInfoPg->FillConsoleMode(hInfoPg, pRCon);
}


typedef long(WINAPI* ThemeFunction_t)();

bool CSettings::CheckTheming()
{
	static bool bChecked = false;

	if (bChecked)
		return mb_ThemingEnabled;

	bChecked = true;
	mb_ThemingEnabled = false;

	if (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1))
	{
		ThemeFunction_t fIsAppThemed = nullptr;
		ThemeFunction_t fIsThemeActive = nullptr;
		HMODULE hUxTheme = GetModuleHandle(L"UxTheme.dll");

		if (hUxTheme)
		{
			fIsAppThemed = (ThemeFunction_t)GetProcAddress(hUxTheme, "IsAppThemed");
			fIsThemeActive = (ThemeFunction_t)GetProcAddress(hUxTheme, "IsThemeActive");

			if (fIsAppThemed && fIsThemeActive)
			{
				long llThemed = fIsAppThemed();
				long llActive = fIsThemeActive();

				if (llThemed && llActive)
					mb_ThemingEnabled = true;
			}
		}
	}

	return mb_ThemingEnabled;
}

void CSettings::CenterMoreDlg(HWND hWnd2)
{
	RECT rcParent, rc;
	GetWindowRect(ghOpWnd ? ghOpWnd : ghWnd, &rcParent);
	GetWindowRect(hWnd2, &rc);
	MoveWindow(hWnd2,
	           (rcParent.left+rcParent.right-rc.right+rc.left)/2,
	           (rcParent.top+rcParent.bottom-rc.bottom+rc.top)/2,
	           rc.right - rc.left, rc.bottom - rc.top, TRUE);
}

void CSettings::OnPanelViewAppeared(BOOL abAppear)
{
	HWND hViewsPg = GetPage(thi_Views);
	if (hViewsPg && IsWindow(hViewsPg))
	{
		if (abAppear != IsWindowEnabled(GetDlgItem(hViewsPg, bApplyViewSettings)))
			EnableWindow(GetDlgItem(hViewsPg, bApplyViewSettings), abAppear);
	}
}

bool CSettings::IsBackgroundEnabled(CVirtualConsole* apVCon) const
{
	// Если плагин фара установил свой фон
	if (gpSet->isBgPluginAllowed && apVCon && apVCon->HasBackgroundImage(nullptr,nullptr))
	{
		if (apVCon->isEditor || apVCon->isViewer)
			return (gpSet->isBgPluginAllowed == 1);

		return true;
	}

	// Иначе - по настройкам ConEmu
	#ifndef APPDISTINCTBACKGROUND
	if (!isBackgroundImageValid)
		return false;
	#else
	CBackgroundInfo* pBgObject = apVCon ? apVCon->GetBackgroundObject() : mp_BgInfo;
	const bool bBgExist = (pBgObject && pBgObject->GetBgImgData() != nullptr);
	SafeRelease(pBgObject);
	if (!bBgExist)
		return false;
	#endif

	if (apVCon && (apVCon->isEditor || apVCon->isViewer))
	{
		return (gpSet->isShowBgImage == 1);
	}
	else
	{
		return (gpSet->isShowBgImage != 0);
	}
}

void CSettings::SetBgImageDarker(uint8_t newValue, bool bUpdate)
{
	if (/*newV < 256*/ newValue != gpSet->bgImageDarker)
	{
		gpSet->bgImageDarker = newValue;

		HWND hBgPg = GetPage(thi_Backgr);
		if (hBgPg)
		{
			SendDlgItemMessage(hBgPg, slDarker, TBM_SETPOS, static_cast<WPARAM>(true), static_cast<LPARAM>(gpSet->bgImageDarker));

			TCHAR tmp[10];
			swprintf_c(tmp, L"%u", static_cast<UINT>(gpSet->bgImageDarker));
			SetDlgItemText(hBgPg, tDarker, tmp);
		}

		if (bUpdate)
		{
			// Картинку может установить и плагин
			if (gpSet->isShowBgImage && gpSet->sBgImage[0])
				LoadBackgroundFile(gpSet->sBgImage);

			NeedBackgroundUpdate();

			gpConEmu->Update(true);
		}
	}
}

CBackgroundInfo* CSettings::GetBackgroundObject() const
{
	if (mp_BgInfo)
		mp_BgInfo->AddRef();
	return mp_BgInfo;
}

bool CSettings::LoadBackgroundFile(LPCWSTR inPath, bool abShowErrors)
{
	bool lRes = false;

#ifdef APPDISTINCTBACKGROUND
	CBackgroundInfo* pNew = CBackgroundInfo::CreateBackgroundObject(inPath, abShowErrors);
	SafeRelease(mp_BgInfo);
	mp_BgInfo = pNew;
	lRes = (mp_BgInfo != nullptr);
#else
	//_ASSERTE(isMainThread());
	if (!inPath || _tcslen(inPath)>=MAX_PATH)
	{
		if (abShowErrors)
			MBoxA(L"Invalid 'inPath' in Settings::LoadImageFrom");

		return false;
	}

	_ASSERTE(isMainThread());
	BY_HANDLE_FILE_INFORMATION inf = {0};
	BITMAPFILEHEADER* pBkImgData = nullptr;

	if (wcspbrk(inPath, L"%\\.") == nullptr)
	{
		// May be "Solid color"
		COLORREF clr = (COLORREF)-1;
		if (::GetColorRef(inPath, &clr))
		{
			pBkImgData = CreateSolidImage(clr, 128, 128);
		}
	}

	if (!pBkImgData)
	{
		TCHAR exPath[MAX_PATH + 2];

		if (!ExpandEnvironmentStrings(inPath, exPath, MAX_PATH))
		{
			if (abShowErrors)
			{
				wchar_t szError[MAX_PATH*2];
				DWORD dwErr = GetLastError();
				swprintf_c(szError, L"Can't expand environment strings:\r\n%s\r\nError code=0x%08X\r\nImage loading failed",
				          inPath, dwErr);
				MBoxA(szError);
			}

			return false;
		}

		pBkImgData = LoadImageEx(exPath, inf);
	}

	if (pBkImgData)
	{
		ftBgModified = inf.ftLastWriteTime;
		nBgModifiedTick = GetTickCount();
		NeedBackgroundUpdate();
		//MSectionLock SBG; SBG.Lock(&mcs_BgImgData);
		SafeFree(mp_BgImgData);
		isBackgroundImageValid = true;
		mp_BgImgData = pBkImgData;
		lRes = true;
	}
#endif
	return lRes;
}


void CSettings::NeedBackgroundUpdate()
{
	#ifndef APPDISTINCTBACKGROUND
	mb_NeedBgUpdate = TRUE;
	#else
	CVConGuard VCon;
	for (int i = 0; CVConGroup::GetVCon(i, &VCon); i++)
	{
		VCon->NeedBackgroundUpdate();
	}
	#endif
}

void CSettings::ShowModifierErrorTip(LPCTSTR asInfo, HWND hDlg, WORD nID)
{
	ms_BalloonErrTip = asInfo;

	ShowErrorTip(ms_BalloonErrTip.ms_Val, hDlg, nID, hwndBalloon, &tiBalloon, hwndTip, FAILED_SELMOD_TIMEOUT, true);
}

void CSettings::ShowFontErrorTip(LPCTSTR asInfo)
{
	if (!ghOpWnd)
	{
		ms_BalloonErrTip.Release();
		return;
	}

	ms_BalloonErrTip = asInfo;

	ShowErrorTip(ms_BalloonErrTip.ms_Val, GetPage(thi_Fonts), tFontFace, hwndBalloon, &tiBalloon, hwndTip, FAILED_FONT_TIMEOUT);
}

void CSettings::ShowConFontErrorTip()
{
	ShowErrorTip(ms_ConFontError.ms_Val, hConFontDlg, tConsoleFontFace, hwndConFontBalloon, &tiConFontBalloon, nullptr, FAILED_CONFONT_TIMEOUT);
}

// общая функция
void CSettings::ShowErrorTip(wchar_t* asInfo, HWND hDlg, int nCtrlID, HWND hBall, TOOLINFO *pti, HWND hTip, DWORD nTimeout, bool bLeftAligh /*= false*/)
{
	pti->lpszText = asInfo;

	if (asInfo && *asInfo)
	{
		if (hTip) SendMessage(hTip, TTM_ACTIVATE, FALSE, 0);

		SendMessage(hBall, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(pti));
		RECT rcControl = {};
		HWND hCtrl = nCtrlID ? GetDlgItem(hDlg, nCtrlID) : nullptr;
		GetWindowRect(hCtrl ? hCtrl : hDlg, &rcControl);
		const int ptx = bLeftAligh ? (rcControl.left + 10) : (rcControl.right - 10);
		const int pty = bLeftAligh ? rcControl.bottom : (rcControl.top + rcControl.bottom) / 2;
		SendMessage(hBall, TTM_TRACKPOSITION, 0, MAKELONG(ptx,pty));
		SendMessage(hBall, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(pti));
		SetTimer(hDlg, BALLOON_MSG_TIMERID, nTimeout/*FAILED_FONT_TIMEOUT*/, nullptr);
	}
	else
	{
		SendMessage(hBall, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(pti));

		if (hTip) SendMessage(hTip, TTM_ACTIVATE, TRUE, 0);
	}
}



/* ********************************************** */
/*         обработка шрифта в RealConsole         */
/* ********************************************** */

bool CSettings::EditConsoleFont(HWND hParent)
{
	hConFontDlg = nullptr; nConFontError = 0;
	const INT_PTR nRc = CDynDialog::ExecuteDialog(IDD_MORE_CONFONT, hParent, EditConsoleFontProc, 0);
	hConFontDlg = nullptr;
	return (nRc == IDOK);
}

bool CSettings::CheckConsoleFontRegistry(LPCWSTR asFaceName)
{
	bool lbFound = false;
	HKEY hk;
	const DWORD nRights = KEY_READ | WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0), 0);

	if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
	                  0, nRights, &hk))
	{
		wchar_t szId[32] = {0}, szFont[255]; DWORD dwLen, dwType;
		LONG iRc;

		if (gbIsDBCS)
		{
			DWORD idx = 0, cchName = countof(szId), dwLen = sizeof(szFont)-2;
			while ((iRc = RegEnumValue(hk, idx++, szId, &cchName, nullptr, &dwType, (LPBYTE)szFont, &dwLen)) == 0)
			{
				szId[std::min<size_t>(countof(szId)-1,cchName)] = 0;
				szFont[std::min<size_t>(countof(szFont)-1,dwLen/2)] = 0;
				wchar_t* pszEnd;
				if (wcstoul(szId, &pszEnd, 10) && *szFont)
				{
					if (lstrcmpi((szFont[0] == L'*') ? (szFont+1) : szFont, asFaceName) == 0)
					{
						lbFound = true; break;
					}
				}
				cchName = countof(szId); dwLen = sizeof(szFont)-2;
			}
		}

		if (!lbFound)
		{
			for (DWORD i = 0; i < 20; i++)
			{
				szId[i] = L'0'; szId[i+1] = 0; wmemset(szFont, 0, 255);

				if (RegQueryValueExW(hk, szId, nullptr, &dwType, reinterpret_cast<LPBYTE>(szFont), &(dwLen = 255*2)))
					break;

				if (lstrcmpi(szFont, asFaceName) == 0)
				{
					lbFound = true; break;
				}
			}
		}

		RegCloseKey(hk);
	}

	return lbFound;
}

// Вызывается при запуске ConEmu для быстрой проверки шрифта
// EnumFontFamilies не вызывается, т.к. занимает время
bool CSettings::CheckConsoleFontFast(LPCWSTR asCheckName /*= nullptr*/)
{
	// В ReactOS шрифт не меняется и в реестре не регистрируется
	if (gpStartEnv->bIsReactOS)
	{
		return true;
	}

	//wchar_t szCreatedFaceName[32] = {0};
	LOGFONT LF = gpSet->ConsoleFont;
	BOOL bCheckStarted = FALSE;
	DWORD nCheckResult = -1;
	DWORD nCheckWait = -1;

	gpSetCls->nConFontError = 0; //ConFontErr_NonSystem|ConFontErr_NonRegistry|ConFontErr_InvalidName;
	if (asCheckName && *asCheckName)
		wcscpy_c(LF.lfFaceName, asCheckName);

	HFONT hf = CreateFontIndirect(&LF);

	if (!hf)
	{
		gpSetCls->nConFontError = ConFontErr_InvalidName;
	}
	else
	{
		LPOUTLINETEXTMETRICW lpOutl = gpFontMgr->LoadOutline(nullptr, hf);

		if (!lpOutl)
		{
			gpSetCls->nConFontError = ConFontErr_InvalidName;
		}
		else
		{
			const wchar_t* pszFamilyName = reinterpret_cast<LPCWSTR>(lpOutl->otmpFamilyName);

			// Интересуют только TrueType (вроде только для TTF доступен lpOutl - проверить
			if (pszFamilyName[0] != L'@'
			        && (gbIsDBCS || CFontMgr::IsAlmostMonospace(pszFamilyName, &lpOutl->otmTextMetrics, lpOutl /*lpOutl->otmTextMetrics.tmMaxCharWidth, lpOutl->otmTextMetrics.tmAveCharWidth, lpOutl->otmTextMetrics.tmHeight*/))
			        && lpOutl->otmPanoseNumber.bProportion == PAN_PROP_MONOSPACED
			        && lstrcmpi(pszFamilyName, LF.lfFaceName) == 0
			  )
			{
				BOOL lbNonSystem = FALSE;

				// Нельзя использовать шрифты, которые зарегистрированы нами (для ConEmu). Они должны быть системными
				//for (std::vector<RegFont>::iterator iter = gpSetCls->m_RegFonts.begin(); !lbNonSystem && iter != gpSetCls->m_RegFonts.end(); ++iter)
				for (auto& font : gpFontMgr->m_RegFonts)
				{
					if (!font.bAlreadyInSystem &&
					        lstrcmpi(font.szFontName, LF.lfFaceName) == 0)
						lbNonSystem = TRUE;
				}

				if (lbNonSystem)
					gpSetCls->nConFontError = ConFontErr_NonSystem;
			}
			else
			{
				gpSetCls->nConFontError = ConFontErr_NonSystem;
			}

			free(lpOutl);
		}
		DeleteObject(hf);
	}

	// Если успешно - проверить зарегистрированность в реестре
	if (gpSetCls->nConFontError == 0)
	{
		if (!CheckConsoleFontRegistry(LF.lfFaceName))
			gpSetCls->nConFontError |= ConFontErr_NonRegistry;
	}

	// WinPE may not have "Lucida Console" preinstalled
	if (gpSetCls->nConFontError && !asCheckName && gpStartEnv && (gpStartEnv->bIsWinPE == 1) && (lstrcmpi(LF.lfFaceName, gsAltConFont) != 0))
	{
		const DWORD errSave = gpSetCls->nConFontError;
		if (CheckConsoleFontFast(gsAltConFont))
		{
			// But has "Courier New"
			wcscpy_c(gpSet->ConsoleFont.lfFaceName, gsAltConFont);
			goto wrap;
		}
		gpSetCls->nConFontError = errSave;
	}

	if ((gpSetCls->nConFontError & ConFontErr_NonRegistry)
		|| (gbIsWine && gpSetCls->nConFontError))
	{
		wchar_t szCmd[MAX_PATH+64] = L"\"";
		wcscat_c(szCmd, gpConEmu->ms_ConEmuBaseDir);
		wchar_t* psz = szCmd + _tcslen(szCmd);
		_wcscpy_c(psz, 64, L"\\" ConEmuC_32_EXE);
		if (IsWindows64() && !FileExists(szCmd+1))
		{
			_wcscpy_c(psz, 64, L"\\" ConEmuC_64_EXE);
		}
		wcscat_c(szCmd, L"\" /CheckUnicode");

		PROCESS_INFORMATION pi = {};
		STARTUPINFO si = {};
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_HIDE; //RELEASEDEBUGTEST(SW_HIDE,SW_SHOW);

		#if 0
		AllocConsole();
		#endif

		bCheckStarted = CreateProcess(nullptr, szCmd, nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS|CREATE_DEFAULT_ERROR_MODE|CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);
		if (bCheckStarted)
		{
			nCheckWait = WaitForSingleObject(pi.hProcess, 5000);
			if (nCheckWait == WAIT_OBJECT_0)
			{
				GetExitCodeProcess(pi.hProcess, &nCheckResult);

				if (static_cast<int>(nCheckResult) == CERR_UNICODE_CHK_OKAY)
				{
					gpSetCls->nConFontError = 0;
				}
			}
			else
			{
				TerminateProcess(pi.hProcess, 100);
			}

			#if 0
			wchar_t szDbg[1024];
			swprintf_c(szDbg, L"Cmd:\n%s\nExitCode=%i", szCmd, nCheckResult);
			MBoxA(szDbg);
			FreeConsole();
			#endif
		}
	}

wrap:
	bConsoleFontChecked = (gpSetCls->nConFontError == 0);
	if (gpSet->isLogging())
	{
		wchar_t szInfo[128];
		swprintf_c(szInfo, L"CheckConsoleFontFast(`%s`,`%s`) = %u",
			asCheckName ? asCheckName : L"nullptr", LF.lfFaceName, gpSetCls->nConFontError);
		LogString(szInfo);
	}
	return bConsoleFontChecked;
}

bool CSettings::CheckConsoleFont(HWND ahDlg)
{
	if (gbIsDBCS)
	{
		// В DBCS IsAlmostMonospace работает неправильно
		gpSetCls->CheckConsoleFontFast();

		// Заполнить список шрифтами из реестра
		HKEY hk;
		const DWORD nRights = KEY_READ | WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0), 0);

		if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
						  0, nRights, &hk))
		{
			wchar_t szId[32] = {0}, szFont[255]; DWORD dwType; LONG iRc;

			DWORD idx = 0, cchName = countof(szId), dwLen = sizeof(szFont)-2;
			INT_PTR nIdx = -1;
			while ((iRc = RegEnumValue(hk, idx++, szId, &cchName, nullptr, &dwType, reinterpret_cast<LPBYTE>(szFont), &dwLen)) == 0)
			{
				szId[std::min<size_t>(countof(szId)-1,cchName)] = 0;
				szFont[std::min<size_t>(countof(szFont)-1,dwLen/2)] = 0;
				if (*szFont)
				{
					LPCWSTR pszFaceName = (szFont[0] == L'*') ? (szFont+1) : szFont;
					if (SendDlgItemMessage(ahDlg, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(pszFaceName))==-1)
					{
						nIdx = SendDlgItemMessage(ahDlg, tConsoleFontFace, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(pszFaceName)); //-V103
					}
				}
				cchName = countof(szId); dwLen = sizeof(szFont)-2;
			}
			UNREFERENCED_PARAMETER(nIdx);
		}
	}
	else
	{
		gpSetCls->nConFontError = ConFontErr_NonSystem|ConFontErr_NonRegistry;

		bool bLoaded = false;

		if (ahDlg && IsWinDBCS())
		{
			// Chinese
			HKEY hk = nullptr;
			const DWORD nRights = KEY_READ | WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0), 0);
			if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont", 0, nRights, &hk))
			{
				DWORD dwIndex = 0;
				wchar_t szName[255], szValue[255] = {};
				DWORD cchName = countof(szName), cbData = sizeof(szValue)-2, dwType;
				while (!RegEnumValue(hk, dwIndex++, szName, &cchName, nullptr, &dwType, reinterpret_cast<LPBYTE>(szValue), &cbData))
				{
					if ((dwType == REG_DWORD) && *szName && *szValue)
					{
						SendDlgItemMessage(ahDlg, tConsoleFontFace, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(szValue));
						bLoaded = true;
					}

					// Next
					cchName = countof(szName); cbData = sizeof(szValue);
					ZeroStruct(szValue);
				}
				RegCloseKey(hk);
			}
		}

		if (!bLoaded)
		{
			// Сначала загрузить имена шрифтов, установленных в систему
			HDC hdc = GetDC(nullptr);
			EnumFontFamilies(hdc, static_cast<LPCTSTR>(nullptr), reinterpret_cast<FONTENUMPROC>(EnumConFamCallBack), reinterpret_cast<LPARAM>(ahDlg));
			DeleteDC(hdc);
		}

		// Проверить реестр
		if (CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
			gpSetCls->nConFontError &= ~static_cast<DWORD>(ConFontErr_NonRegistry);
	}

	// Показать текущий шрифт
	if (ahDlg)
	{
		if (CSetDlgLists::SelectString(ahDlg, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName)<0)
			SetDlgItemText(ahDlg, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName);
	}

	return (gpSetCls->nConFontError == 0);
}

INT_PTR CSettings::EditConsoleFontProc(HWND hWnd2, UINT messg, WPARAM wParam, LPARAM lParam)
{
	switch (messg)
	{
		case WM_NOTIFY:
			{
				LPNMHDR phdr = reinterpret_cast<LPNMHDR>(lParam);

				if (phdr->code == TTN_GETDISPINFO)
				{
					return gpSetCls->ProcessTipHelp(hWnd2, messg, wParam, lParam);
				}

				break;
			}
		case WM_INITDIALOG:
		{
			SendMessage(hWnd2, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hClassIcon));
			SendMessage(hWnd2, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hClassIconSm));

			gpSetCls->hConFontDlg = nullptr; // пока не выставим - на смену в контролах не реагировать
			wchar_t temp[10];
			const DWORD* pnSizesSmall = nullptr;
			const unsigned nCount = CSetDlgLists::GetListItems(CSetDlgLists::eFSizesSmall, pnSizesSmall);
			for (unsigned i = 0; i < nCount; i++)
			{
				swprintf_c(temp, L"%i", pnSizesSmall[i]);
				SendDlgItemMessage(hWnd2, tConsoleFontSizeY, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(temp));
				swprintf_c(temp, L"%i", static_cast<int>(pnSizesSmall[i] * 3 / 2));
				SendDlgItemMessage(hWnd2, tConsoleFontSizeX, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(temp));

				if (static_cast<LONG>(pnSizesSmall[i]) >= gpFontMgr->LogFont.lfHeight)
					break; // не допускаются шрифты больше, чем выбрано для основного шрифта!
			}

			swprintf_c(temp, L"%i", gpSet->ConsoleFont.lfHeight);
			CSetDlgLists::SelectStringExact(hWnd2, tConsoleFontSizeY, temp);
			swprintf_c(temp, L"%i", gpSet->ConsoleFont.lfWidth);
			CSetDlgLists::SelectStringExact(hWnd2, tConsoleFontSizeX, temp);

			// Показать текущий шрифт и проверить его
			if (CheckConsoleFont(hWnd2))
			{
				EnableWindow(GetDlgItem(hWnd2, bConFontOK), TRUE);
				EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), FALSE);
			}
			else
			{
				EnableWindow(GetDlgItem(hWnd2, bConFontOK), FALSE);
				EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), (gpSetCls->nConFontError&ConFontErr_NonRegistry)!=0);
			}

			// BCM_SETSHIELD = 5644
			if (gOSVer.dwMajorVersion >= 6)
				SendDlgItemMessage(hWnd2, bConFontAdd2HKLM, 5644/*BCM_SETSHIELD*/, 0, TRUE);

			// запускаем user-mode
			gpSetCls->hConFontDlg = hWnd2;
			gpSetCls->RegisterTipsFor(hWnd2);
			CenterMoreDlg(hWnd2);

			if (gpConEmu->isVConExists(0))
				EnableWindow(GetDlgItem(hWnd2, tConsoleFontConsoleNote), TRUE);

			// Если есть ошибка - показать
			gpSetCls->bShowConFontError = (gpSetCls->nConFontError != 0);
		}
		break;
		case WM_DESTROY:

			if (gpSetCls->hwndConFontBalloon) {DestroyWindow(gpSetCls->hwndConFontBalloon); gpSetCls->hwndConFontBalloon = nullptr;}

			gpSetCls->hConFontDlg = nullptr;
			break;
		case WM_TIMER:

			if (wParam == BALLOON_MSG_TIMERID)
			{
				KillTimer(hWnd2, BALLOON_MSG_TIMERID);
				SendMessage(gpSetCls->hwndConFontBalloon, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&gpSetCls->tiConFontBalloon));
			}

			break;
		//case WM_GETICON:

		//	if (wParam==ICON_BIG)
		//	{
		//		/*SetWindowLong(hWnd2, DWL_MSGRESULT, (LRESULT)hClassIcon);
		//		return 1;*/
		//	}
		//	else
		//	{
		//		SetWindowLongPtr(hWnd2, DWLP_MSGRESULT, (LRESULT)hClassIconSm);
		//		return 1;
		//	}

		//	return 0;
		case WM_COMMAND:

			if (HIWORD(wParam) == BN_CLICKED)
			{
				const WORD TB = LOWORD(wParam);

				if (TB == IDOK)
					return 0;
				else if (TB == bConFontOK)
				{
					// На всякий случай, повторно считаем поля диалога
					GetDlgItemText(hWnd2, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName, countof(gpSet->ConsoleFont.lfFaceName));
					gpSet->ConsoleFont.lfHeight = GetNumber(hWnd2, tConsoleFontSizeY);
					gpSet->ConsoleFont.lfWidth = GetNumber(hWnd2, tConsoleFontSizeX);

					// Проверка валидности
					if (gpSetCls->nConFontError == ConFontErr_NonRegistry && gpSetCls->CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
					{
						gpSetCls->nConFontError &= ~static_cast<DWORD>(ConFontErr_NonRegistry);
					}

					if (gpSetCls->nConFontError)
					{
						_ASSERTE(gpSetCls->nConFontError==0);
						MsgBox((gpSetCls->ms_ConFontError ? gpSetCls->ms_ConFontError.c_str() : gpSetCls->CreateConFontError(nullptr,nullptr)),
							MB_OK|MB_ICONSTOP, gpConEmu->GetDefaultTitle(), hWnd2);
						return 0;
					}

					gpSet->SaveConsoleFont(); // Сохранить шрифт в настройке
					EndDialog(hWnd2, IDOK);
				}
				else if (TB == IDCANCEL || TB == bConFontCancel)
				{
					if (!gpSetCls->bConsoleFontChecked)
					{
						wcscpy_c(gpSet->ConsoleFont.lfFaceName, gpSetCls->sDefaultConFontName[0] ? gpSetCls->sDefaultConFontName : gsDefConFont);
						gpSet->ConsoleFont.lfHeight = 5; gpSet->ConsoleFont.lfWidth = 3;
					}

					EndDialog(hWnd2, IDCANCEL);
				}
				else if (TB == bConFontAdd2HKLM)
				{
					// Добавить шрифт в HKLM\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Console\TrueTypeFont
					gpSetCls->ms_ConFontError.Release();
					gpSetCls->ShowConFontErrorTip();
					EnableWindow(GetDlgItem(hWnd2, tConsoleFontHklmNote), TRUE);
					wchar_t szFaceName[32] = {0};
					bool lbFontJustRegistered = false;
					bool lbFound = false;
					GetDlgItemText(hWnd2, tConsoleFontFace, szFaceName, countof(szFaceName));
					HKEY hk;
					const DWORD nRights = KEY_ALL_ACCESS | WIN3264TEST((IsWindows64() ? KEY_WOW64_64KEY : 0), 0);

					if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Console\\TrueTypeFont",
					                  0, nRights, &hk))
					{
						wchar_t szId[32] = {0}, szFont[255]; DWORD dwLen, dwType;

						for(DWORD i = 0; i <20; i++)
						{
							szId[i] = L'0'; szId[i+1] = 0; wmemset(szFont, 0, 255);

							if (RegQueryValueExW(hk, szId, nullptr, &dwType, reinterpret_cast<LPBYTE>(szFont), &(dwLen = 255*2)))
							{
								if (!RegSetValueExW(hk, szId, 0, REG_SZ, reinterpret_cast<LPBYTE>(szFaceName), (_tcslen(szFaceName)+1)*2))
								{
									lbFontJustRegistered = lbFound = true; // OK, добавили
								}

								break;
							}

							if (lstrcmpi(szFont, szFaceName) == 0)
							{
								lbFound = true; break; // он уже добавлен
							}
						}

						RegCloseKey(hk);
					}

					// Если не удалось (нет прав) то попробовать запустить ConEmuC.exe под админом (Vista+)
					if (!lbFound && gOSVer.dwMajorVersion >= 6)
					{
						wchar_t szCommandLine[MAX_PATH];
						SHELLEXECUTEINFO sei{};
						sei.cbSize = sizeof(sei);
						sei.hwnd = hWnd2;
						sei.fMask = SEE_MASK_NO_CONSOLE|SEE_MASK_NOCLOSEPROCESS|SEE_MASK_NOASYNC;
						sei.lpVerb = L"runas";
						sei.lpFile = WIN3264TEST(gpConEmu->ms_ConEmuC32Full,gpConEmu->ms_ConEmuC64Full);
						swprintf_c(szCommandLine, L" \"/REGCONFONT=%s\"", szFaceName);
						sei.lpParameters = szCommandLine;
						wchar_t szWorkDir[MAX_PATH+1];
						wcscpy_c(szWorkDir, gpConEmu->WorkDir());
						sei.lpDirectory = szWorkDir;
						sei.nShow = SW_SHOWMINIMIZED;
						const BOOL lbRunAsRc = ::ShellExecuteEx(&sei);

						if (!lbRunAsRc)
						{
							//DisplayLastError( IsWindows64()
							//	? L"Can't start ConEmuC64.exe, console font registration failed!"
							//	: L"Can't start ConEmuC.exe, console font registration failed!");
#ifdef WIN64
							DisplayLastError(L"Can't start ConEmuC64.exe, console font registration failed!");
#else
							DisplayLastError(L"Can't start ConEmuC.exe, console font registration failed!");
#endif
						}
						else
						{
							DWORD nWait = WaitForSingleObject(sei.hProcess, 30000); UNREFERENCED_PARAMETER(nWait);
							CloseHandle(sei.hProcess);

							if (gpSetCls->CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName))
							{
								lbFontJustRegistered = lbFound = true; // OK, добавили
							}
						}
					}

					if (lbFound)
					{
						SetFocus(GetDlgItem(hWnd2, tConsoleFontFace));
						EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), FALSE);
						gpSetCls->nConFontError &= ~static_cast<DWORD>(ConFontErr_NonRegistry);

						if (lbFontJustRegistered)
						{
							// Если шрифт только что зарегистрировали - его нельзя использовать до перезагрузки компьютера
							if (lbFontJustRegistered && gpSetCls->sDefaultConFontName[0])
							{
								wcscpy_c(gpSet->ConsoleFont.lfFaceName, gpSetCls->sDefaultConFontName);

								if (CSetDlgLists::SelectString(hWnd2, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName)<0)
									SetDlgItemText(hWnd2, tConsoleFontFace, gpSet->ConsoleFont.lfFaceName);

								EnableWindow(GetDlgItem(hWnd2, bConFontOK), TRUE);
								SetFocus(GetDlgItem(hWnd2, bConFontOK));
							}
						}
						else
						{
							EnableWindow(GetDlgItem(hWnd2, bConFontOK), TRUE);
							SetFocus(GetDlgItem(hWnd2, bConFontOK));
						}
					}
				}

				return 0;
			}
			else if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				PostMessage(hWnd2, (WM_APP+47), wParam, lParam);
			}

			break;
		case(WM_APP+47):

			if (HIWORD(wParam) == CBN_EDITCHANGE || HIWORD(wParam) == CBN_SELCHANGE)
			{
				wchar_t szCreatedFaceName[32] = {0};
				const WORD TB = LOWORD(wParam);
				LOGFONT LF{};
				LF.lfOutPrecision = OUT_TT_PRECIS;
				LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
				LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
				GetDlgItemText(hWnd2, tConsoleFontFace, LF.lfFaceName, countof(LF.lfFaceName));
				LF.lfHeight = GetNumber(hWnd2, tConsoleFontSizeY);

				if (TB != tConsoleFontSizeY)
					LF.lfWidth = GetNumber(hWnd2, tConsoleFontSizeX);

				LF.lfWeight = FW_NORMAL;
				gpSetCls->nConFontError = 0; //ConFontErr_NonSystem|ConFontErr_NonRegistry|ConFontErr_InvalidName;
				const int nIdx = SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(LF.lfFaceName)); //-V103

				if (nIdx < 0)
				{
					gpSetCls->nConFontError = ConFontErr_NonSystem;
				}
				else
				{
					HFONT hf = CreateFontIndirect(&LF);

					if (!hf)
					{
						EnableWindow(GetDlgItem(hWnd2, bConFontOK), FALSE);
						gpSetCls->nConFontError = ConFontErr_InvalidName;
					}
					else
					{
						LPOUTLINETEXTMETRICW lpOutl = gpFontMgr->LoadOutline(nullptr, hf);

						if (!lpOutl)
						{
							// Ошибка
							gpSetCls->nConFontError = ConFontErr_InvalidName;
						}
						else
						{
							wcscpy_c(szCreatedFaceName, reinterpret_cast<wchar_t*>(lpOutl->otmpFamilyName));
							wchar_t temp[10];

							if (TB != tConsoleFontSizeX)
							{
								swprintf_c(temp, L"%i", lpOutl->otmTextMetrics.tmAveCharWidth);
								CSetDlgLists::SelectStringExact(hWnd2, tConsoleFontSizeX, temp);
							}

							if (lpOutl->otmTextMetrics.tmHeight != LF.lfHeight)
							{
								swprintf_c(temp, L"%i", lpOutl->otmTextMetrics.tmHeight);
								if (TB == tConsoleFontSizeY)
								{
									const CEStr lsMsg(L"The created font height differs: ", temp);
									gpSetCls->ShowModifierErrorTip(lsMsg, hWnd2, TB);
								}
								else
								{
									CSetDlgLists::SelectStringExact(hWnd2, tConsoleFontSizeY, temp);
									gpSetCls->ShowModifierErrorTip(L"", hWnd2, TB);
								}
							}
							else
							{
								gpSetCls->ShowModifierErrorTip(L"", hWnd2, TB);
							}

							free(lpOutl); lpOutl = nullptr;

							if (lstrcmpi(szCreatedFaceName, LF.lfFaceName))
								gpSetCls->nConFontError |= ConFontErr_InvalidName;
						}

						DeleteObject(hf);
					}
				}

				if (gpSetCls->nConFontError == 0)
				{
					// Осталось проверить регистрацию в реестре
					wcscpy_c(gpSet->ConsoleFont.lfFaceName, LF.lfFaceName);
					gpSet->ConsoleFont.lfHeight = LF.lfHeight;
					gpSet->ConsoleFont.lfWidth = LF.lfWidth;
					const bool lbRegChecked = CheckConsoleFontRegistry(gpSet->ConsoleFont.lfFaceName);

					if (!lbRegChecked) gpSetCls->nConFontError |= ConFontErr_NonRegistry;

					EnableWindow(GetDlgItem(hWnd2, bConFontOK), lbRegChecked);
					EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), !lbRegChecked);
				}
				else
				{
					EnableWindow(GetDlgItem(hWnd2, bConFontOK), FALSE);
					EnableWindow(GetDlgItem(hWnd2, bConFontAdd2HKLM), FALSE);
				}

				gpSetCls->CreateConFontError(LF.lfFaceName, szCreatedFaceName);
				gpSetCls->ShowConFontErrorTip();
			}

			break;
		case WM_ACTIVATE:

			if (LOWORD(wParam) == WA_INACTIVE)
			{
				gpSetCls->ms_ConFontError.Release();
				gpSetCls->ShowConFontErrorTip();
			}
			else if (gpSetCls->bShowConFontError)
			{
				gpSetCls->bShowConFontError = FALSE;
				gpSetCls->CreateConFontError(nullptr,nullptr);
				gpSetCls->ShowConFontErrorTip();
			}

			break;
	}

	return 0;
}

LPCWSTR CSettings::CreateConFontError(LPCWSTR asReqFont/*=nullptr*/, LPCWSTR asGotFont/*=nullptr*/)
{
	if (!nConFontError)
	{
		return nullptr;
	}

	wchar_t sConFontError[512] = L"";

	SendMessage(gpSetCls->hwndConFontBalloon, TTM_SETTITLE, TTI_WARNING,
		reinterpret_cast<LPARAM>(asReqFont ? asReqFont : gpSet->ConsoleFont.lfFaceName));
	wcscpy_c(sConFontError, L"Console font test failed!\n");
	//wcscat_c(sConFontError, asReqFont ? asReqFont : ConsoleFont.lfFaceName);
	//wcscat_c(sConFontError, L"\n");

	if ((nConFontError & ConFontErr_InvalidName))
	{
		if (asReqFont && asGotFont)
		{
			int nCurLen = _tcslen(sConFontError);
			_wsprintf(sConFontError+nCurLen, SKIPLEN(countof(sConFontError)-nCurLen)
			          L"Requested: %s\nCreated: %s\n", asReqFont , asGotFont);
		}
		else
		{
			wcscat_c(sConFontError, L"Invalid font face name!\n");
		}
	}

	if ((nConFontError & ConFontErr_NonSystem))
		wcscat_c(sConFontError, L"Font is non public or non Unicode\n");

	if ((nConFontError & ConFontErr_NonRegistry))
		wcscat_c(sConFontError, L"Font is not registered for use in console\n");

	sConFontError[_tcslen(sConFontError)-1] = 0;

	ms_ConFontError = sConFontError;

	return ms_ConFontError.ms_Val;
}

int CSettings::EnumConFamCallBack(LPLOGFONT lplf, LPNEWTEXTMETRIC lpntm, DWORD FontType, LPVOID aFontCount)
{
	MCHKHEAP
	HWND hWnd2 = (HWND)aFontCount;

	// Интересуют только TrueType
	if ((FontType & TRUETYPE_FONTTYPE) == 0)
		return TRUE;

	if (lplf->lfFaceName[0] == L'@')
		return TRUE; // Alias?

	// Нельзя использовать шрифты, которые зарегистрированы нами (для ConEmu). Они должны быть системными
	//for (std::vector<RegFont>::iterator iter = gpSetCls->m_RegFonts.begin(); iter != gpSetCls->m_RegFonts.end(); ++iter)
	for (INT_PTR j = 0; j < gpFontMgr->m_RegFonts.size(); ++j)
	{
		const RegFont* iter = &(gpFontMgr->m_RegFonts[j]);

		if (!iter->bAlreadyInSystem &&
		        lstrcmpi(iter->szFontName, lplf->lfFaceName) == 0)
		{
			return TRUE;
		}
	}

	// PAN_PROP_MONOSPACED - не дает правильного результата. Например 'MS Mincho' заявлен как моноширинный,
	// но совсем таковым не является. Кириллица у него дофига какая...
	// И только моноширинные!
	DWORD bAlmostMonospace = gpFontMgr->IsAlmostMonospace(lplf->lfFaceName, (LPTEXTMETRIC)lpntm /*lpntm->tmMaxCharWidth, lpntm->tmAveCharWidth, lpntm->tmHeight*/) ? 1 : 0;

	if (!bAlmostMonospace)
		return TRUE;

	// Проверяем, реальное ли это имя. Или просто алиас?
	LOGFONT LF = {0};
	LF.lfHeight = 10; LF.lfWidth = 0; LF.lfWeight = 0; LF.lfItalic = 0;
	LF.lfOutPrecision = OUT_TT_PRECIS;
	LF.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	LF.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	wcscpy_c(LF.lfFaceName, lplf->lfFaceName);
	HFONT hf = CreateFontIndirect(&LF);

	if (!hf) return TRUE;  // не получилось создать

	LPOUTLINETEXTMETRICW lpOutl = gpFontMgr->LoadOutline(nullptr, hf);

	if (!lpOutl) return TRUE;  // Ошибка получения параметров шрифта

	if (lpOutl->otmPanoseNumber.bProportion != PAN_PROP_MONOSPACED  // шрифт не заявлен как моноширинный
	        || lstrcmpi((wchar_t*)lpOutl->otmpFamilyName, LF.lfFaceName)) // или алиас
	{
		free(lpOutl);
		return TRUE; // следующий шрифт
	}

	free(lpOutl); lpOutl = nullptr;

	// Сравниваем с текущим, выбранным в настройке
	if (lstrcmpi(LF.lfFaceName, gpSet->ConsoleFont.lfFaceName) == 0)
		gpSetCls->nConFontError &= ~(DWORD)ConFontErr_NonSystem;

	INT_PTR nIdx = -1;
	if (hWnd2)
	{
		if (SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_FINDSTRINGEXACT, -1, reinterpret_cast<LPARAM>(LF.lfFaceName))==-1)
		{
			nIdx = SendDlgItemMessage(hWnd2, tConsoleFontFace, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(LF.lfFaceName)); //-V103
		}
	}
	UNREFERENCED_PARAMETER(nIdx);

	if (gpSetCls->sDefaultConFontName[0] == 0)
	{
		if (CheckConsoleFontRegistry(LF.lfFaceName))
			wcscpy_c(gpSetCls->sDefaultConFontName, LF.lfFaceName);
	}

	MCHKHEAP
	return TRUE;
	UNREFERENCED_PARAMETER(lpntm);
}

bool CSettings::isDialogMessage(MSG &Msg)
{
	if (((Msg.message == WM_KEYDOWN) || (Msg.message == WM_KEYUP))
		&& ((Msg.wParam == VK_UP) || (Msg.wParam == VK_DOWN)))
	{
		// Is it button? WM_GETDLGCODE?
		wchar_t szClass[64] = L""; GetClassName(Msg.hwnd, szClass, countof(szClass));
		if (wcscmp(szClass, L"Button") == 0)
		{
			CSetPgBase* pg = GetActivePageObj();
			if (pg && pg->SelectNextItem((Msg.wParam == VK_DOWN), (Msg.message == WM_KEYDOWN)))
			{
				return true;
			}
		}
	}

	if (IsDialogMessage(ghOpWnd, &Msg))
	{
		return true;
	}

	return false;
}

const ConEmuSetupPages* CSettings::GetPageData(TabHwndIndex nPage)
{
	if (!m_Pages)
	{
		_ASSERTE(m_Pages!=nullptr && "Pages list must be initialized already!");
		return nullptr;
	}

	const ConEmuSetupPages* pData = nullptr;

	_ASSERTE((thi_General == (TabHwndIndex)0) && (nPage >= thi_General) && (nPage < thi_Last));

	if ((nPage >= thi_General) && (nPage < thi_Last))
	{
		if (m_Pages[nPage].PageIndex == nPage)
		{
			pData = &(m_Pages[nPage]);
		}
		else
		{
			_ASSERTE(m_Pages[nPage].PageIndex == nPage);

			for (const ConEmuSetupPages* p = m_Pages; p->DialogID; p++)
			{
				if (p->PageIndex == nPage)
				{
					pData = p;
					break;
				}
			}
		}
	}

	if (!pData)
	{
		_ASSERTE(FALSE && "Invalid nPage identifier!");
	}

	return pData;
}

TabHwndIndex CSettings::GetPageIdByDialogId(UINT DialogID)
{
	TabHwndIndex pageId = thi_Last;

	if (!ghOpWnd || !m_Pages)
	{
		_ASSERTE(ghOpWnd && m_Pages);
	}
	else
	{
		for (const ConEmuSetupPages* p = m_Pages; p->DialogID; p++)
		{
			if (p->DialogID == DialogID)
			{
				pageId = p->PageIndex;
				break;
			}
		}
	}

	return pageId;
}

bool CSettings::ActivatePage(TabHwndIndex showPage)
{
	if (showPage != thi_Last)
	{
		for (size_t i = 0; m_Pages[i].DialogID; i++)
		{
			if (m_Pages[i].PageIndex == showPage)
			{
				//PostMessage(GetDlgItem(ghOpWnd, tvSetupCategories), TVM_SELECTITEM, TVGN_CARET, reinterpret_cast<LPARAM>(gpSetCls)->m_Pages[i].hTI);
				SelectTreeItem(GetDlgItem(ghOpWnd, tvSetupCategories), m_Pages[i].hTI, true);
				return true;;
			}
		}
	}

	return false;
}

HWND CSettings::GetActivePage()
{
	const ConEmuSetupPages* p = (m_LastActivePageId != thi_Last) ? GetPageData(m_LastActivePageId) : nullptr;
	if (!p)
		return nullptr;
	if (!p->hPage || !IsWindowVisible(p->hPage))
		return nullptr;
	return p->hPage;
}

CSetPgBase* CSettings::GetActivePageObj()
{
	const ConEmuSetupPages* p = (m_LastActivePageId != thi_Last) ? GetPageData(m_LastActivePageId) : nullptr;
	if (!p)
		return nullptr;
	if (!p->hPage || !IsWindowVisible(p->hPage))
		return nullptr;
	return p->pPage;
}

LPCWSTR CSettings::GetActivePageWiki(CEStr& lsWiki)
{
	lsWiki.Release();
	const ConEmuSetupPages* p = (m_LastActivePageId != thi_Last) ? GetPageData(m_LastActivePageId) : nullptr;
	if (!p)
		return nullptr;
	lsWiki = CEStr(CEWIKIBASE, p->wikiPage);
	return lsWiki;
}

HWND CSettings::GetPage(TabHwndIndex nPage)
{
	const ConEmuSetupPages* p = GetPageData(nPage);
	if (!p)
		return nullptr;
	return p->hPage;
}

CSetPgBase* CSettings::GetPageObj(TabHwndIndex nPage)
{
	const ConEmuSetupPages* p = GetPageData(nPage);
	if (!p)
		return nullptr;
	return p->pPage;
}

TabHwndIndex CSettings::GetPageId(HWND hPage, ConEmuSetupPages** pp)
{
	TabHwndIndex pgId = thi_Last;

	if (ghOpWnd && m_Pages && hPage)
	{
		for (ConEmuSetupPages* p = m_Pages; p->DialogID; p++)
		{
			if (p->hPage == hPage)
			{
				pgId = p->PageIndex;
				if (pp)
					*pp = p;
				break;
			}
		}
	}

	return pgId;
}

TabHwndIndex CSettings::GetPageId(HWND hPage)
{
	return GetPageId(hPage, nullptr);
}

void CSettings::ClearPages()
{
	if (!m_Pages)
	{
		_ASSERTE(m_Pages);
		return;
	}

	for (ConEmuSetupPages *p = m_Pages; p->DialogID; p++)
	{
		SafeDelete(p->pPage);
		p->hPage = nullptr;
	}

	if (mp_DpiAware)
		mp_DpiAware->Detach();

	SafeDelete(mp_ImgBtn);
}

int CSettings::GetDialogDpi()
{
	if (mp_DpiAware)
		return mp_DpiAware->GetCurDpi().Ydpi;
	return _dpi.Ydpi;
}
