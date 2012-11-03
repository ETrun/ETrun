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


#include <limits.h>
#include "../../etrun/ui/menudef.h"
#include "g_local.h"

typedef struct teamgame_s {
	float last_flag_capture;
	int last_capture_team;
} teamgame_t;

teamgame_t teamgame;

void Team_InitGame(void) {
	memset(&teamgame, 0, sizeof teamgame);
}

int OtherTeam(int team) {
	if (team == TEAM_AXIS) {
		return TEAM_ALLIES;
	} else if (team == TEAM_ALLIES) {
		return TEAM_AXIS;
	}
	return team;
}

const char *TeamName(int team) {
	if (team == TEAM_AXIS) {
		return "RED";
	} else if (team == TEAM_ALLIES) {
		return "BLUE";
	} else if (team == TEAM_SPECTATOR) {
		return "SPECTATOR";
	}
	return "FREE";
}

const char *OtherTeamName(int team) {
	if (team == TEAM_AXIS) {
		return "BLUE";
	} else if (team == TEAM_ALLIES) {
		return "RED";
	} else if (team == TEAM_SPECTATOR) {
		return "SPECTATOR";
	}
	return "FREE";
}

const char *TeamColorString(int team) {
	if (team == TEAM_AXIS) {
		return S_COLOR_RED;
	} else if (team == TEAM_ALLIES) {
		return S_COLOR_BLUE;
	} else if (team == TEAM_SPECTATOR) {
		return S_COLOR_YELLOW;
	}
	return S_COLOR_WHITE;
}

// NULL for everyone
void QDECL PrintMsg(gentity_t *ent, const char *fmt, ...) {
	char    msg[1024];
	va_list argptr;
	char    *p;

	// NOTE: if buffer overflow, it's more likely to corrupt stack and crash than do a proper G_Error?
	va_start(argptr, fmt);
	if (Q_vsnprintf(msg, sizeof (msg), fmt, argptr) > (int)sizeof (msg)) {
		G_Error("PrintMsg overrun");
	}
	va_end(argptr);

	// double quotes are bad
	while ((p = strchr(msg, '"')) != NULL)
		*p = '\'';

	trap_SendServerCommand(((ent == NULL) ? -1 : ent - g_entities), va("print \"%s\"", msg));
}

/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2) {
	if (!ent1->client || !ent2->client) {
		return qfalse;
	}

	if (ent1->client->sess.sessionTeam == ent2->client->sess.sessionTeam) {
		return qtrue;
	}

	return qfalse;
}

// JPW NERVE moved these up
#define WCP_ANIM_NOFLAG             0
#define WCP_ANIM_RAISE_AXIS         1
#define WCP_ANIM_RAISE_AMERICAN     2
#define WCP_ANIM_AXIS_RAISED        3
#define WCP_ANIM_AMERICAN_RAISED    4
#define WCP_ANIM_AXIS_TO_AMERICAN   5
#define WCP_ANIM_AMERICAN_TO_AXIS   6
#define WCP_ANIM_AXIS_FALLING       7
#define WCP_ANIM_AMERICAN_FALLING   8
// jpw

/*
================
Team_FragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumlative.  You get one, they are in importance
order.
================
*/
void Team_FragBonuses(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker) {
	int       i;
	gentity_t *ent;
	int       flag_pw, enemy_flag_pw;
	int       otherteam;
	gentity_t *flag, *carrier = NULL;
	char      *c;
	vec3_t    v1, v2;
	int       team;

	// Nico, silent GCC
	inflictor = inflictor;

	// no bonus for fragging yourself
	if (!targ->client || !attacker->client || targ == attacker) {
		return;
	}

	team      = targ->client->sess.sessionTeam;
	otherteam = OtherTeam(targ->client->sess.sessionTeam);
	if (otherteam < 0) {
		return; // whoever died isn't on a team

	}
// JPW NERVE -- no bonuses for fragging friendlies, penalties scored elsewhere
	if (team == (int)attacker->client->sess.sessionTeam) {
		return;
	}
// jpw

	// same team, if the flag at base, check to he has the enemy flag
	if (team == TEAM_AXIS) {
		flag_pw       = PW_REDFLAG;
		enemy_flag_pw = PW_BLUEFLAG;
	} else {
		flag_pw       = PW_BLUEFLAG;
		enemy_flag_pw = PW_REDFLAG;
	}

	// did the attacker frag the flag carrier?
	if (targ->client->ps.powerups[enemy_flag_pw]) {
		attacker->client->pers.teamState.lastfraggedcarrier = level.time;
		attacker->client->pers.teamState.fragcarrier++;

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;
			if (ent->inuse && (int)ent->client->sess.sessionTeam == otherteam) {
				ent->client->pers.teamState.lasthurtcarrier = 0;
			}
		}
		return;
	}
	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

	// find the flag
	switch (attacker->client->sess.sessionTeam) {
	case TEAM_AXIS:
		c = "team_CTF_redflag";
		break;
	case TEAM_ALLIES:
		c = "team_CTF_blueflag";
		break;
	default:
		return;
	}

	flag = NULL;
	while ((flag = G_Find(flag, FOFS(classname), c)) != NULL) {
		if (!(flag->flags & FL_DROPPED_ITEM)) {
			break;
		}
	}

	if (flag) {   // JPW NERVE -- added some more stuff after this fn
		//		return; // can't find attacker's flag

		// find attacker's team's flag carrier
		for (i = 0; i < g_maxclients.integer; i++) {
			carrier = g_entities + i;
			if (carrier->inuse && carrier->client->ps.powerups[flag_pw]) {
				break;
			}
			carrier = NULL;
		}

		// ok we have the attackers flag and a pointer to the carrier

		// check to see if we are defending the base's flag
		VectorSubtract(targ->client->ps.origin, flag->s.origin, v1);
		VectorSubtract(attacker->client->ps.origin, flag->s.origin, v2);

		if ((VectorLengthSquared(v1) < SQR(CTF_TARGET_PROTECT_RADIUS) ||
		     VectorLengthSquared(v2) < SQR(CTF_TARGET_PROTECT_RADIUS) ||
		     CanDamage(flag, targ->client->ps.origin) || CanDamage(flag, attacker->client->ps.origin)) &&
		    attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {
			// we defended the base flag
			// JPW NERVE FIXME -- don't report flag defense messages, change to gooder message
			attacker->client->pers.teamState.basedefense++;
			return;
		}
	} // JPW NERVE

// JPW NERVE -- look for nearby checkpoints and spawnpoints
	flag = NULL;
	while ((flag = G_Find(flag, FOFS(classname), "team_WOLF_checkpoint")) != NULL) {
		VectorSubtract(targ->client->ps.origin, flag->s.origin, v1);
		if ((flag->s.frame != WCP_ANIM_NOFLAG) && (flag->count == (int)attacker->client->sess.sessionTeam)) {
			if (VectorLengthSquared(v1) < SQR(WOLF_CP_PROTECT_RADIUS)) {
			}
		}
	}
// jpw

}

