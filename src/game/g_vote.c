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

// g_vote.c: All callvote handling
// -------------------------------
//
#include "g_local.h"
#include "../../etrun/ui/menudef.h" // For vote options
#include "g_api.h"

#define T_FFA   0x01
#define T_1V1   0x02
#define T_SP    0x04
#define T_TDM   0x08
#define T_CTF   0x10
#define T_WLF   0x20
#define T_WSW   0x40
#define T_WCP   0x80
#define T_WCH   0x100


static const char *ACTIVATED = "ACTIVATED";
static const char *DEACTIVATED = "DEACTIVATED";
static const char *ENABLED = "ENABLED";
static const char *DISABLED = "DISABLED";

//
// Update info:
//	1. Add line to aVoteInfo w/appropriate info
//	2. Add implementation for attempt and success (see an existing command for an example)
//
typedef struct {
	unsigned int dwGameTypes;
	const char *pszVoteName;
	int ( *pVoteCommand )( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd );
	const char *pszVoteMessage;
	const char *pszVoteHelp;
} vote_reference_t;

// VC optimizes for dup strings :)
static const vote_reference_t aVoteInfo[] = {
	{ 0x1ff, "kick",		G_Kick_v,			"KICK",             " <player_id>^7\n  Attempts to kick player from server" },
	{ 0x1ff, "mute",		G_Mute_v,			"MUTE",             " <player_id>^7\n  Removes the chat capabilities of a player" },
	{ 0x1ff, "unmute",      G_UnMute_v,			"UN-MUTE",          " <player_id>^7\n  Restores the chat capabilities of a player" },
	{ 0x1ff, "map",         G_Map_v,			"Change map to",	" <mapname>^7\n  Votes for a new map to be loaded" },
	{ 0x1ff, "maprestart",  G_MapRestart_v,		"Map Restart",      "^7\n  Restarts the current map in progress" },
	{ 0x1ff, "matchreset",  G_MatchReset_v,		"Match Reset",      "^7\n  Resets the entire match" },
	{ 0x1ff, "randommap",   G_Randommap_v,      "Load Random Map",	"^7\n  Loads a random map" },
	{ 0x1ff, "referee",     G_Referee_v,		"Referee",          " <player_id>^7\n  Elects a player to have admin abilities" },
	{ 0x1ff, "startmatch",  G_StartMatch_v,		"Start Match",      " ^7\n  Sets all players to \"ready\" status to start the match" },
	{ 0x1ff, "unreferee",   G_Unreferee_v,		"UNReferee",		" <player_id>^7\n  Elects a player to have admin abilities removed" },
	{ 0x1ff, "antilag",     G_AntiLag_v,		"Anti-Lag",         " <0|1>^7\n  Toggles Anit-Lag on the server" },
	{ 0, 0, NULL, 0, NULL }
};


// Checks for valid custom callvote requests from the client.
int G_voteCmdCheck( gentity_t *ent, char *arg, char *arg2, qboolean fRefereeCmd ) {
	unsigned int i, cVoteCommands = sizeof( aVoteInfo ) / sizeof( aVoteInfo[0] );

	for ( i = 0; i < cVoteCommands; i++ ) {
		if ( !Q_stricmp( arg, aVoteInfo[i].pszVoteName ) ) {
			int hResult = aVoteInfo[i].pVoteCommand( ent, i, arg, arg2, fRefereeCmd );

			if ( hResult == G_OK ) {
                // Nico, string format bug security fix
				Com_sprintf( arg, VOTE_MAXSTRING, "%s", aVoteInfo[i].pszVoteMessage );
				level.voteInfo.vote_fn = aVoteInfo[i].pVoteCommand;
			} else {
				level.voteInfo.vote_fn = NULL;
			}

			return( hResult );
		}
	}

	return( G_NOTFOUND );
}


