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

#define NUM_PM_STACK_ITEMS  32
#define NUM_PM_STACK_ITEMS_BIG 8 // Gordon: we shouldn't need many of these
#define PM_ICON_SIZE_NORMAL 20
#define PM_ICON_SIZE_SMALL 12

typedef struct pmStackItem_s pmListItem_t;
typedef struct pmStackItemBig_s pmListItemBig_t;

struct pmStackItem_s {
	popupMessageType_t type;
	qboolean inuse;
	int time;
	char message[128];
	qhandle_t shader;
	pmListItem_t *next;
};

struct pmStackItemBig_s {
	popupMessageBigType_t type;
	qboolean inuse;
	int time;
	char message[128];
	qhandle_t shader;
	pmListItemBig_t *next;
};

pmListItem_t    cg_pmStack[NUM_PM_STACK_ITEMS];
pmListItem_t    *cg_pmOldList;
pmListItem_t    *cg_pmWaitingList;
pmListItemBig_t *cg_pmWaitingListBig;
pmListItemBig_t cg_pmStackBig[NUM_PM_STACK_ITEMS_BIG];

void CG_InitPMGraphics(void) {
	cgs.media.pmImages[PM_DYNAMITE]     = trap_R_RegisterShaderNoMip("gfx/limbo/cm_dynamite");
	cgs.media.pmImages[PM_CONSTRUCTION] = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_MINES]        = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_DEATH]        = trap_R_RegisterShaderNoMip("gfx/hud/pm_death");
	cgs.media.pmImages[PM_MESSAGE]      = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_OBJECTIVE]    = trap_R_RegisterShaderNoMip("sprites/objective");
	cgs.media.pmImages[PM_DESTRUCTION]  = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImages[PM_TEAM]         = trap_R_RegisterShaderNoMip("sprites/voiceChat");
	cgs.media.pmImageAlliesConstruct    = trap_R_RegisterShaderNoMip("gfx/hud/pm_constallied");
	cgs.media.pmImageAxisConstruct      = trap_R_RegisterShaderNoMip("gfx/hud/pm_constaxis");
	cgs.media.pmImageAlliesMine         = trap_R_RegisterShaderNoMip("gfx/hud/pm_mineallied");
	cgs.media.pmImageAxisMine           = trap_R_RegisterShaderNoMip("gfx/hud/pm_mineaxis");
	cgs.media.hintKey                   = trap_R_RegisterShaderNoMip("gfx/hud/keyboardkey_old");
}

void CG_InitPM(void) {
	memset(&cg_pmStack, 0, sizeof (cg_pmStack));
	memset(&cg_pmStackBig, 0, sizeof (cg_pmStackBig));

	cg_pmOldList        = NULL;
	cg_pmWaitingList    = NULL;
	cg_pmWaitingListBig = NULL;
}

void CG_AddToListFront(pmListItem_t **list, pmListItem_t *item) {
	item->next = *list;
	*list      = item;
}

