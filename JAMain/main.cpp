#include "main.h"
#include "LZW.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;

#define MAX_DICT_SIZE 65536 // maksymalny rozmiar s�ownika


int main(int argc, char* argv[]) {
// prototyp pracy wielu w�tk�w
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
/* Nieznane polecenie - wy�wietlenie odpowiedniego komunikatu */
	if(c == -1)
		writeUnknow();

/* ====================================== */
/* KOMPRESJA */
	if(c == 1 || c == 3) { // kompresja
		cout << "Przygotowywanie do kompresji..." << endl;

		// Parsowanie argument�w - ustalenie nazwy pliku �r�d�owego i docelowego
		string srcFilename = argv[2];
		string destFilename;
		if(argc == 3)
			destFilename = srcFilename + ".packed";
		else
			destFilename = argv[3];

		
		fstream srcFile;
		srcFile.open(srcFilename.c_str(), ios::binary | ios::in); // otwarcie pliku �r�d�owego

		if(!srcFile.is_open()) { // sprawdzenie czy plik �r�d�owy istnieje
			cout << "Nie mozna otworzyc pliku zrodlowego lub plik nie istnieje!";
			system("pause");
			return 0;
		}

		// za�adowanie zawarto�ci pliku do pami�ci
		srcFile.seekg(0, ios::end);
		int filesize = srcFile.tellg();
		srcFile.seekg(0, ios::beg);
		cout << "Rozmiar pliku zrodlowego: " << filesize << endl;
		
		char* buffer = new char[filesize];

		srcFile.read(buffer, filesize);
		srcFile.close();

		// przygotowanie parametr�w i pami�ci dla procesu kompresji
		CompressParams params;
		params.srcData = buffer; // wska�nik na dane do kompresji
		params.srcDataSize = filesize; // rozmiar danych do skompresowania
		params.dictData = new char[MAX_DICT_SIZE]; // alokacja pami�ci dla s�ownika
		params.dictSize = MAX_DICT_SIZE; // rozmiar s�ownika
		params.compressedData = new char[filesize*2 + 256]; // zaalokowanie pami�ci dla skompresowanych danych

		// realizacja w pojedynczym watku
		HANDLE h;
		DWORD threadID;
		h = CreateThread(NULL, 0, CompressThread, &params, 0, &threadID);
		WaitForSingleObject(h, INFINITE);

		cout << "Zakonczono watki" << endl;
		cout << "Rozmiar danych skompresowanych: " << params.compressedDataSize << endl;

		// zapis do pliku:

		// wyliczenie rozmiaru danych do zapisu
		// rozmiar s�ownika + rozmiar danych (ilo�� kod�w * 2 bajty) + 5 bajt�w nag��wku (4 bajty rozmiaru danych + 1 bajt rozmiaru s�ownika)
		int destSize = (int)(params.compressedData[4]) + params.compressedDataSize * 2 + 5;

		// otwarcie i w�a�ciwy zapis do pliku
		fstream destFile;
		destFile.open(destFilename.c_str(), ios::binary | ios::out | ios::trunc);
		destFile.write(params.compressedData, destSize);
		destFile.close();

		// zwolnienie zasob�w
		delete params.dictData;
		delete params.compressedData;
		delete buffer;
	}

	system("pause");
	return 0;
}

/* Zwraca kod wybranego polecenia
-1 - nieznane polecenie
0 - w�asny komunikat funkcji
1 - kompresja
2 - dekompresja 
3 - kompresja asm
4 - dekompresja asm*/
int parseCommand(int argc, char* argv[]) {
	if(argc >=5) return -1; // zbyt wiele argument�w

	if(argc == 1) { // brak argument�w - wpisano sam� nazw� programu
		writeHelp();
		return 0;
	}

	if(argc == 2) { // jeden argument zosta� podany - sprawdzenie czy u�ytkownik prosi o pomoc
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