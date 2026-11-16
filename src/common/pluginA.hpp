#ifndef __PLUGIN_HPP__
#define __PLUGIN_HPP__

/*
  plugin.hpp

  Plugin API for FAR Manager 1.75 build 2479

  Copyright (c) 1996-2000 Eugene Roshal
  Copyright (c) 2000-2008 FAR group
*/

#if defined(_MSC_VER)
#pragma message ("Using plugin.hpp for FAR Manager 1.75 build 2479")
#endif

#define FARMANAGERVERSION_MAJOR 1
#define FARMANAGERVERSION_MINOR 75
#define FARMANAGERVERSION_BUILD 2479

#ifndef RC_INVOKED

#define MAKEFARVERSION(major,minor,build) ( ((major)<<8) | (minor) | ((build)<<16))

#define FARMANAGERVERSION MAKEFARVERSION(FARMANAGERVERSION_MAJOR,FARMANAGERVERSION_MINOR,FARMANAGERVERSION_BUILD)

#if !defined(_INC_WINDOWS) && !defined(_WINDOWS_)
#if (defined(__GNUC__) || defined(_MSC_VER)) && !defined(_WIN64)
#if !defined(_WINCON_H) && !defined(_WINCON_)
#define _WINCON_H
#define _WINCON_ // to prevent including wincon.h
#if defined(_MSC_VER)
#pragma pack(push,2)
#else
#pragma pack(2)
#endif
#include<windows.h>
#if defined(_MSC_VER)
#pragma pack(pop)
#else
#pragma pack()
#endif
#undef _WINCON_
#undef  _WINCON_H

#if defined(_MSC_VER)
#pragma pack(push,8)
#else
#pragma pack(8)
#endif
#include<wincon.h>
#if defined(_MSC_VER)
#pragma pack(pop)
#else
#pragma pack()
#endif
#endif
#define _WINCON_
#else
#include<windows.h>
#endif
#endif

#if defined(__BORLANDC__)
#ifndef _WIN64
#pragma option -a2
#endif
#elif defined(__GNUC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1100)) || defined(__LCC__)
#ifndef _WIN64
#pragma pack(2)
#endif
#if defined(__LCC__)
#define _export __declspec(dllexport)
#endif
#else
#ifndef _WIN64
#pragma pack(push,2)
#endif
#if _MSC_VER
#ifdef _export
#undef _export
#endif
#define _export
#endif
#endif

#define NM 260

#undef DefDlgProc

#define FARMACRO_KEY_EVENT  (KEY_EVENT|0x8000)

// To ensure compatibility of plugin.hpp with compilers not supporting C++,
// you can #define _FAR_NO_NAMELESS_UNIONS. In this case, to access,
// for example, the Data field of the FarDialogItem structure
// you will need to use Data.Data, and the Selected field - Param.Selected
//#define _FAR_NO_NAMELESS_UNIONS

// To ensure correct structure packing the member PluginPanelItem.FindData
// has the type FAR_FIND_DATA, not WIN32_FIND_DATA (since version 1.71
// build 2250). The structure FAR_FIND_DATA has the same layout as
// WIN32_FIND_DATA, but since it is declared in this file, it is
// generated with correct 2-byte alignment.
// This #define is necessary to compile plugins that expect
// PluginPanelItem.FindData to be of WIN32_FIND_DATA type.
//#define _FAR_USE_WIN32_FIND_DATA

#ifndef _WINCON_
typedef struct _INPUT_RECORD INPUT_RECORD;
typedef struct _CHAR_INFO    CHAR_INFO;
#endif

enum FARMESSAGEFLAGS
{
	FMSG_WARNING             = 0x00000001,
	FMSG_ERRORTYPE           = 0x00000002,
	FMSG_KEEPBACKGROUND      = 0x00000004,
	FMSG_DOWN                = 0x00000008,
	FMSG_LEFTALIGN           = 0x00000010,

	FMSG_ALLINONE            = 0x00000020,

	FMSG_MB_OK               = 0x00010000,
	FMSG_MB_OKCANCEL         = 0x00020000,
	FMSG_MB_ABORTRETRYIGNORE = 0x00030000,
	FMSG_MB_YESNO            = 0x00040000,
	FMSG_MB_YESNOCANCEL      = 0x00050000,
	FMSG_MB_RETRYCANCEL      = 0x00060000,
};

typedef int (WINAPI *FARAPIMESSAGE)(
    INT_PTR PluginNumber,
    DWORD Flags,
    const char *HelpTopic,
    const char * const *Items,
    int ItemsNumber,
    int ButtonsNumber
);


enum DialogItemTypes
{
	DI_TEXT,
	DI_VTEXT,
	DI_SINGLEBOX,
	DI_DOUBLEBOX,
	DI_EDIT,
	DI_PSWEDIT,
	DI_FIXEDIT,
	DI_BUTTON,
	DI_CHECKBOX,
	DI_RADIOBUTTON,
	DI_COMBOBOX,
	DI_LISTBOX,

	DI_USERCONTROL=255,
};

enum FarDialogItemFlags
{
	DIF_COLORMASK             = 0x000000ffUL,
	DIF_SETCOLOR              = 0x00000100UL,
	DIF_BOXCOLOR              = 0x00000200UL,
	DIF_GROUP                 = 0x00000400UL,
	DIF_LEFTTEXT              = 0x00000800UL,
	DIF_MOVESELECT            = 0x00001000UL,
	DIF_SHOWAMPERSAND         = 0x00002000UL,
	DIF_CENTERGROUP           = 0x00004000UL,
	DIF_NOBRACKETS            = 0x00008000UL,
	DIF_MANUALADDHISTORY      = 0x00008000UL,
	DIF_SEPARATOR             = 0x00010000UL,
	DIF_VAREDIT               = 0x00010000UL,
	DIF_SEPARATOR2            = 0x00020000UL,
	DIF_EDITOR                = 0x00020000UL,
	DIF_LISTNOAMPERSAND       = 0x00020000UL,
	DIF_LISTNOBOX             = 0x00040000UL,
	DIF_HISTORY               = 0x00040000UL,
	DIF_BTNNOCLOSE            = 0x00040000UL,
	DIF_CENTERTEXT            = 0x00040000UL,
	DIF_NOTCVTUSERCONTROL     = 0x00040000UL,
	DIF_EDITEXPAND            = 0x00080000UL,
	DIF_DROPDOWNLIST          = 0x00100000UL,
	DIF_USELASTHISTORY        = 0x00200000UL,
	DIF_MASKEDIT              = 0x00400000UL,
	DIF_SELECTONENTRY         = 0x00800000UL,
	DIF_3STATE                = 0x00800000UL,
	DIF_LISTWRAPMODE          = 0x01000000UL,
	DIF_NOAUTOCOMPLETE        = 0x02000000UL,
	DIF_LISTAUTOHIGHLIGHT     = 0x02000000UL,
	DIF_LISTNOCLOSE           = 0x04000000UL,
	DIF_HIDDEN                = 0x10000000UL,
	DIF_READONLY              = 0x20000000UL,
	DIF_NOFOCUS               = 0x40000000UL,
	DIF_DISABLE               = 0x80000000UL,
};

enum FarMessagesProc
{
	DM_FIRST=0,
	DM_CLOSE,
	DM_ENABLE,
	DM_ENABLEREDRAW,
	DM_GETDLGDATA,
	DM_GETDLGITEM,
	DM_GETDLGRECT,
	DM_GETTEXT,
	DM_GETTEXTLENGTH,
	DM_KEY,
	DM_MOVEDIALOG,
	DM_SETDLGDATA,
	DM_SETDLGITEM,
	DM_SETFOCUS,
	DM_REDRAW,
	DM_SETREDRAW=DM_REDRAW,
	DM_SETTEXT,
	DM_SETMAXTEXTLENGTH,
	DM_SETTEXTLENGTH=DM_SETMAXTEXTLENGTH,
	DM_SHOWDIALOG,
	DM_GETFOCUS,
	DM_GETCURSORPOS,
	DM_SETCURSORPOS,
	DM_GETTEXTPTR,
	DM_SETTEXTPTR,
	DM_SHOWITEM,
	DM_ADDHISTORY,

