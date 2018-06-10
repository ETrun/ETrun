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
 * name:		cg_main.c
 *
 * desc:		initialization and primary entry point for cgame
 *
*/

#include "cg_local.h"
#include "../../libs/sha-1/sha1.h"

displayContextDef_t cgDC;

void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum);
void CG_Shutdown(void);
qboolean CG_CheckExecKey(int key);
extern itemDef_t *g_bindItem;
extern qboolean  g_waitingForKey;

/*
================
vmMain

This is the only way control passes into the module.
================
*/
Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11) {
	// Nico, silent GCC
	(void)arg3;
	(void)arg4;
	(void)arg5;
	(void)arg6;
	(void)arg6;
	(void)arg7;
	(void)arg8;
	(void)arg9;
	(void)arg10;
	(void)arg11;

	switch (command) {
	case CG_INIT:
		CG_Init(arg0, arg1, arg2);
		cgs.initing = qfalse;
		return 0;
	case CG_SHUTDOWN:
		CG_Shutdown();
		return 0;
	case CG_CONSOLE_COMMAND:
		return CG_ConsoleCommand();
	case CG_DRAW_ACTIVE_FRAME:
		CG_DrawActiveFrame(arg0, arg1, arg2);
		return 0;
	case CG_CROSSHAIR_PLAYER:
		return CG_CrosshairPlayer();
	case CG_LAST_ATTACKER:
		return CG_LastAttacker();
	case CG_KEY_EVENT:
		CG_KeyEvent(arg0, arg1);
		return 0;
	case CG_MOUSE_EVENT:
		cgDC.cursorx = cgs.cursorX;
		cgDC.cursory = cgs.cursorY;
		CG_MouseEvent(arg0, arg1);
		return 0;
	case CG_EVENT_HANDLING:
		CG_EventHandling(arg0, qtrue);
		return 0;
	case CG_GET_TAG:
		return CG_GetTag(arg0, (char *)arg1, (orientation_t *)arg2);
	case CG_CHECKEXECKEY:
		return CG_CheckExecKey(arg0);
	case CG_WANTSBINDKEYS:
		return (g_waitingForKey && g_bindItem) ? qtrue : qfalse;
	case CG_MESSAGERECEIVED:
		return -1;
	default:
		CG_Error("vmMain: unknown command %i", command);
		break;
	}
	return -1;
}

cg_t         cg;
cgs_t        cgs;
centity_t    cg_entities[MAX_GENTITIES];
weaponInfo_t cg_weapons[MAX_WEAPONS];
itemInfo_t   cg_items[MAX_ITEMS];

vmCvar_t cg_railTrailTime;
vmCvar_t cg_centertime;
vmCvar_t cg_runpitch;
vmCvar_t cg_runroll;
vmCvar_t cg_bobup;
vmCvar_t cg_bobpitch;
vmCvar_t cg_bobroll;
vmCvar_t cg_bobyaw;
vmCvar_t cg_swingSpeed;
vmCvar_t cg_shadows;
vmCvar_t cg_draw2D;
vmCvar_t cg_drawFPS;
vmCvar_t cg_drawSnapshot;
vmCvar_t cg_drawCrosshair;
vmCvar_t cg_drawCrosshairNames;
vmCvar_t cg_drawCrosshairPickups;
vmCvar_t cg_weaponCycleDelay;       //----(SA)	added
vmCvar_t cg_cycleAllWeaps;
vmCvar_t cg_useWeapsForZoom;
vmCvar_t cg_crosshairSize;
vmCvar_t cg_crosshairX;
vmCvar_t cg_crosshairY;
vmCvar_t cg_crosshairHealth;
vmCvar_t cg_teamChatsOnly;
vmCvar_t cg_noVoiceChats;           // NERVE - SMF
vmCvar_t cg_noVoiceText;            // NERVE - SMF
vmCvar_t cg_drawStatus;
vmCvar_t cg_animSpeed;
vmCvar_t cg_drawSpreadScale;
vmCvar_t cg_debugAnim;
vmCvar_t cg_debugPosition;
vmCvar_t cg_debugEvents;
vmCvar_t cg_errorDecay;
vmCvar_t cg_nopredict;
vmCvar_t cg_noPlayerAnims;
vmCvar_t cg_showmiss;
vmCvar_t cg_footsteps;
vmCvar_t cg_markTime;
vmCvar_t cg_brassTime;
vmCvar_t cg_letterbox;   //----(SA)	added
vmCvar_t cg_drawGun;
vmCvar_t cg_cursorHints;    //----(SA)	added
vmCvar_t cg_gun_frame;
vmCvar_t cg_gun_x;
vmCvar_t cg_gun_y;
vmCvar_t cg_gun_z;
vmCvar_t cg_tracerChance;
vmCvar_t cg_tracerWidth;
vmCvar_t cg_tracerLength;
vmCvar_t cg_tracerSpeed;
vmCvar_t cg_autoswitch;
vmCvar_t cg_ignore;
vmCvar_t cg_fov;
vmCvar_t cg_zoomStepSniper;
vmCvar_t cg_zoomStepBinoc;
vmCvar_t cg_zoomDefaultSniper;
vmCvar_t cg_thirdPerson;
vmCvar_t cg_thirdPersonRange;
vmCvar_t cg_thirdPersonAngle;
vmCvar_t cg_stereoSeparation;
vmCvar_t cg_lagometer;
vmCvar_t cg_teamChatTime;
vmCvar_t cg_teamChatHeight;
vmCvar_t cg_stats;
vmCvar_t cg_buildScript;
vmCvar_t cg_coronafardist;
vmCvar_t cg_coronas;
vmCvar_t cg_paused;
vmCvar_t cg_predictItems;
vmCvar_t cg_enableBreath;
vmCvar_t cg_autoactivate;
vmCvar_t pmove_fixed;
vmCvar_t pmove_msec;
vmCvar_t cg_wolfparticles;
vmCvar_t cg_norender;
vmCvar_t cg_skybox;
vmCvar_t cg_messageType;
vmCvar_t cg_messagePlayer;
vmCvar_t cg_timescale;
vmCvar_t cg_noTaunt;                // NERVE - SMF
vmCvar_t cg_voiceSpriteTime;    // DHM - Nerve
vmCvar_t cg_drawNotifyText;
vmCvar_t cg_quickMessageAlt;
vmCvar_t cg_descriptiveText;
vmCvar_t cg_antilag;
vmCvar_t developer;
vmCvar_t authLevel;
vmCvar_t cg_autoAction;
vmCvar_t cg_autoReload;
vmCvar_t cg_crosshairAlpha;
vmCvar_t cg_crosshairAlphaAlt;
vmCvar_t cg_crosshairColor;
vmCvar_t cg_crosshairColorAlt;
vmCvar_t cg_crosshairPulse;
vmCvar_t cg_drawWeaponIconFlash;
vmCvar_t cg_noAmmoAutoSwitch;
vmCvar_t cg_printObjectiveInfo;
vmCvar_t cg_specHelp;
vmCvar_t cg_uinfo;
vmCvar_t cg_useScreenshotJPEG;
vmCvar_t demo_avifpsF1;
vmCvar_t demo_avifpsF2;
vmCvar_t demo_avifpsF3;
vmCvar_t demo_avifpsF4;
vmCvar_t demo_avifpsF5;
vmCvar_t demo_drawTimeScale;
vmCvar_t demo_infoWindow;
vmCvar_t int_cl_maxpackets;
vmCvar_t int_cl_timenudge;
vmCvar_t int_cl_yawspeed;
vmCvar_t int_cl_pitchspeed;
vmCvar_t int_timescale;
vmCvar_t cg_rconPassword;
vmCvar_t cg_refereePassword;
vmCvar_t cg_atmosphericEffects;
vmCvar_t cg_drawFireteamOverlay;
vmCvar_t cg_drawSmallPopupIcons;

//bani - demo recording cvars
vmCvar_t cl_demorecording;
vmCvar_t cl_demofilename;
vmCvar_t cl_demooffset;
//bani - wav recording cvars
vmCvar_t cl_waverecording;
vmCvar_t cl_wavefilename;
vmCvar_t cl_waveoffset;
vmCvar_t cg_recording_statusline;

// Nico, beginning of ETrun client cvars

// Game physics
vmCvar_t physics;

// Is level a timerun?
vmCvar_t isTimerun;

// Speed meter
vmCvar_t cg_drawSpeedMeter;
vmCvar_t cg_speedMeterX;
vmCvar_t cg_speedMeterY;

// Accel HUD
vmCvar_t cg_drawAccel;
vmCvar_t cg_accelSmoothness;

// Timer
vmCvar_t cg_drawTimer;
vmCvar_t cg_timerX;
vmCvar_t cg_timerY;

// Check points
vmCvar_t cg_drawCheckPoints;
vmCvar_t cg_checkPointsX;
vmCvar_t cg_checkPointsY;
vmCvar_t cg_maxCheckPoints;

// Noclip speed scale
vmCvar_t cg_noclipSpeed;

// Max FPS
vmCvar_t com_maxfps;

// Slick detector
vmCvar_t cg_drawSlick;

// OB detector
vmCvar_t cg_drawOB;

// Hide other players
vmCvar_t cg_hideOthers;
vmCvar_t cg_hideRange;

// Auth related
vmCvar_t cg_authToken;
vmCvar_t cg_autoLogin;

// CGaz
vmCvar_t cg_drawCGaz;

// Velocity Snapping
vmCvar_t cg_drawVelocitySnapping;
vmCvar_t cg_velocitySnappingH;
vmCvar_t cg_velocitySnappingY;
vmCvar_t cg_velocitySnappingFov;

// Load view angles on load
vmCvar_t cg_loadViewAngles;

// Load weapon on load
vmCvar_t cg_loadWeapon;

// Show pressed keys
vmCvar_t cg_drawKeys;
vmCvar_t cg_keysX;
vmCvar_t cg_keysY;
vmCvar_t cg_keysSize;

// Automatically load player position when he gets killed (except /kill)
vmCvar_t cg_autoLoad;

// View log (ET Console)
vmCvar_t cg_viewLog;

// Hide me
vmCvar_t cg_hideMe;

// Auto demo
vmCvar_t cg_autoDemo;
vmCvar_t cg_keepAllDemos;

// Popups
vmCvar_t cg_numPopups;
vmCvar_t cg_popupTime;
vmCvar_t cg_popupStayTime;
vmCvar_t cg_popupFadeTime;

// Automatically load checkpoints
vmCvar_t cg_autoLoadCheckpoints;

// Persistant speclock
vmCvar_t cg_specLock;

// Info panel
vmCvar_t cg_drawInfoPanel;
vmCvar_t cg_infoPanelX;
vmCvar_t cg_infoPanelY;

// Country flags
vmCvar_t cg_countryFlags;

// Minimum start speed
vmCvar_t cg_minStartSpeed;

// Nico, end of ETrun cvars

typedef struct {
	vmCvar_t *vmCvar;
	char *cvarName;
	char *defaultString;
	int cvarFlags;
	int modificationCount;
} cvarTable_t;

