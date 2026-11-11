
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
#include <wintrust.h>

#undef USE_TRACE
//#define USE_TRACE

#ifdef _DEBUG
	#if !defined(__GNUC__)
		#include <crtdbg.h>
	#endif
#endif

#ifndef _ASSERTE
	#define _ASSERT(x)
	#define _ASSERTE(x)
#endif

#ifndef verc0_EXPORTS
	#include "../ThumbSDK.h"
	#include "../../../common/MStrSafe.h"
	#include "../../../common/defines.h"
	#include "../../../common/Memory.h"
#else
	#define _wsprintf wsprintfW
	#define SKIPLEN(x)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))
#endif

#ifdef _DEBUG
	#define Msg(f,s) //MessageBoxA(NULL,s,f,MB_OK|MB_SETFOREGROUND)
#else
	#define Msg(f,s)
#endif

#undef _T
#define _T(x)      L ## x
#undef _tcscpy
#define _tcscpy lstrcpyW

HMODULE ghModule;


#define PEE_FILE_NOT_FOUND       0x80001003
#define PEE_NOT_ENOUGH_MEMORY    0x80001004
#define PEE_INVALID_CONTEXT      0x80001005
#define PEE_INVALID_VERSION      0x8000100D
#define PEE_UNSUPPORTEDFORMAT    0x8000100F
#define PEE_OLD_PLUGIN           0x80001010




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

enum tag_PeStrMagics
{
	ePeStr_Info = 0x1005,
};

enum tag_PeStrFlags
{
	PE_Far1         = 0x0001,
	PE_Far2         = 0x0002,
	PE_Far3         = 0x0004,
	PE_DOTNET       = 0x0100,
	PE_UPX          = 0x0200,
	PE_VER_EXISTS   = 0x0400,
	PE_ICON_EXISTS  = 0x0800,
	PE_SIGNED       = 0x1000,
};


struct PEData
{
	DWORD nMagic;
	BOOL  bValidateFailed;
	int   nBits;  // x16/x32/x64
	UINT  nFlags; // tag_PeStrFlags
	wchar_t szExtension[32];
	wchar_t *szVersion, szVersionN[32], szVersionF[128], szVersionP[128];
	wchar_t szProduct[128];
	wchar_t szCompany[128]; // или company, или copyright?
	wchar_t szInfo[512]; // полная информация
	//
	PBYTE pMappedFileBase;
	ULARGE_INTEGER FileSize;
	ULARGE_INTEGER FileFullSize;
	PIMAGE_NT_HEADERS32 pNTHeader32;
	PIMAGE_NT_HEADERS64 pNTHeader64;
	bool bIs64Bit;
	WORD Machine;


	PEData()
	{
		nMagic = ePeStr_Info; nBits = 0; nFlags = 0; bValidateFailed = FALSE; Machine = 0;
		szInfo[0] = szVersionN[0] = szProduct[0] = szCompany[0] = szExtension[0] = szVersionP[0] = szVersionF[0] = 0;
		szVersion = szVersionN;
		pMappedFileBase = NULL; FileSize.QuadPart = 0; pNTHeader32 = NULL; pNTHeader64 = NULL; bIs64Bit = false;
		FileFullSize.QuadPart = 0;
	};

	void Close()
	{
		FREE(this);
	};
};

//PEData *gpCurData = NULL;

//PBYTE pData->pMappedFileBase = 0;
//ULARGE_INTEGER pData->FileSize = {{0,0}};
//PIMAGE_NT_HEADERS32 pData->pNTHeader32 = NULL;
//PIMAGE_NT_HEADERS64 pData->pNTHeader64 = NULL;
//bool pData->bIs64Bit = false;
//PDWORD g_pCVHeader = 0;
//PIMAGE_COFF_SYMBOLS_HEADER g_pCOFFHeader = 0;
//COFFSymbolTable * g_pCOFFSymbolTable = 0;

bool DumpExeFilePE(PEData *pData, PIMAGE_DOS_HEADER dosHeader, PIMAGE_NT_HEADERS32 pNTHeader);
//bool DumpExeFileVX( PEData *pData, PIMAGE_DOS_HEADER dosHeader, PIMAGE_VXD_HEADER pVXDHeader );
void DumpResourceDirectory(PEData *pData, PIMAGE_RESOURCE_DIRECTORY pResDir,
                           PBYTE pResourceBase,
                           DWORD level,
                           DWORD resourceType, DWORD rootType = 0, DWORD parentType = 0);


// MakePtr is a macro that allows you to easily add to values (including
// pointers) together without dealing with C's pointer arithmetic.  It
// essentially treats the last two parameters as DWORDs.  The first
// parameter is used to typecast the result to the appropriate pointer type.
#define MakePtr( cast, ptr, addValue ) (cast)( (DWORD_PTR)(ptr) + (DWORD_PTR)(addValue))

#define GetImgDirEntryRVA( pNTHdr, IDE ) \
	(pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress)

#define GetImgDirEntrySize( pNTHdr, IDE ) \
	(pNTHdr->OptionalHeader.DataDirectory[IDE].Size)

//#define ARRAY_SIZE( x ) (sizeof(x) / sizeof(x[0]))

#define IMAGE_SIZEOF_ROM_OPTIONAL_HEADER 56
#define IMAGE_SIZEOF_NT_OPTIONAL32_HEADER    224
#define IMAGE_SIZEOF_NT_OPTIONAL64_HEADER    240


#ifdef USE_TRACE
static char szTrace[1024];
#define _TRACE(sz)               { OutputDebugStringA(sz); OutputDebugStringA("\n"); }
#define _TRACE0(sz)              _TRACE(sz)
#define _TRACE1(sz, p1)          { wsprintfA(szTrace, sz, p1); OutputDebugStringA(szTrace); OutputDebugStringA("\n"); }
#define _TRACE2(sz, p1, p2)      { wsprintfA(szTrace, sz, p1, p2); OutputDebugStringA(szTrace); OutputDebugStringA("\n"); }
#define _TRACE3(sz, p1, p2, p3)  { wsprintfA(szTrace, sz, p1, p2, p3); OutputDebugStringA(szTrace); OutputDebugStringA("\n"); }
#define _TRACE4(sz,p1,p2,p3,p4)  { wsprintfA(szTrace, sz, p1, p2, p3, p4); OutputDebugStringA(szTrace); OutputDebugStringA("\n"); }
#define _TRACE5(sz,p1,p2,p3,p4,p5) { wsprintfA(szTrace, sz, p1, p2, p3, p4, p5); OutputDebugStringA(szTrace); OutputDebugStringA("\n"); }
#define _TRACE_ASSERT(f,sz)      if (!(f)) {_TRACE(sz);}
#else
#define _TRACE(sz)
#define _TRACE0(sz)
#define _TRACE1(sz, p1)
#define _TRACE2(sz, p1, p2)
#define _TRACE3(sz, p1, p2, p3)
#define _TRACE4(sz,p1,p2,p3,p4)
#define _TRACE5(sz,p1,p2,p3,p4,p5)
#define _TRACE_ASSERT(f,sz)
#endif


//================================================================================
//
// Given an RVA, look up the section header that encloses it and return a
// pointer to its IMAGE_SECTION_HEADER
//
template <class T> PIMAGE_SECTION_HEADER GetEnclosingSectionHeader(DWORD rva, T* pNTHeader)	// 'T' == PIMAGE_NT_HEADERS
{
	_TRACE1("GetEnclosingSectionHeader(rva=0x%08X)", rva);
	PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader); //-V220
	unsigned i;

	for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
	{
		// This 3 line idiocy is because Watcom's linker actually sets the
		// Misc.VirtualSize field to 0.  (!!! - Retards....!!!)
		DWORD size = section->Misc.VirtualSize;

		if (0 == size)
			size = section->SizeOfRawData;

		// Is the RVA within this section?
		if ((rva >= section->VirtualAddress) &&
		        (rva < (section->VirtualAddress + size)))
		{
			_TRACE2("GetEnclosingSectionHeader(rva=0x%08X)=0x%08X", rva, (DWORD)section);
			return section;
		}
	}

	_TRACE1("GetEnclosingSectionHeader(rva=0x%08X)=0", rva);
	return 0;
}

template <class T> LPVOID GetPtrFromRVA(DWORD rva, T* pNTHeader, PBYTE imageBase)   // 'T' = PIMAGE_NT_HEADERS
{
	_TRACE1("GetPtrFromRVA(rva=0x%08X)", rva);
	_ASSERTE(pNTHeader!=NULL);

	if (!pNTHeader || !imageBase)
	{
		return NULL;
	}

	PIMAGE_SECTION_HEADER pSectionHdr;
	INT delta;
	pSectionHdr = GetEnclosingSectionHeader(rva, pNTHeader);

	if (!pSectionHdr)
		return 0;

	delta = (INT)(pSectionHdr->VirtualAddress-pSectionHdr->PointerToRawData);
	_TRACE2("GetPtrFromRVA(rva=0x%08X) -> (delta=%i)", (DWORD)(rva - delta), delta);
	return (PVOID)(imageBase + rva - delta);
}


////
//// Like GetPtrFromRVA, but works with addresses that already include the
//// default image base
////
//template <class T> LPVOID GetPtrFromVA( PVOID ptr, T* pNTHeader, PBYTE pImageBase ) // 'T' = PIMAGE_NT_HEADERS
//{
//	// Yes, under Win64, we really are lopping off the high 32 bits of a 64 bit
//	// value.  We'll knowingly believe that the two pointers are within the
//	// same load module, and as such, are RVAs
//	DWORD rva = PtrToLong( (PBYTE)ptr - pNTHeader->OptionalHeader.ImageBase );
//
//	return GetPtrFromRVA( rva, pNTHeader, pImageBase );
//}

////
//// Given a section name, look it up in the section table and return a
//// pointer to its IMAGE_SECTION_HEADER
////
//template <class T> PIMAGE_SECTION_HEADER GetSectionHeader(PSTR name, T* pNTHeader)	// 'T' == PIMAGE_NT_HEADERS
//{
//    PIMAGE_SECTION_HEADER section = IMAGE_FIRST_SECTION(pNTHeader);
//    unsigned i;
//
//    for ( i=0; i < pNTHeader->FileHeader.NumberOfSections; i++, section++ )
//    {
//        if ( 0 == strncmp((char *)section->Name,name,IMAGE_SIZEOF_SHORT_NAME) )
//            return section;
//    }
//
//    return 0;
//}

bool ValidateMemory(PEData *pData, LPCVOID ptr, size_t nSize)
{
	if (pData->bValidateFailed)
		return false;

	bool lbRc = false;
	if (!ptr || (LPBYTE)ptr < (LPBYTE)pData->pMappedFileBase)
	{
		//return false;
	}
	else
	{
		ULONGLONG nPos = ((LPBYTE)ptr - (LPBYTE)pData->pMappedFileBase);
		if ((nPos+nSize) > pData->FileSize.QuadPart)
		{
			//return false;
		}
		else
		{
			lbRc = true;
		}
	}

	if (!lbRc)
	{
		_TRACE2("ValidateMemory(R%08x, %i) FAILED", ((LPBYTE)ptr - pData->pMappedFileBase), nSize);
		pData->bValidateFailed = TRUE;
	}
	else
	{
		_TRACE2("ValidateMemory(R%08x, %i) OK", ((LPBYTE)ptr - pData->pMappedFileBase), nSize);
	}
	return lbRc;
}


//static const TCHAR cszLanguage[] = _T("Lang");
//static const TCHAR cszString[] = _T("String value");
//static const TCHAR cszType[] = _T("Type");
//static const TCHAR cszInfo[] = _T("Information");

typedef const BYTE *LPCBYTE;

//// The predefined resource types
//char *SzResourceTypes[] = {
//"???_0",
//"CURSOR",
//"BITMAP",
//"ICON",
//"MENU",
//"DIALOG",
//"STRING",
//"FONTDIR",
//"FONT",
//"ACCELERATORS",
//"RCDATA",
//"MESSAGETABLE",
//"GROUP_CURSOR",
//"???_13",
//"GROUP_ICON",
//"???_15",
//"VERSION",
//"DLGINCLUDE",
//"???_18",
//"PLUGPLAY",
//"VXD",
//"ANICURSOR",
//"ANIICON",
//"HTML",
//"MANIFEST"
//};

//typedef struct
//{
//	DWORD          flag;
//	const wchar_t* name;
//} DWORD_FLAG_DESCRIPTIONSW;
//
//DWORD_FLAG_DESCRIPTIONSW VersionInfoFlags[] = {
//	{VS_FF_DEBUG, L"VS_FF_DEBUG"},
//	{VS_FF_INFOINFERRED, L"VS_FF_INFOINFERRED"},
//	{VS_FF_PATCHED, L"VS_FF_PATCHED"},
//	{VS_FF_PRERELEASE, L"VS_FF_PRERELEASE"},
//	{VS_FF_PRIVATEBUILD, L"VS_FF_PRIVATEBUILD"},
//	{VS_FF_SPECIALBUILD, L"VS_FF_SPECIALBUILD"},
//	{0}
//};

//DWORD_FLAG_DESCRIPTIONSW VersionInfoFileOS[] = {
//	{VOS_DOS, L"VOS_DOS"},
//	{VOS_NT, L"VOS_NT"},
//	{VOS__WINDOWS16, L"VOS__WINDOWS16"},
//	{VOS__WINDOWS32, L"VOS__WINDOWS32"},
//	{VOS_OS216, L"VOS_OS216"},
//	{VOS_OS232, L"VOS_OS232"},
//	{VOS__PM16, L"VOS__PM16"},
//	{VOS__PM32, L"VOS__PM32"},
//	{VOS_WINCE, L"VOS_WINCE"},
//	{0}
//};

//DWORD_FLAG_DESCRIPTIONSW VersionInfoDrvSubtype[] = {
//	{VFT2_DRV_PRINTER, L"VFT2_DRV_PRINTER"},
//	{VFT2_DRV_KEYBOARD, L"VFT2_DRV_KEYBOARD"},
//	{VFT2_DRV_LANGUAGE, L"VFT2_DRV_LANGUAGE"},
//	{VFT2_DRV_DISPLAY, L"VFT2_DRV_DISPLAY"},
//	{VFT2_DRV_NETWORK, L"VFT2_DRV_NETWORK"},
//	{VFT2_DRV_SYSTEM, L"VFT2_DRV_SYSTEM"},
//	{VFT2_DRV_INSTALLABLE, L"VFT2_DRV_INSTALLABLE"},
//	{VFT2_DRV_SOUND, L"VFT2_DRV_SOUND"},
//	{VFT2_DRV_COMM, L"VFT2_DRV_COMM"},
//	{VFT2_DRV_INPUTMETHOD, L"VFT2_DRV_INPUTMETHOD"},
//	{VFT2_DRV_VERSIONED_PRINTER, L"VFT2_DRV_VERSIONED_PRINTER"},
//	{0}
//};

//DWORD_FLAG_DESCRIPTIONSW VersionInfoFontSubtype[] = {
//	{VFT2_FONT_RASTER, L"VFT2_FONT_RASTER"},
//	{VFT2_FONT_VECTOR, L"VFT2_FONT_VECTOR"},
//	{VFT2_FONT_TRUETYPE, L"VFT2_FONT_TRUETYPE"},
//	{0}
//};

struct String
{
	WORD   wLength;
	WORD   wValueLength;
	WORD   wType;
	WCHAR  szKey[1];
	WORD   Padding[ANYSIZE_ARRAY];
	WORD   Value[1];
};
struct StringA
{
	WORD   wLength;
	WORD   wValueLength;
	char   szKey[1];
	char   Padding[ANYSIZE_ARRAY];
	char   Value[1];
};

