
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR

#include "../common/Common.h"
#include "../common/CEStr.h"
#include "../common/wcwidth.h"

#include "ConData.h"
#include "../common/UnicodeGrapheme.h"
#include <memory>
#include <atomic>
#include <set>

#undef TABLE_DEBUGDUMP
#undef TABLE_UNITTESTS
//#ifdef _DEBUG
//#define TABLE_DEBUGDUMP
//#ifndef TABLE_UNITTESTS
//#define TABLE_UNITTESTS
//#endif
//#endif

namespace condata
{


console_error::console_error(const char* message)
	: logic_error(message)
{
}



#ifdef TABLE_UNITTESTS
struct RowUnitTest
{
	struct Cell { wchar_t ch; Attribute attr; };

	void Validate(std::vector<Cell>& data, Row& row)
	{
		const auto len = row.GetLength();
		row.Validate();
		_ASSERTE(data.size() == len);
		const ssize_t max_len = std::min<ssize_t>(data.size(), row.GetLength());
		Attribute attr; unsigned hint = 0;
		for (ssize_t i = 0; i < max_len; ++i)
		{
			const auto& cell = data[i];
			hint = row.GetCellAttr(unsigned(i), attr, hint);
			const auto wc = row.GetCellChar(unsigned(i));
			_ASSERTE((attr == cell.attr));
			_ASSERTE((wc == cell.ch));
		}
	}

	void RequestSize(std::vector<Cell>& data, unsigned new_size)
	{
		if (data.size() < new_size)
		{
			Cell cell { L' ' };
			if (!data.empty())
				cell.attr = data.back().attr;
			data.insert(data.end(), new_size - data.size(), cell);
		}
	}

	void Write(std::vector<Cell>& data, Row& row, unsigned cell, const Attribute& attr, const wchar_t* text, unsigned count)
	{
		RequestSize(data, cell + count);
		for (unsigned i = 0; i < count; ++i)
		{
			data[cell + i].ch = text[i];
			data[cell + i].attr = attr;
		}
		row.Write(cell, attr, text, count);
		Validate(data, row);
	}

	void Insert(std::vector<Cell>& data, Row& row, unsigned cell, const Attribute& attr, const wchar_t chr, unsigned count)
	{
		RequestSize(data, cell);
		data.insert(data.begin() + cell, count, Cell { chr, attr });
		row.Insert(cell, attr, chr, count);
		Validate(data, row);
	}

	void Delete(std::vector<Cell>& data, Row& row, unsigned cell, unsigned count)
	{
		if (cell < data.size())
			data.erase(data.begin()+cell, data.begin()+std::min<size_t>(cell+count, data.size()));
		row.Delete(cell, count);
		Validate(data, row);
	}

	void SetAttr(std::vector<Cell>& data, Row& row, unsigned cell, const Attribute& attr, unsigned count)
	{
		RequestSize(data, cell + count);
		for (size_t i = 0; i < count; ++i)
		{
			data[cell + i].attr = attr;
		}
		row.SetCellAttr(cell, attr, count);
		Validate(data, row);
	}

