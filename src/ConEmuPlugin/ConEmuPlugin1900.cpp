
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

#include "../common/Common.h"
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginW1900.hpp" // Far3
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/plugin_helper.h"
#include "PluginHeader.h"
#include "../ConEmu/version.h"
#include "../common/farcolor3.hpp"
#include "../common/ConEmuColors3.h"
#include "../common/WThreads.h"
#include "../ConEmuHk/ConEmuHooks.h"

#include "ConEmuPlugin1900.h"

#ifdef _DEBUG
//#define SHOW_DEBUG_EVENTS
#endif

extern GUID guid_ConEmu;
extern GUID guid_ConEmuPluginItems;
extern GUID guid_ConEmuPluginMenu;
extern GUID guid_ConEmuGuiMacroDlg;
extern GUID guid_ConEmuWaitEndSynchro;
extern GUID guid_ConEmuMsg;
extern GUID guid_ConEmuInput;
extern GUID guid_ConEmuMenu;

struct PluginStartupInfo *InfoW1900=NULL;
struct FarStandardFunctions *FSFW1900=NULL;

CPluginW1900::CPluginW1900()
{
	ee_Read = EE_READ;
	ee_Save = EE_SAVE;
	ee_Redraw = EE_REDRAW;
	ee_Close = EE_CLOSE;
	ee_GotFocus = EE_GOTFOCUS;
	ee_KillFocus = EE_KILLFOCUS;
	ee_Change = EE_CHANGE;
	ve_Read = VE_READ;
	ve_Close = VE_CLOSE;
	ve_GotFocus = VE_GOTFOCUS;
	ve_KillFocus = VE_KILLFOCUS;
	se_CommonSynchro = SE_COMMONSYNCHRO;
	wt_Desktop = -1;
	wt_Panels = WTYPE_PANELS;
	wt_Viewer = WTYPE_VIEWER;
	wt_Editor = WTYPE_EDITOR;
	wt_Dialog = WTYPE_DIALOG;
	wt_VMenu = WTYPE_VMENU;
	wt_Help = WTYPE_HELP;
	ma_Other = MACROAREA_OTHER;
	ma_Shell = MACROAREA_SHELL;
	ma_Viewer = MACROAREA_VIEWER;
	ma_Editor = MACROAREA_EDITOR;
	ma_Dialog = MACROAREA_DIALOG;
	ma_Search = MACROAREA_SEARCH;
	ma_Disks = MACROAREA_DISKS;
	ma_MainMenu = MACROAREA_MAINMENU;
	ma_Menu = MACROAREA_MENU;
	ma_Help = MACROAREA_HELP;
	ma_InfoPanel = MACROAREA_INFOPANEL;
	ma_QViewPanel = MACROAREA_QVIEWPANEL;
	ma_TreePanel = MACROAREA_TREEPANEL;
	ma_FindFolder = MACROAREA_FINDFOLDER;
	ma_UserMenu = MACROAREA_USERMENU;
	ma_ShellAutoCompletion = MACROAREA_SHELLAUTOCOMPLETION;
	ma_DialogAutoCompletion = MACROAREA_DIALOGAUTOCOMPLETION;
	of_LeftDiskMenu = OPEN_LEFTDISKMENU;
	of_PluginsMenu = OPEN_PLUGINSMENU;
	of_FindList = OPEN_FINDLIST;
	of_Shortcut = OPEN_SHORTCUT;
	of_CommandLine = OPEN_COMMANDLINE;
	of_Editor = OPEN_EDITOR;
	of_Viewer = OPEN_VIEWER;
	of_FilePanel = OPEN_FILEPANEL;
	of_Dialog = OPEN_DIALOG;
	of_Analyse = OPEN_ANALYSE;
	of_RightDiskMenu = OPEN_RIGHTDISKMENU;
	of_FromMacro = OPEN_FROMMACRO;
	fctl_GetPanelDirectory = FCTL_GETPANELDIRECTORY;
	fctl_GetPanelFormat = FCTL_GETPANELFORMAT;
	fctl_GetPanelPrefix = FCTL_GETPANELPREFIX;
	fctl_GetPanelHostFile = FCTL_GETPANELHOSTFILE;
	pt_FilePanel = PTYPE_FILEPANEL;
	pt_TreePanel = PTYPE_TREEPANEL;
}

int CPluginW1900::ProcessEditorEventPtr(void* p)
{
	const ProcessEditorEventInfo* Info = (const ProcessEditorEventInfo*)p;
	return ProcessEditorViewerEvent(Info->Event, -1);
}

int CPluginW1900::ProcessViewerEventPtr(void* p)
{
	const ProcessViewerEventInfo* Info = (const ProcessViewerEventInfo*)p;
	return ProcessEditorViewerEvent(-1, Info->Event);
}

int CPluginW1900::ProcessSynchroEventPtr(void* p)
{
	const ProcessSynchroEventInfo* Info = (const ProcessSynchroEventInfo*)p;
	return Plugin()->ProcessSynchroEvent(Info->Event, Info->Param);
}

wchar_t* CPluginW1900::GetPanelDir(GetPanelDirFlags Flags, wchar_t* pszBuffer /*= NULL*/, int cchBufferMax /*= 0*/)
{
	wchar_t* pszDir = NULL;
	HANDLE hPanel = (Flags & gpdf_Active) ? PANEL_ACTIVE : PANEL_PASSIVE;
	size_t nSize;
	PanelInfo pi = {sizeof(pi)};

	if (!InfoW1900)
		goto wrap;

	nSize = InfoW1900->PanelControl(hPanel, FCTL_GETPANELINFO, 0, &pi);

	if ((Flags & gpdf_NoHidden) && !(pi.Flags & PFLAGS_VISIBLE))
		goto wrap;
	if ((Flags & gpdf_NoPlugin) && (pi.Flags & PFLAGS_PLUGIN))
		goto wrap;

	nSize = InfoW1900->PanelControl(hPanel, FCTL_GETPANELDIRECTORY, 0, 0);

	if (nSize)
	{
		FarPanelDirectory* pDir = (FarPanelDirectory*)calloc(nSize, 1);
		if (pDir)
		{
			pDir->StructSize = sizeof(*pDir);
			nSize = InfoW1900->PanelControl(hPanel, FCTL_GETPANELDIRECTORY, nSize, pDir);
			if (pszBuffer && cchBufferMax > 0)
			{
				lstrcpyn(pszBuffer, pDir->Name, cchBufferMax);
				pszDir = pszBuffer;
			}
			else
			{
				pszDir = lstrdup(pDir->Name).Detach();
			}
			free(pDir);
		}
	}
	// допустимо во время закрытия фара, если это был редактор
	//_ASSERTE(nSize>0 || (pi.Flags & PFLAGS_PLUGIN));

wrap:
	if (!pszDir && pszBuffer)
		*pszBuffer = 0;
	return pszDir;
}

