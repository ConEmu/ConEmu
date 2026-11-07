//TODO: Осталось сделать только список цветов с перебором по букве диска
//TODO: И аналогично для цвета фона градусника/картинок/текста и самого градусника

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


#ifdef _DEBUG
//  Раскомментировать, чтобы сразу после загрузки плагина показать MessageBox, чтобы прицепиться дебаггером
//  #define SHOW_STARTED_MSGBOX
//  #define SHOW_WRITING_RECTS
#endif


#include "../common/Common.h"
#include "../common/CEStr.h"
#ifdef _DEBUG
#pragma warning( disable : 4995 )
#endif
#include "../common/pluginW1761.hpp" // Отличается от 995 наличием SynchoApi
#ifdef _DEBUG
#pragma warning( default : 4995 )
#endif
#define GDIPVER 0x0110
#include "../common/GdiPlusLt.h"
#include "../common/xmllite.h"
#include "../common/MStrDup.h"
#include "../common/MStream.h"
#include "../common/UnicodeChars.h"
#include "../common/FarVersion.h"
#include "../common/MSectionSimple.h"
#include "../common/WThreads.h"
#include "ConEmuBg.h"

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

#define DBGSTR(s) OutputDebugString(s);

#if defined(__GNUC__)
extern "C" {
	BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved);
	void WINAPI SetStartupInfoW(void *aInfo);
	void WINAPI OnConEmuLoaded(struct ConEmuLoadedArg* pConEmuInfo);
	void WINAPI GetPluginInfoWcmn(void *piv);
};
#endif

#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX               0x00100000
#endif

HMODULE ghPluginModule = NULL; // ConEmuBg.dll - сам плагин
//wchar_t* gszRootKey = NULL;
FarVersion gFarVersion = {};
static RegisterBackground_t gfRegisterBackground = NULL;
bool gbInStartPlugin = false;
bool gbSetStartupInfoOk = false;
ConEmuBgSettings gSettings[] = {
	{L"PluginEnabled", (LPBYTE)&gbBackgroundEnabled, REG_BINARY, 1},
	{L"XmlConfigFile", (LPBYTE)gsXmlConfigFile, REG_SZ, countof(gsXmlConfigFile)},
	//{L"LinesColor", (LPBYTE)&gcrLinesColor, REG_DWORD, 4},
	//{L"HilightType", (LPBYTE)&giHilightType, REG_DWORD, 4},
	//{L"HilightPlugins", (LPBYTE)&gbHilightPlugins, REG_BINARY, 1},
	//{L"HilightPlugBack", (LPBYTE)&gcrHilightPlugBack, REG_DWORD, 4},
	{NULL}
};

BOOL gbBackgroundEnabled = FALSE;
wchar_t gsXmlConfigFile[MAX_PATH] = {};
BOOL gbMonitorFileChange = FALSE;
//COLORREF gcrLinesColor = RGB(0,0,0xA8); // чуть светлее синего
//int giHilightType = 0; // 0 - линии, 1 - полосы
//BOOL gbHilightPlugins = FALSE;
//COLORREF gcrHilightPlugBack = RGB(0xA8,0,0); // чуть светлее красного



// minimal(?) FAR version 2.0 alpha build FAR_X_VER
int WINAPI GetMinFarVersionW(void)
{
	// ACTL_SYNCHRO required
	return MAKEFARVERSION(2,0,std::max(1007,FAR_X_VER));
}

void WINAPI GetPluginInfoWcmn(void *piv)
{
	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(GetPluginInfoW)(piv);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(GetPluginInfoW)(piv);
	else
		FUNC_X(GetPluginInfoW)(piv);
}


BOOL gbInfoW_OK = FALSE;


void WINAPI ExitFARW(void);
void WINAPI ExitFARW3(void*);
int WINAPI ConfigureW(int ItemNumber);
int WINAPI ConfigureW3(void*);
bool FMatch(LPCWSTR asMask, LPWSTR asPath);

#include "../common/SetExport.h"
ExportFunc Far3Func[] =
{
	{"ExitFARW",   (void*)ExitFARW,   (void*)ExitFARW3},
	{"ConfigureW", (void*)ConfigureW, (void*)ConfigureW3},
	{NULL}
};

