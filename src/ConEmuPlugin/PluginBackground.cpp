
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


#define SHOWDEBUGSTR
//#define MCHKHEAP
#define DEBUGSTRBG(s) //DEBUGSTR(s)

#define THSET_TIMEOUT_PANELS 100
#define THSET_TIMEOUT_VIEWER 1500
#undef CREATE_EMF_TEMP_FILES

#define SETBACKGROUND_USE_EMF 1 // Use metafiles
//#define SETBACKGROUND_USE_EMF 0 // Use BITMAP

#undef HIDE_TODO

#include "../common/Common.h"
#include "../common/MSection.h"
#include "../common/MFileMapping.h"
#include "../ConEmuHk/ConEmuHooks.h"
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginW1761.hpp"
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#include "../common/ConsoleAnnotation.h"
#include "../common/WModuleCheck.h"
#include "PluginHeader.h"
#include "PluginBackground.h"
#include "ConEmuPluginBase.h"

#include "../common/ConEmuCheck.h"

TODO("При завершении нити отрисовки - нужно дернуть все зарегистрированные PaintConEmuBackground(pbp_Finalize)");

BOOL gbNeedBgActivate = FALSE; // требуется активация модуля в главной нити
BOOL gbBgPluginsAllowed = FALSE;
CPluginBackground *gpBgPlugin = NULL;

extern BOOL WINAPI IsConsoleActive();

CPluginBackground::CPluginBackground()
{
	mn_BgPluginsCount = 0;
	mn_BgPluginsMax = 16;
	mp_BgPlugins = (struct RegisterBackgroundArg*)calloc(mn_BgPluginsMax,sizeof(struct RegisterBackgroundArg));
	mn_ReqActions = 0;
	// Инициализируем в TRUE, потому что иначе при запуске второго фара (far/e)
	// как внешний редактор, в ConEmu останется фон от предыдущей копии фара.
	// Пусть запущенная копия всегда очищает при старте, если плагинов нет.
	mb_BgWasSent = TRUE;
	mb_BgErrorShown = FALSE;
	csBgPlugins = new MSection();
	m_LastThSetCheck = 0;
	memset(&m_ThSet, 0, sizeof(m_ThSet));
	memset(&m_Default, 0, sizeof(m_Default));
	m_Default.LeftPanel.szCurDir = (wchar_t*)calloc(BkPanelInfo_CurDirMax,sizeof(wchar_t));
	m_Default.LeftPanel.szFormat = (wchar_t*)calloc(BkPanelInfo_FormatMax,sizeof(wchar_t));
	m_Default.LeftPanel.szHostFile = (wchar_t*)calloc(BkPanelInfo_HostFileMax,sizeof(wchar_t));
	m_Default.RightPanel.szCurDir = (wchar_t*)calloc(BkPanelInfo_CurDirMax,sizeof(wchar_t));
	m_Default.RightPanel.szFormat = (wchar_t*)calloc(BkPanelInfo_FormatMax,sizeof(wchar_t));
	m_Default.RightPanel.szHostFile = (wchar_t*)calloc(BkPanelInfo_HostFileMax,sizeof(wchar_t));
	memset(&m_Last, 0, sizeof(m_Last));
	m_Last.LeftPanel.szCurDir = (wchar_t*)calloc(BkPanelInfo_CurDirMax,sizeof(wchar_t));
	m_Last.LeftPanel.szFormat = (wchar_t*)calloc(BkPanelInfo_FormatMax,sizeof(wchar_t));
	m_Last.LeftPanel.szHostFile = (wchar_t*)calloc(BkPanelInfo_HostFileMax,sizeof(wchar_t));
	m_Last.RightPanel.szCurDir = (wchar_t*)calloc(BkPanelInfo_CurDirMax,sizeof(wchar_t));
	m_Last.RightPanel.szFormat = (wchar_t*)calloc(BkPanelInfo_FormatMax,sizeof(wchar_t));
	m_Last.RightPanel.szHostFile = (wchar_t*)calloc(BkPanelInfo_HostFileMax,sizeof(wchar_t));
	//m_Default.pszLeftCurDir    = ms_LeftCurDir; ms_LeftCurDir[0] = 0;
	//m_Default.pszLeftFormat    = ms_LeftFormat; ms_LeftFormat[0] = 0;
	//m_Default.pszLeftHostFile  = ms_LeftHostFile; ms_LeftHostFile[0] = 0;
	//m_Default.pszRightCurDir   = ms_RightCurDir; ms_RightCurDir[0] = 0;
	//m_Default.pszRightFormat   = ms_RightFormat; ms_RightFormat[0] = 0;
	//m_Default.pszRightHostFile = ms_RightHostFile; ms_RightHostFile[0] = 0;
	mb_ThNeedLoad = TRUE;

	if (ghConEmuWndDC && IsWindow(ghConEmuWndDC))
	{
		LoadThSet(FALSE);
	}

	SetForceCheck();
}

