// SPDX-License-Identifier: BSD-3-Clause
#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include "../../common/ConPtyData.h"
int wmain(int argc,wchar_t**argv){if(argc!=5)return 2;FreeConsole();if(!AllocConsole())return 3;ShowWindow(GetConsoleWindow(),SW_HIDE);HANDLE out=GetStdHandle(STD_OUTPUT_HANDLE),in=GetStdHandle(STD_INPUT_HANDLE);SMALL_RECT viewport{0,0,79,24};SetConsoleWindowInfo(out,TRUE,&viewport);SetConsoleScreenBufferSize(out,{80,25});
 std::wstring prefix=argv[4],file=prefix+L".txt";{std::ofstream f(file.c_str());f<<"FIRST_LINE_CONPTY\nsecond\nEmoji: \xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\n";for(int n=4;n<=80;n++)f<<"Line "<<n<<" - full screen sample\n";}
 std::ofstream report((prefix+L".report").c_str());int failures=0;auto check=[&](bool b,const char*n){report<<(b?"PASS ":"FAIL ")<<n<<"\n";report.flush();failures+=!b;};
 auto screen=[&](){CONSOLE_SCREEN_BUFFER_INFO i{};GetConsoleScreenBufferInfo(out,&i);std::vector<wchar_t> chars(i.dwSize.X*i.dwSize.Y);DWORD count;ReadConsoleOutputCharacterW(out,chars.data(),chars.size(),{0,0},&count);return std::wstring(chars.data(),count);};
 auto waitText=[&](const wchar_t*text){for(int n=0;n<200;n++){if(screen().find(text)!=std::wstring::npos)return true;Sleep(50);}return false;};
 auto send=[&](const std::wstring&text){std::vector<INPUT_RECORD> keys(text.size());for(size_t n=0;n<text.size();n++){keys[n].EventType=KEY_EVENT;auto&key=keys[n].Event.KeyEvent;key.bKeyDown=TRUE;key.wRepeatCount=1;key.uChar.UnicodeChar=text[n];key.wVirtualKeyCode=LOBYTE(VkKeyScanW(text[n]));}DWORD count;WriteConsoleInputW(in,keys.data(),keys.size(),&count);};
 std::wstring cmd=L"\""+std::wstring(argv[1])+L"\" /PTY \""+argv[2]+L"\" "+argv[3]+L" \""+file+L"\"";STARTUPINFOW si{sizeof(si)};PROCESS_INFORMATION pi{};if(!CreateProcessW(nullptr,&cmd[0],nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi))return 4;
 bool vim=wcscmp(argv[3],L"vim")==0;check(waitText(L"FIRST_LINE_CONPTY"),"Real fullscreen application displayed file");

 wchar_t mapName[128];swprintf(mapName,128,ConPtyDataName,static_cast<unsigned>(reinterpret_cast<uintptr_t>(GetConsoleWindow())));HANDLE mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,mapName);auto* frame=mapping?static_cast<ConPtySharedData*>(MapViewOfFile(mapping,FILE_MAP_READ,0,0,0)):nullptr;
 bool woman=false,girl=false;for(int attempt=0;frame&&attempt<30&&!(woman&&girl);attempt++){if(!frame->annotation.locked)for(unsigned n=0;n<frame->graphemeCount;n++){auto e=frame->graphemes[n];auto text=std::wstring(frame->graphemeText+e.offset,e.length);woman|=text.find(L"\U0001F469")!=std::wstring::npos;girl|=text.find(L"\U0001F467")!=std::wstring::npos;}if(!(woman&&girl))Sleep(30);}check(woman&&girl,"Supplementary Unicode retained from real fullscreen application");if(frame)UnmapViewOfFile(frame);if(mapping)CloseHandle(mapping);
 if(vim){send(L"j0iEDITED_\x1b:wq\r");}else{send(L" ");Sleep(500);auto text=screen();check(text.find(L"Line 30")!=std::wstring::npos,"less page-down updates visible screen");send(L"q");}
 DWORD wait=WaitForSingleObject(pi.hProcess,15000),code;GetExitCodeProcess(pi.hProcess,&code);if(wait!=WAIT_OBJECT_0)TerminateProcess(pi.hProcess,99);check(wait==WAIT_OBJECT_0&&code==0,"Fullscreen application exits through ConPTY input");check(waitText(L"AFTER_APP")&&screen().find(L"BEFORE_APP")!=std::wstring::npos,"Main screen restored after fullscreen application");
 if(vim){std::ifstream f(file.c_str());std::string text((std::istreambuf_iterator<char>(f)),{});check(text.find("EDITED_second")!=std::string::npos,"Vim edit saved to file");}
 CloseHandle(pi.hProcess);CloseHandle(pi.hThread);return failures?1:0;}
