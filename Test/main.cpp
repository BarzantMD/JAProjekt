#include <iostream>
#include <fstream>
#include <string>

#include "main.h"

using namespace std;


int main(int argc, char* argv[]) {

	if(argc != 3) {
		cout << "Nieprawidlowa liczba parametrow!" << endl;
		system("pause");
		return 0;
	}


	string filename1 = argv[1];
	string filename2 = argv[2];

	fstream file1;
		file1.open(filename1.c_str(), ios::binary | ios::in);

		if(!file1.is_open()) {
			cout << "Nie mozna otworzyc pliku lub plik nie istnieje!";
			return 0;
		}

		file1.seekg(0, ios::end);
		int filesize1 = file1.tellg();
		file1.seekg(0, ios::beg);
		cout << "Rozmiar pliku 1: " << filesize1 << endl;
		
		char* buffer1 = new char[filesize1];

		file1.read(buffer1, filesize1);
		file1.close();

		// przygotowanie alfabetu
		char* alphabet1 = new char[256]; // zarezerwowanie pamiêci dla alfabetu - max 256B

		int alphabet1Size = 0; // iloœæ znaków w alfabecie
		for (int i = 0; i < filesize1; i++) { // przegl¹damy ca³e dane
			bool isAlready = false; // czy jest ju¿ w s³owniku

			for (int j = 0; j < alphabet1Size; j++) { // pêtla porównuj¹ca znak z pêtli wy¿ej z ka¿dym znakiem z dotychczasowego alfabetu
				if(alphabet1[j] == buffer1[i]) {
					isAlready = true; // znak wystêpuje ju¿ w alfabecie
					break;
				}
			}

			if(!isAlready) {
					alphabet1[alphabet1Size] = buffer1[i]; // dodajemy nowy znak do alfabetu
					alphabet1Size++;
				}
		}


		// plik 2
		fstream file2;
		file2.open(filename2.c_str(), ios::binary | ios::in);

		if(!file2.is_open()) {
			cout << "Nie mozna otworzyc pliku lub plik nie istnieje!";
			return 0;
		}

		file2.seekg(0, ios::end);
		int filesize2 = file2.tellg();
		file2.seekg(0, ios::beg);
		cout << "Rozmiar pliku 2: " << filesize2 << endl;
		
		char* buffer2 = new char[filesize2];

		file2.read(buffer2, filesize2);
		file2.close();

		// przygotowanie alfabetu
		char* alphabet2 = new char[256]; // zarezerwowanie pamiêci dla alfabetu - max 256B

		int alphabet2Size = 0; // iloœæ znaków w alfabecie
		for (int i = 0; i < filesize2; i++) { // przegl¹damy ca³e dane
			bool isAlready = false; // czy jest ju¿ w s³owniku

			for (int j = 0; j < alphabet2Size; j++) { // pêtla porównuj¹ca znak z pêtli wy¿ej z ka¿dym znakiem z dotychczasowego alfabetu
				if(alphabet2[j] == buffer2[i]) {
					isAlready = true; // znak wystêpuje ju¿ w alfabecie
					break;
				}
			}

			if(!isAlready) {
					alphabet2[alphabet2Size] = buffer2[i]; // dodajemy nowy znak do alfabetu
					alphabet2Size++;
				}
		}

		// porównywanie

		if(filesize1 != filesize2) {
			cout << "Pliki maja rozny rozmiar!" << endl;
			return 0;
		}

		int roznica = 0;

		cout << "Roznice\tPowinien\tJest" << endl;
		for(int i = 0; i < filesize1; i++) {
			if(buffer1[i] != buffer2[i]) {
				roznica++;
				cout << roznica << ". Bajt nr: " << i << " \t" << (int)buffer1[i] << "\t" << (int)buffer2[i] << endl;
			}
		}

		// alfabety
		if(alphabet1Size != alphabet2Size) {
			cout << "Alfabety amja rozna ilosc znakow!" << endl;
			return 0;
		}

		cout << "Alfabet" << endl;
		cout << "A1\tA2" << endl;
		for(int i = 0; i < alphabet1Size; i++) {
			cout << (int)alphabet1[i] << "\t" << (int)alphabet2[i] << endl;
		}


	return 0;
}
