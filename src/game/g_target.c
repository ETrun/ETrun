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
 * name:		g_target.c
 *
 * desc:
 *
*/

#include "g_local.h"
#include "g_api.h"

//==========================================================

/*QUAKED target_give (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_Give(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	gentity_t *t;
	trace_t   trace;

	// Nico, silent GCC
	(void)other;

	if (!activator->client) {
		return;
	}

	if (!ent->target) {
		return;
	}

	memset(&trace, 0, sizeof (trace));
	t = NULL;
	while ((t = G_FindByTargetname(t, ent->target)) != NULL) {
		if (!t->item) {
			continue;
		}
		Touch_Item(t, activator, &trace);

		// make sure it isn't going to respawn or show any events
		t->nextthink = 0;
		trap_UnlinkEntity(t);
	}
}

void SP_target_give(gentity_t *ent) {
	ent->use = Use_Target_Give;
}

//==========================================================

/*QUAKED target_remove_powerups (1 0 0) (-8 -8 -8) (8 8 8)
takes away all the activators powerups.
Used to drop flight powerups into death puts.
*/
void Use_target_remove_powerups(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)ent;
	(void)other;

	if (!activator->client) {
		return;
	}

	if (activator->client->ps.powerups[PW_REDFLAG] || activator->client->ps.powerups[PW_BLUEFLAG]) {
		Team_ReturnFlag(&g_entities[activator->client->flagParent]);
	}

	memset(activator->client->ps.powerups, 0, sizeof (activator->client->ps.powerups));
}

void SP_target_remove_powerups(gentity_t *ent) {
	ent->use = Use_target_remove_powerups;
}

//==========================================================

/*QUAKED target_delay (1 1 0) (-8 -8 -8) (8 8 8)
"wait" seconds to pause before firing targets.
"random" delay variance, total delay = delay +/- random seconds
*/
void Think_Target_Delay(gentity_t *ent) {
	G_UseTargets(ent, ent->activator);
}

void Use_Target_Delay(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;

	ent->nextthink = level.time + (ent->wait + ent->random * crandom()) * 1000;
	ent->think     = Think_Target_Delay;
	ent->activator = activator;
}

void SP_target_delay(gentity_t *ent) {
	// check delay for backwards compatability
	if (!G_SpawnFloat("delay", "0", &ent->wait)) {
		G_SpawnFloat("wait", "1", &ent->wait);
	}

	if (!ent->wait) {
		ent->wait = 1;
	}
	ent->use = Use_Target_Delay;
}

//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)ent;
	(void)other;
	(void)activator;
}

void SP_target_score(gentity_t *ent) {
	if (!ent->count) {
		ent->count = 1;
	}
	ent->use = Use_Target_Score;
}

//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) redteam blueteam private
"message"	text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void Use_Target_Print(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;

	if (ent->spawnflags & 4) {
		if (!activator) {
			G_Error("Use_Target_Print: call to client only target_print with no activator\n");
		}

		if (activator->client) {
			trap_SendServerCommand(activator - g_entities, va("cp \"%s\"", ent->message));
			return;
		}
	}

	if (ent->spawnflags & 3) {
		if (ent->spawnflags & 1) {
			G_TeamCommand(TEAM_AXIS, va("cp \"%s\"", ent->message));
		}
		if (ent->spawnflags & 2) {
			G_TeamCommand(TEAM_ALLIES, va("cp \"%s\"", ent->message));
		}
		return;
	}

	trap_SendServerCommand(-1, va("cp \"%s\"", ent->message));
}

void SP_target_print(gentity_t *ent) {
	ent->use = Use_Target_Print;
}

//==========================================================

/*QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) LOOPED_ON LOOPED_OFF GLOBAL ACTIVATOR VIS_MULTIPLE NO_PVS
"noise"		wav file to play

A global sound will play full volume throughout the level.
Activator sounds will play on the player that activated the target.
Global and activator sounds can't be combined with looping.
Normal sounds play each time the target is used.
Looped sounds will be toggled by use functions.
Multiple identical looping sounds will just increase volume without any speed cost.
NO_PVS - this sound will not turn off when not in the player's PVS
"wait" : Seconds between auto triggerings, 0 = don't auto trigger
"random" : wait variance, default is 0
"volume" volume control 255 is default
*/
void Use_Target_Speaker(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;

	if (ent->spawnflags & 3) {    // looping sound toggles
		if (ent->s.loopSound) {
			ent->s.loopSound = 0;   // turn it off
		} else {
			ent->s.loopSound = ent->noise_index;    // start it
		}
	} else {   // normal sound
		if (ent->spawnflags & 8) {
			G_AddEvent(activator, EV_GENERAL_SOUND_VOLUME, ent->noise_index);
		} else {
			G_AddEvent(ent, EV_GENERAL_SOUND_VOLUME, ent->noise_index);
		}
	}
}

void target_speaker_multiple(gentity_t *ent) {
	gentity_t *vis_dummy = NULL;

	if (!(ent->target)) {
		G_Error("target_speaker missing target at pos %s", vtos(ent->s.origin));
	}

	vis_dummy = G_FindByTargetname(NULL, ent->target);

	if (vis_dummy) {
		ent->s.otherEntityNum = vis_dummy->s.number;
	} else {
		G_Error("target_speaker cant find vis_dummy_multiple %s", vtos(ent->s.origin));
	}
}

