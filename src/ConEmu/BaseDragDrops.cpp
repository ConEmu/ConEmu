
#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "Header.h"
#include "../common/shlobj.h"


#if defined(__GNUC__) && !defined(__MINGW64_VERSION_MAJOR)
#include "ShObjIdl_Part.h"
#endif // __GNUC__

#include "BaseDragDrops.h"
#include "ConEmu.h"
#include "DragDrop.h"
#include "resource.h"

#define DEBUGSTRDATA(s) DEBUGSTR(s)

extern HINSTANCE g_hInstance;

/*
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{000214E4-0000-0000-C000-000000000046}]
@="IContextMenu"
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{000214F4-0000-0000-C000-000000000046}]
@="IContextMenu2"
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{0811AEBE-0B87-4C54-9E72-548CF649016B}]
@="IContextMenuSite"
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{15F936F5-A3FA-11D2-AEC3-00C04F79D1EB}]
@="IContextNotify"
[HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Interface\{15F936F6-A3FA-11D2-AEC3-00C04F79D1EB}]
@="IContextRegisterNotify"


*/


//CBaseDropTarget::CBaseDropTarget(/*HWND hwnd*/)
//{
//	//m_hWnd = hwnd;
//	m_lRefCount = 1;
//}

CBaseDropTarget::CBaseDropTarget()
{
	m_lRefCount = 1;
}

CBaseDropTarget::~CBaseDropTarget()
{
}

HRESULT STDMETHODCALLTYPE CBaseDropTarget::DragEnter(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	return 0;
}

HRESULT STDMETHODCALLTYPE CBaseDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	return 0;
}


HRESULT STDMETHODCALLTYPE CBaseDropTarget::Drop(IDataObject * pDataObject, DWORD grfKeyState, POINTL pt, DWORD * pdwEffect)
{
	//gpConEmu->SetDragCursor(nullptr);
	return 0;
}

HRESULT STDMETHODCALLTYPE CBaseDropTarget::DragLeave(void)
{
	return 0;
}

