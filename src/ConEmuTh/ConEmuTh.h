
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


#pragma once

#include "../common/Common.h"
#include "../common/PaletteColors.h"
#include "ImgCache.h"

//#define SafeCloseHandle(h) { if ((h)!=NULL) { HANDLE hh = (h); (h) = NULL; if (hh!=INVALID_HANDLE_VALUE) CloseHandle(hh); } }

#ifdef _DEBUG
	#define OUTPUTDEBUGSTRING(m) OutputDebugString(m)
	#define SHOWDBGINFO(x) OutputDebugStringW(x)

	//#define CONEMU_LOG_FILES
	#undef CONEMU_LOG_FILES
#else
	#define OUTPUTDEBUGSTRING(m)
	#define SHOWDBGINFO(x)

	//#define CONEMU_LOG_FILES
	#undef CONEMU_LOG_FILES
#endif


#define ISALPHA(c) ((((c) >= (BYTE)'c') && ((c) <= (BYTE)'z')) || (((c) >= (BYTE)'C') && ((c) <= (BYTE)'Z')))
#define isPressed(inp) ((GetKeyState(inp) & 0x8000) == 0x8000)
//#define countof(array) (sizeof(array)/sizeof(*array))

// X - меньшая, Y - большая
#define FAR_X_VER 995
#define FAR_Y1_VER 1900
#define FAR_Y2_VER 2800
#define FUNC_X(fn) fn##995
#define FUNC_Y1(fn) fn##1900
#define FUNC_Y2(fn) fn##2800


#define ConEmuTh_SysID 0x43455568 // 'CETh'


//#define EVENT_TYPE_REDRAW 250
#define TH_ENVVAR_NAME L"FarThumbnails"
#define TH_ENVVAR_ACTIVE L"Active"
#define TH_ENVVAR_SCROLL L"Scrolling"



//typedef struct tag_FarVersion {
//	union {
//		DWORD dwVer;
//		struct {
//			WORD dwVerMinor;
//			WORD dwVerMajor;
//		};
//	};
//	DWORD dwBuild;
//} FarVersion;

struct CEFAR_FIND_DATA
{
	//FILETIME ftCreationTime;
	//FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	union
	{
		struct
		{
			DWORD nFileSizeHigh;
			DWORD nFileSizeLow;
		};
		unsigned __int64 nFileSize;
	};
	const wchar_t *lpwszFileName;
	const wchar_t *lpwszFileNamePart;
	const wchar_t *lpwszFileExt;
	//const wchar_t *lpwszAlternateFileName;
	DWORD    dwFileAttributes; // Decrease 32bit structure size by rearrange
};

struct CePluginPanelItem
{
	DWORD			 cbSize;
	CEFAR_FIND_DATA  FindData;
	BOOL             bVirtualItem;
	HANDLE           hPlugin;
	DWORD_PTR        UserData;
	FARPROC          FreeUserDataCallback;
	ImgLoadType      PreviewLoaded; // пытались ли уже загружать превьюшку, и что удалось загрузить
	const wchar_t*   pszFullName; // Для упрощения отрисовки - ссылка на временный буфер
	const wchar_t*   pszDescription; // ссылка на данные в этом CePluginPanelItem
	unsigned __int64 Flags;
	DWORD            NumberOfLinks;
	BOOL             bIsCurrent; // ТОЛЬКО ИНФОРМАЦИОННО, ориентироваться на это поле нельзя, оно может быть неактуально
	//BOOL           bItemColorLoaded;
	//COLORREF       crFore, crBack;
	struct
	{
		BOOL Loaded;
		unsigned __int64 Flags;
		COLORREF ForegroundColor;
		COLORREF BackgroundColor;
		int PosX, PosY; // 1-based, relative to Far workspace
	} BisInfo;
	//
	void FreeItem();
	void FreeUserData();
};

struct CePluginPanelItemColor
{
	BOOL            bItemColorLoaded;
	COLORREF        crFore, crBack;
};

enum CEPANELINFOFLAGS
{
	CEPFLAGS_SHOWHIDDEN         = 0x00000001,
	CEPFLAGS_HIGHLIGHT          = 0x00000002,
	CEPFLAGS_REVERSESORTORDER   = 0x00000004,
	CEPFLAGS_USESORTGROUPS      = 0x00000008,
	CEPFLAGS_SELECTEDFIRST      = 0x00000010,
	CEPFLAGS_REALNAMES          = 0x00000020,
	CEPFLAGS_NUMERICSORT        = 0x00000040,
	CEPFLAGS_PANELLEFT          = 0x00000080,
};