// Voting help summary.
void G_voteHelp( gentity_t *ent, qboolean fShowVote ) {
	int i, rows = 0, num_cmds = sizeof( aVoteInfo ) / sizeof( aVoteInfo[0] ) - 1; // Remove terminator;
	int vi[100];            // Just make it large static.


	if ( fShowVote ) {
		CP( "print \"\nValid ^3callvote^7 commands are:\n^3----------------------------\n\"" );
	}

	for ( i = 0; i < num_cmds; i++ ) {
		vi[rows++] = i;
	}

	num_cmds = rows;
	rows = num_cmds / HELP_COLUMNS;

	if ( num_cmds % HELP_COLUMNS ) {
		rows++;
	}
	if ( rows < 0 ) {
		return;
	}

	for ( i = 0; i < rows; i++ ) {
		if ( i + rows * 3 + 1 <= num_cmds ) {
			G_refPrintf( ent, "^5%-17s%-17s%-17s%-17s", aVoteInfo[vi[i]].pszVoteName,
						 aVoteInfo[vi[i + rows]].pszVoteName,
						 aVoteInfo[vi[i + rows * 2]].pszVoteName,
						 aVoteInfo[vi[i + rows * 3]].pszVoteName );
		} else if ( i + rows * 2 + 1 <= num_cmds ) {
			G_refPrintf( ent, "^5%-17s%-17s%-17s", aVoteInfo[vi[i]].pszVoteName,
						 aVoteInfo[vi[i + rows]].pszVoteName,
						 aVoteInfo[vi[i + rows * 2]].pszVoteName );
		} else if ( i + rows + 1 <= num_cmds ) {
			G_refPrintf( ent, "^5%-17s%-17s", aVoteInfo[vi[i]].pszVoteName,
						 aVoteInfo[vi[i + rows]].pszVoteName );
		} else {
			G_refPrintf( ent, "^5%-17s", aVoteInfo[vi[i]].pszVoteName );
		}
	}

	if ( fShowVote ) {
		CP( "print \"\nUsage: ^3\\callvote <command> <params>\n^7For current settings/help, use: ^3\\callvote <command> ?\n\n\"" );
	}

	return;
}

// Set disabled votes for client UI
void G_voteFlags( void ) {
	int i, flags = 0;

	for ( i = 0; i < numVotesAvailable; i++ ) {
		if ( trap_Cvar_VariableIntegerValue( voteToggles[i].pszCvar ) == 0 ) {
			flags |= voteToggles[i].flag;
		}
	}

	if ( flags != voteFlags.integer ) {
		trap_Cvar_Set( "voteFlags", va( "%d", flags ) );
	}
}

// Prints specific callvote command help description.
qboolean G_voteDescription( gentity_t *ent, qboolean fRefereeCmd, int cmd ) {
	char arg[MAX_TOKEN_CHARS];
	char *ref_cmd = ( fRefereeCmd ) ? "\\ref" : "\\callvote";

	if ( !ent ) {
		return( qfalse );
	}

	trap_Argv( 2, arg, sizeof( arg ) );
	if ( !Q_stricmp( arg, "?" ) || trap_Argc() == 2 ) {
		trap_Argv( 1, arg, sizeof( arg ) );
		G_refPrintf( ent, "\nUsage: ^3%s %s%s\n", ref_cmd, arg, aVoteInfo[cmd].pszVoteHelp );
		return( qtrue );
	}

	return( qfalse );
}


// Localize disable message info.
void G_voteDisableMessage( gentity_t *ent, const char *cmd ) {
	G_refPrintf( ent, "Sorry, [lof]^3%s^7 [lon]voting has been disabled", cmd );
}


// Player ID message stub.
void G_playersMessage( gentity_t *ent ) {
	G_refPrintf( ent, "Use the ^3players^7 command to find a valid player ID." );
}


// Localize current parameter setting.
void G_voteCurrentSetting( gentity_t *ent, const char *cmd, const char *setting ) {
	G_refPrintf( ent, "^2%s^7 is currently ^3%s\n", cmd, setting );
}


// Vote toggling
int G_voteProcessOnOff( gentity_t *ent, char *arg, char *arg2, qboolean fRefereeCmd, int curr_setting, int vote_allow, int vote_type ) {
	// Nico, silent GCC
	arg = arg;

	if ( !vote_allow && ent && !ent->client->sess.referee ) {
		G_voteDisableMessage( ent, aVoteInfo[vote_type].pszVoteName );
		G_voteCurrentSetting( ent, aVoteInfo[vote_type].pszVoteName, ( ( curr_setting ) ? ENABLED : DISABLED ) );
		return( G_INVALID );
	}
	if ( G_voteDescription( ent, fRefereeCmd, vote_type ) ) {
		G_voteCurrentSetting( ent, aVoteInfo[vote_type].pszVoteName, ( ( curr_setting ) ? ENABLED : DISABLED ) );
		return( G_INVALID );
	}

	if ( ( atoi( arg2 ) && curr_setting ) || ( !atoi( arg2 ) && !curr_setting ) ) {
		G_refPrintf( ent, "^3%s^5 is already %s!", aVoteInfo[vote_type].pszVoteName, ( ( curr_setting ) ? ENABLED : DISABLED ) );
		return( G_INVALID );
	}

	Com_sprintf( level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2 );
	Com_sprintf( arg2, VOTE_MAXSTRING, "%s", ( atoi( arg2 ) ) ? ACTIVATED : DEACTIVATED );

	return( G_OK );
}


