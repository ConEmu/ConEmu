
// WM_DRAWITEM, SS_OWNERDRAW|SS_NOTIFY, Bmp & hBmp -> global vars
// DPI resize.

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

#undef HIDE_USE_EXCEPTION_INFO

#define SHOWDEBUGSTR

#include "Header.h"
#include "DarkMode.h"
#include "About.h"
#include "AboutDlg.h"
#include "ConEmu.h"
#include "DontEnable.h"
#include "DpiAware.h"
#include "DynDialog.h"
#include "ImgButton.h"
#include "LngRc.h"
#include "OptionsClass.h"
#include "PushInfo.h"
#include "RealConsole.h"
#include "SearchCtrl.h"
#include "Update.h"
#include "VirtualConsole.h"
#include "version.h"
#include "../common/MSetter.h"
#include "../common/WObjects.h"
#include "../common/StartupEnvEx.h"

namespace ConEmuAbout
{
	void InitCommCtrls();
	bool mb_CommCtrlsInitialized = false;
	HWND mh_AboutDlg = nullptr;
	DWORD nLastCrashReported = 0;
	CDpiForDialog* mp_DpiAware = nullptr;

	INT_PTR WINAPI aboutProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam);
	void searchProc(HWND hDlg, HWND hSearch, bool bReentr);

	void OnInfo_DonateLink();
	void OnInfo_FlattrLink();

	CImgButtons* mp_ImgBtn = nullptr;

	static struct {LPCWSTR Title; LPCWSTR Text;} Pages[] =
	{
		{L"About", pAbout},
		{L"ConEmu /?", pCmdLine},
		{L"Tasks", pAboutTasks},
		{L"-new_console", pNewConsoleHelpFull},
		{L"ConEmuC /?", pConsoleHelpFull},
		{L"Macro", pGuiMacro},
		{L"DosBox", pDosBoxHelpFull},
		{L"Contributors", pAboutContributors},
		{L"License", pAboutLicense},
		{L"SysInfo", L""},
	};

	wchar_t sLastOpenTab[32] = L"";

	void TabSelected(HWND hDlg, int idx);

	CEStr* gsSysInfo = nullptr;
	void ReloadSysInfo();
	void LogStartEnvInt(LPCWSTR asText, LPARAM lParam, bool bFirst, bool bNewLine);

	DWORD nTextSelStart = 0, nTextSelEnd = 0;
};

