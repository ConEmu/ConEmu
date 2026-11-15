
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
#include "../common/pluginW2800.hpp" // Far3
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/plugin_helper.h"
#include "PluginHeader.h"
#include "../ConEmu/version.h"
#include "../common/farcolor3.hpp"
#include "../common/ConEmuColors3.h"
#include "../common/EnvVar.h"
#include "../common/MArray.h"
#include "../common/WThreads.h"
#include "../ConEmuHk/ConEmuHooks.h"

#include "ConEmuPlugin2800.h"

#ifdef _DEBUG
//#define SHOW_DEBUG_EVENTS
#endif

GUID guid_ConEmu = { /* 4b675d80-1d4a-4ea9-8436-fdc23f2fc14b */
	0x4b675d80,
	0x1d4a,
	0x4ea9,
	{0x84, 0x36, 0xfd, 0xc2, 0x3f, 0x2f, 0xc1, 0x4b}
};
GUID guid_ConEmuPluginItems = { /* 3836ad1f-5130-4a13-93d8-15fefbdc3185 */
	0x3836ad1f,
	0x5130,
	0x4a13,
	{0x93, 0xd8, 0x15, 0xfe, 0xfb, 0xdc, 0x31, 0x85}
};
GUID guid_ConEmuPluginMenu = { /* 830d40da-ccf3-417b-b378-87f9441c4c95 */
	0x830d40da,
	0xccf3,
	0x417b,
	{0xb3, 0x78, 0x87, 0xf9, 0x44, 0x1c, 0x4c, 0x95}
};
GUID guid_ConEmuGuiMacroDlg = { /* a0484f91-a800-4e3a-abac-aed4485da79d */
	0xa0484f91,
	0xa800,
	0x4e3a,
	{0xab, 0xac, 0xae, 0xd4, 0x48, 0x5d, 0xa7, 0x9d}
};
GUID guid_ConEmuWaitEndSynchro = { /* e93fba92-d7de-4651-9be1-c9b064254f65 */
	0xe93fba92,
	0xd7de,
	0x4651,
	{0x9b, 0xe1, 0xc9, 0xb0, 0x64, 0x25, 0x4f, 0x65}
};
GUID guid_ConEmuMsg = { /* aba0df6c-163f-4950-9029-a3f595c1c351 */
	0xaba0df6c,
	0x163f,
	0x4950,
	{0x90, 0x29, 0xa3, 0xf5, 0x95, 0xc1, 0xc3, 0x51}
};
GUID guid_ConEmuInput = { /* 78ba0189-7dd7-4cb9-aff8-c70bca9f9cb6 */
	0x78ba0189,
	0x7dd7,
	0x4cb9,
	{0xaf, 0xf8, 0xc7, 0x0b, 0xca, 0x9f, 0x9c, 0xb6}
};
GUID guid_ConEmuMenu = { /* 2dc6b821-fd8e-4165-adcf-a4eda7b44e8e */
    0x2dc6b821,
    0xfd8e,
    0x4165,
    {0xad, 0xcf, 0xa4, 0xed, 0xa7, 0xb4, 0x4e, 0x8e}
};

struct PluginStartupInfo *InfoW2800=NULL;
struct FarStandardFunctions *FSFW2800=NULL;

static MArray<WindowInfo>* pwList = NULL;

/* EXPORTS BEGIN */
void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	_ASSERTE(Info->StructSize >= (size_t)((LPBYTE)(&Info->Instance) - (LPBYTE)(Info)));

	if (gFarVersion.dwBuild >= 2800)
		Info->MinFarVersion = FARMANAGERVERSION;
	else
		Info->MinFarVersion = MAKEFARVERSION(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, MIN_FAR3_BUILD, FARMANAGERVERSION_STAGE);

	// Build: YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10),VS_RELEASE);

	Info->Guid = guid_ConEmu;
	Info->Title = L"ConEmu";
	Info->Description = L"ConEmu support for Far Manager";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}
/* EXPORTS END */

CPluginW2800::CPluginW2800()
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
	wt_Desktop = WTYPE_DESKTOP;
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