/*
================
Team_CheckHurtCarrier

Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses for assistance to flag
carrier defense.
================
*/
void Team_CheckHurtCarrier(gentity_t *targ, gentity_t *attacker) {
	int flag_pw;

	if (!targ->client || !attacker->client) {
		return;
	}

	if (targ->client->sess.sessionTeam == TEAM_AXIS) {
		flag_pw = PW_BLUEFLAG;
	} else {
		flag_pw = PW_REDFLAG;
	}

	if (targ->client->ps.powerups[flag_pw] &&
	    targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam) {
		attacker->client->pers.teamState.lasthurtcarrier = level.time;
	}
}

void Team_ResetFlag(gentity_t *ent) {
	if (ent->flags & FL_DROPPED_ITEM) {
		Team_ResetFlag(&g_entities[ent->s.otherEntityNum]);
		G_FreeEntity(ent);
	} else {
		ent->s.density++;

		// do we need to respawn?
		if (ent->s.density == 1) {
			RespawnItem(ent);
		}
	}
}

void Team_ResetFlags(void) {
	gentity_t *ent;

	ent = NULL;
	while ((ent = G_Find(ent, FOFS(classname), "team_CTF_redflag")) != NULL) {
		Team_ResetFlag(ent);
	}

	ent = NULL;
	while ((ent = G_Find(ent, FOFS(classname), "team_CTF_blueflag")) != NULL) {
		Team_ResetFlag(ent);
	}
}

void Team_ReturnFlagSound(gentity_t *ent, int team) {
	// play powerup spawn sound to all clients
	gentity_t *pm;

	if (ent == NULL) {
		G_Printf("Warning:  NULL passed to Team_ReturnFlagSound\n");
		return;
	}

	pm                = G_PopupMessage(PM_OBJECTIVE);
	pm->s.effect3Time = G_StringIndex(ent->message);
	pm->s.effect2Time = team;
	pm->s.density     = 1; // 1 = returned
}

void Team_ReturnFlag(gentity_t *ent) {
	int team = ent->item->giTag == PW_REDFLAG ? TEAM_AXIS : TEAM_ALLIES;

	Team_ReturnFlagSound(ent, team);
	Team_ResetFlag(ent);
	PrintMsg(NULL, "The %s flag has returned!\n", TeamName(team));
}

/*
==============
Team_DroppedFlagThink

Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned when they time out
==============
*/
void Team_DroppedFlagThink(gentity_t *ent) {
	if (ent->item->giTag == PW_REDFLAG) {
		G_Script_ScriptEvent(&g_entities[ent->s.otherEntityNum], "trigger", "returned");

		Team_ReturnFlagSound(ent, TEAM_AXIS);
		Team_ResetFlag(ent);

		if (level.gameManager) {
			G_Script_ScriptEvent(level.gameManager, "trigger", "axis_object_returned");
		}

		// Nico, bugfix: removed left printf
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=058
		// trap_SendServerCommand( -1, "cp \"Axis have returned the objective!\" 2" );
	} else if (ent->item->giTag == PW_BLUEFLAG) {
		G_Script_ScriptEvent(&g_entities[ent->s.otherEntityNum], "trigger", "returned");

		Team_ReturnFlagSound(ent, TEAM_ALLIES);
		Team_ResetFlag(ent);

		if (level.gameManager) {
			G_Script_ScriptEvent(level.gameManager, "trigger", "allied_object_returned");
		}
	}
	// Reset Flag will delete this entity
}

int Team_TouchOurFlag(gentity_t *ent, gentity_t *other, int team) {
	gclient_t *cl = other->client;

	if (ent->flags & FL_DROPPED_ITEM) {
		// hey, its not home.  return it by teleporting it back
		if (cl->sess.sessionTeam == TEAM_AXIS) {
			if (level.gameManager) {
				G_Script_ScriptEvent(level.gameManager, "trigger", "axis_object_returned");
			}
			G_Script_ScriptEvent(&g_entities[ent->s.otherEntityNum], "trigger", "returned");
		} else {
			if (level.gameManager) {
				G_Script_ScriptEvent(level.gameManager, "trigger", "allied_object_returned");
			}
			G_Script_ScriptEvent(&g_entities[ent->s.otherEntityNum], "trigger", "returned");
		}
		// dhm
// jpw 800 672 2420
		other->client->pers.teamState.flagrecovery++;
		other->client->pers.teamState.lastreturnedflag = level.time;
		//ResetFlag will remove this entity!  We must return zero
		Team_ReturnFlagSound(ent, team);
		Team_ResetFlag(ent);
		return 0;
	}

	// DHM - Nerve :: GT_WOLF doesn't support capturing the flag
	return 0;
}

