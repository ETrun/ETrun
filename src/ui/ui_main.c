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

/*
=======================================================================

USER INTERFACE MAIN

=======================================================================
*/

#include "ui_local.h"

// NERVE - SMF
#define AXIS_TEAM       0
#define ALLIES_TEAM     1
#define SPECT_TEAM      2
// -NERVE - SMF

extern qboolean g_waitingForKey;
extern qboolean g_editingField;

uiInfo_t uiInfo;

static const char *MonthAbbrev[] =
{
	"Jan", "Feb", "Mar",
	"Apr", "May", "Jun",
	"Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec"
};

static const serverFilter_t serverFilters[] =
{
	{ "All", "" }
};

static const int numServerFilters = sizeof (serverFilters) / sizeof (serverFilter_t);

static char *netnames[] =
{
	"???",
	"UDP",
	"IPX",
	NULL
};

static int gamecodetoui[] = { 4, 2, 3, 0, 5, 1, 6 };
static int uitogamecode[] = { 4, 6, 2, 3, 1, 5, 7 };

// NERVE - SMF - enabled for multiplayer
static void UI_StartServerRefresh(qboolean full);
static void UI_StopServerRefresh(void);
static void UI_DoServerRefresh(void);
static void UI_FeederSelection(float feederID, int index);
qboolean UI_FeederSelectionClick(itemDef_t *item);
static void UI_BuildServerDisplayList(int force);
static void UI_BuildServerStatus(qboolean force);
static void UI_BuildFindPlayerList(qboolean force);
static int QDECL UI_ServersQsortCompare(const void *arg1, const void *arg2);
static const char *UI_SelectedMap(int index, int *actual);
qboolean    UI_CheckExecKey(int key);
// -NERVE - SMF - enabled for multiplayer

itemDef_t *Menu_FindItemByName(menuDef_t *menu, const char *p);
void Menu_ShowItemByName(menuDef_t *menu, const char *p, qboolean bShow);

// TTimo
static char translated_yes[4], translated_no[4];

extern displayContextDef_t *DC;

/*
================
vmMain

This is the only way control passes into the module.
================
*/
vmCvar_t ui_teamArenaFirstRun;

extern itemDef_t *g_bindItem;

void _UI_Init(void);
void _UI_Shutdown(void);
void _UI_KeyEvent(int key, qboolean down);
void _UI_MouseEvent(int dx, int dy);
void _UI_Refresh(int realtime);
qboolean _UI_IsFullscreen(void);

Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11) {
	// Nico, silent GCC
	(void)arg2;
	(void)arg3;
	(void)arg4;
	(void)arg5;
	(void)arg6;
	(void)arg7;
	(void)arg8;
	(void)arg9;
	(void)arg10;
	(void)arg11;

	switch (command) {
	case UI_GETAPIVERSION:
		return UI_API_VERSION;

	case UI_INIT:
		_UI_Init();
		return 0;

	case UI_SHUTDOWN:
		_UI_Shutdown();
		return 0;

	case UI_KEY_EVENT:
		_UI_KeyEvent(arg0, arg1);
		return 0;

	case UI_MOUSE_EVENT:
		_UI_MouseEvent(arg0, arg1);
		return 0;

	case UI_REFRESH:
		_UI_Refresh(arg0);
		return 0;

	case UI_IS_FULLSCREEN:
		return _UI_IsFullscreen();

	case UI_SET_ACTIVE_MENU:
		_UI_SetActiveMenu(arg0);
		return 0;

	case UI_GET_ACTIVE_MENU:
		return _UI_GetActiveMenu();

	case UI_CONSOLE_COMMAND:
		return UI_ConsoleCommand(arg0);

	case UI_DRAW_CONNECT_SCREEN:
		UI_DrawConnectScreen(arg0);
		return 0;
	case UI_HASUNIQUECDKEY:             // mod authors need to observe this
		return qtrue;
	// NERVE - SMF
	case UI_CHECKEXECKEY:
		return UI_CheckExecKey(arg0);
	case UI_WANTSBINDKEYS:
		return (g_waitingForKey && g_bindItem) ? qtrue : qfalse;
	}

	return -1;
}

void AssetCache() {
	int n;

	uiInfo.uiDC.Assets.gradientBar         = trap_R_RegisterShaderNoMip(ASSET_GRADIENTBAR);
	uiInfo.uiDC.Assets.fxBasePic           = trap_R_RegisterShaderNoMip(ART_FX_BASE);
	uiInfo.uiDC.Assets.fxPic[0]            = trap_R_RegisterShaderNoMip(ART_FX_RED);
	uiInfo.uiDC.Assets.fxPic[1]            = trap_R_RegisterShaderNoMip(ART_FX_YELLOW);
	uiInfo.uiDC.Assets.fxPic[2]            = trap_R_RegisterShaderNoMip(ART_FX_GREEN);
	uiInfo.uiDC.Assets.fxPic[3]            = trap_R_RegisterShaderNoMip(ART_FX_TEAL);
	uiInfo.uiDC.Assets.fxPic[4]            = trap_R_RegisterShaderNoMip(ART_FX_BLUE);
	uiInfo.uiDC.Assets.fxPic[5]            = trap_R_RegisterShaderNoMip(ART_FX_CYAN);
	uiInfo.uiDC.Assets.fxPic[6]            = trap_R_RegisterShaderNoMip(ART_FX_WHITE);
	uiInfo.uiDC.Assets.scrollBar           = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR);
	uiInfo.uiDC.Assets.scrollBarArrowDown  = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWDOWN);
	uiInfo.uiDC.Assets.scrollBarArrowUp    = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWUP);
	uiInfo.uiDC.Assets.scrollBarArrowLeft  = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWLEFT);
	uiInfo.uiDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWRIGHT);
	uiInfo.uiDC.Assets.scrollBarThumb      = trap_R_RegisterShaderNoMip(ASSET_SCROLL_THUMB);
	uiInfo.uiDC.Assets.sliderBar           = trap_R_RegisterShaderNoMip(ASSET_SLIDER_BAR);
	uiInfo.uiDC.Assets.sliderThumb         = trap_R_RegisterShaderNoMip(ASSET_SLIDER_THUMB);
	uiInfo.uiDC.Assets.checkboxCheck       = trap_R_RegisterShaderNoMip(ASSET_CHECKBOX_CHECK);
	uiInfo.uiDC.Assets.checkboxCheckNot    = trap_R_RegisterShaderNoMip(ASSET_CHECKBOX_CHECK_NOT);
	uiInfo.uiDC.Assets.checkboxCheckNo     = trap_R_RegisterShaderNoMip(ASSET_CHECKBOX_CHECK_NO);

	for (n = 0; n < NUM_CROSSHAIRS; ++n) {
		uiInfo.uiDC.Assets.crosshairShader[n]    = trap_R_RegisterShaderNoMip(va("gfx/2d/crosshair%c", 'a' + n));
		uiInfo.uiDC.Assets.crosshairAltShader[n] = trap_R_RegisterShaderNoMip(va("gfx/2d/crosshair%c_alt", 'a' + n));
	}
}

void _UI_DrawSides(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640(&x, &y, &w, &h);
	size *= uiInfo.uiDC.xscale;
	trap_R_DrawStretchPic(x, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
	trap_R_DrawStretchPic(x + w - size, y, size, h, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
}

void _UI_DrawTopBottom(float x, float y, float w, float h, float size) {
	UI_AdjustFrom640(&x, &y, &w, &h);
	size *= uiInfo.uiDC.yscale;
	trap_R_DrawStretchPic(x, y, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
	trap_R_DrawStretchPic(x, y + h - size, w, size, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);
}
/*
================
UI_DrawRect

Coordinates are 640*480 virtual values
=================
*/
void _UI_DrawRect(float x, float y, float width, float height, float size, const float *color) {
	trap_R_SetColor(color);

	_UI_DrawTopBottom(x, y, width, height, size);
	_UI_DrawSides(x, y, width, height, size);

	trap_R_SetColor(NULL);
}

// NERVE - SMF
void Text_SetActiveFont(int font) {
	uiInfo.activeFont = font;
}

int Text_Width_Ext(const char *text, float scale, int limit, fontInfo_t *font) {
	float      out = 0;
	const char *s  = text;

	if (text) {
		int count, len;

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
				glyphInfo_t *glyph = &font->glyphs[(unsigned char)*s];

				out += glyph->xSkip;
				s++;
				count++;
			}
		}
	}
	return out * scale * font->glyphScale;
}

int Text_Width(const char *text, float scale, int limit) {
	fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[uiInfo.activeFont];

	return Text_Width_Ext(text, scale, limit, font);
}

int Multiline_Text_Width(const char *text, float scale, int limit) {
	float      out = 0;
	float      width, widest = 0;
	const char *s    = text;
	fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[uiInfo.activeFont];

	if (text) {
		int count, len;

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
				if (*s == '\n') {
					width = out * scale * font->glyphScale;
					if (width > widest) {
						widest = width;
					}
					out = 0;
				} else {
					glyphInfo_t *glyph = &font->glyphs[(unsigned char)*s];

					out += glyph->xSkip;
				}
				s++;
				count++;
			}
		}
	}

	if (widest > 0) {
		width = out * scale * font->glyphScale;
		if (width > widest) {
			widest = width;
		}

		return widest;
	}
	return out * scale * font->glyphScale;
}

int Text_Height_Ext(const char *text, float scale, int limit, fontInfo_t *font) {
	float       max;
	glyphInfo_t *glyph;
	const char  *s = text;

	max = 0;
	if (text) {
		int len, count;

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
				glyph = &font->glyphs[(unsigned char)*s];           // NERVE - SMF - this needs to be an unsigned cast for localization
				if (max < glyph->height) {
					max = glyph->height;
				}
				s++;
				count++;
			}
		}
	}

	return max * scale * font->glyphScale;
}

int Text_Height(const char *text, float scale, int limit) {
	fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[uiInfo.activeFont];

	return Text_Height_Ext(text, scale, limit, font);
}

int Multiline_Text_Height(const char *text, float scale, int limit) {
	float       max;
	float       totalheight = 0;
	glyphInfo_t *glyph;
	const char  *s    = text;
	fontInfo_t  *font = &uiInfo.uiDC.Assets.fonts[uiInfo.activeFont];

	max = 0;
	if (text) {
		int len, count;

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
				if (*s == '\n') {
					if (!totalheight) {
						totalheight += 5;   // 5 is the vertical spacing that autowrap painting uses
					}
					totalheight += max;
					max          = 0;
				} else {
					glyph = &font->glyphs[(unsigned char)*s];           // NERVE - SMF - this needs to be an unsigned cast for localization
					if (max < glyph->height) {
						max = glyph->height;
					}
				}
				s++;
				count++;
			}
		}
	}

	if (totalheight > 0) {
		if (!totalheight) {
			totalheight += 5;   // 5 is the vertical spacing that autowrap painting uses
		}
		totalheight += max;
		return totalheight * scale * font->glyphScale;
	}
	return max * scale * font->glyphScale;
}

void Text_PaintCharExt(float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2, qhandle_t hShader) {
	w *= scalex;
	h *= scaley;
	UI_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

void Text_PaintChar(float x, float y, float w, float h, float scale, float s, float t, float s2, float t2, qhandle_t hShader) {
	w *= scale;
	h *= scale;
	UI_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s, t, s2, t2, hShader);
}

void Text_Paint_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *font) {
	vec4_t newColor;

	scalex *= font->glyphScale;
	scaley *= font->glyphScale;

	if (text) {
		const char *s = text;
		int        len, count;

		trap_R_SetColor(color);
		memcpy(&newColor[0], &color[0], sizeof (vec4_t));
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count = 0;
		while (s && *s && count < len) {
			glyphInfo_t *glyph;
			int         index = (unsigned char)*s;

			// NERVE - SMF - don't draw tabs and newlines
			if (index < 20) {
				s++;
				count++;
				continue;
			}

			glyph = &font->glyphs[index];
			if (Q_IsColorString(s)) {
				if (*(s + 1) == COLOR_NULL) {
					memcpy(&newColor[0], &color[0], sizeof (vec4_t));
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
					Text_PaintCharExt(x + (glyph->pitch * scalex) + ofs, y - yadj + ofs, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);
					trap_R_SetColor(newColor);

					colorBlack[3] = 1.0;
				}
				Text_PaintCharExt(x + (glyph->pitch * scalex), y - yadj, glyph->imageWidth, glyph->imageHeight, scalex, scaley, glyph->s, glyph->t, glyph->s2, glyph->t2, glyph->glyph);

				x += (glyph->xSkip * scalex) + adjust;
				s++;
				count++;
			}
		}
		trap_R_SetColor(NULL);
	}
}

void Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style) {
	fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[uiInfo.activeFont];

	Text_Paint_Ext(x, y, scale, scale, color, text, adjust, limit, style, font);
}

void Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
	vec4_t     newColor;
	float      useScale;
	fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[uiInfo.activeFont];

	useScale = scale * font->glyphScale;

	if (text) {
		const char  *s = text;
		int         len, count;
		glyphInfo_t *glyph2;
		float       yadj;

		trap_R_SetColor(color);
		memcpy(&newColor[0], &color[0], sizeof (vec4_t));
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		count  = 0;
		glyph2 = &font->glyphs[(unsigned char)cursor];
		while (s && *s && count < len) {
			glyphInfo_t *glyph = &font->glyphs[(unsigned char)*s];           // NERVE - SMF - this needs to be an unsigned cast for localization

			{
				yadj = useScale * glyph->top;
				if (style == ITEM_TEXTSTYLE_SHADOWED || style == ITEM_TEXTSTYLE_SHADOWEDMORE) {
					int ofs = style == ITEM_TEXTSTYLE_SHADOWED ? 1 : 2;
					colorBlack[3] = newColor[3];
					trap_R_SetColor(colorBlack);
					Text_PaintChar(x + (glyph->pitch * useScale) + ofs, y - yadj + ofs,
					               glyph->imageWidth,
					               glyph->imageHeight,
					               useScale,
					               glyph->s,
					               glyph->t,
					               glyph->s2,
					               glyph->t2,
					               glyph->glyph);
					colorBlack[3] = 1.0;
					trap_R_SetColor(newColor);
				}
				Text_PaintChar(x + (glyph->pitch * useScale), y - yadj,
				               glyph->imageWidth,
				               glyph->imageHeight,
				               useScale,
				               glyph->s,
				               glyph->t,
				               glyph->s2,
				               glyph->t2,
				               glyph->glyph);

				yadj = useScale * glyph2->top;
				if (count == cursorPos && !((uiInfo.uiDC.realTime / BLINK_DIVISOR) & 1)) {
					Text_PaintChar(x + (glyph->pitch * useScale), y - yadj,
					               glyph2->imageWidth,
					               glyph2->imageHeight,
					               useScale,
					               glyph2->s,
					               glyph2->t,
					               glyph2->s2,
					               glyph2->t2,
					               glyph2->glyph);
				}

				x += (glyph->xSkip * useScale);
				s++;
				count++;
			}
		}
		// need to paint cursor at end of text
		if (cursorPos == len && !((uiInfo.uiDC.realTime / BLINK_DIVISOR) & 1)) {
			yadj = useScale * glyph2->top;
			Text_PaintChar(x + (glyph2->pitch * useScale), y - yadj,
			               glyph2->imageWidth,
			               glyph2->imageHeight,
			               useScale,
			               glyph2->s,
			               glyph2->t,
			               glyph2->s2,
			               glyph2->t2,
			               glyph2->glyph);

		}

		trap_R_SetColor(NULL);
	}
}

static void Text_Paint_Limit(float *maxX, float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit) {
	if (text) {
		const char *s = text;
		float      max = *maxX;
		fontInfo_t *font = &uiInfo.uiDC.Assets.fonts[uiInfo.activeFont];
		float      useScale = scale * font->glyphScale;
		int        len, count = 0;
		vec4_t     newColor;

		trap_R_SetColor(color);
		len = strlen(text);
		if (limit > 0 && len > limit) {
			len = limit;
		}
		while (s && *s && count < len) {
			glyphInfo_t *glyph = &font->glyphs[(unsigned char)*s];           // NERVE - SMF - this needs to be an unsigned cast for localization

			if (Q_IsColorString(s)) {
				if (*(s + 1) == COLOR_NULL) {
					memcpy(&newColor[0], &color[0], sizeof (vec4_t));
				} else {
					memcpy(newColor, g_color_table[ColorIndex(*(s + 1))], sizeof (newColor));
					newColor[3] = color[3];
				}
				trap_R_SetColor(newColor);
				s += 2;
				continue;
			} else {
				float yadj = useScale * glyph->top;
				if (Text_Width(s, useScale, 1) + x > max) {
					*maxX = 0;
					break;
				}
				Text_PaintChar(x + (glyph->pitch * useScale), y - yadj,
				               glyph->imageWidth,
				               glyph->imageHeight,
				               useScale,
				               glyph->s,
				               glyph->t,
				               glyph->s2,
				               glyph->t2,
				               glyph->glyph);
				x    += (glyph->xSkip * useScale) + adjust;
				*maxX = x;
				count++;
				s++;
			}
		}
		trap_R_SetColor(NULL);
	}
}

/*
=================
_UI_Refresh
=================
*/

int frameCount = 0;
int startTime;

#define UI_FPS_FRAMES   4
void _UI_Refresh(int realtime) {
	static int index;
	static int previousTimes[UI_FPS_FRAMES];

	uiInfo.uiDC.frameTime = realtime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime  = realtime;

	previousTimes[index % UI_FPS_FRAMES] = uiInfo.uiDC.frameTime;
	index++;
	if (index > UI_FPS_FRAMES) {
		int i, total;
		// average multiple frames together to smooth changes out a bit
		total = 0;
		for (i = 0 ; i < UI_FPS_FRAMES ; ++i) {
			total += previousTimes[i];
		}
		if (!total) {
			total = 1;
		}
		uiInfo.uiDC.FPS = (int)(1000 * UI_FPS_FRAMES / total);
	}

	UI_UpdateCvars();

	if (trap_Cvar_VariableValue("ui_connecting")) {
		UI_DrawLoadPanel(qfalse, qtrue);
		if (!trap_Cvar_VariableValue("ui_connecting")) {
			trap_Cvar_Set("ui_connecting", "1");
		}
		return;
	}

	// OSP - blackout if speclocked
	if (ui_blackout.integer > 0) {
		UI_FillRect(-10, -10, 650, 490, colorBlack);
	}

	if (Menu_Count() > 0) {
		// paint all the menus
		Menu_PaintAll();
		// refresh server browser list
		UI_DoServerRefresh();
		// refresh server status
		UI_BuildServerStatus(qfalse);
		// refresh find player list
		UI_BuildFindPlayerList(qfalse);
	}

	// draw cursor
	UI_SetColor(NULL);
	if (Menu_Count() > 0) {
		uiClientState_t cstate;
		trap_GetClientState(&cstate);
		if (cstate.connState <= CA_DISCONNECTED || cstate.connState >= CA_ACTIVE) {
			UI_DrawHandlePic(uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory, 32, 32, uiInfo.uiDC.Assets.cursor);
		}
	}
}

/*
=================
_UI_Shutdown
=================
*/
void _UI_Shutdown(void) {
	trap_LAN_SaveCachedServers();
}