wchar_t* CPluginW2800::GetPanelDir(GetPanelDirFlags Flags, wchar_t* pszBuffer /*= NULL*/, int cchBufferMax /*= 0*/)
{
	wchar_t* pszDir = NULL;
	HANDLE hPanel = (Flags & gpdf_Active) ? PANEL_ACTIVE : PANEL_PASSIVE;
	size_t nSize;
	PanelInfo pi = {sizeof(pi)};

	if (!InfoW2800)
		goto wrap;

	nSize = InfoW2800->PanelControl(hPanel, FCTL_GETPANELINFO, 0, &pi);

	if ((Flags & gpdf_NoHidden) && !(pi.Flags & PFLAGS_VISIBLE))
		goto wrap;
	if ((Flags & gpdf_NoPlugin) && (pi.Flags & PFLAGS_PLUGIN))
		goto wrap;

	nSize = InfoW2800->PanelControl(hPanel, FCTL_GETPANELDIRECTORY, 0, 0);

	if (nSize)
	{
		FarPanelDirectory* pDir = (FarPanelDirectory*)calloc(nSize, 1);
		if (pDir)
		{
			pDir->StructSize = sizeof(*pDir);
			nSize = InfoW2800->PanelControl(hPanel, FCTL_GETPANELDIRECTORY, nSize, pDir);
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

bool CPluginW2800::GetPanelInfo(GetPanelDirFlags Flags, CEPanelInfo* pInfo)
{
	if (!InfoW2800 || !InfoW2800->PanelControl)
		return false;

	HANDLE hPanel = (Flags & gpdf_Active) ? PANEL_ACTIVE : PANEL_PASSIVE;
	PanelInfo pasv = {sizeof(pasv)}, actv = {sizeof(actv)};
	PanelInfo* p;

	if (Flags & (gpdf_Left|gpdf_Right))
	{
		InfoW2800->PanelControl(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, &actv);
		InfoW2800->PanelControl(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, &pasv);
		PanelInfo* pLeft = (actv.Flags & PFLAGS_PANELLEFT) ? &actv : &pasv;
		PanelInfo* pRight = (actv.Flags & PFLAGS_PANELLEFT) ? &pasv : &actv;
		p = (Flags & gpdf_Left) ? pLeft : pRight;
		hPanel = ((p->Flags & PFLAGS_FOCUS) == PFLAGS_FOCUS) ? PANEL_ACTIVE : PANEL_PASSIVE;
	}
	else
	{
		hPanel = (Flags & gpdf_Active) ? PANEL_ACTIVE : PANEL_PASSIVE;
		InfoW2800->PanelControl(hPanel, FCTL_GETPANELINFO, 0, &actv);
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

bool CPluginW2800::GetPanelItemInfo(const CEPanelInfo& PnlInfo, bool bSelected, INT_PTR iIndex, WIN32_FIND_DATAW& Info, wchar_t** ppszFullPathName)
{
	if (!InfoW2800 || !InfoW2800->PanelControl)
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

	FarGetPluginPanelItem gppi = {sizeof(gppi), static_cast<size_t>(sz), pItem};
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

INT_PTR CPluginW2800::PanelControlApi(HANDLE hPanel, int Command, INT_PTR Param1, void* Param2)
{
	if (!InfoW2800 || !InfoW2800->PanelControl)
		return -1;
	INT_PTR iRc = InfoW2800->PanelControl(hPanel, (FILE_CONTROL_COMMANDS)Command, Param1, Param2);
	return iRc;
}

void CPluginW2800::GetPluginInfoPtr(void *piv)
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

void CPluginW2800::SetStartupInfoPtr(void *aInfo)
{
	INIT_FAR_PSI(::InfoW2800, ::FSFW2800, (PluginStartupInfo*)aInfo);
	mb_StartupInfoOk = true;

	VersionInfo FarVer = {0};
	if (InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETFARMANAGERVERSION, 0, &FarVer))
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

DWORD CPluginW2800::GetEditorModifiedState()
{
	EditorInfo ei = { sizeof(EditorInfo) };
	InfoW2800->EditorControl(-1/*Active editor*/, ECTL_GETINFO, 0, &ei);
#ifdef SHOW_DEBUG_EVENTS
	char szDbg[255];
	wsprintfA(szDbg, "Editor:State=%i\n", ei.CurState);
	OutputDebugStringA(szDbg);
#endif
	// Если он сохранен, то уже НЕ модифицирован
	DWORD currentModifiedState = ((ei.CurState & (ECSTATE_MODIFIED|ECSTATE_SAVED)) == ECSTATE_MODIFIED) ? 1 : 0;
	return currentModifiedState;
}

int CPluginW2800::ProcessEditorEventPtr(void* p)
{
	const ProcessEditorEventInfo* Info = (const ProcessEditorEventInfo*)p;
	return ProcessEditorViewerEvent(Info->Event, -1);
}

int CPluginW2800::ProcessViewerEventPtr(void* p)
{
	const ProcessViewerEventInfo* Info = (const ProcessViewerEventInfo*)p;
	return ProcessEditorViewerEvent(-1, Info->Event);
}

int CPluginW2800::ProcessSynchroEventPtr(void* p)
{
	const ProcessSynchroEventInfo* Info = (const ProcessSynchroEventInfo*)p;
	return Plugin()->ProcessSynchroEvent(Info->Event, Info->Param);
}

// watch non-modified -> modified editor status change
int CPluginW2800::ProcessEditorInputPtr(LPCVOID aRec)
{
	if (!InfoW2800)
		return 0;

	const ProcessEditorInputInfo *apInfo = (const ProcessEditorInputInfo*)aRec;
	ProcessEditorInputInternal(apInfo->Rec);

	return 0;
}

int CPluginW2800::GetWindowCount()
{
	if (!InfoW2800 || !InfoW2800->AdvControl)
		return 0;

	INT_PTR windowCount = InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETWINDOWCOUNT, 0, NULL);
	return (int)windowCount;
}

static INT_PTR WExists(const WindowInfo& C, const MArray<WindowInfo>& wList)
{
	for (INT_PTR j = 0; j < wList.size(); j++)
	{
		const WindowInfo& L = wList[j];
		if (L.Type != C.Type)
			continue;
		if ((L.Type != WTYPE_PANELS) && (L.Id != C.Id))
			continue;
		return j;
	}

	return -1;
}

static INT_PTR WLoadWindows(MArray<WindowInfo>& wCurrent, int windowCount)
{
	WindowInfo WInfo;
	wCurrent.clear();

	// Load window list
	for (int i = 0; i < windowCount; i++)
	{
		ZeroStruct(WInfo);
		WInfo.StructSize = sizeof(WInfo);
		WInfo.Pos = i;
		if (!InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, 0, &WInfo))
			continue;
		if (WInfo.Type != WTYPE_EDITOR && WInfo.Type != WTYPE_VIEWER && WInfo.Type != WTYPE_PANELS)
			continue;

		if (WInfo.Type == WTYPE_PANELS)
		{
			if ((wCurrent.size() > 0) && (wCurrent[0].Type == WTYPE_PANELS))
				wCurrent[0] = WInfo;
			else
				wCurrent.insert(0, WInfo);
		}
		else
		{
			wCurrent.push_back(WInfo);
		}
	}

	return wCurrent.size();
}

bool CPluginW2800::UpdateConEmuTabsApi(int windowCount)
{
	if (!InfoW2800 || !InfoW2800->AdvControl || gbIgnoreUpdateTabs)
		return false;

	bool lbCh = false;
	WindowInfo WInfo = {sizeof(WindowInfo)};
	wchar_t szWNameBuffer[CONEMUTABMAX];
	int tabCount = 0;
	bool lbActiveFound = false;

	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);

	WindowInfo WActive = {sizeof(WActive)};
	WActive.Pos = -1;
	bool bActiveInfo = InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, 0, &WActive)!=0;
	// Если фар запущен с ключом "/e" (как standalone редактор) - будет ассерт при первой попытке
	// считать информацию об окне (редактор еще не создан?, а панелей вообще нет)
	_ASSERTE(bActiveInfo && (WActive.Flags & WIF_CURRENT));
	static WindowInfo WLastActive;

	if (!pwList)
		pwList = new MArray<WindowInfo>();

	// Another weird Far API breaking change. How more?..
	MArray<WindowInfo> wCurrent;
	// Load window list
	WLoadWindows(wCurrent, windowCount);

	// Clear closed windows
	for (INT_PTR i = 0; i < pwList->size();)
	{
		const WindowInfo& L = (*pwList)[i];

		INT_PTR iFound = WExists(L, wCurrent);

		if (iFound < 0)
			pwList->erase(i);
		else
			i++;
	}
	// Add new windows
	for (INT_PTR i = 0; i < wCurrent.size(); i++)
	{
		const WindowInfo& C = wCurrent[i];

		INT_PTR iFound = WExists(C, *pwList);

		if (iFound >= 0)
		{
			(*pwList)[iFound] = C;
		}
		else
		{
			if (C.Type == WTYPE_PANELS)
			{
				if ((pwList->size() > 0) && ((*pwList)[0].Type == WTYPE_PANELS))
					(*pwList)[0] = C;
				else
					pwList->insert(0, C);
			}
			else
			{
				pwList->push_back(C);
			}
		}
	}
	// And check the count
	windowCount = pwList->size();

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
			WInfo = (*pwList)[i];
			_ASSERTE(WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS);

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
		WInfo = (*pwList)[i];

		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			WInfo.Name = szWNameBuffer;
			WInfo.NameSize = CONEMUTABMAX;
			InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETWINDOWINFO, 0, &WInfo);
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
				lbCh |= AddTab(tabCount, i, false/*losingFocus*/, false/*editorSave*/,
				               WInfo.Type, WInfo.Name, /*editorSave ? ei.FileName :*/ NULL,
				               (WInfo.Flags & WIF_CURRENT), (WInfo.Flags & WIF_MODIFIED), (WInfo.Flags & WIF_MODAL),
							   WInfo.Id);
			}
		}
	}

	bool bHasPanels = this->CheckPanelExist();

	if (!lbActiveFound)
	{
		// Порядок инициализации поменялся, при запуске "far /e ..." редактора сначала вообще "нет".
		_ASSERTE((!bHasPanels && windowCount==0 && bActiveInfo && WActive.Type == WTYPE_DESKTOP) && "Active window must be detected already!");

		if (tabCount == 0)
		{
			// Добавить в табы хоть что-то
			lbCh |= AddTab(tabCount, 0, false/*losingFocus*/, false/*editorSave*/,
					               WTYPE_PANELS, L"far", /*editorSave ? ei.FileName :*/ NULL,
					               1/*Current*/, 0/*Modified*/, 1/*Modal*/, 0);
		}

		if (tabCount > 0)
		{
			gpTabs->Tabs.CurrentType = gnCurrentWindowType = gpTabs->Tabs.tabs[tabCount-1].Type;
		}
		else
		{
			_ASSERTE(tabCount>0);
		}
	}

	// 101224 - сразу запомнить количество!
	AddTabFinish(tabCount);

	return lbCh;
}

