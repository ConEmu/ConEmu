-- ConEmu's solution generator script
-- 1. Download premake5 from https://premake.github.io/download.html
-- 2. Run `premake5 vs2019` with Premake 5.0.0.
-- The ConEmu and Tests source globs include new GUI sources and headers.

workspace "CE"
  configurations { "Release", "Debug", "Remote" }
  platforms { "Win32", "x64" }
  -- Where *.vcxproj files would be placed
  location "src"
  --basedir "%{cfg.location}"
  -- Subdir for temporary files
  local build_dir = "_VCBUILD"

  startproject "ConEmu"

  staticruntime "On"
  multiprocessorcompile "On"
  mapfile "On"
  filter "configurations:Release"
    incrementallink "Off"

  filter "platforms:Win32"
    architecture "x86"
    defines { "WIN32", "_WIN32" }

  filter "platforms:x64"
    architecture "x64"
    defines { "WIN64", "_WIN64" }

  filter { "action:vs2019", "configurations:Release or Remote", "platforms:Win32" }
    toolset "v141_xp"
  filter { "action:vs2019", "configurations:Debug", "platforms:Win32" }
    toolset "v141_xp"
  filter "action:vs2017"
    toolset "v141_xp"
  filter "action:vs2015"
    toolset "v140_xp"
  filter "action:vs2013"
    toolset "v120_xp"
  filter "action:vs2012"
    toolset "v110_xp"

  filter "configurations:Release"
    defines { "NDEBUG", "HIDE_TODO", "NOMINMAX" }
    optimize "Size"
    symbols "On"

  filter "configurations:Debug"
    defines { "_DEBUG", "HIDE_TODO", "MSGLOGGER", "NOMINMAX" }
    symbols "On"

  filter "configurations:Remote"
    defines { "_DEBUG", "HIDE_TODO", "MSGLOGGER", "NOMINMAX" }
    symbols "On"

  filter{}

function target_dir(folder)
  filter "configurations:Release"
    targetdir("Release/"..folder)

  filter "configurations:Debug"
    targetdir("Debug/"..folder)

  filter "configurations:Remote"
    targetdir("Z:/"..folder)

  filter{}
end

function postbuild_module_(dllname, cfg, ext)
  if cfg == "Release" or cfg == "Debug" then
    postbuildcommands {
      "copy \"$(TargetPath)\" \"../"..cfg.."/plugins/ConEmu/Thumbs/"..dllname.."."..ext.."\"",
      "copy \"$(TargetDir)$(TargetName).map\" \"../"..cfg.."/plugins/ConEmu/Thumbs/"..dllname.."."..ext..".map\"",
      "copy \"$(TargetDir)$(TargetName).pdb\" \"../"..cfg.."/plugins/ConEmu/Thumbs/"..dllname.."."..ext..".pdb\"",
    }
  else
    postbuildcommands {
      "copy \"$(TargetPath)\" \"Z:/plugins/ConEmu/Thumbs/"..dllname.."."..ext.."\"",
      "copy \"$(TargetDir)$(TargetName).map\" \"Z:/plugins/ConEmu/Thumbs/"..dllname.."."..ext..".map\"",
      "copy \"$(TargetDir)$(TargetName).pdb\" \"Z:/plugins/ConEmu/Thumbs/"..dllname.."."..ext..".pdb\"",
    }
  end
end

function postbuild_module(dllname)
  filter { "configurations:Release", "platforms:Win32" }
    postbuild_module_(dllname, "Release", "t32")

  filter { "configurations:Release", "platforms:x64" }
    postbuild_module_(dllname, "Release", "t64")

  filter { "configurations:Debug", "platforms:Win32" }
    postbuild_module_(dllname, "Debug", "t32")

  filter { "configurations:Debug", "platforms:x64" }
    postbuild_module_(dllname, "Debug", "t64")

  filter { "configurations:Remote", "platforms:Win32" }
    postbuild_module_(dllname, "Z:/", "t32")

  filter { "configurations:Remote", "platforms:x64" }
    postbuild_module_(dllname, "Z:/", "t64")

  filter {}
