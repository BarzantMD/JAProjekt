#pragma once

#include <Windows.h>

extern "C" int _stdcall TestProc (DWORD var);
extern "C" DWORD WINAPI CompressThreadAsm (LPVOID params);

int parseCommand(int argc, char* argv[]);
void writeHelp();
void writeUnknow();

struct CompressParamsAsm {
	char* srcData; // OFFSET:0  ;dane do skompresowania
	int srcDataSize; // OFFSET:4  ;rozmiar danych srcData
	char* compressedData; // OFFSET:8  ;miejsce na skompresowane dane
	int compressedDataSize; // OFFSET:12  ;rozmiar danych skompresowanych (w s³owach kodowych, nie bajtach)
	int blockCount; // OFFSET:16
	char* dictData; // OFFSET:20  ;miejsce na s³ownik
	int dictSize; // OFFSET:24  ;rozmiar s³ownika
	char* alphabet; // OFFSET:28  ;wskaŸnik na alfabet
	int alphabetSize;  // OFFSET:32  ;liczba znaków w alfabecie

	CompressParamsAsm() :
		srcData(NULL),
		srcDataSize(0),
		compressedData(NULL),
		compressedDataSize(0),
		blockCount(0),
		dictData(NULL),
		dictSize(0),
		alphabet(NULL),
		alphabetSize(0)
	{}
};

struct temp {
	int a;
	int b;
};