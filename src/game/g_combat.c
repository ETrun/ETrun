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
 * name:		g_combat.c
 *
 * desc:
 *
*/

#include "g_local.h"
#include "../game/q_shared.h"

extern vec3_t muzzleTrace;

/*
============
AddScore

Adds score to both the client and his team
============
*/
// Nico, #fixme useless?
void AddScore( gentity_t *ent, int score ) {
	if ( !ent || !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( g_gamestate.integer != GS_PLAYING ) {
		return;
	}

	ent->client->sess.game_points += score;

	CalculateRanks();
}

/*
============
AddKillScore

Adds score to both the client and his team, only used for playerkills, for lms
============
*/
void AddKillScore( gentity_t *ent, int score ) {
	if ( !ent || !ent->client ) {
		return;
	}

	ent->client->sess.game_points += score;

	CalculateRanks();
}

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems( gentity_t *self ) {
	weapon_t primaryWeapon;

	primaryWeapon = G_GetPrimaryWeaponForClient( self->client );

	if ( primaryWeapon ) {
		// drop our primary weapon
		G_DropWeapon( self, primaryWeapon );
	}
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t dir;
	// vec3_t angles; Nico, unused warning fix

	if ( attacker && attacker != self ) {
		VectorSubtract( attacker->s.pos.trBase, self->s.pos.trBase, dir );
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract( inflictor->s.pos.trBase, self->s.pos.trBase, dir );
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw( dir );
}

/*
==================
GibEntity
==================
*/
void GibEntity( gentity_t *self, int killer ) {
	gentity_t *other = &g_entities[killer];
	vec3_t dir;

	VectorClear( dir );
	if ( other->inuse ) {
		if ( other->client ) {
			VectorSubtract( self->r.currentOrigin, other->r.currentOrigin, dir );
			VectorNormalize( dir );
		} else if ( !VectorCompare( other->s.pos.trDelta, vec3_origin ) ) {
			VectorNormalize2( other->s.pos.trDelta, dir );
		}
	}

	G_AddEvent( self, EV_GIB_PLAYER, DirToByte( dir ) );
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

/*
==================
body_die
==================
*/
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( self->health <= GIB_HEALTH ) {
		GibEntity( self, 0 );
	}
}


// these are just for logging, the client prints its own messages
char *modNames[] =
{
	"MOD_UNKNOWN",
	"MOD_MACHINEGUN",
	"MOD_BROWNING",
	"MOD_MG42",
	"MOD_GRENADE",
	"MOD_ROCKET",

	// (SA) modified wolf weap mods
	"MOD_KNIFE",
	"MOD_LUGER",
	"MOD_COLT",
	"MOD_MP40",
	"MOD_THOMPSON",
	"MOD_STEN",
	"MOD_GARAND",
	"MOD_SNOOPERSCOPE",
	"MOD_SILENCER",  //----(SA)
	"MOD_FG42",
	"MOD_FG42SCOPE",
	"MOD_PANZERFAUST",
	"MOD_GRENADE_LAUNCHER",
	"MOD_FLAMETHROWER",
	"MOD_GRENADE_PINEAPPLE",
	"MOD_CROSS",
	// end

	"MOD_MAPMORTAR",
	"MOD_MAPMORTAR_SPLASH",

	"MOD_KICKED",
	"MOD_GRABBER",

	"MOD_DYNAMITE",
	"MOD_AIRSTRIKE", // JPW NERVE
	"MOD_SYRINGE",   // JPW NERVE
	"MOD_AMMO",  // JPW NERVE
	"MOD_ARTY",  // JPW NERVE

	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
	"MOD_EXPLOSIVE",

	"MOD_CARBINE",
	"MOD_KAR98",
	"MOD_GPG40",
	"MOD_M7",
	"MOD_LANDMINE",
	"MOD_SATCHEL",
	"MOD_TRIPMINE",
	"MOD_SMOKEBOMB",
	"MOD_MOBILE_MG42",
	"MOD_SILENCED_COLT",
	"MOD_GARAND_SCOPE",

	"MOD_CRUSH_CONSTRUCTION",
	"MOD_CRUSH_CONSTRUCTIONDEATH",
	"MOD_CRUSH_CONSTRUCTIONDEATH_NOATTACKER",

	"MOD_K43",
	"MOD_K43_SCOPE",

	"MOD_MORTAR",

	"MOD_AKIMBO_COLT",
	"MOD_AKIMBO_LUGER",
	"MOD_AKIMBO_SILENCEDCOLT",
	"MOD_AKIMBO_SILENCEDLUGER",

	"MOD_SMOKEGRENADE",

	// RF
	"MOD_SWAP_PLACES",

	// OSP -- keep these 2 entries last
	"MOD_SWITCHTEAM"
};

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	int contents = 0, i, killer = ENTITYNUM_WORLD;
	char        *killerName = "<world>";
	qboolean nogib = qtrue;
	gitem_t     *item = NULL;
	gentity_t   *ent;
	qboolean killedintank = qfalse;

	weapon_t weap = BG_WeaponForMOD( meansOfDeath );

	if ( attacker == self ) {
		if ( self->client ) {

			trap_PbStat( self - g_entities, "suicide",
						 va( "%d %d %d", self->client->sess.sessionTeam, self->client->sess.playerType, weap ) ) ;
		}
	} else if ( OnSameTeam( self, attacker ) ) {
	} else {
		if ( g_gamestate.integer == GS_PLAYING ) {
			if ( attacker->client ) {
				attacker->client->combatState |= ( 1 << COMBATSTATE_KILLEDPLAYER );
			}
		}
	}

	// RF, record this death in AAS system so that bots avoid areas which have high death rates
	if ( !OnSameTeam( self, attacker ) ) {
		self->isProp = qfalse;  // were we teamkilled or not?
	} else {
		self->isProp = qtrue;
	}

	// if we got killed by a landmine, update our map
	if ( self->client && meansOfDeath == MOD_LANDMINE ) {
		// if it's an enemy mine, update both teamlists
		mapEntityData_t *mEnt;

		if ( ( mEnt = G_FindMapEntityData( &mapEntityData[0], inflictor - g_entities ) ) != NULL ) {
			G_FreeMapEntityData( &mapEntityData[0], mEnt );
		}

		if ( ( mEnt = G_FindMapEntityData( &mapEntityData[1], inflictor - g_entities ) ) != NULL ) {
			G_FreeMapEntityData( &mapEntityData[1], mEnt );
		}
	}

	{
		mapEntityData_t *mEnt;
		mapEntityData_Team_t *teamList = self->client->sess.sessionTeam == TEAM_AXIS ? &mapEntityData[1] : &mapEntityData[0];   // swapped, cause enemy team

		mEnt = G_FindMapEntityDataSingleClient( teamList, NULL, self->s.number, -1 );

		while ( mEnt ) {
			mEnt = G_FindMapEntityDataSingleClient( teamList, mEnt, self->s.number, -1 );
		}
	}

	if ( self->tankLink ) {
		G_LeaveTank( self, qfalse );

		killedintank = qtrue;
	}

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	self->client->ps.pm_type = PM_DEAD;

	G_AddEvent( self, EV_STOPSTREAMINGSOUND, 0 );

	if ( attacker ) {
		killer = attacker->s.number;
		killerName = ( attacker->client ) ? attacker->client->pers.netname : "<non-client>";
	}

	if ( attacker == 0 || killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( g_gamestate.integer == GS_PLAYING ) {
		char *obit;

		if ( meansOfDeath < 0 || meansOfDeath >= sizeof( modNames ) / sizeof( modNames[0] ) ) {
			obit = "<bad obituary>";
		} else {
			obit = modNames[meansOfDeath];
		}

		G_LogPrintf( "Kill: %i %i %i: %s killed %s by %s\n", killer, self->s.number, meansOfDeath, killerName, self->client->pers.netname, obit );
	}

	// broadcast the death event to everyone
	ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	// JPW NERVE -- if player is holding ticking grenade, drop it
	if ( ( self->client->ps.grenadeTimeLeft ) && ( self->s.weapon != WP_DYNAMITE ) && ( self->s.weapon != WP_LANDMINE ) && ( self->s.weapon != WP_SATCHEL ) && ( self->s.weapon != WP_TRIPMINE ) ) {
		vec3_t launchvel, launchspot;

		launchvel[0] = crandom();
		launchvel[1] = crandom();
		launchvel[2] = random();
		VectorScale( launchvel, 160, launchvel );
		VectorCopy( self->r.currentOrigin, launchspot );
		launchspot[2] += 40;

		{
			// Gordon: fixes premature grenade explosion, ta bani ;)
			gentity_t *m = fire_grenade( self, launchspot, launchvel, self->s.weapon );
			m->damage = 0;
		}
	}

	if ( attacker && attacker->client ) {
		if ( attacker == self || OnSameTeam( self, attacker ) ) {
		} else {
			// JPW NERVE -- mostly added as conveneience so we can tweak from the #defines all in one place
			AddScore( attacker, WOLF_FRAG_BONUS );
		}
	} else {
		AddScore( self, -1 );
	}

	// Add team bonuses
	Team_FragBonuses( self, inflictor, attacker );

	// drop flag regardless
	if ( self->client->ps.powerups[PW_REDFLAG] ) {
		item = BG_FindItem( "Red Flag" );
		if ( !item ) {
			item = BG_FindItem( "Objective" );
		}

		self->client->ps.powerups[PW_REDFLAG] = 0;
	}
	if ( self->client->ps.powerups[PW_BLUEFLAG] ) {
		item = BG_FindItem( "Blue Flag" );
		if ( !item ) {
			item = BG_FindItem( "Objective" );
		}

		self->client->ps.powerups[PW_BLUEFLAG] = 0;
	}

	if ( item ) {
		vec3_t launchvel = { 0, 0, 0 };
		gentity_t *flag = LaunchItem( item, self->r.currentOrigin, launchvel, self->s.number );

		flag->s.modelindex2 = self->s.otherEntityNum2; // JPW NERVE FIXME set player->otherentitynum2 with old modelindex2 from flag and restore here
		flag->message = self->message;  // DHM - Nerve :: also restore item name
		// Clear out player's temp copies
		self->s.otherEntityNum2 = 0;
		self->message = NULL;
	}

	// send a fancy "MEDIC!" scream.  Sissies, ain' they?
	if ( self->client != NULL ) {
		if ( self->health > GIB_HEALTH && meansOfDeath != MOD_SUICIDE && meansOfDeath != MOD_SWITCHTEAM ) {
			G_AddEvent( self, EV_MEDIC_CALL, 0 );
		}
	}

	Cmd_Score_f( self );        // show scores

	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0; i < level.numConnectedClients; i++ ) {
		gclient_t *client = &level.clients[level.sortedClients[i]];

		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}

		if ( client->sess.spectatorClient == self->s.number ) {
			Cmd_Score_f( g_entities + level.sortedClients[i] );
		}
	}

	self->takedamage = qtrue;   // can still be gibbed
	self->r.contents = CONTENTS_CORPSE;

	//self->s.angles[2] = 0;
	self->s.powerups = 0;
	self->s.loopSound = 0;

	self->client->limboDropWeapon = self->s.weapon; // store this so it can be dropped in limbo

	LookAtKiller( self, inflictor, attacker );
	self->client->ps.viewangles[0] = 0;
	self->client->ps.viewangles[2] = 0;

	self->r.maxs[2] = self->client->ps.crouchMaxZ;  //%	0;			// ydnar: so bodies don't clip into world
	self->client->ps.maxs[2] = self->client->ps.crouchMaxZ; //%	0;	// ydnar: so bodies don't clip into world
	trap_LinkEntity( self );

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	/* Nico, instant reswawn
	self->client->respawnTime = level.timeCurrent + 800;*/

	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof( self->client->ps.powerups ) );

	// never gib in a nodrop
	// FIXME: contents is always 0 here
	if ( self->health <= GIB_HEALTH && !( contents & CONTENTS_NODROP ) ) {
		GibEntity( self, killer );
		nogib = qfalse;
	}

	if ( nogib ) {
		// normal death
		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if ( self->health <= GIB_HEALTH ) {
			self->health = GIB_HEALTH + 1;
		}

		// Arnout: re-enable this for flailing
/*		if( self->client->ps.groundEntityNum == ENTITYNUM_NONE ) {
			self->client->ps.pm_flags |= PMF_FLAILING;
			self->client->ps.pm_time = 750;
			BG_AnimScriptAnimation( &self->client->ps, ANIM_MT_FLAILING, qtrue );

			// Face explosion directory
			{
				vec3_t angles;

				vectoangles( self->client->ps.velocity, angles );
				self->client->ps.viewangles[YAW] = angles[YAW];
				SetClientViewAngle( self, self->client->ps.viewangles );
			}
		} else*/

		// DHM - Play death animation, and set pm_time to delay 'fallen' animation
		//if( G_IsSinglePlayerGame() && self->client->sess.sessionTeam == TEAM_ALLIES ) {
		//	// play "falldown" animation since allies bots won't ever die completely
		//	self->client->ps.pm_time = BG_AnimScriptEvent( &self->client->ps, self->client->pers.character->animModelInfo, ANIM_ET_FALLDOWN, qfalse, qtrue );
		//	G_StartPlayerAppropriateSound(self, "death");
		//} else {
		self->client->ps.pm_time = BG_AnimScriptEvent( &self->client->ps, self->client->pers.character->animModelInfo, ANIM_ET_DEATH, qfalse, qtrue );
		// death animation script already contains sound
		//}

		// record the death animation to be used later on by the corpse
		self->client->torsoDeathAnim = self->client->ps.torsoAnim;
		self->client->legsDeathAnim = self->client->ps.legsAnim;

		G_AddEvent( self, EV_DEATH1 + 1, killer );

		// the body can still be gibbed
		self->die = body_die;
	}

	if ( meansOfDeath == MOD_MACHINEGUN ) {
		switch ( self->client->sess.sessionTeam ) {
		case TEAM_AXIS:
			level.axisMG42Counter = level.time;
			break;
		case TEAM_ALLIES:
			level.alliesMG42Counter = level.time;
			break;
		default:
			break;
		}
	}

	G_FadeItems( self, MOD_SATCHEL );

	CalculateRanks();

	if ( killedintank /*Gordon: automatically go to limbo from tank*/ ) {
		limbo( self ); // but no corpse
	} else if ( ( meansOfDeath == MOD_SUICIDE && g_gamestate.integer == GS_PLAYING ) ) {
		limbo( self );
	}
}

