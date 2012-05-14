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

#include "g_local.h"
#include "g_api.h"

qboolean G_IsOnFireteam( int entityNum, fireteamData_t** teamNum );

/*
==================
G_SendScore

Sends current scoreboard information
==================
*/
void G_SendScore( gentity_t *ent ) {
	char entry[128];
	int i;
	gclient_t   *cl;
	int numSorted;
	int team, size, count;
	char buffer[1024];
	char startbuffer[32];

	// send the latest information on all clients
	numSorted = level.numConnectedClients;

	// Nico, replaced hardcoded value by a define
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=068
	if ( numSorted > MAX_CLIENTS ) {
		numSorted = MAX_CLIENTS;
	}

	i = 0;
	// Gordon: team doesnt actually mean team, ignore...
	for ( team = 0; team < 2; team++ ) {
		*buffer = '\0';
		*startbuffer = '\0';
		if ( team == 0 ) {
			Q_strncpyz( startbuffer, "sc0", 32 );
		} else {
			Q_strncpyz( startbuffer, "sc1", 32 );
		}
		size = strlen( startbuffer ) + 1;
		count = 0;

		for (; i < numSorted ; i++ ) {
			int ping, playerClass;

			cl = &level.clients[level.sortedClients[i]];

			if ( g_entities[level.sortedClients[i]].r.svFlags & SVF_POW ) {
				continue;
			}

			// NERVE - SMF - if on same team, send across player class
			// Gordon: FIXME: remove/move elsewhere?
			if (cl->ps.persistant[PERS_TEAM] == ent->client->ps.persistant[PERS_TEAM]) {
				playerClass = cl->ps.stats[STAT_PLAYER_CLASS];
			} else {
				playerClass = 0;
			}

			if ( cl->pers.connected == CON_CONNECTING ) {
				ping = -1;
			} else {
				ping = cl->ps.ping < 999 ? cl->ps.ping : 999;
			}

			// Nico, added timerun best time, timerun best speed, timerun status, followed client
			Com_sprintf( entry, sizeof( entry ), " %i %i %i %i %i %i %i %i %i",
				level.sortedClients[i], 
				ping,
				( level.time - cl->pers.enterTime ) / 60000, 
				g_entities[level.sortedClients[i]].s.powerups, 
				playerClass, 
				cl->sess.timerunBestTime[0], 
				cl->sess.timerunBestSpeed, 
				cl->timerunActive ? 1 : 0,
				cl->ps.clientNum);

			if ( size + strlen( entry ) > 1000 ) {
				i--; // we need to redo this client in the next buffer (if we can)
				break;
			}
			size += strlen( entry );

			Q_strcat( buffer, 1024, entry );
			if ( ++count >= 32 ) {
				i--; // we need to redo this client in the next buffer (if we can)
				break;
			}
		}

		if ( count > 0 || team == 0 ) {
			trap_SendServerCommand( ent - g_entities, va( "%s %i%s", startbuffer, count, buffer ) );
		}
	}
}

/*
==================
Cmd_Score_f

Request current scoreboard information
==================
*/
void Cmd_Score_f( gentity_t *ent ) {
	ent->client->wantsscore = qtrue;
}

/*
==================
ConcatArgs
==================
*/
char    *ConcatArgs( int start ) {
	int i, c, tlen;
	static char line[MAX_STRING_CHARS];
	int len;
	char arg[MAX_STRING_CHARS];

	len = 0;
	c = trap_Argc();
	for ( i = start ; i < c ; i++ ) {
		trap_Argv( i, arg, sizeof( arg ) );
		tlen = strlen( arg );
		if ( len + tlen >= MAX_STRING_CHARS - 1 ) {
			break;
		}
		memcpy( line + len, arg, tlen );
		len += tlen;
		if ( i != c - 1 ) {
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}

/*
==================
SanitizeString

Remove case and control characters
==================
*/
void SanitizeString( char *in, char *out, qboolean fToLower ) {
	while ( *in ) {
		if ( *in == 27 || *in == '^' ) {
			in++;       // skip color code
			if ( *in ) {
				in++;
			}
			continue;
		}

		if ( *in < 32 ) {
			in++;
			continue;
		}

		*out++ = ( fToLower ) ? tolower( *in++ ) : *in++;
	}

	*out = 0;
}

/*
==================
ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 if invalid
==================
*/
int ClientNumberFromString( gentity_t *to, char *s ) {
	gclient_t   *cl;
	int idnum;
	char s2[MAX_STRING_CHARS];
	char n2[MAX_STRING_CHARS];
	qboolean fIsNumber = qtrue;

	// See if its a number or string
	for ( idnum = 0; idnum < strlen( s ) && s[idnum] != 0; idnum++ ) {
		if ( s[idnum] < '0' || s[idnum] > '9' ) {
			fIsNumber = qfalse;
			break;
		}
	}

	// check for a name match
	SanitizeString( s, s2, qtrue );
	for ( idnum = 0, cl = level.clients; idnum < level.maxclients; idnum++, cl++ ) {
		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}

		SanitizeString( cl->pers.netname, n2, qtrue );
		if ( !strcmp( n2, s2 ) ) {
			return( idnum );
		}
	}

	// numeric values are just slot numbers
	if ( fIsNumber ) {
		idnum = atoi( s );
		if ( idnum < 0 || idnum >= level.maxclients ) {
			CPx( to - g_entities, va( "print \"Bad client slot: [lof]%i\n\"", idnum ) );
			return -1;
		}

		cl = &level.clients[idnum];
		if ( cl->pers.connected != CON_CONNECTED ) {
			CPx( to - g_entities, va( "print \"Client[lof] %i [lon]is not active\n\"", idnum ) );
			return -1;
		}
		return( idnum );
	}

	CPx( to - g_entities, va( "print \"User [lof]%s [lon]is not on the server\n\"", s ) );
	return( -1 );
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent ) {
	char    *msg;

	char    *name = ConcatArgs( 1 );

	// Nico, only available if client is not logged in
	if (ent->client->sess.logged) {
		trap_SendServerCommand( ent - g_entities, va( "print \"You must /logout to use this command.\n\"" ) );
		return;
	}

	if ( ent->health <= 0 ) {
		trap_SendServerCommand( ent - g_entities, va( "print \"You must be alive to use this command.\n\"" ) );
		return;
	}

	if ( !Q_stricmp( name, "on" ) || atoi( name ) ) {
		ent->client->noclip = qtrue;
	} else if ( !Q_stricmp( name, "off" ) || !Q_stricmp( name, "0" ) ) {
		ent->client->noclip = qfalse;
	} else {
		ent->client->noclip = !ent->client->noclip;
	}

	if ( ent->client->noclip ) {
		msg = "noclip ON\n";
	} else {
		msg = "noclip OFF\n";
	}

	trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ||
		 ( ent->client->ps.pm_flags & PMF_LIMBO ) ||
		 ent->health <= 0 || level.match_pause != PAUSE_NONE ) {
		return;
	}

	ent->flags &= ~FL_GODMODE;
	ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
	ent->client->ps.persistant[PERS_HWEAPON_USE] = 0; // TTimo - if using /kill while at MG42
	player_die( ent, ent, ent, ( g_gamestate.integer == GS_PLAYING ) ? 100000 : 135, MOD_SUICIDE );
}

void G_TeamDataForString( const char* teamstr, int clientNum, team_t* team, spectatorState_t* sState, int* specClient ) {
	*sState = SPECTATOR_NOT;
	if ( !Q_stricmp( teamstr, "follow1" ) ) {
		*team =     TEAM_SPECTATOR;
		*sState =   SPECTATOR_FOLLOW;
		if ( specClient ) {
			*specClient = -1;
		}
	} else if ( !Q_stricmp( teamstr, "follow2" ) ) {
		*team =     TEAM_SPECTATOR;
		*sState =   SPECTATOR_FOLLOW;
		if ( specClient ) {
			*specClient = -2;
		}
	} else if ( !Q_stricmp( teamstr, "spectator" ) || !Q_stricmp( teamstr, "s" ) ) {
		*team =     TEAM_SPECTATOR;
		*sState =   SPECTATOR_FREE;
	} else if ( !Q_stricmp( teamstr, "red" ) || !Q_stricmp( teamstr, "r" ) || !Q_stricmp( teamstr, "axis" ) ) {
		*team =     TEAM_AXIS;
	} else if ( !Q_stricmp( teamstr, "blue" ) || !Q_stricmp( teamstr, "b" ) || !Q_stricmp( teamstr, "allies" ) ) {
		*team =     TEAM_ALLIES;
	} else {
		*team = PickTeam( clientNum );
		if ( !G_teamJoinCheck( *team, &g_entities[clientNum] ) ) {
			*team = ( ( TEAM_AXIS | TEAM_ALLIES ) & ~*team );
		}
	}
}

