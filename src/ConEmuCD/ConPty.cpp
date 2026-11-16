#include "../common/Common.h"
#include "../common/ConEmuCheck.h"
#include "../common/ConsoleAnnotation.h"
#include "../common/ConPtyData.h"
#include "../common/ConPtyInput.h"
#include "../common/WConsole.h"
#include "ConsoleMain.h"
#include "ConsoleState.h"
#include "ConAnsi.h"
#include "ConPty.h"

#include <atomic>
#include <string>
#include <vector>

namespace {

using CreatePty = HRESULT (WINAPI*)(COORD, HANDLE, HANDLE, DWORD, HANDLE*);
using ResizePty = HRESULT (WINAPI*)(HANDLE, COORD);
using ClosePty = void (WINAPI*)(HANDLE);
constexpr DWORD_PTR PtyAttribute = 0x00020016;

class ConPtySession
{
public:
	~ConPtySession() { Close(); }
	int Run(const wchar_t* command);

private:
	void Close();
	bool Start(const wchar_t* command);
	bool Paint();
	bool CreateSharedData();
	static DWORD WINAPI ReadOutput(void* context);
	static DWORD WINAPI ReadInput(void* context);

	HANDLE input_ = nullptr, output_ = nullptr;
	HANDLE pty_ = nullptr, inputPipe_ = nullptr, outputPipe_ = nullptr;
	HANDLE inputThread_ = nullptr, outputThread_ = nullptr, job_ = nullptr;
	HANDLE guiOwner_ = nullptr, serverOwner_ = nullptr;
	PROCESS_INFORMATION child_ = {};
	ResizePty resizePty_ = nullptr;
	ClosePty closePty_ = nullptr;
	SrvAnsi* ansi_ = nullptr;
	std::atomic_bool stopping_{false};
	std::atomic<DWORD> outputError_{0};
	DWORD inputMode_ = 0;
	UINT inputCodePage_ = 0;
	bool modeChanged_ = false;
	CONSOLE_CURSOR_INFO originalCursor_ = {};
	COORD size_ = {};
	uint64_t frameVersion_ = 0;
	std::vector<condata::CharInfo> frame_;
	std::vector<CHAR_INFO> legacy_;
	HANDLE sharedMap_ = nullptr, sessionMutex_ = nullptr;
	bool ownsSession_ = false;
	ConPtySharedData* shared_ = nullptr;
	unsigned history_ = 0;
	std::map<unsigned, std::wstring> links_, graphemes_;
	std::wstring originalTitle_;
};

bool ConPtySession::Start(const wchar_t* command)
{
	const auto kernel = GetModuleHandleW(L"kernel32.dll");
	const auto createPty = reinterpret_cast<CreatePty>(GetProcAddress(kernel, "CreatePseudoConsole"));
	resizePty_ = reinterpret_cast<ResizePty>(GetProcAddress(kernel, "ResizePseudoConsole"));
	closePty_ = reinterpret_cast<ClosePty>(GetProcAddress(kernel, "ClosePseudoConsole"));
	if (!createPty || !resizePty_ || !closePty_)
	{
		SetLastError(ERROR_NOT_SUPPORTED);
		return false;
	}
	input_ = GetStdHandle(STD_INPUT_HANDLE);
	output_ = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD outputMode = 0;
	CONSOLE_SCREEN_BUFFER_INFO info = {};
	if (!GetConsoleMode(input_, &inputMode_) || !GetConsoleMode(output_, &outputMode)
		|| !GetConsoleScreenBufferInfo(output_, &info))
		return false;
	size_ = {static_cast<SHORT>(info.srWindow.Right - info.srWindow.Left + 1),
		static_cast<SHORT>(info.srWindow.Bottom - info.srWindow.Top + 1)};
	wchar_t title[32768] = {};
	GetConsoleTitleW(title, countof(title));
	originalTitle_ = title;
	GetConsoleCursorInfo(output_, &originalCursor_);
	if (!CreateSharedData())
		return false;

	HANDLE readInput = nullptr, writeOutput = nullptr;
	if (!CreateConPtyInputPipe(readInput, inputPipe_))
		return false;
	if (!CreatePipe(&outputPipe_, &writeOutput, nullptr, 0))
	{
		CloseHandle(readInput);
		return false;
	}
	const HRESULT result = createPty(size_, readInput, writeOutput, 0, &pty_);
	CloseHandle(readInput);
	CloseHandle(writeOutput);
	if (FAILED(result))
	{
		SetLastError(HRESULT_CODE(result));
		return false;
	}

	ansi_ = SrvAnsi::Object();
	ansi_->SetConPtyInput(inputPipe_);
	inputCodePage_ = GetConsoleCP();
	if (!SetConsoleMode(input_, (inputMode_ | ENABLE_VIRTUAL_TERMINAL_INPUT | ENABLE_WINDOW_INPUT)
		& ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE)))
		return false;
	modeChanged_ = true;
	if (!SetConsoleCP(CP_UTF8))
		return false;

