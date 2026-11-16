# ConEmu contributions

- Keep pull requests independently buildable and reviewable. Propose CI and provider changes separately from product features. Explain any dependencies in the PR.
- Preserve file encoding, UTF-8 BOM and line endings. Use UTF-8 with a BOM for new C++ sources and headers under `src/ConEmu`. Keep whitespace changes limited to the edited code.
- Edit `premake5.lua` for project configuration changes. New GUI sources and headers are picked up by wildcards. After initializing submodules, regenerate project and filter files with Premake 5.0.0-alpha15 using `premake5 vs2019`.
- Credit reused commits and identify their licenses in the PR. Preserve required notices and coordinate overlapping work with its author.
- Run `powershell -NoProfile -File Deploy/Check-Contribution.ps1 -Base origin/master` from a clean checkout. Substitute the actual PR base, such as `origin/daily`. Build and run the Release tests for Win32 and x64.
