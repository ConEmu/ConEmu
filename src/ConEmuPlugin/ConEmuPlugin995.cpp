
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
#include "../common/pluginW1761.hpp" // Отличается от 995 наличием SynchoApi
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/plugin_helper.h"
#include "PluginHeader.h"
#include "../common/farcolor2.hpp"

#include "ConEmuPlugin995.h"

#ifdef _DEBUG
//#define SHOW_DEBUG_EVENTS
#endif

//WARNING("Far colorer TrueMod - последняя сборка была 1721");
//#define FORCE_FAR_1721
#undef FORCE_FAR_1721

struct PluginStartupInfo *InfoW995=NULL;
struct FarStandardFunctions *FSFW995=NULL;

CPluginW995::CPluginW995()
{
	ee_Read = EE_READ;
	ee_Save = EE_SAVE;
	ee_Redraw = EE_REDRAW;
	ee_Close = EE_CLOSE;
	ee_GotFocus = EE_GOTFOCUS;
	ee_KillFocus = EE_KILLFOCUS;
	ee_Change = -1;
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
	ma_ShellAutoCompletion = -1;
	ma_DialogAutoCompletion = -1;
	of_LeftDiskMenu = -1;
	of_PluginsMenu = OPEN_PLUGINSMENU;
	of_FindList = OPEN_FINDLIST;
	of_Shortcut = OPEN_SHORTCUT;
	of_CommandLine = OPEN_COMMANDLINE;
	of_Editor = OPEN_EDITOR;
	of_Viewer = OPEN_VIEWER;
	of_FilePanel = OPEN_FILEPANEL;
	of_Dialog = OPEN_DIALOG;
	of_Analyse = OPEN_ANALYSE;
	of_RightDiskMenu = -1;
	of_FromMacro = OPEN_FROMMACRO;
	fctl_GetPanelDirectory = FCTL_GETPANELDIR;
	fctl_GetPanelFormat = FCTL_GETPANELFORMAT;
	fctl_GetPanelPrefix = -1;
	fctl_GetPanelHostFile = FCTL_GETPANELHOSTFILE;
	pt_FilePanel = PTYPE_FILEPANEL;
	pt_TreePanel = PTYPE_TREEPANEL;

	InitRootRegKey();
}

wchar_t* CPluginW995::GetPanelDir(GetPanelDirFlags Flags, wchar_t* pszBuffer /*= NULL*/, int cchBufferMax /*= 0*/)
{
	wchar_t* pszDir = NULL;
	HANDLE hPanel = (Flags & gpdf_Active) ? PANEL_ACTIVE : PANEL_PASSIVE;
	size_t nSize;
	PanelInfo pi = {};

	if (!InfoW995)
		goto wrap;

	InfoW995->Control(hPanel, FCTL_GETPANELINFO, 0, (LONG_PTR)&pi);

	if ((Flags & gpdf_NoHidden) && !pi.Visible)
		goto wrap;
	if ((Flags & gpdf_NoPlugin) && pi.Plugin)
		goto wrap;

	nSize = InfoW995->Control(hPanel, FCTL_GETPANELDIR, 0, 0);

	if (nSize)
	{
		pszDir = (wchar_t*)calloc(nSize, sizeof(*pszDir));
		if (pszDir)
		{
			nSize = InfoW995->Control(hPanel, FCTL_GETPANELDIR, (DWORD)nSize, (LONG_PTR)pszDir);
			if (pszBuffer)
			{
				lstrcpyn(pszBuffer, pszDir, cchBufferMax);
				SafeFree(pszDir);
				pszDir = pszBuffer;
			}
		}
	}
	// May happen on Far termination
	//_ASSERTE(nSize>0);

wrap:
	if (!pszDir && pszBuffer)
		*pszBuffer = 0;
	return pszDir;
}