/*
=================
SetTeam
=================
*/
qboolean SetTeam( gentity_t *ent, char *s, qboolean force, weapon_t w1, weapon_t w2, qboolean setweapons ) {
	team_t team, oldTeam;
	gclient_t           *client;
	int clientNum;
	spectatorState_t specState;
	int specClient;

	//
	// see what change is requested
	//
	client = ent->client;

	clientNum = client - level.clients;
	specClient = 0;

	G_TeamDataForString( s, client - level.clients, &team, &specState, &specClient );

	if ( team != TEAM_SPECTATOR ) {
		// Ensure the player can join
		if ( !G_teamJoinCheck( team, ent ) ) {
			// Leave them where they were before the command was issued
			return( qfalse );
		}
	}

	//
	// decide if we will allow the change
	//
	oldTeam = client->sess.sessionTeam;
	if ( team == oldTeam && team != TEAM_SPECTATOR ) {
		return qfalse;
	}

	// DHM - Nerve
	// OSP
	if ( team != TEAM_SPECTATOR ) {
		client->pers.initialSpawn = qfalse;
	}

	if ( oldTeam != TEAM_SPECTATOR ) {
		if ( !( ent->client->ps.pm_flags & PMF_LIMBO ) ) {
			// Kill him (makes sure he loses flags, etc)
			ent->flags &= ~FL_GODMODE;
			ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
			player_die( ent, ent, ent, 100000, MOD_SWITCHTEAM );
		}
	}
	// they go to the end of the line for tournements
	if ( team == TEAM_SPECTATOR ) {
		client->sess.spectatorTime = level.time;
		if ( !client->sess.referee ) {
			client->pers.invite = 0;
		}
	}

	G_LeaveTank( ent, qfalse );

	// Nico, keep fireteams
	// G_RemoveClientFromFireteams( clientNum, qtrue, qfalse );

	G_FadeItems( ent, MOD_SATCHEL );

	// remove ourself from teamlists
	{
		int i;
		mapEntityData_t *mEnt;
		mapEntityData_Team_t *teamList;

		for ( i = 0; i < 2; i++ ) {
			teamList = &mapEntityData[i];

			if ( ( mEnt = G_FindMapEntityData( &mapEntityData[0], ent - g_entities ) ) != NULL ) {
				G_FreeMapEntityData( teamList, mEnt );
			}

			mEnt = G_FindMapEntityDataSingleClient( teamList, NULL, ent->s.number, -1 );

			while ( mEnt ) {
				mapEntityData_t *mEntFree = mEnt;

				mEnt = G_FindMapEntityDataSingleClient( teamList, mEnt, ent->s.number, -1 );

				G_FreeMapEntityData( teamList, mEntFree );
			}
		}
	}
	client->sess.spec_team = 0;
	client->sess.sessionTeam = team;
	client->sess.spectatorState = specState;
	client->sess.spectatorClient = specClient;
	client->pers.ready = qfalse;

	// (l)users will spam spec messages... honest!
	if ( team != oldTeam ) {
		gentity_t* tent = G_PopupMessage( PM_TEAM );
		tent->s.effect2Time = team;
		tent->s.effect3Time = clientNum;
		tent->s.density = 0;
	}

	if ( setweapons ) {
		G_SetClientWeapons( ent, w1, w2, qfalse );
	}

	// get and distribute relevent paramters
	G_UpdateCharacter( client );            // FIXME : doesn't ClientBegin take care of this already?
	ClientUserinfoChanged( clientNum );

	ClientBegin( clientNum );

	G_UpdateSpawnCounts();

	if ( g_gamestate.integer == GS_PLAYING && ( client->sess.sessionTeam == TEAM_AXIS || client->sess.sessionTeam == TEAM_ALLIES ) ) {
		int i;
		int x = client->sess.sessionTeam - TEAM_AXIS;

		for ( i = 0; i < MAX_COMMANDER_TEAM_SOUNDS; i++ ) {
			if ( level.commanderSounds[ x ][ i ].index ) {
				gentity_t* tent = G_TempEntity( client->ps.origin, EV_GLOBAL_CLIENT_SOUND );
				tent->s.eventParm = level.commanderSounds[ x ][ i ].index - 1;
				tent->s.teamNum = clientNum;
			}
		}
	}

	return qtrue;
}

/*
=================
StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void StopFollowing( gentity_t *ent ) {
	// ATVI Wolfenstein Misc #474
	// divert behaviour if TEAM_SPECTATOR, moved the code from SpectatorThink to put back into free fly correctly
	// (I am not sure this can be called in non-TEAM_SPECTATOR situation, better be safe)
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		// drop to free floating, somewhere above the current position (that's the client you were following)
		vec3_t pos, angle;
		gclient_t   *client = ent->client;
		VectorCopy( client->ps.origin, pos );
		VectorCopy( client->ps.viewangles, angle );
		// Need this as it gets spec mode reset properly
		SetTeam( ent, "s", qtrue, -1, -1, qfalse );
		VectorCopy( pos, client->ps.origin );
		SetClientViewAngle( ent, angle );
	} else {
		// legacy code, FIXME: useless?
		// Gordon: no this is for limbo i'd guess
		ent->client->sess.spectatorState = SPECTATOR_FREE;
		ent->client->ps.clientNum = ent - g_entities;
	}
}

int G_NumPlayersWithWeapon( weapon_t weap, team_t team ) {
	int i, j, cnt = 0;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];

		if ( level.clients[j].sess.playerType != PC_SOLDIER ) {
			continue;
		}

		if ( level.clients[j].sess.sessionTeam != team ) {
			continue;
		}

		if ( level.clients[j].sess.latchPlayerWeapon != weap && level.clients[j].sess.playerWeapon != weap ) {
			continue;
		}

		cnt++;
	}

	return cnt;
}

int G_NumPlayersOnTeam( team_t team ) {
	int i, j, cnt = 0;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];

		if ( level.clients[j].sess.sessionTeam != team ) {
			continue;
		}

		cnt++;
	}

	return cnt;
}

qboolean G_IsHeavyWeapon( weapon_t weap ) {
	int i;

	for ( i = 0; i < NUM_HEAVY_WEAPONS; i++ ) {
		if ( bg_heavyWeapons[i] == weap ) {
			return qtrue;
		}
	}

	return qfalse;
}

int G_TeamCount( gentity_t* ent, weapon_t weap ) {
	int i, j, cnt;

	if ( weap == -1 ) { // we aint checking for a weapon, so always include ourselves
		cnt = 1;
	} else { // we ARE checking for a weapon, so ignore ourselves
		cnt = 0;
	}

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];

		if ( j == ent - g_entities ) {
			continue;
		}

		if ( level.clients[j].sess.sessionTeam != ent->client->sess.sessionTeam ) {
			continue;
		}

		if ( weap != -1 ) {
			if ( level.clients[j].sess.playerWeapon != weap && level.clients[j].sess.latchPlayerWeapon != weap ) {
				continue;
			}
		}

		cnt++;
	}

	return cnt;
}

qboolean G_IsWeaponDisabled( gentity_t* ent, weapon_t weapon ) {
	int count, wcount;

	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return qtrue;
	}

	if ( !G_IsHeavyWeapon( weapon ) ) {
		return qfalse;
	}

	count =     G_TeamCount( ent, -1 );
	wcount =    G_TeamCount( ent, weapon );

	if ( wcount >= ceil( count * g_heavyWeaponRestriction.integer * 0.01f ) ) {
		return qtrue;
	}

	return qfalse;
}

void G_SetClientWeapons( gentity_t* ent, weapon_t w1, weapon_t w2, qboolean updateclient ) {
	qboolean changed = qfalse;

	if ( ent->client->sess.latchPlayerWeapon2 != w2 ) {
		ent->client->sess.latchPlayerWeapon2 = w2;
		changed = qtrue;
	}

	if ( !G_IsWeaponDisabled( ent, w1 ) ) {
		if ( ent->client->sess.latchPlayerWeapon != w1 ) {
			ent->client->sess.latchPlayerWeapon = w1;
			changed = qtrue;
		}
	} else {
		if ( ent->client->sess.latchPlayerWeapon != 0 ) {
			ent->client->sess.latchPlayerWeapon = 0;
			changed = qtrue;
		}
	}

	if ( updateclient && changed ) {
		ClientUserinfoChanged( ent - g_entities );
	}
}


/*
=================
Cmd_Team_f
=================
*/
// Nico, flood protection
// void Cmd_Team_f( gentity_t *ent, unsigned int dwCommand, qboolean fValue ) {
void Cmd_Team_f( gentity_t *ent ) {
	char s[MAX_TOKEN_CHARS];
	char ptype[4];
	char weap[4], weap2[4];
	weapon_t w, w2;

	if ( trap_Argc() < 2 ) {
		char *pszTeamName;

		switch ( ent->client->sess.sessionTeam ) {
		case TEAM_ALLIES:
			pszTeamName = "Allies";
			break;
		case TEAM_AXIS:
			pszTeamName = "Axis";
			break;
		case TEAM_SPECTATOR:
			pszTeamName = "Spectator";
			break;
		case TEAM_FREE:
		default:
			pszTeamName = "Free";
			break;
		}

		CP( va( "print \"%s team\n\"", pszTeamName ) );
		return;
	}

	trap_Argv( 1, s,        sizeof( s       ) );
	trap_Argv( 2, ptype,    sizeof( ptype   ) );
	trap_Argv( 3, weap,     sizeof( weap    ) );
	trap_Argv( 4, weap2,    sizeof( weap2   ) );

	w =     atoi( weap );
	w2 =    atoi( weap2 );

	ent->client->sess.latchPlayerType = atoi( ptype );
	if ( ent->client->sess.latchPlayerType < PC_SOLDIER || ent->client->sess.latchPlayerType > PC_COVERTOPS ) {
		ent->client->sess.latchPlayerType = PC_SOLDIER;
	}

	if ( ent->client->sess.latchPlayerType < PC_SOLDIER || ent->client->sess.latchPlayerType > PC_COVERTOPS ) {
		ent->client->sess.latchPlayerType = PC_SOLDIER;
	}

	if ( !SetTeam( ent, s, qfalse, w, w2, qtrue ) ) {
		G_SetClientWeapons( ent, w, w2, qtrue );
	}
}

