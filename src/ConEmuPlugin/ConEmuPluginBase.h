
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

#include "../common/Common.h"
#include "../common/MStrDup.h"

struct ConEmuPluginMenuItem;

typedef DWORD GetPanelDirFlags;
const GetPanelDirFlags
	gpdf_NoPlugin = 1,
	gpdf_NoHidden = 2,
	gpdf_Active   = 4,
	gpdf_Left     = 8,
	gpdf_Right    = 16,
	ppdf_GetItems = 32,
	gpdf_Passive  = 0
;

enum CallPluginCmdId
{
	// Add new items - before first numbered item!
	CE_CALLPLUGIN_REQ_DIRA = 96,
	CE_CALLPLUGIN_REQ_DIRP = 97,
	CE_CALLPLUGIN_REQ_DIRS = 98,
	CE_CALLPLUGIN_UPDATEBG = 99,
	CE_CALLPLUGIN_SENDTABS = 100,
	SETWND_CALLPLUGIN_BASE /*= (CE_CALLPLUGIN_SENDTABS+1)*/
	// Following number are reserved for "SetWnd(idx)" switching
};

struct CEPanelInfo
{
	BOOL bVisible;   // Наличие панели
	BOOL bFocused;   // В фокусе
	BOOL bPlugin;    // Плагиновая панель
	int  nPanelType; // enum PANELINFOTYPE
	wchar_t *szCurDir/*[BkPanelInfo_CurDirMax]*/;    // Текущая папка на панели
	wchar_t *szFormat/*[BkPanelInfo_FormatMax]*/; // Доступно только в FAR2, в FAR3 это может быть префикс, если "формат" плагином не опереден
	wchar_t *szHostFile/*[BkPanelInfo_HostFileMax]*/;  // Доступно только в FAR2
	RECT rcPanelRect; // Консольные кооринаты панели. В FAR 2+ с ключом /w верх может быть != {0,0}
	INT_PTR ItemsNumber;
	INT_PTR SelectedItemsNumber;
	INT_PTR CurrentItem;
	const void* panelInfo;
};

extern "C" BOOL WINAPI DllMain(HANDLE, DWORD, LPVOID);

class CPluginBase
{
protected:
	bool mb_StartupInfoOk;
protected:
	int ee_Read, ee_Save, ee_Redraw, ee_Close, ee_GotFocus, ee_KillFocus, ee_Change;
	int ve_Read, ve_Close, ve_GotFocus, ve_KillFocus;
	int se_CommonSynchro;
	int wt_Desktop, wt_Panels, wt_Viewer, wt_Editor, wt_Dialog, wt_VMenu, wt_Help;
	int ma_Other, ma_Shell, ma_Viewer, ma_Editor, ma_Dialog, ma_Search, ma_Disks, ma_MainMenu, ma_Menu, ma_Help;
	int ma_InfoPanel, ma_QViewPanel, ma_TreePanel, ma_FindFolder, ma_UserMenu;
	int ma_ShellAutoCompletion, ma_DialogAutoCompletion;
	int of_LeftDiskMenu, of_PluginsMenu, of_FindList, of_Shortcut, of_CommandLine, of_Editor, of_Viewer, of_FilePanel, of_Dialog, of_Analyse, of_RightDiskMenu, of_FromMacro;
	int fctl_GetPanelDirectory, fctl_GetPanelFormat, fctl_GetPanelPrefix, fctl_GetPanelHostFile;
	int pt_FilePanel, pt_TreePanel;
	HANDLE InvalidPanelHandle;

	friend HANDLE WINAPI OpenPluginW(int OpenFrom, INT_PTR Item);

protected:
	// Используется только в Far 1.7x и Far 2.x
	// и ТОЛЬКО для ЧТЕНИЯ настроек PanelTabs (SeparateTabs/ButtonColors)
	wchar_t* ms_RootRegKey; // НЕ ВКЛЮЧАЯ "\\Plugins"
	void InitRootRegKey();
	void SetRootRegKey(wchar_t* asKeyPtr);

	bool ReloadFarInfoApi();

