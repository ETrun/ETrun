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

// cg_info.c -- display information while data is being loading

#include "cg_local.h"

/*
======================
CG_LoadingString

======================
*/
void CG_LoadingString(const char *s) {
	Q_strncpyz(cg.infoScreenText, s, sizeof (cg.infoScreenText));

	if (s && *s) {
		CG_Printf(va("LOADING... %s\n", s));      //----(SA)	added so you can see from the console what's going on

	}
}

/*
====================
CG_DrawInformation

Draw all the status / pacifier stuff during level loading
====================
*/
void CG_DrawInformation(qboolean forcerefresh) {
	static int lastcalled = 0;

	if (lastcalled && (trap_Milliseconds() - lastcalled < 500)) {
		return;
	}
	lastcalled = trap_Milliseconds();

	if (cg.snap) {
		return;     // we are in the world, no need to draw information
	}

	CG_DrawConnectScreen(qfalse, forcerefresh);
}


void CG_ShowHelp_On(int *status) {
	int milli = trap_Milliseconds();

	if (*status == SHOW_SHUTDOWN && milli < cg.fadeTime) {
		cg.fadeTime = 2 * milli + STATS_FADE_TIME - cg.fadeTime;
	} else if (*status != SHOW_ON) {
		cg.fadeTime = milli + STATS_FADE_TIME;
	}

	*status = SHOW_ON;
}

void CG_ShowHelp_Off(int *status) {
	if (*status != SHOW_OFF) {
		int milli = trap_Milliseconds();

		if (milli < cg.fadeTime) {
			cg.fadeTime = 2 * milli + STATS_FADE_TIME - cg.fadeTime;
		} else {
			cg.fadeTime = milli + STATS_FADE_TIME;
		}

		*status = SHOW_SHUTDOWN;
	}
}


