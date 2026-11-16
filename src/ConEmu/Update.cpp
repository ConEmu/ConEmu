
/*
Copyright (c) 2009-present Maximus5
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
#include "Header.h"
#include <wininet.h>
#include "../common/shlobj.h"
#include "Update.h"
#include "UpdateConst.h"
#include "UpdateSet.h"
#include "Options.h"
#include "OptionsClass.h"
#include "ConEmu.h"
#include "TrayIcon.h"
#include "version.h"

#include <commctrl.h>
#include "AboutDlg.h"
#include "ConfirmDlg.h"
#include "DontEnable.h"
#include "LngRc.h"
#include "../common/MSection.h"
#include "../common/WFiles.h"
#include "../common/WThreads.h"
#include "../common/WUser.h"

CConEmuUpdate* gpUpd = nullptr;

// Timeout for mh_CheckThread in milliseconds
const DWORD UPDATE_THREAD_TIMEOUT = 2500;


CConEmuUpdate::CConEmuUpdate()
{
	_ASSERTE(isMainThread());

	wchar_t szVer4[8] = L""; lstrcpyn(szVer4, _T(MVV_4a), countof(szVer4));
	swprintf_c(ms_OurVersion, L"%02u%02u%02u%s",
		(MVV_1%100), MVV_2, MVV_3, szVer4);

	lstrcpyn(ms_DefaultTitle, gpConEmu->GetDefaultTitle(), countof(ms_DefaultTitle));

	bNeedRunElevation = false;
}

CConEmuUpdate::~CConEmuUpdate()
{
	if (mh_CheckThread)
	{
		DWORD nWait;
		if ((nWait = WaitForSingleObject(mh_CheckThread, 0)) == WAIT_TIMEOUT)
		{
			RequestTerminate();
			nWait = WaitForSingleObject(mh_CheckThread, UPDATE_THREAD_TIMEOUT);
		}

		if (nWait != WAIT_OBJECT_0)
		{
			_ASSERTE(FALSE && "Terminating Updater(mh_CheckThread) thread");
			apiTerminateThread(mh_CheckThread, 100);
		}

		CloseHandle(mh_CheckThread);
		mh_CheckThread = nullptr;
	}

	//if (mh_StopThread)
	//{
	//	CloseHandle(mh_StopThread);
	//	mh_StopThread = nullptr;
	//}

	DeleteBadTempFiles();

	Inet.Deinit(true);

	msz_ConfirmSource.Clear();

	if (((m_UpdateStep == UpdateStep::ExitAndUpdate) || (m_UpdateStep == UpdateStep::PostponeUpdate)) && msz_PendingBatchFile.IsValid())
	{
		WaitAllInstances();

		wchar_t szCmd[] = L"cmd.exe";
		// Double quotation is required due to cmd dequote rules
		const CEStr szParm(L"/c \"\"", msz_PendingBatchFile.GetFilePath(), L"\"\"");

		// Наверное на Elevated процесс это не распространится, но для четкости - взведем флажок
		SetEnvironmentVariable(ENV_CONEMU_INUPDATE_W, ENV_CONEMU_INUPDATE_YES);

		// ghWnd уже закрыт
		const auto nShellRc = reinterpret_cast<INT_PTR>(ShellExecute(
			nullptr, bNeedRunElevation ? L"runas" : L"open", szCmd, szParm.ms_Val,
			nullptr, SW_SHOWMINIMIZED));
		if (nShellRc <= 32)
		{
			wchar_t szErrInfo[MAX_PATH*4];
			swprintf_c(szErrInfo,
				L"Failed to start update batch\n%s\nError code=%i", msz_PendingBatchFile.GetFilePath(), (int)nShellRc);
			MsgBox(szErrInfo, MB_ICONSTOP|MB_SYSTEMMODAL, L"ConEmu", nullptr, false);

			msz_PendingBatchFile.Release(true);
			if (!(mp_Set && mp_Set->isUpdateLeavePackages))
				msz_PendingPackageFile.Release(true);
		}
	}
	msz_PendingBatchFile.Release(false);
	msz_PendingPackageFile.Release(false);

	if (mp_Set)
	{
		delete mp_Set;
		mp_Set = nullptr;
	}
}

void CConEmuUpdate::StartCheckProcedure(const UpdateCallMode callMode)
{
	//DWORD nWait = WAIT_OBJECT_0;

	if ((InUpdate() != UpdateStep::NotStarted) || (mn_InShowMsgBox > 0))
	{
		// Already in update procedure
		if ((m_UpdateStep == UpdateStep::PostponeUpdate) || (m_UpdateStep == UpdateStep::ExitAndUpdate))
		{
			if (gpConEmu && (m_UpdateStep == UpdateStep::ExitAndUpdate))
			{
				// Повторно?
				gpConEmu->CallMainThread(true, RequestExitUpdate, 0);
			}
			else if (ms_NewVersion[0] && (m_UpdateStep == UpdateStep::PostponeUpdate))
			{
				CEStr lsMsg;
				LPCWSTR pszFormat = L"Update to version %s will be started when you close ConEmu window";
				INT_PTR cchMax = wcslen(pszFormat) + wcslen(ms_NewVersion) + 3;
				msprintf(lsMsg.GetBuffer(cchMax), cchMax, pszFormat, ms_NewVersion);
				Icon.ShowTrayIcon(lsMsg, tsa_Source_Updater);
			}
		}
		else if (callMode != UpdateCallMode::Automatic)
		{
			MBoxError(L"Checking for updates already started");
		}
		return;
	}

	gpSet->UpdSet.lastUpdateCheck = std::chrono::system_clock::now();

	// Сразу проверим, как нужно будет запускаться
	bNeedRunElevation = NeedRunElevation();

	mb_RequestTerminate = false;
	//if (!mh_StopThread)
	//	mh_StopThread = CreateEvent(nullptr, TRUE/*manual*/, FALSE, nullptr);
	//ResetEvent(mh_StopThread);

	// Запомнить текущие параметры обновления
	if (!mp_Set)
		mp_Set = new ConEmuUpdateSettings;
	mp_Set->LoadFrom(&gpSet->UpdSet);

	m_ManualCallMode = callMode;

	// Don't clear it here, must be cleared by display procedure
	_ASSERTE(ms_LastErrorInfo == nullptr);

	wchar_t szReason[128];
	wchar_t szErrMsg[255];

	if (!mp_Set->UpdatesAllowed(szReason))
	{
		wcscpy_c(szErrMsg, L"Updates are not enabled in ConEmu settings\r\n");
		wcscat_c(szErrMsg, szReason);
		DisplayLastError(szErrMsg, -1);
		return;
	}

	if (gpConEmu)
	{
		gpConEmu->LogString(callMode != UpdateCallMode::Automatic
			? L"Update: starting in manual mode"
			: L"Update: starting in automatic mode");
	}

	mb_InCheckProcedure = TRUE;
	mh_CheckThread = apiCreateThread(CheckThreadProc, this, &mn_CheckThreadId, "AutoUpdateThread");
	if (!mh_CheckThread)
	{
		mb_InCheckProcedure = FALSE;
		const DWORD nErrCode = GetLastError();
		wcscpy_c(szErrMsg, L"ConEmu automatic update check failed!\r\n");
		if (nErrCode == ERROR_ACCESS_DENIED) wcscat_c(szErrMsg, L"Check your antivirus software\r\n");
		wcscat_c(szErrMsg, L"\r\nCreateThread(CheckThreadProc) failed\r\n");
		DisplayLastError(szErrMsg, nErrCode);
		return;
	}
	// OK
}

void CConEmuUpdate::RequestTerminate()
{
	if (gpConEmu)
		gpConEmu->LogString(L"Update: TERMINATE WAS REQUESTED");
	Inet.DownloadCommand(dc_RequestTerminate, 0, nullptr);
	mb_RequestTerminate = true;
}

void CConEmuUpdate::StopChecking()
{
	if (MsgBox(L"Are you sure, stop updates checking?", MB_SYSTEMMODAL|MB_ICONQUESTION|MB_YESNO, ms_DefaultTitle, nullptr, false) != IDYES)
		return;

	RequestTerminate();

	if (mh_CheckThread)
	{
		DWORD nWait;
		if ((nWait = WaitForSingleObject(mh_CheckThread, 0)) == WAIT_TIMEOUT)
		{
			RequestTerminate();
			nWait = WaitForSingleObject(mh_CheckThread, UPDATE_THREAD_TIMEOUT);
		}

		if (nWait != WAIT_OBJECT_0)
		{
			_ASSERTE(FALSE && "Terminating Updater(mh_CheckThread) thread");
			apiTerminateThread(mh_CheckThread, 100);
		}
	}

	DeleteBadTempFiles();
	m_UpdateStep = UpdateStep::NotStarted;

	msz_PendingBatchFile.Release(true);
	msz_PendingPackageFile.Release(true);
}

LRESULT CConEmuUpdate::ShowLastError(LPARAM apObj)
{
	_ASSERTE(gpUpd == (CConEmuUpdate*)apObj);
	// Now we store error text in member var protected by CS
	CEStr szError; // (wchar_t*)apErrText;
	{
		MSectionLockSimple CS;
		if (CS.Lock(&gpUpd->ms_ErrorLock, 5000))
		{
			szError.Set(gpUpd->ms_LastErrorInfo);
			gpUpd->ms_LastErrorInfo.Clear();
		}
		else
		{
			szError.Set(L"Updater error was occurred but we failed to lock it");
		}
	}

	if (!szError.IsEmpty())
	{
		InterlockedIncrement(&gpUpd->mn_InShowMsgBox);

		MsgBox(szError, MB_ICONINFORMATION, gpConEmu?gpConEmu->GetDefaultTitle():L"ConEmu Update");

		InterlockedDecrement(&gpUpd->mn_InShowMsgBox);
	}

	return 0;
}

