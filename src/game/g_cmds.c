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

qboolean G_IsOnFireteam(int entityNum, fireteamData_t **teamNum);

/*
==================
G_SendScore

Sends current scoreboard information
==================
*/
void G_SendScore(gentity_t *ent) {
	char      entry[128];
	int       i;
	gclient_t *cl;
	int       numSorted;
	int       team;

	// send the latest information on all clients
	numSorted = level.numConnectedClients;

	// Nico, replaced hardcoded value by a define
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=068
	if (numSorted > MAX_CLIENTS) {
		numSorted = MAX_CLIENTS;
	}

	i = 0;
	// Gordon: team doesnt actually mean team, ignore...
	for (team = 0; team < 2; ++team) {
		int  size, count = 0;
		char buffer[1024] = { 0 }, startbuffer[32] = { 0 };

		if (team == 0) {
			Q_strncpyz(startbuffer, "sc0", 32);
		} else {
			Q_strncpyz(startbuffer, "sc1", 32);
		}
		size = strlen(startbuffer) + 1;

		for (; i < numSorted ; ++i) {
			int ping;

			cl = &level.clients[level.sortedClients[i]];

			if (g_entities[level.sortedClients[i]].r.svFlags & SVF_POW) {
				continue;
			}

			if (cl->pers.connected == CON_CONNECTING) {
				ping = -1;
			} else {
				ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
			}

			// Nico, added timerun best time, timerun best speed, timerun status, followed client, login status, cgaz, speclocked status
			// Nico, don't send time and speed while in cup mode
			Com_sprintf(entry, sizeof (entry), " %i %i %i %i %i %i %i %i %i %i",
			            level.sortedClients[i],
			            ping,
			            (level.time - cl->pers.enterTime) / 60000,
			            g_cupMode.integer > 0 ? 0 : cl->sess.timerunBestTime[cl->sess.currentTimerunNum],
			            g_cupMode.integer > 0 ? 0 : cl->sess.timerunBestSpeed[cl->sess.currentTimerunNum],
			            g_cupMode.integer > 0 ? 0 : (cl->sess.timerunActive ? 1 : 0),
			            cl->ps.clientNum,
			            cl->sess.logged ? 1 : 0,
			            cl->pers.cgaz > 0 ? 1 : 0,
			            g_cupMode.integer > 0 ? 1 : (cl->sess.specLocked ? 1 : 0)
			            );

			if (size + strlen(entry) > 1000) {
				i--; // we need to redo this client in the next buffer (if we can)
				break;
			}
			size += strlen(entry);

			Q_strcat(buffer, 1024, entry);
			if (++count >= 32) {
				i--; // we need to redo this client in the next buffer (if we can)
				break;
			}
		}

		if (count > 0 || team == 0) {
			trap_SendServerCommand(ent - g_entities, va("%s %i%s", startbuffer, count, buffer));
		}
	}
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f(gentity_t *ent) {
	ent->client->wantsscore = qtrue;
}

/*
==================
ConcatArgs
==================
*/
char *ConcatArgs(int start) {
	int         i, c;
	static char line[MAX_STRING_CHARS];
	int         len;
	char        arg[MAX_STRING_CHARS];

	len = 0;
	c   = trap_Argc();
	for (i = start ; i < c ; ++i) {
		int tlen;

		trap_Argv(i, arg, sizeof (arg));
		tlen = strlen(arg);
		if (len + tlen >= MAX_STRING_CHARS - 1) {
			break;
		}
		memcpy(line + len, arg, tlen);
		len += tlen;
		if (i != c - 1) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString(char *in, char *out, qboolean fToLower) {
	while (*in) {
		if (*in == 27 || *in == '^') {
			in++;       // skip color code
			if (*in) {
				in++;
			}
			continue;
		}

		if (*in < 32) {
			in++;
			continue;
		}

		*out++ = (fToLower) ? tolower(*in++) : *in++;
	}

	*out = 0;
}

/**
==================
ClientNumbersFromString

Sets plist to an array of integers that represent client numbers that have
names that are a partial match for s. List is terminated by a -1.

Returns number of matching clientids.
==================
@source: ETpub
*/
int ClientNumbersFromString(char *s, int *plist) {
	gclient_t *p                   = NULL;
	int       i                    = 0;
	int       found                = 0;
	char      s2[MAX_STRING_CHARS] = { 0 };
	char      n2[MAX_STRING_CHARS] = { 0 };
	char      *m                   = NULL;
	qboolean  is_slot              = qtrue;
	int       len                  = 0;

	*plist = -1;

	// if a number is provided, it might be a slot #
	len = strlen(s);
	for (i = 0; i < len; ++i) {
		if (s[i] < '0' || s[i] > '9') {
			is_slot = qfalse;
			break;
		}
	}
	if (is_slot) {
		i = atoi(s);
		if (i >= 0 && i < level.maxclients) {
			p = &level.clients[i];
			if (p->pers.connected == CON_CONNECTED ||
			    p->pers.connected == CON_CONNECTING) {

				*plist++ = i;
				*plist   = -1;
				return 1;
			}
		}
	}

	// now look for name matches
	SanitizeString(s, s2, qtrue);
	if (strlen(s2) < 1) {
		return 0;
	}
	for (i = 0; i < level.maxclients; ++i) {
		p = &level.clients[i];
		if (p->pers.connected != CON_CONNECTED &&
		    p->pers.connected != CON_CONNECTING) {
			continue;
		}
		SanitizeString(p->pers.netname, n2, qtrue);
		m = strstr(n2, s2);
		if (m != NULL) {
			*plist++ = i;
			found++;
		}
	}
	*plist = -1;
	return found;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString(gentity_t *to, char *s) {
	gclient_t *cl;
	int       idnum;
	char      s2[MAX_STRING_CHARS];
	char      n2[MAX_STRING_CHARS];
	qboolean  fIsNumber = qtrue;
	int       len       = 0;

	// See if its a number or string
	len = strlen(s);
	for (idnum = 0; idnum < len && s[idnum] != 0; ++idnum) {
		if (s[idnum] < '0' || s[idnum] > '9') {
			fIsNumber = qfalse;
			break;
		}
	}

	// check for a name match
	SanitizeString(s, s2, qtrue);
	for (idnum = 0, cl = level.clients; idnum < level.maxclients; ++idnum, ++cl) {
		if (cl->pers.connected != CON_CONNECTED) {
			continue;
		}

		SanitizeString(cl->pers.netname, n2, qtrue);
		if (!strcmp(n2, s2)) {
			return idnum;
		}
	}

	// numeric values are just slot numbers
	if (fIsNumber) {
		idnum = atoi(s);
		if (idnum < 0 || idnum >= level.maxclients) {
			CPx(to - g_entities, va("print \"Bad client slot: [lof]%i\n\"", idnum));
			return -1;
		}

		cl = &level.clients[idnum];
		if (cl->pers.connected != CON_CONNECTED) {
			CPx(to - g_entities, va("print \"Client[lof] %i [lon]is not active\n\"", idnum));
			return -1;
		}
		return idnum;
	}

	CPx(to - g_entities, va("print \"User [lof]%s [lon]is not on the server\n\"", s));
	return -1;
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f(gentity_t *ent) {
	char *msg;

	char *name = ConcatArgs(1);

	// suburb, only available while unfollowed to avoid playermodel duplication
	if (ent->client->ps.pm_flags & PMF_FOLLOW) {
		trap_SendServerCommand(ent - g_entities, va("print \"You must unfollow to use this command.\n\""));
		return;
	}

	// Nico, only available if client is not logged in
	if (ent->client->sess.logged) {
		trap_SendServerCommand(ent - g_entities, va("print \"You must /logout to use this command.\n\""));
		return;
	}

	if (ent->health <= 0) {
		trap_SendServerCommand(ent - g_entities, va("print \"You must be alive to use this command.\n\""));
		return;
	}

	// suburb, only available while standing upright
	if ((ent->client->ps.eFlags & EF_CROUCHING) || ent->client->ps.eFlags & EF_PRONE || ent->client->ps.eFlags & EF_PRONE_MOVING) {
		trap_SendServerCommand(ent - g_entities, va("print \"You must stand up to use this command.\n\""));
		return;
	}

	if (!Q_stricmp(name, "on") || atoi(name)) {
		ent->client->noclip = qtrue;
	} else if (!Q_stricmp(name, "off") || !Q_stricmp(name, "0")) {
		ent->client->noclip = qfalse;
	} else {
		ent->client->noclip = !ent->client->noclip;
	}

	if (ent->client->noclip) {
		msg = "noclip ON\n";
	} else {
		msg = "noclip OFF\n";
	}

	trap_SendServerCommand(ent - g_entities, va("print \"%s\"", msg));
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f(gentity_t *ent) {
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
	    (ent->client->ps.pm_flags & PMF_LIMBO) ||
	    ent->health <= 0) {
		return;
	}

	ent->flags                                  &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH]           = ent->health = 0;
	ent->client->ps.persistant[PERS_HWEAPON_USE] = 0; // TTimo - if using /kill while at MG42
	player_die(ent, ent, ent, (g_gamestate.integer == GS_PLAYING) ? 100000 : 135, MOD_SUICIDE);
}

void G_TeamDataForString(const char *teamstr, int clientNum, team_t *team, spectatorState_t *sState, int *specClient) {
	*sState = SPECTATOR_NOT;
	if (!Q_stricmp(teamstr, "follow1")) {
		*team   = TEAM_SPECTATOR;
		*sState = SPECTATOR_FOLLOW;
		if (specClient) {
			*specClient = -1;
		}
	} else if (!Q_stricmp(teamstr, "follow2")) {
		*team   = TEAM_SPECTATOR;
		*sState = SPECTATOR_FOLLOW;
		if (specClient) {
			*specClient = -2;
		}
	} else if (!Q_stricmp(teamstr, "spectator") || !Q_stricmp(teamstr, "s")) {
		*team   = TEAM_SPECTATOR;
		*sState = SPECTATOR_FREE;
	} else if (!Q_stricmp(teamstr, "red") || !Q_stricmp(teamstr, "r") || !Q_stricmp(teamstr, "axis")) {
		*team = TEAM_AXIS;
	} else if (!Q_stricmp(teamstr, "blue") || !Q_stricmp(teamstr, "b") || !Q_stricmp(teamstr, "allies")) {
		*team = TEAM_ALLIES;
	} else {
		*team = PickTeam(clientNum);
		if (!G_teamJoinCheck(*team, &g_entities[clientNum])) {
			*team = ((TEAM_AXIS | TEAM_ALLIES) & ~*team);
		}
	}
}

/*
=================
SetTeam
=================
*/
qboolean SetTeam(gentity_t *ent, char *s, weapon_t w1, weapon_t w2, qboolean setweapons) {
	team_t           team, oldTeam;
	gclient_t        *client   = ent->client;
	int              clientNum = client - level.clients;
	spectatorState_t specState;
	int              specClient = 0;

	//
	// see what change is requested
	//

	G_TeamDataForString(s, client - level.clients, &team, &specState, &specClient);

	// Ensure the player can join
	if (team != TEAM_SPECTATOR && !G_teamJoinCheck(team, ent)) {
		// Leave them where they were before the command was issued
		return qfalse;
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if (team == oldTeam && team != TEAM_SPECTATOR) {
		return qfalse;
	}

	if (oldTeam != TEAM_SPECTATOR && !(ent->client->ps.pm_flags & PMF_LIMBO)) {
		// Kill him (makes sure he loses flags, etc)
		ent->flags                        &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die(ent, ent, ent, 100000, MOD_SWITCHTEAM);
	}
	// they go to the end of the line for tournements
	if (team == TEAM_SPECTATOR) {
		client->sess.spectatorTime = level.time;
		if (!client->sess.referee) {
			client->pers.invite = 0;
		}
	}

	G_LeaveTank(ent, qfalse);

	G_FadeItems(ent, MOD_SATCHEL);

	// remove ourself from teamlists
	{
		int             i;
		mapEntityData_t *mEnt;

		for (i = 0; i < 2; ++i) {
			mapEntityData_Team_t *teamList = &mapEntityData[i];

			if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL) {
				G_FreeMapEntityData(teamList, mEnt);
			}

			mEnt = G_FindMapEntityDataSingleClient(teamList, NULL, ent->s.number, -1);

			while (mEnt) {
				mapEntityData_t *mEntFree = mEnt;

				mEnt = G_FindMapEntityDataSingleClient(teamList, mEnt, ent->s.number, -1);

				G_FreeMapEntityData(teamList, mEntFree);
			}
		}
	}
	client->sess.spec_team       = 0;
	client->sess.sessionTeam     = team;
	client->sess.spectatorState  = specState;
	client->sess.spectatorClient = specClient;

	// (l)users will spam spec messages... honest!
	if (team != oldTeam) {
		gentity_t *tent = G_PopupMessage(PM_TEAM);
		tent->s.effect2Time = team;
		tent->s.effect3Time = clientNum;
		tent->s.density     = 0;
	}

	if (setweapons) {
		G_SetClientWeapons(ent, w1, w2, qfalse);
	}

	// get and distribute relevent paramters
	G_UpdateCharacter(client);              // FIXME : doesn't ClientBegin take care of this already?
	ClientUserinfoChanged(clientNum);

	ClientBegin(clientNum);

	G_UpdateSpawnCounts();

	if (g_gamestate.integer == GS_PLAYING && (client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES)) {
		int i;
		int x = client->sess.sessionTeam - TEAM_AXIS;

		for (i = 0; i < MAX_COMMANDER_TEAM_SOUNDS; ++i) {
			if (level.commanderSounds[x][i].index) {
				gentity_t *tent = G_TempEntity(client->ps.origin, EV_GLOBAL_CLIENT_SOUND);
				tent->s.eventParm = level.commanderSounds[x][i].index - 1;
				tent->s.teamNum   = clientNum;
			}
		}
	}

	// suburb, start autodemo
	trap_SendServerCommand(ent - g_entities, "tempDemoStart");

	return qtrue;
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing(gentity_t *ent) {
	// ATVI Wolfenstein Misc #474
	// divert behaviour if TEAM_SPECTATOR, moved the code from SpectatorThink to put back into free fly correctly
	// (I am not sure this can be called in non-TEAM_SPECTATOR situation, better be safe)
	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		// drop to free floating, somewhere above the current position (that's the client you were following)
		vec3_t    pos, angle;
		gclient_t *client = ent->client;
		VectorCopy(client->ps.origin, pos);
		VectorCopy(client->ps.viewangles, angle);
		// Need this as it gets spec mode reset properly
		SetTeam(ent, "s", -1, -1, qfalse);
		VectorCopy(pos, client->ps.origin);
		SetClientViewAngle(ent, angle);
	} else {
		// legacy code, FIXME: useless?
		// Gordon: no this is for limbo i'd guess
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->ps.clientNum        = ent - g_entities;
	}
}

void G_SetClientWeapons(gentity_t *ent, weapon_t w1, weapon_t w2, qboolean updateclient) {
	qboolean changed = qfalse;

	if (ent->client->sess.latchPlayerWeapon2 != (int)w2) {
		ent->client->sess.latchPlayerWeapon2 = w2;
		changed                              = qtrue;
	}

	if (ent->client->sess.latchPlayerWeapon != (int)w1) {
		ent->client->sess.latchPlayerWeapon = w1;
		changed                             = qtrue;
	}

	if (updateclient && changed) {
		ClientUserinfoChanged(ent - g_entities);
	}
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f(gentity_t *ent) {
	char     s[MAX_TOKEN_CHARS];
	char     ptype[4];
	char     weap[4], weap2[4];
	weapon_t w, w2;

	if (trap_Argc() < 2) {
		char *pszTeamName;

		switch (ent->client->sess.sessionTeam) {
		case TEAM_ALLIES:
			pszTeamName = "Allies";
			break;
		case TEAM_AXIS:
			pszTeamName = "Axis";
			break;
		case TEAM_SPECTATOR:
			pszTeamName = "Spectator";
			break;
		case TEAM_FREE:
		default:
			pszTeamName = "Free";
			break;
		}

		CP(va("print \"%s team\n\"", pszTeamName));
		return;
	}

	trap_Argv(1, s, sizeof (s));
	trap_Argv(2, ptype, sizeof (ptype));
	trap_Argv(3, weap, sizeof (weap));
	trap_Argv(4, weap2, sizeof (weap2));

	w  = atoi(weap);
	w2 = atoi(weap2);

	ent->client->sess.latchPlayerType = atoi(ptype);
	if (ent->client->sess.latchPlayerType < PC_SOLDIER || ent->client->sess.latchPlayerType > PC_COVERTOPS) {
		ent->client->sess.latchPlayerType = PC_SOLDIER;
	}

	if (ent->client->sess.latchPlayerType < PC_SOLDIER || ent->client->sess.latchPlayerType > PC_COVERTOPS) {
		ent->client->sess.latchPlayerType = PC_SOLDIER;
	}

	if (!SetTeam(ent, s, w, w2, qtrue)) {
		G_SetClientWeapons(ent, w, w2, qtrue);
	}
}

// Nico, class command from TJMod
void Cmd_Class_f(gentity_t *ent) {
	char     ptype[4];
	char     weap[4], weap2[4];
	weapon_t w, w2;

	if (trap_Argc() < 2) {
		CP("Print \"^dUsage:\n\n\"");
		CP("Print \"^dMedic - /class m\n\"");
		CP("Print \"^dEngineer with SMG - /class e 1\n\"");
		CP("Print \"^dEngineer with Rifle - /class e 2\n\"");
		CP("Print \"^dField ops - /class f\n\"");
		CP("Print \"^dCovert ops with sten - /class c 1\n\"");
		CP("Print \"^dCovert ops with FG42 - /class c 2\n\"");
		CP("Print \"^dCovert ops with Rifle - /class c 3\n\"");
		CP("Print \"^dSoldier with SMG - /class s 1\n\"");
		CP("Print \"^dSoldier with MG42 - /class s 2\n\"");
		CP("Print \"^dSoldier with Flamethrower - /class s 3\n\"");
		CP("Print \"^dSoldier with Panzerfaust - /class s 4\n\"");
		CP("Print \"^dSoldier with Mortar - /class s 5\n\"");
		return;
	}

	trap_Argv(1, ptype, sizeof (ptype));
	trap_Argv(2, weap, sizeof (weap));
	trap_Argv(3, weap2, sizeof (weap2));

	if (!Q_stricmp(ptype, "m")) {
		Q_strncpyz(ptype, "1", sizeof (ptype));
	}

	if (!Q_stricmp(ptype, "e")) {
		Q_strncpyz(ptype, "2", sizeof (ptype));
		if (!Q_stricmp(weap, "2")) {
			Q_strncpyz(weap, "23", sizeof (weap));
		}
	}

	if (!Q_stricmp(ptype, "f")) {
		Q_strncpyz(ptype, "3", sizeof (ptype));
	}

	if (!Q_stricmp(ptype, "c")) {
		Q_strncpyz(ptype, "4", sizeof (ptype));
		if (!Q_stricmp(weap, "2")) {
			Q_strncpyz(weap, "33", sizeof (weap));
		} else if (!Q_stricmp(weap, "3")) {
			Q_strncpyz(weap, "25", sizeof (weap));
		}
	}

	if (!Q_stricmp(ptype, "s")) {
		Q_strncpyz(ptype, "5", sizeof (ptype));
		if (!Q_stricmp(weap, "2")) {
			Q_strncpyz(weap, "31", sizeof (weap));
		} else if (!Q_stricmp(weap, "3")) {
			Q_strncpyz(weap, "6", sizeof (weap));
		} else if (!Q_stricmp(weap, "4")) {
			Q_strncpyz(weap, "5", sizeof (weap));
		} else if (!Q_stricmp(weap, "5")) {
			Q_strncpyz(weap, "35", sizeof (weap));
		}
	}

	w  = atoi(weap);
	w2 = atoi(weap2);

	ent->client->sess.latchPlayerType = atoi(ptype);
	if (ent->client->sess.latchPlayerType < PC_SOLDIER || ent->client->sess.latchPlayerType > PC_COVERTOPS) {
		ent->client->sess.latchPlayerType = PC_SOLDIER;
	}

	G_SetClientWeapons(ent, w, w2, qtrue);
}

void Cmd_ResetSetup_f(gentity_t *ent) {
	qboolean changed = qfalse;

	if (!ent || !ent->client) {
		return;
	}

	ent->client->sess.latchPlayerType = ent->client->sess.playerType;

	if (ent->client->sess.latchPlayerWeapon != ent->client->sess.playerWeapon) {
		ent->client->sess.latchPlayerWeapon = ent->client->sess.playerWeapon;
		changed                             = qtrue;
	}

	if (ent->client->sess.latchPlayerWeapon2 != ent->client->sess.playerWeapon2) {
		ent->client->sess.latchPlayerWeapon2 = ent->client->sess.playerWeapon2;
		changed                              = qtrue;
	}

	if (changed) {
		ClientUserinfoChanged(ent - g_entities);
	}
}

// END Mad Doc - TDF
/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue) {
	int  i;
	char arg[MAX_TOKEN_CHARS];

	// Nico, silent GCC
	(void)dwCommand;
	(void)fValue;

	if (trap_Argc() != 2) {
		if (ent->client->sess.spectatorState == SPECTATOR_FOLLOW) {
			StopFollowing(ent);
		}
		return;
	}

	if (ent->client->ps.pm_flags & PMF_LIMBO) {
		// Nico, replaced cpm by print to display into console
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=065
		CP("print \"Can't issue a follow command while in limbo.\n\"");
		CP("print \"Hit FIRE to switch between teammates.\n\"");
		return;
	}

	trap_Argv(1, arg, sizeof (arg));
	i = ClientNumberFromString(ent, arg);
	if (i == -1) {
		if (!Q_stricmp(arg, "allies")) {
			i = TEAM_ALLIES;
		} else if (!Q_stricmp(arg, "axis")) {
			i = TEAM_AXIS;
		} else {
			return;
		}

		if (!TeamCount(ent - g_entities, i)) {
			CP(va("print \"The %s team %s empty!  Follow command ignored.\n\"", aTeams[i],
			      (((int)ent->client->sess.sessionTeam != i) ? "is" : "would be")));
			return;
		}

		// Allow for simple toggle
		if (ent->client->sess.spec_team != i) {
			ent->client->sess.spec_team = i;
			CP(va("print \"Spectator follow is now locked on the %s team.\n\"", aTeams[i]));
			Cmd_FollowCycle_f(ent, 1);
		} else {
			ent->client->sess.spec_team = 0;
			CP(va("print \"%s team spectating is now disabled.\n\"", aTeams[i]));
		}

		return;
	}

	// can't follow self
	if (&level.clients[i] == ent->client) {
		return;
	}

	// can't follow another spectator
	if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR) {
		return;
	}
	if (level.clients[i].ps.pm_flags & PMF_LIMBO) {
		return;
	}

	// can't follow a speclocked client, unless allowed
	if (!G_AllowFollow(ent, g_entities + i) && !ent->client->sess.freeSpec) {
		CP(va("print \"Sorry, player %s ^7is locked from spectators.\n\"", level.clients[i].pers.netname));
		return;
	}

	// first set them to spectator
	if (ent->client->sess.sessionTeam != TEAM_SPECTATOR) {
		SetTeam(ent, "spectator", -1, -1, qfalse);
	}

	ent->client->sess.spectatorState  = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f(gentity_t *ent, int dir) {
	int clientnum;
	int original;

	// first set them to spectator
	if ((ent->client->sess.spectatorState == SPECTATOR_NOT) && (!(ent->client->ps.pm_flags & PMF_LIMBO))) {         // JPW NERVE for limbo state
		SetTeam(ent, "spectator", -1, -1, qfalse);
	}

	if (dir != 1 && dir != -1) {
		G_Error("Cmd_FollowCycle_f: bad dir %i", dir);
	}

	// Nico, fix from etlegacy for team follow1/2 crash
	// if dedicated follow client, just switch between the two auto clients
	if (ent->client->sess.spectatorClient < 0) {
		if (ent->client->sess.spectatorClient == -1) {
			ent->client->sess.spectatorClient = -2;
		} else if (ent->client->sess.spectatorClient == -2) {
			ent->client->sess.spectatorClient = -1;
		}
		return;
	}

	clientnum = ent->client->sess.spectatorClient;
	original  = clientnum;
	do {
		clientnum += dir;
		if (clientnum >= level.maxclients) {
			clientnum = 0;
		}
		if (clientnum < 0) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if (level.clients[clientnum].pers.connected != CON_CONNECTED) {
			continue;
		}

		// can't follow another spectator
		if (level.clients[clientnum].sess.sessionTeam == TEAM_SPECTATOR) {
			continue;
		}

		// JPW NERVE -- couple extra checks for limbo mode
		if (ent->client->ps.pm_flags & PMF_LIMBO) {
			if (level.clients[clientnum].ps.pm_flags & PMF_LIMBO) {
				continue;
			}
			if (level.clients[clientnum].sess.sessionTeam != ent->client->sess.sessionTeam) {
				continue;
			}
		}

		if (level.clients[clientnum].ps.pm_flags & PMF_LIMBO) {
			continue;
		}

		// OSP
		if (!G_DesiredFollow(ent, g_entities + clientnum) && !ent->client->sess.freeSpec) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState  = SPECTATOR_FOLLOW;

		// Nico, send an event to the client
		trap_SendServerCommand(ent - g_entities, "followedClientUpdate");

		// suburb, start autodemo
		trap_SendServerCommand(ent - g_entities, "tempDemoStart");

		return;
	} while (clientnum != original);

	// leave it where it was
}

/*
==================
G_Say
==================
*/
#define MAX_SAY_TEXT    150

void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, qboolean localize, qboolean encoded) {
	char *cmd = NULL;

	if (!other || !other->inuse || !other->client) {
		return;
	}
	if (mode == SAY_TEAM && !OnSameTeam(ent, other)) {
		return;
	}

	// NERVE - SMF - if spectator, no chatting to players in WolfMP
	if (ent->client->sess.referee == 0 &&    // OSP
	    ((ent->client->sess.sessionTeam == TEAM_FREE && other->client->sess.sessionTeam != TEAM_FREE))) {
		return;
	}
	// send only to people who have the sender on their buddy list
	if (mode == SAY_BUDDY && ent->s.clientNum != other->s.clientNum) {
		fireteamData_t *ft1, *ft2;
		if (!G_IsOnFireteam(other - g_entities, &ft1)) {
			return;
		}
		if (!G_IsOnFireteam(ent - g_entities, &ft2)) {
			return;
		}
		if (ft1 != ft2) {
			return;
		}
	}

	if (encoded) {
		cmd = mode == SAY_TEAM || mode == SAY_BUDDY ? "enc_tchat" : "enc_chat";
	} else {
		cmd = mode == SAY_TEAM || mode == SAY_BUDDY ? "tchat" : "chat";
	}
	trap_SendServerCommand(other - g_entities, va("%s \"%s%c%c%s\" %d %i", cmd, name, Q_COLOR_ESCAPE, color, message, (int)(ent - g_entities), localize));
}

void G_Say(gentity_t *ent, gentity_t *target, int mode, qboolean encoded, const char *chatText) {
	int  j;
	int  color;
	char name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];
	// suburb, add timestamps
	char    displayTime[18] = { 0 };
	qtime_t tm;

	trap_RealTime(&tm);
	displayTime[0] = '\0';
	Q_strcat(displayTime, sizeof (displayTime), va("[%d:%02d:%02d] ", tm.tm_hour, tm.tm_min, tm.tm_sec));

	switch (mode) {
	default:
	case SAY_ALL:
		if (g_chatLog.integer) {
			G_LogChat("say", "%s: %s\n", ent->client->pers.netname, chatText);
		}
		G_LogPrintf(qtrue, "say: %s: %s\n", ent->client->pers.netname, chatText);
		color = COLOR_GREEN;
		break;
	case SAY_BUDDY:
		if (g_chatLog.integer) {
			G_LogChat("saybuddy", "%s: %s\n", ent->client->pers.netname, chatText);
		}
		G_LogPrintf(qtrue, "saybuddy: %s: %s\n", ent->client->pers.netname, chatText);
		color = COLOR_YELLOW;
		break;
	case SAY_TEAM:
		if (g_chatLog.integer) {
			G_LogChat("sayteam", "%s: %s\n", ent->client->pers.netname, chatText);
		}
		G_LogPrintf(qtrue, "sayteam: %s: %s\n", ent->client->pers.netname, chatText);
		color = COLOR_CYAN;
		break;
	}

	Com_sprintf(name, sizeof (name), "^g%s^7%s%c%c: ", displayTime, ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE);
	Q_strncpyz(text, chatText, sizeof (text));

	if (target) {
		if (!COM_BitCheck(target->client->sess.ignoreClients, ent - g_entities)) {
			G_SayTo(ent, target, mode, color, name, text, qfalse, encoded);
		}
		return;
	}

	// echo the text to the console
	if (g_dedicated.integer) {
		G_Printf("%s%s\n", name, text);
	}

	// send it to all the apropriate clients
	for (j = 0; j < level.numConnectedClients; ++j) {
		gentity_t *other = &g_entities[level.sortedClients[j]];

		if (!COM_BitCheck(other->client->sess.ignoreClients, ent - g_entities)) {
			G_SayTo(ent, other, mode, color, name, text, qfalse, encoded);
		}
	}
}

/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0, qboolean encoded) {
	if (trap_Argc() < 2 && !arg0) {
		return;
	}
	G_Say(ent, NULL, mode, encoded, ConcatArgs((arg0 ? 0 : 1)));
}

// NERVE - SMF
void G_VoiceTo(gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly) {
	int  color;
	char *cmd;

	if (!other) {
		return;
	}
	if (!other->inuse) {
		return;
	}
	if (!other->client) {
		return;
	}
	if (mode == SAY_TEAM && !OnSameTeam(ent, other)) {
		return;
	}

	// send only to people who have the sender on their buddy list
	if (mode == SAY_BUDDY && ent->s.clientNum != other->s.clientNum) {
		fireteamData_t *ft1, *ft2;
		if (!G_IsOnFireteam(other - g_entities, &ft1)) {
			return;
		}
		if (!G_IsOnFireteam(ent - g_entities, &ft2)) {
			return;
		}
		if (ft1 != ft2) {
			return;
		}
	}

	if (mode == SAY_TEAM) {
		color = COLOR_CYAN;
		cmd   = "vtchat";
	} else if (mode == SAY_BUDDY) {
		color = COLOR_YELLOW;
		cmd   = "vbchat";
	} else {
		color = COLOR_GREEN;
		cmd   = "vchat";
	}

	if (mode == SAY_TEAM || mode == SAY_BUDDY) {
		CPx(other - g_entities, va("%s %d %d %d %s %i %i %i", cmd, voiceonly, (int)(ent - g_entities), color, id, (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2]));
	} else {
		CPx(other - g_entities, va("%s %d %d %d %s", cmd, voiceonly, (int)(ent - g_entities), color, id));
	}
}

void G_Voice(gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly) {
	int j;

	// DHM - Nerve :: Don't allow excessive spamming of voice chats
	ent->voiceChatSquelch     -= (level.time - ent->voiceChatPreviousTime);
	ent->voiceChatPreviousTime = level.time;

	if (ent->voiceChatSquelch < 0) {
		ent->voiceChatSquelch = 0;
	}

	// Only do the spam check for MP
	if (ent->voiceChatSquelch >= 30000) {
		// Nico, voicechat spam protection was cluttering the popups message
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=066
		trap_SendServerCommand(ent - g_entities, "cp \"^1Spam Protection^7: VoiceChat ignored\"");
		return;
	}

	if (g_voiceChatsAllowed.integer) {
		ent->voiceChatSquelch += (34000 / g_voiceChatsAllowed.integer);
	} else {
		return;
	}
	// dhm

	if (target) {
		G_VoiceTo(ent, target, mode, id, voiceonly);
		return;
	}

	// echo the text to the console
	if (g_dedicated.integer) {
		G_Printf("voice: %s %s\n", ent->client->pers.netname, id);
	}

	// Nico, log to chat log
	if (g_chatLog.integer) {
		G_LogChat("voice", "%s: %s\n", ent->client->pers.netname, id);
	}

	if (mode == SAY_BUDDY) {
		char     buffer[32];
		int      cls, i, cnt;
		qboolean allowclients[MAX_CLIENTS];

		memset(allowclients, 0, sizeof (allowclients));

		trap_Argv(1, buffer, 32);

		cls = atoi(buffer);

		trap_Argv(2, buffer, 32);
		cnt = atoi(buffer);
		if (cnt > MAX_CLIENTS) {
			cnt = MAX_CLIENTS;
		}

		for (i = 0; i < cnt; ++i) {
			int num;

			trap_Argv(3 + i, buffer, 32);

			num = atoi(buffer);
			if (num < 0) {
				continue;
			}
			if (num >= MAX_CLIENTS) {
				continue;
			}

			allowclients[num] = qtrue;
		}

		for (j = 0; j < level.numConnectedClients; ++j) {
			if (level.sortedClients[j] != ent->s.clientNum &&
			    cls != -1 &&
			    cls != level.clients[level.sortedClients[j]].sess.playerType) {
				continue;
			}

			if (cnt && !allowclients[level.sortedClients[j]]) {
				continue;
			}

			G_VoiceTo(ent, &g_entities[level.sortedClients[j]], mode, id, voiceonly);
		}
	} else {
		// send it to all the apropriate clients
		for (j = 0; j < level.numConnectedClients; ++j) {
			G_VoiceTo(ent, &g_entities[level.sortedClients[j]], mode, id, voiceonly);
		}
	}
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f(gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly) {
	if (mode != SAY_BUDDY) {
		if (trap_Argc() < 2 && !arg0) {
			return;
		}
		G_Voice(ent, NULL, mode, ConcatArgs(((arg0) ? 0 : 1)), voiceonly);
	} else {
		char buffer[16];
		int  index;

		trap_Argv(2, buffer, sizeof (buffer));
		index = atoi(buffer);
		if (index < 0) {
			index = 0;
		}

		if (trap_Argc() < 3 + index && !arg0) {
			return;
		}
		G_Voice(ent, NULL, mode, ConcatArgs(((arg0) ? 2 + index : 3 + index)), voiceonly);
	}
}

/*
==================
Cmd_CallVote_f
==================
*/
qboolean Cmd_CallVote_f(gentity_t *ent, unsigned int dwCommand, qboolean fRefCommand) {
	int  i;
	int  waitTime = 0;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];

	// Nico, silent GCC
	(void)dwCommand;

	// Normal checks, if its not being issued as a referee command
	// Nico, moved 'callvote' command erros from popup messages to center print and console
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=067

	waitTime = (vote_delay.integer - (level.time - level.voteInfo.lastVoteTime)) / 1000;

	if (!fRefCommand) {
		if (level.voteInfo.voteTime) {
			G_printFull("^1Callvote:^7 A vote is already in progress.\n\"", ent);
			return qfalse;
		} else if (!ent->client->sess.referee) {
			if (voteFlags.integer == VOTING_DISABLED) {
				G_printFull("^1Callvote:^7 Voting is not enabled on this server.\n\"", ent);
				return qfalse;
			} else if (g_cupMode.integer != 0) {   // Nico, disable voting in cup mode
				G_printFull("^1Callvote:^7 Voting is disabled in cup mode.\n\"", ent);
				return qfalse;
			} else if (vote_limit.integer > 0 && ent->client->pers.voteCount >= vote_limit.integer) {
				G_printFull(va("^1Callvote:^7 You have already called the maximum number of votes (%d).\n\"", vote_limit.integer), ent);
				return qfalse;
			} else if (level.delayedMapChange.pendingChange) {  // suburb, block all votes during a pending map change
				G_printFull("^1Callvote:^7 There is a pending map change.\n\"", ent);
				return qfalse;
			} else if (waitTime > 0) {  // suburb, block all votes until vote_delay has passed
				G_printFull(va("^1Callvote:^7 Please wait %d second%s before voting.\n\"", waitTime, waitTime > 1 ? "s" : ""), ent);
				return qfalse;
			}
		}
	}

	// make sure it is a valid command to vote on
	trap_Argv(1, arg1, sizeof (arg1));
	trap_Argv(2, arg2, sizeof (arg2));

	// Nico, bugfix: callvote exploit fixed
	// http://aluigi.freeforums.org/quake3-engine-callvote-bug-t686.html
	if (strchr(arg1, ';') || strchr(arg2, ';') ||
	    strchr(arg1, '\r') || strchr(arg2, '\r') ||
	    strchr(arg1, '\n') || strchr(arg2, '\n')) {
		char *strCmdBase = (!fRefCommand) ? "vote" : "ref command";

		G_refPrintf(ent, "Invalid %s string.", strCmdBase);
		return qfalse;
	}

	// Nico, if it's a map vote, do these checks
	if (!Q_stricmp(arg1, "map")) {
		char         mapfile[MAX_QPATH];
		fileHandle_t f;

		if (arg2[0] == '\0' || trap_Argc() == 1) {
			G_printFull("^1Callvote:^7 No map specified.\n\"", ent);
			return qfalse;
		}

		Com_sprintf(mapfile, sizeof (mapfile), "maps/%s.bsp", arg2);

		trap_FS_FOpenFile(mapfile, &f, FS_READ);

		trap_FS_FCloseFile(f);

		if (!f) {
			G_printFull(va("^1Callvote:^7 The map ^3%s^7 is not on the server.\n\"", arg2), ent);
			return qfalse;
		}
	}

	if (trap_Argc() > 1 && (i = G_voteCmdCheck(ent, arg1, arg2, fRefCommand)) != G_NOTFOUND) {         //  --OSP
		if (i != G_OK) {
			return qtrue;
		}
	} else {
		if (!fRefCommand) {
			G_printFull(va("^1Callvote:^7 Unknown vote command: ^3%s %s\n\"", arg1, arg2), ent);
			G_voteHelp(ent, qtrue);
		}
		return qfalse;
	}

	Com_sprintf(level.voteInfo.voteString, sizeof (level.voteInfo.voteString), "%s %s", arg1, arg2);

	// start the voting, the caller automatically votes yes
	// If a referee, vote automatically passes.	// OSP
	if (fRefCommand) {
		// Don't announce some votes, as in comp mode, it is generally a ref
		// who is policing people who shouldn't be joining and players don't want
		// this sort of spam in the console
		if (level.voteInfo.vote_fn != G_Kick_v && level.voteInfo.vote_fn != G_Mute_v) {
			AP("cp \"^1** Referee Server Setting Change **\n\"");
		}

		// Gordon: just call the stupid thing.... don't bother with the voting faff
		level.voteInfo.vote_fn(NULL, 0, NULL, NULL, qfalse);

		G_globalSound("sound/misc/referee.wav");
	} else {
		level.voteInfo.voteYes = 1;
		AP(va("print \"[lof]%s^7 [lon]called a vote.[lof]  Voting for: %s\n\"", ent->client->pers.netname, level.voteInfo.voteString));
		AP(va("cp \"[lof]%s\n^7[lon]called a vote.\n\"", ent->client->pers.netname));
		G_globalSound("sound/misc/vote.wav");
	}

	level.voteInfo.voteTime     = level.time;
	level.voteInfo.lastVoteTime = level.time;
	level.voteInfo.voteNo       = 0;

	// Nico, used to check if voter switches team
	level.voteInfo.voter_team = ent->client->sess.sessionTeam;
	level.voteInfo.voter_cn   = ent - g_entities;

	// Don't send the vote info if a ref initiates (as it will automatically pass)
	if (!fRefCommand) {
		for (i = 0; i < level.numConnectedClients; ++i) {
			level.clients[level.sortedClients[i]].ps.eFlags &= ~EF_VOTED;
		}

		ent->client->pers.voteCount++;
		ent->client->ps.eFlags |= EF_VOTED;

		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteInfo.voteYes));
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteInfo.voteNo));
		trap_SetConfigstring(CS_VOTE_STRING, level.voteInfo.voteString);
		trap_SetConfigstring(CS_VOTE_TIME, va("%i", level.voteInfo.voteTime));
	}

	// Nico, need to recompute
	CalculateRanks();

	return qtrue;
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f(gentity_t *ent) {
	char msg[64];

	if (ent->client->pers.applicationEndTime > level.time) {

		gclient_t *cl = g_entities[ent->client->pers.applicationClient].client;
		if (!cl) {
			return;
		}
		if (cl->pers.connected != CON_CONNECTED) {
			return;
		}

		trap_Argv(1, msg, sizeof (msg));

		if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1') {
			trap_SendServerCommand(ent - g_entities, "application -4");
			trap_SendServerCommand(ent->client->pers.applicationClient, "application -3");

			G_AddClientToFireteam(ent->client->pers.applicationClient, ent - g_entities);
		} else {
			trap_SendServerCommand(ent - g_entities, "application -4");
			trap_SendServerCommand(ent->client->pers.applicationClient, "application -2");
		}

		ent->client->pers.applicationEndTime = 0;
		ent->client->pers.applicationClient  = -1;

		return;
	}

	ent->client->pers.applicationEndTime = 0;
	ent->client->pers.applicationClient  = -1;

	if (ent->client->pers.invitationEndTime > level.time) {

		gclient_t *cl = g_entities[ent->client->pers.invitationClient].client;
		if (!cl) {
			return;
		}
		if (cl->pers.connected != CON_CONNECTED) {
			return;
		}

		trap_Argv(1, msg, sizeof (msg));

		if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1') {
			trap_SendServerCommand(ent - g_entities, "invitation -4");
			trap_SendServerCommand(ent->client->pers.invitationClient, "invitation -3");

			G_AddClientToFireteam(ent - g_entities, ent->client->pers.invitationClient);
		} else {
			trap_SendServerCommand(ent - g_entities, "invitation -4");
			trap_SendServerCommand(ent->client->pers.invitationClient, "invitation -2");
		}

		ent->client->pers.invitationEndTime = 0;
		ent->client->pers.invitationClient  = -1;

		return;
	}

	ent->client->pers.invitationEndTime = 0;
	ent->client->pers.invitationClient  = -1;

	if (ent->client->pers.propositionEndTime > level.time) {
		gclient_t *cl = g_entities[ent->client->pers.propositionClient].client;
		if (!cl) {
			return;
		}
		if (cl->pers.connected != CON_CONNECTED) {
			return;
		}

		trap_Argv(1, msg, sizeof (msg));

		if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1') {
			trap_SendServerCommand(ent - g_entities, "proposition -4");
			trap_SendServerCommand(ent->client->pers.propositionClient2, "proposition -3");

			G_InviteToFireTeam(ent - g_entities, ent->client->pers.propositionClient);
		} else {
			trap_SendServerCommand(ent - g_entities, "proposition -4");
			trap_SendServerCommand(ent->client->pers.propositionClient2, "proposition -2");
		}

		ent->client->pers.propositionEndTime = 0;
		ent->client->pers.propositionClient  = -1;
		ent->client->pers.propositionClient2 = -1;

		return;
	}

	ent->client->pers.propositionEndTime = 0;
	ent->client->pers.propositionClient  = -1;
	ent->client->pers.propositionClient2 = -1;

	// dhm
	// Reset this ent's complainEndTime so they can't send multiple complaints

	if (!level.voteInfo.voteTime) {
		trap_SendServerCommand(ent - g_entities, "print \"No vote in progress.\n\"");
		return;
	}
	if (ent->client->ps.eFlags & EF_VOTED) {
		trap_SendServerCommand(ent - g_entities, "print \"Vote already cast.\n\"");
		return;
	}

	if (level.voteInfo.vote_fn == G_Kick_v) {
		int pid = atoi(level.voteInfo.vote_value);
		if (!g_entities[pid].client) {
			return;
		}

		if (g_entities[pid].client->sess.sessionTeam != TEAM_SPECTATOR && ent->client->sess.sessionTeam != g_entities[pid].client->sess.sessionTeam) {
			trap_SendServerCommand(ent - g_entities, "print \"Cannot vote to kick player on opposing team.\n\"");
			return;
		}
	}

	trap_SendServerCommand(ent - g_entities, "print \"Vote cast.\n\"");

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv(1, msg, sizeof (msg));

	if (msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1') {
		level.voteInfo.voteYes++;
		trap_SetConfigstring(CS_VOTE_YES, va("%i", level.voteInfo.voteYes));
	} else {
		level.voteInfo.voteNo++;
		trap_SetConfigstring(CS_VOTE_NO, va("%i", level.voteInfo.voteNo));
	}

	// Nico, need to recompute
	CalculateRanks();

	// a majority will be determined in G_CheckVote, which will also account
	// for players entering or leaving
}

