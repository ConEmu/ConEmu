
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


/*

Spacing chars
0020	SPACE
	* sometimes considered a control code
	* other space characters: 2000-200A
	x (no-break space - 00A0)
	x (zero width space - 200B)
	x (word joiner - 2060)
	x (ideographic space - 3000)
	x (zero width no-break space - FEFF)

200C    ZERO WIDTH NON-JOINER
200C    ZERO WIDTH NON-JOINER
200E    LEFT-TO-RIGHT MARK
2060    WORD JOINER
FEFF    ZERO WIDTH NO-BREAK SPACE


*/


/*
200E    LEFT-TO-RIGHT MARK
    * commonly abbreviated LRM
200F    RIGHT-TO-LEFT MARK
    * commonly abbreviated RLM
*/

#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR
#ifdef _DEBUG
//#define SHOWDEBUGSTEPS
#endif

#include "Header.h"

#include "Background.h"
#include "ConEmu.h"
#include "ConEmuPipe.h"
#include "Font.h"
#include "FontMgr.h"
#include "FontPtr.h"
#include "Options.h"
#include "OptionsClass.h"
#include "RealConsole.h"
#include "../common/ConPtyGraphemeDraw.h"
#include "ScreenDump.h"
#include "SetPgInfo.h"
#include "TabBar.h"
#include "TaskBarGhost.h"
#include "VConGroup.h"
#include "VConText.h"
#include "VirtualConsole.h"


#include "LngRc.h"
#include "../common/MSetter.h"

using ConEmu::PaletteColors;


#ifdef _DEBUG
#define DEBUGDRAW_RCONPOS VK_SCROLL // -- при включенном ScrollLock отрисовать прямоугольник, соответствующий положению окна RealConsole
#define DEBUGDRAW_DIALOGS VK_CAPITAL // -- при включенном Caps отрисовать прямоугольники, соответствующие найденным диалогам
#define DEBUGDRAW_VERTICALS VK_SCROLL // -- при включенном ScrollLock отрисовать прямоугольники, соответствующие граням диалогов (которые строго выравниваются по ячейкам)
#endif

#define DEBUGSTRDRAW(s) //DEBUGSTR(s)
#define DEBUGSTRCOORD(s) //DEBUGSTR(s)
#define DEBUGSTRFAIL(s) DEBUGSTR(s)
#define DEBUGSTRAPPID(s) DEBUGSTR(s)
#define DEBUGSTRDESTROY(s) DEBUGSTR(s)

// WARNING("не появляются табы во второй консоли");
WARNING("На предыдущей строке символы под курсором прыгают влево");
WARNING("При быстром наборе текста курсор часто 'замерзает' на одной из букв, но продолжает двигаться дальше");

WARNING("Глюк с частичной отрисовкой экрана часто появляется при Alt-F7, Enter. Особенно, если файла нет");
// Иногда не отрисовывается диалог поиска полностью - только бежит текущая сканируемая директория.
// Иногда диалог отрисовался, но часть до текста "..." отсутствует

WARNING("В каждой VCon создать буфер BYTE[256] для хранения распознанных клавиш (Ctrl,...,Up,PgDn,Add,и пр.");
WARNING("Нераспознанные можно помещать в буфер {VKEY,wchar_t=0}, в котором заполнять последний wchar_t по WM_CHAR/WM_SYSCHAR");
WARNING("При WM_(SYS)CHAR помещать wchar_t в начало, в первый незанятый VKEY");
WARNING("При нераспознанном WM_KEYUP - брать(и убрать) wchar_t из этого буфера, послав в консоль UP");
TODO("А периодически - проводить проверку isKeyDown, и чистить буфер");
WARNING("при переключении на другую консоль (да наверное и в процессе просто нажатий - модификатор может быть изменен в самой программе) требуется проверка caps, scroll, num");
WARNING("а перед пересылкой символа/клавиши проверять нажат ли на клавиатуре Ctrl/Shift/Alt");

WARNING("Часто после разблокирования компьютера размер консоли изменяется (OK), но обновленное содержимое консоли не приходит в GUI - там остается обрезанная верхняя и нижняя строка");



#define ISBGIMGCOLOR(a) (nBgImageColors & (1 << a))

#define CURSOR_PAT_COLOR  0xC0C0C0
#define HILIGHT_PAT_COLOR 0xC0C0C0

#ifndef CONSOLE_SELECTION_NOT_EMPTY
#define CONSOLE_SELECTION_NOT_EMPTY     0x0002   // non-null select rectangle
#endif

#ifdef _DEBUG
#define DUMPDC(f) if (mb_DebugDumpDC) DumpImage((HDC)m_DC, nullptr, m_DC.iWidth, m_DC.iHeight, f);
#else
#define DUMPDC(f)
#endif


#ifdef VAlloc
#undef VAlloc
#endif
#ifdef VFree
#undef VFree
#endif
#ifdef HEAPVAL
#undef HEAPVAL
#endif
#ifdef HEAPVALPTR
#undef HEAPVALPTR
#endif
#define VAlloc(c,s) m_Heap.Alloc(c,s)
#define VFree(f) if (f) { m_Heap.Free(f); f = nullptr; }
#define HEAPVAL m_Heap.Validate()
#define HEAPVALPTR(x) //

void CVirtualConsole::VConHeap::Init(size_t initSize)
{
	if (mh_Heap)
	{
		_ASSERTE(mh_Heap==nullptr);
		return;
	}
	mh_Heap = HeapCreate(HEAP_GENERATE_EXCEPTIONS, initSize, 0);
}

LPVOID CVirtualConsole::VConHeap::Alloc(size_t nCount, size_t nSize)
{
	Validate();

	size_t nWhole = nCount * nSize;
	LPVOID ptr = HeapAlloc(mh_Heap, HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, nWhole);

	//HEAPVAL;
	return ptr;
}

void CVirtualConsole::VConHeap::Free(LPVOID ptr)
{
	if (ptr && mh_Heap)
	{
		Validate();

		HeapFree(mh_Heap, 0, ptr);

		//HEAPVAL;
	}
}

void CVirtualConsole::VConHeap::Validate()
{
	#ifdef _DEBUG
	HeapValidate(mh_Heap, 0, nullptr);
	#endif
}

CVirtualConsole::VConHeap::~VConHeap()
{
	if (mh_Heap)
	{
		HeapDestroy(mh_Heap);
		mh_Heap = nullptr;
	}
}



namespace VConCreateLogger
{
	static const int BUFFER_SIZE = 64;   // Must be a power of 2
	enum EventType
	{
		eCreate,
		eDelete,
	} Event;
	struct VConNewDel
	{
		CVirtualConsole* pVCon;
		EventType Event;
		DWORD Tick;
	};
	VConNewDel g_pos[BUFFER_SIZE] = {{nullptr}};
	LONG g_posidx = -1;

	void Log(CVirtualConsole* pVCon, EventType Event)
	{
		// Get next message index
		LONG i = _InterlockedIncrement(&g_posidx);
		// Write a message at this index
		VConNewDel& e = g_pos[i & (BUFFER_SIZE - 1)]; // Wrap to buffer size

		e.pVCon = pVCon;
		e.Event = Event;
		e.Tick = GetTickCount(); // msg.time == 0 скорее всего
	}
}



CVirtualConsole::PARTBRUSHES CVirtualConsole::m_PartBrushes[MAX_COUNT_PART_BRUSHES] = {{0}};
// MAX_SPACES == 0x400
wchar_t CVirtualConsole::ms_Spaces[MAX_SPACES];
wchar_t CVirtualConsole::ms_HorzDbl[MAX_SPACES];
wchar_t CVirtualConsole::ms_HorzSingl[MAX_SPACES];
//HMENU CVirtualConsole::mh_PopupMenu = nullptr;
//HMENU CVirtualConsole::mh_TerminatePopup = nullptr;
//HMENU CVirtualConsole::mh_DebugPopup = nullptr;
//HMENU CVirtualConsole::mh_EditPopup = nullptr;

static LONG gnVConLastCreatedID = 0;

#pragma warning(disable: 4355)
CVirtualConsole::CVirtualConsole(CConEmuMain* pOwner, int index)
	: CVConRelease(this)
	, CConEmuChild(this)
	, mp_RCon(nullptr)
	, mp_ConEmu(pOwner)
	, mp_Ghost(nullptr)
	, mp_Group(nullptr)
	, mn_Flags(vf_None)
	, mn_Index(index) // !!! Informational !!!
	, m_DC(nullptr)
	, m_SelectedFont(fnt_NULL)
	#ifdef __GNUC__
	, GdiAlphaBlend(nullptr)
	#endif
	, m_Sizes()
	, mp_Set(nullptr)
	, cinf()
	, winSize()
	, coord()
	, TextLen()
	, mb_RequiredForceUpdate(true)
	, mb_LastFadeFlag(false)
	, mn_LastBitsPixel()
	#ifdef APPDISTINCTBACKGROUND
	, mp_BgInfo(nullptr)
	#endif
	, TransparentInfo()
	, isFade(false)
	, isForeground(true)
	, m_SelfPalette()
	, mn_AppSettingsChangCount()
	, m_LeftPanelView()
	, m_RightPanelView()
	, mn_ConEmuFadeMsg(0)
	, mb_LeftPanelRedraw(false)
	, mb_RightPanelRedraw(false)
	, mn_LastDialogsCount(0)
	, mn_DialogsCount(0)
	, mn_DialogAllFlags(0)
	, mb_InPaintCall(false)
	, mb_InConsoleResize(false)
	, mb_ConDataChanged(false)
	, mh_TransparentRgn(nullptr)
	, mb_ChildWindowWasFound(false)
	, mn_BackColorIdx(RELEASEDEBUGTEST(0/*Black*/,2/*Green*/))
	, Cursor()
	, mb_PaintSkippedLogged(false)
	, isForce(true)
	, isFontSizeChanged(true)
	, mrc_Client()
	, mrc_Back()
	, mrc_UCharMap()
	, attrBackLast(0)
	, mn_LogScreenIdx(0)
	, isCursorValid(true)
	, drawImage(true)
	, textChanged(true)
	, attrChanged(true)
	, hBgDc(nullptr)
	, bgBmpSize()
	, mb_IsForceUpdate(false)
	, mb_InUpdate(false)
	, hSelectedBrush(nullptr)
	, hOldBrush(nullptr)
	, isEditor(false)
	, isViewer(false)
	, isFilePanel(false)
	, csbi()
	, mpsz_ConChar(nullptr)
	, mpsz_ConCharSave(nullptr)
	, mpn_ConAttrEx(nullptr)
	, mpn_ConAttrExSave(nullptr)
	, ConCharX(nullptr)
	, pbLineChanged(nullptr)
	, pbBackIsPic(nullptr)
	, pnBackRGB(nullptr)
	, m_etr()
	, mb_DialogsChanged(false)
	, mp_Bg(nullptr)
	, mpsz_LogScreen(nullptr)
	, mdw_LastError(0)
	, nBgImageColors(0)
	, m_HighlightInfo()
	, mb_PointersAllocated(false)
	#ifdef _DEBUG
	, mb_DebugDumpDC(false)
	#endif
{
	#pragma warning(default: 4355)

	mn_ID = InterlockedIncrement(&gnVConLastCreatedID);
	VConCreateLogger::Log(this, VConCreateLogger::eCreate);
	IndexStr();
	IDStr();

	ZeroStruct(ms_LastUCharMapFont);
	ZeroStruct(mrc_LastDialogs);
	ZeroStruct(mn_LastDialogFlags);
	ZeroStruct(mrc_Dialogs);
	ZeroStruct(mn_DialogFlags);

	_ASSERTE(mh_WndDC == nullptr);
}

CConEmuMain* CVirtualConsole::Owner()
{
	return this ? mp_ConEmu : nullptr;
}

int CVirtualConsole::Index()
{
	return mn_Index;
}

LPCWSTR CVirtualConsole::IndexStr()
{
	// Used for logging and user UX, 1-based indexes
	ltow_s(mn_Index + 1, ms_Idx, 10);
	return ms_Idx;
}

int CVirtualConsole::ID()
{
	return mn_ID;
}

LPCWSTR CVirtualConsole::IDStr()
{
	ltow_s(mn_ID, ms_ID, 10);
	return ms_ID;
}

bool CVirtualConsole::SetFlags(VConFlags Set, VConFlags Mask, int index)
{
	if (!this)
		return false;

	bool bChanged = false;

	VConFlags NewFlags = ((mn_Flags & ~Mask) | Set);

	if (NewFlags != mn_Flags)
	{
		mn_Flags = NewFlags;
		bChanged = true;
	}

	if ((index >= 0) && (index != mn_Index))
	{
		mn_Index = index;
	}

	return bChanged;
}

bool CVirtualConsole::Constructor(RConStartArgsEx *args)
{
	#ifdef __GNUC__
	MModule gdi32(GetModuleHandle(L"gdi32.dll"));
	gdi32.GetProcAddress("GdiAlphaBlend", GdiAlphaBlend);
	#endif

	mp_ConEmu->OnVConCreated(this, args);

	// #MEMORY скорректировать размер кучи
	size_t nMinHeapSize = (1000 + (200 * 90 * 2) * 6 + MAX_PATH*2)*2;
	m_Heap.Init(nMinHeapSize);

	cinf.dwSize = 15; cinf.bVisible = TRUE;

	mp_Bg = new CBackground();
	#ifdef APPDISTINCTBACKGROUND
	mp_BgInfo = args->pszWallpaper ? CBackgroundInfo::CreateBackgroundObject(args->pszWallpaper, false) : nullptr;
	#endif

	m_Colors = gpSet->GetColors(-1);

	m_Sizes.nFontHeight = gpFontMgr->FontHeight();
	m_Sizes.nFontWidth = gpFontMgr->FontWidth();

	Cursor.nBlinkTime = GetCaretBlinkTime();

	_ASSERTE((HDC)m_DC == nullptr);

	ResetHighlightCoords();

	// Some static-s initializers
	if (!ms_Spaces[0])
	{
		wmemset(ms_Spaces, L' ', MAX_SPACES);
		wmemset(ms_HorzDbl, ucBoxDblHorz, MAX_SPACES);
		wmemset(ms_HorzSingl, ucBoxSinglHorz, MAX_SPACES);
	}

	mp_ConEmu->EvalVConCreateSize(this, m_Sizes.TextWidth, m_Sizes.TextHeight);

	if (gpSet->isLogging(3) == 3)
	{
		mn_LogScreenIdx = 0;
		wchar_t szFile[MAX_PATH+64], *pszDot;
		wcscpy_c(szFile, mp_ConEmu->ms_ConEmuExe);

		if ((pszDot = wcsrchr(szFile, L'\\')) == nullptr)
		{
			DisplayLastError(L"wcsrchr failed!");
			return false; // ошибка
		}

		*pszDot = 0;
		size_t cchMax = (pszDot - szFile) + 64;
		mpsz_LogScreen = (wchar_t*)calloc(cchMax, 2);
		_wcscpy_c(mpsz_LogScreen, cchMax, szFile);
		_wcscat_c(mpsz_LogScreen, cchMax, L"\\ConEmu-VCon-%i-%i.con"/*, RCon()->GetServerPID()*/);
	}

	CreateView();

	mp_RCon = new CRealConsole(this, mp_ConEmu);
	if (!mp_RCon)
	{
		_ASSERTE(mp_RCon);
		return false;
	}

	return mp_RCon->Construct(this, args);

	// No sense to call InitDC - no console yet
}

CVirtualConsole::~CVirtualConsole()
{
	if (!isMainThread())
	{
		//_ASSERTE(isMainThread());
		MBoxA(L"~CVirtualConsole() called from background thread");
	}

	OnDestroy();

	if (mp_RCon)
		mp_RCon->StopThread();

	HEAPVAL;


	m_DC.Delete();


	PointersFree();

	VFree(mpsz_LogScreen);

	//DeleteCriticalSection(&csDC);
	//DeleteCriticalSection(&csCON);

	if (mh_TransparentRgn)
	{
		DeleteObject(mh_TransparentRgn);
		mh_TransparentRgn = nullptr;
	}

	// No need, actually, just for clearness
	m_UCharMapFont.Release();

	SafeDelete(mp_RCon);

	CVConGroup::OnVConDestroyed(this);

	//if (mh_PopupMenu) { -- static на все экземпляры
	//	DestroyMenu(mh_PopupMenu); mh_PopupMenu = nullptr;
	//}

	//MSectionLock SC;

	//if (mcs_BkImgData)
	//	SC.Lock(mcs_BkImgData, TRUE);

	//if (mp_BkImgData)
	//{
	//	free(mp_BkImgData);
	//	mp_BkImgData = nullptr;
	//	mn_BkImgDataMax = 0;
	//}

	//if (mp_BkEmfData)
	//{
	//	free(mp_BkEmfData);
	//	mp_BkEmfData = nullptr;
	//	mn_BkImgDataMax = 0;
	//}
	//
	//if (mcs_BkImgData)
	//{
	//	SC.Unlock();
	//	delete mcs_BkImgData;
	//	mcs_BkImgData = nullptr;
	//}

	SafeDelete(mp_Bg);
	#ifdef APPDISTINCTBACKGROUND
	SafeRelease(mp_BgInfo);
	#endif

	//FreeBackgroundImage();

	VConCreateLogger::Log(this, VConCreateLogger::eDelete);
}

void CVirtualConsole::InitGhost()
{
	if (gpSet->isTabsOnTaskBar())
	{
		if (mp_Ghost)
		{
			_ASSERTE(mp_Ghost==nullptr);
		}
		else if (isMainThread())
		{
			mp_Ghost = CTaskBarGhost::Create(this);
		}
		else
		{
			// Ghost-окна нужно создавать в главной нити
			// сюда мы попадаем при внешнем аттаче (например, при "-new_console")
			mp_ConEmu->CreateGhostVCon(this);
		}
	}
}

// WM_DESTROY
void CVirtualConsole::OnDestroy()
{
	DEBUGSTRDESTROY(L"CVirtualConsole::OnDestroy");

	if (this && mp_Ghost)
	{
		CTaskBarGhost* p = mp_Ghost;
		mp_Ghost = nullptr;
		delete p;
	}
}

CRealConsole* CVirtualConsole::RCon()
{
	if (this)
		return mp_RCon;
	_ASSERTE(this!=nullptr);
	return nullptr;
}

HWND CVirtualConsole::GuiWnd()
{
	if (this && mp_RCon)
		return mp_RCon->GuiWnd();
	_ASSERTE(this!=nullptr);
	return nullptr;
}

// Вызывается из WM_SETFOCUS в Child & Back
// т.к. фокус должен быть или в главном окне ConEmu или в ChildGUI
void CVirtualConsole::setFocus()
{
	if (!this)
		return;

	HWND hGuiWnd = GuiWnd();
	if (hGuiWnd && ::IsWindowVisible(hGuiWnd))
	{
		RCon()->GuiWndFocusRestore();
	}
	else
	{
		mp_ConEmu->setFocus();
	}
}

HWND CVirtualConsole::GhostWnd()
{
	if (this && mp_Ghost)
		return mp_Ghost->GhostWnd();
	return nullptr;
}

bool CVirtualConsole::isActive(bool abAllowGroup)
{
	if (!this)
		return false;
	VConFlags test = abAllowGroup ? vf_Visible : vf_Active;
	if (!(mn_Flags & test))
		return false;
	return true;
}

bool CVirtualConsole::isVisible()
{
	if (!this)
		return false;
	if (!(mn_Flags & vf_Visible))
		return false;
	if ((mn_Flags & vf_Maximized) && !(mn_Flags & vf_Active))
		return false;
	return true;
}

bool CVirtualConsole::isGroup()
{
	return CVConGroup::isGroup(this);
}

EnumVConFlags CVirtualConsole::isGroupedInput()
{
	if (!this)
		return evf_None;
	if (mp_ConEmu->isInputGrouped())
		return evf_All;
	if ((mn_Flags & vf_GroupSplit))
		return evf_Visible;
	if ((mn_Flags & vf_GroupSet))
		return evf_GroupSet;
	return evf_None;
}

void CVirtualConsole::PointersFree()
{
	HEAPVAL;
	VFree(mpsz_ConChar);
	VFree(mpsz_ConCharSave);
	VFree(mpn_ConAttrEx);
	VFree(mpn_ConAttrExSave);
	VFree(ConCharX);
	VFree(pbLineChanged);
	VFree(pbBackIsPic);
	VFree(pnBackRGB);
	// Row/Col highlights & Hyperlink underlining
	ResetHighlightCoords();
	HEAPVAL;
	mb_PointersAllocated = false;
}

bool CVirtualConsole::PointersAlloc()
{
	mb_PointersAllocated = false;
	HEAPVAL;

	unsigned nWidthHeight = (m_Sizes.nMaxTextWidth * m_Sizes.nMaxTextHeight);
#ifdef AllocArray
#undef AllocArray
#endif
#define AllocArray(p,t,c) p = (t*)VAlloc(c,sizeof(t)); if (!p) return false;
	AllocArray(mpsz_ConChar, wchar_t, nWidthHeight+1); // ASCIIZ
	AllocArray(mpsz_ConCharSave, wchar_t, nWidthHeight+1); // ASCIIZ
	AllocArray(mpn_ConAttrEx, CharAttr, nWidthHeight);
	AllocArray(mpn_ConAttrExSave, CharAttr, nWidthHeight);
	AllocArray(ConCharX, DWORD, nWidthHeight);
	AllocArray(pbLineChanged, bool, m_Sizes.nMaxTextHeight);
	AllocArray(pbBackIsPic, bool, m_Sizes.nMaxTextHeight);
	AllocArray(pnBackRGB, COLORREF, m_Sizes.nMaxTextHeight);
	HEAPVAL;

	mb_PointersAllocated = true;
	return true;
}

void CVirtualConsole::PointersZero()
{
	unsigned nWidthHeight = (m_Sizes.nMaxTextWidth * m_Sizes.nMaxTextHeight);
	//100607: Сбрасывать ВСЕ не будем. Эти массивы могут использоваться и в других нитях!
	HEAPVAL;
	//ZeroMemory(mpsz_ConChar, nWidthHeight*sizeof(*mpsz_ConChar));
	ZeroMemory(mpsz_ConCharSave, (nWidthHeight+1)*sizeof(*mpsz_ConChar)); // ASCIIZ
	HEAPVAL;
	//ZeroMemory(mpn_ConAttrEx, nWidthHeight*sizeof(*mpn_ConAttrEx));
	ZeroMemory(mpn_ConAttrExSave, nWidthHeight*sizeof(*mpn_ConAttrExSave));
	HEAPVAL;
	//ZeroMemory(ConCharX, nWidthHeight*sizeof(*ConCharX));
	HEAPVAL;
	ZeroMemory(pbLineChanged, m_Sizes.nMaxTextHeight*sizeof(*pbLineChanged));
	ZeroMemory(pbBackIsPic, m_Sizes.nMaxTextHeight*sizeof(*pbBackIsPic));
	ZeroMemory(pnBackRGB, m_Sizes.nMaxTextHeight*sizeof(*pnBackRGB));
	HEAPVAL;
	// Row/Col highlights & Hyperlink underlining
	ResetHighlightCoords();
}

// Function called when RCon is about to start new root
void CVirtualConsole::ResetOnStart()
{
	// Placeholder
}

