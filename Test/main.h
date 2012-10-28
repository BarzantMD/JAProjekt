#ifndef MAIN_H
#define MAIN_H

#include <iostream>

using namespace std;


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
		if(head == NULL) return;
		Element* tmp = head;
		while(tmp->next != NULL) {
			Element* tmp_prev = tmp;
			tmp = tmp->next;
			delete tmp_prev;
		}
		delete tmp;
	}

	// dodawanie s³ów kodowych do s³ownika
	int addCodeword(char* pWord, int length) {
		if(head == NULL) {
			head = new Element(pWord, length);
			tail = head;
			count++;
			size += length;
			return count - 1; // zwraca kod nowego s³owa w s³owniku
		}

		tail->next = new Element(pWord, length);
		tail = tail->next;
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