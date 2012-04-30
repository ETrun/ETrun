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

// g_match.c: Match handling
// -------------------------
//
#include "g_local.h"
#include "../../etrun/ui/menudef.h"


void G_initMatch( void ) {
	int i;

	for ( i = TEAM_AXIS; i <= TEAM_ALLIES; i++ ) {
		G_teamReset( i, qfalse );
	}
}


// Setting initialization
void G_loadMatchGame( void ) {
	unsigned int i, dwBlueOffset, dwRedOffset;
	unsigned int aRandomValues[MAX_REINFSEEDS];
	char strReinfSeeds[MAX_STRING_CHARS];

	G_Printf( "Setting MOTD...\n" );
	trap_SetConfigstring( CS_CUSTMOTD + 0, server_motd0.string );
	trap_SetConfigstring( CS_CUSTMOTD + 1, server_motd1.string );
	trap_SetConfigstring( CS_CUSTMOTD + 2, server_motd2.string );
	trap_SetConfigstring( CS_CUSTMOTD + 3, server_motd3.string );
	trap_SetConfigstring( CS_CUSTMOTD + 4, server_motd4.string );
	trap_SetConfigstring( CS_CUSTMOTD + 5, server_motd5.string );

	// Voting flags
	G_voteFlags();

	// Set up the random reinforcement seeds for both teams and send to clients
	dwBlueOffset = rand() % MAX_REINFSEEDS;
	dwRedOffset = rand() % MAX_REINFSEEDS;
	strcpy( strReinfSeeds, va( "%d %d", ( dwBlueOffset << REINF_BLUEDELT ) + ( rand() % ( 1 << REINF_BLUEDELT ) ),
							   ( dwRedOffset << REINF_REDDELT )  + ( rand() % ( 1 << REINF_REDDELT ) ) ) );

	for ( i = 0; i < MAX_REINFSEEDS; i++ ) {
		aRandomValues[i] = ( rand() % REINF_RANGE ) * aReinfSeeds[i];
		strcat( strReinfSeeds, va( " %d", aRandomValues[i] ) );
	}

	trap_SetConfigstring( CS_REINFSEEDS, strReinfSeeds );
}


// Simple alias for sure-fire print :)
void G_printFull( char *str, gentity_t *ent ) {
	if ( ent != NULL ) {
		CP( va( "print \"%s\n\"", str ) );
		CP( va( "cp \"%s\n\"", str ) );
	} else {
		AP( va( "print \"%s\n\"", str ) );
		AP( va( "cp \"%s\n\"", str ) );
	}
}

// Plays specified sound globally.
void G_globalSound( char *sound ) {
	gentity_t *te = G_TempEntity( level.intermission_origin, EV_GLOBAL_SOUND );
	te->s.eventParm = G_SoundIndex( sound );
	te->r.svFlags |= SVF_BROADCAST;
}


void G_delayPrint( gentity_t *dpent ) {
	int think_next = 0;
	qboolean fFree = qtrue;

	switch ( dpent->spawnflags ) {
	case DP_PAUSEINFO:
	{
		break;
	}

	case DP_UNPAUSING:
	{
		if ( level.match_pause == PAUSE_UNPAUSING ) {
			int cSeconds = 11 * 1000 - ( level.time - dpent->timestamp );

			if ( cSeconds > 1000 ) {
				AP( va( "cp \"^3Match resuming in ^1%d^3 seconds!\n\"", cSeconds / 1000 ) );
				think_next = level.time + 1000;
				fFree = qfalse;
			} else {
				level.match_pause = PAUSE_NONE;
				G_globalSound( "sound/osp/fight.wav" );
				G_printFull( "^1FIGHT!", NULL );
				trap_SetConfigstring( CS_LEVEL_START_TIME, va( "%i", level.startTime + level.timeDelta ) );
				level.server_settings &= ~CV_SVS_PAUSE;
				trap_SetConfigstring( CS_SERVERTOGGLES, va( "%d", level.server_settings ) );
			}
		}
		break;
	}

	default:
		break;
	}

	dpent->nextthink = think_next;
	if ( fFree ) {
		dpent->think = 0;
		G_FreeEntity( dpent );
	}
}

static char *pszDPInfo[] = {
	"DPRINTF_PAUSEINFO",
	"DPRINTF_UNPAUSING",
	"DPRINTF_CONNECTINFO",
	"DPRINTF_UNK1",
	"DPRINTF_UNK2",
	"DPRINTF_UNK3",
	"DPRINTF_UNK4",
	"DPRINTF_UNK5"
};

void G_spawnPrintf( int print_type, int print_time, gentity_t *owner ) {
	gentity_t   *ent = G_Spawn();

	ent->classname = pszDPInfo[print_type];
	ent->clipmask = 0;
	ent->parent = owner;
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags |= EF_NODRAW;
	ent->s.eType = ET_ITEM;

	ent->spawnflags = print_type;       // Tunnel in DP enum
	ent->timestamp = level.time;        // Time entity was created

	ent->nextthink = print_time;
	ent->think = G_delayPrint;
}

// Dumps end-of-match info
void G_matchInfoDump( unsigned int dwDumpType ) {
	int i, ref;
	gentity_t *ent;
	gclient_t *cl;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		ref = level.sortedClients[i];
		ent = &g_entities[ref];
		cl = ent->client;

		if ( cl->pers.connected != CON_CONNECTED ) {
			continue;
		}
	}
}


// Update configstring for vote info
int G_checkServerToggle( vmCvar_t *cv ) {
	int nFlag;

	if ( cv == &g_antilag ) {
		nFlag = CV_SVS_ANTILAG;
	}
	
	else if ( cv == &g_nextmap ) {
		if ( *cv->string ) {
			level.server_settings |= CV_SVS_NEXTMAP;
		} else {
			level.server_settings &= ~CV_SVS_NEXTMAP;
		}
		return( qtrue );
	} 

	else {return( qfalse );}

	if ( cv->integer > 0 ) {
		level.server_settings |= nFlag;
	} else {
		level.server_settings &= ~nFlag;
	}

	return( qtrue );
}