bool CPluginW1900::GetPanelInfo(GetPanelDirFlags Flags, CEPanelInfo* pInfo)
{
	if (!InfoW1900 || !InfoW1900->PanelControl)
		return false;

	HANDLE hPanel = (Flags & gpdf_Active) ? PANEL_ACTIVE : PANEL_PASSIVE;
	PanelInfo pasv = {sizeof(pasv)}, actv = {sizeof(actv)};
	PanelInfo* p;

	if (Flags & (gpdf_Left|gpdf_Right))
	{
		InfoW1900->PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &actv);
		InfoW1900->PanelControl(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, &pasv);
		PanelInfo* pLeft = (actv.Flags & PFLAGS_PANELLEFT) ? &actv : &pasv;
		PanelInfo* pRight = (actv.Flags & PFLAGS_PANELLEFT) ? &pasv : &actv;
		p = (Flags & gpdf_Left) ? pLeft : pRight;
		hPanel = ((p->Flags & PFLAGS_FOCUS) == PFLAGS_FOCUS) ? PANEL_ACTIVE : PANEL_PASSIVE;
	}
	else
	{
		hPanel = (Flags & gpdf_Active) ? PANEL_ACTIVE : PANEL_PASSIVE;
		InfoW1900->PanelControl(hPanel, FCTL_GETPANELINFO, 0, &actv);
		p = &actv;
	}

	if (Flags & ppdf_GetItems)
	{
		static PanelInfo store;
		store = *p;
		pInfo->panelInfo = &store;
	}
	else
	{
		pInfo->panelInfo = NULL;
	}

	pInfo->bVisible = ((p->Flags & PFLAGS_VISIBLE) == PFLAGS_VISIBLE);
	pInfo->bFocused = ((p->Flags & PFLAGS_FOCUS) == PFLAGS_FOCUS);
	pInfo->bPlugin = ((p->Flags & PFLAGS_PLUGIN) == PFLAGS_PLUGIN);
	pInfo->nPanelType = (int)p->PanelType;
	pInfo->rcPanelRect = p->PanelRect;
	pInfo->ItemsNumber = p->ItemsNumber;
	pInfo->SelectedItemsNumber = p->SelectedItemsNumber;
	pInfo->CurrentItem = p->CurrentItem;

	if ((Flags & gpdf_NoHidden) && !pInfo->bVisible)
		return false;

	if (pInfo->szCurDir)
	{
		GetPanelDir(pInfo->bFocused ? gpdf_Active : gpdf_Passive, pInfo->szCurDir, BkPanelInfo_CurDirMax);
	}

	if (pInfo->bPlugin)
	{
		if (pInfo->szFormat)
		{
			PanelControl(hPanel, FCTL_GETPANELFORMAT, BkPanelInfo_FormatMax, pInfo->szFormat);
			// Если "Формат" панели получить не удалось - попробуем взять "префикс" плагина
			if (!*pInfo->szFormat)
				PanelControl(hPanel, FCTL_GETPANELPREFIX, BkPanelInfo_FormatMax, pInfo->szFormat);
		}

		if (pInfo->szHostFile)
		{
			PanelControl(hPanel, FCTL_GETPANELHOSTFILE, BkPanelInfo_HostFileMax, pInfo->szHostFile);
		}
	}
	else
	{
		if (pInfo->szFormat)
			pInfo->szFormat[0] = 0;
		if (pInfo->szHostFile)
			pInfo->szHostFile[0] = 0;
	}

	return true;
}

bool CPluginW1900::GetPanelItemInfo(const CEPanelInfo& PnlInfo, bool bSelected, INT_PTR iIndex, WIN32_FIND_DATAW& Info, wchar_t** ppszFullPathName)
{
	if (!InfoW1900 || !InfoW1900->PanelControl)
		return false;

	FILE_CONTROL_COMMANDS iCmd = bSelected ? FCTL_GETSELECTEDPANELITEM : FCTL_GETPANELITEM;
	INT_PTR iItemsNumber = bSelected ? ((PanelInfo*)PnlInfo.panelInfo)->SelectedItemsNumber : ((PanelInfo*)PnlInfo.panelInfo)->ItemsNumber;

	if ((iIndex < 0) || (iIndex >= iItemsNumber))
	{
		_ASSERTE(FALSE && "iItem out of bounds");
		return false;
	}

	INT_PTR sz = PanelControlApi(PANEL_ACTIVE, iCmd, iIndex, NULL);

	if (sz <= 0)
		return false;

	PluginPanelItem* pItem = (PluginPanelItem*)calloc(sz, 1); // размер возвращается в байтах
	if (!pItem)
		return false;

	FarGetPluginPanelItem gppi = {sizeof(gppi), pItem};
	if (PanelControlApi(PANEL_ACTIVE, iCmd, iIndex, &gppi) <= 0)
	{
		free(pItem);
		return false;
	}

	ZeroStruct(Info);

	Info.dwFileAttributes = LODWORD(pItem->FileAttributes);
	Info.ftCreationTime = pItem->CreationTime;
	Info.ftLastAccessTime = pItem->LastAccessTime;
	Info.ftLastWriteTime = pItem->LastWriteTime;
	Info.nFileSizeLow = LODWORD(pItem->FileSize);
	Info.nFileSizeHigh = HIDWORD(pItem->FileSize);

	if (pItem->FileName)
	{
		LPCWSTR pszName = pItem->FileName;
		int iLen = lstrlen(pszName);
		// If full paths exceeds MAX_PATH chars - return in Info.cFileName the file name only
		if (iLen >= countof(Info.cFileName))
			pszName = PointToName(pItem->FileName);
		lstrcpyn(Info.cFileName, pszName, countof(Info.cFileName));
		if (ppszFullPathName)
			*ppszFullPathName = lstrdup(pItem->FileName).Detach();
	}
	else if (ppszFullPathName)
	{
		_ASSERTE(*ppszFullPathName == NULL);
		*ppszFullPathName = NULL;
	}

	if (pItem->AlternateFileName)
	{
		lstrcpyn(Info.cAlternateFileName, pItem->AlternateFileName, countof(Info.cAlternateFileName));
	}

	free(pItem);

	return true;
}