	RowUnitTest()
	{
		// single row - test 1
		{
			Row row;
			std::vector<Cell> data;
			// " abc"
			Write(data, row, 1, Attribute{1}, L"abc", 3);
			// " abc def"
			Write(data, row, 5, Attribute{2}, L"def", 3);
			// " abc def 123"
			Write(data, row, 9, Attribute{3}, L"123", 3);
			// "abc def 123"
			Delete(data, row, 0, 1);
			// "abc ef 123"
			Delete(data, row, 4, 1);
			// "abc e f 123"
			Insert(data, row, 5, Attribute{4}, L' ', 1);
			// "abc e f  123"
			Insert(data, row, 8, Attribute{5}, L' ', 1);
			// "ab23"
			Delete(data, row, 2, 8);
			// "ab  23"
			Insert(data, row, 2, Attribute{6}, L' ', 2);
			// "ab  23"
			SetAttr(data, row, 1, Attribute{7}, 4);
			// ""
			Delete(data, row, 0, row.GetLength());
		}

		// single row - test 2
		{
			Row row{Attribute{1}};
			std::vector<Cell> data;
			// "a"
			Write(data, row, 0, Attribute{1}, L"a", 1);
			// "ab"
			Write(data, row, 1, Attribute{1}, L"b", 1);
			// "abc"
			Write(data, row, 2, Attribute{1}, L"c", 1);
			// "bc"
			Delete(data, row, 0, 1);
			// "c"
			Delete(data, row, 0, 1);
			// ""
			Delete(data, row, 0, 1);
			// "a"
			Write(data, row, 0, Attribute{1}, L"a", 1);
			// "ab"
			Write(data, row, 1, Attribute{1}, L"b", 1);
			// "abc"
			Write(data, row, 2, Attribute{2}, L"c", 1);
			// "abcd"
			Write(data, row, 3, Attribute{2}, L"d", 1);
			// "abcde"
			Write(data, row, 4, Attribute{3}, L"e", 1);
			// "abcdef"
			Write(data, row, 5, Attribute{3}, L"f", 1);
			// "12345f"
			Write(data, row, 0, Attribute{1}, L"12345", 5);
			// "a2345f"
			Write(data, row, 0, Attribute{2}, L"a", 1);
			// "a25f"
			Delete(data, row, 2, 2);
		}

		// working table - test 1
		{
			Table table({7}, {10/*cols*/, 4/*rows*/}, 10);
			const wchar_t txt1[] = L"1234567890abcdefg";
			table.SetAttr(Attribute{1});
			table.Write(txt1, (unsigned)wcslen(txt1));
			table.DebugDump();
			//
			const wchar_t txt2[] = L"***\n###\t###\n\t===\r---\n";
			table.SetAttr(Attribute{2});
			table.Write(txt2, (unsigned)wcslen(txt2));
			table.DebugDump();
			//
			CharInfo data[10][12];
			table.GetData((CharInfo*)data, Coord{12, 10}, RECT{0, -4, 9, 3});
			//
			wchar_t txt3[10];
			for (int i = 0; i < 14; ++i)
			{
				for (int j = 0; j < countof(txt3); ++j) txt3[j] = '0'+i;
				table.SetAttr(Attribute{(unsigned short)(3+i)});
				table.Write(txt3, 10);
				table.DebugDump();
			}
		}

		// working table - test 2 (excessive output)
		//_ASSERTE(FALSE && "Continue to 100000 lines test");
		DWORD start_tick = GetTickCount();
		{
			Table table({7}, {110/*cols*/, 34/*rows*/}, -1);
			wchar_t line[32];
			for (unsigned i = 1; i <= 100000; ++i)
			{
				msprintf(line, std::size(line), L"%u\n", i);
				unsigned len = unsigned(wcslen(line));
				table.Write(line, len);
			}
		}
		DWORD end_tick = GetTickCount();
		DWORD duration = end_tick - start_tick;
		wchar_t dbg[80]; swprintf_s(dbg, L"100000 test duration: %.3fs\n", duration / 1000.0f);
		OutputDebugStringW(dbg);
		//_ASSERTE(FALSE && "100000 lines test finished");

		// working table - test 3 (scrolling)
		{
			Table table({7}, {4/*cols*/, 8/*rows*/}, 10);
			wchar_t line[4] = L"**\n";
			for (char c = '1'; c <= '8'; ++c)
			{
				line[0] = line[1] = c;
				table.Write(line, 3);
			}
			table.DebugDump();
			//
			table.Scroll(-4);
			table.DebugDump();
			//
			table.Scroll(-2);
			table.DebugDump();
			//
			table.Scroll(1);
			table.DebugDump();
			//
			table.Scroll(5);
			table.DebugDump();
		}
	};
};
RowUnitTest rowUnitTest;
#endif



/**
	Color and font outline attributes
**/

bool Attribute::isBold() const
{
	return (flags & fBold);
}
bool Attribute::isItalic() const
{
	return (flags & fItalic);
}
bool Attribute::isUnderscore() const
{
	return (attr.attr & COMMON_LVB_UNDERSCORE);
}
bool Attribute::isReverse() const
{
	return (attr.attr & COMMON_LVB_REVERSE_VIDEO);
}
bool Attribute::isFore4b() const
{
	return !(flags & (fFore8b | fFore24b));
}
bool Attribute::isFore8b() const
{
	return (flags & fFore8b);
}
bool Attribute::isFore24b() const
{
	return (flags & fFore24b);
}
bool Attribute::isBack4b() const
{
	return !(flags & (fBack8b | fBack24b));
}
bool Attribute::isBack8b() const
{
	return (flags & fBack8b);
}
bool Attribute::isBack24b() const
{
	return (flags & fBack24b);
}
bool Attribute::operator==(const Attribute& a) const
{
	return (attr.attr == a.attr.attr && flags == a.flags
		&& crForeColor == a.crForeColor && crBackColor == a.crBackColor && hyperlinkId == a.hyperlinkId && graphemeId == a.graphemeId);
}
bool Attribute::operator!=(const Attribute& a) const
{
	return !(*this == a);
}



/**
	Class represents one console row (line)
**/

Row::Row(const Attribute& attr /*= {}*/)
{
	m_Colors.push_back(RowColor{0, attr});
}

Row::~Row()
{
}

Row::Row(const Row& src)
{
	(*this) = src;
}

Row::Row(Row&& src)
{
	*this = std::move(src);
}

Row& Row::operator=(const Row& src)
{
	src.Validate();
	m_RowID = src.m_RowID;
	m_WasCR = src.m_WasCR;
	m_WasLF = src.m_WasLF;
	SoftWrapped = src.SoftWrapped;
	SoftWrapLength = src.SoftWrapLength;
	m_CWD = src.m_CWD;

	// If src is not empty
	if (unsigned length = src.GetLength())
	{
		m_Text = src.m_Text;
		m_Colors = src.m_Colors;
	}
	else
	{
		m_Text = decltype(m_Text){};
		m_Colors = decltype(m_Colors){};
		if (!src.m_Colors.empty())
		{
			_ASSERTE(src.m_Colors[0].cell == 0);
			m_Colors.push_back(src.m_Colors[0]);
			m_Colors[0].cell = 0;
		}
		else
		{
			_ASSERTE(!src.m_Colors.empty());
			m_Colors.push_back(RowColor{0});
		}
	}

	Validate();

	return *this;
}

Row& Row::operator=(Row&& src)
{
	src.Validate();
	m_RowID = src.m_RowID; src.m_RowID = 0;
	m_WasCR = src.m_WasCR; src.m_WasCR = -1;
	m_WasLF = src.m_WasLF;
	SoftWrapped = src.SoftWrapped;
	SoftWrapLength = src.SoftWrapLength; src.m_WasLF = -1;
	m_Colors = std::move(src.m_Colors);
	m_Text = std::move(src.m_Text);
	m_CWD = std::move(src.m_CWD);
	return *this;
}

unsigned Row::SetReqSize(unsigned cell)
{
	if (m_Colors.empty())
	{
		_ASSERTE(m_Colors.size() > 0);
		// Should be at least one color (even black-on-black) per row
		m_Colors.push_back({0, Attribute{}});
	}

	const unsigned cur_len = GetLength();
	if (cell <= cur_len)
		return cur_len;

	// if cell is greater than previous output (e.g. cursor was moved after "eol")
	_ASSERTE(cell > m_Text.length());
	// additional '\0'-termination
	m_Text.resize(cell, L' ');

	return cell;
}

void Row::Reserve(unsigned len)
{
	m_Text.reserve(len);
}

void Row::Write(unsigned cell, const Attribute& attr, const wchar_t* text, unsigned count)
{
	if (!text || !count)
		return;
	Write(cell, attr, {text, 0}, count);
}

void Row::Write(unsigned cell, const Attribute& attr, const wchar_t chr, unsigned count)
{
	if (!count)
		return;
	_ASSERTE(chr != 0);
	Write(cell, attr, {nullptr, chr?chr:L' '}, count);
}

void Row::ClearWideOutside(unsigned cell, unsigned count, const Attribute& replacement)
{
	if (!count) return;
	auto clear = [&](unsigned index) {
		if (index >= GetLength()) return;
		Attribute attr; GetCellAttr(index, attr);
		attr.graphemeId = attr.hyperlinkId = 0;
		attr.attr.attr &= ~(COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE);
		m_Text[index] = L' ';
		SetCellAttr(index, attr, 1);
	};
	Attribute attr;
	if (cell && cell < GetLength())
	{
		GetCellAttr(cell, attr);
		if ((attr.attr.attr & COMMON_LVB_TRAILING_BYTE) &&
			(!(replacement.attr.attr & COMMON_LVB_TRAILING_BYTE) || attr.graphemeId != replacement.graphemeId)) clear(cell - 1);
	}
	const unsigned last = cell + count - 1;
	if (last < GetLength())
	{
		GetCellAttr(last, attr);
		if ((attr.attr.attr & COMMON_LVB_LEADING_BYTE) &&
			(!(replacement.attr.attr & COMMON_LVB_LEADING_BYTE) || attr.graphemeId != replacement.graphemeId)) clear(last + 1);
	}
}

// protected
void Row::Write(unsigned cell, const Attribute& attr, std::pair<const wchar_t*,wchar_t> txt, unsigned count)
{
	if (!count)
		return;

	ClearWideOutside(cell, count, attr);
	ResetCRLF();

	// Reserve(((cell + count + 80) / 80) * 80);

	SetReqSize(cell + count);
	for (unsigned i = 0; i < count; ++i)
	{
		if (txt.first)
			m_Text[cell + i] = txt.first[i];
		else
			m_Text[cell + i] = txt.second;
	}

	SetCellAttr(cell, attr, count);
}

void Row::ResetCRLF()
{
	if (m_WasCR != -1)
		m_WasCR = -1;
	if (m_WasLF != -1)
		m_WasLF = -1;
}

void Row::Insert(unsigned cell, const Attribute& attr, const wchar_t chr, unsigned count)
{
	ResetCRLF();

	if (!count)
		return;

	if (cell < GetLength())
	{
		Attribute old; GetCellAttr(cell, old);
		if (old.attr.attr & COMMON_LVB_TRAILING_BYTE)
		{
			old.attr.attr &= ~(COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE);
			old.graphemeId = old.hyperlinkId = 0;
			Write(cell, old, L' ', 1);
		}
	}

	Reserve(m_Text.length() + count);
	SetReqSize(cell);
	m_Text.insert(cell, count, chr);

	Validate();
	for (size_t i = 0; i < m_Colors.size(); ++i)
	{
		if (m_Colors[i].cell >= cell)
			m_Colors[i].cell += count;
	}

	SetCellAttr(cell, attr, count);
}

void Row::Delete(unsigned cell, unsigned count)
{
	ResetCRLF();

	const unsigned cur_len = GetLength();
	if (!count || cell >= cur_len)
		return;
	if (cell + count > cur_len)
		count = cur_len - cell;

	ClearWideOutside(cell, count, Attribute{});

	Attribute firstAttr, lastAttr, newAttr;
	unsigned first = GetCellAttr(cell, firstAttr);
	unsigned last = GetCellAttr(cell + count, lastAttr, first);

	Validate();
	if (first < last)
	{
		unsigned del_first = (m_Colors[first].cell == cell) ? first : first + 1;
		if (del_first < last)
		{
			m_Colors.erase(m_Colors.begin() + del_first, (last < m_Colors.size()) ? (m_Colors.begin() + last) : m_Colors.end());
			// Validate(); -- invalid till next decrement cycle
		}
	}
	for (size_t i = first; i < m_Colors.size(); ++i)
	{
		auto& ic = m_Colors[i];
		if (ic.cell >= cell + count)
			ic.cell -= count;
		else if (ic.cell > cell)
			ic.cell = cell;
	}
	Validate();

	if (cell < GetLength())
	{
		GetCellAttr(cell, newAttr, first);
		if (!(newAttr == lastAttr))
			m_Colors.insert(m_Colors.begin() + first, RowColor{cell, lastAttr});
		Validate();
	}

	// Do it at the end because GetCellAttr take into account m_Text length
	m_Text.erase(cell, count);
	Validate();
}

unsigned Row::GetLength() const
{
	const auto tsize = m_Text.length();
	if (tsize > 0)
	{
		_ASSERTE(tsize == unsigned(tsize));
		return unsigned(tsize);
	}
	// Empty line
	return 0;
}

void Row::SetCR(int pos)
{
	if (pos >= 0)
	{
		if (m_WasLF != -1)
			m_WasLF = -1;

		unsigned len = GetLength();

		if (pos > 0)
			m_WasCR = pos;
	}
}

void Row::SetLF(int pos)
{
	SoftWrapped = false;
	if (pos < 0)
	{
		m_WasLF = -1;
	}
	else
	{
		unsigned len = GetLength();
		unsigned new_pos = pos ? pos : (m_WasCR >= 0) ? unsigned(m_WasCR) : 0;
		//_ASSERTE((unsigned)new_pos == len);

		if ((unsigned)new_pos >= len)
			m_WasLF = new_pos;
		else
			m_WasLF = -1;
	}
}

bool Row::HasLineFeed() const
{
	if (m_WasLF >= 0 && (unsigned)m_WasLF >= GetLength())
		return true;
	return false;
}

const wchar_t* Row::GetText() const
{
	unsigned len = GetLength();
	if (!len)
		return L"";
	return m_Text.c_str();
}

unsigned Row::GetCellAttr(unsigned cell, Attribute& attr, unsigned hint /*= 0*/) const
{
	if (m_Colors.empty())
	{
		_ASSERTE(FALSE && "m_Colors should be already filled");
		attr = Attribute{};
		return 0;
	}

	unsigned result;

	// We may use binary search for optimization, but
	// expected size of the m_Colors is just a few elements
	for (size_t i = hint; i < m_Colors.size(); ++i)
	{
		const auto& ic = m_Colors[i].cell;
		if (ic < cell)
			continue;
		if ((ic > cell) && (i > 0))
		{
			_ASSERTE(m_Colors[i - 1].cell < cell);
			attr = m_Colors[i - 1].attr;
			result = unsigned(i - 1);
			goto wrap;
		}
		else
		{
			_ASSERTE(ic == cell);
			attr = m_Colors[i].attr;
			result = unsigned(i);
			goto wrap;
		}
	}

	attr = m_Colors[m_Colors.size()-1].attr;
	result = unsigned(m_Colors.size()-1);
wrap:
	return result;
}

void Row::SetCellAttr(unsigned cell, const Attribute& attr, unsigned count)
{
	_ASSERTE(cell < GetLength() && count);

	// #condata Optimize SetCellAttr to use explicit m_Attr0 instead of vector

	Validate();

	SetReqSize(cell + count);

	Attribute firstAttr, nextAttr, newAttr;
	unsigned first = GetCellAttr(cell, firstAttr);
	unsigned next = GetCellAttr(cell + count, nextAttr, first);
	#ifdef _DEBUG
	const decltype(m_Colors) color_copy(m_Colors.cbegin(), m_Colors.cend());
	#endif

	// [...] ... [first:A] ... [next:B] ... [...]

	if (firstAttr == attr)
	{
		_ASSERTE(m_Colors.size() > 0); // must be validated in SetReqSize
		if (first < next)
		{
			if (m_Colors[next].cell < cell + count)
				m_Colors[next].cell = cell + count;
			if (first + 1 < next)
			{
				const unsigned del_first = first + 1;
				m_Colors.erase(m_Colors.begin() + del_first, m_Colors.begin() + next);
			}
		}
	}
	else
	{
		if (m_Colors[first].cell == cell)
		{
			m_Colors[first].attr = attr;
			// [...] ... [first:attr] ... [next:B] ... [...]
		}
		else
		{
			_ASSERTE(m_Colors[first].cell<cell && (first+1 >= m_Colors.size() || m_Colors[first+1].cell > cell));
			m_Colors.insert(m_Colors.begin() + (++first), RowColor{cell, attr});
			// [...] ... [first-1:A] [first:attr] ... [next:B] ... [...]
		}
		next = first + 1;
		// Validate(); -- may be invalid at the moment

		if (next < m_Colors.size())
		{
			if (m_Colors[next].cell < cell + count)
			{
				while (next + 1 < m_Colors.size() && m_Colors[next + 1].cell <= cell + count)
				{
					++next;
				}
				if (next < m_Colors.size() && m_Colors[next].cell < cell + count)
				{
					_ASSERTE(next > 0);
					m_Colors[next].cell = cell + count;
				}
			}
		}

		// Drop overwritten regions
		if (first + 1 < next)
		{
			const unsigned del_first = first + 1;
			m_Colors.erase(m_Colors.begin() + del_first, m_Colors.begin() + next);
		}

		// Validate next, may be changed after insert/erase
		if (cell + count < GetLength())
		{
			unsigned new_next = GetCellAttr(cell + count, newAttr, first);
			if (!(newAttr == nextAttr))
			{
				m_Colors.insert(m_Colors.begin() + (new_next + 1), RowColor{cell + count, nextAttr});
				Validate();
			}
		}
	}

	Validate();
}

void Row::Reset(const Attribute& attr)
{
	SoftWrapped = false;
	m_RowID = 0;
	ResetCRLF();
	m_Text.clear();
	m_Colors.clear();
	m_CWD.Release();

	m_Colors.push_back(RowColor{0, attr});
}

void Row::SetWorkingDirectory(const wchar_t* cwd)
{
	if (!cwd || !*cwd)
		m_CWD.Release();
	else
		m_CWD.Set(cwd);
}

const wchar_t* Row::GetWorkingDirectory() const
{
	return m_CWD.IsEmpty() ? nullptr : m_CWD.c_str();
}

wchar_t Row::GetCellChar(unsigned cell) const
{
	wchar_t ch = (cell < m_Text.length()) ? m_Text[cell] : L' ';
	return ch ? ch : L' ';
}

void Row::Validate() const
{
#ifdef _DEBUG
	const unsigned len = GetLength();
	_ASSERTE(len==0 || (m_Colors.size()>=1 && m_Colors[0].cell==0));
	ssize_t prev = 0;
	for (size_t i = 1; i < m_Colors.size(); ++i)
	{
		_ASSERTE(prev < ssize_t(m_Colors[i].cell));
		_ASSERTE(m_Colors[i].cell < len);
		prev = ssize_t(m_Colors[i].cell);
	}
#endif
}



iRow::iRow(unsigned _index, Row* _row)
	: row(_row), index(_index)
{
}

Row* iRow::operator->() const
{
	return row;
}

Row& iRow::operator*() const
{
	_ASSERTE(row);
	return *row;
}



/**
	Main class to manipulate console surface
**/

Table::Table(const Attribute& _attr /*= {7}*/, const Coord& _size /*= {80, 25}*/,
			 const int _backscrollMax /*= 1<<20*/, BellCallback _bellCallback /*= nullptr*/)
	: m_Attr(_attr)
	, m_Size(_size)
	, m_BackscrollMax(_backscrollMax)
	, m_BellCallback(_bellCallback)
{
}

Table::~Table()
{
}

bool Table::IsEmpty() const
{
	//if (m_Workspace.empty())
	//	return true;
	if (m_Size.x < 1 || m_Size.y < 1)
		return true;
	return false;
}

Coord Table::GetSize() const
{
	_ASSERTE(m_Size.x >= 1 && m_Size.y >= 1);
	return Coord{m_Size.x, std::max<int>(0, m_Size.y)};
}

Coord Table::GetCursor() const
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return Coord{};
	}
	_ASSERTE(m_Cursor.y >= 0);
	//Unix terminals do not restrict the cursor to be in the scrolling area
	//only incremental movement (up/down/left/right) are restricted,
	//and only if the cursor IS inside scrolling area
	//const Region rgn = GetScrollRegion();
	return Coord{std::max<unsigned>(0, std::min(m_Cursor.x, m_Size.x)),
		         std::max<int>(0, std::min<int>(m_Cursor.y, m_Size.y))};
}

