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

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

#define STATUSBARHEIGHT 452
char *BindingFromName(const char *cvar);
void Controls_GetConfig(void);
void SetHeadOrigin(clientInfo_t *ci, playerInfo_t *pi);
void CG_DrawOverlays();
int activeFont;

#define TEAM_OVERLAY_TIME 1000

////////////////////////
////////////////////////
////// new hud stuff
///////////////////////
///////////////////////

void CG_Text_SetActiveFont(int font) {
	activeFont = font;
}

int CG_Text_Width_Ext(const char *text, float scale, int limit, fontInfo_t *font) {
	int         count, len;
	glyphInfo_t *glyph;
	const char  *s = text;
	float       out, useScale = scale * font->glyphScale;

	out = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if (Q_IsColorString(s)) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(unsigned char)*s];
				out  += glyph->xSkip;
				s++;
				count++;
			}
		}
	}

	return out * useScale;
}

int CG_Text_Width(const char *text, float scale, int limit) {
	fontInfo_t *font = &cgDC.Assets.fonts[activeFont];

	return CG_Text_Width_Ext(text, scale, limit, font);
}

int CG_Text_Height_Ext(const char *text, float scale, int limit, fontInfo_t *font) {
	int         len, count;
	float       max;
	glyphInfo_t *glyph;
	float       useScale;
	const char  *s = text;

	useScale = scale * font->glyphScale;
	max      = 0;
	if (text) {
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			if (Q_IsColorString(s)) {
				s += 2;
				continue;
			} else {
				glyph = &font->glyphs[(unsigned char)*s];
				if (max < glyph->height) {
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}
	return max * useScale;
}

int CG_Text_Height(const char *text, float scale, int limit) {
	fontInfo_t *font = &cgDC.Assets.fonts[activeFont];

	return CG_Text_Height_Ext(text, scale, limit, font);
}

void CG_Text_PaintChar_Ext(float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2, qhandle_t hShader) {
	w *= scalex;
	h *= scaley;
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

void CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
	float w, h;

	w = width * scale;
	h = height * scale;
	CG_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

void CG_Text_Paint_Centred_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *font) {
	x -= CG_Text_Width_Ext(text, scalex, limit, font) * 0.5f;

	CG_Text_Paint_Ext(x, y, scalex, scaley, color, text, adjust, limit, style, font);
}

void CG_Text_Paint_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *font) {
	int         len, count;
	vec4_t      newColor;
	glyphInfo_t *glyph;

	scalex *= font->glyphScale;
	scaley *= font->glyphScale;

	if (text) {
		const char *s = text;
		trap_R_SetColor(color);
		memcpy(&newColor[0], &color[0], sizeof (vec4_t));
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyph = &font->glyphs[(unsigned char)*s]; //-V595
			if (Q_IsColorString(s)) {
				if (*(s + 1) == COLOR_NULL) {
					memcpy(newColor, color, sizeof (newColor));
				} else {
					memcpy(newColor, g_color_table[ColorIndex(*(s + 1))], sizeof (newColor));
					newColor[3] = color[3];
				}
				trap_R_SetColor(newColor);
				s += 2;
				continue;
			} else {
				float yadj = scaley * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor(colorBlack);
					CG_Text_PaintChar_Ext(x + (glyph->pitch * scalex) + ofs, y - yadj + ofs, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor(newColor);
				}
				CG_Text_PaintChar_Ext(x + (glyph->pitch * scalex), y - yadj, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
				x += (glyph->xSkip * scalex) + adjust;
				s++;
				count++;
			}
		}
		trap_R_SetColor(NULL);
	}
}

void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
	fontInfo_t *font = &cgDC.Assets.fonts[activeFont];

	CG_Text_Paint_Ext(x, y, scale, scale, color, text, adjust, limit, style, font);
}

/*
================
CG_Draw3DModel

================
*/
void CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles) {
	refdef_t    refdef;
	refEntity_t ent;

	CG_AdjustFrom640(&x, &y, &w, &h);

	memset(&refdef, 0, sizeof (refdef));

	memset(&ent, 0, sizeof (ent));
	AnglesToAxis(angles, ent.axis);
	VectorCopy(origin, ent.origin);
	ent.hModel     = model;
	ent.customSkin = skin;
	ent.renderfx   = RF_NOSHADOW;   // no stencil shadows

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear(refdef.viewaxis);

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x      = x;
	refdef.y      = y;
	refdef.width  = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene(&ent);
	trap_R_RenderScene(&refdef);
}

/*
===========================================================================================

  UPPER RIGHT CORNER

===========================================================================================
*/

#define UPPERRIGHT_X 634
/*
==================
CG_DrawSnapshot
==================
*/
static float CG_DrawSnapshot(float y) {
	char *s;
	int  w;

	s = va("time:%i snap:%i cmd:%i", cg.snap->serverTime,
	       cg.latestSnapshotNum, cgs.serverCommandSequence);
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;

	CG_DrawBigString(UPPERRIGHT_X - w, y + 2, s, 1.0F);

	return y + BIGCHAR_HEIGHT + 4;
}

/*
==================
CG_DrawFPS
==================
*/
#define FPS_FRAMES  4
static float CG_DrawFPS(float y) {
	char       *s;
	int        w;
	static int previousTimes[FPS_FRAMES];
	static int index;
	int        i, total;
	int        fps;
	static int previous;
	int        t, frameTime;
	vec4_t     timerBackground = { 0.16f, 0.2f, 0.17f, 0.8f };
	vec4_t     timerBorder     = { 0.5f, 0.5f, 0.5f, 0.5f };
	vec4_t     tclr            = { 0.625f, 0.625f, 0.6f, 1.0f };

	// don't use serverTime, because that will be drifting to
	// correct for internet lag changes, timescales, timedemos, etc
	t         = trap_Milliseconds();
	frameTime = t - previous;
	previous  = t;

	previousTimes[index % FPS_FRAMES] = frameTime;
	index++;
	if (index > FPS_FRAMES) {
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for (i = 0 ; i < FPS_FRAMES ; i++) {
			total += previousTimes[i];
		}
		if (!total) {
			total = 1;
		}
		fps = 1000 * FPS_FRAMES / total;

		s = va("%i FPS", fps);
		w = CG_Text_Width_Ext(s, 0.19f, 0, &cgs.media.limboFont1);

		CG_FillRect(UPPERRIGHT_X - w - 2, y, w + 5, 12 + 2, timerBackground);
		CG_DrawRect_FixedBorder(UPPERRIGHT_X - w - 2, y, w + 5, 12 + 2, 1, timerBorder);

		CG_Text_Paint_Ext(UPPERRIGHT_X - w, y + 11, 0.19f, 0.19f, tclr, s, 0, 0, 0, &cgs.media.limboFont1);
	}

	return y + 12 + 4;
}

// Nico, #fixme, is this global needed?
int maxCharsBeforeOverlay;

/*
=====================
CG_DrawUpperRight

=====================
*/
static void CG_DrawUpperRight(void) {
	float y = 20 + 100 + 32;

	// Nico, bugfix: round timer and respawn timer gone when cg_drawFireteamOverlay
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=008
	if (cg_drawFireteamOverlay.integer) {
		if (CG_IsOnFireteam(cg.clientNum)) {
			rectDef_t rect = { 10, 10, 100, 100 };
			CG_DrawFireTeamOverlay(&rect);
		}
	}

	if (cg_drawFPS.integer) {
		y = CG_DrawFPS(y);
	}

	if (cg_drawSnapshot.integer) {
		y = CG_DrawSnapshot(y);
	}
}

/*
===========================================================================================

  LOWER RIGHT CORNER

===========================================================================================
*/

#define CHATLOC_X 160
#define CHATLOC_Y 478
#define CHATLOC_TEXT_X (CHATLOC_X + 0.25f * TINYCHAR_WIDTH)

/*
=================
CG_DrawTeamInfo
=================
*/
static void CG_DrawTeamInfo(void) {
	int    w;
	int    i, len;
	vec4_t hcolor;
	int    chatHeight;
	float  alphapercent;
	float  lineHeight = 9.f;

	int chatWidth = 640 - CHATLOC_X - 100;

	if (cg_teamChatHeight.integer < TEAMCHAT_HEIGHT) {
		chatHeight = cg_teamChatHeight.integer;
	} else {
		chatHeight = TEAMCHAT_HEIGHT;
	}

	if (chatHeight <= 0) {
		return; // disabled
	}

	if (cgs.teamLastChatPos != cgs.teamChatPos) {
		if (cg.time - cgs.teamChatMsgTimes[cgs.teamLastChatPos % chatHeight] > cg_teamChatTime.integer) {
			cgs.teamLastChatPos++;
		}

		w = 0;

		for (i = cgs.teamLastChatPos; i < cgs.teamChatPos; i++) {
			len = CG_Text_Width_Ext(cgs.teamChatMsgs[i % chatHeight], 0.2f, 0, &cgs.media.limboFont2);
			if (len > w) {
				w = len;
			}
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		for (i = cgs.teamChatPos - 1; i >= cgs.teamLastChatPos; i--) {
			alphapercent = 1.0f - (cg.time - cgs.teamChatMsgTimes[i % chatHeight]) / (float)(cg_teamChatTime.integer);
			if (alphapercent > 1.0f) {
				alphapercent = 1.0f;
			} else if (alphapercent < 0.f) {
				alphapercent = 0.f;
			}

			if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_AXIS) {
				hcolor[0] = 1;
				hcolor[1] = 0;
				hcolor[2] = 0;
			} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES) {
				hcolor[0] = 0;
				hcolor[1] = 0;
				hcolor[2] = 1;
			} else {
				hcolor[0] = 0;
				hcolor[1] = 1;
				hcolor[2] = 0;
			}

			hcolor[3] = 0.33f * alphapercent;

			trap_R_SetColor(hcolor);
			CG_DrawPic(CHATLOC_X, CHATLOC_Y - (cgs.teamChatPos - i) * lineHeight, chatWidth, lineHeight, cgs.media.teamStatusBar);

			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = alphapercent;
			trap_R_SetColor(hcolor);

			CG_Text_Paint_Ext(CHATLOC_TEXT_X, CHATLOC_Y - (cgs.teamChatPos - i - 1) * lineHeight - 1, 0.2f, 0.2f, hcolor, cgs.teamChatMsgs[i % chatHeight], 0, 0, 0, &cgs.media.limboFont2);
		}
	}
}

