
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

#include "../common/defines.h"
#include "../common/MAssert.h"
#pragma warning( disable : 4995 )
#include "../common/pluginW2800.hpp" // Far3
#pragma warning( default : 4995 )
#include "../common/plugin_helper.h"
#include "ConEmuTh.h"
#include "ConEmuTh_Lang.h"
#include "../common/farcolor3.hpp"
#include "../common/ConEmuColors3.h"
#include "../ConEmu/version.h" // Far3

//#define FCTL_GETPANELDIR FCTL_GETCURRENTDIRECTORY

//#define _ACTL_GETFARRECT 32

#ifdef _DEBUG
#define SHOW_DEBUG_EVENTS
#endif

struct PluginStartupInfo *InfoW2800=NULL;
struct FarStandardFunctions *FSFW2800=NULL;

GUID guid_ConEmuTh = { /* bd454d48-448e-46cc-909d-b6cf789c2d65 */
    0xbd454d48,
    0x448e,
    0x46cc,
    {0x90, 0x9d, 0xb6, 0xcf, 0x78, 0x9c, 0x2d, 0x65}
};
GUID guid_ConEmuThPluginMenu = { /* 128414a5-68a2-44d2-b092-c9c5225324e1 */
    0x128414a5,
    0x68a2,
    0x44d2,
    {0xb0, 0x92, 0xc9, 0xc5, 0x22, 0x53, 0x24, 0xe1}
};
//INTERFACENAME = { /* 81b782cf-1d4e-4269-aeca-f3d7ac759363 */
//    0x81b782cf,
//    0x1d4e,
//    0x4269,
//    {0xae, 0xca, 0xf3, 0xd7, 0xac, 0x75, 0x93, 0x63}
//  };
//INTERFACENAME = { /* 857d6089-5fc7-4284-b66a-ce54dfae7efc */
//    0x857d6089,
//    0x5fc7,
//    0x4284,
//    {0xb6, 0x6a, 0xce, 0x54, 0xdf, 0xae, 0x7e, 0xfc}
//  };

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info)
{
	_ASSERTE(Info->StructSize >= (size_t)((LPBYTE)(&Info->Instance) - (LPBYTE)(Info)));

	if (gFarVersion.dwBuild >= FAR_Y2_VER)
		Info->MinFarVersion = FARMANAGERVERSION;
	else
		Info->MinFarVersion = MAKEFARVERSION(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, 2578, FARMANAGERVERSION_STAGE);

	// Build: YYMMDDX (YY - две цифры года, MM - месяц, DD - день, X - 0 и выше-номер подсборки)
	Info->Version = MAKEFARVERSION(MVV_1,MVV_2,MVV_3,((MVV_1 % 100)*100000) + (MVV_2*1000) + (MVV_3*10) + (MVV_4 % 10),VS_RELEASE);

	Info->Guid = guid_ConEmuTh;
	Info->Title = L"ConEmu Panel Views";
	Info->Description = L"Thumbnails and Tiles in ConEmu window";
	Info->Author = L"ConEmu.Maximus5@gmail.com";
}

void GetPluginInfoW2800(void *piv)
{
	PluginInfo *pi = (PluginInfo*)piv;
	//memset(pi, 0, sizeof(PluginInfo));
	//pi->StructSize = sizeof(struct PluginInfo);
	_ASSERTE(pi->StructSize>0 && ((size_t)pi->StructSize >= sizeof(*pi)/*(size_t)(((LPBYTE)&pi->MacroFunctionNumber) - (LPBYTE)pi))*/));

	static wchar_t *szMenu[1], szMenu1[255];
	szMenu[0] = szMenu1;
	lstrcpynW(szMenu1, GetMsgW(CEPluginName), 240); //-V303

	pi->Flags = isPreloadByDefault()?PF_PRELOAD:0;
	pi->PluginMenu.Guids = &guid_ConEmuThPluginMenu;
	pi->PluginMenu.Strings = szMenu;
	pi->PluginMenu.Count = 1;
}


