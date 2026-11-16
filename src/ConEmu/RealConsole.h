
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
#include <atomic>
#include "../common/ConsoleAnnotation.h"
#include "../common/ConPtyData.h"
#include "../common/RgnDetect.h"
#include "../common/MArray.h"
#include "../common/MEvent.h"
#include "../common/MMap.h"
#include "../common/MPipe.h"
#include "../common/MSection.h"
#include "../common/MFileMapping.h"
#include "../common/RConStartArgsEx.h"

#define DEFINE_EXIT_DESC
#include "../ConEmuCD/ExitCodes.h"

#define SB_HALFPAGEUP   32
#define SB_HALFPAGEDOWN 33
#define SB_GOTOCURSOR   34
#define SB_PROMPTUP     35
#define SB_PROMPTDOWN   36

#define CES_CMDACTIVE      0x01
#define CES_TELNETACTIVE   0x02
#define CES_FARACTIVE      0x04
#define CES_FARINSTACK     0x08
//#define CES_CONALTERNATIVE 0x08
//#define CES_PROGRAMS (0x0F)

//#define CES_NTVDM 0x10 -- Common.h
//#define CES_PROGRAMS2 0xFF

#define CES_FILEPANEL      0x0001
#define CES_FARPANELS      0x000F // на будущее, должен содержать все возможные флаги возможных панелей
//#define CES_TEMPPANEL      0x0002
//#define CES_PLUGINPANEL    0x0004
#define CES_EDITOR         0x0010
#define CES_VIEWER         0x0020
#define CES_COPYING        0x0040
#define CES_MOVING         0x0080
#define CES_NOTPANELFLAGS  0xFFF0
#define CES_FARFLAGS       0xFFFF
#define CES_MAYBEPANEL   0x010000
#define CES_WASPROGRESS  0x020000
#define CES_OPER_ERROR   0x040000
//... and so on


#define CONSOLE_BLOCK_SELECTION 0x0100 // selecting text (standard mode)
#define CONSOLE_TEXT_SELECTION 0x0200 // selecting text (stream mode)
#define CONSOLE_DBLCLICK_SELECTION 0x0400 // word was selected by double click, skip WM_LBUTTONUP
#define CONSOLE_LEFT_ANCHOR 0x0800 // If selection was started rightward
#define CONSOLE_RIGHT_ANCHOR 0x1000 // If selection was started leftward
#define CONSOLE_EXPANDED 0x2000 // Support Shift+LClick & Shift+LClick to mark start and end of the selection
#define CONSOLE_TRIPLE_CLICK_SELECTION 0x4000 // line(s) was selected by triple click, skip WM_LBUTTONUP
#define CONSOLE_KEYMOD_MASK 0xFF000000 // Здесь хранится модификатор, которым начали выделение мышкой

#define PROCESS_WAIT_START_TIME RELEASEDEBUGTEST(1000,1000)

#define SETSYNCSIZEAPPLYTIMEOUT 500
#define SETSYNCSIZEMAPPINGTIMEOUT 300
#define CONSOLEPROGRESSTIMEOUT 1500
#define CONSOLEPROGRESSWARNTIMEOUT 2000 // поставил 2с, т.к. при минимизации консоль обновляется раз в секунду
#define CONSOLEINACTIVERGNTIMEOUT 500
#define SERVERCLOSETIMEOUT 2000
#define UPDATESERVERACTIVETIMEOUT 2500

struct ConProcess
{
	DWORD ProcessID, ParentPID; //, InputTID;
	bool  IsMainSrv; // Root ConEmuC
	bool  IsTermSrv; // cygwin/msys connector
	bool  IsConHost; // conhost.exe (Win7 и выше)
	bool  IsFar, IsFarPlugin;
	bool  IsTelnet;  // может быть включен ВМЕСТЕ с IsFar, если удалось подцепиться к фару через сетевой пайп
	bool  IsNtvdm;   // 16bit приложения
	bool  IsCmd;     // значит фар выполняет команду
	bool  inConsole;
	int   Bits;
	wchar_t Name[64]; // чтобы полная инфа об ошибке влезала
};


class CVirtualConsole;
class CRgnDetect;
class CRealBuffer;
class CDpiForDialog;
class CDynDialog;
class MFileLog;
class CRConFiles;
class CAltNumpad;
class CRConPalette;
struct AppSettings;
struct TermX;

enum RealBufferType
{
	rbt_Undefined = 0,
	rbt_Primary,
	rbt_Alternative,
	rbt_Selection,
	rbt_Find,
	rbt_DumpScreen,
};

enum LoadAltMode
{
	lam_Default = 0,    // режим выбирается автоматически
	lam_LastOutput = 1, // имеет смысл только для фара и "Long console output"
	lam_FullBuffer = 2, // снимок экрана - полный буфер с прокруткой
	lam_Primary = 3,    // TODO: для быстрого начала выделения - копия буфера rbt_Primary
};

enum StartDebugType
{
	sdt_DumpMemory,
	sdt_DumpMemoryTree,
	sdt_DebugActiveProcess,
	sdt_DebugProcessTree,
};

enum RConStartState
{
	rss_NotStarted,
	rss_MonitorStarted,
	rss_StartupRequested,
	rss_StartingServer,
	rss_ServerStarted,
	rss_ServerConnected,
	rss_DataAquired,
	rss_ProcessActive,
};

enum ChangeTermAction
{
	cta_Disable = 0,
	cta_Enable = 1,
	cta_Switch = 2,
};

struct ConEmuHotKey;
struct ConsoleInfoArg;

#include "HotkeyChord.h"
#include "RealServer.h"
#include "SetTypes.h"
#include "TabID.h"

class CRealConsole
{
#ifdef _DEBUG
		friend class CVirtualConsole;
#endif
	protected:
		CVirtualConsole* mp_VCon = nullptr;
		CConEmuMain*     mp_ConEmu = nullptr;
		bool mb_ConstuctorFinished = false;

	public:

		unsigned TextWidth();
		unsigned TextHeight();
		unsigned BufferHeight(unsigned nNewBufferHeight=0);
		unsigned BufferWidth();
		void OnBufferHeight();

