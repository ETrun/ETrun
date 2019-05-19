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

/*
==============
weapIconDrawSize
==============
*/
static int weapIconDrawSize(int weap) {
	switch (weap) {
	// weapons with 'wide' icons
	case WP_THOMPSON:
	case WP_MP40:
	case WP_STEN:
	case WP_PANZERFAUST:
	case WP_FLAMETHROWER:
	case WP_GARAND:
	case WP_FG42:
	case WP_FG42SCOPE:
	case WP_KAR98:
	case WP_GPG40:
	case WP_CARBINE:
	case WP_M7:
	case WP_MOBILE_MG42:
	case WP_MOBILE_MG42_SET:
	case WP_K43:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_MORTAR:
	case WP_MORTAR_SET:
		return 2;
	}

	return 1;
}

/*
==============
CG_DrawPlayerWeaponIcon
==============
*/
void CG_DrawPlayerWeaponIcon(int align, vec4_t *refcolor) {
	int       size, realweap;
	int       rx = CG_WideX(SCREEN_WIDTH + cg_playerWeaponIconXoffset.value) - 82;
	int       ry = SCREEN_HEIGHT - 56 + cg_playerWeaponIconYoffset.value;
	int       rw = 60;
	int       rh = 32;
	float     scale, halfScale;
	qhandle_t icon;
	vec4_t    hcolor;

	if (!cg_drawPlayerWeaponIcon.integer) {
		return;
	}

	VectorCopy(*refcolor, hcolor);
	hcolor[3] = 1.f;

	if (cg.predictedPlayerEntity.currentState.eFlags & EF_MG42_ACTIVE ||
	    cg.predictedPlayerEntity.currentState.eFlags & EF_MOUNTEDTANK) {
		realweap = WP_MOBILE_MG42;
	} else {
		realweap = cg.predictedPlayerState.weapon;
	}

	size = weapIconDrawSize(realweap);

	if (!size) {
		return;
	}

	if (cg.predictedPlayerEntity.currentState.eFlags & EF_MOUNTEDTANK && cg_entities[cg_entities[cg_entities[cg.snap->ps.clientNum].tagParent].tankparent].currentState.density & 8) {
		icon = cgs.media.browningIcon;
	} else {
		icon = cg_weapons[realweap].weaponIcon[1];
	}

	// pulsing grenade icon to help the player 'count' in their head
	if (cg.predictedPlayerState.grenadeTimeLeft) {     // grenades and dynamite set this
		// these time differently
		if (realweap == WP_DYNAMITE) {
			if (((cg.grenLastTime) % 1000) > ((cg.predictedPlayerState.grenadeTimeLeft) % 1000)) {
				trap_S_StartLocalSound(cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND);
			}
		} else {
			if (((cg.grenLastTime) % 1000) < ((cg.predictedPlayerState.grenadeTimeLeft) % 1000)) {
				switch (cg.predictedPlayerState.grenadeTimeLeft / 1000) {
				case 3:
					trap_S_StartLocalSound(cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND);
					break;
				case 2:
					trap_S_StartLocalSound(cgs.media.grenadePulseSound3, CHAN_LOCAL_SOUND);
					break;
				case 1:
					trap_S_StartLocalSound(cgs.media.grenadePulseSound2, CHAN_LOCAL_SOUND);
					break;
				case 0:
					trap_S_StartLocalSound(cgs.media.grenadePulseSound1, CHAN_LOCAL_SOUND);
					break;
				}
			}
		}

		scale     = (float)((cg.predictedPlayerState.grenadeTimeLeft) % 1000) / 100.0f;
		halfScale = scale * 0.5f;

		cg.grenLastTime = cg.predictedPlayerState.grenadeTimeLeft;
	} else {
		scale = halfScale = 0;
	}

	if (icon) {
		float x, y, w, h;

		if (size == 1) {   // draw half width to match the icon asset
			// start at left
			x = rx - halfScale;
			y = ry - halfScale;
			w = rw / 2 + scale;
			h = rh + scale;

			switch (align) {
			case ITEM_ALIGN_CENTER:
				x += rw / 4;
				break;
			case ITEM_ALIGN_RIGHT:
				x += rw / 2;
				break;
			case ITEM_ALIGN_LEFT:
			default:
				break;
			}

		} else {
			x = rx - halfScale;
			y = ry - halfScale;
			w = rw + scale;
			h = rh + scale;
		}

		trap_R_SetColor(hcolor);   // JPW NERVE
		CG_DrawPic(x, y, w, h, icon);
	}
}

