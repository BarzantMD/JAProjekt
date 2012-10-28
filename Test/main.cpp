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

	char* data = new char[20];
	short* data2 = (short*)data;

	cout << "data: " << hex << (int)data << endl;
	cout << "data2: " << hex << (int)data2 << endl;
	
	data = data + 5;
	data2 = data2 + 5;

	cout << "data: " << hex << (int)data << endl;
	cout << "data2: " << hex << (int)data2 << endl;

	system("pause");
	return 0;
}