INT_PTR CPluginW1900::PanelControlApi(HANDLE hPanel, int Command, INT_PTR Param1, void* Param2)
{
	if (!InfoW1900 || !InfoW1900->PanelControl)
		return -1;
	INT_PTR iRc = InfoW1900->PanelControl(hPanel, (FILE_CONTROL_COMMANDS)Command, Param1, Param2);
	return iRc;
}

void CPluginW1900::GetPluginInfoPtr(void *piv)
{
	PluginInfo *pi = (PluginInfo*)piv;
	//memset(pi, 0, sizeof(PluginInfo));
	//pi->StructSize = sizeof(struct PluginInfo);
	_ASSERTE(pi->StructSize>0 && ((size_t)pi->StructSize >= sizeof(*pi)/*(size_t)(((LPBYTE)&pi->MacroFunctionNumber) - (LPBYTE)pi))*/));

	static wchar_t *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1; //lstrcpyW(szMenu[0], L"[&\x2560] ConEmu"); -> 0x2584
	GetMsg(CEPluginName, szMenu1, 240);

	//static WCHAR *szMenu[1], szMenu1[255];
	//szMenu[0]=szMenu1; //lstrcpyW(szMenu[0], L"[&\x2560] ConEmu"); -> 0x2584
	//szMenu[0][1] = L'&';
	//szMenu[0][2] = 0x2560;
	// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
	//IsKeyChanged(TRUE); -- в FAR2 устарело, используем Synchro
	//if (gcPlugKey) szMenu1[0]=0; else lstrcpyW(szMenu1, L"[&\x2584] ");
	//lstrcpynW(szMenu1+lstrlenW(szMenu1), GetMsgW(2), 240);
	//lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240);
	//_ASSERTE(pi->StructSize = sizeof(struct PluginInfo));
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG | PF_PRELOAD;
	//pi->DiskMenuStrings = NULL;
	//pi->DiskMenuNumbers = 0;
	pi->PluginMenu.Guids = &guid_ConEmuPluginMenu;
	pi->PluginMenu.Strings = szMenu;
	pi->PluginMenu.Count = 1;
	//pi->PluginConfigStrings = NULL;
	//pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = L"ConEmu";
	//pi->Reserved = ConEmu_SysID; // 'CEMU'
}

void CPluginW1900::SetStartupInfoPtr(void *aInfo)
{
	INIT_FAR_PSI(::InfoW1900, ::FSFW1900, (PluginStartupInfo*)aInfo);
	mb_StartupInfoOk = true;

	VersionInfo FarVer = {0};
	if (InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETFARMANAGERVERSION, 0, &FarVer))
	{
		if (FarVer.Major == 3)
		{
			gFarVersion.dwBuild = FarVer.Build;
			_ASSERTE(FarVer.Major<=0xFFFF && FarVer.Minor<=0xFFFF);
			gFarVersion.dwVerMajor = (WORD)FarVer.Major;
			gFarVersion.dwVerMinor = (WORD)FarVer.Minor;
			gFarVersion.Bis = (FarVer.Stage==VS_BIS);
			_ASSERTE(gFarVersion.dwBits == WIN3264TEST(32,64));
		}
		else
		{
			_ASSERTE(FarVer.Major == 3);
		}
	}
}

DWORD CPluginW1900::GetEditorModifiedState()
{
	EditorInfo ei{};
	InfoW1900->EditorControl(-1/*Active editor*/, ECTL_GETINFO, 0, &ei);
#ifdef SHOW_DEBUG_EVENTS
	char szDbg[255];
	wsprintfA(szDbg, "Editor:State=%i\n", ei.CurState);
	OutputDebugStringA(szDbg);
#endif
	// Если он сохранен, то уже НЕ модифицирован
	DWORD currentModifiedState = ((ei.CurState & (ECSTATE_MODIFIED|ECSTATE_SAVED)) == ECSTATE_MODIFIED) ? 1 : 0;
	return currentModifiedState;
}


// watch non-modified -> modified editor status change
int CPluginW1900::ProcessEditorInputPtr(LPCVOID aRec)
{
	if (!InfoW1900)
		return 0;

	const ProcessEditorInputInfo *apInfo = (const ProcessEditorInputInfo*)aRec;
	ProcessEditorInputInternal(apInfo->Rec);

	return 0;
}

int CPluginW1900::GetWindowCount()
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return 0;

	INT_PTR windowCount = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWCOUNT, 0, NULL);
	return (int)windowCount;
}

