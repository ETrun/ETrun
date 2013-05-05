#include "g_local.h"
#include "g_api.h"

/**
 * Global handles
 */
#if defined _WIN32
typedef int (*API_query_t)(char *, char *, char *, int);
API_query_t API_query;
typedef int (*API_init_t)(char *, char *, char *);
API_init_t API_init;
typedef void (*API_clean_t)(void);
API_clean_t API_clean;
HMODULE     api_module;
#else
void *api_module;
int  (*API_query)(char *, char *, char *, int);
int  (*API_init)(char *, char *, char *);
void (*API_clean)(void);
#endif

/**
 * Module loading
 */
static qboolean loadModule(char *basepath, char *homepath) {
	char searchPath[512];

	// Try opening module in basepath
#if defined _WIN32
	sprintf(searchPath, "%s\\etrun\\%s", basepath, g_APImoduleName.string);
	api_module = LoadLibraryA(searchPath);
#else
	sprintf(searchPath, "%s/etrun/%s", basepath, g_APImoduleName.string);
	api_module = dlopen(searchPath, RTLD_LAZY);
#endif

	if (api_module != NULL) {
		return qtrue;
	}

	// Try opening module form homepath
	#if defined _WIN32
		sprintf(searchPath, "%s\\etrun\\%s", homepath, g_APImoduleName.string);
		api_module = LoadLibraryA(searchPath);
	#else
		sprintf(searchPath, "%s/etrun/%s", homepath, g_APImoduleName.string);
		api_module = dlopen(searchPath, RTLD_LAZY);
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
	API_init  = (API_init_t)GetProcAddress(api_module, API_INIT_NAME);
	API_clean = (API_clean_t)GetProcAddress(api_module, API_CLEAN_NAME);
#else
	*(void **) (&API_query) = dlsym(api_module, API_INTERFACE_NAME);
	*(void **) (&API_init)  = dlsym(api_module, API_INIT_NAME);
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
	    0, NULL);

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
	int  len       = 0;
	char buf[1000];
	int  count     = 0;

	len = strlen(data);

	while (count < len) {
		memset(buf, 0, sizeof (buf));
		Q_strncpyz(buf, data + count * sizeof (char), sizeof (buf));
		count += sizeof (buf);
		CP(va("print \"%s\"", buf));
	}
}

/*
 *
 * @unused
static char from_hex(char ch) {
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'A' + 10;
}*/

static char to_hex(char code) {
	char hex[] = "0123456789ABCDEF";

	return hex[code & 15];
}

/**
 * Function used to encode an url
 *
 * note: dst must be already allocated and have the required size
 */
qboolean url_encode(char *str, char *dst) {
	char *pstr = str;
	int  i     = 0;

	if (!str) {
		LDE("str is NULL\n");
		return qfalse;
	}

	if (!dst) {
		LDE("dst is NULL\n");
		return qfalse;
	}

	while (*pstr) {
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') {
			dst[i] = *pstr;
		} else {
			dst[i++] = '%';
			dst[i++] = to_hex(*pstr >> 4);
			dst[i]   = to_hex(*pstr & 15);
		}
		pstr++;
		i++;
	}
	dst[i] = '\0';
	return qtrue;
}

/**
 * Function used to decode an url
 *
 * @unused
qboolean url_decode(char *str, char *dst) {
	char *pstr = str;
	int  i     = 0;

	if (!str) {
		LDE("str is NULL\n");
		return qfalse;
	}

	if (!dst) {
		LDE("dst is NULL\n");
		return qfalse;
	}

	while (*pstr) {
		if (*pstr == '%' && pstr[1] && pstr[2]) {
			dst[i] = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
			pstr  += 2;
		} else {
			dst[i] = *pstr;
		}
		pstr++;
		i++;
	}
	dst[i] = '\0';
	return qtrue;
}*/

/**
 * Check for errors in API string result
 */
static qboolean checkAPIResult(char *result) {
	if (!result) {
		return qfalse;
	}

	if (!Q_stricmp(result, "timeout")) {
		return qfalse;
	}
	if (!Q_stricmp(result, "error")) {
		return qfalse;
	}
	return qtrue;
}

