
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
#include <shellapi.h>
using namespace std;
#include <gdiplus.h>
#include <crtdbg.h>
#include "../../../common/MStrSafe.h"
#include "../../../common/defines.h"
#include "../../../common/Memory.h"
#include "../ThumbSDK.h"
#include "../MStream.h"

#pragma comment(lib, "gdiplus.lib")

// Некритичные ошибки ( < 0x7FFFFFFF )
// При подборе декодера в PicView нужно возвращать именно их ( < 0x7FFFFFFF )
// для тех файлов (форматов), которые неизвестны данному декодеру.
// PicView не будет отображать эти ошибки пользователю.
#define PIE_NOT_ICO_FILE             0x1001

// Далее идут критичные ошибки. Информация об ошибке будет показана
// пользователю, если PicView не сможет открыть файл никаким декодером
#define PIE_NO_IMAGES                0x80000001
#define PIE_BUFFER_EMPTY             0x80000002
#define PIE_WIN32_ERROR              0x80000003
#define PIE_TOO_LARGE_FILE           0x80000004
#define PIE_NOT_ENOUGH_MEMORY        0x80000005
#define PIE_INVALID_CONTEXT          0x80000006
#define PIE_INVALID_NUMBER           0x80000007
#define PIE_JUMP_OUT_OF_FILE         0x80000008
#define PIE_INVALID_BPP              0x80000009
#define PIE_UNSUPPORTED_BPP          0x8000000A
#define PIE_OLD_PLUGIN               0x8000000B
#define PIE_BUFFER_NOT_FULL          0x8000000C
#define PIE_INVALID_VERSION          0x8000000D
#define PIE_FILE_NOT_FOUND           0x8000000E
#define PIE_UNSUPPORTEDFORMAT        0x8000000F
DWORD gnLastWin32Error = 0;
//pvdInitPlugin2 ip = {0};

#define PVD_MAX_ICON_SIZE 0x100000 /*1Mb*/

#define CALLOC(n) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, n)
#define FREE(p) HeapFree(GetProcessHeap(), 0, p)
//#define STRING2(x) #x
//#define STRING(x) STRING2(x)
//#define FILE_LINE __FILE__ "(" STRING(__LINE__) "): "
//#ifdef HIDE_TODO
//#define TODO(s)
//#define WARNING(s)
//#else
//#define TODO(s) __pragma(message (FILE_LINE "TODO: " s))
//#define WARNING(s) __pragma(message (FILE_LINE "warning: " s))
//#endif
//#define PRAGMA_ERROR(s) __pragma(message (FILE_LINE "error: " s))


HMODULE ghModule = NULL;


#pragma pack( push )
#pragma pack( 1 )

typedef struct
{
	BYTE        bWidth;          // Width, in pixels, of the image
	BYTE        bHeight;         // Height, in pixels, of the image
	BYTE        bColorCount;     // Number of colors in image (0 if >=8bpp)
	BYTE        bReserved;       // Reserved ( must be 0)
	WORD        wPlanes;         // Color Planes
	WORD        wBitCount;       // Bits per pixel
	DWORD       dwBytesInRes;    // How many bytes in this resource?
	DWORD       dwImageOffset;   // Where in the file is this image?
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	WORD           idReserved;   // Reserved (must be 0)
	WORD           idType;       // Resource Type (1 for icons, 2 for cursors)
	WORD           idCount;      // How many images?
	ICONDIRENTRY   idEntries[1]; // An entry for each image (idCount of 'em)
} ICONDIR, *LPICONDIR;

typedef struct
{
	DWORD          nDataSize;
	ICONDIR        Icon;
} ICONCONTEXT, *LPICONCONTEXT;

/*typedef struct
{
	MStream *pStrm;
	Gdiplus::Bitmap *pBmp;
} GDIPLUSICON, *LPGDIPLUSICON;*/

//typedef struct
//{
//   BITMAPINFOHEADER   icHeader;      // DIB header
//   RGBQUAD         icColors[1];   // Color table
//   BYTE            icXOR[1];      // DIB bits for XOR mask
//   BYTE            icAND[1];      // DIB bits for AND mask
//} ICONIMAGE, *LPICONIMAGE;

// DLL and EXE Files
// Icons can also be stored in .DLL and .EXE files. The structures used to store icon images in .EXE and .DLL files differ only slightly from those used in .ICO files.
// Analogous to the ICONDIR data in the ICO file is the RT_GROUP_ICON resource. In fact, one RT_GROUP_ICON resource is created for each ICO file bound to the EXE or DLL with the resource compiler/linker. The RT_GROUP_ICON resource is simply a GRPICONDIR structure:
// #pragmas are used here to insure that the structure's
// packing in memory matches the packing of the EXE or DLL.
typedef struct
{
	BYTE   bWidth;               // Width, in pixels, of the image
	BYTE   bHeight;              // Height, in pixels, of the image
	BYTE   bColorCount;          // Number of colors in image (0 if >=8bpp)
	BYTE   bReserved;            // Reserved
	WORD   wPlanes;              // Color Planes
	WORD   wBitCount;            // Bits per pixel
	DWORD  dwBytesInRes;         // how many bytes in this resource?
	WORD   nID;                  // the ID
} GRPICONDIRENTRY, *LPGRPICONDIRENTRY;

typedef struct
{
	WORD            idReserved;   // Reserved (must be 0)
	WORD            idType;       // Resource type (1 for icons, 2 for cursors)
	WORD            idCount;      // How many images?
	GRPICONDIRENTRY   idEntries[1]; // The entries for each image
} GRPICONDIR, *LPGRPICONDIR;
#pragma pack( pop )

/*

// Load the DLL/EXE without executing its code
hLib = LoadLibraryEx( szFileName, NULL, LOAD_LIBRARY_AS_DATAFILE );
// Find the group resource which lists its images
hRsrc = FindResource( hLib, MAKEINTRESOURCE( nId ), RT_GROUP_ICON );
// Load and Lock to get a pointer to a GRPICONDIR
hGlobal = LoadResource( hLib, hRsrc );
lpGrpIconDir = LockResource( hGlobal );
// Using an ID from the group, Find, Load and Lock the RT_ICON
hRsrc = FindResource( hLib, MAKEINTRESOURCE( lpGrpIconDir->idEntries[0].nID ),
                      RT_ICON );
hGlobal = LoadResource( hLib, hRsrc );
lpIconImage = LockResource( hGlobal );
// Here, lpIconImage points to an ICONIMAGE structure

*/

