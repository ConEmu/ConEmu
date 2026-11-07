
/*
Copyright (c) 2014-present Maximus5
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

#include "Header.h"
#include <commctrl.h>

#include "../common/CEStr.h"
#include "ConEmu.h"
#include "DynDialog.h"
#include "LngRc.h"
#include "OptionsClass.h"

#if defined(_DEBUG)
#include "OptionsFast.h"
#endif

wchar_t CDynDialog::Button[] = L"Button";
wchar_t CDynDialog::Edit[] = L"Edit";
wchar_t CDynDialog::Static[] = L"Static";
wchar_t CDynDialog::ListBox[] = L"ListBox";
wchar_t CDynDialog::ScrollBar[] = L"ScrollBar";
wchar_t CDynDialog::ComboBox[] = L"ComboBox";
wchar_t CDynDialog::Unknown[] = L"Unknown";

CDynDialog* CDynDialog::mp_Creating = nullptr;
MMap<HWND,CDynDialog*>* CDynDialog::mp_DlgMap = nullptr;

CDynDialog* CDynDialog::ShowDialog(UINT nDlgId, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	CDynDialog* pDlg = new CDynDialog(nDlgId);
	if (!pDlg->LoadTemplate())
	{
		SafeDelete(pDlg);
		return nullptr;
	}

	// Correct font size?
	pDlg->FixupDialogSize();

	// Create modeless dialog
	pDlg->PrepareDlg(lpDialogFunc);
	pDlg->mh_Dlg = CreateDialogIndirectParam(g_hInstance, (LPCDLGTEMPLATEW)pDlg->mlp_Template, hWndParent, DynDialogBox, dwInitParam);
	_ASSERTE(mp_Creating == nullptr);
	if (!pDlg->mh_Dlg)
	{
		DisplayLastError(L"CreateDialogIndirectParam failed");
		SafeDelete(pDlg);
	}

	return pDlg;
}

INT_PTR CDynDialog::ExecuteDialog(UINT nDlgId, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	CDynDialog* pDlg = new CDynDialog(nDlgId);
	if (!pDlg->LoadTemplate())
	{
		SafeDelete(pDlg);
		return -1;
	}

	// Correct font size?
	pDlg->FixupDialogSize();

	// Create modal dialog
	pDlg->PrepareDlg(lpDialogFunc);
	INT_PTR iRc = DialogBoxIndirectParam(g_hInstance, (LPCDLGTEMPLATEW)pDlg->mlp_Template, hWndParent, DynDialogBox, dwInitParam);
	_ASSERTE(mp_Creating == nullptr);
	if (iRc == 0 || iRc == -1)
	{
		DisplayLastError(L"DialogBoxIndirectParam failed");
	}
	SafeDelete(pDlg);

	return iRc;
}

CDynDialog::CDynDialog(UINT nDlgId)
	: mh_Dlg(nullptr)
	, mn_DlgId(nDlgId)
	, mp_DlgProc(nullptr)
	, mlp_Template(nullptr)
	, mp_pointsize(nullptr)
	, mpsz_TypeFace(nullptr)
	, mn_TemplateLength(0)
	, mpsz_Title(nullptr)
{
}

CDynDialog::~CDynDialog()
{
	SafeFree(mlp_Template);

	if (mp_DlgMap && mh_Dlg)
	{
		mp_DlgMap->Del(mh_Dlg);
		if (!mp_DlgMap->GetNext(nullptr, nullptr, nullptr))
		{
			MMap<HWND,CDynDialog*>* p = mp_DlgMap;
			mp_DlgMap = nullptr;
			delete p;
		}
	}
}

void CDynDialog::PrepareDlg(DLGPROC lpDialogFunc)
{
	if (!mp_DlgMap)
	{
		mp_DlgMap = new MMap<HWND,CDynDialog*>();
		mp_DlgMap->Init(256, true);
	}
	mp_DlgProc = lpDialogFunc;
	mp_Creating = this;
}

CDynDialog* CDynDialog::GetDlgClass(HWND hwndDlg)
{
	CDynDialog* pDlg = nullptr;
	if (mp_DlgMap)
		mp_DlgMap->Get(hwndDlg, &pDlg);
	return pDlg;
}

static void PaintDialogIcon(HWND control, HDC dc)
{
	RECT bounds = {};
	GetClientRect(control, &bounds);
	const auto brush = reinterpret_cast<HBRUSH>(SendMessage(GetParent(control), WM_CTLCOLORSTATIC,
		reinterpret_cast<WPARAM>(dc), reinterpret_cast<LPARAM>(control)));
	FillRect(dc, &bounds, brush ? brush : GetSysColorBrush(COLOR_3DFACE));

	// Composite the original icon directly onto the dialog background.
	if (const auto icon = reinterpret_cast<HICON>(SendMessage(control, STM_GETICON, 0, 0)))
		DrawIconEx(dc, 0, 0, icon, bounds.right, bounds.bottom, 0, nullptr, DI_NORMAL);
}

static LRESULT CALLBACK DialogIconProc(HWND control, UINT message, WPARAM wParam, LPARAM lParam,
	UINT_PTR subclassId, DWORD_PTR)
{
	switch (message)
	{
	case WM_ERASEBKGND:
		return TRUE;
	case WM_PAINT:
	{
		PAINTSTRUCT paint = {};
		if (const HDC dc = BeginPaint(control, &paint))
			PaintDialogIcon(control, dc);
		EndPaint(control, &paint);
		return 0;
	}
	case WM_PRINTCLIENT:
		PaintDialogIcon(control, reinterpret_cast<HDC>(wParam));
		return 0;
	case WM_NCDESTROY:
		RemoveWindowSubclass(control, DialogIconProc, subclassId);
		break;
	}
	return DefSubclassProc(control, message, wParam, lParam);
}

static BOOL CALLBACK ApplyDialogIconTheme(HWND control, LPARAM)
{
	wchar_t className[16] = {};
	GetClassNameW(control, className, countof(className));
	if (lstrcmpiW(className, L"Static") == 0
		&& (GetWindowLongPtr(control, GWL_STYLE) & SS_TYPEMASK) == SS_ICON)
		SetWindowSubclass(control, DialogIconProc, 0, 0);
	return TRUE;
}

INT_PTR /*CALLBACK*/ CDynDialog::DynDialogBox(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR iRc = 0;
	CDynDialog* pDlg = nullptr;

	if (uMsg == WM_INITDIALOG)
	{
		pDlg = mp_Creating;
		if (mp_DlgMap)
			mp_DlgMap->Set(hwndDlg, pDlg);
		_ASSERTE(pDlg->mh_Dlg == nullptr || pDlg->mh_Dlg == hwndDlg);
		pDlg->mh_Dlg = hwndDlg;
		mp_Creating = nullptr;
	}
	else
	{
		pDlg = GetDlgClass(hwndDlg);
	}

	if (pDlg && pDlg->mp_DlgProc)
	{
		iRc = pDlg->mp_DlgProc(hwndDlg, uMsg, wParam, lParam);
	}

	// Settings pages already handle their own colors, including palette previews.
	if (pDlg && gpConEmu && pDlg->mn_DlgId != IDD_SETTINGS
		&& (pDlg->mn_DlgId == IDD_SPG_APPDISTINCT2 || !(GetWindowLongPtr(hwndDlg, GWL_STYLE) & WS_CHILD)))
	{
		switch (uMsg)
		{
		case WM_INITDIALOG:
			if (IsWindow(hwndDlg) && CDwmHelper::IsWindowsDarkMode())
			{
				CSettings::ApplySettingsTheme(hwndDlg);
				if (pDlg->mn_DlgId == IDD_SPG_APPDISTINCT2)
					gpConEmu->SetWindowTheme(hwndDlg,
						gOSVer.dwBuildNumber >= 26300 ? L"DarkMode_DarkTheme" : L"DarkMode_Explorer", nullptr);
				EnumChildWindows(hwndDlg, ApplyDialogIconTheme, 0);
			}
			break;
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		case WM_CTLCOLOREDIT:
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORBTN:
		case WM_CTLCOLORSCROLLBAR:
			if (!iRc)
				iRc = CSettings::SettingsThemeBrush(uMsg, reinterpret_cast<HDC>(wParam), reinterpret_cast<HWND>(lParam));
			break;
		}
	}

	if (uMsg == WM_DESTROY)
	{
		if (mp_DlgMap)
		{
			mp_DlgMap->Del(hwndDlg);
			/*
			if (!mp_DlgMap->GetNext(nullptr, nullptr, nullptr))
			{
				delete mp_DlgMap;
				mp_DlgMap = nullptr;
			}
			*/
		}
	}

	return iRc;
}

