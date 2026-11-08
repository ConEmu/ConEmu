// SPDX-License-Identifier: BSD-3-Clause
#include <windows.h>

#include <algorithm>
#include <fstream>
#include <shellapi.h>
#include <string>
#include <tlhelp32.h>
#include <vector>
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#include "../../common/ConPtyData.h"
// AllocConsole may be delegated to a terminal's pseudoconsole. Such a console
// accepts buffer-size requests but retains only the viewport, so it cannot be
// used to validate the physical scrollback bridge. Run in an explicit conhost.
int RunInConhost(wchar_t **argv)
{
    wchar_t executable[32768], systemDirectory[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, executable, countof(executable)) ||
        !GetSystemDirectoryW(systemDirectory, countof(systemDirectory)))
        return 3;
    std::wstring host = std::wstring(systemDirectory) + L"\\conhost.exe";
    // WOW64's system directory may not contain conhost (Windows ARM64).
    if (GetFileAttributesW(host.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        wchar_t windows[MAX_PATH];
        if (!GetWindowsDirectoryW(windows, countof(windows))) return 3;
        host = std::wstring(windows) + L"\\Sysnative\\conhost.exe";
        if (GetFileAttributesW(host.c_str()) == INVALID_FILE_ATTRIBUTES) return 3;
    }
    const std::wstring arguments = L"-- \"" + std::wstring(executable) + L"\" \"" + argv[1] + L"\" \"" + argv[2] +
                                   L"\" \"" + argv[3] + L"\" --conhost";
    const std::wstring exitFile = std::wstring(argv[3]) + L".exit";
    DeleteFileW(exitFile.c_str());
    SHELLEXECUTEINFOW launch{sizeof(launch)};
    launch.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    launch.lpFile = host.c_str();
    launch.lpParameters = arguments.c_str();
    launch.nShow = SW_HIDE;
    if (!ShellExecuteExW(&launch) || !launch.hProcess)
        return 3;
    DWORD code = 3;
    if (WaitForSingleObject(launch.hProcess, 120000) == WAIT_OBJECT_0)
        GetExitCodeProcess(launch.hProcess, &code);
    else
    {
        TerminateProcess(launch.hProcess, ERROR_TIMEOUT);
        code = ERROR_TIMEOUT;
    }
    CloseHandle(launch.hProcess);
    // conhost itself may exit successfully even when the hosted executable fails.
    // Read the fixture's own result instead of treating the host exit as a pass.
    if (code == ERROR_TIMEOUT)
        return static_cast<int>(code);
    std::ifstream result(exitFile.c_str());
    int childCode = ERROR_INVALID_DATA;
    if (!(result >> childCode))
        return ERROR_INVALID_DATA;
    return childCode;
}
int RunHistoryTest(wchar_t **argv)
{
    if (!GetConsoleWindow())
        return 3;
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    SMALL_RECT viewport{0, 0, 79, 24};
    SetConsoleWindowInfo(out, TRUE, &viewport);
    SetConsoleScreenBufferSize(out, {80, 25});
    std::wstring prefix = argv[3];
    std::ofstream report((prefix + L".report").c_str());
    int failures = 0;
    auto check = [&](bool b, const char *s) {
        report << (b ? "PASS " : "FAIL ") << s << "\n";
        report.flush();
        if (!b)
            failures++;
    };
    auto gate = [&](const wchar_t *s) {
        std::ofstream f((prefix + s).c_str());
        f << "go";
    };
    auto ready = [&](const wchar_t *s) {
        for (int i = 0; i < 600; i++)
        {
            if (GetFileAttributesW((prefix + s + L".ready").c_str()) != INVALID_FILE_ATTRIBUTES)
            {
                Sleep(200);
                return true;
            }
            Sleep(20);
        }
        return false;
    };
    std::wstring command = L"\"" + std::wstring(argv[1]) + L"\" /PTY \"" + argv[2] + L"\" \"" + prefix + L"\"";
    STARTUPINFOW si{sizeof(si)};
    PROCESS_INFORMATION pi{};
    if (!CreateProcessW(nullptr, &command[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
        return 4;
    check(ready(L".initial"), "History child ready");
    wchar_t name[128];
    swprintf(name, 128, ConPtyDataName, static_cast<unsigned>(reinterpret_cast<uintptr_t>(GetConsoleWindow())));
    HANDLE map = OpenFileMappingW(FILE_MAP_READ, FALSE, name);
    auto *data = map ? static_cast<ConPtySharedData *>(MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0)) : nullptr;
    check(data != nullptr, "Shared history and hyperlink frame available");
    check(data && data->version == ConPtyDataVersion, "Matching ConPTY frame protocol");
    HANDLE modules = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pi.dwProcessId);
    MODULEENTRY32W module{};
    module.dwSize = sizeof(module);
    bool inspected = false, hasHook = false, hasServer = false;
    if (modules != INVALID_HANDLE_VALUE)
    {
        if (Module32FirstW(modules, &module))
            do
            {
                inspected = true;
                hasHook |= _wcsnicmp(module.szModule, L"ConEmuHk", 8) == 0;
                hasServer |= _wcsicmp(module.szModule, L"ConEmuCD64.dll") == 0 ||
                             _wcsicmp(module.szModule, L"ConEmuCD.dll") == 0;
            } while (Module32NextW(modules, &module));
        CloseHandle(modules);
    }
    check(inspected && hasServer && !hasHook, "Loaded server has no ConEmuHk");

    auto text = [&]() {
        CONSOLE_SCREEN_BUFFER_INFO i{};
        GetConsoleScreenBufferInfo(out, &i);
        std::vector<wchar_t> buf(i.dwSize.X * i.dwSize.Y);
        DWORD read;
        ReadConsoleOutputCharacterW(out, buf.data(), buf.size(), {0, 0}, &read);
        return std::wstring(buf.data(), read);
    };
    CONSOLE_SCREEN_BUFFER_INFO info{};
    GetConsoleScreenBufferInfo(out, &info);
    check(info.dwSize.Y > 25 && info.srWindow.Top > 0, "Physical console retains scrollback");
    check(text().find(L"ROW0000") != std::wstring::npos && text().find(L"ROW0099") != std::wstring::npos,
          "Old and newest output retained");
    if (data)
    {
        check(data->cells[0].fg_valid && data->cells[0].fg_color == RGB(12, 34, 56) &&
                  data->cells[0].bk_color == RGB(65, 43, 21),
              "History retains exact TrueColor");
        std::wstring uri;
        COORD a{}, b{};
        check(ResolveConPtyLink(data, {8, 0}, uri, a, b) && uri == L"https://example.com/docs?test=1" && a.X == 8 &&
                  b.X == 16,
              "OSC8 ST target resolves from label in history");
        check(!ResolveConPtyLink(data, {19, 0}, uri, a, b), "OSC8 close excludes following text");
        auto screen = text();
        auto pos = screen.find(L"BELLINK");
        check(pos != std::wstring::npos &&
                  ResolveConPtyLink(data, {static_cast<SHORT>(pos % 80), static_cast<SHORT>(pos / 80)}, uri, a, b) &&
                  uri == L"https://example.com/bel" && b.X - a.X == 6,
              "Fragmented BEL hyperlink survives SGR reset");
        pos = screen.find(L"UNSAFE");
        check(pos != std::wstring::npos &&
                  !ResolveConPtyLink(data, {static_cast<SHORT>(pos % 80), static_cast<SHORT>(pos / 80)}, uri, a, b),
              "Unsafe OSC8 scheme rejected");
    }
    check(!IsConPtyUriAllowed(L"file://remote-server/share/file") && IsConPtyUriAllowed(L"file:///C:/Windows"),
          "Local file links allowed and remote shares rejected");
    check(!IsConPtyUriAllowed(L"file:////remote-server/share/file")
              && !IsConPtyUriAllowed(L"file:///%5c%5cremote-server/share/file")
              && !IsConPtyUriAllowed(L"file:///C:/%2f%2fremote-server/share"),
          "Empty-host and encoded-separator UNC disguises rejected");
    gate(L".bulk");
    check(ready(L".bulk"), "Large output completed");
    GetConsoleScreenBufferInfo(out, &info);
    auto saved = text();
    check(info.dwSize.Y == 2025 && saved.find(L"ROW0000") == std::wstring::npos &&
              saved.find(L"BULK2099") != std::wstring::npos,
          "History bounded to 2000 rows and newest retained");
    gate(L".alt");
    check(ready(L".alt"), "Alternate screen entered");
    GetConsoleScreenBufferInfo(out, &info);
    check(info.dwSize.Y == 25 && text().find(L"ALTERNATE") == 0, "Alternate screen has no main history");
    gate(L".restore");
    check(ready(L".restore"), "Main screen restored");
    check(text() == saved, "Alternate screen restores main history and content");
    gate(L".clear");
    check(ready(L".clear"), "Clear scrollback completed");
    GetConsoleScreenBufferInfo(out, &info);
    check(info.dwSize.Y == 25 && text().find(L"BULK2099") != std::wstring::npos,
          "CSI 3J clears history and preserves live screen");
    gate(L".stop");
    DWORD wait = WaitForSingleObject(pi.hProcess, 10000), code;
    GetExitCodeProcess(pi.hProcess, &code);
    if (wait != WAIT_OBJECT_0 || code != 19) report << "DETAIL wait=" << wait << " exit=" << code << "\n";
    check(wait == WAIT_OBJECT_0 && code == 19, "History session exits normally");
    if (wait != WAIT_OBJECT_0)
        TerminateProcess(pi.hProcess, 99);
    if (data)
        UnmapViewOfFile(data);
    if (map)
        CloseHandle(map);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return failures ? 1 : 0;
}

int wmain(int argc, wchar_t **argv)
{
    if (argc == 4)
        return RunInConhost(argv);
    if (argc != 5 || wcscmp(argv[4], L"--conhost") != 0)
        return 2;
    const int code = RunHistoryTest(argv);
    std::ofstream result((std::wstring(argv[3]) + L".exit").c_str());
    result << code << "\n";
    result.close();
    return result ? code : ERROR_WRITE_FAULT;
}
