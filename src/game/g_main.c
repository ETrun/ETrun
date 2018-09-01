/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "g_local.h"
#include "g_api.h"

// Nico, active threads counter
int activeThreadsCounter;

// Nico, global threads
#define DELAYED_MAP_CHANGE_THREAD_ID 0
pthread_t globalThreads[4];

// Nico, this var help to prevent new thread from being spawned
// when we don't want too. i.e. when we are changing map
qboolean threadingAllowed;

level_locals_t level;

typedef struct {
	vmCvar_t *vmCvar;
	char *cvarName;
	char *defaultString;
	int cvarFlags;
	int modificationCount;          // for tracking changes
	qboolean trackChange;           // track this variable, and announce if changed
	qboolean fConfigReset;          // OSP: set this var to the default on a config reset
} cvarTable_t;

gentity_t g_entities[MAX_GENTITIES];
gclient_t g_clients[MAX_CLIENTS];

g_campaignInfo_t g_campaigns[MAX_CAMPAIGNS];

mapEntityData_Team_t mapEntityData[2];

vmCvar_t g_password;
vmCvar_t sv_privatepassword;
vmCvar_t g_maxclients;
vmCvar_t g_dedicated;
vmCvar_t g_inactivity;
vmCvar_t g_debugMove;
vmCvar_t g_motd;
vmCvar_t g_gamestate;
vmCvar_t g_gametype;
// -NERVE - SMF

vmCvar_t g_restarted;
vmCvar_t g_log;
vmCvar_t g_logSync;
vmCvar_t voteFlags;
vmCvar_t g_filtercams;
vmCvar_t g_voiceChatsAllowed;       // DHM - Nerve
vmCvar_t g_needpass;
vmCvar_t g_banIPs;
vmCvar_t g_filterBan;
vmCvar_t g_smoothClients;
vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;

// Rafael
vmCvar_t g_scriptName;          // name of script file to run (instead of default for that map)

vmCvar_t g_developer;
vmCvar_t g_footstepAudibleRange;

// charge times for character class special weapons
vmCvar_t g_medicChargeTime;
vmCvar_t g_engineerChargeTime;
vmCvar_t g_LTChargeTime;
vmCvar_t g_soldierChargeTime;
// screen shakey magnitude multiplier

// Gordon
vmCvar_t g_antilag;

// OSP
vmCvar_t g_spectatorInactivity;
vmCvar_t team_maxPanzers;
vmCvar_t team_maxplayers;
vmCvar_t team_nocontrols;
vmCvar_t server_motd0;
vmCvar_t server_motd1;
vmCvar_t server_motd2;
vmCvar_t server_motd3;
vmCvar_t server_motd4;
vmCvar_t server_motd5;
vmCvar_t vote_allow_kick;
vmCvar_t vote_allow_map;
vmCvar_t vote_allow_randommap;
vmCvar_t vote_allow_referee;
vmCvar_t vote_allow_antilag;
vmCvar_t vote_allow_muting;
vmCvar_t vote_limit;
vmCvar_t vote_delay;
vmCvar_t vote_percent;
vmCvar_t g_covertopsChargeTime;
vmCvar_t refereePassword;

// Variable for setting the current level of debug printing/logging
// enabled in bot scripts and regular scripts.
// Added by Mad Doctor I, 8/23/2002
vmCvar_t g_scriptDebugLevel;

// Nico, beginning of ETrun server cvars

// Max connections per IP
vmCvar_t g_maxConnsPerIP;

// Game physics
vmCvar_t physics;

// Enable certain map entities
vmCvar_t g_enableMapEntities;

// Force timer reset, i.e. bypass "wait 9999" on start triggers
vmCvar_t g_forceTimerReset;

// Is level a timerun?
vmCvar_t isTimerun;

// Flood protection
vmCvar_t g_floodProtect;
vmCvar_t g_floodThreshold;
vmCvar_t g_floodWait;

// Name changes limit
vmCvar_t g_maxNameChanges;

// API module
vmCvar_t g_useAPI;
vmCvar_t g_APImoduleName;

// Hold doors open
vmCvar_t g_holdDoorsOpen;

// Disable drowning
vmCvar_t g_disableDrowning;

// Mapscript support
vmCvar_t g_mapScriptDirectory;

// Cup mode
vmCvar_t g_cupMode;
vmCvar_t g_cupKey;

// Timelimit mode
vmCvar_t g_timelimit;

// Logging
vmCvar_t g_debugLog;
vmCvar_t g_chatLog;

// GeoIP
vmCvar_t g_useGeoIP;
vmCvar_t g_geoIPDbPath;

// Strict save/load
vmCvar_t g_strictSaveLoad;

// Nico, end of ETrun cvars