//BOOL __stdcall pvdFileOpen2(void *pContext, const wchar_t *pFileName, INT64 lFileSize, const BYTE *pBuf, UINT32 lBuf, pvdInfoImage2 *pImageInfo)
//{
//	_ASSERTE(pImageInfo->cbSize >= sizeof(pvdInfoImage2));
//	_ASSERTE(pBuf && lBuf);
//
//	// При открытии ресурсов - pBuf это BITMAPINFOHEADER
//	// Соответственно, при просмотре ресурса - один файл это строго одна иконка,
//	// информацию нужно грузить строго из BITMAPINFOHEADER
//	// Тут может быть и PNG, но это нас уже не интересует. PNG может быть открыт любым декодером...
//
//	LPICONCONTEXT pIcon = NULL;
//	LPICONDIR pTest = (LPICONDIR)pBuf;
//	LPBITMAPINFOHEADER pBmp = (LPBITMAPINFOHEADER)pBuf;
//
//	if (!pBuf || !lBuf || (lFileSize && lFileSize != (__int64)lBuf)) {
//		nErrNumber = PIE_BUFFER_NOT_FULL;
//		return FALSE;
//	}
//
//	if (lBuf > sizeof(ICONDIR) && pTest->idReserved==0 && (pTest->idType==1 /* ICON */ || pTest->idType == 2 /* CURSOR */)) {
//		if (lFileSize > PVD_MAX_ICON_SIZE) {
//			nErrNumber = PIE_TOO_LARGE_FILE;
//			return FALSE;
//		}
//		if (pTest->idCount == 0) {
//			nErrNumber = PIE_NO_IMAGES;
//			return FALSE;
//		}
//
//		// Делаем копию буфера
//		pIcon = (LPICONCONTEXT)CALLOC(lBuf+sizeof(DWORD));
//		if (!pIcon) {
//			nErrNumber = PIE_NOT_ENOUGH_MEMORY;
//			return FALSE;
//		}
//		pIcon->nDataSize = lBuf;
//		memmove(&(pIcon->Icon), pBuf, lBuf);
//
//	} else {
//		// Test BITMAPINFOHEADER
//		if (lBuf > sizeof(BITMAPINFOHEADER) && pBmp->biSize == sizeof(BITMAPINFOHEADER)
//			&& (pBmp->biWidth && pBmp->biWidth < 256)
//			&& (pBmp->biHeight == (pBmp->biWidth * 2)))
//		{
//			// Делаем копию буфера, но предваряем его заголовком иконки
//			DWORD nSize = lBuf + sizeof(ICONDIR);
//			pIcon = (LPICONCONTEXT)CALLOC(nSize+sizeof(DWORD));
//			if (!pIcon) {
//				nErrNumber = PIE_NOT_ENOUGH_MEMORY;
//				return FALSE;
//			}
//			pIcon->nDataSize = nSize;
//			pIcon->Icon.idReserved = 0;
//			pIcon->Icon.idType = 1;
//			pIcon->Icon.idCount = 1;
//			pIcon->Icon.idEntries[0].bWidth = (BYTE)pBmp->biWidth;
//			pIcon->Icon.idEntries[0].bHeight = (BYTE)pBmp->biWidth;
//			pIcon->Icon.idEntries[0].bColorCount = (pBmp->biBitCount >= 8) ? 0 : (1 << pBmp->biBitCount);
//			pIcon->Icon.idEntries[0].bReserved = 0;
//			pIcon->Icon.idEntries[0].wPlanes = pBmp->biPlanes;
//			pIcon->Icon.idEntries[0].wBitCount = pBmp->biBitCount;
//			pIcon->Icon.idEntries[0].dwBytesInRes = lBuf;
//			pIcon->Icon.idEntries[0].dwImageOffset = sizeof(ICONDIR);
//			memmove(&(pIcon->Icon.idEntries[1]), pBuf, lBuf);
//
//		} else {
//			nErrNumber = PIE_NOT_ICO_FILE;
//			return FALSE;
//		}
//	}
//
//
//	WARNING("Хорошо бы возвращать умолчательный индекс отображаемой иконки, если первая НЕ содержит изображения вообще");
//	// file_view_hc.ico - первый фрейм вообще пустой (полностью прозрачный), а второй содержит картинку
//
//
//	if (pIcon->nDataSize < (sizeof(ICONDIR) + (pIcon->Icon.idCount-1)*sizeof(ICONDIRENTRY))) {
//		nErrNumber = PIE_JUMP_OUT_OF_FILE;
//		FREE(pIcon);
//		return FALSE;
//	}
//
//	pImageInfo->pImageContext = pIcon;
//
//	pImageInfo->nPages = pIcon->Icon.idCount;
//	pImageInfo->Flags = 0;
//	pImageInfo->pFormatName = pIcon->Icon.idType!=2 ? L"ICO" : L"CUR";
//	pImageInfo->pCompression = NULL;
//	pImageInfo->pComments = NULL;
//	pImageInfo->pImageContext = pIcon;
//	return TRUE;
//}

DWORD LoadPageInfo(LPICONCONTEXT pIcon, int iPage, UINT32& nBPP, UINT32& nWidth, UINT32& nHeight, UINT& nColors, const wchar_t** ppFormat)
{
	if (iPage >= (int)pIcon->Icon.idCount || iPage < 0)
		return PIE_INVALID_NUMBER;

	nWidth = pIcon->Icon.idEntries[iPage].bWidth;

	if (nWidth == 0) nWidth = 256;

	nHeight = pIcon->Icon.idEntries[iPage].bHeight;

	if (nHeight == 0) nHeight = 256;

	nBPP = 0; nColors = pIcon->Icon.idEntries[iPage].bColorCount;

	switch(pIcon->Icon.idEntries[iPage].bColorCount)
	{
		case 1: _ASSERTE(pIcon->Icon.idEntries[iPage].bColorCount!=1); break;
		case 2: nBPP = 1; break;
		case 4: nBPP = 2; break;
		case 8: nBPP = 3; break;
		case 16: nBPP = 4; break; //-V112
		case 32: nBPP = 5; break;
		case 64: nBPP = 6; break;
		case 128: nBPP = 7; break;
		default:
			_ASSERTE(pIcon->Icon.idEntries[iPage].bColorCount == 0);
			nBPP = pIcon->Icon.idEntries[iPage].wBitCount * pIcon->Icon.idEntries[iPage].wPlanes;
			//if (nBPP == 0)			nBPP = 8;
			//nColors = 1 << nBPP;
	}

	LPICONDIRENTRY pImage = pIcon->Icon.idEntries + iPage;
	LPBYTE pDataStart = ((LPBYTE)&(pIcon->Icon));
	LPBYTE pImageStart = (pDataStart + pImage->dwImageOffset);

	if (nBPP <= 256)
	{
		if (*((DWORD*)pImageStart) == 0x474e5089)
		{
			//PNG Mark
			//_ASSERTE(nColors == 0);
			nBPP = 32; nColors = 0;

			if (ppFormat) *ppFormat = L"ICO.PNG";
		}
		else if (*((DWORD*)pImageStart) == sizeof(BITMAPINFOHEADER))
		{
			BITMAPINFOHEADER *pDIB = (BITMAPINFOHEADER*)pImageStart;

			//if (nBPP == 0) {
			if (pDIB->biPlanes && pDIB->biBitCount)
			{
				nBPP = pDIB->biPlanes * pDIB->biBitCount;
			}

			_ASSERTE(nBPP == (pDIB->biPlanes * pDIB->biBitCount));

			if (nBPP <= 8)
				nColors = 1 << nBPP;
			else
				nColors = 0;

			if (ppFormat) *ppFormat = L"ICO.DIB";
		}
		else
		{
			_ASSERTE(*(DWORD*)pImageStart == sizeof(BITMAPINFOHEADER));
		}
	}

	_ASSERTE(nBPP);

	if (!nBPP)
		return PIE_INVALID_BPP;

	return NO_ERROR;
}