void Cmd_ResetSetup_f( gentity_t* ent ) {
	qboolean changed = qfalse;

	if ( !ent || !ent->client ) {
		return;
	}

	ent->client->sess.latchPlayerType =     ent->client->sess.playerType;

	if ( ent->client->sess.latchPlayerWeapon != ent->client->sess.playerWeapon ) {
		ent->client->sess.latchPlayerWeapon = ent->client->sess.playerWeapon;
		changed = qtrue;
	}

	if ( ent->client->sess.latchPlayerWeapon2 != ent->client->sess.playerWeapon2 ) {
		ent->client->sess.latchPlayerWeapon2 =  ent->client->sess.playerWeapon2;
		changed = qtrue;
	}

	if ( changed ) {
		ClientUserinfoChanged( ent - g_entities );
	}
}

void Cmd_SetClass_f( gentity_t* ent, unsigned int dwCommand, qboolean fValue ) {
}

void Cmd_SetWeapons_f( gentity_t* ent, unsigned int dwCommand, qboolean fValue ) {
}

// END Mad Doc - TDF
/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent, unsigned int dwCommand, qboolean fValue ) {
	int i;
	char arg[MAX_TOKEN_CHARS];

	if ( trap_Argc() != 2 ) {
		if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
			StopFollowing( ent );
		}
		return;
	}

	if ( ent->client->ps.pm_flags & PMF_LIMBO ) {
		// Nico, replaced cpm by print to display into console
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=065
		CP( "print \"Can't issue a follow command while in limbo.\n\"" );
		CP( "print \"Hit FIRE to switch between teammates.\n\"" );
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	i = ClientNumberFromString( ent, arg );
	if ( i == -1 ) {
		if ( !Q_stricmp( arg, "allies" ) ) {
			i = TEAM_ALLIES;
		} else if ( !Q_stricmp( arg, "axis" ) ) {
			i = TEAM_AXIS;
		} else { return;}

		if ( !TeamCount( ent - g_entities, i ) ) {
			CP( va( "print \"The %s team %s empty!  Follow command ignored.\n\"", aTeams[i],
					( ( ent->client->sess.sessionTeam != i ) ? "is" : "would be" ) ) );
			return;
		}

		// Allow for simple toggle
		if ( ent->client->sess.spec_team != i ) {
			if ( teamInfo[i].spec_lock && !( ent->client->sess.spec_invite & i ) ) {
				CP( va( "print \"Sorry, the %s team is locked from spectators.\n\"", aTeams[i] ) );
			} else {
				ent->client->sess.spec_team = i;
				CP( va( "print \"Spectator follow is now locked on the %s team.\n\"", aTeams[i] ) );
				Cmd_FollowCycle_f( ent, 1 );
			}
		} else {
			ent->client->sess.spec_team = 0;
			CP( va( "print \"%s team spectating is now disabled.\n\"", aTeams[i] ) );
		}

		return;
	}

	// can't follow self
	if ( &level.clients[ i ] == ent->client ) {
		return;
	}

	// can't follow another spectator
	if ( level.clients[ i ].sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}
	if ( level.clients[ i ].ps.pm_flags & PMF_LIMBO ) {
		return;
	}

	// OSP - can't follow a player on a speclocked team, unless allowed
	if ( !G_allowFollow( ent, level.clients[i].sess.sessionTeam ) ) {
		CP( va( "print \"Sorry, the %s team is locked from spectators.\n\"", aTeams[level.clients[i].sess.sessionTeam] ) );
		return;
	}

	// first set them to spectator
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		SetTeam( ent, "spectator", qfalse, -1, -1, qfalse );
	}

	ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
	ent->client->sess.spectatorClient = i;
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent, int dir ) {
	int clientnum;
	int original;

	// first set them to spectator
	if ( ( ent->client->sess.spectatorState == SPECTATOR_NOT ) && ( !( ent->client->ps.pm_flags & PMF_LIMBO ) ) ) { // JPW NERVE for limbo state
		SetTeam( ent, "spectator", qfalse, -1, -1, qfalse );
	}

	if ( dir != 1 && dir != -1 ) {
		G_Error( "Cmd_FollowCycle_f: bad dir %i", dir );
	}

	clientnum = ent->client->sess.spectatorClient;
	original = clientnum;
	do {
		clientnum += dir;
		if ( clientnum >= level.maxclients ) {
			clientnum = 0;
		}
		if ( clientnum < 0 ) {
			clientnum = level.maxclients - 1;
		}

		// can only follow connected clients
		if ( level.clients[ clientnum ].pers.connected != CON_CONNECTED ) {
			continue;
		}

		// can't follow another spectator
		if ( level.clients[ clientnum ].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		// JPW NERVE -- couple extra checks for limbo mode
		if ( ent->client->ps.pm_flags & PMF_LIMBO ) {
			if ( level.clients[clientnum].ps.pm_flags & PMF_LIMBO ) {
				continue;
			}
			if ( level.clients[clientnum].sess.sessionTeam != ent->client->sess.sessionTeam ) {
				continue;
			}
		}

		if ( level.clients[clientnum].ps.pm_flags & PMF_LIMBO ) {
			continue;
		}

		// OSP
		if ( !G_desiredFollow( ent, level.clients[clientnum].sess.sessionTeam ) ) {
			continue;
		}

		// this is good, we can use it
		ent->client->sess.spectatorClient = clientnum;
		ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
		return;
	} while ( clientnum != original );

	// leave it where it was
}


/*======================
G_EntitySound
	Mad Doc xkan, 11/06/2002 -

	Plays a sound (wav file or sound script) on this entity

	Note that calling G_AddEvent(..., EV_GENERAL_SOUND, ...) has the danger of
	the event never getting through to the client because the entity might not
	be visible (unless it has the SVF_BROADCAST flag), so if you want to make sure
	the sound is heard, call this function instead.
======================*/
void G_EntitySound(
	gentity_t *ent,         // entity to play the sound on
	const char *soundId,    // sound file name or sound script ID
	int volume ) {           // sound volume, only applies to sound file name call
							 //   for sound script, volume is currently always 127.
	trap_SendServerCommand( -1, va( "entitySound %d %s %d %i %i %i normal", ent->s.number, soundId, volume,
									(int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2] ) );
}

/*======================
G_EntitySoundNoCut
	Mad Doc xkan, 1/16/2003 -

	Similar to G_EntitySound, but do not cut this sound off

======================*/
void G_EntitySoundNoCut(
	gentity_t *ent,         // entity to play the sound on
	const char *soundId,    // sound file name or sound script ID
	int volume ) {           // sound volume, only applies to sound file name call
							 //   for sound script, volume is currently always 127.
	trap_SendServerCommand( -1, va( "entitySound %d %s %d %i %i %i noCut", ent->s.number, soundId, volume,
									(int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2] ) );
}


/*
==================
G_Say
==================
*/
#define MAX_SAY_TEXT    150

void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, qboolean localize, qboolean encoded) {
	char *cmd = NULL;

	if ( !other || !other->inuse || !other->client ) {
		return;
	}
	if ( ( mode == SAY_TEAM || mode == SAY_TEAMNL ) && !OnSameTeam( ent, other ) ) {
		return;
	}

	// NERVE - SMF - if spectator, no chatting to players in WolfMP
	if ( ent->client->sess.referee == 0 &&   // OSP
		 ( ( ent->client->sess.sessionTeam == TEAM_FREE && other->client->sess.sessionTeam != TEAM_FREE ) ) ) {
		return;
	} else {
		if ( mode == SAY_BUDDY ) {  // send only to people who have the sender on their buddy list
			if ( ent->s.clientNum != other->s.clientNum ) {
				fireteamData_t *ft1, *ft2;
				if ( !G_IsOnFireteam( other - g_entities, &ft1 ) ) {
					return;
				}
				if ( !G_IsOnFireteam( ent - g_entities, &ft2 ) ) {
					return;
				}
				if ( ft1 != ft2 ) {
					return;
				}
			}
		}

		if (encoded) {
			cmd = mode == SAY_TEAM || mode == SAY_BUDDY ? "enc_tchat" : "enc_chat";
		} else {
			cmd = mode == SAY_TEAM || mode == SAY_BUDDY ? "tchat" : "chat";
		}
		trap_SendServerCommand( other - g_entities, va( "%s \"%s%c%c%s\" %i %i", cmd, name, Q_COLOR_ESCAPE, color, message, ent - g_entities, localize ) );
	}
}

void G_Say(gentity_t *ent, gentity_t *target, int mode, qboolean encoded, const char *chatText) {
	int j;
	gentity_t   *other;
	int color;
	char name[64];
	// don't let text be too long for malicious reasons
	char text[MAX_SAY_TEXT];
	qboolean localize = qfalse;
	char        *loc;

	switch ( mode ) {
	default:
	case SAY_ALL:
		G_LogPrintf( "say: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf( name, sizeof( name ), "%s%c%c: ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE );
		color = COLOR_GREEN;
		break;
	case SAY_BUDDY:
		localize = qtrue;
		G_LogPrintf( "saybuddy: %s: %s\n", ent->client->pers.netname, chatText );
		loc = BG_GetLocationString( ent->r.currentOrigin );
		Com_sprintf( name, sizeof( name ), "[lof](%s%c%c) (%s): ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, loc );
		color = COLOR_YELLOW;
		break;
	case SAY_TEAM:
		localize = qtrue;
		G_LogPrintf( "sayteam: %s: %s\n", ent->client->pers.netname, chatText );
		loc = BG_GetLocationString( ent->r.currentOrigin );
		Com_sprintf( name, sizeof( name ), "[lof](%s%c%c) (%s): ", ent->client->pers.netname, Q_COLOR_ESCAPE, COLOR_WHITE, loc );
		color = COLOR_CYAN;
		break;
	case SAY_TEAMNL:
		G_LogPrintf( "sayteamnl: %s: %s\n", ent->client->pers.netname, chatText );
		Com_sprintf( name, sizeof( name ), "(%s^7): ", ent->client->pers.netname );
		color = COLOR_CYAN;
		break;
	}

	Q_strncpyz( text, chatText, sizeof( text ) );

	if ( target ) {
		if ( !COM_BitCheck( target->client->sess.ignoreClients, ent - g_entities ) ) {
			G_SayTo(ent, target, mode, color, name, text, localize, encoded);
		}
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "%s%s\n", name, text );
	}

	// send it to all the apropriate clients
	for ( j = 0; j < level.numConnectedClients; j++ ) {
		other = &g_entities[level.sortedClients[j]];
		if ( !COM_BitCheck( other->client->sess.ignoreClients, ent - g_entities ) ) {
			G_SayTo(ent, other, mode, color, name, text, localize, encoded);
		}
	}
}


