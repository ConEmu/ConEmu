
/*
Copyright (c) 2016-present Maximus5
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

#include "defines.h"

#include <functional>
#include <tlhelp32.h>

#include "CmdLine.h"
#include "MArray.h"
#include "WObjects.h"


template <
	typename T,
	BOOL (WINAPI* get_first)(HANDLE h, T* lp),
	BOOL (WINAPI* get_next)(HANDLE h, T* lp)
>
class MToolHelp
{
protected:
	// Arguments for CreateToolhelp32Snapshot
	DWORD m_Flags = 0;
	DWORD m_ByProcessId = 0;
	// CreateToolhelp32Snapshot was called?
	bool mb_Opened = false;
	bool mb_Valid = false;
	// Enumeration was finished, all items are placed in m_Items
	bool mb_EndOfList = false;
	// Enumeration was started
	bool mb_Started = false;
	// Toolhelp handle
	HANDLE mh_Snapshot = nullptr;
	// Internal variable
	T m_Item = {};

protected:
	MArray<T> m_Items = {};
	INT_PTR mn_Position = 0;

	virtual bool Filter(const T& item)
	{
		return true;
	}

	bool getNext()
	{
		if (!mb_Opened)
		{
			if (!Open())
				return false;
		}

		if (!mb_Valid)
		{
			return false;
		}

		if (mb_EndOfList)
		{
			return false;
		}

		_ASSERTE(mh_Snapshot && (mh_Snapshot != INVALID_HANDLE_VALUE));


		for (;;)
		{
			BOOL bRc;

			if (!mb_Started)
			{
				mb_Started = true;
				ZeroStruct(m_Item);
				m_Item.dwSize = static_cast<DWORD>(sizeof(T));
				bRc = get_first(mh_Snapshot, &m_Item);
			}
			else
			{
				bRc = get_next(mh_Snapshot, &m_Item);
			}

			if (!bRc)
			{
				mb_EndOfList = true;
				return false;
			}

			if (Filter(m_Item))
			{
				break;
			}
		}

		m_Items.push_back(m_Item);
		return true;
	}

public:
	explicit MToolHelp(const DWORD dwFlags = 0, const DWORD th32ProcessId = 0)
		: m_Flags(dwFlags)
		, m_ByProcessId(th32ProcessId)
	{
		m_Item.dwSize = static_cast<DWORD>(sizeof(T));
	}

	MToolHelp(const MToolHelp&) = delete;
	MToolHelp(MToolHelp&&) = delete;
	MToolHelp& operator=(const MToolHelp&) = delete;
	MToolHelp& operator=(MToolHelp&&) = delete;

	virtual ~MToolHelp()
	{
		Close();
	}

	void Close()
	{
		SafeCloseHandle(mh_Snapshot);

		ZeroStruct(m_Item);
		m_Item.dwSize = static_cast<DWORD>(sizeof(T));

		mb_Opened = false;
		mb_Valid = false;
		mb_EndOfList = false;
		mb_Started = false;
		mn_Position = 0;

		MArray<T>().swap(m_Items);
	}

	bool Open()
	{
		Close();

		const DWORD startTick = GetTickCount();
		bool doOpen = true;
		DWORD errCode = -1, wait = 0;
		const DWORD maxWait = 5000;
		while (doOpen)
		{
			doOpen = false;
			mh_Snapshot = CreateToolhelp32Snapshot(m_Flags, m_ByProcessId);
			mb_Opened = true;
			mb_Valid = (mh_Snapshot && (mh_Snapshot != INVALID_HANDLE_VALUE));
			if (mb_Valid)
				break; // OK
			_ASSERTE(!mb_EndOfList && !mb_Started && (mn_Position == 0));
			// If the function fails with ERROR_BAD_LENGTH, retry the function until it succeeds.
			if (m_Flags & (TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32))
			{
				errCode = GetLastError();
				if (errCode == ERROR_BAD_LENGTH)
				{
					wait = GetTickCount() - startTick;
					doOpen = (wait <= maxWait);
				}
			}
		}

		return mb_Valid;
	}

	bool Open(const DWORD dwFlags, const DWORD th32ProcessId)
	{
		m_Flags = dwFlags;
		m_ByProcessId = th32ProcessId;
		return Open();
	}

	bool LoadAll()
	{
		while (getNext())
			;
		return (m_Items.size() > 0);
	}

	void Reset()
	{
		mn_Position = 0;
	}

	bool Next(T& result)
	{
		if (mn_Position >= m_Items.size())
		{
			if (!getNext())
			{
				return false;
			}
			_ASSERTE(mn_Position < m_Items.size());
		}

		result = m_Items[mn_Position];
		mn_Position++;
		return true;
	}

	bool Find(const std::function<bool(const T& item)>& compare, T& result)
	{
		T item = {};
		item.dwSize = static_cast<DWORD>(sizeof(T));

		Reset();

		while (Next(item))
		{
			if (compare(item))
			{
				result = item;
				return true;
			}
		}
		return false;
	}
};

class MToolHelpProcess : public MToolHelp<PROCESSENTRY32, Process32First, Process32Next>
{
public:
	MToolHelpProcess()
		: MToolHelp(TH32CS_SNAPPROCESS, 0)
	{
	}

	MToolHelpProcess(const MToolHelpProcess&) = delete;
	MToolHelpProcess(MToolHelpProcess&&) = delete;
	MToolHelpProcess& operator=(const MToolHelpProcess&) = delete;
	MToolHelpProcess& operator=(MToolHelpProcess&&) = delete;

	~MToolHelpProcess() override = default;

	// Inherited
	// bool Next(PROCESSENTRY32* lp);

	using MToolHelp::Find;

	bool Find(DWORD pid, PROCESSENTRY32& result)
	{
		return MToolHelp::Find([pid](const PROCESSENTRY32& item) {return item.th32ProcessID == pid; }, result);
	}

	bool Find(LPCWSTR asExeName, PROCESSENTRY32& result)
	{
		LPCWSTR pszName = PointToName(asExeName);
		if (!pszName || !*pszName)
			return false;
		return MToolHelp::Find([pszName](const PROCESSENTRY32& item)
			{
				const auto* itemName = PointToName(item.szExeFile);
				if (!itemName)
					return false;
				const int iCmp = lstrcmpi(itemName, pszName);
				return (iCmp == 0);
			}, result);
	}

	// Visual Studio Code spawns a lot of children processes
	// So we have a long tree of Code.exe/Code.exe/Code.exe
	// The function will find all first-level forks of processId
	MArray<PROCESSENTRY32> FindForks(const DWORD processId)
	{
		MArray<PROCESSENTRY32> forks;
		CEStr lsSelfExe;
		if (!GetModulePathName(nullptr, lsSelfExe))
			return {};
		const auto* pszName = PointToName(lsSelfExe);
		if (!pszName)
			return {};
		PROCESSENTRY32 dummy = {};
		MToolHelp::Find([pszName, processId, &forks](const PROCESSENTRY32& item)
			{
				// Process only first-level children
				if (item.th32ParentProcessID == processId)
				{
					// Compare names
					const auto* itemName = PointToName(item.szExeFile);
					if (itemName)
					{
						const int iCmp = lstrcmpi(itemName, pszName);
						if (iCmp == 0)
							forks.push_back(item);
					}
				}
				return false; // Process all processes in system...
			}, dummy);
		return forks;
	}
};

class MToolHelpModule final : public MToolHelp<MODULEENTRY32, Module32First, Module32Next>
{
public:
	explicit MToolHelpModule(const DWORD th32ProcessId, const bool SnapModule32 = false)
		: MToolHelp(TH32CS_SNAPMODULE|(SnapModule32 ? TH32CS_SNAPMODULE32 : 0), th32ProcessId)
	{
	}

	MToolHelpModule(const MToolHelpModule&) = delete;
	MToolHelpModule(MToolHelpModule&&) = delete;
	MToolHelpModule& operator=(const MToolHelpModule&) = delete;
	MToolHelpModule& operator=(MToolHelpModule&&) = delete;

	~MToolHelpModule() override = default;

	// Inherited
	// bool Next(MODULEENTRY32* lp);
};

class MToolHelpThread final : public MToolHelp<THREADENTRY32, Thread32First, Thread32Next>
{
public:
	// Unfortunately, Thread32First/Next enumerate all thread in the system
	// We shall filter found threads by THREADENTRY32::th32OwnerProcessID
	explicit MToolHelpThread(const DWORD th32ProcessId)
		: MToolHelp(TH32CS_SNAPTHREAD, th32ProcessId)
	{
	}

	MToolHelpThread(const MToolHelpThread&) = delete;
	MToolHelpThread(MToolHelpThread&&) = delete;
	MToolHelpThread& operator=(const MToolHelpThread&) = delete;
	MToolHelpThread& operator=(MToolHelpThread&&) = delete;

	~MToolHelpThread() override = default;

	// Inherited
	// bool Next(THREADENTRY32* lp);

protected:
	bool Filter(const THREADENTRY32& item) override
	{
		return (item.th32OwnerProcessID == m_ByProcessId);
	}
};
