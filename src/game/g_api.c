#include "g_api.h"
#include "g_local.h"

#if defined _WIN32
	typedef api_status (*MYPROC)(char *, char *);
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
#if defined _WIN32
    LPVOID error;

    DWORD dw = GetLastError(); 

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &error,
        0, NULL );

	G_Printf("Error: %s\n", error);

    LocalFree(error);
#else
	// #TODO
#endif
}

static void *myCallAPI(void *data) {
	int code = -1;

	struct query_s *queryStruct;

	queryStruct = (struct query_s *)data;

	code = API_query(queryStruct->result, queryStruct->query);

	if (code == OK) {
		G_Printf("[THREAD]Result: size = %d, %s\n", strlen(queryStruct->result), queryStruct->result);
	} else {
		G_Printf("[THREAD]Error, code: %d\n", code);
	}

	free(queryStruct);

	return NULL;
}

void G_callAPI(char *result, char *query) {
	struct query_s *queryStruct;
	pthread_t thread;
	int returnCode = 0;

#if defined _WIN32

#else
	// #TODO
#endif
	if (API_query == NULL) {
		G_Error("G_callAPI: API module is not loaded.");
	}

	queryStruct = malloc(sizeof (struct query_s));

	if (queryStruct == NULL) {
		G_Error("G_callAPI: malloc failed\n");
	}

	Q_strncpyz(queryStruct->query, query, 256);
	queryStruct->result = result;

	returnCode = pthread_create(&thread, NULL, myCallAPI, (void *)queryStruct);

	if (returnCode) {
		G_Error("G_callAPI: error creating thread\n");
	}
	G_Printf("G_callAPI: thread started!\n");
}

void G_loadAPI() {

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

	G_Printf("ETrun: API module loaded!\n");
}
