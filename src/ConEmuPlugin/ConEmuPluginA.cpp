
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
#include "../common/pluginA.hpp"
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/plugin_helper.h"
#include "PluginHeader.h"
#include "PluginBackground.h"
#include "../common/farcolor1.hpp"
#include "../common/MSection.h"

#include "ConEmuPluginA.h"

//#define SHOW_DEBUG_EVENTS


struct PluginStartupInfo *InfoA=NULL;
struct FarStandardFunctions *FSFA=NULL;

CPluginAnsi::CPluginAnsi()
{
	memset(ms_TempMsgBuf, 0, sizeof(ms_TempMsgBuf));
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
	se_CommonSynchro = -1;
	wt_Desktop = -1;
	wt_Panels = WTYPE_PANELS;
	wt_Viewer = WTYPE_VIEWER;
	wt_Editor = WTYPE_EDITOR;
	wt_Dialog = WTYPE_DIALOG;
	wt_VMenu = WTYPE_VMENU;
	wt_Help = WTYPE_HELP;
	ma_Other = -1;
	ma_Shell = 1;
	ma_Viewer = ma_Editor = ma_Dialog = ma_Search = ma_Disks = ma_MainMenu = ma_Menu = ma_Help = -1;
	ma_InfoPanel = ma_QViewPanel = ma_TreePanel = ma_FindFolder = ma_UserMenu = -1;
	ma_ShellAutoCompletion = ma_DialogAutoCompletion = -1;
	of_LeftDiskMenu = OPEN_DISKMENU;
	of_PluginsMenu = OPEN_PLUGINSMENU;
	of_FindList = OPEN_FINDLIST;
	of_Shortcut = OPEN_SHORTCUT;
	of_CommandLine = OPEN_COMMANDLINE;
	of_Editor = OPEN_EDITOR;
	of_Viewer = OPEN_VIEWER;
	of_FilePanel = OPEN_PLUGINSMENU;
	of_Dialog = OPEN_DIALOG;
	of_Analyse = -1;
	of_RightDiskMenu = OPEN_DISKMENU;
	of_FromMacro = -1;
	pt_FilePanel = PTYPE_FILEPANEL;
	pt_TreePanel = PTYPE_TREEPANEL;

	InitRootRegKey();
}

wchar_t* CPluginAnsi::GetPanelDir(GetPanelDirFlags Flags, wchar_t* pszBuffer /*= NULL*/, int cchBufferMax /*= 0*/)
{
	wchar_t* pszDir = NULL;
	PanelInfo pi = {};

	if (!InfoA)
		goto wrap;

	InfoA->Control(INVALID_HANDLE_VALUE, (Flags & gpdf_Active) ? FCTL_GETPANELSHORTINFO : FCTL_GETANOTHERPANELSHORTINFO, &pi);

	if ((Flags & gpdf_NoHidden) && !pi.Visible)
		goto wrap;
	if ((Flags & gpdf_NoPlugin) && pi.Plugin)
		goto wrap;

	if (pi.CurDir[0])
	{
		if (pszBuffer)
		{
			MultiByteToWideChar(CP_OEMCP, 0, pi.CurDir, -1, pszBuffer, cchBufferMax);
			pszDir = pszBuffer;
		}
		else
		{
			pszDir = ToUnicode(pi.CurDir);
		}
	}

wrap:
	if (!pszDir && pszBuffer)
		*pszBuffer = 0;
	return pszDir;
}

