#pragma once
#ifndef __PLUGIN_HPP__
#define __PLUGIN_HPP__

/*
  plugin.hpp

  Plugin API for Far Manager 3.0 build 4040
*/

/*
Copyright © 1996 Eugene Roshal
Copyright © 2000 Far Group
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

THIS SOFTWARE IS PROVIDED BY THE AUTHOR `AS IS' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

EXCEPTION:
Far Manager plugins that use this header file can be distributed under any
other possible license with no implications from the above license on them.
*/

#define FARMANAGERVERSION_MAJOR 3
#define FARMANAGERVERSION_MINOR 0
#define FARMANAGERVERSION_REVISION 0
#define FARMANAGERVERSION_BUILD 4040
#define FARMANAGERVERSION_STAGE VS_RELEASE //VS_BIS

#ifndef RC_INVOKED

#if defined(__GNUC__) || (_MSC_VER >= 1600)
#include <stdint.h>
#else
#include "stdint_.h"
#endif
#include <windows.h>

#undef DefDlgProc

#define FARMACRO_KEY_EVENT  (KEY_EVENT|0x8000)


#define CP_UNICODE    ((uintptr_t)1200)
#define CP_REVERSEBOM ((uintptr_t)1201)
#define CP_DEFAULT    ((uintptr_t)-1)
#define CP_REDETECT   ((uintptr_t)-2)

typedef unsigned __int64 FARCOLORFLAGS;
static const FARCOLORFLAGS
	FCF_FG_4BIT       = 0x0000000000000001ULL,
	FCF_BG_4BIT       = 0x0000000000000002ULL,
	FCF_4BITMASK      = 0x0000000000000003ULL, // FCF_FG_4BIT|FCF_BG_4BIT

	FCF_RAWATTR_MASK  = 0x000000000000FF00ULL, // stored console attributes

	FCF_EXTENDEDFLAGS = 0xFFFFFFFFFFFFFFFCULL, // ~FCF_4BITMASK

	FCF_FG_BOLD       = 0x1000000000000000ULL,
	FCF_FG_ITALIC     = 0x2000000000000000ULL,
	FCF_FG_UNDERLINE  = 0x4000000000000000ULL,
	FCF_STYLEMASK     = 0x7000000000000000ULL, // FCF_FG_BOLD|FCF_FG_ITALIC|FCF_FG_UNDERLINE

	FCF_NONE          = 0;

struct rgba { unsigned char r, g, b, a; };

struct FarColor
{
	FARCOLORFLAGS Flags;
	union
	{
		COLORREF ForegroundColor;
		struct rgba ForegroundRGBA;
	}
#ifndef __cplusplus
	Foreground
#endif
	;
	union
	{
		COLORREF BackgroundColor;
		struct rgba BackgroundRGBA;
	}
#ifndef __cplusplus
	Background
#endif
	;
	void* Reserved;

#ifdef __cplusplus
	bool operator ==(const FarColor& rhs) const
	{
		return Flags == rhs.Flags
			&& ForegroundColor == rhs.ForegroundColor
			&& BackgroundColor == rhs.BackgroundColor
			&& Reserved == rhs.Reserved;
	}

	bool operator !=(const FarColor& rhs) const
	{
		return !(*this == rhs);
	}
#endif
};

#define INDEXMASK 0x0000000f
#define COLORMASK 0x00ffffff
#define ALPHAMASK 0xff000000

#define INDEXVALUE(x) ((x)&INDEXMASK)
#define COLORVALUE(x) ((x)&COLORMASK)
#define ALPHAVALUE(x) ((x)&ALPHAMASK)

#define IS_OPAQUE(x) (ALPHAVALUE(x)==ALPHAMASK)
#define IS_TRANSPARENT(x) (!ALPHAVALUE(x))
#define MAKE_OPAQUE(x) (x|=ALPHAMASK)
#define MAKE_TRANSPARENT(x) (x&=COLORMASK)

typedef unsigned __int64 COLORDIALOGFLAGS;
static const COLORDIALOGFLAGS
    CDF_NONE = 0;

typedef BOOL (WINAPI *FARAPICOLORDIALOG)(
    const GUID* PluginId,
    COLORDIALOGFLAGS Flags,
    struct FarColor *Color
);

typedef unsigned __int64 FARMESSAGEFLAGS;
static const FARMESSAGEFLAGS
	FMSG_WARNING             = 0x0000000000000001ULL,
	FMSG_ERRORTYPE           = 0x0000000000000002ULL,
	FMSG_KEEPBACKGROUND      = 0x0000000000000004ULL,
	FMSG_LEFTALIGN1900       = 0x0000000000000008ULL,
	FMSG_ALLINONE1900        = 0x0000000000000010ULL,
	FMSG_MB_OK               = 0x0000000000010000ULL,
	FMSG_MB_OKCANCEL         = 0x0000000000020000ULL,
	FMSG_MB_ABORTRETRYIGNORE = 0x0000000000030000ULL,
	FMSG_MB_YESNO            = 0x0000000000040000ULL,
	FMSG_MB_YESNOCANCEL      = 0x0000000000050000ULL,
	FMSG_MB_RETRYCANCEL      = 0x0000000000060000ULL,
	FMSG_NONE                = 0;

typedef intptr_t (WINAPI *FARAPIMESSAGE)(
    const GUID* PluginId,
    const GUID* Id,
    FARMESSAGEFLAGS Flags,
    const wchar_t *HelpTopic,
    const wchar_t * const *Items,
    size_t ItemsNumber,
    intptr_t ButtonsNumber
);

enum FARDIALOGITEMTYPES
{
	DI_TEXT                         =  0,
	DI_VTEXT                        =  1,
	DI_SINGLEBOX                    =  2,
	DI_DOUBLEBOX                    =  3,
	DI_EDIT                         =  4,
	DI_PSWEDIT                      =  5,
	DI_FIXEDIT                      =  6,
	DI_BUTTON                       =  7,
	DI_CHECKBOX                     =  8,
	DI_RADIOBUTTON                  =  9,
	DI_COMBOBOX                     = 10,
	DI_LISTBOX                      = 11,

	DI_USERCONTROL                  =255,
};

/*
   Check diagol element type has inputstring?
   (DI_EDIT, DI_FIXEDIT, DI_PSWEDIT, etc)
*/
static __inline BOOL IsEdit(enum FARDIALOGITEMTYPES Type)
{
	switch (Type)
	{
		case DI_EDIT:
		case DI_FIXEDIT:
		case DI_PSWEDIT:
		case DI_COMBOBOX:
			return TRUE;
		default:
			return FALSE;
	}
}

typedef unsigned __int64 FARDIALOGITEMFLAGS;
static const FARDIALOGITEMFLAGS
	DIF_BOXCOLOR              = 0x0000000000000200ULL,
	DIF_GROUP                 = 0x0000000000000400ULL,
	DIF_LEFTTEXT              = 0x0000000000000800ULL,
	DIF_MOVESELECT            = 0x0000000000001000ULL,
	DIF_SHOWAMPERSAND         = 0x0000000000002000ULL,
	DIF_CENTERGROUP           = 0x0000000000004000ULL,
	DIF_NOBRACKETS            = 0x0000000000008000ULL,
	DIF_MANUALADDHISTORY      = 0x0000000000008000ULL,
	DIF_SEPARATOR             = 0x0000000000010000ULL,
	DIF_SEPARATOR2            = 0x0000000000020000ULL,
	DIF_EDITOR                = 0x0000000000020000ULL,
	DIF_LISTNOAMPERSAND       = 0x0000000000020000ULL,
	DIF_LISTNOBOX             = 0x0000000000040000ULL,
	DIF_HISTORY               = 0x0000000000040000ULL,
	DIF_BTNNOCLOSE            = 0x0000000000040000ULL,
	DIF_CENTERTEXT            = 0x0000000000040000ULL,
	DIF_SEPARATORUSER         = 0x0000000000080000ULL,
	DIF_SETSHIELD             = 0x0000000000080000ULL,
	DIF_EDITEXPAND            = 0x0000000000080000ULL,
	DIF_DROPDOWNLIST          = 0x0000000000100000ULL,
	DIF_USELASTHISTORY        = 0x0000000000200000ULL,
	DIF_MASKEDIT              = 0x0000000000400000ULL,
	DIF_LISTTRACKMOUSE        = 0x0000000000400000ULL,
	DIF_LISTTRACKMOUSEINFOCUS = 0x0000000000800000ULL,
	DIF_SELECTONENTRY         = 0x0000000000800000ULL,
	DIF_3STATE                = 0x0000000000800000ULL,
	DIF_EDITPATH              = 0x0000000001000000ULL,
	DIF_LISTWRAPMODE          = 0x0000000001000000ULL,
	DIF_NOAUTOCOMPLETE        = 0x0000000002000000ULL,
	DIF_LISTAUTOHIGHLIGHT     = 0x0000000002000000ULL,
	DIF_LISTNOCLOSE           = 0x0000000004000000ULL,
	DIF_EDITPATHEXEC          = 0x0000000004000000ULL,
	DIF_HIDDEN                = 0x0000000010000000ULL,
	DIF_READONLY              = 0x0000000020000000ULL,
	DIF_NOFOCUS               = 0x0000000040000000ULL,
	DIF_DISABLE               = 0x0000000080000000ULL,
	DIF_DEFAULTBUTTON         = 0x0000000100000000ULL,
	DIF_FOCUS                 = 0x0000000200000000ULL,
	DIF_RIGHTTEXT             = 0x0000000400000000ULL,
	DIF_WORDWRAP              = 0x0000000800000000ULL,
	DIF_NONE                  = 0;

enum FARMESSAGE
{
	DM_FIRST                        = 0,
	DM_CLOSE                        = 1,
	DM_ENABLE                       = 2,
	DM_ENABLEREDRAW                 = 3,
	DM_GETDLGDATA                   = 4,
	DM_GETDLGITEM                   = 5,
	DM_GETDLGRECT                   = 6,
	DM_GETTEXT                      = 7,
	DM_KEY                          = 9,
	DM_MOVEDIALOG                   = 10,
	DM_SETDLGDATA                   = 11,
	DM_SETDLGITEM                   = 12,
	DM_SETFOCUS                     = 13,
	DM_REDRAW                       = 14,
	DM_SETTEXT                      = 15,
	DM_SETMAXTEXTLENGTH             = 16,
	DM_SHOWDIALOG                   = 17,
	DM_GETFOCUS                     = 18,
	DM_GETCURSORPOS                 = 19,
	DM_SETCURSORPOS                 = 20,
	DM_SETTEXTPTR                   = 22,
	DM_SHOWITEM                     = 23,
	DM_ADDHISTORY                   = 24,

	DM_GETCHECK                     = 25,
	DM_SETCHECK                     = 26,
	DM_SET3STATE                    = 27,

	DM_LISTSORT                     = 28,
	DM_LISTGETITEM                  = 29,
	DM_LISTGETCURPOS                = 30,
	DM_LISTSETCURPOS                = 31,
	DM_LISTDELETE                   = 32,
	DM_LISTADD                      = 33,
	DM_LISTADDSTR                   = 34,
	DM_LISTUPDATE                   = 35,
	DM_LISTINSERT                   = 36,
	DM_LISTFINDSTRING               = 37,
	DM_LISTINFO                     = 38,
	DM_LISTGETDATA                  = 39,
	DM_LISTSETDATA                  = 40,
	DM_LISTSETTITLES                = 41,
	DM_LISTGETTITLES                = 42,

	DM_RESIZEDIALOG                 = 43,
	DM_SETITEMPOSITION              = 44,

	DM_GETDROPDOWNOPENED            = 45,
	DM_SETDROPDOWNOPENED            = 46,

	DM_SETHISTORY                   = 47,

	DM_GETITEMPOSITION              = 48,
	DM_SETMOUSEEVENTNOTIFY          = 49,

	DM_EDITUNCHANGEDFLAG            = 50,

	DM_GETITEMDATA                  = 51,
	DM_SETITEMDATA                  = 52,

	DM_LISTSET                      = 53,

	DM_GETCURSORSIZE                = 54,
	DM_SETCURSORSIZE                = 55,

	DM_LISTGETDATASIZE              = 56,

	DM_GETSELECTION                 = 57,
	DM_SETSELECTION                 = 58,

	DM_GETEDITPOSITION              = 59,
	DM_SETEDITPOSITION              = 60,

	DM_SETCOMBOBOXEVENT             = 61,
	DM_GETCOMBOBOXEVENT             = 62,

