/*
Copyright (c) 2015-present Maximus5
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


#define HIDE_USE_EXCEPTION_INFO
#define SHOWDEBUGSTR
#define DEBUGSTRCMD(x) DEBUGSTR(x)

#ifdef _DEBUG
//#define SHOW_ATTACH_MSGBOX
//#define SHOW_LOADCFGFILE_MSGBOX
//#define SHOW_SERVER_STARTED_MSGBOX
//#define SHOW_COMSPEC_STARTED_MSGBOX
//#define SHOW_INJECTS_MSGBOX
#endif

#include "ConsoleArgs.h"

#include "Actions.h"
#include "ConsoleMain.h"
#include "ExitCodes.h"
#include "ExportedFunctions.h"

#include "../common/CmdLine.h"
#include "../common/MStrDup.h"
#include "../common/RConStartArgs.h"
#ifdef SHOW_INJECTS_MSGBOX
#include "../common/MToolHelp.h"
#endif

#include <tuple>

#include "ConsoleState.h"


ConsoleArgs* gpConsoleArgs = nullptr;


Switch::Switch(const SwitchType aType)
	: switchType(aType)
{
}

Switch::~Switch()
{
	Clear();
}

// Helpers

void Switch::Clear()
{
	switchType = SwitchType::None;
	exists = false;
}

SwitchBool::SwitchBool(const bool defaultValue /*= false*/)
	: Switch(SwitchType::Simple)
	, value(defaultValue)
{
}

SwitchBool::operator bool() const
{
	return GetBool();
}

SwitchInt::SwitchInt()
	: Switch(SwitchType::Int)
{
}

SwitchBool& SwitchBool::operator=(const bool newVal)
{
	SetBool(newVal);
	return *this;
}

SwitchInt& SwitchInt::operator=(const ValueType_t newVal)
{
	SetInt(newVal);
	return *this;
}

SwitchStr::SwitchStr()
	: Switch(SwitchType::Str)
{
}

SwitchStr::~SwitchStr()
{
	Clear();
}

SwitchStr& SwitchStr::operator=(const wchar_t* const newVal)
{
	SetStr(newVal);
	return *this;
}

void SwitchBool::Clear()
{
	value = false;
	Switch::Clear();
}

bool SwitchBool::GetBool() const
{
	_ASSERTE(switchType == SwitchType::Simple || (switchType == SwitchType::None && !exists));
	return (exists && value);
}

void SwitchBool::SetBool(const bool newVal)
{
	value = newVal;
	exists = true;
	if (switchType != SwitchType::Simple)
	{
		_ASSERTE(switchType == SwitchType::Simple || switchType == SwitchType::None);
		switchType = SwitchType::Simple;
	}
}

void SwitchInt::Clear()
{
	value = 0;
	Switch::Clear();
}

bool SwitchInt::IsValid() const
{
	return exists && switchType == SwitchType::Int;
}

SwitchInt::ValueType_t SwitchInt::GetInt() const
{
	_ASSERTE(switchType==SwitchType::Int || (switchType==SwitchType::None && !exists));
	return (exists && (switchType == SwitchType::Int)) ? value : 0;
}

SwitchInt::ValueType_t SwitchInt::SetInt(const ValueType_t newVal)
{
	value = newVal;
	exists = true;
	if (switchType != SwitchType::Int)
	{
		_ASSERTE(switchType == SwitchType::Int || switchType == SwitchType::None);
		switchType = SwitchType::Int;
	}
	return value;
}

SwitchInt::ValueType_t SwitchInt::SetInt(const wchar_t* const newVal, const int radix /*= 10*/)
{
	if (!newVal || !*newVal)
	{
		Clear();
		exists = true;
	}
	else
	{
		wchar_t* endPtr = nullptr;
		const auto iVal = wcstoll(newVal, &endPtr, radix);
		SetInt(iVal);
	}
	return value;
}

void SwitchStr::Clear()
{
	if (value)
		free(value);
	Switch::Clear();
}

bool SwitchStr::IsEmpty() const
{
	if (!exists)
		return true;
	if (!value || !*value)
		return true;
	return false;
}

LPCWSTR SwitchStr::GetStr() const
{
	if (!exists || (switchType != SwitchType::Str))
		return nullptr;
	if (!value || !*value)
		return nullptr;
	return value;
}