qboolean IsHeadShotWeapon( int mod ) {
	// players are allowed headshots from these weapons
	if (    mod == MOD_LUGER ||
			mod == MOD_COLT ||
			mod == MOD_AKIMBO_COLT ||
			mod == MOD_AKIMBO_LUGER ||
			mod == MOD_AKIMBO_SILENCEDCOLT ||
			mod == MOD_AKIMBO_SILENCEDLUGER ||
			mod == MOD_MP40 ||
			mod == MOD_THOMPSON ||
			mod == MOD_STEN ||
			mod == MOD_GARAND

			|| mod == MOD_KAR98
			|| mod == MOD_K43
			|| mod == MOD_K43_SCOPE
			|| mod == MOD_CARBINE
			|| mod == MOD_GARAND
			|| mod == MOD_GARAND_SCOPE
			|| mod == MOD_SILENCER
			|| mod == MOD_SILENCED_COLT
			|| mod == MOD_FG42
			|| mod == MOD_FG42SCOPE
			) {
		return qtrue;
	}

	return qfalse;
}

gentity_t* G_BuildHead( gentity_t *ent ) {
	gentity_t* head;
	orientation_t or;           // DHM - Nerve

	head = G_Spawn();

	if ( trap_GetTag( ent->s.number, 0, "tag_head", &or ) ) {
		G_SetOrigin( head, or.origin );
	} else {
		float height, dest;
		vec3_t v, angles, forward, up, right;

		G_SetOrigin( head, ent->r.currentOrigin );

		if ( ent->client->ps.eFlags & EF_PRONE ) {
			height = ent->client->ps.viewheight - 56;
		} else if ( ent->client->ps.pm_flags & PMF_DUCKED ) {    // closer fake offset for 'head' box when crouching
			height = ent->client->ps.crouchViewHeight - 12;
		} else {
			height = ent->client->ps.viewheight;
		}

		// NERVE - SMF - this matches more closely with WolfMP models
		VectorCopy( ent->client->ps.viewangles, angles );
		if ( angles[PITCH] > 180 ) {
			dest = ( -360 + angles[PITCH] ) * 0.75;
		} else {
			dest = angles[PITCH] * 0.75;
		}
		angles[PITCH] = dest;

		AngleVectors( angles, forward, right, up );
		if ( ent->client->ps.eFlags & EF_PRONE ) {
			VectorScale( forward, 24, v );
		} else {
			VectorScale( forward, 5, v );
		}
		VectorMA( v, 18, up, v );

		VectorAdd( v, head->r.currentOrigin, head->r.currentOrigin );
		head->r.currentOrigin[2] += height / 2;
		// -NERVE - SMF
	}

	VectorCopy( head->r.currentOrigin, head->s.origin );
	VectorCopy( ent->r.currentAngles, head->s.angles );
	VectorCopy( head->s.angles, head->s.apos.trBase );
	VectorCopy( head->s.angles, head->s.apos.trDelta );
	VectorSet( head->r.mins, -6, -6, -2 ); // JPW NERVE changed this z from -12 to -6 for crouching, also removed standing offset
	VectorSet( head->r.maxs, 6, 6, 10 ); // changed this z from 0 to 6
	head->clipmask = CONTENTS_SOLID;
	head->r.contents = CONTENTS_SOLID;
	head->parent = ent;
	head->s.eType = ET_TEMPHEAD;

	trap_LinkEntity( head );

	return head;
}

