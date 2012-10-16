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
#include "../../etrun/ui/menudef.h"
#include "g_api.h"

// g_client.c -- client functions that don't happen every frame
vec3_t playerMins = {-18, -18, -24};
vec3_t playerMaxs = {18, 18, 48};
// done.

/*QUAKED info_player_deathmatch (1 0 1) (-18 -18 -24) (18 18 48)
potential spawning position for deathmatch games.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
If the start position is targeting an entity, the players camera will start out facing that ent (like an info_notnull)
*/
void SP_info_player_deathmatch( gentity_t *ent ) {
	int i;
	vec3_t dir;

	G_SpawnInt( "nobots", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	ent->enemy = G_PickTarget( ent->target );
	if ( ent->enemy ) {
		VectorSubtract( ent->enemy->s.origin, ent->s.origin, dir );
		vectoangles( dir, ent->s.angles );
	}

}

//----(SA) added
/*QUAKED info_player_checkpoint (1 0 0) (-16 -16 -24) (16 16 32) a b c d
these are start points /after/ the level start
the letter (a b c d) designates the checkpoint that needs to be complete in order to use this start position
*/
void SP_info_player_checkpoint( gentity_t *ent ) {
	ent->classname = "info_player_checkpoint";
	SP_info_player_deathmatch( ent );
}

//----(SA) end


/*QUAKED info_player_start (1 0 0) (-18 -18 -24) (18 18 48)
equivelant to info_player_deathmatch
*/
void SP_info_player_start( gentity_t *ent ) {
	ent->classname = "info_player_deathmatch";
	SP_info_player_deathmatch( ent );
}

/*
QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32) AXIS ALLIED
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {
	// Nico, silent GCC
	ent = ent;
}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int i, num;
	int touch[MAX_GENTITIES];
	gentity_t   *hit;
	vec3_t mins, maxs;

	VectorAdd( spot->r.currentOrigin, playerMins, mins );
	VectorAdd( spot->r.currentOrigin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for ( i = 0 ; i < num ; i++ ) {
		hit = &g_entities[touch[i]];
		if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
			return qtrue;
		}

	}

	return qfalse;
}

/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define MAX_SPAWN_POINTS    128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t   *spot;
	vec3_t delta;
	float dist, nearestDist;
	gentity_t   *nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ( ( spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" ) ) != NULL ) {

		VectorSubtract( spot->r.currentOrigin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define MAX_SPAWN_POINTS    128
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t   *spot;
	int count;
	int selection;
	gentity_t   *spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	while ( ( spot = G_Find( spot, FOFS( classname ), "info_player_deathmatch" ) ) != NULL ) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	if ( !count ) { // no spots that won't telefrag
		return G_Find( NULL, FOFS( classname ), "info_player_deathmatch" );
	}

	selection = rand() % count;
	return spots[ selection ];
}


/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint( vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	gentity_t   *spot;
	gentity_t   *nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint();
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint();
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint();
		}
	}

	// find a single player start spot
	if ( !spot ) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy( spot->r.currentOrigin, origin );
	origin[2] += 9;
	VectorCopy( spot->s.angles, angles );

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
=============
BodyUnlink

Called by BodySink
=============
*/
void BodyUnlink( gentity_t *ent ) {
	trap_UnlinkEntity( ent );
	ent->physicsObject = qfalse;
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink2( gentity_t *ent ) {
	ent->physicsObject = qfalse;
	ent->nextthink = level.time + BODY_TIME( BODY_TEAM( ent ) ) + 1500;
	ent->think = BodyUnlink;
	ent->s.pos.trType = TR_LINEAR;
	ent->s.pos.trTime = level.time;
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	VectorSet( ent->s.pos.trDelta, 0, 0, -8 );
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	if ( ent->activator ) {
		ent->activator = NULL;
	}

	BodySink2( ent );
}
//======================================================================


/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int i;

	// set the delta angle
	for ( i = 0 ; i < 3 ; i++ ) {
		int cmdAngle;

		cmdAngle = ANGLE2SHORT( angle[i] );
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy( ent->s.angles, ent->client->ps.viewangles );
}

void SetClientViewAnglePitch( gentity_t *ent, vec_t angle ) {
	int cmdAngle;

	cmdAngle = ANGLE2SHORT( angle );
	ent->client->ps.delta_angles[PITCH] = cmdAngle - ent->client->pers.cmd.angles[PITCH];

	ent->s.angles[ PITCH ] = 0;
	VectorCopy( ent->s.angles, ent->client->ps.viewangles );
}

/* JPW NERVE
================
limbo
================
*/
void limbo( gentity_t *ent) {
	int i;
	int startclient = ent->client->ps.clientNum;

	if ( ent->r.svFlags & SVF_POW ) {
		return;
	}

	if ( !( ent->client->ps.pm_flags & PMF_LIMBO ) ) {
		// DHM - Nerve :: First save off persistant info we'll need for respawn
		for ( i = 0; i < MAX_PERSISTANT; i++ ) {
			ent->client->saved_persistant[i] = ent->client->ps.persistant[i];
		}

		ent->client->ps.pm_flags |= PMF_LIMBO;
		ent->client->ps.pm_flags |= PMF_FOLLOW;

		trap_UnlinkEntity( ent );

		// DHM - Nerve :: reset these values
		ent->client->ps.viewlocked = 0;
		ent->client->ps.viewlocked_entNum = 0;

		ent->r.maxs[2] = 0;
		ent->r.currentOrigin[2] += 8;

		trap_PointContents( ent->r.currentOrigin, -1 ); // drop stuff

		ent->client->sess.spectatorClient = startclient;
	}
}

/* JPW NERVE
================
reinforce
================
// -- called when time expires for a team deployment cycle and there is at least one guy ready to go
*/
void reinforce( gentity_t *ent ) {
	int p, team;
	gclient_t *rclient;
	char userinfo[MAX_INFO_STRING], *respawnStr;

	if ( ent->r.svFlags & SVF_BOT ) {
		trap_GetUserinfo( ent->s.number, userinfo, sizeof( userinfo ) );
		respawnStr = Info_ValueForKey( userinfo, "respawn" );
		if ( !Q_stricmp( respawnStr, "no" ) || !Q_stricmp( respawnStr, "off" ) ) {
			return; // no respawns
		}
	}

	if ( !( ent->client->ps.pm_flags & PMF_LIMBO ) ) {
		G_Printf( "player already deployed, skipping\n" );
		return;
	}

	// get team to deploy from passed entity
	team = ent->client->sess.sessionTeam;

	// find number active team spawnpoints
	if ( team == TEAM_AXIS ) {
	} else if ( team == TEAM_ALLIES ) {
	} else {
		assert( 0 );
	}

	// DHM - Nerve :: restore persistant data now that we're out of Limbo
	rclient = ent->client;
	for ( p = 0; p < MAX_PERSISTANT; p++ )
		rclient->ps.persistant[p] = rclient->saved_persistant[p];
	// dhm

	respawn( ent );
}
// jpw


/*
================
respawn
================
*/
void respawn( gentity_t *ent ) {
	ent->client->ps.pm_flags &= ~PMF_LIMBO; // JPW NERVE turns off limbo

	ClientSpawn( ent );
}

// NERVE - SMF - merge from team arena
/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, int team ) {
	int i, ref, count = 0;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		if ( ( ref = level.sortedClients[i] ) == ignoreClientNum ) {
			continue;
		}
		if ( (int)level.clients[ref].sess.sessionTeam == team ) {
			count++;
		}
	}

	return( count );
}
// -NERVE - SMF