	DM_GETCHECK,
	DM_SETCHECK,
	DM_SET3STATE,

	DM_LISTSORT,
	DM_LISTGETITEM,
	DM_LISTGETCURPOS,
	DM_LISTSETCURPOS,
	DM_LISTDELETE,
	DM_LISTADD,
	DM_LISTADDSTR,
	DM_LISTUPDATE,
	DM_LISTINSERT,
	DM_LISTFINDSTRING,
	DM_LISTINFO,
	DM_LISTGETDATA,
	DM_LISTSETDATA,
	DM_LISTSETTITLES,
	DM_LISTGETTITLES,

	DM_RESIZEDIALOG,
	DM_SETITEMPOSITION,

	DM_GETDROPDOWNOPENED,
	DM_SETDROPDOWNOPENED,

	DM_SETHISTORY,

	DM_GETITEMPOSITION,
	DM_SETMOUSEEVENTNOTIFY,

	DM_EDITUNCHANGEDFLAG,

	DM_GETITEMDATA,
	DM_SETITEMDATA,

	DM_LISTSET,
	DM_LISTSETMOUSEREACTION,

	DM_GETCURSORSIZE,
	DM_SETCURSORSIZE,

	DM_LISTGETDATASIZE,

	DM_GETSELECTION,
	DM_SETSELECTION,

	DN_LISTHOTKEY,

	DM_GETEDITPOSITION,
	DM_SETEDITPOSITION,

	DM_SETCOMBOBOXEVENT,
	DM_GETCOMBOBOXEVENT,

	DN_FIRST=0x1000,
	DN_BTNCLICK,
	DN_CTLCOLORDIALOG,
	DN_CTLCOLORDLGITEM,
	DN_CTLCOLORDLGLIST,
	DN_DRAWDIALOG,
	DN_DRAWDLGITEM,
	DN_EDITCHANGE,
	DN_ENTERIDLE,
	DN_GOTFOCUS,
	DN_HELP,
	DN_HOTKEY,
	DN_INITDIALOG,
	DN_KILLFOCUS,
	DN_LISTCHANGE,
	DN_MOUSECLICK,
	DN_DRAGGED,
	DN_RESIZECONSOLE,
	DN_MOUSEEVENT,
	DN_DRAWDIALOGDONE,

	DN_CLOSE=DM_CLOSE,
	DN_KEY=DM_KEY,

	DM_USER=0x4000,

};

enum FARCHECKEDSTATE
{
	BSTATE_UNCHECKED = 0,
	BSTATE_CHECKED   = 1,
	BSTATE_3STATE    = 2,
	BSTATE_TOGGLE    = 3,
};

enum FARLISTMOUSEREACTIONTYPE
{
	LMRT_ONLYFOCUS   = 0,
	LMRT_ALWAYS      = 1,
	LMRT_NEVER       = 2,
};

enum FARCOMBOBOXEVENTTYPE
{
	CBET_KEY         = 0x00000001,
	CBET_MOUSE       = 0x00000002,
};

enum LISTITEMFLAGS
{
	LIF_SELECTED           = 0x00010000UL,
	LIF_CHECKED            = 0x00020000UL,
	LIF_SEPARATOR          = 0x00040000UL,
	LIF_DISABLE            = 0x00080000UL,
	LIF_DELETEUSERDATA     = 0x80000000UL,
};

struct FarListItem
{
	DWORD Flags;
	char  Text[128];
	DWORD Reserved[3];
};

struct FarListUpdate
{
	int Index;
	struct FarListItem Item;
};

struct FarListInsert
{
	int Index;
	struct FarListItem Item;
};

struct FarListGetItem
{
	int ItemIndex;
	struct FarListItem Item;
};

struct FarListPos
{
	int SelectPos;
	int TopPos;
};

enum FARLISTFINDFLAGS
{
	LIFIND_EXACTMATCH = 0x00000001,
};

struct FarListFind
{
	int StartIndex;
	const char *Pattern;
	DWORD Flags;
	DWORD Reserved;
};

struct FarListDelete
{
	int StartIndex;
	int Count;
};

enum FARLISTINFOFLAGS
{
	LINFO_SHOWNOBOX             = 0x00000400,
	LINFO_AUTOHIGHLIGHT         = 0x00000800,
	LINFO_REVERSEHIGHLIGHT      = 0x00001000,
	LINFO_WRAPMODE              = 0x00008000,
	LINFO_SHOWAMPERSAND         = 0x00010000,
};

struct FarListInfo
{
	DWORD Flags;
	int ItemsNumber;
	int SelectPos;
	int TopPos;
	int MaxHeight;
	int MaxLength;
	DWORD Reserved[6];
};

struct FarListItemData
{
	int   Index;
	int   DataSize;
	void *Data;
	DWORD Reserved;
};

struct FarList
{
	int ItemsNumber;
	struct FarListItem *Items;
};

struct FarListTitles
{
	int   TitleLen;
	char *Title;
	int   BottomLen;
	char *Bottom;
};

struct FarListColors
{
	DWORD  Flags;
	DWORD  Reserved;
	int    ColorCount;
	LPBYTE Colors;
};

struct FarDialogItem
{
	int Type;
	int X1,Y1,X2,Y2;
	int Focus;
	union
	{
		DWORD_PTR Reserved;
		int Selected;
		const char *History;
		const char *Mask;
		struct FarList *ListItems;
		int  ListPos;
		CHAR_INFO *VBuf;
	}
#ifdef _FAR_NO_NAMELESS_UNIONS
	Param
#endif
	;
	DWORD Flags;
	int DefaultButton;
	union
	{
		char Data[512];
		struct
		{
			DWORD PtrFlags;
			int   PtrLength;
			char *PtrData;
			char  PtrTail[1];
		} Ptr;
	}
#ifdef _FAR_NO_NAMELESS_UNIONS
	Data
#endif
	;
};

struct FarDialogItemData
{
	int   PtrLength;
	char *PtrData;
};

struct FarDialogEvent
{
	HANDLE hDlg;
	int Msg;
	int Param1;
	LONG_PTR Param2;
	LONG_PTR Result;
};

struct OpenDlgPluginData
{
	int ItemNumber;
	HANDLE hDlg;
};

#define Dlg_RedrawDialog(Info,hDlg)            Info.SendDlgMessage(hDlg,DM_REDRAW,0,0)

#define Dlg_GetDlgData(Info,hDlg)              Info.SendDlgMessage(hDlg,DM_GETDLGDATA,0,0)
#define Dlg_SetDlgData(Info,hDlg,Data)         Info.SendDlgMessage(hDlg,DM_SETDLGDATA,0,(LONG_PTR)Data)

#define Dlg_GetDlgItemData(Info,hDlg,ID)       Info.SendDlgMessage(hDlg,DM_GETITEMDATA,0,0)
#define Dlg_SetDlgItemData(Info,hDlg,ID,Data)  Info.SendDlgMessage(hDlg,DM_SETITEMDATA,0,(LONG_PTR)Data)

#define DlgItem_GetFocus(Info,hDlg)            Info.SendDlgMessage(hDlg,DM_GETFOCUS,0,0)
#define DlgItem_SetFocus(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_SETFOCUS,ID,0)
#define DlgItem_Enable(Info,hDlg,ID)           Info.SendDlgMessage(hDlg,DM_ENABLE,ID,TRUE)
#define DlgItem_Disable(Info,hDlg,ID)          Info.SendDlgMessage(hDlg,DM_ENABLE,ID,FALSE)
#define DlgItem_IsEnable(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_ENABLE,ID,-1)
#define DlgItem_SetText(Info,hDlg,ID,Str)      Info.SendDlgMessage(hDlg,DM_SETTEXTPTR,ID,(LONG_PTR)Str)