// Demo playback key catcher support
void CG_DemoClick(int key, qboolean down) {
	int milli = trap_Milliseconds();

	// Avoid active console keypress issues
	if (!down && !cgs.fKeyPressed[key]) {
		return;
	}

	cgs.fKeyPressed[key] = down;

	switch (key) {
	case K_ESCAPE:
		CG_ShowHelp_Off(&cg.demohelpWindow);
		CG_keyOff_f();
		return;

	case K_TAB:
		if (down) {
			CG_ScoresDown_f();
		} else {
			CG_ScoresUp_f();
		}
		return;

	// Help info
	case K_BACKSPACE:
		if (!down) {
			if (cg.demohelpWindow != SHOW_ON) {
				CG_ShowHelp_On(&cg.demohelpWindow);
			} else {
				CG_ShowHelp_Off(&cg.demohelpWindow);
			}
		}
		return;

	// Screenshot keys
	case K_F11:
		if (!down) {
			trap_SendConsoleCommand(va("screenshot%s\n", ((cg_useScreenshotJPEG.integer) ? "JPEG" : "")));
		}
		return;
	case K_F12:
		if (!down) {
			CG_autoScreenShot_f();
		}
		return;

	// Window controls
	case K_SHIFT:
	case K_CTRL:
	case K_MOUSE4:
		cgs.fResize = down;
		return;
	case K_MOUSE1:
		cgs.fSelect = down;
		return;
	case K_MOUSE2:
	case K_INS:
	case K_KP_PGUP:
	case K_DEL:
	case K_KP_PGDN:
	case K_MOUSE3:
		return;

	// Third-person controls
	case K_ENTER:
		if (!down) {
			trap_Cvar_Set("cg_thirdperson", ((cg_thirdPerson.integer == 0) ? "1" : "0"));
		}
		return;
	case K_UPARROW:
		if (milli > cgs.thirdpersonUpdate) {
			float range = cg_thirdPersonRange.value;

			cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
			range                -= ((range >= 4 * DEMO_RANGEDELTA) ? DEMO_RANGEDELTA : (range - DEMO_RANGEDELTA));
			trap_Cvar_Set("cg_thirdPersonRange", va("%f", range));
		}
		return;
	case K_DOWNARROW:
		if (milli > cgs.thirdpersonUpdate) {
			float range = cg_thirdPersonRange.value;

			cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
			range                += ((range >= 120 * DEMO_RANGEDELTA) ? 0 : DEMO_RANGEDELTA);
			trap_Cvar_Set("cg_thirdPersonRange", va("%f", range));
		}
		return;
	case K_RIGHTARROW:
		if (milli > cgs.thirdpersonUpdate) {
			float angle = cg_thirdPersonAngle.value - DEMO_ANGLEDELTA;

			cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
			if (angle < 0) {
				angle += 360.0f;
			}
			trap_Cvar_Set("cg_thirdPersonAngle", va("%f", angle));
		}
		return;
	case K_LEFTARROW:
		if (milli > cgs.thirdpersonUpdate) {
			float angle = cg_thirdPersonAngle.value + DEMO_ANGLEDELTA;

			cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
			if (angle >= 360.0f) {
				angle -= 360.0f;
			}
			trap_Cvar_Set("cg_thirdPersonAngle", va("%f", angle));
		}
		return;

	// Timescale controls
	case K_KP_5:
	case K_KP_INS:
	case K_SPACE:
		if (!down) {
			trap_Cvar_Set("timescale", "1");
			cgs.timescaleUpdate = cg.time + 1000;
		}
		return;
	case K_KP_DOWNARROW:
		if (!down) {
			float tscale = cg_timescale.value;

			if (tscale <= 1.1f) {
				if (tscale > 0.1f) {
					tscale -= 0.1f;
				}
			} else {
				tscale -= 1.0;
			}
			trap_Cvar_Set("timescale", va("%f", tscale));
			cgs.timescaleUpdate = cg.time + (int)(1000.0f * tscale);
		}
		return;
	case K_MWHEELDOWN:
		if (!cgs.fKeyPressed[K_SHIFT]) {
			if (!down) {
				CG_ZoomOut_f();
			}
			return;
		}       // Roll over into timescale changes
	case K_KP_LEFTARROW:
		if (!down && cg_timescale.value > 0.1f) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value - 0.1f));
			cgs.timescaleUpdate = cg.time + (int)(1000.0f * cg_timescale.value - 0.1f);
		}
		return;
	case K_KP_UPARROW:
		if (!down) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value + 1.0f));
			cgs.timescaleUpdate = cg.time + (int)(1000.0f * cg_timescale.value + 1.0f);
		}
		return;
	case K_MWHEELUP:
		if (!cgs.fKeyPressed[K_SHIFT]) {
			if (!down) {
				CG_ZoomIn_f();
			}
			return;
		}       // Roll over into timescale changes
	case K_KP_RIGHTARROW:
		if (!down) {
			trap_Cvar_Set("timescale", va("%f", cg_timescale.value + 0.1f));
			cgs.timescaleUpdate = cg.time + (int)(1000.0f * cg_timescale.value + 0.1f);
		}
		return;

	// AVI recording controls
	case K_F1:
		if (down) {
			cgs.aviDemoRate = demo_avifpsF1.integer;
		} else {
			trap_Cvar_Set("cl_avidemo", demo_avifpsF1.string);
		}
		return;
	case K_F2:
		if (down) {
			cgs.aviDemoRate = demo_avifpsF2.integer;
		} else {
			trap_Cvar_Set("cl_avidemo", demo_avifpsF2.string);
		}
		return;
	case K_F3:
		if (down) {
			cgs.aviDemoRate = demo_avifpsF3.integer;
		} else {
			trap_Cvar_Set("cl_avidemo", demo_avifpsF3.string);
		}
		return;
	case K_F4:
		if (down) {
			cgs.aviDemoRate = demo_avifpsF4.integer;
		} else {
			trap_Cvar_Set("cl_avidemo", demo_avifpsF4.string);
		}
		return;
	case K_F5:
		if (down) {
			cgs.aviDemoRate = demo_avifpsF5.integer;
		} else {
			trap_Cvar_Set("cl_avidemo", demo_avifpsF5.string);
		}
		return;
	}
}

//
// Color/font info used for all overlays (below)
//
#define COLOR_BG            { 0.0f, 0.0f, 0.0f, 0.6f }
#define COLOR_BORDER        { 0.5f, 0.5f, 0.5f, 0.5f }
#define COLOR_BG_TITLE      { 0.16, 0.2f, 0.17f, 0.8f }
#define COLOR_BG_VIEW       { 0.16, 0.2f, 0.17f, 0.8f }
#define COLOR_BORDER_TITLE  { 0.1f, 0.1f, 0.1f, 0.2f }
#define COLOR_BORDER_VIEW   { 0.2f, 0.2f, 0.2f, 0.4f }
#define COLOR_HDR           { 0.6f, 0.6f, 0.6f, 1.0f }
#define COLOR_HDR2          { 0.6f, 0.6f, 0.4f, 1.0f }
#define COLOR_TEXT          { 0.625f, 0.625f, 0.6f, 1.0f }

