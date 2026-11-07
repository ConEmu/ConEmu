
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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после загрузки плагина показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
//  #define SHOW_WRITING_RECTS
#endif

#define SHOWDEBUGSTR
//#define MCHKHEAP
#define DEBUGSTRMENU(s) DEBUGSTR(s)
#define DEBUGSTRCTRL(s) DEBUGSTR(s)


#include "../common/Common.h"
#pragma warning( disable : 4995 )
#include "../common/pluginW1761.hpp" // Отличается от 995 наличием SynchoApi
#pragma warning( default : 4995 )
#include "../common/ConEmuCheck.h"
#include "../common/RgnDetect.h"
#include "../common/TerminalMode.h"
#include "../common/FarVersion.h"
#include "../common/HkFunc.h"
#include "../common/MFileMapping.h"
#include "../common/MSection.h"
#include "../common/WFiles.h"
#include "../common/WObjects.h"
#include "ConEmuTh.h"
#include "ConEmuTh_Lang.h"
#include "ImgCache.h"

#include <tlhelp32.h>

using ConEmu::PaletteColors;

#define Free free
#define Alloc calloc

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

//#define ConEmuTh_SysID 0x43455568 // 'CETh'

#if defined(__GNUC__)
extern "C" {
	BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
	void WINAPI SetStartupInfoW(void *aInfo);
};
#endif

BOOL gbPreloadByDefault = TRUE;
DWORD gdwModes[2] = {0,0}; // pvm_XXX
PanelViewSetMapping gThSet = {0}; // параметры получаются из мэппинга при открытии плага или при перерегистрации
HWND ghConEmuRoot = NULL, ghConEmuWnd = NULL;
HMODULE ghPluginModule = NULL; // ConEmuTh.dll - сам плагин
BOOL TerminalMode = FALSE;
DWORD gnSelfPID = 0;
DWORD gnMainThreadId = 0, gnMainThreadIdInitial = 0;
HANDLE ghDisplayThread = NULL; DWORD gnDisplayThreadId = 0;
//HWND ghLeftView = NULL, ghRightView = NULL;
//wchar_t* gszRootKey = NULL;
FarVersion gFarVersion = {};
//HMODULE ghConEmuDll = NULL;
RegisterPanelView_t gfRegisterPanelView = NULL;
GetFarHWND2_t gfGetFarHWND2 = NULL;
CeFullPanelInfo pviLeft = {0}, pviRight = {0};
int ShowLastError();
CRgnDetect *gpRgnDetect = NULL;
CImgCache  *gpImgCache = NULL;
CEFAR_INFO_MAPPING gFarInfo = {0};
CESERVER_PALETTE gThPal = {};
PaletteColors gcrCurColors; // << gThPal.crPalette; use RefreshPalette
bool gbFadeColors = false;
//bool gbLastCheckWindow = false;
bool gbFarPanelsReady = false;
DWORD gnRgnDetectFlags = 0;
void CheckVarsInitialized();
//SECURITY_ATTRIBUTES* gpLocalSecurity = NULL;
// *** lng resources begin ***
wchar_t gsFolder[64], gsHardLink[64], gsSymLink[64], gsJunction[64], gsTitleThumbs[64], gsTitleTiles[64];
// *** lng resources end ***
CEFarPanelSettings gFarPanelSettings = {};
CEFarInterfaceSettings gFarInterfaceSettings = {};

//bool gbWaitForKeySequenceEnd = false;
DWORD gnWaitForKeySeqTick = 0;
//int gnUngetCount = 0;
//INPUT_RECORD girUnget[100];
WORD wScanCodeUp=0, wScanCodeDown=0; //p->Event.KeyEvent.wVirtualScanCode = MapVirtualKey(vk, 0/*MAPVK_VK_TO_VSC*/);
#ifdef _DEBUG
WORD wScanCodeLeft=0, wScanCodeRight=0, wScanCodePgUp=0, wScanCodePgDn=0;
#endif
//BOOL GetBufferInput(BOOL abRemove, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead);
//BOOL UngetBufferInput(WORD nCount, PINPUT_RECORD lpOneInput);
void ResetUngetBuffer();
BOOL ProcessConsoleInput(BOOL abReadMode, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead);
DWORD gnConsoleChanges = 0; // bitmask: 1-left, 2-right
bool  gbConsoleChangesSyncho = false;
void OnReadyForPanelsReload();
void ReloadResourcesW();

ConEmuThSynchroArg* gpLastSynchroArg = NULL;
bool gbSynchoRedrawPanelRequested = false;

//#ifdef _DEBUG
MFileMapping<DetectedDialogs> *gpDbgDlg = NULL;
//#endif


// minimal(?) FAR version 2.0 alpha build FAR_X_VER
int WINAPI _export GetMinFarVersionW(void)
{
	// ACTL_SYNCHRO required
	// build 1765: Новая команда в FARMACROCOMMAND - MCMD_GETAREA
	return MAKEFARVERSION(2,0,1765);
}

extern "C" void WINAPI _export GetPluginInfoWcmn(void *piv)
{
	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(GetPluginInfoW)(piv);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(GetPluginInfoW)(piv);
	else
		FUNC_X(GetPluginInfoW)(piv);
}


BOOL gbInfoW_OK = FALSE;
HANDLE OpenPluginWcmn(int OpenFrom,INT_PTR Item,bool FromMacro)
{
	HANDLE hResult = (gFarVersion.dwVerMajor >= 3) ? NULL : INVALID_HANDLE_VALUE;

	if (!gbInfoW_OK)
		return hResult;

	ReloadResourcesW();
	EntryPoint(OpenFrom, Item, FromMacro);

	return hResult;
}

// !!! WARNING !!! Version independent !!!
void EntryPoint(int OpenFrom,INT_PTR Item,bool FromMacro)
{
	if (!CheckConEmu())
		return;

	if (ghDisplayThread && gnDisplayThreadId == 0)
	{
		CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
	}

	//gThSet.Load();
	// При открытии плагина - загрузить информацию об обеих панелях. Нужно для определения регионов!
	ReloadPanelsInfo();
	// Получить активную
	CeFullPanelInfo* pi = GetActivePanel();

	if (!pi)
	{
		return;
	}

	pi->OurTopPanelItem = pi->TopPanelItem;
	HWND lhView = pi->hView; // (pi->bLeftPanel) ? ghLeftView : ghRightView;
	BOOL lbWasVisible = pi->Visible;
	PanelViewMode PVM = pvm_None;

	// В Far2 плагин можно позвать через callplugin(...)
	if (gFarVersion.dwVerMajor >= 2)
	{
		//if ((OpenFrom & OPEN_FROMMACRO) == OPEN_FROMMACRO)
		if (FromMacro)
		{
			if (Item == pvm_Thumbnails || Item == pvm_Tiles || Item == pvm_Icons || Item == pvm_TurnOff)
				PVM = (PanelViewMode)Item;
		}
	}

	// Вызов плагина из меню - нужно выбрать режим
	if (PVM == pvm_None)
	{
		switch (ShowPluginMenu())
		{
			case 0:
				PVM = pvm_Thumbnails;
				break;
			case 1:
				PVM = pvm_Tiles;
				break;
			case 2:
				PVM = pvm_TurnOff;
				break;
			case 3:
				PVM = pvm_Icons;
				break;
			default:
				// Отмена
				return;
		}
	}

	if (PVM == pvm_Icons)
	{
		if ((gFarVersion.dwVerMajor < 3) || (gFarVersion.dwBuild < 2579) || !gFarVersion.Bis)
		{
			ShowMessage(CEBisReqForIcons, 1);
			return;
		}
		TODO("CENoPlaceForIcons: Проверить чтобы было место для отображения иконок - 'M' mark в колонке 'N'");
	}

	BOOL lbRc = FALSE;
	DWORD dwErr = 0;
	DWORD dwMode = pvm_None; //PanelViewMode

	// Если View не создан, или смена режима
	if ((PVM != pvm_TurnOff) && ((lhView == NULL) || (!lbWasVisible && pi->Visible) || (PVM != pi->PVM)))
	{
		// Для корректного определения положения колонок необходим один из флажков в настройке панели:
		// [x] Показывать заголовки колонок [x] Показывать суммарную информацию
		if (!CheckPanelSettings(FALSE))
		{
			return;
		}

		pi->PVM = PVM;
		pi->DisplayReloadPanel();

		if (lhView == NULL)
		{
			// Нужно создать View
			lhView = pi->CreateView();
		}

		if (lhView == NULL)
		{
			// Показать ошибку
			ShowLastError();
		}
		else
		{
			// Зарегистрироваться
			_ASSERTE(pi->PVM==PVM);
			pi->RegisterPanelView();
			_ASSERTE(pi->PVM==PVM);
		}

		if (pi->hView)
			dwMode = pi->PVM;
	}
	else
	{
		// Отрегистрироваться
		pi->UnregisterPanelView();
		dwMode = pvm_None;
	}

	HKEY hk = NULL;

	SavePanelViewState(pi->bLeftPanel, dwMode);
	//SettingsSave(pi->bLeftPanel ? L"LeftPanelView" : L"RightPanelView", &dwMode);
	//if (!RegCreateKeyExW(HKEY_CURRENT_USER, gszRootKey, 0, NULL, 0, KEY_WRITE, NULL, &hk, NULL))
	//{
	//	RegSetValueEx(hk, pi->bLeftPanel ? L"LeftPanelView" : L"RightPanelView", 0,
	//	              REG_DWORD, (LPBYTE)&dwMode, sizeof(dwMode));
	//	RegCloseKey(hk);
	//}

	return;
}

// Плагин может быть вызван в первый раз из фоновой нити.
// Поэтому простой "gnMainThreadId = GetCurrentThreadId();" не прокатит. Нужно искать первую нить процесса!
DWORD GetMainThreadId()
{
	DWORD nThreadID = 0;
	DWORD nProcID = GetCurrentProcessId();
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (h != INVALID_HANDLE_VALUE)
	{
		THREADENTRY32 ti = {sizeof(THREADENTRY32)};

		if (Thread32First(h, &ti))
		{
			do
			{
				// Нужно найти ПЕРВУЮ нить процесса
				if (ti.th32OwnerProcessID == nProcID)
				{
					nThreadID = ti.th32ThreadID;
					break;
				}
			}
			while(Thread32Next(h, &ti));
		}

		CloseHandle(h);
	}

	// Нехорошо. Должна быть найдена. Вернем хоть что-то (текущую нить)
	if (!nThreadID)
	{
		_ASSERTE(nThreadID!=0);
		nThreadID = GetCurrentThreadId();
	}

	return nThreadID;
}


void WINAPI _export ExitFARW(void);
void WINAPI _export ExitFARW3(void*);
int WINAPI ProcessSynchroEventW(int Event, void *Param);
INT_PTR WINAPI ProcessSynchroEventW3(void*);

#include "../common/SetExport.h"
ExportFunc Far3Func[] =
{
	{"ExitFARW", reinterpret_cast<void*>(ExitFARW), reinterpret_cast<void*>(ExitFARW3)},
	{"ProcessSynchroEventW", reinterpret_cast<void*>(ProcessSynchroEventW), reinterpret_cast<void*>(ProcessSynchroEventW3)},
	{NULL}
};


BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			ghPluginModule = (HMODULE)hModule;
			ghWorkingModule = hModule;
			gnSelfPID = GetCurrentProcessId();
			gnMainThreadId = gnMainThreadIdInitial = GetMainThreadId();
			HeapInitialize();

			hkFunc.Init(WIN3264TEST(L"ConEmuTh.dll",L"ConEmuTh.x64.dll"), ghPluginModule);

			gfnSearchAppPaths = SearchAppPaths;
			gpLocalSecurity = LocalSecurity();

			_ASSERTE(FAR_X_VER<FAR_Y1_VER && FAR_Y1_VER<FAR_Y2_VER);
			#ifdef SHOW_STARTED_MSGBOX
			if (!IsDebuggerPresent()) MessageBoxA(NULL, "ConEmuTh*.dll loaded", "ConEmu Thumbnails", 0);
			#endif

			// Check Terminal mode
			TerminalMode = isTerminalMode();
			//TCHAR szVarValue[MAX_PATH];
			//szVarValue[0] = 0;
			//if (GetEnvironmentVariable(L"TERM", szVarValue, 63)) {
			//    TerminalMode = TRUE;
			//}
			bool lbExportsChanged = false;
			if (LoadFarVersion())
			{
				if (gFarVersion.dwVerMajor == 3)
				{
					lbExportsChanged = ChangeExports( Far3Func, ghPluginModule );
					if (!lbExportsChanged)
					{
						_ASSERTE(lbExportsChanged);
					}
				}
			}
		}
		break;
		case DLL_PROCESS_DETACH:
			CommonShutdown();
			HeapDeinitialize();
			break;
	}

	return TRUE;
}

#if defined(CRTSTARTUP)
extern "C" {
	BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
};

BOOL WINAPI _DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
#endif




BOOL LoadFarVersion()
{
	wchar_t ErrText[512]; ErrText[0] = 0;
	BOOL lbRc = LoadFarVersion(gFarVersion, ErrText);

	if (!lbRc)
	{
		gFarVersion.dwVerMajor = 2;
		gFarVersion.dwVerMinor = 0;
		gFarVersion.dwBuild = FAR_X_VER;
	}

	return lbRc;
}

#define CONEMUCHECKDELTA 2000
//static DWORD nLastCheckConEmu = 0;
BOOL CheckConEmu(BOOL abSilence/*=FALSE*/)
{
	// Функция проверяет доступность плагина conemu.dll и GUI
	// делать это нужно при каждом вызове, т.к. плагин могли и выгрузить...

	//// Страховка от того, что conemu.dll могли выгрузить (unload:...)
	//if (!abForceCheck) {
	//	if ((GetTickCount() - nLastCheckConEmu) > CONEMUCHECKDELTA) {
	//		if (!ghConEmuWnd)
	//			return FALSE;
	//		return TRUE;
	//	}
	//}
	if (TerminalMode)
	{
		if (!abSilence)
			ShowMessage(CEUnavailableInTerminal, 0);

		return FALSE;
	}

	//nLastCheckConEmu = GetTickCount();
	HMODULE hConEmu = GetModuleHandle
	                  (
#if defined(_M_ARM64) || defined(__aarch64__)
	                      L"ConEmu.arm64.dll"
#elif defined(WIN64)
	                      L"ConEmu.x64.dll"
#else
	                      L"ConEmu.dll"
#endif
	                  );

	if (!hConEmu)
	{
		gfRegisterPanelView = NULL;
		gfGetFarHWND2 = NULL;

		if (!abSilence)
			ShowMessage(CEPluginNotFound, 0);

		return FALSE;
	}

	gfRegisterPanelView = (RegisterPanelView_t)GetProcAddress(hConEmu, "RegisterPanelView");
	gfGetFarHWND2 = (GetFarHWND2_t)GetProcAddress(hConEmu, "GetFarHWND2");

	if (!gfRegisterPanelView || !gfGetFarHWND2)
	{
		if (!abSilence)
			ShowMessage(CEOldPluginVersion, 0);

		return FALSE;
	}

	HWND hWnd = gfGetFarHWND2(1);
	HWND hRoot = gfGetFarHWND2(2);

	if (hWnd != ghConEmuWnd)
	{
		ghConEmuWnd = hWnd;
		ghConEmuRoot = hRoot;

		if (hWnd)
		{
			//MFileMapping<PanelViewSetMapping> ThSetMap;
			//DWORD nGuiPID;
			//GetWindowThreadProcessId(ghConEmuWnd, &nGuiPID);
			//_ASSERTE(nGuiPID!=0);
			//ThSetMap.InitName(CECONVIEWSETNAME, nGuiPID);
			//if (!ThSetMap.Open()) {
			//	MessageBox(NULL, ThSetMap.GetErrorText(), L"ConEmuTh", MB_ICONSTOP|MB_SETFOREGROUND|MB_SYSTEMMODAL);
			//} else {
			//	ThSetMap.GetTo(&gThSet);
			//	ThSetMap.CloseMap();
			//}
			LoadThSet();
		}
	}

	if (!ghConEmuWnd)
	{
		if (!abSilence)
			ShowMessage(CEFarNonGuiMode, 0);

		return FALSE;
	}

	return TRUE;
}













void ReloadResourcesW()
{
	lstrcpynW(gsFolder, GetMsgW(CEDirFolder), countof(gsFolder)); //-V303
	//lstrcpynW(gsHardLink, GetMsgW(CEDirHardLink), countof(gsHardLink));
	lstrcpynW(gsSymLink, GetMsgW(CEDirSymLink), countof(gsSymLink)); //-V303
	lstrcpynW(gsJunction, GetMsgW(CEDirJunction), countof(gsJunction)); //-V303
	lstrcpynW(gsTitleThumbs, GetMsgW(CEColTitleThumbnails), countof(gsTitleThumbs)); //-V303
	lstrcpynW(gsTitleTiles, GetMsgW(CEColTitleTiles), countof(gsTitleTiles)); //-V303
}

void WINAPI _export SetStartupInfoW(void *aInfo)
{
	if (!gFarVersion.dwVerMajor) LoadFarVersion();

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(SetStartupInfoW)(aInfo);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(SetStartupInfoW)(aInfo);
	else
		FUNC_X(SetStartupInfoW)(aInfo);

	//_ASSERTE(gszRootKey!=NULL && *gszRootKey!=0);
	ReloadResourcesW();
	gbInfoW_OK = TRUE;
#if defined(_M_ARM64) || defined(__aarch64__)
	// Wait until Far has initialized its panels before restoring the view.
	if (gFarVersion.dwBuild >= FAR_Y2_VER)
		ExecuteInMainThread(SYNCHRO_RESTORE_PANELS);
	else
#endif
	StartPlugin(FALSE);
}

extern "C" HANDLE WINAPI _export OpenW(const struct OpenInfo *Info)
{
	HANDLE hResult = NULL;

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		hResult = FUNC_Y2(OpenW)((void*)Info);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		hResult = FUNC_Y1(OpenW)((void*)Info);
	else
	{
		_ASSERTE(FALSE && "Must not called in Far2");
	}

	return hResult;
}





void StartPlugin(BOOL abManual)
{
	// Это делаем всегда, потому как это общая точка входа в плагин
	if (!gpImgCache)
	{
		gpImgCache = new CImgCache(ghPluginModule);
	}

	if (!CheckConEmu(!abManual))
		return;

	HKEY hk = NULL;

	//if (!RegOpenKeyExW(HKEY_CURRENT_USER, gszRootKey, 0, KEY_READ, &hk))
	{
		//DWORD dwModes[2] = {0,0}; //, dwSize = 4;
		SettingsLoad(L"LeftPanelView", gdwModes);
		SettingsLoad(L"RightPanelView", gdwModes+1);

		//if (RegQueryValueEx(hk, L"LeftPanelView", NULL, NULL, (LPBYTE)dwModes, &(dwSize=4)))
		//	dwModes[0] = 0;

		//if (RegQueryValueEx(hk, L"RightPanelView", NULL, NULL, (LPBYTE)(dwModes+1), &(dwSize=4)))
		//	dwModes[1] = 0;

		//RegCloseKey(hk);

		if (ghDisplayThread && gnDisplayThreadId == 0)
		{
			CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
		}

		if (gdwModes[0] || gdwModes[1])
		{
			if (gThSet.bRestoreOnStartup)
			{
				// Для корректного определения положения колонок необходим один из флажков в настройке панели:
				// [x] Показывать заголовки колонок [x] Показывать суммарную информацию
				if (!CheckPanelSettings(!abManual))
				{
					return;
				}

				// При открытии плагина - загрузить информацию об обеих панелях. Нужно для определения регионов!
				ReloadPanelsInfo();
				CeFullPanelInfo* pi[2] = {&pviLeft, &pviRight};

				for(int i = 0; i < 2; i++)
				{
					if (!pi[i]->hView && gdwModes[i])
					{
						pi[i]->PVM = (PanelViewMode)gdwModes[i];
						pi[i]->DisplayReloadPanel();

						if (pi[i]->hView == NULL)
						{
							// Нужно создать View
							pi[i]->hView = pi[i]->CreateView();
						}

						if (pi[i]->hView == NULL)
						{
							// Показать ошибку
							ShowLastError();
						}
						else
						{
							pi[i]->RegisterPanelView();
						}
					}
				}
			}
		}
	}
}

void ExitPlugin(void)
{
	//if (ghLeftView)
	if (pviLeft.hView)
		pviLeft.UnregisterPanelView();

	//PostMessage(ghLeftView, WM_CLOSE,0,0);
	//if (ghRightView)
	if (pviRight.hView)
		pviRight.UnregisterPanelView();

	//PostMessage(ghRightView, WM_CLOSE,0,0);
	DWORD dwWait = 0;

	if (ghDisplayThread)
		dwWait = WaitForSingleObject(ghDisplayThread, 1000);

	if (dwWait)
		apiTerminateThread(ghDisplayThread, 100);

	if (ghDisplayThread)
	{
		CloseHandle(ghDisplayThread); ghDisplayThread = NULL;
	}

	gnDisplayThreadId = 0;

	// Освободить память
	if (gpRgnDetect)
	{
		delete gpRgnDetect;
		gpRgnDetect = NULL;
	}

	if (gpImgCache)
	{
		delete gpImgCache;
		gpImgCache = NULL;
	}

	// Сброс переменных, окон, и т.п.
	pviLeft.FinalRelease();
	pviRight.FinalRelease();

	if (gpLastSynchroArg)
	{
		LocalFree(gpLastSynchroArg); gpLastSynchroArg = NULL;
	}

	//if (gszRootKey)
	//{
	//	free(gszRootKey); gszRootKey = NULL;
	//}

	//#ifdef _DEBUG
	if (gpDbgDlg)
	{
		delete gpDbgDlg; gpDbgDlg = NULL;
	}
	//#endif
}

