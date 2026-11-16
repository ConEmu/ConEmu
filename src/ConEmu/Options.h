
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

#define MAX_CONFIG_NAME 240

#define MIN_ALPHA_VALUE 40
#define MIN_INACTIVE_ALPHA_VALUE 0
#define MAX_ALPHA_VALUE 255

//TODO: #define MAX_FONT_GROUPS 20 // Main, Borders, Japan, Cyrillic, ...
// #define MAX_FONT_STYLES 8 //normal/(bold|italic|underline)
enum CEFontStyles
{
	fnt_Normal    = 0,
	fnt_Bold      = 1,
	fnt_Italic    = 2,
	fnt_Underline = 4,

	fnt_StdFontMask = (fnt_Bold|fnt_Italic|fnt_Underline), // 7
	MAX_FONT_STYLES = (fnt_StdFontMask+1), // 8

	fnt_UCharMap    = (MAX_FONT_STYLES),   // 8

	fnt_Alternative = (MAX_FONT_STYLES+1), // 9 (old fnt_FarBorders)

	fnt_NULL        = (MAX_FONT_STYLES+2), // 10 (used in VirtualConsole SelectFont)
};

// #include "../common/DefTermBase.h"
enum class TerminalConfirmClose : uint32_t;

#define CURSORSIZE_MIN 5
#define CURSORSIZE_MAX 100
#define CURSORSIZEPIX_MIN 1
#define CURSORSIZEPIX_STD 2
#define CURSORSIZEPIX_MAX 99

// Animation delay used not only in Quake mode,
// but also when hiding/restoring window to/from TSA
#define QUAKEANIMATION_DEF 300
#define QUAKEANIMATION_MAX 2000
#define QUAKEVISIBLELIMIT 80 // Если "Видимая область" окна стала менее (%) - считаем что окно стало "не видимым"
#define QUAKEVISIBLETRASH 10 // Не "выезжать" а просто "вынести наверх", если видимая область достаточно большая

#define HIDECAPTIONALWAYSFRAME_DEF 255
#define HIDECAPTIONALWAYSFRAME_MAX 0x7F

#define RUNQUEUE_CREATE_LAG_DEF 100
#define RUNQUEUE_CREATE_LAG_MIN 10 // 10 ms
#define RUNQUEUE_CREATE_LAG_MAX (10*1000) // 10 sec

enum FarMacroVersion
{
	fmv_Default = 0,
	fmv_Standard,
	fmv_Lua,
};

// -> Header.h
//enum BackgroundOp
//{
//	eUpLeft = 0,
//	eStretch = 1,
//	eTile = 2,
//	eUpRight = 3,
//	eDownLeft = 4,
//	eDownRight = 5,
//};

#define BgImageColorsDefaults (1|2)
#define DefaultSelectionConsoleColor 0xE0

#include "Hotkeys.h"
#include "SetTypes.h"
#include "SetAppSettings.h"
#include "SetCmdTask.h"
#include "UpdateSet.h"
#include "../common/PaletteColors.h"
#include "../common/wcwidth.h"

class CSettings;
class CSetDlgButtons;
struct CommandHistory;
struct ColorPalette;


#define SCROLLBAR_DELAY_MIN 100
#define SCROLLBAR_DELAY_MAX 15000

#define CENTERCONSOLEPAD_MIN 0
#define CENTERCONSOLEPAD_MAX 64


#define DefaultAdminTitleSuffix L" (Admin)"

#define DEFAULT_TERMINAL_APPS L"explorer.exe"

struct TabBarDblClick
{
	enum {
		NoAction         = 0,
		Auto             = 1,
		MaxRestoreWindow = 2,
		OpenNewShell     = 3,
		Last
	};
};
#define TABBAR_DEFAULT_CLICK_ACTION  TabBarDblClick::Auto

struct TabBtnDblClick
{
	enum {
		NoAction         = 0,
		MaxRestoreWindow = 1,
		CloseTab         = 2,
		RestartTab       = 3,
		DuplicateTab     = 4,
		MaxRestorePane   = 5,
		RenameTab        = 6,
		Last
	};
};
#define TABBTN_DEFAULT_CLICK_ACTION  TabBtnDblClick::RenameTab

// ‘%1’ - line number, ‘%2’ - column number, ‘%3’ - C:\\Path\\File, ‘%4’ - C:/Path/File, ‘%5’ - /C/Path/File
#define HI_GOTO_EDITOR_FAR     L"{Far} /e%1:%2 \"%3\""
#define HI_GOTO_EDITOR_VIMW    L"vim.exe +%1 \"%3\""
// Use '#' prefix to run GUI editor outside of ConEmu
#define HI_GOTO_EDITOR_SCITE   L"#scite.exe \"-open:%4\" -goto:%1,%2"
#define HI_GOTO_EDITOR_NPADP   L"#notepad++.exe -n%1 \"%3\""
#define HI_GOTO_EDITOR_SUBLM   L"#sublime_text.exe \"%3:%1:%2\""
// And just a starter for highlighted file
#define HI_GOTO_EDITOR_CMD     L"cmd.exe /c \"echo Starting \"%3\" & \"%3\"\""
#define HI_GOTO_EDITOR_CMD_NC  L"cmd.exe /c \"echo Starting \"%3\" & \"%3\"\" -new_console:n"

enum class MouseButtonAction : uint8_t
{
	// Mouse button click does nothing.
	None = 0,
	// Copy selected text to Windows clipboard if selection exists.
	Copy = 1,
	// Always paste text from Windows clipboard.
	Paste = 2,
	// If there is no selection in console - do paste from Windows clipboard.
	// If selection exists and mouse cursor is *outside* the  - do copy to Windows cliboard.
	// If mouse cursor is *inside* the selection region - do paste of selected text without touching Windows clipboard.
	Auto = 3,
	Menu = 4,
	// Last one for comparison.
	MaxId,
};


extern const wchar_t gsDefaultColorScheme[64]; // = L"<ConEmu>";


struct Settings
{
	public:
		Settings();
		~Settings();
	protected:
		friend class CSettings;
		friend class CSetDlgButtons;
		friend class CSetDlgColors;
		friend class CSetPgApps;
		friend class CSetPgMarkCopy;
		friend class CSetPgColors;
		friend class CSetPgKeys;

		void ResetSettings();
		void ReleasePointers();
	public:

		//reg->Load(L"Language", Language, countof(Language))
		wchar_t Language[8]; // "ru", "en", "zh", ...

		//reg->Load(L"DynamicBufferHeight", DynamicBufferHeight);
		bool isDynamicBufferHeight;
		//reg->Load(L"DefaultBufferHeight", DefaultBufferHeight);
		int DefaultBufferHeight;
		//reg->Load(L"AutoBufferHeight", AutoBufferHeight);
		bool AutoBufferHeight; // Long console output

		//reg->Load(L"UseScrollLock", UseScrollLock);
		bool UseScrollLock;

		ConEmuComspec ComSpec;

		//reg->Load(L"AutoReloadEnvironment", AutoReloadEnvironment);
		bool AutoReloadEnvironment;
		//reg->LoadMSZ(L"EnvironmentSet", psEnvironmentSet);
		CEStr* psEnvironmentSet; // commands: multiline, "\r\n" separated

		// Service functions
		CEStr LineDelimited2MSZ(const wchar_t* apszApps, bool bLowerCase = true) const;                    // "|"-delimited string -> MSZ
		CEStr MSZ2LineDelimited(const wchar_t* apszLines, LPCWSTR asDelim = L"|", bool bFinalToo = false); // MSZ -> "<asDelim>"-delimited string
		CEStr MultiLine2MSZ(const wchar_t* apszLines, DWORD* pcbSize/*in bytes*/) const;                   // "\r\n"-delimited string -> MSZ

