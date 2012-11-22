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
	int dictSize; // rozmiar s³ownika
	char* alphabet; // wskaŸnik na alfabet
	int alphabetSize;  // liczba znaków w alfabecie

	CompressParams() :
		srcData(NULL),
		srcDataSize(0),
		compressedData(NULL),
		compressedDataSize(0),
		blockCount(0),
		dictSize(0),
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

	//DEBUG
	int number; // numer s³owa kodowegos

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
		//if(head == NULL) return;
		Element* tmp = head;
		/*while(tmp->next != NULL) {
			Element* tmp_prev = tmp;
			tmp = tmp->next;
			delete tmp_prev;
		}*/
		//delete tmp;

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

			//DEBUG
			head->number = count;

			tail = head;
			count++;
			size += length;
			return count - 1; // zwraca kod nowego s³owa w s³owniku
		}

		tail->next = new Element(pWord, length);
		tail = tail->next;

		//DEBUG
		tail->number = count;

		count++;
		size += length;
		return count - 1; // zwraca kod nowego s³owa w s³owniku
	}

	// zwraca kod danego s³owa w s³owniku; jeœli brak w s³owniku to zwraca -1
	int getCodewordId(char* pWord, int length) {
		Element* tmp = head;
		int result = -1; // domyœlnie nie znaleziono elementu
		for (int i = 0; i < count; i++) {
			if(length != tmp->size) {// nie ma sensu porównywaæ o ci¹gów o ró¿nych d³ugoœciach
				tmp = tmp->next;
				continue;
			}
			if(memcmp(pWord, tmp->value, length) == 0) {
				result = i; // znaleziono dopasowanie
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

	Element* operator[](int index) {
		if(index >= count) return NULL;

		Element* tmp = head;
		for (int i = 0; i < index; i++) {
			head = head->next;
		}
		
		return head;
	}

	void printDebugInfo() {
		cout << "Count: " << count << endl;
		cout << "Size: " << size << endl;
		cout << "Words: " << endl;
		Element* tmp = head;
		for(int i = 0; i < count; i++) {
			cout << i << ": " << tmp->value << endl;
			tmp = tmp->next;
		}
	}
};

#endif