gentity_t* G_BuildLeg( gentity_t *ent ) {
	gentity_t* leg;
	vec3_t flatforward, org;
	//orientation_t or;			// DHM - Nerve

	if ( !( ent->client->ps.eFlags & EF_PRONE ) ) {
		return NULL;
	}

	leg = G_Spawn();

	AngleVectors( ent->client->ps.viewangles, flatforward, NULL, NULL );
	flatforward[2] = 0;
	VectorNormalizeFast( flatforward );

	org[0] = ent->r.currentOrigin[0] + flatforward[0] * -32;
	org[1] = ent->r.currentOrigin[1] + flatforward[1] * -32;
	org[2] = ent->r.currentOrigin[2] + ent->client->pmext.proneLegsOffset;

	G_SetOrigin( leg, org );

	VectorCopy( leg->r.currentOrigin, leg->s.origin );
	VectorCopy( ent->r.currentAngles, leg->s.angles );
	VectorCopy( leg->s.angles, leg->s.apos.trBase );
	VectorCopy( leg->s.angles, leg->s.apos.trDelta );
	VectorCopy( playerlegsProneMins, leg->r.mins );
	VectorCopy( playerlegsProneMaxs, leg->r.maxs );
	leg->clipmask = CONTENTS_SOLID;
	leg->r.contents = CONTENTS_SOLID;
	leg->parent = ent;
	leg->s.eType = ET_TEMPLEGS;

	trap_LinkEntity( leg );

	return leg;
}