void CPluginW2800::ExitFar()
{
	if (!mb_StartupInfoOk)
		return;

	CPluginBase::ShutdownPluginStep(L"ExitFARW2800");

	WaitEndSynchro();

	if (InfoW2800)
	{
		free(InfoW2800);
		InfoW2800=NULL;
	}

	if (FSFW2800)
	{
		free(FSFW2800);
		FSFW2800=NULL;
	}

	CPluginBase::ShutdownPluginStep(L"ExitFARW2800 - done");
}

int CPluginW2800::ShowMessage(LPCWSTR asMsg, int aiButtons, bool bWarning)
{
	if (!InfoW2800 || !InfoW2800->Message)
		return -1;

	return InfoW2800->Message(&guid_ConEmu, &guid_ConEmuMsg,
					FMSG_ALLINONE1900|(aiButtons?0:FMSG_MB_OK)|(bWarning ? FMSG_WARNING : 0), NULL,
					(const wchar_t * const *)asMsg, 0, aiButtons);
}

LPCWSTR CPluginW2800::GetMsg(int aiMsg, wchar_t* psMsg /*= NULL*/, size_t cchMsgMax /*= 0*/)
{
	LPCWSTR pszRc = (InfoW2800 && InfoW2800->GetMsg) ? InfoW2800->GetMsg(&guid_ConEmu, aiMsg) : L"";
	if (!pszRc)
		pszRc = L"";
	if (psMsg)
		lstrcpyn(psMsg, pszRc, cchMsgMax);
	return pszRc;
}

