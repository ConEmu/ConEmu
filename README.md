## About ConEmu

[![master build status](https://dev.azure.com/MaksimMoisiuk/conemu/_apis/build/status/Maximus5.ConEmu?branchName=master&label=master)](https://dev.azure.com/MaksimMoisiuk/conemu/_build/latest?definitionId=1&branchName=master&label=master)
[![daily build status](https://dev.azure.com/MaksimMoisiuk/conemu/_apis/build/status/Maximus5.ConEmu?branchName=daily&label=daily)](https://dev.azure.com/MaksimMoisiuk/conemu/_build/latest?definitionId=1&branchName=daily&label=daily)

[ConEmu-Maximus5](https://conemu.github.io) is a Windows console emulator with tabs, which represents
multiple consoles as one customizable GUI window with various features.

Initially, the program was created as a companion to
[Far Manager](http://en.wikipedia.org/wiki/FAR_Manager),
my favorite shell replacement - file and archive management,
command history and completion, powerful editor.

Today, ConEmu can be used with any other console application or simple GUI tools
(like PuTTY for example). ConEmu is an active project, open to
[suggestions](https://github.com/Maximus5/ConEmu/issues).

<a href="https://www.fosshub.com/ConEmu.html">![Fosshub.com ConEmu mirror](https://github.com/Maximus5/ConEmu/wiki/downloads-new.png)</a>
<a href="https://conemu.github.io/donate.html">![Donate](https://github.com/Maximus5/ConEmu/wiki/donate-new.png)</a>

Take a look at [screencast](http://dotnetsurfers.com/blog/2013/12/15/developer-tools-screencast-7-conemu/) about ConEmu.

This fork grew up from ConEmu by Zoin.


## License (BSD 3-clause)
    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
    IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
    OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.

See [Release/ConEmu/License.txt](https://github.com/Maximus5/ConEmu/blob/master/Release/ConEmu/License.txt) for the full license text.


## Some links
Wiki: https://conemu.github.io/en/TableOfContents.html
What's new: https://conemu.github.io/en/Whats_New.html
Release stages: https://conemu.github.io/en/StableVsPreview.html
Donate this project: <a ref="https://conemu.github.io/donate.html" rel="nofollow">https://conemu.github.io/donate.html</a>



## Description
ConEmu starts a console program in hidden console window and provides
an alternative customizable GUI window with various features:

  * smooth window resizing;
  * tabs and splits (panes);
  * easy run old DOS applications (games) in Windows 7 or 64bit OS (DosBox required);
  * quake-style, normal, maximized and full screen window graphic modes;
  * window font anti-aliasing: standard, clear type, disabled;
  * window fonts: family, height, width, bold, italic, etc.;
  * using normal/bold/italic fonts for different parts of console simultaneously;
  * cursor: standard console (horizontal) or GUI (vertical);
  * and more, and more...

### Far Manager related features
  * tabs for editors, viewers, panels and consoles;
  * thumbnails and tiles;
  * show full output (1K+ lines) of last command in editor/viewer;
  * customizable right click behaviour (long click opens context menu);
  * drag and drop (explorer style);
  * and more, and more...

All settings are read from the registry or ConEmu.xml file, after which the
[command line parameters](https://conemu.github.io/en/CommandLine.html)
are applied. You may easily use several named configurations (for different PCs for example).


## Requirements
  * Windows XP or later for 32-bit.
  * Windows Vista or later for 64-bit.


## Installation
In general, ConEmu installation is easy.
Just unpack or install to any folder and run `ConEmu.exe`.

Read [Installation wiki](https://conemu.github.io/en/Installation.html)
about release stages, distro packets, some warnings and much more...


## Windows dark mode

On supported Windows 10 and 11 versions, ConEmu can follow Windows app mode or
use a Dark or Light override in **Settings > Appearance**. High contrast uses
the Windows colors. See [ConEmuDark by 59de44955ebd](https://github.com/ConEmu/ConEmu/pull/2604)
for the original main window, Settings and About screenshots, and
[Sim-md's Windows theming](https://github.com/ConEmu/ConEmu/pull/2643).

## Building from sources
https://github.com/Maximus5/ConEmu/blob/master/src/HowToBuild.md

## Screenshots
![Splits and tabs in ConEmu](https://github.com/Maximus5/ConEmu/wiki/ConEmuSplits.png)

![ConEmu+Powershell inside Windows Explorer pane](https://github.com/Maximus5/ConEmu/wiki/ConEmuInside.png)

[More screenshots](https://conemu.github.io/en/Screenshots.html)