struct StringTable
{
	WORD   wLength;
	WORD   wValueLength;
	WORD   wType;
	WCHAR  szKey[9];
	WORD   Padding[ANYSIZE_ARRAY];
	String Children[ANYSIZE_ARRAY];
};
struct StringTableA
{
	WORD    wLength;
	WORD    wType;
	char    szKey[9];
	char    Padding[ANYSIZE_ARRAY];
	StringA Children[ANYSIZE_ARRAY];
};

struct StringFileInfo
{
	WORD        wLength;
	WORD        wValueLength;
	WORD        wType;
	WCHAR       szKey[ANYSIZE_ARRAY];
	WORD        Padding[ANYSIZE_ARRAY];
	StringTable Children[ANYSIZE_ARRAY];
};
struct StringFileInfoA
{
	WORD         wLength;
	WORD         wType;
	char         szKey[ANYSIZE_ARRAY];
	char         Padding[ANYSIZE_ARRAY];
	StringTableA Children[ANYSIZE_ARRAY];
};

struct Var
{
	WORD  wLength;
	WORD  wValueLength;
	WORD  wType;
	WCHAR szKey[ANYSIZE_ARRAY];
	WORD  Padding[ANYSIZE_ARRAY];
	DWORD Value[ANYSIZE_ARRAY];
};

struct VarFileInfo
{
	WORD  wLength;
	WORD  wValueLength;
	WORD  wType;
	WCHAR szKey[ANYSIZE_ARRAY];
	WORD  Padding[ANYSIZE_ARRAY];
	Var   Children[ANYSIZE_ARRAY];
};

//PIMAGE_RESOURCE_DIRECTORY_ENTRY g_pStrResEntries = 0;
//PIMAGE_RESOURCE_DIRECTORY_ENTRY g_pDlgResEntries = 0;
//DWORD g_cStrResEntries = 0;
//DWORD g_cDlgResEntries = 0;

//// Get an ASCII string representing a resource type
//void GetResourceTypeName(DWORD type, PSTR buffer, UINT cBytes)
//{
//	if ( type <= (WORD)RT_MANIFEST )
//		strncpy(buffer, SzResourceTypes[type], cBytes);
//	else
//		wsprintfA(buffer, "0x%X", type);
//}

////
//// If a resource entry has a string name (rather than an ID), go find
//// the string and convert it from unicode to ascii.
////
//void GetResourceNameFromId
//(
// DWORD id, PBYTE pResourceBase, PSTR buffer, UINT cBytes
// )
//{
//	PIMAGE_RESOURCE_DIR_STRING_U prdsu;
//
//	// If it's a regular ID, just format it.
//	if ( !(id & IMAGE_RESOURCE_NAME_IS_STRING) )
//	{
//		wsprintfA(buffer, "0x%04X", id);
//		return;
//	}
//
//	id &= 0x7FFFFFFF;
//	prdsu = MakePtr(PIMAGE_RESOURCE_DIR_STRING_U, pResourceBase, id);
//
//	// prdsu->Length is the number of unicode characters
//	WideCharToMultiByte(CP_ACP, 0, prdsu->NameString, prdsu->Length,
//		buffer, cBytes, 0, 0);
//	buffer[ std::min(cBytes-1,prdsu->Length) ] = 0;  // Null terminate it!!!
//}


//// Get an ASCII string representing a resource type
//void GetOS2ResourceTypeName(DWORD type, PSTR buffer, UINT cBytes)
//{
//	if ((type & 0x8000) == 0x8000) {
//		GetResourceTypeName(type & 0x7FFF, buffer, cBytes-10);
//		strcat(buffer, " [16bit]");
//	} else {
//		GetResourceTypeName(type, buffer, cBytes);
//	}
//}

typedef struct tag_OS2RC_TNAMEINFO
{
	USHORT rnOffset;
	USHORT rnLength;
	UINT   rnID;
	USHORT rnHandle;
	USHORT rnUsage;
} OS2RC_TNAMEINFO, *POS2RC_TNAMEINFO;

typedef struct tag_OS2RC_TYPEINFO
{
	USHORT rtTypeID;
	USHORT rtResourceCount;
	UINT   rtReserved;
	OS2RC_TNAMEINFO rtNameInfo[1];
} OS2RC_TYPEINFO, *POS2RC_TYPEINFO;

void CreateResource(PEData *pData, DWORD rootType, LPVOID ptrRes, DWORD resSize,
                    DWORD resourceType, DWORD resourceID);


void DumpNEResourceTable(PEData *pData, PIMAGE_DOS_HEADER dosHeader, LPBYTE pResourceTable)
{
	_TRACE0("DumpNEResourceTable");
	PBYTE pImageBase = (PBYTE)dosHeader;

	//MPanelItem* pChild = pRoot->AddFolder(_T("Resource Table"));
	//pChild->AddText(_T("<Resource Table>\n"));

	// минимальный размер
	size_t nReqSize = sizeof(OS2RC_TYPEINFO)+12;
	//if (IsBadReadPtr(pResourceTable, nReqSize)) {
	if (!ValidateMemory(pData, pResourceTable, nReqSize))
	{
		//pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
		//	(DWORD)(pResourceTable - pImageBase));
		return;
	}

	//
	USHORT rscAlignShift = *(USHORT*)pResourceTable;
	OS2RC_TYPEINFO* pTypeInfo = (OS2RC_TYPEINFO*)(pResourceTable+2);
	//char szTypeName[128];
	//char szResName[256];
	UINT nResLength = 0, nResOffset = 0;
	//LPBYTE pNames;

	// Сначала нужно найти начало имен
	pTypeInfo = (OS2RC_TYPEINFO*)(pResourceTable+2);

	while(pTypeInfo->rtTypeID)
	{
		OS2RC_TNAMEINFO* pResName = pTypeInfo->rtNameInfo;

		// Next resource type
		pTypeInfo = (OS2RC_TYPEINFO*)(pResName+pTypeInfo->rtResourceCount);
		//if (IsBadReadPtr(pTypeInfo, 2)) {
		if (!ValidateMemory(pData, pTypeInfo, 2))
		{
			//pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
			//	(DWORD)(((LPBYTE)pTypeInfo) - pImageBase));
			return;
		}
	}
	//pNames = ((LPBYTE)pTypeInfo)+2;

	// Теперь, собственно ресурсы
	pTypeInfo = (OS2RC_TYPEINFO*)(pResourceTable+2);

	while(pTypeInfo->rtTypeID)
	{
		//szTypeName[0] = 0;
		//GetOS2ResourceTypeName(pTypeInfo->rtTypeID, szTypeName, sizeof(szTypeName));

		//MPanelItem* pType = pChild->AddFolder(szTypeName);
		//pType->printf("  <%s>:\n", szTypeName);

		//pType->printf(_T("    Resource count:   %i\n"), pTypeInfo->rtResourceCount);
		OS2RC_TNAMEINFO* pResName = pTypeInfo->rtNameInfo;

		for(USHORT i = pTypeInfo->rtResourceCount; i--; pResName++)
		{
			nResLength = pResName->rnLength * (1 << rscAlignShift);
			nResOffset = pResName->rnOffset * (1 << rscAlignShift);

			//szResName[0] = 0;
			//if (pNames) {
			//	if (IsBadReadPtr(pNames, 1)) {
			//		//pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
			//		//	(DWORD)(pNames - pImageBase));
			//		pNames = NULL;
			//	} else if (IsBadReadPtr(pNames, 1+(*pNames))) {
			//		//pChild->printf(_T("!!! Can't read memory at offset:  0x%08X\n"),
			//		//	(DWORD)(pNames - pImageBase));
			//		pNames = NULL;
			//	} else if (*pNames) {
			//		memmove(szResName, pNames+1, *pNames);
			//		szResName[*pNames] = 0;
			//		pNames += (*pNames)+1;
			//	} else {
			//		pNames++;
			//	}
			//}
			//if (szResName[0]) {
			//	wsprintfA(szResName+strlen(szResName), ".0x%08X", pResName->rnID);
			//} else {
			//	wsprintfA(szResName, "ResID=0x%08X", pResName->rnID);
			//}

			//MPanelItem* pRes = NULL;
			if (nResLength && nResOffset)
			{
				/*pRes =*/ CreateResource(pData, pTypeInfo->rtTypeID,
				                          pImageBase+nResOffset, nResLength,
				                          pResName->rnID/*szResName*/, NULL);
				//} else {
				//	pRes = pType->AddFile(szResName, nResOffset ? nResLength : 0);
			}
			//pType->printf("    <%s>\n", szResName);
			//pType->printf("      Resource Name:    %s\n", szResName);
			//pType->printf("      Resource ID:      0x%08X\n", pResName->rnID);
			//pType->printf("      Resource length:  %u bytes\n", nResLength);
			//pType->printf("      Resource offset:  0x%08X\n", nResOffset);
			//pType->printf("      Handle(reserved): 0x%04X\n", (DWORD)pResName->rnHandle);
			//pType->printf("      Usage(reserved):  0x%04X\n", (DWORD)pResName->rnUsage);
			//if (nResLength && nResOffset) {
			//	pRes->SetData(pImageBase+nResOffset, nResLength);
			//}
		}


		// Next resource type
		pTypeInfo = (OS2RC_TYPEINFO*)pResName;
	}
}

bool DumpExeFileNE(PEData *pData, PIMAGE_DOS_HEADER dosHeader, PIMAGE_OS2_HEADER pOS2Header)
{
	_TRACE0("DumpExeFileNE");
	//PBYTE pImageBase = (PBYTE)dosHeader;

	pData->nBits = 16;
	//pRoot->Root()->AddFlags(_T("16BIT"));

	//MPanelItem* pChild = pRoot->AddFolder(_T("OS2 Header"));
	//pChild->AddText(_T("<OS2 Header>\n"));

	//MPanelItem* pDos = pRoot->AddFile(_T("DOS_Header"), sizeof(*dosHeader));
	//pDos->SetData((const BYTE*)dosHeader, sizeof(*dosHeader));

	//MPanelItem* pOS2 = pRoot->AddFile(_T("OS2_Header"), sizeof(*pOS2Header));
	//pOS2->SetData((const BYTE*)pOS2Header, sizeof(*pOS2Header));

	if (pOS2Header->ne_magic != IMAGE_OS2_SIGNATURE)
	{
		//pChild->AddText(_T("  IMAGE_OS2_SIGNATURE_LE signature not supported\n"));
		return false;
	}

	//pChild->AddText(_T("  Signature:         IMAGE_OS2_SIGNATURE\n"));
	//
	//pChild->printf(_T("  Version number:                     %02u\n"), (UINT)pOS2Header->ne_ver);
	//pChild->printf(_T("  Revision number:                    %02u\n"), (UINT)pOS2Header->ne_rev);
	//pChild->printf(_T("  Offset of Entry Table:              %04u\n"), (UINT)pOS2Header->ne_enttab);
	//pChild->printf(_T("  Number of bytes in Entry Table:     %04u\n"), (UINT)pOS2Header->ne_cbenttab);
	//pChild->printf(_T("  Checksum of whole file:             %08u\n"), (UINT)pOS2Header->ne_crc);
	//pChild->printf(_T("  Flag word:                          %04u\n"), (UINT)pOS2Header->ne_flags);
	//pChild->printf(_T("  Automatic data segment number:      %04u\n"), (UINT)pOS2Header->ne_autodata);
	//pChild->printf(_T("  Initial heap allocation:            %04u\n"), (UINT)pOS2Header->ne_heap);
	//pChild->printf(_T("  Initial stack allocation:           %04u\n"), (UINT)pOS2Header->ne_stack);
	//pChild->printf(_T("  Initial CS:IP setting:              %08u\n"), (UINT)pOS2Header->ne_csip);
	//pChild->printf(_T("  Initial SS:SP setting:              %08u\n"), (UINT)pOS2Header->ne_sssp);
	//pChild->printf(_T("  Count of file segments:             %04u\n"), (UINT)pOS2Header->ne_cseg);
	//pChild->printf(_T("  Entries in Module Reference Table:  %04u\n"), (UINT)pOS2Header->ne_cmod);
	//pChild->printf(_T("  Size of non-resident name table:    %04u\n"), (UINT)pOS2Header->ne_cbnrestab);
	//pChild->printf(_T("  Offset of Segment Table:            %04u\n"), (UINT)pOS2Header->ne_segtab);
	//pChild->printf(_T("  Offset of Resource Table:           %04u\n"), (UINT)pOS2Header->ne_rsrctab);
	//pChild->printf(_T("  Offset of resident name table:      %04u\n"), (UINT)pOS2Header->ne_restab);
	//pChild->printf(_T("  Offset of Module Reference Table:   %04u\n"), (UINT)pOS2Header->ne_modtab);
	//pChild->printf(_T("  Offset of Imported Names Table:     %04u\n"), (UINT)pOS2Header->ne_imptab);
	//pChild->printf(_T("  Offset of Non-resident Names Table: %08u\n"), (UINT)pOS2Header->ne_nrestab);
	//pChild->printf(_T("  Count of movable entries:           %04u\n"), (UINT)pOS2Header->ne_cmovent);
	//pChild->printf(_T("  Segment alignment shift count:      %04u\n"), (UINT)pOS2Header->ne_align);
	//pChild->printf(_T("  Count of resource segments:         %04u\n"), (UINT)pOS2Header->ne_cres);
	//pChild->printf(_T("  Target Operating system:            %02u\n"), (UINT)pOS2Header->ne_exetyp);
	//pChild->printf(_T("  Other .EXE flags:                   %02u\n"), (UINT)pOS2Header->ne_flagsothers);
	//pChild->printf(_T("  offset to return thunks:            %04u\n"), (UINT)pOS2Header->ne_pretthunks);
	//pChild->printf(_T("  offset to segment ref. bytes:       %04u\n"), (UINT)pOS2Header->ne_psegrefbytes);
	//pChild->printf(_T("  Minimum code swap area size:        %04u\n"), (UINT)pOS2Header->ne_swaparea);
	//pChild->printf(_T("  Expected Windows version number:    %04u\n"), (UINT)pOS2Header->ne_expver);
	//
	//pChild->AddText(_T("\n"));

	if (pOS2Header->ne_rsrctab)
	{
		LPBYTE pResourceTable = (((LPBYTE)pOS2Header)+pOS2Header->ne_rsrctab);
		DumpNEResourceTable(pData, dosHeader, pResourceTable);
		//MPanelItem* pChild = pRoot->AddFolder(_T("Resource Table"));
		//pChild->AddText(_T("<Resource Table>\n"));
	}

	return true;
}


//DWORD GetOffsetToDataFromResEntry( 	PBYTE pResourceBase,
//									PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry )
//{
//	// The IMAGE_RESOURCE_DIRECTORY_ENTRY is gonna point to a single
//	// IMAGE_RESOURCE_DIRECTORY, which in turn will point to the
//	// IMAGE_RESOURCE_DIRECTORY_ENTRY, which in turn will point
//	// to the IMAGE_RESOURCE_DATA_ENTRY that we're really after.  In
//	// other words, traverse down a level.
//
//	PIMAGE_RESOURCE_DIRECTORY pStupidResDir;
//	pStupidResDir = (PIMAGE_RESOURCE_DIRECTORY)(pResourceBase + pResEntry->OffsetToDirectory);
//
//    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry =
//	    	(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pStupidResDir + 1);// PTR MATH
//
//	PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry =
//			(PIMAGE_RESOURCE_DATA_ENTRY)(pResourceBase + pResDirEntry->OffsetToData);
//
//	return pResDataEntry->OffsetToData;
//}




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
	DWORD   dwBytesInRes;         // how many bytes in this resource?
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

