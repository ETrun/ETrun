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

#define SOUNDEVENT(sound) trap_S_StartLocalSound(sound, CHAN_LOCAL_SOUND)

#define SOUND_SELECT    SOUNDEVENT(cgs.media.sndLimboSelect)
#define SOUND_FOCUS     SOUNDEVENT(cgs.media.sndLimboFocus)
#define SOUND_CANCEL    SOUNDEVENT(cgs.media.sndLimboCancel)

void CG_DrawBorder(float x, float y, float w, float h, qboolean fill, qboolean drawMouseOver);

team_t teamOrder[3] =
{
	TEAM_ALLIES,
	TEAM_AXIS,
	TEAM_SPECTATOR,
};

panel_button_text_t nameEditFont =
{
	0.22f,                   0.24f,
	{ 1.f,                   1.f,  1.f,0.8f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

panel_button_text_t classBarFont =
{
	0.22f,                 0.24f,
	{ 0.f,                 0.f,  0.f,0.8f },
	0,                     0,
	&cgs.media.limboFont2,
};

panel_button_text_t titleLimboFont =
{
	0.24f,                 0.28f,
	{ 1.f,                 1.f,  1.f,0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t titleLimboFontBig =
{
	0.3f,                  0.3f,
	{ 1.f,                 1.f, 1.f,  0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t titleLimboFontBigCenter =
{
	0.3f,                  0.3f,
	{ 1.f,                 1.f,              1.f,  0.6f },
	0,                     ITEM_ALIGN_CENTER,
	&cgs.media.limboFont1,
};

panel_button_text_t spawnLimboFont =
{
	0.18f,                 0.22f,
	{ 1.f,                 1.f,  1.f,0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t weaponButtonFont =
{
	0.33f,                 0.33f,
	{ 0.f,                 0.f,  0.f,0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t weaponPanelNameFont =
{
	0.20f,                 0.24f,
	{ 1.0f,                1.0f, 1.0f,  0.4f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t weaponPanelFilterFont =
{
	0.17f,                    0.17f,
	{ 1.0f,                   1.0f, 1.0f,  0.6f },
	0,                        0,
	&cgs.media.limboFont1_lo,
};

panel_button_text_t weaponPanelStatsFont =
{
	0.15f,                    0.17f,
	{ 1.0f,                   1.0f, 1.0f,  0.6f },
	0,                        0,
	&cgs.media.limboFont1_lo,
};

panel_button_text_t weaponPanelStatsPercFont =
{
	0.2f,                  0.2f,
	{ 1.0f,                1.0f,1.0f,    0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_t rightLimboPannel =
{
	"gfx/limbo/limbo_back",
	NULL,
	{ 440,                    0,  200, 480 },
	{ 0,                      0,  0,   0, 0, 0, 0, 0},
	NULL,                     /* font		*/
	NULL,                     /* keyDown	*/
	NULL,                     /* keyUp	*/
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

#define TEAM_COUNTER_GAP    ((TEAM_COUNTER_SIZE - (TEAM_COUNTER_WIDTH * TEAM_COUNTER_COUNT)) / (TEAM_COUNTER_COUNT + 1.f))
#define TEAM_COUNTER_COUNT  3.f
#define TEAM_COUNTER_WIDTH  20.f
#define TEAM_COUNTER_X      432.f
#define TEAM_COUNTER_SIZE   (660.f - TEAM_COUNTER_X)
#define TEAM_COUNTER_BUTTON_DIFF -24.f
#define TEAM_COUNTER_SPACING    4.f

#define TEAM_COUNTER(number)             \
	panel_button_t teamCounter ## number = {      \
		NULL,                                   \
		NULL,                                   \
		{ TEAM_COUNTER_X + TEAM_COUNTER_GAP + (number * (TEAM_COUNTER_GAP + TEAM_COUNTER_WIDTH)),236,                                                                                     TEAM_COUNTER_WIDTH, 14 },  \
		{ 1,                         number,                                                                                  0,                  0, 0, 0, 0, 0},        \
		NULL,                        /* font		*/              \
		NULL,                        /* keyDown	*/                  \
		NULL,                        /* keyUp	*/                  \
		CG_LimboPanel_RenderCounter,            \
		NULL,                                   \
		0                                       \
	};                                          \
	panel_button_t teamCounterLight ## number = { \
		NULL,                                   \
		NULL,                                   \
		{ TEAM_COUNTER_X + TEAM_COUNTER_GAP + (number * (TEAM_COUNTER_GAP + TEAM_COUNTER_WIDTH)) - 20,236,                                                                                          16, 16 }, \
		{ 1,                       number,                                                                                       0,  0, 0, 0, 0, 0},        \
		NULL,                      /* font		*/              \
		NULL,                      /* keyDown	*/                  \
		NULL,                      /* keyUp	*/                  \
		CG_LimboPanel_RenderLight,              \
		NULL,                                   \
		0                                       \
	};                                          \
	panel_button_t teamButton ## number = {       \
		NULL,                                   \
		NULL,                                   \
		{ TEAM_COUNTER_X + TEAM_COUNTER_GAP + (number * (TEAM_COUNTER_GAP + TEAM_COUNTER_WIDTH) + (TEAM_COUNTER_BUTTON_DIFF / 2.f)) - 17 + TEAM_COUNTER_SPACING, \
		  188 + TEAM_COUNTER_SPACING, \
		  TEAM_COUNTER_WIDTH - TEAM_COUNTER_BUTTON_DIFF + 20 - 2 * TEAM_COUNTER_SPACING, \
		  44 - 2 * TEAM_COUNTER_SPACING },  \
		{ number,                                                                        0,0, 0, 0, 0, 0, 0 },        \
		NULL,                                                                            /* font		*/              \
		CG_LimboPanel_TeamButton_KeyDown,                                                /* keyDown	*/ \
		NULL,                                                                            /* keyUp	*/                  \
		CG_LimboPanel_RenderTeamButton,         \
		NULL,                                   \
		0                                   \
	}

TEAM_COUNTER(0);
TEAM_COUNTER(1);
TEAM_COUNTER(2);

#define CLASS_COUNTER_GAP   ((CLASS_COUNTER_SIZE - (CLASS_COUNTER_WIDTH * CLASS_COUNTER_COUNT)) / (CLASS_COUNTER_COUNT + 1.f))
#define CLASS_COUNTER_COUNT 5.f
#define CLASS_COUNTER_WIDTH 20.f
#define CLASS_COUNTER_X     435.f
#define CLASS_COUNTER_SIZE  (645.f - CLASS_COUNTER_X)
#define CLASS_COUNTER_LIGHT_DIFF 4.f
#define CLASS_COUNTER_BUTTON_DIFF -18.f
#define CLASS_COUNTER(number)            \
	panel_button_t classCounter ## number = {     \
		NULL,                                   \
		NULL,                                   \
		{ CLASS_COUNTER_X + CLASS_COUNTER_GAP + (number * (CLASS_COUNTER_GAP + CLASS_COUNTER_WIDTH)),302,                                                                                         CLASS_COUNTER_WIDTH, 14 }, \
		{ 0,                         number,                                                                                      0,                   0, 0, 0, 0, 0},        \
		NULL,                        /* font		*/              \
		NULL,                        /* keyDown	*/                  \
		NULL,                        /* keyUp	*/                  \
		CG_LimboPanel_RenderCounter,            \
		NULL,                                   \
		0                                       \
	};                                          \
	panel_button_t classButton ## number = {      \
		NULL,                                   \
		NULL,                                   \
		{ CLASS_COUNTER_X + CLASS_COUNTER_GAP + (number * (CLASS_COUNTER_GAP + CLASS_COUNTER_WIDTH)) + (CLASS_COUNTER_BUTTON_DIFF / 2.f),266,                                                                                                                             CLASS_COUNTER_WIDTH - CLASS_COUNTER_BUTTON_DIFF, 34 },   \
		{ 0,                             number,                                                                                                                          0,                                               0, 0, 0, 0, 0},        \
		NULL,                            /* font		*/              \
		CG_LimboPanel_ClassButton_KeyDown,/* keyDown	*/  \
		NULL,                            /* keyUp	*/                  \
		CG_LimboPanel_RenderClassButton,        \
		NULL,                                   \
		0                                       \
	}

panel_button_t classBar =
{
	"gfx/limbo/lightup_bar",
	NULL,
	{ 470,                    320,140, 20 },
	{ 0,                      0,  0,   0, 0, 0, 0, 0},
	NULL,                     /* font		*/
	NULL,                     /* keyDown	*/
	NULL,                     /* keyUp	*/
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

panel_button_t classBarText =
{
	NULL,
	NULL,
	{ 460,                      334,   160, 16 },
	{ 0,                        0,     0,   0, 0, 0, 0, 0},
	&classBarFont,              /* font		*/
	NULL,                       /* keyDown	*/
	NULL,                       /* keyUp	*/
	CG_LimboPanel_ClassBar_Draw,
	NULL,
	0
};

CLASS_COUNTER(0);
CLASS_COUNTER(1);
CLASS_COUNTER(2);
CLASS_COUNTER(3);
CLASS_COUNTER(4);

// =======================

panel_button_t weaponPanel =
{
	NULL,
	NULL,
	{ 455,                    353,   140, 56 },
	{ 0,                      0,     0,   0, 0, 0, 0, 0},
	NULL,                     /* font		*/
	CG_LimboPanel_WeaponPanel_KeyDown,/* keyDown	*/
	CG_LimboPanel_WeaponPanel_KeyUp,/* keyUp	*/
	CG_LimboPanel_WeaponPanel,
	NULL,
	0
};

panel_button_t weaponLight1 =
{
	NULL,
	NULL,
	{ 605,                     362,   20, 20 },
	{ 0,                       0,     0,  0, 0, 0, 0, 0},
	NULL,                      /* font		*/
	CG_LimboPanel_WeaponLights_KeyDown,/* keyDown	*/
	NULL,                      /* keyUp	*/
	CG_LimboPanel_WeaponLights,
	NULL,
	0
};

panel_button_t weaponLight1Text =
{
	NULL,
	"1",
	{ 609,                     378,   0, 0 },
	{ 0,                       0,     0, 0, 0, 0, 0, 0},
	&weaponButtonFont,         /* font		*/
	NULL,                      /* keyDown	*/
	NULL,                      /* keyUp	*/
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

panel_button_t weaponLight2 =
{
	NULL,
	NULL,
	{ 605,                     386,   20, 20 },
	{ 1,                       0,     0,  0, 0, 0, 0, 0},
	NULL,                      /* font		*/
	CG_LimboPanel_WeaponLights_KeyDown,/* keyDown	*/
	NULL,                      /* keyUp	*/
	CG_LimboPanel_WeaponLights,
	NULL,
	0
};

panel_button_t weaponLight2Text =
{
	NULL,
	"2",
	{ 609,                     402,   0, 0 },
	{ 0,                       0,     0, 0, 0, 0, 0, 0},
	&weaponButtonFont,         /* font		*/
	NULL,                      /* keyDown	*/
	NULL,                      /* keyUp	*/
	BG_PanelButtonsRender_Text,
	NULL,
	0
};
// =======================

panel_button_t okButtonText =
{
	NULL,
	"OK",
	{ 484,                     469,   100, 40 },
	{ 0,                       0,     0,   0, 0, 0, 0, 0},
	&titleLimboFont,           /* font		*/
	NULL,                      /* keyDown	*/
	NULL,                      /* keyUp	*/
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

panel_button_t okButton =
{
	NULL,
	NULL,
	{ 454 + 2,                454 + 2,       82 - 4, 18 - 4 },
	{ 0,                      0,             0,      0, 0, 0, 0, 0},
	NULL,                     /* font		*/
	CG_LimboPanel_OkButton_KeyDown,/* keyDown	*/
	NULL,                     /* keyUp	*/
	CG_LimboPanel_Border_Draw,
	NULL,
	0
};

panel_button_t cancelButtonText =
{
	NULL,
	"CANCEL",
	{ 556,                     469,100, 40 },
	{ 0,                       0,  0,   0, 0, 0, 0, 0},
	&titleLimboFont,           /* font		*/
	NULL,                      /* keyDown	*/
	NULL,                      /* keyUp	*/
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

panel_button_t cancelButton =
{
	NULL,
	NULL,
	{ 543 + 2,                454 + 2,       82 - 4, 18 - 4 },
	{ 0,                      0,             0,      0, 0, 0, 0, 0},
	NULL,                     /* font		*/
	CG_LimboPanel_CancelButton_KeyDown,/* keyDown	*/
	NULL,                     /* keyUp	*/
	CG_LimboPanel_Border_Draw,
	NULL,
	0
};

// =======================

panel_button_t nameEdit =
{
	NULL,
	"limboname",
	{ 480,                       150,120, 20 },
	{ 0,                         0,  0,   0, 0, 0, 0, 0},
	&nameEditFont,               /* font		*/
	BG_PanelButton_EditClick,    /* keyDown	*/
	NULL,                        /* keyUp	*/
	BG_PanelButton_RenderEdit,
	CG_LimboPanel_NameEditFinish,
	0
};

panel_button_t *limboPanelButtons[] =
{
	&rightLimboPannel,

	&classCounter0,    &classCounter1,      &classCounter2,     &classCounter3, &classCounter4,
	&classButton0,     &classButton1,       &classButton2,      &classButton3,  &classButton4,

	&classBar,         &classBarText,

	&teamCounter0,     &teamCounter1,       &teamCounter2,
	&teamCounterLight0,&teamCounterLight1,  &teamCounterLight2,
	&teamButton0,      &teamButton1,        &teamButton2,

	&okButton,         &okButtonText,
	&cancelButton,     &cancelButtonText,

	&nameEdit,

	&weaponLight1,     &weaponLight2,
	&weaponLight1Text, &weaponLight2Text,
	&weaponPanel,

	NULL,
};

void CG_LimboPanel_NameEditFinish(panel_button_t *button) {
	char buffer[256];

	trap_Cvar_VariableStringBuffer(button->text, buffer, 256);
	trap_Cvar_Set("name", buffer);
}

qboolean CG_LimboPanel_CancelButton_KeyDown(panel_button_t *button, int key) {
	// Nico, silent GCC
	(void)button;

	if (key == K_MOUSE1) {
		SOUND_CANCEL;

		if (cgs.limboLoadoutModified) {
			trap_SendClientCommand("rs");

			cgs.limboLoadoutSelected = qfalse;
		}

		CG_EventHandling(CGAME_EVENT_NONE, qfalse);

		return qtrue;
	}
	return qfalse;
}

void CG_LimboPanel_SendSetupMsg(qboolean forceteam) {
	weapon_t     weap1, weap2;
	const char   *str;
	team_t       team;
	weaponType_t *wt;

	if (forceteam) {
		team = CG_LimboPanel_GetTeam();
	} else {
		team = cgs.clientinfo[cg.clientNum].team;
	}

	if (team == TEAM_SPECTATOR) {
		if (forceteam) {
			if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR) {
				trap_SendClientCommand("team s 0 0 0\n");
			}
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		}
		return;
	}

	weap1 = CG_LimboPanel_GetSelectedWeaponForSlot(1);
	weap2 = CG_LimboPanel_GetSelectedWeaponForSlot(0);

	switch (team) {
	case TEAM_AXIS:
		str = "r";
		break;
	case TEAM_ALLIES:
		str = "b";
		break;
	default:
		str = NULL;     // rain - don't go spec
		break;
	}

	// rain - if this happens, we're dazed and confused, abort
	if (!str) {
		return;
	}

	trap_SendClientCommand(va("team %s %i %i %i\n", str, CG_LimboPanel_GetClass(), weap1, weap2));

	if (forceteam) {
		CG_EventHandling(CGAME_EVENT_NONE, qfalse);
	}

	// print center message
	switch (CG_LimboPanel_GetTeam()) {
	case TEAM_AXIS:
		str = "Axis";
		break;
	case TEAM_ALLIES:
		str = "Allied";
		break;
	default:     // rain - added default
		str = "unknown";
		break;
	}

	wt = WM_FindWeaponTypeForWeapon(weap1);
	CG_PriorityCenterPrint(va("You will spawn as an %s %s with a %s.", str, BG_ClassnameForNumber(CG_LimboPanel_GetClass()), wt ? wt->desc : "^1UNKNOWN WEAPON"), SCREEN_HEIGHT - 88, SMALLCHAR_WIDTH, -1);

	cgs.limboLoadoutSelected = qtrue;
	cgs.limboLoadoutModified = qtrue;
}

qboolean CG_LimboPanel_OkButton_KeyDown(panel_button_t *button, int key) {
	// Nico, silent GCC
	(void)button;

	if (key == K_MOUSE1) {
		SOUND_SELECT;

		CG_LimboPanel_SendSetupMsg(qtrue);

		return qtrue;
	}

	return qfalse;
}

qboolean CG_LimboPanel_TeamButton_KeyDown(panel_button_t *button, int key) {
	if (key == K_MOUSE1) {
		SOUND_SELECT;

		if (cgs.ccSelectedTeam != button->data[0]) {
			cgs.ccSelectedTeam = button->data[0];

			CG_LimboPanel_SetSelectedWeaponNumForSlot(0, 0);

			cgs.limboLoadoutModified = qtrue;
		}

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_RenderTeamButton(panel_button_t *button) {
	qhandle_t shader;

	trap_R_SetColor(colorBlack);
	CG_DrawPic(button->rect.x + 1, button->rect.y + 1, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_off);

	trap_R_SetColor(NULL);
	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_off);

	if (CG_LimboPanel_GetTeam() == teamOrder[button->data[0]]) {
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_on);
	} else if (BG_CursorInRect(&button->rect)) {
		vec4_t clr2 = { 1.f, 1.f, 1.f, 0.4f };

		trap_R_SetColor(clr2);
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_on);
		trap_R_SetColor(NULL);
	}

	switch (button->data[0]) {
	case 0:
		shader = cgs.media.limboTeamButtonAllies;
		break;
	case 1:
		shader = cgs.media.limboTeamButtonAxis;
		break;
	case 2:
		shader = cgs.media.limboTeamButtonSpec;
		break;
	default:
		return;
	}

	trap_R_SetColor(NULL);
	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, shader);
}

qboolean CG_LimboPanel_ClassButton_KeyDown(panel_button_t *button, int key) {
	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		return qfalse;
	}

	if (key == K_MOUSE1) {
		SOUND_SELECT;

		if (cgs.ccSelectedClass != button->data[1]) {
			cgs.ccSelectedClass = button->data[1];

			CG_LimboPanel_SetSelectedWeaponNumForSlot(0, 0);

			CG_LimboPanel_SendSetupMsg(qfalse);
		}

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_ClassBar_Draw(panel_button_t *button) {
	const char *text = NULL;
	char       buffer[64];
	float      w;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		text = "JOIN A TEAM";
	} else if (BG_CursorInRect(&classButton0.rect)) {
		text = BG_ClassnameForNumber(0);
	} else if (BG_CursorInRect(&classButton1.rect)) {
		text = BG_ClassnameForNumber(1);
	} else if (BG_CursorInRect(&classButton2.rect)) {
		text = BG_ClassnameForNumber(2);
	} else if (BG_CursorInRect(&classButton3.rect)) {
		text = BG_ClassnameForNumber(3);
	} else if (BG_CursorInRect(&classButton4.rect)) {
		text = BG_ClassnameForNumber(4);
	}

	if (!text) {
		text = BG_ClassnameForNumber(CG_LimboPanel_GetClass());
	}

	Q_strncpyz(buffer, text, sizeof (buffer));
	Q_strupr(buffer);

	w = CG_Text_Width_Ext(buffer, button->font->scalex, 0, button->font->font);
	CG_Text_Paint_Ext(button->rect.x + (button->rect.w - w) * 0.5f, button->rect.y, button->font->scalex, button->font->scaley, button->font->colour, buffer, 0, 0, button->font->style, button->font->font);
}

void CG_LimboPanel_RenderClassButton(panel_button_t *button) {
	CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButton2Back_off);

	if (CG_LimboPanel_GetTeam() != TEAM_SPECTATOR) {
		if (button->data[1] == CG_LimboPanel_GetClass()) {
			CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButton2Back_on);
		} else if (BG_CursorInRect(&button->rect)) {
			vec4_t clr = { 1.f, 1.f, 1.f, 0.4f };

			trap_R_SetColor(clr);
			CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButton2Back_on);
			trap_R_SetColor(NULL);
		}
	}

	if (CG_LimboPanel_GetTeam() != TEAM_SPECTATOR && button->data[1] == CG_LimboPanel_GetClass()) {
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButtons2[button->data[1]]);
	} else {
		vec4_t clr2 = { 1.f, 1.f, 1.f, 0.75f };

		trap_R_SetColor(clr2);
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButtons2[button->data[1]]);
		trap_R_SetColor(NULL);
	}
}

qboolean CG_LimboPanel_RenderLight_GetValue(panel_button_t *button) {
	switch (button->data[0]) {
	case 0:
		return CG_LimboPanel_GetClass() == button->data[1] ? qtrue : qfalse;
	case 1:
		return CG_LimboPanel_GetTeam() == teamOrder[button->data[1]] ? qtrue : qfalse;
	}

	return qfalse;
}

void CG_LimboPanel_RenderLight(panel_button_t *button) {
	if (CG_LimboPanel_RenderLight_GetValue(button)) {
		button->data[3] = button->data[3] ^ 1;

		CG_DrawPic(button->rect.x - 4, button->rect.y - 2, button->rect.w + 4, button->rect.h + 4, button->data[3] ? cgs.media.limboLight_on2 : cgs.media.limboLight_on);
	} else {
		CG_DrawPic(button->rect.x - 4, button->rect.y - 2, button->rect.w + 4, button->rect.h + 4, cgs.media.limboLight_off);
	}
}

qboolean CG_LimboPanel_WeaponLights_KeyDown(panel_button_t *button, int key) {
	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		return qfalse;
	}

	if (key == K_MOUSE1) {
		SOUND_SELECT;

		cgs.ccSelectedWeaponNumber = button->data[0];

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_WeaponLights(panel_button_t *button) {
	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponNumber_off);
	} else {
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, button->data[0] == cgs.ccSelectedWeaponNumber ? cgs.media.limboWeaponNumber_on : cgs.media.limboWeaponNumber_off);
	}
}

qboolean CG_LimboPanel_WeaponPanel_KeyDown(panel_button_t *button, int key) {
	button->data[7] = 0;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		return qfalse;
	}

	if (key == K_MOUSE1) {
		SOUND_SELECT;

		BG_PanelButtons_SetFocusButton(button);
		return qtrue;
	}

	return qfalse;
}

qboolean CG_LimboPanel_WeaponPanel_KeyUp(panel_button_t *button, int key) {
	rectDef_t rect;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		return qfalse;
	}

	if (key == K_MOUSE1 && BG_PanelButtons_GetFocusButton() == button) {
		int cnt, i;

		memcpy(&rect, &button->rect, sizeof (rect));
		rect.y -= rect.h;

		cnt = CG_LimboPanel_WeaponCount();
		for (i = 1; i < cnt; ++i, rect.y -= rect.h) {

			if (!BG_CursorInRect(&rect)) {
				continue;
			}

			if (!CG_LimboPanel_GetSelectedWeaponNum()) {
				CG_LimboPanel_SetSelectedWeaponNum(i);
				CG_LimboPanel_SendSetupMsg(qfalse);
			} else {
				if (i <= CG_LimboPanel_GetSelectedWeaponNum()) {
					CG_LimboPanel_SetSelectedWeaponNum(i - 1);
					CG_LimboPanel_SendSetupMsg(qfalse);
				} else {
					CG_LimboPanel_SetSelectedWeaponNum(i);
					CG_LimboPanel_SendSetupMsg(qfalse);
				}
			}
		}

		BG_PanelButtons_SetFocusButton(NULL);

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_WeaponPanel_DrawWeapon(rectDef_t *rect, weapon_t weap, qboolean highlight, const char *ofTxt, qboolean disabled) {
	weaponType_t *wt    = WM_FindWeaponTypeForWeapon(weap);
	qhandle_t    shader = cgs.media.limboWeaponCard;
	int          width  = CG_Text_Width_Ext(ofTxt, 0.2f, 0, &cgs.media.limboFont2);
	float        x      = rect->x + rect->w - width - 4;
	vec4_t       clr;

	if (!wt) {
		return;
	}

	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, shader);
	if (wt->desc) {
		if (highlight && BG_CursorInRect(rect)) {
			Vector4Copy(weaponPanelNameFont.colour, clr);
			clr[3] *= 1.5;
			CG_Text_Paint_Ext(rect->x + 4, rect->y + 12, weaponPanelNameFont.scalex, weaponPanelNameFont.scaley, clr, wt->desc, 0, 0, weaponPanelNameFont.style, weaponPanelNameFont.font);
		} else {
			CG_Text_Paint_Ext(rect->x + 4, rect->y + 12, weaponPanelNameFont.scalex, weaponPanelNameFont.scaley, weaponPanelNameFont.colour, wt->desc, 0, 0, weaponPanelNameFont.style, weaponPanelNameFont.font);
		}
	}

	{
		float x2, y2, w, h, s0, s1, t0, t1;

		trap_R_SetColor(NULL);

		x2 = rect->x;
		y2 = rect->y + (rect->h * 0.25f);

		CG_LimboPanel_GetWeaponCardIconData(weap, &shader, &w, &h, &s0, &t0, &s1, &t1);

		w *= rect->w;
		h *= rect->h * 0.75f;

		CG_DrawPicST(x2, y2, w, h, s0, t0, s1, t1, shader);

		if (disabled) {
			vec4_t clr = { 1.f, 1.f, 1.f, 0.6f };

			trap_R_SetColor(clr);
			CG_DrawPic(x2, y2 + 4 + (h - 16) * 0.5f, w, 16, cgs.media.limboWeaponCardOOS);
			trap_R_SetColor(NULL);
		}
	}
	CG_Text_Paint_Ext(x, rect->y + rect->h - 2, 0.2f, 0.2f, colorBlack, ofTxt, 0, 0, 0, &cgs.media.limboFont2);
}

#define BRDRSIZE 4
void CG_DrawBorder(float x, float y, float w, float h, qboolean fill, qboolean drawMouseOver) {
	// top / bottom
	CG_DrawPic(x, y - BRDRSIZE, w, BRDRSIZE, cgs.media.limboWeaponCardSurroundH);
	CG_DrawPicST(x, y + h, w, BRDRSIZE, 0.f, 1.f, 1.f, 0.f, cgs.media.limboWeaponCardSurroundH);

	CG_DrawPic(x - BRDRSIZE, y, BRDRSIZE, h, cgs.media.limboWeaponCardSurroundV);
	CG_DrawPicST(x + w, y, BRDRSIZE, h, 1.f, 0.f, 0.f, 1.f, cgs.media.limboWeaponCardSurroundV);

	CG_DrawPicST(x - BRDRSIZE, y - BRDRSIZE, BRDRSIZE, BRDRSIZE, 0.f, 0.f, 1.f, 1.f, cgs.media.limboWeaponCardSurroundC);
	CG_DrawPicST(x + w, y - BRDRSIZE, BRDRSIZE, BRDRSIZE, 1.f, 0.f, 0.f, 1.f, cgs.media.limboWeaponCardSurroundC);
	CG_DrawPicST(x + w, y + h, BRDRSIZE, BRDRSIZE, 1.f, 1.f, 0.f, 0.f, cgs.media.limboWeaponCardSurroundC);
	CG_DrawPicST(x - BRDRSIZE, y + h, BRDRSIZE, BRDRSIZE, 0.f, 1.f, 1.f, 0.f, cgs.media.limboWeaponCardSurroundC);

	if (fill) {
		vec4_t clrBack = { 0.1f, 0.1f, 0.1f, 1.f };

		if (drawMouseOver) {
			rectDef_t rect;

			rect.x = x;
			rect.y = y;
			rect.w = w;
			rect.h = h;

			if (BG_CursorInRect(&rect)) {
				vec4_t clrBack2 = { 0.2f, 0.2f, 0.2f, 1.f };

				CG_FillRect(x, y, w, h, clrBack2);
			} else {
				CG_FillRect(x, y, w, h, clrBack);
			}
		} else {
			CG_FillRect(x, y, w, h, clrBack);
		}
	}
}

void CG_LimboPanel_Border_Draw(panel_button_t *button) {
	CG_DrawBorder(button->rect.x, button->rect.y, button->rect.w, button->rect.h, qtrue, qtrue);
}

void CG_LimboPanel_WeaponPanel(panel_button_t *button) {
	weapon_t weap = CG_LimboPanel_GetSelectedWeapon();
	int      cnt  = CG_LimboPanel_WeaponCount();

	if (cgs.ccSelectedWeapon2 >= CG_LimboPanel_WeaponCount_ForSlot(0)) {
		cgs.ccSelectedWeapon2 = CG_LimboPanel_WeaponCount_ForSlot(0) - 1;
	}

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		vec4_t clr = { 0.f, 0.f, 0.f, 0.4f };

		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponCard);

		trap_R_SetColor(clr);
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponBlendThingy);
		trap_R_SetColor(NULL);

		CG_Text_Paint_Ext(button->rect.x + 4, button->rect.y + 12, weaponPanelNameFont.scalex, weaponPanelNameFont.scaley, weaponPanelNameFont.colour, "SPECTATOR", 0, 0, weaponPanelNameFont.style, weaponPanelNameFont.font);

		return;
	}

	if (BG_PanelButtons_GetFocusButton() == button && cnt > 1) {
		int       i, x;
		rectDef_t rect;
		memcpy(&rect, &button->rect, sizeof (rect));

		CG_LimboPanel_WeaponPanel_DrawWeapon(&rect, weap, qtrue, va("%iof%i", CG_LimboPanel_GetSelectedWeaponNum() + 1, cnt), CG_LimboPanel_RealWeaponIsDisabled(weap));
		if (BG_CursorInRect(&rect) && button->data[7] != 0) {
			SOUND_FOCUS;
			button->data[7] = 0;
		}
		rect.y -= rect.h;

		// render in expanded mode ^
		for (i = 0, x = 1; i < cnt; ++i) {
			weapon_t cycleWeap = CG_LimboPanel_GetWeaponForNumber(i, cgs.ccSelectedWeaponNumber, qtrue);
			if (cycleWeap != weap) {
				CG_LimboPanel_WeaponPanel_DrawWeapon(&rect, cycleWeap, qtrue, va("%iof%i", i + 1, cnt), CG_LimboPanel_RealWeaponIsDisabled(cycleWeap));

				if (BG_CursorInRect(&rect) && button->data[7] != x) {
					SOUND_FOCUS;
					button->data[7] = x;
				}

				rect.y -= rect.h;
				x++;
			}
		}

		CG_DrawBorder(button->rect.x, button->rect.y - ((cnt - 1) * button->rect.h), button->rect.w, button->rect.h * cnt, qfalse, qfalse);
	} else {
		vec4_t clr = { 0.f, 0.f, 0.f, 0.4f };

		// render in normal mode
		CG_LimboPanel_WeaponPanel_DrawWeapon(&button->rect, weap, cnt > 1 ? qtrue : qfalse, va("%iof%i", CG_LimboPanel_GetSelectedWeaponNum() + 1, cnt), CG_LimboPanel_RealWeaponIsDisabled(weap));

		if (cnt <= 1 || !BG_CursorInRect(&button->rect)) {
			vec4_t clr2 = { 1.f, 1.f, 1.f, 0.4f };

			trap_R_SetColor(clr2);
		}
		CG_DrawPic(button->rect.x + button->rect.w - 20, button->rect.y + 4, 16, 12, cgs.media.limboWeaponCardArrow);

		trap_R_SetColor(clr);
		CG_DrawPic(button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponBlendThingy);
		trap_R_SetColor(NULL);
	}
}