		bool LoadMSZ(SettingsBase* reg, LPCWSTR asName, CEStr& rsLines, LPCWSTR asDelim /*= L"|"*/, bool bFinalToo /*= false*/);
		void SaveMSZ(SettingsBase* reg, LPCWSTR asName, LPCWSTR rsLines, LPCWSTR asDelim /*= L"|"*/, bool bLowerCase /*= true*/);

		// Replace default terminal
		bool isSetDefaultTerminal;
		bool isRegisterOnOsStartup;
		bool isRegisterOnOsStartupTSA;
		bool isRegisterAggressive;
		bool isDefaultTerminalNoInjects;
		bool isDefaultTerminalNewWindow;
		bool isDefaultTerminalDebugLog;
		TerminalConfirmClose nDefaultTerminalConfirmClose; // "Press Enter to close console". 0 - Auto, 1 - Always, 2 - Never
		CEStr GetDefaultTerminalApps(); // "|" delimited
		const wchar_t* GetDefaultTerminalAppsMSZ(); // "\0" delimited
		void SetDefaultTerminalApps(const wchar_t* apszApps); // "|" delimited
	private:
		wchar_t* psDefaultTerminalApps; // MSZ

	public:
		int GetAppSettingsId(LPCWSTR asExeAppName, bool abElevated);
		const AppSettings* GetAppSettings(int anAppId=-1);
	private:
		static bool GetDefColors(LPCWSTR asDefName, ConEmu::PaletteColors& colors);
		static void GetWindowsColors(ConEmu::PaletteColors& colors);
	public:
		const ConEmu::PaletteColors& GetColors(int anAppId=-1, BOOL abFade = FALSE);
		const ConEmu::PaletteColors& GetColorsPrepare(const ConEmu::PaletteColors& colors,
			ConEmu::PaletteColors& colorsFade, bool* pbFadeInitialized, BOOL abFade);
		void PrepareFadeColors(const ConEmu::PaletteColors& colors, ConEmu::PaletteColors& colorsFade, bool* pbFadeInitialized);
		const ConEmu::PaletteColors& GetPaletteColors(LPCWSTR asPalette, BOOL abFade = FALSE);
		COLORREF GetFadeColor(COLORREF cr);
		void ResetFadeColors();

		bool CmdTaskGetDefaultShell(RConStartArgsEx& args, CEStr& lsTitle);
		const CommandTasks* CmdTaskGet(int anIndex); // 0-based, index of CmdTasks. "-1" == autosaved task
		const CommandTasks* CmdTaskGetByName(LPCWSTR asTaskName);
		void CmdTaskSetVkMod(int anIndex, DWORD VkMod); // 0-based, index of CmdTasks
		int  CmdTaskSet(int anIndex, LPCWSTR asName, LPCWSTR asGuiArgs, LPCWSTR asCommands, CETASKFLAGS aFlags = CETF_DONT_CHANGE); // 0-based, index of CmdTasks
		bool CmdTaskXch(int anIndex1, int anIndex2); // 0-based, index of CmdTasks

		const ColorPalette* PaletteGet(int anIndex); // 0-based, index of Palettes, or -1 for "<Current color scheme>"
		const ColorPalette* PaletteGetByName(LPCWSTR asName);
		const ColorPalette* PaletteFindCurrent(bool bMatchAttributes);
		const ColorPalette* PaletteFindByColors(bool bMatchAttributes, const ColorPalette* pCur);
		int PaletteGetIndex(LPCWSTR asName);
		void PaletteSaveAs(LPCWSTR asName); // Save active colors to named palette
		void PaletteSaveAs(LPCWSTR asName, BYTE anTextColorIdx, BYTE anBackColorIdx, BYTE anPopTextColorIdx, BYTE anPopBackColorIdx, const ConEmu::PaletteColors& aColors, bool abSaveSettings);
		void PaletteDelete(LPCWSTR asName); // Delete named palette
		void PaletteSetStdIndexes();
		int PaletteSetActive(LPCWSTR asName);


		//
		struct ConEmuProgressStore
		{
			// This two are stored in settings
			wchar_t* pszName;
			DWORD    nDuration;
			// Following are used in runtime
			DWORD    nStartTick;

			void FreePtr()
			{
				SafeFree(pszName);
			};
		};

		DWORD ProgressesGetDuration(LPCWSTR asName);
		void ProgressesSetDuration(LPCWSTR asName, DWORD anDuration);

	public:
		AppSettings AppStd;
	protected:
		friend struct AppSettings;
		int AppCount;
		AppSettings** Apps;
		// Для CSettings
		AppSettings* GetAppSettingsPtr(int anAppId, BOOL abCreateNew = FALSE);
		void AppSettingsDelete(int anAppId);
		bool AppSettingsXch(int anIndex1, int anIndex2); // 0-based, index of Apps

		int CmdTaskCount;
		CommandTasks** CmdTasks;
		CommandTasks* StartupTask;
		void FreeCmdTasks();
		void FreeStartupTask();

		int PaletteCount;
		ColorPalette** Palettes;
		ColorPalette* PaletteGetPtr(int anIndex); // 0-based, index of Palettes
		void SavePalettes(SettingsBase* reg);
		void SortPalettes();
		void FreePalettes();

		int ProgressesCount;
		ConEmuProgressStore** Progresses;
		bool LoadProgress(SettingsBase* reg, ConEmuProgressStore* &pProgress);
		bool SaveProgress(SettingsBase* reg, ConEmuProgressStore* pProgress);
		void FreeProgresses();

		void SaveStatusSettings(SettingsBase* reg);

		void LoadSizeSettings(SettingsBase* reg);
		void SaveSizeSettings(SettingsBase* reg);

	private:
		// reg->Load(L"ColorTableNN", Colors[i]);
		ConEmu::PaletteColors Colors{};
		ConEmu::PaletteColors ColorsFade{};
		bool mb_FadeInitialized = false;

		void LoadCursorSettings(SettingsBase* reg, CECursorType* pActive, CECursorType* pInactive);

		void LoadAppsSettings(SettingsBase* reg, bool abFromOpDlg = false);
		void LoadAppSettings(SettingsBase* reg, AppSettings* pApp);
		void SaveAppSettings(SettingsBase* reg, AppSettings* pApp);

		void SaveStdColors(SettingsBase* reg);
		void SaveStartCommands(SettingsBase* reg);

		void FreeApps(int NewAppCount = 0, AppSettings** NewApps = nullptr/*, Settings::CEAppColors** NewAppColors = nullptr*/);

		DWORD mn_FadeMul;
		inline BYTE GetFadeColorItem(BYTE c);

	public:
		//reg->Load(L"FontAutoSize", isFontAutoSize);
		bool isFontAutoSize;
		//reg->Load(L"AutoRegisterFonts", isAutoRegisterFonts);
		bool isAutoRegisterFonts;

		//reg->Load(L"ConsoleFontName", ConsoleFont.lfFaceName, countof(ConsoleFont.lfFaceName));
		//reg->Load(L"ConsoleFontWidth", ConsoleFont.lfWidth);
		//reg->Load(L"ConsoleFontHeight", ConsoleFont.lfHeight);
		LOGFONT ConsoleFont;

		bool NeedDialogDetect();


		//reg->Load(L"TrueColorerSupport", isTrueColorer);
		bool isTrueColorer;

		//reg->Load(L"VividColors", isVividColors);
		bool isVividColors;