int Team_TouchEnemyFlag(gentity_t *ent, gentity_t *other, int team) {
	gclient_t *cl = other->client;
	gentity_t *tmp;

	ent->s.density--;

	tmp         = ent->parent;
	ent->parent = other;

	if (cl->sess.sessionTeam == TEAM_AXIS) {
		gentity_t *pm = G_PopupMessage(PM_OBJECTIVE);
		pm->s.effect3Time = G_StringIndex(ent->message);
		pm->s.effect2Time = TEAM_AXIS;
		pm->s.density     = 0; // 0 = stolen

		if (level.gameManager) {
			G_Script_ScriptEvent(level.gameManager, "trigger", "allied_object_stolen");
		}
		G_Script_ScriptEvent(ent, "trigger", "stolen");
	} else {
		gentity_t *pm = G_PopupMessage(PM_OBJECTIVE);
		pm->s.effect3Time = G_StringIndex(ent->message);
		pm->s.effect2Time = TEAM_ALLIES;
		pm->s.density     = 0; // 0 = stolen

		if (level.gameManager) {
			G_Script_ScriptEvent(level.gameManager, "trigger", "axis_object_stolen");
		}
		G_Script_ScriptEvent(ent, "trigger", "stolen");
	}

	ent->parent = tmp;

	if (team == TEAM_AXIS) {
		cl->ps.powerups[PW_REDFLAG] = INT_MAX;
	} else {
		cl->ps.powerups[PW_BLUEFLAG] = INT_MAX;
	} // flags never expire

	// store the entitynum of our original flag spawner
	if (ent->flags & FL_DROPPED_ITEM) {
		cl->flagParent = ent->s.otherEntityNum;
	} else {
		cl->flagParent = ent->s.number;
	}

	cl->pers.teamState.flagsince = level.time;

	other->client->speedScale = ent->splashDamage; // Alter player speed

	if (ent->s.density > 0) {
		return 1; // We have more flags to give out, spawn back quickly
	} else {
		return -1; // Do not respawn this automatically, but do delete it if it was FL_DROPPED
	}
}

int Pickup_Team(gentity_t *ent, gentity_t *other) {
	int       team;
	gclient_t *cl = other->client;

	// figure out what team this flag is
	if (strcmp(ent->classname, "team_CTF_redflag") == 0) {
		team = TEAM_AXIS;
	} else if (strcmp(ent->classname, "team_CTF_blueflag") == 0) {
		team = TEAM_ALLIES;
	} else {
		PrintMsg(other, "Don't know what team the flag is on.\n");
		return 0;
	}

	// JPW NERVE -- set flag model in carrying entity if multiplayer and flagmodel is set
	other->message           = ent->message;
	other->s.otherEntityNum2 = ent->s.modelindex2;
	// jpw

	return ((team == (int)cl->sess.sessionTeam) ?
	        Team_TouchOurFlag : Team_TouchEnemyFlag)
	           (ent, other, team);
}

/*---------------------------------------------------------------------------*/

// JPW NERVE
/*
=======================
FindFarthestObjectiveIndex

pick MP objective farthest from passed in vector, return table index
=======================
*/
int FindFarthestObjectiveIndex(vec3_t source) {
	int    i, j = 0;
	float  dist = 0, tdist;
	vec3_t tmp;

	for (i = 0; i < level.numspawntargets; i++) {
		VectorSubtract(level.spawntargets[i], source, tmp);
		tdist = VectorLength(tmp);
		if (tdist > dist) {
			dist = tdist;
			j    = i;
		}
	}

	return j;
}
// jpw

