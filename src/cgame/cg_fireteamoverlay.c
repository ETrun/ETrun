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

/******************************************************************************
***** teh firetams! (sic)
****/

#include "cg_local.h"

/******************************************************************************
***** Defines, constants, etc
****/

static int sortedFireTeamClients[MAX_CLIENTS];

/******************************************************************************
***** Support Routines
****/

int QDECL CG_SortFireTeam(const void *a, const void *b) {
	int cna, cnb;

	cna = *(int *)a;
	cnb = *(int *)b;

	// Not on our team, so shove back
	if (!CG_IsOnSameFireteam(cnb, cg.clientNum)) {
		return -1;
	}
	if (!CG_IsOnSameFireteam(cna, cg.clientNum)) {
		return 1;
	}

	// Leader comes first
	if (CG_IsFireTeamLeader(cna)) {
		return -1;
	}
	if (CG_IsFireTeamLeader(cnb)) {
		return 1;
	}
	return 0;
}

// Sorts client's fireteam by leader then rank
void CG_SortClientFireteam() {
	int i;

	for (i = 0; i < MAX_CLIENTS; ++i) {
		sortedFireTeamClients[i] = i;
	}

	qsort(sortedFireTeamClients, MAX_CLIENTS, sizeof (sortedFireTeamClients[0]), CG_SortFireTeam);
}

// Parses fireteam servercommand
void CG_ParseFireteams() {
	int  i;
	char *s;
	int  clnts[2];

	for (i = 0; i < MAX_CLIENTS; ++i) {
		cgs.clientinfo[i].fireteamData = NULL;
	}

	for (i = 0; i < MAX_FIRETEAMS; ++i) {
		int        j;
		const char *p;
		char       hexbuffer[11] = "0x00000000";

		p = CG_ConfigString(CS_FIRETEAMS + i);

		j = atoi(Info_ValueForKey(p, "id"));
		if (j == -1) {
			cg.fireTeams[i].inuse = qfalse;
			continue;
		} else {
			cg.fireTeams[i].inuse = qtrue;
			cg.fireTeams[i].ident = j;
		}

		s                      = Info_ValueForKey(p, "l");
		cg.fireTeams[i].leader = atoi(s);

		s = Info_ValueForKey(p, "c");
		Q_strncpyz(hexbuffer + 2, s, 9);
		sscanf(hexbuffer, "%10x", (unsigned int *)&clnts[1]);
		Q_strncpyz(hexbuffer + 2, s + 8, 9);
		sscanf(hexbuffer, "%10x", (unsigned int *)&clnts[0]);

		// Nico, private (1) or public (0)
		s                    = Info_ValueForKey(p, "p");
		cg.fireTeams[i].priv = atoi(s) == 0 ? qfalse : qtrue;

		for (j = 0; j < MAX_CLIENTS; ++j) {
			if (COM_BitCheck(clnts, j)) {
				cg.fireTeams[i].joinOrder[j]   = qtrue;
				cgs.clientinfo[j].fireteamData = &cg.fireTeams[i];
			} else {
				cg.fireTeams[i].joinOrder[j] = qfalse;
			}
		}
	}

	CG_SortClientFireteam();
}

// Fireteam that both specified clients are on, if they both are on the same team
fireteamData_t *CG_IsOnSameFireteam(int clientNum, int clientNum2) {
	if (CG_IsOnFireteam(clientNum) == CG_IsOnFireteam(clientNum2)) {
		return CG_IsOnFireteam(clientNum);
	}

	return NULL;
}

// Fireteam that specified client is leader of, or NULL if none
fireteamData_t *CG_IsFireTeamLeader(int clientNum) {
	fireteamData_t *f;

	f = CG_IsOnFireteam(clientNum);
	if (!f) {
		return NULL;
	}

	if (f->leader != clientNum) {
		return NULL;
	}

	return f ;
}

// Client, sorted by rank, on CLIENT'S fireteam
clientInfo_t *CG_SortedFireTeamPlayerForPosition(int pos, int max) {
	int            i, cnt = 0;
	fireteamData_t *f = CG_IsOnFireteam(cg.clientNum);

	if (!f) {
		return NULL;
	}

	for (i = 0; i < MAX_CLIENTS && cnt < max; ++i) {
		if (!(f == CG_IsOnFireteam(sortedFireTeamClients[i]))) {
			return NULL;
		}

		if (cnt == pos) {
			return &cgs.clientinfo[sortedFireTeamClients[i]];
		}
		cnt++;
	}

	return NULL;
}

