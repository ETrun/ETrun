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

// g_cmds_ext.c: Extended command set handling
// -------------------------------------------
//
#include "g_local.h"
#include "../../etrun/ui/menudef.h"

char *lock_status[2] = { "unlock", "lock" };

//
// Update info:
//	1. Add line to aCommandInfo w/appropriate info
//	2. Add implementation for specific command (see an existing command for an example)
//
typedef struct {
	char *pszCommandName;
	qboolean fValue;
	void ( *pCommand )( gentity_t *ent, unsigned int dwCommand, qboolean fValue );
	const char *pszHelpInfo;
} cmd_reference_t;

// VC optimizes for dup strings :)
static const cmd_reference_t aCommandInfo[] = {
	{ "?",				qtrue,  G_commands_cmd, ":^7 Gives a list of OSP-specific commands" },
	{ "autorecord",     qtrue,  NULL, ":^7 Creates a demo with a consistent naming scheme" },
	{ "autoscreenshot", qtrue,  NULL, ":^7 Creates a screenshot with a consistent naming scheme" },
	{ "callvote",       qfalse, ( void( * ) ( gentity_t *, unsigned int, qboolean ) )Cmd_CallVote_f, " <params>:^7 Calls a vote" },
	{ "commands",       qtrue,  G_commands_cmd, ":^7 Gives a list of OSP-specific commands" },
	{ "currenttime",	qtrue,  NULL, ":^7 Displays current local time" },
	{ "follow",			qtrue,  Cmd_Follow_f, " <player_ID|allies|axis>:^7 Spectates a particular player or team" },
	{ "players",		qtrue,  G_players_cmd, ":^7 Lists all active players and their IDs/information" },
	{ "ref",			qtrue,  G_ref_cmd, " <password>:^7 Become a referee (admin access)" },
	{ "specinvite",		qtrue,	Cmd_SpecInvite_f, ":^7 Invites a player to spectate" },
	{ "specuninvite",	qfalse,	Cmd_SpecInvite_f, ":^7 Uninvites a player to spectate" },
	{ "speclock",		qtrue,	Cmd_SpecLock_f, ":^7 Locks a player from spectators" },
	{ "specunlock",		qfalse,	Cmd_SpecLock_f, ":^7 Unlocks a player from spectators" },
	// Nico, stoprecord command was missing from commands list
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=012
	{ "stoprecord",		qtrue,	NULL, ":^7 Stops a demo recording currently in progress" },
	{ 0,                qtrue,  NULL, 0 }
};

// Nico, here are ignored commands, (no warning issued for them)
static cmd_reference_t ignoredServerCommands[] = {
	{"forcetapout",		qtrue, NULL, 0}
};

// OSP-specific Commands
qboolean G_commandCheck( gentity_t *ent, char *cmd ) {
	unsigned int i, cCommands = sizeof( aCommandInfo ) / sizeof( aCommandInfo[0] );
	const cmd_reference_t *pCR;

	for ( i = 0; i < cCommands; i++ ) {
		pCR = &aCommandInfo[i];
		if ( NULL != pCR->pCommand && 0 == Q_stricmp( cmd, pCR->pszCommandName ) ) {
			if ( !G_commandHelp( ent, cmd, i ) ) {
				pCR->pCommand( ent, i, pCR->fValue );
			}
			return( qtrue );
		}
	}

	// Nico, check ignored commands
	cCommands = sizeof(ignoredServerCommands) / sizeof (ignoredServerCommands[0]);
	for (i = 0; i < cCommands; ++i) {
		pCR = &ignoredServerCommands[i];
		if (!Q_stricmp(cmd, pCR->pszCommandName)) {
			// G_DPrintf("Ignoring client command: %s\n", cmd);
			return(qtrue);
		}
	}

	return (qfalse);
}