		/* *** Background image *** */
		//reg->Load(L"BackGround Image show", isShowBgImage);
		BYTE isShowBgImage;
		//reg->Load(L"BackGround Image", sBgImage, countof(sBgImage));
		WCHAR sBgImage[MAX_PATH];
		//reg->Load(L"bgImageDarker", bgImageDarker);
		uint8_t bgImageDarker;
		//reg->Load(L"bgImageColors", nBgImageColors);
		DWORD nBgImageColors;
		//reg->Load(L"bgOperation", bgOperation);
		BYTE bgOperation; // BackgroundOp {eUpLeft = 0, eStretch = 1, eTile = 2, ...}
		//reg->Load(L"bgPluginAllowed", isBgPluginAllowed);
		char isBgPluginAllowed;


		//bool isBackgroundImageValid;



		/* *** Transparency *** */
		bool isTransparentAllowed();
		//reg->Load(L"AlphaValue", nTransparent);
		uint8_t nTransparent;
		//reg->Load(L"AlphaValueSeparate", nTransparentSeparate);
		bool isTransparentSeparate;
		//reg->Load(L"AlphaValueInactive", nTransparentInactive);
		uint8_t nTransparentInactive;
		//reg->Load(L"UserScreenTransparent", isUserScreenTransparent);
		bool isUserScreenTransparent;
		//reg->Load(L"ColorKeyTransparent", isColorKeyTransparent);
		bool isColorKeyTransparent;
		//reg->Load(L"ColorKeyValue", nColorKeyValue);
		DWORD nColorKeyValue;

		/* *** Command Line History (from start dialog) *** */
		//reg->Load(L"SaveCmdHistory", isSaveCmdHistory);
		bool isSaveCmdHistory;
		//reg->Load(L"CmdLineHistory", &psCmdHistory);
		CommandHistory* pHistory;
		//reg->Load(L"CmdHistoryLocation", &psHistoryLocation);
		wchar_t* psHistoryLocation;
		// Helpers
		void HistoryAdd(LPCWSTR asCmd);
		void HistoryReset();
		void HistoryLoad(SettingsBase* reg);
		void HistorySave(SettingsBase* reg);
		LPCWSTR HistoryGet(int index);

		/* *** Startup options *** */
		//reg->Load(L"StartType", nStartType);
		BYTE nStartType; // 0-cmd line, 1-cmd task file, 2-named task, 3-auto saved task (*StartupTask)
		//reg->Load(L"CmdLine", &psStartSingleApp);
		LPTSTR psStartSingleApp;
		//reg->Load(L"StartTasksFile", &psStartTasksFile);
		LPTSTR psStartTasksFile; // Stored with prefix, e.g. "@D:\ConEmu\test-start.conemu"
		//reg->Load(L"StartTasksName", &psStartTasksName);
		LPTSTR psStartTasksName;
		//reg->Load(L"StartFarFolders", isStartFarFolders);
		bool isStartFarFolders;
		//reg->Load(L"StartFarEditors", isStartFarEditors);
		bool isStartFarEditors;
		//reg->Load(L"StartCreateDelay", nStartCreateDelay);
		UINT nStartCreateDelay; // RUNQUEUE_CREATE_LAG

		//reg->Load(L"StoreTaskbarkTasks", isStoreTaskbarkTasks);
		bool isStoreTaskbarkTasks; // default is true
		//reg->Load(L"StoreTaskbarCommands", isStoreTaskbarCommands);
		bool isStoreTaskbarCommands;

		// "JumpListAutoUpdate"
		bool isJumpListAutoUpdate; // default is true


	//	/* Command Line ("/cmd" arg) */
	//private:
	//	LPTSTR psCurCmd;
	//public:
	//	LPCTSTR GetCurCmd();
	//	void SetCmdPtr(wchar_t*& psNewCmd);


		/* 'Default' command line (if nor Registry, nor /cmd specified) */
		//WCHAR  szDefCmd[16];
	public:
		//reg->Load(L"FontName", inFont, countof(inFont))
		wchar_t inFont[LF_FACESIZE];
		//reg->Load(L"FontBold", isBold);
		bool isBold;
		//reg->Load(L"FontItalic", isItalic);
		bool isItalic;
		//reg->Load(L"Anti-aliasing", Quality);
		UINT mn_AntiAlias; //загружался как Quality
		//reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;
		BYTE mn_LoadFontCharSet; // То что загружено изначально (или уже сохранено в реестр)
		//reg->Load(L"FontCharSet", mn_LoadFontCharSet); mb_CharSetWasSet = FALSE;
		BOOL mb_CharSetWasSet;
		//reg->Load(L"FontSize", FontSizeY);
		UINT FontSizeY;  // высота основного шрифта (загруженная из настроек!)
		//reg->Load(L"FontSizeX", FontSizeX);
		UINT FontSizeX;  // ширина основного шрифта
		//reg->Load(L"FontSizeX3", FontSizeX3);
		UINT FontSizeX3; // ширина знакоместа при моноширинном режиме (не путать с FontSizeX2)
		//reg->Load(L"FontUseDpi", FontUseDpi);
		bool FontUseDpi;
		//reg->Load(L"FontUseUnits", FontUseUnits);
		bool FontUseUnits;

		// "CompressLongStrings"
		bool isCompressLongStrings; // default is true

		// Previously, the option was used to define different font
		// generally for Far Manager frames (pseudographics)
		// Now user may use it for any range of characters (CJK, etc.)
		// Used with settings: inFont2, FontSizeX2, mpc_CharAltFontRanges
		// "FixFarBorders"
		BYTE isFixFarBorders;
		// "FontName2"
		wchar_t inFont2[LF_FACESIZE];
		// "FontSizeX2" : width of *alternative* font (to avoid dashed frames this may be larger than main font width)
		UINT FontSizeX2;
		// "Anti-aliasing2"
		bool isAntiAlias2;
		// "FixFarBordersRanges" ==> ParseCharRanges(...)
		// Default: "2013-25C4"; Example: "0410-044F;2013-25C4;"
		BYTE mpc_CharAltFontRanges[0x10000];
		int ParseCharRanges(LPCWSTR asRanges, BYTE (&Chars)[0x10000], BYTE abValue = TRUE);
		CEStr CreateCharRanges(BYTE (&Chars)[0x10000]); // caller must free(result)
		bool CheckCharAltFont(ucs32 inChar);


