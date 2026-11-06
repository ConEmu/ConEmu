
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


#define HIDE_USE_EXCEPTION_INFO

#define SHOWDEBUGSTR

#define DEBUGSTRICON(s) //DEBUGSTR(s)

#include "../common/defines.h"
#include <commctrl.h>
#include "Header.h"
#include "ConEmu.h"
#include "IconList.h"
#include "Options.h"
#include "OptionsClass.h"
#include "ToolImg.h"
#include "../common/EnvVar.h"
#include "../common/MSectionSimple.h"
#include "../common/MWow64Disable.h"
#include "../common/WFiles.h"

#ifdef _DEBUG
#include "LoadImg.h"
#endif

CIconList::CIconList()
{
	mpcs = new MSectionSimple(true);
}

CIconList::~CIconList()
{
	if (mh_TabIcons)
	{
		ImageList_Destroy(mh_TabIcons);
		mh_TabIcons = nullptr;
	}

	for (INT_PTR i = 0; i < m_Icons.size(); i++)
	{
		SafeFree(m_Icons[i].pszIconDescr);
	}
	m_Icons.clear();

	SafeDelete(mpcs);
}

bool CIconList::IsInitialized()
{
	return (mh_TabIcons != nullptr);
}

bool CIconList::Initialize()
{
	bool lbOk = true;

	if (!mh_TabIcons)
	{
		int iSysX = GetSystemMetrics(SM_CXSMICON), iSysY = GetSystemMetrics(SM_CYSMICON);
		int iFontY = gpSetCls->EvalSize(gpSet->nTabFontHeight, esf_Vertical|esf_CanUseUnits|esf_CanUseDpi|esf_CanUseZoom);
		if (iFontY < 0)
			iFontY = gpFontMgr->EvalFontHeight(gpSet->sTabFontFace, iFontY, gpSet->nTabFontHeight);
		int iDpyY = gpSetCls->EvalSize(std::max(16,iSysY), esf_Vertical|esf_CanUseDpi);
		mn_CxIcon = 16; mn_CyIcon = 16;
		if (iFontY > 16)
		{
			if (iDpyY <= iFontY)
			{
				mn_CxIcon = gpSetCls->EvalSize(std::max(16,iSysX), esf_Horizontal|esf_CanUseDpi);
				mn_CyIcon = iDpyY;
			}
			else
			{
				// Issue 1939: Tab bar height too big since version 150307
				int iSizes[] = {20, 24, 28, 32, 40, 48, 60, 64};
				iFontY++; // Allow one pixes larger...
				for (INT_PTR j = countof(iSizes)-1; j >= 0; j--)
				{
					if (iFontY >= iSizes[j])
					{
						mn_CyIcon = iSizes[j];
						mn_CxIcon = (iSysX * iSizes[j]) / iSysY;
						break;
					}
				}
			}
		}

		wchar_t szLog[100];
		swprintf_c(szLog, L"Creating IconList for size {%ix%i} SysIcon size is {%ix%i}", mn_CxIcon, mn_CyIcon, iSysX, iSysY);
		gpConEmu->LogString(szLog);

		if ((mh_TabIcons = ImageList_Create(mn_CxIcon, mn_CyIcon, ILC_COLOR32|ILC_MASK, 0, 16)) != nullptr)
		{
			CToolImg img;
			int nFirstAdd = -1;
			const int iShieldButtons = 4;
			if (img.Create(mn_CxIcon, mn_CyIcon, iShieldButtons, GetSysColor(COLOR_BTNFACE)))
			{
				if (img.AddButtons(g_hInstance, IDB_SHIELD16, iShieldButtons))
				{
					HBITMAP hShieldUser = img.GetBitmap();
					#ifdef _DEBUG
					BITMAP bmi = {};
					GetObject(hShieldUser, sizeof(bmi), &bmi);
					#ifdef _DEBUG
					//SaveImageEx(L"T:\\ShieldUser.png", hShieldUser);
					#endif
					#endif

					nFirstAdd = ImageList_AddMasked(mh_TabIcons, hShieldUser, RGB(128,0,0));
				}
			}

			#ifdef _DEBUG
			int iCount = ImageList_GetImageCount(mh_TabIcons);
			_ASSERTE(iCount==4);
			IMAGEINFO ii = {}; BOOL b;
			// An application should not call DeleteObject to destroy the bitmaps retrieved by ImageList_GetImageInfo
			b = ImageList_GetImageInfo(mh_TabIcons, 0, &ii);
			b = ImageList_GetImageInfo(mh_TabIcons, 1, &ii);
			#endif

			if (nFirstAdd >= 0)
			{
				mn_AdminIcon = nFirstAdd + ((gOSVer.dwMajorVersion >= 6) ? 0 : 1);
				int overlayIndex = 1;
				if (ImageList_SetOverlayImage(mh_TabIcons, mn_AdminIcon + 2, overlayIndex))
				{
					mn_AdminOverlayIndex = overlayIndex;
				}
				else
				{
					gpConEmu->LogString(L"Setting up the overlay admin icon failed");
				}
			}
			else
			{
				lbOk = false;
			}
		}
		else
		{
			lbOk = false;
		}
	}

	return lbOk;
}