bool CPluginAnsi::GetPanelInfo(GetPanelDirFlags Flags, CEPanelInfo* pInfo)
{
	if (!InfoA || !InfoA->Control)
		return false;

	PanelInfo pasv = {}, actv = {};
	PanelInfo* p;

	int getPanelInfo = (Flags & ppdf_GetItems) ? FCTL_GETPANELINFO : FCTL_GETPANELSHORTINFO;
	int getAnotherPanelInfo = (Flags & ppdf_GetItems) ? FCTL_GETANOTHERPANELINFO : FCTL_GETANOTHERPANELSHORTINFO;

	if (Flags & (gpdf_Left|gpdf_Right))
	{
		InfoA->Control(INVALID_HANDLE_VALUE, getPanelInfo, &actv);
		InfoA->Control(INVALID_HANDLE_VALUE, getAnotherPanelInfo, &pasv);
		PanelInfo* pLeft = (actv.Flags & PFLAGS_PANELLEFT) ? &actv : &pasv;
		PanelInfo* pRight = (actv.Flags & PFLAGS_PANELLEFT) ? &pasv : &actv;
		p = (Flags & gpdf_Left) ? pLeft : pRight;
	}
	else
	{
		InfoA->Control(INVALID_HANDLE_VALUE, (Flags & gpdf_Active) ? getPanelInfo : getAnotherPanelInfo, &actv);
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
		MultiByteToWideChar(CP_OEMCP, 0, p->CurDir, -1, pInfo->szCurDir, BkPanelInfo_CurDirMax);

	if (pInfo->szFormat)
		lstrcpyW(pInfo->szFormat, p->Plugin ? L"Plugin" : L"");
	if (pInfo->szHostFile)
		pInfo->szHostFile[0] = 0;

	return true;
}

bool CPluginAnsi::GetPanelItemInfo(const CEPanelInfo& PnlInfo, bool bSelected, INT_PTR iIndex, WIN32_FIND_DATAW& Info, wchar_t** ppszFullPathName)
{
	if (!InfoA)
		return false;

	if (!PnlInfo.panelInfo)
	{
		_ASSERTE(PnlInfo.panelInfo!=NULL);
		return false;
	}

	PluginPanelItem *pPanelItems = bSelected ? ((PanelInfo*)PnlInfo.panelInfo)->SelectedItems : ((PanelInfo*)PnlInfo.panelInfo)->PanelItems;
	INT_PTR iItemsNumber = bSelected ? ((PanelInfo*)PnlInfo.panelInfo)->SelectedItemsNumber : ((PanelInfo*)PnlInfo.panelInfo)->ItemsNumber;

	if (!pPanelItems)
	{
		_ASSERTE(pPanelItems != NULL);
		return false;
	}

	if ((iIndex < 0) || (iIndex >= iItemsNumber))
	{
		_ASSERTE(FALSE && "iItem out of bounds");
		return false;
	}

	PluginPanelItem* pItem = pPanelItems+iIndex;

	ZeroStruct(Info);

	Info.dwFileAttributes = pItem->FindData.dwFileAttributes;
	Info.ftCreationTime = pItem->FindData.ftCreationTime;
	Info.ftLastAccessTime = pItem->FindData.ftLastAccessTime;
	Info.ftLastWriteTime = pItem->FindData.ftLastWriteTime;
	Info.nFileSizeHigh = pItem->FindData.nFileSizeHigh;
	Info.nFileSizeLow = pItem->FindData.nFileSizeLow;

	MultiByteToWideChar(CP_OEMCP, 0, pItem->FindData.cFileName, -1, Info.cFileName, countof(Info.cFileName));
	MultiByteToWideChar(CP_OEMCP, 0, pItem->FindData.cAlternateFileName, -1, Info.cAlternateFileName, countof(Info.cAlternateFileName));

	if (ppszFullPathName)
		*ppszFullPathName = lstrdup(Info.cFileName).Detach();

	return true;
}

INT_PTR CPluginAnsi::PanelControlApi(HANDLE hPanel, int Command, INT_PTR Param1, void* Param2)
{
	if (!InfoA || !InfoA->Control)
		return -1;
	INT_PTR iRc = InfoA->Control(hPanel, (FILE_CONTROL_COMMANDS)Command, Param2);
	return iRc;
}

void CPluginAnsi::SetStartupInfoPtr(void *aInfo)
{
	INIT_FAR_PSI(::InfoA, ::FSFA, (PluginStartupInfo*)aInfo);
	mb_StartupInfoOk = true;

	DWORD nFarVer = 0;
	if (InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETFARVERSION, &nFarVer))
	{
		if (HIBYTE(LOWORD(nFarVer)) == 1)
		{
			gFarVersion.dwBuild = HIWORD(nFarVer);
			gFarVersion.dwVerMajor = (HIBYTE(LOWORD(nFarVer)));
			gFarVersion.dwVerMinor = (LOBYTE(LOWORD(nFarVer)));
			_ASSERTE(gFarVersion.dwBits == WIN3264TEST(32,64));
		}
		else
		{
			_ASSERTE(HIBYTE(HIWORD(nFarVer)) == 1);
		}
	}

	SetRootRegKey(ToUnicode(InfoA->RootKey));
}