#define DlgItem_GetCheck(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_GETCHECK,ID,0)
#define DlgItem_SetCheck(Info,hDlg,ID,State)   Info.SendDlgMessage(hDlg,DM_SETCHECK,ID,State)

#define DlgEdit_AddHistory(Info,hDlg,ID,Str)   Info.SendDlgMessage(hDlg,DM_ADDHISTORY,ID,(LONG_PTR)Str)

#define DlgList_AddString(Info,hDlg,ID,Str)    Info.SendDlgMessage(hDlg,DM_LISTADDSTR,ID,(LONG_PTR)Str)
#define DlgList_GetCurPos(Info,hDlg,ID)        Info.SendDlgMessage(hDlg,DM_LISTGETCURPOS,ID,0)
#define DlgList_SetCurPos(Info,hDlg,ID,NewPos) {struct FarListPos LPos={NewPos,-1};Info.SendDlgMessage(hDlg,DM_LISTSETCURPOS,ID,(LONG_PTR)&LPos);}
#define DlgList_ClearList(Info,hDlg,ID)        Info.SendDlgMessage(hDlg,DM_LISTDELETE,ID,0)
#define DlgList_DeleteItem(Info,hDlg,ID,Index) {struct FarListDelete FLDItem={Index,1}; Info.SendDlgMessage(hDlg,DM_LISTDELETE,ID,(LONG_PTR)&FLDItem);}
#define DlgList_SortUp(Info,hDlg,ID)           Info.SendDlgMessage(hDlg,DM_LISTSORT,ID,0)
#define DlgList_SortDown(Info,hDlg,ID)         Info.SendDlgMessage(hDlg,DM_LISTSORT,ID,1)
#define DlgList_GetItemData(Info,hDlg,ID,Index)          Info.SendDlgMessage(hDlg,DM_LISTGETDATA,ID,Index)
#define DlgList_SetItemStrAsData(Info,hDlg,ID,Index,Str) {struct FarListItemData FLID{Index,0,Str,0}; Info.SendDlgMessage(hDlg,DM_LISTSETDATA,ID,(LONG_PTR)&FLID);}

enum FARDIALOGFLAGS
{
	FDLG_WARNING             = 0x00000001,
	FDLG_SMALLDIALOG         = 0x00000002,
	FDLG_NODRAWSHADOW        = 0x00000004,
	FDLG_NODRAWPANEL         = 0x00000008,
};

typedef LONG_PTR(WINAPI *FARWINDOWPROC)(
    HANDLE   hDlg,
    int      Msg,
    int      Param1,
    LONG_PTR Param2
);

typedef LONG_PTR(WINAPI *FARAPISENDDLGMESSAGE)(
    HANDLE   hDlg,
    int      Msg,
    int      Param1,
    LONG_PTR Param2
);

typedef LONG_PTR(WINAPI *FARAPIDEFDLGPROC)(
    HANDLE   hDlg,
    int      Msg,
    int      Param1,
    LONG_PTR Param2
);

typedef int (WINAPI *FARAPIDIALOG)(
    INT_PTR               PluginNumber,
    int                   X1,
    int                   Y1,
    int                   X2,
    int                   Y2,
    const char           *HelpTopic,
    struct FarDialogItem *Item,
    int                   ItemsNumber
);

typedef int (WINAPI *FARAPIDIALOGEX)(
    INT_PTR               PluginNumber,
    int                   X1,
    int                   Y1,
    int                   X2,
    int                   Y2,
    const char           *HelpTopic,
    struct FarDialogItem *Item,
    int                   ItemsNumber,
    DWORD                 Reserved,
    DWORD                 Flags,
    FARWINDOWPROC         DlgProc,
    LONG_PTR              Param
);


struct FarMenuItem
{
	char Text[128];
	int  Selected;
	int  Checked;
	int  Separator;
};

enum MENUITEMFLAGS
{
	MIF_SELECTED   = 0x00010000UL,
	MIF_CHECKED    = 0x00020000UL,
	MIF_SEPARATOR  = 0x00040000UL,
	MIF_DISABLE    = 0x00080000UL,
	MIF_USETEXTPTR = 0x80000000UL,
};

struct FarMenuItemEx
{
	DWORD Flags;
	union
	{
		char  Text[128];
		const char *TextPtr;
	} Text;
	DWORD AccelKey;
	DWORD Reserved;
	DWORD_PTR UserData;
};

enum FARMENUFLAGS
{
	FMENU_SHOWAMPERSAND        = 0x00000001,
	FMENU_WRAPMODE             = 0x00000002,
	FMENU_AUTOHIGHLIGHT        = 0x00000004,
	FMENU_REVERSEAUTOHIGHLIGHT = 0x00000008,
	FMENU_USEEXT               = 0x00000020,
	FMENU_CHANGECONSOLETITLE   = 0x00000040,

	FMENU_TRUNCPATH            = 0x10000000,
	FMENU_TRUNCSTR             = 0x20000000,
	FMENU_TRUNCSTREND          = 0x30000000,
};

typedef int (WINAPI *FARAPIMENU)(
    INT_PTR                   PluginNumber,
    int                       X,
    int                       Y,
    int                       MaxHeight,
    DWORD                     Flags,
    const char               *Title,
    const char               *Bottom,
    const char               *HelpTopic,
    const int                *BreakKeys,
    int                      *BreakCode,
    const struct FarMenuItem *Item,
    int                       ItemsNumber
);


enum PLUGINPANELITEMFLAGS
{
	PPIF_PROCESSDESCR           = 0x80000000,
	PPIF_SELECTED               = 0x40000000,
	PPIF_USERDATA               = 0x20000000,
};

#ifndef _FAR_USE_WIN32_FIND_DATA

struct FAR_FIND_DATA
{
	DWORD    dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD    nFileSizeHigh;
	DWORD    nFileSizeLow;
	DWORD    dwReserved0;
	DWORD    dwReserved1;
	CHAR     cFileName[MAX_PATH];
	CHAR     cAlternateFileName[14];
};

#endif

struct PluginPanelItem
{
#ifndef _FAR_USE_WIN32_FIND_DATA
	struct FAR_FIND_DATA FindData;
#else
	WIN32_FIND_DATA      FindData;
#endif
	DWORD                PackSizeHigh;
	DWORD                PackSize;
	DWORD                Flags;
	DWORD                NumberOfLinks;
	char                *Description;
	char                *Owner;
	char               **CustomColumnData;
	int                  CustomColumnNumber;
	DWORD_PTR            UserData;
	DWORD                CRC32;
	DWORD_PTR            Reserved[2];
};

#if defined(__BORLANDC__)
#if sizeof(struct PluginPanelItem) != 366
#if defined(STRICT)
#error Incorrect alignment: sizeof(PluginPanelItem)!=366
#else
#pragma message Incorrect alignment: sizeof(PluginPanelItem)!=366
#endif
#endif
#endif

enum PANELINFOFLAGS
{
	PFLAGS_SHOWHIDDEN         = 0x00000001,
	PFLAGS_HIGHLIGHT          = 0x00000002,
	PFLAGS_REVERSESORTORDER   = 0x00000004,
	PFLAGS_USESORTGROUPS      = 0x00000008,
	PFLAGS_SELECTEDFIRST      = 0x00000010,
	PFLAGS_REALNAMES          = 0x00000020,
	PFLAGS_NUMERICSORT        = 0x00000040,
	PFLAGS_PANELLEFT          = 0x00000080,
	PFLAGS_PANELRIGHT         = 0x00000100,
};

enum PANELINFOTYPE
{
	PTYPE_FILEPANEL,
	PTYPE_TREEPANEL,
	PTYPE_QVIEWPANEL,
	PTYPE_INFOPANEL
};