LRESULT CConEmuUpdate::QueryConfirmationCallback(LPARAM lParam)
{
	if (!gpUpd || gpUpd->mb_RequestTerminate)
		return 0;
	return gpUpd->QueryConfirmation(static_cast<UpdateStep>(LOWORD(lParam)));
}

// Worker thread
DWORD CConEmuUpdate::CheckThreadProc(LPVOID lpParameter)
{
	DWORD nRc = 0;
	CConEmuUpdate *pUpdate = static_cast<CConEmuUpdate*>(lpParameter);
	_ASSERTE(pUpdate!=nullptr && pUpdate->mn_CheckThreadId==GetCurrentThreadId());

	SAFETRY
	{
		nRc = pUpdate->CheckProcInt();
	}
	SAFECATCH
	{
		pUpdate->ReportError(L"Exception in CheckProcInt", 0);
	}

	pUpdate->mb_InCheckProcedure = FALSE;
	// May be null, if update package was dropped on ConEmu icon
	if (gpConEmu && ghWnd)
	{
		gpConEmu->UpdateProgress();
	}
	return nRc;
}

#if 0
bool CConEmuUpdate::CanUpdateInstallation()
{
	if (UpdateDownloadSetup() == 1)
	{
		// Если через Setupper - то msi сам разберется и ругнется когда надо
		return true;
	}

	// Раз дошли сюда - значит ConEmu был просто "распакован"

	if (IsUserAdmin())
	{
		// ConEmu запущен "Под администратором", проверки не нужны
		return true;
	}

	wchar_t szTestFile[MAX_PATH*2];
	wcscpy_c(szTestFile, gpConEmu->ms_ConEmuExeDir);
	wcscat_c(szTestFile, L"\\ConEmuUpdate.check");

	HANDLE hFile = CreateFile(szTestFile, GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_TEMPORARY, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD nErr = GetLastError();
		wcscpy_c(szTestFile, L"Can't update installation folder!\r\n");
		wcscat_c(szTestFile, gpConEmu->ms_ConEmuExeDir);
		DisplayLastError(szTestFile, nErr);
		return false;
	}
	CloseHandle(hFile);
	DeleteFile(szTestFile);

	// OK
	return true;
}
#endif

// static
bool CConEmuUpdate::LocalUpdate(LPCWSTR asDownloadedPackage)
{
	bool bOk = false;

	if (!gpUpd)
		gpUpd = new CConEmuUpdate;

	if (gpUpd)
		bOk = gpUpd->StartLocalUpdate(asDownloadedPackage);

	return bOk;
}

// static
bool CConEmuUpdate::IsUpdatePackage(LPCWSTR asFilePath)
{
	bool bIsUpdatePackage = false;

	if (asFilePath && *asFilePath && lstrlen(asFilePath) <= MAX_PATH)
	{
		wchar_t szName[MAX_PATH+1];
		LPCWSTR pszName = PointToName(asFilePath);
		if (pszName && *pszName)
		{
			lstrcpyn(szName, pszName, countof(szName));
			CharLowerBuff(szName, lstrlen(szName));
			LPCWSTR pszExt = PointToExt(szName);
			if (pszExt)
			{
				if ((wcsncmp(szName, L"conemupack.", 11) == 0)
					&& (wcscmp(pszExt, L".7z") == 0))
				{
					bIsUpdatePackage = true;
				}
				else if ((wcsncmp(szName, L"conemusetup.", 12) == 0)
					&& (wcscmp(pszExt, L".exe") == 0))
				{
					bIsUpdatePackage = true;
				}
			}
		}
	}

	return bIsUpdatePackage;
}

bool CConEmuUpdate::StartLocalUpdate(LPCWSTR asDownloadedPackage)
{
	bool bRc = false;
	HANDLE hTarget = nullptr;
	TempFile pszLocalPackage;
	TempFile pszBatchFile;
	DWORD nLocalCRC = 0;
	BOOL lbDownloadRc = FALSE, lbExecuteRc = FALSE;
	int iConfirmUpdate = -1;
	ErrorInfo error{};

	const wchar_t pszPackPref[] = L"conemupack.";
	const size_t lnPackPref = _tcslen(pszPackPref);
	const wchar_t  pszSetupPref[] = L"conemusetup.";
	const size_t lnSetupPref = _tcslen(pszSetupPref);

	const wchar_t* pszName = PointToName(asDownloadedPackage);
	const wchar_t* pszExt = PointToExt(pszName);

	_ASSERTE(gpConEmu && isMainThread());
	
	try {

		if (InUpdate() != UpdateStep::NotStarted)
		{
			const wchar_t* errMsg = L"Checking for updates already started";
			MBoxError(errMsg);
			throw ErrorInfo(errMsg);
		}

		if (mb_InCheckProcedure)
		{
			Assert(mb_InCheckProcedure == FALSE);
			throw ErrorInfo(L"Already in the check procedure");
		}

		DeleteBadTempFiles();
		Inet.Deinit(true);

		if (!pszName || !*pszName || !pszExt || !*pszExt)
		{
			AssertMsg(L"Invalid asDownloadedPackage");
			throw ErrorInfo(L"Invalid asDownloadedPackage");
		}

		// Запомнить текущие параметры обновления
		if (!mp_Set)
			mp_Set = new ConEmuUpdateSettings;
		mp_Set->LoadFrom(&gpSet->UpdSet);

		m_ManualCallMode = UpdateCallMode::Manual;

		mb_RequestTerminate = false;

		// Don't clear it here, must be cleared by display procedure
		_ASSERTE(ms_LastErrorInfo == nullptr);

		ms_NewVersion[0] = 0;

		if ((lstrcmpni(pszName, pszPackPref, lnPackPref) == 0)
			&& (lstrcmpi(pszExt, L".7z") == 0)
			&& (((pszExt - pszName) - lnPackPref + 1) < sizeof(ms_NewVersion)))
		{
			// Check it was NOT installed with "Setupper"
			if (mp_Set->UpdateDownloadSetup() == 1)
			{
				DontEnable de;
				const LPCWSTR pszConfirm = L"ConEmu was installed with setup!\nAre you sure to update installation with 7zip?";
				const int iBtn = MsgBox(pszConfirm, MB_ICONEXCLAMATION | MB_SETFOREGROUND | MB_SYSTEMMODAL | MB_YESNO | MB_DEFBUTTON2, ms_DefaultTitle, nullptr, false);
				if (iBtn != IDYES)
				{
					throw ErrorInfo(L"Installer installation, don't update with 7zip");
				}
			}

			if (!Check7zipInstalled())
			{
				// TODO: Download 7za.exe from conemu.github.io
				throw ErrorInfo(L"7zip is not found");
			}

			// Forcing usage of 7zip package!
			mp_Set->isUpdateDownloadSetup = 2;

			//if (!CanUpdateInstallation())
			//{
			//	// Значит 7zip обломается при попытке распаковки
			//	goto wrap;
			//}

			// OK
			const size_t nLen = (pszExt - pszName) - lnPackPref;
			wmemmove_s(ms_NewVersion, countof(ms_NewVersion), pszName + lnPackPref, nLen);
			ms_NewVersion[nLen] = 0;
		}
		else if ((lstrcmpni(pszName, pszSetupPref, lnSetupPref) == 0)
			&& (lstrcmpi(pszExt, L".exe") == 0)
			&& (((pszExt - pszName) - lnSetupPref + 1) < sizeof(ms_NewVersion)))
		{
			// Must be installed with "Setupper"
			if (mp_Set->UpdateDownloadSetup() != 1)
			{
				const wchar_t* errMsg = L"ConEmu was not installed with setup! Can't update!";
				MBoxError(errMsg);
				throw ErrorInfo(errMsg);
			}

			// OK
			const size_t nLen = (pszExt - pszName) - lnSetupPref;
			wmemmove_s(ms_NewVersion, countof(ms_NewVersion), pszName + lnSetupPref, nLen);
			ms_NewVersion[nLen] = 0;
		}
		else
		{
			const wchar_t* errMsg = L"Invalid asDownloadedPackage (2)";
			AssertMsg(errMsg);
			throw ErrorInfo(errMsg);
		}


		// Сразу проверим, как нужно будет запускаться
		bNeedRunElevation = NeedRunElevation();

		// StartLocalUpdate - запуск обновления из локального пакета

		mb_InetMode = false;
		mb_DroppedMode = true;


		if (!pszLocalPackage.CreateTempFile(mp_Set->szUpdateDownloadPath, PointToName(asDownloadedPackage), error))
			throw ErrorInfo(error);
		pszLocalPackage.CloseFile();

		lbDownloadRc = DownloadFile(asDownloadedPackage, pszLocalPackage.GetFilePath(), nLocalCRC, TRUE);
		if (!lbDownloadRc)
			throw ErrorInfo(L"Failed to download the file");

		if (mb_RequestTerminate)
			throw ErrorInfo(L"Update cancelled, termination was requested");

		pszBatchFile = CreateBatchFile(pszLocalPackage.GetFilePath());
		if (!pszBatchFile.IsValid())
			throw ErrorInfo(L"Failed to create update batch file");

		iConfirmUpdate = static_cast<int>(gpConEmu->CallMainThread(true, QueryConfirmationCallback, static_cast<LPARAM>(UpdateStep::ConfirmUpdate)));
		if ((iConfirmUpdate <= 0) || (iConfirmUpdate == IDCANCEL))
			throw ErrorInfo(L"Update cancelled because of confirmation");

		Assert(m_ManualCallMode == UpdateCallMode::Manual);
		Assert(!msz_PendingBatchFile.IsValid());

		msz_PendingPackageFile = std::move(pszLocalPackage);
		msz_PendingBatchFile = std::move(pszBatchFile);

		if (iConfirmUpdate == IDYES)
		{
			m_UpdateStep = UpdateStep::ExitAndUpdate;
			if (gpConEmu) gpConEmu->CallMainThread(false, RequestExitUpdate, 0);
		}
		else
		{
			m_UpdateStep = UpdateStep::PostponeUpdate;
			if (gpConEmu) gpConEmu->UpdateProgress();
		}
		lbExecuteRc = TRUE;
		return true;
	}
	catch (const ErrorInfo& ex)
	{
		if (gpConEmu)
			gpConEmu->LogString(ex.What());
		
		_ASSERTE(!msz_DeletePackageFile.IsValid());
		msz_DeletePackageFile.Release();
		if (pszLocalPackage.IsValid())
		{
			if (!lbDownloadRc || (!lbExecuteRc && !mp_Set->isUpdateLeavePackages))
				msz_DeletePackageFile = std::move(pszLocalPackage);
			else
				pszLocalPackage.Release();
		}

		_ASSERTE(!msz_DeleteBatchFile.IsValid());
		msz_DeleteBatchFile.Release();
		if (pszBatchFile.IsValid())
		{
			if (!lbExecuteRc)
				msz_DeleteBatchFile = std::move(pszBatchFile);
			else
				pszBatchFile.Release();
		}

		m_UpdateStep = UpdateStep::NotStarted;
		mb_DroppedMode = false;

		return false;
	}
}