/**
 * Login handler
 */
static void *loginHandler(void *data) {
	int            code;
	int            len          = 0;
	struct query_s *queryStruct = (struct query_s *)data;
	gentity_t      *ent         = queryStruct->ent;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query, sizeof (queryStruct->query));

	len = strlen(queryStruct->result);

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
	G_DPrintf("%s: decreasing threads counter to %d\n", GAME_VERSION, activeThreadsCounter);

	return NULL;
}

/**
 * Login request command
 */
qboolean G_API_login(char *result, gentity_t *ent, char *authToken) {
	char net_port[8] = { 0 };
	char cphysics[8] = { 0 };

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	return G_AsyncAPICall("l", result, ent, 3, authToken, cphysics, net_port);
}

/**
 * Map records handler
 */
static void *mapRecordsHandler(void *data) {
	int            code;
	struct query_s *queryStruct = (struct query_s *)data;
	gentity_t      *ent         = queryStruct->ent;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query, sizeof (queryStruct->query));

	if (code == 0) {
		clientBigDataPrint(ent, queryStruct->result);
	} else {
		CP(va("print \"%s^w: error while requesting records\n\"", GAME_VERSION_COLORED));
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("%s: decreasing threads counter to %d\n", GAME_VERSION, activeThreadsCounter);

	return NULL;
}

/**
 * Map records request command
 */
qboolean G_API_mapRecords(char *result, gentity_t *ent, char *mapName) {
	char net_port[8]         = { 0 };
	char cphysics[8]         = { 0 };
	char encodedMapName[255] = { 0 };

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	if (url_encode(mapName, encodedMapName) == qfalse) {
		return qfalse;
	}

	return G_AsyncAPICall("m", result, ent, 3, encodedMapName, cphysics, net_port);
}

/**
 * Check API handler
 */
static void *checkAPIHandler(void *data) {
	int            code;
	struct query_s *queryStruct = (struct query_s *)data;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query, sizeof (queryStruct->query));

	if (code == 0) {
		G_Printf("%s: %s\n", GAME_VERSION, queryStruct->result);
	} else {
		G_Printf("%s: failed to check API (code: %d, result: %s)\n", GAME_VERSION, code, queryStruct->result);
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("%s: decreasing threads counter to %d\n", GAME_VERSION, activeThreadsCounter);

	return NULL;
}

/**
 * Check API command
 */
qboolean G_API_check(char *result, gentity_t *ent) {
	char net_port[8] = { 0 };
	char cphysics[8] = { 0 };
	char cupMode[8]  = { 0 };
	char cupKey[255] = { 0 };

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);
	sprintf(cupMode, "%d", g_cupMode.integer);

	// Check cupKey value
	sprintf(cupKey, "%s", g_cupKey.string[0] == '\0' ? "0" : g_cupKey.string);

	return G_AsyncAPICall("c", result, ent, 4, cupMode, cupKey, cphysics, net_port);
}

/**
 * Record handler
 */