const char *CG_PickupItemText(int item) {
	if (bg_itemlist[item].giType == IT_HEALTH) {
		if (bg_itemlist[item].world_model[2]) {        // this is a multi-stage item
			// FIXME: print the correct amount for multi-stage
			return va("a %s", bg_itemlist[item].pickup_name);
		} else {
			return va("%i %s", bg_itemlist[item].quantity, bg_itemlist[item].pickup_name);
		}
	} else if (bg_itemlist[item].giType == IT_TEAM) {
		return "an Objective";
	} else {
		if (bg_itemlist[item].pickup_name[0] == 'a' ||  bg_itemlist[item].pickup_name[0] == 'A') {
			return va("an %s", bg_itemlist[item].pickup_name);
		} else {
			return va("a %s", bg_itemlist[item].pickup_name);
		}
	}
}

/*
=================
CG_DrawNotify
=================
*/
#define NOTIFYLOC_Y 42 // bottom end
#define NOTIFYLOC_X 0
#define NOTIFYLOC_Y_SP 128

static void CG_DrawNotify(void) {
	int    w;
	int    i, len;
	vec4_t hcolor;
	int    chatHeight;
	float  alphapercent;
	char   var[MAX_TOKEN_CHARS];
	float  notifytime = 1.0f;
	int    yLoc;

	return;

	yLoc = NOTIFYLOC_Y;

	trap_Cvar_VariableStringBuffer("con_notifytime", var, sizeof (var));
	notifytime = atof(var) * 1000;

	if (notifytime <= 100.f) {
		notifytime = 100.0f;
	}

	chatHeight = NOTIFY_HEIGHT;

	if (cgs.notifyLastPos != cgs.notifyPos) {
		if (cg.time - cgs.notifyMsgTimes[cgs.notifyLastPos % chatHeight] > notifytime) {
			cgs.notifyLastPos++;
		}

		w = 0;

		for (i = cgs.notifyLastPos; i < cgs.notifyPos; i++) {
			len = CG_DrawStrlen(cgs.notifyMsgs[i % chatHeight]);
			if (len > w) {
				w = len;
			}
		}
		w *= TINYCHAR_WIDTH;
		w += TINYCHAR_WIDTH * 2;

		if (maxCharsBeforeOverlay <= 0) {
			maxCharsBeforeOverlay = 80;
		}

		for (i = cgs.notifyPos - 1; i >= cgs.notifyLastPos; i--) {
			alphapercent = 1.0f - ((cg.time - cgs.notifyMsgTimes[i % chatHeight]) / notifytime);
			if (alphapercent > 0.5f) {
				alphapercent = 1.0f;
			} else {
				alphapercent *= 2;
			}

			if (alphapercent < 0.f) {
				alphapercent = 0.f;
			}

			hcolor[0] = hcolor[1] = hcolor[2] = 1.0;
			hcolor[3] = alphapercent;
			trap_R_SetColor(hcolor);

			CG_DrawStringExt(NOTIFYLOC_X + TINYCHAR_WIDTH,
			                 yLoc - (cgs.notifyPos - i) * TINYCHAR_HEIGHT,
			                 cgs.notifyMsgs[i % chatHeight], hcolor, qfalse, qfalse,
			                 TINYCHAR_WIDTH, TINYCHAR_HEIGHT, maxCharsBeforeOverlay);
		}
	}
}

/*
===============================================================================

LAGOMETER

===============================================================================
*/

#define LAG_SAMPLES     128


typedef struct {
	int frameSamples[LAG_SAMPLES];
	int frameCount;
	int snapshotFlags[LAG_SAMPLES];
	int snapshotSamples[LAG_SAMPLES];
	int snapshotCount;
} lagometer_t;

lagometer_t lagometer;

/*
==============
CG_AddLagometerFrameInfo

Adds the current interpolate / extrapolate bar for this frame
==============
*/
void CG_AddLagometerFrameInfo(void) {
	int offset;

	offset                                                           = cg.time - cg.latestSnapshotTime;
	lagometer.frameSamples[lagometer.frameCount & (LAG_SAMPLES - 1)] = offset;
	lagometer.frameCount++;
}

/*
==============
CG_AddLagometerSnapshotInfo

Each time a snapshot is received, log its ping time and
the number of snapshots that were dropped before it.

Pass NULL for a dropped packet.
==============
*/
void CG_AddLagometerSnapshotInfo(snapshot_t *snap) {
	// dropped packet
	if (!snap) {
		lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = -1;
		lagometer.snapshotCount++;
		return;
	}

	// add this snapshot's info
	lagometer.snapshotSamples[lagometer.snapshotCount & (LAG_SAMPLES - 1)] = snap->ping;
	lagometer.snapshotFlags[lagometer.snapshotCount & (LAG_SAMPLES - 1)]   = snap->snapFlags;
	lagometer.snapshotCount++;
}

/*
==============
CG_DrawDisconnect

Should we draw something differnet for long lag vs no packets?
==============
*/
static void CG_DrawDisconnect(void) {
	float      x, y;
	int        cmdNum;
	usercmd_t  cmd;
	const char *s;
	int        w;   // bk010215 - FIXME char message[1024];

	// OSP - dont draw if a demo and we're running at a different timescale
	if (cg.demoPlayback && cg_timescale.value != 1.0f) {
		return;
	}

	// ydnar: don't draw if the server is respawning
	if (cg.serverRespawning) {
		return;
	}

	// draw the phone jack if we are completely past our buffers
	cmdNum = trap_GetCurrentCmdNumber() - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &cmd);
	if (cmd.serverTime <= cg.snap->ps.commandTime
	    || cmd.serverTime > cg.time) {   // special check for map_restart // bk 0102165 - FIXME
		return;
	}

	// also add text in center of screen
	s = CG_TranslateString("Connection Interrupted");   // bk 010215 - FIXME
	w = CG_DrawStrlen(s) * BIGCHAR_WIDTH;
	CG_DrawBigString(320 - w / 2, 100, s, 1.0F);

	// blink the icon
	if ((cg.time >> 9) & 1) {
		return;
	}

	x = 640 - 48;
	y = 480 - 200;

	CG_DrawPic(x, y, 48, 48, cgs.media.disconnectIcon);
}


#define MAX_LAGOMETER_PING  900
#define MAX_LAGOMETER_RANGE 300