// NERVE - SMF
/*
=======================
FindClosestObjectiveIndex

NERVE - SMF - pick MP objective closest to the passed in vector, return table index
=======================
*/
int FindClosestObjectiveIndex(vec3_t source) {
	int    i, j = 0;
	float  dist = 10E20, tdist;
	vec3_t tmp;

	for (i = 0; i < level.numspawntargets; i++) {
		VectorSubtract(level.spawntargets[i], source, tmp);
		tdist = VectorLength(tmp);
		if (tdist < dist) {
			dist = tdist;
			j    = i;
		}
	}

	return j;
}
// -NERVE - SMF

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_TEAM_SPAWN_POINTS   256
gentity_t *SelectRandomTeamSpawnPoint(int teamstate, team_t team, int spawnObjective) {
	gentity_t *spot;
	gentity_t *spots[MAX_TEAM_SPAWN_POINTS];
	int       count, closest;
	int       i = 0;
	char      *classname;
	float     shortest, tmp;
	vec3_t    target;
	vec3_t    farthest;

	// Nico, silent GCC
	teamstate = teamstate;

	if (team == TEAM_AXIS) {
		classname = "team_CTF_redspawn";
	} else if (team == TEAM_ALLIES) {
		classname = "team_CTF_bluespawn";
	} else {
		return NULL;
	}

	count = 0;

	spot = NULL;

	while ((spot = G_Find(spot, FOFS(classname), classname)) != NULL) {
		if (SpotWouldTelefrag(spot)) {
			continue;
		}

		// Arnout - modified to allow initial spawnpoints to be disabled at gamestart
		if (!(spot->spawnflags & 2)) {
			continue;
		}

		// Arnout: invisible entities can't be used for spawning
		if (spot->entstate == STATE_INVISIBLE || spot->entstate == STATE_UNDERCONSTRUCTION) {
			continue;
		}

		spots[count] = spot;
		if (++count == MAX_TEAM_SPAWN_POINTS) {
			break;
		}
	}

	if (!count) {   // no spots that won't telefrag
		spot = NULL;
		while ((spot = G_Find(spot, FOFS(classname), classname)) != NULL) {
			// Arnout - modified to allow initial spawnpoints to be disabled at gamestart
			if (!(spot->spawnflags & 2)) {
				continue;
			}

			// Arnout: invisible entities can't be used for spawning
			if (spot->entstate == STATE_INVISIBLE || spot->entstate == STATE_UNDERCONSTRUCTION) {
				continue;
			}

			return spot;
		}

		return G_Find(NULL, FOFS(classname), classname);
	}

	if ((!level.numspawntargets)) {
		G_Error("No spawnpoints found\n");
		return NULL;
	} else {
		// Gordon: adding ability to set autospawn
		if (!spawnObjective) {
			switch (team) {
			case TEAM_AXIS:
				spawnObjective = level.axisAutoSpawn + 1;
				break;
			case TEAM_ALLIES:
				spawnObjective = level.alliesAutoSpawn + 1;
				break;
			default:
				break;
			}
		}

		i = spawnObjective - 1;

		VectorCopy(level.spawntargets[i], farthest);

		// now that we've got farthest vector, figure closest spawnpoint to it
		VectorSubtract(farthest, spots[0]->s.origin, target);
		shortest = VectorLength(target);
		closest  = 0;
		for (i = 0; i < count; i++) {
			VectorSubtract(farthest, spots[i]->s.origin, target);
			tmp = VectorLength(target);

			if (tmp < shortest) {
				shortest = tmp;
				closest  = i;
			}
		}
		return spots[closest];
	}
}



/*
===========
SelectCTFSpawnPoint

============
*/
gentity_t *SelectCTFSpawnPoint(team_t team, int teamstate, vec3_t origin, vec3_t angles, int spawnObjective) {
	gentity_t *spot;

	spot = SelectRandomTeamSpawnPoint(teamstate, team, spawnObjective);

	if (!spot) {
		return SelectSpawnPoint(vec3_origin, origin, angles);
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);

	return spot;
}

/*---------------------------------------------------------------------------*/

void GetBotAutonomies(int clientNum, int *weapAutonomy, int *moveAutonomy);
/*
==================
TeamplayLocationsMessage

Format:
    clientNum location health armor weapon powerups

==================
*/

void TeamplayInfoMessage(team_t team) {
	char      entry[1024];
	char      string[1400];
	int       stringlength;
	int       i, j;
	gentity_t *player;
	int       cnt;
	int       h;
	char      *bufferedData;
	char      *tinfo;

	// send the latest information on all clients
	string[0]    = 0;
	stringlength = 0;

	for (i = 0, cnt = 0; i < level.numConnectedClients; i++) {
		player = g_entities + level.sortedClients[i];
		if (player->inuse && player->client->sess.sessionTeam == team) {

			// DHM - Nerve :: If in LIMBO, don't show followee's health
			if (player->client->ps.pm_flags & PMF_LIMBO) {
				h = -1;
			} else {
				h = player->client->ps.stats[STAT_HEALTH];
				if (h < 0) {
					h = 0;
				}
			}

			if (player->r.svFlags & SVF_POW) {
				continue;
			}
			Com_sprintf(entry, sizeof (entry), " %i %i %i %i", level.sortedClients[i], player->client->pers.teamState.location[0], player->client->pers.teamState.location[1], h);

			j = strlen(entry);
			if (stringlength + j > (int)sizeof (string)) {
				break;
			}
			strcpy(string + stringlength, entry);
			stringlength += j;
			cnt++;
		}
	}

	bufferedData = team == TEAM_AXIS ? level.tinfoAxis : level.tinfoAllies;

	tinfo = va("tinfo %i%s", cnt, string);
	if (!Q_stricmp(bufferedData, tinfo)) {     // Gordon: no change so just return
		return;
	}

	Q_strncpyz(bufferedData, tinfo, 1400);

	for (i = 0; i < level.numConnectedClients; i++) {
		player = g_entities + level.sortedClients[i];
		if (player->inuse && player->client->sess.sessionTeam == team) {
			if (player->client->pers.connected == CON_CONNECTED) {
				trap_SendServerCommand(player - g_entities, tinfo);
			}
		}
	}
}

void CheckTeamStatus(void) {
	int       i;
	gentity_t *ent;

	if (level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME) {
		level.lastTeamLocationTime = level.time;
		for (i = 0; i < level.numConnectedClients; i++) {
			ent = g_entities + level.sortedClients[i];
			if (ent->inuse && (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES)) {
				ent->client->pers.teamState.location[0] = (int)ent->r.currentOrigin[0];
				ent->client->pers.teamState.location[1] = (int)ent->r.currentOrigin[1];
			}
		}

		TeamplayInfoMessage(TEAM_AXIS);
		TeamplayInfoMessage(TEAM_ALLIES);
	}
}

/*-----------------------------------------------------------------*/

void Use_Team_InitialSpawnpoint(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	other     = other;
	activator = activator;

	if (ent->spawnflags & 4) {
		ent->spawnflags &= ~4;
	} else {
		ent->spawnflags |= 4;
	}
}

void Use_Team_Spawnpoint(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	other     = other;
	activator = activator;

	if (ent->spawnflags & 2) {
		ent->spawnflags &= ~2;

		if (g_developer.integer) {
			G_Printf("setting %s %s inactive\n", ent->classname, ent->targetname);
		}
	} else {
		ent->spawnflags |= 2;

		if (g_developer.integer) {
			G_Printf("setting %s %s active\n", ent->classname, ent->targetname);
		}
	}
}

