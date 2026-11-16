
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

#undef USE_PALETTE_POPUP_COLORS

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"

#ifdef _DEBUG
#include <tlhelp32.h>
#endif
#include "../common/clink.h"
#include "../common/CEHandle.h"
#include "../common/DefTermBase.h"
#include "../common/Monitors.h"
#include "../common/WRegistry.h"
#include "../ConEmuPlugin/FarDefaultMacros.h"

#include "Background.h"
#include "CmdHistory.h"
#include "ConEmu.h"
#include "ConEmuStart.h"
#include "HotkeyList.h"
#include "Inside.h"
#include "LngRc.h"
#include "Macro.h"
#include "Options.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "SetCmdTask.h"
#include "SetColorPalette.h"
#include "Status.h"
#include "TabBar.h"
#include "TrayIcon.h"
#include "VConGroup.h"

using ConEmu::PaletteColors;

TODO("Convert Settings::mpc_CharAltFontRanges to binary ranges?");
TODO("Load/Save Settings::bHideDisabledTabs?");

//#define DEBUGSTRFONT(s) DEBUGSTR(s)

//#define COUNTER_REFRESH 5000
//#define BACKGROUND_FILE_POLL 5000

//#define RASTER_FONTS_NAME L"Raster Fonts"
//SIZE szRasterSizes[100] = {{0,0}}; // {{16,8},{6,9},{8,9},{5,12},{7,12},{8,12},{16,12},{12,16},{10,18}};
//const wchar_t szRasterAutoError[] = L"Font auto size is not allowed for a fixed raster font size. Select 'Terminal' instead of '[Raster Fonts ...]'";

//#define TEST_FONT_WIDTH_STRING_EN L"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
//#define TEST_FONT_WIDTH_STRING_RU L"АБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"

//#define FAILED_FONT_TIMERID 101
//#define FAILED_FONT_TIMEOUT 3000
//#define FAILED_CONFONT_TIMEOUT 30000

//const uint8_t chSetsNums[] = {0, 178, 186, 136, 1, 238, 134, 161, 177, 129, 130, 77, 255, 204, 128, 2, 222, 162, 163};
//const char *ChSets[] = {"ANSI", "Arabic", "Baltic", "Chinese Big 5", "Default", "East Europe",
//                        "GB 2312", "Greek", "Hebrew", "Hangul", "Johab", "Mac", "OEM", "Russian", "Shiftjis",
//                        "Symbol", "Thai", "Turkish", "Vietnamese"
//                       };
//const WORD HostkeyCtrlIds[] = {cbHostWin, cbHostApps, cbHostLCtrl, cbHostRCtrl, cbHostLAlt, cbHostRAlt, cbHostLShift, cbHostRShift};
//int upToFontHeight=0;
//HWND ghOpWnd=nullptr;

#ifdef _DEBUG
#define HEAPVAL HeapValidate(GetProcessHeap(), 0, nullptr);
#else
#define HEAPVAL
#endif

#define DEFAULT_FADE_LOW 0
#define DEFAULT_FADE_HIGH 0xD0

#define MAX_SPLITTER_SIZE 32

// That is positive value for LF.lfHeight
#define DEF_INSIDESIZEY_P 10
#define DEF_FONTSIZEY_P   16
#define DEF_TABFONTY_P    16
#define DEF_STATUSFONTY_P 14
// That is positive value for LF.lfHeight
#define DEF_INSIDESIZEY_U 9
#define DEF_FONTSIZEY_U   14
#define DEF_TABFONTY_U    13
#define DEF_STATUSFONTY_U 12

struct CONEMUDEFCOLORS
{
	const wchar_t* pszTitle;
	PaletteColors dwDefColors;
	BYTE  nIndexes[4]; // Text/Back/PopupText/PopupBack
	struct {
	bool  isBackground;
	DWORD clrBackground;
	} Bk;

	bool isIndexes() const { return (nIndexes[0] && nIndexes[1] && nIndexes[2] && nIndexes[3]); };
};

const wchar_t gsDefaultColorScheme[64] = L"<ConEmu>";

const CONEMUDEFCOLORS DefColors[] =
{
	{ L"<Default Windows scheme>", {
			0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
		}
	},
	{ L"<Babun>", {
			0x001e1d1b, 0x00d28b26, 0x0014b482, 0x00d6c256, 0x007226f9, 0x00fe548c, 0x001f97fd, 0x00c6cccc,
			0x00545350, 0x00e3ad62, 0x0046ebb7, 0x00e5d894, 0x009559ff, 0x00fea0bf, 0x006cedfe, 0x00f2f8f8
		}
	},
	{ L"<Base16>", {
			0x00181818, 0x00383838, 0x00d8d8d8, 0x004669a1, 0x005696dc, 0x00282828, 0x00b8b8b8, 0x00e8e8e8,
			0x00585858, 0x00c2af7c, 0x006cb5a1, 0x00b9c186, 0x004246ab, 0x00af8bba, 0x0088caf7, 0x00f8f8f8
		}
	},
	{ L"<Cobalt2>", {
			0x00000000, 0x00ff5555, 0x003bd01d, 0x00fae36a, 0x000000ff, 0x00ff55ff, 0x0009c8ed, 0x00ffffff,
			0x00555555, 0x00d26014, 0x0021de38, 0x00bbbb00, 0x00170ef4, 0x005d00ff, 0x000ae5ff, 0x00bbbbbb
		}, {}, {true, 0x00240A30}
	},
	{ L"<ConEmu>", {
			0x00362b00, 0x00423607, 0x00808000, 0x00a48231, 0x00164bcb, 0x00b6369c, 0x00009985, 0x00d5e8ee,
			0x00a1a193, 0x00d28b26, 0x0036b64f, 0x0098a12a, 0x002f32dc, 0x008236d3, 0x000089b5, 0x00e3f6fd
		}
	},
	{ L"<Gamma 1>", {
			0x00000000, 0x00960000, 0x0000aa00, 0x00aaaa00, 0x000000aa, 0x00800080, 0x0000aaaa, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
		}
	},
	{ L"<Monokai>", {
			0x00222827, 0x009e5401, 0x0004aa74, 0x00a6831a, 0x003403a7, 0x009c5689, 0x0049b6b6, 0x00cacaca,
			0x007c7c7c, 0x00f58303, 0x0006d08d, 0x00e5c258, 0x004b04f3, 0x00b87da8, 0x0081cccc, 0x00ffffff
		}
	},
	{ L"<Murena scheme>", {
			0x00000000, 0x00644100, 0x00008000, 0x00808000, 0x00000080, 0x00800080, 0x00008080, 0x00c0c0c0,
			0x00808080, 0x00ff0000, 0x0076c587, 0x00ffff00, 0x00004bff, 0x00d78ce6, 0x0000ffff, 0x00ffffff
		}
	},
	{ L"<PowerShell>", {
			0x00000000, 0x00800000, 0x00008000, 0x00808000, 0x00000080, 0x00562401, 0x00F0EDEE, 0x00C0C0C0,
			0x00808080, 0x00ff0000, 0x0000FF00, 0x00FFFF00, 0x000000FF, 0x00FF00FF, 0x0000FFFF, 0x00FFFFFF
		}, {6,5,3,15}
	},
	{ L"<Solarized>", {
			0x00362b00, 0x00423607, 0x00756e58, 0x00837b65, 0x00164bcb, 0x00c4716c, 0x00969483, 0x00d5e8ee,
			0x00a1a193, 0x00d28b26, 0x00009985, 0x0098a12a, 0x002f32dc, 0x008236d3, 0x000089b5, 0x00e3f6fd
		}
	},
	{ L"<Solarized Git>", {
			0x00362b00, 0x00423607, 0x0098a12a, 0x00837b65, 0x00164bcb, 0x00756e58, 0x00969483, 0x00e3f6fd,
			0x00d5e8ee, 0x00d28b26, 0x00009985, 0x00c4716c, 0x002f32dc, 0x008236d3, 0x000089b5, 0x00a1a193
		}
	},
	{ L"<Solarized (Luke Maciak)>", {
			0x00423607, 0x00d28b26, 0x00009985, 0x000089b5, 0x002f32dc, 0x008236d3, 0x0098a12a, 0x00d5e8ee,
			0x00362b00, 0x00aaa897, 0x00756e58, 0x00837b65, 0x00004ff2, 0x00c4716c, 0x00a1a193, 0x00e3f6fd
		}
	},
	{ L"<Solarized (John Doe)>", {
			0x00362b00, 0x00423607, 0x00756e58, 0x00837b65, 0x002f32dc, 0x00c4716c, 0x00164bcb, 0x00d5e8ee,
			0x00a1a193, 0x00d28b26, 0x00009985, 0x0098a12a, 0x00969483, 0x008236d3, 0x000089b5, 0x00e3f6fd
		}
	},
	{ L"<Solarized Light>", {
			0x00d5e8ee, 0x00e3f6fd, 0x00756e58, 0x00837b65, 0x00164bcb, 0x00c4716c, 0x00969483, 0x00423607,
			0x00a1a193, 0x00d28b26, 0x00009985, 0x0098a12a, 0x002f32dc, 0x008236d3, 0x000089b5, 0x00362b00
		}
	},
	{ L"<SolarMe>", {
			// Standard <Solarized> has too many monotones
			// which made it unpleasant with some applications
			// <SolarMe> has 3 monotones changed to scaled colors
			0x00362b00, 0x00423607, 0x0098a12a, 0x00756e58, 0x00164bcb, 0x00542388, 0x00005875, 0x00d5e8ee,
			0x00a1a193, 0x00c4716c, 0x00009985, 0x00d28b26, 0x002f32dc, 0x008236d3, 0x000089b5, 0x00e3f6fd
		}
	},
	{ L"<Standard VGA>", {
			0x00000000, 0x00aa0000, 0x0000aa00, 0x00aaaa00, 0x000000aa, 0x00aa00aa, 0x000055aa, 0x00aaaaaa,
			0x00555555, 0x00ff5555, 0x0055ff55, 0x00ffff55, 0x005555ff, 0x00ff55ff, 0x0055ffff, 0x00ffffff
		}
	},
	{ L"<tc-maxx>", {
			0x00000000, RGB(11,27,59), RGB(0,128,0), RGB(0,90,135), RGB(106,7,28), RGB(128,0,128), RGB(128,128,0), RGB(40,150,177),
			RGB(128,128,128), RGB(0,0,255), RGB(0,255,0), RGB(0,215,243), RGB(190,7,23), RGB(255,0,255), RGB(255,255,0), RGB(255,255,255)
		}
	},
	{ L"<Terminal.app>", {
			0x00000000, 0x00e12e49, 0x0024bc25, 0x00c8bb33, 0x002136c2, 0x00d338d3, 0x0027adad, 0x00cdcccb,
			0x00838381, 0x00ff3358, 0x0022e731, 0x00f0f014, 0x001f39fc, 0x00f835f9, 0x0023ecea, 0x00ebebe9
		}
	},
	{ L"<Tomorrow>", {
			0x00ffffff, 0x00573821, 0x00004638, 0x004f4c1f, 0x00141464, 0x00542c44, 0x00005b75, 0x004c4d4d,
			0x008c908e, 0x00ae7142, 0x00008c71, 0x009f993e, 0x002928c8, 0x00a85989, 0x0000b7ea, 0x00000000
		}
	},
	{ L"<Tomorrow Night>", {
			0x00211f1d, 0x005f5140, 0x00345e5a, 0x005b5d45, 0x00333366, 0x005d4a59, 0x003a6378, 0x00c6c8c5,
			0x00969896, 0x00bea281, 0x0068bdb5, 0x00b7ba8a, 0x006666cc, 0x00bb94b2, 0x0074c6f0, 0x00ffffff
		}
	},
	{ L"<Tomorrow Night Blue>", {
			0x00512400, 0x00806d5d, 0x00547868, 0x0080801a, 0x00524e80, 0x00805d75, 0x00567780, 0x00ffffff,
			0x00b78572, 0x00ffdabb, 0x00a9f1d1, 0x00ffff35, 0x00a49dff, 0x00ffbbeb, 0x00adeeff, 0x00ffffff
		}
	},
	{ L"<Tomorrow Night Bright>", {
			0x00000000, 0x006d533d, 0x0025655c, 0x00586038, 0x0029276a, 0x006c4b61, 0x00236273, 0x00eaeaea,
			0x00969896, 0x00daa67a, 0x004acab9, 0x00b1c070, 0x00534ed5, 0x00d897c3, 0x0047c5e7, 0x00ffffff
		}
	},
	{ L"<Tomorrow Night Eighties>", {
			0x002d2d2d, 0x00664c33, 0x00386638, 0x00666633, 0x00383679, 0x00664c66, 0x004c6680, 0x00cccccc,
			0x00999999, 0x00cc9966, 0x0099cc99, 0x00cccc66, 0x007a77f2, 0x00cc99cc, 0x0099ccff, 0x00ffffff
		}
	},
	{ L"<Twilight>", {
			0x00141414, 0x004c6acf, 0x00619083, 0x0069a8ce, 0x00a68775, 0x009d859b, 0x00b3a605, 0x00d7d7d7,
			0x00666666, 0x00a68775, 0x004da459, 0x00e6e60a, 0x00520ad8, 0x008b00e6, 0x003eeee1, 0x00e6e6e6
		}
	},
	{ L"<Ubuntu>", { // Need to set up backround "picture" with colorfill #300A24
			0x0036342e, 0x00a46534, 0x00069a4e, 0x009a9806, 0x000000cc, 0x007b5075, 0x0000a0c4, 0x00cfd7d3,
			0x00535755, 0x00cf9f72, 0x0034e28a, 0x00e2e234, 0x002929ef, 0x00a87fad, 0x004fe9fc, 0x00eceeee
		}, {15, 0, 0, 0}, {true, 0x00240A30}
	},
	{ L"<xterm>", {
			0x00000000, 0x00ee0000, 0x0000cd00, 0x00cdcd00, 0x000000cd, 0x00cd00cd, 0x0000cdcd, 0x00e5e5e5,
			0x007f7f7f, 0x00ff5c5c, 0x0000ff00, 0x00ffff00, 0x000000ff, 0x00ff00ff, 0x0000ffff, 0x00ffffff
		}
	},
	{ L"<Zenburn>", {
			0x003f3f3f, 0x00af6464, 0x00008000, 0x00808000, 0x00232333, 0x00aa50aa, 0x0000dcdc, 0x00ccdcdc,
			0x008080c0, 0x00ffafaf, 0x007f9f7f, 0x00d3d08c, 0x007071e3, 0x00c880c8, 0x00afdff0, 0x00ffffff
		},
	},
};




#define SetThumbColor(s,rgb,idx,us) { (s).RawColor = 0; (s).ColorRGB = rgb; (s).ColorIdx = idx; (s).UseIndex = us; }
#define SetThumbSize(s,sz,x1,y1,x2,y2,ls,lp,fn,fs) { \
		(s).nImgSize = sz; (s).nSpaceX1 = x1; (s).nSpaceY1 = y1; (s).nSpaceX2 = x2; (s).nSpaceY2 = y2; \
		(s).nLabelSpacing = ls; (s).nLabelPadding = lp; wcscpy_c((s).sFontName,fn); (s).nFontHeight=fs; }
#define ThumbLoadSet(s,n) { \
		reg->Load(L"PanView." s L".ImgSize", n.nImgSize); \
		reg->Load(L"PanView." s L".SpaceX1", n.nSpaceX1); \
		reg->Load(L"PanView." s L".SpaceY1", n.nSpaceY1); \
		reg->Load(L"PanView." s L".SpaceX2", n.nSpaceX2); \
		reg->Load(L"PanView." s L".SpaceY2", n.nSpaceY2); \
		reg->Load(L"PanView." s L".LabelSpacing", n.nLabelSpacing); \
		reg->Load(L"PanView." s L".LabelPadding", n.nLabelPadding); \
		reg->Load(L"PanView." s L".FontName", n.sFontName, countof(n.sFontName)); \
		reg->Load(L"PanView." s L".FontHeight", n.nFontHeight); }
#define ThumbSaveSet(s,n) { \
		reg->Save(L"PanView." s L".ImgSize", n.nImgSize); \
		reg->Save(L"PanView." s L".SpaceX1", n.nSpaceX1); \
		reg->Save(L"PanView." s L".SpaceY1", n.nSpaceY1); \
		reg->Save(L"PanView." s L".SpaceX2", n.nSpaceX2); \
		reg->Save(L"PanView." s L".SpaceY2", n.nSpaceY2); \
		reg->Save(L"PanView." s L".LabelSpacing", n.nLabelSpacing); \
		reg->Save(L"PanView." s L".LabelPadding", n.nLabelPadding); \
		reg->Save(L"PanView." s L".FontName", n.sFontName); \
		reg->Save(L"PanView." s L".FontHeight", n.nFontHeight); }




Settings::Settings()
{
	//gpSet = this; // -- нельзя. Settings может использоваться как копия настроек (!= gpSet)

	// #SETTINGS Get rid of ResetSettings in ctor, instead move all defaults to header
	ResetSettings();

	// Умолчания устанавливаются в CSettings::CSettings
	//-- // Теперь установим умолчания настроек
	//-- InitSettings();
}

void Settings::ResetSettings()
{
	#ifdef OPTION_TYPES_USED
	PRAGMA_ERROR("memset not allowed for classes!");

	#else

	// #SETTINGS While Settings is POD it's valid
	memset(this, 0, sizeof(*this));

	#endif
}

void Settings::ReleasePointers()
{
	SafeFree(sTabCloseMacro);

	SafeFree(sSafeFarCloseMacro);

	SafeFree(sSaveAllMacro);

	SafeFree(sRClickMacro);

	SafeFree(psStartSingleApp);
	SafeFree(psStartTasksFile);
	SafeFree(psStartTasksName);
	//SafeFree(psCurCmd);
	SafeDelete(pHistory); isSaveCmdHistory = true;
	SafeFree(psHistoryLocation);
	SafeFree(psDefaultTerminalApps);

	SafeDelete(psAnsiAllowed);
	SafeFree(pszAnsiLog);

	SafeFree(pszCTSIntelligentExceptions);
	SafeFree(sFarGotoEditor);
	SafeFree(pszTabSkipWords);
	SafeDelete(psEnvironmentSet);

	FreeCmdTasks();
	FreeStartupTask();
	CmdTaskCount = 0;

	FreePalettes();
	PaletteCount = 0;

	FreeApps();
	AppStd.FreeApps();

	FreeProgresses();
	ProgressesCount = 0;

	UpdSet.FreePointers();
	FindOptions.text.Release();
}

Settings::~Settings()
{
	ReleasePointers();
}

void Settings::InitSettings()
{
	//TODO: Has no effect - called from CSettings ctor
	if (gpConEmu) gpConEmu->LogString(gpConEmu->IsResetBasicSettings() ? L"Settings::InitSettings(basic)" : L"Settings::InitSettings()");
	MCHKHEAP

	// Освободить память, т.к. функция может быть вызвана из окна интерфейса настроек
	ReleasePointers();

	// Сброс переменных
	ResetSettings();

	wcscpy_c(Language, L"en");
	nStartCreateDelay = RUNQUEUE_CREATE_LAG_DEF;
	isAutoRegisterFonts = true;
	nHostkeyNumberModifier = VK_LCONTROL; //TestHostkeyModifiers(nHostkeyNumberModifier);
	nHostkeyArrowModifier = VK_LWIN; //TestHostkeyModifiers(nHostkeyArrowModifier);
	isSingleInstance = false;
	isShowHelpTooltips = true;
	mb_isMulti = true;
	isMultiShowButtons = true;
	isMultiShowSearch = true;
	isNumberInCaption = false;
	//vmMultiNew = 'W' | (nMultiHotkeyModifier << 8);
	//vmMultiNext = 'Q' | (nMultiHotkeyModifier << 8);
	//vmMultiRecreate = 192/*VK_тильда*/ | (nMultiHotkeyModifier << 8);
	//vmMultiBuffer = 'A' | (nMultiHotkeyModifier << 8);
	//vmMinimizeRestore = 'C' | (nMultiHotkeyModifier << 8);
	//vmMultiClose = VK_DELETE | (nMultiHotkeyModifier << 8);
	//vmMultiCmd = 'X' | (nMultiHotkeyModifier << 8);
	isMultiAutoCreate = false;
	isMultiLeaveOnClose = 0;
	isMultiHideOnClose = 0;
	isMultiIterate = true;
	isMultiMinByEsc = 2; isMapShiftEscToEsc = true; // isMapShiftEscToEsc used only when isMultiMinByEsc==1 and only for console apps
	isMultiNewConfirm = false;
	isMultiDupConfirm = true;
	isMultiDetachConfirm = true;
	nCloseConfirmFlags = cc_Running;
	isResetTerminalConfirm = true;
	isUseWinNumber = true; isUseWinArrows = false; isUseWinTab = false;
	nSplitWidth = nSplitHeight = 4;
	//nSplitClr1 = nSplitClr2 = RGB(160,160,160);
	m_isKeyboardHooks = 0;
	isFARuseASCIIsort = false; isFixAltOnAltTab = false; isShellNoZoneCheck = false;
	isUseAltGrayPlus = true;
	isFadeInactive = true; mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH; mb_FadeInitialized = false;
	mn_LastFadeSrc = mn_LastFadeDst = -1;
	nMainTimerElapse = 10;
	nMainTimerInactiveElapse = 1000;
	nAffinity = 0; // 0 - don't change default affinity
	isSkipFocusEvents = false;
	//isSendAltEnter = isSendAltSpace = isSendAltF9 = false;
	isSendAltTab = isSendAltEsc = isSendAltPrintScrn = isSendPrintScrn = isSendCtrlEsc = false;
	isMonitorConsoleLang = 3; // bitmask. 1 - follow up console HKL (e.g. after XLat in Far Manager), 2 - use one HKL for all tabs
	isDynamicBufferHeight = true;
	DefaultBufferHeight = LONGOUTPUTHEIGHT_MAX;
	AutoBufferHeight = true;
	UseScrollLock = true;
	isSaveCmdHistory = true;
	ComSpec.AddConEmu2Path = CEAP_AddAll;
	{
		ComSpec.isAllowUncPaths = FALSE;
		// Load defaults from windows registry (Command Processor settings)
		SettingsRegistry UncChk;
		if (UncChk.OpenKey(HKEY_CURRENT_USER, L"Software\\Microsoft\\Command Processor", KEY_READ))
		{
			DWORD DisableUNCCheck = 0;
			if (UncChk.Load(L"DisableUNCCheck", (LPBYTE)&DisableUNCCheck, sizeof(DisableUNCCheck)))
			{
				ComSpec.isAllowUncPaths = (DisableUNCCheck == 1);
			}
		}
	}

	AutoReloadEnvironment = true;
	psEnvironmentSet = new CEStr(
		L"set PATH=%ConEmuBaseDir%\\Scripts;%PATH%\r\n"
		L"\r\n"
		L"# set ConEmuPromptNames=NO\r\n" // created after gh-1570
		L"# set ConEmuPromptNL=NO\r\n"
		L"# set ConEmuSudoSplit=NO\r\n"
		L"# set ConEmuSudoConfirm=NO\r\n"
		L"\r\n"
		L"# set LANG=ru_RU.utf8\r\n"
		L"# set LANG=ru_RU.CP1251\r\n"
		L"\r\n"
		L"# alias cdc=cd /d \"%ConEmuDir%\"\r\n"
		L"# alias cd~=cd /d \"%UserProfile%\"\r\n"
		L"# alias gl=git-log\r\n"
		L"# alias glb=git-log --branches --date-order\r\n"
		);

	bool bIsDbcs = IsWinDBCS();

	/* ********** Font initialization begins ********** */
	{
	// Specially left it in ‘false’ state because that was the defaults in old builds
	// Also use DEF_FONTSIZEY_P, DEF_STATUSFONTY_P, DEF_TABFONTY_P for the same reason
	// Will be changed in ‘InitVanillaFontSettings’ if settings are ‘new’
	FontUseUnits = false;

	// Font initialization (all fields must be already zeroed)
	_ASSERTE(!FontSizeY && !FontSizeX && !FontSizeX2 && !FontSizeX3 && !FontUseDpi && !FontUseUnits);
	// Let take into account monitor dpi
	FontUseDpi = true;
	// Font height in pixels
	FontSizeY = DEF_FONTSIZEY_P; // dpi must not be embedded into font height!
	//-- Issue 577: Для иероглифов - сделаем "пошире", а то глифы в консоль не влезут...
	//-- пошире не будем. DBCS консоль хитрая, на каждый иероглиф отводится 2 ячейки
	//-- "не влезть" может только если выполнить "chcp 65001", что врядли, а у если надо - руками пусть ставят
	FontSizeX3 = 0; // bIsDbcs ? 15 : 0;
	// Bold/Italic
	isBold = false;
	isItalic = false;
	// Charset. That may be very important for raster fonts.
	mn_LoadFontCharSet = DEFAULT_CHARSET;
	// Antialias style, the defaults
	BOOL bClearType = FALSE; if (SystemParametersInfo(SPI_GETCLEARTYPE, 0, &bClearType, 0) && bClearType)
		mn_AntiAlias = CLEARTYPE_NATURAL_QUALITY;
	else
		mn_AntiAlias = ANTIALIASED_QUALITY;
	// Some resets?
	inFont[0] = inFont2[0] = 0;

	// StatusBar fonts
	wcscpy_c(sStatusFontFace, gsDefMUIFont); nStatusFontCharSet = ANSI_CHARSET;
	nStatusFontHeight = DEF_STATUSFONTY_P; // dpi must not be embedded into font height!

	// TabBar fonts
	wcscpy_c(sTabFontFace, gsDefMUIFont); nTabFontCharSet = ANSI_CHARSET;
	nTabFontHeight = DEF_TABFONTY_P; // dpi must not be embedded into font height!
	}
	/* ********** Font initialization ends ********** */

	isCompressLongStrings = true;

	isTryToCenter = true;
	nCenterConsolePad = 4;
	isAlwaysShowScrollbar = 2;
	nScrollBarAppearDelay = 100;
	nScrollBarDisappearDelay = 1000;

	//Issue 577: Для иероглифов - по умолчанию отключим моноширинность
	//150311 - proportial is not required for DBCS anymore
	isMonospace = 1; // bIsDbcs ? 0 : 1;

	mb_MinToTray = false;
	mb_AlwaysShowTrayIcon = false;

	ConsoleFont.lfHeight = 5;
	ConsoleFont.lfWidth = 3;
	wcscpy_c(ConsoleFont.lfFaceName, gsDefConFont);

	// Standard <Solarized> has too many monotones
	// which made it unpleasant with a lot of tools, especially
	// those which were designed for 8-color consoles (cygwin)
	if (GetDefColors(L"<ConEmu>", Colors))
	{
		// Initialized
	}
	else
	{
		// Default palette MUST exists in our list!
		_ASSERTE(FALSE && "Must not get here");
		GetWindowsColors(Colors);
	}

	isTrueColorer = true; // включим по умолчанию, ибо Far3

	isVividColors = true;

	AppStd.nTextColorIdx = AppStd.nBackColorIdx = CEDEF_BackColorAuto/*16*/;
	AppStd.nPopTextColorIdx = AppStd.nPopBackColorIdx = CEDEF_BackColorAuto/*16*/;
	AppStd.isExtendFonts = false;
	AppStd.nFontNormalColor = CEDEF_FontNormalColor/*1*/;
	AppStd.nFontBoldColor = CEDEF_FontBoldColor/*12*/;
	AppStd.nFontItalicColor = CEDEF_FontItalicColor/*13*/;
	{
		_ASSERTE(sizeof(AppStd.CursorActive) == sizeof(DWORD));
		AppStd.CursorActive.Raw = 0; // Сброс
		AppStd.CursorActive.CursorType = cur_Vert; // 0 - Horz, 1 - Vert, 2 - Hollow-block
		AppStd.CursorActive.isBlinking = true;
		AppStd.CursorActive.isColor = true;
		//AppStd.CursorActive.isCursorBlockInactive = true;
		AppStd.CursorActive.isFixedSize = false;
		AppStd.CursorActive.FixedSize = 25; // Percents
		AppStd.CursorActive.MinSize = CURSORSIZEPIX_STD/*2*/; // Pixels

		AppStd.CursorInactive.Raw = AppStd.CursorActive.Raw; // копирование
		AppStd.CursorInactive.Used = true;
		AppStd.CursorInactive.CursorType = cur_Rect;
		AppStd.CursorInactive.isBlinking = false;
	}
	AppStd.isCTSDetectLineEnd = true;
	AppStd.isCTSBashMargin = false;
	AppStd.isCTSTrimTrailing = 2;
	AppStd.isCTSEOL = 0;
	AppStd.isCTSShiftArrowStart = true;
	AppStd.isPasteAllLines = true;
	AppStd.isPosixAllLines = pxm_Auto;
	AppStd.isPasteFirstLine = true;
	AppStd.isPosixFirstLine = pxm_Auto;

	// 0 - off, 1 - force, 2 - try to detect "ReadConsole" (don't use 2 in bash)
	AppStd.isCTSClickPromptPosition = 2; // Кликом мышки позиционировать курсор в Cmd Prompt (cmd.exe, Powershell.exe, ...) + vkCTSVkPromptClk
	AppStd.isCTSDeleteLeftWord = 2; // Ctrl+BS - удалять слово слева от курсора

	//CheckTheming(); -- сейчас - нельзя. нужно дождаться, пока главное окно будет создано
	//mb_ThemingEnabled = (gOSVer.dwMajorVersion >= 6 || (gOSVer.dwMajorVersion == 5 && gOSVer.dwMinorVersion >= 1));
//------------------------------------------------------------------------
///| Default settings |///////////////////////////////////////////////////
//------------------------------------------------------------------------
	isShowBgImage = 0;
	wcscpy_c(sBgImage, L"c:\\back.bmp");
	bgImageDarker = 255; // 0x46;
	//nBgImageColors = 1|2; синий,зеленый
	//nBgImageColors = 2; // только синий
	nBgImageColors = (DWORD)-1; // Получить цвет панелей из фара - иначе "1|2" == BgImageColorsDefaults.
	bgOperation = eUpLeft;
	isBgPluginAllowed = 1;
	nTransparent = nTransparentInactive = 255;
	isTransparentSeparate = false;
	//isColorKey = false;
	//ColorKey = RGB(1,1,1);
	isUserScreenTransparent = false;
	isColorKeyTransparent = false;
	nColorKeyValue = RGB(1,1,1);
	isEnhanceGraphics = true; isEnhanceButtons = false;
	isPartBrush75 = 0xC8; isPartBrush50 = 0x96; isPartBrush25 = 0x5A;
	isPartBrushBlack = 32; //-V112
	isExtendUCharMap = true;
	isDownShowHiddenMessage = false;
	isDownShowExOnTopMessage = false;
	isFixFarBorders = 1;
	isAntiAlias2 = false; // disabled by default to avoid dashed framed
	ParseCharRanges(L"2013-25C4", mpc_CharAltFontRanges);

	// [Debug] Максимальный видимый размер подкорректируется в CConEmuMain::CreateMainWindow()
	wndWidth.Set(true, ss_Standard, DEF_CON_WIDTH);    // RELEASEDEBUGTEST(80,110)
	wndHeight.Set(false, ss_Standard, DEF_CON_HEIGHT); // RELEASEDEBUGTEST(25,35)

	ntvdmHeight = 0; // Подбирать автоматически
	mb_IntegralSize = false;
	_WindowMode = wmNormal;
	isUseCurrentSizePos = true; // Show in settings dialog and save current window size/pos
	//isFullScreen = false;
	isHideCaption = false;
	mb_HideCaptionAlways = false; // IsWin10(); -- let default behavior be the same on all OS versions
	isQuakeStyle = false; nQuakeAnimation = QUAKEANIMATION_DEF;
	isRestore2ActiveMon = false;
	isRestoreInactive = false;
	isHideChildCaption = true;
	isFocusInChildWindows = true;
	nHideCaptionAlwaysFrame = HIDECAPTIONALWAYSFRAME_DEF; nHideCaptionAlwaysDelay = 2000; nHideCaptionAlwaysDisappear = 2000;
	isSnapToDesktopEdges = true;
	isAlwaysOnTop = false;
	isSleepInBackground = true; // don't be CPU consuming in background
	isRetardInactivePanes = false; // не включать "засыпание в видимых-но-неактивных сплитах"
	mb_MinimizeOnLoseFocus = false; // не "прятаться" при потере фокуса
	RECT rcWork = {}; SystemParametersInfo(SPI_GETWORKAREA, 0, &rcWork, 0);
	// If window is started at {0,0} position, than Win+Left (on Windows 10) tiles window only on height-half
	// Let's position by default ConEmu window in a normal position, slightly away from screen corner
	_wndX = std::max<int>(90,rcWork.left); _wndY = std::max<int>(90,rcWork.top);
	wndCascade = true;
	isAutoSaveSizePos = true;
	mb_ExitSettingsAutoSaved = false;
	isConVisible = false; //isLockRealConsolePos = false;
	isUseInjects = true; // Fuck... Features or speed. User must choose!

	isSetDefaultTerminal = false;
	isRegisterOnOsStartup = false;
	isRegisterOnOsStartupTSA = false;
	isRegisterAggressive = true;
	isDefaultTerminalNoInjects = false;
	isDefaultTerminalNewWindow = false;
	isDefaultTerminalDebugLog = false;
	nDefaultTerminalConfirmClose = TerminalConfirmClose::Always;
	SetDefaultTerminalApps(L"explorer.exe"/* to default value */); // "|"-delimited string -> MSZ

	isProcessAnsi = true;
	isAnsiExec = ansi_CmdOnly;
	psAnsiAllowed = new CEStr(L"cmd -cur_console:R /cGitShowBranch.cmd\r\n");
	isAnsiLog = false;
	isAnsiLogCodes = false;
	pszAnsiLog = lstrdup(CEANSILOGFOLDER).Detach();
	isKillSshAgent = true;
	isProcessNewConArg = true;
	isProcessCmdStart = false; // gh#420
	isProcessCtrlZ = false; // gh#465, golang/go#6303
	isSuppressBells = true;
	isConsoleExceptionHandler = false; // по умолчанию - false
	mb_UseClink = true;

	isDisableMouse = false;

	nSlideShowElapse = 2500;
	nIconID = IDI_ICON1;
	isRClickSendKey = 2;
	sRClickMacro = nullptr;
	wcscpy_c(szTabConsole, L"<%c> %s");
	wcscpy_c(szTabModifiedSuffix, L"[*]");
	wchar_t szTabSkipWords[64];
	wcscpy_c(szTabSkipWords, L"Administrator:|Администратор:");
	pszTabSkipWords = lstrdup(szTabSkipWords).Detach();
	wcscpy_c(szTabPanels, szTabConsole); // Раньше была только настройка "TabConsole". Унаследовать ее в "TabPanels"
	wcscpy_c(szTabEditor, L"<%c.%i>{%s}");
	wcscpy_c(szTabEditorModified, L"<%c.%i>[%s] *");
	wcscpy_c(szTabViewer, L"<%c.%i>[%s]");
	nTabLenMax = 20;
	nTabWidthMax = 200;
	nTabStyle = ts_Win8;
	isSafeFarClose = true;
	sSafeFarCloseMacro = nullptr; // если nullptr - то используется макрос по умолчанию
	isCTSIntelligent = true;
	pszCTSIntelligentExceptions = LineDelimited2MSZ(L"far|vim").Detach();
	isCTSAutoCopy = true;
	isCTSResetOnRelease = false;
	isCTSIBeam = true;
	isCTSEndOnTyping = false;
	isCTSEndOnKeyPress = false;
	isCTSEraseBeforeReset = true;
	isCTSFreezeBeforeSelect = false;
	isCTSSelectBlock = true; //isCTSVkBlock = VK_LMENU; // по умолчанию - блок выделяется c LAlt
	isCTSSelectText = true; //isCTSVkText = VK_LSHIFT; // а текст - при нажатом LShift
	isCTSHtmlFormat = 0; // Don't use HTML formatting with copy (by default)
	isCTSForceLocale = 0; // Try to bypass clipboard locale problems (pasting to old non-unicode apps)
	#ifdef _DEBUG
	isCTSForceLocale = 0x0419; // russian locale
	#endif
	isCTSRBtnAction = MouseButtonAction::Auto;
	isCTSMBtnAction = MouseButtonAction::None;
	isCTSColorIndex = DefaultSelectionConsoleColor/*0xE0*/;
	isPasteConfirmEnter = true;
	nPasteConfirmLonger = 200;
	isFarGotoEditor = true; //isFarGotoEditorVk = VK_LCONTROL;
	sFarGotoEditor = lstrdup(HI_GOTO_EDITOR_FAR).Detach();
	isHighlightMouseRow = false; // Not turned on by default
	isHighlightMouseCol = false; // Not turned on by default

	isStatusBarShow = true;
	isStatusBarFlags = csf_HorzDelim;
	#if 1
	// Use solarized palette by default
	nStatusBarBack = RGB(7,54,66); // 101 123 131
	nStatusBarLight = RGB(253,246,227); // 253 246 227
	nStatusBarDark = RGB(147,161,161); // 147 161 161
	#else
	// Old palette
	nStatusBarBack = RGB(64,64,64);
	nStatusBarLight = RGB(255,255,255);
	nStatusBarDark = RGB(160,160,160);
	#endif
	//nHideStatusColumns = ces_CursorInfo;
	_ASSERTE(countof(isStatusColumnHidden)>csi_Last);
	memset(isStatusColumnHidden, 0, sizeof(isStatusColumnHidden));
	// выставим те колонки, которые не нужны "юзеру" по умолчанию
	isStatusColumnHidden[csi_ConsoleTitle] = true;
	#ifndef _DEBUG
	isStatusColumnHidden[csi_KeyHooks] = true;
	isStatusColumnHidden[csi_ViewLock] = true;
	#endif
	isStatusColumnHidden[csi_CapsLock] = true;
	isStatusColumnHidden[csi_ScrollLock] = true;
	isStatusColumnHidden[csi_InputLocale] = true;
	isStatusColumnHidden[csi_TermModes] = false; // adjusted in InitVanilla()
	isStatusColumnHidden[csi_RConModes] = true;
	isStatusColumnHidden[csi_WindowPos] = true;
	isStatusColumnHidden[csi_WindowSize] = true;
	isStatusColumnHidden[csi_WindowClient] = true;
	isStatusColumnHidden[csi_WindowWork] = true;
	isStatusColumnHidden[csi_WindowBack] = true;
	isStatusColumnHidden[csi_WindowDC] = true;
	isStatusColumnHidden[csi_WindowMode] = true;
	isStatusColumnHidden[csi_WindowStyle] = true;
	isStatusColumnHidden[csi_WindowStyleEx] = true;
	isStatusColumnHidden[csi_HwndFore] = true;
	isStatusColumnHidden[csi_HwndFocus] = true;
	isStatusColumnHidden[csi_Zoom] = true;
	isStatusColumnHidden[csi_DPI] = true;
	isStatusColumnHidden[csi_ConEmuPID] = true;
	isStatusColumnHidden[csi_ConsolePos] = true; // adjusted in InitVanilla()
	isStatusColumnHidden[csi_BufferSize] = true; // adjusted in InitVanilla()
	//isStatusColumnHidden[csi_CursorInfo] = true; -- show one info col instead of three cursor columns (by default)
	isStatusColumnHidden[csi_CursorX] = true;
	isStatusColumnHidden[csi_CursorY] = true;
	isStatusColumnHidden[csi_CursorSize] = true;
	isStatusColumnHidden[csi_CellInfo] = true; // adjusted in InitVanilla()
	isStatusColumnHidden[csi_ConEmuHWND] = true;
	isStatusColumnHidden[csi_ConEmuView] = true;
	isStatusColumnHidden[csi_Server] = true;
	isStatusColumnHidden[csi_ServerHWND] = true;
	isStatusColumnHidden[csi_Transparency] = true;
	isStatusColumnHidden[csi_Time] = true;

	isTabs = 1; nTabsLocation = 0; isTabIcons = true; isOneTabPerGroup = false;
	bActivateSplitMouseOver = 1; // 1 - always, 2 - Regarding Windows settings (Active window tracking)
	isTabSelf = true; isTabRecent = true; isTabLazy = true;
	nTabFlashChanged = 8;
	nTabBarDblClickAction = TABBAR_DEFAULT_CLICK_ACTION; nTabBtnDblClickAction = TABBTN_DEFAULT_CLICK_ACTION;
	//ilDragHeight = 10;
	m_isTabsOnTaskBar = 2;
	isTaskbarOverlay = false;
	isTaskbarProgress = true;

	isStoreTaskbarkTasks = true;
	isJumpListAutoUpdate = true;

	sTabCloseMacro = sSaveAllMacro = nullptr;
	nToolbarAddSpace = 0;
	// Show only shield (szAdminTitleSuffix is ignored if ats_Shield)
	bAdminShield = ats_Shield;
	wcscpy_c(szAdminTitleSuffix, DefaultAdminTitleSuffix/*L" (Admin)"*/);
	//
	bHideInactiveConsoleTabs = false;
	bShowFarWindows = true;
	isRSelFix = true; isMouseSkipActivation = true; isMouseSkipMoving = true;
	isMouseDragWindow = true;
	isFarWheelScrollback = false;
	isFarHourglass = true; nFarHourglassDelay = 2000;
	isDisableFarFlashing = false; isDisableAllFlashing = false;
	isDragEnabled = DRAG_L_ALLOWED; isDropEnabled = (BYTE)1; isDefCopy = true; isDropUseMenu = 2;
	//nLDragKey = 0; nRDragKey = VK_LCONTROL;
	isDragOverlay = 1; isDragShowIcons = true;
	// изменение размера панелей мышкой
	isDragPanel = 2; // по умолчанию сделаем чтобы драгалось макросами (вдруг у юзера на Ctrl-Left/Right/Up/Down макросы висят... как бы конфуза не получилось)
	isDragPanelBothEdges = false; // таскать за обе рамки (правую-левой панели и левую-правой панели)
	isKeyBarRClick = true;
	isDebugSteps = false;
	isDebugLog = 0; mb_DisableLogging = false;
	MCHKHEAP
	FindOptions.bMatchCase = false;
	FindOptions.bMatchWholeWords = false;
	FindOptions.bFreezeConsole = true;
	FindOptions.bHighlightAll = true;
	FindOptions.bTransparent = true;
	// Thumbnails
	memset(&ThSet, 0, sizeof(ThSet));
	ThSet.cbSize = sizeof(ThSet);
	// фон превьюшки: RGB или Index
	SetThumbColor(ThSet.crBackground, RGB(255,255,255), 16, TRUE); // Поставим по умолчанию "Auto"
	// серая рамка вокруг превьюшки
	ThSet.nPreviewFrame = 1;
	SetThumbColor(ThSet.crPreviewFrame, RGB(128,128,128), 8, TRUE);
	// рамка вокруг текущего элемента
	ThSet.nSelectFrame = 1;
	SetThumbColor(ThSet.crSelectFrame, RGB(192,192,192), 7, FALSE);
	/* теперь разнообразные размеры */
	// отступы для preview
	SetThumbSize(ThSet.Thumbs,96,1,1,5,20,2,0,gsDefMUIFont,14);
	// отступы для tiles
	SetThumbSize(ThSet.Tiles,48,4,4,172,4,4,1,gsDefMUIFont,14); //-V112
	// Прочие параметры загрузки
	ThSet.bLoadPreviews = 3;   // bitmask of {1=Thumbs, 2=Tiles}
	ThSet.bLoadFolders = true; // true - load infolder previews (only for Thumbs)
	ThSet.nLoadTimeout = 15;   // 15 sec
	ThSet.nMaxZoom = 600; // %%
	ThSet.bUsePicView2 = true;
	ThSet.bRestoreOnStartup = false;
	//// Пока не используется
	//DWORD nCacheFolderType; // юзер/программа/temp/и т.п.
	//wchar_t sCacheFolder[MAX_PATH];

	/* *** AutoUpdate *** */
	_ASSERTE(UpdSet.szUpdateVerLocation==nullptr); // Уже должен был быть вызван ReleasePointers
	UpdSet.ResetToDefaults();
}