void SetStartupInfoW2800(void *aInfo)
{
	INIT_FAR_PSI(::InfoW2800, ::FSFW2800, (PluginStartupInfo*)aInfo);

	VersionInfo FarVer = {0};
	if (InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETFARMANAGERVERSION, 0, &FarVer))
	{
		if (FarVer.Major == 3)
		{
			gFarVersion.dwBuild = FarVer.Build;
			_ASSERTE(FarVer.Major<=0xFFFF && FarVer.Minor<=0xFFFF)
			gFarVersion.dwVerMajor = (WORD)FarVer.Major;
			gFarVersion.dwVerMinor = (WORD)FarVer.Minor;
			gFarVersion.Bis = (FarVer.Stage==VS_BIS);
		}
		else
		{
			_ASSERTE(FarVer.Major == 3);
		}
	}

	//int nLen = lstrlenW(InfoW2800->RootKey)+16;
	//if (gszRootKey) free(gszRootKey);
	//gszRootKey = (wchar_t*)calloc(nLen,2);
	//lstrcpyW(gszRootKey, InfoW2800->RootKey);
	//WCHAR* pszSlash = gszRootKey+lstrlenW(gszRootKey)-1;
	//if (*pszSlash != L'\\') *(++pszSlash) = L'\\';
	//lstrcpyW(pszSlash+1, L"ConEmuTh\\");
}

void SetCurrentPanelItemW2800Int(BOOL abLeftPanel, INT_PTR anTopItem, INT_PTR anCurItem);
#define SetCurItem L"SetCurItem"

extern BOOL gbInfoW_OK;
HANDLE OpenW2800(const void* aInfo)
{
	_ASSERTE(gFarVersion.dwVerMajor >= 3)
	HANDLE hResult = NULL;

	const struct OpenInfo *Info = (OpenInfo*)aInfo;

	if (!gbInfoW_OK)
		return hResult;

	INT_PTR Item = Info->Data;
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
					if (lstrcmp(p->Values[0].String, SetCurItem) == 0)
					{
						if (p->Count >= 4)
						{
							INT_PTR abLeftPanel = (INT_PTR)p->Values[1].Double;
							INT_PTR anTopItem = (INT_PTR)p->Values[2].Double;
							INT_PTR anCurItem = (INT_PTR)p->Values[3].Double;
							SetCurrentPanelItemW2800Int(abLeftPanel!=0, anTopItem, anCurItem);
						}
					}
					return PANEL_NONE;
				default:
					_ASSERTE(p->Values[0].Type==FMVT_INTEGER || p->Values[0].Type==FMVT_STRING);
				}
			}
		}
		else
		{
			_ASSERTE(p->StructSize >= sizeof(*p));
		}
	}

	return OpenPluginWcmn(Info->OpenFrom, Item, (Info->OpenFrom == OPEN_FROMMACRO));
}

// Common
int ProcessSynchroEventCommon(int Event, void *Param);

#if defined(_M_ARM64) || defined(__aarch64__)
extern "C" __declspec(dllexport) intptr_t WINAPI ProcessConsoleInputW(struct ProcessConsoleInputInfo* info)
{
	if (info)
	{
		const auto& input = info->Rec;
		if ((input.EventType == KEY_EVENT && input.Event.KeyEvent.bKeyDown)
			|| (input.EventType == MOUSE_EVENT && input.Event.MouseEvent.dwEventFlags != MOUSE_MOVED))
		{
			// Queue the refresh so Far can apply the input before we read the panel.
			RefreshPanelViewsAfterInput();
		}
	}
	return 0;
}
#endif

// Far3 export
INT_PTR WINAPI ProcessSynchroEventW3(void* p)
{
	const ProcessSynchroEventInfo *Info = (const ProcessSynchroEventInfo*)p;
	return ProcessSynchroEventCommon(Info->Event, Info->Param);
}

void ExitFARW2800(void)
{
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
}

int ShowMessageW2800(LPCWSTR asMsg, int aiButtons)
{
	if (!InfoW2800 || !InfoW2800->Message)
		return -1;
	
	GUID lguid_ShowMsg = { /* d24045b3-93e2-4310-931d-717c0894d741 */
	    0xd24045b3,
	    0x93e2,
	    0x4310,
	    {0x93, 0x1d, 0x71, 0x7c, 0x08, 0x94, 0xd7, 0x41}
	};

	return InfoW2800->Message(&guid_ConEmuTh, &lguid_ShowMsg, FMSG_ALLINONE1900|FMSG_MB_OK|FMSG_WARNING, NULL,
	                         (const wchar_t * const *)asMsg, 0, aiButtons);
}

int ShowMessageW2800(int aiMsg, int aiButtons)
{
	if (!InfoW2800 || !InfoW2800->Message || !InfoW2800->GetMsg)
		return -1;

	return ShowMessageW2800(
	           (LPCWSTR)InfoW2800->GetMsg(&guid_ConEmuTh,aiMsg), aiButtons);
}

LPCWSTR GetMsgW2800(int aiMsg)
{
	if (!InfoW2800 || !InfoW2800->GetMsg)
		return L"";

	return InfoW2800->GetMsg(&guid_ConEmuTh,aiMsg);
}