void DropToFloor(gentity_t *ent);

/*QUAKED team_CTF_redplayer (0.5 0 0) (-16 -16 -16) (16 16 32) INVULNERABLE - STARTDISABLED
Axis players spawn here at game start.
*/
void SP_team_CTF_redplayer(gentity_t *ent) {
	// Gordon: these are obsolete
	G_Printf("^1team_ctf_*player entities are now obsolete, please remove them!\n");
	G_FreeEntity(ent);
	return;
}

/*QUAKED team_CTF_blueplayer (0 0 0.5) (-16 -16 -16) (16 16 32) INVULNERABLE - STARTDISABLED
Allied players spawn here at game start.
*/
void SP_team_CTF_blueplayer(gentity_t *ent) {
	// Gordon: these are obsolete
	G_Printf("^1team_ctf_*player entities are now obsolete, please remove them!\n");
	G_FreeEntity(ent);
	return;
}

// JPW NERVE edited quaked def
/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32) ? INVULNERABLE STARTACTIVE
potential spawning position for axis team in wolfdm games.

TODO: SelectRandomTeamSpawnPoint() will choose team_CTF_redspawn point that:

1) has been activated (FL_SPAWNPOINT_ACTIVE)
2) isn't occupied and
3) is closest to team_WOLF_objective

This allows spawnpoints to advance across the battlefield as new ones are
placed and/or activated.

If target is set, point spawnpoint toward target activation
*/
void SP_team_CTF_redspawn(gentity_t *ent) {
// JPW NERVE
	vec3_t dir;

	ent->enemy = G_PickTarget(ent->target);
	if (ent->enemy) {
		VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
		vectoangles(dir, ent->s.angles);
	}

	ent->use = Use_Team_Spawnpoint;
// jpw

	VectorSet(ent->r.mins, -16, -16, -24);
	VectorSet(ent->r.maxs, 16, 16, 32);

	ent->think = DropToFloor;
}

// JPW NERVE edited quaked def
/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32) ? INVULNERABLE STARTACTIVE
potential spawning position for allied team in wolfdm games.

TODO: SelectRandomTeamSpawnPoint() will choose team_CTF_bluespawn point that:

1) has been activated (active)
2) isn't occupied and
3) is closest to selected team_WOLF_objective

This allows spawnpoints to advance across the battlefield as new ones are
placed and/or activated.

If target is set, point spawnpoint toward target activation
*/
void SP_team_CTF_bluespawn(gentity_t *ent) {
// JPW NERVE
	vec3_t dir;

	ent->enemy = G_PickTarget(ent->target);
	if (ent->enemy) {
		VectorSubtract(ent->enemy->s.origin, ent->s.origin, dir);
		vectoangles(dir, ent->s.angles);
	}

	ent->use = Use_Team_Spawnpoint;
// jpw

	VectorSet(ent->r.mins, -16, -16, -24);
	VectorSet(ent->r.maxs, 16, 16, 32);

	ent->think = DropToFloor;
}


// JPW NERVE
/*QUAKED team_WOLF_objective (1 1 0.3) (-16 -16 -24) (16 16 32) DEFAULT_AXIS DEFAULT_ALLIES
marker for objective

This marker will be used for computing effective radius for
dynamite damage, as well as generating a list of objectives
that players can elect to spawn near to in the limbo spawn
screen.

    "description"	short text key for objective name that will appear in objective selection in limbo UI.

DEFAULT_AXIS - This spawn region belongs to the Axis at the start of the map
DEFAULT_ALLIES - This spawn region belongs to the Alles at the start of the map
*/
static int numobjectives = 0; // TTimo

// swaps the team
void team_wolf_objective_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	char cs[MAX_STRING_CHARS];

	// Nico, silent GCC
	other     = other;
	activator = activator;

	// Gordon 256 is a disabled flag
	if ((self->count2 & ~256) == TEAM_AXIS) {
		self->count2 = (self->count2 & 256) + TEAM_ALLIES;
	} else if ((self->count2 & ~256) == TEAM_ALLIES) {
		self->count2 = (self->count2 & 256) + TEAM_AXIS;
	}

	// And update configstring
	trap_GetConfigstring(self->count, cs, sizeof (cs));
	Info_SetValueForKey(cs, "spawn_targ", self->message);
	Info_SetValueForKey(cs, "x", va("%i", (int)self->s.origin[0]));
	Info_SetValueForKey(cs, "y", va("%i", (int)self->s.origin[1]));
	Info_SetValueForKey(cs, "t", va("%i", self->count2));
	trap_SetConfigstring(self->count, cs);
}

void objective_Register(gentity_t *self) {

	char numspawntargets[128];
	int  cs_obj = CS_MULTI_SPAWNTARGETS;
	char cs[MAX_STRING_CHARS];

	if (numobjectives == MAX_MULTI_SPAWNTARGETS) {
		G_Error("SP_team_WOLF_objective: exceeded MAX_MULTI_SPAWNTARGETS (%d)\n", MAX_MULTI_SPAWNTARGETS);
	} else {   // Set config strings
		cs_obj += numobjectives;
		trap_GetConfigstring(cs_obj, cs, sizeof (cs));
		Info_SetValueForKey(cs, "spawn_targ", self->message);
		Info_SetValueForKey(cs, "x", va("%i", (int)self->s.origin[0]));
		Info_SetValueForKey(cs, "y", va("%i", (int)self->s.origin[1]));
		if (level.ccLayers) {
			Info_SetValueForKey(cs, "z", va("%i", (int)self->s.origin[2]));
		}
		Info_SetValueForKey(cs, "t", va("%i", self->count2));
		self->use   = team_wolf_objective_use;
		self->count = cs_obj;
		trap_SetConfigstring(cs_obj, cs);
		VectorCopy(self->s.origin, level.spawntargets[numobjectives]);
	}

	numobjectives++;

	// set current # spawntargets
	level.numspawntargets = numobjectives;
	trap_GetConfigstring(CS_MULTI_INFO, cs, sizeof (cs));
	sprintf(numspawntargets, "%d", numobjectives);
	Info_SetValueForKey(cs, "numspawntargets", numspawntargets);
	trap_SetConfigstring(CS_MULTI_INFO, cs);
}