struct PanelInfo
{
	int                     PanelType;
	int                     Plugin;
	RECT                    PanelRect;
	struct PluginPanelItem *PanelItems;
	int                     ItemsNumber;
	struct PluginPanelItem *SelectedItems;
	int                     SelectedItemsNumber;
	int                     CurrentItem;
	int                     TopPanelItem;
	int                     Visible;
	int                     Focus;
	int                     ViewMode;
	char                    ColumnTypes[80];
	char                    ColumnWidths[80];
	char                    CurDir[NM];
	int                     ShortNames;
	int                     SortMode;
	DWORD                   Flags;
	DWORD                   Reserved;
};


struct PanelRedrawInfo
{
	int CurrentItem;
	int TopPanelItem;
};

struct CmdLineSelect
{
	int SelStart;
	int SelEnd;
};

enum FILE_CONTROL_COMMANDS
{
	FCTL_CLOSEPLUGIN,
	FCTL_GETPANELINFO,
	FCTL_GETANOTHERPANELINFO,
	FCTL_UPDATEPANEL,
	FCTL_UPDATEANOTHERPANEL,
	FCTL_REDRAWPANEL,
	FCTL_REDRAWANOTHERPANEL,
	FCTL_SETANOTHERPANELDIR,
	FCTL_GETCMDLINE,
	FCTL_SETCMDLINE,
	FCTL_SETSELECTION,
	FCTL_SETANOTHERSELECTION,
	FCTL_SETVIEWMODE,
	FCTL_SETANOTHERVIEWMODE,
	FCTL_INSERTCMDLINE,
	FCTL_SETUSERSCREEN,
	FCTL_SETPANELDIR,
	FCTL_SETCMDLINEPOS,
	FCTL_GETCMDLINEPOS,
	FCTL_SETSORTMODE,
	FCTL_SETANOTHERSORTMODE,
	FCTL_SETSORTORDER,
	FCTL_SETANOTHERSORTORDER,
	FCTL_GETCMDLINESELECTEDTEXT,
	FCTL_SETCMDLINESELECTION,
	FCTL_GETCMDLINESELECTION,
	FCTL_GETPANELSHORTINFO,
	FCTL_GETANOTHERPANELSHORTINFO,
	FCTL_CHECKPANELSEXIST,
	FCTL_SETNUMERICSORT,
	FCTL_SETANOTHERNUMERICSORT,
	FCTL_GETUSERSCREEN,
};

typedef int (WINAPI *FARAPICONTROL)(
    HANDLE hPlugin,
    int    Command,
    void  *Param
);

typedef void (WINAPI *FARAPITEXT)(
    int         X,
    int         Y,
    int         Color,
    const char *Str
);

typedef HANDLE(WINAPI *FARAPISAVESCREEN)(int X1, int Y1, int X2, int Y2);

typedef void (WINAPI *FARAPIRESTORESCREEN)(HANDLE hScreen);


typedef int (WINAPI *FARAPIGETDIRLIST)(
    const char              *Dir,
    struct PluginPanelItem **pPanelItem,
    int                     *pItemsNumber
);

typedef int (WINAPI *FARAPIGETPLUGINDIRLIST)(
    INT_PTR                  PluginNumber,
    HANDLE                   hPlugin,
    const char              *Dir,
    struct PluginPanelItem **pPanelItem,
    int                     *pItemsNumber
);

typedef void (WINAPI *FARAPIFREEDIRLIST)(const struct PluginPanelItem *PanelItem);

enum VIEWER_FLAGS
{
	VF_NONMODAL              = 0x00000001,
	VF_DELETEONCLOSE         = 0x00000002,
	VF_ENABLE_F6             = 0x00000004,
	VF_DISABLEHISTORY        = 0x00000008,
	VF_IMMEDIATERETURN       = 0x00000100,
	VF_DELETEONLYFILEONCLOSE = 0x00000200,
};

typedef int (WINAPI *FARAPIVIEWER)(
    const char *FileName,
    const char *Title,
    int X1,
    int Y1,
    int X2,
    int Y2,
    DWORD Flags
);

enum EDITOR_FLAGS
{
	EF_NONMODAL              = 0x00000001,
	EF_CREATENEW             = 0x00000002,
	EF_ENABLE_F6             = 0x00000004,
	EF_DISABLEHISTORY        = 0x00000008,
	EF_DELETEONCLOSE         = 0x00000010,
	EF_IMMEDIATERETURN       = 0x00000100,
	EF_DELETEONLYFILEONCLOSE = 0x00000200,
};

enum EDITOR_EXITCODE
{
	EEC_OPEN_ERROR          = 0,
	EEC_MODIFIED            = 1,
	EEC_NOT_MODIFIED        = 2,
	EEC_LOADING_INTERRUPTED = 3,
};

typedef int (WINAPI *FARAPIEDITOR)(
    const char *FileName,
    const char *Title,
    int X1,
    int Y1,
    int X2,
    int Y2,
    DWORD Flags,
    int StartLine,
    int StartChar
);

typedef int (WINAPI *FARAPICMPNAME)(
    const char *Pattern,
    const char *String,
    int SkipPath
);


enum FARCHARTABLE_COMMAND
{
	FCT_DETECT=0x40000000,
};

struct CharTableSet
{
	unsigned char DecodeTable[256];
	unsigned char EncodeTable[256];
	unsigned char UpperTable[256];
	unsigned char LowerTable[256];
	char TableName[128];
};

typedef int (WINAPI *FARAPICHARTABLE)(
    int Command,
    char *Buffer,
    int BufferSize
);

typedef const char* (WINAPI *FARAPIGETMSG)(
    INT_PTR PluginNumber,
    int     MsgId
);


enum FarHelpFlags
{
	FHELP_NOSHOWERROR = 0x80000000,
	FHELP_SELFHELP    = 0x00000000,
	FHELP_FARHELP     = 0x00000001,
	FHELP_CUSTOMFILE  = 0x00000002,
	FHELP_CUSTOMPATH  = 0x00000004,
	FHELP_USECONTENTS = 0x40000000,
};

typedef BOOL (WINAPI *FARAPISHOWHELP)(
    const char *ModuleName,
    const char *Topic,
    DWORD       Flags
);

enum ADVANCED_CONTROL_COMMANDS
{
	ACTL_GETFARVERSION        = 0,
	ACTL_CONSOLEMODE          = 1,
	ACTL_GETSYSWORDDIV        = 2,
	ACTL_WAITKEY              = 3,
	ACTL_GETCOLOR             = 4,
	ACTL_GETARRAYCOLOR        = 5,
	ACTL_EJECTMEDIA           = 6,
	ACTL_KEYMACRO             = 7,
	ACTL_POSTKEYSEQUENCE      = 8,
	ACTL_GETWINDOWINFO        = 9,
	ACTL_GETWINDOWCOUNT       = 10,
	ACTL_SETCURRENTWINDOW     = 11,
	ACTL_COMMIT               = 12,
	ACTL_GETFARHWND           = 13,
	ACTL_GETSYSTEMSETTINGS    = 14,
	ACTL_GETPANELSETTINGS     = 15,
	ACTL_GETINTERFACESETTINGS = 16,
	ACTL_GETCONFIRMATIONS     = 17,
	ACTL_GETDESCSETTINGS      = 18,
	ACTL_SETARRAYCOLOR        = 19,
	ACTL_GETWCHARMODE         = 20,
	ACTL_GETPLUGINMAXREADDATA = 21,
	ACTL_GETDIALOGSETTINGS    = 22,
	ACTL_GETSHORTWINDOWINFO   = 23,
	ACTL_REDRAWALL            = 27,
};


enum FarSystemSettings
{
	FSS_CLEARROATTRIBUTE               = 0x00000001,
	FSS_DELETETORECYCLEBIN             = 0x00000002,
	FSS_USESYSTEMCOPYROUTINE           = 0x00000004,
	FSS_COPYFILESOPENEDFORWRITING      = 0x00000008,
	FSS_CREATEFOLDERSINUPPERCASE       = 0x00000010,
	FSS_SAVECOMMANDSHISTORY            = 0x00000020,
	FSS_SAVEFOLDERSHISTORY             = 0x00000040,
	FSS_SAVEVIEWANDEDITHISTORY         = 0x00000080,
	FSS_USEWINDOWSREGISTEREDTYPES      = 0x00000100,
	FSS_AUTOSAVESETUP                  = 0x00000200,
	FSS_SCANSYMLINK                    = 0x00000400,
};