// Warning, напрямую НЕ вызывать. Пользоваться "общей" PostMacro
void PostMacroW2800(wchar_t* asMacro)
{
	if (!InfoW2800 || !InfoW2800->AdvControl)
		return;

	//ActlKeyMacro mcr;
	MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
	//mcr.Command = MCMD_POSTMACROSTRING;
	//mcr.Param.PlainText.Flags = 0; // По умолчанию - вывод на экран разрешен

	if (*asMacro == L'@' && asMacro[1] && asMacro[1] != L' ')
	{
		asMacro ++;
	}
	else
	{
		mcr.Flags |= KMFLAGS_ENABLEOUTPUT;
	}

	mcr.SequenceText = asMacro;
	InfoW2800->MacroControl(&guid_ConEmuTh, MCTL_SENDSTRING, 0, &mcr);
}

int ShowPluginMenuW2800()
{
	if (!InfoW2800)
		return -1;

	FarMenuItem items[] =
	{
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW2800->GetMsg(&guid_ConEmuTh,CEMenuThumbnails)},
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW2800->GetMsg(&guid_ConEmuTh,CEMenuTiles)},
		{ghConEmuRoot ? 0 : MIF_DISABLE,  InfoW2800->GetMsg(&guid_ConEmuTh,CEMenuTurnOff)},
		{(ghConEmuRoot && (gFarVersion.Bis)) ? 0 : MIF_DISABLE,  InfoW2800->GetMsg(&guid_ConEmuTh,CEMenuIcons)},
	};
	size_t nCount = countof(items);
	CeFullPanelInfo* pi = GetFocusedThumbnails();

	if (!pi)
	{
		items[0].Flags |= MIF_SELECTED;
	}
	else
	{
		if (pi->PVM == pvm_Thumbnails)
		{
			items[0].Flags |= MIF_SELECTED|MIF_CHECKED;
		}
		else if (pi->PVM == pvm_Tiles)
		{
			items[1].Flags |= MIF_SELECTED|MIF_CHECKED;
		}
		else if (pi->PVM == pvm_Icons)
		{
			items[2].Flags |= MIF_SELECTED|MIF_CHECKED;
		}
		else
		{
			items[0].Flags |= MIF_SELECTED;
		}
	}

	#ifndef _DEBUG
	nCount--;
	#endif
	
	GUID lguid_TypeMenu = { /* f3e4df2c-7ecc-42db-ba2e-6f43f7cd9415 */
	    0xf3e4df2c,
	    0x7ecc,
	    0x42db,
	    {0xba, 0x2e, 0x6f, 0x43, 0xf7, 0xcd, 0x94, 0x15}
	};

	int nRc = InfoW2800->Menu(&guid_ConEmuTh, &lguid_TypeMenu, -1,-1, 0,
	                         FMENU_AUTOHIGHLIGHT|FMENU_CHANGECONSOLETITLE|FMENU_WRAPMODE,
	                         InfoW2800->GetMsg(&guid_ConEmuTh,2),
	                         NULL, NULL, NULL, NULL, (FarMenuItem*)items, nCount);
	return nRc;
}

BOOL IsMacroActiveW2800()
{
	if (!InfoW2800) return FALSE;

	INT_PTR liRc = InfoW2800->MacroControl(&guid_ConEmuTh, MCTL_GETSTATE, 0, 0);

	if (liRc == MACROSTATE_NOMACRO)
		return FALSE;

	return TRUE;
}

int GetMacroAreaW2800()
{
	int nArea = (int)InfoW2800->MacroControl(&guid_ConEmuTh, MCTL_GETAREA, 0, 0);
	return nArea;
}

void FreePanelItemUserData2800(HANDLE hPlugin, DWORD_PTR UserData, FARPROC FreeUserDataCallback)
{
	if (hPlugin && UserData && FreeUserDataCallback)
	{
		FARPANELITEMFREECALLBACK FreeData = (FARPANELITEMFREECALLBACK)FreeUserDataCallback;
		FarPanelItemFreeInfo fi = {sizeof(fi), hPlugin};
		FreeData((void*)UserData, &fi);
	}
}