/*
================
PickTeam

================
*/
team_t PickTeam( int ignoreClientNum ) {
	int counts[TEAM_NUM_TEAMS] = { 0, 0, 0 };

	counts[TEAM_ALLIES] = TeamCount( ignoreClientNum, TEAM_ALLIES );
	counts[TEAM_AXIS] = TeamCount( ignoreClientNum, TEAM_AXIS );

	if ( counts[TEAM_ALLIES] > counts[TEAM_AXIS] ) {
		return( TEAM_AXIS );
	}
	if ( counts[TEAM_AXIS] > counts[TEAM_ALLIES] ) {
		return( TEAM_ALLIES );
	}

	// Nico, let's pick axis team, why not ? :)
	return TEAM_AXIS;
}

qboolean AddWeaponToPlayer( gclient_t *client, weapon_t weapon, int ammo, int ammoclip, qboolean setcurrent ) {
	COM_BitSet( client->ps.weapons, weapon );
	client->ps.ammoclip[BG_FindClipForWeapon( weapon )] = ammoclip;
	client->ps.ammo[BG_FindAmmoForWeapon( weapon )] = ammo;
	if ( setcurrent ) {
		client->ps.weapon = weapon;
	}

	return qtrue;
}

/*
===========
SetWolfSpawnWeapons
===========
*/
void SetWolfSpawnWeapons( gclient_t *client ) {
	int pc = client->sess.playerType;

	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		return;
	}

	// Reset special weapon time
	client->ps.classWeaponTime = -999999;

	// Communicate it to cgame
	client->ps.stats[STAT_PLAYER_CLASS] = pc;

	// Abuse teamNum to store player class as well (can't see stats for all clients in cgame)
	client->ps.teamNum = pc;

	// JPW NERVE -- zero out all ammo counts
	memset( client->ps.ammo, 0, MAX_WEAPONS * sizeof( int ) );

	// All players start with a knife (not OR-ing so that it clears previous weapons)
	client->ps.weapons[0] = 0;
	client->ps.weapons[1] = 0;

	AddWeaponToPlayer( client, WP_KNIFE, 1, 0, qtrue );

	client->ps.weaponstate = WEAPON_READY;

	// Nico, give binoculars to everyone
	if (AddWeaponToPlayer( client, WP_BINOCULARS, 1, 0, qfalse )) {
		client->ps.stats[STAT_KEYS] |= ( 1 << INV_BINOCS );
	}

	if ( pc == PC_ENGINEER ) {

		// Nico, give pliers
		AddWeaponToPlayer( client, WP_PLIERS, 0, 1, qfalse );

		if ( client->sess.sessionTeam == TEAM_AXIS ) {
			switch ( client->sess.playerWeapon ) {
			case WP_KAR98:
				AddWeaponToPlayer( client, WP_KAR98, GetAmmoTableData( WP_KAR98 )->defaultStartingAmmo, GetAmmoTableData( WP_KAR98 )->defaultStartingClip, qtrue );
				break;
			default:
				AddWeaponToPlayer( client, WP_MP40, GetAmmoTableData( WP_MP40 )->defaultStartingAmmo, GetAmmoTableData( WP_MP40 )->defaultStartingClip, qtrue );
				break;
			}

		} else {
			switch ( client->sess.playerWeapon ) {
			case WP_CARBINE:
				AddWeaponToPlayer( client, WP_CARBINE, GetAmmoTableData( WP_CARBINE )->defaultStartingAmmo, GetAmmoTableData( WP_CARBINE )->defaultStartingClip, qtrue );
				break;
			default:
				AddWeaponToPlayer( client, WP_THOMPSON, GetAmmoTableData( WP_THOMPSON )->defaultStartingAmmo, GetAmmoTableData( WP_THOMPSON )->defaultStartingClip, qtrue );
				break;
			}
		}
	} else if ( pc == PC_FIELDOPS ) {

		if ( client->sess.sessionTeam == TEAM_AXIS ) {
			AddWeaponToPlayer( client, WP_MP40,  GetAmmoTableData( WP_MP40 )->defaultStartingAmmo, GetAmmoTableData( WP_MP40 )->defaultStartingClip, qtrue );
		} else {
			AddWeaponToPlayer( client, WP_THOMPSON, GetAmmoTableData( WP_THOMPSON )->defaultStartingAmmo, GetAmmoTableData( WP_THOMPSON )->defaultStartingClip, qtrue );
		}
	} else if ( pc == PC_MEDIC ) {

		AddWeaponToPlayer( client, WP_MEDKIT, GetAmmoTableData( WP_MEDKIT )->defaultStartingAmmo, GetAmmoTableData( WP_MEDKIT )->defaultStartingClip, qfalse );

		if ( client->sess.sessionTeam == TEAM_AXIS ) {
			AddWeaponToPlayer( client, WP_MP40, 0, GetAmmoTableData( WP_MP40 )->defaultStartingClip, qtrue );
		} else {
			AddWeaponToPlayer( client, WP_THOMPSON, 0, GetAmmoTableData( WP_THOMPSON )->defaultStartingClip, qtrue );
		}
	} else if ( pc == PC_SOLDIER ) {

		switch ( client->sess.sessionTeam ) {
		case TEAM_AXIS:
			/* Nico, #todo rocket runs support
			switch ( client->sess.playerWeapon ) {
			default:
			case WP_FLAMETHROWER:
			case WP_MOBILE_MG42:
			case WP_MORTAR:
			case WP_MP40:*/
				AddWeaponToPlayer( client, WP_MP40, GetAmmoTableData( WP_MP40 )->defaultStartingAmmo, GetAmmoTableData( WP_MP40 )->defaultStartingClip, qtrue );
				/*break;

			case WP_PANZERFAUST:
				AddWeaponToPlayer( client, WP_PANZERFAUST, GetAmmoTableData( WP_PANZERFAUST )->defaultStartingAmmo, GetAmmoTableData( WP_PANZERFAUST )->defaultStartingClip, qtrue );
				break;
			}
			break;*/
		case TEAM_ALLIES:
			/* Nico, #todo rocket runs support
			switch ( client->sess.playerWeapon ) {
			default:
			// Nico, replaced weapons
			case WP_FLAMETHROWER:
			case WP_MOBILE_MG42:
			case WP_MORTAR:
			case WP_THOMPSON:*/
				AddWeaponToPlayer( client, WP_THOMPSON, GetAmmoTableData( WP_THOMPSON )->defaultStartingAmmo, GetAmmoTableData( WP_THOMPSON )->defaultStartingClip, qtrue );
				/*break;
			case WP_PANZERFAUST:
				AddWeaponToPlayer( client, WP_PANZERFAUST, GetAmmoTableData( WP_PANZERFAUST )->defaultStartingAmmo, GetAmmoTableData( WP_PANZERFAUST )->defaultStartingClip, qtrue );
				break;
			}
			break;*/
		default:
			break;
		}
	} else if ( pc == PC_COVERTOPS ) {
		switch ( client->sess.playerWeapon ) {
		case WP_K43:
		case WP_GARAND:
			if ( client->sess.sessionTeam == TEAM_AXIS ) {
				if ( AddWeaponToPlayer( client, WP_K43, GetAmmoTableData( WP_K43 )->defaultStartingAmmo, GetAmmoTableData( WP_K43 )->defaultStartingClip, qtrue ) ) {
					AddWeaponToPlayer( client, WP_K43_SCOPE, GetAmmoTableData( WP_K43_SCOPE )->defaultStartingAmmo, GetAmmoTableData( WP_K43_SCOPE )->defaultStartingClip, qfalse );
				}
				break;
			} else {
				if ( AddWeaponToPlayer( client, WP_GARAND, GetAmmoTableData( WP_GARAND )->defaultStartingAmmo, GetAmmoTableData( WP_GARAND )->defaultStartingClip, qtrue ) ) {
					AddWeaponToPlayer( client, WP_GARAND_SCOPE, GetAmmoTableData( WP_GARAND_SCOPE )->defaultStartingAmmo, GetAmmoTableData( WP_GARAND_SCOPE )->defaultStartingClip, qfalse );
				}
				break;
			}
		case WP_FG42:
			if ( AddWeaponToPlayer( client, WP_FG42, GetAmmoTableData( WP_FG42 )->defaultStartingAmmo, GetAmmoTableData( WP_FG42 )->defaultStartingClip, qtrue ) ) {
				AddWeaponToPlayer( client, WP_FG42SCOPE, GetAmmoTableData( WP_FG42SCOPE )->defaultStartingAmmo, GetAmmoTableData( WP_FG42SCOPE )->defaultStartingClip, qfalse );
			}
			break;
		default:
			AddWeaponToPlayer( client, WP_STEN, GetAmmoTableData( WP_STEN )->defaultStartingAmmo, GetAmmoTableData( WP_STEN )->defaultStartingClip, qtrue );
			break;
		}
	}

	// Nico, note: pistols are given here
	switch ( client->sess.sessionTeam ) {
	case TEAM_AXIS:
		switch ( pc ) {
		case PC_SOLDIER:
			AddWeaponToPlayer( client, WP_LUGER, GetAmmoTableData( WP_LUGER )->defaultStartingAmmo, GetAmmoTableData( WP_LUGER )->defaultStartingClip, qfalse );
			break;

		case PC_COVERTOPS:
			AddWeaponToPlayer( client, WP_LUGER, GetAmmoTableData( WP_LUGER )->defaultStartingAmmo, GetAmmoTableData( WP_LUGER )->defaultStartingClip, qfalse );
			AddWeaponToPlayer( client, WP_SILENCER, GetAmmoTableData( WP_SILENCER )->defaultStartingAmmo, GetAmmoTableData( WP_SILENCER )->defaultStartingClip, qfalse );
			client->pmext.silencedSideArm = 1;
			break;

		default:
			AddWeaponToPlayer( client, WP_LUGER, GetAmmoTableData( WP_LUGER )->defaultStartingAmmo, GetAmmoTableData( WP_LUGER )->defaultStartingClip, qfalse );
			break;
		}
		break;
	default:
		switch ( pc ) {
		case PC_SOLDIER:
			AddWeaponToPlayer( client, WP_COLT, GetAmmoTableData( WP_COLT )->defaultStartingAmmo, GetAmmoTableData( WP_COLT )->defaultStartingClip, qfalse );
			break;

		case PC_COVERTOPS:
			AddWeaponToPlayer( client, WP_COLT, GetAmmoTableData( WP_COLT )->defaultStartingAmmo, GetAmmoTableData( WP_COLT )->defaultStartingClip, qfalse );
			AddWeaponToPlayer( client, WP_SILENCED_COLT, GetAmmoTableData( WP_SILENCED_COLT )->defaultStartingAmmo, GetAmmoTableData( WP_SILENCED_COLT )->defaultStartingClip, qfalse );
			client->pmext.silencedSideArm = 1;
			break;

		default:
			AddWeaponToPlayer( client, WP_COLT, GetAmmoTableData( WP_COLT )->defaultStartingAmmo, GetAmmoTableData( WP_COLT )->defaultStartingClip, qfalse );
			break;
		}
	}
}