/*
=================
Cmd_StartCamera_f
=================
*/
void Cmd_StartCamera_f(gentity_t *ent) {

	if (ent->client->cameraPortal) {
		G_FreeEntity(ent->client->cameraPortal);
	}
	ent->client->cameraPortal = G_Spawn();

	ent->client->cameraPortal->s.eType           = ET_CAMERA;
	ent->client->cameraPortal->s.apos.trType     = TR_STATIONARY;
	ent->client->cameraPortal->s.apos.trTime     = 0;
	ent->client->cameraPortal->s.apos.trDuration = 0;
	VectorClear(ent->client->cameraPortal->s.angles);
	VectorClear(ent->client->cameraPortal->s.apos.trDelta);
	G_SetOrigin(ent->client->cameraPortal, ent->r.currentOrigin);
	VectorCopy(ent->r.currentOrigin, ent->client->cameraPortal->s.origin2);

	ent->client->cameraPortal->s.frame = 0;

	ent->client->cameraPortal->r.svFlags     |= (SVF_PORTAL | SVF_SINGLECLIENT);
	ent->client->cameraPortal->r.singleClient = ent->client->ps.clientNum;

	ent->client->ps.eFlags |= EF_VIEWING_CAMERA;
	ent->s.eFlags          |= EF_VIEWING_CAMERA;

	VectorCopy(ent->r.currentOrigin, ent->client->cameraOrigin);    // backup our origin
}