bool CPluginW1900::UpdateConEmuTabsApi(int windowCount)
{
	if (!InfoW1900 || !InfoW1900->AdvControl || gbIgnoreUpdateTabs)
		return false;

	bool lbCh = false, lbDummy = false;
	WindowInfo WInfo = {sizeof(WindowInfo)};
	wchar_t szWNameBuffer[CONEMUTABMAX];
	int tabCount = 0;
	bool lbActiveFound = false;

	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);

	WindowInfo WActive = {sizeof(WActive)};
	WActive.Pos = -1;
	bool bActiveInfo = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, 0, &WActive)!=0;
	// Если фар запущен с ключом "/e" (как standalone редактор) - будет ассерт при первой попытке
	// считать информацию об окне (редактор еще не создан?, а панелей вообще нет)
	_ASSERTE(bActiveInfo && (WActive.Flags & WIF_CURRENT));
	static WindowInfo WLastActive;

	// Проверить, есть ли активный редактор/вьювер/панель
	if (bActiveInfo && (WActive.Type == WTYPE_EDITOR || WActive.Type == WTYPE_VIEWER || WActive.Type == WTYPE_PANELS))
	{
		if (!(WActive.Flags & WIF_MODAL))
			WLastActive = WActive;
	}
	else
	{
		int nTabs = 0, nModalTabs = 0;
		bool bFound = false;
		WindowInfo WModal, WFirst;
		// Поскольку в табах диалоги не отображаются - надо подменить "активное" окно
		// т.е. предпочитаем тот таб, который был активен ранее
		for (int i = 0; i < windowCount; i++)
		{
			WInfo.Pos = i;
			if (InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, 0, &WInfo)
				&& (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS))
			{
				if (!nTabs)
					WFirst = WInfo;
				nTabs++;
				if (WInfo.Flags & WIF_MODAL)
				{
					nModalTabs++;
					WModal = WInfo;
				}

				if (WLastActive.StructSize && (WInfo.Type == WLastActive.Type) && (WInfo.Id == WLastActive.Id))
				{
					bActiveInfo = bFound = true;
					WActive = WInfo;
				}
			}
		}

		if (!bFound)
		{
			if (nModalTabs)
			{
				bActiveInfo = true;
				WActive = WModal;
			}
			else if (nTabs)
			{
				bActiveInfo = true;
				WActive = WFirst;
			}
		}
	}

	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, 0, &WInfo);

		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			WInfo.Pos = i;
			WInfo.Name = szWNameBuffer;
			WInfo.NameSize = CONEMUTABMAX;
			InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, 0, &WInfo);
			WARNING("Для получения имени нужно пользовать ECTL_GETFILENAME");

			//// Проверить, чего там...
			//_ASSERTE((WInfo.Flags & WIF_MODAL) == 0);

			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
			{
				if ((WInfo.Flags & WIF_CURRENT))
				{
					lbActiveFound = true;
				}
				else if (bActiveInfo && (WInfo.Type == WActive.Type) && (WInfo.Id == WActive.Id))
				{
					WInfo.Flags |= WIF_CURRENT;
					lbActiveFound = true;
				}

				TODO("Определение ИД редактора/вьювера");
				lbCh |= AddTab(tabCount, -1, false/*losingFocus*/, false/*editorSave*/,
				               WInfo.Type, WInfo.Name, /*editorSave ? ei.FileName :*/ NULL,
				               (WInfo.Flags & WIF_CURRENT), (WInfo.Flags & WIF_MODIFIED), (WInfo.Flags & WIF_MODAL),
							   0/*WInfo.Id?*/);
			}
		}
	}

	// Скорее всего это модальный редактор (или вьювер?)
	if (!lbActiveFound)
	{
		_ASSERTE("Active window must be detected already!" && 0);
		WInfo.Pos = -1;

		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		if (InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, 0, &WInfo))
		{
			// Проверить, чего там...
			_ASSERTE((WInfo.Flags & WIF_MODAL) == 0);

			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
			{
				WInfo.Pos = -1;
				WInfo.Name = szWNameBuffer;
				WInfo.NameSize = CONEMUTABMAX;
				InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, 0, &WInfo);

				if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
				{
					tabCount = 0;
					TODO("Определение ИД Редактора/вьювера");
					lbCh |= AddTab(tabCount, -1, false/*losingFocus*/, false/*editorSave*/,
					               WInfo.Type, WInfo.Name, /*editorSave ? ei.FileName :*/ NULL,
					               (WInfo.Flags & WIF_CURRENT), (WInfo.Flags & WIF_MODIFIED), 1/*Modal*/,
								   0);
				}
			}
			else if (WInfo.Type == WTYPE_PANELS)
			{
				gpTabs->Tabs.CurrentType = gnCurrentWindowType = WInfo.Type;
			}
		}
	}

	// 101224 - сразу запомнить количество!
	AddTabFinish(tabCount);

	return lbCh;
}

void CPluginW1900::ExitFar()
{
	if (!mb_StartupInfoOk)
		return;

	CPluginBase::ShutdownPluginStep(L"ExitFARW1900");

	WaitEndSynchro();

	if (InfoW1900)
	{
		free(InfoW1900);
		InfoW1900=NULL;
	}

	if (FSFW1900)
	{
		free(FSFW1900);
		FSFW1900=NULL;
	}

	CPluginBase::ShutdownPluginStep(L"ExitFARW1900 - done");
}

int CPluginW1900::ShowMessage(LPCWSTR asMsg, int aiButtons, bool bWarning)
{
	if (!InfoW1900 || !InfoW1900->Message)
		return -1;

	return InfoW1900->Message(&guid_ConEmu, &guid_ConEmuMsg,
					FMSG_ALLINONE1900|(aiButtons?0:FMSG_MB_OK)|(bWarning ? FMSG_WARNING : 0), NULL,
					(const wchar_t * const *)asMsg, 0, aiButtons);
}

LPCWSTR CPluginW1900::GetMsg(int aiMsg, wchar_t* psMsg /*= NULL*/, size_t cchMsgMax /*= 0*/)
{
	LPCWSTR pszRc = (InfoW1900 && InfoW1900->GetMsg) ? InfoW1900->GetMsg(&guid_ConEmu, aiMsg) : L"";
	if (!pszRc)
		pszRc = L"";
	if (psMsg)
		lstrcpyn(psMsg, pszRc, cchMsgMax);
	return pszRc;
}

//void ReloadMacroW1900()
//{
//	if (!InfoW1900 || !InfoW1900->AdvControl)
//		return;
//
//	ActlKeyMacro command;
//	command.Command=MCMD_LOADALL;
//	InfoW1900->AdvControl(&guid_ConEmu,ACTL_KEYMACRO,&command);
//}