// Здесь можно включить настройки, которые должны включаться только для новых конфигураций!
void Settings::InitVanilla()
{
	if (!gpSetCls->IsConfigNew)
	{
		_ASSERTE(gpSetCls->IsConfigNew == true);
		return;
	}

	// WARNING!!! Following settings MUST be saved in Settings::SaveVanilla
	InitVanillaFontSettings();
	// WARNING!!! These settings MUST be saved in Settings::SaveVanilla

	// to avoid hotkey conflicts for existing users we set some keys here instead of ConEmuHotKeyList::AllocateHotkeys()
	gpSet->SetHotkeyById(vkEditMenu2, ConEmuChord::MakeHotKey(VK_RBUTTON, VK_SHIFT));

	// And some settings we need to load from registry if started with "-basic" switch
	if (gpConEmu->IsResetBasicSettings())
	{
		CEStr lsStr;
		if (RegGetStringValue(HKEY_CURRENT_USER, CONEMU_ROOT_KEY, L"Notification.StopBuzzingDate", lsStr) > 0)
		{
			lstrcpyn(StopBuzzingDate, lsStr, countof(StopBuzzingDate));
		}

		// These would be nice to have in InitSettings()
		isStatusColumnHidden[csi_TermModes] = false;
		isStatusColumnHidden[csi_ConsolePos] = false;
		isStatusColumnHidden[csi_BufferSize] = false;
		isStatusColumnHidden[csi_ConsoleSize] = true;
		isStatusColumnHidden[csi_NumLock] = true;
		isStatusColumnHidden[csi_CellInfo] = false;

		// `-basic` mode was intended for troubleshooting and debugging
		// So disable Automatic Update if it was not requested by `-update`
		if (!gpConEmu->opt.AutoUpdateOnStart.GetBool())
		{
			UpdSet.isUpdateCheckOnStartup = false;
			UpdSet.isUpdateCheckHourly = false;
		}
	}
}

void Settings::InitVanillaFontSettings()
{
	// For new users, let use ‘standard’ font heights? Meaning of display units and character height
	FontUseUnits = true;
	// dpi must not be embedded into font height!
	// To avoid fatal conflicts with old versions - we are storing only positive values
	FontSizeY = gpConEmu->mp_Inside ? DEF_INSIDESIZEY_U : DEF_FONTSIZEY_U;
	nStatusFontHeight = DEF_STATUSFONTY_U;
	nTabFontHeight = DEF_TABFONTY_U;
}

bool Settings::SaveVanilla(SettingsBase* reg)
{
	bool bOk = false;

	if (reg && reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		/* Start command */
		SaveStartCommands(reg);

		/* Palette */
		SaveStdColors(reg);

		/* Force Single instance mode */
		reg->Save(L"SingleInstance", gpSet->isSingleInstance);

		/* Quake mode? */
		reg->Save(L"QuakeStyle", isQuakeStyle);

		/* Install Keyboard hooks */
		reg->Save(L"KeyboardHooks", gpSet->m_isKeyboardHooks);

		/* Inject ConEmuHk.dll */
		reg->Save(L"UseInjects", gpSet->isUseInjects);

		/* Auto Update settings */
		reg->Save(L"Update.CheckOnStartup", gpSet->UpdSet.isUpdateCheckOnStartup);
		reg->Save(L"Update.CheckHourly", gpSet->UpdSet.isUpdateCheckHourly);
		reg->Save(L"Update.ConfirmDownload", gpSet->UpdSet.isUpdateConfirmDownload);
		reg->Save(L"Update.UseBuilds", (BYTE)gpSet->UpdSet.isUpdateUseBuilds);

		/* Font related */
		reg->Save(L"FontUseDpi", FontUseDpi);
		reg->Save(L"FontUseUnits", FontUseUnits);
		reg->Save(L"FontSize", FontSizeY);
		reg->Save(L"StatusFontHeight", nStatusFontHeight);
		reg->Save(L"TabFontHeight", nTabFontHeight);

		// Fast configuration done
		reg->CloseKey();

		// Minimize/Restore key from Fast Settings
		SaveHotkeys(reg, vkMinimizeRestore);

		bOk = true;
	}

	return bOk;
}

void Settings::ResetSavedOnExit()
{
	mb_ExitSettingsAutoSaved = false;
}

// В Desktop, Inside (и еще может быть когда) Transparent включать нельзя
bool Settings::isTransparentAllowed()
{
	// If the window is "Child" - transparency is not allowed
	if (gpConEmu->opt.DesktopMode || gpConEmu->mp_Inside)
		return false;

	// Чтобы не рушилось отображение картинки плагинами
	if (gpConEmu->isPictureView())
		return false;

	// Можно, по настройкам
	return true;
}

// true - не допускать Gaps в Normal режиме. Подгонять размер окна точно под консоль.
// false - размер окна - свободный (попиксельно)
bool Settings::isIntegralSize() const
{
	if (mb_IntegralSize)
		return false;

	if (isQuakeStyle || gpConEmu->mp_Inside)
		return false;

	if ((wndWidth.Style == ss_Percents && wndWidth.Value == 100)
		|| (wndHeight.Style == ss_Percents && wndHeight.Value == 100))
		return false;

	#if 0
	if ((1 & (WORD)GetKeyState(VK_NUMLOCK)) == 0)
		return false;
	#endif

	return true;
}

bool Settings::isActivateSplitMouseOver()
{
	if (bActivateSplitMouseOver == 2)
	{
		BOOL bTracking = FALSE;
		SystemParametersInfo(SPI_GETACTIVEWINDOWTRACKING, 0, &bTracking, 0);
		return (bTracking != 0);
	}
	else
	{
		return (bActivateSplitMouseOver == 1);
	}
}

bool Settings::isAdminShield()
{
	return ((bAdminShield & ats_Shield) == ats_Shield);
}

bool Settings::isAdminSuffix()
{
	return (szAdminTitleSuffix[0] && ((bAdminShield == ats_Empty) || (bAdminShield == ats_ShieldSuffix)));
}

void Settings::FreeApps(int NewAppCount, AppSettings** NewApps/*, Settings::CEAppColors** NewAppColors*/)
{
	int OldAppCount = this->AppCount;
	this->AppCount = NewAppCount;
	AppSettings** OldApps = Apps; Apps = NewApps;
	//CEAppColors** OldAppColors = AppColors; AppColors = NewAppColors;
	for (int i = 0; i < OldAppCount && OldApps; i++)
	{
		if (OldApps[i])
		{
			OldApps[i]->FreeApps();
			SafeFree(OldApps[i]);
		}
		//SafeFree(OldAppColors[i]);
	}
	SafeFree(OldApps);
	//SafeFree(OldAppColors);
}

void Settings::LoadAppsSettings(SettingsBase* reg, bool abFromOpDlg /*= false*/)
{
	bool lbDelete = false;
	if (!reg)
	{
		// Если открыто окно настроек - не передергивать
		if (!abFromOpDlg && ghOpWnd)
		{
			return;
		}

		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return;
		}
		lbDelete = true;
	}

	BOOL lbOpened = FALSE;
	CEStr szBaseKey(gpSetCls->GetConfigPath(), L"\\Apps");


	lbOpened = reg->OpenKey(szBaseKey, KEY_READ);
	if (lbOpened)
	{
		int NewAppCount = 0;
		AppSettings** NewApps = nullptr;

		reg->Load(L"Count", NewAppCount);
		reg->CloseKey();

		if (lbOpened && NewAppCount > 0)
		{
			NewApps = (AppSettings**)calloc(NewAppCount, sizeof(*NewApps));
			//NewAppColors = (CEAppColors**)calloc(NewAppCount, sizeof(*NewAppColors));

			int nSucceeded = 0;
			for (int i = 0; i < NewAppCount; i++)
			{
				wchar_t szNumber[16];
				CEStr szAppKey(szBaseKey, L"\\App", ltow_s(i+1, szNumber, 10));

				lbOpened = reg->OpenKey(szAppKey, KEY_READ);
				if (lbOpened)
				{
					_ASSERTE(AppStd.AppNames == nullptr && AppStd.AppNamesLwr == nullptr);

					NewApps[nSucceeded] = (AppSettings*)malloc(sizeof(AppSettings));
					//NewAppColors[nSucceeded] = (CEAppColors*)calloc(1,sizeof(CEAppColors));

					// Умолчания берем из основной ветки!
					*NewApps[nSucceeded] = AppStd;
					//memmove(NewAppColors[nSucceeded]->Colors, Colors, sizeof(Colors));
					NewApps[nSucceeded]->AppNames = nullptr;
					NewApps[nSucceeded]->AppNamesLwr = nullptr;
					NewApps[nSucceeded]->cchNameMax = 0;

					// Загрузка "AppNames" - снаружи, т.к. LoadAppSettings используется и для загрузки &AppStd
					reg->Load(L"AppNames", &NewApps[nSucceeded]->AppNames);
					if (NewApps[nSucceeded]->AppNames /*&& *NewApps[nSucceeded]->AppNames*/)
					{
						NewApps[nSucceeded]->cchNameMax = wcslen(NewApps[nSucceeded]->AppNames)+1;
						NewApps[nSucceeded]->AppNamesLwr = lstrdup(NewApps[nSucceeded]->AppNames).Detach();
						CharLowerBuff(NewApps[nSucceeded]->AppNamesLwr, lstrlen(NewApps[nSucceeded]->AppNamesLwr));
						reg->Load(L"Elevated", NewApps[nSucceeded]->Elevated);
						LoadAppSettings(reg, NewApps[nSucceeded]/*, NewAppColors[nSucceeded]->Colors*/);
						nSucceeded++;
					}
					reg->CloseKey();
				}
			}
			NewAppCount = nSucceeded;
		}

		FreeApps(NewAppCount, NewApps/*, NewAppColors*/);
	}

	if (lbDelete)
		delete reg;
}

void Settings::LoadCursorSettings(SettingsBase* reg, CECursorType* pActive, CECursorType* pInactive)
{
	#define _MinMax(a,mn,mx) if (a < mn) a = mn; else if (a > mx) a = mx;

	// Is there new style was saved?
	bool bActive = reg->Load(L"CursorTypeActive", pActive->Raw);
	DEBUGTEST(bool bInactive = false);

	if (bActive)
	{
		_MinMax(pActive->CursorType, cur_First, cur_Last);
		_MinMax(pActive->FixedSize, CURSORSIZE_MIN, CURSORSIZE_MAX);
		_MinMax(pActive->MinSize, CURSORSIZEPIX_MIN, CURSORSIZEPIX_MAX);

		DEBUGTEST(bInactive = )
		reg->Load(L"CursorTypeInactive", pInactive->Raw);

		_MinMax(pInactive->CursorType, cur_First, cur_Last);
		_MinMax(pInactive->FixedSize, 0, CURSORSIZE_MAX);
		_MinMax(pInactive->MinSize, 0, CURSORSIZEPIX_MAX);
	}
	else
	{
		// Need to convert old settings style

		BYTE isCursorType = cur_Vert; // 0 - Horz, 1 - Vert, 2 - Hollow-block
		bool isCursorBlink = true;
		bool isCursorColor = true;
		bool isCursorBlockInactive = true;
		bool isCursorIgnoreSize = false;
		BYTE nCursorFixedSize = 25; // в процентах
		BYTE nCursorMinSize = 2; // в пикселях

		reg->Load(L"CursorType", isCursorType);
			_MinMax(isCursorType, cur_First, cur_Last);
		reg->Load(L"CursorColor", isCursorColor);
		reg->Load(L"CursorBlink", isCursorBlink);
		reg->Load(L"CursorBlockInactive", isCursorBlockInactive);
		reg->Load(L"CursorIgnoreSize", isCursorIgnoreSize);
		reg->Load(L"CursorFixedSize", nCursorFixedSize);
			_MinMax(nCursorFixedSize, CURSORSIZE_MIN, CURSORSIZE_MAX);
		reg->Load(L"CursorMinSize", nCursorMinSize);
			_MinMax(nCursorMinSize, CURSORSIZEPIX_MIN, CURSORSIZEPIX_MAX);

		pActive->Raw = 0; // Сброс
		pActive->CursorType = (CECursorStyle)isCursorType; // 0 - Horz, 1 - Vert, 2 - Hollow-block
		pActive->isBlinking = isCursorBlink;
		pActive->isColor = isCursorColor;
		//AppStd.CursorActive.isCursorBlockInactive = true;
		pActive->isFixedSize = isCursorIgnoreSize;
		pActive->FixedSize = nCursorFixedSize; // в процентах
		pActive->MinSize = nCursorMinSize; // в пикселях

		pInactive->Raw = AppStd.CursorActive.Raw; // копирование
		pInactive->Used = isCursorBlockInactive;
		pInactive->CursorType = cur_Rect;
		pInactive->isBlinking = false;
	}
}

void Settings::LoadAppSettings(SettingsBase* reg, AppSettings* pApp/*, COLORREF* pColors*/)
{
	// Для AppStd данные загружаются из основной ветки! В том числе и цвета (RGB[32] а не имя палитры)
	bool bStd = (pApp == &AppStd);

	pApp->OverridePalette = bStd;
	if (bStd)
	{
		TCHAR ColorName[] = L"ColorTable00";
		const size_t digitPos = _tcslen(ColorName) - 2;
		for (size_t i = Colors.size(); i--;)
		{
			ColorName[digitPos] = TCHAR(i/10 + '0');
			ColorName[digitPos + 1] = TCHAR(i%10 + '0');
			reg->Load(ColorName, Colors[i]);
		}

		reg->Load(L"TextColorIdx", pApp->nTextColorIdx);
		if (pApp->nTextColorIdx > 16) pApp->nTextColorIdx=CEDEF_BackColorAuto/*16*/;
		reg->Load(L"BackColorIdx", pApp->nBackColorIdx);
		if (pApp->nBackColorIdx > 16) pApp->nBackColorIdx=CEDEF_BackColorAuto/*16*/;
		reg->Load(L"PopTextColorIdx", pApp->nPopTextColorIdx);
		if (pApp->nPopTextColorIdx > 16) pApp->nPopTextColorIdx=CEDEF_BackColorAuto/*16*/;
		reg->Load(L"PopBackColorIdx", pApp->nPopBackColorIdx);
		if (pApp->nPopBackColorIdx > 16) pApp->nPopBackColorIdx=CEDEF_BackColorAuto/*16*/;
	}
	else
	{
		reg->Load(L"OverridePalette", pApp->OverridePalette);
		if (!reg->Load(L"PaletteName", pApp->szPaletteName, countof(pApp->szPaletteName)))
			pApp->szPaletteName[0] = 0;
		pApp->ResetPaletteIndex();
		const ColorPalette* pPal = PaletteGet(pApp->GetPaletteIndex());

		_ASSERTE(pPal!=nullptr); // nullptr не может быть. Всегда как минимум - стандартная палитра

		pApp->nTextColorIdx = pPal->nTextColorIdx;
		pApp->nBackColorIdx = pPal->nBackColorIdx;
		pApp->nPopTextColorIdx = pPal->nPopTextColorIdx;
		pApp->nPopBackColorIdx = pPal->nPopBackColorIdx;
	}

	pApp->OverrideExtendFonts = bStd;
	if (!bStd)
		reg->Load(L"OverrideExtendFonts", pApp->OverrideExtendFonts);
	reg->Load(L"ExtendFonts", pApp->isExtendFonts);
	reg->Load(L"ExtendFontNormalIdx", pApp->nFontNormalColor);
	if (pApp->nFontNormalColor>15 && pApp->nFontNormalColor!=255) pApp->nFontNormalColor=1;
	reg->Load(L"ExtendFontBoldIdx", pApp->nFontBoldColor);
	if (pApp->nFontBoldColor>15 && pApp->nFontBoldColor!=255) pApp->nFontBoldColor=12;
	reg->Load(L"ExtendFontItalicIdx", pApp->nFontItalicColor);
	if (pApp->nFontItalicColor>15 && pApp->nFontItalicColor!=255) pApp->nFontItalicColor=13;

	pApp->OverrideCursor = bStd;
	if (!bStd)
		reg->Load(L"OverrideCursor", pApp->OverrideCursor);
	LoadCursorSettings(reg, &pApp->CursorActive, &pApp->CursorInactive);

	pApp->OverrideClipboard = bStd;
	if (!bStd)
		reg->Load(L"OverrideClipboard", pApp->OverrideClipboard);
	reg->Load(L"ClipboardDetectLineEnd", pApp->isCTSDetectLineEnd);
	reg->Load(L"ClipboardBashMargin", pApp->isCTSBashMargin);
	reg->Load(L"ClipboardTrimTrailing", pApp->isCTSTrimTrailing);
	reg->Load(L"ClipboardEOL", pApp->isCTSEOL); MinMax(pApp->isCTSEOL,2);
	reg->Load(L"ClipboardArrowStart", pApp->isCTSShiftArrowStart);
	reg->Load(L"ClipboardAllLines", pApp->isPasteAllLines);
	reg->Load(L"ClipboardAllLinesPosix", pApp->isPosixAllLines);
	reg->Load(L"ClipboardFirstLine", pApp->isPasteFirstLine);
	reg->Load(L"ClipboardFirstLinePosix", pApp->isPosixFirstLine);
	reg->Load(L"ClipboardClickPromptPosition", pApp->isCTSClickPromptPosition); MinMax(pApp->isCTSClickPromptPosition,2);
	reg->Load(L"ClipboardDeleteLeftWord", pApp->isCTSDeleteLeftWord); MinMax(pApp->isCTSDeleteLeftWord,2);
}

void Settings::FreeCmdTasks()
{
	if (CmdTasks)
	{
		for (int i = 0; i < CmdTaskCount; i++)
		{
			if (CmdTasks[i])
				CmdTasks[i]->FreePtr();
		}
		SafeFree(CmdTasks);
	}
}

void Settings::FreeStartupTask()
{
	if (StartupTask)
	{
		StartupTask->FreePtr();
		StartupTask = nullptr; // освобождается в FreePtr
	}
}