/*
=================
Cmd_StopCamera_f
=================
*/
void Cmd_StopCamera_f(gentity_t *ent) {
	if (ent->client->cameraPortal && (ent->client->ps.eFlags & EF_VIEWING_CAMERA)) {

		// go back into noclient mode
		G_FreeEntity(ent->client->cameraPortal);
		ent->client->cameraPortal = NULL;

		ent->s.eFlags          &= ~EF_VIEWING_CAMERA;
		ent->client->ps.eFlags &= ~EF_VIEWING_CAMERA;
	}
}

/*
=================
Cmd_SetCameraOrigin_f
=================
*/
void Cmd_SetCameraOrigin_f(gentity_t *ent) {
	char   buffer[MAX_TOKEN_CHARS];
	int    i;
	vec3_t origin;

	if (trap_Argc() != 4) {
		return;
	}

	for (i = 0 ; i < 3 ; ++i) {
		trap_Argv(i + 1, buffer, sizeof (buffer));
		origin[i] = atof(buffer);
	}

	if (ent->client->cameraPortal) {
		VectorCopy(origin, ent->client->cameraPortal->s.origin2);
		trap_LinkEntity(ent->client->cameraPortal);
	}
}

extern vec3_t playerMins;
extern vec3_t playerMaxs;

qboolean G_TankIsOccupied(gentity_t *ent) {
	if (!ent->tankLink) {
		return qfalse;
	}

	return qtrue;
}