void LoadPanelItemInfoW2800(CeFullPanelInfo* pi, INT_PTR nItem)
{
	//HANDLE hPanel = pi->hPanel;
	HANDLE hPanel = pi->Focus ? PANEL_ACTIVE : PANEL_PASSIVE;
	size_t nSize = InfoW2800->PanelControl(hPanel, FCTL_GETPANELITEM, (int)nItem, NULL);
	//PluginPanelItem *ppi = (PluginPanelItem*)malloc(nMaxSize);
	//if (!ppi)
	//	return;

	PanelInfo fpi = {sizeof(fpi)};
	InfoW2800->PanelControl(hPanel, FCTL_GETPANELINFO, 0, &fpi);

	if ((pi->pFarTmpBuf == NULL) || (pi->nFarTmpBuf < nSize))
	{
		if (pi->pFarTmpBuf) free(pi->pFarTmpBuf);

		pi->nFarTmpBuf = nSize+MAX_PATH; // + про запас немножко //-V101
		pi->pFarTmpBuf = malloc(pi->nFarTmpBuf);
	}

	PluginPanelItem *ppi = (PluginPanelItem*)pi->pFarTmpBuf;

	if (ppi)
	{
		FarGetPluginPanelItem gppi = {sizeof(gppi), nSize, ppi};
		nSize = InfoW2800->PanelControl(hPanel, FCTL_GETPANELITEM, (int)nItem, &gppi);
	}
	else
	{
		return;
	}

	if (!nSize)  // ошибка?
	{
		// FAR не смог заполнить ppi данными, поэтому накидаем туда нулей, чтобы мусор не рисовать
		ppi->FileName = L"???";
		ppi->Flags = 0;
		ppi->NumberOfLinks = 0;
		ppi->FileAttributes = 0;
		ppi->LastWriteTime.dwLowDateTime = ppi->LastWriteTime.dwHighDateTime = 0;
		ppi->FileSize = 0;
	}

	// Скопировать данные в наш буфер (функция сама выделит память)
	const wchar_t* pszName = ppi->FileName;

	if ((!pszName || !*pszName) && ppi->AlternateFileName && *ppi->AlternateFileName)
		pszName = ppi->AlternateFileName;
	else if (pi->ShortNames && ppi->AlternateFileName && *ppi->AlternateFileName)
		pszName = ppi->AlternateFileName;

	pi->FarItem2CeItem(nItem,
	                   pszName,
	                   ppi->Description,
	                   ppi->FileAttributes,
	                   ppi->LastWriteTime,
	                   ppi->FileSize,
	                   (pi->bPlugin && (pi->Flags & CEPFLAGS_REALNAMES) == 0) /*abVirtualItem*/,
					   fpi.PluginHandle,
	                   (DWORD_PTR)ppi->UserData.Data,
					   (FARPROC)ppi->UserData.FreeData,
	                   ppi->Flags,
	                   ppi->NumberOfLinks);
	// ppi не освобождаем - это ссылка на pi->pFarTmpBuf

	nSize = 0;
	FarGetPluginPanelItemInfo gppi = {sizeof(gppi)};
	// FCTL_GETPANELITEMINFO
	if (gFarVersion.Bis)
		nSize = InfoW2800->PanelControl(hPanel, (FILE_CONTROL_COMMANDS)1001, (int)nItem, &gppi);
	#ifdef _DEBUG
	else if (gFarVersion.dwBuild >= 4127) // Mantis#1886
		nSize = InfoW2800->PanelControl(hPanel, (FILE_CONTROL_COMMANDS)36, (int)nItem, &gppi);
	#endif
	// Succeeded or not?
	if (nSize == sizeof(FarGetPluginPanelItemInfo))
	{
		pi->BisItem2CeItem(nItem, TRUE, gppi.Color.Flags, gppi.Color.ForegroundColor, gppi.Color.BackgroundColor, gppi.PosX, gppi.PosY);
	}
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

static void LoadFarSettingsW2800(CEFarInterfaceSettings* pInterface, CEFarPanelSettings* pPanel)
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

BOOL LoadPanelInfoW2800(BOOL abActive)
{
	if (!InfoW2800) return FALSE;

	CeFullPanelInfo* pcefpi = NULL;
	PanelInfo pi = {sizeof(pi)};
	HANDLE hPanel = abActive ? PANEL_ACTIVE : PANEL_PASSIVE;
	INT_PTR nRc = InfoW2800->PanelControl(hPanel, FCTL_GETPANELINFO, 0, &pi);

	if (!nRc)
	{
		TODO("Показать информацию об ошибке");
		return FALSE;
	}

	// Даже если невидима - обновить информацию!
	//// Проверим, что панель видима. Иначе - сразу выходим.
	//if (!pi.Visible) {
	//	TODO("Показать информацию об ошибке");
	//	return NULL;
	//}

	if (pi.Flags & PFLAGS_PANELLEFT)
		pcefpi = &pviLeft;
	else
		pcefpi = &pviRight;

	pcefpi->cbSize = sizeof(*pcefpi);
	//pcefpi->hPanel = hPanel;

	// Если элементов на панели стало больше, чем выделено в (pviLeft/pviRight)
	if (pcefpi->ItemsNumber < (INT_PTR)pi.ItemsNumber)
	{
		if (!pcefpi->ReallocItems(pi.ItemsNumber))
			return FALSE;
	}

	// Копируем что нужно
	pcefpi->bLeftPanel = (pi.Flags & PFLAGS_PANELLEFT) == PFLAGS_PANELLEFT;
	pcefpi->bPlugin = (pi.Flags & PFLAGS_PLUGIN) == PFLAGS_PLUGIN;
	pcefpi->PanelRect = pi.PanelRect;
	pcefpi->ItemsNumber = pi.ItemsNumber;
	pcefpi->CurrentItem = pi.CurrentItem;
	pcefpi->TopPanelItem = pi.TopPanelItem;
	pcefpi->Visible = (pi.PanelType == PTYPE_FILEPANEL) && ((pi.Flags & PFLAGS_VISIBLE) == PFLAGS_VISIBLE);
	pcefpi->ShortNames = (pi.Flags & PFLAGS_ALTERNATIVENAMES) == PFLAGS_ALTERNATIVENAMES;
	pcefpi->Focus = (pi.Flags & PFLAGS_FOCUS) == PFLAGS_FOCUS;
	pcefpi->Flags = pi.Flags; // CEPANELINFOFLAGS
	pcefpi->PanelMode = pi.ViewMode;
	pcefpi->IsFilePanel = (pi.PanelType == PTYPE_FILEPANEL);
	// Настройки интерфейса
	LoadFarSettingsW2800(&pcefpi->FarInterfaceSettings, &pcefpi->FarPanelSettings);

	// Цвета фара
	INT_PTR nColorSize = InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETARRAYCOLOR, 0, NULL);
#ifdef _DEBUG
	INT_PTR nDefColorSize = COL_LASTPALETTECOLOR;
	_ASSERTE(nColorSize==nDefColorSize);
#endif
	FarColor* pColors = (FarColor*)calloc(nColorSize, sizeof(*pColors));
	if (pColors)
		nColorSize = InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETARRAYCOLOR, (int)nColorSize, pColors);
	WARNING("Поддержка более 4бит цветов");
	if (pColors && nColorSize > 0)
	{
		pcefpi->nFarColors[col_PanelText] = FarColor_3_2(pColors[COL_PANELTEXT]);
		pcefpi->nFarColors[col_PanelSelectedCursor] = FarColor_3_2(pColors[COL_PANELSELECTEDCURSOR]);
		pcefpi->nFarColors[col_PanelSelectedText] = FarColor_3_2(pColors[COL_PANELSELECTEDTEXT]);
		pcefpi->nFarColors[col_PanelCursor] = FarColor_3_2(pColors[COL_PANELCURSOR]);
		pcefpi->nFarColors[col_PanelColumnTitle] = FarColor_3_2(pColors[COL_PANELCOLUMNTITLE]);
		pcefpi->nFarColors[col_PanelBox] = FarColor_3_2(pColors[COL_PANELBOX]);
		pcefpi->nFarColors[col_HMenuText] = FarColor_3_2(pColors[COL_HMENUTEXT]);
		pcefpi->nFarColors[col_WarnDialogBox] = FarColor_3_2(pColors[COL_WARNDIALOGBOX]);
		pcefpi->nFarColors[col_DialogBox] = FarColor_3_2(pColors[COL_DIALOGBOX]);
		pcefpi->nFarColors[col_CommandLineUserScreen] = FarColor_3_2(pColors[COL_COMMANDLINEUSERSCREEN]);
		pcefpi->nFarColors[col_PanelScreensNumber] = FarColor_3_2(pColors[COL_PANELSCREENSNUMBER]);
		pcefpi->nFarColors[col_KeyBarNum] = FarColor_3_2(pColors[COL_KEYBARNUM]);
	}
	else
	{
		_ASSERTE(pColors!=NULL && nColorSize>0);
		memset(pcefpi->nFarColors, 7, countof(pcefpi->nFarColors)*sizeof(*pcefpi->nFarColors));
	}
	SafeFree(pColors);
	//int nColorSize = InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETARRAYCOLOR, 0, NULL);
	//if ((pcefpi->nFarColors == NULL) || (nColorSize > pcefpi->nMaxFarColors))
	//{
	//	if (pcefpi->nFarColors) free(pcefpi->nFarColors);
	//	pcefpi->nFarColors = (BYTE*)calloc(nColorSize,1);
	//	pcefpi->nMaxFarColors = nColorSize;
	//}
	////nColorSize = InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETARRAYCOLOR, 0, pcefpi->nFarColors);
	//FarColor* pColors = (FarColor*)calloc(nColorSize, sizeof(*pColors));
	//
	//if (pColors)
	//	nColorSize = InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETARRAYCOLOR, nColorSize, pColors);
	//
	//WARNING("Поддержка более 4бит цветов");
	//if (pColors && nColorSize > 0)
	//{
	//	for (int i = 0; i < nColorSize; i++)
	//		pcefpi->nFarColors[i] = FarColor_3_2(pColors[i]);
	//}
	//else
	//{
	//	memset(pcefpi->nFarColors, 7, pcefpi->nMaxFarColors*sizeof(*pcefpi->nFarColors));
	//}
	//SafeFree(pColors);
	
	// Текущая папка панели
	size_t nSize = InfoW2800->PanelControl(hPanel, FCTL_GETPANELDIRECTORY, 0, 0);

	if (nSize)
	{
		if ((pcefpi->pFarPanelDirectory == NULL) || (nSize > pcefpi->nMaxPanelGetDir))
		{
			pcefpi->nMaxPanelGetDir = nSize + 1024; // + выделим немножко заранее
			pcefpi->pFarPanelDirectory = calloc(pcefpi->nMaxPanelGetDir,1);
		}
		((FarPanelDirectory*)pcefpi->pFarPanelDirectory)->StructSize = sizeof(FarPanelDirectory);
		nSize = InfoW2800->PanelControl(hPanel, FCTL_GETPANELDIRECTORY, nSize, pcefpi->pFarPanelDirectory);

		if ((pcefpi->pszPanelDir == NULL) || (nSize > pcefpi->nMaxPanelDir))
		{
			pcefpi->nMaxPanelDir = nSize + MAX_PATH; // + выделим немножко заранее
			SafeFree(pcefpi->pszPanelDir);
			pcefpi->pszPanelDir = (wchar_t*)calloc(pcefpi->nMaxPanelDir,2);
		}
		lstrcpyn(pcefpi->pszPanelDir, ((FarPanelDirectory*)pcefpi->pFarPanelDirectory)->Name, pcefpi->nMaxPanelDir);

		if (!nSize)
		{
			SafeFree(pcefpi->pszPanelDir);
			pcefpi->nMaxPanelDir = 0;
		}
	}
	else
	{
		SafeFree(pcefpi->pszPanelDir);
	}

	// Готовим буфер для информации об элементах
	pcefpi->ReallocItems(pcefpi->ItemsNumber);

	// и буфер для загрузки элемента из FAR
	nSize = sizeof(PluginPanelItem)+6*MAX_PATH;

	if ((pcefpi->pFarTmpBuf == NULL) || (pcefpi->nFarTmpBuf < nSize))
	{
		if (pcefpi->pFarTmpBuf) free(pcefpi->pFarTmpBuf);

		pcefpi->nFarTmpBuf = nSize;
		pcefpi->pFarTmpBuf = malloc(pcefpi->nFarTmpBuf);
	}

	return TRUE;
}