end



local common_remove = {
  "**/ExecPty.*",
  "**/ProcList.*",
  "**/Processes.*",
  "**/base64.*",
  "**/ConsoleTrueMod_Concept.h",
  "**/*.bak",
  "**/!*.*",
  "**/_*.*",
}

local tests_remove = {
  "**/*_test.cpp",
}

local common_kernel = {
  "src/common/CEHandle.*",
  "src/common/CEStr.*",
  "src/common/CmdArg.*",
  "src/common/CmdLine.*",
  "src/common/Common.cpp",
  "src/common/Common.h",
  "src/common/ConEmuCheck.*",
  "src/common/ConsoleMixAttr.*",
  "src/common/ConsoleRead.*",
  "src/common/EnvVar.*",
  "src/common/Execute.*",
  "src/common/HandleKeeper.*",
  "src/common/HkFunc.*",
  "src/common/InQueue.*",
  "src/common/Keyboard.*",
  "src/common/MAssert.*",
  "src/common/MConHandle.*",
  "src/common/Memory.*",
  "src/common/MEvent.*",
  "src/common/MFileLog.*",
  "src/common/MHandle.*",
  "src/common/MModule.*",
  "src/common/MProcess.*",
  "src/common/MProcessBits.*",
  "src/common/MRect.*",
  "src/common/MSection.*",
  "src/common/MSectionSimple.*",
  "src/common/MSecurity.*",
  "src/common/MStrDup.*",
  "src/common/MStrSafe.*",
  "src/common/MWnd.*",
  "src/common/RConStartArgs.*",
  "src/common/WCodePage.*",
  "src/common/WConsole.*",
  "src/common/WModuleCheck.*",
  "src/common/WObjects.*",
  "src/common/WThreads.*",
}

local conemu_remove = {
  "**/ConEmuMinGW.rc",
  "**/ToolBarClass.*",
  "**/Theming.*",
  "**/FontRanges.*",
  "**/*-old.*",
  "**/RegExp.*",
  "**/ConsoleTrueMod_Concept.h",
  "**/hourglass.bmp",
  "**/UserImages.bmp",
  "**/User16.bmp",
  "**/Shield32*.bmp",
  "**/Scroll256.bmp",
  "**/Scroll16.bmp",
  "**/Scroll0.bmp",
  "**/base64.h",
  "**/!*.*",
}



-- ############################### --
-- ############################### --
-- ############################### --
project "common-kernel"
  kind "StaticLib"
  language "C++"
  exceptionhandling "Off"

  files (common_kernel)
  files {"src/common/defines.h"}

  removefiles (common_remove)
  removefiles (tests_remove)

  vpaths {
    ["Headers"] = {"**.h"},
    ["Sources"] = {"**.cpp"},
  }

  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  targetdir "%{cfg.objdir}"
  filter "platforms:Win32"
    targetsuffix "32"
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "common-kernel"




-- ############################### --
-- ############################### --
-- ############################### --
project "common-user"
  kind "StaticLib"
  language "C++"
  --exceptionhandling "Off"

  files {
    "src/common/*.cpp",
    "src/common/*.h",
  }

  removefiles (common_remove)
  removefiles (common_kernel)
  removefiles (tests_remove)

  vpaths {
    ["Headers"] = {"**.h"},
    ["Sources"] = {"**.cpp"},
  }

  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  targetdir ("%{cfg.objdir}")
  filter "platforms:Win32"
    targetsuffix "32"
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "common-user"



