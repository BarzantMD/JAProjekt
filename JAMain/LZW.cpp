#include "LZW.h"

#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;

DWORD WINAPI CompressThread (LPVOID lpParameter) {
	CompressParams* pParams = (CompressParams*) lpParameter; // Uzyskanie struktury z parametrami.
	
	char* srcDataPointer = pParams->srcData; // wska�nik na dane do skompresowania
	char* compressedDataPointer = pParams->compressedData; // wska�nik na miejsce w pami�ci, gdzie b�dzie zaczyna� si� blok
	char* alphabetPointer = compressedDataPointer + 5; // alfabet zaczyna si� od 5. bajtu

	// analiza alfabetu
	unsigned char alphabetSize = 0; // ilo�� znak�w w alfabecie
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
	Dictionary* dict = new Dictionary;
	dict->initAlphabet(alphabetPointer, alphabetSize);
	
	// w�a�ciwy algorytm kodowania (kompresji)
	short* compressedCodePointer = (short*)(compressedDataPointer + 5 + alphabetSize); // wska�nik na miejsce, od kt�rego b�dziemy umieszcza� zakodowane dane (po alfabecie) w danym bloku
	int compressedDataSize = 0; // rozmiar (ilo�� kod�w) skompresowanych danych w bloku
	int summaryCompressedDataSize = 0;
	int lastCode = 0; // kod ostatniego rozpoznanego s�owa kodowego
	int tmpCode = 0; // tymczasowy kod do analizy wyniku funkcji zwracaj�cej kod ze s�ownika
	short codewordLength = 0; // ilo�� aktualnie analizowanych znak�w od (char* c)
	char* c = srcDataPointer; // wska�nik na aktualnie analizowany ci�g znak�w
	unsigned short blockCount = 1; // liczba blok�w danych

	codewordLength = 1; // pierwszy symbol wej�ciowy
	lastCode = dict->getCodewordId(c, 1); // kod pierwszego symbolu
	for (int i = 0; i < pParams->srcDataSize - 1; i++) { // dop�ki s� dane na wej�ciu
		codewordLength++; // wczytujemy kolejny znak (czyli zwi�kszamy ilo�� analizowanych znak�w)
		tmpCode = dict->getCodewordId(c, codewordLength); // pr�bujemy pobra� kod ci�gu
		if(tmpCode != -1) { // je�eli dany ci�g jest ju� w s�owniku to:
			lastCode = tmpCode; // aktualizujemy ostatnio rozpoznany kod i kontynuujemy wczytuj�c kolejny znak (pocz�tek p�tli)
		}
		else { // ci�gu nie ma s�owniku, wi�c:
			compressedCodePointer[compressedDataSize] = lastCode; // dodajemy do danych skompresowanych ostatnio rozpoznany kod
			compressedDataSize++; // zwi�kszamy rozmiar (ilo�� kod�w) skompresowanych danych
			dict->addCodeword(c, codewordLength); // dodajemy ci�g kt�rego nie ma w s�owniku
			// ostatni znak ostatnio nierozpoznanego ci�gu jest pierwszym znakiem nowego ci�gu
			c += (codewordLength - 1);
			codewordLength = 1;

			//sprawdzenie czy s�ownik osi�gn�� limit
			if(dict->getSize() >= MAX_DICT_DATA_SIZE || dict->getCount() >= 65536) { // ograniczenie rozmiaru b�d� ilo�ci wpis�w
				cout << "Rozmiar bloku: " << compressedDataSize << endl;

				summaryCompressedDataSize += compressedDataSize;
				cout << "Przetworzonych danych: " << summaryCompressedDataSize * 2 << "B" << endl;
				cout << "Tworzenie nowego slownika" << endl;
				delete dict; // usuwamy dotychczasowy s�ownik
				dict = new Dictionary; // tworzymy nowy s�ownik
				dict->initAlphabet(alphabetPointer, alphabetSize); // inicjujemy go alfabetem
				
				((int*)compressedDataPointer)[0] = compressedDataSize; // zapisujemy rozmiar (ilo�� kod�w) aktualnego bloku

				// przesuwamy compressedCodePointer w miejsce gdzie b�dzie si� zaczyna� nowy blok
				// (b�dziemy numerowa� kody od pocz�tku)
				compressedCodePointer += compressedDataSize;
				compressedDataPointer = (char*)compressedCodePointer; // zapami�tujemy miejsce rozpocz�cia bloku
				compressedCodePointer += 2; // rezerwujemy 2 shorty (4 bajty) na rozmiar bloku
				compressedDataSize = 0; // numerujemy od pocz�tku

				blockCount++; // rozpoczynamy kolejny blok danych
			}

			lastCode = dict->getCodewordId(c, 1); // aktualizacja lastCode
		}
	}
	compressedCodePointer[compressedDataSize] = lastCode; // dodajemy do danych skompresowanych ostatni kod
	compressedDataSize++; // zwi�kszamy rozmiar (ilo�� kod�w) skompresowanych danych

	cout << "Rozmiar bloku: " << compressedDataSize << endl;

	summaryCompressedDataSize += compressedDataSize;
	pParams->compressedDataSize = summaryCompressedDataSize; // aktualizacja zwrotna
	((int*)compressedDataPointer)[0] = compressedDataSize; // zapisanie rozmiaru danych skompresowanych dla ostatniego bloku
	pParams->blockCount = blockCount;
	
	return 0;
}