	static void EmergencyShow();

protected:
	friend BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
	static void DllMain_ProcessAttach(HMODULE hModule);
	static void DllMain_ProcessDetach();

	static bool gb_DllUniqueWarned, gb_DllUniqueChecked;
	static bool DllCheckUnique(bool bFromMenu = false);

	static bool gb_ProtocolWarned;
	static void ShowProtocolWarning();

	static int ShowMessageBox(LPCWSTR asMessage, UINT uType);

public:
	CPluginBase();
	virtual ~CPluginBase();

	int ShowMessageGui(int aiMsg, int aiButtons);
	void PostMacro(const wchar_t* asMacro, INPUT_RECORD* apRec, bool abShowParseErrors = true);
	bool isMacroActive(int& iMacroActive);
	void UpdatePanelDirs();
	bool StorePanelDirs(LPCWSTR asActive, LPCWSTR asPassive);
	bool RunExternalProgram(wchar_t* pszCommand);
	bool ProcessCommandLine(wchar_t* pszCommand);
	void ShowPluginMenu(PluginCallCommands nCallID = pcc_None);
	int ProcessSynchroEvent(int Event, void *Param);
	int ProcessEditorViewerEvent(int EditorEvent, int ViewerEvent);
	bool isEditorViewer();
	bool isModalEditorViewer();
	void ShowConsoleInfo();
	bool StartDebugger();
	bool Attach2Gui(bool bLeaveOpened = false);
	bool FindServerCmd(DWORD nServerCmd, DWORD &dwServerPID, bool bFromAttach = false);
	int ShowMessage(int aiMsg, int aiButtons);
	bool UpdateConEmuTabs(bool abSendChanges);
	void CheckResources(bool abFromStartup);
	void InitResources();
	HANDLE OpenPluginCommon(int OpenFrom, INT_PTR Item, bool FromMacro);
	void ExitFarCommon(bool bFromDllMain = false);
	void OnMainThreadActivated();
	void ProcessSetWindowCommand();
	void CommonPluginStartup();
	void StopThread(bool bFromDllMain = false);
	void ExecuteSynchro();
	bool ProcessCommand(DWORD nCmd, BOOL bReqMainThread, LPVOID pCommandData, CESERVER_REQ** ppResult = NULL, bool bForceSendTabs = false);
	static bool FarSetConsoleSize(SHORT nNewWidth, SHORT nNewHeight);
	bool ReloadFarInfo(bool abForce);
	void ProcessEditorInputInternal(const INPUT_RECORD& Rec);
	bool CheckBufferEnabled();
	void FillUpdateBackground(struct PaintBackgroundArg* pFar);
	bool GetPanelInfo(GetPanelDirFlags Flags, BkPanelInfo* pBk);
	INT_PTR PanelControl(HANDLE hPanel, int Command, INT_PTR Param1, void* Param2);
	void ProcessDragFrom();
	void ProcessDragTo();
	bool IsCurrentTabModal();
	void ShowTabsList();
	void ShowPanelsList();
	void EditViewConsoleOutput(bool abView);
	void SwitchTabCommand(PluginMenuCommands cmd);
	bool PrintText(LPCWSTR pszText);

	bool cmd_OpenEditorLine(CESERVER_REQ_FAREDITOR *pCmd);
	bool cmd_RedrawFarCall(CESERVER_REQ*& pCmdRet, CESERVER_REQ** ppResult);
	bool cmd_SetWindow(LPVOID pCommandData, bool bForceSendTabs);
	void cmd_LeftClickSync(LPVOID pCommandData);
	void cmd_CloseQSearch();
	void cmd_EMenu(LPVOID pCommandData);
	bool cmd_ExternalCallback(LPVOID pCommandData);
	void cmd_ConSetFont(LPVOID pCommandData);
	void cmd_GuiChanged(LPVOID pCommandData);

	#ifdef _DEBUG
	static bool DebugGetKeyboardState(LPBYTE pKeyStates);
	static DWORD DebugCheckKeyboardLayout();
	static void DebugInputPrint(INPUT_RECORD r);
	#endif