void ReloadPanelsInfoW2800()
{
	if (!InfoW2800) return;

	// в FAR3 все просто
	LoadPanelInfoW2800(TRUE);
	LoadPanelInfoW2800(FALSE);
}

void SetCurrentPanelItemW2800(BOOL abLeftPanel, INT_PTR anTopItem, INT_PTR anCurItem)
{
	if (!InfoW2800) return;

	wchar_t szMacro[200], szTop[65], szCur[65];
	FSFW2800->itoa64(anTopItem, szTop, 10);
	FSFW2800->itoa64(anCurItem, szCur, 10);
	LPCWSTR pszEsc, pszFn;
	if (gFarVersion.IsFarLua())
	{
		pszEsc = L"if Area.Search then Keys(\"Esc\") end";
		pszFn = L"Plugin.SyncCall";
	}
	else
	{
		pszEsc = L"$if (Search) Esc $end";
		pszFn = L"callplugin";
	}
	swprintf_c(szMacro, L"%s %s(\"bd454d48-448e-46cc-909d-b6cf789c2d65\",\"%s\",%u,%s,%s)",
			pszEsc, pszFn, SetCurItem, abLeftPanel, szTop, szCur);

	MacroSendMacroText mcr = {sizeof(MacroSendMacroText)};
	mcr.Flags = KMFLAGS_NOSENDKEYSTOPLUGINS;
	mcr.SequenceText = szMacro;
	InfoW2800->MacroControl(&guid_ConEmuTh, MCTL_SENDSTRING, 0, &mcr);
}

