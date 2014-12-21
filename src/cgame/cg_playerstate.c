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



// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"

/*
==============
CG_DamageFeedback
==============
*/
void CG_DamageFeedback(int yawByte, int pitchByte, int damage) {
	float        kick;
	int          health;
	float        scale;
	int          slot;
	viewDamage_t *vd;

	// show the attacking player's head and name in corner
	cg.attackerTime = cg.time;

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[STAT_HEALTH];
	if (health < 40) {
		scale = 1;
	} else {
		scale = 40.0 / health;
	}
	kick = damage * scale;

	if (kick < 5) {
		kick = 5;
	}
	if (kick > 10) {
		kick = 10;
	}

	// find a free slot
	for (slot = 0; slot < MAX_VIEWDAMAGE; ++slot) {
		if (cg.viewDamage[slot].damageTime + cg.viewDamage[slot].damageDuration < cg.time) {
			break;
		}
	}

	if (slot == MAX_VIEWDAMAGE) {
		return;     // no free slots, never override or splats will suddenly disappear

	}
	vd = &cg.viewDamage[slot];

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if (yawByte == 255 && pitchByte == 255) {
		vd->damageX    = 0;
		vd->damageY    = 0;
		cg.v_dmg_roll  = 0;
		cg.v_dmg_pitch = -kick;
	} else {
		float        left, front, up, dist, yaw, pitch;
		vec3_t       dir, angles;

		// positional
		pitch = pitchByte / 255.0 * 360;
		yaw   = yawByte / 255.0 * 360;

		angles[PITCH] = pitch;
		angles[YAW]   = yaw;
		angles[ROLL]  = 0;

		AngleVectors(angles, dir, NULL, NULL);
		VectorSubtract(vec3_origin, dir, dir);

		front = DotProduct(dir, cg.refdef.viewaxis[0]);
		left  = DotProduct(dir, cg.refdef.viewaxis[1]);
		up    = DotProduct(dir, cg.refdef.viewaxis[2]);

		dir[0] = front;
		dir[1] = left;
		dir[2] = 0;
		dist   = VectorLength(dir);
		if (dist < 0.1) {
			dist = 0.1f;
		}

		cg.v_dmg_roll = kick * left;

		cg.v_dmg_pitch = -kick * front;

		if (front <= 0.1) {
			front = 0.1f;
		}
		vd->damageX = crandom() * 0.3 + -left / front;
		vd->damageY = crandom() * 0.3 + up / dist;
	}

	// clamp the position
	if (vd->damageX > 1.0) {
		vd->damageX = 1.0;
	}
	if (vd->damageX < -1.0) {
		vd->damageX = -1.0;
	}

	if (vd->damageY > 1.0) {
		vd->damageY = 1.0;
	}
	if (vd->damageY < -1.0) {
		vd->damageY = -1.0;
	}

	// don't let the screen flashes vary as much
	if (kick > 10) {
		kick = 10;
	}
	vd->damageValue    = kick;
	vd->damageTime     = cg.snap->serverTime;
	vd->damageDuration = kick * 50 * (1 + 2 * (!vd->damageX && !vd->damageY));
	cg.damageTime      = cg.snap->serverTime;
}




/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn() {
	cg.serverRespawning = qfalse;   // Arnout: just in case

	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// need to reset client-side weapon animations
	cg.predictedPlayerState.weapAnim    = ((cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT) ^ ANIM_TOGGLEBIT) | PM_IdleAnimForWeapon(cg.snap->ps.weapon);      // reset weapon animations
	cg.predictedPlayerState.weaponstate = WEAPON_READY; // hmm, set this?  what to?

	// display weapons available
	cg.weaponSelectTime = cg.time;

	cg.cursorHintIcon = 0;
	cg.cursorHintTime = 0;

	cg.cameraMode = qfalse; //----(SA)	get out of camera for sure

	// select the weapon the server says we are using
	cg.weaponSelect = cg.snap->ps.weapon;
	// DHM - Nerve :: Clear even more things on respawn
	cg.zoomedBinoc = qfalse;
	cg.zoomedScope = qfalse;
	cg.zoomTime    = 0;
	cg.zoomval     = 0;

	trap_SendConsoleCommand("-zoom\n");
	cg.binocZoomTime = 0;

	// clear pmext
	memset(&cg.pmext, 0, sizeof (cg.pmext));

	cg.pmext.bAutoReload = (cg_autoReload.integer > 0);

	cgs.limboLoadoutSelected = qfalse;

	if (cg.predictedPlayerState.stats[STAT_PLAYER_CLASS] == PC_COVERTOPS) {
		cg.pmext.silencedSideArm = 1;
	}

	cg.proneMovingTime = 0;

	// reset fog to world fog (if present)
	trap_R_SetFog(FOG_CMD_SWITCHFOG, FOG_MAP, 20, 0, 0, 0, 0);
	// dhm - end
}