unsigned Table::GetBackscroll() const
{
	return (unsigned) m_Backscroll.size();
}

void Table::SetBackscrollMax(const int backscrollMax)
{
	m_BackscrollMax = (backscrollMax < 0) ? -1 : backscrollMax;

	if (m_BackscrollMax >= 0 && m_Backscroll.size() > size_t(m_BackscrollMax))
		m_Backscroll.erase(m_Backscroll.begin() + m_BackscrollMax, m_Backscroll.end());
}

condata::Attribute Table::GetAttr() const
{
	return m_Attr;
}

void Table::Reset(const Attribute& attr)
{
	OriginMode = InsertMode = false;
	m_PendingHigh = 0;
	SetScrollRegion(false);
	SetCursor({});
	PromptPosReset();
	m_WrapAt = -1;
	//DeleteRow((unsigned)m_Workspace.size());
	m_Workspace.clear();
	m_Backscroll.clear();
	SetAttr(attr);
}

void Table::ClearBackscroll()
{
	m_Backscroll.clear();
}

void Table::ClearWorkspace()
{
	SetScrollRegion(false); // ?
	PromptPosReset();
	m_Workspace.clear();
}

void Table::ReflowResize(const Coord newSize)
{
	std::vector<Row*> source;
	for (auto i = m_Backscroll.rbegin(); i != m_Backscroll.rend(); ++i) source.push_back(i->get());
	const size_t history = source.size();
	size_t used = std::min<size_t>(m_Workspace.size(), m_Size.y);
	while (used > static_cast<size_t>(m_Cursor.y + 1) && !m_Workspace[used - 1]->GetLength()) --used;
	for (size_t i = 0; i < used; ++i) source.push_back(m_Workspace[i].get());
	while (source.size() <= history + m_Cursor.y) source.push_back(nullptr);
	std::vector<std::unique_ptr<Row>> rows;
	std::vector<CharInfo> logical;
	std::wstring workingDirectory;
	size_t cursorOffset = 0, cursorRow = 0;
	unsigned cursorX = 0;
	bool hasCursor = false;
	auto flush = [&]() {
		rows.emplace_back(new Row{m_Attr});
		rows.back()->SetWorkingDirectory(workingDirectory.c_str());
		unsigned x = 0;
		for (size_t i = 0; i < logical.size();)
		{
			const auto& cell = logical[i];
			const bool pair = i + 1 < logical.size() &&
				((cell.chr >= 0xD800 && cell.chr <= 0xDBFF && logical[i + 1].chr >= 0xDC00 && logical[i + 1].chr <= 0xDFFF)
				|| (cell.attr.attr.attr & COMMON_LVB_LEADING_BYTE));
			const unsigned width = pair ? 2 : 1;
			if (x && x + width > newSize.x)
			{
				rows.back()->SoftWrapped = true;
				rows.back()->SoftWrapLength = x;
				rows.emplace_back(new Row{m_Attr});
				rows.back()->SetWorkingDirectory(workingDirectory.c_str());
				x = 0;
			}
			if (hasCursor && cursorOffset >= i && cursorOffset < i + width) { cursorRow = rows.size() - 1; cursorX = x; }
			for (unsigned n = 0; n < width; ++n)
				rows.back()->Write(x + n, logical[i + n].attr, logical[i + n].chr, 1);
			x += width;
			i += width;
			if (x >= newSize.x && i < logical.size())
			{
				rows.back()->SoftWrapped = true;
				rows.back()->SoftWrapLength = x;
				rows.emplace_back(new Row{m_Attr});
				rows.back()->SetWorkingDirectory(workingDirectory.c_str());
				x = 0;
			}
		}
		if (hasCursor && cursorOffset >= logical.size()) { cursorRow = rows.size() - 1; cursorX = x; }
		if (!rows.back()->SoftWrapped) rows.back()->SetLF(x);
		logical.clear();
		hasCursor = false;
	};
	for (size_t y = 0; y < source.size(); ++y)
	{
		const auto* row = source[y];
		if (row && row->GetWorkingDirectory()) workingDirectory = row->GetWorkingDirectory();
		const bool cursor = y == history + m_Cursor.y;
		unsigned length = row ? std::min(row->GetLength(), m_Size.x) : 0;
		if (row && row->SoftWrapped) length = std::min(row->SoftWrapLength, m_Size.x);
		if (cursor)
		{
			length = std::max(length, m_Cursor.x);
			hasCursor = true;
			cursorOffset = logical.size() + m_Cursor.x;
		}
		Attribute attr = m_Attr;
		for (unsigned x = 0; x < length; ++x)
		{
			if (row) row->GetCellAttr(x, attr);
			logical.push_back({row ? row->GetCellChar(x) : L' ', attr});
		}
		if (!row || !row->SoftWrapped || y + 1 == source.size()) flush();
	}
	if (rows.empty())
	{
		rows.emplace_back(new Row{m_Attr});
		rows.back()->SetWorkingDirectory(workingDirectory.c_str());
	}
	const size_t top = std::min(cursorRow, rows.size() > static_cast<size_t>(newSize.y) ? rows.size() - newSize.y : 0);
	const size_t oldest = m_BackscrollMax >= 0 && top > static_cast<size_t>(m_BackscrollMax) ? top - m_BackscrollMax : 0;
	m_Backscroll.clear(); m_Workspace.clear();
	for (size_t y = oldest; y < top; ++y) m_Backscroll.emplace_front(rows[y].release());
	for (size_t y = top; y < rows.size() && y < top + newSize.y; ++y) m_Workspace.emplace_back(rows[y].release());
	m_Size = newSize;
	SetCursor({std::min(cursorX, newSize.x), static_cast<int>(cursorRow - top)});
	PromptPosReset();
}