void   WINAPI _export ExitFARW(void)
{
	ExitPlugin();

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(ExitFARW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
}

void WINAPI _export ExitFARW3(void*)
{
	ExitPlugin();

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(ExitFARW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
}






int ShowMessage(int aiMsg, int aiButtons)
{
	if (gFarVersion.dwVerMajor==1)
		return ShowMessageA(aiMsg, aiButtons);
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(ShowMessageW)(aiMsg, aiButtons);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(ShowMessageW)(aiMsg, aiButtons);
	else
		return FUNC_X(ShowMessageW)(aiMsg, aiButtons);
}

LPCWSTR GetMsgW(int aiMsg)
{
	if (gFarVersion.dwVerMajor==1)
		return L"";
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(GetMsgW)(aiMsg);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(GetMsgW)(aiMsg);
	else
		return FUNC_X(GetMsgW)(aiMsg);
}

void PostMacro(wchar_t* asMacro)
{
	if (!asMacro || !*asMacro)
		return;

	if (gFarVersion.dwVerMajor==1)
	{
		int nLen = lstrlenW(asMacro); //-V303
		char* pszMacro = (char*)Alloc(nLen+1,1); //-V106

		if (pszMacro)
		{
			WideCharToMultiByte(CP_OEMCP,0,asMacro,nLen+1,pszMacro,nLen+1,0,0);
			PostMacroA(pszMacro);
			Free(pszMacro);
		}
	}
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
	{
		FUNC_Y2(PostMacroW)(asMacro);
	}
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
	{
		FUNC_Y1(PostMacroW)(asMacro);
	}
	else
	{
		FUNC_X(PostMacroW)(asMacro);
	}

	//FAR BUGBUG: Макрос не запускается на исполнение, пока мышкой не дернем :(
	//  Это чаще всего проявляется при вызове меню по RClick
	//  Если курсор на другой панели, то RClick сразу по пассивной
	//  не вызывает отрисовку :(
	//if (!mcr.Param.PlainText.Flags) {
	INPUT_RECORD ir[2] = {{MOUSE_EVENT},{MOUSE_EVENT}};

	if (isPressed(VK_CAPITAL))
		ir[0].Event.MouseEvent.dwControlKeyState |= CAPSLOCK_ON;

	if (isPressed(VK_NUMLOCK))
		ir[0].Event.MouseEvent.dwControlKeyState |= NUMLOCK_ON;

	if (isPressed(VK_SCROLL))
		ir[0].Event.MouseEvent.dwControlKeyState |= SCROLLLOCK_ON;

	ir[0].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	ir[1].Event.MouseEvent.dwControlKeyState = ir[0].Event.MouseEvent.dwControlKeyState;
	ir[1].Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
	ir[1].Event.MouseEvent.dwMousePosition.X = 1;
	ir[1].Event.MouseEvent.dwMousePosition.Y = 1;
	//2010-01-29 попробуем STD_OUTPUT
	//if (!ghConIn) {
	//	ghConIn  = CreateFile(L"CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_READ,
	//		0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	//	if (ghConIn == INVALID_HANDLE_VALUE) {
	//		#ifdef _DEBUG
	//		DWORD dwErr = GetLastError();
	//		_ASSERTE(ghConIn!=INVALID_HANDLE_VALUE);
	//		#endif
	//		ghConIn = NULL;
	//		return;
	//	}
	//}
	HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
	DWORD cbWritten = 0;
#ifdef _DEBUG
	BOOL fSuccess =
#endif
	    WriteConsoleInput(hIn/*ghConIn*/, ir, 1, &cbWritten);
	_ASSERTE(fSuccess && cbWritten==1);
	//}
	//InfoW995->AdvControl(InfoW995->ModuleNumber,ACTL_REDRAWALL,NULL);
}





int ShowPluginMenu()
{
	int nItem = -1;
	//if (!FarHwnd) {
	//	SHOWDBGINFO(L"*** ShowPluginMenu failed, FarHwnd is NULL\n");
	//	return;
	//}

	//if (nID != -1) {
	//	nItem = nID;
	//	if (nItem >= 2) nItem++; //Separator
	//	if (nItem >= 7) nItem++; //Separator
	//	if (nItem >= 9) nItem++; //Separator
	//	SHOWDBGINFO(L"*** ShowPluginMenu used default item\n");
	//} else
	if (gFarVersion.dwVerMajor==1)
	{
		SHOWDBGINFO(L"*** calling ShowPluginMenuA\n");
		nItem = ShowPluginMenuA();
	}
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
	{
		SHOWDBGINFO(L"*** calling ShowPluginMenuWY2\n");
		nItem = FUNC_Y2(ShowPluginMenuW)();
	}
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
	{
		SHOWDBGINFO(L"*** calling ShowPluginMenuWY1\n");
		nItem = FUNC_Y1(ShowPluginMenuW)();
	}
	else
	{
		SHOWDBGINFO(L"*** calling ShowPluginMenuWX\n");
		nItem = FUNC_X(ShowPluginMenuW)();
	}

	return nItem;
	//if (nItem < 0) {
	//	SHOWDBGINFO(L"*** ShowPluginMenu cancelled, nItem < 0\n");
	//	return;
	//}
	//#ifdef _DEBUG
	//wchar_t szInfo[128]; StringCchPrintf(szInfo, countof(szInfo), L"*** ShowPluginMenu done, nItem == %i\n", nItem);
	//SHOWDBGINFO(szInfo);
	//#endif
	//switch (nItem) {
	//	case 0: case 1:
	//	{ // Открыть в редакторе вывод последней консольной программы
	//		CESERVER_REQ* pIn = (CESERVER_REQ*)calloc(sizeof(CESERVER_REQ_HDR)+4,1);
	//		if (!pIn) return;
	//		CESERVER_REQ* pOut = NULL;
	//		ExecutePrepareCmd(pIn, CECMD_GETOUTPUTFILE, sizeof(CESERVER_REQ_HDR)+4);
	//		pIn->OutputFile.bUnicode = (gFarVersion.dwVerMajor>=2);
	//		pOut = ExecuteGuiCmd(FarHwnd, pIn, FarHwnd);
	//		if (pOut) {
	//			if (pOut->OutputFile.szFilePathName[0]) {
	//				BOOL lbRc = FALSE;
	//				if (gFarVersion.dwVerMajor==1)
	//					lbRc = EditOutputA(pOut->OutputFile.szFilePathName, (nItem==1));
	//				else if (gFarVersion.dwBuild>=FAR_Y_VER)
	//					lbRc = FUNC_Y(EditOutput)(pOut->OutputFile.szFilePathName, (nItem==1));
	//				else
	//					lbRc = FUNC_X(EditOutput)(pOut->OutputFile.szFilePathName, (nItem==1));
	//				if (!lbRc) {
	//					DeleteFile(pOut->OutputFile.szFilePathName);
	//				}
	//			}
	//			ExecuteFreeResult(pOut);
	//		}
	//		free(pIn);
	//	} break;
	//	case 3: // Показать/спрятать табы
	//	case 4: case 5: case 6:
	//	{
	//		CESERVER_REQ in, *pOut = NULL;
	//		ExecutePrepareCmd(&in, CECMD_TABSCMD, sizeof(CESERVER_REQ_HDR)+1);
	//		in.Data[0] = nItem - 3;
	//		pOut = ExecuteGuiCmd(FarHwnd, &in, FarHwnd);
	//		if (pOut) ExecuteFreeResult(pOut);
	//	} break;
	//	case 8: // Attach to GUI (если FAR был CtrlAltTab)
	//	{
	//		if (TerminalMode) break; // низзя
	//		if (ConEmuHwnd && IsWindow(ConEmuHwnd)) break; // Мы и так подключены?
	//		Attach2Gui();
	//	} break;
	//	case 10: // Start "ConEmuC.exe /DEBUGPID="
	//	{
	//		if (TerminalMode) break; // низзя
	//		StartDebugger();
	//	}
	//}
}




BOOL IsMacroActive()
{
	BOOL lbActive = FALSE;

	if (gFarVersion.dwVerMajor==1)
		lbActive = IsMacroActiveA();
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		lbActive = FUNC_Y2(IsMacroActiveW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		lbActive = FUNC_Y1(IsMacroActiveW)();
	else
		lbActive = FUNC_X(IsMacroActiveW)();

	return lbActive;
}

int GetMacroArea()
{
	int nMacroArea = 0/*MACROAREA_OTHER*/;

	if (gFarVersion.dwVerMajor==1)
	{
		_ASSERTE(gFarVersion.dwVerMajor>1);
		nMacroArea = 1; // в Far 1.7x не поддерживается
	}
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		nMacroArea = FUNC_Y2(GetMacroAreaW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		nMacroArea = FUNC_Y1(GetMacroAreaW)();
	else
		nMacroArea = FUNC_X(GetMacroAreaW)();

	return nMacroArea;
}

BOOL CheckPanelSettings(BOOL abSilence)
{
	BOOL lbOk = FALSE;

	if (gFarVersion.dwVerMajor==1)
		lbOk = CheckPanelSettingsA(abSilence);
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		lbOk = FUNC_Y2(CheckPanelSettingsW)(abSilence);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		lbOk = FUNC_Y1(CheckPanelSettingsW)(abSilence);
	else
		lbOk = FUNC_X(CheckPanelSettingsW)(abSilence);

	return lbOk;
}

void LoadPanelItemInfo(CeFullPanelInfo* pi, INT_PTR nItem)
{
	if (gFarVersion.dwVerMajor==1)
		LoadPanelItemInfoA(pi, nItem);
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(LoadPanelItemInfoW)(pi, nItem);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(LoadPanelItemInfoW)(pi, nItem);
	else
		FUNC_X(LoadPanelItemInfoW)(pi, nItem);
}


// Возвращает (для удобства) ссылку на одну из глобальных переменных (pviLeft/pviRight)
CeFullPanelInfo* GetActivePanel()
{
	if (pviLeft.Visible && pviLeft.Focus && pviLeft.IsFilePanel)
		return &pviLeft;

	if (pviRight.Visible && pviRight.Focus && pviRight.IsFilePanel)
		return &pviRight;

	return NULL;
}
//CeFullPanelInfo* LoadPanelInfo(BOOL abActive)
//{
//	TODO("Добавить вызов ACTL_GETWINDOW что-ли?");
//
//	CheckVarsInitialized();
//
//	CeFullPanelInfo* ppi = NULL;
//
//	if (gFarVersion.dwVerMajor==1)
//		ppi = LoadPanelInfoA(abActive);
//	else if (gFarVersion.dwBuild>=FAR_Y_VER)
//		ppi = FUNC_Y(LoadPanelInfo)(abActive);
//	else
//		ppi = FUNC_X(LoadPanelInfo)(abActive);
//
//	if (ppi) {
//		ppi->cbSize = sizeof(*ppi);
//
//		int n = std::min(ppi->nMaxFarColors, countof(gFarInfo.nFarColors));
//		if (n && ppi->nFarColors) memmove(gFarInfo.nFarColors, ppi->nFarColors, n);
//		gFarInfo.nFarInterfaceSettings = ppi->nFarInterfaceSettings;
//		gFarInfo.nFarPanelSettings = ppi->nFarPanelSettings;
//		gFarInfo.bFarPanelAllowed = TRUE;
//	}
//
//	return ppi;
//}

void ReloadPanelsInfo()
{
	TODO("Добавить вызов ACTL_GETWINDOW что-ли?");
	// Хотя уже и должен быть создан
	CheckVarsInitialized();
	// Если меняется прямоугольник панели - нужно повторно зарегистрироваться в GUI
	RECT rcLeft = pviLeft.PanelRect;
	BOOL bLeftVisible = pviLeft.Visible;
	RECT rcRight = pviRight.PanelRect;
	BOOL bRightVisible = pviRight.Visible;

	if (gFarVersion.dwVerMajor==1)
		ReloadPanelsInfoA();
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(ReloadPanelsInfoW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(ReloadPanelsInfoW)();
	else
		FUNC_X(ReloadPanelsInfoW)();

	// Обновить gFarInfo (используется в RgnDetect)
	CeFullPanelInfo* p = pviLeft.hView ? &pviLeft : &pviRight;
	_ASSERTE(countof(p->nFarColors)==countof(gFarInfo.nFarColors) && sizeof(*p->nFarColors) == sizeof(*gFarInfo.nFarColors));
	memmove(gFarInfo.nFarColors, p->nFarColors, sizeof(gFarInfo.nFarColors));

	gFarInfo.FarInterfaceSettings.Raw = p->FarInterfaceSettings.Raw;
	gFarInfo.FarPanelSettings.Raw = p->FarPanelSettings.Raw;
	gFarInfo.bFarPanelAllowed = TRUE;
	// Положения панелей
	gFarInfo.bFarLeftPanel = pviLeft.Visible;
	gFarInfo.FarLeftPanel.PanelRect = pviLeft.PanelRect;
	gFarInfo.bFarRightPanel = pviRight.Visible;
	gFarInfo.FarRightPanel.PanelRect = pviRight.PanelRect;

	if (pviLeft.hView)
	{
		if ((bLeftVisible != pviLeft.Visible)
			|| memcmp(&rcLeft, &pviLeft.PanelRect, sizeof(RECT)))
		{
			pviLeft.RegisterPanelView();
		}
	}

	if (pviRight.hView)
	{
		if ((bRightVisible != pviRight.Visible)
			|| memcmp(&rcRight, &pviRight.PanelRect, sizeof(RECT)))
		{
			pviRight.RegisterPanelView();
		}
	}
}


//BOOL IsLeftPanelActive()
//{
//	BOOL lbLeftActive = FALSE;
//	if (gFarVersion.dwVerMajor==1)
//		lbLeftActive = IsLeftPanelActiveA();
//	else if (gFarVersion.dwBuild>=FAR_Y_VER)
//		lbLeftActive = FUNC_Y(IsLeftPanelActive)();
//	else
//		lbLeftActive = FUNC_X(IsLeftPanelActive)();
//	return lbLeftActive;
//}

//WORD PopUngetBuffer(BOOL abRemove, PINPUT_RECORD lpDst)
//{
//	if (gnUngetCount<1) {
//		lpDst->EventType = 0;
//	} else {
//		*lpDst = girUnget[0];
//		BOOL lbNeedPop = FALSE;
//		if (girUnget[0].EventType == EVENT_TYPE_REDRAW)
//			abRemove = TRUE; // Это - сразу удаляем из буфера
//
//		if (girUnget[0].EventType == KEY_EVENT) {
//			_ASSERTE(((short)girUnget[0].Event.KeyEvent.wRepeatCount) > 0);
//			lpDst->Event.KeyEvent.wRepeatCount = 1;
//
//			if (girUnget[0].Event.KeyEvent.wRepeatCount == 1) {
//				lbNeedPop = abRemove;
//			} else if (abRemove) {
//				girUnget[0].Event.KeyEvent.wRepeatCount --;
//			}
//		} else {
//			lbNeedPop = abRemove;
//		}
//
//		if (lbNeedPop) {
//			_ASSERTE(abRemove);
//			gnUngetCount --;
//			_ASSERTE(gnUngetCount >= 0);
//			if (gnUngetCount > 0) {
//				// Подвинуть в буфере то что осталось к началу
//				memmove(girUnget, girUnget+1, sizeof(girUnget[0])*gnUngetCount);
//			}
//			girUnget[gnUngetCount].EventType = 0;
//		}
//	}
//
//	return lpDst->EventType;
//}

//BOOL GetBufferInput(BOOL abRemove, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead)
//{
//	if (gnUngetCount<0)
//	{
//		_ASSERTE(gnUngetCount>=0);
//		gnUngetCount = 0;
//		*lpNumberOfEventsRead = 0;
//		return FALSE;
//	}
//	if (nBufSize < 1)
//	{
//		_ASSERTE(nBufSize>=1);
//		*lpNumberOfEventsRead = 0;
//		return FALSE;
//	}
//
//	BOOL lbRedraw = FALSE, lbSetEnvVar = FALSE;
//	WORD wType = 0;
//	PINPUT_RECORD p = lpBuffer;
//
//	while (nBufSize && gnUngetCount)
//	{
//		if ((wType = PopUngetBuffer(abRemove, p)) == 0)
//			break; // буфер кончился
//		if (wType == EVENT_TYPE_REDRAW)
//		{
//			lbRedraw = TRUE;
//		}
//		else
//		{
//			if (abRemove && gnUngetCount && girUnget[0].EventType == EVENT_TYPE_REDRAW)
//			{
//				lbSetEnvVar = TRUE;
//			}
//			nBufSize--; p++;
//			if (!abRemove) break; // В режиме Peek - возвращаем не более одного нажатияs
//		}
//	}
//
//	//// Если натыкаемся на событие EVENT_TYPE_REDRAW - сбрасывать переменную EnvVar
//	//// SetEnvironmentVariable(TH_ENVVAR_NAME, TH_ENVVAR_ACTIVE);
//	//if (girUnget[0].EventType == EVENT_TYPE_REDRAW)
//	//{
//	//	lbRedraw = TRUE;
//	//	gnUngetCount --;
//	//	_ASSERTE(gnUngetCount >= 0);
//	//	if (gnUngetCount > 0) {
//	//		// Подвинуть в буфере то что осталось к началу
//	//		memmove(girUnget, girUnget+1, sizeof(girUnget[0])*gnUngetCount);
//	//	}
//	//	girUnget[gnUngetCount].EventType = 0;
//	//}
//
//	//int nMax = countof(girUnget);
//	//if (gnUngetCount>=nMax) {
//	//	_ASSERTE(gnUngetCount==nMax);
//	//	if (gnUngetCount>nMax) gnUngetCount = nMax;
//	//}
//
//	//nMax = std::min(gnUngetCount,(int)nBufSize);
//
//	//int i = 0, j = 0;
//	//while (i < nMax && j < gnUngetCount) {
//	//	if (girUnget[j].EventType == EVENT_TYPE_REDRAW) {
//	//		j++; lbRedraw = TRUE; continue;
//	//	}
//
//	//	lpBuffer[i++] = girUnget[j++];
//	//}
//	//nMax = i;
//
//	//if (abRemove) {
//	//	gnUngetCount -= j;
//	//	_ASSERTE(gnUngetCount >= 0);
//	//	if (gnUngetCount > 0) {
//	//		// Подвинуть в буфере то что осталось к началу
//	//		memmove(girUnget, girUnget+nMax, sizeof(girUnget[0])*gnUngetCount);
//	//		girUnget[gnUngetCount].EventType = 0;
//	//	}
//	//}
//
//	if (lbRedraw || lbSetEnvVar)
//	{
//		gbWaitForKeySequenceEnd = (gnUngetCount > 0) && !lbSetEnvVar;
//		UpdateEnvVar(TRUE);
//
//		if (IsThumbnailsActive(FALSE))
//		{
//			if (lbRedraw && !gbWaitForKeySequenceEnd)
//				OnReadyForPanelsReload();
//		}
//	}
//
//	*lpNumberOfEventsRead = (DWORD)(p - lpBuffer);
//
//	return TRUE;
//}

//BOOL UngetBufferInput(WORD nCount, PINPUT_RECORD lpOneInput)
//{
//	_ASSERTE(nCount);
//	if (gnUngetCount<0) {
//		_ASSERTE(gnUngetCount>=0);
//		gnUngetCount = 0;
//	}
//	int nMax = countof(girUnget);
//	if (gnUngetCount>=nMax) {
//		_ASSERTE(gnUngetCount==nMax);
//		if (gnUngetCount>nMax) gnUngetCount = nMax;
//		return FALSE;
//	}
//
//	girUnget[gnUngetCount] = *lpOneInput;
//	if (lpOneInput->EventType == KEY_EVENT) {
//		girUnget[gnUngetCount].Event.KeyEvent.wRepeatCount = nCount;
//	}
//	gnUngetCount ++;
//
//	return TRUE;
//}

void ResetUngetBuffer()
{
	gnConsoleChanges = 0;
	gbConsoleChangesSyncho = false;
	//gnUngetCount = 0;
	//gbWaitForKeySequenceEnd = false;
}

#if defined(_M_ARM64) || defined(__aarch64__)
void RefreshPanelViewsAfterInput()
{
	if (!pviLeft.hView && !pviRight.hView)
		return;

	gnConsoleChanges |= 3;
	if (!gbConsoleChangesSyncho)
	{
		gbConsoleChangesSyncho = true;
		ExecuteInMainThread(SYNCHRO_RELOAD_PANELS);
	}
}
#endif

// Должен активироваться или через Synchro или через PeekConsoleInput в FAR1
void OnReadyForPanelsReload()
{
	if (!gnConsoleChanges)
		return;

	gbConsoleChangesSyncho = false;
	DWORD nCurChanges = gnConsoleChanges; gnConsoleChanges = 0;
	// Сбросим, чтобы RgnDetect попытался сам найти панели и диалоги.
	// Это нужно чтобы избежать возможных блокировок фара
	gFarInfo.bFarPanelInfoFilled = gFarInfo.bFarLeftPanel = gFarInfo.bFarRightPanel = FALSE;
	bool bFarUserscreen = (isPressed(VK_CONTROL) && isPressed(VK_SHIFT) && isPressed(VK_MENU));
	gpRgnDetect->PrepareTransparent(&gFarInfo, gcrCurColors, bFarUserscreen);
	gnRgnDetectFlags = gpRgnDetect->GetFlags();
	
	//#ifdef _DEBUG
	if (!gpDbgDlg)
	{
		gpDbgDlg = new MFileMapping<DetectedDialogs>();
		gpDbgDlg->InitName(CEPANELDLGMAPNAME, GetCurrentProcessId());
		gpDbgDlg->Create();
	}

	gpDbgDlg->SetFrom(gpRgnDetect->GetDetectedDialogsPtr());
	//#endif

	WARNING("Если панели скрыты (активен редактор/вьювер) - не пытаться считывать панели");

	if (!CheckWindows())
	{
		// Спрятать/разрегистрировать?
	}
	else
	{
		//if (pviLeft.hView || pviRight.hView) {
		//	ReloadPanelsInfo();
		//	/* После реального получения панелей - можно повторно "обнаружить диалоги"? */
		//	CeFullPanelInfo* p = pviLeft.hView ? &pviLeft : &pviRight;
		//	gFarInfo.bFarPanelInfoFilled = TRUE;
		//	gFarInfo.bFarLeftPanel = (pviLeft.Visible!=0);
		//	gFarInfo.FarLeftPanel.PanelRect = pviLeft.PanelRect;
		//	gFarInfo.bFarRightPanel = (pviRight.Visible!=0);
		//	gFarInfo.FarRightPanel.PanelRect = pviRight.PanelRect;
		//	gpRgnDetect->PrepareTransparent(&gFarInfo, gcrColors);
		//}
		//if (!gbWaitForKeySequenceEnd)
		//|| girUnget[0].EventType == EVENT_TYPE_REDRAW)
		{
			if (pviLeft.hView || pviRight.hView)
			{
				ReloadPanelsInfo();
			}

			if (pviLeft.hView)
			{
				pviLeft.DisplayReloadPanel();
				//Inva lidateRect(pviLeft.hView, NULL, FALSE);
				pviLeft.Invalidate();
			}

			if (pviRight.hView)
			{
				pviRight.DisplayReloadPanel();
				//Inva lidateRect(pviRight.hView, NULL, FALSE);
				pviRight.Invalidate();
			}
		}
	}

	//WARNING("Проверить список окон FAR и если активное НЕ панели - сразу выйти (и спрятать разрегистрировать)");
	//WARNING("Проверить и сравнить, если изменился регион панели - обновить PanelView");
	//!. Сначала проверить на какой панели находится фокус!
	//   Это требуется для того, чтобы НЕактивная панель не перехватывала стрелки
	//Далее
	//1. Панель видима -> считать информацию о прямоугольнике, элементах панели, и выполнить отрисовку
	//   информацию о прямоугольнике (если он изменился) передать в GUI
	//   Если окошко панели невидимо - выполнить повторную регистрацию в GUI - оно само сделает apiShowWindow
	//2. Панель невидима -> спрятать окошко панели и разрегистрироваться в GUI
}

BOOL WINAPI OnPrePeekConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult)
{
	// Только для FAR1, в FAR2 - через Synchro!
	if (gFarVersion.dwVerMajor == 1)
	{
		// Выполняем только если фар считывает единственное событие (аналог Synchro в Far2)
		if (nBufSize == 1)
		{
			if (gbSynchoRedrawPanelRequested)
			{
				ProcessSynchroEventW(SE_COMMONSYNCHRO, SYNCHRO_REDRAW_PANEL);
			}
			else if (gnConsoleChanges && !gbConsoleChangesSyncho)
			{
				gbConsoleChangesSyncho = true;
			}
			else if (gbConsoleChangesSyncho)
			{
				OnReadyForPanelsReload();
			}

			// Отдельно стоящая - выполнение макроса
			if (gpLastSynchroArg)
			{
				ProcessSynchroEventW(SE_COMMONSYNCHRO, gpLastSynchroArg);
			}
		}
	}
	else
	{
		if (gnConsoleChanges && !gbConsoleChangesSyncho)
		{
			gbConsoleChangesSyncho = true;
			ExecuteInMainThread(SYNCHRO_RELOAD_PANELS);
		}
	}

	if (!lpBuffer || !lpNumberOfEventsRead)
	{
		*pbResult = FALSE;
		return FALSE; // ошибка в параметрах
	}

	//// если в girUnget что-то есть вернуть из него и FALSE (тогда OnPostPeekConsole не будет вызван)");
	//if (gnUngetCount)
	//{
	//	if (GetBufferInput(FALSE/*abRemove*/, lpBuffer, nBufSize, lpNumberOfEventsRead))
	//		return FALSE; // PeekConsoleInput & OnPostPeekConsole не будет вызван
	//}
	//// Для FAR1 - эмуляция ACTL_SYNCHRO
	//if ((gpLastSynchroArg || gbSynchoRedrawPanelRequested) // ожидает команда
	//	&& nBufSize == 1  // только когда размер буфера == 1 - считается что ФАР готов
	//	&& gFarVersion.dwVerMajor==1) // FAR1
	//{
	//	if (gbSynchoRedrawPanelRequested)
	//		ProcessSynchroEventW(SE_COMMONSYNCHRO, SYNCHRO_REDRAW_PANEL);
	//	if (gpLastSynchroArg)
	//		ProcessSynchroEventW(SE_COMMONSYNCHRO, gpLastSynchroArg);
	//}
	return TRUE; // продолжить без изменений
}

BOOL WINAPI OnPostPeekConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult)
{
	if (!lpBuffer || !lpNumberOfEventsRead)
	{
		*pbResult = FALSE;
		return FALSE; // ошибка в параметрах
	}

	// заменить обработку стрелок, но girUnget не трогать
	if (*lpNumberOfEventsRead)
	{
		if (ProcessConsoleInput(FALSE/*abReadMode*/, lpBuffer, nBufSize, lpNumberOfEventsRead))
		{
			if (*lpNumberOfEventsRead == 0)
			{
				*pbResult = FALSE;
			}

			return FALSE; // вернуться в вызывающую функцию
		}
	}

	return TRUE; // продолжить без изменений
}

BOOL WINAPI OnPreReadConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult)
{
	if (!lpBuffer || !lpNumberOfEventsRead)
	{
		*pbResult = FALSE;
		return FALSE; // ошибка в параметрах
	}

	//// если в girUnget что-то есть вернуть из него и FALSE (тогда OnPostPeekConsole не будет вызван)");
	//if (gnUngetCount)
	//{
	//	if (GetBufferInput(TRUE/*abRemove*/, lpBuffer, nBufSize, lpNumberOfEventsRead))
	//		return FALSE; // ReadConsoleInput & OnPostReadConsole не будет вызван
	//}
	return TRUE; // продолжить без изменений
}

BOOL WINAPI OnPostReadConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult)
{
	if (!lpBuffer || !lpNumberOfEventsRead)
	{
		*pbResult = FALSE;
		return FALSE; // ошибка в параметрах
	}

	// заменить обработку стрелок, если надо - поместить серию нажатий в girUnget
	if (*lpNumberOfEventsRead)
	{
		if (ProcessConsoleInput(TRUE/*abReadMode*/, lpBuffer, nBufSize, lpNumberOfEventsRead))
		{
			if (*lpNumberOfEventsRead == 0)
			{
				*pbResult = FALSE;
			}

			return FALSE; // вернуться в вызывающую функцию
		}
	}

	return TRUE; // продолжить без изменений
}




//lpBuffer
//The data to be written to the console screen buffer. This pointer is treated as the origin of a
//two-dimensional array of CHAR_INFO structures whose size is specified by the dwBufferSize parameter.
//The total size of the array must be less than 64K.
//
//dwBufferSize
//The size of the buffer pointed to by the lpBuffer parameter, in character cells.
//The X member of the COORD structure is the number of columns; the Y member is the number of rows.
//
//dwBufferCoord
//The coordinates of the upper-left cell in the buffer pointed to by the lpBuffer parameter.
//The X member of the COORD structure is the column, and the Y member is the row.
//
//lpWriteRegion
//A pointer to a SMALL_RECT structure. On input, the structure members specify the upper-left and lower-right
//coordinates of the console screen buffer rectangle to write to.
//On output, the structure members specify the actual rectangle that was used.
BOOL WINAPI OnPreWriteConsoleOutput(HANDLE hOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion)
{
	WARNING("После повторного отображения view - хорошо бы сначала полностью считать gpRgnDetect из консоли");

	if (gpRgnDetect && lpBuffer && lpWriteRegion)
	{
#ifdef SHOW_WRITING_RECTS

		if (IsDebuggerPresent())
		{
			wchar_t szDbg[80]; StringCchPrintf(szDbg, countof(szDbg), L"ConEmuTh.OnPreWriteConsoleOutput( {%ix%i} - {%ix%i} )\n",
			                                   lpWriteRegion->Left, lpWriteRegion->Top, lpWriteRegion->Right, lpWriteRegion->Bottom);
			OutputDebugStringW(szDbg);
		}

#endif
		SMALL_RECT rcFarRect; GetFarRect(&rcFarRect);
		gpRgnDetect->SetFarRect(&rcFarRect);
		gpRgnDetect->OnWriteConsoleOutput(lpBuffer, dwBufferSize, dwBufferCoord, lpWriteRegion, gcrCurColors);
		WARNING("Перед установкой флагов измененности - сначала хорошо бы проверить, а менялась ли сама панель?");
		DWORD nChanges = 0;
		RECT rcTest = {0};
		RECT rcWrite =
		{
			lpWriteRegion->Left-rcFarRect.Left,lpWriteRegion->Top-rcFarRect.Top,
			lpWriteRegion->Right+1-rcFarRect.Left,lpWriteRegion->Bottom+1-rcFarRect.Top
		};

		if (pviLeft.hView)
		{
			WARNING("Проверить в far/w");

			WARNING("IntersectRect не работает, если низ совпадает?");
			RECT rcPanel = pviLeft.WorkRect;
			rcPanel.bottom++;
			if (IntersectRect(&rcTest, &rcPanel, &rcWrite))
			{
				nChanges |= 1; // 1-left, 2-right.
			}
		}

		if (pviRight.hView)
		{
			WARNING("Проверить в far/w");

			WARNING("IntersectRect не работает, если низ совпадает?");
			RECT rcPanel = pviRight.WorkRect;
			rcPanel.bottom++;
			if (IntersectRect(&rcTest, &rcPanel, &rcWrite))
			{
				nChanges |= 2; // 1-left, 2-right.
			}
		}

		//if (pviLeft.hView || pviRight.hView)
		if (nChanges)
		{
			//bool lbNeedSynchro = (gn ConsoleChanges == 0 && gFarVersion.dwVerMajor >= 2);
			// 1-left, 2-right. Но пока - ставим все, т.к. еще не проверяется что собственно изменилось!
			//gnConsoleChanges |= 3;
			gnConsoleChanges |= nChanges;
			//if (lbNeedSynchro)
			//{
			//	ExecuteInMainThread(SYNCHRO_RELOAD_PANELS);
			//}
		}
	}

	return TRUE; // Продолжить без изменений
}


//// Вернуть TRUE, если был Unget
//BOOL ProcessKeyPress(CeFullPanelInfo* pi, BOOL abReadMode, PINPUT_RECORD lpBuffer)
//{
//	_ASSERTE(lpBuffer->EventType == KEY_EVENT);
//
//	// Перехватываемые клавиши
//	WORD vk = lpBuffer->Event.KeyEvent.wVirtualKeyCode;
//	BOOL lbWasUnget = FALSE;
//
//	if (vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT)
//	{
//		if (!lpBuffer->Event.KeyEvent.bKeyDown) {
//			switch (vk) {
//				case VK_LEFT:  lpBuffer->Event.KeyEvent.wVirtualKeyCode = VK_UP;   break;
//				case VK_RIGHT: lpBuffer->Event.KeyEvent.wVirtualKeyCode = VK_DOWN; break;
//			}
//
//		} else {
//			// Переработать
//			int n = 0;
//			switch (vk) {
//				case VK_UP: {
//					n = std::min(pi->CurrentItem,pi->nXCount);
//				} break;
//				case VK_DOWN: {
//					n = std::min((pi->ItemsNumber-pi->CurrentItem-1),pi->nXCount);
//				} break;
//				case VK_LEFT: {
//					lpBuffer->Event.KeyEvent.wVirtualKeyCode = VK_UP;
//				} break;
//				case VK_RIGHT: {
//					lpBuffer->Event.KeyEvent.wVirtualKeyCode = VK_DOWN;
//				} break;
//			}
//
//			lbWasUnget = (n > 1);
//			while ((n--) > 1) {
//				UngetBufferInput(lpBuffer);
//			}
//		}
//	}
//	return lbWasUnget;
//}

// Выполнить замену клавиш и при необходимости запросить SYNCHRO_REDRAW_PANEL
// Перехватываются ВСЕ VK_LEFT/RIGHT/UP/DOWN/PgUp/PgDn/WHEEL_UP/WHEEL_DOWN
// иначе, если их обработает сам Фар - то собъется TopPanelItem!
// Вернуть TRUE, если замены были произведены
BOOL ProcessConsoleInput(BOOL abReadMode, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead)
{
	static bool sbInClearing = false;

	if (sbInClearing)
	{
		// Если это наше считывание буфера, чтобы убрать из него обработанные нажатия
		return FALSE;
	}

	// Обрабатываем только одиночные нажатия (фар дергает по одному событию из буфера ввода!)
	if (nBufSize != 1 || !lpNumberOfEventsRead || *lpNumberOfEventsRead != 1)
	{
		return FALSE;
	}

	// Перехват управления курсором
	CeFullPanelInfo* pi = IsThumbnailsActive(TRUE/*abFocusRequired*/);

	if (!pi)
	{
		return FALSE; // панель создана, но она не активна или скрыта диалогом или погашена фаровская панель
	}

	if (!wScanCodeUp)
	{
		wScanCodeUp = MapVirtualKey(VK_UP, 0/*MAPVK_VK_TO_VSC*/);
		wScanCodeDown = MapVirtualKey(VK_DOWN, 0/*MAPVK_VK_TO_VSC*/);
#ifdef _DEBUG
		wScanCodeLeft = MapVirtualKey(VK_LEFT, 0/*MAPVK_VK_TO_VSC*/);
		wScanCodeRight = MapVirtualKey(VK_RIGHT, 0/*MAPVK_VK_TO_VSC*/);
		wScanCodePgUp = MapVirtualKey(VK_PRIOR, 0/*MAPVK_VK_TO_VSC*/);
		wScanCodePgDn = MapVirtualKey(VK_NEXT, 0/*MAPVK_VK_TO_VSC*/);
#endif
	}

	WARNING("Проверять один из DWORD-ов окна на предмет наличия диалогов");
	WARNING("Ставить его должен GUI");
	WARNING("Если есть хотя бы один диалог - значит перехватывать нельзя");
	PanelViewMode PVM = pi->PVM;
	BOOL lbWasChanges = FALSE;
	INT_PTR iCurItem, iTopItem, iShift = 0;

	if (pi->bRequestItemSet)
	{
		iCurItem = pi->ReqCurrentItem; iTopItem = pi->ReqTopPanelItem;
	}
	else
	{
		iCurItem = pi->CurrentItem; iTopItem = std::max(pi->TopPanelItem,pi->ReqTopPanelItem);
	}

	// Пойдем в два прохода. В первом - обработка замен, и помещение в буфер Unget.
	PINPUT_RECORD p = lpBuffer;
	//PINPUT_RECORD pEnd = lpBuffer+*lpNumberOfEventsRead;
	//PINPUT_RECORD pReplace = lpBuffer;
	//while (p != pEnd)
	{
		bool bEraseEvent = false;

		if (p->EventType == KEY_EVENT)
		{
			INT_PTR iCurKeyShift = 0;
			// Перехватываемые клавиши
			WORD vk = p->Event.KeyEvent.wVirtualKeyCode;

			if (vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT
			        || vk == VK_PRIOR || vk == VK_NEXT)
			{
				if (!(p->Event.KeyEvent.dwControlKeyState
				        & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED|LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED|SHIFT_PRESSED)))
				{
					// Переработать
					int n = 1;

					switch(vk)
					{
						case VK_UP:
						{
							//if (PVM == pvm_Thumbnails)
							//	n = std::min(pi->CurrentItem,pi->nXCountFull);
							DEBUGSTRCTRL(L"ProcessConsoleInput(VK_UP)\n");

							if (PVM == pvm_Thumbnails)
								iCurKeyShift = -pi->nXCountFull; //-V101
							else
								iCurKeyShift = -1;
						} break;
						case VK_DOWN:
						{
							//if (PVM == pvm_Thumbnails)
							//	n = std::min((pi->ItemsNumber-pi->CurrentItem-1),pi->nXCountFull);
							DEBUGSTRCTRL(L"ProcessConsoleInput(VK_DOWN)\n");

							if (PVM == pvm_Thumbnails)
								iCurKeyShift = pi->nXCountFull; //-V101
							else
								iCurKeyShift = 1;
						} break;
						case VK_LEFT:
						{
							//p->Event.KeyEvent.wVirtualKeyCode = VK_UP;
							//p->Event.KeyEvent.wVirtualScanCode = wScanCodeUp;
							//if (PVM != pvm_Thumbnails)
							//	n = std::min(pi->CurrentItem,pi->nYCountFull);
							DEBUGSTRCTRL(L"ProcessConsoleInput(VK_LEFT)\n");

							if (PVM != pvm_Thumbnails)
								iCurKeyShift = -pi->nYCountFull; //-V101
							else
								iCurKeyShift = -1;
						} break;
						case VK_RIGHT:
						{
							//p->Event.KeyEvent.wVirtualKeyCode = VK_DOWN;
							//p->Event.KeyEvent.wVirtualScanCode = wScanCodeDown;
							//if (PVM != pvm_Thumbnails)
							//	n = std::min((pi->ItemsNumber-pi->CurrentItem-1),pi->nYCountFull);
							DEBUGSTRCTRL(L"ProcessConsoleInput(VK_RIGHT)\n");

							if (PVM != pvm_Thumbnails)
								iCurKeyShift = pi->nYCountFull; //-V101
							else
								iCurKeyShift = 1;
						} break;
						case VK_PRIOR:
						{
							//p->Event.KeyEvent.wVirtualKeyCode = VK_UP;
							//p->Event.KeyEvent.wVirtualScanCode = wScanCodeUp;
							//n = std::min(pi->CurrentItem,pi->nXCountFull*pi->nYCountFull);
							DEBUGSTRCTRL(L"ProcessConsoleInput(VK_PRIOR)\n");
							int nRowCol = (PVM == pvm_Thumbnails) ? pi->nXCountFull : pi->nYCountFull;

							if (iCurItem >= (iTopItem + nRowCol)) //-V104
							{
								int nCorrection = (PVM == pvm_Thumbnails) //-V103
								                  ? (iCurItem % nRowCol) //-V104
								                  : 0;
								// Если PgUp нажат когда текущий элемент НЕ на верхней строке
								iCurKeyShift = (iTopItem - iCurItem);
							}
							else
							{
								iCurKeyShift = -(pi->nXCountFull*pi->nYCountFull); //-V101
							}
						} break;
						case VK_NEXT:
						{
							//p->Event.KeyEvent.wVirtualKeyCode = VK_DOWN;
							//p->Event.KeyEvent.wVirtualScanCode = wScanCodeUp;
							//n = std::min((pi->ItemsNumber-pi->CurrentItem-1),pi->nXCountFull*pi->nYCountFull);
							DEBUGSTRCTRL(L"ProcessConsoleInput(VK_NEXT)\n");
							int nRowCol = (PVM == pvm_Thumbnails) ? pi->nXCountFull : pi->nYCountFull;
							int nFull = (pi->nXCountFull*pi->nYCountFull);

							if (iCurItem >= iTopItem && iCurItem < (iTopItem + nFull - nRowCol)) //-V104
							{
								// Если PgDn нажат когда текущий элемент НЕ на последней строке
								INT_PTR nCorrection = (PVM == pvm_Thumbnails) //-V105
								                  ? (iCurItem % nRowCol) //-V104 //-V105
								                  : (nRowCol - 1);
								iCurKeyShift = (iTopItem + nFull - nRowCol - iCurItem) + nCorrection; //-V104
							}
							else
							{
								iCurKeyShift = nFull; //-V101
							}
						} break;
					}

					// Если это нажатие - то дополнительные нужно поместить в Unget буфер
					if (iCurKeyShift)
					{
						// Это событие из буфера нужно убрать
						bEraseEvent = true;

						// Плюсовать к общему сдвигу
						if (p->Event.KeyEvent.bKeyDown)
						{
							iShift += iCurKeyShift;
						}

						//if (abReadMode && n > 1)
						//{
						//	pFirstReplace = p+1;
						//	UngetBufferInput(n-1, p);
						//}
					}
				}
			} //end: if (vk == VK_UP || vk == VK_DOWN || vk == VK_LEFT || vk == VK_RIGHT || vk == VK_PRIOR || vk == VK_NEXT)

			//end: if (p->EventType == KEY_EVENT)
		}
		// Колесо мышки тоже перехватывать нужно
		else if (p->EventType == MOUSE_EVENT)
		{
			WARNING("Перехвать Wheel, чтобы не сбился TopPanelItem");
		}
		// При смене размера окна - нужно передернуть детектор
		else if (p->EventType == WINDOW_BUFFER_SIZE_EVENT)
		{
			if (gpRgnDetect)
				gpRgnDetect->OnWindowSizeChanged();
		}

		//// Если были помещены события в буфер Unget - требуется поместить в буфер и все что идут за ним,
		//// т.к. между ними будет "виртуальная" вставка событий
		//if (pFirstReplace && abReadMode)
		//{
		//	UngetBufferInput(1,p);
		//}

		//p++;
		if (bEraseEvent)
		{
			lbWasChanges = TRUE;
			// Если в буфере больше одного события - сдвинуть хвост
			//if (p < pEnd)
			//{
			//	memmove(pReplace, p, pEnd-p);
			//	pReplace++;
			//}
			// Нужно убрать это событие из буфера
			DWORD nRead = 0;
			INPUT_RECORD rr[2];

			if (!abReadMode)
			{
				sbInClearing = true;
				DEBUGSTRCTRL(L"-- removing processed event from input queue\n");
				ReadConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), rr, 1, &nRead);
				DEBUGSTRCTRL(nRead ? L"-- removing succeeded\n" : L"-- removing failed\n");
				sbInClearing = false;
			}

			if ((*lpNumberOfEventsRead) >= 1)
			{
				*lpNumberOfEventsRead = (*lpNumberOfEventsRead) - 1;
			}

			bEraseEvent = false;
		}
	}
	//// Скорректировать количество "считанных" событий
	//if (pFirstReplace && abReadMode)
	//{
	//	DWORD nReady = (int)(pFirstReplace - lpBuffer);
	//	if (nReady != *lpNumberOfEventsRead)
	//	{
	//		_ASSERTE(nReady <= nBufSize);
	//		_ASSERTE(nReady < *lpNumberOfEventsRead);
	//		*lpNumberOfEventsRead = nReady;
	//	}
	//}

	// Если в буфере есть "отложенные" события - возможно потребуется установка переменной окружения
	//if (pFirstReplace && abReadMode && gnUngetCount)
	if (iShift != 0)
	{
		_ASSERTE(lbWasChanges); lbWasChanges = TRUE;
		// Посчитать новые CurItem & TopItem оптимально для PanelViews
		iCurItem += iShift;

		if (iCurItem >= pi->ItemsNumber) iCurItem = pi->ItemsNumber-1;

		if (iCurItem < 0) iCurItem = 0;

		// Прикинуть идеальный TopItem для текущего направления движения курсора
		iTopItem = pi->CalcTopPanelItem(iCurItem, iTopItem);
#ifdef _DEBUG
		wchar_t szDbg[512];
		swprintf_c(szDbg,
		          L"Requesting panel redraw: {Cur:%i, Top:%i}.\n"
		          L"  Current state: {Cur:%i, Top:%i, Count:%i, OurTop:%i}\n"
		          L"  Current request: {%s, Cur:%i, Top=%i}\n",
		          (int)iCurItem, (int)iTopItem,
		          (int)pi->CurrentItem, (int)pi->TopPanelItem, (int)pi->ItemsNumber, (int)pi->OurTopPanelItem,
		          pi->bRequestItemSet ? L"YES" : L"No", (int)pi->ReqCurrentItem, (int)pi->ReqTopPanelItem);
		DEBUGSTRCTRL(szDbg);
#endif
		// Вызвать обновление панели с новой позицией
		pi->RequestSetPos(iCurItem, iTopItem);
		//pi->ReqCurrentItem = iCurItem; pi->ReqTopPanelItem = iTopItem;
		//pi->bRequestItemSet = true;
		//if (!gbSynchoRedrawPanelRequested)
		//{
		//	gbWaitForKeySequenceEnd = true;
		//	UpdateEnvVar(FALSE);
		//
		//	gbSynchoRedrawPanelRequested = true;
		//	ExecuteInMainThread(SYNCHRO_REDRAW_PANEL);
		//}
		//_ASSERTE(gnUngetCount>0);
		//INPUT_RECORD r = {EVENT_TYPE_REDRAW};
		//UngetBufferInput(1,&r);
		////SetEnvironmentVariable(TH_ENVVAR_NAME, TH_ENVVAR_SCROLL);
		//gbWaitForKeySequenceEnd = true;
		//UpdateEnvVar(FALSE);
	}

	//// Второй проход - если буфер достаточно большой - добавить в него события из Unget буфера
	//if (gnUngetCount && nBufSize > *lpNumberOfEventsRead)
	//{
	//	DWORD nAdd = 0;
	//	if (GetBufferInput(abReadMode/*abRemove*/, pFirstReplace, nBufSize-(*lpNumberOfEventsRead), &nAdd))
	//		*lpNumberOfEventsRead += nAdd;
	//}
	return lbWasChanges;
}