#include "Display.h"



//struct ThumbnailSettings
//{
//	TODO("nWidth & nHeight - deprecated");
//	// Наверное заменить на WholeWidth() & WholeHeight()
//	int nWidth, nHeight; // 98x98
//
//	int nThumbSize; // 96
//	int nIconSize; // 48
//	DWORD crBackground; // 0xFFFFFF (RGB) или 0xFF000000 (Index)
//
//	int nThumbFrame; // 1 (серая рамка вокруг превьюшки
//	DWORD crThumbFrame; // 0xFFFFFF (RGB) или 0xFF000000 (Index)
//	int nSelectFrame; // 1 (рамка вокруг текущего элемента)
//	DWORD crSelectFrame; // 0xFFFFFF (RGB) или 0xFF000000 (Index)
//	int nHSpacing, nVSpacing; // 5, 25 - промежуток между двумя рамками
//
//	TODO("Вроде не нужно - оставить только nSelectFrame? Или пусть будет?");
//	int nHPadding, nVPadding; // 1, 1 - зарезевированный отступ
//
//	int nFontHeight; // 14
//	wchar_t sFontName[32]; // Tahoma
//	BOOL bLoadPreviews, bLoadFolders;
//	int nLoadTimeout; // 15 sec
//
//	int nMaxZoom;
//	BOOL bUsePicView2;
//
//	wchar_t sCacheFolder[MAX_PATH];
//
//
//	void Load() {
//		TODO("nWidth & nHeight - deprecated");
//		nWidth = nHeight = 98;
//
//		nThumbSize = 96; // пусть реально будет 96. Чтобы можно было 500% на 16х16 поставить
//		nIconSize = 32;
//
//		nThumbFrame = 1;
//		nSelectFrame = 1;
//
//		nHSpacing = 5; nVSpacing = 25;
//		nHPadding = 1; nVPadding = 1;
//
//		nFontHeight = 14;
//		lstrcpy(sFontName, L"Tahoma");
//
//		bLoadPreviews = TRUE;
//		bLoadFolders = TRUE;
//		nLoadTimeout = 15;
//		nMaxZoom = 500; // но не больше размера превьюшки :)
//		bUsePicView2 = TRUE;
//        sCacheFolder[0] = 0;
//	};
//};
BOOL LoadThSet(DWORD anGuiPid=-1);
extern PanelViewSetMapping gThSet;
extern CESERVER_PALETTE gThPal;
extern BOOL gbCancelAll;



extern HWND ghConEmuRoot;
extern HWND ghConEmuWnd;
extern HMODULE ghPluginModule; // ConEmuTh.dll - сам плагин
extern DWORD gnSelfPID, gnMainThreadId;
//extern int lastModifiedStateW;
//extern bool gbHandleOneRedraw; //, gbHandleOneRedrawCh;
//extern WCHAR gszDir1[CONEMUTABMAX], gszDir2[CONEMUTABMAX],
//extern wchar_t* gszRootKey;
//extern int maxTabCount, lastWindowCount;
//extern CESERVER_REQ* tabs; //(ConEmuTab*) calloc(maxTabCount, sizeof(ConEmuTab));
//extern CESERVER_REQ* gpCmdRet;
//extern HWND ConEmuHwnd;
//extern HWND FarHwnd;
extern FarVersion gFarVersion;
//#define IsFarLua ((gFarVersion.dwVerMajor > 3) || ((gFarVersion.dwVerMajor == 3) && (gFarVersion.dwBuild >= 2851)))
//extern int lastModifiedStateW;
////extern HANDLE hEventCmd[MAXCMDCOUNT];
//extern HANDLE hThread;
////extern WCHAR gcPlugKey;
////WARNING("Убрать, заменить ghConIn на GetStdHandle()"); // Иначе в Win7 будет буфер разрушаться
////extern HANDLE ghConIn;
//extern BOOL gbNeedPostTabSend;
//extern HANDLE ghServerTerminateEvent;
//extern CESERVER_CONSOLE_MAPPING_HDR *gpConsoleInfo;
//extern DWORD gnSelfPID;
extern CeFullPanelInfo pviLeft, pviRight;
extern HANDLE ghDisplayThread; extern DWORD gnDisplayThreadId;
//extern HWND ghLeftView, ghRightView;
//extern bool gbWaitForKeySequenceEnd;
extern DWORD gnWaitForKeySeqTick;
extern CEFarPanelSettings gFarPanelSettings;
extern CEFarInterfaceSettings gFarInterfaceSettings;
//class CRgnDetect;
//extern CRgnDetect *gpRgnDetect;