//void ResourceParseFlags(DWORD dwFlags, wchar_t* pszFlags, DWORD_FLAG_DESCRIPTIONSW* pFlags)
//{
//	BOOL bFirst = TRUE;
//	wchar_t* pszStart = pszFlags;
//	StringCchPrintf(pszFlags, countof(pszFlags), L"0x%XL", dwFlags);
//	pszFlags += lstrlenW(pszFlags);
//	*pszFlags = 0;
//
//	while (pFlags->flag)
//	{
//		if ( (dwFlags & pFlags->flag) == pFlags->flag ) {
//			if (bFirst) {
//				bFirst = FALSE;
//				while ((pszFlags - pszStart) < 8) {
//					*(pszFlags++) = L' '; *pszFlags = 0;
//				}
//				wcscpy(pszFlags, L" // "); pszFlags += lstrlen(pszFlags);
//			} else {
//				*(pszFlags++) = L'|';
//			}
//			wcscpy(pszFlags, pFlags->name);
//			pszFlags += lstrlen(pszFlags);
//		}
//		pFlags++;
//	}
//	*pszFlags = 0;
//}

void ParseVersionInfoFixed(PEData *pData,  VS_FIXEDFILEINFO* pVer)
{
	_TRACE0("ParseVersionInfoFixed");
	wchar_t szTest[32];
	UINT
		nV1 = HIWORD(pVer->dwFileVersionMS),
		nV2 = LOWORD(pVer->dwFileVersionMS),
		nV3 = HIWORD(pVer->dwFileVersionLS),
		nV4 = LOWORD(pVer->dwFileVersionLS);
	wsprintfW(pData->szVersionN,
		nV4 ? L"%i.%i.%i.%i" : nV3 ? L"%i.%i.%i" : L"%i.%i",
	    nV1, nV2, nV3, nV4);

	if (*pData->szVersionF)
		pData->szVersion = pData->szVersionF;
	else if (*pData->szVersionP)
		pData->szVersion = pData->szVersionP;
	if (pData->szVersion && *pData->szVersion)
	{
		// Чтобы сравнить версию с той, что лежит в строковой части VersionInfo
		// В таком формате, например, формирует ее VisualC
		swprintf_c(szTest, L"%i, %i, %i, %i", nV1, nV2, nV3, nV4);
		if (!lstrcmpiW(szTest, pData->szVersion))
			pData->szVersion = NULL;
		// А в таком - можно сэкономить место, отрезав замыкающие нули
		swprintf_c(szTest, L"%i.%i.%i.%i", nV1, nV2, nV3, nV4);
		if (!lstrcmpiW(szTest, pData->szVersion))
			pData->szVersion = NULL;
		
		if (pData->szVersion != NULL)
		{
			// Если в строковой части таки нет информации о версии?
			// Некоторые дурные rc файлы содержат не строки, а "рожицы" :)
			if (!wcschr(pData->szVersion, L'.') && !wcschr(pData->szVersion, L',')
				&& (nV2 || nV3 || nV4))
			{
				_ASSERTE(ARRAYSIZE(pData->szVersionF)==ARRAYSIZE(pData->szVersionP));
				int nCurLen = lstrlen(pData->szVersion);
				if ((nCurLen+lstrlen(pData->szVersionN)+4) < ARRAYSIZE(pData->szVersionF))
				{
					wchar_t* psz = pData->szVersion+nCurLen;
					*(psz++) = L' '; *(psz++) = L'(';
					lstrcpy(psz, pData->szVersionN);
					lstrcat(psz, L")");
				}
			}
		}
	}
	if (pData->szVersion == NULL)
		pData->szVersion = pData->szVersionN;
	//wchar_t szMask[255], szFlags[255], szOS[255], szFileType[64], szFileSubType[64];
	//ResourceParseFlags(pVer->dwFileFlagsMask, szMask, VersionInfoFlags);
	//ResourceParseFlags(pVer->dwFileFlags, szFlags, VersionInfoFlags);
	//ResourceParseFlags(pVer->dwFileOS, szOS, VersionInfoFileOS);
	//szFileType[0] = 0;
	//StringCchPrintf(szFileSubType, countof(szFileSubType), L"0x%XL", pVer->dwFileSubtype);
	//switch (pVer->dwFileType) {
	//	case VFT_APP: wcscpy(szFileType, L"     // VFT_APP"); break;
	//	case VFT_DLL: wcscpy(szFileType, L"     // VFT_DLL"); break;
	//	case VFT_DRV: wcscpy(szFileType, L"     // VFT_DRV");
	//		{
	//			wchar_t* pszStart = szFileSubType;
	//			wchar_t* pszFlags = pszStart + lstrlenW(pszStart);
	//			while ((pszFlags - pszStart) < 8) {
	//				*(pszFlags++) = L' '; *pszFlags = 0;
	//			}
	//			for (int k=0; VersionInfoDrvSubtype[k].flag; k++) {
	//				if (pVer->dwFileSubtype == VersionInfoDrvSubtype[k].flag) {
	//					wcscat(szFileSubType, L" // "); wcscat(szFileSubType, VersionInfoDrvSubtype[k].name);
	//					break;
	//				}
	//			}
	//		}
	//		break;
	//	case VFT_FONT: wcscpy(szFileType, L"     // VFT_FONT");
	//		{
	//			wchar_t* pszStart = szFileSubType;
	//			wchar_t* pszFlags = pszStart + lstrlenW(pszStart);
	//			while ((pszFlags - pszStart) < 8) {
	//				*(pszFlags++) = L' '; *pszFlags = 0;
	//			}
	//			for (int k=0; VersionInfoFontSubtype[k].flag; k++) {
	//				if (pVer->dwFileSubtype == VersionInfoFontSubtype[k].flag) {
	//					wcscat(szFileSubType, L" // "); wcscat(szFileSubType, VersionInfoFontSubtype[k].name);
	//					break;
	//				}
	//			}
	//		}
	//		break;
	//	case VFT_VXD: wcscpy(szFileType, L"     // VFT_VXD"); break;
	//	case VFT_STATIC_LIB: wcscpy(szFileType, L"     // VFT_STATIC_LIB"); break;
	//}
	//StringCchPrintf(psz, countof(psz),
	//	L"#include <windows.h>\n\n"
	//	L"VS_VERSION_INFO VERSIONINFO\n"
	//	L" FILEVERSION    %u,%u,%u,%u\n"
	//	L" PRODUCTVERSION %u,%u,%u,%u\n"
	//	L" FILEFLAGSMASK  %s\n"
	//	L" FILEFLAGS      %s\n"
	//	L" FILEOS         %s\n"
	//	L" FILETYPE       0x%XL%s\n"
	//	L" FILESUBTYPE    %s\n"
	//	L"BEGIN\n"
	//	,
	//	HIWORD(pVer->dwFileVersionMS), LOWORD(pVer->dwFileVersionMS),
	//	HIWORD(pVer->dwFileVersionLS), LOWORD(pVer->dwFileVersionLS),
	//	HIWORD(pVer->dwProductVersionMS), LOWORD(pVer->dwProductVersionMS),
	//	HIWORD(pVer->dwProductVersionLS), LOWORD(pVer->dwProductVersionLS),
	//	szMask, szFlags,
	//	szOS,
	//	pVer->dwFileType, szFileType,
	//	szFileSubType
	//	);
	//psz += lstrlen(psz);
}

#define ALIGN_TOKEN(p) p = (LPWORD)( ((( ((DWORD_PTR)p) - ((DWORD_PTR)ptrRes) + 3) >> 2) << 2 ) + ((DWORD_PTR)ptrRes))

void ParseVersionInfoVariableString(PEData *pData, LPVOID ptrRes, DWORD &resSize, LPWORD pToken)
{
	_TRACE0("ParseVersionInfoVariableString");
	StringFileInfo *pSFI = (StringFileInfo*)pToken;
	LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);

	if (pToken < pEnd && *pToken > sizeof(StringFileInfo))
	{
		LPWORD pEnd1 = (LPWORD)(((LPBYTE)pToken)+*pToken);
		if (pEnd < pEnd1)
			pEnd1 = pEnd;
		//wcscat(psz, L"    BLOCK \"");
		//wcscat(psz, pSFI->szKey);
		//wcscat(psz, L"\"\n");
		if (pSFI->wType != 1)
		{
			//wcscat(psz, L"    // Warning! Binary data in StringFileInfo\n");
		}
		{
			//wcscat(psz, L"    BEGIN\n");
			//psz += lstrlen(psz);
			// Padding - Contains as many zero words as necessary to align the Children member on a 32-bit boundary.
			pToken = (LPWORD)(pSFI->szKey+lstrlen(pSFI->szKey)+1);
			//while (*pToken == 0 && pToken < pEnd1) pToken++;
			ALIGN_TOKEN(pToken);

			if ((((LPBYTE)pToken)+sizeof(StringTable)) <= (LPBYTE)pEnd1)
			{
				StringTable *pST = (StringTable*)pToken;
				LPWORD pEnd2 = (LPWORD)(((LPBYTE)pToken)+*pToken);
				if (pEnd1 < pEnd2)
					pEnd2 = pEnd1;

				// Specifies an 8-digit hexadecimal number stored as a Unicode string.
				// The four most significant digits represent the language identifier.
				// The four least significant digits represent the code page for which
				// the data is formatted. Each Microsoft Standard Language identifier contains
				// two parts: the low-order 10 bits specify the major language,
				// and the high-order 6 bits specify the sublanguage.
				//wcscat(psz, L"        BLOCK \"");
				//psz += lstrlen(psz);
				//memmove(psz, pST->szKey, 8*2);
				//psz += 8; *psz = 0; wcscat(psz, L"\"\n");
				//wcscat(psz, L"        BEGIN\n");
				pToken = (LPWORD)(pST->szKey+8);
				//while (*pToken == 0 && pToken < pEnd2) pToken++;
				ALIGN_TOKEN(pToken);

				while((((LPBYTE)pToken)+sizeof(String)) <= (LPBYTE)pEnd2)
				{
					String *pS = (String*)pToken;

					if (pS->wLength == 0) break;  // Invalid?

					LPWORD pNext = (LPWORD)(((LPBYTE)pToken)+pS->wLength);
					//wcscat(psz, L"            VALUE \""); psz += lstrlen(psz);
					//wcscat(psz, pS->szKey);
					//wcscat(psz, L"\", "); psz += lstrlen(psz);
					// Выровнять текст в результирующем .rc
					//for (int k = lstrlenW(pS->szKey); k < 17; k++) *(psz++) = L' ';
					//*(psz++) = L'"'; *psz = 0;
					pToken = (LPWORD)(pS->szKey+lstrlen(pS->szKey)+1);
					//while (*pToken == 0 && pToken < pEnd2) pToken++;
					ALIGN_TOKEN(pToken);
					int nLenLeft = pS->wValueLength;
					wchar_t* psz = NULL; int nDstLeft = 0;
					if (!lstrcmpW(pS->szKey, L"FileVersion"))
					{
						psz = pData->szVersionF; nDstLeft = (int)ARRAYSIZE(pData->szVersionF);
					}
					else if (!lstrcmpW(pS->szKey, L"ProductVersion"))
					{
						psz = pData->szVersionP; nDstLeft = (int)ARRAYSIZE(pData->szVersionF);
					}

					while ((pToken < pEnd2) && (nLenLeft > 0) && *pToken)
					{
						if ((--nDstLeft)>0) switch (*pToken)
						{
							case 0:
								/**(psz++) = L'\\'; *(psz++) = L'0';*/ break;
							case L'\r':
								//*(psz++) = L'\\'; *(psz++) = L'r'; break;
							case L'\n':
								//*(psz++) = L'\\'; *(psz++) = L'n'; break;
							case L'\t':
								//*(psz++) = L'\\'; *(psz++) = L't'; break;
								*(psz++) = L' ';
							default:
								*(psz++) = *pToken;
						}
						pToken++; nLenLeft--;
					}
					if (psz) *psz = 0;
					//if (pToken < pEnd2 && pS->wValueLength) {
					//	// Вообще-то тут бы провести замены \r\n\t"
					//	wcscat(psz, (LPCWSTR)pToken);
					//}
					//wcscat(psz, L"\"\n"); psz += lstrlen(psz);

					// Next value
					pToken = pNext;
					if (pToken < pEnd2 && *pToken == 0)
					{
						//wcscat(psz, L"            // Zero-length item found\n"); psz += lstrlen(psz);
						while (pToken < pEnd2 && *pToken == 0) pToken ++;
					}
				}
				//wcscat(psz, L"        END\n");
			}
			//
			//wcscat(psz, L"    END\n");
		}
	}
}
void ParseVersionInfoVariableStringA(PEData *pData, LPVOID ptrRes, DWORD &resSize, char* pToken)
{
	_TRACE0("ParseVersionInfoVariableStringA");
	wchar_t szTemp[MAX_PATH*2+1], *pwsz = NULL;
	StringFileInfoA *pSFI = (StringFileInfoA*)pToken;
	char* pEnd = (char*)(((LPBYTE)ptrRes)+resSize);

	if (pToken < pEnd && *pToken > sizeof(StringFileInfoA))
	{
		char* pEnd1 = (char*)(((LPBYTE)pToken)+*((WORD*)pToken));
		if (pEnd < pEnd1)
			pEnd1 = pEnd;
		//wcscat(psz, L"    BLOCK \"");
		//wcscat(psz, pSFI->szKey);
		//psz += lstrlen(psz);
		//MultiByteToWideChar(CP_ACP,0,pSFI->szKey,-1,psz,64);
		//for (int x=0; x<8; x++) {
		//	*psz++ = pSFI->szKey[x] ? pSFI->szKey[x] : L' ';
		//}
		//wcscat(psz, L"\"\n");
		//if (pSFI->wType != 1) {
		//	wcscat(psz, L"    // Warning! Binary data in StringFileInfo\n");
		//}
		{
			//wcscat(psz, L"    BEGIN\n");
			//psz += lstrlen(psz);
			// Padding - Contains as many zero words as necessary to align the Children member on a 32-bit boundary.
			pToken = (char*)(pSFI->szKey+strlen(pSFI->szKey)+1);
			//while (*pToken == 0 && pToken < pEnd1) pToken++;
			//ALIGN_TOKEN(pToken); ???
			pToken++; // ???

			if ((((LPBYTE)pToken)+sizeof(StringTableA)) <= (LPBYTE)pEnd1)
			{
				StringTableA *pST = (StringTableA*)pToken;
				char* pEnd2 = (char*)(((LPBYTE)pToken)+*((WORD*)pToken));
				if (pEnd2 > pEnd1)
					pEnd2 = pEnd1;

				// Specifies an 8-digit hexadecimal number stored as a Unicode string.
				// The four most significant digits represent the language identifier.
				// The four least significant digits represent the code page for which
				// the data is formatted. Each Microsoft Standard Language identifier contains
				// two parts: the low-order 10 bits specify the major language,
				// and the high-order 6 bits specify the sublanguage.
				//wcscat(psz, L"        BLOCK \"");
				//psz += lstrlen(psz);
				//wmemmove(psz, pST->szKey, 8); ???
				//psz[MultiByteToWideChar(CP_ACP,0,pST->szKey,8,psz,64)] = 0;
				//psz += 8; *psz = 0; wcscat(psz, L"\"\n");
				//wcscat(psz, L"        BEGIN\n");
				pToken = (char*)(pST->szKey+8);
				//while (*pToken == 0 && pToken < pEnd2) pToken++;
				//ALIGN_TOKEN(pToken); ???
				pToken += 4; //???

				while((((LPBYTE)pToken)+sizeof(StringA)) <= (LPBYTE)pEnd2)
				{
					StringA *pS = (StringA*)pToken;

					if (pS->wLength == 0) break;  // Invalid?

					char* pNext = (char*)(((LPBYTE)pToken)+pS->wLength);
					if (pNext > pEnd2)
						pNext = pEnd2;
					//wcscat(psz, L"            VALUE \""); psz += lstrlen(psz);
					//wcscat(psz, pS->szKey); ???
					//psz[MultiByteToWideChar(CP_ACP,0,pS->szKey,-1,psz,32)] = 0;
					//wcscat(psz, L"\", "); psz += lstrlen(psz);
					// Выровнять текст в результирующем .rc
					//for (int k = lstrlenA(pS->szKey); k < 17; k++) *(psz++) = L' ';
					//*(psz++) = L'"'; *psz = 0;
					pToken = (char*)(pS->szKey+strlen(pS->szKey)+1);

					while(*pToken == 0 && pToken < pNext) pToken++;

					//ALIGN_TOKEN(pToken); ???
					int nLenLeft = std::min<WORD>(pS->wValueLength, MAX_PATH*2);
					if (nLenLeft > (pNext-pToken))
						nLenLeft = (int)(pNext-pToken);
					szTemp[MultiByteToWideChar(CP_ACP,0,pToken,nLenLeft,szTemp,nLenLeft)] = 0;
					pwsz = szTemp;
					wchar_t* psz = NULL; int nDstLeft = 0;
					if (!lstrcmpA(pS->szKey, "FileVersion"))
					{
						psz = pData->szVersionF; nDstLeft = (int)ARRAYSIZE(pData->szVersionF);
					}
					else if (!lstrcmpA(pS->szKey, "ProductVersion"))
					{
						psz = pData->szVersionP; nDstLeft = (int)ARRAYSIZE(pData->szVersionF);
					}

					while (nLenLeft>0 && *pwsz)
					{
						if ((--nDstLeft)>0) switch (*pwsz)
						{
							case 0:
								/**(psz++) = L'\\'; *(psz++) = L'0';*/ break;
							case L'\r':
								//*(psz++) = L'\\'; *(psz++) = L'r'; break;
							case L'\n':
								//*(psz++) = L'\\'; *(psz++) = L'n'; break;
							case L'\t':
								//*(psz++) = L'\\'; *(psz++) = L't'; break;
								*(psz++) = L' ';
							default:
								*(psz++) = *pwsz;
						}
						pwsz++; nLenLeft--;
					}
					if (psz) *psz = 0;
					//if (pToken < pEnd2 && pS->wValueLength) {
					//	// Вообще-то тут бы провести замены \r\n\t"
					//	wcscat(psz, (LPCWSTR)pToken);
					//}
					//wcscat(psz, L"\"\n"); psz += lstrlen(psz);

					// Next value
					pToken = pNext;
					if (pToken < pEnd2 && *pToken == 0)
					{
						//wcscat(psz, L"            // Zero-length item found\n"); psz += lstrlen(psz);
						while (pToken < pEnd2 && *pToken == 0) pToken ++;
					}
				}
				//wcscat(psz, L"        END\n");
			}
			//
			//wcscat(psz, L"    END\n");
		}
	}
}