CPluginBackground::~CPluginBackground()
{
	if (csBgPlugins)
	{
		delete csBgPlugins;
		csBgPlugins = NULL;
	}

	SafeFree(m_Default.LeftPanel.szCurDir);
	SafeFree(m_Default.LeftPanel.szFormat);
	SafeFree(m_Default.LeftPanel.szHostFile);
	SafeFree(m_Default.RightPanel.szCurDir);
	SafeFree(m_Default.RightPanel.szFormat);
	SafeFree(m_Default.RightPanel.szHostFile);
	SafeFree(m_Last.LeftPanel.szCurDir);
	SafeFree(m_Last.LeftPanel.szFormat);
	SafeFree(m_Last.LeftPanel.szHostFile);
	SafeFree(m_Last.RightPanel.szCurDir);
	SafeFree(m_Last.RightPanel.szFormat);
	SafeFree(m_Last.RightPanel.szHostFile);
}

int CPluginBackground::RegisterSubplugin(RegisterBackgroundArg *pbk)
{
	if (!pbk)
	{
		_ASSERTE(pbk != NULL);
		return esbr_InvalidArg;
	}

	if (!gbBgPluginsAllowed)
	{
		_ASSERTE(gbBgPluginsAllowed == TRUE);
		return esbr_PluginForbidden;
	}

	if (pbk->cbSize != sizeof(*pbk))
	{
		_ASSERTE(pbk->cbSize == sizeof(*pbk));
		return esbr_InvalidArgSize;
	}

	if (pbk->Cmd == rbc_Register)
	{
		BOOL lbCheckCallback = CheckCallbackPtr(pbk->hPlugin, 1, (FARPROC*)&pbk->PaintConEmuBackground, TRUE, FALSE, FALSE);

		if (!lbCheckCallback)
		{
			_ASSERTE(lbCheckCallback==TRUE);
			return esbr_InvalidArgProc;
		}
	}

	MSectionLock SC; SC.Lock(csBgPlugins, TRUE);

	// Выделение памяти под плагины
	if (mp_BgPlugins == NULL || (mn_BgPluginsCount == mn_BgPluginsMax))
		ReallocItems(16);

	// go
	//BOOL lbNeedResort = FALSE;

	if (pbk->Cmd == rbc_Register)
	{
		// Память уже выделена
		int nFound = -1, nFirstEmpty = -1;

		for(int i = 0; i < mn_BgPluginsCount; i++)
		{
			if (mp_BgPlugins[i].Cmd == rbc_Register &&
			        mp_BgPlugins[i].hPlugin == pbk->hPlugin &&
			        mp_BgPlugins[i].PaintConEmuBackground == pbk->PaintConEmuBackground &&
			        mp_BgPlugins[i].lParam == pbk->lParam)
			{
				nFound = i; break;
			}

			if (nFirstEmpty == -1 && 0 == (int)mp_BgPlugins[i].Cmd)
				nFirstEmpty = i;
		}

		if (nFound == -1)
		{
			if (nFirstEmpty >= 0)
				nFound = nFirstEmpty;
			else
				nFound = mn_BgPluginsCount++;

			//lbNeedResort = (mn_BgPluginsCount>1);
		}

		mp_BgPlugins[nFound] = *pbk;
	}
	else if (pbk->Cmd == rbc_Unregister)
	{
		for(int i = 0; i < mn_BgPluginsCount; i++)
		{
			if (mp_BgPlugins[i].Cmd == rbc_Register &&
			        mp_BgPlugins[i].hPlugin == pbk->hPlugin &&
			        ((pbk->PaintConEmuBackground == NULL) ||
			         (mp_BgPlugins[i].PaintConEmuBackground == pbk->PaintConEmuBackground
			          && mp_BgPlugins[i].lParam == pbk->lParam)))
			{
				memset(mp_BgPlugins+i, 0, sizeof(*mp_BgPlugins));
				//lbNeedResort = TRUE;
			}
		}

		WARNING("Если количество зарегистрированных плагинов уменьшилось до 0");
		// Послать в GUI CECMD_SETBACKGROUND{bEnabled = FALSE}
		// Проверить, чтобы не было противного мелькания при закрытии FAR
	}
	else if (pbk->Cmd == rbc_Redraw)
	{
		// Refresh dwPlaces
		if (pbk->dwPlaces != pbp_None)
		{
			for (int i = 0; i < mn_BgPluginsCount; i++)
			{
				if (mp_BgPlugins[i].Cmd == rbc_Register &&
					mp_BgPlugins[i].hPlugin == pbk->hPlugin &&
					((pbk->PaintConEmuBackground == NULL) ||
						(mp_BgPlugins[i].PaintConEmuBackground == pbk->PaintConEmuBackground
							&& mp_BgPlugins[i].lParam == pbk->lParam)))
				{
					mp_BgPlugins[i].dwPlaces = pbk->dwPlaces;
				}
			}
		}
		// and just set gbNeedBgActivate
	}

	////TODO: Сортировка
	//if (lbNeedResort)
	//{
	//	WARNING("Сортировка зарегистрированных плагинов - CPluginBackground::RegisterBackground");
	//}
	SC.Unlock();
	// Когда активируется MainThread - обновить background
	mn_ReqActions |= ra_UpdateBackground;
	gbNeedBgActivate = TRUE;

	// В фар2 сразу дернем Synchro
	if (IS_SYNCHRO_ALLOWED)
	{
		Plugin()->ExecuteSynchro();
	}

	return esbr_OK;
}