// *** lng resources begin ***
extern wchar_t gsFolder[64], /*gsHardLink[64],*/ gsSymLink[64], gsJunction[64], gsTitleThumbs[64], gsTitleTiles[64];
// *** lng resources end ***

const ConEmu::PaletteColors& RefreshPalette();
const ConEmu::PaletteColors& RefreshPalette(bool isFade);
const ConEmu::PaletteColors& GetPalette();

extern bool gbFarPanelsReady;

class CRgnDetect;
extern CRgnDetect *gpRgnDetect;
extern CEFAR_INFO_MAPPING gFarInfo;
extern DWORD gnRgnDetectFlags;


//typedef struct tag_SynchroArg {
//	enum {
//		eCommand,
//		eInput
//	} SynchroType;
//	HANDLE hEvent;
//	LPARAM Result;
//	LPARAM Param1, Param2;
//	BOOL Obsolete;
//	//BOOL Processed;
//} SynchroArg;

void FUNC_X(GetPluginInfoW)(void* piv);
void FUNC_Y1(GetPluginInfoW)(void* piv);
void FUNC_Y2(GetPluginInfoW)(void* piv);

HANDLE OpenPluginWcmn(int OpenFrom,INT_PTR Item,bool FromMacro);
HANDLE WINAPI OpenPluginW1(int OpenFrom,INT_PTR Item);
HANDLE WINAPI OpenPluginW2(int OpenFrom,const GUID* Guid,INT_PTR Data);

BOOL SettingsLoad(LPCWSTR pszName, DWORD* pValue);
BOOL SettingsLoadReg(LPCWSTR pszRegKey, LPCWSTR pszName, DWORD* pValue);
BOOL SettingsLoadA(LPCWSTR pszName, DWORD* pValue);
BOOL FUNC_X(SettingsLoadW)(LPCWSTR pszName, DWORD* pValue);
BOOL FUNC_Y1(SettingsLoadW)(LPCWSTR pszName, DWORD* pValue);
BOOL FUNC_Y2(SettingsLoadW)(LPCWSTR pszName, DWORD* pValue);
void SettingsLoadOther(LPCWSTR pszRegKey);
void SettingsLoadOtherA();
void FUNC_X(SettingsLoadOtherW)();
void FUNC_Y1(SettingsLoadOtherW)();
void FUNC_Y2(SettingsLoadOtherW)();
void SettingsSave(LPCWSTR pszName, DWORD* pValue);
void SettingsSaveReg(LPCWSTR pszRegKey, LPCWSTR pszName, DWORD* pValue);
void SettingsSaveA(LPCWSTR pszName, DWORD* pValue);
void FUNC_X(SettingsSaveW)(LPCWSTR pszName, DWORD* pValue);
void FUNC_Y1(SettingsSaveW)(LPCWSTR pszName, DWORD* pValue);
void FUNC_Y2(SettingsSaveW)(LPCWSTR pszName, DWORD* pValue);
void SavePanelViewState(BOOL bLeftPanel, DWORD dwMode);

bool isPreloadByDefault();

