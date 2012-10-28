#ifndef MAIN_H
#define MAIN_H

#include <iostream>

using namespace std;


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
		if(head == NULL) return;
		Element* tmp = head;
		while(tmp->next != NULL) {
			Element* tmp_prev = tmp;
			tmp = tmp->next;
			delete tmp_prev;
		}
		delete tmp;
	}

	// dodawanie s��w kodowych do s�ownika
	int addCodeword(char* pWord, int length) {
		if(head == NULL) {
			head = new Element(pWord, length);
			tail = head;
			count++;
			size += length;
			return count - 1; // zwraca kod nowego s�owa w s�owniku
		}

		tail->next = new Element(pWord, length);
		tail = tail->next;
		count++;
		size += length;
		return count - 1; // zwraca kod nowego s�owa w s�owniku
	}

	// zwraca kod danego s�owa w s�owniku; je�li brak w s�owniku to zwraca -1
	int getCodewordId(char* pWord, int length) {
		Element* tmp = head;
		int result = -1; // domy�lnie nie znaleziono elementu
		for (int i = 0; i < count; i++) {
			if(length != tmp->size) {// nie ma sensu por�wnywa� o ci�g�w o r�nych d�ugo�ciach
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

	int getSize() {
		return size;
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