/*
==================
Cmd_Say_f
==================
*/
void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0, qboolean encoded) {
	if (trap_Argc() < 2 && !arg0) {
		return;
	}
	G_Say(ent, NULL, mode, encoded, ConcatArgs((arg0 ? 0 : 1)));
}

// NERVE - SMF
void G_VoiceTo( gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly ) {
	int color;
	char *cmd;

	if ( !other ) {
		return;
	}
	if ( !other->inuse ) {
		return;
	}
	if ( !other->client ) {
		return;
	}
	if ( mode == SAY_TEAM && !OnSameTeam( ent, other ) ) {
		return;
	}

	// OSP - spec vchat rules follow the same as normal chatting rules
	if ( ent->client->sess.referee == 0 &&
		 ent->client->sess.sessionTeam == TEAM_SPECTATOR && other->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		return;
	}

	// send only to people who have the sender on their buddy list
	if ( mode == SAY_BUDDY ) {
		if ( ent->s.clientNum != other->s.clientNum ) {
			fireteamData_t *ft1, *ft2;
			if ( !G_IsOnFireteam( other - g_entities, &ft1 ) ) {
				return;
			}
			if ( !G_IsOnFireteam( ent - g_entities, &ft2 ) ) {
				return;
			}
			if ( ft1 != ft2 ) {
				return;
			}
		}
	}

	if ( mode == SAY_TEAM ) {
		color = COLOR_CYAN;
		cmd = "vtchat";
	} else if ( mode == SAY_BUDDY ) {
		color = COLOR_YELLOW;
		cmd = "vbchat";
	} else {
		color = COLOR_GREEN;
		cmd = "vchat";
	}

	if ( voiceonly == 2 ) {
		voiceonly = qfalse;
	}

	if ( mode == SAY_TEAM || mode == SAY_BUDDY ) {
		CPx( other - g_entities, va( "%s %d %d %d %s %i %i %i", cmd, voiceonly, ent - g_entities, color, id, (int)ent->s.pos.trBase[0], (int)ent->s.pos.trBase[1], (int)ent->s.pos.trBase[2] ) );
	} else {
		CPx( other - g_entities, va( "%s %d %d %d %s", cmd, voiceonly, ent - g_entities, color, id ) );
	}
}

void G_Voice( gentity_t *ent, gentity_t *target, int mode, const char *id, qboolean voiceonly ) {
	int j;

	// DHM - Nerve :: Don't allow excessive spamming of voice chats
	ent->voiceChatSquelch -= ( level.time - ent->voiceChatPreviousTime );
	ent->voiceChatPreviousTime = level.time;

	if ( ent->voiceChatSquelch < 0 ) {
		ent->voiceChatSquelch = 0;
	}

	// Only do the spam check for MP
	if ( ent->voiceChatSquelch >= 30000 ) {
		// Nico, voicechat spam protection was cluttering the popups message
		// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=066
		trap_SendServerCommand( ent - g_entities, "cp \"^1Spam Protection^7: VoiceChat ignored\"" );
		return;
	}

	if ( g_voiceChatsAllowed.integer ) {
		ent->voiceChatSquelch += ( 34000 / g_voiceChatsAllowed.integer );
	} else {
		return;
	}
	// dhm

	if ( target ) {
		G_VoiceTo( ent, target, mode, id, voiceonly );
		return;
	}

	// echo the text to the console
	if ( g_dedicated.integer ) {
		G_Printf( "voice: %s %s\n", ent->client->pers.netname, id );
	}

	if ( mode == SAY_BUDDY ) {
		char buffer[32];
		int cls = -1, i, cnt, num;
		qboolean allowclients[MAX_CLIENTS];

		memset( allowclients, 0, sizeof( allowclients ) );

		trap_Argv( 1, buffer, 32 );

		cls = atoi( buffer );

		trap_Argv( 2, buffer, 32 );
		cnt = atoi( buffer );
		if ( cnt > MAX_CLIENTS ) {
			cnt = MAX_CLIENTS;
		}

		for ( i = 0; i < cnt; i++ ) {
			trap_Argv( 3 + i, buffer, 32 );

			num = atoi( buffer );
			if ( num < 0 ) {
				continue;
			}
			if ( num >= MAX_CLIENTS ) {
				continue;
			}

			allowclients[ num ] = qtrue;
		}

		for ( j = 0; j < level.numConnectedClients; j++ ) {

			if ( level.sortedClients[j] != ent->s.clientNum ) {
				if ( cls != -1 && cls != level.clients[level.sortedClients[j]].sess.playerType ) {
					continue;
				}
			}

			if ( cnt ) {
				if ( !allowclients[ level.sortedClients[j] ] ) {
					continue;
				}
			}

			G_VoiceTo( ent, &g_entities[level.sortedClients[j]], mode, id, voiceonly );
		}
	} else {

		// send it to all the apropriate clients
		for ( j = 0; j < level.numConnectedClients; j++ ) {
			G_VoiceTo( ent, &g_entities[level.sortedClients[j]], mode, id, voiceonly );
		}
	}
}

/*
==================
Cmd_Voice_f
==================
*/
static void Cmd_Voice_f( gentity_t *ent, int mode, qboolean arg0, qboolean voiceonly ) {
	if ( mode != SAY_BUDDY ) {
		if ( trap_Argc() < 2 && !arg0 ) {
			return;
		}
		G_Voice( ent, NULL, mode, ConcatArgs( ( ( arg0 ) ? 0 : 1 ) ), voiceonly );
	} else {
		char buffer[16];
		int index;

		trap_Argv( 2, buffer, sizeof( buffer ) );
		index = atoi( buffer );
		if ( index < 0 ) {
			index = 0;
		}

		if ( trap_Argc() < 3 + index && !arg0 ) {
			return;
		}
		G_Voice( ent, NULL, mode, ConcatArgs( ( ( arg0 ) ? 2 + index : 3 + index ) ), voiceonly );
	}
}

/*
==================
Cmd_CallVote_f
==================
*/
qboolean Cmd_CallVote_f( gentity_t *ent, unsigned int dwCommand, qboolean fRefCommand ) {
	int i;
	char arg1[MAX_STRING_TOKENS];
	char arg2[MAX_STRING_TOKENS];

	// Normal checks, if its not being issued as a referee command
	// Nico, moved 'callvote' command erros from popup messages to center print and console
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=067
	if ( !fRefCommand ) {
		if ( level.voteInfo.voteTime ) {
			G_printFull("A vote is already in progress.", ent);
			return qfalse;
		}
		else if ( !ent->client->sess.referee ) {
			if ( voteFlags.integer == VOTING_DISABLED ) {
				G_printFull("Voting not enabled on this server.", ent);
				return qfalse;
			} else if ( vote_limit.integer > 0 && ent->client->pers.voteCount >= vote_limit.integer ) {
				G_printFull(va("You have already called the maximum number of votes (%d).", vote_limit.integer), ent);
				return qfalse;
			} else if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
				G_printFull("Not allowed to call a vote as a spectator.", ent);
				return qfalse;
			}
		}
	}

	// make sure it is a valid command to vote on
	trap_Argv( 1, arg1, sizeof( arg1 ) );
	trap_Argv( 2, arg2, sizeof( arg2 ) );


	// Nico, bugfix: callvote exploit fixed
	// http://aluigi.freeforums.org/quake3-engine-callvote-bug-t686.html
	if (strchr( arg1, ';' ) || strchr( arg2, ';' ) ||
		strchr( arg1, '\r' ) || strchr( arg2, '\r' ) ||
		strchr( arg1, '\n' ) || strchr( arg2, '\n' )) {
		char *strCmdBase = ( !fRefCommand ) ? "vote" : "ref command";

		G_refPrintf( ent, "Invalid %s string.", strCmdBase );
		return( qfalse );
	}


	if ( trap_Argc() > 1 && ( i = G_voteCmdCheck( ent, arg1, arg2, fRefCommand ) ) != G_NOTFOUND ) {   //  --OSP
		if ( i != G_OK ) {
			if ( i == G_NOTFOUND ) {
				return( qfalse );               // Command error
			} else { return( qtrue );}
		}
	} else {
		if ( !fRefCommand ) {
			CP( va( "print \"\n^3>>> Unknown vote command: ^7%s %s\n\"", arg1, arg2 ) );
			G_voteHelp( ent, qtrue );
		}
		return( qfalse );
	}

	Com_sprintf( level.voteInfo.voteString, sizeof( level.voteInfo.voteString ), "%s %s", arg1, arg2 );

	// start the voting, the caller automatically votes yes
	// If a referee, vote automatically passes.	// OSP
	if ( fRefCommand ) {
//		level.voteInfo.voteYes = level.voteInfo.numVotingClients + 10;	// JIC :)
		// Don't announce some votes, as in comp mode, it is generally a ref
		// who is policing people who shouldn't be joining and players don't want
		// this sort of spam in the console
		if ( level.voteInfo.vote_fn != G_Kick_v && level.voteInfo.vote_fn != G_Mute_v ) {
			AP( "cp \"^1** Referee Server Setting Change **\n\"" );
		}

		// Gordon: just call the stupid thing.... don't bother with the voting faff
		level.voteInfo.vote_fn( NULL, 0, NULL, NULL, qfalse );

		G_globalSound( "sound/misc/referee.wav" );
	} else {
		level.voteInfo.voteYes = 1;
		AP( va( "print \"[lof]%s^7 [lon]called a vote.[lof]  Voting for: %s\n\"", ent->client->pers.netname, level.voteInfo.voteString ) );
		AP( va( "cp \"[lof]%s\n^7[lon]called a vote.\n\"", ent->client->pers.netname ) );
		G_globalSound( "sound/misc/vote.wav" );
	}

	level.voteInfo.voteTime = level.time;
	level.voteInfo.voteNo = 0;

	// Nico, used to check if voter switches team
	level.voteInfo.voter_team = ent->client->sess.sessionTeam;
	level.voteInfo.voter_cn = ent - g_entities;

	// Don't send the vote info if a ref initiates (as it will automatically pass)
	if ( !fRefCommand ) {
		for ( i = 0; i < level.numConnectedClients; i++ ) {
			level.clients[level.sortedClients[i]].ps.eFlags &= ~EF_VOTED;
		}

		ent->client->pers.voteCount++;
		ent->client->ps.eFlags |= EF_VOTED;

		trap_SetConfigstring( CS_VOTE_YES,    va( "%i", level.voteInfo.voteYes ) );
		trap_SetConfigstring( CS_VOTE_NO,     va( "%i", level.voteInfo.voteNo ) );
		trap_SetConfigstring( CS_VOTE_STRING, level.voteInfo.voteString );
		trap_SetConfigstring( CS_VOTE_TIME,   va( "%i", level.voteInfo.voteTime ) );
	}

	return( qtrue );
}