void CPluginW1900::SetWindow(int nTab)
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return;

	if (InfoW1900->AdvControl(&guid_ConEmu, ACTL_SETCURRENTWINDOW, nTab, NULL))
		InfoW1900->AdvControl(&guid_ConEmu, ACTL_COMMIT, 0, 0);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void CPluginW1900::PostMacroApi(const wchar_t* asMacro, INPUT_RECORD* apRec, bool abShowParseErrors)
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return;

	MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
	//mcr.Flags = 0; // По умолчанию - вывод на экран разрешен

	while ((asMacro[0] == L'@' || asMacro[0] == L'^') && asMacro[1] && asMacro[1] != L' ')
	{
		switch (*asMacro)
		{
		case L'@':
			mcr.Flags |= KMFLAGS_DISABLEOUTPUT;
			break;
		case L'^':
			mcr.Flags |= KMFLAGS_NOSENDKEYSTOPLUGINS;
			break;
		}
		asMacro++;
	}

	wchar_t* pszMacroCopy = NULL;

	//Far3 build 2576: удален $Text
	//т.к. макросы у нас фаро-независимые - нужны танцы с бубном
	pszMacroCopy = lstrdup(asMacro).Detach();
	CharUpperBuff(pszMacroCopy, lstrlen(pszMacroCopy));
	if (wcsstr(pszMacroCopy, L"$TEXT") && !InfoW1900->MacroControl(&guid_ConEmu, MCTL_SENDSTRING, MSSC_CHECK, &mcr))
	{
		SafeFree(pszMacroCopy);
		pszMacroCopy = (wchar_t*)calloc(lstrlen(asMacro)+1,sizeof(wchar_t)*2);
		wchar_t* psz = pszMacroCopy;
		while (*asMacro)
		{
			if (asMacro[0] == L'$'
				&& (asMacro[1] == L'T' || asMacro[1] == L't')
				&& (asMacro[2] == L'E' || asMacro[2] == L'e')
				&& (asMacro[3] == L'X' || asMacro[3] == L'x')
				&& (asMacro[4] == L'T' || asMacro[4] == L't'))
			{
				lstrcpy(psz, L"print("); psz += 6;

				// Пропустить spasing-symbols
				asMacro += 5;
				while (*asMacro == L' ' || *asMacro == L'\t' || *asMacro == L'\r' || *asMacro == L'\n')
					asMacro++;
				// Копировать строку или переменную
				if (*asMacro == L'@' && *(asMacro+1) == L'"')
				{
					*(psz++) = *(asMacro++); *(psz++) = *(asMacro++);
					while (*asMacro)
					{
						*(psz++) = *(asMacro++);
						if (*(asMacro-1) == L'"')
						{
							if (*asMacro != L'"')
								break;
							*(psz++) = *(asMacro++);
						}
					}
				}
				else if (*asMacro == L'"')
				{
					*(psz++) = *(asMacro++);
					while (*asMacro)
					{
						*(psz++) = *(asMacro++);
						if (*(asMacro-1) == L'\\' && *asMacro == L'"')
						{
							*(psz++) = *(asMacro++);
						}
						else if (*(asMacro-1) == L'"')
						{
							break;
						}
					}
				}
				else if (*asMacro == L'%')
				{
					*(psz++) = *(asMacro++);
					while (*asMacro)
					{
						if (wcschr(L" \t\r\n", *asMacro))
							break;
						*(psz++) = *(asMacro++);
					}
				}
				else
				{
					SafeFree(pszMacroCopy);
					break; // ошибка
				}
				// закрыть скобку
				*(psz++) = L')';
			}
			else
			{
				*(psz++) = *(asMacro++);
			}
		}

		// Если успешно пропатчили макрос
		if (pszMacroCopy)
			asMacro = pszMacroCopy;
	}

	mcr.SequenceText = asMacro;
	if (apRec)
		mcr.AKey = *apRec;

	mcr.Flags |= KMFLAGS_SILENTCHECK;

	if (!InfoW1900->MacroControl(&guid_ConEmu, MCTL_SENDSTRING, MSSC_CHECK, &mcr))
	{
		if (abShowParseErrors)
		{
			wchar_t* pszErrText = NULL;
			size_t iRcSize = InfoW1900->MacroControl(&guid_ConEmu, MCTL_GETLASTERROR, 0, NULL);
			MacroParseResult* Result = iRcSize ? (MacroParseResult*)calloc(iRcSize,1) : NULL;
			if (Result)
			{
				Result->StructSize = sizeof(*Result);
				_ASSERTE(FALSE && "Check MCTL_GETLASTERROR");
				InfoW1900->MacroControl(&guid_ConEmu, MCTL_GETLASTERROR, iRcSize, Result);

				size_t cchMax = (Result->ErrSrc ? lstrlen(Result->ErrSrc) : 0) + lstrlen(asMacro) + 255;
				pszErrText = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
				_wsprintf(pszErrText, SKIPLEN(cchMax)
					L"Error in Macro. Far %u.%u build %u r%u\n"
					L"ConEmu plugin %02u%02u%02u%s[%u] {1900}\n"
					L"Code: %u, Line: %u, Col: %u%s%s\n"
					L"----------------------------------\n"
					L"%s",
					gFarVersion.dwVerMajor, gFarVersion.dwVerMinor, gFarVersion.dwBuild, gFarVersion.Bis ? 1 : 0,
					MVV_1, MVV_2, MVV_3, _CRT_WIDE(MVV_4a), WIN3264TEST(32,64),
					Result->ErrCode, (UINT)(int)Result->ErrPos.Y+1, (UINT)(int)Result->ErrPos.X+1,
					Result->ErrSrc ? L", Hint: " : L"", Result->ErrSrc ? Result->ErrSrc : L"",
					asMacro);

				SafeFree(Result);
			}
			else
			{
				size_t cchMax = lstrlen(asMacro) + 255;
				pszErrText = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
				_wsprintf(pszErrText, SKIPLEN(cchMax)
					L"Error in Macro. Far %u.%u build %u r%u\n"
					L"ConEmu plugin %02u%02u%02u%s[%u] {1900}\n"
					L"----------------------------------\n"
					L"%s",
					gFarVersion.dwVerMajor, gFarVersion.dwVerMinor, gFarVersion.dwBuild, gFarVersion.Bis ? 1 : 0,
					MVV_1, MVV_2, MVV_3, _CRT_WIDE(MVV_4a), WIN3264TEST(32,64),
					asMacro);
			}

			if (pszErrText)
			{
				DWORD nTID;
				HANDLE h = apiCreateThread(BackgroundMacroError, pszErrText, &nTID, "BackgroundMacroError");
				SafeCloseHandle(h);
			}
		}
	}
	else
	{
		//gFarVersion.dwBuild
		InfoW1900->MacroControl(&guid_ConEmu, MCTL_SENDSTRING, 0, &mcr);
	}

	SafeFree(pszMacroCopy);
}

int CPluginW1900::ShowPluginMenu(ConEmuPluginMenuItem* apItems, int Count, int TitleMsgId /*= CEPluginName*/)
{
	if (!InfoW1900)
		return -1;

	//FarMenuItem items[] =
	//{
	//	{ConEmuHwnd ? MIF_SELECTED : MIF_DISABLE,  InfoW1900->GetMsg(&guid_ConEmu,CEMenuEditOutput)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuViewOutput)},
	//	{MIF_SEPARATOR},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuShowHideTabs)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuNextTab)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuPrevTab)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW1900->GetMsg(&guid_ConEmu,CEMenuCommitTab)},
	//	{MIF_SEPARATOR},
	//	{0,                                        InfoW1900->GetMsg(&guid_ConEmu,CEMenuGuiMacro)},
	//	{MIF_SEPARATOR},
	//	{ConEmuHwnd||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED,  InfoW1900->GetMsg(&guid_ConEmu,CEMenuAttach)},
	//	{MIF_SEPARATOR},
	//	//#ifdef _DEBUG
	//	//{0, L"&~. Raise exception"},
	//	//#endif
	//	{IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0,    InfoW1900->GetMsg(&guid_ConEmu,CEMenuDebug)},
	//};

	FarMenuItem* items = (FarMenuItem*)calloc(Count, sizeof(*items));
	for (int i = 0; i < Count; i++)
	{
		if (apItems[i].Separator)
		{
			items[i].Flags = MIF_SEPARATOR;
			continue;
		}
		items[i].Flags	= (apItems[i].Disabled ? MIF_DISABLE : 0)
						| (apItems[i].Selected ? MIF_SELECTED : 0)
						| (apItems[i].Checked  ? MIF_CHECKED : 0)
						;
		items[i].Text = apItems[i].MsgText ? apItems[i].MsgText : InfoW1900->GetMsg(&guid_ConEmu, apItems[i].MsgID);
	}

	int nRc = InfoW1900->Menu(&guid_ConEmu, &guid_ConEmuMenu, -1,-1, 0,
	                         FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                         InfoW1900->GetMsg(&guid_ConEmu,TitleMsgId),
	                         NULL, NULL, NULL, NULL, (FarMenuItem*)items, Count);
	SafeFree(items);
	return nRc;
}