void ParseVersionInfoVariableVar(PEData *pData, LPVOID ptrRes, DWORD &resSize, LPWORD pToken)
{
	_TRACE0("ParseVersionInfoVariableVar");
	VarFileInfo *pSFI = (VarFileInfo*)pToken;
	LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
	if (pToken < pEnd && *pToken > sizeof(VarFileInfo))
	{
		LPWORD pEnd1 = (LPWORD)(((LPBYTE)pToken)+*pToken);
		if (pEnd < pEnd1)
			pEnd1 = pEnd;
		//wcscat(psz, L"    BLOCK \"");
		//wcscat(psz, pSFI->szKey);
		//wcscat(psz, L"\"\n");
		if (pSFI->wType != 1)
		{
			//wcscat(psz, L"    // Warning! Binary data in VarFileInfo\n");
		}
		{
			//wcscat(psz, L"    BEGIN\n");
			//psz += lstrlen(psz);
			// Padding - Contains as many zero words as necessary to align the Children member on a 32-bit boundary.
			pToken = (LPWORD)(pSFI->szKey+lstrlen(pSFI->szKey)+1);
			//while (*pToken == 0 && pToken < pEnd1) pToken++;
			ALIGN_TOKEN(pToken);

			if ((((LPBYTE)pToken)+sizeof(Var)) <= (LPBYTE)pEnd1)
			{
				pToken = (LPWORD)(pSFI->szKey+lstrlen(pSFI->szKey)+1);
				//while (*pToken == 0 && pToken < pEnd1) pToken++;
				ALIGN_TOKEN(pToken);

				while((((LPBYTE)pToken)+sizeof(Var)) <= (LPBYTE)pEnd1)
				{
					Var *pS = (Var*)pToken;

					if (pS->wLength == 0) break;  // Invalid?

					LPWORD pNext = (LPWORD)(((LPBYTE)pToken)+pS->wLength);
					//wcscat(psz, L"        VALUE \""); psz += lstrlen(psz);
					//wcscat(psz, pS->szKey);
					//wcscat(psz, L"\""); psz += lstrlen(psz);
					pToken = (LPWORD)(pS->szKey+lstrlen(pS->szKey)+1);
					// Align to 32bit boundary
					ALIGN_TOKEN(pToken);
					//pToken++;
					//while (*pToken == 0 && pToken < pEnd1) pToken++;

					// The low-order word of each DWORD must contain a Microsoft language identifier,
					// and the high-order word must contain the IBM code page number.
					// Either high-order or low-order word can be zero, indicating that the file
					// is language or code page independent.
					while((pToken+2) <= pEnd1)
					{
						//psz += lstrlen(psz);
						//DWORD nLangCP = *((LPDWORD)pToken);
						//StringCchPrintf(psz, countof(psz), L", 0x%X, %u", (DWORD)(pToken[0]), (DWORD)(pToken[1]));
						pToken += 2;
					}
					//	// Вообще-то тут бы провести замены \r\n\t"
					//	wcscat(psz, (LPCWSTR)pToken);
					//}
					//wcscat(psz, L"\n"); psz += lstrlen(psz);

					// Next value
					pToken = pNext;
				}
			}
			//wcscat(psz, L"    END\n");
		}
	}
}

void ParseVersionInfoVariable(PEData *pData, LPVOID ptrRes, DWORD &resSize, LPWORD pToken)
{
	_TRACE0("ParseVersionInfoVariable");
	StringFileInfo *pSFI = (StringFileInfo*)pToken;

	if (lstrcmpiW(pSFI->szKey, L"StringFileInfo") == 0)
	{
		LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
		if (pToken < pEnd && *pToken > sizeof(StringFileInfo))
		{
			ParseVersionInfoVariableString(pData, ptrRes, resSize, /*ptrBuf, bIsCopy,*/ pToken/*, psz*/);
		}
	}
	else if (lstrcmpiW(pSFI->szKey, L"VarFileInfo") == 0)
	{
		LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);

		if (pToken < pEnd && *pToken > sizeof(VarFileInfo))
		{
			ParseVersionInfoVariableVar(pData, ptrRes, resSize/*, ptrBuf, bIsCopy*/, pToken/*, psz*/);
		}
	}
}
void ParseVersionInfoVariableA(PEData *pData, LPVOID ptrRes, DWORD &resSize, char* pToken)
{
	StringFileInfoA *pSFI = (StringFileInfoA*)pToken;

	if (lstrcmpiA(pSFI->szKey, "StringFileInfo") == 0)
	{
		char* pEnd = (char*)(((LPBYTE)ptrRes)+resSize);
		if (pToken < pEnd && *pToken > sizeof(StringFileInfo))
		{
			ParseVersionInfoVariableStringA(pData, ptrRes, resSize, pToken);
		}
	}
}

void ParseVersionInfo(PEData *pData, LPVOID &ptrRes, DWORD &resSize)
{
	_TRACE0("ParseVersionInfo");
	//Msg("ParseVersionInfo","0");

	if (resSize < sizeof(VS_FIXEDFILEINFO)+0x28)
	{
		_TRACE2("Invalid resource size=%i in ParseVersionInfo. MinRequired=%i", resSize, sizeof(VS_FIXEDFILEINFO)+0x28);
		return;
	}

	if (!ValidateMemory(pData, ptrRes, std::max<size_t>(resSize, sizeof(VS_FIXEDFILEINFO)+0x28)))
	{
		_TRACE("Invalid memory pointer (ptrRes) in ParseVersionInfo");
		return;
	}

	WORD nTestSize = ((WORD*)ptrRes)[0];
	WORD nTestShift = ((WORD*)ptrRes)[1];

	//Msg("ParseVersionInfo","1");

	if (nTestSize == resSize && nTestShift < resSize)
	{
		VS_FIXEDFILEINFO* pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+0x28);
		// По идее, должно быть здесь, но если нет - ищем сигнатуру
		if (pVer->dwSignature != 0xfeef04bd)
		{
			DWORD nMax = resSize - sizeof(VS_FIXEDFILEINFO);

			for(UINT i = 4; i < nMax; i++)
			{
				if (((VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i))->dwSignature == 0xfeef04bd)
				{
					pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i);
					break;
				}
			}
		}
		//Msg("ParseVersionInfo","2");
		DWORD nNewSize = resSize*2 + 2048;

		if (pVer->dwSignature == 0xfeef04bd && nNewSize > resSize)
		{
			/*ptrBuf = (LPBYTE)malloc(nNewSize);
			if (!ptrBuf) {
				//pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
			} else {*/
				//wchar_t* psz = (LPWSTR)ptrBuf;
				//WORD nBOM = 0xFEFF; // CP-1200
				//*psz++ = nBOM;
				
				//#ifndef verc0_EXPORTS

				StringFileInfo *pSFI = (StringFileInfo*)(pVer+1);
				LPWORD pToken = (LPWORD)pSFI;
				LPWORD pEnd = (LPWORD)(((LPBYTE)ptrRes)+resSize);
				while (pToken < pEnd && *pToken > sizeof(StringFileInfo))
				{
					pSFI = (StringFileInfo*)pToken;
					if (pSFI->wLength == 0)
						break; // Invalid
					ParseVersionInfoVariable(pData, ptrRes, resSize, pToken);
					//
					pToken = (LPWORD)(((LPBYTE)pSFI)+pSFI->wLength);
					while (pToken < pEnd && *pToken == 0)
						pToken ++;
					//psz += lstrlen(psz);
				}

				//#endif

				//Msg("ParseVersionInfo","3");

				// VS_FIXEDFILEINFO
				ParseVersionInfoFixed(pData, pVer);
				

				//Msg("ParseVersionInfo","4");

				pData->nFlags |= PE_VER_EXISTS;

				// Done
				//wcscat(psz, L"END\n");
				{
					// сначала добавим "бинарные данные" ресурса (NOT PARSED)
					//pChild = pRoot->AddFile(fileNameBuffer, resSize);
					//pChild->SetData((LPBYTE)ptrRes, resSize);
				}
				//strcat(fileNameBuffer, ".rc");
				//ptrRes = ptrBuf;
				// И вернем преобразованные в "*.rc" (PARSED)
				//resSize = lstrlenW((LPWSTR)ptrBuf)*2; // первый WORD - BOM
				//bIsCopy = TRUE;
			//}
		}
	}

	//Msg("ParseVersionInfo","5");
}

// ANSI for 16bit PE's
void ParseVersionInfoA(PEData *pData, LPVOID &ptrRes, DWORD &resSize)
{
	if (resSize < sizeof(VS_FIXEDFILEINFO)+0x28)
	{
		_TRACE2("Invalid resource size=%i in ParseVersionInfo. MinRequired=%i", resSize, sizeof(VS_FIXEDFILEINFO)+0x28);
		return;
	}

	if (!ValidateMemory(pData, ptrRes, std::max<size_t>(resSize,sizeof(VS_FIXEDFILEINFO)+0x28)))
	{
		_TRACE("Invalid memory pointer (ptrRes) in ParseVersionInfo");
		return;
	}

	_TRACE0("ParseVersionInfoA");
	WORD nTestSize = ((WORD*)ptrRes)[0];
	//WORD nTestShift = ((WORD*)ptrRes)[1]; // ???
	if (nTestSize >= sizeof(VS_FIXEDFILEINFO) && nTestSize <= resSize /*&& nTestShift < resSize*/)
	{
		VS_FIXEDFILEINFO* pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+0x28);
		// По идее, должно быть здесь, но если нет - ищем сигнатуру
		if (pVer->dwSignature != 0xfeef04bd)
		{
			DWORD nMax = resSize - sizeof(VS_FIXEDFILEINFO);

			for(UINT i = 4; i < nMax; i++)
			{
				//BUGBUG: Проверить, на x64 жить такое будет, или свалится на Alignment?
				if (((VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i))->dwSignature == 0xfeef04bd)
				{
					pVer = (VS_FIXEDFILEINFO*)(((LPBYTE)ptrRes)+i);
					break;
				}
			}
		}
		DWORD nNewSize = resSize*2 + 2048;

		if (pVer->dwSignature == 0xfeef04bd  // Сигнатура найдена
		        && nNewSize > resSize)          // Размер ресурса (resSize) не превышает DWORD :)
		{
			/*ptrBuf = (LPBYTE)malloc(nNewSize);
			if (!ptrBuf) {
				//pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
			} else {*/
				//wchar_t* psz = (LPWSTR)ptrBuf;
				//WORD nBOM = 0xFEFF; // CP-1200
				//*psz++ = nBOM;

				//#ifndef verc0_EXPORTS

				StringFileInfoA *pSFI = (StringFileInfoA*)(pVer+1);
				char* pToken = (char*)pSFI;
				char* pEnd = (char*)(((LPBYTE)ptrRes)+resSize);
				while (pToken < pEnd && *pToken > sizeof(StringFileInfoA))
				{
					pSFI = (StringFileInfoA*)pToken;
					if (pSFI->wLength == 0)
						break; // Invalid
					ParseVersionInfoVariableA(pData, ptrRes, resSize, pToken);
					//
					pToken = (char*)(((LPBYTE)pSFI)+pSFI->wLength);
					while (pToken < pEnd && *pToken == 0)
						pToken ++;
					//psz += lstrlen(psz);
				}

				//#endif

				// VS_FIXEDFILEINFO
				ParseVersionInfoFixed(pData, pVer);

				pData->nFlags |= PE_VER_EXISTS;

				// Done
				//wcscat(psz, L"END\n");
				//{
				//	// сначала добавим "бинарные данные" ресурса (NOT PARSED)
				//	//pChild = pRoot->AddFile(fileNameBuffer, resSize);
				//	//pChild->SetData((LPBYTE)ptrRes, resSize);
				//}
				//strcat(fileNameBuffer, ".rc");
				//ptrRes = ptrBuf;
				//// И вернем преобразованные в "*.rc" (PARSED)
				//resSize = lstrlenW((LPWSTR)ptrBuf)*2;
				//bIsCopy = TRUE;
			//}
		}
	}
}