qboolean G_TankIsMountable(gentity_t *ent, gentity_t *other) {
	if (!(ent->spawnflags & 128)) {
		return qfalse;
	}

	if (level.disableTankEnter) {
		return qfalse;
	}

	if (G_TankIsOccupied(ent)) {
		return qfalse;
	}

	if (ent->health <= 0) {
		return qfalse;
	}

	if (other->client->ps.weaponDelay) {
		return qfalse;
	}

	return qtrue;
}

// TAT 1/14/2003 - extracted out the functionality of Cmd_Activate_f from finding the object to use
//		so we can force bots to use items, without worrying that they are looking EXACTLY at the target
qboolean Do_Activate_f(gentity_t *ent, gentity_t *traceEnt) {
	qboolean found   = qfalse;
	qboolean walking = qfalse;

	// Arnout: invisible entities can't be used

	if (traceEnt->entstate == STATE_INVISIBLE || traceEnt->entstate == STATE_UNDERCONSTRUCTION) {
		return qfalse;
	}

	if (ent->client->pers.cmd.buttons & BUTTON_WALKING) {
		walking = qtrue;
	}

	if (traceEnt->classname) {
		traceEnt->flags &= ~FL_SOFTACTIVATE;    // FL_SOFTACTIVATE will be set if the user is holding 'walk' key

		if (traceEnt->s.eType == ET_ALARMBOX) {
			trace_t trace;

			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
				return qfalse;
			}

			memset(&trace, 0, sizeof (trace));

			if (traceEnt->use) {
				G_UseEntity(traceEnt, ent, 0);
			}
			found = qtrue;
		} else if (traceEnt->s.eType == ET_ITEM) {
			trace_t trace;

			if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
				return qfalse;
			}

			memset(&trace, 0, sizeof (trace));

			if (traceEnt->touch) {
				if (ent->client->pers.autoActivate == PICKUP_ACTIVATE) {
					ent->client->pers.autoActivate = PICKUP_FORCE;      //----(SA) force pickup
				}
				traceEnt->active = qtrue;
				traceEnt->touch(traceEnt, ent, &trace);
			}

			found = qtrue;
		} else if (traceEnt->s.eType == ET_MOVER && G_TankIsMountable(traceEnt, ent)) {
			G_Script_ScriptEvent(traceEnt, "mg42", "mount");
			ent->tagParent = traceEnt->nextTrain;
			Q_strncpyz(ent->tagName, "tag_player", MAX_QPATH);
			ent->backupWeaponTime                   = ent->client->ps.weaponTime;
			ent->client->ps.weaponTime              = traceEnt->backupWeaponTime;
			ent->client->ps.weapHeat[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

			ent->tankLink      = traceEnt;
			traceEnt->tankLink = ent;

			G_ProcessTagConnect(ent, qtrue);
			found = qtrue;
		} else if (G_EmplacedGunIsMountable(traceEnt, ent)) {
			gclient_t *cl = &level.clients[ent->s.clientNum];
			vec3_t    point, forward;

			AngleVectors(traceEnt->s.apos.trBase, forward, NULL, NULL);
			VectorMA(traceEnt->r.currentOrigin, -36, forward, point);
			point[2] = ent->r.currentOrigin[2];

			// Save initial position
			VectorCopy(point, ent->TargetAngles);

			// Zero out velocity
			VectorCopy(vec3_origin, ent->client->ps.velocity);
			VectorCopy(vec3_origin, ent->s.pos.trDelta);

			traceEnt->active     = qtrue;
			ent->active          = qtrue;
			traceEnt->r.ownerNum = ent->s.number;
			VectorCopy(traceEnt->s.angles, traceEnt->TargetAngles);
			traceEnt->s.otherEntityNum = ent->s.number;

			cl->pmext.harc = traceEnt->harc;
			cl->pmext.varc = traceEnt->varc;
			VectorCopy(traceEnt->s.angles, cl->pmext.centerangles);
			cl->pmext.centerangles[PITCH] = AngleNormalize180(cl->pmext.centerangles[PITCH]);
			cl->pmext.centerangles[YAW]   = AngleNormalize180(cl->pmext.centerangles[YAW]);
			cl->pmext.centerangles[ROLL]  = AngleNormalize180(cl->pmext.centerangles[ROLL]);

			ent->backupWeaponTime                   = ent->client->ps.weaponTime;
			ent->client->ps.weaponTime              = traceEnt->backupWeaponTime;
			ent->client->ps.weapHeat[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

			G_UseTargets(traceEnt, ent);     //----(SA)	added for Mike so mounting an MG42 can be a trigger event (let me know if there's any issues with this)
			found = qtrue;
		} else if ((Q_stricmp(traceEnt->classname, "func_door") == 0) || (Q_stricmp(traceEnt->classname, "func_door_rotating") == 0)) {
			if (walking) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			G_TryDoor(traceEnt, ent);        // (door,other,activator)
			found = qtrue;
		} else if (Q_stricmp(traceEnt->classname, "team_WOLF_checkpoint") == 0) {
			if (traceEnt->count != (int)ent->client->sess.sessionTeam) {
				traceEnt->health++;
			}
			found = qtrue;
		} else if ((Q_stricmp(traceEnt->classname, "func_button") == 0) && (traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY) && traceEnt->active == qfalse) {
			Use_BinaryMover(traceEnt, ent, ent);
			traceEnt->active = qtrue;
			found            = qtrue;
		} else if (!Q_stricmp(traceEnt->classname, "func_invisible_user")) {
			if (walking) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			G_UseEntity(traceEnt, ent, ent);
			found = qtrue;
		} else if (!Q_stricmp(traceEnt->classname, "props_footlocker")) {
			G_UseEntity(traceEnt, ent, ent);
			found = qtrue;
		}
	}

	return found;
}