void CG_LimboPanel_RenderCounterNumber(float x, float y, float w, float h, float number, qhandle_t shaderBack, qhandle_t shaderRoll, int numbuttons) {
	float numberS = (((numbuttons - 1) - number) + 0) * (1.f / numbuttons);
	float numberE = (((numbuttons - 1) - number) + 1) * (1.f / numbuttons);

	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, 0, 0, 1, 1, shaderBack);
	trap_R_DrawStretchPic(x, y, w, h, 0, numberS, 1, numberE, shaderRoll);
}

int CG_LimboPanel_RenderCounter_ValueForButton(panel_button_t *button) {
	int i, count = 0;

	switch (button->data[0]) {
	case 0:     // class counts
		if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR || CG_LimboPanel_GetRealTeam() != CG_LimboPanel_GetTeam()) {
			return 0;     // dont give class counts unless we are on that team (or spec)
		}
		for (i = 0; i < MAX_CLIENTS; ++i) {
			if (!cgs.clientinfo[i].infoValid) {
				continue;
			}
			if (cgs.clientinfo[i].team != CG_LimboPanel_GetTeam() || cgs.clientinfo[i].cls != button->data[1]) {
				continue;
			}

			count++;
		}
		return count;
	case 1:     // team counts
		for (i = 0; i < MAX_CLIENTS; ++i) {
			if (!cgs.clientinfo[i].infoValid) {
				continue;
			}

			if (cgs.clientinfo[i].team != teamOrder[button->data[1]]) {
				continue;
			}

			count++;
		}
		return count;
	case 4:     // skills
		return (1 << count) - 1;
	default:
		break;
	}

	return 0;
}

