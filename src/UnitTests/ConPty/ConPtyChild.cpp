// SPDX-License-Identifier: BSD-3-Clause
#include <cstdio>
#include <fstream>
#include <string>
#include <windows.h>
int wmain(int argc, wchar_t **argv)
{
    if (argc == 2 && wcscmp(argv[1], L"--idle") == 0)
    {
        Sleep(INFINITE);
        return 0;
    }
    if (argc == 3 && wcscmp(argv[1], L"--spawn") == 0)
    {
        wchar_t executable[32768];
        GetModuleFileNameW(nullptr, executable, 32768);
        std::wstring command = L"\"" + std::wstring(executable) + L"\" --idle";
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION descendant{};
        if (!CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &descendant))
            return 4;
        std::ofstream processId(argv[2]);
        processId << descendant.dwProcessId;
        CloseHandle(descendant.hThread);
        CloseHandle(descendant.hProcess);
        return 0;
    }
    if (argc != 2)
        return 2;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE), in = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = 0;
    GetConsoleMode(out, &mode);
    SetConsoleMode(out, mode | 4);
    GetConsoleMode(in, &mode);
    SetConsoleMode(in, mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT));
    const std::string text = "\x1b[2J\x1b[H\x1b[31mR\x1b[38;5;202mP\x1b[38;2;12;"
                             "34;56m\x1b[48;2;65;43;21mT\x1b[0m\r\nUTF8: "
                             "\xd0\x9f\xd1\x80\xd0\xb8\xd0\xb2\xd0\xb5\xd1\x82\r"
                             "\nCJK: \xe4\xb8\xadX\r\n\x1b[?1049h\x1b[HALT\x1b"
                             "7\x1b[4;4H\x1b"
                             "8\x1b[?1049l\x1b]2;ConPTY title test\x07";
    for (char c : text)
    {
        DWORD n;
        if (!WriteFile(out, &c, 1, &n, nullptr))
            return 3;
        Sleep(1);
    }
    std::wofstream ready((std::wstring(argv[1]) + L".ready").c_str());
    ready << L"ready";
    ready.close();
    CONSOLE_SCREEN_BUFFER_INFO info{};
    for (int i = 0; i < 300; i++)
    {
        GetConsoleScreenBufferInfo(out, &info);
        if (info.dwSize.X == 90 && info.dwSize.Y == 30)
            break;
        Sleep(10);
    }
    std::ofstream size((std::wstring(argv[1]) + L".size").c_str());
    size << info.dwSize.X << " " << info.dwSize.Y;
    size.close();
    const char query[] = "\x1b[6n";
    DWORD queryWritten;
    WriteFile(out, query, sizeof(query) - 1, &queryWritten, nullptr);
    char response[64]{};
    DWORD responseSize = 0;
    ReadFile(in, response, sizeof(response), &responseSize, nullptr);
    std::ofstream dsr((std::wstring(argv[1]) + L".dsr").c_str());
    dsr.write(response, responseSize);
    dsr.close();
    char input[32]{};
    DWORD count = 0;
    ReadFile(in, input, sizeof(input), &count, nullptr);
    std::ofstream answer((std::wstring(argv[1]) + L".input").c_str());
    answer.write(input, count);
    answer.close();
    return count && input[0] == 'Q' ? 17 : 18;
}