bool CPluginW995::GetPanelInfo(GetPanelDirFlags Flags, CEPanelInfo* pInfo)
{
	if (!InfoW995 || !InfoW995->Control)
		return false;

	_ASSERTE(gFarVersion.dwBuild >= 1657);

	HANDLE hPanel = (Flags & gpdf_Active) ? PANEL_ACTIVE : PANEL_PASSIVE;
	PanelInfo pasv = {}, actv = {};
	PanelInfo* p;

	if (Flags & (gpdf_Left|gpdf_Right))
	{
		InfoW995->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&actv);
		InfoW995->Control(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, (LONG_PTR)&pasv);
		PanelInfo* pLeft = (actv.Flags & PFLAGS_PANELLEFT) ? &actv : &pasv;
		PanelInfo* pRight = (actv.Flags & PFLAGS_PANELLEFT) ? &pasv : &actv;
		p = (Flags & gpdf_Left) ? pLeft : pRight;
		hPanel = (p->Focus) ? PANEL_ACTIVE : PANEL_PASSIVE;
	}
	else
	{
		hPanel = (Flags & gpdf_Active) ? PANEL_ACTIVE : PANEL_PASSIVE;
		InfoW995->Control(hPanel, FCTL_GETPANELINFO, 0, (LONG_PTR)&actv);
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

	pInfo->bVisible = p->Visible;
	pInfo->bFocused = p->Focus;
	pInfo->bPlugin = p->Plugin;
	pInfo->nPanelType = p->PanelType;
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
			PanelControl(hPanel, FCTL_GETPANELFORMAT, BkPanelInfo_FormatMax, pInfo->szFormat);
		if (pInfo->szHostFile)
			PanelControl(hPanel, FCTL_GETPANELHOSTFILE, BkPanelInfo_HostFileMax, pInfo->szHostFile);
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

bool CPluginW995::GetPanelItemInfo(const CEPanelInfo& PnlInfo, bool bSelected, INT_PTR iIndex, WIN32_FIND_DATAW& Info, wchar_t** ppszFullPathName)
{
	if (!InfoW995 || !InfoW995->Control)
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

	if (PanelControlApi(PANEL_ACTIVE, iCmd, iIndex, pItem) <= 0)
	{
		free(pItem);
		return false;
	}

	ZeroStruct(Info);

	Info.dwFileAttributes = pItem->FindData.dwFileAttributes;
	Info.ftCreationTime = pItem->FindData.ftCreationTime;
	Info.ftLastAccessTime = pItem->FindData.ftLastAccessTime;
	Info.ftLastWriteTime = pItem->FindData.ftLastWriteTime;
	Info.nFileSizeLow = LODWORD(pItem->FindData.nFileSize);
	Info.nFileSizeHigh = HIDWORD(pItem->FindData.nFileSize);

	if (pItem->FindData.lpwszFileName)
	{
		LPCWSTR pszName = pItem->FindData.lpwszFileName;
		int iLen = lstrlen(pszName);
		// If full paths exceeds MAX_PATH chars - return in Info.cFileName the file name only
		if (iLen >= countof(Info.cFileName))
			pszName = PointToName(pItem->FindData.lpwszFileName);
		lstrcpyn(Info.cFileName, pszName, countof(Info.cFileName));
		if (ppszFullPathName)
			*ppszFullPathName = lstrdup(pItem->FindData.lpwszFileName).Detach();
	}
	else if (ppszFullPathName)
	{
		_ASSERTE(*ppszFullPathName == NULL);
		*ppszFullPathName = NULL;
	}

	if (pItem->FindData.lpwszAlternateFileName)
	{
		lstrcpyn(Info.cAlternateFileName, pItem->FindData.lpwszAlternateFileName, countof(Info.cAlternateFileName));
	}

	free(pItem);

	return true;
}

INT_PTR CPluginW995::PanelControlApi(HANDLE hPanel, int Command, INT_PTR Param1, void* Param2)
{
	if (!InfoW995 || !InfoW995->Control)
		return -1;
	INT_PTR iRc = InfoW995->Control(hPanel, (FILE_CONTROL_COMMANDS)Command, Param1, (LONG_PTR)Param2);
	return iRc;
}

void CPluginW995::GetPluginInfoPtr(void *piv)
{
	PluginInfo *pi = (PluginInfo*)piv;
	//memset(pi, 0, sizeof(PluginInfo));
	_ASSERTE(pi->StructSize==0);
	pi->StructSize = sizeof(struct PluginInfo);
	//_ASSERTE(pi->StructSize>0 && (pi->StructSize >= sizeof(*pi)));

	static WCHAR *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1; //lstrcpyW(szMenu[0], L"[&\x2560] ConEmu"); -> 0x2584
	//szMenu[0][1] = L'&';
	//szMenu[0][2] = 0x2560;
	// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
	//IsKeyChanged(TRUE); -- в FAR2 устарело, используем Synchro
	//if (gcPlugKey) szMenu1[0]=0; else lstrcpyW(szMenu1, L"[&\x2584] ");
	//lstrcpynW(szMenu1+lstrlenW(szMenu1), GetMsgW(2), 240);
	GetMsg(CEPluginName, szMenu1, 240);
	_ASSERTE(pi->StructSize == sizeof(struct PluginInfo));
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG | PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	//pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = L"ConEmu";
	pi->Reserved = ConEmu_SysID; // 'CEMU'
}

void CPluginW995::SetStartupInfoPtr(void *aInfo)
{
	INIT_FAR_PSI(::InfoW995, ::FSFW995, (PluginStartupInfo*)aInfo);
	mb_StartupInfoOk = true;

	DWORD nFarVer = 0;
	if (InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETFARVERSION, &nFarVer))
	{
		if (HIBYTE(LOWORD(nFarVer)) == 2)
		{
			gFarVersion.dwBuild = HIWORD(nFarVer);
			gFarVersion.dwVerMajor = (HIBYTE(LOWORD(nFarVer)));
			gFarVersion.dwVerMinor = (LOBYTE(LOWORD(nFarVer)));
			_ASSERTE(gFarVersion.dwBits == WIN3264TEST(32,64));
		}
		else
		{
			_ASSERTE(HIBYTE(HIWORD(nFarVer)) == 2);
		}
	}

	SetRootRegKey(lstrdup(InfoW995->RootKey).Detach());
}