int G_CountTeamMedics( team_t team, qboolean alivecheck ) {
	int numMedics = 0;
	int i, j;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		j = level.sortedClients[i];

		if ( level.clients[j].sess.sessionTeam != team ) {
			continue;
		}

		if ( level.clients[j].sess.playerType != PC_MEDIC ) {
			continue;
		}

		if ( alivecheck ) {
			if ( g_entities[j].health <= 0 ) {
				continue;
			}

			if ( level.clients[j].ps.pm_type == PM_DEAD || level.clients[j].ps.pm_flags & PMF_LIMBO ) {
				continue;
			}
		}

		numMedics++;
	}

	return numMedics;
}

//
// AddMedicTeamBonus
//
void AddMedicTeamBonus( gclient_t *client ) {
	int numMedics = G_CountTeamMedics( client->sess.sessionTeam, qfalse );

	// compute health mod
	client->pers.maxHealth = 100 + 10 * numMedics;

	if ( client->pers.maxHealth > 125 ) {
		client->pers.maxHealth = 125;
	}

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
}

/*
===========
ClientCheckName
============
*/
static void ClientCleanName( const char *in, char *out, int outSize ) {
	int len, colorlessLen;
	char ch;
	char    *p;
	int spaces;

	//save room for trailing null byte
	outSize--;

	len = 0;
	colorlessLen = 0;
	p = out;
	*p = 0;
	spaces = 0;

	while ( 1 ) {
		ch = *in++;
		if ( !ch ) {
			break;
		}

		// don't allow leading spaces
		if ( !*p && ch == ' ' ) {
			continue;
		}

		// check colors
		if ( ch == Q_COLOR_ESCAPE ) {
			// solo trailing carat is not a color prefix
			if ( !*in ) {
				break;
			}

			// make sure room in dest for both chars
			if ( len > outSize - 2 ) {
				break;
			}

			*out++ = ch;
			*out++ = *in++;
			len += 2;
			continue;
		}

		// don't allow too many consecutive spaces
		if ( ch == ' ' ) {
			spaces++;
			if ( spaces > 3 ) {
				continue;
			}
		} else {
			spaces = 0;
		}

		if ( len > outSize - 1 ) {
			break;
		}

		*out++ = ch;
		colorlessLen++;
		len++;
	}
	*out = 0;

	// don't allow empty names
	if ( *p == 0 || colorlessLen == 0 ) {
		Q_strncpyz( p, "UnnamedPlayer", outSize );
	}
}