qboolean Asset_Parse(int handle) {
	pc_token_t token;
	const char *tempStr;

	if (!trap_PC_ReadToken(handle, &token)) {
		return qfalse;
	}
	if (Q_stricmp(token.string, "{") != 0) {
		return qfalse;
	}

	for (;;) {
		memset(&token, 0, sizeof (pc_token_t));

		if (!trap_PC_ReadToken(handle, &token)) {
			return qfalse;
		}

		if (Q_stricmp(token.string, "}") == 0) {
			return qtrue;
		}

		// font
		if (Q_stricmp(token.string, "font") == 0) {
			int pointSize, fontIndex;
			if (!PC_Int_Parse(handle, &fontIndex) || !PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize)) {
				return qfalse;
			}
			if (fontIndex < 0 || fontIndex >= 6) {
				return qfalse;
			}
			trap_R_RegisterFont(tempStr, pointSize, &uiInfo.uiDC.Assets.fonts[fontIndex]);
			continue;
		}

		// gradientbar
		if (Q_stricmp(token.string, "gradientbar") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if (Q_stricmp(token.string, "menuEnterSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuEnterSound = trap_S_RegisterSound(tempStr);
			continue;
		}

		// exitMenuSound
		if (Q_stricmp(token.string, "menuExitSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuExitSound = trap_S_RegisterSound(tempStr);
			continue;
		}

		// itemFocusSound
		if (Q_stricmp(token.string, "itemFocusSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.itemFocusSound = trap_S_RegisterSound(tempStr);
			continue;
		}

		// menuBuzzSound
		if (Q_stricmp(token.string, "menuBuzzSound") == 0) {
			if (!PC_String_Parse(handle, &tempStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.menuBuzzSound = trap_S_RegisterSound(tempStr);
			continue;
		}

		if (Q_stricmp(token.string, "cursor") == 0) {
			if (!PC_String_Parse(handle, &uiInfo.uiDC.Assets.cursorStr)) {
				return qfalse;
			}
			uiInfo.uiDC.Assets.cursor = trap_R_RegisterShaderNoMip(uiInfo.uiDC.Assets.cursorStr);
			continue;
		}

		if (Q_stricmp(token.string, "fadeClamp") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeClamp)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeCycle") == 0) {
			if (!PC_Int_Parse(handle, &uiInfo.uiDC.Assets.fadeCycle)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "fadeAmount") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.fadeAmount)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowX") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowX)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowY") == 0) {
			if (!PC_Float_Parse(handle, &uiInfo.uiDC.Assets.shadowY)) {
				return qfalse;
			}
			continue;
		}

		if (Q_stricmp(token.string, "shadowColor") == 0) {
			if (!PC_Color_Parse(handle, &uiInfo.uiDC.Assets.shadowColor)) {
				return qfalse;
			}
			continue;
		}

	}
}

static void QDECL Com_DPrintf(const char *fmt, ...) {
	va_list argptr;
	char    msg[4096];
	int     developer;

	developer = trap_Cvar_VariableValue("developer");
	if (!developer) {
		return;
	}

	va_start(argptr, fmt);
	Q_vsnprintf(msg, sizeof (msg), fmt, argptr);
	va_end(argptr);

	Com_Printf("%s", msg);
}

qboolean UI_ParseMenu(const char *menuFile) {
	int        handle;
	pc_token_t token;

	Com_DPrintf("Parsing menu file: %s\n", menuFile);

	handle = trap_PC_LoadSource(menuFile);
	if (!handle) {
		return qfalse;
	}

	for (;;) {
		memset(&token, 0, sizeof (pc_token_t));
		if (!trap_PC_ReadToken(handle, &token)) {
			break;
		}

		if (token.string[0] == '}') {
			break;
		}

		if (Q_stricmp(token.string, "assetGlobalDef") == 0) {
			if (Asset_Parse(handle)) {
				continue;
			} else {
				break;
			}
		}

		if (Q_stricmp(token.string, "menudef") == 0) {
			// start a new menu
			Menu_New(handle);
		}
	}
	trap_PC_FreeSource(handle);
	return qtrue;
}

qboolean Load_Menu(int handle) {
	pc_token_t token;

	if (!trap_PC_ReadToken(handle, &token)) {
		return qfalse;
	}
	if (token.string[0] != '{') {
		return qfalse;
	}

	for (;;) {
		if (!trap_PC_ReadToken(handle, &token)) {
			return qfalse;
		}

		if (token.string[0] == 0) {
			return qfalse;
		}

		if (token.string[0] == '}') {
			return qtrue;
		}

		UI_ParseMenu(token.string);
	}
}

void UI_LoadMenus(const char *menuFile, qboolean reset) {
	pc_token_t      token;
	int             handle;
	int             start;
	uiClientState_t cstate;

	start = trap_Milliseconds();
	trap_GetClientState(&cstate);

	if (cstate.connState <= CA_DISCONNECTED) {
		trap_PC_AddGlobalDefine("FUI");
	}

	handle = trap_PC_LoadSource(menuFile);
	if (!handle) {
		trap_Error(va(S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile));
		handle = trap_PC_LoadSource("ui/menus.txt");
		if (!handle) {
			trap_Error(S_COLOR_RED "default menu file not found: ui_mp/menus.txt, unable to continue!\n");
		}
	}

	if (reset) {
		Menu_Reset();
	}

	for (;;) {
		if (!trap_PC_ReadToken(handle, &token)) {
			break;
		}
		if (token.string[0] == 0 || token.string[0] == '}') {
			break;
		}

		if (token.string[0] == '}') {
			break;
		}

		if (Q_stricmp(token.string, "loadmenu") == 0) {
			if (Load_Menu(handle)) {
				continue;
			} else {
				break;
			}
		}
	}

	Com_DPrintf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);

	trap_PC_FreeSource(handle);
}

void UI_Load() {
	char      lastName[1024];
	menuDef_t *menu    = Menu_GetFocused();
	char      *menuSet = UI_Cvar_VariableString("ui_menuFiles");

	if (menu && menu->window.name) {
		Q_strncpyz(lastName, menu->window.name, sizeof (lastName));
	}
	if (menuSet == NULL || menuSet[0] == '\0') {
		menuSet = "ui/menus.txt";
	}

	String_Init();

	UI_LoadArenas();
	UI_LoadMenus(menuSet, qtrue);
	Menus_CloseAll();
	Menus_ActivateByName(lastName, qtrue);
}

static void UI_DrawEffects(rectDef_t *rect) {
	UI_DrawHandlePic(rect->x, rect->y, 128, 8, uiInfo.uiDC.Assets.fxBasePic);
	UI_DrawHandlePic(rect->x + uiInfo.effectsColor * 16 + 8, rect->y, 16, 12, uiInfo.uiDC.Assets.fxPic[uiInfo.effectsColor]);
}

void UI_DrawMapPreview(rectDef_t *rect, float scale, qboolean net) {
	int map = (net) ? ui_currentNetMap.integer : ui_currentMap.integer;

	if (map < 0 || map > uiInfo.mapCount) {
		if (net) {
			ui_currentNetMap.integer = 0;
			trap_Cvar_Set("ui_currentNetMap", "0");
		} else {
			ui_currentMap.integer = 0;
			trap_Cvar_Set("ui_currentMap", "0");
		}
		map = 0;
	}

	if (uiInfo.mapList[map].mappos[0] && uiInfo.mapList[map].mappos[1]) {
		float  x, y, w, h;
		vec2_t tl, br;
		vec4_t colourFadedBlack = { 0.f, 0.f, 0.f, 0.4f };

		tl[0] = uiInfo.mapList[map].mappos[0] - .5 * 650.f;
		if (tl[0] < 0) {
			tl[0] = 0;
		}
		br[0] = tl[0] + 650.f;
		if (br[0] > 1024.f) {
			br[0] = 1024.f;
			tl[0] = br[0] - 650.f;
		}

		tl[1] = uiInfo.mapList[map].mappos[1] - .5 * 650.f;
		if (tl[1] < 0) {
			tl[1] = 0;
		}
		br[1] = tl[1] + 650.f;
		if (br[1] > 1024.f) {
			br[1] = 1024.f;
			tl[1] = br[1] - 650.f;
		}

		x = rect->x;
		y = rect->y;
		w = rect->w;
		h = rect->h;
		UI_AdjustFrom640(&x, &y, &w, &h);
		trap_R_DrawStretchPic(x, y, w, h,
		                      tl[0] / 1024.f,
		                      tl[1] / 1024.f,
		                      br[0] / 1024.f,
		                      br[1] / 1024.f,
		                      uiInfo.campaignMap);

		x = rect->x + ((uiInfo.mapList[map].mappos[0] - tl[0]) / 650.f * rect->w);
		y = rect->y + ((uiInfo.mapList[map].mappos[1] - tl[1]) / 650.f * rect->h);

		w = Text_Width(uiInfo.mapList[map].mapName, scale, 0);

		if (x + 10 + w > rect->x + rect->w) {
			UI_FillRect(x - w - 12 + 1, y - 6 + 1, 12 + w, 12, colourFadedBlack);
			UI_FillRect(x - w - 12, y - 6, 12 + w, 12, colorBlack);
		} else {
			UI_FillRect(x + 1, y - 6 + 1, 12 + w, 12, colourFadedBlack);
			UI_FillRect(x, y - 6, 12 + w, 12, colorBlack);
		}

		UI_DrawHandlePic(x - 8, y - 8, 16, 16, trap_R_RegisterShaderNoMip("gfx/loading/pin_neutral"));

		if (x + 10 + w > rect->x + rect->w) {
			Text_Paint(x - w - 10, y + 3, scale, colorWhite, uiInfo.mapList[map].mapName, 0, 0, 0);
		} else {
			Text_Paint(x + 10, y + 3, scale, colorWhite, uiInfo.mapList[map].mapName, 0, 0, 0);
		}
	} else {
		UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip("levelshots/unknownmap"));
	}
}

void UI_DrawNetMapPreview(rectDef_t *rect) {
	if (uiInfo.serverStatus.currentServerPreview > 0) {
		UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, uiInfo.serverStatus.currentServerPreview);
	} else {
		UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip("levelshots/unknownmap"));
	}
}

static void UI_DrawMissionBriefingMap(rectDef_t *rect) {
	static qhandle_t image = -1;

	if (image == -1) {
		char buffer[64];

		trap_Cvar_VariableStringBuffer("mapname", buffer, 64);
		image = trap_R_RegisterShaderNoMip(va("levelshots/%s_cc.tga", buffer));
	}

	if (image) {
		UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, image);
	} else {
		UI_DrawHandlePic(rect->x, rect->y, rect->w, rect->h, trap_R_RegisterShaderNoMip("levelshots/unknownmap"));
	}
}

static void UI_DrawMissionBriefingTitle(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	char    buffer[64];
	mapInfo *mi;

	trap_Cvar_VariableStringBuffer("mapname", buffer, 64);

	mi = UI_FindMapInfoByMapname(buffer);
	if (!mi) {
		return;
	}

	Text_Paint(rect->x, rect->y, scale, color, va("%s Objectives", mi->mapName), 0, 0, textStyle);
}

static void UI_DrawMissionBriefingObjectives(rectDef_t *rect, float scale, vec4_t color, float text_x, float text_y, int textStyle, int align) {
	const char *p, *textPtr, *newLinePtr;
	char       buff[1024];
	int        height, len, textWidth, newLine, newLineWidth;
	float      y;
	rectDef_t  textRect;

	char    buffer[64];
	mapInfo *mi;

	trap_Cvar_VariableStringBuffer("mapname", buffer, 64);

	mi = UI_FindMapInfoByMapname(buffer);
	if (!mi) {
		return;
	}

	textPtr = mi->objectives;

	height = Text_Height(textPtr, scale, 0);

	textRect.x = 0;
	textRect.y = 0;
	textRect.w = rect->w;
	textRect.h = rect->h;

	textWidth    = 0;
	newLinePtr   = NULL;
	y            = text_y;
	len          = 0;
	buff[0]      = '\0';
	newLine      = 0;
	newLineWidth = 0;
	p            = textPtr;
	while (p) {
		if (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\0') {
			newLine      = len;
			newLinePtr   = p + 1;
			newLineWidth = textWidth;
		}
		textWidth = Text_Width(buff, scale, 0);
		if ((newLine && textWidth > rect->w) || *p == '\n' || *p == '\0') {
			if (len) {
				if (align == ITEM_ALIGN_LEFT) {
					textRect.x = text_x;
				} else if (align == ITEM_ALIGN_RIGHT) {
					textRect.x = text_x - newLineWidth;
				} else if (align == ITEM_ALIGN_CENTER) {
					textRect.x = (float)(text_x - newLineWidth / 2);
				}
				textRect.y = y;

				textRect.x += rect->x;
				textRect.y += rect->y;

				//
				buff[newLine] = '\0';
				Text_Paint(textRect.x, textRect.y, scale, color, buff, 0, 0, textStyle);
			}
			if (*p == '\0') {
				break;
			}
			//
			y           += height + 5;
			p            = newLinePtr;
			len          = 0;
			newLine      = 0;
			newLineWidth = 0;
			continue;
		}
		buff[len++] = *p++;

		if (buff[len - 1] == 13) {
			buff[len - 1] = ' ';
		}

		buff[len] = '\0';
	}
}

static void UI_DrawNetFilter(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	if (ui_serverFilterType.integer < 0 || ui_serverFilterType.integer > numServerFilters) {
		ui_serverFilterType.integer = 0;
	}
	Text_Paint(rect->x, rect->y, scale, color, va("Filter: %s", serverFilters[ui_serverFilterType.integer].description), 0, 0, textStyle);
}

static int UI_OwnerDrawWidth(int ownerDraw, float scale) {
	const char *s = NULL;

	switch (ownerDraw) {
	case UI_NETFILTER:
		if (ui_serverFilterType.integer < 0 || ui_serverFilterType.integer > numServerFilters) {
			ui_serverFilterType.integer = 0;
		}
		s = va("Filter: %s", serverFilters[ui_serverFilterType.integer].description);
		break;
	case UI_KEYBINDSTATUS:
		if (Display_KeyBindPending()) {
			s = trap_TranslateString("Waiting for new key... Press ESCAPE to cancel");
		} else {
			s = trap_TranslateString("Press ENTER or CLICK to change, Press BACKSPACE to clear");
		}
		break;
	case UI_SERVERREFRESHDATE:
		s = UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource.integer));
		break;
	default:
		break;
	}

	if (s) {
		return Text_Width(s, scale, 0);
	}
	return 0;
}

static void UI_DrawRedBlue(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	Text_Paint(rect->x, rect->y, scale, color, (uiInfo.redBlue == 0) ? "Red" : "Blue", 0, 0, textStyle);
}

static void UI_DrawCrosshair(rectDef_t *rect) {
	float size = cg_crosshairSize.integer;

	if (uiInfo.currentCrosshair < 0 || uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
		uiInfo.currentCrosshair = 0;
	}

	size = (rect->w / 96.0f) * ((size > 96.0f) ? 96.0f : ((size < 24.0f) ? 24.0f : size));

	trap_R_SetColor(uiInfo.xhairColor);
	UI_DrawHandlePic(rect->x + (rect->w - size) / 2, rect->y + (rect->h - size) / 2, size, size, uiInfo.uiDC.Assets.crosshairShader[uiInfo.currentCrosshair]);
	trap_R_SetColor(uiInfo.xhairColorAlt);
	UI_DrawHandlePic(rect->x + (rect->w - size) / 2, rect->y + (rect->h - size) / 2, size, size, uiInfo.uiDC.Assets.crosshairAltShader[uiInfo.currentCrosshair]);

	trap_R_SetColor(NULL);
}

/*
===============
UI_BuildPlayerList
===============
*/
static void UI_BuildPlayerList() {
	uiClientState_t cs;
	int             n, count, team, team2, playerTeamNumber, muted;
	char            info[MAX_INFO_STRING];
	char            namebuf[64];

	trap_GetClientState(&cs);
	trap_GetConfigString(CS_PLAYERS + cs.clientNum, info, MAX_INFO_STRING);
	uiInfo.playerNumber = cs.clientNum;
	uiInfo.teamLeader   = atoi(Info_ValueForKey(info, "tl"));
	team                = atoi(Info_ValueForKey(info, "t"));
	trap_GetConfigString(CS_SERVERINFO, info, sizeof (info));
	count              = atoi(Info_ValueForKey(info, "sv_maxclients"));
	uiInfo.playerCount = 0;
	uiInfo.myTeamCount = 0;
	playerTeamNumber   = 0;
	for (n = 0; n < count; ++n) {
		trap_GetConfigString(CS_PLAYERS + n, info, MAX_INFO_STRING);

		if (info[0]) {
			Q_strncpyz(namebuf, Info_ValueForKey(info, "n"), sizeof (namebuf));
// fretn - dont expand colors twice, so: foo^^xbar -> foo^bar -> fooar
			Q_strncpyz(uiInfo.playerNames[uiInfo.playerCount], namebuf, sizeof (uiInfo.playerNames[0]));
			muted = atoi(Info_ValueForKey(info, "mu"));
			if (muted) {
				uiInfo.playerMuted[uiInfo.playerCount] = qtrue;
			} else {
				uiInfo.playerMuted[uiInfo.playerCount] = qfalse;
			}
			uiInfo.playerRefereeStatus[uiInfo.playerCount] = atoi(Info_ValueForKey(info, "ref"));
			uiInfo.playerCount++;
			team2 = atoi(Info_ValueForKey(info, "t"));
			if (team2 == team) {
				Q_strncpyz(namebuf, Info_ValueForKey(info, "n"), sizeof (namebuf));
// fretn - dont expand colors twice, so: foo^^xbar -> foo^bar -> fooar
				Q_strncpyz(uiInfo.teamNames[uiInfo.myTeamCount], namebuf, sizeof (uiInfo.teamNames[0]));
				uiInfo.teamClientNums[uiInfo.myTeamCount] = n;
				if (uiInfo.playerNumber == n) {
					playerTeamNumber = uiInfo.myTeamCount;
				}
				uiInfo.myTeamCount++;
			}
		}
	}

	if (!uiInfo.teamLeader) {
		trap_Cvar_Set("cg_selectedPlayer", va("%d", playerTeamNumber));
	}

	n = trap_Cvar_VariableValue("cg_selectedPlayer");
	if (n < 0 || n > uiInfo.myTeamCount) {
		n = 0;
	}
	if (n < uiInfo.myTeamCount) {
		trap_Cvar_Set("cg_selectedPlayerName", uiInfo.teamNames[n]);
	}
}

static void UI_DrawSelectedPlayer(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
		uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
		UI_BuildPlayerList();
	}
	Text_Paint(rect->x, rect->y, scale, color, (uiInfo.teamLeader) ? UI_Cvar_VariableString("cg_selectedPlayerName") : UI_Cvar_VariableString("name"), 0, 0, textStyle);
}

static void UI_DrawServerRefreshDate(rectDef_t *rect, float scale, vec4_t color, int textStyle) {
	if (uiInfo.serverStatus.refreshActive) {
		vec4_t lowLight, newColor;
		int    serverCount;

		lowLight[0] = 0.8 * color[0];
		lowLight[1] = 0.8 * color[1];
		lowLight[2] = 0.8 * color[2];
		lowLight[3] = 0.8 * color[3];
		LerpColor(color, lowLight, newColor, 0.5 + 0.5 * sin((float)(uiInfo.uiDC.realTime / PULSE_DIVISOR)));
		// NERVE - SMF
		serverCount = trap_LAN_GetServerCount(ui_netSource.integer);
		if (serverCount >= 0) {
			Text_Paint(rect->x, rect->y, scale, newColor, va(trap_TranslateString("Getting info for %d servers (ESC to cancel)"), serverCount), 0, 0, textStyle);
		} else {
			Text_Paint(rect->x, rect->y, scale, newColor, trap_TranslateString("Waiting for response from Master Server"), 0, 0, textStyle);
		}
	} else {
		char buff[64];

		Q_strncpyz(buff, UI_Cvar_VariableString(va("ui_lastServerRefresh_%i", ui_netSource.integer)), 64);
		Text_Paint(rect->x, rect->y, scale, color, va(trap_TranslateString("Refresh Time: %s"), buff), 0, 0, textStyle);
	}
}

static void UI_DrawServerMOTD(rectDef_t *rect, float scale, vec4_t color) {
	if (uiInfo.serverStatus.motdLen) {
		float maxX;

		if (uiInfo.serverStatus.motdWidth == -1) {
			uiInfo.serverStatus.motdWidth   = 0;
			uiInfo.serverStatus.motdPaintX  = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if (uiInfo.serverStatus.motdOffset > uiInfo.serverStatus.motdLen) {
			uiInfo.serverStatus.motdOffset  = 0;
			uiInfo.serverStatus.motdPaintX  = rect->x + 1;
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

		if (uiInfo.uiDC.realTime > uiInfo.serverStatus.motdTime) {
			uiInfo.serverStatus.motdTime = uiInfo.uiDC.realTime + 10;
			if (uiInfo.serverStatus.motdPaintX <= rect->x + 2) {
				if (uiInfo.serverStatus.motdOffset < uiInfo.serverStatus.motdLen) {
					uiInfo.serverStatus.motdPaintX += Text_Width(&uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], scale, 1) - 1;
					uiInfo.serverStatus.motdOffset++;
				} else {
					uiInfo.serverStatus.motdOffset = 0;
					if (uiInfo.serverStatus.motdPaintX2 >= 0) {
						uiInfo.serverStatus.motdPaintX = uiInfo.serverStatus.motdPaintX2;
					} else {
						uiInfo.serverStatus.motdPaintX = rect->x + rect->w - 2;
					}
					uiInfo.serverStatus.motdPaintX2 = -1;
				}
			} else {
				uiInfo.serverStatus.motdPaintX -= 2;
				if (uiInfo.serverStatus.motdPaintX2 >= 0) {
					uiInfo.serverStatus.motdPaintX2 -= 2;
				}
			}
		}

		maxX = rect->x + rect->w - 2;
		Text_Paint_Limit(&maxX, uiInfo.serverStatus.motdPaintX, rect->y, scale, color, &uiInfo.serverStatus.motd[uiInfo.serverStatus.motdOffset], 0, 0);
		if (uiInfo.serverStatus.motdPaintX2 >= 0) {
			float maxX2 = rect->x + rect->w - 2;
			Text_Paint_Limit(&maxX2, uiInfo.serverStatus.motdPaintX2, rect->y, scale, color, uiInfo.serverStatus.motd, 0, uiInfo.serverStatus.motdOffset);
		}
		if (uiInfo.serverStatus.motdOffset && maxX > 0) {
			// if we have an offset ( we are skipping the first part of the string ) and we fit the string
			if (uiInfo.serverStatus.motdPaintX2 == -1) {
				uiInfo.serverStatus.motdPaintX2 = rect->x + rect->w - 2;
			}
		} else {
			uiInfo.serverStatus.motdPaintX2 = -1;
		}

	}
}

static void UI_DrawKeyBindStatus(rectDef_t *rect, float scale, vec4_t color, int textStyle, float text_x, float text_y) {
	if (Display_KeyBindPending()) {
		Text_Paint(rect->x + text_x, rect->y + text_y, scale, color, trap_TranslateString("Waiting for new key... Press ESCAPE to cancel"), 0, 0, textStyle);
	} else {
		Text_Paint(rect->x + text_x, rect->y + text_y, scale, color, trap_TranslateString("Press ENTER or CLICK to change, Press BACKSPACE to clear"), 0, 0, textStyle);
	}
}

static void UI_ParseGLConfig(void) {
	char *eptr;

	uiInfo.numGlInfoLines = 0;

	eptr = uiInfo.uiDC.glconfig.extensions_string;

	while (*eptr) {
		while (*eptr && *eptr == ' ')
			*eptr++ = '\0';

		// track start of valid string
		if (*eptr && *eptr != ' ') {
			uiInfo.glInfoLines[uiInfo.numGlInfoLines++] = eptr;
		}

		if (uiInfo.numGlInfoLines == GLINFO_LINES) {
			break;  // Arnout: failsafe
		}

		while (*eptr && *eptr != ' ')
			eptr++;
	}

	uiInfo.numGlInfoLines += 4; // vendor, version and pixelformat + a whiteline
}

// FIXME: table drive
//
static void UI_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle) {
	rectDef_t rect;

	// Nico, silent GCC
	(void)ownerDrawFlags;
	(void)special;
	(void)shader;

	rect.x = x + text_x;
	rect.y = y + text_y;
	rect.w = w;
	rect.h = h;

	switch (ownerDraw) {
	case UI_EFFECTS:
		UI_DrawEffects(&rect);
		break;
	case UI_MAPPREVIEW:
		UI_DrawMapPreview(&rect, scale, qtrue);
		break;
	case UI_NETMAPPREVIEW:
		UI_DrawNetMapPreview(&rect);
		break;
	case UI_MB_MAP:
		UI_DrawMissionBriefingMap(&rect);
		break;
	case UI_MB_TITLE:
		UI_DrawMissionBriefingTitle(&rect, scale, color, textStyle);
		break;
	case UI_MB_OBJECTIVES:
		UI_DrawMissionBriefingObjectives(&rect, scale, color, text_x, text_y, textStyle, align);
		break;
	case UI_NETFILTER:
		UI_DrawNetFilter(&rect, scale, color, textStyle);
		break;
	case UI_REDBLUE:
		UI_DrawRedBlue(&rect, scale, color, textStyle);
		break;
	case UI_CROSSHAIR:
		UI_DrawCrosshair(&rect);
		break;
	case UI_SELECTEDPLAYER:
		UI_DrawSelectedPlayer(&rect, scale, color, textStyle);
		break;
	case UI_SERVERREFRESHDATE:
		UI_DrawServerRefreshDate(&rect, scale, color, textStyle);
		break;
	case UI_SERVERMOTD:
		UI_DrawServerMOTD(&rect, scale, color);
		break;
	case UI_KEYBINDSTATUS:
		UI_DrawKeyBindStatus(&rect, scale, color, textStyle, text_x, text_y);
		break;
	case UI_LOADPANEL:
		UI_DrawLoadPanel(qtrue, qfalse);
		break;
	default:
		break;
	}
}

