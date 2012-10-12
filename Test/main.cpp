#include <iostream>
#include <fstream>

using namespace std;

struct data{
	int liczba:4;
	int liczba2:4;
};

union u{
	data z1;
	char c1;
};

int main() {
	u zmienna;
	zmienna.z1.liczba = 2;
	zmienna.z1.liczba2 = 0;

	cout << sizeof(short) << endl;
	cout << (int)zmienna.c1 << endl;

	fstream file;
	file.open("plik.txt", ios::out | ios::binary | ios::trunc);

	file.close();

	system("pause");
	return 0;
}