enum FarPanelSettings
{
	FPS_SHOWHIDDENANDSYSTEMFILES       = 0x00000001,
	FPS_HIGHLIGHTFILES                 = 0x00000002,
	FPS_AUTOCHANGEFOLDER               = 0x00000004,
	FPS_SELECTFOLDERS                  = 0x00000008,
	FPS_ALLOWREVERSESORTMODES          = 0x00000010,
	FPS_SHOWCOLUMNTITLES               = 0x00000020,
	FPS_SHOWSTATUSLINE                 = 0x00000040,
	FPS_SHOWFILESTOTALINFORMATION      = 0x00000080,
	FPS_SHOWFREESIZE                   = 0x00000100,
	FPS_SHOWSCROLLBAR                  = 0x00000200,
	FPS_SHOWBACKGROUNDSCREENSNUMBER    = 0x00000400,
	FPS_SHOWSORTMODELETTER             = 0x00000800,
};

enum FarDialogSettings
{
	FDIS_HISTORYINDIALOGEDITCONTROLS    = 0x00000001,
	FDIS_PERSISTENTBLOCKSINEDITCONTROLS = 0x00000002,
	FDIS_AUTOCOMPLETEININPUTLINES       = 0x00000004,
	FDIS_BSDELETEUNCHANGEDTEXT          = 0x00000008,
	FDIS_DELREMOVESBLOCKS               = 0x00000010,
	FDIS_MOUSECLICKOUTSIDECLOSESDIALOG  = 0x00000020,
};

enum FarInterfaceSettings
{
	FIS_CLOCKINPANELS                  = 0x00000001,
	FIS_CLOCKINVIEWERANDEDITOR         = 0x00000002,
	FIS_MOUSE                          = 0x00000004,
	FIS_SHOWKEYBAR                     = 0x00000008,
	FIS_ALWAYSSHOWMENUBAR              = 0x00000010,
	FIS_USERIGHTALTASALTGR             = 0x00000080,
	FIS_SHOWTOTALCOPYPROGRESSINDICATOR = 0x00000100,
	FIS_SHOWCOPYINGTIMEINFO            = 0x00000200,
	FIS_USECTRLPGUPTOCHANGEDRIVE       = 0x00000800,
};

enum FarConfirmationsSettings
{
	FCS_COPYOVERWRITE                  = 0x00000001,
	FCS_MOVEOVERWRITE                  = 0x00000002,
	FCS_DRAGANDDROP                    = 0x00000004,
	FCS_DELETE                         = 0x00000008,
	FCS_DELETENONEMPTYFOLDERS          = 0x00000010,
	FCS_INTERRUPTOPERATION             = 0x00000020,
	FCS_DISCONNECTNETWORKDRIVE         = 0x00000040,
	FCS_RELOADEDITEDFILE               = 0x00000080,
	FCS_CLEARHISTORYLIST               = 0x00000100,
	FCS_EXIT                           = 0x00000200,
};

enum FarDescriptionSettings
{
	FDS_UPDATEALWAYS                   = 0x00000001,
	FDS_UPDATEIFDISPLAYED              = 0x00000002,
	FDS_SETHIDDEN                      = 0x00000004,
	FDS_UPDATEREADONLY                 = 0x00000008,
};

#define FAR_CONSOLE_GET_MODE       (-2)
#define FAR_CONSOLE_TRIGGER        (-1)
#define FAR_CONSOLE_SET_WINDOWED   (0)
#define FAR_CONSOLE_SET_FULLSCREEN (1)
#define FAR_CONSOLE_WINDOWED       (0)
#define FAR_CONSOLE_FULLSCREEN     (1)

enum FAREJECTMEDIAFLAGS
{
	EJECT_NO_MESSAGE                    = 0x00000001,
	EJECT_LOAD_MEDIA                    = 0x00000002,
};

struct ActlEjectMedia
{
	DWORD Letter;
	DWORD Flags;
};


enum FARKEYSEQUENCEFLAGS
{
	KSFLAGS_DISABLEOUTPUT       = 0x00000001,
	KSFLAGS_NOSENDKEYSTOPLUGINS = 0x00000002,
	KSFLAGS_REG_MULTI_SZ        = 0x00100000,
};

struct KeySequence
{
	DWORD  Flags;
	int    Count;
	DWORD *Sequence;
};

enum FARMACROCOMMAND
{
	MCMD_LOADALL           = 0,
	MCMD_SAVEALL           = 1,
	MCMD_POSTMACROSTRING   = 2,
	MCMD_GETSTATE          = 5,
};

enum FARMACROSTATE
{
	MACROSTATE_NOMACRO          =0,
	MACROSTATE_EXECUTING        =1,
	MACROSTATE_EXECUTING_COMMON =2,
	MACROSTATE_RECORDING        =3,
	MACROSTATE_RECORDING_COMMON =4,
};

struct ActlKeyMacro
{
	int Command;
	union
	{
		struct
		{
			char *SequenceText;
			DWORD Flags;
		} PlainText;
		DWORD_PTR Reserved[3];
	} Param;
};

enum FARCOLORFLAGS
{
	FCLR_REDRAW                 = 0x00000001,
};

struct FarSetColors
{
	DWORD Flags;
	int StartIndex;
	int ColorCount;
	LPBYTE Colors;
};

enum WINDOWINFO_TYPE
{
	WTYPE_PANELS=1,
	WTYPE_VIEWER,
	WTYPE_EDITOR,
	WTYPE_DIALOG,
	WTYPE_VMENU,
	WTYPE_HELP,
};

struct WindowInfo
{
	int  Pos;
	int  Type;
	int  Modified;
	int  Current;
	char TypeName[64];
	char Name[NM];
};

typedef INT_PTR(WINAPI *FARAPIADVCONTROL)(
    INT_PTR ModuleNumber,
    int     Command,
    void   *Param
);


enum VIEWER_CONTROL_COMMANDS
{
	VCTL_GETINFO,
	VCTL_QUIT,
	VCTL_REDRAW,
	VCTL_SETKEYBAR,
	VCTL_SETPOSITION,
	VCTL_SELECT,
	VCTL_SETMODE,
};

enum VIEWER_OPTIONS
{
	VOPT_SAVEFILEPOSITION=1,
	VOPT_AUTODETECTTABLE=2,
};

enum VIEWER_SETMODE_TYPES
{
	VSMT_HEX,
	VSMT_WRAP,
	VSMT_WORDWRAP,
};

enum VIEWER_SETMODEFLAGS_TYPES
{
	VSMFL_REDRAW    = 0x00000001,
};

struct ViewerSetMode
{
	int Type;
	union
	{
		int iParam;
		char *cParam;
	} Param;
	DWORD Flags;
	DWORD Reserved;
};

typedef union
{
	__int64 i64;
	struct
	{
		DWORD LowPart;
		LONG  HighPart;
	} Part;
} FARINT64;

struct ViewerSelect
{
	FARINT64 BlockStartPos;
	int      BlockLen;
};

enum VIEWER_SETPOS_FLAGS
{
	VSP_NOREDRAW    = 0x0001,
	VSP_PERCENT     = 0x0002,
	VSP_RELATIVE    = 0x0004,
	VSP_NORETNEWPOS = 0x0008,
};

struct ViewerSetPosition
{
	DWORD    Flags;
	FARINT64 StartPos;
	int      LeftPos;
};

struct ViewerMode
{
	int UseDecodeTable;
	int TableNum;
	int AnsiMode;
	int Unicode;
	int Wrap;
	int WordWrap;
	int Hex;
	DWORD Reserved[4];
};