// Prints specific command help info.
qboolean G_commandHelp( gentity_t *ent, char *pszCommand, unsigned int dwCommand ) {
	char arg[MAX_TOKEN_CHARS];

	if ( !ent ) {
		return( qfalse );
	}
	trap_Argv( 1, arg, sizeof( arg ) );
	if ( !Q_stricmp( arg, "?" ) ) {
		CP( va( "print \"\n^3%s%s\n\n\"", pszCommand, aCommandInfo[dwCommand].pszHelpInfo ) );
		return( qtrue );
	}

	return( qfalse );
}


// Debounces cmd request as necessary.
qboolean G_cmdDebounce( gentity_t *ent, const char *pszCommandName ) {
	if ( ent->client->pers.cmd_debounce > level.time ) {
		CP( va( "print \"Wait another %.1fs to issue ^3%s\n\"", 1.0 * (float)( ent->client->pers.cmd_debounce - level.time ) / 1000.0,
				pszCommandName ) );
		return( qfalse );
	}

	ent->client->pers.cmd_debounce = level.time + CMD_DEBOUNCE;
	return( qtrue );
}


void G_noTeamControls( gentity_t *ent ) {
	CP( "cpm \"Team commands not enabled on this server.\n\"" );
}





////////////////////////////////////////////////////////////////////////////
/////
/////			Match Commands
/////
/////


// ************** COMMANDS / ?
//
// Lists server commands.
void G_commands_cmd( gentity_t *ent, unsigned int dwCommand, qboolean fValue ) {
	int i, rows, num_cmds = sizeof( aCommandInfo ) / sizeof( aCommandInfo[0] ) - 1;

	rows = num_cmds / HELP_COLUMNS;
	if ( num_cmds % HELP_COLUMNS ) {
		rows++;
	}
	if ( rows < 0 ) {
		return;
	}

	// Nico, replaced cpm by print
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=046
	CP( "print \"^5\nAvailable OSP Game-Commands:\n----------------------------\n\"" );
	for ( i = 0; i < rows; i++ ) {
		if ( i + rows * 3 + 1 <= num_cmds ) {
			CP( va( "print \"^3%-17s%-17s%-17s%-17s\n\"", aCommandInfo[i].pszCommandName,
					aCommandInfo[i + rows].pszCommandName,
					aCommandInfo[i + rows * 2].pszCommandName,
					aCommandInfo[i + rows * 3].pszCommandName ) );
		} else if ( i + rows * 2 + 1 <= num_cmds ) {
			CP( va( "print \"^3%-17s%-17s%-17s\n\"", aCommandInfo[i].pszCommandName,
					aCommandInfo[i + rows].pszCommandName,
					aCommandInfo[i + rows * 2].pszCommandName ) );
		} else if ( i + rows + 1 <= num_cmds ) {
			CP( va( "print \"^3%-17s%-17s\n\"", aCommandInfo[i].pszCommandName, aCommandInfo[i + rows].pszCommandName ) );
		} else {
			CP( va( "print \"^3%-17s\n\"", aCommandInfo[i].pszCommandName ) );
		}
	}
	// Nico, replaced cpm by print
	CP( "print \"\nType: ^3\\command_name ?^7 for more information\n\"" );
}

