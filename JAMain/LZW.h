#ifndef LZW_H
#define LZW_H
#include <Windows.h>
#include <string>
#include <iostream>

#define MAX_DICT_DATA_SIZE 65536

using namespace std;

DWORD WINAPI CompressThread(LPVOID lpParameter);
DWORD WINAPI DecompressThread(LPVOID lpParameter);

// strukrua zawieraj¹ca parametry dla w¹tku kompresji
struct CompressParams {
	char* srcData; // dane do skompresowania
	int srcDataSize; // rozmiar danych srcData
	char* compressedData; // miejsce na skompresowane dane
	int compressedDataSize; // rozmiar danych skompresowanych (w s³owach kodowych, nie bajtach)
	unsigned short blockCount;
	char* alphabet; // wskaŸnik na alfabet
	int alphabetSize;  // liczba znaków w alfabecie

	CompressParams() :
		srcData(NULL),
		srcDataSize(0),
		compressedData(NULL),
		compressedDataSize(0),
		blockCount(0),
		alphabet(NULL),
		alphabetSize(0)
	{}
};

// strukrua zawieraj¹ca parametry dla w¹tku dekompresji
struct DecompressParams {
	char* compressedData; // dane do dekompresji
	char* decompressedData; // miejsce na rozpakowane dane
	int decompressedDataSize; // rozmiar danych po dekompresji
	char* alphabet; // wskaŸnik na alfabet
	int alphabetSize;  // liczba znaków w alfabecie
	unsigned short blockCount; // iloœæ bloków w danych do dekompresji

	DecompressParams() :
		compressedData(NULL),
		decompressedData(NULL),
		decompressedDataSize(0),
		alphabet(NULL),
		alphabetSize(0),
		blockCount(0)
	{}
};

// pojedynczy element s³ownika (jedno s³owo kodowe, jako czêœæ listy jednokierunkowej)
struct Element {
	char* value;
	int size; // ile bajtów ma value

	Element* next;

	Element(char* pWord, int length) {
		value = new char[length];
		memcpy(value, pWord, length);
		next = NULL;
		size = length;
	}

	~Element() {
		delete value;
	}
};

// klasa obs³uguj¹ca s³ownik
class Dictionary {
private:
	// lista jednokierunkowa
	Element* head;
	Element* tail; 

	int count; // iloœæ s³ów w s³owniku
	int size; // rozmiar s³ownika w bajtach


public:
	// inicjalizacja domyœlnymi wartoœciami
	Dictionary() : head(NULL), tail(NULL), count(0), size(0) {}

	// zwolnienie zasobów
	~Dictionary() {
		Element* tmp = head;

		while(tmp != NULL) {
			Element* toDelete = tmp;
			tmp = tmp->next;
			delete toDelete;
		}
	}

	// inicjalizacja s³ownika alfabetem
	void initAlphabet(char* alphabetArray, int alphabetSize) {
		for (int i = 0; i < alphabetSize; i++) {
			addCodeword(&(alphabetArray[i]), 1);
		}
	}

	// dodawanie s³ów kodowych do s³ownika
	int addCodeword(char* pWord, int length) {
		if(head == NULL) {
			head = new Element(pWord, length);

			tail = head;
			count++;
			size += length;
			// zwraca kod nowego s³owa w s³owniku
			return count - 1; 
		}

		tail->next = new Element(pWord, length);
		tail = tail->next;

		count++;
		size += length;
		// zwraca kod nowego s³owa w s³owniku
		return count - 1; 
	}

	// zwraca kod danego s³owa w s³owniku; jeœli brak w s³owniku to zwraca -1
	int getCodewordId(char* pWord, int length) {
		Element* tmp = head;
		// domyœlnie nie znaleziono elementu
		int result = -1; 
		for (int i = 0; i < count; i++) {
			if(length != tmp->size) {
				// nie ma sensu porównywaæ o ci¹gów o ró¿nych d³ugoœciach
				tmp = tmp->next;
				continue;
			}
			if(memcmp(pWord, tmp->value, length) == 0) {
				// znaleziono dopasowanie
				result = i; 
				break; 
			}
			tmp = tmp->next;
		}
		return result;
	}

	Element* getElementById(int id) {
		if(id >= count)
			return NULL;
		Element* tmp = head;
		for (int i = 0; i < id; i++) {
			tmp = tmp->next;
		}
		return tmp;
	}

	int getSize() {
		return size;
	}

	int getCount() {
		return count;
	}
};

#endif