void G_LeaveTank(gentity_t *ent, qboolean position) {
	gentity_t *tank;
	trace_t   tr;

	tank = ent->tankLink;
	if (!tank) {
		return;
	}

	if (position) {
		vec3_t axis[3], pos;

		AnglesToAxis(tank->s.angles, axis);
		VectorMA(ent->client->ps.origin, 128, axis[1], pos);
		trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

		if (tr.startsolid) {
			// try right
			VectorMA(ent->client->ps.origin, -128, axis[1], pos);
			trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

			if (tr.startsolid) {
				// try back
				VectorMA(ent->client->ps.origin, -224, axis[0], pos);
				trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

				if (tr.startsolid) {
					// try front
					VectorMA(ent->client->ps.origin, 224, axis[0], pos);
					trap_Trace(&tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID);

					if (tr.startsolid) {
						// give up
						return;
					}
				}
			}
		}

		VectorClear(ent->client->ps.velocity);   // Gordon: dont want them to fly away ;D
		TeleportPlayer(ent, pos, ent->client->ps.viewangles);
	}

	tank->mg42weapHeat         = ent->client->ps.weapHeat[WP_DUMMY_MG42];
	tank->backupWeaponTime     = ent->client->ps.weaponTime;
	ent->client->ps.weaponTime = ent->backupWeaponTime;

	G_Script_ScriptEvent(tank, "mg42", "unmount");
	ent->tagParent          = NULL;
	*ent->tagName           = '\0';
	ent->s.eFlags          &= ~EF_MOUNTEDTANK;
	ent->client->ps.eFlags &= ~EF_MOUNTEDTANK;
	tank->s.powerups        = -1;

	tank->tankLink = NULL;
	ent->tankLink  = NULL;
}