		//reg->Load(L"HideCaption", isHideCaption);
		bool isHideCaption; // Hide caption when maximized
		//reg->Load(L"HideChildCaption", isHideChildCaption);
		bool isHideChildCaption; // Hide caption of child GUI applications, started in ConEmu tabs (PuTTY, Notepad, etc.)
		//reg->Load(L"FocusInChildWindows", isFocusInChildWindows);
		bool isFocusInChildWindows;
		//reg->Load(L"IntegralSize", mb_IntegralSize);
		bool mb_IntegralSize;
		//reg->Load(L"QuakeStyle", isQuakeStyle);
		BYTE isQuakeStyle; // 0 - NoQuake, 1 - Quake, 2 - Quake+HideOnLoseFocus
		UINT nQuakeAnimation;
		//reg->Load(L"Restore2ActiveMon", isRestore2ActiveMon);
		bool isRestore2ActiveMon;
		//reg->Load(L"RestoreInactive", isRestoreInactive);
		bool isRestoreInactive;
		protected:
		//reg->Load(L"HideCaptionAlways", mb_HideCaptionAlways);
		bool mb_HideCaptionAlways;
		public:
		void SetHideCaptionAlways(bool bHideCaptionAlways);
		void SwitchHideCaptionAlways();
		bool isHideCaptionAlways(); //<<mb_HideCaptionAlways
		bool isMinimizeOnLoseFocus();
		bool isForcedHideCaptionAlways(); // true, если mb_HideCaptionAlways отключать нельзя
		//reg->Load(L"HideCaptionAlwaysFrame", nHideCaptionAlwaysFrame);
		BYTE nHideCaptionAlwaysFrame;
		int HideCaptionAlwaysFrame();
		//reg->Load(L"HideCaptionAlwaysDelay", nHideCaptionAlwaysDelay);
		UINT nHideCaptionAlwaysDelay;
		//reg->Load(L"HideCaptionAlwaysDisappear", nHideCaptionAlwaysDisappear);
		UINT nHideCaptionAlwaysDisappear;
		//reg->Load(L"DownShowHiddenMessage", isDownShowHiddenMessage);
		bool isDownShowHiddenMessage;
		//reg->Load(L"DownShowExOnTopMessage", isDownShowExOnTopMessage);
		bool isDownShowExOnTopMessage;
		//reg->Load(L"AlwaysOnTop", isAlwaysOnTop);
		bool isAlwaysOnTop;
		//reg->Load(L"SnapToDesktopEdges", isSnapToDesktopEdges);
		bool isSnapToDesktopEdges;
		//reg->Load(L"ExtendUCharMap", isExtendUCharMap);
		bool isExtendUCharMap;
		//reg->Load(L"DisableMouse", isDisableMouse);
		bool isDisableMouse;
		//reg->Load(L"MouseSkipActivation", isMouseSkipActivation);
		bool isMouseSkipActivation;
		//reg->Load(L"MouseSkipMoving", isMouseSkipMoving);
		bool isMouseSkipMoving;
		//reg->Load(L"MouseDragWindow", isMouseDragWindow);
		bool isMouseDragWindow;
		//reg->Load(L"FarHourglass", isFarHourglass);
		bool isFarHourglass;
		bool isFarWheelScrollback; // Handle hidden-panel wheel in the GUI, without a Far macro
		//reg->Load(L"FarHourglassDelay", nFarHourglassDelay);
		UINT nFarHourglassDelay;
		//reg->Load(L"DisableFarFlashing", isDisableFarFlashing); if (isDisableFarFlashing>2) isDisableFarFlashing = 2;
		BYTE isDisableFarFlashing;
		//reg->Load(L"DisableAllFlashing", isDisableAllFlashing); if (isDisableAllFlashing>2) isDisableAllFlashing = 2;
		BYTE isDisableAllFlashing;
		/* *** Text selection *** */
		//reg->Load(L"CTSIntelligent", isCTSIntelligent);
		bool isCTSIntelligent;
		private:
		//reg->Load(L"CTSIntelligentExceptions", &pszCTSIntelligentExceptions);
		wchar_t* pszCTSIntelligentExceptions; // Don't use IntelliSel in these app-processes
		public:
		// Service functions
		CEStr GetIntelligentExceptions(); // "|" delimited
		const wchar_t* GetIntelligentExceptionsMSZ(); // "\0" delimited
		void SetIntelligentExceptions(const wchar_t* apszApps); // "|" delimited
		//reg->Load(L"CTS.AutoCopy", isCTSAutoCopy);
		bool isCTSAutoCopy;
		//reg->Load(L"CTS.ResetOnRelease", isCTSResetOnRelease);
		bool isCTSResetOnRelease;
		//reg->Load(L"CTS.IBeam", isCTSIBeam);
		bool isCTSIBeam;
		//reg->Load(L"CTS.EndOnTyping", isCTSEndOnTyping);
		BYTE isCTSEndOnTyping; // 0 - off, 1 - copy & reset, 2 - reset only
		//reg->Load(L"CTS.EndOnKeyPress", isCTSEndOnKeyPress);
		bool isCTSEndOnKeyPress; // +isCTSEndOnTyping. +все, что не генерит WM_CHAR (стрелки и пр.)
		//reg->Load(L"CTS.EraseBeforeReset", isCTSEraseBeforeReset);
		bool isCTSEraseBeforeReset;
		//reg->Load(L"CTS.Freeze", isCTSFreezeBeforeSelect);
		bool isCTSFreezeBeforeSelect;
		//reg->Load(L"CTS.SelectBlock", isCTSSelectBlock);
		bool isCTSSelectBlock;
		//reg->Load(L"CTS.SelectText", isCTSSelectText);
		bool isCTSSelectText;
		//reg->Load(L"CTS.HtmlFormat", isCTSHtmlFormat);
		BYTE isCTSHtmlFormat; // 0 - Plain text only, 1 - HTML formatting, 2 - Copy as HTML, 3 - ANSI sequences
		//reg->Load(L"CTS.ForceLocale", isCTSForceLocale);
		DWORD isCTSForceLocale; // Try to bypass clipboard locale problems (pasting to old non-unicode apps)

		//reg->Load(L"CTS.RBtnAction", isCTSRBtnAction);
		MouseButtonAction isCTSRBtnAction;
		//reg->Load(L"CTS.MBtnAction", isCTSMBtnAction);
		MouseButtonAction isCTSMBtnAction;
		//reg->Load(L"CTS.ColorIndex", isCTSColorIndex);
		BYTE isCTSColorIndex;
		//reg->Load(L"ClipboardConfirmEnter", isPasteConfirmEnter);
		bool isPasteConfirmEnter;
		//reg->Load(L"AutoTrimSingleLine", isAutoTrimSingleLine);
		bool isAutoTrimSingleLine;
		//reg->Load(L"ClipboardConfirmLonger", nPasteConfirmLonger);
		UINT nPasteConfirmLonger;
		//reg->Load(L"FarGotoEditorOpt", isFarGotoEditor);
		bool isFarGotoEditor; // Подсвечивать и переходить на файл/строку (ошибки компилятора)
		//reg->Load(L"FarGotoEditorVk", isFarGotoEditorVk);
		//BYTE isFarGotoEditorVk; // Клавиша-модификатор для isFarGotoEditor
		//reg->Load(L"FarGotoEditorPath", &sFarGotoEditor);
		wchar_t* sFarGotoEditor; // Команда запуска редактора
		//reg->Load(L"HighlightMouseRow", isHighlightMouseRow);
		bool isHighlightMouseRow;
		//reg->Load(L"HighlightMouseCol", isHighlightMouseCol);
		bool isHighlightMouseCol;

		bool IsModifierPressed(int nDescrID, bool bAllowEmpty);
		void IsModifierPressed(int nDescrID, bool* pbNoEmpty, bool* pbAllowEmpty);

		//reg->Load(L"KeyboardHooks", m_isKeyboardHooks); if (m_isKeyboardHooks>2) m_isKeyboardHooks = 0;
		BYTE m_isKeyboardHooks;
	public:
		bool isKeyboardHooks(bool abNoDisable = false, bool abNoDbgCheck = false);

		//bool CheckUpdatesWanted();

		//reg->Load(L"PartBrush75", isPartBrush75); if (isPartBrush75<5) isPartBrush75=5; else if (isPartBrush75>250) isPartBrush75=250;
		BYTE isPartBrush75;
		//reg->Load(L"PartBrush50", isPartBrush50); if (isPartBrush50<5) isPartBrush50=5; else if (isPartBrush50>250) isPartBrush50=250;
		BYTE isPartBrush50;
		//reg->Load(L"PartBrush25", isPartBrush25); if (isPartBrush25<5) isPartBrush25=5; else if (isPartBrush25>250) isPartBrush25=250;
		BYTE isPartBrush25;
		//reg->Load(L"PartBrushBlack", isPartBrushBlack);
		BYTE isPartBrushBlack;