void SP_target_speaker(gentity_t *ent) {
	char buffer[MAX_QPATH];
	char *s;

	G_SpawnFloat("wait", "0", &ent->wait);
	G_SpawnFloat("random", "0", &ent->random);

	if (!G_SpawnString("noise", "NOSOUND", &s)) {
		G_Error("target_speaker without a noise key at %s", vtos(ent->s.origin));
	}

	// force all client reletive sounds to be "activator" speakers that
	// play on the entity that activates it
	if (s[0] == '*') {
		ent->spawnflags |= 8;
	}

	Q_strncpyz(buffer, s, sizeof (buffer));
	ent->noise_index = G_SoundIndex(buffer);

	// a repeating speaker can be done completely client side
	ent->s.eType     = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame     = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;

	// check for prestarted looping sound
	if (ent->spawnflags & 1) {
		ent->s.loopSound = ent->noise_index;
	}

	ent->use = Use_Target_Speaker;

	// GLOBAL
	if (ent->spawnflags & (4 | 32)) {
		ent->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy(ent->s.origin, ent->s.pos.trBase);

	if (ent->spawnflags & 16) {
		ent->think     = target_speaker_multiple;
		ent->nextthink = level.time + 50;
	}

	// NO_PVS
	if (ent->spawnflags & 32) {
		ent->s.density = 1;
	} else {
		ent->s.density = 0;
	}

	if (ent->radius) {
		ent->s.dmgFlags = ent->radius;  // store radius in dmgflags
	} else {
		ent->s.dmgFlags = 0;
	}

	// Gordon: Volume control!, i want some cookies for this Tim! :o
	G_SpawnInt("volume", "255", &ent->s.onFireStart);
	if (!ent->s.onFireStart) {
		ent->s.onFireStart = 255;
	}

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity(ent);
}

/*QUAKED misc_beam (0 .5 .8) (-8 -8 -8) (8 8 8)
When on, displays a electric beam from target to target2.
"target"	start of beam
"target2"	end of beam
"shader"	the shader
"color"		colour of beam		*NOT WORKIN YET*
"scale"		width of beam		*NOT WORKIN YET*
*/

void misc_beam_think(gentity_t *self) {
	if (self->enemy) {
		if (self->enemy != self) {
			self->s.apos.trType     = self->enemy->s.pos.trType;
			self->s.apos.trTime     = self->enemy->s.pos.trTime;
			self->s.apos.trDuration = self->enemy->s.pos.trDuration;
			VectorCopy(self->enemy->s.pos.trBase, self->s.apos.trBase);
			VectorCopy(self->enemy->s.pos.trDelta, self->s.apos.trDelta);

			self->s.effect2Time = self->enemy->s.effect2Time;
		} else {
			self->s.apos.trType = TR_STATIONARY;
			VectorCopy(self->s.origin, self->s.apos.trBase);
		}
	}

	self->s.pos.trType     = self->target_ent->s.pos.trType;
	self->s.pos.trTime     = self->target_ent->s.pos.trTime;
	self->s.pos.trDuration = self->target_ent->s.pos.trDuration;
	VectorCopy(self->target_ent->s.pos.trBase, self->s.pos.trBase);
	VectorCopy(self->target_ent->s.pos.trDelta, self->s.pos.trDelta);

	self->s.effect1Time = self->target_ent->s.effect2Time;

	self->nextthink = level.time + FRAMETIME;

	if (self->s.pos.trType != TR_STATIONARY || self->s.apos.trType != TR_STATIONARY || !self->accuracy) {
		int i;

		self->accuracy = 1;

		self->r.contents = CONTENTS_SOLID;
		VectorCopy(self->s.pos.trBase, self->r.mins);
		VectorCopy(self->s.apos.trBase, self->r.maxs);

		for (i = 0; i < 3; ++i) {
			if (self->r.maxs[i] < self->r.mins[i]) {
				float bleh = self->r.mins[i];
				self->r.mins[i] = self->r.maxs[i];
				self->r.maxs[i] = bleh;
			}
		}

		self->r.mins[0] -= 4;
		self->r.mins[1] -= 4;
		self->r.mins[2] -= 4;
		self->r.maxs[0] += 4;
		self->r.maxs[1] += 4;
		self->r.maxs[2] += 4;

		VectorCopy(self->s.origin, self->r.currentOrigin);
		VectorSubtract(self->r.mins, self->r.currentOrigin, self->r.mins);
		VectorSubtract(self->r.maxs, self->r.currentOrigin, self->r.maxs);

		trap_LinkEntity(self);
	}
}

void misc_beam_start(gentity_t *self) {
	gentity_t *ent;

	self->s.eType = ET_BEAM_2;

	if (self->target) {
		ent = G_FindByTargetname(NULL, self->target);
		if (!ent) {
			G_Printf("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
			G_FreeEntity(self);
			return;
		}
		self->target_ent = ent;
	} else {
		G_Printf("%s at %s: with no target\n", self->classname, vtos(self->s.origin));
		G_FreeEntity(self);
		return;
	}

	if (self->message) {
		ent = G_FindByTargetname(NULL, self->message);
		if (!ent) {
			G_Printf("%s at %s: %s is a bad target2\n", self->classname, vtos(self->s.origin), self->message);
			G_FreeEntity(self);
			return; // No targets by this name.
		}
		self->enemy = ent;
	} else {   // the misc_beam is it's own ending point
		self->enemy = self;
	}

	self->accuracy  = 0;
	self->think     = misc_beam_think;
	self->nextthink = level.time + FRAMETIME;
}

void SP_misc_beam(gentity_t *self) {
	char *str;

	G_SpawnString("target2", "", &str);
	if (*str) {
		self->message = G_NewString(str);
	}

	G_SpawnString("shader", "lightningBolt", &str);
	if (*str) {
		self->s.modelindex2 = G_ShaderIndex(str);
	}

	G_SpawnInt("scale", "1", &self->s.torsoAnim);

	if (!G_SpawnVector("color", "1 1 1", self->s.angles2)) {
		G_DPrintf("Warning: SP_misc_beam does not have angles2\n");
	}

	// let everything else get spawned before we start firing
	self->accuracy  = 0;
	self->think     = misc_beam_start;
	self->nextthink = level.time + FRAMETIME;
}

//==========================================================

/*QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON
When triggered, fires a laser.  You can either set a target or a direction.
*/
void target_laser_think(gentity_t *self) {
	vec3_t  end;
	trace_t tr;

	// if pointed at another entity, set movedir to point at it
	if (self->enemy) {
		vec3_t point;

		VectorMA(self->enemy->s.origin, 0.5, self->enemy->r.mins, point);
		VectorMA(point, 0.5, self->enemy->r.maxs, point);
		VectorSubtract(point, self->s.origin, self->movedir);
		VectorNormalize(self->movedir);
	}

	// fire forward and see what we hit
	VectorMA(self->s.origin, 2048, self->movedir, end);

	trap_Trace(&tr, self->s.origin, NULL, NULL, end, self->s.number, CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE);

	if (tr.entityNum) {
		// hurt it if we can
		G_Damage(&g_entities[tr.entityNum], self, self->activator, self->movedir,
		         tr.endpos, self->damage, DAMAGE_NO_KNOCKBACK, MOD_TARGET_LASER);
	}

	VectorCopy(tr.endpos, self->s.origin2);

	trap_LinkEntity(self);
	self->nextthink = level.time + FRAMETIME;
}

void target_laser_on(gentity_t *self) {
	if (!self->activator) {
		self->activator = self;
	}
	target_laser_think(self);
}

void target_laser_off(gentity_t *self) {
	trap_UnlinkEntity(self);
	self->nextthink = 0;
}

void target_laser_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;

	self->activator = activator;
	if (self->nextthink > 0) {
		target_laser_off(self);
	} else {
		target_laser_on(self);
	}
}

void target_laser_start(gentity_t *self) {
	gentity_t *ent;

	self->s.eType = ET_BEAM;

	if (self->target) {
		ent = G_FindByTargetname(NULL, self->target);
		if (!ent) {
			G_Printf("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
		}
		self->enemy = ent;
	} else {
		G_SetMovedir(self->s.angles, self->movedir);
	}

	self->use   = target_laser_use;
	self->think = target_laser_think;

	if (!self->damage) {
		self->damage = 1;
	}

	if (self->spawnflags & 1) {
		target_laser_on(self);
	} else {
		target_laser_off(self);
	}
}

void SP_target_laser(gentity_t *self) {
	self->s.legsAnim = 1;

	self->s.angles2[0] = 1.f;
	self->s.angles2[1] = 1.f;
	self->s.angles2[2] = 1.f;

	// let everything else get spawned before we start firing
	self->think     = target_laser_start;
	self->nextthink = level.time + FRAMETIME;
}

//==========================================================

void target_teleporter_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	gentity_t *dest;

	// Nico, silent GCC
	(void)other;

	if (!activator->client) {
		return;
	}
	dest = G_PickTarget(self->target);
	if (!dest) {
		G_Printf("Couldn't find teleporter destination\n");
		return;
	}

	TeleportPlayer(activator, dest->s.origin, dest->s.angles);
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter(gentity_t *self) {
	if (!self->targetname) {
		G_Printf("untargeted %s at %s\n", self->classname, vtos(self->s.origin));
	}

	self->use = target_teleporter_use;
}

//==========================================================

/*QUAKED target_relay (1 1 0) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY RANDOM NOKEY_ONLY TAKE_KEY NO_LOCKED_NOISE
This doesn't perform any actions except fire its targets.
The activator can be forced to be from a certain team.
if RANDOM is checked, only one of the targets will be fired, not all of them
"key" specifies an item you can be carrying that affects the operation of this relay
this key is currently an int (1-16) which matches the id of a key entity (key_key1 = 1, etc)
NOKEY_ONLY means "fire only if I do /not/ have the specified key"
TAKE_KEY removes the key from the players inventory
"lockednoise" specifies a .wav file to play if the relay is used and the player doesn't have the necessary key.
By default this sound is "sound/movers/doors/default_door_locked.wav"
NO_LOCKED_NOISE specifies that it will be silent if activated without proper key
*/
void target_relay_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;

	if ((self->spawnflags & 1) && activator && activator->client
	    && activator->client->sess.sessionTeam != TEAM_AXIS) {
		return;
	}
	if ((self->spawnflags & 2) && activator && activator->client
	    && activator->client->sess.sessionTeam != TEAM_ALLIES) {
		return;
	}

	if (self->spawnflags & 4) {
		gentity_t *ent;

		ent = G_PickTarget(self->target);
		if (ent && ent->use) {
			G_UseEntity(ent, self, activator);
		}
		return;
	}

	// activator can be NULL if called from script
	if (activator && self->key && self->key == -1) {   // relay permanently locked
		if (self->soundPos1) {
			G_Sound(self, self->soundPos1);      //----(SA)	added
		}
		return;
	}

	G_UseTargets(self, activator);
}

/*
==============
SP_target_relay
==============
*/
void SP_target_relay(gentity_t *self) {
	char *sound;

	self->use = target_relay_use;

	if (!(self->spawnflags & 32)) {      // !NO_LOCKED_NOISE
		if (G_SpawnString("lockednoise", "0", &sound)) {
			self->soundPos1 = G_SoundIndex(sound);
		} else {
			self->soundPos1 = G_SoundIndex("sound/movers/doors/default_door_locked.wav");
		}
	}
}

//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8) kill_user_too
Kills the activator. (default)
If targets, they will be killed when this is fired
"kill_user_too" will still kill the activator when this ent has targets (default is only kill targets, not activator)
*/

void G_KillEnts(const char *target, gentity_t *ignore, gentity_t *killer) {
	gentity_t *targ = NULL;

	while ((targ = G_FindByTargetname(targ, target)) != NULL) {

		// make sure it isn't going to respawn or show any events
		targ->nextthink = 0;

		if (targ == ignore) {
			continue;
		}

		// RF, script_movers should die!
		if (targ->s.eType == ET_MOVER && !Q_stricmp(targ->classname, "script_mover") && targ->die) {
			G_Damage(targ, killer, killer, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
			continue;
		}

		if (targ->s.eType == ET_CONSTRUCTIBLE) {
			targ->die(targ, killer, killer, targ->health, 0);
			continue;
		}

		trap_UnlinkEntity(targ);
		targ->nextthink = level.time + FRAMETIME;

		targ->use   = NULL;
		targ->touch = NULL;
		targ->think = G_FreeEntity;
	}
}

void target_kill_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;

	// Nico, if kill triggers are enabled, kill activator (=player) too
	if (g_enableMapEntities.integer & MAP_FORCE_KILL_ENTITIES || self->spawnflags & 1) {  // kill usertoo
		G_Damage(activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG);
	}

	G_KillEnts(self->target, activator, self);
}

void SP_target_kill(gentity_t *self) {
	self->use = target_kill_use;
}

/*DEFUNCT target_position (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
*/
void SP_target_position(gentity_t *self) {
	G_SetOrigin(self, self->s.origin);
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
void SP_target_location(gentity_t *self) {

	// Nico, location jumppads support
	if (!(g_enableMapEntities.integer & MAP_LOCATION_JUMPPADS)) {
		G_FreeEntity(self);
	}
}

//---- (SA) Wolf targets

/*
==============
Use_Target_Counter
==============
*/
void Use_Target_Counter(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)activator;

	if (ent->count < 0) {   // if the count has already been hit, ignore this
		return;
	}

	ent->count -= 1;    // dec count

	if (!ent->count) {     // specified count is now hit
		G_UseTargets(ent, other);
	}
}

/*
==============
Use_Target_Lock
==============
*/
void Use_Target_Lock(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	gentity_t *t = 0;

	// Nico, silent GCC
	(void)other;
	(void)activator;

	while ((t = G_Find(t, FOFS(targetname), ent->target)) != NULL) {
		t->key = ent->key;
	}
}

//==========================================================

/*
==============
Use_target_fog
==============
*/
void Use_target_fog(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;
	(void)activator;

//	CS_FOGVARS reads:
//		near
//		far
//		density
//		r,g,b
//		time to complete
	trap_SetConfigstring(CS_FOGVARS, va("%f %f %f %f %f %f %i", 1.0f, (float)ent->s.density, 1.0f, (float)ent->dl_color[0], (float)ent->dl_color[1], (float)ent->dl_color[2], ent->s.time));
}

/*QUAKED target_fog (1 1 0) (-8 -8 -8) (8 8 8)
color picker chooses color of fog
"distance" sets fog distance.  Use value '0' to give control back to the game (and use the fog values specified in the sky shader if present)
"time" time it takes to change fog to new value.  default time is 1 sec
*/
void SP_target_fog(gentity_t *ent) {
	int   dist;
	float ftime;

	ent->use = Use_target_fog;

	// ent->s.density will carry the 'distance' value
	if (G_SpawnInt("distance", "0", &dist) && dist >= 0) {
		ent->s.density = dist;
	}

	// ent->s.time will carry the 'time' value
	if (G_SpawnFloat("time", "0.5", &ftime) && ftime >= 0) {
		ent->s.time = ftime * 1000; // sec to ms
	}
}

//==========================================================

/*QUAKED target_counter (1 1 0) (-8 -8 -8) (8 8 8)
Increments the counter pointed to.
"count" is the key for the count value
*/
void SP_target_counter(gentity_t *ent) {
	ent->use = Use_Target_Counter;
}

/*QUAKED target_autosave (1 1 0) (-8 -8 -8) (8 8 8)
saves game to 'autosave.sav' when triggered then dies.
*/
void SP_target_autosave(gentity_t *ent) {
	G_FreeEntity(ent);
}

//==========================================================

/*QUAKED target_lock (1 1 0) (-8 -8 -8) (8 8 8)
Sets the door to a state requiring key n
"key" is the required key
so:
key:0  unlocks the door
key:-1 locks the door until a target_lock with key:0
key:n  means the door now requires key n
*/
void SP_target_lock(gentity_t *ent) {
	ent->use = Use_Target_Lock;
}

void Use_Target_Alarm(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)activator;

	G_UseTargets(ent, other);
}

