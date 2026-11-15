@echo off
rem Dev build with msbuild: ms.build.dev.cmd [Debug|Remote|Release] [min|full|far|tests]
rem   min   - ConEmu, ConEmuC, ConEmuCD, ConEmuHk for x64 (default)
rem   full  - min + ConEmuC, ConEmuCD, ConEmuHk for Win32
rem   far   - Far Manager plugins (x64 + Win32)
rem   tests - Tests project (x64)
setlocal
set "CFG=%~1"
if "%CFG%" == "" set CFG=Debug
set "SET=%~2"
if "%SET%" == "" set SET=min
set VSInstallDir=
for /f "usebackq tokens=1* delims=: " %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -prerelease -requires Microsoft.Component.MSBuild`) do (
  if /i "%%i"=="installationPath" set "VSInstallDir=%%j"
)
if not defined VSInstallDir (
  echo Visual Studio with MSBuild not found
  exit /b 1
)
call "%VSInstallDir%\VC\Auxiliary\Build\vcvars64.bat" > NUL
cd /d "%~dp0"
set "COMMON=-m -nologo -v:m -p:Configuration=%CFG%"
if /I "%SET%" == "far" goto far
if /I "%SET%" == "tests" goto tests
MSBuild.exe CE.sln %COMMON% -p:Platform=x64 "-t:ConEmu;ConEmuC;ConEmuCD;ConEmuHk"
if errorlevel 1 exit /b 1
if /I "%SET%" == "full" MSBuild.exe CE.sln %COMMON% -p:Platform=Win32 "-t:ConEmuC;ConEmuCD;ConEmuHk"
exit /b %errorlevel%
:far
MSBuild.exe CE.sln %COMMON% -p:Platform=x64 "-t:Far_ConEmuBg;Far_ConEmuLn;Far_ConEmuPlugin;Far_ConEmuTh;Far_ExtendedConsole"
if errorlevel 1 exit /b 1
MSBuild.exe CE.sln %COMMON% -p:Platform=Win32 "-t:Far_ConEmuBg;Far_ConEmuLn;Far_ConEmuPlugin;Far_ConEmuTh;Far_ExtendedConsole"
exit /b %errorlevel%
:tests
MSBuild.exe CE.sln %COMMON% -p:Platform=x64 "-t:Tests"
exit /b %errorlevel%