-- ############################### --
-- ############################### --
-- ############################### --
project "ConEmu"
  kind "WindowedApp"
  language "C++"
  --exceptionhandling "Off"
  entrypoint "WinMainCRTStartup"

  links {
    "common-kernel",
    "common-user",
    "comctl32",
    "shlwapi",
    "version",
    "gdiplus",
    "winmm",
    "netapi32",
  }

  -- we can't use pch because SHOWDEBUGSTR is defined or not per source file
  -- pchheader "Header.h"
  -- pchsource "src/ConEmu/Header.cpp"

  files {
    "src/common/*.h",
    "src/ConEmu/*.cpp",
    "src/ConEmu/*.h",
    "src/ConEmu/*.rc",
    "src/ConEmu/*.rc2",
    "src/ConEmu/*.bmp",
    "src/ConEmu/*.cur",
    "src/ConEmu/*.ico",
    "Release/ConEmu/ConEmu.l10n",
    "src/ConEmu/conemu.gcc.manifest",
  }

  removefiles (conemu_remove)
  removefiles (common_remove)
  removefiles (tests_remove)

  vpaths {
    { ["Common"]    = {"src/common/*.h"} },
    { ["Resources"] = {"**/*.rc", "**/*.rc2", "**/*.manifest", "**/*.bmp", "**/*.cur", "**/*.ico"} },
    { ["Localizations"] = {"**/Lng*.*", "Release/ConEmu/ConEmu.l10n"} },
    { ["Exports"]   = {"**.def"} },
    { ["Macro"]     = {"**/Macro*.*"} },
    { ["Graphics"]  = {"**/GdiObjects.*", "**/Font*.*", "**/CustomFonts.*", "**/Background.*", "**/ColorFix.*",
                       "**/IconList.*", "**/ImgButton.*", "**/LoadImg.*", "**/ScreenDump.*", "**/ToolImg.*"} },
    { ["Settings"]  = {"**/SetPg*.*", "**/Options*.*", "**/SettingsStorage.*"} },
    { ["Headers"]   = {"**/*.h"} },
    { ["Sources"]   = {"**/*.cpp"} },
  }

  target_dir("")
  targetname "ConEmu"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "ConEmu"




-- ############################### --
-- ############################### --
-- ############################### --
project "ConEmuC"
  kind "ConsoleApp"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  dependson {
    "ConEmuCD",
    "ConEmuHk",
  }

  files {
    "src/ConEmuC/*.cpp",
    "src/ConEmuC/*.h",
    "src/ConEmuC/*.rc",
    "src/ConEmuC/ConEmuC.exe.manifest",
  }

  removefiles (tests_remove)

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  filter { "configurations:Debug or Remote" }
  postbuildcommands {
    '{ECHO} Copying "%{wks.location}../Release/ConEmu/*.cmd" "%{cfg.targetdir}"',
    '{COPYFILE} "%{wks.location}/../Release/ConEmu/*.cmd" "%{cfg.targetdir}"',
    '{ECHO} Copying "%{wks.location}../Release/ConEmu/ConEmu.l10n" "%{cfg.targetdir}"',
    '{COPYFILE} "%{wks.location}/../Release/ConEmu/ConEmu.l10n" "%{cfg.targetdir}"',
  }
  filter{}

  target_dir("ConEmu/")
  targetname "ConEmuC"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "ConEmuC"