qboolean UI_OwnerDrawVisible(int flags) {
	qboolean vis = qtrue;

	while (flags) {

		if (flags & UI_SHOW_FFA) {
			flags &= ~UI_SHOW_FFA;
		}

		if (flags & UI_SHOW_NOTFFA) {
			vis    = qfalse;
			flags &= ~UI_SHOW_NOTFFA;
		}

		if (flags & UI_SHOW_LEADER) {
			// these need to show when this client can give orders to a player or a group
			if (!uiInfo.teamLeader) {
				vis = qfalse;
			} else {
				// if showing yourself
				if (ui_selectedPlayer.integer < uiInfo.myTeamCount && uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber) {
					vis = qfalse;
				}
			}
			flags &= ~UI_SHOW_LEADER;
		}
		if (flags & UI_SHOW_NOTLEADER) {
			// these need to show when this client is assigning their own status or they are NOT the leader
			if (uiInfo.teamLeader &&
			    !(ui_selectedPlayer.integer < uiInfo.myTeamCount &&
			      uiInfo.teamClientNums[ui_selectedPlayer.integer] == uiInfo.playerNumber)) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTLEADER;
		}
		if (flags & UI_SHOW_FAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource.integer != AS_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_FAVORITESERVERS;
		}
		if (flags & UI_SHOW_NOTFAVORITESERVERS) {
			// this assumes you only put this type of display flag on something showing in the proper context
			if (ui_netSource.integer == AS_FAVORITES) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_NOTFAVORITESERVERS;
		}
		if (flags & UI_SHOW_ANYTEAMGAME) {
			flags &= ~UI_SHOW_ANYTEAMGAME;
		}
		if (flags & UI_SHOW_ANYNONTEAMGAME) {
			vis    = qfalse;
			flags &= ~UI_SHOW_ANYNONTEAMGAME;
		}
		if (flags & UI_SHOW_NETANYTEAMGAME) {
			flags &= ~UI_SHOW_NETANYTEAMGAME;
		}
		if (flags & UI_SHOW_NETANYNONTEAMGAME) {
			vis    = qfalse;
			flags &= ~UI_SHOW_NETANYNONTEAMGAME;
		}
		if (flags & UI_SHOW_PLAYERMUTED) {
			if (!uiInfo.playerMuted[uiInfo.playerIndex]) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_PLAYERMUTED;
		}
		if (flags & UI_SHOW_PLAYERNOTMUTED) {
			if (uiInfo.playerMuted[uiInfo.playerIndex]) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_PLAYERNOTMUTED;
		}

		if (flags & UI_SHOW_PLAYERNOREFEREE) {
			if (uiInfo.playerRefereeStatus[uiInfo.playerIndex] != RL_NONE) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_PLAYERNOREFEREE;
		}
		if (flags & UI_SHOW_PLAYERREFEREE) {
			if (uiInfo.playerRefereeStatus[uiInfo.playerIndex] != RL_REFEREE) {
				vis = qfalse;
			}
			flags &= ~UI_SHOW_PLAYERREFEREE;
		}
	}
	return vis;
}

