#include "LZW.h"

#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;

DWORD WINAPI CompressThread (LPVOID lpParameter) {
	// Uzyskanie struktury z parametrami.
	CompressParams* pParams = (CompressParams*) lpParameter; 
	
	// wska�nik na dane do skompresowania
	char* srcDataPointer = pParams->srcData;
	// wska�nik na miejsce w pami�ci, gdzie b�dzie zaczyna� si� blok
	char* compressedDataPointer = pParams->compressedData; 
	// wska�nik na alfabet
	char* alphabetPointer = pParams->alphabet;
	// rozmiar alfabetu
	int alphabetSize = pParams->alphabetSize; 

	// dodanie alfabetu do s�ownika
	Dictionary* dict = new Dictionary;
	dict->initAlphabet(alphabetPointer, alphabetSize);
	
	// w�a�ciwy algorytm kodowania (kompresji)
	// wska�nik na miejsce, od kt�rego b�dziemy umieszcza� zakodowane dane (po 4 bajtach liczby kod�w) w danym bloku
	short* compressedCodePointer = (short*)(compressedDataPointer + 4); 
	// rozmiar (ilo�� kod�w) skompresowanych danych w bloku
	int compressedDataSize = 0;
	int summaryCompressedDataSize = 0;
	// kod ostatniego rozpoznanego s�owa kodowego
	int lastCode = 0;
	// tymczasowy kod do analizy wyniku funkcji zwracaj�cej kod ze s�ownika
	int tmpCode = 0; 
	// ilo�� aktualnie analizowanych znak�w od (char* c)
	short codewordLength = 0;
	// wska�nik na aktualnie analizowany ci�g znak�w
	char* c = srcDataPointer;
	// liczba blok�w danych
	unsigned short blockCount = 1; 

	// pierwszy symbol wej�ciowy
	codewordLength = 1; 
	// kod pierwszego symbolu
	lastCode = dict->getCodewordId(c, 1); 
	// dop�ki s� dane na wej�ciu
	for (int i = 0; i < pParams->srcDataSize - 1; i++) { 
		// wczytujemy kolejny znak (czyli zwi�kszamy ilo�� analizowanych znak�w)
		codewordLength++; 
		// pr�bujemy pobra� kod ci�gu
		tmpCode = dict->getCodewordId(c, codewordLength); 
		// je�eli dany ci�g jest ju� w s�owniku to:
		if(tmpCode != -1) { 
			// aktualizujemy ostatnio rozpoznany kod i kontynuujemy wczytuj�c kolejny znak (pocz�tek p�tli)
			lastCode = tmpCode; 
		}
		else { // ci�gu nie ma s�owniku, wi�c:
			// dodajemy do danych skompresowanych ostatnio rozpoznany kod
			compressedCodePointer[compressedDataSize] = lastCode; 
			// zwi�kszamy rozmiar (ilo�� kod�w) skompresowanych danych
			compressedDataSize++; 
			// dodajemy ci�g kt�rego nie ma w s�owniku
			dict->addCodeword(c, codewordLength); 
			// ostatni znak ostatnio nierozpoznanego ci�gu jest pierwszym znakiem nowego ci�gu
			c += (codewordLength - 1);
			codewordLength = 1;

			//sprawdzenie czy s�ownik osi�gn�� limit
			// ograniczenie rozmiaru b�d� ilo�ci wpis�w
			if(dict->getSize() >= MAX_DICT_DATA_SIZE || dict->getCount() >= 16384) { 
				summaryCompressedDataSize += compressedDataSize;
				// usuwamy dotychczasowy s�ownik
				delete dict; 
				// tworzymy nowy s�ownik
				dict = new Dictionary; 
				// inicjujemy go alfabetem
				dict->initAlphabet(alphabetPointer, alphabetSize); 
				
				// zapisujemy rozmiar (ilo�� kod�w) aktualnego bloku
				((int*)compressedDataPointer)[0] = compressedDataSize; 

				// przesuwamy compressedCodePointer w miejsce gdzie b�dzie si� zaczyna� nowy blok
				// (b�dziemy numerowa� kody od pocz�tku)
				compressedCodePointer += compressedDataSize;
				// zapami�tujemy miejsce rozpocz�cia bloku
				compressedDataPointer = (char*)compressedCodePointer; 
				// rezerwujemy 2 shorty (4 bajty) na rozmiar bloku
				compressedCodePointer += 2; 
				// numerujemy od pocz�tku
				compressedDataSize = 0; 

				// rozpoczynamy kolejny blok danych
				blockCount++; 
			}

			// aktualizacja lastCode
			lastCode = dict->getCodewordId(c, 1); 
		}
	}
	// dodajemy do danych skompresowanych ostatni kod
	compressedCodePointer[compressedDataSize] = lastCode; 
	// zwi�kszamy rozmiar (ilo�� kod�w) skompresowanych danych
	compressedDataSize++; 

	summaryCompressedDataSize += compressedDataSize;
	// aktualizacja zwrotna
	pParams->compressedDataSize = summaryCompressedDataSize; 
	// zapisanie rozmiaru danych skompresowanych dla ostatniego bloku
	((int*)compressedDataPointer)[0] = compressedDataSize; 
	pParams->blockCount = blockCount;
	
	return 0;
}