int CIconList::GetTabIcon(bool bAdmin)
{
	int iIconIdx = (bAdmin && gpSet->isAdminShield()) ? mn_AdminIcon : -1;
	return iIconIdx;
}

int CIconList::CreateTabIcon(LPCWSTR asIconDescr, bool bAdmin, LPCWSTR asWorkDir)
{
	if (bAdmin && !gpSet->isAdminShield())
		bAdmin = false;

	if (!asIconDescr || !*asIconDescr)
		return GetTabIcon(bAdmin);

	int iCreatedIcon = -1;

	MSectionLockSimple CS; CS.Lock(mpcs, 30000);

	for (INT_PTR i = 0; i < m_Icons.size(); i++)
	{
		const TabIconCache& icn = m_Icons[i];
		if ((icn.bAdmin!=FALSE) != bAdmin)
			continue;
		if (lstrcmpi(icn.pszIconDescr, asIconDescr) != 0)
			continue;

		// If tab was not created yet?
		if (icn.bWasNotLoaded && mh_TabIcons)
		{
			m_Icons.erase(i);
			break;
		}

		// Already was created!
		iCreatedIcon = icn.nIconIdx;
		goto wrap;
	}

	if (mh_TabIcons)
	{
		iCreatedIcon = CreateTabIconInt(asIconDescr, bAdmin, asWorkDir);
		if (iCreatedIcon == -1)
		{
			MWow64Disable wow; wow.Disable();
			iCreatedIcon = CreateTabIconInt(asIconDescr, bAdmin, asWorkDir);
		}
	}

	if (iCreatedIcon == -1)
	{
		// To avoid numerous CreateTabIconInt calls - just remember "No icon" for that asIconDescr
		TabIconCache DummyIcon = {lstrdup(asIconDescr).Detach(), -1, bAdmin, (mh_TabIcons==nullptr)};
		m_Icons.push_back(DummyIcon);
	}
wrap:
	return iCreatedIcon;
}

