
/*
Copyright (c) 2010-present Maximus5
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

#pragma once

#ifdef FAR_UNICODE
	#if FAR_UNICODE>=1867
		#if FARMANAGERVERSION_BUILD>=2800
			#define FAR_INT intptr_t
			#define InfoT InfoW2800
		#else
			#define FAR_INT int
			#define InfoT InfoW1900
		#endif
		#define _GetCheck(i) (int)InfoT->SendDlgMessage(hDlg,DM_GETCHECK,i,0)
		#define GetDataPtr(i) ((const wchar_t *)InfoT->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define GetTextPtr(i) ((const wchar_t *)InfoT->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define SETTEXT(itm,txt) itm.Data = txt
		#define wsprintfT wsprintfW
		#define GetMsgT GetMsgW
		#define FAR_T(s) L ## s
		#define FAR_CHAR wchar_t
		#define strcpyT lstrcpyW
		#define strlenT lstrlenW
		#define FAR_PTR void*
	#else
		#define FAR_INT int
		#define InfoT InfoW995
		#define _GetCheck(i) (int)InfoW995->SendDlgMessage(hDlg,DM_GETCHECK,i,0)
		#define GetDataPtr(i) ((const wchar_t *)InfoW995->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define GetTextPtr(i) ((const wchar_t *)InfoW995->SendDlgMessage(hDlg,DM_GETCONSTTEXTPTR,i,0))
		#define SETTEXT(itm,txt) itm.PtrData = txt
		#define wsprintfT wsprintfW
		#define GetMsgT GetMsgW
		#define FAR_T(s) L ## s
		#define FAR_CHAR wchar_t
		#define strcpyT lstrcpyW
		#define strlenT lstrlenW
		#define FAR_PTR LONG_PTR
	#endif
#else
	#define FAR_INT int
	#define InfoT InfoA
	#define _GetCheck(i) items[i].Selected
	#define GetDataPtr(i) items[i].Data
	#define SETTEXT(itm,txt) lstrcpyA(itm.Data, txt)
	#define wsprintfT wsprintfA
	#define GetMsgT GetMsg
	#define FAR_T(s) s
	#define FAR_CHAR char
	#define strcpyT lstrcpyA
	#define strlenT lstrlenA
	#define FAR_PTR LONG_PTR
#endif


enum
{
	cfgTitle         = 0,
	cfgPluginEnabled /*= 1*/,
	cfgPathLabel     /*= 2*/,
	cfgPath          /*= 3*/,
	cfgMonitorFile   /*= 4*/,
	cfgSeparator     /*= 5*/,
	cfgOk            /*= 6*/,
	cfgCancel        /*= 7*/,
};

extern bool gbGdiPlusInitialized;

#if FAR_UNICODE>=1867
static INT_PTR WINAPI ConfigDlgProc(HANDLE hDlg, FAR_INT Msg, FAR_INT Param1, FAR_PTR Param2)
#else
static LONG_PTR WINAPI ConfigDlgProc(HANDLE hDlg, int Msg, int Param1, LONG_PTR Param2)
#endif
{
	if (Msg == DN_BTNCLICK)
	{
		if (Param1 == cfgPluginEnabled)
		{
			gbBackgroundEnabled = (Param2 != 0);

			// If plugin was already loaded, and checkbox turned ON again
			if (gbBackgroundEnabled && WasXmlLoaded())
			{
				// Force recheck xml file
				CheckXmlFile(true);
			}

			// Обновить или отключить
			StartPlugin(TRUE /*НЕ считывать параметры из реестра*/);
		}
		else if (Param1 == cfgMonitorFile)
		{
			//Issue 1230
			gbMonitorFileChange = (Param2 != 0);
			CheckXmlFile(true);
		}
	}
	else if (Msg == DN_EDITCHANGE)
	{
		if (Param1 == cfgPath)
		{
			FarDialogItem* pItem = (FarDialogItem*)Param2;
#ifdef FAR_UNICODE
	#if FAR_UNICODE>=1867
			lstrcpyn(gsXmlConfigFile, pItem->Data, countof(gsXmlConfigFile));
	#else
			lstrcpyn(gsXmlConfigFile, pItem->PtrData, countof(gsXmlConfigFile));
	#endif
#else
			MultiByteToWideChar(CP_OEMCP, 0, pItem->Data, -1, gsXmlConfigFile, countof(gsXmlConfigFile));
#endif
			CheckXmlFile(true);

			// Обновить или отключить
			StartPlugin(TRUE /*НЕ считывать параметры из реестра*/);
		}
	}

	return InfoT->DefDlgProc(hDlg, Msg, Param1, Param2);
}