void CPluginBackground::ReallocItems(int nAddCount)
{
	_ASSERTE(nAddCount > 0);
	int nNewMax = mn_BgPluginsMax + nAddCount;
	_ASSERTE(nNewMax > 0);
	struct RegisterBackgroundArg *pNew = (struct RegisterBackgroundArg*)calloc(nNewMax,sizeof(struct RegisterBackgroundArg));

	if (mp_BgPlugins)
	{
		if (mn_BgPluginsCount > 0)
			memmove(pNew, mp_BgPlugins, mn_BgPluginsCount*sizeof(struct RegisterBackgroundArg));

		free(mp_BgPlugins);
	}

	mp_BgPlugins = pNew;
	mn_BgPluginsMax = nNewMax;
}

void CPluginBackground::OnMainThreadActivated(int anEditorEvent /*= -1*/, int anViewerEvent /*= -1*/)
{
#ifdef _DEBUG
	DWORD nCurActions = mn_ReqActions;
#endif

	if (anEditorEvent != -1 || anViewerEvent != -1)
	{
		mn_ReqActions |= ra_CheckPanelFolders;
		// При закрытии QView нельзя перечитывать параметры панелей,
		// т.к. панель на месте QView еще НЕ видима, будет глюк отрисовки
		if (anViewerEvent == VE_CLOSE)
		{
			SetForceCheck();
			return;
		}
	}

	if ((mn_ReqActions & ra_CheckPanelFolders))
	{
		mn_ReqActions &= ~ra_CheckPanelFolders;
		CheckPanelFolders((anEditorEvent == EE_GOTFOCUS) ? pbp_Editor : (anViewerEvent == VE_GOTFOCUS) ? pbp_Viewer : 0);
	}
	else if ((anEditorEvent == EE_GOTFOCUS) || (anViewerEvent == VE_GOTFOCUS))
	{
		m_Default.Place = (anEditorEvent == EE_GOTFOCUS) ? pbp_Editor : pbp_Viewer;
		if (IsParmChanged(&m_Default, &m_Last))
		{
			mn_ReqActions |= ra_UpdateBackground;
		}
	}

	if ((mn_ReqActions & ra_UpdateBackground))
	{
		mn_ReqActions &= ~ra_UpdateBackground;
		UpdateBackground();
	}
}

void CPluginBackground::SetForceCheck()
{
	mn_ReqActions |= (ra_CheckPanelFolders);
	gbNeedBgActivate = TRUE;
}

void CPluginBackground::SetForceUpdate(bool bFlagsOnly /*= false*/)
{
	mn_ReqActions |= (ra_UpdateBackground|ra_CheckPanelFolders);

	if (!bFlagsOnly)
	{
		gbNeedBgActivate = TRUE;
	}
}

void CPluginBackground::SetForceThLoad()
{
	mb_ThNeedLoad = TRUE;
	mn_ReqActions |= (ra_UpdateBackground|ra_CheckPanelFolders);
	gbNeedBgActivate = TRUE;
}

bool CPluginBackground::IsParmChanged(struct PaintBackgroundArg* p1, struct PaintBackgroundArg* p2)
{
	if (!p1 || !p2)
		return false;

#if defined(_M_ARM64) || defined(__aarch64__)
	if (p1->Place != p2->Place)
		return true;
#endif

	if (p1->dcSizeX != p2->dcSizeX || p1->dcSizeY != p2->dcSizeY)
		return true;

	if (memcmp(&p1->rcConWorkspace, &p2->rcConWorkspace, sizeof(p1->rcConWorkspace))
	        || memcmp(&p1->conCursor, &p2->conCursor, sizeof(p1->conCursor))
	        || p1->FarInterfaceSettings.Raw != p2->FarInterfaceSettings.Raw
	        || p1->FarPanelSettings.Raw != p2->FarPanelSettings.Raw)
		return true;

	if (p1->bPanelsAllowed != p2->bPanelsAllowed)
		return true;

	if (p1->LeftPanel.bVisible != p2->LeftPanel.bVisible
	        || p1->RightPanel.bVisible != p2->RightPanel.bVisible)
		return true;

	if (p1->LeftPanel.bVisible)
	{
		if (p1->LeftPanel.bFocused != p2->LeftPanel.bFocused
		        || p1->LeftPanel.bPlugin != p2->LeftPanel.bPlugin
		        || memcmp(&p1->LeftPanel.rcPanelRect, &p2->LeftPanel.rcPanelRect, sizeof(p1->LeftPanel.rcPanelRect)))
			return true;

		if (lstrcmpW(p1->LeftPanel.szCurDir, p2->LeftPanel.szCurDir)
		        || lstrcmpW(p1->LeftPanel.szFormat, p2->LeftPanel.szFormat)
		        || lstrcmpW(p1->LeftPanel.szHostFile, p2->LeftPanel.szHostFile))
			return true;
	}

	if (p1->RightPanel.bVisible)
	{
		if (p1->RightPanel.bFocused != p2->RightPanel.bFocused
		        || p1->RightPanel.bPlugin != p2->RightPanel.bPlugin
		        || memcmp(&p1->RightPanel.rcPanelRect, &p2->RightPanel.rcPanelRect, sizeof(p1->RightPanel.rcPanelRect)))
			return true;

		if (lstrcmpW(p1->RightPanel.szCurDir, p2->RightPanel.szCurDir)
		        || lstrcmpW(p1->RightPanel.szFormat, p2->RightPanel.szFormat)
		        || lstrcmpW(p1->RightPanel.szHostFile, p2->RightPanel.szHostFile))
			return true;
	}

	if (memcmp(p1->nFarColors, p2->nFarColors, sizeof(p1->nFarColors))
	        || memcmp(p1->crPalette, p2->crPalette, sizeof(p1->crPalette)))
		return true;

	return false;
}