//void ReloadMacroW2800()
//{
//	if (!InfoW2800 || !InfoW2800->AdvControl)
//		return;
//
//	ActlKeyMacro command;
//	command.Command=MCMD_LOADALL;
//	InfoW2800->AdvControl(&guid_ConEmu,ACTL_KEYMACRO,&command);
//}

void CPluginW2800::SetWindow(int nTab)
{
	if (!InfoW2800 || !InfoW2800->AdvControl)
		return;

	// We have to find **current** window ID because Far entangles them constantly
	INT_PTR iNewPos = -1;
	if (pwList && (nTab >= 0) && (nTab < pwList->size()))
	{
		MArray<WindowInfo> wCurrent;
		// Load window list
		WLoadWindows(wCurrent, GetWindowCount());
		INT_PTR i = WExists((*pwList)[nTab], wCurrent);
		if (i >= 0)
			iNewPos = wCurrent[i].Pos;
	}

	if ((iNewPos >= 0)
		&& InfoW2800->AdvControl(&guid_ConEmu, ACTL_SETCURRENTWINDOW, iNewPos, NULL))
		InfoW2800->AdvControl(&guid_ConEmu, ACTL_COMMIT, 0, 0);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void CPluginW2800::PostMacroApi(const wchar_t* asMacro, INPUT_RECORD* apRec, bool abShowParseErrors)
{
	if (!InfoW2800 || !InfoW2800->AdvControl)
		return;

	MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
	//mcr.Flags = 0; // По умолчанию - вывод на экран разрешен
	bool bEnableOutput = true;

	while ((asMacro[0] == L'@' || asMacro[0] == L'^') && asMacro[1] && asMacro[1] != L' ')
	{
		switch (*asMacro)
		{
		case L'@':
			bEnableOutput = false;
			break;
		case L'^':
			mcr.Flags |= KMFLAGS_NOSENDKEYSTOPLUGINS;
			break;
		}
		asMacro++;
	}

	if (bEnableOutput)
		mcr.Flags |= KMFLAGS_ENABLEOUTPUT;

	// This macro was not adopted to Lua?
	_ASSERTE(*asMacro && *asMacro != L'$');

	// Вообще говоря, если тут попадается макрос в старом формате - то мы уже ничего не сделаем...
	// Начиная с Far 3 build 2851 - все макросы переведены на Lua

	mcr.SequenceText = asMacro;
	if (apRec)
		mcr.AKey = *apRec;

	mcr.Flags |= KMFLAGS_SILENTCHECK;

	if (!InfoW2800->MacroControl(&guid_ConEmu, MCTL_SENDSTRING, MSSC_CHECK, &mcr))
	{
		if (abShowParseErrors)
		{
			wchar_t* pszErrText = NULL;
			size_t iRcSize = InfoW2800->MacroControl(&guid_ConEmu, MCTL_GETLASTERROR, 0, NULL);
			MacroParseResult* Result = iRcSize ? (MacroParseResult*)calloc(iRcSize,1) : NULL;
			if (Result)
			{
				Result->StructSize = sizeof(*Result);
				_ASSERTE(FALSE && "Check MCTL_GETLASTERROR");
				InfoW2800->MacroControl(&guid_ConEmu, MCTL_GETLASTERROR, iRcSize, Result);

				size_t cchMax = (Result->ErrSrc ? lstrlen(Result->ErrSrc) : 0) + lstrlen(asMacro) + 255;
				pszErrText = (wchar_t*)malloc(cchMax*sizeof(wchar_t));
				_wsprintf(pszErrText, SKIPLEN(cchMax)
					L"Error in Macro. Far %u.%u build %u r%u\n"
					L"ConEmu plugin %02u%02u%02u%s[%u] {2800}\n"
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
					L"ConEmu plugin %02u%02u%02u%s[%u] {2800}\n"
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
		InfoW2800->MacroControl(&guid_ConEmu, MCTL_SENDSTRING, 0, &mcr);
	}
}

int CPluginW2800::ShowPluginMenu(ConEmuPluginMenuItem* apItems, int Count, int TitleMsgId /*= CEPluginName*/)
{
	if (!InfoW2800)
		return -1;

	//FarMenuItem items[] =
	//{
	//	{ConEmuHwnd ? MIF_SELECTED : MIF_DISABLE,  InfoW2800->GetMsg(&guid_ConEmu,CEMenuEditOutput)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW2800->GetMsg(&guid_ConEmu,CEMenuViewOutput)},
	//	{MIF_SEPARATOR},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW2800->GetMsg(&guid_ConEmu,CEMenuShowHideTabs)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW2800->GetMsg(&guid_ConEmu,CEMenuNextTab)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW2800->GetMsg(&guid_ConEmu,CEMenuPrevTab)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW2800->GetMsg(&guid_ConEmu,CEMenuCommitTab)},
	//	{MIF_SEPARATOR},
	//	{0,                                        InfoW2800->GetMsg(&guid_ConEmu,CEMenuGuiMacro)},
	//	{MIF_SEPARATOR},
	//	{ConEmuHwnd||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED,  InfoW2800->GetMsg(&guid_ConEmu,CEMenuAttach)},
	//	{MIF_SEPARATOR},
	//	//#ifdef _DEBUG
	//	//{0, L"&~. Raise exception"},
	//	//#endif
	//	{IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0,    InfoW2800->GetMsg(&guid_ConEmu,CEMenuDebug)},
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
		items[i].Text = apItems[i].MsgText ? apItems[i].MsgText : InfoW2800->GetMsg(&guid_ConEmu, apItems[i].MsgID);
	}

	int nRc = InfoW2800->Menu(&guid_ConEmu, &guid_ConEmuMenu, -1,-1, 0,
	                         FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                         InfoW2800->GetMsg(&guid_ConEmu,TitleMsgId),
	                         NULL, NULL, NULL, NULL, (FarMenuItem*)items, Count);
	SafeFree(items);
	return nRc;
}

bool CPluginW2800::OpenEditor(LPCWSTR asFileName, bool abView, bool abDeleteTempFile, bool abDetectCP /*= false*/, int anStartLine /*= 0*/, int anStartChar /*= 1*/)
{
	if (!InfoW2800)
		return false;

	bool lbRc;
	INT_PTR iRc;

	LPCWSTR pszTitle = abDeleteTempFile ? InfoW2800->GetMsg(&guid_ConEmu,CEConsoleOutput) : NULL;

	if (!abView)
	{
		iRc = InfoW2800->Editor(asFileName, pszTitle, 0,0,-1,-1,
		                     EF_NONMODAL|EF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (EF_DELETEONLYFILEONCLOSE|EF_DISABLEHISTORY) : 0)
		                     |EF_ENABLE_F6,
		                     anStartLine, anStartChar,
		                     abDetectCP ? CP_DEFAULT : 1200);
		lbRc = (iRc != EEC_OPEN_ERROR);
	}
	else
	{
		iRc = InfoW2800->Viewer(asFileName, pszTitle, 0,0,-1,-1,
		                     VF_NONMODAL|VF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (VF_DELETEONLYFILEONCLOSE|VF_DISABLEHISTORY) : 0)
		                     |VF_ENABLE_F6,
		                     abDetectCP ? CP_DEFAULT : 1200);
		lbRc = (iRc != 0);
	}

	return lbRc;
}

bool CPluginW2800::ExecuteSynchroApi()
{
	if (!InfoW2800)
		return false;

	// получается более 2-х, если фар в данный момент чем-то занят (сканирует каталог?)
	//_ASSERTE(gnSynchroCount<=3);
	gnSynchroCount++;
	InfoW2800->AdvControl(&guid_ConEmu, ACTL_SYNCHRO, 0, NULL);
	return true;
}

static HANDLE ghSyncDlg = NULL;

void CPluginW2800::WaitEndSynchro()
{
	// Считаем, что в Far 3 починили
#if 0
	if ((gnSynchroCount == 0) || !(IS_SYNCHRO_ALLOWED))
		return;

	FarDialogItem items[] =
	{
		{DI_DOUBLEBOX,  3,  1,  51, 3, {0}, 0, 0, 0, GetMsgW2800(CEPluginName)},

		{DI_BUTTON,     0,  2,  0,  0, {0},  0, 0, DIF_FOCUS|DIF_CENTERGROUP|DIF_DEFAULTBUTTON, GetMsgW2800(CEStopSynchroWaiting)},
	};

	//GUID ConEmuWaitEndSynchro = { /* d0f369dc-2800-4833-a858-43dd1c115370 */
	//	    0xd0f369dc,
	//	    0x2800,
	//	    0x4833,
	//	    {0xa8, 0x58, 0x43, 0xdd, 0x1c, 0x11, 0x53, 0x70}
	//	  };

	ghSyncDlg = InfoW2800->DialogInit(&guid_ConEmu, &guid_ConEmuWaitEndSynchro,
			-1,-1, 55, 5, NULL, items, countof(items), 0, 0, NULL, 0);

	if (ghSyncDlg == INVALID_HANDLE_VALUE)
	{
		// Видимо, Фар в состоянии выхода (финальная выгрузка всех плагинов)
		// В этом случае, по идее, Synchro вызываться более не должно
		gnSynchroCount = 0; // так что просто сбросим счетчик
	}
	else
	{
		InfoW2800->DialogRun(ghSyncDlg);
		InfoW2800->DialogFree(ghSyncDlg);
	}

	ghSyncDlg = NULL;
#endif
}

void CPluginW2800::StopWaitEndSynchro()
{
	// Считаем, что в Far 3 починили
#if 0
	if (ghSyncDlg)
	{
		InfoW2800->SendDlgMessage(ghSyncDlg, DM_CLOSE, -1, 0);
	}
#endif
}

bool CPluginW2800::IsMacroActive()
{
	if (!InfoW2800 || !FarHwnd)
		return false;

	INT_PTR liRc = InfoW2800->MacroControl(&guid_ConEmu, MCTL_GETSTATE, 0, 0);

	if (liRc == MACROSTATE_NOMACRO)
		return false;

	return true;
}

int CPluginW2800::GetMacroArea()
{
	int nArea = (int)InfoW2800->MacroControl(&guid_ConEmu, MCTL_GETAREA, 0, 0);
	return nArea;
}


void CPluginW2800::RedrawAll()
{
	if (!InfoW2800 || !FarHwnd)
		return;

	InfoW2800->AdvControl(&guid_ConEmu, ACTL_REDRAWALL, 0, NULL);
}

#if 0
bool LoadPluginW2800(wchar_t* pszPluginPath)
{
	if (!InfoW2800) return false;

	InfoW2800->PluginsControl(INVALID_HANDLE_VALUE,PCTL_LOADPLUGIN,PLT_PATH,pszPluginPath);
	return true;
}
#endif

bool CPluginW2800::InputBox(LPCWSTR Title, LPCWSTR SubTitle, LPCWSTR HistoryName, LPCWSTR SrcText, wchar_t*& DestText)
{
	_ASSERTE(DestText==NULL);
	if (!InfoW2800)
		return false;

	wchar_t strTemp[MAX_PATH+1];
	if (!InfoW2800->InputBox(&guid_ConEmu, &guid_ConEmuInput, Title, SubTitle, HistoryName, SrcText, strTemp, countof(strTemp), NULL, FIB_BUTTONS))
		return false;
	DestText = lstrdup(strTemp).Detach();
	return true;
}

void CPluginW2800::ShowUserScreen(bool bUserScreen)
{
	if (!InfoW2800)
		return;

	if (bUserScreen)
		InfoW2800->PanelControl(INVALID_HANDLE_VALUE, FCTL_GETUSERSCREEN, 0, 0);
	else
		InfoW2800->PanelControl(INVALID_HANDLE_VALUE, FCTL_SETUSERSCREEN, 0, 0);
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

void CPluginW2800::LoadFarColors(BYTE (&nFarColors)[col_LastIndex])
{
	INT_PTR nColorSize = InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, 0, NULL);
	FarColor* pColors = (FarColor*)calloc(nColorSize, sizeof(*pColors));
	if (pColors)
		nColorSize = InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETARRAYCOLOR, (int)nColorSize, pColors);
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
	FarSettingsItem fsi = {sizeof(fsi), Root, Name};
	if (InfoW2800->SettingsControl(h, SCTL_GET, 0, &fsi))
	{
		_ASSERTE(fsi.Type == FST_QWORD);
		nValue = (fsi.Number != 0);
	}
	else
	{
		_ASSERTE("InfoW2800->SettingsControl failed" && 0);
	}
	return nValue;
}