void SwitchStr::SetStr(const wchar_t* const newVal)
{
	if (value != newVal)
	{
		if (value)
			free(value);
		value = (newVal && *newVal) ? lstrdup(newVal).Detach() : nullptr;
	}
	exists = true;
	switchType = SwitchType::Str;
}

void SwitchStr::SetStr(CEStr&& newVal)
{
	if (value != newVal.data() && value != nullptr)
		free(value);
	value = newVal.Detach();
	exists = true;
	switchType = SwitchType::Str;
}


// ********************************************
ConsoleArgs::ConsoleArgs() // NOLINT(modernize-use-equals-default)
{
}

ConsoleArgs::~ConsoleArgs() // NOLINT(modernize-use-equals-default)
{
}

bool ConsoleArgs::GetCfgParam(LPCWSTR& cmdLineRest, SwitchStr& Val)
{
	Val.Clear();

	if (!cmdLineRest || !*cmdLineRest)
	{
		_ASSERTE(cmdLineRest && *cmdLineRest);
		return false;
	}

	CmdArg newArg;
	if (!((cmdLineRest = NextArg(cmdLineRest, newArg))))
	{
		return false;
	}

	Val.SetStr(newArg.Detach());
	_ASSERTE(Val.switchType == SwitchType::Str);
	_ASSERTE(Val.exists == (Val.value && *Val.value));
	Val.exists = (Val.value && *Val.value);

	return true;
}

/// Debug helper, show message box if SHOW_ATTACH_MSGBOX is defined and "/ADMIN" switch is specified
// ReSharper disable once CppMemberFunctionMayBeStatic
void ConsoleArgs::ShowAttachMsgBox(const CEStr& szArg) const
{
#ifdef SHOW_ATTACH_MSGBOX
	if (!IsDebuggerPresent())
	{
		wchar_t szTitle[100]; swprintf_c(szTitle, L"%s PID=%u %s", gsModuleName, gnSelfPID, szArg.c_str(L""));
		MessageBox(NULL, fullCmdLine_.c_str(L""), szTitle, MB_SYSTEMMODAL);
	}
#endif
}

void ConsoleArgs::ShowConfigMsgBox(const CEStr& szArg, LPCWSTR cmdLineRest)
{
#ifdef SHOW_LOADCFGFILE_MSGBOX
	MessageBox(NULL, lsCmdLine, szArg.c_str(L""), MB_SYSTEMMODAL);
#endif
}

void ConsoleArgs::ShowServerStartedMsgBox(LPCWSTR asCmdLine)
{
#ifdef SHOW_SERVER_STARTED_MSGBOX
	wchar_t szTitle[100]; swprintf_c(szTitle, L"ConEmuC [Server] started (PID=%i)", gnSelfPID);
	const wchar_t* pszCmdLine = asCmdLine;
	MessageBox(NULL,pszCmdLine,szTitle,0);
#endif
}

void ConsoleArgs::ShowComspecStartedMsgBox(LPCWSTR asCmdLine)
{
#ifdef SHOW_COMSPEC_STARTED_MSGBOX
	wchar_t szTitle[100]; swprintf_c(szTitle, L"ConEmuC [ComSpec] started (PID=%i)", gnSelfPID);
	const wchar_t* pszCmdLine = asCmdLine;
	MessageBox(NULL,pszCmdLine,szTitle,0);
#endif
}

void ConsoleArgs::ShowInjectsMsgBox(const ConEmuExecAction mode, const wchar_t* asCmdLine, const wchar_t* param)
{
#ifdef SHOW_INJECTS_MSGBOX
	wchar_t szTitle[100] = L"";
	const DWORD hookPid = wcstoul(param, nullptr, 10);
	MToolHelpModule modules(hookPid);
	MODULEENTRY32W exeInfo{};
	modules.Next(exeInfo);
	swprintf_c(szTitle, ConEmuC_EXE_3264 L" [%s] started (PID=%i)",
		mode == ConEmuExecAction::InjectDefTrm ? L"InjectDefTerm"
		: mode == ConEmuExecAction::InjectHooks ? L"InjectHooks"
		: L"InjectRemote", gnSelfPID);
	const CEStr msg(L"Command:\n", asCmdLine, L"\n\n", L"Target process:\n", exeInfo.szExePath);
	MessageBoxW(nullptr, msg, szTitle, 0);
#endif
}