static int ConfigureProc(int ItemNumber)
{
	if (!InfoT)
		return false;

	int height = 12, width = 50;
	static FarDialogItem items[] =
	{
		{DI_DOUBLEBOX, 3,  1,  width - 4, height - 2}, //cfgTitle

		#if FAR_UNICODE>=1867
		{DI_CHECKBOX,  5,  3,  0,  0, {0}, NULL, NULL, DIF_FOCUS},    //cfgPluginEnabled
		#else
		{DI_CHECKBOX,  5,  3,  0,  0, true},    //cfgPluginEnabled
		#endif

		{DI_TEXT,      5,  5,  0,  0},   //cfgPathLabel, cfgPath
		#if FAR_UNICODE>=1867
		{DI_EDIT,  5,  6,  width - 6, 0},
		#else
		{DI_EDIT,  5,  6,  width - 6, 0},
		#endif

		#if FAR_UNICODE>=1867
		{DI_CHECKBOX,  5,  7},    //cfgMonitorFile
		#else
		{DI_CHECKBOX,  5,  7},    //cfgMonitorFile
		#endif
		
		#if FAR_UNICODE>=1867
		{DI_TEXT,      0, 8,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_SEPARATOR},
		#else
		{DI_TEXT,      0, 8,  0,  0, false, {(DWORD_PTR)0}, DIF_SEPARATOR},
		#endif

		#if FAR_UNICODE>=1867
		{DI_BUTTON,    0, 9,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_CENTERGROUP|DIF_DEFAULTBUTTON},  //cfgOk
		{DI_BUTTON,    0, 9,  0,  0, {(DWORD_PTR)0}, NULL, NULL, DIF_CENTERGROUP}, //cfgCancel
		#else
		{DI_BUTTON,    0, 9,  0,  0, false,  {(DWORD_PTR)true},        DIF_CENTERGROUP, true},  //cfgOk
		{DI_BUTTON,    0, 9,  0,  0, false,  {(DWORD_PTR)false},       DIF_CENTERGROUP, false}, //cfgCancel
		#endif
	};
	SETTEXT(items[cfgTitle], GetMsgT(CEPluginName));
	SETTEXT(items[cfgPluginEnabled], GetMsgT(CEPluginEnable));
	items[cfgPluginEnabled].Selected = gbBackgroundEnabled;
	SETTEXT(items[cfgPathLabel], GetMsgT(CEPathLabel));
#ifdef FAR_UNICODE
	SETTEXT(items[cfgPath], *gsXmlConfigFile ? gsXmlConfigFile : szDefaultXmlName);
#else
	char szXmlConfigFile[MAX_PATH];
	WideCharToMultiByte(CP_OEMCP, 0, *gsXmlConfigFile ? gsXmlConfigFile : szDefaultXmlName, -1, szXmlConfigFile, countof(szXmlConfigFile), 0,0);
	SETTEXT(items[cfgPath], szXmlConfigFile);
#endif
	SETTEXT(items[cfgMonitorFile], GetMsgT(CEMonitorFileChange));
	items[cfgMonitorFile].Selected = gbMonitorFileChange;
	SETTEXT(items[cfgOk], GetMsgT(CEBtnOK));
	SETTEXT(items[cfgCancel], GetMsgT(CEBtnCancel));
	FAR_INT dialog_res = 0;
	// Запомнить текущие значения, чтобы восстановить их если Esc нажат
	BOOL bCurBackgroundEnabled = gbBackgroundEnabled;
	wchar_t szCurXmlConfigFile[MAX_PATH]; lstrcpyn(szCurXmlConfigFile, gsXmlConfigFile, countof(szCurXmlConfigFile));
#ifdef FAR_UNICODE
	HANDLE hDlg = NULL;
	#if FAR_UNICODE>=1867
		hDlg = InfoT->DialogInit(&guid_ConEmuBg, &guid_ConEmuBgCfgDlg,
								 -1, -1, width, height,
								 NULL/*L"Configure"*/, items, countof(items), 
								 0, 0/*Flags*/, ConfigDlgProc, 0);
	#else
		hDlg = InfoT->DialogInit(InfoT->ModuleNumber, -1, -1, width, height,
								 NULL/*L"Configure"*/, items, countof(items), 0, 0/*Flags*/, ConfigDlgProc, 0/*DlgProcParam*/);
	#endif
	dialog_res = InfoT->DialogRun(hDlg);
#else
	dialog_res = InfoT->DialogEx(InfoT->ModuleNumber,-1,-1,width,height,
	                             NULL/*"Configure"*/, items, countof(items), 0, 0, ConfigDlgProc, NULL);
#endif

	if (dialog_res != -1 && dialog_res != cfgCancel)
	{
		BYTE cVal;
		gbBackgroundEnabled = cVal = _GetCheck(cfgPluginEnabled);
		const FAR_CHAR* psz;
#ifdef FAR_UNICODE
		psz = GetDataPtr(cfgPath);
		lstrcpyn(gsXmlConfigFile, psz, countof(gsXmlConfigFile));
#else
		psz = GetDataPtr(cfgPath);
		MultiByteToWideChar(CP_OEMCP, 0, psz, -1, gsXmlConfigFile, countof(gsXmlConfigFile));
#endif
		SettingsSave();
	}
	else
	{
		gbBackgroundEnabled = bCurBackgroundEnabled;
		lstrcpyn(gsXmlConfigFile, szCurXmlConfigFile, countof(gsXmlConfigFile));
	}

	if (gbGdiPlusInitialized)
	{
		CheckXmlFile(true);
	}

	// Обновить или отключить
	StartPlugin(TRUE /*НЕ считывать параметры из реестра*/);
#ifdef FAR_UNICODE
	InfoT->DialogFree(hDlg);
#if FAR_UNICODE >= 2800 && (defined(_M_ARM64) || defined(__aarch64__))
	// Registration queues painting. Redraw once the dialog no longer covers the panels.
	InfoT->AdvControl(&guid_ConEmuBg, ACTL_REDRAWALL, 0, nullptr);
#endif
#endif
	return(true);
}
