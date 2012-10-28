#include "LZW.h"

#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;

DWORD WINAPI CompressThread (LPVOID lpParameter) {
	int id = (int)GetCurrentThreadId();
	cout << "Jestem sobie watek" << endl << "ID watku: " << id << endl;

	CompressParams* pParams = (CompressParams*) lpParameter; // Uzyskanie struktury z parametrami.
	
	char* srcDataPointer = pParams->srcData; // wska�nik na dane do skompresowania
	char* compressedDataPointer = pParams->compressedData; // wska�nik na miejsce w pami�ci, gdzie b�dziemy umieszcza� skompresowane dane
	char* alphabetPointer = compressedDataPointer + 5; // alfabet zaczyna si� od 5. bajtu

	// analiza alfabetu
	char alphabetSize = 0; // ilo�� znak�w w alfabecie
	for (int i = 0; i < pParams->srcDataSize; i++) { // przegl�damy ca�e dane
		bool isAlready = false; // czy jest ju� w s�owniku

		for (int j = 0; j < alphabetSize; j++) { // p�tla por�wnuj�ca znak z p�tli wy�ej z ka�dym znakiem z dotychczasowego alfabetu
			if(alphabetPointer[j] == srcDataPointer[i]) {
				isAlready = true; // znak wyst�puje ju� w alfabecie
				break;
			}
		}

		if(!isAlready) {
				alphabetPointer[alphabetSize] = srcDataPointer[i]; // dodajemy nowy znak do alfabetu
				alphabetSize++;
			}
	}

	compressedDataPointer[4] = alphabetSize; // zapisanie na 4. bajt ilo�ci znak�w w alfabecie
	cout << "Rozmiar alfabetu: " << (int)alphabetSize << endl;

	// dodanie alfabetu do s�ownika
	Dictionary dict;
	for (int i = 0; i < alphabetSize; i++) {
		dict.addCodeword(&(alphabetPointer[i]), 1);
	}

	// w�a�ciwy algorytm kodowania (kompresji)
	short* compressedCodePointer = (short*)(compressedDataPointer + 5 + alphabetSize); // wska�nik na miejsce, od kt�rego b�dziemy umieszcza� zakodowane dane (po alfabecie)
	int compressedDataSize = 0; // rozmiar (ilo�� kod�w) skompresowanych danych
	short lastCode = 0; // kod ostatniego rozpoznanego s�owa kodowego
	short tmpCode = 0; // tymczasowy kod do analizy wyniku funkcji zwracaj�cej kod ze s�ownika
	short codewordLength = 0; // ilo�� aktualnie analizowanych znak�w od (char* c)
	char* c = srcDataPointer; // wska�nik na aktualnie analizowany ci�g znak�w

	codewordLength = 1; // pierwszy symbol wej�ciowy
	lastCode = dict.getCodewordId(c, 1); // kod pierwszego symbolu
	for (int i = 0; i < pParams->srcDataSize - 1; i++) { // dop�ki s� dane na wej�ciu
		codewordLength++; // wczytujemy kolejny znak (czyli zwi�kszamy ilo�� analizowanych znak�w)
		tmpCode = dict.getCodewordId(c, codewordLength); // pr�bujemy pobra� kod ci�gu
		if(tmpCode != -1) { // je�eli dany ci�g jest ju� w s�owniku to:
			lastCode = tmpCode; // aktualizujemy ostatnio rozpoznany kod i kontynuujemy wczytuj�c kolejny znak (pocz�tek p�tli)
		}
		else { // ci�gu nie ma s�owniku, wi�c:
			compressedCodePointer[compressedDataSize] = lastCode; // dodajemy do danych skompresowanych ostatnio rozpoznany kod
			compressedDataSize++; // zwi�kszamy rozmiar (ilo�� kod�w) skompresowanych danych
			dict.addCodeword(c, codewordLength); // dodajemy ci�g kt�rego nie ma w s�owniku
			// ostatni znak ostatnio nierozpoznanego ci�gu jest pierwszym znakiem nowego ci�gu
			c += (codewordLength - 1);
			codewordLength = 1;
			lastCode = dict.getCodewordId(c, 1); // aktualizacja lastCode
		}
	}
	compressedCodePointer[compressedDataSize] = lastCode; // dodajemy do danych skompresowanych ostatni kod
	compressedDataSize++; // zwi�kszamy rozmiar (ilo�� kod�w) skompresowanych danych

	pParams->compressedDataSize = compressedDataSize; // aktualizacja zwrotna
	((int*)compressedDataPointer)[0] = compressedDataSize; // pierwsze cztery bajty to rozmiar danych skompresowanych w kodach

	cout << "Ilosc slow kodowych slownika: " << dict.getCount() << endl;
	cout << "Rozmiar slownika: " << dict.getSize() << endl;
	
	return 0;
}

