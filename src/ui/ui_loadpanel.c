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

#include "ui_local.h"
#include "ui_shared.h"

qboolean   bg_loadscreeninited = qfalse;
fontInfo_t bg_loadscreenfont1;
fontInfo_t bg_loadscreenfont2;

void UI_LoadPanel_RenderHeaderText(panel_button_t *button);
void UI_LoadPanel_RenderLoadingText(panel_button_t *button);

panel_button_text_t missiondescriptionTxt =
{
	0.2f,                0.2f,
	{ 0.0f,              0.0f,0.0f,    1.f },
	0,                   0,
	&bg_loadscreenfont2,
};

panel_button_text_t campaignpTxt =
{
	0.35f,               0.35f,
	{ 1.0f,              1.0f, 1.0f,  0.6f },
	0,                   0,
	&bg_loadscreenfont2,
};

panel_button_t loadScreenMap =
{
	"gfx/loading/camp_map",
	NULL,
	{ 0,                      0,  440, 480 }, // shouldn't this be square??
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

panel_button_t loadingPanelText =
{
	NULL,
	NULL,
	{ 460,                         72,   160, 244 },
	{ 0,                           0,    0,   0, 0, 0, 0, 0},
	&missiondescriptionTxt,        /* font		*/
	NULL,                          /* keyDown	*/
	NULL,                          /* keyUp	*/
	UI_LoadPanel_RenderLoadingText,
	NULL,
	0
};

panel_button_t campaignPanelText =
{
	NULL,
	NULL,                         //"CONNECTING...",
	{ 470,                        33,   152, 232 },
	{ 0,                          0,    0,   0, 0, 0, 0, 0},
	&campaignpTxt,                /* font		*/
	NULL,                         /* keyDown	*/
	NULL,                         /* keyUp	*/
	UI_LoadPanel_RenderHeaderText,
	NULL,
	0
};

panel_button_t *loadpanelButtons[] =
{
	&loadScreenMap,    &loadScreenBack,
	&loadingPanelText, &campaignPanelText,
	NULL,
};

/*
================
CG_DrawConnectScreen
================
*/
static qboolean connect_ownerdraw;
void UI_DrawLoadPanel(qboolean ownerdraw, qboolean uihack) {
	static qboolean inside = qfalse;

	// suburb, widescreen support
	// to avoid a flickering screen on widescreens, we erase it before drawing onto it
	if (((float)(uiInfo.uiDC.glconfig.vidWidth) / (float)(uiInfo.uiDC.glconfig.vidHeight)) != RATIO43)
	{
		float xoffset = UI_WideXoffset() * uiInfo.uiDC.xscale;
		
		trap_R_DrawStretchPic(0, 0, xoffset, uiInfo.uiDC.glconfig.vidHeight, 0, 0, 1, 1, uiInfo.uiDC.registerShaderNoMip("gfx/2d/backtile"));
		trap_R_DrawStretchPic(uiInfo.uiDC.glconfig.vidWidth - xoffset, 0, xoffset, uiInfo.uiDC.glconfig.vidHeight, 0, 0, 1, 1, uiInfo.uiDC.registerShaderNoMip("gfx/2d/backtile"));
	}

	if (inside) {
		if (!uihack && trap_Cvar_VariableValue("ui_connecting")) {
			trap_Cvar_Set("ui_connecting", "0");
		}
		return;
	}

	connect_ownerdraw = ownerdraw;

	inside = qtrue;

	if (!bg_loadscreeninited) {
		trap_R_RegisterFont("ariblk", 27, &bg_loadscreenfont1);
		trap_R_RegisterFont("courbd", 30, &bg_loadscreenfont2);

		BG_PanelButtonsSetup(loadpanelButtons);
		// suburb, widescreen support
		BG_PanelButtonsSetupWide(loadpanelButtons, UI_WideXoffset());

		bg_loadscreeninited = qtrue;
	}

	BG_PanelButtonsRender(loadpanelButtons);

	if (!uihack && trap_Cvar_VariableValue("ui_connecting")) {
		trap_Cvar_Set("ui_connecting", "0");
	}

	inside = qfalse;
}

void UI_LoadPanel_RenderHeaderText(panel_button_t *button) {
	uiClientState_t cstate;
	char            downloadName[MAX_INFO_VALUE];

	trap_GetClientState(&cstate);

	trap_Cvar_VariableStringBuffer("cl_downloadName", downloadName, sizeof (downloadName));

	if ((cstate.connState == CA_DISCONNECTED || cstate.connState == CA_CONNECTED) && *downloadName) {
		button->text = "DOWNLOADING...";
	} else {
		button->text = "CONNECTING...";
	}

	BG_PanelButtonsRender_Text(button);
}

#define ESTIMATES 80
const char *UI_DownloadInfo(const char *downloadName) {
	static char dlText[] = "Downloading:";
	static char etaText[] = "Estimated time left:";
	static char xferText[] = "Transfer rate:";
	char        dlSizeBuf[64], totalSizeBuf[64], xferRateBuf[64], dlTimeBuf[64];
	int         downloadSize, downloadCount, downloadTime;
	int         xferRate;
	const char  *s, *ds;

	downloadSize  = trap_Cvar_VariableValue("cl_downloadSize");
	downloadCount = trap_Cvar_VariableValue("cl_downloadCount");
	downloadTime  = trap_Cvar_VariableValue("cl_downloadTime");

	if (downloadSize > 0) {
		ds = va("%s (%d%%)", downloadName, (int)((float)downloadCount * 100.0f / (float)downloadSize));
	} else {
		ds = downloadName;
	}

	UI_ReadableSize(dlSizeBuf, sizeof dlSizeBuf, downloadCount);
	UI_ReadableSize(totalSizeBuf, sizeof totalSizeBuf, downloadSize);

	if (downloadCount < 4096 || !downloadTime) {
		s = va("%s\n %s\n%s\n\n%s\n estimating...\n\n%s\n\n%s copied", dlText, ds, totalSizeBuf,
		       etaText,
		       xferText,
		       dlSizeBuf);
		return s;
	}
	if ((uiInfo.uiDC.realTime - downloadTime) / 1000) {
		xferRate = downloadCount / ((uiInfo.uiDC.realTime - downloadTime) / 1000);
	} else {
		xferRate = 0;
	}
	UI_ReadableSize(xferRateBuf, sizeof xferRateBuf, xferRate);

	// Extrapolate estimated completion time
	if (downloadSize && xferRate) {
		int        n = downloadSize / xferRate;                       // estimated time for entire d/l in secs
		int        timeleft = 0, i;
		static int tleEstimates[ESTIMATES] = { 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
			                                   60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
			                                   60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
			                                   60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60 };
		static int tleIndex = 0;

		// We do it in K (/1024) because we'd overflow around 4MB
		tleEstimates[tleIndex] = (n - (((downloadCount / 1024) * n) / (downloadSize / 1024)));
		tleIndex++;
		if (tleIndex >= ESTIMATES) {
			tleIndex = 0;
		}

		for (i = 0; i < ESTIMATES; ++i)
			timeleft += tleEstimates[i];

		timeleft /= ESTIMATES;

		UI_PrintTime(dlTimeBuf, sizeof dlTimeBuf, timeleft);
	} else {
		dlTimeBuf[0] = '\0';
	}

	if (xferRate) {
		s = va("%s\n %s\n%s\n\n%s\n %s\n\n%s\n %s/sec\n\n%s copied", dlText, ds, totalSizeBuf,
		       etaText, dlTimeBuf,
		       xferText, xferRateBuf,
		       dlSizeBuf);
	} else {
		if (downloadSize) {
			s = va("%s\n %s\n%s\n\n%s\n estimating...\n\n%s\n\n%s copied", dlText, ds, totalSizeBuf,
			       etaText,
			       xferText,
			       dlSizeBuf);
		} else {
			s = va("%s\n %s\n\n%s\n estimating...\n\n%s\n\n%s copied", dlText, ds,
			       etaText,
			       xferText,
			       dlSizeBuf);
		}
	}

	return s;
}

void UI_LoadPanel_RenderLoadingText(panel_button_t *button) {
	uiClientState_t cstate;
	char            downloadName[MAX_INFO_VALUE];
	char            buff[2560];
	char            *p, *s = "";
	float           y;

	trap_GetClientState(&cstate);

	Com_sprintf(buff, sizeof (buff), "Connecting to:\n %s^*\n\n%s", cstate.servername, Info_ValueForKey(cstate.updateInfoString, "motd"));

	trap_Cvar_VariableStringBuffer("cl_downloadName", downloadName, sizeof (downloadName));

	if (!connect_ownerdraw) {
		if (!trap_Cvar_VariableValue("ui_connecting")) {
			switch (cstate.connState) {
			case CA_CONNECTING:
				s = va(trap_TranslateString("Awaiting connection...%i"), cstate.connectPacketCount);
				break;
			case CA_CHALLENGING:
				s = va(trap_TranslateString("Awaiting challenge...%i"), cstate.connectPacketCount);
				break;
			case CA_DISCONNECTED:
			case CA_CONNECTED:
				if (*downloadName || cstate.connState == CA_DISCONNECTED) {
					s = (char *)UI_DownloadInfo(downloadName);
				} else {
					s = trap_TranslateString("Awaiting gamestate...");
				}
				break;
			case CA_LOADING:
			case CA_PRIMED:
			default:
				break;
			}
		} else if (trap_Cvar_VariableValue("ui_dl_running")) {
			// only toggle during a disconnected download
			s = (char *)UI_DownloadInfo(downloadName);
		}

		Q_strcat(buff, sizeof (buff), va("\n\n%s^*", s));

		if (cstate.connState < CA_CONNECTED && *cstate.messageString) {
			Q_strcat(buff, sizeof (buff), va("\n\n%s^*", cstate.messageString));
		}
	}

	BG_FitTextToWidth_Ext(buff, button->font->scalex, button->rect.w, sizeof (buff), button->font->font);

	y = button->rect.y + 12;

	s = p = buff;

	while (*p) {
		if (*p == '\n') {
			*p++ = '\0';
			Text_Paint_Ext(button->rect.x + 4, y, button->font->scalex, button->font->scaley, button->font->colour, s, 0, 0, 0, button->font->font);
			y += 8;
			s  = p;
		} else {
			p++;
		}
	}
}