static void *recordHandler(void *data) {
	int            code;
	struct query_s *queryStruct = (struct query_s *)data;
	gentity_t      *ent         = queryStruct->ent;
	int            timerunNum;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query, sizeof (queryStruct->query));

	timerunNum = ent->client->sess.currentTimerunNum;

	switch (code) {
	case 1001: // PB
		if (ent->client->sess.timerunCheckpointWereLoaded[timerunNum]) {
			memcpy(ent->client->sess.timerunBestCheckpointTimes[timerunNum], ent->client->sess.timerunCheckpointTimes, sizeof (ent->client->sess.timerunCheckpointTimes));
		}
		AP(va("print \"%s^w: %s\n\"", GAME_VERSION_COLORED, queryStruct->result));

		// Nico, keep this demo if autodemo is enabled
		if (ent->client->pers.autoDemo) {
			saveDemo(ent);
		}
		break;

	case 1002: // SB
	case 1003: // SB but player was already rec holder
	case 1004: // SB was tied
		if (ent->client->sess.timerunCheckpointWereLoaded[timerunNum]) {
			memcpy(ent->client->sess.timerunBestCheckpointTimes[timerunNum], ent->client->sess.timerunCheckpointTimes, sizeof (ent->client->sess.timerunCheckpointTimes));
		}
		AP(va("bp \"^w%s\n\"", queryStruct->result));

		// Nico, keep this demo if autodemo is enabled
		if (ent->client->pers.autoDemo) {
			saveDemo(ent);
		}
		break;

	case 1005: // Slow time
		CP(va("print \"%s^w: %s\n\"", GAME_VERSION_COLORED, queryStruct->result));

		// Nico, check player keepDemo setting to see if we keep this one or not
		if (ent->client->pers.autoDemo && ent->client->pers.keepAllDemos) {
			saveDemo(ent);
		}
		break;

	default: // Error
		CP(va("print \"%s^w: error: %s\n\"", GAME_VERSION_COLORED, queryStruct->result));
		break;
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("%s: decreasing threads counter to %d\n", GAME_VERSION, activeThreadsCounter);

	return NULL;
}

/**
 * Record send command
 */
qboolean G_API_sendRecord(char *result, gentity_t *ent, char *mapName, char *runName,
						  char *authToken, char *data, char *etrunVersion) {
	char net_port[8]         = { 0 };
	char cphysics[8]         = { 0 };
	char encodedMapName[255] = { 0 };
	char encodedRunName[255] = { 0 };

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	if (url_encode(mapName, encodedMapName) == qfalse ||
		url_encode(runName, encodedRunName) == qfalse) {
		return qfalse;
	}

	return G_AsyncAPICall("d", result, ent, 7, encodedMapName, encodedRunName, authToken, data, etrunVersion, cphysics, net_port);
}

/**
 * Get checkpoints handler
 */
static void *checkpointsHandler(void *data) {
	int            code;
	struct query_s *queryStruct = (struct query_s *)data;
	gentity_t      *ent         = queryStruct->ent;
	char           *pch         = NULL;
	int            i            = 0;
	int            timerunNum   = 0;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query, sizeof (queryStruct->query));

	if (code >= 1000) {

		timerunNum = code - 1000;

		if (queryStruct->result && checkAPIResult(queryStruct->result) && timerunNum >= 0 && timerunNum < MAX_TIMERUNS) {
			// No error, no timeout

			// Reset client checkpoints
			memset(ent->client->sess.timerunBestCheckpointTimes[timerunNum], 0, sizeof (ent->client->sess.timerunBestCheckpointTimes[timerunNum]));
			ent->client->sess.timerunCheckpointsPassed = 0;

			// Set new checkpoints
			pch = strtok(queryStruct->result, "O");
			while (pch != NULL && i < MAX_TIMERUN_CHECKPOINTS) {
				ent->client->sess.timerunBestCheckpointTimes[timerunNum][i] = atoi(pch);
				pch                                                         = strtok(NULL, "O");
				i++;
			}

			// Mark CP were loaded for this run
			ent->client->sess.timerunCheckpointWereLoaded[timerunNum] = 1;

			CP(va("print \"%s^w: checkpoints loaded for run #%d!\n\"", GAME_VERSION_COLORED, timerunNum));
		} else {
			CP(va("print \"%s^w: error while loading checkpoints\n\"", GAME_VERSION_COLORED));
		}
	} else if (code == 0) {
		clientBigDataPrint(ent, queryStruct->result);
	} else {
		CP(va("print \"%s^w: error while loading checkpoints\n\"", GAME_VERSION_COLORED));
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("%s: decreasing threads counter to %d\n", GAME_VERSION, activeThreadsCounter);

	return NULL;
}

/**
 * Checkpoints request command
 */