// ************** PLAYERS
//
// Show client info
void G_players_cmd( gentity_t *ent, unsigned int dwCommand, qboolean fValue ) {
	int i, idnum, max_rate, cnt = 0, tteam;
	int user_rate, user_snaps;
	gclient_t *cl;
	gentity_t *cl_ent;
	char n2[MAX_NETNAME], ready[16], ref[16], rate[256];
	char *s, *tc, *coach, userinfo[MAX_INFO_STRING];


	if ( g_gamestate.integer == GS_PLAYING ) {
		if ( ent ) {
			CP( "print \"\n^3 ID^1 : ^3Player                    Nudge  Rate  MaxPkts  Snaps\n\"" );
			CP(  "print \"^1-----------------------------------------------------------^7\n\"" );
		} else {
			G_Printf( " ID : Player                    Nudge  Rate  MaxPkts  Snaps\n" );
			G_Printf( "-----------------------------------------------------------\n" );
		}
	} else {
		if ( ent ) {
			CP( "print \"\n^3Status^1   : ^3ID^1 : ^3Player                    Nudge  Rate  MaxPkts  Snaps\n\"" );
			CP(  "print \"^1---------------------------------------------------------------------^7\n\"" );
		} else {
			G_Printf( "Status   : ID : Player                    Nudge  Rate  MaxPkts  Snaps\n" );
			G_Printf( "---------------------------------------------------------------------\n" );
		}
	}

	max_rate = trap_Cvar_VariableIntegerValue( "sv_maxrate" );

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		idnum = level.sortedClients[i]; //level.sortedNames[i];
		cl = &level.clients[idnum];
		cl_ent = g_entities + idnum;

		SanitizeString( cl->pers.netname, n2, qtrue );
		n2[26] = 0;
		ref[0] = 0;
		ready[0] = 0;

		// Rate info
		if ( cl_ent->r.svFlags & SVF_BOT ) {
			strcpy( rate, va( "%s%s%s%s", "[BOT]", " -----", "       --", "     --" ) );
		} else if ( cl->pers.connected == CON_CONNECTING ) {
			strcpy( rate, va( "%s", "^3>>> CONNECTING <<<" ) );
		} else {
			trap_GetUserinfo( idnum, userinfo, sizeof( userinfo ) );
			s = Info_ValueForKey( userinfo, "rate" );
			user_rate = ( max_rate > 0 && atoi( s ) > max_rate ) ? max_rate : atoi( s );
			s = Info_ValueForKey( userinfo, "snaps" );
			user_snaps = atoi( s );

			strcpy( rate, va( "%5d%6d%9d%7d", cl->pers.clientTimeNudge, user_rate, cl->pers.clientMaxPackets, user_snaps ) );
		}

		if ( g_gamestate.integer != GS_PLAYING ) {
			if ( cl->sess.sessionTeam == TEAM_SPECTATOR || cl->pers.connected == CON_CONNECTING ) {
				strcpy( ready, ( ( ent ) ? "^5--------^1 :" : "-------- :" ) );
			} else if ( cl->pers.ready || ( g_entities[idnum].r.svFlags & SVF_BOT ) ) {
				strcpy( ready, ( ( ent ) ? "^3(READY)^1  :" : "(READY)  :" ) );
			} else {
				strcpy( ready, ( ( ent ) ? "NOTREADY^1 :" : "NOTREADY :" ) );
			}
		}

		if ( cl->sess.referee ) {
			strcpy( ref, "REF" );
		}

		if ( cl->sess.coach_team ) {
			tteam = cl->sess.coach_team;
			coach = ( ent ) ? "^3C" : "C";
		} else {
			tteam = cl->sess.sessionTeam;
			coach = " ";
		}

		tc = ( ent ) ? "^7 " : " ";
		if ( tteam == TEAM_AXIS ) {
			tc = ( ent ) ? "^1X^7" : "X";
		}
		if ( tteam == TEAM_ALLIES ) {
			tc = ( ent ) ? "^4L^7" : "L";
		}

		if ( ent ) {
			CP( va( "print \"%s%s%2d%s^1:%s %-26s^7%s  ^3%s\n\"", ready, tc, idnum, coach, ( ( ref[0] ) ? "^3" : "^7" ), n2, rate, ref ) );
		} else { G_Printf( "%s%s%2d%s: %-26s%s  %s\n", ready, tc, idnum, coach, n2, rate, ref );}

		cnt++;
	}

	if ( ent ) {
		CP( va( "print \"\n^3%2d^7 total players\n\n\"", cnt ) );
	} else { G_Printf( "\n%2d total players\n\n", cnt );}
}