	DM_GETCONSTTEXTPTR              = 63,
	DM_GETDLGITEMSHORT              = 64,
	DM_SETDLGITEMSHORT              = 65,

	DM_GETDIALOGINFO                = 66,

	DM_GETDIALOGTITLE               = 67,

	DN_FIRST                        = 4096,
	DN_BTNCLICK                     = 4097,
	DN_CTLCOLORDIALOG               = 4098,
	DN_CTLCOLORDLGITEM              = 4099,
	DN_CTLCOLORDLGLIST              = 4100,
	DN_DRAWDIALOG                   = 4101,
	DN_DRAWDLGITEM                  = 4102,
	DN_EDITCHANGE                   = 4103,
	DN_ENTERIDLE                    = 4104,
	DN_GOTFOCUS                     = 4105,
	DN_HELP                         = 4106,
	DN_HOTKEY                       = 4107,
	DN_INITDIALOG                   = 4108,
	DN_KILLFOCUS                    = 4109,
	DN_LISTCHANGE                   = 4110,
	DN_DRAGGED                      = 4111,
	DN_RESIZECONSOLE                = 4112,
	DN_DRAWDIALOGDONE               = 4113,
	DN_LISTHOTKEY                   = 4114,
	DN_INPUT                        = 4115,
	DN_CONTROLINPUT                 = 4116,
	DN_CLOSE                        = 4117,
	DN_GETVALUE                     = 4118,
	DN_DROPDOWNOPENED               = 4119,
	DN_DRAWDLGITEMDONE              = 4120,

	DM_USER                         = 0x4000,

};

enum FARCHECKEDSTATE
{
	BSTATE_UNCHECKED = 0,
	BSTATE_CHECKED   = 1,
	BSTATE_3STATE    = 2,
	BSTATE_TOGGLE    = 3,
};

enum FARCOMBOBOXEVENTTYPE
{
	CBET_KEY         = 0x00000001,
	CBET_MOUSE       = 0x00000002,
};

typedef unsigned __int64 LISTITEMFLAGS;
static const LISTITEMFLAGS
	LIF_SELECTED           = 0x0000000000010000ULL,
	LIF_CHECKED            = 0x0000000000020000ULL,
	LIF_SEPARATOR          = 0x0000000000040000ULL,
	LIF_DISABLE            = 0x0000000000080000ULL,
	LIF_GRAYED             = 0x0000000000100000ULL,
	LIF_HIDDEN             = 0x0000000000200000ULL,
	LIF_DELETEUSERDATA     = 0x0000000080000000ULL,
	LIF_NONE               = 0;



struct FarListItem
{
	LISTITEMFLAGS Flags;
	const wchar_t *Text;
	intptr_t Reserved[2];
};

struct FarListUpdate
{
	size_t StructSize;
	intptr_t Index;
	struct FarListItem Item;
};

struct FarListInsert
{
	size_t StructSize;
	intptr_t Index;
	struct FarListItem Item;
};

struct FarListGetItem
{
	size_t StructSize;
	intptr_t ItemIndex;
	struct FarListItem Item;
};

struct FarListPos
{
	size_t StructSize;
	intptr_t SelectPos;
	intptr_t TopPos;
};

typedef unsigned __int64 FARLISTFINDFLAGS;
static const FARLISTFINDFLAGS
	LIFIND_EXACTMATCH = 0x0000000000000001ULL,
	LIFIND_NONE       = 0;


struct FarListFind
{
	size_t StructSize;
	intptr_t StartIndex;
	const wchar_t *Pattern;
	FARLISTFINDFLAGS Flags;
};

struct FarListDelete
{
	size_t StructSize;
	intptr_t StartIndex;
	intptr_t Count;
};

typedef unsigned __int64 FARLISTINFOFLAGS;
static const FARLISTINFOFLAGS
	LINFO_SHOWNOBOX             = 0x0000000000000400ULL,
	LINFO_AUTOHIGHLIGHT         = 0x0000000000000800ULL,
	LINFO_REVERSEHIGHLIGHT      = 0x0000000000001000ULL,
	LINFO_WRAPMODE              = 0x0000000000008000ULL,
	LINFO_SHOWAMPERSAND         = 0x0000000000010000ULL,
	LINFO_NONE                  = 0;

struct FarListInfo
{
	size_t StructSize;
	FARLISTINFOFLAGS Flags;
	size_t ItemsNumber;
	intptr_t SelectPos;
	intptr_t TopPos;
	intptr_t MaxHeight;
	intptr_t MaxLength;
};

struct FarListItemData
{
	size_t StructSize;
	intptr_t Index;
	size_t DataSize;
	void *Data;
};

struct FarList
{
	size_t StructSize;
	size_t ItemsNumber;
	struct FarListItem *Items;
};

struct FarListTitles
{
	size_t StructSize;
	size_t TitleSize;
	const wchar_t *Title;
	size_t BottomSize;
	const wchar_t *Bottom;
};

struct FarDialogItemColors
{
	size_t StructSize;
	unsigned __int64 Flags;
	size_t ColorsCount;
	struct FarColor* Colors;
};

struct FAR_CHAR_INFO
{
	WCHAR Char;
	struct FarColor Attributes;

#ifdef __cplusplus
	bool operator ==(const FAR_CHAR_INFO& rhs) const
	{
		return Char == rhs.Char && Attributes == rhs.Attributes;
	}

	bool operator !=(const FAR_CHAR_INFO& rhs) const
	{
		return !(*this == rhs);
	}
#endif
};

struct FarDialogItem
{
	enum FARDIALOGITEMTYPES Type;
	intptr_t X1,Y1,X2,Y2;
	union
	{
		intptr_t Selected;
		struct FarList *ListItems;
		struct FAR_CHAR_INFO *VBuf;
		intptr_t Reserved0;
	}
#ifndef __cplusplus
	Param
#endif
	;
	const wchar_t *History;
	const wchar_t *Mask;
	FARDIALOGITEMFLAGS Flags;
	const wchar_t *Data;
	size_t MaxLength; // terminate 0 not included (if == 0 string size is unlimited)
	intptr_t UserData;
	intptr_t Reserved[2];
};

struct FarDialogItemData
{
	size_t StructSize;
	size_t  PtrLength;
	wchar_t *PtrData;
};

struct FarDialogEvent
{
	size_t StructSize;
	HANDLE hDlg;
	intptr_t Msg;
	intptr_t Param1;
	void* Param2;
	intptr_t Result;
};

struct OpenDlgPluginData
{
	size_t StructSize;
	HANDLE hDlg;
};

struct DialogInfo
{
	size_t StructSize;
	GUID Id;
	GUID Owner;
};

struct FarGetDialogItem
{
	size_t StructSize;
	size_t Size;
	struct FarDialogItem* Item;
};

typedef unsigned __int64 FARDIALOGFLAGS;
static const FARDIALOGFLAGS
	FDLG_WARNING             = 0x0000000000000001ULL,
	FDLG_SMALLDIALOG         = 0x0000000000000002ULL,
	FDLG_NODRAWSHADOW        = 0x0000000000000004ULL,
	FDLG_NODRAWPANEL         = 0x0000000000000008ULL,
	FDLG_KEEPCONSOLETITLE    = 0x0000000000000010ULL,
	FDLG_NONE                = 0;