qboolean G_API_getPlayerCheckpoints(char *result, gentity_t *ent, char *userName, char *mapName, char *runName, int runNum, char *authToken) {
	char net_port[8]             = { 0 };
	char bufferRunNum[8]         = { 0 };
	char encodedMapName[255]     = { 0 };
	char encodedOptUserName[255] = { 0 };
	char encodedRunName[255]     = { 0 };
	char cphysics[8]             = { 0 };

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(bufferRunNum, "%d", runNum);
	sprintf(cphysics, "%d", physics.integer);

	if (url_encode(mapName, encodedMapName) == qfalse ||
		url_encode(userName, encodedOptUserName) == qfalse ||
		url_encode(runName, encodedRunName) == qfalse) {
		return qfalse;
	}

	// Check authtoken emptiness
	if (authToken[0] == '\0') {
		Q_strncpyz(authToken, "undefined", MAX_QPATH);
	}

	return G_AsyncAPICall("e", result, ent, 7, encodedMapName, encodedOptUserName, encodedRunName, bufferRunNum, authToken, cphysics, net_port);
}

/**
 * Random map handler
 */
static void *randommapHandler(void *data) {
	int            code;
	struct query_s *queryStruct       = (struct query_s *)data;
	gentity_t      *ent               = queryStruct->ent;// Nico, note: this is NULL is randomMap was asked by server (timelimit)
	char           mapfile[MAX_QPATH] = { 0 };
	fileHandle_t   f;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query, sizeof (queryStruct->query));

	if (code == 0 && queryStruct->result && checkAPIResult(queryStruct->result)) {

		Com_sprintf(mapfile, sizeof (mapfile), "maps/%s.bsp", queryStruct->result);

		trap_FS_FOpenFile(mapfile, &f, FS_READ);

		trap_FS_FCloseFile(f);

		if (!f) {
			AP(va("print \"^1Error:^7 the map ^3%s^7 is not on the server.\n\"", queryStruct->result));
		} else {
			// Check from where the map change come
			// randommap vote or timelimit?
			if (g_timelimit.integer > 0) {
				G_delay_map_change(queryStruct->result, g_timelimit.integer);
			} else {
				// Nico, delay the map change
				G_delay_map_change(queryStruct->result, 0);
			}
		}
	} else {
		CP(va("print \"%s^w: error while getting a random map!\n\"", GAME_VERSION_COLORED));
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("%s: decreasing threads counter to %d\n", GAME_VERSION, activeThreadsCounter);

	return NULL;
}

/**
 * Checkpoints request command
 */
qboolean G_API_randommap(char *result, gentity_t *ent, char *mapName) {
	char net_port[8]         = { 0 };
	char encodedMapName[255] = { 0 };
	char cphysics[8]         = { 0 };

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	if (url_encode(mapName, encodedMapName) == qfalse) {
		return qfalse;
	}

	return G_AsyncAPICall("f", result, ent, 3, encodedMapName, cphysics, net_port);
}

/**
 * Map rank command handler
 */
static void *mapRankHandler(void *data) {
	int            code;
	struct query_s *queryStruct = (struct query_s *)data;
	gentity_t      *ent         = queryStruct->ent;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query, sizeof (queryStruct->query));

	if (code == 0) {
		clientBigDataPrint(ent, queryStruct->result);
	} else {
		CP(va("print \"%s^w: error while requesting rank\n\"", GAME_VERSION_COLORED));
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("%s: decreasing threads counter to %d\n", GAME_VERSION, activeThreadsCounter);

	return NULL;
}

/**
 * Function used to check an user input string
 *
 * note: dst must be already allocated and have the required size
 */
static qboolean check_string(char *str) {
	char *pstr = str;

	if (!str) {
		LDE("str is NULL\n");
		return qfalse;
	}

	while (*pstr) {
		if (*pstr == '/') {
			return qfalse;
		}
		pstr++;
	}

	return qtrue;
}

/**
 * Map rank request command
 */