void CConEmuUpdate::GetVersionsFromIni(LPCWSTR pszUpdateVerLocation, wchar_t (&szServer)[100], wchar_t (&szServerRA)[100], wchar_t (&szInfo)[100])
{
	wchar_t szTest[64]; // Дописать stable/preview/alpha
	bool bDetected = false, bNewer;

	wcscpy_c(szInfo, ms_OurVersion);

	struct {
		LPCWSTR szSect, szPref, szName;
	} Vers[] = {
		{sectionConEmuStable,        CV_Stable L":\t",  L" " CV_STABLE },
		{sectionConEmuPreview, L"\n" CV_Preview L":\t", L" " CV_PREVIEW},
		{sectionConEmuDevel,   L"\n" CV_Devel L":\t",   L" " CV_DEVEL  }
	};

	szServer[0] = 0;
	szServerRA[0] = 0;

	for (size_t i = 0; i < countof(Vers); i++)
	{
		wcscat_c(szServer, Vers[i].szPref);
		if (GetPrivateProfileString(Vers[i].szSect, L"version", L"", szTest, countof(szTest), pszUpdateVerLocation))
		{
			bNewer = (lstrcmp(szTest, ms_OurVersion) >= 0);
			if (!bDetected && bNewer)
			{
				bDetected = true;
				wcscat_c(szInfo, Vers[i].szName);
			}
			szTest[10] = 0;
			wcscat_c(szServer, szTest);
			wcscat_c(szServerRA, L"    ");
			wcscat_c(szServerRA, szTest);
			wcscat_c(szServerRA, Vers[i].szName);
			if (bNewer)
			{
				wcscat_c(szServer, (lstrcmp(szTest, ms_OurVersion) > 0) ? L" (newer)" : L" (equal)");
				wcscat_c(szServerRA, (lstrcmp(szTest, ms_OurVersion) > 0) ? L" (newer)" : L" (equal)");
			}
			wcscat_c(szServerRA, L"\n");
		}
		else
		{
			wcscat_c(szServer, L"<Not found>");
		}
	}

	if (szServerRA[0] == 0)
	{
		wcscpy_c(szServerRA, L"<Not found>");
	}
}

DWORD CConEmuUpdate::CheckProcInt()
{
	BOOL lbDownloadRc = FALSE, lbExecuteRc = FALSE;
	TempFile szLocalPackage;
	TempFile szBatchFile;
	wchar_t* pszSource, * pszEnd;
	DWORD nSrcLen, nSrcCRC, nLocalCRC = 0;
	bool lbSourceLocal = false;
	int iConfirmUpdate = -1;

	try
	{

#if 0
		// Under Windows7 we may get a debug warning
		// if conhost.exe was not found yet for all
		// started consoles...
		if (m_ManualCallMode == UpdateCallMode::Automatic)
			Sleep(2500);
#endif

		_ASSERTE(m_UpdateStep == UpdateStep::NotStarted);
		m_UpdateStep = UpdateStep::Check;

		DeleteBadTempFiles();

		if (gpConEmu)
			gpConEmu->LogString(L"Update: Initializing internet");

		// This implies Inet.Deinit(false) too
		if (!Inet.Init(this))
		{
			const wchar_t* errInfo = L"Update: Internet failed";
			if (gpConEmu)
				gpConEmu->LogString(errInfo);
			throw ErrorInfo(errInfo);
		}

		// Download and parse latest version information
		while (!LoadVersionInfoFromServer())
		{
			const LRESULT lRetryRc = (!gpConEmu || m_ManualCallMode == UpdateCallMode::Automatic || mb_RequestTerminate)
				? IDCANCEL
				: gpConEmu->CallMainThread(true/*bSync*/, CConEmuUpdate::QueryRetryVersionCheck, reinterpret_cast<LPARAM>(szRetryVersionIniCheck));

			// Error must be already shown or logged, clear it
			if (ms_LastErrorInfo)
			{
				MSectionLockSimple CS;
				if (CS.Lock(&ms_ErrorLock, 5000))
				{
					ms_LastErrorInfo.Release();
				}
			}

			if (lRetryRc != IDRETRY)
			{
				throw ErrorInfo(L"Update cancelled after confirmation");
			}
		}

		if ((lstrcmpi(ms_NewVersion, ms_OurVersion) <= 0)
			// If user declined the update in that session - don't suggest that version on hourly update checks
			|| (m_ManualCallMode == UpdateCallMode::Automatic && (lstrcmp(ms_NewVersion, ms_SkipVersion) == 0)))
		{
			mb_NewVersionAvailable = false;

			if (m_ManualCallMode != UpdateCallMode::Automatic)
			{
				// No newer %s version is available
				gpConEmu->CallMainThread(true, QueryConfirmationCallback, static_cast<LPARAM>(UpdateStep::NotStarted));
			}

			throw ErrorInfo(L"Update cancelled, no newer version was found");
		}

		pszSource = ms_SourceFull.ms_Val;
		nSrcLen = wcstoul(pszSource, &pszEnd, 10);
		if (!nSrcLen || !pszEnd || *pszEnd != L',' || *(pszEnd + 1) != L'x')
		{
			ReportError(L"Invalid format in version description (size)\n%s", ms_SourceFull, 0);
			throw ErrorInfo(L"Update cancelled, invalid format, size");
		}
		mn_PackageSize = nSrcLen;
		pszSource = pszEnd + 2;
		nSrcCRC = wcstoul(pszSource, &pszEnd, 16);
		if (!nSrcCRC || !pszEnd || *pszEnd != L',')
		{
			ReportError(L"Invalid format in version description (CRC32)\n%s", ms_SourceFull, 0);
			throw ErrorInfo(L"Update cancelled, invalid format, CRC32");
		}
		pszSource = pszEnd + 1;
		lbSourceLocal = IsLocalFile(pszSource);

		if (mb_RequestTerminate)
			throw ErrorInfo(L"Update cancelled, termination was requested");

		// It returns true, if updating with "exe" installer.
		if (!Check7zipInstalled())
			throw ErrorInfo(L"Update cancelled, 7zip is not installed"); // Error already reported

		msz_ConfirmSource.Set(pszSource);

		if (gpConEmu && !gpConEmu->CallMainThread(true, QueryConfirmationCallback, static_cast<LPARAM>(UpdateStep::ConfirmDownload)))
		{
			// Если пользователь отказался от обновления в этом сеансе - не предлагать ту же версию при ежечасных проверках
			wcscpy_c(ms_SkipVersion, ms_NewVersion);
			throw ErrorInfo(L"Update cancelled, version skipped");
		}

		mn_InternetContentReady = 0;
		m_UpdateStep = UpdateStep::Downloading;
		// May be null, if update package was dropped on ConEmu icon
		if (gpConEmu && ghWnd)
		{
			gpConEmu->UpdateProgress();
		}

		// Download the package
		while (!mb_RequestTerminate)
		{
			LARGE_INTEGER liSize = {};
			lbDownloadRc = FALSE;

			if (gpConEmu)
			{
				CEStr lsInfo(L"Update: Downloading package: ", pszSource);
				gpConEmu->LogString(lsInfo);
			}

			_ASSERTE(!ms_TemplFilename.IsEmpty());
			ErrorInfo error;
			if (!szLocalPackage.CreateTempFile(mp_Set->szUpdateDownloadPath, ms_TemplFilename, error))
			{
				ReportErrorInt(error);
			}
			else
			{
				szLocalPackage.CloseFile();
				// liSize returns only .LowPart yet
				lbDownloadRc = DownloadFile(pszSource, szLocalPackage.GetFilePath(), nLocalCRC, TRUE, &liSize);
				if (lbDownloadRc)
				{
					wchar_t szInfo[2048];
					if (liSize.HighPart || liSize.LowPart != nSrcLen)
					{
						swprintf_c(szInfo, L"%s\nRequired size=%u, local size=%u", pszSource, nSrcLen, liSize.LowPart);
						ReportError(L"Downloaded file does not match\n%s\n%s", szLocalPackage.GetFilePath(), szInfo, 0);
						lbDownloadRc = FALSE;
					}
					else if (nLocalCRC != nSrcCRC)
					{
						swprintf_c(szInfo, L"Required CRC32=x%08X, local CRC32=x%08X", nSrcCRC, nLocalCRC);
						ReportError(L"Invalid local file\n%s\n%s", szLocalPackage.GetFilePath(), szInfo, 0);
						lbDownloadRc = FALSE;
					}
				}
			}

			// Succeeded
			if (lbDownloadRc)
				break;

			// Now we show an error even in automatic mode because
			// the download was started after user confirmation
			const LRESULT lRetryRc = (!gpConEmu || mb_RequestTerminate)
				? IDCANCEL
				: gpConEmu->CallMainThread(true/*bSync*/, CConEmuUpdate::QueryRetryVersionCheck, reinterpret_cast<LPARAM>(szRetryPackageDownload));

			// Error must be already shown or logged, clear it
			if (ms_LastErrorInfo)
			{
				MSectionLockSimple CS;
				if (CS.Lock(&ms_ErrorLock, 5000))
				{
					ms_LastErrorInfo.Clear();
				}
			}

			if (lRetryRc != IDRETRY)
			{
				throw ErrorInfo(L"Update cancelled after confirmation");
			}
		}

		Inet.Deinit(true);

		if (mb_RequestTerminate)
			throw ErrorInfo(L"Update cancelled, termination was requested");

		if (gpConEmu)
			gpConEmu->LogString(L"Update: Creating update script");

		szBatchFile = CreateBatchFile(szLocalPackage.GetFilePath());
		if (!szBatchFile.IsValid())
			throw ErrorInfo(L"Failed to create update batch file");

		iConfirmUpdate = static_cast<int>(gpConEmu->CallMainThread(true, QueryConfirmationCallback, static_cast<LPARAM>(UpdateStep::ConfirmUpdate)));
		if ((iConfirmUpdate <= 0) || (iConfirmUpdate == IDCANCEL))
		{
			// If user declined the update in that session - don't suggest that version on hourly update checks
			wcscpy_c(ms_SkipVersion, ms_NewVersion);
			throw ErrorInfo(L"Update cancelled, version skipped by user");
		}

		msz_PendingPackageFile = std::move(szLocalPackage);
		msz_PendingBatchFile = std::move(szBatchFile);

		if (gpConEmu)
			gpConEmu->LogString(L"Update: Ready to update on exit");
		if (iConfirmUpdate == IDYES)
		{
			m_UpdateStep = UpdateStep::ExitAndUpdate;
			if (gpConEmu) gpConEmu->CallMainThread(false, RequestExitUpdate, 0);
		}
		else
		{
			m_UpdateStep = UpdateStep::PostponeUpdate;
			if (gpConEmu) gpConEmu->UpdateProgress();
		}

		lbExecuteRc = TRUE;
	}
	catch (const ErrorInfo& ex)
	{
		if (gpConEmu)
			gpConEmu->LogString(ex.What());

		if (!lbExecuteRc && gpConEmu && ms_LastErrorInfo)
		{
			gpConEmu->CallMainThread(false, ShowLastError, reinterpret_cast<LPARAM>(this));
		}

		_ASSERTE(!msz_DeletePackageFile.IsValid());
		msz_DeletePackageFile.Release();
		if (szLocalPackage.IsValid())
		{
			if (!lbDownloadRc || (!lbExecuteRc && !mp_Set->isUpdateLeavePackages))
				msz_DeletePackageFile = std::move(szLocalPackage);
			else
				szLocalPackage.Release();
		}

		_ASSERTE(!msz_DeleteBatchFile.IsValid());
		msz_DeleteBatchFile.Release();
		if (szBatchFile.IsValid())
		{
			if (!lbExecuteRc)
				msz_DeleteBatchFile = std::move(szBatchFile);
			else
				szBatchFile.Release();
		}

		if (!lbExecuteRc)
			m_UpdateStep = UpdateStep::NotStarted;
	}

	Inet.Deinit(true);

	std::ignore = lbSourceLocal;
	mb_InCheckProcedure = FALSE;
	return 0;
}