#if 0
INT_PTR CDynDialog::ShowDialog(bool Modal, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam)
{
	INT_PTR iRc = 0;

	if (!LoadTemplate())
	{
		LogString(L"Template loading failed, creating dialog by ID");

		if (Modal)
		{
			iRc = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(mn_DlgId), hWndParent, lpDialogFunc, dwInitParam);
		}
		else
		{
			mh_Dlg = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(mn_DlgId), hWndParent, lpDialogFunc, dwInitParam);
			iRc = (INT_PTR)mh_Dlg;
		}
	}
	else
	{
		if (Modal)
		{
			iRc = DialogBoxIndirectParam(g_hInstance, (LPCDLGTEMPLATEW)mlp_Template, hWndParent, lpDialogFunc, dwInitParam);
		}
		else
		{
			mh_Dlg = CreateDialogIndirectParam(g_hInstance, (LPCDLGTEMPLATEW)mlp_Template, hWndParent, lpDialogFunc, dwInitParam);
			iRc = (INT_PTR)mh_Dlg;
		}
	}

	return iRc;
}
#endif

bool CDynDialog::LoadTemplate()
{
	if (mlp_Template)
	{
		// Template was already loaded
		return true;
	}

	bool bOk = false;
	HRSRC hsrc;
	HGLOBAL hrc;
	wchar_t szInfo[120];
	DLGTEMPLATEEX* ptr;
	DWORD_PTR data;
	DWORD nErr = 0;

	hsrc = FindResource(g_hInstance, MAKEINTRESOURCE(mn_DlgId), RT_DIALOG);
	if (!hsrc)
	{
		nErr = GetLastError();
		swprintf_c(szInfo, L"FindResource failed, RT_DIALOG=%u", mn_DlgId);
		goto wrap;
	}
	hrc = LoadResource(g_hInstance, hsrc);
	if (!hrc)
	{
		nErr = GetLastError();
		swprintf_c(szInfo, L"LoadResource failed, RT_DIALOG=%u", mn_DlgId);
		goto wrap;
	}
	mn_TemplateLength = SizeofResource(g_hInstance, hsrc);
	if (!mn_TemplateLength)
	{
		nErr = GetLastError();
		swprintf_c(szInfo, L"SizeofResource failed, RT_DIALOG=%u", mn_DlgId);
		goto wrap;
	}
	ptr = (DLGTEMPLATEEX*)LockResource(hrc);
	if (!ptr)
	{
		nErr = GetLastError();
		swprintf_c(szInfo, L"LockResource failed, RT_DIALOG=%u", mn_DlgId);
		goto wrap;
	}
	if ((ptr->signature == 0xFFFF) && (ptr->dlgVer != 1))
	{
		nErr = GetLastError();
		swprintf_c(szInfo, L"Unsupported dialog resource type, RT_DIALOG=%u, ver=x%04X, sign=x%04X", mn_DlgId, ptr->dlgVer, ptr->signature);
		goto wrap;
	}

	mlp_Template = (DLGTEMPLATEEX*)calloc(mn_TemplateLength, 1);
	if (!mlp_Template)
	{
		nErr = GetLastError();
		swprintf_c(szInfo, L"Resource data allocation failed, RT_DIALOG=%u, size=%u", mn_DlgId, mn_TemplateLength);
		goto wrap;
	}

	memmove(mlp_Template, ptr, mn_TemplateLength);

	data = (DWORD_PTR)mlp_Template;
	if ((ptr->signature == 0xFFFF) && (ptr->dlgVer == 1))
	{
		if (!ParseDialogEx(data))
		{
			size_t cchPos = data - (DWORD_PTR)mlp_Template;
			nErr = (DWORD)-1;
			swprintf_c(szInfo, L"Parse DialogEx failed, RT_DIALOG=%u, pos=%u", mn_DlgId, (DWORD)cchPos);
			goto wrap;
		}
	}
	else
	{
		if (!ParseDialog(data))
		{
			size_t cchPos = data - (DWORD_PTR)mlp_Template;
			nErr = (DWORD)-1;
			swprintf_c(szInfo, L"Parse Dialog failed, RT_DIALOG=%u, pos=%u", mn_DlgId, (DWORD)cchPos);
			goto wrap;
		}
	}

	bOk = true;
wrap:
	if (!bOk)
	{
		DisplayLastError(szInfo, nErr);
		SafeFree(mlp_Template);
	}
	return bOk;
}