//BOOL __stdcall pvdPageInfo2(void *pContext, void *pImageContext, pvdInfoPage2 *pPageInfo)
//{
//	_ASSERTE(pPageInfo->cbSize >= sizeof(pvdInfoPage2));
//	_ASSERTE(pImageContext);
//	if (!pImageContext) {
//		pPageInfo->nErrNumber = PIE_INVALID_CONTEXT;
//		return FALSE;
//	}
//
//	LPICONCONTEXT pIcon = (LPICONCONTEXT)pImageContext;
//
//	UINT nColors = 0;
//	if ((pPageInfo->nErrNumber = LoadPageInfo(pIcon, pPageInfo->iPage, pPageInfo->nBPP,
//		pPageInfo->lWidth, pPageInfo->lHeight, nColors, &pPageInfo->pFormatName)) != NO_ERROR)
//		return FALSE;
//
//	return TRUE;
//}

LPBYTE Decode1BPP(UINT nWidth, UINT nHeight, RGBQUAD* pPAL, LPBYTE pXOR, LPBYTE pAND, DWORD& lDstStride)
{
	UINT lXorStride = (((nWidth>>3)+3)>>2)<<2; // ((nWidth + 7) >> 3);
	UINT lAndStride = lXorStride; //((nWidth + 3) >> 2) << 1;
	lDstStride = ((nWidth + 3) >> 2) << 4;
	LPBYTE pData = (LPBYTE)CALLOC(lDstStride*nHeight); // Делаем 32битное изображение
	//UINT nPoints = nWidth * nHeight;
	//UINT nStride = nWidth << 2; // Это приемник - 32бит на точку
	//LPBYTE pXorSrc, pAndSrc, pDst = pData + nStride * (nHeight - 1);
	////for (UINT i = nPoints; i--;) {
	//UINT i = nPoints - 1, n;
	//for (UINT Y = 0; Y < nHeight; Y++) {
	//	pDst = pData + nStride * Y;
	//	for (UINT X = nWidth; X--; i--) {
	//		n = pAND[i>>1];
	//		((UINT*)pDst)[X] = ((UINT*)pPAL)[ (i & 1) ? (n & 0x0F) : (n >> 4) ];
	//	}
	//}
	LPBYTE pXorSrc = pXOR; LPBYTE pAndSrc = pAND; LPBYTE pDst = pData + (nHeight - 1) * lDstStride;

	for(UINT j = nHeight; j--; pXorSrc += lXorStride, pAndSrc += lAndStride, pDst -= lDstStride)
	{
		_ASSERTE(pDst >= pData);

		for(UINT i = nWidth; i--;)
		{
			if ((pXorSrc[i/8] >> (7 - ((i) & 7)) & 1) == 0)
			{
				if ((pAndSrc[i/8] >> (7 - ((i) & 7)) & 1) != 0)
					((UINT*)pDst)[i] = 0xFFFFFFFF; //-V112
				else
					((UINT*)pDst)[i] = 0xFF000000;
			}

			//#ifdef _DEBUG
			//else {
			//	((UINT*)pDst)[i] = 0xFFFF0000;
			//}
			//#endif
		}
	}

	return pData;
}

LPBYTE Decode4BPP(UINT nWidth, UINT nHeight, RGBQUAD* pPAL, LPBYTE pXOR, LPBYTE pAND, DWORD& lDstStride)
{
	UINT lXorStride = (((nWidth>>3)+3)>>2)<<2; // ((nWidth + 7) >> 3);
	UINT lAndStride = (((nWidth>>1)+3)>>2)<<2; //((nWidth + 3) >> 2) << 1;
	lDstStride = ((nWidth + 3) >> 2) << 4;
	LPBYTE pData = (LPBYTE)CALLOC(lDstStride*nHeight); // Делаем 32битное изображение
	//UINT nPoints = nWidth * nHeight;
	//UINT nStride = nWidth << 2; // Это приемник - 32бит на точку
	//LPBYTE pXorSrc, pAndSrc, pDst = pData + nStride * (nHeight - 1);
	////for (UINT i = nPoints; i--;) {
	//UINT i = nPoints - 1, n;
	//for (UINT Y = 0; Y < nHeight; Y++) {
	//	pDst = pData + nStride * Y;
	//	for (UINT X = nWidth; X--; i--) {
	//		n = pAND[i>>1];
	//		((UINT*)pDst)[X] = ((UINT*)pPAL)[ (i & 1) ? (n & 0x0F) : (n >> 4) ];
	//	}
	//}
	LPBYTE pXorSrc = pXOR; LPBYTE pAndSrc = pAND; LPBYTE pDst = pData + (nHeight - 1) * lDstStride;
	UINT n;

	for(UINT j = nHeight; j--; pXorSrc += lXorStride, pAndSrc += lAndStride, pDst -= lDstStride)
	{
		_ASSERTE(pDst >= pData);

		for(UINT i = nWidth; i--;)
		{
			if ((pXorSrc[i/8] >> (7 - ((i) & 7)) & 1) == 0)
			{
				n = pAndSrc[i>>1];
				((UINT*)pDst)[i] = 0xFF000000 | ((UINT*)pPAL)[(i & 1) ? (n & 0x0F) : (n >> 4) ];
			}

			//#ifdef _DEBUG
			//else {
			//	((UINT*)pDst)[i] = 0xFFFF0000;
			//}
			//#endif
		}
	}

	return pData;
}

LPBYTE Decode8BPP(UINT& nWidth, UINT nHeight, RGBQUAD* pPAL, LPBYTE pXOR, LPBYTE pAND, DWORD& lDstStride)
{
	UINT lXorStride = ((nWidth+31)>>5)<<2;
	UINT lAndStride = (((nWidth<<3)+31)>>5)<<2;
	lDstStride = (((nWidth<<5)+31)>>5)<<2;
	LPBYTE pData = (LPBYTE)CALLOC(lDstStride*nHeight); // Делаем 32битное изображение
	LPBYTE pXorSrc = pXOR; LPBYTE pAndSrc = pAND; LPBYTE pDst = pData + (nHeight - 1) * lDstStride;

	for(UINT j = nHeight; j--; pXorSrc += lXorStride, pAndSrc += lAndStride, pDst -= lDstStride)
	{
		_ASSERTE(pDst >= pData);

		for(UINT i = nWidth; i--;)
		{
			if ((pXorSrc[i/8] >> (7 - ((i) & 7)) & 1) == 0)
				//if (i != j)
			{
				((UINT*)pDst)[i] = 0xFF000000 | ((UINT*)pPAL)[ pAndSrc[i] ];
			}

			//#ifdef _DEBUG
			//else {
			//	((UINT*)pDst)[i] = 0xFFFF0000;
			//}
			//#endif
		}
	}

	return pData;
}