		//reg->Load(L"RightClick opens context menu", isRClickSendKey);
		// 0 - не звать EMenu, 1 - звать всегда, 2 - звать по длинному клику
		char isRClickSendKey;
		//Для тачскринов - удобнее по длинному тапу показывать меню,
		// а по двойному (Press and tap) выполнять выделение файлов
		// Поэтому, если isRClickTouch, то "длинный"/"короткий" клик инвертируется
		// --> isRClickSendKey==1 - звать всегда (isRClickTouchInvert не влияет)
		// --> isRClickSendKey==2 - звать по длинному тапу (аналог простого RClick)
		// При этом, PressAndTap всегда посылает RClick в консоль (для выделения файлов).
		bool isRClickTouchInvert();
		//reg->Load(L"RightClickMacro2", &sRClickMacro);
		wchar_t *sRClickMacro;
		LPCWSTR RClickMacro(FarMacroVersion fmv);
		LPCWSTR RClickMacroDefault(FarMacroVersion fmv);

		//reg->Load(L"SafeFarClose", isSafeFarClose);
		bool isSafeFarClose;
		//reg->Load(L"SafeFarCloseMacro", &sSafeFarCloseMacro);
		wchar_t *sSafeFarCloseMacro;
		LPCWSTR SafeFarCloseMacro(FarMacroVersion fmv);
		LPCWSTR SafeFarCloseMacroDefault(FarMacroVersion fmv);

		////reg->Load(L"AltEnter", isSendAltEnter);
		//bool isSendAltEnter;
		////reg->Load(L"AltSpace", isSendAltSpace);
		//bool isSendAltSpace;
		//reg->Load(L"SendAltTab", isSendAltTab);
		bool isSendAltTab;
		//reg->Load(L"SendAltEsc", isSendAltEsc);
		bool isSendAltEsc;
		//reg->Load(L"SendAltPrintScrn", isSendAltPrintScrn);
		bool isSendAltPrintScrn;
		//reg->Load(L"SendPrintScrn", isSendPrintScrn);
		bool isSendPrintScrn;
		//reg->Load(L"SendCtrlEsc", isSendCtrlEsc);
		bool isSendCtrlEsc;
		////reg->Load(L"SendAltF9", isSendAltF9);
		//bool isSendAltF9;

		//reg->Load(L"Min2Tray", mb_MinToTray);
		public:
		bool mb_MinToTray;
		bool isMinToTray(bool bRawOnly = false);
		void SetMinToTray(bool bMinToTray);
		//reg->Load(L"AlwaysShowTrayIcon", mb_AlwaysShowTrayIcon);
		bool mb_AlwaysShowTrayIcon;
		bool isAlwaysShowTrayIcon();
		//bool isForceMonospace, isProportional;
		//reg->Load(L"Monospace", isMonospace)
		BYTE isMonospace; // 0 - proportional, 1 - monospace, 2 - forcemonospace
		//bool isUpdConHandle;
		//reg->Load(L"RSelectionFix", isRSelFix);
		bool isRSelFix;

		/* *** Drag *** */
		//reg->Load(L"Dnd", isDragEnabled);
		BYTE isDragEnabled;
		//reg->Load(L"DndDrop", isDropEnabled);
		BYTE isDropEnabled;
		//reg->Load(L"DndLKey", nLDragKey);
		//BYTE nLDragKey;
		//reg->Load(L"DndRKey", nRDragKey);
		//BYTE nRDragKey; // Был DWORD
		//reg->Load(L"DefCopy", isDefCopy);
		bool isDefCopy;
		//reg->Load(L"DropUseMenu", isDropUseMenu);
		BYTE isDropUseMenu;
		//reg->Load(L"DragOverlay", isDragOverlay);
		bool isDragOverlay;
		//reg->Load(L"DragShowIcons", isDragShowIcons);
		bool isDragShowIcons;
		//reg->Load(L"DragPanel", isDragPanel); if (isDragPanel > 2) isDragPanel = 1;
		BYTE isDragPanel; // изменение размера панелей мышкой
		//reg->Load(L"DragPanelBothEdges", isDragPanelBothEdges);
		bool isDragPanelBothEdges; // таскать за обе рамки (правую-левой панели и левую-правой панели)

		//reg->Load(L"KeyBarRClick", isKeyBarRClick);
		bool isKeyBarRClick; // Правый клик по кейбару - показать PopupMenu

		//reg->Load(L"DebugSteps", isDebugSteps);
		bool isDebugSteps;

		//reg->Load(L"DebugLog", isDebugSteps);
		BYTE isDebugLog;
		// Helpers
		bool mb_DisableLogging;
		unsigned isLogging(unsigned level = 1);
		void EnableLogging();
		void DisableLogging();

		//reg->Load(L"EnhanceGraphics", isEnhanceGraphics);
		bool isEnhanceGraphics; // Progressbars and scrollbars (pseudographics)
		//reg->Load(L"EnhanceButtons", isEnhanceButtons);
		bool isEnhanceButtons; // Buttons, CheckBoxes and RadioButtons (pseudographics)

		//reg->Load(L"FadeInactive", isFadeInactive);
		bool isFadeInactive;
		protected:
		//reg->Load(L"FadeInactiveLow", mn_FadeLow);
		BYTE mn_FadeLow;
		//reg->Load(L"FadeInactiveHigh", mn_FadeHigh);
		BYTE mn_FadeHigh;
		//mn_LastFadeSrc = mn_LastFadeDst = -1;
		COLORREF mn_LastFadeSrc;
		//mn_LastFadeSrc = mn_LastFadeDst = -1;
		COLORREF mn_LastFadeDst;
		public:

		//reg->Load(L"StatusBar.Show", isStatusBarShow);
		bool isStatusBarShow;
		//reg->Load(L"StatusBar.Flags", isStatusBarFlags);
		DWORD isStatusBarFlags; // set of CEStatusFlags
		//reg->Load(L"StatusFontFace", sStatusFontFace, countof(sStatusFontFace));
		wchar_t sStatusFontFace[LF_FACESIZE];
		//reg->Load(L"StatusFontCharSet", nStatusFontCharSet);
		UINT nStatusFontCharSet;
		//reg->Load(L"StatusFontHeight", nStatusFontHeight);
		int nStatusFontHeight;
		int StatusBarFontHeight(); // { return std::max(4,nStatusFontHeight); };
		int StatusBarHeight(); // { return StatusBarFontHeight() + ((isStatusBarFlags & csf_NoVerticalPad) ? ((isStatusBarFlags & csf_HorzDelim) ? 1 : 0) : 2); };
		//reg->Load(L"StatusBar.Color.Back", nStatusBarBack);
		DWORD nStatusBarBack;
		//reg->Load(L"StatusBar.Color.Light", nStatusBarLight);
		DWORD nStatusBarLight;
		//reg->Load(L"StatusBar.Color.Dark", nStatusBarDark);
		DWORD nStatusBarDark;
		//reg->Load(L"StatusBar.HideColumns", nHideStatusColumns);
		bool isStatusColumnHidden[64]; // _ASSERTE(countof(isStatusColumnHidden)>csi_Last);
		//для информации, чтобы сохранить изменения при выходе
		bool mb_StatusSettingsWasChanged;