DWORD CPluginW995::GetEditorModifiedState()
{
	EditorInfo ei{};
	InfoW995->EditorControl(ECTL_GETINFO, &ei);

	#ifdef SHOW_DEBUG_EVENTS
	char szDbg[255];
	wsprintfA(szDbg, "Editor:State=%i\n", ei.CurState);
	OutputDebugStringA(szDbg);
	#endif

	// Если он сохранен, то уже НЕ модифицирован
	DWORD currentModifiedState = ((ei.CurState & (ECSTATE_MODIFIED|ECSTATE_SAVED)) == ECSTATE_MODIFIED) ? 1 : 0;
	return currentModifiedState;
}

//extern int lastModifiedStateW;

// watch non-modified -> modified editor status change
int CPluginW995::ProcessEditorInputPtr(LPCVOID aRec)
{
	if (!InfoW995)
		return 0;

	const INPUT_RECORD *Rec = (const INPUT_RECORD*)aRec;
	ProcessEditorInputInternal(*Rec);

	return 0;
}

int CPluginW995::GetWindowCount()
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return 0;

	INT_PTR windowCount = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	return (int)windowCount;
}

bool CPluginW995::UpdateConEmuTabsApi(int windowCount)
{
	if (!InfoW995 || !InfoW995->AdvControl || gbIgnoreUpdateTabs)
		return false;

	bool lbCh = false;
	WindowInfo WInfo = {0};
	wchar_t szWNameBuffer[CONEMUTABMAX];
	int tabCount = 0;
	bool lbActiveFound = false;

	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);

		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			WInfo.Pos = i;
			WInfo.Name = szWNameBuffer;
			WInfo.NameSize = CONEMUTABMAX;
			InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);
			WARNING("Для получения имени нужно пользовать ECTL_GETFILENAME");

			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
			{
				if (WInfo.Current)
					lbActiveFound = true;

				TODO("Определение ИД редактора/вьювера");
				lbCh |= AddTab(tabCount, i, false/*losingFocus*/, false/*editorSave*/,
				               WInfo.Type, WInfo.Name, /*editorSave ? ei.FileName :*/ NULL,
				               WInfo.Current, WInfo.Modified, 0, 0);
			}
		}
	}

	// Скорее всего это модальный редактор (или вьювер?)
	if (!lbActiveFound)
	{
		WInfo.Pos = -1;

		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		if (InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo))
		{
			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
			{
				WInfo.Pos = -1;
				WInfo.Name = szWNameBuffer;
				WInfo.NameSize = CONEMUTABMAX;
				InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);

				if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
				{
					tabCount = 0;
					TODO("Определение ИД Редактора/вьювера");
					lbCh |= AddTab(tabCount, -1, false/*losingFocus*/, false/*editorSave*/,
					               WInfo.Type, WInfo.Name, /*editorSave ? ei.FileName :*/ NULL,
					               WInfo.Current, WInfo.Modified, 1/*Modal*/, 0);
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

void CPluginW995::ExitFar()
{
	if (!mb_StartupInfoOk)
		return;

	ShutdownPluginStep(L"ExitFARW995");

	WaitEndSynchro();

	if (InfoW995)
	{
		free(InfoW995);
		InfoW995=NULL;
	}

	if (FSFW995)
	{
		free(FSFW995);
		FSFW995=NULL;
	}

	ShutdownPluginStep(L"ExitFARW995 - done");
}

int CPluginW995::ShowMessage(LPCWSTR asMsg, int aiButtons, bool bWarning)
{
	if (!InfoW995 || !InfoW995->Message || !InfoW995->GetMsg)
		return -1;

	return InfoW995->Message(InfoW995->ModuleNumber,
					FMSG_ALLINONE995|(aiButtons?0:FMSG_MB_OK)|(bWarning ? FMSG_WARNING : 0), NULL,
					(const wchar_t * const *)asMsg, 0, aiButtons);
}

LPCWSTR CPluginW995::GetMsg(int aiMsg, wchar_t* psMsg /*= NULL*/, size_t cchMsgMax /*= 0*/)
{
	LPCWSTR pszRc = (InfoW995 && InfoW995->GetMsg) ? InfoW995->GetMsg(InfoW995->ModuleNumber, aiMsg) : L"";
	if (!pszRc)
		pszRc = L"";
	if (psMsg)
		lstrcpyn(psMsg, pszRc, cchMsgMax);
	return pszRc;
}

//void ReloadMacro995()
//{
//	if (!InfoW995 || !InfoW995->AdvControl)
//		return;
//
//	ActlKeyMacro command;
//	command.Command=MCMD_LOADALL;
//	InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_KEYMACRO,&command);
//}

void CPluginW995::SetWindow(int nTab)
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return;

	if (InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)(UINT_PTR)nTab))
		InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_COMMIT, 0);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void CPluginW995::PostMacroApi(const wchar_t* asMacro, INPUT_RECORD* apRec, bool abShowParseErrors)
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен

	while ((asMacro[0] == L'@' || asMacro[0] == L'^') && asMacro[1] && asMacro[1] != L' ')
	{
		switch (*asMacro)
		{
		case L'@':
			mcr.Param.PlainText.Flags |= KSFLAGS_DISABLEOUTPUT;
			break;
		case L'^':
			mcr.Param.PlainText.Flags |= KSFLAGS_NOSENDKEYSTOPLUGINS;
			break;
		}
		asMacro++;
	}

	mcr.Param.PlainText.Flags |= KSFLAGS_SILENTCHECK;

	mcr.Param.PlainText.SequenceText = asMacro;

	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);
}

