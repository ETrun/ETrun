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
 * name:		cg_consolecmds.c
 *
 * desc:		text commands typed in at the local console, or executed by a key binding
 *
*/

#include "cg_local.h"

#if defined _WIN32
# include <Windows.h>     // Nico, needed for minimize
#endif

void CG_TargetCommand_f(void) {
	int  targetNum;
	char test[4];

	targetNum = CG_CrosshairPlayer();
	if (!targetNum) {
		return;
	}

	trap_Argv(1, test, 4);
	trap_SendConsoleCommand(va("gc %i %i", targetNum, atoi(test)));
}

/*
=============
CG_Viewpos_f

Debugging command to print the current position
=============
*/
static void CG_Viewpos_f(void) {
	CG_Printf("(%i %i %i) : %i\n", (int)cg.refdef.vieworg[0],
	          (int)cg.refdef.vieworg[1], (int)cg.refdef.vieworg[2],
	          (int)cg.refdefViewAngles[YAW]);
}

void CG_LimboMenu_f(void) {
	if (cg.showGameView) {
		CG_EventHandling(CGAME_EVENT_NONE, qfalse);
	} else {
		CG_EventHandling(CGAME_EVENT_GAMEVIEW, qfalse);
	}
}

void CG_ScoresDown_f(void) {
	if (cg.scoresRequestTime + 2000 < cg.time) {
		// the scores are more than two seconds out of data,
		// so request new ones
		cg.scoresRequestTime = cg.time;

		// OSP - we get periodic score updates if we are merging clients
		if (!cg.demoPlayback) {
			trap_SendClientCommand("score");
		}

		// leave the current scores up if they were already
		// displayed, but if this is the first hit, clear them out
		if (!cg.showScores) {
			cg.showScores = qtrue;
			if (!cg.demoPlayback) {
				cg.numScores = 0;
			}
		}
	} else {
		// show the cached contents even if they just pressed if it
		// is within two seconds
		cg.showScores = qtrue;
	}
}

void CG_ScoresUp_f(void) {
	if (cg.showScores) {
		cg.showScores    = qfalse;
		cg.scoreFadeTime = cg.time;
	}
}
static void CG_LoadWeapons_f(void) {
	int i;

	for (i = WP_KNIFE; i < WP_NUM_WEAPONS; ++i) {
		CG_RegisterWeapon(i, qtrue);
	}
}