void G_StartPlayerAppropriateSound( gentity_t *ent, char *soundType ) {
	// Nico, silent GCC
	ent = ent;
	soundType = soundType;
}

// Nico, returns the IP is it's well-formed, NULL otherwise (from ETpub)
const char *getParsedIp(const char *ipadd) {
	// code by Dan Pop, http://bytes.com/forum/thread212174.html
	unsigned b1, b2, b3, b4, port = 0;
	unsigned char c;
	int rc;
	static char ipge[20];

	if (!Q_strncmp(ipadd,"localhost",strlen("localhost"))) {
		return "localhost";
	}

	rc = sscanf(ipadd, "%3u.%3u.%3u.%3u:%u%c", &b1, &b2, &b3, &b4, &port, &c);
	if (rc < 4 || rc > 5) {
		return NULL;
	}
	if ( (b1 | b2 | b3 | b4) > 255 || port > 65535) {
		return NULL;
	}
	if (strspn(ipadd, "0123456789.:") < strlen(ipadd)) {
		return NULL;
	}
	sprintf(ipge, "%u.%u.%u.%u", b1, b2, b3, b4);

	return ipge;
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent;
	char    *s;
	char oldname[MAX_STRING_CHARS];
	char userinfo[MAX_INFO_STRING];
	gclient_t   *client;
	size_t	len = 0;// Nico, userinfo length
	int	count = 0;// Nico, used in userinfo backslash count
	int i = 0;
	char *ip = NULL;// Nico, used to store client ip.
	char *name = NULL;// Nico, used to store client name
	char oldAuthToken[MAX_QPATH];// Nico, used to see if auth token was changed

	ent = g_entities + clientNum;
	client = ent->client;

	client->ps.clientNum = clientNum;

	// Nico, flood protection
	if (ClientIsFlooding(ent)) {
		G_LogPrintf("Dropping client %d: flooded userinfo\n", clientNum); 
		trap_DropClient(clientNum, "^1Flooded userinfo" , 0);
		return;
	}

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate( userinfo ) ) {
		// Nico, changing malformed user info is a nonsense, simply drop the client
		// Q_strncpyz( userinfo, "\\name\\badinfo", sizeof( userinfo ) );
		G_LogPrintf("Dropping client %d: forbidden character in userinfo\n", clientNum); 
		trap_DropClient(clientNum, "^1Forbidden character in userinfo" , 0);
	}

	// Nico, check userinfo length (from combinedfixes)
	len = strlen(userinfo);
	if (len > MAX_INFO_STRING - 44) {
		G_LogPrintf("Dropping client %d: oversized userinfo\n", clientNum); 
		trap_DropClient(clientNum, "^1Oversized userinfo" , 0);
	}

	// Nico, check userinfo leading backslash (from combinedfixes)
	if (userinfo[0] != '\\') {
		G_LogPrintf("Dropping client %d: malformed userinfo (missing leading backslash)\n", clientNum); 
		trap_DropClient(clientNum, "^1Malformed userinfo" , 0);
	}

	// Nico, check userinfo trailing backslash (from combinedfixes)
	if (len > 0 && userinfo[len - 1] == '\\') {
		G_LogPrintf("Dropping client %d: malformed userinfo (trailing backslash)\n", clientNum); 
		trap_DropClient(clientNum, "^1Malformed userinfo" , 0);
	}

	// Nico, make sure backslah number is even (from combinedfixes)
	for (i = 0; i < (int)len; ++i) {
		if (userinfo[i] == '\\') {
			count++;
		}
	}
	if (count % 2 != 0) {
		G_LogPrintf("Dropping client %d: malformed userinfo (odd number of backslash)\n", clientNum); 
		trap_DropClient(clientNum, "^1Malformed userinfo" , 0);
	}

	// Nico, make sure client ip is not empty or malformed (from combinedfixes)
	ip = Info_ValueForKey( userinfo, "ip" );
	if (!strcmp(ip, "") || getParsedIp(ip) == NULL) {
		G_LogPrintf("Dropping client %d: malformed userinfo (empty or malformed ip)\n", clientNum); 
		trap_DropClient(clientNum, "^1Malformed userinfo" , 0);
	}

	// Nico, make sure client name is not empty (from combinedfixes)
	name = Info_ValueForKey( userinfo, "name" );
	if (!strcmp(name, "")) {
		G_LogPrintf("Dropping client %d: malformed userinfo (empty name)\n", clientNum); 
		trap_DropClient(clientNum, "^1Malformed userinfo" , 0);
	}

	// Nico, one ip in userinfo (from ETpub)
	count = 0;
	if (len > 4) {
		for (i = 0; userinfo[i + 3]; ++i) {
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'i' &&
				userinfo[i + 2] == 'p' && userinfo[i + 3] == '\\') {
				count++;
			}
		}
	}
	if (count > 1) {
		G_LogPrintf("Dropping client %d: malformed userinfo (too many IP fields)\n", clientNum); 
		trap_DropClient(clientNum, "^1Malformed userinfo" , 0);
	} 

	// Nico, one cl_guid in userinfo (from ETpub)
	count = 0;
	if (len > 9) {
		for (i = 0; userinfo[i + 8]; ++i) {
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'c' &&
				userinfo[i + 2] == 'l' && userinfo[i + 3] == '_' &&
				userinfo[i + 4] == 'g' && userinfo[i + 5] == 'u' &&
				userinfo[i + 6] == 'i' && userinfo[i + 7] == 'd' &&
				userinfo[i + 8] == '\\') {
				count++;
			}
		}
	}
	if (count > 1) {
		G_LogPrintf("Dropping client %d: malformed userinfo (too many cl_guid fields)\n", clientNum); 
		trap_DropClient(clientNum, "^1Malformed userinfo" , 0);
	}

	// Nico, one name in userinfo (from ETpub)
	count = 0;
	if (len > 6) {
		for (i = 0; userinfo[i + 5]; ++i) {
			if (userinfo[i] == '\\' && userinfo[i + 1] == 'n' &&
				userinfo[i + 2] == 'a' && userinfo[i + 3] == 'm' &&
				userinfo[i + 4] == 'e' && userinfo[i + 5] == '\\') {
				count++;
			}
		}
	}
	if (count > 1) {
		G_LogPrintf("Dropping client %d: malformed userinfo (too many name fields)\n", clientNum); 
		trap_DropClient(clientNum, "^1Malformed userinfo" , 0);
	}

	if ( g_developer.integer || *g_log.string || g_dedicated.integer ) 
	{
		G_Printf( "Userinfo: %s\n", userinfo );
	}

	// check for local client
	if (ip && !strcmp( ip, "localhost" ) ) {
		client->pers.localClient = qtrue;
		level.fLocalHost = qtrue;
		client->sess.referee = RL_REFEREE;
	}

	// Nico, backup old auth token
	Q_strncpyz(oldAuthToken, client->pers.authToken, sizeof (oldAuthToken));

	s = Info_ValueForKey( userinfo, "cg_uinfo" );
	sscanf( s, "%i %i %i %i %s %i %i %i %i %i",
			&client->pers.clientFlags,
			&client->pers.clientTimeNudge,
			&client->pers.clientMaxPackets,

			// Nico, max FPS
			&client->pers.maxFPS,

			// Nico, auth Token
			(char *)&client->pers.authToken,

			// Nico, load view angles on load
			&client->pers.loadViewAngles,

			// Nico, load position when player dies
			&client->pers.autoLoad,

			// Nico, cgaz
			&client->pers.cgaz,

			// Nico, hideme
			&client->pers.hideme,

			// Nico, client auto demo record setting
			&client->pers.autoDemo

			);

	// Nico, check if auth token was changed
	if (strlen(oldAuthToken) > 0 && Q_stricmp(oldAuthToken, client->pers.authToken)) {
		// Nico, auth token was changed => logout player if he was logged in
		if (client->sess.logged) {
			CP("cp \"You are no longer logged in!\n\"");
			G_LogPrintf("ClientUserinfoChanged: authToken changed for client %d, forcing logout\n", clientNum);
			ent->client->sess.logged = qfalse;
		}
	}

	client->pers.autoActivate = ( client->pers.clientFlags & CGF_AUTOACTIVATE ) ? PICKUP_TOUCH : PICKUP_ACTIVATE;
	client->pers.predictItemPickup = ( ( client->pers.clientFlags & CGF_PREDICTITEMS ) != 0 );

	if ( client->pers.clientFlags & CGF_AUTORELOAD ) {
		client->pers.bAutoReloadAux = qtrue;
		client->pmext.bAutoReload = qtrue;
	} else {
		client->pers.bAutoReloadAux = qfalse;
		client->pmext.bAutoReload = qfalse;
	}

	// Nico, pmove_fixed
	client->pers.pmoveFixed = client->pers.clientFlags & CGF_PMOVEFIXED;

	// Nico, autologin
	client->pers.autoLogin = client->pers.clientFlags & CGF_AUTOLOGIN;

	// set name
	Q_strncpyz( oldname, client->pers.netname, sizeof( oldname ) );
	ClientCleanName( name, client->pers.netname, sizeof( client->pers.netname ) );

	if ( client->pers.connected == CON_CONNECTED && strcmp( oldname, client->pers.netname ) != 0 ) {
		// Nico, name changes limit
		if (g_maxNameChanges.integer > -1 && client->pers.nameChanges >= g_maxNameChanges.integer) {
			Q_strncpyz( client->pers.netname, oldname, sizeof( client->pers.netname ) );
			Info_SetValueForKey( userinfo, "name", oldname);
			trap_SetUserinfo( clientNum, userinfo );
			CPx(clientNum, "print \"^1You had too many namechanges\n\"");
			G_LogPrintf("Client %d name change refused\n", clientNum); 
			return;
		} else {
			client->pers.nameChanges++;
			trap_SendServerCommand( -1, va( "print \"[lof]%s" S_COLOR_WHITE " [lon]renamed to[lof] %s\n\"", oldname,
										client->pers.netname ) );
		}
	}

	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;

	// To communicate it to cgame
	client->ps.stats[ STAT_PLAYER_CLASS ] = client->sess.playerType;
	// Gordon: Not needed any more as it's in clientinfo?

	// send over a subset of the userinfo keys so other clients can
	// print scoreboards, display models, and play custom sounds

	s = va( "n\\%s\\t\\%i\\c\\%i\\w\\%i\\lw\\%i\\sw\\%i\\mu\\%i\\ref\\%i\\pm\\%i\\l\\%i\\h\\%i",
	client->pers.netname,
	client->sess.sessionTeam,
	client->sess.playerType,
	client->sess.playerWeapon,
	client->sess.latchPlayerWeapon,
	client->sess.latchPlayerWeapon2,
	client->sess.muted ? 1 : 0,
	client->sess.referee,
	client->pers.pmoveFixed ? 1 : 0,// Nico, pmove_fixed
	client->sess.logged ? 1 : 0,// Nico, login status
	client->pers.hideme// Nico, hideme
	);

	trap_GetConfigstring( CS_PLAYERS + clientNum, oldname, sizeof( oldname ) );

	trap_SetConfigstring( CS_PLAYERS + clientNum, s );

	if ( !Q_stricmp( oldname, s ) ) {
		return;
	}

	G_LogPrintf( "ClientUserinfoChanged: %i %s\n", clientNum, s );
	G_DPrintf( "ClientUserinfoChanged: %i :: %s\n", clientNum, s );
}