	static BOOL OnConsoleReadInputWork(HookCallbackArg* pArgs);
	static bool OnPanelViewCallbacks(HookCallbackArg* pArgs, PanelViewInputCallback pfnLeft, PanelViewInputCallback pfnRight);
	static BOOL WINAPI OnShellExecuteExW_Except(HookCallbackArg* pArgs);
	static BOOL WINAPI OnWaitForMultipleObjects(HookCallbackArg* pArgs);
	static BOOL WINAPI OnWaitForMultipleObjectsPost(HookCallbackArg* pArgs);
	static BOOL WINAPI OnGetNumberOfConsoleInputEventsPost(HookCallbackArg* pArgs);
	static BOOL WINAPI OnConsolePeekInput(HookCallbackArg* pArgs);
	static BOOL WINAPI OnConsolePeekInputPost(HookCallbackArg* pArgs);
	static BOOL WINAPI OnConsoleReadInput(HookCallbackArg* pArgs);
	static BOOL WINAPI OnConsoleReadInputPost(HookCallbackArg* pArgs);
	static BOOL WINAPI OnWriteConsoleOutput(HookCallbackArg* pArgs);
	static BOOL WINAPI OnPostWriteConsoleOutput(HookCallbackArg* pArgs);
	static BOOL WINAPI OnConsoleDetaching(HookCallbackArg* pArgs);
	static BOOL WINAPI OnConsoleWasAttached(HookCallbackArg* pArgs);
	static VOID WINAPI OnCurDirChanged();
	static void WINAPI OnLibraryLoaded(HMODULE ahModule);
	static void FillLoadedParm(struct ConEmuLoadedArg* pArg, HMODULE hSubPlugin, BOOL abLoaded);
	static void NotifyConEmuUnloaded();
	static bool LoadFarVersion();
	static bool UngetDummyMouseEvent(bool abRead, HookCallbackArg* pArgs);
	static void OnConsolePeekReadInput(bool abPeek);
	static void TouchReadPeekConsoleInputs(int Peek /*= -1*/);
	static bool pcc_Selected(PluginMenuCommands nMenuID);
	static bool pcc_Disabled(PluginMenuCommands nMenuID);
	bool ActivatePlugin(DWORD nCmd, LPVOID pCommandData, DWORD nTimeout = CONEMUFARTIMEOUT); // Release=10сек, Debug=2мин.
	static DWORD WaitPluginActivation(DWORD nCount, HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
	static void ShutdownPluginStep(LPCWSTR asInfo, int nParm1 = 0, int nParm2 = 0, int nParm3 = 0, int nParm4 = 0);
	static DWORD WINAPI MonitorThreadProcW(LPVOID lpParameter);
	static void CheckConEmuDetached();
	static void CloseMapHeader();
	static int OpenMapHeader();
	static void InitHWND();
	static DWORD GetMainThreadId();
	static bool RunExternalProgramW(wchar_t* pszCommand, wchar_t* pszCurDir, bool bSilent=false);
	static bool OutDataAlloc(DWORD anSize);
	static bool OutDataRealloc(DWORD anNewSize);
	static bool OutDataWrite(LPVOID apData, DWORD anSize);
	static bool CreateTabs(int windowCount);
	bool AddTab(int &tabCount, int WindowPos, bool losingFocus, bool editorSave,
			int Type, LPCWSTR Name, LPCWSTR FileName,
			int Current, int Modified, int Modal,
			int EditViewId);
	void AddTabFinish(int tabCount);
	static void SendTabs(int tabCount, bool abForceSend=false);
	static void CloseTabs();
	static LPWSTR ToUnicode(LPCSTR asOemStr);
	static void ToOem(LPCWSTR asUnicode, char* rsOem, INT_PTR cchOemMax);
	static LPSTR ToOem(LPCWSTR asUnicode);
	static DWORD WINAPI BackgroundMacroError(LPVOID lpParameter);

protected:
	void LoadPanelTabsFromRegistry();

	static void TouchReadPeekConsoleInputsMark(int Peek /*= -1*/);

public:
	virtual bool    CheckPanelExist() = 0;
	virtual bool    ExecuteSynchroApi() = 0;
	virtual void    ExitFar() = 0;
	virtual int     GetActiveWindowType() = 0;
	virtual DWORD   GetEditorModifiedState() = 0;
	virtual bool    GetFarRect(SMALL_RECT& rcFar) = 0;
	virtual int     GetMacroArea() = 0;
	virtual LPCWSTR GetMsg(int aiMsg, wchar_t* psMsg = NULL, size_t cchMsgMax = 0) = 0;
	virtual LPWSTR  GetPanelDir(GetPanelDirFlags Flags, wchar_t* pszBuffer = NULL, int cchBufferMax = 0) = 0;
	virtual bool    GetPanelInfo(GetPanelDirFlags Flags, CEPanelInfo* pInfo) = 0;
	virtual bool    GetPanelItemInfo(const CEPanelInfo& PnlInfo, bool bSelected, INT_PTR iIndex, WIN32_FIND_DATAW& Info, wchar_t** ppszFullPathName) = 0;
	virtual void    GetPluginInfoPtr(void* piv) = 0; // PluginInfo* versioned
	virtual int     GetWindowCount() = 0;
	virtual LPCWSTR GetWindowTypeName(int WindowType) = 0;
	virtual void    GuiMacroDlg() = 0;
	virtual bool    InputBox(LPCWSTR Title, LPCWSTR SubTitle, LPCWSTR HistoryName, LPCWSTR SrcText, wchar_t*& DestText) = 0;
	virtual bool    IsMacroActive() = 0;
	virtual void    LoadFarColors(BYTE (&nFarColors)[col_LastIndex]) = 0;
	virtual void    LoadFarSettings(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel) = 0;
	//virtual void    LoadPanelDirs() = 0;
	//virtual void    LoadPanelTabsSettings() = 0;
	#if 0
	virtual bool    LoadPlugin(wchar_t* pszPluginPath) = 0;
	#endif
	virtual HANDLE  Open(const void* apInfo) = 0;
	virtual bool    OpenEditor(LPCWSTR asFileName, bool abView, bool abDeleteTempFile, bool abDetectCP = false, int anStartLine = 0, int anStartChar = 1) = 0;
	virtual INT_PTR PanelControlApi(HANDLE hPanel, int Command, INT_PTR Param1, void* Param2) = 0;
	virtual void    PostMacroApi(const wchar_t* asMacro, INPUT_RECORD* apRec, bool abShowParseErrors) = 0;
	virtual int     ProcessEditorEventPtr(void* p) = 0;
	virtual int     ProcessEditorInputPtr(LPCVOID Rec) = 0;
	virtual int     ProcessSynchroEventPtr(void* p) = 0;
	virtual int     ProcessViewerEventPtr(void* p) = 0;
	virtual void    RedrawAll() = 0;
	virtual void    SetStartupInfoPtr(void *aInfo) = 0;
	virtual void    SetWindow(int nTab) = 0;
	virtual int     ShowMessage(LPCWSTR asMsg, int aiButtons, bool bWarning) = 0;
	virtual int     ShowPluginMenu(ConEmuPluginMenuItem* apItems, int Count, int TitleMsgId = CEPluginName) = 0;
	virtual void    ShowUserScreen(bool bUserScreen) = 0;
	virtual void    StopWaitEndSynchro() = 0;
	virtual bool    UpdateConEmuTabsApi(int windowCount) = 0;
	virtual void    WaitEndSynchro() = 0;
};

CPluginBase* Plugin();

// Exports
#if defined(__GNUC__)
extern "C" {
#endif
int WINAPI GetMinFarVersion();
int WINAPI GetMinFarVersionW();
int WINAPI ProcessEditorEventW(int Event, void *Param);
INT_PTR WINAPI ProcessEditorEventW3(void*);
int WINAPI ProcessViewerEventW(int Event, void *Param);
INT_PTR WINAPI ProcessViewerEventW3(void*);
int WINAPI ProcessSynchroEventW(int Event,void *Param);
INT_PTR WINAPI ProcessSynchroEventW3(void* p);
#if defined(__GNUC__)
}
#endif
// Exports
