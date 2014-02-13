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

// cg_window.c: cgame window handling
// ----------------------------------
// 24 Jul 02
// rhea@OrangeSmoothie.org
//
#include "cg_local.h"
#include "../ui/ui_shared.h"

//////////////////////////////////////////////
//////////////////////////////////////////////
//
//      WINDOW HANDLING AND PRIMITIVES
//
//////////////////////////////////////////////
//////////////////////////////////////////////


// Windowing system setup
void CG_windowInit(void) {
	int i;

	cg.winHandler.numActiveWindows = 0;
	for (i = 0; i < MAX_WINDOW_COUNT; i++) {
		cg.winHandler.window[i].inuse = qfalse;
	}
}

// Free up a window reservation
void CG_windowFree(cg_window_t *w) {
	int                i, j;
	cg_windowHandler_t *wh = &cg.winHandler;

	if (w == NULL) {
		return;
	}

	if (w->effects >= WFX_FADEIN && w->state != WSTATE_OFF && w->inuse == qtrue) {
		w->state = WSTATE_SHUTDOWN;
		w->time  = trap_Milliseconds();
		return;
	}

	for (i = 0; i < wh->numActiveWindows; i++) {
		if (w == &wh->window[wh->activeWindows[i]]) {
			for (j = i; j < wh->numActiveWindows; j++) {
				if (j + 1 < wh->numActiveWindows) {
					// Nico, #fixme: GCC 4.8.2 with optimization says
					// warning: array subscript is above array bounds
					wh->activeWindows[j] = wh->activeWindows[j + 1];
				}
			}

			w->id    = WID_NONE;
			w->inuse = qfalse;
			w->state = WSTATE_OFF;

			CG_removeStrings(w);

			wh->numActiveWindows--;

			break;
		}
	}
}

void CG_windowCleanup(void) {
	int                i;
	cg_windowHandler_t *wh = &cg.winHandler;

	for (i = 0; i < wh->numActiveWindows; ++i) {
		cg_window_t *w = &wh->window[wh->activeWindows[i]];
		if (!w->inuse || w->state == WSTATE_OFF) {
			CG_windowFree(w);
			i--;
		}
	}
}

void CG_demoAviFPSDraw(void) {
	qboolean fKeyDown = cgs.fKeyPressed[K_F1] | cgs.fKeyPressed[K_F2] | cgs.fKeyPressed[K_F3] | cgs.fKeyPressed[K_F4] | cgs.fKeyPressed[K_F5];

	if (cg.demoPlayback && fKeyDown && cgs.aviDemoRate >= 0) {
		CG_DrawStringExt(42, 425,
		                 ((cgs.aviDemoRate > 0) ? va("^3Record AVI @ ^7%d^2fps", cgs.aviDemoRate) : "^1Stop AVI Recording"),
		                 colorWhite, qfalse, qfalse, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT - 2, 0);
	}
}

void CG_demoTimescaleDraw(void) {
	if (cg.demoPlayback && cgs.timescaleUpdate > cg.time && demo_drawTimeScale.integer != 0) {
		char *s = va("^3TimeScale: ^7%.1f", cg_timescale.value);
		int  w  = CG_DrawStrlen(s) * SMALLCHAR_WIDTH;

		CG_FillRect(42 - 2, 400, w + 5, SMALLCHAR_HEIGHT + 3, colorDkGreen);
		CG_DrawRect(42 - 2, 400, w + 5, SMALLCHAR_HEIGHT + 3, 1, colorMdYellow);

		CG_DrawStringExt(42, 400, s, colorWhite, qfalse, qtrue, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, 0);
	}
}


