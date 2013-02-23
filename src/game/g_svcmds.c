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




// this file holds commands that can be executed by the server console, but not remote clients

#include "g_local.h"
#include "g_api.h"

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and you can use '*' to match any value
so you can specify an entire class C network with "addip 192.246.40.*"

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

g_filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.

TTimo NOTE: GUID functions are copied over from the model of IP banning,
used to enforce max lives independently from server reconnect and team changes (Xian)

TTimo NOTE: for persistence, bans are stored in g_banIPs cvar MAX_CVAR_VALUE_STRING
The size of the cvar string buffer is limiting the banning to around 20 masks
this could be improved by putting some g_banIPs2 g_banIps3 etc. maybe
still, you should rely on PB for banning instead

==============================================================================
*/

#define MAX_IPFILTERS   1024

typedef struct ipFilterList_s {
	ipFilter_t ipFilters[MAX_IPFILTERS];
	int numIPFilters;
	char cvarIPList[32];
} ipFilterList_t;

static ipFilterList_t ipFilters;

/*
=================
StringToFilter
=================
*/
qboolean StringToFilter(const char *s, ipFilter_t *f) {
	char num[128];
	int  i, j;
	byte b[4];
	byte m[4];

	for (i = 0 ; i < 4 ; i++) {
		b[i] = 0;
		m[i] = 0;
	}

	for (i = 0 ; i < 4 ; i++) {
		if (*s < '0' || *s > '9') {
			if (*s == '*') {   // 'match any'
				// b[i] and m[i] to 0
				s++;
				if (!*s) {
					break;
				}
				s++;
				continue;
			}
			G_Printf("Bad filter address: %s\n", s);
			return qfalse;
		}

		j = 0;
		while (*s >= '0' && *s <= '9') {
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i]   = atoi(num);
		m[i]   = 255;

		if (!*s) {
			break;
		}
		s++;
	}

	// Nico, fix for strict-aliasing rule break
	f->mask = ((m[0] << 24) | (m[1] << 16) | (m[2] << 8) | (m[3]));
	f->compare = ((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | (b[3]));
	//f->mask    = *(unsigned *)m;
	//f->compare = *(unsigned *)b;

	return qtrue;
}

/*
=================
UpdateIPBans
=================
*/
static void UpdateIPBans(ipFilterList_t *ipFilterList) {
	byte b[4];
	byte m[4];
	int  i, j;
	char iplist_final[MAX_CVAR_VALUE_STRING];
	char ip[64];

	*iplist_final = 0;
	for (i = 0 ; i < ipFilterList->numIPFilters ; i++) {
		if (ipFilterList->ipFilters[i].compare == 0xffffffff) {
			continue;
		}

		// Nico, strict-aliasing rule break fix
		b[0] = (ipFilterList->ipFilters[i].compare >> 24) & 0xFF;
		b[1] = (ipFilterList->ipFilters[i].compare >> 16) & 0xFF;
		b[2] = (ipFilterList->ipFilters[i].compare >> 8) & 0xFF;
		b[3] = ipFilterList->ipFilters[i].compare & 0xFF;
		m[0] = (ipFilterList->ipFilters[i].mask >> 24) & 0xFF;
		m[1] = (ipFilterList->ipFilters[i].mask >> 16) & 0xFF;
		m[2] = (ipFilterList->ipFilters[i].mask >> 8) & 0xFF;
		m[3] = ipFilterList->ipFilters[i].mask & 0xFF;
		//*(unsigned *)b = ipFilterList->ipFilters[i].compare;
		//*(unsigned *)m = ipFilterList->ipFilters[i].mask;

		*ip            = 0;
		for (j = 0; j < 4 ; j++) {
			if (m[j] != 255) {
				Q_strcat(ip, sizeof (ip), "*");
			} else {
				Q_strcat(ip, sizeof (ip), va("%i", b[j]));
			}
			Q_strcat(ip, sizeof (ip), (j < 3) ? "." : " ");
		}
		if (strlen(iplist_final) + strlen(ip) < MAX_CVAR_VALUE_STRING) {
			Q_strcat(iplist_final, sizeof (iplist_final), ip);
		} else {
			Com_Printf("%s overflowed at MAX_CVAR_VALUE_STRING\n", ipFilterList->cvarIPList);
			break;
		}
	}

	trap_Cvar_Set(ipFilterList->cvarIPList, iplist_final);
}

/*
=================
G_FilterPacket
=================
*/
qboolean G_FilterPacket(ipFilterList_t *ipFilterList, char *from) {
	int      i;
	unsigned in;
	byte     m[4];
	char     *p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i] * 10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':') {
			break;
		}
		i++, p++;
	}

	// Nico, fix against strict-aliasing rule break
	in = ((m[0] << 24) | (m[1] << 16) | (m[2] << 8) | (m[3]));
	//in = *(unsigned *)m;

	for (i = 0; i < ipFilterList->numIPFilters; i++)
		if ((in & ipFilterList->ipFilters[i].mask) == ipFilterList->ipFilters[i].compare) {
			return g_filterBan.integer != 0;
		}

	return g_filterBan.integer == 0;
}