HRESULT __stdcall CBaseDropTarget::QueryInterface(REFIID iid, void ** ppvObject)
{
	if (iid == IID_IDropTarget || iid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

ULONG __stdcall CBaseDropTarget::AddRef(void)
{
	return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall CBaseDropTarget::Release(void)
{
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}




////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////




//
//	Constructor
//
CDropSource::CDropSource(CDragDropData* pCallback)
{
	m_lRefCount = 1;
	mh_CurCopy = nullptr; mh_CurMove = nullptr; mh_CurLink = nullptr;
	mp_Callback = pCallback;
}

//
//	Destructor
//
CDropSource::~CDropSource()
{
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CDropSource::AddRef(void)
{
	// increment object reference count
	return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CDropSource::Release(void)
{
	// decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CDropSource::QueryInterface(REFIID iid, void **ppvObject)
{
	// check to see what interface has been requested
	if (iid == IID_IDropSource || iid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

//
//	CDropSource::QueryContinueDrag
//
//	Called by OLE whenever Escape/Control/Shift/Mouse buttons have changed
//
HRESULT __stdcall CDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
	// if the <Escape> key has been pressed since the last call, cancel the drop
	if (fEscapePressed == TRUE)
	{
		if (mp_Callback)
			mp_Callback->DragFeedBack(DROPEFFECT_STOP_INTERNAL);

		return DRAGDROP_S_CANCEL;
	}

	DWORD nDragKey = ((gpConEmu->mouse.state & DRAG_L_STARTED) == DRAG_L_STARTED) ? MK_LBUTTON : MK_RBUTTON;
	DWORD nOtherKey = ((nDragKey & MK_LBUTTON) == MK_LBUTTON) ? (MK_RBUTTON|MK_MBUTTON) : (MK_LBUTTON|MK_MBUTTON);

	// if the <LeftMouse> button has been released, then do the drop!
	if ((grfKeyState & nDragKey) == 0)
	{
		if (mp_Callback)
			mp_Callback->DragFeedBack((DWORD)-1);

		return DRAGDROP_S_DROP;
	}

	// Если юзер нажимает другую мышиную кнопку
	if ((grfKeyState & nOtherKey) == nOtherKey)
	{
		if (mp_Callback)
			mp_Callback->DragFeedBack((DWORD)-1);

		return DRAGDROP_S_CANCEL;
	}

	// continue with the drag-drop
	return S_OK;
}

//
//	CDropSource::GiveFeedback
//
//	Return either S_OK, or DRAGDROP_S_USEDEFAULTCURSORS to instruct OLE to use the
//  default mouse cursor images
//
HRESULT __stdcall CDropSource::GiveFeedback(DWORD dwEffect)
{
	HRESULT hr = DRAGDROP_S_USEDEFAULTCURSORS;
	HCURSOR hCur = nullptr;
	DEBUGTEST(HRESULT hrTest = S_FALSE);

	//-- пока CFSTR_DROPDESCRIPTION не заводится...
	//if ((gOSVer.dwMajorVersion < 6) || !mp_Callback)
	{
		if (dwEffect != DROPEFFECT_NONE)
		{
			if (dwEffect & DROPEFFECT_COPY)
			{
				if (!mh_CurCopy) mh_CurCopy = LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_COPY));

				hCur = mh_CurCopy;
			}
			else if (dwEffect & DROPEFFECT_MOVE)
			{
				if (!mh_CurMove) mh_CurMove = LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_MOVE));

				hCur = mh_CurMove;
			}
			else if (dwEffect & DROPEFFECT_LINK)
			{
				if (!mh_CurLink) mh_CurLink = LoadCursor(g_hInstance, MAKEINTRESOURCE(IDC_LINK));

				hCur = mh_CurLink;
			}
		}
	}

	gpConEmu->SetDragCursor(hCur);
	if (hCur)
		hr = S_OK;

	if (mp_Callback)
		mp_Callback->DragFeedBack(dwEffect);

	return hr;
}

//
//	Helper routine to create an IDropSource object
//
HRESULT CreateDropSource(IDropSource **ppDropSource, CDragDropData* pCallback)
{
	if (ppDropSource == 0)
		return E_INVALIDARG;

	*ppDropSource = new CDropSource(pCallback);
	return (*ppDropSource) ? S_OK : E_OUTOFMEMORY;
}




////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

//
//	Constructor
//
CDataObject::CDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmed, int count)
{
	m_lRefCount  = 1;

	m_Data.reserve(32+std::max(count, 32));

	for (int i = 0; i < count; i++)
	{
		_ASSERTE(fmtetc && stgmed);
		m_Data[i].fUsed = TRUE;
		m_Data[i].fRelease = TRUE;
		m_Data[i].FormatEtc = fmtetc[i];
		m_Data[i].StgMedium = stgmed[i];
	}
}

//
//	Destructor
//
CDataObject::~CDataObject()
{
	// Освобождать данные в m_pStgMedium.hGlobal
	for (int i = 0; i < m_Data.size(); i++)
	{
		if (m_Data[i].fUsed && m_Data[i].fRelease)
		{
			ReleaseStgMedium(&(m_Data[i].StgMedium));
			m_Data[i].fRelease = FALSE;
		}
	}

	// cleanup
	m_Data.clear();

	DEBUGSTR(L"CDataObject::~CDataObject()\n");
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CDataObject::AddRef(void)
{
	// increment object reference count
	return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CDataObject::Release(void)
{
	// decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
	// check to see what interface has been requested
	if (iid == IID_IDataObject || iid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

HGLOBAL DupMem(HGLOBAL hMem)
{
	// lock the source memory object
	SIZE_T  len    = GlobalSize(hMem);
	PVOID   source = GlobalLock(hMem);
	if (!source)
		return nullptr;
	// create a fixed "global" block - i.e. just
	// a regular lump of our process heap
	PVOID   dest   = GlobalAlloc(GMEM_FIXED, len);
	if (dest)
		memcpy(dest, source, len);
	GlobalUnlock(hMem);
	return dest;
}

int CDataObject::LookupFormatEtc(FORMATETC *pFormatEtc)
{
	for (int i = 0; i < m_Data.size(); i++)
	{
		if (m_Data[i].fUsed
			&& (pFormatEtc->tymed    &  m_Data[i].FormatEtc.tymed)
			&& (pFormatEtc->cfFormat == m_Data[i].FormatEtc.cfFormat)
			&& (pFormatEtc->dwAspect == m_Data[i].FormatEtc.dwAspect))
		{
			return i;
		}
	}

	return -1;
}

LPCWSTR CDataObject::GetFormatName(CLIPFORMAT cfFormat, bool bRaw)
{
	static wchar_t szName[128];
	szName[0] = bRaw ? 0 : L'\'';

	if (GetClipboardFormatName(cfFormat, szName+(bRaw?0:1), 80) >= 1)
	{
		if (!bRaw)
		{
			wcscat_c(szName, L"',");
		}
	}
	else
	{
		szName[0] = 0;
	}

	if (!bRaw || (szName[0] == 0))
	{
		int nLen = lstrlen(szName);
		swprintf_c(szName+nLen, countof(szName)-nLen/*#SECURELEN*/, L"x%04X(%u)",
			cfFormat, cfFormat);
	}
	return szName;
}

//
//	IDataObject::GetData
//
HRESULT __stdcall CDataObject::GetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	int idx;

	#ifdef _DEBUG
	wchar_t szDbg[200];
	#endif

	//
	// try to match the requested FORMATETC with one of our supported formats
	//
	if ((idx = LookupFormatEtc(pFormatEtc)) == -1)
	{
		#ifdef _DEBUG
		swprintf_c(szDbg, L"!!! CDataObject::LookupFormatEtc(%s) failed\n", GetFormatName(pFormatEtc->cfFormat));
		DEBUGSTRDATA(szDbg);
		#endif
		return DV_E_FORMATETC;
	}

	#ifdef _DEBUG
	swprintf_c(szDbg, L"CDataObject::GetData {cfFormat=%s, lindex=%i, tymed=x%02X(%u)})",
		GetFormatName(pFormatEtc->cfFormat), pFormatEtc->lindex, pFormatEtc->tymed, pFormatEtc->tymed);
	LPCWSTR pszName = GetFormatName(pFormatEtc->cfFormat, true);
	DWORD nData = (DWORD)-1;
	if (lstrcmp(pszName, L"IsShowingLayered")==0
		|| lstrcmp(pszName, L"IsShowingText")==0
		|| lstrcmp(pszName, L"DragContext")==0
		|| lstrcmp(pszName, L"UsingDefaultDragImage")==0
		|| lstrcmp(pszName, L"DragSourceHelperFlags")==0
		|| lstrcmp(pszName, L"DragWindow")==0
		|| lstrcmp(pszName, L"DisableDragText")==0
		)
	{
		LPDWORD pdw = (LPDWORD)GlobalLock(m_Data[idx].StgMedium.hGlobal);
		if (pdw)
		{
			nData = *pdw;
			int nLen = lstrlen(szDbg);
			swprintf_c(szDbg+nLen, countof(szDbg)-nLen/*#SECURELEN*/, L", Data=x%02X(%u)", nData, nData);
		}
		GlobalUnlock(m_Data[idx].StgMedium.hGlobal);
	}
	wcscat_c(szDbg, L"\n");
	DEBUGSTRDATA(szDbg);
	#endif


	HRESULT hr = DV_E_FORMATETC;


	switch (m_Data[idx].FormatEtc.tymed)
	{
		case TYMED_HGLOBAL:
			//ReleaseStgMedium(pMedium);
			pMedium->hGlobal = DupMem(m_Data[idx].StgMedium.hGlobal);
			pMedium->pUnkForRelease = nullptr; // m_Data[idx].StgMedium.pUnkForRelease;
			hr = S_OK;
			break;

		case TYMED_ISTREAM:
			_ASSERTE(pMedium->pstm != m_Data[idx].StgMedium.pstm);
			//ReleaseStgMedium(pMedium);
			pMedium->pstm = m_Data[idx].StgMedium.pstm;
			if (m_Data[idx].StgMedium.pstm)
				m_Data[idx].StgMedium.pstm->AddRef();
			pMedium->pUnkForRelease = m_Data[idx].StgMedium.pUnkForRelease;
			hr = S_OK;
			break;

		case TYMED_ISTORAGE:
			_ASSERTE(pMedium->pstg != m_Data[idx].StgMedium.pstg);
			//ReleaseStgMedium(pMedium);
			pMedium->pstg = m_Data[idx].StgMedium.pstg;
			if (m_Data[idx].StgMedium.pstg)
				m_Data[idx].StgMedium.pstg->AddRef();
			pMedium->pUnkForRelease = m_Data[idx].StgMedium.pUnkForRelease;
			hr = S_OK;
			break;
		default:
			AssertMsg(L"Unsupported value in m_Data[idx].FormatEtc.tymed");
	}

	if (hr == S_OK)
	{
		//
		// found a match! transfer the data into the supplied storage-medium
		//
		pMedium->tymed			 = m_Data[idx].FormatEtc.tymed;
		//Assert(pMedium->pUnkForRelease==nullptr && m_Data[idx].StgMedium.pUnkForRelease==nullptr);
		//pMedium->pUnkForRelease  = nullptr;
	}
	else
	{
		#ifdef _DEBUG
		swprintf_c(szDbg, L"!!! CDataObject::GetData(tymed=%u) failed", m_Data[idx].FormatEtc.tymed);
		DEBUGSTRDATA(szDbg);
		//_ASSERTE(FALSE && "Unsupported tymed!");
		#endif
	}

	return hr;
}

//
//	IDataObject::GetDataHere
//
HRESULT __stdcall CDataObject::GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
	#ifdef _DEBUG
	wchar_t szDbg[200];
	swprintf_c(szDbg, L"CDataObject::GetDataHere({cfFormat=x%04X(%u), lindex=%i, tymed=x%02X(%u)}, {tymed=x%02X})\n",
		pFormatEtc->cfFormat, pFormatEtc->cfFormat, pFormatEtc->lindex, pFormatEtc->tymed, pFormatEtc->tymed, pMedium->tymed);
	DEBUGSTRDATA(szDbg);
	#endif

	_ASSERTE(FALSE && "CDataObject::GetDataHere not impemented!");

	// GetDataHere is only required for IStream and IStorage mediums
	// It is an error to call GetDataHere for things like HGLOBAL and other clipboard formats
	//
	//	OleFlushClipboard
	//
	return DATA_E_FORMATETC;
}

//
//	IDataObject::QueryGetData
//
//	Called to see if the IDataObject supports the specified format of data
//
HRESULT __stdcall CDataObject::QueryGetData(FORMATETC *pFormatEtc)
{
	#ifdef _DEBUG
	wchar_t szDbg[200];
	swprintf_c(szDbg, L"CDataObject::LookupFormatEtc({cfFormat=x%04X(%u), lindex=%i, tymed=x%02X(%u)})\n",
		pFormatEtc->cfFormat, pFormatEtc->cfFormat, pFormatEtc->lindex, pFormatEtc->tymed, pFormatEtc->tymed);
	DEBUGSTRDATA(szDbg);
	#endif

	return (LookupFormatEtc(pFormatEtc) == -1) ? DV_E_FORMATETC : S_OK;
}

//
//	IDataObject::GetCanonicalFormatEtc
//
HRESULT __stdcall CDataObject::GetCanonicalFormatEtc(FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut)
{
	// Apparently we have to set this field to nullptr even though we don't do anything else
	pFormatEtcOut->ptd = nullptr;
	return E_NOTIMPL;
}

HRESULT CDataObject::SetDataInt(LPCWSTR sFmtName, const void* hData, DWORD nDataSize /*= 0*/)
{
	HRESULT hr;

	FORMATETC       fmtetc[] =
	{
		{ (CLIPFORMAT)RegisterClipboardFormat(sFmtName), 0, DVASPECT_CONTENT, -1, TYMED_HGLOBAL },
	};
	STGMEDIUM       stgmed[] =
	{
		{ TYMED_HGLOBAL },
	};

	if (nDataSize == 0)
	{
		stgmed[0].hGlobal = (HGLOBAL)hData;
	}
	else
	{
		stgmed[0].hGlobal = GlobalAlloc(GPTR, nDataSize);
		memmove(stgmed[0].hGlobal, hData, nDataSize);
	}

	hr = SetData(fmtetc, stgmed, TRUE);

	return hr;
}

//
//	IDataObject::SetData
//
HRESULT __stdcall CDataObject::SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium,  BOOL fRelease)
{
	_ASSERTE(pMedium && pMedium->pUnkForRelease==nullptr);

	#ifdef _DEBUG
	LPCWSTR pszName = GetFormatName(pFormatEtc->cfFormat, true);
	DWORD nData = (DWORD)-1;
	if (lstrcmp(pszName, L"IsShowingLayered")==0
		|| lstrcmp(pszName, L"IsShowingText")==0
		|| lstrcmp(pszName, L"DragContext")==0
		|| lstrcmp(pszName, L"UsingDefaultDragImage")==0
		|| lstrcmp(pszName, L"DragSourceHelperFlags")==0
		|| lstrcmp(pszName, L"DragWindow")==0
		|| lstrcmp(pszName, L"DisableDragText")==0
		)
	{
		LPDWORD pdw = (LPDWORD)GlobalLock(pMedium->hGlobal);
		if (pdw)
		{
			nData = *pdw;
		}
		GlobalUnlock(pMedium->hGlobal);
	}

	wchar_t szDbg[255];
	swprintf_c(szDbg, L"CDataObject::SetData {cfFormat=%s, lindex=%i, tymed=x%02X(%u)}, {tymed=x%02X}",
		GetFormatName(pFormatEtc->cfFormat), pFormatEtc->lindex, pFormatEtc->tymed, pFormatEtc->tymed, pMedium->tymed);
	if (nData != (DWORD)-1)
	{
		int nLen = lstrlen(szDbg);
		swprintf_c(szDbg+nLen, countof(szDbg)-nLen/*#SECURELEN*/, L", Data=x%02X(%u)", nData, nData);
	}
	wcscat_c(szDbg, L"\n");
	DEBUGSTRDATA(szDbg);
	#endif

	DEBUGTEST(bool bNew = false);
	LONG nIndex = LookupFormatEtc(pFormatEtc);

	if (nIndex >= 0)
	{
		if ((pMedium != &(m_Data[nIndex].StgMedium)) && (pMedium->hGlobal != &(m_Data[nIndex].StgMedium)))
		{
			if (m_Data[nIndex].fRelease)
			{
				ReleaseStgMedium(&m_Data[nIndex].StgMedium);
			}
			else
			{
				ZeroStruct(m_Data[nIndex].StgMedium);
			}
		}
		else
		{
			Assert((pMedium != &(m_Data[nIndex].StgMedium)) && (pMedium->hGlobal != &(m_Data[nIndex].StgMedium)));
		}
	}
	else //	if (nIndex < 0)
	{
		DEBUGTEST(bNew = true);
		_ASSERTE(nIndex < 0);

		DragData newItem = {};
		newItem.FormatEtc = *pFormatEtc;
		nIndex = m_Data.push_back(newItem);
	}

	m_Data[nIndex].fUsed = TRUE;
	m_Data[nIndex].fRelease = fRelease;
	m_Data[nIndex].StgMedium = *pMedium;

	return S_OK;
}

//
//	IDataObject::EnumFormatEtc
//
HRESULT __stdcall CDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
	HRESULT hr = E_NOTIMPL;

	if (dwDirection == DATADIR_GET)
	{
		// for Win2k+ you can use the SHCreateStdEnumFmtEtc API call, however
		// to support all Windows platforms we need to implement IEnumFormatEtc ourselves.

		UINT nNumFormats = m_Data.size();
		FORMATETC *pFormats = nNumFormats ? (FORMATETC*)calloc(nNumFormats, sizeof(*pFormats)) : nullptr;
		if (!pFormats)
		{
			return E_OUTOFMEMORY;
		}

		for (UINT i = 0; i < nNumFormats; i++)
		{
			pFormats[i] = m_Data[i].FormatEtc;
		}

		hr = CreateEnumFormatEtc(nNumFormats, pFormats, ppEnumFormatEtc);

		SafeFree(pFormats);
	}
	else
	{
		// the direction specified is not support for drag+drop
	}
	return hr;
}

//
//	IDataObject::DAdvise
//
HRESULT __stdcall CDataObject::DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	IDataObject::DUnadvise
//
HRESULT __stdcall CDataObject::DUnadvise(DWORD dwConnection)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	IDataObject::EnumDAdvise
//
HRESULT __stdcall CDataObject::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise)
{
	return OLE_E_ADVISENOTSUPPORTED;
}

//
//	Helper function
//
HRESULT CreateDataObject(FORMATETC *fmtetc, STGMEDIUM *stgmeds, UINT count, CDataObject **ppDataObject)
{
	if (ppDataObject == 0)
		return E_INVALIDARG;

	*ppDataObject = new CDataObject(fmtetc, stgmeds, count);
	return (*ppDataObject) ? S_OK : E_OUTOFMEMORY;
}








////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////












//
//	"Drop-in" replacement for SHCreateStdEnumFmtEtc. Called by CDataObject::EnumFormatEtc
//
HRESULT CreateEnumFormatEtc(UINT nNumFormats, FORMATETC *pFormatEtc, IEnumFORMATETC **ppEnumFormatEtc)
{
	if (nNumFormats == 0 || pFormatEtc == 0 || ppEnumFormatEtc == 0)
		return E_INVALIDARG;

	*ppEnumFormatEtc = new CEnumFormatEtc(pFormatEtc, nNumFormats);
	return (*ppEnumFormatEtc) ? S_OK : E_OUTOFMEMORY;
}

//
//	Helper function to perform a "deep" copy of a FORMATETC
//
static void DeepCopyFormatEtc(FORMATETC *dest, FORMATETC *source)
{
	// copy the source FORMATETC into dest
	*dest = *source;

	if (source->ptd)
	{
		// allocate memory for the DVTARGETDEVICE if necessary
		dest->ptd = (DVTARGETDEVICE*)CoTaskMemAlloc(sizeof(DVTARGETDEVICE));
		// copy the contents of the source DVTARGETDEVICE into dest->ptd
		if (dest->ptd)
			*(dest->ptd) = *(source->ptd);
	}
}

//
//	Constructor
//
CEnumFormatEtc::CEnumFormatEtc(FORMATETC *pFormatEtc, int nNumFormats)
{
	m_lRefCount   = 1;
	m_nIndex      = 0;
	m_nNumFormats = nNumFormats;
	m_pFormatEtc  = nNumFormats ? (FORMATETC*)calloc(nNumFormats, sizeof(FORMATETC)) : nullptr;

	// copy the FORMATETC structures
	for (int i = 0; i < nNumFormats; i++)
	{
		DeepCopyFormatEtc(&m_pFormatEtc[i], &pFormatEtc[i]);
	}
}

//
//	Destructor
//
CEnumFormatEtc::~CEnumFormatEtc()
{
	if (m_pFormatEtc)
	{
		for (ULONG i = 0; i < m_nNumFormats; i++)
		{
			if (m_pFormatEtc[i].ptd)
				CoTaskMemFree(m_pFormatEtc[i].ptd);
		}

		SafeFree(m_pFormatEtc);
	}
}

//
//	IUnknown::AddRef
//
ULONG __stdcall CEnumFormatEtc::AddRef(void)
{
	// increment object reference count
	return InterlockedIncrement(&m_lRefCount);
}

//
//	IUnknown::Release
//
ULONG __stdcall CEnumFormatEtc::Release(void)
{
	// decrement object reference count
	LONG count = InterlockedDecrement(&m_lRefCount);

	if (count == 0)
	{
		delete this;
		return 0;
	}
	else
	{
		return count;
	}
}

//
//	IUnknown::QueryInterface
//
HRESULT __stdcall CEnumFormatEtc::QueryInterface(REFIID iid, void **ppvObject)
{
	// check to see what interface has been requested
	if (iid == IID_IEnumFORMATETC || iid == IID_IUnknown)
	{
		AddRef();
		*ppvObject = this;
		return S_OK;
	}
	else
	{
		*ppvObject = 0;
		return E_NOINTERFACE;
	}
}

//
//	IEnumFORMATETC::Next
//
//	If the returned FORMATETC structure contains a non-null "ptd" member, then
//  the caller must free this using CoTaskMemFree (stated in the COM documentation)
//
HRESULT __stdcall CEnumFormatEtc::Next(ULONG celt, FORMATETC *pFormatEtc, ULONG * pceltFetched)
{
	ULONG copied  = 0;

	// validate arguments
	if (celt == 0 || pFormatEtc == 0)
		return E_INVALIDARG;

	// copy FORMATETC structures into caller's buffer
	while (m_nIndex < m_nNumFormats && copied < celt)
	{
		DeepCopyFormatEtc(&pFormatEtc[copied], &m_pFormatEtc[m_nIndex]);
		copied++;
		m_nIndex++;
	}

	// store result
	if (pceltFetched != 0)
		*pceltFetched = copied;

	// did we copy all that was requested?
	return (copied == celt) ? S_OK : S_FALSE;
}

//
//	IEnumFORMATETC::Skip
//
HRESULT __stdcall CEnumFormatEtc::Skip(ULONG celt)
{
	m_nIndex += celt;
	return (m_nIndex <= m_nNumFormats) ? S_OK : S_FALSE;
}

//
//	IEnumFORMATETC::Reset
//
HRESULT __stdcall CEnumFormatEtc::Reset(void)
{
	m_nIndex = 0;
	return S_OK;
}

//
//	IEnumFORMATETC::Clone
//
HRESULT __stdcall CEnumFormatEtc::Clone(IEnumFORMATETC ** ppEnumFormatEtc)
{
	HRESULT hResult;
	// make a duplicate enumerator
	hResult = CreateEnumFormatEtc(m_nNumFormats, m_pFormatEtc, ppEnumFormatEtc);

	if (hResult == S_OK)
	{
		// manually set the index state
		((CEnumFormatEtc *) *ppEnumFormatEtc)->m_nIndex = m_nIndex;
	}

	return hResult;
}









//HANDLE StringToHandle(LPCWSTR szText, int nTextLen)
//{
//	void  *ptr;
//
//	// if text length is -1 then treat as a nul-terminated string
//	if (nTextLen <= 0)
//		nTextLen = _tcslen(szText) + 1;
//
//	// allocate and lock a global memory buffer. Make it fixed
//	// data so we don't have to use GlobalLock
//	ptr = (void *)GlobalAlloc(GMEM_FIXED, nTextLen+1);
//	// copy the string into the buffer
//	if (ptr)
//		memcpy(ptr, szText, nTextLen);
//	return ptr;
//}
