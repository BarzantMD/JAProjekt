#include "main.h"
#include "LZW.h"
#include <iostream>
#include <fstream>
#include <string>
#include <time.h>
#include <sstream>

// mo¿emy zdefiniowaæ na sztywno na ilu w¹tkach ma dzia³aæ program
//#define NUM_OF_CORES 4

int threadCount = 0;

using namespace std;

int main(int argc, char* argv[]) {
	int c = parseCommand(argc, argv);

	// ustalenie liczby w¹tków
#ifndef NUM_OF_CORES
	// jeœli nie zdefiniowano na sztywno liczby w¹tków, to wczytujemy iloœæ
	// rdzeni komputera
	if(threadCount == 0) {
		SYSTEM_INFO sysinfo;
		GetSystemInfo(&sysinfo);
		threadCount = sysinfo.dwNumberOfProcessors;
	}
#else
	// liczba w¹tków na sztywno zdefiniowana
	threadCount = NUM_OF_CORES;
#endif

	
/* ====================================== */
/* Nieznane polecenie - wyœwietlenie odpowiedniego komunikatu */
	if(c == -1)
		writeUnknow();

/* ====================================== */
/* KOMPRESJA ASEMBLER */
	if(c == 3) { // kompresja ASM
		cout << "Przygotowywanie do kompresji..." << endl;
		cout << "Ilosc watkow: " << threadCount << endl;

		// Parsowanie argumentów - ustalenie nazwy pliku Ÿród³owego i docelowego
		string srcFilename = argv[2];
		string destFilename;
		if(argc == 3)
			destFilename = srcFilename + ".asmpacked";
		else
			destFilename = argv[3];

		
		fstream srcFile;
		srcFile.open(srcFilename.c_str(), ios::binary | ios::in); 

		if(!srcFile.is_open()) { 
			cout << "Nie mozna otworzyc pliku zrodlowego lub plik nie istnieje!";
			return 0;
		}

		// za³adowanie zawartoœci pliku do pamiêci
		srcFile.seekg(0, ios::end);
		int filesize = srcFile.tellg();
		srcFile.seekg(0, ios::beg);
		cout << "Rozmiar pliku zrodlowego: " << filesize << endl;

		// ograniczenie pliku przeznaczonego do kompresji do 20MB (dla uproszczenia gospodarki pamiêci¹)
		if(filesize > 20480000){
			cout << "Plik jest zbyt duzy! (Ograniczenie do 20MB)" << endl;
			srcFile.close();
			return 0;
		}
		
		char* buffer = new char[filesize];

		srcFile.read(buffer, filesize);
		srcFile.close();

		// przygotowanie alfabetu
		char* alphabet = new char[256];

		int alphabetSize = 0;
		for (int i = 0; i < filesize; i++) { 
			// czy jest ju¿ w s³owniku
			bool isAlready = false; 

			for (int j = 0; j < alphabetSize; j++) {
				if(alphabet[j] == buffer[i]) {
					isAlready = true;
					break;
				}
			}

			if(!isAlready) {
					alphabet[alphabetSize] = buffer[i];
					alphabetSize++;
				}
		}

		// przygotowanie danych dla w¹tków
		DWORD* threadIDArray = new DWORD[threadCount];
		HANDLE* handleArray = new HANDLE[threadCount];

		CompressParamsAsm* paramsArray = new CompressParamsAsm[threadCount];

		// rozmiar danych dla w¹tków (oprócz ostatniego)
		int dataSizePerThread = filesize / threadCount;
		// rozmiar danych dla ostatniego w¹tku (reszta danych)
		// ca³kowity rozmiar danych minus rozmiar danych dla poprzednich w¹tków
		int restDataSize = filesize - (dataSizePerThread * (threadCount - 1));

		for (int i = 0; i < threadCount; i++) {
			paramsArray[i].alphabet = alphabet;
			paramsArray[i].alphabetSize = alphabetSize;
			paramsArray[i].dictSize = MAX_DICT_DATA_SIZE;
			paramsArray[i].dictData = new char[MAX_DICT_DATA_SIZE * 2 + 65536];
			paramsArray[i].srcData = &(buffer[i * dataSizePerThread]);
			// maksymalnie dwukrotny przyrost danych (gdy kompresujemy pliki binarne zamiast tekstowych)
			paramsArray[i].compressedData = new char[50120000/threadCount];
			
			if(i == (threadCount - 1)) {
				// parametry dla ostatniego w¹tku
				paramsArray[i].srcDataSize = restDataSize;
			}
			else {
				// parametry dla wszystkich w¹tków oprócz ostatniego
				paramsArray[i].srcDataSize = dataSizePerThread;
			}
		}

		cout << "Kompresja danych. Prosze czekac..." << endl;

		// uruchomienie w¹tków
		unsigned int start = clock();

		for(int i = 0; i < threadCount; ++i) {
			handleArray[i] = CreateThread(NULL, 0, CompressThreadAsm, &paramsArray[i], 0, &threadIDArray[i]);
		}

		WaitForMultipleObjects(threadCount, handleArray, TRUE, INFINITE);
		unsigned int stop = clock();
		double time = static_cast<double>(stop - start) / CLOCKS_PER_SEC;

		for(int i = 0; i < threadCount; ++i) {
			CloseHandle(handleArray[i]);
		}

		// podliczenie sumarycznych danych skompresowanych oraz ka¿dego w¹tku
		int summaryCompressedDataSize = 0;
		int summaryBlockCount = 0;
		for (int i = 0; i < threadCount; i++) {
			summaryCompressedDataSize += paramsArray[i].compressedDataSize;
			summaryBlockCount += paramsArray[i].blockCount;
		}

		cout << "Czas dzialania: " << time << " sekund" << endl;

		cout << "Rozmiar danych po skompresowaniu: " << summaryCompressedDataSize * 2 << endl;

		fstream destFile;
		destFile.open(destFilename.c_str(), ios::binary | ios::out | ios::trunc);
		destFile.write((char*)(&summaryBlockCount), 2);
		destFile.write((char*)(&alphabetSize), 2); 
		destFile.write(alphabet, alphabetSize); 

		// zapis danych z poszczególnych w¹tków
		for (int i = 0; i < threadCount; i++) {
			// wyliczenie rozmiaru danych do zapisu
			// rozmiar danych (iloœæ kodów * 2 bajty) + 4 bajty na ka¿dy blok
			int destSize = (int)(paramsArray[i].compressedDataSize * 2 + paramsArray[i].blockCount * 4);
			destFile.write(paramsArray[i].compressedData, destSize);
		}

		destFile.close();

		// zwolnienie zasobów
		for (int i = 0; i < threadCount; i++) {
			delete paramsArray[i].compressedData;
			delete paramsArray[i].dictData;
		}
		delete buffer;
	}

/* ====================================== */
/* KOMPRESJA */
	if(c == 1) { // kompresja C++
		cout << "Przygotowywanie do kompresji..." << endl;
		cout << "Ilosc watkow: " << threadCount << endl;

		// Parsowanie argumentów - ustalenie nazwy pliku Ÿród³owego i docelowego
		string srcFilename = argv[2];
		string destFilename;
		if(argc == 3)
			destFilename = srcFilename + ".packed";
		else
			destFilename = argv[3];

		
		fstream srcFile;
		srcFile.open(srcFilename.c_str(), ios::binary | ios::in); 

		if(!srcFile.is_open()) { 
			cout << "Nie mozna otworzyc pliku zrodlowego lub plik nie istnieje!";
			return 0;
		}

		// za³adowanie zawartoœci pliku do pamiêci
		srcFile.seekg(0, ios::end);
		int filesize = srcFile.tellg();
		srcFile.seekg(0, ios::beg);
		cout << "Rozmiar pliku zrodlowego: " << filesize << endl;

		// ograniczenie pliku przeznaczonego do kompresji do 20MB (dla uproszczenia gospodarki pamiêci¹)
		if(filesize > 20480000){
			cout << "Plik jest zbyt duzy! (Ograniczenie do 20MB)" << endl;
			srcFile.close();
			return 0;
		}
		
		char* buffer = new char[filesize];

		srcFile.read(buffer, filesize);
		srcFile.close();

		// przygotowanie alfabetu
		char* alphabet = new char[256];

		int alphabetSize = 0; 
		for (int i = 0; i < filesize; i++) {
			// czy jest ju¿ w s³owniku
			bool isAlready = false; 

			for (int j = 0; j < alphabetSize; j++) {
				if(alphabet[j] == buffer[i]) {
					isAlready = true;
					break;
				}
			}

			if(!isAlready) {
					alphabet[alphabetSize] = buffer[i]; 
					alphabetSize++;
				}
		}


		// przygotowanie danych dla w¹tków
		DWORD* threadIDArray = new DWORD[threadCount];
		HANDLE* handleArray = new HANDLE[threadCount];

		CompressParams* paramsArray = new CompressParams[threadCount];

		// rozmiar danych dla w¹tków (oprócz ostatniego)
		int dataSizePerThread = filesize / threadCount;
		// rozmiar danych dla ostatniego w¹tku (reszta danych)
		// ca³kowity rozmiar danych minus rozmiar danych dla poprzednich w¹tków
		int restDataSize = filesize - (dataSizePerThread * (threadCount - 1));

		for (int i = 0; i < threadCount; i++) {
			paramsArray[i].alphabet = alphabet;
			paramsArray[i].alphabetSize = alphabetSize;
			paramsArray[i].srcData = &(buffer[i * dataSizePerThread]);
			// maksymalnie dwukrotny przyrost danych (gdy kompresujemy pliki binarne zamiast tekstowych)
			paramsArray[i].compressedData = new char[50120000/threadCount];
			
			if(i == (threadCount - 1)) {
				// parametry dla ostatniego w¹tku
				paramsArray[i].srcDataSize = restDataSize;
			}
			else {
				// parametry dla wszystkich w¹tków oprócz ostatniego
				paramsArray[i].srcDataSize = dataSizePerThread;
			}
		}

		cout << "Kompresja danych. Prosze czekac..." << endl;

		// uruchomienie w¹tków
		unsigned int start = clock();
		for(int i = 0; i < threadCount; ++i) {
			handleArray[i] = CreateThread(NULL, 0, CompressThread, &paramsArray[i], 0, &threadIDArray[i]);
		}

		WaitForMultipleObjects(threadCount, handleArray, TRUE, INFINITE);
		unsigned int stop = clock();
		double time = static_cast<double>(stop - start) / CLOCKS_PER_SEC;

		for(int i = 0; i < threadCount; ++i) {
			CloseHandle(handleArray[i]);
		}

		// podliczenie sumarycznych danych skompresowanych oraz ka¿dego w¹tku
		int summaryCompressedDataSize = 0;
		int summaryBlockCount = 0;
		for (int i = 0; i < threadCount; i++) {
			summaryCompressedDataSize += paramsArray[i].compressedDataSize;
			summaryBlockCount += paramsArray[i].blockCount;
		}

		cout << "Czas dzialania: " << time << " sekund" << endl;

		cout << "Rozmiar danych po skompresowaniu: " << summaryCompressedDataSize * 2 << endl;

		fstream destFile;
		destFile.open(destFilename.c_str(), ios::binary | ios::out | ios::trunc);
		destFile.write((char*)(&summaryBlockCount), 2); 
		destFile.write((char*)(&alphabetSize), 2); 
		destFile.write(alphabet, alphabetSize);

		// zapis danych z poszczególnych w¹tków
		for (int i = 0; i < threadCount; i++) {
			// wyliczenie rozmiaru danych do zapisu
			// rozmiar danych (iloœæ kodów * 2 bajty) + 4 bajty na ka¿dy blok
			int destSize = (int)(paramsArray[i].compressedDataSize * 2 + paramsArray[i].blockCount * 4);
			destFile.write(paramsArray[i].compressedData, destSize);
		}

		destFile.close();

		// zwolnienie zasobów
		for (int i = 0; i < threadCount; i++) {
			delete paramsArray[i].compressedData;
		}
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
		srcFile.open(srcFilename.c_str(), ios::binary | ios::in); 

		if(!srcFile.is_open()) { 
			cout << "Nie mozna otworzyc pliku zrodlowego lub plik nie istnieje!";
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

		// przygotowanie alfabetu
		char* alphabet = new char[256]; 
		int alphabetSize = ((short*)(buffer + 2))[0]; 
		memcpy(alphabet, &(buffer[4]), alphabetSize);

		// przygotowanie parametrów i pamiêci dla procesu dekompresji
		DecompressParams params;
		params.compressedData = (buffer + 2 + 2 + alphabetSize); 
		// ograniczenie plików zdekompresowanych do 20MB
		params.decompressedData = new char[20480000]; 
		params.alphabet = alphabet;
		params.alphabetSize = alphabetSize;
		params.blockCount = ((short*)(buffer))[0]; 

		DecompressThread(&params);

		cout << "Czas dzialania: " << time << " sekund" << endl;
		cout << "Rozmiar danych zdekompresowanych: " << params.decompressedDataSize << endl;

		fstream destFile;
		destFile.open(destFilename.c_str(), ios::binary | ios::out | ios::trunc);
		destFile.write(params.decompressedData, params.decompressedDataSize);
		destFile.close();

		// zwolnienie zasobów
		delete params.decompressedData;
		delete buffer;
	}
	return 0;
}

/* Zwraca kod wybranego polecenia
-1 - nieznane polecenie
0 - w³asny komunikat funkcji
1 - kompresja
2 - dekompresja 
3 - kompresja asm*/
int parseCommand(int argc, char* argv[]) {
	if(argc >=6) return -1; // zbyt wiele argumentów

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


	// próba wczytania zdefiniowanej iloœci w¹tków
	if(argc == 5) {
		stringstream convert(argv[4]);
		convert >> threadCount;
	}

	/* argc = <3,4> */

	if(string(argv[1]) == "/c")
		return 1;

	if(string(argv[1]) == "/d")
		return 2;

	if(string(argv[1]) == "/ca")
		return 3;

	return -1;
}

void writeHelp() {
	cout << "Uzycie: JAMain opcja plik_zrodlowy [plik_docelowy] [liczba watkow]" << endl << endl;
	cout << "Opcje:" << endl;
	cout << "/c - kompresja pliku" << endl;
	cout << "/d - dekompresja pliku" << endl;
	cout << "/ca - kompresja pliku funkcja asemblera" << endl;
	cout << "/? - wyswietlenie tej pomocy" << endl;
	cout << "/help - wyswietlenie tej pomocy" << endl;
}

void writeUnknow() {
	cout << "Nieznane polecenie. Aby skorzystac z pomocy wpisz JAMain /? lub JAMain /help" << endl;
}