// SPDX-License-Identifier: BSD-3-Clause
#include <windows.h>
#include <string>
#include <fstream>
int wmain(int argc,wchar_t**argv){if(argc!=2)return 2;HANDLE out=GetStdHandle(STD_OUTPUT_HANDLE);DWORD mode,n;GetConsoleMode(out,&mode);SetConsoleMode(out,mode|4);std::string text="\x1b[2J\x1b[HREFLOW_MARK:";for(int i=0;i<500;i++)text+=char('A'+i%26);text+="\r\n";
 text+="UNICODE: e\xcc\x81 \xf0\x9f\x98\x80 \xf0\x9f\x91\xa9\xe2\x80\x8d\xf0\x9f\x91\xa7 \xf0\x9f\x87\xba\xf0\x9f\x87\xa6 \xf0\x9f\x91\x8d\xf0\x9f\x8f\xbd X\r\n";
 for(int i=0;i<60;i++)text+="History row "+std::to_string(i)+"\r\n";text+="END_MARK\r\n";WriteFile(out,text.data(),text.size(),&n,nullptr);{std::ofstream f((std::wstring(argv[1])+L".ready").c_str());f<<"ready";}for(int i=0;i<12000;i++){if(GetFileAttributesW((std::wstring(argv[1])+L".stop").c_str())!=INVALID_FILE_ATTRIBUTES)return 0;Sleep(50);}return 3;}