void EntryPoint(int OpenFrom,INT_PTR Item,bool FromMacro);
BOOL LoadFarVersion();
void StartPlugin(BOOL abManual);
void ExitPlugin(void);
void FUNC_X(ExitFARW)(void);
void FUNC_Y1(ExitFARW)(void);
void FUNC_Y2(ExitFARW)(void);
void FUNC_X(SetStartupInfoW)(void *aInfo);
void FUNC_Y1(SetStartupInfoW)(void *aInfo);
void FUNC_Y2(SetStartupInfoW)(void *aInfo);
HANDLE FUNC_Y1(OpenW)(const void* aInfo);
HANDLE FUNC_Y2(OpenW)(const void* aInfo);
int ShowMessage(int aiMsg, int aiButtons);
int ShowMessageA(int aiMsg, int aiButtons);
int FUNC_X(ShowMessageW)(int aiMsg, int aiButtons);
int FUNC_Y1(ShowMessageW)(int aiMsg, int aiButtons);
int FUNC_Y2(ShowMessageW)(int aiMsg, int aiButtons);
int ShowMessageA(LPCSTR asMsg, int aiButtons);
int FUNC_X(ShowMessageW)(LPCWSTR asMsg, int aiButtons);
int FUNC_Y1(ShowMessageW)(LPCWSTR asMsg, int aiButtons);
int FUNC_Y2(ShowMessageW)(LPCWSTR asMsg, int aiButtons);
void PostMacro(wchar_t* asMacro);
void PostMacroA(char* asMacro);
void FUNC_X(PostMacroW)(wchar_t* asMacro);
void FUNC_Y1(PostMacroW)(wchar_t* asMacro);
void FUNC_Y2(PostMacroW)(wchar_t* asMacro);
LPCWSTR GetMsgW(int aiMsg);
const wchar_t* GetMsgA(int aiMsg, wchar_t* rsMsg/*MAX_PATH*/);
LPCWSTR FUNC_Y1(GetMsgW)(int aiMsg);
LPCWSTR FUNC_Y2(GetMsgW)(int aiMsg);
LPCWSTR FUNC_X(GetMsgW)(int aiMsg);
int ShowPluginMenu();
int ShowPluginMenuA();
int FUNC_Y1(ShowPluginMenuW)();
int FUNC_Y2(ShowPluginMenuW)();
int FUNC_X(ShowPluginMenuW)();
BOOL IsMacroActive();
BOOL IsMacroActiveA();
BOOL FUNC_X(IsMacroActiveW)();
BOOL FUNC_Y1(IsMacroActiveW)();
BOOL FUNC_Y2(IsMacroActiveW)();
int GetMacroArea();
int FUNC_X(GetMacroAreaW)();
int FUNC_Y1(GetMacroAreaW)();
int FUNC_Y2(GetMacroAreaW)();
//CeFullPanelInfo* LoadPanelInfo(BOOL abActive);
CeFullPanelInfo* GetActivePanel();
BOOL LoadPanelInfoA(BOOL abActive);
BOOL FUNC_X(LoadPanelInfoW)(BOOL abActive);
BOOL FUNC_Y1(LoadPanelInfoW)(BOOL abActive);
BOOL FUNC_Y2(LoadPanelInfoW)(BOOL abActive);
void ReloadPanelsInfo();
void ReloadPanelsInfoA();
void FUNC_X(ReloadPanelsInfoW)();
void FUNC_Y1(ReloadPanelsInfoW)();
void FUNC_Y2(ReloadPanelsInfoW)();
//BOOL IsLeftPanelActive();
//BOOL IsLeftPanelActiveA();
//BOOL FUNC_X(IsLeftPanelActive)();
//BOOL FUNC_Y(IsLeftPanelActive)();
void LoadPanelItemInfo(CeFullPanelInfo* pi, INT_PTR nItem);
void LoadPanelItemInfoA(CeFullPanelInfo* pi, INT_PTR nItem);
void FUNC_X(LoadPanelItemInfoW)(CeFullPanelInfo* pi, INT_PTR nItem);
void FUNC_Y1(LoadPanelItemInfoW)(CeFullPanelInfo* pi, INT_PTR nItem);
void FUNC_Y2(LoadPanelItemInfoW)(CeFullPanelInfo* pi, INT_PTR nItem);
bool CheckWindows();
//bool CheckWindowsA();
//bool FUNC_X(CheckWindows)();
//bool FUNC_Y(CheckWindows)();
bool CheckFarPanelsA();
bool FUNC_X(CheckFarPanelsW)();
bool FUNC_Y1(CheckFarPanelsW)();
bool FUNC_Y2(CheckFarPanelsW)();

// Эта "дисплейная" функция вызывается из основной нити, там можно дергать FAR Api
//void DisplayReloadPanel(CeFullPanelInfo* pi);

extern int gnCreateViewError;
extern DWORD gnWin32Error;
//HWND CreateView(CeFullPanelInfo* pi);
void UpdateEnvVar(BOOL abForceRedraw);
CeFullPanelInfo* IsThumbnailsActive(BOOL abFocusRequired);
CeFullPanelInfo* GetFocusedThumbnails();
BOOL CheckPanelSettings(BOOL abSilence);
BOOL CheckPanelSettingsA(BOOL abSilence);
BOOL FUNC_X(CheckPanelSettingsW)(BOOL abSilence);
BOOL FUNC_Y1(CheckPanelSettingsW)(BOOL abSilence);
BOOL FUNC_Y2(CheckPanelSettingsW)(BOOL abSilence);