void CPluginAnsi::GetPluginInfoPtr(void *piv)
{
	PluginInfo *pi = (PluginInfo*)piv;
	_ASSERTE(pi->StructSize==0);
	pi->StructSize = sizeof(struct PluginInfo);
	//_ASSERTE(pi->StructSize>0 && (pi->StructSize >= sizeof(*pi)));

	static char *szMenu[1], szMenu1[255];
	szMenu[0]=szMenu1;
	// Устарело. активация через [Read/Peek]ConsoleInput
	//// Проверить, не изменилась ли горячая клавиша плагина, и если да - пересоздать макросы
	//IsKeyChanged(TRUE);
	//if (gcPlugKey) szMenu1[0]=0; else lstrcpyA(szMenu1, "[&\xDC] "); // а тут действительно OEM
	lstrcpynA(szMenu1/*+lstrlenA(szMenu1)*/, InfoA->GetMsg(InfoA->ModuleNumber,CEPluginName), 240);
	_ASSERTE(pi->StructSize == sizeof(struct PluginInfo));
	pi->Flags = PF_EDITOR | PF_VIEWER | PF_DIALOG | PF_PRELOAD;
	pi->DiskMenuStrings = NULL;
	pi->DiskMenuNumbers = 0;
	pi->PluginMenuStrings = szMenu;
	pi->PluginMenuStringsNumber = 1;
	pi->PluginConfigStrings = NULL;
	pi->PluginConfigStringsNumber = 0;
	pi->CommandPrefix = "ConEmu";
	pi->Reserved = 0;
}

DWORD CPluginAnsi::GetEditorModifiedState()
{
	EditorInfo ei{};
	InfoA->EditorControl(ECTL_GETINFO, &ei);
#ifdef SHOW_DEBUG_EVENTS
	char szDbg[255];
	wsprintfA(szDbg, "Editor:State=%i\n", ei.CurState);
	OutputDebugStringA(szDbg);
#endif
	// Если он сохранен, то уже НЕ модифицирован
	DWORD currentModifiedState = ((ei.CurState & (ECSTATE_MODIFIED|ECSTATE_SAVED)) == ECSTATE_MODIFIED) ? 1 : 0;
	return currentModifiedState;
}

extern MSection *csTabs;

int CPluginAnsi::GetWindowCount()
{
	if (!InfoA || !InfoA->AdvControl)
		return 0;

	INT_PTR windowCount = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWCOUNT, NULL);
	return (int)windowCount;
}

bool CPluginAnsi::UpdateConEmuTabsApi(int windowCount)
{
	if (!InfoA || gbIgnoreUpdateTabs)
		return false;

	bool lbCh = false, lbDummy = false;
	WindowInfo WInfo;
	WCHAR* pszName = gszDir1; pszName[0] = 0; //(WCHAR*)calloc(CONEMUTABMAX, sizeof(WCHAR));
	int tabCount = 0;
	bool lbActiveFound = false;

	for (int i = 0; i < windowCount; i++)
	{
		WInfo.Pos = i;
		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);

		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
		{
			InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);

			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER || WInfo.Type == WTYPE_PANELS)
			{
				#ifdef SHOW_DEBUG_EVENTS
				char szDbg[255]; wsprintfA(szDbg, "Window %i (Type=%i, Modified=%i)\n", i, WInfo.Type, WInfo.Modified);
				OutputDebugStringA(szDbg);
				#endif

				if (WInfo.Current)
					lbActiveFound = true;

				MultiByteToWideChar(CP_OEMCP, 0, WInfo.Name, lstrlenA(WInfo.Name)+1, pszName, CONEMUTABMAX);
				TODO("Определение ИД редактора/вьювера");
				lbCh |= AddTab(tabCount, -1, false/*losingFocus*/, false/*editorSave*/,
				               WInfo.Type, pszName, /*editorSave ? pszFileName :*/ NULL,
				               WInfo.Current, WInfo.Modified, 0, 0);
				//if (WInfo.Type == WTYPE_EDITOR && WInfo.Current) //2009-08-17
				//	lastModifiedStateW = WInfo.Modified;
			}
		}
	}

	// Скорее всего это модальный редактор (или вьювер?)
	if (!lbActiveFound)
	{
		WInfo.Pos = -1;
		_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);

		if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
		{
			WInfo.Pos = -1;
			InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETWINDOWINFO, (void*)&WInfo);

			if (WInfo.Type == WTYPE_EDITOR || WInfo.Type == WTYPE_VIEWER)
			{
				tabCount = 0;
				MultiByteToWideChar(CP_OEMCP, 0, WInfo.Name, lstrlenA(WInfo.Name)+1, pszName, CONEMUTABMAX);
				TODO("Определение ИД редактора/вьювера");
				lbCh |= AddTab(tabCount, -1, false/*losingFocus*/, false/*editorSave*/,
				               WInfo.Type, pszName, /*editorSave ? pszFileName :*/ NULL,
				               WInfo.Current, WInfo.Modified, 0, 0);
			}
		}
		else if (WInfo.Type == WTYPE_PANELS)
		{
			gpTabs->Tabs.CurrentType = gnCurrentWindowType = WInfo.Type;
		}
	}

	// 101224 - сразу запомнить количество!
	AddTabFinish(tabCount);

	return lbCh;
}