void Table::Resize(const Coord newSize)
{
	if (newSize.x < 1 || newSize.y < 1)
	{
		_ASSERTE(newSize.x >= 1 && newSize.y >= 1);
		return;
	}

	if (m_Reflow && !m_Flags.Region && newSize.x != m_Size.x)
	{
		ReflowResize(newSize);
		return;
	}

	// If newSize.y becomes larger than m_Size.y, return lines from backscroll to working area
	while (newSize.y > m_Size.y)
	{
		if (m_Backscroll.empty())
			break;
		m_Workspace.insert(m_Workspace.begin(), m_Backscroll.front());
		m_Backscroll.erase(m_Backscroll.begin());
		++m_Size.y;
		ShiftRowIndexes(+1);
	}

	// If newSize.y becomes lesser than m_Size.y, push upper lines to backscroll and adjust cursor position
	size_t drop_row = 0;
	while (newSize.y < m_Size.y)
	{
		if (m_Workspace.empty())
			break;
		if (m_BackscrollMax != 0)
			m_Backscroll.push_front(m_Workspace[drop_row]);
		if (m_BackscrollMax >= 0 && (ssize_t)m_Backscroll.size() > m_BackscrollMax)
			m_Backscroll.erase(m_Backscroll.begin() + m_BackscrollMax, m_Backscroll.end());
		++drop_row;
		--m_Size.y;
		ShiftRowIndexes(-1);
	}
	if (drop_row > 0)
	{
		m_Workspace.erase(m_Workspace.begin(), m_Workspace.begin() + drop_row);
	}

	// The height may be not increased, if backscroll if yet empty for example
	_ASSERTE(m_Size.y == newSize.y || (m_Backscroll.empty() && m_Size.y < newSize.y));
	m_Size = newSize;

	// Ensure the cursor is in allowed place
	SetCursor(m_Cursor);
}

