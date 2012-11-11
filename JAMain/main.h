#pragma once

#include <Windows.h>

extern "C" int _stdcall TestProc (LPVOID var);
extern "C" int _stdcall CompressAsm (LPVOID params);

int parseCommand(int argc, char* argv[]);
void writeHelp();
void writeUnknow();

struct CompressParamsAsm {
	char* srcData; // dane do skompresowania
	int srcDataSize; // rozmiar danych srcData
	char* compressedData; // miejsce na skompresowane dane
	int compressedDataSize; // rozmiar danych skompresowanych (w s�owach kodowych, nie bajtach)
	char* dictData; // miejsce na s�ownik
	int dictSize; // rozmiar s�ownika
	unsigned short blockCount; // ilo�� blok�w skompresowanych danych

	CompressParamsAsm() :
		srcData(NULL),
		srcDataSize(0),
		compressedData(NULL),
		compressedDataSize(0),
		blockCount(0),
		dictData(NULL),
		dictSize(0)
	{}
};

struct temp {
	int a;
	int b;
};