int ShowLastError()
{
	if (gnCreateViewError)
	{
		wchar_t szErrMsg[512];
		const wchar_t* pszTempl = GetMsgW(gnCreateViewError);

		if (pszTempl && *pszTempl)
		{
			swprintf_c(szErrMsg, pszTempl, gnWin32Error);

			if (gFarVersion.dwBuild>=FAR_Y2_VER)
				return FUNC_Y2(ShowMessageW)(szErrMsg, 0);
			else if (gFarVersion.dwBuild>=FAR_Y1_VER)
				return FUNC_Y1(ShowMessageW)(szErrMsg, 0);
			else
				return FUNC_X(ShowMessageW)(szErrMsg, 0);
		}
	}

	return 0;
}

void UpdateEnvVar(BOOL abForceRedraw)
{
	//WARNING("сбрасывать скролл нужно ПЕРЕД возвратом последней клавиши ДО REDRAW");
	//if (gbWaitForKeySequenceEnd)
	//{
	//	SetEnvironmentVariable(TH_ENVVAR_NAME, TH_ENVVAR_SCROLL);
	//}
	//else
	if (IsThumbnailsActive(FALSE/*abFocusRequired*/))
	{
		SetEnvironmentVariable(TH_ENVVAR_NAME, TH_ENVVAR_ACTIVE);
		//abForceRedraw = TRUE;
	}
	else
	{
		SetEnvironmentVariable(TH_ENVVAR_NAME, NULL);
	}

	//if (abForceRedraw)
	//{
	//	if (pviLeft.hView && IsWindowVisible(pviLeft.hView))
	//		Inva lidateRect(pviLeft.hView, NULL, FALSE);
	//	if (pviRight.hView && IsWindowVisible(pviRight.hView))
	//		Inva lidateRect(pviRight.hView, NULL, FALSE);
	//}
}