void Table::ShiftRowIndexes(int direction, unsigned flags /*= ShiftAll*/)
{
	if (direction > 0)
	{
		_ASSERTE(direction == 1);
		if (flags & ShiftCursor)
		{
			SetCursor({m_Cursor.x, m_Cursor.y + direction});
		}
		if ((flags & ShiftRegion) && m_Flags.Region && ((int)m_Region.Bottom + 1 < m_Size.y))
		{
			m_Region.Top += direction;
			m_Region.Bottom += direction;
		}
		if (flags & ShiftPrompt)
		{
			if (m_Flags.Prompt && (m_Prompt.y + 1 < m_Size.y))
				m_Prompt.y += direction;
			else
				m_Flags.Prompt = false;
		}
	}
	else if (direction < 0)
	{
		_ASSERTE(direction == -1);
		if (flags & ShiftCursor)
		{
			SetCursor({m_Cursor.x, m_Cursor.y - direction});
		}
		if ((flags & ShiftRegion) && m_Flags.Region && ((int)m_Region.Top > 0))
		{
			m_Region.Top += direction;
			m_Region.Bottom += direction;
		}
		if (flags & ShiftPrompt)
		{
			if (m_Flags.Prompt && (m_Prompt.y > 0))
				m_Prompt.y += direction;
			else
				m_Flags.Prompt = false;
		}
	}
}

void Table::SetAttr(const Attribute& attr)
{
	// #condata Show warning when set "black-on-black"?
	m_Attr = attr;
}

void Table::SetCursor(const Coord cursor)
{
	_ASSERTE(m_Size.x >= 1 && m_Size.y >= 1);
	m_Cursor = {
		// If AutoWrap == false - cursor stays at *m_Size.x-1* indefinitely
		std::min<unsigned>(cursor.x, m_Flags.AutoWrap ? m_Size.x  : (m_Size.x - 1)),
		std::max<int>(0, std::min<int>(cursor.y, m_Size.y - 1))
	};
}

void Table::SetScrollRegion(bool setRegion /*= false*/, unsigned top /*= 0*/, unsigned bottom /*= 0*/)
{
	if (setRegion && ((int)top >= 0 && (int)top <= (int)bottom && (int)bottom < m_Size.y))
	{
		m_Region.Top = top;
		m_Region.Bottom = bottom;
		m_Flags.Region = true;
		// #condata e.g. mintty resets cursor explicitly in position {0,0}, but that looks like a bug (check this)
		SetCursor({0, int(top)});
	}
	else
	{
		_ASSERTE(!setRegion);
		m_Flags.Region = false;
		m_Region.Top = m_Region.Bottom = 0;
	}
}

void Table::SetAutoCRLF(bool autoCRLF)
{
	m_Flags.AutoCRLF = autoCRLF;
}

void Table::SetAutoWrap(bool autoWrap, int wrapAt /*= -1*/)
{
	m_Flags.AutoWrap = autoWrap;
	m_WrapAt = wrapAt;
}

void Table::SetBeyondEOL(bool beyondEOL)
{
	m_Flags.BeyondEOL = beyondEOL;
}

iRow Table::CurrentRow()
{
	_ASSERTE(m_Cursor.y >= 0 && m_Cursor.y < m_Size.y);
	// Extend workspace to requested row
	const unsigned idx = (unsigned)std::max<int>(0, (int) std::min<ssize_t>(m_Size.y - 1, m_Cursor.y));
	_ASSERTE(idx < 1000); // sane console dimensions?

	return GetRow(idx);
}