BOOL WINAPI DllMain(HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch(ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		{
			ghPluginModule = (HMODULE)hModule;
			//ghWorkingModule = hModule;
			HeapInitialize();

#ifdef SHOW_STARTED_MSGBOX
			if (!IsDebuggerPresent())
				MessageBoxA(NULL, "ConEmuBg*.dll loaded", "ConEmuBg plugin", 0);
#endif

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

void WINAPI SetStartupInfoW(void *aInfo)
{
	gbSetStartupInfoOk = true;

	if (!gFarVersion.dwVerMajor) LoadFarVersion();

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(SetStartupInfoW)(aInfo);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(SetStartupInfoW)(aInfo);
	else
		FUNC_X(SetStartupInfoW)(aInfo);

	//_ASSERTE(gszRootKey!=NULL && *gszRootKey!=0);
	gbInfoW_OK = TRUE;
	StartPlugin(FALSE);
}


struct DrawInfo
{
	LPCWSTR  szVolume, szVolumeRoot, szVolumeSize, szVolumeFree;
	DWORD    nDriveType;
	enum {
		dib_Small = 0, dib_Large = 1, dib_Off = 2
	} nSpaceBar;

	wchar_t  szPic[MAX_PATH];  // Это относительный путь! считая от папки с плагином ("img/plugin.png")
	wchar_t  szText[MAX_PATH]; // Текст, который отображается под картинкой ("G: 128Gb")
	COLORREF crBack[32];       // Цвет фона
	int      nBackCount;
	COLORREF crDark[32];       // Цвет спрайтов, текста, фона градусника
	int      nDarkCount;
	COLORREF crLight[32];      // Цвет градусника
	int      nLightCount;

	enum DrawInfoFlags
	{
		dif_PicFilled        = 0x0000001,
		dif_TextFilled       = 0x0000002,
		dif_BackFilled       = 0x0000004,
		dif_DarkFilled       = 0x0000008,
		dif_LightFilled      = 0x0000010,
		dif_ShiftBackColor   = 0x0000020,
		dif_Disabled         = 0x8000000,
	};
	DWORD Flags; // of DrawInfoFlags
};

bool gbGdiPlusInitialized = false;
typedef HRESULT (WINAPI* CreateXmlReader_t)(REFIID riid, void** ppvObject, void* pMalloc);
CreateXmlReader_t gfCreateXmlReader = NULL;
HMODULE ghXmlLite = NULL;

struct XmlConfigFile
{
	MSectionSimple* cr;
	FILETIME FileTime;
	LPBYTE FileData;
	DWORD FileSize;
};
XmlConfigFile XmlFile = {};
wchar_t* gpszXmlFile = NULL;
wchar_t* gpszXmlFolder = NULL;
HANDLE ghXmlNotification = NULL;
const wchar_t* szDefaultXmlName = L"Background.xml";

void ReportFail(LPCWSTR asInfo)
{
	wchar_t szTitle[128];
	swprintf_c(szTitle, L"ConEmuBg, PID=%u", GetCurrentProcessId());

	const CEStr pszErr(asInfo,
		L"\n" L"Config value: ", gsXmlConfigFile[0] ? gsXmlConfigFile : szDefaultXmlName,
		L"\n" L"Expanded: ", gpszXmlFile ? gpszXmlFile : L"<NULL>");

	MessageBox(nullptr, pszErr, szTitle, MB_ICONSTOP | MB_SYSTEMMODAL);
}

bool WasXmlLoaded()
{
	return (XmlFile.FileData != NULL);
}

// Возвращает TRUE, если файл изменился
bool CheckXmlFile(bool abUpdateName /*= false*/)
{
	bool lbChanged = false, lbNeedCheck = false;
	DWORD nNotify = 0;
	wchar_t szErr[128];

	_ASSERTE(XmlFile.cr && XmlFile.cr->IsInitialized());
	MSectionLockSimple CS; CS.Lock(XmlFile.cr);

	if (abUpdateName)
	{
		bool lbNameOk = false;
		size_t cchMax = MAX_PATH+1;
		if (!gpszXmlFile)
			gpszXmlFile = (wchar_t*)malloc(cchMax*sizeof(*gpszXmlFile));

		if (gpszXmlFile)
		{
			gpszXmlFile[0] = 0;
			// If config file has full path or environment variable (supposing full path with %FARHOME% or %ConEmuDir%)
			if (*gsXmlConfigFile && (wcschr(gsXmlConfigFile, L'\\') || wcschr(gsXmlConfigFile, L'%')))
			{
				DWORD nRc = ExpandEnvironmentStrings(gsXmlConfigFile, gpszXmlFile, MAX_PATH+1);
				if (!nRc || nRc > MAX_PATH)
				{
					gpszXmlFile[0] = 0;
					ReportFail(L"Too long xml path after ExpandEnvironmentStrings");
				}
				else
				{
					lbNameOk = true;
				}
			}
			if (!lbNameOk && !*gpszXmlFile)
			{
				LPCWSTR pszXmlName = *gsXmlConfigFile ? gsXmlConfigFile : szDefaultXmlName;
				int nNameLen = lstrlen(pszXmlName);
				if (GetModuleFileName(ghPluginModule, gpszXmlFile, MAX_PATH-nNameLen))
				{
					wchar_t* pszSlash = wcsrchr(gpszXmlFile, L'\\');
					if (pszSlash) pszSlash++; else pszSlash = gpszXmlFile;
					_wcscpy_c(pszSlash, cchMax-(pszSlash-gpszXmlFile), pszXmlName);
					lbNameOk = true;
				}
			}
		}
		if (!lbNameOk)
		{
			if (gpszXmlFile)
			{
				free(gpszXmlFile);
				gpszXmlFile = NULL;
			};
			ReportFail(L"Can't initialize name of xml file");
			lbChanged = false;
			goto wrap;
		}

		// Notification - мониторинг изменения файла
		if (ghXmlNotification && (ghXmlNotification != INVALID_HANDLE_VALUE))
		{
			FindCloseChangeNotification(ghXmlNotification);
			ghXmlNotification = NULL;
		}
		if (gpszXmlFolder)
			free(gpszXmlFolder);
		gpszXmlFolder = lstrdup(gpszXmlFile).Detach();
		if (gpszXmlFolder)
		{
			wchar_t* pszSlash = wcsrchr(gpszXmlFolder, L'\\');
			if (pszSlash)
			{
				if (wcschr(gpszXmlFolder, L'\\') < pszSlash)
					pszSlash[0] = 0;
				else
					pszSlash[1] = 0;
			}

			//Issue 1230
			if (gbMonitorFileChange)
			{
				ghXmlNotification = FindFirstChangeNotification(gpszXmlFolder, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_SIZE|FILE_NOTIFY_CHANGE_LAST_WRITE);
			}
			else
			{
				_ASSERTE(ghXmlNotification==NULL);
				lbNeedCheck = true;
			}
		}
	}

	if (ghXmlNotification && (ghXmlNotification != INVALID_HANDLE_VALUE))
	{
		nNotify = WaitForSingleObject(ghXmlNotification, 0);
		if (nNotify == WAIT_OBJECT_0)
		{
			lbNeedCheck = true;
			//FindNextChangeNotification(ghXmlNotification); -- в конце позовем
		}
	}

	// в первый раз - выставляем строго
	if (!lbNeedCheck && !XmlFile.FileData)
		lbNeedCheck = true;

	if (gpszXmlFile && *gpszXmlFile && lbNeedCheck)
	{
		HANDLE hFile = CreateFile(gpszXmlFile, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, NULL);
		if (hFile && hFile != INVALID_HANDLE_VALUE)
		{
			BY_HANDLE_FILE_INFORMATION inf = {};
			GetFileInformationByHandle(hFile, &inf);

			if (!inf.nFileSizeLow)
			{
				ReportFail(L"Configuration xml file is empty");
			}
			else if (!XmlFile.FileData)
			{
				lbChanged = true;
			}
			else
			{
				if ((memcmp(&inf.ftLastWriteTime, &XmlFile.FileTime, sizeof(XmlFile.FileTime)) != 0)
					|| (inf.nFileSizeLow != XmlFile.FileSize))
				{
					lbChanged = true;
				}
			}

			if (lbChanged)
			{
				DWORD dwRead = 0;
				LPBYTE ptrData = (LPBYTE)calloc(inf.nFileSizeLow+1, sizeof(*ptrData));
				if (!ptrData)
				{
					swprintf_c(szErr, L"Can't allocate %u bytes for xml configuration", inf.nFileSizeLow+1);
					ReportFail(szErr);
				}
				else if (!ReadFile(hFile, ptrData, inf.nFileSizeLow, &dwRead, NULL) || (dwRead != inf.nFileSizeLow))
				{
					swprintf_c(szErr, L"Can't read %u bytes from xml configuration file", inf.nFileSizeLow);
					ReportFail(szErr);
				}
				else
				{
					if (XmlFile.FileData)
						free(XmlFile.FileData);
					XmlFile.FileData = ptrData;
					ptrData = NULL;
					XmlFile.FileSize = dwRead;
				}

				if (ptrData) // Если ошибка, освободить память
					free(ptrData);
			}

			// Done
			CloseHandle(hFile);
		}
		else
		{
			// В первый раз - на отсутствие файла ругнемся
			if (!XmlFile.FileData)
			{
				ReportFail(L"Configuration xml file was not found in the plugin folder");
			}
		}
	}

	// Если файла нет, или его не удалось прочитать - подсунем пустышку
	if (!XmlFile.FileData)
	{
		XmlFile.FileSize = 0;
		XmlFile.FileData = (LPBYTE)calloc(1,sizeof(*XmlFile.FileData));
	}

wrap:
	if (ghXmlNotification && (ghXmlNotification != INVALID_HANDLE_VALUE) && (nNotify == WAIT_OBJECT_0))
	{
		FindNextChangeNotification(ghXmlNotification);
	}
	return lbChanged;
}

#define DllGetFunction(hModule, FunctionName) FunctionName = (FunctionName##_t)GetProcAddress(hModule, #FunctionName)

enum tag_GdiStrMagics
{
	eGdiStr_Decoder = 0x2001,
	eGdiStr_Image   = 0x2002,
};

struct CachedImage
{
	DWORD   nMagic;
	BOOL    bUsed;
	// Имя картинки
	wchar_t sImgName[MAX_PATH]; // i.e. L"img/plugin.png"

	// Хэндлы. CompatibleDC & Bitmap
	HDC     hDc;
	HBITMAP hBmp, hOldBmp;
	int     nWidth, nHeight;

	// Next pointer
	CachedImage *pNextImage;

	bool Init(LPCWSTR apszName, HDC ahDc, UINT anWidth, UINT anHeight, const HDC hScreenDC)
	{
		_ASSERTE(hDc==NULL && hBmp==NULL && hOldBmp==NULL);

		nMagic = eGdiStr_Image;
		bUsed = TRUE;

		wcscpy_c(sImgName, apszName);

		//hDc = ahDc; ahDc = NULL;
		//hBmp = ahBmp; ahBmp = NULL;
		//hOldBmp = ahOldBmp; ahOldBmp = NULL;

		nWidth = anWidth;
		nHeight = anHeight;

		hDc = CreateCompatibleDC(hScreenDC);
		hBmp = CreateCompatibleBitmap(hScreenDC, nWidth, nHeight);
		if (!hDc || !hBmp)
		{
			ReportFail(L"CreateCompatible[DC|Bitmap] failed in CachedImage::Init");
			if (hDc)
			{
				DeleteDC(hDc);
				hDc = NULL;
			}
			if (hBmp)
			{
				DeleteObject(hBmp);
				hBmp = NULL;
			}
			return false;
		}
		hOldBmp = (HBITMAP)SelectObject(hDc, hBmp);

		//BitBlt, MaskBlt, AlphaBlend, TransparentBlt
		BitBlt(hDc, 0, 0, nWidth, nHeight, ahDc, 0, 0, SRCCOPY); // PATINVERT/PATPAINT?

		return true;
	};

	// Деструктор
	void Close()
	{
		sImgName[0] = 0;
		if (hOldBmp && hDc)
		{
			SelectObject(hDc, hOldBmp);
			hOldBmp = NULL;
		}
		if (hDc)
		{
			DeleteDC(hDc);
			hDc = NULL;
		}
		if (hBmp)
		{
			DeleteObject(hBmp);
			hBmp = NULL;
		}

		if (pNextImage)
		{
			pNextImage->Close();
		}

		free(this);
	};
};

struct GDIPlusDecoder
{
	DWORD   nMagic;
	HMODULE hGDIPlus;
	ULONG_PTR gdiplusToken; bool bTokenInitialized;
	HRESULT nLastError;
	BOOL bCoInitialized;
	wchar_t sModulePath[MAX_PATH];
	CachedImage *pImages;


	typedef Gdiplus::Status(WINAPI *GdiplusStartup_t)(ULONG_PTR *token, const Gdiplus::GdiplusStartupInput *input, void /*Gdiplus::GdiplusStartupOutput*/ *output);
	typedef VOID (WINAPI *GdiplusShutdown_t)(ULONG_PTR token);
	typedef Gdiplus::Status(WINAPI *GdipCreateBitmapFromFile_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
	typedef Gdiplus::Status(WINAPI *GdipGetImageWidth_t)(Gdiplus::GpImage *image, UINT *width);
	typedef Gdiplus::Status(WINAPI *GdipGetImageHeight_t)(Gdiplus::GpImage *image, UINT *height);
	typedef Gdiplus::Status(WINAPI *GdipDisposeImage_t)(Gdiplus::GpImage *image);
	//typedef Gdiplus::Status(WINAPI *GdipCreateHBITMAPFromBitmap_t)(Gdiplus::GpBitmap* bitmap, HBITMAP* hbmReturn, ARGB background);
	typedef Gdiplus::Status(WINAPI *GdipDrawImageI_t)(Gdiplus::GpGraphics *graphics, Gdiplus::GpImage *image, INT x, INT y);
	typedef Gdiplus::Status(WINAPI *GdipCreateFromHDC_t)(HDC hdc, Gdiplus::GpGraphics **graphics);
	typedef Gdiplus::Status(WINAPI *GdipDeleteGraphics_t)(Gdiplus::GpGraphics *graphics);
	typedef Gdiplus::Status(WINAPI *GdipGetDC_t)(Gdiplus::GpGraphics* graphics, HDC * hdc);
	typedef Gdiplus::Status(WINAPI *GdipReleaseDC_t)(Gdiplus::GpGraphics* graphics, HDC hdc);
	typedef Gdiplus::Status(WINAPI *GdipGetImageGraphicsContext_t)(Gdiplus::GpImage *image, Gdiplus::GpGraphics **graphics);
	typedef Gdiplus::Status(WINAPI *GdipBitmapConvertFormat_t)(Gdiplus::GpBitmap *pInputBitmap, Gdiplus::PixelFormat format, Gdiplus::DitherType dithertype, Gdiplus::PaletteType palettetype, Gdiplus::ColorPalette *palette, DWORD alphaThresholdPercent);


	GdiplusStartup_t GdiplusStartup;
	GdiplusShutdown_t GdiplusShutdown;
	GdipCreateBitmapFromFile_t GdipCreateBitmapFromFile;
	GdipGetImageWidth_t GdipGetImageWidth;
	GdipGetImageHeight_t GdipGetImageHeight;
	GdipDisposeImage_t GdipDisposeImage;
	//GdipCreateHBITMAPFromBitmap_t GdipCreateHBITMAPFromBitmap;
	GdipDrawImageI_t GdipDrawImageI;
	GdipCreateFromHDC_t GdipCreateFromHDC;
	GdipDeleteGraphics_t GdipDeleteGraphics;
	GdipGetDC_t GdipGetDC;
	GdipReleaseDC_t GdipReleaseDC;
	GdipGetImageGraphicsContext_t GdipGetImageGraphicsContext;
	GdipBitmapConvertFormat_t GdipBitmapConvertFormat;

	bool Init()
	{
		nMagic = eGdiStr_Decoder;
		hGDIPlus = NULL; gdiplusToken = 0; bTokenInitialized = false;
		nLastError = 0;

		if (!GetModuleFileName(ghPluginModule, sModulePath, countof(sModulePath)))
		{
			ReportFail(L"GetModuleFileName failed in GDIPlusDecoder::Init");
			sModulePath[0] = 0;
		}
		else
		{
			wchar_t* pszSlash = wcsrchr(sModulePath, L'\\');
			if (pszSlash)
				pszSlash[1] = 0; // Оставить слеш.
			else
				sModulePath[0] = 0; // Ошибка?
		}

		bool result = false;
		HRESULT hrCoInitialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		bCoInitialized = SUCCEEDED(hrCoInitialized);
		_ASSERTE(bCoInitialized);
		//wchar_t FullPath[MAX_PATH*2+15]; FullPath[0] = 0;
		hGDIPlus = LoadLibraryW(L"GdiPlus.dll");

		if (!hGDIPlus)
		{
			ReportFail(L"GdiPlus.dll not found!");
		}
		else
		{
			DllGetFunction(hGDIPlus, GdiplusStartup);
			DllGetFunction(hGDIPlus, GdiplusShutdown);
			DllGetFunction(hGDIPlus, GdipCreateBitmapFromFile);
			DllGetFunction(hGDIPlus, GdipGetImageWidth);
			DllGetFunction(hGDIPlus, GdipGetImageHeight);
			DllGetFunction(hGDIPlus, GdipDisposeImage);
			//DllGetFunction(hGDIPlus, GdipCreateHBITMAPFromBitmap);
			DllGetFunction(hGDIPlus, GdipDrawImageI);
			DllGetFunction(hGDIPlus, GdipCreateFromHDC);
			DllGetFunction(hGDIPlus, GdipDeleteGraphics);
			DllGetFunction(hGDIPlus, GdipGetDC);
			DllGetFunction(hGDIPlus, GdipReleaseDC);
			DllGetFunction(hGDIPlus, GdipGetImageGraphicsContext);
			DllGetFunction(hGDIPlus, GdipBitmapConvertFormat); // may be absent in old versions of GdiPlus.dll

			if (GdiplusStartup && GdiplusShutdown && GdipCreateBitmapFromFile
				&& GdipGetImageWidth && GdipGetImageHeight
				&& GdipDisposeImage //&& GdipCreateHBITMAPFromBitmap
				&& GdipDrawImageI && GdipCreateFromHDC && GdipDeleteGraphics
				&& GdipGetDC && GdipReleaseDC && GdipGetImageGraphicsContext
				)
			{
				Gdiplus::GdiplusStartupInput gdiplusStartupInput;
				Gdiplus::Status lRc = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
				result = (lRc == Gdiplus::Ok);

				if (!result)
				{
					nLastError = GetLastError();
					GdiplusShutdown(gdiplusToken); bTokenInitialized = false;
					ReportFail(L"GdiplusStartup failed");
				}
				else
				{
					bTokenInitialized = true;
				}
			}
			else
			{
				ReportFail(L"Functions not found in GdiPlus.dll");
			}

			if (!result)
				FreeLibrary(hGDIPlus);
		}

		return result;
	};

	// При использовании в фаре GdiPlus иногда может зависать на FreeLibrary.
	// Причины пока не выяснены
	static DWORD WINAPI FreeThreadProc(LPVOID lpParameter)
	{
		struct GDIPlusDecoder* p = (struct GDIPlusDecoder*)lpParameter;

		if (p && p->hGDIPlus)
		{
			FreeLibrary(p->hGDIPlus);
		}

		return 0;
	}

	void Close()
	{
		if (pImages)
		{
			pImages->Close();
			pImages = NULL;
		}

		if (bTokenInitialized)
		{
			GdiplusShutdown(gdiplusToken);
			bTokenInitialized = false;
		}

		if (hGDIPlus)
		{
			//FreeLibrary(hGDIPlus);
			DWORD nFreeTID = 0, nWait = 0, nErr = 0;
			HANDLE hFree = apiCreateThread(FreeThreadProc, this, &nFreeTID, "Bg::FreeThreadProc");

			if (!hFree)
			{
				nErr = GetLastError();
				FreeLibrary(hGDIPlus);
			}
			else
			{
				nWait = WaitForSingleObject(hFree, 5000);

				if (nWait != WAIT_OBJECT_0)
					apiTerminateThread(hFree, 100);

				CloseHandle(hFree);
			}

			UNREFERENCED_PARAMETER(nErr);
			hGDIPlus = NULL;
		}

		if (bCoInitialized)
		{
			bCoInitialized = FALSE;
			CoUninitialize();
		}

		free(this);
	};

	CachedImage* GetImage(LPCWSTR asRelativeName)
	{
		if (!asRelativeName || !*asRelativeName)
			return NULL;

		size_t cchFullName = MAX_PATH*2;
		wchar_t* szFullName = (wchar_t*)malloc(cchFullName*sizeof(*szFullName));
		size_t cchName = MAX_PATH;
		wchar_t* szName = (wchar_t*)malloc(cchName*sizeof(*szName));
		size_t cchError = MAX_PATH*3;
		wchar_t* szError = (wchar_t*)malloc(cchError*sizeof(*szError));
		wchar_t* psz = NULL;
		Gdiplus::Status lRc;
		Gdiplus::GpBitmap *bmp = NULL;
		Gdiplus::GpGraphics *gr = NULL;
		CachedImage *pI = NULL, *pLast = NULL;
		UINT nW = 0, nH = 0;
		HDC hDc = NULL, hCompDc = NULL;
		HBITMAP hBmp = NULL, hOldBmp = NULL;
		const HDC hScreenDC = GetDC(0);

		if (!szFullName || !szName || !szError)
		{
			ReportFail(L"Can't allocate memory for szFullName");
			goto wrap;
		}

		if (*asRelativeName == L'\\' || *asRelativeName == L'/')
			asRelativeName++;
		// Заменить "/" на "\"
		_wcscpy_c(szName, cchName, asRelativeName);
		psz = wcschr(szName, L'/');
		while (psz)
		{
			*psz = L'\\';
			psz = wcschr(psz+1, L'/');
		}
		// Lowercase, для сравнения
		CharLowerBuff(szName, lstrlen(szName));

		// Может уже есть?
		pI = pImages;
		while (pI != NULL)
		{
			pLast = pI;
			if (pI->bUsed && lstrcmp(pI->sImgName, szName) == 0)
			{
				goto wrap;
			}
			pI = pI->pNextImage;
		}


		// Загрузить из файла
		_ASSERTE(pI == NULL);
		pI = NULL;

		_wcscpy_c(szFullName, cchFullName, sModulePath);
		_wcscat_c(szFullName, cchFullName, szName);


		lRc = GdipCreateBitmapFromFile(szFullName, &bmp);
		if (lRc != Gdiplus::Ok)
		{
			swprintf_c(szError, cchError/*#SECURELEN*/, L"Can't load image, Err=%i\n%s", (int)lRc, szFullName);
			ReportFail(szError);
			_ASSERTE(bmp == NULL);
			bmp = NULL;
			goto wrap;
		}

		if (GdipBitmapConvertFormat)
		{
			Gdiplus::ColorPalette pal[2];
			pal[0].Flags = Gdiplus::PaletteFlagsGrayScale;
			pal[0].Count = 2;
			pal[0].Entries[0] = (DWORD)0;
			pal[0].Entries[1] = (DWORD)-1;
			GdipBitmapConvertFormat(bmp, PixelFormat1bppIndexed, Gdiplus::DitherTypeSolid, Gdiplus::PaletteTypeFixedBW, pal, 0);
		}

		if (((lRc = GdipGetImageWidth(bmp, &nW)) != Gdiplus::Ok) || (GdipGetImageHeight(bmp, &nH) != Gdiplus::Ok)
			|| (nW < 1) || (nH < 1))
		{
			swprintf_c(szError, cchError/*#SECURELEN*/, L"GetImageSize failed, Err=%i\n%s", (int)lRc, szFullName);
			ReportFail(szError);
			goto wrap;
		}

		hCompDc = CreateCompatibleDC(hScreenDC);
		hBmp = CreateCompatibleBitmap(hScreenDC, nW, nH);
		if (!hCompDc || !hBmp)
		{
			ReportFail(L"CreateCompatible[DC|Bitmap] failed");
			goto wrap;
		}
		hOldBmp = (HBITMAP)SelectObject(hCompDc, hBmp);

		lRc = GdipCreateFromHDC(hCompDc, &gr);
		if (lRc != Gdiplus::Ok)
		{
			swprintf_c(szError, cchError/*#SECURELEN*/, L"GdipCreateFromHDC failed, Err=%i\n%s", (int)lRc, szFullName);
			ReportFail(szError);
			goto wrap;
		}

		lRc = GdipDrawImageI(gr, bmp, 0, 0);
		if (lRc != Gdiplus::Ok)
		{
			swprintf_c(szError, cchError/*#SECURELEN*/, L"GdipDrawImageI failed, Err=%i\n%s", (int)lRc, szFullName);
			ReportFail(szError);
			goto wrap;
		}

		//lRc = GdipGetImageGraphicsContext(bmp, &gr);
		//if (lRc != Gdiplus::Ok || !gr)
		//{
		//	swprintf_c(szError, cchError/*#SECURELEN*/, L"GdipGetImageGraphicsContext failed, Err=%i\n%s", (int)lRc, szFullName);
		//	ReportFail(szError);
		//	goto wrap;
		//}

		lRc = GdipGetDC(gr, &hDc);
		if (lRc != Gdiplus::Ok || !hDc)
		{
			swprintf_c(szError, cchError/*#SECURELEN*/, L"GdipGetDC failed, Err=%i\n%s", (int)lRc, szFullName);
			ReportFail(szError);
			goto wrap;
		}
		#ifdef _DEBUG
		//SelectObject(hDc, (HPEN)GetStockObject(WHITE_PEN));
		//MoveToEx(hDc, 0,0, NULL);
		//LineTo(hDc, nW, nH);
		/*{
		RECT rcDbg = {nW/2,nH/2, nW, nH};
		FillRect(hDc, &rcDbg, (HBRUSH)GetStockObject(WHITE_BRUSH));
		//BitBlt(hScreenDC, 0, 0, nW, nH, hDc, 0, 0, SRCCOPY); // PATINVERT/PATPAINT?
		}*/
		#endif

		// OK, теперь можно запомнить в кеше
		if (!pLast)
		{
			_ASSERTE(pImages == NULL);
			pImages = (CachedImage*)calloc(1,sizeof(CachedImage));
			pI = pImages;
		}
		else
		{
			pI = (CachedImage*)calloc(1,sizeof(CachedImage));
			pLast->pNextImage = pI;
		}
		if (!pI)
		{
			ReportFail(L"Can't allocate memory for CachedImage");
			goto wrap;
		}
		// Done
		if (!pI->Init(szName, hDc, nW, nH, hScreenDC))
		{
			if (pLast)
				pLast->pNextImage = NULL;
			if (pI == pImages)
				pImages = NULL;
			pI->Close();
			pI = NULL;
			goto wrap;
		}

	wrap:
		if (szFullName)
			free(szFullName);
		if (szName)
			free(szName);
		if (szError)
			free(szError);
		if (hDc && gr)
			GdipReleaseDC(gr, hDc);
		if (gr)
			GdipDeleteGraphics(gr);
		if (hCompDc && hOldBmp)
			SelectObject(hCompDc, hOldBmp);
		if (hCompDc)
			DeleteDC(hCompDc);
		if (hBmp)
			DeleteObject(hBmp);
		if (bmp)
			GdipDisposeImage(bmp);
		if (hScreenDC)
			ReleaseDC(0, hScreenDC);
		return pI;
	}
};

GDIPlusDecoder* gpDecoder = NULL;

bool GdipInit()
{
	if (gbGdiPlusInitialized)
		return true; // уже

	_ASSERTE(XmlFile.cr && XmlFile.cr->IsInitialized());

	// Загрузить его содержимое
	CheckXmlFile(true);

	// Инициализация XMLLite
	ghXmlLite = LoadLibrary(L"xmllite.dll");
	if (!ghXmlLite)
	{
		ReportFail(L"xmllite.dll not found");
	}
	else
	{
		gfCreateXmlReader = (CreateXmlReader_t)GetProcAddress(ghXmlLite, "CreateXmlReader");
		if (!gfCreateXmlReader)
		{
			ReportFail(L"function CreateXmlReader in the xmllite.dll not found");
			FreeLibrary(ghXmlLite);
			ghXmlLite = NULL;
		}
	}

	gpDecoder = (GDIPlusDecoder*)calloc(1,sizeof(*gpDecoder));
	if (!gpDecoder->Init())
	{
		gpDecoder->Close();
		gpDecoder = NULL;
	}

	gbGdiPlusInitialized = true;
	return true;
}

void GdipDone()
{
	if (!gbGdiPlusInitialized)
		return; // Не выполнялось

	MSectionLockSimple CS;

	if (XmlFile.cr)
	{
		CS.Lock(XmlFile.cr);
	}

	if (ghXmlLite)
	{
		FreeLibrary(ghXmlLite);
		ghXmlLite = NULL;
	}
	gfCreateXmlReader = NULL;

	if (gpszXmlFile)
	{
		free(gpszXmlFile);
		gpszXmlFile = NULL;
	}
	if (gpszXmlFolder)
	{
		free(gpszXmlFolder);
		gpszXmlFolder = NULL;
	}

	if (XmlFile.FileData)
	{
		free(XmlFile.FileData);
		XmlFile.FileData = NULL;
	}
	XmlFile.FileSize = 0;

	if (ghXmlNotification && (ghXmlNotification != INVALID_HANDLE_VALUE))
	{
		FindCloseChangeNotification(ghXmlNotification);
		ghXmlNotification = NULL;
	}

	if (XmlFile.cr)
	{
		CS.Unlock();
		SafeDelete(XmlFile.cr);
	}

	if (gpDecoder)
	{
		gpDecoder->Close();
		gpDecoder = NULL;
	}

	gbGdiPlusInitialized = false;
}

bool CompareNames(LPCWSTR asMaskList, LPWSTR asPath)
{
	if (!asMaskList || !asPath)
		return false;
	if ((*asMaskList && !*asPath) || (*asPath && !*asMaskList))
		return false;

	wchar_t szMask[MAX_PATH];
	while (*asMaskList)
	{
		LPCWSTR pszSep = wcschr(asMaskList, L'|');
		if (pszSep)
		{
			int nLen = (int)std::min((INT_PTR)countof(szMask)-1,(INT_PTR)(pszSep-asMaskList));
			lstrcpyn(szMask, asMaskList, nLen+1);
			szMask[nLen] = 0;
		}
		else
		{
			lstrcpyn(szMask, asMaskList, countof(szMask));
		}

		wchar_t* pszSlash = wcschr(szMask, L'/');
		while (pszSlash)
		{
			*pszSlash = L'\\';
			pszSlash = wcschr(pszSlash+1, L'/');
		}

		if (FMatch(szMask, asPath))
			return true;

		if (!pszSep)
			break;
		asMaskList = pszSep+1;
	}

	return false;
}


struct RGBColor
{
	union
	{
		COLORREF clr;
		struct
		{
			BYTE R, G, B, Dummy;
		};
	};
};


void ParseColors(LPCWSTR asColors, BOOL abSwap/*RGB->COLORREF*/, COLORREF (&crValues)[32], int &nCount)
{
	int i = 0;
	wchar_t* pszEnd = NULL;

	while (asColors && *asColors)
	{
		if (abSwap)
		{
			// Нам нужен COLORREF - это перевернутые R&B
			RGBColor rgb = {}, bgr;
			if (*asColors == L'#')
				bgr.clr = wcstoul(asColors+1, &pszEnd, 16) & 0xFFFFFF;
			else
				bgr.clr = wcstoul(asColors, &pszEnd, 10) & 0xFFFFFF;
			rgb.R = bgr.B;
			rgb.G = bgr.G;
			rgb.B = bgr.R;
			crValues[i++] = rgb.clr;
		}
		else
		{
			RGBColor rgb;
			if (*asColors == L'#')
				rgb.clr = wcstoul(asColors+1, &pszEnd, 16) & 0xFFFFFF;
			else
				rgb.clr = wcstoul(asColors, &pszEnd, 10) & 0xFFFFFF;
			crValues[i++] = rgb.clr;
		}

		if (pszEnd && *pszEnd == L'|')
			asColors = pszEnd + 1;
		else
			break;
	}

	nCount = i;

	//while (i < countof(crValues))
	//{
	//	crValues[i++] = (COLORREF)-1;
	//}
}

int FillPanelParams(PaintBackgroundArg* pBk, BkPanelInfo *pPanel, DrawInfo *pDraw)
{
	int iFound = 0;
	MStream strm;
	IXmlReader* pXmlReader = NULL;
	HRESULT hr = S_OK;

	//int WindowType = -1;
	struct ChunkInfo
	{
		enum {
			ci_WindowNone = 0,
			ci_WindowPanels = 1,
			ci_WindowEditor = 2,
			ci_WindowViewer = 3,
		} WindowType;

		enum {
			ci_PanelNone = 0,
			ci_PanelDrive = 1,
			ci_PanelPlugin = 2,
			ci_PanelTree = 3,
			ci_PanelQView = 4,
			ci_PanelInfo = 5,
		} PanelType;

		BOOL CondFailed;

		bool Valid()
		{
			if (WindowType == ci_WindowNone)
				return false;

			if (WindowType == ci_WindowPanels)
			{
				if (PanelType == ci_PanelNone)
					return false;
			}

			return true;
		}

		bool Equal(ChunkInfo* pTest)
		{
			if (pTest->WindowType == ci_WindowNone || pTest->WindowType != this->WindowType)
				return false;

			if (pTest->WindowType == ci_WindowPanels)
			{
				if (pTest->PanelType == ci_PanelNone || pTest->PanelType != this->PanelType)
					return false;
			}

			if (CondFailed)
				return false;

			return true;
		}
	};

	bool bFound = false;
	ChunkInfo Test = {ChunkInfo::ci_WindowPanels, ChunkInfo::ci_PanelNone};
	ChunkInfo Panel = {ChunkInfo::ci_WindowPanels, ChunkInfo::ci_PanelNone};
	MSectionLockSimple CS;

	CEStr pszFormat(pPanel->szFormat ? pPanel->szFormat : L"");
	size_t nMaxLen = lstrlen(pPanel->szCurDir ? pPanel->szCurDir : L"");
	LPWSTR pszPath = (wchar_t*)malloc((nMaxLen+2)*sizeof(*pszPath));
	_wcscpy_c(pszPath, nMaxLen+1, pPanel->szCurDir ? pPanel->szCurDir : L"");
	if (nMaxLen > 1 && pszPath[nMaxLen-1] != L'\\')
	{
		pszPath[nMaxLen] = L'\\';
		pszPath[nMaxLen+1] = 0;
	}
	BOOL bTempPath = FALSE;

	wchar_t szTemp[MAX_PATH+1];

	// Проверка, путь - %TEMP%?
	if (*pszPath)
	{
		if (!bTempPath)
		{
			lstrcpyn(szTemp, pszPath, countof(szTemp));
			CharUpperBuff(szTemp, lstrlen(szTemp));
			if (wcsstr(szTemp, L"\\TEMP\\") != NULL)
				bTempPath = TRUE;
		}
		if (!bTempPath && GetTempPath(countof(szTemp)-1, szTemp))
		{
			szTemp[countof(szTemp)-1] = 0;
			if (lstrcmpi(pszPath, szTemp) == 0)
				bTempPath = TRUE;
		}
	}


	pDraw->Flags = 0;


	if (pBk->Place == pbp_Panels)
	{
		Panel.WindowType = ChunkInfo::ci_WindowPanels;
		if (pPanel->bPlugin)
		{
			Panel.PanelType = ChunkInfo::ci_PanelPlugin;
		}
		else if (pPanel->nPanelType == PTYPE_FILEPANEL)
		{
			Panel.PanelType = ChunkInfo::ci_PanelDrive;
		}
		else
		{
			switch (pPanel->nPanelType)
			{
			case PTYPE_TREEPANEL:
				Panel.PanelType = ChunkInfo::ci_PanelTree;
				break;
			case PTYPE_QVIEWPANEL:
				Panel.PanelType = ChunkInfo::ci_PanelQView;
				break;
			case PTYPE_INFOPANEL:
				Panel.PanelType = ChunkInfo::ci_PanelInfo;
				break;
			}
		}
	}
	else if (pBk->Place == pbp_Editor || pBk->Place == pbp_Viewer)
	{
		Panel.WindowType = (pBk->Place == pbp_Editor) ? ChunkInfo::ci_WindowEditor : ChunkInfo::ci_WindowViewer;
	}
	else
	{
		_ASSERTE(pBk->Place == pbp_Panels || pBk->Place == pbp_Editor || pBk->Place == pbp_Viewer);
		goto wrap;
	}


	CS.Lock(XmlFile.cr);
	if (!XmlFile.FileData || !XmlFile.FileSize)
	{
		CS.Unlock();
		goto wrap;
	}
	strm.SetData(XmlFile.FileData, XmlFile.FileSize);
	CS.Unlock();

    hr = gfCreateXmlReader(IID_IXmlReader, (void**)&pXmlReader, NULL);
    if (SUCCEEDED(hr))
    {
        hr = pXmlReader->SetInput(&strm);

		// read through the stream
		XmlNodeType nodeType;
		while (!iFound && (S_OK == (hr = pXmlReader->Read(&nodeType))))
		{
			PCWSTR pszName = NULL;
			PCWSTR pszValue = NULL;
			#ifdef _DEBUG
			PCWSTR pszPrefix = NULL;
			#endif
			PCWSTR pszAttrName = NULL;
			PCWSTR pszAttrValue = NULL;

			switch (nodeType)
			{
			case XmlNodeType_EndElement:
				hr = pXmlReader->GetLocalName(&pszName, NULL);
				if (SUCCEEDED(hr))
				{
					bool bEndWindow = (lstrcmpi(pszName, L"window") == 0);
					if (bEndWindow || (lstrcmpi(pszName, L"panel") == 0))
					{
						if (bEndWindow)
							Test.WindowType = ChunkInfo::ci_WindowNone;
						Test.PanelType = ChunkInfo::ci_PanelNone;
						Test.CondFailed = FALSE;
						if (bFound)
							iFound = TRUE;
					}
				}
				break;

			case XmlNodeType_Element:
				hr = pXmlReader->GetLocalName(&pszName, NULL);
				if (FAILED(hr))
				{
					continue;
				}

				if (lstrcmpi(pszName, L"window") == 0)
				{
					// Сначала - сброс
					Test.WindowType = ChunkInfo::ci_WindowNone;
					// Смотрим атрибуты
					hr = pXmlReader->MoveToFirstAttribute();
					if (SUCCEEDED(hr))
					{
						do {
							#ifdef _DEBUG
							hr = pXmlReader->GetPrefix(&pszPrefix, NULL);
							#endif
							if (SUCCEEDED(hr = pXmlReader->GetLocalName(&pszAttrName, NULL)))
							{
								if (SUCCEEDED(hr = pXmlReader->GetValue(&pszAttrValue, NULL)))
								{
									if (lstrcmpi(pszAttrName, L"type") == 0)
									{
										if (lstrcmpi(pszAttrValue, L"PANELS") == 0 || lstrcmpi(pszAttrValue, L"PANEL") == 0)
											Test.WindowType = ChunkInfo::ci_WindowPanels;
										else if (lstrcmpi(pszAttrValue, L"EDITOR") == 0)
											Test.WindowType = ChunkInfo::ci_WindowEditor;
										else if (lstrcmpi(pszAttrValue, L"VIEWER") == 0)
											Test.WindowType = ChunkInfo::ci_WindowViewer;
										// Check
										if (Test.WindowType != Panel.WindowType)
											Test.CondFailed = TRUE;
									}
								}
							}
						} while (!Test.CondFailed && (hr = pXmlReader->MoveToNextAttribute()) == S_OK);
					}
				}
				else if (lstrcmpi(pszName, L"panel") == 0)
				{
					if ((Test.WindowType == ChunkInfo::ci_WindowNone) || (Test.WindowType != Panel.WindowType))
					{
						Test.CondFailed = TRUE;
					}
					else
					{
						// Сначала - сброс
						Test.PanelType = ChunkInfo::ci_PanelNone;
						// Смотрим атрибуты
						hr = pXmlReader->MoveToFirstAttribute();
						if (SUCCEEDED(hr))
						{
							do {
								#ifdef _DEBUG
								hr = pXmlReader->GetPrefix(&pszPrefix, NULL);
								#endif
								if (SUCCEEDED(hr = pXmlReader->GetLocalName(&pszAttrName, NULL)))
								{
									if (SUCCEEDED(hr = pXmlReader->GetValue(&pszAttrValue, NULL)))
									{
										if (lstrcmpi(pszAttrName, L"type") == 0)
										{
											if (lstrcmpi(pszAttrValue, L"DRIVE") == 0)
												Test.PanelType = ChunkInfo::ci_PanelDrive;
											else if (lstrcmpi(pszAttrValue, L"PLUGIN") == 0)
												Test.PanelType = ChunkInfo::ci_PanelPlugin;
											else if (lstrcmpi(pszAttrValue, L"TREE") == 0)
												Test.PanelType = ChunkInfo::ci_PanelTree;
											else if (lstrcmpi(pszAttrValue, L"QVIEW") == 0)
												Test.PanelType = ChunkInfo::ci_PanelQView;
											else if (lstrcmpi(pszAttrValue, L"INFO") == 0)
												Test.PanelType = ChunkInfo::ci_PanelInfo;
											else
												Test.PanelType = ChunkInfo::ci_PanelNone;
											// Check
											if (Test.PanelType != Panel.PanelType)
												Test.CondFailed = TRUE;
										}
										else if (lstrcmpi(pszAttrName, L"drivetype") == 0)
										{
											DWORD nDriveType = DRIVE_UNKNOWN;
											if (lstrcmpi(pszAttrValue, L"NO_ROOT_DIR") == 0)
												nDriveType = DRIVE_NO_ROOT_DIR;
											else if (lstrcmpi(pszAttrValue, L"REMOVABLE") == 0)
												nDriveType = DRIVE_REMOVABLE;
											else if (lstrcmpi(pszAttrValue, L"FIXED") == 0)
												nDriveType = DRIVE_FIXED;
											else if (lstrcmpi(pszAttrValue, L"REMOTE") == 0)
												nDriveType = DRIVE_REMOTE;
											else if (lstrcmpi(pszAttrValue, L"CDROM") == 0)
												nDriveType = DRIVE_CDROM;
											else if (lstrcmpi(pszAttrValue, L"RAMDISK") == 0)
												nDriveType = DRIVE_RAMDISK;
											else
												nDriveType = DRIVE_UNKNOWN;
											// Check
											if (nDriveType != pDraw->nDriveType)
												Test.CondFailed = TRUE;
										}
										else if (lstrcmpi(pszAttrName, L"pathtype") == 0)
										{
											if (lstrcmpi(pszAttrValue, L"TEMP") == 0)
											{
												if (!bTempPath)
													Test.CondFailed = TRUE;
											}
										}
										else if (lstrcmpi(pszAttrName, L"pathmatch") == 0)
										{
											if (!CompareNames(pszAttrValue, pszPath))
												Test.CondFailed = TRUE;
										}
										else if (lstrcmpi(pszAttrName, L"format") == 0)
										{
											if (!CompareNames(pszAttrValue, pszFormat.data()))
												Test.CondFailed = TRUE;
										}
									}
								}
							} while (!Test.CondFailed && (hr = pXmlReader->MoveToNextAttribute()) == S_OK);
						}
					}
				}
				else if (Test.Valid() && Test.Equal(&Panel))
				{
					if (lstrcmpi(pszName, L"disabled") == 0)
					{
						pDraw->Flags |= DrawInfo::dif_Disabled;
						iFound = TRUE;
						break;
					}

					// Блок подошел, больше не проверяем
					bFound = true;
					// Смотрим, что там настроили
					if (lstrcmpi(pszName, L"color") == 0)
					{
						// Смотрим атрибуты
						hr = pXmlReader->MoveToFirstAttribute();
						if (SUCCEEDED(hr))
						{
							do {
								#ifdef _DEBUG
								hr = pXmlReader->GetPrefix(&pszPrefix, NULL);
								#endif
								if (SUCCEEDED(hr = pXmlReader->GetLocalName(&pszAttrName, NULL)))
								{
									if (SUCCEEDED(hr = pXmlReader->GetValue(&pszAttrValue, NULL)))
									{
										if (lstrcmpi(pszAttrName, L"rgb") == 0)
										{
											// Нам нужен COLORREF - это перевернутые R&B
											ParseColors(pszAttrValue, TRUE/*RGB->COLORREF*/, pDraw->crBack, pDraw->nBackCount);
											pDraw->Flags |= DrawInfo::dif_BackFilled;
										}
										else if (lstrcmpi(pszAttrName, L"rgb_dark") == 0)
										{
											// Нам нужен COLORREF - это перевернутые R&B
											ParseColors(pszAttrValue, TRUE/*RGB->COLORREF*/, pDraw->crDark, pDraw->nDarkCount);
											pDraw->Flags |= DrawInfo::dif_DarkFilled;
										}
										else if (lstrcmpi(pszAttrName, L"rgb_light") == 0)
										{
											// Нам нужен COLORREF - это перевернутые R&B
											ParseColors(pszAttrValue, TRUE/*RGB->COLORREF*/, pDraw->crLight, pDraw->nLightCount);
											pDraw->Flags |= DrawInfo::dif_LightFilled;
										}
										else if (lstrcmpi(pszAttrName, L"bgr") == 0)
										{
											ParseColors(pszAttrValue, FALSE/*RGB->COLORREF*/, pDraw->crBack, pDraw->nBackCount);
											pDraw->Flags |= DrawInfo::dif_BackFilled;
										}
										else if (lstrcmpi(pszAttrName, L"bgr_dark") == 0)
										{
											ParseColors(pszAttrValue, FALSE/*RGB->COLORREF*/, pDraw->crDark, pDraw->nDarkCount);
											pDraw->Flags |= DrawInfo::dif_DarkFilled;
										}
										else if (lstrcmpi(pszAttrName, L"bgr_light") == 0)
										{
											ParseColors(pszAttrValue, FALSE/*RGB->COLORREF*/, pDraw->crLight, pDraw->nLightCount);
											pDraw->Flags |= DrawInfo::dif_LightFilled;
										}
										else if (lstrcmpi(pszAttrName, L"shift") == 0)
										{
											if (lstrcmpi(pszAttrValue, L"yes") == 0)
												pDraw->Flags |= DrawInfo::dif_ShiftBackColor;
										}
									}
								}
							} while ((hr = pXmlReader->MoveToNextAttribute()) == S_OK);
						}
					}
					else if (lstrcmpi(pszName, L"space") == 0)
					{
						// Смотрим атрибуты
						hr = pXmlReader->MoveToFirstAttribute();
						if (SUCCEEDED(hr))
						{
							do {
								#ifdef _DEBUG
								hr = pXmlReader->GetPrefix(&pszPrefix, NULL);
								#endif
								if (SUCCEEDED(hr = pXmlReader->GetLocalName(&pszAttrName, NULL)))
								{
									if (SUCCEEDED(hr = pXmlReader->GetValue(&pszAttrValue, NULL)))
									{
										if (lstrcmpi(pszAttrName, L"type") == 0)
										{
											if (lstrcmpi(pszAttrValue, L"large") == 0)
												pDraw->nSpaceBar = DrawInfo::dib_Large;
											else if (lstrcmpi(pszAttrValue, L"off") == 0)
												pDraw->nSpaceBar = DrawInfo::dib_Off;
											else //if (lstrcmpi(pszAttrValue, L"small") == 0)
												pDraw->nSpaceBar = DrawInfo::dib_Small;
										}
									}
								}
							} while ((hr = pXmlReader->MoveToNextAttribute()) == S_OK);
						}
					}
					else if (lstrcmpi(pszName, L"title") == 0)
					{
						//TODO: Обработка функций
						hr = pXmlReader->Read(&nodeType);
						if (SUCCEEDED(hr) && (nodeType == XmlNodeType_Text))
						{
							hr = pXmlReader->GetValue(&pszValue, NULL);
							//lstrcpyn(pDraw->szText, pszValue, countof(pDraw->szPic));

							// szTemp
							pDraw->szText[0] = 0;
							szTemp[0] = 0;
							wchar_t* pszDst = pDraw->szText;
							wchar_t* pszEnd = pDraw->szText+countof(pDraw->szText)-1;
							wchar_t* pszTemp = szTemp;
							wchar_t* pszTempEnd = szTemp+countof(szTemp)-1;
							bool bPlain = false;
							while (TRUE)
							{
								if (*pszValue == L'"')
								{
									if (!bPlain)
									{
										bPlain = true;
									}
									else if (pszValue[1] == L'"')
									{
										pszValue++;
										*(pszDst++) = L'"';
									}
									else
									{
										bPlain = false;
									}
									pszValue++;
								}
								else if (bPlain)
								{
									*(pszDst++) = *(pszValue++);
									if (pszDst >= pszEnd)
										break;
								}
								else if (!*pszValue || wcschr(L" \t\r\n+", *pszValue))
								{
									if (*szTemp)
									{
										*pszTemp = 0;
										LPCWSTR pszVar = szTemp;
										if (lstrcmpi(szTemp, L"VOLUME") == 0)
											lstrcpyn(szTemp, pDraw->szVolume, countof(szTemp));
										else if (lstrcmpi(szTemp, L"VOLUMESIZE") == 0)
											lstrcpyn(szTemp, pDraw->szVolumeSize, countof(szTemp));
										else if (lstrcmpi(szTemp, L"VOLUMEFREE") == 0)
											lstrcpyn(szTemp, pDraw->szVolumeFree, countof(szTemp));
										else if (lstrcmpi(szTemp, L"PANELFORMAT") == 0)
											lstrcpyn(szTemp, pPanel->szFormat ? pPanel->szFormat : L"", countof(szTemp));

										int nTempLen = lstrlen(pszVar);
										if ((pszDst + nTempLen) >= pszEnd)
											break;
										lstrcpyn(pszDst, pszVar, nTempLen+1);
										pszDst += nTempLen;
										pszTemp = szTemp;
										*szTemp = 0;
									}
									if (!*pszValue)
										break;
									pszValue++;
								}
								else
								{
									*(pszTemp++) = *(pszValue++);
									if (pszTemp >= pszTempEnd)
										break;
								}
							}
							while ((pszDst > pDraw->szText) && (*(pszDst-1) == L' '))
								pszDst--;
							*pszDst = 0; // ASCIIZ

							pDraw->Flags |= DrawInfo::dif_TextFilled;
						}
					}
					else if (lstrcmpi(pszName, L"img") == 0)
					{
						if (SUCCEEDED(hr = pXmlReader->MoveToAttributeByName(L"ref", NULL)))
						{
							if (SUCCEEDED(hr = pXmlReader->GetValue(&pszAttrValue, NULL)))
							{
								lstrcpyn(pDraw->szPic, pszAttrValue, countof(pDraw->szPic));
								pDraw->Flags |= DrawInfo::dif_PicFilled;
							}
						}
					}
				}
				break;
			default:
				; // GCC warning elimination
			}
		} // end - while (!iFound && (S_OK == (hr = pXmlReader->Read(&nodeType))))
		pXmlReader->Release();
    }

wrap:
	if (pszPath)
		free(pszPath);
    return iFound;
}


// HSB
#if 0
struct HSBColor
{
	double Hue, Saturnation, Brightness;
};

struct RGBColor
{
	union
	{
		COLORREF clr;
		struct
		{
			BYTE Red, Green, Blue, Dummy;
		};
	};
};

void COLORREF2HSB(COLORREF rgbclr, HSBColor& hsb)
{
	double minRGB, maxRGB, Delta;
    double H, s, b;

    RGBColor rgb; rgb.clr = rgbclr;

	H = 0.0;
	minRGB = std::min(std::min(rgb.Red, rgb.Green), rgb.Blue);
	maxRGB = std::max(std::max(rgb.Red, rgb.Green), rgb.Blue);
	Delta = (maxRGB - minRGB);
	b = maxRGB;

	if (maxRGB != 0.0)
	{
		s = 255.0 * Delta / maxRGB;
	}
	else
	{
		s = 0.0;
	}

	if (s != 0.0)
	{
		if (rgb.Red == maxRGB)
		{
			H = (rgb.Green - rgb.Blue) / Delta;
		}
		else if (rgb.Green == maxRGB)
		{
			H = 2.0 + (rgb.Blue - rgb.Red) / Delta;
		}
		else if (rgb.Blue == maxRGB)
		{
			H = 4.0 + (rgb.Red - rgb.Green) / Delta;
		}
	}
	else
	{
		H = -1.0;
	}

	H = H * 60 ;
	if (H < 0.0)
	{
		H = H + 360.0;
	}

	hsb.Hue = H;
	hsb.Saturnation = s * 100 / 255;
	hsb.Brightness = b * 100 / 255;
}

inline BYTE ClrPart(double v)
{
	return (v < 0) ? 0 : (v > 1) ? 255 : (int)(v*255);
}

inline double Abs(double v)
{
	return (v < 0) ? (-v) : v;
}

void HSB2COLORREF(const HSBColor& hsb, COLORREF& rgbclr)
{
	RGBColor rgb = {};
	double C, H1, m, X;

	//C = (1 - Abs(2*(hsb.Brightness/100.0) - 1)) * (hsb.Saturnation/100.0);
	C = (hsb.Brightness/100.0) * (hsb.Saturnation/100.0);
	H1 = (hsb.Hue / 60);
	X = C*(1 - Abs((H1 - 2*(int)(H1 / 2)) - 1));
	m = 0;// (hsb.Brightness/100.0) - (C / 2); -- пока без доп.яркости

	if ((hsb.Brightness == 0) || (hsb.Hue < 0 || hsb.Hue >= 360))
	{
		rgb.clr = 0;
	}
	else if (hsb.Saturnation == 0)
	{
		rgb.Red = rgb.Green = rgb.Blue = ClrPart(hsb.Brightness/100);
	}
	else
	{
  //       double r=0,g=0,b=0;
  //       double temp1,temp2;

  //             temp2 = (((hsb.Brightness/100)<=0.5)
		//			? (hsb.Brightness/100)*(1.0+(hsb.Saturnation/100))
		//			: (hsb.Brightness/100)+(hsb.Saturnation/100)-((hsb.Brightness/100)*(hsb.Saturnation/100)));

  //             temp1 = 2.0*(hsb.Brightness/100)-temp2;

  //

  //             double t3[]={hsb.Hue/360+1.0/3.0,hsb.Hue/360,(hsb.Hue/360)-1.0/3.0};

  //             double clr[]={0,0,0};

  //             for(int i=0;i<3;i++)

  //             {

  //                if(t3[i]<0)

  //                   t3[i]+=1.0;

  //                if(t3[i]>1)

  //                   t3[i]-=1.0;

  //

  //                if(6.0*t3[i] < 1.0)

  //                   clr[i]=temp1+(temp2-temp1)*t3[i]*6.0;

  //                else if(2.0*t3[i] < 1.0)

  //                   clr[i]=temp2;

  //                else if(3.0*t3[i] < 2.0)

  //                   clr[i]=(temp1+(temp2-temp1)*((2.0/3.0)-t3[i])*6.0);

  //                else

  //                   clr[i]=temp1;

  //             }

  //             r=clr[0];

  //             g=clr[1];

  //             b=clr[2];

		//rgb.Red = ClrPart(255*r); rgb.Green = ClrPart(255*g); rgb.Blue = ClrPart(255*b);

		if (0 <= H1 && H1 < 1)
		{
			rgb.Red = ClrPart(C+m); rgb.Green = ClrPart(X+m); rgb.Blue = ClrPart(0+m);
		}
		else if (1 <= H1 && H1 < 2)
		{
			rgb.Red = ClrPart(X+m); rgb.Green = ClrPart(C+m); rgb.Blue = ClrPart(0+m);
		}
		else if (2 <= H1 && H1 < 3)
		{
			rgb.Red = ClrPart(0+m); rgb.Green = ClrPart(C+m); rgb.Blue = ClrPart(X+m);
		}
		else if (3 <= H1 && H1 < 4)
		{
			rgb.Red = ClrPart(0+m); rgb.Green = ClrPart(X+m); rgb.Blue = ClrPart(C+m);
		}
		else if (4 <= H1 && H1 < 5)
		{
			rgb.Red = ClrPart(X+m); rgb.Green = ClrPart(0+m); rgb.Blue = ClrPart(C+m);
		}
		else if (5 <= H1 && H1 < 6)
		{
			rgb.Red = ClrPart(C+m); rgb.Green = ClrPart(0+m); rgb.Blue = ClrPart(X+m);
		}
	}

	rgbclr = rgb.clr;
}
#endif

//#define RETURN_HSV(h, s, v) {HSV.H = h; HSV.S = s; HSV.V = v; return
//HSV;}
//
//#define RETURN_RGB(r, g, b) {RGB.R = r; RGB.G = g; RGB.B = b; return
//RGB;}

#define UNDEFINED 0

// Theoretically, hue 0 (pure red) is identical to hue 6 in these transforms. Pure

// red always maps to 6 in this implementation. Therefore UNDEFINED can be

// defined as 0 in situations where only unsigned numbers are desired.

//typedef struct {float R, G, B;} RGBType;
//
//typedef struct {float H, S, V;} HSVType;

struct HSVColor
{
	int H; // (0..1)*360
	int S; // (0..1)*100
	int V; // (0..1)*100
};


void RGB2HSV(const RGBColor& rgb, HSVColor& HSV)
{
	// rgb are each on [0, 1]. S and V are returned on [0, 1] and H is
	// returned on [0, 1]. Exception: H is returned UNDEFINED if S==0.
	int R = rgb.R, G = rgb.G, B = rgb.B, v, x, f;
	int i;
	x = std::min(R, G);
	x = std::min(x, B);
	v = std::max(R, G);
	v = std::max(v, B);

	if (v == x)
	{
		HSV.H = UNDEFINED;
		HSV.S = 0;
		HSV.V = v;
	}
	else if (v == 0)
	{
		_ASSERTE(v!=0);
		HSV.H = UNDEFINED;
		HSV.S = 0;
		HSV.V = v;
	}
	else
	{
		f = (R == x) ? G - B : ((G == x) ? B - R : R - G); // -255 .. 255
		i = (R == x) ? 3 : ((G == x) ? 5 : 1);

		HSV.H = (((360*i) - ((360*f)/(v - x)))/6);
		HSV.S = (100*(v - x))/v;
		HSV.V = 100*v/255;
	}
}

inline BYTE ClrPart(int v)
{
	return (v < 0) ? 0 : (v > 100) ? 255 : (int)((v*255)/100);
}

void HSV2RGB(const HSVColor& HSV, RGBColor& rgb)
{
	rgb.clr = 0;

	// H is given on [0, 1] or UNDEFINED. S and V are given on [0, 1].
	// rgb are each returned on [0, 1].
	int h = HSV.H * 6, s = HSV.S, v = HSV.V, m, n, f;
	int i;
	//if (h == 0) h = 1;
	if ((HSV.H < 0) /*== UNDEFINED*/ || (!HSV.H && !HSV.V))
	{
		rgb.R = rgb.G = rgb.B = ClrPart(v);
	}
	else
	{
		//i = floorf(h);
		if (HSV.H < 60)
			i = 0;
		else if (HSV.H < 120)
			i = 1;
		else if (HSV.H < 180)
			i = 2;
		else if (HSV.H < 240)
			i = 3;
		else if (HSV.H < 300)
			i = 4;
		else if (HSV.H < 360)
			i = 5;
		else
			i = 6;
		//TODO:
		f = h - i*360;
		if(!(i & 1)) f = 360 - f; // if i is even
		m = v * (100 - s) / 100;
		n = v * (100 - s * f / 360) / 100;

		#define RETURN_RGB(r,g,b) rgb.R = ClrPart(r); rgb.G = ClrPart(g); rgb.B = ClrPart(b);
		switch (i)
		{
	        case 6:
	        case 0:  RETURN_RGB(v, n, m); break;
	        case 1:  RETURN_RGB(n, v, m); break;
	        case 2:  RETURN_RGB(m, v, n); break;
	        case 3:  RETURN_RGB(m, n, v); break;
	        case 4:  RETURN_RGB(n, m, v); break;
	        case 5:  RETURN_RGB(v, m, n); break;
	        default: RETURN_RGB(0, 0, 0);
		}
	}
}


// Сколько линий в области статуса (без учета рамок)
int GetStatusLineCount(struct PaintBackgroundArg* pBk, BOOL bLeft)
{
	if (!(pBk->FarPanelSettings.ShowStatusLine))
	{
		// Что-то при запуске (1.7x?) иногда картинки прыгают, как будто статус сразу не нашли
		_ASSERTE(pBk->FarPanelSettings.ShowStatusLine);
		return 0;
	}

	if ((bLeft ? pBk->LeftPanel.nPanelType : pBk->RightPanel.nPanelType) != PTYPE_FILEPANEL)
	{
		_ASSERTE(((UINT)(bLeft ? pBk->LeftPanel.nPanelType : pBk->RightPanel.nPanelType)) <= PTYPE_INFOPANEL);
		return 0;
	}

	RECT rcPanel = bLeft ? pBk->LeftPanel.rcPanelRect : pBk->RightPanel.rcPanelRect;

	if ((rcPanel.bottom-rcPanel.top) <= ((pBk->FarPanelSettings.ShowColumnTitles) ? 5 : 4))
		return 1; // минимальная высота панели

	COORD bufSize = {(SHORT)(rcPanel.right-rcPanel.left+1), std::min<SHORT>(10, (SHORT)(rcPanel.bottom-rcPanel.top))};
	COORD bufCoord = {0,0};

	HANDLE hStd = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi = {};
	short nShiftX = 0, nShiftY = 0;
	if (GetConsoleScreenBufferInfo(hStd, &csbi))
	{
		// Начиная с какой-то версии в фаре поменяли координаты :(
		if (rcPanel.top <= 1)
		{
			nShiftY = csbi.dwSize.Y - (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
		}
	}

	SMALL_RECT readRect = {
		(SHORT)rcPanel.left + nShiftX,
		(SHORT)(rcPanel.bottom-bufSize.Y)+nShiftY,
		(SHORT)rcPanel.right+nShiftX,
		(SHORT)rcPanel.bottom+nShiftY
	};

	PCHAR_INFO pChars = (PCHAR_INFO)malloc(bufSize.X*bufSize.Y*sizeof(*pChars));
	if (!pChars)
	{
		_ASSERTE(pChars);
		return 1;
	}

	int nLines = 0;

	BOOL lbReadRc = ReadConsoleOutputW(hStd, pChars, bufSize, bufCoord, &readRect);
	if (!lbReadRc)
	{
		_ASSERTE(lbReadRc);
	}
	else
	{
		for (int i = 2; i <= bufSize.Y; i++)
		{
			if ((pChars[bufSize.X*(bufSize.Y-i)].Char.UnicodeChar == ucBoxDblVertSinglRight)
				|| (pChars[bufSize.X*(bufSize.Y-i)+bufSize.X-1].Char.UnicodeChar == ucBoxDblVertSinglLeft))
			{
				nLines = i - 1;
				break;
			}
		}
	}

	// Что-то при запуске (1.7x?) иногда картинки прыгают, как будто статус сразу не нашли
#ifdef _DEBUG
	int nArea = -1;
	if (nLines<1)
	{
		static bool bWarnLines = false;
		if (!bWarnLines)
		{
			nArea = GetMacroArea();
			if (nArea == 1/*MACROAREA_SHELL*/ || nArea == 5/*MACROAREA_SEARCH*/)
			{
				bWarnLines = true;
				// Far 3.0.3716. Assert при старте. Плагин активирован, а панелей на экране еще НЕТ.
				_ASSERTE(nLines>0);
			}
		}
	}
#endif

	free(pChars);

	return nLines;
}


void FormatSize(ULARGE_INTEGER size, wchar_t* out)
{
	if (size.QuadPart)
	{
		// Сформатировать размер
		uint64_t lSize = size.QuadPart, lDec = 0;
		const wchar_t *SizeSymbol[]={L"B",L"KB",L"MB",L"GB",L"TB",L"PB"};
		for (size_t n = 0; n < countof(SizeSymbol); n++)
		{
			if (lSize < 1000)
			{
				if (lDec > 0 && lDec < 10 && lSize < 10)
					swprintf_c(out, MAX_PATH/*#SECURELEN*/, L"%u.%u %s", (UINT)lSize, (UINT)lDec, SizeSymbol[n]);
				else
					swprintf_c(out, MAX_PATH/*#SECURELEN*/, L"%u %s", (UINT)lSize, SizeSymbol[n]);
				break;
			}

			uint64_t lNext = lSize >> 10;
			lDec = (lSize % 1024) / 100;
			lSize = lNext;
		}
		//if (!*szVolumeSize)
		//{
		//	swprintf_c(szVolumeSize, MAX_PATH/*#SECURELEN*/, L"%u %s", (UINT)lSize, SizeSymbol[countof(SizeSymbol)-1]);
		//}
	}
}


int PaintPanel(struct PaintBackgroundArg* pBk, BOOL bLeft, COLORREF& crOtherColor, int& cOtherDrive)
{
	DrawInfo* pDraw = (DrawInfo*)calloc(sizeof(*pDraw), 1);
	if (!pDraw)
		return FALSE;

	wchar_t szDbg[128];

	// Инициализация и начальное определение размеров и прочего...
	int nStatusLines = GetStatusLineCount(pBk, bLeft);
	RECT rcPanel = bLeft ? pBk->rcDcLeft : pBk->rcDcRight;
	RECT rcConPanel = bLeft ? pBk->LeftPanel.rcPanelRect : pBk->RightPanel.rcPanelRect;
	BkPanelInfo *bkInfo = bLeft ? &pBk->LeftPanel : &pBk->RightPanel;
	// Не должен содержать рамки
	int nPanelWidth = std::max<int>(1, rcConPanel.right - rcConPanel.left + 1);  // ширина в символах
	int nPanelHeight = std::max<int>(1, rcConPanel.bottom - rcConPanel.top + 1); // высота в символах
	RECT rcWork = {}; //rcPanel;
	rcWork.left = rcPanel.left + ((rcPanel.right - rcPanel.left + 1) / nPanelWidth);
	rcWork.right = rcPanel.right - ((rcPanel.right - rcPanel.left + 1) / nPanelWidth);
	rcWork.top = rcPanel.top + ((rcPanel.bottom - rcPanel.top + 1) / nPanelHeight);
	rcWork.bottom = rcPanel.bottom - ((rcPanel.bottom - rcPanel.top + 1) / nPanelHeight);
	RECT rcInt = rcWork;
	if (nStatusLines > 0)
		rcInt.bottom -= ((rcPanel.bottom - rcPanel.top + 1) * (nStatusLines + 1) / nPanelHeight);
	int nMaxPicSize = (rcInt.bottom - rcInt.top) * 35 / 100;

	ULARGE_INTEGER llTotalSize = {}, llFreeSize = {};
	UINT nDriveType = DRIVE_UNKNOWN;
	BOOL bProgressAllowed = FALSE; // Градусник свободного места

	int nMaxVolumeLen = lstrlen(bkInfo->szCurDir ? bkInfo->szCurDir : L"");
	wchar_t* szVolumeRoot = (wchar_t*)calloc(nMaxVolumeLen+2,sizeof(*szVolumeRoot));
	wchar_t* szVolumeSize = (wchar_t*)calloc(MAX_PATH,sizeof(*szVolumeSize));
	wchar_t* szVolumeFree = (wchar_t*)calloc(MAX_PATH,sizeof(*szVolumeFree));
	wchar_t* szVolume = (wchar_t*)calloc(MAX_PATH,sizeof(*szVolume));
	if (bkInfo->szCurDir && *bkInfo->szCurDir)
	{
		if (!bkInfo->bPlugin)
		{
			// Определить размер свободного места на диске
			bool lbSizeOk = false, lbTypeOk = false;
			_wcscpy_c(szVolumeRoot, nMaxVolumeLen+2, bkInfo->szCurDir);
			if (szVolumeRoot[nMaxVolumeLen-1] != L'\\')
			{
				szVolumeRoot[nMaxVolumeLen++] = L'\\';
				szVolumeRoot[nMaxVolumeLen] = 0;
			}
			int nLen = lstrlen(szVolumeRoot);
			while (!(lbSizeOk && lbTypeOk) && (nLen > 0))
			{
				// To determine whether a drive is a USB-type drive,
				// call SetupDiGetDeviceRegistryProperty and specify the SPDRP_REMOVAL_POLICY property.
				ULARGE_INTEGER llTotal = {}, llFree = {};
				UINT nType = DRIVE_UNKNOWN;
				if (!lbSizeOk && GetDiskFreeSpaceEx(szVolumeRoot, &llFree, &llTotal, NULL))
				{
					llTotalSize.QuadPart = llTotal.QuadPart;
					llFreeSize.QuadPart  = llFree.QuadPart;
					lbSizeOk = TRUE;
				}
				if (!lbTypeOk && ((nType = GetDriveType(szVolumeRoot)) != DRIVE_UNKNOWN))
				{
					nDriveType = nType;
					lbTypeOk = TRUE;
				}
				// Если не все определили - пробуем откусить папку
				if (!lbSizeOk || !lbTypeOk)
				{
					if (szVolumeRoot[nLen-1] != L'\\')
					{
						_ASSERTE(szVolumeRoot[nLen-1] == L'\\');
						break;
					}
					szVolumeRoot[--nLen] = 0;
					// Найти следующий слеш
					while ((nLen > 0) && (szVolumeRoot[nLen-1] != L'\\'))
						nLen--;
					// Пропустить все слеши
					while ((nLen > 0) && (szVolumeRoot[nLen-1] == L'\\'))
						nLen--;
					// Откусить
					szVolumeRoot[nLen] = 0;
				}
			}

			FormatSize(llTotalSize, szVolumeSize);
			FormatSize(llFreeSize, szVolumeFree);
		}

		// Извлечь "Букву" диска
		LPCWSTR psz = bkInfo->szCurDir;
		if (psz[0] == L'\\' && psz[1] == L'\\' && (psz[2] == L'.' || psz[2] == L'?') && psz[3] == L'\\')
		{
			psz += 4;
			if (psz[0] == L'U' && psz[1] == L'N' && psz[2] == L'C' && psz[3] == L'\\')
				psz += 4;
		}
		LPCWSTR pszSlash = wcschr(psz, L'\\');
		if (!pszSlash)
			pszSlash = wcschr(psz, L'/');
		if (!pszSlash)
			pszSlash = psz + lstrlen(psz);
		if (pszSlash > psz)
			lstrcpyn(szVolume, psz, (int)(pszSlash - psz + 1));
	}

	pDraw->szVolume = szVolume;
	pDraw->szVolumeRoot = szVolumeRoot;
	pDraw->szVolumeSize = szVolumeSize;
	pDraw->szVolumeFree = szVolumeFree;
	pDraw->nDriveType = nDriveType;

	if (nDriveType != DRIVE_UNKNOWN && nDriveType != DRIVE_CDROM && nDriveType != DRIVE_NO_ROOT_DIR)
	{
		bProgressAllowed = TRUE;
	}
	else
	{
		bProgressAllowed = FALSE;
	}

	// Если есть xml - получить из него настройки для текущего случая
	if (gfCreateXmlReader && XmlFile.FileData && XmlFile.FileSize)
	{
		FillPanelParams(pBk, bkInfo, pDraw);
	}

	if ((pDraw->Flags & DrawInfo::dif_Disabled))
	{
		if (pBk->dwLevel == 0)
		{
			DWORD nPanelBackIdx = CONBACKCOLOR(pBk->nFarColors[col_PanelText]);
			COLORREF crPanel = pBk->crPalette[nPanelBackIdx];
			swprintf_c(szDbg, L"ConEmuBk: Disabled %s - {%i,%i,%i,%i) #%06X\n",
				bLeft ? L"Left" : L"Right", rcConPanel.left, rcConPanel.top, rcConPanel.right, rcConPanel.bottom,
				crPanel);
			DBGSTR(szDbg);
			HBRUSH hBr = CreateSolidBrush(crPanel);
			FillRect(pBk->hdc, &rcPanel, hBr);
			DeleteObject(hBr);
		}
	}
	else
	{
		// Если не было задано - инициализируем умолчания
		if (!(pDraw->Flags & DrawInfo::dif_BackFilled))
		{
			if (bkInfo->bPlugin)
			{
				pDraw->crBack[0] = RGB(128,128,128);
			}
			else
			{
				switch (nDriveType)
				{
				case DRIVE_REMOVABLE:
					pDraw->crBack[0] = 0x00D98C; break;
				case DRIVE_REMOTE:
					pDraw->crBack[0] = 0xFF00E6; break;
				case DRIVE_CDROM:
					pDraw->crBack[0] = 0x7E00FF; break;
				case DRIVE_RAMDISK:
					pDraw->crBack[0] = 0x008080; break;
				default:
					pDraw->crBack[0] = 0xFF0000;
				}
				pDraw->Flags |= DrawInfo::dif_ShiftBackColor;
			}
			pDraw->nBackCount = 1;
		}

		int nLetter = 0;

		if ((pDraw->Flags & DrawInfo::dif_ShiftBackColor) && pDraw->crBack && (szVolume[1] == L':'))
		{
			nLetter = (szVolume[0] >= L'a' && szVolume[0] <= L'b') ? (szVolume[0] - L'a' + 24) :
					  (szVolume[0] >= L'c' && szVolume[0] <= L'z') ? (szVolume[0] - L'c') :
					  (szVolume[0] >= L'A' && szVolume[0] <= L'B') ? (szVolume[0] - L'A' + 24) :
					  (szVolume[0] >= L'C' && szVolume[0] <= L'Z') ? (szVolume[0] - L'C') :
					  0;
			if (pDraw->nBackCount > 0)
			{
				_ASSERTE(pDraw->nBackCount <= countof(pDraw->crBack));
				pDraw->crBack[0] = pDraw->crBack[nLetter % pDraw->nBackCount];
			}
			else
			{
				// Сконвертить в HSV, сдвинуть, и обратно в COLORREF
				int nShift = (nLetter % 5) * 15;
				HSVColor hsv = {};
				RGBColor rgb; rgb.clr = *pDraw->crBack;
				//COLORREF2HSB(pDraw->crBack, hsb);
				RGB2HSV(rgb, hsv);
				#ifdef _DEBUG
				HSV2RGB(hsv, rgb);
				#endif
				//hsv.H += nShift;
				hsv.S -= nShift;
				if (hsv.S < 0) hsv.S += 100;
				hsv.V = hsv.V / 2;
				HSV2RGB(hsv, rgb);
				//hsv.Brightness *= 0.7;
				//HSB2COLORREF(hsb, clr);
				if (bLeft)
				{
					crOtherColor = rgb.clr;
					cOtherDrive = nLetter;
				}
				else if ((rgb.clr == crOtherColor) && (cOtherDrive != nLetter))
				{
					hsv.H += 15;
					HSV2RGB(hsv, rgb);
				}

				pDraw->crBack[0] = rgb.clr;
			}
		}

		// Цвет спрайтов, текста, фона градусника
#if defined(_M_ARM64) || defined(__aarch64__)
		// Use Far's current panel color for the fill and the emblem's base.
		const auto panelBackground = CONBACKCOLOR(pBk->nFarColors[col_PanelText]);
		pDraw->crBack[0] = pBk->crPalette[panelBackground];
#endif
		if (!(pDraw->Flags & DrawInfo::dif_DarkFilled))
		{
			RGBColor rgb; rgb.clr = *pDraw->crBack;
			rgb.R = (BYTE)((int)rgb.R * 2 / 3);
			rgb.G = (BYTE)((int)rgb.G * 2 / 3);
			rgb.B = (BYTE)((int)rgb.B * 2 / 3);
			pDraw->crDark[0] = rgb.clr;
			pDraw->nDarkCount = 1;
		}
		else if (pDraw->Flags & DrawInfo::dif_ShiftBackColor)
		{
			if (pDraw->nDarkCount > 0)
			{
				_ASSERTE(pDraw->nDarkCount <= countof(pDraw->crDark));
				pDraw->crDark[0] = pDraw->crDark[nLetter % pDraw->nDarkCount];
			}
		}
		// Цвет градусника
		if (!(pDraw->Flags & DrawInfo::dif_LightFilled))
		{
			HSVColor hsv = {};
			RGBColor rgb; rgb.clr = *pDraw->crBack;
			RGB2HSV(rgb, hsv);
			hsv.H += 20;
			hsv.S = std::min(100,hsv.S+25);
			hsv.V = std::min(100,hsv.V+25);
			HSV2RGB(hsv, rgb);
			pDraw->crLight[0] = rgb.clr;
			pDraw->nLightCount = 1;
		}
		else if (pDraw->Flags & DrawInfo::dif_ShiftBackColor)
		{
			if (pDraw->nLightCount > 0)
			{
				_ASSERTE(pDraw->nLightCount <= countof(pDraw->crLight));
				pDraw->crLight[0] = pDraw->crLight[nLetter % pDraw->nLightCount];
			}
		}


		if (!(pDraw->Flags & DrawInfo::dif_PicFilled))
		{
			if (bkInfo->bPlugin)
			{
				wcscpy_c(pDraw->szPic, L"img/plugin.png");
			}
			else
			{
				switch (nDriveType)
				{
				case DRIVE_REMOVABLE:
					wcscpy_c(pDraw->szPic, L"img/drive_removable.png"); break;
				case DRIVE_REMOTE:
					wcscpy_c(pDraw->szPic, L"img/drive_network.png"); break;
				case DRIVE_CDROM:
					wcscpy_c(pDraw->szPic, L"img/drive_cdrom.png"); break;
				case DRIVE_RAMDISK:
					wcscpy_c(pDraw->szPic, L"img/drive_ramdisk.png"); break;
				default:
					wcscpy_c(pDraw->szPic, L"img/drive_fixed.png");
				}
			}
		}
		if (!(pDraw->Flags & DrawInfo::dif_TextFilled))
		{
			TODO("Размер диска");
			wcscpy_c(pDraw->szText, szVolume);
		}


		// Поехали рисовать
		if (pBk->dwLevel == 0)
		{
			swprintf_c(szDbg, L"ConEmuBk: %s - {%i,%i,%i,%i) #%06X\n",
				bLeft ? L"Left" : L"Right", rcConPanel.left, rcConPanel.top, rcConPanel.right, rcConPanel.bottom,
				*pDraw->crBack);
			DBGSTR(szDbg);
			HBRUSH hBrush = CreateSolidBrush(*pDraw->crBack);
			FillRect(pBk->hdc, &rcPanel, hBrush);
			DeleteObject(hBrush);
		}

		LOGFONT lf = {};
		lf.lfHeight = std::max<LONG>(20, (rcInt.bottom - rcInt.top) * 12 / 100);
		lf.lfWeight = 700;
		lstrcpy(lf.lfFaceName, L"Arial");

		// GO
		HFONT hText = CreateFontIndirect(&lf);
		HFONT hOldFont = (HFONT)SelectObject(pBk->hdc, hText);

		#define IMG_SHIFT_X 0
		#define IMG_SHIFT_Y 0
		#define LINE_SHIFT_Y (lf.lfHeight)
		#define LINE_SHIFT_X (lf.lfHeight/6)

		// Determine appropriate font size:
		int nY = std::max(rcInt.top, rcInt.bottom - (LINE_SHIFT_Y));
		RECT rcText = {rcInt.left+IMG_SHIFT_X, nY, rcInt.right-LINE_SHIFT_X, nY+LINE_SHIFT_Y};
		RECT rcTemp = rcText;

		DrawText(pBk->hdc, pDraw->szText, -1, &rcTemp, DT_HIDEPREFIX|DT_RIGHT|DT_SINGLELINE|DT_TOP|DT_CALCRECT);

		LONG width = rcText.right - rcText.left;
		LONG actualWidth = rcTemp.right - rcTemp.left;

		if (actualWidth > width)
		{
			// Delete current font:
			SelectObject(pBk->hdc, hOldFont);
			DeleteObject(hText);

			// Create new font of appropriate size:
			lf.lfHeight = lf.lfHeight * width / actualWidth;
			hText = CreateFontIndirect(&lf);
			hOldFont = (HFONT)SelectObject(pBk->hdc, hText);
		}

		CachedImage* pI = NULL;
		if (gpDecoder && *pDraw->szPic)
		{
			pI = gpDecoder->GetImage(pDraw->szPic);
			if (pI)
			{
				int nPicDim = std::max(pI->nWidth,pI->nHeight);
				int nW = std::min(nMaxPicSize,nPicDim), nH = std::min(nMaxPicSize,nPicDim); //TODO: Пропорционально pI->nWidth/pI->nHeight
				if (pI && (rcWork.top <= (rcText.top - nH - IMG_SHIFT_Y)) && (rcWork.left <= (rcWork.right - nW - IMG_SHIFT_X)))
				{
					// - картинки чисто черного цвета
				#if 0
					const HDC hScreenDC = GetDC(0);

					HDC hReadyDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hReadyBmp = CreateCompatibleBitmap(hScreenDC, nW, nH);
					HBITMAP hOldReadyBmp = (HBITMAP)SelectObject(hReadyDC, hReadyBmp);

					StretchBlt(hReadyDC, 0, 0, nW, nH, pI->hDc, 0,0, pI->nWidth, pI->nHeight, SRCCOPY);

					BitBlt(pBk->hdc, rcWork.right - nW - IMG_SHIFT_X, rcText.top - nH - IMG_SHIFT_Y, nW, nH,
						hReadyDC, 0,0, SRCAND);

					SelectObject(hReadyDC, hOldReadyBmp);
					DeleteObject(hReadyBmp);
					DeleteDC(hReadyDC);

					ReleaseDC(0, hScreenDC);
				#endif

					// OK
				#if 1
					const HDC hScreenDC = GetDC(0);

					HDC hMaskDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hMaskBmp = CreateCompatibleBitmap(hScreenDC, pI->nWidth, pI->nHeight);
					HBITMAP hOldMaskBmp = (HBITMAP)SelectObject(hMaskDC, hMaskBmp);

					StretchBlt(hMaskDC, 0, 0, nW, nH, pI->hDc, 0,0, pI->nWidth, pI->nHeight, NOTSRCCOPY);

					HDC hInvDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hInvBmp = CreateCompatibleBitmap(hScreenDC, pI->nWidth, pI->nHeight);
					HBITMAP hOldInvBmp = (HBITMAP)SelectObject(hInvDC, hInvBmp);

					HBRUSH hDarkBr = CreateSolidBrush(*pDraw->crDark);
					HBRUSH hOldBr = (HBRUSH)SelectObject(hInvDC, hDarkBr);

					BitBlt(hInvDC, 0, 0, nW, nH,
						hMaskDC, 0,0, MERGECOPY);

					SelectObject(hInvDC, hOldBr);

					HDC hReadyDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hReadyBmp = CreateCompatibleBitmap(hScreenDC, nW, nH);
					HBITMAP hOldReadyBmp = (HBITMAP)SelectObject(hReadyDC, hReadyBmp);
					HBRUSH hBackBr = CreateSolidBrush(*pDraw->crBack);
					hOldBr = (HBRUSH)SelectObject(hReadyDC, hBackBr);

					BitBlt(hMaskDC, 0, 0, nW, nH,
						hMaskDC, 0,0, DSTINVERT);

					BitBlt(hReadyDC, 0, 0, nW, nH,
						hMaskDC, 0,0, MERGECOPY);

					SelectObject(hReadyDC, hOldBr);

					BitBlt(hReadyDC, 0, 0, nW, nH,
						hInvDC, 0,0, SRCPAINT);

					DeleteObject(hDarkBr);
					DeleteObject(hBackBr);

					BitBlt(pBk->hdc, rcWork.right - nW - IMG_SHIFT_X, rcText.top - nH - IMG_SHIFT_Y, nW, nH,
						hReadyDC, 0,0, SRCCOPY);

					SelectObject(hReadyDC, hOldReadyBmp);
					DeleteObject(hReadyBmp);
					DeleteDC(hReadyDC);

					SelectObject(hInvDC, hOldInvBmp);
					DeleteObject(hInvBmp);
					DeleteDC(hInvDC);

					SelectObject(hMaskDC, hOldMaskBmp);
					DeleteObject(hMaskBmp);
					DeleteDC(hMaskDC);

					ReleaseDC(0, hScreenDC);
				#endif

				#if 0
					const HDC hScreenDC = GetDC(0);

					HDC hInvDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hInvBmp = CreateCompatibleBitmap(hScreenDC, pI->nWidth, pI->nHeight);
					HBITMAP hOldInvBmp = (HBITMAP)SelectObject(hInvDC, hInvBmp);

					RECT rcFill = {0,0,pI->nWidth, pI->nHeight};
					HBRUSH hDarkBr = CreateSolidBrush(/*RGB(128,128,128)*/ *pDraw->crDark);
					FillRect(hInvDC, &rcFill, hDarkBr);
					DeleteObject(hDarkBr);

					BitBlt(hInvDC, 0, 0, pI->nWidth, pI->nHeight,
						pI->hDc, 0,0, SRCAND);

					HDC hReadyDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hReadyBmp = CreateCompatibleBitmap(hScreenDC, nW, nH);
					HBITMAP hOldReadyBmp = (HBITMAP)SelectObject(hReadyDC, hReadyBmp);

					//SetStretchBltMode(hReadyDC, HALFTONE);
					rcFill.right = nW; rcFill.bottom = nH;
					FillRect(hReadyDC, &rcFill, (HBRUSH)GetStockObject(BLACK_BRUSH));
					StretchBlt(hReadyDC, 0, 0, nW, nH, hInvDC, 0,0, pI->nWidth, pI->nHeight, SRCINVERT);

					BitBlt(pBk->hdc, rcWork.right - nW - IMG_SHIFT_X, rcText.top - nH - IMG_SHIFT_Y, nW, nH,
						hReadyDC, 0,0, SRCCOPY);

					SelectObject(hReadyDC, hOldReadyBmp);
					DeleteObject(hReadyBmp);
					DeleteDC(hReadyDC);

					ReleaseDC(0, hScreenDC);
				#endif

					// - Черная окантовка вокруг плагиновой картинки
				#if 0
					const HDC hScreenDC = GetDC(0);

					HDC hInvDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hInvBmp = CreateCompatibleBitmap(hScreenDC, pI->nWidth, pI->nHeight);
					HBITMAP hOldInvBmp = (HBITMAP)SelectObject(hInvDC, hInvBmp);

					BitBlt(hInvDC, 0, 0, pI->nWidth, pI->nHeight, pI->hDc, 0,0, SRCCOPY); //NOTSRCCOPY

					HDC hCompDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hCompBmp = CreateCompatibleBitmap(hScreenDC, pI->nWidth, pI->nHeight);
					HBITMAP hOldCompBmp = (HBITMAP)SelectObject(hCompDC, hCompBmp);
					HDC hBackDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hBackBmp = CreateCompatibleBitmap(hScreenDC, pI->nWidth, pI->nHeight);
					HBITMAP hOldBackBmp = (HBITMAP)SelectObject(hBackDC, hBackBmp);

					HBRUSH hPaintBr = CreateSolidBrush(*pDraw->crDark);
					HBRUSH hBackBr = CreateSolidBrush(*pDraw->crBack);

					RECT rcFill = {0,0,pI->nWidth, pI->nHeight};

					HBRUSH hOldCompBr = (HBRUSH)SelectObject(hCompDC, hBackBr);


					BitBlt(hCompDC, 0, 0, pI->nWidth, pI->nHeight, hInvDC, 0,0, MERGECOPY);

					BitBlt(hInvDC, 0, 0, pI->nWidth, pI->nHeight, pI->hDc, 0,0, NOTSRCCOPY);
					HBRUSH hOldBackBr = (HBRUSH)SelectObject(hBackDC, hPaintBr);
					BitBlt(hBackDC, 0, 0, pI->nWidth, pI->nHeight, hInvDC, 0,0, MERGECOPY);

					BitBlt(hCompDC, 0, 0, pI->nWidth, pI->nHeight, hBackDC, 0,0, SRCPAINT);


					HDC hReadyDC = CreateCompatibleDC(hScreenDC);
					HBITMAP hReadyBmp = CreateCompatibleBitmap(hScreenDC, nW, nH);
					HBITMAP hOldReadyBmp = (HBITMAP)SelectObject(hReadyDC, hReadyBmp);

					StretchBlt(hReadyDC, 0, 0, nW, nH, hCompDC, 0,0, pI->nWidth, pI->nHeight, SRCCOPY); //NOTSRCCOPY

					BOOL lbBitRc = BitBlt(pBk->hdc, rcWork.right - nW - IMG_SHIFT_X, rcText.top - nH - IMG_SHIFT_Y, nW, nH,
						hReadyDC, 0,0, SRCCOPY);

					SelectObject(hReadyDC, hOldReadyBmp);
					DeleteObject(hReadyBmp);
					DeleteDC(hReadyDC);

					DeleteObject(hPaintBr);
					SelectObject(hCompDC, hOldCompBr);
					DeleteObject(hBackBr);

					SelectObject(hInvDC, hOldInvBmp);
					DeleteObject(hInvBmp);
					DeleteDC(hInvDC);

					SelectObject(hCompDC, hOldCompBmp);
					DeleteObject(hCompBmp);
					DeleteDC(hCompDC);

					SelectObject(hBackDC, hOldBackBmp);
					DeleteObject(hBackBmp);
					DeleteDC(hBackDC);

					ReleaseDC(0, hScreenDC);
				#endif
				}
			}
		}

		SetBkMode(pBk->hdc, TRANSPARENT);
		SetTextColor(pBk->hdc, *pDraw->crDark);

		DrawText(pBk->hdc, pDraw->szText, -1, &rcText, DT_HIDEPREFIX|DT_RIGHT|DT_SINGLELINE|DT_TOP);
		/*
		OffsetRect(&rcText, 0, LINE_SHIFT_Y);

		swprintf_c(szText, L"Volume: «%s» %s, Format: «%s»", szVolume, szVolumeSize, bkInfo->szFormat ? bkInfo->szFormat : L"");
		DrawText(pBk->hdc, szText, -1, &rcText, DT_HIDEPREFIX|DT_RIGHT|DT_SINGLELINE|DT_TOP);
		OffsetRect(&rcText, 0, LINE_SHIFT_Y);

		if (bkInfo->szCurDir)
			DrawText(pBk->hdc, bkInfo->szCurDir, -1, &rcText, DT_HIDEPREFIX|DT_RIGHT|DT_SINGLELINE|DT_TOP|DT_PATH_ELLIPSIS);
		OffsetRect(&rcText, 0, LINE_SHIFT_Y);
		*/

		TODO("В Far3 можно и в плагинах свободное место получить");
		if (bProgressAllowed && pDraw->nSpaceBar == DrawInfo::dib_Off)
			bProgressAllowed = FALSE;
		if (bProgressAllowed && (nStatusLines > 0) && !bkInfo->bPlugin && llTotalSize.QuadPart)
		{
			//llTotalSize = {},
			HBRUSH hUsedBr = CreateSolidBrush(*pDraw->crLight);
			HBRUSH hFreeBr = CreateSolidBrush(*pDraw->crDark);
			RECT rcUsed = (pDraw->nSpaceBar == DrawInfo::dib_Small) ? rcWork : rcPanel;
			int iShift = (pDraw->nSpaceBar == DrawInfo::dib_Small) ? 0 : 2;
			rcUsed.top = rcUsed.bottom - (nStatusLines+iShift) * ((rcPanel.bottom - rcPanel.top + 1) / nPanelHeight);
			rcUsed.right = rcUsed.right - (int)((uint64_t)(rcUsed.right - rcUsed.left + 1) * llFreeSize.QuadPart / llTotalSize.QuadPart);
			if (rcUsed.right > rcWork.right)
			{
				_ASSERTE(rcUsed.right <= rcWork.right);
				rcUsed.right = rcWork.right;
			}
			else if (rcUsed.right < rcWork.left)
			{
				_ASSERTE(rcUsed.right >= rcWork.left);
				rcUsed.right = rcWork.left;
			}
			RECT rcFree = rcUsed;
			rcFree.left = rcUsed.right;
			rcFree.right = (pDraw->nSpaceBar == DrawInfo::dib_Small) ? rcWork.right : rcPanel.right;
			FillRect(pBk->hdc, &rcFree, hFreeBr);
			FillRect(pBk->hdc, &rcUsed, hUsedBr);
			DeleteObject(hUsedBr);
			DeleteObject(hFreeBr);
		}

		SelectObject(pBk->hdc, hOldFont);
		DeleteObject(hText);
	}

	// Free pointers
	free(pDraw);
	free(szVolume);
	free(szVolumeSize);
	free(szVolumeFree);
	return TRUE;
}

int WINAPI PaintConEmuBackground(struct PaintBackgroundArg* pBk)
{
	int iLeftRc = 0, iRightRc = 0;

	if (pBk->Place == pbp_Finalize)
	{
		GdipDone();
		return TRUE;
	}

	if (!gbGdiPlusInitialized)
	{
		if (!GdipInit())
		{
			return FALSE;
		}
	}
	else
	{
		// Проверить, может содержимое xml-ки изменилось?
		CheckXmlFile();
	}

	wchar_t szDbg[128];
	COLORREF crOtherColor = (COLORREF)-1;
	int cOtherDrive = -1;

	if (pBk->LeftPanel.bVisible)
	{
		iLeftRc = PaintPanel(pBk, TRUE/*bLeft*/, crOtherColor, cOtherDrive);
	}
	else
	{
		RECT rcConPanel = pBk->LeftPanel.rcPanelRect;
		swprintf_c(szDbg, L"ConEmuBk: Invisible Left - {%i,%i,%i,%i)\n",
			rcConPanel.left, rcConPanel.top, rcConPanel.right, rcConPanel.bottom);
		DBGSTR(szDbg);
	}

	if (pBk->RightPanel.bVisible)
	{
		iRightRc = PaintPanel(pBk, FALSE/*bLeft*/, crOtherColor, cOtherDrive);
	}
	else
	{
		RECT rcConPanel = pBk->RightPanel.rcPanelRect;
		swprintf_c(szDbg, L"ConEmuBk: Invisible Right - {%i,%i,%i,%i)\n",
			rcConPanel.left, rcConPanel.top, rcConPanel.right, rcConPanel.bottom);
		DBGSTR(szDbg);
	}

	UNREFERENCED_PARAMETER(iLeftRc); UNREFERENCED_PARAMETER(iRightRc);

	return TRUE;

	//DWORD nPanelBackIdx = (pBk->nFarColors[col_PanelText] & 0xF0) >> 4;
	//
	//if (bDragBackground)
	//{
	//	if (pBk->LeftPanel.bVisible)
	//	{
	//		COLORREF crPanel = pBk->crPalette[nPanelBackIdx];
	//
	//		if (pBk->LeftPanel.bPlugin && gbHilightPlugins)
	//			crPanel = gcrHilightPlugBack;
	//
	//		HBRUSH hBr = CreateSolidBrush(crPanel);
	//		FillRect(pBk->hdc, &pBk->rcDcLeft, hBr);
	//		DeleteObject(hBr);
	//	}
	//
	//	if (pBk->RightPanel.bVisible)
	//	{
	//		COLORREF crPanel = pBk->crPalette[nPanelBackIdx];
	//
	//		if (pBk->RightPanel.bPlugin && gbHilightPlugins)
	//			crPanel = gcrHilightPlugBack;
	//
	//		HBRUSH hBr = CreateSolidBrush(crPanel);
	//		FillRect(pBk->hdc, &pBk->rcDcRight, hBr);
	//		DeleteObject(hBr);
	//	}
	//}
	//
	//if ((pBk->LeftPanel.bVisible || pBk->RightPanel.bVisible) /*&& pBk->MainFont.nFontHeight>0*/)
	//{
	//	HPEN hPen = CreatePen(PS_SOLID, 1, gcrLinesColor);
	//	HPEN hOldPen = (HPEN)SelectObject(pBk->hdc, hPen);
	//	HBRUSH hBrush = CreateSolidBrush(gcrLinesColor);
	//	int nCellHeight = 12;
	//
	//	if (pBk->LeftPanel.bVisible)
	//		nCellHeight = pBk->rcDcLeft.bottom / (pBk->LeftPanel.rcPanelRect.bottom - pBk->LeftPanel.rcPanelRect.top + 1);
	//	else
	//		nCellHeight = pBk->rcDcRight.bottom / (pBk->RightPanel.rcPanelRect.bottom - pBk->RightPanel.rcPanelRect.top + 1);
	//
	//	int nY1 = (pBk->LeftPanel.bVisible) ? pBk->rcDcLeft.top : pBk->rcDcRight.top;
	//	int nY2 = (pBk->rcDcLeft.bottom >= pBk->rcDcRight.bottom) ? pBk->rcDcLeft.bottom : pBk->rcDcRight.bottom;
	//	int nX1 = (pBk->LeftPanel.bVisible) ? 0 : pBk->rcDcRight.left;
	//	int nX2 = (pBk->RightPanel.bVisible) ? pBk->rcDcRight.right : pBk->rcDcLeft.right;
	//
	//	bool bDrawStipe = true;
	//
	//	for(int Y = nY1; Y < nY2; Y += nCellHeight)
	//	{
	//		if (giHilightType == 0)
	//		{
	//			MoveToEx(pBk->hdc, nX1, Y, NULL);
	//			LineTo(pBk->hdc, nX2, Y);
	//		}
	//		else if (giHilightType == 1)
	//		{
	//			if (bDrawStipe)
	//			{
	//				bDrawStipe = false;
	//				RECT rc = {nX1, Y - nCellHeight + 1, nX2, Y};
	//				FillRect(pBk->hdc, &rc, hBrush);
	//			}
	//			else
	//			{
	//				bDrawStipe = true;
	//			}
	//		}
	//	}
	//
	//	SelectObject(pBk->hdc, hOldPen);
	//	DeleteObject(hPen);
	//	DeleteObject(hBrush);
	//}
	//
	//return TRUE;
}

void WINAPI OnConEmuLoaded(struct ConEmuLoadedArg* pConEmuInfo)
{
	gfRegisterBackground = pConEmuInfo->RegisterBackground;
	ghPluginModule = pConEmuInfo->hPlugin;

	if (gfRegisterBackground && gbBackgroundEnabled)
	{
		StartPlugin(FALSE/*считать параметры из реестра*/);
	}
}

void SettingsLoad()
{
	if (!gbSetStartupInfoOk)
	{
		_ASSERTE(gbSetStartupInfoOk);
		return;
	}

	if (!*gsXmlConfigFile)
	{
		lstrcpyn(gsXmlConfigFile, szDefaultXmlName, countof(gsXmlConfigFile));
	}

	if (gFarVersion.dwVerMajor == 1)
		SettingsLoadA();
	else if (gFarVersion.dwBuild >= FAR_Y2_VER)
		FUNC_Y2(SettingsLoadW)();
	else if (gFarVersion.dwBuild >= FAR_Y1_VER)
		FUNC_Y1(SettingsLoadW)();
	else
		FUNC_X(SettingsLoadW)();
}

void SettingsLoadReg(LPCWSTR pszRegKey)
{
	HKEY hkey = NULL;

	if (!RegOpenKeyExW(HKEY_CURRENT_USER, pszRegKey, 0, KEY_READ, &hkey))
	{
		DWORD nVal, nType, nSize; BYTE cVal;

		for (ConEmuBgSettings *p = gSettings; p->pszValueName; p++)
		{
			if (p->nValueType == REG_BINARY)
			{
				_ASSERTE(p->nValueSize == 1);
				if (!RegQueryValueExW(hkey, p->pszValueName, 0, &(nType = REG_BINARY), &cVal, &(nSize = sizeof(cVal))))
					*((BOOL*)p->pValue) = (cVal != 0);
			}
			else if (p->nValueType == REG_DWORD)
			{
				_ASSERTE(p->nValueSize == 4);
				if (!RegQueryValueExW(hkey, p->pszValueName, 0, &(nType = REG_DWORD), (LPBYTE)&nVal, &(nSize = sizeof(nVal))))
					*((DWORD*)p->pValue) = nVal;
			}
			else if (p->nValueType == REG_SZ)
			{
				wchar_t szValue[MAX_PATH] = {};
				if (!RegQueryValueExW(hkey, p->pszValueName, 0, &(nType = REG_SZ), (LPBYTE)szValue, &(nSize = sizeof(szValue))))
					lstrcpyn((wchar_t*)p->pValue, szValue, p->nValueSize);
			}
		}

		RegCloseKey(hkey);
	}
}

void SettingsSave()
{
	if (gFarVersion.dwVerMajor == 1)
		SettingsSaveA();
	else if (gFarVersion.dwBuild >= FAR_Y2_VER)
		FUNC_Y2(SettingsSaveW)();
	else if (gFarVersion.dwBuild >= FAR_Y1_VER)
		FUNC_Y1(SettingsSaveW)();
	else
		FUNC_X(SettingsSaveW)();
}

void SettingsSaveReg(LPCWSTR pszRegKey)
{
	HKEY hkey = NULL;

	if (!RegCreateKeyExW(HKEY_CURRENT_USER, pszRegKey, 0, 0, 0, KEY_ALL_ACCESS, 0, &hkey, NULL))
	{
		BYTE cVal;

		for (ConEmuBgSettings *p = gSettings; p->pszValueName; p++)
		{
			if (p->nValueType == REG_BINARY)
			{
				_ASSERTE(p->nValueSize == 1);
				cVal = (BYTE)*(BOOL*)p->pValue;
				RegSetValueExW(hkey, p->pszValueName, 0, REG_BINARY, &cVal, sizeof(cVal));
			}
			else if (p->nValueType == REG_DWORD)
			{
				_ASSERTE(p->nValueSize == 4);
				RegSetValueExW(hkey, p->pszValueName, 0, REG_DWORD, p->pValue, p->nValueSize);
			}
			else if (p->nValueType == REG_SZ)
			{
				RegSetValueExW(hkey, p->pszValueName, 0, REG_SZ, p->pValue, sizeof(wchar_t)*(1+lstrlenW((wchar_t*)p->pValue)));
			}
		}

		RegCloseKey(hkey);
	}
}

void StartPlugin(BOOL bConfigure)
{
	if (gbInStartPlugin)
	{
		// Вложенных вызовов быть не должно
		_ASSERTE(gbInStartPlugin==false);
		return;
	}

	gbInStartPlugin = true;

	if (!XmlFile.cr)
	{
		XmlFile.cr = new MSectionSimple(true);
	}

	if (!bConfigure)
	{
		SettingsLoad();
	}

#if defined(_M_ARM64) || defined(__aarch64__)
	// Far can load this plugin after ConEmu when the loader hook is unavailable.
	if (!gfRegisterBackground)
	{
		if (const HMODULE conemu = GetModuleHandleW(L"ConEmu.arm64.dll"))
			gfRegisterBackground = reinterpret_cast<RegisterBackground_t>(GetProcAddress(conemu, "RegisterBackground"));
	}
#endif

	static bool bWasRegistered = false;

	if (gfRegisterBackground)
	{
		if (gbBackgroundEnabled)
		{
			if (bConfigure && bWasRegistered)
			{
				RegisterBackgroundArg upd = {sizeof(RegisterBackgroundArg), rbc_Redraw};
				gfRegisterBackground(&upd);
			}
			else
			{
				RegisterBackgroundArg reg = {sizeof(RegisterBackgroundArg), rbc_Register, ghPluginModule};
				reg.PaintConEmuBackground = ::PaintConEmuBackground;
				reg.dwPlaces = pbp_Panels;
				reg.dwSuggestedLevel = 0;
				gfRegisterBackground(&reg);
				bWasRegistered = true;
			}
		}
		else
		{
			RegisterBackgroundArg unreg = {sizeof(RegisterBackgroundArg), rbc_Unregister, ghPluginModule};
			gfRegisterBackground(&unreg);
			bWasRegistered = false;
		}
	}
	else
	{
		bWasRegistered = false;
	}

	// Вернуть флаг обратно
	gbInStartPlugin = false;
}

void ExitPlugin(void)
{
	if (gfRegisterBackground != NULL)
	{
		RegisterBackgroundArg inf = {sizeof(RegisterBackgroundArg), rbc_Unregister, ghPluginModule};
		gfRegisterBackground(&inf);
	}

	// Вообще-то это нужно делать в нити отрисовки, но на всякий случай, дернем здесь (LastChance)
	GdipDone();
	gbSetStartupInfoOk = false;
}

void   WINAPI ExitFARW(void)
{
	ExitPlugin();

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(ExitFARW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
}

void WINAPI ExitFARW3(void*)
{
	ExitPlugin();

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		FUNC_Y2(ExitFARW)();
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		FUNC_Y1(ExitFARW)();
	else
		FUNC_X(ExitFARW)();
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


extern "C" HANDLE WINAPI OpenW(const void* Info)
{
	HANDLE hResult = NULL;

	if (gFarVersion.dwBuild>=FAR_Y2_VER)
		hResult = FUNC_Y2(OpenW)(Info);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		hResult = FUNC_Y1(OpenW)(Info);
	else
	{
		_ASSERTE(FALSE && "Must not called in Far2");
	}

	return hResult;
}


int WINAPI ConfigureW(int ItemNumber)
{
	if (gFarVersion.dwVerMajor==1)
		return false;
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(ConfigureW)(ItemNumber);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(ConfigureW)(ItemNumber);
	else
		return FUNC_X(ConfigureW)(ItemNumber);
}

int WINAPI ConfigureW3(void*)
{
	if (gFarVersion.dwVerMajor==1)
		return false;
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(ConfigureW)(0);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(ConfigureW)(0);
	else
		return FUNC_X(ConfigureW)(0);
}

HANDLE OpenPluginWcmn(int OpenFrom,INT_PTR Item)
{
	HANDLE hResult = (gFarVersion.dwVerMajor >= 3) ? NULL : INVALID_HANDLE_VALUE;

	if (!gbInfoW_OK)
		return hResult;

	ConfigureW(0);

	return hResult;
}

bool FMatch(LPCWSTR asMask, LPWSTR asPath)
{
	if (!asMask || !asPath)
		return false;
	if ((*asMask && !*asPath) || (*asPath && !*asMask))
		return false;

	if (gFarVersion.dwVerMajor==1)
	{
		bool lbRc = false;
		int nMaskLen = lstrlenW(asMask);
		char* pszMask = (char*)malloc(nMaskLen+1);
		int nPathLen = lstrlenW(asPath);
		char* pszPath = (char*)malloc(nPathLen+1);

		if (pszMask && pszPath)
		{
			WideCharToMultiByte(CP_ACP, 0, asMask, nMaskLen+1, pszMask, nMaskLen+1, 0,0);
			WideCharToMultiByte(CP_ACP, 0, asPath, nPathLen+1, pszPath, nPathLen+1, 0,0);

			lbRc = FMatchA(pszMask, pszPath);
		}

		if (pszMask)
			free(pszMask);
		if (pszPath)
			free(pszPath);
		return lbRc;
	}
	else if (gFarVersion.dwBuild>=FAR_Y2_VER)
		return FUNC_Y2(FMatchW)(asMask, asPath);
	else if (gFarVersion.dwBuild>=FAR_Y1_VER)
		return FUNC_Y1(FMatchW)(asMask, asPath);
	else
		return FUNC_X(FMatchW)(asMask, asPath);
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