qboolean G_API_mapRank(char *result, gentity_t *ent, char *mapName, char *optUserName, char *optMapName, char *optRunName, char *optPhysicsName, char *authToken) {
	char net_port[8]             = { 0 };
	char cphysics[8]             = { 0 };
	char encodedMapName[255]     = { 0 };
	char encodedOptUserName[255] = { 0 };
	char encodedOptMapName[255]  = { 0 };
	char encodedOptRunName[255]  = { 0 };

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	if (!check_string(optUserName) || !check_string(optMapName) || !check_string(optRunName) || !check_string(optPhysicsName)) {
		return qfalse;
	}

	if (url_encode(mapName, encodedMapName) == qfalse ||
		url_encode(optUserName, encodedOptUserName) == qfalse ||
		url_encode(optMapName, encodedOptMapName) == qfalse ||
		url_encode(optRunName, encodedOptRunName) == qfalse) {
		return qfalse;
	}

	// Check authtoken emptiness
	if (authToken[0] == '\0') {
		Q_strncpyz(authToken, "undefined", MAX_QPATH);
	}

	return G_AsyncAPICall("r", result, ent, 8, encodedOptUserName, encodedOptMapName, encodedOptRunName, optPhysicsName, encodedMapName, authToken, cphysics, net_port);
}

/**
 * Event record handler
 */
static void *eventRecordHandler(void *data) {
	int            code;
	struct query_s *queryStruct = (struct query_s *)data;
	gentity_t      *ent         = queryStruct->ent;
	int            timerunNum;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query, sizeof (queryStruct->query));

	timerunNum = ent->client->sess.currentTimerunNum;

	switch (code) {
	case 1001: // PB
		if (ent->client->sess.timerunCheckpointWereLoaded[timerunNum]) {
			memcpy(ent->client->sess.timerunBestCheckpointTimes[timerunNum], ent->client->sess.timerunCheckpointTimes, sizeof (ent->client->sess.timerunCheckpointTimes));
		}
		CP(va("print \"%s^w: %s\n\"", GAME_VERSION_COLORED, queryStruct->result));

		// Nico, keep this demo if autodemo is enabled
		if (ent->client->pers.autoDemo) {
			saveDemo(ent);
		}
		break;

	case 1005: // Slow time
		CP(va("print \"%s^w: %s\n\"", GAME_VERSION_COLORED, queryStruct->result));

		// Nico, check player keepDemo setting to see if we keep this one or not
		if (ent->client->pers.autoDemo && ent->client->pers.keepAllDemos) {
			saveDemo(ent);
		}
		break;

	default: // Error
		CP(va("print \"%s^w: error: %s\n\"", GAME_VERSION_COLORED, queryStruct->result));
		break;
	}

	free(queryStruct->result);
	free(queryStruct);

	// Decrease global active thread counter
	activeThreadsCounter--;
	G_DPrintf("%s: decreasing threads counter to %d\n", GAME_VERSION, activeThreadsCounter);

	return NULL;
}

/**
 * Event record send command
 */
qboolean G_API_sendEventRecord(char *result, gentity_t *ent, char *mapName, char *runName,
							   char *authToken, char *data, char *etrunVersion) {
	char net_port[8]         = { 0 };
	char cphysics[8]         = { 0 };
	char encodedMapName[255] = { 0 };
	char encodedRunName[255] = { 0 };
	char cupKey[255]		 = { 0 };

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	if (url_encode(mapName, encodedMapName) == qfalse ||
		url_encode(runName, encodedRunName) == qfalse) {
		return qfalse;
	}

	// Check cupKey value
	sprintf(cupKey, "%s", g_cupKey.string[0] == '\0' ? "0" : g_cupKey.string);

	return G_AsyncAPICall("v", result, ent, 8, cupKey, encodedMapName, encodedRunName, authToken, data, etrunVersion, cphysics, net_port);
}

/**
 * Get config handler
 */
