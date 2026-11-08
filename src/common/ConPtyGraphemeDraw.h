// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include <windows.h>
#include <usp10.h>
#include <string>

// Shape a whole grapheme rather than drawing UTF-16 units independently.
inline bool DrawConPtyGrapheme(HDC dc, const RECT& rect, const std::wstring& text,
	COLORREF foreground, COLORREF background, int fontHeight, HFONT cachedEmojiFont = nullptr)
{
	static const HMODULE usp = LoadLibraryW(L"usp10.dll");
	static const auto analyse = reinterpret_cast<decltype(&ScriptStringAnalyse)>(GetProcAddress(usp, "ScriptStringAnalyse"));
	static const auto draw = reinterpret_cast<decltype(&ScriptStringOut)>(GetProcAddress(usp, "ScriptStringOut"));
	static const auto release = reinterpret_cast<decltype(&ScriptStringFree)>(GetProcAddress(usp, "ScriptStringFree"));
	if (!analyse || !draw || !release || text.empty()) return false;
	std::wstring shaped = text;
	using Normalize = int (WINAPI*)(int, LPCWSTR, int, LPWSTR, int);
	static const auto normalize = reinterpret_cast<Normalize>(GetProcAddress(LoadLibraryW(L"normaliz.dll"), "NormalizeString"));
	wchar_t normalized[128];
	const int normalizedLength = normalize ? normalize(1, text.data(), static_cast<int>(text.size()), normalized, 128) : 0;
	if (normalizedLength > 0) shaped.assign(normalized, normalizedLength);
	const int saved = SaveDC(dc);
	if (!saved) return false;
	HFONT emojiFont = nullptr;
	bool emoji = false;
	for (const auto ch : text) emoji |= (ch >= 0xD800 && ch <= 0xDBFF) || ch == 0xFE0F || ch == 0x20E3;
	if (emoji)
	{
		emojiFont = cachedEmojiFont ? cachedEmojiFont : CreateFontW(-fontHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI Emoji");
		if (emojiFont) SelectObject(dc, emojiFont);
	}
	SetTextColor(dc, foreground); SetBkColor(dc, background); SetBkMode(dc, TRANSPARENT);
	SCRIPT_STRING_ANALYSIS analysis = nullptr;
	const HRESULT result = analyse(dc, shaped.data(), static_cast<int>(shaped.size()), static_cast<int>(shaped.size() * 3 / 2 + 16),
		-1, SSA_GLYPHS | SSA_FALLBACK | SSA_LINK, rect.right - rect.left,
		nullptr, nullptr, nullptr, nullptr, nullptr, &analysis);
	bool painted = false;
	if (SUCCEEDED(result))
	{
		const HBRUSH brush = CreateSolidBrush(background);
		FillRect(dc, &rect, brush); DeleteObject(brush);
		painted = SUCCEEDED(draw(analysis, rect.left, rect.top, ETO_CLIPPED, &rect, 0, 0, FALSE));
		release(&analysis);
	}
	RestoreDC(dc, saved);
	if (emojiFont && !cachedEmojiFont) DeleteObject(emojiFont);
	return painted;
}
