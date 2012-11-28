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
	short* compressedCodePointer = (short*)(compressedDataPointer + 4); // wskaŸnik na miejsce, od którego bêdziemy umieszczaæ zakodowane dane (po 4 bajtach liczby kodów) w danym bloku
	int compressedDataSize = 0; // rozmiar (iloœæ kodów) skompresowanych danych w bloku
	int summaryCompressedDataSize = 0;
	int lastCode = 0; // kod ostatniego rozpoznanego s³owa kodowego
	int tmpCode = 0; // tymczasowy kod do analizy wyniku funkcji zwracaj¹cej kod ze s³ownika
	short codewordLength = 0; // iloœæ aktualnie analizowanych znaków od (char* c)
	char* c = srcDataPointer; // wskaŸnik na aktualnie analizowany ci¹g znaków
	unsigned short blockCount = 1; // liczba bloków danych

	codewordLength = 1; // pierwszy symbol wejœciowy
	lastCode = dict->getCodewordId(c, 1); // kod pierwszego symbolu
	for (int i = 0; i < pParams->srcDataSize - 1; i++) { // dopóki s¹ dane na wejœciu
		codewordLength++; // wczytujemy kolejny znak (czyli zwiêkszamy iloœæ analizowanych znaków)
		tmpCode = dict->getCodewordId(c, codewordLength); // próbujemy pobraæ kod ci¹gu
		if(tmpCode != -1) { // je¿eli dany ci¹g jest ju¿ w s³owniku to:
			lastCode = tmpCode; // aktualizujemy ostatnio rozpoznany kod i kontynuujemy wczytuj¹c kolejny znak (pocz¹tek pêtli)
		}
		else { // ci¹gu nie ma s³owniku, wiêc:
			compressedCodePointer[compressedDataSize] = lastCode; // dodajemy do danych skompresowanych ostatnio rozpoznany kod
			compressedDataSize++; // zwiêkszamy rozmiar (iloœæ kodów) skompresowanych danych
			dict->addCodeword(c, codewordLength); // dodajemy ci¹g którego nie ma w s³owniku
			// ostatni znak ostatnio nierozpoznanego ci¹gu jest pierwszym znakiem nowego ci¹gu
			c += (codewordLength - 1);
			codewordLength = 1;

			//sprawdzenie czy s³ownik osi¹gn¹³ limit
			if(dict->getSize() >= MAX_DICT_DATA_SIZE || dict->getCount() >= 16384) { // ograniczenie rozmiaru b¹dŸ iloœci wpisów
				cout << "Rozmiar bloku: " << compressedDataSize << endl;

				summaryCompressedDataSize += compressedDataSize;
				cout << "Przetworzonych danych: " << summaryCompressedDataSize * 2 << "B" << endl;
				cout << "Liczba wpisow w slowniku: " << dict->getCount() << endl;
				cout << "Tworzenie nowego slownika" << endl;
				delete dict; // usuwamy dotychczasowy s³ownik
				dict = new Dictionary; // tworzymy nowy s³ownik
				dict->initAlphabet(alphabetPointer, alphabetSize); // inicjujemy go alfabetem
				
				((int*)compressedDataPointer)[0] = compressedDataSize; // zapisujemy rozmiar (iloœæ kodów) aktualnego bloku

				// przesuwamy compressedCodePointer w miejsce gdzie bêdzie siê zaczyna³ nowy blok
				// (bêdziemy numerowaæ kody od pocz¹tku)
				compressedCodePointer += compressedDataSize;
				compressedDataPointer = (char*)compressedCodePointer; // zapamiêtujemy miejsce rozpoczêcia bloku
				compressedCodePointer += 2; // rezerwujemy 2 shorty (4 bajty) na rozmiar bloku
				compressedDataSize = 0; // numerujemy od pocz¹tku

				blockCount++; // rozpoczynamy kolejny blok danych
			}

			lastCode = dict->getCodewordId(c, 1); // aktualizacja lastCode
		}
	}
	compressedCodePointer[compressedDataSize] = lastCode; // dodajemy do danych skompresowanych ostatni kod
	compressedDataSize++; // zwiêkszamy rozmiar (iloœæ kodów) skompresowanych danych

	cout << "Rozmiar bloku: " << compressedDataSize << endl;

	summaryCompressedDataSize += compressedDataSize;
	pParams->compressedDataSize = summaryCompressedDataSize; // aktualizacja zwrotna
	((int*)compressedDataPointer)[0] = compressedDataSize; // zapisanie rozmiaru danych skompresowanych dla ostatniego bloku
	pParams->blockCount = blockCount;
	
	return 0;
}