void SetCurrentPanelItemW2800Int(BOOL abLeftPanel, INT_PTR anTopItem, INT_PTR anCurItem)
{
	// В Far2 можно быстро проверить валидность индексов
	HANDLE hPanel = NULL;
	PanelInfo piActive = {sizeof(piActive)}, piPassive = {sizeof(piActive)}, *pi = NULL;
	TODO("Проверять текущую видимость панелей?");
	InfoW2800->PanelControl(PANEL_ACTIVE,  FCTL_GETPANELINFO, 0, &piActive);

	if ((piActive.Flags & PFLAGS_PANELLEFT) == (abLeftPanel ? PFLAGS_PANELLEFT : 0))
	{
		pi = &piActive; hPanel = PANEL_ACTIVE;
	}
	else
	{
		InfoW2800->PanelControl(PANEL_PASSIVE, FCTL_GETPANELINFO, 0, &piPassive);
		pi = &piPassive; hPanel = PANEL_PASSIVE;
	}

	// Проверяем индексы (может фар в процессе обновления панели, и количество элементов изменено?)
	if (pi->ItemsNumber < 1)
		return;

	if (anTopItem >= (INT_PTR)pi->ItemsNumber)
		anTopItem = pi->ItemsNumber - 1;

	if (anCurItem >= (INT_PTR)pi->ItemsNumber)
		anCurItem = pi->ItemsNumber - 1;

	if (anCurItem < anTopItem)
		anCurItem = anTopItem;

	// Обновляем панель
	#pragma warning(disable: 4244)
	PanelRedrawInfo pri = {sizeof(pri), anCurItem, anTopItem};
	#pragma warning(default: 4244)
	InfoW2800->PanelControl(hPanel, FCTL_REDRAWPANEL, 0, &pri);
}