	// Drain output before starting the child. Closing ConPTY may wait for this reader.
	outputThread_ = CreateThread(nullptr, 0, ReadOutput, this, 0, nullptr);
	if (!outputThread_)
		return false;

	SIZE_T bytes = 0;
	InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
	std::vector<BYTE> attributes(bytes);
	STARTUPINFOEXW startup = {};
	startup.StartupInfo.cb = sizeof(startup);
	startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributes.data());
	if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &bytes))
		return false;
	bool started = false;
	if (UpdateProcThreadAttribute(startup.lpAttributeList, 0, PtyAttribute, pty_, sizeof(pty_), nullptr, nullptr))
	{
		std::wstring line(command);
		started = CreateProcessW(nullptr, &line[0], nullptr, nullptr, FALSE,
			EXTENDED_STARTUPINFO_PRESENT | CREATE_SUSPENDED, nullptr, nullptr, &startup.StartupInfo, &child_) != FALSE;
	}
	const DWORD startError = GetLastError();
	DeleteProcThreadAttributeList(startup.lpAttributeList);
	if (!started)
	{
		SetLastError(startError);
		return false;
	}
	job_ = CreateJobObjectW(nullptr, nullptr);
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
	limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	if (!job_ || !SetInformationJobObject(job_, JobObjectExtendedLimitInformation, &limits, sizeof(limits))
		|| !AssignProcessToJobObject(job_, child_.hProcess))
		return false;
	inputThread_ = CreateThread(nullptr, 0, ReadInput, this, 0, nullptr);
	if (!inputThread_)
		return false;
	return ResumeThread(child_.hThread) != static_cast<DWORD>(-1);
}

DWORD WINAPI ConPtySession::ReadOutput(void* context)
{
	auto& session = *static_cast<ConPtySession*>(context);
	char bytes[16384];
	DWORD count = 0;
	try
	{
		while (ReadFile(session.outputPipe_, bytes, sizeof(bytes), &count, nullptr) && count)
		{
			if (!session.ansi_->OurWriteConsoleA(bytes, count, nullptr))
			{
				session.outputError_ = ERROR_INVALID_DATA;
				return 1;
			}
		}
		const DWORD error = GetLastError();
		if (error != ERROR_BROKEN_PIPE && error != ERROR_OPERATION_ABORTED)
			session.outputError_ = error;
	}
	catch (...)
	{
		session.outputError_ = ERROR_INVALID_DATA;
	}
	return 0;
}

DWORD WINAPI ConPtySession::ReadInput(void* context)
{
	auto& session = *static_cast<ConPtySession*>(context);
	char bytes[4096];
	DWORD count = 0;
	while (!session.stopping_ && ReadFile(session.input_, bytes, sizeof(bytes), &count, nullptr))
	{
		if (count && !session.ansi_->WriteTerminalInput(bytes, count))
			break;
	}
	return 0;
}