//
// Several commands to help with cvar setting
//
void G_voteSetOnOff( const char *desc, const char *cvar ) {
	AP( va( "cpm \"^3%s is: ^5%s\n\"", desc, ( atoi( level.voteInfo.vote_value ) ) ? ENABLED : DISABLED ) );
	//trap_SendConsoleCommand(EXEC_APPEND, va("%s %s\n", cvar, level.voteInfo.vote_value));
	trap_Cvar_Set( cvar, level.voteInfo.vote_value );
}

void G_voteSetValue( const char *desc, const char *cvar ) {
	AP( va( "cpm \"^3%s set to: ^5%s\n\"", desc, level.voteInfo.vote_value ) );
	//trap_SendConsoleCommand(EXEC_APPEND, va("%s %s\n", cvar, level.voteInfo.vote_value));
	trap_Cvar_Set( cvar, level.voteInfo.vote_value );
}

void G_voteSetVoteString( const char *desc ) {
	AP( va( "print \"^3%s set to: ^5%s\n\"", desc, level.voteInfo.vote_value ) );
	trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteInfo.voteString ) );
}

// *** Player Kick ***
int G_Kick_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	// Vote request (vote is being initiated)
	if ( arg ) {
		int pid;

		if ( !vote_allow_kick.integer && ent && !ent->client->sess.referee ) {
			G_voteDisableMessage( ent, arg );
			return G_INVALID;
		} else if ( G_voteDescription( ent, fRefereeCmd, dwVoteIndex ) ) {
			return G_INVALID;
		} else if ( ( pid = ClientNumberFromString( ent, arg2 ) ) == -1 ) {
			return G_INVALID;
		}

		if ( level.clients[ pid ].sess.referee ) {
			G_refPrintf( ent, "Can't vote to kick referees!" );
			return G_INVALID;
		}

		if ( !fRefereeCmd && ent ) {
			if ( level.clients[ pid ].sess.sessionTeam != TEAM_SPECTATOR && level.clients[ pid ].sess.sessionTeam != ent->client->sess.sessionTeam ) {
				G_refPrintf( ent, "Can't vote to kick players on opposing team!" );
				return G_INVALID;
			}
		}

		Com_sprintf( level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid );
		Com_sprintf( arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname );

		// Vote action (vote has passed)
	} else {
		// Kick a player
		trap_SendConsoleCommand( EXEC_APPEND, va( "clientkick %d\n", atoi( level.voteInfo.vote_value ) ) );
		AP( va( "cp \"%s\n^3has been kicked!\n\"", level.clients[ atoi( level.voteInfo.vote_value ) ].pers.netname ) );
	}

	return G_OK;
}

// *** Player Mute ***
int G_Mute_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	if ( fRefereeCmd ) {
		// handled elsewhere
		return( G_NOTFOUND );
	}

	// Vote request (vote is being initiated)
	if ( arg ) {
		int pid;

		if ( !vote_allow_muting.integer && ent && !ent->client->sess.referee ) {
			G_voteDisableMessage( ent, arg );
			return( G_INVALID );
		} else if ( G_voteDescription( ent, fRefereeCmd, dwVoteIndex ) ) {
			return( G_INVALID );
		} else if ( ( pid = ClientNumberFromString( ent, arg2 ) ) == -1 ) {
			return( G_INVALID );
		}

		if ( level.clients[pid].sess.referee ) {
			G_refPrintf( ent, "Can't vote to mute referees!" );
			return( G_INVALID );
		}

		if ( level.clients[pid].sess.muted ) {
			G_refPrintf( ent, "Player is already muted!" );
			return( G_INVALID );
		}

		Com_sprintf( level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid );
		Com_sprintf( arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname );

		// Vote action (vote has passed)
	} else {
		int pid = atoi( level.voteInfo.vote_value );

		// Mute a player
		if ( level.clients[pid].sess.referee != RL_RCON ) {
			trap_SendServerCommand( pid, va( "cpm \"^3You have been muted\"" ) );
			level.clients[pid].sess.muted = qtrue;
			AP( va( "cp \"%s\n^3has been muted!\n\"", level.clients[pid].pers.netname ) );
			ClientUserinfoChanged( pid );
		} else {
			G_Printf( "Cannot mute a referee.\n" );
		}
	}

	return( G_OK );
}