static qboolean UI_Effects_HandleKey(int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {

		if (key == K_MOUSE2) {
			uiInfo.effectsColor--;
		} else {
			uiInfo.effectsColor++;
		}

		if (uiInfo.effectsColor > 6) {
			uiInfo.effectsColor = 0;
		} else if (uiInfo.effectsColor < 0) {
			uiInfo.effectsColor = 6;
		}

		trap_Cvar_SetValue("color", uitogamecode[uiInfo.effectsColor]);
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_NetFilter_HandleKey(int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		if (key == K_MOUSE2) {
			ui_serverFilterType.integer--;
		} else {
			ui_serverFilterType.integer++;
		}

		if (ui_serverFilterType.integer >= numServerFilters) {
			ui_serverFilterType.integer = 0;
		} else if (ui_serverFilterType.integer < 0) {
			ui_serverFilterType.integer = numServerFilters - 1;
		}
		UI_BuildServerDisplayList(1);
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_RedBlue_HandleKey(int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		uiInfo.redBlue ^= 1;
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_Crosshair_HandleKey(int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		if (key == K_MOUSE2) {
			uiInfo.currentCrosshair--;
		} else {
			uiInfo.currentCrosshair++;
		}

		if (uiInfo.currentCrosshair >= NUM_CROSSHAIRS) {
			uiInfo.currentCrosshair = 0;
		} else if (uiInfo.currentCrosshair < 0) {
			uiInfo.currentCrosshair = NUM_CROSSHAIRS - 1;
		}
		trap_Cvar_Set("cg_drawCrosshair", va("%d", uiInfo.currentCrosshair));
		return qtrue;
	}
	return qfalse;
}

static qboolean UI_SelectedPlayer_HandleKey(int key) {
	if (key == K_MOUSE1 || key == K_MOUSE2 || key == K_ENTER || key == K_KP_ENTER) {
		int selected;

		UI_BuildPlayerList();
		if (!uiInfo.teamLeader) {
			return qfalse;
		}
		selected = trap_Cvar_VariableValue("cg_selectedPlayer");

		if (key == K_MOUSE2) {
			selected--;
		} else {
			selected++;
		}

		if (selected > uiInfo.myTeamCount) {
			selected = 0;
		} else if (selected < 0) {
			selected = uiInfo.myTeamCount;
		}

		if (selected == uiInfo.myTeamCount) {
			trap_Cvar_Set("cg_selectedPlayerName", "Everyone");
		} else {
			trap_Cvar_Set("cg_selectedPlayerName", uiInfo.teamNames[selected]);
		}
		trap_Cvar_Set("cg_selectedPlayer", va("%d", selected));
	}
	return qfalse;
}

static qboolean UI_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key) {
	// Nico, silent GCC
	(void)flags;
	(void)special;

	switch (ownerDraw) {
	case UI_EFFECTS:
		return UI_Effects_HandleKey(key);
		break;
	case UI_GAMETYPE:
		break;
	case UI_NETFILTER:
		UI_NetFilter_HandleKey(key);
		break;
	case UI_REDBLUE:
		UI_RedBlue_HandleKey(key);
		break;
	case UI_CROSSHAIR:
		UI_Crosshair_HandleKey(key);
		break;
	case UI_SELECTEDPLAYER:
		UI_SelectedPlayer_HandleKey(key);
		break;
	default:
		break;
	}

	return qfalse;
}

/*
=================
UI_ServersQsortCompare
=================
*/
static int QDECL UI_ServersQsortCompare(const void *arg1, const void *arg2) {
	return trap_LAN_CompareServers(ui_netSource.integer, uiInfo.serverStatus.sortKey, uiInfo.serverStatus.sortDir, *(int *)arg1, *(int *)arg2);
}

/*
=================
UI_ServersSort
=================
*/
void UI_ServersSort(int column, qboolean force) {

	if (!force && uiInfo.serverStatus.sortKey == column) {
		return;
	}

	uiInfo.serverStatus.sortKey = column;
	qsort(&uiInfo.serverStatus.displayServers[0], uiInfo.serverStatus.numDisplayServers, sizeof (int), UI_ServersQsortCompare);
}

/*
===============
UI_LoadMods
===============
*/
static void UI_LoadMods() {
	int  numdirs;
	char dirlist[2048];
	char *dirptr;
	int  i;

	uiInfo.modCount = 0;
	numdirs         = trap_FS_GetFileList("$modlist", "", dirlist, sizeof (dirlist));
	dirptr          = dirlist;
	for (i = 0; i < numdirs; ++i) {
		char *descptr;
		int  dirlen;

		dirlen                                   = strlen(dirptr) + 1;
		descptr                                  = dirptr + dirlen;
		uiInfo.modList[uiInfo.modCount].modName  = String_Alloc(dirptr);
		uiInfo.modList[uiInfo.modCount].modDescr = String_Alloc(descptr);
		dirptr                                  += dirlen + strlen(descptr) + 1;
		uiInfo.modCount++;
		if (uiInfo.modCount >= MAX_MODS) {
			break;
		}
	}
}

/*
===============
UI_LoadProfiles
===============
*/
static void UI_LoadProfiles() {
	int  numdirs;
	char dirlist[2048];
	char *dirptr;
	int  i;

	uiInfo.profileCount = 0;
	uiInfo.profileIndex = -1;
	numdirs             = trap_FS_GetFileList("profiles", "/", dirlist, sizeof (dirlist));
	dirptr              = dirlist;

	for (i = 0; i < numdirs; ++i) {
		int dirlen;

		dirlen = strlen(dirptr) + 1;

		if (dirptr[0] && Q_stricmp(dirptr, ".") && Q_stricmp(dirptr, "..")) {
			int        handle;
			pc_token_t token;

			if ((handle = trap_PC_LoadSource(va("profiles/%s/profile.dat", dirptr))) == 0) {
				dirptr += dirlen;
				continue;
			}

			if (!trap_PC_ReadToken(handle, &token)) {
				trap_PC_FreeSource(handle);
				dirptr += dirlen;
				continue;
			}

			uiInfo.profileList[uiInfo.profileCount].name = String_Alloc(token.string);
			trap_PC_FreeSource(handle);

			uiInfo.profileList[uiInfo.profileCount].dir = String_Alloc(dirptr);
			uiInfo.profileCount++;

			if (uiInfo.profileIndex == -1) {
				Q_CleanStr(token.string);
				Q_CleanDirName(token.string);
				if (!Q_stricmp(token.string, cl_profile.string)) {
					int j;

					uiInfo.profileIndex = i;
					trap_Cvar_Set("ui_profile", uiInfo.profileList[0].name);
					trap_Cvar_Update(&ui_profile);

					for (j = 0; j < Menu_Count(); ++j) {
						Menu_SetFeederSelection(Menu_Get(j), FEEDER_PROFILES, uiInfo.profileIndex, NULL);
					}
				}
			}

			if (uiInfo.profileCount >= MAX_PROFILES) {
				break;
			}
		}

		dirptr += dirlen;
	}

	if (uiInfo.profileIndex == -1) {
		int j;

		uiInfo.profileIndex = 0;
		trap_Cvar_Set("ui_profile", uiInfo.profileList[0].name);
		trap_Cvar_Update(&ui_profile);

		for (j = 0; j < Menu_Count(); ++j) {
			Menu_SetFeederSelection(Menu_Get(j), FEEDER_PROFILES, 0, NULL);
		}
	}
}

/*
===============
UI_LoadMovies
===============
*/
static void UI_LoadMovies() {
	char movielist[4096];
	char *moviename;

	uiInfo.movieCount = trap_FS_GetFileList("video", "roq", movielist, 4096);

	if (uiInfo.movieCount) {
		int i;

		if (uiInfo.movieCount > MAX_MOVIES) {
			uiInfo.movieCount = MAX_MOVIES;
		}
		moviename = movielist;
		for (i = 0; i < uiInfo.movieCount; ++i) {
			int len;

			len = strlen(moviename);
			if (!Q_stricmp(moviename +  len - 4, ".roq")) {
				moviename[len - 4] = '\0';
			}
			Q_strupr(moviename);
			uiInfo.movieList[i] = String_Alloc(moviename);
			moviename          += len + 1;
		}
	}
}

/*
===============
UI_LoadDemos
===============
*/
static void UI_LoadDemos() {
	char demolist[30000];
	char demoExt[32];
	char *demoname;
	int  demoExtLen = 0;

	Com_sprintf(demoExt, sizeof (demoExt), "dm_%d", (int)trap_Cvar_VariableValue("protocol"));

	uiInfo.demoCount = trap_FS_GetFileList("demos", demoExt, demolist, sizeof (demolist));

	Com_sprintf(demoExt, sizeof (demoExt), ".dm_%d", (int)trap_Cvar_VariableValue("protocol"));

	demoExtLen = strlen(demoExt);

	if (uiInfo.demoCount) {
		int i;

		if (uiInfo.demoCount > MAX_DEMOS) {
			uiInfo.demoCount = MAX_DEMOS;
		}
		demoname = demolist;
		for (i = 0; i < uiInfo.demoCount; ++i) {
			int len;

			len = strlen(demoname);
			if (!Q_stricmp(demoname +  len - demoExtLen, demoExt)) {
				demoname[len - demoExtLen] = '\0';
			}
			uiInfo.demoList[i] = String_Alloc(demoname);
			demoname          += len + 1;
		}
	}
}

qboolean UI_CheckExecKey(int key) {
	menuDef_t *menu = Menu_GetFocused();

	if (g_editingField) {
		return qtrue;
	}

	if (key > 254) {
		return qfalse;
	}

	if (!menu) {
		if (cl_bypassMouseInput.integer && !trap_Key_GetCatcher()) {
			trap_Cvar_Set("cl_bypassMouseInput", "0");
		}
		return qfalse;
	}

	if (menu->onKey[key]) {
		return qtrue;
	}

	return qfalse;
}
// -NERVE - SMF

/*
==============
UI_Update
==============
*/
void UI_Update(const char *name) {
	int val = trap_Cvar_VariableValue(name);

	if (Q_stricmp(name, "ui_SetName") == 0) {
		trap_Cvar_Set("name", UI_Cvar_VariableString("ui_Name"));
	} else if (Q_stricmp(name, "ui_setRate") == 0) {
		float rate = trap_Cvar_VariableValue("ui_rate");
		if (rate >= 5000) {
			trap_Cvar_Set("ui_cl_maxpackets", "30");
			trap_Cvar_Set("ui_cl_packetdup", "1");
		} else if (rate >= 4000) {
			trap_Cvar_Set("ui_cl_maxpackets", "15");
			trap_Cvar_Set("ui_cl_packetdup", "2");          // favor less prediction errors when there's packet loss
		} else {
			trap_Cvar_Set("ui_cl_maxpackets", "15");
			trap_Cvar_Set("ui_cl_packetdup", "1");          // favor lower bandwidth
		}
	} else if (Q_stricmp(name, "ui_GetName") == 0) {
		trap_Cvar_Set("ui_Name", UI_Cvar_VariableString("name"));
	} else if (Q_stricmp(name, "r_colorbits") == 0) {
		switch (val) {
		case 0:
			trap_Cvar_SetValue("r_depthbits", 0);
			trap_Cvar_SetValue("r_stencilbits", 0);
			break;
		case 16:
			trap_Cvar_SetValue("r_depthbits", 16);
			trap_Cvar_SetValue("r_stencilbits", 0);
			break;
		case 32:
			trap_Cvar_SetValue("r_depthbits", 24);
			break;
		}
	} else if (Q_stricmp(name, "ui_r_lodbias") == 0) {
		switch (val) {
		case 0:
			trap_Cvar_SetValue("ui_r_subdivisions", 4);
			break;
		case 1:
			trap_Cvar_SetValue("ui_r_subdivisions", 12);
			break;
		case 2:
			trap_Cvar_SetValue("ui_r_subdivisions", 20);
			break;
		}
	} else if (Q_stricmp(name, "ui_glCustom") == 0) {
		switch (val) {
		case 0:     // high quality
			trap_Cmd_ExecuteText(EXEC_APPEND, "exec preset_high_ui.cfg\n");
			break;
		case 1:     // normal
			trap_Cmd_ExecuteText(EXEC_APPEND, "exec preset_normal_ui.cfg\n");
			break;
		case 2:     // fast
			trap_Cmd_ExecuteText(EXEC_APPEND, "exec preset_fast_ui.cfg\n");
			break;
		case 3:     // fastest
			trap_Cmd_ExecuteText(EXEC_APPEND, "exec preset_fastest_ui.cfg\n");
			break;
		}
	} else if (Q_stricmp(name, "ui_mousePitch") == 0) {
		if (val == 0) {
			trap_Cvar_SetValue("m_pitch", 0.022f);
		} else {
			trap_Cvar_SetValue("m_pitch", -0.022f);
		}
	}
}

/*
==============
UI_RunMenuScript
==============
*/
void UI_RunMenuScript(char **args) {
	const char *name, *name2;

	if (String_Parse(args, &name)) {
		char      *s;
		char      buff[1024];
		menuDef_t *menu;

		if (Q_stricmp(name, "StartServer") == 0) {
			trap_Cvar_Set("ui_connecting", "1");
			trap_Cvar_Set("cg_thirdPerson", "0");
			trap_Cvar_Set("cg_cameraOrbit", "0");
			trap_Cvar_Set("ui_singlePlayerActive", "0");
			trap_Cvar_SetValue("dedicated", Com_Clamp(0, 2, ui_dedicated.integer));
			trap_Cmd_ExecuteText(EXEC_APPEND, va("wait ; wait ; map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName));
			trap_Cvar_VariableValue("g_spSkill");
		} else if (Q_stricmp(name, "resetDefaults") == 0) {
			trap_Cmd_ExecuteText(EXEC_APPEND, "cvar_restart\n");           // NERVE - SMF - changed order
			trap_Cmd_ExecuteText(EXEC_APPEND, "exec default.cfg\n");
			trap_Cmd_ExecuteText(EXEC_APPEND, "exec language.cfg\n");          // NERVE - SMF
			trap_Cmd_ExecuteText(EXEC_APPEND, "setRecommended\n");        // NERVE - SMF
			Controls_SetDefaults(qfalse);
			trap_Cvar_Set("com_introPlayed", "1");
			trap_Cvar_Set("com_recommendedSet", "1");                     // NERVE - SMF
			trap_Cmd_ExecuteText(EXEC_APPEND, "vid_restart\n");
		} else if (Q_stricmp(name, "loadArenas") == 0) {
			UI_LoadArenas();
			Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, 0, NULL);
		} else if (Q_stricmp(name, "updateNetMap") == 0) {
			Menu_SetFeederSelection(NULL, FEEDER_ALLMAPS, ui_currentNetMap.integer, NULL);
		} else if (Q_stricmp(name, "saveControls") == 0) {
			Controls_SetConfig(qtrue);
		} else if (Q_stricmp(name, "loadControls") == 0) {
			Controls_GetConfig();
		} else if (Q_stricmp(name, "clearError") == 0) {
			trap_Cvar_Set("com_errorMessage", "");
			trap_Cvar_Set("com_errorDiagnoseIP", "");
			trap_Cvar_Set("com_missingFiles", "");
		} else if (Q_stricmp(name, "RefreshServers") == 0) {
			UI_StartServerRefresh(qtrue);
			UI_BuildServerDisplayList(1);
		} else if (Q_stricmp(name, "RefreshFilter") == 0) {
			UI_StartServerRefresh(uiInfo.serverStatus.numDisplayServers ? qfalse : qtrue);      // if we don't have any valid servers, it's kinda safe to assume we would like to get a full new list
			UI_BuildServerDisplayList(1);
		} else if (Q_stricmp(name, "LoadDemos") == 0) {
			UI_LoadDemos();
		} else if (Q_stricmp(name, "LoadMovies") == 0) {
			UI_LoadMovies();
		} else if (Q_stricmp(name, "LoadMods") == 0) {
			UI_LoadMods();
		} else if (Q_stricmp(name, "RunMod") == 0) {
			trap_Cvar_Set("fs_game", uiInfo.modList[uiInfo.modIndex].modName);
			trap_Cmd_ExecuteText(EXEC_APPEND, "vid_restart;");
		} else if (Q_stricmp(name, "RunDemo") == 0) {
			if (uiInfo.demoIndex >= 0 && uiInfo.demoIndex < uiInfo.demoCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("demo \"%s\"\n", uiInfo.demoList[uiInfo.demoIndex]));
			}
		} else if (Q_stricmp(name, "deleteDemo") == 0) {
			if (uiInfo.demoIndex >= 0 && uiInfo.demoIndex < uiInfo.demoCount) {
				trap_FS_Delete(va("demos/%s.dm_%d", uiInfo.demoList[uiInfo.demoIndex], (int)trap_Cvar_VariableValue("protocol")));
			}
		} else if (Q_stricmp(name, "closeJoin") == 0) {
			if (uiInfo.serverStatus.refreshActive) {
				UI_StopServerRefresh();
				uiInfo.serverStatus.nextDisplayRefresh = 0;
				uiInfo.nextServerStatusRefresh         = 0;
				uiInfo.nextFindPlayerRefresh           = 0;
				UI_BuildServerDisplayList(1);
			} else {
				Menus_CloseByName("joinserver");
				Menus_OpenByName("main");
			}
		} else if (Q_stricmp(name, "StopRefresh") == 0) {
			UI_StopServerRefresh();
			uiInfo.serverStatus.nextDisplayRefresh = 0;
			uiInfo.nextServerStatusRefresh         = 0;
			uiInfo.nextFindPlayerRefresh           = 0;
		} else if (Q_stricmp(name, "UpdateFilter") == 0) {
			trap_Cvar_Update(&ui_netSource);
			if (ui_netSource.integer == AS_LOCAL || !uiInfo.serverStatus.numDisplayServers) {
				UI_StartServerRefresh(qtrue);
			}
			UI_BuildServerDisplayList(1);
			UI_FeederSelection(FEEDER_SERVERS, 0);
		} else if (Q_stricmp(name, "check_ServerStatus") == 0) {
			s    = UI_Cvar_VariableString("com_errorDiagnoseIP");
			menu = Menus_FindByName("ingame_options");
			if (strlen(s) && strcmp(s, "localhost")) {
				if (menu) {
					Menu_ShowItemByName(menu, "ctr_serverinfo", qtrue);
				}
			} else {
				if (menu) {
					Menu_ShowItemByName(menu, "ctr_serverinfo", qfalse);
				}
			}
		} else if (Q_stricmp(name, "ServerStatus") == 0) {
			// the server info dialog has been turned into a modal thing
			// it can be called in several situations
			if (trap_Cvar_VariableValue("ui_serverBrowser") == 1) {
				// legacy, from the server browser
				trap_LAN_GetServerAddressString(ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], uiInfo.serverStatusAddress, sizeof (uiInfo.serverStatusAddress));
				UI_BuildServerStatus(qtrue);
			} else {
				// use com_errorDiagnoseIP otherwise
				s = UI_Cvar_VariableString("com_errorDiagnoseIP");
				if (strlen(s) && strcmp(s, "localhost")) {
					trap_Cvar_VariableStringBuffer("com_errorDiagnoseIP", uiInfo.serverStatusAddress, sizeof (uiInfo.serverStatusAddress));
					uiInfo.serverStatus.numDisplayServers = 1;   // this is ugly, have to force a non zero display server count to emit the query
					UI_BuildServerStatus(qtrue);
				} else {
					// we can't close the menu from here, it's not open yet .. (that's the onOpen script)
					Com_Printf("Can't show Server Info (not found, or local server)\n");
				}
			}
		} else if (Q_stricmp(name, "InGameServerStatus") == 0) {
			uiClientState_t cstate;
			trap_GetClientState(&cstate);
			Q_strncpyz(uiInfo.serverStatusAddress, cstate.servername, sizeof (uiInfo.serverStatusAddress));
			UI_BuildServerStatus(qtrue);
		} else if (Q_stricmp(name, "ServerStatus_diagnose") == 0) {
			// query server and display the URL buttons if the error happened during a server connection situation
			s    = UI_Cvar_VariableString("com_errorDiagnoseIP");
			menu = Menus_FindByName("error_popmenu_diagnose");
			if (strlen(s) && strcmp(s, "localhost")) {
				trap_Cvar_VariableStringBuffer("com_errorDiagnoseIP", uiInfo.serverStatusAddress, sizeof (uiInfo.serverStatusAddress));
				uiInfo.serverStatus.numDisplayServers = 1;   // this is ugly, have to force a non zero display server count to emit the query
				// toggle the "Server Info" button
				if (menu) {
					Menu_ShowItemByName(menu, "serverinfo", qtrue);
				}
				UI_BuildServerStatus(qtrue);
			} else {
				// don't send getinfo packet, hide "Server Info" button
				if (menu) {
					Menu_ShowItemByName(menu, "serverinfo", qfalse);
				}
			}
		} else if (Q_stricmp(name, "FoundPlayerServerStatus") == 0) {
			Q_strncpyz(uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof (uiInfo.serverStatusAddress));
			UI_BuildServerStatus(qtrue);
			Menu_SetFeederSelection(NULL, FEEDER_FINDPLAYER, 0, NULL);
		} else if (Q_stricmp(name, "FindPlayer") == 0) {
			UI_BuildFindPlayerList(qtrue);
			// clear the displayed server status info
			uiInfo.serverStatusInfo.numLines = 0;
			Menu_SetFeederSelection(NULL, FEEDER_FINDPLAYER, 0, NULL);
		} else if (Q_stricmp(name, "JoinServer") == 0) {
			if (uiInfo.serverStatus.currentServer >= 0 && uiInfo.serverStatus.currentServer < uiInfo.serverStatus.numDisplayServers) {
				Menus_CloseAll();
				trap_Cvar_Set("ui_connecting", "1");
				trap_Cvar_Set("cg_thirdPerson", "0 ");
				trap_Cvar_Set("cg_cameraOrbit", "0");
				trap_Cvar_Set("ui_singlePlayerActive", "0");
				trap_LAN_GetServerAddressString(ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, 1024);
				trap_Cmd_ExecuteText(EXEC_APPEND, va("connect %s\n", buff));
			}
		} else if (Q_stricmp(name, "JoinDirectServer") == 0) {
			Menus_CloseAll();
			trap_Cvar_Set("ui_connecting", "1");
			trap_Cvar_Set("cg_thirdPerson", "0");
			trap_Cvar_Set("cg_cameraOrbit", "0");
			trap_Cvar_Set("ui_singlePlayerActive", "0");
			trap_Cmd_ExecuteText(EXEC_APPEND, va("connect %s\n", UI_Cvar_VariableString("ui_connectToIPAddress")));
		} else if (Q_stricmp(name, "FoundPlayerJoinServer") == 0) {
			trap_Cvar_Set("ui_singlePlayerActive", "0");
			if (uiInfo.currentFoundPlayerServer >= 0 && uiInfo.currentFoundPlayerServer < uiInfo.numFoundPlayerServers) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("connect %s\n", uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer]));
			}
		} else if (Q_stricmp(name, "Quit") == 0) {
			trap_Cvar_Set("ui_singlePlayerActive", "0");
			trap_Cmd_ExecuteText(EXEC_NOW, "quit");
		} else if (Q_stricmp(name, "Controls") == 0) {
			trap_Cvar_Set("cl_paused", "1");
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_CloseAll();
			Menus_ActivateByName("setup_menu2", qtrue);
		} else if (Q_stricmp(name, "Leave") == 0) {
			// ATVI Wolfenstein Misc #460
			// if we are running a local server, make sure we kill it cleanly for other clients
			if (trap_Cvar_VariableValue("sv_running")) {
				trap_Cvar_Set("sv_killserver", "1");
			} else {
				trap_Cmd_ExecuteText(EXEC_APPEND, "disconnect\n");
				trap_Key_SetCatcher(KEYCATCH_UI);
				Menus_CloseAll();
				Menus_ActivateByName("backgroundmusic", qtrue);
				Menus_ActivateByName("main_opener", qtrue);
			}
		} else if (Q_stricmp(name, "ServerSort") == 0) {
			int sortColumn;
			if (Int_Parse(args, &sortColumn)) {
				// if same column we're already sorting on then flip the direction
				if (sortColumn == uiInfo.serverStatus.sortKey) {
					uiInfo.serverStatus.sortDir = !uiInfo.serverStatus.sortDir;
				}
				// make sure we sort again
				UI_ServersSort(sortColumn, qtrue);
			}
		} else if (Q_stricmp(name, "ServerSortDown") == 0) {
			int sortColumn;
			if (Int_Parse(args, &sortColumn)) {
				uiInfo.serverStatus.sortDir = 0;

				// make sure we sort again
				UI_ServersSort(sortColumn, qtrue);
			}
		} else if (Q_stricmp(name, "nextSkirmish") == 0) {

		} else if (Q_stricmp(name, "SkirmishStart") == 0) {

		} else if (Q_stricmp(name, "closeingame") == 0) {
			trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
			trap_Key_ClearStates();
			trap_Cvar_Set("cl_paused", "0");
			Menus_CloseAll();
		} else if (Q_stricmp(name, "voteMap") == 0) {
			if (ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.mapCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("callvote map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName));
			}
		} else if (Q_stricmp(name, "voteKick") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("callvote kick \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "voteMute") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("callvote mute \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "voteUnMute") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("callvote unmute \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "voteReferee") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("callvote referee \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "voteUnReferee") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("callvote unreferee \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "voteInitToggles") == 0) {
			// Nico, note: keep this in order to avoid unknown UI script
		} else if (Q_stricmp(name, "addFavorite") == 0) {
			if (ui_netSource.integer != AS_FAVORITES) {
				char name[MAX_NAME_LENGTH];
				char addr[MAX_NAME_LENGTH];

				trap_LAN_GetServerInfo(ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS);
				name[0] = addr[0] = '\0';
				Q_strncpyz(name, Info_ValueForKey(buff, "hostname"), MAX_NAME_LENGTH);
				Q_strncpyz(addr, Info_ValueForKey(buff, "addr"), MAX_NAME_LENGTH);
				if (name[0] != '\0' && addr[0] != '\0') {
					int res;

					res = trap_LAN_AddServer(AS_FAVORITES, name, addr);
					if (res == 0) {
						// server already in the list
						// Nico, string format bug security fix
						Com_Printf("%s", trap_TranslateString("Favorite already in list\n"));
					} else if (res == -1) {
						// list full
						// Nico, string format bug security fix
						Com_Printf("%s", trap_TranslateString("Favorite list full\n"));
					} else {
						// successfully added
						Com_Printf(trap_TranslateString("Added favorite server %s\n"), addr);
					}
				}
			}
		} else if (Q_stricmp(name, "deleteFavorite") == 0) {
			if (ui_netSource.integer == AS_FAVORITES) {
				char addr[MAX_NAME_LENGTH];
				trap_LAN_GetServerInfo(ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS);
				addr[0] = '\0';
				Q_strncpyz(addr, Info_ValueForKey(buff, "addr"), MAX_NAME_LENGTH);
				if (addr[0] != '\0') {
					trap_LAN_RemoveServer(AS_FAVORITES, addr);
				}
			}
		} else if (Q_stricmp(name, "createFavorite") == 0) {
			if (ui_netSource.integer == AS_FAVORITES) {
				char name[MAX_NAME_LENGTH];
				char addr[MAX_NAME_LENGTH];

				name[0] = addr[0] = '\0';
				Q_strncpyz(name, UI_Cvar_VariableString("ui_favoriteName"), MAX_NAME_LENGTH);
				Q_strncpyz(addr, UI_Cvar_VariableString("ui_favoriteAddress"), MAX_NAME_LENGTH);
				if (name[0] != '\0' && addr[0] != '\0') {
					int res;

					res = trap_LAN_AddServer(AS_FAVORITES, name, addr);
					if (res == 0) {
						// server already in the list
						// Nico, string format bug security fix
						Com_Printf("%s", trap_TranslateString("Favorite already in list\n"));
					} else if (res == -1) {
						// list full
						// Nico, string format bug security fix
						Com_Printf("%s", trap_TranslateString("Favorite list full\n"));
					} else {
						// successfully added
						Com_Printf(trap_TranslateString("Added favorite server %s\n"), addr);
					}
				}
			}
		} else if (Q_stricmp(name, "createFavoriteIngame") == 0) {
			uiClientState_t cstate;
			char            name[MAX_NAME_LENGTH];
			char            addr[MAX_NAME_LENGTH];

			trap_GetClientState(&cstate);

			addr[0] = '\0';
			name[0] = '\0';
			Q_strncpyz(addr, cstate.servername, MAX_NAME_LENGTH);
			Q_strncpyz(name, cstate.servername, MAX_NAME_LENGTH);
			if (*name && *addr && Q_stricmp(addr, "localhost")) {
				int res;

				res = trap_LAN_AddServer(AS_FAVORITES, name, addr);
				if (res == 0) {
					// server already in the list
					// Nico, string format bug security fix
					Com_Printf("%s", trap_TranslateString("Favorite already in list\n"));
				} else if (res == -1) {
					// list full
					// Nico, string format bug security fix
					Com_Printf("%s", trap_TranslateString("Favorite list full\n"));
				} else {
					// successfully added
					Com_Printf(trap_TranslateString("Added favorite server %s\n"), addr);
				}
			}
		} else if (Q_stricmp(name, "orders") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap_Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer < uiInfo.myTeamCount) {
					Q_strncpyz(buff, orders, sizeof (buff));
					trap_Cmd_ExecuteText(EXEC_APPEND, va(buff, uiInfo.teamClientNums[selectedPlayer]));
					trap_Cmd_ExecuteText(EXEC_APPEND, "\n");
				} else {
					int i;
					for (i = 0; i < uiInfo.myTeamCount; ++i) {
						if (Q_stricmp(UI_Cvar_VariableString("name"), uiInfo.teamNames[i]) == 0) {
							continue;
						}
						Q_strncpyz(buff, orders, sizeof (buff));
						trap_Cmd_ExecuteText(EXEC_APPEND, va(buff, uiInfo.teamNames[i]));
						trap_Cmd_ExecuteText(EXEC_APPEND, "\n");
					}
				}
				trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
				trap_Key_ClearStates();
				trap_Cvar_Set("cl_paused", "0");
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "voiceOrdersTeam") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap_Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer == uiInfo.myTeamCount) {
					trap_Cmd_ExecuteText(EXEC_APPEND, orders);
					trap_Cmd_ExecuteText(EXEC_APPEND, "\n");
				}
				trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
				trap_Key_ClearStates();
				trap_Cvar_Set("cl_paused", "0");
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "voiceOrders") == 0) {
			const char *orders;
			if (String_Parse(args, &orders)) {
				int selectedPlayer = trap_Cvar_VariableValue("cg_selectedPlayer");
				if (selectedPlayer < uiInfo.myTeamCount) {
					Q_strncpyz(buff, orders, sizeof (buff));
					trap_Cmd_ExecuteText(EXEC_APPEND, va(buff, uiInfo.teamClientNums[selectedPlayer]));
					trap_Cmd_ExecuteText(EXEC_APPEND, "\n");
				}
				trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
				trap_Key_ClearStates();
				trap_Cvar_Set("cl_paused", "0");
				Menus_CloseAll();
			}
		} else if (Q_stricmp(name, "glCustom") == 0) {
			trap_Cvar_Set("ui_glCustom", "4");
		} else if (Q_stricmp(name, "update") == 0) {
			if (String_Parse(args, &name2)) {
				UI_Update(name2);
			}
			// NERVE - SMF
		} else if (Q_stricmp(name, "startSingleplayer") == 0) {
			trap_Cmd_ExecuteText(EXEC_APPEND, "startSingleplayer\n");
		} else if (Q_stricmp(name, "showSpecScores") == 0) {
			if (atoi(UI_Cvar_VariableString("ui_isSpectator"))) {
				trap_Cmd_ExecuteText(EXEC_APPEND, "+scores\n");
			}
		} else if (Q_stricmp(name, "setPbClStatus") == 0) {
		} else if (Q_stricmp(name, "rconGame") == 0) {
		} else if (Q_stricmp(name, "rconMap") == 0) {
			if (ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.mapCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("rcon map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName));
			}
		} else if (Q_stricmp(name, "refMap") == 0) {
			if (ui_currentNetMap.integer >= 0 && ui_currentNetMap.integer < uiInfo.mapCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref map %s\n", uiInfo.mapList[ui_currentNetMap.integer].mapLoadName));
			}
		} else if (Q_stricmp(name, "rconKick") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("rcon kick \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "refKick") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref kick \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "rconBan") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("rcon ban \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "refMute") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref mute \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "refUnMute") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref unmute \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "refMakeAxis") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref putaxis \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "refMakeAllied") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref putallies \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "refMakeSpec") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref remove \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "refUnReferee") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref unreferee \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "refMakeReferee") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref referee \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "rconMakeReferee") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("rcon makeReferee \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "rconRemoveReferee") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("rcon removeReferee \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "rconMute") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("rcon mute \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "rconUnMute") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("rcon unmute \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "refWarning") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				char buffer[128];
				trap_Cvar_VariableStringBuffer("ui_warnreason", buffer, 128);

				trap_Cmd_ExecuteText(EXEC_APPEND, va("ref warn \"%s\" \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex], buffer));
			}
		} else if (Q_stricmp(name, "ignorePlayer") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("ignore \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "unIgnorePlayer") == 0) {
			if (uiInfo.playerIndex >= 0 && uiInfo.playerIndex < uiInfo.playerCount) {
				trap_Cmd_ExecuteText(EXEC_APPEND, va("unignore \"%s\"\n", uiInfo.playerNames[uiInfo.playerIndex]));
			}
		} else if (Q_stricmp(name, "loadCachedServers") == 0) {
			trap_LAN_LoadCachedServers();   // load servercache.dat
		} else if (Q_stricmp(name, "loadProfiles") == 0) {
			UI_LoadProfiles();
		} else if (Q_stricmp(name, "createProfile") == 0) {
			fileHandle_t f;
			char         buff[MAX_CVAR_VALUE_STRING];

			Q_strncpyz(buff, ui_profile.string, sizeof (buff));
			Q_CleanStr(buff);
			Q_CleanDirName(buff);

			if (trap_FS_FOpenFile(va("profiles/%s/profile.dat", buff), &f, FS_WRITE) >= 0) {
				trap_FS_Write(va("\"%s\"", ui_profile.string), strlen(ui_profile.string) + 2, f);
				trap_FS_FCloseFile(f);
			}
			trap_Cvar_Set("name", ui_profile.string);
		} else if (Q_stricmp(name, "clearPID") == 0) {
			fileHandle_t f;

			// delete profile.pid from current profile
			if (trap_FS_FOpenFile(va("profiles/%s/profile.pid", cl_profile.string), &f, FS_READ) >= 0) {
				trap_FS_FCloseFile(f);
				trap_FS_Delete(va("profiles/%s/profile.pid", cl_profile.string));
			}
		} else if (Q_stricmp(name, "applyProfile") == 0) {
			Q_strncpyz(cl_profile.string, ui_profile.string, sizeof (cl_profile.string));
			Q_CleanStr(cl_profile.string);
			Q_CleanDirName(cl_profile.string);
			trap_Cvar_Set("cl_profile", cl_profile.string);
		} else if (Q_stricmp(name, "setDefaultProfile") == 0) {
			fileHandle_t f;

			Q_strncpyz(cl_defaultProfile.string, ui_profile.string, sizeof (cl_profile.string));
			Q_CleanStr(cl_defaultProfile.string);
			Q_CleanDirName(cl_defaultProfile.string);
			trap_Cvar_Set("cl_defaultProfile", cl_defaultProfile.string);

			if (trap_FS_FOpenFile("profiles/defaultprofile.dat", &f, FS_WRITE) >= 0) {
				trap_FS_Write(va("\"%s\"", cl_defaultProfile.string), strlen(cl_defaultProfile.string) + 2, f);
				trap_FS_FCloseFile(f);
			}
		} else if (Q_stricmp(name, "deleteProfile") == 0) {
			char buff[MAX_CVAR_VALUE_STRING];

			Q_strncpyz(buff, ui_profile.string, sizeof (buff));
			Q_CleanStr(buff);
			Q_CleanDirName(buff);

			// can't delete active profile
			if (Q_stricmp(buff, cl_profile.string)) {
				if (!Q_stricmp(buff, cl_defaultProfile.string)) {
					// if deleting the default profile, set the default to the current active profile
					fileHandle_t f;

					trap_Cvar_Set("cl_defaultProfile", cl_profile.string);
					if (trap_FS_FOpenFile("profiles/defaultprofile.dat", &f, FS_WRITE) >= 0) {
						trap_FS_Write(va("\"%s\"", cl_profile.string), strlen(cl_profile.string) + 2, f);
						trap_FS_FCloseFile(f);
					}
				}

				trap_FS_Delete(va("profiles/%s", buff));
			}
		} else if (Q_stricmp(name, "renameProfileInit") == 0) {
			trap_Cvar_Set("ui_profile_renameto", ui_profile.string);
		} else if (Q_stricmp(name, "renameProfile") == 0) {
			fileHandle_t f, f2;
			char         buff[MAX_CVAR_VALUE_STRING];
			char         ui_renameprofileto[MAX_CVAR_VALUE_STRING];
			char         uiprofile[MAX_CVAR_VALUE_STRING];

			trap_Cvar_VariableStringBuffer("ui_profile_renameto", ui_renameprofileto, sizeof (ui_renameprofileto));
			Q_strncpyz(buff, ui_renameprofileto, sizeof (buff));
			Q_CleanStr(buff);
			Q_CleanDirName(buff);

			Q_strncpyz(uiprofile, ui_profile.string, sizeof (uiprofile));
			Q_CleanStr(uiprofile);
			Q_CleanDirName(uiprofile);

			if (trap_FS_FOpenFile(va("profiles/%s/profile.dat", buff), &f, FS_WRITE) >= 0) {
				trap_FS_Write(va("\"%s\"", ui_renameprofileto), strlen(ui_renameprofileto) + 2, f);
				trap_FS_FCloseFile(f);
			}

			// FIXME: make this copying handle all files in the profiles directory
			if (Q_stricmp(uiprofile, buff)) {
				int len;

				if (trap_FS_FOpenFile(va("profiles/%s/%s", buff, CONFIG_NAME), &f, FS_WRITE) >= 0) {
					if ((len = trap_FS_FOpenFile(va("profiles/%s/%s", uiprofile, CONFIG_NAME), &f2, FS_READ)) >= 0) {
						int i;

						for (i = 0; i < len; ++i) {
							byte b;

							trap_FS_Read(&b, 1, f2);
							trap_FS_Write(&b, 1, f);
						}
						trap_FS_FCloseFile(f2);
					}
					trap_FS_FCloseFile(f);
				}

				if (trap_FS_FOpenFile(va("profiles/%s/servercache.dat", buff), &f, FS_WRITE) >= 0) {
					if ((len = trap_FS_FOpenFile(va("profiles/%s/servercache.dat", cl_profile.string), &f2, FS_READ)) >= 0) {
						int i;

						for (i = 0; i < len; ++i) {
							byte b;

							trap_FS_Read(&b, 1, f2);
							trap_FS_Write(&b, 1, f);
						}
						trap_FS_FCloseFile(f2);
					}
					trap_FS_FCloseFile(f);
				}

				if (!Q_stricmp(uiprofile, cl_defaultProfile.string)) {
					// if renaming the default profile, set the default to the new profile
					trap_Cvar_Set("cl_defaultProfile", buff);
					if (trap_FS_FOpenFile("profiles/defaultprofile.dat", &f, FS_WRITE) >= 0) {
						trap_FS_Write(va("\"%s\"", buff), strlen(buff) + 2, f);
						trap_FS_FCloseFile(f);
					}
				}

				// if renaming the active profile, set active to new name
				if (!Q_stricmp(uiprofile, cl_profile.string)) {
					trap_Cvar_Set("cl_profile", buff);
				}

				// delete the old profile
				trap_FS_Delete(va("profiles/%s", uiprofile));
			}

			trap_Cvar_Set("ui_profile", ui_renameprofileto);
			trap_Cvar_Set("ui_profile_renameto", "");
		} else if (Q_stricmp(name, "initHostGameFeatures") == 0) {
		} else if (Q_stricmp(name, "togglePbSvStatus") == 0) {
		} else if (Q_stricmp(name, "openModURL") == 0) {
			trap_Cvar_Set("ui_finalURL", UI_Cvar_VariableString("ui_modURL"));
		} else if (Q_stricmp(name, "openServerURL") == 0) {
			trap_Cvar_Set("ui_finalURL", UI_Cvar_VariableString("ui_URL"));
		} else if (Q_stricmp(name, "validate_openURL") == 0) {
			// this is the only one that effectively triggers the URL, after the disclaimers are done with
			// we use ui_finalURL as an auxiliary variable to gather URLs from various sources
			trap_openURL(UI_Cvar_VariableString("ui_finalURL"));
		} else if (Q_stricmp(name, "clientCheckVote") == 0) {
			int flags = trap_Cvar_VariableValue("cg_ui_voteFlags");

			if (flags == VOTING_DISABLED) {
				trap_Cvar_SetValue("cg_ui_novote", 1);
			} else {
				trap_Cvar_SetValue("cg_ui_novote", 0);
			}
		} else if (Q_stricmp(name, "reconnect") == 0) {
			// TODO: if dumped because of cl_allowdownload problem, toggle on first (we don't have appropriate support for this yet)
			trap_Cmd_ExecuteText(EXEC_APPEND, "reconnect\n");
		} else if (Q_stricmp(name, "redirect") == 0) {       // fretn
			char buf[MAX_STRING_CHARS];
			trap_Cvar_VariableStringBuffer("com_errorMessage", buf, sizeof (buf));
			trap_Cmd_ExecuteText(EXEC_APPEND, va("connect %s\n", buf));
			trap_Cvar_Set("com_errorMessage", "");
		} else if (Q_stricmp(name, "updateGameType") == 0) {

		}
		// ydnar
		else if (Q_stricmp(name, "vidSave") == 0) {
			int mode;

			// get mode
			mode = trap_Cvar_VariableValue("r_mode");

			// save mode to old mode
			trap_Cvar_SetValue("r_oldMode", mode);
		} else if (Q_stricmp(name, "vidReset") == 0) {
			int oldMode;

			// get old mode
			oldMode = trap_Cvar_VariableValue("r_oldMode");
			if (oldMode == 0) {
				oldMode = 3;
			}

			// reset mode to old mode
			trap_Cvar_SetValue("r_mode", oldMode);
			trap_Cvar_Set("r_oldMode", "");
		} else if (Q_stricmp(name, "vidConfirm") == 0) {
			trap_Cvar_Set("r_oldMode", "");
		} else if (Q_stricmp(name, "systemCvarsGet") == 0) {
			int   ui_r_mode                    = trap_Cvar_VariableValue("r_mode");
			float ui_r_gamma                   = trap_Cvar_VariableValue("r_gamma");
			int   ui_rate                      = trap_Cvar_VariableValue("rate");
			int   ui_cl_maxpackets             = trap_Cvar_VariableValue("cl_maxpackets");
			int   ui_cl_packetdup              = trap_Cvar_VariableValue("cl_packetdup");
			float ui_sensitivity               = trap_Cvar_VariableValue("sensitivity");
			int   ui_r_colorbits               = trap_Cvar_VariableValue("r_colorbits");
			int   ui_r_fullscreen              = trap_Cvar_VariableValue("r_fullscreen");
			int   ui_r_lodbias                 = trap_Cvar_VariableValue("r_lodbias");
			int   ui_r_subdivisions            = trap_Cvar_VariableValue("r_subdivisions");
			int   ui_r_picmip                  = trap_Cvar_VariableValue("r_picmip");
			int   ui_r_texturebits             = trap_Cvar_VariableValue("r_texturebits");
			int   ui_r_depthbits               = trap_Cvar_VariableValue("r_depthbits");
			int   ui_r_ext_compressed_textures = trap_Cvar_VariableValue("r_ext_compressed_textures");
			int   ui_r_finish                  = trap_Cvar_VariableValue("r_finish");
			int   ui_r_dynamiclight            = trap_Cvar_VariableValue("r_dynamiclight");
			int   ui_r_allowextensions         = trap_Cvar_VariableValue("r_allowextensions");
			int   ui_m_filter                  = trap_Cvar_VariableValue("m_filter");
			int   ui_s_khz                     = trap_Cvar_VariableValue("s_khz");
			int   ui_r_detailtextures          = trap_Cvar_VariableValue("r_detailtextures");
			char  ui_r_texturemode[MAX_CVAR_VALUE_STRING];
			trap_Cvar_VariableStringBuffer("r_texturemode", ui_r_texturemode, sizeof (ui_r_texturemode));

			trap_Cvar_Set("ui_r_mode", va("%i", ui_r_mode));
			trap_Cvar_Set("ui_r_gamma", va("%f", ui_r_gamma));
			trap_Cvar_Set("ui_rate", va("%i", ui_rate));
			trap_Cvar_Set("ui_cl_maxpackets", va("%i", ui_cl_maxpackets));
			trap_Cvar_Set("ui_cl_packetdup", va("%i", ui_cl_packetdup));
			trap_Cvar_Set("ui_sensitivity", va("%f", ui_sensitivity));
			trap_Cvar_Set("ui_r_colorbits", va("%i", ui_r_colorbits));
			trap_Cvar_Set("ui_r_fullscreen", va("%i", ui_r_fullscreen));
			trap_Cvar_Set("ui_r_lodbias", va("%i", ui_r_lodbias));
			trap_Cvar_Set("ui_r_subdivisions", va("%i", ui_r_subdivisions));
			trap_Cvar_Set("ui_r_picmip", va("%i", ui_r_picmip));
			trap_Cvar_Set("ui_r_texturebits", va("%i", ui_r_texturebits));
			trap_Cvar_Set("ui_r_depthbits", va("%i", ui_r_depthbits));
			trap_Cvar_Set("ui_r_ext_compressed_textures", va("%i", ui_r_ext_compressed_textures));
			trap_Cvar_Set("ui_r_finish", va("%i", ui_r_finish));
			trap_Cvar_Set("ui_r_dynamiclight", va("%i", ui_r_dynamiclight));
			trap_Cvar_Set("ui_r_allowextensions", va("%i", ui_r_allowextensions));
			trap_Cvar_Set("ui_m_filter", va("%i", ui_m_filter));
			trap_Cvar_Set("ui_s_khz", va("%i", ui_s_khz));
			trap_Cvar_Set("ui_r_detailtextures", va("%i", ui_r_detailtextures));
			trap_Cvar_Set("ui_r_texturemode", ui_r_texturemode);
		} else if (Q_stricmp(name, "systemCvarsReset") == 0) {
			trap_Cvar_Set("ui_r_mode", "");
			trap_Cvar_Set("ui_r_gamma", "");
			trap_Cvar_Set("ui_rate", "");
			trap_Cvar_Set("ui_cl_maxpackets", "");
			trap_Cvar_Set("ui_cl_packetdup", "");
			trap_Cvar_Set("ui_sensitivity", "");
			trap_Cvar_Set("ui_r_colorbits", "");
			trap_Cvar_Set("ui_r_fullscreen", "");
			trap_Cvar_Set("ui_r_lodbias", "");
			trap_Cvar_Set("ui_r_subdivisions", "");
			trap_Cvar_Set("ui_r_picmip", "");
			trap_Cvar_Set("ui_r_texturebits", "");
			trap_Cvar_Set("ui_r_depthbits", "");
			trap_Cvar_Set("ui_r_ext_compressed_textures", "");
			trap_Cvar_Set("ui_r_finish", "");
			trap_Cvar_Set("ui_r_dynamiclight", "");
			trap_Cvar_Set("ui_r_allowextensions", "");
			trap_Cvar_Set("ui_m_filter", "");
			trap_Cvar_Set("ui_s_khz", "");
			trap_Cvar_Set("ui_r_detailtextures", "");
			trap_Cvar_Set("ui_r_texturemode", "");
		} else if (Q_stricmp(name, "systemCvarsApply") == 0) {
			int   ui_r_mode                    = trap_Cvar_VariableValue("ui_r_mode");
			float ui_r_gamma                   = trap_Cvar_VariableValue("ui_r_gamma");
			int   ui_rate                      = trap_Cvar_VariableValue("ui_rate");
			int   ui_cl_maxpackets             = trap_Cvar_VariableValue("ui_cl_maxpackets");
			int   ui_cl_packetdup              = trap_Cvar_VariableValue("ui_cl_packetdup");
			float ui_sensitivity               = trap_Cvar_VariableValue("ui_sensitivity");
			int   ui_r_colorbits               = trap_Cvar_VariableValue("ui_r_colorbits");
			int   ui_r_fullscreen              = trap_Cvar_VariableValue("ui_r_fullscreen");
			int   ui_r_lodbias                 = trap_Cvar_VariableValue("ui_r_lodbias");
			int   ui_r_subdivisions            = trap_Cvar_VariableValue("ui_r_subdivisions");
			int   ui_r_picmip                  = trap_Cvar_VariableValue("ui_r_picmip");
			int   ui_r_texturebits             = trap_Cvar_VariableValue("ui_r_texturebits");
			int   ui_r_depthbits               = trap_Cvar_VariableValue("ui_r_depthbits");
			int   ui_r_ext_compressed_textures = trap_Cvar_VariableValue("ui_r_ext_compressed_textures");
			int   ui_r_finish                  = trap_Cvar_VariableValue("ui_r_finish");
			int   ui_r_dynamiclight            = trap_Cvar_VariableValue("ui_r_dynamiclight");
			int   ui_r_allowextensions         = trap_Cvar_VariableValue("ui_r_allowextensions");
			int   ui_m_filter                  = trap_Cvar_VariableValue("ui_m_filter");
			int   ui_s_khz                     = trap_Cvar_VariableValue("ui_s_khz");
			int   ui_r_detailtextures          = trap_Cvar_VariableValue("ui_r_detailtextures");
			char  ui_r_texturemode[MAX_CVAR_VALUE_STRING];
			trap_Cvar_VariableStringBuffer("ui_r_texturemode", ui_r_texturemode, sizeof (ui_r_texturemode));

			// failsafe
			if (ui_rate == 0) {
				ui_rate          = 5000;
				ui_cl_maxpackets = 30;
				ui_cl_packetdup  = 1;
			}

			trap_Cvar_Set("r_mode", va("%i", ui_r_mode));
			trap_Cvar_Set("r_gamma", va("%f", ui_r_gamma));
			trap_Cvar_Set("rate", va("%i", ui_rate));
			trap_Cvar_Set("cl_maxpackets", va("%i", ui_cl_maxpackets));
			trap_Cvar_Set("cl_packetdup", va("%i", ui_cl_packetdup));
			trap_Cvar_Set("sensitivity", va("%f", ui_sensitivity));
			trap_Cvar_Set("r_colorbits", va("%i", ui_r_colorbits));
			trap_Cvar_Set("r_fullscreen", va("%i", ui_r_fullscreen));
			trap_Cvar_Set("r_lodbias", va("%i", ui_r_lodbias));
			trap_Cvar_Set("r_subdivisions", va("%i", ui_r_subdivisions));
			trap_Cvar_Set("r_picmip", va("%i", ui_r_picmip));
			trap_Cvar_Set("r_texturebits", va("%i", ui_r_texturebits));
			trap_Cvar_Set("r_depthbits", va("%i", ui_r_depthbits));
			trap_Cvar_Set("r_ext_compressed_textures", va("%i", ui_r_ext_compressed_textures));
			trap_Cvar_Set("r_finish", va("%i", ui_r_finish));
			trap_Cvar_Set("r_dynamiclight", va("%i", ui_r_dynamiclight));
			trap_Cvar_Set("r_allowextensions", va("%i", ui_r_allowextensions));
			trap_Cvar_Set("m_filter", va("%i", ui_m_filter));
			trap_Cvar_Set("s_khz", va("%i", ui_s_khz));
			trap_Cvar_Set("r_detailtextures", va("%i", ui_r_detailtextures));
			trap_Cvar_Set("r_texturemode", ui_r_texturemode);

			trap_Cvar_Set("ui_r_mode", "");
			trap_Cvar_Set("ui_r_gamma", "");
			trap_Cvar_Set("ui_rate", "");
			trap_Cvar_Set("ui_cl_maxpackets", "");
			trap_Cvar_Set("ui_cl_packetdup", "");
			trap_Cvar_Set("ui_sensitivity", "");
			trap_Cvar_Set("ui_r_colorbits", "");
			trap_Cvar_Set("ui_r_fullscreen", "");
			trap_Cvar_Set("ui_r_lodbias", "");
			trap_Cvar_Set("ui_r_subdivisions", "");
			trap_Cvar_Set("ui_r_picmip", "");
			trap_Cvar_Set("ui_r_texturebits", "");
			trap_Cvar_Set("ui_r_depthbits", "");
			trap_Cvar_Set("ui_r_ext_compressed_textures", "");
			trap_Cvar_Set("ui_r_finish", "");
			trap_Cvar_Set("ui_r_dynamiclight", "");
			trap_Cvar_Set("ui_r_allowextensions", "");
			trap_Cvar_Set("ui_m_filter", "");
			trap_Cvar_Set("ui_s_khz", "");
			trap_Cvar_Set("ui_r_detailtextures", "");
			trap_Cvar_Set("ui_r_texturemode", "");
		} else if (Q_stricmp(name, "profileCvarsGet") == 0) {
			int ui_profile_mousePitch = trap_Cvar_VariableValue("ui_mousePitch");

			trap_Cvar_Set("ui_profile_mousePitch", va("%i", ui_profile_mousePitch));
		} else if (Q_stricmp(name, "profileCvarsApply") == 0) {
			int ui_handedness         = trap_Cvar_VariableValue("ui_handedness");
			int ui_profile_mousePitch = trap_Cvar_VariableValue("ui_profile_mousePitch");

			trap_Cvar_Set("ui_mousePitch", va("%i", ui_profile_mousePitch));

			if (ui_profile_mousePitch == 0) {
				trap_Cvar_SetValue("m_pitch", 0.022f);
			} else {
				trap_Cvar_SetValue("m_pitch", -0.022f);
			}

			if (ui_handedness == 0) {
				// exec default.cfg
				trap_Cmd_ExecuteText(EXEC_APPEND, "exec default.cfg\n");
				Controls_SetDefaults(qfalse);
			} else {
				// exec default_left.cfg
				trap_Cmd_ExecuteText(EXEC_APPEND, "exec default_left.cfg\n");
				Controls_SetDefaults(qtrue);
			}

			trap_Cvar_Set("ui_handedness", "");
			trap_Cvar_Set("ui_profile_mousePitch", "");
		} else if (Q_stricmp(name, "profileCvarsReset") == 0) {
			trap_Cvar_Set("ui_handedness", "");
			trap_Cvar_Set("ui_profile_mousePitch", "");
		} else if (Q_stricmp(name, "defaultControls") == 0) {
			int ui_handedness = trap_Cvar_VariableValue("ui_handedness");

			if (ui_handedness == 0) {
				// exec default.cfg
				trap_Cmd_ExecuteText(EXEC_APPEND, "exec default.cfg\n");
				Controls_SetDefaults(qfalse);
			} else {
				// exec default_left.cfg
				trap_Cmd_ExecuteText(EXEC_APPEND, "exec default_left.cfg\n");
				Controls_SetDefaults(qtrue);
			}
		} else {
			Com_Printf("^3WARNING: unknown UI script %s\n", name);
		}
	}
}