void SP_team_WOLF_objective(gentity_t *ent) {
	char *desc;

	G_SpawnString("description", "WARNING: No objective description set", &desc);



	// Gordon: wtf is this g_alloced? just use a static buffer fgs...
	ent->message = G_Alloc(strlen(desc) + 1);
	Q_strncpyz(ent->message, desc, strlen(desc) + 1);

	ent->nextthink = level.time + FRAMETIME;
	ent->think     = objective_Register;
	ent->s.eType   = ET_WOLF_OBJECTIVE;

	if (ent->spawnflags & 1) {
		ent->count2 = TEAM_AXIS;
	} else if (ent->spawnflags & 2) {
		ent->count2 = TEAM_ALLIES;
	}
}

// DHM - Nerve :: Capture and Hold Checkpoint flag
#define SPAWNPOINT  1
#define CP_HOLD     2
#define AXIS_ONLY   4
#define ALLIED_ONLY 8

void checkpoint_touch(gentity_t *self, gentity_t *other, trace_t *trace);

void checkpoint_use_think(gentity_t *self) {

	self->count2 = -1;

	if (self->count == TEAM_AXIS) {
		self->health = 0;
	} else {
		self->health = 10;
	}
}

void checkpoint_use(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	int holderteam;
	int time;

	// Nico, silent GCC
	other = other;

	if (!activator->client) {
		return;
	}

	if (ent->count < 0) {
		checkpoint_touch(ent, activator, NULL);
	}

	holderteam = activator->client->sess.sessionTeam;

	if (ent->count == holderteam) {
		return;
	}

	if (ent->count2 == level.time) {
		if (holderteam == TEAM_AXIS) {
			time = ent->health / 2;
			time++;
			trap_SendServerCommand(activator - g_entities, va("cp \"Flag will be held in %i seconds!\n\"", time));
		} else {
			time = (10 - ent->health) / 2;
			time++;
			trap_SendServerCommand(activator - g_entities, va("cp \"Flag will be held in %i seconds!\n\"", time));
		}
		return;
	}

	if (holderteam == TEAM_AXIS) {
		ent->health--;
		if (ent->health < 0) {
			checkpoint_touch(ent, activator, NULL);
			return;
		}

		time = ent->health / 2;
		time++;
		trap_SendServerCommand(activator - g_entities, va("cp \"Flag will be held in %i seconds!\n\"", time));
	} else {
		ent->health++;
		if (ent->health > 10) {
			checkpoint_touch(ent, activator, NULL);
			return;
		}

		time = (10 - ent->health) / 2;
		time++;
		trap_SendServerCommand(activator - g_entities, va("cp \"Flag will be held in %i seconds!\n\"", time));
	}

	ent->count2    = level.time;
	ent->think     = checkpoint_use_think;
	ent->nextthink = level.time + 2000;
}

void checkpoint_spawntouch(gentity_t *self, gentity_t *other, trace_t *trace);   // JPW NERVE

// JPW NERVE
void checkpoint_hold_think(gentity_t *self) {
	switch (self->s.frame) {
	case WCP_ANIM_RAISE_AXIS:
	case WCP_ANIM_AXIS_RAISED:
	case WCP_ANIM_RAISE_AMERICAN:
	case WCP_ANIM_AMERICAN_RAISED:
	default:
		break;
	}
	self->nextthink = level.time + 5000;
}
// jpw

void checkpoint_think(gentity_t *self) {

	switch (self->s.frame) {

	case WCP_ANIM_NOFLAG:
		break;
	case WCP_ANIM_RAISE_AXIS:
		self->s.frame = WCP_ANIM_AXIS_RAISED;
		break;
	case WCP_ANIM_RAISE_AMERICAN:
		self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		break;
	case WCP_ANIM_AXIS_RAISED:
		break;
	case WCP_ANIM_AMERICAN_RAISED:
		break;
	case WCP_ANIM_AXIS_TO_AMERICAN:
		self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		break;
	case WCP_ANIM_AMERICAN_TO_AXIS:
		self->s.frame = WCP_ANIM_AXIS_RAISED;
		break;
	case WCP_ANIM_AXIS_FALLING:
		self->s.frame = WCP_ANIM_NOFLAG;
		break;
	case WCP_ANIM_AMERICAN_FALLING:
		self->s.frame = WCP_ANIM_NOFLAG;
		break;
	default:
		break;

	}

// JPW NERVE
	if (self->spawnflags & SPAWNPOINT) {
		self->touch = checkpoint_spawntouch;
	} else if (!(self->spawnflags & CP_HOLD)) {
		self->touch = checkpoint_touch;
	}
	self->nextthink = 0;
// jpw
}

