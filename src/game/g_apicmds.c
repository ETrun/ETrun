#include "g_local.h"
#include "g_api.h"

// Nico, ETrun login command
void Cmd_Login_f(gentity_t *ent) {
	char token[MAX_QPATH];
	char *result = NULL;
	int  i       = 0;

	// Check if API is used
	if (!g_useAPI.integer) {
		CP("cp \"Login is disabled on this server.\n\"");
		return;
	}

	if (!ent || !ent->client) {
		G_DPrintf("%s: Cmd_Login_f, invalid ent\n", GAME_VERSION);
		return;
	}

	// Check if already logged in
	if (ent->client->sess.logged) {
		CP("cp \"You are already logged in!\n\"");
		G_DPrintf("%s: Cmd_Login_f, client is already logged in\n", GAME_VERSION);
		return;
	}

	// Nico, reset saves
	for (i = 0; i < MAX_SAVED_POSITIONS; ++i) {
		ent->client->sess.alliesSaves[i].valid = qfalse;
		ent->client->sess.axisSaves[i].valid   = qfalse;
	}

	// Nico, kill player
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);
	}

	result = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!result) {
		LDE("%s\n", "failed to allocate memory");

		return;
	}

	Q_strncpyz(token, ent->client->pers.authToken, MAX_QPATH);

	if (token[0] == '\0') {
		CP("cp \"Empty auth token!\n\"");
		G_DPrintf("%s: Cmd_Login_f, empty_token\n", GAME_VERSION);
		free(result);
	} else {
		if (!G_API_login(result, ent, token)) {
			CP(va("print \"%s^w: error while login\n\"", GAME_VERSION_COLORED));
		}
		// Do not free result here!
	}
}

// Nico, ETrun logout command
void Cmd_Logout_f(gentity_t *ent) {
	if (!ent || !ent->client) {
		G_DPrintf("%s: Cmd_Logout_f, invalid ent!\n", GAME_VERSION);
		return;
	}

	// Check if already logged in
	if (!ent->client->sess.logged) {
		CP("cp \"You are not logged in!\n\"");
		return;
	}

	CP("cp \"You are no longer logged in!\n\"");
	ent->client->sess.logged = qfalse;
	ClientUserinfoChanged(ent->client->ps.clientNum);

	// Notify client that he is now logged out
	trap_SendServerCommand(ent - g_entities, "client_logout");
}

// Nico, records command
void Cmd_Records_f(gentity_t *ent) {
	char *buf = NULL;

	// Check if API is used
	if (!g_useAPI.integer) {
		CP("cp \"This command is disabled on this server.\n\"");
		return;
	}

	// Check cup mode
	if (g_cupMode.integer != 0) {
		CP("cp \"This command is disabled in cup mode.\n\"");
		return;
	}

	buf = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!buf) {
		LDE("%s\n", "failed to allocate memory");

		return;
	}

	if (!G_API_mapRecords(buf, ent, level.rawmapname)) {
		CP(va("print \"%s^w: error while requesting records\n\"", GAME_VERSION_COLORED));
	}

	// Do *not* free buf here
}