typedef intptr_t(WINAPI *FARWINDOWPROC)(
    HANDLE   hDlg,
    intptr_t Msg,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t(WINAPI *FARAPISENDDLGMESSAGE)(
    HANDLE   hDlg,
    intptr_t Msg,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t(WINAPI *FARAPIDEFDLGPROC)(
    HANDLE   hDlg,
    intptr_t Msg,
    intptr_t Param1,
    void* Param2
);

typedef HANDLE(WINAPI *FARAPIDIALOGINIT)(
    const GUID*           PluginId,
    const GUID*           Id,
    intptr_t              X1,
    intptr_t              Y1,
    intptr_t              X2,
    intptr_t              Y2,
    const wchar_t        *HelpTopic,
    const struct FarDialogItem *Item,
    size_t                ItemsNumber,
    intptr_t              Reserved,
    FARDIALOGFLAGS        Flags,
    FARWINDOWPROC         DlgProc,
    void*                 Param
);

typedef intptr_t (WINAPI *FARAPIDIALOGRUN)(
    HANDLE hDlg
);

typedef void (WINAPI *FARAPIDIALOGFREE)(
    HANDLE hDlg
);

struct FarKey
{
    WORD VirtualKeyCode;
    DWORD ControlKeyState;
};

typedef unsigned __int64 MENUITEMFLAGS;
static const MENUITEMFLAGS
	MIF_SELECTED   = 0x000000000010000ULL,
	MIF_CHECKED    = 0x000000000020000ULL,
	MIF_SEPARATOR  = 0x000000000040000ULL,
	MIF_DISABLE    = 0x000000000080000ULL,
	MIF_GRAYED     = 0x000000000100000ULL,
	MIF_HIDDEN     = 0x000000000200000ULL,
	MIF_NONE       = 0;

struct FarMenuItem
{
	MENUITEMFLAGS Flags;
	const wchar_t *Text;
	struct FarKey AccelKey;
	intptr_t UserData;
	intptr_t Reserved[2];
};

typedef unsigned __int64 FARMENUFLAGS;
static const FARMENUFLAGS
	FMENU_SHOWAMPERSAND        = 0x0000000000000001ULL,
	FMENU_WRAPMODE             = 0x0000000000000002ULL,
	FMENU_AUTOHIGHLIGHT        = 0x0000000000000004ULL,
	FMENU_REVERSEAUTOHIGHLIGHT = 0x0000000000000008ULL,
	FMENU_CHANGECONSOLETITLE   = 0x0000000000000010ULL,
	FMENU_NONE                 = 0;

typedef intptr_t (WINAPI *FARAPIMENU)(
	const GUID*         PluginId,
    const GUID*         Id,
    intptr_t            X,
    intptr_t            Y,
    intptr_t            MaxHeight,
    FARMENUFLAGS        Flags,
    const wchar_t      *Title,
    const wchar_t      *Bottom,
    const wchar_t      *HelpTopic,
    const struct FarKey *BreakKeys,
    intptr_t           *BreakCode,
    const struct FarMenuItem *Item,
    size_t              ItemsNumber
);


typedef unsigned __int64 PLUGINPANELITEMFLAGS;
static const PLUGINPANELITEMFLAGS
	PPIF_SELECTED               = 0x0000000040000000ULL,
	PPIF_PROCESSDESCR           = 0x0000000080000000ULL,
	PPIF_HASNOTEXTENSION        = 0x0000000100000000ULL,
	PPIF_NONE                   = 0;

struct FarPanelItemFreeInfo
{
	size_t StructSize;
	HANDLE hPlugin;
};

typedef void (WINAPI *FARPANELITEMFREECALLBACK)(void* UserData, const struct FarPanelItemFreeInfo* Info);

struct UserDataItem
{
	void* Data;
	FARPANELITEMFREECALLBACK FreeData;
};


struct PluginPanelItem
{
	FILETIME CreationTime;
	FILETIME LastAccessTime;
	FILETIME LastWriteTime;
	FILETIME ChangeTime;
	unsigned __int64 FileSize;
	unsigned __int64 AllocationSize;
	const wchar_t *FileName;
	const wchar_t *AlternateFileName;
	const wchar_t *Description;
	const wchar_t *Owner;
	const wchar_t * const *CustomColumnData;
	size_t CustomColumnNumber;
	PLUGINPANELITEMFLAGS Flags;
	struct UserDataItem UserData;
	uintptr_t FileAttributes;
	uintptr_t NumberOfLinks;
	uintptr_t CRC32;
	intptr_t Reserved[2];
};

struct FarGetPluginPanelItem
{
	size_t StructSize;
	size_t Size;
	struct PluginPanelItem* Item;
};

struct SortingPanelItem
{
	FILETIME CreationTime;
	FILETIME LastAccessTime;
	FILETIME LastWriteTime;
	FILETIME ChangeTime;
	unsigned __int64 FileSize;
	unsigned __int64 AllocationSize;
	const wchar_t *FileName;
	const wchar_t *AlternateFileName;
	const wchar_t *Description;
	const wchar_t *Owner;
	const wchar_t * const *CustomColumnData;
	size_t CustomColumnNumber;
	PLUGINPANELITEMFLAGS Flags;
	struct UserDataItem UserData;
	uintptr_t FileAttributes;
	uintptr_t NumberOfLinks;
	uintptr_t CRC32;
	intptr_t Position;
	intptr_t SortGroup;
	uintptr_t NumberOfStreams;
	unsigned __int64 StreamsSize;
};

#if 1
//Maximus: FCTL_GETPANELITEMINFO
struct FarGetPluginPanelItemInfo
{
	size_t StructSize;
	struct FarColor Color; // Current element color
    int    PosX; // 1-based, relative to Far workspace, 0 means 'not visible now'
    int    PosY; // 1-based, relative to Far workspace, 0 means 'not visible now'
};
#endif

typedef unsigned __int64 PANELINFOFLAGS;
static const PANELINFOFLAGS
	PFLAGS_SHOWHIDDEN         = 0x0000000000000001ULL,
	PFLAGS_HIGHLIGHT          = 0x0000000000000002ULL,
	PFLAGS_REVERSESORTORDER   = 0x0000000000000004ULL,
	PFLAGS_USESORTGROUPS      = 0x0000000000000008ULL,
	PFLAGS_SELECTEDFIRST      = 0x0000000000000010ULL,
	PFLAGS_REALNAMES          = 0x0000000000000020ULL,
	PFLAGS_NUMERICSORT        = 0x0000000000000040ULL,
	PFLAGS_PANELLEFT          = 0x0000000000000080ULL,
	PFLAGS_DIRECTORIESFIRST   = 0x0000000000000100ULL,
	PFLAGS_USECRC32           = 0x0000000000000200ULL,
	PFLAGS_CASESENSITIVESORT  = 0x0000000000000400ULL,
	PFLAGS_PLUGIN             = 0x0000000000000800ULL,
	PFLAGS_VISIBLE            = 0x0000000000001000ULL,
	PFLAGS_FOCUS              = 0x0000000000002000ULL,
	PFLAGS_ALTERNATIVENAMES   = 0x0000000000004000ULL,
	PFLAGS_SHORTCUT           = 0x0000000000008000ULL,
	PFLAGS_NONE               = 0;

enum PANELINFOTYPE
{
	PTYPE_FILEPANEL                 = 0,
	PTYPE_TREEPANEL                 = 1,
	PTYPE_QVIEWPANEL                = 2,
	PTYPE_INFOPANEL                 = 3,
};

enum OPENPANELINFO_SORTMODES
{
	SM_DEFAULT                   =  0,
	SM_UNSORTED                  =  1,
	SM_NAME                      =  2,
	SM_EXT                       =  3,
	SM_MTIME                     =  4,
	SM_CTIME                     =  5,
	SM_ATIME                     =  6,
	SM_SIZE                      =  7,
	SM_DESCR                     =  8,
	SM_OWNER                     =  9,
	SM_COMPRESSEDSIZE            = 10,
	SM_NUMLINKS                  = 11,
	SM_NUMSTREAMS                = 12,
	SM_STREAMSSIZE               = 13,
	SM_FULLNAME                  = 14,
	SM_CHTIME                    = 15,
};

struct PanelInfo
{
	size_t StructSize;
	HANDLE PluginHandle;
	GUID OwnerGuid;
	PANELINFOFLAGS Flags;
	size_t ItemsNumber;
	size_t SelectedItemsNumber;
	RECT PanelRect;
	size_t CurrentItem;
	size_t TopPanelItem;
	intptr_t ViewMode;
	enum PANELINFOTYPE PanelType;
	enum OPENPANELINFO_SORTMODES SortMode;
};


struct PanelRedrawInfo
{
	size_t StructSize;
	size_t CurrentItem;
	size_t TopPanelItem;
};

struct CmdLineSelect
{
	size_t StructSize;
	intptr_t SelStart;
	intptr_t SelEnd;
};

struct FarPanelDirectory
{
	size_t StructSize;
	const wchar_t* Name;
	const wchar_t* Param;
	GUID PluginId;
	const wchar_t* File;
};

#define PANEL_NONE    ((HANDLE)(-1))
#define PANEL_ACTIVE  ((HANDLE)(-1))
#define PANEL_PASSIVE ((HANDLE)(-2))
#define PANEL_STOP ((HANDLE)(-1))

enum FILE_CONTROL_COMMANDS
{
	FCTL_CLOSEPANEL                 = 0,
	FCTL_GETPANELINFO               = 1,
	FCTL_UPDATEPANEL                = 2,
	FCTL_REDRAWPANEL                = 3,
	FCTL_GETCMDLINE                 = 4,
	FCTL_SETCMDLINE                 = 5,
	FCTL_SETSELECTION               = 6,
	FCTL_SETVIEWMODE                = 7,
	FCTL_INSERTCMDLINE              = 8,
	FCTL_SETUSERSCREEN              = 9,
	FCTL_SETPANELDIRECTORY          = 10,
	FCTL_SETCMDLINEPOS              = 11,
	FCTL_GETCMDLINEPOS              = 12,
	FCTL_SETSORTMODE                = 13,
	FCTL_SETSORTORDER               = 14,
	FCTL_SETCMDLINESELECTION        = 15,
	FCTL_GETCMDLINESELECTION        = 16,
	FCTL_CHECKPANELSEXIST           = 17,
	FCTL_SETNUMERICSORT             = 18,
	FCTL_GETUSERSCREEN              = 19,
	FCTL_ISACTIVEPANEL              = 20,
	FCTL_GETPANELITEM               = 21,
	FCTL_GETSELECTEDPANELITEM       = 22,
	FCTL_GETCURRENTPANELITEM        = 23,
	FCTL_GETPANELDIRECTORY          = 24,
	FCTL_GETCOLUMNTYPES             = 25,
	FCTL_GETCOLUMNWIDTHS            = 26,
	FCTL_BEGINSELECTION             = 27,
	FCTL_ENDSELECTION               = 28,
	FCTL_CLEARSELECTION             = 29,
	FCTL_SETDIRECTORIESFIRST        = 30,
	FCTL_GETPANELFORMAT             = 31,
	FCTL_GETPANELHOSTFILE           = 32,
	FCTL_SETCASESENSITIVESORT       = 33,
	FCTL_GETPANELPREFIX             = 34,
	FCTL_SETACTIVEPANEL             = 35,

	#if 1
	//Maximus5
	FCTL_GETPANELITEMINFO           = 1001,
	#endif
};

typedef void (WINAPI *FARAPITEXT)(
    intptr_t X,
    intptr_t Y,
    const struct FarColor* Color,
    const wchar_t *Str
);

typedef HANDLE(WINAPI *FARAPISAVESCREEN)(intptr_t X1, intptr_t Y1, intptr_t X2, intptr_t Y2);

typedef void (WINAPI *FARAPIRESTORESCREEN)(HANDLE hScreen);


typedef intptr_t (WINAPI *FARAPIGETDIRLIST)(
    const wchar_t *Dir,
    struct PluginPanelItem **pPanelItem,
    size_t *pItemsNumber
);

typedef intptr_t (WINAPI *FARAPIGETPLUGINDIRLIST)(
    const GUID* PluginId,
    HANDLE hPanel,
    const wchar_t *Dir,
    struct PluginPanelItem **pPanelItem,
    size_t *pItemsNumber
);

typedef void (WINAPI *FARAPIFREEDIRLIST)(struct PluginPanelItem *PanelItem, size_t nItemsNumber);
typedef void (WINAPI *FARAPIFREEPLUGINDIRLIST)(HANDLE hPanel, struct PluginPanelItem *PanelItem, size_t nItemsNumber);

typedef unsigned __int64 VIEWER_FLAGS;
static const VIEWER_FLAGS
	VF_NONMODAL              = 0x0000000000000001ULL,
	VF_DELETEONCLOSE         = 0x0000000000000002ULL,
	VF_ENABLE_F6             = 0x0000000000000004ULL,
	VF_DISABLEHISTORY        = 0x0000000000000008ULL,
	VF_IMMEDIATERETURN       = 0x0000000000000100ULL,
	VF_DELETEONLYFILEONCLOSE = 0x0000000000000200ULL,
	VF_NONE                  = 0;

typedef intptr_t (WINAPI *FARAPIVIEWER)(
    const wchar_t *FileName,
    const wchar_t *Title,
    intptr_t X1,
    intptr_t Y1,
    intptr_t X2,
    intptr_t Y2,
    VIEWER_FLAGS Flags,
    uintptr_t CodePage
);

typedef unsigned __int64 EDITOR_FLAGS;
static const EDITOR_FLAGS
	EF_NONMODAL              = 0x0000000000000001ULL,
	EF_CREATENEW             = 0x0000000000000002ULL,
	EF_ENABLE_F6             = 0x0000000000000004ULL,
	EF_DISABLEHISTORY        = 0x0000000000000008ULL,
	EF_DELETEONCLOSE         = 0x0000000000000010ULL,
	EF_IMMEDIATERETURN       = 0x0000000000000100ULL,
	EF_DELETEONLYFILEONCLOSE = 0x0000000000000200ULL,
	EF_LOCKED                = 0x0000000000000400ULL,
	EF_DISABLESAVEPOS        = 0x0000000000000800ULL,
	EF_OPENMODE_MASK         = 0x00000000F0000000ULL,
	EF_OPENMODE_QUERY        = 0x0000000000000000ULL,
	EF_OPENMODE_NEWIFOPEN    = 0x0000000010000000ULL,
	EF_OPENMODE_USEEXISTING  = 0x0000000020000000ULL,
	EF_OPENMODE_BREAKIFOPEN  = 0x0000000030000000ULL,
	EF_OPENMODE_RELOADIFOPEN = 0x0000000040000000ULL,
	EN_NONE                  = 0;

enum EDITOR_EXITCODE
{
	EEC_OPEN_ERROR          = 0,
	EEC_MODIFIED            = 1,
	EEC_NOT_MODIFIED        = 2,
	EEC_LOADING_INTERRUPTED = 3,
};

typedef intptr_t (WINAPI *FARAPIEDITOR)(
    const wchar_t *FileName,
    const wchar_t *Title,
    intptr_t X1,
    intptr_t Y1,
    intptr_t X2,
    intptr_t Y2,
    EDITOR_FLAGS Flags,
    intptr_t StartLine,
    intptr_t StartChar,
    uintptr_t CodePage
);

typedef const wchar_t*(WINAPI *FARAPIGETMSG)(
    const GUID* PluginId,
    intptr_t MsgId
);

typedef unsigned __int64 FARHELPFLAGS;
static const FARHELPFLAGS
	FHELP_NOSHOWERROR = 0x0000000080000000ULL,
	FHELP_SELFHELP    = 0x0000000000000000ULL,
	FHELP_FARHELP     = 0x0000000000000001ULL,
	FHELP_CUSTOMFILE  = 0x0000000000000002ULL,
	FHELP_CUSTOMPATH  = 0x0000000000000004ULL,
	FHELP_GUID        = 0x0000000000000008ULL,
	FHELP_USECONTENTS = 0x0000000040000000ULL,
	FHELP_NONE        = 0;

typedef BOOL (WINAPI *FARAPISHOWHELP)(
    const wchar_t *ModuleName,
    const wchar_t *Topic,
    FARHELPFLAGS Flags
);

enum ADVANCED_CONTROL_COMMANDS
{
	ACTL_GETFARMANAGERVERSION       = 0,
	ACTL_WAITKEY                    = 2,
	ACTL_GETCOLOR                   = 3,
	ACTL_GETARRAYCOLOR              = 4,
	ACTL_GETWINDOWINFO              = 6,
	ACTL_GETWINDOWCOUNT             = 7,
	ACTL_SETCURRENTWINDOW           = 8,
	ACTL_COMMIT                     = 9,
	ACTL_GETFARHWND                 = 10,
	ACTL_SETARRAYCOLOR              = 16,
	ACTL_REDRAWALL                  = 19,
	ACTL_SYNCHRO                    = 20,
	ACTL_SETPROGRESSSTATE           = 21,
	ACTL_SETPROGRESSVALUE           = 22,
	ACTL_QUIT                       = 23,
	ACTL_GETFARRECT                 = 24,
	ACTL_GETCURSORPOS               = 25,
	ACTL_SETCURSORPOS               = 26,
	ACTL_PROGRESSNOTIFY             = 27,
	ACTL_GETWINDOWTYPE              = 28,


};




enum FAR_MACRO_CONTROL_COMMANDS
{
	MCTL_LOADALL           = 0,
	MCTL_SAVEALL           = 1,
	MCTL_SENDSTRING        = 2,
	MCTL_GETSTATE          = 5,
	MCTL_GETAREA           = 6,
	MCTL_ADDMACRO          = 7,
	MCTL_DELMACRO          = 8,
	MCTL_GETLASTERROR      = 9,
	MCTL_EXECSTRING        = 10,
};

typedef unsigned __int64 FARKEYMACROFLAGS;
static const FARKEYMACROFLAGS
	KMFLAGS_SILENTCHECK         = 0x0000000000000001,
	KMFLAGS_NOSENDKEYSTOPLUGINS = 0x0000000000000002,
	KMFLAGS_ENABLEOUTPUT        = 0x0000000000000004,
	KMFLAGS_LANGMASK            = 0x0000000000000070, // 3 bits reserved for 8 languages
	KMFLAGS_LUA                 = 0x0000000000000000,
	KMFLAGS_MOONSCRIPT          = 0x0000000000000010,
	KMFLAGS_NONE                = 0;

enum FARMACROSENDSTRINGCOMMAND
{
	MSSC_POST              =0,
	MSSC_CHECK             =2,
};

enum FARMACROAREA
{
	MACROAREA_OTHER                      =   0,   // Mode of copying text from the screen; vertical menus
	MACROAREA_SHELL                      =   1,   // File panels
	MACROAREA_VIEWER                     =   2,   // Internal viewer program
	MACROAREA_EDITOR                     =   3,   // Editor
	MACROAREA_DIALOG                     =   4,   // Dialogs
	MACROAREA_SEARCH                     =   5,   // Quick search in panels
	MACROAREA_DISKS                      =   6,   // Menu of disk selection
	MACROAREA_MAINMENU                   =   7,   // Main menu
	MACROAREA_MENU                       =   8,   // Other menus
	MACROAREA_HELP                       =   9,   // Help system
	MACROAREA_INFOPANEL                  =  10,   // Info panel
	MACROAREA_QVIEWPANEL                 =  11,   // Quick view panel
	MACROAREA_TREEPANEL                  =  12,   // Folders tree panel
	MACROAREA_FINDFOLDER                 =  13,   // Find folder
	MACROAREA_USERMENU                   =  14,   // User menu
	MACROAREA_SHELLAUTOCOMPLETION        =  15,   // Autocompletion list in command line
	MACROAREA_DIALOGAUTOCOMPLETION       =  16,   // Autocompletion list in dialogs

	MACROAREA_COMMON                     = 255,
};

enum FARMACROSTATE
{
	MACROSTATE_NOMACRO          = 0,
	MACROSTATE_EXECUTING        = 1,
	MACROSTATE_EXECUTING_COMMON = 2,
	MACROSTATE_RECORDING        = 3,
	MACROSTATE_RECORDING_COMMON = 4,
};

enum FARMACROPARSEERRORCODE
{
	MPEC_SUCCESS = 0,
	MPEC_ERROR   = 1,
};

struct MacroParseResult
{
	size_t StructSize;
	DWORD ErrCode;
	COORD ErrPos;
	const wchar_t *ErrSrc;
};


struct MacroSendMacroText
{
	size_t StructSize;
	FARKEYMACROFLAGS Flags;
	INPUT_RECORD AKey;
	const wchar_t *SequenceText;
};

typedef unsigned __int64 FARADDKEYMACROFLAGS;
static const FARADDKEYMACROFLAGS
	AKMFLAGS_NONE                = 0;

typedef intptr_t (WINAPI *FARMACROCALLBACK)(void* Id,FARADDKEYMACROFLAGS Flags);

struct MacroAddMacro
{
	size_t StructSize;
	void* Id;
	const wchar_t *SequenceText;
	const wchar_t *Description;
	FARKEYMACROFLAGS Flags;
	INPUT_RECORD AKey;
	enum FARMACROAREA Area;
	FARMACROCALLBACK Callback;
};

enum FARMACROVARTYPE
{
	FMVT_UNKNOWN                = 0,
	FMVT_INTEGER                = 1,
	FMVT_STRING                 = 2,
	FMVT_DOUBLE                 = 3,
	FMVT_BOOLEAN                = 4,
	FMVT_BINARY                 = 5,
	FMVT_POINTER                = 6,
	FMVT_NIL                    = 7,
	FMVT_ARRAY                  = 8,
	FMVT_PANEL                  = 9,
};

struct FarMacroValue
{
	enum FARMACROVARTYPE Type;
	union
	{
		__int64        Integer;
		__int64        Boolean;
		double         Double;
		const wchar_t *String;
		void          *Pointer;
		struct
		{
			void *Data;
			size_t Size;
		} Binary;
		struct
		{
			struct FarMacroValue *Values;
			size_t Count;
		} Array;
	}
#ifndef __cplusplus
	Value
#endif
	;
#ifdef __cplusplus
	FarMacroValue()                   { Type=FMVT_NIL; }
	FarMacroValue(int v)              { Type=FMVT_INTEGER; Integer=v; }
	FarMacroValue(unsigned int v)     { Type=FMVT_INTEGER; Integer=v; }
	FarMacroValue(__int64 v)          { Type=FMVT_INTEGER; Integer=v; }
	FarMacroValue(unsigned __int64 v) { Type=FMVT_INTEGER; Integer=v; }
	FarMacroValue(bool v)             { Type=FMVT_BOOLEAN; Boolean=v?1:0; }
	FarMacroValue(double v)           { Type=FMVT_DOUBLE; Double=v; }
	FarMacroValue(const wchar_t* v)   { Type=FMVT_STRING; String=v; }
	FarMacroValue(void* v)            { Type=FMVT_POINTER; Pointer=v; }
	FarMacroValue(const GUID& v)      { Type=FMVT_BINARY; Binary.Data=&const_cast<GUID&>(v); Binary.Size=sizeof(GUID); }
	FarMacroValue(FarMacroValue* arr,size_t count) { Type=FMVT_ARRAY; Array.Values=arr; Array.Count=count; }
#endif
};

struct FarMacroCall
{
	size_t StructSize;
	size_t Count;
	struct FarMacroValue *Values;
	void (WINAPI *Callback)(void *CallbackData, struct FarMacroValue *Values, size_t Count);
	void *CallbackData;
};

struct FarGetValue
{
	size_t StructSize;
	intptr_t Type;
	struct FarMacroValue Value;
};

struct MacroExecuteString
{
	size_t StructSize;
	FARKEYMACROFLAGS Flags;
	const wchar_t *SequenceText;
	size_t InCount;
	struct FarMacroValue *InValues;
	size_t OutCount;
	const struct FarMacroValue *OutValues;
};

struct FarMacroLoad
{
	size_t StructSize;
	const wchar_t *Path;
	unsigned __int64 Flags;
};

typedef unsigned __int64 FARSETCOLORFLAGS;
static const FARSETCOLORFLAGS
	FSETCLR_REDRAW                 = 0x0000000000000001ULL,
	FSETCLR_NONE                   = 0;

struct FarSetColors
{
	size_t StructSize;
	FARSETCOLORFLAGS Flags;
	size_t StartIndex;
	size_t ColorsCount;
	struct FarColor* Colors;
};

enum WINDOWINFO_TYPE
{
	WTYPE_DESKTOP                   = 0,
	WTYPE_PANELS                    = 1,
	WTYPE_VIEWER                    = 2,
	WTYPE_EDITOR                    = 3,
	WTYPE_DIALOG                    = 4,
	WTYPE_VMENU                     = 5,
	WTYPE_HELP                      = 6,
	WTYPE_SEARCH                    = 7, // Far3bis 4187+
};

typedef unsigned __int64 WINDOWINFO_FLAGS;
static const WINDOWINFO_FLAGS
	WIF_MODIFIED = 0x0000000000000001ULL,
	WIF_CURRENT  = 0x0000000000000002ULL,
	WIF_MODAL    = 0x0000000000000004ULL;

struct WindowInfo
{
	size_t StructSize;
	intptr_t Id;
	wchar_t *TypeName;
	wchar_t *Name;
	intptr_t TypeNameSize;
	intptr_t NameSize;
	intptr_t Pos;
	enum WINDOWINFO_TYPE Type;
	WINDOWINFO_FLAGS Flags;
};

struct WindowType
{
	size_t StructSize;
	enum WINDOWINFO_TYPE Type;
};

enum TASKBARPROGRESSTATE
{
	TBPS_NOPROGRESS   =0x0,
	TBPS_INDETERMINATE=0x1,
	TBPS_NORMAL       =0x2,
	TBPS_ERROR        =0x4,
	TBPS_PAUSED       =0x8,
};

struct ProgressValue
{
	size_t StructSize;
	unsigned __int64 Completed;
	unsigned __int64 Total;
};

enum VIEWER_CONTROL_COMMANDS
{
	VCTL_GETINFO                    = 0,
	VCTL_QUIT                       = 1,
	VCTL_REDRAW                     = 2,
	VCTL_SETKEYBAR                  = 3,
	VCTL_SETPOSITION                = 4,
	VCTL_SELECT                     = 5,
	VCTL_SETMODE                    = 6,
	VCTL_GETFILENAME                = 7,
};

typedef unsigned __int64 VIEWER_OPTIONS;
static const VIEWER_OPTIONS
	VOPT_SAVEFILEPOSITION   = 0x0000000000000001ULL,
	VOPT_AUTODETECTCODEPAGE = 0x0000000000000002ULL,
	VOPT_NONE               = 0;

enum VIEWER_SETMODE_TYPES
{
	VSMT_VIEWMODE                   = 0,
	VSMT_WRAP                       = 1,
	VSMT_WORDWRAP                   = 2,
};

typedef unsigned __int64 VIEWER_SETMODEFLAGS_TYPES;
static const VIEWER_SETMODEFLAGS_TYPES
	VSMFL_REDRAW    = 0x0000000000000001ULL;

struct ViewerSetMode
{
	size_t StructSize;
	enum VIEWER_SETMODE_TYPES Type;
	union
	{
		intptr_t iParam;
		wchar_t *wszParam;
	}
#ifndef __cplusplus
	Param
#endif
	;
	VIEWER_SETMODEFLAGS_TYPES Flags;
};

struct ViewerSelect
{
	size_t StructSize;
	__int64 BlockStartPos;
	__int64 BlockLen;
};

typedef unsigned __int64 VIEWER_SETPOS_FLAGS;
static const VIEWER_SETPOS_FLAGS
	VSP_NOREDRAW    = 0x0000000000000001ULL,
	VSP_PERCENT     = 0x0000000000000002ULL,
	VSP_RELATIVE    = 0x0000000000000004ULL,
	VSP_NORETNEWPOS = 0x0000000000000008ULL;

struct ViewerSetPosition
{
	size_t StructSize;
	VIEWER_SETPOS_FLAGS Flags;
	__int64 StartPos;
	__int64 LeftPos;
};

typedef unsigned __int64 VIEWER_MODE_FLAGS;
static const VIEWER_MODE_FLAGS
	VMF_WRAP     = 0x0000000000000001ULL,
	VMF_WORDWRAP = 0x0000000000000002ULL;

enum VIEWER_MODE_TYPE
{
	VMT_TEXT    =0,
	VMT_HEX     =1,
	VMT_DUMP    =2,
};

struct ViewerMode
{
	uintptr_t CodePage;
	VIEWER_MODE_FLAGS Flags;
	enum VIEWER_MODE_TYPE ViewMode;
};

struct ViewerInfo
{
	size_t StructSize;
	intptr_t ViewerID;
	intptr_t TabSize;
	struct ViewerMode CurMode;
	__int64 FileSize;
	__int64 FilePos;
	__int64 LeftPos;
	VIEWER_OPTIONS Options;
	intptr_t WindowSizeX;
	intptr_t WindowSizeY;
};

enum VIEWER_EVENTS
{
	VE_READ       =0,
	VE_CLOSE      =1,

	VE_GOTFOCUS   =6,
	VE_KILLFOCUS  =7,
};


enum EDITOR_EVENTS
{
	EE_READ       =0,
	EE_SAVE       =1,
	EE_REDRAW     =2,
	EE_CLOSE      =3,

	EE_GOTFOCUS   =6,
	EE_KILLFOCUS  =7,
	EE_CHANGE     =8,
};

enum DIALOG_EVENTS
{
	DE_DLGPROCINIT    =0,
	DE_DEFDLGPROCINIT =1,
	DE_DLGPROCEND     =2,
};

enum SYNCHRO_EVENTS
{
	SE_COMMONSYNCHRO  =0,
};

#define EEREDRAW_ALL    (void*)0
#define CURRENT_EDITOR -1

enum EDITOR_CONTROL_COMMANDS
{
	ECTL_GETSTRING                  = 0,
	ECTL_SETSTRING                  = 1,
	ECTL_INSERTSTRING               = 2,
	ECTL_DELETESTRING               = 3,
	ECTL_DELETECHAR                 = 4,
	ECTL_INSERTTEXT                 = 5,
	ECTL_GETINFO                    = 6,
	ECTL_SETPOSITION                = 7,
	ECTL_SELECT                     = 8,
	ECTL_REDRAW                     = 9,
	ECTL_TABTOREAL                  = 10,
	ECTL_REALTOTAB                  = 11,
	ECTL_EXPANDTABS                 = 12,
	ECTL_SETTITLE                   = 13,
	ECTL_READINPUT                  = 14,
	ECTL_PROCESSINPUT               = 15,
	ECTL_ADDCOLOR                   = 16,
	ECTL_GETCOLOR                   = 17,
	ECTL_SAVEFILE                   = 18,
	ECTL_QUIT                       = 19,
	ECTL_SETKEYBAR                  = 20,

	ECTL_SETPARAM                   = 22,
	ECTL_GETBOOKMARKS               = 23,
	ECTL_DELETEBLOCK                = 25,
	ECTL_ADDSESSIONBOOKMARK         = 26,
	ECTL_PREVSESSIONBOOKMARK        = 27,
	ECTL_NEXTSESSIONBOOKMARK        = 28,
	ECTL_CLEARSESSIONBOOKMARKS      = 29,
	ECTL_DELETESESSIONBOOKMARK      = 30,
	ECTL_GETSESSIONBOOKMARKS        = 31,
	ECTL_UNDOREDO                   = 32,
	ECTL_GETFILENAME                = 33,
	ECTL_DELCOLOR                   = 34,
	ECTL_SUBSCRIBECHANGEEVENT       = 36,
	ECTL_UNSUBSCRIBECHANGEEVENT     = 37,
	ECTL_GETTITLE                   = 38,

	//Maximus. Требуется для "прозрачного" заворота строк (declined in official)
	ECTL_DROPMODIFEDFLAG            = 999,
};

enum EDITOR_SETPARAMETER_TYPES
{
	ESPT_TABSIZE                    = 0,
	ESPT_EXPANDTABS                 = 1,
	ESPT_AUTOINDENT                 = 2,
	ESPT_CURSORBEYONDEOL            = 3,
	ESPT_CHARCODEBASE               = 4,
	ESPT_CODEPAGE                   = 5,
	ESPT_SAVEFILEPOSITION           = 6,
	ESPT_LOCKMODE                   = 7,
	ESPT_SETWORDDIV                 = 8,
	ESPT_GETWORDDIV                 = 9,
	ESPT_SHOWWHITESPACE             = 10,
	ESPT_SETBOM                     = 11,
};



struct EditorSetParameter
{
	size_t StructSize;
	enum EDITOR_SETPARAMETER_TYPES Type;
	union
	{
		intptr_t iParam;
		wchar_t *wszParam;
		intptr_t Reserved;
	}
#ifndef __cplusplus
	Param
#endif
	;
	unsigned __int64 Flags;
	size_t Size;
};


enum EDITOR_UNDOREDO_COMMANDS
{
	EUR_BEGIN                       = 0,
	EUR_END                         = 1,
	EUR_UNDO                        = 2,
	EUR_REDO                        = 3,
};


struct EditorUndoRedo
{
	size_t StructSize;
	enum EDITOR_UNDOREDO_COMMANDS Command;
};

struct EditorGetString
{
	size_t StructSize;
	intptr_t StringNumber;
	intptr_t StringLength;
	const wchar_t *StringText;
	const wchar_t *StringEOL;
	intptr_t SelStart;
	intptr_t SelEnd;
};


struct EditorSetString
{
	size_t StructSize;
	intptr_t StringNumber;
	intptr_t StringLength;
	const wchar_t *StringText;
	const wchar_t *StringEOL;
};

enum EXPAND_TABS
{
	EXPAND_NOTABS                   = 0,
	EXPAND_ALLTABS                  = 1,
	EXPAND_NEWTABS                  = 2,
};


enum EDITOR_OPTIONS
{
	EOPT_EXPANDALLTABS     = 0x00000001,
	EOPT_PERSISTENTBLOCKS  = 0x00000002,
	EOPT_DELREMOVESBLOCKS  = 0x00000004,
	EOPT_AUTOINDENT        = 0x00000008,
	EOPT_SAVEFILEPOSITION  = 0x00000010,
	EOPT_AUTODETECTCODEPAGE= 0x00000020,
	EOPT_CURSORBEYONDEOL   = 0x00000040,
	EOPT_EXPANDONLYNEWTABS = 0x00000080,
	EOPT_SHOWWHITESPACE    = 0x00000100,
	EOPT_BOM               = 0x00000200,
	EOPT_SHOWLINEBREAK     = 0x00000400,
};


enum EDITOR_BLOCK_TYPES
{
	BTYPE_NONE                      = 0,
	BTYPE_STREAM                    = 1,
	BTYPE_COLUMN                    = 2,
};

enum EDITOR_CURRENTSTATE
{
	ECSTATE_MODIFIED       = 0x00000001,
	ECSTATE_SAVED          = 0x00000002,
	ECSTATE_LOCKED         = 0x00000004,
};


struct EditorInfo
{
	size_t StructSize;
	intptr_t EditorID;
	intptr_t WindowSizeX;
	intptr_t WindowSizeY;
	intptr_t TotalLines;
	intptr_t CurLine;
	intptr_t CurPos;
	intptr_t CurTabPos;
	intptr_t TopScreenLine;
	intptr_t LeftPos;
	intptr_t Overtype;
	intptr_t BlockType;
	intptr_t BlockStartLine;
	uintptr_t Options;
	intptr_t TabSize;
	size_t BookmarkCount;
	size_t SessionBookmarkCount;
	uintptr_t CurState;
	uintptr_t CodePage;
};

struct EditorBookmarks
{
	size_t StructSize;
	size_t Size;
	size_t Count;
	intptr_t *Line;
	intptr_t *Cursor;
	intptr_t *ScreenLine;
	intptr_t *LeftPos;
};

struct EditorSetPosition
{
	size_t StructSize;
	intptr_t CurLine;
	intptr_t CurPos;
	intptr_t CurTabPos;
	intptr_t TopScreenLine;
	intptr_t LeftPos;
	intptr_t Overtype;
};


struct EditorSelect
{
	size_t StructSize;
	intptr_t BlockType;
	intptr_t BlockStartLine;
	intptr_t BlockStartPos;
	intptr_t BlockWidth;
	intptr_t BlockHeight;
};


struct EditorConvertPos
{
	size_t StructSize;
	intptr_t StringNumber;
	intptr_t SrcPos;
	intptr_t DestPos;
};

typedef unsigned __int64 EDITORCOLORFLAGS;
static const EDITORCOLORFLAGS
	ECF_TABMARKFIRST   = 0x0000000000000001ULL,
	ECF_TABMARKCURRENT = 0x0000000000000002ULL,
	ECF_AUTODELETE     = 0x0000000000000004ULL;

struct EditorColor
{
	size_t StructSize;
	intptr_t StringNumber;
	intptr_t ColorItem;
	intptr_t StartPos;
	intptr_t EndPos;
	uintptr_t Priority;
	EDITORCOLORFLAGS Flags;
	struct FarColor Color;
	GUID Owner;
};

struct EditorDeleteColor
{
	size_t StructSize;
	GUID Owner;
	intptr_t StringNumber;
	intptr_t StartPos;
};

#define EDITOR_COLOR_NORMAL_PRIORITY 0x80000000U

struct EditorSaveFile
{
	size_t StructSize;
	const wchar_t *FileName;
	const wchar_t *FileEOL;
	uintptr_t CodePage;
};

enum EDITOR_CHANGETYPE
{
	ECTYPE_CHANGED = 0,
	ECTYPE_ADDED   = 1,
	ECTYPE_DELETED = 2,
};

struct EditorChange
{
	size_t StructSize;
	enum EDITOR_CHANGETYPE Type;
	intptr_t StringNumber;
};

struct EditorSubscribeChangeEvent
{
	size_t StructSize;
	GUID PluginId;
};

typedef unsigned __int64 INPUTBOXFLAGS;
static const INPUTBOXFLAGS
	FIB_ENABLEEMPTY      = 0x0000000000000001ULL,
	FIB_PASSWORD         = 0x0000000000000002ULL,
	FIB_EXPANDENV        = 0x0000000000000004ULL,
	FIB_NOUSELASTHISTORY = 0x0000000000000008ULL,
	FIB_BUTTONS          = 0x0000000000000010ULL,
	FIB_NOAMPERSAND      = 0x0000000000000020ULL,
	FIB_EDITPATH         = 0x0000000000000040ULL,
	FIB_EDITPATHEXEC     = 0x0000000000000080ULL,
	FIB_NONE             = 0;

typedef intptr_t (WINAPI *FARAPIINPUTBOX)(
    const GUID* PluginId,
    const GUID* Id,
    const wchar_t *Title,
    const wchar_t *SubTitle,
    const wchar_t *HistoryName,
    const wchar_t *SrcText,
    wchar_t *DestText,
    size_t DestSize,
    const wchar_t *HelpTopic,
    INPUTBOXFLAGS Flags
);

enum FAR_PLUGINS_CONTROL_COMMANDS
{
	PCTL_LOADPLUGIN           = 0,
	PCTL_UNLOADPLUGIN         = 1,
	PCTL_FORCEDLOADPLUGIN     = 2,
	PCTL_FINDPLUGIN           = 3,
	PCTL_GETPLUGININFORMATION = 4,
	PCTL_GETPLUGINS           = 5,
};

enum FAR_PLUGIN_LOAD_TYPE
{
	PLT_PATH = 0,
};

enum FAR_PLUGIN_FIND_TYPE
{
	PFM_GUID       = 0,
	PFM_MODULENAME = 1,
};

typedef unsigned __int64 FAR_PLUGIN_FLAGS;
static const FAR_PLUGIN_FLAGS
	FPF_LOADED         = 0x0000000000000001ULL,
	FPF_ANSI           = 0x1000000000000000ULL,
	FPF_FAR2           = 0x2000000000000000ULL,
	FPF_NONE           = 0;

enum FAR_FILE_FILTER_CONTROL_COMMANDS
{
	FFCTL_CREATEFILEFILTER          = 0,
	FFCTL_FREEFILEFILTER            = 1,
	FFCTL_OPENFILTERSMENU           = 2,
	FFCTL_STARTINGTOFILTER          = 3,
	FFCTL_ISFILEINFILTER            = 4,
};

enum FAR_FILE_FILTER_TYPE
{
	FFT_PANEL                       = 0,
	FFT_FINDFILE                    = 1,
	FFT_COPY                        = 2,
	FFT_SELECT                      = 3,
	FFT_CUSTOM                      = 4,
};

enum FAR_REGEXP_CONTROL_COMMANDS
{
	RECTL_CREATE                    = 0,
	RECTL_FREE                      = 1,
	RECTL_COMPILE                   = 2,
	RECTL_OPTIMIZE                  = 3,
	RECTL_MATCHEX                   = 4,
	RECTL_SEARCHEX                  = 5,
	RECTL_BRACKETSCOUNT             = 6,
};

struct RegExpMatch
{
	intptr_t start,end;
};

struct RegExpSearch
{
	const wchar_t* Text;
	intptr_t Position;
	intptr_t Length;
	struct RegExpMatch* Match;
	intptr_t Count;
	void* Reserved;
};

enum FAR_SETTINGS_CONTROL_COMMANDS
{
	SCTL_CREATE                     = 0,
	SCTL_FREE                       = 1,
	SCTL_SET                        = 2,
	SCTL_GET                        = 3,
	SCTL_ENUM                       = 4,
	SCTL_DELETE                     = 5,
	SCTL_CREATESUBKEY               = 6,
	SCTL_OPENSUBKEY                 = 7,
};

enum FARSETTINGSTYPES
{
	FST_UNKNOWN                     = 0,
	FST_SUBKEY                      = 1,
	FST_QWORD                       = 2,
	FST_STRING                      = 3,
	FST_DATA                        = 4,
};

enum FARSETTINGS_SUBFOLDERS
{
	FSSF_ROOT                       =  0,
	FSSF_HISTORY_CMD                =  1,
	FSSF_HISTORY_FOLDER             =  2,
	FSSF_HISTORY_VIEW               =  3,
	FSSF_HISTORY_EDIT               =  4,
	FSSF_HISTORY_EXTERNAL           =  5,
	FSSF_FOLDERSHORTCUT_0           =  6,
	FSSF_FOLDERSHORTCUT_1           =  7,
	FSSF_FOLDERSHORTCUT_2           =  8,
	FSSF_FOLDERSHORTCUT_3           =  9,
	FSSF_FOLDERSHORTCUT_4           = 10,
	FSSF_FOLDERSHORTCUT_5           = 11,
	FSSF_FOLDERSHORTCUT_6           = 12,
	FSSF_FOLDERSHORTCUT_7           = 13,
	FSSF_FOLDERSHORTCUT_8           = 14,
	FSSF_FOLDERSHORTCUT_9           = 15,
	FSSF_CONFIRMATIONS              = 16,
	FSSF_SYSTEM                     = 17,
	FSSF_PANEL                      = 18,
	FSSF_EDITOR                     = 19,
	FSSF_SCREEN                     = 20,
	FSSF_DIALOG                     = 21,
	FSSF_INTERFACE                  = 22,
	FSSF_PANELLAYOUT                = 23,
};

enum FAR_PLUGIN_SETTINGS_LOCATION
{
	PSL_ROAMING = 0,
	PSL_LOCAL   = 1,
};

struct FarSettingsCreate
{
	size_t StructSize;
	GUID Guid;
	HANDLE Handle;
};

struct FarSettingsItem
{
	size_t StructSize;
	size_t Root;
	const wchar_t* Name;
	enum FARSETTINGSTYPES Type;
	union
	{
		unsigned __int64 Number;
		const wchar_t* String;
		struct
		{
			size_t Size;
			const void* Data;
		} Data;
	}
#ifndef __cplusplus
	Value
#endif
	;
};

struct FarSettingsName
{
	const wchar_t* Name;
	enum FARSETTINGSTYPES Type;
};

struct FarSettingsHistory
{
	const wchar_t* Name;
	const wchar_t* Param;
	GUID PluginId;
	const wchar_t* File;
	FILETIME Time;
	BOOL Lock;
};

struct FarSettingsEnum
{
	size_t StructSize;
	size_t Root;
	size_t Count;
	union
	{
		const struct FarSettingsName* Items;
		const struct FarSettingsHistory* Histories;
	}
#ifndef __cplusplus
	Value
#endif
	;
};

struct FarSettingsValue
{
	size_t StructSize;
	size_t Root;
	const wchar_t* Value;
};

typedef intptr_t (WINAPI *FARAPIPANELCONTROL)(
    HANDLE hPanel,
    enum FILE_CONTROL_COMMANDS Command,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t(WINAPI *FARAPIADVCONTROL)(
    const GUID* PluginId,
    enum ADVANCED_CONTROL_COMMANDS Command,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t (WINAPI *FARAPIVIEWERCONTROL)(
    intptr_t ViewerID,
    enum VIEWER_CONTROL_COMMANDS Command,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t (WINAPI *FARAPIEDITORCONTROL)(
    intptr_t EditorID,
    enum EDITOR_CONTROL_COMMANDS Command,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t (WINAPI *FARAPIMACROCONTROL)(
    const GUID* PluginId,
    enum FAR_MACRO_CONTROL_COMMANDS Command,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t (WINAPI *FARAPIPLUGINSCONTROL)(
    HANDLE hHandle,
    enum FAR_PLUGINS_CONTROL_COMMANDS Command,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t (WINAPI *FARAPIFILEFILTERCONTROL)(
    HANDLE hHandle,
    enum FAR_FILE_FILTER_CONTROL_COMMANDS Command,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t (WINAPI *FARAPIREGEXPCONTROL)(
    HANDLE hHandle,
    enum FAR_REGEXP_CONTROL_COMMANDS Command,
    intptr_t Param1,
    void* Param2
);

typedef intptr_t (WINAPI *FARAPISETTINGSCONTROL)(
    HANDLE hHandle,
    enum FAR_SETTINGS_CONTROL_COMMANDS Command,
    intptr_t Param1,
    void* Param2
);

enum FARCLIPBOARD_TYPE
{
	FCT_ANY=0,
	FCT_STREAM=1,
	FCT_COLUMN=2
};

// <C&C++>
typedef int (WINAPIV *FARSTDSPRINTF)(wchar_t *Buffer,const wchar_t *Format,...);
typedef int (WINAPIV *FARSTDSNPRINTF)(wchar_t *Buffer,size_t Sizebuf,const wchar_t *Format,...);
typedef int (WINAPIV *FARSTDSSCANF)(const wchar_t *Buffer, const wchar_t *Format,...);
// </C&C++>
typedef void (WINAPI *FARSTDQSORT)(void *base, size_t nelem, size_t width, int (WINAPI *fcmp)(const void *, const void *,void *userparam),void *userparam);
typedef void   *(WINAPI *FARSTDBSEARCH)(const void *key, const void *base, size_t nelem, size_t width, int (WINAPI *fcmp)(const void *, const void *,void *userparam),void *userparam);
typedef size_t (WINAPI *FARSTDGETFILEOWNER)(const wchar_t *Computer,const wchar_t *Name,wchar_t *Owner,size_t Size);
typedef size_t (WINAPI *FARSTDGETNUMBEROFLINKS)(const wchar_t *Name);
typedef int (WINAPI *FARSTDATOI)(const wchar_t *s);
typedef __int64 (WINAPI *FARSTDATOI64)(const wchar_t *s);
typedef wchar_t   *(WINAPI *FARSTDITOA64)(__int64 value, wchar_t *Str, int radix);
typedef wchar_t   *(WINAPI *FARSTDITOA)(int value, wchar_t *Str, int radix);
typedef wchar_t   *(WINAPI *FARSTDLTRIM)(wchar_t *Str);
typedef wchar_t   *(WINAPI *FARSTDRTRIM)(wchar_t *Str);
typedef wchar_t   *(WINAPI *FARSTDTRIM)(wchar_t *Str);
typedef wchar_t   *(WINAPI *FARSTDTRUNCSTR)(wchar_t *Str,intptr_t MaxLength);
typedef wchar_t   *(WINAPI *FARSTDTRUNCPATHSTR)(wchar_t *Str,intptr_t MaxLength);
typedef wchar_t   *(WINAPI *FARSTDQUOTESPACEONLY)(wchar_t *Str);
typedef const wchar_t*(WINAPI *FARSTDPOINTTONAME)(const wchar_t *Path);
typedef BOOL (WINAPI *FARSTDADDENDSLASH)(wchar_t *Path);
typedef BOOL (WINAPI *FARSTDCOPYTOCLIPBOARD)(enum FARCLIPBOARD_TYPE Type, const wchar_t *Data);
typedef size_t (WINAPI *FARSTDPASTEFROMCLIPBOARD)(enum FARCLIPBOARD_TYPE Type, wchar_t *Data, size_t Size);
typedef int (WINAPI *FARSTDLOCALISLOWER)(wchar_t Ch);
typedef int (WINAPI *FARSTDLOCALISUPPER)(wchar_t Ch);
typedef int (WINAPI *FARSTDLOCALISALPHA)(wchar_t Ch);
typedef int (WINAPI *FARSTDLOCALISALPHANUM)(wchar_t Ch);
typedef wchar_t (WINAPI *FARSTDLOCALUPPER)(wchar_t LowerChar);
typedef wchar_t (WINAPI *FARSTDLOCALLOWER)(wchar_t UpperChar);
typedef void (WINAPI *FARSTDLOCALUPPERBUF)(wchar_t *Buf,intptr_t Length);
typedef void (WINAPI *FARSTDLOCALLOWERBUF)(wchar_t *Buf,intptr_t Length);
typedef void (WINAPI *FARSTDLOCALSTRUPR)(wchar_t *s1);
typedef void (WINAPI *FARSTDLOCALSTRLWR)(wchar_t *s1);
typedef int (WINAPI *FARSTDLOCALSTRICMP)(const wchar_t *s1,const wchar_t *s2);
typedef int (WINAPI *FARSTDLOCALSTRNICMP)(const wchar_t *s1,const wchar_t *s2,intptr_t n);

typedef unsigned __int64 PROCESSNAME_FLAGS;
static const PROCESSNAME_FLAGS
	//             0xFFFF - length
	//           0xFF0000 - mode
	// 0xFFFFFFFFFF000000 - flags
	PN_CMPNAME          = 0x0000000000000000ULL,
	PN_CMPNAMELIST      = 0x0000000000010000ULL,
	PN_GENERATENAME     = 0x0000000000020000ULL,
	PN_CHECKMASK        = 0x0000000000030000ULL,

	PN_SKIPPATH         = 0x0000000001000000ULL,
	PN_SHOWERRORMESSAGE = 0x0000000002000000ULL;

typedef size_t (WINAPI *FARSTDPROCESSNAME)(const wchar_t *param1, wchar_t *param2, size_t size, PROCESSNAME_FLAGS flags);

typedef void (WINAPI *FARSTDUNQUOTE)(wchar_t *Str);

typedef unsigned __int64 XLAT_FLAGS;
static const XLAT_FLAGS
	XLAT_SWITCHKEYBLAYOUT  = 0x0000000000000001ULL,
	XLAT_SWITCHKEYBBEEP    = 0x0000000000000002ULL,
	XLAT_USEKEYBLAYOUTNAME = 0x0000000000000004ULL,
	XLAT_CONVERTALLCMDLINE = 0x0000000000010000ULL,
	XLAT_NONE              = 0;

typedef size_t (WINAPI *FARSTDINPUTRECORDTOKEYNAME)(const INPUT_RECORD* Key, wchar_t *KeyText, size_t Size);

typedef wchar_t*(WINAPI *FARSTDXLAT)(wchar_t *Line,intptr_t StartPos,intptr_t EndPos,XLAT_FLAGS Flags);

typedef BOOL (WINAPI *FARSTDKEYNAMETOINPUTRECORD)(const wchar_t *Name,INPUT_RECORD* Key);

typedef int (WINAPI *FRSUSERFUNC)(
    const struct PluginPanelItem *FData,
    const wchar_t *FullName,
    void *Param
);

typedef unsigned __int64 FRSMODE;
static const FRSMODE
	FRS_RETUPDIR             = 0x0000000000000001ULL,
	FRS_RECUR                = 0x0000000000000002ULL,
	FRS_SCANSYMLINK          = 0x0000000000000004ULL;

typedef void (WINAPI *FARSTDRECURSIVESEARCH)(const wchar_t *InitDir,const wchar_t *Mask,FRSUSERFUNC Func,FRSMODE Flags,void *Param);
typedef size_t (WINAPI *FARSTDMKTEMP)(wchar_t *Dest, size_t DestSize, const wchar_t *Prefix);
typedef size_t (WINAPI *FARSTDGETPATHROOT)(const wchar_t *Path,wchar_t *Root, size_t DestSize);

enum LINK_TYPE
{
	LINK_HARDLINK         = 1,
	LINK_JUNCTION         = 2,
	LINK_VOLMOUNT         = 3,
	LINK_SYMLINKFILE      = 4,
	LINK_SYMLINKDIR       = 5,
	LINK_SYMLINK          = 6,
};

typedef unsigned __int64 MKLINK_FLAGS;
static const MKLINK_FLAGS
	MLF_SHOWERRMSG       = 0x0000000000010000ULL,
	MLF_DONOTUPDATEPANEL = 0x0000000000020000ULL,
	MLF_HOLDTARGET       = 0x0000000000040000ULL,
	MLF_NONE             = 0;

typedef BOOL (WINAPI *FARSTDMKLINK)(const wchar_t *Src,const wchar_t *Dest,enum LINK_TYPE Type, MKLINK_FLAGS Flags);
typedef size_t (WINAPI *FARGETREPARSEPOINTINFO)(const wchar_t *Src, wchar_t *Dest, size_t DestSize);

enum CONVERTPATHMODES
{
	CPM_FULL                        = 0,
	CPM_REAL                        = 1,
	CPM_NATIVE                      = 2,
};

typedef size_t (WINAPI *FARCONVERTPATH)(enum CONVERTPATHMODES Mode, const wchar_t *Src, wchar_t *Dest, size_t DestSize);

typedef size_t (WINAPI *FARGETCURRENTDIRECTORY)(size_t Size, wchar_t* Buffer);

typedef unsigned __int64 FARFORMATFILESIZEFLAGS;
static const FARFORMATFILESIZEFLAGS
	FFFS_COMMAS                 = 0x0100000000000000LL,
	FFFS_FLOATSIZE              = 0x0200000000000000LL,
	FFFS_SHOWBYTESINDEX         = 0x0400000000000000LL,
	FFFS_ECONOMIC               = 0x0800000000000000LL,
	FFFS_THOUSAND               = 0x1000000000000000LL,
	FFFS_MINSIZEINDEX           = 0x2000000000000000LL,
	FFFS_MINSIZEINDEX_MASK      = 0x0000000000000003LL;

typedef size_t (WINAPI *FARFORMATFILESIZE)(unsigned __int64 Size, intptr_t Width, FARFORMATFILESIZEFLAGS Flags, wchar_t *Dest, size_t DestSize);

typedef struct FarStandardFunctions
{
	size_t StructSize;

	FARSTDATOI                 atoi;
	FARSTDATOI64               atoi64;
	FARSTDITOA                 itoa;
	FARSTDITOA64               itoa64;
	// <C&C++>
	FARSTDSPRINTF              sprintf;
	FARSTDSSCANF               sscanf;
	// </C&C++>
	FARSTDQSORT                qsort;
	FARSTDBSEARCH              bsearch;
	// <C&C++>
	FARSTDSNPRINTF             snprintf;
	// </C&C++>

	FARSTDLOCALISLOWER         LIsLower;
	FARSTDLOCALISUPPER         LIsUpper;
	FARSTDLOCALISALPHA         LIsAlpha;
	FARSTDLOCALISALPHANUM      LIsAlphanum;
	FARSTDLOCALUPPER           LUpper;
	FARSTDLOCALLOWER           LLower;
	FARSTDLOCALUPPERBUF        LUpperBuf;
	FARSTDLOCALLOWERBUF        LLowerBuf;
	FARSTDLOCALSTRUPR          LStrupr;
	FARSTDLOCALSTRLWR          LStrlwr;
	FARSTDLOCALSTRICMP         LStricmp;
	FARSTDLOCALSTRNICMP        LStrnicmp;

	FARSTDUNQUOTE              Unquote;
	FARSTDLTRIM                LTrim;
	FARSTDRTRIM                RTrim;
	FARSTDTRIM                 Trim;
	FARSTDTRUNCSTR             TruncStr;
	FARSTDTRUNCPATHSTR         TruncPathStr;
	FARSTDQUOTESPACEONLY       QuoteSpaceOnly;
	FARSTDPOINTTONAME          PointToName;
	FARSTDGETPATHROOT          GetPathRoot;
	FARSTDADDENDSLASH          AddEndSlash;
	FARSTDCOPYTOCLIPBOARD      CopyToClipboard;
	FARSTDPASTEFROMCLIPBOARD   PasteFromClipboard;
	FARSTDINPUTRECORDTOKEYNAME FarInputRecordToName;
	FARSTDKEYNAMETOINPUTRECORD FarNameToInputRecord;
	FARSTDXLAT                 XLat;
	FARSTDGETFILEOWNER         GetFileOwner;
	FARSTDGETNUMBEROFLINKS     GetNumberOfLinks;
	FARSTDRECURSIVESEARCH      FarRecursiveSearch;
	FARSTDMKTEMP               MkTemp;
	FARSTDPROCESSNAME          ProcessName;
	FARSTDMKLINK               MkLink;
	FARCONVERTPATH             ConvertPath;
	FARGETREPARSEPOINTINFO     GetReparsePointInfo;
	FARGETCURRENTDIRECTORY     GetCurrentDirectory;
	FARFORMATFILESIZE          FormatFileSize;
} FARSTANDARDFUNCTIONS;

struct PluginStartupInfo
{
	size_t StructSize;
	const wchar_t *ModuleName;
	FARAPIMENU             Menu;
	FARAPIMESSAGE          Message;
	FARAPIGETMSG           GetMsg;
	FARAPIPANELCONTROL     PanelControl;
	FARAPISAVESCREEN       SaveScreen;
	FARAPIRESTORESCREEN    RestoreScreen;
	FARAPIGETDIRLIST       GetDirList;
	FARAPIGETPLUGINDIRLIST GetPluginDirList;
	FARAPIFREEDIRLIST      FreeDirList;
	FARAPIFREEPLUGINDIRLIST FreePluginDirList;
	FARAPIVIEWER           Viewer;
	FARAPIEDITOR           Editor;
	FARAPITEXT             Text;
	FARAPIEDITORCONTROL    EditorControl;

	FARSTANDARDFUNCTIONS  *FSF;

	FARAPISHOWHELP         ShowHelp;
	FARAPIADVCONTROL       AdvControl;
	FARAPIINPUTBOX         InputBox;
	FARAPICOLORDIALOG      ColorDialog;
	FARAPIDIALOGINIT       DialogInit;
	FARAPIDIALOGRUN        DialogRun;
	FARAPIDIALOGFREE       DialogFree;

	FARAPISENDDLGMESSAGE   SendDlgMessage;
	FARAPIDEFDLGPROC       DefDlgProc;
	FARAPIVIEWERCONTROL    ViewerControl;
	FARAPIPLUGINSCONTROL   PluginsControl;
	FARAPIFILEFILTERCONTROL FileFilterControl;
	FARAPIREGEXPCONTROL    RegExpControl;
	FARAPIMACROCONTROL     MacroControl;
	FARAPISETTINGSCONTROL  SettingsControl;
	void                  *Private;
	void* Instance;
};

typedef HANDLE (WINAPI *FARAPICREATEFILE)(const wchar_t *Object,DWORD DesiredAccess,DWORD ShareMode,LPSECURITY_ATTRIBUTES SecurityAttributes,DWORD CreationDistribution,DWORD FlagsAndAttributes,HANDLE TemplateFile);
typedef DWORD (WINAPI *FARAPIGETFILEATTRIBUTES)(const wchar_t *FileName);
typedef BOOL (WINAPI *FARAPISETFILEATTRIBUTES)(const wchar_t *FileName,DWORD dwFileAttributes);
typedef BOOL (WINAPI *FARAPIMOVEFILEEX)(const wchar_t *ExistingFileName,const wchar_t *NewFileName,DWORD dwFlags);
typedef BOOL (WINAPI *FARAPIDELETEFILE)(const wchar_t *FileName);
typedef BOOL (WINAPI *FARAPIREMOVEDIRECTORY)(const wchar_t *DirName);
typedef BOOL (WINAPI *FARAPICREATEDIRECTORY)(const wchar_t *PathName,LPSECURITY_ATTRIBUTES lpSecurityAttributes);

struct ArclitePrivateInfo
{
	size_t StructSize;
	FARAPICREATEFILE CreateFile;
	FARAPIGETFILEATTRIBUTES GetFileAttributes;
	FARAPISETFILEATTRIBUTES SetFileAttributes;
	FARAPIMOVEFILEEX MoveFileEx;
	FARAPIDELETEFILE DeleteFile;
	FARAPIREMOVEDIRECTORY RemoveDirectory;
	FARAPICREATEDIRECTORY CreateDirectory;
};

struct NetBoxPrivateInfo
{
	size_t StructSize;
	FARAPICREATEFILE CreateFile;
	FARAPIGETFILEATTRIBUTES GetFileAttributes;
	FARAPISETFILEATTRIBUTES SetFileAttributes;
	FARAPIMOVEFILEEX MoveFileEx;
	FARAPIDELETEFILE DeleteFile;
	FARAPIREMOVEDIRECTORY RemoveDirectory;
	FARAPICREATEDIRECTORY CreateDirectory;
};

struct MacroPluginReturn
{
	intptr_t ReturnType;
	size_t Count;
	struct FarMacroValue *Values;
};

typedef intptr_t (WINAPI *FARAPICALLFAR)(intptr_t CheckCode, struct FarMacroCall* Data);
typedef void (WINAPI *FARAPICALLPLUGIN)(struct MacroPluginReturn* Data, struct FarMacroCall** Target, int* Boolean);

struct MacroPrivateInfo
{
	size_t StructSize;
	FARAPICALLFAR CallFar;
	FARAPICALLPLUGIN CallPlugin;
};

typedef unsigned __int64 PLUGIN_FLAGS;
static const PLUGIN_FLAGS
	PF_PRELOAD        = 0x0000000000000001ULL,
	PF_DISABLEPANELS  = 0x0000000000000002ULL,
	PF_EDITOR         = 0x0000000000000004ULL,
	PF_VIEWER         = 0x0000000000000008ULL,
	PF_FULLCMDLINE    = 0x0000000000000010ULL,
	PF_DIALOG         = 0x0000000000000020ULL,
	PF_NONE           = 0;

struct PluginMenuItem
{
	const GUID *Guids;
	const wchar_t * const *Strings;
	size_t Count;
};

enum VERSION_STAGE
{
	VS_RELEASE                      = 0,
	VS_ALPHA                        = 1,
	VS_BETA                         = 2,
	VS_RC                           = 3,
	VS_BIS                          = 8,
};

struct VersionInfo
{
	DWORD Major;
	DWORD Minor;
	DWORD Revision;
	DWORD Build;
	enum VERSION_STAGE Stage;
};

static __inline BOOL CheckVersion(const struct VersionInfo* Current, const struct VersionInfo* Required)
{
	return (Current->Major > Required->Major) || (Current->Major == Required->Major && Current->Minor > Required->Minor) || (Current->Major == Required->Major && Current->Minor == Required->Minor && Current->Revision > Required->Revision) || (Current->Major == Required->Major && Current->Minor == Required->Minor && Current->Revision == Required->Revision && Current->Build >= Required->Build);
}

static __inline struct VersionInfo MAKEFARVERSION(DWORD Major, DWORD Minor, DWORD Revision, DWORD Build, enum VERSION_STAGE Stage)
{
	struct VersionInfo Info = {Major, Minor, Revision, Build, Stage};
	return Info;
}

#define FARMANAGERVERSION MAKEFARVERSION(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR, FARMANAGERVERSION_REVISION, FARMANAGERVERSION_BUILD, VS_RELEASE)

struct GlobalInfo
{
	size_t StructSize;
	struct VersionInfo MinFarVersion;
	struct VersionInfo Version;
	GUID Guid;
	const wchar_t *Title;
	const wchar_t *Description;
	const wchar_t *Author;
	void* Instance;
};

struct PluginInfo
{
	size_t StructSize;
	PLUGIN_FLAGS Flags;
	struct PluginMenuItem DiskMenu;
	struct PluginMenuItem PluginMenu;
	struct PluginMenuItem PluginConfig;
	const wchar_t *CommandPrefix;
	void* Instance;
};

struct FarGetPluginInformation
{
	size_t StructSize;
	const wchar_t *ModuleName;
	FAR_PLUGIN_FLAGS Flags;
	struct PluginInfo *PInfo;
	struct GlobalInfo *GInfo;
};

typedef unsigned __int64 INFOPANELLINE_FLAGS;
static const INFOPANELLINE_FLAGS
	IPLFLAGS_SEPARATOR      = 0x0000000000000001ULL;

struct InfoPanelLine
{
	const wchar_t *Text;
	const wchar_t *Data;
	INFOPANELLINE_FLAGS Flags;
};

typedef unsigned __int64 PANELMODE_FLAGS;
static const PANELMODE_FLAGS
	PMFLAGS_FULLSCREEN      = 0x0000000000000001ULL,
	PMFLAGS_DETAILEDSTATUS  = 0x0000000000000002ULL,
	PMFLAGS_ALIGNEXTENSIONS = 0x0000000000000004ULL,
	PMFLAGS_CASECONVERSION  = 0x0000000000000008ULL;

struct PanelMode
{
	const wchar_t *ColumnTypes;
	const wchar_t *ColumnWidths;
	const wchar_t * const *ColumnTitles;
	const wchar_t *StatusColumnTypes;
	const wchar_t *StatusColumnWidths;
	PANELMODE_FLAGS Flags;
};

typedef unsigned __int64 OPENPANELINFO_FLAGS;
static const OPENPANELINFO_FLAGS
	OPIF_DISABLEFILTER           = 0x0000000000000001ULL,
	OPIF_DISABLESORTGROUPS       = 0x0000000000000002ULL,
	OPIF_DISABLEHIGHLIGHTING     = 0x0000000000000004ULL,
	OPIF_ADDDOTS                 = 0x0000000000000008ULL,
	OPIF_RAWSELECTION            = 0x0000000000000010ULL,
	OPIF_REALNAMES               = 0x0000000000000020ULL,
	OPIF_SHOWNAMESONLY           = 0x0000000000000040ULL,
	OPIF_SHOWRIGHTALIGNNAMES     = 0x0000000000000080ULL,
	OPIF_SHOWPRESERVECASE        = 0x0000000000000100ULL,
	OPIF_COMPAREFATTIME          = 0x0000000000000400ULL,
	OPIF_EXTERNALGET             = 0x0000000000000800ULL,
	OPIF_EXTERNALPUT             = 0x0000000000001000ULL,
	OPIF_EXTERNALDELETE          = 0x0000000000002000ULL,
	OPIF_EXTERNALMKDIR           = 0x0000000000004000ULL,
	OPIF_USEATTRHIGHLIGHTING     = 0x0000000000008000ULL,
	OPIF_USECRC32                = 0x0000000000010000ULL,
	OPIF_USEFREESIZE             = 0x0000000000020000ULL,
	OPIF_SHORTCUT                = 0x0000000000040000ULL,
	OPIF_NONE                    = 0;

struct KeyBarLabel
{
	struct FarKey Key;
	const wchar_t *Text;
	const wchar_t *LongText;
};

struct KeyBarTitles
{
	size_t CountLabels;
	struct KeyBarLabel *Labels;
};

struct FarSetKeyBarTitles
{
	size_t StructSize;
	struct KeyBarTitles *Titles;
};

typedef unsigned __int64 OPERATION_MODES;
static const OPERATION_MODES
	OPM_SILENT     =0x0000000000000001ULL,
	OPM_FIND       =0x0000000000000002ULL,
	OPM_VIEW       =0x0000000000000004ULL,
	OPM_EDIT       =0x0000000000000008ULL,
	OPM_TOPLEVEL   =0x0000000000000010ULL,
	OPM_DESCR      =0x0000000000000020ULL,
	OPM_QUICKVIEW  =0x0000000000000040ULL,
	OPM_PGDN       =0x0000000000000080ULL,
	OPM_COMMANDS   =0x0000000000000100ULL,
	OPM_NONE       =0;

struct OpenPanelInfo
{
	size_t                       StructSize;
	HANDLE                       hPanel;
	OPENPANELINFO_FLAGS          Flags;
	const wchar_t               *HostFile;
	const wchar_t               *CurDir;
	const wchar_t               *Format;
	const wchar_t               *PanelTitle;
	const struct InfoPanelLine  *InfoLines;
	size_t                       InfoLinesNumber;
	const wchar_t * const       *DescrFiles;
	size_t                       DescrFilesNumber;
	const struct PanelMode      *PanelModesArray;
	size_t                       PanelModesNumber;
	intptr_t                     StartPanelMode;
	enum OPENPANELINFO_SORTMODES StartSortMode;
	intptr_t                     StartSortOrder;
	const struct KeyBarTitles   *KeyBar;
	const wchar_t               *ShortcutData;
	unsigned __int64             FreeSize;
	struct UserDataItem          UserData;
	void* Instance;
};

struct AnalyseInfo
{
	size_t          StructSize;
	const wchar_t  *FileName;
	void           *Buffer;
	size_t          BufferSize;
	OPERATION_MODES OpMode;
	void* Instance;
};

struct OpenAnalyseInfo
{
	size_t StructSize;
	struct AnalyseInfo* Info;
	HANDLE Handle;
};

struct OpenMacroInfo
{
	size_t StructSize;
	size_t Count;
	struct FarMacroValue *Values;
};

typedef unsigned __int64 FAROPENSHORTCUTFLAGS;
static const FAROPENSHORTCUTFLAGS
	FOSF_ACTIVE = 0x0000000000000001ULL,
	FOSF_NONE   = 0;

struct OpenShortcutInfo
{
	size_t StructSize;
	const wchar_t *HostFile;
	const wchar_t *ShortcutData;
	FAROPENSHORTCUTFLAGS Flags;
};

struct OpenCommandLineInfo
{
	size_t StructSize;
	const wchar_t *CommandLine;
};

enum OPENFROM
{
	OPEN_LEFTDISKMENU       = 0,
	OPEN_PLUGINSMENU        = 1,
	OPEN_FINDLIST           = 2,
	OPEN_SHORTCUT           = 3,
	OPEN_COMMANDLINE        = 4,
	OPEN_EDITOR             = 5,
	OPEN_VIEWER             = 6,
	OPEN_FILEPANEL          = 7,
	OPEN_DIALOG             = 8,
	OPEN_ANALYSE            = 9,
	OPEN_RIGHTDISKMENU      = 10,
	OPEN_FROMMACRO          = 11,
	OPEN_LUAMACRO           = 100,
};

enum MACROCALLTYPE
{
	MCT_MACROPARSE         = 0,
	MCT_LOADMACROS         = 1,
	MCT_ENUMMACROS         = 2,
	MCT_WRITEMACROS        = 3,
	MCT_GETMACRO           = 4,
	MCT_RECORDEDMACRO      = 5,
	MCT_DELMACRO           = 6,
	MCT_RUNSTARTMACRO      = 7,
	MCT_EXECSTRING         = 8,
	MCT_PANELSORT          = 9,
	MCT_GETCUSTOMSORTMODES = 10,
	MCT_ADDMACRO           = 11,
	MCT_KEYMACRO           = 12,
};

enum MACROPLUGINRETURNTYPE
{
	MPRT_NORMALFINISH  = 0,
	MPRT_ERRORFINISH   = 1,
	MPRT_ERRORPARSE    = 2,
	MPRT_KEYS          = 3,
	MPRT_PRINT         = 4,
	MPRT_PLUGINCALL    = 5,
	MPRT_PLUGINMENU    = 6,
	MPRT_PLUGINCONFIG  = 7,
	MPRT_PLUGINCOMMAND = 8,
	MPRT_USERMENU      = 9,
	MPRT_HASNOMACRO    = 10,
};

struct OpenMacroPluginInfo
{
	enum MACROCALLTYPE CallType;
	struct FarMacroCall *Data;
	struct MacroPluginReturn Ret;
};


enum FAR_EVENTS
{
	FE_CHANGEVIEWMODE   =0,
	FE_REDRAW           =1,
	FE_IDLE             =2,
	FE_CLOSE            =3,
	FE_BREAK            =4,
	FE_COMMAND          =5,

	FE_GOTFOCUS         =6,
	FE_KILLFOCUS        =7,
	FE_CHANGESORTPARAMS =8,
};

struct OpenInfo
{
	size_t StructSize;
	enum OPENFROM OpenFrom;
	const GUID* Guid;
	intptr_t Data;
	void* Instance;
};

struct SetDirectoryInfo
{
	size_t StructSize;
	HANDLE hPanel;
	const wchar_t *Dir;
	intptr_t Reserved;
	OPERATION_MODES OpMode;
	struct UserDataItem UserData;
	void* Instance;
};

struct SetFindListInfo
{
	size_t StructSize;
	HANDLE hPanel;
	const struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	void* Instance;
};

struct PutFilesInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	BOOL Move;
	const wchar_t *SrcPath;
	OPERATION_MODES OpMode;
	void* Instance;
};

struct ProcessHostFileInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	OPERATION_MODES OpMode;
	void* Instance;
};

struct MakeDirectoryInfo
{
	size_t StructSize;
	HANDLE hPanel;
	const wchar_t *Name;
	OPERATION_MODES OpMode;
	void* Instance;
};

struct CompareInfo
{
	size_t StructSize;
	HANDLE hPanel;
	const struct PluginPanelItem *Item1;
	const struct PluginPanelItem *Item2;
	enum OPENPANELINFO_SORTMODES Mode;
	void* Instance;
};

struct GetFindDataInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	OPERATION_MODES OpMode;
	void* Instance;
};


struct FreeFindDataInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	void* Instance;
};

struct GetFilesInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	BOOL Move;
	const wchar_t *DestPath;
	OPERATION_MODES OpMode;
	void* Instance;
};

struct DeleteFilesInfo
{
	size_t StructSize;
	HANDLE hPanel;
	struct PluginPanelItem *PanelItem;
	size_t ItemsNumber;
	OPERATION_MODES OpMode;
	void* Instance;
};

struct ProcessPanelInputInfo
{
	size_t StructSize;
	HANDLE hPanel;
	INPUT_RECORD Rec;
	void* Instance;
};

struct ProcessEditorInputInfo
{
	size_t StructSize;
	INPUT_RECORD Rec;
	void* Instance;
};

typedef unsigned __int64 PROCESSCONSOLEINPUT_FLAGS;
static const PROCESSCONSOLEINPUT_FLAGS
	PCIF_FROMMAIN = 0x0000000000000001ULL,
	PCIF_NONE     = 0;

struct ProcessConsoleInputInfo
{
	size_t StructSize;
	PROCESSCONSOLEINPUT_FLAGS Flags;
	INPUT_RECORD Rec;
	void* Instance;
};

struct ExitInfo
{
	size_t StructSize;
	void* Instance;
};

struct ProcessPanelEventInfo
{
	size_t StructSize;
	intptr_t Event;
	void* Param;
	HANDLE hPanel;
	void* Instance;
};

struct ProcessEditorEventInfo
{
	size_t StructSize;
	intptr_t Event;
	void* Param;
	intptr_t EditorID;
	void* Instance;
};

struct ProcessDialogEventInfo
{
	size_t StructSize;
	intptr_t Event;
	struct FarDialogEvent* Param;
	void* Instance;
};

struct ProcessSynchroEventInfo
{
	size_t StructSize;
	intptr_t Event;
	void* Param;
	void* Instance;
};

struct ProcessViewerEventInfo
{
	size_t StructSize;
	intptr_t Event;
	void* Param;
	intptr_t ViewerID;
	void* Instance;
};

struct ClosePanelInfo
{
	size_t StructSize;
	HANDLE hPanel;
	void* Instance;
};

struct CloseAnalyseInfo
{
	size_t StructSize;
	HANDLE Handle;
	void* Instance;
};

struct ConfigureInfo
{
	size_t StructSize;
	const GUID* Guid;
	void* Instance;
};

static const GUID FarGuid =
{0x00000000, 0x0000, 0x0000, {0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00}};

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef CONEMU_INTERNAL_PLUGIN_EXPORTS
// Exported Functions

	HANDLE   WINAPI AnalyseW(const struct AnalyseInfo *Info);
	void     WINAPI CloseAnalyseW(const struct CloseAnalyseInfo *Info);
	void     WINAPI ClosePanelW(const struct ClosePanelInfo *Info);
	intptr_t WINAPI CompareW(const struct CompareInfo *Info);
	intptr_t WINAPI ConfigureW(const struct ConfigureInfo *Info);
	intptr_t WINAPI DeleteFilesW(const struct DeleteFilesInfo *Info);
	void     WINAPI ExitFARW(const struct ExitInfo *Info);
	void     WINAPI FreeFindDataW(const struct FreeFindDataInfo *Info);
	intptr_t WINAPI GetFilesW(struct GetFilesInfo *Info);
	intptr_t WINAPI GetFindDataW(struct GetFindDataInfo *Info);
	void     WINAPI GetGlobalInfoW(struct GlobalInfo *Info);
	void     WINAPI GetOpenPanelInfoW(struct OpenPanelInfo *Info);
	void     WINAPI GetPluginInfoW(struct PluginInfo *Info);
	intptr_t WINAPI MakeDirectoryW(struct MakeDirectoryInfo *Info);
	HANDLE   WINAPI OpenW(const struct OpenInfo *Info);
	intptr_t WINAPI ProcessDialogEventW(const struct ProcessDialogEventInfo *Info);
	intptr_t WINAPI ProcessEditorEventW(const struct ProcessEditorEventInfo *Info);
	intptr_t WINAPI ProcessEditorInputW(const struct ProcessEditorInputInfo *Info);
	intptr_t WINAPI ProcessPanelEventW(const struct ProcessPanelEventInfo *Info);
	intptr_t WINAPI ProcessHostFileW(const struct ProcessHostFileInfo *Info);
	intptr_t WINAPI ProcessPanelInputW(const struct ProcessPanelInputInfo *Info);
	intptr_t WINAPI ProcessConsoleInputW(struct ProcessConsoleInputInfo *Info);
	intptr_t WINAPI ProcessSynchroEventW(const struct ProcessSynchroEventInfo *Info);
	intptr_t WINAPI ProcessViewerEventW(const struct ProcessViewerEventInfo *Info);
	intptr_t WINAPI PutFilesW(const struct PutFilesInfo *Info);
	intptr_t WINAPI SetDirectoryW(const struct SetDirectoryInfo *Info);
	intptr_t WINAPI SetFindListW(const struct SetFindListInfo *Info);
	void     WINAPI SetStartupInfoW(const struct PluginStartupInfo *Info);

#endif
#ifdef __cplusplus
};
#endif

#endif /* RC_INVOKED */

#endif /* __PLUGIN_HPP__ */