CeFullPanelInfo* GetFocusedThumbnails()
{
	if (pviLeft.hView == NULL && pviRight.hView == NULL)
		return NULL;

	CeFullPanelInfo* pi = NULL;

	if (pviLeft.hView && pviLeft.Focus && pviLeft.Visible)
		pi = &pviLeft;
	else if (pviRight.hView && pviRight.Focus && pviRight.Visible)
		pi = &pviRight;

	return pi;
}

CeFullPanelInfo* IsThumbnailsActive(BOOL abFocusRequired)
{
	if (pviLeft.hView == NULL && pviRight.hView == NULL)
		return NULL;

	if (!CheckWindows())
		return NULL;

	CeFullPanelInfo* pi = NULL;

	if (gpRgnDetect)
	{
		DWORD dwFlags = gpRgnDetect->GetFlags();

		if ((dwFlags & FR_ACTIVEMENUBAR) == FR_ACTIVEMENUBAR)
			return NULL; // активно меню

		if ((dwFlags & FR_FREEDLG_MASK) != 0)
			return NULL; // есть какой-то диалог
	}

	if (abFocusRequired)
	{
		pi = GetFocusedThumbnails();

		// Видим?
		if (pi)
		{
			if (!pi->hView || !IsWindowVisible(pi->hView))
			{
				return NULL; // панель полностью скрыта диалогом или погашена фаровская панель
			}
		}
	}
	else
	{
		if (pviLeft.hView && IsWindowVisible(pviLeft.hView))
			pi = &pviLeft;
		else if (pviRight.hView && IsWindowVisible(pviRight.hView))
			pi = &pviRight;
	}

	// Может быть PicView/MMView...
	if (pi)
	{
		RECT rc;
		GetClientRect(pi->hView, &rc);
		POINT pt = {((rc.left+rc.right)>>1),((rc.top+rc.bottom)>>1)};
		MapWindowPoints(pi->hView, ghConEmuWnd, &pt, 1);
		HWND hChild[2];
		hChild[0] = ChildWindowFromPointEx(ghConEmuWnd, pt, CWP_SKIPINVISIBLE|CWP_SKIPTRANSPARENT);
		// Теперь проверим полноэкранные окна
		MapWindowPoints(ghConEmuWnd, NULL, &pt, 1);
		hChild[1] = WindowFromPoint(pt);

		for (int i = 0; i <= 1; i++)
		{
			// В принципе, может быть и NULL, если координата попала в "прозрачную" часть hView
			if (hChild[i] && hChild[i] != pi->hView)
			{
				wchar_t szClass[128];

				if (GetClassName(hChild[i], szClass, 128))
				{
					if (lstrcmpi(szClass, L"FarPictureViewControlClass") == 0)
						return NULL; // активен PicView!

					if (lstrcmpi(szClass, L"FarMultiViewControlClass") == 0)
						return NULL; // активен MMView!
				}
			}
		}
	}

	return pi;
}