bool CPluginW1900::OpenEditor(LPCWSTR asFileName, bool abView, bool abDeleteTempFile, bool abDetectCP /*= false*/, int anStartLine /*= 0*/, int anStartChar /*= 1*/)
{
	if (!InfoW1900)
		return false;

	bool lbRc;
	INT_PTR iRc;

	LPCWSTR pszTitle = abDeleteTempFile ? InfoW1900->GetMsg(&guid_ConEmu,CEConsoleOutput) : NULL;

	if (!abView)
	{
		iRc = InfoW1900->Editor(asFileName, pszTitle, 0,0,-1,-1,
		                     EF_NONMODAL|EF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (EF_DELETEONLYFILEONCLOSE|EF_DISABLEHISTORY) : 0)
		                     |EF_ENABLE_F6,
		                     anStartLine, anStartChar,
		                     abDetectCP ? CP_AUTODETECT : 1200);
		lbRc = (iRc != EEC_OPEN_ERROR);
	}
	else
	{
		iRc = InfoW1900->Viewer(asFileName, pszTitle, 0,0,-1,-1,
		                     VF_NONMODAL|VF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (VF_DELETEONLYFILEONCLOSE|VF_DISABLEHISTORY) : 0)
		                     |VF_ENABLE_F6,
		                     abDetectCP ? CP_AUTODETECT : 1200);
		lbRc = (iRc != 0);
	}

	return lbRc;
}

bool CPluginW1900::ExecuteSynchroApi()
{
	if (!InfoW1900)
		return false;

	// получается более 2-х, если фар в данный момент чем-то занят (сканирует каталог?)
	//_ASSERTE(gnSynchroCount<=3);
	gnSynchroCount++;
	InfoW1900->AdvControl(&guid_ConEmu, ACTL_SYNCHRO, 0, NULL);
	return true;
}

static HANDLE ghSyncDlg = NULL;

void CPluginW1900::WaitEndSynchro()
{
	// Считаем, что в Far 3 починили
#if 0
	if ((gnSynchroCount == 0) || !(IS_SYNCHRO_ALLOWED))
		return;

	FarDialogItem items[] =
	{
		{DI_DOUBLEBOX,  3,  1,  51, 3, {0}, 0, 0, 0, GetMsgW1900(CEPluginName)},

		{DI_BUTTON,     0,  2,  0,  0, {0},  0, 0, DIF_FOCUS|DIF_CENTERGROUP|DIF_DEFAULTBUTTON, GetMsgW1900(CEStopSynchroWaiting)},
	};

	//GUID ConEmuWaitEndSynchro = { /* d0f369dc-2800-4833-a858-43dd1c115370 */
	//	    0xd0f369dc,
	//	    0x2800,
	//	    0x4833,
	//	    {0xa8, 0x58, 0x43, 0xdd, 0x1c, 0x11, 0x53, 0x70}
	//	  };

	ghSyncDlg = InfoW1900->DialogInit(&guid_ConEmu, &guid_ConEmuWaitEndSynchro,
			-1,-1, 55, 5, NULL, items, countof(items), 0, 0, NULL, 0);

	if (ghSyncDlg == INVALID_HANDLE_VALUE)
	{
		// Видимо, Фар в состоянии выхода (финальная выгрузка всех плагинов)
		// В этом случае, по идее, Synchro вызываться более не должно
		gnSynchroCount = 0; // так что просто сбросим счетчик
	}
	else
	{
		InfoW1900->DialogRun(ghSyncDlg);
		InfoW1900->DialogFree(ghSyncDlg);
	}

	ghSyncDlg = NULL;
#endif
}

void CPluginW1900::StopWaitEndSynchro()
{
	// Считаем, что в Far 3 починили
#if 0
	if (ghSyncDlg)
	{
		InfoW1900->SendDlgMessage(ghSyncDlg, DM_CLOSE, -1, 0);
	}
#endif
}

bool CPluginW1900::IsMacroActive()
{
	if (!InfoW1900 || !FarHwnd)
		return false;

	INT_PTR liRc = InfoW1900->MacroControl(&guid_ConEmu, MCTL_GETSTATE, 0, 0);

	if (liRc == MACROSTATE_NOMACRO)
		return false;

	return true;
}

int CPluginW1900::GetMacroArea()
{
	int nArea = (int)InfoW1900->MacroControl(&guid_ConEmu, MCTL_GETAREA, 0, 0);
	return nArea;
}


void CPluginW1900::RedrawAll()
{
	if (!InfoW1900 || !FarHwnd)
		return;

	InfoW1900->AdvControl(&guid_ConEmu, ACTL_REDRAWALL, 0, NULL);
}

#if 0
bool CPluginW1900::LoadPlugin(wchar_t* pszPluginPath)
{
	if (!InfoW1900) return false;

	InfoW1900->PluginsControl(INVALID_HANDLE_VALUE,PCTL_LOADPLUGIN,PLT_PATH,pszPluginPath);
	return true;
}
#endif

bool CPluginW1900::InputBox(LPCWSTR Title, LPCWSTR SubTitle, LPCWSTR HistoryName, LPCWSTR SrcText, wchar_t*& DestText)
{
	_ASSERTE(DestText==NULL);
	if (!InfoW1900)
		return false;

	wchar_t strTemp[MAX_PATH+1];
	if (!InfoW1900->InputBox(&guid_ConEmu, &guid_ConEmuInput, Title, SubTitle, HistoryName, SrcText, strTemp, countof(strTemp), NULL, FIB_BUTTONS))
		return false;
	DestText = lstrdup(strTemp).Detach();
	return true;
}