//BOOL IsLeftPanelActiveW2800()
//{
//	WARNING("TODO: IsLeftPanelActiveW2800");
//	return TRUE;
//}

BOOL CheckPanelSettingsW2800(BOOL abSilence)
{
	if (!InfoW2800)
		return FALSE;

	LoadFarSettingsW2800(&gFarInterfaceSettings, &gFarPanelSettings);

	if (!(gFarPanelSettings.ShowColumnTitles))
	{
		// Для корректного определения положения колонок необходим один из флажков в настройке панели:
		// [x] Показывать заголовки колонок [x] Показывать суммарную информацию
		if (!abSilence)
		{
			GUID lguid_PanelErr = { /* ba9380d3-af0b-4d9f-ad12-d8d548bf7519 */
			    0xba9380d3,
			    0xaf0b,
			    0x4d9f,
			    {0xad, 0x12, 0xd8, 0xd5, 0x48, 0xbf, 0x75, 0x19}
			};
			InfoW2800->Message(&guid_ConEmuTh, &lguid_PanelErr, FMSG_ALLINONE1900|FMSG_MB_OK|FMSG_WARNING|FMSG_LEFTALIGN1900, NULL,
			                  (const wchar_t * const *)InfoW2800->GetMsg(&guid_ConEmuTh,CEInvalidPanelSettings), 0, 0);
		}

		return FALSE;
	}

	return TRUE;
}

void ExecuteInMainThreadW2800(ConEmuThSynchroArg* pCmd)
{
	if (!InfoW2800) return;

	InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_SYNCHRO, 0, pCmd);
}

void GetFarRectW2800(SMALL_RECT* prcFarRect)
{
	if (!InfoW2800) return;

	_ASSERTE(ACTL_GETFARRECT!=32); //-V112
	if (!InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETFARRECT, 0, prcFarRect))
	{
		prcFarRect->Left = prcFarRect->Right = prcFarRect->Top = prcFarRect->Bottom = 0;
	}
}

// Использовать только ACTL_GETWINDOWTYPE. С ней проблем с потоками быть не должно
bool CheckFarPanelsW2800()
{
	if (!InfoW2800 || !InfoW2800->AdvControl) return false;

	WindowType wt = {sizeof(WindowType)};
	bool lbPanelsActive = false;
	//_ASSERTE(GetCurrentThreadId() == gnMainThreadId); -- ACTL_GETWINDOWTYPE - thread safe
	INT_PTR iRc = InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETWINDOWTYPE, 0, &wt);
	lbPanelsActive = (iRc != 0)
		&& ((wt.Type == WTYPE_PANELS)
			|| (gFarVersion.Bis && (gFarVersion.dwBuild >= 4188) && (wt.Type == WTYPE_SEARCH)));

	return lbPanelsActive;
}