/*
==================
UI_MapCountByGameType
==================
*/
static int UI_MapCountByGameType(qboolean singlePlayer) {
	int i, c;

	c = 0;
	for (i = 0; i < uiInfo.mapCount; ++i) {
		uiInfo.mapList[i].active = qfalse;
		if (singlePlayer) {
			continue;
		}
		c++;
		uiInfo.mapList[i].active = qtrue;
	}
	return c;
}

/*
==================
UI_InsertServerIntoDisplayList
==================
*/
static void UI_InsertServerIntoDisplayList(int num, int position) {
	int i;

	if (position < 0 || position > uiInfo.serverStatus.numDisplayServers) {
		return;
	}
	//
	uiInfo.serverStatus.numDisplayServers++;
	for (i = uiInfo.serverStatus.numDisplayServers; i > position; --i) {
		uiInfo.serverStatus.displayServers[i] = uiInfo.serverStatus.displayServers[i - 1];
	}
	uiInfo.serverStatus.displayServers[position] = num;
}

/*
==================
UI_RemoveServerFromDisplayList
==================
*/
static void UI_RemoveServerFromDisplayList(int num) {
	int i, j;

	for (i = 0; i < uiInfo.serverStatus.numDisplayServers; ++i) {
		if (uiInfo.serverStatus.displayServers[i] == num) {
			uiInfo.serverStatus.numDisplayServers--;
			for (j = i; j < uiInfo.serverStatus.numDisplayServers; ++j) {
				uiInfo.serverStatus.displayServers[j] = uiInfo.serverStatus.displayServers[j + 1];
			}
			return;
		}
	}
}

/*
==================
UI_BinaryServerInsertion
==================
*/
static void UI_BinaryServerInsertion(int num) {
	int mid, offset, res, len;

	// use binary search to insert server
	len    = uiInfo.serverStatus.numDisplayServers;
	mid    = len;
	offset = 0;
	res    = 0;
	while (mid > 0) {
		mid = len >> 1;
		//
		res = trap_LAN_CompareServers(ui_netSource.integer, uiInfo.serverStatus.sortKey,
		                              uiInfo.serverStatus.sortDir, num, uiInfo.serverStatus.displayServers[offset + mid]);
		// if equal
		if (res == 0) {
			UI_InsertServerIntoDisplayList(num, offset + mid);
			return;
		}
		// if larger
		else if (res == 1) {
			offset += mid;
			len    -= mid;
		}
		// if smaller
		else {
			len -= mid;
		}
	}
	if (res == 1) {
		offset++;
	}
	UI_InsertServerIntoDisplayList(num, offset);
}