int CG_LimboPanel_RenderCounter_RollTimeForButton(panel_button_t *button) {
	float diff;

	switch (button->data[0]) {
	case 0:     // class counts
	case 1:     // team counts
		return 100.f;

	case 4:     // skills
		return 1000.f;

	case 6:     // stats
		diff = Q_fabs(button->data[3] - CG_LimboPanel_RenderCounter_ValueForButton(button));
		if (diff < 5) {
			return 200.f / diff;
		}
		return 50.f;

	case 5:     // clock
	case 3:     // respawn time
	case 2:     // xp
		return 50.f;
	}

	return 1000.f;
}

int CG_LimboPanel_RenderCounter_MaxChangeForButton(panel_button_t *button) {
	switch (button->data[0]) {
	case 2:     // xp
	case 6:     // stats
		return 5;
	}

	return 1;
}

int CG_LimboPanel_RenderCounter_NumRollers(panel_button_t *button) {
	switch (button->data[0]) {
	case 0:     // class counts
	case 1:     // team counts
	case 5:     // clock
	case 3:     // respawn time
		return 2;

	case 4:     // skills
		return 4;

	case 6:     // stats
		switch (button->data[1]) {
		case 0:
		case 1:
			return 4;
		case 2:
			return 3;
		}

	case 2:     // xp
		return 6;
	}

	return 0;
}