void Settings::LoadCmdTasks(SettingsBase* reg, bool abFromOpDlg /*= false*/)
{
	bool lbDelete = false;
	if (!reg)
	{
		// Если открыто окно настроек - не передергивать
		if (!abFromOpDlg && ghOpWnd)
		{
			return;
		}

		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return;
		}
		lbDelete = true;
	}


	BOOL lbOpened = FALSE;
	CEStr szBaseKey(gpSetCls->GetConfigPath(), L"\\Tasks");


	{
		// Autoload "<Startup>" task
		CEStr szCmdKey(szBaseKey, L"\\", AutoStartTaskName);
		lbOpened = reg->OpenKey(szCmdKey, KEY_READ);
		if (lbOpened)
		{
			FreeStartupTask();
			_ASSERTE(StartupTask == nullptr);
			if ((StartupTask = (CommandTasks*)calloc(1, sizeof(CommandTasks))) != nullptr)
				StartupTask->LoadCmdTask(reg, -1);

			reg->CloseKey();
		}
	}



	lbOpened = reg->OpenKey(szBaseKey, KEY_READ);
	if (lbOpened)
	{
		// #SETTINGS Try to append instead of resetting tasks list?
		FreeCmdTasks();

		int NewTasksCount = 0;
		reg->Load(L"Count", NewTasksCount);
		reg->CloseKey();

		if (lbOpened && NewTasksCount > 0)
		{
			CmdTasks = (CommandTasks**)calloc(NewTasksCount, sizeof(CommandTasks*));

			int nSucceeded = 0;
			for (int i = 0; i < NewTasksCount; i++)
			{
				wchar_t szNumber[16];
				CEStr szCmdKey(szBaseKey, L"\\Task", ltow_s(i+1, szNumber, 10)); // 1-based

				lbOpened = reg->OpenKey(szCmdKey, KEY_READ);
				if (lbOpened)
				{
					_ASSERTE(CmdTasks[i] == nullptr);
					if ((CmdTasks[i] = (CommandTasks*)calloc(1, sizeof(CommandTasks))) != nullptr)
						if (CmdTasks[i]->LoadCmdTask(reg, i))
							nSucceeded++;

					reg->CloseKey();
				}
			}
			CmdTaskCount = nSucceeded;
		}
	}

	if (lbDelete)
		delete reg;
}

bool Settings::SaveCmdTasks(SettingsBase* reg)
{
	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return false;
		}
		lbDelete = true;
	}

	bool lbRc = false;
	BOOL lbOpened = FALSE;
	int OldTaskCount = 0;
	int NewTaskCount = 0;
	wchar_t szCmdKey[MAX_PATH+64];
	wcscpy_c(szCmdKey, gpSetCls->GetConfigPath());
	wcscat_c(szCmdKey, L"\\Tasks");
	wchar_t* pszCmdKey = szCmdKey+lstrlen(szCmdKey);

	lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
	if (lbOpened)
	{
		lbRc = true;
		reg->Load(L"Count", OldTaskCount);
		reg->Save(L"Count", CmdTaskCount);
		reg->CloseKey();
	}

	if (lbOpened && CmdTasks && CmdTaskCount > 0)
	{
		for (int i = 0; i < CmdTaskCount; i++)
		{
			if (CmdTasks[i] == nullptr)
				continue;

			swprintf_c(pszCmdKey, 32/*#SECURELEN*/, L"\\Task%i", NewTaskCount+1); // 1-based

			lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
			if (!lbOpened)
			{
				lbRc = false;
			}
			else
			{
				if (CmdTasks[i]->SaveCmdTask(reg, false/*CmdTasks[i] == StartupTask*/))
					NewTaskCount++;

				reg->CloseKey();
			}
		}
	}

	// Maintain list and count
	if (lbOpened && ((NewTaskCount != CmdTaskCount) || (NewTaskCount < OldTaskCount)))
	{
		// Update the "Count" value
		*pszCmdKey = 0;
		lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
		if (lbOpened)
		{
			lbRc = true;

			// invalid tasks were skipped?
			if (NewTaskCount != CmdTaskCount)
			{
				reg->Save(L"Count", NewTaskCount);
			}

			// Trim obsolete tasks
			wchar_t szDelTaskKey[32];
			for (int i = NewTaskCount; i < OldTaskCount; ++i)
			{
				swprintf_c(szDelTaskKey, L"Task%i", i+1); // 1-based
				reg->DeleteKey(szDelTaskKey);
			}

			reg->CloseKey();
		}
	}

	if (lbDelete)
		delete reg;

	return lbRc;
}

void Settings::SortPalettes()
{
	for (int i = 0; (i+1) < PaletteCount; i++)
	{
		int iMin = i;
		for (int j = (i+1); j < PaletteCount; j++)
		{
			// Don't "sort" predefined palettes at all
			if (Palettes[iMin]->bPredefined /*&& !Palettes[j]->bPredefined*/)
			{
				continue;
			}
			else if (!Palettes[iMin]->bPredefined && Palettes[j]->bPredefined)
			{
				_ASSERTE(!(!Palettes[iMin]->bPredefined && Palettes[j]->bPredefined));
				iMin = j;
			}
			else if (CSTR_GREATER_THAN == CompareString(LOCALE_USER_DEFAULT, NORM_IGNORECASE, Palettes[iMin]->pszName, -1, Palettes[j]->pszName, -1))
			{
				iMin = j;
			}
		}

		if (iMin != i)
		{
			ColorPalette* p = Palettes[i];
			Palettes[i] = Palettes[iMin];
			Palettes[iMin] = p;
		}
	}
}

void Settings::FreePalettes()
{
	if (Palettes)
	{
		for (int i = 0; i < PaletteCount; i++)
		{
			if (Palettes[i])
				Palettes[i]->FreePtr();
		}
		SafeFree(Palettes);
	}
	PaletteCount = 0;
}

void Settings::CreatePredefinedPalettes(int iAddUserCount)
{
	_ASSERTE(Palettes == nullptr);

	// Predefined
	Palettes = (ColorPalette**)calloc((iAddUserCount + countof(DefColors)), sizeof(ColorPalette*));
	for (size_t n = 0; n < countof(DefColors); n++)
	{
		Palettes[n] = (ColorPalette*)calloc(1, sizeof(ColorPalette));
		_ASSERTE(DefColors[n].pszTitle && DefColors[n].pszTitle[0]==L'<' && DefColors[n].pszTitle[lstrlen(DefColors[n].pszTitle)-1]==L'>');
		Palettes[n]->pszName = lstrdup(DefColors[n].pszTitle).Detach();
		Palettes[n]->bPredefined = true;
		Palettes[n]->nTextColorIdx = Palettes[n]->nBackColorIdx = CEDEF_BackColorAuto/*16*/;
		Palettes[n]->nPopTextColorIdx = Palettes[n]->nPopBackColorIdx = CEDEF_BackColorAuto/*16*/;
		_ASSERTE((Palettes[n]->Colors.size()==0x10) && (DefColors[n].dwDefColors.size()==0x10));
		Palettes[n]->Colors = DefColors[n].dwDefColors;
		if (DefColors[n].isIndexes())
		{
			Palettes[n]->nTextColorIdx = DefColors[n].nIndexes[0];
			Palettes[n]->nBackColorIdx = DefColors[n].nIndexes[1];
			Palettes[n]->nPopTextColorIdx = DefColors[n].nIndexes[2];
			Palettes[n]->nPopBackColorIdx = DefColors[n].nIndexes[3];
		}
		else
		{
			Palettes[n]->nTextColorIdx = Palettes[n]->nBackColorIdx = 16; // Auto
			Palettes[n]->nPopTextColorIdx = Palettes[n]->nPopBackColorIdx = 16; // Auto
		}
	}

	// Инициализировали "Палитрами по умолчанию"
	PaletteCount = (int)countof(DefColors);
}

void Settings::LoadPalettes(SettingsBase* reg)
{
	TCHAR ColorName[] = L"ColorTable00";
	const size_t digitPos = _tcslen(ColorName) - 2;

	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return;
		}
		lbDelete = true;
	}

	FreePalettes();

	BOOL lbOpened = FALSE;
	CEStr szBaseKey(gpSetCls->GetConfigPath(), L"\\Colors");

	lbOpened = reg->OpenKey(szBaseKey, KEY_READ);
	if (!lbOpened)
	{
		// Predefined
		CreatePredefinedPalettes(0);
		_ASSERTE(Palettes!=nullptr);
		// Was initialize with "Default palettes"
		_ASSERTE(PaletteCount == (int)countof(DefColors));
	}
	else
	{

		int UserCount = 0;
		reg->Load(L"Count", UserCount);
		reg->CloseKey();

		// Predefined
		CreatePredefinedPalettes(UserCount);
		_ASSERTE(Palettes!=nullptr);
		// Was initialize with "Default palettes"
		_ASSERTE(PaletteCount == (int)countof(DefColors));

		// User
		for (int i = 0; i < UserCount; i++)
		{
			wchar_t szNumber[16];
			CEStr szColorKey(szBaseKey, L"\\Palette", ltow_s(i+1, szNumber, 10)); // 1-based

			lbOpened = reg->OpenKey(szColorKey, KEY_READ);
			if (lbOpened)
			{
				Palettes[PaletteCount] = (ColorPalette*)calloc(1, sizeof(ColorPalette));

				reg->Load(L"Name", &Palettes[PaletteCount]->pszName);

				reg->Load(L"TextColorIdx", Palettes[PaletteCount]->nTextColorIdx); MinMax(Palettes[PaletteCount]->nTextColorIdx,16);
				reg->Load(L"BackColorIdx", Palettes[PaletteCount]->nBackColorIdx); MinMax(Palettes[PaletteCount]->nBackColorIdx,16);
				reg->Load(L"PopTextColorIdx", Palettes[PaletteCount]->nPopTextColorIdx); MinMax(Palettes[PaletteCount]->nPopTextColorIdx,16);
				reg->Load(L"PopBackColorIdx", Palettes[PaletteCount]->nPopBackColorIdx); MinMax(Palettes[PaletteCount]->nPopBackColorIdx,16);

				_ASSERTE(Colors.size() == Palettes[PaletteCount]->Colors.size());
				for (size_t k = 0; k < Palettes[PaletteCount]->Colors.size()/*0x10*/; ++k)
				{
					ColorName[digitPos] = TCHAR(k / 10 + '0');
					ColorName[digitPos + 1] = TCHAR(k % 10 + '0');
					reg->Load(ColorName, Palettes[PaletteCount]->Colors[k]);
				}

				PaletteCount++;

				reg->CloseKey();
			}
		}
	}

	// sort
	SortPalettes();

	if (lbDelete)
		delete reg;
}

void Settings::SavePalettes(SettingsBase* reg)
{
	TCHAR ColorName[] = L"ColorTable00";
	const size_t digitPos = _tcslen(ColorName) - 2;

	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return;
		}
		lbDelete = true;
	}

	// sort
	SortPalettes();

	BOOL lbOpened = FALSE;
	wchar_t szColorKey[MAX_PATH+64];
	wcscpy_c(szColorKey, gpSetCls->GetConfigPath());
	wcscat_c(szColorKey, L"\\Colors");

	// Write "Count" before first "Palette"
	int UserPaletteCount = 0, OldPaletteCount = 0;
	lbOpened = reg->OpenKey(szColorKey, KEY_WRITE);
	if (lbOpened)
	{
		for (int i = 0; i < PaletteCount; i++)
		{
			if (!Palettes[i] || Palettes[i]->bPredefined)
				continue; // Системные - не сохраняем
			UserPaletteCount++;
		}
		reg->Load(L"Count", OldPaletteCount);
		reg->Save(L"Count", UserPaletteCount);
		reg->CloseKey();
	}

	wchar_t* pszColorKey = szColorKey + lstrlen(szColorKey);

	UserPaletteCount = 0;

	for (int i = 0; i < PaletteCount; i++)
	{
		if (!Palettes[i] || Palettes[i]->bPredefined)
			continue; // Системные - не сохраняем

		swprintf_c(pszColorKey, 32/*#SECURELEN*/, L"\\Palette%i", UserPaletteCount+1); // 1-based

		lbOpened = reg->OpenKey(szColorKey, KEY_WRITE);
		if (lbOpened)
		{
			UserPaletteCount++;

			reg->Save(L"Name", Palettes[i]->pszName);

			reg->Save(L"TextColorIdx", Palettes[i]->nTextColorIdx);
			reg->Save(L"BackColorIdx", Palettes[i]->nBackColorIdx);
			reg->Save(L"PopTextColorIdx", Palettes[i]->nPopTextColorIdx);
			reg->Save(L"PopBackColorIdx", Palettes[i]->nPopBackColorIdx);

			_ASSERTE(Colors.size() == Palettes[i]->Colors.size());
			for (size_t k = 0; k < Palettes[i]->Colors.size()/*0x10*/; ++k)
			{
				ColorName[digitPos] = TCHAR(k / 10 + '0');
				ColorName[digitPos + 1] = TCHAR(k % 10 + '0');
				reg->Save(ColorName, Palettes[i]->Colors[k]);
			}

			reg->CloseKey();
		}
	}

	// Clear the tail
	if (OldPaletteCount > UserPaletteCount)
	{
		*pszColorKey = 0;
		if (reg->OpenKey(szColorKey, KEY_WRITE))
		{
			wchar_t szDelPalette[32];
			for (int i = UserPaletteCount; i < OldPaletteCount; ++i)
			{
				swprintf_c(szDelPalette, L"Palette%i", i+1); // 1-based
				reg->DeleteKey(szDelPalette);
			}
		}
	}

	for (int k = 0; k < AppCount; k++)
	{
		bool bFound = false;
		for (int i = 0; i < PaletteCount; i++)
		{
			if (Palettes[i] && Palettes[i]->pszName && (lstrcmpi(Apps[k]->szPaletteName, Palettes[i]->pszName) == 0))
			{
				//memmove(AppColors[k]->Colors, Palettes[i]->Colors, sizeof(Palettes[i]->Colors));
				//AppColors[k]->FadeInitialized = false;

				Apps[k]->nTextColorIdx = Palettes[i]->nTextColorIdx;
				Apps[k]->nBackColorIdx = Palettes[i]->nBackColorIdx;
				Apps[k]->nPopTextColorIdx = Palettes[i]->nPopTextColorIdx;
				Apps[k]->nPopBackColorIdx = Palettes[i]->nPopBackColorIdx;

				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			Apps[k]->OverridePalette = false; // нету такой палитры, будем рисовать "основной"
		}
	}

	if (lbDelete)
	{
		delete reg;

		gpConEmu->Update(true);
	}
}

// 0-based, index of Palettes
// -1 -- current palette, it will show name "<Current color scheme>"
const ColorPalette* Settings::PaletteGet(int anIndex)
{
	return PaletteGetPtr(anIndex);
}

const ColorPalette* Settings::PaletteFindCurrent(bool bMatchAttributes)
{
	const ColorPalette* pCur = PaletteGetPtr(-1);
	if (!pCur)
	{
		// MUST be NOT nullptr
		_ASSERTE(pCur!=nullptr);
		return nullptr;
	}

	const ColorPalette* pFound = PaletteFindByColors(bMatchAttributes, pCur);

	_ASSERTE(pFound != pCur); // pCur is expected to be StdPal ("<Current color scheme>")

	// Return "<Current color scheme>" if was not found in saved palettes
	if (!pFound)
		pFound = pCur;

	return pFound;
}

const ColorPalette* Settings::PaletteFindByColors(bool bMatchAttributes, const ColorPalette* pCur)
{
	const ColorPalette* pFound = nullptr;
	const ColorPalette* pPal = nullptr;
	for (int i = 0; (pPal = PaletteGetPtr(i)) != nullptr; i++)
	{
		if (!bMatchAttributes
				|| ((pCur->nTextColorIdx == pPal->nTextColorIdx)
					&& (pCur->nBackColorIdx == pPal->nBackColorIdx)
					&& (pCur->nPopTextColorIdx == pPal->nPopTextColorIdx)
					&& (pCur->nPopBackColorIdx == pPal->nPopBackColorIdx))
			)
		{
			if (pCur->Colors == pPal->Colors)
			{
				pFound = pPal;
				// Do not break, prefer last palette (use saved)
			}
		}
	}

	return pFound;
}

// 0-based, index of Palettes
// -1 -- current palette, it will show name "<Current color scheme>"
ColorPalette* Settings::PaletteGetPtr(int anIndex)
{
	if ((anIndex >= 0) && (anIndex < PaletteCount) && Palettes && Palettes[anIndex])
	{
		return Palettes[anIndex];
	}

	if (anIndex != -1)
	{
		return nullptr;
	}

	_ASSERTE(anIndex==-1);

	static ColorPalette StdPal = {};
	StdPal.bPredefined = false;

	static wchar_t szCurrentScheme[64] = L"";
	lstrcpyn(szCurrentScheme, CLngRc::getRsrc(lng_CurClrScheme/*"<Current color scheme>"*/), countof(szCurrentScheme));
	StdPal.pszName = szCurrentScheme;

	StdPal.nTextColorIdx = AppStd.nTextColorIdx;
	StdPal.nBackColorIdx = AppStd.nBackColorIdx;
	StdPal.nPopTextColorIdx = AppStd.nPopTextColorIdx;
	StdPal.nPopBackColorIdx = AppStd.nPopBackColorIdx;

	_ASSERTE(StdPal.Colors.size() == this->Colors.size());
	StdPal.Colors = this->Colors;
	return &StdPal;
}

void Settings::PaletteSetStdIndexes()
{
	if (!Apps || AppCount < 1)
		return; // Нету никого

	for (int i = 0; i < AppCount; i++)
	{
		int nPalIdx = Apps[i]->GetPaletteIndex();
		if (nPalIdx == -1)
		{
			Apps[i]->nTextColorIdx = AppStd.nTextColorIdx;
			Apps[i]->nBackColorIdx = AppStd.nBackColorIdx;
			Apps[i]->nPopTextColorIdx = AppStd.nPopTextColorIdx;
			Apps[i]->nPopBackColorIdx = AppStd.nPopBackColorIdx;
		}
	}
}

int AppSettings::GetPaletteIndex() const
{
	if (this == nullptr) // *AppSettings
	{
		_ASSERTE(this!=nullptr);
		return -1;
	}
	return gpSet->PaletteGetIndex(szPaletteName);
}

void AppSettings::SetPaletteName(LPCWSTR asNewPaletteName)
{
	if (this == nullptr)
	{
		_ASSERTE(this!=nullptr);
		return;
	}
	lstrcpyn(szPaletteName, asNewPaletteName, countof(szPaletteName));
	ResetPaletteIndex();
}

void AppSettings::ResetPaletteIndex()
{
	// TODO:
}

const ColorPalette* Settings::PaletteGetByName(LPCWSTR asName)
{
	int iPal = PaletteGetIndex(asName);
	return PaletteGet(iPal);
}

// Returns Zero-based palette index, or "-1" when not found
int Settings::PaletteGetIndex(LPCWSTR asName)
{
	if (!Palettes || (PaletteCount < 1) || !asName || !*asName)
		return -1;

	for (int i = 0; i < PaletteCount; i++)
	{
		if (!Palettes[i] || !Palettes[i]->pszName)
			continue;

		if (lstrcmpi(asName, Palettes[i]->pszName) == 0)
			return i;
	}

	return -1;
}

// Returns Zero-based palette index, or "-1" when not found
int Settings::PaletteSetActive(LPCWSTR asName)
{
	int nPalIdx = PaletteGetIndex(asName);

	const ColorPalette* pPal = (nPalIdx != -1) ? PaletteGet(nPalIdx) : nullptr;

	if (pPal)
	{
		for (size_t i = 0; i < pPal->Colors.size(); ++i)
		{
			Colors[i] = pPal->Colors[i]; //-V108
		}

		AppStd.nTextColorIdx = pPal->nTextColorIdx;
		AppStd.nBackColorIdx = pPal->nBackColorIdx;
		AppStd.nPopTextColorIdx = pPal->nPopTextColorIdx;
		AppStd.nPopBackColorIdx = pPal->nPopBackColorIdx;

		mb_FadeInitialized = false;
		GetColors(-1, true);
	}

	return nPalIdx;
}

// Save active colors to named palette
void Settings::PaletteSaveAs(LPCWSTR asName)
{
	PaletteSaveAs(asName,
		AppStd.nTextColorIdx, AppStd.nBackColorIdx,
		AppStd.nPopTextColorIdx, AppStd.nPopBackColorIdx,
		this->Colors, true);
}

void Settings::PaletteSaveAs(LPCWSTR asName,
		BYTE anTextColorIdx, BYTE anBackColorIdx, BYTE anPopTextColorIdx, BYTE anPopBackColorIdx,
		const PaletteColors& aColors, bool abSaveSettings)
{
	// Пользовательские палитры не могут именоваться как "<...>"
	if (!asName || !*asName || wcspbrk(asName, L"<>"))
	{
		DisplayLastError(L"Invalid palette name!\nUser palette name must not contain '<' or '>' symbols.", -1, MB_ICONSTOP, asName ? asName : L"<Null>");
		return;
	}

	int nIndex = PaletteGetIndex(asName);

	// "Предопределенные" палитры перезаписывать нельзя
	if ((nIndex != -1) && (Palettes != nullptr) && Palettes[nIndex]->bPredefined)
		nIndex = -1;

	bool bNewPalette = false;

	if (nIndex == -1)
	{
		// Добавляем новую палитру
		bNewPalette = true;
		nIndex = PaletteCount;

		ColorPalette** ppNew = (ColorPalette**)calloc(nIndex+1,sizeof(ColorPalette*));
		if (!ppNew)
		{
			_ASSERTE(ppNew!=nullptr);
			return;
		}
		if ((PaletteCount > 0) && (Palettes != nullptr))
		{
			memmove(ppNew, Palettes, PaletteCount*sizeof(ColorPalette*));
		}
		SafeFree(Palettes);
		Palettes = ppNew;

		PaletteCount = nIndex+1;
	}

	_ASSERTE(Palettes);

	if (!Palettes[nIndex])
	{
		Palettes[nIndex] = (ColorPalette*)calloc(1,sizeof(ColorPalette));
		if (!Palettes[nIndex])
		{
			_ASSERTE(Palettes[nIndex]);
			return;
		}
		_ASSERTE(bNewPalette && "Already must be set");
		bNewPalette = true;
	}


	// Сохранять допускается только пользовательские палитры
	Palettes[nIndex]->bPredefined = false;
	Palettes[nIndex]->pszName = lstrdup(asName).Detach();

	BOOL bTextChanged = !bNewPalette && ((Palettes[nIndex]->nTextColorIdx != anTextColorIdx) || (Palettes[nIndex]->nBackColorIdx != anBackColorIdx));
	BOOL bPopupChanged = !bNewPalette && ((Palettes[nIndex]->nPopTextColorIdx != anPopTextColorIdx) || (Palettes[nIndex]->nPopBackColorIdx != anPopBackColorIdx));
	Palettes[nIndex]->nTextColorIdx = anTextColorIdx;
	Palettes[nIndex]->nBackColorIdx = anBackColorIdx;
	Palettes[nIndex]->nPopTextColorIdx = anPopTextColorIdx;
	Palettes[nIndex]->nPopBackColorIdx = anPopBackColorIdx;

	_ASSERTE(Palettes[nIndex]->Colors.size() == aColors.size());
	Palettes[nIndex]->Colors = aColors;
	_ASSERTE(nIndex < PaletteCount);

	if (abSaveSettings)
	{
		// Save setting now
		SavePalettes(nullptr);

		// Refresh all consoles
		if (bTextChanged || bPopupChanged)
		{
			gpSetCls->UpdateTextColorSettings(bTextChanged, bPopupChanged);
		}
	}
}

// Delete named palette
void Settings::PaletteDelete(LPCWSTR asName)
{
	int nIndex = PaletteGetIndex(asName);

	// "Предопределенные" палитры удалять нельзя
	if ((nIndex == -1) || Palettes[nIndex]->bPredefined)
		return;

	// Грохнуть
	Palettes[nIndex]->FreePtr();
	// Сдвинуть хвост
	for (int i = (nIndex+1); i < PaletteCount; i++)
	{
		Palettes[i-1] = Palettes[i];
	}
	// Уменьшить количество
	if (PaletteCount > 0)
	{
		Palettes[PaletteCount-1] = nullptr;
		PaletteCount--;
	}

	// Теперь, собственно, пишем настройки
	SavePalettes(nullptr);
}

/* ************************************************************************ */
/* ************************************************************************ */
/* ************************************************************************ */
/* ************************************************************************ */

DWORD Settings::ProgressesGetDuration(LPCWSTR asName)
{
	if (!Progresses || !asName || !*asName)
		return 0;

	for (int i = 0; i < ProgressesCount; i++)
	{
		if (Progresses[i] && Progresses[i]->pszName)
		{
			if (lstrcmpi(Progresses[i]->pszName, asName) == 0)
				return Progresses[i]->nDuration;
		}
	}

	return 0;
}

void Settings::ProgressesSetDuration(LPCWSTR asName, DWORD anDuration)
{
	if (!asName || !*asName)
	{
		_ASSERTE(asName && *asName);
		return;
	}

	if (Progresses)
	{
		for (int i = 0; i < ProgressesCount; i++)
		{
			if (Progresses[i] && Progresses[i]->pszName)
			{
				if (lstrcmpi(Progresses[i]->pszName, asName) == 0)
				{
					Progresses[i]->nDuration = anDuration;
					goto done; // Update settings
				}
			}
		}
	}

	// Extend Array
	{
		ConEmuProgressStore** ppNew = (ConEmuProgressStore**)calloc(ProgressesCount+1,sizeof(ConEmuProgressStore*));
		if (!ppNew)
		{
			_ASSERTE(ppNew!=nullptr);
			return;
		}
		if ((ProgressesCount > 0) && Progresses)
		{
			memmove(ppNew, Progresses, ProgressesCount*sizeof(ConEmuProgressStore*));
		}
		SafeFree(Progresses);
		Progresses = ppNew;
	}

	// Create new entry
	Progresses[ProgressesCount] = (ConEmuProgressStore*)calloc(1,sizeof(ConEmuProgressStore));
	if (!Progresses[ProgressesCount])
	{
		_ASSERTE(Progresses[ProgressesCount]);
		return;
	}

	Progresses[ProgressesCount]->pszName = lstrdup(asName).Detach();
	Progresses[ProgressesCount]->nDuration = anDuration;

	ProgressesCount++;

done:
	// Теперь, собственно, пишем настройки
	SaveProgresses(nullptr);
}

void Settings::LoadProgresses(SettingsBase* reg)
{
	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return;
		}
		lbDelete = true;
	}

	FreeProgresses();

	BOOL lbOpened = FALSE;
	wchar_t szCmdKey[MAX_PATH+64];
	wcscpy_c(szCmdKey, gpSetCls->GetConfigPath());
	wcscat_c(szCmdKey, L"\\Progress");
	wchar_t* pszCmdKey = szCmdKey+lstrlen(szCmdKey);

	int NewProgressesCount = 0;

	lbOpened = reg->OpenKey(szCmdKey, KEY_READ);
	if (lbOpened)
	{
		reg->Load(L"Count", NewProgressesCount);
		reg->CloseKey();
	}

	if (lbOpened && NewProgressesCount > 0)
	{
		Progresses = (ConEmuProgressStore**)calloc(NewProgressesCount, sizeof(ConEmuProgressStore*));

		int nSucceeded = 0;
		for (int i = 0; i < NewProgressesCount; i++)
		{
			swprintf_c(pszCmdKey, 32/*#SECURELEN*/, L"\\Item%i", i+1); // 1-based

			lbOpened = reg->OpenKey(szCmdKey, KEY_READ);
			if (lbOpened)
			{
				if (LoadProgress(reg, Progresses[i]))
					nSucceeded++;

				reg->CloseKey();
			}
		}
		ProgressesCount = nSucceeded;
	}

	if (lbDelete)
		delete reg;
}

bool Settings::SaveProgresses(SettingsBase* reg)
{
	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return false;
		}
		lbDelete = true;
	}

	bool lbRc = false;
	BOOL lbOpened = FALSE;
	wchar_t szCmdKey[MAX_PATH+64];
	wcscpy_c(szCmdKey, gpSetCls->GetConfigPath());
	wcscat_c(szCmdKey, L"\\Progress");
	wchar_t* pszCmdKey = szCmdKey+lstrlen(szCmdKey);

	lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
	if (lbOpened)
	{
		lbRc = true;
		reg->Save(L"Count", ProgressesCount);
		reg->CloseKey();
	}

	if (lbOpened && Progresses && ProgressesCount > 0)
	{
		//int nSucceeded = 0;
		for (int i = 0; i < ProgressesCount; i++)
		{
			swprintf_c(pszCmdKey, 32/*#SECURELEN*/, L"\\Item%i", i+1); // 1-based

			lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
			if (!lbOpened)
			{
				lbRc = false;
			}
			else
			{
				SaveProgress(reg, Progresses[i]);

				reg->CloseKey();
			}
		}
	}

	if (lbDelete)
		delete reg;

	return lbRc;
}

bool Settings::LoadProgress(SettingsBase* reg, Settings::ConEmuProgressStore* &pProgress)
{
	bool lbRc = false;
	ConEmuProgressStore* p = nullptr;

	wchar_t* pszName = nullptr;
	if (!reg->Load(L"Name", &pszName) || !*pszName)
	{
		SafeFree(pszName);
		goto wrap;
	}

	p = (ConEmuProgressStore*)calloc(1, sizeof(ConEmuProgressStore));
	if (!p)
	{
		SafeFree(pszName);
		goto wrap;
	}

	p->pszName = pszName;
	reg->Load(L"Duration", p->nDuration);

wrap:
	pProgress = p;
	return lbRc;
}

bool Settings::SaveProgress(SettingsBase* reg, Settings::ConEmuProgressStore* pProgress)
{
	reg->Save(L"Name", pProgress->pszName);
	reg->Save(L"Duration", pProgress->nDuration);
	return true;
}

void Settings::FreeProgresses()
{
	if (Progresses)
	{
		for (int i = 0; i < ProgressesCount; i++)
		{
			if (Progresses[i])
				Progresses[i]->FreePtr();
		}
		SafeFree(Progresses);
	}
}







/* ************************************************************************ */
/* ************************************************************************ */
/* ************************************************************************ */
/* ************************************************************************ */