void CPluginW2800::LoadFarSettings(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel)
{
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	GUID FarGuid = {};
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), FarGuid, INVALID_HANDLE_VALUE};
	if (InfoW2800->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc))
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

		InfoW2800->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
}

bool CPluginW2800::GetFarRect(SMALL_RECT& rcFar)
{
	if (!InfoW2800 || !InfoW2800->AdvControl)
		return false;

	ZeroStruct(rcFar);
	if (InfoW2800->AdvControl(&guid_ConEmu, ACTL_GETFARRECT, 0, &rcFar))
	{
		if (rcFar.Bottom > rcFar.Top)
		{
			return true;
		}
	}

	return false;
}

bool CPluginW2800::CheckPanelExist()
{
	if (!InfoW2800 || !InfoW2800->PanelControl)
		return false;

	INT_PTR iRc = InfoW2800->PanelControl(PANEL_NONE, FCTL_CHECKPANELSEXIST, 0, 0);
	return (iRc!=0);
}

int CPluginW2800::GetActiveWindowType()
{
	if (!InfoW2800 || !InfoW2800->AdvControl)
		return -1;

	//_ASSERTE(GetCurrentThreadId() == gnMainThreadId); -- это - ThreadSafe

	INT_PTR nArea = InfoW2800->MacroControl(&guid_ConEmu, MCTL_GETAREA, 0, 0);

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
		case 18: // MACROAREA_LAST
			//_ASSERTE(FALSE && "MACROAREA_LAST must not be returned");
			// Длительная операция, которую можно прервать по Esc
			return WTYPE_DIALOG;
	}

	// Сюда мы попасть не должны, все макрообласти должны быть учтены в switch
	_ASSERTE(nArea==MACROAREA_SHELL);
	return -1;
}