// Должен вернуть true, если активны только панели (нет диалогов или еще каких меню)
bool CheckWindows()
{
	bool lbRc = false;
	// Попробуем частично вернуть проверку окон через Far, но только ACTL_GETSHORTWINDOWINFO
	bool lbFarPanels = false;

	//if (gFarVersion.dwVerMajor==1)
	//	gbLastCheckWindow = CheckWindowsA();
	//else if (gFarVersion.dwBuild>=FAR_Y_VER)
	//	gbLastCheckWindow = FUNC_Y(CheckWindows)();
	//else
	//	gbLastCheckWindow = FUNC_X(CheckWindows)();
	//return gbLastCheckWindow;
	if (gFarVersion.dwVerMajor==1)
		lbFarPanels = CheckFarPanelsA();
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		lbFarPanels = FUNC_Y2(CheckFarPanelsW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		lbFarPanels = FUNC_Y1(CheckFarPanelsW)();
	else
		lbFarPanels = FUNC_X(CheckFarPanelsW)();

	// Запомнить активность панелей.
	if (gbFarPanelsReady != lbFarPanels)
		gbFarPanelsReady = lbFarPanels;

#if defined(_M_ARM64) || defined(__aarch64__)
	// Far 3 reports the active window directly. On ARM64 the region detector
	// does not receive the console output hook used by the legacy check.
	if (gFarVersion.dwBuild >= FAR_Y2_VER)
		return lbFarPanels;
#endif

	// Теперь, если API сказало, что панели есть и активны
	if (lbFarPanels && gpRgnDetect)
	{
		//WARNING: Диалоги уже должны быть "обнаружены"
		// используем gnRgnDetectFlags, т.к. gpRgnDetect может оказаться в процессе распознавания
		DWORD dwFlags = gnRgnDetectFlags; // gpRgnDetect->GetFlags();

		// вдруг панелей вообще не обнаружено?
		if ((dwFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)) != 0)
		{
			// нет диалогов
			if ((dwFlags & FR_FREEDLG_MASK) == 0)
			{
				// и нет активированного меню
				if ((dwFlags & FR_ACTIVEMENUBAR) != FR_ACTIVEMENUBAR)
				{
					lbRc = true;
				}
			}
		}
	}

	//gbLastCheckWindow = lbRc;
	return lbRc;
}

