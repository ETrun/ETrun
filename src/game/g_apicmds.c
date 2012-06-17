#include "g_local.h"
#include "g_api.h"

// Nico, ETrun login command
void Cmd_Login_f(gentity_t *ent) {
	char token[MAX_QPATH];
	char *result = NULL;
	int i = 0;

	// Check if API is used
	if (!g_useAPI.integer) {
		CP("cp \"Login is disabled on this server.\n\"");
		return;
	}

	if (!ent || !ent->client) {
		G_DPrintf("Cmd_Login_f: invalid ent: %d\n", (int)ent);
		return;
	}

	// Check if already logged in
	if (ent->client->sess.logged) {
		CP("cp \"You are already logged in!\n\"");
		G_DPrintf("Cmd_Login_f: client already logged in\n");
		return;
	}

	// Nico, reset saves
	for (i = 0; i < MAX_SAVED_POSITIONS; ++i) {
		ent->client->sess.alliesSaves[i].valid = qfalse;
		ent->client->sess.axisSaves[i].valid = qfalse;
	}

	// Nico, kill player
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		player_die(ent, ent, ent, 100000, MOD_SUICIDE);
	}

	result = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!result) {
		G_Error("Cmd_Login_f: malloc failed\n");
	}

	Q_strncpyz(token, ent->client->pers.authToken, MAX_QPATH);

	if (strlen(token) == 0) {
		CP("cp \"Empty auth token!\n\"");
		G_DPrintf("Cmd_Login_f: empty_token\n");
		free(result);
	} else {
		G_API_login(result, ent, token);
		// Do not free result here!
	}
}

// Nico, ETrun logout command
void Cmd_Logout_f(gentity_t *ent) {
	if (!ent || !ent->client) {
		G_DPrintf("Cmd_Login_f: invalid ent: %d\n", (int)ent);
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
}

// Nico, records command
void Cmd_Records_f(gentity_t *ent) {
	char *buf = NULL;

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

	buf = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!buf) {
		G_Error("Cmd_Records_f: malloc failed\n");
	}

	G_API_mapRecords(buf, ent, level.rawmapname);

	// Do *not* free buf here
}

// Nico, load checkpoints command
void Cmd_LoadCheckpoints_f(gentity_t *ent) {
	char *buf = NULL;
	int argc = 0;
	int runNum = -1;
	char arg[MAX_QPATH] = {0};
	int i = 0;

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

	argc = trap_Argc();

	if (argc == 2) {
		trap_Argv(1, arg, sizeof(arg));

		// Find by run name
		for (i = 0; i < MAX_TIMERUNS; ++i) {
			if (!Q_stricmp(level.timerunsNames[i], arg)) {
				runNum = i;
				break;
			}
		}

		if (runNum == -1) {// Not found by name
			runNum = atoi(arg);
			if (runNum < 0 || runNum >= MAX_TIMERUNS || !level.timerunsNames[runNum]) {
				runNum = 0;
			}
		}
	} else {
		CP("print \"^1> ^wUsage: loadCheckpoints [run name or id]\n\"");
		CP("print \"^1> ^wAvailable runs:\n\"");
		for (i = 0; i < MAX_TIMERUNS; ++i) {
			if (level.timerunsNames[i]) {
				CP(va("print \"^1> ^w #%d => %s\n\"", i, level.timerunsNames[i]));
			}
		}
		runNum = 0;
		CP("print \"^1> ^wNo run specified, loading checkpoints for run #0...\n\"");
	}

	buf = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!buf) {
		G_Error("Cmd_LoadCheckpoints_f: malloc failed\n");
	}

	G_API_getPlayerCheckpoints(buf, ent, level.rawmapname, level.timerunsNames[runNum], runNum, ent->client->pers.authToken);

	// Do *not* free buf here
}