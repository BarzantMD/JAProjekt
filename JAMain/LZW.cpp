#include "LZW.h"

#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;

DWORD WINAPI CompressThread (LPVOID lpParameter) {
	int id = (int)GetCurrentThreadId();
	cout << "Jestem sobie watek" << endl << "ID watku: " << id << endl;

	CompressParams* pParams = (CompressParams*) lpParameter; // Uzyskanie struktury z parametrami.
	
	char* srcDataPointer = pParams->srcData; // wskaŸnik na dane do skompresowania
	char* compressedDataPointer = pParams->compressedData; // wskaŸnik na miejsce w pamiêci, gdzie bêdziemy umieszczaæ skompresowane dane
	char* alphabetPointer = compressedDataPointer + 5; // alfabet zaczyna siê od 5. bajtu

	// analiza alfabetu
	char alphabetSize = 0; // iloœæ znaków w alfabecie
	for (int i = 0; i < pParams->srcDataSize; i++) { // przegl¹damy ca³e dane
		bool isAlready = false; // czy jest ju¿ w s³owniku

		for (int j = 0; j < alphabetSize; j++) { // pêtla porównuj¹ca znak z pêtli wy¿ej z ka¿dym znakiem z dotychczasowego alfabetu
			if(alphabetPointer[j] == srcDataPointer[i]) {
				isAlready = true; // znak wystêpuje ju¿ w alfabecie
				break;
			}
		}

		if(!isAlready) {
				alphabetPointer[alphabetSize] = srcDataPointer[i]; // dodajemy nowy znak do alfabetu
				alphabetSize++;
			}
	}

	compressedDataPointer[4] = alphabetSize; // zapisanie na 4. bajt iloœci znaków w alfabecie
	cout << "Rozmiar alfabetu: " << (int)alphabetSize << endl;

	// dodanie alfabetu do s³ownika
	Dictionary dict;
	for (int i = 0; i < alphabetSize; i++) {
		dict.addCodeword(&(alphabetPointer[i]), 1);
	}

	// w³aœciwy algorytm kodowania (kompresji)
	short* compressedCodePointer = (short*)(compressedDataPointer + 5 + alphabetSize); // wskaŸnik na miejsce, od którego bêdziemy umieszczaæ zakodowane dane (po alfabecie)
	int compressedDataSize = 0; // rozmiar (iloœæ kodów) skompresowanych danych
	short lastCode = 0; // kod ostatniego rozpoznanego s³owa kodowego
	short tmpCode = 0; // tymczasowy kod do analizy wyniku funkcji zwracaj¹cej kod ze s³ownika
	short codewordLength = 0; // iloœæ aktualnie analizowanych znaków od (char* c)
	char* c = srcDataPointer; // wskaŸnik na aktualnie analizowany ci¹g znaków

	codewordLength = 1; // pierwszy symbol wejœciowy
	lastCode = dict.getCodewordId(c, 1); // kod pierwszego symbolu
	for (int i = 0; i < pParams->srcDataSize - 1; i++) { // dopóki s¹ dane na wejœciu
		codewordLength++; // wczytujemy kolejny znak (czyli zwiêkszamy iloœæ analizowanych znaków)
		tmpCode = dict.getCodewordId(c, codewordLength); // próbujemy pobraæ kod ci¹gu
		if(tmpCode != -1) { // je¿eli dany ci¹g jest ju¿ w s³owniku to:
			lastCode = tmpCode; // aktualizujemy ostatnio rozpoznany kod i kontynuujemy wczytuj¹c kolejny znak (pocz¹tek pêtli)
		}
		else { // ci¹gu nie ma s³owniku, wiêc:
			compressedCodePointer[compressedDataSize] = lastCode; // dodajemy do danych skompresowanych ostatnio rozpoznany kod
			compressedDataSize++; // zwiêkszamy rozmiar (iloœæ kodów) skompresowanych danych
			dict.addCodeword(c, codewordLength); // dodajemy ci¹g którego nie ma w s³owniku
			// ostatni znak ostatnio nierozpoznanego ci¹gu jest pierwszym znakiem nowego ci¹gu
			c += (codewordLength - 1);
			codewordLength = 1;
			lastCode = dict.getCodewordId(c, 1); // aktualizacja lastCode
		}
	}
	compressedCodePointer[compressedDataSize] = lastCode; // dodajemy do danych skompresowanych ostatni kod
	compressedDataSize++; // zwiêkszamy rozmiar (iloœæ kodów) skompresowanych danych

	pParams->compressedDataSize = compressedDataSize; // aktualizacja zwrotna
	((int*)compressedDataPointer)[0] = compressedDataSize; // pierwsze cztery bajty to rozmiar danych skompresowanych w kodach

	cout << "Ilosc slow kodowych slownika: " << dict.getCount() << endl;
	cout << "Rozmiar slownika: " << dict.getSize() << endl;
	
	return 0;
}