		//reg->Load(L"Tabs", isTabs);
		char isTabs; // 0 - don't show, 1 - always show, 2 - auto show
		//reg->Load(L"TabsLocation", nTabsLocation);
		BYTE nTabsLocation; // 0 - top, 1 - bottom
		//reg->Load(L"TabIcons", isTabIcons);
		bool isTabIcons;
		//reg->Load(L"OneTabPerGroup", isOneTabPerGroup);
		bool isOneTabPerGroup;
		//reg->Load(L"ActivateSplitMouseOver", bActivateSplitMouseOver);
		BYTE bActivateSplitMouseOver;
		bool isActivateSplitMouseOver();
		//reg->Load(L"TabSelf", isTabSelf);
		bool isTabSelf;
		//reg->Load(L"TabRecent", isTabRecent);
		bool isTabRecent;
		//reg->Load(L"TabLazy", isTabLazy);
		bool isTabLazy;
		//reg->Load(L"TabFlashChanged", nTabFlashChanged);
		int nTabFlashChanged;

		//reg->Load(L"TabDblClick", nTabDblClickAction);
		UINT nTabBarDblClickAction; // enum of TabBarDblClick : TabBarDblClickActions[]
		UINT nTabBtnDblClickAction; // enum of TabBtnDblClick : TabBtnDblClickActions[]

		// Tab theme properties
		//int ilDragHeight; = 10

		protected:
		//reg->Load(L"TabsOnTaskBar", m_isTabsOnTaskBar);
		BYTE m_isTabsOnTaskBar; // 0 - ConEmu only, 1 - all tabs & all OS, 2 - all tabs & Win 7, 3 - DON'T SHOW
		public:
		enum TabsOnTaskbar { tot_ConEmuOnly = 0, tot_AllTabsAllOS = 1, tot_AllTabsWin7 = 2, tot_DontShow = 3};
		TabsOnTaskbar GetRawTabsOnTaskBar() { return (TabsOnTaskbar)m_isTabsOnTaskBar; };
		bool isTabsOnTaskBar();
		bool isWindowOnTaskBar(bool bStrictOnly = false);
		//void SetTabsOnTaskBar(BYTE nTabsOnTaskBar);
		bool isTaskbarOverlay;
		bool isTaskbarProgress;

		//reg->Load(L"TabFontFace", sTabFontFace, countof(sTabFontFace));
		wchar_t sTabFontFace[LF_FACESIZE];
		//reg->Load(L"TabFontCharSet", nTabFontCharSet);
		UINT nTabFontCharSet;
		//reg->Load(L"TabFontHeight", nTabFontHeight);
		int nTabFontHeight;

		//if (!reg->Load(L"TabCloseMacro", &sTabCloseMacro) || (sTabCloseMacro && !*sTabCloseMacro)) { if (sTabCloseMacro) { free(sTabCloseMacro); sTabCloseMacro = nullptr; } }
		wchar_t *sTabCloseMacro;
		LPCWSTR TabCloseMacro(FarMacroVersion fmv);
		LPCWSTR TabCloseMacroDefault(FarMacroVersion fmv);

		//if (!reg->Load(L"SaveAllEditors", &sSaveAllMacro)) { sSaveAllMacro = lstrdup(L"...
		wchar_t *sSaveAllMacro;
		LPCWSTR SaveAllMacro(FarMacroVersion fmv);
		LPCWSTR SaveAllMacroDefault(FarMacroVersion fmv);

		//reg->Load(L"ToolbarAddSpace", nToolbarAddSpace);
		int nToolbarAddSpace;
		//reg->Load(L"ConWnd Width", wndWidth);
		CESize wndWidth;
		//reg->Load(L"ConWnd Height", wndHeight);
		CESize wndHeight;
		//reg->Load(L"16bit Height", ntvdmHeight);
		UINT ntvdmHeight; // в символах
		//reg->Load(L"ConWnd X", wndX);
		int _wndX; // в пикселях
		//reg->Load(L"ConWnd Y", wndY);
		int _wndY; // в пикселях
		// Monitor information
		RECT LastMonRect;
		//reg->Load(L"WindowMode", WindowMode); if (WindowMode!=wmFullScreen && WindowMode!=wmMaximized && WindowMode!=wmNormal) WindowMode = wmNormal;
		DWORD _WindowMode;
		//reg->Load(L"Cascaded", wndCascade);
		bool wndCascade;
		//reg->Load(L"AutoSaveSizePos", isAutoSaveSizePos);
		bool isAutoSaveSizePos;
		//reg->Load(L"UseCurrentSizePos", isUseCurrentSizePos);
		bool isUseCurrentSizePos; // Show in settings dialog and save current window size/pos

		bool isIntegralSize() const;

	private:
		// При закрытии окна крестиком - сохранять только один раз,
		// а то размер может в процессе закрытия консолей измениться
		bool mb_ExitSettingsAutoSaved;
	public:
		//reg->Load(L"SlideShowElapse", nSlideShowElapse);
		UINT nSlideShowElapse;
		//reg->Load(L"IconID", nIconID);
		UINT nIconID;
		//reg->Load(L"TryToCenter", isTryToCenter);
		bool isTryToCenter;
		//reg->Load(L"CenterConsolePad", nCenterConsolePad);
		UINT nCenterConsolePad;
		//reg->Load(L"ShowScrollbar", isAlwaysShowScrollbar); if (isAlwaysShowScrollbar > 2) isAlwaysShowScrollbar = 2;
		BYTE isAlwaysShowScrollbar; // 0-не показывать, 1-всегда, 2-автоматически (на откусывает место от консоли)
		//reg->Load(L"ScrollBarAppearDelay", nScrollBarAppearDelay);
		UINT nScrollBarAppearDelay;
		//reg->Load(L"ScrollBarDisappearDelay", nScrollBarDisappearDelay);
		UINT nScrollBarDisappearDelay;

		////reg->Load(L"TabMargins", rcTabMargins);
		//RECT rcTabMargins;
		////reg->Load(L"TabFrame", isTabFrame);
		//bool isTabFrame;