struct ViewerInfo
{
	int               StructSize;
	int               ViewerID;
	const char       *FileName;
	FARINT64          FileSize;
	FARINT64          FilePos;
	int               WindowSizeX;
	int               WindowSizeY;
	DWORD             Options;
	int               TabSize;
	struct ViewerMode CurMode;
	int               LeftPos;
	DWORD             Reserved3;
};

typedef int (WINAPI *FARAPIVIEWERCONTROL)(
    int Command,
    void *Param
);

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
};

enum DIALOG_EVENTS
{
	DE_DLGPROCINIT    =0,
	DE_DEFDLGPROCINIT =1,
	DE_DLGPROCEND     =2,
};

#define EEREDRAW_ALL    (void*)0
#define EEREDRAW_CHANGE (void*)1
#define EEREDRAW_LINE   (void*)2

enum EDITOR_CONTROL_COMMANDS
{
	ECTL_GETSTRING,
	ECTL_SETSTRING,
	ECTL_INSERTSTRING,
	ECTL_DELETESTRING,
	ECTL_DELETECHAR,
	ECTL_INSERTTEXT,
	ECTL_GETINFO,
	ECTL_SETPOSITION,
	ECTL_SELECT,
	ECTL_REDRAW,
	ECTL_EDITORTOOEM,
	ECTL_OEMTOEDITOR,
	ECTL_TABTOREAL,
	ECTL_REALTOTAB,
	ECTL_EXPANDTABS,
	ECTL_SETTITLE,
	ECTL_READINPUT,
	ECTL_PROCESSINPUT,
	ECTL_ADDCOLOR,
	ECTL_GETCOLOR,
	ECTL_SAVEFILE,
	ECTL_QUIT,
	ECTL_SETKEYBAR,
	ECTL_PROCESSKEY,
	ECTL_SETPARAM,
	ECTL_GETBOOKMARKS,
	ECTL_TURNOFFMARKINGBLOCK,
	ECTL_DELETEBLOCK,
	ECTL_ADDSTACKBOOKMARK,
	ECTL_PREVSTACKBOOKMARK,
	ECTL_NEXTSTACKBOOKMARK,
	ECTL_CLEARSTACKBOOKMARKS,
	ECTL_DELETESTACKBOOKMARK,
	ECTL_GETSTACKBOOKMARKS,
};

enum EDITOR_SETPARAMETER_TYPES
{
	ESPT_TABSIZE,
	ESPT_EXPANDTABS,
	ESPT_AUTOINDENT,
	ESPT_CURSORBEYONDEOL,
	ESPT_CHARCODEBASE,
	ESPT_CHARTABLE,
	ESPT_SAVEFILEPOSITION,
	ESPT_LOCKMODE,
	ESPT_SETWORDDIV,
	ESPT_GETWORDDIV,
};



struct EditorSetParameter
{
	int Type;
	union
	{
		int iParam;
		char *cParam;
		DWORD Reserved1;
	} Param;
	DWORD Flags;
	DWORD Reserved2;
};

struct EditorGetString
{
	int StringNumber;
	const char *StringText;
	const char *StringEOL;
	int StringLength;
	int SelStart;
	int SelEnd;
};


struct EditorSetString
{
	int StringNumber;
	char *StringText;
	char *StringEOL;
	int StringLength;
};

enum EXPAND_TABS
{
	EXPAND_NOTABS,
	EXPAND_ALLTABS,
	EXPAND_NEWTABS
};


enum EDITOR_OPTIONS
{
	EOPT_EXPANDALLTABS     = 0x00000001,
	EOPT_PERSISTENTBLOCKS  = 0x00000002,
	EOPT_DELREMOVESBLOCKS  = 0x00000004,
	EOPT_AUTOINDENT        = 0x00000008,
	EOPT_SAVEFILEPOSITION  = 0x00000010,
	EOPT_AUTODETECTTABLE   = 0x00000020,
	EOPT_CURSORBEYONDEOL   = 0x00000040,
	EOPT_EXPANDONLYNEWTABS = 0x00000080,
};


enum EDITOR_BLOCK_TYPES
{
	BTYPE_NONE,
	BTYPE_STREAM,
	BTYPE_COLUMN
};

enum EDITOR_CURRENTSTATE
{
	ECSTATE_MODIFIED       = 0x00000001,
	ECSTATE_SAVED          = 0x00000002,
	ECSTATE_LOCKED         = 0x00000004,
};


struct EditorInfo
{
	int EditorID;
	const char *FileName;
	int WindowSizeX;
	int WindowSizeY;
	int TotalLines;
	int CurLine;
	int CurPos;
	int CurTabPos;
	int TopScreenLine;
	int LeftPos;
	int Overtype;
	int BlockType;
	int BlockStartLine;
	int AnsiMode;
	int TableNum;
	DWORD Options;
	int TabSize;
	int BookMarkCount;
	DWORD CurState;
	DWORD Reserved[6];
};

struct EditorBookMarks
{
	long *Line;
	long *Cursor;
	long *ScreenLine;
	long *LeftPos;
	DWORD Reserved[4];
};

struct EditorSetPosition
{
	int CurLine;
	int CurPos;
	int CurTabPos;
	int TopScreenLine;
	int LeftPos;
	int Overtype;
};


struct EditorSelect
{
	int BlockType;
	int BlockStartLine;
	int BlockStartPos;
	int BlockWidth;
	int BlockHeight;
};


struct EditorConvertText
{
	char *Text;
	int   TextLength;
};


struct EditorConvertPos
{
	int StringNumber;
	int SrcPos;
	int DestPos;
};


enum EDITORCOLORFLAGS
{
	ECF_TAB1 = 0x10000,
};

struct EditorColor
{
	int StringNumber;
	int ColorItem;
	int StartPos;
	int EndPos;
	int Color;
};

struct EditorSaveFile
{
	char  FileName[NM];
	char *FileEOL;
};

typedef int (WINAPI *FARAPIEDITORCONTROL)(
    int   Command,
    void *Param
);

enum INPUTBOXFLAGS
{
	FIB_ENABLEEMPTY      = 0x00000001,
	FIB_PASSWORD         = 0x00000002,
	FIB_EXPANDENV        = 0x00000004,
	FIB_NOUSELASTHISTORY = 0x00000008,
	FIB_BUTTONS          = 0x00000010,
	FIB_NOAMPERSAND      = 0x00000020,
};

typedef int (WINAPI *FARAPIINPUTBOX)(
    const char *Title,
    const char *SubTitle,
    const char *HistoryName,
    const char *SrcText,
    char *DestText,
    int   DestLength,
    const char *HelpTopic,
    DWORD Flags
);