/******************************************************************************
***** Main Functions
****/

#define FT_BAR_YSPACING 2.f
#define FT_BAR_HEIGHT 10.f
#define FT_WIDTH 160 // suburb, incremented to match ingame main menu
void CG_DrawFireTeamOverlay(void) {
	float          x = CG_WideX(cg_fireteamOverlayX.value);
	float          y = cg_fireteamOverlayY.value;
	float          h;
	clientInfo_t   *ci = NULL;
	char           buffer[64];
	fireteamData_t *f;
	int            i;
	vec4_t         clr1        = { .16f, .2f, .17f, .8f };
	vec4_t         tclr        = { 0.6f, 0.6f, 0.6f, 1.0f };
	vec4_t         bgColor     = { 0.0f, 0.0f, 0.0f, 0.5f }; // window
	vec4_t         borderColor = { 0.5f, 0.5f, 0.5f, 0.5f }; // window

	// suburb, don't draw if any menu is up because of readability
	if (!cg_drawFireteamOverlay.integer || cg.UIisUp) {
		return;
	}

	f = CG_IsOnFireteam(cg.clientNum);
	if (!f) {
		return;
	}

	h = 12 + 2 + 2;
	for (i = 0; i < 6; ++i) {
		ci = CG_SortedFireTeamPlayerForPosition(i, 6);
		if (!ci) {
			break;
		}

		h += FT_BAR_HEIGHT + FT_BAR_YSPACING;
	}

	CG_DrawRect(x, y, FT_WIDTH, h, 1, borderColor);
	CG_FillRect(x + 1, y + 1, FT_WIDTH - 2, h - 2, bgColor);

	x += 2;
	y += 2;

	CG_FillRect(x, y, FT_WIDTH - 4, 12, clr1);

	// Nico, show if it's a private or public FT
	if (f->priv) {
		sprintf(buffer, "FT %s (Priv)", bg_fireteamNames[f->ident]);
	} else {
		sprintf(buffer, "FT %s", bg_fireteamNames[f->ident]);
	}
	Q_strupr(buffer);
	CG_Text_Paint_Ext(x + 3, y + FT_BAR_HEIGHT, .19f, .19f, tclr, buffer, 0, 0, 0, &cgs.media.limboFont1);

	x += 2;

	for (i = 0; i < 6; ++i) {
		y += FT_BAR_HEIGHT + FT_BAR_YSPACING;
		x  = cg_fireteamOverlayX.value + 2;

		ci = CG_SortedFireTeamPlayerForPosition(i, 6);
		if (!ci) {
			break;
		}

		x += 4;

		CG_Text_Paint_Ext(x, y + FT_BAR_HEIGHT, .2f, .2f, tclr, ci->name, 0, 17, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	}
}

qboolean CG_FireteamHasClass(int classnum, qboolean selectedonly) {
	fireteamData_t *ft;
	int            i;

	ft = CG_IsOnFireteam(cg.clientNum);
	if (!ft) {
		return qfalse;
	}

	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (!cgs.clientinfo[i].infoValid) {
			continue;
		}

		if (ft != CG_IsOnFireteam(i)) {
			continue;
		}

		if (cgs.clientinfo[i].cls != classnum) {
			continue;
		}

		if (selectedonly && !cgs.clientinfo[i].selected) {
			continue;
		}

		return qtrue;
	}

	return qfalse;
}

const char *CG_BuildSelectedFirteamString(void) {
	char buffer[256];
	int  cnt = 0;
	int  i;

	*buffer = '\0';
	for (i = 0; i < 6; ++i) {
		clientInfo_t *ci;

		ci = CG_SortedFireTeamPlayerForPosition(i, 6);
		if (!ci) {
			break;
		}

		if (!ci->selected) {
			continue;
		}

		cnt++;

		Q_strcat(buffer, sizeof (buffer), va("%i ", ci->clientNum));
	}

	if (cnt == 0) {
		return "0";
	}

	if (!cgs.clientinfo[cg.clientNum].selected) {
		Q_strcat(buffer, sizeof (buffer), va("%i ", cg.clientNum));
		cnt++;
	}

	return va("%i %s", cnt, buffer);
}
