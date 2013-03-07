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
#include "../ui/ui_shared.h"

extern displayContextDef_t *DC;

qboolean   bg_loadscreeninited = qfalse;
fontInfo_t bg_loadscreenfont1;
fontInfo_t bg_loadscreenfont2;
qhandle_t  bg_neutralpin;
qhandle_t  bg_pin;
qhandle_t bg_filter_al;
qhandle_t bg_mappic;

panel_button_text_t missiondescriptionTxt =
{
	0.2f,                0.2f,
	{ 0.0f,              0.0f,0.0f,    1.f },
	0,                   0,
	&bg_loadscreenfont2,
};

panel_button_text_t missiondescriptionHeaderTxt =
{
	0.2f,                0.2f,
	{ 0.0f,              0.0f,             0.0f,    0.8f },
	0,                   ITEM_ALIGN_CENTER,
	&bg_loadscreenfont2,
};

panel_button_text_t loadScreenMeterBackTxt =
{
	0.22f,               0.22f,
	{ 0.1f,              0.1f,             0.1f,  0.8f },
	0,                   ITEM_ALIGN_CENTER,
	&bg_loadscreenfont2,
};

panel_button_t loadScreenMap =
{
	"gfx/loading/camp_map",
	NULL,
	{ 0,                      0,  440, 480 }, // shouldn't this be square?? // Gordon: no, the map is actually WIDER that tall, which makes it even worse...
	{ 0,                      0,  0,   0, 0, 0, 0, 0},
	NULL,                     /* font		*/
	NULL,                     /* keyDown	*/
	NULL,                     /* keyUp	*/
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

panel_button_t loadScreenBack =
{
	"gfx/loading/camp_side",
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

panel_button_t loadScreenPins =
{
	NULL,
	NULL,
	{ 0,                            0,  640, 480 },
	{ 0,                            0,  0,   0, 0, 0, 0, 0},
	NULL,                           /* font		*/
	NULL,                           /* keyDown	*/
	NULL,                           /* keyUp	*/
	CG_LoadPanel_RenderCampaignPins,
	NULL,
	0
};

panel_button_t missiondescriptionPanelHeaderText =
{
	NULL,
	"***TOP SECRET***",
	{ 440,                     72, 200, 32 },
	{ 0,                       0,  0,   0, 0, 0, 0, 0},
	&missiondescriptionHeaderTxt, /* font		*/
	NULL,                      /* keyDown	*/
	NULL,                      /* keyUp	*/
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

panel_button_t missiondescriptionPanelText =
{
	NULL,
	NULL,
	{ 460,                                    84,   160, 232 },
	{ 0,                                      0,    0,   0, 0, 0, 0, 0},
	&missiondescriptionTxt,                   /* font		*/
	NULL,                                     /* keyDown	*/
	NULL,                                     /* keyUp	*/
	CG_LoadPanel_RenderMissionDescriptionText,
	NULL,
	0
};

panel_button_t loadScreenMeterBack =
{
	"gfx/loading/progressbar_back",
	NULL,
	{ 440 + 26,                    480 - 30 + 1,200 - 56, 20 },
	{ 0,                           0,  0,        0, 0, 0, 0, 0},
	NULL,                          /* font		*/
	NULL,                          /* keyDown	*/
	NULL,                          /* keyUp	*/
	BG_PanelButtonsRender_Img,
	NULL,
	0
};

panel_button_t loadScreenMeterBack2 =
{
	"gfx/loading/progressbar",
	NULL,
	{ 440 + 26,                   480 - 30 + 1,200 - 56, 20 },
	{ 1,                          255,  0,        0, 255, 0, 0, 0},
	NULL,                         /* font		*/
	NULL,                         /* keyDown	*/
	NULL,                         /* keyUp	*/
	CG_LoadPanel_RenderLoadingBar,
	NULL,
	0
};

panel_button_t loadScreenMeterBackText =
{
	NULL,
	"LOADING",
	{ 440 + 28,                480 - 28 + 12 + 1,   200 - 56 - 2, 16 },
	{ 0,                       0,                   0,            0, 0, 0, 0, 0},
	&loadScreenMeterBackTxt,   /* font		*/
	NULL,                      /* keyDown	*/
	NULL,                      /* keyUp	*/
	BG_PanelButtonsRender_Text,
	NULL,
	0
};

panel_button_t *loadpanelButtons[] =
{
	&loadScreenMap,               &loadScreenBack,
	&missiondescriptionPanelText, &missiondescriptionPanelHeaderText,
	&loadScreenMeterBack,         &loadScreenMeterBack2,             &loadScreenMeterBackText,
	&loadScreenPins,
	NULL,
};

void CG_DrawConnectScreen(qboolean interactive, qboolean forcerefresh) {
	static qboolean inside = qfalse;
	char            buffer[1024];

	if (!DC) {
		return;
	}

	if (inside) {
		return;
	}

	inside = qtrue;

	if (!bg_loadscreeninited) {
		trap_Cvar_Set("ui_connecting", "0");

		DC->registerFont("ariblk", 27, &bg_loadscreenfont1);
		DC->registerFont("courbd", 30, &bg_loadscreenfont2);

		bg_neutralpin = DC->registerShaderNoMip("gfx/loading/pin_neutral");
		bg_pin        = DC->registerShaderNoMip("gfx/loading/pin_shot");
		bg_filter_al  = DC->registerShaderNoMip("ui/assets/filter_antilag");

		bg_mappic = 0;

		BG_PanelButtonsSetup(loadpanelButtons);

		bg_loadscreeninited = qtrue;
	}

	BG_PanelButtonsRender(loadpanelButtons);

	if (interactive) {
		DC->drawHandlePic(DC->cursorx, DC->cursory, 32, 32, DC->Assets.cursor);
	}

	DC->getConfigString(CS_SERVERINFO, buffer, sizeof (buffer));
	if (*buffer) {
		const char *str;
		float      x, y;
		int        i;
		vec4_t     clr3 = { 1.f, 1.f, 1.f, .6f };

		y = 322;
		CG_Text_Paint_Centred_Ext(540, y, 0.22f, 0.22f, clr3, GAME_VERSION_COLORED" ^Z"MOD_VERSION, 0, 0, 0, &bg_loadscreenfont1);

		y   = 340;
		str = Info_ValueForKey(buffer, "sv_hostname");
		CG_Text_Paint_Centred_Ext(540, y, 0.2f, 0.2f, colorWhite, str && *str ? str : "ETHost", 0, 26, 0, &bg_loadscreenfont2);


		y += 14;
		for (i = 0; i < MAX_MOTDLINES; i++) {
			str = CG_ConfigString(CS_CUSTMOTD + i);
			if (!str || !*str) {
				break;
			}

			CG_Text_Paint_Centred_Ext(540, y, 0.2f, 0.2f, colorWhite, str, 0, 26, 0, &bg_loadscreenfont2);

			y += 10;
		}

		y = 417;

		str = Info_ValueForKey(buffer, "g_antilag");
		if (str && *str && atoi(str)) {
			x = 575;
			CG_DrawPic(x, y, 16, 16, bg_filter_al);
		}
	}

	if (*cgs.rawmapname) {
		if (!bg_mappic) {
			bg_mappic = DC->registerShaderNoMip(va("levelshots/%s", cgs.rawmapname));

			if (!bg_mappic) {
				bg_mappic = DC->registerShaderNoMip("levelshots/unknownmap");
			}
		}

		trap_R_SetColor(colorBlack);
		CG_DrawPic(16 + 1, 2 + 1, 192, 144, bg_mappic);

		trap_R_SetColor(NULL);
		CG_DrawPic(16, 2, 192, 144, bg_mappic);

		CG_DrawPic(16 + 80, 2 + 6, 20, 20, bg_pin);
	}

	if (forcerefresh) {
		DC->updateScreen();
	}

	inside = qfalse;
}

void CG_LoadPanel_RenderLoadingBar(panel_button_t *button) {
	int   hunkused, hunkexpected;
	float frac;

	trap_GetHunkData(&hunkused, &hunkexpected);

	if (hunkexpected <= 0) {
		return;
	}

	frac = hunkused / (float)hunkexpected;
	if (frac < 0.f) {
		frac = 0.f;
	}
	if (frac > 1.f) {
		frac = 1.f;
	}

	CG_DrawPicST(button->rect.x, button->rect.y, button->rect.w * frac, button->rect.h, 0, 0, frac, 1, button->hShaderNormal);
}

void CG_LoadPanel_RenderMissionDescriptionText(panel_button_t *button) {
	const char *cs;
	char       *s, *p;
	char       buffer[1024];
	float      y;

	if (!cgs.arenaInfoLoaded) {
		return;
	}

	cs = cgs.arenaData.description;

	Q_strncpyz(buffer, cs, sizeof (buffer));
	s = strchr(buffer, '*');
	while (s) {
		*s = '\n';
		s  = strchr(buffer, '*');
	}

	BG_FitTextToWidth_Ext(buffer, button->font->scalex, button->rect.w - 16, sizeof (buffer), button->font->font);

	y = button->rect.y + 12;

	s = p = buffer;
	while (*p) {
		if (*p == '\n') {
			*p++ = '\0';
			DC->drawTextExt(button->rect.x + 4, y, button->font->scalex, button->font->scaley, button->font->colour, s, 0, 0, 0, button->font->font);
			y += 8;
			s  = p;
		} else {
			p++;
		}
	}
}

void CG_LoadPanel_KeyHandling(int key, qboolean down) {
	BG_PanelButtonsKeyEvent(key, down, loadpanelButtons);
}

qboolean CG_LoadPanel_ContinueButtonKeyDown(panel_button_t *button, int key) {
	// Nico, silent GCC
	button = button;

	if (key == K_MOUSE1) {
		CG_EventHandling(CGAME_EVENT_GAMEVIEW, qfalse);
		return qtrue;
	}

	return qfalse;
}

void CG_LoadPanel_DrawPin(const char *text, float px, float py, float sx, float sy, qhandle_t shader, float pinsize, float backheight) {
	float  x, y, w, h;
	vec4_t colourFadedBlack = { 0.f, 0.f, 0.f, 0.4f };

	w = DC->textWidthExt(text, sx, 0, &bg_loadscreenfont2);
	if (px + 30 + w > 440) {
		DC->fillRect(px - w - 28 + 2, py - (backheight / 2.f) + 2, 28 + w, backheight, colourFadedBlack);
		DC->fillRect(px - w - 28, py - (backheight / 2.f), 28 + w, backheight, colorBlack);
	} else {
		DC->fillRect(px + 2, py - (backheight / 2.f) + 2, 28 + w, backheight, colourFadedBlack);
		DC->fillRect(px, py - (backheight / 2.f), 28 + w, backheight, colorBlack);
	}

	x = px - pinsize;
	y = py - pinsize;
	w = pinsize * 2.f;
	h = pinsize * 2.f;

	DC->drawHandlePic(x, y, w, h, shader);

	if (px + 30 + w > 440) {
		DC->drawTextExt(px - 12 - w - 28, py + 4, sx, sy, colorWhite, text, 0, 0, 0, &bg_loadscreenfont2);
	} else {
		DC->drawTextExt(px + 16, py + 4, sx, sy, colorWhite, text, 0, 0, 0, &bg_loadscreenfont2);
	}
}

void CG_LoadPanel_RenderCampaignPins(panel_button_t *button) {
	float px, py;

	// Nico, silent GCC
	button = button;

	if (!cgs.arenaInfoLoaded) {
		return;
	}

	px = (cgs.arenaData.mappos[0] / 1024.f) * 440.f;
	py = (cgs.arenaData.mappos[1] / 1024.f) * 480.f;

	CG_LoadPanel_DrawPin(cgs.arenaData.longname, px, py, 0.22f, 0.25f, bg_neutralpin, 16.f, 16.f);
}