cvarTable_t gameCvarTable[] =
{
	// noset vars
	{ NULL,                    "gamename",               GAME_VERSION,                 CVAR_SERVERINFO | CVAR_ROM,                                    0,      qfalse, qfalse },
	{ NULL,                    "gamedate",               __DATE__,                     CVAR_ROM,                                                      0,      qfalse, qfalse },
	{ &g_restarted,            "g_restarted",            "0",                          CVAR_ROM,                                                      0,      qfalse, qfalse },
	{ NULL,                    "sv_mapname",             "",                           CVAR_SERVERINFO | CVAR_ROM,                                    0,      qfalse, qfalse },

	// latched vars

	{ &g_medicChargeTime,      "g_medicChargeTime",      "45000",                      CVAR_SERVERINFO | CVAR_LATCH,                                  0,      qfalse, qtrue  },
	{ &g_engineerChargeTime,   "g_engineerChargeTime",   "30000",                      CVAR_SERVERINFO | CVAR_LATCH,                                  0,      qfalse, qtrue  },
	{ &g_LTChargeTime,         "g_LTChargeTime",         "40000",                      CVAR_SERVERINFO | CVAR_LATCH,                                  0,      qfalse, qtrue  },
	{ &g_soldierChargeTime,    "g_soldierChargeTime",    "20000",                      CVAR_SERVERINFO | CVAR_LATCH,                                  0,      qfalse, qtrue  },

	{ &g_covertopsChargeTime,  "g_covertopsChargeTime",  "30000",                      CVAR_SERVERINFO | CVAR_LATCH,                                  0,      qfalse, qtrue  },
	{ &g_maxclients,           "sv_maxclients",          "20",                         CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE,                   0,      qfalse, qfalse }, // NERVE - SMF - made 20 from 8

	{ &g_gamestate,            "gamestate",              "-1",                         CVAR_WOLFINFO | CVAR_ROM,                                      0,      qfalse, qfalse },
	{ &g_gametype,             "g_gametype",             "2",                          CVAR_SERVERINFO | CVAR_ROM,                                    0,      qfalse, qfalse },
	// -NERVE - SMF

	{ &g_log,                  "g_log",                  "",                           CVAR_ARCHIVE,                                                  0,      qfalse, qfalse },
	{ &g_logSync,              "g_logSync",              "0",                          CVAR_ARCHIVE,                                                  0,      qfalse, qfalse },

	{ &g_password,             "g_password",             "none",                       CVAR_USERINFO,                                                 0,      qfalse, qfalse },
	{ &sv_privatepassword,     "sv_privatepassword",     "",                           CVAR_TEMP,                                                     0,      qfalse, qfalse },
	{ &g_banIPs,               "g_banIPs",               "",                           CVAR_ARCHIVE,                                                  0,      qfalse, qfalse },
	// show_bug.cgi?id=500
	{ &g_filterBan,            "g_filterBan",            "1",                          CVAR_ARCHIVE,                                                  0,      qfalse, qfalse },

	{ &g_dedicated,            "dedicated",              "0",                          0,                                                             0,      qfalse, qfalse },

	{ &g_needpass,             "g_needpass",             "0",                          CVAR_SERVERINFO | CVAR_ROM,                                    0,      qtrue,  qfalse },

	{ &g_inactivity,           "g_inactivity",           "120",                        0,                                                             0,      qtrue,  qfalse }, // Nico, set to 120 secs (default was 0)
	{ &g_debugMove,            "g_debugMove",            "0",                          0,                                                             0,      qfalse, qfalse },
	{ &g_motd,                 "g_motd",                 "",                           CVAR_ARCHIVE,                                                  0,      qfalse, qfalse },
	{ &voteFlags,              "voteFlags",              "0",                          CVAR_TEMP | CVAR_ROM | CVAR_SERVERINFO,                        0,      qfalse, qfalse },
	{ &g_filtercams,           "g_filtercams",           "0",                          CVAR_ARCHIVE,                                                  0,      qfalse, qfalse },
	{ &g_voiceChatsAllowed,    "g_voiceChatsAllowed",    "4",                          CVAR_ARCHIVE,                                                  0,      qfalse, qfalse }, // DHM - Nerve
	{ &g_developer,            "developer",              "0",                          CVAR_TEMP,                                                     0,      qfalse, qfalse },
	{ &g_smoothClients,        "g_smoothClients",        "1",                          0,                                                             0,      qfalse, qfalse },
	{ &pmove_fixed,            "pmove_fixed",            "0",                          0,                                                             0,      qfalse, qfalse }, // Nico, removed CVAR_SYSTEMINFO
	{ &pmove_msec,             "pmove_msec",             "8",                          CVAR_SYSTEMINFO,                                               0,      qfalse, qfalse },

	{ &g_footstepAudibleRange, "g_footstepAudibleRange", "256",                        CVAR_CHEAT,                                                    0,      qfalse, qfalse },

	{ &g_scriptName,           "g_scriptName",           "",                           CVAR_CHEAT,                                                    0,      qfalse, qfalse },

	{ &g_antilag,              "g_antilag",              "1",                          CVAR_SERVERINFO | CVAR_ARCHIVE,                                0,      qfalse, qfalse },

	//bani - #184
	{ NULL,                    "P",                      "",                           CVAR_SERVERINFO_NOUPDATE,                                      0,      qfalse, qfalse },

	{ &refereePassword,        "refereePassword",        "none",                       0,                                                             0,      qfalse, qfalse },
	{ &g_spectatorInactivity,  "g_spectatorInactivity",  "0",                          0,                                                             0,      qfalse, qfalse },

	{ &server_motd0,           "server_motd0",           " ^NEnemy Territory ^7MOTD ", 0,                                                             0,      qfalse, qfalse },
	{ &server_motd1,           "server_motd1",           "",                           0,                                                             0,      qfalse, qfalse },
	{ &server_motd2,           "server_motd2",           "",                           0,                                                             0,      qfalse, qfalse },
	{ &server_motd3,           "server_motd3",           "",                           0,                                                             0,      qfalse, qfalse },
	{ &server_motd4,           "server_motd4",           "",                           0,                                                             0,      qfalse, qfalse },
	{ &server_motd5,           "server_motd5",           "",                           0,                                                             0,      qfalse, qfalse },
	{ &team_maxPanzers,        "team_maxPanzers",        "-1",                         0,                                                             0,      qfalse, qfalse },
	{ &team_maxplayers,        "team_maxplayers",        "0",                          0,                                                             0,      qfalse, qfalse },
	{ &team_nocontrols,        "team_nocontrols",        "1",                          0,                                                             0,      qfalse, qfalse },

	{ &vote_allow_kick,        "vote_allow_kick",        "0",                          0,                                                             0,      qfalse, qfalse },
	{ &vote_allow_map,         "vote_allow_map",         "1",                          0,                                                             0,      qfalse, qfalse },
	{ &vote_allow_randommap,   "vote_allow_randommap",   "1",                          0,                                                             0,      qfalse, qfalse },
	{ &vote_allow_referee,     "vote_allow_referee",     "0",                          0,                                                             0,      qfalse, qfalse },
	{ &vote_allow_antilag,     "vote_allow_antilag",     "0",                          0,                                                             0,      qfalse, qfalse },
	{ &vote_allow_muting,      "vote_allow_muting",      "0",                          0,                                                             0,      qfalse, qfalse },
	{ &vote_limit,             "vote_limit",             "5",                          0,                                                             0,      qfalse, qfalse },
	{ &vote_delay,             "vote_delay",             "20000",                      0,                                                             0,      qfalse, qfalse },
	{ &vote_percent,           "vote_percent",           "50",                         0,                                                             0,      qfalse, qfalse },

	// state vars
	{ &g_scriptDebug,          "g_scriptDebug",          "0",                          CVAR_CHEAT,                                                    0,      qfalse, qfalse },

	// What level of detail do we want script printing to go to.
	{ &g_scriptDebugLevel,     "g_scriptDebugLevel",     "0",                          CVAR_CHEAT,                                                    0,      qfalse, qfalse },

	// points to the URL for mod information, should not be modified by server admin
	{ NULL,                    "mod_url",                SHORT_MOD_URL,                CVAR_SERVERINFO | CVAR_ROM,                                    0,      qfalse, qfalse },

	{ NULL,                    "mod_version",            MOD_VERSION,                  CVAR_SERVERINFO | CVAR_ROM,                                    0,      qfalse, qfalse },

	// configured by the server admin, points to the web pages for the server
	{ NULL,                    "URL",                    SHORT_MOD_URL,                CVAR_SERVERINFO | CVAR_ARCHIVE,                                0,      qfalse, qfalse },

	// Nico, beginning of ETrun server cvars

	// Max connections per IP
	{ &g_maxConnsPerIP,        "g_maxConnsPerIP",        "3",                          CVAR_ARCHIVE,                                                  qfalse, qfalse, qfalse },

	// Game physics (set serverside but sent to client for prediction)
	{ &physics,                "physics",                "255",                        CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH | CVAR_SYSTEMINFO, qfalse, qfalse, qfalse },

	// Enable certain map entities
	// 3 enabled both kill entities and hurt entities
	{ &g_enableMapEntities,    "g_enableMapEntities",    "31",                         CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// Force timer reset, i.e. bypass "wait 9999" on start triggers
	{ &g_forceTimerReset,      "g_forceTimerReset",      "1",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// Is level a timerun?
	{ &isTimerun,              "isTimerun",              "0",                          CVAR_ROM | CVAR_SYSTEMINFO,                                    qfalse, qfalse, qfalse },

	// Flood protection
	{ &g_floodProtect,         "g_floodProtect",         "1",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },
	{ &g_floodThreshold,       "g_floodThreshold",       "8",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },
	{ &g_floodWait,            "g_floodWait",            "768",                        CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// Name changes limit
	{ &g_maxNameChanges,       "g_maxNameChanges",       "3",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// API module
	{ &g_useAPI,               "g_useAPI",               "0",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },
	{ &g_APImoduleName,        "g_APImoduleName",        "timeruns.mod",               CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// Hold doors open
	{ &g_holdDoorsOpen,        "g_holdDoorsOpen",        "1",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// Disable drowning
	{ &g_disableDrowning,      "g_disableDrowning",      "1",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// Mapscript support
	{ &g_mapScriptDirectory,   "g_mapScriptDirectory",   "custommapscripts",           CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// Cup mode
	{ &g_cupMode,              "g_cupMode",              "0",                          CVAR_ARCHIVE,                                                  qfalse, qfalse, qfalse },
	{ &g_cupKey,               "g_cupKey",               "",                           CVAR_ARCHIVE,                                                  qfalse, qfalse, qfalse },

	// Timelimit mode
	{ &g_timelimit,            "timelimit",              "0",                          CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_LATCH,                   qfalse, qfalse, qfalse },

	// Logging
	{ &g_debugLog,             "g_debugLog",             "0",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },
	{ &g_chatLog,              "g_chatLog",              "1",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// GeoIP
	{ &g_useGeoIP,             "g_useGeoIP",             "1",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },
	{ &g_geoIPDbPath,          "g_geoIPDbPath",          "GeoIP.dat",                  CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse },

	// Strict save/load
	{ &g_strictSaveLoad,       "g_strictSaveLoad",       "0",                          CVAR_ARCHIVE | CVAR_LATCH,                                     qfalse, qfalse, qfalse }

	// Nico, end of ETrun cvars
};

// bk001129 - made static to avoid aliasing
static int gameCvarTableSize = sizeof (gameCvarTable) / sizeof (gameCvarTable[0]);

/*
================
vmMain

This is the only way control passes into the module.
================
*/
Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6) {
	// Nico, silent GCC
	(void)arg2;
	(void)arg3;
	(void)arg4;
	(void)arg5;
	(void)arg6;

	switch (command) {
	case GAME_INIT:
		G_InitGame(arg0, arg1);
		return 0;
	case GAME_SHUTDOWN:
		G_ShutdownGame(arg0);
		return 0;
	case GAME_CLIENT_CONNECT:
		return (intptr_t)ClientConnect(arg0, arg1);
	case GAME_CLIENT_THINK:
		ClientThink(arg0);
		return 0;
	case GAME_CLIENT_USERINFO_CHANGED:
		ClientUserinfoChanged(arg0);
		return 0;
	case GAME_CLIENT_DISCONNECT:
		ClientDisconnect(arg0);
		return 0;
	case GAME_CLIENT_BEGIN:
		ClientBegin(arg0);
		return 0;
	case GAME_CLIENT_COMMAND:
		ClientCommand(arg0);
		return 0;
	case GAME_RUN_FRAME:
		G_RunFrame(arg0);
		return 0;
	case GAME_CONSOLE_COMMAND:
		return ConsoleCommand();
	case GAME_SNAPSHOT_CALLBACK:
		return qtrue;

	case GAME_MESSAGERECEIVED:
		return -1;
	}

	return -1;
}

void QDECL G_Printf(const char *fmt, ...) {
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof (text), fmt, argptr);
	va_end(argptr);

	trap_Printf(text);
}

void QDECL G_DPrintf(const char *fmt, ...) {
	va_list argptr;
	char    text[1024];

	if (!g_developer.integer) {
		return;
	}

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof (text), fmt, argptr);
	va_end(argptr);

	trap_Printf(text);
}

void QDECL G_Error(const char *fmt, ...) {
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof (text), fmt, argptr);
	va_end(argptr);

	trap_Error(text);
}

#define CH_KNIFE_DIST       48  // from g_weapon.c
#define CH_LADDER_DIST      100
#define CH_WATER_DIST       100
#define CH_BREAKABLE_DIST   64
#define CH_DOOR_DIST        96
#define CH_ACTIVATE_DIST    96
#define CH_EXIT_DIST        256
#define CH_FRIENDLY_DIST    1024

#define CH_MAX_DIST         1024    // use the largest value from above
#define CH_MAX_DIST_ZOOM    8192    // max dist for zooming hints

/*
==============
G_CursorHintIgnoreEnt: returns whether the ent should be ignored
for cursor hint purpose (because the ent may have the designed content type
but nevertheless should not display any cursor hint)
==============
*/
static qboolean G_CursorHintIgnoreEnt(gentity_t *traceEnt) {
	return (traceEnt->s.eType == ET_OID_TRIGGER || traceEnt->s.eType == ET_TRIGGER_MULTIPLE) ? qtrue : qfalse;
}

qboolean G_EmplacedGunIsMountable(gentity_t *ent, gentity_t *other) {
	if (Q_stricmp(ent->classname, "misc_mg42") && Q_stricmp(ent->classname, "misc_aagun")) {
		return qfalse;
	}

	if (!other->client) {
		return qfalse;
	}

	if (BG_IsScopedWeapon(other->client->ps.weapon)) {
		return qfalse;
	}

	if (other->client->ps.pm_flags & PMF_DUCKED) {
		return qfalse;
	}

	if (other->client->ps.persistant[PERS_HWEAPON_USE]) {
		return qfalse;
	}

	if (ent->r.currentOrigin[2] - other->r.currentOrigin[2] >= 40) {
		return qfalse;
	}

	if (ent->r.currentOrigin[2] - other->r.currentOrigin[2] < 0) {
		return qfalse;
	}

	if (ent->s.frame != 0) {
		return qfalse;
	}

	if (ent->active) {
		return qfalse;
	}

	if (other->client->ps.grenadeTimeLeft) {
		return qfalse;
	}

	if (infront(ent, other)) {
		return qfalse;
	}

	return qtrue;
}

qboolean G_EmplacedGunIsRepairable(gentity_t *ent, gentity_t *other) {
	if (Q_stricmp(ent->classname, "misc_mg42") && Q_stricmp(ent->classname, "misc_aagun")) {
		return qfalse;
	}

	if (!other->client) {
		return qfalse;
	}

	if (BG_IsScopedWeapon(other->client->ps.weapon)) {
		return qfalse;
	}

	if (other->client->ps.persistant[PERS_HWEAPON_USE]) {
		return qfalse;
	}

	if (ent->s.frame == 0) {
		return qfalse;
	}

	return qtrue;
}

void G_CheckForCursorHints(gentity_t *ent) {
	vec3_t        forward, right, up, offset, end;
	trace_t       *tr;
	float         dist;
	float         chMaxDist = CH_MAX_DIST;
	gentity_t     *checkEnt, *traceEnt = 0;
	playerState_t *ps;
	int           hintType, hintDist, hintVal;
	qboolean      zooming;
	int           trace_contents;       // DHM - Nerve
	int           numOfIgnoredEnts = 0;

	if (!ent->client) {
		return;
	}

	ps = &ent->client->ps;

	zooming = (qboolean)(ps->eFlags & EF_ZOOMING);

	AngleVectors(ps->viewangles, forward, right, up);

	VectorCopy(ps->origin, offset);
	offset[2] += ps->viewheight;

	// lean
	if (ps->leanf) {
		VectorMA(offset, ps->leanf, right, offset);
	}

	if (zooming) {
		VectorMA(offset, CH_MAX_DIST_ZOOM, forward, end);
	} else {
		VectorMA(offset, chMaxDist, forward, end);
	}

	tr = &ps->serverCursorHintTrace;

	trace_contents = (CONTENTS_TRIGGER | CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE);
	trap_Trace(tr, offset, NULL, NULL, end, ps->clientNum, trace_contents);

	// reset all
	hintType = ps->serverCursorHint = HINT_NONE;
	hintVal  = ps->serverCursorHintVal = 0;

	dist = VectorDistanceSquared(offset, tr->endpos);

	if (zooming) {
		hintDist = CH_MAX_DIST_ZOOM;
	} else {
		hintDist = chMaxDist;
	}

	// Arnout: building something - add this here because we don't have anything solid to trace to - quite ugly-ish
	if (ent->client->touchingTOI && ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER) {
		gentity_t *constructible;
		if ((constructible = G_IsConstructible(ent->client->sess.sessionTeam, ent->client->touchingTOI)) != NULL) {
			ps->serverCursorHint    = HINT_CONSTRUCTIBLE;
			ps->serverCursorHintVal = (int)constructible->s.angles2[0];
			return;
		}
	}

	if (tr->fraction == 1) {
		return;
	}

	traceEnt = &g_entities[tr->entityNum];
	while (G_CursorHintIgnoreEnt(traceEnt) && numOfIgnoredEnts < 10) {
		// xkan, 1/9/2003 - we may hit multiple invalid ents at the same point
		// count them to prevent too many loops
		numOfIgnoredEnts++;

		// xkan, 1/8/2003 - advance offset (start point) past the entity to ignore
		VectorMA(tr->endpos, 0.1, forward, offset);

		trap_Trace(tr, offset, NULL, NULL, end, traceEnt->s.number, trace_contents);

		// xkan, 1/8/2003 - (hintDist - dist) is the actual distance in the above
		// trap_Trace call. update dist accordingly.
		dist += VectorDistanceSquared(offset, tr->endpos);
		if (tr->fraction == 1) {
			return;
		}
		traceEnt = &g_entities[tr->entityNum];
	}

	if (tr->entityNum == ENTITYNUM_WORLD) {
		if ((tr->contents & CONTENTS_WATER) && !(ps->powerups[PW_BREATHER])) {
			hintDist = CH_WATER_DIST;
			hintType = HINT_WATER;
		} else if ((tr->surfaceFlags & SURF_LADDER) && !(ps->pm_flags & PMF_LADDER)) {         // ladder
			hintDist = CH_LADDER_DIST;
			hintType = HINT_LADDER;
		}
	} else if (tr->entityNum < MAX_CLIENTS) {
		// Show medics a syringe if they can revive someone

		if (traceEnt->client &&
		    traceEnt->client->sess.sessionTeam == ent->client->sess.sessionTeam &&
		    ps->stats[STAT_PLAYER_CLASS] == PC_MEDIC &&
		    traceEnt->client->ps.pm_type == PM_DEAD &&
		    !(traceEnt->client->ps.pm_flags & PMF_LIMBO)) {
			hintDist = 48;        // JPW NERVE matches weapon_syringe in g_weapon.c
			hintType = HINT_REVIVE;
		}
	} else {
		checkEnt = traceEnt;

		// Arnout: invisible entities don't show hints
		if (traceEnt->entstate == STATE_INVISIBLE || traceEnt->entstate == STATE_UNDERCONSTRUCTION) {
			return;
		}

		// check invisible_users first since you don't want to draw a hint based
		// on that ent, but rather on what they are targeting.
		// so find the target and set checkEnt to that to show the proper hint.
		if (traceEnt->s.eType == ET_GENERAL) {

			// ignore trigger_aidoor.  can't just not trace for triggers, since I need invisible_users...
			// damn, I would like to ignore some of these triggers though.

			if (!Q_stricmp(traceEnt->classname, "trigger_aidoor")) {
				return;
			}

			if (!Q_stricmp(traceEnt->classname, "func_invisible_user")) {
				// DHM - Nerve :: Put this back in only in multiplayer
				if (traceEnt->s.dmgFlags) {    // hint icon specified in entity
					hintType = traceEnt->s.dmgFlags;
					hintDist = CH_ACTIVATE_DIST;
					checkEnt = 0;
				} else {   // use target for hint icon
					checkEnt = G_FindByTargetname(NULL, traceEnt->target);
					if (!checkEnt) {       // no target found
						hintType = HINT_BAD_USER;
						hintDist = CH_MAX_DIST_ZOOM;    // show this one from super far for debugging
					}
				}
			}
		}

		if (checkEnt) {

			// TDF This entire function could be the poster boy for converting to OO programming!!!
			// I'm making this into a switch in a vain attempt to make this readable so I can find which
			// brackets don't match!!!

			switch (checkEnt->s.eType) {
			case ET_CORPSE:
				if (!ent->client->ps.powerups[PW_BLUEFLAG] &&
				    !ent->client->ps.powerups[PW_REDFLAG] &&
				    BODY_TEAM(traceEnt) < 4 &&
				    BODY_TEAM(traceEnt) != (int)ent->client->sess.sessionTeam &&
				    traceEnt->nextthink == traceEnt->timestamp + BODY_TIME(BODY_TEAM(traceEnt)) &&
				    ent->client->ps.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS) {
					hintDist = 48;
					hintType = HINT_UNIFORM;
					hintVal  = BODY_VALUE(traceEnt);
					if (hintVal > 255) {
						hintVal = 255;
					}
				}
				break;
			case ET_GENERAL:
			case ET_MG42_BARREL:
			case ET_AAGUN:
				hintType = HINT_FORCENONE;

				if (G_EmplacedGunIsMountable(traceEnt, ent)) {
					hintDist = CH_ACTIVATE_DIST;
					hintType = HINT_MG42;
					hintVal  = 0;
				} else {
					if (ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER && G_EmplacedGunIsRepairable(traceEnt, ent)) {
						hintType = HINT_BUILD;
						hintDist = CH_BREAKABLE_DIST;
						hintVal  = traceEnt->health;
						if (hintVal > 255) {
							hintVal = 255;
						}
					} else {
						hintDist = 0;
						hintType = ps->serverCursorHint = HINT_FORCENONE;
						hintVal  = ps->serverCursorHintVal = 0;
					}
				}
				break;
			case ET_EXPLOSIVE:
			{
				if (checkEnt->spawnflags & EXPLOSIVE_TANK) {
					hintDist = CH_BREAKABLE_DIST * 2;
					hintType = HINT_TANK;
					hintVal  = ps->serverCursorHintVal = 0;         // no health for tank destructibles
				} else {
					switch (checkEnt->constructibleStats.weaponclass) {
					case 0:
						hintDist = CH_BREAKABLE_DIST;
						hintType = HINT_BREAKABLE;
						hintVal  = checkEnt->health;            // also send health to client for visualization
						break;
					case 1:
						hintDist = CH_BREAKABLE_DIST * 2;
						hintType = HINT_SATCHELCHARGE;
						hintVal  = ps->serverCursorHintVal = 0;     // no health for satchel charges
						break;
					case 2:
						hintDist = 0;
						hintType = ps->serverCursorHint = HINT_FORCENONE;
						hintVal  = ps->serverCursorHintVal = 0;

						if (checkEnt->parent && checkEnt->parent->s.eType == ET_OID_TRIGGER &&
						    (((ent->client->sess.sessionTeam == TEAM_AXIS) && (checkEnt->parent->spawnflags & ALLIED_OBJECTIVE)) ||
						     ((ent->client->sess.sessionTeam == TEAM_ALLIES) && (checkEnt->parent->spawnflags & AXIS_OBJECTIVE)))) {
							hintDist = CH_BREAKABLE_DIST * 2;
							hintType = HINT_BREAKABLE_DYNAMITE;
							hintVal  = ps->serverCursorHintVal = 0;         // no health for dynamite
						}
						break;
					default:
						if (checkEnt->health > 0) {
							hintDist = CH_BREAKABLE_DIST;
							hintType = HINT_BREAKABLE;
							hintVal  = checkEnt->health;            // also send health to client for visualization
						} else {
							hintDist = 0;
							hintType = ps->serverCursorHint = HINT_FORCENONE;
							hintVal  = ps->serverCursorHintVal = 0;
						}
						break;
					}
				}

				break;
			}
			case ET_CONSTRUCTIBLE:
				if (G_ConstructionIsPartlyBuilt(checkEnt) && !(checkEnt->spawnflags & CONSTRUCTIBLE_INVULNERABLE)) {
					// only show hint for players who can blow it up
					if (checkEnt->s.teamNum != (int)ent->client->sess.sessionTeam) {
						switch (checkEnt->constructibleStats.weaponclass) {
						case 0:
							hintDist = CH_BREAKABLE_DIST;
							hintType = HINT_BREAKABLE;
							hintVal  = checkEnt->health;            // also send health to client for visualization
							break;
						case 1:
							hintDist = CH_BREAKABLE_DIST * 2;
							hintType = HINT_SATCHELCHARGE;
							hintVal  = ps->serverCursorHintVal = 0;         // no health for satchel charges
							break;
						case 2:
							hintDist = CH_BREAKABLE_DIST * 2;
							hintType = HINT_BREAKABLE_DYNAMITE;
							hintVal  = ps->serverCursorHintVal = 0;         // no health for dynamite
							break;
						default:
							hintDist = 0;
							hintType = ps->serverCursorHint = HINT_FORCENONE;
							hintVal  = ps->serverCursorHintVal = 0;
							break;
						}
					} else {
						hintDist = 0;
						hintType = ps->serverCursorHint = HINT_FORCENONE;
						hintVal  = ps->serverCursorHintVal = 0;
						return;
					}
				}

				break;
			case ET_ALARMBOX:
				if (checkEnt->health > 0) {
					hintType = HINT_ACTIVATE;
				}
				break;

			case ET_ITEM:
			{
				gitem_t *it = &bg_itemlist[checkEnt->item - bg_itemlist];

				hintDist = CH_ACTIVATE_DIST;

				switch (it->giType) {
				case IT_HEALTH:
					hintType = HINT_HEALTH;
					break;
				case IT_TREASURE:
					hintType = HINT_TREASURE;
					break;
				case IT_WEAPON: {
					qboolean canPickup = COM_BitCheck(ent->client->ps.weapons, it->giTag);

					if (!canPickup && it->giTag == WP_AMMO) {
						canPickup = qtrue;
					}

					if (!canPickup) {
						canPickup = G_CanPickupWeapon(it->giTag, ent);
					}

					if (canPickup) {
						hintType = HINT_WEAPON;
					}
					break;
				}
				case IT_AMMO:
					hintType = HINT_AMMO;
					break;
				case IT_ARMOR:
					hintType = HINT_ARMOR;
					break;
				case IT_HOLDABLE:
					hintType = HINT_HOLDABLE;
					break;
				case IT_KEY:
					hintType = HINT_INVENTORY;
					break;
				case IT_TEAM:
					if (!Q_stricmp(traceEnt->classname, "team_CTF_redflag") && ent->client->sess.sessionTeam == TEAM_ALLIES) {
						hintType = HINT_POWERUP;
					} else if (!Q_stricmp(traceEnt->classname, "team_CTF_blueflag") && ent->client->sess.sessionTeam == TEAM_AXIS) {
						hintType = HINT_POWERUP;
					}
					break;
				case IT_BAD:
				default:
					break;
				}

				break;
			}
			case ET_MOVER:
				if (!Q_stricmp(checkEnt->classname, "script_mover")) {
					if (G_TankIsMountable(checkEnt, ent)) {
						hintDist = CH_ACTIVATE_DIST;
						hintType = HINT_ACTIVATE;
					}
				} else if (!Q_stricmp(checkEnt->classname, "func_door_rotating")) {
					if (checkEnt->moverState == MOVER_POS1ROTATE) {      // stationary/closed
						hintDist = CH_DOOR_DIST;
						hintType = HINT_DOOR_ROTATING;
						if (!G_AllowTeamsAllowed(checkEnt, ent)) {       // locked
							hintType = HINT_DOOR_ROTATING_LOCKED;
						}
					}
				} else if (!Q_stricmp(checkEnt->classname, "func_door")) {
					if (checkEnt->moverState == MOVER_POS1) {    // stationary/closed
						hintDist = CH_DOOR_DIST;
						hintType = HINT_DOOR;

						if (!G_AllowTeamsAllowed(checkEnt, ent)) {       // locked
							hintType = HINT_DOOR_LOCKED;
						}
					}
				} else if (!Q_stricmp(checkEnt->classname, "func_button")) {
					hintDist = CH_ACTIVATE_DIST;
					hintType = HINT_BUTTON;
				} else if (!Q_stricmp(checkEnt->classname, "props_flamebarrel")) {
					hintDist = CH_BREAKABLE_DIST * 2;
					hintType = HINT_BREAKABLE;
				} else if (!Q_stricmp(checkEnt->classname, "props_statue")) {
					hintDist = CH_BREAKABLE_DIST * 2;
					hintType = HINT_BREAKABLE;
				}

				break;
			case ET_MISSILE:
			case ET_BOMB:
				if (ps->stats[STAT_PLAYER_CLASS] == PC_ENGINEER) {
					hintDist = CH_BREAKABLE_DIST;
					hintType = HINT_DISARM;
					hintVal  = checkEnt->health;            // also send health to client for visualization
					if (hintVal > 255) {
						hintVal = 255;
					}
				}

				// hint icon specified in entity (and proper contact was made, so hintType was set)
				// first try the checkent...
				if (checkEnt->s.dmgFlags && hintType) {
					hintType = checkEnt->s.dmgFlags;
				}

				// then the traceent
				if (traceEnt->s.dmgFlags && hintType) {
					hintType = traceEnt->s.dmgFlags;
				}

				break;
			default:
				break;
			}

			if (zooming) {
				hintDist = CH_MAX_DIST_ZOOM;

				// zooming can eat a lot of potential hints
				switch (hintType) {

				// allow while zooming
				case HINT_PLAYER:
				case HINT_TREASURE:
				case HINT_LADDER:
				case HINT_EXIT:
				case HINT_NOEXIT:
				case HINT_PLYR_FRIEND:
				case HINT_PLYR_NEUTRAL:
				case HINT_PLYR_ENEMY:
				case HINT_PLYR_UNKNOWN:
					break;

				default:
					return;
				}
			}
		}
	}

	if (dist <= (float)Square(hintDist)) {
		ps->serverCursorHint    = hintType;
		ps->serverCursorHintVal = hintVal;
	}
}

void G_SetTargetName(gentity_t *ent, char *targetname) {
	if (targetname && *targetname) {
		ent->targetname     = targetname;
		ent->targetnamehash = BG_StringHashValue(targetname);
	} else {
		ent->targetnamehash = -1;
	}
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams(void) {
	gentity_t *e, *e2;
	int       i, j;
	int       c, c2;

	c  = 0;
	c2 = 0;
	for (i = 1, e = g_entities + i ; i < level.num_entities ; ++i, ++e) {
		if (!e->inuse) {
			continue;
		}

		if (!e->team) {
			continue;
		}

		if (e->flags & FL_TEAMSLAVE) {
			continue;
		}

		if (!Q_stricmp(e->classname, "func_tramcar")) {
			if (e->spawnflags & 8) {   // leader
				e->teammaster = e;
			} else {
				continue;
			}
		}

		c++;
		c2++;
		for (j = i + 1, e2 = e + 1 ; j < level.num_entities ; ++j, ++e2) {
			if (!e2->inuse) {
				continue;
			}
			if (!e2->team) {
				continue;
			}
			if (e2->flags & FL_TEAMSLAVE) {
				continue;
			}
			if (!strcmp(e->team, e2->team)) {
				c2++;
				e2->teamchain  = e->teamchain;
				e->teamchain   = e2;
				e2->teammaster = e;
				e2->flags     |= FL_TEAMSLAVE;

				if (!Q_stricmp(e2->classname, "func_tramcar")) {
					trap_UnlinkEntity(e2);
				}

				// make sure that targets only point at the master
				if (e2->targetname) {
					G_SetTargetName(e, e2->targetname);

					// Rafael
					// note to self: added this because of problems
					// pertaining to keys and double doors
					if (Q_stricmp(e2->classname, "func_door_rotating")) {
						e2->targetname = NULL;
					}
				}
			}
		}
	}

	G_DPrintf("%i teams with %i entities\n", c, c2);
}

/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars(void) {
	int         i;
	cvarTable_t *cv;

	level.server_settings = 0;

	for (i = 0, cv = gameCvarTable; i < gameCvarTableSize; ++i, ++cv) {
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar) {
			cv->modificationCount = cv->vmCvar->modificationCount;
			// OSP - Update vote info for clients, if necessary
			G_checkServerToggle(cv->vmCvar);
		}
	}

	// OSP
	trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));

	if (pmove_msec.integer < 8) {
		trap_Cvar_Set("pmove_msec", "8");
	} else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars(void) {
	int         i;
	cvarTable_t *cv;
	qboolean    fToggles          = qfalse;
	qboolean    fVoteFlags        = qfalse;
	qboolean    chargetimechanged = qfalse;

	for (i = 0, cv = gameCvarTable ; i < gameCvarTableSize ; ++i, ++cv) {
		if (cv->vmCvar) {
			trap_Cvar_Update(cv->vmCvar);

			if (cv->modificationCount != cv->vmCvar->modificationCount) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				if (cv->trackChange && !(cv->cvarFlags & CVAR_LATCH)) {
					trap_SendServerCommand(-1, va("print \"Server:[lof] %s [lon]changed to[lof] %s\n\"", cv->cvarName, cv->vmCvar->string));
				}

				if (cv->vmCvar == &g_filtercams) {
					trap_SetConfigstring(CS_FILTERCAMS, va("%i", g_filtercams.integer));
				}

				if (cv->vmCvar == &g_soldierChargeTime) {
					level.soldierChargeTime[0] = g_soldierChargeTime.integer * level.soldierChargeTimeModifier[0];
					level.soldierChargeTime[1] = g_soldierChargeTime.integer * level.soldierChargeTimeModifier[1];
					chargetimechanged          = qtrue;
				} else if (cv->vmCvar == &g_medicChargeTime) {
					level.medicChargeTime[0] = g_medicChargeTime.integer * level.medicChargeTimeModifier[0];
					level.medicChargeTime[1] = g_medicChargeTime.integer * level.medicChargeTimeModifier[1];
					chargetimechanged        = qtrue;
				} else if (cv->vmCvar == &g_engineerChargeTime) {
					level.engineerChargeTime[0] = g_engineerChargeTime.integer * level.engineerChargeTimeModifier[0];
					level.engineerChargeTime[1] = g_engineerChargeTime.integer * level.engineerChargeTimeModifier[1];
					chargetimechanged           = qtrue;
				} else if (cv->vmCvar == &g_LTChargeTime) {
					level.lieutenantChargeTime[0] = g_LTChargeTime.integer * level.lieutenantChargeTimeModifier[0];
					level.lieutenantChargeTime[1] = g_LTChargeTime.integer * level.lieutenantChargeTimeModifier[1];
					chargetimechanged             = qtrue;
				} else if (cv->vmCvar == &g_covertopsChargeTime) {
					level.covertopsChargeTime[0] = g_covertopsChargeTime.integer * level.covertopsChargeTimeModifier[0];
					level.covertopsChargeTime[1] = g_covertopsChargeTime.integer * level.covertopsChargeTimeModifier[1];
					chargetimechanged            = qtrue;
				} else if (cv->vmCvar == &pmove_msec) {
					if (pmove_msec.integer < 8) {
						trap_Cvar_Set(cv->cvarName, "8");
					} else if (pmove_msec.integer > 33) {
						trap_Cvar_Set(cv->cvarName, "33");
					}
				}
				// OSP - Update vote info for clients, if necessary
				if (cv->vmCvar == &vote_allow_kick          || cv->vmCvar == &vote_allow_map            ||
				    cv->vmCvar == &vote_allow_randommap        ||
				    cv->vmCvar == &vote_allow_referee       ||
				    cv->vmCvar == &vote_allow_antilag        ||
				    cv->vmCvar == &vote_allow_muting
				    ) {
					fVoteFlags = qtrue;
				} else {
					fToggles = (G_checkServerToggle(cv->vmCvar) || fToggles);
				}
			}
		}
	}

	if (fVoteFlags) {
		G_voteFlags();
	}

	if (fToggles) {
		trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
	}

	if (chargetimechanged) {
		char cs[MAX_INFO_STRING];
		cs[0] = '\0';
		Info_SetValueForKey(cs, "axs_sld", va("%i", level.soldierChargeTime[0]));
		Info_SetValueForKey(cs, "ald_sld", va("%i", level.soldierChargeTime[1]));
		Info_SetValueForKey(cs, "axs_mdc", va("%i", level.medicChargeTime[0]));
		Info_SetValueForKey(cs, "ald_mdc", va("%i", level.medicChargeTime[1]));
		Info_SetValueForKey(cs, "axs_eng", va("%i", level.engineerChargeTime[0]));
		Info_SetValueForKey(cs, "ald_eng", va("%i", level.engineerChargeTime[1]));
		Info_SetValueForKey(cs, "axs_lnt", va("%i", level.lieutenantChargeTime[0]));
		Info_SetValueForKey(cs, "ald_lnt", va("%i", level.lieutenantChargeTime[1]));
		Info_SetValueForKey(cs, "axs_cvo", va("%i", level.covertopsChargeTime[0]));
		Info_SetValueForKey(cs, "ald_cvo", va("%i", level.covertopsChargeTime[1]));
		trap_SetConfigstring(CS_CHARGETIMES, cs);
	}
}

// Nico, this function is called when map doesn't have a game_manager (i.e. no script_multiplayer)
// this is the case with q3 maps
static void G_loadFakeGameManager(void) {
	gentity_t *ent;

	// get the next free entity
	level.spawning  = qtrue;
	ent             = G_Spawn();
	ent->scriptName = "fake_game_manager";
	G_Script_ScriptParse(ent);
	G_Script_ScriptEvent(ent, "trigger", "");
}

/*
============
G_InitGame

============
*/
void G_InitGame(int levelTime, int randomSeed) {
	int  i;
	char cs[MAX_INFO_STRING];

	G_Printf("------- Game Initialization -------\n");
	G_Printf("Mod: %s - %s\n", GAME_VERSION, __DATE__);

	srand(randomSeed);

	// Nico, windows specific actions
#if defined _WIN32

# if !defined _WIN64
	// Nico, init crash handler
	win32_initialize_handler();
# endif

	// Nico, load pthread
	if (pthread_win32_process_attach_np() != TRUE) {
		G_Error("G_InitGame: failed to load pthread library!\n");
	}
#endif

	//bani - make sure pak2.pk3 gets referenced on server so pure checks pass
	trap_FS_FOpenFile("pak2.dat", &i, FS_READ);
	trap_FS_FCloseFile(i);

	G_RegisterCvars();

	G_ProcessIPBans();

	G_InitMemory();

	// NERVE - SMF - intialize gamestate
	if (g_gamestate.integer == GS_INITIALIZE) {
		// OSP
		trap_Cvar_Set("gamestate", va("%i", GS_PLAYING));
	}

	// set some level globals
	i = level.server_settings;
	{
		qboolean   oldspawning = level.spawning;
		voteInfo_t votedata;

		memcpy(&votedata, &level.voteInfo, sizeof (voteInfo_t));

		memset(&level, 0, sizeof (level));

		memcpy(&level.voteInfo, &votedata, sizeof (voteInfo_t));

		level.spawning = oldspawning;
	}
	level.time            = levelTime;
	level.startTime       = levelTime;
	level.server_settings = i;

	for (i = 0; i < level.numConnectedClients; ++i) {
		level.clients[level.sortedClients[i]].sess.spawnObjectiveIndex = 0;
	}

	// RF, init the anim scripting
	level.animScriptData.soundIndex = G_SoundIndex;
	level.animScriptData.playSound  = G_AnimScriptSound;

	level.soldierChargeTime[0]    = level.soldierChargeTime[1] = g_soldierChargeTime.integer;
	level.medicChargeTime[0]      = level.medicChargeTime[1] = g_medicChargeTime.integer;
	level.engineerChargeTime[0]   = level.engineerChargeTime[1] = g_engineerChargeTime.integer;
	level.lieutenantChargeTime[0] = level.lieutenantChargeTime[1] = g_LTChargeTime.integer;

	level.covertopsChargeTime[0] = level.covertopsChargeTime[1] = g_covertopsChargeTime.integer;

	level.soldierChargeTimeModifier[0]    = level.soldierChargeTimeModifier[1] = 1.f;
	level.medicChargeTimeModifier[0]      = level.medicChargeTimeModifier[1] = 1.f;
	level.engineerChargeTimeModifier[0]   = level.engineerChargeTimeModifier[1] = 1.f;
	level.lieutenantChargeTimeModifier[0] = level.lieutenantChargeTimeModifier[1] = 1.f;
	level.covertopsChargeTimeModifier[0]  = level.covertopsChargeTimeModifier[1] = 1.f;

	cs[0] = '\0';
	Info_SetValueForKey(cs, "axs_sld", va("%i", level.soldierChargeTime[0]));
	Info_SetValueForKey(cs, "ald_sld", va("%i", level.soldierChargeTime[1]));
	Info_SetValueForKey(cs, "axs_mdc", va("%i", level.medicChargeTime[0]));
	Info_SetValueForKey(cs, "ald_mdc", va("%i", level.medicChargeTime[1]));
	Info_SetValueForKey(cs, "axs_eng", va("%i", level.engineerChargeTime[0]));
	Info_SetValueForKey(cs, "ald_eng", va("%i", level.engineerChargeTime[1]));
	Info_SetValueForKey(cs, "axs_lnt", va("%i", level.lieutenantChargeTime[0]));
	Info_SetValueForKey(cs, "ald_lnt", va("%i", level.lieutenantChargeTime[1]));
	Info_SetValueForKey(cs, "axs_cvo", va("%i", level.covertopsChargeTime[0]));
	Info_SetValueForKey(cs, "ald_cvo", va("%i", level.covertopsChargeTime[1]));
	trap_SetConfigstring(CS_CHARGETIMES, cs);
	trap_SetConfigstring(CS_FILTERCAMS, va("%i", g_filtercams.integer));

	G_SoundIndex("sound/misc/referee.wav");
	G_SoundIndex("sound/misc/vote.wav");
	G_SoundIndex("sound/player/gurp1.wav");
	G_SoundIndex("sound/player/gurp2.wav");

	trap_GetServerinfo(cs, sizeof (cs));
	Q_strncpyz(level.rawmapname, Info_ValueForKey(cs, "mapname"), sizeof (level.rawmapname));

	trap_SetConfigstring(CS_SCRIPT_MOVER_NAMES, "");     // clear out

	if (g_log.string[0]) {
		if (g_logSync.integer) {
			trap_FS_FOpenFile(g_log.string, &level.logFile, FS_APPEND_SYNC);
		} else {
			trap_FS_FOpenFile(g_log.string, &level.logFile, FS_APPEND);
		}
		if (!level.logFile) {
			G_Printf("WARNING: Couldn't open logfile: %s\n", g_log.string);
		} else {
			G_LogPrintf(qfalse, "InitGame: %s\n", cs);
		}
	} else {
		G_Printf("Not logging to disk.\n");
	}

	// Nico, API logging
	if (g_debugLog.integer) {
		trap_FS_FOpenFile("debug.log", &level.debugLogFile, FS_APPEND_SYNC);
		if (!level.debugLogFile) {
			G_Printf("WARNING: Couldn't open debug.log\n");
		}
	}

	// Nico, chat logging
	if (g_chatLog.integer) {
		trap_FS_FOpenFile("chat.log", &level.chatLogFile, FS_APPEND_SYNC);
		if (!level.chatLogFile) {
			G_Printf("WARNING: Couldn't open chat.log\n");
		}
	}

	// Nico, load API
	// Note: do not check API here, it could crash
	if (g_useAPI.integer) {
		G_loadAPI();

		if (!G_API_getConfig()) {
			G_Error("%s: failed to get config from API!\n", GAME_VERSION);
		}
	}

	G_InitWorldSession();

	// DHM - Nerve :: Clear out spawn target config strings
	trap_GetConfigstring(CS_MULTI_INFO, cs, sizeof (cs));
	Info_SetValueForKey(cs, "numspawntargets", "0");
	trap_SetConfigstring(CS_MULTI_INFO, cs);

	for (i = CS_MULTI_SPAWNTARGETS; i < CS_MULTI_SPAWNTARGETS + MAX_MULTI_SPAWNTARGETS; ++i) {
		trap_SetConfigstring(i, "");
	}

	G_ResetTeamMapData();

	// initialize all entities for this game
	memset(g_entities, 0, MAX_GENTITIES * sizeof (g_entities[0]));
	level.gentities = g_entities;

	// initialize all clients for this game
	level.maxclients = g_maxclients.integer;
	memset(g_clients, 0, MAX_CLIENTS * sizeof (g_clients[0]));
	level.clients = g_clients;

	// set client fields on player ents
	for (i = 0 ; i < level.maxclients ; ++i) {
		g_entities[i].client = level.clients + i;
	}

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	level.num_entities = MAX_CLIENTS;

	// let the server system know where the entites are
	trap_LocateGameData(level.gentities, level.num_entities, sizeof (gentity_t),
	                    &level.clients[0].ps, sizeof (level.clients[0]));

	// load level script
	G_Script_ScriptLoad();

	numSplinePaths = 0 ;
	numPathCorners = 0;

	// TAT 11/13/2002
	//		similarly set up the Server entities
	InitServerEntities();

	// parse the key/value pairs and spawn gentities
	G_SpawnEntitiesFromString();

	// TAT 11/13/2002 - entities are spawned, so now we can do setup
	InitialServerEntitySetup();

	// Gordon: debris test
	G_LinkDebris();

	// Gordon: link up damage parents
	G_LinkDamageParents();

	BG_ClearScriptSpeakerPool();

	BG_LoadSpeakerScript(va("sound/maps/%s.sps", level.rawmapname));

	// ===================

	if (!level.gameManager) {
		G_DPrintf("WARNING: no 'script_multiplayer' found in map\n");

		// Nico, load fake game manager
		G_loadFakeGameManager();
	}

	// Link all the splines up
	BG_BuildSplinePaths();

	// general initialization
	G_FindTeams();

	BG_ClearAnimationPool();

	BG_ClearCharacterPool();

	BG_InitWeaponStrings();

	G_RegisterPlayerClasses();

	// Match init work
	G_loadMatchGame();

	// Nico, init global active threads counter
	activeThreadsCounter = 0;

	// Nico, enable threading
	threadingAllowed = qtrue;

	// Nico, flood protection
	if (g_floodProtect.integer && trap_Cvar_VariableIntegerValue("sv_floodprotect")) {
		trap_Cvar_Set("sv_floodprotect", "0");
	}

	// Nico, is level a timerun?
	if (!level.isTimerun) {
		trap_Cvar_Set("isTimerun", "0");
		G_Printf("%s: no timerun found in map\n", GAME_VERSION);
	} else {
		trap_Cvar_Set("isTimerun", "1");
	}

	// Nico, load GeoIP databse
	if (g_useGeoIP.integer) {
		GeoIP_open(g_geoIPDbPath.string);
	}

	// Nico, install timelimit
	G_install_timelimit();

	// Nico, enabled delayed map change watcher
	if (!G_enable_delayed_map_change_watcher()) {
		G_Error("%s: error while installing delayed map change watcher\n", GAME_VERSION);
	}

	// suburb, initiate vote delay
	level.voteInfo.lastVoteTime = level.startTime;
}

/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame(int restart) {
	G_Printf("==== ShutdownGame ====\n");

	// Nico, disable delayed map change watcher
	G_disable_delayed_map_change_watcher();

	if (level.logFile) {
		G_LogPrintf(qtrue, "ShutdownGame:\n");
		trap_FS_FCloseFile(level.logFile);
		level.logFile = 0;
	}

	// Nico, close debug log
	if (level.debugLogFile) {
		trap_FS_FCloseFile(level.debugLogFile);
		level.debugLogFile = 0;
	}

	// Nico, close chat log
	if (level.chatLogFile) {
		trap_FS_FCloseFile(level.chatLogFile);
		level.chatLogFile = 0;
	}

	// Nico, unload API
	if (g_useAPI.integer) {
		G_unloadAPI();
	}

	// Nico, unload GeoIP databse
	if (g_useGeoIP.integer) {
		GeoIP_close();
	}

	// write all the client session data so we can get it back
	G_WriteSessionData(restart);

	// Nico, windows specific actions
#if defined _WIN32

# if !defined _WIN64
	// Nico, unload sig handler
	win32_deinitialize_handler();
# endif

	// Nico, unload pthread
	pthread_win32_process_detach_np();
	pthread_win32_thread_detach_np();
#endif
}

/*
=============
SortRanks

=============
*/
int QDECL SortRanks(const void *a, const void *b) {
	gclient_t *ca, *cb;

	ca = &level.clients[*(int *)a];
	cb = &level.clients[*(int *)b];

	// sort special clients last
	if (ca->sess.spectatorClient < 0) {
		return 1;
	}
	if (cb->sess.spectatorClient < 0) {
		return -1;
	}

	// then connecting clients
	if (ca->pers.connected == CON_CONNECTING) {
		return 1;
	}
	if (cb->pers.connected == CON_CONNECTING) {
		return -1;
	}

	// then spectators
	if (ca->sess.sessionTeam == TEAM_SPECTATOR && cb->sess.sessionTeam == TEAM_SPECTATOR) {
		if (ca->sess.spectatorTime < cb->sess.spectatorTime) {
			return -1;
		}
		if (ca->sess.spectatorTime > cb->sess.spectatorTime) {
			return 1;
		}
		return 0;
	}
	if (ca->sess.sessionTeam == TEAM_SPECTATOR) {
		return 1;
	}
	if (cb->sess.sessionTeam == TEAM_SPECTATOR) {
		return -1;
	}
	return 0;
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/

//bani - #184
//(relatively) sane replacement for OSP's Players_Axis/Players_Allies
void etpro_PlayerInfo(void) {
	//128 bits
	char      playerinfo[MAX_CLIENTS + 1];
	gentity_t *e;
	team_t    playerteam;
	int       i;
	int       lastclient;

	memset(playerinfo, 0, sizeof (playerinfo));

	lastclient = -1;
	e          = &g_entities[0];
	for (i = 0; i < MAX_CLIENTS; ++i, ++e) {
		if (e->client == NULL || e->client->pers.connected == CON_DISCONNECTED) {
			playerinfo[i] = '-';
			continue;
		}

		//keep track of highest connected/connecting client
		lastclient = i;

		if (e->inuse == qfalse) {
			playerteam = 0;
		} else {
			playerteam = e->client->sess.sessionTeam;
		}
		playerinfo[i] = (char)'0' + playerteam;
	}
	//terminate the string, if we have any non-0 clients
	if (lastclient != -1) {
		playerinfo[lastclient + 1] = (char)0;
	} else {
		playerinfo[0] = (char)0;
	}

	trap_Cvar_Set("P", playerinfo);
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks(void) {
	int  i;
	char teaminfo[TEAM_NUM_TEAMS][256];     // OSP

	level.follow1                   = -1;
	level.follow2                   = -1;
	level.numConnectedClients       = 0;
	level.numPlayingClients         = 0;
	level.voteInfo.numVotingClients = 0;        // don't count bots

	level.voteInfo.numVotingTeamClients[0] = 0;
	level.voteInfo.numVotingTeamClients[1] = 0;

	for (i = 0; i < TEAM_NUM_TEAMS; ++i) {
		if (i < 2) {
			level.numTeamClients[i] = 0;
		}
		teaminfo[i][0] = 0;         // OSP
	}

	for (i = 0 ; i < level.maxclients ; ++i) {
		if (level.clients[i].pers.connected != CON_DISCONNECTED) {
			int team = level.clients[i].sess.sessionTeam;

			level.sortedClients[level.numConnectedClients] = i;
			level.numConnectedClients++;

			// Nico, count spectators that voted
			if (team == TEAM_SPECTATOR && level.clients[i].ps.eFlags & EF_VOTED) {
				level.voteInfo.numVotingClients++;
			}

			if (team != TEAM_SPECTATOR) {
				// OSP
				Q_strcat(teaminfo[team], sizeof (teaminfo[team]) - 1, va("%d ", level.numConnectedClients));

				// decide if this should be auto-followed
				if (level.clients[i].pers.connected == CON_CONNECTED) {
					int teamIndex = level.clients[i].sess.sessionTeam == TEAM_AXIS ? 0 : 1;
					level.numPlayingClients++;
					level.voteInfo.numVotingClients++;

					if (level.clients[i].sess.sessionTeam == TEAM_AXIS ||
					    level.clients[i].sess.sessionTeam == TEAM_ALLIES) {

						level.numTeamClients[teamIndex]++;
						level.voteInfo.numVotingTeamClients[teamIndex]++;
					}

					if (level.follow1 == -1) {
						level.follow1 = i;
					} else if (level.follow2 == -1) {
						level.follow2 = i;
					}
				}
			}
		}
	}

	// OSP
	for (i = 0; i < TEAM_NUM_TEAMS; ++i) {
		if (0 == teaminfo[i][0]) {
			Q_strncpyz(teaminfo[i], "(None)", sizeof (teaminfo[i]));
		}
	}

	qsort(level.sortedClients, level.numConnectedClients, sizeof (level.sortedClients[0]), SortRanks);

	//bani - #184
	etpro_PlayerInfo();
}

/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint(void) {
	gentity_t *ent;

	// NERVE - SMF - if the match hasn't ended yet, and we're just a spectator
	// try to find the intermission spawnpoint with no team flags set
	ent = G_Find(NULL, FOFS(classname), "info_player_intermission");

	for ( ; ent; ent = G_Find(ent, FOFS(classname), "info_player_intermission")) {
		if (!ent->spawnflags) {
			break;
		}
	}

	if (!ent) {
		ent = G_Find(NULL, FOFS(classname), "info_player_intermission");
		while (ent) {
			if (ent->spawnflags & TEAM_AXIS) {
				break;
			}

			ent = G_Find(ent, FOFS(classname), "info_player_intermission");
		}
	}

	if (!ent) {      // the map creator forgot to put in an intermission point...
		SelectSpawnPoint(vec3_origin, level.intermission_origin, level.intermission_angle);
	} else {
		VectorCopy(ent->s.origin, level.intermission_origin);
		VectorCopy(ent->s.angles, level.intermission_angle);
		// if it has a target, look towards it
		if (ent->target) {
			gentity_t *target;

			target = G_PickTarget(ent->target);
			if (target) {
				vec3_t dir;

				VectorSubtract(target->s.origin, level.intermission_origin, dir);
				vectoangles(dir, level.intermission_angle);
			}
		}
	}
}

/*
==================
CheckVote
==================
*/
void CheckVote(void) {
	gentity_t *other = NULL;

	if (!level.voteInfo.voteTime || level.voteInfo.vote_fn == NULL || level.time - level.voteInfo.voteTime < 1000) {
		return;
	}

	// Nico, check if the voter switches teams (from TJMod)
	other = g_entities + level.voteInfo.voter_cn;
	if (level.voteInfo.voter_team != (int)other->client->sess.sessionTeam) {
		AP("cpm \"^5Vote canceled^z: voter switched teams\n\"");
		G_LogPrintf(qtrue, "Vote Failed: %s (voter %s switched teams)\n", level.voteInfo.voteString, other->client->pers.netname);
	} else if (level.time - level.voteInfo.voteTime >= VOTE_TIME) {
		AP(va("cpm \"^2Vote FAILED! ^3(%s)\n\"", level.voteInfo.voteString));
		G_LogPrintf(qtrue, "Vote Failed: %s\n", level.voteInfo.voteString);
	} else {
		int pcnt = vote_percent.integer;
		int total;

		if (pcnt > 99) {
			pcnt = 99;
		}
		if (pcnt < 1) {
			pcnt = 1;
		}

		if (level.voteInfo.vote_fn == G_Kick_v) {
			gentity_t *other = &g_entities[atoi(level.voteInfo.vote_value)];
			if (!other->client || other->client->sess.sessionTeam == TEAM_SPECTATOR) {
				total = level.voteInfo.numVotingClients;
			} else {
				total = level.voteInfo.numVotingTeamClients[other->client->sess.sessionTeam == TEAM_AXIS ? 0 : 1];
			}
		} else {
			total = level.voteInfo.numVotingClients;
		}

		if (level.voteInfo.voteYes > pcnt * total / 100) {
			// execute the command, then remove the vote
			if (level.voteInfo.voteYes > total + 1) {
				// Don't announce some votes, as in comp mode, it is generally a ref
				// who is policing people who shouldn't be joining and players don't want
				// this sort of spam in the console
				if (level.voteInfo.vote_fn != G_Kick_v) {
					AP(va("cpm \"^5Referee changed setting! ^7(%s)\n\"", level.voteInfo.voteString));
				}
				G_LogPrintf(qtrue, "Referee Setting: %s\n", level.voteInfo.voteString);
			} else {
				AP("cpm \"^5Vote passed!\n\"");
				G_LogPrintf(qtrue, "Vote Passed: %s\n", level.voteInfo.voteString);
			}

			// Perform the passed vote
			level.voteInfo.vote_fn(NULL, 0, NULL, NULL, qfalse);

		} else if (level.voteInfo.voteNo && level.voteInfo.voteNo >= (100 - pcnt) * total / 100) {
			// same behavior as a no response vote
			AP(va("cpm \"^2Vote FAILED! ^3(%s)\n\"", level.voteInfo.voteString));
			G_LogPrintf(qtrue, "Vote Failed: %s\n", level.voteInfo.voteString);
		} else {
			// still waiting for a majority
			return;
		}
	}

	level.voteInfo.voteTime = 0;
	trap_SetConfigstring(CS_VOTE_TIME, "");
}

/*
==================
CheckCvars
==================
*/
void CheckCvars(void) {
	static int g_password_lastMod = -1;

	if (g_password.modificationCount != g_password_lastMod) {
		g_password_lastMod = g_password.modificationCount;
		if (*g_password.string && Q_stricmp(g_password.string, "none")) {
			trap_Cvar_Set("g_needpass", "1");
		} else {
			trap_Cvar_Set("g_needpass", "0");
		}
	}
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink(gentity_t *ent) {
	float thinktime;

	// RF, run scripting
	if (ent->s.number >= MAX_CLIENTS) {
		G_Script_ScriptRun(ent);
	}

	thinktime = ent->nextthink;
	if (thinktime <= 0) {
		return;
	}
	if (thinktime > level.time) {
		return;
	}

	ent->nextthink = 0;
	if (!ent->think) {
		G_Error("NULL ent->think");
	}
	ent->think(ent);
}

void G_RunEntity(gentity_t *ent, int msec);

/*
======================
G_PositionEntityOnTag
======================
*/
qboolean G_PositionEntityOnTag(gentity_t *entity, gentity_t *parent, char *tagName) {
	int           i;
	orientation_t tag;
	vec3_t        axis[3];

	AnglesToAxis(parent->r.currentAngles, axis);

	VectorCopy(parent->r.currentOrigin, entity->r.currentOrigin);

	if (!trap_GetTag(-1, parent->tagNumber, tagName, &tag)) {
		return qfalse;
	}

	for (i = 0 ; i < 3 ; ++i) {
		VectorMA(entity->r.currentOrigin, tag.origin[i], axis[i], entity->r.currentOrigin);
	}

	if (entity->client && entity->s.eFlags & EF_MOUNTEDTANK) {
		// zinx - moved tank hack to here
		// bani - fix tank bb
		// zinx - figured out real values, only tag_player is applied,
		// so there are two left:
		// mg42upper attaches to tag_mg42nest[mg42base] at:
		// 0.03125, -1.171875, 27.984375
		// player attaches to tag_playerpo[mg42upper] at:
		// 3.265625, -1.359375, 2.96875
		// this is a hack, by the way.
		entity->r.currentOrigin[0] += 0.03125 + 3.265625;
		entity->r.currentOrigin[1] += -1.171875 + -1.359375;
		entity->r.currentOrigin[2] += 27.984375 + 2.96875;
	}

	G_SetOrigin(entity, entity->r.currentOrigin);

	if (entity->r.linked &&
	    !entity->client &&
	    !VectorCompare(entity->oldOrigin, entity->r.currentOrigin)) {
		trap_LinkEntity(entity);
	}

	return qtrue;
}

void G_TagLinkEntity(gentity_t *ent, int msec) {
	gentity_t *parent = &g_entities[ent->s.torsoAnim];
	gentity_t *obstacle;
	vec3_t    origin, angles = { 0 };
	vec3_t    v;

	if (ent->linkTagTime >= level.time) {
		return;
	}

	G_RunEntity(parent, msec);

	if (!(parent->s.eFlags & EF_PATH_LINK)) {
		if (parent->s.pos.trType == TR_LINEAR_PATH) {
			int   pos;
			float frac;

			if ((ent->backspline = BG_GetSplineData(parent->s.effect2Time, &ent->back)) == NULL) {
				return;
			}

			ent->backdelta = parent->s.pos.trDuration ? (level.time - parent->s.pos.trTime) / ((float)parent->s.pos.trDuration) : 0;

			if (ent->backdelta < 0.f) {
				ent->backdelta = 0.f;
			} else if (ent->backdelta > 1.f) {
				ent->backdelta = 1.f;
			}

			if (ent->back) {
				ent->backdelta = 1 - ent->backdelta;
			}

			pos = floor(ent->backdelta * (MAX_SPLINE_SEGMENTS));
			if (pos >= MAX_SPLINE_SEGMENTS) {
				pos  = MAX_SPLINE_SEGMENTS - 1;
				frac = ent->backspline->segments[pos].length;
			} else {
				frac = ((ent->backdelta * (MAX_SPLINE_SEGMENTS)) - pos) * ent->backspline->segments[pos].length;
			}

			VectorMA(ent->backspline->segments[pos].start, frac, ent->backspline->segments[pos].v_norm, v);
			if (parent->s.apos.trBase[0]) {
				BG_LinearPathOrigin2(parent->s.apos.trBase[0], &ent->backspline, &ent->backdelta, v);
			}

			VectorCopy(v, origin);

			if (ent->s.angles2[0]) {
				BG_LinearPathOrigin2(ent->s.angles2[0], &ent->backspline, &ent->backdelta, v);
			}

			VectorCopy(v, ent->backorigin);

			if (ent->s.angles2[0] < 0) {
				VectorSubtract(v, origin, v);
				vectoangles(v, angles);
			} else if (ent->s.angles2[0] > 0) {
				VectorSubtract(origin, v, v);
				vectoangles(v, angles);
			} else {
				VectorCopy(vec3_origin, origin);
			}

			ent->moving = qtrue;
		} else {
			ent->moving = qfalse;
		}
	} else {
		if (parent->moving) {
			VectorCopy(parent->backorigin, v);

			ent->back       = parent->back;
			ent->backdelta  = parent->backdelta;
			ent->backspline = parent->backspline;

			VectorCopy(v, origin);

			if (ent->s.angles2[0]) {
				BG_LinearPathOrigin2(ent->s.angles2[0], &ent->backspline, &ent->backdelta, v);
			}

			VectorCopy(v, ent->backorigin);

			if (ent->s.angles2[0] < 0) {
				VectorSubtract(v, origin, v);
				vectoangles(v, angles);
			} else if (ent->s.angles2[0] > 0) {
				VectorSubtract(origin, v, v);
				vectoangles(v, angles);
			} else {
				VectorCopy(vec3_origin, origin);
			}

			ent->moving = qtrue;
		} else {
			ent->moving = qfalse;
		}
	}

	if (ent->moving) {
		vec3_t move, amove;

		VectorSubtract(origin, ent->r.currentOrigin, move);
		VectorSubtract(angles, ent->r.currentAngles, amove);

		if (!G_MoverPush(ent, move, amove, &obstacle)) {
			script_mover_blocked(ent, obstacle);
		}

		VectorCopy(origin, ent->s.pos.trBase);
		VectorCopy(angles, ent->s.apos.trBase);
	} else {
		memset(&ent->s.pos, 0, sizeof (ent->s.pos));
		memset(&ent->s.apos, 0, sizeof (ent->s.apos));

		VectorCopy(ent->r.currentOrigin, ent->s.pos.trBase);
		VectorCopy(ent->r.currentAngles, ent->s.apos.trBase);
	}

	ent->linkTagTime = level.time;
}

void G_RunEntity(gentity_t *ent, int msec) {
	if (ent->runthisframe) {
		return;
	}

	ent->runthisframe = qtrue;

	if (!ent->inuse) {
		return;
	}

	if (ent->tagParent) {

		G_RunEntity(ent->tagParent, msec);

		if (ent->tagParent &&
		    G_PositionEntityOnTag(ent, ent->tagParent, ent->tagName) &&
		    !ent->client) {
			if (!ent->s.density) {
				BG_EvaluateTrajectory(&ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time);
				VectorAdd(ent->tagParent->r.currentAngles, ent->r.currentAngles, ent->r.currentAngles);
			} else {
				BG_EvaluateTrajectory(&ent->s.apos, level.time, ent->r.currentAngles, qtrue, ent->s.effect2Time);
			}
		}
	} else if (ent->s.eFlags & EF_PATH_LINK) {
		G_TagLinkEntity(ent, msec);
	}

	// ydnar: hack for instantaneous velocity
	VectorCopy(ent->r.currentOrigin, ent->oldOrigin);

	// check EF_NODRAW status for non-clients
	if (ent - g_entities > level.maxclients) {
		if (ent->flags & FL_NODRAW) {
			ent->s.eFlags |= EF_NODRAW;
		} else {
			ent->s.eFlags &= ~EF_NODRAW;
		}
	}

	// clear events that are too old
	if (level.time - ent->eventTime > EVENT_VALID_MSEC) {
		if (ent->s.event) {
			ent->s.event = 0;
		}
		if (ent->freeAfterEvent) {
			// tempEntities or dropped items completely go away after their event
			G_FreeEntity(ent);
			return;
		} else if (ent->unlinkAfterEvent) {
			// items that will respawn will hide themselves after their pickup event
			ent->unlinkAfterEvent = qfalse;
			trap_UnlinkEntity(ent);
		}
	}

	// temporary entities don't think
	if (ent->freeAfterEvent) {
		return;
	}

	// Arnout: invisible entities don't think
	// NOTE: hack - constructible one does
	if (ent->s.eType != ET_CONSTRUCTIBLE && (ent->entstate == STATE_INVISIBLE || ent->entstate == STATE_UNDERCONSTRUCTION)) {
		// Gordon: we want them still to run scripts tho :p
		if (ent->s.number >= MAX_CLIENTS) {
			G_Script_ScriptRun(ent);
		}
		return;
	}

	if (!ent->r.linked && ent->neverFree) {
		return;
	}

	if (ent->s.eType == ET_MISSILE
	    || ent->s.eType == ET_FLAMEBARREL
	    || ent->s.eType == ET_FP_PARTS
	    || ent->s.eType == ET_FIRE_COLUMN
	    || ent->s.eType == ET_FIRE_COLUMN_SMOKE
	    || ent->s.eType == ET_EXPLO_PART
	    || ent->s.eType == ET_RAMJET) {

		G_RunMissile(ent);

		return;
	}

	// DHM - Nerve :: Server-side collision for flamethrower
	if (ent->s.eType == ET_FLAMETHROWER_CHUNK) {
		G_RunFlamechunk(ent);

		// ydnar: hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	if (ent->s.eType == ET_ITEM || ent->physicsObject) {
		G_RunItem(ent);

		// ydnar: hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	if (ent->s.eType == ET_MOVER || ent->s.eType == ET_PROP) {
		G_RunMover(ent);

		// ydnar: hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	if (ent - g_entities < MAX_CLIENTS) {
		G_RunClient(ent);

		// ydnar: hack for instantaneous velocity
		VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
		VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);

		return;
	}

	if ((ent->s.eType == ET_HEALER || ent->s.eType == ET_SUPPLIER) && ent->target_ent) {
		ent->target_ent->s.onFireStart = ent->health;
		ent->target_ent->s.onFireEnd   = ent->count;
	}

	G_RunThink(ent);

	// ydnar: hack for instantaneous velocity
	VectorSubtract(ent->r.currentOrigin, ent->oldOrigin, ent->instantVelocity);
	VectorScale(ent->instantVelocity, 1000.0f / msec, ent->instantVelocity);
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame(int levelTime) {
	int i, msec;

	level.timeCurrent = levelTime - level.timeDelta;
	level.frameTime   = trap_Milliseconds();

	level.framenum++;
	level.previousTime = level.time;
	level.time         = levelTime;

	msec = level.time - level.previousTime;

	// get any cvar changes
	G_UpdateCvars();

	for (i = 0; i < level.num_entities; ++i) {
		g_entities[i].runthisframe = qfalse;
	}

	// go through all allocated objects
	for (i = 0; i < level.num_entities; ++i) {
		G_RunEntity(&g_entities[i], msec);
	}

	for (i = 0; i < level.numConnectedClients; ++i) {
		ClientEndFrame(&g_entities[level.sortedClients[i]]);
	}

	// update to team status?
	CheckTeamStatus();

	// cancel vote if timed out
	CheckVote();

	// for tracking changes
	CheckCvars();

	G_UpdateTeamMapData();
}

// Nico, delayed map change watcher helper functions
qboolean G_enable_delayed_map_change_watcher() {
	int            rc = 0;
	pthread_attr_t attr;

	// Create threads as detached
	if (pthread_attr_init(&attr)) {
		LDE("%s\n", "error in pthread_attr_init");
		return qfalse;
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		LDE("%s\n", "error in pthread_attr_setdetachstate");
		return qfalse;
	}

	rc = pthread_create(&globalThreads[DELAYED_MAP_CHANGE_THREAD_ID], &attr, G_delayed_map_change_watcher, NULL);
	if (rc) {
		LDE("error in pthread_create: %d\n", rc);
		return qfalse;
	}

	if (pthread_attr_destroy(&attr)) {
		LDE("%s\n", "error in pthread_attr_destroy");
		return qfalse;
	}

	return qtrue;
}

void G_disable_delayed_map_change_watcher() {
	level.delayedMapChange.disabledWatcher = qtrue;

	G_DPrintf("%s: waiting for delayed map change watcher to end...\n", GAME_VERSION);
	my_sleep(1000);
}
// Nico, end of delayed map change watcher helper functions

// Nico, timelimit function
void G_install_timelimit() {
	// Nico, set default gametype to "map-voting"
	trap_Cvar_Set("g_gametype", "6");

	if (g_timelimit.integer == 0) {
		G_DPrintf("%s: no timelimit set. (timelimit = 0)\n", GAME_VERSION);
		return;
	}

	// Check it's valid
	if (g_timelimit.integer < 1) {
		G_DPrintf("%s: timelimit too low! (%d min%s)\n", GAME_VERSION, g_timelimit.integer, g_timelimit.integer > 1 ? "s" : "");
		trap_Cvar_Set("timelimit", "0");
		return;
	}

	// Check API is available
	if (!g_useAPI.integer) {
		G_DPrintf("%s: API is required in order to use a timelimit. No timelimit set!\n", GAME_VERSION);
		trap_Cvar_Set("timelimit", "0");
		return;
	}

	// Update gametype to campaign mode
	trap_Cvar_Set("g_gametype", "4");

	G_DPrintf("%s: timelimit set to %d min%s (timelimit = %d)\n", GAME_VERSION, g_timelimit.integer, g_timelimit.integer > 1 ? "s" : "", g_timelimit.integer);
	G_randommap();
}

/**
 * Get a random map
 */
int G_randommap() {
	char *result = NULL;

	// Nico, check if API is used
	if (!g_useAPI.integer) {
		G_Printf("API is disabled on this server.\n");
		return G_INVALID;
	}

	result = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!result) {
		G_Error("G_Randommap_v: malloc failed\n");
	}

	if (!G_API_randommap(result, NULL, level.rawmapname)) {
		G_Printf("Random map vote failed!\n");
		return G_INVALID;
	}

	return G_OK;
}