void CG_UpdatePMLists(void) {
	pmListItem_t    *listItem;
	pmListItem_t    *lastItem;
	pmListItemBig_t *listItem2;

	listItem = cg_pmWaitingList;
	if (listItem) {
		int t = (listItem->time + cg_popupTime.integer);
		if (cg.time > t) {
			if (listItem->next) {
				// there's another item waiting to come on, so move to old list
				cg_pmWaitingList       = listItem->next;
				cg_pmWaitingList->time = cg.time; // set time we popped up at

				CG_AddToListFront(&cg_pmOldList, listItem);
			} else {
				if (cg.time > t + cg_popupStayTime.integer + cg_popupFadeTime.integer) {
					// we're gone completely
					cg_pmWaitingList = NULL;
					listItem->inuse  = qfalse;
					listItem->next   = NULL;
				}
			}
		}
	}

	listItem = cg_pmOldList;
	lastItem = NULL;
	while (listItem) {
		int t = (listItem->time + cg_popupTime.integer + cg_popupStayTime.integer + cg_popupFadeTime.integer);
		if (cg.time > t) {
			// nuke this, and everything below it (though there shouldn't BE anything below us anyway)
			pmListItem_t *next;

			if (!lastItem) {
				// we're the top of the old list, so set to NULL
				cg_pmOldList = NULL;
			} else {
				lastItem->next = NULL;
			}

			do {
				next = listItem->next;

				listItem->next  = NULL;
				listItem->inuse = qfalse;
				listItem        = next;
			} while (listItem);
			break;
		}

		lastItem = listItem;
		listItem = listItem->next;
	}

	listItem2 = cg_pmWaitingListBig;
	if (listItem2) {
		int t = listItem2->time + cg_popupTime.integer;
		if (cg.time > t) {
			if (listItem2->next) {
				// there's another item waiting to come on, so kill us and shove the next one to the front
				cg_pmWaitingListBig       = listItem2->next;
				cg_pmWaitingListBig->time = cg.time; // set time we popped up at

				listItem2->inuse = qfalse;
				listItem2->next  = NULL;
			} else {
				if (cg.time > t + cg_popupStayTime.integer + cg_popupFadeTime.integer) {
					// we're gone completely
					cg_pmWaitingListBig = NULL;
					listItem2->inuse    = qfalse;
					listItem2->next     = NULL;
				}
			}
		}
	}
}

pmListItem_t *CG_FindFreePMItem(void) {
	pmListItem_t *listItem;
	pmListItem_t *lastItem;

	int i = 0;

	for ( ; i < NUM_PM_STACK_ITEMS; i++) {
		if (!cg_pmStack[i].inuse) {
			return &cg_pmStack[i];
		}
	}

	// no totally free items, so just grab the last item in the oldlist
	lastItem = listItem = cg_pmOldList;
	if (lastItem) {
		while (listItem->next) {
			lastItem = listItem;
			listItem = listItem->next;
		}

		if (lastItem == cg_pmOldList) {
			cg_pmOldList = NULL;
		} else {
			lastItem->next = NULL;
		}

		listItem->inuse = qfalse;

		return listItem;
	} else {
		// there is no old list... PANIC!
		return NULL;
	}
}

void CG_AddPMItem(popupMessageType_t type, const char *message, qhandle_t shader) {
	pmListItem_t *listItem;
	char         *end;

	if (!message || !*message) {
		return;
	}
	if ((int)type < 0 || type >= PM_NUM_TYPES) {
		CG_Printf("Invalid popup type: %d\n", type);
		return;
	}

	listItem = CG_FindFreePMItem();

	if (!listItem) {
		return;
	}

	if (shader) {
		listItem->shader = shader;
	} else {
		listItem->shader = cgs.media.pmImages[type];
	}

	listItem->inuse = qtrue;
	listItem->type  = type;
	Q_strncpyz(listItem->message, message, sizeof (cg_pmStack[0].message));

	// rain - moved this: print and THEN chop off the newline, as the
	// console deals with newlines perfectly.  We do chop off the newline
	// at the end, if any, though.
	if (listItem->message[strlen(listItem->message) - 1] == '\n') {
		listItem->message[strlen(listItem->message) - 1] = 0;
	}

	trap_Print(va("%s\n", listItem->message));

	end = strchr(listItem->message, '\n');
	while (end) {
		*end = '\0';
		end  = strchr(listItem->message, '\n');
	}

	// rain - don't eat popups for empty lines
	if (*listItem->message == '\0') {
		return;
	}

	if (!cg_pmWaitingList) {
		cg_pmWaitingList = listItem;
		listItem->time   = cg.time;
	} else {
		pmListItem_t *loop = cg_pmWaitingList;
		while (loop->next) {
			loop = loop->next;
		}

		loop->next = listItem;
	}
}