void /*MPanelItem* */ CreateResource(PEData *pData, DWORD rootType, LPVOID ptrRes, DWORD resSize,
                                     DWORD resourceType, DWORD resourceID/*LPCSTR asID, LPCSTR langID, DWORD stringIdBase, DWORD anLangId*/)
{
	_TRACE5("CreateResource(rootType=%x, ptrRes=R%08X, size=%u, resourceType=%i, resourceID=%i)",
	        rootType, ((LPBYTE)ptrRes - pData->pMappedFileBase), resSize, resourceType, resourceID);
#ifndef verc0_EXPORTS
	//char fileNameBuffer[MAX_PATH];
	//MPanelItem *pChild = NULL;
	//LPBYTE ptrBuf = NULL;
	//BOOL bIsCopy = FALSE;
	//TCHAR szType[32] = {0}, szInfo[64] = {0};
	//TCHAR szTextInfo[1024];
	//BOOL bDontSetData = FALSE;

	//if (asID && *asID) {
	//	strcpy(fileNameBuffer, asID);
	//} else {
	//	strcpy(fileNameBuffer, "????");
	//}
	//if (langID && *langID) {
	//	strcat(fileNameBuffer, "."); strcat(fileNameBuffer, langID);
	//}
#endif

	//Msg("CreateResource","0");

	// Numeric PE resource IDs for RT_ICON and RT_GROUP_ICON.
	constexpr DWORD iconResource = 3;
	constexpr DWORD groupIconResource = 14;
	switch(rootType)
	{
#ifndef verc0_EXPORTS
		case groupIconResource:
		case 0x8000 + groupIconResource:
		//case (WORD)RT_GROUP_CURSOR:
		//case 0x8000+(WORD)RT_GROUP_CURSOR:
		{
			//strcat(fileNameBuffer, ".txt");
			//pChild = pRoot->AddFile(fileNameBuffer, 0);
			if (!ValidateMemory(pData, ptrRes, std::max<size_t>(resSize, sizeof(GRPICONDIR))))
			{
				//pChild->SetErrorPtr(ptrRes, resSize);
			}
			else if (resSize < sizeof(GRPICONDIR))
			{
				//pChild->SetColumns(
				//	((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
				//	_T("!!! Invalid size of resource"));
				//pChild->printf(_T("\n!!! Invalid size of %s resource, expected %i bytes, but only %i bytes exists !!!"),
				//	((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
				//	sizeof(GRPICONDIR), resSize);
			}
			else
			{
				GRPICONDIR* pIcon = (GRPICONDIR*)ptrRes;
				int nSizeLeft = resSize - 6;
				int nResCount = pIcon->idCount;
				//StringCchPrintf(szInfo, countof(szInfo), _T("Count: %i"), nResCount);
				//pChild->SetColumns(
				//	((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
				//	szInfo);
				//StringCchPrintf(szTextInfo, countof(szTextInfo), _T("%s, Count: %u\n%s\n"),
				//	((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("GROUP_ICON") : _T("GROUP_CURSOR"),
				//	nResCount,
				//	((rootType & 0x7FFF)==(WORD)RT_GROUP_ICON) ? _T("====================") : _T("======================"));
				//pChild->AddText(szTextInfo, -1, TRUE); // Не добавлять в DUMP.TXT
				LPGRPICONDIRENTRY pEntry = pIcon->idEntries;

				while(nSizeLeft >= sizeof(GRPICONDIRENTRY))
				{
					if (!ValidateMemory(pData, pEntry, sizeof(GRPICONDIRENTRY)))
					{
						//pChild->SetErrorPtr(pEntry, sizeof(GRPICONDIRENTRY));
						break;
					}

					//StringCchPrintf(szTextInfo, countof(szTextInfo),
					//	_T("ID: %u,  BytesInRes: %i\n")
					//	_T("  Width: %i, Height: %i\n")
					//	_T("  ColorCount: %i, Planes: %i, BitCount: %i\n\n"),
					//	(UINT)pEntry->nID, (UINT)pEntry->dwBytesInRes,
					//	(UINT)pEntry->bWidth, (UINT)pEntry->bHeight,
					//	(UINT)pEntry->bColorCount, (UINT)pEntry->wPlanes, (UINT)pEntry->wBitCount);
					//
					//pChild->AddText(szTextInfo, -1, TRUE); // Не добавлять в DUMP.TXT
					nSizeLeft -= sizeof(GRPICONDIRENTRY);
					nResCount --;
					pEntry++;
				}
			}
			return; // pChild;
		} break;
	case iconResource: case 0x8000 + iconResource:
		{
			//_tcscpy(szType, _T("ICON"));
			if (!ValidateMemory(pData, ptrRes, resSize))
			{
				//pRoot->SetErrorPtr(ptrRes, resSize);
			}
			else if (resSize>4 && *((DWORD*)ptrRes) == 0x474e5089/* %PNG */) //-V112
			{
				//strcat(fileNameBuffer, ".png");
				//_tcscpy(szInfo, _T("PNG format"));
			}
			else if (resSize > sizeof(BITMAPINFOHEADER)
			        && ((BITMAPINFOHEADER*)ptrRes)->biSize == sizeof(BITMAPINFOHEADER)
			        && (((BITMAPINFOHEADER*)ptrRes)->biWidth && ((BITMAPINFOHEADER*)ptrRes)->biWidth < 256)
			        && (((BITMAPINFOHEADER*)ptrRes)->biHeight == (((BITMAPINFOHEADER*)ptrRes)->biWidth * 2)))
			{
				//StringCchPrintf(szInfo, countof(szInfo), _T("%ix%i (%ibpp)"),
				//	((BITMAPINFOHEADER*)ptrRes)->biWidth, ((BITMAPINFOHEADER*)ptrRes)->biWidth,
				//	((BITMAPINFOHEADER*)ptrRes)->biBitCount*((BITMAPINFOHEADER*)ptrRes)->biPlanes);

				//// Делаем копию буфера, но предваряем его заголовком иконки
				//DWORD nNewSize = resSize + sizeof(ICONDIR);
				//ptrBuf = (LPBYTE)malloc(nNewSize);
				//if (!ptrBuf) {
				//	//pRoot->printf(_T("\n!!! Can't allocate %i bytes !!!\n"), nNewSize);
				//} else {
				//	ICONDIR* pIcon = (ICONDIR*)ptrBuf;
				//	pIcon->idReserved = 0;
				//	pIcon->idType = 1;
				//	pIcon->idCount = 1;
				//	pIcon->idEntries[0].bWidth = (BYTE)((BITMAPINFOHEADER*)ptrRes)->biWidth;
				//	pIcon->idEntries[0].bHeight = (BYTE)((BITMAPINFOHEADER*)ptrRes)->biWidth;
				//	pIcon->idEntries[0].bColorCount = (((BITMAPINFOHEADER*)ptrRes)->biBitCount >= 8)
				//		? 0 : (1 << ((BITMAPINFOHEADER*)ptrRes)->biBitCount);
				//	pIcon->idEntries[0].bReserved = 0;
				//	pIcon->idEntries[0].wPlanes = ((BITMAPINFOHEADER*)ptrRes)->biPlanes;
				//	pIcon->idEntries[0].wBitCount = ((BITMAPINFOHEADER*)ptrRes)->biBitCount;
				//	pIcon->idEntries[0].dwBytesInRes = resSize;
				//	pIcon->idEntries[0].dwImageOffset = sizeof(ICONDIR);
				//	memmove(&(pIcon->idEntries[1]), ptrRes, resSize);

				//	ptrRes = ptrBuf;
				//	resSize = nNewSize;
				//	bIsCopy = TRUE;
				//	strcat(fileNameBuffer, ".ico");
				//}
			}
		} break;
#endif
		//case (WORD)RT_VERSION:
		case 0x10:
		{
			//Msg("CreateResource","1");
			if (resSize > sizeof(VS_FIXEDFILEINFO))
			{
				ParseVersionInfo(pData, ptrRes, resSize);
			}
		} break;
		//case 0x8000+(WORD)RT_VERSION:
		case 0x8010:
		{
			//Msg("CreateResource","2");
			if (resSize > sizeof(VS_FIXEDFILEINFO))
			{
				ParseVersionInfoA(pData, ptrRes, resSize);
			}
		} break;
	}

	//Msg("CreateResource","3");

	return; // pChild;
}


//
// Dump the information about one resource directory entry.  If the
// entry is for a subdirectory, call the directory dumping routine
// instead of printing information in this routine.
//
void DumpResourceEntry(
    PEData *pData, DWORD resourceType /*LPCSTR asID*/,
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry,
    PBYTE pResourceBase,
    DWORD level, DWORD rootType, DWORD parentType)
{
	_TRACE0("DumpResourceEntry");
	//char nameBuffer[128];
	PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry;

	//Msg("DumpResourceEntry","1");

	if (!ValidateMemory(pData, pResDirEntry, sizeof(*pResDirEntry)))
	{
		_TRACE("Invalid memory pointer (pResDirEntry) in DumpResourceEntry");
		return;
	}

	if (pResDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY)
	{
		DumpResourceDirectory(pData, (PIMAGE_RESOURCE_DIRECTORY)
		                      ((pResDirEntry->OffsetToData & 0x7FFFFFFF) + pResourceBase), //-V112
		                      pResourceBase, level, pResDirEntry->Name, rootType);
		return;
	}

	//Msg("DumpResourceEntry","2");
	pResDataEntry = MakePtr(PIMAGE_RESOURCE_DATA_ENTRY, pResourceBase, pResDirEntry->OffsetToData);

	if (!ValidateMemory(pData, pResDataEntry, sizeof(*pResDataEntry)))
	{
		//Msg("DumpResourceEntry", "Invalid PTR");
		return;
	}

	LPVOID ptrRes = NULL;

	if (pData->bIs64Bit)
	{
		//Msg("DumpResourceEntry","2.1");
		//char szDbg[128];
		//wsprintfA(szDbg, "GetPtrFromRVA(0x%08X)", (DWORD)pResDataEntry); Msg("DumpResourceEntry", szDbg);
		//if (!ValidateMemory(pData, pResDataEntry, sizeof(*pResDataEntry)))
		//	Msg("DumpResourceEntry", "Invalid PTR");

		ptrRes = GetPtrFromRVA(pResDataEntry->OffsetToData, pData->pNTHeader64, pData->pMappedFileBase);
		//Msg("DumpResourceEntry","2.2");
	}
	else
	{
		//Msg("DumpResourceEntry","2.3");
		ptrRes = GetPtrFromRVA(pResDataEntry->OffsetToData, pData->pNTHeader32, pData->pMappedFileBase);
		//Msg("DumpResourceEntry","2.4");
	}

	//  if ( pResDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING )
	//  {
	//      GetResourceNameFromId(pResDirEntry->Name, pResourceBase, nameBuffer,
	//                            sizeof(nameBuffer));
	//  }
	//  else
	//  {
	//wsprintfA(nameBuffer, "0x%04X", pResDirEntry->Name);
	//  }
	//Msg("DumpResourceEntry","CreateResource");
	CreateResource(pData, rootType, ptrRes, pResDataEntry->Size, resourceType/*asID*/, pResDirEntry->Name /*nameBuffer, parentType, pResDirEntry->Name*/);

	//Msg("DumpResourceEntry","3");

	//// Spit out the spacing for the level indentation
	//for ( i=0; i < level; i++ ) pRoot->printf("    ");
	//pRoot->printf("Name: %s  DataEntryOffs: %08X\n",
	//	nameBuffer, pResDirEntry->OffsetToData);
	//
	//
	//// the resDirEntry->OffsetToData is a pointer to an
	//// IMAGE_RESOURCE_DATA_ENTRY.  Go dump out that information.  First,
	//// spit out the proper indentation
	//for ( i=0; i < level; i++ ) pRoot->printf("    ");
	//
	//pRoot->printf("DataRVA: %05X  DataSize: %05X  CodePage: %X\n",
	//        pResDataEntry->OffsetToData, pResDataEntry->Size,
	//        pResDataEntry->CodePage);
}


//
// Dump the information about one resource directory.
//
void DumpResourceDirectory(PEData *pData, PIMAGE_RESOURCE_DIRECTORY pResDir,
                           PBYTE pResourceBase,
                           DWORD level,
                           DWORD resourceType, DWORD rootType /*= 0*/, DWORD parentType /*= 0*/)
{
	_TRACE4("DumpResourceDirectory(pResDir=r%08X, pResourceBase=r%08X, level=%i, resourceType=%i",
	        ((LPBYTE)pResDir)-pData->pMappedFileBase, pResourceBase-pData->pMappedFileBase,
	        level, resourceType);

	if (!ValidateMemory(pData, pResDir, sizeof(*pResDir)))
	{
		_TRACE("Invalid memory pointer (pResDir) in DumpResourceDirectory");
		return;
	}

	PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry;
	//char szType[64];
	UINT i;

	// Условие останова, когда нам больше ничего не интересно
	if ((pData->nFlags & (PE_VER_EXISTS|PE_ICON_EXISTS)) == (PE_VER_EXISTS|PE_ICON_EXISTS))
		return;

	// Level 1 resources are the resource types
	if (level == 1)
	{
		rootType = resourceType;

		//if ( resourceType & IMAGE_RESOURCE_NAME_IS_STRING )
		//{
		//	GetResourceNameFromId( resourceType, pResourceBase,
		//							szType, sizeof(szType) );
		//}
		//else
		//{
		//       GetResourceTypeName( resourceType, szType, sizeof(szType) );
		//}
	}

	//else    // All other levels, just print out the regular id or name
	//{
	//    GetResourceNameFromId( resourceType, pResourceBase, szType,
	//                           sizeof(szType) );
	//}
	//if (level == 1) {
	//	// заложимся на то, что в 16бит PE в типах ресурсов установлен бит 0x8000
	//	if ((resourceType & 0x7FFF) == (WORD)RT_GROUP_CURSOR)
	//		pChild = pRoot->AddFolder(_T("CURSOR"), FALSE);
	//	else if ((resourceType & 0x7FFF) == (WORD)RT_GROUP_ICON)
	//		pChild = pRoot->AddFolder(_T("ICON"), FALSE);
	//	else
	//		pChild = pRoot->AddFolder(szType, FALSE);
	//}


	//
	// The "directory entries" immediately follow the directory in memory
	//
	resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pResDir+1);

	for(i=0; i < pResDir->NumberOfNamedEntries && !pData->bValidateFailed; i++, resDirEntry++)
	{
		DumpResourceEntry(pData, resourceType/*szType*/, resDirEntry, pResourceBase, level+1, rootType, resourceType);

		// Условие останова, когда нам больше ничего не интересно
		if ((pData->nFlags & (PE_VER_EXISTS|PE_ICON_EXISTS)) == (PE_VER_EXISTS|PE_ICON_EXISTS))
			return;
	}

	for(i=0; i < pResDir->NumberOfIdEntries && !pData->bValidateFailed; i++, resDirEntry++)
	{
		DumpResourceEntry(pData, resourceType/*szType*/, resDirEntry, pResourceBase, level+1, rootType, resourceType);

		// Условие останова, когда нам больше ничего не интересно
		if ((pData->nFlags & (PE_VER_EXISTS|PE_ICON_EXISTS)) == (PE_VER_EXISTS|PE_ICON_EXISTS))
			return;
	}
}