void CheckVarsInitialized()
{
	if (!gpRgnDetect)
	{
		gpRgnDetect = new CRgnDetect();
	}

	// Должно инититься в SetStartupInfo
	_ASSERTE(gpImgCache!=NULL);
	//if (!gpImgCache) {
	//	gpImgCache = new CImgCache(ghPluginModule);
	//}

	//CeFullPanelInfo* p = pviLeft.hView ? &pviLeft : &pviRight;

	if (gFarInfo.cbSize == 0)
	{
		gFarInfo.cbSize = sizeof(gFarInfo);
		gFarInfo.FarVer = gFarVersion;
		gFarInfo.nFarPID = GetCurrentProcessId();
		gFarInfo.nFarTID = GetCurrentThreadId();
		gFarInfo.bFarPanelAllowed = TRUE;

		// Загрузить из реестра настройки PanelTabs
		// -- gFarInfo.PanelTabs.SeparateTabs = gFarInfo.PanelTabs.ButtonColor = -1;
		// Перезагрузить настройки PanelTabs
		gFarInfo.PanelTabs.SeparateTabs = 1; gFarInfo.PanelTabs.ButtonColor = 0x1B; // умолчания...
		if (gFarVersion.dwVerMajor == 1)
			SettingsLoadOtherA();
		else if (gFarVersion.dwBuild>=FAR_Y2_VER)
			FUNC_Y2(SettingsLoadOtherW)();
		else if (gFarVersion.dwBuild>=FAR_Y1_VER)
			FUNC_Y1(SettingsLoadOtherW)();
		else
			FUNC_X(SettingsLoadOtherW)();

		//if (gszRootKey && *gszRootKey)
		//{
		//	int nLen = lstrlenW(gszRootKey);
		//	int cchSize = nLen+32;
		//	wchar_t* pszTabsKey = (wchar_t*)malloc(cchSize*2);
		//	_wcscpy_c(pszTabsKey, cchSize, gszRootKey);
		//	pszTabsKey[nLen-1] = 0;
		//	wchar_t* pszSlash = wcsrchr(pszTabsKey, L'\\');
		//	if (pszSlash)
		//	{
		//		_wcscpy_c(pszSlash, cchSize-(pszSlash-pszTabsKey), L"\\Plugins\\PanelTabs");
		//		HKEY hk;
		//		if (0 == RegOpenKeyExW(HKEY_CURRENT_USER, pszTabsKey, 0, KEY_READ, &hk))
		//		{
		//			DWORD dwVal, dwSize;
		//			if (!RegQueryValueExW(hk, L"SeparateTabs", NULL, NULL, (LPBYTE)&dwVal, &(dwSize = 4)))
		//				gFarInfo.PanelTabs.SeparateTabs = dwVal ? 1 : 0;
		//			if (!RegQueryValueExW(hk, L"ButtonColor", NULL, NULL, (LPBYTE)&dwVal, &(dwSize = 4)))
		//				gFarInfo.PanelTabs.ButtonColor = dwVal & 0xFF;
		//			RegCloseKey(hk);
		//		}
		//	}
		//	free(pszTabsKey);
		//}
	}

	if (!pviLeft.pSection)
	{
		pviLeft.pSection = new MSection();
	}

	if (!pviRight.pSection)
	{
		pviRight.pSection = new MSection();
	}
}

void ExecuteInMainThread(ConEmuThSynchroArg* pCmd)
{
	if (!pCmd) return;

	if (pCmd != SYNCHRO_REDRAW_PANEL && pCmd != SYNCHRO_RELOAD_PANELS && pCmd != SYNCHRO_RESTORE_PANELS)
	{
		if (gpLastSynchroArg && gpLastSynchroArg != pCmd)
		{
			LocalFree(gpLastSynchroArg); gpLastSynchroArg = NULL;
		}

		gpLastSynchroArg = pCmd;
		_ASSERTE(gpLastSynchroArg->bValid==1 && gpLastSynchroArg->bExpired==0);
	}

	if (pCmd == SYNCHRO_REDRAW_PANEL)
		gbSynchoRedrawPanelRequested = true;

	DEBUGSTRCTRL(
	    (pCmd == SYNCHRO_REDRAW_PANEL) ? L"ExecuteInMainThread(SYNCHRO_REDRAW_PANEL)\n" :
	    (pCmd == SYNCHRO_RELOAD_PANELS) ? L"ExecuteInMainThread(SYNCHRO_RELOAD_PANELS)\n" :
	    L"ExecuteInMainThread(...)\n"
	);
	BOOL lbLeftActive = FALSE;

	if (gFarVersion.dwVerMajor == 1)
	{
		// в 1.75 такой функции нет, придется хаком
	}
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
	{
		FUNC_Y2(ExecuteInMainThreadW)(pCmd);
	}
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
	{
		FUNC_Y1(ExecuteInMainThreadW)(pCmd);
	}
	else
	{
		FUNC_X(ExecuteInMainThreadW)(pCmd);
	}
}