/*
==================
UI_BuildServerDisplayList
==================
*/
static void UI_BuildServerDisplayList(int force) {
	int        i, count, clients, maxClients, ping, len, antilag, password;
	char       info[MAX_STRING_CHARS];
	static int numinvisible;

	if (!(force || uiInfo.uiDC.realTime > uiInfo.serverStatus.nextDisplayRefresh)) {
		return;
	}

	// if we shouldn't reset
	if (force == 2) {
		force = 0;
	}

	// do motd updates here too
	trap_Cvar_VariableStringBuffer("cl_motdString", uiInfo.serverStatus.motd, sizeof (uiInfo.serverStatus.motd));
	len = strlen(uiInfo.serverStatus.motd);
	if (len == 0) {
		Q_strncpyz(uiInfo.serverStatus.motd, va("Enemy Territory - Mod: %s", GAME_VERSION), sizeof (uiInfo.serverStatus.motd));
		len = strlen(uiInfo.serverStatus.motd);
	}
	if (len != uiInfo.serverStatus.motdLen) {
		uiInfo.serverStatus.motdLen   = len;
		uiInfo.serverStatus.motdWidth = -1;
	}

	if (force) {
		numinvisible = 0;
		// clear number of displayed servers
		uiInfo.serverStatus.numDisplayServers   = 0;
		uiInfo.serverStatus.numPlayersOnServers = 0;
		// set list box index to zero
		Menu_SetFeederSelection(NULL, FEEDER_SERVERS, 0, NULL);
		// mark all servers as visible so we store ping updates for them
		trap_LAN_MarkServerVisible(ui_netSource.integer, -1, qtrue);
	}

	// get the server count (comes from the master)
	count = trap_LAN_GetServerCount(ui_netSource.integer);
	if (count == -1 || (ui_netSource.integer == AS_LOCAL && count == 0)) {
		// still waiting on a response from the master
		uiInfo.serverStatus.numDisplayServers    = 0;
		uiInfo.serverStatus.numPlayersOnServers  = 0;
		uiInfo.serverStatus.nextDisplayRefresh   = uiInfo.uiDC.realTime + 500;
		uiInfo.serverStatus.currentServerPreview = 0;
		return;
	}

	if (!uiInfo.serverStatus.numDisplayServers) {
		uiInfo.serverStatus.currentServerPreview = 0;
	}

	for (i = 0; i < count; ++i) {
		// if we already got info for this server
		if (!trap_LAN_ServerIsVisible(ui_netSource.integer, i)) {
			continue;
		}

		// get the ping for this server
		ping = trap_LAN_GetServerPing(ui_netSource.integer, i);
		if (ping > 0 || ui_netSource.integer == AS_FAVORITES) {

			trap_LAN_GetServerInfo(ui_netSource.integer, i, info, MAX_STRING_CHARS);

			clients                                  = atoi(Info_ValueForKey(info, "clients"));
			uiInfo.serverStatus.numPlayersOnServers += clients;

			trap_Cvar_Update(&ui_browserShowEmptyOrFull);
			if (ui_browserShowEmptyOrFull.integer) {
				maxClients = atoi(Info_ValueForKey(info, "sv_maxclients"));

				if (clients != maxClients && (
						(!clients && ui_browserShowEmptyOrFull.integer == 2) ||
						(clients && ui_browserShowEmptyOrFull.integer == 1))) {
					trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
					continue;
				}

				if (clients && (
						(clients == maxClients && ui_browserShowEmptyOrFull.integer == 2) ||
						(clients != maxClients && ui_browserShowEmptyOrFull.integer == 1))) {
					trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
					continue;
				}
			}

			trap_Cvar_Update(&ui_browserShowPasswordProtected);
			if (ui_browserShowPasswordProtected.integer) {
				password = atoi(Info_ValueForKey(info, "needpass"));
				if ((password && ui_browserShowPasswordProtected.integer == 2) ||
				    (!password && ui_browserShowPasswordProtected.integer == 1)) {
					trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
					continue;
				}
			}

			trap_Cvar_Update(&ui_browserShowAntilag);
			if (ui_browserShowAntilag.integer) {
				antilag = atoi(Info_ValueForKey(info, "g_antilag"));

				if ((antilag && ui_browserShowAntilag.integer == 2) ||
				    (!antilag && ui_browserShowAntilag.integer == 1)) {
					trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
					continue;
				}
			}

			// make sure we never add a favorite server twice
			if (ui_netSource.integer == AS_FAVORITES) {
				UI_RemoveServerFromDisplayList(i);
			}
			// insert the server into the list
			if (uiInfo.serverStatus.numDisplayServers == 0) {
				char *s = Info_ValueForKey(info, "mapname");

				if (s && *s) {
					uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip(va("levelshots/%s", Info_ValueForKey(info, "mapname")));
				} else {
					uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip("levelshots/unknownmap");
				}
			}

			UI_BinaryServerInsertion(i);
			// done with this server
			if (ping > /*=*/ 0) {
				trap_LAN_MarkServerVisible(ui_netSource.integer, i, qfalse);
				numinvisible++;
			}
		}
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime;
}

typedef struct {
	char *name, *altName;
} serverStatusCvar_t;

serverStatusCvar_t serverStatusCvars[] =
{
	{ "sv_hostname", "Name"      },
	{ "Address",     ""          },
	{ "gamename",    "Game name" },
	{ "mapname",     "Map"       },
	{ "version",     ""          },
	{ "protocol",    ""          },
	{ NULL,          NULL        }
};

/*
==================
UI_SortServerStatusInfo
==================
*/
static void UI_SortServerStatusInfo(serverStatusInfo_t *info) {
	int  i, j, index;
	char *tmp1, *tmp2;

	// FIXME: if "gamename" == "baseq3" or "missionpack" then
	// replace the gametype number by FFA, CTF etc.
	//
	index = 0;
	for (i = 0; serverStatusCvars[i].name; ++i) {
		for (j = 0; j < info->numLines; ++j) {
			if (!info->lines[j][1] || info->lines[j][1][0]) {
				continue;
			}
			if (!Q_stricmp(serverStatusCvars[i].name, info->lines[j][0])) {
				// swap lines
				tmp1                  = info->lines[index][0];
				tmp2                  = info->lines[index][3];
				info->lines[index][0] = info->lines[j][0];
				info->lines[index][3] = info->lines[j][3];
				info->lines[j][0]     = tmp1;
				info->lines[j][3]     = tmp2;
				//
				if (strlen(serverStatusCvars[i].altName)) {
					info->lines[index][0] = serverStatusCvars[i].altName;
				}
				index++;
			}
		}
	}
}

/*
==================
UI_GetServerStatusInfo
==================
*/
static int UI_GetServerStatusInfo(const char *serverAddress, serverStatusInfo_t *info) {
	char *p, *score, *ping, *name, *p_val = NULL, *p_name = NULL;

	if (!info) {
		trap_LAN_ServerStatus(serverAddress, NULL, 0);
		return qfalse;
	}
	memset(info, 0, sizeof (*info));
	if (trap_LAN_ServerStatus(serverAddress, info->text, sizeof (info->text))) {
		menuDef_t *menu, *menu2; // we use the URL buttons in several menus

		menu  = Menus_FindByName("serverinfo_popmenu");
		menu2 = Menus_FindByName("popupError");

		Q_strncpyz(info->address, serverAddress, sizeof (info->address));
		p                              = info->text;
		info->numLines                 = 0;
		info->lines[info->numLines][0] = "Address";
		info->lines[info->numLines][1] = "";
		info->lines[info->numLines][2] = "";
		info->lines[info->numLines][3] = info->address;
		info->numLines++;
		// cleanup of the URL cvars
		trap_Cvar_Set("ui_URL", "");
		trap_Cvar_Set("ui_modURL", "");
		// get the cvars
		while (p && *p) {
			p = strchr(p, '\\');
			if (!p) {
				break;
			}
			*p++ = '\0';
			if (p_name) {
				if (!Q_stricmp(p_name, "url")) {
					trap_Cvar_Set("ui_URL", p_val);
					if (menu) {
						Menu_ShowItemByName(menu, "serverURL", qtrue);
					}
					if (menu2) {
						Menu_ShowItemByName(menu2, "serverURL", qtrue);
					}
				} else if (!Q_stricmp(p_name, "mod_url")) {
					trap_Cvar_Set("ui_modURL", p_val);
					if (menu) {
						Menu_ShowItemByName(menu, "modURL", qtrue);
					}
					if (menu2) {
						Menu_ShowItemByName(menu2, "modURL", qtrue);
					}
				}
			}
			if (*p == '\\') {
				break;
			}
			p_name                         = p;
			info->lines[info->numLines][0] = p;
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			p                              = strchr(p, '\\');
			if (!p) {
				break;
			}
			*p++                           = '\0';
			p_val                          = p;
			info->lines[info->numLines][3] = p;

			info->numLines++;
			if (info->numLines >= MAX_SERVERSTATUS_LINES) {
				break;
			}
		}
		// get the player list
		if (info->numLines < MAX_SERVERSTATUS_LINES - 3) {
			int len = 0, i = 0;

			// empty line
			info->lines[info->numLines][0] = "";
			info->lines[info->numLines][1] = "";
			info->lines[info->numLines][2] = "";
			info->lines[info->numLines][3] = "";
			info->numLines++;
			// header
			info->lines[info->numLines][0] = "num";
			info->lines[info->numLines][1] = "score";
			info->lines[info->numLines][2] = "ping";
			info->lines[info->numLines][3] = "name";
			info->numLines++;
			// parse players
			while (p && *p) {
				if (*p == '\\') {
					*p++ = '\0';
				}
				score = p;
				p     = strchr(p, ' ');
				if (!p) {
					break;
				}
				*p++ = '\0';
				ping = p;
				p    = strchr(p, ' ');
				if (!p) {
					break;
				}
				*p++ = '\0';
				name = p;
				Com_sprintf(&info->pings[len], sizeof (info->pings) - len, "%d", i);
				info->lines[info->numLines][0] = &info->pings[len];
				len                           += strlen(&info->pings[len]) + 1;
				info->lines[info->numLines][1] = score;
				info->lines[info->numLines][2] = ping;
				info->lines[info->numLines][3] = name;
				info->numLines++;
				if (info->numLines >= MAX_SERVERSTATUS_LINES) {
					break;
				}
				p = strchr(p, '\\');
				if (!p) {
					break;
				}
				*p++ = '\0';
				i++;
			}
		}
		UI_SortServerStatusInfo(info);
		return qtrue;
	}
	return qfalse;
}

/*
==================
stristr
==================
*/
static char *stristr(char *str, char *charset) {
	int i;

	while (*str) {
		for (i = 0; charset[i] && str[i]; ++i) {
			if (toupper(charset[i]) != toupper(str[i])) {
				break;
			}
		}
		if (!charset[i]) {
			return str;
		}
		str++;
	}
	return NULL;
}

/*
==================
UI_BuildFindPlayerList
==================
*/
static void UI_BuildFindPlayerList(qboolean force) {
	static int         numFound, numTimeOuts;
	int                i, j;
	serverStatusInfo_t info;
	char               name[MAX_NAME_LENGTH + 2];
	char               infoString[MAX_STRING_CHARS];

	if (!force) {
		if (!uiInfo.nextFindPlayerRefresh || uiInfo.nextFindPlayerRefresh > uiInfo.uiDC.realTime) {
			return;
		}
	} else {
		int resend;

		memset(&uiInfo.pendingServerStatus, 0, sizeof (uiInfo.pendingServerStatus));
		uiInfo.numFoundPlayerServers    = 0;
		uiInfo.currentFoundPlayerServer = 0;
		trap_Cvar_VariableStringBuffer("ui_findPlayer", uiInfo.findPlayerName, sizeof (uiInfo.findPlayerName));
		Q_CleanStr(uiInfo.findPlayerName);
		// should have a string of some length
		if (!strlen(uiInfo.findPlayerName)) {
			uiInfo.nextFindPlayerRefresh = 0;
			return;
		}
		// set resend time
		resend = ui_serverStatusTimeOut.integer / 2 - 10;
		if (resend < 50) {
			resend = 50;
		}
		trap_Cvar_Set("cl_serverStatusResendTime", va("%d", resend));
		// reset all server status requests
		trap_LAN_ServerStatus(NULL, NULL, 0);
		//
		uiInfo.numFoundPlayerServers = 1;
		Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1],
		            sizeof (uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1]),
		            "searching %d...", uiInfo.pendingServerStatus.num);
		numFound = 0;
		numTimeOuts++;
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; ++i) {
		// if this pending server is valid
		if (uiInfo.pendingServerStatus.server[i].valid && UI_GetServerStatusInfo(uiInfo.pendingServerStatus.server[i].adrstr, &info)) {
			numFound++;
			// parse through the server status lines
			for (j = 0; j < info.numLines; ++j) {
				// should have ping info
				if (!info.lines[j][2] || !info.lines[j][2][0]) {
					continue;
				}
				// clean string first
				Q_strncpyz(name, info.lines[j][3], sizeof (name));
				Q_CleanStr(name);
				// if the player name is a substring
				if (stristr(name, uiInfo.findPlayerName)) {
					// add to found server list if we have space (always leave space for a line with the number found)
					if (uiInfo.numFoundPlayerServers < MAX_FOUNDPLAYER_SERVERS - 1) {
						//
						Q_strncpyz(uiInfo.foundPlayerServerAddresses[uiInfo.numFoundPlayerServers - 1],
						           uiInfo.pendingServerStatus.server[i].adrstr,
						           sizeof (uiInfo.foundPlayerServerAddresses[0]));
						Q_strncpyz(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1],
						           uiInfo.pendingServerStatus.server[i].name,
						           sizeof (uiInfo.foundPlayerServerNames[0]));
						uiInfo.numFoundPlayerServers++;
					} else {
						// can't add any more so we're done
						uiInfo.pendingServerStatus.num = uiInfo.serverStatus.numDisplayServers;
					}
				}
			}
			Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1],
			            sizeof (uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1]),
			            "searching %d/%d...", uiInfo.pendingServerStatus.num, numFound);
			// retrieved the server status so reuse this spot
			uiInfo.pendingServerStatus.server[i].valid = qfalse;
		}
		// if empty pending slot or timed out
		if (!uiInfo.pendingServerStatus.server[i].valid ||
		    uiInfo.pendingServerStatus.server[i].startTime < uiInfo.uiDC.realTime - ui_serverStatusTimeOut.integer) {
			if (uiInfo.pendingServerStatus.server[i].valid) {
				numTimeOuts++;
			}
			// reset server status request for this address
			UI_GetServerStatusInfo(uiInfo.pendingServerStatus.server[i].adrstr, NULL);
			// reuse pending slot
			uiInfo.pendingServerStatus.server[i].valid = qfalse;
			// if we didn't try to get the status of all servers in the main browser yet
			if (uiInfo.pendingServerStatus.num < uiInfo.serverStatus.numDisplayServers) {
				uiInfo.pendingServerStatus.server[i].startTime = uiInfo.uiDC.realTime;
				trap_LAN_GetServerAddressString(ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num],
				                                uiInfo.pendingServerStatus.server[i].adrstr, sizeof (uiInfo.pendingServerStatus.server[i].adrstr));
				trap_LAN_GetServerInfo(ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.pendingServerStatus.num], infoString, sizeof (infoString));
				Q_strncpyz(uiInfo.pendingServerStatus.server[i].name, Info_ValueForKey(infoString, "hostname"), sizeof (uiInfo.pendingServerStatus.server[0].name));
				uiInfo.pendingServerStatus.server[i].valid = qtrue;
				uiInfo.pendingServerStatus.num++;
				Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1],
				            sizeof (uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1]),
				            "searching %d/%d...", uiInfo.pendingServerStatus.num, numFound);
			}
		}
	}
	for (i = 0; i < MAX_SERVERSTATUSREQUESTS; ++i) {
		if (uiInfo.pendingServerStatus.server[i].valid) {
			break;
		}
	}
	// if still trying to retrieve server status info
	if (i < MAX_SERVERSTATUSREQUESTS) {
		uiInfo.nextFindPlayerRefresh = uiInfo.uiDC.realTime + 25;
	} else {
		// add a line that shows the number of servers found
		if (!uiInfo.numFoundPlayerServers) {
			Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1], sizeof (uiInfo.foundPlayerServerAddresses[0]), "no servers found");
		} else {
			Com_sprintf(uiInfo.foundPlayerServerNames[uiInfo.numFoundPlayerServers - 1], sizeof (uiInfo.foundPlayerServerAddresses[0]),
			            "%d server%s found with player %s", uiInfo.numFoundPlayerServers - 1,
			            uiInfo.numFoundPlayerServers == 2 ? "" : "s", uiInfo.findPlayerName);
		}
		uiInfo.nextFindPlayerRefresh = 0;
		// show the server status info for the selected server
		UI_FeederSelection(FEEDER_FINDPLAYER, uiInfo.currentFoundPlayerServer);
	}
}

/*
==================
UI_BuildServerStatus
==================
*/
static void UI_BuildServerStatus(qboolean force) {
	uiClientState_t cstate;

	trap_GetClientState(&cstate);

	if (uiInfo.nextFindPlayerRefresh) {
		return;
	}
	if (!force) {
		if (!uiInfo.nextServerStatusRefresh || uiInfo.nextServerStatusRefresh > uiInfo.uiDC.realTime) {
			return;
		}
	} else {
		menuDef_t *menu;

		Menu_SetFeederSelection(NULL, FEEDER_SERVERSTATUS, 0, NULL);
		uiInfo.serverStatusInfo.numLines = 0;
		// TTimo - reset the server URL / mod URL till we get the new ones
		// the URL buttons are used in the two menus, serverinfo_popmenu and error_popmenu_diagnose
		menu = Menus_FindByName("serverinfo_popmenu");
		if (menu) {
			Menu_ShowItemByName(menu, "serverURL", qfalse);
			Menu_ShowItemByName(menu, "modURL", qfalse);
		}
		menu = Menus_FindByName("popupError");
		if (menu) {
			Menu_ShowItemByName(menu, "serverURL", qfalse);
			Menu_ShowItemByName(menu, "modURL", qfalse);
		}
		// reset all server status requests
		trap_LAN_ServerStatus(NULL, NULL, 0);
	}
	if (cstate.connState < CA_CONNECTED &&
	    (uiInfo.serverStatus.currentServer < 0 || uiInfo.serverStatus.currentServer > uiInfo.serverStatus.numDisplayServers || uiInfo.serverStatus.numDisplayServers == 0)) {
		return;
	}
	if (UI_GetServerStatusInfo(uiInfo.serverStatusAddress, &uiInfo.serverStatusInfo)) {
		uiInfo.nextServerStatusRefresh = 0;
		UI_GetServerStatusInfo(uiInfo.serverStatusAddress, NULL);
	} else {
		uiInfo.nextServerStatusRefresh = uiInfo.uiDC.realTime + 500;
	}
}

/*
==================
UI_FeederCount
==================
*/
static int UI_FeederCount(float feederID) {
	if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		return UI_MapCountByGameType(feederID == FEEDER_MAPS ? qtrue : qfalse);
	} else if (feederID == FEEDER_GLINFO) {
		return uiInfo.numGlInfoLines;
	} else if (feederID == FEEDER_PROFILES) {
		return uiInfo.profileCount;
	} else if (feederID == FEEDER_SERVERS) {
		return uiInfo.serverStatus.numDisplayServers;
	} else if (feederID == FEEDER_SERVERSTATUS) {
		return uiInfo.serverStatusInfo.numLines;
	} else if (feederID == FEEDER_FINDPLAYER) {
		return uiInfo.numFoundPlayerServers;
	} else if (feederID == FEEDER_PLAYER_LIST) {
		if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}
		return uiInfo.playerCount;
	} else if (feederID == FEEDER_TEAM_LIST) {
		if (uiInfo.uiDC.realTime > uiInfo.playerRefresh) {
			uiInfo.playerRefresh = uiInfo.uiDC.realTime + 3000;
			UI_BuildPlayerList();
		}
		return uiInfo.myTeamCount;
	} else if (feederID == FEEDER_MODS) {
		return uiInfo.modCount;
	} else if (feederID == FEEDER_DEMOS) {
		return uiInfo.demoCount;
	}
	return 0;
}

static const char *UI_SelectedMap(int index, int *actual) {
	int i, c;

	c       = 0;
	*actual = 0;

	for (i = 0; i < uiInfo.mapCount; ++i) {
		if (uiInfo.mapList[i].active) {
			if (c == index) {
				*actual = i;
				return uiInfo.mapList[i].mapName;
			}
			c++;
		}
	}

	return "";
}

static void UI_UpdatePendingPings() {
	trap_LAN_ResetPings(ui_netSource.integer);
	uiInfo.serverStatus.refreshActive = qtrue;
	uiInfo.serverStatus.refreshtime   = uiInfo.uiDC.realTime + 1000;
}

//----(SA)	added (whoops, this got nuked in a check-in...)
static const char *UI_FileText(char *fileName) {
	int          len;
	fileHandle_t f;
	static char  buf[MAX_MENUDEFFILE];

	len = trap_FS_FOpenFile(fileName, &f, FS_READ);
	if (!f) {
		return NULL;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);
	return &buf[0];
}
//----(SA)	end