static void CG_TellTarget_f(void) {
	int  clientNum;
	char command[128];
	char message[128];

	clientNum = CG_CrosshairPlayer();
	if (clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

static void CG_TellAttacker_f(void) {
	int  clientNum;
	char command[128];
	char message[128];

	clientNum = CG_LastAttacker();
	if (clientNum == -1) {
		return;
	}

	trap_Args(message, 128);
	Com_sprintf(command, 128, "tell %i %s", clientNum, message);
	trap_SendClientCommand(command);
}

/////////// cameras

#define MAX_CAMERAS 64  // matches define in splines.cpp
qboolean cameraInuse[MAX_CAMERAS];

int CG_LoadCamera(const char *name) {
	int i;

	for (i = 1; i < MAX_CAMERAS; ++i) {      // start at '1' since '0' is always taken by the cutscene camera
		if (!cameraInuse[i] && trap_loadCamera(i, name)) {
			cameraInuse[i] = qtrue;
			return i;
		}
	}
	return -1;
}

/*
==============
CG_StartCamera
==============
*/
void CG_StartCamera(const char *name, qboolean startBlack) {
	char lname[MAX_QPATH];

	COM_StripExtension(name, lname);      //----(SA)	added
	strcat(lname, ".camera");

	if (trap_loadCamera(CAM_PRIMARY, va("cameras/%s", lname))) {
		cg.cameraMode = qtrue;                  // camera on in cgame
		if (startBlack) {
			CG_Fade(255, cg.time, 0);    // go black
		}
		trap_Cvar_Set("cg_letterbox", "1");   // go letterbox
		trap_startCamera(CAM_PRIMARY, cg.time);   // camera on in client
	} else {
//----(SA)	removed check for cams in main dir
		cg.cameraMode = qfalse;                 // camera off in cgame
		trap_SendClientCommand("stopCamera");      // camera off in game
		trap_stopCamera(CAM_PRIMARY);             // camera off in client
		CG_Fade(0, cg.time, 0);          // ensure fadeup
		trap_Cvar_Set("cg_letterbox", "0");
		CG_Printf("Unable to load camera %s\n", lname);
	}
}

/*
==============
CG_StopCamera
==============
*/
void CG_StopCamera(void) {
	cg.cameraMode = qfalse;                 // camera off in cgame
	trap_SendClientCommand("stopCamera");      // camera off in game
	trap_stopCamera(CAM_PRIMARY);             // camera off in client
	trap_Cvar_Set("cg_letterbox", "0");

	// fade back into world
	CG_Fade(255, 0, 0);
	CG_Fade(0, cg.time + 500, 2000);
}

static void CG_Fade_f(void) {
	int   a;
	float duration;

	if (trap_Argc() < 6) {
		return;
	}

	a = atof(CG_Argv(4));

	duration = atof(CG_Argv(5)) * 1000;

	CG_Fade(a, cg.time, duration);
}

void CG_QuickMessage_f(void) {
	CG_EventHandling(CGAME_EVENT_NONE, qfalse);

	if (cg_quickMessageAlt.integer) {
		trap_UI_Popup(UIMENU_WM_QUICKMESSAGEALT);
	} else {
		trap_UI_Popup(UIMENU_WM_QUICKMESSAGE);
	}
}

void CG_QuickFireteamMessage_f(void) {
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
		return;
	}

	CG_EventHandling(CGAME_EVENT_NONE, qfalse);

	if (cg_quickMessageAlt.integer) {
		trap_UI_Popup(UIMENU_WM_FTQUICKMESSAGEALT);
	} else {
		trap_UI_Popup(UIMENU_WM_FTQUICKMESSAGE);
	}
}

void CG_QuickFireteamAdmin_f(void) {
	trap_UI_Popup(UIMENU_NONE);

	if (cg.showFireteamMenu) {
		if (cgs.ftMenuMode == 1) {
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		} else {
			cgs.ftMenuMode = 1;
		}
	} else {   // Nico, allow FT menu for spectators too
		CG_EventHandling(CGAME_EVENT_FIRETEAMMSG, qfalse);
		cgs.ftMenuMode = 1;
	}
}

static void CG_QuickFireteams_f(void) {
	if (cg.showFireteamMenu) {
		if (cgs.ftMenuMode == 0) {
			CG_EventHandling(CGAME_EVENT_NONE, qfalse);
		} else {
			cgs.ftMenuMode = 0;
		}
	} else if (CG_IsOnFireteam(cg.clientNum)) {
		CG_EventHandling(CGAME_EVENT_FIRETEAMMSG, qfalse);
		cgs.ftMenuMode = 0;
	}
}

static void CG_FTSayPlayerClass_f(void) {
	int        playerType;
	const char *s;

	playerType = cgs.clientinfo[cg.clientNum].cls;

	if (playerType == PC_MEDIC) {
		s = "IamMedic";
	} else if (playerType == PC_ENGINEER) {
		s = "IamEngineer";
	} else if (playerType == PC_FIELDOPS) {
		s = "IamFieldOps";
	} else if (playerType == PC_COVERTOPS) {
		s = "IamCovertOps";
	} else {
		s = "IamSoldier";
	}

	trap_SendConsoleCommand(va("cmd vsay_buddy -1 %s %s\n", CG_BuildSelectedFirteamString(), s));
}

static void CG_SayPlayerClass_f(void) {
	int        playerType;
	const char *s;

	playerType = cgs.clientinfo[cg.clientNum].cls;

	if (playerType == PC_MEDIC) {
		s = "IamMedic";
	} else if (playerType == PC_ENGINEER) {
		s = "IamEngineer";
	} else if (playerType == PC_FIELDOPS) {
		s = "IamFieldOps";
	} else if (playerType == PC_COVERTOPS) {
		s = "IamCovertOps";
	} else {
		s = "IamSoldier";
	}

	trap_SendConsoleCommand(va("cmd vsay_team %s\n", s));
}

static void CG_VoiceChat_f(void) {
	char chatCmd[64];

	if (trap_Argc() != 2) {
		return;
	}

	trap_Argv(1, chatCmd, 64);

	trap_SendConsoleCommand(va("cmd vsay %s\n", chatCmd));
}

static void CG_TeamVoiceChat_f(void) {
	char chatCmd[64];

	if (trap_Argc() != 2) {
		return;
	}

	trap_Argv(1, chatCmd, 64);

	trap_SendConsoleCommand(va("cmd vsay_team %s\n", chatCmd));
}

static void CG_BuddyVoiceChat_f(void) {
	char chatCmd[64];

	if (trap_Argc() != 2) {
		return;
	}

	trap_Argv(1, chatCmd, 64);

	trap_SendConsoleCommand(va("cmd vsay_buddy -1 %s %s\n", CG_BuildSelectedFirteamString(), chatCmd));
}

// ydnar: say, team say, etc
static void CG_MessageMode_f(void) {
	char cmd[64];

	if ((int)cgs.eventHandling != CGAME_EVENT_NONE) {
		return;
	}

	// get the actual command
	trap_Argv(0, cmd, 64);

	// team say
	if (!Q_stricmp(cmd, "messagemode2")) {
		trap_Cvar_Set("cg_messageType", "2");
	}
	// fireteam say
	else if (!Q_stricmp(cmd, "messagemode3")) {
		trap_Cvar_Set("cg_messageType", "3");
	}
	// (normal) say
	else {
		trap_Cvar_Set("cg_messageType", "1");
	}

	// clear the chat text
	trap_Cvar_Set("cg_messageText", "");

	// open the menu
	trap_UI_Popup(UIMENU_INGAME_MESSAGEMODE);
}

// Nico, note: using Quoted-Printable encoding
static void CG_MessageSend_f(void) {
	char messageText[256];
	char messageTextEncoded[3 * 256];
	int  messageType;

	// get values
	trap_Cvar_VariableStringBuffer("cg_messageType", messageText, sizeof (messageText));
	messageType = atoi(messageText);
	trap_Cvar_VariableStringBuffer("cg_messageText", messageText, sizeof (messageText));

	// reset values
	trap_Cvar_Set("cg_messageText", "");
	trap_Cvar_Set("cg_messageType", "");
	trap_Cvar_Set("cg_messagePlayer", "");

	// don't send empty messages
	if (messageText[0] == '\0') {
		return;
	}

	CG_EncodeQP(messageText, messageTextEncoded, sizeof (messageTextEncoded));

	// team say
	if (messageType == 2) {
		trap_SendConsoleCommand(va("enc_say_team \"%s\"\n", messageTextEncoded));
	} else if (messageType == 3) {   // fireteam say
		trap_SendConsoleCommand(va("enc_say_buddy \"%s\"\n", messageTextEncoded));
	} else {    // normal say
		trap_SendConsoleCommand(va("enc_say \"%s\"\n", messageTextEncoded));
	}
}

static void CG_SetWeaponCrosshair_f(void) {
	char crosshair[64];

	trap_Argv(1, crosshair, 64);
	cg.newCrosshairIndex = atoi(crosshair) + 1;
}
// -NERVE - SMF

static void CG_SelectBuddy_f(void) {
	int          pos = atoi(CG_Argv(1));
	int          i;
	clientInfo_t *ci;

	// Gordon:
	// 0 - 5 = select that person
	// -1 = none
	// -2 = all

	switch (pos) {
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		if (!CG_IsOnFireteam(cg.clientNum)) {
			break;     // Gordon: we aren't a leader, so dont allow selection
		}

		ci = CG_SortedFireTeamPlayerForPosition(pos, 6);
		if (!ci) {
			break;     // there was no-one in this position
		}

		ci->selected ^= qtrue;

		break;

	case -1:
		if (!CG_IsOnFireteam(cg.clientNum)) {
			break;     // Gordon: we aren't a leader, so dont allow selection
		}

		for (i = 0; i < 6; ++i) {
			ci = CG_SortedFireTeamPlayerForPosition(i, 6);
			if (!ci) {
				break;     // there was no-one in this position
			}

			ci->selected = qfalse;
		}
		break;

	case -2:
		if (!CG_IsOnFireteam(cg.clientNum)) {
			break;     // Gordon: we aren't a leader, so dont allow selection
		}

		for (i = 0; i < 6; ++i) {
			ci = CG_SortedFireTeamPlayerForPosition(i, 6);
			if (!ci) {
				break;     // there was no-one in this position
			}

			ci->selected = qtrue;
		}
		break;

	}
}

// OSP
const char *aMonths[12] =
{
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void CG_currentTime_f(void) {
	qtime_t ct;

	trap_RealTime(&ct);
	CG_Printf("[cgnotify]Current time: ^3%02d:%02d:%02d (%02d %s %d)\n", ct.tm_hour, ct.tm_min, ct.tm_sec, ct.tm_mday, aMonths[ct.tm_mon], 1900 + ct.tm_year);
}

// Dynamically names a demo and sets up the recording
void CG_autoRecord_f(void) {
	trap_SendConsoleCommand(va("record %s\n", CG_generateFilename()));
}

// Dynamically names a screenshot[JPEG]
void CG_autoScreenShot_f(void) {
	trap_SendConsoleCommand(va("screenshot%s %s\n", ((cg_useScreenshotJPEG.integer) ? "JPEG" : ""), CG_generateFilename()));
}

void CG_vstrDown_f(void) {
	// The engine also passes back the key code and time of the key press
	if (trap_Argc() == 5) {
		trap_SendConsoleCommand(va("vstr %s;", CG_Argv(1)));
	} else {
		CG_Printf("[cgnotify]Usage: +vstr [down_vstr] [up_vstr]\n");
	}
}

void CG_vstrUp_f(void) {
	// The engine also passes back the key code and time of the key press
	if (trap_Argc() == 5) {
		trap_SendConsoleCommand(va("vstr %s;", CG_Argv(2)));
	} else {
		CG_Printf("[cgnotify]Usage: +vstr [down_vstr] [up_vstr]\n");
	}
}

void CG_keyOn_f(void) {
	if (!cg.demoPlayback) {
		CG_Printf("[cgnotify]^3*** NOT PLAYING A DEMO!!\n");
		return;
	}

	if (demo_infoWindow.integer > 0) {
		CG_ShowHelp_On(&cg.demohelpWindow);
	}

	CG_EventHandling(CGAME_EVENT_DEMO, qtrue);
}

void CG_keyOff_f(void) {
	if (!cg.demoPlayback) {
		return;
	}
	CG_EventHandling(CGAME_EVENT_NONE, qfalse);
}
// -OSP

static void CG_EditSpeakers_f(void) {
	if (cg.editingSpeakers) {
		CG_DeActivateEditSoundMode();
	} else {
		const char *s = Info_ValueForKey(CG_ConfigString(CS_SYSTEMINFO), "sv_cheats");
		if (s[0] != '1') {
			CG_Printf("editSpeakers is cheat protected.\n");
			return;
		}
		CG_ActivateEditSoundMode();
	}
}

static void CG_DumpSpeaker_f(void) {
	bg_speaker_t speaker;
	trace_t      tr;
	vec3_t       end;

	if (!cg.editingSpeakers) {
		CG_Printf("Speaker Edit mode needs to be activated to dump speakers\n");
		return;
	}

	memset(&speaker, 0, sizeof (speaker));

	speaker.volume = 127;
	speaker.range  = 1250;

	VectorMA(cg.refdef_current->vieworg, 32, cg.refdef_current->viewaxis[0], end);
	CG_Trace(&tr, cg.refdef_current->vieworg, NULL, NULL, end, -1, MASK_SOLID);

	if (tr.fraction < 1.f) {
		VectorCopy(tr.endpos, speaker.origin);
		VectorMA(speaker.origin, -4, cg.refdef_current->viewaxis[0], speaker.origin);
	} else {
		VectorCopy(tr.endpos, speaker.origin);
	}

	if (!BG_SS_StoreSpeaker(&speaker)) {
		CG_Printf(S_COLOR_RED "ERROR: Failed to store speaker\n");
	}
}

static void CG_ModifySpeaker_f(void) {
	if (cg.editingSpeakers) {
		CG_ModifyEditSpeaker();
	}
}

static void CG_UndoSpeaker_f(void) {
	if (cg.editingSpeakers) {
		CG_UndoEditSpeaker();
	}
}

static void CG_CPM_f(void) {
	CG_AddPMItem(PM_MESSAGE, CG_Argv(1), cgs.media.voiceChatShader);
}

/**
 * Nico, game minimizer (windows only)
 * @source: http://forums.warchestgames.com/showthread.php/24040-CODE-Tutorial-Minimize-Et-(Only-Windoof)
 *
 */
static void CG_Minimize_f(void) {
#if defined _WIN32
	HWND wnd;

	wnd = GetForegroundWindow();
	if (wnd) {
		ShowWindow(wnd, SW_MINIMIZE);
	}
#else
	CG_Printf(S_COLOR_RED "ERROR: minimize command is not supported on this OS.\n");
#endif
}

/*
==================
CG_Tutorial_f

Tutorial command

@author suburb
==================
*/
static void CG_Tutorial_f(void) {
	CG_Printf("^9-----------------------------------------------------------------------------\n");
	CG_Printf(va("Welcome to %s^7, an Enemy Territory game modification with timeruns\n", GAME_VERSION_COLORED));
	CG_Printf("support. In order to permanently save records, you need to create an\n");
	CG_Printf(va("account on ^fhttps://timeruns.net/^7, the official %s^7 website, and\n", GAME_VERSION_COLORED));
	CG_Printf("link it to your game. Here is a step-by-step tutorial:\n");
	CG_Printf("\n");
	CG_Printf("^51. ^7Go to ^fhttps://timeruns.net/ ^7and open the Signup tab.\n");
	CG_Printf("^52. ^7Follow the instructions and wait for the account activation email.\n");
	CG_Printf(va("^53. ^7Once your account has been activated, login on the %s^7 forum,\n", GAME_VERSION_COLORED));
	CG_Printf("   which is located here: ^fhttps://forum.timeruns.net/\n");
	CG_Printf("^54. ^7In the top right corner, click on your nickname and follow this path:\n");
	CG_Printf("   User Control Panel -> Profile -> Edit account settings\n");
	CG_Printf("   Now you can see your Timeruns token. This is your password which\n");
	CG_Printf("   links your game to your own website account. Never share it!\n");
	CG_Printf("^55. ^7Copy your Timeruns token.\n");
	CG_Printf("^56. ^7Insert your Timeruns token ingame into the ^n/etr_authToken ^7cvar.\n");
	CG_Printf("^57. ^7Type ^n/login ^7into the console.\n");
	CG_Printf("\n");
	CG_Printf("Congratulations! You are now logged in and able to set records. You can\n");
	CG_Printf(va("now find your stats on the %s^7 website and share them with your friends.\n", GAME_VERSION_COLORED));
	CG_Printf("\n");
	CG_Printf("Have fun.\n");
	CG_Printf("^9-----------------------------------------------------------------------------\n");
}

typedef struct {
	char *cmd;
	void (*function)(void);
} consoleCommand_t;

static consoleCommand_t commands[] =
{
	{ "testgun",             CG_TestGun_f            },
	{ "testmodel",           CG_TestModel_f          },
	{ "nextframe",           CG_TestModelNextFrame_f },
	{ "prevframe",           CG_TestModelPrevFrame_f },
	{ "nextskin",            CG_TestModelNextSkin_f  },
	{ "prevskin",            CG_TestModelPrevSkin_f  },
	{ "viewpos",             CG_Viewpos_f            },
	{ "+scores",             CG_ScoresDown_f         },
	{ "-scores",             CG_ScoresUp_f           },
	{ "zoomin",              CG_ZoomIn_f             },
	{ "zoomout",             CG_ZoomOut_f            },
	{ "weaplastused",        CG_LastWeaponUsed_f     },
	{ "weapnextinbank",      CG_NextWeaponInBank_f   },
	{ "weapprevinbank",      CG_PrevWeaponInBank_f   },
	{ "weapnext",            CG_NextWeapon_f         },
	{ "weapprev",            CG_PrevWeapon_f         },
	{ "weapalt",             CG_AltWeapon_f          },
	{ "weapon",              CG_Weapon_f             },
	{ "weaponbank",          CG_WeaponBank_f         },
	{ "tell_target",         CG_TellTarget_f         },
	{ "tell_attacker",       CG_TellAttacker_f       },
	{ "tcmd",                CG_TargetCommand_f      },
	{ "fade",                CG_Fade_f               }, // duffy
	{ "loadweapons",         CG_LoadWeapons_f        },
	{ "mp_QuickMessage",     CG_QuickMessage_f       },
	{ "mp_fireteammsg",      CG_QuickFireteams_f     },
	{ "mp_fireteamadmin",    CG_QuickFireteamAdmin_f },
	{ "wm_sayPlayerClass",   CG_SayPlayerClass_f     },
	{ "wm_ftsayPlayerClass", CG_FTSayPlayerClass_f   },
	{ "VoiceChat",           CG_VoiceChat_f          },
	{ "VoiceTeamChat",       CG_TeamVoiceChat_f      },
	{ "messageMode",         CG_MessageMode_f        },
	{ "messageMode2",        CG_MessageMode_f        },
	{ "messageMode3",        CG_MessageMode_f        },
	{ "messageSend",         CG_MessageSend_f        },
	{ "SetWeaponCrosshair",  CG_SetWeaponCrosshair_f },
	{ "VoiceFireTeamChat",   CG_BuddyVoiceChat_f     },
	{ "openlimbomenu",       CG_LimboMenu_f          },
	{ "autoRecord",          CG_autoRecord_f         },
	{ "autoScreenshot",      CG_autoScreenShot_f     },
	{ "currentTime",         CG_currentTime_f        },
	{ "keyoff",              CG_keyOff_f             },
	{ "keyon",               CG_keyOn_f              },
	{ "+vstr",               CG_vstrDown_f           },
	{ "-vstr",               CG_vstrUp_f             },
	{ "selectbuddy",         CG_SelectBuddy_f        },
	{ "editSpeakers",        CG_EditSpeakers_f       },
	{ "dumpSpeaker",         CG_DumpSpeaker_f        },
	{ "modifySpeaker",       CG_ModifySpeaker_f      },
	{ "undoSpeaker",         CG_UndoSpeaker_f        },
	{ "cpm",                 CG_CPM_f                },

	// Nico, minimize command
	{ "minimize",            CG_Minimize_f           },

	// suburb, tutorial command
	{ "tutorial",            CG_Tutorial_f           },
};

// Nico, here are ignored commands, (no warning issued for them)
static consoleCommand_t ignoredClientCommands[] =
{
	{ "loadhud",       NULL },
	{ "+stats",        NULL },
	{ "-stats",        NULL },
	{ "statsdump",     NULL },
	{ "ToggleAutoMap", NULL },
	{ "+topshots",     NULL },
	{ "-topshots",     NULL },
};

/*
=================
CG_ConsoleCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
qboolean CG_ConsoleCommand(void) {
	const char *cmd;
	int        i;

	// Arnout - don't allow console commands until a snapshot is present
	if (!cg.snap) {
		return qfalse;
	}

	cmd = CG_Argv(0);

	// Nico, check for ignored client commands
	for (i = 0 ; i < (int)(sizeof (ignoredClientCommands) / sizeof (ignoredClientCommands[0])) ; ++i) {
		if (!Q_stricmp(cmd, ignoredClientCommands[i].cmd)) {
			return qtrue;
		}
	}

	for (i = 0 ; i < (int)(sizeof (commands) / sizeof (commands[0])) ; ++i) {
		if (!Q_stricmp(cmd, commands[i].cmd)) {
			commands[i].function();
			return qtrue;
		}
	}

	return qfalse;
}

/*
=================
CG_InitConsoleCommands

Let the client system know about all of our commands
so it can perform tab completion
=================
*/
void CG_InitConsoleCommands(void) {
	int i;

	for (i = 0 ; i < (int)(sizeof (commands) / sizeof (commands[0])) ; ++i) {
		trap_AddCommand(commands[i].cmd);
	}

	//
	// the game server will interpret these commands, which will be automatically
	// forwarded to the server after they are not recognized locally
	//
	trap_AddCommand("kill");
	trap_AddCommand("say");
	trap_AddCommand("noclip");
	trap_AddCommand("team");
	trap_AddCommand("follow");   // Nico, note: this is used to spectate
	trap_AddCommand("callvote");
	trap_AddCommand("vote");
	trap_AddCommand("follownext");
	trap_AddCommand("followprev");
	trap_AddCommand("?");
	trap_AddCommand("commands");
	trap_AddCommand("players");
	trap_AddCommand("ref");
	trap_AddCommand("say_team");
	trap_AddCommand("specinvite");
	trap_AddCommand("specuninvite");
	trap_AddCommand("speclock");
	trap_AddCommand("specunlock");
	trap_AddCommand("fireteam");
	trap_AddCommand("ignore");
	trap_AddCommand("unignore");
	trap_AddCommand("say_buddy");
	trap_AddCommand("setspawnpt");
	trap_AddCommand("vsay");
	trap_AddCommand("vsay_buddy");
	trap_AddCommand("vsay_team");

	// Nico, save/load
	trap_AddCommand("load");
	trap_AddCommand("save");

	// Nico, login/logout
	trap_AddCommand("login");
	trap_AddCommand("logout");

	// Nico, records & rank
	trap_AddCommand("records");
	trap_AddCommand("rank");

	// Nico, class command
	trap_AddCommand("class");

	// Nico, load player checkpoints command
	trap_AddCommand("loadCheckpoints");

	// Nico, private message
	trap_AddCommand("m");

	// Nico, help
	trap_AddCommand("h");

	// suburb, interrupt run command
	trap_AddCommand("interruptRun");

	// suburb, tutorial
	trap_AddCommand("tutorial");

	// suburb, mod info
	trap_AddCommand("mod_information");
}
