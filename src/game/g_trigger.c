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

void InitTrigger(gentity_t *self) {
	if (!VectorCompare(self->s.angles, vec3_origin)) {
		G_SetMovedir(self->s.angles, self->movedir);
	}

	// Nico, fix against error: SV_SetBrushModel: NULL
	if (self->model) {
		trap_SetBrushModel(self, self->model);
	}

	self->r.contents = CONTENTS_TRIGGER;        // replaces the -1 from trap_SetBrushModel
	self->r.svFlags  = SVF_NOCLIENT;
}

// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
// Nico, note: ent->random is effect less on this entity
void multi_trigger(gentity_t *ent, gentity_t *activator) {
	ent->activator = activator;
	G_Script_ScriptEvent(ent, "activate", NULL);

	// Nico, #todo: handle case when ent->triggerTime[activator->client->ps.clientNum] = 0
	if (ent->wait > 0) {
		if (activator->client && ent->triggerTime[activator->client->ps.clientNum] + ent->wait * 1000 > level.time) {
			// Client has to wait before triggering this entity!
			return;
		}
		G_UseTargets(ent, ent->activator);
		if (activator->client) {
			ent->triggerTime[activator->client->ps.clientNum] = level.time;
		}
	} else {
		// Nico, #todo: update ent->triggerTime[activator->client->ps.clientNum]???
		G_UseTargets(ent, ent->activator);
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch     = 0;
		ent->nextthink = level.time + FRAMETIME;
		ent->think     = G_FreeEntity;
	}
}

void Use_Multi(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;

	multi_trigger(ent, activator);
}

void Touch_Multi(gentity_t *self, gentity_t *other, trace_t *trace) {
	// Nico, silent GCC
	(void)trace;

	if (!other->client) {
		return;
	}

	if (self->spawnflags & 1) {
		if (other->client->sess.sessionTeam != TEAM_AXIS) {
			return;
		}
	} else if ((self->spawnflags & 2) && other->client->sess.sessionTeam != TEAM_ALLIES) {
		return;
	}

	if (self->spawnflags & 8) {
		return;
	}

	// START Mad Doc - TDF
	if ((self->spawnflags & 16) && !(other->client->sess.playerType == PC_SOLDIER)) {
		return;
	}

	if ((self->spawnflags & 32) && !(other->client->sess.playerType == PC_FIELDOPS)) {
		return;
	}

	if ((self->spawnflags & 64) && !(other->client->sess.playerType == PC_MEDIC)) {
		return;
	}

	if ((self->spawnflags & 128) && !(other->client->sess.playerType == PC_ENGINEER)) {
		return;
	}

	if ((self->spawnflags & 256) && !(other->client->sess.playerType == PC_COVERTOPS)) {
		return;
	}
	// END Mad Doc - TDF

	multi_trigger(self, other);
}

/*QUAKED trigger_multiple (.5 .5 .5) ? AXIS_ONLY ALLIED_ONLY NOBOT BOTONLY SOLDIERONLY LTONLY MEDICONLY ENGINEERONLY COVERTOPSONLY
"wait" : Seconds between triggerings, 0.5 default, -1 = one time only.
"random"	wait variance, default is 0
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
*/
void SP_trigger_multiple(gentity_t *ent) {
	G_SpawnFloat("wait", "0.5", &ent->wait);

	ent->touch   = Touch_Multi;
	ent->use     = Use_Multi;
	ent->s.eType = ET_TRIGGER_MULTIPLE;

	InitTrigger(ent);

	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity(ent);
}

/*
==============================================================================

trigger_always

==============================================================================
*/

void trigger_always_think(gentity_t *ent) {
	G_UseTargets(ent, ent);
	G_FreeEntity(ent);
}

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always(gentity_t *ent) {
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 300;
	ent->think     = trigger_always_think;
}

/*
==============================================================================

trigger_push

==============================================================================
*/

void trigger_push_touch(gentity_t *self, gentity_t *other, trace_t *trace) {
	// Nico, silent GCC
	(void)trace;

	// Nico, jumppads support
	if (!(g_enableMapEntities.integer & MAP_JUMPPADS) || !other->client) {
		return;
	}

	BG_TouchJumpPad(&other->client->ps, &self->s);
}

/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void AimAtTarget(gentity_t *self) {
	gentity_t *ent;
	vec3_t    origin;
	float     height, gravity, time, forward;
	float     dist;

	VectorAdd(self->r.absmin, self->r.absmax, origin);
	VectorScale(origin, 0.5, origin);

	ent = G_PickTarget(self->target);
	if (!ent) {
		G_FreeEntity(self);
		return;
	}

	height = ent->s.origin[2] - origin[2];

	gravity = DEFAULT_GRAVITY;

	time = sqrt(fabs(height / (0.5f * gravity)));
	if (!time) {
		G_FreeEntity(self);
		return;
	}

	// set s.origin2 to the push velocity
	VectorSubtract(ent->s.origin, origin, self->s.origin2);
	self->s.origin2[2] = 0;
	dist               = VectorNormalize(self->s.origin2);

	forward = dist / time;
	VectorScale(self->s.origin2, forward, self->s.origin2);

	self->s.origin2[2] = time * gravity;
}

/*QUAKED trigger_push (.5 .5 .5) ? TOGGLE REMOVEAFTERTOUCH PUSHPLAYERONLY
Must point at a target_position, which will be the apex of the leap.
This will be client side predicted, unlike target_push
*/
void SP_trigger_push(gentity_t *self) {
	// Nico, jumppads support (from TJMod)
	InitTrigger(self);

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;

	self->s.eType   = ET_PUSH_TRIGGER;
	self->touch     = trigger_push_touch;
	self->think     = AimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	trap_LinkEntity(self);
}