qboolean IsHeadShot( gentity_t *targ, vec3_t dir, vec3_t point, int mod ) {
	gentity_t   *head;
	trace_t tr;
	vec3_t start, end;
	gentity_t   *traceEnt;

	// not a player or critter so bail
	if ( !( targ->client ) ) {
		return qfalse;
	}

	if ( targ->health <= 0 ) {
		return qfalse;
	}

	if ( !IsHeadShotWeapon( mod ) ) {
		return qfalse;
	}

	head = G_BuildHead( targ );

	// trace another shot see if we hit the head
	VectorCopy( point, start );
	VectorMA( start, 64, dir, end );
	trap_Trace( &tr, start, NULL, NULL, end, targ->s.number, MASK_SHOT );

	traceEnt = &g_entities[ tr.entityNum ];

	if ( g_debugBullets.integer >= 3 ) { // show hit player head bb
		gentity_t *tent;
		vec3_t b1, b2;
		VectorCopy( head->r.currentOrigin, b1 );
		VectorCopy( head->r.currentOrigin, b2 );
		VectorAdd( b1, head->r.mins, b1 );
		VectorAdd( b2, head->r.maxs, b2 );
		tent = G_TempEntity( b1, EV_RAILTRAIL );
		VectorCopy( b2, tent->s.origin2 );
		tent->s.dmgFlags = 1;

		// show headshot trace
		// end the headshot trace at the head box if it hits
		if ( tr.fraction != 1 ) {
			VectorMA( start, ( tr.fraction * 64 ), dir, end );
		}
		tent = G_TempEntity( start, EV_RAILTRAIL );
		VectorCopy( end, tent->s.origin2 );
		tent->s.dmgFlags = 0;
	}

	G_FreeEntity( head );

	if ( traceEnt == head ) {
		level.totalHeadshots++;     // NERVE - SMF
		return qtrue;
	} else {
		level.missedHeadshots++;    // NERVE - SMF

	}
	return qfalse;
}

qboolean IsLegShot( gentity_t *targ, vec3_t dir, vec3_t point, int mod ) {
	float height;
	float theight;
	gentity_t *leg;

	if ( !( targ->client ) ) {
		return qfalse;
	}

	if ( targ->health <= 0 ) {
		return qfalse;
	}

	if ( !point ) {
		return qfalse;
	}

	if ( !IsHeadShotWeapon( mod ) ) {
		return qfalse;
	}

	leg = G_BuildLeg( targ );

	if ( leg ) {
		gentity_t   *traceEnt;
		vec3_t start, end;
		trace_t tr;

		// trace another shot see if we hit the legs
		VectorCopy( point, start );
		VectorMA( start, 64, dir, end );
		trap_Trace( &tr, start, NULL, NULL, end, targ->s.number, MASK_SHOT );

		traceEnt = &g_entities[ tr.entityNum ];

		if ( g_debugBullets.integer >= 3 ) { // show hit player head bb
			gentity_t *tent;
			vec3_t b1, b2;
			VectorCopy( leg->r.currentOrigin, b1 );
			VectorCopy( leg->r.currentOrigin, b2 );
			VectorAdd( b1, leg->r.mins, b1 );
			VectorAdd( b2, leg->r.maxs, b2 );
			tent = G_TempEntity( b1, EV_RAILTRAIL );
			VectorCopy( b2, tent->s.origin2 );
			tent->s.dmgFlags = 1;

			// show headshot trace
			// end the headshot trace at the head box if it hits
			if ( tr.fraction != 1 ) {
				VectorMA( start, ( tr.fraction * 64 ), dir, end );
			}
			tent = G_TempEntity( start, EV_RAILTRAIL );
			VectorCopy( end, tent->s.origin2 );
			tent->s.dmgFlags = 0;
		}

		G_FreeEntity( leg );

		if ( traceEnt == leg ) {
			return qtrue;
		}
	} else {
		height = point[2] - targ->r.absmin[2];
		theight = targ->r.absmax[2] - targ->r.absmin[2];

		if ( height < ( theight * 0.4f ) ) {
			return qtrue;
		}
	}

	return qfalse;
}

