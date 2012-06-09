#include "g_local.h"
#include "g_api.h"

/**
 * Global handles
 */
#if defined _WIN32
	typedef int (*MYPROC)(char *, char *, char *);
	MYPROC API_query;
	HMODULE api_module;
# else
	void *api_module;
	int (*API_query)(char *, char *, char *);
# endif

/**
 * Module loading
 */
static qboolean loadModule() {
#if defined _WIN32
	api_module = LoadLibraryA(g_APImodulePath.string);
#else
	api_module = dlopen(g_APImodulePath.string, RTLD_LAZY);
#endif
	
	if (api_module == NULL) {
		return qfalse;
	}
	return qtrue;
}

/**
 * Module interface linking
 */
static qboolean loadAPIquery() {
#if defined _WIN32
	API_query = (MYPROC)GetProcAddress(api_module, API_INTERFACE_NAME);
#else
	*(void **) (&API_query) = dlsym(api_module, API_INTERFACE_NAME);
#endif
	
	if (API_query == NULL) {
		return qfalse;
	}
	return qtrue;
}

/**
 * Error printing
 */
static void printError() {
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
	G_Printf("Error: %s\n", dlerror());
#endif
}

/**
 * Function used to print large buffers (> 1022) to client
 */
static void clientBigDataPrint(gentity_t *ent, char *data) {
	int len = 0;
	char buf[512];
	int count = 0;

	len = strlen(data);

	while (count < len) {
		Q_strncpyz(buf, data + count * sizeof (char), sizeof (buf) + 1);
		count += sizeof (buf);
		CP(va("print \"%s\"", buf));
	}
}

static char to_hex(char code) {
  char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/**
 * Function used to encode an url
 */
void url_encode(char *str, char *dst) {
	char *pstr = str;
	int i = 0;

	while (*pstr) {
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') {
			dst[i] = *pstr;
		} else {
			dst[i++] = '%';
			dst[i++] = to_hex(*pstr >> 4);
			dst[i] = to_hex(*pstr & 15);
		}
		pstr++;
		i++;
	}
	dst[i] = '\0';
}

/**
 * Login handler
 */
static void *loginHandler(void *data) {
	int code = -1;
	int len = 0;
	gentity_t *ent = NULL;
	struct query_s *queryStruct;

	queryStruct = (struct query_s *)data;
	ent = queryStruct->ent;

	G_Printf("[THREAD]Calling API now!\n");

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	len = strlen(queryStruct->result);

	if (code == 0) {
		G_Printf("[THREAD]Result: size = %d, %s\n", (int)len, queryStruct->result);

		if (len > 0 && ent && ent->client) {
			ent->client->sess.logged = qtrue;
			CP("cp \"You are now logged in!\n\"");
			G_Printf("[THREAD] %s is now authentificated!\n", queryStruct->result);
			ClientUserinfoChanged(ent->client->ps.clientNum);
		} else {
			G_Printf("[THREAD]Authentification failed\n");
		}
	} else {
		CP("cp \"Login failed!\n\"");
		G_Printf("[THREAD]Error, code: %d\n", code);
	}

	free(queryStruct->result);
	free(queryStruct);

	return NULL;
}

/**
 * Login request command
 */
void G_API_login(char *result, gentity_t *ent, char *authToken) {
	char net_port[8];

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));

	G_callAPI("l", result, ent, 2, authToken, net_port);

	G_Printf("Login request sent!\n");
}

/**
 * Map records handler
 */
static void *mapRecordsHandler(void *data) {
	int code = -1;
	gentity_t *ent = NULL;
	struct query_s *queryStruct;

	queryStruct = (struct query_s *)data;
	ent = queryStruct->ent;

	G_Printf("[THREAD]Calling API now!\n");

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	if (code == 0) {
		G_Printf("[THREAD]Result size = %d:\n", (int)strlen(queryStruct->result));
		G_Printf("%s\n", queryStruct->result);
		clientBigDataPrint(ent, queryStruct->result);
	} else {
		G_Printf("[THREAD]Error, code: %d, %s\n", code, queryStruct->result);
		CP(va("print \"Error while requesting records\n\""));
	}

	free(queryStruct->result);
	free(queryStruct);

	return NULL;
}

/**
 * Map records request command
 */
void G_API_mapRecords(char *result, gentity_t *ent, char *mapName) {
	char net_port[8];
	char encodedMapName[255] = {0};

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));

	url_encode(mapName, encodedMapName);

	G_callAPI("m", result, ent, 2, encodedMapName, net_port);

	G_Printf("Map records request sent!\n");
}

/**
 * Check API handler
 */
static void *checkAPIHandler(void *data) {
	int code = -1;
	struct query_s *queryStruct;

	queryStruct = (struct query_s *)data;

	G_Printf("[THREAD]Calling API now!\n");// Crash here on OSX

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	if (code == 0) {
		G_Printf("[THREAD]Result size = %d:\n", (int)strlen(queryStruct->result));
		G_Printf("%s\n", queryStruct->result);
	} else {
		G_Printf("[THREAD]Error, code: %d, %s\n", code, queryStruct->result);
	}

	free(queryStruct->result);
	free(queryStruct);

	return NULL;
}

/**
 * Check API command
 */
void G_API_check(char *result, gentity_t *ent) {
	char net_port[8];

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));

	G_callAPI("c", result, ent, 1, net_port);

	G_Printf("Check API request sent!\n");
}

