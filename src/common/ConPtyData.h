// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <windows.h>
#include "ConsoleAnnotation.h"
#include <algorithm>
#include <string>

constexpr DWORD ConPtyDataVersion = 2;
constexpr unsigned ConPtyCellCapacity = 262144;
constexpr unsigned ConPtyHistoryLimit = 2000;
constexpr unsigned ConPtyLinkCapacity = 4096;
constexpr unsigned ConPtyUriCapacity = 1048576;
constexpr unsigned ConPtyUriMaxLength = 2048;
constexpr unsigned ConPtyGraphemeCapacity = 8192;
constexpr unsigned ConPtyGraphemeChars = 524288;
constexpr wchar_t ConPtyDataName[] = L"ConEmu_ConPTY_%x";

struct ConPtyLinkEntry
{
	DWORD id, offset, length;
};

// Fixed-size, pointer-free protocol shared by the terminal bridge and GUI.
struct ConPtySharedData
{
	AnnotationHeader annotation;
	DWORD version, owner, columns, rows, viewportRows, linkCount, uriChars;
	ConPtyLinkEntry links[ConPtyLinkCapacity];
	wchar_t uris[ConPtyUriCapacity];
	DWORD graphemeCount, graphemeChars;
	ConPtyLinkEntry graphemes[ConPtyGraphemeCapacity];
	wchar_t graphemeText[ConPtyGraphemeChars];
	AnnotationInfo cells[ConPtyCellCapacity];
};

inline bool IsConPtyUriAllowed(const std::wstring& uri)
{
	if (uri.empty() || uri.size() > ConPtyUriMaxLength
		|| std::any_of(uri.begin(), uri.end(), [](wchar_t ch) { return ch < 32 || ch == 127; }))
		return false;
	if (_wcsnicmp(uri.c_str(), L"https://", 8) == 0 || _wcsnicmp(uri.c_str(), L"http://", 7) == 0
		|| _wcsnicmp(uri.c_str(), L"ftp://", 6) == 0 || _wcsnicmp(uri.c_str(), L"mailto:", 7) == 0)
		return true;
	if (_wcsnicmp(uri.c_str(), L"file://", 7) != 0)
		return false;
	const auto slash = uri.find(L'/', 7);
	if (slash == std::wstring::npos)
		return false;
	const auto host = uri.substr(7, slash - 7);
	// An empty host followed by // or encoded separators can still become a UNC
	// path in ShellExecute. Only allow canonical absolute local drive paths.
	if (uri.size() < slash + 4 || !((uri[slash + 1] >= L'A' && uri[slash + 1] <= L'Z')
		|| (uri[slash + 1] >= L'a' && uri[slash + 1] <= L'z'))
		|| uri[slash + 2] != L':' || uri[slash + 3] != L'/' || uri.find(L'\\') != std::wstring::npos)
		return false;
	for (size_t i = slash; i + 2 < uri.size(); ++i)
		if (uri[i] == L'%' && ((uri[i + 1] == L'2' && (uri[i + 2] == L'f' || uri[i + 2] == L'F'))
			|| (uri[i + 1] == L'5' && (uri[i + 2] == L'c' || uri[i + 2] == L'C')))) return false;
	wchar_t computer[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD length = sizeof(computer) / sizeof(computer[0]);
	return host.empty() || _wcsicmp(host.c_str(), L"localhost") == 0
		|| (GetComputerNameW(computer, &length) && _wcsicmp(host.c_str(), computer) == 0);
}

// Resolve a label's target without opening it. Retry if the publisher changes the frame.
inline bool ResolveConPtyLink(const ConPtySharedData* data, COORD position,
	std::wstring& uri, COORD& first, COORD& last)
{
	if (!data || position.X < 0 || position.Y < 0)
		return false;
	for (unsigned attempt = 0; attempt < 3; ++attempt)
	{
		const auto counter = data->annotation.flushCounter;
		const DWORD columns = data->columns, rows = data->rows;
		if (data->annotation.locked || data->version != ConPtyDataVersion || !columns
			|| rows > ConPtyCellCapacity / columns || position.X >= columns || position.Y >= rows
			|| data->linkCount > ConPtyLinkCapacity || data->uriChars > ConPtyUriCapacity)
			return false;
		const size_t index = static_cast<size_t>(position.Y) * columns + position.X;
		const DWORD id = data->cells[index].raw[7];
		if (!id)
			return false;
		const auto entry = std::lower_bound(data->links, data->links + data->linkCount, id,
			[](const ConPtyLinkEntry& link, DWORD value) { return link.id < value; });
		if (entry == data->links + data->linkCount || entry->id != id || entry->length > ConPtyUriMaxLength
			|| entry->offset > data->uriChars || entry->length > data->uriChars - entry->offset)
			return false;
		std::wstring target(data->uris + entry->offset, entry->length);
		size_t begin = index, end = index;
		while (begin && static_cast<DWORD>(data->cells[begin - 1].raw[7]) == id) --begin;
		while (end + 1 < static_cast<size_t>(columns) * rows
			&& static_cast<DWORD>(data->cells[end + 1].raw[7]) == id) ++end;
		MemoryBarrier();
		if (!data->annotation.locked && counter == data->annotation.flushCounter && IsConPtyUriAllowed(target))
		{
			uri = std::move(target);
			first = {static_cast<SHORT>(begin % columns), static_cast<SHORT>(begin / columns)};
			last = {static_cast<SHORT>(end % columns), static_cast<SHORT>(end / columns)};
			return true;
		}
	}
	return false;
}

struct ConPtyGlyph
{
	COORD position;
	unsigned width;
	std::wstring text;
};
