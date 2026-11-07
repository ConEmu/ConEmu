# Building ConEmu for Windows ARM64

Use a POSIX environment such as WSL with GNU Make, standard shell utilities,
and the LLVM-MinGW cross toolchain on `PATH`. Use a checkout and build path
without spaces for the existing GNU Make rules.

```sh
git submodule update --init --recursive
export PATH="/path/to/llvm-mingw/bin:$PATH"
sh build-arm64.sh
```

The script builds the GUI, console executable, server DLL, and the Far plugins.
The package includes ConEmuBg, ConEmuLn, and ConEmuTh with its GDI+, ICO, and
PE thumbnail modules. Keep their subdirectories and resource files together.
The files to install are under `Release-arm64/package`. Set `BUILD_DIR` to an
absolute path to change the build directory or `JOBS` to change the number of
parallel Make jobs.

## Run with Far Manager

1. Copy the contents of `Release-arm64/package` into the ARM64 Far Manager
   directory, preserving the `ConEmu` and `Plugins/ConEmu` subdirectories.
   Use the ARM64 Far distribution with its LuaMacro plugin for panel mouse
   handling.
2. Start `ConEmu64.exe -run Far.exe` from that directory. In ConEmu settings
   under **Far Manager**, enable **LDrag** and **Drop & Confirmation**.
3. To copy from Explorer, hold Ctrl while dropping a file on a Far file panel.
   The copy should appear in that panel's directory.
4. To copy from Far, start with the left mouse button without modifiers when
   using the default left-drag binding. Hold Ctrl after the drag starts and
   drop in an Explorer folder.

The ARM64 build disables injection and uses the Far plugin's IPC for panel
information and drag operations. Features that require `ConEmuHk` are
unavailable in this build.
