#include "LZW.h"

#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;

DWORD WINAPI CompressThread (LPVOID lpParameter) {
	// Uzyskanie struktury z parametrami.
	CompressParams* pParams = (CompressParams*) lpParameter; 
	
	// wskaŸnik na dane do skompresowania
	char* srcDataPointer = pParams->srcData;
	// wskaŸnik na miejsce w pamiêci, gdzie bêdzie zaczynaæ siê blok
	char* compressedDataPointer = pParams->compressedData; 
	// wskaŸnik na alfabet
	char* alphabetPointer = pParams->alphabet;
	// rozmiar alfabetu
	int alphabetSize = pParams->alphabetSize; 

	// dodanie alfabetu do s³ownika
	Dictionary* dict = new Dictionary;
	dict->initAlphabet(alphabetPointer, alphabetSize);
	
	// w³aœciwy algorytm kodowania (kompresji)
	// wskaŸnik na miejsce, od którego bêdziemy umieszczaæ zakodowane dane (po 4 bajtach liczby kodów) w danym bloku
	short* compressedCodePointer = (short*)(compressedDataPointer + 4); 
	// rozmiar (iloœæ kodów) skompresowanych danych w bloku
	int compressedDataSize = 0;
	int summaryCompressedDataSize = 0;
	// kod ostatniego rozpoznanego s³owa kodowego
	int lastCode = 0;
	// tymczasowy kod do analizy wyniku funkcji zwracaj¹cej kod ze s³ownika
	int tmpCode = 0; 
	// iloœæ aktualnie analizowanych znaków od (char* c)
	short codewordLength = 0;
	// wskaŸnik na aktualnie analizowany ci¹g znaków
	char* c = srcDataPointer;
	// liczba bloków danych
	unsigned short blockCount = 1; 

	// pierwszy symbol wejœciowy
	codewordLength = 1; 
	// kod pierwszego symbolu
	lastCode = dict->getCodewordId(c, 1); 
	// dopóki s¹ dane na wejœciu
	for (int i = 0; i < pParams->srcDataSize - 1; i++) { 
		// wczytujemy kolejny znak (czyli zwiêkszamy iloœæ analizowanych znaków)
		codewordLength++; 
		// próbujemy pobraæ kod ci¹gu
		tmpCode = dict->getCodewordId(c, codewordLength); 
		// je¿eli dany ci¹g jest ju¿ w s³owniku to:
		if(tmpCode != -1) { 
			// aktualizujemy ostatnio rozpoznany kod i kontynuujemy wczytuj¹c kolejny znak (pocz¹tek pêtli)
			lastCode = tmpCode; 
		}
		else { // ci¹gu nie ma s³owniku, wiêc:
			// dodajemy do danych skompresowanych ostatnio rozpoznany kod
			compressedCodePointer[compressedDataSize] = lastCode; 
			// zwiêkszamy rozmiar (iloœæ kodów) skompresowanych danych
			compressedDataSize++; 
			// dodajemy ci¹g którego nie ma w s³owniku
			dict->addCodeword(c, codewordLength); 
			// ostatni znak ostatnio nierozpoznanego ci¹gu jest pierwszym znakiem nowego ci¹gu
			c += (codewordLength - 1);
			codewordLength = 1;

			//sprawdzenie czy s³ownik osi¹gn¹³ limit
			// ograniczenie rozmiaru b¹dŸ iloœci wpisów
			if(dict->getSize() >= MAX_DICT_DATA_SIZE || dict->getCount() >= 16384) { 
				summaryCompressedDataSize += compressedDataSize;
				// usuwamy dotychczasowy s³ownik
				delete dict; 
				// tworzymy nowy s³ownik
				dict = new Dictionary; 
				// inicjujemy go alfabetem
				dict->initAlphabet(alphabetPointer, alphabetSize); 
				
				// zapisujemy rozmiar (iloœæ kodów) aktualnego bloku
				((int*)compressedDataPointer)[0] = compressedDataSize; 

				// przesuwamy compressedCodePointer w miejsce gdzie bêdzie siê zaczyna³ nowy blok
				// (bêdziemy numerowaæ kody od pocz¹tku)
				compressedCodePointer += compressedDataSize;
				// zapamiêtujemy miejsce rozpoczêcia bloku
				compressedDataPointer = (char*)compressedCodePointer; 
				// rezerwujemy 2 shorty (4 bajty) na rozmiar bloku
				compressedCodePointer += 2; 
				// numerujemy od pocz¹tku
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
	// zwiêkszamy rozmiar (iloœæ kodów) skompresowanych danych
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
	
	// wskaŸnik na skompresowane dane
	char* compressedDataPointer = pParams->compressedData; 
	// wskaŸnik na pamiêæ, gdzie bêdziemy umieszczaæ rozpakowane dane
	char* decompressedDataPointer = pParams->decompressedData; 
	// kopia wskaŸnika, na której bêdziemy operowaæ (przesuwaæ siê po pamiêci)
	char* out = decompressedDataPointer; 
	// liczba przechowuj¹ca rozmiar danych zdekompresowanych
	int decompressedDataSize = 0; 

	// wczytanie alfabetu
	unsigned short blockCount = pParams->blockCount;
	// iloœæ znaków w alfabecie
	int alphabetSize = pParams->alphabetSize; 
	// wskaŸnik na alfabet
	char* alphabetPointer = pParams->alphabet; 
	// wskaŸnik na pierwszy blok danych
	short* compressedCodePointer = (short*)(compressedDataPointer); 
	int blockSize = 0;

	for (int i = 0; i < blockCount; i++) {
		// odczytujemy iloœæ kodów w bloku
		blockSize = ((int*)compressedCodePointer)[0]; 
		// przesuwamy siê o 4 bajty (2 shorty) - za 4 bajty iloœci kodów, tam gdzie zaczynaj¹ siê kody kolejnego bloku
		compressedCodePointer += 2; 

		Dictionary dict;
		dict.initAlphabet(alphabetPointer, alphabetSize);

		// pk = pierwszy kod skompresowanych danych
		short pk = compressedCodePointer[0]; 
		// pobieramy element ze s³ownika o danym s³owie kodowym
		Element* tmp = dict.getElementById(pk); 

		// wypisujemy ci¹g zwi¹zany z danym kodem
		memcpy(out, tmp->value, tmp->size); 
		// przesuwamy wskaŸnik o odpowiedni¹ iloœæ znaków do przodu
		out += tmp->size; 
		// zwiêkszamy iloœæ danych zdekompresowanych
		decompressedDataSize += tmp->size; 

		for (int i = 1; i < blockSize; i++) {
			// wczytujemy kolejny kod k
			short  k = compressedCodePointer[i]; 
			// ci¹g skojarzony z poprzednim kodem
			Element* prev = tmp; 
			// próba pobrania ciagu o id=k
			tmp = dict.getElementById(k); 
			if(tmp != NULL) { 
				// znaleziono kod
				// nowe s³owo kodowe to poprzednie s³owo kodowe + pierwszy znak kolejnego s³owa kodowego
				char* newCodeword = new char[prev->size + 1];	
				// poprzednie s³owo kodowe +
				memcpy(newCodeword, prev->value, prev->size);	
				// pierwszy znak kolejnego s³owa kodowego
				newCodeword[prev->size] = tmp->value[0];		
				// dodanie nowego s³owa kodowego do s³ownika
				dict.addCodeword(newCodeword, prev->size + 1);	
				// wypisujemy s³ownik[k] na wyjœcie
				memcpy(out, tmp->value, tmp->size);				
				// przesuwamy wskaŸnik o odpowiedni¹ iloœæ znaków do przodu
				out += tmp->size;								
				// zwiêkszamy iloœæ danych zdekompresowanych
				decompressedDataSize += tmp->size;				
			}
			else {
				// w przeciwnym wypadku (jeœli nie ma s³owa kodowego o takim kodzie w s³owniku)
				// nowe s³owo kodowe to poprzednie s³owo kodowe + pierwszy znak poprzedniego s³owa kodowego
				char* newCodeword = new char[prev->size + 1]; 
				// poprzednie s³owo kodowe +
				memcpy(newCodeword, prev->value, prev->size); 
				// pierwszy znak poprzedniego s³owa kodowego
				newCodeword[prev->size] = prev->value[0]; 
				// dodanie nowego s³owa kodowego do s³ownika
				dict.addCodeword(newCodeword, prev->size + 1); 
				// aby mo¿na by³o zapamiêtaæ ci¹g skojarzony z poprzednim kodem
				tmp = dict.getElementById(k); 
				// i to œwie¿o dodane s³owo kodowe wypisujemy na wyjœcie
				memcpy(out, newCodeword, prev->size + 1); 
				// przesuwamy wskaŸnik o odpowiedni¹ iloœæ znaków do przodu
				out += prev->size + 1; 
				// zwiêkszamy iloœæ danych zdekompresowanych
				decompressedDataSize += prev->size + 1; 
			}
		}

		// przesuwamy siê do nastêpnego bloku
		compressedCodePointer += blockSize; 
	}

	// zwracamy informacjê o rozmiarze danych zdekompresowanych
	pParams->decompressedDataSize = decompressedDataSize; 
	
	return 0;
}