qboolean IsArmShot( gentity_t *targ, gentity_t* ent, vec3_t point, int mod ) {
	vec3_t path, view;
	vec_t dot;

	if ( !( targ->client ) ) {
		return qfalse;
	}

	if ( targ->health <= 0 ) {
		return qfalse;
	}

	if ( !IsHeadShotWeapon( mod ) ) {
		return qfalse;
	}

	VectorSubtract( targ->client->ps.origin, point, path );
	path[2] = 0;

	AngleVectors( targ->client->ps.viewangles, view, NULL, NULL );
	view[2] = 0;

	VectorNormalize( path );

	dot = DotProduct( path, view );

	if ( dot > 0.4f || dot < -0.75f ) {
		return qfalse;
	}

	return qtrue;
}

/*
============
G_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,  vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t   *client;
	int take;
	// int save; Nico, unused warning fix
	int knockback;
	qboolean headShot;
	qboolean wasAlive;
	hitRegion_t hr = HR_NUM_HITREGIONS;

	if ( !targ->takedamage ) {
		return;
	}

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// Arnout: invisible entities can't be damaged
	if ( targ->entstate == STATE_INVISIBLE ||
		 targ->entstate == STATE_UNDERCONSTRUCTION ) {
		return;
	}

	// xkan, 12/23/2002 - was the bot alive before applying any damage?
	wasAlive = ( targ->health > 0 );

	// Arnout: combatstate
	if ( targ->client && attacker && attacker->client && attacker != targ ) {
		if ( g_gamestate.integer == GS_PLAYING ) {
			if ( !OnSameTeam( attacker, targ ) ) {
				targ->client->combatState |= ( 1 << COMBATSTATE_DAMAGERECEIVED );
				attacker->client->combatState |= ( 1 << COMBATSTATE_DAMAGEDEALT );
			}
		}
	}

// JPW NERVE
	if ( ( targ->waterlevel >= 3 ) && ( mod == MOD_FLAMETHROWER ) ) {
		return;
	}
// jpw

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER && !( targ->isProp ) && !targ->scriptName ) {
		if ( targ->use && targ->moverState == MOVER_POS1 ) {
			G_UseEntity( targ, inflictor, attacker );
		}
		return;
	}


	// TAT 11/22/2002
	//		In the old code, this check wasn't done for props, so I put that check back in to make props_statue properly work
	// 4 means destructible
	if ( targ->s.eType == ET_MOVER && ( targ->spawnflags & 4 ) && !targ->isProp ) {
		if ( !G_WeaponIsExplosive( mod ) ) {
			return;
		}

		// check for team
		if ( G_GetTeamFromEntity( inflictor ) == G_GetTeamFromEntity( targ ) ) {
			return;
		}
	} else if ( targ->s.eType == ET_EXPLOSIVE ) {
		if ( targ->parent && G_GetWeaponClassForMOD( mod ) == 2 ) {
			return;
		}

		if ( G_GetTeamFromEntity( inflictor ) == G_GetTeamFromEntity( targ ) ) {
			return;
		}

		if ( G_GetWeaponClassForMOD( mod ) < targ->constructibleStats.weaponclass ) {
			return;
		}
	} else if ( targ->s.eType == ET_MISSILE && targ->methodOfDeath == MOD_LANDMINE )   {
		if ( targ->s.modelindex2 ) {
			if ( G_WeaponIsExplosive( mod ) ) {
				mapEntityData_t *mEnt;

				if ( ( mEnt = G_FindMapEntityData( &mapEntityData[0], targ - g_entities ) ) != NULL ) {
					G_FreeMapEntityData( &mapEntityData[0], mEnt );
				}

				if ( ( mEnt = G_FindMapEntityData( &mapEntityData[1], targ - g_entities ) ) != NULL ) {
					G_FreeMapEntityData( &mapEntityData[1], mEnt );
				}

				if ( attacker && attacker->client ) {
					AddScore( attacker, 1 );
				}

				G_ExplodeMissile( targ );
			}
		}
		return;
	} else if ( targ->s.eType == ET_CONSTRUCTIBLE ) {

		if ( G_GetTeamFromEntity( inflictor ) == G_GetTeamFromEntity( targ ) ) {
			return;
		}

		if ( G_GetWeaponClassForMOD( mod ) < targ->constructibleStats.weaponclass ) {
			return;
		}
		//bani - fix #238
		if ( mod == MOD_DYNAMITE ) {
			if ( !( inflictor->etpro_misc_1 & 1 ) ) {
				return;
			}
		}
	}

	client = targ->client;

	if ( client ) {
		if ( client->noclip || client->ps.powerups[PW_INVULNERABLE] ) {
			return;
		}
	}

	// check for godmode
	if ( targ->flags & FL_GODMODE ) {
		return;
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize( dir );
	}

	knockback = damage;
	if ( knockback > 200 ) {
		knockback = 200;
	}
	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	} else if ( dflags & DAMAGE_HALF_KNOCKBACK ) {
		knockback *= 0.5f;
	}

	// ydnar: set weapons means less knockback
	if ( client && ( client->ps.weapon == WP_MORTAR_SET || client->ps.weapon == WP_MOBILE_MG42_SET ) ) {
		knockback *= 0.5;
	}

	/* Nico, no friendlyfire
	if ( targ->client && g_friendlyFire.integer && OnSameTeam( targ, attacker ) ) {
		knockback = 0;
	}*/

	// Nico, only do knockback to ourself at panzerfaust
	if (attacker->client == targ->client && mod == MOD_PANZERFAUST) {
		knockback = qtrue;
	} else {
		knockback = qfalse;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client ) {
		vec3_t kvel;

		/* Nico, set knockback
		float mass;

		mass = 200;

		VectorScale( dir, g_knockback.value * (float)knockback / mass, kvel );*/

		VectorScale( dir, KNOCKBACK_VALUE, kvel );
		VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );

		if ( targ == attacker && !(  mod != MOD_ROCKET &&
									 mod != MOD_GRENADE &&
									 mod != MOD_GRENADE_LAUNCHER &&
									 mod != MOD_DYNAMITE
									 && mod != MOD_GPG40
									 && mod != MOD_M7
									 && mod != MOD_LANDMINE
									 ) ) {
			targ->client->ps.velocity[2] *= 0.25;
		}

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !targ->client->ps.pm_time ) {
			int t;

			t = knockback * 2;
			if ( t < 50 ) {
				t = 50;
			}
			if ( t > 200 ) {
				t = 200;
			}
			targ->client->ps.pm_time = t;
			targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
		}
	}

	// check for completely getting out of the damage
	if ( !( dflags & DAMAGE_NO_PROTECTION ) ) {

		// Nico, disable damage between players
		if (targ->client && attacker->client && 
			(OnSameTeam (targ, attacker) || 
			(targ->client->sess.sessionTeam == TEAM_AXIS && attacker->client->sess.sessionTeam == TEAM_ALLIES) ||
			(targ->client->sess.sessionTeam == TEAM_ALLIES && attacker->client->sess.sessionTeam == TEAM_AXIS))) {
				return;
		}
	}

	// add to the attacker's hit counter
	if ( attacker->client && targ != attacker && targ->health > 0 ) {
		if ( OnSameTeam( targ, attacker ) ) {
			attacker->client->ps.persistant[PERS_HITS] -= damage;
		} else {
			attacker->client->ps.persistant[PERS_HITS] += damage;
		}
	}

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;

	headShot = IsHeadShot( targ, dir, point, mod );
	if ( headShot ) {
		if ( take * 2 < 50 ) { // head shots, all weapons, do minimum 50 points damage
			take = 50;
		} else {
			take *= 2; // sniper rifles can do full-kill (and knock into limbo)

		}
		if ( dflags & DAMAGE_DISTANCEFALLOFF ) {
			vec_t dist;
			vec3_t shotvec;
			float scale;

			VectorSubtract( point, muzzleTrace, shotvec );
			dist = VectorLength( shotvec );

#if DO_BROKEN_DISTANCEFALLOFF
			// ~~~___---___
			if ( dist > 1500.f ) {
				if ( dist > 2500.f ) {
					take *= 0.2f;
				} else {
					float scale = 1.f - 0.2f * ( 1000.f / ( dist - 1000.f ) );

					take *= scale;
				}
			}
#else
			// ~~~---______
			// zinx - start at 100% at 1500 units (and before),
			// and go to 20% at 2500 units (and after)

			// 1500 to 2500 -> 0.0 to 1.0
			scale = ( dist - 1500.f ) / ( 2500.f - 1500.f );
			// 0.0 to 1.0 -> 0.0 to 0.8
			scale *= 0.8f;
			// 0.0 to 0.8 -> 1.0 to 0.2
			scale = 1.0f - scale;

			// And, finally, cap it.
			if ( scale > 1.0f ) {
				scale = 1.0f;
			} else if ( scale < 0.2f ) {
				scale = 0.2f;
			}

			take *= scale;
#endif
		}

		if ( !( targ->client->ps.eFlags & EF_HEADSHOT ) ) {    // only toss hat on first headshot
			G_AddEvent( targ, EV_LOSE_HAT, DirToByte( dir ) );

			if ( mod != MOD_K43_SCOPE &&
				 mod != MOD_GARAND_SCOPE ) {
				take *= .8f;    // helmet gives us some protection
			}
		}

		targ->client->ps.eFlags |= EF_HEADSHOT;

		// OSP - Record the headshot
		if ( g_debugBullets.integer ) {
			trap_SendServerCommand( attacker - g_entities, "print \"Head Shot\n\"\n" );
		}

		hr = HR_HEAD;
	} else if ( IsLegShot( targ, dir, point, mod ) ) {

		hr = HR_LEGS;
		if ( g_debugBullets.integer ) {
			trap_SendServerCommand( attacker - g_entities, "print \"Leg Shot\n\"\n" );
		}
	} else if ( IsArmShot( targ, attacker, point, mod ) ) {

		hr = HR_ARMS;
		if ( g_debugBullets.integer ) {
			trap_SendServerCommand( attacker - g_entities, "print \"Arm Shot\n\"\n" );
		}
	} else if ( targ->client && targ->health > 0 && IsHeadShotWeapon( mod ) ) {

		hr = HR_BODY;
		if ( g_debugBullets.integer ) {
			trap_SendServerCommand( attacker - g_entities, "print \"Body Shot\n\"\n" );
		}
	}

	if ( g_debugDamage.integer )
	{
		G_Printf( "client:%i health:%i damage:%i mod:%s\n", targ->s.number, targ->health, take, modNames[mod] );
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_blood += take;
		client->damage_knockback += knockback;

		if ( dir ) {
			VectorCopy( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}
	}

	if ( targ->client ) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	// do the damage
	if ( take ) {
		targ->health -= take;

		// Gordon: don't ever gib POWS
		if ( ( targ->health <= 0 ) && ( targ->r.svFlags & SVF_POW ) ) {
			targ->health = -1;
		}

		// Ridah, can't gib with bullet weapons (except VENOM)
		// Arnout: attacker == inflictor can happen in other cases as well! (movers trying to gib things)
		//if ( attacker == inflictor && targ->health <= GIB_HEALTH) {
		if ( targ->health <= GIB_HEALTH ) {
			if ( !G_WeaponIsExplosive( mod ) ) {
				targ->health = GIB_HEALTH + 1;
			}
		}

// JPW NERVE overcome previous chunk of code for making grenades work again
//		if ((take > 190)) // 190 is greater than 2x mauser headshot, so headshots don't gib
		// Arnout: only player entities! messes up ents like func_constructibles and func_explosives otherwise
		if ( ( ( targ->s.number < MAX_CLIENTS ) && ( take > 190 ) ) && !( targ->r.svFlags & SVF_POW ) ) {
			targ->health = GIB_HEALTH - 1;
		}

		if ( targ->s.eType == ET_MOVER && !Q_stricmp( targ->classname, "script_mover" ) ) {
			targ->s.dl_intensity = 255.f * ( targ->health / (float)targ->count ); // send it to the client
		}

		//G_Printf("health at: %d\n", targ->health);
		if ( targ->health <= 0 ) {
			if ( client && !wasAlive ) {
				targ->flags |= FL_NO_KNOCKBACK;
				if ( ( targ->health < FORCE_LIMBO_HEALTH ) && ( targ->health > GIB_HEALTH ) ) {
					limbo( targ );
				}

				// xkan, 1/13/2003 - record the time we died.
				if ( !client->deathTime ) {
					client->deathTime = level.time;
				}
			} else {


				targ->sound1to2 = hr;
				targ->sound2to1 = mod;
				targ->sound2to3 = ( dflags & DAMAGE_RADIUS ) ? 1 : 0;

				if ( client ) {
					if ( G_GetTeamFromEntity( inflictor ) != G_GetTeamFromEntity( targ ) ) {
					}
				}

				if ( targ->health < -999 ) {
					targ->health = -999;
				}


				targ->enemy = attacker;
				targ->deathType = mod;

				// Ridah, mg42 doesn't have die func (FIXME)
				if ( targ->die ) {
					// Kill the entity.  Note that this funtion can set ->die to another
					// function pointer, so that next time die is applied to the dead body.
					targ->die( targ, inflictor, attacker, take, mod );
					// OSP - kill stats in player_die function
				}

				if ( targ->s.eType == ET_MOVER && !Q_stricmp( targ->classname, "script_mover" ) && ( targ->spawnflags & 8 ) ) {
					return; // reseructable script mover doesn't unlink itself but we don't want a second death script to be called
				}

				// if we freed ourselves in death function
				if ( !targ->inuse ) {
					return;
				}

				// RF, entity scripting
				if ( targ->health <= 0 ) {   // might have revived itself in death function
					if (  ( targ->s.eType != ET_CONSTRUCTIBLE && targ->s.eType != ET_EXPLOSIVE ) ||
								 ( targ->s.eType == ET_CONSTRUCTIBLE && !targ->desstages ) ) { // call manually if using desstages
						G_Script_ScriptEvent( targ, "death", "" );
					}
				}
			}

		} else if ( targ->pain ) {
			if ( dir ) {  // Ridah, had to add this to fix NULL dir crash
				VectorCopy( dir, targ->rotate );
				VectorCopy( point, targ->pos3 ); // this will pass loc of hit
			} else {
				VectorClear( targ->rotate );
				VectorClear( targ->pos3 );
			}

			targ->pain( targ, attacker, take, point );
		}

		// RF, entity scripting
		G_Script_ScriptEvent( targ, "pain", va( "%d %d", targ->health, targ->health + take ) );

		// Ridah, this needs to be done last, incase the health is altered in one of the event calls
		if ( targ->client ) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}
	}
}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/

