#include "main.h"
#include "LZW.h"
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>

using namespace std;

#define MAX_DICT_SIZE 65536 // maksymalny rozmiar s³ownika


int main(int argc, char* argv[]) {
// prototyp pracy wielu w¹tków
/*
	int threadCount = 1; // default thread count

	DWORD* threadIDArray = new DWORD[threadCount];
	HANDLE* handleArray = new HANDLE[threadCount];

	for(int i = 0; i < threadCount; ++i) {
		handleArray[i] = CreateThread(NULL, 0, CompressThread, &params, 0, &threadIDArray[i]);
	}

	WaitForMultipleObjects(threadCount, handleArray, TRUE, INFINITE);
	
	for(int i = 0; i < threadCount; ++i) {
		CloseHandle(handleArray[i]);
	}
	cout << "Zakonczono program" << endl;
	cout << "ID watkow: " << endl;
	for (int i = 0; i < threadCount; i++) {
		cout << threadIDArray[i] << endl;
	}
*/

	//system("pause");
	//return 0;

	int c = parseCommand(argc, argv);
	
/* ====================================== */
/* Nieznane polecenie - wyœwietlenie odpowiedniego komunikatu */
	if(c == -1)
		writeUnknow();

/* ====================================== */
/* KOMPRESJA ASEMBLER */
	if(c == 3) { // kompresja ASM
		CompressParamsAsm params;
		params.dictSize = 5;
		cout << "Rozmiar slownika przed: " << params.dictSize << endl;
		CompressAsm(&params);
		cout << "Rozmiar slownika po: " << params.dictSize << endl;
	}

/* ====================================== */
/* KOMPRESJA */
	if(c == 1) { // kompresja C++
		cout << "Przygotowywanie do kompresji..." << endl;

		// Parsowanie argumentów - ustalenie nazwy pliku Ÿród³owego i docelowego
		string srcFilename = argv[2];
		string destFilename;
		if(argc == 3)
			destFilename = srcFilename + ".packed";
		else
			destFilename = argv[3];

		
		fstream srcFile;
		srcFile.open(srcFilename.c_str(), ios::binary | ios::in); // otwarcie pliku Ÿród³owego

		if(!srcFile.is_open()) { // sprawdzenie czy plik Ÿród³owy istnieje
			cout << "Nie mozna otworzyc pliku zrodlowego lub plik nie istnieje!";
			system("pause");
			return 0;
		}

		// za³adowanie zawartoœci pliku do pamiêci
		srcFile.seekg(0, ios::end);
		int filesize = srcFile.tellg();
		srcFile.seekg(0, ios::beg);
		cout << "Rozmiar pliku zrodlowego: " << filesize << endl;
		
		char* buffer = new char[filesize];

		srcFile.read(buffer, filesize);
		srcFile.close();

		// przygotowanie parametrów i pamiêci dla procesu kompresji
		CompressParams params;
		params.srcData = buffer; // wskaŸnik na dane do kompresji
		params.srcDataSize = filesize; // rozmiar danych do skompresowania
		params.dictSize = MAX_DICT_SIZE; // rozmiar s³ownika
		params.compressedData = new char[filesize*2 + 256]; // zaalokowanie pamiêci dla skompresowanych danych

		// realizacja w pojedynczym watku
		HANDLE h;
		DWORD threadID;
		unsigned int start = clock();
		h = CreateThread(NULL, 0, CompressThread, &params, 0, &threadID);
		WaitForSingleObject(h, INFINITE);
		unsigned int stop = clock();
		double time = static_cast<double>(stop - start) / CLOCKS_PER_SEC;

		cout << "Zakonczono watki" << endl;
		cout << "Czas dzialania: " << time << " sekund" << endl;
		cout << "Rozmiar danych skompresowanych: " << params.compressedDataSize * 2 << endl;
		cout << "Ilosc blokow: " << params.blockCount << endl;

		// zapis do pliku:

		// wyliczenie rozmiaru danych do zapisu
		// rozmiar s³ownika + rozmiar danych (iloœæ kodów * 2 bajty) + 4 bajty na ka¿dy blok + 1 bajt rozmiaru s³ownika
		int destSize = (int)(params.compressedData[4]) + params.compressedDataSize * 2 + params.blockCount * 4 + 1;

		// otwarcie i w³aœciwy zapis do pliku
		fstream destFile;
		destFile.open(destFilename.c_str(), ios::binary | ios::out | ios::trunc);
		destFile.write((char*)(&params.blockCount), 2);
		destFile.write(params.compressedData, destSize);
		destFile.close();

		// zwolnienie zasobów
		delete params.compressedData;
		delete buffer;
	}



/* ====================================== */
/* DEKOMPRESJA */
	if(c == 2 || c == 4) { // dekompresja
		cout << "Przygotowywanie do dekompresji..." << endl;

		// Parsowanie argumentów - ustalenie nazwy pliku Ÿród³owego i docelowego
		string srcFilename = argv[2];
		string destFilename;
		if(argc == 3)
			destFilename = srcFilename + "unpacked.txt";
		else
			destFilename = argv[3];

		
		fstream srcFile;
		srcFile.open(srcFilename.c_str(), ios::binary | ios::in); // otwarcie pliku Ÿród³owego

		if(!srcFile.is_open()) { // sprawdzenie czy plik Ÿród³owy istnieje
			cout << "Nie mozna otworzyc pliku zrodlowego lub plik nie istnieje!";
			system("pause");
			return 0;
		}

		// za³adowanie zawartoœci pliku do pamiêci
		srcFile.seekg(0, ios::end);
		int filesize = srcFile.tellg();
		srcFile.seekg(0, ios::beg);
		cout << "Rozmiar pliku zrodlowego: " << filesize << endl;
		
		char* buffer = new char[filesize];

		srcFile.read(buffer, filesize);
		srcFile.close();

		// przygotowanie parametrów i pamiêci dla procesu dekompresji
		DecompressParams params;
		params.compressedData = buffer; // wskaŸnik na dane do dekompresji
		params.compressedDataSize = filesize;
		params.decompressedData = new char[filesize*3]; // wstêpnie oszacowana pamiêæ na zdekompresowane dane

		// realizacja w pojedynczym watku
		HANDLE h;
		DWORD threadID;
		unsigned int start = clock();
		h = CreateThread(NULL, 0, DecompressThread, &params, 0, &threadID);
		WaitForSingleObject(h, INFINITE);
		unsigned int stop = clock();
		double time = static_cast<double>(stop - start) / CLOCKS_PER_SEC;

		cout << "Zakonczono watki" << endl;
		cout << "Czas dzialania: " << time << " sekund" << endl;
		cout << "Rozmiar danych zdekompresowanych: " << params.decompressedDataSize << endl;

		// zapis do pliku:

		// otwarcie i w³aœciwy zapis do pliku
		fstream destFile;
		destFile.open(destFilename.c_str(), ios::binary | ios::out | ios::trunc);
		destFile.write(params.decompressedData, params.decompressedDataSize);
		destFile.close();

		// zwolnienie zasobów
		delete params.decompressedData;
		delete buffer;
	}

	system("pause");
	return 0;
}