/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char        *value;
	gclient_t   *client;
	char userinfo[MAX_INFO_STRING];
	gentity_t   *ent;
	char userinfo2[MAX_INFO_STRING];// Nico, used in connections limit check
	int i = 0;
	int clientNum2;// Nico, used in connections limit check
	int conn_per_ip = 1;// Nico, connections per IP counter
	char ip[20], ip2[20];// Nico, used in connections limit check

	ent = &g_entities[ clientNum ];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// IP filtering
	// show_bug.cgi?id=500
	// recommanding PB based IP / GUID banning, the builtin system is pretty limited
	// check to see if they are on the banned IP list
	value = Info_ValueForKey( userinfo, "ip" );
	if ( G_FilterIPBanPacket( value ) ) {
		return "You are banned from this server.";
	}

	// Nico, check maximum connextions per IP (from ETpub)
	// (prevents fakeplayers DOS http://aluigi.altervista.org/fakep.htm )
	// note: value is the client ip
	Q_strncpyz(ip, getParsedIp(value), sizeof(ip));
	for (i = 0; i < level.numConnectedClients; ++i) {
		clientNum2 = level.sortedClients[i];
		if (clientNum == clientNum2) {
			continue;
		}
		trap_GetUserinfo(clientNum2, userinfo2,	sizeof(userinfo2));
		value = Info_ValueForKey (userinfo2, "ip");
		Q_strncpyz(ip2, getParsedIp(value), sizeof(ip2));
		if (strcmp(ip, ip2) == 0) {
			conn_per_ip++;
		}
	}
	if (conn_per_ip > g_maxConnsPerIP.integer) {
		G_LogPrintf("ETrun: possible DoS attack, rejecting client from %s (%d connections already)\n", ip, g_maxConnsPerIP.integer);
		return "Too many connections from your IP.";
	}
	// Nico, end of check maximum connextions per IP

	// we don't check password for bots and local client
	// NOTE: local client <-> "ip" "localhost"
	//   this means this client is not running in our current process
	if ( !isBot && !( ent->r.svFlags & SVF_BOT ) && ( strcmp( Info_ValueForKey( userinfo, "ip" ), "localhost" ) != 0 ) ) {
		// check for a password
		value = Info_ValueForKey( userinfo, "password" );
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) && strcmp( g_password.string, value ) != 0 ) {
			if ( !sv_privatepassword.string[ 0 ] || strcmp( sv_privatepassword.string, value ) != 0 ) {
				return "Invalid password";
			}
		}
	}

	// Gordon: porting q3f flag bug fix
	// If a player reconnects quickly after a disconnect, the client disconnect may never be called, thus flag can get lost in the ether
	if ( ent->inuse ) {
		G_LogPrintf( "Forcing disconnect on active client: %d\n", (int)(ent - g_entities) );
		// so lets just fix up anything that should happen on a disconnect
		ClientDisconnect( ent - g_entities );
	}

	// they can connect
	ent->client = level.clients + clientNum;
	client = ent->client;

	memset( client, 0, sizeof( *client ) );

	client->pers.connected = CON_CONNECTING;
	client->pers.connectTime = level.time;          // DHM - Nerve

	if ( firstTime ) {
		client->pers.initialSpawn = qtrue;              // DHM - Nerve

	}
	// read or initialize the session data
	if ( firstTime ) {
		G_InitSessionData( client, userinfo );
		client->pers.enterTime = level.time;
		client->ps.persistant[PERS_SCORE] = 0;
	} else {
		G_ReadSessionData( client );
	}
	client->pers.enterTime = level.time;

	if ( firstTime ) {
		// force into spectator
		client->sess.sessionTeam = TEAM_SPECTATOR;
		client->sess.spectatorState = SPECTATOR_FREE;
		client->sess.spectatorClient = 0;

		// unlink the entity - just in case they were already connected
		trap_UnlinkEntity( ent );
	}

	// get and distribute relevent paramters
	G_LogPrintf( "ClientConnect: %i\n", clientNum );
	G_UpdateCharacter( client );
	ClientUserinfoChanged( clientNum );

	// don't do the "xxx connected" messages if they were caried over from previous level
	//		TAT 12/10/2002 - Don't display connected messages in single player

	if ( firstTime ) {
		trap_SendServerCommand( -1, va( "cpm \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname ) );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	return NULL;
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum ) {
	gentity_t   *ent;
	gclient_t   *client;
	int flags;
	int spawn_count;

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}

	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.teamState.state = TEAM_BEGIN;

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	// DHM - Nerve :: Also save PERS_SPAWN_COUNT, so that CG_Respawn happens

	spawn_count = client->ps.persistant[PERS_SPAWN_COUNT];

	//bani - proper fix for #328

	flags = client->ps.eFlags;
	memset( &client->ps, 0, sizeof( client->ps ) );
	client->ps.eFlags = flags;

	client->ps.persistant[PERS_SPAWN_COUNT] = spawn_count;

	// locate ent at a spawn point
	ClientSpawn( ent );


	// DHM - Nerve :: Start players in limbo mode if they change teams during the match
	if ( client->sess.sessionTeam != TEAM_SPECTATOR && ( level.time - level.startTime > FRAMETIME * GAME_INIT_FRAMES ) ) {
		ent->health = 0;
		ent->r.contents = CONTENTS_CORPSE;

		client->ps.pm_type = PM_DEAD;
		client->ps.stats[STAT_HEALTH] = 0;

		limbo( ent );
	}

	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
		trap_SendServerCommand( -1, va( "print \"[lof]%s" S_COLOR_WHITE " [lon]entered the game\n\"", client->pers.netname ) );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	// No surface determined yet.
	ent->surfaceFlags = 0;

	// Nico, check for autologin
	if (g_useAPI.integer && client->pers.autoLogin && !client->sess.logged) {
		G_LogPrintf("ClientBegin: login client %d via autoLogin\n", clientNum);
		Cmd_Login_f(ent);
	}

	G_LogPrintf( "ClientBegin: %i\n", clientNum );
}

gentity_t *SelectSpawnPointFromList( char *list, vec3_t spawn_origin, vec3_t spawn_angles ) {
	char *pStr, *token;
	gentity_t   *spawnPoint = NULL, *trav;
	#define MAX_SPAWNPOINTFROMLIST_POINTS   16
	int valid[MAX_SPAWNPOINTFROMLIST_POINTS];
	int numValid;

	memset( valid, 0, sizeof( valid ) );
	numValid = 0;

	pStr = list;
	while ( ( token = COM_Parse( &pStr ) ) != NULL && token[0] ) {
		trav = g_entities + level.maxclients;
		while ( ( trav = G_FindByTargetname( trav, token ) ) != NULL ) {
			if ( !spawnPoint ) {
				spawnPoint = trav;
			}
			if ( !SpotWouldTelefrag( trav ) ) {
				valid[numValid++] = trav->s.number;
				if ( numValid >= MAX_SPAWNPOINTFROMLIST_POINTS ) {
					break;
				}
			}
		}
	}

	if ( numValid ) {
		spawnPoint = &g_entities[valid[rand() % numValid]];

		// Set the origin of where the bot will spawn
		VectorCopy( spawnPoint->r.currentOrigin, spawn_origin );
		spawn_origin[2] += 9;

		// Set the angle we'll spawn in to
		VectorCopy( spawnPoint->s.angles, spawn_angles );
	}

	return spawnPoint;
}

/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn( gentity_t *ent ) {
	int index;
	vec3_t spawn_origin, spawn_angles;
	gclient_t   *client;
	int i;
	clientPersistant_t saved;
	clientSession_t savedSess;
	int persistant[MAX_PERSISTANT];
	gentity_t   *spawnPoint;
	int flags;
	int savedPing;
	int savedTeam;
	qboolean update = qfalse;
	save_position_t *pos = NULL;

	index = ent - g_entities;
	client = ent->client;

	G_UpdateSpawnCounts();

	client->pers.lastSpawnTime = level.time;

	// Arnout: let's just be sure it does the right thing at all times. (well maybe not the right thing, but at least not the bad thing!)
	//if( client->sess.sessionTeam == TEAM_SPECTATOR || client->sess.sessionTeam == TEAM_FREE ) {
	if ( client->sess.sessionTeam != TEAM_AXIS && client->sess.sessionTeam != TEAM_ALLIES ) {
		spawnPoint = SelectSpectatorSpawnPoint( spawn_origin, spawn_angles );
	} else {
		// RF, if we have requested a specific spawn point, use it (fixme: what if this will place us inside another character?)
		spawnPoint = SelectCTFSpawnPoint( client->sess.sessionTeam, client->pers.teamState.state, spawn_origin, spawn_angles, client->sess.spawnObjectiveIndex );
	}

	client->pers.teamState.state = TEAM_ACTIVE;

	// toggle the teleport bit so the client knows to not lerp
	flags = ent->client->ps.eFlags & EF_TELEPORT_BIT;
	flags ^= EF_TELEPORT_BIT;
	flags |= ( client->ps.eFlags & EF_VOTED );
	// clear everything but the persistant data

	ent->s.eFlags &= ~EF_MOUNTEDTANK;

	// Nico, notify timerun_stop (not if physics is VET and player just selfkilled)
	if (physics.integer != PHYSICS_MODE_VET || (physics.integer == PHYSICS_MODE_VET && client->sess.lastDieWasASelfkill)) {
		notify_timerun_stop(ent, 0);
		ent->client->sess.timerunActive = qfalse;
	}

	saved           = client->pers;
	savedSess       = client->sess;
	savedPing       = client->ps.ping;
	savedTeam       = client->ps.teamNum;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		persistant[i] = client->ps.persistant[i];
	}

	memset( client, 0, sizeof( *client ) );

	client->pers            = saved;
	client->sess            = savedSess;
	client->ps.ping         = savedPing;
	client->ps.teamNum      = savedTeam;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}

	// increment the spawncount so the client will detect the respawn

	client->ps.persistant[PERS_SPAWN_COUNT]++;

	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;
	client->ps.persistant[PERS_HWEAPON_USE] = 0;

	client->airOutTime = level.time + 12000;

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	if ( ent->r.svFlags & SVF_BOT ) {
		ent->classname = "bot";
	} else {
		ent->classname = "player";
	}
	ent->r.contents = CONTENTS_BODY;

	ent->clipmask = MASK_PLAYERSOLID;

	// DHM - Nerve :: Init to -1 on first spawn;
	ent->props_frame_state = -1;

	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	VectorCopy( playerMins, ent->r.mins );
	VectorCopy( playerMaxs, ent->r.maxs );

	// Ridah, setup the bounding boxes and viewheights for prediction
	VectorCopy( ent->r.mins, client->ps.mins );
	VectorCopy( ent->r.maxs, client->ps.maxs );

	client->ps.crouchViewHeight = CROUCH_VIEWHEIGHT;
	client->ps.standViewHeight = DEFAULT_VIEWHEIGHT;
	client->ps.deadViewHeight = DEAD_VIEWHEIGHT;

	client->ps.crouchMaxZ = client->ps.maxs[2] - ( client->ps.standViewHeight - client->ps.crouchViewHeight );

	client->ps.runSpeedScale = 0.8;
	client->ps.sprintSpeedScale = 1.1;
	client->ps.crouchSpeedScale = 0.25;
	client->ps.weaponstate = WEAPON_READY;

	// Rafael

	client->ps.friction = 1.0;
	// done.

	// TTimo
	// retrieve from the persistant storage (we use this in pmoveExt_t beause we need it in bg_*)
	client->pmext.bAutoReload = client->pers.bAutoReloadAux;
	// done

	client->ps.clientNum = index;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );  // NERVE - SMF - moved this up here

	if ( client->sess.playerType != client->sess.latchPlayerType ) {
		update = qtrue;
	}

	client->sess.playerType = client->sess.latchPlayerType;

	if ( client->sess.playerWeapon != client->sess.latchPlayerWeapon ) {
		client->sess.playerWeapon = client->sess.latchPlayerWeapon;
		update = qtrue;
	}

	client->sess.playerWeapon2 = client->sess.latchPlayerWeapon2;

	if ( update ) {
		ClientUserinfoChanged( index );
	}


	G_UpdateCharacter( client );

	SetWolfSpawnWeapons( client );

	// START	Mad Doctor I changes, 8/17/2002

	// JPW NERVE -- increases stats[STAT_MAX_HEALTH] based on # of medics in game
	AddMedicTeamBonus( client );

	// END		Mad Doctor I changes, 8/17/2002

	client->pers.cmd.weapon = ent->client->ps.weapon;
