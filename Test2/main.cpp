#include <iostream>
#include <sstream>

using namespace std;

int main(int argc, char* argv[]) {

	stringstream convert(argv[1]);
	int liczba = 0;
	convert >> liczba;

	cout << "Liczba: " << liczba << endl;

	system("pause");
	return 0;
}