qboolean StringToFilter( const char *s, ipFilter_t *f );

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent ) {
	char msg[64];

	if ( ent->client->pers.applicationEndTime > level.time ) {

		gclient_t *cl = g_entities[ ent->client->pers.applicationClient ].client;
		if ( !cl ) {
			return;
		}
		if ( cl->pers.connected != CON_CONNECTED ) {
			return;
		}

		trap_Argv( 1, msg, sizeof( msg ) );

		if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
			trap_SendServerCommand( ent - g_entities, "application -4" );
			trap_SendServerCommand( ent->client->pers.applicationClient, "application -3" );

			G_AddClientToFireteam( ent->client->pers.applicationClient, ent - g_entities );
		} else {
			trap_SendServerCommand( ent - g_entities, "application -4" );
			trap_SendServerCommand( ent->client->pers.applicationClient, "application -2" );
		}

		ent->client->pers.applicationEndTime = 0;
		ent->client->pers.applicationClient = -1;

		return;
	}

	ent->client->pers.applicationEndTime = 0;
	ent->client->pers.applicationClient = -1;

	if ( ent->client->pers.invitationEndTime > level.time ) {

		gclient_t *cl = g_entities[ ent->client->pers.invitationClient ].client;
		if ( !cl ) {
			return;
		}
		if ( cl->pers.connected != CON_CONNECTED ) {
			return;
		}

		trap_Argv( 1, msg, sizeof( msg ) );

		if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
			trap_SendServerCommand( ent - g_entities, "invitation -4" );
			trap_SendServerCommand( ent->client->pers.invitationClient, "invitation -3" );

			G_AddClientToFireteam( ent - g_entities, ent->client->pers.invitationClient );
		} else {
			trap_SendServerCommand( ent - g_entities, "invitation -4" );
			trap_SendServerCommand( ent->client->pers.invitationClient, "invitation -2" );
		}

		ent->client->pers.invitationEndTime = 0;
		ent->client->pers.invitationClient = -1;

		return;
	}

	ent->client->pers.invitationEndTime = 0;
	ent->client->pers.invitationClient = -1;

	if ( ent->client->pers.propositionEndTime > level.time ) {
		gclient_t *cl = g_entities[ ent->client->pers.propositionClient ].client;
		if ( !cl ) {
			return;
		}
		if ( cl->pers.connected != CON_CONNECTED ) {
			return;
		}

		trap_Argv( 1, msg, sizeof( msg ) );

		if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
			trap_SendServerCommand( ent - g_entities, "proposition -4" );
			trap_SendServerCommand( ent->client->pers.propositionClient2, "proposition -3" );

			G_InviteToFireTeam( ent - g_entities, ent->client->pers.propositionClient );
		} else {
			trap_SendServerCommand( ent - g_entities, "proposition -4" );
			trap_SendServerCommand( ent->client->pers.propositionClient2, "proposition -2" );
		}

		ent->client->pers.propositionEndTime = 0;
		ent->client->pers.propositionClient = -1;
		ent->client->pers.propositionClient2 = -1;

		return;
	}

	ent->client->pers.propositionEndTime = 0;
	ent->client->pers.propositionClient = -1;
	ent->client->pers.propositionClient2 = -1;

	// dhm
	// Reset this ent's complainEndTime so they can't send multiple complaints

	if ( !level.voteInfo.voteTime ) {
		trap_SendServerCommand( ent - g_entities, "print \"No vote in progress.\n\"" );
		return;
	}
	if ( ent->client->ps.eFlags & EF_VOTED ) {
		trap_SendServerCommand( ent - g_entities, "print \"Vote already cast.\n\"" );
		return;
	}
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		trap_SendServerCommand( ent - g_entities, "print \"Not allowed to vote as spectator.\n\"" );
		return;
	}

	if ( level.voteInfo.vote_fn == G_Kick_v ) {
		int pid = atoi( level.voteInfo.vote_value );
		if ( !g_entities[ pid ].client ) {
			return;
		}

		if ( g_entities[ pid ].client->sess.sessionTeam != TEAM_SPECTATOR && ent->client->sess.sessionTeam != g_entities[ pid ].client->sess.sessionTeam ) {
			trap_SendServerCommand( ent - g_entities, "print \"Cannot vote to kick player on opposing team.\n\"" );
			return;
		}
	}

	trap_SendServerCommand( ent - g_entities, "print \"Vote cast.\n\"" );

	ent->client->ps.eFlags |= EF_VOTED;

	trap_Argv( 1, msg, sizeof( msg ) );

	if ( msg[0] == 'y' || msg[1] == 'Y' || msg[1] == '1' ) {
		level.voteInfo.voteYes++;
		trap_SetConfigstring( CS_VOTE_YES, va( "%i", level.voteInfo.voteYes ) );
	} else {
		level.voteInfo.voteNo++;
		trap_SetConfigstring( CS_VOTE_NO, va( "%i", level.voteInfo.voteNo ) );
	}

	// a majority will be determined in G_CheckVote, which will also account
	// for players entering or leaving
}


qboolean G_canPickupMelee( gentity_t *ent ) {
// JPW NERVE -- no "melee" weapons in net play
	return qfalse;
}
// jpw

/*
=================
Cmd_StartCamera_f
=================
*/
void Cmd_StartCamera_f( gentity_t *ent ) {

	if ( ent->client->cameraPortal  ) {
		G_FreeEntity( ent->client->cameraPortal );
	}
	ent->client->cameraPortal = G_Spawn();

	ent->client->cameraPortal->s.eType = ET_CAMERA;
	ent->client->cameraPortal->s.apos.trType = TR_STATIONARY;
	ent->client->cameraPortal->s.apos.trTime = 0;
	ent->client->cameraPortal->s.apos.trDuration = 0;
	VectorClear( ent->client->cameraPortal->s.angles );
	VectorClear( ent->client->cameraPortal->s.apos.trDelta );
	G_SetOrigin( ent->client->cameraPortal, ent->r.currentOrigin );
	VectorCopy( ent->r.currentOrigin, ent->client->cameraPortal->s.origin2 );

	ent->client->cameraPortal->s.frame = 0;

	ent->client->cameraPortal->r.svFlags |= ( SVF_PORTAL | SVF_SINGLECLIENT );
	ent->client->cameraPortal->r.singleClient = ent->client->ps.clientNum;

	ent->client->ps.eFlags |= EF_VIEWING_CAMERA;
	ent->s.eFlags |= EF_VIEWING_CAMERA;

	VectorCopy( ent->r.currentOrigin, ent->client->cameraOrigin );  // backup our origin
}

/*
=================
Cmd_StopCamera_f
=================
*/
void Cmd_StopCamera_f( gentity_t *ent ) {
	if ( ent->client->cameraPortal && ( ent->client->ps.eFlags & EF_VIEWING_CAMERA ) ) {

		// go back into noclient mode
		G_FreeEntity( ent->client->cameraPortal );
		ent->client->cameraPortal = NULL;

		ent->s.eFlags &= ~EF_VIEWING_CAMERA;
		ent->client->ps.eFlags &= ~EF_VIEWING_CAMERA;
	}
}

