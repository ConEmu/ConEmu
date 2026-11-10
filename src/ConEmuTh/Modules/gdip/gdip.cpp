
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

#include "../../../common/defines.h"
using namespace std;
#include <gdiplus.h>
#include <crtdbg.h>
#include "../../../common/Memory.h"
#include "../../../common/WThreads.h"
#include "../ThumbSDK.h"

//#include "../PVD2Helper.h"
//#include "../BltHelper.h"
#include "../MStream.h"

HMODULE ghModule;

//pvdInitPlugin2 ip = {0};

#define PGE_INVALID_FRAME        0x1001
#define PGE_ERROR_BASE           0x80000000
#define PGE_DLL_NOT_FOUND        0x80001001
#define PGE_FILE_NOT_FOUND       0x80001003
#define PGE_NOT_ENOUGH_MEMORY    0x80001004
#define PGE_INVALID_CONTEXT      0x80001005
#define PGE_FUNCTION_NOT_FOUND   0x80001006
#define PGE_WIN32_ERROR          0x80001007
#define PGE_UNKNOWN_COLORSPACE   0x80001008
#define PGE_UNSUPPORTED_PITCH    0x80001009
#define PGE_INVALID_PAGEDATA     0x8000100A
#define PGE_OLD_PLUGIN           0x8000100B
#define PGE_BITBLT_FAILED        0x8000100C
#define PGE_INVALID_VERSION      0x8000100D
#define PGE_INVALID_IMGSIZE      0x8000100E
#define PGE_UNSUPPORTEDFORMAT    0x8000100F


DWORD gnLastWin32Error = 0;




#define DllGetFunction(hModule, FunctionName) FunctionName = (FunctionName##_t)GetProcAddress(hModule, #FunctionName)
//#define MALLOC(n) HeapAlloc(GetProcessHeap(), 0, n)
#define CALLOC(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n)
#define FREE(p) HeapFree(GetProcessHeap(), 0, p)
#define STRING2(x) #x
#define STRING(x) STRING2(x)
#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "

enum tag_GdiStrMagics
{
	eGdiStr_Decoder = 0x1002,
	eGdiStr_Image = 0x1003,
	eGdiStr_Bits = 0x1004,
};

#define cfTIFF 0xb96b3cb1
#define cfJPEG 0xb96b3cae
#define cfPNG  0xb96b3caf
#define cfEXIF 0xb96b3cb2
//const wchar_t* csBMP  = L"BMP";
//const wchar_t* csEMF  = L"EMF";
//const wchar_t* csWMF  = L"WMF";
//const wchar_t* csJPEG = L"JPEG";
//const wchar_t* csPNG  = L"PNG";
//const wchar_t* csGIF  = L"GIF";
//const wchar_t* csTIFF = L"TIFF";
//const wchar_t* csEXIF = L"EXIF";
//const wchar_t* csICO  = L"ICO";
//const wchar_t* Format[] = {csBMP, csEMF, csWMF, csJPEG, csPNG, csGIF, csTIFF, csEXIF, L"", L"", csICO};

struct GDIPlusDecoder
{
	DWORD   nMagic;
	HMODULE hGDIPlus;
	ULONG_PTR gdiplusToken; bool bTokenInitialized;
	HRESULT nErrNumber, nLastError;
	//bool bUseICM, bForceSelfDisplay, bCMYK2RGB;
	BOOL bCoInitialized;
	//const wchar_t* pszPluginKey;
	//BOOL bAsDisplay;
	BOOL bCancelled;