/*QUAKED target_alarm (1 1 0) (-4 -4 -4) (4 4 4)
does nothing yet (effectively a relay right now)
*/
void SP_target_alarm(gentity_t *ent) {
	ent->use = Use_Target_Alarm;
}

//---- end

/*QUAKED target_smoke (1 0 0) (-32 -32 -16) (32 32 16) Black White SmokeON Gravity
1 second	= 1000
1 FRAME		= 100
delay		= 100 = one millisecond default this is the maximum smoke that will show up
time		= 5000 default before the smoke disipates
duration	= 2000 before the smoke starts to alpha
start_size	= 24 default
end_size	= 96 default
wait		= default is 50 the rate at which it will travel up
shader		= custom shader to use for particles
*/

void smoke_think(gentity_t *ent) {
	ent->nextthink = level.time + ent->s.constantLight;

	if (!(ent->spawnflags & 4)) {
		return;
	}

	if (ent->s.dl_intensity) {
		ent->s.dl_intensity--;
		if (!ent->s.dl_intensity) {
			ent->think     = G_FreeEntity;
			ent->nextthink = level.time + FRAMETIME;
		}
	}
}

void smoke_toggle(gentity_t *ent, gentity_t *self, gentity_t *activator) {
	// Nico, silent GCC
	(void)self;
	(void)activator;

	if (ent->spawnflags & 4) {   // smoke is on turn it off
		ent->spawnflags &= ~4;
		trap_UnlinkEntity(ent);
	} else {
		ent->spawnflags |= 4;
		trap_LinkEntity(ent);
	}
}

