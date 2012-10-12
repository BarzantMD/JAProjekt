// JADLL.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "JADLL.h"


// This is an example of an exported variable
JADLL_API int nJADLL=0;

// This is an example of an exported function.
JADLL_API int fnJADLL(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see JADLL.h for the class definition
CJADLL::CJADLL()
{
	return;
}