void ConsoleArgs::AddConEmuArg(LPCWSTR asSwitch, LPCWSTR asValue)
{
	conemuAddArgs_.Append(asSwitch);
	if (asValue && *asValue)
	{
		const bool needQuot = IsQuotationNeeded(asValue);
		conemuAddArgs_.Append(
			needQuot ? L" \"" : L" ",
			asValue,
			needQuot ? L"\"" : nullptr);
	}
	SetEnvironmentVariable(ENV_CONEMU_EXEARGS_W, conemuAddArgs_);
}

/// AutoAttach is not allowed in some cases
bool ConsoleArgs::IsAutoAttachAllowed() const
{
	if (!gState.realConWnd_)
	{
		LogString("IsAutoAttachAllowed is not allowed, gState.realConWnd is not null");
		return false;
	}

	if (gState.attachMode_ & am_Admin)
	{
		LogString("IsAutoAttachAllowed is allowed due to am_Admin");
		return true;
	}

	if (!IsWindowVisible(gState.realConWnd_))
	{
		if (defTermCall_ || attachFromFar_)
		{
			LogString(
				defTermCall_
					? "IsAutoAttachAllowed is allowed due to gbDefTermCall"
					: "IsAutoAttachAllowed is allowed due to gbAttachFromFar");
			return true;
		}
		return false;
	}

	if (IsIconic(gState.realConWnd_))
	{
		LogString("IsAutoAttachAllowed is not allowed, gState.realConWnd is iconic");
		return false;
	}

	LogString("IsAutoAttachAllowed is allowed");
	return true;
}

/// Are we sure to hide RealConsole window
bool ConsoleArgs::IsForceHideConWnd() const
{
	// В принципе, консоль может действительно запуститься видимой. В этом случае ее скрывать не нужно
	// Но скорее всего, консоль запущенная под Админом в Win7 будет отображена ошибочно
	// 110807 - Если gpStatus->attachMode_, тоже консоль нужно спрятать
	return forceHideConWnd_ || (gState.attachMode_ && (gState.attachMode_ != am_Admin));
}