int CPluginW995::ShowPluginMenu(ConEmuPluginMenuItem* apItems, int Count, int TitleMsgId /*= CEPluginName*/)
{
	if (!InfoW995)
		return -1;

	//FarMenuItemEx items[] =
	//{
	//	{ConEmuHwnd ? MIF_SELECTED : MIF_DISABLE,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuEditOutput)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuViewOutput)},
	//	{MIF_SEPARATOR},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuShowHideTabs)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuNextTab)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuPrevTab)},
	//	{ConEmuHwnd ? 0 : MIF_DISABLE,             InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuCommitTab)},
	//	{MIF_SEPARATOR},
	//	{0,                                        InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuGuiMacro)},
	//	{MIF_SEPARATOR},
	//	{ConEmuHwnd||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED,  InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuAttach)},
	//	{MIF_SEPARATOR},
	//	//#ifdef _DEBUG
	//	//{0, L"&~. Raise exception"},
	//	//#endif
	//	{IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0,    InfoW995->GetMsg(InfoW995->ModuleNumber,CEMenuDebug)},
	//};
	//int nCount = sizeof(items)/sizeof(items[0]);

	FarMenuItemEx* items = (FarMenuItemEx*)calloc(Count, sizeof(*items));
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
		items[i].Text = apItems[i].MsgText ? apItems[i].MsgText : InfoW995->GetMsg(InfoW995->ModuleNumber, apItems[i].MsgID);
	}

	int nRc = InfoW995->Menu(InfoW995->ModuleNumber, -1,-1, 0,
	                         FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                         InfoW995->GetMsg(InfoW995->ModuleNumber,TitleMsgId),
	                         NULL, NULL, NULL, NULL, (FarMenuItem*)items, Count);
	SafeFree(items);
	return nRc;
}