void Cmd_Activate_f(gentity_t *ent) {
	trace_t   tr;
	vec3_t    end;
	gentity_t *traceEnt;
	vec3_t    forward, right, up, offset;
	qboolean  found = qfalse;
	qboolean  pass2 = qfalse;
	int       i;

	if (ent->health <= 0) {
		return;
	}

	if (ent->s.weapon == WP_MORTAR_SET || ent->s.weapon == WP_MOBILE_MG42_SET) {
		return;
	}

	if (ent->active) {
		if (ent->client->ps.persistant[PERS_HWEAPON_USE]) {
			// DHM - Nerve :: Restore original position if current position is bad
			trap_Trace(&tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, MASK_PLAYERSOLID);
			if (tr.startsolid) {
				VectorCopy(ent->TargetAngles, ent->client->ps.origin);
				VectorCopy(ent->TargetAngles, ent->r.currentOrigin);
				ent->r.contents = CONTENTS_CORPSE;      // DHM - this will correct itself in ClientEndFrame
			}

			ent->client->ps.eFlags &= ~EF_MG42_ACTIVE;          // DHM - Nerve :: unset flag
			ent->client->ps.eFlags &= ~EF_AAGUN_ACTIVE;

			ent->client->ps.persistant[PERS_HWEAPON_USE] = 0;
			ent->active                                  = qfalse;

			for (i = 0; i < level.num_entities; ++i) {
				if (g_entities[i].s.eType == ET_MG42_BARREL && g_entities[i].r.ownerNum == ent->s.number) {
					g_entities[i].mg42weapHeat     = ent->client->ps.weapHeat[WP_DUMMY_MG42];
					g_entities[i].backupWeaponTime = ent->client->ps.weaponTime;
					break;
				}
			}
			ent->client->ps.weaponTime = ent->backupWeaponTime;
		} else {
			ent->active = qfalse;
		}
		return;
	} else if (ent->client->ps.eFlags & EF_MOUNTEDTANK && ent->s.eFlags & EF_MOUNTEDTANK && !level.disableTankExit) {
		G_LeaveTank(ent, qtrue);
		return;
	}

	AngleVectors(ent->client->ps.viewangles, forward, right, up);

	VectorCopy(ent->client->ps.origin, offset);
	offset[2] += ent->client->ps.viewheight;

	// lean
	if (ent->client->ps.leanf) {
		VectorMA(offset, ent->client->ps.leanf, right, offset);
	}

	VectorMA(offset, 96, forward, end);

	trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE));

	if (tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD) {
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MISSILECLIP | CONTENTS_TRIGGER));
		pass2 = qtrue;
	}

tryagain:

	if (tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD) {
		return;
	}

	traceEnt = &g_entities[tr.entityNum];

	found = Do_Activate_f(ent, traceEnt);

	if (!found && !pass2) {
		pass2 = qtrue;
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
		goto tryagain;
	}
}

void Cmd_Activate2_f(gentity_t *ent) {
	trace_t  tr;
	vec3_t   end;
	vec3_t   forward, right, up, offset;
	qboolean pass2 = qfalse;

	if (ent->client->sess.playerType != PC_COVERTOPS) {
		return;
	}

	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePointForActivate(ent, offset);
	VectorMA(offset, 96, forward, end);

	trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE));

	if (tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD) {
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
		pass2 = qtrue;
	}

tryagain:

	if (tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD) {
		return;
	}

	if (!pass2) {
		pass2 = qtrue;
		trap_Trace(&tr, offset, NULL, NULL, end, ent->s.number, (CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER));
		goto tryagain;
	}
}

void G_UpdateSpawnCounts(void) {
	int  i, j;
	char cs[MAX_STRING_CHARS];

	for (i = 0; i < level.numspawntargets; ++i) {
		int current, count = 0, team;

		trap_GetConfigstring(CS_MULTI_SPAWNTARGETS + i, cs, sizeof (cs));

		current = atoi(Info_ValueForKey(cs, "c"));
		team    = atoi(Info_ValueForKey(cs, "t")) & ~256;

		for (j = 0; j < level.numConnectedClients; ++j) {
			gclient_t *client = &level.clients[level.sortedClients[j]];

			if (client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES) {
				continue;
			}

			if ((int)client->sess.sessionTeam == team && client->sess.spawnObjectiveIndex == i + 1) {
				count++;
				continue;
			}

			if (client->sess.spawnObjectiveIndex == 0) {
				if (client->sess.sessionTeam == TEAM_AXIS) {
					if (level.axisAutoSpawn == i) {
						count++;
						continue;
					}
				} else {
					if (level.alliesAutoSpawn == i) {
						count++;
						continue;
					}
				}
			}
		}

		if (count == current) {
			continue;
		}

		Info_SetValueForKey(cs, "c", va("%i", count));
		trap_SetConfigstring(CS_MULTI_SPAWNTARGETS + i, cs);
	}
}

/*
============
Cmd_SetSpawnPoint_f
============
*/
void SetPlayerSpawn(gentity_t *ent, int spawn, qboolean update) {
	ent->client->sess.spawnObjectiveIndex = spawn;
	if (ent->client->sess.spawnObjectiveIndex >= MAX_MULTI_SPAWNTARGETS || ent->client->sess.spawnObjectiveIndex < 0) {
		ent->client->sess.spawnObjectiveIndex = 0;
	}

	if (update) {
		G_UpdateSpawnCounts();
	}
}

void Cmd_SetSpawnPoint_f(gentity_t *ent) {
	char arg[MAX_TOKEN_CHARS];
	int  val, i;

	if (trap_Argc() != 2) {
		return;
	}

	trap_Argv(1, arg, sizeof (arg));
	val = atoi(arg);

	if (ent->client) {
		SetPlayerSpawn(ent, val, qtrue);
	}

	for (i = 0; i < level.numLimboCams; ++i) {
		int x = (g_entities[level.limboCams[i].targetEnt].count - CS_MULTI_SPAWNTARGETS) + 1;
		if (level.limboCams[i].spawn && x == val) {
			VectorCopy(level.limboCams[i].origin, ent->s.origin2);
			ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
			break;
		}
	}
}

void Cmd_Ignore_f(gentity_t *ent) {
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof (cmd));

	if (!*cmd) {
		trap_SendServerCommand(ent - g_entities, "print \"usage: Ignore <clientname>.\n\"\n");
		return;
	}

	cnum = G_refClientnumForName(ent, cmd);

	if (cnum != MAX_CLIENTS) {
		COM_BitSet(ent->client->sess.ignoreClients, cnum);
	}
}

void Cmd_UnIgnore_f(gentity_t *ent) {
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof (cmd));

	if (!*cmd) {
		trap_SendServerCommand(ent - g_entities, "print \"usage: Unignore <clientname>.\n\"\n");
		return;
	}

	cnum = G_refClientnumForName(ent, cmd);

	if (cnum != MAX_CLIENTS) {
		COM_BitClear(ent->client->sess.ignoreClients, cnum);
	}
}

void Cmd_Load_f(gentity_t *ent) {
	int             argc;
	int             posNum;
	save_position_t *pos;

	// get save slot (do this first so players can get usage message even if
	// they are not allowed to use this command)
	argc = trap_Argc();
	if (argc == 1) {
		posNum = 0;
	} else if (argc == 2) {
		char cmd[MAX_TOKEN_CHARS];

		trap_Argv(1, cmd, sizeof (cmd));
		if ((posNum = atoi(cmd)) < 0 || posNum >= MAX_SAVED_POSITIONS) {
			CP("print \"Invalid position!\n\"");
			return;
		}
	} else {
		CP("print \"usage: load [position]\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		CP("cp \"You can not load as a spectator!\n\"");
		return;
	}

	if (ent->client->ps.eFlags & EF_PRONE) {
		CP("cp \"You can not load while proning!\n\"");
		return;
	}

	// suburb, prevent trigger bug
	if (level.time - ent->client->pers.lastLoadedTime < 550) {
		CP("cp \"Loading aborted\n\"");
		return;
	}
	ent->client->pers.lastLoadedTime = 0;

	if (ent->client->sess.sessionTeam == TEAM_ALLIES) {
		pos = ent->client->sess.alliesSaves + posNum;
	} else {
		pos = ent->client->sess.axisSaves + posNum;
	}

	if (pos->valid) {
		// Nico, don't stop timer on load for VET except if strict save/load mode is enabled
		if (ent->client->sess.timerunActive && (physics.integer != PHYSICS_MODE_VET || g_strictSaveLoad.integer != 0)) {
			// Nico, notify the client and its spectators the timerun has stopped
			notify_timerun_stop(ent, 0);

			ent->client->sess.timerunActive = qfalse;
		}

		VectorCopy(pos->origin, ent->client->ps.origin);

		// Nico, load angles if cg_loadViewAngles = 1
		if (ent->client->pers.loadViewAngles) {
			SetClientViewAngle(ent, pos->vangles);
		}

		// Nico, load saved weapon if cg_loadWeapon = 1
		if (ent->client->pers.loadWeapon) {
			ent->client->ps.weapon = pos->weapon;
			// Nico, inform client the need to change weapon
			trap_SendServerCommand(ent - g_entities, va("weaponUpdate %d", pos->weapon));
		}

		VectorClear(ent->client->ps.velocity);

		if (ent->client->ps.stats[STAT_HEALTH] < 100 && ent->client->ps.stats[STAT_HEALTH] > 0) {
			ent->health = 100;
		}

		if (level.rocketRun && ent->client->ps.weapon == WP_PANZERFAUST) {
			ent->client->ps.ammoclip[WP_PANZERFAUST] = level.rocketRun;
		}

		if (posNum == 0) {
			CP("cp \"Loaded\n\"");
		} else {
			CP(va("cp \"Loaded ^z%d\n\"", posNum));
		}

		// Start recording a new temp demo.
		trap_SendServerCommand(ent - g_entities, "tempDemoStart");
	} else {
		CP("cp \"Use save first!\n\"");
	}
}