void CPluginAnsi::ExitFar()
{
	if (!mb_StartupInfoOk)
		return;

	if (InfoA)
	{
		free(InfoA);
		InfoA=NULL;
	}

	if (FSFA)
	{
		free(FSFA);
		FSFA=NULL;
	}
}

//void ReloadMacroA()
//{
//	if (!InfoA || !InfoA->AdvControl)
//		return;
//
//	ActlKeyMacro command;
//	command.Command=MCMD_LOADALL;
//	InfoA->AdvControl(InfoA->ModuleNumber,ACTL_KEYMACRO,&command);
//}

void CPluginAnsi::SetWindow(int nTab)
{
	if (!InfoA || !InfoA->AdvControl)
		return;

	if (InfoA->AdvControl(InfoA->ModuleNumber, ACTL_SETCURRENTWINDOW, (void*)(UINT_PTR)nTab))
		InfoA->AdvControl(InfoA->ModuleNumber, ACTL_COMMIT, 0);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void CPluginAnsi::PostMacroApi(const wchar_t* asMacro, INPUT_RECORD* apRec, bool abShowParseErrors)
{
	if (!InfoA || !InfoA->AdvControl) return;

	char* pszMacroA = ToOem(asMacro);
	if (!pszMacroA)
		return;

	char* asMacroA = pszMacroA;

	ActlKeyMacro mcr;
	mcr.Command = MCMD_POSTMACROSTRING;
	mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен

	while ((asMacroA[0] == '@' || asMacroA[0] == '^') && asMacroA[1] && asMacroA[1] != ' ')
	{
		switch (*asMacroA)
		{
		case '@':
			mcr.Param.PlainText.Flags |= KSFLAGS_DISABLEOUTPUT;
			break;
		case '^':
			mcr.Param.PlainText.Flags |= KSFLAGS_NOSENDKEYSTOPLUGINS;
			break;
		}
		asMacroA++;
	}

	mcr.Param.PlainText.SequenceText = asMacroA;

	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_KEYMACRO, (void*)&mcr);

	free(pszMacroA);
}

