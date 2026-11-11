
/*
Copyright (c) 2013-present Maximus5
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

#include <mutex>
#include <atomic>
#include <map>
#include "../common/MPipe.h"
#include "../common/WCodePage.h"
#include "../common/WThreads.h"
#include "ConData.h"



struct SrvAnsi
{
private:
	SrvAnsi();
	~SrvAnsi();

	/// Registered for shutdown process
	static void Release(LPARAM lParam);

	/// Returns console buffer attributes set on ConEmuC startup
	static CONSOLE_SCREEN_BUFFER_INFO GetDefaultCSBI();
	/// Returns *ANSI* color indexes!!!
	static SHORT GetDefaultAnsiAttr();

public:
	static SrvAnsi* Object();

	/// we need to ‘cache’ parts of non-translated MBCS chars (one UTF-8 symbol may be transmitted by up to *three* parts)
	bool OurWriteConsoleA(const char* lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);
	/// for converted parts of UTF-8 data ready to written in UTF-16
	bool OurWriteConsoleW(const wchar_t* lpBuffer, DWORD nNumberOfCharsToWrite, LPDWORD lpNumberOfCharsWritten);

	void SetConPtyInput(HANDLE input);
	bool IsConPty() const { return m_ConPtyInput.load() != nullptr; }
	DWORD TerminalInputError() const { return m_ConPtyInputError.load(); }
	bool WriteTerminalInput(const char* data, DWORD size);
	bool ReportTerminalString(const wchar_t* text);
	bool ReadTerminalFrame(COORD size, std::vector<condata::CharInfo>& cells, COORD& cursor, uint64_t& version,
		unsigned& history, std::map<unsigned, std::wstring>& links, std::map<unsigned, std::wstring>& graphemes);

	/// Prepare server pipes, return client handles
	std::pair<HANDLE,HANDLE> GetClientHandles(DWORD clientPID);

protected:
	friend class SrvAnsiImpl;

	enum class GetTableEnum { current, primary, alternative };
	condata::Table* GetTable(GetTableEnum _table);
	CESERVER_CONSOLE_MAPPING_HDR* GetConMap() const;
	bool IsCmdProcess() const;
	static void BellCallback(void* param);

private:
	// Singleton
	static SrvAnsi* object_;
	static std::mutex& GetObjectMutex();

	/// Console processors
	std::mutex m_UseMutex;
	std::mutex m_InputMutex;
	std::atomic<HANDLE> m_ConPtyInput{nullptr};
	std::atomic<DWORD> m_ConPtyInputError{ERROR_SUCCESS};
	uint64_t m_FrameVersion = 1;
	unsigned m_ActiveHyperlink = 0, m_NextHyperlink = 1;
	size_t m_UriChars = 0;
	std::map<unsigned, std::wstring> m_Hyperlinks;
	void SetHyperlink(const std::wstring& uri);
	void PruneHyperlinks();
	mutable CESERVER_CONSOLE_MAPPING_HDR m_ClientMap = {};
	condata::Table m_Primary;
	condata::Table m_Alternative;
	bool m_UsePrimary = true;

	/// Pipes for processing input and output
	MPipeDual m_pipes;
	bool m_pipe_stop = false;
	MThread m_pipe_thread[2] = {};
	struct PipeArg { SrvAnsi* self; bool input; };
	static DWORD WINAPI PipeThread(LPVOID pipe_arg);

protected:
	// #condata Replace with MLogFile?
	HANDLE ghAnsiLogFile = NULL;
	bool   gbAnsiLogCodes = false;
	LONG   gnEnterPressed = 0;
	bool   gbAnsiLogNewLine = false;
	bool   gbAnsiWasNewLine = false;

	CEStr  gsInitConTitle;

	struct CpConv
	{
		// for example, "git add -p" uses codepage 1252 while printing chunks to be staged
		// that forces the printed text to be converted to nToCP before printing (OnWriteConsoleW)
		DWORD nFromCP, nToCP;
		// that parm may be used for overriding default console CP
		DWORD nDefaultCP;
	};
	struct CpConv gCpConv = {};

	bool gbIsXTermOutput = false;
	struct TermModeSet {
		DWORD value, pid;
	} gWasXTermModeSet[tmc_Last] = {};

	enum VTCharSet
	{
		VTCS_DEFAULT = 0,
		VTCS_DRAWING,
	};
	VTCharSet mCharSet = VTCS_DEFAULT;

	#undef DP_PROP
	#define DP_PROP(t,n) \
		private: t _##n; \
		public: t get##n() const { return _##n; }; \
		public: void set##n(const t val);
	enum cbit { clr4b = 0, clr8b, clr24b };
	struct DisplayParm
	{
		void Reset(const bool full);

		DP_PROP(bool, BrightOrBold)     // 1
		DP_PROP(bool, Italic)           // 3
		DP_PROP(bool, Underline)        // 4
		DP_PROP(bool, Inverse)          // 7
		DP_PROP(bool, Crossed)          // 9
		DP_PROP(bool, BrightFore)       // 90-97
		DP_PROP(bool, BrightBack)       // 100-107
		DP_PROP(int,  TextColor)        // 30-37,38,39
		DP_PROP(cbit, Text256)          // 38
		DP_PROP(int,  BackColor)        // 40-47,48,49
		DP_PROP(cbit, Back256)          // 48

		// set if there was a call of any PROP defined above
		DP_PROP(bool, Dirty)
	}; // gDisplayParm = {};
	#undef DP_PROP

	DisplayParm gDisplayParm = {};

	struct DisplayCursorPos
	{
		// Internal
		bool bCursorPosStored{ false };
		condata::Coord StoredCursorPos{};
		// ESC[?1h 	Set cursor key to application 	DECCKM
		// ESC[?1l 	Set cursor key to cursor 	DECCKM
		bool CursorKeysApp{ false }; // "?1h"
	}; // gDisplayCursor = {};

	DisplayCursorPos gDisplayCursor = {};
	DisplayCursorPos m_PrimaryDisplayCursor = {};

	// #condata replace or forward with m_Table calls, e.g. SetAutoCRLF instead of AutoLfNl
	struct DisplayOpt
	{
		BOOL  WrapWasSet = FALSE;
		SHORT WrapAt = 0; // Rightmost X coord (1-based)
		// #condata implement set (automatically on startup?)
		BOOL  AutoLfNl = FALSE; // LF/NL (default on for Windows, off for XTerm): Automatically follow echo of LF, VT or FF with CR.
		//
		BOOL  ShowRawAnsi = FALSE; // \e[3h display ANSI control characters (TRUE), \e[3l process ANSI (FALSE, normal mode)
	}; // gDisplayOpt;

	DisplayOpt gDisplayOpt = {};

	static constexpr unsigned CEAnsi_MaxPrevPart = 8192;
	static constexpr unsigned CEAnsi_MaxPrevAnsiPart = 4096;

	wchar_t gsPrevAnsiPart[CEAnsi_MaxPrevPart] = {};
	ssize_t gnPrevAnsiPart = 0;
	wchar_t gsPrevAnsiPart2[CEAnsi_MaxPrevPart] = {};
	ssize_t gnPrevAnsiPart2 = 0;

	bool mb_SuppressBells = false;
	/// Counter reset on each output start
	size_t m_BellsCounter = 0;

	// In "ReadLine" we can't control scrolling
	// thats why we need to mark some rows for identification
	struct XtermStoreMarks
	{
		// [0] - above CursorPos, but if CursorPos was on FIRST line - it can't be...
		// [1] - exactly on CursorPos, but if CursorX on the FIRST column - this will be illegal
		SHORT SaveRow[2];
		WORD  RowId[2];
		CONSOLE_SCREEN_BUFFER_INFO csbi;
	} m_RowMarks = {};

	CpCvt m_Cvt = {};
	wchar_t m_LastWrittenChar = L' ';

	enum DumpEscapeCodes
	{
		de_Normal = 0,
		de_Unknown = 1,
		de_BadUnicode = 2,
		de_Ignored = 3,
		de_UnkControl = 4,
		de_Report = 5,
		de_ScrollRegion = 6,
		de_Comment = 7,
	};


	/* ************ Globals for xTerm/ViM ************ */
	// #condata BufferWidth
	typedef DWORD XTermAltBufferFlags;
	const XTermAltBufferFlags
		xtb_AltBuffer          = 0x0001,
		xtb_StoredCursor       = 0x0002,
		xtb_StoredScrollRegion = 0x0004,
		xtb_None               = 0;
	struct XTermAltBufferData
	{
		XTermAltBufferFlags Flags;
		int    BufferSize;
		COORD  CursorPos;
		SHORT  ScrollStart, ScrollEnd;
	} gXTermAltBuffer = {};
	/* ************ Globals for xTerm/ViM ************ */

	DWORD last_write_tick_ = 0;

