#pragma once

#include <Windows.h>

extern "C" int _stdcall MyProc1 (DWORD x, DWORD y);
extern "C" int _stdcall TestProc (DWORD x, DWORD y);


int parseCommand(int argc, char* argv[]);
void writeHelp();
void writeUnknow();