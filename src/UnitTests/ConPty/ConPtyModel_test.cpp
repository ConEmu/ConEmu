// SPDX-License-Identifier: BSD-3-Clause
#include <windows.h>
#include <cstdio>
#include <vector>
#include <string>
#include "../../common/Common.h"
#include "../../ConEmuCD/ConData.h"
int wmain(){int failed=0;auto check=[&](bool b,const char*n){printf("%s %s\n",b?"PASS":"FAIL",n);failed+=!b;};
 condata::Table t({7},{8,3},100);t.SetReflow(true);t.Write(L"abcdefghijklmnopqrst",20);t.Resize({5,3});
 std::vector<condata::CharInfo> v(20);t.GetData(v.data(),{5,4},{0,-1,4,2});std::wstring text;for(auto c:v)text+=c.chr;
 check(text==L"abcdefghijklmnopqrst","Soft wrap reflows narrower across history");check(t.GetCursor().x==5&&t.GetCursor().y==2,"Cursor follows logical text after narrowing");
 t.Resize({10,3});v.resize(30);t.GetData(v.data(),{10,3},{0,0,9,2});text.clear();for(auto c:v)text+=c.chr;
 check(text.substr(0,20)==L"abcdefghijklmnopqrst","Soft wrap joins after widening");
 condata::Table hard({7},{8,3},100);hard.SetReflow(true);hard.Write(L"abc\r\ndef",8);hard.Resize({16,3});v.resize(48);hard.GetData(v.data(),{16,3},{0,0,15,2});check(v[16].chr==L'd',"Hard newlines remain separate");

 auto unicode = [&](const wchar_t* input, unsigned expected, const wchar_t* cluster, const char* label) {
  condata::Table u({7},{20,3},100);u.SetReflow(true);u.Write(input,wcslen(input));
  std::vector<condata::CharInfo> data(60);u.GetData(data.data(),{20,3},{0,0,19,2});
  const auto glyph=u.GetGraphemes().find(data[0].attr.graphemeId);
  check(u.GetCursor().x==expected && glyph!=u.GetGraphemes().end() && glyph->second==cluster,label);
 };
 unicode(L"e\u0301X",2,L"e\u0301","Combining accent retained in one cell");
 unicode(L"\U0001F600X",3,L"\U0001F600","Emoji surrogate pair retained in two cells");
 unicode(L"\U0001F469\u200D\U0001F467X",3,L"\U0001F469\u200D\U0001F467","ZWJ emoji is one grapheme");
 unicode(L"\U0001F1FA\U0001F1E6X",3,L"\U0001F1FA\U0001F1E6","Regional indicators form one flag");
 unicode(L"\U0001F44D\U0001F3FDX",3,L"\U0001F44D\U0001F3FD","Skin tone joins base emoji");
 unicode(L"\u2764\uFE0FX",3,L"\u2764\uFE0F","Emoji selector expands base to two cells");
 unicode(L"\U00010400X",2,L"\U00010400","Narrow supplementary character occupies one cell");
 condata::Table fragmented({7},{20,3},100);fragmented.SetReflow(true);wchar_t hi=0xD83D,lo=0xDE00;
 fragmented.Write(&hi,1);fragmented.Write(&lo,1);check(fragmented.GetCursor().x==2,"Surrogate pair may span writes");

 condata::Table overwrite({7},{20,3},100);overwrite.SetReflow(true);overwrite.Write(L"\U0001F600",2);overwrite.SetCursor({1,0});overwrite.Write(L"X",1);v.resize(60);overwrite.GetData(v.data(),{20,3},{0,0,19,2});check(v[0].chr==L' '&&v[0].attr.graphemeId==0&&v[1].chr==L'X',"Partial overwrite clears entire wide grapheme");
 condata::Table deletion({7},{20,3},100);deletion.SetReflow(true);deletion.Write(L"\U0001F600X",3);deletion.SetCursor({0,0});deletion.DeleteCell(1);v.resize(60);deletion.GetData(v.data(),{20,3},{0,0,19,2});check(v[0].attr.graphemeId==0&&v[1].chr==L'X',"Partial delete leaves no orphan grapheme");
 return failed?1:0;}