cvarTable_t cvarTable[] =
{
	{ &cg_ignore,               "cg_ignore",               "0",     0,                        0 }, // used for debugging
	{ &cg_autoswitch,           "cg_autoswitch",           "2",     CVAR_ARCHIVE,             0 },
	{ &cg_drawGun,              "cg_drawGun",              "1",     CVAR_ARCHIVE,             0 },
	{ &cg_gun_frame,            "cg_gun_frame",            "0",     CVAR_TEMP,                0 },
	{ &cg_cursorHints,          "cg_cursorHints",          "1",     CVAR_ARCHIVE,             0 },
	{ &cg_zoomDefaultSniper,    "cg_zoomDefaultSniper",    "20",    CVAR_ARCHIVE,             0 }, // JPW NERVE changed per atvi req
	{ &cg_zoomStepSniper,       "cg_zoomStepSniper",       "2",     CVAR_ARCHIVE,             0 },
	{ &cg_zoomStepBinoc,        "cg_zoomStepBinoc",        "2",     CVAR_ARCHIVE,             0 },
	{ &cg_fov,                  "cg_fov",                  "90",    CVAR_ARCHIVE,             0 },
	{ &cg_letterbox,            "cg_letterbox",            "0",     CVAR_TEMP,                0 }, //----(SA)	added
	{ &cg_stereoSeparation,     "cg_stereoSeparation",     "0.4",   CVAR_ARCHIVE,             0 },
	{ &cg_shadows,              "cg_shadows",              "1",     CVAR_ARCHIVE,             0 },
	{ &cg_draw2D,               "cg_draw2D",               "1",     CVAR_ARCHIVE,             0 },
	{ &cg_drawSpreadScale,      "cg_drawSpreadScale",      "1",     CVAR_ARCHIVE,             0 },
	{ &cg_drawStatus,           "cg_drawStatus",           "1",     CVAR_ARCHIVE,             0 },
	{ &cg_drawFPS,              "cg_drawFPS",              "0",     CVAR_ARCHIVE,             0 },
	{ &cg_drawSnapshot,         "cg_drawSnapshot",         "0",     CVAR_ARCHIVE,             0 },
	{ &cg_drawCrosshair,        "cg_drawCrosshair",        "1",     CVAR_ARCHIVE,             0 },
	{ &cg_drawCrosshairNames,   "cg_drawCrosshairNames",   "1",     CVAR_ARCHIVE,             0 },
	{ &cg_drawCrosshairPickups, "cg_drawCrosshairPickups", "1",     CVAR_ARCHIVE,             0 },
	{ &cg_useWeapsForZoom,      "cg_useWeapsForZoom",      "1",     CVAR_ARCHIVE,             0 },
	{ &cg_weaponCycleDelay,     "cg_weaponCycleDelay",     "150",   CVAR_ARCHIVE,             0 }, //----(SA)	added
	{ &cg_cycleAllWeaps,        "cg_cycleAllWeaps",        "1",     CVAR_ARCHIVE,             0 },
	{ &cg_crosshairSize,        "cg_crosshairSize",        "48",    CVAR_ARCHIVE,             0 },
	{ &cg_crosshairHealth,      "cg_crosshairHealth",      "0",     CVAR_ARCHIVE,             0 },
	{ &cg_crosshairX,           "cg_crosshairX",           "0",     CVAR_ARCHIVE,             0 },
	{ &cg_crosshairY,           "cg_crosshairY",           "0",     CVAR_ARCHIVE,             0 },
	{ &cg_brassTime,            "cg_brassTime",            "2500",  CVAR_ARCHIVE,             0 }, // JPW NERVE
	{ &cg_markTime,             "cg_marktime",             "20000", CVAR_ARCHIVE,             0 },
	{ &cg_lagometer,            "cg_lagometer",            "0",     CVAR_ARCHIVE,             0 },
	{ &cg_railTrailTime,        "cg_railTrailTime",        "400",   CVAR_ARCHIVE,             0 },
	{ &cg_gun_x,                "cg_gunX",                 "0",     CVAR_CHEAT,               0 },
	{ &cg_gun_y,                "cg_gunY",                 "0",     CVAR_CHEAT,               0 },
	{ &cg_gun_z,                "cg_gunZ",                 "0",     CVAR_CHEAT,               0 },
	{ &cg_centertime,           "cg_centertime",           "5",     CVAR_CHEAT,               0 }, // DHM - Nerve :: changed from 3 to 5
	{ &cg_runpitch,             "cg_runpitch",             "0.002", CVAR_ARCHIVE,             0 },
	{ &cg_runroll,              "cg_runroll",              "0.005", CVAR_ARCHIVE,             0 },
	{ &cg_bobup,                "cg_bobup",                "0.005", CVAR_ARCHIVE,             0 },
	{ &cg_bobpitch,             "cg_bobpitch",             "0.002", CVAR_ARCHIVE,             0 },
	{ &cg_bobroll,              "cg_bobroll",              "0.002", CVAR_ARCHIVE,             0 },
	{ &cg_bobyaw,               "cg_bobyaw",               "0.002", CVAR_ARCHIVE,             0 },

	// JOSEPH 10-27-99
	{ &cg_autoactivate,         "cg_autoactivate",         "1",     CVAR_ARCHIVE,             0 },
	// END JOSEPH

	// Ridah, more fluid rotations
	{ &cg_swingSpeed,           "cg_swingSpeed",           "0.1",   CVAR_CHEAT,               0 }, // was 0.3 for Q3
	{ &cg_skybox,               "cg_skybox",               "1",     CVAR_CHEAT,               0 },
	// done.

	// ydnar: say, team say, etc.
	{ &cg_messageType,          "cg_messageType",          "1",     CVAR_TEMP,                0 },
	{ &cg_messagePlayer,        "cg_messagePlayer",        "",      CVAR_TEMP,                0 },

	{ &cg_animSpeed,            "cg_animspeed",            "1",     CVAR_CHEAT,               0 },
	{ &cg_debugAnim,            "cg_debuganim",            "0",     CVAR_CHEAT,               0 },
	{ &cg_debugPosition,        "cg_debugposition",        "0",     CVAR_CHEAT,               0 },
	{ &cg_debugEvents,          "cg_debugevents",          "0",     CVAR_CHEAT,               0 },
	{ &cg_errorDecay,           "cg_errordecay",           "100",   0,                        0 },
	{ &cg_nopredict,            "cg_nopredict",            "0",     CVAR_CHEAT,               0 },
	{ &cg_noPlayerAnims,        "cg_noplayeranims",        "0",     CVAR_CHEAT,               0 },
	{ &cg_showmiss,             "cg_showmiss",             "0",     0,                        0 },
	{ &cg_footsteps,            "cg_footsteps",            "1",     CVAR_ARCHIVE,             0 }, // Nico, removed CVAR_CHEAT flag
	{ &cg_tracerChance,         "cg_tracerchance",         "0.4",   CVAR_CHEAT,               0 },
	{ &cg_tracerWidth,          "cg_tracerwidth",          "0.8",   CVAR_CHEAT,               0 },
	{ &cg_tracerSpeed,          "cg_tracerSpeed",          "4500",  CVAR_CHEAT,               0 },
	{ &cg_tracerLength,         "cg_tracerlength",         "160",   CVAR_CHEAT,               0 },
	{ &cg_thirdPersonRange,     "cg_thirdPersonRange",     "80",    CVAR_CHEAT,               0 }, // JPW NERVE per atvi req
	{ &cg_thirdPersonAngle,     "cg_thirdPersonAngle",     "0",     CVAR_CHEAT,               0 },
	{ &cg_thirdPerson,          "cg_thirdPerson",          "0",     CVAR_CHEAT,               0 }, // JPW NERVE per atvi req
	{ &cg_teamChatTime,         "cg_teamChatTime",         "8000",  CVAR_ARCHIVE,             0 },
	{ &cg_teamChatHeight,       "cg_teamChatHeight",       "8",     CVAR_ARCHIVE,             0 },
	{ &cg_coronafardist,        "cg_coronafardist",        "1536",  CVAR_ARCHIVE,             0 },
	{ &cg_coronas,              "cg_coronas",              "1",     CVAR_ARCHIVE,             0 },
	{ &cg_predictItems,         "cg_predictItems",         "1",     CVAR_ARCHIVE,             0 },
	{ &cg_stats,                "cg_stats",                "0",     0,                        0 },

	{ &cg_enableBreath,         "cg_enableBreath",         "1",     CVAR_SERVERINFO,          0 },
	{ &cg_timescale,            "timescale",               "1",     0,                        0 },

	{ &pmove_fixed,             "pmove_fixed",             "1",     0,                        0 },
	{ &pmove_msec,              "pmove_msec",              "8",     CVAR_CHEAT,               0 },

	{ &cg_noTaunt,              "cg_noTaunt",              "0",     CVAR_ARCHIVE,             0 }, // NERVE - SMF
	{ &cg_voiceSpriteTime,      "cg_voiceSpriteTime",      "6000",  CVAR_ARCHIVE,             0 }, // DHM - Nerve

	{ &cg_teamChatsOnly,        "cg_teamChatsOnly",        "0",     CVAR_ARCHIVE,             0 },
	{ &cg_noVoiceChats,         "cg_noVoiceChats",         "0",     CVAR_ARCHIVE,             0 }, // NERVE - SMF
	{ &cg_noVoiceText,          "cg_noVoiceText",          "0",     CVAR_ARCHIVE,             0 }, // NERVE - SMF

	// the following variables are created in other parts of the system,
	// but we also reference them here

	{ &cg_buildScript,          "com_buildScript",         "0",     0,                        0 }, // force loading of all possible data amd error on failures
	{ &cg_paused,               "cl_paused",               "0",     CVAR_ROM,                 0 },
	// Rafael - particle switch
	{ &cg_wolfparticles,        "cg_wolfparticles",        "1",     CVAR_ARCHIVE,             0 },

	{ &cg_norender,             "cg_norender",             "0",     0,                        0 }, // only used during single player, to suppress rendering until the server is ready

	{ &cg_drawNotifyText,       "cg_drawNotifyText",       "1",     CVAR_ARCHIVE,             0 },
	{ &cg_quickMessageAlt,      "cg_quickMessageAlt",      "0",     CVAR_ARCHIVE,             0 },
	{ &cg_descriptiveText,      "cg_descriptiveText",      "1",     CVAR_ARCHIVE,             0 },
	{ &cg_antilag,              "g_antilag",               "1",     0,                        0 },
	{ &developer,               "developer",               "0",     CVAR_CHEAT,               0 },

	{ &cg_autoAction,           "cg_autoAction",           "0",     CVAR_ARCHIVE,             0 },
	{ &cg_autoReload,           "cg_autoReload",           "1",     CVAR_ARCHIVE,             0 },

	{ &cg_crosshairAlpha,       "cg_crosshairAlpha",       "1.0",   CVAR_ARCHIVE,             0 },
	{ &cg_crosshairAlphaAlt,    "cg_crosshairAlphaAlt",    "1.0",   CVAR_ARCHIVE,             0 },
	{ &cg_crosshairColor,       "cg_crosshairColor",       "White", CVAR_ARCHIVE,             0 },
	{ &cg_crosshairColorAlt,    "cg_crosshairColorAlt",    "White", CVAR_ARCHIVE,             0 },
	{ &cg_crosshairPulse,       "cg_crosshairPulse",       "1",     CVAR_ARCHIVE,             0 },
	{ &cg_drawWeaponIconFlash,  "cg_drawWeaponIconFlash",  "0",     CVAR_ARCHIVE,             0 },
	{ &cg_noAmmoAutoSwitch,     "cg_noAmmoAutoSwitch",     "1",     CVAR_ARCHIVE,             0 },
	{ &cg_printObjectiveInfo,   "cg_printObjectiveInfo",   "1",     CVAR_ARCHIVE,             0 },
	{ &cg_specHelp,             "cg_specHelp",             "1",     CVAR_ARCHIVE,             0 },
	{ &cg_uinfo,                "cg_uinfo",                "0",     CVAR_ROM | CVAR_USERINFO, 0 },
	{ &cg_useScreenshotJPEG,    "cg_useScreenshotJPEG",    "1",     CVAR_ARCHIVE,             0 },

	{ &demo_avifpsF1,           "demo_avifpsF1",           "0",     CVAR_ARCHIVE,             0 },
	{ &demo_avifpsF2,           "demo_avifpsF2",           "10",    CVAR_ARCHIVE,             0 },
	{ &demo_avifpsF3,           "demo_avifpsF3",           "15",    CVAR_ARCHIVE,             0 },
	{ &demo_avifpsF4,           "demo_avifpsF4",           "20",    CVAR_ARCHIVE,             0 },
	{ &demo_avifpsF5,           "demo_avifpsF5",           "24",    CVAR_ARCHIVE,             0 },
	{ &demo_drawTimeScale,      "demo_drawTimeScale",      "1",     CVAR_ARCHIVE,             0 },
	{ &demo_infoWindow,         "demo_infoWindow",         "1",     CVAR_ARCHIVE,             0 },

	// Engine mappings
	{ &int_cl_maxpackets,       "cl_maxpackets",           "30",    CVAR_ARCHIVE,             0 },
	{ &int_cl_timenudge,        "cl_timenudge",            "0",     CVAR_ARCHIVE,             0 },
	
	// suburb, yawspeed & pitchspeed
	{ &int_cl_yawspeed,         "cl_yawspeed",             "140",   CVAR_ARCHIVE,             0 },
	{ &int_cl_pitchspeed,       "cl_pitchspeed",           "140",   CVAR_ARCHIVE,             0 },

	{ &cg_atmosphericEffects,   "cg_atmosphericEffects",   "1",     CVAR_ARCHIVE,             0 },
	{ &authLevel,               "authLevel",               "0",     CVAR_TEMP | CVAR_ROM,     0 },

	{ &cg_rconPassword,         "auth_rconPassword",       "",      CVAR_TEMP,                0 },
	{ &cg_refereePassword,      "auth_refereePassword",    "",      CVAR_TEMP,                0 },

	{ NULL,                     "cg_etVersion",            "",      CVAR_USERINFO | CVAR_ROM, 0 },
	{ &cg_drawFireteamOverlay,  "cg_drawFireteamOverlay",  "1",     CVAR_ARCHIVE,             0 },
	{ &cg_drawSmallPopupIcons,  "cg_drawSmallPopupIcons",  "0",     CVAR_ARCHIVE,             0 },

	//bani - demo recording cvars
	{ &cl_demorecording,        "cl_demorecording",        "0",     CVAR_ROM,                 0 },
	{ &cl_demofilename,         "cl_demofilename",         "",      CVAR_ROM,                 0 },
	{ &cl_demooffset,           "cl_demooffset",           "0",     CVAR_ROM,                 0 },
	//bani - wav recording cvars
	{ &cl_waverecording,        "cl_waverecording",        "0",     CVAR_ROM,                 0 },
	{ &cl_wavefilename,         "cl_wavefilename",         "",      CVAR_ROM,                 0 },
	{ &cl_waveoffset,           "cl_waveoffset",           "0",     CVAR_ROM,                 0 },
	{ &cg_recording_statusline, "cg_recording_statusline", "9",     CVAR_ARCHIVE,             0 },

	// Nico, beginning of ETrun client cvars

	// Game physics
	{ &physics,                 "physics",                 "0",     CVAR_ROM,                 0 },

	// Is level a timerun?
	{ &isTimerun,               "isTimerun",               "0",     CVAR_ROM | CVAR_CHEAT,    0 },

	// Speed meter
	{ &cg_drawSpeedMeter,       "cg_drawSpeedMeter",       "1",     CVAR_ARCHIVE,             0 },
	{ &cg_speedMeterX,          "cg_speedMeterX",          "320",   CVAR_ARCHIVE,             0 },
	{ &cg_speedMeterY,          "cg_speedMeterY",          "220",   CVAR_ARCHIVE,             0 },

	// Accel HUD
	{ &cg_drawAccel,            "cg_drawAccel",            "0",     CVAR_ARCHIVE,             0 },
	{ &cg_accelSmoothness,      "cg_accelSmoothness",      "100",   CVAR_ARCHIVE,             0 },

	// Timer
	{ &cg_drawTimer,            "cg_drawTimer",            "1",     CVAR_ARCHIVE,             0 },
	{ &cg_timerX,               "cg_timerX",               "320",   CVAR_ARCHIVE,             0 },
	{ &cg_timerY,               "cg_timerY",               "420",   CVAR_ARCHIVE,             0 },

	// Check points
	{ &cg_drawCheckPoints,      "cg_drawCheckPoints",      "1",     CVAR_ARCHIVE,             0 },
	{ &cg_checkPointsX,         "cg_checkPointsX",         "320",   CVAR_ARCHIVE,             0 },
	{ &cg_checkPointsY,         "cg_checkPointsY",         "435",   CVAR_ARCHIVE,             0 },
	{ &cg_maxCheckPoints,       "cg_maxCheckPoints",       "5",     CVAR_ARCHIVE,             0 },

	// Com_maxFPS
	{ &com_maxfps,              "com_maxfps",              "125",   CVAR_ARCHIVE,             0 },

	// Noclip speed scale
	{ &cg_noclipSpeed,          "cg_noclipSpeed",          "1000",  CVAR_ARCHIVE,             0 },

	// Slick detector
	{ &cg_drawSlick,            "cg_drawSlick",            "0",     CVAR_ARCHIVE,             0 },

	// OB detector
	{ &cg_drawOB,               "cg_drawOB",               "0",     CVAR_ARCHIVE,             0 },

	// Hide other players
	{ &cg_hideOthers,           "cg_hideOthers",           "1",     CVAR_ARCHIVE,             0 },
	{ &cg_hideRange,            "cg_hideRange",            "128",   CVAR_ARCHIVE,             0 },

	// Auth related
	{ &cg_authToken,            "cg_timerunsToken",        "",      CVAR_ARCHIVE,             0 },
	{ &cg_autoLogin,            "cg_autoLogin",            "0",     CVAR_ARCHIVE,             0 },

	// CGaz
	{ &cg_drawCGaz,             "cg_drawCGaz",             "0",     CVAR_ARCHIVE,             0 },

	// Velocity Snapping
	{ &cg_drawVelocitySnapping,   "cg_drawVelocitySnapping",    "0",     CVAR_ARCHIVE,        0 },
	{ &cg_velocitySnappingH,      "cg_velocitySnappingH",       "8",     CVAR_ARCHIVE,        0 },
	{ &cg_velocitySnappingY,      "cg_velocitySnappingY",       "248",   CVAR_ARCHIVE,        0 },
	{ &cg_velocitySnappingFov,    "cg_velocitySnappingFov",     "120",   CVAR_ARCHIVE,        0 },

	// Load view angles on load
	{ &cg_loadViewAngles,       "cg_loadViewAngles",       "1",     CVAR_ARCHIVE,             0 },

	// Load weapon on load
	{ &cg_loadWeapon,           "cg_loadWeapon",           "1",     CVAR_ARCHIVE,             0 },

	// Show pressed keys
	{ &cg_drawKeys,             "cg_drawKeys",             "1",     CVAR_ARCHIVE,             0 },
	{ &cg_keysX,                "cg_keysX",                "550",   CVAR_ARCHIVE,             0 },
	{ &cg_keysY,                "cg_keysY",                "210",   CVAR_ARCHIVE,             0 },
	{ &cg_keysSize,             "cg_keysSize",             "64",    CVAR_ARCHIVE,             0 },

	// Automatically load player position when he gets killed (except /kill)
	{ &cg_autoLoad,             "cg_autoLoad",             "1",     CVAR_ARCHIVE,             0 },

	// View log (ET Console)
	{ &cg_viewLog,              "cg_viewLog",              "0",     CVAR_ARCHIVE,             0 },

	// Hide me
	{ &cg_hideMe,               "cg_hideMe",               "0",     CVAR_ARCHIVE,             0 },

	// Auto demo
	{ &cg_autoDemo,             "cg_autoDemo",             "0",     CVAR_ARCHIVE,             0 },
	{ &cg_keepAllDemos,         "cg_keepAllDemos",         "1",     CVAR_ARCHIVE,             0 },

	// Popups
	{ &cg_numPopups,            "cg_numPopups",            "5",     CVAR_ARCHIVE,             0 },
	{ &cg_popupTime,            "cg_popupTime",            "1000",  CVAR_ARCHIVE,             0 },
	{ &cg_popupStayTime,        "cg_popupStayTime",        "2000",  CVAR_ARCHIVE,             0 },
	{ &cg_popupFadeTime,        "cg_popupFadeTime",        "2500",  CVAR_ARCHIVE,             0 },

	// Automatically load checkpoints
	{ &cg_autoLoadCheckpoints,  "cg_autoLoadCheckpoints",  "0",     CVAR_ARCHIVE,             0 },

	// Persistant speclock
	{ &cg_specLock,             "cg_specLock",             "0",     CVAR_ARCHIVE,             0 },

	// Info panel
	{ &cg_drawInfoPanel,        "cg_drawInfoPanel",        "1",     CVAR_ARCHIVE,             0 },
	{ &cg_infoPanelX,           "cg_infoPanelX",           "537",   CVAR_ARCHIVE,             0 },
	{ &cg_infoPanelY,           "cg_infoPanelY",           "2",     CVAR_ARCHIVE,             0 },

	// Country flags
	{ &cg_countryFlags,         "cg_countryFlags",         "0",     CVAR_ARCHIVE,             0 },

	// Minimum start speed
	{ &cg_minStartSpeed,        "cg_minStartSpeed",        "0",     CVAR_ARCHIVE,             0 },

	// Nico, end of ETrun cvars
};