/// <summary>
/// Parse command line into member variables
/// </summary>
/// <param name="pszCmdLine">Full command line of ConEmuC.exe or argument passes to ConsoleMain3</param>
/// <param name="anWorkMode">Passed from ConsoleMain3</param>
/// <returns>0 on success or error code otherwise</returns>
int ConsoleArgs::ParseCommandLine(LPCWSTR pszCmdLine, const ConsoleMainMode anWorkMode)
{
	LogFunction(L"ParseCommandLine");

	_ASSERTE(pszCmdLine!=nullptr);

	fullCmdLine_.Set(pszCmdLine ? pszCmdLine : L"");

	if (anWorkMode == ConsoleMainMode::AltServer)
	{
		// Skip argument parsing, we were loaded into already running process
		gState.runMode_ = RunMode::AltServer;
		return 0;
	}

	// pszCmdLine *may* or *may not* start with our executable or full path to our executable
	const auto* cmdLineRest = SkipNonPrintable(fullCmdLine_);
	CmdArg szArg;

	// Check the first argument in the command line (most probably it will be our executable path/name)
	{
		LPCWSTR pszTemp = fullCmdLine_;
		if (!((pszTemp = NextArg(pszTemp, szArg))))
		{
			_ASSERTE(FALSE && "GetCommandLine() is empty");
			return 0;
		}
		if (!szArg.IsPossibleSwitch())
		{
			_ASSERTE(IsFilePath(szArg));
			cmdLineRest = SkipNonPrintable(pszTemp);
		}
	}

	int iResult = 0;
	LPCWSTR pszArgStart;

	// Must be empty at the moment
	_ASSERTE(command_.IsEmpty());

	// Let parse the reset
	szArg.Clear();

	// Processing loop begin
	while ((cmdLineRest = NextArg(cmdLineRest, szArg, &pszArgStart)))
	{
		if (!szArg.IsPossibleSwitch())
		{
			_ASSERTE(FALSE && "Unknown switch!");
			// Show error on unknown switch
			unknownSwitch_.SetStr(pszArgStart);
			break;
		}


		// Main processing cycle
		params_++;

		if (szArg.OneOfSwitches(L"-?", L"-h", L"-help"))
		{
			iResult = CERR_HELPREQUESTED;
			break;
		}
#ifdef _DEBUG
		else if (szArg.IsSwitch(L"/DebugTrap"))
		{
			// ReSharper disable once CppJoinDeclarationAndAssignment
			int i, j = 1;
			j--;
			// ReSharper disable once CppJoinDeclarationAndAssignment
			i = 1 / j;
			std::ignore = i;
		}
#endif
			// **** Unit tests ****
		else if (szArg.OneOfSwitches(L"/Args", L"/ParseArgs"))
		{
			eExecAction_ = ConEmuExecAction::ParseArgs;
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.IsSwitch(L"/ConInfo"))
		{
			eExecAction_ = ConEmuExecAction::PrintConsoleInfo;
			break;
		}
		else if (szArg.IsSwitch(L"/CheckUnicode"))
		{
			eExecAction_ = ConEmuExecAction::CheckUnicodeFont;
			break;
		}
		else if (szArg.IsSwitch(L"/TestUnicode"))
		{
			eExecAction_ = ConEmuExecAction::TestUnicodeCvt;
			break;
		}
		else if (szArg.IsSwitch(L"/OsVerInfo"))
		{
			eExecAction_ = ConEmuExecAction::OsVerInfo;
			break;
		}
		else if (szArg.IsSwitch(L"/ErrorLevel"))
		{
			eExecAction_ = ConEmuExecAction::ErrorLevel;
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.IsSwitch(L"/Result"))
		{
			printRetErrLevel_.SetBool(true);
		}
		else if (szArg.IsSwitch(L"/Verbose"))
		{
			debugVerbose_.SetBool(true);
		}
		else if (szArg.OneOfSwitches(L"/echo", L"/e"))
		{
			eExecAction_ = ConEmuExecAction::OutEcho;
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.IsSwitch(L"/PTY"))
		{
			eExecAction_ = ConEmuExecAction::ConPty;
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.OneOfSwitches(L"/type", L"/t"))
		{
			eExecAction_ = ConEmuExecAction::OutType;
			command_.Set(cmdLineRest);
			break;
		}
			// **** Regular use ****
		else if (szArg.IsSwitch(L"/RegConFont="))
		{
			eExecAction_ = ConEmuExecAction::RegConFont;
			command_.Set(szArg.GetExtra());
			break;
		}
		else if (szArg.IsSwitch(L"/SetHooks="))
		{
			gState.runMode_ = RunMode::SetHook64;
			eExecAction_ = ConEmuExecAction::InjectHooks;
			command_.Set(szArg.GetExtra());
			ShowInjectsMsgBox(eExecAction_, fullCmdLine_.c_str(L""), command_.c_str(L"0"));
			break;
		}
		else if (szArg.IsSwitch(L"/INJECT="))
		{
			gState.runMode_ = RunMode::SetHook64;
			eExecAction_ = ConEmuExecAction::InjectRemote;
			command_.Set(szArg.GetExtra());
			ShowInjectsMsgBox(eExecAction_, fullCmdLine_.c_str(L""), command_.c_str(L"0"));
			break;
		}
		else if (szArg.IsSwitch(L"/DEFTRM="))
		{
			gState.runMode_ = RunMode::SetHook64;
			eExecAction_ = ConEmuExecAction::InjectDefTrm;
			command_.Set(szArg.GetExtra());
			ShowInjectsMsgBox(eExecAction_, fullCmdLine_.c_str(L""), command_.c_str(L"0"));
			break;
		}
		else if (szArg.OneOfSwitches(L"/STRUCT", L"/DumpStruct"))
		{
			eExecAction_ = ConEmuExecAction::DumpStruct;
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.IsSwitch(L"/StoreCWD"))
		{
			eExecAction_ = ConEmuExecAction::StoreCWD;
			command_.Set(cmdLineRest);
			break;
		}
			// /GUIMACRO[:PID|0xHWND][:T<tab>][:S<split>] <Macro string>
		else if (szArg.OneOfSwitches(L"/GuiMacro", L"/GuiMacro="))
		{
			// Execute rest of the command line as GuiMacro
			//gState.runMode_ = RunMode::GuiMacro;
			guiMacro_.SetStr(szArg);
			eExecAction_ = ConEmuExecAction::GuiMacro;
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.IsSwitch(L"/UseExport"))
		{
			macroExportResult_.SetBool(true);
		}
		else if (szArg.IsSwitch(L"/SILENT"))
		{
			preferSilentMode_.SetBool(true);
		}
		else if (szArg.OneOfSwitches(L"/EXPORT=ALL", L"/ExportAll"))
		{
			eExecAction_ = ConEmuExecAction::ExportAll;
			//_ASSERTE(FALSE && "Continue to export");
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.OneOfSwitches(L"/EXPORT=CON", L"/ExportCon"))
		{
			eExecAction_ = ConEmuExecAction::ExportCon;
			//_ASSERTE(FALSE && "Continue to export");
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.OneOfSwitches(L"/EXPORT=GUI", L"/ExportGui"))
		{
			eExecAction_ = ConEmuExecAction::ExportGui;
			//_ASSERTE(FALSE && "Continue to export");
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.IsSwitch(L"/EXPORT"))
		{
			eExecAction_ = ConEmuExecAction::ExportTab;
			//_ASSERTE(FALSE && "Continue to export");
			command_.Set(cmdLineRest);
			break;
		}
		else if (szArg.IsSwitch(L"/IsConEmu"))
		{
			eStateCheck_ = ConEmuStateCheck::IsConEmu;
			break;
		}
		else if (szArg.IsSwitch(L"/IsTerm"))
		{
			eStateCheck_ = ConEmuStateCheck::IsTerm;
			break;
		}
		else if (szArg.IsSwitch(L"/IsAnsi"))
		{
			eStateCheck_ = ConEmuStateCheck::IsAnsi;
			break;
		}
		else if (szArg.IsSwitch(L"/IsAdmin"))
		{
			eStateCheck_ = ConEmuStateCheck::IsAdmin;
			break;
		}
		else if (szArg.IsSwitch(L"/IsRedirect"))
		{
			eStateCheck_ = ConEmuStateCheck::IsRedirect;
			break;
		}
		else if (szArg.OneOfSwitches(L"/CONFIRM", L"/ConfHalt", L"/EConfirm"))
		{
			const auto* name = szArg.Mid(1); // Skip the "/", it could be "-"
			confirmExitParm_ = (lstrcmpi(name, L"CONFIRM") == 0)
								   ? RConStartArgs::eConfAlways
								   : (lstrcmpi(name, L"ConfHalt") == 0)
								   ? RConStartArgs::eConfHalt
								   : RConStartArgs::eConfEmpty;
			gState.alwaysConfirmExit_ = true;
			gState.autoDisableConfirmExit_ = false;
		}
		else if (szArg.IsSwitch(L"/NoConfirm"))
		{
			confirmExitParm_ = RConStartArgs::eConfNever;
			gState.alwaysConfirmExit_ = false;
			gState.autoDisableConfirmExit_ = false;
		}
		else if (szArg.IsSwitch(L"/OmitHooksWarn"))
		{
			skipHookSettersCheck_.SetBool(true);
		}
		else if (szArg.IsSwitch(L"/ADMIN"))
		{
			ShowAttachMsgBox(szArg);
			gState.attachMode_ |= am_Admin;
			gState.runMode_ = RunMode::Server;
		}
		else if (szArg.IsSwitch(L"/ATTACH"))
		{
			ShowAttachMsgBox(szArg);
			if (!(gState.attachMode_ & am_Modes))
				gState.attachMode_ |= am_Simple;
			gState.runMode_ = RunMode::Server;
		}
		else if (szArg.IsSwitch(L"/InheritDefTerm"))
		{
			inheritDefTerm_.SetBool(true);
		}
		else if (szArg.OneOfSwitches(L"/AutoAttach", L"/AttachDefTerm"))
		{
			ShowAttachMsgBox(szArg);
			gState.attachMode_ |= am_Auto;
			gState.alienMode_ = true;
			gState.noCreateProcess_ = true;
			if (szArg.IsSwitch(L"/AutoAttach"))
			{
				gState.runMode_ = RunMode::AutoAttach;
				gState.attachMode_ |= am_Async;
			}
			if (szArg.IsSwitch(L"/AttachDefTerm"))
			{
				gState.runMode_ = RunMode::Server;
				gState.attachMode_ |= am_DefTerm;
				defTermCall_.SetBool(true);
			}

			// Below is also "/GHWND=NEW". There would be "requestNewGuiWnd_" set to "true"

			//ConEmu autorun (c) Maximus5
			//Starting "%ConEmuPath%" in "Attach" mode (NewWnd=%FORCE_NEW_WND%)
		}
		else if (szArg.IsSwitch(L"/GuiAttach="))
		{
			ShowAttachMsgBox(szArg);

			gState.runMode_ = RunMode::Server;
			if (!(gState.attachMode_ & am_Modes))
				gState.attachMode_ |= am_Simple;
			attachGuiAppWnd_.SetInt(szArg.GetExtra(), 16);
		}
		else if (szArg.IsSwitch(L"/NoCmd"))
		{
			gState.runMode_ = RunMode::Server;
			gState.noCreateProcess_ = true;
			gState.alienMode_ = true;
		}
		else if (szArg.IsSwitch(L"/ParentFarPid="))
		{
			parentFarPid_.SetInt(szArg.GetExtra(), 10);
		}
		else if (szArg.IsSwitch(L"/CreateCon"))
		{
			creatingHiddenConsole_.SetBool(true);
			//_ASSERTE(FALSE && "Continue to create con");
		}
		else if (szArg.IsSwitch(L"/ROOTEXE"))
		{
			if ((cmdLineRest = NextArg(cmdLineRest, szArg, &pszArgStart)))
				rootExe_.SetStr(CEStr(L"\"", szArg, L"\""));
		}
		else if (szArg.OneOfSwitches(L"/PID=", L"/TRMPID=", L"/FARPID=", L"/CONPID="))
		{
			gState.runMode_ = RunMode::Server;
			gState.noCreateProcess_ = true; // process already started
			gState.alienMode_ = true;       // console was not created by us

			if (szArg.IsSwitch(L"/TRMPID="))
			{
				// This is called from *.vshost.exe when "AllocConsole" just created
				defTermCall_.SetBool(true);
				doNotInjectConEmuHk_.SetBool(true);
			}
			else if (szArg.IsSwitch(L"/FARPID="))
			{
				attachFromFar_.SetBool(true);
				gState.rootIsCmdExe_ = false;
			}
			else if (szArg.IsSwitch(L"/CONPID="))
			{
				//_ASSERTE(FALSE && "Continue to alternative attach mode");
				alternativeAttach_.SetBool(true);
				gState.rootIsCmdExe_ = false;
			}
			else if (szArg.IsSwitch(L"/CONPID="))
			{
				// Nothing to set here
			}
			else
			{
				_ASSERTE(FALSE && "Should not get here");
			}

			rootPid_.SetInt(szArg.GetExtra(), 10);
		}
		else if (szArg.IsSwitch(L"/CInMode="))
		{
			consoleModeFlags_.SetInt(szArg.GetExtra(), 16);
		}
		else if (szArg.IsSwitch(L"/HIDE"))
		{
			forceHideConWnd_.SetBool(true);
		}
		else if (szArg.OneOfSwitches(L"/BW=", L"/BH=", L"/BZ=", L"/BX="))
		{
			if (szArg.IsSwitch(L"/BW="))
			{
				if (visibleWidth_.SetInt(szArg.GetExtra(), 10) > 0)
				{
					gcrVisibleSize.X = static_cast<decltype(gcrVisibleSize.X)>(visibleWidth_.GetInt());
					gbParmVisibleSize = true;
				}
			}
			else if (szArg.IsSwitch(L"/BH="))
			{
				if (visibleHeight_.SetInt(szArg.GetExtra(), 10) > 0)
				{
					gcrVisibleSize.Y = static_cast<decltype(gcrVisibleSize.Y)>(visibleHeight_.GetInt());
					gbParmVisibleSize = true;
				}
			}
			else if (szArg.IsSwitch(L"/BZ="))
			{
				if (bufferHeight_.SetInt(szArg.GetExtra(), 10) > 0)
				{
					gnBufferHeight = static_cast<decltype(gnBufferHeight)>(bufferHeight_.GetInt());
					gbParmVisibleSize = true;
				}
			}
			else if (szArg.IsSwitch(L"/BX="))
			{
				bufferWidth_.SetInt(szArg.GetExtra(), 10);
				// #ConsoleArgs BufferWidth
			}
			else
			{
				_ASSERTE(FALSE && "Should not get here");
			}
		}
		else if (szArg.OneOfSwitches(L"/FN=", L"/FW=", L"/FH="))
		{
			if (szArg.IsSwitch(L"/FN="))
			{
				consoleFontName_.SetStr(szArg.GetExtra());
				_ASSERTE(!consoleFontName_.IsEmpty() && wcslen(consoleFontName_.GetStr()) < LF_FACESIZE);
			}
			else if (szArg.IsSwitch(L"/FW="))
			{
				consoleFontWidth_.SetInt(szArg.GetExtra(), 10);
			}
			else if (szArg.IsSwitch(L"/FH="))
			{
				consoleFontHeight_.SetInt(szArg.GetExtra(), 10);
			}
		}
		else if (szArg.OneOfSwitches(L"/LOG", L"/LOG0", L"/LOG1", L"/LOG2", L"/LOG3", L"/LOG4"))
		{
			// we use here lstrcmpni, because different logging levels are possible (e.g. /LOG1)
			// for now, levels are ignored, so just /LOG
			isLogging_.SetBool(true);
		}
		else if (szArg.IsSwitch(L"/GID="))
		{
			gState.runMode_ = RunMode::Server;
			conemuPid_.SetInt(szArg.GetExtra(), 10);
		}
		else if (szArg.IsSwitch(L"/AID="))
		{
			wchar_t* pszEnd = nullptr;
			gState.dwGuiAID = wcstoul(szArg.Mid(5), &pszEnd, 10);
		}
		else if (szArg.IsSwitch(L"/GHWND="))
		{
			if (gState.runMode_ == RunMode::Undefined)
			{
				gState.runMode_ = RunMode::Server;
			}
			else
			{
				_ASSERTE(
					gState.runMode_ == RunMode::AutoAttach
					|| gState.runMode_ == RunMode::Server || gState.runMode_ == RunMode::AltServer);
			}

			if (lstrcmpi(szArg.GetExtra(), L"NEW") == 0)
			{
				requestNewGuiWnd_.SetBool(true);
			}
			else
			{
				LPCWSTR pszDescriptor = szArg.GetExtra();
				if (pszDescriptor[0] == L'0' && (pszDescriptor[1] == L'x' || pszDescriptor[1] == L'X'))
					pszDescriptor += 2; // That may be useful for calling from batch files
				conemuHwnd_.SetInt(pszDescriptor, 16);
				requestNewGuiWnd_.SetBool(false);
			}
		}
		else if (szArg.IsSwitch(L"/TA="))
		{
			consoleColorIndexes_.SetInt(szArg.GetExtra(), 16);
		}
		else if (szArg.IsSwitch(L"/DEBUGPID="))
		{
			//gpStatus->runMode_ = RunMode::RM_SERVER; -- don't set, the RM_UNDEFINED is flag for debugger only
			debugPidList_.SetStr(szArg.GetExtra());
		}
		else if (szArg.OneOfSwitches(L"/DebugExe", L"/DebugTree"))
		{
			//gpStatus->runMode_ = RunMode::RM_SERVER; -- don't set, the RM_UNDEFINED is flag for debugger only
			if (szArg.IsSwitch(L"/DebugExe"))
				debugExe_.SetBool(true);
			else
				debugTree_.SetBool(true);
			command_.Set(cmdLineRest);
			// STOP processing rest of command line, it goes to debugger
			break;
		}
		else if (szArg.IsSwitch(L"/DUMP"))
		{
			debugDump_.SetBool(true);
		}
		else if (szArg.OneOfSwitches(L"/MINIDUMP", L"/MINI"))
		{
			debugDump_.SetBool(true);
			debugMiniDump_.SetBool(true);
		}
		else if (szArg.OneOfSwitches(L"/FULLDUMP", L"/FULL"))
		{
			debugDump_.SetBool(true);
			debugFullDump_.SetBool(true);
		}
		else if (szArg.IsSwitch(L"/AutoMini"))
		{
			debugDump_.SetBool(true);
			//_ASSERTE(FALSE && "Continue to /AUTOMINI");
			const wchar_t* interval = nullptr;
			if (cmdLineRest && *cmdLineRest && isDigit(cmdLineRest[0]))
			{
				if ((cmdLineRest = NextArg(cmdLineRest, szArg, &pszArgStart)))
					interval = szArg.ms_Val;
			}
			debugAutoMini_.SetStr(interval);
		}
		else if (szArg.IsSwitch(L"/ProfileCd"))
		{
			needCdToProfileDir_.SetBool(true);
		}
		else if (szArg.IsSwitch(L"/CONFIG"))
		{
			ShowConfigMsgBox(szArg, cmdLineRest);
			if (!((cmdLineRest = NextArg(cmdLineRest, szArg, &pszArgStart))))
			{
				iResult = CERR_CMDLINEEMPTY;
				_ASSERTE(FALSE && "Config name was not specified!");
				PrintBuffer(L"Config name was not specified!\r\n");
				break;
			}
			configName_.SetStr(szArg);
			// Reuse config if starting "ConEmu.exe" from console server!
			SetEnvironmentVariable(ENV_CONEMU_CONFIG_W, szArg);
			AddConEmuArg(L"-config", szArg);
		}
		else if (szArg.IsSwitch(L"/LoadCfgFile"))
		{
			ShowConfigMsgBox(szArg, cmdLineRest);
			// Reuse specified xml file if starting "ConEmu.exe" from console server!
			if (!((cmdLineRest = NextArg(cmdLineRest, szArg, &pszArgStart))))
			{
				iResult = CERR_CMDLINEEMPTY;
				_ASSERTE(FALSE && "Xml file name was not specified!");
				PrintBuffer(L"Xml file name was not specified!\r\n");
				break;
			}
			configFile_.SetStr(szArg);
			AddConEmuArg(L"-LoadCfgFile", szArg);
		}
		else if (szArg.OneOfSwitches(L"/ASYNC", L"/FORK"))
		{
			asyncRun_.SetBool(true);
		}
		else if (szArg.IsSwitch(L"/NoInject"))
		{
			doNotInjectConEmuHk_.SetBool(true);
		}
		else if (szArg.IsSwitch(L"/DOSBOX"))
		{
			gState.dosbox_.use_ = true;
		}
		else if (szArg.IsSwitch(L"-STD"))
		{
			gState.reopenStdHandles_ = true;
		}
			// После этих аргументов - идет то, что передается в CreateProcess!
		else if (szArg.IsSwitch(L"/ROOT"))
		{
			ShowServerStartedMsgBox(cmdLineRest);
			gState.runMode_ = RunMode::Server;
			gState.noCreateProcess_ = FALSE;
			gpConsoleArgs->asyncRun_ = FALSE;
			command_.Set(cmdLineRest);
			break;
		}
			// После этих аргументов - идет то, что передается в COMSPEC (CreateProcess)!
			//if (wcscmp(szArg, L"/C")==0 || wcscmp(szArg, L"/c")==0 || wcscmp(szArg, L"/K")==0 || wcscmp(szArg, L"/k")==0) {
		else if (szArg.OneOfSwitches(L"/C", L"/K"))
		{
			ShowComspecStartedMsgBox(cmdLineRest);
			gState.noCreateProcess_ = FALSE;

			//_ASSERTE(FALSE && "ConEmuC -c ...");

			if (szArg.GetLen() == 2) // "/c" or "/k"
				gState.runMode_ = RunMode::Comspec;

			if (gState.runMode_ == RunMode::Undefined && szArg[4] == 0
				&& ((szArg[2] & ~0x20) == L'M') && ((szArg[3] & ~0x20) == L'D'))
			{
				_ASSERTE(FALSE && "'/cmd' obsolete switch. use /c, /k, /root");
				gState.runMode_ = RunMode::Server;
			}

			if (gState.runMode_ == RunMode::Undefined)
			{
				gState.runMode_ = RunMode::Comspec;
				// Support weird legacy "cmd /cecho xxx"
				command_.Set(SkipNonPrintable(pszArgStart + 2));
			}
			else
			{
				command_.Set(cmdLineRest);
			}

			if (gState.runMode_ == RunMode::Comspec)
			{
				cmdK_.SetBool(szArg.IsSwitch(L"/K"));
			}

			break;
		}
		else
		{
			_ASSERTE(FALSE && "Unknown switch!");
			// Show error on unknown switch
			unknownSwitch_.SetStr(szArg);
			//PrintBuffer(L"Unknown switch: ");
			//PrintBuffer(szArg);
			//PrintBuffer(L"\r\n");
		}

		// Avoid assertions in NextArg
		szArg.Clear();
	} // while (NextArg(&cmdLineRest, szArg, &pszArgStart) == 0)
	// Processing loop end

	return iResult;
}