	private:
		HWND    hConWnd;
		BYTE    m_ConsoleKeyShortcuts = 0;
		BYTE    mn_TextColorIdx, mn_BackColorIdx, mn_PopTextColorIdx, mn_PopBackColorIdx;
		HKEY    PrepareConsoleRegistryKey(LPCWSTR asSubKey) const;
		void    PrepareDefaultColors(BYTE& nTextColorIdx, BYTE& nBackColorIdx, BYTE& nPopTextColorIdx, BYTE& nPopBackColorIdx, bool bUpdateRegistry = false, HKEY hkConsole = nullptr);
		void    PrepareNewConArgs();
	public:
		void    PrepareDefaultColors();
	private:
		// ChildGui related
		struct {
			HWND    hGuiWnd;            // ChildGui mode (Notepad, Putty, ...)
			RECT    rcLastGuiWnd;       // Screen coordinates!
			HWND    hGuiWndFocusStore;
			DWORD   nGuiAttachInputTID;
			DWORD   nGuiAttachFlags;    // Stored in SetGuiMode
			RECT    rcPreGuiWndRect;    // Window coordinates before attach into ConEmu
			bool    bGuiExternMode;     // FALSE user asked to show ChildGui outside of ConEmu, temporarily detached (Ctrl-Win-Alt-Space).
			bool    bGuiForceConView;   // TRUE if user asked to hide ChildGui and show our VirtualConsole (with current console contents).
			bool    bChildConAttached;  // TRUE if ChildGui started CONSOLE application (CommandPromptPortable.exe). Don't confuse with Putty/plink-proxy.
			bool    bInGuiAttaching;
			bool    bInSetFocus;
			bool    bCreateHidden;
			DWORD   nGuiWndStyle, nGuiWndStylEx; // Исходные стили окна ДО подцепления в ConEmu
			ConProcess Process;
			CESERVER_REQ_PORTABLESTARTED paf;
			// some helpers
			bool    isGuiWnd() { return (hGuiWnd && (hGuiWnd != (HWND)INVALID_HANDLE_VALUE)); };
		} m_ChildGui;
		void setGuiWndPID(HWND ahGuiWnd, DWORD anPID, int anBits, LPCWSTR asProcessName);
		void setGuiWnd(HWND ahGuiWnd);
		static  BOOL CALLBACK FindChildGuiWindowProc(HWND hwnd, LPARAM lParam);

	public:
		enum ConStatusOption
		{
			cso_Default             = 0x0000,
			cso_ResetOnConsoleReady = 0x0001,
			cso_DontUpdate          = 0x0002, // Не нужно обновлять статусную строку сразу
			cso_Critical            = 0x0004,
		};
		LPCWSTR GetConStatus();
		void SetConStatus(LPCWSTR asStatus, DWORD/*enum ConStatusOption*/ Options = cso_Default);
	private:
		struct {
			DWORD   Options = ConStatusOption::cso_Default; /*enum ConStatusOption*/
			wchar_t szText[80] = L"";
		} m_ConStatus;

		struct {
			SHORT nLastHeight = 0;
			SHORT nLastWndHeight = 0;
			SHORT nLastTop = 0;
		} m_ScrollStatus;

		struct {
			/// Fix of dblclick in the editor
			MOUSE_EVENT_RECORD rLastEvent = {};
			POINT ptLastMouseGuiPos = {}; // in pixels
			bool bBtnClicked = false;
			COORD crBtnClickPos = {-1, -1};
			/// Some mouse devices send wheel in much smaller portions than +120/-120
			int WheelDirAccum = 0;
			bool WheelAccumulated = false;
			/// If BtnDown event was sent to console
			bool bMouseButtonDown = false;
			COORD crLastMouseEventPos = {-1, -1};
			/// Far Manager: let touchscreen taps be easy
			bool bMouseTapChanged = false;
			COORD crMouseTapReal = {-1, -1};
			COORD crMouseTapChanged = {-1, -1};
			/// Useful to know when processing LBtnUp
			bool bWasMouseSelection = false;
		} m_Mouse;

	public:
		HWND    ConWnd();  // HWND RealConsole
		HWND    GetView(); // HWND отрисовки

		// Если работаем в Gui-режиме (Notepad, Putty, ...)
		HWND    GuiWnd();  // HWND Gui приложения
		DWORD   GuiWndPID();  // HWND Gui приложения
		bool    isGuiForceConView(); // mb_GuiForceConView
		bool    isGuiExternMode(); // bGuiExternMode
		bool    isGuiEagerFocus(); // ставить фокус в ChildGui при попадании оного в ConEmu
		void    GuiNotifyChildWindow();
		void    GuiWndFocusStore();
		void    GuiWndFocusRestore(bool bForce = false);
	private:
		void    GuiWndFocusThread(HWND hSetFocus, BOOL& bAttached, BOOL& bAttachCalled, DWORD& nErr);
	public:
		bool    isGuiVisible();
		bool    isGuiOverCon();
		void    StoreGuiChildRect(LPRECT prcNewPos);
		void    SetGuiMode(DWORD anFlags, HWND ahGuiWnd, DWORD anStyle, DWORD anStyleEx, LPCWSTR asAppFileName, DWORD anAppPID, int anBits, RECT arcPrev);
		void    UpdateStartArgs(RConStartArgsEx::SplitType aSplitType, UINT aSplitValue, UINT aSplitPane, bool active);
		static void CorrectGuiChildRect(DWORD anStyle, DWORD anStyleEx, RECT& rcGui, LPCWSTR pszExeName);
		static bool CanCutChildFrame(LPCWSTR pszExeName);

		explicit CRealConsole(CVirtualConsole* pVCon, CConEmuMain* pOwner);
		bool Construct(CVirtualConsole* apVCon, RConStartArgsEx *args);
		~CRealConsole();

		CVirtualConsole* VCon();
		CConEmuMain* Owner();

		BYTE GetConsoleKeyShortcuts() { return this ? m_ConsoleKeyShortcuts : 0; };
		BYTE GetDefaultTextColorIdx() { return this ? (mn_TextColorIdx & 0xF) : 7; };
		BYTE GetDefaultBackColorIdx() { return this ? (mn_BackColorIdx & 0xF) : 0; };

		bool PreInit();
		void DumpConsole(HANDLE ahFile);
		bool LoadDumpConsole(LPCWSTR asDumpFile);