// dhm - end

	// JPW NERVE ***NOTE*** the following line is order-dependent and must *FOLLOW* SetWolfSpawnWeapons() in multiplayer
	// AddMedicTeamBonus() now adds medic team bonus and stores in ps.stats[STAT_MAX_HEALTH].

	ent->health = client->ps.stats[STAT_HEALTH] = client->ps.stats[STAT_MAX_HEALTH];

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;

	SetClientViewAngle( ent, spawn_angles );

	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		trap_LinkEntity( ent );
	}

	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;
	client->latched_wbuttons = 0;   //----(SA)	added

	// xkan, 1/13/2003 - reset death time
	client->deathTime = 0;

	// fire the targets of the spawn point
	G_UseTargets( spawnPoint, ent );

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent - g_entities );

	// positively link the client, even if the command times are weird
	if ( ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	// run the presend to set anything else
	ClientEndFrame( ent );

	// set idle animation on weapon
	ent->client->ps.weapAnim = ( ( ent->client->ps.weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | PM_IdleAnimForWeapon( ent->client->ps.weapon );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

	// show_bug.cgi?id=569
	G_ResetMarkers( ent );

	// RF, start the scripting system
	if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {

		// RF, call entity scripting event
		G_Script_ScriptEvent( ent, "playerstart", "" );
	}

	if (ent->client->pers.autoLoad && !ent->client->sess.lastDieWasASelfkill && (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES)) {
		if (ent->client->sess.sessionTeam == TEAM_ALLIES) {
			pos = ent->client->sess.alliesSaves;
		} else {
			pos = ent->client->sess.axisSaves;
		}

		if (pos->valid) {
			VectorCopy(pos->origin, ent->client->ps.origin);

			// Nico, load angles if cg_loadViewAngles = 1
			if (ent->client->pers.loadViewAngles) {
				SetClientViewAngle(ent, pos->vangles);
			}

			VectorClear(ent->client->ps.velocity);

			if (ent->client->ps.stats[STAT_HEALTH] < 100 && ent->client->ps.stats[STAT_HEALTH] > 0) {
				ent->health = 100;
			}
		}
	}
}


/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t   *ent;
	gentity_t   *flag = NULL;
	gitem_t     *item = NULL;
	vec3_t launchvel;
	int i;

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

	G_RemoveClientFromFireteams( clientNum, qtrue, qfalse );
	G_RemoveFromAllIgnoreLists( clientNum );
	G_LeaveTank( ent, qfalse );

	// Nico, remove the client from all specInvited lists
	for (i = 0; i < level.numConnectedClients; ++i) {
		COM_BitClear(level.clients[level.sortedClients[i]].sess.specInvitedClients, clientNum);
	}

	// stop any following clients
	for ( i = 0 ; i < level.numConnectedClients ; i++ ) {
		flag = g_entities + level.sortedClients[i];
		if ( flag->client->sess.sessionTeam == TEAM_SPECTATOR
			 && flag->client->sess.spectatorState == SPECTATOR_FOLLOW
			 && flag->client->sess.spectatorClient == clientNum ) {
			StopFollowing( flag );
		}
		if ( flag->client->ps.pm_flags & PMF_LIMBO
			 && flag->client->sess.spectatorClient == clientNum ) {
			Cmd_FollowCycle_f( flag, 1 );
		}
	}

	G_FadeItems( ent, MOD_SATCHEL );

	// remove ourself from teamlists
	{
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

	// send effect if they were completely connected
	if ( ent->client->pers.connected == CON_CONNECTED
		 && ent->client->sess.sessionTeam != TEAM_SPECTATOR
		 && !( ent->client->ps.pm_flags & PMF_LIMBO ) ) {

		// They don't get to take powerups with them!
		// Especially important for stuff like CTF flags
		// New code for tossing flags
		if ( ent->client->ps.powerups[PW_REDFLAG] ) {
			item = BG_FindItem( "Red Flag" );
			if ( !item ) {
				item = BG_FindItem( "Objective" );
			}

			ent->client->ps.powerups[PW_REDFLAG] = 0;
		}
		if ( ent->client->ps.powerups[PW_BLUEFLAG] ) {
			item = BG_FindItem( "Blue Flag" );
			if ( !item ) {
				item = BG_FindItem( "Objective" );
			}

			ent->client->ps.powerups[PW_BLUEFLAG] = 0;
		}

		if ( item ) {
			// OSP - fix for suicide drop exploit through walls/gates
			launchvel[0] = 0;    //crandom()*20;
			launchvel[1] = 0;    //crandom()*20;
			launchvel[2] = 0;    //10+random()*10;

			flag = LaunchItem( item,ent->r.currentOrigin,launchvel,ent - g_entities );
			flag->s.modelindex2 = ent->s.otherEntityNum2;    // JPW NERVE FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
			flag->message = ent->message;       // DHM - Nerve :: also restore item name
			// Clear out player's temp copies
			ent->s.otherEntityNum2 = 0;
			ent->message = NULL;
		}
	}

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

	trap_UnlinkEntity( ent );
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	i = ent->client->sess.sessionTeam;
	ent->client->sess.sessionTeam = TEAM_FREE;
	ent->active = 0;

	trap_SetConfigstring( CS_PLAYERS + clientNum, "" );

	CalculateRanks();
}

// In just the GAME DLL, we want to store the groundtrace surface stuff,
// so we don't have to keep tracing.
void ClientStoreSurfaceFlags
(
	int clientNum,
	int surfaceFlags
) {
	// Store the surface flags
	g_entities[clientNum].surfaceFlags = surfaceFlags;

}