LPBYTE Decode24BPP(UINT& nWidth, UINT nHeight, LPBYTE pXOR, LPBYTE pAND, DWORD& lDstStride)
{
	UINT lXorStride = ((nWidth+31)>>5)<<2;
	UINT lAndStride = (((nWidth*24)+31)>>5)<<2;
	lDstStride = (((nWidth<<5)+31)>>5)<<2;
	LPBYTE pData = (LPBYTE)CALLOC(lDstStride*nHeight); // Делаем 32битное изображение
	LPBYTE pXorSrc = pXOR; LPBYTE pAndSrc = pAND; LPBYTE pDst = pData + (nHeight - 1) * lDstStride;

	for(UINT j = nHeight; j--; pXorSrc += lXorStride, pAndSrc += lAndStride, pDst -= lDstStride)
	{
		_ASSERTE(pDst >= pData);
		////memmove(pDst, pAndSrc, lAndStride);
		//for (UINT i = nWidth; i--;) {
		//	if ((pXorSrc[i/8] >> (7 - ((i) & 7)) & 1) == 0)
		//	//if (i != j)
		//	{
		//		((UINT*)pDst)[i] = /*0xFF000000 |*/ ((UINT*)pAndSrc)[i];
		//	}
		//	//#ifdef _DEBUG
		//	//else {
		//	//	((UINT*)pDst)[i] = 0xFFFF0000;
		//	//}
		//	//#endif
		//}
		UINT nStep = 0;
		DWORD nLeft = 0, nCur = 0;
		UINT i = 0;
		DWORD* p = (DWORD*)pAndSrc;

		while(i < nWidth)
		{
			nCur = *p;

			switch(nStep)
			{
				case 0:

					if ((pXorSrc[i/8] >> (7 - ((i) & 7)) & 1) == 0)
						((UINT*)pDst)[i] = 0xFF000000 | (nCur & 0xFFFFFF);

					nLeft = (nCur & 0xFF000000) >> 24;
					nStep++;
					break;
				case 1:

					if ((pXorSrc[i/8] >> (7 - ((i) & 7)) & 1) == 0)
						((UINT*)pDst)[i] = 0xFF000000 | nLeft | ((nCur & 0xFFFF) << 8);

					nLeft = (nCur & 0xFFFF0000) >> 16;
					nStep++;
					break;
				case 2:

					if ((pXorSrc[i/8] >> (7 - ((i) & 7)) & 1) == 0)
						((UINT*)pDst)[i] = 0xFF000000 | nLeft | ((nCur & 0xFF) << 16);

					nLeft = (nCur & 0xFFFFFF00) >> 8;
					i++;

					if (i < nWidth)
					{
						if ((pXorSrc[i/8] >> (7 - ((i) & 7)) & 1) == 0)
							((UINT*)pDst)[i] = 0xFF000000 | nLeft;

						nLeft = 0;
					}

					nStep = 0;
					break;
			}

			i++; p++;
		}
	}

	return pData;
}

LPBYTE Decode32BPP(UINT& nWidth, UINT nHeight, LPBYTE pXOR, LPBYTE pAND, DWORD& lDstStride)
{
	UINT lXorStride = ((nWidth+31)>>5)<<2;
	UINT lAndStride = (((nWidth<<5)+31)>>5)<<2;
	lDstStride = lAndStride;
	LPBYTE pData = (LPBYTE)CALLOC(lDstStride*nHeight); // Делаем 32битное изображение
	LPBYTE pXorSrc = pXOR; LPBYTE pAndSrc = pAND; LPBYTE pDst = pData + (nHeight - 1) * lDstStride;

	for(UINT j = nHeight; j--; pXorSrc += lXorStride, pAndSrc += lAndStride, pDst -= lDstStride)
	{
		_ASSERTE(pDst >= pData);

		//memmove(pDst, pAndSrc, lAndStride);
		for(UINT i = nWidth; i--;)
		{
			if ((pXorSrc[i/8] >> (7 - ((i) & 7)) & 1) == 0)
				//if (i != j)
			{
				((UINT*)pDst)[i] = /*0xFF000000 |*/ ((UINT*)pAndSrc)[i];
			}

			//#ifdef _DEBUG
			//else {
			//	((UINT*)pDst)[i] = 0xFFFF0000;
			//}
			//#endif
		}
	}

	return pData;
}

LPBYTE DecodeDummy(UINT nWidth, UINT nHeight, DWORD& nBPP)
{
	DWORD nSize = nWidth*nHeight*4;
	LPBYTE pData = (LPBYTE)CALLOC(nSize); // Делаем 32битное изображение
	nBPP = 32;
	memset(pData, 0xFF, nSize);
	return pData;
}