int CPluginAnsi::ShowPluginMenu(ConEmuPluginMenuItem* apItems, int Count, int TitleMsgId /*= CEPluginName*/)
{
	if (!InfoA)
		return -1;

	//FarMenuItemEx items[] =
	//{
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? MIF_SELECTED : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_SEPARATOR},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC ? 0 : MIF_DISABLE)},
	//	{MIF_SEPARATOR},
	//	{MIF_USETEXTPTR|0},
	//	{MIF_SEPARATOR},
	//	{MIF_USETEXTPTR|(ghConEmuWndDC||IsTerminalMode() ? MIF_DISABLE : MIF_SELECTED)},
	//	{MIF_SEPARATOR},
	//	//#ifdef _DEBUG
	//	//		{MIF_USETEXTPTR},
	//	//#endif
	//	{MIF_USETEXTPTR|(IsDebuggerPresent()||IsTerminalMode() ? MIF_DISABLE : 0)}
	//};
	//items[0].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuEditOutput);
	//items[1].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuViewOutput);
	//items[3].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuShowHideTabs);
	//items[4].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuNextTab);
	//items[5].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuPrevTab);
	//items[6].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuCommitTab);
	//items[8].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuGuiMacro);
	//items[10].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuAttach);
	////#ifdef _DEBUG
	////items[10].Text.TextPtr = "&~. Raise exception";
	////items[11].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuDebug);
	////#else
	//items[12].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber,CEMenuDebug);
	////#endif
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
		if (apItems[i].MsgText)
		{
			WideCharToMultiByte(CP_OEMCP, 0, apItems[i].MsgText, -1, items[i].Text.Text, countof(items[i].Text.Text), 0,0);
		}
		else
		{
			items[i].Text.TextPtr = InfoA->GetMsg(InfoA->ModuleNumber, apItems[i].MsgID);
			items[i].Flags |= MIF_USETEXTPTR;
		}
	}

	int nRc = InfoA->Menu(InfoA->ModuleNumber, -1,-1, 0,
	                      FMENU_USEEXT|FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                      InfoA->GetMsg(InfoA->ModuleNumber,TitleMsgId),
	                      NULL, NULL, NULL, NULL, (FarMenuItem*)items, Count);
	SafeFree(items);
	return nRc;
}

bool CPluginAnsi::OpenEditor(LPCWSTR asFileName, bool abView, bool abDeleteTempFile, bool abDetectCP /*= false*/, int anStartLine /*= 0*/, int anStartChar /*= 1*/)
{
	if (!InfoA)
		return false;

	bool lbRc;
	int iRc;

	char szFileName[MAX_PATH] = "";
	ToOem(asFileName, szFileName, countof(szFileName));
	LPCSTR pszTitle = abDeleteTempFile ? InfoA->GetMsg(InfoA->ModuleNumber,CEConsoleOutput) : NULL;

	if (!abView)
	{
		iRc = InfoA->Editor(szFileName, pszTitle, 0,0,-1,-1,
		                     EF_NONMODAL|EF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (EF_DELETEONLYFILEONCLOSE|EF_DISABLEHISTORY) : 0)
		                     |EF_ENABLE_F6,
		                     anStartLine, anStartChar);
		lbRc = (iRc != EEC_OPEN_ERROR);
	}
	else
	{
		iRc = InfoA->Viewer(szFileName, pszTitle, 0,0,-1,-1,
		                     VF_NONMODAL|VF_IMMEDIATERETURN
		                     |(abDeleteTempFile ? (VF_DELETEONLYFILEONCLOSE|VF_DISABLEHISTORY) : 0)
		                     |VF_ENABLE_F6);
		lbRc = (iRc != 0);
	}

	return lbRc;
}

int CPluginAnsi::ShowMessage(LPCWSTR asMsg, int aiButtons, bool bWarning)
{
	if (!InfoA || !InfoA->Message)
		return -1;

	if (!asMsg)
		asMsg = L"";
	int nLen = lstrlen(asMsg);
	char* szOem = (char*)calloc(nLen+1,sizeof(*szOem));
	ToOem(asMsg, szOem, nLen+1);

	int iMsgRc = InfoA->Message(InfoA->ModuleNumber,
					FMSG_ALLINONE|(aiButtons?0:FMSG_MB_OK)|(bWarning ? FMSG_WARNING : 0), NULL,
					(const char * const *)szOem, 0, aiButtons);
	free(szOem);

	return iMsgRc;
}

LPCWSTR CPluginAnsi::GetMsg(int aiMsg, wchar_t* psMsg /*= NULL*/, size_t cchMsgMax /*= 0*/)
{
	if (!psMsg || !cchMsgMax)
	{
		psMsg = ms_TempMsgBuf;
		cchMsgMax = countof(ms_TempMsgBuf);
	}

	LPCSTR pszRcA = (InfoA && InfoA->GetMsg) ? InfoA->GetMsg(InfoA->ModuleNumber, aiMsg) : "";
	if (!pszRcA)
		pszRcA = "";

	int nLen = MultiByteToWideChar(CP_OEMCP, 0, pszRcA, -1, psMsg, cchMsgMax-1);
	if (nLen>=0)
		psMsg[nLen] = 0;

	return psMsg;
}