//
// Top level routine called to dump out the entire resource hierarchy
//
template <class T> void DumpResourceSection(PEData *pData, PBYTE pImageBase, T * pNTHeader)	// 'T' = PIMAGE_NT_HEADERS 32/64
{
	_TRACE0("DumpResourceSection");
	DWORD resourcesRVA;
	PBYTE pResDir;

	if (pData->bValidateFailed)
		return;

	//bool bIs64Bit = ( pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC );

	resourcesRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_RESOURCE);
	_TRACE1("DumpResourceSection. resourcesRVA = 0x%08X", resourcesRVA);

	if (!resourcesRVA)
		return;

	pResDir = (PBYTE)GetPtrFromRVA(resourcesRVA, pNTHeader, pImageBase);
	_TRACE_ASSERT(pImageBase==pData->pMappedFileBase,"pImageBase!=pData->pMappedFileBase");
	_TRACE1("DumpResourceSection. pResDir = r%08X", pResDir - pData->pMappedFileBase);

	if (!pResDir)
		return;

	DumpResourceDirectory(pData, (PIMAGE_RESOURCE_DIRECTORY)pResDir, pResDir, 0, 0);
	//if ( !fShowResources )
	//	return;
}

void DumpResources(PEData *pData, PBYTE pImageBase, PIMAGE_NT_HEADERS32 pNTHeader)
{
	if (!ValidateMemory(pData, pNTHeader, sizeof(*pNTHeader)))
	{
		_TRACE("Invalid memory pointer in DumpResources");
		return;
	}

	if (pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		_TRACE("DumpResources - x64");
		_TRACE_ASSERT(sizeof(IMAGE_NT_HEADERS64)==264, "sizeof(IMAGE_NT_HEADERS64)!=264");
		// sizeof(IMAGE_NT_HEADERS64)=264
		DumpResourceSection(pData, pImageBase, (PIMAGE_NT_HEADERS64)pNTHeader);
	}
	else
	{
		_TRACE("DumpResources - x32");
		_TRACE_ASSERT(sizeof(IMAGE_NT_HEADERS32)==248, "sizeof(IMAGE_NT_HEADERS32)!=248");
		// sizeof(IMAGE_NT_HEADERS32)=248
		DumpResourceSection(pData, pImageBase, (PIMAGE_NT_HEADERS32)pNTHeader);
	}
}

template <class T> void DumpCOR20Header(PEData *pData, PBYTE pImageBase, T* pNTHeader)	// T = PIMAGE_NT_HEADERS
{
	DWORD cor20HdrRVA;   // COR20_HEADER RVA
	cor20HdrRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR);

	if (!cor20HdrRVA)
		return;

	pData->nFlags |= PE_DOTNET;

	//PIMAGE_COR20_HEADER pCor20Hdr = (PIMAGE_COR20_HEADER)GetPtrFromRVA( cor20HdrRVA, pNTHeader, pImageBase );

	//pRoot->Root()->AddFlags(_T("NET"));

	//MPanelItem* pChild = pRoot->AddFolder(_T(".NET"));

	//pChild->printf( "<.NET Runtime Header>:\n" );

	//pChild->printf( "  Size:       %u\n", pCor20Hdr->cb );
	//pChild->printf( "  Version:    %u.%u\n", pCor20Hdr->MajorRuntimeVersion, pCor20Hdr->MinorRuntimeVersion );
	//pChild->printf( "  Flags:      %X\n", pCor20Hdr->Flags );
	//if ( pCor20Hdr->Flags & COMIMAGE_FLAGS_ILONLY ) pChild->printf( "    ILONLY\n" );
	//if ( pCor20Hdr->Flags & COMIMAGE_FLAGS_32BITREQUIRED ) pChild->printf( "    32BITREQUIRED\n" );
	//if ( pCor20Hdr->Flags & COMIMAGE_FLAGS_IL_LIBRARY ) pChild->printf( "    IL_LIBRARY\n" );
	//if ( pCor20Hdr->Flags & 8 ) pChild->printf( "    STRONGNAMESIGNED\n" );		// At this moment, WINNT.H and CorHdr.H are out of sync...
	//if ( pCor20Hdr->Flags & COMIMAGE_FLAGS_TRACKDEBUGDATA ) pChild->printf( "    TRACKDEBUGDATA\n" );

	//DisplayDataDirectoryEntry( pChild, "MetaData", pCor20Hdr->MetaData );
	//DisplayDataDirectoryEntry( pChild, "Resources", pCor20Hdr->Resources );
	//DisplayDataDirectoryEntry( pChild, "StrongNameSig", pCor20Hdr->StrongNameSignature );
	//DisplayDataDirectoryEntry( pChild, "CodeManagerTable", pCor20Hdr->CodeManagerTable );
	//DisplayDataDirectoryEntry( pChild, "VTableFixups", pCor20Hdr->VTableFixups );
	//DisplayDataDirectoryEntry( pChild, "ExprtAddrTblJmps", pCor20Hdr->ExportAddressTableJumps );
	//DisplayDataDirectoryEntry( pChild, "ManagedNativeHdr", pCor20Hdr->ManagedNativeHeader );

	//pRoot->printf( "\n" );
}

template <class T> void DumpCertificates(PEData *pData, PBYTE pImageBase, T* pNTHeader)
{
	// Note that the this DataDirectory entry gives a >>> FILE OFFSET <<< rather than
	// an RVA.
	DWORD certOffset;
	certOffset = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_SECURITY);

	if (!certOffset || ((certOffset+sizeof(WIN_CERTIFICATE)) > pData->FileFullSize.QuadPart))
		return;

	//__int64 dwTotalSize = GetImgDirEntrySize( pNTHeader, IMAGE_DIRECTORY_ENTRY_SECURITY );

	// Скорее всего сертификаты находятся ЗА пределами отображенного в память файла, т.к. они не входят в ImageSize
	// Поэтому для простоты - считаем что он подписан, без проверки типа подписи
	pData->nFlags |= PE_SIGNED;


	//LPWIN_CERTIFICATE pCert = MakePtr( LPWIN_CERTIFICATE, pImageBase, certOffset );
	//// Скорее всего сертификаты находятся ЗА пределами отображенного в память файла,
	//// т.к. они не входят в ImageSize

	//while ( dwTotalSize > 0 )	// As long as there is unprocessed certificate data...
	//{
	//	//LPWIN_CERTIFICATE pCert = MakePtr( LPWIN_CERTIFICATE, pImageBase, certOffset );

	//	//if (!pCert || IsBadReadPtr(pCert, sizeof(*pCert))) {
	//	if (!pCert || !ValidateMemory(pData, pCert, sizeof(*pCert)))
	//	{
	//		//pChild->printf(_T("\n!!! Failed to read LPWIN_CERTIFICATE at offset: 0x%08X !!!\n"), certOffset);
	//		break;
	//	}

	//	if (!pCert->dwLength) {
	//		break; // кончились
	//	}

	//	size_t nAllLen = pCert->dwLength;

	//	switch( pCert->wCertificateType )
	//	{
	//	case WIN_CERT_TYPE_X509: //lstrcpy(szCertType, _T("X509")); break;
	//	case WIN_CERT_TYPE_PKCS_SIGNED_DATA: //lstrcpy(szCertType, _T("PKCS_SIGNED_DATA")); break;
	//	case WIN_CERT_TYPE_TS_STACK_SIGNED: //lstrcpy(szCertType, _T("TS_STACK_SIGNED")); break;
	//		// OK, поддерживаемый тип
	//		break;
	//	default: //StringCchPrintf(szCertType, countof(szCertType), _T("0x%04X"), pCert->wCertificateType);
	//		continue;
	//	}
	//
	//	//nCertNo++;

	//	//if (IsBadReadPtr(pCert, nAllLen)) {
	//	if (!ValidateMemory(pData, pCert, nAllLen))
	//	{
	//		break;
	//	}
	//
	//	pData->nFlags |= PE_SIGNED;
	//	return;
	//}
}


//
// top level routine called from PEDUMP.CPP to dump the components of a PE file
//
bool DumpExeFile(PEData *pData, PIMAGE_DOS_HEADER dosHeader)
{
	_TRACE("DumpExeFile");
	PIMAGE_NT_HEADERS32 pNTHeader;
	//PBYTE pImageBase = (PBYTE)dosHeader;
	// Make pointers to 32 and 64 bit versions of the header.
	pNTHeader = MakePtr(PIMAGE_NT_HEADERS32, dosHeader,
	                    dosHeader->e_lfanew);
	DWORD nSignature = 0;

	// First, verify that the e_lfanew field gave us a reasonable
	// pointer, then verify the PE signature.
	//if ( !IsBadReadPtr( pNTHeader, sizeof(pNTHeader->Signature) ) )
	if (ValidateMemory(pData, pNTHeader, sizeof(pNTHeader->Signature)))
	{
		nSignature = pNTHeader->Signature;

		if (nSignature == IMAGE_NT_SIGNATURE)
		{
			return DumpExeFilePE(pData, dosHeader, pNTHeader);
		}
		else if ((nSignature & 0xFFFF) == IMAGE_OS2_SIGNATURE)
		{
			return DumpExeFileNE(pData, dosHeader, (IMAGE_OS2_HEADER*)pNTHeader);
		}
		else if ((nSignature & 0xFFFF) == IMAGE_OS2_SIGNATURE_LE)
		{
			return DumpExeFileNE(pData, dosHeader, (IMAGE_OS2_HEADER*)pNTHeader);
		}
		//else if ( (nSignature & 0xFFFF) == IMAGE_VXD_SIGNATURE )
		//{
		//	return DumpExeFileVX( pData, dosHeader, (IMAGE_VXD_HEADER*)pNTHeader );
		//}
	}

	return false;
}

//bool DumpExeFileVX( PEData *pData, PIMAGE_DOS_HEADER dosHeader, PIMAGE_VXD_HEADER pVXDHeader )
//{
//	//PBYTE pImageBase = (PBYTE)dosHeader;
//	//
//	//MPanelItem* pChild = pRoot->AddFolder(_T("VxD Header"));
//	//pChild->AddText(_T("<VxD Header>\n"));
//	//
//	//pChild->AddText(_T("  Signature:         IMAGE_VXD_SIGNATURE\n"));
//	//
//	//MPanelItem* pDos = pRoot->AddFile(_T("DOS_Header"), sizeof(*dosHeader));
//	//pDos->SetData((const BYTE*)dosHeader, sizeof(*dosHeader));
//	//
//	//MPanelItem* pVXD = pRoot->AddFile(_T("VxD_Header"), sizeof(*pVXDHeader));
//	//pVXD->SetData((const BYTE*)pVXDHeader, sizeof(*pVXDHeader));
//
//	return false;
//}

//
// Dump the section table from a PE file or an OBJ
//
void DumpSectionTable(PEData *pData, PIMAGE_SECTION_HEADER section,
                      unsigned cSections,
                      BOOL IsEXE)
{
	_TRACE("DumpSectionTable");
	//MPanelItem *pChild = pRoot->AddFolder(_T("Section Table"));

	//pChild->AddText(_T("<Section Table>\n"));

	//pChild->SetColumnsTitles(cszCharacteristics, 7, cszRawVirtSize, 21);

	for (unsigned i=1; i <= cSections; i++, section++)
	{
		//MAX section name length is 8, but may be not zero terminated
		//char cSectName[9];
		//lstrcpynA(cSectName, (char*)section->Name, 9);
		//MPanelItem *pSect = pChild->AddFile(cSectName, section->Misc.VirtualSize);
		if (!ValidateMemory(pData, section, sizeof(*section)))
		{
			return;
		}

		if (section->Name[0] == 'U' && section->Name[1] == 'P' && section->Name[2] == 'X')
		{
			pData->nFlags |= PE_UPX;
			break; // пока больше ничего не интересует
			//g_bUPXed = true;
			//pRoot->Root()->AddFlags(_T("UPX"));
		}

		//// Как то странно смотрится: IsEXE ? "VirtSize" : "PhysAddr",
		//// но так было в оригинале PEDUMP
		//pSect->printf( "  %02X %-8.8s  %s: %08X  VirtAddr:  %08X\n",
		//	i, section->Name,
		//	IsEXE ? "VirtSize" : "PhysAddr",
		//	section->Misc.VirtualSize, section->VirtualAddress);
		//pSect->printf( "    raw data offs:   %08X  raw data size: %08X\n",
		//	section->PointerToRawData, section->SizeOfRawData );
		//pSect->printf( "    relocation offs: %08X  relocations:   %08X\n",
		//	section->PointerToRelocations, section->NumberOfRelocations );
		//pSect->printf( "    line # offs:     %08X  line #'s:      %08X\n",
		//	section->PointerToLinenumbers, section->NumberOfLinenumbers );
		//pSect->printf( "    characteristics: %08X\n", section->Characteristics);

		//pSect->printf("    ");
		//TCHAR sChars[32]; TCHAR *pszChars = sChars; *pszChars = 0; TCHAR chCurAbbr = 0;
		//for ( unsigned j=0; j < NUMBER_SECTION_CHARACTERISTICS; j++ )
		//{
		//	chCurAbbr = 0;
		//	if ( section->Characteristics &
		//		SectionCharacteristics[j].flag )
		//	{
		//		pSect->printf( "  %s", SectionCharacteristics[j].name );
		//		if (SectionCharacteristics[j].abbr)
		//			chCurAbbr = SectionCharacteristics[j].abbr;
		//	} else if (SectionCharacteristics[j].abbr)
		//		chCurAbbr = _T(' ');
		//	if (chCurAbbr) {
		//		*(pszChars++) = chCurAbbr;
		//		*pszChars = 0;
		//	}
		//}
		//TCHAR sRawVirtSize[64];
		//StringCchPrintf(sRawVirtSize, countof(sRawVirtSize), _T("0x%08X/0x%08X"), section->SizeOfRawData, section->Misc.VirtualSize);
		//pSect->SetColumns(sChars, sRawVirtSize);

		//unsigned alignment = (section->Characteristics & IMAGE_SCN_ALIGN_MASK);
		//if ( alignment == 0 )
		//{
		//	pSect->printf( "  ALIGN_DEFAULT(16)" );
		//}
		//else
		//{
		//	// Yeah, it's hard to read this, but it works, and it's elegant
		//	alignment = alignment >>= 20;
		//	pSect->printf( "  ALIGN_%uBYTES", 1 << (alignment-1) );
		//}

		//pSect->printf("\n\n");

		//if (gpNTHeader32 || gpNTHeader64) {
		//	LPVOID ptrSect = NULL;
		//	if (g_bIs64Bit)
		//		ptrSect = GetPtrFromRVA(section->VirtualAddress, gpNTHeader64, g_pMappedFileBase);
		//	else
		//		ptrSect = GetPtrFromRVA(section->VirtualAddress, gpNTHeader32, g_pMappedFileBase);
		//	// section->Misc.VirtualSize - If this value is greater than the SizeOfRawData member, the section is filled with zeroes
		//	if (ptrSect)
		//		pSect->SetData((const BYTE*)ptrSect, section->SizeOfRawData);
		//} else {
		//	// Dumping *.obj file?
		//	if (section->PointerToRawData && section->SizeOfRawData) {
		//		LPVOID ptrSect = g_pMappedFileBase+section->PointerToRawData;
		//		pSect->SetData((const BYTE*)ptrSect, section->SizeOfRawData);
		//	}
		//}
	}
}