iRow Table::GetRow(unsigned row)
{
	// It checks only m_Size, m_Workspace would be resized to required amount
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		throw console_error("Workspace is empty");
	}
	_ASSERTE((int)row >= 0 && (int)row < m_Size.y);
	// Extend workspace to requested row
	const unsigned idx = (unsigned)std::max<int>(0, (int) std::min<ssize_t>(m_Size.y - 1, row));
	_ASSERTE(idx < 1000); // sane console dimensions?
	m_Workspace.reserve(idx+1);
	while (m_Workspace.size() <= idx)
	{
		m_Workspace.emplace_back(new Row{m_Attr});
	}
	return iRow(idx, m_Workspace[idx].get());
}

void Table::Scroll(int dir)
{
	if (dir == 0)
	{
		// nothing to do?
		_ASSERTE(dir != 0);
		return;
	}

	DoAutoScroll(dir);
	PromptPosReset();
}

void Table::DoAutoScroll(int dir)
{
	if (dir == 0)
	{
		// nothing to do?
		_ASSERTE(dir != 0);
		return;
	}

	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		throw console_error("Workspace is empty");
	}

	const Region rgn = GetScrollRegion();
	if (rgn.Top > rgn.Bottom)
	{
		_ASSERTE(FALSE && "Region is broken");
		throw console_error("Region is broken");
	}

	// Ensure the workspace has proper depth
	GetRow(rgn.Bottom);
	_ASSERTE(m_Workspace.size() > rgn.Bottom);

	ssize_t src, dst, back_erase = 0;
	const ssize_t step = (dir < 0) ? +1 : -1;
	if (dir < 0)
	{
		src = rgn.Top;
		dst = ssize_t(rgn.Top) + dir;
		_ASSERTE(dst < src);
	}
	else
	{
		src = ssize_t(rgn.Bottom) - dir;
		dst = rgn.Bottom;
		_ASSERTE(dst > src);
	}

	// copy/move rows one by one
	Row dummy;
	for (unsigned counter = (rgn.Bottom - rgn.Top + 1); counter; --counter, src += step, dst += step)
	{
		// workspace or backscroll operation
		if (dst >= ssize_t(rgn.Top) && dst <= ssize_t(rgn.Bottom))
		{
			_ASSERTE((ssize_t)m_Workspace.size() > dst);
			if (src >= ssize_t(rgn.Top) && src <= ssize_t(rgn.Bottom) && size_t(src) < m_Workspace.size())
				m_Workspace[dst] = std::move(m_Workspace[src]);
			else if (src < 0 && size_t(-src) < m_Backscroll.size())
				m_Workspace[dst] = std::move(m_Backscroll[-src - 1]);
		}
		else if (rgn.Top == 0 && m_BackscrollMax != 0 && dir < 0 && dst < 0)
		{
			if (src >= ssize_t(rgn.Top) && src <= ssize_t(rgn.Bottom) && size_t(src) < m_Workspace.size())
				m_Backscroll.emplace_front(std::move(m_Workspace[src]));
			else
				m_Backscroll.emplace_front(new Row{m_Attr});
		}

		// if backscroll operation is allowed
		if (rgn.Top == 0 && m_BackscrollMax != 0 && dir > 0 && src < 0)
			++back_erase;
	}

	if (dst >= ssize_t(rgn.Top) && dst <= ssize_t(rgn.Bottom))
	{
		const unsigned from_row = (step > 0) ? unsigned(dst) : rgn.Top;
		const unsigned to_row = (step > 0) ? rgn.Bottom : unsigned(dst);
		for (unsigned row = from_row; row <= to_row; ++row)
		{
			m_Workspace[row].reset(new Row{m_Attr});
		}
	}

	if (m_BackscrollMax != 0)
	{
		// if rows were pushed from backscroll to workspace
		if (back_erase > 0)
		{
			_ASSERTE(dir > 0);
			m_Backscroll.erase(m_Backscroll.begin(), m_Backscroll.begin() + std::min<size_t>(back_erase, m_Backscroll.size()));
		}
		// validate maximum backscroll lines count
		if (m_BackscrollMax > 0 && m_Backscroll.size() > unsigned(m_BackscrollMax))
		{
			m_Backscroll.erase(m_Backscroll.begin() + m_BackscrollMax, m_Backscroll.end());
		}
	}
}

void Table::LineFeed(bool cr)
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		throw console_error("Workspace is empty");
	}

	const Region rgn = GetScrollRegion();
	Coord cur = GetCursor();

	if (cr)
	{
		cur.x = 0;
	}

	if (cur.y >= (int)rgn.Bottom)
	{
		_ASSERTE(cur.y == (int)rgn.Bottom);
		// scroll scrollable contents upward, insert empty line at the bottom
		// cursor y-position is not changed
		DoAutoScroll(-1);
	}
	else
	{
		_ASSERTE(cur.y >= 0);
		++cur.y;
	}

	ShiftRowIndexes(-1, ShiftPrompt);

	SetCursor(cur);
}

Table::Region Table::GetScrollRegion() const
{
	const auto sz = GetSize();
	Region rgn = {};
	rgn.Top = (m_Flags.Region && m_Region.Top <= m_Region.Bottom)
		? std::max<int>(0, std::min<int>(m_Region.Top, sz.y - 1))
		: 0;
	rgn.Bottom = (m_Flags.Region && m_Region.Top <= m_Region.Bottom)
		? std::max<int>(rgn.Top, std::min<int>(m_Region.Bottom, sz.y - 1))
		: sz.y - 1;
	_ASSERTE(rgn.Top >= 0 && rgn.Bottom >= rgn.Top);
	return rgn;
}

unsigned Table::StoreGrapheme(const std::wstring& text)
{
	if (text.empty() || text.size() > 64) return 0;
	const auto known = m_GraphemeLookup.find(text);
	if (known != m_GraphemeLookup.end()) return known->second;
	if (m_Graphemes.size() >= 8192)
	{
		std::set<unsigned> used;
		auto scan = [&](const Row* row) {
			for (unsigned x = 0; row && x < row->GetLength(); ++x)
			{
				Attribute attr; row->GetCellAttr(x, attr);
				if (attr.graphemeId) used.insert(attr.graphemeId);
			}
		};
		for (const auto& row : m_Backscroll) scan(row.get());
		for (const auto& row : m_Workspace) scan(row.get());
		for (auto i = m_Graphemes.begin(); i != m_Graphemes.end();)
			if (!used.count(i->first)) { m_GraphemeLookup.erase(i->second); i = m_Graphemes.erase(i); } else ++i;
	}
	if (m_Graphemes.size() >= 8192) return 0;
	static std::atomic_uint64_t next{1};
	const auto candidate = next++;
	if (candidate > 0xFFFFFFFFu) return 0;
	const unsigned id = static_cast<unsigned>(candidate);
	m_Graphemes.emplace(id, text);
	m_GraphemeLookup.emplace(text, id);
	return id;
}