int CIconList::CreateTabIconInt(LPCWSTR asIconDescr, bool bAdmin, LPCWSTR asWorkDir)
{
	CEStr pszExpanded = ExpandEnvStr(asIconDescr);

	// Need to be created!
	int iIconIdx = -1;
	HICON hFileIcon = nullptr;
	CEStr szLoadFile;
	LPCWSTR lpszExt = nullptr;
	int nIndex = 0;
	bool bDirChanged = false;

	if (!szLoadFile.Set(pszExpanded ? pszExpanded.c_str() : asIconDescr))
	{
		goto wrap;
	}

	lpszExt = ParseIconFileIndex(szLoadFile, nIndex);

	if (asWorkDir && *asWorkDir)
	{
		// Executable (or icon) file may be not available by %PATH%, let "cd" to it...
		bDirChanged = gpConEmu->ChangeWorkDir(asWorkDir);
	}

	if (!lpszExt)
	{
		if (apiSearchPath(nullptr, szLoadFile, L".exe", szLoadFile))
		{
			lpszExt = PointToExt(szLoadFile);
		}

		if (!lpszExt)
			goto wrap;
	}

	if (lstrcmpi(lpszExt, L".ico") == 0)
	{
		hFileIcon = static_cast<HICON>(LoadImage(nullptr, szLoadFile, IMAGE_ICON, mn_CxIcon, mn_CyIcon, LR_DEFAULTCOLOR | LR_LOADFROMFILE));
	}
	else if ((lstrcmpi(lpszExt, L".exe") == 0) || (lstrcmpi(lpszExt, L".dll") == 0))
	{
		HICON hIconLarge = nullptr, hIconSmall = nullptr;
		const UINT extracted = ExtractIconEx(szLoadFile, nIndex, &hIconLarge, &hIconSmall, 1);
		if (!(hIconLarge || hIconSmall))
		{
			_ASSERTE(!extracted || (extracted == static_cast<UINT>(-1)));
			CEStr rsFound;
			if (SearchAppPaths(szLoadFile, rsFound, false))
			{
				ExtractIconEx(rsFound, nIndex, &hIconLarge, &hIconSmall, 1);
			}
		}
		const bool bUseLargeIcon = ((mn_CxIcon > 16) && (hIconLarge != nullptr)) || (hIconSmall == nullptr);
		// ReSharper disable once CppLocalVariableMayBeConst
		HICON hDestroyIcon = bUseLargeIcon ? hIconSmall : hIconLarge;
		if (hDestroyIcon)
			DestroyIcon(hDestroyIcon);
		hFileIcon = bUseLargeIcon ? hIconLarge : hIconSmall;
	}
	else
	{
		//#TODO: Shell icons for registered files (cmd, bat, sh, pl, py, ...)
	}

	if (hFileIcon)
	{
		wchar_t szIconInfo[80] = L"", szMergedInfo[80] = L"";
		GetIconInfoStr(hFileIcon, szIconInfo);

		if (gpSet->isLogging())
		{
			const CEStr lsLog(L"Icon `", asIconDescr, L"` was loaded: ", szIconInfo);
			gpConEmu->LogString(lsLog);
		}

		int iIconIdxAdm = -1;
		iIconIdx = ImageList_ReplaceIcon(mh_TabIcons, -1, hFileIcon);

		const TabIconCache newIcon = {lstrdup(asIconDescr).Detach(), iIconIdx, false, false};
		m_Icons.push_back(newIcon);

		if (mn_AdminOverlayIndex >= 0)
		{
			// ReSharper disable once CppLocalVariableMayBeConst
			HICON hNewIcon = ImageList_GetIcon(mh_TabIcons, iIconIdx, ILD_TRANSPARENT | INDEXTOOVERLAYMASK(mn_AdminOverlayIndex));
			if (hNewIcon)
			{
				const CEStr lsLog(L"Admin icon `", asIconDescr, L"` was created: ", GetIconInfoStr(hNewIcon, szMergedInfo));
				gpConEmu->LogString(lsLog);

				iIconIdxAdm = ImageList_ReplaceIcon(mh_TabIcons, -1, hNewIcon);
				DestroyIcon(hNewIcon);

				const TabIconCache admIcon = {lstrdup(asIconDescr).Detach(), iIconIdxAdm, true, false};
				m_Icons.push_back(admIcon);

				if (bAdmin && (iIconIdxAdm > 0))
				{
					iIconIdx = iIconIdxAdm;
				}
			}
			else
			{
				gpConEmu->LogString(L"GetIcon for admin icon was failed");
			}
		}

		DestroyIcon(hFileIcon);
	}

wrap:
	if (bDirChanged)
	{
		gpConEmu->ChangeWorkDir(nullptr);
	}
	if (gpSet->isLogging() && (iIconIdx < 0))
	{
		const CEStr lsLog(L"Icon `", asIconDescr, L"` loading was failed");
		gpConEmu->LogString(lsLog);
	}
	return iIconIdx;
}

LPCWSTR CIconList::GetIconInfoStr(HICON h, wchar_t (&szInfo)[80])
{
	ICONINFO ii = {};
	GetIconInfo(h, &ii);
	BITMAP bi = {};
	GetObject(ii.hbmColor, sizeof(bi), &bi);
	swprintf_c(szInfo, L"{%ix%i} planes=%u bpp=%u", bi.bmWidth, bi.bmHeight, bi.bmPlanes, bi.bmBitsPixel);
	SafeDeleteObject(ii.hbmColor);
	SafeDeleteObject(ii.hbmMask);
	return szInfo;
}

LPCWSTR CIconList::ParseIconFileIndex(CEStr& lsIconFileIndex, int& nIndex)
{
	LPCWSTR lpszExt = PointToExt(lsIconFileIndex);
	LPCWSTR lpszIndex = wcsrchr(lpszExt ? lpszExt : (LPCWSTR)lsIconFileIndex, L',');
	if (lpszIndex)
	{
		lsIconFileIndex.ms_Val[lpszIndex - lsIconFileIndex.ms_Val] = 0;

		nIndex = wcstol(lpszIndex + 1, nullptr, 10);
		if (nIndex == LONG_MAX || nIndex == LONG_MIN)
		{
			nIndex = 0;
		}
	}
	else
	{
		nIndex = 0;
	}
	return lpszExt;
}

HICON CIconList::GetTabIconByIndex(int IconIndex)
{
	if (!this || !mh_TabIcons)
		return nullptr;
	HICON hIcon = ImageList_GetIcon(mh_TabIcons, IconIndex, ILD_TRANSPARENT);
	return hIcon;
}