void smoke_init(gentity_t *ent) {
	ent->think     = smoke_think;
	ent->nextthink = level.time + FRAMETIME;

	if (ent->target) {
		gentity_t *target;

		target = G_Find(NULL, FOFS(targetname), ent->target);
		if (target) {
			vec3_t vec;

			VectorSubtract(target->s.origin, ent->s.origin, vec);
			VectorCopy(vec, ent->s.origin2);
		} else {
			VectorSet(ent->s.origin2, 0, 0, 1);
		}
	} else {
		VectorSet(ent->s.origin2, 0, 0, 1);
	}

	if (ent->spawnflags & 4) {
		trap_LinkEntity(ent);
	}
}

void SP_target_smoke(gentity_t *ent) {
	char *buffer;

	if (G_SpawnString("shader", "", &buffer)) {
		ent->s.modelindex2 = G_ShaderIndex(buffer);
	} else {
		ent->s.modelindex2 = 0;
	}

	// Arnout - modified this a lot to be sent to the client as one entity and then is shown at the client
	if (!ent->delay) {
		ent->delay = 100;
	}

	ent->use = smoke_toggle;

	ent->think     = smoke_init;
	ent->nextthink = level.time + FRAMETIME;

	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_SMOKER;

	if (ent->spawnflags & 2) {
		ent->s.density = 4;
	} else {
		ent->s.density = 0;
	}

	// using "time"
	ent->s.time = ent->speed;
	if (!ent->s.time) {
		ent->s.time = 5000; // 5 seconds

	}
	ent->s.time2 = ent->duration;
	if (!ent->s.time2) {
		ent->s.time2 = 2000;
	}

	ent->s.angles2[0] = ent->start_size;
	if (!ent->s.angles2[0]) {
		ent->s.angles2[0] = 24;
	}

	ent->s.angles2[1] = ent->end_size;
	if (!ent->s.angles2[1]) {
		ent->s.angles2[1] = 96;
	}

	ent->s.angles2[2] = ent->wait;
	if (!ent->s.angles2[2]) {
		ent->s.angles2[2] = 50;
	}

	// idiot check
	if (ent->s.time < ent->s.time2) {
		ent->s.time = ent->s.time2 + 100;
	}

	if (ent->spawnflags & 8) {
		ent->s.frame = 1;
	}

	ent->s.dl_intensity  = ent->health;
	ent->s.constantLight = ent->delay;

	if (ent->spawnflags & 4) {
		trap_LinkEntity(ent);
	}
}