	/* ++ */ typedef Gdiplus::Status(WINAPI *GdiplusStartup_t)(OUT ULONG_PTR *token, const Gdiplus::GdiplusStartupInput *input, OUT Gdiplus::GdiplusStartupOutput *output);
	/* ++ */ typedef VOID (WINAPI *GdiplusShutdown_t)(ULONG_PTR token);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipCreateBitmapFromFile_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImageThumbnail_t)(Gdiplus::GpImage *image, UINT thumbWidth, UINT thumbHeight, Gdiplus::GpImage **thumbImage, Gdiplus::GetThumbnailImageAbort callback, VOID * callbackData);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromStream_t)(IStream* stream, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromFileICM_t)(GDIPCONST WCHAR* filename, Gdiplus::GpBitmap **bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromStreamICM_t)(IStream* stream, Gdiplus::GpBitmap **bitmap);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImageWidth_t)(Gdiplus::GpImage *image, UINT *width);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImageHeight_t)(Gdiplus::GpImage *image, UINT *height);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImagePixelFormat_t)(Gdiplus::GpImage *image, Gdiplus::PixelFormat *format);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipBitmapLockBits_t)(Gdiplus::GpBitmap* bitmap, GDIPCONST Gdiplus::GpRect* rect, UINT flags, Gdiplus::PixelFormat format, Gdiplus::BitmapData* lockedBitmapData);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipBitmapUnlockBits_t)(Gdiplus::GpBitmap* bitmap, Gdiplus::BitmapData* lockedBitmapData);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipDisposeImage_t)(Gdiplus::GpImage *image);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipImageGetFrameCount_t)(Gdiplus::GpImage *image, GDIPCONST GUID* dimensionID, UINT* count);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipImageSelectActiveFrame_t)(Gdiplus::GpImage *image, GDIPCONST GUID* dimensionID, UINT frameIndex);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetPropertyItemSize_t)(Gdiplus::GpImage *image, PROPID propId, UINT* size);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetPropertyItem_t)(Gdiplus::GpImage *image, PROPID propId, UINT propSize, Gdiplus::PropertyItem* buffer);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipImageRotateFlip_t)(Gdiplus::GpImage *image, Gdiplus::RotateFlipType rfType);
	/* ++ */ typedef Gdiplus::GpStatus(WINGDIPAPI *GdipGetImageRawFormat_t)(Gdiplus::GpImage *image, OUT GUID* format);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImageFlags_t)(Gdiplus::GpImage *image, UINT *flags);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePalette_t)(Gdiplus::GpImage *image, Gdiplus::ColorPalette *palette, INT size);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipGetImagePaletteSize_t)(Gdiplus::GpImage *image, INT *size);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipCreateFromHDC_t)(HDC hdc, Gdiplus::GpGraphics **graphics);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipDeleteGraphics_t)(Gdiplus::GpGraphics *graphics);
	typedef Gdiplus::GpStatus(WINGDIPAPI *GdipDrawImageRectRectI_t)(Gdiplus::GpGraphics *graphics, Gdiplus::GpImage *image, INT dstx, INT dsty, INT dstwidth, INT dstheight, INT srcx, INT srcy, INT srcwidth, INT srcheight, Gdiplus::GpUnit srcUnit, const Gdiplus::GpImageAttributes* imageAttributes, Gdiplus::DrawImageAbort callback, VOID * callbackData);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateBitmapFromScan0_t)(INT width, INT height, INT stride, Gdiplus::PixelFormat format, BYTE* scan0, Gdiplus::GpBitmap** bitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipFillRectangleI_t)(Gdiplus::GpGraphics *graphics, Gdiplus::GpBrush *brush, INT x, INT y, INT width, INT height);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCreateSolidFill_t)(Gdiplus::ARGB color, Gdiplus::GpSolidFill **brush);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipDeleteBrush_t)(Gdiplus::GpBrush *brush);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCloneImage_t)(Gdiplus::GpImage *image, Gdiplus::GpImage **cloneImage);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipCloneBitmapAreaI_t)(INT x, INT y, INT width, INT height, Gdiplus::PixelFormat format, Gdiplus::GpBitmap *srcBitmap, Gdiplus::GpBitmap **dstBitmap);
	//typedef Gdiplus::GpStatus (WINGDIPAPI *GdipSetImagePalette_t)(Gdiplus::GpImage *image, GDIPCONST Gdiplus::ColorPalette *palette);



	GdiplusStartup_t GdiplusStartup;
	GdiplusShutdown_t GdiplusShutdown;
	GdipCreateBitmapFromFile_t GdipCreateBitmapFromFile;
	GdipGetImageThumbnail_t GdipGetImageThumbnail;
	//GdipCreateBitmapFromStream_t GdipCreateBitmapFromStream;
	//GdipCreateBitmapFromFileICM_t GdipCreateBitmapFromFileICM;
	//GdipCreateBitmapFromStreamICM_t GdipCreateBitmapFromStreamICM;
	GdipGetImageWidth_t GdipGetImageWidth;
	GdipGetImageHeight_t GdipGetImageHeight;
	GdipGetImagePixelFormat_t GdipGetImagePixelFormat;
	//GdipBitmapLockBits_t GdipBitmapLockBits;
	//GdipBitmapUnlockBits_t GdipBitmapUnlockBits;
	GdipDisposeImage_t GdipDisposeImage;
	GdipImageGetFrameCount_t GdipImageGetFrameCount;
	GdipImageSelectActiveFrame_t GdipImageSelectActiveFrame;
	GdipGetPropertyItemSize_t GdipGetPropertyItemSize;
	GdipGetPropertyItem_t GdipGetPropertyItem;
	GdipImageRotateFlip_t GdipImageRotateFlip;
	GdipGetImageRawFormat_t GdipGetImageRawFormat;
	//GdipGetImageFlags_t GdipGetImageFlags;
	//GdipGetImagePalette_t GdipGetImagePalette;
	//GdipGetImagePaletteSize_t GdipGetImagePaletteSize;
	GdipCreateFromHDC_t GdipCreateFromHDC;
	GdipDeleteGraphics_t GdipDeleteGraphics;
	GdipDrawImageRectRectI_t GdipDrawImageRectRectI;
	//GdipCreateBitmapFromScan0_t GdipCreateBitmapFromScan0;
	//GdipFillRectangleI_t GdipFillRectangleI;
	//GdipCreateSolidFill_t GdipCreateSolidFill;
	//GdipDeleteBrush_t GdipDeleteBrush;
	//GdipCloneImage_t GdipCloneImage;
	//GdipCloneBitmapAreaI_t GdipCloneBitmapAreaI;
	//GdipSetImagePalette_t GdipSetImagePalette;

	GDIPlusDecoder()
	{
		nMagic = eGdiStr_Decoder;
		hGDIPlus = NULL; gdiplusToken = NULL; bTokenInitialized = false;
		nErrNumber = 0; nLastError = 0; //bUseICM = false; bCoInitialized = FALSE; bCMYK2RGB = false;
		//pszPluginKey = NULL;
		bCancelled = FALSE;
		//bForceSelfDisplay = false;
	}


	bool Init(struct CET_Init* pInit)
	{
		bool result = false;
		nErrNumber = 0;
		//pszPluginKey = pInit->pRegKey;
		//ReloadConfig();
		HRESULT hrCoInitialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		bCoInitialized = SUCCEEDED(hrCoInitialized);
		wchar_t FullPath[MAX_PATH*2+15]; FullPath[0] = 0;
		//if (ghModule)
		//{
		//	PVDSettings::FindFile(L"GdiPlus.dll", FullPath, sizeofarray(FullPath));
		//	hGDIPlus = LoadLibraryW(FullPath);
		//}
		//if (!hGDIPlus)
		hGDIPlus = LoadLibraryW(L"GdiPlus.dll");

		if (!hGDIPlus)
		{
			nErrNumber = PGE_DLL_NOT_FOUND;
		}
		else
		{
			DllGetFunction(hGDIPlus, GdiplusStartup);
			DllGetFunction(hGDIPlus, GdiplusShutdown);
			DllGetFunction(hGDIPlus, GdipCreateBitmapFromFile);
			DllGetFunction(hGDIPlus, GdipGetImageThumbnail);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromFileICM);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromStream);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromStreamICM);
			DllGetFunction(hGDIPlus, GdipGetImageWidth);
			DllGetFunction(hGDIPlus, GdipGetImageHeight);
			DllGetFunction(hGDIPlus, GdipGetImagePixelFormat);
			//DllGetFunction(hGDIPlus, GdipBitmapLockBits);
			//DllGetFunction(hGDIPlus, GdipBitmapUnlockBits);
			DllGetFunction(hGDIPlus, GdipDisposeImage);
			DllGetFunction(hGDIPlus, GdipImageGetFrameCount);
			DllGetFunction(hGDIPlus, GdipImageSelectActiveFrame);
			DllGetFunction(hGDIPlus, GdipGetPropertyItemSize);
			DllGetFunction(hGDIPlus, GdipGetPropertyItem);
			DllGetFunction(hGDIPlus, GdipImageRotateFlip);
			DllGetFunction(hGDIPlus, GdipGetImageRawFormat);
			//DllGetFunction(hGDIPlus, GdipGetImageFlags);
			//DllGetFunction(hGDIPlus, GdipGetImagePalette);
			//DllGetFunction(hGDIPlus, GdipGetImagePaletteSize);
			DllGetFunction(hGDIPlus, GdipCreateFromHDC);
			DllGetFunction(hGDIPlus, GdipDeleteGraphics);
			DllGetFunction(hGDIPlus, GdipDrawImageRectRectI);
			//DllGetFunction(hGDIPlus, GdipCreateBitmapFromScan0);
			//DllGetFunction(hGDIPlus, GdipFillRectangleI);
			//DllGetFunction(hGDIPlus, GdipCreateSolidFill);
			//DllGetFunction(hGDIPlus, GdipDeleteBrush);
			//DllGetFunction(hGDIPlus, GdipCloneBitmapAreaI);
			//DllGetFunction(hGDIPlus, GdipSetImagePalette);

			if (GdiplusStartup && GdiplusShutdown && GdipCreateBitmapFromFile && GdipGetImageThumbnail
			        && GdipGetImageWidth && GdipGetImageHeight && GdipGetImagePixelFormat && GdipGetImageRawFormat
			        //&& GdipBitmapLockBits && GdipBitmapUnlockBits
			        && GdipDisposeImage && GdipImageGetFrameCount && GdipImageSelectActiveFrame
			        && GdipGetPropertyItemSize && GdipGetPropertyItem && GdipImageRotateFlip
			        //&& GdipGetImagePalette && GdipGetImagePaletteSize && GdipCloneBitmapAreaI && GdipGetImageFlags
			        && GdipCreateFromHDC && GdipDeleteGraphics && GdipDrawImageRectRectI
			        //&& GdipCreateBitmapFromScan0 && GdipFillRectangleI && GdipCreateSolidFill && GdipDeleteBrush
			        //&& GdipSetImagePalette
			  )
			{
				Gdiplus::GdiplusStartupInput gdiplusStartupInput;
				Gdiplus::Status lRc = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
				result = (lRc == Gdiplus::Ok);

				if (!result)
				{
					nLastError = GetLastError();
					GdiplusShutdown(gdiplusToken); bTokenInitialized = false;
					nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
				}
				else
				{
					bTokenInitialized = true;
				}
			}
			else
			{
				nErrNumber = PGE_FUNCTION_NOT_FOUND;
			}

			if (!result)
				FreeLibrary(hGDIPlus);
		}

		if (result)
		{
			pInit->pContext = this;
			pInit->nModuleID = MODULE_GDIP;
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
		if (bTokenInitialized)
		{
			GdiplusShutdown(gdiplusToken);
			bTokenInitialized = false;
		}

		if (hGDIPlus)
		{
			//FreeLibrary(hGDIPlus);
			DWORD nFreeTID = 0, nWait = 0, nErr = 0;
			HANDLE hFree = apiCreateThread(FreeThreadProc, this, &nFreeTID, "gdip::FreeThreadProc");

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

			hGDIPlus = NULL;
		}

		if (bCoInitialized)
		{
			bCoInitialized = FALSE;
			CoUninitialize();
		}

		FREE(this);
	};
};