DWORD WINAPI DecompressThread (LPVOID lpParameter) {
	// Uzyskanie struktury z parametrami.
	DecompressParams* pParams = (DecompressParams*) lpParameter; 
	
	// wska�nik na skompresowane dane
	char* compressedDataPointer = pParams->compressedData; 
	// wska�nik na pami��, gdzie b�dziemy umieszcza� rozpakowane dane
	char* decompressedDataPointer = pParams->decompressedData; 
	// kopia wska�nika, na kt�rej b�dziemy operowa� (przesuwa� si� po pami�ci)
	char* out = decompressedDataPointer; 
	// liczba przechowuj�ca rozmiar danych zdekompresowanych
	int decompressedDataSize = 0; 

	// wczytanie alfabetu
	unsigned short blockCount = pParams->blockCount;
	// ilo�� znak�w w alfabecie
	int alphabetSize = pParams->alphabetSize; 
	// wska�nik na alfabet
	char* alphabetPointer = pParams->alphabet; 
	// wska�nik na pierwszy blok danych
	short* compressedCodePointer = (short*)(compressedDataPointer); 
	int blockSize = 0;

	for (int i = 0; i < blockCount; i++) {
		// odczytujemy ilo�� kod�w w bloku
		blockSize = ((int*)compressedCodePointer)[0]; 
		// przesuwamy si� o 4 bajty (2 shorty) - za 4 bajty ilo�ci kod�w, tam gdzie zaczynaj� si� kody kolejnego bloku
		compressedCodePointer += 2; 

		Dictionary dict;
		dict.initAlphabet(alphabetPointer, alphabetSize);

		// pk = pierwszy kod skompresowanych danych
		short pk = compressedCodePointer[0]; 
		// pobieramy element ze s�ownika o danym s�owie kodowym
		Element* tmp = dict.getElementById(pk); 

		// wypisujemy ci�g zwi�zany z danym kodem
		memcpy(out, tmp->value, tmp->size); 
		// przesuwamy wska�nik o odpowiedni� ilo�� znak�w do przodu
		out += tmp->size; 
		// zwi�kszamy ilo�� danych zdekompresowanych
		decompressedDataSize += tmp->size; 

		for (int i = 1; i < blockSize; i++) {
			// wczytujemy kolejny kod k
			short  k = compressedCodePointer[i]; 
			// ci�g skojarzony z poprzednim kodem
			Element* prev = tmp; 
			// pr�ba pobrania ciagu o id=k
			tmp = dict.getElementById(k); 
			if(tmp != NULL) { 
				// znaleziono kod
				// nowe s�owo kodowe to poprzednie s�owo kodowe + pierwszy znak kolejnego s�owa kodowego
				char* newCodeword = new char[prev->size + 1];	
				// poprzednie s�owo kodowe +
				memcpy(newCodeword, prev->value, prev->size);	
				// pierwszy znak kolejnego s�owa kodowego
				newCodeword[prev->size] = tmp->value[0];		
				// dodanie nowego s�owa kodowego do s�ownika
				dict.addCodeword(newCodeword, prev->size + 1);	
				// wypisujemy s�ownik[k] na wyj�cie
				memcpy(out, tmp->value, tmp->size);				
				// przesuwamy wska�nik o odpowiedni� ilo�� znak�w do przodu
				out += tmp->size;								
				// zwi�kszamy ilo�� danych zdekompresowanych
				decompressedDataSize += tmp->size;				
			}
			else {
				// w przeciwnym wypadku (je�li nie ma s�owa kodowego o takim kodzie w s�owniku)
				// nowe s�owo kodowe to poprzednie s�owo kodowe + pierwszy znak poprzedniego s�owa kodowego
				char* newCodeword = new char[prev->size + 1]; 
				// poprzednie s�owo kodowe +
				memcpy(newCodeword, prev->value, prev->size); 
				// pierwszy znak poprzedniego s�owa kodowego
				newCodeword[prev->size] = prev->value[0]; 
				// dodanie nowego s�owa kodowego do s�ownika
				dict.addCodeword(newCodeword, prev->size + 1); 
				// aby mo�na by�o zapami�ta� ci�g skojarzony z poprzednim kodem
				tmp = dict.getElementById(k); 
				// i to �wie�o dodane s�owo kodowe wypisujemy na wyj�cie
				memcpy(out, newCodeword, prev->size + 1); 
				// przesuwamy wska�nik o odpowiedni� ilo�� znak�w do przodu
				out += prev->size + 1; 
				// zwi�kszamy ilo�� danych zdekompresowanych
				decompressedDataSize += prev->size + 1; 
			}
		}

		// przesuwamy si� do nast�pnego bloku
		compressedCodePointer += blockSize; 
	}

	// zwracamy informacj� o rozmiarze danych zdekompresowanych
	pParams->decompressedDataSize = decompressedDataSize; 
	
	return 0;
}