static void *getConfigHandler(void *data) {
	int            code;
	struct query_s *queryStruct = (struct query_s *)data;
	int	config_strictSaveLoad;
	int	config_physics;
	int	config_disableDrowning;
	int	config_holdDoorsOpen;
	int	config_enableMapEntities;
	int config_script_size;
	int len;

	code = API_query(queryStruct->cmd, queryStruct->result, queryStruct->query, sizeof (queryStruct->query));

	if (code != 0) {
		G_Error("%s: error #1 while getting config from API!\n", GAME_VERSION);
	}

	code = sscanf(queryStruct->result, "%d %d %d %d %d %d %*s",// Nico, last field is ignored for the moment
		&config_strictSaveLoad,
		&config_physics,
		&config_disableDrowning,
		&config_holdDoorsOpen,
		&config_enableMapEntities,
		&config_script_size);

	if (code != 6) {
		G_Error("%s: error #2 while getting config from API!\n", GAME_VERSION);
	}

	if (config_script_size != 0) {
		level.useAPImapscript = qtrue;

		level.scriptEntity = G_Alloc(config_script_size + 1);
		code = sscanf(queryStruct->result, "%*d %*d %*d %*d %*d %*d %512000c", level.scriptEntity);

		if (code != 1) {
			G_Error("%s: error #3 while getting config from API!\n", GAME_VERSION);
		}
		*(level.scriptEntity + config_script_size) = '\0';

		// Nico, check script len
		len = strlen(level.scriptEntity);
		if (len != config_script_size) {
			G_Error("%s: error #4 while getting config from API (%d != %d)!\n", GAME_VERSION, config_script_size, len);
		}

		// Nico, do the same as G_Script_ScriptLoad do when loading from a local file
		trap_Cvar_Set("g_scriptName", "");
		G_Script_EventStringInit();
	}

	// Nico, check cvars from API
	if (config_strictSaveLoad != g_strictSaveLoad.integer) {
		G_Printf("%s: updating g_strictSaveLoad from %d to %d\n", GAME_VERSION, g_strictSaveLoad.integer, config_strictSaveLoad);
		trap_Cvar_Set("g_strictSaveLoad", va("%d", config_strictSaveLoad));
	}
	if (config_physics != physics.integer) {
		G_DPrintf("%s: updating physics from %d to %d\n", GAME_VERSION, physics.integer, config_physics);
		trap_Cvar_Set("physics", va("%d", config_physics));
	}
	if (config_disableDrowning != g_disableDrowning.integer) {
		G_DPrintf("%s: updating g_disableDrowning from %d to %d\n", GAME_VERSION, g_disableDrowning.integer, config_disableDrowning);
		// Nico, update the cvar
		g_disableDrowning.integer = config_disableDrowning;
		g_disableDrowning.modificationCount++;
		trap_Cvar_Set("g_disableDrowning", va("%d", config_disableDrowning));
	}
	if (config_holdDoorsOpen != g_holdDoorsOpen.integer) {
		G_DPrintf("%s: updating g_holdDoorsOpen from %d to %d\n", GAME_VERSION, g_holdDoorsOpen.integer, config_holdDoorsOpen);
		// Nico, update this way to make sure the cvar value is updated earlier enough when map elements get spawned
		g_holdDoorsOpen.integer = config_holdDoorsOpen;
		g_holdDoorsOpen.modificationCount++;
		trap_Cvar_Set("g_holdDoorsOpen", va("%d", config_holdDoorsOpen));
	}

	if (config_enableMapEntities != g_enableMapEntities.integer) {
		G_Printf("%s: updating g_enableMapEntities from %d to %d\n", GAME_VERSION, g_enableMapEntities.integer, config_enableMapEntities);
		// Nico, update this way to make sure the cvar value is updated earlier enough when map elements get spawned
		g_enableMapEntities.integer = config_enableMapEntities;
		g_enableMapEntities.modificationCount++;
		trap_Cvar_Set("g_enableMapEntities", va("%d", config_enableMapEntities));
	}

	free(queryStruct->result);
	free(queryStruct);

	return NULL;
}

/**
 * Get config command
 */