qboolean CG_LimboPanel_RenderCounter_CountsDown(panel_button_t *button) {
	switch (button->data[0]) {
	case 4:     // skill
	case 2:     // xp
		return qfalse;

	default:
		break;
	}

	return qtrue;
}

qboolean CG_LimboPanel_RenderCounter_CountsUp(panel_button_t *button) {
	switch (button->data[0]) {
	case 4:     // skill
	case 3:     // respawn time
	case 5:     // clock
		return qfalse;

	default:
		break;
	}

	return qtrue;
}

qboolean CG_LimboPanel_RenderCounter_StartSet(panel_button_t *button) {
	switch (button->data[0]) {
	case 3:     // respawn time
	case 5:     // clock
		return qtrue;

	default:
		break;
	}

	return qfalse;
}

qboolean CG_LimboPanel_RenderCounter_IsReversed(panel_button_t *button) {
	switch (button->data[0]) {
	case 4:     // skill
		return qtrue;

	default:
		break;
	}

	return qfalse;
}

void CG_LimboPanel_RenderCounter_GetShaders(panel_button_t *button, qhandle_t *shaderBack, qhandle_t *shaderRoll, int *numimages) {
	switch (button->data[0]) {
	case 4:     // skills
		*shaderBack = cgs.media.limboStar_back;
		*shaderRoll = cgs.media.limboStar_roll;
		*numimages  = 2;
		return;
	default:
		*shaderBack = cgs.media.limboNumber_back;
		*shaderRoll = cgs.media.limboNumber_roll;
		*numimages  = 10;
		return;
	}
}