/*
=================
Cmd_SetCameraOrigin_f
=================
*/
void Cmd_SetCameraOrigin_f( gentity_t *ent ) {
	char buffer[MAX_TOKEN_CHARS];
	int i;
	vec3_t origin;

	if ( trap_Argc() != 4 ) {
		return;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		trap_Argv( i + 1, buffer, sizeof( buffer ) );
		origin[i] = atof( buffer );
	}

	if ( ent->client->cameraPortal ) {
		VectorCopy( origin, ent->client->cameraPortal->s.origin2 );
		trap_LinkEntity( ent->client->cameraPortal );
	}
}

extern vec3_t playerMins;
extern vec3_t playerMaxs;

qboolean G_TankIsOccupied( gentity_t* ent ) {
	if ( !ent->tankLink ) {
		return qfalse;
	}

	return qtrue;
}

qboolean G_TankIsMountable( gentity_t* ent, gentity_t* other ) {
	if ( !( ent->spawnflags & 128 ) ) {
		return qfalse;
	}

	if ( level.disableTankEnter ) {
		return qfalse;
	}

	if ( G_TankIsOccupied( ent ) ) {
		return qfalse;
	}

	if ( ent->health <= 0 ) {
		return qfalse;
	}

	if ( other->client->ps.weaponDelay ) {
		return qfalse;
	}

	return qtrue;
}

// TAT 1/14/2003 - extracted out the functionality of Cmd_Activate_f from finding the object to use
//		so we can force bots to use items, without worrying that they are looking EXACTLY at the target
qboolean Do_Activate_f( gentity_t *ent, gentity_t *traceEnt ) {
	qboolean found = qfalse;
	qboolean walking = qfalse;
	vec3_t forward;         //, offset, end;

	// Arnout: invisible entities can't be used

	if ( traceEnt->entstate == STATE_INVISIBLE || traceEnt->entstate == STATE_UNDERCONSTRUCTION ) {
		return qfalse;
	}

	if ( ent->client->pers.cmd.buttons & BUTTON_WALKING ) {
		walking = qtrue;
	}

	if ( traceEnt->classname ) {
		traceEnt->flags &= ~FL_SOFTACTIVATE;    // FL_SOFTACTIVATE will be set if the user is holding 'walk' key

		if ( traceEnt->s.eType == ET_ALARMBOX ) {
			trace_t trace;

			if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
				return qfalse;
			}

			memset( &trace, 0, sizeof( trace ) );

			if ( traceEnt->use ) {
				G_UseEntity( traceEnt, ent, 0 );
			}
			found = qtrue;
		} else if ( traceEnt->s.eType == ET_ITEM )     {
			trace_t trace;

			if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
				return qfalse;
			}

			memset( &trace, 0, sizeof( trace ) );

			if ( traceEnt->touch ) {
				if ( ent->client->pers.autoActivate == PICKUP_ACTIVATE ) {
					ent->client->pers.autoActivate = PICKUP_FORCE;      //----(SA) force pickup
				}
				traceEnt->active = qtrue;
				traceEnt->touch( traceEnt, ent, &trace );
			}

			found = qtrue;
		} else if ( traceEnt->s.eType == ET_MOVER && G_TankIsMountable( traceEnt, ent ) ) {
			G_Script_ScriptEvent( traceEnt, "mg42", "mount" );
			ent->tagParent = traceEnt->nextTrain;
			Q_strncpyz( ent->tagName, "tag_player", MAX_QPATH );
			ent->backupWeaponTime = ent->client->ps.weaponTime;
			ent->client->ps.weaponTime = traceEnt->backupWeaponTime;
			ent->client->ps.weapHeat[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

			ent->tankLink = traceEnt;
			traceEnt->tankLink = ent;

			G_ProcessTagConnect( ent, qtrue );
			found = qtrue;
		} else if ( G_EmplacedGunIsMountable( traceEnt, ent ) ) {
			gclient_t* cl = &level.clients[ ent->s.clientNum ];
			vec3_t point;

			AngleVectors( traceEnt->s.apos.trBase, forward, NULL, NULL );
			VectorMA( traceEnt->r.currentOrigin, -36, forward, point );
			point[2] = ent->r.currentOrigin[2];

			// Save initial position
			VectorCopy( point, ent->TargetAngles );

			// Zero out velocity
			VectorCopy( vec3_origin, ent->client->ps.velocity );
			VectorCopy( vec3_origin, ent->s.pos.trDelta );

			traceEnt->active = qtrue;
			ent->active = qtrue;
			traceEnt->r.ownerNum = ent->s.number;
			VectorCopy( traceEnt->s.angles, traceEnt->TargetAngles );
			traceEnt->s.otherEntityNum = ent->s.number;

			cl->pmext.harc = traceEnt->harc;
			cl->pmext.varc = traceEnt->varc;
			VectorCopy( traceEnt->s.angles, cl->pmext.centerangles );
			cl->pmext.centerangles[PITCH] = AngleNormalize180( cl->pmext.centerangles[PITCH] );
			cl->pmext.centerangles[YAW] = AngleNormalize180( cl->pmext.centerangles[YAW] );
			cl->pmext.centerangles[ROLL] = AngleNormalize180( cl->pmext.centerangles[ROLL] );

			ent->backupWeaponTime = ent->client->ps.weaponTime;
			ent->client->ps.weaponTime = traceEnt->backupWeaponTime;
			ent->client->ps.weapHeat[WP_DUMMY_MG42] = traceEnt->mg42weapHeat;

			G_UseTargets( traceEnt, ent );   //----(SA)	added for Mike so mounting an MG42 can be a trigger event (let me know if there's any issues with this)
			found = qtrue;
		} else if ( ( ( Q_stricmp( traceEnt->classname, "func_door" ) == 0 ) || ( Q_stricmp( traceEnt->classname, "func_door_rotating" ) == 0 ) ) ) {
			if ( walking ) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			G_TryDoor( traceEnt, ent, ent );      // (door,other,activator)
			found = qtrue;
		} else if ( ( Q_stricmp( traceEnt->classname, "team_WOLF_checkpoint" ) == 0 ) ) {
			if ( traceEnt->count != ent->client->sess.sessionTeam ) {
				traceEnt->health++;
			}
			found = qtrue;
		} else if ( ( Q_stricmp( traceEnt->classname, "func_button" ) == 0 ) && ( traceEnt->s.apos.trType == TR_STATIONARY && traceEnt->s.pos.trType == TR_STATIONARY ) && traceEnt->active == qfalse ) {
			Use_BinaryMover( traceEnt, ent, ent );
			traceEnt->active = qtrue;
			found = qtrue;
		} else if ( !Q_stricmp( traceEnt->classname, "func_invisible_user" ) ) {
			if ( walking ) {
				traceEnt->flags |= FL_SOFTACTIVATE;     // no noise
			}
			G_UseEntity( traceEnt, ent, ent );
			found = qtrue;
		} else if ( !Q_stricmp( traceEnt->classname, "props_footlocker" ) ) {
			G_UseEntity( traceEnt, ent, ent );
			found = qtrue;
		}
	}

	return found;
}

void G_LeaveTank( gentity_t* ent, qboolean position ) {
	gentity_t* tank;

	// found our tank (or whatever)
	vec3_t axis[3];
	vec3_t pos;
	trace_t tr;

	tank = ent->tankLink;
	if ( !tank ) {
		return;
	}

	if ( position ) {

		AnglesToAxis( tank->s.angles, axis );

		VectorMA( ent->client->ps.origin, 128, axis[1], pos );
		trap_Trace( &tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID );

		if ( tr.startsolid ) {
			// try right
			VectorMA( ent->client->ps.origin, -128, axis[1], pos );
			trap_Trace( &tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID );

			if ( tr.startsolid ) {
				// try back
				VectorMA( ent->client->ps.origin, -224, axis[0], pos );
				trap_Trace( &tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID );

				if ( tr.startsolid ) {
					// try front
					VectorMA( ent->client->ps.origin, 224, axis[0], pos );
					trap_Trace( &tr, pos, playerMins, playerMaxs, pos, -1, CONTENTS_SOLID );

					if ( tr.startsolid ) {
						// give up
						return;
					}
				}
			}
		}

		VectorClear( ent->client->ps.velocity ); // Gordon: dont want them to fly away ;D
		TeleportPlayer( ent, pos, ent->client->ps.viewangles );
	}


	tank->mg42weapHeat = ent->client->ps.weapHeat[WP_DUMMY_MG42];
	tank->backupWeaponTime = ent->client->ps.weaponTime;
	ent->client->ps.weaponTime = ent->backupWeaponTime;

	G_Script_ScriptEvent( tank, "mg42", "unmount" );
	ent->tagParent = NULL;
	*ent->tagName = '\0';
	ent->s.eFlags &= ~EF_MOUNTEDTANK;
	ent->client->ps.eFlags &= ~EF_MOUNTEDTANK;
	tank->s.powerups = -1;

	tank->tankLink = NULL;
	ent->tankLink = NULL;
}

