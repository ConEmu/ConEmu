// SPDX-License-Identifier: BSD-3-Clause
#include <windows.h>
#include <fstream>
#include <string>
int wmain(int argc, wchar_t** argv) {
 if(argc!=2) return 2;
 const std::wstring prefix=argv[1];
 const bool hook=GetModuleHandleW(L"ConEmuHk.dll") || GetModuleHandleW(L"ConEmuHk64.dll");
 {std::ofstream report((prefix+L".modules").c_str());report<<"pid="<<GetCurrentProcessId()<<"\nhook="<<hook<<"\n";}
 DWORD mode=0,n=0;HANDLE output=GetStdHandle(STD_OUTPUT_HANDLE);
 if(GetConsoleMode(output,&mode))SetConsoleMode(output,mode|4);
 const char* text="MODE_PROBE_READY\r\n\x1b[38;2;12;34;56mTRUECOLOR_PROBE\x1b[0m\r\n\x1b]8;;https://example.com/\x1b\\OSC8_PROBE\x1b]8;;\x1b\\\r\n";
 WriteFile(output,text,static_cast<DWORD>(strlen(text)),&n,nullptr);
 for(int i=0;i<600;i++){if(GetFileAttributesW((prefix+L".stop").c_str())!=INVALID_FILE_ATTRIBUTES)return 0;Sleep(100);}
 return 3;
}