void Use_target_push(gentity_t *self, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;

	if (!activator->client) {
		return;
	}

	if (activator->client->ps.pm_type != PM_NORMAL) {
		return;
	}

	VectorCopy(self->s.origin2, activator->client->ps.velocity);

	// play fly sound every 1.5 seconds
	if (activator->fly_sound_debounce_time < level.time) {
		activator->fly_sound_debounce_time = level.time + 1500;
		G_Sound(activator, self->noise_index);
	}
}

/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) bouncepad
Pushes the activator in the direction.of angle, or towards a target apex.
"speed"		defaults to 1000
if "bouncepad", play bounce noise instead of windfly
*/
void SP_target_push(gentity_t *self) {
	if (!self->speed) {
		self->speed = 1000;
	}
	G_SetMovedir(self->s.angles, self->s.origin2);
	VectorScale(self->s.origin2, self->speed, self->s.origin2);

	if (self->spawnflags & 1) {
		self->noise_index = G_SoundIndex("sound/world/jumppad.wav");
	} else {
		self->noise_index = G_SoundIndex("sound/misc/windfly.wav");
	}
	if (self->target) {
		VectorCopy(self->s.origin, self->r.absmin);
		VectorCopy(self->s.origin, self->r.absmax);
		self->think     = AimAtTarget;
		self->nextthink = level.time + FRAMETIME;
	}
	self->use = Use_target_push;
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/

void trigger_teleporter_touch(gentity_t *self, gentity_t *other, trace_t *trace) {
	gentity_t *dest;

	// Nico, silent GCC
	(void)trace;

	if (!other->client) {
		return;
	}
	if (other->client->ps.pm_type == PM_DEAD) {
		return;
	}

	dest = G_PickTarget(self->target);
	if (!dest) {
		G_Printf("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer(other, dest->s.origin, dest->s.angles);
}

/*QUAKED trigger_teleport (.5 .5 .5) ?
Allows client side prediction of teleportation events.
Must point at a target_position, which will be the teleport destination.
*/
void SP_trigger_teleport(gentity_t *self) {
	InitTrigger(self);

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;

	// make sure the client precaches this sound
	G_SoundIndex("sound/world/jumppad.wav");

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->touch   = trigger_teleporter_touch;

	trap_LinkEntity(self);
}

/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF - SILENT NO_PROTECTION SLOW ONCE
Any entity that touches this will be hurt.
It does dmg points of damage each server frame
Targeting the trigger will toggle its on / off state.

SILENT			supresses playing the sound
SLOW			changes the damage rate to once per second
NO_PROTECTION	*nothing* stops the damage

"dmg"			default 5 (whole numbers only)

"life"	time this brush will exist if value is zero will live for ever ei 0.5 sec 2.sec
default is zero

the entity must be used first before it will count down its life
*/
void hurt_touch(gentity_t *self, gentity_t *other, trace_t *trace) {
	int dflags;

	// Nico, silent GCC
	(void)trace;

	if (!other->takedamage) {
		return;
	}

	if (self->timestamp > level.time) {
		return;
	}

	// Nico, make hurt triggers slow
	if (self->spawnflags & 16) {
		self->timestamp = level.time + 1000;
	} else {
		self->timestamp = level.time + FRAMETIME;
	}

	// play sound
	if (!(self->spawnflags & 4)) {
		G_Sound(other, self->noise_index);
	}

	if (self->spawnflags & 8) {
		dflags = DAMAGE_NO_PROTECTION;
	} else {
		dflags = 0;
	}

	G_Damage(other, self, self, NULL, NULL, self->damage, dflags, MOD_TRIGGER_HURT);

	if (self->spawnflags & 32) {
		self->touch = NULL;
	}
}

void hurt_think(gentity_t *ent) {
	ent->nextthink = level.time + FRAMETIME;

	if (ent->wait < level.time) {
		G_FreeEntity(ent);
	}
}

void hurt_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;
	(void)activator;

	if (self->touch) {
		self->touch = NULL;
	} else {
		self->touch = hurt_touch;
	}

	if (self->delay) {
		self->nextthink = level.time + 50;
		self->think     = hurt_think;
		self->wait      = level.time + (self->delay * 1000);
	}
}

/*
==============
SP_trigger_hurt
==============
*/
void SP_trigger_hurt(gentity_t *self) {

	char  *life, *sound;   // JPW NERVE
	float dalife;

	InitTrigger(self);

	// Nico, new sound
	G_SpawnString("sound", "sound/weapons/grenade/gren_throw.wav", &sound);

	self->noise_index = G_SoundIndex(sound);

	if (!self->damage) {
		self->damage = 5;
	}

	self->r.contents = CONTENTS_TRIGGER;

	self->use = hurt_use;

	// Nico, enable hurt entity if needed
	if (!(g_enableMapEntities.integer & MAP_DISABLE_HURT_ENTITIES) && (g_enableMapEntities.integer & MAP_FORCE_HURT_ENTITIES || !(self->spawnflags & 1))) {
		self->touch = hurt_touch;
	}

	G_SpawnString("life", "0", &life);
	dalife      = atof(life);
	self->delay = dalife;
}

// START	xkan, 9/17/2002
/*
==============================================================================

trigger_heal

==============================================================================
*/

/*QUAKED trigger_heal (.5 .5 .5) ?
Any entity that touches this will be healed at a specified rate up to a specified
maximum.

"healrate"		rate of healing per second, default 5 (whole numbers only)
"healtotal"		the maximum of healing this trigger can do. if <= 0, it's unlimited.
                default 0 (whole numbers only)
"target"		cabinet that this entity is linked to
*/

qboolean G_IsAllowedHeal(gentity_t *ent) {
	if (!ent || !ent->client) {
		return qfalse;
	}

	if (ent->health <= 0 || ent->health >= ent->client->ps.stats[STAT_MAX_HEALTH]) {
		return qfalse;
	}

	return qtrue;
}

void heal_touch(gentity_t *self, gentity_t *other, trace_t *trace) {
	int       i, clientcount = 0;
	gentity_t *touchClients[MAX_CLIENTS];

	// Nico, silent GCC
	(void)trace;

	memset(touchClients, 0, sizeof (touchClients));

	if (!other->client) {
		return;
	}

	if (self->timestamp > level.time) {
		return;
	}
	self->timestamp = level.time + 1000;

	for (i = 0; i < level.numConnectedClients; ++i) {
		int j = level.sortedClients[i];

		if (level.clients[j].ps.stats[STAT_MAX_HEALTH] > g_entities[j].health && trap_EntityContactCapsule(g_entities[j].r.absmin, g_entities[j].r.absmax, self) && G_IsAllowedHeal(&g_entities[j])) {
			touchClients[clientcount] = &g_entities[j];
			clientcount++;
		}
	}

	if (clientcount == 0) {
		return;
	}

	for (i = 0; i < clientcount; ++i) {
		int healvalue;

		healvalue = min(touchClients[i]->client->ps.stats[STAT_MAX_HEALTH] - touchClients[i]->health, self->damage);
		if (self->health != -9999) {
			healvalue = min(healvalue, self->health);
		}
		if (healvalue <= 0) {
			continue;
		}

		touchClients[i]->health += healvalue;
		// add the medicheal event (to get sound, etc.)
		G_AddPredictableEvent(other, EV_ITEM_PICKUP, BG_FindItemForClassName("item_health_cabinet") - bg_itemlist);

		if (self->health != -9999) {
			self->health -= healvalue;
		}
	}
}

#define HEALTH_REGENTIME 10000
void trigger_heal_think(gentity_t *self) {
	self->nextthink = level.time + HEALTH_REGENTIME;

	self->health += self->damage;
	if (self->health > self->count) {
		self->health = self->count;
	}
}

#define TRIGGER_HEAL_CANTHINK(self) self->count != -9999
void trigger_heal_setup(gentity_t *self) {
	self->target_ent = G_FindByTargetname(NULL, self->target);
	if (!self->target_ent) {
		G_Error("trigger_heal failed to find target: %s\n", self->target);
	}

	if (TRIGGER_HEAL_CANTHINK(self)) {
		self->think     = trigger_heal_think;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
==============
SP_misc_cabinet_health
==============
*/

/*QUAKED misc_cabinet_health (.5 .5 .5) (-20 -20 0) (20 20 60)
*/
void SP_misc_cabinet_health(gentity_t *self) {
	VectorSet(self->r.mins, -20, -20, 0);
	VectorSet(self->r.maxs, 20, 20, 60);

	G_SetOrigin(self, self->s.origin);
	G_SetAngle(self, self->s.angles);

	self->s.eType = ET_CABINET_H;

	self->clipmask   = CONTENTS_SOLID;
	self->r.contents = CONTENTS_SOLID;

	trap_LinkEntity(self);
}

/*
==============
SP_trigger_heal
==============
*/
void SP_trigger_heal(gentity_t *self) {

	char *spawnstr;
	int  healvalue;

	InitTrigger(self);

	self->touch = heal_touch;

	// healtotal specifies the maximum amount of health this trigger area restores
	G_SpawnString("healtotal", "0", &spawnstr);
	healvalue = atoi(spawnstr);
	// Gordon: -9999 means infinite now
	self->health = healvalue;
	if (self->health <= 0) {
		self->health = -9999;
	}
	self->count   = self->health;
	self->s.eType = ET_HEALER;

	self->target_ent = NULL;
	if (self->target && *self->target) {
		self->think     = trigger_heal_setup;
		self->nextthink = level.time + FRAMETIME;
	} else if (TRIGGER_HEAL_CANTHINK(self)) {
		self->think     = trigger_heal_think;
		self->nextthink = level.time + HEALTH_REGENTIME;
	}

	// healrate specifies the amount of healing per second
	G_SpawnString("healrate", "20", &spawnstr);
	healvalue    = atoi(spawnstr);
	self->damage = healvalue;   // store the rate of heal in damage
}
// END		xkan, 9/17/2002

// START	xkan, 9/17/2002
/*
==============================================================================

trigger_ammo

==============================================================================
*/

/*QUAKED trigger_ammo (.5 .5 .5) ?
Any entity that touches this will get additional ammo a specified rate up to a
specified maximum.

"ammorate"		rate of ammo clips per second. default 1. (whole number only)
"ammototal"		the maximum clips of ammo this trigger can add. if <= 0, it's unlimited.
                default 0 (whole numbers only)
"target"		cabinet that this entity is linked to
*/

qboolean G_IsAllowedAmmo(gentity_t *ent) {
	if (!ent || !ent->client) {
		return qfalse;
	}

	if (ent->health < 0) {
		return qfalse;
	}

	return qtrue;
}

void ammo_touch(gentity_t *self, gentity_t *other, trace_t *trace) {
	int       i, clientcount = 0;
	gentity_t *touchClients[MAX_CLIENTS];

	// Nico, silent GCC
	(void)trace;

	memset(touchClients, 0, sizeof (touchClients));

	if (other->client == NULL) {
		return;
	}

	// flags is for the last entity number that got ammo
	if (self->timestamp > level.time) {
		return;
	}
	self->timestamp = level.time + 1000;

	for (i = 0; i < level.numConnectedClients; ++i) {
		int j = level.sortedClients[i];

		if (trap_EntityContactCapsule(g_entities[j].r.absmin, g_entities[j].r.absmax, self) && G_IsAllowedAmmo(&g_entities[j])) {
			touchClients[clientcount] = &g_entities[j];
			clientcount++;
		}
	}

	if (clientcount == 0) {
		return;
	}
}

#define AMMO_REGENTIME 60000
void trigger_ammo_think(gentity_t *self) {
	self->nextthink = level.time + AMMO_REGENTIME;
	self->health   += self->damage;
	if (self->health > self->count) {
		self->health = self->count;
	}
}

#define TRIGGER_AMMO_CANTHINK(self) self->count != -9999
void trigger_ammo_setup(gentity_t *self) {
	self->target_ent = G_FindByTargetname(NULL, self->target);
	if (!self->target_ent) {
		G_Error("trigger_ammo failed to find target: %s\n", self->target);
	}

	if (TRIGGER_AMMO_CANTHINK(self)) {
		self->think     = trigger_ammo_think;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
==============
SP_misc_cabinet_supply
==============
*/
/*QUAKED misc_cabinet_supply (.5 .5 .5) (-20 -20 0) (20 20 60)
*/
void SP_misc_cabinet_supply(gentity_t *self) {
	VectorSet(self->r.mins, -20, -20, 0);
	VectorSet(self->r.maxs, 20, 20, 60);

	G_SetOrigin(self, self->s.origin);
	G_SetAngle(self, self->s.angles);

	self->s.eType = ET_CABINET_A;

	self->clipmask   = CONTENTS_SOLID;
	self->r.contents = CONTENTS_SOLID;

	trap_LinkEntity(self);
}

/*
==============
SP_trigger_ammo
==============
*/
void SP_trigger_ammo(gentity_t *self) {

	char *spawnstr;
	int  ammovalue;

	InitTrigger(self);

	self->touch = ammo_touch;

	// ammototal specifies the maximum amount of ammo this trigger contains
	G_SpawnString("ammototal", "0", &spawnstr);
	ammovalue = atoi(spawnstr);
	// Gordon: -9999 means infinite now
	self->health = ammovalue;
	if (self->health <= 0) {
		self->health = -9999;
	}
	self->count   = self->health;
	self->s.eType = ET_SUPPLIER;

	self->target_ent = NULL;
	if (self->target && *self->target) {
		self->think     = trigger_ammo_setup;
		self->nextthink = level.time + FRAMETIME;
	} else if (TRIGGER_AMMO_CANTHINK(self)) {
		self->think     = trigger_ammo_think;
		self->nextthink = level.time + AMMO_REGENTIME;
	}

	// ammorate specifies the amount of ammo added per second
	G_SpawnString("ammorate", "1", &spawnstr);
	ammovalue = atoi(spawnstr);
	// store the rate of ammo addition in damage
	self->damage = ammovalue;
}
// END		xkan, 9/17/2002

/*
==============================================================================

timer

==============================================================================
*/

/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
This should be renamed trigger_timer...
Repeatedly fires its targets.
Can be turned on or off by using.

"wait"			base time between triggering all targets, default is 1
"random"		wait variance, default is 0
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

*/
void func_timer_think(gentity_t *self) {
	G_UseTargets(self, self->activator);
	// set time before next firing
	self->nextthink = level.time + 1000 * (self->wait + crandom() * self->random);
}

void func_timer_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;

	self->activator = activator;

	// if on, turn it off
	if (self->nextthink) {
		self->nextthink = 0;
		return;
	}

	// turn it on
	func_timer_think(self);
}

void SP_func_timer(gentity_t *self) {
	G_SpawnFloat("random", "0", &self->random);
	G_SpawnFloat("wait", "1", &self->wait);

	self->use   = func_timer_use;
	self->think = func_timer_think;

	if (self->random >= self->wait) {
		self->random = self->wait - (FRAMETIME / 1000.f);   //Gordon div 1000 for milisecs...*cough*
		G_Printf("func_timer at %s has random >= wait\n", vtos(self->s.origin));
	}

	if (self->spawnflags & 1) {
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	self->r.svFlags = SVF_NOCLIENT;
}

//---- (SA) Wolf triggers

/*QUAKED trigger_once (.5 .5 .5) ? AI_Touch
Must be targeted at one or more entities.
Once triggered, this entity is destroyed
(you can actually do the same thing with trigger_multiple with a wait of -1)
*/
void SP_trigger_once(gentity_t *ent) {
	ent->wait  = -1;            // this will remove itself after one use
	ent->touch = Touch_Multi;
	ent->use   = Use_Multi;

	InitTrigger(ent);
	trap_LinkEntity(ent);
}

//---- end

/*QUAKED test_gas (0 0.5 0) (-4 -4 -4) (4 4 4)
*/
void SP_gas(gentity_t *self) {
	// Nico, silent GCC
	(void)self;
}

// DHM - Nerve :: Multiplayer triggers

#define RED_FLAG 1
#define BLUE_FLAG 2

void Touch_flagonly(gentity_t *ent, gentity_t *other, trace_t *trace) {
	gentity_t *tmp;

	// Nico, silent GCC
	(void)trace;

	if (!other->client) {
		return;
	}

	if (ent->spawnflags & RED_FLAG && other->client->ps.powerups[PW_REDFLAG]) {

		if (ent->spawnflags & 4) {
			other->client->ps.powerups[PW_REDFLAG] = 0;
			other->client->speedScale              = 0;
		}

		tmp         = ent->parent;
		ent->parent = other;

		G_Script_ScriptEvent(ent, "death", "");

		G_Script_ScriptEvent(&g_entities[other->client->flagParent], "trigger", "captured");

		ent->parent = tmp;

		// Removes itself
		ent->touch     = NULL;
		ent->nextthink = level.time + FRAMETIME;
		ent->think     = G_FreeEntity;
	} else if (ent->spawnflags & BLUE_FLAG && other->client->ps.powerups[PW_BLUEFLAG]) {

		if (ent->spawnflags & 4) {
			other->client->ps.powerups[PW_BLUEFLAG] = 0;
			other->client->speedScale               = 0;
		}

		tmp         = ent->parent;
		ent->parent = other;

		G_Script_ScriptEvent(ent, "death", "");

		G_Script_ScriptEvent(&g_entities[other->client->flagParent], "trigger", "captured");

		ent->parent = tmp;

		// Removes itself
		ent->touch     = NULL;
		ent->nextthink = level.time + FRAMETIME;
		ent->think     = G_FreeEntity;
	}
}

void Touch_flagonly_multiple(gentity_t *ent, gentity_t *other, trace_t *trace) {
	gentity_t *tmp;

	// Nico, silent GCC
	(void)trace;

	if (!other->client) {
		return;
	}

	if (ent->spawnflags & RED_FLAG && other->client->ps.powerups[PW_REDFLAG]) {

		other->client->ps.powerups[PW_REDFLAG] = 0;
		other->client->speedScale              = 0;

		tmp         = ent->parent;
		ent->parent = other;

		G_Script_ScriptEvent(ent, "death", "");

		G_Script_ScriptEvent(&g_entities[other->client->flagParent], "trigger", "captured");

		ent->parent = tmp;
	} else if (ent->spawnflags & BLUE_FLAG && other->client->ps.powerups[PW_BLUEFLAG]) {

		other->client->ps.powerups[PW_BLUEFLAG] = 0;
		other->client->speedScale               = 0;

		tmp         = ent->parent;
		ent->parent = other;

		G_Script_ScriptEvent(ent, "death", "");

		G_Script_ScriptEvent(&g_entities[other->client->flagParent], "trigger", "captured");

		ent->parent = tmp;
	}
}

/*QUAKED trigger_flagonly (.5 .5 .5) ? RED_FLAG BLUE_FLAG KILL_FLAG
Player must be carrying the proper flag for it to trigger.
It will call the "death" function in the object's script.

"scriptName"	The object name in the script file

RED_FLAG -- only trigger if player is carrying red flag
BLUE_FLAG -- only trigger if player is carrying blue flag
*/
void SP_trigger_flagonly(gentity_t *ent) {
	char *scorestring; // JPW NERVE

	ent->touch = Touch_flagonly;

	InitTrigger(ent);

	// JPW NERVE -- if this trigger has a "score" field set, then completing an objective
	//  inside of this field will add "score" to the right player team.  storing this
	//  in ent->accuracy since that's unused.
	G_SpawnString("score", "20", &scorestring);
	ent->accuracy   = atof(scorestring);
	ent->s.eType    = ET_TRIGGER_FLAGONLY;
	ent->r.svFlags &= ~SVF_NOCLIENT;

	trap_LinkEntity(ent);
}

/*QUAKED trigger_flagonly_multiple (.5 .5 .5) ? RED_FLAG BLUE_FLAG
Player must be carrying the proper flag for it to trigger.
It will call the "death" function in the object's script.

"scriptName"	The object name in the script file

RED_FLAG -- only trigger if player is carrying red flag
BLUE_FLAG -- only trigger if player is carrying blue flag
*/
void SP_trigger_flagonly_multiple(gentity_t *ent) {
	char *scorestring; // JPW NERVE

	ent->touch = Touch_flagonly_multiple;

	InitTrigger(ent);

	// JPW NERVE -- if this trigger has a "score" field set, then completing an objective
	//  inside of this field will add "score" to the right player team.  storing this
	//  in ent->accuracy since that's unused.
	G_SpawnString("score", "20", &scorestring);
	ent->accuracy   = atof(scorestring);
	ent->s.eType    = ET_TRIGGER_FLAGONLY_MULTIPLE;
	ent->r.svFlags &= ~SVF_NOCLIENT;

	trap_LinkEntity(ent);
}

// NERVE - SMF - spawn an explosive indicator
void explosive_indicator_think(gentity_t *ent) {
	gentity_t *parent;

	parent = &g_entities[ent->r.ownerNum];

	if (!parent->inuse || (parent->s.eType == ET_CONSTRUCTIBLE && !parent->r.linked)) {

		// update our map
		{
			mapEntityData_t *mEnt;

			if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL) {
				G_FreeMapEntityData(&mapEntityData[0], mEnt);
			}
			if ((mEnt = G_FindMapEntityData(&mapEntityData[1], ent - g_entities)) != NULL) {
				G_FreeMapEntityData(&mapEntityData[1], mEnt);
			}
		}

		G_FreeEntity(ent);
		return;
	}

	if (ent->s.eType == ET_TANK_INDICATOR || ent->s.eType == ET_TANK_INDICATOR_DEAD) {
		VectorCopy(ent->parent->r.currentOrigin, ent->s.pos.trBase);
	}
	ent->nextthink = level.time + FRAMETIME;

	if (parent->s.eType == ET_OID_TRIGGER && parent->target_ent) {
		ent->s.effect1Time = parent->target_ent->constructibleStats.weaponclass;
	} else {
		ent->s.effect1Time = parent->constructibleStats.weaponclass;
	}
}

// Arnout: spawn a constructible indicator
void constructible_indicator_think(gentity_t *ent) {
	gentity_t *parent        = &g_entities[ent->r.ownerNum];
	gentity_t *constructible = parent->target_ent;

	if (parent->chain && constructible->s.teamNum != ent->s.teamNum) {
		// use the target that has the same team as the indicator
		constructible = parent->chain;
	}

	// Arnout: why are we checking for the classname?
	if (!parent->inuse || !parent->r.linked || (constructible && constructible->s.angles2[1] != 0)) {
		// update our map
		{
			mapEntityData_t *mEnt;

			if (parent->spawnflags & 8) {
				if ((mEnt = G_FindMapEntityData(&mapEntityData[0], ent - g_entities)) != NULL) {
					G_FreeMapEntityData(&mapEntityData[0], mEnt);
				}
				if ((mEnt = G_FindMapEntityData(&mapEntityData[1], ent - g_entities)) != NULL) {
					G_FreeMapEntityData(&mapEntityData[1], mEnt);
				}
			} else {
				mapEntityData_Team_t *teamList = ent->s.teamNum == TEAM_AXIS ? &mapEntityData[0] : &mapEntityData[1];
				if ((mEnt = G_FindMapEntityData(teamList, ent - g_entities)) != NULL) {
					G_FreeMapEntityData(teamList, mEnt);
				}
			}
		}

		parent->count2 = 0;

		G_FreeEntity(ent);
		return;
	}

	if (ent->s.eType == ET_TANK_INDICATOR || ent->s.eType == ET_TANK_INDICATOR_DEAD) {
		VectorCopy(ent->parent->r.currentOrigin, ent->s.pos.trBase);
	}
	ent->s.effect1Time = parent->constructibleStats.weaponclass;
	ent->nextthink     = level.time + FRAMETIME;
}

void G_SetConfigStringValue(int num, const char *key, const char *value) {
	char cs[MAX_STRING_CHARS];

	trap_GetConfigstring(num, cs, sizeof (cs));
	Info_SetValueForKey(cs, key, value);
	trap_SetConfigstring(num, cs);
}

void Touch_ObjectiveInfo(gentity_t *ent, gentity_t *other, trace_t *trace) {
	// Nico, silent GCC
	(void)trace;

	if (!other->client) {
		return;
	}

	other->client->touchingTOI = ent;
}

// Arnout: links the trigger to it's objective, determining if it's a func_explosive
// of func_constructible and spawning the right indicator
void Think_SetupObjectiveInfo(gentity_t *ent) {

	ent->target_ent = G_FindByTargetname(NULL, ent->target);

	if (!ent->target_ent) {
		// Nico, removed G_Error here
		G_Printf("'trigger_objective_info' has a missing target '%s'\n", ent->target);
		return;
	}

	if (ent->target_ent->s.eType == ET_EXPLOSIVE) {
		// Arnout: this is for compass usage
		if ((ent->spawnflags & AXIS_OBJECTIVE) || (ent->spawnflags & ALLIED_OBJECTIVE)) {
			gentity_t *e;
			e = G_Spawn();

			e->r.svFlags = SVF_BROADCAST;
			e->classname = "explosive_indicator";
			if (ent->spawnflags & 8) {
				e->s.eType = ET_TANK_INDICATOR;
			} else {
				e->s.eType = ET_EXPLOSIVE_INDICATOR;
			}
			e->parent       = ent;
			e->s.pos.trType = TR_STATIONARY;

			if (ent->spawnflags & AXIS_OBJECTIVE) {
				e->s.teamNum = 1;
			} else if (ent->spawnflags & ALLIED_OBJECTIVE) {
				e->s.teamNum = 2;
			}

			G_SetOrigin(e, ent->r.currentOrigin);

			e->s.modelindex2 = ent->s.teamNum;
			e->r.ownerNum    = ent->s.number;
			e->think         = explosive_indicator_think;
			e->nextthink     = level.time + FRAMETIME;

			e->s.effect1Time = ent->target_ent->constructibleStats.weaponclass;

			if (ent->tagParent) {
				e->tagParent = ent->tagParent;
				Q_strncpyz(e->tagName, ent->tagName, MAX_QPATH);
			} else {
				VectorCopy(ent->r.absmin, e->s.pos.trBase);
				VectorAdd(ent->r.absmax, e->s.pos.trBase, e->s.pos.trBase);
				VectorScale(e->s.pos.trBase, 0.5, e->s.pos.trBase);
			}

			SnapVector(e->s.pos.trBase);

			trap_LinkEntity(e);

			ent->target_ent->parent = ent;
		}
	} else if (ent->target_ent->s.eType == ET_CONSTRUCTIBLE) {
		gentity_t *constructibles[2];
		int       team[2] = { 0 };

		ent->target_ent->parent = ent;

		constructibles[0] = ent->target_ent;
		constructibles[1] = G_FindByTargetname(constructibles[0], ent->target);     // see if we are targetting a 2nd one for two team constructibles

		team[0] = constructibles[0]->spawnflags & AXIS_CONSTRUCTIBLE ? TEAM_AXIS : TEAM_ALLIES;

		constructibles[0]->s.otherEntityNum2 = ent->s.teamNum;

		if (constructibles[1]) {
			team[1] = constructibles[1]->spawnflags & AXIS_CONSTRUCTIBLE ? TEAM_AXIS : TEAM_ALLIES;

			if (constructibles[1]->s.eType != ET_CONSTRUCTIBLE) {
				// Nico, removed G_Error here
				G_Printf("ERROR: 'trigger_objective_info' targets multiple entities with targetname '%s', the second one isn't a 'func_constructible'\n", ent->target);
				return;
			}

			if (team[0] == team[1]) {
				// Nico, removed G_Error here
				G_Printf("ERROR: 'trigger_objective_info' targets two 'func_constructible' entities with targetname '%s' that are constructible by the same team\n", ent->target);
				return;
			}

			constructibles[1]->s.otherEntityNum2 = ent->s.teamNum;

			ent->chain         = constructibles[1];
			ent->chain->parent = ent;

			constructibles[0]->chain = constructibles[1];
			constructibles[1]->chain = constructibles[0];
		} else {
			constructibles[0]->chain = NULL;
		}

		// if already constructed (in case of START_BUILT)
		if (constructibles[0]->s.angles2[1] == 0) {
			// Arnout: spawn a constructible icon - this is for compass usage
			gentity_t *e;
			e = G_Spawn();

			e->r.svFlags = SVF_BROADCAST;
			e->classname = "constructible_indicator";
			if (ent->spawnflags & 8) {
				e->s.eType = ET_TANK_INDICATOR_DEAD;
			} else {
				e->s.eType = ET_CONSTRUCTIBLE_INDICATOR;
			}
			e->s.pos.trType = TR_STATIONARY;

			if (constructibles[1]) {
				// see if one of the two is still partially built (happens when a multistage destructible construction blows up for the first time)
				if (constructibles[0]->count2 && constructibles[0]->grenadeFired > 1) {
					e->s.teamNum = team[0];
				} else if (constructibles[1]->count2 && constructibles[1]->grenadeFired > 1) {
					e->s.teamNum = team[1];
				} else {
					e->s.teamNum = 3;   // both teams
				}
			} else {
				e->s.teamNum = team[0];
			}

			e->s.modelindex2 = ent->s.teamNum;
			e->r.ownerNum    = ent->s.number;
			ent->count2      = (e - g_entities);
			e->think         = constructible_indicator_think;
			e->nextthink     = level.time + FRAMETIME;

			e->parent = ent;

			if (ent->tagParent) {
				e->tagParent = ent->tagParent;
				Q_strncpyz(e->tagName, ent->tagName, MAX_QPATH);
			} else {
				VectorCopy(ent->r.absmin, e->s.pos.trBase);
				VectorAdd(ent->r.absmax, e->s.pos.trBase, e->s.pos.trBase);
				VectorScale(e->s.pos.trBase, 0.5, e->s.pos.trBase);
			}

			SnapVector(e->s.pos.trBase);

			trap_LinkEntity(e);     // moved down
		}
		ent->touch = Touch_ObjectiveInfo;

	} else if (ent->target_ent->s.eType == ET_COMMANDMAP_MARKER) {
		ent->target_ent->parent = ent;
	}

	trap_LinkEntity(ent);
}

/*QUAKED trigger_objective_info (.5 .5 .5) ? AXIS_OBJECTIVE ALLIED_OBJECTIVE MESSAGE_OVERRIDE TANK IS_OBJECTIVE IS_HEALTHAMMOCABINET IS_COMMANDPOST
Players in this field will see a message saying that they are near an objective.

  "track"		Mandatory, this is the text that is appended to "You are near "
  "shortname"	Short name to show on command centre
*/
#define AXIS_OBJECTIVE      1
#define ALLIED_OBJECTIVE    2
#define MESSAGE_OVERRIDE    4
#define TANK                8

void SP_trigger_objective_info(gentity_t *ent) {
	char *scorestring;
	char *customimage;
	int  cix, cia, objflags;

	if (!ent->track) {
		G_Error("'trigger_objective_info' does not have a 'track' \n");
	}

	if ((ent->spawnflags & MESSAGE_OVERRIDE) && !ent->spawnitem) {
		G_Error("'trigger_objective_info' has override flag set but no override text\n");
	}

	// Gordon: for specifying which commandmap objectives this entity "belongs" to
	G_SpawnInt("objflags", "0", &objflags);

	if (G_SpawnString("customimage", "", &customimage)) {
		cix = cia = G_ShaderIndex(customimage);
	} else {
		if (G_SpawnString("customaxisimage", "", &customimage)) {
			cix = G_ShaderIndex(customimage);
		} else {
			cix = 0;
		}

		if (G_SpawnString("customalliesimage", "", &customimage)) {
			cia = G_ShaderIndex(customimage);
		} else if (G_SpawnString("customalliedimage", "", &customimage)) {
			cia = G_ShaderIndex(customimage);
		} else {
			cia = 0;
		}
	}

	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "e", va("%d", (int)(ent - g_entities)));
	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "o", va("%i", objflags));
	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "cix", va("%i", cix));
	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "cia", va("%i", cia));
	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "s", va("%i", ent->spawnflags));
	G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "n", ent->message ? ent->message : "");

	if (level.numOidTriggers >= MAX_OID_TRIGGERS) {
		G_Error("Exceeded maximum number of 'trigger_objective_info' entities\n");
	}

	// JPW NERVE -- if this trigger has a "score" field set, then blowing up an objective
	//  inside of this field will add "score" to the right player team.  storing this
	//  in ent->accuracy since that's unused.
	G_SpawnString("score", "0", &scorestring);
	ent->accuracy = atof(scorestring);

	trap_SetConfigstring(CS_OID_TRIGGERS + level.numOidTriggers, ent->track);

	InitTrigger(ent);

	if (ent->s.origin[0] || ent->s.origin[1] || ent->s.origin[2]) {
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "x", va("%i", (int)ent->s.origin[0]));
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "y", va("%i", (int)ent->s.origin[1]));
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "z", va("%i", (int)ent->s.origin[2]));
	} else {
		vec3_t mid;
		VectorAdd(ent->r.absmin, ent->r.absmax, mid);
		VectorScale(mid, 0.5f, mid);

		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "x", va("%i", (int)mid[0]));
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "y", va("%i", (int)mid[1]));
		G_SetConfigStringValue(CS_OID_DATA + level.numOidTriggers, "z", va("%i", (int)mid[2]));
	}

	ent->s.teamNum = level.numOidTriggers++;

	// unlike other triggers, we need to send this one to the client
	ent->r.svFlags &= ~SVF_NOCLIENT;
	ent->s.eType    = ET_OID_TRIGGER;

	if (!ent->target) {
		// no target - just link and go
		trap_LinkEntity(ent);
	} else {
		// Arnout: finalize spawing on fourth frame to allow for proper linking with targets
		ent->nextthink = level.time + (3 * FRAMETIME);
		ent->think     = Think_SetupObjectiveInfo;
	}
}
// dhm - end