#define DEFINE_GUID_(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY name \
	= { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID_(FrameDimensionTime, 0x6aedbd6d,0x3fb5,0x418a,0x83,0xa6,0x7f,0x45,0x22,0x9d,0xc8,0x72);
DEFINE_GUID_(FrameDimensionPage, 0x7462dc86,0x6180,0x4c7e,0x8e,0x3f,0xee,0x73,0x33,0xa7,0xa4,0x83);

struct GDIPlusImage;

struct GDIPlusData
{
	DWORD nMagic;
	struct GDIPlusImage *pImg;
	HDC hCompDc1;
	HBITMAP hDIB, hOld1;
	wchar_t szInfo[255];

	GDIPlusData()
	{
		nMagic = eGdiStr_Bits;
	};

	void Close()
	{
		if (hCompDc1 && hOld1)
			SelectObject(hCompDc1, hOld1);

		DeleteObject(hDIB);
		DeleteDC(hCompDc1);
		FREE(this);
	};
};

struct GDIPlusImage
{
	DWORD nMagic;

	wchar_t szTempFile[MAX_PATH+1];
	GDIPlusDecoder *gdi;
	Gdiplus::GpImage *img;
	MStream* strm;
	HRESULT nErrNumber, nLastError;
	//
	UINT lWidth, lHeight, pf, nBPP, nPages, /*lFrameTime,*/ nActivePage, nTransparent; //, nImgFlags;
	bool Animation;
	wchar_t FormatName[5];
	DWORD nFormatID;

	GDIPlusImage()
	{
		nMagic = eGdiStr_Image;
		szTempFile[0] = 0;
	};


	//Gdiplus::GpBitmap* OpenBitmapFromStream(const uint8_t *pBuffer, int64_t lFileSize)
	//{
	//	Gdiplus::Status lRc = Gdiplus::Ok;
	//	Gdiplus::GpBitmap *img = NULL;

	//	// IStream используется в процессе декодирования, поэтому его нужно держать созданным
	//	strm = new MStream();
	//	if (strm) {
	//		nLastError = strm->Write(pBuffer, (ULONG)lFileSize, NULL);
	//
	//		if (nLastError == S_OK) {
	//			LARGE_INTEGER ll; ll.QuadPart = 0;
	//			strm->Seek(ll, STREAM_SEEK_SET, NULL);
	//
	//			lRc = gdi->GdipCreateBitmapFromStream(strm, &img);
	//		}
	//	}
	//
	//	if (!img) {
	//		nLastError = GetLastError();
	//		nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
	//	}
	//	return img;
	//}
	Gdiplus::GpImage* OpenBitmapFromFile(const wchar_t *pFileName, COORD crLoadSize)
	{
		Gdiplus::Status lRc = Gdiplus::Ok;
		Gdiplus::GpBitmap *bmp = NULL;
		_ASSERTE(img==NULL);
		lRc = gdi->GdipCreateBitmapFromFile(pFileName, &bmp);

		if (!bmp)
		{
			nLastError = GetLastError();
			nErrNumber = PGE_ERROR_BASE + (DWORD)lRc;
		}

		return bmp;
	}

	bool Open(bool bVirtual, const wchar_t *pFileName, const uint8_t *pBuffer, int64_t lFileSize, COORD crLoadSize)
	{
		_ASSERTE(img == NULL);
		_ASSERTE(gdi != NULL);
		bool result = false;
		Gdiplus::Status lRc;
		nActivePage = -1; nTransparent = -1; //nImgFlags = 0;

		if (bVirtual && pBuffer && lFileSize)
		{
			DWORD n;
			wchar_t szTempDir[MAX_PATH];
			n = GetTempPath(MAX_PATH-16, szTempDir);

			if (n && n < (MAX_PATH-16))
			{
				n = GetTempFileName(szTempDir, L"CET", 0, szTempFile);

				if (n)
				{
					HANDLE hFile = CreateFile(szTempFile, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);

					if (hFile == INVALID_HANDLE_VALUE)
					{
						szTempFile[0] = 0; // не создали, значит и удалять будет нечего
					}
					else
					{
						DWORD nSize = (DWORD)lFileSize; DWORD nWritten = 0;

						if (WriteFile(hFile, pBuffer, nSize, &nWritten, NULL) && nWritten == nSize)
						{
							bVirtual = false; pFileName = szTempFile;
						}

						CloseHandle(hFile);

						if (bVirtual)
						{
							DeleteFile(szTempFile); szTempFile[0] = 0;
						}
					}
				}
			}
		}

		if (bVirtual)
		{
			nErrNumber = PGE_FILE_NOT_FOUND;
			return false;
		}

		//if (!bVirtual)
		img = OpenBitmapFromFile(pFileName, crLoadSize);
		//else // лучше бы его вообще не использовать, GDI+ как-то не очень с потоками работает...
		//	img = OpenBitmapFromStream(pBuffer, lFileSize);

		if (!img)
		{
			//nErrNumber = gdi->nErrNumber; -- ошибка УЖЕ в nErrNumber
		}
		else
		{
			lRc = gdi->GdipGetImageWidth(img, &lWidth);
			lRc = gdi->GdipGetImageHeight(img, &lHeight);
			lRc = gdi->GdipGetImagePixelFormat(img, (Gdiplus::PixelFormat*)&pf);
			nBPP = pf >> 8 & 0xFF;
			//lRc = gdi->GdipGetImageFlags(img, &nImgFlags);
			Animation = false; nPages = 1;

			if (!(lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionTime, &nPages)))
				Animation = nPages > 1;
			else if ((lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionPage, &nPages)))
				nPages = 1;

			FormatName[0] = 0;

			if (gdi->GdipGetImageRawFormat)
			{
				GUID gformat;

				if (!(lRc = gdi->GdipGetImageRawFormat(img, &gformat)))
				{
					// DEFINE_GUID(ImageFormatUndefined, 0xb96b3ca9,0x0728,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
					// DEFINE_GUID(ImageFormatMemoryBMP, 0xb96b3caa,0x0728,0x11d3,0x9d,0x7b,0x00,0x00,0xf8,0x1e,0xf3,0x2e);
					const wchar_t Format[][5] = {L"BMP", L"EMF", L"WMF", L"JPEG", L"PNG", L"GIF", L"TIFF", L"EXIF", L"", L"", L"ICO"};

					if (gformat.Data1 >= 0xB96B3CAB && gformat.Data1 <= 0xB96B3CB5)
					{
						//FormatName = Format[gformat.Data1 - 0xB96B3CAB];
						nFormatID = gformat.Data1;
						lstrcpy(FormatName, Format[gformat.Data1 - 0xB96B3CAB]);
					}
				}
			}

			result = SelectPage(0);
		}

		return result;
	};
	bool SelectPage(UINT iPage)
	{
		bool result = false;
		Gdiplus::Status lRc;

		if ((lRc = gdi->GdipImageGetFrameCount(img, &FrameDimensionTime, &pf)))
			lRc = gdi->GdipImageSelectActiveFrame(img, &FrameDimensionPage, iPage);
		else
			lRc = gdi->GdipImageSelectActiveFrame(img, &FrameDimensionTime, iPage);

		lRc = gdi->GdipGetImageWidth(img, &lWidth);
		lRc = gdi->GdipGetImageHeight(img, &lHeight);
		lRc = gdi->GdipGetImagePixelFormat(img, (Gdiplus::PixelFormat*)&pf);
		nBPP = pf >> 8 & 0xFF;
		nActivePage = iPage;
		result = true;
		return result;
	};
	void Close()
	{
		if (!gdi) return;

		Gdiplus::Status lRc;

		if (img)
		{
			lRc = gdi->GdipDisposeImage(img);
			img = NULL;
		}

		if (strm)
		{
			delete strm;
			strm = NULL;
		}

		if (szTempFile[0])
		{
			DeleteFile(szTempFile);
		}

		FREE(this);
	};
	static BOOL CALLBACK DrawImageAbortCallback(GDIPlusDecoder *pGDI)
	{
		if (pGDI)
			return pGDI->bCancelled;

		return FALSE;
	};
	bool GetExifTagValueAsInt(PROPID pid, int& nValue)
	{
		bool bExists = false;
		Gdiplus::Status lRc;
		nValue = 0;
		UINT lPropSize;

		if (!(lRc = gdi->GdipGetPropertyItemSize(img, pid, &lPropSize)))
		{
			Gdiplus::PropertyItem* p = (Gdiplus::PropertyItem*)CALLOC(lPropSize);

			if (!(lRc = gdi->GdipGetPropertyItem(img, pid, lPropSize, p)))
			{
				switch(p->type)
				{
					case PropertyTagTypeByte:
						nValue = *(BYTE*)p->value; bExists = true;
						break;
					case PropertyTagTypeShort:
						nValue = *(short*)p->value; bExists = true;
						break;
					case PropertyTagTypeLong: case PropertyTagTypeSLONG:
						nValue = *(int*)p->value; bExists = true;
						break;
				}
			}

			FREE(p);
		}

		return bExists;
	};
	void CalculateShowSize(int& nCanvasWidth, int& nCanvasHeight, int& nShowWidth, int& nShowHeight, BOOL& lbAllowThumb)
	{
		nShowWidth = lWidth;
		nShowHeight = lHeight;
		int64_t aSrc = (100 * (int64_t) lWidth / lHeight);
		int64_t aCvs = (100 * (int64_t) nCanvasWidth / nCanvasHeight);

		if (aSrc > aCvs)
		{
			if (lWidth >= (UINT)nCanvasWidth)
			{
				nShowWidth = nCanvasWidth;
				nShowHeight = (int)((((int64_t)lHeight) * nCanvasWidth) / lWidth); //-V537

				if (!nShowHeight || nShowHeight < (nShowWidth/8))
				{
					nShowHeight = std::min(std::min<UINT>(8, (UINT)nCanvasHeight), lHeight);
					UINT lNewWidth = (UINT)((((int64_t)nCanvasWidth) * lHeight) / nShowHeight);

					if (lNewWidth < lWidth)
					{
						lWidth = lNewWidth;
						lbAllowThumb = FALSE;
					}
				}
			}
		}
		else
		{
			if (lHeight >= (UINT)nCanvasHeight)
			{
				nShowWidth = (int)((((int64_t)lWidth) * nCanvasHeight) / lHeight); //-V537
				nShowHeight = nCanvasHeight;

				if (!nShowWidth || nShowWidth < (nShowHeight/8))
				{
					nShowWidth = std::min(std::min<UINT>(8, (UINT)nCanvasWidth), lWidth);
					UINT lNewHeight = (UINT)((((int64_t)nCanvasHeight) * lWidth) / nShowWidth);

					if (lNewHeight < lHeight)
					{
						lHeight = lNewHeight;
						lbAllowThumb = FALSE;
					}
				}
			}
		}

		nCanvasWidth  = nShowWidth;
		nCanvasHeight = nShowHeight;
	};
	bool GetPageBits(CET_LoadInfo *pDecodeInfo)
	{
		bool result = false;

		if (!lWidth || !lHeight)
		{
			pDecodeInfo->nErrNumber = PGE_INVALID_IMGSIZE;
			return false;
		}

		GDIPlusData *pData = (GDIPlusData*)CALLOC(sizeof(GDIPlusData));

		if (!pData)
		{
			pDecodeInfo->nErrNumber = PGE_NOT_ENOUGH_MEMORY;
		}
		else
		{
			pData->nMagic = eGdiStr_Bits;
			pData->pImg = this;
			pDecodeInfo->pFileContext = pData;
			wsprintf(pData->szInfo, L"%i x %i x %ibpp", lWidth, lHeight, nBPP);

			if (nPages > 1) wsprintf(pData->szInfo+lstrlen(pData->szInfo), L" [%i]", nPages);

			if (FormatName[0])
			{
				lstrcat(pData->szInfo, L" ");
				lstrcat(pData->szInfo, FormatName);
			}

			int nCanvasWidth  = pDecodeInfo->crLoadSize.X;
			int nCanvasHeight = pDecodeInfo->crLoadSize.Y;
			BOOL lbAllowThumb = (nFormatID == cfPNG || nFormatID == cfTIFF || nFormatID == cfEXIF || nFormatID == cfJPEG);
			//&& (lWidth > (UINT)nCanvasWidth*5) && (lHeight > (UINT)nCanvasHeight*5);
			int nShowWidth, nShowHeight;
			CalculateShowSize(nCanvasWidth, nCanvasHeight, nShowWidth, nShowHeight, lbAllowThumb);
			// Получим из EXIF ориентацию
			int nOrient;

			if (!GetExifTagValueAsInt(PropertyTagOrientation, nOrient)) nOrient = 0;

			if (lbAllowThumb && nOrient)
			{
				Gdiplus::GpImage *thmb = NULL;
				// Сразу пытаемся извлечь в режиме превьюшки (полная картинка нам не нужна)
				Gdiplus::Status lRc = gdi->GdipGetImageThumbnail(img, nShowWidth, nShowHeight, &thmb,
				                      (Gdiplus::GetThumbnailImageAbort)DrawImageAbortCallback, gdi);

				if (thmb)
				{
					lRc = gdi->GdipDisposeImage(img);
					img = thmb;
					lRc = gdi->GdipGetImageWidth(img, &lWidth);
					lRc = gdi->GdipGetImageHeight(img, &lHeight);
				}

				// Теперь - крутим
				Gdiplus::RotateFlipType rft = Gdiplus::RotateNoneFlipNone;

				switch(nOrient)
				{
					case 3: rft = Gdiplus::Rotate180FlipNone; break;
					case 6: rft = Gdiplus::Rotate90FlipNone; break;
					case 8: rft = Gdiplus::Rotate270FlipNone; break;
					case 2: rft = Gdiplus::RotateNoneFlipX; break;
					case 4: rft = Gdiplus::RotateNoneFlipY; break;
					case 5: rft = Gdiplus::Rotate90FlipX; break;
					case 7: rft = Gdiplus::Rotate270FlipX; break;
				}

				if (rft)
				{
					lRc = gdi->GdipImageRotateFlip(img, rft);

					if (!lRc)
					{
						lRc = gdi->GdipGetImageWidth(img, &lWidth);
						lRc = gdi->GdipGetImageHeight(img, &lHeight); //-V519
						nCanvasWidth  = pDecodeInfo->crLoadSize.X;
						nCanvasHeight = pDecodeInfo->crLoadSize.Y;
						CalculateShowSize(nCanvasWidth, nCanvasHeight, nShowWidth, nShowHeight, lbAllowThumb);
					}
				}
			}

			nCanvasWidth  = nShowWidth;
			nCanvasHeight = nShowHeight;
			int nCanvasWidthS = nCanvasWidth; //((nCanvasWidth+7) >> 3) << 3; // try to align x8 pixels
			pData->hCompDc1 = CreateCompatibleDC(NULL);
			BITMAPINFOHEADER bmi = {sizeof(BITMAPINFOHEADER)};
			bmi.biWidth = nCanvasWidthS;
			bmi.biHeight = -nCanvasHeight; // Top-Down DIB
			bmi.biPlanes = 1;
			bmi.biBitCount = 32;
			bmi.biCompression = BI_RGB;
			LPBYTE pBits = NULL;
			pData->hDIB = CreateDIBSection(pData->hCompDc1, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, (void**)&pBits, NULL, 0);

			if (!pData->hDIB)
			{
				_ASSERTE(pData->hDIB);
			}
			else
			{
				pData->hOld1 = (HBITMAP)SelectObject(pData->hCompDc1, pData->hDIB);
				RECT rcFull = {0,0,nCanvasWidthS, nCanvasHeight};
				HBRUSH hBr = CreateSolidBrush(pDecodeInfo->crBackground);
				FillRect(pData->hCompDc1, &rcFull, hBr);
				DeleteObject(hBr);
				Gdiplus::GpGraphics *pGr = NULL;
				Gdiplus::Status stat = gdi->GdipCreateFromHDC(pData->hCompDc1, &pGr);

				if (!stat)
				{
#ifdef _DEBUG

					if (nCanvasWidth!=nShowWidth || nCanvasHeight!=nShowHeight)
					{
						_ASSERTE(nCanvasWidth==nShowWidth && nCanvasHeight==nShowHeight);
					}

#endif
					//int x = (nCanvasWidth-nShowWidth)>>1;
					//int y = (nCanvasHeight-nShowHeight)>>1;
					stat = gdi->GdipDrawImageRectRectI(
					           pGr, img,
					           0, 0, nShowWidth, nShowHeight,
					           0, 0, lWidth, lHeight,
					           Gdiplus::UnitPixel, NULL, //NULL, NULL);
					           (Gdiplus::DrawImageAbort)DrawImageAbortCallback, gdi);
					gdi->GdipDeleteGraphics(pGr);
				}

				if (stat)
				{
					pDecodeInfo->nErrNumber = PGE_BITBLT_FAILED;
				}
				else
				{
					result = true;
					pDecodeInfo->pFileContext = (LPVOID)pData;
					pDecodeInfo->crSize.X = nCanvasWidth; pDecodeInfo->crSize.Y = nCanvasHeight;
					pDecodeInfo->cbStride = nCanvasWidthS * 4;
					pDecodeInfo->nBits = 32;
					pDecodeInfo->ColorModel = CET_CM_BGR;
					pDecodeInfo->pszComments = pData->szInfo;
					pDecodeInfo->cbPixelsSize = pDecodeInfo->cbStride * nCanvasHeight;
					pDecodeInfo->Pixels = (const DWORD*)pBits;
				}
			}

			pData->pImg = NULL;

			if (!result)
			{
				pDecodeInfo->pFileContext = this;
				pData->Close();
			}
		}

		return result;
	};
};





