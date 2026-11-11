# Mouse-wheel scrollback with hidden Far panels

This opt-in setting is shared by the x86, x64 and ARM64 builds:

**Settings → Far Manager → Wheel scrolls buffer when Far panels are hidden**

In Russian: **Прокручивать буфер колесом при скрытых панелях Far**.
The label and tooltip are translated into all 12 supported interface languages.
The saved XML/registry value is `FarWheelScrollback` (boolean, off by default).

## Setup

1. Update both ConEmu and its main Far plugin from the matching architecture ZIP.
2. Start Far directly inside ConEmu with `Far.exe -w`, and allow a console buffer
   taller than the window (for example, 1000 lines in ConEmu settings).
3. Enable the setting above and save settings.
4. Hide both panels with Ctrl+O. The wheel now scrolls output, rather than Far's
   command history. Ctrl+wheel scrolls by pages.

No ConEmuHk or Lua macro is required. Remove an earlier wheel-to-`Scroll()` Lua
macro if one was installed. ZIPs contain neither user profiles nor a ConEmu.xml,
so updating the files does not enable the setting or replace existing settings.
This applies to direct Far sessions, not to running Far through `/PTY`.

## Behavior

The GUI consumes wheel events before the console mouse-input path can reset its
viewport. Scrolling moves the virtual GUI viewport while keeping the real console
window at Far's input cursor, so a Far redraw does not pull the view back down.
Typing and other existing actions may still intentionally return to the live view.

The main Far plugin reports panel visibility and the current macro area using its
existing shared mapping. Panel API calls run on Far's main thread; hook-free
sessions schedule a metadata refresh through Far's synchronization API, even
without an optional background plugin. Editor/dialog areas are distinguished
correctly instead of treating every area as the shell.

With visible panels, an editor or a dialog active, wheel events retain their
existing application behavior. Missing or invalid plugin metadata falls back to
the previous behavior. Existing configurations remain unchanged unless enabled.

## Validation

Native ARM64 manual testing confirmed wheel scrolling up/down with pauses, no
snap to the bottom and no command-history cycling, with both panels hidden and
neither ConEmuHk nor a Lua wheel macro loaded. API checks separately confirmed
that the real console viewport stays at its input cursor during virtual scrolling.
Intel builds are tested under Windows ARM64 emulation, not on physical Intel PCs;
their automated startup/API checks are recorded in the packaged validation report.