		//reg->Load(L"MinimizeRestore", vmMinimizeRestore);
		//DWORD vmMinimizeRestore;
		//reg->Load(L"SingleInstance", isSingleInstance);
		bool isSingleInstance;
		//reg->Load(L"ShowHelpTooltips", isShowHelpTooltips);
		bool isShowHelpTooltips;
		//reg->Load(L"Multi", mb_isMulti);
		bool mb_isMulti;
		//reg->Load(L"Multi.ShowButtons", isMultiShowButtons);
		bool isMultiShowButtons;
		//reg->Load(L"Multi.ShowSearch", isMultiShowSearch);
		bool isMultiShowSearch;
		//reg->Load(L"NumberInCaption", isNumberInCaption);
		bool isNumberInCaption;
		private:
		//reg->Load(L"Multi.Modifier", nHostkeyModifier); TestHostkeyModifiers();
		DWORD nHostkeyNumberModifier; // Используется для 0..9, WinSize
		//reg->Load(L"Multi.ArrowsModifier", nHostkeyArrowModifier); TestHostkeyModifiers();
		DWORD nHostkeyArrowModifier; // Используется для WinSize
		public:
		// Max - 3 keys, so lower 3 bytes only
		DWORD HostkeyNumberModifier() { return (nHostkeyNumberModifier & 0xFFFFFF); };
		DWORD HostkeyArrowModifier()  { return (nHostkeyArrowModifier  & 0xFFFFFF); };
		//
		public:
		//reg->Load(L"Multi.NewConsole", vmMultiNew);
		//DWORD vmMultiNew;
		//reg->Load(L"Multi.NewConsoleShift", vmMultiNewShift);
		//DWORD vmMultiNewShift; // Default - vmMultiNew+Shift
		//reg->Load(L"Multi.Next", vmMultiNext);
		//DWORD vmMultiNext;
		//reg->Load(L"Multi.NextShift", vmMultiNextShift);
		//DWORD vmMultiNextShift;
		//reg->Load(L"Multi.Recreate", vmMultiRecreate);
		//DWORD vmMultiRecreate;
		//reg->Load(L"Multi.Buffer", vmMultiBuffer);
		//DWORD vmMultiBuffer;
		//reg->Load(L"Multi.Close", vmMultiClose);
		//DWORD vmMultiClose;
		//reg->Load(L"Multi.CloseConfirm", isCloseConsoleConfirm);
		enum CloseConfirmOptions {
			cc_None      = 0, // Don't confirm any close actions
			cc_Window    = 1, // Window close (cross clicking)
			cc_Console   = 2, // Tab close (from tab menu for example)
			cc_Running   = 4, // When running process was detected
			cc_FarEV     = 8, // was isCloseEditViewConfirm
		};
		BYTE nCloseConfirmFlags; // CloseConfirmOptions
		//reg->Load(L"ResetTerminalConfirm", isResetTerminalConfirm);
		bool isResetTerminalConfirm;
		//reg->Load(L"Multi.CmdKey", vmMultiCmd);
		//DWORD vmMultiCmd;
		//reg->Load(L"Multi.AutoCreate", isMultiAutoCreate);
		bool isMultiAutoCreate;
		//reg->Load(L"Multi.LeaveOnClose", isMultiLeaveOnClose);
		BYTE isMultiLeaveOnClose; // 0 - закрываться, 1 - оставаться, 2 - НЕ оставаться при закрытии "крестиком"
		//reg->Load(L"Multi.HideOnClose", isMultiHideOnClose);
		BYTE isMultiHideOnClose; // 0 - не скрываться, 1 - в трей, 2 - просто минимизация
		// helpers
		bool isCloseOnLastTabClose();
		bool isCloseOnLastTabClose(BYTE MultiLeaveOnClose);
		bool isCloseOnCrossClick();
		bool isMinOnLastTabClose();
		bool isHideOnLastTabClose();
		//reg->Load(L"Multi.MinByEsc", isMultiMinByEsc);
		enum MultiMinByEsc { mbe_Never = 0, mbe_Always = 1, mbe_NoConsoles = 2 };
		BYTE isMultiMinByEsc; // 0 - Never, 1 - Always, 2 - NoConsoles
		//reg->Load(L"MapShiftEscToEsc", isMapShiftEscToEsc);
		bool isMapShiftEscToEsc; // used only when isMultiMinByEsc==1 and only for console apps
		//reg->Load(L"Multi.Iterate", isMultiIterate);
		bool isMultiIterate;
		//reg->Load(L"Multi.NewConfirm", isMultiNewConfirm);
		bool isMultiNewConfirm;
		//reg->Load(L"Multi.DupConfirm", isMultiDupConfirm);
		bool isMultiDupConfirm;
		//reg->Load(L"Multi.DetachConfirm", isMultiDetachConfirm);
		bool isMultiDetachConfirm;
		//reg->Load(L"Multi.UseNumbers", isUseWinNumber);
		bool isUseWinNumber;
		//reg->Load(L"Multi.UseWinTab", isUseWinTab);
		bool isUseWinTab;
		//reg->Load(L"Multi.UseArrows", isUseWinArrows);
		bool isUseWinArrows;
		//reg->Load(L"Multi.SplitWidth", nSplitWidth);
		BYTE nSplitWidth;
		//reg->Load(L"Multi.SplitHeight", nSplitHeight);
		BYTE nSplitHeight;
		////reg->Load(L"Multi.SplitClr1", nSplitClr1);
		//DWORD nSplitClr1;
		////reg->Load(L"Multi.SplitClr2", nSplitClr2);
		//DWORD nSplitClr2;
		//reg->Load(L"FARuseASCIIsort", isFARuseASCIIsort);
		bool isFARuseASCIIsort;
		//reg->Load(L"FixAltOnAltTab", isFixAltOnAltTab);
		bool isFixAltOnAltTab;
		//"UseAltGrayPlus"
		bool isUseAltGrayPlus;
		//reg->Load(L"ShellNoZoneCheck", isShellNoZoneCheck);
		bool isShellNoZoneCheck;

		// FindText: bMatchCase, bMatchWholeWords, bFreezeConsole, bHighlightAll;
		// FindOptions.pszText may be used to pre-fill search dialog field if search-bar is hidden
		FindTextOptions FindOptions;


	public:
		/* ************************ */
		/* ************************ */
		/* *** Hotkeys/Hostkeys *** */
		/* ************************ */
		/* ************************ */

		// VkMod = LOBYTE - VK, старшие три байта - модификаторы (тоже VK)

		// Вернуть заданный VkMod, или 0 если не задан. nDescrID = vkXXX (e.g. vkMinimizeRestore)
		DWORD GetHotkeyById(int nDescrID, const ConEmuHotKey** ppHK = nullptr);
		// Return hotkeyname by ID
		LPCWSTR GetHotkeyNameById(int nDescrID, wchar_t (&szFull)[128], bool bShowNone = true);
		// Проверить, задан ли этот hotkey. nDescrID = vkXXX (e.g. vkMinimizeRestore)
		bool IsHotkey(int nDescrID);
		// Установить новый hotkey. nDescrID = vkXXX (e.g. vkMinimizeRestore).
		void SetHotkeyById(int nDescrID, DWORD VkMod);
		// Проверить, есть ли хоткеи с назначенным одиночным модификатором
		bool isModifierExist(BYTE Mod/*VK*/, bool abStrictSingle = false);
		// Есть ли такой хоткей или модификатор (актуально для VK_APPS)
		bool isKeyOrModifierExist(BYTE Mod/*VK*/);
		// Проверить на дубли
		void CheckHotkeyUnique();
	private:
		void LoadHotkeys(SettingsBase* reg, const bool& bSendAltEnter, const bool& bSendAltSpace, const bool& bSendAltF9);
		void SaveHotkeys(SettingsBase* reg, int SaveDescrLangID = 0);
	public:

		/* *** Tab Templates *** */
		//reg->Load(L"TabConsole", szTabConsole, countof(szTabConsole));
		WCHAR szTabConsole[32];
		//reg->Load(L"TabModifiedSuffix", szTabModifiedSuffix, countof(szTabModifiedSuffix));
		WCHAR szTabModifiedSuffix[16];
		//reg->Load(L"TabSkipWords", &pszTabSkipWords);
		wchar_t* pszTabSkipWords;
		//reg->Load(L"TabPanels", szTabPanels, countof(szTabPanels));
		WCHAR szTabPanels[32];
		//reg->Load(L"TabEditor", szTabEditor, countof(szTabEditor));
		WCHAR szTabEditor[32];
		//reg->Load(L"TabEditorModified", szTabEditorModified, countof(szTabEditorModified));
		WCHAR szTabEditorModified[32];
		//reg->Load(L"TabViewer", szTabViewer, countof(szTabViewer));
		WCHAR szTabViewer[32];
		//reg->Load(L"TabLenMax", nTabLenMax); if (nTabLenMax < 10 || nTabLenMax >= CONEMUTABMAX) nTabLenMax = 20;
		UINT nTabLenMax;
		//todo
		UINT nTabWidthMax;
		TabStyle nTabStyle; // enum