/* Zwraca kod wybranego polecenia
-1 - nieznane polecenie
0 - w³asny komunikat funkcji
1 - kompresja
2 - dekompresja 
3 - kompresja asm
4 - dekompresja asm*/
int parseCommand(int argc, char* argv[]) {
	if(argc >=5) return -1; // zbyt wiele argumentów

	if(argc == 1) { // brak argumentów - wpisano sam¹ nazwê programu
		writeHelp();
		return 0;
	}

	if(argc == 2) { // jeden argument zosta³ podany - sprawdzenie czy u¿ytkownik prosi o pomoc
		if(string(argv[1]) == "/?" || string(argv[1]) == "/help") {
			writeHelp();
			return 0;
		}
		else
			return -1;
	}

	/* argc = <3,4> */

	if(string(argv[1]) == "/c")
		return 1;

	if(string(argv[1]) == "/d")
		return 2;

	if(string(argv[1]) == "/ca")
		return 3;

	if(string(argv[1]) == "/da")
		return 4;

	return -1;
}

void writeHelp() {
	cout << "Uzycie: JAMain opcja plik_zrodlowy [plik_docelowy]" << endl << endl;
	cout << "Opcje:" << endl;
	cout << "/c - kompresja pliku" << endl;
	cout << "/d - dekompresja pliku" << endl;
	cout << "/ca - kompresja pliku funkcja asemblera" << endl;
	cout << "/da - dekompresja pliku funkcja asemblera" << endl;
	cout << "/? - wyswietlenie tej pomocy" << endl;
	cout << "/help - wyswietlenie tej pomocy" << endl;
}

void writeUnknow() {
	cout << "Nieznane polecenie. Aby skorzystac z pomocy wpisz JAMain /? lub JAMain /help" << endl;
}