		RealBufferType GetActiveBufferType();
		bool SetActiveBuffer(RealBufferType aBufferType);

		void DoLockUnlock(bool bLock);

		bool SetConsoleSize(SHORT sizeX, SHORT sizeY, USHORT sizeBuffer=0, DWORD anCmdID=CECMD_SETSIZESYNC);
		void EndSizing();
	private:
		bool SetActiveBuffer(CRealBuffer* aBuffer, bool abTouchMonitorEvent = true);
		bool LoadAlternativeConsole(LoadAltMode iMode = lam_Default);
	public:
		COORD ScreenToBuffer(COORD crMouse);
		COORD BufferToScreen(COORD crMouse, bool bFixup = true, bool bVertOnly = false);
		bool PostCtrlBreakEvent(DWORD nEvent, DWORD nGroupId);
		bool PostConsoleEvent(INPUT_RECORD* piRec, bool bFromIME = false);
		bool PostKeyPress(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode = -1);
		void AddIndicatorsCtrlState(DWORD& dwControlKeyState);
		void AddModifiersCtrlState(DWORD& dwControlKeyState);
		bool DeleteWordKeyPress(bool bTestOnly = false);
		bool PostKeyUp(WORD vkKey, DWORD dwControlState, wchar_t wch, int ScanCode = -1);
		bool PostLeftClickSync(COORD crDC);
		bool PostConsoleEventPipe(MSG64 *pMsg, size_t cchCount = 1);
		void ShowKeyBarHint(WORD nID);
		bool PostPromptCmd(bool CD, LPCWSTR asCmd);
		void OnKeysSending();

	public:
		enum class PostStringFlags
		{
			None = 1,
			AllowGroup = 2,
			XTermSequence = 4,
		};
	protected:
		friend class CAltNumpad;
		bool PostString(wchar_t* pszChars, size_t cchCount, PostStringFlags flags);
	private:
		bool ChangePromptPosition(const AppSettings* pApp, COORD crMouse);
		bool IsPromptActionAllowed(bool bFromMouse, const AppSettings* pApp);
		int  EvalPromptCtrlBSCount(const AppSettings* pApp);
		int  EvalPromptLeftRightCount(const AppSettings* pApp, COORD crMouse, WORD& vkKey);
		void PostMouseEvent(UINT messg, WPARAM wParam, COORD crMouse, bool abForceSend = false);
	public:
		bool OpenConsoleEventPipe();
		bool PostConsoleMessage(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
		bool SetFullScreen();
		bool ShowOtherWindow(HWND hWnd, int swShow, bool abAsync = true);
		bool SetOtherWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags);
		bool SetOtherWindowFocus(HWND hWnd, bool abSetForeground);
		HWND SetOtherWindowParent(HWND hWnd, HWND hParent);
		bool SetOtherWindowRgn(HWND hWnd, int nRects, LPRECT prcRects, bool bRedraw);
		void PostDragCopy(bool abMove);
		void PostMacro(LPCWSTR asMacro, bool abAsync = false);
		CEStr PostponeMacro(CEStr&& asMacro);
		bool GetFarVersion(FarVersion* pfv);
		bool IsFarLua();
		bool StartDebugger(StartDebugType sdt);
	private:
		struct PostMacroAnyncArg
		{
			CRealConsole* pRCon;
			BOOL    bPipeCommand;
			DWORD   nCmdSize;
			DWORD   nCmdID;
			union
			{
				wchar_t szMacro[1];
				BYTE    Data[1];
			};
		};
		CEStr mpsz_PostCreateMacro;
		void ProcessPostponedMacro();
		static DWORD WINAPI PostMacroThread(LPVOID lpParameter);
		HANDLE mh_PostMacroThread = nullptr; DWORD mn_PostMacroThreadID = 0;
		void PostCommand(DWORD anCmdID, DWORD anCmdSize, LPCVOID ptrData);
		DWORD mn_InPostDeadChar;
		void OnKeyboardInt(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars, const MSG* pDeadCharMsg);
		struct KeyboardIntArg
		{
			HWND hWnd; UINT messg; WPARAM wParam; LPARAM lParam; const wchar_t *pszChars; const MSG* pDeadCharMsg;
		};
		static bool OnKeyboardBackCall(CVirtualConsole* pVCon, LPARAM lParam);
	public:
		void OnKeyboard(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars, const MSG* pDeadCharMsg);
		const ConEmuHotKey* ProcessSelectionHotKey(const ConEmuChord& VkState, bool bKeyDown, const wchar_t *pszChars);
		TermEmulationType GetTermType();
		bool UseTrueColor() const;
		bool IsConPty() const { return mb_ConPty.load(); }
		bool GetConPtyGlyphs(int columns, int rows, std::vector<ConPtyGlyph>& glyphs, bool fullFrame = false);
		bool GetColorData(AnnotationHeader& header, const AnnotationInfo*& cells, bool& absolute);
		bool GetConPtyLink(COORD position, CEStr& uri, COORD& first, COORD& last);
		bool GetBracketedPaste();
		bool GetAppCursorKeys();
		LPCWSTR GetMntPrefix();
		bool ProcessXtermSubst(const INPUT_RECORD& r);
		void ProcessKeyboard(UINT messg, WPARAM wParam, LPARAM lParam, const wchar_t *pszChars);
		void OnKeyboardIme(HWND hWnd, UINT messg, WPARAM wParam, LPARAM lParam);
		bool OnMouse(UINT messg, WPARAM wParam, int x, int y, bool abForceSend = false);
		bool OnMouseSelection(UINT messg, WPARAM wParam, int x, int y);
		void OnScroll(UINT messg, WPARAM wParam, int x, int y, bool abFromTouch = false);
		void OnFocus(bool abFocused);
		void OnConsoleDataChanged();

		void StopSignal();
		void StopThread(bool abRecreating = false);

		bool StartStopTermMode(DWORD pid, TermModeCommand mode, DWORD value);
		bool StartStopTermMode(TermModeCommand mode, ChangeTermAction action);
		void StartStopXTerm(DWORD nPID, bool xTerm);
		void StartStopXMouse(DWORD nPID, TermMouseMode MouseMode);
		void StartStopBracketedPaste(DWORD nPID, bool bUseBracketedPaste);
		void StartStopAppCursorKeys(DWORD nPID, bool bAppCursorKeys);
		void SetCursorShape(TermCursorShapes xtermShape);