/*
==============
CG_DrawCursorHints

  cg_cursorHints.integer ==
    0:	no hints
    1:	sin size pulse
    2:	one way size pulse
    3:	alpha pulse
    4+:	static image

==============
*/
void CG_DrawCursorhint(void) {
	int       h = 48;
	int       w = 48;
	int       x = 0.5f * SCREEN_WIDTH - 0.5f * w;
	int       y = 260;
	float     middle = x + cgs.wideXoffset;
	float     *color;
	float     scale, halfscale;
	qboolean  yellowbar = qfalse;
	qhandle_t icon = 0;

	if (!cg_cursorHints.integer || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	CG_CheckForCursorHints();

	switch (cg.cursorHintIcon) {

	case HINT_NONE:
	case HINT_FORCENONE:
		icon = 0;
		break;
	case HINT_DOOR:
		icon = cgs.media.doorHintShader;
		break;
	case HINT_DOOR_ROTATING:
		icon = cgs.media.doorRotateHintShader;
		break;
	case HINT_DOOR_LOCKED:
		icon = cgs.media.doorLockHintShader;
		break;
	case HINT_DOOR_ROTATING_LOCKED:
		icon = cgs.media.doorRotateLockHintShader;
		break;
	case HINT_MG42:
		icon = cgs.media.mg42HintShader;
		break;
	case HINT_BREAKABLE:
		icon = cgs.media.breakableHintShader;
		break;
	case HINT_BREAKABLE_DYNAMITE:
		icon = cgs.media.dynamiteHintShader;
		break;
	case HINT_TANK:
		icon = cgs.media.tankHintShader;
		break;
	case HINT_SATCHELCHARGE:
		icon = cgs.media.satchelchargeHintShader;
		break;
	case HINT_CONSTRUCTIBLE:
		icon = cgs.media.buildHintShader;
		break;
	case HINT_UNIFORM:
		icon = cgs.media.uniformHintShader;
		break;

	case HINT_CHAIR:
		icon = cgs.media.notUsableHintShader;
		break;
	case HINT_ALARM:
		icon = cgs.media.alarmHintShader;
		break;
	case HINT_HEALTH:
		icon = cgs.media.healthHintShader;
		break;
	case HINT_TREASURE:
		icon = cgs.media.treasureHintShader;
		break;
	case HINT_KNIFE:
		break;
	case HINT_LADDER:
		icon = cgs.media.ladderHintShader;
		break;
	case HINT_BUTTON:
		icon = cgs.media.buttonHintShader;
		break;
	case HINT_WATER:
		icon = cgs.media.waterHintShader;
		break;
	case HINT_CAUTION:
		icon = cgs.media.cautionHintShader;
		break;
	case HINT_DANGER:
		icon = cgs.media.dangerHintShader;
		break;
	case HINT_SECRET:
		icon = cgs.media.secretHintShader;
		break;
	case HINT_QUESTION:
		icon = cgs.media.qeustionHintShader;
		break;
	case HINT_EXCLAMATION:
		icon = cgs.media.exclamationHintShader;
		break;
	case HINT_CLIPBOARD:
		icon = cgs.media.clipboardHintShader;
		break;
	case HINT_WEAPON:
		icon = cgs.media.weaponHintShader;
		break;
	case HINT_AMMO:
		icon = cgs.media.ammoHintShader;
		break;
	case HINT_ARMOR:
		icon = cgs.media.armorHintShader;
		break;
	case HINT_POWERUP:
		icon = cgs.media.powerupHintShader;
		break;
	case HINT_HOLDABLE:
		icon = cgs.media.holdableHintShader;
		break;
	case HINT_INVENTORY:
		icon = cgs.media.inventoryHintShader;
		break;
	case HINT_PLYR_FRIEND:
		icon = cgs.media.hintPlrFriendShader;
		break;
	case HINT_PLYR_NEUTRAL:
		icon = cgs.media.hintPlrNeutralShader;
		break;
	case HINT_PLYR_ENEMY:
		icon = cgs.media.hintPlrEnemyShader;
		break;
	case HINT_PLYR_UNKNOWN:
		icon = cgs.media.hintPlrUnknownShader;
		break;

	// DHM - Nerve :: multiplayer hints
	case HINT_BUILD:
		icon = cgs.media.buildHintShader;
		break;
	case HINT_DISARM:
		icon = cgs.media.disarmHintShader;
		break;
	case HINT_REVIVE:
		break;
	case HINT_DYNAMITE:
		icon = cgs.media.dynamiteHintShader;
		break;
	// dhm - end

	// Mad Doc - TDF
	case HINT_LOCKPICK:
		icon      = cgs.media.doorLockHintShader;       // TAT 1/30/2003 - use the locked door hint cursor
		yellowbar = qtrue;      // draw the status bar in yellow so it shows up better
		break;

	case HINT_ACTIVATE:
	case HINT_PLAYER:
	default:
		icon = cgs.media.usableHintShader;
		break;
	}

	if (!icon) {
		return;
	}

	// color
	color = CG_FadeColor(cg.cursorHintTime, cg.cursorHintFade);
	if (!color) {
		trap_R_SetColor(NULL);
		return;
	}

	if (cg_cursorHints.integer == 3) {
		color[3] *= 0.5 + 0.5 * sin((float)cg.time / 150.0);
	}

	// size
	if (cg_cursorHints.integer >= 3) {     // no size pulsing
		scale = halfscale = 0;
	} else {
		if (cg_cursorHints.integer == 2) {
			scale = (float)((cg.cursorHintTime) % 1000) / 100.0f;     // one way size pulse
		} else {
			scale = 10 * (0.5 + 0.5 * sin((float)cg.time / 150.0));     // sin pulse
		}
		halfscale = scale * 0.5f;
	}

	// set color and draw the hint
	trap_R_SetColor(color);
	CG_DrawPic(middle - halfscale, y - halfscale, w + scale, h + scale, icon);

	trap_R_SetColor(NULL);

	// draw status bar under the cursor hint
	if (cg.cursorHintValue) {
		if (yellowbar) {
			Vector4Set(color, 1, 1, 0, 1.0f);
		} else {
			Vector4Set(color, 0, 0, 1, 0.5f);
		}
		CG_FilledBar(middle, y + h + 4, w, 8, color, NULL, NULL, (float)cg.cursorHintValue / 255.0f, 0);
	}
}

// THINKABOUTME: should these be exclusive or inclusive..
//
qboolean CG_OwnerDrawVisible(int flags) {
	// Nico, silent GCC
	(void)flags;

	return qfalse;
}

#define PIC_WIDTH 12

/*
==============
CG_DrawWeapStability
    draw a bar showing current stability level (0-255), max at current weapon/ability, and 'perfect' reference mark

    probably only drawn for scoped weapons
==============
*/
void CG_DrawWeapStability(void) {
	int x = 50;
	int y = 208;
	int w = 10;
	int h = 64;
	vec4_t goodColor = { 0, 1, 0, 0.5f }, badColor = { 1, 0, 0, 0.5f };

	if (!cg_drawSpreadScale.integer || cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	if (cg_drawSpreadScale.integer == 1 && !BG_IsScopedWeapon(cg.predictedPlayerState.weapon)) {
		// cg_drawSpreadScale of '1' means only draw for scoped weapons, '2' means draw all the time (for debugging)
		return;
	}

	if (cg.predictedPlayerState.weaponstate != WEAPON_READY) {
		return;
	}

	if (!(cg.snap->ps.aimSpreadScale)) {
		return;
	}

	if (cg.renderingThirdPerson) {
		return;
	}

	CG_FilledBar(x, y, w, h, goodColor, badColor, NULL, (float)cg.snap->ps.aimSpreadScale / 255.0f, 2 | 4 | 256);   // flags (BAR_CENTER|BAR_VERT|BAR_LERP_COLOR)
}

/*
==============
CG_DrawWeapHeat
==============
*/
void CG_DrawWeapHeat(int align) {
	vec4_t color = { 1, 0, 0, 0.2f }, color2 = { 1, 0, 0, 0.5f };
	int    flags = 0;
	float  x, y;

	if (!(cg.snap->ps.curWeapHeat)) {
		return;
	}

	if (align != HUD_HORIZONTAL) {
		flags |= 4;   // BAR_VERT
	}

	flags |= 1;       // BAR_LEFT			- this is hardcoded now, but will be decided by the menu script
	flags |= 16;      // BAR_BG			- draw the filled contrast box
	flags |= 256;     // BAR_COLOR_LERP

	x = CG_WideX(cg_weaponHeatX.value);
	y = cg_weaponHeatX.value;

	CG_FilledBar(x, y, 60, 32, color, color2, NULL, (float)cg.snap->ps.curWeapHeat / 255.0f, flags);
}

void CG_MouseEvent(int x, int y) {
	switch ((int)cgs.eventHandling) {
	case CGAME_EVENT_SPEAKEREDITOR:
	case CGAME_EVENT_GAMEVIEW:
	case CGAME_EVENT_CAMPAIGNBREIFING:
	case CGAME_EVENT_FIRETEAMMSG:
		cgs.cursorX += x;
		if (cgs.cursorX < 0) {
			cgs.cursorX = 0;
		} else if (cgs.cursorX > CG_WideX(SCREEN_WIDTH)) {
			cgs.cursorX = CG_WideX(SCREEN_WIDTH);
		}

		cgs.cursorY += y;
		if (cgs.cursorY < 0) {
			cgs.cursorY = 0;
		} else if (cgs.cursorY > SCREEN_HEIGHT) {
			cgs.cursorY = SCREEN_HEIGHT;
		}

		if ((int)cgs.eventHandling == CGAME_EVENT_SPEAKEREDITOR) {
			CG_SpeakerEditorMouseMove_Handling(x, y);
		}

		break;
	case CGAME_EVENT_DEMO:
		cgs.cursorX += x;
		if (cgs.cursorX < 0) {
			cgs.cursorX = 0;
		} else if (cgs.cursorX > CG_WideX(SCREEN_WIDTH)) {
			cgs.cursorX = CG_WideX(SCREEN_WIDTH);
		}

		cgs.cursorY += y;
		if (cgs.cursorY < 0) {
			cgs.cursorY = 0;
		} else if (cgs.cursorY > SCREEN_HEIGHT) {
			cgs.cursorY = SCREEN_HEIGHT;
		}

		if (x != 0 || y != 0) {
			cgs.cursorUpdate = cg.time + 5000;
		}
		break;

	default:
		// default handling
		if ((cg.predictedPlayerState.pm_type == PM_NORMAL ||
		     cg.predictedPlayerState.pm_type == PM_SPECTATOR) &&
		    cg.showScores == qfalse) {
			trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_CGAME);
			return;
		}
		break;
	}
}

/*
==================
CG_EventHandling
==================
*/
void CG_EventHandling(int type, qboolean fForced) {
	if (cg.demoPlayback && type == CGAME_EVENT_NONE && !fForced) {
		type = CGAME_EVENT_DEMO;
	}

	if (type != CGAME_EVENT_NONE) {
		trap_Cvar_Set("cl_bypassMouseInput", 0);
	}

	switch (type) {
	// OSP - Demo support
	case CGAME_EVENT_DEMO:
		cgs.fResize         = qfalse;
		cgs.fSelect         = qfalse;
		cgs.cursorUpdate    = cg.time + 10000;
		cgs.timescaleUpdate = cg.time + 4000;
		CG_ScoresUp_f();
		break;

	default:
		// default handling (cleanup mostly)
		if ((int)cgs.eventHandling == CGAME_EVENT_GAMEVIEW) {
			cg.showGameView = qfalse;
			trap_S_FadeBackgroundTrack(0.0f, 500, 0);

			trap_S_StopStreamingSound(-1);

			if (fForced && cgs.limboLoadoutModified) {
				trap_SendClientCommand("rs");
				cgs.limboLoadoutSelected = qfalse;
			}
		} else if ((int)cgs.eventHandling == CGAME_EVENT_SPEAKEREDITOR) {
			if (type == -CGAME_EVENT_SPEAKEREDITOR) {
				type = CGAME_EVENT_NONE;
			} else {
				trap_Key_SetCatcher(KEYCATCH_CGAME);
				return;
			}
		} else if ((int)cgs.eventHandling == CGAME_EVENT_CAMPAIGNBREIFING) {
			type = CGAME_EVENT_GAMEVIEW;
		} else if ((int)cgs.eventHandling == CGAME_EVENT_FIRETEAMMSG) {
			cg.showFireteamMenu = qfalse;
			trap_Cvar_Set("cl_bypassmouseinput", "0");
		}
		break;
	}

	cgs.eventHandling = type;

	if (type == CGAME_EVENT_NONE) {
		trap_Key_SetCatcher(trap_Key_GetCatcher() & ~KEYCATCH_CGAME);

		if (cg.demoPlayback && cg.demohelpWindow != SHOW_OFF) {
			CG_ShowHelp_Off(&cg.demohelpWindow);
		}
	} else if (type == CGAME_EVENT_GAMEVIEW) {
		cg.showGameView = qtrue;
		CG_LimboPanel_Setup();
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	} else if (type == CGAME_EVENT_FIRETEAMMSG) {
		cgs.ftMenuPos       = -1;
		cgs.ftMenuMode      = 0;
		cg.showFireteamMenu = qtrue;
		trap_Cvar_Set("cl_bypassmouseinput", "1");
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	} else {
		trap_Key_SetCatcher(KEYCATCH_CGAME);
	}
}

void CG_KeyEvent(int key, qboolean down) {
	switch ((int)cgs.eventHandling) {
	// Demos get their own keys
	case CGAME_EVENT_DEMO:
		CG_DemoClick(key, down);
		return;

	case CGAME_EVENT_CAMPAIGNBREIFING:
		CG_LoadPanel_KeyHandling(key, down);
		break;

	case CGAME_EVENT_FIRETEAMMSG:
		CG_Fireteams_KeyHandling(key, down);
		break;

	case CGAME_EVENT_GAMEVIEW:
		CG_LimboPanel_KeyHandling(key, down);
		break;

	case CGAME_EVENT_SPEAKEREDITOR:
		CG_SpeakerEditor_KeyHandling(key, down);
		break;

	default:
		// default handling
		if (!down) {
			return;
		}

		if (cg.predictedPlayerState.pm_type == PM_NORMAL ||
		    (cg.predictedPlayerState.pm_type == PM_SPECTATOR && cg.showScores == qfalse)) {

			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
			return;
		}
		break;
	}
}

void CG_RunMenuScript(char **args) {
	// Nico, silent GCC
	(void)args;
}
