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

/**********************************************************************
    UI_ATOMS.C

    User interface building blocks and support functions.
**********************************************************************/
#include "ui_local.h"

char *UI_Argv(int arg) {
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof (buffer));

	return buffer;
}

char *UI_Cvar_VariableString(const char *var_name) {
	static char buffer[2][MAX_STRING_CHARS];
	static int  toggle;

	toggle ^= 1;        // flip-flop to allow two returns without clash

	trap_Cvar_VariableStringBuffer(var_name, buffer[toggle], sizeof (buffer[0]));

	return buffer[toggle];
}

static void UI_Cache_f(void) {
	Display_CacheAll();
}

/*
=================
UI_ConsoleCommand
=================
*/
qboolean UI_ConsoleCommand(int realTime) {
	char *cmd;

	uiInfo.uiDC.frameTime = realTime - uiInfo.uiDC.realTime;
	uiInfo.uiDC.realTime  = realTime;

	cmd = UI_Argv(0);

	if (Q_stricmp(cmd, "ui_report") == 0) {
		String_Report();
		return qtrue;
	}

	if (Q_stricmp(cmd, "ui_load") == 0) {
		UI_Load();
		return qtrue;
	}

	if (Q_stricmp(cmd, "ui_cache") == 0) {
		UI_Cache_f();
		return qtrue;
	}

	if (Q_stricmp(cmd, "ui_teamOrders") == 0) {
		return qtrue;
	}

	if (Q_stricmp(cmd, "ui_cdkey") == 0) {
		return qtrue;
	}

	return qfalse;
}

/*
================
UI_AdjustFrom640

Adjusted for resolution and screen aspect ratio
================
*/
void UI_AdjustFrom640(float *x, float *y, float *w, float *h) {
	// expect valid pointers
	*x *= uiInfo.uiDC.xscale;
	*y *= uiInfo.uiDC.yscale;
	*w *= uiInfo.uiDC.xscale;
	*h *= uiInfo.uiDC.yscale;

	// suburb, widescreen support
	if (!UI_Is43Screen()) {
		*x *= RATIO43 / uiInfo.uiDC.glconfig.windowAspect;
		*w *= RATIO43 / uiInfo.uiDC.glconfig.windowAspect;
	}
}

void UI_DrawHandlePic(float x, float y, float w, float h, qhandle_t hShader) {
	float s0;
	float s1;
	float t0;
	float t1;

	if (w < 0) {     // flip about vertical
		w  = -w;
		s0 = 1;
		s1 = 0;
	} else {
		s0 = 0;
		s1 = 1;
	}

	if (h < 0) {     // flip about horizontal
		h  = -h;
		t0 = 1;
		t1 = 0;
	} else {
		t0 = 0;
		t1 = 1;
	}

	UI_AdjustFrom640(&x, &y, &w, &h);
	trap_R_DrawStretchPic(x, y, w, h, s0, t0, s1, t1, hShader);
}

/*
================
UI_FillRect

Coordinates are 640*480 virtual values
=================
*/
void UI_FillRect(float x, float y, float width, float height, const float *color) {
	trap_R_SetColor(color);

	UI_AdjustFrom640(&x, &y, &width, &height);
	trap_R_DrawStretchPic(x, y, width, height, 0, 0, 0, 0, uiInfo.uiDC.whiteShader);

	trap_R_SetColor(NULL);
}

void UI_SetColor(const float *rgba) {
	trap_R_SetColor(rgba);
}
