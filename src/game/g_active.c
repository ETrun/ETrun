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

/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback(gentity_t *player) {
	gclient_t *client;
	float     count;
	vec3_t    angles;

	client = player->client;
	if (client->ps.pm_type == PM_DEAD) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood;
	if (count == 0) {
		return;     // didn't take any damage
	}

	if (count > 127) {
		count = 127;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
	if (client->damage_fromWorld) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw   = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles(client->damage_from, angles);
		client->ps.damagePitch = angles[PITCH] / 360.0 * 256;
		client->ps.damageYaw   = angles[YAW] / 360.0 * 256;
	}

	// play an apropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && !(player->s.powerups & PW_INVULNERABLE)) {          //----(SA)
		player->pain_debounce_time = level.time + 700;
		G_AddEvent(player, EV_PAIN, player->health);
	}

	client->ps.damageEvent++;   // Ridah, always increment this since we do multiple view damage anims

	client->ps.damageCount = count;

	//
	// clear totals
	//
	client->damage_blood     = 0;
	client->damage_knockback = 0;
}


#define MIN_BURN_INTERVAL 399 // JPW NERVE set burn timeinterval so we can do more precise damage (was 199 old model)

/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects(gentity_t *ent) {
	int waterlevel;

	if (ent->client->noclip) {
		ent->client->airOutTime = level.time + HOLDBREATHTIME;  // don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	//
	// check for drowning
	//
	if (waterlevel == 3) {
		// if out of air, start drowning
		if (ent->client->airOutTime < level.time) {

			if (ent->client->ps.powerups[PW_BREATHER]) {   // take air from the breather now that we need it
				ent->client->ps.powerups[PW_BREATHER] -= (level.time - ent->client->airOutTime);
				ent->client->airOutTime                = level.time + (level.time - ent->client->airOutTime);
			} else if (!g_disableDrowning.integer) {   // Nico, check if drowning is disabled
				// drown!
				ent->client->airOutTime += 1000;
				if (ent->health > 0) {
					// take more damage the longer underwater
					ent->damage += 2;
					if (ent->damage > 15) {
						ent->damage = 15;
					}

					// play a gurp sound instead of a normal pain sound
					if (ent->health <= ent->damage) {
						G_Sound(ent, G_SoundIndex("*drown.wav"));
					} else if (rand() & 1) {
						G_Sound(ent, G_SoundIndex("sound/player/gurp1.wav"));
					} else {
						G_Sound(ent, G_SoundIndex("sound/player/gurp2.wav"));
					}

					// don't play a normal pain sound
					ent->pain_debounce_time = level.time + 200;

					G_Damage(ent, NULL, NULL, NULL, NULL, ent->damage, 0, MOD_WATER);
				}
			}
		}
	} else {
		ent->client->airOutTime = level.time + 12000;
		ent->damage             = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel && (ent->watertype & CONTENTS_LAVA)) {
		if (ent->health > 0 && ent->pain_debounce_time <= level.time) {

			if (ent->watertype & CONTENTS_LAVA) {
				G_Damage(ent, NULL, NULL, NULL, NULL,
				         30 * waterlevel, 0, MOD_LAVA);
			}

		}
	}

	//
	// check for burning from flamethrower
	//
	// JPW NERVE MP way
	if (ent->s.onFireEnd && ent->client) {
		if (level.time - ent->client->lastBurnTime >= MIN_BURN_INTERVAL) {

			// JPW NERVE server-side incremental damage routine / player damage/health is int (not float)
			// so I can't allocate 1.5 points per server tick, and 1 is too weak and 2 is too strong.
			// solution: allocate damage far less often (MIN_BURN_INTERVAL often) and do more damage.
			// That way minimum resolution (1 point) damage changes become less critical.

			ent->client->lastBurnTime = level.time;
			if ((ent->s.onFireEnd > level.time) && (ent->health > 0)) {
				gentity_t *attacker;
				attacker = g_entities + ent->flameBurnEnt;
				G_Damage(ent, attacker, attacker, NULL, NULL, 5, DAMAGE_NO_KNOCKBACK, MOD_FLAMETHROWER);   // JPW NERVE was 7
			}
		}
	}
	// jpw
}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound(gentity_t *ent) {
	ent->s.loopSound = 0;
}



// Are we ready to construct?  Optionally, will also update the time while we are constructing
qboolean ReadyToConstruct(gentity_t *ent, gentity_t *constructible, qboolean updateState) {
	int weaponTime = ent->client->ps.classWeaponTime;

	// "Ammo" for this weapon is time based
	if (weaponTime + level.engineerChargeTime[ent->client->sess.sessionTeam - 1] < level.time) {
		weaponTime = level.time - level.engineerChargeTime[ent->client->sess.sessionTeam - 1];
	}

	if (g_debugConstruct.integer) {
		weaponTime += 0.5f * ((float)level.engineerChargeTime[ent->client->sess.sessionTeam - 1] / (constructible->constructibleStats.duration / (float)FRAMETIME));
	} else {
		weaponTime += constructible->constructibleStats.chargebarreq * ((float)level.engineerChargeTime[ent->client->sess.sessionTeam - 1] / (constructible->constructibleStats.duration / (float)FRAMETIME));
	}

	// if the time is in the future, we have NO energy left
	if (weaponTime > level.time) {
		return qfalse;
	}

	// only set the actual weapon time for this entity if they want us to
	if (updateState) {
		ent->client->ps.classWeaponTime = weaponTime;
	}

	return qtrue;
}

//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts(gentity_t *ent, pmove_t *pm) {
	int       i, j;
	gentity_t *other;
	trace_t   trace;

	memset(&trace, 0, sizeof (trace));
	for (i = 0 ; i < pm->numtouch ; i++) {
		for (j = 0 ; j < i ; j++) {
			if (pm->touchents[j] == pm->touchents[i]) {
				break;
			}
		}
		if (j != i) {
			continue;   // duplicated
		}
		other = &g_entities[pm->touchents[i]];

		if (!other->touch) {
			continue;
		}

		other->touch(other, ent, &trace);
	}

}

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void    G_TouchTriggers(gentity_t *ent) {
	int           i, num;
	int           touch[MAX_GENTITIES];
	gentity_t     *hit;
	trace_t       trace;
	vec3_t        mins, maxs;
	static vec3_t range = { 40, 40, 52 };

	if (!ent->client) {
		return;
	}

	// Arnout: reset the pointer that keeps track of trigger_objective_info tracking
	ent->client->touchingTOI = NULL;

	// dead clients don't activate triggers!
	if (ent->client->ps.stats[STAT_HEALTH] <= 0) {
		return;
	}

	VectorSubtract(ent->client->ps.origin, range, mins);
	VectorAdd(ent->client->ps.origin, range, maxs);

	num = trap_EntitiesInBox(mins, maxs, touch, MAX_GENTITIES);

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd(ent->client->ps.origin, ent->r.mins, mins);
	VectorAdd(ent->client->ps.origin, ent->r.maxs, maxs);

	for (i = 0 ; i < num ; i++) {
		hit = &g_entities[touch[i]];

		if (!hit->touch && !ent->touch) {
			continue;
		}
		if (!(hit->r.contents & CONTENTS_TRIGGER)) {
			continue;
		}

		// Arnout: invisible entities can't be touched
		// Gordon: radiant tabs arnout! ;)
		if (hit->entstate == STATE_INVISIBLE ||
		    hit->entstate == STATE_UNDERCONSTRUCTION) {
			continue;
		}

		// ignore most entities if a spectator
		if (ent->client->sess.sessionTeam == TEAM_SPECTATOR) {
			if (hit->s.eType != ET_TELEPORT_TRIGGER) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if (hit->s.eType == ET_ITEM) {
			if (!BG_PlayerTouchesItem(&ent->client->ps, &hit->s, level.time)) {
				continue;
			}
		} else {
			// MrE: always use capsule for player
			if (!trap_EntityContactCapsule(mins, maxs, hit)) {
				//if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset(&trace, 0, sizeof (trace));

		if (hit->touch) {
			hit->touch(hit, ent, &trace);
		}

		if ((ent->r.svFlags & SVF_BOT) && (ent->touch)) {
			ent->touch(ent, hit, &trace);
		}
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink(gentity_t *ent, usercmd_t *ucmd) {
	pmove_t   pm;
	gclient_t *client;

	client = ent->client;

	if (client->sess.spectatorState != SPECTATOR_FOLLOW) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed   = SPECTATOR_SPEED; // was: 400 // faster than normal

		if (client->ps.sprintExertTime) {
			client->ps.speed *= 3;  // (SA) allow sprint in free-cam mode
		}

		// OSP - dead players are frozen too, in a timeout
		if ((client->ps.pm_flags & PMF_LIMBO) && level.match_pause != PAUSE_NONE) {
			client->ps.pm_type = PM_FREEZE;
		} else if (client->noclip) {
			client->ps.pm_type = PM_NOCLIP;
		}

		// set up for pmove
		memset(&pm, 0, sizeof (pm));
		pm.ps            = &client->ps;
		pm.pmext         = &client->pmext;
		pm.character     = client->pers.character;
		pm.cmd           = *ucmd;
		pm.tracemask     = MASK_PLAYERSOLID & ~CONTENTS_BODY; // spectators can fly through bodies
		pm.trace         = trap_TraceCapsuleNoEnts;
		pm.pointcontents = trap_PointContents;

		Pmove(&pm);   // JPW NERVE

		// Rafael - Activate
		// Ridah, made it a latched event (occurs on keydown only)
		if (client->latched_buttons & BUTTON_ACTIVATE) {
			Cmd_Activate_f(ent);
		}

		// save results of pmove
		VectorCopy(client->ps.origin, ent->s.origin);

		G_TouchTriggers(ent);
		trap_UnlinkEntity(ent);
	}

	client->oldbuttons = client->buttons;
	client->buttons    = ucmd->buttons;

//----(SA)	added
	client->oldwbuttons = client->wbuttons;
	client->wbuttons    = ucmd->wbuttons;

	// attack button cycles through spectators
	if ((client->buttons & BUTTON_ATTACK) && !(client->oldbuttons & BUTTON_ATTACK)) {
		Cmd_FollowCycle_f(ent, 1);
	} else if (
	    (client->sess.sessionTeam == TEAM_SPECTATOR) &&   // don't let dead team players do free fly
	    (client->sess.spectatorState == SPECTATOR_FOLLOW) &&
	    (((client->buttons & BUTTON_ACTIVATE) &&
	      !(client->oldbuttons & BUTTON_ACTIVATE)) || ucmd->upmove > 0)) {
		// code moved to StopFollowing
		StopFollowing(ent);
	}
}


/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer(gclient_t *client) {
	// OSP - modified
	if ((g_inactivity.integer == 0 && client->sess.sessionTeam != TEAM_SPECTATOR) || (g_spectatorInactivity.integer == 0 && client->sess.sessionTeam == TEAM_SPECTATOR)) {

		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime    = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if (client->pers.cmd.forwardmove ||
	           client->pers.cmd.rightmove ||
	           client->pers.cmd.upmove ||
	           (client->pers.cmd.wbuttons & WBUTTON_ATTACK2) ||
	           (client->pers.cmd.buttons & BUTTON_ATTACK) ||
	           (client->pers.cmd.wbuttons & WBUTTON_LEANLEFT) ||
	           (client->pers.cmd.wbuttons & WBUTTON_LEANRIGHT)
	           || client->ps.pm_type == PM_DEAD) {

		client->inactivityWarning = qfalse;
		client->inactivityTime    = level.time + 1000 *
		                            ((client->sess.sessionTeam != TEAM_SPECTATOR) ?
		                          g_inactivity.integer :
		                          g_spectatorInactivity.integer);

	} else if (!client->pers.localClient) {
		if (level.time > client->inactivityTime && client->inactivityWarning) {
			client->inactivityWarning = qfalse;
			client->inactivityTime    = level.time + 60 * 1000;

			// Nico, move inactive player to spec instead of kicking them
			// trap_DropClient( client - level.clients, "Dropped due to inactivity", 0 );

			AP(va("cpm \"%s ^7removed from teams due to inactivity! ^z(%i seconds) \n\"", client->pers.netname, g_inactivity.integer));
			SetTeam(g_entities + (client - level.clients), "s", qtrue, -1, -1, qfalse);

			return(qfalse);
		}

		if (!client->inactivityWarning && level.time > client->inactivityTime - 10000) {
			CPx(client - level.clients, "cp \"^310 seconds until inactivity drop!\n\"");
			CPx(client - level.clients, "print \"^310 seconds until inactivity drop!\n\"");
			G_Printf("10s inactivity warning issued to: %s\n", client->pers.netname);

			client->inactivityWarning = qtrue;
			client->inactivityTime    = level.time + 10000; // Just for safety
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions(gentity_t *ent, int msec) {
	gclient_t *client;

	client                = ent->client;
	client->timeResidual += msec;

	while (client->timeResidual >= 1000) {
		client->timeResidual -= 1000;

		// regenerate
		if (client->sess.playerType == PC_MEDIC) {
			if (ent->health < client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health += 3;
				if (ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.1) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.1;
				}
			} else if (ent->health < client->ps.stats[STAT_MAX_HEALTH] * 1.12) {
				ent->health += 2;
				if (ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.12) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.12;
				}
			}
		} else {
			// count down health when over max
			if (ent->health > client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health--;
			}
		}
	}
}

/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents(gentity_t *ent, int oldEventSequence) {
	int       i;
	int       event;
	gclient_t *client;
	int       damage;

	client = ent->client;

	if (oldEventSequence < client->ps.eventSequence - MAX_EVENTS) {
		oldEventSequence = client->ps.eventSequence - MAX_EVENTS;
	}
	for (i = oldEventSequence ; i < client->ps.eventSequence ; i++) {
		event = client->ps.events[i & (MAX_EVENTS - 1)];

		switch (event) {
		case EV_FALL_NDIE:
		case EV_FALL_DMG_10:
		case EV_FALL_DMG_15:
		case EV_FALL_DMG_25:
		case EV_FALL_DMG_50:

			// rain - VectorClear() used to be done here whenever falling
			// damage occured, but I moved it to bg_pmove where it belongs.

			if (ent->s.eType != ET_PLAYER) {
				break;      // not in the player model
			}
			if (event == EV_FALL_NDIE) {
				damage = 9999;
			} else if (event == EV_FALL_DMG_50) {
				damage                    = 50;
				ent->client->ps.pm_time   = 1000;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			} else if (event == EV_FALL_DMG_25) {
				damage                    = 25;
				ent->client->ps.pm_time   = 250;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			} else if (event == EV_FALL_DMG_15) {
				damage                    = 15;
				ent->client->ps.pm_time   = 1000;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			} else if (event == EV_FALL_DMG_10) {
				damage                    = 10;
				ent->client->ps.pm_time   = 1000;
				ent->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			} else {
				damage = 5; // never used
			}
			// VectorSet( dir, 0, 0, 1 );
			ent->pain_debounce_time = level.time + 200; // no normal pain sound
			G_Damage(ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			break;

		case EV_FIRE_WEAPON_MG42:
			mg42_fire(ent);
			break;
		case EV_FIRE_WEAPON_MOUNTEDMG42:
			mountedmg42_fire(ent);
			break;

		case EV_FIRE_WEAPON_AAGUN:
			aagun_fire(ent);
			break;

		case EV_FIRE_WEAPON:
		case EV_FIRE_WEAPONB:
		case EV_FIRE_WEAPON_LASTSHOT:
			FireWeapon(ent);
			break;

		default:
			break;
		}
	}

}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real(gentity_t *ent) {
	int       msec, oldEventSequence;
	pmove_t   pm;
	usercmd_t *ucmd;
	gclient_t *client = ent->client;


	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}

	if (ent->s.eFlags & EF_MOUNTEDTANK) {
		client->pmext.centerangles[YAW]   = ent->tagParent->r.currentAngles[YAW];
		client->pmext.centerangles[PITCH] = ent->tagParent->r.currentAngles[PITCH];
	}

	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	ent->client->ps.identifyClient = ucmd->identClient;     // NERVE - SMF

	// sanity check the command time to prevent speedup cheating
	if (ucmd->serverTime > level.time + 200) {
		ucmd->serverTime = level.time + 200;
	}
	if (ucmd->serverTime < level.time - 1000) {
		ucmd->serverTime = level.time - 1000;
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if (msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW) {
		return;
	}
	if (msec > 200) {
		msec = 200;
	}

	// Nico, pmove_fixed
	if (client->pers.pmoveFixed) {
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer - 1) / pmove_msec.integer) * pmove_msec.integer;
	}

	if (client->wantsscore) {
		G_SendScore(ent);
		client->wantsscore = qfalse;
	}

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	// OSP - moved here to allow for spec inactivity checks as well
	if (!ClientInactivityTimer(client)) {
		return;
	}

	if (!(ucmd->flags & 0x01) || ucmd->forwardmove || ucmd->rightmove || ucmd->upmove || ucmd->wbuttons || ucmd->doubleTap) {
		ent->r.svFlags &= ~(SVF_SELF_PORTAL_EXCLUSIVE | SVF_SELF_PORTAL);
	}

	// spectators don't do much
	// DHM - Nerve :: In limbo use SpectatorThink
	if (client->sess.sessionTeam == TEAM_SPECTATOR || client->ps.pm_flags & PMF_LIMBO) {
		SpectatorThink(ent, ucmd);
		return;
	}

	if ((client->ps.eFlags & EF_VIEWING_CAMERA) || level.match_pause != PAUSE_NONE) {
		ucmd->buttons     = 0;
		ucmd->forwardmove = 0;
		ucmd->rightmove   = 0;
		ucmd->upmove      = 0;
		ucmd->wbuttons    = 0;
		ucmd->doubleTap   = 0;

		// freeze player (RELOAD_FAILED still allowed to move/look)
		if (level.match_pause != PAUSE_NONE) {
			client->ps.pm_type = PM_FREEZE;
		} else if ((client->ps.eFlags & EF_VIEWING_CAMERA)) {
			VectorClear(client->ps.velocity);
			client->ps.pm_type = PM_FREEZE;
		}
	} else if (client->noclip) {
		client->ps.pm_type = PM_NOCLIP;
	} else if (client->ps.stats[STAT_HEALTH] <= 0) {
		client->ps.pm_type = PM_DEAD;
	} else {
		client->ps.pm_type = PM_NORMAL;
	}

	client->ps.aiState = AISTATE_COMBAT;
	client->ps.gravity = DEFAULT_GRAVITY;
	client->ps.speed   = DEFAULT_SPEED;

	if (client->speedScale) {                // Goalitem speed scale
		client->ps.speed *= (client->speedScale * 0.01);
	}

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	client->currentAimSpreadScale = (float)client->ps.aimSpreadScale / 255.0;

	memset(&pm, 0, sizeof (pm));

	pm.ps        = &client->ps;
	pm.pmext     = &client->pmext;
	pm.character = client->pers.character;
	pm.cmd       = *ucmd;
	pm.oldcmd    = client->pers.oldcmd;
	// MrE: always use capsule for AI and player
	pm.trace = trap_TraceCapsule;

	// Nico, ghost players
	pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	if (pm.ps->pm_type == PM_DEAD) {
		pm.ps->eFlags |= EF_DEAD;
	} else if (pm.ps->pm_type == PM_SPECTATOR) {
		pm.trace = trap_TraceCapsuleNoEnts;
	}
	// Nico, end of ghost players

	//DHM - Nerve :: We've gone back to using normal bbox traces
	pm.pointcontents = trap_PointContents;
	pm.debugLevel    = g_debugMove.integer;
	pm.noFootsteps   = qfalse;

	// Nico, pmove_fixed
	// pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_fixed = client->pers.pmoveFixed;
	pm.pmove_msec  = pmove_msec.integer;

	// Nico, game physics
	pm.physics = physics.integer;

	pm.isTimerun        = isTimerun.integer;
	pm.timerunActive    = client->sess.timerunActive;
	pm.timerunStartTime = client->sess.timerunStartTime + 500;

	// Nico, store logins status in pmove
	if (client->sess.logged) {
		pm.isLogged = 1;
	} else {
		pm.isLogged = 0;
	}

	pm.noWeapClips = qfalse;

	VectorCopy(client->ps.origin, client->oldOrigin);

	// NERVE - SMF
	pm.ltChargeTime       = level.lieutenantChargeTime[client->sess.sessionTeam - 1];
	pm.soldierChargeTime  = level.soldierChargeTime[client->sess.sessionTeam - 1];
	pm.engineerChargeTime = level.engineerChargeTime[client->sess.sessionTeam - 1];
	pm.medicChargeTime    = level.medicChargeTime[client->sess.sessionTeam - 1];
	// -NERVE - SMF

	client->pmext.airleft = ent->client->airOutTime - level.time;

	pm.covertopsChargeTime = level.covertopsChargeTime[client->sess.sessionTeam - 1];

	pm.leadership = qfalse;

	// Gordon: bit hacky, stop the slight lag from client -> server even on locahost, switching back to the weapon you were holding
	//			and then back to what weapon you should have, became VERY noticible for the kar98/carbine + gpg40, esp now i've added the
	//			animation locking
	if (level.time - client->pers.lastSpawnTime < 1000) {
		pm.cmd.weapon = client->ps.weapon;
	}

	Pmove(&pm);

	// Gordon: thx to bani for this
	// ikkyo - fix leaning players bug
	VectorCopy(client->ps.velocity, ent->s.pos.trDelta);
	SnapVector(ent->s.pos.trDelta);
	// end

	// server cursor hints
	// TAT 1/10/2003 - bots don't need to check for cursor hints
	if (!(ent->r.svFlags & SVF_BOT) && ent->lastHintCheckTime < level.time) {
		G_CheckForCursorHints(ent);

		ent->lastHintCheckTime = level.time + FRAMETIME;
	}

	// DHM - Nerve :: Set animMovetype to 1 if ducking
	if (ent->client->ps.pm_flags & PMF_DUCKED) {
		ent->s.animMovetype = 1;
	} else {
		ent->s.animMovetype = 0;
	}

	// save results of pmove
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime   = level.time;
		ent->r.eventTime = level.time;
	}

	// Ridah, fixes jittery zombie movement
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate(&ent->client->ps, &ent->s, level.time, qfalse);
	} else {
		BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, qfalse);
	}

	if (!(ent->client->ps.eFlags & EF_FIRING)) {
		client->fireHeld = qfalse;      // for grapple
	}

//	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy(ent->s.pos.trBase, ent->r.currentOrigin);

	VectorCopy(pm.mins, ent->r.mins);
	VectorCopy(pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype  = pm.watertype;

	// execute client events
	if (level.match_pause == PAUSE_NONE) {
		ClientEvents(ent, oldEventSequence);
	}

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity(ent);
	if (!ent->client->noclip) {
		G_TouchTriggers(ent);
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy(ent->client->ps.origin, ent->r.currentOrigin);

	// touch other objects
	ClientImpacts(ent, &pm);

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons      = client->buttons;
	client->buttons         = ucmd->buttons;
	client->latched_buttons = client->buttons & ~client->oldbuttons;

	//----(SA)	added
	client->oldwbuttons      = client->wbuttons;
	client->wbuttons         = ucmd->wbuttons;
	client->latched_wbuttons = client->wbuttons & ~client->oldwbuttons;

	// Rafael - Activate
	// Ridah, made it a latched event (occurs on keydown only)
	if (client->latched_buttons & BUTTON_ACTIVATE) {
		Cmd_Activate_f(ent);
	}

	if (g_entities[ent->client->ps.identifyClient].team == ent->team && g_entities[ent->client->ps.identifyClient].client) {
	} else {
		ent->client->ps.identifyClient = -1;
	}

	// check for respawning
	if (client->ps.stats[STAT_HEALTH] <= 0) {
		// Nico, forcing respawn
		limbo(ent);

		return;
	}

	// perform once-a-second actions
	if (level.match_pause == PAUSE_NONE) {
		ClientTimerActions(ent, msec);
	}

	// Nico, pmove_fixed
	if (!client->pers.pmoveFixed) {
		CP(va("cpm \"%s^w: ^1you were removed from teams because you can not use pmove_fixed 0\n\"", GAME_VERSION_COLORED));
		trap_SendServerCommand(ent - g_entities, "pmoveon");
		SetTeam(ent, "s", qtrue, -1, -1, qfalse);
	}

	// Nico, check max FPS
	if (client->pers.maxFPS < 40) {
		CP(va("cpm \"%s^w: ^1you were removed from teams because you must use com_maxfps > 40\n\"", GAME_VERSION_COLORED));
		trap_SendServerCommand(ent - g_entities, "resetMaxFPS");
		SetTeam(ent, "s", qtrue, -1, -1, qfalse);
	}

	// Nico, force auto demo record in cup mode
	if (g_cupMode.integer != 0 && client->pers.autoDemo == 0) {
		CP(va("cpm \"%s^w: ^1you were removed from teams because you must use cg_autoDemo 1\n\"", GAME_VERSION_COLORED));
		trap_SendServerCommand(ent - g_entities, "autoDemoOn");
		SetTeam(ent, "s", qtrue, -1, -1, qfalse);
	}

	// Nico, check ping
	if (client->sess.timerunActive && client->ps.ping > 400) {
		CP(va("cpm \"%s^w: ^1too high ping detected, timerun stopped\n\"", GAME_VERSION_COLORED));
		// Nico, notify the client and its spectators the timerun has stopped
		notify_timerun_stop(ent, 0);
		client->sess.timerunActive = qfalse;
	}

	// Nico, check maxpackets
	if (client->pers.clientMaxPackets < 30) {
		CP(va("cpm \"%s^w: ^1cl_maxpackets is too low, must be >= 30\n\"", GAME_VERSION_COLORED));
		trap_SendServerCommand(ent - g_entities, "resetMaxpackets");
		SetTeam(ent, "s", qtrue, -1, -1, qfalse);
	}

	// Nico, force hide me in cup mode
	if (g_cupMode.integer != 0 && client->pers.hideme == 0) {
		CP(va("cpm \"%s^w: ^1you were removed from teams because you must use cg_hideMe 1\n\"", GAME_VERSION_COLORED));
		trap_SendServerCommand(ent - g_entities, "hideMeOn");
		SetTeam(ent, "s", qtrue, -1, -1, qfalse);
	}

	// Nico, force CGaz 0 in cup mode
	if (g_cupMode.integer != 0 && client->pers.cgaz != 0) {
		CP(va("cpm \"%s^w: ^1you were removed from teams because you must use cg_drawCGaz 0\n\"", GAME_VERSION_COLORED));
		trap_SendServerCommand(ent - g_entities, "CGazOff");
		SetTeam(ent, "s", qtrue, -1, -1, qfalse);
	}
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink(int clientNum) {
	gentity_t *ent;

	ent                      = g_entities + clientNum;
	ent->client->pers.oldcmd = ent->client->pers.cmd;
	trap_GetUsercmd(clientNum, &ent->client->pers.cmd);

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	ClientThink_real(ent);
}


void G_RunClient(gentity_t *ent) {
	// Gordon: special case for uniform grabbing
	if (ent->client->pers.cmd.buttons & BUTTON_ACTIVATE) {
		Cmd_Activate2_f(ent);
	}

	if (ent->health <= 0 && ent->client->ps.pm_flags & PMF_LIMBO) {
		if (ent->r.linked) {
			trap_UnlinkEntity(ent);
		}
	}
}

/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame(gentity_t *ent) {
	// OSP - specs periodically get score updates for useful demo playback info

	// if we are doing a chase cam or a remote view, grab the latest info
	if ((ent->client->sess.spectatorState == SPECTATOR_FOLLOW) || (ent->client->ps.pm_flags & PMF_LIMBO)) {
		int       clientNum;
		gclient_t *cl;

		if (ent->client->sess.sessionTeam == TEAM_AXIS || ent->client->sess.sessionTeam == TEAM_ALLIES) {
			reinforce(ent);
			return;
		}

		// Limbos aren't following while in MV
		if ((ent->client->ps.pm_flags & PMF_LIMBO)) {
			return;
		}

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if (clientNum == -1) {
			clientNum = level.follow1;
		} else if (clientNum == -2) {
			clientNum = level.follow2;
		}

		if (clientNum >= 0) {
			cl = &level.clients[clientNum];
			if (cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR) {
				int flags = (cl->ps.eFlags & ~(EF_VOTED)) | (ent->client->ps.eFlags & (EF_VOTED));
				int ping  = ent->client->ps.ping;

				if (ent->client->sess.sessionTeam != TEAM_SPECTATOR && (ent->client->ps.pm_flags & PMF_LIMBO)) {
					int savedScore = ent->client->ps.persistant[PERS_SCORE];
					int savedClass = ent->client->ps.stats[STAT_PLAYER_CLASS];

					ent->client->ps                          = cl->ps;
					ent->client->ps.pm_flags                |= PMF_FOLLOW;
					ent->client->ps.pm_flags                |= PMF_LIMBO;
					ent->client->ps.persistant[PERS_SCORE]   = savedScore;          // put score back
					ent->client->ps.stats[STAT_PLAYER_CLASS] = savedClass;          // NERVE - SMF - put player class back
				} else {
					ent->client->ps           = cl->ps;
					ent->client->ps.pm_flags |= PMF_FOLLOW;
				}

				// DHM - Nerve :: carry flags over
				ent->client->ps.eFlags = flags;
				ent->client->ps.ping   = ping;

				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if (ent->client->sess.spectatorClient >= 0) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin(ent->client - level.clients);
				}
			}
		}
	}
}


// DHM - Nerve :: After reviving a player, their contents stay CONTENTS_CORPSE until it is determined
//					to be safe to return them to PLAYERSOLID

qboolean StuckInClient(gentity_t *self) {
	int       i;
	vec3_t    hitmin, hitmax;
	vec3_t    selfmin, selfmax;
	gentity_t *hit;

	for (i = 0; i < level.numConnectedClients; i++) {
		hit = g_entities + level.sortedClients[i];

		if (!hit->inuse || hit == self || !hit->client ||
		    !hit->s.solid || hit->health <= 0) {
			continue;
		}

		VectorAdd(hit->r.currentOrigin, hit->r.mins, hitmin);
		VectorAdd(hit->r.currentOrigin, hit->r.maxs, hitmax);
		VectorAdd(self->r.currentOrigin, self->r.mins, selfmin);
		VectorAdd(self->r.currentOrigin, self->r.maxs, selfmax);

		if (hitmin[0] > selfmax[0]) {
			continue;
		}
		if (hitmax[0] < selfmin[0]) {
			continue;
		}
		if (hitmin[1] > selfmax[1]) {
			continue;
		}
		if (hitmax[1] < selfmin[1]) {
			continue;
		}
		if (hitmin[2] > selfmax[2]) {
			continue;
		}
		if (hitmax[2] < selfmin[2]) {
			continue;
		}

		return(qtrue);
	}

	return(qfalse);
}

extern vec3_t playerMins, playerMaxs;

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEndFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame(gentity_t *ent) {
	int i;

	// Nico, timerun stats
	float currentSpeed = 0;

	// Nico, flood protection
	if (level.time >= (ent->client->sess.nextReliableTime + 1000) &&
	    ent->client->sess.numReliableCmds) {
		ent->client->sess.numReliableCmds--;

		// Reset the threshold because they were good for a bit
		if (!ent->client->sess.numReliableCmds) {
			ent->client->sess.thresholdTime = 0;
		}
	}

	// Nico, update best speeds
	if (ent->client->sess.timerunActive) {
		currentSpeed = sqrt(ent->client->ps.velocity[0] * ent->client->ps.velocity[0] + ent->client->ps.velocity[1] * ent->client->ps.velocity[1]);

		// Nico, update overall max speed
		if (currentSpeed > ent->client->sess.overallMaxSpeed) {
			ent->client->sess.overallMaxSpeed = currentSpeed;
		}

		// Nico, update max speed of the current run
		if (currentSpeed > ent->client->sess.maxSpeed) {
			ent->client->sess.maxSpeed = currentSpeed;
		}
	}

	// used for informing of speclocked teams.
	// Zero out here and set only for certain specs
	ent->client->ps.powerups[PW_BLACKOUT] = 0;

	if ((ent->client->sess.sessionTeam == TEAM_SPECTATOR) || (ent->client->ps.pm_flags & PMF_LIMBO)) {       // JPW NERVE
		SpectatorClientEndFrame(ent);
		return;
	}

	// turn off any expired powerups
	// OSP -- range changed for MV
	for (i = 0 ; i < PW_NUM_POWERUPS ; i++) {

		if (i == PW_FIRE ||                 // these aren't dependant on level.time
		    i == PW_ELECTRIC ||
		    i == PW_BREATHER ||
		    ent->client->ps.powerups[i] == 0            // OSP
		    || i == PW_OPS_CLASS_1
		    || i == PW_OPS_CLASS_2
		    || i == PW_OPS_CLASS_3

		    ) {

			continue;
		}
		// OSP -- If we're paused, update powerup timers accordingly.
		// Make sure we dont let stuff like CTF flags expire.
		if (level.match_pause != PAUSE_NONE &&
		    ent->client->ps.powerups[i] != INT_MAX) {
			ent->client->ps.powerups[i] += level.time - level.previousTime;
		}


		if (ent->client->ps.powerups[i] < level.time) {
			ent->client->ps.powerups[i] = 0;
		}
	}

	// OSP - If we're paused, make sure other timers stay in sync
	//		--> Any new things in ET we should worry about?
	if (level.match_pause != PAUSE_NONE) {
		int time_delta = level.time - level.previousTime;

		ent->client->airOutTime                        += time_delta;
		ent->client->inactivityTime                    += time_delta;
		ent->client->lastBurnTime                      += time_delta;
		ent->client->pers.connectTime                  += time_delta;
		ent->client->pers.enterTime                    += time_delta;
		ent->client->pers.teamState.lastreturnedflag   += time_delta;
		ent->client->pers.teamState.lasthurtcarrier    += time_delta;
		ent->client->pers.teamState.lastfraggedcarrier += time_delta;
		ent->client->ps.classWeaponTime                += time_delta;
		ent->lastHintCheckTime                         += time_delta;
		ent->pain_debounce_time                        += time_delta;
		ent->s.onFireEnd                               += time_delta;
	}

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//

	// burn from lava, etc
	P_WorldEffects(ent);

	// apply all the damage taken this frame
	P_DamageFeedback(ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if (level.time - ent->client->lastCmdTime > 1000) {
		ent->s.eFlags |= EF_CONNECTION;
	} else {
		ent->s.eFlags &= ~EF_CONNECTION;
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;   // FIXME: get rid of ent->health...
	                                                    // Gordon: WHY? other ents use it.

	G_SetClientSound(ent);

	// set the latest infor

	// Ridah, fixes jittery zombie movement
	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate(&ent->client->ps, &ent->s, level.time, qfalse);
	} else {
		BG_PlayerStateToEntityState(&ent->client->ps, &ent->s, qfalse);
	}

	// DHM - Nerve :: If it's been a couple frames since being revived, and props_frame_state
	//					wasn't reset, go ahead and reset it
	if (ent->props_frame_state >= 0 && ((level.time - ent->s.effect3Time) > 100)) {
		ent->props_frame_state = -1;
	}

	// DHM - Nerve :: Reset 'count2' for flamethrower
	if (!(ent->client->buttons & BUTTON_ATTACK)) {
		ent->count2 = 0;
	}
	// dhm

	// zinx - #280 - run touch functions here too, so movers don't have to wait
	// until the next ClientThink, which will be too late for some map
	// scripts (railgun)
	G_TouchTriggers(ent);

	// run entity scripting
	G_Script_ScriptRun(ent);

	// store the client's current position for antilag traces
	G_StoreClientPosition(ent);
}