bool CDynDialog::ParseDialog(DWORD_PTR& data)
{
	DLGTEMPLATE* ptr = (DLGTEMPLATE*)data;
	_ASSERTE(ptr == mlp_Template);

	// First item - dialog descriptor itself
	ItemInfo i = {0, FALSE, &ptr->x, &ptr->y, &ptr->cx, &ptr->cy};
	m_Items.push_back(i);

	data += 18; // sizeof may be aligned

	// menu - sz_Or_Ord
	SkipSzOrOrd(data);

	// windowClass - sz_Or_Ord
	SkipSzOrOrd(data);

	//WCHAR     title[titleLen];
	mpsz_Title = SkipSz(data);

	if (ptr->style & (DS_SETFONT))
	{
		//WORD      pointsize;
		mp_pointsize = (LPWORD)data;
		data += sizeof(WORD);
		//WCHAR     typeface[stringLen];
		mpsz_TypeFace = SkipSz(data);
	}

	for (INT_PTR i = ptr->cdit; i--;)
	{
		if (!ParseItem(data))
			return false;
	}

	return true;
}

bool CDynDialog::ParseItem(DWORD_PTR& data)
{
	if (((data - (DWORD_PTR)mlp_Template) + sizeof(DLGITEMTEMPLATE)) > mn_TemplateLength)
	{
		return false;
	}

	// Each DLGITEMTEMPLATE structure in the template must be aligned on a DWORD boundary
	data = (((data + 3) >> 2) << 2);

	DLGITEMTEMPLATE* ptr = (DLGITEMTEMPLATE*)data;
	ItemInfo i = {ptr->id, FALSE, &ptr->x, &ptr->y, &ptr->cx, &ptr->cy};
	i.ptr = ptr;

	i.Style = ptr->style;
	i.pStyle = &ptr->style;

	data += 18; // sizeof may be aligned

	// windowClass - sz_Or_Atom
	i.ItemType = SkipSzOrAtom(data);

	// title - sz_Or_Ord
	SkipSzOrOrd(data);

	// extraCount
	WORD extraCount = *((LPWORD)data);
	data += sizeof(WORD)+extraCount;

	m_Items.push_back(i);
	return true;
}