//BOOL __stdcall pvdPageDecode2(void *pContext, void *pImageContext, pvdInfoDecode2 *pDecodeInfo, pvdDecodeCallback2 DecodeCallback, void *pDecodeCallbackContext)
//{
//	_ASSERTE(pDecodeInfo->cbSize >= sizeof(pvdInfoDecode2));
//	if (!pImageContext) {
//		nErrNumber = PIE_INVALID_CONTEXT;
//		return FALSE;
//	}
//
//	LPICONCONTEXT pIcon = (LPICONCONTEXT)pImageContext;
//
//	UINT32 nIconBPP = 0, nColors = 0;
//
//	if ((nErrNumber = LoadPageInfo(pIcon, pDecodeInfo->iPage, nIconBPP,
//		pDecodeInfo->lWidth, pDecodeInfo->lHeight, nColors, NULL)) != NO_ERROR)
//		return FALSE;
//
//	pDecodeInfo->Flags = 0; // PVD_IDF_READONLY не нужен, т.к память под pImage выделяется специально
//	pDecodeInfo->nBPP = 32;
//	pDecodeInfo->ColorModel = PVD_CM_BGRA;
//
//	LPICONDIRENTRY pImage = pIcon->Icon.idEntries + pDecodeInfo->iPage;
//	LPBYTE pDataStart = ((LPBYTE)&(pIcon->Icon));
//	LPBYTE pImageStart = (pDataStart + pImage->dwImageOffset);
//	if (*((DWORD*)pImageStart) == 0x474e5089) {
//		//PNG Mark
//		TODO("Хорошо бы в заголовке показать PNG");
//		BOOL lbPngLoaded = FALSE;
//		#ifndef _NO_EXEPTION_
//		try {
//		#endif
//			if (gbTokenInitialized) {
//				MStream strm;
//				strm.SetData(pImageStart, pImage->dwBytesInRes);
//				Gdiplus::BitmapData data;
//				Gdiplus::Bitmap *pBmp = Gdiplus::Bitmap::FromStream (&strm, FALSE);
//				if (pBmp) {
//					Gdiplus::PixelFormat pf = pBmp->GetPixelFormat();
//					Gdiplus::Rect rc(0,0,pDecodeInfo->lWidth,pDecodeInfo->lHeight);
//					if (!pBmp->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data)) {
//						_ASSERTE(data.PixelFormat == PixelFormat32bppARGB);
//						pDecodeInfo->lWidth = data.Width;
//						pDecodeInfo->lHeight = data.Height;
//						pDecodeInfo->nBPP = 32;
//						_ASSERTE(data.Scan0 && data.Stride);
//						pDecodeInfo->pImage = (LPBYTE)CALLOC(data.Height*((data.Stride<0) ? (-data.Stride) : data.Stride));
//						if (pDecodeInfo->pImage) {
//							if (data.Stride < 0) {
//								pDecodeInfo->lImagePitch = -data.Stride;
//								LPBYTE pSrc = (LPBYTE)data.Scan0;
//								LPBYTE pDst = pDecodeInfo->pImage;
//								for (UINT i=0; i<data.Height; i++, pDst-=data.Stride, pSrc+=data.Stride)
//									memmove(pDst, pSrc, -data.Stride);
//							} else {
//								pDecodeInfo->lImagePitch = data.Stride;
//								memmove(pDecodeInfo->pImage, data.Scan0, data.Height*data.Stride);
//							}
//							pDecodeInfo->Flags |= PVD_IDF_ALPHA;
//							lbPngLoaded = TRUE;
//						}
//						pBmp->UnlockBits(&data);
//					}
//					delete pBmp;
//				}
//			}
//		#ifndef _NO_EXEPTION_
//		} catch(...) {
//			lbPngLoaded = FALSE;
//		}
//		#endif
//
//		if (!lbPngLoaded) // пустой белый квадрат
//			pDecodeInfo->pImage = DecodeDummy ( pDecodeInfo->lWidth,pDecodeInfo->lHeight,pDecodeInfo->nBPP );
//
//	} else if (*((DWORD*)pImageStart) == sizeof(BITMAPINFOHEADER)) {
//		BITMAPINFOHEADER *pDIB = (BITMAPINFOHEADER*)pImageStart;
//		// Icons are stored in funky format where height is doubled - account for it
//		_ASSERTE(pDIB->biWidth==pDecodeInfo->lWidth && (pDecodeInfo->lHeight*2)==pDIB->biHeight);
//		_ASSERTE(pDIB->biSize == sizeof(BITMAPINFOHEADER));
//		RGBQUAD *pPAL = (RGBQUAD*)(((LPBYTE)pDIB) + pDIB->biSize);
//		if (nColors != pDIB->biClrUsed && pDIB->biClrUsed)
//			nColors = pDIB->biClrUsed;
//		LPBYTE   pAND = ((LPBYTE)pPAL) + sizeof(RGBQUAD)*nColors;
//		LPBYTE   pXOR = ((LPBYTE)pAND) + pDecodeInfo->lHeight
//			* ((((pDecodeInfo->lWidth * nIconBPP) + 31 )>>5)<<2);
//
//		_ASSERTE(pImage->dwBytesInRes >= (((pDecodeInfo->lWidth * nIconBPP) >> 3) * pDecodeInfo->lHeight));
//
//		if (nIconBPP == 1) {
//			pDecodeInfo->pImage = Decode1BPP ( pDecodeInfo->lWidth,pDecodeInfo->lHeight,pPAL,pXOR,pAND,pDecodeInfo->lImagePitch );
//			pDecodeInfo->nBPP = 32;
//			pDecodeInfo->Flags |= PVD_IDF_ALPHA;
//			TODO("Хорошо бы в заголовке показать RLE");
//
//		} else if (nIconBPP == 4) {
//			pDecodeInfo->pImage = Decode4BPP ( pDecodeInfo->lWidth,pDecodeInfo->lHeight,pPAL,pXOR,pAND,pDecodeInfo->lImagePitch );
//			pDecodeInfo->nBPP = 32;
//			pDecodeInfo->Flags |= PVD_IDF_ALPHA;
//			TODO("Хорошо бы в заголовке показать RLE");
//
//		} else if (nIconBPP == 8) {
//			pDecodeInfo->pImage = Decode8BPP ( pDecodeInfo->lWidth,pDecodeInfo->lHeight,pPAL,pXOR,pAND,pDecodeInfo->lImagePitch );
//			pDecodeInfo->nBPP = 32;
//			pDecodeInfo->Flags |= PVD_IDF_ALPHA;
//			TODO("Хорошо бы в заголовке показать RLE");
//
//		} else if (nIconBPP == 24) {
//			pDecodeInfo->pImage = Decode24BPP(pDecodeInfo->lWidth,pDecodeInfo->lHeight,pXOR,pAND,pDecodeInfo->lImagePitch);
//			pDecodeInfo->nBPP = 32;
//			pDecodeInfo->Flags |= PVD_IDF_ALPHA;
//
//		} else if (nIconBPP == 32) {
//			pDecodeInfo->pImage = Decode32BPP(pDecodeInfo->lWidth,pDecodeInfo->lHeight,pXOR,pAND,pDecodeInfo->lImagePitch);
//			pDecodeInfo->nBPP = 32;
//			pDecodeInfo->Flags |= PVD_IDF_ALPHA;
//
//		} else {
//			//nErrNumber = PIE_UNSUPPORTED_BPP;
//			//return FALSE;
//			pDecodeInfo->pImage = DecodeDummy ( pDecodeInfo->lWidth,pDecodeInfo->lHeight,pDecodeInfo->nBPP );
//		}
//	} else {
//		// Unknown image type
//		pDecodeInfo->pImage = DecodeDummy ( pDecodeInfo->lWidth,pDecodeInfo->lHeight,pDecodeInfo->nBPP );
//	}
//
//	pDecodeInfo->pPalette = NULL;
//	pDecodeInfo->nColorsUsed = 0;
//	//pDecodeInfo->lImagePitch = (pDecodeInfo->lWidth * pDecodeInfo->nBPP) >> 3;
//
//	return TRUE;
//}

//void __stdcall pvdPageFree2(void *pContext, void *pImageContext, pvdInfoDecode2 *pDecodeInfo)
//{
//	_ASSERTE(pDecodeInfo->cbSize >= sizeof(pvdInfoDecode2));
//	if (pDecodeInfo->pImage)
//		FREE(pDecodeInfo->pImage);
//}

//void __stdcall pvdFileClose2(void *pContext, void *pImageContext)
//{
//	LPICONCONTEXT pIcon = (LPICONCONTEXT)pImageContext;
//	if (pIcon) FREE(pIcon);
//}

