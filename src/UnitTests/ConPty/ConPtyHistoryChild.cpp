// SPDX-License-Identifier: BSD-3-Clause
#include <cstdio>
#include <fstream>
#include <string>
#include <windows.h>
int wmain(int argc, wchar_t **argv)
{
    if (argc != 2)
        return 2;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0, n;
    GetConsoleMode(out, &mode);
    SetConsoleMode(out, mode | 4);
    auto emit = [&](const std::string &s) { WriteFile(out, s.data(), static_cast<DWORD>(s.size()), &n, nullptr); };
    auto stage = [&](const wchar_t *name) {
        std::ofstream done((std::wstring(argv[1]) + name + L".ready").c_str());
        done << "ready";
        done.close();
    };
    auto wait = [&](const wchar_t *name) {
        for (int i = 0; i < 12000; i++)
        {
            if (GetFileAttributesW((std::wstring(argv[1]) + name).c_str()) != INVALID_FILE_ATTRIBUTES)
                return true;
            Sleep(50);
        }
        return false;
    };
    emit("\x1b[2J\x1b[H");
    for (int i = 0; i < 100; i++)
    {
        char line[512];
        sprintf(line,
                "\x1b[38;2;12;34;56m\x1b[48;2;65;43;21mROW%04d "
                "\x1b]8;id=docs;https://example.com/docs?test=1\x1b\\Open docs\x1b]8;;\x07 plain\x1b[0m\r\n",
                i);
        emit(line);
    }
    // Fragment the introducer/terminator deliberately.
    for (char c : std::string("\x1b]8;;https://example.com/bel\x07"))
    {
        emit(std::string(1, c));
        Sleep(1);
    }
    emit("BEL\x1b[0mLINK\x1b]8;;\x1b\\ END\r\n\x1b]8;;javascript:alert(1)\x07UNSAFE\x1b]8;;\x07\r\n");
    stage(L".initial");
    if (!wait(L".bulk"))
        return 3;
    for (int i = 0; i < 2100; i++)
    {
        char line[64];
        sprintf(line, "BULK%04d\r\n", i);
        emit(line);
    }
    stage(L".bulk");
    if (!wait(L".alt"))
        return 3;
    emit("\x1b[?1049h\x1b[HALTERNATE");
    stage(L".alt");
    if (!wait(L".restore"))
        return 3;
    emit("\x1b[?1049l");
    stage(L".restore");
    if (!wait(L".clear"))
        return 3;
    emit("\x1b[3J");
    stage(L".clear");
    if (!wait(L".stop"))
        return 3;
    return 19;
}