bool CDynDialog::ParseDialogEx(DWORD_PTR& data)
{
	DLGTEMPLATEEX* ptr = (DLGTEMPLATEEX*)data;
	_ASSERTE(ptr == mlp_Template);

	if ((ptr->signature != 0xFFFF) || (ptr->dlgVer != 1))
	{
		// Must be checked in LoadTemplate()
		_ASSERTE((ptr->signature == 0xFFFF) && (ptr->dlgVer == 1));
		return false;
	}

	// First item - dialog descriptor itself
	ItemInfo i = {0, TRUE, &ptr->x, &ptr->y, &ptr->cx, &ptr->cy};
	m_Items.push_back(i);

	data += 26; // sizeof may be aligned

	// menu - sz_Or_Ord
	SkipSzOrOrd(data);

	// windowClass - sz_Or_Ord
	SkipSzOrOrd(data);

	//WCHAR     title[titleLen];
	mpsz_Title = SkipSz(data);

	if (ptr->style & (DS_SETFONT|DS_SHELLFONT))
	{
		//WORD      pointsize;
		mp_pointsize = (LPWORD)data;
		//WORD      weight;
		//BYTE      italic;
		//BYTE      charset;
		data += 6; // 2*WORD+2*BYTE
		//WCHAR     typeface[stringLen];
		mpsz_TypeFace = SkipSz(data);
	}

	for (INT_PTR i = ptr->cDlgItems; i--;)
	{
		if (!ParseItemEx(data))
			return false;
	}

	return true;
}