extern "C" BOOL WINAPI CET_Init(struct CET_Init* pInit)
{
	HeapInitialize();
	_ASSERTE(pInit->cbSize >= sizeof(struct CET_Init));

	if (pInit->cbSize < sizeof(struct CET_Init))
	{
		pInit->nErrNumber = PGE_OLD_PLUGIN;
		return FALSE;
	}

	ghModule = pInit->hModule;
	GDIPlusDecoder *pDecoder = (GDIPlusDecoder*) CALLOC(sizeof(GDIPlusDecoder));

	if (!pDecoder)
	{
		pInit->nErrNumber = PGE_NOT_ENOUGH_MEMORY;
		return FALSE;
	}

	pDecoder->nMagic = eGdiStr_Decoder;

	if (!pDecoder->Init(pInit))
	{
		pInit->nErrNumber = pDecoder->nErrNumber;
		pDecoder->Close();
		//FREE(pDecoder);
		return FALSE;
	}

	_ASSERTE(pInit->pContext == (LPVOID)pDecoder); // Уже вернул pDecoder->Init
	return TRUE;
}

extern "C" VOID WINAPI CET_Done(struct CET_Init* pInit)
{
	if (pInit)
	{
		GDIPlusDecoder *pDecoder = (GDIPlusDecoder*)pInit->pContext;

		if (pDecoder)
		{
			_ASSERTE(pDecoder->nMagic == eGdiStr_Decoder);

			if (pDecoder->nMagic == eGdiStr_Decoder)
			{
				pDecoder->Close();
				//FREE(pDecoder);
			}
		}
	}

	HeapDeinitialize();
}