// <C&C++>
typedef int (WINAPIV *FARSTDSPRINTF)(char *Buffer,const char *Format,...);
typedef int (WINAPIV *FARSTDSNPRINTF)(char *Buffer,size_t Sizebuf,const char *Format,...);
typedef int (WINAPIV *FARSTDSSCANF)(const char *Buffer, const char *Format,...);
// </C&C++>
typedef void (WINAPI *FARSTDQSORT)(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
typedef void (WINAPI *FARSTDQSORTEX)(void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *,void *userparam),void *userparam);
typedef void   *(WINAPI *FARSTDBSEARCH)(const void *key, const void *base, size_t nelem, size_t width, int (__cdecl *fcmp)(const void *, const void *));
typedef int (WINAPI *FARSTDGETFILEOWNER)(const char *Computer,const char *Name,char *Owner);
typedef int (WINAPI *FARSTDGETNUMBEROFLINKS)(const char *Name);
typedef int (WINAPI *FARSTDATOI)(const char *s);
typedef __int64(WINAPI *FARSTDATOI64)(const char *s);
typedef char   *(WINAPI *FARSTDITOA64)(__int64 value, char *string, int radix);
typedef char   *(WINAPI *FARSTDITOA)(int value, char *string, int radix);
typedef char   *(WINAPI *FARSTDLTRIM)(char *Str);
typedef char   *(WINAPI *FARSTDRTRIM)(char *Str);
typedef char   *(WINAPI *FARSTDTRIM)(char *Str);
typedef char   *(WINAPI *FARSTDTRUNCSTR)(char *Str,int MaxLength);
typedef char   *(WINAPI *FARSTDTRUNCPATHSTR)(char *Str,int MaxLength);
typedef char   *(WINAPI *FARSTDQUOTESPACEONLY)(char *Str);
typedef char*   (WINAPI *FARSTDPOINTTONAME)(const char *Path);
typedef void (WINAPI *FARSTDGETPATHROOT)(const char *Path,char *Root);
typedef BOOL (WINAPI *FARSTDADDENDSLASH)(char *Path);
typedef int (WINAPI *FARSTDCOPYTOCLIPBOARD)(const char *Data);
typedef char   *(WINAPI *FARSTDPASTEFROMCLIPBOARD)(void);
typedef int (WINAPI *FARSTDINPUTRECORDTOKEY)(const INPUT_RECORD *r);
typedef int (WINAPI *FARSTDLOCALISLOWER)(unsigned Ch);
typedef int (WINAPI *FARSTDLOCALISUPPER)(unsigned Ch);
typedef int (WINAPI *FARSTDLOCALISALPHA)(unsigned Ch);
typedef int (WINAPI *FARSTDLOCALISALPHANUM)(unsigned Ch);
typedef unsigned(WINAPI *FARSTDLOCALUPPER)(unsigned LowerChar);
typedef unsigned(WINAPI *FARSTDLOCALLOWER)(unsigned UpperChar);
typedef void (WINAPI *FARSTDLOCALUPPERBUF)(char *Buf,int Length);
typedef void (WINAPI *FARSTDLOCALLOWERBUF)(char *Buf,int Length);
typedef void (WINAPI *FARSTDLOCALSTRUPR)(char *s1);
typedef void (WINAPI *FARSTDLOCALSTRLWR)(char *s1);
typedef int (WINAPI *FARSTDLOCALSTRICMP)(const char *s1,const char *s2);
typedef int (WINAPI *FARSTDLOCALSTRNICMP)(const char *s1,const char *s2,int n);

enum PROCESSNAME_FLAGS
{
	PN_CMPNAME      = 0x00000000UL,
	PN_CMPNAMELIST  = 0x00001000UL,
	PN_GENERATENAME = 0x00002000UL,
	PN_SKIPPATH     = 0x00100000UL,
};

typedef int (WINAPI *FARSTDPROCESSNAME)(const char *param1, char *param2, DWORD flags);

typedef void (WINAPI *FARSTDUNQUOTE)(char *Str);

typedef DWORD (WINAPI *FARSTDEXPANDENVIRONMENTSTR)(
    const char *src,
    char *dst,
    size_t size
);

enum XLATMODE
{
	XLAT_SWITCHKEYBLAYOUT  = 0x00000001UL,
	XLAT_SWITCHKEYBBEEP    = 0x00000002UL,
};

typedef char*   (WINAPI *FARSTDXLAT)(char *Line,int StartPos,int EndPos,const struct CharTableSet *TableSet,DWORD Flags);
typedef BOOL (WINAPI *FARSTDKEYTOKEYNAME)(int Key,char *KeyText,int Size);
typedef int (WINAPI *FARSTDKEYNAMETOKEY)(const char *Name);

typedef int (WINAPI *FRSUSERFUNC)(
    const WIN32_FIND_DATA *FData,
    const char *FullName,
    void *Param
);

enum FRSMODE
{
	FRS_RETUPDIR             = 0x01,
	FRS_RECUR                = 0x02,
	FRS_SCANSYMLINK          = 0x04,
};

typedef void (WINAPI *FARSTDRECURSIVESEARCH)(const char *InitDir,const char *Mask,FRSUSERFUNC Func,DWORD Flags,void *Param);
typedef char*   (WINAPI *FARSTDMKTEMP)(char *Dest,const char *Prefix);
typedef void (WINAPI *FARSTDDELETEBUFFER)(void *Buffer);

enum MKLINKOP
{
	FLINK_HARDLINK         = 1,
	FLINK_JUNCTION         = 2,
	FLINK_SYMLINK          = FLINK_JUNCTION,
	FLINK_VOLMOUNT         = 3,
	FLINK_SYMLINKFILE      = 4,
	FLINK_SYMLINKDIR       = 5,

	FLINK_SHOWERRMSG       = 0x10000,
	FLINK_DONOTUPDATEPANEL = 0x20000,
};
typedef int (WINAPI *FARSTDMKLINK)(const char *Src,const char *Dest,DWORD Flags);
typedef int (WINAPI *FARCONVERTNAMETOREAL)(const char *Src,char *Dest, int DestSize);
typedef int (WINAPI *FARGETREPARSEPOINTINFO)(const char *Src,char *Dest,int DestSize);

typedef struct FarStandardFunctions
{
	int StructSize;

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
	FARSTDQSORTEX              qsortex;

	// <C&C++>
	FARSTDSNPRINTF             snprintf;
	// </C&C++>

	DWORD_PTR                  Reserved[8];

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
	FARSTDEXPANDENVIRONMENTSTR ExpandEnvironmentStr;
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
	FARSTDKEYTOKEYNAME         FarKeyToName;
	FARSTDKEYNAMETOKEY         FarNameToKey;
	FARSTDINPUTRECORDTOKEY     FarInputRecordToKey;
	FARSTDXLAT                 XLat;
	FARSTDGETFILEOWNER         GetFileOwner;
	FARSTDGETNUMBEROFLINKS     GetNumberOfLinks;
	FARSTDRECURSIVESEARCH      FarRecursiveSearch;
	FARSTDMKTEMP               MkTemp;
	FARSTDDELETEBUFFER         DeleteBuffer;
	FARSTDPROCESSNAME          ProcessName;
	FARSTDMKLINK               MkLink;
	FARCONVERTNAMETOREAL       ConvertNameToReal;
	FARGETREPARSEPOINTINFO     GetReparsePointInfo;
} FARSTANDARDFUNCTIONS;

struct PluginStartupInfo
{
	int                    StructSize;
	char                   ModuleName[NM];
	INT_PTR                ModuleNumber;
	const char            *RootKey;

	FARAPIMENU             Menu;
	FARAPIDIALOG           Dialog;
	FARAPIMESSAGE          Message;
	FARAPIGETMSG           GetMsg;
	FARAPICONTROL          Control;
	FARAPISAVESCREEN       SaveScreen;
	FARAPIRESTORESCREEN    RestoreScreen;
	FARAPIGETDIRLIST       GetDirList;
	FARAPIGETPLUGINDIRLIST GetPluginDirList;
	FARAPIFREEDIRLIST      FreeDirList;
	FARAPIVIEWER           Viewer;
	FARAPIEDITOR           Editor;
	FARAPICMPNAME          CmpName;
	FARAPICHARTABLE        CharTable;
	FARAPITEXT             Text;
	FARAPIEDITORCONTROL    EditorControl;

	FARSTANDARDFUNCTIONS  *FSF;

	FARAPISHOWHELP         ShowHelp;
	FARAPIADVCONTROL       AdvControl;
	FARAPIINPUTBOX         InputBox;
	FARAPIDIALOGEX         DialogEx;
	FARAPISENDDLGMESSAGE   SendDlgMessage;
	FARAPIDEFDLGPROC       DefDlgProc;
	DWORD_PTR              Reserved;
	FARAPIVIEWERCONTROL    ViewerControl;
};


enum PLUGIN_FLAGS
{
	PF_PRELOAD        = 0x0001,
	PF_DISABLEPANELS  = 0x0002,
	PF_EDITOR         = 0x0004,
	PF_VIEWER         = 0x0008,
	PF_FULLCMDLINE    = 0x0010,
	PF_DIALOG         = 0x0020,
};


