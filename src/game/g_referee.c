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

// G_referee.c: Referee handling
// -------------------------------
// 04 Apr 02
// rhea@OrangeSmoothie.org
//
#include "g_local.h"
#include "../../etrun/ui/menudef.h"

//
// UGH!  Clean me!!!!
//

// Parses for a referee command.
//	--> ref arg allows for the server console to utilize all referee commands (ent == NULL)
//
qboolean G_refCommandCheck(gentity_t *ent, char *cmd) {
	if (!Q_stricmp(cmd, "lock")) {
		G_refLockTeams_cmd(ent, qtrue);
	} else if (!Q_stricmp(cmd, "help")) {
		G_refHelp_cmd(ent);
	} else if (!Q_stricmp(cmd, "putallies")) {
		G_refPlayerPut_cmd(ent, TEAM_ALLIES);
	} else if (!Q_stricmp(cmd, "putaxis")) {
		G_refPlayerPut_cmd(ent, TEAM_AXIS);
	} else if (!Q_stricmp(cmd, "remove")) {
		G_refRemove_cmd(ent);
	} else if (!Q_stricmp(cmd, "unlock")) {
		G_refLockTeams_cmd(ent, qfalse);
	} else if (!Q_stricmp(cmd, "warn")) {
		G_refWarning_cmd(ent);
	} else if (!Q_stricmp(cmd, "mute")) {
		G_refMute_cmd(ent, qtrue);
	} else if (!Q_stricmp(cmd, "unmute")) {
		G_refMute_cmd(ent, qfalse);
	} else {
		return qfalse;
	}

	return qtrue;
}

// Lists ref commands.
// Nico, removed non-existing restart command, fixed remove missing parameter
// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=038
// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=039
void G_refHelp_cmd(gentity_t *ent) {
	// List commands only for enabled refs.
	if (ent) {
		CP("print \"\n^3Referee commands:^7\n\"");
		CP("print \"------------------------------------------\n\"");

		G_voteHelp(ent, qfalse);

		CP("print \"\n^5help             putallies^7 <pid>  ^5warn ^7<pid>       ^5putaxis^7 <pid>\n\"");
		CP("print \"^5unlock           ^5mute ^7<pid>       ^5remove^7 <pid>     ^5unmute ^7<pid>\n\"");
		CP("print \"Usage: ^3\\ref <cmd> [params]\n\n\"");

		// Help for the console
	} else {
		G_Printf("\nAdditional console commands:\n");
		G_Printf("----------------------------------------------\n");
		G_Printf("help     remove <pid>      warn <pid>    lock\n");
		G_Printf("putallies <pid>   unlock	 putaxis <pid>\n\n");

		G_Printf("Usage: <cmd> [params]\n\n");
	}
}

// Request for ref status or lists ref commands.
void G_ref_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue) {
	char arg[MAX_TOKEN_CHARS];

	// Nico, silent GCC
	(void)dwCommand;
	(void)fValue;

	// Roll through ref commands if already a ref
	if (ent == NULL || ent->client->sess.referee) {
		voteInfo_t votedata;

		trap_Argv(1, arg, sizeof (arg));

		memcpy(&votedata, &level.voteInfo, sizeof (voteInfo_t));

		if (Cmd_CallVote_f(ent, 0, qtrue)) {
			memcpy(&level.voteInfo, &votedata, sizeof (voteInfo_t));
			return;
		}
		memcpy(&level.voteInfo, &votedata, sizeof (voteInfo_t));

		if (G_refCommandCheck(ent, arg)) {
			return;
		}
		G_refHelp_cmd(ent);
		return;
	}

	if (ent) {
		// Nico, replaced cpm by print (x3)
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=046
		if (!Q_stricmp(refereePassword.string, "none") || !refereePassword.string[0]) {
			CP("print \"^dSorry, referee status ^ndisabled ^don this server\n\"");
			return;
		}

		if (trap_Argc() < 2) {
			CP("print \"^dUsage: ^nref [password]\n\"");
			return;
		}

		trap_Argv(1, arg, sizeof (arg));

		if (Q_stricmp(arg, refereePassword.string)) {
			CP("print \"^nInvalid ^dreferee password\n\"");
			return;
		}

		ent->client->sess.referee = 1;
		AP(va("cp \"%s\n^dhas become a ^nreferee\n\"", ent->client->pers.netname));
		ClientUserinfoChanged(ent - g_entities);
	}
}