// InitDC вызывается только при критических изменениях (размеры, шрифт, и т.п.) когда нужно пересоздать DC и Bitmap
bool CVirtualConsole::InitDC(bool abNoDc, bool abNoWndResize, MSectionLock *pSDC, MSectionLock *pSCON)
{
	if (!this || !mp_RCon)
	{
		_ASSERTE(mp_RCon != nullptr);
		return false;
	}
	_ASSERTE(isMainThread());

	MSectionLock _SCON;
	if (!(pSCON ? pSCON : &_SCON)->isLocked())
		(pSCON ? pSCON : &_SCON)->Lock(&csCON);
	BOOL lbNeedCreateBuffers = FALSE;
	unsigned rTextWidth = mp_RCon->TextWidth();
	unsigned rTextHeight = mp_RCon->TextHeight();

	if (!rTextWidth || !rTextHeight)
	{
		WARNING("Если тут ошибка - будет просто DC Initialization failed, что не понятно...");
		Assert(mp_RCon->TextWidth() && mp_RCon->TextHeight());
		return false;
	}

#ifdef _DEBUG
	// Теперь - выполняется в главной нити
	//if (mp_RCon->IsConsoleThread())
	//{
	//    //_ASSERTE(!mp_RCon->IsConsoleThread());
	//}
	//if (TextHeight == 24)
	//    TextHeight = 24;
	_ASSERT(m_Sizes.TextHeight >= MIN_CON_HEIGHT);
#endif

	// Буфер пересоздаем только если требуется его увеличение
	if (!mb_PointersAllocated ||
	        (m_Sizes.nMaxTextWidth * m_Sizes.nMaxTextHeight) < (rTextWidth * rTextHeight) ||
	        (m_Sizes.nMaxTextWidth < rTextWidth) // а это нужно для TextParts & tmpOem
	  )
		lbNeedCreateBuffers = TRUE;

	if (lbNeedCreateBuffers && mb_PointersAllocated)
	{
		DEBUGSTRDRAW(L"Relocking SCON exclusively\n");
		(pSCON ? pSCON : &_SCON)->RelockExclusive();
		DEBUGSTRDRAW(L"Relocking SCON exclusively (done)\n");
		PointersFree();
	}

	// InitDC вызывается только при первой инициализации или смене размера
	mb_IsForceUpdate = true; // поэтому выставляем флажок Force
	m_Sizes.TextWidth = rTextWidth;
	m_Sizes.TextHeight = rTextHeight;

	if (lbNeedCreateBuffers)
	{
		// Если увеличиваем размер - то с запасом, чтобы "два раза не ходить"...
		if (m_Sizes.nMaxTextWidth < m_Sizes.TextWidth)
			m_Sizes.nMaxTextWidth = m_Sizes.TextWidth+80;

		if (m_Sizes.nMaxTextHeight < m_Sizes.TextHeight)
			m_Sizes.nMaxTextHeight = m_Sizes.TextHeight+30;

		DEBUGSTRDRAW(L"Relocking SCON exclusively\n");
		(pSCON ? pSCON : &_SCON)->RelockExclusive();
		DEBUGSTRDRAW(L"Relocking SCON exclusively (done)\n");

		if (!PointersAlloc())
		{
			WARNING("Если тут ошибка - будет просто DC Initialization failed, что не понятно...");
			return false;
		}
	}
	else
	{
		PointersZero();
	}

	if (!pSCON)
		_SCON.Unlock();
	HEAPVAL;

	SelectFont(fnt_NULL);

	bool lbRc = false;

	// Это может быть, если отключена буферизация (debug) и вывод идет сразу на экран
	if (!abNoDc)
	{
		DEBUGSTRDRAW(L"*** Recreate DC\n");
		//MSectionLock _SDC;
		//// Если в этой нити уже заблокирован - секция не дергается
		//(pSDC ? pSDC : &_SDC)->Lock(&csDC, TRUE, 200); // но по таймауту, чтобы не повисли ненароком

		m_DC.Delete();


		Assert(gpFontMgr->FontWidth() && gpFontMgr->FontHeight());

		m_Sizes.nFontHeight = gpFontMgr->FontHeight();
		m_Sizes.nFontWidth = gpFontMgr->FontWidth();

		DEBUGTEST(BOOL lbWasInitialized = m_Sizes.TextWidth && m_Sizes.TextHeight);

		// Посчитать новый размер в пикселях
		m_Sizes.Width = m_Sizes.TextWidth * m_Sizes.nFontWidth;
		m_Sizes.Height = m_Sizes.TextHeight * m_Sizes.nFontHeight;

		UINT Pad = std::min<UINT>(gpSet->nCenterConsolePad, CENTERCONSOLEPAD_MAX) * 2
			+ std::max<LONG>(m_Sizes.nFontHeight, m_Sizes.nFontWidth) * 3;

		#ifdef _DEBUG
		if (m_Sizes.Height > 5000)
		{
			_ASSERTE(m_Sizes.Height <= 5000);
		}
		#endif

		if (ghOpWnd)
			mp_ConEmu->UpdateSizes();

		if (m_DC.Create(m_Sizes.Width+Pad, m_Sizes.Height+Pad))
		{
			// Запомнить кое-что
			m_Sizes.LastPadSize = gpSet->nCenterConsolePad;
			// OK
			lbRc = true;
		}

		goto wrap;
	}

	lbRc = true;
wrap:
	// сбросим, уже не требуется
	isFontSizeChanged = false;

	return lbRc;
}