#define MAX_ROLLERS 8
#define COUNTER_ROLLTOTAL (cg.time - button->data[4])
// Gordon: this function is mental, i love it :)
void CG_LimboPanel_RenderCounter(panel_button_t *button) {
	float     x, w;
	float     count[MAX_ROLLERS];
	int       i, j;
	qhandle_t shaderBack;
	qhandle_t shaderRoll;
	int       numimages;

	float counter_rolltime = CG_LimboPanel_RenderCounter_RollTimeForButton(button);
	int   num              = CG_LimboPanel_RenderCounter_NumRollers(button);
	int   value            = CG_LimboPanel_RenderCounter_ValueForButton(button);

	if (num > MAX_ROLLERS) {
		num = MAX_ROLLERS;
	}

	CG_LimboPanel_RenderCounter_GetShaders(button, &shaderBack, &shaderRoll, &numimages);

	if (COUNTER_ROLLTOTAL < counter_rolltime) {
		// we're rolling
		float frac = COUNTER_ROLLTOTAL / counter_rolltime;

		for (i = 0, j = 1; i < num; ++i, j *= numimages) {
			int valueOld = (button->data[3] / j) % numimages;
			int valueNew = (button->data[5] / j) % numimages;

			if (valueNew == valueOld) {
				count[i] = valueOld;
			} else if ((valueNew > valueOld) != (button->data[5] > button->data[3])) {
				// we're flipping around so....
				if (button->data[5] > button->data[3]) {
					count[i] = valueOld + frac;
				} else {
					count[i] = valueOld - frac;
				}
			} else {
				// normal flip
				count[i] = valueOld + ((valueNew - valueOld) * frac);
			}
		}
	} else {
		if (button->data[3] != button->data[5]) {
			button->data[3] = button->data[5];
		} else if (value != button->data[3]) {
			int maxchange = abs(value - button->data[3]);
			if (maxchange > CG_LimboPanel_RenderCounter_MaxChangeForButton(button)) {
				maxchange = CG_LimboPanel_RenderCounter_MaxChangeForButton(button);
			}

			if (value > button->data[3]) {
				if (CG_LimboPanel_RenderCounter_CountsUp(button)) {
					button->data[5] = button->data[3] + maxchange;
				} else {
					button->data[5] = value;
				}
			} else {
				if (CG_LimboPanel_RenderCounter_CountsDown(button)) {
					button->data[5] = button->data[3] - maxchange;
				} else {
					button->data[5] = value;
				}
			}
			button->data[4] = cg.time;
		}

		for (i = 0, j = 1; i < num; ++i, j *= numimages) {
			count[i] = (int)(button->data[3] / j);
		}
	}

	x = button->rect.x;
	w = (num > 1) ? (button->rect.w / (float)num) : button->rect.w;

	if (CG_LimboPanel_RenderCounter_IsReversed(button)) {
		for (i = 0; i < num; ++i) {
			CG_LimboPanel_RenderCounterNumber(x, button->rect.y, w, button->rect.h, count[i], shaderBack, shaderRoll, numimages);

			x += w + button->data[6];
		}
	} else {
		for (i = num - 1; i >= 0; --i) {
			CG_LimboPanel_RenderCounterNumber(x, button->rect.y, w, button->rect.h, count[i], shaderBack, shaderRoll, numimages);

			x += w + button->data[6];
		}
	}

	if (button->data[0] == 0 || button->data[0] == 1) {
		CG_DrawPic(button->rect.x - 2, button->rect.y - 2, button->rect.w * 1.4f, button->rect.h + 7, cgs.media.limboCounterBorder);
	}
}