// Changes team lock status
void G_refLockTeams_cmd(gentity_t *ent, qboolean fLock) {
	char *status;

	teamInfo[TEAM_AXIS].team_lock   = (TeamCount(-1, TEAM_AXIS)) ? fLock : qfalse;
	teamInfo[TEAM_ALLIES].team_lock = (TeamCount(-1, TEAM_ALLIES)) ? fLock : qfalse;

	status = va("Referee has ^3%sLOCKED^7 teams", ((fLock) ? "" : "UN"));

	G_printFull(status, ent);
	// Nico, removed unneeded linebreak
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=047
	G_refPrintf(ent, "You have %sLOCKED teams", ((fLock) ? "" : "UN"));

	if (fLock) {
		level.server_settings |= CV_SVS_LOCKTEAMS;
	} else {
		level.server_settings &= ~CV_SVS_LOCKTEAMS;
	}
	trap_SetConfigstring(CS_SERVERTOGGLES, va("%d", level.server_settings));
}

// Puts a player on a team.
void G_refPlayerPut_cmd(gentity_t *ent, int team_id) {
	int       pid;
	char      arg[MAX_TOKEN_CHARS];
	gentity_t *player;

	// Find the player to place.
	trap_Argv(2, arg, sizeof (arg));
	if ((pid = ClientNumberFromString(ent, arg)) == -1) {
		return;
	}

	player = g_entities + pid;

	// Can only move to other teams.
	if ((int)player->client->sess.sessionTeam == team_id) {
		// Nico, removed unneeded linebreak
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=047
		G_refPrintf(ent, "\"%s\" is already on team %s!", player->client->pers.netname, aTeams[team_id]);
		return;
	}

	if (team_maxplayers.integer && (int)TeamCount(-1, team_id) >= team_maxplayers.integer) {
		// Nico, removed unneeded linebreak
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=047
		G_refPrintf(ent, "Sorry, the %s team is already full!", aTeams[team_id]);
		return;
	}

	player->client->pers.invite = team_id;

	if (team_id == TEAM_AXIS) {
		SetTeam(player, "red", -1, -1, qfalse);
	} else {
		SetTeam(player, "blue", -1, -1, qfalse);
	}
}

// Removes a player from a team.
void G_refRemove_cmd(gentity_t *ent) {
	int       pid;
	char      arg[MAX_TOKEN_CHARS];
	gentity_t *player;

	// Find the player to remove.
	trap_Argv(2, arg, sizeof (arg));
	if ((pid = ClientNumberFromString(ent, arg)) == -1) {
		return;
	}

	player = g_entities + pid;

	// Can only remove active players.
	if (player->client->sess.sessionTeam == TEAM_SPECTATOR) {
		G_refPrintf(ent, "You can only remove people in the game!");
		return;
	}

	// Announce the removal
	AP(va("cp \"%s\n^7removed from team %s\n\"", player->client->pers.netname, aTeams[player->client->sess.sessionTeam]));
	CPx(pid, va("print \"^dYou've been removed from the ^n%s ^dteam\n\"", aTeams[player->client->sess.sessionTeam]));

	SetTeam(player, "s", -1, -1, qfalse);
}

void G_refWarning_cmd(gentity_t *ent) {
	char cmd[MAX_TOKEN_CHARS];
	char reason[MAX_TOKEN_CHARS];
	int  kicknum;

	trap_Argv(2, cmd, sizeof (cmd));

	if (!*cmd) {
		G_refPrintf(ent, "usage: ref warn <clientname> [reason].");
		return;
	}

	trap_Argv(3, reason, sizeof (reason));

	kicknum = G_refClientnumForName(ent, cmd);

	if (kicknum != MAX_CLIENTS) {
		if (level.clients[kicknum].sess.referee == RL_NONE || ((!ent || ent->client->sess.referee == RL_RCON) && level.clients[kicknum].sess.referee <= RL_REFEREE)) {
			trap_SendServerCommand(-1, va("cpm \"%s^7 was issued a ^1Warning^7 (%s)\n\"\n", level.clients[kicknum].pers.netname, *reason ? reason : "No Reason Supplied"));
		} else {
			G_refPrintf(ent, "Insufficient rights to issue client a warning.");
		}
	}
}