const char *UI_FeederItemText(float feederID, int index, int column, qhandle_t *handles, int *numhandles) {
	*numhandles = 0;
	if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		int actual;
		return UI_SelectedMap(index, &actual);
	} else if (feederID == FEEDER_GLINFO) {
		if (index == 0) {
			return va("Vendor: %s", uiInfo.uiDC.glconfig.vendor_string);
		} else if (index == 1) {
			return va("Version: %s: %s", uiInfo.uiDC.glconfig.version_string, uiInfo.uiDC.glconfig.renderer_string);
		} else if (index == 2) {
			return va("Pixelformat: color(%d-bits) Z(%d-bits) stencil(%d-bits)", uiInfo.uiDC.glconfig.colorBits, uiInfo.uiDC.glconfig.depthBits, uiInfo.uiDC.glconfig.stencilBits);
		} else if (index >= 4 && index < uiInfo.numGlInfoLines) {
			return uiInfo.glInfoLines[index - 4];
		}
		return "";
	} else if (feederID == FEEDER_SERVERS) {
		if (index >= 0 && index < uiInfo.serverStatus.numDisplayServers) {
			int         ping, antilag, needpass, serverload;
			static char info[MAX_STRING_CHARS];
			static char clientBuff[32];
			static char pingstr[10];
			static int  lastColumn = -1;
			static int  lastTime   = 0;

			if (lastColumn != column || lastTime > uiInfo.uiDC.realTime + 5000) {
				trap_LAN_GetServerInfo(ui_netSource.integer, uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS);
				lastColumn = column;
				lastTime   = uiInfo.uiDC.realTime;
			}
			ping = atoi(Info_ValueForKey(info, "ping"));
			switch (column) {
			case SORT_HOST:
				if (ping <= 0) {
					return Info_ValueForKey(info, "addr");
				}
				if (ui_netSource.integer == AS_LOCAL) {
					static char hostname[1024];

					Com_sprintf(hostname, sizeof (hostname), "%s [%s]",
					            Info_ValueForKey(info, "hostname"),
					            netnames[atoi(Info_ValueForKey(info, "nettype"))]);
					return hostname;
				}
				return Info_ValueForKey(info, "hostname");
			case SORT_MAP:
				return Info_ValueForKey(info, "mapname");
			case SORT_CLIENTS:
				Com_sprintf(clientBuff, sizeof (clientBuff), "%s/%s", Info_ValueForKey(info, "clients"), Info_ValueForKey(info, "sv_maxclients"));
				return clientBuff;
			case SORT_PING:
				if (ping <= 0) {
					return "...";
				}
				serverload = atoi(Info_ValueForKey(info, "serverload"));
				if (serverload == -1) {
					Com_sprintf(pingstr, sizeof (pingstr), " %3i", ping);
				} else if (serverload > 75) {
					Com_sprintf(pingstr, sizeof (pingstr), "^1 %3i", ping);
				} else if (serverload > 40) {
					Com_sprintf(pingstr, sizeof (pingstr), "^3 %3i", ping);
				} else {
					Com_sprintf(pingstr, sizeof (pingstr), "^2 %3i", ping);
				}
				return pingstr;
			case SORT_FILTERS:
				if (ping <= 0) {
					*numhandles = 0;
					return "";
				}
				*numhandles = 3;
				needpass    = atoi(Info_ValueForKey(info, "needpass"));
				antilag     = atoi(Info_ValueForKey(info, "g_antilag"));

				if (needpass) {
					handles[0] = uiInfo.passwordFilter;
				} else {
					handles[0] = -1;
				}
				handles[1] = -1; // Nico, removed pb
				if (antilag) {
					handles[2] = uiInfo.antiLagFilter;
				} else {
					handles[2] = -1;
				}
				return "";
			case SORT_FAVOURITES:
				*numhandles = 1;

				if (trap_LAN_ServerIsInFavoriteList(ui_netSource.integer, uiInfo.serverStatus.displayServers[index])) {
					handles[0] = uiInfo.uiDC.Assets.checkboxCheck;
				} else {
					handles[0] = uiInfo.uiDC.Assets.checkboxCheckNot;
				}
				return "";
			}
		}
	} else if (feederID == FEEDER_SERVERSTATUS) {
		if (index >= 0 && index < uiInfo.serverStatusInfo.numLines && column >= 0 && column < 4) {
			return uiInfo.serverStatusInfo.lines[index][column];
		}
	} else if (feederID == FEEDER_FINDPLAYER) {
		if (index >= 0 && index < uiInfo.numFoundPlayerServers) {
			return uiInfo.foundPlayerServerNames[index];
		}
	} else if (feederID == FEEDER_PLAYER_LIST) {
		if (index >= 0 && index < uiInfo.playerCount) {
			return uiInfo.playerNames[index];
		}
	} else if (feederID == FEEDER_TEAM_LIST) {
		if (index >= 0 && index < uiInfo.myTeamCount) {
			return uiInfo.teamNames[index];
		}
	} else if (feederID == FEEDER_MODS) {
		if (index >= 0 && index < uiInfo.modCount) {
			if (uiInfo.modList[index].modDescr && *uiInfo.modList[index].modDescr) {
				return uiInfo.modList[index].modDescr;
			}
			return uiInfo.modList[index].modName;
		}
	} else if (feederID == FEEDER_DEMOS) {
		if (index >= 0 && index < uiInfo.demoCount) {
			return uiInfo.demoList[index];
		}
	} else if (feederID == FEEDER_PROFILES && index >= 0 && index < uiInfo.profileCount) {
		char buff[MAX_CVAR_VALUE_STRING];

		Q_strncpyz(buff, uiInfo.profileList[index].name, sizeof (buff));
		Q_CleanStr(buff);
		Q_CleanDirName(buff);

		if (!Q_stricmp(buff, cl_profile.string)) {
			if (!Q_stricmp(buff, cl_defaultProfile.string)) {
				return va("^7(Default) %s", uiInfo.profileList[index].name);
			}
			return va("^7%s", uiInfo.profileList[index].name);
		} else if (!Q_stricmp(buff, cl_defaultProfile.string)) {
			return va("(Default) %s", uiInfo.profileList[index].name);
		}
		return uiInfo.profileList[index].name;
	}
	// -NERVE - SMF
	return "";
}

static qhandle_t UI_FeederItemImage(float feederID, int index) {
	if (feederID == FEEDER_ALLMAPS || feederID == FEEDER_MAPS) {
		int actual;

		UI_SelectedMap(index, &actual);
		index = actual;

		if (index >= 0 && index < uiInfo.mapCount) {
			if (uiInfo.mapList[index].levelShot == -1) {
				uiInfo.mapList[index].levelShot = trap_R_RegisterShaderNoMip(uiInfo.mapList[index].imageName);
			}
			return uiInfo.mapList[index].levelShot;
		}
	}

	return 0;
}

void UI_FeederSelection(float feederID, int index) {
	if (feederID == FEEDER_MAPS || feederID == FEEDER_ALLMAPS) {
		int actual;

		UI_SelectedMap(index, &actual);
		trap_Cvar_Set("ui_mapIndex", va("%d", index));
		ui_mapIndex.integer = index;

		// NERVE - SMF - setup advanced server vars
		if (feederID == FEEDER_ALLMAPS) {
			ui_currentMap.integer = actual;
			trap_Cvar_Set("ui_currentMap", va("%d", actual));
		}
		// -NERVE - SMF

		if (feederID == FEEDER_MAPS) {
			ui_currentMap.integer = actual;
			trap_Cvar_Set("ui_currentMap", va("%d", actual));
		} else {
			ui_currentNetMap.integer = actual;
			trap_Cvar_Set("ui_currentNetMap", va("%d", actual));
		}
	} else if (feederID == FEEDER_SERVERS) {
		const char  *mapName;
		static char info[MAX_STRING_CHARS];

		uiInfo.serverStatus.currentServer = index;
		trap_LAN_GetServerInfo(ui_netSource.integer, uiInfo.serverStatus.displayServers[index], info, MAX_STRING_CHARS);

		mapName = Info_ValueForKey(info, "mapname");

		if (mapName && *mapName) {
			uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip(va("levelshots/%s", Info_ValueForKey(info, "mapname")));
		} else {
			uiInfo.serverStatus.currentServerPreview = trap_R_RegisterShaderNoMip("levelshots/unknownmap");
		}
	} else if (feederID == FEEDER_FINDPLAYER) {
		uiInfo.currentFoundPlayerServer = index;
		//
		if (index < uiInfo.numFoundPlayerServers - 1) {
			// build a new server status for this server
			Q_strncpyz(uiInfo.serverStatusAddress, uiInfo.foundPlayerServerAddresses[uiInfo.currentFoundPlayerServer], sizeof (uiInfo.serverStatusAddress));
			Menu_SetFeederSelection(NULL, FEEDER_SERVERSTATUS, 0, NULL);
			UI_BuildServerStatus(qtrue);
		}
	} else if (feederID == FEEDER_PLAYER_LIST) {
		uiInfo.playerIndex = index;
	} else if (feederID == FEEDER_TEAM_LIST) {
		uiInfo.teamIndex = index;
	} else if (feederID == FEEDER_MODS) {
		uiInfo.modIndex = index;
	} else if (feederID == FEEDER_DEMOS) {
		uiInfo.demoIndex = index;
	} else if (feederID == FEEDER_PROFILES) {
		uiInfo.profileIndex = index;
		trap_Cvar_Set("ui_profile", uiInfo.profileList[index].name);
	}
}

qboolean UI_FeederSelectionClick(itemDef_t *item) {
	listBoxDef_t *listPtr = (listBoxDef_t *)item->typeData;

	if (item->special == FEEDER_SERVERS && !Menus_CaptureFuncActive()) {
		// ugly hack for favourites handling
		rectDef_t rect;

		Item_ListBox_MouseEnter(item, DC->cursorx, DC->cursory, qtrue);

		rect.x = item->window.rect.x + listPtr->columnInfo[SORT_FAVOURITES].pos;
		rect.y = item->window.rect.y + (listPtr->cursorPos - listPtr->startPos) * listPtr->elementHeight;
		rect.w = listPtr->columnInfo[SORT_FAVOURITES].width;
		rect.h = listPtr->columnInfo[SORT_FAVOURITES].width;

		if (BG_CursorInRect(&rect)) {
			char buff[MAX_STRING_CHARS];
			char addr[MAX_NAME_LENGTH];

			if (trap_LAN_ServerIsInFavoriteList(ui_netSource.integer, uiInfo.serverStatus.displayServers[listPtr->cursorPos])) {
				trap_LAN_GetServerInfo(ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS);
				addr[0] = '\0';
				Q_strncpyz(addr, Info_ValueForKey(buff, "addr"), MAX_NAME_LENGTH);
				if (addr[0] != '\0') {
					trap_LAN_RemoveServer(AS_FAVORITES, addr);
					if (ui_netSource.integer == AS_FAVORITES) {
						UI_BuildServerDisplayList(1);
						UI_FeederSelection(FEEDER_SERVERS, 0);
					}
				}
			} else {
				char name[MAX_NAME_LENGTH];

				trap_LAN_GetServerInfo(ui_netSource.integer, uiInfo.serverStatus.displayServers[uiInfo.serverStatus.currentServer], buff, MAX_STRING_CHARS);
				addr[0] = '\0';
				name[0] = '\0';
				Q_strncpyz(addr, Info_ValueForKey(buff, "addr"), MAX_NAME_LENGTH);
				Q_strncpyz(name, Info_ValueForKey(buff, "hostname"), MAX_NAME_LENGTH);
				if (*name && *addr) {
					trap_LAN_AddServer(AS_FAVORITES, name, addr);
				}
			}

			return qtrue;
		}
	}

	return qfalse;
}

static void UI_Pause(qboolean b) {
	if (b) {
		// pause the game and set the ui keycatcher
		trap_Cvar_Set("cl_paused", "1");
		trap_Key_SetCatcher(KEYCATCH_UI);
	} else {
		// unpause the game and clear the ui keycatcher
		trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
		trap_Key_ClearStates();
		trap_Cvar_Set("cl_paused", "0");
	}
}

/*
=================
UI_Init
=================
*/
void _UI_Init(void) {
	int x;

	UI_RegisterCvars();
	UI_InitMemory();
	trap_PC_RemoveAllGlobalDefines();

	trap_Cvar_Set("ui_menuFiles", "ui/menus.txt");   // NERVE - SMF - we need to hardwire for wolfMP

	// cache redundant calulations
	trap_GetGlconfig(&uiInfo.uiDC.glconfig);

	UI_ParseGLConfig();

	// for 640x480 virtualized screen
	uiInfo.uiDC.yscale = uiInfo.uiDC.glconfig.vidHeight * (1.0 / 480.0);
	uiInfo.uiDC.xscale = uiInfo.uiDC.glconfig.vidWidth * (1.0 / 640.0);
	if (uiInfo.uiDC.glconfig.vidWidth * 480 > uiInfo.uiDC.glconfig.vidHeight * 640) {
		// wide screen
		uiInfo.uiDC.bias = 0.5 * (uiInfo.uiDC.glconfig.vidWidth - (uiInfo.uiDC.glconfig.vidHeight * (640.0 / 480.0)));
	} else {
		// no wide screen
		uiInfo.uiDC.bias = 0;
	}

	uiInfo.uiDC.registerShaderNoMip  = &trap_R_RegisterShaderNoMip;
	uiInfo.uiDC.setColor             = &UI_SetColor;
	uiInfo.uiDC.drawHandlePic        = &UI_DrawHandlePic;
	uiInfo.uiDC.drawStretchPic       = &trap_R_DrawStretchPic;
	uiInfo.uiDC.drawText             = &Text_Paint;
	uiInfo.uiDC.drawTextExt          = &Text_Paint_Ext;
	uiInfo.uiDC.textWidth            = &Text_Width;
	uiInfo.uiDC.textWidthExt         = &Text_Width_Ext;
	uiInfo.uiDC.multiLineTextWidth   = &Multiline_Text_Width;
	uiInfo.uiDC.textHeight           = &Text_Height;
	uiInfo.uiDC.textHeightExt        = &Text_Height_Ext;
	uiInfo.uiDC.multiLineTextHeight  = &Multiline_Text_Height;
	uiInfo.uiDC.textFont             = &Text_SetActiveFont;
	uiInfo.uiDC.registerModel        = &trap_R_RegisterModel;
	uiInfo.uiDC.modelBounds          = &trap_R_ModelBounds;
	uiInfo.uiDC.fillRect             = &UI_FillRect;
	uiInfo.uiDC.drawRect             = &_UI_DrawRect;
	uiInfo.uiDC.drawSides            = &_UI_DrawSides;
	uiInfo.uiDC.drawTopBottom        = &_UI_DrawTopBottom;
	uiInfo.uiDC.clearScene           = &trap_R_ClearScene;
	uiInfo.uiDC.drawSides            = &_UI_DrawSides;
	uiInfo.uiDC.addRefEntityToScene  = &trap_R_AddRefEntityToScene;
	uiInfo.uiDC.renderScene          = &trap_R_RenderScene;
	uiInfo.uiDC.registerFont         = &trap_R_RegisterFont;
	uiInfo.uiDC.ownerDrawItem        = &UI_OwnerDraw;
	uiInfo.uiDC.ownerDrawVisible     = &UI_OwnerDrawVisible;
	uiInfo.uiDC.runScript            = &UI_RunMenuScript;
	uiInfo.uiDC.setCVar              = trap_Cvar_Set;
	uiInfo.uiDC.getCVarString        = trap_Cvar_VariableStringBuffer;
	uiInfo.uiDC.getCVarValue         = trap_Cvar_VariableValue;
	uiInfo.uiDC.drawTextWithCursor   = &Text_PaintWithCursor;
	uiInfo.uiDC.setOverstrikeMode    = &trap_Key_SetOverstrikeMode;
	uiInfo.uiDC.getOverstrikeMode    = &trap_Key_GetOverstrikeMode;
	uiInfo.uiDC.startLocalSound      = &trap_S_StartLocalSound;
	uiInfo.uiDC.ownerDrawHandleKey   = &UI_OwnerDrawHandleKey;
	uiInfo.uiDC.feederCount          = &UI_FeederCount;
	uiInfo.uiDC.feederItemImage      = &UI_FeederItemImage;
	uiInfo.uiDC.feederItemText       = &UI_FeederItemText;
	uiInfo.uiDC.fileText             = &UI_FileText; //----(SA)	re-added
	uiInfo.uiDC.feederSelection      = &UI_FeederSelection;
	uiInfo.uiDC.feederSelectionClick = &UI_FeederSelectionClick;
	uiInfo.uiDC.setBinding           = &trap_Key_SetBinding;
	uiInfo.uiDC.getBindingBuf        = &trap_Key_GetBindingBuf;
	uiInfo.uiDC.getKeysForBinding    = &trap_Key_KeysForBinding;
	uiInfo.uiDC.keynumToStringBuf    = &trap_Key_KeynumToStringBuf;
	uiInfo.uiDC.keyIsDown            = &trap_Key_IsDown;
	uiInfo.uiDC.executeText          = &trap_Cmd_ExecuteText;
	uiInfo.uiDC.Error                = &Com_Error;
	uiInfo.uiDC.Print                = &Com_Printf;
	uiInfo.uiDC.Pause                = &UI_Pause;
	uiInfo.uiDC.ownerDrawWidth       = &UI_OwnerDrawWidth;
	uiInfo.uiDC.registerSound        = &trap_S_RegisterSound;
	uiInfo.uiDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	uiInfo.uiDC.stopBackgroundTrack  = &trap_S_StopBackgroundTrack;
	uiInfo.uiDC.translateString      = &trap_TranslateString;       // NERVE - SMF
	uiInfo.uiDC.checkAutoUpdate      = &trap_CheckAutoUpdate;       // DHM - Nerve
	uiInfo.uiDC.getAutoUpdate        = &trap_GetAutoUpdate;         // DHM - Nerve
	uiInfo.uiDC.add2dPolys           = &trap_R_Add2dPolys;
	uiInfo.uiDC.updateScreen         = &trap_UpdateScreen;
	uiInfo.uiDC.getHunkData          = &trap_GetHunkData;
	uiInfo.uiDC.getConfigString      = &trap_GetConfigString;

	Init_Display(&uiInfo.uiDC);

	String_Init();

	uiInfo.uiDC.whiteShader = trap_R_RegisterShaderNoMip("white");

	AssetCache();

	uiInfo.passwordFilter = trap_R_RegisterShaderNoMip("ui/assets/filter_pass.tga");
	uiInfo.antiLagFilter  = trap_R_RegisterShaderNoMip("ui/assets/filter_antilag.tga");

	uiInfo.campaignMap = trap_R_RegisterShaderNoMip("gfx/loading/camp_map.tga");

	trap_Milliseconds();

	uiInfo.teamCount = 0;

	UI_LoadMenus("ui/menus.txt", qfalse);

	Menus_CloseAll();

	trap_LAN_LoadCachedServers();

	// sets defaults for ui temp cvars
	// rain - bounds check array index, although I'm pretty sure this
	// stuff isn't used anymore...
	x = (int)trap_Cvar_VariableValue("color") - 1;
	if (x < 0 || x >= (int)(sizeof (gamecodetoui) / sizeof (gamecodetoui[0]))) {
		x = 0;
	}
	uiInfo.effectsColor     = gamecodetoui[x];
	uiInfo.currentCrosshair = (int)trap_Cvar_VariableValue("cg_drawCrosshair");
	trap_Cvar_Set("ui_mousePitch", (trap_Cvar_VariableValue("m_pitch") >= 0) ? "0" : "1");

	if (trap_Cvar_VariableValue("ui_TeamArenaFirstRun") == 0) {
		trap_Cvar_Set("s_volume", "0.8");
		trap_Cvar_Set("s_musicvolume", "0.5");
		trap_Cvar_Set("ui_TeamArenaFirstRun", "1");
	}

	trap_Cvar_Register(NULL, "debug_protocol", "", 0);

	// init Yes/No once for cl_language -> server browser
	Q_strncpyz(translated_yes, DC->translateString("Yes"), sizeof (translated_yes));
	Q_strncpyz(translated_no, DC->translateString("NO"), sizeof (translated_no));
}

/*
=================
UI_KeyEvent
=================
*/
void _UI_KeyEvent(int key, qboolean down) {
	if (Menu_Count() > 0) {
		menuDef_t       *menu          = Menu_GetFocused();
		static qboolean bypassKeyClear = qfalse;

		if (menu) {
			if (trap_Cvar_VariableValue("cl_bypassMouseInput")) {
				bypassKeyClear = qtrue;
			}
			// always have the menus do the proper handling
			Menu_HandleKey(menu, key, down);
		} else {
			trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);

			// NERVE - SMF - we don't want to clear key states if bypassing input
			if (!bypassKeyClear) {
				trap_Key_ClearStates();
			}

			if (cl_bypassMouseInput.integer && !trap_Key_GetCatcher()) {
				trap_Cvar_Set("cl_bypassMouseInput", 0);
			}

			bypassKeyClear = qfalse;

			trap_Cvar_Set("cl_paused", "0");
		}
	}
}

/*
=================
UI_MouseEvent
=================
*/
void _UI_MouseEvent(int dx, int dy) {
	// update mouse screen position
	uiInfo.uiDC.cursorx += dx;
	if (uiInfo.uiDC.cursorx < 0) {
		uiInfo.uiDC.cursorx = 0;
	} else if (uiInfo.uiDC.cursorx > SCREEN_WIDTH) {
		uiInfo.uiDC.cursorx = SCREEN_WIDTH;
	}

	uiInfo.uiDC.cursory += dy;
	if (uiInfo.uiDC.cursory < 0) {
		uiInfo.uiDC.cursory = 0;
	} else if (uiInfo.uiDC.cursory > SCREEN_HEIGHT) {
		uiInfo.uiDC.cursory = SCREEN_HEIGHT;
	}

	if (Menu_Count() > 0) {
		Display_MouseMove(NULL, uiInfo.uiDC.cursorx, uiInfo.uiDC.cursory);
	}
}