BOOL GetFarRect(SMALL_RECT* prcFarRect);
void FUNC_X(GetFarRectW)(SMALL_RECT* prcFarRect);
void FUNC_Y1(GetFarRectW)(SMALL_RECT* prcFarRect);
void FUNC_Y2(GetFarRectW)(SMALL_RECT* prcFarRect);

typedef struct
{
	int bValid; // Must be ==1
	int bExpired; // Must be ==0, if !=0 - просто освободить память
	enum
	{
		eExecuteMacro = 0,
	} nCommand;
	WORD Data[1]; // variable array
} ConEmuThSynchroArg;
extern bool gbSynchoRedrawPanelRequested;
extern ConEmuThSynchroArg* gpLastSynchroArg;
void ExecuteInMainThread(ConEmuThSynchroArg* pCmd);
#if defined(_M_ARM64) || defined(__aarch64__)
void RefreshPanelViewsAfterInput();
#endif
void FUNC_X(ExecuteInMainThreadW)(ConEmuThSynchroArg* pCmd);
void FUNC_Y1(ExecuteInMainThreadW)(ConEmuThSynchroArg* pCmd);
void FUNC_Y2(ExecuteInMainThreadW)(ConEmuThSynchroArg* pCmd);
//int WINAPI ProcessSynchroEventW(int Event, void *Param);

#define SYNCHRO_REDRAW_PANEL ((ConEmuThSynchroArg*)1)
void SetCurrentPanelItemA(BOOL abLeftPanel, INT_PTR anTopItem, INT_PTR anCurItem);
void FUNC_X(SetCurrentPanelItemW)(BOOL bLeftPanel, INT_PTR anTopItem, INT_PTR anCurItem);
void FUNC_Y1(SetCurrentPanelItemW)(BOOL bLeftPanel, INT_PTR anTopItem, INT_PTR anCurItem);
void FUNC_Y2(SetCurrentPanelItemW)(BOOL bLeftPanel, INT_PTR anTopItem, INT_PTR anCurItem);

#define SYNCHRO_RELOAD_PANELS ((ConEmuThSynchroArg*)2)
#define SYNCHRO_RESTORE_PANELS ((ConEmuThSynchroArg*)3)


// ConEmu.dll
typedef int (WINAPI *RegisterPanelView_t)(PanelViewInit *ppvi);
typedef HWND (WINAPI *GetFarHWND2_t)(BOOL abConEmuOnly);
extern RegisterPanelView_t gfRegisterPanelView;
extern GetFarHWND2_t gfGetFarHWND2;
BOOL CheckConEmu(BOOL abSilence=FALSE);
BOOL WINAPI OnPrePeekConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult);
BOOL WINAPI OnPostPeekConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult);
BOOL WINAPI OnPreReadConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult);
BOOL WINAPI OnPostReadConsole(HANDLE hInput, PINPUT_RECORD lpBuffer, DWORD nBufSize, LPDWORD lpNumberOfEventsRead, BOOL* pbResult);
BOOL WINAPI OnPreWriteConsoleOutput(HANDLE hOutput,const CHAR_INFO *lpBuffer,COORD dwBufferSize,COORD dwBufferCoord,PSMALL_RECT lpWriteRegion);


/* Other plugin integrations */
#define IMPEX_MAGIC 0x78456D49 // 'ImEx'
struct ImpExPanelItem
{
	DWORD nMagic;			// IMPEX_MAGIC
	DWORD cbSizeOfStruct;	// 1196 в этой версии
	DWORD nBinarySize;		// размер
	LPBYTE pBinaryData;		// и собственно бинарные данные
	// Остальные поля не интересуют
};
#define PBFAR_MAGIC 0x50624672 // 'PbFr'
struct PbFarPanelItem
{
	DWORD nMagic;			// PBFAR_MAGIC
	DWORD cbSizeOfStruct;	//
	DWORD_PTR pPlugin;      // CPBPlugin*
};
typedef int (WINAPI* GetPbFarFileData_t)(DWORD_PTR pPlugin, LPCWSTR asFile, LPVOID* ppData, DWORD* pDataSize);

#define REGEDIT_MAGIC 0x52674564 // 'RgEd'