int ProcessSynchroEventCommon(int Event, void *Param)
{
	if (Event != SE_COMMONSYNCHRO) return 0;

	// Некоторые плагины (NetBox) блокируют главный поток, и открывают
	// в своем потоке диалог. Это ThreadSafe. Некорректные открытия
	// отследить не удастся. Поэтому, считаем, если Far дернул наш
	// ProcessSynchroEventW, то это (временно) стала главная нить
	DWORD nPrevID = gnMainThreadId;
	gnMainThreadId = GetCurrentThreadId();

	if (Param == SYNCHRO_RESTORE_PANELS)
	{
		StartPlugin(FALSE);
	}
	else if (Param == SYNCHRO_REDRAW_PANEL)
	{
		DEBUGSTRCTRL(L"ProcessSynchroEventW(SYNCHRO_REDRAW_PANEL)\n");
		gbSynchoRedrawPanelRequested = false;

		for(int i = 0; i <= 1; i++)
		{
			CeFullPanelInfo* pp = (i==0) ? &pviLeft : &pviRight;

			if (pp->hView && pp->Visible && pp->bRequestItemSet)
			{
				// СРАЗУ сбросить флаг, чтобы потом не накалываться
				pp->bRequestItemSet = false;

				// а теперь - собственно курсор
				if (gFarVersion.dwVerMajor==1)
					SetCurrentPanelItemA((i==0), pp->ReqTopPanelItem, pp->ReqCurrentItem);
				else if (gFarVersion.dwBuild>=FAR_Y2_VER)
					FUNC_Y2(SetCurrentPanelItemW)((i==0), pp->ReqTopPanelItem, pp->ReqCurrentItem);
				else if (gFarVersion.dwBuild>=FAR_Y1_VER)
					FUNC_Y1(SetCurrentPanelItemW)((i==0), pp->ReqTopPanelItem, pp->ReqCurrentItem);
				else
					FUNC_X(SetCurrentPanelItemW)((i==0), pp->ReqTopPanelItem, pp->ReqCurrentItem);
			}
		}

		// Если отрисовка была отложена до окончания обработки клавиатуры - передернуть
		if (/*gbWaitForKeySequenceEnd &&*/ !gbConsoleChangesSyncho)
		{
			//gbWaitForKeySequenceEnd = false;
			UpdateEnvVar(FALSE);
			gbConsoleChangesSyncho = true;
			ExecuteInMainThread(SYNCHRO_RELOAD_PANELS);
		}
	}
	else if (Param == SYNCHRO_RELOAD_PANELS)
	{
		DEBUGSTRCTRL(L"ProcessSynchroEventW(SYNCHRO_RELOAD_PANELS)\n");
		_ASSERTE(gbConsoleChangesSyncho);
		gbConsoleChangesSyncho = false;
		OnReadyForPanelsReload();
	}
	else if (Param != NULL)
	{
		DEBUGSTRCTRL(L"ProcessSynchroEventW(...)\n");
		ConEmuThSynchroArg* pCmd = (ConEmuThSynchroArg*)Param;

		if (gpLastSynchroArg == pCmd) gpLastSynchroArg = NULL;

		if (pCmd->bValid == 1)
		{
			if (pCmd->bExpired == 0)
			{
				if (pCmd->nCommand == ConEmuThSynchroArg::eExecuteMacro)
				{
					PostMacro((wchar_t*)pCmd->Data);
				}
			}

			LocalFree(pCmd);
		}
	}

	gnMainThreadId = nPrevID;

	return 0;
}

int WINAPI ProcessSynchroEventW(int Event, void *Param)
{
	return ProcessSynchroEventCommon(Event, Param);
}

const PaletteColors& RefreshPalette()
{
	return RefreshPalette(gbFadeColors);
}

const PaletteColors& RefreshPalette(const bool isFade)
{
	gbFadeColors = isFade;
	_ASSERTE(gcrCurColors.size() == countof(gThPal.crFadePalette) && gcrCurColors.size() == countof(gThPal.crPalette));
	const auto elements = std::min(gcrCurColors.size(), countof(gThPal.crFadePalette));
	if (isFade)
	{
		std::copy(gThPal.crFadePalette, gThPal.crFadePalette + elements, gcrCurColors.begin());
	}
	else
	{
		std::copy(gThPal.crPalette, gThPal.crPalette + elements, gcrCurColors.begin());
	}
	return gcrCurColors;
}

const PaletteColors& GetPalette()
{
	return gcrCurColors;
}

BOOL LoadThSet(DWORD anGuiPid/* =-1 */)
{
	BOOL lbRc = FALSE;
	MFileMapping<PanelViewSetMapping> ThSetMap;
	_ASSERTE(ghConEmuRoot!=NULL);
	DWORD nGuiPID;
	GetWindowThreadProcessId(ghConEmuRoot, &nGuiPID);

	if (anGuiPid != -1)
	{
		_ASSERTE(nGuiPID == anGuiPid);
		nGuiPID = anGuiPid;
	}

	ThSetMap.InitName(CECONVIEWSETNAME, nGuiPID);

	if (!ThSetMap.Open())
	{
		MessageBox(NULL, ThSetMap.GetErrorText(), L"ConEmuTh", MB_ICONSTOP|MB_SETFOREGROUND|MB_SYSTEMMODAL);
	}
	else
	{
		ThSetMap.GetTo(&gThSet);
		ThSetMap.CloseMap();

		// Palette may be AppDistict
		HWND hRealConWnd = gfGetFarHWND2 ? gfGetFarHWND2(3) : NULL;
		if (hRealConWnd)
		{
			CESERVER_REQ* pIn = ExecuteNewCmd(CECMD_QUERYPALETTE, sizeof(CESERVER_REQ_HDR));
			if (pIn)
			{
				CESERVER_REQ* pOut = ExecuteGuiCmd(hRealConWnd, pIn, hRealConWnd);
				if (pOut->DataSize() >= sizeof(CESERVER_PALETTE))
				{
					std::copy(pOut->Palette.crPalette, pOut->Palette.crPalette + countof(pOut->Palette.crPalette),
						gThPal.crPalette);
					std::copy(pOut->Palette.crFadePalette, pOut->Palette.crFadePalette + countof(pOut->Palette.crFadePalette),
						gThPal.crFadePalette);
					RefreshPalette();
					lbRc = TRUE;
				}
				ExecuteFreeResult(pOut);
				ExecuteFreeResult(pIn);
			}
		}

		if (!lbRc)
		{
			MessageBox(NULL, L"Failed to get ConEmu palette!", L"ConEmuTh", MB_ICONSTOP|MB_SETFOREGROUND|MB_SYSTEMMODAL);
		}
	}

	return lbRc;
}

//sx, sy - смещение правого верхнего угла панелей от правого верхнего угла консоли
//cx, cy - если не 0 - то определяет ширину и высоту панелей
BOOL GetFarRect(SMALL_RECT* prcFarRect)
{
	BOOL lbFarBuffer = FALSE;
	prcFarRect->Left = prcFarRect->Right = prcFarRect->Top = prcFarRect->Bottom = 0;

	if (gFarVersion.dwVerMajor>2
	        || (gFarVersion.dwVerMajor==2 && gFarVersion.dwBuild>=1573))
	{
		if (gFarVersion.dwBuild>=FAR_Y2_VER)
			FUNC_Y2(GetFarRectW)(prcFarRect);
		else if (gFarVersion.dwBuild>=FAR_Y1_VER)
			FUNC_Y1(GetFarRectW)(prcFarRect);
		else
			FUNC_X(GetFarRectW)(prcFarRect);
		lbFarBuffer = (prcFarRect->Bottom && prcFarRect->Right);
	}

	return lbFarBuffer;
}

BOOL SettingsLoad(LPCWSTR pszName, DWORD* pValue)
{
	if (gFarVersion.dwVerMajor == 1)
		return SettingsLoadA(pszName, pValue);
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(SettingsLoadW)(pszName, pValue);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(SettingsLoadW)(pszName, pValue);
	else
		return FUNC_X(SettingsLoadW)(pszName, pValue);
}
BOOL SettingsLoadReg(LPCWSTR pszRegKey, LPCWSTR pszName, DWORD* pValue)
{
	BOOL lbValue = FALSE;
	HKEY hk = NULL;
	if (!RegOpenKeyExW(HKEY_CURRENT_USER, pszRegKey, 0, KEY_READ, &hk))
	{
		DWORD dwValue, dwSize;
		if (!RegQueryValueEx(hk, pszName, NULL, NULL, (LPBYTE)&dwValue, &(dwSize=sizeof(dwValue))))
		{
			*pValue = dwValue;
			lbValue = TRUE;
		}
		//RegSetValueEx(hk, pi->bLeftPanel ? L"LeftPanelView" : L"RightPanelView", 0,
		//	REG_DWORD, (LPBYTE)&dwMode, sizeof(dwMode));
		//if (RegQueryValueEx(hk, L"LeftPanelView", NULL, NULL, (LPBYTE)dwModes, &(dwSize=4)))
		RegCloseKey(hk);
	}
	return lbValue;
}
void SettingsSave(LPCWSTR pszName, DWORD* pValue)
{
	if (gFarVersion.dwVerMajor == 1)
		return SettingsSaveA(pszName, pValue);
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(SettingsSaveW)(pszName, pValue);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(SettingsSaveW)(pszName, pValue);
	else
		return FUNC_X(SettingsSaveW)(pszName, pValue);
}
void SettingsSaveReg(LPCWSTR pszRegKey, LPCWSTR pszName, DWORD* pValue)
{
	HKEY hk = NULL;
	if (!RegCreateKeyExW(HKEY_CURRENT_USER, pszRegKey, 0, NULL, 0, KEY_WRITE, NULL, &hk, NULL))
	{
		RegSetValueEx(hk, pszName, 0, REG_DWORD, (LPBYTE)pValue, sizeof(*pValue));
		RegCloseKey(hk);
	}
}

void SavePanelViewState(BOOL bLeftPanel, DWORD dwMode)
{
	gdwModes[bLeftPanel ? 0 : 1] = dwMode;
	SettingsSave(bLeftPanel ? L"LeftPanelView" : L"RightPanelView", &dwMode);
}

bool isPreloadByDefault()
{
	// Только когда плагин запущен из-под ConEmu можно получить настройки "PanelViews"
	// (они задаются через графический интерфейс)
	if (!ghConEmuWnd)
		return true;
	
	// Теперь можно настройки смотреть
	if ((gdwModes[0] || gdwModes[1]) && gThSet.bRestoreOnStartup)
		return true;
	return false;
}

void SettingsLoadOther(LPCWSTR pszRegKey)
{
	_ASSERTE(gFarVersion.dwVerMajor<=2);
	if (pszRegKey && *pszRegKey)
	{
		int nLen = lstrlenW(pszRegKey); //-V303
		int cchSize = nLen+32; //-V112
		wchar_t* pszTabsKey = (wchar_t*)malloc(cchSize*sizeof(*pszTabsKey));
		_wcscpy_c(pszTabsKey, cchSize, pszRegKey); //-V106
		pszTabsKey[nLen-1] = 0;
		wchar_t* pszSlash = wcsrchr(pszTabsKey, L'\\');

		WARNING("Переделать на Far3 api, когда соответствующие плагины будут переписаны");

		if (pszSlash)
		{
			_wcscpy_c(pszSlash, cchSize-(pszSlash-pszTabsKey), L"\\Plugins\\PanelTabs");
			HKEY hk;

			if (0 == RegOpenKeyExW(HKEY_CURRENT_USER, pszTabsKey, 0, KEY_READ, &hk))
			{
				DWORD dwVal, dwSize;

				if (!RegQueryValueExW(hk, L"SeparateTabs", NULL, NULL, (LPBYTE)&dwVal, &(dwSize = sizeof(dwVal))))
					gFarInfo.PanelTabs.SeparateTabs = dwVal ? 1 : 0;

				if (!RegQueryValueExW(hk, L"ButtonColor", NULL, NULL, (LPBYTE)&dwVal, &(dwSize = sizeof(dwVal))))
					gFarInfo.PanelTabs.ButtonColor = dwVal & 0xFF;

				RegCloseKey(hk);
			}
		}

		free(pszTabsKey);
	}
}

// Far3+
void FreePanelItemUserData2800(HANDLE hPlugin, DWORD_PTR UserData, FARPROC FreeUserDataCallback);

void CePluginPanelItem::FreeUserData()
{
	if (this)
	{
		if (gFarVersion.dwVerMajor >= 3)
		{
			if (hPlugin && UserData && FreeUserDataCallback)
			{
				FreePanelItemUserData2800(hPlugin, UserData, FreeUserDataCallback);
			}
			hPlugin = NULL;
			UserData = NULL;
			FreeUserDataCallback = NULL;
		}
	}
}
void CePluginPanelItem::FreeItem()
{
	CePluginPanelItem* p = this;
	if (p)
	{
		FreeUserData();
		free(p);
	}
}