void CVirtualConsole::DumpConsole()
{
	if (!this) return;

	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	wchar_t temp[MAX_PATH+5];
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghWnd;
	ofn.lpstrFilter = _T("ConEmu dumps (*.con)\0*.con\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = temp; temp[0] = 0;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Save console dump...";
	ofn.lpstrDefExt = L"con";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (!GetSaveFileName(&ofn))
		return;

	Dump(temp);
}

bool CVirtualConsole::Dump(LPCWSTR asFile)
{
	if (!this || !mp_RCon)
		return FALSE;

	// Saves a copy of our latest render to the png-file (it changes extension automatically to .png)
	DumpImage(m_DC.hDC, m_DC.hBitmap, m_DC.iWidth, m_DC.iHeight, asFile);

	HANDLE hFile = CreateFile(asFile, GENERIC_WRITE, FILE_SHARE_READ,
	                          nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		DisplayLastError(_T("Can't create file!"));
		return FALSE;
	}

	DWORD dw;
	LPCTSTR pszTitle = mp_ConEmu->GetLastTitle();
	WriteFile(hFile, pszTitle, _tcslen(pszTitle)*sizeof(*pszTitle), &dw, nullptr);
	wchar_t temp[100];
	swprintf_c(temp, L"\r\nSize: %ix%i   Cursor: %ix%i\r\n", m_Sizes.TextWidth, m_Sizes.TextHeight, Cursor.x, Cursor.y);
	WriteFile(hFile, temp, wcslen(temp)*sizeof(wchar_t), &dw, nullptr);
	WriteFile(hFile, mpsz_ConChar, m_Sizes.TextWidth * m_Sizes.TextHeight * sizeof(*mpsz_ConChar), &dw, nullptr);
	WriteFile(hFile, mpn_ConAttrEx, m_Sizes.TextWidth * m_Sizes.TextHeight * sizeof(*mpn_ConAttrEx), &dw, nullptr);
	WriteFile(hFile, mpsz_ConCharSave, m_Sizes.TextWidth * m_Sizes.TextHeight * sizeof(*mpsz_ConCharSave), &dw, nullptr);
	WriteFile(hFile, mpn_ConAttrExSave, m_Sizes.TextWidth * m_Sizes.TextHeight * sizeof(*mpn_ConAttrExSave), &dw, nullptr);

	if (mp_RCon)
	{
		mp_RCon->DumpConsole(hFile);
	}

	CloseHandle(hFile);
	return TRUE;
}

bool CVirtualConsole::LoadDumpConsole()
{
	if (!this || !mp_RCon) return false;

	OPENFILENAME ofn; memset(&ofn,0,sizeof(ofn));
	wchar_t temp[MAX_PATH+5];
	ofn.lStructSize=sizeof(ofn);
	ofn.hwndOwner = ghWnd;
	ofn.lpstrFilter = _T("ConEmu dumps (*.con)\0*.con\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = temp; temp[0] = 0;
	ofn.nMaxFile = MAX_PATH;
	ofn.lpstrTitle = L"Load console dump...";
	ofn.lpstrDefExt = L"con";
	ofn.Flags = OFN_ENABLESIZING|OFN_NOCHANGEDIR
	            | OFN_PATHMUSTEXIST|OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

	if (!GetOpenFileName(&ofn)
		|| !mp_RCon->LoadDumpConsole(temp))
	{
		if (mp_RCon->GetActiveBufferType() == rbt_DumpScreen)
			mp_RCon->SetActiveBuffer(rbt_Primary);
		return false;
	}

	return true;
}


void CVirtualConsole::PaintBackgroundImage(HDC hdc, const RECT& rcText, const COLORREF crBack, bool Background /*= false*/)
{
	#ifdef _DEBUG
	bool lbDump = false;
	if (lbDump) DumpImage(hBgDc, nullptr, bgBmpSize.X, bgBmpSize.Y, L"F:\\bgtemp.png");
	#endif

	// If Back is larger than DC (gap or non-integral size)
	const POINT dcOffset = {Background ? 0 : (int)m_Sizes.OffsetX, Background ? 0 : (int)m_Sizes.OffsetY};
	// Where to place bitmap
	const BackgroundOp& op = (BackgroundOp)gpSet->bgOperation;
	// We shall get coordinates relative to our DC due to `op`
	RECT bgRect = {0, 0, bgBmpSize.X, bgBmpSize.Y};
	POINT bgOffset = {};

	if (op == eUpRight)
	{
		bgOffset.x = std::max(0,((int)m_Sizes.BackWidth - bgBmpSize.X));
	}
	else if (op == eDownLeft)
	{
		bgOffset.y = std::max(0,((int)m_Sizes.BackHeight - bgBmpSize.Y));
	}
	else if (op == eDownRight)
	{
		bgOffset.x = std::max(0,((int)m_Sizes.BackWidth - bgBmpSize.X));
		bgOffset.y = std::max(0,((int)m_Sizes.BackHeight - bgBmpSize.Y));
	}
	else if ((op == eFit) || (op == eFill) || (op == eCenter))
	{
		bgOffset.x = ((int)m_Sizes.BackWidth - bgBmpSize.X)/2;
		bgOffset.y = ((int)m_Sizes.BackHeight - bgBmpSize.Y)/2;
	}

	RECT dcMapped = rcText;
	POINT totalOffset = {dcOffset.x - bgOffset.x, dcOffset.y - bgOffset.y};
	OffsetRect(&dcMapped, totalOffset.x, totalOffset.y);

	HRGN hRgnDc = CreateRectRgnIndirect(&rcText);

	// Background part
	RECT blt = {};
	if (IntersectRect(&blt, &dcMapped, &bgRect))
	{
		POINT bltPos = {blt.left - totalOffset.x, blt.top - totalOffset.y};
		BitBlt(hdc, bltPos.x, bltPos.y, RectWidth(blt), RectHeight(blt), hBgDc, blt.left, blt.top, SRCCOPY);

		HRGN hRgnBg = CreateRectRgn(bltPos.x, bltPos.y, bltPos.x + RectWidth(blt), bltPos.y + RectHeight(blt));
		if (hRgnDc && hRgnBg)
			CombineRgn(hRgnDc, hRgnDc, hRgnBg, RGN_DIFF);
		if (hRgnBg)
			DeleteObject(hRgnBg);
	}

	// Fill with requested color (if background image is not large enough)
	if (hRgnDc)
	{
		#ifndef SKIP_ALL_FILLRECT
		HBRUSH hBr = PartBrush(L' ', crBack, crBack);
		if (hBr)
			FillRgn(hdc, hRgnDc, hBr);
		#endif
		DeleteObject(hRgnDc);
	}
}

bool CVirtualConsole::GetUCharMapFontPtr(CFontPtr& pFont)
{
	pFont = m_UCharMapFont.Ptr();
	return pFont.IsSet();
}

void CVirtualConsole::SelectFont(CEFontStyles newFont)
{
	if (newFont == m_SelectedFont)
	{
		// Already selected, nothing to do
		return;
	}

	CFontPtr hFontPtr;

	if (newFont != fnt_NULL)
	{
		if (!gpFontMgr->QueryFont(newFont, this, hFontPtr))
		{
			_ASSERTE(FALSE && "FontMgr failed?");
			// Don't change active font?
			return;
		}
	}

	SelectFont(hFontPtr);
	m_SelectedFont = newFont;
}

void CVirtualConsole::SelectFont(const CFontPtr& hFontPtr)
{
	TODO("Move logic to m_DC?");

	if (!hFontPtr.IsSet())
	{
		m_DC.SelectFont(nullptr);
	}
	else
	{
		#ifdef _DEBUG
		if (hFontPtr == m_UCharMapFont)
			_ASSERTE(hFontPtr.IsSet());
		#endif

		m_DC.SelectFont(hFontPtr);
	}
}

void CVirtualConsole::SelectBrush(HBRUSH hNew)
{
	if (!hNew)
	{
		if (hOldBrush)
		{
			SelectObject((HDC)m_DC, hOldBrush);
		}

		hOldBrush = nullptr;
	}
	else if (hSelectedBrush != hNew)
	{
		hSelectedBrush = (HBRUSH)SelectObject((HDC)m_DC, hNew);

		if (!hOldBrush)
			hOldBrush = hSelectedBrush;

		hSelectedBrush = hNew;
	}
}

bool CVirtualConsole::CheckSelection(const CONSOLE_SELECTION_INFO& select, SHORT row, SHORT col)
{
	TODO("Выделение пока не живет");
	return false;
	//if ((select.dwFlags & CONSOLE_SELECTION_NOT_EMPTY) == 0)
	//    return false;
	//if (row < select.srSelection.Top || row > select.srSelection.Bottom)
	//    return false;
	//if (col < select.srSelection.Left || col > select.srSelection.Right)
	//    return false;
	//return true;
}

#ifdef MSGLOGGER
class DcDebug
{
	public:
		DcDebug(HDC* ahDcVar, HDC* ahPaintDC)
		{
			mb_Attached=FALSE; mh_OrigDc=nullptr; mh_DcVar=nullptr; mh_Dc=nullptr;

			if (!ahDcVar || !ahPaintDC) return;

			mh_DcVar = ahDcVar;
			mh_OrigDc = *ahDcVar;
			mh_Dc = *ahPaintDC;
			*mh_DcVar = mh_Dc;
		};
		~DcDebug()
		{
			if (mb_Attached && mh_DcVar)
			{
				mb_Attached = FALSE;
				*mh_DcVar = mh_OrigDc;
			}
		};
	protected:
		BOOL mb_Attached;
		HDC mh_OrigDc, mh_Dc;
		HDC* mh_DcVar;
};
#endif

// Преобразовать консольный цветовой атрибут (фон+текст) в цвета фона и текста
// (!) Количество цветов текста могут быть расширено за счет одного цвета фона
// atr принудительно в BYTE, т.к. верхние флаги нас не интересуют
//void CVirtualConsole::GetCharAttr(WORD atr, BYTE& foreColorNum, BYTE& backColorNum, HFONT* pFont)
//{
//	// Нас интересует только нижний байт
//    atr &= 0xFF;
//
//    // быстрее будет создать заранее массив с цветами, и получать нужное по индексу
//    foreColorNum = m_ForegroundColors[atr];
//    backColorNum = m_BackgroundColors[atr];
//    if (pFont) *pFont = mh_FontByIndex[atr];
//
//    if (bExtendColors) {
//        if (backColorNum == nExtendColor) {
//            backColorNum = attrBackLast; // фон нужно заменить на обычный цвет из соседней ячейки
//            foreColorNum += 0x10;
//        } else {
//            attrBackLast = backColorNum; // запомним обычный цвет соседней ячейки
//        }
//    }
//}

#if 0
TODO("Move to CFont");
void CVirtualConsole::CharABC(wchar_t ch, ABC *abc)
{
	BOOL lbCharABCOk;

	WARNING("Не работает с *.bdf?");

	if (!gpFontMgr->m_CharABC[ch].abcB)
	{
		if (isCharRTL(ch))
		{
			WARNING("Поскольку с RTL все достаточно сложно, пока считаем шрифт моноширинным");
			gpFontMgr->m_CharABC[ch].abcA = gpFontMgr->m_CharABC[ch].abcC = 0;
			gpFontMgr->m_CharABC[ch].abcB = m_Sizes.nFontWidth;
		}
		else
		{
			if (gpFontMgr->m_Font2.IsSet() && gpSet->isFixFarBorders && isCharAltFont(ch))
			{
				SelectFont(fnt_Alternative);
			}
			else
			{
				TODO("Тут надо бы деление по стилям сделать");
				SelectFont(fnt_Normal);
			}

			//This function succeeds only with TrueType fonts
			lbCharABCOk = GetCharABCWidths((HDC)m_DC, ch, ch, &gpFontMgr->m_CharABC[ch]);

			if (!lbCharABCOk)
			{
				// Значит шрифт не TTF/OTF
				CharAttr nilAttr = {};
				gpFontMgr->m_CharABC[ch].abcB = CharWidth(ch, nilAttr);
				_ASSERTE(gpFontMgr->m_CharABC[ch].abcB);

				if (!gpFontMgr->m_CharABC[ch].abcB)
					gpFontMgr->m_CharABC[ch].abcB = 1;

				gpFontMgr->m_CharABC[ch].abcA = gpFontMgr->m_CharABC[ch].abcC = 0;
			}
		}
	}

	*abc = gpFontMgr->m_CharABC[ch];
}
#endif

TODO("Replace wchar_t with ucs32");
// Возвращает ширину символа, учитывает FixBorders
WORD CVirtualConsole::CharWidth(wchar_t ch, const CharAttr& attr)
{
	if (attr.Flags2 & (CharAttr2_NonSpacing|CharAttr2_Combining))
	{
		return 0;
	}
	else if (attr.Flags2 & CharAttr2_DoubleSpaced)
	{
		return MakeUShort(2 * m_Sizes.nFontWidth);
	}
	else if (gpSet->isEnhanceGraphics && isCharProgress(ch))
	{
		// Font mapper is not used in this case
		return MakeUShort(m_Sizes.nFontWidth);
	}
	else if (isCharSpaceSingle(ch))
	{
		// mitigate font bugs
		return MakeUShort(m_Sizes.nFontWidth);
	}

	TODO("fnt_UCharMap, m_UCharMapFont");

	TODO("Use FontLinks to acquire proper font!");

	CEFontStyles newFont = fnt_Normal;
	if (gpSet->isFixFarBorders && isCharAltFont(ch))
		newFont = fnt_Alternative;

	CFontPtr hFontPtr;
	if (!gpFontMgr->QueryFont(newFont, this, hFontPtr))
	{
		_ASSERTE(FALSE && "Font must be created already");
		return MakeUShort(m_Sizes.nFontWidth);
	}

	TODO("Surrogates!");
	ucs32 wwch = ch;
	char_width_map::const_iterator exist = hFontPtr->m_CharWidth.find(wwch);
	if (exist != hFontPtr->m_CharWidth.end())
	{
		return exist->second;
	}

	SelectFont(hFontPtr);

	SIZE sz; WORD nWidth;
	if (m_DC.TextExtentPoint(&ch, 1, &sz) && sz.cx)
		nWidth = MakeUShort(sz.cx);
	else
		nWidth = MakeUShort(m_Sizes.nFontWidth);

	if (!nWidth)
		nWidth = 1; // JIC, avoid devision by zero

	// Store in font hash map
	hFontPtr->m_CharWidth[wwch] = nWidth;

	return nWidth;
}

bool CVirtualConsole::CheckChangedTextAttr()
{
	bool lbChanged = false;
	RECT conLocked;
	int nLocked = IsDcLocked(&conLocked);

	if ((isForce || m_HighlightInfo.mb_ChangeDetected) && !nLocked)
	{
		lbChanged = textChanged = attrChanged = true;
	}
	else
	{
		textChanged = 0!=memcmp(mpsz_ConChar, mpsz_ConCharSave, TextLen * sizeof(*mpsz_ConChar));
		attrChanged = 0!=memcmp(mpn_ConAttrEx, mpn_ConAttrExSave, TextLen * sizeof(*mpn_ConAttrEx));

		if ((lbChanged = (textChanged || attrChanged)))
		{
			if (nLocked)
			{
				// Нужно проверить, были ли изменения в conLocked
				if ((UINT)conLocked.right >= m_Sizes.TextWidth || (UINT)conLocked.bottom >= m_Sizes.TextHeight)
				{
					// Хм, по идее, уже должен был быть сброшен при ресайзе консоли
					_ASSERTE((UINT)conLocked.right < m_Sizes.TextWidth && (UINT)conLocked.bottom < m_Sizes.TextHeight);
					LockDcRect(false);
				}
				else
				{
					wchar_t* pSpaces = nullptr;
					bool lbLineChanged;
					size_t nChars = conLocked.right - conLocked.left + 1;
					if (nLocked == 1)
					{
						pSpaces = (wchar_t*)malloc((nChars+1)*sizeof(*pSpaces));
						wmemset(pSpaces, L' ', nChars);
						pSpaces[nChars] = 0;
					}

					for (int Y = conLocked.top; Y <= conLocked.bottom; Y++)
					{
						size_t nShift = Y * m_Sizes.TextWidth + conLocked.left;
						lbLineChanged = 0!=memcmp(mpsz_ConChar+nShift, mpsz_ConCharSave+nShift, nChars * sizeof(*mpsz_ConChar));

						if (lbLineChanged)
						{
							if (nLocked == 1)
							{
								// Игнорировать заполнение пробелами
								if (0==memcmp(mpsz_ConChar+nShift, pSpaces, nChars * sizeof(*mpsz_ConChar)))
									continue;
							}
							// При любых изменениях - сброс
							LockDcRect(false);
							break;
						}

						TODO("Можно бы еще и атрибуты проверять, но это практически всегда избыточно");
					}

					if (pSpaces)
						free(pSpaces);
				}
			}
		}
		else
		{
			lbChanged = isForce;
		}
	}

	return lbChanged;

//#ifdef MSGLOGGER
//    COORD ch;
//    if (textChanged) {
//        for (UINT i=0; i<TextLen; i++) {
//            if (mpsz_ConChar[i] != mpsz_ConCharSave[i]) {
//                ch.Y = i % TextWidth;
//                ch.X = i - TextWidth * ch.Y;
//                break;
//            }
//        }
//    }
//    if (attrChanged) {
//        for (UINT i=0; i<TextLen; i++) {
//            if (mpn_ConAttr[i] != mpn_ConAttrSave[i]) {
//                ch.Y = i % TextWidth;
//                ch.X = i - TextWidth * ch.Y;
//                break;
//            }
//        }
//    }
//#endif
//	return textChanged || attrChanged;
}

void CVirtualConsole::UpdateThumbnail(bool abNoSnapshot /*= FALSE*/)
{
	if (!this)
		return;

	if (!mp_Ghost || !mp_ConEmu->Taskbar_GhostSnapshotRequired())
		return;

	// Скорее всего мы не в главной нити, но оно само разберется
	mp_Ghost->UpdateTabSnapshot(FALSE, abNoSnapshot);
}

bool CVirtualConsole::Update(bool abForce, HDC *ahDc)
{
	if (!this || !mp_RCon || !mp_RCon->ConWnd())
		return false;

	isForce = abForce;

	if (!isMainThread())
	{
		if (isForce)
			mb_RequiredForceUpdate = true;

		//if (mb_RequiredForceUpdate || updateText || updateCursor)
		{
			if (isVisible())
			{
				if (gpSet->isLogging(3)) mp_RCon->LogString("Invalidating from CVirtualConsole::Update.1");

				Invalidate();
			}

			return true;
		}
		return false;
	}

	/* -- 2009-06-20 переделал. графика теперь крутится ТОЛЬКО в MainThread
	if (!mp_RCon->IsConsoleThread()) {
		if (!ahDc) {
			mp_RCon->SetForceRead();
			mp_RCon->WaitEndUpdate(1000);
			//mp_ConEmu->InvalidateAll(); -- зачем на All??? Update и так Invalidate зовет
			return false;
		}
	}
	*/

	if (mb_InUpdate)
	{
		// Не должен вызываться по рекурсии
		if (!mb_InConsoleResize)
		{
			_ASSERTE(!mb_InUpdate);
		}

		return false;
	}

	MSetter inUpdate(&mb_InUpdate);
	// Рисуем актуальную информацию из CRealConsole. ЗДЕСЬ запросы в консоль не делаются.
	//RetrieveConsoleInfo();
	#if defined(_DEBUG) && defined(MSGLOGGER)
	DcDebug dbg(&m_DC.hDC, ahDc); // для отладки - рисуем сразу на канвасе окна
	#endif

	HEAPVAL;
	bool lRes = false;
	MSectionLock SCON; SCON.Lock(&csCON);
	//MSectionLock SDC; //&csDC, &ncsTDC);
	_ASSERTE(isMainThread());

	//if (mb_PaintRequested) -- не должно быть. Эта функция работает ТОЛЬКО в консольной нити
	//if (mb_PaintLocked)  // Значит идет асинхронный PaintVCon (BitBlt) - это может быть во время ресайза, или над окошком что-то протащили
	//	SDC.Lock(&csDC, TRUE, 200); // но по таймауту, чтобы не повисли ненароком

	mp_RCon->GetConsoleScreenBufferInfo(&csbi);
	// start timer before "Read Console Output*" calls, they do take time
	//gpSetCls->Performance(tPerfRead, FALSE);
	//if (gbNoDblBuffer) isForce = TRUE; // Debug, dblbuffer
	isForeground = (mp_ConEmu->InQuakeAnimation() || mp_ConEmu->isMeForeground(true))
		&& isActive(false);

	if (isFade == isForeground && gpSet->isFadeInactive)
		isForce = true;

	//------------------------------------------------------------------------
	///| Read console output and cursor info... |/////////////////////////////
	//------------------------------------------------------------------------
	if (!UpdatePrepare(ahDc, nullptr/*&SDC*/, &SCON))
	{
		mp_ConEmu->DebugStep(_T("DC initialization failed!"));
		return false;
	}

	//gpSetCls->Performance(tPerfRead, TRUE);
	gpSetCls->Performance(tPerfRender, FALSE);
	//------------------------------------------------------------------------
	///| Drawing text (if there were changes in console) |////////////////////
	//------------------------------------------------------------------------
	bool updateText, updateCursor;

	// Do we have to update changed text? take isForce into account
	updateText = CheckChangedTextAttr();

	if (isForce)
		updateCursor = attrChanged = textChanged = true;
	// Do we have to update text under changed cursor?
	// Important: check both 'cinf.bVisible' and 'Cursor.isVisible',
	// because console may have cursor hidden and its position changed -
	// in this case last visible cursor remains shown at its old place.
	// Also, don't check foreground window here for the same reasons.
	// If position is the same then check the cursor becomes hidden.
	else if (Cursor.x != csbi.dwCursorPosition.X || Cursor.y != csbi.dwCursorPosition.Y)
		// сменилась позиция. обновляем если курсор видим
		updateCursor = cinf.bVisible || Cursor.isVisible || Cursor.isVisiblePrevFromInfo;
	else
		updateCursor = Cursor.isVisiblePrevFromInfo && !cinf.bVisible;

	mb_DialogsChanged = CheckDialogsChanged();
	// Если на панелях открыт ConEmuTh - причесать заголовки и
	// строку разделителя инф.части (там не должно оставаться угловых элементов)
	// И обновить видимые регионы (скрыть части, которые находятся "под" диалогами)
	PolishPanelViews();

	//gpSetCls->Performance(tPerfRender, FALSE);

	// For optimization - remove highlights if they was painted
	if (!isForce)
	{
		UndoHighlights();
	}

	if (updateText /*|| updateCursor*/)
	{
		lRes = true;
		DEBUGSTRDRAW(L" +++ updateText detected in VCon\n");
		//gpSetCls->Performance(tPerfRender, FALSE);
		//------------------------------------------------------------------------
		///| Drawing modified text |//////////////////////////////////////////////
		//------------------------------------------------------------------------
		UpdateText();
		//gpSetCls->Performance(tPerfRender, TRUE);
		//HEAPVAL
		//------------------------------------------------------------------------
		///| Now, store data for further comparison |/////////////////////////////
		//------------------------------------------------------------------------
		memcpy(mpsz_ConCharSave, mpsz_ConChar, TextLen * sizeof(*mpsz_ConChar));
		memcpy(mpn_ConAttrExSave, mpn_ConAttrEx, TextLen * sizeof(*mpn_ConAttrEx));
	}

	//-- перенесено в PaintVCon
	//// Если зарегистрированы панели (ConEmuTh) - обновить видимые регионы
	//// Делать это нужно после UpdateText, потому что иначе ConCharX может быть обнулен
	//if (m_LeftPanelView.bRegister || m_RightPanelView.bRegister) {
	//	if (mb_DialogsChanged) {
	//		UpdatePanelRgn(TRUE);
	//		UpdatePanelRgn(FALSE);
	//	}
	//}
	//HEAPVAL
	//------------------------------------------------------------------------
	///| Checking cursor |////////////////////////////////////////////////////
	//------------------------------------------------------------------------
	UpdateCursor(lRes);
	HEAPVAL;
	//SDC.Leave();
	SCON.Unlock();

	bool bVisible = isVisible();

	// Highlight row/col under mouse cursor
	if (bVisible && isHighlightAny())
	{
		UpdateHighlights();
	}

	// После успешного обновления внутренних буферов (символы/атрибуты)
	//
	if (lRes && bVisible)
	{
		if (mpsz_LogScreen && mp_RCon && mp_RCon->GetServerPID())
		{
			// Скинуть буферы в лог
			mn_LogScreenIdx++;
			wchar_t szLogPath[MAX_PATH]; swprintf_c(szLogPath, mpsz_LogScreen, mp_RCon->GetServerPID(), mn_LogScreenIdx);
			Dump(szLogPath);
		}

		// Если допустимо - передернуть обновление viewport'а
		if (!mb_InPaintCall)
		{
			// должен вызываться в основной нити
			_ASSERTE(isMainThread());
			//mb_PaintRequested = TRUE;
			Invalidate();

			if (gpSet->isLogging(3)) mp_RCon->LogString("Invalidating from CVirtualConsole::Update.2");

			//09.06.13 а если так? быстрее изменения на экране не появятся?
			//UpdateWindow('ghWnd DC'); // оно посылает сообщение в окно, и ждет окончания отрисовки
			#ifdef _DEBUG
			//_ASSERTE(!mp_ConEmu->m_Child->mb_Invalidated);
			#endif
			//mb_PaintRequested = FALSE;
		}
	}

	m_HighlightInfo.mb_ChangeDetected = false;

	gpSetCls->Performance(tPerfRender, TRUE);
	/* ***************************************** */
	/*       Finalization, release objects       */
	/* ***************************************** */
	SelectBrush(nullptr);

	/*
	for (UINT br=0; br<m_PartBrushes.size(); br++) {
		DeleteObject(m_PartBrushes[br].hBrush);
	}
	m_PartBrushes.clear();
	*/
	SelectFont(fnt_NULL);
	HEAPVAL;
	return lRes;
}

void CVirtualConsole::PatInvertRect(HDC hPaintDC, const RECT& rect, HDC hFromDC, bool bFill)
{
	if (bFill)
	{
		BitBlt(hPaintDC, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, hFromDC, 0,0,
				PATINVERT);
	}
	else
	{
		BitBlt(hPaintDC, rect.left, rect.top, 1, rect.bottom-rect.top, hFromDC, 0,0,
				PATINVERT);
		BitBlt(hPaintDC, rect.left, rect.top, rect.right-rect.left, 1, hFromDC, 0,0,
				PATINVERT);
		BitBlt(hPaintDC, rect.right-1, rect.top, 1, rect.bottom-rect.top, hFromDC, 0,0,
				PATINVERT);
		BitBlt(hPaintDC, rect.left, rect.bottom-1, rect.right-rect.left, 1, hFromDC, 0,0,
				PATINVERT);
	}
}

void CVirtualConsole::ResetHighlightCoords()
{
	ResetHighlightHyperlinks();
	m_HighlightInfo.LastRect.clear();
	m_HighlightInfo.m_Last.X = m_HighlightInfo.m_Last.Y = -1;
	m_HighlightInfo.m_Cur.X = m_HighlightInfo.m_Cur.Y = -1;
}

void CVirtualConsole::ResetHighlightHyperlinks()
{
	ZeroStruct(m_etr);
}

bool CVirtualConsole::isHighlightAny()
{
	return (isHighlightHyperlink() || isHighlightMouseRow() || isHighlightMouseCol());
}

bool CVirtualConsole::isHighlightMouseRow()
{
	return m_HighlightInfo.mb_SelfSettings ? m_HighlightInfo.mb_HighlightRow : gpSet->isHighlightMouseRow;
}

bool CVirtualConsole::isHighlightMouseCol()
{
	return m_HighlightInfo.mb_SelfSettings ? m_HighlightInfo.mb_HighlightCol : gpSet->isHighlightMouseCol;
}

bool CVirtualConsole::isHighlightHyperlink()
{
	return (m_etr.etrLast != etr_None);
}

void CVirtualConsole::ChangeHighlightMouse(int nWhat, int nSwitch)
{
	if (!m_HighlightInfo.mb_SelfSettings)
	{
		m_HighlightInfo.mb_SelfSettings = true;
		m_HighlightInfo.mb_HighlightRow = gpSet->isHighlightMouseRow;
		m_HighlightInfo.mb_HighlightCol = gpSet->isHighlightMouseCol;
	}

	if (nWhat <= 0)
	{
		// GuiMacro ‘HighlightMouse(0)’ would change highlighting state cyclically
		if (!m_HighlightInfo.mb_HighlightRow && !m_HighlightInfo.mb_HighlightCol)
		{
			m_HighlightInfo.mb_HighlightRow = true;
		}
		else if (m_HighlightInfo.mb_HighlightRow && !m_HighlightInfo.mb_HighlightCol)
		{
			m_HighlightInfo.mb_HighlightRow = false; m_HighlightInfo.mb_HighlightCol = true;
		}
		else if (!m_HighlightInfo.mb_HighlightRow && m_HighlightInfo.mb_HighlightCol)
		{
			m_HighlightInfo.mb_HighlightRow = m_HighlightInfo.mb_HighlightCol = true;
		}
		else
		{
			m_HighlightInfo.mb_HighlightRow = m_HighlightInfo.mb_HighlightCol = false;
		}
	}
	else
	{
		// nWhat is bitmask: 1 = row, 2 = col
		for (int i = 1; i <= 2; i++)
		{
			if (!(nWhat & i))
				continue;
			// And what to change
			bool& bOpt = (i == 1) ? m_HighlightInfo.mb_HighlightRow : m_HighlightInfo.mb_HighlightCol;
			switch (nSwitch)
			{
			case 0:
				bOpt = false; break;
			case 1:
				bOpt = true;  break;
			case 2:
				bOpt = !bOpt; break;
			}
		}
	}

	// Set same state to all visible panes
	struct impl
	{
		bool bHighlightRow, bHighlightCol;
		static bool SetOtherTheSame(CVirtualConsole* pVCon, LPARAM lParam)
		{
			pVCon->m_HighlightInfo.mb_SelfSettings = true;
			pVCon->m_HighlightInfo.mb_HighlightRow = ((impl*)lParam)->bHighlightRow;
			pVCon->m_HighlightInfo.mb_HighlightCol = ((impl*)lParam)->bHighlightCol;
			pVCon->Invalidate();
			return true;
		}
	} Impl = {m_HighlightInfo.mb_HighlightRow, m_HighlightInfo.mb_HighlightCol};
	CVConGroup::EnumVCon(evf_Visible, impl::SetOtherTheSame, (LPARAM)&Impl);

	// Redraw all consoles
	Update(true);
}

// This is called from TrackMouse. It must NOT trigger more than one invalidate before next repaint
bool CVirtualConsole::WasHighlightRowColChanged()
{
	bool bChanged = false;
	bool bInvalidate = false;
	COORD crPos = {-1,-1};

	if (isHighlightHyperlink() && (mp_RCon->isSelectionPresent() || !mp_RCon->IsFarHyperlinkAllowed(false)))
	{
		m_etr.etrLast = etr_None;
		bInvalidate = true;
	}

	// Invalidate already pended?
	if (m_HighlightInfo.mb_ChangeDetected)
	{
		goto wrap;
	}

	// If cursor goes out of VCon - leave row/col marks?
	if (!CalcHighlightRowCol(&crPos))
	{
		for (INT_PTR i = 0; !bInvalidate && i < m_HighlightInfo.LastRect.size(); ++i)
		{
			if (!m_HighlightInfo.LastRect[i].crPatColor)
				continue;
			bInvalidate = true;
		}
		goto wrap;
	}

	if ((isHighlightMouseRow() && (crPos.Y != m_HighlightInfo.m_Last.Y))
		|| (isHighlightMouseCol() && (crPos.X != m_HighlightInfo.m_Last.X)))
	{
		// Refresh our state
		m_HighlightInfo.mb_ChangeDetected = true;
		bInvalidate = true;

		// Tell the caller the state was changed
		bChanged = true;
		goto wrap;
	}

wrap:
	if (bInvalidate)
	{
		Invalidate();
	}

	// Apply to all visible panes
	if (isActive(false) && isGroup())
	{
		struct impl
		{
			CVirtualConsole* pSelf;
			static bool RefreshOthers(CVirtualConsole* pVCon, LPARAM lParam)
			{
				if (pVCon != ((impl*)lParam)->pSelf)
					pVCon->WasHighlightRowColChanged();
				return true;
			}
		} Impl = {this};
		CVConGroup::EnumVCon(evf_Visible, impl::RefreshOthers, (LPARAM)&Impl);
	}

	return bChanged;
}

bool CVirtualConsole::CalcHighlightRowCol(COORD* pcrPos)
{
	if (!(isHighlightMouseRow() || isHighlightMouseCol())
		//|| !mp_ConEmu->isMeForeground() // show highlights even if ConEmu loses focus
		|| !isVisible())
	{
		m_HighlightInfo.mb_Exists = false;
		m_HighlightInfo.m_Cur.X = m_HighlightInfo.m_Cur.Y = -1;
		return false;
	}

	// Get SCREEN coordinates
	bool  bExist = false;
	BOOL  bCanChange = FALSE;
	COORD pos = {-1,-1};
	POINT ptCursor = {};
	RECT  rcDC = {};
	RECT  rcWork = {};

	// When to skip?
	if (!mp_ConEmu->isMenuActive()
		// may be it is not handy, let check if Cursor is inside ConEmu window?
		// && mp_ConEmu->isMeForeground(false, false)
		)
	{
		GetCursorPos(&ptCursor);
		GetWindowRect(mh_WndDC, &rcDC);
		GetWindowRect(ghWndWork, &rcWork);
		bCanChange = PtInRect(&rcWork, ptCursor);
	}

	if (!bCanChange)
	{
		// Do not drop highlights when cursor is out of VCon. Annoying behavior with splits...
		// So, return last detected (saved) state of highlights
		bExist = m_HighlightInfo.mb_Exists;
		pos = m_HighlightInfo.m_Cur;
	}
	else
	{
		// Get COORDs (relative to upper-left visible pos)
		pos = ClientToConsole(ptCursor.x-rcDC.left, ptCursor.y-rcDC.top);
		// If cursor is outside of VCon, but fit with one of axis
		if ((ptCursor.x < rcDC.left) || (ptCursor.x > rcDC.right))
			pos.X = -1;
		if ((ptCursor.y < rcDC.top) || (ptCursor.y > rcDC.bottom))
			pos.Y = -1;
		// And store
		m_HighlightInfo.m_Cur = pos;
		m_HighlightInfo.mb_Exists = bExist = (pos.X >= 0 || pos.Y >= 0);
	}

	if (bExist && pcrPos)
		*pcrPos = pos;
	return bExist;
}

void CVirtualConsole::UpdateHighlights()
{
	m_HighlightInfo.LastRect.clear();
	m_HighlightInfo.m_Last.X = m_HighlightInfo.m_Last.Y = -1;

	// During any popup menus (system, tab, etc.) don't highlight row/col
	if (mp_ConEmu->isMenuActive())
		return;

	if (isHighlightMouseRow() || isHighlightMouseCol())
		UpdateHighlightsRowCol();

	if (isHighlightHyperlink())
		UpdateHighlightsHyperlink();
}

void CVirtualConsole::UpdateHighlightsRowCol()
{
	_ASSERTE(this && (isHighlightMouseRow() || isHighlightMouseCol()));
	_ASSERTE(isVisible());

	// Get COORDs (relative to upper-left visible pos)
	COORD pos = {-1,-1};
	if (!CalcHighlightRowCol(&pos))
		return;

	// And MARK! (nFontHeight or cell (nFontWidth))

	COORD pix;
	pix.X = MakeShort(pos.X * m_Sizes.nFontWidth);
	pix.Y = MakeShort(pos.Y * m_Sizes.nFontHeight);

	if ((pos.X >= 0) && ConCharX)
	{
		int CurChar = std::max(0, std::min((int)pos.Y, (int)m_Sizes.TextHeight-1)) * m_Sizes.TextWidth + pos.X;
		if ((CurChar >= 0) && (CurChar < (int)(m_Sizes.TextWidth * m_Sizes.TextHeight)))
		{
			if (ConCharX[CurChar-1])
				pix.X = MakeShort(ConCharX[CurChar-1]);
		}
	}

	HDC hPaintDC = (HDC)m_DC;
	HighlightRect hrPaint = {};
	hrPaint.crPatColor = HILIGHT_PAT_COLOR;
	HBRUSH hBr = CreateSolidBrush(hrPaint.crPatColor);
	HBRUSH hOld = (HBRUSH)SelectObject(hPaintDC, hBr);

	_ASSERTE(m_HighlightInfo.LastRect.empty());

	if (isHighlightMouseRow())
	{
		hrPaint.rcPaint = MakeRect(0, pix.Y, (LONG)m_Sizes.Width, pix.Y + m_Sizes.nFontHeight);
		if (pos.Y >= 0)
			PatInvertRect(hPaintDC, hrPaint.rcPaint, hPaintDC, false);
		m_HighlightInfo.m_Last.Y = pos.Y;
		m_HighlightInfo.LastRect.push_back(hrPaint);
	}

	if (isHighlightMouseCol())
	{
		// This will be not "precise" on other rows if using proportional font...
		hrPaint.rcPaint = MakeRect(pix.X, 0, pix.X + m_Sizes.nFontWidth, (LONG)m_Sizes.Height);
		if (pos.X >= 0)
			PatInvertRect(hPaintDC, hrPaint.rcPaint, hPaintDC, false);
		m_HighlightInfo.m_Last.X = pos.X;
		m_HighlightInfo.LastRect.push_back(hrPaint);
	}

	SelectObject(hPaintDC, hOld);
	DeleteObject(hBr);
}

void CVirtualConsole::UpdateHighlightsHyperlink()
{
	_ASSERTE(this && isHighlightHyperlink());

	HDC hPaintDC = (HDC)m_DC;
	HighlightRect hrPaint = {};
	hrPaint.StockBrush = WHITE_BRUSH;
	HBRUSH hBr = (HBRUSH)GetStockObject(hrPaint.StockBrush);
	HBRUSH hOld = (HBRUSH)SelectObject(hPaintDC, hBr);

	// Support multi-line hyperlinks
	int nWidth = GetTextWidth();
	// Height of "underline"?
	int nHeight = m_Sizes.nFontHeight / 10;
	if (nHeight < 1) nHeight = 1;

	for (SHORT Y = m_etr.mcr_FileLineStart.Y; Y <= m_etr.mcr_FileLineEnd.Y; ++Y)
	{
		POINT ptStart = ConsoleToClient((Y > m_etr.mcr_FileLineStart.Y) ? 0 : m_etr.mcr_FileLineStart.X, Y);
		POINT ptEnd = ConsoleToClient((Y < m_etr.mcr_FileLineEnd.Y) ? nWidth : m_etr.mcr_FileLineEnd.X+1, Y);
		// Just fill it (with color of the text?)
		hrPaint.rcPaint = MakeRect(ptStart.x, ptStart.y + m_Sizes.nFontHeight-nHeight, ptEnd.x, ptEnd.y + m_Sizes.nFontHeight);

		PatInvertRect(hPaintDC, hrPaint.rcPaint, hPaintDC, true);
		//FillRect((HDC)m_DC, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

		m_HighlightInfo.LastRect.push_back(hrPaint);
	}

	SelectObject(hPaintDC, hOld);
}

void CVirtualConsole::UndoHighlights()
{
	if (m_HighlightInfo.LastRect.empty())
		return;

	HDC hPaintDC = (HDC)m_DC;

	for (INT_PTR i = 0; i < m_HighlightInfo.LastRect.size(); ++i)
	{
		const HighlightRect& cur = m_HighlightInfo.LastRect[i];
		bool UseFill = (cur.crPatColor == 0);
		HBRUSH hBr = UseFill ? (HBRUSH)GetStockObject(cur.StockBrush) : CreateSolidBrush(cur.crPatColor);
		HBRUSH hOld = (HBRUSH)SelectObject(hPaintDC, hBr);

		PatInvertRect(hPaintDC, cur.rcPaint, hPaintDC, UseFill);

		SelectObject(hPaintDC, hOld);
		if (!UseFill)
			DeleteObject(hBr);
	}

	m_HighlightInfo.LastRect.clear();
}

bool CVirtualConsole::CheckTransparent()
{
	AssertThisRet(false);

	bool lbChanged = FALSE;
	static LONG llInCheckTransparent = false;

	if (llInCheckTransparent)
	{
		_ASSERTE(llInCheckTransparent==0);
		return FALSE;
	}

	bool lbHasChildWindows = false;
	if (mp_RCon)
	{
		// Если работа идет в GUI режиме (notepad во вкладке ConEmu)
		if (mp_RCon->GuiWnd() != nullptr)
			lbHasChildWindows = true;
		// Или включен просмотр картинок/видефайлов
		if (mp_RCon->isPictureView())
			lbHasChildWindows = true;
	}

	InterlockedIncrement(&llInCheckTransparent);

	// Проверим условия, в которых вообще не нужно проверять регионы
	if (gpSet->isUserScreenTransparent)
	{
		// Если была смена стиля: был дырявый регион, не стало дырявого региона
		if (CheckTransparentRgn(lbHasChildWindows))
		{
			lbChanged = TRUE;
		}
	}

	// Если сменился статус наличия внедренного PicView
	if (mb_ChildWindowWasFound != lbHasChildWindows)
	{
		mb_ChildWindowWasFound = lbHasChildWindows;
		// нужно передернуть общую альфа-прозрачность окна
		mp_ConEmu->OnTransparent();
	}

	InterlockedDecrement(&llInCheckTransparent);

	return lbChanged;
}

bool CVirtualConsole::CheckTransparentRgn(bool abHasChildWindows)
{
	bool lbRgnChanged = false;
	bool lbTransparentChanged = false;


	// Если изменены данные, или была прозрачность и появились дочерние окна (или наоборот)
	if (mb_ConDataChanged || !((mh_TransparentRgn != nullptr) ^ abHasChildWindows))
	{
		mb_ConDataChanged = false;

		// (пере)Создать регион
		if (mpsz_ConChar && mpn_ConAttrEx && m_Sizes.TextWidth && m_Sizes.TextHeight)
		{
			MSectionLock SCON; SCON.Lock(&csCON);
			CharAttr* pnAttr = mpn_ConAttrEx;
			int nFontHeight = gpFontMgr->FontHeight();
			int nMaxRects = m_Sizes.TextHeight * 5;
			//#ifdef _DEBUG
			//nMaxRects = 5;
			//#endif
			POINT *lpAllPoints = (POINT*)VAlloc(nMaxRects*4,sizeof(POINT)); // 4 угла
			//INT   *lpAllCounts = (INT*)VAlloc(nMaxRects,sizeof(INT));
			HEAPVAL;
			int    nRectCount = 0;

			if (lpAllPoints /*&& lpAllCounts*/)
			{
				POINT *lpPoints = lpAllPoints;
				//INT   *lpCounts = lpAllCounts;

				if (!abHasChildWindows)
				{
					for(unsigned nY = 0; nY < m_Sizes.TextHeight; nY++)
					{
						unsigned nX = 0;
						//int nYPix1 = nY * nFontHeight;

						while (nX < m_Sizes.TextWidth)
						{
							// Найти первый прозрачный cell
							#if 0
							while (nX < TextWidth && !pnAttr[nX].bTransparent)
								nX++;
							#else
							WARNING("CharAttr_Transparent");
							// хорошо бы требуемые проверки прозрачности делать здесь, а не в RgnDetect
							while (nX < m_Sizes.TextWidth && !(pnAttr[nX].Flags & CharAttr_Transparent))
								nX++;
							#endif

							if (nX >= m_Sizes.TextWidth)
								break;

							// Найти конец прозрачного блока
							unsigned nTranStart = nX;

							#if 0
							while (++nX < TextWidth && pnAttr[nX].bTransparent)
								;
							#else
							WARNING("CharAttr_Transparent");
							// хорошо бы требуемые проверки прозрачности делать здесь, а не в RgnDetect
							while (++nX < m_Sizes.TextWidth && (pnAttr[nX].Flags & CharAttr_Transparent))
								;
							#endif

							// Сформировать соответствующий Rect
							//int nXPix = 0;

							if (nRectCount>=nMaxRects)
							{
								int nNewMaxRects = nMaxRects + std::max((int)m_Sizes.TextHeight,(int)(nRectCount-nMaxRects+1));
								_ASSERTE(nNewMaxRects > nRectCount);

								HEAPVAL;
								POINT *lpTmpPoints = (POINT*)VAlloc(nNewMaxRects*4,sizeof(POINT)); // 4 угла
								HEAPVAL;

								if (!lpTmpPoints)
								{
									_ASSERTE(lpTmpPoints);
									VFree(lpAllPoints);
									return false;
								}

								memmove(lpTmpPoints, lpAllPoints, nMaxRects*4*sizeof(POINT));
								HEAPVAL;
								lpPoints = lpTmpPoints + (lpPoints - lpAllPoints);
								VFree(lpAllPoints);
								lpAllPoints = lpTmpPoints;
								nMaxRects = nNewMaxRects;
								HEAPVAL;
							}

							nRectCount++;
	#ifdef _DEBUG

							if (nRectCount>=nMaxRects)
							{
								nRectCount = nRectCount;
							}

	#endif
							//*(lpCounts++) = 4;
							lpPoints[0] = ConsoleToClient(nTranStart, nY);
							lpPoints[1] = ConsoleToClient(nX, nY);
							// x - 1?
							//if (lpPoints[1].x > lpPoints[0].x) lpPoints[1].x--;
							lpPoints[2] = lpPoints[1]; lpPoints[2].y += m_Sizes.nFontHeight;
							lpPoints[3] = lpPoints[0]; lpPoints[3].y = lpPoints[2].y;
							lpPoints += 4;
							nX++;
							HEAPVAL;
						}

						pnAttr += m_Sizes.TextWidth;
					}
				}

				HEAPVAL;
				lbRgnChanged = (nRectCount != TransparentInfo.nRectCount);
				lbTransparentChanged = ((nRectCount==0) != (TransparentInfo.nRectCount==0));

				if (!lbRgnChanged && TransparentInfo.nRectCount)
				{
					_ASSERTE(TransparentInfo.pAllPoints && TransparentInfo.pAllCounts);
					if (nRectCount > nMaxRects)
					{
						_ASSERTE(nRectCount <= nMaxRects); // Must not be, we (re)allocate enough rectangles
						nRectCount = nMaxRects;
					}
					lbRgnChanged = memcmp(TransparentInfo.pAllPoints, lpAllPoints, sizeof(POINT)*4*nRectCount)!=0;
				}

				HEAPVAL;

				if (lbRgnChanged)
				{
					if (mh_TransparentRgn) { DeleteObject(mh_TransparentRgn); mh_TransparentRgn = nullptr; }

					INT   *lpAllCounts = nullptr;

					if (nRectCount > 0)
					{
						lpAllCounts = (INT*)VAlloc(nRectCount,sizeof(INT));
						INT   *lpCounts = lpAllCounts;

						for(int n = 0; n < nRectCount; n++)
							*(lpCounts++) = 4;

						mh_TransparentRgn = CreatePolyPolygonRgn(lpAllPoints, lpAllCounts, nRectCount, WINDING);
					}

					HEAPVAL;

					VFree(TransparentInfo.pAllCounts);

					HEAPVAL;
					TransparentInfo.pAllCounts = lpAllCounts; lpAllCounts = nullptr;

					VFree(TransparentInfo.pAllPoints);

					HEAPVAL;
					TransparentInfo.pAllPoints = lpAllPoints; lpAllPoints = nullptr;
					HEAPVAL;
					TransparentInfo.nRectCount = nRectCount;
				}

				HEAPVAL;

				VFree(lpAllPoints);

				HEAPVAL;
			}
		}
	}

	if (lbRgnChanged && isVisible())
	{
		mp_ConEmu->UpdateWindowRgn();
	}

	return lbTransparentChanged;
}

bool CVirtualConsole::LoadConsoleData()
{
	if (!this)
	{
		_ASSERTE(this!=nullptr);
		return false;
	}

	WARNING("Поскольку тут мы с буфером работаем, хорошо бы выполнять только в главном потоке");

	// Может быть, если консоль неактивна, т.к. CVirtualConsole::Update не вызывается и диалоги не детектятся. А это требуется.
	if (!mpsz_ConChar || !mpn_ConAttrEx)
	{
		// Если при старте ConEmu создано несколько консолей через '@'
		// то все кроме активной - не инициализированы (InitDC не вызывался),
		// что нужно делать в главной нити,
		// PostMessage(ghWnd, mn_MsgInitInactiveDC, 0, apVCon) об этом позаботится
		mp_ConEmu->InitInactiveDC(this);
		return false;
	}

	gpSetCls->Performance(tPerfData, FALSE);
	{
		#ifdef SHOWDEBUGSTEPS
		mp_ConEmu->DebugStep(L"mp_RCon->GetConsoleData");
		#endif

		mp_RCon->GetConsoleData(mpsz_ConChar, mpn_ConAttrEx, m_Sizes.TextWidth, m_Sizes.TextHeight, m_etr); //TextLen*2);

		#ifdef SHOWDEBUGSTEPS
		mp_ConEmu->DebugStep(nullptr);
		#endif
	}
	SMALL_RECT rcFull, rcGlyph = {0,0,-1,-1};

	const CRgnDetect* pRgn = mp_RCon->GetDetector();

	_ASSERTE(countof(mrc_Dialogs)==countof(mn_DialogFlags));
	mn_DialogsCount = pRgn ? pRgn->GetDetectedDialogs(countof(mrc_Dialogs), mrc_Dialogs, mn_DialogFlags) : 0;
	mn_DialogAllFlags = pRgn ? pRgn->GetFlags() : 0;

	// Even mp_RCon->isFilePanel(true, false, true) may be not suitable
	// because there may be some lags in detection or window switching
	isFilePanel = (mn_DialogAllFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)) != 0;

#if 0
	if (mn_DialogsCount == 7 && mn_LastDialogsCount == 6)
	{
		Dump(L"T:\\Dialogs.con");
	}
#endif

	if (gpSet->isExtendUCharMap)
	{
		if (pRgn && (mn_DialogAllFlags & (FR_UCHARMAP|FR_UCHARMAPGLYPH)) == (FR_UCHARMAP|FR_UCHARMAPGLYPH))
		{
			if (pRgn->GetDetectedDialogs(1, &rcFull, nullptr, FR_UCHARMAP, FR_UCHARMAP)
					&& pRgn->GetDetectedDialogs(1, &rcGlyph, nullptr, FR_UCHARMAPGLYPH, FR_UCHARMAPGLYPH))
			{
				wchar_t szFontName[32], *pszStart, *pszEnd;
				pszStart = mpsz_ConChar + m_Sizes.TextWidth*(rcFull.Top+1) + rcFull.Left + 1;
				pszEnd = pszStart + 31;

				while (*pszEnd == L' ' && pszEnd >= pszStart) pszEnd--;

				if (pszEnd > pszStart)
				{
					wmemcpy(szFontName, pszStart, pszEnd-pszStart+1);
					szFontName[pszEnd-pszStart+1] = 0;

					if (!m_UCharMapFont.IsSet() || lstrcmp(ms_LastUCharMapFont, szFontName))
					{
						wcscpy_c(ms_LastUCharMapFont, szFontName);

						gpFontMgr->CreateOtherFont(ms_LastUCharMapFont, m_UCharMapFont);
					}
				}

				// Шрифт создали, теперь - пометить соответствующий регион для использования этого шрифта
				if (m_UCharMapFont.IsSet())
				{
					for(int Y = rcGlyph.Top; Y <= rcGlyph.Bottom; Y++)
					{
						CharAttr *pAtr = mpn_ConAttrEx + (m_Sizes.TextWidth*Y + rcGlyph.Left);

						for(int X = rcGlyph.Left; X <= rcGlyph.Right; X++, pAtr++)
						{
							pAtr->nFontIndex = fnt_UCharMap;
						}
					}
				}
			}
		}
	}

	mrc_UCharMap = rcGlyph;
	gpSetCls->Performance(tPerfData, TRUE);
	return true;
}

void CVirtualConsole::SetSelfPalette(WORD wAttributes, WORD wPopupAttributes, const COLORREF (&ColorTable)[16])
{
	for (INT_PTR i = 0; i < 16; i++)
	{
		m_SelfPalette.Colors[i] = ColorTable[i];
		m_SelfPalette.Colors[i+16] = ColorTable[i];
	}

	m_SelfPalette.nTextColorIdx = CONFORECOLOR(wAttributes);
	m_SelfPalette.nBackColorIdx = CONBACKCOLOR(wAttributes);
	m_SelfPalette.nPopTextColorIdx = CONFORECOLOR(wPopupAttributes);
	m_SelfPalette.nPopBackColorIdx = CONBACKCOLOR(wPopupAttributes);

	const ColorPalette* pFound = gpSet->PaletteFindByColors(true, &m_SelfPalette);

	// If the matching palette does not exist - try to create temporary new one
	if (!pFound)
	{
		CEStr lsPrefix;
		LPCWSTR pszRootProcessName = mp_RCon->GetRootProcessName();
		if (pszRootProcessName)
		{
			LPCWSTR pszExt = PointToExt(pszRootProcessName);
			if (!pszExt || (pszExt > pszRootProcessName))
			{
				lsPrefix = CEStr(L"#Attached:", pszRootProcessName);
				wchar_t* pszDot = lsPrefix.ms_Val ? wcsrchr(lsPrefix.ms_Val, L'.') : nullptr;
				if (pszDot)
				{
					*pszDot = 0;
				}
			}
		}

		CEStr szAutoName; wchar_t szSuffix[8];
		// Don't create too many palettes...
		for (int i = 0; i <= 99; i++)
		{
			if (!i)
			{
				if (lsPrefix.IsEmpty())
					continue;
				szAutoName.Set(lsPrefix);
			}
			else
			{
				swprintf_c(szSuffix, L":%02i", i);
				szAutoName = CEStr(lsPrefix.IsEmpty() ? L"#Attached" : lsPrefix.ms_Val, szSuffix);
			}

			if (gpSet->PaletteGetIndex(szAutoName) != -1)
				continue;

			break;
		}

		// Save new (temporary) palette
		gpSet->PaletteSaveAs(szAutoName,
			m_SelfPalette.nTextColorIdx, m_SelfPalette.nBackColorIdx,
			m_SelfPalette.nPopTextColorIdx, m_SelfPalette.nPopBackColorIdx,
			m_SelfPalette.Colors, false);
		// And try to match it
		pFound = gpSet->PaletteFindByColors(true, &m_SelfPalette);
		_ASSERTE(pFound != nullptr);
	}

	if (pFound)
	{
		mp_RCon->SetPaletteName(pFound->pszName);
	}
	else
	{
		m_SelfPalette.bPredefined = true;
		m_SelfPalette.FadeInitialized = false;
		gpSet->PrepareFadeColors(m_SelfPalette.Colors, m_SelfPalette.ColorsFade, &m_SelfPalette.FadeInitialized);
		mp_RCon->PrepareDefaultColors();
	}

	Invalidate();
}

const PaletteColors& CVirtualConsole::GetColors()
{
	m_Colors = GetColors(isFade);
	return m_Colors;
}

PaletteColors CVirtualConsole::GetColors(const bool bFade)
{
	if (!this || !mp_RCon)
	{
		return gpSet->GetColors(bFade);
	}

	// Was specified palette forced to this console?
	LPCWSTR pszPalName = nullptr;

	if (m_SelfPalette.bPredefined)
	{
		// original palette from attached RealConsole
		return m_SelfPalette.GetColors(bFade);
	}
	else if (((pszPalName = mp_RCon->GetArgs().pszPalette) != nullptr) && *pszPalName)
	{
		// Palette was set by VCon menu or -new_console:P:...
		return gpSet->GetPaletteColors(pszPalName, bFade);
	}
	else
	{
		// Update AppID if needed
		const int nCurAppId = mp_RCon ? mp_RCon->GetActiveAppSettingsId() : -1;
		return gpSet->GetColors(nCurAppId, bFade);
	}
}

int CVirtualConsole::GetPaletteIndex()
{
	if (!this || !mp_RCon)
	{
		return -1;
	}

	int iActiveIndex = -1;

	if (m_SelfPalette.bPredefined)
	{
		iActiveIndex = 0; // Treat it as "Windows Default"
	}
	else
	{
		LPCWSTR pszPalName = mp_RCon->GetArgs().pszPalette;
		if (pszPalName && *pszPalName)
		{
			iActiveIndex = gpSet->PaletteGetIndex(pszPalName);
		}
		else
		{
			const AppSettings* pAppSet = gpSet->GetAppSettings(mp_RCon ? mp_RCon->GetActiveAppSettingsId() : -1);
			iActiveIndex = pAppSet ? pAppSet->GetPaletteIndex() : -1;
		}
	}

	return iActiveIndex;
}

bool CVirtualConsole::ChangePalette(int aNewPaletteIdx)
{
	const ColorPalette* pPal = gpSet->PaletteGet(aNewPaletteIdx);
	return ChangePalette(pPal);
}

bool CVirtualConsole::ChangePalette(LPCWSTR asNewPalette)
{
	const ColorPalette* pPal = gpSet->PaletteGetByName(asNewPalette);
	return ChangePalette(pPal);
}

bool CVirtualConsole::ChangePalette(const ColorPalette* pPal)
{
	if (!this || !mp_RCon)
		return false;

	const ColorPalette* pOldPal = gpSet->PaletteGet(GetPaletteIndex());
	if (!pPal)
		return false;

	if (gpSet->isLogging())
	{
		CEStr lsLog(L"VCon[", IndexStr(), L"]: Color Palette: `", pPal->pszName, L"`");
		LogString(lsLog);
	}

	bool bTextChanged = (pOldPal==nullptr), bPopupChanged = (pOldPal==nullptr);
	if (pOldPal)
	{
		bTextChanged = (pOldPal->nTextColorIdx != pPal->nTextColorIdx) || (pOldPal->nBackColorIdx != pPal->nBackColorIdx);
		bPopupChanged = (pOldPal->nPopTextColorIdx != pPal->nPopTextColorIdx) || (pOldPal->nPopBackColorIdx != pPal->nPopBackColorIdx);
	}

	m_SelfPalette.bPredefined = false;
	mp_RCon->SetPaletteName(pPal->pszName);
	mp_RCon->UpdateTextColorSettings(bTextChanged, bPopupChanged);

	Update(true);
	return true;
}

void CVirtualConsole::OnAppSettingsChanged(int iAppId /*= -1*/)
{
	wchar_t szInfo[80];
	LONG lNew = InterlockedIncrement(&mn_AppSettingsChangCount);
	swprintf_c(szInfo, L"VCon::OnAppSettingsChanged AppId=%i, Counter=%i", iAppId, lNew);

	if (lNew == 1)
	{
		if (!mb_InUpdate)
		{
			wcscat_c(szInfo, L" - calling Update(true)");
			DEBUGSTRAPPID(szInfo);
			Update(true);
		}
		else
		{
			wcscat_c(szInfo, L" - skipped, mb_InUpdate");
			DEBUGSTRAPPID(szInfo);
		}
	}
	else
	{
		wcscat_c(szInfo, L" - skipped, counter");
		DEBUGSTRAPPID(szInfo);
	}
}

bool CVirtualConsole::UpdatePrepare(HDC *ahDc, MSectionLock *pSDC, MSectionLock *pSCON)
{
	MSectionLock SCON; SCON.Lock(&csCON);
	attrBackLast = 0;
	isEditor = mp_RCon->isEditor();
	isViewer = mp_RCon->isViewer();
	// Even mp_RCon->isFilePanel(true, false, true) may be not suitable
	// because there may be some lags in detection or window switching
	// It would be repeated in LoadConsoleData
	isFilePanel = (mn_DialogAllFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)) != 0;
	isFade = !isForeground && gpSet->isFadeInactive;

	// Retrieve palette colors
	GetColors();

	// The application settings were changed?
	LONG lOldChange = InterlockedExchange(&mn_AppSettingsChangCount, 0);
	if (lOldChange)
	{
		InvalidateBack();
	}

	if ((m_Sizes.nFontHeight != gpFontMgr->FontHeight()) || (m_Sizes.nFontWidth != gpFontMgr->FontWidth()))
		isFontSizeChanged = true;
	m_Sizes.nFontHeight = gpFontMgr->FontHeight();
	m_Sizes.nFontWidth = gpFontMgr->FontWidth();
	winSize = MakeCoord(mp_RCon->TextWidth(),mp_RCon->TextHeight());
	//csbi.dwCursorPosition.X -= csbi.srWindow.Left; -- горизонтальная прокрутка игнорируется!
	csbi.dwCursorPosition.Y -= csbi.srWindow.Top;
	isCursorValid =
		csbi.dwCursorPosition.X >= 0 && csbi.dwCursorPosition.Y >= 0 &&
		csbi.dwCursorPosition.X < winSize.X && csbi.dwCursorPosition.Y < winSize.Y;

	if (mb_RequiredForceUpdate || mb_IsForceUpdate)
	{
		isForce = true;
		mb_RequiredForceUpdate = false;
	}

	// Первая инициализация, или смена размера
	BOOL lbSizeChanged = ((HDC)m_DC == nullptr) || (m_Sizes.TextWidth != (unsigned)winSize.X || m_Sizes.TextHeight != (unsigned)winSize.Y)
		|| (m_Sizes.LastPadSize != gpSet->nCenterConsolePad)
		|| isFontSizeChanged; // или смена шрифта ('Auto' на 'Main')

	#ifdef _DEBUG
	COORD dbgWinSize = winSize;
	_ASSERTE(!HIWORD(m_Sizes.TextWidth)&&!HIWORD(m_Sizes.TextHeight));
	COORD dbgTxtSize = {LOSHORT(m_Sizes.TextWidth),LOSHORT(m_Sizes.TextHeight)};
	#endif

	_ASSERTE(isMainThread());

	// 120108 - пересоздавать DC на каждый чих не нужно (isForce выставляется, например, при смене Fade)
	if (/*isForce ||*/ !mb_PointersAllocated || lbSizeChanged)
	{
		// 100627 перенес после InitDC, т.к. циклилось
		//if (lbSizeChanged)
		//	mp_ConEmu->OnConsole Resize(TRUE);
		//if (pSDC && !pSDC->isLocked())  // Если секция еще не заблокирована (отпускает - вызывающая функция)
		//	pSDC->Lock(&csDC, TRUE, 200); // но по таймауту, чтобы не повисли ненароком

		if (!InitDC(ahDc!=nullptr && !isForce/*abNoDc*/ , false/*abNoWndResize*/, pSDC, pSCON))
		{
			return false;
		}

		if (lbSizeChanged)
		{
			// В общем здесь, если размер RConSrv совпадает с размером RCon
			// то нужно просто подровнять наше окошко...
			// Это происходит при выходе из NTVDM
			bool bSizeProcessed = false;
			if (!mp_RCon->isFixAndCenter())
			{
				RECT rcBack = {}; GetClientRect(mh_WndBack, &rcBack);
				RECT rcCon = CVConGroup::CalcRect(CER_CONSOLE_CUR, rcBack, CER_BACK, this);
				if ((rcCon.right == (LONG)m_Sizes.TextWidth) && (rcCon.bottom == (LONG)m_Sizes.TextHeight))
				{
					// Просто подвинуть окошко
					MapWindowPoints(mh_WndBack, ghWnd, (LPPOINT)&rcBack, 2);
					RECT rcNewPos = CVConGroup::CalcRect(CER_DC, rcBack, CER_BACK, this);
					RECT rcCurPos = {}; GetClientRect(mh_WndDC, &rcCurPos);
					MapWindowPoints(mh_WndDC, ghWnd, (LPPOINT)&rcCurPos, 2);
					if (memcmp(&rcNewPos, &rcCurPos, sizeof(rcCurPos)) != 0)
					{
						SetVConSizePos(rcBack, rcNewPos, true);
						// Refresh status columns
						OnVConSizePosChanged();
					}
				}
			}
			if (!bSizeProcessed)
			{
				MSetter lInConsoleResize(&mb_InConsoleResize);
				mp_ConEmu->OnConsoleResize();
			}
		}


		isForce = true; // После сброса буферов и DC - необходим полный refresh...


		#ifdef _DEBUG
		if (m_Sizes.TextWidth == 80 && !mp_RCon->isNtvdm())
		{
			m_Sizes.TextWidth = m_Sizes.TextWidth;
		}
		#endif

		//if (lbSizeChanged) -- попробуем вверх перенести
		//	mp_ConEmu->OnConsole Resize();
	}

	RECT rcBack = {}, rcDC = {};
	GetWindowRect(GetBack(), &rcBack);
	GetWindowRect(GetView(), &rcDC);
	m_Sizes.BackWidth = RectWidth(rcBack);
	m_Sizes.BackHeight = RectHeight(rcBack);
	m_Sizes.OffsetX = rcDC.left - rcBack.left;
	m_Sizes.OffsetY = rcDC.top - rcBack.top;

	// Требуется полная перерисовка!
	if (mb_IsForceUpdate)
	{
		isForce = true;
		mb_IsForceUpdate = false;
	}

	// Отладочный режим. Двойная буферизация отключена, отрисовка идет сразу на ahDc
	if (ahDc)
		isForce = true;

	//drawImage = (gpSet->isShowBgImage == 1 || (gpSet->isShowBgImage == 2 && !(isEditor || isViewer)) )
	//	&& gpSet->isBackgroundImageValid;
	drawImage = gpSetCls->IsBackgroundEnabled(this);
	TextLen = m_Sizes.TextWidth * m_Sizes.TextHeight;
	coord.X = csbi.srWindow.Left; coord.Y = csbi.srWindow.Top;

	DWORD nIndexes = gpSet->nBgImageColors;
	if (nIndexes == (DWORD)-1 && mp_RCon)
	{
		// Retrieve palette from Far Manager
		const CEFAR_INFO_MAPPING* pFar = mp_RCon->GetFarInfo();
		if (pFar)
		{
			// Get background color of panels/editors/viewers
			UINT nDefIdx = isEditor ? col_EditorText : isViewer ? col_ViewerText : col_PanelText;
			BYTE bgIndex = CONBACKCOLOR(pFar->nFarColors[nDefIdx]);
			nIndexes = 1 << (DWORD)bgIndex;
		}
	}
	if (nIndexes == (DWORD)-1)
		nIndexes = BgImageColorsDefaults; // Defaults
	nBgImageColors = nIndexes;

	if (drawImage)
	{
		// Она заодно проверит, не изменился ли файл?
		if (mp_Bg->PrepareBackground(this, hBgDc, bgBmpSize))
		{
			isForce = true;
		}
		else
		{
			drawImage = (hBgDc!=nullptr);
		}
	}

	// скопировать данные из состояния консоли В mpn_ConAttrEx/mpsz_ConChar
	BOOL bConDataChanged = isForce || mp_RCon->IsConsoleDataChanged();

	if (bConDataChanged)
	{
		mb_ConDataChanged = true; // В false - НЕ сбрасывать
		// Вынес в функцию, т.к. только так RealConsole может вызвать обновление детектора диалогов
		LoadConsoleData();
	}

	HEAPVAL;
	// Определить координаты панелей (Переехало в CRealConsole)
	// get cursor info
	mp_RCon->GetConsoleCursorInfo(/*hConOut(),*/ &cinf);
	HEAPVAL;
	return true;
}