bool Table::AppendGrapheme(const std::wstring& text, unsigned codepoint)
{
	if (!m_Cursor.x) return false;
	unsigned x = m_Cursor.x - 1;
	Row* row = &*CurrentRow();
	Attribute attr; row->GetCellAttr(x, attr);
	if ((attr.attr.attr & COMMON_LVB_TRAILING_BYTE) && x) { --x; row->GetCellAttr(x, attr); }
	const unsigned oldWidth = (attr.attr.attr & COMMON_LVB_LEADING_BYTE) ? 2 : 1;
	std::wstring previous;
	const auto stored = m_Graphemes.find(attr.graphemeId);
	if (stored != m_Graphemes.end()) previous = stored->second;
	else previous.assign(1, row->GetCellChar(x));
	if (previous.empty()) return false;
	std::vector<unsigned> points;
	for (size_t i = 0; i < previous.size(); ++i)
	{
		unsigned cp = previous[i];
		if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < previous.size())
			cp = 0x10000 + ((cp - 0xD800) << 10) + previous[++i] - 0xDC00;
		points.push_back(cp);
	}
	if (conpty_unicode::BreakBefore(points, codepoint)) return false;
	previous += text;
	const unsigned id = StoreGrapheme(previous);
	if (!id) return false;
	const unsigned width = oldWidth == 2 || (codepoint >= 0x1F000 && codepoint <= 0x1FAFF) || codepoint == 0xFE0F || codepoint == 0x20E3 ? 2 : 1;
	if (x + width > m_Size.x)
	{
		row->Delete(x, row->GetLength() - x);
		row->SoftWrapped = true; row->SoftWrapLength = x;
		LineFeed(true);
		row = &*CurrentRow(); x = 0;
	}
	attr.graphemeId = id;
	attr.attr.attr &= ~(COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE);
	if (width == 2) attr.attr.attr |= COMMON_LVB_LEADING_BYTE;
	row->Write(x, attr, L'\x25A1', 1);
	if (width == 2)
	{
		attr.attr.attr &= ~COMMON_LVB_LEADING_BYTE; attr.attr.attr |= COMMON_LVB_TRAILING_BYTE;
		row->Write(x + 1, attr, L' ', 1);
	}
	SetCursor({x + width, m_Cursor.y});
	return true;
}

void Table::Write(const wchar_t* text, unsigned count)
{
	if (!text || !count)
		return;
	if (m_ExtendedUnicode && m_PendingHigh)
	{
		const wchar_t pair[2] = {m_PendingHigh, text[0]};
		m_PendingHigh = 0;
		if (text[0] >= 0xDC00 && text[0] <= 0xDFFF)
		{
			Write(pair, 2); ++text; --count;
		}
		else { const wchar_t replacement = 0xFFFD; Write(&replacement, 1); }
		if (!count) return;
	}

	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}
	_ASSERTE(m_Size.x >= 1 && m_Size.y >= 1);


	#ifdef _DEBUG
	const Coord prev_cur = m_Cursor;
	const Coord prev_size = m_Size;
	const Region rgn = GetScrollRegion();
	#endif


	// Iterate characters
	for (unsigned pos = 0; pos < count; ++pos)
	{
		switch (text[pos])
		{
		case L'\t':
			_ASSERTE(!m_Flags.AutoWrap || m_Flags.BeyondEOL || m_Cursor.x < m_Size.x);
			// #condata Store in the row attribute that '\t' was "written" at this place?
			SetCursor({std::min<unsigned>(m_Size.x - 1, ((m_Cursor.x + 8) >> 3) << 3), m_Cursor.y});
			break;
		case L'\r':
			_ASSERTE(!m_Flags.AutoWrap || m_Flags.BeyondEOL || m_Cursor.x < m_Size.x);
			CurrentRow()->SetCR((int)m_Cursor.x);
			// just put cursor at the line start
			SetCursor({0, m_Cursor.y});
			break;
		case L'\n':
			_ASSERTE(!m_Flags.AutoWrap || m_Flags.BeyondEOL || m_Cursor.x < m_Size.x);
			CurrentRow()->SetLF((int)m_Cursor.x);
			LineFeed(m_Flags.AutoCRLF);
			// New line == prompt reset
			PromptPosReset();
			break;
		case 7:
			//Beep (no spacing)
			if (m_BellCallback)
				m_BellCallback(m_BellCallbackParam);
			// cursor position is not changed, we just skip the bell position in text
			break;
		case 8: // L'\b'
			//BS
			SetCursor({m_Cursor.x > 0 ? (m_Cursor.x - 1) : m_Cursor.x, m_Cursor.y});
			break;

		// #condata support '\v' (11) and other codes from https://en.wikipedia.org/wiki/Control_character

		default:
		{
			// Just a letter
			// RealConsole can't have non-spacing glyphs, exceptions are processed above

			// First, ensure the cursor is on allowed place
			SetCursor(m_Cursor);
			// If AutoWrap allowed and we are at EOL (xterm mode)
			const bool surrogate = text[pos] >= 0xD800 && text[pos] <= 0xDBFF
				&& pos + 1 < count && text[pos + 1] >= 0xDC00 && text[pos + 1] <= 0xDFFF;
			if (m_ExtendedUnicode && text[pos] >= 0xD800 && text[pos] <= 0xDBFF && pos + 1 == count)
			{
				m_PendingHigh = text[pos];
				break;
			}
			const unsigned codepoint = surrogate ? 0x10000 + ((text[pos] - 0xD800) << 10) + text[pos + 1] - 0xDC00 : text[pos];
			const std::wstring glyph(text + pos, surrogate ? 2 : 1);
			if (m_ExtendedUnicode && AppendGrapheme(glyph, codepoint))
			{
				if (surrogate) ++pos;
				break;
			}
			unsigned width = (!m_ExtendedUnicode && surrogate) || get_wcwidth(codepoint) == 2 || (codepoint >= 0x1F000 && codepoint <= 0x1FAFF) ? 2 : 1;
			width = std::min(width, m_Size.x);
			if (m_Flags.AutoWrap && m_Cursor.x + width > m_Size.x)
			{
				CurrentRow()->SoftWrapped = true;
				CurrentRow()->SoftWrapLength = m_Cursor.x;
				LineFeed(true);
			}

			// They should not go to the console data (processed above)
			_ASSERTE(text[pos]!=L'\n' && text[pos]!=L'\r' && text[pos]!=L'\t' && text[pos]!=L'\b' && text[pos]!=7/*bell*/);

			if (InsertMode) CurrentRow()->Insert(m_Cursor.x, m_Attr, L' ', width);

			// Extended graphemes have a legacy placeholder plus lossless GUI metadata.
			auto firstAttr = m_Attr;
			if (m_ExtendedUnicode && surrogate) firstAttr.graphemeId = StoreGrapheme(glyph);
			const bool extended = firstAttr.graphemeId != 0;
			if (width == 2 && (!surrogate || extended)) firstAttr.attr.attr |= COMMON_LVB_LEADING_BYTE;
			CurrentRow()->Write(m_Cursor.x, firstAttr, extended ? L'□' : text[pos], 1);
			if (width == 2 && m_Cursor.x + 1 < m_Size.x)
			{
				auto secondAttr = firstAttr;
				secondAttr.attr.attr &= ~COMMON_LVB_LEADING_BYTE;
				if (!surrogate || extended) secondAttr.attr.attr |= COMMON_LVB_TRAILING_BYTE;
				CurrentRow()->Write(m_Cursor.x + 1, secondAttr, extended ? L' ' : (surrogate ? text[pos + 1] : text[pos]), 1);
			}
			if (surrogate)
				++pos;

			// advance cursor position
			SetCursor({m_Cursor.x + width, m_Cursor.y});

			// And in WinApi mode do line feed
			if (m_Flags.AutoWrap && !m_Flags.BeyondEOL && m_Cursor.x >= m_Size.x)
			{
				CurrentRow()->SoftWrapped = true;
				CurrentRow()->SoftWrapLength = m_Cursor.x;
				LineFeed(true);
			}
		}
		}
	}
}