void CPluginBackground::CheckPanelFolders(int anForceSetPlace /*= 0*/)
{
	Plugin()->FillUpdateBackground(&m_Default);

	// Заполнить текущее "место"
	if (anForceSetPlace != 0)
		m_Default.Place = (PaintBackgroundPlaces)anForceSetPlace;
#if defined(_M_ARM64) || defined(__aarch64__)
	else
	{
		// A modal dialog sits over the editor or panels. Keep the background
		// of that underlying window until Far switches to another area.
		switch (Plugin()->GetActiveWindowType())
		{
		case WTYPE_PANELS: m_Default.Place = pbp_Panels; break;
		case WTYPE_EDITOR: m_Default.Place = pbp_Editor; break;
		case WTYPE_VIEWER: m_Default.Place = pbp_Viewer; break;
		}
	}
#else
	else if (!gpTabs)
		m_Default.Place = pbp_Panels;
	else if (gnCurrentWindowType == WTYPE_EDITOR)
		m_Default.Place = pbp_Editor;
	else if (gnCurrentWindowType == WTYPE_VIEWER)
		m_Default.Place = pbp_Viewer;
	else if (m_Default.LeftPanel.bVisible || m_Default.RightPanel.bVisible)
	{
		_ASSERTE(gnCurrentWindowType == WTYPE_PANELS);
		m_Default.Place = pbp_Panels;
	}
#endif

	if (IsParmChanged(&m_Default, &m_Last))
	{
		mn_ReqActions |= ra_UpdateBackground;
	}

	WARNING("Нужно бы перечитать его, при изменении данных в ConEmu!");

	if (mb_ThNeedLoad && ghConEmuWndDC)
	{
		LoadThSet(TRUE/*мы уже в главной нити*/);
	}
}

void CPluginBackground::SetDcPanelRect(RECT *rcDc, BkPanelInfo *Panel, PaintBackgroundArg *Arg)
{
	if (!Panel->bVisible)
	{
		rcDc->left = rcDc->top = rcDc->right = rcDc->bottom = -1;
	}
	else
	{
		int nConW = Arg->rcConWorkspace.right - Arg->rcConWorkspace.left + 1;
		int nConH = Arg->rcConWorkspace.bottom - Arg->rcConWorkspace.top + 1;

		if (nConW < 1)
		{
			_ASSERTE(nConW>0);
			nConW = 1;
		}

		if (nConH < 1)
		{
			_ASSERTE(nConH>0);
			nConH = 1;
		}

		int nShiftLeft = 0, nShiftTop = 0;
		if (Panel->rcPanelRect.top > 1)
		{
			_ASSERTE(Panel->rcPanelRect.top >= Arg->rcConWorkspace.top);
			nShiftLeft = Arg->rcConWorkspace.left;
			nShiftTop = Arg->rcConWorkspace.top;
		}

		rcDc->left = (Panel->rcPanelRect.left - nShiftLeft) * Arg->dcSizeX / nConW;
		rcDc->top = (Panel->rcPanelRect.top - nShiftTop) * Arg->dcSizeY / nConH;
		rcDc->right = (Panel->rcPanelRect.right - nShiftLeft + 1) * Arg->dcSizeX / nConW;
		rcDc->bottom = (Panel->rcPanelRect.bottom - nShiftTop + 1) * Arg->dcSizeY / nConH;
	}
}

void CPluginBackground::UpdateBackground_Exec(struct RegisterBackgroundArg *pPlugin, struct PaintBackgroundArg *pArg)
{
	if (!CheckCallbackPtr(pPlugin->hPlugin, 1, (FARPROC*)&pPlugin->PaintConEmuBackground, TRUE, FALSE, FALSE))
		return;

	pPlugin->PaintConEmuBackground(pArg);
	//if (!pPlugin->PaintConEmuBackground)
	//{
	//	_ASSERTE(pPlugin->PaintConEmuBackground!=NULL);
	//	return;
	//}
	//if (((DWORD_PTR)pPlugin->PaintConEmuBackground) < ((DWORD_PTR)pPlugin->hPlugin))
	//{
	//	_ASSERTE(((DWORD_PTR)pPlugin->PaintConEmuBackground) >= ((DWORD_PTR)pPlugin->hPlugin));
	//	return;
	//}
	//#ifdef _DEBUG
	//	if (((DWORD_PTR)pPlugin->PaintConEmuBackground) > ((4<<20) + (DWORD_PTR)pPlugin->hPlugin))
	//	{
	//		_ASSERTE(((DWORD_PTR)pPlugin->PaintConEmuBackground) <= ((4<<20) + (DWORD_PTR)pPlugin->hPlugin));
	//	}
	//#endif
}

