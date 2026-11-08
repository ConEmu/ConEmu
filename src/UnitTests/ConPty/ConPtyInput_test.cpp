// SPDX-License-Identifier: BSD-3-Clause
#include <cstdio>
#include <string>
#include "../../common/ConPtyInput.h"

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char* text) { std::printf("%s %s\n", ok ? "PASS" : "FAIL", text); if (!ok) ++failures; };
	HANDLE read = nullptr, write = nullptr;
	check(CreateConPtyInputPipe(read, write), "create cancellable input pipe");
	if (!read || !write) return 1;
	check(WriteConPtyInput(write, "hello", 5), "short input write completes");
	char bytes[5]; DWORD count = 0;
	check(ReadFile(read, bytes, sizeof(bytes), &count, nullptr) && count == 5 && std::string(bytes, 5) == "hello", "read end receives exact bytes");
	std::string flood(1024 * 1024, 'X');
	const auto start = GetTickCount64();
	const bool written = WriteConPtyInput(write, flood.data(), static_cast<DWORD>(flood.size()), 100);
	const DWORD error = GetLastError();
	check(!written && error == ERROR_TIMEOUT && GetTickCount64() - start < 3000, "non-reading child cannot block input indefinitely");
	CloseHandle(read); CloseHandle(write);
	DWORD baseline = 0, after = 0;
	GetProcessHandleCount(GetCurrentProcess(), &baseline);
	for (unsigned i = 0; i < 50; ++i)
	{
		if (!CreateConPtyInputPipe(read, write)) { ++failures; break; }
		CloseHandle(read);
		check(!WriteConPtyInput(write, "X", 1, 100), "broken input pipe fails without hanging");
		CloseHandle(write);
	}
	GetProcessHandleCount(GetCurrentProcess(), &after);
	check(after <= baseline + 1, "50 failed writes leave no pipe/event handle growth");
	return failures ? 1 : 0;
}