DWORD WINAPI DecompressThread (LPVOID lpParameter) {
	DecompressParams* pParams = (DecompressParams*) lpParameter; // Uzyskanie struktury z parametrami.
	
	char* compressedDataPointer = pParams->compressedData; // wskaŸnik na skompresowane dane
	char* decompressedDataPointer = pParams->decompressedData; // wskaŸnik na pamiêæ, gdzie bêdziemy umieszczaæ rozpakowane dane
	char* out = decompressedDataPointer; // kopia wskaŸnika, na której bêdziemy operowaæ (przesuwaæ siê po pamiêci)
	int decompressedDataSize = 0; // liczba przechowuj¹ca rozmiar danych zdekompresowanych

	// wczytanie alfabetu
	//unsigned short blockCount = ((short*)compressedDataPointer)[0]; // odczytujemy iloœæ bloków
	unsigned short blockCount = pParams->blockCount;
	//compressedDataPointer += 2; // przesuwamy siê za iloœæ bloków (dwa bajty w przód - jeden short)
	//int alphabetSize = compressedDataPointer[4]; // iloœæ znaków w alfabecie (zapisana na 4. bajcie numeruj¹c od 0)
	//char* alphabetPointer = compressedDataPointer + 5; // alfabet zaczyna siê od 5. bajtu numeruj¹c od 0
	int alphabetSize = pParams->alphabetSize; // iloœæ znaków w alfabecie
	char* alphabetPointer = pParams->alphabet; // wskaŸnik na alfabet
	//short* compressedCodePointer = (short*)(alphabetPointer + alphabetSize); // wskaŸnik na kody skompresowanych danych pierwszego bloku
	//short* compressedCodePointer = (short*)(compressedDataPointer + 4); // wskaŸnik na kody skompresowanych danych 1. bloku (za rozmiarem bloku)
	short* compressedCodePointer = (short*)(compressedDataPointer); // wskaŸnik na pierwszy blok danych
	//int blockSize = ((int*)compressedDataPointer)[0]; // odczytujemy iloœæ kodów w pierwszym bloku (pierwsze 4 bajty)
	int blockSize = 0;

	cout << "Ilosc blokow: " << blockCount << endl;

	for (int i = 0; i < blockCount; i++) {

		blockSize = ((int*)compressedCodePointer)[0]; // odczytujemy iloœæ kodów w bloku
		compressedCodePointer += 2; // przesuwamy siê o 4 bajty (2 shorty) - za 4 bajty iloœci kodów, tam gdzie zaczynaj¹ siê kody kolejnego bloku
		cout << "Rpzmiar bloku: " << blockSize <<endl;

		Dictionary dict;
		dict.initAlphabet(alphabetPointer, alphabetSize);

		short pk = compressedCodePointer[0]; // pk = pierwszy kod skompresowanych danych
		Element* tmp = dict.getElementById(pk); // pobieramy element ze s³ownika o danym s³owie kodowym

		memcpy(out, tmp->value, tmp->size); // wypisujemy ci¹g zwi¹zany z danym kodem
		out += tmp->size; // przesuwamy wskaŸnik o odpowiedni¹ iloœæ znaków do przodu
		decompressedDataSize += tmp->size; // zwiêkszamy iloœæ danych zdekompresowanych

		for (int i = 1; i < blockSize; i++) {
			short  k = compressedCodePointer[i]; // wczytujemy kolejny kod k
			Element* prev = tmp; // ci¹g skojarzony z poprzednim kodem
			tmp = dict.getElementById(k); // próba pobrania ciagu o id=k
			if(tmp != NULL) { // znaleziono kod
				char* newCodeword = new char[prev->size + 1];	// nowe s³owo kodowe to poprzednie s³owo kodowe + pierwszy znak kolejnego s³owa kodowego
				memcpy(newCodeword, prev->value, prev->size);	// poprzednie s³owo kodowe +
				newCodeword[prev->size] = tmp->value[0];		// pierwszy znak kolejnego s³owa kodowego
				dict.addCodeword(newCodeword, prev->size + 1);	// dodanie nowego s³owa kodowego do s³ownika
				memcpy(out, tmp->value, tmp->size);				// wypisujemy s³ownik[k] na wyjœcie
				out += tmp->size;								// przesuwamy wskaŸnik o odpowiedni¹ iloœæ znaków do przodu
				decompressedDataSize += tmp->size;				// zwiêkszamy iloœæ danych zdekompresowanych
			}
			else { // w przeciwnym wypadku (jeœli nie ma s³owa kodowego o takim kodzie w s³owniku)
				char* newCodeword = new char[prev->size + 1]; // nowe s³owo kodowe to poprzednie s³owo kodowe + pierwszy znak poprzedniego s³owa kodowego
				memcpy(newCodeword, prev->value, prev->size); // poprzednie s³owo kodowe +
				newCodeword[prev->size] = prev->value[0]; // pierwszy znak poprzedniego s³owa kodowego
				dict.addCodeword(newCodeword, prev->size + 1); // dodanie nowego s³owa kodowego do s³ownika
				tmp = dict.getElementById(k); // aby mo¿na by³o zapamiêtaæ ci¹g skojarzony z poprzednim kodem
				memcpy(out, newCodeword, prev->size + 1); // i to œwie¿o dodane s³owo kodowe wypisujemy na wyjœcie
				out += prev->size + 1; // przesuwamy wskaŸnik o odpowiedni¹ iloœæ znaków do przodu
				decompressedDataSize += prev->size + 1; // zwiêkszamy iloœæ danych zdekompresowanych
			}
		}

		compressedCodePointer += blockSize; // przesuwamy siê do nastêpnego bloku
	}

	pParams->decompressedDataSize = decompressedDataSize; // zwracamy informacjê o rozmiarze danych zdekompresowanych
	
	return 0;
}