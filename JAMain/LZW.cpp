#include "LZW.h"

#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;

// pojedynczy element s³ownika (jedno s³owo kodowe, jako czêœæ listy jednokierunkowej)

struct CompressParams; // strukrua zawieraj¹ca parametry dla w¹tku
class Dictionary; // klasa obs³uguj¹ca s³ownik

DWORD WINAPI CompressThread (LPVOID lpParameter) {
	int id = (int)GetCurrentThreadId();
	cout << "Jestem sobie watek" << endl << "ID watku: " << id << endl;
	
	return 0;
}

struct CompressParams {
	char* srcData; // dane do skompresowania
	int dataSize; // rozmiar danych srcData
	char* compressedData; // miejsce na skompresowane dane
};

struct Element {
	string value;
	Element* next;

	Element(string word) {
		value = word;
		next = NULL;
	}
};

class Dictionary {
private:
	// lista jednokierunkowa
	Element* head;
	Element* tail; 

	int count; // iloœæ s³ówk w s³owniku


public:
	// inicjalizacja domyœlnymi wartoœciami
	Dictionary() : head(NULL), tail(NULL), count(0) {}

	// dodawanie s³ów kodowych do s³ownika
	int addCodeword(string word) {
		if(head == NULL) {
			head = new Element(word);
			tail = head;
			count++;
			return count - 1;
		}

		tail->next = new Element(word);
		tail = tail->next;
		count++;
	}

	// zwraca kod danego s³owa w s³owniku; jeœli brak w s³owniku to zwraca -1
	int getCodewordId(string word) {
		Element* tmp = head;
		int result = -1; // domyœlnie nie znaleziono elementu
		for (int i = 0; i < count; i++) {
			if(head->value == word) {
				result = i;
				break;
			}
		}
		return result;
	}


	Element* operator[](int index) {
		if(index >= count) return NULL;

		Element* tmp = head;
		for (int i = 0; i < index; i++) {
			head = head->next;
		}
		
		return head;
	}
};