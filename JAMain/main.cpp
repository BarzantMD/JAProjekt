#include "main.h"
#include "LZW.h"
#include <iostream>
#include <fstream>
#include <string>

using namespace std;


int main(int argc, char* argv[]) {
	//cout << "hello world" << endl;
	//char pZmienna[20] = "hello world";
	//int z = TestProc((int)pZmienna,3);
	//cout << "wynik testproc to: " << z << endl;
	//cout << "Adres pzmienna to: " << (int)pZmienna << endl;
	//cout << "Wartosc zmiennej to: " << pZmienna << endl;

	int c = parseCommand(argc, argv);
	
	if(c == -1)
		writeUnknow();

	if(c == 1 || c == 3) { // kompresja
		cout << "Przygotowywanie do kompresji..." << endl;

		string srcFilename = argv[2];
		string destFilename;
		if(argc == 3)
			destFilename = srcFilename + ".packed";
		else
			destFilename = argv[3];

		
		fstream srcFile;
		srcFile.open(srcFilename.c_str(), ios::binary | ios::in); // otwarcie pliku Ÿród³owego

		if(!srcFile.is_open()) { // sprawdzenie czy plik Ÿród³owy istnieje
			cout << "Nie mozna otworzyc pliku zrodlowego lub plik nie istnieje!";
			system("pause");
			return 0;
		}

		// za³adowanie zawartoœci pliku do pamiêci
		srcFile.seekg(0, ios::end);
		int filesize = srcFile.tellg();
		cout << "Rozmiar pliku zrodlowego: " << filesize << endl;
		
		char* buffer = new char[filesize];
		srcFile.read(buffer, filesize);
		srcFile.close();


		
		delete buffer;
	}

	system("pause");
	return 0;
}

/* Zwraca kod wybranego polecenia
-1 - nieznane polecenie
0 - w³asny komunikat funkcji
1 - kompresja
2 - dekompresja 
3 - kompresja asm
4 - dekompresja asm*/
int parseCommand(int argc, char* argv[]) {
	if(argc >=5) return -1; // zbyt wiele argumentów

	if(argc == 1) { // brak argumentów - wpisano sam¹ nazwê programu
		writeHelp();
		return 0;
	}

	if(argc == 2) { // jeden argument zosta³ podany - sprawdzenie czy u¿ytkownik prosi o pomoc
		if(string(argv[1]) == "/?" || string(argv[1]) == "/help") {
			writeHelp();
			return 0;
		}
		else
			return -1;
	}

	/* argc = <3,4> */

	if(string(argv[1]) == "/c")
		return 1;

	if(string(argv[1]) == "/d")
		return 2;

	if(string(argv[1]) == "/ca")
		return 3;

	if(string(argv[1]) == "/da")
		return 4;

	return -1;
}

void writeHelp() {
	cout << "Uzycie: JAMain opcja plik_zrodlowy [plik_docelowy]" << endl << endl;
	cout << "Opcje:" << endl;
	cout << "/c - kompresja pliku" << endl;
	cout << "/d - dekompresja pliku" << endl;
	cout << "/ca - kompresja pliku funkcja asemblera" << endl;
	cout << "/da - dekompresja pliku funkcja asemblera" << endl;
	cout << "/? - wyswietlenie tej pomocy" << endl;
	cout << "/help - wyswietlenie tej pomocy" << endl;
}

void writeUnknow() {
	cout << "Nieznane polecenie. Aby skorzystac z pomocy wpisz JAMain /? lub JAMain /help" << endl;
}