void CPluginW1900::ShowUserScreen(bool bUserScreen)
{
	if (!InfoW1900)
		return;

	if (bUserScreen)
		InfoW1900->PanelControl(INVALID_HANDLE_VALUE, FCTL_GETUSERSCREEN, 0, 0);
	else
		InfoW1900->PanelControl(INVALID_HANDLE_VALUE, FCTL_SETUSERSCREEN, 0, 0);
}

//static void FarPanel2CePanel(PanelInfo* pFar, CEFAR_SHORT_PANEL_INFO* pCE)
//{
//	pCE->PanelType = pFar->PanelType;
//	pCE->Plugin = ((pFar->Flags & PFLAGS_PLUGIN) == PFLAGS_PLUGIN);
//	pCE->PanelRect = pFar->PanelRect;
//	pCE->ItemsNumber = pFar->ItemsNumber;
//	pCE->SelectedItemsNumber = pFar->SelectedItemsNumber;
//	pCE->CurrentItem = pFar->CurrentItem;
//	pCE->TopPanelItem = pFar->TopPanelItem;
//	pCE->Visible = ((pFar->Flags & PFLAGS_VISIBLE) == PFLAGS_VISIBLE);
//	pCE->Focus = ((pFar->Flags & PFLAGS_FOCUS) == PFLAGS_FOCUS);
//	pCE->ViewMode = pFar->ViewMode;
//	pCE->ShortNames = ((pFar->Flags & PFLAGS_ALTERNATIVENAMES) == PFLAGS_ALTERNATIVENAMES);
//	pCE->SortMode = pFar->SortMode;
//	pCE->Flags = pFar->Flags;
//}

void CPluginW1900::LoadFarColors(BYTE (&nFarColors)[col_LastIndex])
{
	INT_PTR nColorSize = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, 0, NULL);
	FarColor* pColors = (FarColor*)calloc(nColorSize, sizeof(*pColors));
	if (pColors)
		nColorSize = InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, (int)nColorSize, pColors);
	WARNING("Поддержка более 4бит цветов");
	if (pColors && nColorSize > 0)
	{
#ifdef _DEBUG
		INT_PTR nDefColorSize = COL_LASTPALETTECOLOR;
		_ASSERTE(nColorSize==nDefColorSize);
#endif
		nFarColors[col_PanelText] = FarColor_3_2(pColors[COL_PANELTEXT]);
		nFarColors[col_PanelSelectedCursor] = FarColor_3_2(pColors[COL_PANELSELECTEDCURSOR]);
		nFarColors[col_PanelSelectedText] = FarColor_3_2(pColors[COL_PANELSELECTEDTEXT]);
		nFarColors[col_PanelCursor] = FarColor_3_2(pColors[COL_PANELCURSOR]);
		nFarColors[col_PanelColumnTitle] = FarColor_3_2(pColors[COL_PANELCOLUMNTITLE]);
		nFarColors[col_PanelBox] = FarColor_3_2(pColors[COL_PANELBOX]);
		nFarColors[col_HMenuText] = FarColor_3_2(pColors[COL_HMENUTEXT]);
		nFarColors[col_WarnDialogBox] = FarColor_3_2(pColors[COL_WARNDIALOGBOX]);
		nFarColors[col_DialogBox] = FarColor_3_2(pColors[COL_DIALOGBOX]);
		nFarColors[col_CommandLineUserScreen] = FarColor_3_2(pColors[COL_COMMANDLINEUSERSCREEN]);
		nFarColors[col_PanelScreensNumber] = FarColor_3_2(pColors[COL_PANELSCREENSNUMBER]);
		nFarColors[col_KeyBarNum] = FarColor_3_2(pColors[COL_KEYBARNUM]);
		nFarColors[col_EditorText] = FarColor_3_2(pColors[COL_EDITORTEXT]);
		nFarColors[col_ViewerText] = FarColor_3_2(pColors[COL_VIEWERTEXT]);
	}
	else
	{
		_ASSERTE(pColors && nColorSize > 0);
		memset(nFarColors, 7, countof(nFarColors)*sizeof(*nFarColors));
	}
	SafeFree(pColors);
}

static int GetFarSetting(HANDLE h, size_t Root, LPCWSTR Name)
{
	int nValue = 0;
	FarSettingsItem fsi = {Root, Name};
	if (InfoW1900->SettingsControl(h, SCTL_GET, 0, &fsi))
	{
		_ASSERTE(fsi.Type == FST_QWORD);
		nValue = (fsi.Number != 0);
	}
	else
	{
		_ASSERTE("InfoW1900->SettingsControl failed" && 0);
	}
	return nValue;
}

void CPluginW1900::LoadFarSettings(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel)
{
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	GUID FarGuid = {};
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
	if (InfoW1900->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc))
	{
		if (pInterface)
		{
			memset(pInterface, 0, sizeof(*pInterface));
			pInterface->AlwaysShowMenuBar = GetFarSetting(sc.Handle, FSSF_INTERFACE, L"ShowMenuBar");
			pInterface->ShowKeyBar = GetFarSetting(sc.Handle, FSSF_SCREEN, L"KeyBar");
		}

		if (pPanel)
		{
			memset(pPanel, 0, sizeof(*pPanel));
			pPanel->ShowColumnTitles = GetFarSetting(sc.Handle, FSSF_PANELLAYOUT, L"ColumnTitles");
			pPanel->ShowStatusLine = GetFarSetting(sc.Handle, FSSF_PANELLAYOUT, L"StatusLine");
			pPanel->ShowSortModeLetter = GetFarSetting(sc.Handle, FSSF_PANELLAYOUT, L"SortMode");
		}

		InfoW1900->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
}

bool CPluginW1900::GetFarRect(SMALL_RECT& rcFar)
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return false;

	ZeroStruct(rcFar);
	if (InfoW1900->AdvControl(&guid_ConEmu, ACTL_GETFARRECT, 0, &rcFar))
	{
		if (rcFar.Bottom > rcFar.Top)
		{
			return true;
		}
	}

	return false;
}

bool CPluginW1900::CheckPanelExist()
{
	if (!InfoW1900 || !InfoW1900->PanelControl)
		return false;

	INT_PTR iRc = InfoW1900->PanelControl(INVALID_HANDLE_VALUE, FCTL_CHECKPANELSEXIST, 0, 0);
	return (iRc!=0);
}