void CG_DrawPMItems(void) {
	vec4_t       colour     = { 0.f, 0.f, 0.f, 1.f };
	vec4_t       colourText = { 1.f, 1.f, 1.f, 1.f };
	float        t;
	int          i, size;
	pmListItem_t *listItem = cg_pmOldList;
	float        y         = 360;
	int          numPopups = 0;

	if (cg_drawSmallPopupIcons.integer) {
		size = PM_ICON_SIZE_SMALL;
		y   += 4;
	} else {
		size = PM_ICON_SIZE_NORMAL;
	}

	if (!cg_pmWaitingList) {
		return;
	}

	if (cg_numPopups.integer <= 0) {
		return;
	}

	t = cg_pmWaitingList->time + cg_popupTime.integer + cg_popupStayTime.integer;
	if (cg.time > t) {
		colourText[3] = colour[3] = 1 - ((cg.time - t) / (float)cg_popupFadeTime.integer);
	}

	trap_R_SetColor(colourText);
	CG_DrawPic(4, y, size, size, cg_pmWaitingList->shader);
	trap_R_SetColor(NULL);
	CG_Text_Paint_Ext(4 + size + 2, y + 12, 0.2f, 0.2f, colourText, cg_pmWaitingList->message, 0, 0, 0, &cgs.media.limboFont2);

	if (cg_numPopups.integer >= 1 && cg_numPopups.integer <= 16) {
		numPopups = cg_numPopups.integer - 1;
	} else {
		numPopups = 4;
	}

	for (i = 0; i < numPopups && listItem; i++, listItem = listItem->next) {
		y -= size + 2;

		t = listItem->time + cg_popupTime.integer + cg_popupStayTime.integer;
		if (cg.time > t) {
			colourText[3] = colour[3] = 1 - ((cg.time - t) / (float)cg_popupFadeTime.integer);
		} else {
			colourText[3] = colour[3] = 1.f;
		}

		trap_R_SetColor(colourText);
		CG_DrawPic(4, y, size, size, listItem->shader);
		trap_R_SetColor(NULL);
		CG_Text_Paint_Ext(4 + size + 2, y + 12, 0.2f, 0.2f, colourText, listItem->message, 0, 0, 0, &cgs.media.limboFont2);
	}
}

void CG_DrawPMItemsBig(void) {
	vec4_t colourText = { 1.f, 1.f, 1.f, 1.f };
	float  t;
	float  y = 270;
	float  w;

	if (!cg_pmWaitingListBig) {
		return;
	}

	t = cg_pmWaitingListBig->time + cg_popupTime.integer + cg_popupStayTime.integer;
	if (cg.time > t) {
		colourText[3] = 1 - ((cg.time - t) / (float)cg_popupFadeTime.integer);
	}

	trap_R_SetColor(colourText);
	CG_DrawPic(640 - 56, y, 48, 48, cg_pmWaitingListBig->shader);
	trap_R_SetColor(NULL);


	w = CG_Text_Width_Ext(cg_pmWaitingListBig->message, 0.22f, 0, &cgs.media.limboFont2);
	CG_Text_Paint_Ext(640 - 4 - w, y + 56, 0.22f, 0.24f, colourText, cg_pmWaitingListBig->message, 0, 0, 0, &cgs.media.limboFont2);
}