// Nico, load checkpoints command
void Cmd_LoadCheckpoints_f(gentity_t *ent) {
	int  argc                = 0;
	char userName[MAX_QPATH] = { 0 };
	char runName[MAX_QPATH]  = { 0 };
	int  runNum              = -1;
	int  i                   = 0;

	// Check if level is timerun
	if (!level.isTimerun) {
		CP("cp \"There is no timerun on this map.\n\"");
		return;
	}

	// Check if API is used
	if (!g_useAPI.integer) {
		CP("cp \"This command is disabled on this server.\n\"");
		return;
	}

	// Check if client is logged in
	if (!ent->client->sess.logged) {
		CP("cp \"You must login to use this command.\n\"");
		return;
	}

	// Check cup mode
	if (g_cupMode.integer != 0) {
		CP("cp \"This command is disabled in cup mode.\n\"");
		return;
	}

	// Parse options
	argc = trap_Argc();

	if (argc == 1 || argc > 2) {
		CP("print \"\n  ^8Usage: loadCheckpoints [userName] [run name or id]\n\"");
		CP("print \"  ^8Available runs:\n\"");
		for (i = 0; i < MAX_TIMERUNS; ++i) {
			if (level.timerunsNames[i]) {
				CP(va("print \"  ^8#%d => %s\n\"", i, level.timerunsNames[i]));
			}
		}
		runNum = 0;
		CP("print \"  ^8No run specified, loading your own checkpoints for run #0...\n\n\"");
	}

	if (argc > 1) {
		trap_Argv(1, userName, sizeof (userName));
	}
	if (userName[0] == '\0') {
		sprintf(userName, "0");
	}

	if (argc > 2) {
		trap_Argv(2, runName, sizeof (runName));
		// Find by run name
		for (i = 0; i < MAX_TIMERUNS; ++i) {
			if (!Q_stricmp(level.timerunsNames[i], runName)) {
				runNum = i;
				break;
			}
		}
	}
	if (runNum == -1) { // Not found by name
		runNum = atoi(runName);
		if (runNum < 0 || runNum >= MAX_TIMERUNS || !level.timerunsNames[runNum]) {
			runNum = 0;
		}
	}

	Cmd_LoadCheckpoints_real(ent, userName, runNum);
}

// Nico, load checkpoints command
// Does not check anything, it's caller responsability to check:
// level is timerun
// API is used
// player login status
void Cmd_LoadCheckpoints_real(gentity_t *ent, char *userName, int runNum) {
	char *buf;

	buf = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!buf) {
		LDE("%s\n", "failed to allocate memory");

		return;
	}

	if (!G_API_getPlayerCheckpoints(buf, ent, userName, level.rawmapname, level.timerunsNames[runNum], runNum, ent->client->pers.authToken)) {
		CP(va("print \"%s^w: error while requesting checkpoints\n\"", GAME_VERSION_COLORED));
	}

	// Do *not* free buf here
}

// Nico, rank command
// /rank [userName] [mapName] [runName] [physicsName]
void Cmd_Rank_f(gentity_t *ent) {
	char *buf                   = NULL;
	char userName[MAX_QPATH]    = { 0 };
	char mapName[MAX_QPATH]     = { 0 };
	char runName[MAX_QPATH]     = { 0 };
	char physicsName[MAX_QPATH] = { 0 };
	int  argc                   = 0;

	// Check if API is used
	if (!g_useAPI.integer) {
		CP("cp \"This command is disabled on this server.\n\"");
		return;
	}

	// Check cup mode
	if (g_cupMode.integer != 0) {
		CP("cp \"This command is disabled in cup mode.\n\"");
		return;
	}

	// Parse options
	argc = trap_Argc();
	if (argc >= 1) { // Nico, #fixme, why >= ?
		trap_Argv(1, userName, sizeof (userName));
	}
	if (userName[0] == '\0') {
		sprintf(userName, "0");
	}

	if (argc >= 2) {
		trap_Argv(2, mapName, sizeof (mapName));
	}
	if (mapName[0] == '\0') {
		sprintf(mapName, "0");
	}

	if (argc >= 3) {
		trap_Argv(3, runName, sizeof (runName));
	}
	if (runName[0] == '\0') {
		sprintf(runName, "0");
	}

	if (argc >= 4) {
		trap_Argv(4, physicsName, sizeof (physicsName));
	}
	if (physicsName[0] == '\0') {
		sprintf(physicsName, "0");
	}

	buf = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!buf) {
		LDE("%s\n", "failed to allocate memory");

		return;
	}

	// Parse options

	if (!G_API_mapRank(buf, ent, level.rawmapname, userName, mapName, runName, physicsName, ent->client->pers.authToken)) {
		CP(va("print \"%s^w: error while requesting rank\n\"", GAME_VERSION_COLORED));
	}

	// Do *not* free buf here
}