//void __stdcall pvdExit2(void *pContext)
//{
//	#ifndef _NO_EXEPTION_
//	try{
//	#endif
//		if (gbTokenInitialized) {
//			Gdiplus::GdiplusShutdown(gdiplusToken);
//			gbTokenInitialized = false;
//		}
//	#ifndef _NO_EXEPTION_
//	} catch(...) {  }
//	#endif
//
//	if (gbCoInitialized) {
//		gbCoInitialized = FALSE;
//		CoUninitialize();
//	}
//}


enum tag_IcoStrMagics
{
	eIcoStr_Decoder = 0x1010,
	eIcoStr_Image = 0x1011,
};

struct ICODecoder
{
	DWORD   nMagic;
	DWORD   nErrNumber;
	BOOL    bCancelled;
	bool    gbCoInitialized, gbTokenInitialized;
	ULONG_PTR gdiplusToken;

	BOOL Init(struct CET_Init* pInit)
	{
		pInit->pContext = this;
		HRESULT hrCoInitialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
		gbCoInitialized = SUCCEEDED(hrCoInitialized);
		Gdiplus::GdiplusStartupInput gdiplusStartupInput;
		Gdiplus::Status lRc = GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

		if (lRc != Gdiplus::Ok)
		{
			Gdiplus::GdiplusShutdown(gdiplusToken); gbTokenInitialized = false;
			return FALSE;
		}
		else
		{
			gbTokenInitialized = true;
		}

		return TRUE;
	};

	void Close()
	{
		if (gbTokenInitialized)
		{
			Gdiplus::GdiplusShutdown(gdiplusToken);
			gbTokenInitialized = false;
		}

		if (gbCoInitialized)
		{
			gbCoInitialized = FALSE;
			CoUninitialize();
		}

		FREE(this);
	};
};

struct ICOImage
{
	DWORD   nMagic;
	ICODecoder *decoder;
	DWORD   nErrNumber;
	LPICONCONTEXT pIcon;
	LPBYTE pImageData;
	int iPage;
	UINT32 nIconBPP, lWidth, lHeight, nColors;
	Gdiplus::BitmapData data;
	Gdiplus::Bitmap *pBmp;
	BOOL bPNG;
	wchar_t szComments[128];

	BOOL Open(bool bVirtual, const wchar_t *pFileName, const BYTE *pBuf, DWORD lBuf, COORD crLoadSize)
	{
		_ASSERTE(pIcon == NULL);
		_ASSERTE(decoder != NULL);
		_ASSERTE(pBuf && lBuf);
		// При открытии ресурсов - pBuf это BITMAPINFOHEADER
		// Соответственно, при просмотре ресурса - один файл это строго одна иконка,
		// информацию нужно грузить строго из BITMAPINFOHEADER
		// Тут может быть и PNG, но это нас уже не интересует. PNG может быть открыт любым декодером...
		LPICONDIR pTest = (LPICONDIR)pBuf;
		LPBITMAPINFOHEADER pBmp = (LPBITMAPINFOHEADER)pBuf;

		if (!pBuf || !lBuf)
		{
			nErrNumber = PIE_BUFFER_NOT_FULL;
			return FALSE;
		}

		if (lBuf > sizeof(ICONDIR) && pTest->idReserved==0 && (pTest->idType==1 /* ICON */ || pTest->idType == 2 /* CURSOR */))
		{
			if (lBuf > PVD_MAX_ICON_SIZE)
			{
				nErrNumber = PIE_TOO_LARGE_FILE;
				return FALSE;
			}

			if (pTest->idCount == 0)
			{
				nErrNumber = PIE_NO_IMAGES;
				return FALSE;
			}

			// Делаем копию буфера
			pIcon = (LPICONCONTEXT)CALLOC((DWORD)lBuf+sizeof(DWORD));

			if (!pIcon)
			{
				nErrNumber = PIE_NOT_ENOUGH_MEMORY;
				return FALSE;
			}

			pIcon->nDataSize = (DWORD)lBuf;
			memmove(&(pIcon->Icon), pBuf, (DWORD)lBuf);
		}
		else
		{
			// Test BITMAPINFOHEADER
			if (lBuf > sizeof(BITMAPINFOHEADER) && pBmp->biSize == sizeof(BITMAPINFOHEADER)
			        && (pBmp->biWidth && pBmp->biWidth < 256)
			        && (pBmp->biHeight == (pBmp->biWidth * 2)))
			{
				// Делаем копию буфера, но предваряем его заголовком иконки
				DWORD nSize = lBuf + sizeof(ICONDIR);
				pIcon = (LPICONCONTEXT)CALLOC(nSize+sizeof(DWORD));

				if (!pIcon)
				{
					nErrNumber = PIE_NOT_ENOUGH_MEMORY;
					return FALSE;
				}

				pIcon->nDataSize = nSize;
				pIcon->Icon.idReserved = 0;
				pIcon->Icon.idType = 1;
				pIcon->Icon.idCount = 1;
				pIcon->Icon.idEntries[0].bWidth = (BYTE)pBmp->biWidth;
				pIcon->Icon.idEntries[0].bHeight = (BYTE)pBmp->biWidth; //-V537 //помним, что (pBmp->biHeight == (pBmp->biWidth * 2))
				pIcon->Icon.idEntries[0].bColorCount = (pBmp->biBitCount >= 8) ? 0 : (1 << pBmp->biBitCount);
				pIcon->Icon.idEntries[0].bReserved = 0;
				pIcon->Icon.idEntries[0].wPlanes = pBmp->biPlanes;
				pIcon->Icon.idEntries[0].wBitCount = pBmp->biBitCount;
				pIcon->Icon.idEntries[0].dwBytesInRes = (DWORD)lBuf;
				pIcon->Icon.idEntries[0].dwImageOffset = sizeof(ICONDIR);
				memmove(&(pIcon->Icon.idEntries[1]), pBuf, lBuf);
			}
			else
			{
				nErrNumber = PIE_NOT_ICO_FILE;
				return FALSE;
			}
		}

		WARNING("Horosho by vozvraschatj umolchatel'nyj indeks otobrazhaemoj ikonki, esli pervaja NE soderzhit izobrazhenija voobsche");
		// file_view_hc.ico - первый фрейм вообще пустой (полностью прозрачный), а второй содержит картинку

		if (pIcon->nDataSize < (sizeof(ICONDIR) + (pIcon->Icon.idCount-1)*sizeof(ICONDIRENTRY)))
		{
			nErrNumber = PIE_JUMP_OUT_OF_FILE;
			FREE(pIcon);
			return FALSE;
		}

		//pImageInfo->pImageContext = pIcon;
		//pImageInfo->nPages = pIcon->Icon.idCount;
		//pImageInfo->Flags = 0;
		//pImageInfo->pFormatName = pIcon->Icon.idType!=2 ? L"ICO" : L"CUR";
		//pImageInfo->pCompression = NULL;
		//pImageInfo->pComments = NULL;
		//pImageInfo->pImageContext = pIcon;
		return TRUE;
	};

