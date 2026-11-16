
/*
Copyright (c) 2013-present Maximus5
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
Sample

Version:0.9
StartHTML:0000000???
EndHTML:0000000???
StartFragment:0000000???
EndFragment:0000000???
SourceURL:https://e.mail.ru/compose/...
<html>
<body>
<!--StartFragment-->
<span style="color: rgb(255, 255, 255); font-family: 'comic sans ms', sans-serif; font-size: 14px; font-style: normal; font-variant: normal; font-weight: normal; letter-spacing: normal; line-height: normal; orphans: auto; text-align: start; text-indent: 0px; text-transform: none; white-space: normal; widows: auto; word-spacing: 0px; -webkit-text-stroke-width: 0px; background-color: rgb(23, 59, 211); display: inline !important; float: none;">
Text
</span>
<!--EndFragment-->
</body>
</html>

*/

#pragma once

#include "../common/MArray.h"
#include "../common/PaletteColors.h"
#include "../ConEmuCD/ConAnsi.h"

class CFormatCopy
{
protected:
	bool bCopyRawCodes;
	struct txt {
		INT_PTR nLen;
		union
		{
			char* pasz;    // UTF-8: Used if (bCopyRawCodes == false)
			wchar_t* pwsz; // Used if (bCopyRawCodes == true)
			void* ptr;
		};
	};
	MArray<txt> m_Items;
	size_t mn_AllItemsLen;
	char szUTF8[6001]; // (szTemp*3 + \0)
public:
	CFormatCopy();
	virtual ~CFormatCopy();

	virtual HGLOBAL CreateResult() = 0;

	virtual void LineAdd(LPCWSTR asText, const WORD* apAttr, const ConEmu::PaletteColors& pPal, INT_PTR nLen);

	virtual void LineAdd(LPCWSTR asText, const CharAttr* apAttr, INT_PTR nLen);

protected:
	virtual void ParBegin() = 0;
	virtual void ParEnd() = 0;
	virtual void TextAdd(LPCWSTR asText, INT_PTR cchLen, COLORREF crFore, COLORREF crBack, bool Bold = false, bool Italic = false, bool Underline = false) = 0;

	void RawAdd(LPCWSTR asText, INT_PTR cchLen);

	HGLOBAL CreateResultInternal(const char* pszHdr = nullptr, INT_PTR nHdrLen = 0);
};

class CHtmlCopy : public CFormatCopy
{
protected:
	size_t mn_TextStart, mn_TextEnd;
	bool mb_Finalized, mb_ParOpened;
	wchar_t szTemp[2000], szBack[64], szFore[64], szId[8];
	wchar_t szBold[20], szItalic[20], szUnderline[32];
protected:
	LPCWSTR FormatColor(COLORREF clr, wchar_t* rsBuf);;
public:
	CHtmlCopy(bool abPlainHtml, LPCWSTR asBuild, LPCWSTR asFont, int anFontHeight, COLORREF crFore, COLORREF crBack);

	virtual ~CHtmlCopy();

protected:
	void ParBegin() override;
	void ParEnd() override;

	void TextAdd(LPCWSTR asText, INT_PTR cchLen, COLORREF crFore, COLORREF crBack, bool Bold = false, bool Italic = false, bool Underline = false) override;

public:
	HGLOBAL CreateResult() override;
};

class CAnsiCopy : public CFormatCopy
{
protected:
	bool mb_ParOpened;
	wchar_t szSeq[80], szTemp[256];
	const ConEmu::PaletteColors clrPalette;
	bool mb_Bold, mb_Italic, mb_Underline;
protected:
	LPCWSTR FormatColor(COLORREF crFore, COLORREF crBack, bool Bold, bool Italic, bool Underline, wchar_t (&rsBuf)[80]);
public:
	CAnsiCopy(const ConEmu::PaletteColors& pclrPalette, COLORREF crFore, COLORREF crBack);

	virtual ~CAnsiCopy();

protected:
	void ParBegin() override;
	void ParEnd() override;

	void TextAdd(LPCWSTR asText, INT_PTR cchLen, COLORREF crFore, COLORREF crBack, bool Bold = false, bool Italic = false, bool Underline = false) override;

public:
	HGLOBAL CreateResult() override;
};
