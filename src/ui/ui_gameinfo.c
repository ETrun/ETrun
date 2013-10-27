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

//
// gameinfo.c
//

#include "ui_local.h"

/*
===============
UI_LoadArenasFromFile
===============
*/
static void UI_LoadArenasFromFile(char *filename) {
	int        handle;
	pc_token_t token;

	handle = trap_PC_LoadSource(filename);

	if (!handle) {
		trap_Print(va(S_COLOR_RED "file not found: %s\n", filename));
		return;
	}

	if (!trap_PC_ReadToken(handle, &token)) {
		trap_PC_FreeSource(handle);
		return;
	}

	if (*token.string != '{') {
		trap_PC_FreeSource(handle);
		return;
	}

	uiInfo.mapList[uiInfo.mapCount].levelShot = -1;
	uiInfo.mapList[uiInfo.mapCount].typeBits  = 0;

	while (trap_PC_ReadToken(handle, &token)) {
		if (*token.string == '}') {

			uiInfo.mapCount++;
			if (uiInfo.mapCount >= MAX_MAPS) {
				break;
			}

			if (!trap_PC_ReadToken(handle, &token)) {
				// eof
				trap_PC_FreeSource(handle);
				return;
			}

			if (*token.string != '{') {
				trap_Print(va(S_COLOR_RED "unexpected token '%s' inside: %s\n", token.string, filename));
				trap_PC_FreeSource(handle);
				return;
			}
		} else if (!Q_stricmp(token.string, "map")) {
			if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mapLoadName)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		} else if (!Q_stricmp(token.string, "longname")) {
			if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mapName)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		} else if (!Q_stricmp(token.string, "briefing")) {
			if (!PC_String_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].briefing)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		} else if (!Q_stricmp(token.string, "type")) {
			if (!trap_PC_ReadToken(handle, &token)) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		} else if (!Q_stricmp(token.string, "mapposition_x")) {
			if (!PC_Float_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mappos[0])) {
				trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
				trap_PC_FreeSource(handle);
				return;
			}
		} else if (!Q_stricmp(token.string, "mapposition_y") &&
		           !PC_Float_Parse(handle, &uiInfo.mapList[uiInfo.mapCount].mappos[1])) {
			trap_Print(va(S_COLOR_RED "unexpected end of file inside: %s\n", filename));
			trap_PC_FreeSource(handle);
			return;
		}
	}

	trap_PC_FreeSource(handle);
}

/*
=============
UI_SortArenas

Nico, sorting the map list
// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=090
=============
*/
int QDECL UI_SortArenas(const void *a, const void *b) {
	mapInfo ca = *(mapInfo *)a;
	mapInfo cb = *(mapInfo *)b;
	char    cleanNameA[MAX_STRING_CHARS];
	char    cleanNameB[MAX_STRING_CHARS];

	Q_strncpyz(cleanNameA, ca.mapName, sizeof (cleanNameA));
	Q_strncpyz(cleanNameB, cb.mapName, sizeof (cleanNameB));
	Q_CleanStr(cleanNameA);
	Q_CleanStr(cleanNameB);

	return strcmp(cleanNameA, cleanNameB);
}

/*
===============
UI_LoadArenas
===============
*/
void UI_LoadArenas(void) {
	int  numdirs;
	char filename[128];
	char dirlist[1024];
	char *dirptr;
	int  i;
	int  dirlen;

	uiInfo.mapCount = 0;

	// get all arenas from .arena files
	numdirs = trap_FS_GetFileList("scripts", ".arena", dirlist, 1024);
	dirptr  = dirlist;
	for (i = 0; i < numdirs; i++, dirptr += dirlen + 1) {
		dirlen = strlen(dirptr);
		strcpy(filename, "scripts/");
		strcat(filename, dirptr);
		UI_LoadArenasFromFile(filename);
	}

	// Nico, sorting map list
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=090
	qsort(uiInfo.mapList, uiInfo.mapCount, sizeof (uiInfo.mapList[0]), UI_SortArenas);
}

mapInfo *UI_FindMapInfoByMapname(const char *name) {
	int i;

	if (uiInfo.mapCount == 0) {
		UI_LoadArenas();
	}

	for (i = 0; i < uiInfo.mapCount; i++) {
		if (!Q_stricmp(uiInfo.mapList[i].mapLoadName, name)) {
			return &uiInfo.mapList[i];
		}
	}

	return NULL;
}