	BOOL PageDecode(struct CET_LoadInfo* pLoadPreview)
	{
		//pDecodeInfo->Flags = 0; // PVD_IDF_READONLY не нужен, т.к память под pImage выделяется специально
		pLoadPreview->nBits = 32;
		//pDecodeInfo->ColorModel = PVD_CM_BGRA;
		pLoadPreview->cbStride = (lWidth * pLoadPreview->nBits) >> 3;
		LPICONDIRENTRY pImage = pIcon->Icon.idEntries + iPage;
		LPBYTE pDataStart = ((LPBYTE)&(pIcon->Icon));
		LPBYTE pImageStart = (pDataStart + pImage->dwImageOffset);

		if (*((DWORD*)pImageStart) == 0x474e5089)
		{
			//PNG Mark
			bPNG = TRUE;
			BOOL lbPngLoaded = FALSE;

			if (decoder->gbTokenInitialized)
			{
				MStream strm;
				strm.SetData(pImageStart, pImage->dwBytesInRes);
				//Gdiplus::BitmapData data;
				memset(&data, 0, sizeof(data));
				pBmp = Gdiplus::Bitmap::FromStream(&strm, FALSE);

				if (pBmp)
				{
					Gdiplus::PixelFormat pf = pBmp->GetPixelFormat();
					Gdiplus::Rect rc(0,0,lWidth,lHeight);

					if (!pBmp->LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &data))
					{
						_ASSERTE(data.PixelFormat == PixelFormat32bppARGB);
						lWidth = data.Width;
						lHeight = data.Height;
						//nBPP = 32;
						_ASSERTE(data.Scan0 && data.Stride);
						pImageData = (LPBYTE)CALLOC(data.Height*((data.Stride<0) ? (-data.Stride) : data.Stride));

						if (pImageData)
						{
							if (data.Stride < 0)
							{
								pLoadPreview->cbStride = -data.Stride;
								LPBYTE pSrc = (LPBYTE)data.Scan0;
								LPBYTE pDst = pImageData;

								for(UINT i=0; i<data.Height; i++, pDst-=data.Stride, pSrc+=data.Stride)
									memmove(pDst, pSrc, -data.Stride);
							}
							else
							{
								pLoadPreview->cbStride = data.Stride;
								memmove(pImageData, data.Scan0, data.Height*data.Stride);
							}

							//pDecodeInfo->Flags |= PVD_IDF_ALPHA;
							lbPngLoaded = TRUE;
						}

						pBmp->UnlockBits(&data);
					}

					delete pBmp;
					pBmp = NULL;
				}
			}

			if (!lbPngLoaded)  // пустой белый квадрат
				pImageData = DecodeDummy(lWidth,lHeight,pLoadPreview->nBits);
		}
		else if (*((DWORD*)pImageStart) == sizeof(BITMAPINFOHEADER))
		{
			BITMAPINFOHEADER *pDIB = (BITMAPINFOHEADER*)pImageStart;
			// Icons are stored in funky format where height is doubled - account for it
			_ASSERTE(pDIB->biWidth==lWidth && (lHeight*2)==pDIB->biHeight);
			_ASSERTE(pDIB->biSize == sizeof(BITMAPINFOHEADER));
			RGBQUAD *pPAL = (RGBQUAD*)(((LPBYTE)pDIB) + pDIB->biSize);

			if (nColors != pDIB->biClrUsed && pDIB->biClrUsed)
				nColors = pDIB->biClrUsed;

			LPBYTE   pAND = ((LPBYTE)pPAL) + sizeof(RGBQUAD)*nColors;
			LPBYTE   pXOR = ((LPBYTE)pAND) + lHeight
			                * ((((lWidth * nIconBPP) + 31)>>5)<<2);
			_ASSERTE(pImage->dwBytesInRes >= (((lWidth * nIconBPP) >> 3) * lHeight));

			if (nIconBPP == 1)
			{
				pImageData = Decode1BPP(lWidth,lHeight,pPAL,pXOR,pAND,pLoadPreview->cbStride);
				pLoadPreview->nBits = 32;
				//pDecodeInfo->Flags |= PVD_IDF_ALPHA;
				//TODO("Хорошо бы в заголовке показать RLE");
			}
			else if (nIconBPP == 4) //-V112
			{
				pImageData = Decode4BPP(lWidth,lHeight,pPAL,pXOR,pAND,pLoadPreview->cbStride);
				pLoadPreview->nBits = 32;
				//pDecodeInfo->Flags |= PVD_IDF_ALPHA;
				//TODO("Хорошо бы в заголовке показать RLE");
			}
			else if (nIconBPP == 8)
			{
				pImageData = Decode8BPP(lWidth,lHeight,pPAL,pXOR,pAND,pLoadPreview->cbStride);
				pLoadPreview->nBits = 32;
				//pDecodeInfo->Flags |= PVD_IDF_ALPHA;
				//TODO("Хорошо бы в заголовке показать RLE");
			}
			else if (nIconBPP == 24)
			{
				pImageData = Decode24BPP(lWidth,lHeight,pXOR,pAND,pLoadPreview->cbStride);
				pLoadPreview->nBits = 32;
				//pDecodeInfo->Flags |= PVD_IDF_ALPHA;
			}
			else if (nIconBPP == 32)
			{
				pImageData = Decode32BPP(lWidth,lHeight,pXOR,pAND,pLoadPreview->cbStride);
				pLoadPreview->nBits = 32;
				//pDecodeInfo->Flags |= PVD_IDF_ALPHA;
			}
			else
			{
				//nErrNumber = PIE_UNSUPPORTED_BPP;
				//return FALSE;
				pImageData = DecodeDummy(lWidth,lHeight,pLoadPreview->nBits);
			}
		}
		else
		{
			// Unknown image type
			pImageData = DecodeDummy(lWidth,lHeight,pLoadPreview->nBits);
		}