/*QUAKED target_script_trigger (1 .7 .2) (-8 -8 -8) (8 8 8)
must have an aiName
must have a target

when used it will fire its targets
*/
void target_script_trigger_use(gentity_t *ent, gentity_t *other, gentity_t *activator) {
// START	Mad Doctor I changes, 8/16/2002
	qboolean found = qfalse;

	// Nico, silent GCC
	(void)activator;

	// Are we using ainame to find another ent instead of using scriptname for this one?
	if (ent->aiName) {
		gentity_t *trent = NULL;

		// Find the first entity with this name
		trent = G_Find(trent, FOFS(scriptName), ent->aiName);

		// Was there one?
		if (trent) {
			// We found it
			found = qtrue;

			// Play the script
			G_Script_ScriptEvent(trent, "trigger", ent->target);

		} // if (trent)...

	} // if (ent->aiName)...

	// Use the old method if we didn't find an entity with the ainame
	if (!found && ent->scriptName) {
		G_Script_ScriptEvent(ent, "trigger", ent->target);
	}

	G_UseTargets(ent, other);
}

void SP_target_script_trigger(gentity_t *ent) {
	G_SetOrigin(ent, ent->s.origin);
	ent->r.svFlags = 0;
	ent->s.eType   = ET_GENERAL;
	ent->use       = target_script_trigger_use;
}

/*QUAKED target_rumble (0 0.75 0.8) (-8 -8 -8) (8 8 8) STARTOFF
wait = default is 2 seconds = time the entity will enable rumble effect
"pitch" value from 1 to 10 default is 5
"yaw"   value from 1 to 10 default is 5

"rampup" how much time it will take to reach maximum pitch and yaw in seconds
"rampdown" how long till effect ends after rampup is reached in seconds

"startnoise" startingsound
"noise"  the looping sound entity is to make
"endnoise" endsound

"duration" the amount of time the effect is to last ei 1.0 sec 3.6 sec
*/
int rumble_snd;

void target_rumble_think(gentity_t *ent) {
	float    ratio;
	float    dapitch, dayaw;
	qboolean validrumble = qtrue;

	if (!(ent->count)) {
		ent->timestamp = level.time;
		ent->count++;
		// start sound here
		if (ent->soundPos1) {
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos1);
		}
	} else {
		// looping sound
		ent->s.loopSound = ent->soundLoop;
	}

	dapitch = ent->delay;
	dayaw   = ent->random;
	ratio   = 1.0f;

	if (ent->start_size) {
		float time, time2;

		if (level.time < (ent->timestamp + ent->start_size)) {
			time  = level.time - ent->timestamp;
			time2 = (ent->timestamp + ent->start_size) - ent->timestamp;
			ratio = time / time2;
		} else if (level.time < (ent->timestamp + ent->end_size + ent->start_size)) {
			time  = level.time - ent->timestamp;
			time2 = (ent->timestamp + ent->start_size + ent->end_size) - ent->timestamp;
			ratio = time2 / time;
		} else {
			validrumble = qfalse;
		}
	}

	if (validrumble) {
		gentity_t *tent;

		tent              = G_TempEntity(ent->r.currentOrigin, EV_RUMBLE_EFX);
		tent->s.angles[0] = dapitch * ratio;
		tent->s.angles[1] = dayaw * ratio;
	}

	// end sound
	if (level.time > ent->duration + ent->timestamp) {
		if (ent->soundPos2) {
			G_AddEvent(ent, EV_GENERAL_SOUND, ent->soundPos2);
			ent->s.loopSound = 0;
		}

		ent->nextthink = 0;
	} else {
		ent->nextthink = level.time + 50;
	}
}

void target_rumble_use(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	// Nico, silent GCC
	(void)other;
	(void)activator;

	if (ent->spawnflags & 1) {
		ent->spawnflags &= ~1;
		ent->think       = target_rumble_think;
		ent->count       = 0;
		ent->nextthink   = level.time + 50;
	} else {
		ent->spawnflags |= 1;
		ent->think       = NULL;
		ent->count       = 0;
	}
}

void SP_target_rumble(gentity_t *self) {
	char  *pitch;
	char  *yaw;
	char  *rampup;
	char  *rampdown;
	float dapitch;
	float dayaw;
	char  *sound;
	char  *startsound;
	char  *endsound;

	if (G_SpawnString("noise", "", &sound)) {
		self->soundLoop = G_SoundIndex(sound);
	}

	if (G_SpawnString("startnoise", "", &startsound)) {
		self->soundPos1 = G_SoundIndex(startsound);
	}

	if (G_SpawnString("endnoise", "", &endsound)) {
		self->soundPos2 = G_SoundIndex(endsound);
	}

	self->use = target_rumble_use;

	G_SpawnString("pitch", "0", &pitch);
	dapitch     = atof(pitch);
	self->delay = dapitch;
	if (!(self->delay)) {
		self->delay = 5;
	}

	G_SpawnString("yaw", "0", &yaw);
	dayaw        = atof(yaw);
	self->random = dayaw;
	if (!(self->random)) {
		self->random = 5;
	}

	G_SpawnString("rampup", "0", &rampup);
	self->start_size = atoi(rampup) * 1000;
	if (!(self->start_size)) {
		self->start_size = 1000;
	}

	G_SpawnString("rampdown", "0", &rampdown);
	self->end_size = atoi(rampdown) * 1000;
	if (!(self->end_size)) {
		self->end_size = 1000;
	}

	if (!(self->duration)) {
		self->duration = 1000;
	} else {
		self->duration *= 1000;
	}

	trap_LinkEntity(self);
}

/**
 * Help function for target_starttimer, target_stoptimer, target_checkpoint.
 * Creates a new timerun if there isn't any timerun with such name.
 * source: TJMod
 */