void CVirtualConsole::UpdateText()
{
	_ASSERTE((HDC)m_DC!=nullptr);

	// Refresh fonts array
	//memmove(mh_FontByIndex, gpFontMgr->mh_Font, sizeof(gpFontMgr->mh_Font));
	//mh_FontByIndex[fnt_UCharMap] = mh_UCharMapFont ? mh_UCharMapFont : mh_FontByIndex[0];
	//mh_FontByIndex[fnt_FarBorders] = (gpSetCls->mh_Font2.IsSet() && gpSet->isFixFarBorders) ? gpSetCls->mh_Font2 : mh_FontByIndex[0];

	SelectFont(fnt_Normal);

	// pointers
	wchar_t* ConCharLine = mpsz_ConChar;
	CharAttr* ConAttrLine = mpn_ConAttrEx;
	DWORD* ConCharXLine = ConCharX;
	// counters
	int pos = 0, row = 0;
	// max vertical coordinate of line allowed
	int nMaxPos = m_Sizes.Height - m_Sizes.nFontHeight;

	if (!ConCharLine || !ConAttrLine || !ConCharXLine)
	{
		MBoxAssert(ConCharLine && ConAttrLine && ConCharXLine);
		return;
	}


	if (/*gpSet->isForceMonospace ||*/ !drawImage)
		m_DC.SetBkMode(OPAQUE);
	else
		m_DC.SetBkMode(TRANSPARENT);

	// #MEMORY Try buffer on stack, allocate only if width is too large
	int *nDX = (int*)VAlloc(m_Sizes.TextWidth + 1, sizeof(int));

	bool bEnhanceGraphics = gpSet->isEnhanceGraphics;
	bool bFixFarBorders = _bool(gpSet->isFixFarBorders);
	bool bFontProportional = !gpFontMgr->FontMonospaced();
	//CEFONT hFont = gpFontMgr->mh_Font[0];
	//CEFONT hFont2 = gpFontMgr->mh_Font2;
	unsigned partIndex;
	VConTextPart *part, *nextPart;

	CVConLine lp(mp_RCon->isFar()); // Line parser
	lp.SetDialogs(mn_DialogsCount, mrc_Dialogs, mn_DialogFlags, mn_DialogAllFlags, mrc_UCharMap);

	bool bFixFrameCoord = mp_RCon->isFar();

	_ASSERTE(((m_Sizes.TextWidth * m_Sizes.nFontWidth) >= m_Sizes.Width));

	BYTE nFontCharSet = gpFontMgr->FontCharSet();

	wchar_t* tmpOemWide = nullptr;
	char* tmpOem = nullptr;
	INT_PTR cchOemMax = 0;
	if (nFontCharSet == OEM_CHARSET)
	{
		cchOemMax = (m_Sizes.TextWidth*3+1);
		tmpOemWide = (wchar_t*)VAlloc(cchOemMax, sizeof(*tmpOemWide));
		tmpOem = (char*)VAlloc(cchOemMax, sizeof(*tmpOem));
	}

	_ASSERTE(m_Sizes.nFontHeight > 0 && m_Sizes.nFontWidth > 0);

	//BUGBUG: хорошо бы отрисовывать последнюю строку, даже если она чуть не влазит
	for (; pos <= nMaxPos;
			ConCharLine += m_Sizes.TextWidth, ConAttrLine += m_Sizes.TextWidth, ConCharXLine += m_Sizes.TextWidth,
			pos += m_Sizes.nFontHeight, row++)
	{
		if (row >= (int)m_Sizes.TextHeight)
		{
			//_ASSERTE(row < (int)TextHeight);
			DEBUGSTRFAIL(L"############ _ASSERTE(row < (int)TextHeight) #############\n");
			break;
		}

		// the line
		const CharAttr* const ConAttrLine2 = mpn_ConAttrExSave + (ConAttrLine - mpn_ConAttrEx);
		const wchar_t* const ConCharLine2 = mpsz_ConCharSave + (ConCharLine - mpsz_ConChar);

		// If there were no changes in this line
		if (!isForce && !FindChanges(row, ConCharLine, ConAttrLine, ConCharLine2, ConAttrLine2))
			continue;

		// skip not changed symbols except the old cursor or selection
		int j = 0, end = m_Sizes.TextWidth;

		// Was cursor visible on this line?
		if (Cursor.isVisiblePrev && row == Cursor.y
				&& (j <= Cursor.x && Cursor.x <= end))
			Cursor.isVisiblePrev = false;

		// To be able to draw OEM text properly, we have to convert
		wchar_t* pszDrawLine = ConCharLine;
		BYTE charSet;
		if ((nFontCharSet == OEM_CHARSET) && tmpOem && tmpOemWide)
		{
			int iCvt = WideCharToMultiByte(CP_OEMCP, 0, ConCharLine, m_Sizes.TextWidth, tmpOem, cchOemMax, nullptr, nullptr);
			if (iCvt > 0 && iCvt <= cchOemMax)
			{
				int iWide = MultiByteToWideChar(CP_OEMCP, 0, tmpOem, iCvt, tmpOemWide, cchOemMax);
				if ((iWide > 0) && (iWide <= cchOemMax))
				{
					_ASSERTE(iWide == (int)m_Sizes.TextWidth); // Or we probably have problems with color mismatch
					pszDrawLine = tmpOemWide;
					for (int i = 0; i < iWide; i++)
					{
						if ((unsigned)(tmpOemWide[i]) <= 31)
						{
							tmpOemWide[i] = gszAnalogues[tmpOemWide[i]];
						}
					}
				}
			}
		}

		// May return false on memory allocation errors only
		if (!lp.ParseLine(isForce, m_Sizes.TextWidth, m_Sizes.nFontWidth, row, pszDrawLine, ConAttrLine, ConCharLine2, ConAttrLine2))
		{
			// Fill with background?
			continue;
		}

		// Calculate proper widths for proportional fonts
		//TODO: Evaluate width for monospace fonts too! Linked fonts may be used for some char ranges!
		//if (bFontProportional)
		{
			partIndex = 0;
			_ASSERTE(m_Sizes.nFontWidth > 0);
			unsigned nChWidth = static_cast<unsigned>(m_Sizes.nFontWidth);
			unsigned nChWidthAndHalf = nChWidth * 8 / 6;
			while (lp.GetNextPart(partIndex, part, nextPart))
			{
				TextCharType *pcf = part->CharFlags;
				unsigned cw, *pcw = part->CharWidth;
				bool bDoubled;
				for (unsigned i = 0; i < part->Length; i++, pcf++, pcw++)
				{
					if (*pcf >= TCF_WidthFree)
					{
						TODO("Use ucs32 (surrogates) to evaluate width");
						cw = CharWidth(part->Chars[i], part->Attrs[i]);
						if ((bDoubled = (cw >= nChWidthAndHalf)))
							*pcf = TCF_WidthDouble;
						*pcw = bFontProportional ? cw : bDoubled ? cw : nChWidth;
					}
				}
			}
		}

		SelectFont(fnt_NULL);

		// Final preparations
		lp.PolishParts(ConCharXLine);

		RECT rect = {};

		// Do first run to paint only background (background image support)
		partIndex = 0;
		while (lp.GetNextPart(partIndex, part, nextPart))
		{
			const CharAttr attr = part->Attrs[0];
			//m_DC.SetBkColor(attr.crBackColor);

			rect.left = part->PositionX;
			rect.top = pos;
			rect.bottom = rect.top + m_Sizes.nFontHeight;
			rect.right = part->PositionX + part->TotalWidth;

			if (nextPart)
			{
				while (nextPart
					&& (nextPart->Attrs[0].crBackColor == attr.crBackColor)
					&& (nextPart->Attrs[0].nBackIdx == attr.nBackIdx)
					)
				{
					if (!lp.GetNextPart(partIndex, part, nextPart))
					{
						_ASSERTE(FALSE && "Must not be here because nextPart!=nullptr");
						break;
					}
				}

				if (!part)
				{
					_ASSERTE(FALSE && "part must be !nullptr if nextPart was detected previously");
				}
			}

			// Take last part and set .right to its coords (doesn't matter if .TotalWidth == 0)
			if (part)
			{
				rect.right = part->PositionX + part->TotalWidth;
			}

			// If these parts failed to fit in space
			if (rect.left == rect.right)
			{
				continue;
			}

			// Fill the space with background (erase previously printed text)
			if (drawImage && ISBGIMGCOLOR(attr.nBackIdx))
			{
				PaintBackgroundImage(m_DC, rect, attr.crBackColor, false);
			}
			else
			{
				HBRUSH hbr = PartBrush(L' ', attr.crBackColor, attr.crForeColor);
				FillRect((HDC)m_DC, &rect, hbr);
			}

			// Fin of the sequence?
			if (!nextPart)
			{
				break;
			}
		}

		// Second run - paint text
		partIndex = 0;
		m_DC.SetBkMode(TRANSPARENT);
		while (lp.GetNextPart(partIndex, part, nextPart))
		{
			const CharAttr& attr = part->Attrs[0];
			m_DC.SetBkColor(attr.crBackColor);
			m_DC.SetTextColor(attr.crForeColor);
			if (part->Flags & TRF_TextAlternative)
			{
				SelectFont(fnt_Alternative);
				charSet = gpFontMgr->BorderFontCharSet();
			}
			else
			{
				SelectFont((CEFontStyles)attr.nFontIndex);
				charSet = nFontCharSet;
			}

			const unsigned nRightEx = std::max((unsigned)1, (unsigned)m_Sizes.nFontWidth / 4);
			rect.left = part->PositionX;
			rect.top = pos;
			// For Bold and Italic we slightly extend drawing rect to avoid clipping
			// Old note: if nextPart is VertBorder, then increase rect.right by ((FontWidth>>1)-1)
			// Old note: to ensure, that our possible *italic* text will not be clipped
			if (attr.nFontIndex & 3)
				rect.right = std::min((unsigned)part->PositionX + part->TotalWidth + nRightEx, m_Sizes.Width);
			else
				rect.right = part->PositionX + part->TotalWidth;
			rect.bottom = rect.top + m_Sizes.nFontHeight;

			UINT nFlags = ETO_CLIPPED;

			//TODO: ETO_GLYPH_INDEX, GetCharacterPlacement

			// Blocks ░▒▓█ (25%, 50%, 75%, 100%)
			if (bEnhanceGraphics && (part->Flags & TRF_TextProgress))
			{
				// It may be done on first step, but if we do not do that here,
				// some text may overlap scrollbars and progressbars...
				wchar_t wc = part->Chars[0];
				unsigned nFrom = 0;
				while (nFrom < part->Length)
				{
					wc = part->Chars[nFrom];
					unsigned nTo = nFrom + 1;
					while (((nTo + 1) < part->Length) && (part->Chars[nTo] == wc))
						nTo++;
					HBRUSH hbr = PartBrush(wc, attr.crBackColor, attr.crForeColor);
					rect.left = part->PositionX + nFrom * part->TotalWidth / part->Length;
					rect.right = part->PositionX + nTo * part->TotalWidth / part->Length;
					FillRect((HDC)m_DC, &rect, hbr);
					nFrom = nTo;
				}
			}
			// Lefward  and Rightward  triangles (used in some custom prompt layouts and status bars)
			else if (bEnhanceGraphics && (part->Flags & TRF_TextTriangles))
			{
				HBRUSH hbrBack = PartBrush(ucBox100, attr.crBackColor, attr.crBackColor);
				FillRect((HDC)m_DC, &rect, hbrBack);

				HBRUSH hbrFore = PartBrush(ucBox100, attr.crBackColor, attr.crForeColor);
				bool bRight = (part->Chars[0] == ucTrgRight);
				POINT ptTrg[] = {
					{bRight ? rect.left : rect.right, rect.top},
					{bRight ? rect.right : rect.left, (rect.top + rect.bottom)/2},
					{bRight ? rect.left : rect.right, rect.bottom}
				};
				HRGN hrgn = CreatePolygonRgn(ptTrg, countof(ptTrg), WINDING);
				//Polygon((HDC)m_DC, ptTrg, countof(ptTrg));
				FillRgn((HDC)m_DC, hrgn, hbrFore);
				DeleteObject(hrgn);
			}
			else if ((pszDrawLine == tmpOemWide) && (charSet == OEM_CHARSET))
			{
				LPCSTR pszDrawOem = tmpOem + (part->Chars - tmpOemWide);
				m_DC.TextDrawOem(rect.left, rect.top, nFlags, &rect,
					pszDrawOem, part->Length, (int*)part->CharWidth/*lpDX*/);
			}
			else
			{
				m_DC.TextDraw(rect.left, rect.top, nFlags, &rect,
					part->Chars, part->Length, (int*)part->CharWidth/*lpDX*/);
			}
		}

		if (rect.right < (int)m_Sizes.Width)
		{
			bool lbDelBrush = false;
			HBRUSH hBr = CreateBackBrush(false, lbDelBrush);

			RECT rcFill = rect;
			rcFill.left = rect.right;
			rcFill.right = (int)m_Sizes.Width;
			FillRect((HDC)m_DC, &rcFill, hBr);

			if (lbDelBrush)
				DeleteObject(hBr);
		}

		HEAPVALPTR(nDX);
	}

	if ((pos > nMaxPos) && (pos < (int)m_Sizes.Height))
	{
		bool lbDelBrush = false;
		HBRUSH hBr = CreateBackBrush(false, lbDelBrush);

		RECT rcFill = {0, pos, (int)m_Sizes.Width, (int)m_Sizes.Height};
		FillRect((HDC)m_DC, &rcFill, hBr);

		if (lbDelBrush)
			DeleteObject(hBr);
	}

	std::vector<ConPtyGlyph> graphemes;
	if (mp_RCon->GetConPtyGlyphs(m_Sizes.TextWidth, m_Sizes.TextHeight, graphemes) && !graphemes.empty())
	{
		const HFONT emojiFont = CreateFontW(-m_Sizes.nFontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI Emoji");
		for (const auto& glyph : graphemes)
		{
			const unsigned cell = glyph.position.Y * m_Sizes.TextWidth + glyph.position.X;
			const auto& attr = mpn_ConAttrEx[cell];
			const RECT rect = {glyph.position.X * m_Sizes.nFontWidth, glyph.position.Y * m_Sizes.nFontHeight,
				(glyph.position.X + static_cast<int>(glyph.width)) * m_Sizes.nFontWidth,
				(glyph.position.Y + 1) * m_Sizes.nFontHeight};
			DrawConPtyGrapheme(m_DC, rect, glyph.text, attr.crForeColor, attr.crBackColor, m_Sizes.nFontHeight, emojiFont);
		}
		if (emojiFont) DeleteObject(emojiFont);
	}

	VFree(nDX);

	VFree(tmpOemWide);
	VFree(tmpOem);

	HEAPVAL;

	// Screen updated, reset until next "UpdateHighlights()" call
	m_HighlightInfo.m_Last.X = m_HighlightInfo.m_Last.Y = -1;
	m_HighlightInfo.LastRect.clear();

	return;
}

void CVirtualConsole::ClearPartBrushes()
{
	_ASSERTE(isMainThread());

	for (UINT br=0; br<MAX_COUNT_PART_BRUSHES; br++)
	{
		DeleteObject(m_PartBrushes[br].hBrush);
	}

	memset(m_PartBrushes, 0, sizeof(m_PartBrushes));
}

HBRUSH CVirtualConsole::PartBrush(wchar_t ch, COLORREF nBackCol, COLORREF nForeCol)
{
	_ASSERTE(isMainThread());
	//std::vector<PARTBRUSHES>::iterator iter = m_PartBrushes.begin();
	//while (iter != m_PartBrushes.end()) {
	PARTBRUSHES *pbr = m_PartBrushes;

	for (UINT br=0; pbr->ch && br<MAX_COUNT_PART_BRUSHES; br++, pbr++)
	{
		if (pbr->ch == ch && pbr->nBackCol == nBackCol && pbr->nForeCol == nForeCol)
		{
			_ASSERTE(pbr->hBrush);
			return pbr->hBrush;
		}
	}

	if (pbr >= (m_PartBrushes + MAX_COUNT_PART_BRUSHES))
	{
		#ifdef _DEBUG
		static bool bShown;
		if (!bShown)
		{
			bShown = true;
			_ASSERTE(FALSE && "Too much background brushes were created!");
		}
		#endif
		pbr = &m_PartBrushes[MAX_COUNT_PART_BRUSHES-1];
		DeleteObject(pbr->hBrush);
		pbr->hBrush = nullptr;
	}

	MYRGB clrBack, clrFore, clrMy;
	clrBack.color = nBackCol;
	clrFore.color = nForeCol;
	clrMy.color = clrFore.color; // 100 %
	////#define   PART_75(f,b) ((((int)f) + ((int)b)*3) / 4)
	////#define   PART_50(f,b) ((((int)f) + ((int)b)) / 2)
	////#define   PART_25(f,b) (((3*(int)f) + ((int)b)) / 4)
	////#define   PART_75(f,b) (b + 0.80*(f-b))
	////#define   PART_50(f,b) (b + 0.75*(f-b))
	////#define   PART_25(f,b) (b + 0.50*(f-b))
	// (gpSet->isPartBrushXX >> 8) дает как бы дробь (0 .. 1), например, 0.80
#define PART_XX(f,b,p) ((f==b) ? f : ( (b + ((p*(f-b))>>8))))
#define PART_75(f,b) ((f>b) ? PART_XX(f,b,gpSet->isPartBrush75) : PART_XX(b,f,gpSet->isPartBrush25))
#define PART_50(f,b) ((f>b) ? PART_XX(f,b,gpSet->isPartBrush50) : PART_XX(b,f,gpSet->isPartBrush50))
#define PART_25(f,b) ((f>b) ? PART_XX(f,b,gpSet->isPartBrush25) : PART_XX(b,f,gpSet->isPartBrush75))

	if (ch == ucBox75 || ch == ucBox50 || ch == ucBox25)
	{
		// Для темных мониторов. Бокс 25% нифига не отличается от заполненного 100% (видится черным)
		if (clrBack.color == 0)
		{
			if (gpSet->isPartBrushBlack
			        && clrFore.rgbRed > gpSet->isPartBrushBlack
			        && clrFore.rgbGreen > gpSet->isPartBrushBlack
			        && clrFore.rgbBlue > gpSet->isPartBrushBlack)
			{
				clrBack.rgbBlue = clrBack.rgbGreen = clrBack.rgbRed = gpSet->isPartBrushBlack;
			}
		}
		else if (clrFore.color == 0)
		{
			if (gpSet->isPartBrushBlack
			        && clrBack.rgbRed > gpSet->isPartBrushBlack
			        && clrBack.rgbGreen > gpSet->isPartBrushBlack
			        && clrBack.rgbBlue > gpSet->isPartBrushBlack)
			{
				clrFore.rgbBlue = clrFore.rgbGreen = clrFore.rgbRed = gpSet->isPartBrushBlack;
			}
		}
	}

	if (ch == ucBox75 /* 75% */)
	{
		clrMy.rgbRed = PART_75(clrFore.rgbRed,clrBack.rgbRed);
		clrMy.rgbGreen = PART_75(clrFore.rgbGreen,clrBack.rgbGreen);
		clrMy.rgbBlue = PART_75(clrFore.rgbBlue,clrBack.rgbBlue);
		clrMy.rgbReserved = 0;
	}
	else if (ch == ucBox50 /* 50% */)
	{
		clrMy.rgbRed = PART_50(clrFore.rgbRed,clrBack.rgbRed);
		clrMy.rgbGreen = PART_50(clrFore.rgbGreen,clrBack.rgbGreen);
		clrMy.rgbBlue = PART_50(clrFore.rgbBlue,clrBack.rgbBlue);
		clrMy.rgbReserved = 0;
	}
	else if (ch == ucBox25 /* 25% */)
	{
		clrMy.rgbRed = PART_25(clrFore.rgbRed,clrBack.rgbRed);
		clrMy.rgbGreen = PART_25(clrFore.rgbGreen,clrBack.rgbGreen);
		clrMy.rgbBlue = PART_25(clrFore.rgbBlue,clrBack.rgbBlue);
		clrMy.rgbReserved = 0;
	}
	else if (ch == ucSpace || ch == ucNoBreakSpace /* Non breaking space */ || !ch)
	{
		clrMy.color = clrBack.color;
	}
	#ifdef _DEBUG
	else
	{
		// must be already set
		_ASSERTE(ch == ucBox100 && clrMy.color == clrFore.color);
	}
	#endif

	PARTBRUSHES pb;
	pb.ch = ch; pb.nBackCol = nBackCol; pb.nForeCol = nForeCol;
	pb.hBrush = CreateSolidBrush(clrMy.color);
	//m_PartBrushes.push_back(pb);
	*pbr = pb;
	return pb.hBrush;
}

void CVirtualConsole::UpdateCursorDraw(HDC hPaintDC, RECT rcClient, COORD pos, UINT dwSize)
{
	if (!mp_RCon || !mp_RCon->isNeedCursorDraw())
		return;
	//if (mp_RCon && mp_RCon->GuiWnd())
	//{
	//	// В GUI режиме VirtualConsole скрыта под GUI окном и видна только при "включении" BufferHeight
	//	if (!mp_RCon->isBufferHeight())
	//	{
	//		return;
	//	}
	//}

	Cursor.x = csbi.dwCursorPosition.X;
	Cursor.y = csbi.dwCursorPosition.Y;
	int CurChar = pos.Y * m_Sizes.TextWidth + pos.X;

	if (CurChar < 0 || CurChar>=(int)(m_Sizes.TextWidth * m_Sizes.TextHeight))
	{
		return; // может быть или глюк - или размер консоли был резко уменьшен и предыдущая позиция курсора пропала с экрана
	}

	if (!ConCharX)
	{
		return; // защита
	}

	POINT pix = {pos.X * m_Sizes.nFontWidth, pos.Y * m_Sizes.nFontHeight};
	if (pos.X && ConCharX[CurChar-1])
		pix.x = ConCharX[CurChar-1];

	POINT pix2 = {pix.x + m_Sizes.nFontWidth, pix.y + m_Sizes.nFontHeight};
	if (((pos.X+1) < (int)m_Sizes.TextWidth) && ((CurChar+1) < (int)(m_Sizes.TextWidth * m_Sizes.TextHeight)) && ConCharX[CurChar])
		pix2.x = ConCharX[CurChar];
	else if (pix2.x > rcClient.right)
		pix2.x = std::max(rcClient.right,pix2.x);

	RECT rect;
	bool bForeground = mp_ConEmu->isMeForeground();
	bool bActive = bForeground && isActive(false) && cinf.bVisible;
	bool bGroupActive = bForeground && isGroupedInput() && isActive(true);

	// указатель на настройки разделяемые по приложениям
	mp_Set = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());

	// Use half-cell horz cursor in keyboard selection
	bool bInSel = mp_RCon->isSelectionPresent();
	if (bInSel) dwSize = 50;

	CECursorType curType = GetCursor(bActive);

	CECursorStyle curStyle = bInSel ? cur_Horz : curType.CursorType;
	int MinSize = bInSel ? CURSORSIZEPIX_STD/*2*/ : curType.MinSize;

	if ((curStyle == cur_Block) || (curStyle == cur_Rect))
	{
		dwSize = 100; // percents
		rect.left = pix.x;
		rect.right = pix2.x;
		rect.bottom = pix2.y;
		rect.top = pix.y;
	}
	else if (curStyle == cur_Horz) // Horizontal
	{
		rect.left = pix.x; /*Cursor.x * nFontWidth;*/
		rect.right = pix2.x;

		rect.bottom = pix2.y;
		rect.top = pix.y;
		int nHeight = 0;

		if (dwSize)
		{
			nHeight = std::min((int)m_Sizes.nFontHeight, std::max(MulDiv(m_Sizes.nFontHeight, dwSize, 100), MinSize));
		}

		if (!nHeight)
		{
			// Hollow cursor, nothing to draw
			return;
		}

		rect.top = std::max(rect.top, (rect.bottom-nHeight));
	}
	else // Vertical
	{
		_ASSERTE(curStyle == cur_Vert); // Validate type

		rect.left = pix.x;

		rect.top = pos.Y * m_Sizes.nFontHeight;
		int nR = pix2.x;
		int nWidth = MinSize;

		if (dwSize)
		{
			int nMaxWidth = (nR - rect.left);

			nWidth = std::min(nMaxWidth, std::max(MulDiv(nMaxWidth, dwSize, 100), MinSize));
		}

		if (!nWidth)
		{
			// Hollow cursor, nothing to draw
			return;
		}

		rect.right = std::min<int>(nR, (rect.left + nWidth));
		rect.bottom = pix2.y;
	}

	rect.left += rcClient.left;
	rect.right += rcClient.left;
	rect.top += rcClient.top;
	rect.bottom += rcClient.top;

	if (bActive)
	{
		mp_ConEmu->UpdateCaretPos(*this, rect);
	}

	// Если курсор занимает более 40% площади - принудительно включим
	// XOR режим, иначе (тем более при немигающем) курсор закрывает
	// весь символ и его не видно
	// 110131 Если курсор мигающий - разрешим нецветной курсор для AltIns в фаре
	bool bCursorColor = bInSel ? true
		: (curType.isColor || (dwSize >= 40 && !GetCursor(bGroupActive).isBlinking && (curStyle != cur_Rect)));

	// Теперь в rect нужно отобразить курсор (XOR'ом попробуем?)
	if (bCursorColor)
	{
		HBRUSH hBr = CreateSolidBrush(CURSOR_PAT_COLOR);
		HBRUSH hOld = (HBRUSH)SelectObject(hPaintDC, hBr);

		PatInvertRect(hPaintDC, rect, (HDC)m_DC, (curStyle != cur_Rect));

		SelectObject(hPaintDC, hOld);
		DeleteObject(hBr);
		return;
	}

	//lbDark = Cursor.foreColorNum < 5; // Было раньше
	//BOOL lbIsProgr = isCharProgress(Cursor.ch[0]);
	BOOL lbDark = FALSE;
	DWORD clr = (Cursor.ch != ucBox100 && Cursor.ch != ucBox75) ? Cursor.bgColor : Cursor.foreColor;
	BYTE R = (BYTE)(clr & 0xFF);
	BYTE G = (BYTE)((clr & 0xFF00) >> 8);
	BYTE B = (BYTE)((clr & 0xFF0000) >> 16);
	lbDark = (R <= 0xC0) && (G <= 0xC0) && (B <= 0xC0);
	clr = lbDark ? m_Colors[15] : m_Colors[0];
	HBRUSH hBr = CreateSolidBrush(clr);
	if (curStyle != cur_Rect)
	{
		FillRect(hPaintDC, &rect, hBr);
	}
	else
	{
		RECT r = {rect.left, rect.top+1, rect.left+1, rect.bottom-1};
		FillRect(hPaintDC, &r, hBr);
		r = MakeRect(rect.left+1, rect.top, rect.right-1, rect.top+1);
		FillRect(hPaintDC, &r, hBr);
		r = MakeRect(rect.right-1, rect.top+1, rect.right, rect.bottom-1);
		FillRect(hPaintDC, &r, hBr);
		r = MakeRect(rect.left+1, rect.bottom-1, rect.right-1, rect.bottom);
		FillRect(hPaintDC, &r, hBr);
	}
	DeleteObject(hBr);
}

