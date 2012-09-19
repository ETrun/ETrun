#include "g_local.h"
#include "g_api.h"

/**
 * Global handles
 */
#if defined _WIN32
	typedef int (*API_query_t)(char *, char *, char *);
	API_query_t API_query;
	typedef int (*API_init_t)(void);
	API_init_t API_init;
	typedef void (*API_clean_t)(void);
	API_clean_t API_clean;
	HMODULE api_module;
# else
	void *api_module;
	int (*API_query)(char *, char *, char *);
	int (*API_init)(void);
	void (*API_clean)(void);
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
static qboolean loadAPISymbols() {
#if defined _WIN32
	API_query = (API_query_t)GetProcAddress(api_module, API_INTERFACE_NAME);
	API_init = (API_init_t)GetProcAddress(api_module, API_INIT_NAME);
	API_clean = (API_clean_t)GetProcAddress(api_module, API_CLEAN_NAME);
#else
	*(void **) (&API_query) = dlsym(api_module, API_INTERFACE_NAME);
	*(void **) (&API_init) = dlsym(api_module, API_INIT_NAME);
	*(void **) (&API_clean) = dlsym(api_module, API_CLEAN_NAME);
#endif
	
	if (API_query == NULL || API_init == NULL || API_clean == NULL) {
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
	char buf[1000] = {0};
	int count = 0;

	len = strlen(data);

	while (count < len) {
		Q_strncpyz(buf, data + count * sizeof (char), sizeof (buf) + 1);
		count += sizeof (buf);
		CP(va("print \"%s\"", buf));
	}
}

static char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'A' + 10;
}

static char to_hex(char code) {
  char hex[] = "0123456789ABCDEF";
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
 * Function used to decode an url
 */
void url_decode(char *str, char *dst) {
	char *pstr = str;
	int i = 0;

	while (*pstr) {
		if (*pstr == '%' && pstr[1] && pstr[2]) {
			dst[i] = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
			pstr += 2;
		} else {
			dst[i] = *pstr;
		}
		pstr++;
		i++;
	}
	dst[i] = '\0';
}

/**
 * Check for errors in API string result
 */
static qboolean checkAPIResult(char *result) {
	if (!result) {
		return qfalse;
	}

	if (!Q_stricmp(result, "timemout")) {
		return qfalse;
	}
	if (!Q_stricmp(result, "error")) {
		return qfalse;
	}
	return qtrue;
}

/**
 * Log (and print) an API message
 */
void APILog(const char *s, qboolean printIt) {
	char string[1024] = {0};
	const char *aMonths[12] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	qtime_t ct;
	trap_RealTime(&ct);

	if (printIt) {
		G_Printf("%s", s);
	}

	Com_sprintf(string, sizeof (string), "[%s%02d-%02d %02d:%02d:%02d] %s", aMonths[ct.tm_mon], ct.tm_mday, 1900 + ct.tm_year, ct.tm_hour, ct.tm_min, ct.tm_sec, s);

	if (level.APILog) {
		trap_FS_Write(string, strlen(string), level.APILog);
	} else {
		G_Printf("APILog: error while logging\n");
	}
}

/**
 * Login handler
 */
static void *loginHandler(void *data) {
	int code = -1;
	int len = 0;
	gentity_t *ent = NULL;
	struct query_s *queryStruct = NULL;

	queryStruct = (struct query_s *)data;
	ent = queryStruct->ent;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	len = strlen(queryStruct->result);

	APILog(va("Login: code = %d, result = %s\n", code, queryStruct->result), qfalse);

	if (code == 0) {
		if (len > 0 && ent && ent->client) {
			ent->client->sess.logged = qtrue;
			CP(va("print \"%s^w: you are now logged in!\n\"", GAME_VERSION_COLORED));
			ClientUserinfoChanged(ent->client->ps.clientNum);

			// Notify client that he is now logged in
			trap_SendServerCommand(ent - g_entities, "client_login");

			// Start recording a new temp demo.
			trap_SendServerCommand(ent - g_entities, "tempDemoStart");
		} else {
			CP(va("print \"%s^w: login failed!\n\"", GAME_VERSION_COLORED));
		}
	} else {
		CP(va("print \"%s^w: login failed!\n\"", GAME_VERSION_COLORED));
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("Decreasing threads counter to %d\n", activeThreadsCounter);

	return NULL;
}

/**
 * Login request command
 */
void G_API_login(char *result, gentity_t *ent, char *authToken) {
	char net_port[8] = {0};

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));

	G_callAPI("l", result, ent, 2, authToken, net_port);

	APILog("Login request sent!\n", qfalse);
}