bool CPluginW995::OpenEditor(LPCWSTR asFileName, bool abView, bool abDeleteTempFile, bool abDetectCP /*= false*/, int anStartLine /*= 0*/, int anStartChar /*= 1*/)
{
	if (!InfoW995)
		return false;

	bool lbRc;
	int iRc;

	LPCWSTR pszTitle = abDeleteTempFile ? InfoW995->GetMsg(InfoW995->ModuleNumber,CEConsoleOutput) : NULL;

	if (!abView)
	{
		iRc = InfoW995->Editor(asFileName, pszTitle, 0,0,-1,-1,
		                     EF_NONMODAL|EF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (EF_DELETEONLYFILEONCLOSE|EF_DISABLEHISTORY) : 0)
		                     |EF_ENABLE_F6,
		                     anStartLine, anStartChar,
		                     abDetectCP ? CP_AUTODETECT : 1200);
		lbRc = (iRc != EEC_OPEN_ERROR);
	}
	else
	{
		iRc = InfoW995->Viewer(asFileName, pszTitle, 0,0,-1,-1,
		                     VF_NONMODAL|VF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (VF_DELETEONLYFILEONCLOSE|VF_DISABLEHISTORY) : 0)
		                     |VF_ENABLE_F6,
		                     abDetectCP ? CP_AUTODETECT : 1200);
		lbRc = (iRc != 0);
	}

	return lbRc;
}

bool CPluginW995::ExecuteSynchroApi()
{
	if (!InfoW995)
		return false;

	// получается более 2-х, если фар в данный момент чем-то занят (сканирует каталог?)
	//_ASSERTE(gnSynchroCount<=3);
	gnSynchroCount++;
	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_SYNCHRO, NULL);
	return true;
}

static HANDLE ghSyncDlg = NULL;

void CPluginW995::WaitEndSynchro()
{
	if (gFarVersion.dwVerMajor == 2 && gFarVersion.dwVerMinor >= 1)
		return;
	TODO("lbSynchroSafe - может и в Far3 починят. http://bugs.farmanager.com/view.php?id=1604");

	if ((gnSynchroCount == 0) || !(IS_SYNCHRO_ALLOWED))
		return;

	FarDialogItem items[] =
	{
		{DI_DOUBLEBOX,  3,  1,  51, 3, false, {0}, 0, false, GetMsg(CEPluginName)},

		{DI_BUTTON,     0,  2,  0,  0, true,  {0}, DIF_CENTERGROUP, true, GetMsg(CEStopSynchroWaiting)},
	};
	ghSyncDlg = InfoW995->DialogInit(InfoW995->ModuleNumber, -1,-1, 55, 5, NULL, items, countof(items), 0, 0, NULL, 0);

	if (ghSyncDlg == INVALID_HANDLE_VALUE)
	{
		// Видимо, Фар в состоянии выхода (финальная выгрузка всех плагинов)
		// В этом случае, по идее, Synchro вызываться более не должно
		gnSynchroCount = 0; // так что просто сбросим счетчик
	}
	else
	{
		InfoW995->DialogRun(ghSyncDlg);
		InfoW995->DialogFree(ghSyncDlg);
	}

	ghSyncDlg = NULL;
}