DWORD WINAPI DecompressThread (LPVOID lpParameter) {
	DecompressParams* pParams = (DecompressParams*) lpParameter; // Uzyskanie struktury z parametrami.
	
	char* compressedDataPointer = pParams->compressedData; // wska�nik na skompresowane dane
	int compressedDataSize = pParams->compressedDataSize; // rozmiar danych skompresowanych
	char* decompressedDataPointer = pParams->decompressedData; // wska�nik na pami��, gdzie b�dziemy umieszcza� rozpakowane dane
	char* out = decompressedDataPointer; // kopia wska�nika, na kt�rej b�dziemy operowa� (przesuwa� si� po pami�ci)
	int decompressedDataSize = 0; // liczba przechowuj�ca rozmiar danych zdekompresowanych

	// wczytanie alfabetu
	unsigned short blockCount = ((short*)compressedDataPointer)[0]; // odczytujemy ilo�� blok�w
	compressedDataPointer += 2; // przesuwamy si� za ilo�� blok�w (dwa bajty w prz�d - jeden short)
	unsigned char alphabetSize = compressedDataPointer[4]; // ilo�� znak�w w alfabecie (zapisana na 4. bajcie numeruj�c od 0)
	char* alphabetPointer = compressedDataPointer + 5; // alfabet zaczyna si� od 5. bajtu numeruj�c od 0
	short* compressedCodePointer = (short*)(alphabetPointer + alphabetSize); // wska�nik na kody skompresowanych danych pierwszego bloku
	int blockSize = ((int*)compressedDataPointer)[0]; // odczytujemy ilo�� kod�w w pierwszym bloku (pierwsze 4 bajty)

	cout << "Ilosc blokow: " << blockCount << endl;

	for (int i = 0; i < blockCount; i++) {
		cout << "Rpzmiar bloku: " << blockSize <<endl;

		Dictionary dict;
		dict.initAlphabet(alphabetPointer, alphabetSize);

		short pk = compressedCodePointer[0]; // pk = pierwszy kod skompresowanych danych
		Element* tmp = dict.getElementById(pk); // pobieramy element ze s�ownika o danym s�owie kodowym

		memcpy(out, tmp->value, tmp->size); // wypisujemy ci�g zwi�zany z danym kodem
		out += tmp->size; // przesuwamy wska�nik o odpowiedni� ilo�� znak�w do przodu
		decompressedDataSize += tmp->size; // zwi�kszamy ilo�� danych zdekompresowanych

		for (int i = 1; i < blockSize; i++) {
			short  k = compressedCodePointer[i]; // wczytujemy kolejny kod k
			Element* prev = tmp; // ci�g skojarzony z poprzednim kodem
			tmp = dict.getElementById(k); // pr�ba pobrania ciagu o id=k
			if(tmp != NULL) { // znaleziono kod
				char* newCodeword = new char[prev->size + 1]; // nowe s�owo kodowe to poprzednie s�owo kodowe + pierwszy znak kolejnego s�owa kodowego
				memcpy(newCodeword, prev->value, prev->size); // poprzednie s�owo kodowe +
				newCodeword[prev->size] = tmp->value[0]; // pierwszy znak kolejnego s�owa kodowego
				dict.addCodeword(newCodeword, prev->size + 1); // dodanie nowego s�owa kodowego do s�ownika
				memcpy(out, tmp->value, tmp->size); // wypisujemy s�ownik[k] na wyj�cie
				out += tmp->size; // przesuwamy wska�nik o odpowiedni� ilo�� znak�w do przodu
				decompressedDataSize += tmp->size; // zwi�kszamy ilo�� danych zdekompresowanych
			}
			else { // w przeciwnym wypadku (je�li nie ma s�owa kodowego o takim kodzie w s�owniku)
				char* newCodeword = new char[prev->size + 1]; // nowe s�owo kodowe to poprzednie s�owo kodowe + pierwszy znak poprzedniego s�owa kodowego
				memcpy(newCodeword, prev->value, prev->size); // poprzednie s�owo kodowe +
				newCodeword[prev->size] = prev->value[0]; // pierwszy znak poprzedniego s�owa kodowego
				dict.addCodeword(newCodeword, prev->size + 1); // dodanie nowego s�owa kodowego do s�ownika
				tmp = dict.getElementById(k); // aby mo�na by�o zapami�ta� ci�g skojarzony z poprzednim kodem
				memcpy(out, newCodeword, prev->size + 1); // i to �wie�o dodane s�owo kodowe wypisujemy na wyj�cie
				out += prev->size + 1; // przesuwamy wska�nik o odpowiedni� ilo�� znak�w do przodu
				decompressedDataSize += prev->size + 1; // zwi�kszamy ilo�� danych zdekompresowanych
			}
		}

		compressedCodePointer += blockSize; // przesuwamy si� do nast�pnego bloku
		blockSize = ((int*)compressedCodePointer)[0]; // odczytujemy ilo�� kod�w w kolejnym bloku
		compressedCodePointer += 2; // przesuwamy si� o 4 bajty (2 shorty) - za 4 bajty ilo�ci kod�w, tam gdzie zaczynaj� si� kody kolejnego bloku


	}

	pParams->decompressedDataSize = decompressedDataSize; // zwracamy informacj� o rozmiarze danych zdekompresowanych
	
	return 0;
}