void CG_LimboPanel_Setup(void) {
	panel_button_t *button;
	panel_button_t **buttons = limboPanelButtons;
	clientInfo_t   *ci       = &cgs.clientinfo[cg.clientNum];
	char           buffer[256];

	cgs.limboLoadoutModified = qfalse;

	trap_Cvar_VariableStringBuffer("name", buffer, 256);
	trap_Cvar_Set("limboname", buffer);

	for ( ; *buttons; ++buttons) {
		button = (*buttons);

		if (button->onDraw == CG_LimboPanel_RenderCounter && CG_LimboPanel_RenderCounter_StartSet(button)) {
			button->data[3] = button->data[5] = CG_LimboPanel_RenderCounter_ValueForButton(button);
			button->data[4] = 0;
		}
	}

	if (!cgs.limboLoadoutSelected) {
		int              i;
		bg_playerclass_t *classInfo = CG_LimboPanel_GetPlayerClass();

		for (i = 0; i < MAX_WEAPS_PER_CLASS; ++i) {
			if (!classInfo->classWeapons[i]) {
				cgs.ccSelectedWeapon = 0;
				break;
			}

			if ((int)classInfo->classWeapons[i] == cgs.clientinfo[cg.clientNum].latchedweapon) {
				cgs.ccSelectedWeapon = i;
				break;
			}
		}

		if (cgs.ccSelectedWeapon2 >= CG_LimboPanel_WeaponCount_ForSlot(0)) {
			cgs.ccSelectedWeapon2 = CG_LimboPanel_WeaponCount_ForSlot(0) - 1;
		}

		for (i = 0; i < 3; ++i) {
			if (teamOrder[i] == ci->team) {
				cgs.ccSelectedTeam = i;
			}
		}

		if (ci->team != TEAM_SPECTATOR) {
			cgs.ccSelectedClass = ci->cls;
		}
	}

	cgs.ccSelectedWeaponNumber = 1;

	if (CG_LimboPanel_WeaponIsDisabled(cgs.ccSelectedWeapon)) {
		// set weapon to default if disabled
		// NOTE classWeapons[0] must NEVER be disabled
		cgs.ccSelectedWeapon = 0;
	}
}