qboolean G_FilterIPBanPacket(char *from) {
	return(G_FilterPacket(&ipFilters, from));
}

/*
=================
AddIP
=================
*/
void AddIP(ipFilterList_t *ipFilterList, const char *str) {
	int i;

	for (i = 0; i < ipFilterList->numIPFilters; i++) {
		if (ipFilterList->ipFilters[i].compare == 0xffffffff) {
			break;      // free spot
		}
	}

	if (i == ipFilterList->numIPFilters) {
		if (ipFilterList->numIPFilters == MAX_IPFILTERS) {
			G_Printf("IP filter list is full\n");
			return;
		}
		ipFilterList->numIPFilters++;
	}

	if (!StringToFilter(str, &ipFilterList->ipFilters[i])) {
		ipFilterList->ipFilters[i].compare = 0xffffffffu;
	}

	UpdateIPBans(ipFilterList);
}

void AddIPBan(const char *str) {
	AddIP(&ipFilters, str);
}

/*
=================
G_ProcessIPBans
=================
*/
void G_ProcessIPBans(void) {
	char *s, *t;
	char str[MAX_CVAR_VALUE_STRING];

	ipFilters.numIPFilters = 0;
	Q_strncpyz(ipFilters.cvarIPList, "g_banIPs", sizeof (ipFilters.cvarIPList));

	Q_strncpyz(str, g_banIPs.string, sizeof (str));

	for (t = s = g_banIPs.string; *t; /* */) {
		s = strchr(s, ' ');
		if (!s) {
			break;
		}
		while (*s == ' ')
			*s++ = 0;
		if (*t) {
			AddIP(&ipFilters, t);
		}
		t = s;
	}
}

/*
=================
Svcmd_AddIP_f
=================
*/
void Svcmd_AddIP_f(void) {
	char str[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2) {
		G_Printf("Usage:  addip <ip-mask>\n");
		return;
	}

	trap_Argv(1, str, sizeof (str));

	AddIP(&ipFilters, str);

}

/*
=================
Svcmd_RemoveIP_f
=================
*/
void Svcmd_RemoveIP_f(void) {
	ipFilter_t f;
	int        i;
	char       str[MAX_TOKEN_CHARS];

	if (trap_Argc() < 2) {
		G_Printf("Usage:  removeip <ip-mask>\n");
		return;
	}

	trap_Argv(1, str, sizeof (str));

	if (!StringToFilter(str, &f)) {
		return;
	}

	for (i = 0 ; i < ipFilters.numIPFilters ; i++) {
		if (ipFilters.ipFilters[i].mask == f.mask   &&
		    ipFilters.ipFilters[i].compare == f.compare) {
			ipFilters.ipFilters[i].compare = 0xffffffffu;
			G_Printf("Removed.\n");

			UpdateIPBans(&ipFilters);
			return;
		}
	}

	G_Printf("Didn't find %s.\n", str);
}

