#pragma once

#include <Windows.h>

extern "C" int _stdcall MyProc1 (DWORD x, DWORD y);
extern "C" int _stdcall TestProc (DWORD x, DWORD y);
extern "C" int _stdcall TestProc2 (LPVOID x);


int parseCommand(int argc, char* argv[]);
void writeHelp();
void writeUnknow();

struct temp {
	int a;
	int b;
};