//
// Dump the exports table (usually the .edata section) of a PE file
//
template <class T> void DumpExportsSection(PEData *pData, PBYTE pImageBase, T * pNTHeader)	// 'T' = PIMAGE_NT_HEADERS
{
	_TRACE("DumpExportsSection");
	// Пока - нас интересуют только экспорты dll-ек (модули фара)
	if (lstrcmpi(pData->szExtension, L".dll"))
		return;

	PIMAGE_EXPORT_DIRECTORY pExportDir;
	PIMAGE_SECTION_HEADER header;
	INT delta;
	//PSTR pszFilename;
	DWORD i;
	PDWORD pdwFunctions;
	PWORD pwOrdinals;
	DWORD *pszFuncNames;
	DWORD exportsStartRVA, exportsEndRVA;

	exportsStartRVA = GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_EXPORT);
	exportsEndRVA = exportsStartRVA +
	                GetImgDirEntrySize(pNTHeader, IMAGE_DIRECTORY_ENTRY_EXPORT);
	// Get the IMAGE_SECTION_HEADER that contains the exports.  This is
	// usually the .edata section, but doesn't have to be.
	header = GetEnclosingSectionHeader(exportsStartRVA, pNTHeader);

	if (!header)
		return;

	if (!ValidateMemory(pData, header, sizeof(*header)))
	{
		return;
	}

	delta = (INT)(header->VirtualAddress - header->PointerToRawData);

	pExportDir = (PIMAGE_EXPORT_DIRECTORY)GetPtrFromRVA(exportsStartRVA, pNTHeader, pImageBase);

	if (!ValidateMemory(pData, pExportDir, sizeof(*pExportDir)))
	{
		return;
	}

	//pszFilename = (PSTR)GetPtrFromRVA( pExportDir->Name, pNTHeader, pImageBase );

	//MPanelItem *pChild = pRoot->AddFolder(_T("Exports Table"));
	//pChild->SetColumnsTitles(cszOrdinal,4,cszEntryPoint,8);
	//pChild->printf("<Exports Table>:\n");
	//pChild->printf("  Name:            %s\n", pszFilename);
	//pChild->printf("  Characteristics: %08X\n", pExportDir->Characteristics);

	//__time32_t timeStamp = pExportDir->TimeDateStamp;
	//pChild->printf("  TimeDateStamp:   %08X -> %s",
	//	pExportDir->TimeDateStamp, _ctime32(&timeStamp) );
	//pChild->printf("  Version:         %u.%02u\n", pExportDir->MajorVersion,
	//	pExportDir->MinorVersion);
	//pChild->printf("  Ordinal base:    %08X\n", pExportDir->Base);
	//pChild->printf("  # of functions:  %08X\n", pExportDir->NumberOfFunctions);
	//pChild->printf("  # of Names:      %08X\n", pExportDir->NumberOfNames);
	pdwFunctions =	(PDWORD)GetPtrFromRVA(pExportDir->AddressOfFunctions, pNTHeader, pImageBase);
	pwOrdinals =	(PWORD)	GetPtrFromRVA(pExportDir->AddressOfNameOrdinals, pNTHeader, pImageBase);
	pszFuncNames =	(DWORD *)GetPtrFromRVA(pExportDir->AddressOfNames, pNTHeader, pImageBase);
	LPCSTR pszFuncName = NULL;
	//char szNameBuffer[MAX_PATH+1]; //, szEntryPoint[32], szOrdinal[16];
	bool lbOpenPluginW = false;

	//pChild->printf("\n  Entry Pt  Ordn  Name\n");
	if (pdwFunctions)
		for (i=0;
		        i < pExportDir->NumberOfFunctions;
		        i++, pdwFunctions++)
		{
			DWORD entryPointRVA = *pdwFunctions;

			if (entryPointRVA == 0)     // Skip over gaps in exported function
				continue;               // ordinals (the entrypoint is 0 for
										// these functions).

			////pRoot->printf("  %08X  %4u", entryPointRVA, i + pExportDir->Base );
			//sprintf(szEntryPoint, "%08X", entryPointRVA);
			//sprintf(szOrdinal, "%4u", i + pExportDir->Base);

			// See if this function has an associated name exported for it.
			pszFuncName = NULL;

			if (pwOrdinals && pszFuncNames)
			{
				for(unsigned j=0; j < pExportDir->NumberOfNames; j++)
				{
					if (pwOrdinals[j] == i)
					{
						pszFuncName = (LPCSTR)GetPtrFromRVA(pszFuncNames[j], pNTHeader, pImageBase);

						if (!ValidateMemory(pData, pszFuncName, 16))
						{
							return;
						}

						//pRoot->printf("  %s", GetPtrFromRVA(pszFuncNames[j], pNTHeader, pImageBase) );
						if (pszFuncName && *pszFuncName == 'S')
						{
							if (!lstrcmpA(pszFuncName, "SetStartupInfo"))
							{
								//pRoot->AddFlags(_T("FAR1"));
								pData->nFlags |= PE_Far1;
							}
							else if (!lstrcmpA(pszFuncName, "SetStartupInfoW"))
							{
								//pRoot->AddFlags(_T("FAR2"));
								pData->nFlags |= PE_Far2;
							}
							else if (!lstrcmpA(pszFuncName, "GetGlobalInfoW"))
							{
								//pRoot->AddFlags(_T("FAR3"));
								pData->nFlags |= PE_Far3;
							}
							else if (!lstrcmpA(pszFuncName, "OpenPluginW") || !lstrcmpA(pszFuncName, "OpenFilePluginW"))
							{
								lbOpenPluginW = true;
							}
							//if ((pData->nFlags & (PE_Far1|PE_Far2)) == (PE_Far1|PE_Far2))
							//	return;
						}
					}
				}
			}
			//if (!pszFuncName) {
			//	sprintf(szNameBuffer, "(Ordinal@%u)", i + pExportDir->Base);
			//	pszFuncName = szNameBuffer;
			//}

			//MPanelItem* pFunc = pChild->AddFile(pszFuncName);
			//pFunc->printf("  %s  %s  %s", szEntryPoint, szOrdinal, pszFuncName);
			//pFunc->SetColumns(szOrdinal, szEntryPoint);

			//// Is it a forwarder?  If so, the entry point RVA is inside the
			//// .edata section, and is an RVA to the DllName.EntryPointName
			//if ( (entryPointRVA >= exportsStartRVA)
			//	&& (entryPointRVA <= exportsEndRVA) )
			//{
			//	pFunc->printf(" (forwarder -> %s)", GetPtrFromRVA(entryPointRVA, pNTHeader, pImageBase) );
			//}

			//pFunc->AddText(_T("\n"));
		}

	if ((pData->nFlags & PE_Far2) && (pData->nFlags & PE_Far3) && !lbOpenPluginW)
		pData->nFlags &= ~PE_Far2;

	//pChild->printf( "\n" );
}


bool DumpExeFilePE(PEData *pData, PIMAGE_DOS_HEADER dosHeader, PIMAGE_NT_HEADERS32 pNTHeader)
{
	_TRACE("DumpExeFilePE");
	PBYTE pImageBase = (PBYTE)dosHeader;
	PIMAGE_NT_HEADERS64 pNTHeader64;

	pNTHeader64 = (PIMAGE_NT_HEADERS64)pNTHeader;

	// Тут пока не важно 64/32 - положение одинаковое
	pData->Machine = pNTHeader->FileHeader.Machine;
	bool bIs64Bit = (pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);
	pData->bIs64Bit = bIs64Bit;
	pData->nBits = (bIs64Bit) ? 64 : 32;

	if (bIs64Bit)
	{
		pData->pNTHeader64 = pNTHeader64;
	}
	else
	{
		pData->pNTHeader32 = (PIMAGE_NT_HEADERS32)pNTHeader;
	}

	//pRoot->printf("\n");
	//
	//// IsExe = TRUE, means "NOT *.obj file"
	//DumpSectionTable( pRoot, IMAGE_FIRST_SECTION(pNTHeader),
	//                    pNTHeader->FileHeader.NumberOfSections, TRUE);
	//pRoot->printf("\n");
	//
	//if ( bIs64Bit )
	//	DumpExeDebugDirectory( pRoot, pImageBase, pNTHeader64 );
	//else
	//	DumpExeDebugDirectory( pRoot, pImageBase, pNTHeader );
	//
	//if ( pNTHeader->FileHeader.PointerToSymbolTable == 0 )
	//    g_pCOFFHeader = 0; // Doesn't really exist!
	//pRoot->printf("\n");
	DumpResources(pData, pImageBase, pNTHeader);

	//bIs64Bit
	//	? DumpTLSDirectory( pRoot, pImageBase, pNTHeader64, (PIMAGE_TLS_DIRECTORY64)0 )	// Passing NULL ptr is a clever hack
	//	: DumpTLSDirectory( pRoot, pImageBase, pNTHeader, (PIMAGE_TLS_DIRECTORY32)0 );		// See if you can figure it out! :-)
	//
	//bIs64Bit
	//    ? DumpImportsSection(pRoot, pImageBase, pNTHeader64 )
	//	: DumpImportsSection(pRoot, pImageBase, pNTHeader);
	//
	//bIs64Bit
	//	? DumpDelayedImportsSection(pRoot, pImageBase, pNTHeader64, bIs64Bit )
	//	: DumpDelayedImportsSection(pRoot, pImageBase, pNTHeader, bIs64Bit );
	//
	//bIs64Bit
	//	? DumpBoundImportDescriptors( pRoot, pImageBase, pNTHeader64 )
	//	: DumpBoundImportDescriptors( pRoot, pImageBase, pNTHeader );

#ifndef verc0_EXPORTS
	bIs64Bit
	? DumpExportsSection(pData, pImageBase, pNTHeader64)
	: DumpExportsSection(pData, pImageBase, pNTHeader);
#endif

	bIs64Bit
	? DumpCOR20Header(pData, pImageBase, pNTHeader64)
	: DumpCOR20Header(pData, pImageBase, pNTHeader);

	//bIs64Bit
	//	? DumpLoadConfigDirectory( pRoot, pImageBase, pNTHeader64, (PIMAGE_LOAD_CONFIG_DIRECTORY64)0 )	// Passing NULL ptr is a clever hack
	//	: DumpLoadConfigDirectory( pRoot, pImageBase, pNTHeader, (PIMAGE_LOAD_CONFIG_DIRECTORY32)0 );	// See if you can figure it out! :-)

	bIs64Bit
	? DumpCertificates(pData, pImageBase, pNTHeader64)
	: DumpCertificates(pData, pImageBase, pNTHeader);
	////=========================================================================
	////
	//// If we have COFF symbols, create a symbol table now
	////
	////=========================================================================
	//
	//if ( g_pCOFFHeader )	// Did we see a COFF symbols header while looking
	//{						// through the debug directory?
	//	g_pCOFFSymbolTable = new COFFSymbolTable(
	//			pImageBase+ pNTHeader->FileHeader.PointerToSymbolTable,
	//			pNTHeader->FileHeader.NumberOfSymbols );
	//}
	//
	//if ( fShowPDATA )
	//{
	//	bIs64Bit
	//		? DumpRuntimeFunctions( pRoot, pImageBase, pNTHeader64 )
	//		: DumpRuntimeFunctions( pRoot, pImageBase, pNTHeader );
	//
	//	pRoot->printf( "\n" );
	//}
	//
	//if ( fShowRelocations )
	//{
	//    bIs64Bit
	//		? DumpBaseRelocationsSection( pRoot, pImageBase, pNTHeader64 )
	//		: DumpBaseRelocationsSection( pRoot, pImageBase, pNTHeader );
	//    pRoot->printf("\n");
	//}
	//
	//if ( fShowSymbolTable && g_pMiscDebugInfo )
	//{
	//	DumpMiscDebugInfo( pRoot, g_pMiscDebugInfo );
	//	pRoot->printf( "\n" );
	//}
	//
	//if ( fShowSymbolTable && g_pCVHeader )
	//{
	//	DumpCVDebugInfoRecord( pRoot, g_pCVHeader );
	//	pRoot->printf( "\n" );
	//}
	//
	//if ( fShowSymbolTable && g_pCOFFHeader )
	//{
	//    DumpCOFFHeader( pRoot, g_pCOFFHeader );
	//    pRoot->printf("\n");
	//}
	//
	//if ( fShowLineNumbers && g_pCOFFHeader )
	//{
	//    DumpLineNumbers( pRoot, MakePtr(PIMAGE_LINENUMBER, g_pCOFFHeader,
	//                        g_pCOFFHeader->LvaToFirstLinenumber),
	//                        g_pCOFFHeader->NumberOfLinenumbers);
	//    pRoot->printf("\n");
	//}
	//
	//if ( fShowSymbolTable )
	//{
	//    if ( pNTHeader->FileHeader.NumberOfSymbols
	//        && pNTHeader->FileHeader.PointerToSymbolTable
	//		&& g_pCOFFSymbolTable )
	//    {
	//        DumpCOFFSymbolTable( pRoot, g_pCOFFSymbolTable );
	//        pRoot->printf("\n");
	//    }
	//}
	//
	//// 04.03.2010 Maks - В Exe не инетересно видеть HexDump, да это еще и долго
	////if ( fShowRawSectionData )
	////{
	////	PIMAGE_SECTION_HEADER pSectionHdr;
	////
	////	pSectionHdr = bIs64Bit ? (PIMAGE_SECTION_HEADER)(pNTHeader64+1) : (PIMAGE_SECTION_HEADER)(pNTHeader+1);
	////
	////    DumpRawSectionData( pRoot, pSectionHdr, dosHeader, pNTHeader->FileHeader.NumberOfSections);
	////}
	//
	//if ( g_pCOFFSymbolTable )
	//	delete g_pCOFFSymbolTable;

	return true;
}


bool DumpFile(PEData *pData, LPBYTE pFileData, __int64 nFileSize/*mapped*/, __int64 nFullFileSize)
{
	bool lbSucceeded = false;
	PIMAGE_DOS_HEADER dosHeader;
	pData->pNTHeader32 = NULL;
	pData->pNTHeader64 = NULL;
	pData->bIs64Bit = false;

	//g_pCVHeader = 0;
	//g_pCOFFHeader = 0;
	//g_pCOFFSymbolTable = 0;


	pData->FileSize.QuadPart = nFileSize;
	pData->FileFullSize.QuadPart = nFullFileSize;
	pData->pMappedFileBase = pFileData;
	dosHeader = (PIMAGE_DOS_HEADER)pData->pMappedFileBase;

	if (dosHeader->e_magic == IMAGE_DOS_SIGNATURE)
	{
		lbSucceeded = DumpExeFile(pData, dosHeader);
	}

	return lbSucceeded;
}



#ifndef verc0_EXPORTS

extern "C" BOOL WINAPI CET_Init(struct CET_Init* pInit)
{
	HeapInitialize();
	_ASSERTE(pInit->cbSize >= sizeof(struct CET_Init));

	if (pInit->cbSize < sizeof(struct CET_Init))
	{
		pInit->nErrNumber = PEE_OLD_PLUGIN;
		return FALSE;
	}

	ghModule = pInit->hModule;

	pInit->pContext = (LPVOID)1;
	return TRUE;
}

extern "C" VOID WINAPI CET_Done(struct CET_Init* pInit)
{
	HeapDeinitialize();
}


#define SETERROR(n) if (pLoadPreview) pLoadPreview->nErrNumber = n;