/*
===================
Svcmd_EntityList_f
===================
*/
void    Svcmd_EntityList_f(void) {
	int       e;
	gentity_t *check;

	check = g_entities + 1;
	for (e = 1; e < level.num_entities ; e++, check++) {
		if (!check->inuse) {
			continue;
		}
		G_Printf("%3i:", e);
		switch (check->s.eType) {
		case ET_GENERAL:
			G_Printf("ET_GENERAL          ");
			break;
		case ET_PLAYER:
			G_Printf("ET_PLAYER           ");
			break;
		case ET_ITEM:
			G_Printf("ET_ITEM             ");
			break;
		case ET_MISSILE:
			G_Printf("ET_MISSILE          ");
			break;
		case ET_MOVER:
			G_Printf("ET_MOVER            ");
			break;
		case ET_BEAM:
			G_Printf("ET_BEAM             ");
			break;
		case ET_PORTAL:
			G_Printf("ET_PORTAL           ");
			break;
		case ET_SPEAKER:
			G_Printf("ET_SPEAKER          ");
			break;
		case ET_PUSH_TRIGGER:
			G_Printf("ET_PUSH_TRIGGER     ");
			break;
		case ET_CONCUSSIVE_TRIGGER:
			G_Printf("ET_CONCUSSIVE_TRIGGR");
			break;
		case ET_TELEPORT_TRIGGER:
			G_Printf("ET_TELEPORT_TRIGGER ");
			break;
		case ET_INVISIBLE:
			G_Printf("ET_INVISIBLE        ");
			break;
		case ET_EXPLOSIVE:
			G_Printf("ET_EXPLOSIVE        ");
			break;
		case ET_EF_SPOTLIGHT:
			G_Printf("ET_EF_SPOTLIGHT     ");
			break;
		case ET_ALARMBOX:
			G_Printf("ET_ALARMBOX          ");
			break;
		default:
			G_Printf("%3i                 ", check->s.eType);
			break;
		}

		if (check->classname) {
			G_Printf("%s", check->classname);
		}
		G_Printf("\n");
	}
}

// fretn, note: if a player is called '3' and there are only 2 players
// on the server (clientnum 0 and 1)
// this function will say 'client 3 is not connected'
// solution: first check for usernames, if none is found, check for slotnumbers
gclient_t *ClientForString(const char *s) {
	gclient_t *cl;
	int       i;
	int       idnum;

	// check for a name match
	for (i = 0 ; i < level.maxclients ; i++) {
		cl = &level.clients[i];
		if (cl->pers.connected == CON_DISCONNECTED) {
			continue;
		}
		if (!Q_stricmp(cl->pers.netname, s)) {
			return cl;
		}
	}

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9') {
		idnum = atoi(s);
		if (idnum < 0 || idnum >= level.maxclients) {
			Com_Printf("Bad client slot: %i\n", idnum);
			return NULL;
		}

		cl = &level.clients[idnum];
		if (cl->pers.connected == CON_DISCONNECTED) {
			G_Printf("Client %i is not connected\n", idnum);
			return NULL;
		}
		return cl;
	}

	G_Printf("User %s is not on the server\n", s);

	return NULL;
}

// fretn