-- ############################### --
-- ############################### --
-- ############################### --
project "ConEmuCD"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  filter {"action:vs*", "platforms:Win32"}
    linkoptions { "/DYNAMICBASE:NO", "/FIXED:NO", "/BASE:0x6F780000" }
  filter {"action:vs*", "platforms:x64"}
    linkoptions { "/DYNAMICBASE:NO", "/FIXED:NO", "/BASE:0x6F780000000" }
  filter {"action:gmake", "platforms:Win32"}
    linkoptions { "--image-base=0x6F780000" }
  filter {"action:gmake", "platforms:x64"}
    linkoptions { "--image-base=0x6F780000000" }
  filter {}

  defines { "DLL_CONEMUCD_EXPORTS" }

  links {
    "common-kernel",
    "common-user",
  }

  files {
    "src/common/Common.h",
    "src/ConEmuCD/*.cpp",
    "src/ConEmuCD/*.h",
    "src/ConEmuHk/Injects.*",
    "src/ConEmuCD/*.rc",
  }

  removefiles (tests_remove)

  filter "action:vs*"
    files "src/ConEmuCD/export.def"
  filter "action:gmake"
    files "src/ConEmuCD/export.gcc.def"
  filter {}

  vpaths {
    { ["Exports"]   = {"**.def", "**/ExportedFunctions.h"} },
    { ["Interface"] = {"**/Common.h", "**/SrvCommands.*", "**/Queue.*", "**/SrvPipes.*"} },
    { ["Automation"] = {"**/Actions.*", "**/GuiMacro.*"} },
    { ["Console"] = {"**/ConAnsi.*", "**/ConAnsiImpl.*", "**/ConData.*"} },
    { ["Server"] = {"**/ConEmuSrv.*", "**/ConEmuCmd.*", "**/WorkerBase.*"} },
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
  }

  target_dir("ConEmu/")
  targetname "ConEmuCD"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "ConEmuCD"




-- ############################### --
-- ############################### --
-- ############################### --
project "ConEmuHk"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  filter {"action:vs*", "platforms:Win32"}
    linkoptions { "/DYNAMICBASE:NO", "/FIXED:NO", "/BASE:0x7E110000" }
  filter {"action:vs*", "platforms:x64"}
    linkoptions { "/DYNAMICBASE:NO", "/FIXED:NO", "/BASE:0x7E1100000000" }
  filter {"action:gmake", "platforms:Win32"}
    linkoptions { "--image-base=0x7E110000" }
  filter {"action:gmake", "platforms:x64"}
    linkoptions { "--image-base=0x7E1100000000" }
  filter {}

  --filter { "configurations:Release" }
  --  optimize "Full"
  --filter {}

  defines { "CONEMU_MINIMAL" }

  links {
    "common-kernel",
  }

  files {
    "src/ConEmu/ConsoleMessages.h",
    "src/ConEmu/DebugMsgLog.h",
    "src/ConEmuHk/*.cpp",
    "src/ConEmuHk/*.h",
    "src/ConEmuHk/*.rc",
    "src/ConEmuHk/hde.c",
    "src/modules/terminals/ConnectorAPI.h",
    "src/modules/minhook/src/**.c",
    "src/modules/minhook/src/**.h",
    "src/modules/minhook/include/*.h",
    "src/modules/minhook/LICENSE.txt",
  }

  removefiles {
    "**/CETaskBar_.*",
    "**/*-orig.*",
  }
  removefiles (tests_remove)

  filter { "files:**/HDE/*.*" }
    excludefrombuild "On"
  filter {}

  filter "action:vs*"
    files "src/ConEmuHk/export.def"
  filter "action:gmake"
    files "src/ConEmuHk/export.gcc.def"
  filter {}

  vpaths {
    { ["MinHook/HDE"] = {"**/minhook/src/HDE/*.*"} },
    { ["MinHook/Trampoline"] = {"**/minhook/src/*.*"} },
    { ["MinHook"] = {"**/minhook/**.*", "**/hde.c"} },
    { ["Hooks"] = {"**/hk*.*"} },
    { ["Connector"] = {"**/Connector*.*"} },
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  target_dir("ConEmu/")
  targetname "ConEmuHk"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "ConEmuHk"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuPlugin"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  links {
    "common-kernel",
    "common-user",
    "version",
  }

  files {
    "src/ConEmuPlugin/*.cpp",
    "src/ConEmuPlugin/*.h",
    "src/ConEmuPlugin/*.rc",
    "src/ConEmuPlugin/Lang.templ",
  }

  filter "action:vs*"
    files "src/ConEmuPlugin/export.def"
  filter "action:gmake"
    files "src/ConEmuPlugin/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest", "**.templ"} },
    { ["Exports"]   = {"**.def"} },
  }

  target_dir("plugins/ConEmu/")
  targetname "ConEmu"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix ".x64"
  filter {}
