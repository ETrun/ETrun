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
#include "../../etrun/ui/menudef.h"


/*
=======================================================================

  SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData(gclient_t *client, qboolean restart) {
	int        mvc = 0;
	const char *s;

	s = va("%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
	       client->sess.sessionTeam,
	       client->sess.spectatorState,
	       client->sess.spectatorClient,
	       client->sess.playerType,         // DHM - Nerve
	       client->sess.playerWeapon,       // DHM - Nerve
	       client->sess.playerWeapon2,
	       client->sess.latchPlayerType,    // DHM - Nerve
	       client->sess.latchPlayerWeapon,  // DHM - Nerve
	       client->sess.latchPlayerWeapon2,
	       client->sess.referee,
	       client->sess.spec_team,
	       (mvc & 0xFFFF),
	       ((mvc >> 16) & 0xFFFF),
	       client->sess.muted,
	       client->sess.ignoreClients[0],
	       client->sess.ignoreClients[1],
	       client->pers.enterTime,
	       restart ? client->sess.spawnObjectiveIndex : 0,
	       client->sess.specLocked,
	       client->sess.specInvitedClients[0],
	       client->sess.specInvitedClients[1]
	       );

	trap_Cvar_Set(va("session%d", (int)(client - level.clients)), s);
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData(gclient_t *client) {
	int  mvc_l, mvc_h;
	char s[MAX_STRING_CHARS];

	trap_Cvar_VariableStringBuffer(va("session%d", (int)(client - level.clients)), s, sizeof (s));

	sscanf(s, "%i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
	       (int *)&client->sess.sessionTeam,
	       (int *)&client->sess.spectatorState,
	       &client->sess.spectatorClient,
	       &client->sess.playerType, // DHM - Nerve
	       &client->sess.playerWeapon, // DHM - Nerve
	       &client->sess.playerWeapon2,
	       &client->sess.latchPlayerType, // DHM - Nerve
	       &client->sess.latchPlayerWeapon, // DHM - Nerve
	       &client->sess.latchPlayerWeapon2,
	       &client->sess.referee,
	       &client->sess.spec_team,
	       &mvc_l,
	       &mvc_h,
	       (int *)&client->sess.muted,
	       &client->sess.ignoreClients[0],
	       &client->sess.ignoreClients[1],
	       &client->pers.enterTime,
	       &client->sess.spawnObjectiveIndex,
	       (int *) &client->sess.specLocked,
	       &client->sess.specInvitedClients[0],
	       &client->sess.specInvitedClients[1]
	       );
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData(gclient_t *client) {
	clientSession_t *sess;

	sess = &client->sess;

	// initial team determination
	sess->sessionTeam = TEAM_SPECTATOR;

	sess->spectatorState = SPECTATOR_FREE;

	// DHM - Nerve
	sess->latchPlayerType    = sess->playerType = 0;
	sess->latchPlayerWeapon  = sess->playerWeapon = 0;
	sess->latchPlayerWeapon2 = sess->playerWeapon2 = 0;

	sess->spawnObjectiveIndex = 0;
	// dhm - end

	memset(sess->ignoreClients, 0, sizeof (sess->ignoreClients));
	sess->muted = qfalse;

	// OSP
	sess->referee    = (client->pers.localClient) ? RL_REFEREE : RL_NONE;
	sess->spec_team  = 0;
	// OSP

	G_WriteClientSessionData(client, qfalse);
}


/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession(void) {
	char s[MAX_STRING_CHARS];
	int  i, j;

	for (i = 0; i < MAX_FIRETEAMS; i++) {
		char *p, *c;

		trap_Cvar_VariableStringBuffer(va("fireteam%i", i), s, sizeof (s));

		p = Info_ValueForKey(s, "id");
		j = atoi(p);
		if (!*p || j == -1) {
			level.fireTeams[i].inuse = qfalse;
		} else {
			level.fireTeams[i].inuse = qtrue;
		}
		level.fireTeams[i].ident = j + 1;

		p                       = Info_ValueForKey(s, "p");
		level.fireTeams[i].priv = !atoi(p) ? qfalse : qtrue;

		p = Info_ValueForKey(s, "i");

		j = 0;
		if (p && *p) {
			c = p;
			for (c = strchr(c, ' ') + 1; c && *c; ) {
				char str[8];
				char *l = strchr(c, ' ');
				if (!l) {
					break;
				}
				Q_strncpyz(str, c, l - c + 1);
				str[l - c]                        = '\0';
				level.fireTeams[i].joinOrder[j++] = atoi(str);
				c                                 = l + 1;
			}
		}

		for ( ; j < MAX_CLIENTS; j++) {
			level.fireTeams[i].joinOrder[j] = -1;
		}
		G_UpdateFireteamConfigString(&level.fireTeams[i]);
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData(qboolean restart) {
	int  i;
	char strServerInfo[MAX_INFO_STRING];
	int  j;

	trap_GetServerinfo(strServerInfo, sizeof (strServerInfo));

	trap_Cvar_Set("session", va("%s",
	                            Info_ValueForKey(strServerInfo, "mapname")));

	for (i = 0; i < level.numConnectedClients; i++) {
		if (level.clients[level.sortedClients[i]].pers.connected == CON_CONNECTED) {
			G_WriteClientSessionData(&level.clients[level.sortedClients[i]], restart);
			// For slow connecters and a short warmup
		}
	}

	for (i = 0; i < MAX_FIRETEAMS; i++) {
		char buffer[MAX_STRING_CHARS];
		if (!level.fireTeams[i].inuse) {
			Com_sprintf(buffer, MAX_STRING_CHARS, "\\id\\-1");
		} else {
			char buffer2[MAX_STRING_CHARS];

			*buffer2 = '\0';
			for (j = 0; j < MAX_CLIENTS; j++) {
				char p[8];
				Com_sprintf(p, 8, " %i", level.fireTeams[i].joinOrder[j]);
				Q_strcat(buffer2, MAX_STRING_CHARS, p);
			}
			Com_sprintf(buffer, MAX_STRING_CHARS, "\\id\\%i\\i\\%s\\p\\%i", level.fireTeams[i].ident - 1, buffer2, level.fireTeams[i].priv ? 1 : 0);
		}

		trap_Cvar_Set(va("fireteam%i", i), buffer);
	}
}