		//reg->Load(L"AdminTitleSuffix", szAdminTitleSuffix, countof(szAdminTitleSuffix)); szAdminTitleSuffix[countof(szAdminTitleSuffix)-1] = 0;
		wchar_t szAdminTitleSuffix[64]; // DefaultAdminTitleSuffix /* " (Admin)" */
		//reg->Load(L"AdminShowShield", bAdminShield);
		BYTE bAdminShield; // enum AdminTabStyle
		// Old style:
		// * Disabled: bAdminShield = false, szAdminTitleSuffix = ""
		// * Shield:   bAdminShield = true,  szAdminTitleSuffix ignored (may be filled!)
		// * Suffix:   bAdminShield = false, szAdminTitleSuffix = " (Admin)"
		// New style:
		// * Disabled: bAdminShield = 0,  szAdminTitleSuffix = ""
		// * Shield:   bAdminShield = 1,  szAdminTitleSuffix ignored (may be filled!)
		// * Suffix:   bAdminShield = 0,  szAdminTitleSuffix = " (Admin)"
		// * Shld+Suf: bAdminShield = 3,  szAdminTitleSuffix = " (Admin)"
		bool isAdminShield();
		bool isAdminSuffix();
		//reg->Load(L"HideInactiveConsoleTabs", bHideInactiveConsoleTabs);
		bool bHideInactiveConsoleTabs;

		// L"HideDisabledTabs" -- is not saved
		bool isHideDisabledTabs() { return false; };

		//reg->Load(L"ShowFarWindows", bShowFarWindows);
		bool bShowFarWindows;

		bool NeedCreateAppWindow();

		//reg->Load(L"MainTimerElapse", nMainTimerElapse); if (nMainTimerElapse>1000) nMainTimerElapse = 1000;
		UINT nMainTimerElapse; // периодичность, с которой из консоли считывается текст
		//reg->Load(L"MainTimerInactiveElapse", nMainTimerInactiveElapse); if (nMainTimerInactiveElapse>10000) nMainTimerInactiveElapse = 10000;
		UINT nMainTimerInactiveElapse; // периодичность при неактивности

		//bool isAdvLangChange; // в Висте без ConIme в самой консоли не меняется язык, пока не послать WM_SETFOCUS. Но при этом исчезает диалог быстрого поиска

		//reg->Load(L"SkipFocusEvents", isSkipFocusEvents);
		bool isSkipFocusEvents;

		//bool isLangChangeWsPlugin;

		//reg->Load(L"MonitorConsoleLang", isMonitorConsoleLang);
		BYTE isMonitorConsoleLang; // bitmask. 1 - follow up console HKL (e.g. after XLat in Far Manager), 2 - use one HKL for all tabs

		//reg->Load(L"SleepInBackground", isSleepInBackground);
		bool isSleepInBackground;
		//reg->Load(L"RetardInactivePanes", isRetardInactivePanes);
		bool isRetardInactivePanes;

		//reg->Load(L"MinimizeOnLoseFocus", mb_MinimizeOnLoseFocus);
		bool mb_MinimizeOnLoseFocus;

		//reg->Load(L"AffinityMask", nAffinity);
		DWORD nAffinity;

		//reg->Load(L"UseInjects", isUseInjects);
		bool isUseInjects; // 0 - off, 1 - always. Note, Root process is infiltrated always.

		//reg->Load(L"ProcessAnsi", isProcessAnsi);
		bool isProcessAnsi; // ANSI X3.64 & XTerm-256-colors Support
		// "AnsiExecution"
		BYTE isAnsiExec; // enum AnsiExecutionPerm
		// "AnsiAllowedCommands"
		CEStr* psAnsiAllowed; // commands: multiline, "\r\n" separated

		//reg->Load(L"AnsiLog", isAnsiLog);
		bool isAnsiLog; // Limited logging of console contents (same output as processed by ConEmu::ConsoleFlags::ProcessAnsi)
		//reg->Load(L"AnsiLogCodes", isAnsiLogCodes);
		bool isAnsiLogCodes; // Write to logfile ANSI sequences (from app and our internals)
		//reg->Load(L"AnsiLogPath", &pszAnsiLog);
		wchar_t* pszAnsiLog;
		//reg->Load(L"KillSshAgent", isKillSshAgent)
		bool isKillSshAgent; // Terminate all `ssh-agent.exe` processes started from ConEmu consoles on ConEmu termination.
		//reg->Load(L"ProcessNewConArg", isProcessNewConArg)
		bool isProcessNewConArg; // Enable processing of '-new_console' and '-cur_console' switches in your shell prompt, scripts etc. started in ConEmu tabs
		//reg->Load(L"ProcessCmdStart", isProcessCmdStart)
		bool isProcessCmdStart; // Use "start xxx.exe" to start new tab
		//reg->Load(L"ProcessCtrlZ", isProcessCtrlZ)
		bool isProcessCtrlZ; // Treat Ctrl-Z as ‘EndOfStream’. On new line press Ctrl-Z and Enter. Refer to the gh#465 for details (Go input streams).
		//reg->Load(L"UseClink", mb_UseClink);
		bool mb_UseClink; // использовать расширение командной строки (ReadConsole)
		DWORD isUseClink();
		//reg->Load(L"PortableReg", isPortableReg);
		bool isPortableReg;
		//reg->Load(L"SuppressBells", isSuppressBells);
		bool isSuppressBells;
		//reg->Load(L"ConsoleExceptionHandler", isConsoleExceptionHandler);
		bool isConsoleExceptionHandler; //

		/* *** Debugging *** */
		//reg->Load(L"ConVisible", isConVisible);
		bool isConVisible;

		/* *** Thumbnails and Tiles *** */
		//reg->Load(L"PanView.BackColor", ThSet.crBackground.RawColor);
		//reg->Load(L"PanView.PFrame", ThSet.nPreviewFrame); if (ThSet.nPreviewFrame!=0 && ThSet.nPreviewFrame!=1) ThSet.nPreviewFrame = 1;
		//и т.п...
		PanelViewSetMapping ThSet;

		/* *** AutoUpdate *** */
		ConEmuUpdateSettings UpdSet;

		/* *** Notification *** */
		wchar_t StopBuzzingDate[16];


		/* *** HotKeys & GuiMacros *** */
		//reg->Load(L"GuiMacro<N>.Key", &Macros.vk);
		//reg->Load(L"GuiMacro<N>.Macro", &Macros.szGuiMacro);
		//struct HotGuiMacro
		//{
		//	union {
		//		BYTE vk;
		//		LPVOID dummy;
		//	};
		//	wchar_t* szGuiMacro;
		//};
		//HotGuiMacro Macros[24];

	public:
		void LoadSettings(bool& rbNeedCreateVanilla, const SettingsStorage* apStorage = nullptr);
		void InitSettings();
		void InitVanilla();
		void InitVanillaFontSettings();
		bool SaveVanilla(SettingsBase* reg);
		void LoadCmdTasks(SettingsBase* reg, bool abFromOpDlg = false);
		void LoadPalettes(SettingsBase* reg);
		void CreatePredefinedPalettes(int iAddUserCount);
		void LoadProgresses(SettingsBase* reg);
		BOOL SaveSettings(BOOL abSilent = FALSE, const SettingsStorage* apStorage = nullptr);
		void SaveAppsSettings(SettingsBase* reg);
		bool SaveCmdTasks(SettingsBase* reg);
		bool SaveProgresses(SettingsBase* reg);
		void SaveConsoleFont();
		void SaveFindOptions(SettingsBase* reg = nullptr);
		void OnAutoSaveTimer();
		void AutoSaveSettings(SettingsBase* reg = nullptr, bool saveAll = false);
		bool IsAutoSaveSettings(bool saveAll);
		void SaveSettingsOnExit();
		void SaveStopBuzzingDate();
		//void UpdateMargins(RECT arcMargins);
	public:
		void CheckConsoleSettings();
		void ResetSavedOnExit();

		SettingsBase* CreateSettings(const SettingsStorage* apStorage);
		CEStr GetStoragePlaceDescr(const SettingsStorage* apStorage, LPCWSTR asPrefix);

		SettingsStorage GetSettingsType();
};