void Settings::LoadSettings(bool& rbNeedCreateVanilla, const SettingsStorage* apStorage /*= nullptr*/)
{
	if (!gpConEmu)
	{
		_ASSERTE(gpConEmu);
		return;
	}
	// Log xml/reg + file + config
	CEStr lsDesc(GetStoragePlaceDescr(apStorage, L"Settings::LoadSettings"));
	gpConEmu->LogString(lsDesc.ms_Val);

	// Settings service
	SettingsBase* reg = nullptr;
	bool lbOpened = false;

	// For compatibility
	bool bSendAltEnter = false, bSendAltSpace = false, bSendAltF9 = false;
	bool bInitVanillaFontSizes = true;

	MCHKHEAP
	mb_CharSetWasSet = FALSE;

	// -- перенесено в CheckOptionsFast()
	//;; Q. В Windows Vista зависают другие консольные процессы.
	//	;; A. "Виноват" процесс ConIme.exe. Вроде бы он служит для ввода иероглифов
	//	;;    (китай и т.п.). Зачем он нужен, если ввод теперь идет в графическом окне?
	//	;;    Нужно запретить его автозапуск или вообще переименовать этот файл, например
	//	;;    в 'ConIme.ex1' (видимо это возможно только в безопасном режиме).
	//	;;    Запретить автозапуск: Внесите в реестр и перезагрузитесь
	//if (gOSVer.dwMajorVersion == 6 && gOSVer.dwMinorVersion == 0)
	//{
	//	CheckConIme();
	//}

//-----------------------------------------------------------------------
///| Preload settings actions |//////////////////////////////////////////
//-----------------------------------------------------------------------
	if (gpConEmu->mp_Inside)
	{
		if (gpConEmu->mp_Inside->GetInsideIntegration() == CConEmuInside::ii_Auto)
		{
			HWND hParent = gpConEmu->mp_Inside->InsideFindParent();

			if (hParent == INSIDE_PARENT_NOT_FOUND)
			{
				_ASSERTE(FALSE && "Settings initialization skipped because InsideFindParent was failed");
				goto wrap;
			}
			else if (hParent)
			{
				// Типа, запуститься как панель в Explorer (не в таскбаре, а в проводнике)
				//isStatusBarShow = false;
				isTabs = 0;
				FontSizeY = DEF_FONTSIZEY_P;
				isTryToCenter = true;
				nCenterConsolePad = 3;
				// Скрыть колонки, чтобы много слишком не было...
				isStatusColumnHidden[csi_ConsolePos] = true;
				isStatusColumnHidden[csi_ConsoleSize] = true;
				isStatusColumnHidden[csi_CursorX] = true;
				isStatusColumnHidden[csi_CursorY] = true;
				isStatusColumnHidden[csi_CursorSize] = true;
				isStatusColumnHidden[csi_Server] = true;
			}
		}

		if (gpConEmu->mp_Inside
			&& ((gpConEmu->mp_Inside->GetParentWnd() == nullptr)
				|| ((gpConEmu->mp_Inside->GetParentWnd() != INSIDE_PARENT_NOT_FOUND)
					&& !IsWindow(gpConEmu->mp_Inside->GetParentWnd()))))
		{
			SafeDelete(gpConEmu->mp_Inside);
		}
	}


//-----------------------------------------------------------------------
///| Loading from reg/xml |//////////////////////////////////////////////
//-----------------------------------------------------------------------
	reg = CreateSettings(apStorage);
	if (!reg)
	{
		_ASSERTE(reg!=nullptr);
		goto wrap;
	}

	// Update only if settings storage is default
	if (apStorage == nullptr)
	{
		gpSetCls->Type = reg->m_Storage.Type;
	}

	rbNeedCreateVanilla = false;

	lbOpened = reg->OpenKey(gpSetCls->GetConfigPath(), KEY_READ);
	// Поддержка старого стиля хранения (настройки БЕЗ имени конфига - лежали в корне ключа Software\ConEmu)
	if (!lbOpened && (*gpSetCls->GetConfigName() == 0))
	{
		lbOpened = reg->OpenKey(CONEMU_ROOT_KEY, KEY_READ, TRUE);
		// rbNeedCreateVanilla means we need to convert old xml format (re-save all settings after loading)
		if (lbOpened)
		{
			// We need to check if there is a real config stored in the HKCU\Software\ConEmu
			// but not an installer values (like a ConEmuStartShortcutInstalled).
			CEStr cmd;  BYTE KeybHook = 0xFF;
			if (reg->Load(L"KeyboardHooks", KeybHook)
				|| reg->Load(L"CmdLine", &cmd.ms_Val))
			{
				rbNeedCreateVanilla = true;
			}
			else
			{
				lbOpened = false;
				reg->CloseKey();
			}
		}
	}

	if (rbNeedCreateVanilla)
	{
		// That may be only there was old (not ".Vanilla") settings in the CONEMU_ROOT_KEY
		_ASSERTE(lbOpened);
		// The config was saved in the old format, but it is not a 'new config'.
		gpSetCls->IsConfigNew = false;
	}
	else
	{
		gpSetCls->IsConfigNew = !lbOpened;
		// Здесь можно включить настройки, которые должны включаться только для новых конфигураций!
		if (gpSetCls->IsConfigNew)
		{
			InitVanilla();
		}
	}


	if (lbOpened)
	{
		BYTE nVal;

		// Check, if user has saved font settings before
		{
			DWORD dw = 0; bool b = false;
			// If any of following options were saved before - don't change font size options
			bInitVanillaFontSizes = !(reg->Load(L"FontUseUnits", b)
				|| reg->Load(L"FontSize", dw)
				|| reg->Load(L"StatusFontHeight", dw)
				|| reg->Load(L"TabFontHeight", dw));
		}

		reg->Load(L"Language", Language, countof(Language));

		bool bCmdLine = reg->Load(L"CmdLine", &psStartSingleApp);
		reg->Load(L"StartTasksFile", &psStartTasksFile);
		reg->Load(L"StartTasksName", &psStartTasksName);
		reg->Load(L"StartFarFolders", isStartFarFolders);
		reg->Load(L"StartFarEditors", isStartFarEditors);
		if (!reg->Load(L"StartCreateDelay", nStartCreateDelay))
			nStartCreateDelay = RUNQUEUE_CREATE_LAG_DEF;
		else
			MinMax(nStartCreateDelay, RUNQUEUE_CREATE_LAG_MIN, RUNQUEUE_CREATE_LAG_MAX);
		if (!reg->Load(L"StartType", nStartType))
		{
			if (!bCmdLine)
			{
				// Config is new or partial (template)?
				if (!gpSetCls->IsConfigNew
					&& !(psStartSingleApp || psStartTasksFile || psStartTasksName))
				{
					gpSetCls->IsConfigPartial = true;
				}
			}
			else
			{
				if (*psStartSingleApp == CmdFilePrefix)
					nStartType = 1;
				else if (*psStartSingleApp == TaskBracketLeft)
					nStartType = 2;
			}
		}
		// Check
		if (nStartType > (rbStartLastTabs - rbStartSingleApp))
		{
			_ASSERTE(nStartType <= (rbStartLastTabs - rbStartSingleApp));
			nStartType = 0;
		}

		LoadAppSettings(reg, &AppStd/*, Colors*/);

		reg->Load(L"TrueColorerSupport", isTrueColorer);
		reg->Load(L"VividColors", isVividColors);
		reg->Load(L"FadeInactive", isFadeInactive);
		reg->Load(L"FadeInactiveLow", mn_FadeLow);
		reg->Load(L"FadeInactiveHigh", mn_FadeHigh);

		if (mn_FadeHigh <= mn_FadeLow) { mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH; }
		mn_LastFadeSrc = mn_LastFadeDst = -1;

		// Debugging
		reg->Load(L"ConVisible", isConVisible);


		reg->Load(L"UseInjects", isUseInjects); //MinMax(isUseInjects, BST_INDETERMINATE);

		reg->Load(L"SetDefaultTerminal", isSetDefaultTerminal);
		reg->Load(L"SetDefaultTerminalStartup", isRegisterOnOsStartup);
		reg->Load(L"SetDefaultTerminalStartupTSA", isRegisterOnOsStartupTSA);
		// ReSharper disable once StringLiteralTypo
		reg->Load(L"DefaultTerminalAgressive", isRegisterAggressive);
		reg->Load(L"DefaultTerminalNoInjects", isDefaultTerminalNoInjects);
		reg->Load(L"DefaultTerminalNewWindow", isDefaultTerminalNewWindow);
		reg->Load(L"DefaultTerminalDebugLog", isDefaultTerminalDebugLog);
		// ReSharper disable once CppJoinDeclarationAndAssignment
		nVal = static_cast<BYTE>(nDefaultTerminalConfirmClose);
		if (reg->Load(L"DefaultTerminalConfirm", nVal))
		{
			nDefaultTerminalConfirmClose = static_cast<TerminalConfirmClose>(nVal);
		}

		{
		wchar_t* pszApps = nullptr;
		reg->Load(L"DefaultTerminalApps", &pszApps);
		SetDefaultTerminalApps((pszApps && *pszApps) ? pszApps : DEFAULT_TERMINAL_APPS); // "|"-delimited string -> MSZ
		SafeFree(pszApps);
		}

		reg->Load(L"ProcessAnsi", isProcessAnsi);
		reg->Load(L"AnsiExecution", isAnsiExec); if (isAnsiExec > ansi_Disabled) isAnsiExec = ansi_Disabled;
		if (!psAnsiAllowed) psAnsiAllowed = new CEStr();
		this->LoadMSZ(reg, L"AnsiAllowedCommands", *psAnsiAllowed, L"\r\n", true);

		reg->Load(L"AnsiLog", isAnsiLog);
		reg->Load(L"AnsiLogCodes", isAnsiLogCodes);
		reg->Load(L"AnsiLogPath", &pszAnsiLog);
		reg->Load(L"KillSshAgent", isKillSshAgent);
		reg->Load(L"ProcessNewConArg", isProcessNewConArg);
		reg->Load(L"ProcessCmdStart", isProcessCmdStart);
		reg->Load(L"ProcessCtrlZ", isProcessCtrlZ);

		reg->Load(L"SuppressBells", isSuppressBells);

		reg->Load(L"ConsoleExceptionHandler", isConsoleExceptionHandler);

		reg->Load(L"UseClink", mb_UseClink);

		// Don't move invisible real console. This affects GUI eMenu.
		//reg->Load(L"LockRealConsolePos", isLockRealConsolePos);
		//reg->Load(L"DumpPackets", szDumpPackets);
		reg->Load(L"AutoRegisterFonts", isAutoRegisterFonts);

		reg->Load(L"FontName", inFont, countof(inFont));

		reg->Load(L"FontName2", inFont2, countof(inFont2));

		if (!*inFont || !*inFont2)
			isAutoRegisterFonts = true;

		reg->Load(L"FontBold", isBold);
		reg->Load(L"FontItalic", isItalic);

		if (!reg->Load(L"Monospace", isMonospace))
		{
			// Compatibility. Пережитки прошлого, загрузить по "старым" именам параметров
			bool bForceMonospace = false, bProportional = false;
			reg->Load(L"ForceMonospace", bForceMonospace);
			reg->Load(L"Proportional", bProportional);
			isMonospace = bForceMonospace ? 2 : bProportional ? 0 : 1;
		}
		if (isMonospace > 2) isMonospace = 2;

		reg->Load(L"StoreTaskbarkTasks", isStoreTaskbarkTasks);
		reg->Load(L"StoreTaskbarCommands", isStoreTaskbarCommands);
		reg->Load(L"JumpListAutoUpdate", isJumpListAutoUpdate);

		reg->Load(L"SaveCmdHistory", isSaveCmdHistory);
		HistoryLoad(reg); // L"CmdLineHistory"
		reg->Load(L"SingleInstance", isSingleInstance);
		reg->Load(L"ShowHelpTooltips", isShowHelpTooltips);
		reg->Load(L"Multi", mb_isMulti);
		reg->Load(L"Multi.ShowButtons", isMultiShowButtons);
		reg->Load(L"Multi.ShowSearch", isMultiShowSearch);
		reg->Load(L"Multi.NumberInCaption", isNumberInCaption);
		//LoadVkMod(reg, L"Multi.NewConsole", vmMultiNew, vmMultiNew);
		//LoadVkMod(reg, L"Multi.NewConsoleShift", vmMultiNewShift, SetModifier(vmMultiNew,VK_SHIFT, true/*Xor*/));
		//LoadVkMod(reg, L"Multi.Next", vmMultiNext, vmMultiNext);
		//LoadVkMod(reg, L"Multi.NextShift", vmMultiNextShift, SetModifier(vmMultiNext,VK_SHIFT, true/*Xor*/));
		//LoadVkMod(reg, L"Multi.Recreate", vmMultiRecreate, vmMultiRecreate);
		//LoadVkMod(reg, L"Multi.Close", vmMultiClose, vmMultiClose);
		if (!reg->Load(L"Multi.CloseConfirmFlags", nCloseConfirmFlags))
		{
			bool isCloseConsoleConfirm = false, isCloseEditViewConfirm = false;
			reg->Load(L"Multi.CloseConfirm", isCloseConsoleConfirm);
			reg->Load(L"Multi.CloseEditViewConfirm", isCloseEditViewConfirm);
			nCloseConfirmFlags = cc_Running
				| (isCloseConsoleConfirm ? (cc_Console|cc_Window) : cc_None)
				| (isCloseEditViewConfirm ? (cc_FarEV) : cc_None);
		}
		//LoadVkMod(reg, L"Multi.CmdKey", vmMultiCmd, vmMultiCmd);
		reg->Load(L"Multi.NewConfirm", isMultiNewConfirm);
		reg->Load(L"Multi.DupConfirm", isMultiDupConfirm);
		reg->Load(L"Multi.DetachConfirm", isMultiDetachConfirm);
		//LoadVkMod(reg, L"Multi.Buffer", vmMultiBuffer, vmMultiBuffer);
		reg->Load(L"Multi.UseArrows", isUseWinArrows);
		reg->Load(L"Multi.UseNumbers", isUseWinNumber);
		reg->Load(L"Multi.UseWinTab", isUseWinTab);
		reg->Load(L"Multi.AutoCreate", isMultiAutoCreate);
		reg->Load(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		reg->Load(L"Multi.HideOnClose", isMultiHideOnClose);
		reg->Load(L"Multi.MinByEsc", isMultiMinByEsc); MinMax(isMultiMinByEsc, 2);
		reg->Load(L"MapShiftEscToEsc", isMapShiftEscToEsc);
		reg->Load(L"Multi.Iterate", isMultiIterate);
		//LoadVkMod(reg, L"MinimizeRestore", vmMinimizeRestore, vmMinimizeRestore);
		reg->Load(L"Multi.SplitWidth", nSplitWidth); MinMax(nSplitWidth, MAX_SPLITTER_SIZE);
		reg->Load(L"Multi.SplitHeight", nSplitHeight); MinMax(nSplitHeight, MAX_SPLITTER_SIZE);
		//reg->Load(L"Multi.SplitClr1", nSplitClr1);
		//reg->Load(L"Multi.SplitClr2", nSplitClr2);
		reg->Load(L"ResetTerminalConfirm", isResetTerminalConfirm);

		reg->Load(L"KeyboardHooks", m_isKeyboardHooks); if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;

		reg->Load(L"FontAutoSize", isFontAutoSize);
		reg->Load(L"FontSize", FontSizeY);
		reg->Load(L"FontSizeX", FontSizeX);
		reg->Load(L"FontUseDpi", FontUseDpi);
		reg->Load(L"FontUseUnits", FontUseUnits);
		reg->Load(L"FontSizeX3", FontSizeX3);
		reg->Load(L"FontSizeX2", FontSizeX2);
		reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;

		reg->Load(L"CompressLongStrings", isCompressLongStrings);

		reg->Load(L"Anti-aliasing", mn_AntiAlias);
		// Must be explicitly defined, 0 - not allowed
		if (mn_AntiAlias!=NONANTIALIASED_QUALITY && mn_AntiAlias!=ANTIALIASED_QUALITY && mn_AntiAlias!=CLEARTYPE_NATURAL_QUALITY)
			mn_AntiAlias = NONANTIALIASED_QUALITY;

		reg->Load(L"ConsoleFontName", ConsoleFont.lfFaceName, countof(ConsoleFont.lfFaceName));
		reg->Load(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);

		LoadSizeSettings(reg);

		reg->Load(L"QuakeStyle", isQuakeStyle);
		reg->Load(L"Restore2ActiveMon", isRestore2ActiveMon);
		reg->Load(L"RestoreInactive", isRestoreInactive);
		reg->Load(L"QuakeAnimation", nQuakeAnimation); MinMax(nQuakeAnimation, QUAKEANIMATION_MAX);
		reg->Load(L"HideCaption", isHideCaption);
		reg->Load(L"HideChildCaption", isHideChildCaption);
		reg->Load(L"FocusInChildWindows", isFocusInChildWindows);
		// грузим именно в mb_HideCaptionAlways, т.к. прозрачность сбивает темы в заголовке, поэтому возврат идет через isHideCaptionAlways()
		reg->Load(L"HideCaptionAlways", mb_HideCaptionAlways);
		reg->Load(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame); if (nHideCaptionAlwaysFrame > HIDECAPTIONALWAYSFRAME_MAX) nHideCaptionAlwaysFrame = HIDECAPTIONALWAYSFRAME_DEF;

		reg->Load(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);

		if (nHideCaptionAlwaysDelay > 30000) nHideCaptionAlwaysDelay = 30000;

		reg->Load(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);

		if (nHideCaptionAlwaysDisappear > 30000) nHideCaptionAlwaysDisappear = 30000;

		reg->Load(L"DownShowHiddenMessage", isDownShowHiddenMessage);
		reg->Load(L"DownShowExOnTopMessage", isDownShowExOnTopMessage);

		reg->Load(L"DynamicBufferHeight", isDynamicBufferHeight);

		reg->Load(L"DefaultBufferHeight", DefaultBufferHeight);

		if (DefaultBufferHeight < LONGOUTPUTHEIGHT_MIN)
			DefaultBufferHeight = LONGOUTPUTHEIGHT_MIN;
		else if (DefaultBufferHeight > LONGOUTPUTHEIGHT_MAX)
			DefaultBufferHeight = LONGOUTPUTHEIGHT_MAX;

		reg->Load(L"AutoBufferHeight", AutoBufferHeight);

		reg->Load(L"UseScrollLock", UseScrollLock);

		//reg->Load(L"FarSyncSize", FarSyncSize);

		nVal = ComSpec.csType;
		reg->Load(L"ComSpec.Type", nVal);
		if (nVal <= cst_Last) ComSpec.csType = (ComSpecType)nVal;
		//
		nVal = ComSpec.csBits;
		reg->Load(L"ComSpec.Bits", nVal);
		if (nVal <= csb_Last) ComSpec.csBits = (ComSpecBits)nVal;
		//
		nVal = ComSpec.isUpdateEnv;
		reg->Load(L"ComSpec.UpdateEnv", nVal);
		ComSpec.isUpdateEnv = (nVal != 0);
		//
		nVal = (BYTE)((ComSpec.AddConEmu2Path & CEAP_AddConEmuBaseDir) == CEAP_AddConEmuBaseDir);
		reg->Load(L"ComSpec.EnvAddPath", nVal);
		SetConEmuFlags(ComSpec.AddConEmu2Path, CEAP_AddConEmuBaseDir, nVal ? CEAP_AddConEmuBaseDir : CEAP_None);
		//
		nVal = (BYTE)((ComSpec.AddConEmu2Path & CEAP_AddConEmuExeDir) == CEAP_AddConEmuExeDir);
		reg->Load(L"ComSpec.EnvAddExePath", nVal);
		SetConEmuFlags(ComSpec.AddConEmu2Path, CEAP_AddConEmuExeDir, nVal ? CEAP_AddConEmuExeDir : CEAP_None);
		//
		nVal = ComSpec.isAllowUncPaths;
		reg->Load(L"ComSpec.UncPaths", nVal);
		ComSpec.isAllowUncPaths = (nVal != 0);
		//
		reg->Load(L"ComSpec.Path", ComSpec.ComspecExplicit, countof(ComSpec.ComspecExplicit));
		//-- wcscpy_c(ComSpec.ComspecInitial, gpConEmu->ms_ComSpecInitial);
		//// Обработать 32/64 (найти tcc.exe и т.п.)
		//FindComspec(&ComSpec);
		//Update Comspec(&ComSpec); --> CSettings::SettingsLoaded

		reg->Load(L"AutoReloadEnvironment", AutoReloadEnvironment);
		if (!psEnvironmentSet) psEnvironmentSet = new CEStr(); else psEnvironmentSet->Clear();
		this->LoadMSZ(reg, L"EnvironmentSet", *psEnvironmentSet, L"\r\n", true);

		reg->Load(L"CTS.Intelligent", isCTSIntelligent);
		{
		wchar_t* pszApps = nullptr;
		if (reg->Load(L"CTS.IntelligentExceptions", &pszApps)) // do not reset 'default' settings
			SetIntelligentExceptions(pszApps); // "|"-delimited string -> MSZ
		SafeFree(pszApps);
		}

		reg->Load(L"CTS.AutoCopy", isCTSAutoCopy);
		reg->Load(L"CTS.ResetOnRelease", isCTSResetOnRelease);
		reg->Load(L"CTS.IBeam", isCTSIBeam);
		reg->Load(L"CTS.EndOnTyping", isCTSEndOnTyping); MinMax(isCTSEndOnTyping, 2);
		reg->Load(L"CTS.EndOnKeyPress", isCTSEndOnKeyPress);
		reg->Load(L"CTS.EraseBeforeReset", isCTSEraseBeforeReset);
		reg->Load(L"CTS.Freeze", isCTSFreezeBeforeSelect);
		reg->Load(L"CTS.SelectBlock", isCTSSelectBlock);
		//reg->Load(L"CTS.VkBlock", isCTSVkBlock);
		//LoadVkMod(reg, L"CTS.VkBlockStart", vmCTSVkBlockStart, vmCTSVkBlockStart);
		reg->Load(L"CTS.SelectText", isCTSSelectText);
		reg->Load(L"CTS.HtmlFormat", isCTSHtmlFormat);
		reg->Load(L"CTS.ForceLocale", isCTSForceLocale);

		reg->Load(L"CTS.RBtnAction", reinterpret_cast<BYTE&>(isCTSRBtnAction));
		if (isCTSRBtnAction >= MouseButtonAction::MaxId) isCTSRBtnAction = MouseButtonAction::None;

		reg->Load(L"CTS.MBtnAction", reinterpret_cast<BYTE&>(isCTSMBtnAction));
		if (isCTSMBtnAction >= MouseButtonAction::MaxId) isCTSMBtnAction = MouseButtonAction::None;

		reg->Load(L"CTS.ColorIndex", isCTSColorIndex); if (CONFORECOLOR(isCTSColorIndex) == CONBACKCOLOR(isCTSColorIndex)) isCTSColorIndex = DefaultSelectionConsoleColor;

		reg->Load(L"ClipboardConfirmEnter", isPasteConfirmEnter);
		reg->Load(L"AutoTrimSingleLine", isAutoTrimSingleLine);
		reg->Load(L"ClipboardConfirmLonger", nPasteConfirmLonger);

		reg->Load(L"FarGotoEditorOpt", isFarGotoEditor);
		reg->Load(L"FarGotoEditorPath", &sFarGotoEditor);
		//reg->Load(L"FarGotoEditorVk", isFarGotoEditorVk);

		reg->Load(L"HighlightMouseRow", isHighlightMouseRow);
		reg->Load(L"HighlightMouseCol", isHighlightMouseCol);

		// Previously, the option was used to define different font for Far Manager frames (pseudographics)
		// Now user may use it for any range of characters (CJK, etc.)
		// Used with settings: inFont2, FontSizeX2, mpc_CharAltFontRanges
		reg->Load(L"FixFarBorders", isFixFarBorders);
		// Alternative font antialiasing
		reg->Load(L"Anti-aliasing2", isAntiAlias2);

		//TODO: Extend ranges for arbitrary font groups
		{
		wchar_t* pszCharRanges = nullptr; wchar_t szDefaultRanges[] = L"2013-25C4";
		if (!reg->Load(L"FixFarBordersRanges", &pszCharRanges))
			pszCharRanges = szDefaultRanges;
		ParseCharRanges(pszCharRanges, mpc_CharAltFontRanges);
		if (pszCharRanges && (pszCharRanges != szDefaultRanges))
			free(pszCharRanges);
		}

		reg->Load(L"ExtendUCharMap", isExtendUCharMap);
		reg->Load(L"PartBrush75", isPartBrush75); if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
		reg->Load(L"PartBrush50", isPartBrush50); if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
		reg->Load(L"PartBrush25", isPartBrush25); if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
		reg->Load(L"PartBrushBlack", isPartBrushBlack);

		if (isPartBrush50>=isPartBrush75) isPartBrush50=isPartBrush75-10;

		if (isPartBrush25>=isPartBrush50) isPartBrush25=isPartBrush50-10;

		// Выделим в отдельную настройку
		reg->Load(L"EnhanceGraphics", isEnhanceGraphics);

		reg->Load(L"EnhanceButtons", isEnhanceButtons);

		if (isFixFarBorders == 2 && !isEnhanceGraphics)
		{
			isFixFarBorders = 1;
			isEnhanceGraphics = true;
		}

		reg->Load(L"RightClick opens context menu", isRClickSendKey);
		if (!reg->Load(L"RightClickMacro2", &sRClickMacro) || (sRClickMacro && !*sRClickMacro)) { SafeFree(sRClickMacro); }

		//reg->Load(L"AltEnter", isSendAltEnter);
		//reg->Load(L"AltSpace", isSendAltSpace); //if (isSendAltSpace > 2) isSendAltSpace = 2; // когда-то был 3state, теперь - bool
		reg->Load(L"SendAltTab", isSendAltTab);
		reg->Load(L"SendAltEsc", isSendAltEsc);
		reg->Load(L"SendAltPrintScrn", isSendAltPrintScrn);
		reg->Load(L"SendPrintScrn", isSendPrintScrn);
		reg->Load(L"SendCtrlEsc", isSendCtrlEsc);
		//reg->Load(L"SendAltF9", isSendAltF9);

		// Для совместимости настроек
		reg->Load(L"AltEnter", bSendAltEnter);
		reg->Load(L"AltSpace", bSendAltSpace);
		reg->Load(L"SendAltF9", bSendAltF9);


		reg->Load(L"Min2Tray", mb_MinToTray);
		reg->Load(L"AlwaysShowTrayIcon", mb_AlwaysShowTrayIcon);

		reg->Load(L"SafeFarClose", isSafeFarClose);
		if (!reg->Load(L"SafeFarCloseMacro", &sSafeFarCloseMacro) || (sSafeFarCloseMacro && !*sSafeFarCloseMacro)) { SafeFree(sSafeFarCloseMacro); }

		reg->Load(L"FARuseASCIIsort", isFARuseASCIIsort);
		reg->Load(L"ShellNoZoneCheck", isShellNoZoneCheck);
		reg->Load(L"FixAltOnAltTab", isFixAltOnAltTab);

		reg->Load(L"UseAltGrayPlus", isUseAltGrayPlus);

		reg->Load(L"BackGround Image show", isShowBgImage);
		if (isShowBgImage!=0 && isShowBgImage!=1 && isShowBgImage!=2) isShowBgImage = 0;
		reg->Load(L"BackGround Image", sBgImage, countof(sBgImage));
		reg->Load(L"bgImageDarker", bgImageDarker);
		reg->Load(L"bgImageColors", nBgImageColors);
		if (!nBgImageColors) nBgImageColors = (DWORD)-1; //1|2 == BgImageColorsDefaults;
		reg->Load(L"bgOperation", bgOperation); MinMax(bgOperation, eOpLast);
		reg->Load(L"bgPluginAllowed", isBgPluginAllowed);
		if (isBgPluginAllowed!=0 && isBgPluginAllowed!=1 && isBgPluginAllowed!=2) isBgPluginAllowed = 1;

		reg->Load(L"AlphaValue", nTransparent); MinMax(nTransparent, MIN_ALPHA_VALUE, 255);
		reg->Load(L"AlphaValueSeparate", isTransparentSeparate);
		if (!reg->Load(L"AlphaValueInactive", nTransparentInactive))
		{
			nTransparentInactive = nTransparent;
		}
		else
		{
			MinMax(nTransparentInactive, MIN_INACTIVE_ALPHA_VALUE, 255);
		}

		//reg->Load(L"UseColorKey", isColorKey);
		//reg->Load(L"ColorKey", ColorKey);
		reg->Load(L"UserScreenTransparent", isUserScreenTransparent);

		reg->Load(L"ColorKeyTransparent", isColorKeyTransparent);
		reg->Load(L"ColorKeyValue", nColorKeyValue); nColorKeyValue = nColorKeyValue & 0xFFFFFF;


		//reg->Load(L"Update Console handle", isUpdConHandle);
		reg->Load(L"DisableMouse", isDisableMouse);
		reg->Load(L"RSelectionFix", isRSelFix);
		reg->Load(L"MouseSkipActivation", isMouseSkipActivation);
		reg->Load(L"MouseSkipMoving", isMouseSkipMoving);
		reg->Load(L"MouseDragWindow", isMouseDragWindow);
		reg->Load(L"FarWheelScrollback", isFarWheelScrollback);
		reg->Load(L"FarHourglass", isFarHourglass);
		reg->Load(L"FarHourglassDelay", nFarHourglassDelay);

		reg->Load(L"Dnd", isDragEnabled);
		isDropEnabled = (BYTE)(isDragEnabled ? 1 : 0); // ранее "DndDrop" не было, поэтому ставим default
		//reg->Load(L"DndLKey", nLDragKey);
		//reg->Load(L"DndRKey", nRDragKey);
		reg->Load(L"DndDrop", isDropEnabled);
		reg->Load(L"DefCopy", isDefCopy);
		reg->Load(L"DropUseMenu", isDropUseMenu);
		reg->Load(L"DragOverlay", isDragOverlay);
		reg->Load(L"DragShowIcons", isDragShowIcons);

		reg->Load(L"DragPanel", isDragPanel); if (isDragPanel > 2) isDragPanel = 1;
		reg->Load(L"DragPanelBothEdges", isDragPanelBothEdges);

		reg->Load(L"KeyBarRClick", isKeyBarRClick);

		reg->Load(L"DebugSteps", isDebugSteps);

		reg->Load(L"DebugLog", isDebugLog);

		mb_StatusSettingsWasChanged = false;
		reg->Load(L"StatusBar.Show", isStatusBarShow);
		reg->Load(L"StatusBar.Flags", isStatusBarFlags);
		reg->Load(L"StatusFontFace", sStatusFontFace, countof(sStatusFontFace));
		reg->Load(L"StatusFontCharSet", nStatusFontCharSet);
		reg->Load(L"StatusFontHeight", nStatusFontHeight); nStatusFontHeight = std::max(4,nStatusFontHeight);
		reg->Load(L"StatusBar.Color.Back", nStatusBarBack);
		reg->Load(L"StatusBar.Color.Light", nStatusBarLight);
		reg->Load(L"StatusBar.Color.Dark", nStatusBarDark);
		_ASSERTE(countof(isStatusColumnHidden)>csi_Last);
		_ASSERTE(csi_Info==0);
		//reg->Load(L"StatusBar.HideColumns", nHideStatusColumns);
		for (int i = csi_Info; i < csi_Last; i++)
		{
			LPCWSTR pszName = gpConEmu->mp_Status->GetSettingName((CEStatusItems)i);
			if (pszName)
				reg->Load(pszName, isStatusColumnHidden[i]);
			else
				isStatusColumnHidden[i] = false;
		}

		//reg->Load(L"GUIpb", isGUIpb);
		reg->Load(L"Tabs", isTabs);
		reg->Load(L"TabsLocation", nTabsLocation);
		reg->Load(L"TabIcons", isTabIcons);
		reg->Load(L"OneTabPerGroup", isOneTabPerGroup);
		reg->Load(L"ActivateSplitMouseOver", bActivateSplitMouseOver); MinMax(bActivateSplitMouseOver, 2);
		reg->Load(L"TabSelf", isTabSelf);
		reg->Load(L"TabLazy", isTabLazy);
		reg->Load(L"TabFlashChanged", nTabFlashChanged);
		reg->Load(L"TabRecent", isTabRecent);
		reg->Load(L"TabDblClick", nTabBarDblClickAction); MinMax(nTabBarDblClickAction, TabBarDblClick::Last-1);
		reg->Load(L"TabBtnDblClick", nTabBtnDblClickAction); MinMax(nTabBtnDblClickAction, TabBtnDblClick::Last-1);
		reg->Load(L"TabsOnTaskBar", m_isTabsOnTaskBar);
		reg->Load(L"TaskBarOverlay", isTaskbarOverlay);
		reg->Load(L"TaskbarProgress", isTaskbarProgress);

		if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { SafeFree(sTabCloseMacro); }

		reg->Load(L"TabFontFace", sTabFontFace, countof(sTabFontFace));
		reg->Load(L"TabFontCharSet", nTabFontCharSet);
		reg->Load(L"TabFontHeight", nTabFontHeight); nTabFontHeight = std::max(4,nTabFontHeight);

		if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro) || (sSaveAllMacro && !*sSaveAllMacro)) { SafeFree(sSaveAllMacro); }

		//reg->Load(L"TabFrame", isTabFrame);
		//reg->Load(L"TabMargins", rcTabMargins);

		reg->Load(L"ToolbarAddSpace", nToolbarAddSpace);
		if (nToolbarAddSpace<0 || nToolbarAddSpace>100) nToolbarAddSpace = 0;

		reg->Load(L"SlideShowElapse", nSlideShowElapse);

		reg->Load(L"IconID", nIconID);


		reg->Load(L"TabConsole", szTabConsole, countof(szTabConsole));
		reg->Load(L"TabModifiedSuffix", szTabModifiedSuffix, countof(szTabModifiedSuffix));
		reg->Load(L"TabSkipWords", &pszTabSkipWords);
		wcscpy_c(szTabPanels, szTabConsole); // Раньше была только настройка "TabConsole". Унаследовать ее в "TabPanels"
		reg->Load(L"TabPanels", szTabPanels, countof(szTabPanels));
		reg->Load(L"TabEditor", szTabEditor, countof(szTabEditor));
		reg->Load(L"TabEditorModified", szTabEditorModified, countof(szTabEditorModified));
		reg->Load(L"TabViewer", szTabViewer, countof(szTabViewer));
		reg->Load(L"TabLenMax", nTabLenMax); if (nTabLenMax < 10 || nTabLenMax >= CONEMUTABMAX) nTabLenMax = 20;

		/*reg->Load(L"ScrollTitle", isScrollTitle);
		reg->Load(L"ScrollTitleLen", ScrollTitleLen);*/

		// Old style:
		// * Disabled: bAdminShield = false, szAdminTitleSuffix = ""
		// * Shield:   bAdminShield = true,  szAdminTitleSuffix ignored (may be filled!)
		// * Suffix:   bAdminShield = false, szAdminTitleSuffix = " (Admin)"
		// New style:
		// * Disabled: bAdminShield = false, szAdminTitleSuffix = ""
		// * Shield:   bAdminShield = true,  szAdminTitleSuffix ignored (may be filled!)
		// * Suffix:   bAdminShield = false, szAdminTitleSuffix = " (Admin)"
		// * Shld+Suf: bAdminShield = 3,     szAdminTitleSuffix = " (Admin)"
		reg->Load(L"AdminTitleSuffix", szAdminTitleSuffix, countof(szAdminTitleSuffix)); szAdminTitleSuffix[countof(szAdminTitleSuffix)-1] = 0;
		reg->Load(L"AdminShowShield", bAdminShield); if (!bAdminShield && !*szAdminTitleSuffix) bAdminShield = ats_Disabled;

		reg->Load(L"HideInactiveConsoleTabs", bHideInactiveConsoleTabs);
		reg->Load(L"ShowFarWindows", bShowFarWindows);

		reg->Load(L"TryToCenter", isTryToCenter);
		reg->Load(L"CenterConsolePad", nCenterConsolePad); MinMax(nCenterConsolePad,CENTERCONSOLEPAD_MIN,CENTERCONSOLEPAD_MAX);

		reg->Load(L"ShowScrollbar", isAlwaysShowScrollbar); MinMax(isAlwaysShowScrollbar,2);
		reg->Load(L"ScrollBarAppearDelay", nScrollBarAppearDelay); MinMax(nScrollBarAppearDelay,SCROLLBAR_DELAY_MIN,SCROLLBAR_DELAY_MAX);
		reg->Load(L"ScrollBarDisappearDelay", nScrollBarDisappearDelay); MinMax(nScrollBarDisappearDelay,SCROLLBAR_DELAY_MIN,SCROLLBAR_DELAY_MAX);

		//reg->Load(L"CreateAppWindow", isCreateAppWindow);
		//reg->Load(L"AllowDetach", isAllowDetach);

		reg->Load(L"MainTimerElapse", nMainTimerElapse); if (nMainTimerElapse>1000) nMainTimerElapse = 1000;
		reg->Load(L"MainTimerInactiveElapse", nMainTimerInactiveElapse); if (nMainTimerInactiveElapse>10000) nMainTimerInactiveElapse = 10000;

		reg->Load(L"AffinityMask", nAffinity);

		//reg->Load(L"AdvLangChange", isAdvLangChange);

		reg->Load(L"SkipFocusEvents", isSkipFocusEvents);

		//reg->Load(L"LangChangeWsPlugin", isLangChangeWsPlugin);
		reg->Load(L"MonitorConsoleLang", isMonitorConsoleLang);
		reg->Load(L"SnapToDesktopEdges", isSnapToDesktopEdges);
		reg->Load(L"AlwaysOnTop", isAlwaysOnTop);
		reg->Load(L"SleepInBackground", isSleepInBackground);
		reg->Load(L"RetardInactivePanes", isRetardInactivePanes);
		reg->Load(L"MinimizeOnLoseFocus", mb_MinimizeOnLoseFocus);

		reg->Load(L"DisableFarFlashing", isDisableFarFlashing); if (isDisableFarFlashing>2) isDisableFarFlashing = 2;

		reg->Load(L"DisableAllFlashing", isDisableAllFlashing); if (isDisableAllFlashing>2) isDisableAllFlashing = 2;

		// FindText: bMatchCase, bMatchWholeWords, bFreezeConsole, bHighlightAll
		// FindOptions.pszText may be used to pre-fill search dialog field if search-bar is hidden
		reg->Load(L"FindText", &FindOptions.text.ms_Val);
		reg->Load(L"FindMatchCase", FindOptions.bMatchCase);
		reg->Load(L"FindMatchWholeWords", FindOptions.bMatchWholeWords);
		reg->Load(L"FindFreezeConsole", FindOptions.bFreezeConsole);
		reg->Load(L"FindHighlightAll", FindOptions.bHighlightAll);
		reg->Load(L"FindTransparent", FindOptions.bTransparent);


		/* *********** Thumbnails and Tiles ************* */
		reg->Load(L"PanView.BackColor", ThSet.crBackground.RawColor);

		reg->Load(L"PanView.PFrame", ThSet.nPreviewFrame); if (ThSet.nPreviewFrame!=0 && ThSet.nPreviewFrame!=1) ThSet.nPreviewFrame = 1;

		reg->Load(L"PanView.PFrameColor", ThSet.crPreviewFrame.RawColor);

		reg->Load(L"PanView.SFrame", ThSet.nSelectFrame); if (ThSet.nSelectFrame!=0 && ThSet.nSelectFrame!=1) ThSet.nSelectFrame = 1;

		reg->Load(L"PanView.SFrameColor", ThSet.crSelectFrame.RawColor);
		/* теперь разнообразные размеры */
		ThumbLoadSet(L"Thumbs", ThSet.Thumbs);
		ThumbLoadSet(L"Tiles", ThSet.Tiles);
		// Прочие параметры загрузки
		reg->Load(L"PanView.LoadPreviews", ThSet.bLoadPreviews);
		reg->Load(L"PanView.LoadFolders", ThSet.bLoadFolders);
		reg->Load(L"PanView.LoadTimeout", ThSet.nLoadTimeout);
		reg->Load(L"PanView.MaxZoom", ThSet.nMaxZoom);
		reg->Load(L"PanView.UsePicView2", ThSet.bUsePicView2);
		reg->Load(L"PanView.RestoreOnStartup", ThSet.bRestoreOnStartup);

		/* *** AutoUpdate *** */
		reg->Load(L"Update.VerLocation", &UpdSet.szUpdateVerLocation);
		reg->Load(L"Update.CheckOnStartup", UpdSet.isUpdateCheckOnStartup);
		reg->Load(L"Update.CheckHourly", UpdSet.isUpdateCheckHourly);
		reg->Load(L"Update.ConfirmDownload", UpdSet.isUpdateConfirmDownload);
		reg->Load(L"Update.UseBuilds", (BYTE&)UpdSet.isUpdateUseBuilds);
		if (UpdSet.isUpdateUseBuilds < ConEmuUpdateSettings::Builds::Stable || UpdSet.isUpdateUseBuilds > ConEmuUpdateSettings::Builds::Preview)
			UpdSet.isUpdateUseBuilds = ConEmuUpdateSettings::GetDefaultUpdateChannel();
		reg->Load(L"Update.InetTool", UpdSet.isUpdateInetTool);
		reg->Load(L"Update.InetToolCmd", &UpdSet.szUpdateInetTool);
		reg->Load(L"Update.UseProxy", UpdSet.isUpdateUseProxy);
		reg->Load(L"Update.Proxy", &UpdSet.szUpdateProxy);
		reg->Load(L"Update.ProxyUser", &UpdSet.szUpdateProxyUser);
		reg->Load(L"Update.ProxyPassword", &UpdSet.szUpdateProxyPassword);
		//It is not saved to settings, so must not be loaded from
		//reg->Load(L"Update.DownloadMode", UpdSet.isUpdateDownloadSetup); // 1-Installer (ConEmuSetup.exe), 2-7z archive (ConEmu.7z), WinRar or 7z required
		UpdSet.isUpdateDownloadSetup = 0; // Use 'Auto' detection
		reg->Load(L"Update.ExeCmdLine", &UpdSet.szUpdateExeCmdLine);
		reg->Load(L"Update.ArcCmdLine", &UpdSet.szUpdateArcCmdLine);
		reg->Load(L"Update.DownloadPath", &UpdSet.szUpdateDownloadPath);
		reg->Load(L"Update.LeavePackages", UpdSet.isUpdateLeavePackages);
		reg->Load(L"Update.PostUpdateCmd", &UpdSet.szUpdatePostUpdateCmd);

		/* *** Notification *** */
		reg->Load(L"Notification.StopBuzzingDate", StopBuzzingDate, countof(StopBuzzingDate));

		/* Done */
		reg->CloseKey();

		/* Load all children objects */
		LoadHotkeys(reg, bSendAltEnter, bSendAltSpace, bSendAltF9);
		LoadPalettes(reg);
		LoadAppsSettings(reg);
		LoadCmdTasks(reg);
	}

	// сервис больше не нужен
	delete reg;
	reg = nullptr;