void Cmd_Save_f(gentity_t *ent) {
	int             argc;
	int             posNum;
	save_position_t *pos;

	// get save slot (do this first so players can get usage message even if
	// they are not allowed to use this command)
	argc = trap_Argc();
	if (argc == 1) {
		posNum = 0;
	} else if (argc == 2) {
		char cmd[MAX_TOKEN_CHARS];

		trap_Argv(1, cmd, sizeof (cmd));
		if ((posNum = atoi(cmd)) < 0 || posNum >= MAX_SAVED_POSITIONS) {
			CP("print \"Invalid position!\n\"");
			return;
		}
	} else {
		CP("print \"usage: save [position]\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		CP("cp \"You can not save as a spectator!\n\"");
		return;
	}

	// Nico, allow save in the air for VET
	if (physics.integer != PHYSICS_MODE_VET && ent->client->ps.groundEntityNum == ENTITYNUM_NONE) {
		CP("cp \"You can not save while in the air!\n\"");
		return;
	}

	// Nico, allow save while proning for VET
	if (physics.integer != PHYSICS_MODE_VET && (ent->client->ps.eFlags & EF_PRONE || ent->client->ps.eFlags & EF_PRONE_MOVING)) {
		CP("cp \"You can not save while proning!\n\"");
		return;
	}

	// Nico, allow save while crouching for VET
	if (physics.integer != PHYSICS_MODE_VET && ent->client->ps.eFlags & EF_CROUCHING) {
		CP("cp \"You can not save while crouching!\n\"");
		return;
	}

	// suburb, forbid save while proning for VET before starting a run
	if (physics.integer == PHYSICS_MODE_VET && (ent->client->ps.eFlags & EF_PRONE || ent->client->ps.eFlags & EF_PRONE_MOVING) && !ent->client->sess.timerunActive) {
		CP("cp \"You can not save while proning before starting a run!\n\"");
		return;
	}

	// suburb, forbid save while crouching for VET before starting a run
	if (physics.integer == PHYSICS_MODE_VET && ent->client->ps.eFlags & EF_CROUCHING && !ent->client->sess.timerunActive) {
		CP("cp \"You can not save while crouching before starting a run!\n\"");
		return;
	}

	// Nico, strict save/load restrictions: you can not save while timer is active
	if (g_strictSaveLoad.integer != 0 && ent->client->sess.timerunActive) {
		CP("cp \"Strict save mode prevents you from saving while your timer is active!\n\"");
		return;
	}

	// suburb, prevent trigger bug
	if (ent->client->pers.isTouchingTrigger == qtrue && ent->client->sess.timerunActive) {
		CP("cp \"You can not save in triggers during a run!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_ALLIES) {
		pos = ent->client->sess.alliesSaves + posNum;
	} else {
		pos = ent->client->sess.axisSaves + posNum;
	}

	VectorCopy(ent->client->ps.origin, pos->origin);
	VectorCopy(ent->client->ps.viewangles, pos->vangles);
	pos->valid = qtrue;

	// Nico, save player weapon
	pos->weapon = ent->client->ps.weapon;

	if (posNum == 0) {
		CP("cp \"Saved\n\"");
	} else {
		CP(va("cp \"Saved ^z%d\n\"", posNum));
	}
}

// Nico, defines commands that are flood protected or not
static command_t floodProtectedCommands[] =
{
	{ "score",           qfalse, Cmd_Score_f,           qfalse, NULL,                                       NULL                                           },
	{ "vote",            qtrue,  Cmd_Vote_f,            qfalse, NULL,                                       NULL                                           },
	{ "fireteam",        qfalse, Cmd_FireTeam_MP_f,     qfalse, NULL,                                       NULL                                           },
	{ "rconauth",        qfalse, Cmd_AuthRcon_f,        qfalse, NULL,                                       NULL                                           },
	{ "ignore",          qfalse, Cmd_Ignore_f,          qfalse, NULL,                                       NULL                                           },
	{ "unignore",        qfalse, Cmd_UnIgnore_f,        qfalse, NULL,                                       NULL                                           },
	{ "rs",              qfalse, Cmd_ResetSetup_f,      qfalse, NULL,                                       NULL                                           },
	{ "noclip",          qfalse, Cmd_Noclip_f,          qfalse, NULL,                                       NULL                                           },
	{ "kill",            qtrue,  Cmd_Kill_f,            qfalse, NULL,                                       NULL                                           },
	{ "team",            qtrue,  Cmd_Team_f,            qfalse, NULL,                                       NULL                                           },
	{ "stopcamera",      qfalse, Cmd_StopCamera_f,      qfalse, NULL,                                       NULL                                           },
	{ "setcameraorigin", qfalse, Cmd_SetCameraOrigin_f, qfalse, NULL,                                       NULL                                           },
	{ "setspawnpt",      qfalse, Cmd_SetSpawnPoint_f,   qtrue,  "Allows you to choose a spawn point",       "spawnId"                                      },
	{ "load",            qfalse, Cmd_Load_f,            qtrue,  "Allows you to load a saved position",      "[slot]"                                       },
	{ "save",            qfalse, Cmd_Save_f,            qtrue,  "Allows you to save your current position", "[slot]"                                       },

	// Nico, class command
	{ "class",           qtrue,  Cmd_Class_f,           qtrue,  "Allows you to change your current class",  "class [weapon1] [weapon2]"                    },

	// Nico, private messages
	{ "m",               qtrue,  Cmd_PrivateMessage_f,  qtrue,  "Allows you send private messages",         "dest message"                                 },

	// ETrun specific commands
	{ "login",           qtrue,  Cmd_Login_f,           qtrue,  "Allows you to login via timeruns.net",     NULL                                           },
	{ "logout",          qtrue,  Cmd_Logout_f,          qtrue,  "Allows you to logout",                     NULL                                           },
	{ "records",         qtrue,  Cmd_Records_f,         qtrue,  "Displays current map #1 records",          NULL                                           },
	{ "rank",            qtrue,  Cmd_Rank_f,            qtrue,  "Shows rankings for given options",         "[userName] [mapName] [runName] [physicsName]" },
	{ "loadCheckpoints", qtrue,  Cmd_LoadCheckpoints_f, qtrue,  "Loads checkpoints from your PB",           "[userName] [run name or id]"                  },
	{ "h",               qtrue,  Cmd_Help_f,            qtrue,  "Shows help message",                       "[command]"                                    },
	{ "abort",           qtrue,  Cmd_Abort_f,           qtrue,  "Aborts the current run",                   NULL                                           },
	{ "tutorial",        qtrue,  Cmd_Tutorial_f,        qtrue,  "Shows an introduction for beginners",      NULL                                           },
};
// Nico, end of defines commands that are flood protected or not

/*
=================
ClientCommand
=================
*/
void ClientCommand(int clientNum) {
	gentity_t *ent;
	char      cmd[MAX_TOKEN_CHARS];
	int       i   = 0;
	qboolean  enc = qfalse;   // used for enc_say, enc_say_team, enc_say_buddy

	ent = g_entities + clientNum;
	if (!ent->client) {
		return;     // not fully in game yet
	}

	trap_Argv(0, cmd, sizeof (cmd));

	if (!Q_stricmp(cmd, "say") || (enc = !Q_stricmp(cmd, "enc_say")) != 0) {

		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping say\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Say_f(ent, SAY_ALL, qfalse, enc);
		}
		return;
	}

	if (!Q_stricmp(cmd, "say_team") || (enc = !Q_stricmp(cmd, "enc_say_team")) != 0) {
		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping say_team\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Say_f(ent, SAY_TEAM, qfalse, enc);
		}
		return;
	} else if (!Q_stricmp(cmd, "vsay")) {

		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping vsay\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Voice_f(ent, SAY_ALL, qfalse, qfalse);
		}
		return;
	} else if (!Q_stricmp(cmd, "vsay_team")) {
		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping vsay_team\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Voice_f(ent, SAY_TEAM, qfalse, qfalse);
		}
		return;
	} else if (!Q_stricmp(cmd, "say_buddy") || (enc = !Q_stricmp(cmd, "enc_say_buddy")) != 0) {

		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping say_buddy\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Say_f(ent, SAY_BUDDY, qfalse, enc);
		}
		return;
	} else if (!Q_stricmp(cmd, "vsay_buddy")) {

		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping vsay_buddy\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Voice_f(ent, SAY_BUDDY, qfalse, qfalse);
		}
		return;
	} else if (!Q_stricmp(cmd, "follownext")) {
		Cmd_FollowCycle_f(ent, 1);
	} else if (!Q_stricmp(cmd, "followprev")) {
		Cmd_FollowCycle_f(ent, -1);
	} else if (!Q_stricmp(cmd, "mod_information")) { // suburb, added mod info printout
		CP(va("print \"%s %s\n\"", GAME_VERSION " " MOD_VERSION, BUILD_TIME));
		return;
	}

	// Nico, flood protection
	for (i = 0 ; i < (int)(sizeof (floodProtectedCommands) / sizeof (floodProtectedCommands[0])) ; ++i) {
		if (!Q_stricmp(cmd, floodProtectedCommands[i].cmd)) {
			if (floodProtectedCommands[i].isProtected && ClientIsFlooding(ent)) {
				CP(va("print \"^1Spam Protection: ^7dropping %s\n\"", cmd));
			} else {
				floodProtectedCommands[i].function(ent);
			}
			return;
		}
	}

	if (G_commandCheck(ent, cmd)) {
		return;
	}

	CP(va("print \"Unknown command %s^7.\n\"", cmd));
}

/*
 * Nico, anti-flood from ETpub
 * Returns qtrue if user requested too many commands in past time, otherwise
 * qfalse.
 */
qboolean ClientIsFlooding(gentity_t *ent) {
	if (!ent->client || !g_floodProtect.integer) {
		return qfalse;
	}

	if (level.time - ent->client->sess.thresholdTime > 30000) {
		ent->client->sess.thresholdTime = level.time;
	}

	if (level.time < ent->client->sess.nextReliableTime) {
		return qtrue;
	}

	if (level.time - ent->client->sess.thresholdTime <= 30000
	    && ent->client->sess.numReliableCmds > g_floodThreshold.integer) {
		ent->client->sess.nextReliableTime = level.time + g_floodWait.integer;
		return qtrue;
	}

	ent->client->sess.numReliableCmds++;
	// delay between each command (values >0 break batch of commands)
	ent->client->sess.nextReliableTime = level.time + 0;

	return qfalse;
}

/**
 * Locks/unlocks a client from spectators.
 * @source: TJMod
 */