struct PluginInfo
{
	int StructSize;
	DWORD Flags;
	const char * const *DiskMenuStrings;
	int *DiskMenuNumbers;
	int DiskMenuStringsNumber;
	const char * const *PluginMenuStrings;
	int PluginMenuStringsNumber;
	const char * const *PluginConfigStrings;
	int PluginConfigStringsNumber;
	const char *CommandPrefix;
	DWORD Reserved;
};


struct InfoPanelLine
{
	char Text[80];
	char Data[80];
	int  Separator;
};

struct PanelMode
{
	char  *ColumnTypes;
	char  *ColumnWidths;
	char **ColumnTitles;
	int    FullScreen;
	int    DetailedStatus;
	int    AlignExtensions;
	int    CaseConversion;
	char  *StatusColumnTypes;
	char  *StatusColumnWidths;
	DWORD  Reserved[2];
};


enum OPENPLUGININFO_FLAGS
{
	OPIF_USEFILTER               = 0x00000001,
	OPIF_USESORTGROUPS           = 0x00000002,
	OPIF_USEHIGHLIGHTING         = 0x00000004,
	OPIF_ADDDOTS                 = 0x00000008,
	OPIF_RAWSELECTION            = 0x00000010,
	OPIF_REALNAMES               = 0x00000020,
	OPIF_SHOWNAMESONLY           = 0x00000040,
	OPIF_SHOWRIGHTALIGNNAMES     = 0x00000080,
	OPIF_SHOWPRESERVECASE        = 0x00000100,
	OPIF_FINDFOLDERS             = 0x00000200,
	OPIF_COMPAREFATTIME          = 0x00000400,
	OPIF_EXTERNALGET             = 0x00000800,
	OPIF_EXTERNALPUT             = 0x00001000,
	OPIF_EXTERNALDELETE          = 0x00002000,
	OPIF_EXTERNALMKDIR           = 0x00004000,
	OPIF_USEATTRHIGHLIGHTING     = 0x00008000,
};


enum OPENPLUGININFO_SORTMODES
{
	SM_DEFAULT,
	SM_UNSORTED,
	SM_NAME,
	SM_EXT,
	SM_MTIME,
	SM_CTIME,
	SM_ATIME,
	SM_SIZE,
	SM_DESCR,
	SM_OWNER,
	SM_COMPRESSEDSIZE,
	SM_NUMLINKS
};


struct KeyBarTitles
{
	char *Titles[12];
	char *CtrlTitles[12];
	char *AltTitles[12];
	char *ShiftTitles[12];

	char *CtrlShiftTitles[12];
	char *AltShiftTitles[12];
	char *CtrlAltTitles[12];
};


enum OPERATION_MODES
{
	OPM_SILENT     =0x0001,
	OPM_FIND       =0x0002,
	OPM_VIEW       =0x0004,
	OPM_EDIT       =0x0008,
	OPM_TOPLEVEL   =0x0010,
	OPM_DESCR      =0x0020,
	OPM_QUICKVIEW  =0x0040,
};

#define MAXSIZE_SHORTCUTDATA  8192

struct OpenPluginInfo
{
	int                   StructSize;
	DWORD                 Flags;
	const char           *HostFile;
	const char           *CurDir;
	const char           *Format;
	const char           *PanelTitle;
	const struct InfoPanelLine *InfoLines;
	int                   InfoLinesNumber;
	const char * const   *DescrFiles;
	int                   DescrFilesNumber;
	const struct PanelMode *PanelModesArray;
	int                   PanelModesNumber;
	int                   StartPanelMode;
	int                   StartSortMode;
	int                   StartSortOrder;
	const struct KeyBarTitles *KeyBar;
	const char           *ShortcutData;
	long                  Reserverd;
};

enum OPENPLUGIN_OPENFROM
{
	OPEN_DISKMENU     = 0,
	OPEN_PLUGINSMENU  = 1,
	OPEN_FINDLIST     = 2,
	OPEN_SHORTCUT     = 3,
	OPEN_COMMANDLINE  = 4,
	OPEN_EDITOR       = 5,
	OPEN_VIEWER       = 6,
	OPEN_DIALOG       = 8,
};

enum FAR_PKF_FLAGS
{
	PKF_CONTROL     = 0x00000001,
	PKF_ALT         = 0x00000002,
	PKF_SHIFT       = 0x00000004,
	PKF_PREPROCESS  = 0x00080000, // for "Key", function ProcessKey()
};

enum FAR_EVENTS
{
	FE_CHANGEVIEWMODE =0,
	FE_REDRAW         =1,
	FE_IDLE           =2,
	FE_CLOSE          =3,
	FE_BREAK          =4,
	FE_COMMAND        =5,

	FE_GOTFOCUS       =6,
	FE_KILLFOCUS      =7,
};


#if defined(__BORLANDC__) || defined(_MSC_VER) || defined(__GNUC__) || defined(__WATCOMC__)
#ifdef __cplusplus
extern "C" {
#endif
#ifndef CONEMU_INTERNAL_PLUGIN_EXPORTS
// Exported Functions

	void   WINAPI _export ClosePlugin(HANDLE hPlugin);
	int    WINAPI _export Compare(HANDLE hPlugin,const struct PluginPanelItem *Item1,const struct PluginPanelItem *Item2,unsigned int Mode);
	int    WINAPI _export Configure(int ItemNumber);
	int    WINAPI _export DeleteFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	void   WINAPI _export ExitFAR(void);
	void   WINAPI _export FreeFindData(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber);
	void   WINAPI _export FreeVirtualFindData(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber);
	int    WINAPI _export GetFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,char *DestPath,int OpMode);
	int    WINAPI _export GetFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,int OpMode);
	int    WINAPI _export GetMinFarVersion(void);
	void   WINAPI _export GetOpenPluginInfo(HANDLE hPlugin,struct OpenPluginInfo *Info);
	void   WINAPI _export GetPluginInfo(struct PluginInfo *Info);
	int    WINAPI _export GetVirtualFindData(HANDLE hPlugin,struct PluginPanelItem **pPanelItem,int *pItemsNumber,const char *Path);
	int    WINAPI _export MakeDirectory(HANDLE hPlugin,char *Name,int OpMode);
	HANDLE WINAPI _export OpenFilePlugin(char *Name,const unsigned char *Data,int DataSize);
	HANDLE WINAPI _export OpenPlugin(int OpenFrom,INT_PTR Item);
	int    WINAPI _export ProcessDialogEvent(int Event,void *Param);
	int    WINAPI _export ProcessEditorEvent(int Event,void *Param);
	int    WINAPI _export ProcessEditorInput(const INPUT_RECORD *Rec);
	int    WINAPI _export ProcessEvent(HANDLE hPlugin,int Event,void *Param);
	int    WINAPI _export ProcessHostFile(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int OpMode);
	int    WINAPI _export ProcessKey(HANDLE hPlugin,int Key,unsigned int ControlState);
	int    WINAPI _export ProcessViewerEvent(int Event,void *Param);
	int    WINAPI _export PutFiles(HANDLE hPlugin,struct PluginPanelItem *PanelItem,int ItemsNumber,int Move,int OpMode);
	int    WINAPI _export SetDirectory(HANDLE hPlugin,const char *Dir,int OpMode);
	int    WINAPI _export SetFindList(HANDLE hPlugin,const struct PluginPanelItem *PanelItem,int ItemsNumber);
	void   WINAPI _export SetStartupInfo(const struct PluginStartupInfo *Info);

#endif
#ifdef __cplusplus
};
#endif
#endif

#ifndef _WIN64
#if defined(__BORLANDC__)
#pragma option -a.
#elif defined(__GNUC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1100)) || defined(__LCC__)
#pragma pack()
#else
#pragma pack(pop)
#endif
#endif

#endif /* RC_INVOKED */

#endif /* __PLUGIN_HPP__ */