/*
==============
CG_DrawLagometer
==============
*/
static void CG_DrawLagometer(void) {
	int   a, x, y, i;
	float v;
	float ax, ay, aw, ah, mid, range;
	int   color;
	float vscale;

	if (!cg_lagometer.integer || cgs.localServer) {
		CG_DrawDisconnect();
		return;
	}

	//
	// draw the graph
	//
	x = 640 - 48;
	y = 480 - 200;

	trap_R_SetColor(NULL);
	CG_DrawPic(x, y, 48, 48, cgs.media.lagometerShader);

	ax = x;
	ay = y;
	aw = 48;
	ah = 48;
	CG_AdjustFrom640(&ax, &ay, &aw, &ah);

	color = -1;
	range = ah / 3;
	mid   = ay + range;

	vscale = range / MAX_LAGOMETER_RANGE;

	// draw the frame interpoalte / extrapolate graph
	for (a = 0 ; a < aw ; a++) {
		i  = (lagometer.frameCount - 1 - a) & (LAG_SAMPLES - 1);
		v  = lagometer.frameSamples[i];
		v *= vscale;
		if (v > 0) {
			if (color != 1) {
				color = 1;
				trap_R_SetColor(colorYellow);
			}
			if (v > range) {
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		} else if (v < 0) {
			if (color != 2) {
				color = 2;
				trap_R_SetColor(colorBlue);
			}
			v = -v;
			if (v > range) {
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, mid, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	// draw the snapshot latency / drop graph
	range  = ah / 2;
	vscale = range / MAX_LAGOMETER_PING;

	for (a = 0 ; a < aw ; a++) {
		i = (lagometer.snapshotCount - 1 - a) & (LAG_SAMPLES - 1);
		v = lagometer.snapshotSamples[i];
		if (v > 0) {
			if (lagometer.snapshotFlags[i] & SNAPFLAG_RATE_DELAYED) {
				if (color != 5) {
					color = 5;  // YELLOW for rate delay
					trap_R_SetColor(colorYellow);
				}
			} else {
				if (color != 3) {
					color = 3;
					trap_R_SetColor(colorGreen);
				}
			}
			v = v * vscale;
			if (v > range) {
				v = range;
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - v, 1, v, 0, 0, 0, 0, cgs.media.whiteShader);
		} else if (v < 0) {
			if (color != 4) {
				color = 4;      // RED for dropped snapshots
				trap_R_SetColor(colorRed);
			}
			trap_R_DrawStretchPic(ax + aw - a, ay + ah - range, 1, range, 0, 0, 0, 0, cgs.media.whiteShader);
		}
	}

	trap_R_SetColor(NULL);

	if (cg_nopredict.integer) {
		CG_DrawBigString(ax, ay, "snc", 1.0);
	}

	CG_DrawDisconnect();
}
/*
===============================================================================

CENTER PRINTING

===============================================================================
*/


/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
#define CP_LINEWIDTH 56         // NERVE - SMF

void CG_CenterPrint(const char *str, int y, int charWidth) {
	char     *s;
	int      i, len;                    // NERVE - SMF
	qboolean neednewline = qfalse;      // NERVE - SMF
	int      priority    = 0;

	// NERVE - SMF - don't draw if this print message is less important
	if (cg.centerPrintTime && priority < cg.centerPrintPriority) {
		return;
	}

	Q_strncpyz(cg.centerPrint, str, sizeof (cg.centerPrint));
	cg.centerPrintPriority = priority;  // NERVE - SMF

	// NERVE - SMF - turn spaces into newlines, if we've run over the linewidth
	len = strlen(cg.centerPrint);
	for (i = 0; i < len; i++) {

		// NOTE: subtract a few chars here so long words still get displayed properly
		if (i % (CP_LINEWIDTH - 20) == 0 && i > 0) {
			neednewline = qtrue;
		}
		if (cg.centerPrint[i] == ' ' && neednewline) {
			cg.centerPrint[i] = '\n';
			neednewline       = qfalse;
		}
	}
	// -NERVE - SMF

	cg.centerPrintTime      = cg.time;
	cg.centerPrintY         = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s                   = cg.centerPrint;
	while (*s) {
		if (*s == '\n') {
			cg.centerPrintLines++;
		}
		s++;
	}
}

// NERVE - SMF
/*
==============
CG_PriorityCenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_PriorityCenterPrint(const char *str, int y, int charWidth, int priority) {
	char     *s;
	int      i, len;                    // NERVE - SMF
	qboolean neednewline = qfalse;      // NERVE - SMF

	// NERVE - SMF - don't draw if this print message is less important
	if (cg.centerPrintTime && priority < cg.centerPrintPriority) {
		return;
	}

	Q_strncpyz(cg.centerPrint, str, sizeof (cg.centerPrint));
	cg.centerPrintPriority = priority;  // NERVE - SMF

	// NERVE - SMF - turn spaces into newlines, if we've run over the linewidth
	len = strlen(cg.centerPrint);
	for (i = 0; i < len; i++) {

		// NOTE: subtract a few chars here so long words still get displayed properly
		if (i % (CP_LINEWIDTH - 20) == 0 && i > 0) {
			neednewline = qtrue;
		}
		if (cg.centerPrint[i] == ' ' && neednewline) {
			cg.centerPrint[i] = '\n';
			neednewline       = qfalse;
		}
	}
	// -NERVE - SMF

	cg.centerPrintTime      = cg.time + 2000;
	cg.centerPrintY         = y;
	cg.centerPrintCharWidth = charWidth;

	// count the number of lines for centering
	cg.centerPrintLines = 1;
	s                   = cg.centerPrint;
	while (*s) {
		if (*s == '\n') {
			cg.centerPrintLines++;
		}
		s++;
	}
}
// -NERVE - SMF

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString(void) {
	char  *start;
	int   l;
	int   x, y, w;
	float *color;

	if (!cg.centerPrintTime) {
		return;
	}

	color = CG_FadeColor(cg.centerPrintTime, 1000 * cg_centertime.value);
	if (!color) {
		cg.centerPrintTime     = 0;
		cg.centerPrintPriority = 0;
		return;
	}

	trap_R_SetColor(color);

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

	while (1) {
		char linebuffer[1024];

		for (l = 0; l < CP_LINEWIDTH; l++) {            // NERVE - SMF - added CP_LINEWIDTH
			if (!start[l] || start[l] == '\n') {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.centerPrintCharWidth * CG_DrawStrlen(linebuffer);

		x = (SCREEN_WIDTH - w) / 2;

		CG_DrawStringExt(x, y, linebuffer, color, qfalse, qtrue, cg.centerPrintCharWidth, (int)(cg.centerPrintCharWidth * 1.5), 0);

		y += cg.centerPrintCharWidth * 1.5;

		while (*start && (*start != '\n')) {
			start++;
		}
		if (!*start) {
			break;
		}
		start++;
	}

	trap_R_SetColor(NULL);
}



/*
================================================================================

CROSSHAIRS

================================================================================
*/

/*
==============
CG_DrawWeapReticle
==============
*/
static void CG_DrawWeapReticle(void) {
	vec4_t   color = { 0, 0, 0, 1 };
	qboolean fg, garand, k43;

	// DHM - Nerve :: So that we will draw reticle
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback) {
		garand = (qboolean)(cg.snap->ps.weapon == WP_GARAND_SCOPE);
		k43    = (qboolean)(cg.snap->ps.weapon == WP_K43_SCOPE);
		fg     = (qboolean)(cg.snap->ps.weapon == WP_FG42SCOPE);
	} else {
		fg     = (qboolean)(cg.weaponSelect == WP_FG42SCOPE);
		garand = (qboolean)(cg.weaponSelect == WP_GARAND_SCOPE);
		k43    = (qboolean)(cg.weaponSelect == WP_K43_SCOPE);
	}

	if (fg) {
		// sides
		CG_FillRect(0, 0, 80, 480, color);
		CG_FillRect(560, 0, 80, 480, color);

		// center
		if (cgs.media.reticleShaderSimple) {
			CG_DrawPic(80, 0, 480, 480, cgs.media.reticleShaderSimple);
		}

		// hairs
		CG_FillRect(84, 239, 150, 3, color);     // left
		CG_FillRect(234, 240, 173, 1, color);    // horiz center
		CG_FillRect(407, 239, 150, 3, color);    // right


		CG_FillRect(319, 2, 3, 151, color);      // top center top
		CG_FillRect(320, 153, 1, 114, color);    // top center bot

		CG_FillRect(320, 241, 1, 87, color);     // bot center top
		CG_FillRect(319, 327, 3, 151, color);    // bot center bot
	} else if (garand) {
		// sides
		CG_FillRect(0, 0, 80, 480, color);
		CG_FillRect(560, 0, 80, 480, color);

		// center
		if (cgs.media.reticleShaderSimple) {
			CG_DrawPic(80, 0, 480, 480, cgs.media.reticleShaderSimple);
		}

		// hairs
		CG_FillRect(84, 239, 177, 2, color);     // left
		CG_FillRect(320, 242, 1, 58, color);     // center top
		CG_FillRect(319, 300, 2, 178, color);    // center bot
		CG_FillRect(380, 239, 177, 2, color);    // right
	} else if (k43) {
		// sides
		CG_FillRect(0, 0, 80, 480, color);
		CG_FillRect(560, 0, 80, 480, color);

		// center
		if (cgs.media.reticleShaderSimple) {
			CG_DrawPic(80, 0, 480, 480, cgs.media.reticleShaderSimple);
		}

		// hairs
		CG_FillRect(84, 239, 177, 2, color);     // left
		CG_FillRect(320, 242, 1, 58, color);     // center top
		CG_FillRect(319, 300, 2, 178, color);    // center bot
		CG_FillRect(380, 239, 177, 2, color);    // right
	}
}

/*
==============
CG_DrawBinocReticle
==============
*/
static void CG_DrawBinocReticle(void) {
	// an alternative.  This gives nice sharp lines at the expense of a few extra polys
	vec4_t color;

	color[0] = color[1] = color[2] = 0;
	color[3] = 1;

	if (cgs.media.binocShaderSimple) {
		CG_DrawPic(0, 0, 640, 480, cgs.media.binocShaderSimple);
	}

	CG_FillRect(146, 239, 348, 1, color);

	CG_FillRect(188, 234, 1, 13, color);     // ll
	CG_FillRect(234, 226, 1, 29, color);     // l
	CG_FillRect(274, 234, 1, 13, color);     // lr
	CG_FillRect(320, 213, 1, 55, color);     // center
	CG_FillRect(360, 234, 1, 13, color);     // rl
	CG_FillRect(406, 226, 1, 29, color);     // r
	CG_FillRect(452, 234, 1, 13, color);     // rr
}

void CG_FinishWeaponChange(int lastweap, int newweap);   // JPW NERVE


/*
=================
CG_DrawCrosshair
=================
*/
static void CG_DrawCrosshair(void) {
	float     w, h;
	qhandle_t hShader;
	float     f;
	float     x, y;
	int       weapnum;          // DHM - Nerve

	if (cg.renderingThirdPerson) {
		return;
	}

	// Nico, don't draw crosshair if scoreboard is up
	if (cg.showScores) {
		return;
	}

	// using binoculars
	if (cg.zoomedBinoc) {
		CG_DrawBinocReticle();
		return;
	}

	// DHM - Nerve :: show reticle in limbo and spectator
	if ((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback) {
		weapnum = cg.snap->ps.weapon;
	} else {
		weapnum = cg.weaponSelect;
	}


	switch (weapnum) {

	// weapons that get no reticle
	case WP_NONE:       // no weapon, no crosshair
		if (cg.zoomedBinoc) {
			CG_DrawBinocReticle();
		}

		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
			return;
		}
		break;

	// special reticle for weapon
	case WP_FG42SCOPE:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
		if (!BG_PlayerMounted(cg.snap->ps.eFlags)) {
			// JPW NERVE -- don't let players run with rifles -- speed 80 == crouch, 128 == walk, 256 == run
			if (VectorLengthSquared(cg.snap->ps.velocity) > SQR(127)) {
				if (cg.snap->ps.weapon == WP_FG42SCOPE) {
					CG_FinishWeaponChange(WP_FG42SCOPE, WP_FG42);
				}
				if (cg.snap->ps.weapon == WP_GARAND_SCOPE) {
					CG_FinishWeaponChange(WP_GARAND_SCOPE, WP_GARAND);
				}
				if (cg.snap->ps.weapon == WP_K43_SCOPE) {
					CG_FinishWeaponChange(WP_K43_SCOPE, WP_K43);
				}
			}

			// OSP
			if (cg.snap->ps.stats[STAT_HEALTH] > 0) {
				CG_DrawWeapReticle();
			}

			return;
		}
		break;
	default:
		break;
	}

	if (cg.predictedPlayerState.eFlags & EF_PRONE_MOVING) {
		return;
	}

	// FIXME: spectators/chasing?
	if (cg.predictedPlayerState.weapon == WP_MORTAR_SET && cg.predictedPlayerState.weaponstate != WEAPON_RAISING) {
		return;
	}

	if (cg_drawCrosshair.integer < 0) {   //----(SA)	moved down so it doesn't keep the scoped weaps from drawing reticles
		return;
	}

	// no crosshair while leaning
	if (cg.snap->ps.leanf) {
		return;
	}

	// TAT 1/10/2003 - Don't draw crosshair if have exit hintcursor
	if (cg.snap->ps.serverCursorHint >= HINT_EXIT && cg.snap->ps.serverCursorHint <= HINT_NOEXIT) {
		return;
	}

	// set color based on health
	if (cg_crosshairHealth.integer) {
		vec4_t hcolor;

		CG_ColorForHealth(hcolor);
		trap_R_SetColor(hcolor);
	} else {
		trap_R_SetColor(cg.xhairColor);
	}

	w = h = cg_crosshairSize.value;

	// RF, crosshair size represents aim spread
	f  = (float)((cg_crosshairPulse.integer == 0) ? 0 : cg.snap->ps.aimSpreadScale / 255.0);
	w *= (1 + f * 2.0);
	h *= (1 + f * 2.0);

	x = cg_crosshairX.integer;
	y = cg_crosshairY.integer;
	CG_AdjustFrom640(&x, &y, &w, &h);

	hShader = cgs.media.crosshairShader[cg_drawCrosshair.integer % NUM_CROSSHAIRS];

	trap_R_DrawStretchPic(x + 0.5 * (cg.refdef_current->width - w), y + 0.5 * (cg.refdef_current->height - h), w, h, 0, 0, 1, 1, hShader);

	if (cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]) {
		w = h = cg_crosshairSize.value;
		x = cg_crosshairX.integer;
		y = cg_crosshairY.integer;
		CG_AdjustFrom640(&x, &y, &w, &h);

		if (cg_crosshairHealth.integer == 0) {
			trap_R_SetColor(cg.xhairColorAlt);
		}

		trap_R_DrawStretchPic(x + 0.5 * (cg.refdef_current->width - w), y + 0.5 * (cg.refdef_current->height - h), w, h, 0, 0, 1, 1, cg.crosshairShaderAlt[cg_drawCrosshair.integer % NUM_CROSSHAIRS]);
	}
}

/*
=================
CG_ScanForCrosshairEntity
=================

Returns the distance to the entity

*/
static float CG_ScanForCrosshairEntity(float *zChange, qboolean *hitClient) {
	trace_t trace;
	vec3_t  start, end;
	float   dist;

	// We haven't hit a client yet
	*hitClient = qfalse;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 8192, cg.refdef.viewaxis[0], end);      //----(SA)	changed from 8192

	cg.crosshairClientNoShoot = qfalse;

	CG_Trace(&trace, start, NULL, NULL, end, cg.snap->ps.clientNum, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_ITEM);

	// How far from start to end of trace?
	dist = VectorDistance(start, trace.endpos);

	// How far up or down are we looking?
	*zChange = trace.endpos[2] - start[2];

	if (trace.entityNum >= MAX_CLIENTS) {
		if (cg_entities[trace.entityNum].currentState.eFlags & EF_TAGCONNECT) {
			trace.entityNum = cg_entities[trace.entityNum].tagParent;
		}

		// is a tank with a healthbar
		// this might have some side-effects, but none right now as the script_mover is the only one that sets effect1Time
		if ((cg_entities[trace.entityNum].currentState.eType == ET_MOVER && cg_entities[trace.entityNum].currentState.effect1Time) ||
		    cg_entities[trace.entityNum].currentState.eType == ET_CONSTRUCTIBLE_MARKER) {
			// update the fade timer
			cg.crosshairClientNum    = trace.entityNum;
			cg.crosshairClientTime   = cg.time;
			cg.identifyClientRequest = cg.crosshairClientNum;
		}

		// Default: We're not looking at a client
		cg.crosshairNotLookingAtClient = qtrue;

		return dist;
	}

	// Reset the draw time for the SP crosshair
	cg.crosshairSPClientTime = cg.time;

	// Default: We're not looking at a client
	cg.crosshairNotLookingAtClient = qfalse;

	// We hit a client
	*hitClient = qtrue;

	// update the fade timer
	cg.crosshairClientNum  = trace.entityNum;
	cg.crosshairClientTime = cg.time;
	if (cg.crosshairClientNum != cg.snap->ps.identifyClient && cg.crosshairClientNum != ENTITYNUM_WORLD) {
		cg.identifyClientRequest = cg.crosshairClientNum;
	}

	return dist;
}



#define CH_KNIFE_DIST       48  // from g_weapon.c
#define CH_LADDER_DIST      100
#define CH_WATER_DIST       100
#define CH_BREAKABLE_DIST   64
#define CH_DOOR_DIST        96

#define CH_DIST             100 //128		// use the largest value from above

/*
==============
CG_CheckForCursorHints
    concept in progress...
==============
*/
void CG_CheckForCursorHints(void) {
	trace_t   trace;
	vec3_t    start, end;
	centity_t *tracent;
	vec3_t    pforward, eforward;
	float     dist;


	if (cg.renderingThirdPerson) {
		return;
	}

	if (cg.snap->ps.serverCursorHint) {    // server is dictating a cursor hint, use it.
		cg.cursorHintTime  = cg.time;
		cg.cursorHintFade  = 500;   // fade out time
		cg.cursorHintIcon  = cg.snap->ps.serverCursorHint;
		cg.cursorHintValue = cg.snap->ps.serverCursorHintVal;
		return;
	}

	// From here on it's client-side cursor hints.  So if the server isn't sending that info (as an option)
	// then it falls into here and you can get basic cursorhint info if you want, but not the detailed info
	// the server sends.

	// the trace
	VectorCopy(cg.refdef_current->vieworg, start);
	VectorMA(start, CH_DIST, cg.refdef_current->viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.snap->ps.clientNum, MASK_PLAYERSOLID);

	if (trace.fraction == 1) {
		return;
	}

	dist = trace.fraction * CH_DIST;

	tracent = &cg_entities[trace.entityNum];

	// Arnout: invisible entities don't show hints
	if (trace.entityNum >= MAX_CLIENTS &&
	    (tracent->currentState.powerups == STATE_INVISIBLE ||
	     tracent->currentState.powerups == STATE_UNDERCONSTRUCTION)) {
		return;
	}

	//
	// world
	//
	if (trace.entityNum == ENTITYNUM_WORLD) {
		if ((trace.surfaceFlags & SURF_LADDER) && !(cg.snap->ps.pm_flags & PMF_LADDER)) {
			if (dist <= CH_LADDER_DIST) {
				cg.cursorHintIcon  = HINT_LADDER;
				cg.cursorHintTime  = cg.time;
				cg.cursorHintFade  = 500;
				cg.cursorHintValue = 0;
			}
		}


	} else if (trace.entityNum < MAX_CLIENTS) {     // people
		// knife
		if (trace.entityNum < MAX_CLIENTS && (cg.snap->ps.weapon == WP_KNIFE)) {
			if (dist <= CH_KNIFE_DIST) {

				AngleVectors(cg.snap->ps.viewangles, pforward, NULL, NULL);
				AngleVectors(tracent->lerpAngles, eforward, NULL, NULL);

				if (DotProduct(eforward, pforward) > 0.6f) {         // from behind(-ish)
					cg.cursorHintIcon  = HINT_KNIFE;
					cg.cursorHintTime  = cg.time;
					cg.cursorHintFade  = 100;
					cg.cursorHintValue = 0;
				}
			}
		}
	}
}



/*
=====================
CG_DrawCrosshairNames
=====================
*/
// Nico, heavilly modified to always display colored player names
static void CG_DrawCrosshairNames(void) {
	float      *color;
	float      w;
	const char *s;
	// Distance to the entity under the crosshair
	float    zChange;
	qboolean hitClient = qfalse;
	float    dist      = 0;

	if (cg_drawCrosshair.integer < 0) {
		return;
	}

	// Nico, don't draw crosshair names if scoreboard is up
	if (cg.showScores) {
		return;
	}

	// scan the known entities to see if the crosshair is sighted on one
	dist = CG_ScanForCrosshairEntity(&zChange, &hitClient);

	// Nico, don't draw if hiding others is enabled and distance to the player is < cg_hideRange
	if (cg_hideOthers.integer && dist < cg_hideRange.integer) {
		return;
	}

	// Nico, don't draw if client has hideme turned on
	if (cgs.clientinfo[cg.crosshairClientNum].hideme) {
		return;
	}

	if (cg.renderingThirdPerson) {
		return;
	}

	// draw the name of the player being looked at
	color = CG_FadeColor(cg.crosshairClientTime, 500);

	if (!color) {
		trap_R_SetColor(NULL);
		return;
	}

	if (!cg_drawCrosshairNames.integer) {
		return;
	}

	// Nico, only display valid clientnum players
	if (cgs.clientinfo[cg.crosshairClientNum].clientNum != cg.crosshairClientNum) {
		return;
	}

	s = va("%s", cgs.clientinfo[cg.crosshairClientNum].name);

	w = (float)CG_Text_Width_Ext(s, 0.2f, 0.2f, &cgs.media.limboFont1) / 2;

	CG_Text_Paint_Ext(320 - w, 200, 0.2f, 0.2f, colorWhite, s, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	trap_R_SetColor(NULL);
}



//==============================================================================

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote(void) {
	char  *s;
	char  str1[32], str2[32];
	float color[4] = { 1, 1, 0, 1 };
	int   sec;

	if (cgs.applicationEndTime > cg.time && cgs.applicationClient >= 0) {
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = va(CG_TranslateString("Accept %s's application to join your fireteam?"), cgs.clientinfo[cgs.applicationClient].name);
		CG_DrawStringExt(8, 200, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.propositionEndTime > cg.time && cgs.propositionClient >= 0) {
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = va(CG_TranslateString("Accept %s's proposition to invite %s to join your fireteam?"), cgs.clientinfo[cgs.propositionClient2].name, cgs.clientinfo[cgs.propositionClient].name);
		CG_DrawStringExt(8, 200, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.invitationEndTime > cg.time && cgs.invitationClient >= 0) {
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		s = va(CG_TranslateString("Accept %s's invitation to join their fireteam?"), cgs.clientinfo[cgs.invitationClient].name);
		CG_DrawStringExt(8, 200, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

		s = va(CG_TranslateString("Press '%s' for YES, or '%s' for No"), str1, str2);
		CG_DrawStringExt(8, 214, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
		return;
	}

	if (cgs.voteTime) {
		Q_strncpyz(str1, BindingFromName("vote yes"), 32);
		Q_strncpyz(str2, BindingFromName("vote no"), 32);

		// play a talk beep whenever it is modified
		if (cgs.voteModified) {
			cgs.voteModified = qfalse;
		}

		sec = (VOTE_TIME - (cg.time - cgs.voteTime)) / 1000;
		if (sec < 0) {
			sec = 0;
		}

		if (!Q_stricmpn(cgs.voteString, "kick", 4)) {
			if (strlen(cgs.voteString) > 5) {
				int  nameindex;
				char buffer[128];
				Q_strncpyz(buffer, cgs.voteString + 5, sizeof (buffer));
				Q_CleanStr(buffer);

				for (nameindex = 0; nameindex < MAX_CLIENTS; nameindex++) {
					if (!cgs.clientinfo[nameindex].infoValid) {
						continue;
					}

					if (!Q_stricmp(cgs.clientinfo[nameindex].cleanname, buffer)) {
						if (cgs.clientinfo[nameindex].team != TEAM_SPECTATOR && cgs.clientinfo[nameindex].team != cgs.clientinfo[cg.clientNum].team) {
							return;
						}
					}
				}
			}
		}

		if (!(cg.snap->ps.eFlags & EF_VOTED)) {
			s = va(CG_TranslateString("VOTE(%i): %s"), sec, cgs.voteString);
			CG_DrawStringExt(8, 200, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

			/* Nico, spectators can also vote
			if ( cgs.clientinfo[cg.clientNum].team != TEAM_AXIS && cgs.clientinfo[cg.clientNum].team != TEAM_ALLIES ) {
			    s = CG_TranslateString( "Cannot vote as Spectator" );
			} else {*/
			s = va(CG_TranslateString("YES(%s):%i, NO(%s):%i"), str1, cgs.voteYes, str2, cgs.voteNo);
			// }
			CG_DrawStringExt(8, 214, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 60);
			return;
		} else {
			s = va(CG_TranslateString("YOU VOTED ON: %s"), cgs.voteString);
			CG_DrawStringExt(8, 200, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);

			s = va(CG_TranslateString("Y:%i, N:%i"), cgs.voteYes, cgs.voteNo);
			CG_DrawStringExt(8, 214, s, color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 20);
			return;
		}
	}

	if (cgs.applicationEndTime > cg.time && cgs.applicationClient < 0) {
		if (cgs.applicationClient == -1) {
			s = "Your application has been submitted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.applicationClient == -2) {
			s = "Your application failed";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.applicationClient == -3) {
			s = "Your application has been approved";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.applicationClient == -4) {
			s = "Your application reply has been sent";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}
	}

	if (cgs.propositionEndTime > cg.time && cgs.propositionClient < 0) {
		if (cgs.propositionClient == -1) {
			s = "Your proposition has been submitted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.propositionClient == -2) {
			s = "Your proposition was rejected";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.propositionClient == -3) {
			s = "Your proposition was accepted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.propositionClient == -4) {
			s = "Your proposition reply has been sent";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qtrue, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}
	}

	if (cgs.invitationEndTime > cg.time && cgs.invitationClient < 0) {
		if (cgs.invitationClient == -1) {
			s = "Your invitation has been submitted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.invitationClient == -2) {
			s = "Your invitation was rejected";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.invitationClient == -3) {
			s = "Your invitation was accepted";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.invitationClient == -4) {
			s = "Your invitation reply has been sent";
			CG_DrawStringExt(8, 200, CG_TranslateString(s), color, qtrue, qfalse, TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 80);
			return;
		}

		if (cgs.invitationClient < 0) {
			return;
		}
	}
}

/*
=================
CG_DrawSpectatorMessage
=================
*/
static void CG_DrawSpectatorMessage(void) {
	const char *str, *str2;
	float      y;
	static int lastconfigGet = 0;
	float      textScale     = 0.14;

	if (!cg_descriptiveText.integer) {
		return;
	}

	if (!(cg.snap->ps.pm_flags & PMF_LIMBO || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)) {
		return;
	}

	if (cg.time - lastconfigGet > 1000) {
		Controls_GetConfig();

		lastconfigGet = cg.time;
	}

	y = 408;

	y -= 2 * TINYCHAR_HEIGHT;

	str2 = BindingFromName("openlimbomenu");
	if (!Q_stricmp(str2, "(openlimbomenu)")) {
		str2 = "ESCAPE";
	}
	str = va(CG_TranslateString("Press %s to open Limbo Menu"), str2);
	// Nico, better text drawing
	CG_Text_Paint_Ext(8, 154, textScale, textScale, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	str2 = BindingFromName("+attack");
	str  = va(CG_TranslateString("Press %s to follow next player"), str2);
	// Nico, better text drawing
	CG_Text_Paint_Ext(8, 172, textScale, textScale, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

#define INFOTEXT_STARTX 8

/*
=================
CG_DrawFollow
=================
*/
static qboolean CG_DrawFollow(void) {
	char deploytime[128];

	if (!(cg.snap->ps.pm_flags & PMF_FOLLOW)) {
		return(qfalse);
	}

	// if in limbo, show different follow message
	if (cg.snap->ps.pm_flags & PMF_LIMBO) {

		// Don't display if you're following yourself
		if (cg.snap->ps.clientNum != cg.clientNum) {
			sprintf(deploytime, "(%s %s)", CG_TranslateString("Following"),
			        cgs.clientinfo[cg.snap->ps.clientNum].name);

			CG_DrawStringExt(INFOTEXT_STARTX, 136, deploytime, colorWhite, qtrue, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 80);
		}
	} else {
		CG_DrawStringExt(INFOTEXT_STARTX, 118, CG_TranslateString("Following"), colorWhite, qtrue, qtrue, BIGCHAR_WIDTH / 2, BIGCHAR_HEIGHT, 0);
		CG_DrawStringExt(84, 118, va("%s",
		                             cgs.clientinfo[cg.snap->ps.clientNum].name),
		                 colorWhite, qtrue, qtrue, BIGCHAR_WIDTH / 2, BIGCHAR_HEIGHT, 0);
	}

	return(qtrue);
}
//==================================================================================

/*
=================
CG_DrawFlashFade
=================
*/
static void CG_DrawFlashFade(void) {
	static int lastTime;
	int        elapsed, time;

	if (cgs.fadeStartTime + cgs.fadeDuration < cg.time) {
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	} else if (cgs.fadeAlphaCurrent != cgs.fadeAlpha) {
		elapsed  = (time = trap_Milliseconds()) - lastTime;   // we need to use trap_Milliseconds() here since the cg.time gets modified upon reloading
		lastTime = time;
		if (elapsed < 500 && elapsed > 0) {
			if (cgs.fadeAlphaCurrent > cgs.fadeAlpha) {
				cgs.fadeAlphaCurrent -= ((float)elapsed / (float)cgs.fadeDuration);
				if (cgs.fadeAlphaCurrent < cgs.fadeAlpha) {
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
				}
			} else {
				cgs.fadeAlphaCurrent += ((float)elapsed / (float)cgs.fadeDuration);
				if (cgs.fadeAlphaCurrent > cgs.fadeAlpha) {
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
				}
			}
		}
	}
}



/*
==============
CG_DrawFlashZoomTransition
    hide the snap transition from regular view to/from zoomed

  FIXME: TODO: use cg_fade?
==============
*/
static void CG_DrawFlashZoomTransition(void) {
	vec4_t color;
	float  frac;
	int    fadeTime;

	if (!cg.snap) {
		return;
	}

	if (BG_PlayerMounted(cg.snap->ps.eFlags)) {
		// don't draw when on mg_42
		// keep the timer fresh so when you remove yourself from the mg42, it'll fade
		cg.zoomTime = cg.time;
		return;
	}

	if (cg.renderingThirdPerson) {
		return;
	}

	fadeTime = 400;

	frac = cg.time - cg.zoomTime;

	if (frac < fadeTime) {
		frac = frac / (float)fadeTime;
		Vector4Set(color, 0, 0, 0, 1.0f - frac);
		CG_FillRect(-10, -10, 650, 490, color);
	}
}

/*
=================
CG_DrawFlashFire
=================
*/
static void CG_DrawFlashFire(void) {
	vec4_t col = { 1, 1, 1, 1 };
	float  alpha, max, f;

	if (!cg.snap) {
		return;
	}

	if (cg.renderingThirdPerson) {
		return;
	}

	if (!cg.snap->ps.onFireStart) {
		cg.v_noFireTime = cg.time;
		return;
	}

	alpha = (float)((FIRE_FLASH_TIME - 1000) - (cg.time - cg.snap->ps.onFireStart)) / (FIRE_FLASH_TIME - 1000);
	if (alpha > 0) {
		if (alpha >= 1.0) {
			alpha = 1.0;
		}

		// fade in?
		f = (float)(cg.time - cg.v_noFireTime) / FIRE_FLASH_FADEIN_TIME;
		if (f >= 0.0 && f < 1.0) {
			alpha = f;
		}

		max = 0.5 + 0.5 * sin((float)((cg.time / 10) % 1000) / 1000.0);
		if (alpha > max) {
			alpha = max;
		}
		col[0] = alpha;
		col[1] = alpha;
		col[2] = alpha;
		col[3] = alpha;
		trap_R_SetColor(col);
		CG_DrawPic(-10, -10, 650, 490, cgs.media.viewFlashFire[(cg.time / 50) % 16]);
		trap_R_SetColor(NULL);

		trap_S_AddLoopingSound(cg.snap->ps.origin, vec3_origin, cgs.media.flameSound, (int)(255.0 * alpha), 0);
		trap_S_AddLoopingSound(cg.snap->ps.origin, vec3_origin, cgs.media.flameCrackSound, (int)(255.0 * alpha), 0);
	} else {
		cg.v_noFireTime = cg.time;
	}
}



/*
==============
CG_DrawFlashBlendBehindHUD
    screen flash stuff drawn first (on top of world, behind HUD)
==============
*/
static void CG_DrawFlashBlendBehindHUD(void) {
	CG_DrawFlashZoomTransition();
	CG_DrawFlashFade();
}


/*
=================
CG_DrawFlashBlend
    screen flash stuff drawn last (on top of everything)
=================
*/
static void CG_DrawFlashBlend(void) {
	// Gordon: no flash blends if in limbo or spectator, and in the limbo menu
	if ((cg.snap->ps.pm_flags & PMF_LIMBO || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR)) {
		return;
	}

	CG_DrawFlashFire();
}

// NERVE - SMF
/*
=================
CG_DrawObjectiveInfo
=================
*/
#define OID_LEFT    10
#define OID_TOP     360

void CG_ObjectivePrint(const char *str, int charWidth) {
	char     *s;
	int      i, len;                    // NERVE - SMF
	qboolean neednewline = qfalse;      // NERVE - SMF

	if (cg.centerPrintTime) {
		return;
	}

	s = CG_TranslateString(str);

	Q_strncpyz(cg.oidPrint, s, sizeof (cg.oidPrint));

	// NERVE - SMF - turn spaces into newlines, if we've run over the linewidth
	len = strlen(cg.oidPrint);
	for (i = 0; i < len; i++) {

		// NOTE: subtract a few chars here so long words still get displayed properly
		if (i % (CP_LINEWIDTH - 20) == 0 && i > 0) {
			neednewline = qtrue;
		}
		if (cg.oidPrint[i] == ' ' && neednewline) {
			cg.oidPrint[i] = '\n';
			neednewline    = qfalse;
		}
	}
	// -NERVE - SMF

	cg.oidPrintTime      = cg.time;
	cg.oidPrintY         = OID_TOP;
	cg.oidPrintCharWidth = charWidth;

	// count the number of lines for oiding
	cg.oidPrintLines = 1;
	s                = cg.oidPrint;
	while (*s) {
		if (*s == '\n') {
			cg.oidPrintLines++;
		}
		s++;
	}
}

static void CG_DrawObjectiveInfo(void) {
	char   *start;
	int    l;
	int    x, y, w;
	int    x1, y1, x2, y2;
	float  *color;
	vec4_t backColor;

	backColor[0] = 0.2f;
	backColor[1] = 0.2f;
	backColor[2] = 0.2f;
	backColor[3] = 1.f;

	if (!cg.oidPrintTime) {
		return;
	}

	color = CG_FadeColor(cg.oidPrintTime, 250);
	if (!color) {
		cg.oidPrintTime = 0;
		return;
	}

	trap_R_SetColor(color);

	start = cg.oidPrint;

	y = 400 - cg.oidPrintLines * BIGCHAR_HEIGHT / 2;

	x1 = 319;
	y1 = y - 2;
	x2 = 321;

	// first just find the bounding rect
	while (1) {
		char linebuffer[1024];

		for (l = 0; l < CP_LINEWIDTH; l++) {
			if (!start[l] || start[l] == '\n') {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.oidPrintCharWidth * CG_DrawStrlen(linebuffer) + 10;
// JPW NERVE
		if (320 - w / 2 < x1) {
			x1 = 320 - w / 2;
			x2 = 320 + w / 2;
		}

		x = 320 - w / 2;
// jpw
		y += cg.oidPrintCharWidth * 1.5;

		while (*start && (*start != '\n')) {
			start++;
		}
		if (!*start) {
			break;
		}
		start++;
	}

	x2 = x2 + 4;
	y2 = y - cg.oidPrintCharWidth * 1.5 + 4;

	VectorCopy(color, backColor);
	backColor[3] = 0.5 * color[3];
	trap_R_SetColor(backColor);

	CG_DrawPic(x1, y1, x2 - x1, y2 - y1, cgs.media.teamStatusBar);

	VectorSet(backColor, 0, 0, 0);
	CG_DrawRect(x1, y1, x2 - x1, y2 - y1, 1, backColor);

	trap_R_SetColor(color);

	// do the actual drawing
	start = cg.oidPrint;
	y     = 400 - cg.oidPrintLines * BIGCHAR_HEIGHT / 2; // JPW NERVE


	while (1) {
		char linebuffer[1024];

		for (l = 0; l < CP_LINEWIDTH; l++) {
			if (!start[l] || start[l] == '\n') {
				break;
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = cg.oidPrintCharWidth * CG_DrawStrlen(linebuffer);
		if (x1 + w > x2) {
			x2 = x1 + w;
		}

		x = 320 - w / 2; // JPW NERVE

		CG_DrawStringExt(x, y, linebuffer, color, qfalse, qtrue,
		                 cg.oidPrintCharWidth, (int)(cg.oidPrintCharWidth * 1.5), 0);

		y += cg.oidPrintCharWidth * 1.5;

		while (*start && (*start != '\n')) {
			start++;
		}
		if (!*start) {
			break;
		}
		start++;
	}

	trap_R_SetColor(NULL);
}

//==================================================================================

void CG_DrawTimedMenus() {
	if (cg.voiceTime) {
		int t = cg.time - cg.voiceTime;
		if (t > 2500) {
			Menus_CloseByName("voiceMenu");
			trap_Cvar_Set("cl_conXOffset", "0");
			cg.voiceTime = 0;
		}
	}
}

/*
=================
CG_Fade
=================
*/
void CG_Fade(int r, int g, int b, int a, int time, int duration) {

	// incorporate this into the current fade scheme

	cgs.fadeAlpha     = (float)a / 255.0f;
	cgs.fadeStartTime = time;
	cgs.fadeDuration  = duration;

	if (cgs.fadeStartTime + cgs.fadeDuration <= cg.time) {
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	}
	return;


	if (time <= 0) {    // do instantly
		cg.fadeRate = 1;
		cg.fadeTime = cg.time - 1;  // set cg.fadeTime behind cg.time so it will start out 'done'
	} else {
		cg.fadeRate = 1.0f / time;
		cg.fadeTime = cg.time + time;
	}

	cg.fadeColor2[0] = ( float )r / 255.0f;
	cg.fadeColor2[1] = ( float )g / 255.0f;
	cg.fadeColor2[2] = ( float )b / 255.0f;
	cg.fadeColor2[3] = ( float )a / 255.0f;
}

/*
=================
CG_ScreenFade
=================
*/
static void CG_ScreenFade(void) {
	int    msec;
	int    i;
	float  t, invt;
	vec4_t color;

	if (!cg.fadeRate) {
		return;
	}

	msec = cg.fadeTime - cg.time;
	if (msec <= 0) {
		cg.fadeColor1[0] = cg.fadeColor2[0];
		cg.fadeColor1[1] = cg.fadeColor2[1];
		cg.fadeColor1[2] = cg.fadeColor2[2];
		cg.fadeColor1[3] = cg.fadeColor2[3];

		if (!cg.fadeColor1[3]) {
			cg.fadeRate = 0;
			return;
		}

		CG_FillRect(0, 0, 640, 480, cg.fadeColor1);

	} else {
		t    = ( float )msec * cg.fadeRate;
		invt = 1.0f - t;

		for (i = 0; i < 4; i++) {
			color[i] = cg.fadeColor1[i] * t + cg.fadeColor2[i] * invt;
		}

		if (color[3]) {
			CG_FillRect(0, 0, 640, 480, color);
		}
	}
}

static int CG_PlayerAmmoValue(int *ammo, int *clips, int *akimboammo) {
	centity_t     *cent;
	playerState_t *ps;
	int           weap;
	qboolean      skipammo = qfalse;

	*ammo = *clips = *akimboammo = -1;

	if (cg.snap->ps.clientNum == cg.clientNum) {
		cent = &cg.predictedPlayerEntity;
	} else {
		cent = &cg_entities[cg.snap->ps.clientNum];
	}
	ps = &cg.snap->ps;

	weap = cent->currentState.weapon;

	if (!weap) {
		return weap;
	}

	switch (weap) {        // some weapons don't draw ammo count text
	case WP_AMMO:
	case WP_MEDKIT:
	case WP_KNIFE:
	case WP_PLIERS:
	case WP_SMOKE_MARKER:
	case WP_DYNAMITE:
	case WP_SATCHEL:
	case WP_SATCHEL_DET:
	case WP_SMOKE_BOMB:
	case WP_BINOCULARS:
		return weap;

	case WP_LANDMINE:
	case WP_MEDIC_SYRINGE:
	case WP_MEDIC_ADRENALINE:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_FLAMETHROWER:
	case WP_MORTAR:
	case WP_MORTAR_SET:
	case WP_PANZERFAUST:
		skipammo = qtrue;
		break;

	default:
		break;
	}

	if (cg.snap->ps.eFlags & EF_MG42_ACTIVE || cg.snap->ps.eFlags & EF_MOUNTEDTANK) {
		return WP_MOBILE_MG42;
	}

	// total ammo in clips
	*clips = cg.snap->ps.ammo[BG_FindAmmoForWeapon(weap)];

	// current clip
	*ammo = ps->ammoclip[BG_FindClipForWeapon(weap)];

	if (BG_IsAkimboWeapon(weap)) {
		*akimboammo = ps->ammoclip[BG_FindClipForWeapon(BG_AkimboSidearm(weap))];
	} else {
		*akimboammo = -1;
	}

	if (weap == WP_LANDMINE) {
		if (!cgs.gameManager) {
			*ammo = 0;
		} else {
			if (cgs.clientinfo[ps->clientNum].team == TEAM_AXIS) {
				*ammo = cgs.gameManager->currentState.otherEntityNum;
			} else {
				*ammo = cgs.gameManager->currentState.otherEntityNum2;
			}
		}
	} else if (weap == WP_MORTAR || weap == WP_MORTAR_SET || weap == WP_PANZERFAUST) {
		*ammo += *clips;
	}

	if (skipammo) {
		*clips = -1;
	}

	return weap;
}

static void CG_DrawPlayerHealthBar(rectDef_t *rect) {
	vec4_t bgcolour = { 1.f, 1.f, 1.f, 0.3f };
	vec4_t colour;

	int   flags = 1 | 4 | 16 | 64;
	float frac;

	CG_ColorForHealth(colour);
	colour[3] = 0.5f;

	if (cgs.clientinfo[cg.snap->ps.clientNum].cls == PC_MEDIC) {
		frac = cg.snap->ps.stats[STAT_HEALTH] / ((float) cg.snap->ps.stats[STAT_MAX_HEALTH] * 1.12f);
	} else {
		frac = cg.snap->ps.stats[STAT_HEALTH] / (float) cg.snap->ps.stats[STAT_MAX_HEALTH];
	}


	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, colour, NULL, bgcolour, frac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x, rect->y + rect->h + 4, rect->w, rect->w, cgs.media.hudHealthIcon);
}

static void CG_DrawWeapRecharge(rectDef_t *rect) {
	float    barFrac, chargeTime;
	int      flags;
	qboolean fade = qfalse;

	vec4_t bgcolor = { 1.0f, 1.0f, 1.0f, 0.25f };
	vec4_t color;

	flags = 1 | 4 | 16;

	// Draw power bar
	if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_ENGINEER) {
		chargeTime = cg.engineerChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	} else if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_MEDIC) {
		chargeTime = cg.medicChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	} else if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_FIELDOPS) {
		chargeTime = cg.ltChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	} else if (cg.snap->ps.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS) {
		chargeTime = cg.covertopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	} else {
		chargeTime = cg.soldierChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
	}

	barFrac = (float)(cg.time - cg.snap->ps.classWeaponTime) / chargeTime;
	if (barFrac > 1.0) {
		barFrac = 1.0;
	}

	color[0] = 1.0f;
	color[1] = color[2] = barFrac;
	color[3] = 0.25 + barFrac * 0.5;

	if (fade) {
		bgcolor[3] *= 0.4f;
		color[3]   *= 0.4;
	}

	CG_FilledBar(rect->x, rect->y + (rect->h * 0.1f), rect->w, rect->h * 0.84f, color, NULL, bgcolor, barFrac, flags);

	trap_R_SetColor(NULL);
	CG_DrawPic(rect->x, rect->y, rect->w, rect->h, cgs.media.hudSprintBar);
	CG_DrawPic(rect->x + (rect->w * 0.25f) - 1, rect->y + rect->h + 4, (rect->w * 0.5f) + 2, rect->w + 2, cgs.media.hudPowerIcon);
}

static void CG_DrawPlayerStatus(void) {
	int       value, value2, value3;
	char      buffer[32];
	rectDef_t rect;

	// Draw weapon icon and overheat bar
	rect.x = 640 - 82;
	rect.y = 480 - 56;
	rect.w = 60;
	rect.h = 32;
	CG_DrawWeapHeat(&rect, HUD_HORIZONTAL);

	if (cg_drawWeaponIconFlash.integer == 0) {
		CG_DrawPlayerWeaponIcon(&rect, ITEM_ALIGN_RIGHT, &colorWhite);
	} else {
		int ws = BG_simpleWeaponState(cg.snap->ps.weaponstate);
		CG_DrawPlayerWeaponIcon(&rect, ITEM_ALIGN_RIGHT, ((ws == WSTATE_SWITCH) ? &colorWhite : (ws == WSTATE_FIRE) ? &colorRed : &colorYellow));
	}

	// Draw ammo
	CG_PlayerAmmoValue(&value, &value2, &value3);
	if (value3 >= 0) {
		Com_sprintf(buffer, sizeof (buffer), "%i|%i/%i", value3, value, value2);
		CG_Text_Paint_Ext(640 - 22 - CG_Text_Width_Ext(buffer, .25f, 0, &cgs.media.limboFont1), 480 - 1 * (16 + 2) + 12 - 4, .25f, .25f, colorWhite, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else if (value2 >= 0) {
		Com_sprintf(buffer, sizeof (buffer), "%i/%i", value, value2);
		CG_Text_Paint_Ext(640 - 22 - CG_Text_Width_Ext(buffer, .25f, 0, &cgs.media.limboFont1), 480 - 1 * (16 + 2) + 12 - 4, .25f, .25f, colorWhite, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else if (value >= 0) {
		Com_sprintf(buffer, sizeof (buffer), "%i", value);
		CG_Text_Paint_Ext(640 - 22 - CG_Text_Width_Ext(buffer, .25f, 0, &cgs.media.limboFont1), 480 - 1 * (16 + 2) + 12 - 4, .25f, .25f, colorWhite, buffer, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}

	rect.x = 4; // Nico, 24 -> 4
	rect.y = 480 - 92;
	rect.w = 12;
	rect.h = 72;
	CG_DrawPlayerHealthBar(&rect);

	rect.x = 640 - 16;
	rect.y = 480 - 92;
	rect.w = 12;
	rect.h = 72;
	CG_DrawWeapRecharge(&rect);
}

// Nico, heavilly modified
static void CG_DrawPlayerStats(void) {
	const char *str;
	float      w;

	str = va("%i", cg.snap->ps.stats[STAT_HEALTH]);
	w   = CG_Text_Width_Ext(str, 0.25f, 0, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(42 - w, 474, 0.25f, 0.25f, colorWhite, str, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(44, 474, 0.2f, 0.2f, colorWhite, "HP", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

static char statsDebugStrings[6][512];
static int  statsDebugTime[6];
static int  statsDebugTextWidth[6];
static int  statsDebugPos;

void CG_InitStatsDebug(void) {
	memset(&statsDebugStrings, 0, sizeof (statsDebugStrings));
	memset(&statsDebugTime, 0, sizeof (statsDebugTime));
	statsDebugPos = -1;
}

void CG_StatsDebugAddText(const char *text) {
	if (cg_debugSkills.integer) {
		statsDebugPos++;

		if (statsDebugPos >= 6) {
			statsDebugPos = 0;
		}

		Q_strncpyz(statsDebugStrings[statsDebugPos], text, 512);
		statsDebugTime[statsDebugPos]      = cg.time;
		statsDebugTextWidth[statsDebugPos] = CG_Text_Width_Ext(text, .15f, 0, &cgs.media.limboFont2);

		CG_Printf("%s\n", text);
	}
}

//bani
void CG_DrawDemoRecording(void) {
	char   status[1024];
	char   demostatus[128];
	char   wavestatus[128];
	vec4_t clrUiWhite = { 1.0f, 1.0f, 1.0f, 0.8f };
	vec4_t clrUiRed   = { 1.0f, 0.0f, 0.0f, 0.8f };

	if (!cl_demorecording.integer && !cl_waverecording.integer) {
		return;
	}

	if (!cg_recording_statusline.integer) {
		return;
	}

	if (cl_demorecording.integer) {
		Com_sprintf(demostatus, sizeof (demostatus), " demo %s: %ik ", cl_demofilename.string, cl_demooffset.integer / 1024);
	} else {
		strncpy(demostatus, "", sizeof (demostatus));
	}

	if (cl_waverecording.integer) {
		Com_sprintf(wavestatus, sizeof (demostatus), " audio %s: %ik ", cl_wavefilename.string, cl_waveoffset.integer / 1024);
	} else {
		strncpy(wavestatus, "", sizeof (wavestatus));
	}

	Com_sprintf(status, sizeof (status), "recording%s%s", demostatus, wavestatus);

	// Nico, add recording red dot
	CG_Text_Paint_Ext(0, cg_recording_statusline.integer, 0.5f, 0.5f, clrUiRed, ".", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	CG_Text_Paint_Ext(14, cg_recording_statusline.integer, 0.14f, 0.14f, clrUiWhite, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D(void) {
	CG_ScreenFade();

	if (cg.editingSpeakers) {
		CG_SpeakerEditorDraw();
		return;
	}

	//bani - #127 - no longer cheat protected, we draw crosshair/reticle in non demoplayback
	if (cg_draw2D.integer == 0) {
		if (cg.demoPlayback) {
			return;
		}
		CG_DrawCrosshair();
		CG_DrawFlashFade();
		return;
	}

	if (!cg.cameraMode) {
		CG_DrawFlashBlendBehindHUD();

		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {

			CG_DrawCrosshair();
			CG_DrawCrosshairNames();

			// NERVE - SMF - we need to do this for spectators as well
			CG_DrawTeamInfo();
		} else {
			// don't draw any status if dead
			if (cg.snap->ps.stats[STAT_HEALTH] > 0 || (cg.snap->ps.pm_flags & PMF_FOLLOW)) {

				CG_DrawCrosshair();

				CG_DrawCrosshairNames();
			}

			CG_DrawTeamInfo();

			if (cg_drawStatus.integer) {
				Menu_PaintAll();
				CG_DrawTimedMenus();
			}
		}

		// Nico, do not draw this if scoreboard is up
		if (!cg.showScores) {
			CG_DrawVote();

			CG_DrawLagometer();
		}
	}

	// don't draw center string if scoreboard is up
	if (!CG_DrawScoreboard()) {
		if (cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR) {
			rectDef_t rect;

			if (cg.snap->ps.stats[STAT_HEALTH] > 0) {

				CG_DrawPlayerStatus();
				CG_DrawPlayerStats();
			}

			// Cursor hint
			rect.w = rect.h = 48;
			rect.x = .5f * SCREEN_WIDTH - .5f * rect.w;
			rect.y = 260;
			CG_DrawCursorhint(&rect);

			// Stability bar
			rect.x = 50;
			rect.y = 208;
			rect.w = 10;
			rect.h = 64;
			CG_DrawWeapStability(&rect);
		}

		if (!cg_paused.integer) {
			CG_DrawUpperRight();
		}

		CG_DrawCenterString();
		CG_DrawPMItems();
		CG_DrawPMItemsBig();

		CG_DrawFollow();

		CG_DrawNotify();

		CG_DrawObjectiveInfo();

		// Nico, draw speed meter
		CG_DrawSpeedMeter();

		// Nico, draw OB
		CG_DrawOB();

		// Nico, draw timer
		CG_DrawTimer();

		// Nico, draw check points
		CG_DrawCheckpoints();

		// Nico, draw CGaz
		CG_DrawCGaz();

		// Nico, draw keys pressed
		CG_DrawKeys();

		// Nico, draw banners
		CG_DrawBannerPrint();

		// Nico, draw info panel
		CG_DrawInfoPanel();

		CG_DrawSpectatorMessage();
	} else {
		if ((int)cgs.eventHandling != (int)CGAME_EVENT_NONE) {
			trap_R_SetColor(NULL);
			CG_DrawPic(cgDC.cursorx - 14, cgDC.cursory - 14, 32, 32, cgs.media.cursorIcon);
		}
	}

	if (cg.showFireteamMenu) {
		CG_Fireteams_Draw();
	}

	// Info overlays
	CG_DrawOverlays();

	// OSP - window updates
	CG_windowDraw();

	// Ridah, draw flash blends now
	CG_DrawFlashBlend();

	CG_DrawDemoRecording();
}

// NERVE - SMF
void CG_StartShakeCamera(float p) {
	cg.cameraShakeScale = p;

	cg.cameraShakeLength = 1000 * (p * p);
	cg.cameraShakeTime   = cg.time + cg.cameraShakeLength;
	cg.cameraShakePhase  = crandom() * M_PI; // start chain in random dir
}

void CG_ShakeCamera() {
	float x, val;

	if (cg.time > cg.cameraShakeTime) {
		cg.cameraShakeScale = 0; // JPW NERVE all pending explosions resolved, so reset shakescale
		return;
	}

	// JPW NERVE starts at 1, approaches 0 over time
	x = (cg.cameraShakeTime - cg.time) / cg.cameraShakeLength;

	// ydnar: move the camera
	val                   = sin(M_PI * 7 * x + cg.cameraShakePhase) * x * 4.0f * cg.cameraShakeScale;
	cg.refdef.vieworg[2] += val;
	val                   = sin(M_PI * 13 * x + cg.cameraShakePhase) * x * 4.0f * cg.cameraShakeScale;
	cg.refdef.vieworg[1] += val;
	val                   = cos(M_PI * 17 * x + cg.cameraShakePhase) * x * 4.0f * cg.cameraShakeScale;
	cg.refdef.vieworg[0] += val;

	AnglesToAxis(cg.refdefViewAngles, cg.refdef.viewaxis);
}
// -NERVE - SMF

void CG_DrawMiscGamemodels(void) {
	int         i, j;
	refEntity_t ent;
	int         drawn = 0;

	memset(&ent, 0, sizeof (ent));

	ent.reType            = RT_MODEL;
	ent.nonNormalizedAxes = qtrue;

	// ydnar: static gamemodels don't project shadows
	ent.renderfx = RF_NOSHADOW;

	for (i = 0; i < cg.numMiscGameModels; i++) {
		if (cgs.miscGameModels[i].radius) {
			if (CG_CullPointAndRadius(cgs.miscGameModels[i].org, cgs.miscGameModels[i].radius)) {
				continue;
			}
		}

		if (!trap_R_inPVS(cg.refdef_current->vieworg, cgs.miscGameModels[i].org)) {
			continue;
		}

		VectorCopy(cgs.miscGameModels[i].org, ent.origin);
		VectorCopy(cgs.miscGameModels[i].org, ent.oldorigin);
		VectorCopy(cgs.miscGameModels[i].org, ent.lightingOrigin);

		for (j = 0; j < 3; j++) {
			VectorCopy(cgs.miscGameModels[i].axes[j], ent.axis[j]);
		}
		ent.hModel = cgs.miscGameModels[i].model;

		trap_R_AddRefEntityToScene(&ent);

		drawn++;
	}
}

/**
 * @source: TJMod
 */
static void CG_Autodemo() {
	static int rs_time = 0, rs_keep = 0, nd_time = 0, nd_keep = 0;

	// Start recording a new demo automatically when hitting stoptimer with 1sec delay
	// Don't run any of this during demoplayback, same goes for initiating commands.
	if (cg_autoDemo.integer && !cg.demoPlayback) {
		if (cg.startedNewDemo > 1) {
			if (!nd_keep) {
				nd_time = cg.time;
				nd_keep = 1;
			}

			if (cg.time > nd_time + 1000) {
				trap_SendConsoleCommand("stoprecord\n");
				trap_SendConsoleCommand(va("record temp_%i\n", cg.startedNewDemo - 1));
				cg.startedNewDemo = 1;
			}
		} else {
			nd_time = nd_keep = 0;
		}

		if (cg.runsave) {
			if (!rs_keep) {
				if (cg_autoDemo.integer == 1) {
					CG_AddPMItem(PM_MESSAGE, va("%s^w: stopping and saving demo..\n", GAME_VERSION_COLORED), cgs.media.voiceChatShader);
				}
				rs_time = cg.time;
				rs_keep = 1;
			}

			if (cg.time > rs_time + 1500 && rs_keep == 1 && !cg.startedNewDemo) {
				trap_SendConsoleCommand("stoprecord\n");
				rs_keep = 2;
			}

			if (cg.time > rs_time + 2000) {
				int          len = 0;
				fileHandle_t temp, demo;
				char         *name = NULL;
				int          i     = 0;

				len = trap_FS_FOpenFile(va("demos/temp_%i.dm_84", cg.currentdemo), &temp, FS_READ);

				name = va("demos/%s_%s.dm_84", cgs.rawmapname, cg.runsavename);

				trap_FS_FOpenFile(name, &demo, FS_WRITE);

				//
				// BUG: Something > max msglen or something like that,
				// NO IDEA what causes it, since its for both source and copy of the demo..
				// COULD possibly be because both have been opened, but that seems weird.
				//
				for (i = 0; i < len; ++i) {
					byte b = 0;

					trap_FS_Read(&b, 1, temp);
					trap_FS_Write(&b, 1, demo);
				}

				trap_FS_FCloseFile(temp);
				trap_FS_FCloseFile(demo);

				CG_Printf("^3Demo saved as:^7 %s\n", name);

				if (cg_autoDemo.integer == 1) {
					CG_AddPMItem(PM_MESSAGE, va("%s^w: demo saved!\n", GAME_VERSION_COLORED), cgs.media.voiceChatShader);
				}

				cg.runsave = rs_keep = 0;
			}
		} else {
			rs_time = rs_keep = 0;
		}
	}
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive(stereoFrame_t stereoView) {
	float  separation;
	vec3_t baseOrg;

	// optionally draw the info screen instead
	if (!cg.snap) {
		CG_DrawInformation(qfalse);
		return;
	}

	switch (stereoView) {
	case STEREO_CENTER:
		separation = 0;
		break;
	case STEREO_LEFT:
		separation = -cg_stereoSeparation.value / 2;
		break;
	case STEREO_RIGHT:
		separation = cg_stereoSeparation.value / 2;
		break;
	default:
		separation = 0;
		CG_Error("CG_DrawActive: Undefined stereoView");
	}


	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy(cg.refdef_current->vieworg, baseOrg);
	if (separation != 0) {
		VectorMA(cg.refdef_current->vieworg, -separation, cg.refdef_current->viewaxis[1], cg.refdef_current->vieworg);
	}

	cg.refdef_current->glfog.registered = 0;    // make sure it doesn't use fog from another scene

	CG_ShakeCamera();       // NERVE - SMF

	// Gordon
	CG_PB_RenderPolyBuffers();

	// Gordon
	CG_DrawMiscGamemodels();

	trap_R_RenderScene(cg.refdef_current);

	// restore original viewpoint if running stereo
	if (separation != 0) {
		VectorCopy(baseOrg, cg.refdef_current->vieworg);
	}

	// Nico, render while in limbo
	CG_Draw2D();

	// Nico, handle autodemo system
	CG_Autodemo();

	if (cg.showGameView) {
		CG_LimboPanel_Draw();
	}
}