bool CVirtualConsole::UpdateCursorGroup(CVirtualConsole* pVCon, LPARAM lParam)
{
	if (pVCon != ((CVirtualConsole*)lParam))
	{
		bool bNeedUpdate = false;
		pVCon->UpdateCursor(bNeedUpdate);
		if (bNeedUpdate)
		{
			pVCon->Invalidate();
		}
	}

	return true; // continue group
}

CECursorType CVirtualConsole::GetCursor(bool bActive)
{
	CECursorType ct;
	if (this || !mp_Set || !mp_RCon)
		ct = gpSet->AppStd.Cursor(bActive);
	else
		ct = mp_Set->Cursor(bActive);

	TermCursorShapes termShape = mp_RCon->GetCursorShape();
	if (termShape)
	{
		switch (termShape)
		{
		case tcs_BlockBlink:
		case tcs_BlockSteady:
			ct.CursorType = cur_Block; break;
		case tcs_UnderlineBlink:
		case tcs_UnderlineSteady:
			ct.CursorType = cur_Horz; break;
		default:
			ct.CursorType = cur_Vert; break;
		}
		ct.isBlinking = (termShape == tcs_BlockBlink || termShape == tcs_UnderlineBlink || termShape == tcs_VBarBlink);
		ct.isFixedSize = false;
		ct.isColor = true;
		ct.MinSize = CURSORSIZEPIX_STD/*2*/;
	}

	return ct;
}