void checkpoint_touch(gentity_t *self, gentity_t *other, trace_t *trace) {
	// Nico, silent GCC
	trace = trace;

	if (self->count == (int)other->client->sess.sessionTeam) {
		return;
	}

	// Set controlling team
	self->count = other->client->sess.sessionTeam;

	// Set animation
	if (self->count == TEAM_AXIS) {
		if (self->s.frame == WCP_ANIM_NOFLAG) {
			self->s.frame = WCP_ANIM_RAISE_AXIS;
		} else if (self->s.frame == WCP_ANIM_AMERICAN_RAISED) {
			self->s.frame = WCP_ANIM_AMERICAN_TO_AXIS;
		} else {
			self->s.frame = WCP_ANIM_AXIS_RAISED;
		}
	} else {
		if (self->s.frame == WCP_ANIM_NOFLAG) {
			self->s.frame = WCP_ANIM_RAISE_AMERICAN;
		} else if (self->s.frame == WCP_ANIM_AXIS_RAISED) {
			self->s.frame = WCP_ANIM_AXIS_TO_AMERICAN;
		} else {
			self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		}
	}

	self->parent = other;

	// Gordon: reset player disguise on touching flag
	// Run script trigger
	if (self->count == TEAM_AXIS) {
		self->health = 0;
		G_Script_ScriptEvent(self, "trigger", "axis_capture");
	} else {
		self->health = 10;
		G_Script_ScriptEvent(self, "trigger", "allied_capture");
	}

	// Play a sound
	G_AddEvent(self, EV_GENERAL_SOUND, self->soundPos1);

	// Don't allow touch again until animation is finished
	self->touch = NULL;

	self->think     = checkpoint_think;
	self->nextthink = level.time + 1000;
}

// JPW NERVE -- if spawn flag is set, use this touch fn instead to turn on/off targeted spawnpoints
void checkpoint_spawntouch(gentity_t *self, gentity_t *other, trace_t *trace) {
	gentity_t *ent      = NULL;
	qboolean  playsound = qtrue;
	qboolean  firsttime = qfalse;

	// Nico, silent GCC
	trace = trace;

	if (self->count == (int)other->client->sess.sessionTeam) {
		return;
	}

	if (self->count < 0) {
		firsttime = qtrue;
	}

	// Set controlling team
	self->count = other->client->sess.sessionTeam;

	// Set animation
	if (self->count == TEAM_AXIS) {
		if (self->s.frame == WCP_ANIM_NOFLAG && !(self->spawnflags & ALLIED_ONLY)) {
			self->s.frame = WCP_ANIM_RAISE_AXIS;
		} else if (self->s.frame == WCP_ANIM_NOFLAG) {
			self->s.frame = WCP_ANIM_NOFLAG;
			playsound     = qfalse;
		} else if (self->s.frame == WCP_ANIM_AMERICAN_RAISED && !(self->spawnflags & ALLIED_ONLY)) {
			self->s.frame = WCP_ANIM_AMERICAN_TO_AXIS;
		} else if (self->s.frame == WCP_ANIM_AMERICAN_RAISED) {
			self->s.frame = WCP_ANIM_AMERICAN_FALLING;
		} else {
			self->s.frame = WCP_ANIM_AXIS_RAISED;
		}
	} else {
		if (self->s.frame == WCP_ANIM_NOFLAG && !(self->spawnflags & AXIS_ONLY)) {
			self->s.frame = WCP_ANIM_RAISE_AMERICAN;
		} else if (self->s.frame == WCP_ANIM_NOFLAG) {
			self->s.frame = WCP_ANIM_NOFLAG;
			playsound     = qfalse;
		} else if (self->s.frame == WCP_ANIM_AXIS_RAISED && !(self->spawnflags & AXIS_ONLY)) {
			self->s.frame = WCP_ANIM_AXIS_TO_AMERICAN;
		} else if (self->s.frame == WCP_ANIM_AXIS_RAISED) {
			self->s.frame = WCP_ANIM_AXIS_FALLING;
		} else {
			self->s.frame = WCP_ANIM_AMERICAN_RAISED;
		}
	}

	// If this is the first time it's being touched, and it was the opposing team
	// touching a single-team reinforcement flag... don't do anything.
	if (firsttime && !playsound) {
		return;
	}

	// Play a sound
	if (playsound) {
		G_AddEvent(self, EV_GENERAL_SOUND, self->soundPos1);
	}

	self->parent = other;

	// Gordon: reset player disguise on touching flag
	// Run script trigger
	if (self->count == TEAM_AXIS) {
		G_Script_ScriptEvent(self, "trigger", "axis_capture");
	} else {
		G_Script_ScriptEvent(self, "trigger", "allied_capture");
	}

	// Don't allow touch again until animation is finished
	self->touch = NULL;

	self->think     = checkpoint_think;
	self->nextthink = level.time + 1000;

	// activate all targets
	// Arnout - updated this to allow toggling of initial spawnpoints as well, plus now it only
	// toggles spawnflags 2 for spawnpoint entities
	if (self->target) {
		while (1) {
			ent = G_FindByTargetname(ent, self->target);
			if (!ent) {
				break;
			}
			if (other->client->sess.sessionTeam == TEAM_AXIS) {
				if (!strcmp(ent->classname, "team_CTF_redspawn")) {
					ent->spawnflags |= 2;
				} else if (!strcmp(ent->classname, "team_CTF_bluespawn")) {
					ent->spawnflags &= ~2;
				}
			} else {
				if (!strcmp(ent->classname, "team_CTF_bluespawn")) {
					ent->spawnflags |= 2;
				} else if (!strcmp(ent->classname, "team_CTF_redspawn")) {
					ent->spawnflags &= ~2;
				}
			}
		}
	}

}
// jpw
/*QUAKED team_WOLF_checkpoint (.9 .3 .9) (-16 -16 0) (16 16 128) SPAWNPOINT CP_HOLD AXIS_ONLY ALLIED_ONLY
This is the flagpole players touch in Capture and Hold game scenarios.

It will call specific trigger funtions in the map script for this object.
When allies capture, it will call "allied_capture".
When axis capture, it will call "axis_capture".

// JPW NERVE if spawnpoint flag is set, think will turn on spawnpoints (specified as targets)
// for capture team and turn *off* targeted spawnpoints for opposing team
*/
void SP_team_WOLF_checkpoint(gentity_t *ent) {
	char *capture_sound;

	if (!ent->scriptName) {
		G_Error("team_WOLF_checkpoint must have a \"scriptname\"\n");
	}

	// Make sure the ET_TRAP entity type stays valid
	ent->s.eType = ET_TRAP;

	// Model is user assignable, but it will always try and use the animations for flagpole.md3
	if (ent->model) {
		ent->s.modelindex = G_ModelIndex(ent->model);
	} else {
		ent->s.modelindex = G_ModelIndex("models/multiplayer/flagpole/flagpole.md3");
	}

	G_SpawnString("noise", "sound/movers/doors/door6_open.wav", &capture_sound);
	ent->soundPos1 = G_SoundIndex(capture_sound);

	ent->clipmask   = CONTENTS_SOLID;
	ent->r.contents = CONTENTS_SOLID;

	VectorSet(ent->r.mins, -8, -8, 0);
	VectorSet(ent->r.maxs, 8, 8, 128);

	G_SetOrigin(ent, ent->s.origin);
	G_SetAngle(ent, ent->s.angles);

	// s.frame is the animation number
	ent->s.frame = WCP_ANIM_NOFLAG;

	// s.teamNum is which set of animations to use ( only 1 right now )
	ent->s.teamNum = 1;

	// Used later to set animations (and delay between captures)
	ent->nextthink = 0;

	// Used to time how long it must be "held" to switch
	ent->health = -1;
	ent->count2 = -1;

	// 'count' signifies which team holds the checkpoint
	ent->count = -1;

// JPW NERVE
	if (ent->spawnflags & SPAWNPOINT) {
		ent->touch = checkpoint_spawntouch;
	} else {
		if (ent->spawnflags & CP_HOLD) {
			ent->use = checkpoint_use;
		} else {
			ent->touch = checkpoint_touch;
		}
	}
// jpw

	trap_LinkEntity(ent);
}