void G_RailTrail( vec_t* start, vec_t* end ) {
	gentity_t* temp = G_TempEntity( start, EV_RAILTRAIL );
	VectorCopy( end, temp->s.origin2 );
	temp->s.dmgFlags = 0;
}

#define MASK_CAN_DAMAGE     ( CONTENTS_SOLID | CONTENTS_BODY )

qboolean CanDamage( gentity_t *targ, vec3_t origin ) {
	vec3_t dest;
	trace_t tr;
	vec3_t midpoint;
	vec3_t offsetmins = { -16.f, -16.f, -16.f };
	vec3_t offsetmaxs = { 16.f, 16.f, 16.f };

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	// Gordon: well, um, just check then...
	if ( targ->r.currentOrigin[0] || targ->r.currentOrigin[1] || targ->r.currentOrigin[2] ) {
		VectorCopy( targ->r.currentOrigin, midpoint );

		if ( targ->s.eType == ET_MOVER ) {
			midpoint[2] += 32;
		}
	} else {
		VectorAdd( targ->r.absmin, targ->r.absmax, midpoint );
		VectorScale( midpoint, 0.5, midpoint );
	}

//	G_RailTrail( origin, dest );

	trap_Trace( &tr, origin, vec3_origin, vec3_origin, midpoint, ENTITYNUM_NONE, MASK_CAN_DAMAGE );
	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	if ( &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	if ( targ->client ) {
		VectorCopy( targ->client->ps.mins, offsetmins );
		VectorCopy( targ->client->ps.maxs, offsetmaxs );
	}

	// this should probably check in the plane of projection,
	// rather than in world coordinate
	VectorCopy( midpoint, dest );
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE );
	if ( tr.fraction == 1 || &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE );
	if ( tr.fraction == 1 || &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE );
	if ( tr.fraction == 1 || &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE );
	if ( tr.fraction == 1 || &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	// =========================

	VectorCopy( midpoint, dest );
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE );
	if ( tr.fraction == 1 || &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE );
	if ( tr.fraction == 1 || &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE );
	if ( tr.fraction == 1 || &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	VectorCopy( midpoint, dest );
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[2];
	dest[2] += offsetmins[2];
	trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_CAN_DAMAGE );
	if ( tr.fraction == 1 || &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	return qfalse;
}

void G_AdjustedDamageVec( gentity_t *ent, vec3_t origin, vec3_t v ) {
	int i;

	if ( !ent->r.bmodel ) {
		VectorSubtract( ent->r.currentOrigin,origin,v ); // JPW NERVE simpler centroid check that doesn't have box alignment weirdness
	} else {
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}
	}
}