void Cmd_SpecLock_f(gentity_t *ent, unsigned int dwCommand, qboolean lock) {
	int i = 0;

	// Nico, silent GCC
	(void)dwCommand;

	if (ent->client->sess.specLocked == lock) {
		CP(va("print \"You are already %slocked from spectators!\n\"", lock ? "" : "un"));
		return;
	}

	ent->client->sess.specLocked = lock;

	// Nico, update cg_specLock client-side
	trap_SendServerCommand(ent - g_entities, va("updateSpecLockStatus %d", lock));

	// unlocked
	if (!ent->client->sess.specLocked) {
		CP("cpm \"You are now unlocked from spectators!\n\"");
		return;
	}

	// locked
	CP("cpm \"You are now locked from spectators!\n\"");
	CP("cpm \"Use ^3specinvite^7 to invite people to spectate.\n\"");

	// update following players
	// suburb, don't use the level.sortedClients[] array for looping because the StopFollowing() function
	// will indirectly cause this array to be resorted, thus causing some clients not to unfollow
	for (i = 0; i < level.maxclients; ++i) {
		if (level.clients[i].pers.connected == CON_DISCONNECTED) {
			continue;
		}

		gentity_t *other = g_entities + i;

		if (other->client->sess.referee) {
			continue;
		}

		if (other->client->sess.sessionTeam != TEAM_SPECTATOR) {
			continue;
		}

		if (other->client->sess.spectatorState == SPECTATOR_FOLLOW
		    && other->client->sess.spectatorClient == ent - g_entities
		    && !G_AllowFollow(other, ent) && !other->client->sess.freeSpec) {
			StopFollowing(other);
		}
	}
}

/**
 * Sends an invitation to a client to spectate him.
 * @source: TJMod
 */
void Cmd_SpecInvite_f(gentity_t *ent, unsigned int dwCommand, qboolean invite) {
	int       clientNum            = 0;
	gentity_t *other               = NULL;
	char      arg[MAX_TOKEN_CHARS] = { 0 };

	// Nico, silent GCC
	(void)dwCommand;

	if (ClientIsFlooding(ent)) {
		CP("print \"^1Spam Protection:^7 Specinvite ignored\n\"");
		return;
	}

	// find the client to invite
	trap_Argv(1, arg, sizeof (arg));
	if ((clientNum = ClientNumberFromString(ent, arg)) == -1) {
		return;
	}

	other = g_entities + clientNum;

	// can't invite self
	if (other == ent) {
		CP(va("print \"You can not spec%sinvite yourself!\n\"", invite ? "" : "un"));
		return;
	}

	if (invite) {
		if (COM_BitCheck(ent->client->sess.specInvitedClients, clientNum)) {
			CP(va("print \"%s^7 is already specinvited.\n\"", other->client->pers.netname));
			return;
		}
		COM_BitSet(ent->client->sess.specInvitedClients, clientNum);

		CP(va("print \"%s^7 has been sent a spectator invitation.\n\"", other->client->pers.netname));
		CPx(other - g_entities, va("cpm \"You have been invited to spectate %s^7.\n\"", ent->client->pers.netname));
	} else {
		if (!COM_BitCheck(ent->client->sess.specInvitedClients, clientNum)) {
			CP(va("print \"%s^7 is not specinvited.\n\"", other->client->pers.netname));
			return;
		}

		COM_BitClear(ent->client->sess.specInvitedClients, clientNum);
		if (other->client->sess.spectatorState == SPECTATOR_FOLLOW
		    && other->client->sess.spectatorClient == ent - g_entities
		    && !G_AllowFollow(other, ent)) {
			StopFollowing(other);
		}

		CP(va("print \"%s^7 was removed from invited spectators.\n\"", other->client->pers.netname));
		CPx(other - g_entities, va("cpm \"You have been uninvited to spectate %s^7.\n\"", ent->client->pers.netname));
	}
}

/**
 * @source: ETpub
 */
static char *Q_SayConcatArgs(int start) {
	char *s;
	int  c = 0;

	s = ConcatArgs(0);
	while (*s) {
		if (c == start) {
			return s;
		}
		if (*s == ' ') {
			s++;
			if (*s != ' ') {
				c++;
				continue;
			}
			while (*s && *s == ' ') {
				s++;
			}
			c++;
		}
		s++;
	}
	return s;
}

/**
 * Private message command
 * @source: TJMod
 */
void Cmd_PrivateMessage_f(gentity_t *ent) {
	int  pids[MAX_CLIENTS]        = { 0 };
	char name[MAX_NAME_LENGTH]    = { 0 };
	char netname[MAX_NAME_LENGTH] = { 0 };
	char cmd[12]                  = { 0 };
	char str[MAX_STRING_CHARS]    = { 0 };
	char *msg                     = NULL;
	int  pcount                   = 0;
	int  i                        = 0;

	trap_Argv(0, cmd, sizeof (cmd));

	if (trap_Argc() < 3) {
		CP(va("print \"usage: %s [name|slot#] [message]\n\"", cmd));
		return;
	}

	trap_Argv(1, name, sizeof (name));
	msg    = Q_SayConcatArgs(2);
	pcount = ClientNumbersFromString(name, pids);

	if (ent) {
		Q_strncpyz(netname, ent->client->pers.netname, sizeof (name));
	} else {
		Q_strncpyz(netname, "console", sizeof (name));
	}

	Q_strncpyz(str, va("^3sent to %i player%s: ^7", pcount, pcount == 1 ? "" : "s"), sizeof (str));

	for (i = 0; i < pcount; ++i) {
		gentity_t *tmpent = &g_entities[pids[i]];

		if (i > 0) {
			Q_strcat(str, sizeof (str), "^7, ");
		}
		Q_strcat(str, sizeof (str), tmpent->client->pers.netname);

		if (ent && COM_BitCheck(tmpent->client->sess.ignoreClients, ent - g_entities)) {
			CP(va("print \"%s^1 is ignoring you\n\"", tmpent->client->pers.netname));
			continue;
		}
		CPx(pids[i], va(
				"chat \"%s^3 -> ^7%s^7: (%d recipient%s): ^3%s^7\" %d",
				netname,
				name,
				pcount,
				pcount == 1 ? "" : "s",
				msg,
				ent ? (int)(ent - g_entities) : -1));
		CPx(pids[i], va("cp \"^3private message from ^7%s^7\"", netname));
	}

	if (!pcount) {
		CP(va("print \"^3No player matching ^7\'%s^7\' ^3to send message to.\n\"", name));
	} else {
		CP(va("print \"^3Private message: ^7%s\n\"", msg));
		CP(va("print \"%s\n\"", str));

		G_LogPrintf(qtrue, "privmsg: %s: %s: %s\n", netname, name, msg);
	}
}

// Nico, help command
void Cmd_Help_f(gentity_t *ent) {
	int argc = 0;
	int i    = 0;

	// Parse options
	argc = trap_Argc();
	if (argc <= 1) {
		CP(va("print \"  List of %s ^wcommands:\n\"", GAME_VERSION_COLORED));
		CP("print \"-------------------------------------------------------------------\n\"");
		for (i = 0; i < (int)(sizeof (floodProtectedCommands) / sizeof (floodProtectedCommands[0])); ++i) {
			if (floodProtectedCommands[i].inHelp == qtrue && floodProtectedCommands[i].desc) {
				CP(va("print \"  ^8%-15s ^w%s\n\"", floodProtectedCommands[i].cmd, floodProtectedCommands[i].desc));
			}
		}
		CP("print \"-------------------------------------------------------------------\n\"");
	} else {
		char option[MAX_QPATH] = { 0 };

		trap_Argv(1, option, sizeof (option));
		for (i = 0 ; i < (int)(sizeof (floodProtectedCommands) / sizeof (floodProtectedCommands[0])) ; ++i) {
			if (!Q_stricmp(option, floodProtectedCommands[i].cmd)) {
				if (floodProtectedCommands[i].inHelp == qtrue && floodProtectedCommands[i].desc) {
					CP(va("print \"  ^wCommand     ^8%-15s\n\"", floodProtectedCommands[i].cmd));
					CP(va("print \"  ^wDescription ^8%s\n\"", floodProtectedCommands[i].desc));

					// Nico, show usage message
					if (floodProtectedCommands[i].usage) {
						CP(va("print \"  ^wUsage       ^8/%s %s\n\"", floodProtectedCommands[i].cmd, floodProtectedCommands[i].usage));
					} else {
						CP(va("print \"  ^wUsage       ^8/%s\n\"", floodProtectedCommands[i].cmd));
					}
				} else {
					CP(va("print \"  ^1Error^w: unrecognised command ^8'%s'\n\"", option));
					CP("print \"  ^8Usage: /help [command]\n\"");
				}
			}
		}
	}
}

/**
* Abort current timerun command
* @author: suburb
*/
void Cmd_Abort_f(gentity_t *ent) {
	if (ent->client->sess.timerunActive) {
		// triggerbug fix
		if (ent->client->pers.isTouchingTrigger) {
			trap_SendServerCommand(ent - g_entities, va("print \"You can not abort in triggers.\n\""));
			return;
		}
		notify_timerun_stop(ent, 0);
		ent->client->sess.timerunActive = qfalse;
	}
}

/**
* Tutorial command for beginners
* @author: suburb
*/
void Cmd_Tutorial_f(gentity_t *ent) {
	CP("print \"^9-----------------------------------------------------------------------------\n\"");
	CP(va("print \"Welcome to %s^7, an Enemy Territory game modification with timeruns\n\"", GAME_VERSION_COLORED));
	CP("print \"support. In order to permanently save records, you need to create an\n\"");
	CP(va("print \"account on ^fhttps://timeruns.net/^7, the official %s^7 website, and\n\"", GAME_VERSION_COLORED));
	CP("print \"link it to your game. Here is a step-by-step tutorial:\n\"");
	CP("print \"\n\"");
	CP("print \"^51. ^7Go to ^fhttps://timeruns.net/ ^7and open the Signup tab.\n\"");
	CP("print \"^52. ^7Follow the instructions and wait for the account activation email.\n\"");
	CP(va("print \"^53. ^7Once your account has been activated, login on the %s^7 forum,\n\"", GAME_VERSION_COLORED));
	CP("print \"   which is located here: ^fhttps://forum.timeruns.net/\n\"");
	CP("print \"^54. ^7In the top right corner, click on your nickname and follow this path:\n\"");
	CP("print \"   User Control Panel -> Profile -> Edit account settings\n\"");
	CP("print \"   Now you can see your Timeruns token. This is your password which\n\"");
	CP("print \"   links your game to your own website account. Never share it!\n\"");
	CP("print \"^55. ^7Copy your Timeruns token.\n\"");
	CP("print \"^56. ^7Insert your Timeruns token ingame into the ^b/cg_timerunsToken ^7cvar.\n\"");
	CP("print \"^57. ^7Type ^b/login ^7into the console.\n\"");
	CP("print \"\n\"");
	CP("print \"Congratulations! You are now logged in and able to set records. You can\n\"");
	CP(va("print \"now find your stats on the %s^7 website and share them with your friends.\n\"", GAME_VERSION_COLORED));
	CP("print \"\n\"");
	CP("print \"Have fun.\n\"");
	CP("print \"^9-----------------------------------------------------------------------------\n\"");
}