// JPW NERVE -- field which is acted upon (cgame side) by screenshakes to drop dust particles
void trigger_concussive_touch(gentity_t *ent, gentity_t *other, trace_t *trace) {
	// Nico, silent GCC
	(void)ent;
	(void)other;
	(void)trace;
}

/*QUAKED trigger_concussive_dust (.5 .5 .5) ?
Allows client side prediction of teleportation events.
Must point at a target_position, which will be the teleport destination.
*/
void SP_trigger_concussive_dust(gentity_t *self) {
	InitTrigger(self);

	self->s.eType = ET_CONCUSSIVE_TRIGGER;
	self->touch   = trigger_concussive_touch;

	trap_LinkEntity(self);
}
// jpw

// Nico, velocity jumppads support

void trigger_push_velocity_touch(gentity_t *self, gentity_t *other, trace_t *trace) {
	// Nico, silent GCC
	(void)trace;

	// Nico, jumppads support
	if (!(g_enableMapEntities.integer & MAP_VELOCITY_JUMPPADS) || !other->client) {
		return;
	}

	BG_TouchVelocityJumpPad(&other->client->ps, &self->s, self->speed, self->count);
}

/*QUAKED trigger_push_velocity (.5 .5 .5) ? PLAYERDIR_XY ADD_XY PLAYERDIR_Z ADD_Z BIDIRECTIONAL_XY BIDIRECTIONAL_Z CLAMP_NEGATIVE_ADDS
This is used to create jump pads and launch ramps. It MUST point to a target_position or info_notnull entity to work.
Unlike target_push, this is client side predicted.  This is similar to a jumppad, however, it may be configured to add to the player's velocity, as opposed to just setting it.
-------- KEYS --------
target : this points to the target_position to which the player will jump.
notfree : when set to 1, entity will not spawn in "Free for all" and "Tournament" modes.
notteam : when set to 1, entity will not spawn in "Teamplay" and "CTF" modes.
notsingle : when set to 1, entity will not spawn in Single Player mode (bot play mode).
speed: XY speed for player-directional velocity pads - either sets or adds to the player's horizontal velocity.
count: Z speed for player-directional velocity pads - either sets or adds to the player's vectical velocity.
-------- SPAWNFLAGS --------
PLAYERDIR_XY: if set, trigger will apply the horizontal speed in the player's horizontal direction of travel, otherwise it uses the target XY component.
ADD_XY: if set, trigger will add to the player's horizontal velocity, otherwise it set's the player's horizontal velociy.
PLAYERDIR_Z: if set, trigger will apply the vertical speed in the player's vertical direction of travel, otherwise it uses the target Z component.
ADD_Z: if set, trigger will add to the player's vertical velocity, otherwise it set's the player's vectical velociy.
BIDIRECTIONAL_XY: if set, non-playerdir velocity pads will function in 2 directions based on the target specified.  The chosen direction is based on the current direction of travel.  Applies to horizontal direction.
BIDIRECTIONAL_Z: if set, non-playerdir velocity pads will function in 2 directions based on the target specified.  The chosen direction is based on the current direction of travel.  Applies to vertical direction.
CLAMP_NEGATIVE_ADDS: if set, then a velocity pad that adds negative velocity will be clamped to 0, if the resultant velocity would bounce the player in the opposite direction.
-------- NOTES --------
To make a jump pad or launch ramp, place the target_position/info_notnull entity at the highest point of the jump and target it with this entity.*/
void SP_trigger_push_velocity(gentity_t *self) {
	InitTrigger(self);

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;

	self->s.eType   = ET_PUSH_TRIGGER;
	self->touch     = trigger_push_velocity_touch;
	self->think     = AimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	trap_LinkEntity(self);
}
// Nico, end of velocity jumppads support