void CG_LimboPanel_Init(void) {
	BG_PanelButtonsSetup(limboPanelButtons);
	// suburb, widescreen support
	BG_PanelButtonsSetupWide(limboPanelButtons, 2*cgs.wideXoffset);
}

qboolean CG_LimboPanel_Draw(void) {
	static panel_button_t *lastHighlight;
	panel_button_t        *hilight;

	hilight = BG_PanelButtonsGetHighlightButton(limboPanelButtons);
	if (hilight && hilight != lastHighlight) {
		lastHighlight = hilight;
	}

	BG_PanelButtonsRender(limboPanelButtons);

	trap_R_SetColor(NULL);
	CG_DrawPic(cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon);
	return qtrue;
}

void CG_LimboPanel_KeyHandling(int key, qboolean down) {
	int b1, b2;

	if (BG_PanelButtonsKeyEvent(key, down, limboPanelButtons)) {
		return;
	}

	if (down) {
		cgDC.getKeysForBinding("openlimbomenu", &b1, &b2);
		if ((b1 != -1 && b1 == key) || (b2 != -1 && b2 == key)) {
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
			return;
		}
	}
}

void CG_LimboPanel_GetWeaponCardIconData(weapon_t weap, qhandle_t *shader, float *w, float *h, float *s0, float *t0, float *s1, float *t1) {
	// setup the shader
	switch (weap) {
	case WP_MORTAR:
	case WP_PANZERFAUST:
	case WP_FLAMETHROWER:
	case WP_FG42:
	case WP_MOBILE_MG42:
	case WP_MP40:
	case WP_STEN:
	case WP_THOMPSON:
		*shader = cgs.media.limboWeaponCard1;
		break;

	case WP_COLT:
	case WP_LUGER:
	case WP_AKIMBO_COLT:
	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_SILENCEDCOLT:
	case WP_AKIMBO_SILENCEDLUGER:
	case WP_SILENCED_COLT:
	case WP_SILENCER:
	case WP_CARBINE:
	case WP_GARAND:
	case WP_KAR98:
	case WP_K43:
		*shader = cgs.media.limboWeaponCard2;
		break;

	default:     // shouldn't happen
		*shader = 0;
	}

	// setup s co-ords
	switch (weap) {
	case WP_SILENCED_COLT:
	case WP_SILENCER:
	case WP_LUGER:
	case WP_COLT:
		*s0 = 0;
		*s1 = 0.5f;
		break;
	default:
		*s0 = 0;
		*s1 = 1;
		break;
	}

	// setup t co-ords
	switch (weap) {
	case WP_AKIMBO_SILENCEDLUGER:
	case WP_SILENCER:
	case WP_MORTAR:
		*t0 = 0 / 8.f;
		*t1 = 1 / 8.f;
		break;
	case WP_AKIMBO_SILENCEDCOLT:
	case WP_SILENCED_COLT:
	case WP_PANZERFAUST:
		*t0 = 1 / 8.f;
		*t1 = 2 / 8.f;
		break;
	case WP_LUGER:
	case WP_AKIMBO_LUGER:
	case WP_FLAMETHROWER:
		*t0 = 2 / 8.f;
		*t1 = 3 / 8.f;
		break;
	case WP_AKIMBO_COLT:
	case WP_COLT:
	case WP_FG42:
		*t0 = 3 / 8.f;
		*t1 = 4 / 8.f;
		break;
	case WP_CARBINE:
	case WP_MOBILE_MG42:
		*t0 = 4 / 8.f;
		*t1 = 5 / 8.f;
		break;
	case WP_KAR98:
	case WP_MP40:
		*t0 = 5 / 8.f;
		*t1 = 6 / 8.f;
		break;
	case WP_K43:
	case WP_STEN:
		*t0 = 6 / 8.f;
		*t1 = 7 / 8.f;
		break;
	case WP_GARAND:
	case WP_THOMPSON:
		*t0 = 7 / 8.f;
		*t1 = 8 / 8.f;
		break;
	default:     // shouldn't happen
		*t0 = 0.0;
		*t1 = 1.0;
		break;
	}

	*h = 1.f;
	switch (weap) {
	case WP_SILENCED_COLT:
	case WP_SILENCER:
	case WP_COLT:
	case WP_LUGER:
		*w = 0.5f;
		break;
	default:
		*w = 1.f;
		break;
	}
}