// Возникали проблемы с синхронизацией в FAR2 -> FindFile
//bool CheckWindowsW2800()
//{
//	if (!InfoW2800 || !InfoW2800->AdvControl) return false;
//
//	bool lbPanelsActive = false;
//	int nCount = InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETWINDOWCOUNT, NULL);
//	WindowInfo wi = {0};
//
//	wi.Pos = -1;
//	INT_PTR iRc = InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETWINDOWINFO, (LPVOID)&wi);
//	if (wi.Type == WTYPE_PANELS) {
//		lbPanelsActive = (wi.Current != 0);
//	}
//
//	//wchar_t szTypeName[64];
//	//wchar_t szName[MAX_PATH*2];
//	//wchar_t szInfo[MAX_PATH*4];
//	//
//	//OutputDebugStringW(L"\n\n");
//	//// Pos: Номер окна, о котором нужно узнать информацию. Нумерация идет с 0. Pos = -1 вернет информацию о текущем окне.
//	//for (int i=-1; i <= nCount; i++) {
//	//	memset(&wi, 0, sizeof(wi));
//	//	wi.Pos = i;
//	//	wi.TypeName = szTypeName; wi.TypeNameSize = sizeofarray(szTypeName); szTypeName[0] = 0;
//	//	wi.Name = szName; wi.NameSize = sizeofarray(szName); szName[0] = 0;
//	//	INT_PTR iRc = InfoW2800->AdvControl(&guid_ConEmuTh, ACTL_GETWINDOWINFO, (LPVOID)&wi);
//	//	if (iRc) {
//	//		StringCchPrintf(szInfo, countof(szInfo), L"%s%i: {%s-%s} %s\n",
//	//			wi.Current ? L"*" : L"",
//	//			i,
//	//			(wi.Type==WTYPE_PANELS) ? L"WTYPE_PANELS" :
//	//			(wi.Type==WTYPE_VIEWER) ? L"WTYPE_VIEWER" :
//	//			(wi.Type==WTYPE_EDITOR) ? L"WTYPE_EDITOR" :
//	//			(wi.Type==WTYPE_DIALOG) ? L"WTYPE_DIALOG" :
//	//			(wi.Type==WTYPE_VMENU)  ? L"WTYPE_VMENU"  :
//	//			(wi.Type==WTYPE_HELP)   ? L"WTYPE_HELP"   : L"Unknown",
//	//			szTypeName, szName);
//	//	} else {
//	//		StringCchPrintf(szInfo, countof(szInfo), L"%i: <window absent>\n", i);
//	//	}
//	//	OutputDebugStringW(szInfo);
//	//}
//
//	return lbPanelsActive;
//}

BOOL SettingsLoadW2800(LPCWSTR pszName, DWORD* pValue)
{
	BOOL lbValue = FALSE;
	FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_ConEmuTh, INVALID_HANDLE_VALUE};
	FarSettingsItem fsi = {sizeof(fsi)};
	if (InfoW2800->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc))
	{
		//DWORD nValue = *pValue;
		fsi.Name = pszName;
		fsi.Type = FST_DATA;
		//fsi.Data.Size = sizeof(nValue); 
		//fsi.Data.Data = &nValue;
		if (InfoW2800->SettingsControl(sc.Handle, SCTL_GET, 0, &fsi) && (fsi.Data.Size == sizeof(DWORD)))
		{
			*pValue = *((DWORD*)fsi.Data.Data);
		}

		InfoW2800->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
	return lbValue;
}
void SettingsSaveW2800(LPCWSTR pszName, DWORD* pValue)
{
	if (!InfoW2800)
		return;

	FarSettingsCreate sc = {sizeof(FarSettingsCreate), guid_ConEmuTh, INVALID_HANDLE_VALUE};
	FarSettingsItem fsi = {sizeof(fsi)};
	if (InfoW2800->SettingsControl(INVALID_HANDLE_VALUE, SCTL_CREATE, 0, &sc))
	{
		DWORD nValue = *pValue;
		fsi.Name = pszName;
		fsi.Type = FST_DATA;
		fsi.Data.Size = sizeof(nValue); 
		fsi.Data.Data = &nValue;
		InfoW2800->SettingsControl(sc.Handle, SCTL_SET, 0, &fsi);

		InfoW2800->SettingsControl(sc.Handle, SCTL_FREE, 0, 0);
	}
}

void SettingsLoadOtherW2800(void)
{
	WARNING("Как появится PanelTabs для Far3 - переделать чтение его настроек в GUID");
}