bool CDynDialog::ParseItemEx(DWORD_PTR& data)
{
	if (((data - (DWORD_PTR)mlp_Template) + sizeof(DLGITEMTEMPLATEEX)) > mn_TemplateLength)
	{
		return false;
	}

	// Each DLGITEMTEMPLATEEX structure in the template must be aligned on a DWORD boundary
	data = (((data + 3) >> 2) << 2);

	DLGITEMTEMPLATEEX* ptr = (DLGITEMTEMPLATEEX*)data;
	ItemInfo i = {ptr->id, TRUE, &ptr->x, &ptr->y, &ptr->cx, &ptr->cy};
	i.ptrEx = ptr;

	i.Style = ptr->style;
	i.pStyle = &ptr->style;

	data += 24; // sizeof may be aligned

	// windowClass - sz_Or_Atom
	i.ItemType = SkipSzOrAtom(data);

	// title - sz_Or_Ord
	SkipSzOrOrd(data);

	// extraCount
	WORD extraCount = *((LPWORD)data);
	data += sizeof(WORD)+extraCount;

	m_Items.push_back(i);
	return true;
}

LPWSTR CDynDialog::SkipSz(DWORD_PTR& data)
{
	wchar_t* psz = (wchar_t*)data;
	size_t len = wcslen(psz);
	data += (len+1)*sizeof(WORD);
	return psz;
}

LPWSTR CDynDialog::SkipSzOrOrd(DWORD_PTR& data)
{
	LPWSTR pszTitle;

	if (*((LPWORD)data) == 0xFFFF)
	{
		data += sizeof(WORD);
		pszTitle = (LPWSTR)MAKEINTRESOURCE(*((LPWORD)data));
		data += sizeof(WORD);
	}
	else
	{
		pszTitle = SkipSz(data);
	}

	return pszTitle;
}

LPWSTR CDynDialog::SkipSzOrAtom(DWORD_PTR& data)
{
	LPWSTR pszItemType;

	if (*((LPWORD)data) != 0xFFFF)
	{
		pszItemType = SkipSz(data);
	}
	else
	{
		data += sizeof(WORD);

		switch (*((LPWORD)data))
		{
		case 0x0080: pszItemType = CDynDialog::Button; break;
		case 0x0081: pszItemType = CDynDialog::Edit; break;
		case 0x0082: pszItemType = CDynDialog::Static; break;
		case 0x0083: pszItemType = CDynDialog::ListBox; break;
		case 0x0084: pszItemType = CDynDialog::ScrollBar; break;
		case 0x0085: pszItemType = CDynDialog::ComboBox; break;
		default:
			_ASSERTE(FALSE && "Unknown control type?");
			pszItemType = CDynDialog::Unknown;
		}

		data += sizeof(WORD);
	}

	return pszItemType;
}

UINT CDynDialog::GetFontPointSize()
{
	return (mp_pointsize && *mp_pointsize) ? *mp_pointsize : 8;
}

void CDynDialog::FixupDialogSize()
{
	#if 0
	//if (mp_pointsize)
	//	*mp_pointsize = 4;
	for (INT_PTR i = m_Items.size()-1; i >= 0; i--)
	{
		ItemInfo& ii = m_Items[i];
		if (ii.ItemType == CDynDialog::Button)
		{
			// Just use bitfields to check for Radio/Check
			if ((ii.Style & (BS_CHECKBOX|BS_RADIOBUTTON|BS_USERBUTTON)))
			{
				*ii.pStyle = ((ii.Style & ~BS_TYPEMASK) | BS_OWNERDRAW);
			}
		}
	}
	#endif
}

bool CDynDialog::DrawButton(WPARAM wID, DRAWITEMSTRUCT* pDraw)
{
	if (!pDraw || (pDraw->CtlType != ODT_BUTTON))
		return false;

	CDynDialog* pDlg = nullptr;
	if (!mp_DlgMap || !mp_DlgMap->Get(GetParent(pDraw->hwndItem), &pDlg))
		return false;

	ItemInfo* p = nullptr;
	for (INT_PTR i = pDlg->m_Items.size()-1; i >= 0; i--)
	{
		p = &(pDlg->m_Items[i]);
		if ((p->ItemType == Button)
			&& (p->ID == pDraw->CtlID))
		{
			DWORD style = (p->Style & BS_TYPEMASK);
			switch (style)
			{
			case BS_CHECKBOX: case BS_AUTOCHECKBOX:
			case BS_RADIOBUTTON: case BS_AUTORADIOBUTTON:
			case BS_3STATE:
				pDlg->DrawButton(p, pDraw);
				return true;
			}
		}
	}

	return false;
}

