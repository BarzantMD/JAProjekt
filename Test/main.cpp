#include <iostream>
#include <fstream>
#include <string>

#include "main.h"

using namespace std;


int main() {
	//char* data = new char[20];
	//data = "Hello world and bye!";

	//Dictionary dict;
	//
	//dict.printDebugInfo();

	//dict.addCodeword(data, 3);
	//dict.addCodeword(data + 5, 5);

	//dict.printDebugInfo();

	//if(dict.getCodewordId("Heg",3) == -1) {
	//	cout << "Nie znaleziono!" << endl;
	//}
	//else {
	//	cout << "Znaleziono" << endl;
	//}

	//cout << "Zawartosc data: " << data << "|" << endl;

	unsigned short a = 35000;

	int b = a;
	int c = 40000;

	cout << "a = " << a << endl;
	cout << "b = " << b << endl;
	cout << "c = " << c << endl;

	a = c;

	cout << "a = " << a << endl;
	cout << "b = " << b << endl;
	cout << "c = " << c << endl;


	system("pause");
	return 0;
}
