
/*
Copyright (c) 2016-present Maximus5
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

#include "ConEmu.h"
#include "OptionsClass.h"
#include "SetPgAppear.h"

CSetPgAppear::CSetPgAppear()
{
}

CSetPgAppear::~CSetPgAppear()
{
}

LRESULT CSetPgAppear::OnInitDialog(HWND hDlg, bool abInitial)
{
	checkDlgButton(hDlg, cbHideCaption, gpSet->isHideCaption);

	checkDlgButton(hDlg, cbHideCaptionAlways, gpSet->isHideCaptionAlways());
	EnableWindow(GetDlgItem(hDlg, cbHideCaptionAlways), !gpSet->isForcedHideCaptionAlways());

	// Скрытие рамки
	SetDlgItemInt(hDlg, tHideCaptionAlwaysFrame, gpSet->HideCaptionAlwaysFrame(), TRUE);
	SetDlgItemInt(hDlg, tHideCaptionAlwaysDelay, gpSet->nHideCaptionAlwaysDelay, FALSE);
	SetDlgItemInt(hDlg, tHideCaptionAlwaysDissapear, gpSet->nHideCaptionAlwaysDisappear, FALSE);

	const WORD themeButton = gpSet->nTheme == theme_Dark ? rbThemeDark
		: (gpSet->nTheme == theme_Light ? rbThemeLight : rbThemeAuto);
	checkRadioButton(hDlg, rbThemeDark, rbThemeAuto, themeButton);
	for (const WORD id : {gbTheme, rbThemeDark, rbThemeLight, rbThemeAuto})
		enableDlgItem(hDlg, id, global::g_darkModeSupported);

	checkDlgButton(hDlg, cbEnhanceGraphics, gpSet->isEnhanceGraphics);

	//checkDlgButton(hDlg, cbEnhanceButtons, gpSet->isEnhanceButtons);

	//checkDlgButton(hDlg, cbAlwaysShowScrollbar, gpSet->isAlwaysShowScrollbar);
	checkRadioButton(hDlg, rbScrollbarHide, rbScrollbarAuto, (gpSet->isAlwaysShowScrollbar==0) ? rbScrollbarHide : (gpSet->isAlwaysShowScrollbar==1) ? rbScrollbarShow : rbScrollbarAuto);
	checkDlgButton(hDlg, cbScrollbarDynamic, gpSet->isDynamicBufferHeight);
	SetDlgItemInt(hDlg, tScrollAppearDelay, gpSet->nScrollBarAppearDelay, FALSE);
	SetDlgItemInt(hDlg, tScrollDisappearDelay, gpSet->nScrollBarDisappearDelay, FALSE);

	checkDlgButton(hDlg, cbAlwaysOnTop, gpSet->isAlwaysOnTop);

	checkDlgButton(hDlg, cbNumberInCaption, gpSet->isNumberInCaption);

	checkDlgButton(hDlg, cbMultiCon, gpSet->mb_isMulti);
	checkDlgButton(hDlg, cbMultiShowButtons, gpSet->isMultiShowButtons);
	checkDlgButton(hDlg, cbMultiShowSearch, gpSet->isMultiShowSearch);

	checkDlgButton(hDlg, cbSingleInstance, gpSetCls->IsSingleInstanceArg());
	enableDlgItem(hDlg, cbSingleInstance, (gpSet->isQuakeStyle == 0));

	checkDlgButton(hDlg, cbShowHelpTooltips, gpSet->isShowHelpTooltips);

	return 0;
}

LRESULT CSetPgAppear::OnEditChanged(HWND hDlg, WORD nCtrlId)
{
	switch (nCtrlId)
	{
	case tHideCaptionAlwaysFrame:
		{
			BOOL lbOk = FALSE;
			int nNewVal = GetDlgItemInt(hDlg, nCtrlId, &lbOk, TRUE);

			if (lbOk && (gpSet->nHideCaptionAlwaysFrame != ((nNewVal < 0) ? 255 : (BYTE)nNewVal)))
			{
				gpSet->nHideCaptionAlwaysFrame = (nNewVal < 0) ? 255 : (BYTE)nNewVal;
				gpConEmu->RefreshWindowStyles();
				gpConEmu->UpdateWindowRgn();
			}
		}
		break;

	case tHideCaptionAlwaysDelay:
	case tHideCaptionAlwaysDissapear:
	case tScrollAppearDelay:
	case tScrollDisappearDelay:
		{
			BOOL lbOk = FALSE;
			UINT nNewVal = GetDlgItemInt(hDlg, nCtrlId, &lbOk, FALSE);

			if (lbOk)
			{
				switch (nCtrlId)
				{
				case tHideCaptionAlwaysDelay:
					gpSet->nHideCaptionAlwaysDelay = nNewVal;
					break;
				case tHideCaptionAlwaysDissapear:
					gpSet->nHideCaptionAlwaysDisappear = nNewVal;
					break;
				case tScrollAppearDelay:
					if (nNewVal >= SCROLLBAR_DELAY_MIN && nNewVal <= SCROLLBAR_DELAY_MAX)
						gpSet->nScrollBarAppearDelay = nNewVal;
					else if (nNewVal > SCROLLBAR_DELAY_MAX)
						SetDlgItemInt(hDlg, tScrollAppearDelay, SCROLLBAR_DELAY_MAX, FALSE);
					break;
				case tScrollDisappearDelay:
					if (nNewVal >= SCROLLBAR_DELAY_MIN && nNewVal <= SCROLLBAR_DELAY_MAX)
						gpSet->nScrollBarDisappearDelay = nNewVal;
					else if (nNewVal > SCROLLBAR_DELAY_MAX)
						SetDlgItemInt(hDlg, tScrollDisappearDelay, SCROLLBAR_DELAY_MAX, FALSE);
					break;
				}
			}
		}
		break;

	default:
		_ASSERTE(FALSE && "EditBox was not processed");
	}

	return 0;
}
