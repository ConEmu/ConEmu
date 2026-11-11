
/*
Copyright (c) 2018-present Maximus5
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

#include  <stdexcept>
#include "../common/defines.h"
#include <deque>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <stdexcept>
#include "../common/CEStr.h"
#include "../common/ConsoleMixAttr.h"
#include "../common/MArray.h"
/// CharAttr definition
#include "../common/RgnDetect.h"


namespace condata
{

class console_error : public std::logic_error
{
public:
	console_error(const char* message);
};

#include <pshpack2.h>

/// Used for both coordinates and sizes
struct Coord
{
	/// 0-based X coordinate
	unsigned x = 0;
	/// < 0 - backscroll buffer row indexes
	/// >=0 - 0-based index of workspace row
	int y = 0;
};

/// Colors, font type, outlines, etc.
struct Attribute
{
	/// Legacy attribute type definition for WinAPI calls
	union LegacyAttributes
	{
		unsigned short attr; /// RAW value for WinAPI
		struct
		{
			unsigned foreIdx : 4; /// 16-bit color palette
			unsigned backIdx : 4; /// 16-bit color palette
			unsigned _flags  : 8; /// DON'T ACCESS IT DIRECTLY!!! comes from COMMON_LVB_REVERSE_VIDEO, COMMON_LVB_UNDERSCORE etc.
		};
	};
	/// Attributes ready for WinAPI calls
	LegacyAttributes attr;

	/// Special flags not covered by attr.flags
	enum Flags : unsigned
	{
		/// No special flags
		fDefault = 0,

		/// Use Bold font
		fBold = 0x00000001,
		/// Use Italic font
		fItalic = 0x00000002,
		// #condata Not supported yet, reserved
		fCrossed = 0x00000004,
		// #condata Not supported yet, reserved
		fUnderlined = 0x00000008,

		// #condata crForeColor contains xterm-256 palette index (unused yet)
		fFore8b = 0x00000100,
		/// crForeColor contains RGB value
		fFore24b = 0x00000200,
		// #condata crBackColor contains xterm-256 palette index (unused yet)
		fBack8b = 0x00000400,
		/// crBackColor contains RGB value
		fBack24b = 0x00000800,

		// #condata Not supported yet, reserved
		fReversed = 0x00001000,

		/// row was '`n'-terminated
		fWasLF = 0x00010000,
	};
	/// Enum of Flags
	unsigned flags;

	/// Foreground color from xTerm256-color palette if (flags&fFore8b), or RGB if (flags&fFore24b)
	unsigned crForeColor; // only 24 bits are used
	/// Background color from xTerm256-color palette if (flags&fBack8b), or RGB if (flags&fBack24b)
	unsigned crBackColor; // only 24 bits are used
	unsigned hyperlinkId = 0;
	unsigned graphemeId = 0;

	/// Helper functions to access flags
	bool isBold() const;
	bool isItalic() const;
	bool isUnderscore() const;
	bool isReverse() const;
	bool isFore4b() const;
	bool isFore8b() const;
	bool isFore24b() const;
	bool isBack4b() const;
	bool isBack8b() const;
	bool isBack24b() const;

	/// Comparison operators
	bool operator==(const Attribute& a) const;
	bool operator!=(const Attribute& a) const;

}; // gDisplayParm = {};

/// Replace for CHAR_INFO
struct CharInfo
{
	/// Only Unicode symbols
	wchar_t chr;
	/// Colors, fonts, etc.
	Attribute attr;
};

#include <poppack.h>

/// Representation of the row data
class Row
{
private:
	struct RowColor
	{
		//// Number of the first cell where attr is applied
		unsigned  cell = 0;
		//// true-color properties of the cell
		Attribute attr = {};
	};
	using RowColors = std::vector<RowColor, MArrayAllocator<RowColor>>; // MArray<RowColor>;
	using RowChars = std::basic_string<wchar_t, std::char_traits<wchar_t>, MArrayAllocator<wchar_t>>; // MArray<wchar_t>;

public:
	Row(const Attribute& attr = {});
	~Row();

	Row(const Row& src);
	Row& operator=(const Row& src);

	Row(Row&& src);
	Row& operator=(Row&& src);

	/// all cell coordinates are 0-based

	void Write(unsigned cell, const Attribute& attr, const wchar_t* text, unsigned count);
	void Write(unsigned cell, const Attribute& attr, const wchar_t chr, unsigned count);
	void Insert(unsigned cell, const Attribute& attr, const wchar_t chr, unsigned count);
	void Delete(unsigned cell, unsigned count);
	void SetCellAttr(unsigned cell, const Attribute& attr, unsigned count);

	/// Clear row, set attributes for *next* write operation
	void Reset(const Attribute& attr);

	/// If shell emits its current directory - remember it in the current row
	/// @param cwd - current working directory; it may be either in Unix or Windows format, store unchanged
	void SetWorkingDirectory(const wchar_t* cwd);
	/// Returns previously stored working directory
	/// @result returns *nullptr* if directory was not set
	const wchar_t* GetWorkingDirectory() const;

	/// Remember where '`r' was emitted
	void SetCR(int pos);
	/// Remember where '`n' was emitted
	void SetLF(int pos);
	/// Check if the '`n' was written explicitly
	/// @result *true* if '`n'-terminated string was emitted here
	///         *false* if there were no '`n' emitted to the line
	bool HasLineFeed() const;
	bool SoftWrapped = false;
	unsigned SoftWrapLength = 0;

	/// Function will never return '\0'
	wchar_t GetCellChar(unsigned cell) const;
	/// Returns cell attributes
	/// @param  cell - 0-based index
	/// @param  attr - [OUT] cell attributes
	/// @param  hint - previous result of *GetCellAttr*, used to iterate cell search
	/// @result index in the ConsoleRow internal array, may be used with *hint* to iterate search
	unsigned GetCellAttr(unsigned cell, Attribute& attr, unsigned hint = 0) const;
	unsigned GetLength() const;

	/// Internal validation
	void Validate() const;

	/// Ensures that Row contains enough space to write the *cell*
	/// @result current or new (if modified) GetLength()
	unsigned SetReqSize(unsigned cell);

	/// Reserves enough space in m_Text to write *len* cells
	/// It doesn't do actual resize/append of text
	void Reserve(unsigned len);

	/// Returns ASCII-Z string, throw if m_Text is invalid
	const wchar_t* GetText() const;

protected:
	/// *protected* helper to maintain both wchar_t and wchar_t[]
	void Write(unsigned cell, const Attribute& attr, std::pair<const wchar_t*,wchar_t> txt, unsigned count);
	/// *protected* helper to reset CR/LF saved positions
	void ResetCRLF();
	void ClearWideOutside(unsigned cell, unsigned count, const Attribute& replacement);

protected:
	// Reserved for synchronization with RealConsole
	WORD m_RowID = 0;
	// Where '\r' was emitted
	int m_WasCR = -1;
	// Where '\n' was emitted
	int m_WasLF = -1;
	// Simple string of the row
	RowChars m_Text;
	// #condata Optimize SetCellAttr to use explicit m_Attr0 instead of vector
	RowColors m_Colors;
	// If console app informed us about its current working directory
	CEStr m_CWD;
};


class iRow
{
public:
	const unsigned index;
	iRow(unsigned _index, Row* _row);
	Row* operator->() const;
	Row& operator*() const;
protected:
	Row* row;
};


/// Main class to manipulate console surface
class Table
{
public:
	using BellCallback = void(*)(void*);

	struct Region
	{
		unsigned Top, Bottom;
	};

	using deque_type = std::deque<std::shared_ptr<Row>, MArrayAllocator<std::shared_ptr<Row>>>;
	using vector_type = std::vector<std::shared_ptr<Row>, MArrayAllocator<std::shared_ptr<Row>>>;

public:
	Table(const Attribute& _attr = {7}, const Coord& _size = {80, 25}, const int _backscrollMax = 1<<20, BellCallback _bellCallback = nullptr);
	~Table();

	Table(const Table&) = delete;
	Table(Table&&) = delete;

	// To simplify our interface, there are no functions which do write without cursor movement
	// So the cursor position always explicitly defines next write operation
	// The cursor can't be moved to *backscroll* area, only workspace is accessible for modifications

	/// Visual dimensions of working area
	Coord GetSize() const;
	/// Change visual dimensions of working area
	void Resize(const Coord newSize);
	void SetReflow(bool enabled) { m_Reflow = enabled; if (enabled) m_ExtendedUnicode = true; }
	void SetUnicode(bool enabled) { m_ExtendedUnicode = enabled; }
	const std::map<unsigned, std::wstring>& GetGraphemes() const { return m_Graphemes; }
	/// Return count of rows went to backscroll buffer (console history)
	unsigned GetBackscroll() const;
	/// Limit maximum count of backscroll rows
	/// @param backscrollMax = -1 means unlimited
	void SetBackscrollMax(const int backscrollMax);

	/// Set attributes for *next* write operation
	void SetAttr(const Attribute& attr);
	/// Just returns *current* attributes for next write operations
	Attribute GetAttr() const;

	/// Clear working area and backscroll, reset all options to default, set attributes for *next* write operation
	/// This action executed on `ESC c` sequence
	void Reset(const Attribute& attr);

	/// Clear scrollback buffer entirely
	/// This action executed on `ESC [ 3 J`
	void ClearBackscroll();

	/// Clear scrollback buffer entirely
	/// This action executed on `ESC [ 2 J`
	void ClearWorkspace();

	/// If size was set to {0,0} - absolutely unexpected
	bool IsEmpty() const;

	/// Moves cursor to certain location on workspace
	/// @param  cursor - passed by value intentionally (m_Cursor is passed as argument often)
	void SetCursor(const Coord cursor);
	/// Returns current cursor position in workspace coordinates
	Coord GetCursor() const;

	/// Set scrolling region of viewport if *setRegion* is *true*
	/// *top* and *bottom* are 0-based row indexes
	/// If *top* is greater than 0, scrolled lines are completely erased,
	/// so they don't go into m_Backscroll
	void SetScrollRegion(bool setRegion = false, unsigned top = 0, unsigned bottom = 0);

	/// Return top/bottom rows where cursor may be located and text printed
	Region GetScrollRegion() const;

	/// Emit CR automatically after single LF
	void SetAutoCRLF(bool autoCRLF);
	bool OriginMode = false;
	bool InsertMode = false;
	/// Defines behavior when character is written at the last cell of the row
	/// If *true* (enabled by "ESC [ ? 7 h") we may advance cursor and create new line (perhaps on next write if *BeyondEOL*)
	/// If *false* (enabled by "ESC [ ? 7 l") cursor stays at *m_Size.x-1* indefinitely
	void SetAutoWrap(bool autoWrap, int wrapAt = -1);
	/// Option is ignored if *AutoWrap* is *false*, cursor stays at *m_Size.x-1* indefinitely
	/// If *true* cursor may stay "after" EOL until next write or cursor move (default for XTerm)
	/// If *false* new line is created implicitly (default for WinAPI)
	void SetBeyondEOL(bool beyondEOL);

	/// Return iterator (m_Workspace) for row with cursor
	iRow CurrentRow();
	/// Return iterator (m_Workspace) for row by index
	iRow GetRow(unsigned row);

	/// Direct modification
	void Write(const wchar_t* text, unsigned count);
	void InsertCell(unsigned count = 1, const wchar_t chr = L' ');
	void DeleteCell(unsigned count = 1);
	void InsertRow(unsigned count = 1);
	void DeleteRow(unsigned count = 1);

	/// Scroll workspace
	/// @param dir - positive values = scroll downward, negative values = upward
	void Scroll(int dir);

	/// Emulate 'Enter' keypress - move cursor one row down or scroll contents upward
	/// @param cr - move cursor to first cell in row too
	void LineFeed(bool cr);

	/// Returns console data
	/// @param pData - destination buffer of { wchar_t; Attribute; }
	/// @param bufSize - dimensions of *pData*
	/// @param rgn - negative vertical coordinates represent backscroll buffer
	void GetData(CharInfo *pData, const Coord& bufSize, const RECT& rgn) const;

	/// Bell or Flash on Write("\7")
	void SetBellCallback(BellCallback bellCallback, void* param);

	/// Let remember when *prompt* input was started
	/// Used for typed command selection by `Shift+Home`
	void PromptPosStore();
	/// Drop prompt position stored by PromptPosStore()
	void PromptPosReset();

	/// If shell emits its current directory - remember it in the current row
	/// @param cwd - current working directory; it may be either in Unix or Windows format, store unchanged
	void SetWorkingDirectory(const wchar_t* cwd);

	/// Dump of Backscroll & Workspace contents to debug output
	void DebugDump(bool workspace_only = false) const;

protected:
	/// Scroll rows upward, move top row to backscroll buffer if required
	/// @param dir - positive values = scroll downward, negative values = upward
	void DoAutoScroll(int dir);

	/// Options for *ShiftRowIndexes*
	enum ShiftRowIndexesEnum : unsigned
	{
		ShiftCursor = 1,
		ShiftRegion = 2,
		ShiftPrompt = 4,
		ShiftAll    = (unsigned)-1
	};
	/// Called during resize
	/// @param direction - only `-1` or `+1` are allowed now
	/// @param flags - enum *ShiftRowIndexesEnum*
	void ShiftRowIndexes(int direction, unsigned flags = ShiftAll);

private:
	void ReflowResize(const Coord newSize);
	unsigned StoreGrapheme(const std::wstring& text);
	bool AppendGrapheme(const std::wstring& text, unsigned codepoint);
	std::map<unsigned, std::wstring> m_Graphemes;
	std::map<std::wstring, unsigned> m_GraphemeLookup;
	bool m_Reflow = false;
	bool m_ExtendedUnicode = false;
	wchar_t m_PendingHigh = 0;
	/// Bell or Flash on Write("\7")
	BellCallback m_BellCallback = nullptr;
	void* m_BellCallbackParam = nullptr;

	/// Working part of the console, where POSIX applications may write
	vector_type m_Workspace;
	/// History part of the console, here go rows went out of scope on auto-scrolling
	deque_type m_Backscroll;
	/// Maximum count of history rows, -1 means "don't limit"
	int m_BackscrollMax = -1;
	/// Dimensions of viewport (m_Workspace)
	Coord m_Size = {};
	/// Current cursor position
	Coord m_Cursor = {};
	/// Current output attributes
	Attribute m_Attr = {7};
	/// Just for drawing oldschool ANSI arts designed for 80-cell terminals
	/// To enable - "ESC [ 7 ; _col_ h", to disable - "ESC [ 7 ; l"
	int m_WrapAt = -1;
	/// Scrolling region if *m_Flags.Region* is true
	Region m_Region = {};
	/// Coordinates stored by *PromptPosStore*
	Coord m_Prompt = {};

	struct Flags
	{
		/// Emit CR automatically after single LF
		bool AutoCRLF = true;
		/// Defines behavior when character is written at the last cell of the row
		/// If *true* (enabled by "ESC [ ? 7 h") we may advance cursor and create new line (perhaps on next write if *BeyondEOL*)
		/// If *false* (enabled by "ESC [ ? 7 l") cursor stays at *m_Size.x-1* indefinitely
		bool AutoWrap = true;
		/// Option is ignored if *AutoWrap* is *false*, cursor stays at *m_Size.x-1* indefinitely
		/// If *true* cursor may stay "after" EOL until next write or cursor move (default for XTerm)
		/// If *false* new line is created implicitly (default for WinAPI)
		bool BeyondEOL = true;
		/// If *true* the cursor is locked inside specified rows in viewport
		bool Region = false;
		/// If *true* we are in the prompt/readline routine
		bool Prompt = false;

	} m_Flags;

};

}; /// namespace console