INT_PTR WINAPI ConEmuAbout::aboutProc(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam)
{
	INT_PTR lRc = 0;
	if (DarkMode::ProcessDialogMessage(hDlg, messg, wParam, lParam, lRc)) return lRc;
	if ((mp_ImgBtn && mp_ImgBtn->Process(hDlg, messg, wParam, lParam, lRc))
		|| EditIconHint_Process(hDlg, messg, wParam, lParam, lRc))
	{
		SetWindowLongPtr(hDlg, DWLP_MSGRESULT, lRc);
		return TRUE;
	}

	PatchMsgBoxIcon(hDlg, messg, wParam, lParam);


	switch (messg)
	{
		case WM_INITDIALOG:
		{
			gpConEmu->OnOurDialogOpened();
			mh_AboutDlg = hDlg;

			CDynDialog::LocalizeDialog(hDlg);

			_ASSERTE(mp_ImgBtn==nullptr);
			SafeDelete(mp_ImgBtn);
			mp_ImgBtn = new CImgButtons(hDlg, pIconCtrl, IDOK);
			mp_ImgBtn->AddDonateButtons();

			if (mp_DpiAware)
			{
				mp_DpiAware->Attach(hDlg, ghWnd, CDynDialog::GetDlgClass(hDlg));
			}

			CDpiAware::CenterDialog(hDlg);

			if ((ghOpWnd && IsWindow(ghOpWnd)) || (WS_EX_TOPMOST & GetWindowLongPtr(ghWnd, GWL_EXSTYLE)))
			{
				SetWindowPos(hDlg, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);
			}

			if (global::g_darkModeSupported)
			{
				HWND hOkBtn = GetDlgItem(hDlg, IDOK);
				if (hOkBtn)
					gpConEmu->SetWindowTheme(hOkBtn, L"Explorer", nullptr);

				HWND hTab = GetDlgItem(hDlg, tbAboutTabs);
				if (hTab)
					gpConEmu->SetWindowTheme(hTab, L"Explorer", nullptr);

				HWND hEdit = GetDlgItem(hDlg, tAboutSearch);
				if (hEdit)
					gpConEmu->SetWindowTheme(hEdit, L"Explorer", nullptr);

				SendMessageW(hDlg, WM_THEMECHANGED, 0, 0);
			}

			const wchar_t* pszActivePage = reinterpret_cast<LPCWSTR>(lParam);

			const wchar_t* psStage = (ConEmuVersionStage == CEVS_STABLE) ? L"{Stable}"
				: (ConEmuVersionStage == CEVS_PREVIEW) ? L"{Preview}"
				: L"{Alpha}";
			const CEStr lsTitle(
				CLngRc::getRsrc(lng_DlgAbout/*"About"*/),
				L" ",
				gpConEmu->GetDefaultTitle(),
				L" ",
				psStage,
				nullptr);
			if (lsTitle)
			{
				SetWindowText(hDlg, lsTitle);
			}

			if (hClassIcon)
			{
				SendMessage(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hClassIcon));
				SendMessage(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(CreateNullIcon()));
				SetClassLongPtr(hDlg, GCLP_HICON, reinterpret_cast<LONG_PTR>(hClassIcon));
			}

			// "Console Emulation program (local terminal)"
			SetDlgItemText(hDlg, stConEmuAbout, CLngRc::getRsrc(lng_AboutAppName));

			SetDlgItemText(hDlg, stConEmuUrl, gsHomePage);

			EditIconHint_Set(hDlg, GetDlgItem(hDlg, tAboutSearch), true,
				CLngRc::getRsrc(lng_Search/*"Search"*/),
				false, UM_SEARCH, IDOK);
			EditIconHint_Subclass(hDlg);

			CEStr pszLabel = GetDlgItemTextPtr(hDlg, stConEmuVersion);
			if (pszLabel)
			{
				CEStr pszSet;

				if (gpUpd)
				{
					const CEStr pszVerInfo = gpUpd->GetCurVerInfo();
					if (pszVerInfo)
					{
						pszSet = CEStr(pszLabel, L" ", pszVerInfo);
					}
				}

				if (!pszSet)
				{
					pszSet = CEStr(pszLabel, L" ", CLngRc::getRsrc(lng_PleaseCheckManually/*"Please check for updates manually"*/));
				}

				if (pszSet)
				{
					SetDlgItemText(hDlg, stConEmuVersion, pszSet);
				}

				pszLabel.Release();
			}

			// ReSharper disable once CppLocalVariableMayBeConst
			HWND hTab = GetDlgItem(hDlg, tbAboutTabs);
			INT_PTR nPage = -1;

			for (size_t i = 0; i < countof(Pages); i++)
			{
				TCITEM tie = {};
				tie.mask = TCIF_TEXT;
				tie.pszText = const_cast<wchar_t*>(Pages[i].Title);
				TabCtrl_InsertItem(hTab, i, &tie);

				if (pszActivePage && (lstrcmpi(pszActivePage, Pages[i].Title) == 0))
					nPage = i;
			}


			if (nPage >= 0)
			{
				TabSelected(hDlg, static_cast<int>(nPage));
				TabCtrl_SetCurSel(hTab, static_cast<int>(nPage));
			}
			else
			{
				TabSelected(hDlg, 0);
				TabCtrl_SetCurSel(hTab, 0);
			}
			DarkMode::ApplyDialog(hDlg);

			SetFocus(hTab);

			return FALSE;
		}

		case WM_CTLCOLORSTATIC:
		{
			if (const auto brush = CSettings::SettingsThemeBrush(messg,
				reinterpret_cast<HDC>(wParam), reinterpret_cast<HWND>(lParam),
				GetDlgCtrlID(reinterpret_cast<HWND>(lParam)) == stConEmuUrl))
				return brush;
			if (GetWindowLongPtr(reinterpret_cast<HWND>(lParam), GWLP_ID) == stConEmuUrl)
			{
				SetTextColor(reinterpret_cast<HDC>(wParam), GetSysColor(COLOR_HOTLIGHT));
				HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
				SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
				return reinterpret_cast<INT_PTR>(hBrush);
			}
			else
			{
				SetTextColor(reinterpret_cast<HDC>(wParam), GetSysColor(COLOR_WINDOWTEXT));
				HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
				SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
				return reinterpret_cast<INT_PTR>(hBrush);
			}
		}

		case WM_SETCURSOR:
			{
				if (GetWindowLongPtr(reinterpret_cast<HWND>(wParam), GWLP_ID) == stConEmuUrl)
				{
					SetCursor(LoadCursor(nullptr, IDC_HAND));
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, TRUE);
					return TRUE;
				}
				return FALSE;
			}

		case WM_COMMAND:
			switch (HIWORD(wParam))
			{
			case BN_CLICKED:
				switch (LOWORD(wParam))
				{
					case IDOK:
					case IDCANCEL:
					case IDCLOSE:
						aboutProc(hDlg, WM_CLOSE, 0, 0);
						return 1;
					case stConEmuUrl:
						ConEmuAbout::OnInfo_HomePage();
						return 1;
				} // BN_CLICKED
				break;
			case EN_SETFOCUS:
				switch (LOWORD(wParam))
				{
				case tAboutText:
					{
						// Do not autosel all text
						// ReSharper disable once CppLocalVariableMayBeConst
						HWND hEdit = reinterpret_cast<HWND>(lParam);
						DWORD nStart = 0, nEnd = 0;
						SendMessage(hEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&nStart), reinterpret_cast<LPARAM>(&nEnd));
						if (nStart != nEnd)
						{
							SendMessage(hEdit, EM_SETSEL, nTextSelStart, nTextSelEnd);
						}
					}
					break;
				}
			} // switch (HIWORD(wParam))
			break;

		case WM_NOTIFY:
		{
			const auto* nmhdr = reinterpret_cast<LPNMHDR>(lParam);
			if ((nmhdr->code == TCN_SELCHANGE) && (nmhdr->idFrom == tbAboutTabs))
			{
				const int iPage = TabCtrl_GetCurSel(nmhdr->hwndFrom);
				if ((iPage >= 0) && (iPage < static_cast<int>(countof(Pages))))
					TabSelected(hDlg, iPage);
			}
			break;
		}

		case UM_SEARCH:
			searchProc(hDlg, reinterpret_cast<HWND>(lParam), false);
			break;

		case UM_EDIT_KILL_FOCUS:
			SendMessage(reinterpret_cast<HWND>(lParam), EM_GETSEL, reinterpret_cast<WPARAM>(&nTextSelStart), reinterpret_cast<LPARAM>(&nTextSelEnd));
			break;

		case WM_CLOSE:
			//if (ghWnd == nullptr)
			gpConEmu->OnOurDialogClosed();
			if (mp_DpiAware)
				mp_DpiAware->Detach();
			EndDialog(hDlg, IDOK);
			SafeDelete(mp_ImgBtn);
			break;

		case WM_DESTROY:
			mh_AboutDlg = nullptr;
			break;


		default:
			if (mp_DpiAware && mp_DpiAware->ProcessDpiMessages(hDlg, messg, wParam, lParam))
			{
				return TRUE;
			}
	}

	return FALSE;
}

