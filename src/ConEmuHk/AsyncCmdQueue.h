
/*
Copyright (c) 2021-present Maximus5
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#pragma once

#include <windows.h>

#include "../common/Common.h"
#include "../common/MArray.h"
#include "../common/MEvent.h"
#include "../common/MSectionSimple.h"
#include "../common/MHandle.h"

#include <atomic>
#include <deque>

extern class AsyncCmdQueue* gpAsyncCmdQueue;

class AsyncCmdQueue final
{
public:
	static void Initialize();
	static void Terminate();

	void AsyncExecute(CESERVER_REQ*&& request);
	void Stop();

	void AsyncActivateConsole();

	AsyncCmdQueue(const AsyncCmdQueue&) = delete;
	AsyncCmdQueue(AsyncCmdQueue&&) = delete;
	AsyncCmdQueue& operator=(const AsyncCmdQueue&) = delete;
	AsyncCmdQueue& operator=(AsyncCmdQueue&&) = delete;

protected:
	AsyncCmdQueue();
	~AsyncCmdQueue();

	static DWORD WINAPI Thread(LPVOID lpThreadParameter);
	void Run();
	static void Exec(CESERVER_REQ*& request);

private:
	struct CallData
	{
		CESERVER_REQ* request_{ nullptr };
	};
	std::deque<CallData, MArrayAllocator<CallData>> data_;
	MSectionSimple lock_{ true };
	MEvent event_;
	std::atomic_bool stopped_{ false };
	MHandle threadHandle_;
	DWORD threadId_{ 0 };
};