qboolean G_API_getConfig(void) {
	char *buf				 = NULL;
	char net_port[8]         = { 0 };
	char cphysics[8]         = { 0 };
	char encodedMapName[255] = { 0 };

	buf = malloc(LARGE_RESPONSE_MAX_SIZE * sizeof (char));

	if (!buf) {
		LDE("failed to allocate memory\n");
		return qfalse;
	}

	sprintf(net_port, "%d", trap_Cvar_VariableIntegerValue("net_port"));
	sprintf(cphysics, "%d", physics.integer);

	if (url_encode(level.rawmapname, encodedMapName) == qfalse) {
		return qfalse;
	}

	return G_SyncAPICall("o", buf, NULL, 4, encodedMapName, GAME_VERSION_DATED, cphysics, net_port);
}

// Commands handler binding
static const api_glue_t APICommands[] =
{
	{ "l", loginHandler       },
	{ "m", mapRecordsHandler  },
	{ "c", checkAPIHandler    },
	{ "d", recordHandler      },
	{ "e", checkpointsHandler },
	{ "f", randommapHandler   },
	{ "r", mapRankHandler     },
	{ "v", eventRecordHandler },
	{ "o", getConfigHandler   }
};

/**
 * Takes a command string as argument and returns the associated handler if any, NULL otherwise
 */
static handler_t getHandlerForCommand(char *cmd) {
	unsigned int     i, cCommands = sizeof (APICommands) / sizeof (APICommands[0]);
	const api_glue_t *element;

	if (!cmd) {
		return NULL;
	}

	for (i = 0; i < cCommands; ++i) {
		element = &APICommands[i];
		if (element && element->cmd && !Q_stricmp(cmd, element->cmd)) {
			return element->handler;
		}
	}
	return NULL;
}

/**
 * Asynchronous (using pthreads) API call function
 *
 * command: must be a command in apiCommands
 * result: pointer to an *already allocated* buffer for storing result
 * ent: entity who made the request
 * count: number of variadic arguments
 */
qboolean G_AsyncAPICall(char *command, char *result, gentity_t *ent, int count, ...) {
	struct query_s *queryStruct;
	pthread_t      thread;
	pthread_attr_t attr;
	int            returnCode = 0;
	void    *(*handler)(void *) = NULL;
	va_list ap;
	int     i    = 0;
	char    *arg = NULL;

	if (api_module == NULL || API_query == NULL) {
		LDE("API module is not loaded\n");
		return qfalse;
	}

	// Check if thread limit is reached
	if (activeThreadsCounter >= THREADS_MAX) {
		LDE("threads limit (%d) reached: %d\n", THREADS_MAX, activeThreadsCounter);
		return qfalse;
	}

	// Check number of arguments in ... (=count)
	if (count <= 0) {
		LDE("invalid argument count %d\n", count);
		return qfalse;
	}

	queryStruct = malloc(sizeof (struct query_s));

	if (queryStruct == NULL) {
		LDE("failed to allocate memory\n");
		return qfalse;
	}

	va_start(ap, count);

	// Init query buffer
	memset(queryStruct->query, 0, QUERY_MAX_SIZE);

	for (i = 0; i < count; ++i) {
		arg = va_arg(ap, char *);

		if (!arg) {
			LDE("empty argument %d with command '%s'\n", i, command);
			free(queryStruct);
			va_end(ap);
			return qfalse;
		}

		Q_strcat(queryStruct->query, QUERY_MAX_SIZE, arg);

		// No trailing /
		if (i + 1 < count) {
			Q_strcat(queryStruct->query, QUERY_MAX_SIZE, CHAR_SEPARATOR);
		}
	}

	va_end(ap);

	Q_strncpyz(queryStruct->cmd, command, sizeof (queryStruct->cmd));
	queryStruct->result = result;
	queryStruct->ent    = ent;

	// Get the command handler
	handler = getHandlerForCommand(command);

	if (!handler) {
		LDE("no handler for command '%s'\n", command);
		free(queryStruct);
		return qfalse;
	}

	LDI("asynchronous API call with command: '%s', query '%s'\n", command, queryStruct->query);

	// Create threads as detached as they will never be joined
	if (pthread_attr_init(&attr)) {
		LDE("error in pthread_attr_init\n");
		free(queryStruct);
		return qfalse;
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		LDE("error in pthread_attr_setdetachstate\n");
		free(queryStruct);
		return qfalse;
	}

	returnCode = pthread_create(&thread, &attr, handler, (void *)queryStruct);

	if (returnCode) {
		LDE("error in pthread_create, return value is %d\n", returnCode);
		free(queryStruct);
		return qfalse;
	}

	// Nico, increase active threads counter
	activeThreadsCounter++;
	G_DPrintf("%s: increasing threads counter to %d\n", GAME_VERSION, activeThreadsCounter);

	if (pthread_attr_destroy(&attr)) {
		LDE("error in pthread_attr_destroy\n");
		// Nico, note: I don't free querystruct because it's used in the thread
		return qfalse;
	}

	return qtrue;
}