// Gordon: Utility funcs
team_t CG_LimboPanel_GetTeam(void) {
	return teamOrder[cgs.ccSelectedTeam];
}

team_t CG_LimboPanel_GetRealTeam(void) {
	return cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR ? CG_LimboPanel_GetTeam() : cgs.clientinfo[cg.clientNum].team;
}

int CG_LimboPanel_GetClass(void) {
	return cgs.ccSelectedClass;
}

bg_playerclass_t *CG_LimboPanel_GetPlayerClass(void) {
	return BG_GetPlayerClassInfo(CG_LimboPanel_GetTeam(), CG_LimboPanel_GetClass());
}

int CG_LimboPanel_WeaponCount(void) {
	return CG_LimboPanel_WeaponCount_ForSlot(cgs.ccSelectedWeaponNumber);
}

int CG_LimboPanel_WeaponCount_ForSlot(int number) {
	if (number == 1) {
		bg_playerclass_t *classInfo = CG_LimboPanel_GetPlayerClass();
		int              cnt = 0, i;

		for (i = 0; i < MAX_WEAPS_PER_CLASS; ++i) {
			if (!classInfo->classWeapons[i]) {
				break;
			}

			cnt++;
		}
		return cnt;
	}
	return 1;
}

weapon_t CG_LimboPanel_GetWeaponForNumber(int number, int slot, qboolean ignoreDisabled) {
	bg_playerclass_t *classInfo;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		return WP_NONE;
	}

	classInfo = CG_LimboPanel_GetPlayerClass();
	if (!classInfo) {
		return WP_NONE;
	}

	if (slot == 1) {
		if (!ignoreDisabled && CG_LimboPanel_WeaponIsDisabled(number)) {
			if (!number) {
				CG_Error("ERROR: Class weapon 0 disabled\n");
				return WP_NONE;
			}
			return classInfo->classWeapons[0];
		}

		return classInfo->classWeapons[number];
	}
	if (number == 0) {
		if (CG_LimboPanel_GetClass() == PC_COVERTOPS) {
			return CG_LimboPanel_GetTeam() == TEAM_AXIS ? WP_SILENCER : WP_SILENCED_COLT;
		}
		return CG_LimboPanel_GetTeam() == TEAM_AXIS ? WP_LUGER : WP_COLT;
	}

	return 0;
}

weapon_t CG_LimboPanel_GetSelectedWeaponForSlot(int index) {
	return CG_LimboPanel_GetWeaponForNumber(index == 1 ? cgs.ccSelectedWeapon : cgs.ccSelectedWeapon2, index, qfalse);
}

void CG_LimboPanel_SetSelectedWeaponNumForSlot(int index, int number) {
	if (index == 0) {
		cgs.ccSelectedWeapon = number;
	} else {
		cgs.ccSelectedWeapon2 = number;
	}
}

weapon_t CG_LimboPanel_GetSelectedWeapon(void) {
	return CG_LimboPanel_GetWeaponForNumber(CG_LimboPanel_GetSelectedWeaponNum(), cgs.ccSelectedWeaponNumber, qfalse);
}

int CG_LimboPanel_GetSelectedWeaponNum(void) {
	if (!cgs.ccSelectedWeaponNumber) {
		return cgs.ccSelectedWeapon2;
	}

	if (CG_LimboPanel_WeaponIsDisabled(cgs.ccSelectedWeapon)) {
		CG_LimboPanel_SetSelectedWeaponNumForSlot(0, 0);
	}

	return cgs.ccSelectedWeapon;
}

void CG_LimboPanel_SetSelectedWeaponNum(int number) {
	if (cgs.ccSelectedWeaponNumber == 1) {
		if (!CG_LimboPanel_WeaponIsDisabled(number)) {
			cgs.ccSelectedWeapon = number;
		}
	} else {
		cgs.ccSelectedWeapon2 = number;
	}
}

int CG_LimboPanel_TeamCount(weapon_t weap) {
	int i, cnt;

	if ((int)weap == -1) {   // we aint checking for a weapon, so always include ourselves
		cnt = 1;
	} else {   // we ARE checking for a weapon, so ignore ourselves
		cnt = 0;
	}

	for (i = 0; i < MAX_CLIENTS; ++i) {
		if (i == cg.clientNum) {
			continue;
		}

		if (!cgs.clientinfo[i].infoValid) {
			continue;
		}

		if (cgs.clientinfo[i].team != CG_LimboPanel_GetTeam()) {
			continue;
		}

		if ((int)weap != -1 && cgs.clientinfo[i].weapon != (int)weap && cgs.clientinfo[i].latchedweapon != (int)weap) {
			continue;
		}

		cnt++;
	}

	return cnt;
}

qboolean CG_IsHeavyWeapon(weapon_t weap) {
	int i;

	for (i = 0; i < NUM_HEAVY_WEAPONS; ++i) {
		if (bg_heavyWeapons[i] == weap) {
			return qtrue;
		}
	}

	return qfalse;
}

qboolean CG_LimboPanel_WeaponIsDisabled(int index) {
	bg_playerclass_t *classinfo;
	int              count, wcount;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		return qtrue;
	}

	classinfo = CG_LimboPanel_GetPlayerClass();

	if (!CG_IsHeavyWeapon(classinfo->classWeapons[index])) {
		return qfalse;
	}

	count  = CG_LimboPanel_TeamCount(-1);
	wcount = CG_LimboPanel_TeamCount(classinfo->classWeapons[index]);

	if (wcount >= ceil(count * cgs.weaponRestrictions)) {
		return qtrue;
	}

	return qfalse;
}

qboolean CG_LimboPanel_RealWeaponIsDisabled(weapon_t weap) {
	int count, wcount;

	if (CG_LimboPanel_GetTeam() == TEAM_SPECTATOR) {
		return qtrue;
	}

	if (!CG_IsHeavyWeapon(weap)) {
		return qfalse;
	}

	count  = CG_LimboPanel_TeamCount(-1);
	wcount = CG_LimboPanel_TeamCount(weap);

	if (wcount >= ceil(count * cgs.weaponRestrictions)) {
		return qtrue;
	}

	return qfalse;
}
