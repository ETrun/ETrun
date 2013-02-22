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

#include "cg_local.h"

const char *CG_DescriptionForCampaign(void) {
	return cgs.campaignInfoLoaded ? cgs.campaignData.campaignDescription : NULL;
}

const char *CG_NameForCampaign(void) {
	return cgs.campaignInfoLoaded ? cgs.campaignData.campaignName : NULL;
}

qboolean CG_FindArenaInfo(char *filename, char *mapname, arenaInfo_t *info) {
	int        handle;
	pc_token_t token;
	const char *dummy;
	qboolean   found = qfalse;

	handle = trap_PC_LoadSource(filename);

	if (!handle) {
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return qfalse;
	}

	if (!trap_PC_ReadToken(handle, &token)) {
		trap_PC_FreeSource(handle);
		return qfalse;
	}

	if (*token.string != '{') {
		trap_PC_FreeSource(handle);
		return qfalse;
	}

	while (trap_PC_ReadToken(handle, &token)) {
		if (*token.string == '}') {
			if (found) {
				trap_PC_FreeSource(handle);
				return qtrue;
			}
			found = qfalse;

			if (!trap_PC_ReadToken(handle, &token)) {
				// eof
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			if (*token.string != '{') {
				trap_Print(va(S_COLOR_RED "unexpected token '%s' inside: %s\n", token.string, filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
		} else if (!Q_stricmp(token.string, "objectives") || !Q_stricmp(token.string, "description") || !Q_stricmp(token.string, "type")) {
			if (!PC_String_Parse(handle, &dummy)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}
		} else if (!Q_stricmp(token.string, "longname")) {
			if (!PC_String_Parse(handle, &dummy)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			} else {
				Q_strncpyz(info->longname, dummy, 128);
			}
		} else if (!Q_stricmp(token.string, "map")) {
			if (!PC_String_Parse(handle, &dummy)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			} else {
				if (!Q_stricmp(dummy, mapname)) {
					found = qtrue;
				}
			}
		}

		if (!Q_stricmp(token.string, "briefing")) {
			if (!PC_String_Parse(handle, &dummy)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			} else {
				Q_strncpyz(info->description, dummy, sizeof (info->description));
			}
		} else if (!Q_stricmp(token.string, "alliedwintext")) {
			if (!PC_String_Parse(handle, &dummy)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			} else {
				Q_strncpyz(info->alliedwintext, dummy, sizeof (info->description));
			}
		} else if (!Q_stricmp(token.string, "axiswintext")) {
			if (!PC_String_Parse(handle, &dummy)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			} else {
				Q_strncpyz(info->axiswintext, dummy, sizeof (info->description));
			}
		} else if (!Q_stricmp(token.string, "mapposition_x")) {
			vec_t x;

			if (!trap_PC_ReadToken(handle, &token)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			x = token.floatvalue;

			info->mappos[0] = x;
		} else if (!Q_stricmp(token.string, "mapposition_y")) {
			vec_t y;

			if (!trap_PC_ReadToken(handle, &token)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return qfalse;
			}

			y = token.floatvalue;

			info->mappos[1] = y;
		}
	}

	trap_PC_FreeSource(handle);
	return qfalse;
}

void CG_LocateArena(void) {
	char filename[MAX_QPATH];

	Com_sprintf(filename, sizeof (filename), "scripts/%s.arena", cgs.rawmapname);

	if ( !CG_FindArenaInfo(filename, cgs.rawmapname, &cgs.arenaData)) {
		return;
	}

	cgs.arenaInfoLoaded = qtrue;
}
