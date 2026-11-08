// SPDX-License-Identifier: BSD-3-Clause
#include <windows.h>
#include <cstdio>
#include <string>
int wmain(int argc,wchar_t**argv){if(argc!=3)return 2;HANDLE out=GetStdHandle(STD_OUTPUT_HANDLE);DWORD mode,n;GetConsoleMode(out,&mode);SetConsoleMode(out,mode|4);SetEnvironmentVariableW(L"TERM",L"xterm-256color");SetEnvironmentVariableW(L"LESS",nullptr);SetEnvironmentVariableW(L"LESSCHARSET",L"utf-8");
 const char*before="BEFORE_APP\r\n";WriteFile(out,before,strlen(before),&n,nullptr);
 bool vim=wcscmp(argv[1],L"vim")==0;std::wstring cmd=vim?L"\"C:\\Program Files\\Git\\usr\\bin\\vim.exe\" -Nu NONE -n -i NONE ":L"\"C:\\Program Files\\Git\\usr\\bin\\less.exe\" -R -- ";cmd+=L"\""+std::wstring(argv[2])+L"\"";
 STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};if(!CreateProcessW(nullptr,&cmd[0],nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi))return 3;WaitForSingleObject(pi.hProcess,INFINITE);DWORD code;GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hProcess);CloseHandle(pi.hThread);const char*after="AFTER_APP\r\n";WriteFile(out,after,strlen(after),&n,nullptr);return code;}