// *** Player Un-Mute ***
int G_UnMute_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	if ( fRefereeCmd ) {
		// handled elsewhere
		return( G_NOTFOUND );
	}

	// Vote request (vote is being initiated)
	if ( arg ) {
		int pid;

		if ( !vote_allow_muting.integer && ent && !ent->client->sess.referee ) {
			G_voteDisableMessage( ent, arg );
			return( G_INVALID );
		} else if ( G_voteDescription( ent, fRefereeCmd, dwVoteIndex ) ) {
			return( G_INVALID );
		} else if ( ( pid = ClientNumberFromString( ent, arg2 ) ) == -1 ) {
			return( G_INVALID );
		}

		if ( !level.clients[pid].sess.muted ) {
			G_refPrintf( ent, "Player is not muted!" );
			return( G_INVALID );
		}

		Com_sprintf( level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid );
		Com_sprintf( arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname );

		// Vote action (vote has passed)
	} else {
		int pid = atoi( level.voteInfo.vote_value );

		// Mute a player
		if ( level.clients[pid].sess.referee != RL_RCON ) {
			trap_SendServerCommand( pid, va( "cpm \"^3You have been un-muted\"" ) );
			level.clients[pid].sess.muted = qfalse;
			AP( va( "cp \"%s\n^3has been un-muted!\n\"", level.clients[pid].pers.netname ) );
			ClientUserinfoChanged( pid );
		} else {
			G_Printf( "Cannot un-mute a referee.\n" );
		}
	}

	return( G_OK );
}

// Nico, function to delay a map change
void G_delay_map_change(char *mapName) {
	int i = 0;
	int activeRunsCount = 0;
	gclient_t *cl = NULL;

	Q_strncpyz(level.delayedMapChange.passedVote, mapName, VOTE_MAXSTRING);

	// Nico, if no timerun is currenlty active or if player is alone on the server
	// change the map in 1 sec, otherwise wait MAP_CHANGE_DELAY
	for (i = 0; i < level.numConnectedClients; ++i) {
		cl = &level.clients[level.sortedClients[i]];
		if ((cl->sess.sessionTeam == TEAM_ALLIES || cl->sess.sessionTeam == TEAM_AXIS) && cl->sess.timerunActive) {
			activeRunsCount++;
		}
	}
	if (level.numConnectedClients > 1 && activeRunsCount > 0) {
		level.delayedMapChange.timeChange = level.time + MAP_CHANGE_DELAY * 1000;
		AP(va("cpm \"^5Map will be changed in %dsecs\n\"", MAP_CHANGE_DELAY));
	} else {
		level.delayedMapChange.timeChange = level.time + 1000;
	}

	level.delayedMapChange.pendingChange = qtrue;
}

// Nico, delayed map change check function (thread)
void *G_delayed_map_change_watcher(void *arg) {
	int count = 0;
	int limit = 10;// Nico, in seconds

	// Nico, silent GCC
	arg = arg;

	while (!level.delayedMapChange.disabledWatcher) {
		if (level.time && level.delayedMapChange.timeChange) {
			// There is a delayed change

			if (level.time >= level.delayedMapChange.timeChange) {
				// Nico, useless: level.delayedMapChange.pendingChange = qfalse;

				// Nico, do we have to wait for some threads to finish their work?
				while (activeThreadsCounter > 0 && count < limit) {
					G_DPrintf("Waiting for %d thread(s) before changing map\n", activeThreadsCounter);
					my_sleep(1000); // Nico, sleep for 1sec
					count++;
				}

				if (count >= limit) {
					G_Error("Warning: threads waiting timeout reached (threads: %d)", activeThreadsCounter);
				}
				G_DPrintf("Changing map now!\n");
				Svcmd_ResetMatch_f( qtrue, qfalse );
				trap_SendConsoleCommand( EXEC_APPEND, va( "map %s\n", level.delayedMapChange.passedVote ) );
				break;
			} else {
				// Print remaining time each sec
				G_DPrintf("Map change in: %d msecs\n", level.delayedMapChange.timeChange - level.time);
			}
		}
		my_sleep(500);
	}
	return NULL;
}