static qboolean G_Is_SV_Running(void) {

	char cvar[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("sv_running", cvar, sizeof (cvar));
	return (qboolean)atoi(cvar);
}

/*
==================
G_GetPlayerByNum
==================
*/
gclient_t *G_GetPlayerByNum(int clientNum) {
	gclient_t *cl;

	// make sure server is running
	if (!G_Is_SV_Running()) {
		return NULL;
	}

	if (trap_Argc() < 2) {
		G_Printf("No player specified.\n");
		return NULL;
	}

	if (clientNum < 0 || clientNum >= level.maxclients) {
		Com_Printf("Bad client slot: %i\n", clientNum);
		return NULL;
	}

	cl = &level.clients[clientNum];

	if (!cl) {
		G_Printf("User %d is not on the server\n", clientNum);
		return NULL;
	}

	if (cl->pers.connected == CON_DISCONNECTED) {
		G_Printf("Client %i is not connected\n", clientNum);
		return NULL;
	}
	return cl;
}

/*
==================
G_GetPlayerByName
==================
*/
gclient_t *G_GetPlayerByName(char *name) {

	int       i;
	gclient_t *cl;
	char      cleanName[64];

	// make sure server is running
	if (!G_Is_SV_Running()) {
		return NULL;
	}

	if (trap_Argc() < 2) {
		G_Printf("No player specified.\n");
		return NULL;
	}

	for (i = 0; i < level.numConnectedClients; i++) {

		// Nico, bugfix: kick command is not able to kick some players by name
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=097
		cl = &level.clients[level.sortedClients[i]];

		if (!Q_stricmp(cl->pers.netname, name)) {
			return cl;
		}

		Q_strncpyz(cleanName, cl->pers.netname, sizeof (cleanName));
		Q_CleanStr(cleanName);
		if (!Q_stricmp(cleanName, name)) {
			return cl;
		}
	}

	G_Printf("Player %s is not on the server\n", name);

	return NULL;
}

// -fretn

/*
===================
Svcmd_ForceTeam_f

forceteam <player> <team>
===================
*/

void Svcmd_ForceTeam_f(void) {
	gclient_t *cl;
	char      str[MAX_TOKEN_CHARS];

	// find the player
	trap_Argv(1, str, sizeof (str));
	cl = ClientForString(str);
	if (!cl) {
		return;
	}

	// set the team
	trap_Argv(2, str, sizeof (str));
	SetTeam(&g_entities[cl - level.clients], str, qfalse, cl->sess.playerWeapon, cl->sess.playerWeapon2, qtrue);
}

/*
==================
Svcmd_ResetMatch_f

OSP - multiuse now for both map restarts and total match resets
==================
*/
void Svcmd_ResetMatch_f(qboolean fDoReset, qboolean fDoRestart) {
	int i;

	// Nico, silent GCC
	fDoReset = fDoReset;

	for (i = 0; i < level.numConnectedClients; i++) {
		g_entities[level.sortedClients[i]].client->pers.ready = 0;
	}

	if (fDoRestart) {
		trap_SendConsoleCommand(EXEC_APPEND, va("map_restart 0 %i\n", GS_RESET));
	}
}

/*
==================
Svcmd_Kick_f

Kick a user off of the server
==================
*/

// change into qfalse if you want to use the qagame banning system
// which makes it possible to unban IP addresses
#define USE_ENGINE_BANLIST qtrue

static void Svcmd_Kick_f(void) {
	gclient_t *cl;
	int       i;
	int       timeout = -1;
	char      sTimeout[MAX_TOKEN_CHARS];
	char      name[MAX_TOKEN_CHARS];

	// make sure server is running
	if (!G_Is_SV_Running()) {
		G_Printf("Server is not running.\n");
		return;
	}

	if (trap_Argc() < 2 || trap_Argc() > 3) {
		G_Printf("Usage: kick <player name> [timeout]\n");
		return;
	}

	if (trap_Argc() == 3) {
		trap_Argv(2, sTimeout, sizeof (sTimeout));
		timeout = atoi(sTimeout);
	} else {
		timeout = 300;
	}

	trap_Argv(1, name, sizeof (name));
	cl = G_GetPlayerByName(name);   //ClientForString( name );

	if (!cl) {
		if (!Q_stricmp(name, "all")) {
			for (i = 0, cl = level.clients; i < level.numConnectedClients; i++, cl++) {

				// dont kick localclients ...
				if (cl->pers.localClient) {
					continue;
				}

				if (timeout != -1) {
					char *ip;
					char userinfo[MAX_INFO_STRING];

					trap_GetUserinfo(cl->ps.clientNum, userinfo, sizeof (userinfo));
					ip = Info_ValueForKey(userinfo, "ip");

					// use engine banning system, mods may choose to use their own banlist
					if (USE_ENGINE_BANLIST) {
						trap_DropClient(cl->ps.clientNum, "player kicked", timeout);
					} else {
						trap_DropClient(cl->ps.clientNum, "player kicked", 0);
						AddIPBan(ip);
					}

				} else {
					trap_DropClient(cl->ps.clientNum, "player kicked", 0);
				}
			}
		}
		return;
	} else {
		// dont kick localclients ...
		if (cl->pers.localClient) {
			G_Printf("Cannot kick host player\n");
			return;
		}

		if (timeout != -1) {
			char *ip;
			char userinfo[MAX_INFO_STRING];

			trap_GetUserinfo(cl->ps.clientNum, userinfo, sizeof (userinfo));
			ip = Info_ValueForKey(userinfo, "ip");

			// use engine banning system, mods may choose to use their own banlist
			if (USE_ENGINE_BANLIST) {
				trap_DropClient(cl->ps.clientNum, "player kicked", timeout);
			} else {
				trap_DropClient(cl->ps.clientNum, "player kicked", 0);
				AddIPBan(ip);
			}

		} else {
			trap_DropClient(cl->ps.clientNum, "player kicked", 0);
		}
	}
}

/*
==================
Svcmd_KickNum_f

Kick a user off of the server
==================
*/
static void Svcmd_KickNum_f(void) {
	gclient_t *cl;
	int       timeout = -1;
	char      *ip;
	char      userinfo[MAX_INFO_STRING];
	char      sTimeout[MAX_TOKEN_CHARS];
	char      name[MAX_TOKEN_CHARS];
	int       clientNum;

	// make sure server is running
	if (!G_Is_SV_Running()) {
		G_Printf("Server is not running.\n");
		return;
	}

	if (trap_Argc() < 2 || trap_Argc() > 3) {
		G_Printf("Usage: kick <client number> [timeout]\n");
		return;
	}

	if (trap_Argc() == 3) {
		trap_Argv(2, sTimeout, sizeof (sTimeout));
		timeout = atoi(sTimeout);
	} else {
		timeout = 300;
	}

	trap_Argv(1, name, sizeof (name));
	clientNum = atoi(name);

	cl = G_GetPlayerByNum(clientNum);
	if (!cl) {
		return;
	}
	if (cl->pers.localClient) {
		G_Printf("Cannot kick host player\n");
		return;
	}

	trap_GetUserinfo(cl->ps.clientNum, userinfo, sizeof (userinfo));
	ip = Info_ValueForKey(userinfo, "ip");
	// use engine banning system, mods may choose to use their own banlist
	if (USE_ENGINE_BANLIST) {
		trap_DropClient(cl->ps.clientNum, "player kicked", timeout);
	} else {
		trap_DropClient(cl->ps.clientNum, "player kicked", 0);
		AddIPBan(ip);
	}
}

// -fretn

// Nico, check API command
static void Svcmd_CheckAPI_f(void) {
	char *result = NULL;

	// make sure server is running
	if (!G_Is_SV_Running()) {
		G_Printf("Server is not running.\n");
		return;
	}

	if (trap_Argc() > 1) {
		G_Printf("Usage: checkAPI\n");
		return;
	}

	// Check if API is used
	if (!g_useAPI.integer) {
		G_Printf("API is disabled on this server.\n");
		return;
	}

	result = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!result) {
		G_Error("Svcmd_CheckAPI_f: malloc failed\n");
	}

	if (!G_API_check(result, NULL)) {
		G_Error("Svcmd_CheckAPI_f: G_API_check failed\n");
	}
}