int      cvarTableSize = sizeof (cvarTable) / sizeof (cvarTable[0]);
qboolean cvarsLoaded   = qfalse;
void CG_setClientFlags(void);

/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars(void) {
	int         i;
	cvarTable_t *cv;
	char        var[MAX_TOKEN_CHARS];

	trap_Cvar_Set("cg_letterbox", "0");   // force this for people who might have it in their

	// Nico, remove foliage and fog
	trap_Cvar_Set("r_drawfoliage", "0");
	trap_Cvar_Set("r_wolffog", "0");

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; ++i, ++cv) {
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if (cv->vmCvar != NULL) {
			// rain - force the update to range check this cvar on first run
			if (cv->vmCvar == &cg_errorDecay) {
				cv->modificationCount = !cv->vmCvar->modificationCount;
			} else {
				cv->modificationCount = cv->vmCvar->modificationCount;
			}
		}
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof (var));
	cgs.localServer = atoi(var);

	// Gordon: um, here, why?
	CG_setClientFlags();
	BG_setCrosshair(cg_crosshairColor.string, cg.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
	BG_setCrosshair(cg_crosshairColorAlt.string, cg.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt");

	cvarsLoaded = qtrue;
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars(void) {
	int         i;
	qboolean    fSetFlags = qfalse;
	cvarTable_t *cv;

	if (!cvarsLoaded) {
		return;
	}

	for (i = 0, cv = cvarTable ; i < cvarTableSize ; ++i, ++cv) {
		if (cv->vmCvar) {
			trap_Cvar_Update(cv->vmCvar);
			if (cv->modificationCount != cv->vmCvar->modificationCount) {
				cv->modificationCount = cv->vmCvar->modificationCount;

				// Check if we need to update any client flags to be sent to the server
				if (cv->vmCvar == &cg_autoAction || cv->vmCvar == &cg_autoReload ||
				    cv->vmCvar == &int_cl_timenudge || cv->vmCvar == &int_cl_maxpackets ||
				    cv->vmCvar == &cg_autoactivate || cv->vmCvar == &cg_predictItems ||
				    cv->vmCvar == &pmove_fixed || cv->vmCvar == &com_maxfps ||
				    cv->vmCvar == &cg_authToken || cv->vmCvar == &cg_autoLogin ||
				    cv->vmCvar == &cg_loadViewAngles || cv->vmCvar == &cg_autoLoad ||
				    cv->vmCvar == &cg_drawCGaz || cv->vmCvar == &cg_drawVelocitySnapping ||
				    cv->vmCvar == &cg_hideMe || cv->vmCvar == &cg_autoDemo ||
				    cv->vmCvar == &cg_autoLoadCheckpoints || cv->vmCvar == &cg_specLock ||
				    cv->vmCvar == &cg_keepAllDemos || cv->vmCvar == &cg_loadWeapon ||
				    cv->vmCvar == &cg_noclipSpeed || cv->vmCvar == &int_cl_yawspeed || 
				    cv->vmCvar == &int_cl_pitchspeed) {
					fSetFlags = qtrue;
				} else if (cv->vmCvar == &cg_crosshairColor || cv->vmCvar == &cg_crosshairAlpha) {
					BG_setCrosshair(cg_crosshairColor.string, cg.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
				} else if (cv->vmCvar == &cg_crosshairColorAlt || cv->vmCvar == &cg_crosshairAlphaAlt) {
					BG_setCrosshair(cg_crosshairColorAlt.string, cg.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt");
				} else if (cv->vmCvar == &cg_rconPassword && *cg_rconPassword.string) {
					trap_SendConsoleCommand(va("rconAuth %s", cg_rconPassword.string));
				} else if (cv->vmCvar == &cg_refereePassword && *cg_refereePassword.string) {
					trap_SendConsoleCommand(va("ref %s", cg_refereePassword.string));
				} else if (cv->vmCvar == &demo_infoWindow) {
					if (demo_infoWindow.integer == 0 && cg.demohelpWindow == SHOW_ON) {
						CG_ShowHelp_On(&cg.demohelpWindow);
					} else if (demo_infoWindow.integer > 0 && cg.demohelpWindow != SHOW_ON) {
						CG_ShowHelp_On(&cg.demohelpWindow);
					}
				} else if (cv->vmCvar == &cg_errorDecay) {
					// rain - cap errordecay because
					// prediction is EXTREMELY broken
					// right now.
					if (cg_errorDecay.value < 0.0) {
						trap_Cvar_Set("cg_errorDecay", "0");
					} else if (cg_errorDecay.value > 500.0) {
						trap_Cvar_Set("cg_errorDecay", "500");
					}
				} else if (cv->vmCvar == &cg_viewLog) {
					trap_Cvar_Set("viewlog", cg_viewLog.string);
				}
			}
		}
	}

	// Send any relevent updates
	if (fSetFlags) {
		CG_setClientFlags();
	}
}

// Nico, sha-1 hash function
// r: result
// s: source
static int CG_hash(char *r, char *s) {
	SHA1Context sha;

	SHA1Reset(&sha);
	SHA1Input(&sha, (const unsigned char *)s, strlen(s));

	if (!SHA1Result(&sha)) {
		return 1;
	}
	sprintf(r, "%08X%08X%08X%08X%08X",
	        sha.Message_Digest[0],
	        sha.Message_Digest[1],
	        sha.Message_Digest[2],
	        sha.Message_Digest[3],
	        sha.Message_Digest[4]);
	return 0;
}

void CG_setClientFlags(void) {
	char hash[MAX_QPATH] = { 0 };

	if (cg.demoPlayback) {
		return;
	}

	if (cg_authToken.string[0] == '\0') {
		Q_strncpyz(hash, "undefined", sizeof (hash));
	} else if (CG_hash(hash, cg_authToken.string)) {
		CG_Error("%s: error setting client auth token\n", GAME_VERSION);
	}

	cg.pmext.bAutoReload = (cg_autoReload.integer > 0);
	trap_Cvar_Set("cg_uinfo", va("%d %d %d %d %s %d %d %d %d %d %d %d %d %d %d %d %d %d",
	                             // Client Flags
	                             (
	                                 ((cg_autoReload.integer > 0) ? CGF_AUTORELOAD : 0) |
	                                 ((cg_autoactivate.integer > 0) ? CGF_AUTOACTIVATE : 0) |
	                                 ((cg_predictItems.integer > 0) ? CGF_PREDICTITEMS : 0) |
	                                 ((pmove_fixed.integer > 0) ? CGF_PMOVEFIXED : 0) |
	                                 ((cg_autoLogin.integer > 0) ? CGF_AUTOLOGIN : 0)
	                                 // Add more in here, as needed
	                             ),

	                             // Timenudge
	                             int_cl_timenudge.integer,

	                             // MaxPackets
	                             int_cl_maxpackets.integer,

	                             // Nico, max FPS
	                             com_maxfps.integer,

	                             // Nico, auth token
	                             hash,

	                             // Nico, load view angles on load
	                             cg_loadViewAngles.integer,

	                             // Nico, load weapon on load
	                             cg_loadWeapon.integer,

	                             // Nico, automatically load player position when he gets killed (except /kill)
	                             cg_autoLoad.integer,

	                             // Nico, cgaz
	                             cg_drawCGaz.integer,

	                             // suburb, velocity snapping
	                             cg_drawVelocitySnapping.integer,

	                             // Nico, hideme
	                             cg_hideMe.integer,

	                             // Nico, client auto demo record setting
	                             cg_autoDemo.integer,

	                             // Nico, automatically load checkpoints
	                             cg_autoLoadCheckpoints.integer,

	                             // Nico, persistant speclock
	                             cg_specLock.integer,

	                             // Nico, keep all demos
	                             cg_keepAllDemos.integer,

	                             // suburb, noclip speed scale
	                             cg_noclipSpeed.integer,

	                             // suburb, yawspeed
	                             int_cl_yawspeed.integer,

	                             // suburb, pitchspeed
	                             int_cl_pitchspeed.integer
	                             ));
}

int CG_CrosshairPlayer(void) {
	if (cg.time > (cg.crosshairClientTime + 1000)) {
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker(void) {
	return !cg.attackerTime ? -1 : cg.snap->ps.persistant[PERS_ATTACKER];
}

void QDECL CG_Printf(const char *msg, ...) {
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof (text), msg, argptr);
	va_end(argptr);
	if (!Q_strncmp(text, "[cgnotify]", 10)) {
		char buf[1024];

		if (!cg_drawNotifyText.integer) {
			Q_strncpyz(buf, &text[10], 1013);
			trap_Print(buf);
			return;
		}

		CG_AddToNotify(&text[10]);
		Q_strncpyz(buf, &text[10], 1013);
		Q_strncpyz(text, "[skipnotify]", 13);
		Q_strcat(text, 1011, buf);
	}

	trap_Print(text);
}

void QDECL CG_Error(const char *msg, ...) {
	va_list argptr;
	char    text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof (text), msg, argptr);
	va_end(argptr);

	trap_Error(text);
}

/*
================
CG_Argv
================
*/
const char *CG_Argv(int arg) {
	static char buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof (buffer));

	return buffer;
}

// Standard naming for screenshots/demos
char *CG_generateFilename(void) {
	qtime_t    ct;
	const char *pszServerInfo = CG_ConfigString(CS_SERVERINFO);

	trap_RealTime(&ct);
	return va("%d-%02d-%02d-%02d%02d%02d-%s%s",
	          1900 + ct.tm_year, ct.tm_mon + 1, ct.tm_mday,
	          ct.tm_hour, ct.tm_min, ct.tm_sec,
	          Info_ValueForKey(pszServerInfo, "mapname"),
	          "");
}

void CG_LoadObjectiveData(void) {
	pc_token_t token, token2;
	int        handle;

	handle = trap_PC_LoadSource(va("maps/%s.objdata", Q_strlwr(cgs.rawmapname)));

	if (!handle) {
		return;
	}

	for (;;) {
		if (!trap_PC_ReadToken(handle, &token)) {
			break;
		}

		if (!Q_stricmp(token.string, "wm_mapdescription")) {
			if (!trap_PC_ReadToken(handle, &token)) {
				CG_Printf("^1ERROR: bad objdata line : team parameter required\n");
				break;
			}

			if (!trap_PC_ReadToken(handle, &token2)) {
				CG_Printf("^1ERROR: bad objdata line : description parameter required\n");
				break;
			}
		} else if (!Q_stricmp(token.string, "wm_objective_axis_desc")) {
			int i;

			if (!PC_Int_Parse(handle, &i)) {
				CG_Printf("^1ERROR: bad objdata line : number parameter required\n");
				break;
			}

			if (!trap_PC_ReadToken(handle, &token)) {
				CG_Printf("^1ERROR: bad objdata line :  description parameter required\n");
				break;
			}

			i--;

			if (i < 0 || i >= MAX_OBJECTIVES) {
				CG_Printf("^1ERROR: bad objdata line : invalid objective number\n");
				break;
			}
		} else if (!Q_stricmp(token.string, "wm_objective_allied_desc")) {
			int i;

			if (!PC_Int_Parse(handle, &i)) {
				CG_Printf("^1ERROR: bad objdata line : number parameter required\n");
				break;
			}

			if (!trap_PC_ReadToken(handle, &token)) {
				CG_Printf("^1ERROR: bad objdata line :  description parameter required\n");
				break;
			}

			i--;

			if (i < 0 || i >= MAX_OBJECTIVES) {
				CG_Printf("^1ERROR: bad objdata line : invalid objective number\n");
				break;
			}
		}
	}

	trap_PC_FreeSource(handle);
}

//========================================================================
void CG_SetupDlightstyles(void) {
	int       i, j;
	char      *str;
	centity_t *cent;

	cg.lightstylesInited = qtrue;

	for (i = 1; i < MAX_DLIGHT_CONFIGSTRINGS; ++i) {
		char *token;
		int  entnum;

		str = (char *) CG_ConfigString(CS_DLIGHTS + i);
		if (!strlen(str)) {
			break;
		}

		token  = COM_Parse(&str);    // ent num
		entnum = atoi(token);
		cent   = &cg_entities[entnum];

		token = COM_Parse(&str);     // stylestring
		Q_strncpyz(cent->dl_stylestring, token, strlen(token));

		token             = COM_Parse(&str); // offset
		cent->dl_frame    = atoi(token);
		cent->dl_oldframe = cent->dl_frame - 1;
		if (cent->dl_oldframe < 0) {
			cent->dl_oldframe = strlen(cent->dl_stylestring);
		}

		token          = COM_Parse(&str); // sound id
		cent->dl_sound = atoi(token);

		token          = COM_Parse(&str); // attenuation
		cent->dl_atten = atoi(token);

		for (j = 0; j < (int)strlen(cent->dl_stylestring); ++j) {

			cent->dl_stylestring[j] += cent->dl_atten;  // adjust character for attenuation/amplification

			// clamp result
			if (cent->dl_stylestring[j] < 'a') {
				cent->dl_stylestring[j] = 'a';
			}
			if (cent->dl_stylestring[j] > 'z') {
				cent->dl_stylestring[j] = 'z';
			}
		}

		cent->dl_backlerp = 0.0;
		cent->dl_time     = cg.time;
	}
}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds(int itemNum) {
	gitem_t *item;
	char    data[MAX_QPATH];
	char    *s;

	item = &bg_itemlist[itemNum];

	if (item->pickup_sound && *item->pickup_sound) {
		trap_S_RegisterSound(item->pickup_sound);
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if (!s || !s[0]) {
		return;
	}

	while (*s) {
		char *start = s;
		int  len;

		while (*s && *s != ' ') {
			s++;
		}

		len = s - start;
		if (len >= MAX_QPATH || len < 5) {
			CG_Error("PrecacheItem: %s has bad precache string",
			         item->classname);
			return;
		}
		memcpy(data, start, len);
		data[len] = 0;
		if (*s) {
			s++;
		}

		if (!strcmp(data + len - 3, "wav")) {
			trap_S_RegisterSound(data);
		}
	}
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds(void) {
	int  i;
	char name[MAX_QPATH];

	// NERVE - SMF - voice commands
	CG_LoadVoiceChats();

	// Ridah, init sound scripts
	CG_SoundInit();
	// done.

	BG_ClearScriptSpeakerPool();

	BG_LoadSpeakerScript(va("sound/maps/%s.sps", cgs.rawmapname));

	for (i = 0; i < BG_NumScriptSpeakers(); ++i) {
		bg_speaker_t *speaker;

		speaker = BG_GetScriptSpeaker(i);

		speaker->noise = trap_S_RegisterSound(speaker->filename);
	}

	cgs.media.noAmmoSound      = trap_S_RegisterSound("sound/weapons/misc/fire_dry.wav");
	cgs.media.noFireUnderwater = trap_S_RegisterSound("sound/weapons/misc/fire_water.wav");
	cgs.media.selectSound      = trap_S_RegisterSound("sound/weapons/misc/change.wav");
	cgs.media.landHurt         = trap_S_RegisterSound("sound/player/land_hurt.wav");

	// Nico, new gib sound
	cgs.media.gibSound        = trap_S_RegisterSound("sound/teleport1.wav");
	cgs.media.dynamitebounce1 = trap_S_RegisterSound("sound/weapons/dynamite/dynamite_bounce.wav");
	cgs.media.satchelbounce1  = trap_S_RegisterSound("sound/weapons/satchel/satchel_bounce.wav");
	cgs.media.watrInSound     = trap_S_RegisterSound("sound/player/water_in.wav");
	cgs.media.watrOutSound    = trap_S_RegisterSound("sound/player/water_out.wav");
	cgs.media.watrUnSound     = trap_S_RegisterSound("sound/player/water_un.wav");
	cgs.media.watrGaspSound   = trap_S_RegisterSound("sound/player/gasp.wav");
	cgs.media.underWaterSound = trap_S_RegisterSound("sound/player/underwater.wav");

	for (i = 0; i < 2; ++i) {
		cgs.media.grenadebounce[FOOTSTEP_NORMAL][i]         = \
		    cgs.media.grenadebounce[FOOTSTEP_GRAVEL][i]     = \
		        cgs.media.grenadebounce[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound(va("sound/weapons/grenade/bounce_hard%i.wav", i + 1));

		cgs.media.grenadebounce[FOOTSTEP_METAL][i]    = \
		    cgs.media.grenadebounce[FOOTSTEP_ROOF][i] = trap_S_RegisterSound(va("sound/weapons/grenade/bounce_metal%i.wav", i + 1));

		cgs.media.grenadebounce[FOOTSTEP_WOOD][i] = trap_S_RegisterSound(va("sound/weapons/grenade/bounce_wood%i.wav", i + 1));

		cgs.media.grenadebounce[FOOTSTEP_GRASS][i]          = \
		    cgs.media.grenadebounce[FOOTSTEP_SNOW][i]       = \
		        cgs.media.grenadebounce[FOOTSTEP_CARPET][i] = trap_S_RegisterSound(va("sound/weapons/grenade/bounce_soft%i.wav", i + 1));

	}

	cgs.media.landSound[FOOTSTEP_NORMAL] = trap_S_RegisterSound("sound/player/footsteps/stone_jump.wav");
	cgs.media.landSound[FOOTSTEP_SPLASH] = trap_S_RegisterSound("sound/player/footsteps/water_jump.wav");
	cgs.media.landSound[FOOTSTEP_METAL]  = trap_S_RegisterSound("sound/player/footsteps/metal_jump.wav");
	cgs.media.landSound[FOOTSTEP_WOOD]   = trap_S_RegisterSound("sound/player/footsteps/wood_jump.wav");
	cgs.media.landSound[FOOTSTEP_GRASS]  = trap_S_RegisterSound("sound/player/footsteps/grass_jump.wav");
	cgs.media.landSound[FOOTSTEP_GRAVEL] = trap_S_RegisterSound("sound/player/footsteps/gravel_jump.wav");
	cgs.media.landSound[FOOTSTEP_ROOF]   = trap_S_RegisterSound("sound/player/footsteps/roof_jump.wav");
	cgs.media.landSound[FOOTSTEP_SNOW]   = trap_S_RegisterSound("sound/player/footsteps/snow_jump.wav");
	cgs.media.landSound[FOOTSTEP_CARPET] = trap_S_RegisterSound("sound/player/footsteps/carpet_jump.wav");

	for (i = 0; i < 4; ++i) {
		Com_sprintf(name, sizeof (name), "sound/player/footsteps/stone%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound(name);

		Com_sprintf(name, sizeof (name), "sound/player/footsteps/water%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound(name);

		Com_sprintf(name, sizeof (name), "sound/player/footsteps/metal%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound(name);

		Com_sprintf(name, sizeof (name), "sound/player/footsteps/wood%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound(name);

		Com_sprintf(name, sizeof (name), "sound/player/footsteps/grass%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRASS][i] = trap_S_RegisterSound(name);

		Com_sprintf(name, sizeof (name), "sound/player/footsteps/gravel%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRAVEL][i] = trap_S_RegisterSound(name);

		Com_sprintf(name, sizeof (name), "sound/player/footsteps/roof%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_ROOF][i] = trap_S_RegisterSound(name);

		Com_sprintf(name, sizeof (name), "sound/player/footsteps/snow%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SNOW][i] = trap_S_RegisterSound(name);

		Com_sprintf(name, sizeof (name), "sound/player/footsteps/carpet%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_CARPET][i] = trap_S_RegisterSound(name);
	}

	for (i = 1 ; i < bg_numItems ; ++i) {
		CG_RegisterItemSounds(i);
	}

	for (i = 1 ; i < MAX_SOUNDS ; ++i) {
		const char *soundName;

		soundName = CG_ConfigString(CS_SOUNDS + i);
		if (!soundName[0]) {
			break;
		}
		if (soundName[0] == '*') {
			continue;   // custom sound
		}

		// Ridah, register sound scripts seperately
		if (!strstr(soundName, ".wav")) {
			CG_SoundScriptPrecache(soundName);
		} else {
			cgs.gameSounds[i] = trap_S_RegisterSound(soundName);    // FIXME: allow option to compress?
		}
	}

	cgs.media.flameSound         = trap_S_RegisterSound("sound/weapons/flamethrower/flame_burn.wav");
	cgs.media.flameBlowSound     = trap_S_RegisterSound("sound/weapons/flamethrower/flame_pilot.wav");
	cgs.media.flameStartSound    = trap_S_RegisterSound("sound/weapons/flamethrower/flame_up.wav");
	cgs.media.flameStreamSound   = trap_S_RegisterSound("sound/weapons/flamethrower/flame_fire.wav");
	cgs.media.flameCrackSound    = 0;
	cgs.media.grenadePulseSound4 = trap_S_RegisterSound("sound/weapons/grenade/gren_timer4.wav");
	cgs.media.grenadePulseSound3 = trap_S_RegisterSound("sound/weapons/grenade/gren_timer3.wav");
	cgs.media.grenadePulseSound2 = trap_S_RegisterSound("sound/weapons/grenade/gren_timer2.wav");
	cgs.media.grenadePulseSound1 = trap_S_RegisterSound("sound/weapons/grenade/gren_timer1.wav");

	cgs.media.boneBounceSound = trap_S_RegisterSound("sound/world/boardbreak.wav");          // TODO: need a real sound for this

	cgs.media.sfx_rockexp     = trap_S_RegisterSound("sound/weapons/rocket/rocket_expl.wav");
	cgs.media.sfx_rockexpDist = trap_S_RegisterSound("sound/weapons/rocket/rocket_expl_far.wav");

	cgs.media.sfx_dynamiteexp     = trap_S_RegisterSound("sound/weapons/dynamite/dynamite_expl.wav");
	cgs.media.sfx_dynamiteexpDist = trap_S_RegisterSound("sound/weapons/dynamite/dynamite_expl_far.wav");

	cgs.media.sfx_satchelexp     = trap_S_RegisterSound("sound/weapons/satchel/satchel_expl.wav");
	cgs.media.sfx_satchelexpDist = trap_S_RegisterSound("sound/weapons/satchel/satchel_expl_far.wav");
	cgs.media.sfx_mortarexp[0]   = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl1.wav");
	cgs.media.sfx_mortarexp[1]   = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl2.wav");
	cgs.media.sfx_mortarexp[2]   = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl3.wav");
	cgs.media.sfx_mortarexp[3]   = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl.wav");
	cgs.media.sfx_mortarexpDist  = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl_far.wav");
	cgs.media.sfx_grenexp        = trap_S_RegisterSound("sound/weapons/grenade/gren_expl.wav");
	cgs.media.sfx_grenexpDist    = trap_S_RegisterSound("sound/weapons/grenade/gren_expl_far.wav");
	cgs.media.sfx_rockexpWater   = trap_S_RegisterSound("sound/weapons/grenade/gren_expl_water.wav");

	for (i = 0; i < 3; ++i) {
		// Gordon: bouncy shell sounds \o/
		cgs.media.sfx_brassSound[BRASSSOUND_METAL][i] = trap_S_RegisterSound(va("sound/weapons/misc/shell_metal%i.wav", i + 1));
		cgs.media.sfx_brassSound[BRASSSOUND_SOFT][i]  = trap_S_RegisterSound(va("sound/weapons/misc/shell_soft%i.wav", i + 1));
		cgs.media.sfx_brassSound[BRASSSOUND_STONE][i] = trap_S_RegisterSound(va("sound/weapons/misc/shell_stone%i.wav", i + 1));
		cgs.media.sfx_brassSound[BRASSSOUND_WOOD][i]  = trap_S_RegisterSound(va("sound/weapons/misc/shell_wood%i.wav", i + 1));
		cgs.media.sfx_rubbleBounce[i]                 = trap_S_RegisterSound(va("sound/world/debris%i.wav", i + 1));
	}
	cgs.media.sfx_knifehit[0] = trap_S_RegisterSound("sound/weapons/knife/knife_hit1.wav");
	cgs.media.sfx_knifehit[1] = trap_S_RegisterSound("sound/weapons/knife/knife_hit2.wav");
	cgs.media.sfx_knifehit[2] = trap_S_RegisterSound("sound/weapons/knife/knife_hit3.wav");
	cgs.media.sfx_knifehit[3] = trap_S_RegisterSound("sound/weapons/knife/knife_hit4.wav");
	cgs.media.sfx_knifehit[4] = trap_S_RegisterSound("sound/weapons/knife/knife_hitwall1.wav");

	for (i = 0; i < 5; ++i) {
		cgs.media.sfx_bullet_metalhit[i] = trap_S_RegisterSound(va("sound/weapons/impact/metal%i.wav", i + 1));
		cgs.media.sfx_bullet_woodhit[i]  = trap_S_RegisterSound(va("sound/weapons/impact/wood%i.wav", i + 1));
		cgs.media.sfx_bullet_glasshit[i] = trap_S_RegisterSound(va("sound/weapons/impact/glass%i.wav", i + 1));
		cgs.media.sfx_bullet_stonehit[i] = trap_S_RegisterSound(va("sound/weapons/impact/stone%i.wav", i + 1));
		cgs.media.sfx_bullet_waterhit[i] = trap_S_RegisterSound(va("sound/weapons/impact/water%i.wav", i + 1));
	}

	cgs.media.buildDecayedSound = trap_S_RegisterSound("sound/world/build_abort.wav");

	cgs.media.sndLimboSelect = trap_S_RegisterSound("sound/menu/select.wav");
	cgs.media.sndLimboFocus  = trap_S_RegisterSound("sound/menu/focus.wav");
	cgs.media.sndLimboCancel = trap_S_RegisterSound("sound/menu/cancel.wav");

	if (cg_buildScript.integer) {
		CG_PrecacheFXSounds();
	}
}

//===================================================================================

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/
static void CG_RegisterGraphics(void) {
	char        name[1024];
	int         i;
	static char *sb_nums[11] =
	{
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	CG_LoadingString(cgs.mapname);

	trap_R_LoadWorldMap(cgs.mapname);

	CG_LoadingString("entities");

	numSplinePaths = 0;
	numPathCorners = 0;

	BG_ClearAnimationPool();

	BG_ClearCharacterPool();

	BG_InitWeaponStrings();

	CG_ParseEntitiesFromString();

	CG_LoadObjectiveData();

	// precache status bar pics
	CG_LoadingString("game media");

	CG_LoadingString(" - textures");

	for (i = 0 ; i < 11 ; ++i) {
		cgs.media.numberShaders[i] = trap_R_RegisterShader(sb_nums[i]);
	}

	cgs.media.nerveTestShader = trap_R_RegisterShader("jpwtest1");
	cgs.media.idTestShader    = trap_R_RegisterShader("jpwtest2");
	cgs.media.smokePuffShader = trap_R_RegisterShader("smokePuff");

	// Rafael - cannon
	cgs.media.smokePuffShaderdirty = trap_R_RegisterShader("smokePuffdirty");
	cgs.media.smokePuffShaderb1    = trap_R_RegisterShader("smokePuffblack1");
	cgs.media.smokePuffShaderb2    = trap_R_RegisterShader("smokePuffblack2");
	cgs.media.smokePuffShaderb3    = trap_R_RegisterShader("smokePuffblack3");
	cgs.media.smokePuffShaderb4    = trap_R_RegisterShader("smokePuffblack4");
	cgs.media.smokePuffShaderb5    = trap_R_RegisterShader("smokePuffblack5");
	// done

	for (i = 0; i < 16; ++i) {
		cgs.media.viewFlashFire[i] = trap_R_RegisterShader(va("viewFlashFire%i", i + 1));
	}

	cgs.media.smokePuffRageProShader = trap_R_RegisterShader("smokePuffRagePro");
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader("shotgunSmokePuff");
	cgs.media.lagometerShader        = trap_R_RegisterShader("lagometer");
	cgs.media.reticleShaderSimple    = trap_R_RegisterShader("gfx/misc/reticlesimple");
	cgs.media.binocShaderSimple      = trap_R_RegisterShader("gfx/misc/binocsimple");
	cgs.media.snowShader             = trap_R_RegisterShader("snow_tri");
	cgs.media.oilParticle            = trap_R_RegisterShader("oilParticle");
	cgs.media.oilSlick               = trap_R_RegisterShader("oilSlick");
	cgs.media.waterBubbleShader      = trap_R_RegisterShader("waterBubble");
	cgs.media.tracerShader           = trap_R_RegisterShader("gfx/misc/tracer");
	cgs.media.usableHintShader       = trap_R_RegisterShader("gfx/2d/usableHint");
	cgs.media.notUsableHintShader    = trap_R_RegisterShader("gfx/2d/notUsableHint");
	cgs.media.doorHintShader         = trap_R_RegisterShader("gfx/2d/doorHint");
	cgs.media.doorRotateHintShader   = trap_R_RegisterShader("gfx/2d/doorRotateHint");

	// Arnout: these were never used in default wolf
	cgs.media.doorLockHintShader       = trap_R_RegisterShader("gfx/2d/lockedhint");
	cgs.media.doorRotateLockHintShader = trap_R_RegisterShader("gfx/2d/lockedhint");
	cgs.media.mg42HintShader           = trap_R_RegisterShader("gfx/2d/mg42Hint");
	cgs.media.breakableHintShader      = trap_R_RegisterShader("gfx/2d/breakableHint");
	cgs.media.chairHintShader          = trap_R_RegisterShader("gfx/2d/chairHint");
	cgs.media.alarmHintShader          = trap_R_RegisterShader("gfx/2d/alarmHint");
	cgs.media.healthHintShader         = trap_R_RegisterShader("gfx/2d/healthHint");
	cgs.media.treasureHintShader       = trap_R_RegisterShader("gfx/2d/treasureHint");
	cgs.media.knifeHintShader          = trap_R_RegisterShader("gfx/2d/knifeHint");
	cgs.media.ladderHintShader         = trap_R_RegisterShader("gfx/2d/ladderHint");
	cgs.media.buttonHintShader         = trap_R_RegisterShader("gfx/2d/buttonHint");
	cgs.media.waterHintShader          = trap_R_RegisterShader("gfx/2d/waterHint");
	cgs.media.cautionHintShader        = trap_R_RegisterShader("gfx/2d/cautionHint");
	cgs.media.dangerHintShader         = trap_R_RegisterShader("gfx/2d/dangerHint");
	cgs.media.secretHintShader         = trap_R_RegisterShader("gfx/2d/secretHint");
	cgs.media.qeustionHintShader       = trap_R_RegisterShader("gfx/2d/questionHint");
	cgs.media.exclamationHintShader    = trap_R_RegisterShader("gfx/2d/exclamationHint");
	cgs.media.clipboardHintShader      = trap_R_RegisterShader("gfx/2d/clipboardHint");
	cgs.media.weaponHintShader         = trap_R_RegisterShader("gfx/2d/weaponHint");
	cgs.media.ammoHintShader           = trap_R_RegisterShader("gfx/2d/ammoHint");
	cgs.media.armorHintShader          = trap_R_RegisterShader("gfx/2d/armorHint");
	cgs.media.powerupHintShader        = trap_R_RegisterShader("gfx/2d/powerupHint");
	cgs.media.holdableHintShader       = trap_R_RegisterShader("gfx/2d/holdableHint");
	cgs.media.inventoryHintShader      = trap_R_RegisterShader("gfx/2d/inventoryHint");

	cgs.media.buildHintShader    = trap_R_RegisterShader("gfx/2d/buildHint");            // DHM - Nerve
	cgs.media.disarmHintShader   = trap_R_RegisterShader("gfx/2d/disarmHint");           // DHM - Nerve
	cgs.media.dynamiteHintShader = trap_R_RegisterShader("gfx/2d/dynamiteHint");         // DHM - Nerve

	cgs.media.tankHintShader          = trap_R_RegisterShaderNoMip("gfx/2d/tankHint");
	cgs.media.satchelchargeHintShader = trap_R_RegisterShaderNoMip("gfx/2d/satchelchargeHint"),

	cgs.media.uniformHintShader = trap_R_RegisterShaderNoMip("gfx/2d/uniformHint");

	for (i = 0 ; i < NUM_CROSSHAIRS ; ++i) {
		cgs.media.crosshairShader[i] = trap_R_RegisterShader(va("gfx/2d/crosshair%c", 'a' + i));
		cg.crosshairShaderAlt[i]     = trap_R_RegisterShader(va("gfx/2d/crosshair%c_alt", 'a' + i));
	}

	cgs.media.backTileShader = trap_R_RegisterShader("gfx/2d/backtile");

	cgs.media.teamStatusBar = trap_R_RegisterShader("gfx/2d/colorbar.tga");

	cgs.media.hudSprintBar = trap_R_RegisterShader("sprintbar");

	CG_LoadingString(" - models");

	cgs.media.machinegunBrassModel  = trap_R_RegisterModel("models/weapons2/shells/m_shell.md3");
	cgs.media.panzerfaustBrassModel = trap_R_RegisterModel("models/weapons2/shells/pf_shell.md3");

	// Rafael
	cgs.media.smallgunBrassModel = trap_R_RegisterModel("models/weapons2/shells/sm_shell.md3");

	//----(SA) wolf debris
	cgs.media.debBlock[0] = trap_R_RegisterModel("models/mapobjects/debris/brick1.md3");
	cgs.media.debBlock[1] = trap_R_RegisterModel("models/mapobjects/debris/brick2.md3");
	cgs.media.debBlock[2] = trap_R_RegisterModel("models/mapobjects/debris/brick3.md3");
	cgs.media.debBlock[3] = trap_R_RegisterModel("models/mapobjects/debris/brick4.md3");
	cgs.media.debBlock[4] = trap_R_RegisterModel("models/mapobjects/debris/brick5.md3");
	cgs.media.debBlock[5] = trap_R_RegisterModel("models/mapobjects/debris/brick6.md3");

	cgs.media.debRock[0] = trap_R_RegisterModel("models/mapobjects/debris/rubble1.md3");
	cgs.media.debRock[1] = trap_R_RegisterModel("models/mapobjects/debris/rubble2.md3");
	cgs.media.debRock[2] = trap_R_RegisterModel("models/mapobjects/debris/rubble3.md3");

	cgs.media.debWood[0] = trap_R_RegisterModel("models/gibs/wood/wood1.md3");
	cgs.media.debWood[1] = trap_R_RegisterModel("models/gibs/wood/wood2.md3");
	cgs.media.debWood[2] = trap_R_RegisterModel("models/gibs/wood/wood3.md3");
	cgs.media.debWood[3] = trap_R_RegisterModel("models/gibs/wood/wood4.md3");
	cgs.media.debWood[4] = trap_R_RegisterModel("models/gibs/wood/wood5.md3");
	cgs.media.debWood[5] = trap_R_RegisterModel("models/gibs/wood/wood6.md3");

	cgs.media.debFabric[0] = trap_R_RegisterModel("models/shards/fabric1.md3");
	cgs.media.debFabric[1] = trap_R_RegisterModel("models/shards/fabric2.md3");
	cgs.media.debFabric[2] = trap_R_RegisterModel("models/shards/fabric3.md3");

	//----(SA) end

	cgs.media.spawnInvincibleShader = trap_R_RegisterShader("sprites/shield");

	cgs.media.voiceChatShader = trap_R_RegisterShader("sprites/voiceChat");
	cgs.media.balloonShader   = trap_R_RegisterShader("sprites/balloon3");

	cgs.media.objectiveShader = trap_R_RegisterShader("sprites/objective");

	//----(SA)	water splash
	cgs.media.waterSplashModel  = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.waterSplashShader = trap_R_RegisterShader("waterSplash");
	//----(SA)	end

	// Ridah, spark particles
	cgs.media.sparkParticleShader    = trap_R_RegisterShader("sparkParticle");
	cgs.media.smokeTrailShader       = trap_R_RegisterShader("smokeTrail");
	cgs.media.flamethrowerFireStream = trap_R_RegisterShader("flamethrowerFireStream");
	cgs.media.onFireShader2          = trap_R_RegisterShader("entityOnFire1");
	cgs.media.onFireShader           = trap_R_RegisterShader("entityOnFire2");
	cgs.media.sparkFlareShader       = trap_R_RegisterShader("sparkFlareParticle");
	cgs.media.spotLightShader        = trap_R_RegisterShader("spotLight");
	cgs.media.spotLightBeamShader    = trap_R_RegisterShader("lightBeam");
	cgs.media.smokeParticleShader    = trap_R_RegisterShader("smokeParticle");

	// DHM - Nerve :: bullet hitting dirt
	cgs.media.dirtParticle1Shader = trap_R_RegisterShader("dirt_splash");
	cgs.media.dirtParticle2Shader = trap_R_RegisterShader("water_splash");

	cgs.media.genericConstructionShader = trap_R_RegisterShader("textures/sfx/construction");

	// Gordon: limbo menu setup
	CG_LimboPanel_Init();

	CG_Fireteams_Setup();

	cgs.media.railCoreShader = trap_R_RegisterShaderNoMip("railCore");       // (SA) for debugging server traces
	cgs.media.ropeShader     = trap_R_RegisterShader("textures/props/cable_m01");

	cgs.media.thirdPersonBinocModel = trap_R_RegisterModel("models/multiplayer/binocs/binocs.md3");                  // NERVE - SMF
	cgs.media.flamebarrel           = trap_R_RegisterModel("models/furniture/barrel/barrel_a.md3");
	cgs.media.mg42muzzleflash       = trap_R_RegisterModel("models/weapons2/machinegun/mg42_flash.md3");

	// Rafael shards
	cgs.media.shardGlass1 = trap_R_RegisterModel("models/shards/glass1.md3");
	cgs.media.shardGlass2 = trap_R_RegisterModel("models/shards/glass2.md3");
	cgs.media.shardWood1  = trap_R_RegisterModel("models/shards/wood1.md3");
	cgs.media.shardWood2  = trap_R_RegisterModel("models/shards/wood2.md3");
	cgs.media.shardMetal1 = trap_R_RegisterModel("models/shards/metal1.md3");
	cgs.media.shardMetal2 = trap_R_RegisterModel("models/shards/metal2.md3");
	// done

	cgs.media.shardRubble1 = trap_R_RegisterModel("models/mapobjects/debris/brick000.md3");
	cgs.media.shardRubble2 = trap_R_RegisterModel("models/mapobjects/debris/brick001.md3");
	cgs.media.shardRubble3 = trap_R_RegisterModel("models/mapobjects/debris/brick002.md3");

	for (i = 0; i < MAX_LOCKER_DEBRIS; ++i) {
		Com_sprintf(name, sizeof (name), "models/mapobjects/debris/personal%i.md3", i + 1);
		cgs.media.shardJunk[i] = trap_R_RegisterModel(name);
	}

	memset(cg_items, 0, sizeof (cg_items));
	memset(cg_weapons, 0, sizeof (cg_weapons));

// TODO: FIXME:  REMOVE REGISTRATION OF EACH MODEL FOR EVERY LEVEL LOAD

	//----(SA)	okay, new stuff to intialize rather than doing it at level load time (or "give all" time)
	//			(I'm certainly not against being efficient here, but I'm tired of the rocket launcher effect only registering
	//			sometimes and want it to work for sure for this demo)

	CG_LoadingString(" - weapons");
	for (i = WP_KNIFE; i < WP_NUM_WEAPONS; ++i) {
		CG_RegisterWeapon(i, qfalse);
	}

	CG_LoadingString(" - items");
	for (i = 1 ; i < bg_numItems ; ++i) {
		CG_RegisterItemVisuals(i);
	}

	cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");

	cgs.media.hWeaponSnd     = trap_S_RegisterSound("sound/weapons/mg42/mg42_fire.wav");
	cgs.media.hWeaponEchoSnd = trap_S_RegisterSound("sound/weapons/mg42/mg42_far.wav");
	cgs.media.hWeaponHeatSnd = trap_S_RegisterSound("sound/weapons/mg42/mg42_heat.wav");

	cgs.media.hWeaponSnd_2     = trap_S_RegisterSound("sound/weapons/browning/browning_fire.wav");
	cgs.media.hWeaponEchoSnd_2 = trap_S_RegisterSound("sound/weapons/browning/browning_far.wav");
	cgs.media.hWeaponHeatSnd_2 = trap_S_RegisterSound("sound/weapons/browning/browning_heat.wav");

	cgs.media.minePrimedSound = trap_S_RegisterSound("sound/weapons/landmine/mine_on.wav");

	// wall marks
	cgs.media.bulletMarkShader   = trap_R_RegisterShaderNoMip("gfx/damage/bullet_mrk");
	cgs.media.burnMarkShader     = trap_R_RegisterShaderNoMip("gfx/damage/burn_med_mrk");
	cgs.media.shadowFootShader   = trap_R_RegisterShaderNoMip("markShadowFoot");
	cgs.media.shadowTorsoShader  = trap_R_RegisterShaderNoMip("markShadowTorso");
	cgs.media.wakeMarkShader     = trap_R_RegisterShaderNoMip("wake");
	cgs.media.wakeMarkShaderAnim = trap_R_RegisterShaderNoMip("wakeAnim");    // (SA)

	//----(SA)	added
	cgs.media.bulletMarkShaderMetal = trap_R_RegisterShaderNoMip("gfx/damage/metal_mrk");
	cgs.media.bulletMarkShaderWood  = trap_R_RegisterShaderNoMip("gfx/damage/wood_mrk");
	cgs.media.bulletMarkShaderGlass = trap_R_RegisterShaderNoMip("gfx/damage/glass_mrk");

	CG_LoadingString(" - inline models");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	// TAT 12/23/2002 - as a safety check, let's not let the number of models exceed MAX_MODELS
	if (cgs.numInlineModels > MAX_MODELS) {
		CG_Error("CG_RegisterGraphics: Too many inline models: %i\n", cgs.numInlineModels);
	}

	for (i = 1 ; i < cgs.numInlineModels ; ++i) {
		char   name[10];
		vec3_t mins, maxs;
		int    j;

		Com_sprintf(name, sizeof (name), "*%i", i);
		cgs.inlineDrawModel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for (j = 0 ; j < 3 ; ++j) {
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
	}

	CG_LoadingString(" - server models");

	// register all the server specified models
	for (i = 1 ; i < MAX_MODELS ; ++i) {
		const char *modelName;

		modelName = CG_ConfigString(CS_MODELS + i);
		if (!modelName[0]) {
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel(modelName);
	}

	for (i = 1 ; i < MAX_MODELS ; ++i) {
		const char *skinName;

		skinName = CG_ConfigString(CS_SKINS + i);
		if (!skinName[0]) {
			break;
		}
		cgs.gameModelSkins[i] = trap_R_RegisterSkin(skinName);
	}

	for (i = 1 ; i < MAX_CS_SHADERS ; ++i) {
		const char *shaderName;

		shaderName = CG_ConfigString(CS_SHADERS + i);
		if (!shaderName[0]) {
			break;
		}
		cgs.gameShaders[i] = shaderName[0] == '*' ? trap_R_RegisterShader(shaderName + 1) : trap_R_RegisterShaderNoMip(shaderName);
		Q_strncpyz(cgs.gameShaderNames[i], shaderName[0] == '*' ? shaderName + 1 : shaderName, MAX_QPATH);
	}

	for (i = 1 ; i < MAX_CHARACTERS ; ++i) {
		const char *characterName;

		characterName = CG_ConfigString(CS_CHARACTERS + i);
		if (!characterName[0]) {
			break;
		}

		if (!BG_FindCharacter(characterName)) {
			cgs.gameCharacters[i] = BG_FindFreeCharacter(characterName);

			Q_strncpyz(cgs.gameCharacters[i]->characterFile, characterName, sizeof (cgs.gameCharacters[i]->characterFile));

			if (!CG_RegisterCharacter(characterName, cgs.gameCharacters[i])) {
				CG_Error("ERROR: CG_RegisterGraphics: failed to load character file '%s'\n", characterName);
			}
		}
	}

	CG_LoadingString(" - particles");
	CG_ClearParticles();

	InitSmokeSprites();

	CG_LoadingString(" - classes");

	CG_RegisterPlayerClasses();

	CG_InitPMGraphics();

	// mounted gun on tank models
	cgs.media.hMountedMG42Base = trap_R_RegisterModel("models/mapobjects/tanks_sd/mg42nestbase.md3");
	cgs.media.hMountedMG42Nest = trap_R_RegisterModel("models/mapobjects/tanks_sd/mg42nest.md3");
	cgs.media.hMountedMG42     = trap_R_RegisterModel("models/mapobjects/tanks_sd/mg42.md3");
	cgs.media.hMountedBrowning = trap_R_RegisterModel("models/multiplayer/browning/thirdperson.md3");

	// FIXME: temp models
	cgs.media.hMountedFPMG42     = trap_R_RegisterModel("models/multiplayer/mg42/v_mg42.md3");
	cgs.media.hMountedFPBrowning = trap_R_RegisterModel("models/multiplayer/browning/tankmounted.md3");

	trap_R_RegisterFont("ariblk", 27, &cgs.media.limboFont1);
	trap_R_RegisterFont("ariblk", 16, &cgs.media.limboFont1_lo);
	trap_R_RegisterFont("courbd", 30, &cgs.media.limboFont2);

	cgs.media.limboNumber_roll = trap_R_RegisterShaderNoMip("gfx/limbo/number_roll");
	cgs.media.limboNumber_back = trap_R_RegisterShaderNoMip("gfx/limbo/number_back");
	cgs.media.limboStar_roll   = trap_R_RegisterShaderNoMip("gfx/limbo/skill_roll");
	cgs.media.limboStar_back   = trap_R_RegisterShaderNoMip("gfx/limbo/skill_back");
	cgs.media.limboLight_on    = trap_R_RegisterShaderNoMip("gfx/limbo/redlight_on");
	cgs.media.limboLight_on2   = trap_R_RegisterShaderNoMip("gfx/limbo/redlight_on02");
	cgs.media.limboLight_off   = trap_R_RegisterShaderNoMip("gfx/limbo/redlight_off");

	cgs.media.limboWeaponNumber_off = trap_R_RegisterShaderNoMip("gfx/limbo/but_weap_off");
	cgs.media.limboWeaponNumber_on  = trap_R_RegisterShaderNoMip("gfx/limbo/but_weap_on");
	cgs.media.limboWeaponCard       = trap_R_RegisterShaderNoMip("gfx/limbo/weap_card");

	cgs.media.limboWeaponCardSurroundH = trap_R_RegisterShaderNoMip("gfx/limbo/butsur_hor");
	cgs.media.limboWeaponCardSurroundV = trap_R_RegisterShaderNoMip("gfx/limbo/butsur_vert");
	cgs.media.limboWeaponCardSurroundC = trap_R_RegisterShaderNoMip("gfx/limbo/butsur_corn");

	cgs.media.limboWeaponCardOOS = trap_R_RegisterShaderNoMip("gfx/limbo/outofstock");

	cgs.media.limboClassButtons[PC_ENGINEER]  = trap_R_RegisterShaderNoMip("gfx/limbo/ic_engineer");
	cgs.media.limboClassButtons[PC_SOLDIER]   = trap_R_RegisterShaderNoMip("gfx/limbo/ic_soldier");
	cgs.media.limboClassButtons[PC_COVERTOPS] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_covertops");
	cgs.media.limboClassButtons[PC_FIELDOPS]  = trap_R_RegisterShaderNoMip("gfx/limbo/ic_fieldops");
	cgs.media.limboClassButtons[PC_MEDIC]     = trap_R_RegisterShaderNoMip("gfx/limbo/ic_medic");

	cgs.media.limboClassButton2Back_on         = trap_R_RegisterShaderNoMip("gfx/limbo/skill_back_on");
	cgs.media.limboClassButton2Back_off        = trap_R_RegisterShaderNoMip("gfx/limbo/skill_back_off");
	cgs.media.limboClassButton2Wedge_off       = trap_R_RegisterShaderNoMip("gfx/limbo/skill_4pieces_off");
	cgs.media.limboClassButton2Wedge_on        = trap_R_RegisterShaderNoMip("gfx/limbo/skill_4pieces_on");
	cgs.media.limboClassButtons2[PC_ENGINEER]  = trap_R_RegisterShaderNoMip("gfx/limbo/skill_engineer");
	cgs.media.limboClassButtons2[PC_SOLDIER]   = trap_R_RegisterShaderNoMip("gfx/limbo/skill_soldier");
	cgs.media.limboClassButtons2[PC_COVERTOPS] = trap_R_RegisterShaderNoMip("gfx/limbo/skill_covops");
	cgs.media.limboClassButtons2[PC_FIELDOPS]  = trap_R_RegisterShaderNoMip("gfx/limbo/skill_fieldops");
	cgs.media.limboClassButtons2[PC_MEDIC]     = trap_R_RegisterShaderNoMip("gfx/limbo/skill_medic");

	cgs.media.limboTeamButtonBack_on  = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_on");
	cgs.media.limboTeamButtonBack_off = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_off");
	cgs.media.limboTeamButtonAllies   = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_allied");
	cgs.media.limboTeamButtonAxis     = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_axis");
	cgs.media.limboTeamButtonSpec     = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_spec");

	cgs.media.limboWeaponBlendThingy = trap_R_RegisterShaderNoMip("gfx/limbo/weap_blend");

	cgs.media.limboCounterBorder = trap_R_RegisterShaderNoMip("gfx/limbo/number_border");

	cgs.media.hudPowerIcon  = trap_R_RegisterShaderNoMip("gfx/hud/ic_power");
	cgs.media.hudHealthIcon = trap_R_RegisterShaderNoMip("gfx/hud/ic_health");

	cgs.media.limboWeaponCard1     = trap_R_RegisterShaderNoMip("gfx/limbo/weaponcard01");
	cgs.media.limboWeaponCard2     = trap_R_RegisterShaderNoMip("gfx/limbo/weaponcard02");
	cgs.media.limboWeaponCardArrow = trap_R_RegisterShaderNoMip("gfx/limbo/weap_dnarrow.tga");

	cgs.media.limboObjectiveBack[0] = trap_R_RegisterShaderNoMip("gfx/limbo/objective_back_axis");
	cgs.media.limboObjectiveBack[1] = trap_R_RegisterShaderNoMip("gfx/limbo/objective_back_allied");
	cgs.media.limboObjectiveBack[2] = trap_R_RegisterShaderNoMip("gfx/limbo/objective_back");

	cgs.media.limboClassBar = trap_R_RegisterShaderNoMip("gfx/limbo/lightup_bar");

	cgs.media.cursorIcon = trap_R_RegisterShaderNoMip("ui/assets/3_cursor3");

	cgs.media.hudDamagedStates[0] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood01.skin");
	cgs.media.hudDamagedStates[1] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood02.skin");
	cgs.media.hudDamagedStates[2] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood03.skin");
	cgs.media.hudDamagedStates[3] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood04.skin");

	cgs.media.browningIcon = trap_R_RegisterShaderNoMip("icons/iconw_browning_1_select");

	cgs.media.disconnectIcon = trap_R_RegisterShaderNoMip("gfx/2d/net");

	for (i = 0; i < 6; ++i) {
		cgs.media.fireteamicons[i] = trap_R_RegisterShaderNoMip(va("gfx/hud/fireteam/fireteam%i", i + 1));
	}

	// Nico, load keysets
	for (i = 0; i < NUM_KEYS_SETS; ++i) {
		cgs.media.keys[i].ForwardPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_forward_pressed", i + 1));
		cgs.media.keys[i].ForwardNotPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_forward_not_pressed", i + 1));
		cgs.media.keys[i].BackwardPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_backward_pressed", i + 1));
		cgs.media.keys[i].BackwardNotPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_backward_not_pressed", i + 1));
		cgs.media.keys[i].RightPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_right_pressed", i + 1));
		cgs.media.keys[i].RightNotPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_right_not_pressed", i + 1));
		cgs.media.keys[i].LeftPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_left_pressed", i + 1));
		cgs.media.keys[i].LeftNotPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_left_not_pressed", i + 1));
		cgs.media.keys[i].JumpPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_jump_pressed", i + 1));
		cgs.media.keys[i].JumpNotPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_jump_not_pressed", i + 1));
		cgs.media.keys[i].CrouchPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_crouch_pressed", i + 1));
		cgs.media.keys[i].CrouchNotPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_crouch_not_pressed", i + 1));
		cgs.media.keys[i].SprintPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_sprint_pressed", i + 1));
		cgs.media.keys[i].SprintNotPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_sprint_not_pressed", i + 1));
		cgs.media.keys[i].PronePressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_prone_pressed", i + 1));
		cgs.media.keys[i].ProneNotPressedShader
		    = trap_R_RegisterShaderNoMip(va("gfx/2d/keyset%d/key_prone_not_pressed", i + 1));
	}

	// Nico, load CGaz arrow
	cgs.media.CGazArrow = trap_R_RegisterShaderNoMip("gfx/2d/cgaz_arrow");

	// Nico, ETrun logo
	cgs.media.modLogo = trap_R_RegisterShaderNoMip("gfx/2d/ETrun_logo_256");

	// Nico, world flags for GeoIP
	cgs.media.worldFlags = trap_R_RegisterShaderNoMip("gfx/2d/ETrun_world_flags");

	CG_LoadingString(" - game media done");
}

/*
===================
CG_RegisterClients

===================
*/
static void CG_RegisterClients(void) {
	int i;

	for (i = 0 ; i < MAX_CLIENTS ; ++i) {
		const char *clientInfo;

		clientInfo = CG_ConfigString(CS_PLAYERS + i);
		if (!clientInfo[0]) {
			continue;
		}
		CG_NewClientInfo(i);
	}
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/

const char *CG_ConfigString(int index) {
	if (index < 0 || index >= MAX_CONFIGSTRINGS) {
		CG_Error("CG_ConfigString: bad index: %i", index);
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

int CG_ConfigStringCopy(int index, char *buff, int buffsize) {
	Q_strncpyz(buff, CG_ConfigString(index), buffsize);
	return strlen(buff);
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic(void) {
	char *s;
	char parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString(CS_MUSIC);
	Q_strncpyz(parm1, COM_Parse(&s), sizeof (parm1));
	Q_strncpyz(parm2, COM_Parse(&s), sizeof (parm2));

	if (strlen(parm1)) {
		trap_S_StartBackgroundTrack(parm1, parm2, 0);
	}
}

/*
==============
CG_QueueMusic
==============
*/
void CG_QueueMusic(void) {
	char *s;
	char parm[MAX_QPATH];

	// prepare the next background track
	s = (char *)CG_ConfigString(CS_MUSIC_QUEUE);
	Q_strncpyz(parm, COM_Parse(&s), sizeof (parm));

	// even if no strlen(parm).  we want to be able to clear the queue

	// TODO: \/		the values stored in here will be made accessable so
	//				it doesn't have to go through startbackgroundtrack() (which is stupid)
	trap_S_StartBackgroundTrack(parm, "", -2);    // '-2' for 'queue looping track' (QUEUED_PLAY_LOOPED)
}

static int CG_FeederCount(float feederID) {
	int i, count;

	count = 0;
	if (feederID == FEEDER_REDTEAM_LIST) {
		for (i = 0; i < cg.numScores; ++i) {
			if (cg.scores[i].team == TEAM_AXIS) {
				count++;
			}
		}
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		for (i = 0; i < cg.numScores; ++i) {
			if (cg.scores[i].team == TEAM_ALLIES) {
				count++;
			}
		}
	} else if (feederID == FEEDER_SCOREBOARD) {
		return cg.numScores;
	}
	return count;
}

///////////////////////////
///////////////////////////

static clientInfo_t *CG_InfoFromScoreIndex(int index, int team, int *scoreIndex) {
	int i, count;

	count = 0;
	for (i = 0; i < cg.numScores; ++i) {
		if (cg.scores[i].team == team) {
			if (count == index) {
				*scoreIndex = i;
				return &cgs.clientinfo[cg.scores[i].client];
			}
			count++;
		}
	}

	*scoreIndex = index;
	return &cgs.clientinfo[cg.scores[index].client];
}

static const char *CG_FeederItemText(float feederID, int index, int column, qhandle_t *handle, int *numhandles) {
	int          scoreIndex = 0;
	clientInfo_t *info      = NULL;
	int          team       = -1;
	score_t      *sp        = NULL;

	// Nico, silent GCC
	(void)numhandles;

	*handle = -1;

	if (feederID == FEEDER_REDTEAM_LIST) {
		team = TEAM_AXIS;
	} else if (feederID == FEEDER_BLUETEAM_LIST) {
		team = TEAM_ALLIES;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp   = &cg.scores[scoreIndex];

	if (info && info->infoValid) {
		switch (column) {
		case 0:
			break;
		case 3:
			return info->name;
		case 4:
			return va("%i", info->score);
		case 5:
			return va("%4i", sp->time);
		case 6:
			if (sp->ping == -1) {
				return "connecting";
			}
			return va("%4i", sp->ping);
		}
	}

	return "";
}

static void CG_FeederSelection(float feederID, int index) {
	int i, count;
	int team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_AXIS : TEAM_ALLIES;

	count = 0;
	for (i = 0; i < cg.numScores; ++i) {
		if (cg.scores[i].team == team) {
			if (index == count) {
				cg.selectedScore = i;
			}
			count++;
		}
	}
}

float CG_Cvar_Get(const char *cvar) {
	char buff[128];

	memset(buff, 0, sizeof (buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof (buff));
	return atof(buff);
}

void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit, int style) {
	// Nico, silent GCC
	(void)cursor;
	(void)cursorPos;

	CG_Text_Paint(x, y, scale, color, text, 0, limit, style);
}

/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu() {
	cgDC.registerShaderNoMip  = &trap_R_RegisterShaderNoMip;
	cgDC.setColor             = &trap_R_SetColor;
	cgDC.drawHandlePic        = &CG_DrawPic;
	cgDC.drawStretchPic       = &trap_R_DrawStretchPic;
	cgDC.drawText             = &CG_Text_Paint;
	cgDC.drawTextExt          = &CG_Text_Paint_Ext;
	cgDC.textWidth            = &CG_Text_Width;
	cgDC.textWidthExt         = &CG_Text_Width_Ext;
	cgDC.textHeight           = &CG_Text_Height;
	cgDC.textHeightExt        = &CG_Text_Height_Ext;
	cgDC.textFont             = &CG_Text_SetActiveFont;
	cgDC.registerModel        = &trap_R_RegisterModel;
	cgDC.modelBounds          = &trap_R_ModelBounds;
	cgDC.fillRect             = &CG_FillRect;
	cgDC.drawRect             = &CG_DrawRect;
	cgDC.drawSides            = &CG_DrawSides;
	cgDC.drawTopBottom        = &CG_DrawTopBottom;
	cgDC.clearScene           = &trap_R_ClearScene;
	cgDC.addRefEntityToScene  = &trap_R_AddRefEntityToScene;
	cgDC.renderScene          = &trap_R_RenderScene;
	cgDC.registerFont         = &trap_R_RegisterFont;
	cgDC.ownerDrawVisible     = &CG_OwnerDrawVisible;
	cgDC.runScript            = &CG_RunMenuScript;
	cgDC.setCVar              = trap_Cvar_Set;
	cgDC.getCVarString        = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue         = CG_Cvar_Get;
	cgDC.drawTextWithCursor   = &CG_Text_PaintWithCursor;
	cgDC.setOverstrikeMode    = &trap_Key_SetOverstrikeMode;
	cgDC.getOverstrikeMode    = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound      = &trap_S_StartLocalSound;
	cgDC.feederCount          = &CG_FeederCount;
	cgDC.feederItemText       = &CG_FeederItemText;
	cgDC.feederSelection      = &CG_FeederSelection;
	cgDC.setBinding           = &trap_Key_SetBinding;       // NERVE - SMF
	cgDC.getBindingBuf        = &trap_Key_GetBindingBuf;    // NERVE - SMF
	cgDC.getKeysForBinding    = &trap_Key_KeysForBinding;
	cgDC.keynumToStringBuf    = &trap_Key_KeynumToStringBuf;   // NERVE - SMF
	cgDC.translateString      = &CG_TranslateString;        // NERVE - SMF
	cgDC.Error                = &Com_Error;
	cgDC.Print                = &Com_Printf;
	cgDC.registerSound        = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack  = &trap_S_StopBackgroundTrack;
	cgDC.add2dPolys           = &trap_R_Add2dPolys;
	cgDC.updateScreen         = &trap_UpdateScreen;
	cgDC.getHunkData          = &trap_GetHunkData;
	cgDC.getConfigString      = &CG_ConfigStringCopy;

	cgDC.xscale = cgs.screenXScale;
	cgDC.yscale = cgs.screenYScale;

	Init_Display(&cgDC);

	Menu_Reset();

	CG_Text_SetActiveFont(0);
}

void CG_AssetCache() {
	cgDC.Assets.gradientBar         = trap_R_RegisterShaderNoMip(ASSET_GRADIENTBAR);
	cgDC.Assets.fxBasePic           = trap_R_RegisterShaderNoMip(ART_FX_BASE);
	cgDC.Assets.fxPic[0]            = trap_R_RegisterShaderNoMip(ART_FX_RED);
	cgDC.Assets.fxPic[1]            = trap_R_RegisterShaderNoMip(ART_FX_YELLOW);
	cgDC.Assets.fxPic[2]            = trap_R_RegisterShaderNoMip(ART_FX_GREEN);
	cgDC.Assets.fxPic[3]            = trap_R_RegisterShaderNoMip(ART_FX_TEAL);
	cgDC.Assets.fxPic[4]            = trap_R_RegisterShaderNoMip(ART_FX_BLUE);
	cgDC.Assets.fxPic[5]            = trap_R_RegisterShaderNoMip(ART_FX_CYAN);
	cgDC.Assets.fxPic[6]            = trap_R_RegisterShaderNoMip(ART_FX_WHITE);
	cgDC.Assets.scrollBar           = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR);
	cgDC.Assets.scrollBarArrowDown  = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWDOWN);
	cgDC.Assets.scrollBarArrowUp    = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWUP);
	cgDC.Assets.scrollBarArrowLeft  = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWLEFT);
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWRIGHT);
	cgDC.Assets.scrollBarThumb      = trap_R_RegisterShaderNoMip(ASSET_SCROLL_THUMB);
	cgDC.Assets.sliderBar           = trap_R_RegisterShaderNoMip(ASSET_SLIDER_BAR);
	cgDC.Assets.sliderThumb         = trap_R_RegisterShaderNoMip(ASSET_SLIDER_THUMB);
}

void CG_ClearTrails(void);
void CG_ClearParticles(void);

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum) {
	const char *s;

	// clear everything
	memset(&cgs, 0, sizeof (cgs));
	memset(&cg, 0, sizeof (cg));
	memset(cg_entities, 0, sizeof (cg_entities));
	memset(cg_weapons, 0, sizeof (cg_weapons));
	memset(cg_items, 0, sizeof (cg_items));

	cgs.initing = qtrue;

	// OSP - sync to main refdef
	cg.refdef_current = &cg.refdef;

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// RF, init the anim scripting
	cgs.animScriptData.soundIndex = CG_SoundScriptPrecache;
	cgs.animScriptData.playSound  = CG_SoundPlayIndexedScript;

	cg.clientNum = clientNum;       // NERVE - SMF - TA merge

	cgs.processedSnapshotNum  = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	// Nico, bugfix on loading screen
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=079
	trap_R_SetColor(NULL);

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader = trap_R_RegisterShader("gfx/2d/hudchars");       //trap_R_RegisterShader( "gfx/2d/bigchars" );
	// JOSEPH 4-17-00
	cgs.media.menucharsetShader = trap_R_RegisterShader("gfx/2d/hudchars");
	// END JOSEPH
	cgs.media.whiteShader     = trap_R_RegisterShader("white");
	cgs.media.charsetProp     = trap_R_RegisterShaderNoMip("menu/art/font1_prop.tga");
	cgs.media.charsetPropGlow = trap_R_RegisterShaderNoMip("menu/art/font1_prop_glo.tga");
	cgs.media.charsetPropB    = trap_R_RegisterShaderNoMip("menu/art/font2_prop.tga");

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	// Gordon: moved this up so it's initialized for the loading screen
	CG_LoadHudMenu();      // load new hud stuff
	CG_AssetCache();

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);

	CG_ParseServerinfo();
	CG_ParseWolfinfo();     // NERVE - SMF

	cgs.campaignInfoLoaded = qfalse;

	CG_LocateArena();

	CG_ClearTrails();
	CG_ClearParticles();

	InitSmokeSprites();

	// check version
	s = CG_ConfigString(CS_GAME_VERSION);
	if (strcmp(s, GAME_VERSION)) {
		CG_Error("Client/Server game mismatch: '%s/%s'", GAME_VERSION, s);
	}
	trap_Cvar_Set("cg_etVersion", GAME_VERSION_DATED);   // So server can check

	s                  = CG_ConfigString(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);

	CG_initStrings();
	CG_windowInit();

	cgs.smokeWindDir = crandom();

	// load the new map
	CG_LoadingString("collision map");

	trap_CM_LoadMap(cgs.mapname);

	String_Init();

	CG_LoadingString("sounds");

	CG_RegisterSounds();

	CG_LoadingString("graphics");

	CG_RegisterGraphics();

	CG_LoadingString("flamechunks");

	CG_InitFlameChunks();       // RF, register and clear all flamethrower resources

	CG_LoadingString("clients");

	CG_RegisterClients();       // if low on memory, some clients will be deferred

	CG_InitLocalEntities();

	BG_BuildSplinePaths();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	cg.lightstylesInited = qfalse;

	CG_LoadingString("");

	CG_ShaderStateChanged();

	CG_ChargeTimesChanged();

	trap_S_ClearLoopingSounds();
	trap_S_ClearSounds(qfalse);

	cg.filtercams = atoi(CG_ConfigString(CS_FILTERCAMS)) ? qtrue : qfalse;

	CG_ParseFireteams();

	CG_ParseOIDInfos();

	CG_InitPM();

	CG_ParseSpawns();

	CG_ParseTagConnects();

	CG_ParseSkyBox();

	CG_SetupCabinets();

	trap_S_FadeAllSound(1.0f, 0, qfalse);     // fade sound up
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown(void) {
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data

	CG_EventHandling(CGAME_EVENT_NONE, qtrue);
	if (cg.demoPlayback) {
		trap_Cvar_Set("timescale", "1");
	}
}

qboolean CG_CheckExecKey(int key) {
	if (!cg.showFireteamMenu) {
		return qfalse;
	}

	return CG_FireteamCheckExecKey(key, qfalse);
}

// Quoted-Printable like encoding
void CG_EncodeQP(const char *in, char *out, int maxlen) {
	char t;
	char *first = out;

	// sanity check
	if (maxlen <= 0) {
		return;
	}

	while (*in) {
		if (*in == '"' || *in == '%' || *in == '=' || *((byte *) in) > 127) {
			if (out - first + 4 >= maxlen) {
				break;
			}
			*out++ = '=';
			t      = *((byte *) in) / 16;
			*out++ = t <= 9 ? t + '0' : t - 10 + 'A';
			t      = *((byte *) in) % 16;
			*out++ = t <= 9 ? t + '0' : t - 10 + 'A';
			in++;
		} else {
			if (out - first + 1 >= maxlen) {
				break;
			}
			*out++ = *in++;
		}
	}
	*out = '\0';
}

// Quoted-Printable decoding
void CG_DecodeQP(char *line) {
	char *o = line;
	char t;

	while (*line) {
		if (*line == '=') {
			line++;

			if (!*line || !*(line + 1)) {
				break;
			}

			t = 0;
			if (!isxdigit(*line)) {
				line += 2;
				continue;
			}
			t = gethex(*line) * 16;

			line++;
			if (!isxdigit(*line)) {
				line++;
				continue;
			}
			t += gethex(*line);
			line++;
			*o++ = t;
		} else {
			*o++ = *line++;
		}
	}
	*o = '\0';
}