// Вызывается ТОЛЬКО в главной нити!
void CPluginBackground::UpdateBackground()
{
	if (!mn_BgPluginsCount)
		return;

	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
		return;

	if (mb_ThNeedLoad)
	{
		LoadThSet(TRUE/*Мы уже в главной нити*/);
	}

	//RECT rcClient; GetClientRect(ghConEmuWndDC, &rcClient);
	struct PaintBackgroundArg Arg = m_Default;
	Arg.cbSize = sizeof(struct PaintBackgroundArg);
	//m_Default.dcSizeX = Arg.dcSizeX = rcClient.right+1;
	//m_Default.dcSizeY = Arg.dcSizeY = rcClient.bottom+1;
	m_Default.dcSizeX = Arg.dcSizeX = (m_Default.rcConWorkspace.right-m_Default.rcConWorkspace.left+1)*m_Default.MainFont.nFontCellWidth;
	m_Default.dcSizeY = Arg.dcSizeY = (m_Default.rcConWorkspace.bottom-m_Default.rcConWorkspace.top+1)*m_Default.MainFont.nFontHeight;
	// **********************************************************************************
	// запомнить данные из m_Default в m_Last, но т.к. там есть указатели - move не катит
	// **********************************************************************************
	//memmove(&m_Last, &m_Default, sizeof(m_Last));
	m_Last.MainFont = m_Default.MainFont;
	memmove(m_Last.crPalette, m_Default.crPalette, sizeof(m_Last.crPalette));
	m_Last.dcSizeX = m_Default.dcSizeX;
	m_Last.dcSizeY = m_Default.dcSizeY;
	m_Last.rcDcLeft = m_Default.rcDcLeft;
	m_Last.rcDcRight = m_Default.rcDcRight;
	m_Last.rcConWorkspace = m_Default.rcConWorkspace;
	m_Last.conCursor = m_Default.conCursor;
	m_Last.FarInterfaceSettings.Raw = m_Default.FarInterfaceSettings.Raw;
	m_Last.FarPanelSettings.Raw = m_Default.FarPanelSettings.Raw;
	memmove(m_Last.nFarColors, m_Default.nFarColors, sizeof(m_Last.nFarColors));
	m_Last.bPanelsAllowed = m_Default.bPanelsAllowed;
	// struct tag_BkPanelInfo
	m_Last.LeftPanel.bVisible = m_Default.LeftPanel.bVisible;
	m_Last.LeftPanel.bFocused = m_Default.LeftPanel.bFocused;
	m_Last.LeftPanel.bPlugin = m_Default.LeftPanel.bPlugin;
	m_Last.LeftPanel.rcPanelRect = m_Default.LeftPanel.rcPanelRect;
	m_Last.RightPanel.bVisible = m_Default.RightPanel.bVisible;
	m_Last.RightPanel.bFocused = m_Default.RightPanel.bFocused;
	m_Last.RightPanel.bPlugin = m_Default.RightPanel.bPlugin;
	m_Last.RightPanel.rcPanelRect = m_Default.RightPanel.rcPanelRect;
	// строки
	lstrcpyW(m_Last.LeftPanel.szCurDir, m_Default.LeftPanel.szCurDir);
	lstrcpyW(m_Last.LeftPanel.szFormat, m_Default.LeftPanel.szFormat);
	lstrcpyW(m_Last.LeftPanel.szHostFile, m_Default.LeftPanel.szHostFile);
	lstrcpyW(m_Last.RightPanel.szCurDir, m_Default.RightPanel.szCurDir);
	lstrcpyW(m_Last.RightPanel.szFormat, m_Default.RightPanel.szFormat);
	lstrcpyW(m_Last.RightPanel.szHostFile, m_Default.RightPanel.szHostFile);
	// **********************************************************************************

	if (m_Default.dcSizeX < 1 || m_Default.dcSizeY < 1)
	{
		_ASSERTE(m_Default.dcSizeX >= 1 && m_Default.dcSizeY >= 1);
		return;
	}

	SetDcPanelRect(&Arg.rcDcLeft, &Arg.LeftPanel, &Arg);
	SetDcPanelRect(&Arg.rcDcRight, &Arg.RightPanel, &Arg);

	// gpTabs отстает от реальности.
	Arg.Place = m_Default.Place;
	//if (!gpTabs)
	//	Arg.Place = pbp_Panels;
	//else if (gnCurrentWindowType == WTYPE_EDITOR)
	//	Arg.Place = pbp_Editor;
	//else if (gnCurrentWindowType == WTYPE_VIEWER)
	//	Arg.Place = pbp_Viewer;
	//else if (Arg.LeftPanel.bVisible || Arg.RightPanel.bVisible)
	//{
	//	_ASSERTE(gnCurrentWindowType == WTYPE_PANELS);
	//	Arg.Place = pbp_Panels;
	//}

	BITMAPINFOHEADER bi = {sizeof(BITMAPINFOHEADER)};
	bi.biWidth = Arg.dcSizeX;
	bi.biHeight = Arg.dcSizeY;
	bi.biPlanes = 1;
	bi.biBitCount = 32; //-V112
	bi.biCompression = BI_RGB;

	//_ASSERTE(Arg.LeftPanel.bVisible || Arg.RightPanel.bVisible);
	HDC hScreen = GetDC(NULL);

	RECT rcMeta = {0,0, Arg.dcSizeX, Arg.dcSizeY}; // (in pixels)
	int iWidthMM = GetDeviceCaps(hScreen, HORZSIZE); if (iWidthMM <= 0) { _ASSERTE(iWidthMM>0); iWidthMM = 1024; }
	int iHeightMM = GetDeviceCaps(hScreen, VERTSIZE); if (iHeightMM <= 0) { _ASSERTE(iHeightMM>0); iHeightMM = 768; }
	int iWidthPels = GetDeviceCaps(hScreen, HORZRES); if (iWidthPels <= 0) { _ASSERTE(iWidthPels>0); iWidthPels = 0; }
	int iHeightPels = GetDeviceCaps(hScreen, VERTRES); if (iHeightPels <= 0) { _ASSERTE(iHeightPels>0); iHeightPels = 0; }
	RECT rcMetaMM = {0,0, (rcMeta.right * iWidthMM * 100)/iWidthPels, (rcMeta.bottom * iHeightMM * 100)/iHeightPels}; // (in .01-millimeter units)

	HDC hdc = NULL;
	HENHMETAFILE hEmf = NULL;
	COLORREF* pBits = NULL;
	HBITMAP hDib = NULL, hOld = NULL;
#ifdef CREATE_EMF_TEMP_FILES
	wchar_t szEmfFile[MAX_PATH] = {};
#endif
	wchar_t *pszEmfFile = NULL;


	if (SETBACKGROUND_USE_EMF==1)
	{
		#ifdef CREATE_EMF_TEMP_FILES
		GetTempPath(MAX_PATH-32, szEmfFile);
		int nLen = lstrlen(szEmfFile);
		if (*szEmfFile && szEmfFile[nLen-1] != L'\\')
		{
			szEmfFile[nLen++] = L'\\';
			szEmfFile[nLen] = 0;
		}
		swprintf_c(szEmfFile+nLen, 31/*#SECURELEN*/, L"CeBack%u.emf", GetCurrentProcessId());
		pszEmfFile = szEmfFile;
		#endif

		hdc = CreateEnhMetaFile(hScreen, pszEmfFile, &rcMetaMM, L"ConEmu\0Far Background\0\0");
		if (!hdc)
		{
			_ASSERTE(hdc!=NULL);
			return;
		}
	}
	else
	{
		hdc = CreateCompatibleDC(hScreen);
		if (!hdc)
		{
			_ASSERTE(hdc!=NULL);
			return;
		}
		_ASSERTE(pBits == NULL);
		hDib = CreateDIBSection(hScreen, (BITMAPINFO*)&bi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);
	}
	ReleaseDC(NULL, hScreen); hScreen = NULL;

	Arg.hdc = hdc;

	if (SETBACKGROUND_USE_EMF==1)
	{
		HBRUSH hFillBr = CreateSolidBrush (
			#ifdef _DEBUG
				RGB(128,128,0)
			#else
				RGB(0,0,0)
			#endif
		);
		FillRect(hdc, &rcMeta, hFillBr);
		DeleteObject(hFillBr);
	}
	else
	{
		if (!hDib || !pBits)
		{
			_ASSERTE(hDib && pBits);
			if (hDib)
				DeleteObject(hDib);
			if (hdc)
				DeleteDC(hdc);
			return;
		}

		hOld = (HBITMAP)SelectObject(hdc, hDib);

		// Залить черным - по умолчанию
		#ifdef _DEBUG
			memset(pBits, 128, bi.biWidth*bi.biHeight*4); //-V112
		#else
			memset(pBits, 0, bi.biWidth*bi.biHeight*4);
		#endif
	}

	// Painting!
	int nProcessed = 0;
	MSectionLock SC; SC.Lock(csBgPlugins, TRUE);
	DWORD nFromLevel = 0, nNextLevel, nSuggested;
	DWORD dwDrawnPlaces = Arg.Place;

	while(true)
	{
		nNextLevel = nFromLevel;
		struct RegisterBackgroundArg *p = mp_BgPlugins;

		for(int i = 0; i < mn_BgPluginsCount; i++, p++)
		{
			if (p->Cmd != rbc_Register ||
			        !(p->dwPlaces & Arg.Place) ||
			        !(p->PaintConEmuBackground))
				continue; // пустая, неактивная в данном контексте, или некорректная ячейка

			// Слои
			nSuggested = p->dwSuggestedLevel;

			if (nSuggested < nFromLevel)
			{
				continue; // Этот слой уже обработан
			}
			else if (nSuggested > nFromLevel)
			{
				// Этот слой нужно будет обработать в следующий раз
				if ((nNextLevel == nFromLevel) || (nSuggested < nNextLevel))
					nNextLevel = nSuggested;

				continue;
			}

			// На уровне 0 (заливающий фон) должен быть только один плагин
			Arg.dwLevel = (nProcessed == 0) ? 0 : (nFromLevel == 0 && nProcessed) ? 1 : nFromLevel;
			Arg.lParam = mp_BgPlugins[i].lParam;
			Arg.dwDrawnPlaces = 0;
			//mp_BgPlugins[i].PaintConEmuBackground(&Arg);
			UpdateBackground_Exec(mp_BgPlugins+i, &Arg);
			// Что плагин покрасил (панели/редактор/вьювер считаются покрашенными по умолчанию)
			dwDrawnPlaces |= Arg.dwDrawnPlaces;
			//
			nProcessed++;
		}

		if (nNextLevel == nFromLevel)
			break; // больше слоев нет

		nFromLevel = nNextLevel;
	}

	SC.Unlock();
	// Sending background to GUI!

	if (nProcessed < 1)
	{
		// Ситуация возникает при старте ConEmu, когда панелей "еще нет"
		//_ASSERTE(nProcessed >= 1);
		if (mb_BgWasSent)
		{
			mb_BgWasSent = FALSE;
			CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SETBACKGROUND, sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETBACKGROUND));
			if (pIn)
			{
				pIn->Background.nType = 1;
				pIn->Background.bEnabled = FALSE;
				pIn->Background.dwDrawnPlaces = 0;
				CESERVER_REQ *pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);

				if (pOut)
				{
					ExecuteFreeResult(pOut);
				}
				ExecuteFreeResult(pIn);
			}
		}
	}
	else // есть "отработавшие" плагины, обновить Background!
	{
		GdiFlush();
		DWORD nBitSize = 0, nBitsError = 0;
		if (SETBACKGROUND_USE_EMF==1)
		{
			hEmf = CloseEnhMetaFile(hdc);
			hdc = NULL;

			nBitSize = GetEnhMetaFileBits(hEmf, 0, NULL);
			if (nBitSize == 0)
			{
				dwDrawnPlaces = 0;
				nBitsError = GetLastError();
				_ASSERTE(nBitSize!=0);
				if (hEmf)
				{
					// В случае ошибки - сразу удаляем
					DeleteEnhMetaFile(hEmf);
					hEmf = NULL;
				}
			}
		}
		else
		{
			nBitSize = bi.biWidth*bi.biHeight*sizeof(COLORREF);
		}
		DWORD nWholeSize = sizeof(CESERVER_REQ_HDR)+sizeof(CESERVER_REQ_SETBACKGROUND)+nBitSize; //-V103 //-V119
		CESERVER_REQ *pIn = ExecuteNewCmd(CECMD_SETBACKGROUND, nWholeSize);

		if (!pIn)
		{
			_ASSERTE(pIn);
		}
		else
		{
			pIn->Background.nType = 1;
			pIn->Background.bEnabled = TRUE;
			pIn->Background.dwDrawnPlaces = dwDrawnPlaces;
			if (SETBACKGROUND_USE_EMF==1)
				pIn->Background.bmp.bfType = 0x4645/*EF*/;
			else
				pIn->Background.bmp.bfType = 0x4D42/*BM*/;
			pIn->Background.bmp.bfSize = nBitSize+sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER); //-V119
			pIn->Background.bmp.bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER); //-V119
			pIn->Background.bi = bi;

			if (SETBACKGROUND_USE_EMF==1)
			{
				LPBYTE pBits = ((LPBYTE)&pIn->Background)+sizeof(pIn->Background);
				DWORD nBitsRc = (nBitSize && hEmf) ? GetEnhMetaFileBits(hEmf, nBitSize, pBits) : 0;
				_ASSERTE(nBitsRc == nBitSize);
				if (!nBitsRc)
				{
					_ASSERTE(nBitsRc!=NULL);
					// Отключить нафиг
					ExecutePrepareCmd(&pIn->hdr, CECMD_SETBACKGROUND, nWholeSize-nBitSize);
					pIn->Background.nType = 1;
					pIn->Background.bEnabled = FALSE;
					pIn->Background.dwDrawnPlaces = 0;
				}
			}
			else
			{
				memmove(((LPBYTE)&pIn->Background)+sizeof(pIn->Background), pBits, bi.biWidth*bi.biHeight*sizeof(COLORREF));
			}
			CESERVER_REQ *pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);

			// Вызывается ТОЛЬКО в главной нити
			_ASSERTE(GetCurrentThreadId() == gnMainThreadId);
			if (pOut)
			{
				mb_BgWasSent = TRUE;
				// Сбросим флажок "Ошибка уже была показана"
				if (pOut->BackgroundRet.nResult == esbr_OK)
				{
					mb_BgErrorShown = FALSE;
				}
				// Показать ошибку, если есть
				else if ((pOut->BackgroundRet.nResult > esbr_OK) && (pOut->BackgroundRet.nResult <= esbr_LastErrorNo)
					&& (pOut->BackgroundRet.nResult != esbr_ConEmuInShutdown)
					&& !mb_BgErrorShown)
				{
					mb_BgErrorShown = TRUE;
					Plugin()->ShowMessage(CEBkError_ExecFailed+pOut->BackgroundRet.nResult, 0);
				}
				ExecuteFreeResult(pOut);
			}
			else if (!mb_BgErrorShown)
			{
				mb_BgErrorShown = TRUE;
				Plugin()->ShowMessage(CEBkError_ExecFailed, 0);
			}

			ExecuteFreeResult(pIn);
		}
	}

	if (SETBACKGROUND_USE_EMF == 0)
	{
		if (hdc && hOld)
			SelectObject(hdc, hOld);
		if (hDib)
			DeleteObject(hDib);
		if (hdc)
			DeleteDC(hdc);
	}
	else
	{
		if (hdc)
		{
			hEmf = CloseEnhMetaFile(hdc);
			hdc = NULL;
		}
		if (hEmf)
		{
			DeleteEnhMetaFile(hEmf);
			hEmf = NULL;
		}
	}
}