// *** Map - simpleton: we dont verify map is allowed/exists ***
int G_Map_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	// Vote request (vote is being initiated)
	if ( arg ) {
		char serverinfo[MAX_INFO_STRING];
		trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

		if ( !vote_allow_map.integer && ent && !ent->client->sess.referee ) {
			G_voteDisableMessage( ent, arg );
			G_voteCurrentSetting( ent, arg, Info_ValueForKey( serverinfo, "mapname" ) );
			return( G_INVALID );
		} else if ( G_voteDescription( ent, fRefereeCmd, dwVoteIndex ) ) {
			G_voteCurrentSetting( ent, arg, Info_ValueForKey( serverinfo, "mapname" ) );
			return( G_INVALID );
		}

		Com_sprintf( level.voteInfo.vote_value, VOTE_MAXSTRING, "%s", arg2 );

		// Vote action (vote has passed)
	} else {
		// Nico, delay the map change
		G_delay_map_change(level.voteInfo.vote_value);
	}

	return( G_OK );
}

// *** Map Restart ***
int G_MapRestart_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	// Vote request (vote is being initiated)
	if ( arg ) {
		if ( trap_Argc() > 2 ) {
			if ( !Q_stricmp( arg2, "?" ) ) {
				G_refPrintf( ent, "Usage: ^3%s %s%s\n", ( ( fRefereeCmd ) ? "\\ref" : "\\callvote" ), arg, aVoteInfo[dwVoteIndex].pszVoteHelp );
				return( G_INVALID );
			}
		}

		// Vote action (vote has passed)
	} else {
		// Restart the map back to warmup
		Svcmd_ResetMatch_f( qfalse, qtrue );
		AP( "cp \"^1*** Level Restarted! ***\n\"" );
	}

	return( G_OK );
}


// *** Match Restart ***
int G_MatchReset_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	// Nico, silent GCC
	arg2 = arg2;

	// Vote request (vote is being initiated)
	if ( arg ) {
		if ( !vote_allow_matchreset.integer && ent && !ent->client->sess.referee ) {
			G_voteDisableMessage( ent, arg );
			return( G_INVALID );
		} else if ( trap_Argc() != 2 && G_voteDescription( ent, fRefereeCmd, dwVoteIndex ) ) {
			return( G_INVALID );
		}

		// Vote action (vote has passed)
	} else {
		// Restart the map back to warmup
		Svcmd_ResetMatch_f( qtrue, qtrue );
		AP( "cp \"^1*** Match Reset! ***\n\"" );
	}

	return( G_OK );
}

/**
 * Random map
 */
int G_Randommap_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	char *result = NULL;

	// Nico, silent GCC
	arg2 = arg2;

	// Nico, check if API is used
	if (!g_useAPI.integer) {
		G_Printf("API is disabled on this server.\n");
		return G_INVALID;
	}

	// Vote request (vote is being initiated)
	if ( arg ) {
		if ( trap_Argc() > 2 ) {
			G_refPrintf( ent, "Usage: ^3%s %s%s\n", ( ( fRefereeCmd ) ? "\\ref" : "\\callvote" ), arg, aVoteInfo[dwVoteIndex].pszVoteHelp );
			return( G_INVALID );
		} else if ( !vote_allow_randommap.integer && ent && !ent->client->sess.referee ) {
			G_voteDisableMessage( ent, arg );
			return( G_INVALID );
		}

		// Vote action (vote has passed)
	} else {
		AP( "cp \"Loading a random map!\n\"" );

		result = malloc(RESPONSE_MAX_SIZE * sizeof (char));

		if (!result) {
			G_Error("G_Randommap_v: malloc failed\n");
		}

		G_API_randommap(result, ent, level.rawmapname);
	}
	return( G_OK );
}