void ConEmuAbout::searchProc(HWND hDlg, HWND hSearch, bool bReentr)
{
	// ReSharper disable once CppLocalVariableMayBeConst
	HWND hEdit = GetDlgItem(hDlg, tAboutText);
	const CEStr pszPart = GetDlgItemTextPtr(hSearch, 0);
	const CEStr pszText = GetDlgItemTextPtr(hEdit, 0);
	bool bRetry = false;

	if (!pszPart.IsEmpty() && !pszText.IsEmpty())
	{
		LPCWSTR pszFrom = pszText.c_str();

		DWORD nStart = 0, nEnd = 0;
		SendMessage(hEdit, EM_GETSEL, reinterpret_cast<WPARAM>(&nStart), reinterpret_cast<LPARAM>(&nEnd));

		const size_t cchMax = wcslen(pszText);
		const size_t cchFrom = std::max(nStart,nEnd);
		if (cchMax > cchFrom)
			pszFrom += cchFrom;

		LPCWSTR pszFind = StrStrI(pszFrom, pszPart);
		if (!pszFind && bReentr && (pszFrom != pszText))
			pszFind = StrStrI(pszText, pszPart);

		if (pszFind)
		{
			const wchar_t szBrkChars[] = L"()[]<>{}:;,.-=\\/ \t\r\n";
			LPCWSTR pszEnd = wcspbrk(pszFind, szBrkChars);
			const INT_PTR nPartLen = wcslen(pszPart);
			if (!pszEnd || ((pszEnd - pszFind) > std::max<ssize_t>(nPartLen,60)))
				pszEnd = pszFind + nPartLen;
			while ((pszFind > pszFrom) && !wcschr(szBrkChars, *(pszFind-1)))
				pszFind--;
			//SetFocus(hEdit);
			nTextSelStart = static_cast<DWORD>(pszEnd - pszText.c_str());
			nTextSelEnd = static_cast<DWORD>(pszFind - pszText.c_str());
			SendMessage(hEdit, EM_SETSEL, nTextSelStart, nTextSelEnd);
			SendMessage(hEdit, EM_SCROLLCARET, 0, 0);
		}
		else if (!bReentr)
		{
			// ReSharper disable once CppLocalVariableMayBeConst
			HWND hTab = GetDlgItem(hDlg, tbAboutTabs);
			const int iPage = TabCtrl_GetCurSel(hTab);
			int iFound = -1;
			for (int s = 0; (iFound == -1) && (s <= 1); s++)
			{
				const int iFrom = (s == 0) ? (iPage+1) : 0;
				const int iTo = (s == 0) ? static_cast<int>(countof(Pages)) : (iPage - 1);
				for (int i = iFrom; i < iTo; i++)
				{
					if (StrStrI(Pages[i].Title, pszPart)
						|| StrStrI(Pages[i].Text, pszPart))
					{
						iFound = i; break;
					}
				}
			}
			if (iFound >= 0)
			{
				TabSelected(hDlg, iFound);
				TabCtrl_SetCurSel(hTab, iFound);
				//SetFocus(hEdit);
				bRetry = true;
			}
		}
	}

	if (bRetry)
	{
		searchProc(hDlg, hSearch, true);
	}
}