/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage( vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod ) {
	float points, dist;
	gentity_t   *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	int i, e;
	qboolean hitClient = qfalse;
	float boxradius;
	vec3_t dest;
	trace_t tr;
	vec3_t midpoint;
	int flags = DAMAGE_RADIUS;

	if ( mod == MOD_SATCHEL || mod == MOD_LANDMINE ) {
		flags |= DAMAGE_HALF_KNOCKBACK;
	}

	if ( radius < 1 ) {
		radius = 1;
	}

	boxradius = 1.41421356 * radius; // radius * sqrt(2) for bounding box enlargement --
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - boxradius;
		maxs[i] = origin[i] + boxradius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < level.num_entities ; e++ ) {
		g_entities[e].dmginloop = qfalse;
	}

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if ( ent == ignore ) {
			continue;
		}
		if ( !ent->takedamage && ( !ent->dmgparent || !ent->dmgparent->takedamage ) ) {
			continue;
		}

		G_AdjustedDamageVec( ent, origin, v );

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if ( CanDamage( ent, origin ) ) {
			if ( ent->dmgparent ) {
				ent = ent->dmgparent;
			}

			if ( ent->dmginloop ) {
				continue;
			}

			if ( AccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;


			G_Damage( ent, inflictor, attacker, dir, origin, (int)points, flags, mod );
		} else {
			VectorAdd( ent->r.absmin, ent->r.absmax, midpoint );
			VectorScale( midpoint, 0.5, midpoint );
			VectorCopy( midpoint, dest );

			trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
			if ( tr.fraction < 1.0 ) {
				VectorSubtract( dest, origin, dest );
				dist = VectorLength( dest );
				if ( dist < radius * 0.2f ) { // closer than 1/4 dist
					if ( ent->dmgparent ) {
						ent = ent->dmgparent;
					}

					if ( ent->dmginloop ) {
						continue;
					}

					if ( AccuracyHit( ent, attacker ) ) {
						hitClient = qtrue;
					}
					VectorSubtract( ent->r.currentOrigin, origin, dir );
					dir[2] += 24;
					G_Damage( ent, inflictor, attacker, dir, origin, (int)( points * 0.1f ), flags, mod );
				}
			}
		}
	}
	return hitClient;
}