-- end of "Far.ConEmuPlugin"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuBg"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  links {
    "common-kernel",
    "common-user",
    "version",
  }

  files {
    "src/ConEmuBg/*.cpp",
    "src/ConEmuBg/*.h",
    "src/ConEmuBg/*.rc",
    "src/ConEmuBg/Lang.templ",
  }

  filter "action:vs*"
    files "src/ConEmuBg/export.def"
  filter "action:gmake"
    files "src/ConEmuBg/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest", "**.templ"} },
    { ["Exports"]   = {"**.def"} },
  }

  target_dir("plugins/ConEmu/Background/")
  targetname "ConEmuBg"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix ".x64"
  filter {}
-- end of "Far.ConEmuBg"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuLn"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  links {
    "common-kernel",
    "common-user",
    "version",
  }

  files {
    "src/ConEmuLn/*.cpp",
    "src/ConEmuLn/*.h",
    "src/ConEmuLn/*.rc",
    "src/ConEmuLn/Lang.templ",
  }

  filter "action:vs*"
    files "src/ConEmuLn/export.def"
  filter "action:gmake"
    files "src/ConEmuLn/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest", "**.templ"} },
    { ["Exports"]   = {"**.def"} },
  }

  target_dir("plugins/ConEmu/Lines/")
  targetname "ConEmuLn"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix ".x64"
  filter {}
-- end of "Far.ConEmuLn"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh"
  kind "SharedLib"
  language "C++"
  --exceptionhandling "Off"

  links {
    "common-kernel",
    "common-user",
    "version",
  }

  files {
    "src/ConEmuTh/*.cpp",
    "src/ConEmuTh/*.h",
    "src/ConEmuTh/*.rc",
    "src/ConEmuTh/Lang.templ",
  }

  filter "action:vs*"
    files "src/ConEmuTh/export.def"
  filter "action:gmake"
    files "src/ConEmuTh/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest", "**.templ", "**.templ"} },
    { ["Exports"]   = {"**.def"} },
  }

  target_dir("plugins/ConEmu/Thumbs/")
  targetname "ConEmuTh"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix ".x64"
  filter {}
-- end of "Far.ConEmuTh"



-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh.gdi+"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  files {
    "src/ConEmuTh/Modules/gdip/*.cpp",
    "src/ConEmuTh/Modules/gdip/*.h",
    "src/ConEmuTh/Modules/gdip/*.rc",
    "src/ConEmuTh/Modules/gdip/gdip.def",
    "src/ConEmuTh/Modules/ThumbSDK.h",
    "src/ConEmuTh/Modules/MStream.h",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  targetname "gdi+"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  targetdir ("%{cfg.objdir}")
  implibdir ("%{cfg.objdir}")
  postbuild_module ("gdi+")
-- end of "Far.ConEmuTh.gdi+"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh.ico"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  files {
    "src/ConEmuTh/Modules/ico/*.cpp",
    "src/ConEmuTh/Modules/ico/*.h",
    "src/ConEmuTh/Modules/ico/*.rc",
    "src/ConEmuTh/Modules/ico/ico.def",
    "src/ConEmuTh/Modules/ThumbSDK.h",
    "src/ConEmuTh/Modules/MStream.h",
  }

  removefiles {
    "**/!*.*",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  targetname "ico"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  targetdir ("%{cfg.objdir}")
  implibdir ("%{cfg.objdir}")
  postbuild_module ("ico")
-- end of "Far.ConEmuTh.ico"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ConEmuTh.pe"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  files {
    "src/ConEmuTh/Modules/pe/*.cpp",
    "src/ConEmuTh/Modules/pe/*.h",
    "src/ConEmuTh/Modules/pe/*.rc",
    "src/ConEmuTh/Modules/pe/pe.def",
    "src/ConEmuTh/Modules/ThumbSDK.h",
  }

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  targetname "pe"
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  targetdir ("%{cfg.objdir}")
  implibdir ("%{cfg.objdir}")
  postbuild_module ("pe")