void Cmd_Activate_f( gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	gentity_t   *traceEnt;
	vec3_t forward, right, up, offset;
//	int			activatetime = level.time;
	qboolean found = qfalse;
	qboolean pass2 = qfalse;
	int i;

	if ( ent->health <= 0 ) {
		return;
	}

	if ( ent->s.weapon == WP_MORTAR_SET || ent->s.weapon == WP_MOBILE_MG42_SET ) {
		return;
	}

	if ( ent->active ) {
		if ( ent->client->ps.persistant[PERS_HWEAPON_USE] ) {
			// DHM - Nerve :: Restore original position if current position is bad
			trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, ent->r.currentOrigin, ent->s.number, MASK_PLAYERSOLID );
			if ( tr.startsolid ) {
				VectorCopy( ent->TargetAngles, ent->client->ps.origin );
				VectorCopy( ent->TargetAngles, ent->r.currentOrigin );
				ent->r.contents = CONTENTS_CORPSE;      // DHM - this will correct itself in ClientEndFrame
			}

			ent->client->ps.eFlags &= ~EF_MG42_ACTIVE;          // DHM - Nerve :: unset flag
			ent->client->ps.eFlags &= ~EF_AAGUN_ACTIVE;

			ent->client->ps.persistant[PERS_HWEAPON_USE] = 0;
			ent->active = qfalse;

			for ( i = 0; i < level.num_entities; i++ ) {
				if ( g_entities[i].s.eType == ET_MG42_BARREL && g_entities[i].r.ownerNum == ent->s.number ) {
					g_entities[i].mg42weapHeat = ent->client->ps.weapHeat[WP_DUMMY_MG42];
					g_entities[i].backupWeaponTime = ent->client->ps.weaponTime;
					break;
				}
			}
			ent->client->ps.weaponTime = ent->backupWeaponTime;
		} else {
			ent->active = qfalse;
		}
		return;
	} else if ( ent->client->ps.eFlags & EF_MOUNTEDTANK && ent->s.eFlags & EF_MOUNTEDTANK && !level.disableTankExit ) {
		G_LeaveTank( ent, qtrue );
		return;
	}

	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	VectorCopy( ent->client->ps.origin, offset );
	offset[2] += ent->client->ps.viewheight;

	// lean
	if ( ent->client->ps.leanf ) {
		VectorMA( offset, ent->client->ps.leanf, right, offset );
	}

	//VectorMA( offset, 256, forward, end );
	VectorMA( offset, 96, forward, end );

	trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE )  );

	if ( tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD ) {
		trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_MISSILECLIP | CONTENTS_TRIGGER ) );
		pass2 = qtrue;
	}

tryagain:

	if ( tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	found = Do_Activate_f( ent, traceEnt );

	if ( !found && !pass2 ) {
		pass2 = qtrue;
		trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_MISSILECLIP | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER ) );
		goto tryagain;
	}
}


void Cmd_Activate2_f( gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	vec3_t forward, right, up, offset;
	qboolean pass2 = qfalse;

	if ( ent->client->sess.playerType != PC_COVERTOPS ) {
		return;
	}

	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	CalcMuzzlePointForActivate( ent, forward, right, up, offset );
	VectorMA( offset, 96, forward, end );

	trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE ) );

	if ( tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD ) {
		trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER ) );
		pass2 = qtrue;
	}

tryagain:

	if ( tr.surfaceFlags & SURF_NOIMPACT || tr.entityNum == ENTITYNUM_WORLD ) {
		return;
	}

	if ( !pass2 ) {
		pass2 = qtrue;
		trap_Trace( &tr, offset, NULL, NULL, end, ent->s.number, ( CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER ) );
		goto tryagain;
	}
}

void G_UpdateSpawnCounts( void ) {
	int i, j;
	char cs[MAX_STRING_CHARS];
	int current, count, team;

	for ( i = 0; i < level.numspawntargets; i++ ) {
		trap_GetConfigstring( CS_MULTI_SPAWNTARGETS + i, cs, sizeof( cs ) );

		current = atoi( Info_ValueForKey( cs, "c" ) );
		team = atoi( Info_ValueForKey( cs, "t" ) ) & ~256;

		count = 0;
		for ( j = 0; j < level.numConnectedClients; j++ ) {
			gclient_t* client = &level.clients[ level.sortedClients[ j ] ];

			if ( client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES ) {
				continue;
			}

			if ( client->sess.sessionTeam == team && client->sess.spawnObjectiveIndex == i + 1 ) {
				count++;
				continue;
			}

			if ( client->sess.spawnObjectiveIndex == 0 ) {
				if ( client->sess.sessionTeam == TEAM_AXIS ) {
					if ( level.axisAutoSpawn == i ) {
						count++;
						continue;
					}
				} else {
					if ( level.alliesAutoSpawn == i ) {
						count++;
						continue;
					}
				}
			}
		}

		if ( count == current ) {
			continue;
		}

		Info_SetValueForKey( cs, "c", va( "%i", count ) );
		trap_SetConfigstring( CS_MULTI_SPAWNTARGETS + i, cs );
	}
}

/*
============
Cmd_SetSpawnPoint_f
============
*/
void SetPlayerSpawn( gentity_t* ent, int spawn, qboolean update ) {
	ent->client->sess.spawnObjectiveIndex = spawn;
	if ( ent->client->sess.spawnObjectiveIndex >= MAX_MULTI_SPAWNTARGETS || ent->client->sess.spawnObjectiveIndex < 0 ) {
		ent->client->sess.spawnObjectiveIndex = 0;
	}

	if ( update ) {
		G_UpdateSpawnCounts();
	}
}

void Cmd_SetSpawnPoint_f( gentity_t* ent ) {
	char arg[MAX_TOKEN_CHARS];
	int val, i;

	if ( trap_Argc() != 2 ) {
		return;
	}

	trap_Argv( 1, arg, sizeof( arg ) );
	val = atoi( arg );

	if ( ent->client ) {
		SetPlayerSpawn( ent, val, qtrue );
	}

	for ( i = 0; i < level.numLimboCams; i++ ) {
		int x = ( g_entities[level.limboCams[i].targetEnt].count - CS_MULTI_SPAWNTARGETS ) + 1;
		if ( level.limboCams[i].spawn && x == val ) {
			VectorCopy( level.limboCams[i].origin, ent->s.origin2 );
			ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
			break;
		}
	}
}

void Cmd_SelectedObjective_f( gentity_t* ent ) {
	int i, val;
	char buffer[16];
	vec_t dist, neardist = 0;
	int nearest = -1;


	if ( !ent || !ent->client ) {
		return;
	}

	if ( trap_Argc() != 2 ) {
		return;
	}
	trap_Argv( 1, buffer, 16 );
	val = atoi( buffer ) + 1;


	for ( i = 0; i < level.numLimboCams; i++ ) {
		if ( !level.limboCams[i].spawn && level.limboCams[i].info == val ) {
			if ( !level.limboCams[i].hasEnt ) {
				VectorCopy( level.limboCams[i].origin, ent->s.origin2 );
				ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
				break;
			} else {
				dist = VectorDistanceSquared( level.limboCams[i].origin, g_entities[level.limboCams[i].targetEnt].r.currentOrigin );
				if ( nearest == -1 || dist < neardist ) {
					nearest = i;
					neardist = dist;
				}
			}
		}
	}

	if ( nearest != -1 ) {
		i = nearest;

		VectorCopy( level.limboCams[i].origin, ent->s.origin2 );
		ent->r.svFlags |= SVF_SELF_PORTAL_EXCLUSIVE;
	}
}

void Cmd_Ignore_f( gentity_t* ent ) {
	char cmd[MAX_TOKEN_CHARS];
	int cnum;

	trap_Argv( 1, cmd, sizeof( cmd ) );

	if ( !*cmd ) {
		trap_SendServerCommand( ent - g_entities, "print \"usage: Ignore <clientname>.\n\"\n" );
		return;
	}

	cnum = G_refClientnumForName( ent, cmd );

	if ( cnum != MAX_CLIENTS ) {
		COM_BitSet( ent->client->sess.ignoreClients, cnum );
	}
}

void Cmd_UnIgnore_f( gentity_t* ent ) {
	char cmd[MAX_TOKEN_CHARS];
	int cnum;

	trap_Argv( 1, cmd, sizeof( cmd ) );

	if ( !*cmd ) {
		trap_SendServerCommand( ent - g_entities, "print \"usage: Unignore <clientname>.\n\"\n" );
		return;
	}

	cnum = G_refClientnumForName( ent, cmd );

	if ( cnum != MAX_CLIENTS ) {
		COM_BitClear( ent->client->sess.ignoreClients, cnum );
	}
}