/*
============
etpro_RadiusDamage
mutation of G_RadiusDamage which lets us selectively damage only clients or only non clients
============
*/
qboolean etpro_RadiusDamage( vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod, qboolean clientsonly ) {
	float points, dist;
	gentity_t   *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	int i, e;
	qboolean hitClient = qfalse;
	float boxradius;
	vec3_t dest;
	trace_t tr;
	vec3_t midpoint;
	int flags = DAMAGE_RADIUS;

	if ( mod == MOD_SATCHEL || mod == MOD_LANDMINE ) {
		flags |= DAMAGE_HALF_KNOCKBACK;
	}

	if ( radius < 1 ) {
		radius = 1;
	}

	boxradius = 1.41421356 * radius; // radius * sqrt(2) for bounding box enlargement --
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - boxradius;
		maxs[i] = origin[i] + boxradius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < level.num_entities ; e++ ) {
		g_entities[e].dmginloop = qfalse;
	}

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if ( ent == ignore ) {
			continue;
		}
		if ( !ent->takedamage && ( !ent->dmgparent || !ent->dmgparent->takedamage ) ) {
			continue;
		}

		if ( clientsonly && !ent->client ) {
			continue;
		}
		if ( !clientsonly && ent->client ) {
			continue;
		}

		G_AdjustedDamageVec( ent, origin, v );

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if ( CanDamage( ent, origin ) ) {
			if ( ent->dmgparent ) {
				ent = ent->dmgparent;
			}

			if ( ent->dmginloop ) {
				continue;
			}

			if ( AccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract( ent->r.currentOrigin, origin, dir );
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;

			G_Damage( ent, inflictor, attacker, dir, origin, (int)points, flags, mod );
		} else {
			VectorAdd( ent->r.absmin, ent->r.absmax, midpoint );
			VectorScale( midpoint, 0.5, midpoint );
			VectorCopy( midpoint, dest );

			trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
			if ( tr.fraction < 1.0 ) {
				VectorSubtract( dest, origin, dest );
				dist = VectorLength( dest );
				if ( dist < radius * 0.2f ) { // closer than 1/4 dist
					if ( ent->dmgparent ) {
						ent = ent->dmgparent;
					}

					if ( ent->dmginloop ) {
						continue;
					}

					if ( AccuracyHit( ent, attacker ) ) {
						hitClient = qtrue;
					}
					VectorSubtract( ent->r.currentOrigin, origin, dir );
					dir[2] += 24;
					G_Damage( ent, inflictor, attacker, dir, origin, (int)( points * 0.1f ), flags, mod );
				}
			}
		}
	}

	return hitClient;
}