/*
===================
Team_ClassForString
===================
*/
int Team_ClassForString(char *string) {
	if (!Q_stricmp(string, "soldier")) {
		return PC_SOLDIER;
	} else if (!Q_stricmp(string, "medic")) {
		return PC_MEDIC;
	} else if (!Q_stricmp(string, "engineer")) {
		return PC_ENGINEER;
	} else if (!Q_stricmp(string, "lieutenant")) {       // FIXME: remove from missionpack
		return PC_FIELDOPS;
	} else if (!Q_stricmp(string, "fieldops")) {
		return PC_FIELDOPS;
	} else if (!Q_stricmp(string, "covertops")) {
		return PC_COVERTOPS;
	}
	return -1;
}

// OSP
char      *aTeams[TEAM_NUM_TEAMS] = { "FFA", "^1Axis^7", "^4Allies^7", "Spectators" };
team_info teamInfo[TEAM_NUM_TEAMS];


// Resets a team's settings
void G_teamReset(int team_num, qboolean fClearSpecLock) {
	// Nico, silent GCC
	fClearSpecLock = fClearSpecLock;

	teamInfo[team_num].team_lock    = qfalse;
	teamInfo[team_num].team_name[0] = 0;
	teamInfo[team_num].team_score   = 0;
}

// Returns player's "real" team.
int G_teamID(gentity_t *ent) {
	if (ent->client->sess.coach_team) {
		return(ent->client->sess.coach_team);
	}
	return(ent->client->sess.sessionTeam);
}

// Checks to see if a specified team is allowing players to join.
qboolean G_teamJoinCheck(int team_num, gentity_t *ent) {
	int cnt = TeamCount(-1, team_num);

	// Sanity check
	if (cnt == 0) {
		G_teamReset(team_num, qtrue);
		teamInfo[team_num].team_lock = qfalse;
	}

	// Check for locked teams
	if ((team_num == TEAM_AXIS || team_num == TEAM_ALLIES)) {
		if ((int)ent->client->sess.sessionTeam == team_num) {
			return(qtrue);
		}
		// Check for full teams
		if (team_maxplayers.integer > 0 && team_maxplayers.integer <= cnt) {
			G_printFull(va("The %s team is full!", aTeams[team_num]), ent);
			return(qfalse);

			// Check for locked teams
		} else if (teamInfo[team_num].team_lock && (!(ent->client->pers.invite & team_num))) {
			G_printFull(va("The %s team is LOCKED!", aTeams[team_num]), ent);
			return(qfalse);
		}
	}

	return(qtrue);
}

// Nico, check if ew can follow a given client
qboolean G_AllowFollow(gentity_t *ent, gentity_t *other) {

	// Nico, only referee can spec in cup mode
	if (g_cupMode.integer != 0 && ent->client->sess.referee != RL_REFEREE) {
		return qfalse;
	}

	return !other->client->sess.specLocked ||
	       COM_BitCheck(other->client->sess.specInvitedClients, ent - g_entities) ||
	       ent->client->sess.referee == RL_REFEREE;
}


// Figure out if we are allowed/want to follow a given player
qboolean G_DesiredFollow(gentity_t *ent, gentity_t *other) {
	return G_AllowFollow(ent, other)
	       && (ent->client->sess.spec_team == 0
	           || ent->client->sess.spec_team == (int)other->client->sess.sessionTeam);
}
// -OSP