void CPluginW995::StopWaitEndSynchro()
{
	if (ghSyncDlg)
	{
		InfoW995->SendDlgMessage(ghSyncDlg, DM_CLOSE, -1, 0);
	}
}

bool CPluginW995::IsMacroActive()
{
	if (!InfoW995 || !FarHwnd)
		return false;

	ActlKeyMacro akm = {MCMD_GETSTATE};
	INT_PTR liRc = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, &akm);

	if (liRc == MACROSTATE_NOMACRO)
		return false;

	return true;
}

int CPluginW995::GetMacroArea()
{
	#define MCMD_GETAREA 6
	ActlKeyMacro area = {MCMD_GETAREA};
	int nArea = (int)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, &area);
	return nArea;
}


void CPluginW995::RedrawAll()
{
	if (!InfoW995 || !FarHwnd)
		return;

	InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_REDRAWALL, NULL);
}

#if 0
bool CPluginW995::LoadPlugin(wchar_t* pszPluginPath)
{
	if (!InfoW995) return false;

	InfoW995->PluginsControl(INVALID_HANDLE_VALUE,PCTL_LOADPLUGIN,PLT_PATH,(LONG_PTR)pszPluginPath);
	return true;
}
#endif

bool CPluginW995::InputBox(LPCWSTR Title, LPCWSTR SubTitle, LPCWSTR HistoryName, LPCWSTR SrcText, wchar_t*& DestText)
{
	_ASSERTE(DestText==NULL);
	if (!InfoW995)
		return false;

	wchar_t strTemp[MAX_PATH+1];
	if (!InfoW995->InputBox(Title, SubTitle, HistoryName, SrcText, strTemp, countof(strTemp), NULL, FIB_BUTTONS))
		return false;
	DestText = lstrdup(strTemp).Detach();
	return true;
}

void CPluginW995::ShowUserScreen(bool bUserScreen)
{
	if (!InfoW995)
		return;

	if (bUserScreen)
		InfoW995->Control(INVALID_HANDLE_VALUE, FCTL_GETUSERSCREEN, 0, 0);
	else
		InfoW995->Control(INVALID_HANDLE_VALUE, FCTL_SETUSERSCREEN, 0, 0);
}

//static void FarPanel2CePanel(PanelInfo* pFar, CEFAR_SHORT_PANEL_INFO* pCE)
//{
//	pCE->PanelType = pFar->PanelType;
//	pCE->Plugin = pFar->Plugin;
//	pCE->PanelRect = pFar->PanelRect;
//	pCE->ItemsNumber = pFar->ItemsNumber;
//	pCE->SelectedItemsNumber = pFar->SelectedItemsNumber;
//	pCE->CurrentItem = pFar->CurrentItem;
//	pCE->TopPanelItem = pFar->TopPanelItem;
//	pCE->Visible = pFar->Visible;
//	pCE->Focus = pFar->Focus;
//	pCE->ViewMode = pFar->ViewMode;
//	pCE->ShortNames = pFar->ShortNames;
//	pCE->SortMode = pFar->SortMode;
//	pCE->Flags = pFar->Flags;
//}

void CPluginW995::LoadFarColors(BYTE (&nFarColors)[col_LastIndex])
{
	BYTE FarConsoleColors[0x100];
	INT_PTR nColorSize = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETARRAYCOLOR, FarConsoleColors);
#ifdef _DEBUG
	INT_PTR nDefColorSize = COL_LASTPALETTECOLOR;
	_ASSERTE(nColorSize==nDefColorSize);
