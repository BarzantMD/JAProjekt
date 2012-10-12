// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the JADLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// JADLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef JADLL_EXPORTS
#define JADLL_API __declspec(dllexport)
#else
#define JADLL_API __declspec(dllimport)
#endif

// This class is exported from the JADLL.dll
class JADLL_API CJADLL {
public:
	CJADLL(void);
	// TODO: add your methods here.
};

extern JADLL_API int nJADLL;

JADLL_API int fnJADLL(void);