void CVirtualConsole::UpdateCursor(bool& lRes)
{
	if (!this || !mp_RCon)
	{
		return; // Exceptional
	}

	if (mp_RCon->GuiWnd())
	{
		// В GUI режиме VirtualConsole скрыта под GUI окном и видна только при "включении" BufferHeight
		if (!mp_RCon->isBufferHeight())
		{
			lRes = false;
			return;
		}
	}

	if (mp_RCon->isMouseSelectionPresent())
	{
		// TextCursor is not shown during *mouse* selection
		return;
	}

	// указатель на настройки разделяемые по приложениям
	mp_Set = gpSet->GetAppSettings(mp_RCon->GetActiveAppSettingsId());

	//------------------------------------------------------------------------
	///| Drawing cursor |/////////////////////////////////////////////////////
	//------------------------------------------------------------------------
	Cursor.isVisiblePrevFromInfo = _bool(cinf.bVisible);
	BOOL lbUpdateTick = FALSE;
	bool bForeground = mp_ConEmu->isMeForeground();
	bool bConVisible = isVisible();
	bool bConActive = isActive(false); // а тут именно Active и именно false!
	bool bGroupActive = bConActive || (bConVisible && isGroupedInput());
	bool bIsAlive = mp_RCon ? (mp_RCon->isAlive() || !mp_RCon->isFar(TRUE)) : false; // не мигать, если фар "думает"

	// Если курсор (в консоли) видим, и находится в видимой области (при прокрутке)
	if ((cinf.bVisible || GetCursor(false).Invisible) && isCursorValid)
	{
		if (!cinf.bVisible)
			bForeground = false;
		if (!GetCursor(bGroupActive && bForeground).isBlinking)
		{
			Cursor.isVisible = true; // Видим всегда (даже в неактивной консоли), не мигает

			if (Cursor.isPrevBackground == bForeground)
			{
				lRes = true;
			}
		}
		else
		{
			// Смена позиции курсора - его нужно обновить, если окно активно
			if ((Cursor.x != csbi.dwCursorPosition.X) || (Cursor.y != csbi.dwCursorPosition.Y))
			{
				Cursor.isVisible = bConVisible;

				if (Cursor.isVisible) lRes = true;  //force, pos changed

				lbUpdateTick = TRUE;
			}
			else
			{
				// Настало время мигания
				if ((GetTickCount() - Cursor.nLastBlink) > Cursor.nBlinkTime)
				{
					if (mp_ConEmu->isRightClickingPaint())
					{
						// Зажата правая кнопка мышки, начата отрисовка кружочка
						Cursor.isVisible = false;
					}
					else if (Cursor.isPrevBackground && bForeground)
					{
						// после "неактивного" курсора - сразу рисовать активный, только потом - мигать
						Cursor.isVisible = true;
						lRes = true;
					}
					else
					{
						WARNING("DoubleView: в неактивной но видимой консоли тоже отрисовать, но блоком");
						Cursor.isVisible = bConVisible && (!Cursor.isVisible || !bIsAlive);
					}

					lbUpdateTick = TRUE;
				}
			}
		}
	}
	else
	{
		// Иначе - его нужно спрятать (курсор скрыт в консоли, или ушел за границы экрана)
		Cursor.isVisible = false;
	}

	// Смена видимости палки в ConEmu
	if (Cursor.isVisible != Cursor.isVisiblePrev && !mp_ConEmu->isRightClickingPaint())
	{
		lRes = true;
		lbUpdateTick = TRUE;
	}

	if (Cursor.isVisible)
		Cursor.isPrevBackground = !bForeground;

	// Сохраним новое положение
	//Cursor.x = csbi.dwCursorPosition.X;
	//Cursor.y = csbi.dwCursorPosition.Y;
	Cursor.isVisiblePrev = Cursor.isVisible;

	if (lbUpdateTick)
	{
		Cursor.nLastBlink = GetTickCount();
	}

	// From the active console - trigger cursor redraw in other VISIBLE panes
	if (bConActive && isGroupedInput())
	{
		CVConGroup::EnumVCon(evf_Visible, UpdateCursorGroup, (LPARAM)(CVirtualConsole*)this);
	}
}

bool CVirtualConsole::Blit(HDC hPaintDC, int anX, int anY, int anShowWidth, int anShowHeight)
{
	bool lbRc;

	/* TODO: BitBlt has large speed problems when size of picture exceeds some limits */

	//if ((anShowWidth * anShowHeight) <= 450000)
	{
		lbRc = (::BitBlt(hPaintDC, anX, anY, anShowWidth, anShowHeight, (HDC)m_DC, 0, 0, SRCCOPY) != FALSE);
	}
	//else
	//{
	//	int nPartHeight = std::min((450000 / anShowWidth),200);
	//	for (int Y = anY, Ysrc = 0;
	//			Y < anShowHeight;
	//			Y += nPartHeight, Ysrc += nPartHeight)
	//	{
	//		int nHeight = std::min(nPartHeight, (anShowHeight - Y));
	//		lbRc = (::BitBlt(hPaintDC, anX, Y, anShowWidth, nHeight, (HDC)m_DC, 0, Ysrc, SRCCOPY) != FALSE);
	//	}
	//}

	return lbRc;
}

bool CVirtualConsole::StretchPaint(HDC hPaintDC, int anX, int anY, int anShowWidth, int anShowHeight)
{
	if (!this)
		return false;

	if (!isMainThread())
	{
		_ASSERTE(isMainThread());
		return false;
	}

	if (!(HDC)m_DC && mpsz_ConChar)
	{
		Update();
	}

	bool bPaintRC = false;

	if ((HDC)m_DC)
	{
		SetStretchBltMode(hPaintDC, HALFTONE);
		bPaintRC = _bool(StretchBlt(hPaintDC, anX, anY, anShowWidth, anShowHeight, (HDC)m_DC, 0, 0, m_Sizes.Width, m_Sizes.Height, SRCCOPY));
	}
	else
	{
		WARNING("Похоже для ChildGui - mp_RCon->hGuiWnd - это происходит хронически...");
		_ASSERTE((HDC)m_DC!=nullptr);
	}

	return bPaintRC;
}

HBRUSH CVirtualConsole::CreateBackBrush(bool bGuiVisible, bool& rbNonSystem)
{
	HBRUSH hBr = nullptr;

	if (bGuiVisible)
	{
		hBr = GetSysColorBrush(COLOR_APPWORKSPACE);
		rbNonSystem = false;
	}
	else
	{
		// Fill with coloridx 0
		const int nBackColorIdx = mp_RCon->GetDefaultBackColorIdx(); // Black
		//#ifdef _DEBUG
		//nBackColorIdx = 2; // Green
		//#endif

		hBr = CreateSolidBrush(m_Colors[nBackColorIdx]);
		rbNonSystem = (hBr != nullptr);
	}

	return hBr;
}

