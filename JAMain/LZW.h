#ifndef LZW_H
#define LZW_H
#include <Windows.h>
#include <string>
#include <iostream>

#define MAX_DICT_DATA_SIZE 65536

using namespace std;

DWORD WINAPI CompressThread(LPVOID lpParameter);
DWORD WINAPI DecompressThread(LPVOID lpParameter);

// strukrua zawieraj�ca parametry dla w�tku kompresji
struct CompressParams {
	char* srcData; // dane do skompresowania
	int srcDataSize; // rozmiar danych srcData
	char* compressedData; // miejsce na skompresowane dane
	int compressedDataSize; // rozmiar danych skompresowanych (w s�owach kodowych, nie bajtach)
	unsigned short blockCount;
	char* alphabet; // wska�nik na alfabet
	int alphabetSize;  // liczba znak�w w alfabecie

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

// strukrua zawieraj�ca parametry dla w�tku dekompresji
struct DecompressParams {
	char* compressedData; // dane do dekompresji
	char* decompressedData; // miejsce na rozpakowane dane
	int decompressedDataSize; // rozmiar danych po dekompresji
	char* alphabet; // wska�nik na alfabet
	int alphabetSize;  // liczba znak�w w alfabecie
	unsigned short blockCount; // ilo�� blok�w w danych do dekompresji

	DecompressParams() :
		compressedData(NULL),
		decompressedData(NULL),
		decompressedDataSize(0),
		alphabet(NULL),
		alphabetSize(0),
		blockCount(0)
	{}
};

// pojedynczy element s�ownika (jedno s�owo kodowe, jako cz�� listy jednokierunkowej)
struct Element {
	char* value;
	int size; // ile bajt�w ma value

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

// klasa obs�uguj�ca s�ownik
class Dictionary {
private:
	// lista jednokierunkowa
	Element* head;
	Element* tail; 

	int count; // ilo�� s��w w s�owniku
	int size; // rozmiar s�ownika w bajtach


public:
	// inicjalizacja domy�lnymi warto�ciami
	Dictionary() : head(NULL), tail(NULL), count(0), size(0) {}

	// zwolnienie zasob�w
	~Dictionary() {
		Element* tmp = head;

		while(tmp != NULL) {
			Element* toDelete = tmp;
			tmp = tmp->next;
			delete toDelete;
		}
	}

	// inicjalizacja s�ownika alfabetem
	void initAlphabet(char* alphabetArray, int alphabetSize) {
		for (int i = 0; i < alphabetSize; i++) {
			addCodeword(&(alphabetArray[i]), 1);
		}
	}

	// dodawanie s��w kodowych do s�ownika
	int addCodeword(char* pWord, int length) {
		if(head == NULL) {
			head = new Element(pWord, length);

			tail = head;
			count++;
			size += length;
			// zwraca kod nowego s�owa w s�owniku
			return count - 1; 
		}

		tail->next = new Element(pWord, length);
		tail = tail->next;

		count++;
		size += length;
		// zwraca kod nowego s�owa w s�owniku
		return count - 1; 
	}

	// zwraca kod danego s�owa w s�owniku; je�li brak w s�owniku to zwraca -1
	int getCodewordId(char* pWord, int length) {
		Element* tmp = head;
		// domy�lnie nie znaleziono elementu
		int result = -1; 
		for (int i = 0; i < count; i++) {
			if(length != tmp->size) {
				// nie ma sensu por�wnywa� o ci�g�w o r�nych d�ugo�ciach
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