// Main window-drawing handler
void CG_windowDraw(void) {
	int         h, x, y, i, j, milli, t_offset, tmp;
	cg_window_t *w;
	qboolean    fCleanup = qfalse;
	// Gordon: FIXME, the limbomenu var no longer exists
	vec4_t *bg;
	vec4_t textColor, borderColor, bgColor;

	if (cg.winHandler.numActiveWindows == 0) {
		// Draw these for demoplayback no matter what
		CG_demoAviFPSDraw();
		CG_demoTimescaleDraw();
		return;
	}

	milli = trap_Milliseconds();
	memcpy(textColor, colorWhite, sizeof (vec4_t));

	// Mouse cursor position for MV highlighting (offset for cursor pointer position)
	// Also allow for swingcam toggling
	for (i = 0; i < cg.winHandler.numActiveWindows; i++) {
		w = &cg.winHandler.window[cg.winHandler.activeWindows[i]];

		if (!w->inuse || w->state == WSTATE_OFF) {
			fCleanup = qtrue;
			continue;
		}

		if (w->effects & WFX_TEXTSIZING) {
			CG_windowNormalizeOnText(w);
			w->effects &= ~WFX_TEXTSIZING;
		}

		bg = ((w->effects & WFX_FLASH) && (milli % w->flashPeriod) > w->flashMidpoint) ? &w->colorBackground2 : &w->colorBackground;

		h            = w->h;
		x            = w->x;
		y            = w->y;
		t_offset     = milli - w->time;
		textColor[3] = 1.0f;
		memcpy(&borderColor, w->colorBorder, sizeof (vec4_t));
		memcpy(&bgColor, bg, sizeof (vec4_t));

		// TODO: Add in support for ALL scrolling effects
		if (w->state == WSTATE_START) {
			tmp = w->targetTime - t_offset;
			if (w->effects & WFX_SCROLLUP) {
				if (tmp > 0) {
					y += (480 - y) * tmp / w->targetTime;   //(100 * tmp / w->targetTime) / 100;
				} else {
					w->state = WSTATE_COMPLETE;
				}

				w->curY = y;
			}
			if (w->effects & WFX_FADEIN) {
				if (tmp > 0) {
					textColor[3] = (float)((float)t_offset / (float)w->targetTime);
				} else {
					w->state = WSTATE_COMPLETE;
				}
			}
		} else if (w->state == WSTATE_SHUTDOWN) {
			tmp = w->targetTime - t_offset;
			if (w->effects & WFX_SCROLLUP) {
				if (tmp > 0) {
					y = w->curY + (480 - w->y) * t_offset / w->targetTime;        //(100 * t_offset / w->targetTime) / 100;
				}
				if (tmp < 0 || y >= 480) {
					w->state = WSTATE_OFF;
					fCleanup = qtrue;
					continue;
				}
			}
			if (w->effects & WFX_FADEIN) {
				if (tmp > 0) {
					textColor[3] -= (float)((float)t_offset / (float)w->targetTime);
				} else {
					textColor[3] = 0.0f;
					w->state     = WSTATE_OFF;
				}
			}
		}

		borderColor[3] *= textColor[3];
		bgColor[3]     *= textColor[3];

		CG_FillRect(x, y, w->w, h, bgColor);
		CG_DrawRect(x, y, w->w, h, 1, borderColor);

		x += 5;
		y -= (w->effects & WFX_TRUETYPE) ? 3 : 0;

		for (j = w->lineCount - 1; j >= 0; j--) {
			if (w->effects & WFX_TRUETYPE) {
				CG_Text_Paint_Ext(x, y + h, w->fontScaleX, w->fontScaleY, textColor,
				                  (char *)w->lineText[j], 0.0f, 0, 0, &cgs.media.limboFont2);
			}

			h -= (w->lineHeight[j] + 3);

			if (!(w->effects & WFX_TRUETYPE)) {
				CG_DrawStringExt2(x, y + h, (char *)w->lineText[j], textColor,
				                  qfalse, qtrue, w->fontWidth, w->fontHeight, 0);
			}
		}
	}

	// Extra rate info
	CG_demoAviFPSDraw();
	CG_demoTimescaleDraw();

	if (fCleanup) {
		CG_windowCleanup();
	}
}


// Set the window width and height based on the windows text/font parameters
void CG_windowNormalizeOnText(cg_window_t *w) {
	int i, tmp;

	if (w == NULL) {
		return;
	}

	w->w = 0;
	w->h = 0;

	if (!(w->effects & WFX_TRUETYPE)) {
		w->fontWidth  = w->fontScaleX * WINDOW_FONTWIDTH;
		w->fontHeight = w->fontScaleY * WINDOW_FONTHEIGHT;
	}

	for (i = 0; i < w->lineCount; i++) {
		if (w->effects & WFX_TRUETYPE) {
			tmp = CG_Text_Width_Ext((char *)w->lineText[i], w->fontScaleX, 0, &cgs.media.limboFont2);
		} else {
			tmp = CG_DrawStrlen((char *)w->lineText[i]) * w->fontWidth;
		}

		if (tmp > w->w) {
			w->w = tmp;
		}
	}

	for (i = 0; i < w->lineCount; i++) {
		if (w->effects & WFX_TRUETYPE) {
			w->lineHeight[i] = CG_Text_Height_Ext((char *)w->lineText[i], w->fontScaleY, 0, &cgs.media.limboFont2);
		} else {
			w->lineHeight[i] = w->fontHeight;
		}

		w->h += w->lineHeight[i] + 3;
	}

	// Border + margins
	w->w += 10;
	w->h += 3;

	// Set up bottom alignment
	if (w->x < 0) {
		w->x += 640 - w->w;
	}
	if (w->y < 0) {
		w->y += 480 - w->h;
	}
}

//
// String buffer handling
//
void CG_initStrings(void) {
	int i;

	for (i = 0; i < MAX_STRINGS; i++) {
		cg.aStringPool[i].fActive = qfalse;
		cg.aStringPool[i].str[0]  = 0;
	}
}

void CG_removeStrings(cg_window_t *w) {
	int i, j;

	for (i = 0; i < w->lineCount; i++) {
		char *ref = w->lineText[i];

		for (j = 0; j < MAX_STRINGS; j++) {
			if (!cg.aStringPool[j].fActive) {
				continue;
			}

			if (ref == (char *)&cg.aStringPool[j].str) {
				w->lineText[i]            = NULL;
				cg.aStringPool[j].fActive = qfalse;
				cg.aStringPool[j].str[0]  = 0;

				break;
			}
		}
	}
}