/**
 * Map records handler
 */
static void *mapRecordsHandler(void *data) {
	int code = -1;
	gentity_t *ent = NULL;
	struct query_s *queryStruct = NULL;

	queryStruct = (struct query_s *)data;
	ent = queryStruct->ent;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	APILog(va("mapRecords: code = %d\n", code), qfalse);

	if (code == 0) {
		clientBigDataPrint(ent, queryStruct->result);
	} else {
		CP(va("print \"^1> ^wError while requesting records\n\""));
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("Decreasing threads counter to %d\n", activeThreadsCounter);

	return NULL;
}

/**
 * Map records request command
 */
void G_API_mapRecords(char *result, gentity_t *ent, char *mapName) {
	char net_port[8] = {0};
	char cphysics[8] = {0};
	char encodedMapName[255] = {0};

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	url_encode(mapName, encodedMapName);

	G_callAPI("m", result, ent, 3, encodedMapName, cphysics, net_port);

	APILog("Map records request sent!\n", qfalse);
}

/**
 * Check API handler
 */
static void *checkAPIHandler(void *data) {
	int code = -1;
	struct query_s *queryStruct = NULL;

	queryStruct = (struct query_s *)data;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	APILog(va("checkAPI: code = %d, result = %s\n", code, queryStruct->result), qfalse);

	if (code == 0) {
		G_Printf("%s: %s\n", GAME_VERSION, queryStruct->result);
	} else {
		G_Printf("%s: failed to check API (code: %d, result: %s)\n", GAME_VERSION, code, queryStruct->result);

		// Nico, disable use of API
		// trap_Cvar_Set("g_useAPI", "0");
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("Decreasing threads counter to %d\n", activeThreadsCounter);

	return NULL;
}

/**
 * Check API command
 */
void G_API_check(char *result, gentity_t *ent) {
	char net_port[8] = {0};
	char cphysics[8] = {0};

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	G_callAPI("c", result, ent, 2, cphysics, net_port);

	APILog("Check API request sent!\n", qfalse);
}

/**
 * Record handler
 */
static void *recordHandler(void *data) {
	int code = -1;
	struct query_s *queryStruct = NULL;
	gentity_t *ent = NULL;
	int timerunNum = 0;

	queryStruct = (struct query_s *)data;
	ent = queryStruct->ent;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	APILog(va("Record: code = %d, result = %s\n", code, queryStruct->result), qfalse);

	timerunNum = ent->client->sess.currentTimerunNum;

	switch (code) {
	case 1001: // PB
		if (ent->client->sess.timerunCheckpointWereLoaded[timerunNum]) {
			memcpy(ent->client->sess.timerunBestCheckpointTimes[timerunNum], ent->client->sess.timerunCheckpointTimes, sizeof (ent->client->sess.timerunCheckpointTimes));
		}
		AP(va("print \"%s: ^w%s\n\"", GAME_VERSION_COLORED, queryStruct->result));
		break;

	case 1002:// SB
		if (ent->client->sess.timerunCheckpointWereLoaded[timerunNum]) {
			memcpy(ent->client->sess.timerunBestCheckpointTimes[timerunNum], ent->client->sess.timerunCheckpointTimes, sizeof (ent->client->sess.timerunCheckpointTimes));
		}
		AP(va("bp \"^w%s\n\"", queryStruct->result));
		break;

	case 1003:// SB but player was already rec holder
		if (ent->client->sess.timerunCheckpointWereLoaded[timerunNum]) {
			memcpy(ent->client->sess.timerunBestCheckpointTimes[timerunNum], ent->client->sess.timerunCheckpointTimes, sizeof (ent->client->sess.timerunCheckpointTimes));
		}
		AP(va("bp \"^w%s\n\"", queryStruct->result));
		break;

	case 1004:// SB was tied
		if (ent->client->sess.timerunCheckpointWereLoaded[timerunNum]) {
			memcpy(ent->client->sess.timerunBestCheckpointTimes[timerunNum], ent->client->sess.timerunCheckpointTimes, sizeof (ent->client->sess.timerunCheckpointTimes));
		}
		AP(va("bp \"^w%s\n\"", queryStruct->result));
		break;

	case 1005:// Slow time
		CP(va("print \"%s: ^w%s\n\"", GAME_VERSION_COLORED, queryStruct->result));
		break;

	default:// Error
		CP(va("print \"%s: ^wError: %s\n\"", GAME_VERSION_COLORED, queryStruct->result));
		break;
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("Decreasing threads counter to %d\n", activeThreadsCounter);

	return NULL;
}

/**
 * Record send command
 */
void G_API_sendRecord(char *result, gentity_t *ent, char *mapName, char *runName, 
					  char *authToken, char *data, char *etrunVersion) {
	char net_port[8] = {0};
	char encodedMapName[255] = {0};
	char encodedRunName[255] = {0};

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));

	url_encode(mapName, encodedMapName);
	url_encode(runName, encodedRunName);

	G_callAPI("d", result, ent, 6, encodedMapName, encodedRunName, authToken, data, etrunVersion, net_port);

	APILog("Map record send request sent!\n", qfalse);
}

/**
 * Get checkpoints handler
 */
static void *checkpointsHandler(void *data) {
	int code = -1;
	struct query_s *queryStruct = NULL;
	gentity_t *ent = NULL;
	char * pch = NULL;
	int i = 0;
	int timerunNum = 0;

	queryStruct = (struct query_s *)data;
	ent = queryStruct->ent;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	APILog(va("Checkpoints: code = %d, result = %s\n", code, queryStruct->result), qfalse);

	if (code >= 1000) {

		timerunNum = code - 1000;

		if (queryStruct->result && checkAPIResult(queryStruct->result) && timerunNum >= 0 && timerunNum < MAX_TIMERUNS) {
			// No error, no timeout

			// Reset client checkpoints
			memset(ent->client->sess.timerunBestCheckpointTimes[timerunNum], 0, sizeof (ent->client->sess.timerunBestCheckpointTimes[timerunNum]));
			ent->client->sess.timerunCheckpointsPassed = 0;

			// Set new checkpoints
			pch = strtok (queryStruct->result, "O");
			while (pch != NULL && i < MAX_TIMERUN_CHECKPOINTS) {
				ent->client->sess.timerunBestCheckpointTimes[timerunNum][i] = atoi(pch);
				pch = strtok (NULL, "O");
				i++;
			}

			// Mark CP were loaded for this run
			ent->client->sess.timerunCheckpointWereLoaded[timerunNum] = 1;

			CP(va("print \"^1> ^wCheckpoints loaded for run #%d!\n\"", timerunNum));
		} else {
			CP(va("print \"^1> ^wError while loading checkpoints!\n\""));
		}
	} else {
		CP(va("print \"^1> ^wError while loading checkpoints!\n\""));
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("Decreasing threads counter to %d\n", activeThreadsCounter);

	return NULL;
}

/**
 * Checkpoints request command
 */
void G_API_getPlayerCheckpoints(char *result, gentity_t *ent, char *mapName, char *runName, int runNum, char *authToken) {
	char net_port[8] = {0};
	char bufferRunNum[8] = {0};
	char encodedMapName[255] = {0};
	char encodedRunName[255] = {0};
	char cphysics[8] = {0};

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(bufferRunNum, "%d", runNum);
	sprintf(cphysics, "%d", physics.integer);

	url_encode(mapName, encodedMapName);
	url_encode(runName, encodedRunName);

	G_callAPI("e", result, ent, 6, encodedMapName, encodedRunName, bufferRunNum, authToken, cphysics, net_port);

	APILog("Checkpoints request sent!\n", qfalse);
}

/**
 * Random map handler
 */
static void *randommapHandler(void *data) {
	int code = -1;
	struct query_s *queryStruct = NULL;
	gentity_t *ent = NULL;
	char mapfile[MAX_QPATH] = {0};
	fileHandle_t f;

	queryStruct = (struct query_s *)data;
	ent = queryStruct->ent;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	APILog(va("Randommap: code = %d, result = %s\n", code, queryStruct->result), qfalse);

	if (code == 0 && queryStruct->result && checkAPIResult(queryStruct->result)) {

		Com_sprintf(mapfile, sizeof(mapfile), "maps/%s.bsp", queryStruct->result);

		trap_FS_FOpenFile(mapfile, &f, FS_READ);

		trap_FS_FCloseFile(f);

		if (!f) {
			AP(va("print \"^1Error:^7 the map ^3%s^7 is not on the server.\n\"", queryStruct->result));
		} else {
			// Nico, delay the map change
			G_delay_map_change(queryStruct->result);
		}
	} else {
		CP(va("print \"^1> ^wError while getting a random map!\n\""));
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("Decreasing threads counter to %d\n", activeThreadsCounter);

	return NULL;
}

/**
 * Checkpoints request command
 */
void G_API_randommap(char *result, gentity_t *ent, char *mapName) {
	char net_port[8] = {0};
	char encodedMapName[255] = {0};
	char cphysics[8] = {0};

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	url_encode(mapName, encodedMapName);

	G_callAPI("f", result, ent, 3, encodedMapName, cphysics, net_port);

	APILog("Random map request sent!\n", qfalse);
}

/**
 * Map rank command handler
 */
static void *mapRankHandler(void *data) {
	int code = -1;
	gentity_t *ent = NULL;
	struct query_s *queryStruct = NULL;

	queryStruct = (struct query_s *)data;
	ent = queryStruct->ent;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query);

	APILog(va("mapRank: code = %d\n", code), qfalse);

	if (code == 0) {
		clientBigDataPrint(ent, queryStruct->result);
	} else {
		CP(va("print \"^1> ^wError while requesting rank\n\""));
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("Decreasing threads counter to %d\n", activeThreadsCounter);

	return NULL;
}

/**
 * Map rank request command
 */
void G_API_mapRank(char *result, gentity_t *ent, char *mapName, char *optUserName, char *optMapName, char *optRunName, char *optPhysicsName, char *authToken) {
	char net_port[8] = {0};
	char cphysics[8] = {0};
	char encodedMapName[255] = {0};
	char encodedOptUserName[255] = {0};
	char encodedOptMapName[255] = {0};
	char encodedOptRunName[255] = {0};

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	url_encode(mapName, encodedMapName);
	url_encode(optUserName, encodedOptUserName);
	url_encode(optMapName, encodedOptMapName);
	url_encode(optRunName, encodedOptRunName);

	G_callAPI("r", result, ent, 8, encodedOptUserName, encodedOptMapName, encodedOptRunName, optPhysicsName, encodedMapName, authToken, cphysics, net_port);

	APILog("Map rank request sent!\n", qfalse);
}

// Commands handler binding
static const api_glue_t APICommands[] = {
	{ "l",	loginHandler },
	{ "m",	mapRecordsHandler },
	{ "c", 	checkAPIHandler },
	{ "d",	recordHandler },
	{ "e",	checkpointsHandler },
	{ "f",	randommapHandler },
	{ "r",	mapRankHandler }
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
	pthread_attr_t attr;
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
		// G_Printf("arg : %s\n", arg);
	
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

	APILog(va("Calling API with command: %s, query: %s\n", command, queryStruct->query), qfalse);
	G_LogPrintf("Calling API with command: %s, query: %s\n", command, queryStruct->query);

	// Create threads as detached as they will never be joined
	if (pthread_attr_init(&attr)) {
		G_Error("G_callAPI: error in pthread_attr_init\n");
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		G_Error("G_callAPI: error in pthread_attr_setdetachstate\n");
	}

	returnCode = pthread_create(&thread, &attr, handler, (void *)queryStruct);

	if (returnCode) {
		G_Error("G_callAPI: error in pthread_create: %d\n", returnCode);
	} else {
		// Nico, increase active threads counter
		activeThreadsCounter++;
		G_DPrintf("Increasing threads counter to %d\n", activeThreadsCounter);
	}

	if (pthread_attr_destroy(&attr)) {
		G_Error("G_callAPI: error in pthread_attr_destroy\n");
	}

	if (count > 0) {
		va_end (ap);
	}
}

void G_loadAPI() {

	// Load the module
	if (!loadModule()) {
		printError();
		G_Error("Error loading %s\n", g_APImodulePath.string);
	}

	// Load the APIquery function 
	if (!loadAPISymbols()) {
		printError();
		G_Error("Error loading symbols from %s\n", g_APImodulePath.string);
	}

	if (API_init() != 0) {
		G_Error("Error calling API_init()");
	}

	G_Printf("ETrun: API module loaded!\n");
}

void G_unloadAPI() {
	if (api_module == NULL) {
		G_DPrintf("G_callAPI: API module is not loaded.\n");
	} else {

	API_clean();

#if defined _WIN32
	FreeLibrary(api_module);
#else
	dlclose(api_module);
#endif

		G_Printf("ETrun: API module unloaded!\n");
	}
}
