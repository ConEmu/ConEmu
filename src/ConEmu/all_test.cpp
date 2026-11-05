
/*
Copyright (c) 2014-present Maximus5
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

#include "Header.h"
#include <commctrl.h>
#include "../UnitTests/gtest.h"

#pragma warning(disable: 4091)
#include <ShlObj.h>

#include "../common/EnvVar.h"
#include "../common/MJsonReader.h"
#include "../common/ProcessData.h"
#include "../common/ProcessSetEnv.h"
#include "../common/WFiles.h"
#include "../common/WModuleCheck.h"
#include "../common/WUser.h"
#include "ConEmu.h"
#include "SettingsStorage.h"
#include "IatHook.h"
#include "DarkMode.h"
#include "ToolImg.h"

namespace {
bool WINAPI TestAppsUseDarkMode() { return true; }
bool WINAPI TestAppsUseLightMode() { return false; }
LRESULT CALLBACK TestPalettePreview(HWND window, UINT message, WPARAM wp, LPARAM lp, UINT_PTR, DWORD_PTR)
{
	if (message == WM_PRINTCLIENT)
	{
		RECT rc; GetClientRect(window, &rc);
		const HDC dc = reinterpret_cast<HDC>(wp);
		FillRect(dc, &rc, DarkMode::Brush(dc, RGB(255,100,0)));
		return 0;
	}
	return DefSubclassProc(window, message, wp, lp);
}

struct RestoreDarkModeState
{
	fnShouldAppsUseDarkMode shouldAppsUseDarkMode = _ShouldAppsUseDarkMode;
	fnGetIsImmersiveColorUsingHighContrast highContrast = _GetIsImmersiveColorUsingHighContrast;
	bool supported = global::g_darkModeSupported;
	bool enabled = global::g_darkModeEnabled;
	~RestoreDarkModeState()
	{
		_ShouldAppsUseDarkMode = shouldAppsUseDarkMode;
		_GetIsImmersiveColorUsingHighContrast = highContrast;
		global::g_darkModeSupported = supported;
		global::g_darkModeEnabled = enabled;
	}
};
}

TEST(DarkTitleBar, UnsupportedSystemAndOptionalHighContrastApi)
{
	RestoreDarkModeState restore;
	global::g_darkModeSupported = false;
	_ShouldAppsUseDarkMode = nullptr;
	_GetIsImmersiveColorUsingHighContrast = nullptr;
	gpConEmu->RefreshTitleBarThemeColor(nullptr);
	EXPECT_FALSE(IsColorSchemeChangeMessage(reinterpret_cast<LPARAM>(L"ImmersiveColorSet")));
	// An absent optional ordinal must not crash a supported system either.
	global::g_darkModeSupported = true;
	EXPECT_FALSE(IsColorSchemeChangeMessage(0));
}

TEST(DarkTitleBar, NativeWindowTracksAppTheme)
{
	if (!global::g_darkModeSupported || global::g_buildNumber < 22000)
		GTEST_SKIP() << "DWM dark-mode readback requires Windows 11";
	RestoreDarkModeState restore;
	const HWND window = CreateWindowExW(0, L"STATIC", L"Dark title bar regression",
		WS_OVERLAPPEDWINDOW, 0, 0, 320, 200, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	ASSERT_NE(nullptr, window);
	_ShouldAppsUseDarkMode = TestAppsUseDarkMode;
	gpConEmu->RefreshTitleBarThemeColor(window);
	BOOL dark = FALSE;
	EXPECT_EQ(S_OK, gpConEmu->DwmGetWindowAttribute(window, 20, &dark, sizeof(dark)));
	EXPECT_EQ(IsHighContrast() ? FALSE : TRUE, dark);
	_ShouldAppsUseDarkMode = TestAppsUseLightMode;
	gpConEmu->RefreshTitleBarThemeColor(window);
	EXPECT_EQ(S_OK, gpConEmu->DwmGetWindowAttribute(window, 20, &dark, sizeof(dark)));
	EXPECT_EQ(FALSE, dark);
	DestroyWindow(window);
}

TEST(DarkScrollBar, MissingImportDirectoryIsSafe)
{
	alignas(IMAGE_NT_HEADERS) BYTE module[1024] = {};
	auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(module);
	dos->e_magic = IMAGE_DOS_SIGNATURE;
	dos->e_lfanew = sizeof(IMAGE_DOS_HEADER);
	auto nt = reinterpret_cast<PIMAGE_NT_HEADERS>(module + dos->e_lfanew);
	nt->Signature = IMAGE_NT_SIGNATURE;
	nt->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
	EXPECT_EQ(nullptr, FindDelayLoadThunkInModule(module, "uxtheme.dll", uint16_t(49)));
	EXPECT_EQ(nullptr, FindIatThunkInModule(module, "uxtheme.dll", "OpenThemeData"));
	EXPECT_EQ(nullptr, FindDelayLoadThunkInModule(nullptr, "uxtheme.dll", uint16_t(49)));
}

namespace {
HWND testThemeWindow = nullptr;
LPCWSTR testThemeClass = nullptr;
HANDLE WINAPI RecordOpenTheme(HWND window, LPCWSTR name)
{
	testThemeWindow = window;
	testThemeClass = name;
	return nullptr;
}
}

TEST(DarkScrollBar, HookTracksThemeAndPreservesOtherClasses)
{
	if (!global::g_darkModeSupported || !_OpenNcThemeData)
		GTEST_SKIP() << "Dark scrollbar APIs unavailable";
	HMODULE controls = GetModuleHandleW(L"comctl32.dll");
	auto thunk = FindDelayLoadThunkInModule(controls, "uxtheme.dll", uint16_t(49));
	if (!thunk) GTEST_SKIP() << "Common controls have no ordinal-49 delay import";
	RestoreDarkModeState restore;
	const auto original = _OpenNcThemeData;
	_OpenNcThemeData = RecordOpenTheme;
	const auto hook = reinterpret_cast<fnOpenNcThemeData>(thunk->u1.Function);
	const HWND window = reinterpret_cast<HWND>(1);
	_ShouldAppsUseDarkMode = TestAppsUseDarkMode;
	hook(window, L"ScrollBar");
	EXPECT_STREQ(IsHighContrast() ? L"ScrollBar" : L"Explorer::ScrollBar", testThemeClass);
	EXPECT_EQ(IsHighContrast() ? window : nullptr, testThemeWindow);
	_ShouldAppsUseDarkMode = TestAppsUseLightMode;
	hook(window, L"ScrollBar");
	EXPECT_STREQ(L"ScrollBar", testThemeClass);
	EXPECT_EQ(window, testThemeWindow);
	hook(window, L"Button");
	EXPECT_STREQ(L"Button", testThemeClass);
	EXPECT_EQ(window, testThemeWindow);
	hook(window, nullptr);
	EXPECT_EQ(nullptr, testThemeClass);
	_OpenNcThemeData = original;
}

TEST(DarkControls, ToolbarBitmapSurvivesNativeToolbarRegistration)
{
	class TestImage : public CToolImg { public: using CToolImg::AddBitmap; } image;
	const HDC screen = GetDC(nullptr);
	const HDC dc = CreateCompatibleDC(screen);
	const HBITMAP bitmap = CreateCompatibleBitmap(screen, 16, 16);
	const HGDIOBJ old = SelectObject(dc, bitmap);
	RECT rect = {0, 0, 16, 16};
	FillRect(dc, &rect, DarkMode::Brush(dc, RGB(32,32,32)));
	SetPixel(dc, 8, 8, RGB(0,255,0));
	SelectObject(dc, old);
	ASSERT_TRUE(image.Create(16, 16, 1, RGB(32,32,32)));
	ASSERT_EQ(1, image.AddBitmap(bitmap, 1));
	ASSERT_NE(nullptr, image.GetBitmap()); // This releases the construction DC.
	SelectObject(dc, bitmap);
	FillRect(dc, &rect, DarkMode::Brush(dc, RGB(80,80,80)));
	EXPECT_TRUE(image.PaintButtonTransparent(0, dc, 0, 0));
	EXPECT_EQ(RGB(0,255,0), GetPixel(dc, 8, 8));
	EXPECT_EQ(RGB(80,80,80), GetPixel(dc, 0, 0));
	SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc); ReleaseDC(nullptr, screen);
}

TEST(DarkControls, NativeControlsRenderDarkAndReturnToLight)
{
	if (!global::g_darkModeSupported || IsHighContrast()) GTEST_SKIP();
	RestoreDarkModeState restore;
	_ShouldAppsUseDarkMode = TestAppsUseDarkMode;
	const HWND parent = CreateWindowEx(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW,
		0, 0, 300, 200, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
	ASSERT_NE(nullptr, parent);
	const HWND button = CreateWindowEx(0, L"BUTTON", L"Apply", WS_CHILD | BS_PUSHBUTTON,
		0, 0, 100, 30, parent, nullptr, GetModuleHandle(nullptr), nullptr);
	const HWND label = CreateWindowEx(0, L"STATIC", L"Disabled", WS_CHILD | WS_DISABLED,
		0, 40, 100, 30, parent, nullptr, GetModuleHandle(nullptr), nullptr);
	const HWND header = CreateWindowEx(0, WC_HEADER, L"", WS_CHILD,
		0, 80, 100, 30, parent, nullptr, GetModuleHandle(nullptr), nullptr);
	const HWND preview = CreateWindowEx(0, L"STATIC", L"Preview", WS_CHILD,
		0, 120, 100, 30, parent, nullptr, GetModuleHandle(nullptr), nullptr);
	SetWindowSubclass(preview, TestPalettePreview, 1, 0);
	DarkMode::ApplyDialog(parent);
	const HDC screen = GetDC(nullptr), dc = CreateCompatibleDC(screen);
	const HBITMAP bitmap = CreateCompatibleBitmap(screen, 100, 30);
	const HGDIOBJ old = SelectObject(dc, bitmap);
	SendMessage(button, BM_SETSTATE, TRUE, 0);
	SendMessage(button, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
	EXPECT_EQ(RGB(80,80,80), GetPixel(dc, 7, 7));
	SendMessage(label, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
	EXPECT_EQ(DarkMode::Background, GetPixel(dc, 99, 29));
	SendMessage(header, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
	EXPECT_EQ(DarkMode::Background, GetPixel(dc, 99, 29));
	SendMessage(preview, WM_PRINTCLIENT, reinterpret_cast<WPARAM>(dc), PRF_CLIENT);
	EXPECT_EQ(RGB(255,100,0), GetPixel(dc, 99, 29));
	_ShouldAppsUseDarkMode = TestAppsUseLightMode;
	DarkMode::ApplyDialog(parent);
	EXPECT_FALSE(global::g_darkModeEnabled);
	EXPECT_EQ(nullptr, DarkMode::ControlColor(label, dc, WM_CTLCOLORSTATIC));
	SelectObject(dc, old); DeleteObject(bitmap); DeleteDC(dc); ReleaseDC(nullptr, screen);
	DestroyWindow(parent);
}

#if CE_UNIT_TEST==1
#include "../common/MAssert.h"
extern bool gbVerifyVerbose;
#else
#error "CE_UNIT_TEST should be defined for unit tests"
#endif


// Hide from global namespace
namespace {

void UnitMaskTests()
{
	struct {
		LPCWSTR asFileName, asMask;
		bool Result;
	} Tests[] = {
		{L"FileName.txt", L"FileName.txt", true},
		{L"FileName.txt", L"FileName", false},
		{L"FileName.txt", L"FileName.doc", false},
		{L"FileName.txt", L"*", true},
		{L"FileName.txt", L"FileName*", true},
		{L"FileName.txt", L"FileName*txt", true},
		{L"FileName.txt", L"FileName*.txt", true},
		{L"FileName.txt", L"FileName*e.txt", false},
		{L"FileName.txt", L"File*qqq", false},
		{L"FileName.txt", L"File*txt", true},
		{L"FileName.txt", L"Name*txt", false},
		{nullptr}
	};
	bool bCheck;
	for (size_t i = 0; Tests[i].asFileName; i++)
	{
		bCheck = CompareFileMask(Tests[i].asFileName, Tests[i].asMask);
		_ASSERTE(bCheck == Tests[i].Result);
	}
	bCheck = true;
}

void UnitDriveTests()
{
	struct {
		LPCWSTR asPath, asResult;
	} Tests[] = {
		{L"C:", L"C:"},
		{L"C:\\Dir1\\Dir2\\File.txt", L"C:"},
		{L"\\\\Server\\Share", L"\\\\Server\\Share"},
		{L"\\\\Server\\Share\\Dir1\\Dir2\\File.txt", L"\\\\Server\\Share"},
		{L"\\\\?\\UNC\\Server\\Share", L"\\\\?\\UNC\\Server\\Share"},
		{L"\\\\?\\UNC\\Server\\Share\\Dir1\\Dir2\\File.txt", L"\\\\?\\UNC\\Server\\Share"},
		{L"\\\\?\\C:", L"\\\\?\\C:"},
		{L"\\\\?\\C:\\Dir1\\Dir2\\File.txt", L"\\\\?\\C:"},
		{nullptr}
	};
	bool bCheck;
	wchar_t szDrive[MAX_PATH];
	for (size_t i = 0; Tests[i].asPath; i++)
	{
		wmemset(szDrive, L'z', countof(szDrive));
		bCheck = (wcscmp(GetDrive(Tests[i].asPath, szDrive, countof(szDrive)), Tests[i].asResult) == 0);
		_ASSERTE(bCheck);
	}
	bCheck = true;
}

void UnitPathTests()
{
	struct {
		LPCWSTR asPath, asResult;
	} Tests[] = {
		{L"C:", nullptr},
		{L"C:\\Dir1\\", L"C:\\Dir1"},
		{L"C:\\Dir1\\File.txt", L"C:\\Dir1"},
		{L"C:/Dir1/", L"C:/Dir1"},
		{L"C:/Dir1/File.txt", L"C:/Dir1"},
		{nullptr}
	};
	bool bCheck;
	for (size_t i = 0; Tests[i].asPath; i++)
	{
		CEStr path(GetParentPath(Tests[i].asPath));
		bCheck = (!Tests[i].asResult && !path.ms_Val) || (wcscmp(Tests[i].asResult, path.c_str(L"")) == 0);
		_ASSERTE(bCheck);
	}

	struct {
		LPCWSTR asPath; bool reqFull, result;
	} Tests2[] = {
		{L"\"C:\\folder 1\\file\"", false, false},
		{L"C:\\folder \"1\\file", false, false},
		{L"C:\\folder 1>file", false, false},
		{L"C:\\folder 1<file", false, false},
		{L"C:\\folder 1|file", false, false},
		{L"C:\\folder 1\\file", true, true},
		{L"C:\\folder 1\\file", false, true},
		{L"C:\\folder 1:\\file", true, false},
		{L"C:\\folder 1:\\file", false, false},
		{L"C\\folder 1:\\file", true, false},
		{L"C\\folder 1:\\file", false, false},
		{L"folder 1\\file", true, false},
		{L"folder 1\\file", false, true},
		{L"folder 1/file", true, false},
		{L"folder 1/file", false, true},
		{L"\\\\server\\share", true, true},
		{L"\\\\server\\share", false, true},
		{nullptr}
	};
	for (size_t i = 0; Tests2[i].asPath; ++i)
	{
		bCheck = (IsFilePath(Tests2[i].asPath, Tests2[i].reqFull) == Tests2[i].result);
		_ASSERTE(bCheck);
	}

	struct {
		LPCWSTR asPath; bool autoQuote; LPCWSTR asResult;
	} Tests3[] = {
		{L"C:\\Dir1\\", true, L"/mnt/c/Dir1/"},
		{L"C:\\Dir1\\File.txt", true, L"/mnt/c/Dir1/File.txt"},
		{L"C:/Dir1/", true, L"/mnt/c/Dir1/"},
		// specials, quoted
		{L"C:/Dir Dir", true, L"'/mnt/c/Dir Dir'"},
		{L"C:/Dir$Dir", true, L"'/mnt/c/Dir$Dir'"},
		{L"C:/Dir(Dir)", true, L"'/mnt/c/Dir(Dir)'"},
		{L"C:/Dir'Dir", true, L"'/mnt/c/Dir'\\''Dir'"},
		// specials, escaped
		{L"C:/Dir Dir", false, L"/mnt/c/Dir\\ Dir"},
		{L"C:/Dir$Dir", false, L"/mnt/c/Dir\\$Dir"},
		{L"C:/Dir(Dir)", false, L"/mnt/c/Dir\\(Dir\\)"},
		{L"C:/Dir'Dir", false, L"/mnt/c/Dir\\'Dir"},
		// #DupCygwinPath Network shares tests
		{nullptr}
	};
	for (size_t i = 0; Tests3[i].asPath; i++)
	{
		CEStr path; DupCygwinPath(Tests3[i].asPath, Tests3[i].autoQuote, L"/mnt", path);
		bCheck = (path.Compare(Tests3[i].asResult) == 0);
		_ASSERTE(bCheck);
	}

	bCheck = true;
}

void UnitFileNamesTest()
{
	_ASSERTE(IsDotsName(L"."));
	_ASSERTE(IsDotsName(L".."));
	_ASSERTE(!IsDotsName(L"..."));
	_ASSERTE(!IsDotsName(L""));

	struct {
		LPCWSTR asPath, asPart1, asPart2, asResult;
	} Tests[] = {
		{L"C:", L"Dir", L"File.txt", L"C:\\Dir\\File.txt"},
		{L"C:\\", L"\\Dir\\", L"\\File.txt", L"C:\\Dir\\File.txt"},
		{L"C:\\", L"\\File.txt", nullptr, L"C:\\File.txt"},
		{L"C:", L"\\File.txt", nullptr, L"C:\\File.txt"},
		{L"C:\\", L"File.txt", nullptr, L"C:\\File.txt"},
		{nullptr}
	};
	bool bCheck;
	for (size_t i = 0; Tests[i].asPath; i++)
	{
		CEStr pszJoin = JoinPath(Tests[i].asPath, Tests[i].asPart1, Tests[i].asPart2);
		bCheck = (pszJoin && (lstrcmp(pszJoin, Tests[i].asResult) == 0));
		_ASSERTE(bCheck);
	}
	bCheck = true;
}

void UnitExpandTest()
{
	_ASSERTE(gpConEmu!=nullptr);
	CEStr szExe;
	wchar_t szChoc[MAX_PATH] = L"powershell -NoProfile -ExecutionPolicy unrestricted -Command \"iex ((new-object net.webclient).DownloadString('https://chocolatey.org/install.ps1'))\" && SET PATH=%PATH%;%systemdrive%\\chocolatey\\bin";
	const CEStr pszExpanded = ExpandEnvStr(szChoc);
	const auto nLen = pszExpanded.GetLen();
	BOOL bFound = FileExistsSearch(szChoc, szExe, false);
	wcscpy_c(szChoc, gpConEmu->ms_ConEmuExeDir);
	wcscat_c(szChoc, L"\\Tests\\Executables\\abcd");
	bFound = FileExistsSearch(szChoc, szExe, false);
	// TakeCommand
	ConEmuComspec tcc = {};
	tcc.csType = cst_AutoTccCmd;
	FindComspec(&tcc, false);
}

void UnitModuleTest()
{
	const CEStr pszConEmuCD(gpConEmu->ms_ConEmuBaseDir, L"\\", ConEmuCD_DLL_3264);
	HMODULE hMod, hGetMod;
	bool bTest;


	_ASSERTE(!IsModuleValid((HMODULE)nullptr));
	_ASSERTE(!IsModuleValid((HMODULE)INVALID_HANDLE_VALUE));

	hMod = GetModuleHandle(L"kernel32.dll");
	if (hMod)
	{
		bTest = IsModuleValid(hMod);
		_ASSERTE(bTest);
	}
	else
	{
		_ASSERTE(FALSE && "GetModuleHandle(kernel32) failed");
	}

	hMod = LoadLibrary(pszConEmuCD);
	if (hMod)
	{
		bTest = IsModuleValid(hMod);
		_ASSERTE(bTest);

		FreeLibrary(hMod);
		bTest = IsModuleValid(hMod);
		// Due to unknown reason (KIS?) FreeLibrary was not able to release hMod sometimes
		hGetMod = GetModuleHandle(pszConEmuCD);
		if (!hGetMod)
			bTest = IsModuleValid(hMod);
		_ASSERTE(!bTest || (hGetMod!=nullptr));
	}
	else
	{
		_ASSERTE(FALSE && "LoadLibrary(pszConEmuCD) failed");
	}
}

void DebugUnitMprintfTest()
{
	wchar_t szTest[80];
	msprintf(szTest, countof(szTest), L"%u,%02u,%02u,%02u,%02x,%08x", 12345, 1, 23, 4567, 0xFE, 0xABCD1234);
	int nDbg = lstrcmp(szTest, L"12345,01,23,4567,fe,abcd1234");
	_ASSERTE(nDbg==0);
}

void DebugVersionTest()
{
	DWORDLONG const dwlConditionMask = VerSetConditionMask(VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL), VER_MINORVERSION, VER_GREATER_EQUAL);

	_ASSERTE(_WIN32_WINNT_WIN7==0x601);
	OSVERSIONINFOEXW osvi7 = MakeOsVersionEx(HIBYTE(_WIN32_WINNT_WIN7), LOBYTE(_WIN32_WINNT_WIN7));
	const bool bWin7 = _VerifyVersionInfo(&osvi7, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) != 0;

	_ASSERTE(_WIN32_WINNT_VISTA==0x600);
	OSVERSIONINFOEXW osvi6 = MakeOsVersionEx(HIBYTE(_WIN32_WINNT_VISTA), LOBYTE(_WIN32_WINNT_VISTA));
	const bool bWin6 = _VerifyVersionInfo(&osvi6, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask) != 0;

	OSVERSIONINFOW osv = {};
	osv.dwOSVersionInfoSize = sizeof(osv);
	GetOsVersionInformational(&osv);
	const bool bVerWin7 = ((osv.dwMajorVersion > 6) || ((osv.dwMajorVersion == 6) && (osv.dwMinorVersion >= 1)));
	const bool bVerWin6 = (osv.dwMajorVersion >= 6);

	_ASSERTE(bWin7 == bVerWin7);
	_ASSERTE(bWin6 == bVerWin6);
}

void DebugFileExistTests()
{
	bool b;
	b = DirectoryExists(L"C:\\Documents and Settings\\Maks\\.ipython\\");
	b = DirectoryExists(L"C:\\Documents and Settings\\Maks\\.ipython\\.");
	b = DirectoryExists(L"C:\\Documents and Settings\\Maks\\.ipython");
	b = DirectoryExists(L"C:\\Documents and Settings\\Maks\\.ipython-qq");
	b = FileExists(L"C:\\Documents and Settings\\Maks\\.ipython");
	b = FileExists(L"C:\\Documents and Settings\\Maks\\.ipython\\README");
	b = FileExists(L"C:\\Documents and Settings\\Maks\\.ipython\\.");
}

void DebugStrUnitTest()
{
	struct strTests { wchar_t szTest[100], szCmp[100]; }
	Tests[] = {
		{L"Line1\n#Comment1\nLine2\r\n#Comment2\r\nEnd of file", L"Line1\nLine2\r\nEnd of file"},
		{L"Line1\n#Comment1\r\n", L"Line1\n"}
	};
	int iCmp;
	for (INT_PTR i = 0; i < countof(Tests); i++)
	{
		StripLines(Tests[i].szTest, L"#");
		iCmp = wcscmp(Tests[i].szTest, Tests[i].szCmp);
		_ASSERTE(iCmp == 0);
		StripLines(Tests[i].szTest, L"#");
		iCmp = wcscmp(Tests[i].szTest, Tests[i].szCmp);
		_ASSERTE(iCmp == 0);
	}

	// Some compilation and operators check

	CEStr str1;
	const wchar_t* pszTest = Tests[0].szTest;
	{
		str1 = lstrdup(pszTest);
	}
	iCmp = lstrcmp(str1, pszTest);
	_ASSERTE(iCmp == 0);

	{
		const CEStr str2(lstrdup(pszTest));
		iCmp = lstrcmp(str2, pszTest);
		_ASSERTE(iCmp == 0 && str2.ms_Val && str2.ms_Val != pszTest);
	}

	// The following block must to be able to compile
	#if 0
	{
		CEStr str3(pszDup);
		str3 = pszTest;
	}

	{
		CEStr str4;
		str4 = pszTest;
		iCmp = lstrcmp(str4, pszTest);
		_ASSERTE(iCmp == 0 && str4.ms_Val && str4.ms_Val != pszTest);
	}

	{
		CEStr str5;
		str5 = str1;
		iCmp = lstrcmp(str5, pszTest);
		_ASSERTE(iCmp == 0 && str5.ms_Val && str3.ms_Val != str1.ms_Val);
	}
	#endif

	{
		CEStr strSelf;
		const wchar_t szTest1[] = L"Just a test";
		const wchar_t szTest2[] = L"Just a";
		const wchar_t szTest3[] = L"a";
		strSelf.Set(szTest1, 64); // Try to assign larger length, must accept ASCIIZ string length
		iCmp = lstrcmp(strSelf, szTest1);
		_ASSERTE(iCmp == 0);
		strSelf.Set((LPCTSTR)strSelf, 6);
		iCmp = lstrcmp(strSelf, szTest2);
		_ASSERTE(iCmp == 0);
		strSelf.Set(((LPCTSTR)strSelf) + 5, 1);
		iCmp = lstrcmp(strSelf, szTest3);
		_ASSERTE(iCmp == 0);
	}

	iCmp = lstrcmp(str1, pszTest);
	_ASSERTE(iCmp == 0);

	{
		LPCWSTR pszTest;
		CEStr szStr1(L"Test");
		CEStr szStr2;
		//pszTest = szStr1 ? szStr1 : L"<nullptr>"; // -- expected to be cl error in VC14
		pszTest = szStr1 ? (LPCWSTR)szStr1 : L"<nullptr>";
		pszTest = szStr2 ? (LPCWSTR)szStr2 : L"<nullptr>";
		//msprintf(szStr2.GetBuffer(128), 128, L"Str1=`%s`", szStr1); //-- would be nice to forbid or assert this. strict `(LPCWSTR)szStr1` is required here
		UNREFERENCED_PARAMETER(pszTest);
	}
}

void DebugCpUnitTest()
{
	typedef struct {
		LPCWSTR sString;
		UINT nCP;
		wchar_t cEnd;
	} Test;
	Test Tests[] = {
		{L"Utf-8", CP_UTF8},
		{L"Utf-8;Acp", CP_UTF8, L';'},
		{L"ansi", CP_ACP},
		{L"ansicp;none", CP_ACP, L';'},
		{L"65001:1251", 65001, L':'},
		{nullptr},
	};

	LPCWSTR pszEnd;
	UINT nCP;

	_ASSERTE(GetCpFromString(L"866") == 866);

	for (INT_PTR i = 0; Tests[i].sString; i++)
	{
		const Test& p = Tests[i];
		nCP = GetCpFromString(p.sString, &pszEnd);
		_ASSERTE(nCP == p.nCP);
		_ASSERTE((pszEnd == nullptr && p.cEnd == 0) || (pszEnd && *pszEnd == p.cEnd));
	}
}

void DebugProcessNameTest()
{
	CProcessData processes;
	CEStr lsName(lstrdup(L"xxx.exe")), lsPath;
	DWORD nPID = GetCurrentProcessId();
	bool bRc = processes.GetProcessName(nPID, lsName.GetBuffer(MAX_PATH), MAX_PATH, lsPath.GetBuffer(MAX_PATH*2), MAX_PATH*2, nullptr);
	_ASSERTE(bRc);
}

void DebugTestSetParser()
{
	CProcessEnvCmd cmd;
	cmd.AddLines(L"set  V1=From Settings \r\nset V2=Value2 & Value2\r\n set \"V3=\"Value \"\" 3 \" \r\n", false);
	_ASSERTE(cmd.mn_SetCount==3
		&& !wcscmp(cmd.m_Commands[0]->pszName, L"V1") && !wcscmp(cmd.m_Commands[0]->pszValue, L"From Settings")
		&& !wcscmp(cmd.m_Commands[1]->pszName, L"V2") && !wcscmp(cmd.m_Commands[1]->pszValue, L"Value2 & Value2")
		&& !wcscmp(cmd.m_Commands[2]->pszName, L"V3") && !wcscmp(cmd.m_Commands[2]->pszValue, L"\"Value \"\" 3 ")
		);
	CEStr lsTemp = cmd.Allocate(nullptr);
}

void DebugMapsTests()
{
	MCircular<LONG,16> circ = {};
	circ.AddValue(1);
	circ.AddValue(2);
	_ASSERTE(circ.HasValue(1));
	_ASSERTE(circ.HasValue(2));
	_ASSERTE(!circ.HasValue(3));
	circ.DelValue(1);
	_ASSERTE(!circ.HasValue(1));
	_ASSERTE(circ.HasValue(2));
	for (int i = 100; i <= 116; i++)
		circ.AddValue(i);
	_ASSERTE(!circ.HasValue(2));
	_ASSERTE(!circ.HasValue(100));
	_ASSERTE(circ.HasValue(115));
	_ASSERTE(circ.HasValue(116));
}

namespace {
	struct MArrayDeleteTester
	{
		int value = 1;
		static int dtor_called;
		~MArrayDeleteTester() {
			++dtor_called;
		};
	};

	int MArrayDeleteTester::dtor_called = 0;
};

void DebugArrayTests()
{
	MArray<CEStr> sArr;
	sArr.reserve(1);
	_ASSERTE(sArr.size()==0 && sArr.capacity()==1);
	sArr.push_back(CEStr(L"sample string"));
	_ASSERTE(sArr.size()==1 && sArr.capacity()==1);
	const wchar_t* psz = sArr[0].c_str();
	_ASSERTE(psz && wcscmp(psz, L"sample string")==0);
	sArr.swap(MArray<CEStr>());
	_ASSERTE(sArr.size()==0 && sArr.capacity()==0);
	sArr.push_back(CEStr(L"1"));
	_ASSERTE(sArr.size()==1 && sArr.capacity()>=1 && sArr[0]==L"1");
	sArr.push_back(L"3");
	_ASSERTE(sArr.size()==2 && sArr.capacity()>=2 && sArr[0]==L"1" && sArr[1]==L"3");
	const CEStr s2(L"2");
	sArr.insert(1, s2);
	_ASSERTE(sArr.size()==3 && sArr[0]==L"1" && sArr[1]==L"2" && sArr[2]==L"3");
	sArr.erase(1);
	_ASSERTE(sArr.size()==2 && sArr[0]==L"1" && sArr[1]==L"3");
	sArr.push_back(L"4");
	_ASSERTE(sArr.size()==3 && sArr[0]==L"1" && sArr[1]==L"3" && sArr[2]==L"4");
	sArr.insert(1, CEStr(L"2"));
	_ASSERTE(sArr.size()==4 && sArr[0]==L"1" && sArr[1]==L"2" && sArr[2]==L"3" && sArr[3]==L"4");
	sArr.erase(1);
	_ASSERTE(sArr.size()==3 && sArr[0]==L"1" && sArr[1]==L"3" && sArr[2]==L"4");

	bool delete_called = false;
	MArrayDeleteTester::dtor_called = 0;
	MArray<MArrayDeleteTester> dArr;
	dArr.push_back(MArrayDeleteTester{2});
	_ASSERTE(MArrayDeleteTester::dtor_called==1 && dArr.size()==1 && dArr[0].value==2); // dtor of temp object
	MArrayDeleteTester::dtor_called = 0;
	dArr.clear();
	_ASSERTE(MArrayDeleteTester::dtor_called==1); // dtor call from MArray
	MArrayDeleteTester::dtor_called = 0;
	dArr.resize(2);
	_ASSERTE(MArrayDeleteTester::dtor_called==0 && dArr.size()==2 && dArr[0].value==1 && dArr[1].value==1);
	dArr.resize(1);
	_ASSERTE(MArrayDeleteTester::dtor_called==1 && dArr.size()==1 && dArr[0].value==1);

	MArray<int> arr;
	int i1 = 1;
	arr.push_back(i1);
	const int i2 = 2;
	arr.push_back(i2);
	arr.push_back(3);
	arr.push_back(4);
	arr.insert(2, 5);
	_ASSERTE(arr.size()==5 && arr[0]==1 && arr[1]==2 && arr[2]==5 && arr[3]==3 && arr[4]==4);
	arr.sort([](const int& i1, const int& i2){ return (i1 < i2); });
	_ASSERTE(arr.size()==5 && arr[0]==1 && arr[1]==2 && arr[2]==3 && arr[3]==4 && arr[4]==5);
	arr.sort([](const int& i1, const int& i2){ return (i1 > i2); });
	_ASSERTE(arr.size()==5 && arr[0]==5 && arr[1]==4 && arr[2]==3 && arr[3]==2 && arr[4]==1);

	_ASSERTE(&arr[1] == &(arr[1]));

	MArray<int> arr2;
	arr2.push_back(1);
	_ASSERTE(arr2.size() == 1 && arr2[0] == 1);
	arr2.resize(999);
	_ASSERTE(arr2.size() == 999 && arr2[0] == 1);
	for (ssize_t i = 1; i < 999; ++i)
		_ASSERTE(arr2[i] == 0);

	arr2.swap(arr);
	_ASSERTE(arr2.size()==5 && arr2[0]==5 && arr.size()==999 && arr[0]==1);

	#if 0
	// note: 'MArray<int> &MArray<int>::operator =(const MArray<int> &)': function was implicitly deleted because 'MArray<int>' has a user-defined move constructor
	arr2 = arr; // cl error is expected
	#endif

	MArray<int> arr3;
	for (int i = 1; i <= 5; ++i) arr3.push_back(i);
	for (ssize_t i = arr3.size() - 1; i >= 0; --i) arr3.erase(i);
	for (int i = 1; i <= 5; ++i) arr3.push_back(i);
	for (ssize_t i = 0; i < arr3.size(); ++i) arr3.erase(i);
}

void DebugJsonTest()
{
	MJsonValue value, v1, v2, v3, v4;
	bool b;

	b = value.ParseJson(L"{ \"languages\": [ {\"id\": \"en\", \"name\": \"English\" } , {\"id\": \"ru\", \"name\": \"Русский\" } ] }");
	_ASSERTE(b);

	b = value.ParseJson(L"{ \"hints\": { \"bSaveSettings\": {"
		L"\"en\": [\"Save settings to registry/xml\\r\\n\""
		L", \"Don't close dialog if Shift pressed\"],"
		L"\"id\" : 1610"
		L"} } }");
	_ASSERTE(b);
	b = value.getItem(L"hints", v1);
	b = v1.getItem(L"bSaveSettings", v2);
	b = v2.getItem(L"id", v3);
	b = v2.getItem(L"en", v4);
}

void XmlValueConvertTest()
{
	struct xml_test { const char* sType; const char* sData; DWORD expected; bool success; };
	xml_test tests[] = {
		{"ulong", "0", 0, true },
		{"ulong", "0123456789", 123456789, true },
		{"dword", "0", 0, true },
		{"dword", "12345678", 0x12345678, true },
		{"dword", "eA7bfE30", 0xeA7bfE30, true },
		{"long", "0", 0, true },
		{"long", "1234567891", 1234567891, true },
		{"long", "-1234567891", (DWORD)-1234567891, true },
		{"hex", "00", 0x00, true },
		{"hex", "AB", 0xAB, true },
		{"hex", "AB,CD", 0xCDAB, true },
		{"hex", "ABCD", 0xAB, true }, // data is partially incorrect (no commas), we shall take only "AB"
		{"hex", "A,B,C,D", 0x0D0C0B0A, true },
	};

	DWORD value;
	bool rc;
	for (const auto& test : tests)
	{
		value = 0x12345678;
		rc = SettingsXML::ConvertData(test.sType, test.sData, (LPBYTE)&value, sizeof(value));
		_ASSERTE(rc == test.success && (!test.success || (value == test.expected)));
	}
}

} // end of namespace

class General : public testing::Test
{
public:
	void SetUp() override
	{
		gbVerifyFailed = false;
		gbVerifyStepFailed = false;
		gbVerifyVerbose = false;
	}

	void TearDown() override
	{
	}
};


TEST_F(General, UnitMaskTests)
{
	UnitMaskTests();
}
TEST_F(General, UnitDriveTests)
{
	UnitDriveTests();
}
TEST_F(General, UnitPathTests)
{
	UnitPathTests();
}
TEST_F(General, UnitFileNamesTest)
{
	UnitFileNamesTest();
}
TEST_F(General, UnitExpandTest)
{
	UnitExpandTest();
}
TEST_F(General, UnitModuleTest)
{
	UnitModuleTest();
}
TEST_F(General, DebugUnitMprintfTest)
{
	DebugUnitMprintfTest();
}
TEST_F(General, DebugVersionTest)
{
	DebugVersionTest();
}
TEST_F(General, DebugFileExistTests)
{
	DebugFileExistTests();
}
TEST_F(General, DebugStrUnitTest)
{
	DebugStrUnitTest();
}
TEST_F(General, DebugCpUnitTest)
{
	DebugCpUnitTest();
}
TEST_F(General, DebugProcessNameTest)
{
	DebugProcessNameTest();
}
TEST_F(General, DebugTestSetParser)
{
	DebugTestSetParser();
}
TEST_F(General, DebugMapsTests)
{
	DebugMapsTests();
}
TEST_F(General, DebugArrayTests)
{
	DebugArrayTests();
}
TEST_F(General, DebugJsonTest)
{
	DebugJsonTest();
}
TEST_F(General, XmlValueConvertTest)
{
	XmlValueConvertTest();
}