// PaintRect may be specified for stretching
bool CVirtualConsole::PrintClient(HDC hPrintDc, bool bAllowRepaint, const LPRECT PaintRect)
{
	BOOL lbBltRc = FALSE;
	CVirtualConsole* pVCon = this;
	CVConGuard guard(pVCon);

	if (!pVCon)
		return false;

	if (!isMainThread())
	{
		_ASSERTE(isMainThread());
		return false;
	}

	if (((HDC)m_DC) != nullptr)
	{
		RECT rcClient = GetDcClientRect();
		int  nPrintX = 0, nPrintY = 0;

		bool bNeedStretch = false;
		if (PaintRect != nullptr)
		{
			bNeedStretch = (((rcClient.right-rcClient.left) != (PaintRect->right-PaintRect->left))
				|| ((rcClient.bottom-rcClient.top) != (PaintRect->bottom-PaintRect->top)));
			nPrintX = PaintRect->left;
			nPrintY = PaintRect->top;
		}

		if (bNeedStretch)
		{
			_ASSERTE(PaintRect!=nullptr);
			lbBltRc = StretchPaint(hPrintDc, nPrintX, nPrintY, PaintRect->right-PaintRect->left, PaintRect->bottom-PaintRect->top);
		}
		else
		{
			if (bAllowRepaint)
			{
				Update(mb_RequiredForceUpdate);
			}

			lbBltRc = Blit(hPrintDc, nPrintX, nPrintY, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
		}
	}

	UNREFERENCED_PARAMETER(lbBltRc);
	return (lbBltRc != 0);
}

// hPaintDc - это DC ViewWnd!!!
// rcClient - это место, куда нужно "положить" битмап. может быть произвольным!
void CVirtualConsole::PaintVCon(HDC hPaintDc)
{
	// Если "завис" PostUpdate
	if (mp_ConEmu->mp_TabBar->NeedPostUpdate())
		mp_ConEmu->mp_TabBar->Update();

	if (!mh_WndDC)
	{
		_ASSERTE(mh_WndDC!=nullptr);
		return;
	}

	CVConGuard guard(this);

	if (!isMainThread())
	{
		_ASSERTE(isMainThread());
		return;
	}

	RECT rcClient = GetDcClientRect();

	_ASSERTE(hPaintDc);
	_ASSERTE(rcClient.left!=rcClient.right && rcClient.top!=rcClient.bottom);

	WARNING("TODO: Need to offset mrc_Client if padding exists");
	mrc_Client = rcClient;
	WARNING("TODO: Need to calculate mrc_Back = {0,0}-{BackWidth,BackHeight}");
	mrc_Back = mrc_Client;

	bool lbSimpleBlack = false, lbGuiVisible = false;

	if (!guard.VCon())
		lbSimpleBlack = true;
	else if (!mp_RCon)
		lbSimpleBlack = true;
	else if (!mp_RCon->ConWnd() && !mp_RCon->GuiWnd())
		lbSimpleBlack = true;
	else if ((lbGuiVisible = mp_RCon->isGuiOverCon()))
		lbSimpleBlack = true;
#ifdef _DEBUG
	else
	{
		int nConNo = mp_ConEmu->isVConValid(this);
		nConNo = nConNo;
	}
#endif

	if (gpSet->isLogging(4))
	{
		wchar_t szLog[80]; swprintf_c(szLog, L"VCon[%i]: PaintVCon started");
		LogString(szLog);
	}

	if (lbSimpleBlack)
	{
		guard.VCon()->PaintVConSimple(hPaintDc, rcClient, lbGuiVisible);

		if (guard.VCon() && mp_Ghost)
			mp_Ghost->UpdateTabSnapshot(); //CreateTabSnapshoot(hPaintDc, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
	}
	else if (gpConEmu->IsWindowModeChanging())
	{
		if (!mb_PaintSkippedLogged)
		{
			mb_PaintSkippedLogged = true;
			LogString(L"PaintVCon are skipped till Min/Max/Restore/Lock end");
		}
	}
	else
	{
		if (mb_PaintSkippedLogged)
			mb_PaintSkippedLogged = false;

		PaintVConNormal(hPaintDc, rcClient);

		if (mp_Ghost)
			mp_Ghost->UpdateTabSnapshot(); //CreateTabSnapshoot(hPaintDc, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);

		// Debug rects and (scrolllock)counter
		PaintVConDebug(hPaintDc, rcClient);
	}

	if (gpSet->isLogging(4))
	{
		wchar_t szLog[80]; swprintf_c(szLog, L"VCon[%i]: PaintVCon finished");
		LogString(szLog);
	}
}

void CVirtualConsole::PaintVConSimple(HDC hPaintDc, RECT rcClient, bool bGuiVisible)
{
	if (!this)
	{
		_ASSERTE(FALSE && "Called with nullptr pVCon");
		COLORREF crBack = gpSet->GetColors(-1, isFade)[0];
		HBRUSH hBr = CreateSolidBrush(crBack);
		FillRect(hPaintDc, &rcClient, hBr);
		DeleteObject(hBr);
		return;
	}

	// Сброс блокировки, если была
	LockDcRect(false);

	const auto& colors = GetColors();

	bool lbDelBrush = false;
	HBRUSH hBr = CreateBackBrush(bGuiVisible, lbDelBrush);

	//#ifndef SKIP_ALL_FILLRECT
	FillRect(hPaintDc, &rcClient, hBr);
	//#endif

	TODO("Переделать на StatusBar, если gpSet->isStatusBarShow");
	if (!bGuiVisible)
	{
		CFontPtr pFont;
		gpFontMgr->QueryFont(fnt_Normal, this, pFont);
		CEDC cePaintDc(hPaintDc);
		cePaintDc.SelectFont(pFont);
		LPCWSTR pszStarting = L"Initializing ConEmu.";

		// 120721 - если показана статусная строка - не будем писать в саму консоль?
		if (gpSet->isStatusBarShow)
			pszStarting = nullptr;
		else if (mp_ConEmu->isProcessCreated())
			pszStarting = L"No consoles";
		else if (this && mp_RCon)
			pszStarting = mp_RCon->GetConStatus();

		if (pszStarting != nullptr)
		{
			UINT nFlags = ETO_CLIPPED;
			cePaintDc.SetTextColor(colors[7]);
			cePaintDc.SetBkColor(colors[0]);
			cePaintDc.TextDraw(rcClient.left, rcClient.top, nFlags, &rcClient,
				        pszStarting, _tcslen(pszStarting), 0);
		}

		cePaintDc.SelectFont(nullptr);
	}

	if (lbDelBrush)
		DeleteObject(hBr);

	if (mp_Ghost)
		mp_Ghost->UpdateTabSnapshot(TRUE); //CreateTabSnapshoot(hPaintDc, rcClient.right-rcClient.left, rcClient.bottom-rcClient.top);
}

void CVirtualConsole::PaintVConNormal(HDC hPaintDc, RECT rcClient)
{
	if (isActive(false))
		gpSetCls->Performance(tPerfFPS, TRUE); // считается по своему

	// Проверить, не сменилась ли битность с последнего раза
	DWORD nBits = GetDeviceCaps(hPaintDc, BITSPIXEL);

	if (mn_LastBitsPixel != nBits)
	{
		mb_RequiredForceUpdate = true;
	}

	mb_InPaintCall = TRUE;

	if (gbNoDblBuffer)
		Update(true, &hPaintDc);
	else
		Update(mb_RequiredForceUpdate);

	m_HighlightInfo.mb_ChangeDetected = false;
	mb_InPaintCall = FALSE;
	//BOOL lbExcept = FALSE;
	RECT client = rcClient;
	//PAINTSTRUCT ps;
	//HDC hPaintDc = nullptr;

	//Get ClientRect('ghWnd DC', &client);

	//120807 - уберем излишние вызовы ресайзов
#if 0
	if (!mp_ConEmu->isNtvdm())
	{
		// rcClient не учитывает возможную полосу прокрутки
		RECT rcFull; GetWindowRect(mh_WndDC, &rcFull);
		// после глюков с ресайзом могут быть проблемы с размером окна DC
		if ((rcFull.right - rcFull.left) < (LONG)Width || (rcFull.bottom - client.top) < (LONG)Height)
		{
			WARNING("Зацикливается. Вызывает PaintVCon, который вызывает OnSize. В итоге - 100% загрузки проц.");
			mp_ConEmu->OnSize(false); // Только ресайз дочерних окон
		}
	}
#endif

	//MSectionLock S; //&csDC, &ncsTDC);
	//RECT rcUpdateRect = {0};
	//BOOL lbInval = GetUpdateRect('ghWnd DC', &rcUpdateRect, FALSE);
	//if (lbInval)
	//  hPaintDc = BeginPaint('ghWnd DC', &ps);
	//else
	//	hPaintDc = GetDC('ghWnd DC');
	// Если окно больше готового DC - залить края (справа/снизу) фоновым цветом
	HBRUSH hBr = nullptr;

	if (((ULONG)(client.right-client.left)) > m_Sizes.Width)
	{
		if (!hBr) hBr = CreateSolidBrush(m_Colors[mn_BackColorIdx]);

		RECT rcFill = MakeRect(client.left + m_Sizes.Width, client.top, client.right, client.bottom);
#ifndef SKIP_ALL_FILLRECT
		FillRect(hPaintDc, &rcFill, hBr);
#endif
		client.right = client.left + m_Sizes.Width;
	}

	if (((ULONG)(client.bottom-client.top)) > m_Sizes.Height)
	{
		if (!hBr) hBr = CreateSolidBrush(m_Colors[mn_BackColorIdx]);

		RECT rcFill = MakeRect(client.left, client.top + m_Sizes.Height, client.right, client.bottom);
#ifndef SKIP_ALL_FILLRECT
		FillRect(hPaintDc, &rcFill, hBr);
#endif
		client.bottom = client.top + m_Sizes.Height;
	}

	if (hBr) { DeleteObject(hBr); hBr = nullptr; }

	//BOOL lbPaintLocked = FALSE;

	//if (!mb_PaintRequested)    // Если PaintVCon вызыван НЕ из Update (а например ресайз, или над нашим окошком что-то протащили).
	//{
	//	//if (S.Lock(&csDC, 200))  // но по таймауту, чтобы не повисли ненароком
	//	//	mb_PaintLocked = lbPaintLocked = TRUE;
	//}

	//bool bFading = false;
	bool lbLeftExists = (m_LeftPanelView.hWnd && IsWindowVisible(m_LeftPanelView.hWnd));

	if (lbLeftExists && !mb_LeftPanelRedraw)
	{
		DWORD n = GetWindowLong(m_LeftPanelView.hWnd, 16*4); //-V112

		if (n != (isFade ? 2 : 1)) mb_LeftPanelRedraw = TRUE;
	}

	bool lbRightExists = (m_RightPanelView.hWnd && IsWindowVisible(m_RightPanelView.hWnd));

	if (lbRightExists && !mb_RightPanelRedraw)
	{
		DWORD n = GetWindowLong(m_RightPanelView.hWnd, 16*4); //-V112

		if (n != (isFade ? 2 : 1)) mb_RightPanelRedraw = TRUE;
	}

	if (mb_LeftPanelRedraw)
	{
		UpdatePanelView(TRUE); mb_LeftPanelRedraw = FALSE;

		if (lbLeftExists)
		{
			InvalidateRect(m_LeftPanelView.hWnd, nullptr, FALSE);
		}
	}
	else if (mb_DialogsChanged)
	{
		UpdatePanelRgn(TRUE, FALSE, FALSE);
	}

	if (mb_RightPanelRedraw)
	{
		UpdatePanelView(FALSE); mb_RightPanelRedraw = FALSE;

		if (lbRightExists)
		{
			InvalidateRect(m_RightPanelView.hWnd, nullptr, FALSE);
		}
	}
	else if (mb_DialogsChanged)
	{
		UpdatePanelRgn(FALSE, FALSE, FALSE);
	}

	// Собственно, копирование готового bitmap
	if (!gbNoDblBuffer)
	{
		// Normal mode
		if (gpSet->isLogging(3)) mp_RCon->LogString("Blitting to Display");

		gpSetCls->Performance(tPerfBlt, FALSE);

		BOOL lbBltRc;
		if (!m_LockDc.bLocked)
		{
			lbBltRc = Blit(
				hPaintDc, client.left, client.top, client.right-client.left, client.bottom-client.top
				);
		}
		else
		{
			RECT rc = m_LockDc.rcScreen;
			if (client.top < rc.top)
				lbBltRc = BitBlt(
					hPaintDc, client.left, client.top, client.right-client.left, rc.top-client.top,
					(HDC)m_DC, 0, 0,
					SRCCOPY);
			if (client.left < rc.left)
				lbBltRc = BitBlt(
					hPaintDc, client.left, rc.top, rc.left-client.left, client.bottom-rc.top,
					(HDC)m_DC, 0, rc.top-client.top,
					SRCCOPY);
			if (client.right > rc.right)
				lbBltRc = BitBlt(
					hPaintDc, rc.right, rc.top, client.right-rc.right, rc.bottom-rc.top,
					(HDC)m_DC, rc.right-client.left, rc.top-client.top,
					SRCCOPY);
			if (client.bottom > rc.bottom)
				lbBltRc = BitBlt(
					hPaintDc, client.left, rc.bottom, client.right-client.left, client.bottom-rc.bottom,
					(HDC)m_DC, 0, rc.bottom-client.top,
					SRCCOPY);

			TODO("Можно бы еще восстановить то, что нарисовала на нашем DC консольная программа");
		}
		UNREFERENCED_PARAMETER(lbBltRc);

		//#ifdef _DEBUG
		//MoveToEx(hPaintDc, client.left, client.top, nullptr);
		//LineTo(hPaintDc, client.right-client.left, client.bottom-client.top);
		//FillRect(hPaintDc, &rcClient, (HBRUSH)GetStockObject(WHITE_BRUSH));
		//#endif

		CheckTransparent();

		gpSetCls->Performance(tPerfBlt, TRUE);
	}
	else
	{
		GdiSetBatchLimit(1); // отключить буферизацию вывода для текущей нити
		GdiFlush();
		// Рисуем сразу на канвасе, без буферизации
		// Это для отладки отрисовки, поэтому недопустимы табы и прочее
		_ASSERTE(gpSet->isTabs == 0);
		Update(true, &hPaintDc);
	}

	// Запомнить последнюю битность дисплея
	mn_LastBitsPixel = nBits;

	if (gbNoDblBuffer) GdiSetBatchLimit(0);  // вернуть стандартный режим

	if (Cursor.isVisible && isCursorValid
		&& (cinf.bVisible || GetCursor(false).Invisible))
	{
		if (mpsz_ConChar && mpn_ConAttrEx)
		{
			CFontPtr pFont;
			gpFontMgr->QueryFont(fnt_Normal, this, pFont);
			CEDC cePaintDc(hPaintDc);
			cePaintDc.SelectFont(pFont);
			MSectionLock SCON; SCON.Lock(&csCON);

			int CurChar = csbi.dwCursorPosition.Y * m_Sizes.TextWidth + csbi.dwCursorPosition.X;
			Cursor.ch = mpsz_ConChar[CurChar];
			Cursor.foreColor = mpn_ConAttrEx[CurChar].crForeColor;
			Cursor.bgColor = mpn_ConAttrEx[CurChar].crBackColor;

			UpdateCursorDraw(hPaintDc, rcClient, csbi.dwCursorPosition, cinf.dwSize);

			Cursor.isVisiblePrev = Cursor.isVisible;
			cePaintDc.SelectFont(nullptr);
			SCON.Unlock();
		}
	}
}

void CVirtualConsole::PaintVConDebug(HDC hPaintDc, RECT rcClient)
{
	if (mp_RCon)
	{
		#ifdef DEBUGDRAW_RCONPOS
		if (GetKeyState(DEBUGDRAW_RCONPOS) & 1)
		{
			// Прямоугольник, соответствующий положению окна RealConsole
			HWND hConWnd = mp_RCon->hConWnd;
			RECT rcCon; GetWindowRect(hConWnd, &rcCon);
			MapWindowPoints(nullptr, GetView(), (LPPOINT)&rcCon, 2);
			SelectObject(hPaintDc, GetStockObject(WHITE_PEN));
			SelectObject(hPaintDc, GetStockObject(HOLLOW_BRUSH));
			Rectangle(hPaintDc, rcCon.left, rcCon.top, rcCon.right, rcCon.bottom);
		}
		#endif

		if (gbDebugShowRects
				#ifdef DEBUGDRAW_DIALOGS
				|| (GetKeyState(DEBUGDRAW_DIALOGS) & 1)
				#endif
		  )
		{
			// Прямоугольники найденных диалогов
			//SMALL_RECT rcFound[MAX_DETECTED_DIALOGS]; DWORD nDlgFlags[MAX_DETECTED_DIALOGS];
			//int nFound = mp_RCon->GetDetectedDialogs(MAX_DETECTED_DIALOGS, rcFound, nDlgFlags);
			const DetectedDialogs* pDlg = nullptr;
			// Если включены PanelView - попробовать взять диалоги у него
			//#ifdef _DEBUG
			MFileMapping<DetectedDialogs> pvMap;

			if (m_LeftPanelView.bRegister || m_RightPanelView.bRegister)
			{
				pvMap.InitName(CEPANELDLGMAPNAME, mp_RCon->GetFarPID(TRUE));
				pDlg = pvMap.Open();
			}
			//#endif

			if (!pDlg) pDlg = mp_RCon->GetDetector()->GetDetectedDialogsPtr();

			// Поехали
			HPEN hFrame = CreatePen(PS_SOLID, 1, RGB(255,0,255));
			HPEN hPanel = CreatePen(PS_SOLID, 2, RGB(255,255,0));
			HPEN hQSearch = CreatePen(PS_SOLID, 1, RGB(255,255,0));
			HPEN hUCharMap = CreatePen(PS_SOLID, 1, RGB(255,255,0));
			HPEN hActiveMenu = (HPEN)GetStockObject(WHITE_PEN);
			HPEN hOldPen = (HPEN)SelectObject(hPaintDc, hFrame);
			HBRUSH hOldBr = (HBRUSH)SelectObject(hPaintDc, GetStockObject(HOLLOW_BRUSH));
			HPEN hRed = CreatePen(PS_SOLID, 1, 255);
			HPEN hDash = CreatePen(PS_DOT, 1, 0xFFFFFF);
			HFONT hSmall = CreateFont(-7,0,0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,NONANTIALIASED_QUALITY,DEFAULT_PITCH,L"Small fonts");
			HFONT hOldFont = (HFONT)SelectObject(hPaintDc, hSmall);
			SetTextColor(hPaintDc, 0xFFFFFF);
			SetBkColor(hPaintDc, 0);

			for(int i = 0; i < pDlg->Count; i++)
			{
				int n = 1;
				DWORD nFlags = pDlg->DlgFlags[i];

				if (i == (MAX_DETECTED_DIALOGS-1))
					SelectObject(hPaintDc, hRed);
				else if ((nFlags & FR_ACTIVEMENUBAR) == FR_ACTIVEMENUBAR)
					SelectObject(hPaintDc, hActiveMenu);
				else if ((nFlags & FR_QSEARCH))
					SelectObject(hPaintDc, hQSearch);
				else if ((nFlags & (FR_UCHARMAP|FR_UCHARMAPGLYPH)))
					SelectObject(hPaintDc, hUCharMap);
				else if ((nFlags & (FR_LEFTPANEL|FR_RIGHTPANEL|FR_FULLPANEL)))
					SelectObject(hPaintDc, hPanel);
				else if ((nFlags & FR_HASBORDER))
					SelectObject(hPaintDc, hFrame);
				else
				{
					SelectObject(hPaintDc, hDash);
					n = 0;
				}

				//
				POINT pt[2];
				pt[0] = ConsoleToClient(pDlg->Rects[i].Left, pDlg->Rects[i].Top);
				pt[1] = ConsoleToClient(pDlg->Rects[i].Right+1, pDlg->Rects[i].Bottom+1);
				//MapWindowPoints(GetView(), ghWnd, pt, 2);
				Rectangle(hPaintDc, pt[0].x+n, pt[0].y+n, pt[1].x-n, pt[1].y-n);
				wchar_t szCoord[32]; swprintf_c(szCoord, L"%ix%i", pDlg->Rects[i].Left, pDlg->Rects[i].Top);
				TextOut(hPaintDc, pt[0].x+1, pt[0].y+1, szCoord, _tcslen(szCoord));
			}

			SelectObject(hPaintDc, hOldBr);
			SelectObject(hPaintDc, hOldPen);
			SelectObject(hPaintDc, hOldFont);
			DeleteObject(hRed);
			DeleteObject(hPanel);
			DeleteObject(hQSearch);
			DeleteObject(hUCharMap);
			DeleteObject(hDash);
			DeleteObject(hFrame);
			DeleteObject(hSmall);
		}
	}


	//if (lbExcept)
	//{
	//	CRealConsole::Box(_T("Exception triggered in CVirtualConsole::PaintVCon"), MB_RETRYCANCEL);
	//}

	//  if (hPaintDc && 'ghWnd DC') {
	//if (lbInval)
	//	EndPaint('ghWnd DC', &ps);
	//else
	//	ReleaseDC('ghWnd DC', hPaintDc);
	//  }
	//gpSetCls->Performance(tPerfFPS, FALSE); // Обратный
	mb_DialogsChanged = FALSE; // сбросим, флажок больше не нужен

	#ifdef _DEBUG
	if ((GetKeyState(VK_SCROLL) & 1) && (GetKeyState(VK_CAPITAL) & 1))
	{
		mp_ConEmu->DebugStep(L"ConEmu: Sleeping in CVirtualConsole::PaintVCon for 1s");
		Sleep(1000);
		gpSetCls->PostUpdateCounters(true); // Force update "Info" counters
		mp_ConEmu->DebugStep(nullptr);
	}
	#endif
}

void CVirtualConsole::UpdateInfo()
{
	if (!ghOpWnd || !this)
		return;

	if (!isMainThread())
		return;

	HWND hInfo = gpSetCls->GetPage(thi_Info);
	if (hInfo == nullptr)
		return;

	wchar_t szSize[128];

	const RECT rcClient = GetDcClientRect();
	swprintf_c(szSize, _T("%ix%i / %ix%i"), RectWidth(rcClient), RectHeight(rcClient), m_Sizes.Width, m_Sizes.Height);
	SetDlgItemText(hInfo, tDCSize, szSize);

	if (!mp_RCon)
	{
		const auto szNone = CLngRc::getRsrc(lng_SetPgInfoNone/*"(None)"*/);
		SetDlgItemText(hInfo, tConBufferChr, szNone);
		SetDlgItemText(hInfo, tConSizeChr, szNone);
		SetDlgItemText(hInfo, tConLeftTop, szNone);
		SetDlgItemText(hInfo, tCursorPos, szNone);
		SetDlgItemText(hInfo, tPanelLeft,  szNone);
		SetDlgItemText(hInfo, tPanelRight, szNone);
	}
	else
	{
		swprintf_c(szSize, _T("%ix%i"), mp_RCon->BufferWidth(), mp_RCon->BufferHeight());
		SetDlgItemText(hInfo, tConBufferChr, szSize);

		swprintf_c(szSize, _T("%ix%i"), mp_RCon->TextWidth(), mp_RCon->TextHeight());
		SetDlgItemText(hInfo, tConSizeChr, szSize);

		CONSOLE_SCREEN_BUFFER_INFO sbi{};
		mp_RCon->GetConsoleScreenBufferInfo(&sbi);
		swprintf_c(szSize, _T("%ix%i"), static_cast<int>(sbi.srWindow.Left)+1, static_cast<int>(sbi.srWindow.Top)+1);
		SetDlgItemText(hInfo, tConLeftTop, szSize);

		ConsoleInfoArg cursorInfo = {};
		mp_RCon->GetConsoleInfo(&cursorInfo);
		CSetPgInfo::FillCursorInfo(hInfo, &cursorInfo);

		RECT rcPanel;
		RCon()->GetPanelRect(FALSE, &rcPanel);
		if (rcPanel.right>rcPanel.left)
			swprintf_c(szSize, L"(%i, %i)-(%i, %i), %ix%i", rcPanel.left+1, rcPanel.top+1, rcPanel.right+1, rcPanel.bottom+1, rcPanel.right-rcPanel.left+1, rcPanel.bottom-rcPanel.top+1);
		else
			wcscpy_c(szSize, L"<Absent>");
		SetDlgItemText(hInfo, tPanelLeft, szSize);

		RCon()->GetPanelRect(TRUE, &rcPanel);
		if (rcPanel.right>rcPanel.left)
			swprintf_c(szSize, L"(%i, %i)-(%i, %i), %ix%i", rcPanel.left+1, rcPanel.top+1, rcPanel.right+1, rcPanel.bottom+1, rcPanel.right-rcPanel.left+1, rcPanel.bottom-rcPanel.top+1);
		else
			wcscpy_c(szSize, L"<Absent>");
		SetDlgItemText(hInfo, tPanelRight, szSize);
	}
}

LONG CVirtualConsole::GetVConWidth()
{
	_ASSERTE(this && m_Sizes.Width);
	return (LONG)m_Sizes.Width;
}

LONG CVirtualConsole::GetVConHeight()
{
	_ASSERTE(this && m_Sizes.Height);
	return (LONG)m_Sizes.Height;
}

POINT CVirtualConsole::GetVConOffset()
{
	return POINT{LONG(m_Sizes.OffsetX), LONG(m_Sizes.OffsetY)};
}

SIZE CVirtualConsole::GetBackSize()
{
	return SIZE{LONG(m_Sizes.BackWidth), LONG(m_Sizes.BackHeight)};
}

LONG CVirtualConsole::GetTextWidth()
{
	_ASSERTE(this && m_Sizes.TextWidth);
	return (LONG)m_Sizes.TextWidth;
}

LONG CVirtualConsole::GetTextHeight()
{
	_ASSERTE(this && m_Sizes.TextHeight);
	return (LONG)m_Sizes.TextHeight;
}

SIZE CVirtualConsole::GetCellSize()
{
	SIZE sz = {m_Sizes.nFontWidth, m_Sizes.nFontHeight};
	return sz;
}

RECT CVirtualConsole::GetRect()
{
	RECT rc;

	if (m_Sizes.Width == 0 || m_Sizes.Height == 0)    // если консоль еще не загрузилась - прикидываем по размеру GUI окна
	{
		//rc = MakeRect(winSize.X, winSize.Y);
		//RECT rcWnd; Get ClientRect(ghWnd, &rcWnd);
		RECT rcCon = mp_ConEmu->CalcRect(CER_CONSOLE_CUR, this);
		RECT rcDC = mp_ConEmu->CalcRect(CER_DC, rcCon, CER_CONSOLE_CUR, this);
		rc = MakeRect(rcDC.right, rcDC.bottom);
	}
	else
	{
		rc = MakeRect(m_Sizes.Width, m_Sizes.Height);
	}

	return rc;
}

RECT CVirtualConsole::GetDcClientRect()
{
	BOOL lbRc;
	RECT rcClient = {};
	if (gpSet->isAlwaysShowScrollbar == 1)
	{
		// Режим "Полоса прокрутки видна всегда"
		lbRc = ::GetClientRect(mh_WndDC, &rcClient);
	}
	else
	{
		// Полоса прокрутки может перекрывать часть рабочей области, поэтому
		lbRc = ::GetWindowRect(mh_WndDC, &rcClient);
		// И приводим к клиентским координатам
		rcClient.right -= rcClient.left;
		rcClient.bottom -= rcClient.top;
		rcClient.left = rcClient.top = 0;
	}
	UNREFERENCED_PARAMETER(lbRc);
	return rcClient;
}

void CVirtualConsole::OnFontChanged()
{
	if (!this) return;

	mb_RequiredForceUpdate = true;
	//ClearPartBrushes();
}

// This function is expected to be called on resize GUI (ConEmu window)
void CVirtualConsole::OnConsoleSizeChanged()
{
	MSetter lSet(&mb_InPaintCall); // Avoid spare Invalidate-s
	Update(mb_RequiredForceUpdate);
}

void CVirtualConsole::OnConsoleSizeReset(USHORT sizeX, USHORT sizeY)
{
	#ifdef _DEBUG // due to unittests
	// Should could happen only during creation of new console (e.g., new tab panel appears)
	_ASSERTE(mp_RCon && ((mp_RCon->ConWnd()==nullptr) || mp_RCon->mb_InCloseConsole));
	#endif
	// И по идее, DC еще создан быть не должен был
	if ((m_Sizes.Width == 0) && (m_Sizes.Height == 0))
	{
		m_Sizes.TextWidth = sizeX;
		m_Sizes.TextHeight = sizeY;
	}
	else
	{
		#ifdef _DEBUG // due to unittests
		_ASSERTE((m_Sizes.Width==0 && m_Sizes.Height==0) || mp_RCon->mb_InCloseConsole);
		#endif
	}
}

POINT CVirtualConsole::ConsoleToClient(LONG x, LONG y)
{
	POINT pt = {0,0};

	if (!this)
	{
		pt.y = y * gpFontMgr->FontHeight();
		pt.x = x * gpFontMgr->FontWidth();
		return pt;
	}

	pt.y = y * m_Sizes.nFontHeight;

	if (x>0)
	{
		if (ConCharX && y >= 0 && y < (int)m_Sizes.TextHeight && x < (int)m_Sizes.TextWidth)
		{
			pt.x = ConCharX[y*m_Sizes.TextWidth + x-1];

			if (x && !pt.x)
			{
				TODO("При вызове не из основной нити - ConCharX может быть обнулен");
				//2010-06-07
				// 	CRealConsole::RConServerThread -->
				//  CRealConsole::ServerThreadCommand -->
				//  CVirtualConsole::RegisterPanelView -->
				//  CVirtualConsole::UpdatePanelView(int abLeftPanel=1, int abOnRegister=1) -->
				//  CVirtualConsole::UpdatePanelRgn(int abLeftPanel=1, int abTestOnly=0, int abOnRegister=1) -->
				//  и вот тут она обломалась. ConCharX оказался обнулен?
				Sleep(1);
				pt.x = ConCharX[y*m_Sizes.TextWidth + x-1];
				_ASSERTE(pt.x || x==0);
			}
		}
		else
		{
			pt.x = x * m_Sizes.nFontWidth;
		}
	}

	return pt;
}

// Only here we can precisely calc exact coords
COORD CVirtualConsole::ClientToConsole(LONG x, LONG y, bool StrictMonospace/*=false*/)
{
	COORD cr = {0,0};

	if (!this)
	{
		cr.Y = MakeShort(y / gpFontMgr->FontHeight());
		cr.X = MakeShort(x / gpFontMgr->FontWidth());
		return cr;
	}

	_ASSERTE(m_Sizes.nFontWidth!=0 && m_Sizes.nFontHeight!=0);

	// Сначала приблизительный пересчет по размерам шрифта
	if (m_Sizes.nFontHeight)
		cr.Y = MakeShort(y / m_Sizes.nFontHeight);

	if (m_Sizes.nFontWidth)
		cr.X = MakeShort(x / m_Sizes.nFontWidth);

	if (!StrictMonospace)
	{
		// А теперь, если возможно, уточним X координату
		if (x > 0)
		{
			x++; // иначе сбивается на один пиксел влево

			if (ConCharX && cr.Y >= 0 && cr.Y < (int)m_Sizes.TextHeight)
			{
				DWORD* ConCharXLine = ConCharX + cr.Y * m_Sizes.TextWidth;

				for(unsigned i = 0; i < m_Sizes.TextWidth; i++, ConCharXLine++)
				{
					if (((int)*ConCharXLine) >= x)
					{
						if (cr.X != (int)i)
						{
							#ifdef _DEBUG
							wchar_t szDbg[120]; swprintf_c(szDbg, L"Coord corrected from {%i-%i} to {%i-%i}",
														  cr.X, cr.Y, i, cr.Y);
							DEBUGSTRCOORD(szDbg);
							#endif

							cr.X = i;
						}

						break;
					}
				}
			}
		}
	}

	return cr;
}

bool CVirtualConsole::FindChanges(int row, const wchar_t* ConCharLine, const CharAttr* ConAttrLine, const wchar_t* ConCharLine2, const CharAttr* ConAttrLine2)
{
	// UCharMap - перерисовать полностью, там может шрифт измениться
	if (gpSet->isExtendUCharMap && mrc_UCharMap.Right > mrc_UCharMap.Left)
	{
		if (row >= mrc_UCharMap.Top && row <= mrc_UCharMap.Bottom)
			return TRUE; // пока строку целиком будем рисовать
	}

	// Если изменений в строке вообще нет
	if (wmemcmp(ConCharLine, ConCharLine2, m_Sizes.TextWidth) == 0
	        && memcmp(ConAttrLine, ConAttrLine2, m_Sizes.TextWidth*sizeof(*ConAttrLine)) == 0)
		return FALSE;

	return TRUE;
}

HRGN CVirtualConsole::GetExclusionRgn()
{
	if (!gpSet->isUserScreenTransparent)
		return nullptr;

	if (mp_RCon->GetFarPID(TRUE) == 0)
		return nullptr;

	return mh_TransparentRgn;
}

COORD CVirtualConsole::FindOpaqueCell()
{
	COORD cr = {0,0};

	if (this)
	{
		// По умолчанию - сбросим
		cr.X = cr.Y = -1;

		if (mpsz_ConChar && mpn_ConAttrEx && m_Sizes.TextWidth && m_Sizes.TextHeight)
		{
			MSectionLock SCON; SCON.Lock(&csCON);
			CharAttr* pnAttr = mpn_ConAttrEx;

			// Поиск первого непрозрачного
			for (unsigned y = 0; y < m_Sizes.TextHeight; y++)
			{
				for (unsigned x = 0; x < m_Sizes.TextWidth; x++, pnAttr++)
				{
					#if 0
					if (!pnAttr[x].bTransparent)
					#else
					if (!(pnAttr->Flags & CharAttr_Transparent))
					#endif
					{
						cr.X = x; cr.Y = y;
						return cr;
					}
				}

				//pnAttr += TextWidth;
			}
		}
	}

	return cr;
}

void CVirtualConsole::OnPanelViewSettingsChanged()
{
	if (!this) return;

	if (!mp_RCon) return;

	DWORD nPID = 0;

	if ((nPID = mp_RCon->GetFarPID(TRUE)) != 0)
	{
		CConEmuPipe pipe(nPID, 1000);

		if (pipe.Init(_T("CVirtualConsole::OnPanelViewSettingsChanged"), TRUE))
		{
			CESERVER_REQ_GUICHANGED lWindows = {sizeof(CESERVER_REQ_GUICHANGED)};
			lWindows.nGuiPID = GetCurrentProcessId();
			lWindows.hLeftView = m_LeftPanelView.hWnd;
			lWindows.hRightView = m_RightPanelView.hWnd;
			pipe.Execute(CMD_GUICHANGED, &lWindows, sizeof(lWindows));
			mp_ConEmu->DebugStep(nullptr);
		}
	}

	//if (!mn_ConEmuSettingsMsg)
	//	mn_ConEmuSettingsMsg = RegisterWindowMessage(CONEMUMSG_PNLVIEWSETTINGS);
	//DWORD nPID = GetCurrentProcessId();
	//if (m_LeftPanelView.hWnd && IsWindow(m_LeftPanelView.hWnd))
	//{
	//	WARNING("Нифига не будет пересылаться, если консоль запущена под админом");
	//	PostMessage(m_LeftPanelView.hWnd, mn_ConEmuSettingsMsg, nPID, 0);
	//}
	//if (m_RightPanelView.hWnd && IsWindow(m_RightPanelView.hWnd))
	//{
	//	PostMessage(m_RightPanelView.hWnd, mn_ConEmuSettingsMsg, nPID, 0);
	//}
}

bool CVirtualConsole::IsPanelViews()
{
	if (!this) return false;

	if (m_LeftPanelView.hWnd && IsWindow(m_LeftPanelView.hWnd))
		return true;

	if (m_RightPanelView.hWnd && IsWindow(m_RightPanelView.hWnd))
		return true;

	return false;
}

bool CVirtualConsole::RegisterPanelView(PanelViewInit* ppvi)
{
	_ASSERTE(ppvi && ppvi->cbSize == sizeof(PanelViewInit));
	bool lbRc = false;
	PanelViewInit* pp = (ppvi->bLeftPanel) ? &m_LeftPanelView : &m_RightPanelView;
	BOOL lbPrevRegistered = pp->bRegister;
	*pp = *ppvi;
	// Вернуть текущую палитру GUI
	mp_ConEmu->OnPanelViewSettingsChanged(FALSE/*abSendChanges*/);
	//COLORREF *pcrNormal = gpSet->GetColors(FALSE);
	//COLORREF *pcrFade = gpSet->GetColors(TRUE);
	//for (int i=0; i<16; i++) {
	//	// через FOR чтобы с BitMask не наколоться
	//	ppvi->crPalette[i] = (pcrNormal[i]) & 0xFFFFFF;
	//	ppvi->crFadePalette[i] = (pcrFade[i]) & 0xFFFFFF;
	//}
	ppvi->bFadeColors = isFade;
	//memmove(&(ppvi->ThSet), &(gpSet->ThSet), sizeof(gpSet->ThSet));
	DWORD dwSetParentErr = 0;

	if (ppvi->bRegister)
	{
		//_ASSERTE(ppvi->bVisible); -- допустимо. Временное скрытие, например, при Ctrl+L

		// При повторной регистрации - не дергаться
		if (!lbPrevRegistered)
		{
			//for (int i=0; i<20; i++) // на всякий случай - сначала все сбросим
			//	SetWindowLong(pp->hWnd, i*4, 0);
			HWND hViewParent = GetParent(ppvi->hWnd);

			if (hViewParent != GetView())
			{
				MBoxAssert(GetView()==ghWnd);
				// -- смысла пытаться менять родителя - немного, в Win7 это и не получится
				// -- если консольный процесс запущен "Run as administrator"
				// --- но все-таки попробуем?
				//DWORD dwSetParentErr = 0;
				HWND hRc = mp_RCon->SetOtherWindowParent((HWND)ppvi->hWnd, GetView());
				UNREFERENCED_PARAMETER(hRc);
				dwSetParentErr = GetLastError();
			}
			else
			{
				lbRc = true;
			}
		}
		else
		{
			lbRc = true;

			if (ppvi->bLeftPanel)
				mb_LeftPanelRedraw = true;
			else
				mb_RightPanelRedraw = true;
		}

		if (lbRc)
		{
			// Положение и регионы (по текущему состоянию окна)
			lbRc = UpdatePanelView(_bool(ppvi->bLeftPanel), true);
			// Обновить флаг видимости
			ppvi->bVisible = pp->bVisible;
			ppvi->WorkRect = pp->WorkRect;
			// pp->WorkRect для удобства расчитывается "на +1".
			ppvi->WorkRect.bottom--; ppvi->WorkRect.right--;
			// На панелях нужно "затереть" лишние части рамок
			Update(true);

			// Если открыто окно настроек - включить в нем кнопку "Apply"
			if (ghOpWnd) gpSetCls->OnPanelViewAppeared(true);
		}
	}
	else
	{
		lbRc = true;
	}

	return lbRc;
}

//HRGN CVirtualConsole::CreateConsoleRgn(int x1, int y1, int x2, int y2, BOOL abTestOnly)
//{
//	POINT pt[2];
//	if (abTestOnly) {
//		// Интересует принципиальное пересечение прямоугольников, так что координаты не важны
//		pt[0].x = x1 << 3; pt[0].y = y1 << 3;
//		pt[1].x = x2 << 3; pt[1].y = y2 << 3;
//	} else {
//		pt[0] = ConsoleToClient(x1, y1);
//		pt[1] = ConsoleToClient(x2+1, y2+1);
//	}
//	HRGN hRgn = CreateRectRgn(pt[0].x, pt[0].y, pt[1].x, pt[1].y);
//	return hRgn;
//}

bool CVirtualConsole::CheckDialogsChanged()
{
	bool lbChanged = false;
	//SMALL_RECT rcDlg[32];
	_ASSERTE(sizeof(mrc_LastDialogs) == sizeof(mrc_Dialogs));
	//int nDlgCount = mp_RCon->GetDetectedDialogs(countof(rcDlg), rcDlg, nullptr);

	if (mn_LastDialogsCount != mn_DialogsCount)
	{
		lbChanged = true;
	}
	else if (memcmp(mrc_Dialogs, mrc_LastDialogs, mn_DialogsCount*sizeof(mrc_Dialogs[0]))!=0
		|| memcmp(mn_DialogFlags, mn_LastDialogFlags, mn_DialogsCount*sizeof(mn_DialogFlags[0]))!=0)
	{
		lbChanged = true;
	}

	if (lbChanged)
	{
		//memset(mrc_LastDialogs, 0, sizeof(mrc_LastDialogs));
		memmove(mrc_LastDialogs, mrc_Dialogs, mn_DialogsCount*sizeof(mrc_Dialogs[0]));
		memmove(mn_LastDialogFlags, mn_DialogFlags, mn_DialogsCount*sizeof(mn_DialogFlags[0]));
		mn_LastDialogsCount = mn_DialogsCount;
	}

	return lbChanged;
}

const PanelViewInit* CVirtualConsole::GetPanelView(bool abLeftPanel)
{
	if (!this) return nullptr;

	PanelViewInit* pp = abLeftPanel ? &m_LeftPanelView : &m_RightPanelView;

	if (!pp->hWnd || !IsWindow(pp->hWnd))
	{
		if (pp->hWnd)
			pp->hWnd = nullptr;

		pp->bVisible = FALSE;
		return nullptr;
	}

	return pp;
}

// Возвращает TRUE, если панель (или хотя бы часть ее ВИДИМА)
bool CVirtualConsole::UpdatePanelRgn(bool abLeftPanel, bool abTestOnly, bool abOnRegister)
{
	PanelViewInit* pp = abLeftPanel ? &m_LeftPanelView : &m_RightPanelView;

	if (!pp->hWnd || !IsWindow(pp->hWnd))
	{
		if (pp->hWnd)
			pp->hWnd = nullptr;

		pp->bVisible = FALSE;
		return FALSE;
	}
	else if (/*!pp->bVisible ||*/ !pp->bRegister)
	{
		if (IsWindowVisible(pp->hWnd))
			mp_RCon->ShowOtherWindow(pp->hWnd, SW_HIDE);
		return FALSE;
	}

	CRgnRects* pRgn = abLeftPanel ? &m_RgnLeftPanel : &m_RgnRightPanel;
	bool lbPartHidden = false;
	bool lbPanelVisible = false;
	SMALL_RECT rcDlg[MAX_DETECTED_DIALOGS]; DWORD rnDlgFlags[MAX_DETECTED_DIALOGS];
	_ASSERTE(sizeof(mrc_LastDialogs) == sizeof(rcDlg));
	const CRgnDetect* pDetect = mp_RCon->GetDetector();
	int nDlgCount = pDetect->GetDetectedDialogs(countof(rcDlg), rcDlg, rnDlgFlags);
	DWORD nDlgFlags = pDetect->GetFlags();

	if (!nDlgCount)
	{
		lbPartHidden = true;
		pp->bVisible = false;
		//if (!abTestOnly && !abOnRegister) {
		//	if (IsWindowVisible(pp->hWnd))
		//		mp_RCon->ShowOtherWindow(pp->hWnd, SW_HIDE);
		//}
	}
	else
	{
		//HRGN hRgn = nullptr, hSubRgn = nullptr, hCombine = nullptr;
		m_RgnTest.Reset();
		SMALL_RECT rcPanel; DWORD nGetRc;

		if ((nDlgFlags & FR_FULLPANEL) == FR_FULLPANEL)
			nGetRc = pDetect->GetDialog(FR_FULLPANEL, &rcPanel);
		else if (pp->PanelRect.left == 0)
			nGetRc = pDetect->GetDialog(FR_LEFTPANEL, &rcPanel);
		else
			nGetRc = pDetect->GetDialog(FR_RIGHTPANEL, &rcPanel);

		if (!nGetRc)
		{
			lbPanelVisible = false;
		}
		else
		{
			lbPanelVisible = TRUE;
			TODO("Тут хорошо бы обновить WorkRect? или он правильный всегда будет за счет вызовов RegisterPanel из плагина?");
			m_RgnTest.Init(&(pp->WorkRect));

			for(int i = 0; i < nDlgCount; i++)
			{
				// Пропустить прямоугольник соответствующий самой панели
				if (rcDlg[i].Left == rcPanel.Left &&
				        rcDlg[i].Bottom == rcPanel.Bottom &&
				        rcDlg[i].Right == rcPanel.Right)
				{
					continue;
				}

				if (!IntersectSmallRect(pp->WorkRect, rcDlg[i]))
					continue;

				// Все остальные прямоугольники вычитать из hRgn
				int nCRC = m_RgnTest.DiffSmall(rcDlg+i);

				// Проверка, может регион уже пуст?
				if (nCRC == NULLREGION)
				{
					lbPanelVisible = false; break;
				}
			}
		}

		if (abTestOnly)
		{
			// Только проверка
		}
		else
		{
			bool lbChanged = pRgn->LoadFrom(&m_RgnTest);

			if (lbPanelVisible)
			{
				lbPartHidden = (pRgn->nRgnState == COMPLEXREGION);
				pp->bVisible = true;
				_ASSERTE(pRgn->nRectCount > 0);

				if (pRgn->nRgnState == SIMPLEREGION)
				{
					//nullptr регион пусть ставит всегда (окно может быть ReUsed)
					//if (!abOnRegister) {
					mp_RCon->SetOtherWindowRgn(pp->hWnd, 0, nullptr, TRUE);
					//}
				}
				else if (lbChanged)
				{
					// А вот тут уже важно точное попадание в ячейки, поэтому через функцию, а не примерно...
					MSectionLock SCON; SCON.Lock(&csCON);
					POINT ptShift = ConsoleToClient(pp->WorkRect.left, pp->WorkRect.top);
					POINT pt[2];
					RECT  rc[MAX_DETECTED_DIALOGS];
					_ASSERTE(pRgn->nRectCount<MAX_DETECTED_DIALOGS);

					for(int i = 0; i < pRgn->nRectCount; i++)
					{
						pt[0] = ConsoleToClient(pRgn->rcRect[i].left, pRgn->rcRect[i].top);
						pt[1] = ConsoleToClient(pRgn->rcRect[i].right+1, pRgn->rcRect[i].bottom+1);
						rc[i].left  = pt[0].x - ptShift.x;
						rc[i].top   = pt[0].y - ptShift.y;
						rc[i].right = pt[1].x - ptShift.x;
						rc[i].bottom= pt[1].y - ptShift.y;
					}

					mp_RCon->SetOtherWindowRgn(pp->hWnd, pRgn->nRectCount, rc, TRUE);
				}

				//if (!abOnRegister) {
				//	if (!IsWindowVisible(pp->hWnd))
				//		mp_RCon->ShowOtherWindow(pp->hWnd, SW_SHOWNA);
				//}
			}
			else
			{
				lbPartHidden = true;
				pp->bVisible = false;
				//if (IsWindowVisible(pp->hWnd)) {
				//	// Сбросит регион и скроет(!) окно
				//	mp_RCon->SetOtherWindowRgn(pp->hWnd, -1, nullptr, FALSE);
				//}
			}
		}
	}

	if (!abTestOnly && !abOnRegister)
	{
		if (pp->bVisible && !IsWindowVisible(pp->hWnd))
		{
			mp_RCon->ShowOtherWindow(pp->hWnd, SW_SHOWNA);
		}
		else if (!pp->bVisible && IsWindowVisible(pp->hWnd))
		{
			// Сбросит регион и скроет(!) окно
			mp_RCon->SetOtherWindowRgn(pp->hWnd, -1, nullptr, FALSE);
			//mp_RCon->ShowOtherWindow(pp->hWnd, SW_HIDE);
		}
	}

	return lbPanelVisible;
}

// Отсюда - Redraw не звать, только Invalidate!
bool CVirtualConsole::UpdatePanelView(bool abLeftPanel, bool abOnRegister/*=false*/)
{
	PanelViewInit* pp = abLeftPanel ? &m_LeftPanelView : &m_RightPanelView;
	if (!IsWindow(pp->hWnd))
	{
		// Это при закрытии фара может так случиться
		return false;
	}

	// Чтобы плагин знал, что поменялась палитра (это или Fade, или реальная перенастройка цветов).
	//for (int i=0; i<16; i++)
	//	SetWindowLong(pp->hWnd, i*4, m_Colors[i]);
	if (!mn_ConEmuFadeMsg)
		mn_ConEmuFadeMsg = RegisterWindowMessage(CONEMUMSG_PNLVIEWFADE);

	const auto nNewFadeValue = static_cast<LONG>(isFade ? ConEmuPanelViewFade::Fade : ConEmuPanelViewFade::Normal);
	WARNING("Нифига не будет работать на Win7 RunAsAdmin");

	if (GetWindowLong(pp->hWnd, PANEL_VIEW_WINDOW_LONG_FADE) != nNewFadeValue)
		PostMessage(pp->hWnd, mn_ConEmuFadeMsg, 100, nNewFadeValue);

	// Подготовить размеры
	POINT pt[2];
	int nTopShift = 1 + ((pp->FarPanelSettings.ShowColumnTitles) ? 1 : 0); //-V112
	int nBottomShift = 0;
	if (mp_RCon && (pp->FarPanelSettings.ShowStatusLine))
	{
		nBottomShift = mp_RCon->GetStatusLineCount(pp->PanelRect.left) + 1;
		if (nBottomShift < 2)
			nBottomShift = 2;
	}
	pp->WorkRect = MakeRect(
	                   pp->PanelRect.left+1, pp->PanelRect.top+nTopShift,
	                   pp->PanelRect.right, pp->PanelRect.bottom-nBottomShift);
	//if (mb_DialogsChanged)
	// Перенес вниз, т.к. при регистрации окошко отображает GUI
	//UpdatePanelRgn(abLeftPanel, FALSE, abOnRegister);
	// лучше не зависеть от ConCharX - он может оказаться не инициализированным!
	pt[0] = MakePoint(pp->WorkRect.left * gpFontMgr->FontWidth(), pp->WorkRect.top * gpFontMgr->FontHeight());
	pt[1] = MakePoint(pp->WorkRect.right * gpFontMgr->FontWidth(), pp->WorkRect.bottom * gpFontMgr->FontHeight());
	//pt[0] = ConsoleToClient(pp->WorkRect.left, pp->WorkRect.top);
	//pt[1] = ConsoleToClient(pp->WorkRect.right, pp->WorkRect.bottom);
	//TODO("Потребуется коррекция для DoubleView");
	//MapWindowPoints(GetView(), ghWnd, pt, 2);
	//MoveWindow(ahView, pt[0].x,pt[0].y, pt[1].x-pt[0].x,pt[1].y-pt[0].y, FALSE);
	// Не дергаться, если менять ничего не нужно
	DWORD dwErr = 0; BOOL lbRc = TRUE;
	RECT rcCur; GetWindowRect(pp->hWnd, &rcCur);
	//MapWindowPoints(nullptr, ghWnd, (LPPOINT)&rcCur, 2);

	if (mp_RCon && (rcCur.left != pt[0].x || rcCur.top != pt[0].y
	        || rcCur.right != pt[1].x || rcCur.bottom != pt[1].y))
	{
		lbRc = mp_RCon->SetOtherWindowPos(pp->hWnd, HWND_TOP,
		                                  pt[0].x,pt[0].y, pt[1].x-pt[0].x,pt[1].y-pt[0].y,
		                                  SWP_ASYNCWINDOWPOS|SWP_DEFERERASE|SWP_NOREDRAW);

		if (!lbRc)
		{
			dwErr = GetLastError();
			DisplayLastError(L"Can't update position of PanelView window!", dwErr);
		}
	}

	// И отрисовать -- не нужно. окно еще не показано? когда будет показано - тогда и отрисуется
	//InvalidateRect(pp->hWnd, nullptr, FALSE); -- не нужно, так получается двойной WM_PAINT
	// Регионы и видимость (видимость при регистрации не меняется)
	UpdatePanelRgn(abLeftPanel, FALSE, abOnRegister);
	return TRUE;
}

void CVirtualConsole::PolishPanelViews()
{
	if (!this) return;

	//mpsz_ConChar, mpn_ConAttrEx, TextWidth, TextHeight

	if (!m_LeftPanelView.bRegister && !m_RightPanelView.bRegister)
		return;

	for(int i=0; i<=1; i++)
	{
		PanelViewInit *pp = i ? &m_RightPanelView : &m_LeftPanelView;

		if (!pp->bRegister || !pp->hWnd)
			continue; // Панель не зарегистрирована

		if (mb_DialogsChanged)
		{
			// Проверить, реально регион окна не меняется
			if (!UpdatePanelRgn(i==0, TRUE))
				continue; // Панель стала невидимой
		}

		const CEFAR_INFO_MAPPING* pFarInfo = mp_RCon->GetFarInfo();

		if (!pFarInfo || !IsWindowVisible(pp->hWnd))
			continue; // Панель полностью закрыта

		/* Так, панель видима, нужно "поправить" заголовки и разделитель перед статусом */
		RECT rc = pp->PanelRect;

		if (rc.right >= (LONG)m_Sizes.TextWidth || rc.bottom >= (LONG)m_Sizes.TextHeight)
		{
			if (rc.left >= (LONG)m_Sizes.TextWidth || rc.top >= (LONG)m_Sizes.TextHeight)
			{
				_ASSERTE(rc.right<(LONG)m_Sizes.TextWidth && rc.bottom<(LONG)m_Sizes.TextHeight);
				continue;
			}

			if (pp->PanelRect.right >= (LONG)m_Sizes.TextWidth) pp->PanelRect.right = (LONG)m_Sizes.TextWidth-1;

			if (pp->PanelRect.bottom >= (LONG)m_Sizes.TextHeight) pp->PanelRect.bottom = (LONG)m_Sizes.TextHeight-1;

			MBoxAssert(rc.right < GetTextWidth() && rc.bottom < GetTextHeight());
			rc = pp->PanelRect;
		}

		// Цвета фара
		BYTE btNamesColor = pFarInfo->nFarColors[col_PanelColumnTitle];
		BYTE btPanelColor = pFarInfo->nFarColors[col_PanelBox];
		// 1. Заголовок панели
		int x;
		wchar_t *pszLine = mpsz_ConChar;
		CharAttr *pAttrs = mpn_ConAttrEx;
		int nFore = CONFORECOLOR(btPanelColor);
		int nBack = CONBACKCOLOR(btPanelColor);

		for(x = rc.left+1; x < rc.right && pszLine[x] != L' '; x++)
		{
			if ((pszLine[x] == ucBoxSinglDownDblHorz || pszLine[x] == ucBoxDblDownDblHorz) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				pszLine[x] = ucBoxDblHorz;
		}

		for(x = rc.right-1; x > rc.left && pszLine[x] != L' '; x--)
		{
			if ((pszLine[x] == ucBoxSinglDownDblHorz || pszLine[x] == ucBoxDblDownDblHorz) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				pszLine[x] = ucBoxDblHorz;
		}

		// 2. Строка с именами колонок
		pszLine = mpsz_ConChar + m_Sizes.TextWidth;
		pAttrs = mpn_ConAttrEx + m_Sizes.TextWidth;
		int nNFore = CONFORECOLOR(btNamesColor);
		int nNBack = CONBACKCOLOR(btNamesColor);

		if ((pp->FarPanelSettings.ShowColumnTitles) && pp->tColumnTitle.nFlags) //-V112
		{
			LPCWSTR pszNameTitle = pp->tColumnTitle.sText;
			CharAttr ca; CharAttrFromConAttr(MakeUShort(pp->tColumnTitle.bConAttr), &ca);
			int nNameLen = _tcslen(pszNameTitle);
			int nX1 = rc.left + 1;

			if ((pp->tColumnTitle.nFlags & PVI_TEXT_SKIPSORTMODE)
			        && (pp->FarPanelSettings.ShowSortModeLetter & 0x800/*FPS_SHOWSORTMODELETTER*/))
				nX1 += 2;

			int nLineLen = rc.right - nX1;

			//wmemset(pszLine+nX1, L' ', nLineLen);
			if (nNameLen > nLineLen) nNameLen = nLineLen;

			int nX3 = nX1 + ((nLineLen - nNameLen) >> 1);
			_ASSERTE(nX3<=rc.right);

			//wmemcpy(pszLine+nX3, mp_RCon->ms_NameTitle, nNameLen);
			//TODO("Возможно нужно будет и атрибуты (цвет) обновить");
			for(x = nX1; x < nX3; x++)
				if ((pszLine[x] != L' ' && pAttrs[x].nForeIdx == nNFore && pAttrs[x].nBackIdx == nNBack)
				        || ((pszLine[x] == ucBoxSinglVert || pszLine[x] == ucBoxDblVert) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				  ) pszLine[x] = L' ';

			int nX4 = std::min<int>(rc.right, (nX3 + nNameLen));

			for(x = nX3; x < nX4; x++, pszNameTitle++)
				if ((pAttrs[x].nForeIdx == nNFore && pAttrs[x].nBackIdx == nNBack)
				        || ((pszLine[x] == ucBoxSinglVert || pszLine[x] == ucBoxDblVert) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				  )
				{
					pszLine[x] = *pszNameTitle;
					pAttrs[x] = ca;
				}

			for(x = nX4; x < rc.right; x++)
				if ((pszLine[x] != L' ' && pAttrs[x].nForeIdx == nNFore && pAttrs[x].nBackIdx == nNBack)
				        || ((pszLine[x] == ucBoxSinglVert || pszLine[x] == ucBoxDblVert) && pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				  ) pszLine[x] = L' ';
		}

		// 3. Разделитель
		rc = pp->WorkRect;
		pszLine = mpsz_ConChar + m_Sizes.TextWidth * (rc.bottom);
		pAttrs = mpn_ConAttrEx + m_Sizes.TextWidth * (rc.bottom);

		if ((pp->FarPanelSettings.ShowStatusLine))
		{
			for(x = rc.left+1; x < rc.right; x++)
			{
				if ((pszLine[x] == ucBoxSinglUpHorz || pszLine[x] == ucBoxDblUpSinglHorz || pszLine[x] == ucBoxDblUpSinglHorz)
					&& pAttrs[x].nForeIdx == nFore && pAttrs[x].nBackIdx == nBack)
				{
					pszLine[x] = ucBoxSinglHorz;
				}
			}
		}
	}
}

void CVirtualConsole::CharAttrFromConAttr(WORD conAttr, CharAttr* pAttr)
{
	memset(pAttr, 0, sizeof(CharAttr));
	pAttr->nForeIdx = CONFORECOLOR(conAttr);
	pAttr->nBackIdx = CONBACKCOLOR(conAttr);
	pAttr->crForeColor = pAttr->crOrigForeColor = m_Colors[pAttr->nForeIdx];
	pAttr->crBackColor = pAttr->crOrigBackColor = m_Colors[pAttr->nBackIdx];
	pAttr->ConAttr = conAttr;
}

// вызывается при получении нового Background (CECMD_SETBACKGROUND) из плагина
// и для очистки при закрытии (рестарте) консоли
SetBackgroundResult CVirtualConsole::SetBackgroundImageData(CESERVER_REQ_SETBACKGROUND* apImgData)
{
	if (!this) return esbr_Unexpected;

	//if (!isMainThread())
	//{

	// При вызове из серверной нити (только что пришло из плагина)
	if (mp_RCon->isConsoleClosing())
		return esbr_ConEmuInShutdown;

	bool bUpdate = false;
	SetBackgroundResult rc = mp_Bg->SetPluginBackgroundImageData(apImgData, bUpdate);

	// Need force update?
	if (bUpdate)
		Update(true);

	return rc;

}

#ifdef APPDISTINCTBACKGROUND
CBackgroundInfo* CVirtualConsole::GetBackgroundObject()
{
	if (!this) return nullptr;
	if (mp_BgInfo)
	{
		mp_BgInfo->AddRef();
		return mp_BgInfo;
	}
	return gpSetCls->GetBackgroundObject();
}
#endif

void CVirtualConsole::NeedBackgroundUpdate()
{
	if (this && mp_Bg)
		mp_Bg->NeedBackgroundUpdate();
}

bool CVirtualConsole::HasBackgroundImage(LONG* pnBgWidth, LONG* pnBgHeight)
{
	if (!this) return false;

	if (!mp_RCon->isFar())
		return false;

	return mp_Bg->HasPluginBackgroundImage(pnBgWidth, pnBgHeight);

	//if (!mb_BkImgExist || !(mp_BkImgData || (mp_BkEmfData && mb_BkEmfChanged))) return false;

	//// Возвращаем mn_BkImgXXX чтобы не беспокоиться об указателе mp_BkImgData

	//if (pnBgWidth)
	//	*pnBgWidth = mn_BkImgWidth;

	//if (pnBgHeight)
	//	*pnBgHeight = mn_BkImgHeight;

	//return (mn_BkImgWidth != 0 && mn_BkImgHeight != 0);
	////MSectionLock SBK; SBK.Lock(&csBkImgData);
	////if (mp_BkImgData)
	////{
	////	BITMAPINFOHEADER* pBmp = (BITMAPINFOHEADER*)(mp_BkImgData+1);
	////	if (pnBgWidth)
	////		*pnBgWidth = pBmp->biWidth;
	////	if (pnBgHeight)
	////		*pnBgHeight = pBmp->biHeight;
	////}
	////return mp_BkImgData;
}

void CVirtualConsole::OnTitleChanged()
{
	if (!isMainThread())
	{
		PostMessage(GetView(), mn_MsgOnTitleChanged, 0, 0);
		return;
	}
	if (mp_Ghost)
		mp_Ghost->CheckTitle();
	if (isActive(false))
		mp_ConEmu->UpdateTitle();
}

void CVirtualConsole::SavePaneSnapshot()
{
	if (!isMainThread())
	{
		PostMessage(GetView(), mn_MsgSavePaneSnapshot, 0, 0);
		return;
	}

	if (mp_Ghost /*&& !gbNoDblBuffer*/)
	{
		mp_Ghost->UpdateTabSnapshot(); //CreateTabSnapshoot((HDC)m_DC, Width, Height, TRUE);
	}
}

// Вызывается при кликах по CheckBox-у "Show on taskbar"
void CVirtualConsole::OnTaskbarSettingsChanged()
{
	if (gpSet->isTabsOnTaskBar())
	{
		if (!mp_Ghost)
			InitGhost();
	}
	else
	{
		if (mp_Ghost)
		{
			delete mp_Ghost;
			mp_Ghost = nullptr;
		}
	}
}

void CVirtualConsole::OnTaskbarFocus()
{
	if (mp_Ghost)
		mp_Ghost->ActivateTaskbar();
}

HDC CVirtualConsole::GetIntDC()
{
	return (HDC)m_DC;
}