#define SETERROR(n) if (pLoadPreview) pLoadPreview->nErrNumber = n;


extern "C" BOOL WINAPI CET_Load(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo))
	{
		_ASSERTE(*(LPDWORD)pLoadPreview == sizeof(struct CET_LoadInfo));
		SETERROR(PGE_INVALID_VERSION);
		return FALSE;
	}

	if (!pLoadPreview->pContext)
	{
		SETERROR(PGE_INVALID_CONTEXT);
		return FALSE;
	}

	if (pLoadPreview->bVirtualItem && (!pLoadPreview->pFileData || !pLoadPreview->nFileSize))
	{
		SETERROR(PGE_FILE_NOT_FOUND);
		return FALSE;
	}

	if (pLoadPreview->nFileSize < 16 || pLoadPreview->nFileSize > 209715200/*200 MB*/)
	{
		SETERROR(PGE_UNSUPPORTEDFORMAT);
		return FALSE;
	}

	BOOL lbKnown = FALSE;

	if (pLoadPreview->pFileData)
	{
		const BYTE  *pb  = (const BYTE*)pLoadPreview->pFileData;
		const WORD  *pw  = (const WORD*)pLoadPreview->pFileData;
		const DWORD *pdw = (const DWORD*)pLoadPreview->pFileData;

		//TODO("ICO - убрать. пусть специализированный занимается")

		if (*pdw==0x474E5089 /* ‰PNG */)
			lbKnown = TRUE;
		else if (*pw==0x4D42 /* BM */)
			lbKnown = TRUE;
		else if (pb[0]==0xFF && pb[1]==0xD8 && pb[2]==0xFF)  // JPEG?
			lbKnown = TRUE;
		else if (pw[0]==0x4949)  // TIFF?
			lbKnown = TRUE;
		else if (pw[0]==0 && (pw[1]==1/*ICON*/ || pw[1]==2/*CURSOR*/) && (pw[2]>0 && pw[2]<=64/*IMG COUNT*/))  // .ico, .cur
			lbKnown = FALSE; // -> ico.t32
		else if (*pdw == 0x38464947 /*GIF8*/)
			lbKnown = TRUE;
		else
		{
			const wchar_t* pszFile = NULL;
			wchar_t szExt[6]; szExt[0] = 0;
			pszFile = wcsrchr(pLoadPreview->sFileName, L'\\');

			if (pszFile)
			{
				pszFile = wcsrchr(pszFile, L'.');

				if (pszFile)
				{
					int nLen = lstrlenW(pszFile);

					while(nLen > 0 && pszFile[nLen-1] == L' ')
						nLen --;

					if (nLen && nLen<5)
					{
						lstrcpynW(szExt, pszFile+1, nLen+1);
						szExt[nLen+1] = 0;
						CharLowerBuffW(szExt, nLen-1);
					}
				}
			}

			if ((szExt[0]==L'w' || szExt[0]==L'e') && szExt[1]==L'm' && szExt[2]==L'f')
				lbKnown = TRUE;
		}
	}

	if (!lbKnown)
	{
		SETERROR(PGE_UNSUPPORTEDFORMAT);
		return FALSE;
	}

	GDIPlusImage *pImage = (GDIPlusImage*)CALLOC(sizeof(GDIPlusImage));

	if (!pImage)
	{
		SETERROR(PGE_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	pImage->nMagic = eGdiStr_Image;
	pLoadPreview->pFileContext = (void*)pImage;
	pImage->gdi = (GDIPlusDecoder*)pLoadPreview->pContext;
	pImage->gdi->bCancelled = FALSE;

	if (!pImage->Open(
	            (pLoadPreview->bVirtualItem!=FALSE), pLoadPreview->sFileName,
	            pLoadPreview->pFileData, pLoadPreview->nFileSize, pLoadPreview->crLoadSize))
	{
		SETERROR(pImage->nErrNumber);
		pImage->Close();
		pLoadPreview->pFileContext = NULL;
		return FALSE;
	}

	if (pLoadPreview)
	{
		if (!pImage->GetPageBits(pLoadPreview))
		{
			SETERROR(pImage->nErrNumber);
			pImage->Close();
			pLoadPreview->pFileContext = NULL;
			return FALSE;
		}
	}

	if (pLoadPreview->pFileContext == (void*)pImage)
		pLoadPreview->pFileContext = NULL;

	pImage->Close();
	return TRUE;
}

extern "C" VOID WINAPI CET_Free(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo))
	{
		_ASSERTE(*(LPDWORD)pLoadPreview == sizeof(struct CET_LoadInfo));
		SETERROR(PGE_INVALID_VERSION);
		return;
	}

	if (!pLoadPreview->pFileContext)
	{
		SETERROR(PGE_INVALID_CONTEXT);
		return;
	}

	switch(*(LPDWORD)pLoadPreview->pFileContext)
	{
		case eGdiStr_Image:
		{
			// Сюда мы попадем если был exception в CET_Load
			GDIPlusImage *pImg = (GDIPlusImage*)pLoadPreview->pFileContext;
			pImg->Close();
		} break;
		case eGdiStr_Bits:
		{
			GDIPlusData *pData = (GDIPlusData*)pLoadPreview->pFileContext;

			if (pData->pImg)
			{
				pData->pImg->Close();
				pData->pImg = NULL;
			}

			pData->Close();
		} break;
#ifdef _DEBUG
		default:
			_ASSERTE(*(LPDWORD)pLoadPreview->pFileContext == eGdiStr_Bits);
#endif
	}
}

extern "C" VOID WINAPI CET_Cancel(LPVOID pContext)
{
	if (!pContext) return;

	GDIPlusDecoder *pDecoder = (GDIPlusDecoder*)pContext;

	if (pDecoder->nMagic == eGdiStr_Decoder)
	{
		pDecoder->bCancelled = TRUE;
	}
}