bool ConPtySession::CreateSharedData()
{
	CESERVER_CONSOLE_MAPPING_HDR mapping = {};
	LoadSrvMapping(gState.realConWnd_, mapping);
	DWORD guiPID = 0;
	if (mapping.hConEmuRoot) GetWindowThreadProcessId(mapping.hConEmuRoot, &guiPID);
	if (guiPID && guiPID != GetCurrentProcessId())
	{
		guiOwner_ = OpenProcess(SYNCHRONIZE, FALSE, guiPID);
		if (!guiOwner_) return false;
	}
	if (mapping.nServerPID && mapping.nServerPID != GetCurrentProcessId())
	{
		serverOwner_ = OpenProcess(SYNCHRONIZE, FALSE, mapping.nServerPID);
		if (!serverOwner_) return false;
	}
	const DWORD identity = mapping.hConEmuWndDc ? LODWORD(mapping.hConEmuWndDc) : LODWORD(gState.realConWnd_);
	wchar_t name[128];
	msprintf(name, countof(name), L"ConEmu_ConPTY_owner_%x", identity);
	sessionMutex_ = CreateMutexW(nullptr, FALSE, name);
	if (!sessionMutex_) return false;
	const DWORD wait = WaitForSingleObject(sessionMutex_, 0);
	if (wait != WAIT_OBJECT_0 && wait != WAIT_ABANDONED)
	{
		SetLastError(ERROR_BUSY);
		return false;
	}
	ownsSession_ = true;
	msprintf(name, countof(name), ConPtyDataName, identity);
	sharedMap_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(ConPtySharedData), name);
	if (!sharedMap_) return false;
	shared_ = static_cast<ConPtySharedData*>(MapViewOfFile(sharedMap_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(ConPtySharedData)));
	if (!shared_) return false;
	InterlockedExchange(reinterpret_cast<LONG*>(&shared_->annotation.locked), TRUE);
	shared_->version = ConPtyDataVersion;
	shared_->owner = GetCurrentProcessId();
	shared_->rows = shared_->columns = shared_->linkCount = shared_->uriChars = 0;
	shared_->annotation.struct_size = sizeof(AnnotationHeader);
	shared_->annotation.bufferSize = ConPtyCellCapacity;
	InterlockedIncrement(reinterpret_cast<LONG*>(&shared_->annotation.flushCounter));
	InterlockedExchange(reinterpret_cast<LONG*>(&shared_->annotation.locked), FALSE);
	return true;
}

