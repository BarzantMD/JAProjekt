#include "LZW.h"

#include <Windows.h>
#include <iostream>
#include <string>

using namespace std;

// pojedynczy element s�ownika (jedno s�owo kodowe, jako cz�� listy jednokierunkowej)

struct CompressParams; // strukrua zawieraj�ca parametry dla w�tku
class Dictionary; // klasa obs�uguj�ca s�ownik

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

	int count; // ilo�� s��wk w s�owniku


public:
	// inicjalizacja domy�lnymi warto�ciami
	Dictionary() : head(NULL), tail(NULL), count(0) {}

	// dodawanie s��w kodowych do s�ownika
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

	// zwraca kod danego s�owa w s�owniku; je�li brak w s�owniku to zwraca -1
	int getCodewordId(string word) {
		Element* tmp = head;
		int result = -1; // domy�lnie nie znaleziono elementu
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