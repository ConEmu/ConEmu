// SPDX-License-Identifier: BSD-3-Clause
#pragma once

#include <windows.h>
#include <shellapi.h>
#include <fstream>
#include <iterator>
#include <string>
#include <cwchar>

// AllocConsole may be delegated to a terminal's pseudoconsole. Such a console
// accepts buffer-size requests but retains only the viewport, so it cannot be
// used to validate the physical scrollback bridge. Run in an explicit conhost.
inline int RunInConhost(int argc, wchar_t **argv)
{
    wchar_t executable[32768], systemDirectory[MAX_PATH];
    if (!GetModuleFileNameW(nullptr, executable, std::size(executable)) ||
        !GetSystemDirectoryW(systemDirectory, std::size(systemDirectory)))
        return 3;
    std::wstring host = std::wstring(systemDirectory) + L"\\conhost.exe";
    // WOW64's system directory may not contain conhost (Windows ARM64).
    if (GetFileAttributesW(host.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        wchar_t windows[MAX_PATH];
        if (!GetWindowsDirectoryW(windows, std::size(windows))) return 3;
        host = std::wstring(windows) + L"\\Sysnative\\conhost.exe";
        if (GetFileAttributesW(host.c_str()) == INVALID_FILE_ATTRIBUTES) return 3;
    }
    std::wstring arguments = L"-- \"" + std::wstring(executable) + L"\"";
    for (int i = 1; i < argc; ++i)
        arguments += L" \"" + std::wstring(argv[i]) + L"\"";
    arguments += L" --conhost";
    const std::wstring exitFile = std::wstring(argv[argc - 1]) + L".exit";
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
inline int RunConPtyConsoleTest(int argc, wchar_t **argv, int argumentCount,
    int (*test)(wchar_t **))
{
    if (argc == argumentCount)
        return RunInConhost(argc, argv);
    if (argc != argumentCount + 1 || std::wcscmp(argv[argc - 1], L"--conhost") != 0)
        return 2;

    const int code = GetConsoleWindow() ? test(argv) : 3;
    std::ofstream result((std::wstring(argv[argc - 2]) + L".exit").c_str());
    result << code;
    return code;
}