/**
 * Synchronous API call function
 *
 * command: must be a command in apiCommands
 * result: pointer to an *already allocated* buffer for storing result
 * ent: entity who made the request
 * count: number of variadic arguments
 */
qboolean G_SyncAPICall(char *command, char *result, gentity_t *ent, int count, ...) {
	struct query_s *queryStruct;
	void    *(*handler)(void *) = NULL;
	va_list ap;
	int     i    = 0;
	char    *arg = NULL;

	if (api_module == NULL || API_query == NULL) {
		LDE("API module is not loaded\n");
		return qfalse;
	}

	// Check number of arguments in ... (=count)
	if (count <= 0) {
		LDE("invalid argument count %d\n", count);
		return qfalse;
	}

	queryStruct = malloc(sizeof (struct query_s));

	if (queryStruct == NULL) {
		LDE("failed to allocate memory\n");
		return qfalse;
	}

	va_start(ap, count);

	// Init query buffer
	memset(queryStruct->query, 0, QUERY_MAX_SIZE);

	for (i = 0; i < count; ++i) {
		arg = va_arg(ap, char *);

		if (!arg) {
			LDE("empty argument %d with command '%s'\n", i, command);
			free(queryStruct);
			va_end(ap);
			return qfalse;
		}

		Q_strcat(queryStruct->query, QUERY_MAX_SIZE, arg);

		// No trailing /
		if (i + 1 < count) {
			Q_strcat(queryStruct->query, QUERY_MAX_SIZE, CHAR_SEPARATOR);
		}
	}

	va_end(ap);

	Q_strncpyz(queryStruct->cmd, command, sizeof (queryStruct->cmd));
	queryStruct->result = result;
	queryStruct->ent    = ent;

	// Get the command handler
	handler = getHandlerForCommand(command);

	if (!handler) {
		LDE("no handler for command '%s'\n", command);
		free(queryStruct);
		return qfalse;
	}

	LDI("synchronous API call with command: '%s', query '%s'\n", command, queryStruct->query);

	handler(queryStruct);

	return qtrue;
}

void G_loadAPI() {
	char homepath[512];
	char basepath[512];

	trap_Cvar_VariableStringBuffer("fs_homepath", homepath, sizeof (homepath));
	trap_Cvar_VariableStringBuffer("fs_basepath", basepath, sizeof (basepath));

	// Load the module
	if (!loadModule(basepath, homepath)) {
		printError();
		G_Error("Error loading %s\n", g_APImoduleName.string);
	}

	// Load the APIquery function
	if (!loadAPISymbols()) {
		printError();
		G_Error("Error loading symbols from %s\n", g_APImoduleName.string);
	}

	if (API_init(homepath, basepath, g_APImoduleName.string) != 0) {
		G_Error("Error calling API_init()");
	}

	G_Printf("%s: API module loaded!\n", GAME_VERSION);
}

void G_unloadAPI() {
	if (api_module == NULL) {
		G_DPrintf("%s: API module is not loaded (G_unloadAPI).\n", GAME_VERSION);
	} else {

		API_clean();

#if defined _WIN32
		FreeLibrary(api_module);
#else
		dlclose(api_module);
#endif

		G_Printf("%s: API module unloaded!\n", GAME_VERSION);
	}
}
