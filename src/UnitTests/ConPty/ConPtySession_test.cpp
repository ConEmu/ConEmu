// SPDX-License-Identifier: BSD-3-Clause
#include <cstdio>
#include <fstream>
#include <string>
#include <windows.h>
int wmain(int argc, wchar_t **argv)
{
    if (argc != 4)
        return 2;
    for (const auto suffix : {L".ready", L".size", L".input", L".dsr"})
        DeleteFileW((std::wstring(argv[3]) + suffix).c_str());
    FreeConsole();
    if (!AllocConsole())
        return 3;
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE), inputHandle = GetStdHandle(STD_INPUT_HANDLE);
    SMALL_RECT initial{0, 0, 79, 24};
    SetConsoleWindowInfo(outputHandle, TRUE, &initial);
    SetConsoleScreenBufferSize(outputHandle, {80, 25});
    DWORD original;
    GetConsoleMode(inputHandle, &original);
    UINT originalCP = GetConsoleCP();
    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\" /PTY \"" + argv[2] + L"\" \"" + argv[3] + L"\"";
    STARTUPINFOW startupInfo{sizeof(startupInfo)};
    PROCESS_INFORMATION sessionProcess{};
    if (!CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startupInfo,
                        &sessionProcess))
        return 4;
    std::ofstream report((std::wstring(argv[3]) + L".report").c_str());
    int failures = 0;
    auto check = [&](bool ok, const char *name) {
        report << (ok ? "PASS " : "FAIL ") << name << "\n";
        report.flush();
        if (!ok)
            failures++;
    };
    const auto ready = std::wstring(argv[3]) + L".ready";
    for (int i = 0; i < 150 && GetFileAttributesW(ready.c_str()) == INVALID_FILE_ATTRIBUTES; i++)
        Sleep(20);
    check(GetFileAttributesW(ready.c_str()) != INVALID_FILE_ATTRIBUTES, "ConPTY child started and wrote output");
    Sleep(200);
    CHAR_INFO cells[320]{};
    SMALL_RECT rect{0, 0, 79, 3};
    ReadConsoleOutputW(outputHandle, cells, {80, 4}, {0, 0}, &rect);
    check(cells[0].Char.UnicodeChar == L'R' && (cells[0].Attributes & 15) == 4, "ANSI red rendered through ConPTY");
    check(cells[2].Char.UnicodeChar == L'T', "TrueColor text preserved");
    check(cells[86].Char.UnicodeChar == 0x041f && cells[91].Char.UnicodeChar == 0x0442, "Fragmented UTF8 decoded");
    check(cells[165].Char.UnicodeChar == 0x4e2d && cells[167].Char.UnicodeChar == L'X',
          "Wide CJK glyph occupies two cells");
    wchar_t title[100]{};
    GetConsoleTitleW(title, 100);
    check(std::wstring(title) == L"ConPTY title test", "OSC title reached outer console");
    SetConsoleScreenBufferSize(outputHandle, {90, 30});
    SMALL_RECT resized{0, 0, 89, 29};
    SetConsoleWindowInfo(outputHandle, TRUE, &resized);
    const auto responseFile = std::wstring(argv[3]) + L".dsr";
    for (int attempt = 0; attempt < 300 && GetFileAttributesW(responseFile.c_str()) == INVALID_FILE_ATTRIBUTES;
         ++attempt)
        Sleep(10);
    INPUT_RECORD key{};
    key.EventType = KEY_EVENT;
    key.Event.KeyEvent.bKeyDown = TRUE;
    key.Event.KeyEvent.wRepeatCount = 1;
    key.Event.KeyEvent.uChar.UnicodeChar = L'Q';
    key.Event.KeyEvent.wVirtualKeyCode = 'Q';
    DWORD written;
    WriteConsoleInputW(inputHandle, &key, 1, &written);
    DWORD wait = WaitForSingleObject(sessionProcess.hProcess, 10000), exitCode = 0;
    if (wait != WAIT_OBJECT_0)
        TerminateProcess(sessionProcess.hProcess, 99);
    GetExitCodeProcess(sessionProcess.hProcess, &exitCode);
    check(wait == WAIT_OBJECT_0, "Session shuts down without hanging");
    check(exitCode == 17, "Input forwarded and child exit code preserved");
    std::ifstream size((std::wstring(argv[3]) + L".size").c_str());
    int x = 0, y = 0;
    size >> x >> y;
    check(x == 90 && y == 30, "Resize reached pseudoconsole");
    DWORD restored;
    GetConsoleMode(inputHandle, &restored);
    check(restored == original && GetConsoleCP() == originalCP, "Console input mode and codepage restored");
    std::ifstream dsr((std::wstring(argv[3]) + L".dsr").c_str());
    std::string dsrText((std::istreambuf_iterator<char>(dsr)), {});
    check(!dsrText.empty() && dsrText[0] == 27 && dsrText.back() == 'R',
          "Cursor position query answered through ConPTY input");
    auto runCommand = [&](const std::wstring &args, DWORD expected) {
        std::wstring line = L"\"" + std::wstring(argv[1]) + L"\" /PTY " + args;
        STARTUPINFOW startup{sizeof(startup)};
        PROCESS_INFORMATION process{};
        if (!CreateProcessW(nullptr, &line[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup, &process))
            return false;
        DWORD wait = WaitForSingleObject(process.hProcess, 10000), code = 0;
        if (wait != WAIT_OBJECT_0)
            TerminateProcess(process.hProcess, 99);
        GetExitCodeProcess(process.hProcess, &code);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return wait == WAIT_OBJECT_0 && code == expected;
    };
    check(runCommand(L"C:\\nonexistent-ConPTY-test-program.exe", ERROR_FILE_NOT_FOUND),
          "Failed startup closes ConPTY and returns error");
    GetConsoleMode(inputHandle, &restored);
    check(restored == original && GetConsoleCP() == originalCP, "Failed startup restores input mode and codepage");
    check(runCommand(L"cmd.exe /d /c \"echo TAIL&exit /b 23\"", 23), "Fast child exit code preserved");
    CHAR_INFO tail[8]{};
    CONSOLE_SCREEN_BUFFER_INFO finalInfo{};
    GetConsoleScreenBufferInfo(outputHandle, &finalInfo);
    SMALL_RECT tailRect{0, finalInfo.srWindow.Top, 7, finalInfo.srWindow.Top};
    ReadConsoleOutputW(outputHandle, tail, {8, 1}, {0, 0}, &tailRect);
    check(tail[0].Char.UnicodeChar == L'T' && tail[3].Char.UnicodeChar == L'L',
          "Final output drained before session shutdown");
    const auto descendantFile = std::wstring(argv[3]) + L".descendant";
    check(runCommand(L"\"" + std::wstring(argv[2]) + L"\" --spawn \"" + descendantFile + L"\"", 0),
          "Child can launch a descendant");
    std::ifstream processId(descendantFile.c_str());
    DWORD descendantId = 0;
    processId >> descendantId;
    HANDLE descendant = OpenProcess(SYNCHRONIZE, FALSE, descendantId);
    check(descendantId != 0 && ((!descendant && GetLastError() == ERROR_INVALID_PARAMETER) ||
                                (descendant && WaitForSingleObject(descendant, 2000) == WAIT_OBJECT_0)),
          "Session shutdown terminates remaining descendants");
    if (descendant)
        CloseHandle(descendant);
    CloseHandle(sessionProcess.hThread);
    CloseHandle(sessionProcess.hProcess);
    return failures ? 1 : 0;
}
