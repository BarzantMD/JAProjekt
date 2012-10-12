#include "LZW.h"
#include <iostream>

using namespace std;

int compress(char* srcData, int dataSize, char* &compressedData) {
	
	compressedData = new char[dataSize];
	memcpy(compressedData, srcData, dataSize);
	return dataSize;
}