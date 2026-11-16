#pragma once

// Run an explicit terminal session without injecting into the hosted process.
int RunConPty(const wchar_t* command);