int GetTimerunNum(char *name) {
	char **cur = level.timerunsNames;

	if (!name) {
		return 0;
	}

	while (*cur) {
		if (!Q_stricmp(*cur, name)) {
			return cur - level.timerunsNames;
		}
		cur++;
	}

	if (level.numTimeruns >= MAX_TIMERUNS) {
		G_Error("Exceeded maximum number of timeruns (max %i)\n", MAX_TIMERUNS);
	}
	level.timerunsNames[level.numTimeruns] = name;
	trap_SetConfigstring(CS_TIMERUNS + level.numTimeruns, name);
	return level.numTimeruns++;
}

// Nico, function used to notify the client his timerun has started and also the spectators of this client
static void notify_timerun_start(gentity_t *activator) {
	int timerunNum = activator->client->sess.currentTimerunNum;

	// Nico, notify the client itself first
	trap_SendServerCommand(activator - g_entities, va("timerun_start %i %i %i", timerunNum, activator->client->sess.timerunStartTime + 500, (int)activator->client->sess.startSpeed));

	// Nico, notify its spectators if cupmode is DISABLED
	if (g_cupMode.integer == 0) {
		int i;

		for (i = 0; i < level.numConnectedClients; ++i) {
			gentity_t *o = g_entities + level.sortedClients[i];

			if (!o->client) {
				continue;
			}

			if (o == activator) {
				continue;
			}

			if (o->client->sess.sessionTeam != TEAM_SPECTATOR) {
				continue;
			}

			if (o->client->sess.spectatorClient == activator - g_entities) {
				trap_SendServerCommand(o - g_entities, va("timerun_start_spec %i %i %i %i", timerunNum, activator->client->ps.clientNum, activator->client->sess.timerunStartTime + 500, (int)activator->client->sess.startSpeed));
			}
		}
	}
}

/* QUAKED target_startTimer (1 0 0) (-8 -8 -8) (8 8 8)
 * timer start
 *
 * "name"	timerun name
 */
void target_starttimer_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	gclient_t *client = activator->client;

	// Nico, silent GCC
	(void)other;

	if (client->sess.timerunActive) {
		return;
	}

	if ((self->spawnflags & 1) && VectorLength(client->ps.velocity) > 600) {
		// Server or clientside? cvar to toggle whether or not to reset speed in these cases?
		CPx(activator - g_entities, "cpm \"^1Timerun not started, no prejump allowed!\n\"");
		return;
	}

	if (client->ps.pm_type != PM_NORMAL || client->ps.stats[STAT_HEALTH] <= 0) {
		CPx(activator - g_entities, "cpm \"^1Timerun not started, invalid playerstate!\n\"");
		return;
	}

	client->sess.currentTimerun    = self->timerunName;
	client->sess.currentTimerunNum = GetTimerunNum(activator->client->sess.currentTimerun);

	// get the time
	client->sess.timerunStartTime = client->ps.commandTime;
	client->sess.startSpeed       = (int)sqrt(client->ps.velocity[0] * client->ps.velocity[0] + client->ps.velocity[1] * client->ps.velocity[1]);
	client->sess.timerunActive    = qtrue;

	// Nico, notify timerun_start event to client and to its spectators
	notify_timerun_start(activator);

	// reset checkpoints
	memset(client->sess.timerunCheckpointTimes, 0, sizeof (client->sess.timerunCheckpointTimes));
	client->sess.timerunCheckpointsPassed = 0;

	// Nico, reset max speed of the run
	client->sess.maxSpeed = 0;

	// Nico, reset jump counter (cause prediction error?)
	client->ps.identifyClientHealth = 0;

	// Nico, reset saves if physics is VET and strict save/load mod is DISABLED
	if (physics.integer == PHYSICS_MODE_VET && !g_strictSaveLoad.integer) {
		int i;

		for (i = 0; i < MAX_SAVED_POSITIONS; ++i) {
			client->sess.alliesSaves[i].valid = qfalse;
			client->sess.axisSaves[i].valid   = qfalse;
		}
	}
}

void SP_target_starttimer(gentity_t *ent) {
	char *t = NULL;

	// Nico, used to look for parent
	gentity_t *parent = NULL;

	// Nico, override wait -1 or wait 9999 on trigger_multiple where target is start timer
	if (g_forceTimerReset.integer) {
		parent = G_FindByTarget(NULL, ent->targetname);
		if (parent && parent->wait != 0.5 && !Q_stricmp(parent->classname, "trigger_multiple")) {
			G_DPrintf("%s: SP_target_starttimer, wait found = %f, overrided to 0.5\n", GAME_VERSION, parent->wait);
			G_SpawnFloat("wait", "0.5", &parent->wait);
		}
	}

	// Nico, create a timerun with this name if it doesn't exist yet
	G_SpawnString("name", "default", &t);
	ent->timerunName = G_NewString(t);
	GetTimerunNum(ent->timerunName);

	ent->use = target_starttimer_use;

	level.isTimerun = qtrue;
}

// Nico, function used to notify the client his timerun has stopped and also the spectators of this client
// note: it is called when client loads a position, or gets killed
void notify_timerun_stop(gentity_t *activator, int finishTime) {
	int timerunNum = activator->client->sess.currentTimerunNum;

	// Nico, check if timerun is active
	if (!activator->client->sess.timerunActive) {
		return;
	}

	// Nico, notify the client itself first
	trap_SendServerCommand(activator - g_entities, va("timerun_stop %i %i %i %i", timerunNum, finishTime, (int)activator->client->sess.stopSpeed, (int)activator->client->sess.maxSpeed));

	// Nico, notify its spectators if cupmode is DISABLED
	if (g_cupMode.integer == 0) {
		int i;

		for (i = 0; i < level.numConnectedClients; ++i) {
			gentity_t *o = g_entities + level.sortedClients[i];

			if (!o->client) {
				continue;
			}

			if (o == activator) {
				continue;
			}

			if (o->client->sess.sessionTeam != TEAM_SPECTATOR) {
				continue;
			}

			if (o->client->sess.spectatorClient == activator - g_entities) {
				trap_SendServerCommand(o - g_entities, va("timerun_stop_spec %i %i %i %i %i", timerunNum, activator->client->ps.clientNum, finishTime, (int)activator->client->sess.stopSpeed, (int)activator->client->sess.maxSpeed));
			}
		}
	}
}