void Table::InsertCell(unsigned count /*= 1*/, const wchar_t chr /*= L' '*/)
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}

	auto row = CurrentRow();
	const auto cur = GetCursor();
	row->Insert(cur.x, m_Attr, chr, count);
}

void Table::DeleteCell(unsigned count /*= 1*/)
{
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}

	auto row = CurrentRow();
	const auto cur = GetCursor();
	// Row::Delete properly does maximum allowed deletion size
	row->Delete(cur.x, count);
}

void Table::InsertRow(unsigned count /*= 1*/)
{
	if (!count)
	{
		return;
	}
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}

	const auto sz = GetSize();
	auto row = CurrentRow();
	decltype(m_Workspace) elements(count);
	for (unsigned i = 0; i < count; ++i) elements[i].reset(new Row{m_Attr});
	m_Workspace.insert(m_Workspace.begin() + row.index, elements.begin(), elements.end());

	if (m_Workspace.size() >= (unsigned)sz.y)
	{
		// These lines go to trash
		m_Workspace.erase(m_Workspace.begin() + sz.y, m_Workspace.end());
	}
}

void Table::DeleteRow(unsigned count /*= 1*/)
{
	if (!count)
	{
		return;
	}
	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}

	auto row = CurrentRow();

	m_Workspace.erase(m_Workspace.begin() + row.index,
		(row.index + count > m_Workspace.size()) ? m_Workspace.end() : (m_Workspace.begin() + (row.index + count)));
}

void Table::GetData(CharInfo *pData, const Coord& bufSize, const RECT& rgn) const
{
	if (!pData || bufSize.x < 1 || bufSize.y < 1)
		throw console_error("Empty buffer passed to Table::GetData");
	if (rgn.right < rgn.left || rgn.bottom < rgn.top)
		return; // nothing to read

	CharInfo* ptr = pData;
	Attribute attr = m_Attr;
	const Row* row = nullptr;
	const unsigned max_x = std::min<unsigned>(bufSize.x, rgn.right - rgn.left + 1);
	const unsigned from_x = (unsigned)std::max<int>(0, rgn.left);
	_ASSERTE(from_x == rgn.left);

	int buf_y = 0;
	for (int y = rgn.top; y <= rgn.bottom && buf_y < bufSize.y; ++y, ++buf_y)
	{
		// Select either backscroll or workspace
		if (y < 0)
		{
			size_t back_idx = size_t(-(y+1));
			if (back_idx < m_Backscroll.size())
				row = m_Backscroll[back_idx].get();
			else
				row = nullptr;
		}
		else
		{
			size_t work_idx = size_t(y);
			if (work_idx < m_Workspace.size())
				row = m_Workspace[work_idx].get();
			else
				row = nullptr;
		}

		// Retrieve default attribute from topmost row
		if (buf_y == 0 && !row)
		{
			if (y < 0 && !m_Backscroll.empty())
			{
				m_Backscroll.back()->GetCellAttr(0, attr);
			}
			else if (!m_Workspace.empty())
			{
				m_Workspace[0]->GetCellAttr(0, attr);
			}
		}

		unsigned x = 0;
		if (row)
		{
			unsigned hint = 0;
			for (; x < max_x; ++x)
			{
				auto& dst = ptr[x];
				dst.chr = row->GetCellChar(x + from_x);
				hint = row->GetCellAttr(x + from_x, attr, hint);
				dst.attr = attr;
				if (x + from_x >= row->GetLength()) { dst.attr.hyperlinkId = 0; dst.attr.graphemeId = 0; }
			}
		}
		for (; x < bufSize.x; ++x)
		{
			auto& dst = ptr[x];
			dst.chr = L' ';
			dst.attr = attr;
			dst.attr.hyperlinkId = 0; dst.attr.graphemeId = 0;
		}
		ptr += bufSize.x;
	}
	// Clean the tail
	for (; buf_y < bufSize.y; ++buf_y)
	{
		for (unsigned x = 0; x < bufSize.x; ++x)
		{
			auto& dst = ptr[x];
			dst.chr = L' ';
			dst.attr = attr;
			dst.attr.hyperlinkId = 0; dst.attr.graphemeId = 0;
		}
		ptr += bufSize.x;
	}
}

void Table::SetBellCallback(BellCallback bellCallback, void* param)
{
	m_BellCallback = bellCallback;
	m_BellCallbackParam = param;
}

void Table::PromptPosStore()
{
	m_Flags.Prompt = true;
	m_Prompt = GetCursor();
}

void Table::PromptPosReset()
{
	m_Flags.Prompt = false;
}

void Table::SetWorkingDirectory(const wchar_t* cwd)
{
	if (!cwd || !*cwd)
		return;

	if (IsEmpty())
	{
		_ASSERTE(FALSE && "Workspace is empty");
		return;
	}
	_ASSERTE(m_Size.x >= 1 && m_Size.y >= 1);

	CurrentRow()->SetWorkingDirectory(cwd);
}

void Table::DebugDump(bool workspace_only /*= false*/) const
{
#ifdef TABLE_DEBUGDUMP
	wchar_t prefix[20];
	MArray<CEStr> dbgout;
	dbgout.push_back(L"\n<<< Table::DebugDump <<< backscroll begin\n");
	for (size_t i = m_Backscroll.size(); !workspace_only && i > 0; --i)
	{
		swprintf_s(prefix, L"%04i: ", -int(i));
		const auto& row = m_Backscroll[i-1];
		dbgout.push_back(CEStr(prefix, row->GetText(), row->HasLineFeed() ? L"\\n" : L"\\0", L"\n"));
	}
	dbgout.push_back(L"<<< Table::DebugDump <<< workspace begin\n");
	for (size_t i = 0; i < m_Workspace.size(); ++i)
	{
		swprintf_s(prefix, L"%04i: ", int(i));
		const auto& row = m_Workspace[i];
		dbgout.push_back(CEStr(prefix, row->GetText(), row->HasLineFeed() ? L"\\n" : L"\\0", L"\n"));
	}
	dbgout.push_back(L"<<< Table::DebugDump <<< end\n\n");
	ssize_t total = 0;
	MArray<ssize_t> lens;
	for (const auto& s : dbgout)
	{
		const ssize_t l = s.GetLen();
		total += l;
		lens.push_back(l);
	}
	CEStr buffer;
	wchar_t* ptr = buffer.GetBuffer(total);
	for (ssize_t i = 0; i < dbgout.size(); ++i)
	{
		const ssize_t l = lens[i];
		wcscpy_s(ptr, l+1, dbgout[i]);
		ptr += l;
	}

	// #condata Fails on very long texts (backscroll > 1000 lines)
	OutputDebugStringW(buffer);
#endif
}

};