// Load information about current releases
bool CConEmuUpdate::LoadVersionInfoFromServer()
{
	bool bOk = false;

	// E.g. https://conemu.github.io/version.ini
	LPCWSTR pszUpdateVerLocationSet = mp_Set->UpdateVerLocation();
	LPCWSTR pszUpdateVerLocation = nullptr;
	wchar_t szSection[64], szItem[64];

	try {

		// Logging
		if (gpConEmu)
		{
			const CEStr lsInfo(L"Update: Loading versions from server: ", pszUpdateVerLocationSet);
			gpConEmu->LogString(lsInfo);
		}

		ms_VersionUrlCheck.Set(pszUpdateVerLocationSet);

		// Erase previous temp file if any
		ms_TempUpdateVerLocation.Release(true);

		// We need to download information? Or it it is located in the intranet?
		if (IsLocalFile(pszUpdateVerLocationSet))
		{
			pszUpdateVerLocation = pszUpdateVerLocationSet;
		}
		else
		{
			DWORD crc;
			ErrorInfo errInfo;

			// Create temporary file
			if (!ms_TempUpdateVerLocation.CreateTempFile(mp_Set->szUpdateDownloadPath/*L"%TEMP%"*/, L"ConEmuVersion.ini", errInfo))
				throw ErrorInfo(errInfo);
			ms_TempUpdateVerLocation.CloseFile();
			pszUpdateVerLocation = ms_TempUpdateVerLocation.GetFilePath();

			// And download file from server
			const BOOL bInfoRc = DownloadFile(pszUpdateVerLocationSet, pszUpdateVerLocation, crc);
			if (!bInfoRc)
			{
				throw ErrorInfo(L"Failed to download update version information");
			}
		}

		// Logging
		if (gpConEmu)
		{
			const CEStr lsInfo(L"Update: Checking version information: ", pszUpdateVerLocation);
			gpConEmu->LogString(lsInfo);
		}

		// What version user prefers?
		_wcscpy_c(szSection, countof(szSection),
			(mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? sectionConEmuStable
			: (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? sectionConEmuPreview
			: sectionConEmuDevel);
		_wcscpy_c(szItem, countof(szItem), (mp_Set->UpdateDownloadSetup() == 1) ? L"location_exe" : L"location_arc");

		// Check the ini-file
		if (!GetPrivateProfileString(szSection, L"version", L"", ms_NewVersion, countof(ms_NewVersion), pszUpdateVerLocation) || !*ms_NewVersion)
		{
			ReportBrokenIni(szSection, L"version", pszUpdateVerLocationSet, pszUpdateVerLocation);
			throw ErrorInfo(L"Update version information is invalid");
		}

		// URL may not contain file name at all, compile it (predefined)
		if (mp_Set->UpdateDownloadSetup() == 1)
			ms_TemplFilename = CEStr(L"ConEmuSetup.", ms_NewVersion, L".exe");
		else
			ms_TemplFilename = CEStr(L"ConEmuPack.", ms_NewVersion, L".7z");

		ms_SourceFull.Clear();

		if (!GetPrivateProfileString(szSection, szItem, L"", ms_SourceFull.GetBuffer(1024), 1024, pszUpdateVerLocation)
			|| ms_SourceFull.IsEmpty())
		{
			ReportBrokenIni(szSection, szItem, pszUpdateVerLocationSet, pszUpdateVerLocation);
			throw ErrorInfo(L"Update version information is invalid");
		}

		GetVersionsFromIni(pszUpdateVerLocation, ms_VerOnServer, ms_VerOnServerRA, ms_CurVerInfo);

		bOk = true;
	}
	catch (const ErrorInfo& ex)
	{
		if (gpConEmu)
			gpConEmu->LogString(ex.What());
	}
	return bOk;
}

CEStr CConEmuUpdate::CreateVersionOnServerInfo(bool abRightAligned, LPCWSTR asSuffix /*= nullptr*/)
{
	return CEStr(
		L"Versions on server\n",
		ms_VersionUrlCheck,
		L"\n",
		abRightAligned ? ms_VerOnServerRA : ms_VerOnServer,
		asSuffix);
}

LRESULT CConEmuUpdate::QueryRetryVersionCheck(LPARAM lParam)
{
	LRESULT lRc = IDCANCEL;
	CEStr szError;
	const wchar_t* pszMessage = reinterpret_cast<const wchar_t*>(lParam);

	// if the termination was requested
	if (!gpUpd || !gpConEmu || gpUpd->mb_RequestTerminate)
	{
		goto wrap;
	}

	// to avoid infinite recursion
	if (gpUpd->mn_InShowMsgBox > 0)
	{
		InterlockedIncrement(&gpUpd->mn_ErrorInfoSkipCount);
		goto wrap;
	}

	InterlockedIncrement(&gpUpd->mn_ErrorInfoCount);

	_ASSERTE(isMainThread());

	// Concatenate error details if any
	if (gpUpd->ms_LastErrorInfo)
	{
		MSectionLockSimple CS;
		if (CS.Lock(&gpUpd->ms_ErrorLock, 5000))
		{
			szError = CEStr(pszMessage, L"\n\n", gpUpd->ms_LastErrorInfo);
			gpUpd->ms_LastErrorInfo.Clear();
		}
		else
		{
			szError = CEStr(pszMessage, L"\n\n", L"Updater error was occurred but we failed to lock it");
		}
	}
	else
	{
		szError = pszMessage;
	}

	// Now show the error
	if (!szError.IsEmpty())
	{
		InterlockedIncrement(&gpUpd->mn_InShowMsgBox);

		lRc = MsgBox(szError, MB_ICONEXCLAMATION|MB_RETRYCANCEL, gpConEmu?gpConEmu->GetDefaultTitle():L"ConEmu Update");

		InterlockedDecrement(&gpUpd->mn_InShowMsgBox);
	}

wrap:
	return lRc;
}

TempFile CConEmuUpdate::CreateBatchFile(LPCWSTR asPackage)
{
	BOOL lbRc = FALSE;
	TempFile pszBatch;
	CEStr pszCommand;
	LPCWSTR pszWorkDir = nullptr;
	BOOL lbWrite;
	DWORD nLen, nWritten;
	char szOem[4096];
	LPCWSTR pszFormat = nullptr;
	size_t cchCmdMax = 0;
	ErrorInfo error;

	wchar_t szPID[16]; swprintf_c(szPID, L"%u", GetCurrentProcessId());
	// Setupper bitness will be corrected in mp_Set->UpdateExeCmdLine
	wchar_t szCPU[4]; wcscpy_c(szCPU, WIN3264TEST(L"x86",L"x64"));

	if (!gpConEmu)
	{
		ReportError(L"CreateBatchFile failed, gpConEmu==nullptr", 0);
		goto wrap;
	}

	if (!pszBatch.CreateTempFile(mp_Set->szUpdateDownloadPath, L"ConEmuUpdate.cmd", error))
	{
		goto wrap;
	}

	#define WRITE_BATCH_A(s) \
		nLen = lstrlenA(s); \
		lbWrite = WriteFile(pszBatch.GetHandle(), s, nLen, &nWritten, nullptr); \
		if (!lbWrite || (nLen != nWritten)) { ReportError(L"WriteBatch failed, code=%u", GetLastError()); goto wrap; }

	#define WRITE_BATCH_W(s) \
		nLen = WideCharToMultiByte(CP_OEMCP, 0, s, -1, szOem, countof(szOem), nullptr, nullptr); \
		if (!nLen) { ReportError(L"WideCharToMultiByte failed, len=%i", lstrlen(s)); goto wrap; } \
		WRITE_BATCH_A(szOem);

	WRITE_BATCH_A("@echo off\r\n");

	// "set ConEmuInUpdate=YES"
	WRITE_BATCH_W(L"\r\nset " ENV_CONEMU_INUPDATE_W L"=" ENV_CONEMU_INUPDATE_YES L"\r\n");

	WRITE_BATCH_A("cd /d \"");
	WRITE_BATCH_W(gpConEmu->ms_ConEmuExeDir);
	WRITE_BATCH_A("\\\"\r\necho Current folder\r\ncd\r\necho .\r\n\r\necho Starting update...\r\n");

	// Формат.
	pszFormat = (mp_Set->UpdateDownloadSetup()==1) ? mp_Set->UpdateExeCmdLine(szCPU) : mp_Set->UpdateArcCmdLine();

	// Замена %1 и т.п.
	for (int s = 0; s < 2; s++)
	{
		// На первом шаге - считаем требуемый размер под pszCommand, на втором - формируем команду
		if (s)
		{
			if (!cchCmdMax)
			{
				ReportError(L"Invalid %s update command (%s)", (mp_Set->UpdateDownloadSetup()==1) ? L"exe" : L"arc", pszFormat, 0);
				goto wrap;
			}
			if (!pszCommand.GetBuffer(cchCmdMax + 1))
			{
				ReportError(L"Invalid %s update command (%s), allocation of %i bytes failed",
					(mp_Set->UpdateDownloadSetup() == 1) ? L"exe" : L"arc", pszFormat, static_cast<DWORD>(cchCmdMax + 1));
				goto wrap;
			}
		}
		wchar_t* pDst = pszCommand.data();
		LPCWSTR pszMacro;
		for (LPCWSTR pSrc = pszFormat; *pSrc; pSrc++)
		{
			switch (*pSrc)
			{
			case L'%':
				pSrc++;
				switch (*pSrc)
				{
				case L'%':
					pszMacro = L"%";
					break;
				// "%1"-archive or setup file, "%2"-ConEmu.exe folder, "%3"-x86/x64, "%4"-ConEmu PID
				case L'1':
					pszMacro = asPackage;
					break;
				case L'2':
					pszMacro = gpConEmu->ms_ConEmuExeDir;
					break;
				case L'3':
					pszMacro = szCPU;
					break;
				case L'4':
					pszMacro = szPID;
					break;
				default:
					// Недопустимый управляющий символ, это может быть переменная окружения
					pszMacro = nullptr;
					pSrc--;
					if (s)
						*(pDst++) = L'%';
					else
						cchCmdMax++;
				}

				if (pszMacro)
				{
					size_t cchLen = _tcslen(pszMacro);
					if (s)
					{
						_wcscpy_c(pDst, cchLen+1, pszMacro);
						pDst += cchLen;
					}
					else
					{
						cchCmdMax += cchLen;
					}
				}
				break;
			default:
				if (s)
					*(pDst++) = *pSrc;
				else
					cchCmdMax++;
			}
		}
		if (s)
			*pDst = 0;
	}

	// Выполнить команду обновления
	WRITE_BATCH_A("echo ");
	WRITE_BATCH_W(pszCommand);
	WRITE_BATCH_A("\r\ncall ");
	WRITE_BATCH_W(pszCommand);
	WRITE_BATCH_A("\r\nif errorlevel 1 goto err\r\n");

	// Если юзер просил что-то выполнить после распаковки установки
	if (mp_Set->szUpdatePostUpdateCmd && *mp_Set->szUpdatePostUpdateCmd)
	{
		WRITE_BATCH_A("\r\n");
		WRITE_BATCH_W(mp_Set->szUpdatePostUpdateCmd);
		WRITE_BATCH_A("\r\n");
	}

	// Сброс переменной окружения: "set ConEmuInUpdate="
	WRITE_BATCH_W(L"\r\nset " ENV_CONEMU_INUPDATE_W L"=\r\n");

	// Start updated ConEmu instance
	WRITE_BATCH_A("\r\necho Starting ConEmu...\r\n");
	// Return working directory
	pszWorkDir = gpConEmu->WorkDir();
	if (pszWorkDir && *pszWorkDir)
	{
		WRITE_BATCH_A("cd /d \"");
		WRITE_BATCH_W(pszWorkDir);
		WRITE_BATCH_A("\\\"\r\n");
	}
	// start "ConEmu" -demote -cmd ConEmu.exe ...
	WRITE_BATCH_A("start \"ConEmu\" \"");
	WRITE_BATCH_W(gpConEmu->ms_ConEmuExe);
	WRITE_BATCH_A("\" ");
	if (bNeedRunElevation)
	{
		WRITE_BATCH_A("-demote -run \"");
		WRITE_BATCH_W(gpConEmu->ms_ConEmuExe);
		WRITE_BATCH_A("\" ");
	}
	if (!gpConEmu->opt.cfgSwitches.IsEmpty())
	{
		WRITE_BATCH_W(gpConEmu->opt.cfgSwitches);
		WRITE_BATCH_A(" ");
	}
	if (!gpConEmu->opt.cmdRunCommand.IsEmpty())
	{
		WRITE_BATCH_W(gpConEmu->opt.cmdRunCommand);
	}
	// Fin
	WRITE_BATCH_A("\r\ngoto fin\r\n");

	// Сообщение об ошибке?
	WRITE_BATCH_A("\r\n:err\r\n");
	WRITE_BATCH_A((mp_Set->UpdateDownloadSetup()==1) ? "echo \7Installation failed\7" : "echo \7Extraction failed\7\r\n");
	WRITE_BATCH_A("\r\npause\r\n:fin\r\n");

	// Грохнуть пакет обновления
	if (!mp_Set->isUpdateLeavePackages)
	{
		WRITE_BATCH_A("del \"");
		WRITE_BATCH_W(asPackage);
		WRITE_BATCH_A("\"\r\n");
	}

	// Грохнуть сам батч и позвать "exit" чтобы в консоли
	// не появлялось "Batch not found" при попытке выполнить следующую строку файла
	WRITE_BATCH_A("del \"%~0\" & exit\r\n");

	//// Для отладки
	//WRITE_BATCH_A("\r\npause\r\n");

	// Succeeded
	lbRc = TRUE;
wrap:
	pszBatch.CloseFile();

	if (!lbRc)
	{
		pszBatch.Release(true);
	}

	return pszBatch;
}

CConEmuUpdate::UpdateStep CConEmuUpdate::InUpdate()
{
	AssertThisRet(UpdateStep::NotStarted);

	DWORD nWait = WAIT_OBJECT_0;

	if (mh_CheckThread)
		nWait = WaitForSingleObject(mh_CheckThread, 0);

	switch (m_UpdateStep)
	{
	case UpdateStep::Check:
	case UpdateStep::ConfirmDownload:
	case UpdateStep::ConfirmUpdate:
		if (nWait == WAIT_OBJECT_0)
			m_UpdateStep = UpdateStep::NotStarted;
		break;
	case UpdateStep::PostponeUpdate:
	case UpdateStep::ExitAndUpdate:
		// Тут у нас нить уже имеет право завершиться
		break;
	default:
		;
	}

	return m_UpdateStep;
}

// This checks if file is located on local drive
// (has "file://" prefix, or "\\server\share\..." or "X:\path\...")
// and set asPathOrUrl back to local path (if prefix was specified)
bool CConEmuUpdate::IsLocalFile(LPCWSTR& asPathOrUrl)
{
	LPWSTR psz = (LPWSTR)asPathOrUrl;
	bool lbLocal = IsLocalFile(psz);
	asPathOrUrl = psz;
	return lbLocal;
}

// This checks if file is located on local drive
// (has "file://" prefix, or "\\server\share\..." or "X:\path\...")
// and set asPathOrUrl back to local path (if prefix was specified)
// Function DOES NOT modify the contents of buffer pointed by asPathOrUrl!
bool CConEmuUpdate::IsLocalFile(LPWSTR& asPathOrUrl)
{
	if (!asPathOrUrl || !*asPathOrUrl)
	{
		_ASSERTE(asPathOrUrl && *asPathOrUrl);
		return true;
	}

	if (asPathOrUrl[0] == L'\\' && asPathOrUrl[1] == L'\\')
		return true; // network or UNC
	if (asPathOrUrl[1] == L':')
		return true; // Local drive

	wchar_t szPrefix[8]; // "file:"
	lstrcpyn(szPrefix, asPathOrUrl, countof(szPrefix));
	if (lstrcmpi(szPrefix, L"file://") == 0)
	{
		asPathOrUrl += 7;
		if (asPathOrUrl[0] == L'/' && isDriveLetter(asPathOrUrl[1]) && asPathOrUrl[2] == L':')
			asPathOrUrl++;
		return true; // "file:" protocol
	}

	return false;
}

BOOL CConEmuUpdate::DownloadFile(LPCWSTR asSource, LPCWSTR asTarget, DWORD& crc, BOOL abPackage /*= FALSE*/, LARGE_INTEGER* rpSize /*= nullptr*/)
{
	bool lbRc = false;
	BOOL lbRead = FALSE, lbWrite = FALSE;
	CEDownloadErrorArg args[3] = {};

	MCHKHEAP;

	mn_InternetContentReady = 0;

	if (gpConEmu)
	{
		CEStr lsMsg(L"Update: Downloading ", asSource, L" to ", asTarget);
		gpConEmu->LogString(lsMsg);
	}

	// This implies Inet.Deinit(false) too
	if (!Inet.Init(this))
	{
		// Ошибка уже в стеке
		goto wrap;
	}

	mb_InetMode = !IsLocalFile(asSource);
	if (mb_InetMode)
	{
		mb_DroppedMode = false;

		if (mp_Set->isUpdateInetTool && mp_Set->szUpdateInetTool && *mp_Set->szUpdateInetTool)
		{
			args[0].argType = at_Str;args[0].strArg = mp_Set->szUpdateInetTool;
			Inet.DownloadCommand(dc_SetCmdString, 1, args);
		}
		else if (mp_Set->isUpdateUseProxy)
		{
			args[0].argType = at_Str;args[0].strArg = mp_Set->szUpdateProxy;
			args[1].argType = at_Str;args[1].strArg = mp_Set->szUpdateProxyUser;
			args[2].argType = at_Str;args[2].strArg = mp_Set->szUpdateProxyPassword;
			Inet.DownloadCommand(dc_SetProxy, 3, args);
		}
	}

	Inet.SetCallback(dc_ProgressCallback, ProgressCallback, reinterpret_cast<LPARAM>(this));

	args[0].argType = at_Str;  args[0].strArg = asSource;
	args[1].argType = at_Str;  args[1].strArg = asTarget;
	args[2].argType = at_Uint; args[2].uintArg = (m_ManualCallMode != UpdateCallMode::Automatic || abPackage);

	MCHKHEAP;

	lbRc = Inet.DownloadCommand(dc_DownloadFile, 3, args);
	if (lbRc)
	{
		if (rpSize)
		{
			// Files sizes larger than 4GB are not supported (doesn't matter for our update package)
			rpSize->LowPart = static_cast<DWORD>(args[0].uintArg);
			rpSize->HighPart = 0;
		}
		crc = static_cast<DWORD>(args[1].uintArg);
	}
	else
	{
		ReportError(L"File download was failed\n%s\nErrCode=0x%08X", asSource, static_cast<DWORD>(args[0].uintArg));
	}
wrap:
	Inet.SetCallback(dc_ProgressCallback, nullptr, 0);
	MCHKHEAP;
	return lbRc;
}

void CConEmuUpdate::ReportErrorInt(const ErrorInfo& error)
{
	MCHKHEAP;

	if (gpConEmu)
	{
		const CEStr lsLog(L"Update: ", error.What());
		wchar_t* ptr = lsLog.ms_Val;
		while ((ptr = wcspbrk(ptr, L"\r\n")) != nullptr) *ptr = L' ';
		gpConEmu->LogString(lsLog);
	}

	// Append error
	{
		wchar_t szTime[40] = L""; SYSTEMTIME st = {}; GetLocalTime(&st);
		swprintf_c(szTime, L"\r\n%u-%02u-%02u %u:%02u:%02u.%03u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		MSectionLockSimple CS;
		if (CS.Lock(&ms_ErrorLock, 5000))
		{
			const CEStr pszNew(error.What(), szTime, ms_LastErrorInfo ? L"\r\n\r\n" : nullptr, ms_LastErrorInfo);
			if (pszNew)
			{
				ms_LastErrorInfo = std::move(pszNew);
			}
			else
			{
				ms_LastErrorInfo.Set(error.What());
			}
		}
		else
		{
			_ASSERTE(FALSE && "Can't lock ms_LastErrorInfo");
			ms_LastErrorInfo.Set(error.What());
		}
	}

	// Show error message boxes in appropriate places only
	return;
#if 0
	MCHKHEAP;

	if (mn_InShowMsgBox > 0)
	{
		InterlockedIncrement(&mn_ErrorInfoSkipCount);
		return; // to avoid infinite recursion
	}

	InterlockedIncrement(&mn_ErrorInfoCount);

	// This will call back ShowLastError
	if (gpConEmu)
		gpConEmu->CallMainThread(false, ShowLastError, reinterpret_cast<LPARAM>(this));
	else
		SafeFree(asErrorInfo);

	MCHKHEAP;
#endif
}

void CConEmuUpdate::ReportError(LPCWSTR asFormat, const DWORD nErrCode)
{
	if (!asFormat)
		return;

	
	const ErrorInfo error(asFormat, nErrCode);
	if (error.HasError())
	{
		ReportErrorInt(error);
	}
}

void CConEmuUpdate::ReportError(LPCWSTR asFormat, LPCWSTR asArg, const DWORD nErrCode)
{
	if (!asFormat || !asArg)
		return;

	const ErrorInfo error(asFormat, asArg, nErrCode);
	if (error.HasError())
	{
		ReportErrorInt(error);
	}
}

void CConEmuUpdate::ReportError(LPCWSTR asFormat, LPCWSTR asArg1, LPCWSTR asArg2, const DWORD nErrCode)
{
	if (!asFormat || !asArg1 || !asArg2)
		return;

	const ErrorInfo error(asFormat, asArg1, asArg2, nErrCode);
	if (error.HasError())
	{
		ReportErrorInt(error);
	}
}

LRESULT CConEmuUpdate::RequestExitUpdate(LPARAM)
{
	if (!gpUpd)
		return 0;

	CConEmuUpdate::UpdateStep step = gpUpd->InUpdate();

	// Awaiting for exit?
	if ((step != UpdateStep::ExitAndUpdate) && (step != UpdateStep::PostponeUpdate))
	{
		_ASSERTE(step == UpdateStep::ExitAndUpdate);
	}
	// May be null, if update package was dropped on ConEmu icon
	else if (ghWnd)
	{
		gpConEmu->UpdateProgress();
		gpConEmu->PostScClose();
	}

	return 0;
}

void CConEmuUpdate::ReportBrokenIni(LPCWSTR asSection, LPCWSTR asName, LPCWSTR asIniUrl, LPCWSTR asIniLocal)
{
	const DWORD nErr = GetLastError();

	const CEStr lsInfo(
		L"[", asSection, L"] \"", asName, L"\""
		);

	const CEStr lsIni(
		L"URL: ", asIniUrl, L"\n"
		L"File: ", asIniLocal
		);

	ReportError(
		L"Version update information is broken (not found)\n"
		L"%s\n\n"
		L"%s\n\n"
		L"Error code=%u",
		lsInfo.c_str(), lsIni.c_str(), nErr);
}

// If we are installed in "C:\Program Files\..." or any other restricted location.
bool CConEmuUpdate::NeedRunElevation()
{
	if (!gpConEmu)
		return false;

	//TODO: В каких случаях нужен "runas"
	//TODO: Vista+: (если сейчас НЕ "Admin") && (установка в %ProgramFiles%)
	//TODO: WinXP-: (установка в %ProgramFiles%) && (нет доступа в %ProgramFiles%)

	DWORD dwErr = 0;
	wchar_t szTestFile[MAX_PATH+20];
	wcscpy_c(szTestFile, gpConEmu->ms_ConEmuExeDir);
	wcscat_c(szTestFile, L"\\");

	if (gOSVer.dwMajorVersion >= 6)
	{
		if (IsUserAdmin())
			return false; // Уже под админом (Vista+)
		// куда мы установлены? Если НЕ в %ProgramFiles%, то для распаковки совсем не нужно Elevation требовать
		int nFolderIdl[] = {
			CSIDL_PROGRAM_FILES,
			CSIDL_PROGRAM_FILES_COMMON,
			#ifdef _WIN64
			CSIDL_PROGRAM_FILESX86,
			CSIDL_PROGRAM_FILES_COMMONX86,
			#endif
		};
		size_t nLen;
		wchar_t szSystem[MAX_PATH+2];
		for (size_t i = 0; i < countof(nFolderIdl); i++)
		{
			for (size_t j = 0; j <= 1; j++)
			{
				if ((S_OK == SHGetFolderPath(nullptr, nFolderIdl[i], nullptr, j ? SHGFP_TYPE_DEFAULT : SHGFP_TYPE_CURRENT, szSystem))
					&& (nLen = _tcslen(szSystem)))
				{
					if (szSystem[nLen-1] != L'\\')
					{
						szSystem[nLen++] = L'\\'; szSystem[nLen] = 0;
					}

					if (lstrcmpni(szTestFile, szSystem, nLen) == 0)
					{
						return true; // installed in the ProgramFiles
					}
				}
			}
		}
		// Issue 651: Проверим возможность создания/изменения файлов в любом случае
		//// Скорее всего не надо
		//return false;
	}

	// XP и ниже
	wcscat_c(szTestFile, L"ConEmuUpdate.flag");
	HANDLE hFile = CreateFile(szTestFile, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		dwErr = GetLastError();
		return true;
	}
	CloseHandle(hFile);
	DeleteFile(szTestFile);

	// RunAs не нужен
	return false;
}

void CConEmuUpdate::DeleteBadTempFiles()
{
	ms_TempUpdateVerLocation.Release(true);
	msz_DeletePackageFile.Release(true);
	msz_DeleteBatchFile.Release(true);
}

bool CConEmuUpdate::Check7zipInstalled()
{
	if (mp_Set->UpdateDownloadSetup() == 1)
		return true; // Инсталлер, архиватор не требуется!

	const auto* pszCmd = mp_Set->UpdateArcCmdLine();
	CmdArg sz7zip;
	if (!NextArg(pszCmd, sz7zip))
	{
		ReportError(L"Invalid update command\nGoto 'Update' page and check 7-zip command", 0);
		return false;
	}

	if (FileExistsSearch(sz7zip.c_str(), sz7zip, false))
		return true;

	WARNING("TODO: Suggest to download 7zip");

	ReportError(L"7zip or WinRar not found! Not installed?\n%s\nGoto 'Update' page and check 7-zip command", sz7zip, 0);
	return false;
}

int CConEmuUpdate::QueryConfirmation(CConEmuUpdate::UpdateStep step)
{
	if (mb_RequestTerminate)
	{
		return 0;
	}

	int lRc = 0;
	CEStr pszMsg;

	_ASSERTE(isMainThread());

	switch (step)
	{
	case UpdateStep::ConfirmDownload:
		{
			mb_NewVersionAvailable = true;
			m_UpdateStep = step;

			if (m_ManualCallMode == UpdateCallMode::TSA)
			{
				lRc = TRUE;
			}
			else if (mp_Set->isUpdateConfirmDownload || m_ManualCallMode != UpdateCallMode::Automatic)
			{
				lRc = QueryConfirmationDownload();
			}
			else
			{
				const size_t cchMax = 200;
				if (pszMsg.GetBuffer(cchMax))
				{
					swprintf_c(pszMsg.data(), pszMsg.GetMaxCount(),
						L"New %s version available: %s\nClick here to download",
						(mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? CV_STABLE
						: (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? CV_PREVIEW
						: CV_DEVEL,
						ms_NewVersion);
					Icon.ShowTrayIcon(pszMsg, tsa_Source_Updater);
				}
				lRc = 0;
			}
		}
		break;

	case UpdateStep::ConfirmUpdate:
		m_UpdateStep = step;
		lRc = QueryConfirmationUpdate();
		break;

	case UpdateStep::NotStarted:
	case UpdateStep::Check:
	case UpdateStep::Downloading:
	case UpdateStep::PostponeUpdate:
	case UpdateStep::ExitAndUpdate:
	default:
		lRc = false;
		_ASSERTE(mb_NewVersionAvailable == false);
		if (step > UpdateStep::Check)
		{
			_ASSERTE(step<=UpdateStep::Check);
			break;
		}

		QueryConfirmationNoNewVer();
	}

	return lRc;
}

int CConEmuUpdate::QueryConfirmationDownload()
{
	if ((mn_InShowMsgBox > 0) || !gpConEmu)
		return 0; // Если отображается ошибка - не звать

	DontEnable de;
	InterlockedIncrement(&mn_InShowMsgBox);

	int iBtn = -1;
	HRESULT hr = S_FALSE;
	wchar_t szNewVer[120] = L"";
	wchar_t szWWW[MAX_PATH] = L"";
	TASKDIALOGCONFIG tsk{}; tsk.cbSize = sizeof(tsk);
	TASKDIALOG_BUTTON btns[3] = {};
	LPCWSTR pszServerPath = L"";
	size_t cchMax = 0;
	CEStr pszMsg;
	CEStr pszDup;
	CEStr pszBtn1;
	CEStr pszBtn2;

	const wchar_t* pszFile = nullptr;
	if (msz_ConfirmSource)
	{
		pszDup.Set(msz_ConfirmSource);
		if (pszDup)
		{
			wchar_t* lastSlash = wcsrchr(pszDup.data(), L'/');
			if (!lastSlash)
				lastSlash = wcsrchr(pszDup.data(), L'\\'); // Local file?
			if (lastSlash)
			{
				lastSlash[1] = 0;
				pszFile = (msz_ConfirmSource.c_str() + (lastSlash - pszDup.data() + 1));
				pszServerPath = pszDup;
			}
			else
			{
				pszServerPath = msz_ConfirmSource.c_str();
			}
		}
	}

	if (gOSVer.dwMajorVersion < 6)
		goto MsgOnly;

	tsk.hInstance = g_hInstance;
	if (hClassIcon)
		tsk.hMainIcon = hClassIcon;
	else
		tsk.pszMainIcon = MAKEINTRESOURCE(IDI_ICON1);

	swprintf_c(szNewVer, L"New %s version available: %s",
		(mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? CV_STABLE
		: (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? CV_PREVIEW
		: CV_DEVEL, ms_NewVersion);
	tsk.pszMainInstruction = szNewVer;

	swprintf_c(szWWW, L"<a href=\"%s\">%s</a>", gsWhatsNew, szWhatsNewLabel);
	tsk.pszFooter = szWWW;

	pszMsg = CreateVersionOnServerInfo(true);
	tsk.pszContent = pszMsg;

	pszBtn1 = CEStr(L"Download\n", pszServerPath, pszFile ? L"\n" : nullptr, pszFile);
	btns[0].nButtonID  = IDYES;    btns[0].pszButtonText = pszBtn1;
	pszBtn2 = CEStr(L"Visit download page\n", gsDownlPage);
	btns[1].nButtonID  = IDNO;     btns[1].pszButtonText = pszBtn2;
	btns[2].nButtonID  = IDCANCEL; btns[2].pszButtonText = L"Cancel\nSkip this version";

	tsk.dwFlags        = (hClassIcon?TDF_USE_HICON_MAIN:0)|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_ENABLE_HYPERLINKS;
	tsk.pszWindowTitle = ms_DefaultTitle;
	tsk.pButtons       = btns;
	tsk.cButtons       = countof(btns);

	tsk.hwndParent     = ghWnd;

	hr = TaskDialog(&tsk, &iBtn, nullptr, nullptr);
	if (hr == S_OK)
	{
		if (iBtn == IDNO)
			ConEmuAbout::OnInfo_DownloadPage();
		goto wrap;
	}

MsgOnly:

	cchMax = 300 + msz_ConfirmSource.GetLen();
	if (pszMsg.GetBuffer(cchMax))
	{
		swprintf_c(pszMsg.data(), pszMsg.GetMaxCount(), L"New %s version available: %s\n\nVersions on server\n%s\n\n%s\n%s\n\nDownload?",
			(mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? CV_STABLE
			: (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? CV_PREVIEW
			: CV_DEVEL,
			ms_NewVersion,
			ms_VerOnServer,
			pszServerPath,
			pszFile ? pszFile : L"");

		iBtn = MsgBox(pszMsg, MB_SETFOREGROUND | MB_SYSTEMMODAL | MB_ICONQUESTION | MB_YESNO, ms_DefaultTitle, nullptr, false);
	}
	else
	{
		iBtn = IDNO;
	}

wrap:
	InterlockedDecrement(&mn_InShowMsgBox);
	return (iBtn == IDYES);
}

int CConEmuUpdate::QueryConfirmationUpdate()
{
	if ((mn_InShowMsgBox > 0) || !gpConEmu)
		return 0; // Если отображается ошибка - не звать

	DontEnable de;
	InterlockedIncrement(&mn_InShowMsgBox);

	int iBtn = -1;
	HRESULT hr;
	wchar_t szWWW[MAX_PATH];
	TASKDIALOGCONFIG tsk = {sizeof(tsk)};
	TASKDIALOG_BUTTON btns[3] = {};
	wchar_t szMsg[200];


	if (gOSVer.dwMajorVersion < 6)
		goto MsgOnly;

	tsk.hInstance = g_hInstance;
	if (hClassIcon)
		tsk.hMainIcon = hClassIcon;
	else
		tsk.pszMainIcon = MAKEINTRESOURCE(IDI_ICON1);

	tsk.pszMainInstruction = L"Close ConEmu window and update?";

	swprintf_c(szWWW, L"<a href=\"%s\">%s</a>", gsWhatsNew, szWhatsNewLabel);
	tsk.pszFooter = szWWW;

	swprintf_c(szMsg, L"Close and update\nto %s version %s%s",
		mb_DroppedMode ? L"dropped" : (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? L"new " CV_STABLE
		: (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? L"new " CV_PREVIEW : L"new " CV_DEVEL, ms_NewVersion,
		(mp_Set->UpdateDownloadSetup() == 1) ? (mp_Set->isSetup64 ? L".x64" : L".x86") : L" (7-Zip)");
	btns[0].nButtonID  = IDYES;    btns[0].pszButtonText = szMsg;
	btns[1].nButtonID  = IDNO; btns[1].pszButtonText = L"Postpone\nupdate will be started when you close ConEmu window";
	btns[2].nButtonID  = IDCANCEL; btns[2].pszButtonText = L"Cancel";

	tsk.dwFlags        = (hClassIcon?TDF_USE_HICON_MAIN:0)|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_ENABLE_HYPERLINKS;
	tsk.pszWindowTitle = ms_DefaultTitle;
	tsk.pButtons       = btns;
	tsk.cButtons       = countof(btns);

	tsk.hwndParent     = ghWnd;

	hr = TaskDialog(&tsk, &iBtn, nullptr, nullptr);
	if (hr == S_OK)
		goto wrap;

MsgOnly:

	swprintf_c(szMsg,
		L"Do you want to close ConEmu and\n"
		L"update to %s version %s?\n\n"
		L"Press <No> to postpone.",
		mb_DroppedMode ? L"dropped" : (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? L"new " CV_STABLE
		: (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? L"new " CV_PREVIEW : L"new " CV_DEVEL, ms_NewVersion);

	iBtn = MsgBox(szMsg, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONQUESTION|MB_YESNOCANCEL, ms_DefaultTitle, nullptr, false);

wrap:
	InterlockedDecrement(&mn_InShowMsgBox);
	return iBtn;
}

int CConEmuUpdate::QueryConfirmationNoNewVer()
{
	if ((mn_InShowMsgBox > 0) || !gpConEmu)
		return 0; // Если отображается ошибка - не звать

	DontEnable de;
	InterlockedIncrement(&mn_InShowMsgBox);

	int iBtn = -1;
	HRESULT hr;
	CEStr szMsg, szInfo;
	wchar_t szCurVer[120];
	wchar_t szWWW[MAX_PATH];
	TASKDIALOGCONFIG tsk = {sizeof(tsk)};
	TASKDIALOG_BUTTON btns[2] = {};
	LPCWSTR pszServerPath = L"";
	CEStr pszBtn2;

	swprintf_c(szCurVer, L"No newer %s version is available",
		(mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? CV_STABLE
		: (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? CV_PREVIEW : CV_DEVEL);

	if (gOSVer.dwMajorVersion < 6)
		goto MsgOnly;

	tsk.hInstance = g_hInstance;
	if (hClassIcon)
		tsk.hMainIcon = hClassIcon;
	else
		tsk.pszMainIcon = MAKEINTRESOURCE(IDI_ICON1);

	tsk.pszMainInstruction = szCurVer;

	swprintf_c(szWWW, L"<a href=\"%s\">%s</a>", gsWhatsNew, szWhatsNewLabel);
	tsk.pszFooter = szWWW;

	szInfo = CreateVersionOnServerInfo(true);
	szMsg = CEStr(
		L"Your current ConEmu version is ", ms_CurVerInfo, L"\n\n",
		szInfo);
	tsk.pszContent = szMsg.ms_Val;

	btns[0].nButtonID  = IDOK;    btns[0].pszButtonText = L"OK";
	pszBtn2 = CEStr(L"Visit download page\n", gsDownlPage);
	btns[1].nButtonID  = IDNO;    btns[1].pszButtonText = pszBtn2;

	tsk.dwFlags        = (hClassIcon?TDF_USE_HICON_MAIN:0)|TDF_USE_COMMAND_LINKS|TDF_ALLOW_DIALOG_CANCELLATION|TDF_ENABLE_HYPERLINKS;
	tsk.pszWindowTitle = ms_DefaultTitle;
	tsk.pButtons       = btns;
	tsk.cButtons       = countof(btns);

	tsk.hwndParent     = ghWnd;

	hr = TaskDialog(&tsk, &iBtn, nullptr, nullptr);
	if (hr == S_OK)
	{
		if (iBtn == IDNO)
			ConEmuAbout::OnInfo_DownloadPage();
		goto wrap;
	}

MsgOnly:
	szInfo = CreateVersionOnServerInfo(false, L"\n\n");
	szMsg = CEStr(
		L"Your current ConEmu version is ", ms_CurVerInfo, L"\n\n",
		szInfo, szCurVer);

	iBtn = MsgBox(szMsg, MB_SETFOREGROUND|MB_SYSTEMMODAL|MB_ICONINFORMATION|MB_OK, ms_DefaultTitle, nullptr, false);

wrap:
	InterlockedDecrement(&mn_InShowMsgBox);
	return 0;
}

short CConEmuUpdate::GetUpdateProgress()
{
	if (mb_RequestTerminate)
		return -1;

	switch (InUpdate())
	{
	case UpdateStep::NotStarted:
	case UpdateStep::PostponeUpdate:
		return -1;
	case UpdateStep::Check:
		return m_ManualCallMode != UpdateCallMode::Automatic ? 1 : -1;
	case UpdateStep::ConfirmDownload:
		return UPD_PROGRESS_CONFIRM_DOWNLOAD;
	case UpdateStep::Downloading:
		if (mn_PackageSize > 0)
		{
			int nValue = (mn_InternetContentReady * 88 / mn_PackageSize);
			if (nValue < 0)
				nValue = 0;
			else if (nValue > 88)
				nValue = 88;
			return nValue+UPD_PROGRESS_DOWNLOAD_START;
		}
		return UPD_PROGRESS_DOWNLOAD_START;
	case UpdateStep::ConfirmUpdate:
		return UPD_PROGRESS_CONFIRM_UPDATE;
	case UpdateStep::ExitAndUpdate:
		return UPD_PROGRESS_EXIT_AND_UPDATE;
	}

	return -1;
}

CEStr CConEmuUpdate::GetCurVerInfo()
{
	CEStr pszVerInfo;

	if (lstrcmpi(ms_NewVersion, ms_OurVersion) > 0)
	{
		pszVerInfo = CEStr(L"Obsolete, recommended update to ", ms_NewVersion,
			(mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Stable) ? L" " CV_STABLE
			: (mp_Set->isUpdateUseBuilds == ConEmuUpdateSettings::Builds::Preview) ? L" " CV_PREVIEW : L" " CV_DEVEL);
	}
	else if (ms_CurVerInfo[0])
		pszVerInfo.Set(ms_CurVerInfo);
	else
		pszVerInfo.Set(ms_OurVersion);

	return pszVerInfo;
}

void CConEmuUpdate::WaitAllInstances()
{
	while (true)
	{
		bool bStillExists = false;
		CEStr szMessage(CLngRc::getRsrc(lng_UpdateCloseMsg1/*"Please, close all ConEmu instances before continue"*/));

		HWND hFind = FindWindowEx(nullptr, nullptr, VirtualConsoleClassMain, nullptr);
		if (hFind)
		{
			bStillExists = true;
			DWORD nPID; GetWindowThreadProcessId(hFind, &nPID);
			wchar_t szPID[16]; ultow_s(nPID, szPID, 10);
			szMessage.Append(L"\n",
				CLngRc::getRsrc(lng_UpdateCloseMsg2/*"ConEmu still running:"*/), L" ",
				L"PID=", szPID);
		}

		if (!bStillExists)
		{
			TODO("Можно бы проехаться по всем модулям запущенных процессов на предмет блокировки файлов в папках ConEmu");
		}

		if (!bStillExists)
			return;

		const int nBtn = MsgBox(szMessage, MB_ICONEXCLAMATION|MB_OKCANCEL, ms_DefaultTitle, nullptr, false);
		if (nBtn == IDCANCEL)
			return; // "Cancel" - means stop checking
	}
}

bool CConEmuUpdate::wininet::Init(CConEmuUpdate* apUpd)
{
	bool bRc = false;
	CEStr pszLib;
	HMODULE lhDll = nullptr;
	DownloadCommand_t lDownloadCommand = nullptr;

	//  Already initialized?
	if (hDll)
	{
		_ASSERTE(DownloadCommand!=nullptr);
		// But check if local objects are created...
		if (!DownloadCommand(dc_Init, 0, nullptr))
		{
			const DWORD nErr = GetLastError();
			pUpd->ReportError(L"Failed to re-initialize gpInet, code=%u", nErr);
			return false;
		}
		bRc = true;
		goto wrap;
	}

	pUpd = apUpd;

	pszLib = CEStr(gpConEmu->ms_ConEmuBaseDir, L"\\", ConEmuCD_DLL_3264);
	lhDll = pszLib ? LoadLibrary(pszLib) : nullptr;
	if (!lhDll)
	{
		_ASSERTE(lhDll!=nullptr);
		lhDll = LoadLibrary(ConEmuCD_DLL_3264);
	}
	if (!lhDll)
	{
		DWORD nErr = GetLastError();
		pUpd->ReportError(L"Required library '%s' was not loaded, code=%u", ConEmuCD_DLL_3264, nErr);
		goto wrap;
	}

	lDownloadCommand = (DownloadCommand_t)GetProcAddress(lhDll, "DownloadCommand");
	if (!lDownloadCommand)
	{
		pUpd->ReportError(L"Exported function DownloadCommand in '%s' not found! Check your installation!",
			ConEmuCD_DLL_3264, GetLastError());
		goto wrap;
	}

	if (!lDownloadCommand(dc_Init, 0, nullptr))
	{
		DWORD nErr = GetLastError(); // No use?
		pUpd->ReportError(L"Failed to initialize gpInet, code=%u", nErr);
		goto wrap;
	}

	hDll = lhDll;
	DownloadCommand = lDownloadCommand;

	bRc = true;
wrap:
	if (bRc)
	{
		SetCallback(dc_ErrCallback, ErrorCallback, reinterpret_cast<LPARAM>(apUpd));
		SetCallback(dc_LogCallback, LogCallback, reinterpret_cast<LPARAM>(apUpd));
	}
	if (lhDll && !bRc)
	{
		FreeLibrary(lhDll);
	}
	return bRc;
}
bool CConEmuUpdate::wininet::Deinit(bool bFull)
{
	// if bFull - release allocated object too
	if (!DownloadCommand)
		return false;
	if (!DownloadCommand(bFull ? dc_Deinit : dc_Reset, 0, nullptr))
		return false;
	return true;
}

void CConEmuUpdate::wininet::SetCallback(CEDownloadCommand cbk, FDownloadCallback pfnCallback, LPARAM lParam)
{
	CEDownloadErrorArg args[2];
	args[0].argType = at_Uint;  args[0].uintArg = (DWORD_PTR)pfnCallback;
	args[1].argType = at_Uint;  args[1].uintArg = lParam;
	_ASSERTE(dc_ErrCallback<=cbk && cbk<=dc_LogCallback);
	DownloadCommand(cbk, 2, args);
}

void CConEmuUpdate::ProgressCallback(const CEDownloadInfo* pError)
{
	_ASSERTE(pError && pError->lParam == reinterpret_cast<LPARAM>(gpUpd));
	if (!pError || pError->argCount < 1 || pError->Args[0].argType != at_Uint)
	{
		_ASSERTE(pError && (pError->argCount >= 1) && (pError->Args[0].argType == at_Uint));
		return;
	}
	CConEmuUpdate* pUpd = reinterpret_cast<CConEmuUpdate*>(pError->lParam);
	if (!pUpd)
		return;
	pUpd->mn_InternetContentReady = static_cast<DWORD>(pError->Args[0].uintArg); // How many bytes were read
	if (gpConEmu && ghWnd)
	{
		gpConEmu->UpdateProgress();
	}
}

void CConEmuUpdate::ErrorCallback(const CEDownloadInfo* pError)
{
	_ASSERTE(pError && pError->lParam == reinterpret_cast<LPARAM>(gpUpd));
	if (!pError)
		return;
	CConEmuUpdate* pUpd = reinterpret_cast<CConEmuUpdate*>(pError->lParam);
	if (!pUpd)
		return;
	const CEStr pszText = pError->GetFormatted(false);
	pUpd->ReportErrorInt(ErrorInfo(pszText.c_str()));
}

void CConEmuUpdate::LogCallback(const CEDownloadInfo* pError)
{
	_ASSERTE(pError && pError->lParam == reinterpret_cast<LPARAM>(gpUpd));
	if (!pError)
		return;
	const CEStr pszText = pError->GetFormatted(false);
	if (pszText)
	{
		const CEStr lsLog(L"Update: ", pszText.c_str());
		gpConEmu->LogString(lsLog);
	}
}