bool CDynDialog::DrawButton(ItemInfo* pItem, DRAWITEMSTRUCT* pDraw)
{
	int ih = pDraw->rcItem.bottom-pDraw->rcItem.top-1;
	RECT rc = {pDraw->rcItem.left+1, pDraw->rcItem.top+1, pDraw->rcItem.left+ih, pDraw->rcItem.top+ih};
	UINT uType = DFC_BUTTON;
	UINT uState = 0;

	DWORD style = (pItem->Style & BS_TYPEMASK);
	switch (style)
	{
	case BS_CHECKBOX: case BS_AUTOCHECKBOX:
		uState = DFCS_BUTTONCHECK|DFCS_FLAT
			| ((pDraw->itemState & ODS_CHECKED) ? DFCS_CHECKED : 0)
			| ((pDraw->itemState & ODS_DISABLED) ? DFCS_INACTIVE : 0)
			;
		break;
	case BS_RADIOBUTTON: case BS_AUTORADIOBUTTON:
		uState = DFCS_BUTTONRADIO/*|DFCS_FLAT*/
			| ((pDraw->itemState & ODS_CHECKED) ? DFCS_CHECKED : 0)
			| ((pDraw->itemState & ODS_DISABLED) ? DFCS_INACTIVE : 0)
			;
		break;
	case BS_3STATE:
		_ASSERTE(FALSE && "BS_3STATE");
		return false;
	}

	DrawFrameControl(pDraw->hDC, &rc, uType, uState);
	return true;

	//HBRUSH hbr = (HBRUSH)GetStockObject(BLACK_BRUSH);
	//FillRect(pDraw->hDC, &pDraw->rcItem, hbr);
	//return true;
}

// static
void CDynDialog::LocalizeDialog(HWND hDlg, UINT nTitleRsrcId /*= 0*/)
{
	#if defined(_DEBUG)
	static HWND hLastDlg = nullptr;
	if (hLastDlg != hDlg || hDlg == FastConfig::ghFastCfg)
		hLastDlg = hDlg;
	else
		_ASSERTE(hLastDlg != hDlg); // avoid re-localization
	#endif

	// Call it before CLngRc::isLocalized, title may be changed via LngDataRsrcs.h
	if (nTitleRsrcId != 0)
	{
		LPCWSTR pszTitle = CLngRc::getRsrc(nTitleRsrcId);
		if (pszTitle && *pszTitle)
			SetWindowText(hDlg, pszTitle);
	}

	// Control text may be overriden only by ConEmu.l10n
	if (!CLngRc::isLocalized())
		return;
	EnumChildWindows(hDlg, CDynDialog::LocalizeControl, reinterpret_cast<LPARAM>(hDlg));
}

// static, CALLBACK
// Intended to be called from EnumChildWindows
BOOL CDynDialog::LocalizeControl(HWND hChild, LPARAM lParam)
{
	// Nothing to do
	if (!CLngRc::isLocalized())
		return TRUE;

	_ASSERTE(lParam != 0);
	_ASSERTE(IsWindow(reinterpret_cast<HWND>(lParam)));

	// Only first level
	HWND hParent = GetParent(hChild);
	if (hParent != (HWND)lParam)
	{
		return TRUE; // continue enum
	}

	LONG wID = GetWindowLong(hChild, GWL_ID);

	#if 0
	if (wID == stCmdGroupCommands)
		wID = wID; // for debugger breakpoint
	#endif

	_ASSERTE(IDC_STATIC == -1);
	if ((wID != 0)              // Unknown control?
		&& (wID != IDC_STATIC)) // IDC_STATIC
	{
		CEStr lsLoc;
		wchar_t szClass[64];

		if (CLngRc::getControl(wID, lsLoc)
			&& !lsLoc.IsEmpty())
		{
			// BUTTON, STATICTEXT, ...?
			if (GetClassName(hChild, szClass, countof(szClass)))
			{
				if ((lstrcmpi(szClass, L"Button") == 0)
					|| (lstrcmpi(szClass, L"Static") == 0))
				{
					SetWindowText(hChild, lsLoc.ms_Val);
				}
				#ifdef _DEBUG
				else
				{
					int w = wID; // control class is not available for l10n
				}
				#endif
			}
		}
	}

	return TRUE; // continue enumeration
}