int CPluginW1900::GetActiveWindowType()
{
	if (!InfoW1900 || !InfoW1900->AdvControl)
		return -1;

	//_ASSERTE(GetCurrentThreadId() == gnMainThreadId); -- это - ThreadSafe

	INT_PTR nArea = InfoW1900->MacroControl(&guid_ConEmu, MCTL_GETAREA, 0, 0);

	switch (nArea)
	{
		case MACROAREA_SHELL:
		case MACROAREA_INFOPANEL:
		case MACROAREA_QVIEWPANEL:
		case MACROAREA_TREEPANEL:
			return WTYPE_PANELS;
		case MACROAREA_VIEWER:
			return WTYPE_VIEWER;
		case MACROAREA_EDITOR:
			return WTYPE_EDITOR;
		case MACROAREA_DIALOG:
		case MACROAREA_SEARCH:
		case MACROAREA_DISKS:
		case MACROAREA_FINDFOLDER:
		case MACROAREA_SHELLAUTOCOMPLETION:
		case MACROAREA_DIALOGAUTOCOMPLETION:
			return WTYPE_DIALOG;
		case MACROAREA_HELP:
			return WTYPE_HELP;
		case MACROAREA_MAINMENU:
		case MACROAREA_MENU:
		case MACROAREA_USERMENU:
			return WTYPE_VMENU;
		case MACROAREA_OTHER: // Grabber
			return -1;
	}

	// Сюда мы попасть не должны, все макрообласти должны быть учтены в switch
	_ASSERTE(nArea==MACROAREA_SHELL);
	return -1;
}

LPCWSTR CPluginW1900::GetWindowTypeName(int WindowType)
{
	LPCWSTR pszCurType;
	switch (WindowType)
	{
		case WTYPE_PANELS: pszCurType = L"WTYPE_PANELS"; break;
		case WTYPE_VIEWER: pszCurType = L"WTYPE_VIEWER"; break;
		case WTYPE_EDITOR: pszCurType = L"WTYPE_EDITOR"; break;
		case WTYPE_DIALOG: pszCurType = L"WTYPE_DIALOG"; break;
		case WTYPE_VMENU:  pszCurType = L"WTYPE_VMENU"; break;
		case WTYPE_HELP:   pszCurType = L"WTYPE_HELP"; break;
		default:           pszCurType = L"Unknown";
	}
	return pszCurType;
}

//static LONG_PTR WINAPI CallGuiMacroDlg(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2);
//HANDLE CreateGuiMacroDlg(int height, FarDialogItem *items, int nCount)
//{
//	static const GUID ConEmuCallGuiMacro = { /* 21f61504-b40f-45e3-9d27-84db16bc9c22 */
//		    0x21f61504,
//		    0xb40f,
//		    0x45e3,
//		    {0x9d, 0x27, 0x84, 0xdb, 0x16, 0xbc, 0x9c, 0x22}
//		  };
//
//	HANDLE hDlg = InfoW1900->DialogInitW1900(&guid_ConEmu, ConEmuCallGuiMacro,
//									-1, -1, 76, height,
//									NULL/*L"Configure"*/, items, nCount,
//									0, 0/*Flags*/, (FARWINDOWPROC)CallGuiMacroDlg, 0);
//	return hDlg;
//}

#define FAR_UNICODE 1867
#include "Dialogs.h"
void CPluginW1900::GuiMacroDlg()
{
	CallGuiMacroProc();
}

HANDLE CPluginW1900::Open(const void* apInfo)
{
	const struct OpenInfo *Info = (const struct OpenInfo*)apInfo;

	if (!mb_StartupInfoOk)
		return NULL;

	INT_PTR Item = Info->Data;
	if ((Info->OpenFrom & OPEN_FROM_MASK) == OPEN_FROMMACRO)
	{
		Item = 0; // Сразу сброс
		OpenMacroInfo* p = (OpenMacroInfo*)Info->Data;
		if (p->StructSize >= sizeof(*p))
		{
			if (p->Count > 0)
			{
				switch (p->Values[0].Type)
				{
				case FMVT_INTEGER:
					Item = (INT_PTR)p->Values[0].Integer; break;
				// Far 3 Lua macros uses Double instead of Int :( Unlock in Far2 for uniform.
				case FMVT_DOUBLE:
					Item = (INT_PTR)p->Values[0].Double; break;
				case FMVT_STRING:
					_ASSERTE(p->Values[0].String!=NULL);
					Item = (INT_PTR)p->Values[0].String; break;
				default:
					_ASSERTE(p->Values[0].Type==FMVT_INTEGER || p->Values[0].Type==FMVT_STRING);
				}

				if (Item == CE_CALLPLUGIN_REQ_DIRS)
				{
					if (p->Count == 3)
					{
						LPCWSTR pszActive = (p->Values[1].Type == FMVT_STRING) ? p->Values[1].String : NULL;
						LPCWSTR pszPassive = (p->Values[2].Type == FMVT_STRING) ? p->Values[2].String : NULL;
						StorePanelDirs(pszActive, pszPassive);
					}
					return PANEL_NONE;
				}
			}
		}
		else
		{
			_ASSERTE(p->StructSize >= sizeof(*p));
		}
	}

	HANDLE h = OpenPluginCommon(Info->OpenFrom, Item, (Info->OpenFrom == OPEN_FROMMACRO));
	if ((Info->OpenFrom & OPEN_FROM_MASK) == OPEN_FROMMACRO)
	{
		h = (HANDLE)(h != NULL);
	}
	else if ((h == INVALID_HANDLE_VALUE) || (h == (HANDLE)-2))
	{
		if ((Info->OpenFrom & OPEN_FROM_MASK) == OPEN_ANALYSE)
			h = PANEL_STOP;
		else
			h = NULL;
	}

	return h;
}

#if 0
int WINAPI ProcessConsoleInputW1900(void* apInfo)
{
	struct ProcessConsoleInputInfo *Info = (struct ProcessConsoleInputInfo*)apInfo;

if 0
	// Чтобы можно было "нормально" работать в Far3 и без хуков
	BOOL bMainThread = TRUE; // раз вызов через API - значит MainThread
	BOOL lbRc = FALSE;
	HANDLE hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
	const INPUT_RECORD* lpBuffer = Info->Rec;
	DWORD nLength = 1; LPDWORD lpNumberOfEventsRead = &nLength;
	_SETARGS4(&lbRc,hConsoleInput,lpBuffer,nLength,lpNumberOfEventsRead);

	if (!OnConsoleReadInputWork(&args) || (nLength == 0))
		return 1;

	OnConsoleReadInputPost(&args);
endif

	return 0;
}
#endif