//----(SA)	added
static uiMenuCommand_t menutype = UIMENU_NONE;

uiMenuCommand_t _UI_GetActiveMenu(void) {
	return menutype;
}
//----(SA)	end

#define MISSING_FILES_MSG "The following packs are missing:"

void _UI_SetActiveMenu(uiMenuCommand_t menu) {
	// this should be the ONLY way the menu system is brought up
	// enusure minumum menu data is cached
	if (Menu_Count() > 0) {
		char buf[4096]; // com_errorMessage can go up to 4096

		menutype = menu;    //----(SA)	added

		switch (menu) {
		case UIMENU_NONE:
			trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_UI);
			trap_Key_ClearStates();
			trap_Cvar_Set("cl_paused", "0");
			Menus_CloseAll();

			return;
		case UIMENU_MAIN:
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_CloseAll();
			Menus_ActivateByName("backgroundmusic", qtrue);      // Arnout: not nice, but best way to do it - putting the music in it's own menudef
			                                                     // makes sure it doesn't get restarted every time you reach the main menu
			Menus_ActivateByName("main_opener", qtrue);

			trap_Cvar_VariableStringBuffer("com_errorMessage", buf, sizeof (buf));

			// JPW NERVE stricmp() is silly but works, take a look at error.menu to see why.  I think this is bustified in q3ta
			// NOTE TTimo - I'm not sure Q_stricmp is useful to anything anymore
			// show_bug.cgi?id=507
			// TTimo - improved and tweaked that area a whole bunch
			if ((*buf) && (Q_stricmp(buf, ";"))) {
				trap_Cvar_Set("ui_connecting", "0");
				if (!Q_stricmpn(buf, "Invalid password", 16)) {
					trap_Cvar_Set("com_errorMessage", trap_TranslateString(buf));            // NERVE - SMF
					Menus_ActivateByName("popupPassword", qtrue);
				} else if (strlen(buf) > 5 && !Q_stricmpn(buf, "ET://", 5)) {         // fretn
					Q_strncpyz(buf, buf + 5, sizeof (buf));
					Com_Printf("Server is full, redirect to: %s\n", buf);
					switch (ui_autoredirect.integer) {
					//auto-redirect
					case 1:
						trap_Cvar_Set("com_errorMessage", "");
						trap_Cmd_ExecuteText(EXEC_APPEND, va("connect %s\n", buf));
						break;
					//prompt (default)
					default:
						trap_Cvar_Set("com_errorMessage", buf);
						Menus_ActivateByName("popupServerRedirect", qtrue);
						break;
					}
				} else {

					trap_Cvar_Set("com_errorMessage", trap_TranslateString(buf));            // NERVE - SMF
					// hacky, wanted to have the printout of missing files
					// text printing limitations force us to keep it all in a single message
					// NOTE: this works thanks to flip flop in UI_Cvar_VariableString
					if (UI_Cvar_VariableString("com_errorDiagnoseIP")[0]) {
						char *missing_files;

						missing_files = UI_Cvar_VariableString("com_missingFiles");
						if (missing_files[0]) {
							trap_Cvar_Set("com_errorMessage",
							              va("%s\n\n%s\n%s",
							                 UI_Cvar_VariableString("com_errorMessage"),
							                 trap_TranslateString(MISSING_FILES_MSG),
							                 missing_files));
						}
					}
					Menus_ActivateByName("popupError", qtrue);
				}
			}

			trap_S_FadeAllSound(1.0f, 1000, qfalse);    // make sure sound fades up
			return;

		case UIMENU_TEAM:
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_ActivateByName("team", qtrue);
			return;

		case UIMENU_NEED_CD:
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_ActivateByName("needcd", qtrue);
			return;

		case UIMENU_BAD_CD_KEY:
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_ActivateByName("badcd", qtrue);
			return;

		case UIMENU_INGAME:
			trap_Key_SetCatcher(KEYCATCH_UI);
			UI_BuildPlayerList();
			Menu_SetFeederSelection(NULL, FEEDER_PLAYER_LIST, 0, NULL);
			Menus_CloseAll();
			Menus_ActivateByName("ingame_main", qtrue);
			return;

		// NERVE - SMF
		case UIMENU_WM_QUICKMESSAGE:
			uiInfo.uiDC.cursorx = 639;
			uiInfo.uiDC.cursory = 479;
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_CloseAll();
			Menus_OpenByName("wm_quickmessage");
			return;

		case UIMENU_WM_QUICKMESSAGEALT:
			uiInfo.uiDC.cursorx = 639;
			uiInfo.uiDC.cursory = 479;
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_CloseAll();
			Menus_OpenByName("wm_quickmessageAlt");
			return;

		case UIMENU_WM_FTQUICKMESSAGE:
			uiInfo.uiDC.cursorx = 639;
			uiInfo.uiDC.cursory = 479;
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_CloseAll();
			Menus_OpenByName("wm_ftquickmessage");
			return;

		case UIMENU_WM_FTQUICKMESSAGEALT:
			uiInfo.uiDC.cursorx = 639;
			uiInfo.uiDC.cursory = 479;
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_CloseAll();
			Menus_OpenByName("wm_ftquickmessageAlt");
			return;

		case UIMENU_WM_AUTOUPDATE:
			// TTimo - changing the auto-update strategy to a modal prompt
			Menus_OpenByName("wm_autoupdate_modal");
			return;
		// -NERVE - SMF

		// ydnar: say, team say, etc
		case UIMENU_INGAME_MESSAGEMODE:
			trap_Key_SetCatcher(KEYCATCH_UI);
			Menus_OpenByName("ingame_messagemode");
			return;

		default:
			return; // TTimo: a lot of not handled
		}
	}
}

qboolean _UI_IsFullscreen(void) {
	return Menus_AnyFullScreenVisible();
}

void UI_ReadableSize(char *buf, int bufsize, int value) {
	int len = 0;

	if (value > 1024 * 1024 * 1024) {   // gigs
		Com_sprintf(buf, bufsize, "%d", value / (1024 * 1024 * 1024));
		len = strlen(buf);
		Com_sprintf(buf + len, bufsize - len, ".%02d GB",
		            (value % (1024 * 1024 * 1024)) * 100 / (1024 * 1024 * 1024));
	} else if (value > 1024 * 1024) {     // megs
		Com_sprintf(buf, bufsize, "%d", value / (1024 * 1024));
		len = strlen(buf);
		Com_sprintf(buf + len, bufsize - len, ".%02d MB",
		            (value % (1024 * 1024)) * 100 / (1024 * 1024));
	} else if (value > 1024) {     // kilos
		Com_sprintf(buf, bufsize, "%d KB", value / 1024);
	} else {   // bytes
		Com_sprintf(buf, bufsize, "%d bytes", value);
	}
}

// Assumes time is in sec
void UI_PrintTime(char *buf, int bufsize, int time) {
	if (time > 3600) {   // in the hours range
		Com_sprintf(buf, bufsize, "%d hr %d min", time / 3600, (time % 3600) / 60);
	} else if (time > 60) {     // mins
		Com_sprintf(buf, bufsize, "%d min %d sec", time / 60, time % 60);
	} else {    // secs
		Com_sprintf(buf, bufsize, "%d sec", time);
	}
}

/*
========================
UI_DrawConnectScreen

This will also be overlaid on the cgame info screen during loading
to prevent it from blinking away too rapidly on local or lan games.
========================
*/
#define CP_LINEWIDTH 50

void UI_DrawConnectScreen(qboolean overlay) {
	if (!overlay) {
		UI_DrawLoadPanel(qfalse, qfalse);
	}
}

/*
================
cvars
================
*/

typedef struct {
	vmCvar_t *vmCvar;
	char *cvarName;
	char *defaultString;
	int cvarFlags;
	int modificationCount;          // for tracking changes
} cvarTable_t;

vmCvar_t ui_brassTime;
vmCvar_t ui_drawCrosshair;
vmCvar_t ui_drawCrosshairNames;
vmCvar_t ui_drawCrosshairPickups;
vmCvar_t ui_marks;
vmCvar_t ui_autoactivate;

vmCvar_t ui_selectedPlayer;
vmCvar_t ui_netSource;
vmCvar_t ui_menuFiles;
vmCvar_t ui_dedicated;
vmCvar_t ui_serverFilterType;
vmCvar_t ui_currentNetMap;
vmCvar_t ui_currentMap;
vmCvar_t ui_mapIndex;
vmCvar_t ui_browserShowEmptyOrFull;
vmCvar_t ui_browserShowPasswordProtected;
vmCvar_t ui_browserShowAntilag;     // TTimo
vmCvar_t ui_serverStatusTimeOut;
vmCvar_t ui_model;
vmCvar_t ui_limboObjective;
vmCvar_t ui_prevTeam;
vmCvar_t ui_prevClass;
vmCvar_t ui_prevWeapon;
vmCvar_t ui_isSpectator;
vmCvar_t ui_glCustom;    // JPW NERVE missing from q3ta
vmCvar_t cl_profile;
vmCvar_t cl_defaultProfile;
vmCvar_t ui_profile;
// OSP
// cgame mappings
vmCvar_t ui_blackout;       // For speclock
vmCvar_t cg_crosshairColor;
vmCvar_t cg_crosshairColorAlt;
vmCvar_t cg_crosshairAlpha;
vmCvar_t cg_crosshairAlphaAlt;
vmCvar_t cg_crosshairSize;
// OSP

vmCvar_t cl_bypassMouseInput;

//bani
vmCvar_t ui_autoredirect;

cvarTable_t cvarTable[] =
{

	{ &ui_glCustom,                     "ui_glCustom",                     "4",                          CVAR_ARCHIVE,                   0 }, // JPW NERVE missing from q3ta
	{ &ui_teamArenaFirstRun,            "ui_teamArenaFirstRun",            "0",                          CVAR_ARCHIVE,                   0 }, // so sound stuff latches, strange as that seems
	{ &ui_brassTime,                    "cg_brassTime",                    "2500",                       CVAR_ARCHIVE,                   0 }, // JPW NERVE
	{ &ui_drawCrosshair,                "cg_drawCrosshair",                "4",                          CVAR_ARCHIVE,                   0 },
	{ &ui_drawCrosshairNames,           "cg_drawCrosshairNames",           "1",                          CVAR_ARCHIVE,                   0 },
	{ &ui_drawCrosshairPickups,         "cg_drawCrosshairPickups",         "1",                          CVAR_ARCHIVE,                   0 }, //----(SA) added
	{ &ui_marks,                        "cg_marktime",                     "20000",                      CVAR_ARCHIVE,                   0 },
	{ &ui_autoactivate,                 "cg_autoactivate",                 "1",                          CVAR_ARCHIVE,                   0 },

	{ &ui_dedicated,                    "ui_dedicated",                    "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_selectedPlayer,               "cg_selectedPlayer",               "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_netSource,                    "ui_netSource",                    "1",                          CVAR_ARCHIVE,                   0 },
	{ &ui_menuFiles,                    "ui_menuFiles",                    "ui/menus.txt",               CVAR_ARCHIVE,                   0 },

	// NERVE - SMF - multiplayer cvars
	{ &ui_mapIndex,                     "ui_mapIndex",                     "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_currentMap,                   "ui_currentMap",                   "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_currentNetMap,                "ui_currentNetMap",                "0",                          CVAR_ARCHIVE,                   0 },

	{ &ui_browserShowEmptyOrFull,       "ui_browserShowEmptyOrFull",       "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserShowPasswordProtected, "ui_browserShowPasswordProtected", "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_browserShowAntilag,           "ui_browserShowAntilag",           "0",                          CVAR_ARCHIVE,                   0 },
	{ &ui_serverStatusTimeOut,          "ui_serverStatusTimeOut",          "7000",                       CVAR_ARCHIVE,                   0 },

	{ &ui_limboObjective,               "ui_limboObjective",               "0",                          0,                              0 },

	{ &ui_prevTeam,                     "ui_prevTeam",                     "-1",                         0,                              0 },
	{ &ui_prevClass,                    "ui_prevClass",                    "-1",                         0,                              0 },
	{ &ui_prevWeapon,                   "ui_prevWeapon",                   "-1",                         0,                              0 },

	{ &ui_isSpectator,                  "ui_isSpectator",                  "1",                          0,                              0 },
	// -NERVE - SMF

	{ NULL,                             "cg_drawBuddies",                  "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_autoReload",                   "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_noAmmoAutoSwitch",             "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_useWeapsForZoom",              "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_zoomDefaultSniper",            "20",                         CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_zoomstepsniper",               "2",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_voicespritetime",              "6000",                       CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_announcer",                    "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_printObjectiveInfo",           "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_useScreenshotJPEG",            "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_drawGun",                      "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_drawReinforcementTime",        "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_cursorHints",                  "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_crosshairPulse",               "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_drawCrosshairNames",           "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_crosshairColor",               "White",                      CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_crosshairAlpha",               "1.0",                        CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_crosshairColorAlt",            "White",                      CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_coronafardist",                "1536",                       CVAR_ARCHIVE,                   0 },
	{ NULL,                             "cg_wolfparticles",                "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "g_password",                      "none",                       CVAR_USERINFO,                  0 },
	{ NULL,                             "g_antilag",                       "1",                          CVAR_SERVERINFO | CVAR_ARCHIVE, 0 },

	{ &cl_profile,                      "cl_profile",                      "",                           CVAR_ROM,                       0 },
	{ &cl_defaultProfile,               "cl_defaultProfile",               "",                           CVAR_ROM,                       0 },
	{ &ui_profile,                      "ui_profile",                      "",                           CVAR_ROM,                       0 },
	{ &ui_blackout,                     "ui_blackout",                     "0",                          CVAR_ROM,                       0 },
	{ &cg_crosshairAlpha,               "cg_crosshairAlpha",               "1.0",                        CVAR_ARCHIVE,                   0 },
	{ &cg_crosshairAlphaAlt,            "cg_crosshairAlphaAlt",            "1.0",                        CVAR_ARCHIVE,                   0 },
	{ &cg_crosshairColor,               "cg_crosshairColor",               "White",                      CVAR_ARCHIVE,                   0 },
	{ &cg_crosshairColorAlt,            "cg_crosshairColorAlt",            "White",                      CVAR_ARCHIVE,                   0 },
	{ &cg_crosshairSize,                "cg_crosshairSize",                "48",                         CVAR_ARCHIVE,                   0 },

	{ NULL,                             "g_inactivity",                    "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "refereePassword",                 "none",                       CVAR_ARCHIVE,                   0 },
	{ NULL,                             "sv_maxRate",                      "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "g_spectatorInactivity",           "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "server_motd0",                    " ^NEnemy Territory ^7MOTD ", CVAR_ARCHIVE,                   0 },
	{ NULL,                             "server_motd1",                    "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "server_motd2",                    "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "server_motd3",                    "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "server_motd4",                    "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "server_motd5",                    "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "team_maxPanzers",                 "-1",                         CVAR_ARCHIVE,                   0 },
	{ NULL,                             "team_maxplayers",                 "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "team_nocontrols",                 "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "vote_allow_kick",                 "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "vote_allow_map",                  "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "vote_allow_randommap",            "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "vote_allow_referee",              "0",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "vote_allow_antilag",              "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "vote_allow_muting",               "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "vote_allow_kick",                 "1",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "vote_limit",                      "5",                          CVAR_ARCHIVE,                   0 },
	{ NULL,                             "vote_percent",                    "50",                         CVAR_ARCHIVE,                   0 },
	{ NULL,                             "ui_r_mode",                       "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "ui_r_gamma",                      "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "ui_rate",                         "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "ui_handedness",                   "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "ui_sensitivity",                  "",                           CVAR_ARCHIVE,                   0 },
	{ NULL,                             "ui_profile_mousePitch",           "",                           CVAR_ARCHIVE,                   0 },
	{ &cl_bypassMouseInput,             "cl_bypassMouseInput",             "0",                          CVAR_TEMP,                      0 },
	{ NULL,                             "ui_showtooltips",                 "1",                          CVAR_ARCHIVE,                   0 },

	//bani
	{ &ui_autoredirect,                 "ui_autoredirect",                 "0",                          CVAR_ARCHIVE,                   0 },
};

int cvarTableSize = sizeof (cvarTable) / sizeof (cvarTable[0]);

/*
=================
UI_RegisterCvars
=================
*/
void UI_RegisterCvars(void) {
	int         i;
	cvarTable_t *cv;

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; ++i, ++cv) {
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar != NULL) {
			cv->modificationCount = cv->vmCvar->modificationCount;
		}
	}

	// OSP - Always force this to 0 on init
	trap_Cvar_Set("ui_blackout", "0");
	BG_SetRGBACvar(cg_crosshairColor.string, uiInfo.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
	BG_SetRGBACvar(cg_crosshairColorAlt.string, uiInfo.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt");
}

/*
=================
UI_UpdateCvars
=================
*/
void UI_UpdateCvars(void) {
	int         i;
	cvarTable_t *cv;

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; ++i, ++cv) {
		if (cv->vmCvar) {
			trap_Cvar_Update(cv->vmCvar);
			if (cv->modificationCount != cv->vmCvar->modificationCount) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				// OSP
				if (cv->vmCvar == &cg_crosshairColor || cv->vmCvar == &cg_crosshairAlpha) {
					BG_SetRGBACvar(cg_crosshairColor.string, uiInfo.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
				}

				if (cv->vmCvar == &cg_crosshairColorAlt || cv->vmCvar == &cg_crosshairAlphaAlt) {
					BG_SetRGBACvar(cg_crosshairColorAlt.string, uiInfo.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt");
				}
			}
		}
	}
}

// NERVE - SMF
/*
=================
ArenaServers_StopRefresh
=================
*/
static void UI_StopServerRefresh(void) {
	int count;

	if (!uiInfo.serverStatus.refreshActive) {
		// not currently refreshing
		return;
	}
	uiInfo.serverStatus.refreshActive = qfalse;
	Com_Printf("%d servers listed in browser with %d players.\n",
	           uiInfo.serverStatus.numDisplayServers,
	           uiInfo.serverStatus.numPlayersOnServers);
	count = trap_LAN_GetServerCount(ui_netSource.integer);
	if (count - uiInfo.serverStatus.numDisplayServers > 0) {
		// TTimo - used to be about cl_maxping filtering, that was Q3 legacy, RTCW browser has much more filtering options
		Com_Printf("%d servers not listed (filtered out by game browser settings)\n",
		           count - uiInfo.serverStatus.numDisplayServers);
	}
}

/*
=================
UI_DoServerRefresh
=================
*/
static void UI_DoServerRefresh(void) {
	qboolean wait = qfalse;

	if (!uiInfo.serverStatus.refreshActive) {
		return;
	}
	if (ui_netSource.integer != AS_FAVORITES) {
		if (ui_netSource.integer == AS_LOCAL) {
			if (!trap_LAN_GetServerCount(ui_netSource.integer)) {
				wait = qtrue;
			}
		} else {
			if (trap_LAN_GetServerCount(ui_netSource.integer) < 0) {
				wait = qtrue;
			}
		}
	}

	if (uiInfo.uiDC.realTime < uiInfo.serverStatus.refreshtime && wait) {
		return;
	}

	// if still trying to retrieve pings
	if (trap_LAN_UpdateVisiblePings(ui_netSource.integer)) {
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
	} else if (!wait) {
		// get the last servers in the list
		UI_BuildServerDisplayList(2);
		// stop the refresh
		UI_StopServerRefresh();
	}
	UI_BuildServerDisplayList(0);
}

/*
=================
UI_StartServerRefresh
=================
*/
static void UI_StartServerRefresh(qboolean full) {
	char buff[64];

	qtime_t q;

	trap_RealTime(&q);
	Com_sprintf(buff, sizeof (buff), "%s-%i, %i at %s:%s", MonthAbbrev[q.tm_mon], q.tm_mday, 1900 + q.tm_year, q.tm_hour < 10 ? va("0%i", q.tm_hour) : va("%i", q.tm_hour),
	            q.tm_min < 10 ? va("0%i", q.tm_min) : va("%i", q.tm_min));
	trap_Cvar_Set(va("ui_lastServerRefresh_%i", ui_netSource.integer), buff);

	if (!full) {
		UI_UpdatePendingPings();
		return;
	}

	uiInfo.serverStatus.refreshActive      = qtrue;
	uiInfo.serverStatus.nextDisplayRefresh = uiInfo.uiDC.realTime + 1000;
	// clear number of displayed servers
	uiInfo.serverStatus.numDisplayServers   = 0;
	uiInfo.serverStatus.numPlayersOnServers = 0;
	// mark all servers as visible so we store ping updates for them
	trap_LAN_MarkServerVisible(ui_netSource.integer, -1, qtrue);
	// reset all the pings
	trap_LAN_ResetPings(ui_netSource.integer);
	//
	if (ui_netSource.integer == AS_LOCAL) {
		trap_Cmd_ExecuteText(EXEC_APPEND, "localservers\n");
		uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 1000;
		return;
	}

	uiInfo.serverStatus.refreshtime = uiInfo.uiDC.realTime + 5000;
	if (ui_netSource.integer == AS_GLOBAL) {
		char *ptr;

		ptr = UI_Cvar_VariableString("debug_protocol");
		if (*ptr) {
			trap_Cmd_ExecuteText(EXEC_APPEND, va("globalservers %d %s\n", 0, ptr));
		} else {
			trap_Cmd_ExecuteText(EXEC_APPEND, va("globalservers %d %d\n", 0, (int)trap_Cvar_VariableValue("protocol")));
		}
	}
}
