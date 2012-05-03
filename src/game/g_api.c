#include "g_api.h"
#include "g_local.h"

#if defined _WIN32
	typedef int (__stdcall *MYPROC)(HWND hWnd, char *msg);
	HMODULE api_module;
	MYPROC API_query;
# else
	void *api_module;
# endif


static qboolean loadModule() {

#if defined _WIN32
	api_module = LoadLibraryA(MODULE_DIR"\\"MODULE_NAME);
#else
	api_module = dlopen(MODULE_DIR"/"MODULE_NAME, RTLD_LAZY);
#endif
	
	if (api_module == NULL) {
		return qfalse;
	}
	return qtrue;
}

static qboolean loadAPIquery() {

#if defined _WIN32
	API_query = (MYPROC)GetProcAddress(api_module, API_INTERFACE_NAME);
#else
	// #TODO
#endif
	
	if (API_query == NULL) {
		return qfalse;
	}
	return qtrue;
}

static void printError() 
{ 
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;

    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

	G_Printf("Error: %s\n", lpMsgBuf);

    LocalFree(lpMsgBuf);
}

void G_loadApi() {

	// Load the module
	if (!loadModule()) {
		printError();
		G_Error("Error loading %s\n", MODULE_DIR"\\"MODULE_NAME);
	}

	// Load the APIquery function 
	if (!loadAPIquery()) {
		printError();
		G_Error("Error loading %s from %s\n", API_INTERFACE_NAME, MODULE_NAME);
	}

	G_Printf("ETrun: API loaded!\n");
}