BOOL CPluginBackground::LoadThSet(BOOL abFromMainThread)
{
	if (!ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
	{
		_ASSERTE(ghConEmuWndDC!=NULL);
		return FALSE;
	}

	mb_ThNeedLoad = FALSE;
	BOOL lbRc = FALSE;
	MFileMapping<PanelViewSetMapping> ThSetMap;
	DWORD nGuiPID = 0;
	GetWindowThreadProcessId(ghConEmuWndDC, &nGuiPID);
	ThSetMap.InitName(CECONVIEWSETNAME, nGuiPID);

	if (ThSetMap.Open())
	{
		lbRc = ThSetMap.GetTo(&m_ThSet) && (m_ThSet.cbSize != 0);
		ThSetMap.CloseMap();

		if (!lbRc)
		{
			mb_ThNeedLoad = TRUE; // попробовать перечитать в следующий раз?
		}
		else
		{
			//BOOL lbNeedActivate;
			// Если изменились визуальные параметры CE - перерисоваться
			if (memcmp(&m_Default.MainFont, &m_ThSet.MainFont, sizeof(m_ThSet.MainFont)))
			{
				mn_ReqActions |= ra_UpdateBackground;
				m_Default.MainFont = m_ThSet.MainFont;
			}

			//TODO: AppDistinct & fade палитры
			if (memcmp(m_Default.crPalette, m_ThSet.crPalette, sizeof(m_ThSet.crPalette)))
			{
				mn_ReqActions |= ra_UpdateBackground;
				memmove(m_Default.crPalette, m_ThSet.crPalette, sizeof(m_ThSet.crPalette));
			}

			if (mn_ReqActions && !abFromMainThread)
				gbNeedBgActivate = TRUE;
		}
	}

	return lbRc;
}

void CPluginBackground::MonitorBackground()
{
	if (mn_BgPluginsCount == 0 || !ghConEmuWndDC || !IsWindow(ghConEmuWndDC))
		return;

	BOOL lbUpdateRequired = FALSE;
	DWORD nDelta = (GetTickCount() - m_LastThSetCheck);
	DWORD nTimeout = (gnCurrentWindowType == WTYPE_PANELS) ? THSET_TIMEOUT_PANELS : THSET_TIMEOUT_VIEWER;

	if (nDelta > nTimeout)
	{
		m_LastThSetCheck = GetTickCount();
		//BYTE nFarColors[col_LastIndex]; // Массив цветов фара

		//if (LoadThSet())
		//{
		//	// Если изменились визуальные параметры CE - перерисоваться
		//	if (memcmp(&m_Default.MainFont, &m_ThSet.MainFont, sizeof(m_ThSet.MainFont)))
		//	{
		//		lbUpdateRequired = TRUE;
		//		m_Default.MainFont = m_ThSet.MainFont;
		//	}
		//	if (memcmp(m_Default.crPalette, m_ThSet.crPalette, sizeof(m_ThSet.crPalette)))
		//	{
		//		lbUpdateRequired = TRUE;
		//		memmove(m_Default.crPalette, m_ThSet.crPalette, sizeof(m_ThSet.crPalette));
		//	}
		//}

		//if (!lbUpdateRequired)
		//{
		//	// Проверим, а не изменился ли размер окна?
		//	if (IsConsoleActive())
		//	{
		//		RECT rcClient; GetClientRect(ghConEmuWndDC, &rcClient);
		//		if ((m_Default.dcSizeX != (rcClient.right+1))
		//			|| (m_Default.dcSizeY != (rcClient.bottom+1)))
		//		{
		//			lbUpdateRequired = TRUE;
		//		}
		//	}
		//	if (!lbUpdateRequired)
		//	{
		//		TODO("Проверить, не изменился ли размер консольного окна?");
		//	}
		//}

		if (lbUpdateRequired)
		{
			// Когда активируется MainThread - обновить background
			mn_ReqActions |= ra_UpdateBackground;
		}

		// Но проверить все параметры фара тоже нужно
		mn_ReqActions |= ra_CheckPanelFolders;
		gbNeedBgActivate = TRUE;
#if defined(_M_ARM64) || defined(__aarch64__)
		// Without the input hook, ask Far to run the pending panel check on its main thread.
		if (gnCurrentWindowType == WTYPE_PANELS && gnSynchroCount == 0)
			Plugin()->ExecuteSynchro();
#endif
	}
}