void ConEmuAbout::InitCommCtrls()
{
	if (mb_CommCtrlsInitialized)
		return;

	INITCOMMONCONTROLSEX icex{};
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC   = ICC_COOL_CLASSES|ICC_BAR_CLASSES|ICC_TAB_CLASSES|ICC_PROGRESS_CLASS; //|ICC_STANDARD_CLASSES|ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icex);

	mb_CommCtrlsInitialized = true;
}

void ConEmuAbout::OnInfo_OnlineWiki(LPCWSTR asPageName /*= nullptr*/)
{
	const CEStr szUrl(CEWIKIBASE, asPageName ? asPageName : L"TableOfContents", L".html");
	const DWORD shellRc = static_cast<DWORD>(reinterpret_cast<INT_PTR>(
		ShellExecute(ghWnd, L"open", szUrl, nullptr, nullptr, SW_SHOWNORMAL)));
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::OnInfo_Donate()
{
	const int nBtn = MsgBox(
		L"You can show your appreciation and support future development by donating.\n\n"
		L"Open ConEmu's donate web page?"
		,MB_YESNO|MB_ICONINFORMATION);

	if (nBtn == IDYES)
	{
		OnInfo_DonateLink();
		//OnInfo_HomePage();
	}
}

void ConEmuAbout::OnInfo_DonateLink()
{
	const DWORD shellRc = static_cast<DWORD>(reinterpret_cast<INT_PTR>(
		ShellExecute(ghWnd, L"open", gsDonatePage, nullptr, nullptr, SW_SHOWNORMAL)));
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}
void ConEmuAbout::OnInfo_FlattrLink()
{
	const DWORD shellRc = static_cast<DWORD>(reinterpret_cast<INT_PTR>(
		ShellExecute(ghWnd, L"open", gsFlattrPage, nullptr, nullptr, SW_SHOWNORMAL)));
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::TabSelected(HWND hDlg, int idx)
{
	if (idx < 0 || idx >= static_cast<int>(countof(Pages)))
		return;

	wcscpy_c(sLastOpenTab, Pages[idx].Title);
	LPCWSTR pszNewText = Pages[idx].Text;
	// ReSharper disable once CppJoinDeclarationAndAssignment
	CEStr lsTemp;
	if (gpConEmu->mp_PushInfo && gpConEmu->mp_PushInfo->mp_Active && gpConEmu->mp_PushInfo->mp_Active->pszFullMessage)
	{
		// EDIT control requires \r\n as line endings
		lsTemp = CEStr(gpConEmu->mp_PushInfo->mp_Active->pszFullMessage, L"\r\n\r\n\r\n", pszNewText);
		pszNewText = lsTemp.ms_Val;
	}
	SetDlgItemText(hDlg, tAboutText, pszNewText);
}

void ConEmuAbout::LogStartEnvInt(LPCWSTR asText, LPARAM lParam, bool bFirst, bool bNewLine)
{
	_ASSERTE(isMainThread());
	if (!gsSysInfo)
		gsSysInfo = new CEStr;
	gsSysInfo->Append(asText, bNewLine ? L"\r\n" : nullptr);

	if (bFirst && gpConEmu)
	{
		gsSysInfo->Append(L"  AppID: ", gpConEmu->ms_AppID, L"\r\n");
	}
}

void ConEmuAbout::ReloadSysInfo()
{
	if (!gpStartEnv)
		return;

	_ASSERTE(isMainThread());
	_ASSERTE(lstrcmp(Pages[countof(Pages)-1].Title, L"SysInfo") == 0);
	if (gsSysInfo)
		gsSysInfo->Clear();

	LoadStartupEnvEx::ToString(gpStartEnv, LogStartEnvInt, 0);

	Pages[countof(Pages)-1].Text = gsSysInfo ? gsSysInfo->c_str(L"") : L"";
}

void ConEmuAbout::OnInfo_About(LPCWSTR asPageName /*= nullptr*/)
{
	InitCommCtrls();

	bool bOk = false;

	if (!asPageName && sLastOpenTab[0])
	{
		// Reopen last active tab
		asPageName = sLastOpenTab;
	}

	ReloadSysInfo();

	{
		DontEnable de;
		CDpiForDialog::Create(mp_DpiAware);
		// ReSharper disable once CppLocalVariableMayBeConst
		HWND hParent = (ghOpWnd && IsWindowVisible(ghOpWnd)) ? ghOpWnd : ghWnd;
		// Modal dialog (CreateDialog)
		const INT_PTR iRc = CDynDialog::ExecuteDialog(IDD_ABOUT, hParent, aboutProc, reinterpret_cast<LPARAM>(asPageName));
		bOk = (iRc != 0 && iRc != -1);

		mh_AboutDlg = nullptr;
		if (mp_DpiAware)
			mp_DpiAware->Detach();

		#ifdef _DEBUG
		// Any problems with dialog resource?
		if (!bOk) DisplayLastError(L"DialogBoxParam(IDD_ABOUT) failed");
		#endif
	}

	if (!bOk)
	{
		const CEStr szTitle(gpConEmu->GetDefaultTitle(), L" ", CLngRc::getRsrc(lng_DlgAbout/*"About"*/));
		DontEnable de;
		MSGBOXPARAMS mb = {sizeof(MSGBOXPARAMS), ghWnd, g_hInstance,
			pAbout,
			szTitle.ms_Val,
			MB_USERICON, MAKEINTRESOURCE(IMAGE_ICON), 0, nullptr, LANG_NEUTRAL
		};
		MSetter lInCall(&gnInMsgBox);
		// Use MessageBoxIndirect instead of MessageBox to show our icon instead of std ICONINFORMATION
		MessageBoxIndirect(&mb);
	}
}

void ConEmuAbout::OnInfo_WhatsNew(bool bLocal)
{
	INT_PTR iExec = -1;

	if (bLocal)
	{
		const CEStr sFile(gpConEmu->ms_ConEmuBaseDir, L"\\WhatsNew-ConEmu.txt");

		if (FileExists(sFile))
		{
			iExec = reinterpret_cast<INT_PTR>(
				ShellExecute(ghWnd, L"open", sFile, nullptr, nullptr, SW_SHOWNORMAL));
			if (iExec >= 32)
			{
				return;
			}
		}
	}

	iExec = reinterpret_cast<INT_PTR>(
		ShellExecute(ghWnd, L"open", gsWhatsNew, nullptr, nullptr, SW_SHOWNORMAL));
	if (iExec >= 32)
	{
		return;
	}

	DisplayLastError(L"File 'WhatsNew-ConEmu.txt' not found, go to web page failed", static_cast<int>(LODWORD(iExec)));
}

void ConEmuAbout::OnInfo_Help()
{
	static HMODULE hhctrl = nullptr;

	if (!hhctrl) hhctrl = GetModuleHandle(L"hhctrl.ocx");

	if (!hhctrl) hhctrl = LoadLibrary(L"hhctrl.ocx");

	if (hhctrl)
	{
		typedef BOOL (WINAPI* HTMLHelpW_t)(HWND hWnd, LPCWSTR pszFile, INT uCommand, INT dwData);
		// ReSharper disable once CppLocalVariableMayBeConst
		HTMLHelpW_t fHTMLHelpW = reinterpret_cast<HTMLHelpW_t>(GetProcAddress(hhctrl, "HtmlHelpW"));

		if (fHTMLHelpW)
		{
			wchar_t szHelpFile[MAX_PATH*2];
			lstrcpy(szHelpFile, gpConEmu->ms_ConEmuChm);
			//wchar_t* pszSlash = wcsrchr(szHelpFile, L'\\');
			//if (pszSlash) pszSlash++; else pszSlash = szHelpFile;
			//lstrcpy(pszSlash, L"ConEmu.chm");
			// lstrcat(szHelpFile, L::/Intro.htm");
			#define HH_HELP_CONTEXT 0x000F
			#define HH_DISPLAY_TOC  0x0001
			//fHTMLHelpW(nullptr /*чтобы окно не блокировалось*/, szHelpFile, HH_HELP_CONTEXT, contextID);
			fHTMLHelpW(nullptr /*чтобы окно не блокировалось*/, szHelpFile, HH_DISPLAY_TOC, 0);
		}
	}
}

void ConEmuAbout::OnInfo_HomePage()
{
	const DWORD shellRc = static_cast<DWORD>(reinterpret_cast<INT_PTR>(
		ShellExecute(ghWnd, L"open", gsHomePage, nullptr, nullptr, SW_SHOWNORMAL)));
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::OnInfo_DownloadPage()
{
	const DWORD shellRc = static_cast<DWORD>(reinterpret_cast<INT_PTR>(
		ShellExecute(ghWnd, L"open", gsDownlPage, nullptr, nullptr, SW_SHOWNORMAL)));
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::OnInfo_FirstStartPage()
{
	const DWORD shellRc = static_cast<DWORD>(reinterpret_cast<INT_PTR>(
		ShellExecute(ghWnd, L"open", gsFirstStart, nullptr, nullptr, SW_SHOWNORMAL)));
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::OnInfo_ReportBug()
{
	const DWORD shellRc = static_cast<DWORD>(reinterpret_cast<INT_PTR>(
		ShellExecute(ghWnd, L"open", gsReportBug, nullptr, nullptr, SW_SHOWNORMAL)));
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
}

void ConEmuAbout::OnInfo_ReportCrash(LPCWSTR asDumpWasCreatedMsg)
{
	if (nLastCrashReported)
	{
		// if previous gsReportCrash was opened less than 60 sec ago
		const DWORD nLast = GetTickCount() - nLastCrashReported;
		if (nLast < 60000)
		{
			// Skip this time
			return;
		}
	}

	if (asDumpWasCreatedMsg && !*asDumpWasCreatedMsg)
	{
		asDumpWasCreatedMsg = nullptr;
	}

	const DWORD shellRc = static_cast<DWORD>(reinterpret_cast<INT_PTR>(
		ShellExecute(ghWnd, L"open", gsReportCrash, nullptr, nullptr, SW_SHOWNORMAL)));
	if (shellRc <= 32)
	{
		DisplayLastError(L"ShellExecute failed", shellRc);
	}
	else if (asDumpWasCreatedMsg)
	{
		MsgBox(asDumpWasCreatedMsg, MB_OK|MB_ICONEXCLAMATION|MB_SYSTEMMODAL);
	}

	nLastCrashReported = GetTickCount();
}

void ConEmuAbout::OnInfo_ThrowTrapException(bool bMainThread)
{
	if (bMainThread)
	{
		if (MsgBox(L"Are you sure?\nApplication will terminate after that!\nThrow exception in ConEmu's main thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
		{
			//#ifdef _DEBUG
			//MyAssertTrap();
			//#else
			//DebugBreak();
			//#endif
			// -- trigger division by 0
			RaiseTestException();
		}
	}
	else
	{
		if (MsgBox(L"Are you sure?\nApplication will terminate after that!\nThrow exception in ConEmu's monitor thread?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2)==IDYES)
		{
			CVConGuard VCon;
			if ((gpConEmu->GetActiveVCon(&VCon) >= 0) && VCon->RCon())
				VCon->RCon()->MonitorAssertTrap();
		}
	}
}