#endif
	UNREFERENCED_PARAMETER(nColorSize);
	nFarColors[col_PanelText] = FarConsoleColors[COL_PANELTEXT];
	nFarColors[col_PanelSelectedCursor] = FarConsoleColors[COL_PANELSELECTEDCURSOR];
	nFarColors[col_PanelSelectedText] = FarConsoleColors[COL_PANELSELECTEDTEXT];
	nFarColors[col_PanelCursor] = FarConsoleColors[COL_PANELCURSOR];
	nFarColors[col_PanelColumnTitle] = FarConsoleColors[COL_PANELCOLUMNTITLE];
	nFarColors[col_PanelBox] = FarConsoleColors[COL_PANELBOX];
	nFarColors[col_HMenuText] = FarConsoleColors[COL_HMENUTEXT];
	nFarColors[col_WarnDialogBox] = FarConsoleColors[COL_WARNDIALOGBOX];
	nFarColors[col_DialogBox] = FarConsoleColors[COL_DIALOGBOX];
	nFarColors[col_CommandLineUserScreen] = FarConsoleColors[COL_COMMANDLINEUSERSCREEN];
	nFarColors[col_PanelScreensNumber] = FarConsoleColors[COL_PANELSCREENSNUMBER];
	nFarColors[col_KeyBarNum] = FarConsoleColors[COL_KEYBARNUM];
	nFarColors[col_EditorText] = FarConsoleColors[COL_EDITORTEXT];
	nFarColors[col_ViewerText] = FarConsoleColors[COL_VIEWERTEXT];
}

void CPluginW995::LoadFarSettings(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel)
{
	DWORD nSet;

	nSet = (DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);
	if (pInterface)
	{
		pInterface->Raw = nSet;
		_ASSERTE((pInterface->AlwaysShowMenuBar != 0) == ((nSet & FIS_ALWAYSSHOWMENUBAR) != 0));
		_ASSERTE((pInterface->ShowKeyBar != 0) == ((nSet & FIS_SHOWKEYBAR) != 0));
	}

	nSet = (DWORD)InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETPANELSETTINGS, 0);
	if (pPanel)
	{
		pPanel->Raw = nSet;
		_ASSERTE((pPanel->ShowColumnTitles != 0) == ((nSet & FPS_SHOWCOLUMNTITLES) != 0));
		_ASSERTE((pPanel->ShowStatusLine != 0) == ((nSet & FPS_SHOWSTATUSLINE) != 0));
		_ASSERTE((pPanel->ShowSortModeLetter != 0) == ((nSet & FPS_SHOWSORTMODELETTER) != 0));
	}
}

bool CPluginW995::GetFarRect(SMALL_RECT& rcFar)
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return false;

	_ASSERTE(ACTL_GETFARRECT==32); //-V112
	ZeroStruct(rcFar);
	if (InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETFARRECT, &rcFar))
	{
		if (rcFar.Bottom > rcFar.Top)
		{
			return true;
		}
	}

	return false;
}

bool CPluginW995::CheckPanelExist()
{
	if (!InfoW995 || !InfoW995->Control)
		return false;

	INT_PTR iRc = InfoW995->Control(INVALID_HANDLE_VALUE, FCTL_CHECKPANELSEXIST, 0, 0);
	return (iRc!=0);
}

int CPluginW995::GetActiveWindowType()
{
	if (!InfoW995 || !InfoW995->AdvControl)
		return -1;

	//_ASSERTE(GetCurrentThreadId() == gnMainThreadId);


	ActlKeyMacro area = {MCMD_GETAREA};
	INT_PTR nArea = InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_KEYMACRO, &area);

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
		case MACROAREA_AUTOCOMPLETION:
			return WTYPE_DIALOG;
		case MACROAREA_HELP:
			return WTYPE_HELP;
		case MACROAREA_MAINMENU:
		case MACROAREA_MENU:
		case MACROAREA_USERMENU:
			return WTYPE_VMENU;
		case MACROAREA_OTHER: // Grabber
			return -1;
		//default:
		//	return -1;
	}

	//WindowInfo WInfo = {-1};
	//_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	//InfoW995->AdvControl(InfoW995->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	//return WInfo.Type;

	return -1;
}

LPCWSTR CPluginW995::GetWindowTypeName(int WindowType)
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

#define FAR_UNICODE 995
#include "Dialogs.h"
void CPluginW995::GuiMacroDlg()
{
	CallGuiMacroProc();
}