LPCWSTR CPluginW2800::GetWindowTypeName(int WindowType)
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
//	HANDLE hDlg = InfoW2800->DialogInitW2800(&guid_ConEmu, ConEmuCallGuiMacro,
//									-1, -1, 76, height,
//									NULL/*L"Configure"*/, items, nCount,
//									0, 0/*Flags*/, (FARWINDOWPROC)CallGuiMacroDlg, 0);
//	return hDlg;
//}

#define FAR_UNICODE 1867
#include "Dialogs.h"
void CPluginW2800::GuiMacroDlg()
{
	CallGuiMacroProc();
}

static void WINAPI FreeMacroResult(void *CallbackData, struct FarMacroValue *Values, size_t Count)
{
	// Comes from CPluginW2800::Open
	wchar_t* psz = (wchar_t*)Values[0].String;
	free(psz);
}

HANDLE CPluginW2800::Open(const void* apInfo)
{
	const struct OpenInfo *Info = (const struct OpenInfo*)apInfo;

	if (!mb_StartupInfoOk)
		return NULL;

	INT_PTR Item = Info->Data;
	bool bGuiMacroCall = false;

	if (Info->OpenFrom == OPEN_FROMMACRO)
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
				// Far 3 Lua macros uses Double instead of Int :(
				case FMVT_DOUBLE:
					Item = (INT_PTR)p->Values[0].Double; break;
				case FMVT_STRING:
					_ASSERTE(p->Values[0].String!=NULL);
					bGuiMacroCall = true;
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
	else if (Info->OpenFrom == OPEN_COMMANDLINE)
	{
		OpenCommandLineInfo* p = (OpenCommandLineInfo*)Info->Data;
		Item = (INT_PTR)p->CommandLine;
	}

	HANDLE h = OpenPluginCommon(Info->OpenFrom, Item, (Info->OpenFrom == OPEN_FROMMACRO));
	if (Info->OpenFrom == OPEN_FROMMACRO)
	{
		// В Far/lua можно вернуть величину и не только булевского типа
		if (h != NULL)
		{
			// That was GuiMacro call?
			if (bGuiMacroCall)
			{
				static FarMacroCall rc{};
				static FarMacroValue val{};
				rc.StructSize = sizeof(rc);
				rc.Count = 1;
				rc.Values = &val;
				rc.Callback = FreeMacroResult;
				val.Type = FMVT_STRING;
				val.String = GetEnvVar(CEGUIMACRORETENVVAR).Detach();
				h = (HANDLE)&rc;
			}
			else
			{
				h = (HANDLE)TRUE;
			}
		}
		else
		{
			h = NULL;
		}
	}
	else if ((h == INVALID_HANDLE_VALUE) || (h == (HANDLE)-2))
	{
		if (Info->OpenFrom == OPEN_ANALYSE)
			h = PANEL_STOP;
		else
			h = NULL;
	}

	return h;
}

#if 0
INT_PTR WINAPI ProcessConsoleInputW2800(void* apInfo)
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