#define FONT_HEADER         &cgs.media.limboFont1
#define FONT_TEXT           &cgs.media.limboFont2

#define DH_X    -20     // spacing from right
#define DH_Y    -60     // spacing from bottom
#define DH_W    148

void CG_DemoHelpDraw() {
	if (cg.demohelpWindow == SHOW_OFF) {
		return;

	} else {
		const char *help[] =
		{
			"^nTAB       ^mscores",
			"^nF1-F5     ^mavidemo record",
			"^nF11-F12   ^mscreenshot",
			NULL,
			"^nKP_DOWN   ^mslow down (--)",
			"^nKP_LEFT   ^mslow down (-)",
			"^nKP_UP     ^mspeed up (++)",
			"^nKP_RIGHT  ^mspeed up (+)",
			"^nSPACE     ^mnormal speed",
			NULL,
			"^nENTER     ^mExternal view",
			"^nLFT/RGHT  ^mChange angle",
			"^nUP/DOWN   ^mMove in/out"
		};


		int i, x, y = 480, w, h;

		vec4_t bgColor     = COLOR_BG;              // window
		vec4_t borderColor = COLOR_BORDER;          // window

		vec4_t bgColorTitle     = COLOR_BG_TITLE;       // titlebar
		vec4_t borderColorTitle = COLOR_BORDER_TITLE;   // titlebar

		// Main header
		int        hStyle    = ITEM_TEXTSTYLE_SHADOWED;
		float      hScale    = 0.16f;
		float      hScaleY   = 0.21f;
		fontInfo_t *hFont    = FONT_HEADER;
		vec4_t     hdrColor2 = COLOR_HDR2;  // text

		// Text settings
		int        tStyle   = ITEM_TEXTSTYLE_SHADOWED;
		int        tSpacing = 9;        // Should derive from CG_Text_Height_Ext
		float      tScale   = 0.19f;
		fontInfo_t *tFont   = FONT_TEXT;
		vec4_t     tColor   = COLOR_TEXT;   // text

		float diff = cg.fadeTime - trap_Milliseconds();


		// FIXME: Should compute this beforehand
		w = DH_W;
		x = 640 + DH_X - w;
		h = 2 + tSpacing + 2 +                                  // Header
		    2 + 1 +
		    tSpacing * (2 + (sizeof (help)) / sizeof (char *)) +
		    2;

		// Fade-in effects
		if (diff > 0.0f) {
			float scale = (diff / STATS_FADE_TIME);

			if (cg.demohelpWindow == SHOW_ON) {
				scale = 1.0f - scale;
			}

			bgColor[3]          *= scale;
			bgColorTitle[3]     *= scale;
			borderColor[3]      *= scale;
			borderColorTitle[3] *= scale;
			hdrColor2[3]        *= scale;
			tColor[3]           *= scale;

			y += (DH_Y - h) * scale;

		} else if (cg.demohelpWindow == SHOW_SHUTDOWN) {
			cg.demohelpWindow = SHOW_OFF;
			return;
		} else {
			y += DH_Y - h;
		}

		CG_DrawRect(x, y, w, h, 1, borderColor);
		CG_FillRect(x, y, w, h, bgColor);

		// Header
		CG_FillRect(x, y, w, tSpacing + 4, bgColorTitle);
		CG_DrawRect(x, y, w, tSpacing + 4, 1, borderColorTitle);

		x += 4;
		y += 1;
		y += tSpacing;
		CG_Text_Paint_Ext(x, y, hScale, hScaleY, hdrColor2, "DEMO CONTROLS", 0.0f, 0, hStyle, hFont);
		y += 3;

		// Control info
		for (i = 0; i < (int)(sizeof (help) / sizeof (char *)); i++) {
			y += tSpacing;
			if (help[i] != NULL) {
				CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, (char *)help[i], 0.0f, 0, tStyle, tFont);
			}
		}

		y += tSpacing * 2;
		CG_Text_Paint_Ext(x, y, tScale, tScale, tColor, "^nBACKSPACE ^mhelp on/off", 0.0f, 0, tStyle, tFont);
	}
}

void CG_DrawOverlays(void) {
	if (cg.demoPlayback) {
		CG_DemoHelpDraw();
	}
}