// Nico, records command
static void Cmd_SendRecord_f(gentity_t *ent, char *runName, char *authToken, int time, int startSpeed, int stopSpeed, int maxSpeed, int jumpsCount, char *ip, int maxFPS,
                             unsigned int timenudge, unsigned int rate, unsigned int maxPackets, unsigned int snaps, int strictSaveLoad, int disableDrowning, int holdDoorsOpen, int enableMapEntities) {
	char *buf                    = NULL;
	char data[RESPONSE_MAX_SIZE] = { 0 };
	int  i                       = 0;
	char temp[MAX_QPATH]         = { 0 };

	buf = malloc(RESPONSE_MAX_SIZE * sizeof (char));

	if (!buf) {
		G_Error("Cmd_SendRecord_f: malloc failed\n");
	}

	sprintf(data, "%d/%d/%d/%d/%d/%s/%d/%u/%u/%u/%u/%d/%d/%d/%d/O", time, startSpeed, stopSpeed, maxSpeed, jumpsCount, ip, maxFPS, timenudge, rate, maxPackets, snaps, strictSaveLoad, disableDrowning, holdDoorsOpen, enableMapEntities);

	while (i < MAX_TIMERUN_CHECKPOINTS && ent->client->sess.timerunCheckpointTimes[i] != 0) {
		sprintf(temp, "%dO", ent->client->sess.timerunCheckpointTimes[i]);
		Q_strcat(data, sizeof (data), temp);
		i++;
	}

	// Check authtoken emptiness
	if (authToken[0] == '\0') {
		Q_strncpyz(authToken, "undefined", MAX_QPATH);
	}

	// Do we send a cup record?
	if (g_cupMode.integer) {
		if (!G_API_sendEventRecord(buf, ent, level.rawmapname, runName, authToken, data, GAME_VERSION_DATED)) {
			CP(va("print \"%s^w: error while sending an event record!\n\"", GAME_VERSION_COLORED));
		}
	} else {
		if (!G_API_sendRecord(buf, ent, level.rawmapname, runName, authToken, data, GAME_VERSION_DATED)) {
			CP(va("print \"%s^w: error while sending record!\n\"", GAME_VERSION_COLORED));
		}
	}
	// Do *not* free buf here
}

// Nico, save demo
void saveDemo(gentity_t *ent) {
	char cleanRunName[256] = { 0 };
	char physicsName[MAX_QPATH] = { 0 };
	int  i = 0;
	int  len = 0;
	int  time, min, sec, milli;

	// Nico, save run after replacing spaces in run name (in any) by underscores
	Q_strncpyz(cleanRunName, ent->client->sess.currentTimerun, sizeof (cleanRunName));

	Q_CleanStr(cleanRunName); // Nico, strip color tags

	len = strlen(cleanRunName);
	for (i = 0; i < len; ++i) {
		// Nico, only alphanumeric characters, the rest is replaced by an '_'
		if (!isalnum(cleanRunName[i])) {
			cleanRunName[i] = '_';
		}
	}

	getPhysicsName(physicsName, physics.integer);

	time = ent->client->sess.timerunLastTime[ent->client->sess.currentTimerunNum];

	// convert time into MM:SS:mmm
	milli  = time;
	min    = milli / 60000;
	milli -= min * 60000;
	sec    = milli / 1000;
	milli -= sec * 1000;

	trap_SendServerCommand(ent - g_entities, va("runSave %s[%s]_%02d-%02d-%03d", cleanRunName, physicsName, min, sec, milli));

	// Start recording a new temp demo.
	trap_SendServerCommand(ent - g_entities, "tempDemoStart 1");
}

/* QUAKED target_stopTimer (1 0 0) (-8 -8 -8) (8 8 8)
 * timer stop
 *
 * "name"				timerun name
 * "minCheckpoints"		minimal passed checkpoints to activate this stoptimer
 */
void target_stoptimer_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	int       time;
	gclient_t *client = activator->client;
	int       timerunNum;

	// Nico, silent GCC
	(void)other;

	if (!client->sess.timerunActive) {
		return;
	}

	// don't stop the time if this isn't a corresponding stoptimer
	if (Q_stricmp(self->timerunName, client->sess.currentTimerun)) {
		return;
	}

	timerunNum = client->sess.currentTimerunNum;

	// required number of checkpoints passed?
	if (client->sess.timerunCheckpointsPassed < self->count) {
		CPx(activator - g_entities, va("cpm \"^d%s^f:^1 Minimum checkpoints not passed (%d/%d)\n\"", client->sess.currentTimerun, client->sess.timerunCheckpointsPassed, self->count));
		notify_timerun_stop(activator, 0);
		client->sess.timerunActive = qfalse;

		return;
	}

	time = client->sess.timerunLastTime[timerunNum] = client->ps.commandTime - client->sess.timerunStartTime;

	if (!client->sess.timerunBestTime[timerunNum] || time < client->sess.timerunBestTime[timerunNum]) {
		// best personal for this session
		if (client->sess.logged) {
			client->sess.timerunBestTime[timerunNum] = time;

			// Nico, update best speed of run
			client->sess.timerunBestSpeed[timerunNum] = client->sess.maxSpeed;

			// Nico, set score so that xfire can see it (only if cup mode is DISABLED)
			if (g_cupMode.integer != 1) {
				client->ps.persistant[PERS_SCORE] = client->sess.timerunLastTime[timerunNum];
			}
		}

		// CP are updated here if API is not used or if CP were note loaded
		if (!g_useAPI.integer || client->sess.timerunCheckpointWereLoaded[timerunNum] == 0) {
			memcpy(client->sess.timerunBestCheckpointTimes[timerunNum], client->sess.timerunCheckpointTimes, sizeof (client->sess.timerunCheckpointTimes));
		}
	}

	// Nico, stop speed
	client->sess.stopSpeed = (int)sqrt(client->ps.velocity[0] * client->ps.velocity[0] + client->ps.velocity[1] * client->ps.velocity[1]);

	// Nico, send record if needed
	if (g_useAPI.integer && client->sess.logged) {
		Cmd_SendRecord_f(activator, client->sess.currentTimerun, client->pers.authToken,
		                 time, client->sess.startSpeed, client->sess.stopSpeed, client->sess.maxSpeed,
		                 client->ps.identifyClientHealth, // Nico, this is used as a jumps counter
		                 client->pers.ip, client->pers.maxFPS,
		                 client->pers.clientTimeNudge, client->pers.rate, client->pers.clientMaxPackets, client->pers.snaps,
		                 g_strictSaveLoad.integer, g_disableDrowning.integer, g_holdDoorsOpen.integer, g_enableMapEntities.integer);
	} else {
		// Nico, API is not used and/or client is not logged,
		// we cannnot know if his last time his SB/PB or something
		// else. So we keep his last demo.
		saveDemo(activator);
	}

	// Nico, notify the client and its spectators the timerun has stopped
	notify_timerun_stop(activator, client->sess.timerunLastTime[timerunNum]);

	client->sess.timerunActive = qfalse;
}