// (Un)Mutes a player
void G_refMute_cmd(gentity_t *ent, qboolean mute) {
	int       pid;
	char      arg[MAX_TOKEN_CHARS];
	gentity_t *player;

	// Find the player to mute.
	trap_Argv(2, arg, sizeof (arg));
	if ((pid = ClientNumberFromString(ent, arg)) == -1) {
		return;
	}

	player = g_entities + pid;

	// Nico, bugfix: allow ref to be unmuted
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=060
	if (player->client->sess.referee != RL_NONE && mute) {
		// Nico removed unneeded linebreak
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=047
		G_refPrintf(ent, "Cannot mute a referee.");
		return;
	}

	if (player->client->sess.muted == mute) {
		// Nico, removed unneeded linebreak
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=047
		G_refPrintf(ent, "\"%s^*\" %s", player->client->pers.netname, mute ? "is already muted!" : "is not muted!");
		return;
	}

	if (mute) {
		CPx(pid, "print \"^5You've been muted\n\"");
		player->client->sess.muted = qtrue;
		G_Printf("\"%s^*\" has been muted\n", player->client->pers.netname);
		ClientUserinfoChanged(pid);
	} else {
		CPx(pid, "print \"^5You've been unmuted\n\"");
		player->client->sess.muted = qfalse;
		G_Printf("\"%s^*\" has been unmuted\n", player->client->pers.netname);
		ClientUserinfoChanged(pid);
	}
}

//////////////////////////////
//  Client authentication
//
void Cmd_AuthRcon_f(gentity_t *ent) {
	char buf[MAX_TOKEN_CHARS], cmd[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer("rconPassword", buf, sizeof (buf));
	trap_Argv(1, cmd, sizeof (cmd));

	if (*buf && !strcmp(buf, cmd)) {
		ent->client->sess.referee = RL_RCON;
	}
}

//////////////////////////////
//  Console admin commands
//
void G_PlayerBan() {
	char cmd[MAX_TOKEN_CHARS];
	int  bannum;

	trap_Argv(1, cmd, sizeof (cmd));

	if (!*cmd) {
		G_Printf("usage: ban <clientname>.");
		return;
	}

	bannum = G_refClientnumForName(NULL, cmd);

	if (bannum != MAX_CLIENTS) {
		const char *value;
		char       userinfo[MAX_INFO_STRING];

		trap_GetUserinfo(bannum, userinfo, sizeof (userinfo));
		value = Info_ValueForKey(userinfo, "ip");

		AddIPBan(value);
	}
}

void G_MakeReferee() {
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof (cmd));

	if (!*cmd) {
		G_Printf("usage: MakeReferee <clientname>.");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if (cnum != MAX_CLIENTS) {
		if (level.clients[cnum].sess.referee == RL_NONE) {
			level.clients[cnum].sess.referee = RL_REFEREE;
			AP(va("cp \"%s\n^dhas been made a ^nreferee\n\"", cmd));
			G_Printf("%s has been made a referee.\n", cmd);
		} else {
			G_Printf("User is already authed.\n");
		}
	}
}

void G_RemoveReferee() {
	char cmd[MAX_TOKEN_CHARS];
	int  cnum;

	trap_Argv(1, cmd, sizeof (cmd));

	if (!*cmd) {
		G_Printf("usage: RemoveReferee <clientname>.");
		return;
	}

	cnum = G_refClientnumForName(NULL, cmd);

	if (cnum != MAX_CLIENTS) {
		if (level.clients[cnum].sess.referee == RL_REFEREE) {
			level.clients[cnum].sess.referee = RL_NONE;
			G_Printf("%s is no longer a referee.\n", cmd);
		} else {
			G_Printf("User is not a referee.\n");
		}
	}
}

/////////////////
//   Utility
//
int G_refClientnumForName(gentity_t *ent, const char *name) {
	char cleanName[MAX_TOKEN_CHARS];
	int  i;

	if (!*name) {
		return MAX_CLIENTS;
	}

	for (i = 0; i < level.numConnectedClients; ++i) {
		Q_strncpyz(cleanName, level.clients[level.sortedClients[i]].pers.netname, sizeof (cleanName));
		Q_CleanStr(cleanName);
		if (!Q_stricmp(cleanName, name)) {
			return level.sortedClients[i];
		}
	}

	G_refPrintf(ent, "Client not on server.");

	return MAX_CLIENTS;
}

void G_refPrintf(gentity_t *ent, const char *fmt, ...) {
	va_list argptr;
	char    text[1024];

	va_start(argptr, fmt);
	Q_vsnprintf(text, sizeof (text), fmt, argptr);
	va_end(argptr);

	if (ent == NULL) {
		// Nico, add linebreak to the string
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=047
		trap_Printf(va("%s\n", text));
	} else {
		// Nico, replaced cpm by print
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=046
		CP(va("print \"%s\n\"", text));
	}
}