		//pDecodeInfo->pPalette = NULL;
		//pDecodeInfo->nColorsUsed = 0;
		//pLoadPreview->cbStride = (pDecodeInfo->lWidth * pLoadPreview->nBits) >> 3;
		pLoadPreview->pFileContext = this;
		pLoadPreview->ColorModel = CET_CM_BGRA;
		pLoadPreview->Pixels = (DWORD*)pImageData;
		pLoadPreview->crSize.X = lWidth;
		pLoadPreview->crSize.Y = lHeight;
		pLoadPreview->cbPixelsSize = pLoadPreview->cbStride * lHeight;
		swprintf_c(szComments, L"%i x %i x %ibpp [%i] %s%s", lWidth, lHeight, nIconBPP, pIcon->Icon.idCount,
		          pIcon->Icon.idType!=2 ? L"ICO" : L"CUR", bPNG ? L" [PNG]" : L"");
		pLoadPreview->pszComments = szComments;
		return TRUE;
	}

	BOOL GetPageBits(struct CET_LoadInfo* pLoadPreview)
	{
		UINT32 nBPP, nW, nH, nC;
		iPage = -1;

		// Подобрать фрейм наиболее подходящий к запросу
		for(int s = 1; s <= 3; s++)
		{
			for(int i = 0; i < pIcon->Icon.idCount; i++)
			{
				if ((nErrNumber = LoadPageInfo(pIcon, i, nBPP, nW, nH, nC, NULL)) == NO_ERROR)
				{
					// Сначала пытаемся по максимуму
					if (s == 1)
					{
						if (pIcon->Icon.idCount > 1 && (pLoadPreview->crLoadSize.X != nW || pLoadPreview->crLoadSize.Y != nH))
							continue;

						if (nBPP > nIconBPP)
						{
							iPage = i; nIconBPP = nBPP; lWidth = nW; lHeight = nH; nColors = nC;
						}
					}
					else if (s == 2)
					{
						// если не удалось - то не более
						if ((int)pLoadPreview->crLoadSize.X < (int)nW || (int)pLoadPreview->crLoadSize.Y < (int)nH)
							continue;

						if (nBPP >= nIconBPP && nW >= lWidth && nH >= lHeight)
						{
							iPage = i; nIconBPP = nBPP; lWidth = nW; lHeight = nH; nColors = nC;
						}
					}
					else
					{
						// не удалось - любой размер
						if (nBPP >= nIconBPP && nW >= lWidth && nH >= lHeight)
						{
							iPage = i; nIconBPP = nBPP; lWidth = nW; lHeight = nH; nColors = nC;
						}
					}
				}
			}

			if (iPage != -1)
			{
				return PageDecode(pLoadPreview);
			}
		}

		return FALSE;
	};

	void Close()
	{
		if (pIcon)
			FREE(pIcon);

		if (pImageData)
			FREE(pImageData);

		if (pBmp)
		{
			delete pBmp;
			pBmp = NULL;
		}

		FREE(this);
	};
};



extern "C" BOOL WINAPI CET_Init(struct CET_Init* pInit)
{
	HeapInitialize();
	_ASSERTE(pInit->cbSize >= sizeof(struct CET_Init));

	if (pInit->cbSize < sizeof(struct CET_Init))
	{
		pInit->nErrNumber = PIE_OLD_PLUGIN;
		return FALSE;
	}

	ghModule = pInit->hModule;
	ICODecoder *pDecoder = (ICODecoder*) CALLOC(sizeof(ICODecoder));

	if (!pDecoder)
	{
		pInit->nErrNumber = PIE_NOT_ENOUGH_MEMORY;
		return FALSE;
	}

	pDecoder->nMagic = eIcoStr_Decoder;

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
		ICODecoder *pDecoder = (ICODecoder*)pInit->pContext;

		if (pDecoder)
		{
			_ASSERTE(pDecoder->nMagic == eIcoStr_Decoder);

			if (pDecoder->nMagic == eIcoStr_Decoder)
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
	if (!pLoadPreview || (*(LPDWORD)pLoadPreview != sizeof(struct CET_LoadInfo)))
	{
		_ASSERTE(*(LPDWORD)pLoadPreview == sizeof(struct CET_LoadInfo));
		SETERROR(PIE_INVALID_VERSION);
		return FALSE;
	}

	if (!pLoadPreview->pContext)
	{
		SETERROR(PIE_INVALID_CONTEXT);
		return FALSE;
	}

	if (pLoadPreview->bVirtualItem && (!pLoadPreview->pFileData || !pLoadPreview->nFileSize))
	{
		SETERROR(PIE_FILE_NOT_FOUND);
		return FALSE;
	}

	if (pLoadPreview->nFileSize < 16 || pLoadPreview->nFileSize > 10485760/*10 MB*/)
	{
		SETERROR(PIE_UNSUPPORTEDFORMAT);
		return FALSE;
	}

	BOOL lbKnown = FALSE;

	if (pLoadPreview->pFileData)
	{
		const BYTE  *pb  = (const BYTE*)pLoadPreview->pFileData;
		const WORD  *pw  = (const WORD*)pLoadPreview->pFileData;
		const DWORD *pdw = (const DWORD*)pLoadPreview->pFileData;

		if (pw[0]==0 && (pw[1]==1/*ICON*/ || pw[1]==2/*CURSOR*/) && (pw[2]>0 && pw[2]<=64/*IMG COUNT*/))  // .ico, .cur
			lbKnown = TRUE;
	}

	if (!lbKnown)
	{
		SETERROR(PIE_UNSUPPORTEDFORMAT);
		return FALSE;
	}

	ICOImage *pImage = (ICOImage*)CALLOC(sizeof(ICOImage));

	if (!pImage)
	{
		SETERROR(PIE_NOT_ENOUGH_MEMORY);
		return FALSE;
	}

	pImage->nMagic = eIcoStr_Image;
	pLoadPreview->pFileContext = (void*)pImage;
	pImage->decoder = (ICODecoder*)pLoadPreview->pContext;
	pImage->decoder->bCancelled = FALSE;

	if (!pImage->Open(
	            (pLoadPreview->bVirtualItem!=FALSE), pLoadPreview->sFileName,
	            pLoadPreview->pFileData, (DWORD)pLoadPreview->nFileSize, pLoadPreview->crLoadSize))
	{
		SETERROR(pImage->nErrNumber);
		pImage->Close();
		pLoadPreview->pFileContext = NULL;
		return FALSE;
	}

	if (!pImage->GetPageBits(pLoadPreview))
	{
		SETERROR(pImage->nErrNumber);
		pImage->Close();
		pLoadPreview->pFileContext = NULL;
		return FALSE;
	}

	//if (pLoadPreview->pFileContext == (void*)pImage)
	//	pLoadPreview->pFileContext = NULL;
	//
	//pImage->Close();
	return TRUE;
}

extern "C" VOID WINAPI CET_Free(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo))
	{
		_ASSERTE(*(LPDWORD)pLoadPreview == sizeof(struct CET_LoadInfo));
		SETERROR(PIE_INVALID_VERSION);
		return;
	}

	if (!pLoadPreview->pFileContext)
	{
		SETERROR(PIE_INVALID_CONTEXT);
		return;
	}

	switch(*(LPDWORD)pLoadPreview->pFileContext)
	{
		case eIcoStr_Image:
		{
			// Сюда мы попадем если был exception в CET_Load
			ICOImage *pImg = (ICOImage*)pLoadPreview->pFileContext;
			pImg->Close();
		} break;
#ifdef _DEBUG
		default:
			_ASSERTE(*(LPDWORD)pLoadPreview->pFileContext == eIcoStr_Image);
#endif
	}
}

extern "C" VOID WINAPI CET_Cancel(LPVOID pContext)
{
	if (!pContext) return;

	ICODecoder *pDecoder = (ICODecoder*)pContext;

	if (pDecoder->nMagic == eIcoStr_Decoder)
	{
		pDecoder->bCancelled = TRUE;
	}
}