wrap:
	// In some cases for some options we must apply vanilla defaults
	if (bInitVanillaFontSizes)
	{
		// For example, user had pressed ‘Esc’ in the ‘Fast settings’ dialog
		// So, config will be not totally ‘new’ but font settings are ‘new’ actually
		InitVanillaFontSettings();
	}
}

void Settings::LoadSizeSettings(SettingsBase* reg)
{
	reg->Load(L"UseCurrentSizePos", isUseCurrentSizePos);
	reg->Load(L"AutoSaveSizePos", isAutoSaveSizePos);
	reg->Load(L"Cascaded", wndCascade);
	reg->Load(L"IntegralSize", mb_IntegralSize);

	reg->Load(L"WindowMode", _WindowMode);
	switch (_WindowMode)
	{
	case wmNormal:
	case wmMaximized:
	case wmFullScreen:
		break; // OK
	default:
		// Legacy support (if wmFullScreen!=rFullScreen and so on)?
		switch (_WindowMode)
		{
		case rFullScreen:
			_WindowMode = wmFullScreen; break;
		case rMaximized:
			_WindowMode = wmMaximized; break;
		default:
			_WindowMode = wmNormal;
		}
	}

	reg->Load(L"ConWnd X", _wndX);
	reg->Load(L"ConWnd Y", _wndY);

	// Load monitor information: "0,0,1280,960"
	bool bMonitorOk = reg->Load(L"LastMonitor", LastMonRect);

	// Fill monitor information with defaults
	if (!bMonitorOk)
	{
		MONITORINFO mi = {sizeof(mi)}; HMONITOR hLastMon;
		// Avoid to call our evaluation function (they rely on monitor information)
		RECT rcDef = {_wndX, _wndY, _wndX+500, _wndY+300};
		if (((hLastMon = MonitorFromRect(&rcDef, MONITOR_DEFAULTTONULL)) != nullptr)
			&& GetMonitorInfo(hLastMon, &mi))
		{
			if (_WindowMode == wmFullScreen)
				LastMonRect = mi.rcMonitor;
			else
				LastMonRect = mi.rcWork;
		}
	}


	// ЭТО не влияет на szDefCmd. Только прямое указание флажка "/BufferHeight N"
	// может сменить (умолчательную) команду запуска на "cmd" или "far"
	DWORD sizeRaw = wndWidth.Raw;
	reg->Load(L"ConWnd Width", sizeRaw); wndWidth.SetFromRaw(true, sizeRaw);
	sizeRaw = wndHeight.Raw;
	reg->Load(L"ConWnd Height", sizeRaw); wndHeight.SetFromRaw(false, sizeRaw);

	if (gpSet->isLogging())
	{
		wchar_t szInfo[120]; swprintf_c(szInfo, L"Loaded pos: {%i,%i}, size: {%s,%s}", _wndX, _wndY, wndWidth.AsString(), wndHeight.AsString());
		gpConEmu->LogString(szInfo);
	}

	reg->Load(L"16bit Height", ntvdmHeight);
	if (ntvdmHeight != 0 && ntvdmHeight != 25 && ntvdmHeight != 28 && ntvdmHeight != 43 && ntvdmHeight != 50) ntvdmHeight = 25;
}

void Settings::SaveSizeSettings(SettingsBase* reg)
{
	DWORD saveMode = (isUseCurrentSizePos == false) ? _WindowMode // save what user's specified explicitly
		: ((ghWnd == nullptr)  // otherwise - save current state
			? gpConEmu->GetWindowMode()
			: (gpConEmu->isFullScreen() ? wmFullScreen : gpConEmu->isZoomed() ? wmMaximized : wmNormal));

	reg->Save(L"UseCurrentSizePos", isUseCurrentSizePos);
	reg->Save(L"AutoSaveSizePos", isAutoSaveSizePos);
	reg->Save(L"Cascaded", wndCascade);
	reg->Save(L"IntegralSize", mb_IntegralSize);

	reg->Save(L"WindowMode", saveMode);

	// Avoid to call our evaluation function (they rely on monitor information)
	RECT rcWnd = {isUseCurrentSizePos ? gpConEmu->WndPos.x : _wndX, isUseCurrentSizePos ? gpConEmu->WndPos.y : _wndY};
	rcWnd.right = rcWnd.left + 500; rcWnd.bottom = rcWnd.top + 300;
	HMONITOR hMon = MonitorFromRect(&rcWnd, MONITOR_DEFAULTTONULL);
	MONITORINFO mi = {sizeof(mi)};
	if (!hMon || !GetMonitorInfoSafe(hMon, mi))
		ZeroStruct(mi);
	if (saveMode == wmFullScreen)
		LastMonRect = mi.rcMonitor;
	else
		LastMonRect = mi.rcWork;

	reg->Save(L"ConWnd X", rcWnd.left);
	reg->Save(L"ConWnd Y", rcWnd.top);

	// "LastMonitor" = "0,0,1280,960"
	reg->Save(L"LastMonitor", LastMonRect);

	reg->Save(L"ConWnd Width", isUseCurrentSizePos ? gpConEmu->WndWidth.Raw : wndWidth.Raw);
	reg->Save(L"ConWnd Height", isUseCurrentSizePos ? gpConEmu->WndHeight.Raw : wndHeight.Raw);
	reg->Save(L"16bit Height", ntvdmHeight);
}

void Settings::SaveSettingsOnExit()
{
	if (!this)
		return;

	// При закрытии окна крестиком - сохранять только один раз!
	// а то размер/таски/настройки могут в процессе закрытия консолей измениться
	if (mb_ExitSettingsAutoSaved)
		return;
	mb_ExitSettingsAutoSaved = true;

	AutoSaveSettings(nullptr, true);
}

void Settings::OnAutoSaveTimer()
{
	// Allowed to save?
	if (!gpConEmu || gpConEmu->GetStartupStage() != CConEmuMain::ss_VConStarted)
		return;
	// Are there something to save?
	if (!IsAutoSaveSettings(false))
		return;

	// Do the timing
	static DWORD dwLastTick = 0;
	const  DWORD dwAutosaveDelay = 5*60*1000; // 5 minutes
	if (!dwLastTick)
	{
		dwLastTick = GetTickCount();
		return;
	}
	DWORD dwDelay = (GetTickCount() - dwLastTick);
	// Avoid trying to save at the same time from several instances
	if (dwDelay > (dwAutosaveDelay + (rand() % 60)))
	{
		dwLastTick = GetTickCount();
		// Save only startup task
		AutoSaveSettings(nullptr, false);
	}
}

bool Settings::IsAutoSaveSettings(bool saveAll)
{
	bool bTaskAutoSave = (nStartType == (rbStartLastTabs - rbStartSingleApp));

	// What we save on exit?
	if (saveAll && !(isAutoSaveSizePos || mb_StatusSettingsWasChanged || bTaskAutoSave))
		return false;
	// What we save on timer?
	else if (!saveAll && !(bTaskAutoSave))
		return false;

	return true;
}

void Settings::AutoSaveSettings(SettingsBase* reg/* = nullptr*/, bool saveAll/* = false*/)
{
	if (!this)
		return;

	// В некоторых случаях сохранение опций при выходе не допустимо
	if (!gpConEmu->IsAllowSaveSettingsOnExit())
		return;

	// Do we need to save somethings?
	if (!((reg && saveAll) // save from settings dialog
			|| IsAutoSaveSettings(saveAll))) // onTimer and onExit
		return;

	bool bTaskAutoSave = (nStartType == (rbStartLastTabs - rbStartSingleApp));

	gpConEmu->LogWindowPos(L"AutoSaveSettings");

	bool lbDelete = false;
	if (!reg)
	{
		reg = CreateSettings(nullptr);
		if (!reg)
		{
			gpConEmu->LogWindowPos(L"AutoSaveSettings - FAILED(CreateSettings)");
			_ASSERTE(reg!=nullptr);
			// Avoid further errors?
			// gpSetCls->ibDisableSaveSettingsOnExit = true;
			return;
		}
		lbDelete = true;
	}

	// #AutoSave Avoid share violation error on save via named Mutex?

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		if (saveAll && isAutoSaveSizePos)
		{
			SaveSizeSettings(reg);
		}

		if (saveAll && mb_StatusSettingsWasChanged)
		{
			SaveStatusSettings(reg);
		}

		if (bTaskAutoSave)
		{
			BYTE curStartType = -1;
			if (!reg->Load(L"StartType", curStartType)
				|| curStartType != nStartType)
				reg->Save(L"StartType", nStartType);
		}

		reg->CloseKey();

		// Таски пишутся в отдельный ключ
		if (bTaskAutoSave)
		{
			CEStr pszTabs = CVConGroup::GetTasks();
			if (pszTabs)
			{
				BOOL lbOpened = FALSE;
				wchar_t szCmdKey[MAX_PATH+64];
				wcscpy_c(szCmdKey, gpSetCls->GetConfigPath());
				wcscat_c(szCmdKey, L"\\Tasks\\");
				wcscat_c(szCmdKey, AutoStartTaskName);
				lbOpened = reg->OpenKey(szCmdKey, KEY_WRITE);
				if (lbOpened)
				{
					bool taskChanged = false;

					if (!StartupTask)
					{
						StartupTask = (CommandTasks*)calloc(1, sizeof(CommandTasks));
						taskChanged = true;
					}

					if (StartupTask)
					{
						wchar_t szConfig[300] = L"";
						const wchar_t* pszConfigName = gpSetCls->GetConfigName();
						if (pszConfigName && *pszConfigName)
						{
							swprintf_c(szConfig, L"/config \"%s\"", pszConfigName);
						}

						taskChanged |= StartupTask->SetGuiArg(szConfig);

						taskChanged |= StartupTask->SetCommands(pszTabs);

						if (taskChanged)
						{
							StartupTask->SaveCmdTask(reg, true);
						}
					}

					reg->CloseKey();
				}
				else
				{
					gpConEmu->LogWindowPos(L"AutoSaveSettings - FAILED(OpenKey(AutoStartTaskName, KEY_WRITE))");
				}
			}
		}
	}
	else
	{
		gpConEmu->LogWindowPos(L"AutoSaveSettings - FAILED(OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))");
	}

	if (lbDelete)
		delete reg;
}

void Settings::SaveStopBuzzingDate()
{
	SYSTEMTIME st = {}; GetLocalTime(&st);
	swprintf_c(StopBuzzingDate, L"%u-%u-%u", st.wYear, st.wMonth, st.wDay);

	// -basic switch?
	if (gpConEmu->IsResetBasicSettings())
	{
		RegSetStringValue(HKEY_CURRENT_USER, CONEMU_ROOT_KEY, L"Notification.StopBuzzingDate", StopBuzzingDate);
		return;
	}

	SettingsBase* reg = CreateSettings(nullptr);
	if (!reg)
	{
		gpConEmu->LogWindowPos(L"SaveStopBuzzingDate - FAILED(CreateSettings)");
		_ASSERTE(reg!=nullptr);
		return;
	}

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		reg->Save(L"Notification.StopBuzzingDate", StopBuzzingDate);
	}

	delete reg;
}

void Settings::SaveConsoleFont()
{
	if (!this)
		return;

	SettingsBase* reg = CreateSettings(nullptr);
	if (!reg)
	{
		_ASSERTE(reg!=nullptr);
		return;
	}

	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
	{
		reg->Save(L"ConsoleFontName", ConsoleFont.lfFaceName);
		reg->Save(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Save(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		reg->CloseKey();
	}

	delete reg;
}

void Settings::SaveFindOptions(SettingsBase* reg/* = nullptr*/)
{
	if (!reg && gpConEmu->IsResetBasicSettings())
		return;

	bool bDelete = (reg == nullptr);
	if (!reg)
	{
		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return;
		}

		if (!reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE, TRUE))
		{
			delete reg;
			return;
		}
	}

	reg->Save(L"FindText", FindOptions.text.c_str());
	reg->Save(L"FindMatchCase", FindOptions.bMatchCase);
	reg->Save(L"FindMatchWholeWords", FindOptions.bMatchWholeWords);
#if 0
	// пока не работает - не сохраняем
	reg->Save(L"FindFreezeConsole", FindOptions.bFreezeConsole);
	reg->Save(L"FindHighlightAll", FindOptions.bHighlightAll);
#endif
	reg->Save(L"FindTransparent", FindOptions.bTransparent);

	if (bDelete)
	{
		reg->CloseKey();
		delete reg;
	}
}

void Settings::SaveAppsSettings(SettingsBase* reg)
{
	BOOL lbOpened = FALSE;
	int OldAppCount = 0;
	wchar_t szAppKey[MAX_PATH+64];
	wcscpy_c(szAppKey, gpSetCls->GetConfigPath());
	wcscat_c(szAppKey, L"\\Apps");
	wchar_t* pszAppKey = szAppKey+lstrlen(szAppKey);

	lbOpened = reg->OpenKey(szAppKey, KEY_WRITE);
	if (lbOpened)
	{
		reg->Load(L"Count", OldAppCount);
		reg->Save(L"Count", AppCount);
		reg->CloseKey();
	}

	for (int i = 0; i < AppCount; i++)
	{
		swprintf_c(pszAppKey, 32/*#SECURELEN*/, L"\\App%i", i+1);

		lbOpened = reg->OpenKey(szAppKey, KEY_WRITE);
		if (lbOpened)
		{
			// Загрузка "AppNames" - снаружи, т.к. LoadAppSettings используется и для загрузки &AppStd
			reg->Save(L"AppNames", Apps[i]->AppNames);
			reg->Save(L"Elevated", Apps[i]->Elevated);
			SaveAppSettings(reg, Apps[i]/*, Цвета сохраняются как Имя палитры*/);
			reg->CloseKey();
		}
	}

	if (OldAppCount > AppCount)
	{
		*pszAppKey = 0;
		if (reg->OpenKey(szAppKey, KEY_WRITE))
		{
			wchar_t szDelAppKey[20];
			for (int i = AppCount; i < OldAppCount; i++)
			{
				swprintf_c(szDelAppKey, L"App%i", i+1);
				reg->DeleteKey(szDelAppKey);
			}
			reg->CloseKey();
		}
	}
}

void Settings::SaveStdColors(SettingsBase* reg)
{
	TCHAR ColorName[] = L"ColorTable00";
	const size_t digitPos = _tcslen(ColorName) - 2;

	for (size_t i = 0; i < Colors.size()/*0x10*/; ++i)
	{
		ColorName[digitPos] = TCHAR(i / 10 + '0');
		ColorName[digitPos + 1] = TCHAR(i % 10 + '0');
		reg->Save(ColorName, static_cast<DWORD>(Colors[i]));
	}

	reg->Save(L"TextColorIdx", AppStd.nTextColorIdx);
	reg->Save(L"BackColorIdx", AppStd.nBackColorIdx);
	reg->Save(L"PopTextColorIdx", AppStd.nPopTextColorIdx);
	reg->Save(L"PopBackColorIdx", AppStd.nPopBackColorIdx);
}

void Settings::SaveAppSettings(SettingsBase* reg, AppSettings* pApp/*, COLORREF* pColors*/)
{
	// Для AppStd данные загружаются из основной ветки! В том числе и цвета (RGB[32] а не имя палитры)
	bool bStd = (pApp == &AppStd);

	if (bStd)
	{
		SaveStdColors(reg);
	}
	else
	{
		reg->Save(L"OverridePalette", pApp->OverridePalette);
		reg->Save(L"PaletteName", pApp->szPaletteName);
	}

	if (!bStd)
		reg->Save(L"OverrideExtendFonts", pApp->OverrideExtendFonts);
	reg->Save(L"ExtendFonts", pApp->isExtendFonts);
	reg->Save(L"ExtendFontNormalIdx", pApp->nFontNormalColor);
	reg->Save(L"ExtendFontBoldIdx", pApp->nFontBoldColor);
	reg->Save(L"ExtendFontItalicIdx", pApp->nFontItalicColor);

	if (!bStd)
		reg->Save(L"OverrideCursor", pApp->OverrideCursor);
	reg->Save(L"CursorTypeActive", pApp->CursorActive.Raw);
	reg->Save(L"CursorTypeInactive", pApp->CursorInactive.Raw);

	if (!bStd)
		reg->Save(L"OverrideClipboard", pApp->OverrideClipboard);
	reg->Save(L"ClipboardDetectLineEnd", pApp->isCTSDetectLineEnd);
	reg->Save(L"ClipboardBashMargin", pApp->isCTSBashMargin);
	reg->Save(L"ClipboardTrimTrailing", pApp->isCTSTrimTrailing);
	reg->Save(L"ClipboardEOL", pApp->isCTSEOL);
	reg->Save(L"ClipboardArrowStart", pApp->isCTSShiftArrowStart);
	reg->Save(L"ClipboardAllLines", pApp->isPasteAllLines);
	reg->Save(L"ClipboardAllLinesPosix", pApp->isPosixAllLines);
	reg->Save(L"ClipboardFirstLine", pApp->isPasteFirstLine);
	reg->Save(L"ClipboardFirstLinePosix", pApp->isPosixFirstLine);
	reg->Save(L"ClipboardClickPromptPosition", pApp->isCTSClickPromptPosition);
	reg->Save(L"ClipboardDeleteLeftWord", pApp->isCTSDeleteLeftWord);
}

void Settings::SaveStatusSettings(SettingsBase* reg)
{
	mb_StatusSettingsWasChanged = false;
	reg->Save(L"StatusBar.Show", isStatusBarShow);
	reg->Save(L"StatusBar.Flags", isStatusBarFlags);
	reg->Save(L"StatusFontFace", sStatusFontFace);
	reg->Save(L"StatusFontCharSet", nStatusFontCharSet);
	reg->Save(L"StatusFontHeight", nStatusFontHeight);
	reg->Save(L"StatusBar.Color.Back", nStatusBarBack);
	reg->Save(L"StatusBar.Color.Light", nStatusBarLight);
	reg->Save(L"StatusBar.Color.Dark", nStatusBarDark);
	for (int i = csi_Info; i < csi_Last; i++)
	{
		LPCWSTR pszName = gpConEmu->mp_Status->GetSettingName((CEStatusItems)i);
		if (pszName)
			reg->Save(pszName, isStatusColumnHidden[i]);
	}
}

void Settings::SaveStartCommands(SettingsBase* reg)
{
	reg->Save(L"StartType", nStartType);
	reg->Save(L"CmdLine", psStartSingleApp);
	reg->Save(L"StartTasksFile", psStartTasksFile);
	reg->Save(L"StartTasksName", psStartTasksName);
	reg->Save(L"StartFarFolders", isStartFarFolders);
	reg->Save(L"StartFarEditors", isStartFarEditors);
}