bool CPluginAnsi::IsMacroActive()
{
	if (!InfoA || !FarHwnd)
		return false;

	ActlKeyMacro akm = {MCMD_GETSTATE};
	INT_PTR liRc = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_KEYMACRO, &akm);

	if (liRc == MACROSTATE_NOMACRO)
		return false;

	return true;
}

int CPluginAnsi::GetMacroArea()
{
	return 1; // в Far 1.7x не поддерживается
}

void CPluginAnsi::RedrawAll()
{
	if (!InfoA || !FarHwnd)
		return;

	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_REDRAWALL, NULL);
}

bool CPluginAnsi::InputBox(LPCWSTR Title, LPCWSTR SubTitle, LPCWSTR HistoryName, LPCWSTR SrcText, wchar_t*& DestText)
{
	_ASSERTE(DestText==NULL);
	if (!InfoA)
		return false;

	char strTemp[MAX_PATH+1] = "", aTitle[64] = "", aSubTitle[128] = "", aHistoryName[64] = "", aSrcText[128];
	ToOem(Title, aTitle, countof(aTitle));
	ToOem(SubTitle, aSubTitle, countof(aSubTitle));
	ToOem(HistoryName, aHistoryName, countof(aHistoryName));
	ToOem(SrcText, aSrcText, countof(aSrcText));

	if (!InfoA->InputBox(aTitle, aSubTitle, aHistoryName, aSrcText, strTemp, countof(strTemp), NULL, FIB_BUTTONS))
		return false;
	DestText = ToUnicode(strTemp);
	return true;
}

void CPluginAnsi::ShowUserScreen(bool bUserScreen)
{
	if (!InfoA)
		return;

	if (bUserScreen)
		InfoA->Control(INVALID_HANDLE_VALUE, FCTL_GETUSERSCREEN, 0);
	else
		InfoA->Control(INVALID_HANDLE_VALUE, FCTL_SETUSERSCREEN, 0);
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

void CPluginAnsi::LoadFarColors(BYTE (&nFarColors)[col_LastIndex])
{
	BYTE FarConsoleColors[0x100];
	INT_PTR nColorSize = InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETARRAYCOLOR, FarConsoleColors);
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

void CPluginAnsi::LoadFarSettings(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel)
{
	DWORD nSet;

	nSet = (DWORD)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETINTERFACESETTINGS, 0);
	if (pInterface)
	{
		pInterface->Raw = nSet;
		_ASSERTE((pInterface->AlwaysShowMenuBar != 0) == ((nSet & FIS_ALWAYSSHOWMENUBAR) != 0));
		_ASSERTE((pInterface->ShowKeyBar != 0) == ((nSet & FIS_SHOWKEYBAR) != 0));
	}

	nSet = (DWORD)InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETPANELSETTINGS, 0);
	if (pPanel)
	{
		pPanel->Raw = nSet;
		_ASSERTE((pPanel->ShowColumnTitles != 0) == ((nSet & FPS_SHOWCOLUMNTITLES) != 0));
		_ASSERTE((pPanel->ShowStatusLine != 0) == ((nSet & FPS_SHOWSTATUSLINE) != 0));
		_ASSERTE((pPanel->ShowSortModeLetter != 0) == ((nSet & FPS_SHOWSORTMODELETTER) != 0));
	}
}

bool CPluginAnsi::CheckPanelExist()
{
	if (!InfoA || !InfoA->Control)
		return false;

	INT_PTR iRc = InfoA->Control(INVALID_HANDLE_VALUE, FCTL_CHECKPANELSEXIST, 0);
	return (iRc!=0);
}

int CPluginAnsi::GetActiveWindowType()
{
	if (!InfoA || !InfoA->AdvControl)
		return -1;

	WindowInfo WInfo = {-1};
	_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
	InfoA->AdvControl(InfoA->ModuleNumber, ACTL_GETSHORTWINDOWINFO, (void*)&WInfo);
	return WInfo.Type;
}

LPCWSTR CPluginAnsi::GetWindowTypeName(int WindowType)
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

#undef FAR_UNICODE
#include "Dialogs.h"
void CPluginAnsi::GuiMacroDlg()
{
	CallGuiMacroProc();
}