extern char *ConcatArgs(int start);

/*
=================
ConsoleCommand

=================
*/
qboolean    ConsoleCommand(void) {
	char cmd[MAX_TOKEN_CHARS];

	trap_Argv(0, cmd, sizeof (cmd));

	if (Q_stricmp(cmd, "entitylist") == 0) {
		Svcmd_EntityList_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "forceteam") == 0) {
		Svcmd_ForceTeam_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "game_memory") == 0) {
		Svcmd_GameMem_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "addip") == 0) {
		Svcmd_AddIP_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "removeip") == 0) {
		Svcmd_RemoveIP_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "listip") == 0) {
		trap_SendConsoleCommand(EXEC_INSERT, "g_banIPs\n");
		return qtrue;
	}
	// -NERVE - SMF

	if (Q_stricmp(cmd, "makeReferee") == 0) {
		G_MakeReferee();
		return qtrue;
	}

	if (Q_stricmp(cmd, "removeReferee") == 0) {
		G_RemoveReferee();
		return qtrue;
	}

	if (Q_stricmp(cmd, "ban") == 0) {
		G_PlayerBan();
		return qtrue;
	}

	// fretn - moved from engine
	if (!Q_stricmp(cmd, "kick")) {
		Svcmd_Kick_f();
		return qtrue;
	}

	if (!Q_stricmp(cmd, "clientkick")) {
		Svcmd_KickNum_f();
		return qtrue;
	}
	// -fretn

	// Nico, check API command
	if (!Q_stricmp(cmd, "checkAPI")) {
		Svcmd_CheckAPI_f();
		return qtrue;
	}

	if (g_dedicated.integer) {
		if (!Q_stricmp(cmd, "say")) {
			trap_SendServerCommand(-1, va("cpm \"server: %s\n\"", ConcatArgs(1)));
			return qtrue;
		}

		// OSP - console also gets ref commands
		if (!level.fLocalHost && Q_stricmp(cmd, "ref") == 0) {
			// Nico, bugfix, G_refCommandCheck expects the next argument (warn, pause, lock etc)
			// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=005
			trap_Argv(1, cmd, sizeof (cmd));
			if (!G_refCommandCheck(NULL, cmd)) {
				G_refHelp_cmd(NULL);
			}
			return(qtrue);
		}

		// prints to the console instead now
		return qfalse;
	}

	return qfalse;
}