extern "C" BOOL WINAPI CET_Load(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo))
	{
		_ASSERTE(*(LPDWORD)pLoadPreview == sizeof(struct CET_LoadInfo));
		SETERROR(PEE_INVALID_VERSION);
		return FALSE;
	}

	if (pLoadPreview->pContext != (LPVOID)1)
	{
		SETERROR(PEE_INVALID_CONTEXT);
		return FALSE;
	}

	if (!pLoadPreview->pFileData || pLoadPreview->nFileSize < 512)
	{
		SETERROR(PEE_FILE_NOT_FOUND);
		return FALSE;
	}

	const BYTE  *pb  = (const BYTE*)pLoadPreview->pFileData;
	if (pb[0] != 'M' || pb[1] != 'Z')
	{
		SETERROR(PEE_UNSUPPORTEDFORMAT);
		return FALSE;
	}

	PEData *pData = (PEData*)CALLOC(sizeof(PEData));
	if (!pData)
	{
		SETERROR(PEE_NOT_ENOUGH_MEMORY);
		return FALSE;
	}
	pData->nMagic = ePeStr_Info;
	pLoadPreview->pFileContext = (void*)pData;
	LPCWSTR pszSlash, pszDot = NULL;
	pszSlash = wcsrchr(pLoadPreview->sFileName, L'\\');
	if (pszSlash) pszDot = wcsrchr(pszSlash, L'.');
	if (pszDot) lstrcpyn(pData->szExtension, pszDot, ARRAYSIZE(pData->szExtension));

	TODO("Load version info, and ICON?");
	//gpCurData = pData;
	BOOL lbRc = FALSE;

	if (DumpFile(pData, (LPBYTE)pLoadPreview->pFileData, pLoadPreview->nFileSize, pLoadPreview->nFileSize)
	        && (pData->nFlags || pData->nBits))
	{
		pData->pMappedFileBase = NULL;

		// [x64/x86] [FAR1/FAR2] [FileVersion]
		//wchar_t szInfo[512];
		wchar_t* pszInfo = pData->szInfo;
		
		*(pszInfo++) = (pData->nFlags & PE_SIGNED) ? L's' : L'x';

		if (pData->Machine == IMAGE_FILE_MACHINE_IA64)
		{
			//if (pData->nFlags & PE_SIGNED) *(pszInfo++) = L's';

			*(pszInfo++) = L'I'; *(pszInfo++) = L'A'; *(pszInfo++) = L' '; *pszInfo = 0;
		}
		else
		{
			//*(pszInfo++) = (pData->nFlags & PE_SIGNED) ? L's' : L'x';
			lstrcpy(pszInfo, (pData->nBits == 64) ? L"64 " : (pData->nBits == 16) ? L"16 " : L"32 ");
		}

		//lstrcpy(pszInfo, (pData->nBits == 64) ? L"64 " : (pData->nBits == 16) ? L"16 " : L"32 ");

		if ((pData->nFlags & (PE_Far1|PE_Far2|PE_Far3)))
		{
			lstrcat(pData->szInfo, L"Far");
			bool lbFirst = true;
			if (pData->nFlags & PE_Far1)
			{
				if (lbFirst) lbFirst = false; else lstrcat(pData->szInfo, L"&");
				lstrcat(pData->szInfo, L"1");
			}
			if (pData->nFlags & PE_Far2)
			{
				if (lbFirst) lbFirst = false; else lstrcat(pData->szInfo, L"&");
				lstrcat(pData->szInfo, L"2");
			}
			if (pData->nFlags & PE_Far3)
			{
				if (lbFirst) lbFirst = false; else lstrcat(pData->szInfo, L"&");
				lstrcat(pData->szInfo, L"3");
			}
			lstrcat(pData->szInfo, L" ");
		}
		//if ((pData->nFlags & (PE_Far1|PE_Far2)) == (PE_Far1|PE_Far2))
		//	lstrcat(pData->szInfo, L"Far1&2 ");
		//else if ((pData->nFlags & PE_Far1))
		//	lstrcat(pData->szInfo, L"Far1 ");
		//else if ((pData->nFlags & PE_Far2))
		//	lstrcat(pData->szInfo, L"Far2 ");

		if ((pData->nFlags & PE_UPX)) lstrcat(pData->szInfo, L"Upx ");
		if ((pData->nFlags & PE_DOTNET)) lstrcat(pData->szInfo, L".Net ");
		if (pData->szVersion[0])
		{
			lstrcat(pData->szInfo, pData->szVersion);
			if (pData->szVersionN[0])
			{
				lstrcat(pData->szInfo, L" [");
				lstrcat(pData->szInfo, pData->szVersionN);
				lstrcat(pData->szInfo, L"]");
			}
		}
		else if (pData->szVersionN[0])
		{
			lstrcat(pData->szInfo, pData->szVersionN);
		}

		pLoadPreview->pszComments = pData->szInfo;

		lbRc = TRUE;
	}
	else
	{
		pLoadPreview->pFileContext = NULL;
		pData->Close();
	}

	// Done
	//gpCurData = NULL;
	return lbRc;
}

extern "C" VOID WINAPI CET_Free(struct CET_LoadInfo* pLoadPreview)
{
	if (!pLoadPreview || *((LPDWORD)pLoadPreview) != sizeof(struct CET_LoadInfo))
	{
		_ASSERTE(*(LPDWORD)pLoadPreview == sizeof(struct CET_LoadInfo));
		SETERROR(PEE_INVALID_VERSION);
		return;
	}

	if (!pLoadPreview->pFileContext)
	{
		SETERROR(PEE_INVALID_CONTEXT);
		return;
	}

	if ((*(LPDWORD)pLoadPreview->pFileContext) == ePeStr_Info)
	{
		PEData *pData = (PEData*)pLoadPreview->pFileContext;
		pData->Close();
	}
}

extern "C" VOID WINAPI CET_Cancel(LPVOID pContext)
{
}


#else // #ifndef verc0_EXPORTS

// для унификации col0host & PanelView
#include "../farplugin.h"

/* FAR */
PluginStartupInfo psi;
FarStandardFunctions fsf;

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
                     )
{
	return TRUE;
}


#if defined(__GNUC__)

extern
BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
                     );

#ifdef __cplusplus
extern "C" {
#endif
	BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved);
	int WINAPI GetMinFarVersionW(void);
	void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info);
	void WINAPI GetPluginInfoW(struct PluginInfo *Info);
	int WINAPI GetCustomDataW(const wchar_t *FilePath, wchar_t **CustomData);
	void WINAPI FreeCustomDataW(wchar_t *CustomData);
#ifdef __cplusplus
};
#endif

BOOL WINAPI DllMainCRTStartup(HANDLE hDll,DWORD dwReason,LPVOID lpReserved)
{
	DllMain(hDll, dwReason, lpReserved);
	return TRUE;
}
#endif

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

int WINAPI GetMinFarVersionW(void)
{
	return MAKEFARVERSION(2,0,1588);
}

void WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info)
{
	::psi = *Info;
	::fsf = *(Info->FSF);
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info)
{
	Info->StructSize = sizeof(struct PluginInfo);
	Info->Flags = PF_PRELOAD;
}

void cpytag(char* &psz, const char* src, int nMax)
{
	for (int i = 0; i < nMax && *src; i++)
		*(psz++) = *(src++);
	*psz = 0;
}

int WINAPI GetCustomDataW(const wchar_t *FilePath, wchar_t **CustomData)
{
	*CustomData = NULL;
	int nLen = lstrlenW(FilePath);

	// Путь должен быть, просто имя файла (без пути) - пропускаем
	LPCWSTR pszSlash, pszDot = NULL;
	pszSlash = wcsrchr(FilePath, L'\\');
	if (pszSlash) pszDot = wcsrchr(pszSlash, L'.');
	if (!pszDot || !pszDot[0]) return FALSE;

	BOOL lbDosCom = FALSE;
	//TODO: Если появится возможность просмотра нескольких байт - хорошо бы это делать один раз на пачку плагинов?
	if (lstrcmpiW(pszDot, L".exe") && lstrcmpiW(pszDot, L".com")
		&& lstrcmpiW(pszDot, L".dll") && lstrcmpiW(pszDot, L".sys") && lstrcmpiW(pszDot, L".dl_") && lstrcmpiW(pszDot, L".dl")
		&& lstrcmpiW(pszDot, L".pvd") && lstrcmpiW(pszDot, L".so") && lstrcmpiW(pszDot, L".fmt")
		&& lstrcmpiW(pszDot, L".module") && lstrcmpiW(pszDot, L".fll") && lstrcmpiW(pszDot, L".ahp")
		&& lstrcmpiW(pszDot, L".mui") && lstrcmpiW(pszDot, L".arx") && lstrcmpiW(pszDot, L".sfx") 
		&& lstrcmpiW(pszDot, L".t32") && lstrcmpiW(pszDot, L".t64")
		)
	{
		return FALSE;
	}
	lbDosCom = (lstrcmpiW(pszDot, L".com") == 0);
	
	BOOL lbRc = FALSE;

	LPCTSTR pszUNCPath = FilePath;
	//TCHAR* pszBuffer = NULL;
	//nLen = lstrlen(FilePath);
	//if (FilePath[0] == L'\\' && FilePath[1] == L'\\')
	//{
	//	if (FilePath[2] != L'?' && FilePath[2] != L'.')
	//	{
	//		// UNC
	//		pszBuffer = (wchar_t*)malloc((nLen+10)*2);
	//		lstrcpy(pszBuffer, L"\\\\?\\UNC");
	//		lstrcat(pszBuffer, FilePath+1);
	//		pszUNCPath = pszBuffer;
	//	}
	//}
	//else if (FilePath[1] == L':' && FilePath[2] == L'\\')
	//{
	//	// UNC
	//	pszBuffer = (wchar_t*)malloc((nLen+10)*2);
	//	lstrcpy(pszBuffer, L"\\\\?\\");
	//	lstrcat(pszBuffer, FilePath);
	//	pszUNCPath = pszBuffer;
	//}
	
	HANDLE hFile = CreateFileW(pszUNCPath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0,0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwErr = GetLastError();
		if (dwErr == ERROR_SHARING_VIOLATION)
		{
			hFile = CreateFileW(pszUNCPath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0,0);
			//if (hFile == INVALID_HANDLE_VALUE)
			//	hFile = CreateFileW(FilePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0,0);
		}
	}
	LARGE_INTEGER nSize = {{0}};
	LARGE_INTEGER nFullSize = {{0}};
	if (hFile != INVALID_HANDLE_VALUE)
	{
		BYTE Signature[2]; DWORD nRead = 0;
		
		HANDLE hFileMapping = NULL;
		LPBYTE pFileData = NULL;
		BOOL   lbWindows = FALSE;
		
		if (GetFileSizeEx(hFile, &nSize) && (nSize.QuadPart > 512)
			&& ReadFile(hFile, Signature, 2, &nRead, NULL)
			&& (Signature[0] == 'M' && Signature[1] == 'Z'))
		{
			BOOL lbSucceeded = TRUE;
			LARGE_INTEGER liPos, liTest;
			IMAGE_DOS_HEADER dosHeader = {0};
			IMAGE_NT_HEADERS64 NTHeader64 = {0}; // 64-версия больше 32 битной. считываем больший блок данных

			nFullSize.QuadPart = nSize.QuadPart;
			dosHeader.e_magic = IMAGE_DOS_SIGNATURE;
			lbSucceeded = ReadFile(hFile, 2+(LPBYTE)&dosHeader, sizeof(dosHeader)-2, &nRead, NULL);

			if (lbSucceeded)
			{
				liPos.QuadPart = dosHeader.e_lfanew;
				lbSucceeded = (liPos.QuadPart >= sizeof(dosHeader) && liPos.QuadPart < nSize.QuadPart);
			}
			
			if (lbSucceeded)
			{
				lbSucceeded = SetFilePointerEx(hFile, liPos, &liTest, FILE_BEGIN)
					&& ReadFile(hFile, &NTHeader64, sizeof(NTHeader64), &nRead, NULL)
					&& (NTHeader64.Signature == IMAGE_NT_SIGNATURE
						|| (NTHeader64.Signature & 0xFFFF) == IMAGE_OS2_SIGNATURE);
					//	|| (NTHeader64.Signature & 0xFFFF) == IMAGE_OS2_SIGNATURE_LE); - SMARTDRV.EXE, это не win
			}
			
			if (lbSucceeded)
			{
				lbWindows = TRUE;
				// Теперь нужно скорректировать размер отображаемого в память файла (отбрасываем аппендикс, вроде SFX архивов)
				if (NTHeader64.Signature == IMAGE_NT_SIGNATURE)
				{
					if (NTHeader64.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
					{
						if (NTHeader64.OptionalHeader.SizeOfImage < nSize.QuadPart)
							nSize.QuadPart = NTHeader64.OptionalHeader.SizeOfImage;
					}
					else
					{
						PIMAGE_NT_HEADERS32 pNTHeader32 = (PIMAGE_NT_HEADERS32)&NTHeader64;
						if (pNTHeader32->OptionalHeader.SizeOfImage < nSize.QuadPart)
							nSize.QuadPart = pNTHeader32->OptionalHeader.SizeOfImage;
					}
				}
				else
				{
					// Это скорее всего 16битная exe/dll, или что-то другое, в формате OS2
				}
				if (nSize.QuadPart > (256<<20)) // Грузим не больше 256Mb
					nSize.QuadPart = (256<<20);
			}

			if (lbSucceeded)
			{
				hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
				lbSucceeded = (hFileMapping != NULL);
			}

			if (lbSucceeded)
			{
				pFileData = (PBYTE)MapViewOfFile(hFileMapping,FILE_MAP_READ,0,0,nSize.LowPart);
				lbSucceeded = (pFileData != NULL);
			}

			// Можно продолжать?
			if (lbSucceeded)
			{
				PEData Ver;
				if (pszDot) lstrcpyn(Ver.szExtension, pszDot, ARRAYSIZE(Ver.szExtension));

				Ver.nFlags |= PE_ICON_EXISTS; // чтобы иконки не пытаться искать. нужна только версия
				BOOL lbDump = DumpFile(&Ver, pFileData, nSize.QuadPart, nFullSize.QuadPart);

				if (lbDump && (Ver.szVersion[0] != 0 || Ver.nBits != 0))
				{
					nLen = lstrlen(Ver.szVersion)+10;
					if (nLen < 2) return FALSE;
					*CustomData = (wchar_t*)malloc(nLen*2);
					wchar_t szBits[6];
					if (Ver.nFlags & PE_DOTNET)
						wsprintfW(szBits, L"%sDN", (Ver.nFlags & PE_SIGNED) ? L"s" : L"x");
					else if (Ver.Machine==IMAGE_FILE_MACHINE_IA64)
						wsprintfW(szBits, L"%sIA", (Ver.nFlags & PE_SIGNED) ? L"s" : L"x");
					else
						wsprintfW(szBits, L"%s%i", (Ver.nFlags & PE_SIGNED) ? L"s" : L"x", Ver.nBits);
					wsprintfW(*CustomData, L"[%s]%s%s",
					          szBits,
					          Ver.szVersion[0] ? L" " : L"", Ver.szVersion);
					lbRc = TRUE;
				}
			}
			
			if (pFileData)
				UnmapViewOfFile(pFileData);
			if (hFileMapping)
				CloseHandle(hFileMapping);
		}

		if (!lbRc && (nSize.QuadPart > 0))
		{
			LPCWSTR pszDos = lbWindows ? L"[win]" : L"[dos]";
			nLen = lstrlen(pszDos)+1;
			*CustomData = (wchar_t*)malloc(nLen*2);
			lstrcpy(*CustomData, pszDos);

			lbRc = TRUE;
		}

		CloseHandle(hFile);
	}
	return lbRc;
}

void WINAPI FreeCustomDataW(wchar_t *CustomData)
{
	if (CustomData)
		free(CustomData);
}

#endif // #ifndef verc0_EXPORTS