protected:

	/// Codepage set for OurWriteConsoleA
	UINT GetCodePage();

	/// ANSI functions
	void FirstAnsiCall(const BYTE* lpBuf, DWORD nNumberOfBytes);
	void InitAnsiLog(LPCWSTR asFilePath, const bool LogAnsiCodes);
	void DoneAnsiLog(bool bFinal);
	void WriteAnsiLogW(LPCWSTR lpBuffer, DWORD nChars);
	void WriteAnsiLogFormat(const char* format, ...);
	void WriteAnsiLogTime();
	bool WriteAnsiLogUtf8(const char* lpBuffer, DWORD nChars);
	void DebugStringUtf8(LPCWSTR asMessage);
	void DumpEscape(LPCWSTR buf, size_t cchLen, DumpEscapeCodes iUnknown);

	void SetCursorVisibility(bool visible);
	void SetCursorShape(unsigned style);

	void GetFeatures(bool* pbAnsiAllowed, bool* pbSuppressBells) const;

	void ApplyDisplayParm();
	void ReSetDisplayParm(bool bReset, bool bApply);
	condata::Attribute GetDefaultAttr() const;

	void ChangeTermMode(TermModeCommand mode, DWORD value, DWORD nPID = 0);
	/// <summary>
	/// Turn on/off xterm mode for both output and input.
	/// May be triggered by connector, official Vim builds, ENABLE_VIRTUAL_TERMINAL_INPUT, "ESC ] 9 ; 10 ; 1 ST", etc.
	/// </summary>
	/// <param name="bStart">true - start xterm mode, false - stop</param>
	void StartXTermMode(bool bStart);
	/// <summary>
	/// Turn on/off xterm mode only for output (especially for line feeding mode).
	/// Triggered by ENABLE_VIRTUAL_TERMINAL_PROCESSING.
	/// </summary>
	/// <param name="bStart">true - start xterm mode, false - stop</param>
	void StartXTermOutput(bool bStart);
	void RefreshXTermModes();
};