void Cmd_Load_f(gentity_t *ent)
{
	char cmd[MAX_TOKEN_CHARS];
	int argc;
	int posNum;
	save_position_t *pos;

	// get save slot (do this first so players can get usage message even if
	// they are not allowed to use this command)
	argc = trap_Argc();
	if (argc == 1) {
		posNum = 0;
	} else if (argc == 2) {
		trap_Argv(1, cmd, sizeof(cmd));		
		if ((posNum = atoi(cmd)) < 0 || posNum >= MAX_SAVED_POSITIONS) {
			CP("print \"Invalid position!\n\"");
			return;
		}
	} else {
		CP("print \"usage: load [position]\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		CP("cp \"You can not load as a spectator!\n\"");
		return;
	}

	if (ent->client->ps.eFlags & EF_PRONE) {
		CP("cp \"You can not load while proning!\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_ALLIES) {
		pos = ent->client->sess.alliesSaves + posNum;
	} else {
		pos = ent->client->sess.axisSaves + posNum;
	}

	if (pos->valid) {
		if (ent->client->timerunActive) {
			// Nico, notify the client and its spectators the timerun has stopped
			notify_timerun_stop(ent, 0);

			ent->client->timerunActive = qfalse;
		}

		VectorCopy(pos->origin, ent->client->ps.origin);

		SetClientViewAngle(ent, pos->vangles);

		VectorClear(ent->client->ps.velocity);

		if (ent->client->ps.stats[STAT_HEALTH] < 100 && ent->client->ps.stats[STAT_HEALTH] > 0) {
			ent->health = 100;
		}

		if (level.rocketRun && ent->client->ps.weapon == WP_PANZERFAUST) {
			ent->client->ps.ammoclip[WP_PANZERFAUST] = level.rocketRun;
		}
	} else {
		CP("cp \"Use save first!\n\"");
	}
}

void Cmd_Save_f(gentity_t *ent)
{
	char cmd[MAX_TOKEN_CHARS];
	int argc;
	int posNum;
	save_position_t *pos;

	// get save slot (do this first so players can get usage message even if
	// they are not allowed to use this command)
	argc = trap_Argc();
	if (argc == 1) {
		posNum = 0;
	} else if (argc == 2) {
		trap_Argv(1, cmd, sizeof(cmd));		
		if ((posNum = atoi(cmd)) < 0 || posNum >= MAX_SAVED_POSITIONS) {
			CP("print \"Invalid position!\n\"");
			return;
		}
	} else {
		CP("print \"usage: save [position]\n\"");
		return;
	}

	if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
		CP("cp \"You can not save as a spectator!\n\"");
		return;
	}

	if (ent->client->ps.groundEntityNum == ENTITYNUM_NONE) {
		CP("cp \"You can not save while in the air!\n\"");
		return;
	}

	if (ent->client->ps.eFlags & EF_PRONE || ent->client->ps.eFlags & EF_PRONE_MOVING){
		CP("cp \"You can not save while proning!\n\"");
		return;
	}

	if (ent->client->ps.eFlags & EF_CROUCHING){
		CP("cp \"You can not save while crouching!\n\"");
		return;
	}


	if (ent->client->sess.sessionTeam == TEAM_ALLIES) {
		pos = ent->client->sess.alliesSaves + posNum;
	} else {
		pos = ent->client->sess.axisSaves + posNum;
	}

	VectorCopy(ent->client->ps.origin, pos->origin);
	VectorCopy(ent->client->ps.viewangles, pos->vangles);
	pos->valid = qtrue;

	if (posNum == 0) {
		CP("cp \"Saved\n\"");
	} else {
		CP(va("cp \"Saved ^z%d\n\"", posNum));
	}
}

// Nico, ETrun login command
void Cmd_Login_f(gentity_t *ent) {
	char token[MAX_QPATH];
	char *result = NULL;

	// Check if API is used
	if (!g_useAPI.integer) {
		CP("cp \"Login is disabled on this server.\n\"");
		return;
	}

	if (!ent || !ent->client) {
		G_DPrintf("Cmd_Login_f: invalid ent: %d\n", (int)ent);
		return;
	}

	// Check if already logged in
	if (ent->client->sess.logged) {
		CP("cp \"You are already logged in!\n\"");
		G_DPrintf("Cmd_Login_f: client already logged in\n");
		return;
	}

	result = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!result) {
		G_Error("Cmd_Login_f: malloc failed\n");
	}

	Q_strncpyz(token, ent->client->pers.authToken, MAX_QPATH);

	G_DPrintf("Cmd_Login_f: token = %s, ent = %d, ent->client = %d\n", token, (int)ent, (int)ent->client);

	if (strlen(token) == 0) {
		CP("cp \"Empty auth token!\n\"");
		G_DPrintf("Cmd_Login_f: empty_token\n");
	} else {
		G_API_login(result, ent, token);
	}

	// Do not free result here!
}

// Nico, ETrun logout command
void Cmd_Logout_f(gentity_t *ent) {
	if (!ent || !ent->client) {
		G_DPrintf("Cmd_Login_f: invalid ent: %d\n", (int)ent);
		return;
	}

	// Check if already logged in
	if (!ent->client->sess.logged) {
		CP("cp \"You are not logged in!\n\"");
		return;
	}

	CP("cp \"You are no longer logged in!\n\"");
	ent->client->sess.logged = qfalse;
	ClientUserinfoChanged(ent->client->ps.clientNum);
	//#todo: release auth token?
}

// Nico, records command
void Cmd_Records_f(gentity_t *ent) {
	char *buf = NULL;

	// Check if API is used
	if (!g_useAPI.integer) {
		CP("cp \"This command is disabled on this server.\n\"");
		return;
	}

	buf = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!buf) {
		G_Error("Cmd_Records_f: malloc failed\n");
	}

	G_Printf("Asking for map record...\n");
	G_API_mapRecords(buf, ent, level.rawmapname);

	// Do *not* free buf here
}

// Nico, defines commands that are flood protected or not
static command_t floodProtectedCommands[] = {
	{ "score",				qfalse,	Cmd_Score_f },
	{ "vote",				qtrue,	Cmd_Vote_f },
	{ "fireteam",			qfalse,	Cmd_FireTeam_MP_f },
	{ "rconauth",			qfalse,	Cmd_AuthRcon_f },
	{ "ignore",				qfalse,	Cmd_Ignore_f },
	{ "unignore",			qfalse,	Cmd_UnIgnore_f },
	{ "obj",				qfalse,	Cmd_SelectedObjective_f },
	{ "rs",					qfalse,	Cmd_ResetSetup_f },
	{ "noclip",				qfalse,	Cmd_Noclip_f },
	{ "kill",				qtrue,	Cmd_Kill_f },
	{ "team",				qtrue,	Cmd_Team_f },
	{ "stopcamera",			qfalse,	Cmd_StopCamera_f },
	{ "setcameraorigin",	qfalse,	Cmd_SetCameraOrigin_f },
	{ "setspawnpt",			qfalse,	Cmd_SetSpawnPoint_f },
	{ "load",				qfalse,	Cmd_Load_f },
	{ "save",				qfalse,	Cmd_Save_f },

	// ETrun specific commands
	{ "login",				qtrue, Cmd_Login_f },
	{ "logout",				qtrue, Cmd_Logout_f },
	{ "records",			qtrue, Cmd_Records_f }
};
// Nico, end of defines commands that are flood protected or not

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum ) {
	gentity_t *ent;
	char cmd[MAX_TOKEN_CHARS];
	int i = 0;
	qboolean	enc = qfalse; // used for enc_say, enc_say_team, enc_say_buddy

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;     // not fully in game yet
	}

	trap_Argv( 0, cmd, sizeof( cmd ) );

	if (!Q_stricmp(cmd, "say") || (enc = !Q_stricmp(cmd, "enc_say"))) {

		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping say\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Say_f(ent, SAY_ALL, qfalse, enc);
		}
		return;
	}

	if (!Q_stricmp(cmd, "say_team") || (enc = !Q_stricmp(cmd, "enc_say_team"))) {
		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping say_team\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Say_f(ent, SAY_TEAM, qfalse, enc);
		}
		return;
	} else if (!Q_stricmp(cmd, "vsay")) {

		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping vsay\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Voice_f(ent, SAY_ALL, qfalse, qfalse);
		}
		return;
	} else if (!Q_stricmp(cmd, "vsay_team")) {
		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping vsay_team\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Voice_f(ent, SAY_TEAM, qfalse, qfalse);
		}
		return;
	} else if (!Q_stricmp(cmd, "say_buddy") || (enc = !Q_stricmp(cmd, "enc_say_buddy"))) {

		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping say_buddy\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Say_f(ent, SAY_BUDDY, qfalse, enc);
		}
		return;
	} else if (!Q_stricmp(cmd, "vsay_buddy")) {

		// Nico, flood protection
		if (ClientIsFlooding(ent)) {
			CP("print \"^1Spam Protection: ^7dropping vsay_buddy\n\"");
			return;
		}

		if (!ent->client->sess.muted) {
			Cmd_Voice_f(ent, SAY_BUDDY, qfalse, qfalse);
		}
		return;
	} 	
	else if (!Q_stricmp(cmd, "follownext")) {
		Cmd_FollowCycle_f(ent, 1);
	} else if (!Q_stricmp(cmd, "followprev")) {
		Cmd_FollowCycle_f(ent, -1);
	}

	// Nico, flood protection
	for (i = 0 ; i < sizeof(floodProtectedCommands) / sizeof(floodProtectedCommands[0]) ; ++i) {
		if (!Q_stricmp(cmd, floodProtectedCommands[i].cmd)) {
			if (floodProtectedCommands[i].isProtected && ClientIsFlooding(ent)) {
				CP(va("print \"^1Spam Protection: ^7dropping %s\n\"", cmd));
			} else {
				floodProtectedCommands[i].function(ent);
			}
			return;
		}
	}
		
	if (G_commandCheck(ent, cmd)) {
		return;
	}
	
	CP(va("print \"Unknown command %s^7.\n\"", cmd));
}

/*
 * Nico, anti-flood from ETpub
 * Returns qtrue if user requested too many commands in past time, otherwise
 * qfalse.
 */
qboolean ClientIsFlooding(gentity_t *ent) {
	if (!ent->client || !g_floodProtect.integer) {
		return qfalse;
	}

	if (level.time - ent->client->sess.thresholdTime > 30000) {
		ent->client->sess.thresholdTime = level.time;
	}

	if (level.time < ent->client->sess.nextReliableTime) {
		return qtrue;
	}

	if (level.time - ent->client->sess.thresholdTime <= 30000
			&& ent->client->sess.numReliableCmds > g_floodThreshold.integer) {
		ent->client->sess.nextReliableTime = level.time + g_floodWait.integer;
		return qtrue;
	}

	ent->client->sess.numReliableCmds++;	
	// delay between each command (values >0 break batch of commands)
	ent->client->sess.nextReliableTime = level.time + 0;

	return qfalse;
}
