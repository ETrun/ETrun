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

const char *systemMessages[SM_NUM_SYS_MSGS] =
{
	"SYS_NeedMedic",
	"SYS_NeedEngineer",
	"SYS_NeedLT",
	"SYS_NeedCovertOps",
	"SYS_MenDown",
	"SYS_ObjCaptured",
	"SYS_ObjLost",
	"SYS_ObjDestroyed",
	"SYS_ConstructComplete",
	"SYS_ConstructFailed",
	"SYS_Destroyed",
};

int G_GetSysMessageNumber(const char *sysMsg) {
	int i;

	for (i = 0; i < SM_NUM_SYS_MSGS; i++) {
		if (!Q_stricmp(systemMessages[i], sysMsg)) {
			return i;
		}
	}

	return -1;
}

void G_SendSystemMessage(sysMsg_t message, int team) {
	gentity_t *other;
	int       *time;
	int       j;

	time = team == TEAM_AXIS ? &level.lastSystemMsgTime[0] : &level.lastSystemMsgTime[1];

	if (*time && (level.time - *time) < 15000) {
		return;
	}

	*time = level.time;

	for (j = 0; j < level.maxclients; j++) {
		other = &g_entities[j];

		if (!other->client || !other->inuse) {
			continue;
		}

		if ((int)other->client->sess.sessionTeam != team) {
			continue;
		}

		trap_SendServerCommand(other - g_entities, va("vschat 0 %d 3 %s 0 0 0", (int)(other - g_entities), systemMessages[message]));
	}
}
