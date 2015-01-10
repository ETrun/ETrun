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

// g_match.c: Match handling
// -------------------------
//
#include "g_local.h"
#include "../../etrun/ui/menudef.h"

void G_initMatch(void) {
	int i;

	for (i = TEAM_AXIS; i <= TEAM_ALLIES; ++i) {
		G_teamReset(i);
	}
}

// Setting initialization
void G_loadMatchGame(void) {
	G_Printf("Setting MOTD...\n");
	trap_SetConfigstring(CS_CUSTMOTD + 0, server_motd0.string);
	trap_SetConfigstring(CS_CUSTMOTD + 1, server_motd1.string);
	trap_SetConfigstring(CS_CUSTMOTD + 2, server_motd2.string);
	trap_SetConfigstring(CS_CUSTMOTD + 3, server_motd3.string);
	trap_SetConfigstring(CS_CUSTMOTD + 4, server_motd4.string);
	trap_SetConfigstring(CS_CUSTMOTD + 5, server_motd5.string);

	// Voting flags
	G_voteFlags();
}

// Simple alias for sure-fire print :)
void G_printFull(char *str, gentity_t *ent) {
	if (ent != NULL) {
		CP(va("print \"%s\n\"", str));
		CP(va("cp \"%s\n\"", str));    // #fixme, prints "cp" :o
	} else {
		AP(va("print \"%s\n\"", str));
		AP(va("cp \"%s\n\"", str));
	}
}

// Plays specified sound globally.
void G_globalSound(char *sound) {
	gentity_t *te = G_TempEntity(level.intermission_origin, EV_GLOBAL_SOUND);

	te->s.eventParm = G_SoundIndex(sound);
	te->r.svFlags  |= SVF_BROADCAST;
}

// Update configstring for vote info
int G_checkServerToggle(vmCvar_t *cv) {
	int nFlag;

	if (cv == &g_antilag) {
		nFlag = CV_SVS_ANTILAG;
	} else {
		return qfalse;
	}

	if (cv->integer > 0) {
		level.server_settings |= nFlag;
	} else {
		level.server_settings &= ~nFlag;
	}

	return qtrue;
}