void SP_target_stoptimer(gentity_t *ent) {
	char *t = NULL;

	// Nico, used to look for parent
	gentity_t *parent = NULL;

	// Nico, override wait -1 or wait 9999 on stop timer entities
	if (g_forceTimerReset.integer) {
		parent = G_FindByTarget(NULL, ent->targetname);
		if (parent && parent->wait != 0.5 && !Q_stricmp(parent->classname, "trigger_multiple")) {
			G_DPrintf("%s: SP_target_stoptimer, wait found = %f, overrided to 0.5\n", GAME_VERSION, parent->wait);
			G_SpawnFloat("wait", "0.5", &parent->wait);
		}
	}

	G_SpawnString("name", "default", &t);
	ent->timerunName = G_NewString(t);
	// create a timerun with this name if it doesn't exit yet
	GetTimerunNum(ent->timerunName);

	G_SpawnInt("mincheckpoints", "0", &ent->count);

	ent->use = target_stoptimer_use;

	level.isTimerun = qtrue;
}

// Nico, function used to notify the client he has reached a check point and also the spectators of this client
static void notify_timerun_check(gentity_t *activator, int deltaTime, int time, int status) {
	// Nico, check if timerun is active
	if (!activator->client->sess.timerunActive) {
		return;
	}

	// Nico, notify the client itself first
	trap_SendServerCommand(activator - g_entities, va("timerun_check %i %i %i", deltaTime, time, status));

	// Nico, notify its spectators if cupmode is DISABLED
	if (g_cupMode.integer == 0) {
		int i;

		for (i = 0; i < level.numConnectedClients; ++i) {
			gentity_t *o = g_entities + level.sortedClients[i];

			if (!o->client) {
				continue;
			}

			if (o == activator) {
				continue;
			}

			if (o->client->sess.sessionTeam != TEAM_SPECTATOR) {
				continue;
			}

			if (o->client->sess.spectatorClient == activator - g_entities) {
				trap_SendServerCommand(o - g_entities, va("timerun_check_spec %i %i %i", deltaTime, time, status));
			}
		}
	}
}

/* QUAKED target_checkpoint (1 0 0) (-8 -8 -8) (8 8 8)
 * checkpoint
 *
 * "name"  timerun name
 */
void target_checkpoint_use(gentity_t *self, gentity_t *other, gentity_t *activator) {
	int       delta      = 0;
	int       time       = 0;
	gclient_t *client    = activator->client;
	int       timerunNum = 0;
	int       status     = 0;

	// Nico, silent GCC
	(void)other;

	if (!client->sess.timerunActive) {
		return;
	}

	// make sure this is a checkpoint for a current timerun
	if (Q_stricmp(self->timerunName, client->sess.currentTimerun)) {
		return;
	}

	timerunNum = client->sess.currentTimerunNum;

	// Nico, test if the checkpoint was already used
	if (client->sess.timerunCheckpointTimes[self->count]) {
		return;
	}

	client->sess.timerunCheckpointsPassed++;

	time = client->sess.timerunCheckpointTimes[self->count] = client->ps.commandTime - client->sess.timerunStartTime;

	if (client->sess.logged && !client->sess.timerunBestTime[timerunNum] && !client->sess.timerunCheckpointWereLoaded[timerunNum]) {
		status = 0;
	} else if (!client->sess.timerunBestCheckpointTimes[timerunNum][self->count] || time == client->sess.timerunBestCheckpointTimes[timerunNum][self->count]) {
		status = 1;
	} else if (time <= client->sess.timerunBestCheckpointTimes[timerunNum][self->count]) {
		status = 2;
	} else {
		status = 3;
	}

	delta = abs(time - client->sess.timerunBestCheckpointTimes[timerunNum][self->count]);

	notify_timerun_check(activator, delta, time, status);
}

void SP_target_checkpoint(gentity_t *ent) {
	char *t         = NULL;
	int  timerunNum = 0;

	// Nico, used to look for parent
	gentity_t *parent = NULL;

	// Nico, override wait -1 or wait 9999 on timer check entities
	if (g_forceTimerReset.integer) {
		parent = G_FindByTarget(NULL, ent->targetname);
		if (parent && parent->wait != 0.5 && !Q_stricmp(parent->classname, "trigger_multiple")) {
			G_DPrintf("%s: SP_target_checkpoint, wait found = %f, overrided to 0.5\n", GAME_VERSION, parent->wait);
			G_SpawnFloat("wait", "0.5", &parent->wait);
		}
	}

	G_SpawnString("name", "default", &t);
	ent->timerunName = G_NewString(t);
	// create a timerun with this name if it doesn't exit yet
	timerunNum = GetTimerunNum(ent->timerunName);

	if (level.numCheckpoints[timerunNum] >= MAX_TIMERUN_CHECKPOINTS) {
		G_Error("Exceeded maximum number of 'target_checkpoint' entities in '%s' timerun (max %i)\n", ent->timerunName, MAX_TIMERUN_CHECKPOINTS);
		return;
	}

	ent->count = level.numCheckpoints[timerunNum];
	ent->use   = target_checkpoint_use;

	level.numCheckpoints[timerunNum]++;

	level.isTimerun = qtrue;
}

void SP_rocketrun(gentity_t *ent) {
	int count;

	// Nico, silent GCC
	(void)ent;

	// Don't add if already added from one?
	if (!level.rocketRun) {
		G_SpawnInt("count", "1", &count);

		level.rocketRun = count;
	}
}