-- end of "Far.ConEmuTh.pe"




-- ############################### --
-- ############################### --
-- ############################### --
project "Far.ExtendedConsole"
  kind "SharedLib"
  language "C++"
  exceptionhandling "Off"

  links {
    "common-kernel",
  }

  files {
    "src/ConEmuDW/*.cpp",
    "src/ConEmuDW/*.h",
    "src/ConEmuDW/*.rc",
  }

  removefiles {
    "**/!*.*",
  }

  filter "action:vs*"
    files "src/ConEmuDW/export.def"
  filter "action:gmake"
    files "src/ConEmuDW/export.gcc.def"
  filter {}

  vpaths {
    { ["Headers"] = {"**.h"} },
    { ["Sources"] = {"**.cpp"} },
    { ["Resources"] = {"**.rc", "**.rc2", "**.manifest"} },
    { ["Exports"]   = {"**.def"} },
  }

  target_dir("ConEmu/") -- local function
  targetname ("ExtendedConsole")
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  -- implibdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")
  filter "platforms:Win32"
    targetsuffix ""
  filter "platforms:x64"
    targetsuffix "64"
  filter {}
-- end of "Far.ExtendedConsole"



-- ############################### --
-- ############################### --
-- ############################### --
project "Tests"
  kind "ConsoleApp"
  language "C++"
  exceptionhandling "On"

  defines {"TESTS_MEMORY_MODE", "CE_UNIT_TEST=1"}

  files {
    -- tests
    "src/UnitTests/*_test.cpp",
    "src/UnitTests/test_stubs.cpp",
    "src/UnitTests/test_mock*.*",
    "src/**/*_test.cpp",
    -- common files
    "src/common/*.cpp",
    "src/common/*.h",
    -- main sources
    "src/ConEmu/*.rc",
    "src/ConEmu/*.cpp",
    "src/ConEmu/*.h",
    "src/ConEmu/conemu.gcc.manifest",
    -- server sources
    "src/ConEmuCD/StartEnv.cpp",
    -- conemuhk sources
    "src/ConEmuHk/DllOptions.cpp",
    "src/ConEmuHk/ShellProcessor.cpp",
    -- googletest
    "src/modules/googletest/googletest/src/gtest-all.cc",
  }

  removefiles (common_remove)
  removefiles (conemu_remove)
  removefiles {
    "src/ConEmu/ConEmuApp.*",
    "src/UnitTests/test_*/*.cpp",
  }

  vpaths {
    { ["mocks"] = {"**/test_stubs.cpp", "**/test_mock*.*"} },
    { ["tests"] = {"**/*_test.*"} },
    { ["Resources"] = {"**/*.rc", "**/*.rc2", "**/*.manifest", "**/*.bmp", "**/*.cur", "**/*.ico"} },
  }

  includedirs {
    "src/modules/googletest/googletest/include",
    "src/modules/googletest/googletest",
  }

  links {
    -- don't link "common-kernel" or  "common-user"!
    "comctl32",
    "shlwapi",
    "version",
    "gdiplus",
    "winmm",
    "netapi32",
  }

  target_dir("")
  targetname ("Tests_%{cfg.buildcfg}_%{cfg.platform}")
  objdir ("%{wks.location}/"..build_dir.."/%{cfg.buildcfg}/%{prj.name}_%{cfg.platform}")
  implibdir ("%{cfg.objdir}")

  -- postbuildcommands {"$(SolutionDir)UnitTests\\Tests_%{cfg.buildcfg}_%{cfg.platform}.exe"}

  filter {}
-- end of "Tests"