bool ConPtySession::Paint()
{
	CONSOLE_SCREEN_BUFFER_INFO info = {};
	if (!GetConsoleScreenBufferInfo(output_, &info))
		return false;
	const COORD nextSize = {static_cast<SHORT>(info.srWindow.Right - info.srWindow.Left + 1),
		static_cast<SHORT>(info.srWindow.Bottom - info.srWindow.Top + 1)};
	if (nextSize.X != size_.X || nextSize.Y != size_.Y)
	{
		const HRESULT result = pty_ ? resizePty_(pty_, nextSize) : S_OK;
		if (FAILED(result))
			return false;
		size_ = nextSize;
	}
	COORD cursor;
	if (!ansi_->ReadTerminalFrame(size_, frame_, cursor, frameVersion_, history_, links_, graphemes_))
		return true;
	legacy_.resize(frame_.size());
	const COORD bufferSize = {size_.X, static_cast<SHORT>(size_.Y + history_)};
	// Reserve a blank tail to cross the legacy buffer-mode detection threshold.
	// Dynamic buffer height hides this tail; history still contains only real rows.
	const COORD physicalSize = {size_.X, static_cast<SHORT>(history_
		? std::max<int>(bufferSize.Y, std::min<int>(32767, size_.Y + 256)) : bufferSize.Y)};
	InterlockedExchange(reinterpret_cast<LONG*>(&shared_->annotation.locked), TRUE);
	auto* colors = shared_->cells;
	for (size_t i = 0; i < frame_.size(); ++i)
	{
		const auto& cell = frame_[i];
		legacy_[i].Char.UnicodeChar = cell.chr ? cell.chr : L' ';
		legacy_[i].Attributes = cell.attr.attr.attr;
		// Grapheme cell widths travel in the shared frame. The legacy host must
		// not reinterpret placeholder squares as DBCS pairs and shift neighbours.
		if (cell.attr.graphemeId) legacy_[i].Attributes &= ~(COMMON_LVB_LEADING_BYTE | COMMON_LVB_TRAILING_BYTE);
		if (colors)
		{
			AnnotationInfo color = {};
			color.fg_valid = cell.attr.isFore24b();
			color.fg_color = cell.attr.crForeColor;
			color.bk_valid = cell.attr.isBack24b();
			color.bk_color = cell.attr.crBackColor;
			color.style = (cell.attr.isBold() ? AI_STYLE_BOLD : 0)
				| (cell.attr.isItalic() ? AI_STYLE_ITALIC : 0)
				| (cell.attr.isUnderscore() || cell.attr.hyperlinkId ? AI_STYLE_UNDERLINE : 0);
			color.raw[7] = cell.attr.hyperlinkId;
			color.raw[6] = cell.attr.graphemeId;
			color.raw[5] = (cell.attr.attr.attr & COMMON_LVB_TRAILING_BYTE) ? 0 : (cell.attr.attr.attr & COMMON_LVB_LEADING_BYTE) ? 2 : 1;
			colors[i] = color;
		}
	}
	shared_->columns = bufferSize.X;
	shared_->rows = bufferSize.Y;
	shared_->viewportRows = size_.Y;
	shared_->linkCount = shared_->uriChars = 0;
	for (const auto& link : links_)
	{
		auto& entry = shared_->links[shared_->linkCount++];
		entry = {link.first, shared_->uriChars, static_cast<DWORD>(link.second.size())};
		wmemcpy(shared_->uris + entry.offset, link.second.data(), entry.length);
		shared_->uriChars += entry.length;
	}
	shared_->graphemeCount = shared_->graphemeChars = 0;
	for (const auto& glyph : graphemes_)
	{
		if (shared_->graphemeCount == ConPtyGraphemeCapacity || glyph.second.size() > ConPtyGraphemeChars - shared_->graphemeChars) break;
		auto& entry = shared_->graphemes[shared_->graphemeCount++];
		entry = {glyph.first, shared_->graphemeChars, static_cast<DWORD>(glyph.second.size())};
		wmemcpy(shared_->graphemeText + entry.offset, glyph.second.data(), entry.length);
		shared_->graphemeChars += entry.length;
	}
	SMALL_RECT viewport = {0, static_cast<SHORT>(history_), static_cast<SHORT>(size_.X - 1), static_cast<SHORT>(bufferSize.Y - 1)};
	cursor.Y += history_;
	if (physicalSize.Y < info.dwSize.Y)
	{
		SetConsoleWindowInfo(output_, TRUE, &viewport);
		SetConsoleCursorPosition(output_, cursor);
	}
	bool result = true;
	if (physicalSize.X != info.dwSize.X || physicalSize.Y != info.dwSize.Y)
		result = SetConsoleScreenBufferSize(output_, physicalSize) != FALSE;
	SMALL_RECT region = {0, 0, static_cast<SHORT>(bufferSize.X - 1), static_cast<SHORT>(bufferSize.Y - 1)};
	result = result && WriteConsoleOutputW(output_, legacy_.data(), bufferSize, {0, 0}, &region);
	if (physicalSize.Y > bufferSize.Y)
	{
		DWORD written;
		const DWORD tail = static_cast<DWORD>(physicalSize.Y - bufferSize.Y) * size_.X;
		FillConsoleOutputCharacterW(output_, L' ', tail, {0, bufferSize.Y}, &written);
		FillConsoleOutputAttribute(output_, 7, tail, {0, bufferSize.Y}, &written);
	}
	SetConsoleCursorPosition(output_, cursor);
	SetConsoleWindowInfo(output_, TRUE, &viewport);
	InterlockedIncrement(reinterpret_cast<LONG*>(&shared_->annotation.flushCounter));
	InterlockedExchange(reinterpret_cast<LONG*>(&shared_->annotation.locked), FALSE);
	return result;
}