/**
 * Record handler
 */
static void *recordHandler(void *data) {
	int code = -1;
	struct query_s *queryStruct;
	gentity_t *ent = NULL;

	queryStruct = (struct query_s *)data;
	ent = queryStruct->ent;

	G_Printf("[THREAD]Calling API now!\n");// Crash here on OSX

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	if (code == 0) {
		G_Printf("[THREAD]Result size = %d:\n", (int)strlen(queryStruct->result));
		G_Printf("%s\n", queryStruct->result);
		if (queryStruct->result) {
			CP(va("print \"%s\n\"", queryStruct->result));
		}
	} else {
		G_Printf("[THREAD]Error, code: %d, %s\n", code, queryStruct->result);
	}

	free(queryStruct->result);
	free(queryStruct);

	return NULL;
}

/**
 * Record send command
 */
void G_API_sendRecord(char *result, gentity_t *ent, char *mapName, char *runName, 
					  char *authToken, char *data, char *etrunVersion) {
	char net_port[8];
	char encodedMapName[255] = {0};
	char encodedRunName[255] = {0};

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));

	url_encode(mapName, encodedMapName);
	url_encode(runName, encodedRunName);

	G_callAPI("d", result, ent, 6, encodedMapName, encodedRunName, authToken, data, etrunVersion, net_port);

	G_Printf("Map record send request sent!\n");
}

// Commands handler binding
static const api_glue_t APICommands[] = {
	{ "l",	loginHandler },
	{ "m",	mapRecordsHandler },
	{ "c", 	checkAPIHandler },
	{ "d",	recordHandler }
};

/**
 * Takes a command string as argument and returns the associated handler if any, NULL otherwise
 */
static handler_t getHandlerForCommand(char *cmd) {
	unsigned int i, cCommands = sizeof (APICommands) / sizeof (APICommands[0]);
	const api_glue_t *element;

	if (!cmd) {
		return NULL;
	}

	for (i = 0; i < cCommands; ++i) {
		element = &APICommands[i];
		if ( element && element->cmd && !Q_stricmp(cmd, element->cmd)) {
			return element->handler;
		}
	}
	return NULL;
}

/**
 * APImodule entry point
 *
 * command: must be a command in apiCommands
 * result: pointer to an *already allocated* buffer for storing result
 * ent: entity who made the request
 * count: number of variadic arguments
 */
void G_callAPI(char *command, char *result, gentity_t *ent, int count, ...) {
	struct query_s *queryStruct;
	pthread_t thread;
	int returnCode = 0;
	void *(*handler)(void *) = NULL;
	va_list ap;
	int i = 0;
	char *arg = NULL;

	if (api_module == NULL || API_query == NULL) {
		G_Error("G_callAPI: API module is not loaded.");
	}

	queryStruct = malloc(sizeof (struct query_s));

	if (queryStruct == NULL) {
		G_Error("G_callAPI: malloc failed\n");
	}

	if (count > 0) {
		va_start (ap, count);
	}

	// Init query buffer
	memset(queryStruct->query, 0, QUERY_MAX_SIZE);

	for (i = 0; i < count; ++i) {
		arg = va_arg (ap, char*);

		if (!arg) {
			G_Error("G_callAPI: empty arg %d\n", i);
		}
		G_Printf("arg : %s\n", arg);
	
		Q_strcat(queryStruct->query, QUERY_MAX_SIZE, arg);

		// No trailing /
		if (i + 1 < count) {
			Q_strcat(queryStruct->query, QUERY_MAX_SIZE, CHAR_SEPARATOR);
		}
	}
	Q_strncpyz(queryStruct->cmd, command, sizeof (queryStruct->cmd));
	queryStruct->result = result;
	queryStruct->ent = ent;

	// Get the command handler
	handler = getHandlerForCommand(command);

	if (!handler) {
		G_Error("G_callAPI: no handler for command: %s\n", command);
	}

	G_Printf("Calling API with command: %s, query: %s\n", command, queryStruct->query);

	returnCode = pthread_create(&thread, NULL, handler, (void *)queryStruct);

	if (returnCode) {
		G_Error("G_callAPI: error creating thread\n");
	}
	G_Printf("G_callAPI: thread started!\n");

	if (count > 0) {
		va_end (ap);
	}
}

#if defined _WIN32
static void pthread_load() {
	pthread_win32_process_attach_np();
}

static void pthread_unload() {
	pthread_win32_process_detach_np();
}
#endif


void G_loadAPI() {

	// Load the module
	if (!loadModule()) {
		printError();
		G_Error("Error loading %s\n", g_APImodulePath.string);
	}

	// Load the APIquery function 
	if (!loadAPIquery()) {
		printError();
		G_Error("Error loading %s from %s\n", API_INTERFACE_NAME, g_APImodulePath.string);
	}

#if defined _WIN32
	pthread_load();
#endif

	G_Printf("ETrun: API module loaded!\n");
}

void G_unloadAPI() {
	if (api_module == NULL) {
		G_DPrintf("G_callAPI: API module is not loaded.");
	} else {

#if defined _WIN32
	FreeLibrary(api_module);
	pthread_unload();
#else
	dlclose(api_module);
#endif

		G_Printf("ETrun: API module unloaded!\n");
	}
}
