
/*
Copyright (c) 2021-present Maximus5
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

#include "defines.h"
#include "../UnitTests/gtest.h"
#include <vector>

#include "WFiles.h"
#include "MHandle.h"
#include "MStrDup.h"
#include "../ConEmu/TempFile.h"

namespace {
constexpr wchar_t testTempDir[] = L"%TEMP%\\ConEmuTest";
constexpr wchar_t spaces[] = L"                                                                    ";

TempFile WriteBinary(const wchar_t* fileName, const char* data, const DWORD bytesCount)
{
	TempFile tempFile(testTempDir, fileName);
	EXPECT_TRUE(tempFile.IsValid());

	DWORD written = 0;
	const BOOL writeRc = WriteFile(tempFile.GetHandle(), data, bytesCount, &written, nullptr);
	EXPECT_TRUE(writeRc);
	EXPECT_EQ(written, bytesCount);
	EXPECT_TRUE(SetEndOfFile(tempFile.GetHandle()));
	return tempFile;
}

CEStr GetTestFileName()
{
	return CEStr(lstrdupW(::testing::UnitTest::GetInstance()->current_test_info()->name(), CP_ACP), L".txt");
}

}


TEST(WFiles, ReadAnsi)
{
	const std::string twoLinerAnsi{ "Line one\nLonger line two\n" };
	const CEStr expected(lstrdupW(twoLinerAnsi.c_str(), CP_ACP));
	auto tempFile = WriteBinary(GetTestFileName(), twoLinerAnsi.c_str(), static_cast<DWORD>(twoLinerAnsi.size()));
	tempFile.CloseFile();

	CEStr read(spaces);
	DWORD nSize = 0, nErrCode = 0;
	const int rc = ReadTextFile(tempFile.GetFilePath(), (1 << 20), read, nSize, nErrCode);
	EXPECT_EQ(rc, 0);
	EXPECT_EQ(nErrCode, 0);
	EXPECT_EQ(nSize, twoLinerAnsi.size());
	EXPECT_EQ(read.GetLen(), nSize);
	EXPECT_STREQ(read.c_str(), expected.c_str());
}

TEST(WFiles, ReadRawAscii)
{
	// ReSharper disable once StringLiteralTypo
	const std::string asciiChars{ "Abcdefg" };
	constexpr wchar_t expected[] = { 0x6241, 0x6463, 0x6665, L'g', 0 };
	
	auto tempFile = WriteBinary(GetTestFileName(), asciiChars.c_str(), static_cast<DWORD>(asciiChars.size()));
	tempFile.CloseFile();

	CEStr read(spaces);
	DWORD nSize = 0, nErrCode = 0;
	const int rc = ReadTextFile(tempFile.GetFilePath(), (1 << 20), read, nSize, nErrCode, static_cast<DWORD>(-1));
	EXPECT_EQ(rc, 0);
	EXPECT_EQ(nErrCode, 0);
	EXPECT_EQ(nSize, 7);
	EXPECT_EQ(read.GetLen(), 4);
	EXPECT_STREQ(read.c_str(), expected);
	EXPECT_STREQ(reinterpret_cast<const char*>(read.c_str()), asciiChars.c_str());
}

TEST(WFiles, ReadUtf8NoBom)
{
	constexpr unsigned char ansiChars[] = { /*data*/ 0x23,0xC3,0x86,0xCE,0x9E,0xD0,0xA9,0xE6,0x96,0x87,0x2E };
	constexpr wchar_t expected[] = L"#ÆΞЩ文.";

	auto tempFile = WriteBinary(GetTestFileName(), reinterpret_cast<const char*>(ansiChars), countof(ansiChars));
	tempFile.CloseFile();

	CEStr read(spaces);
	DWORD nSize = 0, nErrCode = 0;
	const int rc = ReadTextFile(tempFile.GetFilePath(), (1 << 20), read, nSize, nErrCode, CP_UTF8);
	EXPECT_EQ(rc, 0);
	EXPECT_EQ(nErrCode, 0);
	EXPECT_EQ(nSize, 6);
	EXPECT_EQ(read.GetLen(), 6);
	EXPECT_STREQ(read.c_str(), expected);
}