const char *CG_GetPMItemText(centity_t *cent) {
	switch (cent->currentState.effect1Time) {
	case PM_DYNAMITE:
		switch (cent->currentState.effect2Time) {
		case 0:
			return va("Planted at %s.", CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		case 1:
			return va("Defused at %s.", CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		}
		break;
	case PM_CONSTRUCTION:
		switch (cent->currentState.effect2Time) {
		case -1:
			return CG_ConfigString(CS_STRINGS + cent->currentState.effect3Time);
		case 0:
			return va("%s has been constructed.", CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		}
		break;
	case PM_DESTRUCTION:
		switch (cent->currentState.effect2Time) {
		case 0:
			return va("%s has been damaged.", CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		case 1:
			return va("%s has been destroyed.", CG_ConfigString(CS_OID_TRIGGERS + cent->currentState.effect3Time));
		}
		break;
	case PM_MINES:
		if ((int)cgs.clientinfo[cg.clientNum].team == cent->currentState.effect2Time) {
			return NULL;
		}
		return va("Spotted by %s^7 at %s", cgs.clientinfo[cent->currentState.effect3Time].name, BG_GetLocationString(cent->currentState.origin));
	case PM_OBJECTIVE:
		switch (cent->currentState.density) {
		case 0:
			return va("%s have stolen %s!", cent->currentState.effect2Time == TEAM_ALLIES ? "Allies" : "Axis", CG_ConfigString(CS_STRINGS + cent->currentState.effect3Time));
		case 1:
			return va("%s have returned %s!", cent->currentState.effect2Time == TEAM_ALLIES ? "Allies" : "Axis", CG_ConfigString(CS_STRINGS + cent->currentState.effect3Time));
		}
		break;
	case PM_TEAM:
		switch (cent->currentState.density) {
		case 0:         // joined
		{
			const char *teamstr = NULL;
			switch (cent->currentState.effect2Time) {
			case TEAM_AXIS:
				teamstr = "Axis team";
				break;
			case TEAM_ALLIES:
				teamstr = "Allied team";
				break;
			default:
				teamstr = "Spectators";
				break;
			}

			return va("%s^7 has joined the %s^7!", cgs.clientinfo[cent->currentState.effect3Time].name, teamstr);
		}
		case 1:
			return va("%s^7 disconnected", cgs.clientinfo[cent->currentState.effect3Time].name);
		}
	}

	return NULL;
}

void CG_PlayPMItemSound(centity_t *cent) {
	switch (cent->currentState.effect1Time) {
	case PM_DYNAMITE:
		switch (cent->currentState.effect2Time) {
		case 0:
			if (cent->currentState.teamNum == TEAM_AXIS) {
				CG_SoundPlaySoundScript("axis_hq_dynamite_planted", NULL, -1, qtrue);
			} else {
				CG_SoundPlaySoundScript("allies_hq_dynamite_planted", NULL, -1, qtrue);
			}
			break;
		case 1:
			if (cent->currentState.teamNum == TEAM_AXIS) {
				CG_SoundPlaySoundScript("axis_hq_dynamite_defused", NULL, -1, qtrue);
			} else {
				CG_SoundPlaySoundScript("allies_hq_dynamite_defused", NULL, -1, qtrue);
			}
			break;
		}
		break;
	case PM_MINES:
		if ((int)cgs.clientinfo[cg.clientNum].team != cent->currentState.effect2Time) {
			// inverted teams
			if (cent->currentState.effect2Time == TEAM_AXIS) {
				CG_SoundPlaySoundScript("allies_hq_mines_spotted", NULL, -1, qtrue);
			} else {
				CG_SoundPlaySoundScript("axis_hq_mines_spotted", NULL, -1, qtrue);
			}
		}
		break;
	case PM_OBJECTIVE:
		switch (cent->currentState.density) {
		case 0:
			if (cent->currentState.effect2Time == TEAM_AXIS) {
				CG_SoundPlaySoundScript("axis_hq_objective_taken", NULL, -1, qtrue);
			} else {
				CG_SoundPlaySoundScript("allies_hq_objective_taken", NULL, -1, qtrue);
			}
			break;
		case 1:
			if (cent->currentState.effect2Time == TEAM_AXIS) {
				CG_SoundPlaySoundScript("axis_hq_objective_secure", NULL, -1, qtrue);
			} else {
				CG_SoundPlaySoundScript("allies_hq_objective_secure", NULL, -1, qtrue);
			}
			break;
		}
		break;
	default:
		break;
	}
}

qhandle_t CG_GetPMItemIcon(centity_t *cent) {
	switch (cent->currentState.effect1Time) {
	case PM_CONSTRUCTION:
		if (cent->currentState.density == TEAM_AXIS) {
			return cgs.media.pmImageAxisConstruct;
		}
		return cgs.media.pmImageAlliesConstruct;
	case PM_MINES:
		if (cent->currentState.effect2Time == TEAM_AXIS) {
			return cgs.media.pmImageAlliesMine;
		}
		return cgs.media.pmImageAxisMine;
	default:
		return cgs.media.pmImages[cent->currentState.effect1Time];
	}

	return 0;
}