BOOL Settings::SaveSettings(BOOL abSilent /*= FALSE*/, const SettingsStorage* apStorage /*= nullptr*/)
{
	if (!gpConEmu)
	{
		_ASSERTE(gpConEmu);
		return FALSE;
	}
	gpConEmu->LogString(L"Settings::SaveSettings");

	BOOL lbRc = FALSE;

	gpSetCls->SettingsPreSave();

	SettingsBase* reg = CreateSettings(apStorage);
	if (!reg)
	{
		_ASSERTE(reg!=nullptr);
		return FALSE;
	}

	// Если в реестре настройка есть, или изменилось значение
	bool lbCurAutoRegisterFonts = isAutoRegisterFonts, lbCurAutoRegisterFontsRc = false;
	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_READ, abSilent))
	{
		lbCurAutoRegisterFontsRc = reg->Load(L"AutoRegisterFonts", lbCurAutoRegisterFonts);
		reg->CloseKey();
	}


	if (reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE, abSilent))
	{
		// При сохранении меняем "сохраненный" тип (отображается в заголовке диалога)
		// только если сохранение было "умолчательное", а не "Экспорт в файл"
		if (apStorage == nullptr)
		{
			gpSetCls->Type = reg->m_Storage.Type;
		}

		reg->Save(L"Language", Language);

		SaveStartCommands(reg);

		/* Saved outside of SaveStartCommands because it's not required in ‘Vanilla’ xml */
		reg->Save(L"StartCreateDelay", nStartCreateDelay);

		SaveAppSettings(reg, &AppStd/*, Colors*/);

		reg->Save(L"TrueColorerSupport", isTrueColorer);
		reg->Save(L"VividColors", isVividColors);
		reg->Save(L"FadeInactive", isFadeInactive);
		reg->Save(L"FadeInactiveLow", mn_FadeLow);
		reg->Save(L"FadeInactiveHigh", mn_FadeHigh);

		reg->Save(L"ConVisible", isConVisible);

		reg->Save(L"UseInjects", isUseInjects);

		reg->Save(L"SetDefaultTerminal", isSetDefaultTerminal);
		reg->Save(L"SetDefaultTerminalStartup", isRegisterOnOsStartup);
		reg->Save(L"SetDefaultTerminalStartupTSA", isRegisterOnOsStartupTSA);
		reg->Save(L"DefaultTerminalAgressive", isRegisterAggressive);
		reg->Save(L"DefaultTerminalNoInjects", isDefaultTerminalNoInjects);
		reg->Save(L"DefaultTerminalNewWindow", isDefaultTerminalNewWindow);
		reg->Save(L"DefaultTerminalDebugLog", isDefaultTerminalDebugLog);
		reg->Save(L"DefaultTerminalConfirm", static_cast<BYTE>(nDefaultTerminalConfirmClose));
		{
		const CEStr pszApps = GetDefaultTerminalApps(); // MSZ -> "|"-delimited string
		reg->Save(L"DefaultTerminalApps", pszApps.c_str());
		}

		reg->Save(L"ProcessAnsi", isProcessAnsi);
		reg->Save(L"AnsiExecution", isAnsiExec);
		this->SaveMSZ(reg, L"AnsiAllowedCommands", psAnsiAllowed ? psAnsiAllowed->c_str() : nullptr, L"\r\n", false);

		reg->Save(L"AnsiLog", isAnsiLog);
		reg->Save(L"AnsiLogCodes", isAnsiLogCodes);
		reg->Save(L"AnsiLogPath", pszAnsiLog);
		reg->Save(L"KillSshAgent", isKillSshAgent);
		reg->Save(L"ProcessNewConArg", isProcessNewConArg);
		reg->Save(L"ProcessCmdStart", isProcessCmdStart);
		reg->Save(L"ProcessCtrlZ", isProcessCtrlZ);

		reg->Save(L"SuppressBells", isSuppressBells);

		reg->Save(L"ConsoleExceptionHandler", isConsoleExceptionHandler);

		reg->Save(L"UseClink", mb_UseClink);

		//reg->Save(L"LockRealConsolePos", isLockRealConsolePos);

		reg->Save(L"StoreTaskbarkTasks", isStoreTaskbarkTasks);
		reg->Save(L"StoreTaskbarCommands", isStoreTaskbarCommands);
		reg->Save(L"JumpListAutoUpdate", isJumpListAutoUpdate);

		reg->Save(L"SaveCmdHistory", isSaveCmdHistory);
		// Пишем всегда, даже если (!isSaveCmdHistory), т.к. история могла быть "преднастроена"
		HistorySave(reg); // L"CmdLineHistory"
		reg->Save(L"SingleInstance", isSingleInstance);
		reg->Save(L"ShowHelpTooltips", isShowHelpTooltips);
		reg->Save(L"Multi", mb_isMulti);
		reg->Save(L"Multi.ShowButtons", isMultiShowButtons);
		reg->Save(L"Multi.ShowSearch", isMultiShowSearch);
		reg->Save(L"Multi.NumberInCaption", isNumberInCaption);
		//reg->Save(L"Multi.NewConsole", vmMultiNew);
		//reg->Save(L"Multi.NewConsoleShift", vmMultiNewShift);
		//reg->Save(L"Multi.Next", vmMultiNext);
		//reg->Save(L"Multi.NextShift", vmMultiNextShift);
		//reg->Save(L"Multi.Recreate", vmMultiRecreate);
		//reg->Save(L"Multi.Close", vmMultiClose);
		reg->Save(L"Multi.CloseConfirmFlags", nCloseConfirmFlags);
		//reg->Save(L"Multi.CmdKey", vmMultiCmd);
		reg->Save(L"Multi.NewConfirm", isMultiNewConfirm);
		reg->Save(L"Multi.DupConfirm", isMultiDupConfirm);
		reg->Save(L"Multi.DetachConfirm", isMultiDetachConfirm);
		//reg->Save(L"Multi.Buffer", vmMultiBuffer);
		reg->Save(L"Multi.UseArrows", isUseWinArrows);
		reg->Save(L"Multi.UseNumbers", isUseWinNumber);
		reg->Save(L"Multi.UseWinTab", isUseWinTab);
		reg->Save(L"Multi.AutoCreate", isMultiAutoCreate);
		reg->Save(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		reg->Save(L"Multi.HideOnClose", isMultiHideOnClose);
		reg->Save(L"Multi.MinByEsc", isMultiMinByEsc);
		reg->Save(L"MapShiftEscToEsc", isMapShiftEscToEsc);
		reg->Save(L"Multi.Iterate", isMultiIterate);
		reg->Save(L"Multi.SplitWidth", nSplitWidth);
		reg->Save(L"Multi.SplitHeight", nSplitHeight);
		//reg->Save(L"Multi.SplitClr1", nSplitClr1);
		//reg->Save(L"Multi.SplitClr2", nSplitClr2);
		reg->Save(L"ResetTerminalConfirm", isResetTerminalConfirm);

		//reg->Save(L"MinimizeRestore", vmMinimizeRestore);
		_ASSERTE(m_isKeyboardHooks!=0);
		reg->Save(L"KeyboardHooks", m_isKeyboardHooks);
		reg->Save(L"FontName", inFont);
		reg->Save(L"FontName2", inFont2);

		// Если в реестре настройка есть, или изменилось значение
		if (lbCurAutoRegisterFontsRc || (isAutoRegisterFonts != lbCurAutoRegisterFonts))
			reg->Save(L"AutoRegisterFonts", isAutoRegisterFonts);

		reg->Save(L"FontAutoSize", isFontAutoSize);
		reg->Save(L"FontSize", FontSizeY);
		reg->Save(L"FontSizeX", FontSizeX);
		reg->Save(L"FontUseDpi", FontUseDpi);
		reg->Save(L"FontUseUnits", FontUseUnits);
		reg->Save(L"FontSizeX2", FontSizeX2);
		reg->Save(L"FontSizeX3", FontSizeX3);
		reg->Save(L"FontCharSet", mn_LoadFontCharSet);
		if (ghOpWnd != nullptr)
			mb_CharSetWasSet = FALSE;
		reg->Save(L"Anti-aliasing", mn_AntiAlias);
		reg->Save(L"FontBold", isBold);
		reg->Save(L"FontItalic", isItalic);

		reg->Save(L"Monospace", isMonospace);

		reg->Save(L"CompressLongStrings", isCompressLongStrings);

		reg->Save(L"BackGround Image show", isShowBgImage);
		reg->Save(L"BackGround Image", sBgImage);
		reg->Save(L"bgImageDarker", bgImageDarker);
		reg->Save(L"bgImageColors", nBgImageColors);
		reg->Save(L"bgOperation", bgOperation);
		reg->Save(L"bgPluginAllowed", isBgPluginAllowed);
		reg->Save(L"AlphaValue", nTransparent);
		reg->Save(L"AlphaValueSeparate", isTransparentSeparate);
		reg->Save(L"AlphaValueInactive", nTransparentInactive);
		//reg->Save(L"UseColorKey", isColorKey);
		//reg->Save(L"ColorKey", ColorKey);
		reg->Save(L"UserScreenTransparent", isUserScreenTransparent);
		reg->Save(L"ColorKeyTransparent", isColorKeyTransparent);
		reg->Save(L"ColorKeyValue", nColorKeyValue);
		SaveSizeSettings(reg);
		ResetSavedOnExit(); // Раз было инициированное пользователем сохранение настроек - сбросим флажок (mb_ExitSettingsAutoSaved)
		reg->Save(L"QuakeStyle", isQuakeStyle);
		reg->Save(L"Restore2ActiveMon", isRestore2ActiveMon);
		reg->Save(L"RestoreInactive", isRestoreInactive);
		reg->Save(L"QuakeAnimation", nQuakeAnimation);
		reg->Save(L"HideCaption", isHideCaption);
		reg->Save(L"HideChildCaption", isHideChildCaption);
		reg->Save(L"FocusInChildWindows", isFocusInChildWindows);
		reg->Save(L"HideCaptionAlways", mb_HideCaptionAlways);
		reg->Save(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame);
		reg->Save(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);
		reg->Save(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);
		reg->Save(L"DownShowHiddenMessage", isDownShowHiddenMessage);
		reg->Save(L"DownShowExOnTopMessage", isDownShowExOnTopMessage);
		reg->Save(L"ConsoleFontName", ConsoleFont.lfFaceName);
		reg->Save(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		reg->Save(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		reg->Save(L"DynamicBufferHeight", isDynamicBufferHeight);
		reg->Save(L"DefaultBufferHeight", DefaultBufferHeight);
		reg->Save(L"AutoBufferHeight", AutoBufferHeight);
		reg->Save(L"UseScrollLock", UseScrollLock);
		reg->Save(L"ComSpec.Type", (BYTE)ComSpec.csType);
		reg->Save(L"ComSpec.Bits", (BYTE)ComSpec.csBits);
		reg->Save(L"ComSpec.UpdateEnv", _bool(ComSpec.isUpdateEnv));
		reg->Save(L"ComSpec.EnvAddPath", _bool((ComSpec.AddConEmu2Path & CEAP_AddConEmuBaseDir) == CEAP_AddConEmuBaseDir));
		reg->Save(L"ComSpec.EnvAddExePath", _bool((ComSpec.AddConEmu2Path & CEAP_AddConEmuExeDir) == CEAP_AddConEmuExeDir));
		reg->Save(L"ComSpec.UncPaths", _bool(ComSpec.isAllowUncPaths));
		reg->Save(L"ComSpec.Path", ComSpec.ComspecExplicit);
		reg->Save(L"AutoReloadEnvironment", AutoReloadEnvironment);
		this->SaveMSZ(reg, L"EnvironmentSet", psEnvironmentSet ? psEnvironmentSet->c_str() : nullptr, L"\r\n", false);
		reg->Save(L"CTS.Intelligent", isCTSIntelligent);
		{
		const CEStr pszApps = GetIntelligentExceptions(); // MSZ -> "|"-delimited string
		reg->Save(L"CTS.IntelligentExceptions", pszApps.c_str());
		}
		reg->Save(L"CTS.AutoCopy", isCTSAutoCopy);
		reg->Save(L"CTS.ResetOnRelease", isCTSResetOnRelease);
		reg->Save(L"CTS.IBeam", isCTSIBeam);
		reg->Save(L"CTS.EndOnTyping", isCTSEndOnTyping);
		reg->Save(L"CTS.EndOnKeyPress", isCTSEndOnKeyPress);
		reg->Save(L"CTS.EraseBeforeReset", isCTSEraseBeforeReset);
		reg->Save(L"CTS.Freeze", isCTSFreezeBeforeSelect);
		reg->Save(L"CTS.SelectBlock", isCTSSelectBlock);
		reg->Save(L"CTS.SelectText", isCTSSelectText);
		reg->Save(L"CTS.HtmlFormat", isCTSHtmlFormat);
		reg->Save(L"CTS.ForceLocale", isCTSForceLocale);
		reg->Save(L"CTS.RBtnAction", static_cast<BYTE>(isCTSRBtnAction));
		reg->Save(L"CTS.MBtnAction", static_cast<BYTE>(isCTSMBtnAction));
		reg->Save(L"CTS.ColorIndex", isCTSColorIndex);

		reg->Save(L"ClipboardConfirmEnter", isPasteConfirmEnter);
		reg->Save(L"AutoTrimSingleLine", isAutoTrimSingleLine);
		reg->Save(L"ClipboardConfirmLonger", nPasteConfirmLonger);

		reg->Save(L"FarGotoEditorOpt", isFarGotoEditor);
		reg->Save(L"FarGotoEditorPath", sFarGotoEditor);

		reg->Save(L"HighlightMouseRow", isHighlightMouseRow);
		reg->Save(L"HighlightMouseCol", isHighlightMouseCol);

		reg->Save(L"FixFarBorders", isFixFarBorders);
		reg->Save(L"Anti-aliasing2", isAntiAlias2);
		{
		CEStr pszCharRanges = CreateCharRanges(mpc_CharAltFontRanges);
		reg->Save(L"FixFarBordersRanges", pszCharRanges.c_str(L"2013-25C4"));
		}
		reg->Save(L"ExtendUCharMap", isExtendUCharMap);
		reg->Save(L"EnhanceGraphics", isEnhanceGraphics);
		reg->Save(L"EnhanceButtons", isEnhanceButtons);
		reg->Save(L"PartBrush75", isPartBrush75);
		reg->Save(L"PartBrush50", isPartBrush50);
		reg->Save(L"PartBrush25", isPartBrush25);
		reg->Save(L"PartBrushBlack", isPartBrushBlack);
		reg->Save(L"RightClick opens context menu", isRClickSendKey);
		reg->Save(L"RightClickMacro2", sRClickMacro);
		//reg->Save(L"AltEnter", isSendAltEnter);
		//reg->Save(L"AltSpace", isSendAltSpace);
		reg->Save(L"SendAltTab", isSendAltTab);
		reg->Save(L"SendAltEsc", isSendAltEsc);
		reg->Save(L"SendAltPrintScrn", isSendAltPrintScrn);
		reg->Save(L"SendPrintScrn", isSendPrintScrn);
		reg->Save(L"SendCtrlEsc", isSendCtrlEsc);
		//reg->Save(L"SendAltF9", isSendAltF9);
		reg->Save(L"Min2Tray", mb_MinToTray);
		reg->Save(L"AlwaysShowTrayIcon", mb_AlwaysShowTrayIcon);
		reg->Save(L"SafeFarClose", isSafeFarClose);
		reg->Save(L"SafeFarCloseMacro", sSafeFarCloseMacro);
		reg->Save(L"FARuseASCIIsort", isFARuseASCIIsort);
		reg->Save(L"ShellNoZoneCheck", isShellNoZoneCheck);
		reg->Save(L"FixAltOnAltTab", isFixAltOnAltTab);
		reg->Save(L"UseAltGrayPlus", isUseAltGrayPlus);
		reg->Save(L"DisableMouse", isDisableMouse);
		reg->Save(L"RSelectionFix", isRSelFix);
		reg->Save(L"MouseSkipActivation", isMouseSkipActivation);
		reg->Save(L"MouseSkipMoving", isMouseSkipMoving);
		reg->Save(L"MouseDragWindow", isMouseDragWindow);
		reg->Save(L"FarWheelScrollback", isFarWheelScrollback);
		reg->Save(L"FarHourglass", isFarHourglass);
		reg->Save(L"FarHourglassDelay", nFarHourglassDelay);
		reg->Save(L"Dnd", isDragEnabled);
		//reg->Save(L"DndLKey", nLDragKey);
		//reg->Save(L"DndRKey", nRDragKey);
		reg->Save(L"DndDrop", isDropEnabled);
		reg->Save(L"DefCopy", isDefCopy);
		reg->Save(L"DropUseMenu", isDropUseMenu);
		reg->Save(L"DragOverlay", isDragOverlay);
		reg->Save(L"DragShowIcons", isDragShowIcons);
		reg->Save(L"DebugSteps", isDebugSteps);
		reg->Save(L"DebugLog", isDebugLog);
		reg->Save(L"DragPanel", isDragPanel);
		reg->Save(L"DragPanelBothEdges", isDragPanelBothEdges);
		reg->Save(L"KeyBarRClick", isKeyBarRClick);
		//reg->Save(L"GUIpb", isGUIpb);
		SaveStatusSettings(reg);
		reg->Save(L"Tabs", isTabs);
		reg->Save(L"TabsLocation", nTabsLocation);
		reg->Save(L"TabIcons", isTabIcons);
		reg->Save(L"OneTabPerGroup", isOneTabPerGroup);
		reg->Save(L"ActivateSplitMouseOver", bActivateSplitMouseOver);
		reg->Save(L"TabSelf", isTabSelf);
		reg->Save(L"TabLazy", isTabLazy);
		reg->Save(L"TabFlashChanged", nTabFlashChanged);
		reg->Save(L"TabRecent", isTabRecent);
		reg->Save(L"TabDblClick", nTabBarDblClickAction);
		reg->Save(L"TabBtnDblClick", nTabBtnDblClickAction);
		reg->Save(L"TabsOnTaskBar", m_isTabsOnTaskBar);
		reg->Save(L"TaskBarOverlay", isTaskbarOverlay);
		reg->Save(L"TaskbarProgress", isTaskbarProgress);
		reg->Save(L"TabCloseMacro", sTabCloseMacro);
		reg->Save(L"TabFontFace", sTabFontFace);
		reg->Save(L"TabFontCharSet", nTabFontCharSet);
		reg->Save(L"TabFontHeight", nTabFontHeight);
		reg->Save(L"SaveAllEditors", sSaveAllMacro);
		//reg->Save(L"TabFrame", isTabFrame);
		//reg->Save(L"TabMargins", rcTabMargins);
		reg->Save(L"ToolbarAddSpace", nToolbarAddSpace);
		reg->Save(L"TabConsole", szTabConsole);
		reg->Save(L"TabModifiedSuffix", szTabModifiedSuffix);
		reg->Save(L"TabSkipWords", pszTabSkipWords);
		reg->Save(L"TabPanels", szTabPanels);
		reg->Save(L"TabEditor", szTabEditor);
		reg->Save(L"TabEditorModified", szTabEditorModified);
		reg->Save(L"TabViewer", szTabViewer);
		reg->Save(L"TabLenMax", nTabLenMax);

		if (bAdminShield == ats_Disabled) // Because of using temporarily in Settings dialog
		{
			bAdminShield = ats_Empty; szAdminTitleSuffix[0] = 0;
		}
		reg->Save(L"AdminTitleSuffix", szAdminTitleSuffix);
		reg->Save(L"AdminShowShield", bAdminShield);

		reg->Save(L"HideInactiveConsoleTabs", bHideInactiveConsoleTabs);
		reg->Save(L"ShowFarWindows", bShowFarWindows);
		reg->Save(L"TryToCenter", isTryToCenter);
		reg->Save(L"CenterConsolePad", nCenterConsolePad);
		reg->Save(L"ShowScrollbar", isAlwaysShowScrollbar);
		reg->Save(L"ScrollBarAppearDelay", nScrollBarAppearDelay);
		reg->Save(L"ScrollBarDisappearDelay", nScrollBarDisappearDelay);
		reg->Save(L"IconID", nIconID);
		//reg->Save(L"Update Console handle", isUpdConHandle);
		/*reg->Save(L"ScrollTitle", isScrollTitle);
		reg->Save(L"ScrollTitleLen", ScrollTitleLen);*/
		reg->Save(L"MainTimerElapse", nMainTimerElapse);
		reg->Save(L"MainTimerInactiveElapse", nMainTimerInactiveElapse);
		reg->Save(L"AffinityMask", nAffinity);
		reg->Save(L"SkipFocusEvents", isSkipFocusEvents);
		reg->Save(L"MonitorConsoleLang", isMonitorConsoleLang);
		reg->Save(L"SnapToDesktopEdges", isSnapToDesktopEdges);
		reg->Save(L"AlwaysOnTop", isAlwaysOnTop);
		reg->Save(L"SleepInBackground", isSleepInBackground);
		reg->Save(L"RetardInactivePanes", isRetardInactivePanes);
		reg->Save(L"MinimizeOnLoseFocus", mb_MinimizeOnLoseFocus);
		reg->Save(L"DisableFarFlashing", isDisableFarFlashing);
		reg->Save(L"DisableAllFlashing", isDisableAllFlashing);
		/* FindText: bMatchCase, bMatchWholeWords, bFreezeConsole, bHighlightAll */
		SaveFindOptions(reg);
		/* *********** Thumbnails and Tiles ************* */
		reg->Save(L"PanView.BackColor", ThSet.crBackground.RawColor);
		reg->Save(L"PanView.PFrame", ThSet.nPreviewFrame);
		reg->Save(L"PanView.PFrameColor", ThSet.crPreviewFrame.RawColor);
		reg->Save(L"PanView.SFrame", ThSet.nSelectFrame);
		reg->Save(L"PanView.SFrameColor", ThSet.crSelectFrame.RawColor);
		/* теперь разнообразные размеры */
		ThumbSaveSet(L"Thumbs", ThSet.Thumbs);
		ThumbSaveSet(L"Tiles", ThSet.Tiles);
		// Прочие параметры загрузки
		reg->Save(L"PanView.LoadPreviews", ThSet.bLoadPreviews);
		reg->Save(L"PanView.LoadFolders", ThSet.bLoadFolders);
		reg->Save(L"PanView.LoadTimeout", ThSet.nLoadTimeout);
		reg->Save(L"PanView.MaxZoom", ThSet.nMaxZoom);
		reg->Save(L"PanView.UsePicView2", ThSet.bUsePicView2);
		reg->Save(L"PanView.RestoreOnStartup", ThSet.bRestoreOnStartup);

		/* *** AutoUpdate *** */
		const wchar_t* pszVerLocation = (UpdSet.szUpdateVerLocation && (0 != lstrcmp(UpdSet.szUpdateVerLocation, UpdSet.UpdateVerLocationDefault())))
			? UpdSet.szUpdateVerLocation : nullptr;
		reg->Save(L"Update.VerLocation", pszVerLocation);
		reg->Save(L"Update.CheckOnStartup", UpdSet.isUpdateCheckOnStartup);
		reg->Save(L"Update.CheckHourly", UpdSet.isUpdateCheckHourly);
		reg->Save(L"Update.ConfirmDownload", UpdSet.isUpdateConfirmDownload);
		reg->Save(L"Update.UseBuilds", (BYTE)UpdSet.isUpdateUseBuilds);
		reg->Save(L"Update.InetTool", UpdSet.isUpdateInetTool);
		reg->Save(L"Update.InetToolCmd", UpdSet.szUpdateInetTool);
		reg->Save(L"Update.UseProxy", UpdSet.isUpdateUseProxy);
		reg->Save(L"Update.Proxy", UpdSet.szUpdateProxy);
		reg->Save(L"Update.ProxyUser", UpdSet.szUpdateProxyUser);
		reg->Save(L"Update.ProxyPassword", UpdSet.szUpdateProxyPassword);
		//reg->Save(L"Update.DownloadSetup", UpdSet.isUpdateDownloadSetup); // 1-Installer (ConEmuSetup.exe), 2-7z archive (ConEmu.7z), WinRar or 7z required
		reg->Save(L"Update.ExeCmdLine", UpdSet.szUpdateExeCmdLine);
		reg->Save(L"Update.ArcCmdLine", UpdSet.szUpdateArcCmdLine);
		reg->Save(L"Update.DownloadPath", UpdSet.szUpdateDownloadPath);
		reg->Save(L"Update.LeavePackages", UpdSet.isUpdateLeavePackages);
		reg->Save(L"Update.PostUpdateCmd", UpdSet.szUpdatePostUpdateCmd);

		/* Done */
		reg->CloseKey();

		/* Subsections */
		SaveHotkeys(reg);
		SaveCmdTasks(reg);
		SaveAppsSettings(reg);
		SavePalettes(reg);
		/* Some automatic options: size, autostart task, etc. */
		AutoSaveSettings(reg, true);

		/* Done */
		lbRc = TRUE;
	}

	//}
	delete reg;

	// Вроде и показывать не нужно. Объект уже сам ругнулся
	//MessageBoxA(ghOpWnd, "Failed", "Information", MB_ICONERROR);
	return lbRc;
}

int Settings::StatusBarFontHeight()
{
	return std::max(4,nStatusFontHeight);
}

int Settings::StatusBarHeight()
{
	int lfHeight = gpSetCls->EvalSize(StatusBarFontHeight(), esf_Vertical|esf_CanUseDpi|esf_CanUseUnits);
	int iHeight = gpFontMgr->EvalFontHeight(sStatusFontFace, lfHeight, nStatusFontCharSet);
	_ASSERTE(iHeight > 0);

	if (isStatusBarFlags & csf_NoVerticalPad)
		iHeight += (isStatusBarFlags & csf_HorzDelim) ? 1 : 0;
	else
		iHeight += (isStatusBarFlags & csf_HorzDelim) ? 2 : 1;

	return iHeight;
}


DWORD Settings::isUseClink()
{
	if (!mb_UseClink)
		return 0;

	if (gpConEmu->IsResetBasicSettings())
		return 0;

	LPCWSTR clink_found_path = nullptr;
	wchar_t szClink32[MAX_PATH+30] = L"", szClink64[MAX_PATH+30] = L"";
	const wchar_t* clink_dll_32[] = {CLINK_DLL_NAME_x32_v1, CLINK_DLL_NAME_x32_v0};
	const wchar_t* clink_dll_64[] = {CLINK_DLL_NAME_x64_v1, CLINK_DLL_NAME_x64_v0};
	const wchar_t clink_dir[] = L"\\clink\\";
	for (size_t i = 0; i < std::size(clink_dll_32) && !clink_found_path; ++i)
	{
		wcscpy_c(szClink32, gpConEmu->ms_ConEmuBaseDir);
		wcscat_c(szClink32, clink_dir);
		wcscat_c(szClink32, clink_dll_32[i]);
		if (!FileExists(szClink32))
			continue;

		if (IsWindows64())
		{
			wcscpy_c(szClink64, gpConEmu->ms_ConEmuBaseDir);
			wcscat_c(szClink64, clink_dir);
			wcscat_c(szClink64, clink_dll_64[i]);
			if (!FileExists(szClink64))
				continue;
		}

		clink_found_path = WIN3264TEST(szClink32, szClink64);
	}

	static int nVersionChecked = 0;

	if (!clink_found_path)
	{
		nVersionChecked = 0;
		return 0;
	}

	static VS_FIXEDFILEINFO vi = {};

	if (nVersionChecked == 0)
	{
		DWORD dwRsrvd = 0;
		DWORD dwSize = GetFileVersionInfoSize(clink_found_path, &dwRsrvd);

		if (dwSize == 0)
		{
			nVersionChecked = 1; // clink ver 0.1.1
		}
		else
		{
			void *pVerData = calloc(dwSize, 1);

			if (pVerData)
			{
				VS_FIXEDFILEINFO *lvs = nullptr;

				if (GetFileVersionInfo(clink_found_path, 0, dwSize, pVerData))
				{
					wchar_t szSlash[3]; lstrcpyW(szSlash, L"\\");

					UINT nLen = sizeof(void**);
					if (VerQueryValue((void*)pVerData, szSlash, (void**)&lvs, &nLen))
					{
						vi = *lvs;
						nVersionChecked = 2;
					}
				}

				free(pVerData);
			}
		}
	}

	if (nVersionChecked != 1 && nVersionChecked != 2)
		return 0;
	return (DWORD)nVersionChecked;
}

bool Settings::isKeyboardHooks(bool abNoDisable /*= false*/, bool abNoDbgCheck /*= false*/)
{
	if (!abNoDisable)
	{
		#ifdef _DEBUG
		static int iAsked = 0;
		if (!gpConEmu->DisableKeybHooks)
		{
			if (abNoDbgCheck)
			{
				// Nothing to do
			}
			else if (IsDebuggerPresent())
			{
				// May be it is remote debugger? Check if "devenv.exe" or "WDExpress.exe" processes exists
				if (!iAsked)
				{
					bool bFound = false;
					HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
					if (h && (h != INVALID_HANDLE_VALUE))
					{
						PROCESSENTRY32 PI = {sizeof(PI)};
						if (Process32First(h, &PI))
						{
							do {
								LPCWSTR pszName = PointToName(PI.szExeFile);
								if (lstrcmpi(pszName, L"devenv.exe") == 0 || lstrcmpi(pszName, L"DWExpress.exe") == 0)
								{
									bFound = true;
									break;
								}
							} while (Process32Next(h, &PI));
						}
						CloseHandle(h);
						if (!bFound)
							iAsked = IDNO; // No VisualStudio, No lags?
					}
				}

				if (!iAsked)
				{
					// Need to be set before MsgBox to avoid multiple boxes
					iAsked = IDYES;
					// Ask now
					int iBtn = MsgBox(L"Debugger was detected!\nDo you want to disable hooks to avoid lags?", MB_YESNO|MB_ICONEXCLAMATION, gpConEmu->GetDefaultTitle());
					if (iBtn != iAsked)
						iAsked = iBtn;
				}
				if (iAsked == IDYES)
					return false;
			}
			else
			{
				iAsked = 0;
			}
		}
		#endif

		// Hooks was disabled? Switch "/nokeyhooks" for example
		if (gpConEmu->DisableKeybHooks)
			return false;
	}

	return (m_isKeyboardHooks == 0) || (m_isKeyboardHooks == 1);
}

void Settings::HistoryReset()
{
	if (pHistory)
		pHistory->FreeItems();

	// И сразу сохранить в настройках
	HistorySave(nullptr); // L"CmdLineHistory"
}

void Settings::HistoryLoad(SettingsBase* reg)
{
	bool bSelf = (reg == nullptr);
	if (bSelf)
	{
		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return;
		}

		if (!reg->OpenKey(gpSetCls->GetConfigPath(), KEY_READ))
		{
			SafeDelete(reg);
			return;
		}
	}

	HEAPVAL;
	wchar_t* psCmdHistory = nullptr; // MSZZ
	reg->Load(L"CmdLineHistory", &psCmdHistory);
	if (!pHistory)
		pHistory = new CommandHistory(MAX_CMD_HISTORY);
	pHistory->ParseMSZ(psCmdHistory);
	SafeFree(psCmdHistory);
	HEAPVAL;

	if (bSelf)
	{
		reg->CloseKey();
		delete reg;
	}
}

void Settings::HistorySave(SettingsBase* reg)
{
	bool bSelf = (reg == nullptr);
	if (bSelf)
	{
		reg = CreateSettings(nullptr);
		if (!reg)
		{
			_ASSERTE(reg!=nullptr);
			return;
		}

		if (!reg->OpenKey(gpSetCls->GetConfigPath(), KEY_WRITE))
		{
			SafeDelete(reg);
			return;
		}
	}

	HEAPVAL;
	wchar_t* psCmdHistory = nullptr;
	DWORD nCmdHistorySize = pHistory ? pHistory->CreateMSZ(psCmdHistory) : 0;
	reg->SaveMSZ(L"CmdLineHistory", psCmdHistory, nCmdHistorySize);
	SafeFree(psCmdHistory);
	HEAPVAL;

	if (bSelf)
	{
		reg->CloseKey();
		delete reg;
	}
}

void Settings::HistoryAdd(LPCWSTR asCmd)
{
	if (!isSaveCmdHistory)
		return;

	// Issue 1564: Add tasks to history too
	if (!asCmd || !*asCmd)
		return;

	if (psStartSingleApp && lstrcmp(psStartSingleApp, asCmd)==0)
		return;
	if (psStartTasksFile && lstrcmp(psStartTasksFile, asCmd)==0)
		return;
	// Issue 1564: Add tasks to history too

	LPCWSTR psCurCmd = gpConEmu->GetCurCmd();
	if (psCurCmd && lstrcmp(psCurCmd, asCmd)==0)
		return;
	if (pHistory && pHistory->Compare(0, asCmd)==0)
		return;

	if (!pHistory)
		pHistory = new CommandHistory(MAX_CMD_HISTORY);
	pHistory->Add(asCmd);
	HEAPVAL;

	if (!gpConEmu->IsResetBasicSettings())
	{
		// И сразу сохранить в настройках
		HistorySave(nullptr); // L"CmdLineHistory"
	}
}

LPCWSTR Settings::HistoryGet(int index)
{
	if (pHistory)
		return pHistory->Get(index);

	return nullptr;
}

// например, L"2013-25C3,25C4"
// Возвращает 0 - в случае успеха,
// при ошибке - индекс (1-based) ошибочного символа в asRanges
// -1 - ошибка выделения памяти
int Settings::ParseCharRanges(LPCWSTR asRanges, BYTE (&Chars)[0x10000], BYTE abValue /*= TRUE*/)
{
	if (!asRanges)
	{
		_ASSERTE(asRanges!=nullptr);
		return -1;
	}

	int iRc = 0;
	int n = 0, nMax = _tcslen(asRanges);
	CEStr pszCopy(asRanges);
	if (!pszCopy)
	{
		_ASSERTE(pszCopy!=nullptr);
		return -1;
	}
	wchar_t *pszRange = pszCopy.data();
	wchar_t *pszNext = nullptr;
	UINT cBegin = -1, cEnd;

	memset(Chars, 0, sizeof(Chars));

	while(*pszRange && n < nMax)
	{
		cBegin = (UINT)wcstol(pszRange, &pszNext, 16);
		if (!cBegin || (cBegin > 0xFFFF))
		{
			iRc = (int)(pszRange - asRanges);
			goto wrap;
		}

		switch (*pszNext)
		{
		case L';':
		case 0:
			cEnd = cBegin;
			break;
		case L'-':
		case L' ':
			pszRange = pszNext + 1;
			cEnd = (UINT)wcstol(pszRange, &pszNext, 16);
			if ((cEnd < cBegin) || (cEnd > 0xFFFF))
			{
				iRc = (int)(pszRange - asRanges);
				goto wrap;
			}
			break;
		default:
			iRc = (int)(pszNext - asRanges);
			goto wrap;
		}

		for (UINT i = cBegin; i <= cEnd; i++)
			Chars[i] = abValue;

		if (*pszNext != L';') break;
		pszRange = pszNext + 1;
	}

	iRc = 0; // ok
wrap:
	return iRc;
}

CEStr Settings::CreateCharRanges(BYTE (&Chars)[0x10000])
{
	const size_t nMax = 1024;
	CEStr result;
	wchar_t* pszRanges = result.GetBuffer(nMax);
	if (!pszRanges)
	{
		_ASSERTE(pszRanges!=nullptr);
		return nullptr;
	}

	wchar_t* psz = pszRanges;
	wchar_t* pszEnd = pszRanges + nMax;
	UINT c = 0;
	_ASSERTE((countof(Chars) - 1) == 0xFFFF);
	while (c < countof(Chars))
	{
		if (Chars[c])
		{
			if ((psz + 10) >= pszEnd)
			{
				// Too long block
				_ASSERTE((psz + 10) < pszEnd);
				break;
			}

			const UINT cBegin = (c++);
			UINT cEnd = cBegin;

			while (c < countof(Chars) && Chars[c])
			{
				cEnd = (c++);
			}

			if (cBegin == cEnd)
			{
				wsprintf(psz, L"%04X;", cBegin);
			}
			else
			{
				wsprintf(psz, L"%04X-%04X;", cBegin, cEnd);
			}
			psz += _tcslen(psz);
		}
		else
		{
			c++;
		}
	}

	return result;
}

bool Settings::CheckCharAltFont(ucs32 inChar)
{
	//TODO: Support character codebases >=0xFFFF
	WARNING("Change mpc_CharAltFontRanges to interval map");
	// !!! leave C4800 warning in place, don't forget about refactoring
	return (isFixFarBorders && !(inChar & ~0xFFFF)) ? mpc_CharAltFontRanges[LOWORD(inChar)] : false;
}

void Settings::CheckConsoleSettings()
{
	// Если в ключе [HKEY_CURRENT_USER\Console] будут левые значения - то в Win7 могут
	// начаться страшные глюки :-)
	// например, консольное окно будет "дырявое" - рамка есть, а содержимого - нет :-P
	DWORD nFullScr = 0, nFullScrEmu = 0, nSize;
	HKEY hkCon, hkEmu;
	if (!RegOpenKeyEx(HKEY_CURRENT_USER, L"Console", 0, KEY_ALL_ACCESS, &hkCon))
	{
		if (RegQueryValueEx(hkCon, L"FullScreen", nullptr, nullptr, (LPBYTE)&nFullScr, &(nSize=sizeof(nFullScr))))
			nFullScr = 0;
		if (!RegOpenKeyEx(hkCon, CEC_INITTITLE, 0, KEY_ALL_ACCESS, &hkEmu))
		{
			if (RegQueryValueEx(hkEmu, L"FullScreen", nullptr, nullptr, (LPBYTE)&nFullScrEmu, &(nSize=sizeof(nFullScrEmu))))
				nFullScrEmu = 0;
			RegCloseKey(hkEmu);
		}
		else
		{
			nFullScrEmu = nFullScr;
		}
		RegCloseKey(hkCon);
	}

	if (nFullScr || nFullScrEmu)
	{
		if (gOSVer.dwMajorVersion >= 6)
		{
			wchar_t szWarning[512];
			swprintf_c(szWarning,
				L"Warning!\n"
				L"Dangerous values detected in yours registry\n\n"
				L"Please check [HKEY_CURRENT_USER\\Console] and [HKEY_CURRENT_USER\\Console\\ConEmu] keys\n\n"
				L"\"FullScreen\" recommended value is dword:00000000\n\n"
				L"Current value is dword:%08X",
				nFullScr ? nFullScr : nFullScrEmu);
			MBoxA(szWarning);
		}
	}
}

CEStr Settings::GetStoragePlaceDescr(const SettingsStorage* apStorage, LPCWSTR asPrefix)
{
	SettingsStorage temp = {};
	if (apStorage == nullptr)
	{
		temp = GetSettingsType();
		apStorage = &temp;
	}

	return CEStr(
		asPrefix,
		// [reg], [xml], ...
		L" ", apStorage->getTypeName(),
		// xml file
		apStorage->hasFile() ? L" " : nullptr,
		apStorage->hasFile() ? apStorage->File : nullptr,
		// -config ...
		apStorage->hasConfig() ? L" -config " : nullptr,
		apStorage->hasConfig() ? apStorage->Config : nullptr
		);
}

SettingsBase* Settings::CreateSettings(const SettingsStorage* apStorage)
{
	SettingsBase* pReg = nullptr;

	SettingsStorage Storage = apStorage ? *apStorage : GetSettingsType();

	// When settings location were passed via parameter,
	// we shall check them for validity and file-availability
	if (apStorage)
	{
		if (Storage.isFileType())
		{
			if (!Storage.hasFile())
			{
				CEStr lsMessage(L"Invalid ", Storage.getTypeName(), L"-path was specified!");
				DisplayLastError(lsMessage, -1);
				return nullptr;
			}
			HANDLE hFile = CreateFile(Storage.File,
				GENERIC_READ|(Storage.ReadOnly ? 0 : GENERIC_WRITE),
				FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE || !hFile)
			{
				CEStr lsMessage(L"Failed to create ", Storage.getTypeName(), L"-file!\n", Storage.File);
				DisplayLastError(lsMessage);
				return nullptr;
			}
			CloseHandle(hFile);
		}
	}

	switch (Storage.Type)
	{
	case StorageType::XML:
		pReg = new SettingsXML(Storage);
		break;
	case StorageType::INI:
		pReg = new SettingsINI(Storage);
		break;
	case StorageType::REG:
		pReg = new SettingsRegistry();
		break;
	case StorageType::BASIC:
		_ASSERTE(FALSE && "-basic is not expected to be loaded from settings");
		break;
	default:
		{
			CEStr lsMessage(L"Settings type ", Storage.getTypeName(), L" is not supported yet");
			DisplayLastError(lsMessage, -1);
		}
	}

	return pReg;
}

SettingsStorage Settings::GetSettingsType()
{
	SettingsStorage Storage = {StorageType::REG};
	Storage.Config = gpSetCls->GetConfigName();

	// #SETTINGS Support other storage types? INI, JSON, ...
	LPCWSTR pszXmlFile = gpConEmu->ConEmuXml();
	if (pszXmlFile && *pszXmlFile)
	{
		// XML-file exists
		if (CEHandle hFile = CreateFile(pszXmlFile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE,
		                   nullptr, OPEN_EXISTING, 0, 0))
		{
			// well, at least read-only access is available
			hFile.Close();

			Storage.Type = StorageType::XML;
			Storage.File = pszXmlFile;

			// Check if it's write-enabled
			hFile = CreateFile(pszXmlFile, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE,
				                nullptr, OPEN_EXISTING, 0, 0);

			if (!hFile)
			{
				DWORD nErr = GetLastError();
				//EnableWindow(GetDlgItem(ghOpWnd, bSaveSettings), FALSE); // Сохранение запрещено
				Storage.ReadOnly = true;
				UNREFERENCED_PARAMETER(nErr);
			}
		}
	}

	return Storage;
}

void Settings::SetHideCaptionAlways(bool bHideCaptionAlways)
{
	mb_HideCaptionAlways = bHideCaptionAlways;
}

void Settings::SwitchHideCaptionAlways()
{
	mb_HideCaptionAlways = !mb_HideCaptionAlways;
}

bool Settings::isHideCaptionAlways()
{
	return mb_HideCaptionAlways || (!mb_HideCaptionAlways && isUserScreenTransparent) || isQuakeStyle || gpConEmu->mp_Inside;
}

bool Settings::isForcedHideCaptionAlways()
{
	return (isUserScreenTransparent || isQuakeStyle);
}

bool Settings::isAlwaysShowTrayIcon()
{
	// Issue 1431
	return (mb_AlwaysShowTrayIcon || gpConEmu->opt.DesktopMode || (isQuakeStyle && !isTabsOnTaskBar()));
}

bool Settings::isMinimizeOnLoseFocus()
{
	return (isQuakeStyle == 2) || (isQuakeStyle == 0 && mb_MinimizeOnLoseFocus);
}

bool Settings::isMinToTray(bool bRawOnly /*= false*/)
{
	// Сворачивать в TSA только если включен флажок. Юзер его мог принудительно отключить.
	return (mb_MinToTray || /*(m_isTabsOnTaskBar == 3) ||*/ gpConEmu->ForceMinimizeToTray);
}

void Settings::SetMinToTray(bool bMinToTray)
{
	mb_MinToTray = bMinToTray;

	if (ghOpWnd && gpSetCls->GetPage(thi_Taskbar))
	{
		gpSetCls->checkDlgButton(gpSetCls->GetPage(thi_Taskbar), cbMinToTray, mb_MinToTray);
	}
}

int Settings::HideCaptionAlwaysFrame()
{
	if (gpConEmu->isInside())
		return 0;
	int iFrame = gpConEmu->opt.FrameWidth.Exists ? gpConEmu->opt.FrameWidth.Int : gpSet->nHideCaptionAlwaysFrame;
	return (iFrame > HIDECAPTIONALWAYSFRAME_MAX) ? -1 : iFrame;
}

int Settings::GetAppSettingsId(LPCWSTR asExeAppName, bool abElevated)
{
	if (!asExeAppName || !*asExeAppName || !Apps || (AppCount < 1))
		return -1;

	wchar_t szLower[MAX_PATH]; lstrcpyn(szLower, asExeAppName, countof(szLower));
	CharLowerBuff(szLower, lstrlen(szLower));

	for (int i = 0; i < AppCount; i++)
	{
		if (Apps[i]->Elevated == 1)
		{
			if (!abElevated)
				continue;
		}
		else if (Apps[i]->Elevated == 2)
		{
			if (abElevated)
				continue;
		}

		wchar_t* pch = Apps[i]->AppNamesLwr;
		if (!pch || !*pch)
		{
			_ASSERTE((pch && *pch) || (ghOpWnd && IsWindowVisible(gpSetCls->GetPage(thi_Apps))));
			continue;
		}

		if (lstrcmp(pch, L"*") == 0)
		{
			return i;
		}

		while (*pch)
		{
			wchar_t* pName = szLower;
			while (*pName && *pch && (*pName == *pch))
			{
				pName++; pch++;
			}

			if (!*pName)
			{
				if (!*pch || (*pch == L'|'))
					return i;
			}

			pch = wcschr(pch, L'|');
			if (pch)
			{
				pch++;
				continue; // перечислено несколько допустимых имен
			}
			else
			{
				break; // не совпало
			}
		}
	}

	return -1;
}

const AppSettings* Settings::GetAppSettings(int anAppId/*=-1*/)
{
	if ((anAppId < 0) || (anAppId >= AppCount))
	{
		_ASSERTE(!((anAppId < -1) || (anAppId > AppCount))); // - иначе здесь должен быть валидный индекс для Apps/AppColors
		if (AppStd.AppNames != nullptr)
		{
			_ASSERTE(AppStd.AppNames == nullptr);
			AppStd.AppNames = nullptr;
		}
		if (AppStd.AppNamesLwr != nullptr)
		{
			_ASSERTE(AppStd.AppNamesLwr == nullptr);
			AppStd.AppNamesLwr = nullptr;
		}
		return &AppStd;
	}

	return Apps[anAppId];
}

AppSettings* Settings::GetAppSettingsPtr(int anAppId, BOOL abCreateNew /*= FALSE*/)
{
	if ((anAppId == AppCount) && abCreateNew)
	{
		_ASSERTE(isMainThread());
		int NewAppCount = AppCount+1;
		AppSettings** NewApps = (AppSettings**)calloc(NewAppCount, sizeof(*NewApps));
		//CEAppColors** NewAppColors = (CEAppColors**)calloc(NewAppCount, sizeof(*NewAppColors));
		if (!NewApps /*|| !NewAppColors*/)
		{
			//_ASSERTE(NewApps && NewAppColors);
			_ASSERTE(NewApps);
			return nullptr;
		}
		if (Apps && (AppCount > 0))
		{
			memmove(NewApps, Apps, AppCount*sizeof(*NewApps));
			//memmove(NewAppColors, AppColors, AppCount*sizeof(*NewAppColors));
		}
		AppSettings** pOld = Apps;
		//CEAppColors** pOldColors = AppColors;
		Apps = NewApps;
		//AppColors = NewAppColors;
		SafeFree(pOld);
		//SafeFree(pOldColors);

		Apps[anAppId] = (AppSettings*)calloc(1,sizeof(AppSettings));
		//AppColors[anAppId] = (CEAppColors*)calloc(1,sizeof(CEAppColors));

		if (!Apps[anAppId] /*|| !AppColors[anAppId]*/)
		{
			_ASSERTE(Apps[anAppId]!=nullptr /*&& AppColors[anAppId]!=nullptr*/);
			return nullptr;
		}
		Apps[anAppId]->cchNameMax = MAX_PATH;
		Apps[anAppId]->AppNames = (wchar_t*)calloc(Apps[anAppId]->cchNameMax,sizeof(wchar_t));
		Apps[anAppId]->AppNamesLwr = (wchar_t*)calloc(Apps[anAppId]->cchNameMax,sizeof(wchar_t));

		AppCount = NewAppCount;
	}

	if ((anAppId < 0) || (anAppId >= AppCount))
	{
		_ASSERTE(!((anAppId < 0) || (anAppId > AppCount)));
		return nullptr;
	}

	return Apps[anAppId];
}

void Settings::AppSettingsDelete(int anAppId)
{
	_ASSERTE(isMainThread())
	if (!Apps /*|| !AppColors*/ || (anAppId < 0) || (anAppId >= AppCount))
	{
		_ASSERTE(Apps /*&& AppColors*/ && (anAppId >= 0) && (anAppId < AppCount));
		return;
	}

	AppSettings* pOld = Apps[anAppId];
	//CEAppColors* pOldClr = AppColors[anAppId];

	for (int i = anAppId+1; i < AppCount; i++)
	{
		Apps[i-1] = Apps[i];
		//AppColors[i-1] = AppColors[i];
	}

	_ASSERTE(AppCount>0);
	AppCount--;

	if (pOld)
	{
		pOld->FreeApps();
		free(pOld);
	}
	//SafeFree(pOldClr);
}

// 0-based, index of Apps
bool Settings::AppSettingsXch(int anIndex1, int anIndex2)
{
	if (((anIndex1 < 0) || (anIndex1 >= AppCount))
		|| ((anIndex2 < 0) || (anIndex2 >= AppCount))
		|| !CmdTasks)
	{
		return false;
	}

	AppSettings* p = Apps[anIndex1];
	Apps[anIndex1] = Apps[anIndex2];
	Apps[anIndex2] = p;

	//CEAppColors* pClr = AppColors[anIndex1];
	//AppColors[anIndex1] = AppColors[anIndex2];
	//AppColors[anIndex2] = pClr;

	return true;
}

void Settings::ResetFadeColors()
{
	mn_LastFadeSrc = mn_LastFadeDst = -1;
	mb_FadeInitialized = false;

	for (int i = 0; i < PaletteCount; i++)
	{
		if (Palettes[i])
		{
			Palettes[i]->FadeInitialized = false;
		}
	}
}

const PaletteColors& Settings::GetPaletteColors(LPCWSTR asPalette, BOOL abFade /*= FALSE*/)
{
	_ASSERTE(asPalette && *asPalette);
	const int iPalIdx = PaletteGetIndex(asPalette);
	if (iPalIdx >= 0)
	{
		ColorPalette* palPtr = PaletteGetPtr(iPalIdx);
		if (!palPtr)
		{
			_ASSERTE(palPtr);
		}
		else
		{
			return GetColorsPrepare(palPtr->Colors, palPtr->ColorsFade, &palPtr->FadeInitialized, abFade);
		}
	}

	return GetColorsPrepare(Colors, ColorsFade, &mb_FadeInitialized, abFade);
}

const PaletteColors& Settings::GetColors(int anAppId/*=-1*/, BOOL abFade)
{
	LPCWSTR pszPalette = nullptr;

	if ((anAppId >= 0) && (anAppId < AppCount) && Apps[anAppId]->OverridePalette && Apps[anAppId]->szPaletteName[0])
	{
		pszPalette = Apps[anAppId]->szPaletteName;

		return GetPaletteColors(pszPalette, abFade);
	}

	return GetColorsPrepare(Colors, ColorsFade, &mb_FadeInitialized, abFade);
}

bool Settings::GetDefColors(LPCWSTR asDefName, PaletteColors& colors)
{
	bool result = false;
	if (asDefName && *asDefName)
	{
		for (const auto& defColor : DefColors)
		{
			if (lstrcmpi(defColor.pszTitle, asDefName) == 0)
			{
				colors = defColor.dwDefColors;
				result = true;
			}
		}
	}
	else
	{
		// Windows standard
		colors = DefColors[0].dwDefColors;
		result = true;
	}

	return result;
}

void Settings::GetWindowsColors(PaletteColors& colors)
{
	SettingsRegistry RegConColors, RegConDef;

	const auto& pcrStd = DefColors[0].dwDefColors;
	if (RegConColors.OpenKey(L"Console", KEY_READ))
	{
		RegConDef.OpenKey(HKEY_USERS, L".DEFAULT\\Console", KEY_READ);
		TCHAR ColorName[] = L"ColorTable00";
		const size_t digitPos = _tcslen(ColorName) - 2;
		bool  lbBlackFound = false;

		for (unsigned i = 0x10; i--;)
		{
			// L"ColorTableNN"
			ColorName[digitPos] = TCHAR(i / 10 + '0');
			ColorName[digitPos + 1] = TCHAR(i % 10 + '0');

			if (!RegConColors.Load(ColorName, LPBYTE(&(colors[i])), sizeof(colors[i])))
				if (!RegConDef.Load(ColorName, LPBYTE(&(colors[i])), sizeof(colors[i])))
					colors[i] = pcrStd[i]; //-V108

			if (colors[i] == 0)
			{
				if (!lbBlackFound)
					lbBlackFound = true;
				else if (lbBlackFound)
					colors[i] = pcrStd[i]; //-V108
			}
		}

		RegConDef.CloseKey();
		RegConColors.CloseKey();
	}
}

const PaletteColors& Settings::GetColorsPrepare(const PaletteColors& colors, PaletteColors& colorsFade, bool* pbFadeInitialized, BOOL abFade)
{
	if (!abFade || !isFadeInactive)
		return colors;

	if (!*pbFadeInitialized)
	{
		PrepareFadeColors(colors, colorsFade, pbFadeInitialized);
	}

	return colorsFade;
}

void Settings::PrepareFadeColors(const PaletteColors& colors, PaletteColors& colorsFade, bool* pbFadeInitialized)
{
	// GetFadeColor cache the result
	mn_LastFadeSrc = mn_LastFadeDst = -1;

	// Валидация
	if (((int)mn_FadeHigh - (int)mn_FadeLow) < 64)
	{
		mn_FadeLow = DEFAULT_FADE_LOW; mn_FadeHigh = DEFAULT_FADE_HIGH;
	}

	// Prepare
	mn_FadeMul = mn_FadeHigh - mn_FadeLow;
	*pbFadeInitialized = true;

	// Evaluate fade color
	for (size_t i = 0; i < ColorsFade.size(); ++i)
	{
		colorsFade[i] = GetFadeColor(colors[i]);
	}
}

COLORREF Settings::GetFadeColor(COLORREF cr)
{
	if (!isFadeInactive)
		return cr;

	if (cr == mn_LastFadeSrc)
		return mn_LastFadeDst;

	MYCOLORREF mcr, mcrFade = {}; mcr.color = cr;

	//-нафиг
	//if (!mb_FadeInitialized)
	//{
	//	GetColors(TRUE);
	//}

	mcrFade.rgbRed = GetFadeColorItem(mcr.rgbRed);
	mcrFade.rgbGreen = GetFadeColorItem(mcr.rgbGreen);
	mcrFade.rgbBlue = GetFadeColorItem(mcr.rgbBlue);

	mn_LastFadeSrc = cr;
	mn_LastFadeDst = mcrFade.color;

	return mcrFade.color;
}

BYTE Settings::GetFadeColorItem(BYTE c)
{
	DWORD nRc;

	switch(c)
	{
		case 0:
			return mn_FadeLow;
		case 255:
			return mn_FadeHigh;
		default:
			nRc = ((((DWORD)c) + mn_FadeLow) * mn_FadeHigh) >> 8;

			if (nRc >= 255)
			{
				//_ASSERTE(nRc <= 255); -- такие (mn_FadeLow&mn_FadeHigh) пользователь в настройке мог задать
				return 255;
			}

			return (BYTE)nRc;
	}
}

bool Settings::NeedDialogDetect()
{
	// 100331 теперь оно нужно и для PanelView
	return true;
	//return (isUserScreenTransparent || !isMonospace);
}

// Suppose that only ONE key can be set as modifier!
bool Settings::IsModifierPressed(int nDescrID, bool bAllowEmpty)
{
	bool bIsPressed = false;
	IsModifierPressed(nDescrID,
		bAllowEmpty ? nullptr : &bIsPressed,
		bAllowEmpty ? &bIsPressed : nullptr);
	return bIsPressed;
}

void Settings::IsModifierPressed(int nDescrID, bool* pbNoEmpty, bool* pbAllowEmpty)
{
	if (pbNoEmpty) *pbNoEmpty = false;
	if (pbAllowEmpty) *pbAllowEmpty = false;

	const DWORD vk = ConEmuChord::GetHotkey(GetHotkeyById(nDescrID));

	// If it's not zero - should be pressed
	if (vk)
	{
		if (!isPressed(LOBYTE(vk)))
			return;
	}

	// но другие модификаторы нажаты быть не должны!
	// В том числе если (vk == 0)

	if (vk != VK_SHIFT && vk != VK_LSHIFT && vk != VK_RSHIFT)
	{
		if (isPressed(VK_SHIFT))
			return;
	}

	if (vk != VK_MENU && vk != VK_LMENU && vk != VK_RMENU)
	{
		if (isPressed(VK_MENU))
			return;
	}

	if (vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL)
	{
		if (isPressed(VK_CONTROL))
			return;
	}

	// В том числе, отрубить Apps&Win
	if (!vk)
	{
		if (isPressed(VK_APPS) || isPressed(VK_LWIN) || isPressed(VK_RWIN))
			return;
	}

	// Можно
	if (pbAllowEmpty) *pbAllowEmpty = true;
	if (pbNoEmpty && vk) *pbNoEmpty = true;
}

bool Settings::NeedCreateAppWindow()
{
	// Если просили не показывать на таскбаре - создать скрытое родительское окно
	// m_isTabsOnTaskBar: 0 - ConEmu only, 1 - all tabs & all OS, 2 - all tabs & Win 7, 3 - DON'T SHOW
	if (m_isTabsOnTaskBar == 3)
		return true;

	// Пока что, окно для Application нужно создавать только для XP и ниже
	// в том случае, если на таскбаре отображаются кнопки запущенных консолей
	// Это для того, чтобы при Alt-Tab не светилась "лишняя" иконка главного окна
	if (!IsWindows7 && isTabsOnTaskBar())
		return true;
	return false;
}

unsigned Settings::isLogging(unsigned level /*= 1*/)
{
	if (!gpConEmu || mb_DisableLogging)
		return 0;
	BYTE logLevel = std::max(isDebugLog, (BYTE)(gpConEmu->opt.AdvLogging.GetInt()));
	return (logLevel && (level <= logLevel)) ? logLevel : 0;
}

void Settings::EnableLogging()
{
	mb_DisableLogging = false;
	if (isLogging())
		gpConEmu->CreateLog();
}

void Settings::DisableLogging()
{
	mb_DisableLogging = true;
}

bool Settings::isCloseOnLastTabClose()
{
	return isCloseOnLastTabClose(isMultiLeaveOnClose);
}

bool Settings::isCloseOnLastTabClose(BYTE MultiLeaveOnClose)
{
	// The option "Multi.LeaveOnClose"
	if (MultiLeaveOnClose != 0)
		return false;
	// Switch `-MinTSA`
	if (gpConEmu->WindowStartNoClose)
		return false;
	// Switch `-NoAutoClose`
	if (gpConEmu->opt.NoAutoClose)
		return false;
	return true;
}

bool Settings::isCloseOnCrossClick()
{
	if (isCloseOnLastTabClose())
		return true;
	return (isMultiLeaveOnClose != 1);
}

bool Settings::isMinOnLastTabClose()
{
	if (isCloseOnLastTabClose())
		return false;
	if (gpConEmu->WindowStartNoClose)
		return true;
	return (isMultiHideOnClose != 0);
}

bool Settings::isHideOnLastTabClose()
{
	if (isCloseOnLastTabClose())
		return false;
	if (gpConEmu->WindowStartNoClose)
		return true;
	return (isMultiHideOnClose == 1);
}

// Показывать табы на таскбаре? (для каждой консоли - своя кнопка)
bool Settings::isTabsOnTaskBar()
{
	if (gpConEmu->opt.DesktopMode || (m_isTabsOnTaskBar == 3) || gpConEmu->mp_Inside)
		return false;
	if ((m_isTabsOnTaskBar == 1) || ((m_isTabsOnTaskBar == 2) && IsWindows7))
		return true;
	return false;
}

// Показывать ConEmu (ghWnd) на таскбаре
bool Settings::isWindowOnTaskBar(bool bStrictOnly /*= false*/)
{
	// m_isTabsOnTaskBar: 0 - ConEmu only, 1 - all tabs & all OS, 2 - all tabs & Win 7, 3 - DON'T SHOW

	if (gpConEmu->opt.DesktopMode || (m_isTabsOnTaskBar == 3) || gpConEmu->mp_Inside)
		return false;

	if (bStrictOnly)
		return true;

	if (IsWindows7)
	{
		// Реализовано через API таскбара
		if ((m_isTabsOnTaskBar == 1) || (m_isTabsOnTaskBar == 2))
			return true;
	}
	else
	{
		// в старых OS - нужно создавать фейковые окна, а ghWnd на таскбаре не показывать
		if (m_isTabsOnTaskBar == 1)
			return false;
	}
	return true;
}

bool Settings::isRClickTouchInvert()
{
	return gpConEmu->IsGesturesEnabled() && (isRClickSendKey == 2);
}

#define IsLuaMacroOk(fmv,psz,def) ((fmv != fmv_Lua) || (lstrcmpi(psz,def) != 0))

LPCWSTR Settings::RClickMacro(FarMacroVersion fmv)
{
	if (sRClickMacro && *sRClickMacro && IsLuaMacroOk(fmv, sRClickMacro, FarRClickMacroDefault2))
		return sRClickMacro;
	return RClickMacroDefault(fmv);
}

LPCWSTR Settings::RClickMacroDefault(FarMacroVersion fmv)
{
	// L"@$If (!CmdLine.Empty) %Flg_Cmd=1; %CmdCurPos=CmdLine.ItemCount-CmdLine.CurPos+1; %CmdVal=CmdLine.Value; Esc $Else %Flg_Cmd=0; $End $Text \"rclk_gui:\" Enter $If (%Flg_Cmd==1) $Text %CmdVal %Flg_Cmd=0; %Num=%CmdCurPos; $While (%Num!=0) %Num=%Num-1; CtrlS $End $End"
	static LPCWSTR pszDefaultMacro = FarRClickMacroDefault2;
	static LPCWSTR pszDefaultMacroL = FarRClickMacroDefault3;
	return (fmv == fmv_Lua) ? pszDefaultMacroL : pszDefaultMacro;
}

LPCWSTR Settings::SafeFarCloseMacro(FarMacroVersion fmv)
{
	if (sSafeFarCloseMacro && *sSafeFarCloseMacro && IsLuaMacroOk(fmv, sSafeFarCloseMacro, FarSafeCloseMacroDefault2))
		return sSafeFarCloseMacro;
	return SafeFarCloseMacroDefault(fmv);
}

LPCWSTR Settings::SafeFarCloseMacroDefault(FarMacroVersion fmv)
{
	// L"@$while (Dialog||Editor||Viewer||Menu||Disks||MainMenu||UserMenu||Other||Help) $if (Editor) ShiftF10 $else Esc $end $end  Esc  $if (Shell) F10 $if (Dialog) Enter $end $Exit $end  F10"
	static LPCWSTR pszDefaultMacro = FarSafeCloseMacroDefault2;
	static LPCWSTR pszDefaultMacroL = FarSafeCloseMacroDefault3;
	return (fmv == fmv_Lua) ? pszDefaultMacroL : pszDefaultMacro;
}

LPCWSTR Settings::TabCloseMacro(FarMacroVersion fmv)
{
	if (sTabCloseMacro && *sTabCloseMacro && IsLuaMacroOk(fmv, sTabCloseMacro, FarTabCloseMacroDefault2))
		return sTabCloseMacro;
	return TabCloseMacroDefault(fmv);
}

LPCWSTR Settings::TabCloseMacroDefault(FarMacroVersion fmv)
{
	// L"@$if (Shell) F10 $if (Dialog) Enter $end $else F10 $end";
	static LPCWSTR pszDefaultMacro = FarTabCloseMacroDefault2;
	static LPCWSTR pszDefaultMacroL = FarTabCloseMacroDefault3;
	return (fmv == fmv_Lua) ? pszDefaultMacroL : pszDefaultMacro;
}

LPCWSTR Settings::SaveAllMacro(FarMacroVersion fmv)
{
	if (sSaveAllMacro && *sSaveAllMacro && IsLuaMacroOk(fmv, sSaveAllMacro, FarSaveAllMacroDefault2))
		return sSaveAllMacro;
	return SaveAllMacroDefault(fmv);
}

LPCWSTR Settings::SaveAllMacroDefault(FarMacroVersion fmv)
{
	// L"@F2 $If (!Editor) $Exit $End %i0=-1; F12 %cur = CurPos; Home Down %s = Menu.Select(\" * \",3,2); $While (%s > 0) $If (%s == %i0) MsgBox(\"FAR SaveAll\",\"Asterisk in menuitem for already processed window\",0x10001) $Exit $End Enter $If (Editor) F2 $If (!Editor) $Exit $End $Else $If (!Viewer) $Exit $End $End %i0 = %s; F12 %s = Menu.Select(\" * \",3,2); $End $If (Menu && Title==\"Screens\") Home $Rep (%cur-1) Down $End Enter $End $Exit"
	static LPCWSTR pszDefaultMacro = FarSaveAllMacroDefault2;
	static LPCWSTR pszDefaultMacroL = FarSaveAllMacroDefault3;
	return (fmv == fmv_Lua) ? pszDefaultMacroL : pszDefaultMacro;
}

#undef IsLuaMacroOk

bool Settings::CmdTaskGetDefaultShell(RConStartArgsEx& args, CEStr& lsTitle)
{
	lsTitle.Clear();

	// User defined default shell task? (Win+X)
	int nGroup = 0;
	const CommandTasks* pGrp = nullptr;
	while ((pGrp = gpSet->CmdTaskGet(nGroup++)))
	{
		if (pGrp->pszName && *pGrp->pszName
			&& (pGrp->Flags & CETF_CMD_DEFAULT))
		{
			// Process "/dir ..." and other switches
			pGrp->ParseGuiArgs(&args);
			SafeFree(args.pszSpecialCmd);

			// Run all tabs of the task, if they were specified
			args.pszSpecialCmd = lstrdup(pGrp->pszName).Detach();
			lsTitle.Set(pGrp->pszName);
			break;
		}
	}

	if (!args.pszSpecialCmd)
	{
		args.pszSpecialCmd = GetComspec(&gpSet->ComSpec).Detach(); //lstrdup(L"cmd");
		if (!args.pszSpecialCmd)
		{
			_ASSERTE(FALSE && "Memory allocation failure");
			return false;
		}

		CmdArg lsExe; LPCWSTR pszTemp = args.pszSpecialCmd;
		if ((pszTemp = NextArg(pszTemp, lsExe)))
			lsTitle.Set(PointToName(lsExe));
		if (lsTitle.IsEmpty())
			lsTitle.Set(L"cmd.exe");

		CEStr merged(args.pszSpecialCmd, L" /k \"%ConEmuBaseDir%\\CmdInit.cmd\"");
		if (merged)
		{
			SafeFree(args.pszSpecialCmd);
			args.pszSpecialCmd = merged.Detach();
		}
	}

	if (!args.pszStartupDir)
	{
		args.pszStartupDir = lstrdup(L"%CD%").Detach();
	}

	return true;
}

const CommandTasks* Settings::CmdTaskGet(int anIndex)
{
	if (anIndex == -1)
	{
		if (!StartupTask)
			StartupTask = (CommandTasks*)calloc(1, sizeof(CommandTasks));
		// Создать!
		return StartupTask;
	}

	if (!CmdTasks || (anIndex < 0) || (anIndex >= CmdTaskCount))
		return nullptr;

	if (CmdTasks[anIndex])
	{
		CmdTasks[anIndex]->HotKey.HkType = chk_Task;
		CmdTasks[anIndex]->HotKey.SetTaskIndex(anIndex);
		CmdTasks[anIndex]->HotKey.fkey = CConEmuCtrl::key_RunTask;
	}

	return (CmdTasks[anIndex]);
}

const CommandTasks* Settings::CmdTaskGetByName(LPCWSTR asTaskName)
{
	if (!asTaskName || !*asTaskName)
		return nullptr;

	const CommandTasks* pGrp = nullptr;

	wchar_t szName[MAX_PATH]; lstrcpyn(szName, asTaskName, countof(szName));
	wchar_t* psz = wcschr(szName, TaskBracketRight);
	if (psz) psz[1] = 0;

	if (lstrcmp(szName, AutoStartTaskName) == 0)
	{
		pGrp = CmdTaskGet(-1);
	}
	else
	{
		// Validate if it is a Task
		_ASSERTE(szName[0] == TaskBracketLeft && szName[wcslen(szName)-1] == TaskBracketRight);

		for (int i = 0; (pGrp = CmdTaskGet(i)) != nullptr; i++)
		{
			if (pGrp->pszName && (lstrcmpi(pGrp->pszName, szName) == 0))
			{
				break;
			}
		}

		// May be task was requested without subfolder or subfolder was stripped from task?
		if (!pGrp)
		{
			LPCWSTR pszCmpName = wcsstr(szName, L"::");

			if (!pszCmpName)
				pszCmpName = szName + 1; // Skip "{"
			else
				pszCmpName += 2; // Skip "::"

			for (int i = 0; (pGrp = CmdTaskGet(i)) != nullptr; i++)
			{
				if (!pGrp->pszName)
					continue;

				// Compare only task name, stripping "{Folder::"
				LPCWSTR pszTaskName = wcsstr(pGrp->pszName, L"::");
				if (!pszTaskName)
					pszTaskName = pGrp->pszName + 1; // Skip "{"
				else
					pszTaskName += 2; // Skip "::"

				if (lstrcmpi(pszTaskName, pszCmpName) == 0)
				{
					break;
				}
			}
		}
	}

	return pGrp;
}

// anIndex - 0-based, index of CmdTasks
void Settings::CmdTaskSetVkMod(int anIndex, DWORD VkMod)
{
	if (!CmdTasks || (anIndex < 0) || (anIndex >= CmdTaskCount))
		return;
	if (!CmdTasks[anIndex])
		return;
	CmdTasks[anIndex]->HotKey.SetVkMod(VkMod);
}

/// Add new or change Task contents
/// @param  anIndex - 0-based, index of CmdTasks, or `-1` to append new one
/// @param  asName  - Task name, or nullptr to delete this task (tail will be shifted upward)
/// @param  asCommands - Task's commands
/// @param  aFlags  - CETASKFLAGS
/// @result -1 if error occurred, or 0-based index of the Task
int Settings::CmdTaskSet(int anIndex, LPCWSTR asName, LPCWSTR asGuiArgs, LPCWSTR asCommands, CETASKFLAGS aFlags /*= CETF_DONT_CHANGE*/)
{
	if (anIndex == -1)
	{
		// Append new task at the end
		anIndex = CmdTaskCount;
	}

	if (anIndex < 0)
	{
		_ASSERTE(anIndex>=0);
		return -1;
	}

	// Kill existing task
	if (asName == nullptr)
	{
		if (!CmdTasks || (CmdTaskCount < 1))
			return -1;
		// Release pointers
		if (anIndex < CmdTaskCount)
		{
			CmdTasks[anIndex]->FreePtr();
		}
		// Shift the tail
		for (int i = (anIndex+1); i < CmdTaskCount; i++)
		{
			CmdTasks[i-1] = CmdTasks[i];
		}
		// Decrease overall count
		if (CmdTaskCount > 0)
		{
			CmdTasks[--CmdTaskCount] = nullptr;
		}
		return -1;
	}

	// Allocate more space?
	if (anIndex >= CmdTaskCount)
	{
		CommandTasks** ppNew = (CommandTasks**)calloc(anIndex+1,sizeof(CommandTasks*));
		if (!ppNew)
		{
			_ASSERTE(ppNew!=nullptr);
			return -1;
		}
		if ((CmdTaskCount > 0) && CmdTasks)
		{
			memmove(ppNew, CmdTasks, CmdTaskCount*sizeof(CommandTasks*));
		}
		SafeFree(CmdTasks);
		CmdTasks = ppNew;

		// CmdTaskCount will be incremented below
	}

	if (!CmdTasks)
	{
		_ASSERTE(CmdTasks);
		return -1;
	}

	// New task?
	bool bNewTask = false;
	CEStr lsName;
	if (CmdTasks[anIndex] == nullptr)
	{
		bNewTask = true;
		CmdTasks[anIndex] = (CommandTasks*)calloc(1, sizeof(CommandTasks));
		if (!CmdTasks[anIndex])
		{
			_ASSERTE(CmdTasks[anIndex]);
			return -1;
		}
		// Make unique name?
		if (aFlags & CETF_MAKE_UNIQUE)
		{
			// If task with same name exists - append suffix " (1)" ... " (999)"
			CEStr lsNaked((asName[0] == TaskBracketLeft) ? (asName+1) : asName);
			INT_PTR iLen = wcslen(lsNaked);
			if ((iLen > 0) && (lsNaked.ms_Val[iLen-1] == TaskBracketRight))
				lsNaked.ms_Val[iLen-1] = 0;

			for (UINT s = 0; s < 1000; s++)
			{
				bool bDuplicate = false;
				wchar_t szIndex[16] = L"";
				if (s) swprintf_c(szIndex, L" (%u)", s);
				lsName = CEStr(TaskBracketLeftS, lsNaked.ms_Val, szIndex, TaskBracketRightS);

				for (INT_PTR i = 0; (i < CmdTaskCount) && !bDuplicate; i++)
				{
					if ((i == anIndex) || (CmdTasks[i] == nullptr) || (CmdTasks[i]->pszName == nullptr))
						continue;
					bDuplicate = (lstrcmpi(CmdTasks[i]->pszName, lsName.ms_Val) == 0);
				}

				if (!bDuplicate)
				{
					if (s)
						asName = lsName;
					break;
				}
			}
		}
	}

	CmdTasks[anIndex]->SetName(asName, anIndex);
	CmdTasks[anIndex]->SetGuiArg(asGuiArgs);
	CmdTasks[anIndex]->SetCommands(asCommands);

	if (aFlags != CETF_DONT_CHANGE)
		CmdTasks[anIndex]->Flags = (aFlags & CETF_FLAGS_MASK);
	else if (bNewTask)
		CmdTasks[anIndex]->Flags = CETF_DEFAULT4NEW;

	if (anIndex >= CmdTaskCount)
		CmdTaskCount = anIndex+1;

	return anIndex;
}

bool Settings::CmdTaskXch(int anIndex1, int anIndex2)
{
	if (((anIndex1 < 0) || (anIndex1 >= CmdTaskCount))
		|| ((anIndex2 < 0) || (anIndex2 >= CmdTaskCount))
		|| !CmdTasks)
	{
		return false;
	}

	CommandTasks* p = CmdTasks[anIndex1];
	CmdTasks[anIndex1] = CmdTasks[anIndex2];
	CmdTasks[anIndex2] = p;

	return true;
}

// "\0" delimited
const wchar_t* Settings::GetDefaultTerminalAppsMSZ()
{
	return psDefaultTerminalApps;
}

// returns "|"-delimited string
CEStr Settings::GetDefaultTerminalApps()
{
	return MSZ2LineDelimited(psDefaultTerminalApps);
}
// "|"-delimited apszApps -> MSZ psDefaultTerminalApps
void Settings::SetDefaultTerminalApps(const wchar_t* apszApps)
{
	SafeFree(psDefaultTerminalApps);
	if (!apszApps || !*apszApps)
	{
		_ASSERTE(apszApps && *apszApps);
		apszApps = DEFAULT_TERMINAL_APPS/*L"explorer.exe"*/;
	}

	// "|" delimited String -> MSZ
	if (apszApps && *apszApps)
	{
		psDefaultTerminalApps = LineDelimited2MSZ(apszApps).Detach();
	}

	if (gpConEmu)
	{
		gpConEmu->OnDefaultTermChanged();
	}
}


// MSZ -> "<asDelim>"-delimited string
CEStr Settings::MSZ2LineDelimited(const wchar_t* apszLines, LPCWSTR asDelim /*= L"|"*/, bool bFinalToo /*= false*/)
{
	if (IsStrEmpty(apszLines))
	{
		return CEStr(L"");
	}

	// Evaluate required len
	INT_PTR nTotalLen = bFinalToo ? 2 : 1, nLen;
	const INT_PTR nDelimLen = asDelim ? _tcslen(asDelim) : 0;
	const wchar_t* psz = apszLines;
	while (*psz)
	{
		nLen = _tcslen(psz);
		psz += nLen + 1;
		nTotalLen += nLen + nDelimLen;
	}
	// Buffer
	CEStr result;
	wchar_t* pszRet = result.GetBuffer(nTotalLen);
	if (!pszRet)
	{
		_ASSERTE(pszRet);
		return CEStr(L"");
	}
	// Conversion
	wchar_t* pszDst = pszRet; psz = apszLines;
	while (*psz)
	{
		nLen = _tcslen(psz);
		wmemmove(pszDst, psz, nLen);
		psz += nLen+1; // + '\0'
		pszDst += nLen;

		if (*psz || bFinalToo)
		{
			if (nDelimLen > 0)
			{
				wmemmove(pszDst, asDelim, nDelimLen);
				pszDst += nDelimLen;
			}
		}

		if (!*psz)
		{
			*(pszDst++) = 0;
			break;
		}
	}
	*pszDst = 0;

	return result;
}

// "|"-delimited string -> MSZ
// !!! Returns LOWER-CASE string !!!
CEStr Settings::LineDelimited2MSZ(const wchar_t* apszApps, bool bLowerCase /*= true*/) const
{
	CEStr result;

	// "|" delimited String -> MSZ
	if (IsStrNotEmpty(apszApps))
	{
		const INT_PTR nLen = _tcslen(apszApps);
		auto* pszDst = result.GetBuffer(nLen + 3);

		if (pszDst)
		{
			wchar_t* psz = pszDst;
			while (*apszApps)
			{
				const wchar_t* pszNext = wcschr(apszApps, L'|');
				if (!pszNext) pszNext = apszApps + _tcslen(apszApps);

				if (pszNext > apszApps)
				{
					wchar_t* pszLwr = psz;
					wmemmove(psz, apszApps, pszNext-apszApps);
					psz += pszNext-apszApps;
					*(psz++) = 0;

					if (bLowerCase)
					{
						CharLowerBuff(pszLwr, pszNext-apszApps);
					}
				}

				if (!*pszNext)
					break;
				apszApps = pszNext + 1;
			}
			*(psz++) = 0;
			*(psz++) = 0; // ASCIIZZ
		}
	}

	return result;
}


// "\r\n"-delimited string -> MSZ
CEStr Settings::MultiLine2MSZ(const wchar_t* apszLines, DWORD* pcbSize/*in bytes*/) const
{
	CEStr result;
	DWORD cbSize = 0;

	if (apszLines && *apszLines)
	{
		INT_PTR nLenMax = lstrlen(apszLines) + 2;
		auto* pszDst = result.GetBuffer(nLenMax);
		if (pszDst == nullptr)
		{
			_ASSERTE(FALSE && "Memory allocation failed");
		}
		else
		{
			CEStr lsLine;
			wchar_t* psz = pszDst;
			LPCWSTR pszSrc = apszLines;
			while ((pszSrc = NextLine(pszSrc, lsLine, NLF_NONE)))
			{
				// We can't store empty lines in the middle of MSZZ value
				// That is a registry limitation
				if (lsLine.IsEmpty())
					lsLine.Set(L" ");
				const int iLineLen = lstrlen(lsLine.ms_Val) + 1;
				if ((psz - pszDst + 1 + iLineLen) >= nLenMax)
				{
					const INT_PTR nNewLenMax = std::max((psz - pszDst + 1 + iLineLen), nLenMax) + 1024;
					auto* pszRealloc = result.GetBuffer(nNewLenMax);
					if (!pszRealloc)
					{
						_ASSERTE(FALSE && "Reallocation failed");
						break;
					}
					psz = pszRealloc + (psz - pszDst);
					pszDst = pszRealloc;
					nLenMax = nNewLenMax;
				}
				_ASSERTE((psz - pszDst + 1 + iLineLen) <= nLenMax);

				wmemmove(psz, lsLine.ms_Val, iLineLen);
				psz += iLineLen;
			}

			cbSize = (psz - pszDst + 1)*sizeof(wchar_t);
			_ASSERTE(cbSize <= (nLenMax*sizeof(wchar_t)));
			_ASSERTE(*(psz-1) == 0 && psz < (pszDst + nLenMax));
			*psz = 0; // MSZZ
		}
	}

	if (pcbSize)
		*pcbSize = cbSize;
	return result;
}

bool Settings::LoadMSZ(SettingsBase* reg, LPCWSTR asName, CEStr& rsLines, LPCWSTR asDelim /*= L"|"*/, bool bFinalToo /*= false*/)
{
	wchar_t* pszMsz = nullptr; // MSZZ

	const bool bRc = reg->Load(asName, &pszMsz);
	if (bRc && pszMsz)
	{
		rsLines.Clear();
		rsLines = MSZ2LineDelimited(pszMsz, asDelim, bFinalToo);
		free(pszMsz);
	}

	return bRc;
}

// rsLines - "\r\n" separated lines
void Settings::SaveMSZ(SettingsBase* reg, LPCWSTR asName, LPCWSTR rsLines, LPCWSTR asDelim /*= L"|"*/, bool bLowerCase /*= true*/)
{
	// MSZZ
	DWORD nCbSize = 0;
	CEStr psMSZ = MultiLine2MSZ(rsLines, &nCbSize/*in bytes*/);

	reg->SaveMSZ(asName, psMSZ, nCbSize);
}

// "\0"-delimited
const wchar_t* Settings::GetIntelligentExceptionsMSZ()
{
	return pszCTSIntelligentExceptions;
}
// returns "|"-delimited
CEStr Settings::GetIntelligentExceptions()
{
	return MSZ2LineDelimited(pszCTSIntelligentExceptions);
}
// "|"-delimited apszApps -> MSZ pszCTSIntelligentExceptions
void Settings::SetIntelligentExceptions(const wchar_t* apszApps)
{
	SafeFree(pszCTSIntelligentExceptions);
	pszCTSIntelligentExceptions = LineDelimited2MSZ(apszApps).Detach();
}



// Вернуть заданный VkMod, или 0 если не задан
// nDescrID = vkXXX (e.g. vkMinimizeRestore)
// Используется при отображении клавиш в меню
DWORD Settings::GetHotkeyById(int nDescrID, const ConEmuHotKey** ppHK)
{
	static int iLastFound = -1;

	for (int j = -1;; j++)
	{
		if (j == -1 && iLastFound == -1)
			continue;

		const ConEmuHotKey *pHK = gpHotKeys->GetHotKeyPtr((j == -1) ? iLastFound : j);
		if (!pHK)
		{
			if (j == -1)
				continue;
			else
				break; // Кончились
		}

		if (pHK->DescrLangID == nDescrID)
		{
			if (j >= 0)
				iLastFound = j;

			DWORD VkMod = pHK->GetVkMod();
			if (pHK->HkType == chk_Modifier)
			{
				_ASSERTE(VkMod == ConEmuChord::GetHotkey(VkMod));
				VkMod = ConEmuChord::GetHotkey(VkMod); // младший байт
			}

			if (ppHK)
				*ppHK = pHK;
			return VkMod;
		}
	}

	return 0;
}

// Return hotkeyname by ID
LPCWSTR Settings::GetHotkeyNameById(int nDescrID, wchar_t (&szFull)[128], bool bShowNone /*= true*/)
{
	const ConEmuHotKey* pHK = nullptr;
	if (gpSet->GetHotkeyById(nDescrID, &pHK) && pHK)
	{
		pHK->GetHotkeyName(szFull, bShowNone);
	}
	else
	{
		wcscpy_c(szFull, bShowNone ? gsNoHotkey : L"");
	}
	return szFull;
}

// Проверить, задан ли этот hotkey
// nDescrID = vkXXX (e.g. vkMinimizeRestore)
bool Settings::IsHotkey(int nDescrID)
{
	DWORD nVk = ConEmuChord::GetHotkey(GetHotkeyById(nDescrID));
	return (nVk != 0);
}

// Установить новый hotkey
// nDescrID = vkXXX (e.g. vkMinimizeRestore)
// VkMod = LOBYTE - VK, старшие три байта - модификаторы (тоже VK)
void Settings::SetHotkeyById(int nDescrID, DWORD VkMod)
{
	if (nDescrID > 0)
	{
		if (!gpSetCls)
		{
			_ASSERTE(gpSetCls);
			return;
		}

		for (INT_PTR i = gpHotKeys->size() - 1; i >= 0; i--)
		{
			if ((*gpHotKeys)[i].DescrLangID == nDescrID)
			{
				(*gpHotKeys)[i].SetVkMod(VkMod);
				break;
			}
		}
	}
	else
	{
		_ASSERTE(nDescrID > 0); // Invalid descr!
	}

	CheckHotkeyUnique();
}

bool Settings::isModifierExist(BYTE Mod/*VK*/, bool abStrictSingle /*= false*/)
{
	for (int i = 0;; i++)
	{
		const ConEmuHotKey *ppHK = gpHotKeys->GetHotKeyPtr(i);
		if (!ppHK)
			break;

		if (ppHK->Key.IsEmpty())
			continue;

		if (ppHK->HkType == chk_Modifier)
		{
			if (ppHK->Key.Vk == Mod)
				return true;
		}
		else if (!abStrictSingle)
		{
			if (ConEmuChord::HasModifier(ppHK->GetVkMod(), Mod))
				return true;
		}
		else
		{
			DWORD VkMod = ppHK->GetVkMod();
			if ((ConEmuChord::GetModifier(VkMod, 1) == Mod) && !ConEmuChord::GetModifier(VkMod, 2))
				return true;
		}
	}

	return false;
}

// Есть ли такой хоткей или модификатор (актуально для VK_APPS)
bool Settings::isKeyOrModifierExist(BYTE Mod/*VK*/)
{
	for (int i = 0;; i++)
	{
		const ConEmuHotKey *ppHK = gpHotKeys->GetHotKeyPtr(i);
		if (!ppHK)
			break;

		if (ppHK->Key.IsEmpty())
			continue;

		if (ppHK->HkType == chk_Modifier)
			continue; // эти не рассматриваем

		if ((ppHK->Key.Vk == Mod) || ConEmuChord::HasModifier(ppHK->GetVkMod(), Mod))
			return true;
	}

	return false;
}

void Settings::LoadHotkeys(SettingsBase* reg, const bool& bSendAltEnter, const bool& bSendAltSpace, const bool& bSendAltF9)
{
	if (!reg)
	{
		_ASSERTE(reg!=nullptr);
		return;
	}

	if (!gpSetCls)
	{
		_ASSERTE(gpSetCls);
		return;
	}

	BOOL lbOpened = FALSE;
	wchar_t szMacroName[80];
	wchar_t szKeysPath[MAX_PATH+64];

	wcscpy_c(szKeysPath, gpSetCls->GetConfigPath());
	wchar_t* pszKeysRoot = szKeysPath + lstrlen(szKeysPath);
	wcscat_c(szKeysPath, L"\\HotKeys");

	lbOpened = reg->OpenKey(szKeysPath, KEY_READ);
	if (!lbOpened)
	{
		// HotKeys had been located originally in the root settings keys,
		// but have been moved to subkey later
		*pszKeysRoot = 0;
		lbOpened = reg->OpenKey(szKeysPath, KEY_READ);
		if (!lbOpened)
		{
			_ASSERTE(FALSE && "Settings Open operation failed");
			return;
		}
	}

	// Compatibility with old settings format
	BYTE MacroVersion = 0;
	reg->Load(L"KeyMacroVersion", MacroVersion);

	reg->Load(L"Multi.Modifier", nHostkeyNumberModifier); ConEmuChord::TestHostkeyModifiers(nHostkeyNumberModifier);
	reg->Load(L"Multi.ArrowsModifier", nHostkeyArrowModifier); ConEmuChord::TestHostkeyModifiers(nHostkeyArrowModifier);

	INT_PTR iMax = gpHotKeys->size();
	for (int i = 0; i < iMax; i++)
	{
		ConEmuHotKey *ppHK = &((*gpHotKeys)[i]);
		ppHK->NotChanged = true;

		if (!*ppHK->Name)
		{
			continue;
		}

		// Эти модификаторы раньше в настройке не сохранялись, для совместимости - добавляем VK_SHIFT к предыдущей
		switch (ppHK->DescrLangID)
		{
		case vkMultiNewConfirm:
		case vkMultiNextShift:
			if (i > 0)
			{
				ConEmuHotKey *ppHK1 = &((*gpHotKeys)[i-1]);
				_ASSERTE(ppHK1->DescrLangID == ((ppHK->DescrLangID == vkMultiNewConfirm) ? vkMultiNew : vkMultiNext));
				ppHK->Key = ppHK1->Key;
				if (ppHK->Key.Mod & (cvk_Shift|cvk_LShift|cvk_RShift))
					ppHK->Key.Mod &= ~(cvk_Shift|cvk_LShift|cvk_RShift);
				else
					ppHK->Key.Mod |= cvk_Shift;
			}
			break;
		}

		_ASSERTE(ppHK->HkType != chk_NumHost);
		_ASSERTE(ppHK->HkType != chk_ArrHost);
		_ASSERTE(ppHK->HkType != chk_System);

		DWORD VkMod = ppHK->GetVkMod();
		if (reg->Load(ppHK->Name, VkMod))
		{
			// Чтобы знать, что комбинация уже сохранялась в настройке ранее
			ppHK->NotChanged = false;

			if (ppHK->HkType != chk_Modifier)
			{
				// Если это НЕ 0 (0 - значит HotKey не задан)
				// И это не DWORD (для HotKey без модификатора - пишется CEHOTKEY_NOMOD)
				// Т.к. раньше был Byte - нужно добавить nHostkeyModifier
				if (VkMod && ((VkMod & CEHOTKEY_MODMASK) == 0))
				{
					// Добавляем тот модификатор, который был раньше общий
					_ASSERTE(nHostkeyNumberModifier!=0);
					VkMod |= (nHostkeyNumberModifier << 8);
				}
			}

			ppHK->SetVkMod(VkMod);
		}

		if (ppHK->HkType == chk_Macro)
		{
			wcscpy_c(szMacroName, ppHK->Name);
			wcscat_c(szMacroName, L".Text");
			wchar_t* pszMacro = nullptr;
			if (reg->Load(szMacroName, &pszMacro))
			{
				SafeFree(ppHK->GuiMacro);
				if (MacroVersion < GUI_MACRO_VERSION)
				{
					ppHK->GuiMacro = ConEmuMacro::ConvertMacro(pszMacro, MacroVersion, true).Detach();
					SafeFree(pszMacro);
				}
				else
				{
					ppHK->GuiMacro = pszMacro;
				}
			}
		}
	}

	reg->CloseKey();

	// Для совместимости настроек
	if (bSendAltSpace || bSendAltEnter || bSendAltF9)
	{
		ConEmuHotKey* pHK;
		// Если раньше был включен флажок "Send Alt+Space to console"
		if (bSendAltSpace && GetHotkeyById(vkSystemMenu, (const ConEmuHotKey**)&pHK) && pHK->NotChanged && (pHK->Equal(VK_SPACE,VK_MENU)))
		{
			pHK->Key.Set(); // Сбросить VkMod для vkSystemMenu (раньше назывался vkAltSpace)
		}
		// Если раньше был включен флажок "Send Alt+Enter to console"
		if (bSendAltEnter && GetHotkeyById(vkAltEnter, (const ConEmuHotKey**)&pHK) && pHK->NotChanged && (pHK->Equal(VK_RETURN,VK_MENU)))
		{
			pHK->Key.Set(); // Сбросить VkMod для vkAltEnter
		}
		// Если раньше был включен флажок "Send Alt+F9 to console"
		if (bSendAltF9 && GetHotkeyById(vkMaximize, (const ConEmuHotKey**)&pHK) && pHK->NotChanged && (pHK->Equal(VK_F9,VK_MENU)))
		{
			pHK->Key.Set(); // Сбросить VkMod для vkMaximize
		}
	}
}

void Settings::CheckHotkeyUnique()
{
	// Check for duplicates
	CEStr szFailMsg;

	// Некоторые хоткеи имеют "локальное" действие
	// А некоторые проверять не хочется
	int SkipCheckID[] = {vkCtrlTab_Left, vkCtrlTab_Up, vkCtrlTab_Right, vkCtrlTab_Down, vkEscNoConsoles};
	bool bSkip = false;
	int iHK1, iHK2;
	const ConEmuHotKey *ppHK1, *ppHK2;

	WARNING("Refactoring: create the list of enabled keys, sort them, and find non-unique easy");

	// Go
	for (iHK1 = 0;; iHK1++)
	{
		ppHK1 = gpHotKeys->GetHotKeyPtr(iHK1);
		if (!ppHK1)
			break;
		// Сразу проверить следующий за ним
		ppHK2 = gpHotKeys->GetHotKeyPtr(iHK1+1);
		if (!ppHK2)
			break;


		if ((ppHK1->HkType == chk_Modifier) || (ppHK1->Key.Vk == 0))
			continue;

		// Отключен пользователем?
		if (ppHK1->Enabled && !ppHK1->Enabled())
			continue;

		// Некоторые хоткеи не проверять
		bSkip = false;
		for (size_t i = 0; i < countof(SkipCheckID); i++)
		{
			if (ppHK1->DescrLangID == SkipCheckID[i])
			{
				bSkip = true; break;
			}
		}
		if (bSkip)
			continue;

		//for (ConEmuHotKey *ppHK2 = ppHK1+1; ppHK2[0].DescrLangID; ++ppHK2)
		for (iHK2 = iHK1+1;; iHK2++)
		{
			ppHK2 = gpHotKeys->GetHotKeyPtr(iHK2);
			if (!ppHK2)
				break;

			if ((ppHK2->HkType == chk_Modifier) || (ppHK2->Key.Vk == 0))
				continue;

			// Отключен пользователем?
			if (ppHK2->Enabled && !ppHK2->Enabled())
				continue;

			// Некоторые хоткеи не проверять
			bSkip = false;
			for (size_t i = 0; i < countof(SkipCheckID); i++)
			{
				if (ppHK2->DescrLangID == SkipCheckID[i])
				{
					bSkip = true; break;
				}
			}
			if (bSkip)
				continue;

			// Хоткеи различаются?
			if (!ppHK1->Key.IsEqual(ppHK2->Key))
				continue;

			// Если совпадают - может быть это макрос, переехавший в системную область?
			if (((ppHK1->HkType == chk_Macro) || (ppHK2->HkType == chk_Macro))
				&& ppHK1->GuiMacro && *ppHK1->GuiMacro && ppHK2->GuiMacro && *ppHK2->GuiMacro)
			{
				if (lstrcmp(ppHK1->GuiMacro, ppHK2->GuiMacro) == 0)
				{
					// Нам - можно
					if (ppHK1->HkType == chk_Macro)
						((ConEmuHotKey*)ppHK1)->Key.Set();
					else
						((ConEmuHotKey*)ppHK2)->Key.Set();
					continue;
				}
			}

			wchar_t szDescr1[512], szDescr2[512], szKey[128];

			ppHK1->GetDescription(szDescr1, countof(szDescr1), true);
			ppHK2->GetDescription(szDescr2, countof(szDescr2), true);

			ppHK1->GetHotkeyName(szKey);

			ConEmuHotKey::CreateNotUniqueWarning(szKey, szDescr1, szDescr2, szFailMsg);

			goto wrap;
		}
	}
wrap:
	if (szFailMsg)
	{
		Icon.ShowTrayIcon(szFailMsg, tsa_Config_Error);
	}
}

void Settings::SaveHotkeys(SettingsBase* reg, int SaveDescrLangID /*= 0*/)
{
	if (!reg)
	{
		_ASSERTE(reg!=nullptr);
		return;
	}

	if (!gpSetCls)
	{
		_ASSERTE(gpSetCls);
		return;
	}

	BOOL lbOpened = FALSE;
	wchar_t szMacroName[80];
	wchar_t szKeysPath[MAX_PATH+64];

	wcscpy_c(szKeysPath, gpSetCls->GetConfigPath());
	wcscat_c(szKeysPath, L"\\HotKeys");

	// Write "Count" before first "Palette"
	int UserCount = 0;
	lbOpened = reg->OpenKey(szKeysPath, KEY_WRITE);
	if (!lbOpened)
	{
		_ASSERTE(FALSE && "Settings Open operation failed");
		return;
	}

	if (!SaveDescrLangID)
	{
		BYTE MacroVersion = GUI_MACRO_VERSION;
		reg->Save(L"KeyMacroVersion", MacroVersion);

		reg->Save(L"Multi.Modifier", nHostkeyNumberModifier);
		reg->Save(L"Multi.ArrowsModifier", nHostkeyArrowModifier);
	}

	// Таски сохраняются отдельно

	// Здесь - хоткеи и макро
	INT_PTR iMax = gpHotKeys->size();
	for (INT_PTR i = 0; i < iMax; i++)
	{
		ConEmuHotKey *ppHK = &((*gpHotKeys)[i]);

		if (!*ppHK->Name)
			continue;

		if (SaveDescrLangID && (ppHK->DescrLangID != SaveDescrLangID))
			continue;

		DWORD VkMod = ppHK->GetVkMod();

		if ((ppHK->HkType == chk_Modifier)
			|| (ppHK->HkType == chk_NumHost)
			|| (ppHK->HkType == chk_ArrHost))
		{
			_ASSERTE(VkMod == (VkMod & 0xFF));
			BYTE Mod = (BYTE)(VkMod & 0xFF);
			reg->Save(ppHK->Name, Mod);
		}
		else
		{
			reg->Save(ppHK->Name, VkMod);

			if (ppHK->HkType == chk_Macro)
			{
				wcscpy_c(szMacroName, ppHK->Name);
				wcscat_c(szMacroName, L".Text");
				reg->Save(szMacroName, ppHK->GuiMacro);
			}
		}
	}

	reg->CloseKey();
}