// *** Referee voting ***
int G_Referee_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	// Vote request (vote is being initiated)
	if ( arg ) {
		int pid;

		if ( !vote_allow_referee.integer && ent && !ent->client->sess.referee ) {
			G_voteDisableMessage( ent, arg );
			return( G_INVALID );
		}

		if ( !ent->client->sess.referee && level.numPlayingClients < 3 ) {
			G_refPrintf( ent, "Sorry, not enough clients in the game to vote for a referee" );
			return( G_INVALID );
		}

		if ( ent->client->sess.referee && trap_Argc() == 2 ) {
			G_playersMessage( ent );
			return( G_INVALID );
		} else if ( trap_Argc() == 2 ) {
			pid = ent - g_entities;
		} else if ( G_voteDescription( ent, fRefereeCmd, dwVoteIndex ) ) {
			return( G_INVALID );
		} else if ( ( pid = ClientNumberFromString( ent, arg2 ) ) == -1 )                                                                                        {
			return( G_INVALID );
		}

		if ( level.clients[pid].sess.referee ) {
			G_refPrintf( ent, "[lof]%s [lon]is already a referee!", level.clients[pid].pers.netname );
			return( -1 );
		}

		Com_sprintf( level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid );
		Com_sprintf( arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname );

		// Vote action (vote has passed)
	} else {
		// Voting in a new referee
		gclient_t *cl = &level.clients[atoi( level.voteInfo.vote_value )];

		if ( cl->pers.connected == CON_DISCONNECTED ) {
			AP( "print \"Player left before becoming referee\n\"" );
		} else {
			cl->sess.referee = RL_REFEREE;  // FIXME: Differentiate voted refs from passworded refs
			AP( va( "cp \"%s^7 is now a referee\n\"", cl->pers.netname ) );
			ClientUserinfoChanged( atoi( level.voteInfo.vote_value ) );
		}
	}
	return( G_OK );
}

// *** Start Match ***
int G_StartMatch_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	// Vote request (vote is being initiated)
	if ( arg ) {
		if ( trap_Argc() > 2 ) {
			if ( !Q_stricmp( arg2, "?" ) ) {
				G_refPrintf( ent, "Usage: ^3%s %s%s\n", ( ( fRefereeCmd ) ? "\\ref" : "\\callvote" ), arg, aVoteInfo[dwVoteIndex].pszVoteHelp );
				return( G_INVALID );
			}
		}

		if ( g_gamestate.integer == GS_PLAYING ) {
			G_refPrintf( ent, "^3Match is already in progress!" );
			return( G_INVALID );
		}
		// Vote action (vote has passed)
	}

	return( G_OK );
}

// Anti-Lag
int G_AntiLag_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	// Vote request (vote is being initiated)
	if ( arg ) {
		return( G_voteProcessOnOff( ent, arg, arg2, fRefereeCmd,
									!!( g_antilag.integer ),
									vote_allow_antilag.integer,
									dwVoteIndex ) );

		// Vote action (vote has passed)
	} else {
		// Anti-Lag (g_antilag)
		G_voteSetOnOff( "Anti-Lag", "g_antilag" );
	}

	return( G_OK );
}

// *** Un-Referee voting ***
int G_Unreferee_v( gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd ) {
	// Vote request (vote is being initiated)
	if ( arg ) {
		int pid;

		if ( !vote_allow_referee.integer && ent && !ent->client->sess.referee ) {
			G_voteDisableMessage( ent, arg );
			return( G_INVALID );
		}

		if ( ent->client->sess.referee && trap_Argc() == 2 ) {
			G_playersMessage( ent );
			return( G_INVALID );
		} else if ( trap_Argc() == 2 ) {
			pid = ent - g_entities;
		} else if ( G_voteDescription( ent, fRefereeCmd, dwVoteIndex ) ) {
			return( G_INVALID );
		} else if ( ( pid = ClientNumberFromString( ent, arg2 ) ) == -1 )                                                                                        {
			return( G_INVALID );
		}

		if ( level.clients[pid].sess.referee == RL_NONE ) {
			G_refPrintf( ent, "[lof]%s [lon]isn't a referee!", level.clients[pid].pers.netname );
			return( G_INVALID );
		}

		if ( level.clients[pid].sess.referee == RL_RCON ) {
			G_refPrintf( ent, "[lof]%s's [lon]status cannot be removed", level.clients[pid].pers.netname );
			return( G_INVALID );
		}

		if ( level.clients[pid].pers.localClient ) {
			G_refPrintf( ent, "[lof]%s's [lon]is the Server Host", level.clients[pid].pers.netname );
			return( G_INVALID );
		}

		Com_sprintf( level.voteInfo.vote_value, VOTE_MAXSTRING, "%d", pid );
		Com_sprintf( arg2, VOTE_MAXSTRING, "%s", level.clients[pid].pers.netname );

		// Vote action (vote has passed)
	} else {
		// Stripping of referee status
		gclient_t *cl = &level.clients[atoi( level.voteInfo.vote_value )];

		cl->sess.referee = RL_NONE;
		AP( va( "cp \"%s^7\nis no longer a referee\n\"", cl->pers.netname ) );
		ClientUserinfoChanged( atoi( level.voteInfo.vote_value ) );
	}

	return( G_OK );
}