void ConPtySession::Close()
{
	stopping_ = true;
	if (child_.hProcess && WaitForSingleObject(child_.hProcess, 0) != WAIT_OBJECT_0)
		TerminateProcess(child_.hProcess, ERROR_CANCELLED);
	if (job_)
	{
		CloseHandle(job_);
		job_ = nullptr;
	}
	if (inputThread_)
	{
		do { CancelSynchronousIo(inputThread_); }
		while (WaitForSingleObject(inputThread_, 20) == WAIT_TIMEOUT);
		CloseHandle(inputThread_);
		inputThread_ = nullptr;
	}
	if (pty_)
	{
		if (!outputThread_ && outputPipe_)
		{
			CloseHandle(outputPipe_);
			outputPipe_ = nullptr;
		}
		// The output thread keeps draining while ClosePseudoConsole shuts down.
		closePty_(pty_);
		pty_ = nullptr;
	}
	if (outputThread_)
	{
		WaitForSingleObject(outputThread_, INFINITE);
		CloseHandle(outputThread_);
		outputThread_ = nullptr;
	}
	if (ansi_)
	{
		// Flush the final bytes before restoring the caller's console state.
		try { Paint(); } catch (...) { outputError_ = ERROR_NOT_ENOUGH_MEMORY; }
		ansi_->SetConPtyInput(nullptr);
		ansi_ = nullptr;
	}
	for (auto handle : {inputPipe_, outputPipe_, child_.hThread, child_.hProcess, guiOwner_, serverOwner_})
		if (handle) CloseHandle(handle);
	inputPipe_ = outputPipe_ = child_.hThread = child_.hProcess = nullptr;
	guiOwner_ = serverOwner_ = nullptr;
	if (shared_)
	{
		InterlockedExchange(reinterpret_cast<LONG*>(&shared_->annotation.locked), TRUE);
		shared_->rows = 0;
		InterlockedIncrement(reinterpret_cast<LONG*>(&shared_->annotation.flushCounter));
		InterlockedExchange(reinterpret_cast<LONG*>(&shared_->annotation.locked), FALSE);
		UnmapViewOfFile(shared_);
		shared_ = nullptr;
	}
	if (sharedMap_) CloseHandle(sharedMap_);
	sharedMap_ = nullptr;
	if (ownsSession_) ReleaseMutex(sessionMutex_);
	if (sessionMutex_) CloseHandle(sessionMutex_);
	sessionMutex_ = nullptr;
	ownsSession_ = false;
	if (modeChanged_)
	{
		SetConsoleMode(input_, inputMode_);
		SetConsoleCP(inputCodePage_);
		if (originalCursor_.dwSize)
			SetConsoleCursorInfo(output_, &originalCursor_);
		SetConsoleTitleW(originalTitle_.c_str());
		modeChanged_ = false;
	}
}

int ConPtySession::Run(const wchar_t* command)
{
	if (!command || !*command)
	{
		PrintBuffer(L"Usage: ConEmuC64 /PTY program [arguments]\n");
		return ERROR_INVALID_PARAMETER;
	}
	if (!Start(command))
	{
		const DWORD error = GetLastError();
		Close();
		Printf("ConPTY startup failed, error %u\n", error);
		return error ? error : ERROR_GEN_FAILURE;
	}
	HANDLE wait[] = {child_.hProcess, outputThread_};
	DWORD result, waitCount = countof(wait);
	for (;;)
	{
		// Retain real process handles, not bare PIDs: owner death cancels the
		// session and cannot accidentally target a reused process ID.
		if ((guiOwner_ && WaitForSingleObject(guiOwner_, 0) == WAIT_OBJECT_0)
			|| (serverOwner_ && WaitForSingleObject(serverOwner_, 0) == WAIT_OBJECT_0))
		{
			gbInShutdown = TRUE; // No exit-key prompt in an orphaned helper.
			return ERROR_CANCELLED;
		}
		result = WaitForMultipleObjects(waitCount, wait, FALSE, 20);
		if (result == WAIT_OBJECT_0) break;
		if (result == WAIT_OBJECT_0 + 1 && waitCount == 2)
		{
			// EOF may precede the process signal. Preserve the child's real exit
			// code instead of reporting a read fault and forcing an exit prompt.
			if (outputError_) break;
			waitCount = 1;
		}
		else if (result != WAIT_TIMEOUT) break;
		if (ansi_->TerminalInputError()) return static_cast<int>(ansi_->TerminalInputError());
		if (!Paint())
			return ERROR_WRITE_FAULT;
	}
	DWORD exitCode = ERROR_READ_FAULT;
	if (result == WAIT_OBJECT_0)
		GetExitCodeProcess(child_.hProcess, &exitCode);
	Close();
	return outputError_ ? static_cast<int>(outputError_) : static_cast<int>(exitCode);
}

} // namespace

int RunConPty(const wchar_t* command)
{
	try
	{
		ConPtySession session;
		return session.Run(command);
	}
	catch (const std::exception& error)
	{
		Printf("ConPTY failed: %s\n", error.what());
		return ERROR_INVALID_DATA;
	}
}