		void PortableStarted(CESERVER_REQ_PORTABLESTARTED* pStarted);
		bool InScroll();
		bool isBufferHeight();
		bool isAlternative();
		HWND isPictureView(bool abIgnoreNonModal = false);
		bool isWindowVisible();
		LPCTSTR GetTitle(bool abGetRenamed=false);
		LPCWSTR GetPanelTitle();
		LPCWSTR GetTabTitle(CTab& tab);
		void GetConsoleScreenBufferInfo(CONSOLE_SCREEN_BUFFER_INFO* sbi);
		void GetConsoleInfo(ConsoleInfoArg* pInfo);
		bool QueryPromptStart(COORD *cr);
		void QueryTermModes(wchar_t* pszInfo, int cchMax, bool bFull);
		void QueryRConModes(wchar_t* pszInfo, int cchMax, bool bFull);
		void QueryCellInfo(wchar_t* pszInfo, int cchMax);
		void GetConsoleCursorInfo(CONSOLE_CURSOR_INFO *ci, COORD *cr = nullptr);
		DWORD GetConsoleCP();
		DWORD GetConsoleOutputCP();
		void GetConsoleModes(WORD& nConInMode, WORD& nConOutMode, TermEmulationType& Term, bool& bBracketedPaste);
		void SyncConsole2Window(bool abNtvdmOff = false, LPRECT prcNewWnd=nullptr);
		void SyncGui2Window(const RECT rcVConBack);
		HWND OnServerStarted(const HWND ahConWnd, const DWORD anServerPID, const DWORD dwKeybLayout, CESERVER_REQ_SRVSTARTSTOPRET& pRet);
		void OnDosAppStartStop(enum StartStopType sst, DWORD anPID);
		bool isProcessExist(DWORD anPID);
		int  GetProcesses(ConProcess** ppPrc, bool ClientOnly = false);
		DWORD GetFarPID(bool abPluginRequired=false);
		void SetFarPID(DWORD nFarPID);
		void SetFarPluginPID(DWORD nFarPluginPID);
		void SetProgramStatus(DWORD nDrop, DWORD nSet);
		void SetFarStatus(DWORD nNewFarStatus);
		bool GetProcessInformation(DWORD nPID, ConProcess* rpProcess = nullptr);
		ConEmuAnsiLog GetAnsiLogInfo();
		LPCWSTR GetConsoleInfo(LPCWSTR asWhat, CEStr& rsInfo);
		LPCWSTR GetActiveProcessInfo(CEStr& rsInfo);
		DWORD GetActivePID(ConProcess* rpProcess = nullptr);
		DWORD GetInteractivePID();
		DWORD GetLoadedPID();
		DWORD GetRunningPID();
		LPCWSTR GetActiveProcessName();
		CEActiveAppFlags GetActiveAppFlags();
		DWORD GetActiveDlgFlags();
		int GetActiveAppSettingsId(bool bReload = false);
	private:
		int GetDefaultAppSettingsId();
	public:
		void ResetActiveAppSettingsId();
		DWORD GetProgramStatus();
		DWORD GetFarStatus();
		bool isServerAlive();
		DWORD GetServerPID(bool bMainOnly = false);
		DWORD GetTerminalPID();
		DWORD GetMonitorThreadID();
		bool isServerCreated(bool bFullRequired = false);
		bool isServerAvailable();
		bool isServerClosing(bool bStrict = false);
		void ResetTopLeft();
		LRESULT DoScroll(int nDirection, UINT nCount = 1);
		bool GetConsoleSelectionInfo(CONSOLE_SELECTION_INFO *sel);
		bool isConSelectMode();
		bool isCygwinMsys();
		bool isUnixFS();
		bool isPosixConvertAllowed();
		bool isFar(bool abPluginRequired=false);
		bool isFarBufferSupported();
		bool isSendMouseAllowed();
		bool isFarInStack();
		bool isFarKeyBarShown();
		bool isInternalScroll();
		bool isSelectionAllowed();
		bool isSelectionPresent();
		bool isMouseSelectionPresent();
		bool isPaused();
		CEPauseCmd Pause(CEPauseCmd cmd);
		void AutoCopyTimer(); // Чтобы разрулить "Auto Copy" & "Double click - select word"
		void StartSelection(bool abTextMode, SHORT anX=-1, SHORT anY=-1, bool abByMouse = false, DWORD anAnchorFlag=0);
		void ChangeSelectionByKey(UINT vkKey, bool bCtrl, bool bShift);
		void ExpandSelection(SHORT anX, SHORT anY);
		bool DoSelectionCopy(CECopyMode CopyMode = cm_CopySel, BYTE nFormat = CTSFormatDefault, LPCWSTR pszDstFile = nullptr);
		void DoSelectionFinalize();
		void OnSelectionChanged();
		void DoFindText(int nDirection);
		void DoEndFindText();
		void PasteExplorerPath(bool bDoCd = true, bool bSetFocus = true);
		void CtrlWinAltSpace();
		void ShowConsoleOrGuiClient(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
		void ShowConsole(int nMode); // -1 Toggle, 0 - Hide, 1 - Show
		void ShowGuiClientExt(int nMode, bool bDetach = false); // -1 Toggle, 0 - Hide, 1 - Show
		void ShowGuiClientInt(bool bShow);
		void ChildSystemMenu();
		bool isDetached();
		bool AttachConemuC(HWND ahConWnd, DWORD anConemuC_PID, const CESERVER_REQ_STARTSTOP* rStartStop, CESERVER_REQ_SRVSTARTSTOPRET& pRet);
		void QueryStartStopRet(CESERVER_REQ_SRVSTARTSTOPRET& pRet);
		void SetInitEnvCommands(CESERVER_REQ_SRVSTARTSTOPRET& pRet);
		bool RecreateProcess(RConStartArgsEx *args);
		void GetConsoleData(wchar_t* pChar, CharAttr* pAttr, int nWidth, int nHeight, ConEmuTextRange& etr);
		void ResetHighlightHyperlinks();
		ExpandTextRangeType GetLastTextRangeType();
		bool IsFarHyperlinkAllowed(bool abFarRequired);
	private:
		bool PreCreate(RConStartArgsEx *args);

		CDpiForDialog* mp_RenameDpiAware = nullptr;
		static INT_PTR CALLBACK renameProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	public:
		bool IsConsoleDataChanged();
		void OnActivate(int nNewNum, int nOldNum);
		void OnDeactivate(int nNewNum);
		void ShowHideViews(bool abShow);
		void OnGuiFocused(bool abFocus, bool abForceChild = false);
		void UpdateServerActive(bool abImmediate = false);
		void UpdateScrollInfo();
		void SetTabs(ConEmuTab* apTabs, int anTabsCount, DWORD anFarPID);
		void DoRenameTab();
		bool DuplicateRoot(bool bSkipMsg = false, bool bRunAsAdmin = false, LPCWSTR asNewConsole = nullptr, LPCWSTR asApp = nullptr, LPCWSTR asParm = nullptr);
		void RenameTab(LPCWSTR asNewTabText = nullptr);
		void RenameWindow(LPCWSTR asNewWindowText = nullptr);
		int GetTabCount(bool abVisibleOnly = false);
		int GetRootProcessIcon();
		LPCWSTR GetRootProcessName();
		void NeedRefreshRootProcessIcon();
		int GetActiveTab();
		CEFarWindowType GetActiveTabType();
		bool GetTab(int tabIdx, /*OUT*/ CTab& rTab);
		int GetModifiedEditors();
		bool ActivateFarWindow(int anWndIndex);
		DWORD CanActivateFarWindow(int anWndIndex);
		bool IsSwitchFarWindowAllowed();
		LPCWSTR GetActivateFarWindowError(wchar_t* pszBuffer, size_t cchBufferMax);
		void OnConsoleKeyboardLayout(DWORD dwNewLayout);
		void SwitchKeyboardLayout(WPARAM wParam,DWORD_PTR dwNewKeybLayout);
		void CloseConsole(bool abForceTerminate, bool abConfirm, bool abAllowMacro = true);
		void CloseConsoleWindow(bool abConfirm);
		bool TerminateAllButShell(bool abConfirm);
		bool TerminateActiveProcess(bool abConfirm, DWORD nPID);
		bool TerminateActiveProcessConfirm(DWORD nPID);
		bool ChangeAffinityPriority(LPCWSTR asAffinity = nullptr, LPCWSTR asPriority = nullptr);
		bool isCloseTabConfirmed(CEFarWindowType TabType, LPCWSTR asConfirmation, bool bForceAsk = false);
		void CloseConfirmReset();
		bool CanCloseTab(bool abPluginRequired = false);
		void CloseTab();
		bool isConsoleClosing();
		bool isConsoleReady();
		void OnServerClosing(DWORD anSrvPID, int* pnShellExitCode);
		void Paste(CEPasteMode PasteMode = pm_Standard, LPCWSTR asText = nullptr, bool abNoConfirm = false, PosixPasteMode posixMode = pxm_Auto);
		bool Write(LPCWSTR pszText, int nLen = -1, DWORD* pnWritten = nullptr);
		unsigned isLogging(unsigned level = 1);
		bool LogString(LPCSTR asText);
		bool LogString(LPCWSTR asText);
		bool isActive(bool abAllowGroup);
		bool isInFocus();
		bool isFarPanelAllowed();
		bool isFilePanel(bool abPluginAllowed = false, bool abSkipEditViewCheck = false, bool abSkipDialogCheck = false);
		bool isEditor();
		bool isEditorModified();
		bool isHighlighted();
		bool isViewer();
		bool isVisible();
		bool isNtvdm();
		bool isFixAndCenter(COORD* lpcrConSize = nullptr);
		const RConStartArgsEx& GetArgs();
		void SetPaletteName(LPCWSTR asPaletteName);
		LPCWSTR GetCmd(bool bThisOnly = false);
		LPCWSTR GetStartupDir();
		CEStr CreateCommandLine(bool abForTasks = false);
		bool GetUserPwd(const wchar_t*& rpszUser, const wchar_t*& rpszDomain, bool& rbRestricted) const;
		short GetProgress(AnsiProgressStatus* rpnState/*1-error,2-ind*/, bool* rpbNotFromTitle = nullptr);
		/// <summary>
		/// Set progress state for the console
		/// </summary>
		/// <param name="state">AnsiProgressStatus</param>
		/// <param name="value">0..100 for nState = (Running, Paused, Error)</param>
		/// <param name="pszName">Reserved for future use - exe name of the running process</param>
		/// <returns>true on success</returns>
		bool SetProgress(AnsiProgressStatus state, short value, LPCWSTR pszName = nullptr);
		void UpdateGuiInfoMapping(const ConEmuGuiMapping* apGuiInfo);
		void UpdateFarSettings(DWORD anFarPID = 0, FAR_REQ_FARSETCHANGED* rpSetEnvVar = nullptr);
		void UpdateTextColorSettings(bool ChangeTextAttr = true, bool ChangePopupAttr = true, const AppSettings* apDistinct = nullptr);
		int CoordInPanel(COORD cr, bool abIncludeEdges = false);
		bool GetPanelRect(bool abRight, RECT* prc, bool abFull = false, bool abIncludeEdges = false);
		bool isAdministrator();
		bool isMouseButtonDown();
		void OnConsoleLangChange(DWORD_PTR dwNewKeybLayout);
		void ChangeBufferHeightMode(bool abBufferHeight); // called from TabBar->ConEmu
		bool isAlive();
	protected:
		bool CheckConsoleAppAlive(DWORD const nCurFarPID, bool const bLastAlive);
	public:
		bool GetMaxConSize(COORD* pcrMaxConSize);
		int GetDetectedDialogs(int anMaxCount, SMALL_RECT* rc, DWORD* rf);
		const CRgnDetect* GetDetector();
		int GetStatusLineCount(int nLeftPanelEdge);
		const SYSTEMTIME& GetStartTime() const;
		LPCWSTR GetConsoleStartDir(CEStr& szDir);
		LPCWSTR GetFileFromConsole(LPCWSTR asSrc, CEStr& szFull);
		LPCWSTR GetConsoleCurDir(CEStr& szDir, bool NeedRealPath);
		void GetPanelDirs(CEStr& szActiveDir, CEStr& szPassive);

	public:
		bool IsConsoleThread();
		void SetForceRead();
		void UpdateCursorInfo();
		TermCursorShapes GetCursorShape();
		bool isNeedCursorDraw();
		bool DetachRCon(bool bPosted = false, bool bSendCloseConsole = false, bool bDontConfirm = false);
		void Unfasten();
		void AdminDuplicate();
		const CEFAR_INFO_MAPPING *GetFarInfo(); // FarVer и прочее
		bool InCreateRoot();
		bool InRecreate();
		bool GuiAppAttachAllowed(DWORD anServerPID, LPCWSTR asAppFileName, DWORD anAppPID);
		void ShowPropertiesDialog();
		bool LogInput(UINT uMsg, WPARAM wParam, LPARAM lParam, LPCWSTR pszTranslatedChars = nullptr);

		void OnStartProcessAllowed();
		void OnTimerCheck();
		void OnSelectionTimerCheck();

		static bool RefreshAfterRestore(CVirtualConsole* pVCon, LPARAM lParam);

	public:
		void MonitorAssertTrap();
	private:
		bool mb_MonitorAssertTrap = false;

	protected:
		void SetMainSrvPID(DWORD anMainSrvPID, HANDLE ahMainSrv);
		void SetAltSrvPID(DWORD anAltSrvPID/*, HANDLE ahAltSrv*/);
		void SetTerminalPID(DWORD anTerminalPID);
		void SetInCloseConsole(bool InCloseConsole);
		// Сервер и альтернативный сервер
		DWORD mn_MainSrv_PID; HANDLE mh_MainSrv;
		uint64_t mn_ProcessAffinity = 1; DWORD mn_ProcessPriority = NORMAL_PRIORITY_CLASS;
		CDpiForDialog* mp_PriorityDpiAware = nullptr;
		void RepositionDialogWithTab(HWND hDlg);
		static INT_PTR CALLBACK priorityProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
		DWORD mn_CheckFreqLock;
		DWORD mn_ConHost_PID;
		class CConHostSearch : public CRefRelease
		{
		protected:
			virtual void FinalRelease() override;
		public:
			CConHostSearch();
			MMap<DWORD,BOOL> data;
		};
		CRefGuard<CConHostSearch> m_ConHostSearch;
		void ConHostSearchPrepare();
		DWORD ConHostSearch(bool bFinal);
		void ConHostSetPID(DWORD nConHostPID);
		bool  mb_MainSrv_Ready; // Сервер готов принимать команды?
		DWORD mn_ActiveLayout;
		DWORD mn_AltSrv_PID;  //HANDLE mh_AltSrv;
		DWORD mn_Terminal_PID; // cygwin/msys connector
		HANDLE mh_SwitchActiveServer, mh_ActiveServerSwitched;
		bool mb_SwitchActiveServer;
		enum SwitchActiveServerEvt { eDontChange, eSetEvent, eResetEvent };
		void SetSwitchActiveServer(bool bSwitch, SwitchActiveServerEvt eCall, SwitchActiveServerEvt eResult);
		bool InitAltServer(DWORD nAltServerPID/*, HANDLE hAltServer*/);
		bool ReopenServerPipes();

		// Пайп консольного ввода
		wchar_t ms_ConEmuCInput_Pipe[MAX_PATH];
		HANDLE mh_ConInputPipe; // wsprintf(ms_ConEmuCInput_Pipe, CESERVERINPUTNAME, L".", mn_ConEmuC_PID)

		bool mb_InCreateRoot;
		bool mb_UseOnlyPipeInput;
		TCHAR ms_ConEmuC_Pipe[MAX_PATH], ms_MainSrv_Pipe[MAX_PATH], ms_VConServer_Pipe[MAX_PATH];
		//TCHAR ms_ConEmuC_DataReady[64]; HANDLE mh_ConEmuC_DataReady;
		void InitNames();
		// Текущий заголовок консоли и его значение для сравнения (для определения изменений)
		WCHAR Title[MAX_TITLE_SIZE + 1] = L"";
		WCHAR TitleCmp[MAX_TITLE_SIZE + 1] = L"";
		// А здесь содержится то, что отображается в ConEmu (может быть добавлено " (Admin)")
		WCHAR TitleFull[MAX_TITLE_SIZE + 96] = L"";
		WCHAR TitleAdmin[MAX_TITLE_SIZE + 192] = L"";
		// Буфер для CRealConsole::GetTitle
		wchar_t TempTitleRenamed[MAX_RENAME_TAB_LEN/*128*/] = L"";
		// Принудительно дернуть OnTitleChanged, например, при изменении процентов в консоли
		bool mb_ForceTitleChanged = false;
		// Здесь сохраняется заголовок окна (с панелями), когда FAR фокус с панелей уходит (переходит в редактор...).
		WCHAR ms_PanelTitle[CONEMUTABMAX] = L"";
		// Процентики
		struct {
			short Progress = -1; // "-1" means - no percentage
			short LastShownProgress = -1;
			short PreWarningProgress = -1; DWORD LastWarnCheckTick = 0;
			short ConsoleProgress = -1;
			short LastConsoleProgress = -1;
			DWORD LastConProgrTick = 0;
			// Could be set from the console (Ansi codes, Macro)
			AnsiProgressStatus AppProgressState = AnsiProgressStatus::None;
			// Could be set from the console (Ansi codes, Macro)
			short AppProgress = 0;
		} m_Progress;
		// a-la properties
		void setProgress(short value);
		void setLastShownProgress(short value);
		void setPreWarningProgress(short value);
		void setConsoleProgress(short value);
		void setLastConsoleProgress(short value, bool UpdateTick);
		void setAppProgress(AnsiProgressStatus AppProgressState, short AppProgress);
		void logProgress(LPCWSTR asFormat, int V1, int V2 = 0);
		// method
		short CheckProgressInTitle();
		bool StartProcess();
		static bool CreateOrRunAs(CRealConsole* pRCon, RConStartArgsEx& Args, LPWSTR psCurCmd, LPCWSTR& lpszWorkDir, STARTUPINFO& si, PROCESS_INFORMATION& pi, SHELLEXECUTEINFO*& pp_sei, DWORD& dwLastError, bool bExternal = false);
		private:
		bool StartProcessInt(LPCWSTR& lpszCmd, CEStr& curCmdBuffer, LPCWSTR& lpszWorkDir, bool bNeedConHostSearch,
				HWND hSetForeground, DWORD& nCreateBegin, DWORD& nCreateEnd, DWORD& nCreateDuration, BYTE nTextColorIdx /*= 7*/, BYTE nBackColorIdx /*= 0*/,
				BYTE nPopTextColorIdx /*= 5*/, BYTE nPopBackColorIdx /*= 15*/, STARTUPINFO& si, PROCESS_INFORMATION& pi, DWORD& dwLastError);
		void ResetVarsOnStart();
		protected:
		bool StartMonitorThread();
		void SetMonitorThreadEvent();
		bool mb_NeedStartProcess;

		// Нить наблюдения за консолью
		static DWORD WINAPI MonitorThread(LPVOID lpParameter);
		DWORD MonitorThreadWorker(bool bDetached, bool& rbChildProcessCreated);
		static int WorkerExFilter(unsigned int code, struct _EXCEPTION_POINTERS *ep, LPCTSTR szFile, UINT nLine);
		HANDLE mh_MonitorThread = nullptr; DWORD mn_MonitorThreadID = 0; bool mb_WasForceTerminated = false;
		HANDLE mh_MonitorThreadEvent;
		HANDLE mh_UpdateServerActiveEvent;
		DWORD mn_ServerActiveTick1, mn_ServerActiveTick2;
		//bool mb_UpdateServerActive;
		DWORD mn_LastUpdateServerActive;

		DWORD mn_TermEventTick;
		HANDLE mh_TermEvent;
		MEvent mh_ApplyFinished;
		HANDLE mh_StartExecuted;
		bool mb_StartResult;
		RConStartState m_StartState = rss_NotStarted;
		MSectionSimple m_StartStateCS{true};
		void UpdateStartState(RConStartState state, bool force = false);
		bool mb_FullRetrieveNeeded; //, mb_Detached;
		RConStartArgsEx m_Args;
		SYSTEMTIME m_StartTime;
		CEStr ms_DefTitle;
		CEStr ms_StartWorkDir;
		CEStr ms_CurWorkDir;
		CEStr ms_CurPassiveDir;
		MSectionSimple* mpcs_CurWorkDir;
		void StoreCurWorkDir(CESERVER_REQ_STORECURDIR* pNewCurDir);
		bool ReloadFarWorkDir();

		wchar_t* ms_MountRoot;
		void SetMountRoot(CESERVER_REQ* pConnectorInfo);

		bool mb_WasStartDetached;
		SYSTEMTIME mst_ServerStartingTime;
		void SetRootProcessName(LPCWSTR asProcessName);
		wchar_t ms_RootProcessName[MAX_PATH];
		int mn_RootProcessIcon;
		bool mb_NeedLoadRootProcessIcon;
		CESERVER_ROOT_INFO m_RootInfo;
		void UpdateRootInfo(const CESERVER_ROOT_INFO& RootInfo);

		bool WaitConsoleSize(int anWaitSize, DWORD nTimeout);
	private:
		friend class CRealBuffer;
		friend class CRConData;
		CRealBuffer* mp_RBuf; // Реальный буфер консоли
		CRealBuffer* mp_EBuf; // Сохранение данных после выполненной команды в Far
		CRealBuffer* mp_SBuf; // Временный буфер (полный) для блокирования содержимого (выделение/прокрутка/поиск)
		CRealBuffer* mp_ABuf; // Активный буфер консоли -- ссылка на один из mp_RBuf/mp_EBuf/mp_SBuf
		bool mb_ABufChaged; // Сменился активный буфер, обновить консоль

		CRConPalette* mp_Palette;

		int mn_DefaultBufferHeight;
		DWORD mn_LastInactiveRgnCheck;
		#ifdef _DEBUG
		bool mb_DebugLocked; // для отладки - заморозить все нити, чтобы не мешали отладчику, ставится по контекстному меню
		#endif


		struct ServerClosing
		{
			DWORD  nServerPID;     // PID закрывающегося сервера
			DWORD  nRecieveTick;   // Tick, когда получено сообщение о закрытии
			HANDLE hServerProcess; // Handle процесса сервера
			bool   bBackActivated; // Main server was activated back, when AltServer was closed
		} m_ServerClosing;
		//
		MSection csPRC; //DWORD ncsTPRC;
		MArray<ConProcess> m_Processes;
		int mn_ProcessCount, mn_ProcessClientCount;
		DWORD m_FarPlugPIDs[128];
		UINT mn_FarPlugPIDsCount;
		bool mb_SkipFarPidChange;
		DWORD m_TerminatedPIDs[128]; UINT mn_TerminatedIdx;
		//
		DWORD mn_FarPID;
		int   mn_FarNoPanelsCheck; // "Far /e ..."
		ConProcess m_ActiveProcess;
		ConProcess m_AppDistinctProcess;
		bool mb_ForceRefreshAppId;
		wchar_t ms_LastActiveProcess[64]; // Used internally by GetProcessInformation
		void SetActivePID(const ConProcess* apProcess);
		void SetAppDistinctPID(const ConProcess* apProcess);
		DWORD mn_LastSetForegroundPID; // PID процесса, которому в последний раз было разрешено AllowSetForegroundWindow
		DWORD mn_LastProcessNamePID;
		int mn_LastAppSettingsId;
		//
		struct _TabsInfo
		{
			CTabStack m_Tabs;
			CTab* mp_ActiveTab = nullptr;
			int  mn_tabsCount = 0; // Число текущих табов. Может отличаться (в меньшую сторону) от m_Tabs.GetCount()
			bool mb_WasInitialized = false; // Информационно, чтобы ассертов не было
			bool mb_TabsWasChanged = false;
			bool mb_HasModalWindow; // Far Manager modal editor/viewer
			CEFarWindowType nActiveType = fwt_Panels|fwt_CurrentFarWnd;
			int  nActiveIndex = 0;
			int  nActiveFarWindow = 0;
			bool bConsoleDataChanged = false;
			LONG nFlashCounter = 0;
			wchar_t sTabActivationErr[128] = L"";
			void StoreActiveTab(int anActiveIndex, CTab& ActiveTab);
			bool RefreshFarPID(DWORD nNewPID);
		} tabs;
		void CheckPanelTitle();
		//
		bool ProcessUpdate(const DWORD *apPID, UINT anCount);
		bool ProcessUpdateFlags(bool abProcessChanged);
		void ProcessCheckName(struct ConProcess &ConPrc, LPWSTR asFullFileName);
		DWORD mn_ProgramStatus, mn_FarStatus;
		DWORD mn_Comspec4Ntvdm;
		bool mb_IgnoreCmdStop; // При запуске 16bit приложения не возвращать размер консоли! Это сделает OnWinEvent
		bool isShowConsole;
		WORD mn_SelectModeSkipVk; // пропустить "отпускание" клавиши Esc/Enter при выделении текста

		friend class CRealServer;
		friend class CGuiServer;
		CRealServer m_RConServer;

		void SetHwnd(HWND ahConWnd, bool abForceApprove = false);
		void CheckVConRConPointer(bool bForceSet);
		WORD mn_LastVKeyPressed = 0;
		int mn_Focused = -1; //-1 после запуска, 1 - в фокусе, 0 - не в фокусе
		DWORD mn_InRecreate; // Tick, когда начали пересоздание
		bool mb_RecreateFailed;
		bool mb_InDetach; // DetachRCon was initiated
		DWORD InitiateDetach();
		DWORD mn_StartTick; // для определения GetRunTime()
		DWORD mn_DeactivateTick; // чтобы не мигать сразу после "cmd -new_console" из промпта
		DWORD mn_RunTime; // для информации
		DWORD GetRunTime();
		bool mb_WasVisibleOnce;
		bool mb_ProcessRestarted;
		bool mb_InCloseConsole;
		DWORD mn_CloseConfirmedTick;
		bool mb_CloseFarMacroPosted;
		// Logging
		MFileLog *mp_Log = nullptr;
		void CreateLogFiles();
		void CloseLogFiles();
		bool LogInput(INPUT_RECORD* pRec);
		bool RecreateProcessStart();
		void RequestStartup(bool bForce = false);

		// CEFARALIVEEVENT, used to check if Far is reading console input (is alive)
		MEvent m_FarAliveEvent;
		// last tick when m_FarAliveEvent was checked
		DWORD mn_LastFarReadTick = 0;
		// last tick when m_FarAliveEvent was set
		DWORD mn_LastFarAliveTick = 0;

		MPipe<CESERVER_REQ_HDR,CESERVER_REQ_HDR> m_GetDataPipe;
		MEvent m_ConDataChanged;
		// CECONMAPNAME
		MFileMapping<CESERVER_CONSOLE_MAPPING_HDR> m_ConsoleMap;
		// CECONAPPMAPNAME -- ReadOnly
		MFileMapping<CESERVER_CONSOLE_APP_MAPPING> m_AppMap;
		// CEFARMAPNAME: FarVer and others
		CEFAR_INFO_MAPPING m_FarInfo;
		// Don't use directly, but via ms_FarInfoCS only!
		MFileMapping<const CEFAR_INFO_MAPPING> m__FarInfo;
		// CS Lock for m__FarInfo
		MSection ms_FarInfoCS;

		// TrueColor Mapping
		MFileMapping<AnnotationHeader> m_TrueColorerMap;
		MFileMapping<ConPtySharedData> m_ConPtyMap;
		std::vector<ConPtyGlyph> m_FrozenConPtyGlyphs;
		AnnotationHeader m_TrueColorerHeader;
		const AnnotationInfo *mp_TrueColorerData;
		int mn_TrueColorMaxCells;
		DWORD mn_LastColorFarID = 0;
		void CreateColorMapping(); // Open TrueColor mapping
		void CloseColorMapping();
		//
		DWORD mn_LastConsoleDataIdx = -1, mn_LastConsolePacketIdx = -1;
		bool OpenFarMapData();
		void CloseFarMapData(MSectionLock* pCS = nullptr);
		bool OpenMapHeader(bool abFromAttach = false);
		void CloseMapHeader();
		bool mb_DataChanged;
		void OnServerStarted(DWORD anServerPID, HANDLE ahServerHandle, DWORD dwKeybLayout, bool abFromAttach = false);
		void OnStartedSuccess();
		bool mb_RConStartedSuccess;
		//
		struct TermEmulation
		{
			// nCallTermPID was removed because modes are maintained by server
			TermEmulationType   Term; // win32 or xterm
			bool     bBracketedPaste; // All "pasted" text will be wrapped in `\e[200~ ... \e[201~`
			TermMouseMode nMouseMode; // mask of enum TermMouseMode
		} m_Term;
		struct TermCursor
		{
			TermCursorShapes CursorShape;
		} m_TermCursor;
		//
		bool PrepareOutputFile(bool abUnicodeText, wchar_t* pszFilePathName);
		HANDLE PrepareOutputFileCreate(wchar_t* pszFilePathName);
		//
		wchar_t ms_Editor[32], ms_EditorRus[32], ms_Viewer[32], ms_ViewerRus[32];
		wchar_t ms_TempPanel[32], ms_TempPanelRus[32];
		//
		DWORD mn_FarPID_PluginDetected;
		void CheckFarStates();
		void OnTitleChanged();
		DWORD mn_LastInvalidateTick;
		//
		HWND hPictureView = nullptr; bool mb_PicViewWasHidden = false;
		//
		SHELLEXECUTEINFO* mp_sei = nullptr;
		SHELLEXECUTEINFO* mp_sei_dbg = nullptr;
		//
		HWND FindPicViewFrom(HWND hFrom);
		//
		bool isCharBorderVertical(WCHAR inChar);
		bool isCharBorderLeftVertical(WCHAR inChar);
		bool isCharBorderHorizontal(WCHAR inChar);
		bool ConsoleRect2ScreenRect(const RECT &rcCon, RECT *prcScr);

		bool mb_InPostCloseMacro;

		// Searching for files on the console surface (hyperlinking)
		CRConFiles* mp_Files = nullptr;

		// XTerm keyboard substitutions
		TermX* mp_XTerm = nullptr;
		std::atomic_bool mb_ConPty{false};
};