void CG_CheckPlayerstateEvents(playerState_t *ps, playerState_t *ops) {
	int       i;
	int       event;
	centity_t *cent;

	if (ps->externalEvent && ps->externalEvent != ops->externalEvent) {
		cent                         = &cg_entities[ps->clientNum];
		cent->currentState.event     = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent(cent, cent->lerpOrigin);
	}

	cent = &cg.predictedPlayerEntity;
	// go through the predictable events buffer
	for (i = ps->eventSequence - MAX_EVENTS ; i < ps->eventSequence ; ++i) {
		// if we have a new predictable event
		if (i >= ops->eventSequence
		    // or the server told us to play another event instead of a predicted event we already issued
		    // or something the server told us changed our prediction causing a different event
		    || (i > ops->eventSequence - MAX_EVENTS && ps->events[i & (MAX_EVENTS - 1)] != ops->events[i & (MAX_EVENTS - 1)])) {

			event                        = ps->events[i & (MAX_EVENTS - 1)];
			cent->currentState.event     = event;
			cent->currentState.eventParm = ps->eventParms[i & (MAX_EVENTS - 1)];
			CG_EntityEvent(cent, cent->lerpOrigin);

			cg.predictableEvents[i & (MAX_PREDICTED_EVENTS - 1)] = event;

			cg.eventSequence++;
		}
	}
}

/*
==================
CG_CheckLocalSounds
==================
*/
void CG_CheckLocalSounds(playerState_t *ps, playerState_t *ops) {
	// health changes of more than -1 should make pain sounds
	if (ps->stats[STAT_HEALTH] < ops->stats[STAT_HEALTH] - 1 && ps->stats[STAT_HEALTH] > 0) {
		CG_PainEvent(&cg.predictedPlayerEntity, ps->stats[STAT_HEALTH]);
		cg.painTime = cg.time;
	}
}

/*
===============
CG_TransitionPlayerState

===============
*/
void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops) {
	// check for changing follow mode
	if (ps->clientNum != ops->clientNum) {
		cg.thisFrameTeleport = qtrue;

		// clear voicechat
		cg.predictedPlayerEntity.voiceChatSpriteTime   = 0;
		cg_entities[ps->clientNum].voiceChatSpriteTime = 0;

		// make sure we don't get any unwanted transition effects
		*ops = *ps;

		// DHM - Nerve :: After Limbo, make sure and do a CG_Respawn
		if (ps->clientNum == cg.clientNum) {
			ops->persistant[PERS_SPAWN_COUNT]--;
		}
	}

	if (ps->eFlags & EF_FIRING) {
		cg.lastFiredWeaponTime = 0;
		cg.weaponFireTime     += cg.frametime;
	} else {
		if (cg.weaponFireTime > 500 && cg.weaponFireTime) {
			cg.lastFiredWeaponTime = cg.time;
		}

		cg.weaponFireTime = 0;
	}

	// damage events (player is getting wounded)
	if (ps->damageEvent != ops->damageEvent && ps->damageCount) {
		CG_DamageFeedback(ps->damageYaw, ps->damagePitch, ps->damageCount);
	}

	// respawning
	if (ps->persistant[PERS_SPAWN_COUNT] != ops->persistant[PERS_SPAWN_COUNT]) {
		CG_Respawn();
	}

	CG_CheckLocalSounds(ps, ops);

	if (ps->eFlags & EF_PRONE_MOVING) {
		if (ps->weapon == WP_BINOCULARS && ps->eFlags & EF_ZOOMING) {
			trap_SendConsoleCommand("-zoom\n");
		}

		if (!(ops->eFlags & EF_PRONE_MOVING)) {
			cg.proneMovingTime = cg.time;
		}
	} else if (ops->eFlags & EF_PRONE_MOVING) {
		cg.proneMovingTime = -cg.time;
	}

	if (!(ps->eFlags & EF_PRONE) && ops->eFlags & EF_PRONE && cg.weaponSelect == WP_MOBILE_MG42_SET) {
		CG_FinishWeaponChange(cg.weaponSelect, ps->nextWeapon);
	}

	// run events
	CG_CheckPlayerstateEvents(ps, ops);

	// smooth the ducking viewheight change
	if (ps->viewheight != ops->viewheight) {
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime   = cg.time;
	}
}