TEST(WFiles, ReadUtf8WithBom)
{
	constexpr unsigned char ansiChars[] = {  /*bom*/ 0xEF, 0xBB, 0xBF, /*data*/ 0x23,0xC3,0x86,0xCE,0x9E,0xD0,0xA9,0xE6,0x96,0x87,0x2E };
	constexpr wchar_t expected[] = L"#ÆΞЩ文.";

	auto tempFile = WriteBinary(GetTestFileName(), reinterpret_cast<const char*>(ansiChars), countof(ansiChars));
	tempFile.CloseFile();

	CEStr read(spaces);
	DWORD nSize = 0, nErrCode = 0;
	const int rc = ReadTextFile(tempFile.GetFilePath(), (1 << 20), read, nSize, nErrCode, 0 /*autodetect*/);
	EXPECT_EQ(rc, 0);
	EXPECT_EQ(nErrCode, 0);
	EXPECT_EQ(nSize, 6);
	EXPECT_EQ(read.GetLen(), 6);
	EXPECT_STREQ(read.c_str(), expected);
}

TEST(WFiles, ReadUtf16LeNoBom)
{
	constexpr unsigned char ansiChars[] = { /*data*/  0x23,0x00,0xC6,0x00,0x9E,0x03,0x29,0x04,0x87,0x65,0x2E,0x00 };
	constexpr wchar_t expected[] = L"#ÆΞЩ文.";

	auto tempFile = WriteBinary(GetTestFileName(), reinterpret_cast<const char*>(ansiChars), countof(ansiChars));
	tempFile.CloseFile();

	CEStr read(spaces);
	DWORD nSize = 0, nErrCode = 0;
	const int rc = ReadTextFile(tempFile.GetFilePath(), (1 << 20), read, nSize, nErrCode, 1200);
	EXPECT_EQ(rc, 0);
	EXPECT_EQ(nErrCode, 0);
	EXPECT_EQ(nSize, 6);
	EXPECT_EQ(read.GetLen(), 6);
	EXPECT_STREQ(read.c_str(), expected);
}

TEST(WFiles, ReadUtf16LeWithBom)
{
	constexpr unsigned char ansiChars[] = { /*bom*/ 0xFF,0xFE, /*data*/  0x23,0x00,0xC6,0x00,0x9E,0x03,0x29,0x04,0x87,0x65,0x2E,0x00 };
	constexpr wchar_t expected[] = L"#ÆΞЩ文.";

	auto tempFile = WriteBinary(GetTestFileName(), reinterpret_cast<const char*>(ansiChars), countof(ansiChars));
	tempFile.CloseFile();

	CEStr read(spaces);
	DWORD nSize = 0, nErrCode = 0;
	const int rc = ReadTextFile(tempFile.GetFilePath(), (1 << 20), read, nSize, nErrCode, 1200);
	EXPECT_EQ(rc, 0);
	EXPECT_EQ(nErrCode, 0);
	EXPECT_EQ(nSize, 6);
	EXPECT_EQ(read.GetLen(), 6);
	EXPECT_STREQ(read.c_str(), expected);
}

TEST(WFiles, WriteUtf8NoBom)
{
	constexpr wchar_t expected[] = L"#\u00C6\u039E\u0429\u6587.";
	auto file = WriteBinary(GetTestFileName(), "", 0);
	file.CloseFile();

	DWORD error = 0;
	const int written = WriteTextFile(file.GetFilePath(), expected, countof(expected) - 1, CP_UTF8, false, &error);
	ASSERT_EQ(written, 11) << "Windows error: " << error;

	CEStr actual;
	DWORD length = 0;
	ASSERT_EQ(ReadTextFile(file.GetFilePath(), 1024, actual, length, error, CP_UTF8), 0);
	EXPECT_STREQ(actual.c_str(), expected);
}
