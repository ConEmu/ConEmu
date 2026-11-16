// SPDX-License-Identifier: BSD-3-Clause
#pragma once
#include <windows.h>
#include <objbase.h>
#include <cwchar>
#ifdef _MSC_VER
#pragma comment(lib, "ole32.lib")
#endif

// Anonymous pipes cannot use overlapped I/O. Keep the ConPTY-facing read end
// synchronous and make the bridge's write end cancellable under backpressure.
inline bool CreateConPtyInputPipe(HANDLE& read, HANDLE& write)
{
	read = write = nullptr;
	GUID guid;
	if (FAILED(CoCreateGuid(&guid))) return false;
	wchar_t id[40], name[96];
	StringFromGUID2(guid, id, 40);
	std::swprintf(name, 96, L"\\\\.\\pipe\\ConEmu_pty_input_%ls", id);
	HANDLE server = CreateNamedPipeW(name, PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
		PIPE_TYPE_BYTE | PIPE_WAIT | 0x8 /* PIPE_REJECT_REMOTE_CLIENTS */, 1, 4096, 4096, 0, nullptr);
	if (server == INVALID_HANDLE_VALUE) return false;
	OVERLAPPED connect = {};
	connect.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!connect.hEvent) { CloseHandle(server); return false; }
	const BOOL connected = ConnectNamedPipe(server, &connect);
	const DWORD error = connected ? ERROR_SUCCESS : GetLastError();
	HANDLE client = INVALID_HANDLE_VALUE;
	if (connected || error == ERROR_IO_PENDING || error == ERROR_PIPE_CONNECTED)
		client = CreateFileW(name, GENERIC_READ, 0, nullptr, OPEN_EXISTING,
			SECURITY_SQOS_PRESENT | SECURITY_ANONYMOUS, nullptr);
	DWORD transferred = 0;
	bool ok = client != INVALID_HANDLE_VALUE;
	if (error == ERROR_IO_PENDING)
	{
		if (!ok) CancelIoEx(server, &connect);
		ok = GetOverlappedResult(server, &connect, &transferred, TRUE) && ok;
	}
	CloseHandle(connect.hEvent);
	if (!ok)
	{
		if (client != INVALID_HANDLE_VALUE) CloseHandle(client);
		CloseHandle(server);
		return false;
	}
	read = client; write = server;
	return true;
}

inline bool WriteConPtyInput(HANDLE pipe, const char* data, DWORD size, DWORD timeout = 1000)
{
	OVERLAPPED operation = {};
	operation.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
	if (!operation.hEvent) return false;
	DWORD error = ERROR_SUCCESS;
	while (size)
	{
		ResetEvent(operation.hEvent);
		DWORD written = 0;
		if (!WriteFile(pipe, data, size, &written, &operation))
		{
			error = GetLastError();
			if (error != ERROR_IO_PENDING) break;
			const DWORD wait = WaitForSingleObject(operation.hEvent, timeout);
			if (wait != WAIT_OBJECT_0)
			{
				CancelIoEx(pipe, &operation);
				GetOverlappedResult(pipe, &operation, &written, TRUE);
				error = wait == WAIT_TIMEOUT ? ERROR_TIMEOUT : ERROR_OPERATION_ABORTED;
				break;
			}
			if (!GetOverlappedResult(pipe, &operation, &written, FALSE)) { error = GetLastError(); break; }
		}
		if (!written) { error = ERROR_BROKEN_PIPE; break; }
		data += written; size -= written;
		error = ERROR_SUCCESS;
	}
	CloseHandle(operation.hEvent);
	if (error) SetLastError(error);
	return error == ERROR_SUCCESS;
}
