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



// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate


#ifdef CGAMEDLL
	#include "../cgame/cg_local.h"
#else
	#include "q_shared.h"
	#include "bg_public.h"
#endif // CGAMEDLL

#include "bg_local.h"

pmove_t *pm;
pml_t   pml;

// movement parameters
float pm_stopspeed = 100;

//----(SA)	modified
float pm_waterSwimScale = 0.5;
float pm_waterWadeScale = 0.70;
float pm_slagSwimScale  = 0.30;
float pm_slagWadeScale  = 0.70;

float pm_proneSpeedScale = 0.21;    // was: 0.18 (too slow) then: 0.24 (too fast)

float pm_accelerate      = 10;
float pm_airaccelerate   = 1;
float pm_wateraccelerate = 4;
float pm_slagaccelerate  = 2;
float pm_flyaccelerate   = 8;

float pm_friction          = 6;
float pm_waterfriction     = 1;
float pm_slagfriction      = 1;
float pm_flightfriction    = 3;
float pm_ladderfriction    = 14;
float pm_spectatorfriction = 5.0f;
//----(SA)	end

// Nico, from Racesow
const float pm_aircontrol        = 150.0f; // aircontrol multiplier (intertia velocity to forward velocity conversion), default: 150
const float pm_strafeaccelerate  = 100; // forward acceleration when strafe bunny hopping, default: 70
const float pm_wishspeed         = 30; // Nico, default value 30
const float pm_airstopaccelerate = 2.0f; // Nico, CPM value: 2.5f, Racesow value: 2.0f
const float pm_slickaccelerate   = 10.0f; // Nico, slick control accelerate
const float pm_accelerate_AP     = 15; // Nico, only used for AP

int c_pmove = 0;

#ifdef GAMEDLL

// In just the GAME DLL, we want to store the groundtrace surface stuff,
// so we don't have to keep tracing.
void ClientStoreSurfaceFlags(int clientNum, int surfaceFlags);

#endif


/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent(int newEvent)
{
	BG_AddPredictableEventToPlayerstate(newEvent, 0, pm->ps);
}

void PM_AddEventExt(int newEvent, int eventParm)
{
	BG_AddPredictableEventToPlayerstate(newEvent, eventParm, pm->ps);
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt(int entityNum)
{
	int i;

	if (entityNum == ENTITYNUM_WORLD)
	{
		return;
	}
	if (pm->numtouch == MAXTOUCH)
	{
		return;
	}

	// see if it is already added
	for (i = 0 ; i < pm->numtouch ; i++)
	{
		if (pm->touchents[i] == entityNum)
		{
			return;
		}
	}

	// add it
	pm->touchents[pm->numtouch] = entityNum;
	pm->numtouch++;
}

/*
==================
PM_TraceAll

finds worst trace of body/legs, for collision.
==================
*/

void PM_TraceLegs(trace_t *trace, float *legsOffset, vec3_t start, vec3_t end, trace_t *bodytrace, vec3_t viewangles, void(tracefunc) (trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int ignoreent, int tracemask)
{
	trace_t steptrace;
	vec3_t  ofs, org, point;
	vec3_t  flatforward;
	float   angle;

	// zinx - don't let players block legs
	tracemask &= ~(CONTENTS_BODY | CONTENTS_CORPSE);

	if (legsOffset)
	{
		*legsOffset = 0;
	}

	angle          = DEG2RAD(viewangles[YAW]);
	flatforward[0] = cos(angle);
	flatforward[1] = sin(angle);
	flatforward[2] = 0;

	VectorScale(flatforward, -32, ofs);

	VectorAdd(start, ofs, org);
	VectorAdd(end, ofs, point);
	tracefunc(trace, org, playerlegsProneMins, playerlegsProneMaxs, point, ignoreent, tracemask);
	if (!bodytrace || trace->fraction < bodytrace->fraction ||
	    trace->allsolid)
	{
		// legs are clipping sooner than body
		// see if our legs can step up

		// give it a try with the new height
		ofs[2] += STEPSIZE;

		VectorAdd(start, ofs, org);
		VectorAdd(end, ofs, point);
		tracefunc(&steptrace, org, playerlegsProneMins, playerlegsProneMaxs, point, ignoreent, tracemask);
		if (!steptrace.allsolid && !steptrace.startsolid &&
		    steptrace.fraction > trace->fraction)
		{
			// the step trace did better -- use it instead
			*trace = steptrace;

			// get legs offset
			if (legsOffset)
			{
				*legsOffset = ofs[2];

				VectorCopy(steptrace.endpos, org);
				VectorCopy(steptrace.endpos, point);
				point[2] -= STEPSIZE;

				tracefunc(&steptrace, org, playerlegsProneMins, playerlegsProneMaxs, point, ignoreent, tracemask);
				if (!steptrace.allsolid)
				{
					*legsOffset = ofs[2] - (org[2] - steptrace.endpos[2]);
				}
			}
		}
	}
}

/* Traces all player bboxes -- body and legs */
void    PM_TraceAllLegs(trace_t *trace, float *legsOffset, vec3_t start, vec3_t end)
{
	pm->trace(trace, start, pm->mins, pm->maxs, end, pm->ps->clientNum, pm->tracemask);

	/* legs */
	if (pm->ps->eFlags & EF_PRONE)
	{
		trace_t legtrace;

		PM_TraceLegs(&legtrace, legsOffset, start, end, trace, pm->ps->viewangles, pm->trace, pm->ps->clientNum, pm->tracemask);

		if (legtrace.fraction < trace->fraction ||
		    legtrace.startsolid ||
		    legtrace.allsolid)
		{
			VectorSubtract(end, start, legtrace.endpos);
			VectorMA(start, legtrace.fraction, legtrace.endpos, legtrace.endpos);
			*trace = legtrace;
		}
	}
}

void PM_TraceAll(trace_t *trace, vec3_t start, vec3_t end)
{
	PM_TraceAllLegs(trace, NULL, start, end);
}

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction(void)
{
	vec3_t vec;
	float  *vel;
	float  speed, newspeed, control;
	float  drop;

	vel = pm->ps->velocity;

	VectorCopy(vel, vec);
	if (pml.walking)
	{
		vec[2] = 0; // ignore slope movement
	}

	speed = VectorLength(vec);
	// rain - #179 don't do this for PM_SPECTATOR/PM_NOCLIP, we always want them to stop
	if (speed < 1 && pm->ps->pm_type != PM_SPECTATOR && pm->ps->pm_type != PM_NOCLIP)
	{
		vel[0] = 0;
		vel[1] = 0;     // allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply end of dodge friction
	if (pm->cmd.serverTime < 350 &&
	    pm->cmd.serverTime > 250)
	{
		drop += speed * 20 * pml.frametime;
	}

	// apply ground friction
	if (pm->waterlevel <= 1)
	{
		if (pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK))
		{
			// if getting knocked back, no friction
			if (!(pm->ps->pm_flags & PMF_TIME_KNOCKBACK))
			{
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop   += control * pm_friction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if (pm->waterlevel)
	{
		if (pm->watertype == CONTENTS_SLIME)     //----(SA)	slag
		{
			drop += speed * pm_slagfriction * pm->waterlevel * pml.frametime;
		}
		else
		{
			drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
		}
	}

	if (pm->ps->pm_type == PM_SPECTATOR)
	{
		drop += speed * pm_spectatorfriction * pml.frametime;
	}

	// apply ladder strafe friction
	if (pml.ladder)
	{
		drop += speed * pm_ladderfriction * pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
	{
		newspeed = 0;
	}
	newspeed /= speed;

	// rain - if we're barely moving and barely slowing down, we want to
	// help things along--we don't want to end up getting snapped back to
	// our previous speed
	if (pm->ps->pm_type == PM_SPECTATOR || pm->ps->pm_type == PM_NOCLIP)
	{
		if (drop < 1.0f && speed < 3.0f)
		{
			newspeed = 0.0;
		}
	}

	// rain - used VectorScale instead of multiplying by hand
	VectorScale(vel, newspeed, vel);
}


/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate(vec3_t wishdir, float wishspeed, float accel)
{
	// q2 style
	int   i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct(pm->ps->velocity, wishdir);
	addspeed     = wishspeed - currentspeed;
	if (addspeed <= 0)
	{
		return;
	}
	accelspeed = accel * pml.frametime * wishspeed;
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	// Ridah, variable friction for AI's
	if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		accelspeed *= (1.0 / pm->ps->friction);
	}
	if (accelspeed > addspeed)
	{
		accelspeed = addspeed;
	}

	for (i = 0 ; i < 3 ; i++)
	{
		pm->ps->velocity[i] += accelspeed * wishdir[i];
	}
}

/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale(usercmd_t *cmd, qboolean horizontalOnly)
{
	int         max;
	float       total;
	float       scale;
	signed char upmove = 0;

	// Nico, ignore horizontalOnly flag if we use the original physics (from TJMod)
	if (!(pm->physics & PHYSICS_UPMOVE_BUG_FIX))
	{
		horizontalOnly = qfalse;
	}

	if (horizontalOnly)
	{
		upmove      = cmd->upmove;
		cmd->upmove = 0;
	}

	max = abs(cmd->forwardmove);
	if (abs(cmd->rightmove) > max)
	{
		max = abs(cmd->rightmove);
	}
	if (abs(cmd->upmove) > max)
	{
		max = abs(cmd->upmove);
	}
	if (!max)
	{
		return 0;
	}

	total = sqrt(cmd->forwardmove * cmd->forwardmove
	             + cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove);
	scale = (float)pm->ps->speed * max / (127.0 * total);

	if (pm->cmd.buttons & BUTTON_SPRINT)
	{
		scale *= pm->ps->sprintSpeedScale;
	}
	else
	{
		scale *= pm->ps->runSpeedScale;
	}

	if (pm->ps->pm_type == PM_NOCLIP)
	{
		scale *= 3;
	}

// JPW NERVE -- half move speed if heavy weapon is carried
// this is the counterstrike way of doing it -- ie you can switch to a non-heavy weapon and move at
// full speed.  not completely realistic (well, sure, you can run faster with the weapon strapped to your
// back than in carry position) but more fun to play.  If it doesn't play well this way we'll bog down the
// player if the own the weapon at all.
//
	if ((pm->ps->weapon == WP_PANZERFAUST) ||
	    (pm->ps->weapon == WP_MOBILE_MG42) ||
	    (pm->ps->weapon == WP_MOBILE_MG42_SET) ||
	    (pm->ps->weapon == WP_MORTAR))
	{

		scale *= 0.5;
	}

	if (pm->ps->weapon == WP_FLAMETHROWER)     // trying some different balance for the FT
	{
		if (pm->cmd.buttons & BUTTON_ATTACK)
		{
			scale *= 0.7;
		}
	}

	if (horizontalOnly)
	{
		cmd->upmove = upmove;
	}

	return scale;
}


/*
================
PM_SetMovementDir

Determine the rotation of the legs reletive
to the facing dir
================
*/
static void PM_SetMovementDir(void)
{
// Ridah, changed this for more realistic angles (at the cost of more network traffic?)
	float  speed;
	vec3_t moved;
	int    moveyaw;

	VectorSubtract(pm->ps->origin, pml.previous_origin, moved);

	if ((pm->cmd.forwardmove || pm->cmd.rightmove)
	    &&  (pm->ps->groundEntityNum != ENTITYNUM_NONE)
	    &&  (speed = VectorLength(moved))
	    &&  (speed > pml.frametime * 5))          // if moving slower than 20 units per second, just face head angles
	{
		vec3_t dir;

		VectorNormalize2(moved, dir);
		vectoangles(dir, dir);

		moveyaw = (int)AngleDelta(dir[YAW], pm->ps->viewangles[YAW]);

		if (pm->cmd.forwardmove < 0)
		{
			moveyaw = (int)AngleNormalize180(moveyaw + 180);
		}

		if (abs(moveyaw) > 75)
		{
			if (moveyaw > 0)
			{
				moveyaw = 75;
			}
			else
			{
				moveyaw = -75;
			}
		}

		pm->ps->movementDir = (signed char)moveyaw;
	}
	else
	{
		pm->ps->movementDir = 0;
	}
}


/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump(void)
{
	// no jumpin when prone
	if (pm->ps->eFlags & EF_PRONE)
	{
		return qfalse;
	}

	// JPW NERVE -- jumping in multiplayer uses and requires sprint juice (to prevent turbo skating, sprint + jumps)
	// don't allow jump accel

	// rain - revert to using pmext for this since pmext is fixed now.
	// fix for #166
	/* Nico, add flat jumping support
	if ( pm->cmd.serverTime - pm->pmext->jumpTime < 850 ) {
	    return qfalse;
	}*/
	if (!(pm->physics & PHYSICS_FLAT_JUMPING) && pm->cmd.serverTime - pm->pmext->jumpTime < 850)
	{
		// Nico, => no flat jumping
		return qfalse;
	}
	// Nico, end of add flat jumping support

	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		return qfalse;      // don't allow jump until all buttons are up
	}

	if (pm->cmd.upmove < 10)
	{
		// not holding jump
		return qfalse;
	}

	// must wait for jump to be released
	if (pm->ps->pm_flags & PMF_JUMP_HELD)
	{
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane   = qfalse;     // jumping away
	pml.walking       = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	// Nico, add doublejump support
	if (pm->physics & PHYSICS_RAMPBOUNCE && pm->ps->velocity[2] > 0)
	{
		PM_AddEvent(EV_JUMP);
		pm->ps->velocity[2] += JUMP_VELOCITY;
	}
	else
	{
		pm->ps->velocity[2] = JUMP_VELOCITY;
	}
	// Nico, end of add doublejump support

	if (pm->cmd.forwardmove >= 0)
	{
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMP, qfalse, qtrue);
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
	{
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMPBK, qfalse, qtrue);
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean PM_CheckWaterJump(void)
{
	vec3_t spot;
	int    cont;
	vec3_t flatforward;

	if (pm->ps->pm_time)
	{
		return qfalse;
	}

	// check for water jump
	if (pm->waterlevel != 2)
	{
		return qfalse;
	}

	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	VectorMA(pm->ps->origin, 30, flatforward, spot);
	spot[2] += 4;
	cont     = pm->pointcontents(spot, pm->ps->clientNum);
	if (!(cont & CONTENTS_SOLID))
	{
		return qfalse;
	}

	spot[2] += 16;
	cont     = pm->pointcontents(spot, pm->ps->clientNum);
	if (cont)
	{
		return qfalse;
	}

	// jump out of water
	VectorScale(pml.forward, 200, pm->ps->velocity);
	pm->ps->velocity[2] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time   = 2000;

	return qtrue;
}

/*
==============
PM_CheckProne

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static qboolean PM_CheckProne(void)
{
	//Com_Printf( "%i: PM_CheckProne (%i)\n", pm->cmd.serverTime, pm->pmext->proneGroundTime );

	if (!(pm->ps->eFlags & EF_PRONE))
	{

		// can't go prone on ladders
		if (pm->ps->pm_flags & PMF_LADDER)
		{
			return qfalse;
		}

		// no prone when using mg42's
		if (pm->ps->persistant[PERS_HWEAPON_USE] || pm->ps->eFlags & EF_MOUNTEDTANK)
		{
			return qfalse;
		}

		if (pm->ps->weaponDelay && pm->ps->weapon == WP_PANZERFAUST)
		{
			return qfalse;
		}

		if (pm->ps->weapon == WP_MORTAR_SET)
		{
			return qfalse;
		}

		// can't go prone while swimming
		if (pm->waterlevel > 1)
		{
			return qfalse;
		}

		if (((pm->ps->pm_flags & PMF_DUCKED && pm->cmd.doubleTap == DT_FORWARD) ||
		     (pm->cmd.wbuttons & WBUTTON_PRONE)) && pm->cmd.serverTime - -pm->pmext->proneTime > 750)
		{
			trace_t trace;

			pm->mins[0] = pm->ps->mins[0];
			pm->mins[1] = pm->ps->mins[1];

			pm->maxs[0] = pm->ps->maxs[0];
			pm->maxs[1] = pm->ps->maxs[1];

			pm->mins[2] = pm->ps->mins[2];
			pm->maxs[2] = pm->ps->crouchMaxZ;

			pm->ps->eFlags |= EF_PRONE;
			PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
			pm->ps->eFlags &= ~EF_PRONE;

			if (!trace.startsolid && !trace.allsolid)
			{
				// go prone
				pm->ps->pm_flags          |= PMF_DUCKED; // crouched as well
				pm->ps->eFlags            |= EF_PRONE;
				pm->pmext->proneTime       = pm->cmd.serverTime; // timestamp 'go prone'
				pm->pmext->proneGroundTime = pm->cmd.serverTime;
			}
		}
	}

	if (pm->ps->eFlags & EF_PRONE)
	{
		if (pm->waterlevel > 1 ||
		    pm->ps->pm_type == PM_DEAD ||
		    pm->ps->eFlags & EF_MOUNTEDTANK ||
		    ((pm->cmd.doubleTap == DT_BACK || pm->cmd.upmove > 10 || pm->cmd.wbuttons & WBUTTON_PRONE) && pm->cmd.serverTime - pm->pmext->proneTime > 750))
		{
			trace_t trace;

			// see if we have the space to stop prone
			pm->mins[0] = pm->ps->mins[0];
			pm->mins[1] = pm->ps->mins[1];

			pm->maxs[0] = pm->ps->maxs[0];
			pm->maxs[1] = pm->ps->maxs[1];

			pm->mins[2] = pm->ps->mins[2];
			pm->maxs[2] = pm->ps->crouchMaxZ;

			pm->ps->eFlags &= ~EF_PRONE;
			PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
			pm->ps->eFlags |= EF_PRONE;

			if (!trace.allsolid)
			{
				// crouch for a bit
				pm->ps->pm_flags |= PMF_DUCKED;

				// stop prone
				pm->ps->eFlags      &= ~EF_PRONE;
				pm->ps->eFlags      &= ~EF_PRONE_MOVING;
				pm->pmext->proneTime = -pm->cmd.serverTime; // timestamp 'stop prone'

				if (pm->ps->weapon == WP_MOBILE_MG42_SET)
				{
					PM_BeginWeaponChange(WP_MOBILE_MG42_SET, WP_MOBILE_MG42, qfalse);
				}

				// don't jump for a bit
				pm->pmext->jumpTime = pm->cmd.serverTime - 650;
				pm->ps->jumpTime    = pm->cmd.serverTime - 650;
			}
		}
	}

	if (pm->ps->eFlags & EF_PRONE)
	{
		//float frac;

		// See if we are moving
		float    spd       = VectorLength(pm->ps->velocity);
		qboolean userinput = abs(pm->cmd.forwardmove) + abs(pm->cmd.rightmove) > 10 ? qtrue : qfalse;

		if (userinput && spd > 40.f && !(pm->ps->eFlags & EF_PRONE_MOVING))
		{
			pm->ps->eFlags |= EF_PRONE_MOVING;

			switch (pm->ps->weapon)
			{
			case WP_FG42SCOPE: PM_BeginWeaponChange(WP_FG42SCOPE, WP_FG42, qfalse); break;
			case WP_GARAND_SCOPE: PM_BeginWeaponChange(WP_GARAND_SCOPE, WP_GARAND, qfalse); break;
			case WP_K43_SCOPE: PM_BeginWeaponChange(WP_K43_SCOPE, WP_K43, qfalse); break;
			}
		}
		else if (!userinput && spd < 20.0f && (pm->ps->eFlags & EF_PRONE_MOVING))
		{
			pm->ps->eFlags &= ~EF_PRONE_MOVING;
		}

		pm->mins[0] = pm->ps->mins[0];
		pm->mins[1] = pm->ps->mins[1];

		pm->maxs[0] = pm->ps->maxs[0];
		pm->maxs[1] = pm->ps->maxs[1];

		pm->mins[2] = pm->ps->mins[2];

		//frac = (pm->cmd.serverTime - pm->pmext->proneTime) / 500.f;
		//if( frac > 1.f )
		//	frac = 1.f;

		//pm->maxs[2] = pm->ps->maxs[2] - (frac * (pm->ps->standViewHeight - PRONE_VIEWHEIGHT));
		//pm->ps->viewheight = DEFAULT_VIEWHEIGHT - (frac * (DEFAULT_VIEWHEIGHT - PRONE_VIEWHEIGHT));	// default - prone to get a positive which is subtracted from default
		pm->maxs[2]        = pm->ps->maxs[2] - pm->ps->standViewHeight - PRONE_VIEWHEIGHT;
		pm->ps->viewheight = PRONE_VIEWHEIGHT;

		return(qtrue);
	}

	return(qfalse);
}
//============================================================================

/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove(void)
{
	// waterjump has no control, but falls

	PM_StepSlideMove(qtrue);

	pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	if (pm->ps->velocity[2] < 0)
	{
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time   = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove(void)
{
	int    i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;
	float  vel;

	if (PM_CheckWaterJump())
	{
		PM_WaterJumpMove();
		return;
	}
	PM_Friction();

	scale = PM_CmdScale(&pm->cmd, qfalse);
	//
	// user intentions
	//
	if (!scale)
	{
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = -60;       // sink towards bottom
	}
	else
	{
		for (i = 0 ; i < 3 ; i++)
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	if (pm->watertype == CONTENTS_SLIME)        //----(SA)	slag
	{
		if (wishspeed > pm->ps->speed * pm_slagSwimScale)
		{
			wishspeed = pm->ps->speed * pm_slagSwimScale;
		}

		PM_Accelerate(wishdir, wishspeed, pm_slagaccelerate);
	}
	else
	{
		if (wishspeed > pm->ps->speed * pm_waterSwimScale)
		{
			wishspeed = pm->ps->speed * pm_waterSwimScale;
		}

		PM_Accelerate(wishdir, wishspeed, pm_wateraccelerate);
	}


	// make sure we can go up slopes easily under water
	if (pml.groundPlane && DotProduct(pm->ps->velocity, pml.groundTrace.plane.normal) < 0)
	{
		vel = VectorLength(pm->ps->velocity);
		// slide along the ground plane
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
		                pm->ps->velocity, OVERCLIP);

		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, vel, pm->ps->velocity);
	}

	PM_SlideMove(qfalse);
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove(void)
{
	int    i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;

	// normal slowdown
	PM_Friction();

	scale = PM_CmdScale(&pm->cmd, qfalse);

	//
	// user intentions
	//
	if (!scale)
	{
		wishvel[0] = 0;
		wishvel[1] = 0;
		wishvel[2] = 0;
	}
	else
	{
		for (i = 0 ; i < 3 ; i++)
		{
			wishvel[i] = scale * pml.forward[i] * pm->cmd.forwardmove + scale * pml.right[i] * pm->cmd.rightmove;
		}

		wishvel[2] += scale * pm->cmd.upmove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	PM_Accelerate(wishdir, wishspeed, pm_flyaccelerate);

	PM_StepSlideMove(qfalse);
}

// Nico, from Racesow
// when using +strafe convert the inertia to forward speed.
static void PM_Aircontrol(pmove_t *pm, vec3_t wishdir, float wishspeed)
{
	float zspeed, speed, dot, k;
	int   i;
	float smove;

	if (!pm_aircontrol)
	{
		return;
	}

	// accelerate
	smove = pm->cmd.rightmove;

	if ((smove > 0 || smove < 0) || (wishspeed == 0.0))
	{
		return; // can't control movement if not moving forward or backward
	}

	zspeed              = pm->ps->velocity[2];
	pm->ps->velocity[2] = 0;
	speed               = VectorNormalize(pm->ps->velocity);

	dot = DotProduct(pm->ps->velocity, wishdir);
	k   = 32.0f * pm_aircontrol * dot * dot * pml.frametime;

	if (dot > 0)
	{
		// we can't change direction while slowing down
		for (i = 0; i < 2; i++)
		{
			pm->ps->velocity[i] = pm->ps->velocity[i] * speed + wishdir[i] * k;
		}
		VectorNormalize(pm->ps->velocity);
	}

	for (i = 0; i < 2; i++)
	{
		pm->ps->velocity[i] *= speed;
	}
	pm->ps->velocity[2] = zspeed;
}


/*
===================
PM_AirMove

===================
*/
static void PM_AirMove(void)
{
	int       i;
	vec3_t    wishvel;
	float     fmove, smove;
	vec3_t    wishdir;
	float     wishspeed;
	float     scale;
	usercmd_t cmd;

	// Nico, air control vars
	float wishspeed2;
	float accel;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	// project moves down to flat plane
	if (pm->cmd.serverTime < 350)
	{
		pml.forward[2] = fmove = 0;
		smove          = pm->pmext->dtmove == DT_MOVELEFT ? -2070 : 2070;
		scale          = 1.f;
	}
	else
	{
		cmd   = pm->cmd;
		scale = PM_CmdScale(&cmd, qtrue);

		// Ridah, moved this down, so we use the actual movement direction
		// set the movementDir so clients can rotate the legs for strafing
		//	PM_SetMovementDir();

		pml.forward[2] = 0;
		pml.right[2]   = 0;
	}
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	for (i = 0 ; i < 2 ; i++)
	{
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}
	wishvel[2] = 0;

	VectorCopy(wishvel, wishdir);
	wishspeed  = VectorNormalize(wishdir);
	wishspeed *= scale;

	// Nico, add aircontrol support
	if (!(pm->physics & PHYSICS_AIRCONTROL && (!pm->isTimerun || pm->timerunActive == 1 || pm->isLogged == 0)))
	{
		// Nico, note: disabled air control
		// not on ground, so little effect on velocity
		PM_Accelerate(wishdir, wishspeed, pm_airaccelerate);
	}
	else
	{
		// Air Control
		wishspeed2 = wishspeed;
		if (DotProduct(pm->ps->velocity, wishdir) < 0)
		{
			accel = pm_airstopaccelerate;
		}
		else
		{
			accel = pm_airaccelerate;
		}

		if ((smove > 0 || smove < 0) && !fmove)
		{
			if (wishspeed > pm_wishspeed)
			{
				wishspeed = pm_wishspeed;
			}
			accel = pm_strafeaccelerate;
		}

		// Air control
		PM_Accelerate(wishdir, wishspeed, accel);
		PM_Aircontrol(pm, wishdir, wishspeed2);
	}
	// Nico, end of add aircontrol support

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if (pml.groundPlane)
	{
		PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal,
		                pm->ps->velocity, OVERCLIP);
	}

	PM_StepSlideMove(qtrue);

// Ridah, moved this down, so we use the actual movement direction
	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();
}



/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove(void)
{
	int       i;
	vec3_t    wishvel;
	float     fmove, smove;
	vec3_t    wishdir;
	float     wishspeed;
	float     scale;
	usercmd_t cmd;
	float     accelerate;
	float     vel;

	if (pm->waterlevel > 2 && DotProduct(pml.forward, pml.groundTrace.plane.normal) > 0)
	{
		// begin swimming
		PM_WaterMove();
		return;
	}

	if (PM_CheckJump())
	{
		// jumped away
		if (pm->waterlevel > 1)
		{
			PM_WaterMove();
		}
		else
		{
			PM_AirMove();
		}

		if (!(pm->cmd.serverTime - pm->pmext->jumpTime < 850))
		{
			pm->pmext->jumpTime = pm->cmd.serverTime;
		}

		// JPW NERVE
		pm->ps->jumpTime = pm->cmd.serverTime;  // Arnout: NOTE : TEMP DEBUG

		return;
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd   = pm->cmd;
	scale = PM_CmdScale(&cmd, qtrue);

// Ridah, moved this down, so we use the actual movement direction
	// set the movementDir so clients can rotate the legs for strafing
//	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[2] = 0;
	pml.right[2]   = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity(pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP);
	PM_ClipVelocity(pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP);
	//
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	for (i = 0 ; i < 3 ; i++)
	{
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	}

	VectorCopy(wishvel, wishdir);
	wishspeed  = VectorNormalize(wishdir);
	wishspeed *= scale;

	// clamp the speed lower if prone
	if (pm->ps->eFlags & EF_PRONE)
	{
		if (wishspeed > pm->ps->speed * pm_proneSpeedScale)
		{
			wishspeed = pm->ps->speed * pm_proneSpeedScale;
		}
	}
	else if (pm->ps->pm_flags & PMF_DUCKED)       // clamp the speed lower if ducking
	{
		if (wishspeed > pm->ps->speed * pm->ps->crouchSpeedScale)
		{
			wishspeed = pm->ps->speed * pm->ps->crouchSpeedScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if (pm->waterlevel)
	{
		float waterScale;

		waterScale = pm->waterlevel / 3.0;
		if (pm->watertype == CONTENTS_SLIME)     //----(SA)	slag
		{
			waterScale = 1.0 - (1.0 - pm_slagSwimScale) * waterScale;
		}
		else
		{
			waterScale = 1.0 - (1.0 - pm_waterSwimScale) * waterScale;
		}

		if (wishspeed > pm->ps->speed * waterScale)
		{
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if (pm->ps->pm_flags & PMF_TIME_KNOCKBACK)
	{
		accelerate = pm_airaccelerate;
	}
	else if (pml.groundTrace.surfaceFlags & SURF_SLICK)
	{
		if (pm->physics & PHYSICS_SLICK_CONTROL)
		{
			accelerate = pm_slickaccelerate;
		}
		else
		{
			accelerate = pm_airaccelerate;
		}
	}
	else
	{
		// Nico, AP or stock accel?
		if (pm->physics == PHYSICS_MODE_AP_NO_OB || pm->physics == PHYSICS_MODE_AP_OB)
		{
			accelerate = pm_accelerate_AP;
		}
		else
		{
			accelerate = pm_accelerate;
		}
	}

	PM_Accelerate(wishdir, wishspeed, accelerate);

	if ((pml.groundTrace.surfaceFlags & SURF_SLICK) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK)
	{
		pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
	}

//----(SA)	added
	// show breath when standing on 'snow' surfaces
	if (pml.groundTrace.surfaceFlags & SURF_SNOW)
	{
		pm->ps->eFlags |= EF_BREATH;
	}
	else
	{
		pm->ps->eFlags &= ~EF_BREATH;
	}
//----(SA)	end

	vel = VectorLength(pm->ps->velocity);

	// slide along the ground plane
	PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);

	// don't do anything if standing still
	if (!pm->ps->velocity[0] && !pm->ps->velocity[1])
	{
		if (pm->ps->eFlags & EF_PRONE)
		{
			pm->pmext->proneGroundTime = pm->cmd.serverTime;
		}
		return;
	}

	// don't decrease velocity when going up or down a slope
	VectorNormalize(pm->ps->velocity);
	VectorScale(pm->ps->velocity, vel, pm->ps->velocity);

	PM_StepSlideMove(qfalse);

// Ridah, moved this down, so we use the actual movement direction
	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();
}


/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove(void)
{
	float forward;

	if (!pml.walking)
	{
		return;
	}

	// extra friction

	forward  = VectorLength(pm->ps->velocity);
	forward -= 20;
	if (forward <= 0)
	{
		VectorClear(pm->ps->velocity);
	}
	else
	{
		VectorNormalize(pm->ps->velocity);
		VectorScale(pm->ps->velocity, forward, pm->ps->velocity);
	}
}


/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove(void)
{
	float  speed, drop, friction, control, newspeed;
	int    i;
	vec3_t wishvel;
	float  fmove, smove;
	vec3_t wishdir;
	float  wishspeed;
	float  scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength(pm->ps->velocity);
	if (speed < 1)
	{
		VectorCopy(vec3_origin, pm->ps->velocity);
	}
	else
	{
		drop = 0;

		friction = pm_friction * 1.5; // extra friction
		control  = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop    += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;
		if (newspeed < 0)
		{
			newspeed = 0;
		}
		newspeed /= speed;

		VectorScale(pm->ps->velocity, newspeed, pm->ps->velocity);
	}

	// accelerate
	scale = PM_CmdScale(&pm->cmd, qfalse);

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for (i = 0 ; i < 3 ; i++)
		wishvel[i] = pml.forward[i] * fmove + pml.right[i] * smove;
	wishvel[2] += pm->cmd.upmove;

	VectorCopy(wishvel, wishdir);
	wishspeed  = VectorNormalize(wishdir);
	wishspeed *= scale;

	// Nico, AP or stock accel?
	if (pm->physics == PHYSICS_MODE_AP_NO_OB || pm->physics == PHYSICS_MODE_AP_OB)
	{
		PM_Accelerate(wishdir, wishspeed, pm_accelerate_AP);
	}
	else
	{
		PM_Accelerate(wishdir, wishspeed, pm_accelerate);
	}

	// move
	VectorMA(pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin);
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface(void)
{
#ifdef GAMEDLL
	// In just the GAME DLL, we want to store the groundtrace surface stuff,
	// so we don't have to keep tracing.
	ClientStoreSurfaceFlags(pm->ps->clientNum, pml.groundTrace.surfaceFlags);

#endif // GAMEDLL

	return BG_FootstepForSurface(pml.groundTrace.surfaceFlags);
}

/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand(void)
{
	float delta;
	float dist;
	float vel, acc;
	float t;
	float a, b, c, den;

	// Ridah, only play this if coming down hard
	if (!pm->ps->legsTimer)
	{
		if (pml.previous_velocity[2] < -220)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_LAND, qfalse, qtrue);
		}
	}

	// calculate the exact velocity on landing
	dist = pm->ps->origin[2] - pml.previous_origin[2];
	vel  = pml.previous_velocity[2];
	acc  = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;
	if (den < 0)
	{
		return;
	}
	t = (-b - sqrt(den)) / (2 * a);

	delta = vel + t * acc;
	delta = delta * delta * 0.0001;

	// never take falling damage if completely underwater
	if (pm->waterlevel == 3)
	{
		return;
	}

	// reduce falling damage if there is standing water
	if (pm->waterlevel == 2)
	{
		delta *= 0.25;
	}
	if (pm->waterlevel == 1)
	{
		delta *= 0.5;
	}

	if (delta < 1)
	{
		return;
	}

	// create a local entity event to play the sound

	// Nico, add no fall damage support
	if (pm->physics & PHYSICS_NO_FALLDAMAGE)
	{
		// Nico, no fall damage, simply play a footstep sound
		PM_AddEventExt(EV_FOOTSTEP, PM_FootstepForSurface());
	}
	else
	{
		// SURF_NODAMAGE is used for bounce pads where you don't ever
		// want to take damage or play a crunch sound
		if (!(pml.groundTrace.surfaceFlags & SURF_NODAMAGE))
		{
			if (pm->debugLevel)
			{
				Com_Printf("delta: %5.2f\n", delta);
			}

			if (delta > 77)
			{
				PM_AddEventExt(EV_FALL_NDIE, PM_FootstepForSurface());
			}
			else if (delta > 67)
			{
				PM_AddEventExt(EV_FALL_DMG_50, PM_FootstepForSurface());
			}
			else if (delta > 58)
			{
				// this is a pain grunt, so don't play it if dead
				if (pm->ps->stats[STAT_HEALTH] > 0)
				{
					PM_AddEventExt(EV_FALL_DMG_25, PM_FootstepForSurface());
				}
			}
			else if (delta > 48)
			{
				// this is a pain grunt, so don't play it if dead
				if (pm->ps->stats[STAT_HEALTH] > 0)
				{
					PM_AddEventExt(EV_FALL_DMG_15, PM_FootstepForSurface());
				}
			}
			else if (delta > 38.75)
			{
				// this is a pain grunt, so don't play it if dead
				if (pm->ps->stats[STAT_HEALTH] > 0)
				{
					PM_AddEventExt(EV_FALL_DMG_10, PM_FootstepForSurface());
				}
			}
			else if (delta > 7)
			{
				PM_AddEventExt(EV_FALL_SHORT, PM_FootstepForSurface());
			}
			else
			{
				PM_AddEventExt(EV_FOOTSTEP, PM_FootstepForSurface());
			}

			// rain - when falling damage happens, velocity is cleared, but
			// this needs to happen in pmove, not g_active!  (prediction will be
			// wrong, otherwise.)
			// Nico, don't reset speed on SURF_NODAMAGE
			if (delta > 38.75)
			{
				VectorClear(pm->ps->velocity);
			}
		}
	}
	// Nico, end of add no fall damage support

	// start footstep cycle over
	pm->ps->bobCycle = 0;
}



/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid(trace_t *trace)
{
	int    i, j, k;
	vec3_t point;

	if (pm->debugLevel)
	{
		Com_Printf("%i:allsolid\n", c_pmove);
	}

	// jitter around
	for (i = -1; i <= 1; i++)
	{
		for (j = -1; j <= 1; j++)
		{
			for (k = -1; k <= 1; k++)
			{
				VectorCopy(pm->ps->origin, point);
				point[0] += (float) i;
				point[1] += (float) j;
				point[2] += (float) k;
				PM_TraceAll(trace, point, point);
				if (!trace->allsolid)
				{
					point[0] = pm->ps->origin[0];
					point[1] = pm->ps->origin[1];
					point[2] = pm->ps->origin[2] - 0.25;

					PM_TraceAll(trace, pm->ps->origin, point);
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane         = qfalse;
	pml.walking             = qfalse;

	return qfalse;
}


/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed(void)
{
	trace_t trace;
	vec3_t  point;

	if (pm->ps->groundEntityNum != ENTITYNUM_NONE)
	{
		// we just transitioned into freefall
		if (pm->debugLevel)
		{
			Com_Printf("%i:lift\n", c_pmove);
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy(pm->ps->origin, point);
		point[2] -= 64;

		PM_TraceAll(&trace, pm->ps->origin, point);
		if (trace.fraction == 1.0)
		{
			if (pm->cmd.forwardmove >= 0)
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMP, qfalse, qtrue);
				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else
			{
				BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMPBK, qfalse, qtrue);
				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	// If we've never yet touched the ground, it's because we're spawning, so don't
	// set to "in air"
	if (pm->ps->groundEntityNum != -1)
	{
		// Signify that we're in mid-air
		pm->ps->groundEntityNum = ENTITYNUM_NONE;

	} // if (pm->ps->groundEntityNum != -1)...
	pml.groundPlane = qfalse;
	pml.walking     = qfalse;
}


/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace(void)
{
	vec3_t  point;
	trace_t trace;

	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];


	if (pm->ps->eFlags & EF_MG42_ACTIVE || pm->ps->eFlags & EF_AAGUN_ACTIVE)
	{
		point[2] = pm->ps->origin[2] - 1.f;
	}
	else
	{
		//point[2] = pm->ps->origin[2] - 0.01f;
		point[2] = pm->ps->origin[2] - 0.25f;
	}

	PM_TraceAllLegs(&trace, &pm->pmext->proneLegsOffset, pm->ps->origin, point);
	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if (trace.allsolid && !(pm->ps->eFlags & EF_MOUNTEDTANK))
	{
		if (!PM_CorrectAllSolid(&trace))
		{
			return;
		}
	}

	// if the trace didn't hit anything, we are in free fall
	if (trace.fraction == 1.0)
	{
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking     = qfalse;
		return;
	}

	// check if getting thrown off the ground
	if (pm->ps->velocity[2] > 0 && DotProduct(pm->ps->velocity, trace.plane.normal) > 10 && !(pm->ps->eFlags & EF_PRONE))
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:kickoff\n", c_pmove);
		}
		// go into jump animation
		if (pm->cmd.forwardmove >= 0)
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMP, qfalse, qfalse);
			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
		else
		{
			BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_JUMPBK, qfalse, qfalse);
			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane         = qfalse;
		pml.walking             = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if (trace.plane.normal[2] < MIN_WALK_NORMAL)
	{
		if (pm->debugLevel)
		{
			Com_Printf("%i:steep\n", c_pmove);
		}
		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane         = qtrue;
		pml.walking             = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking     = qtrue;

	// hitting solid ground will end a waterjump
	if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		pm->ps->pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND);
		pm->ps->pm_time   = 0;
	}

	if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{
		// just hit the ground
		if (pm->debugLevel)
		{
			Com_Printf("%i:Land\n", c_pmove);
		}

		PM_CrashLand();

		// Nico, OB enabled/disabled ?
		if (pm->physics & PHYSICS_NO_OVERBOUNCE)
		{
			PM_ClipVelocity(pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity, OVERCLIP);
		}

		// don't do landing time if we were just going down a slope
		if (pml.previous_velocity[2] < -200)
		{
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time   = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// Nico, OB enabled/disabled ?
	if (pm->physics & PHYSICS_NO_OVERBOUNCE && trace.plane.normal[2] == 1)
	{
		pm->ps->velocity[2] = 0;
	}

	PM_AddTouchEnt(trace.entityNum);
}


/*
=============
PM_SetWaterLevel	FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel(void)
{
	vec3_t point;
	int    cont;
	int    sample1;
	int    sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype  = 0;

	// Ridah, modified this
	point[0] = pm->ps->origin[0];
	point[1] = pm->ps->origin[1];
	point[2] = pm->ps->origin[2] + pm->ps->mins[2] + 1;
	cont     = pm->pointcontents(point, pm->ps->clientNum);

	if (cont & MASK_WATER)
	{
		sample2 = pm->ps->viewheight - pm->ps->mins[2];
		sample1 = sample2 / 2;

		pm->watertype  = cont;
		pm->waterlevel = 1;
		point[2]       = pm->ps->origin[2] + pm->ps->mins[2] + sample1;
		cont           = pm->pointcontents(point, pm->ps->clientNum);
		if (cont & MASK_WATER)
		{
			pm->waterlevel = 2;
			point[2]       = pm->ps->origin[2] + pm->ps->mins[2] + sample2;
			cont           = pm->pointcontents(point, pm->ps->clientNum);
			if (cont & MASK_WATER)
			{
				pm->waterlevel = 3;
			}
		}
	}
	// done.

	// UNDERWATER
	BG_UpdateConditionValue(pm->ps->clientNum, ANIM_COND_UNDERWATER, (pm->waterlevel > 2), qtrue);

}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck(void)
{
	trace_t trace;

	// Ridah, modified this for configurable bounding boxes
	pm->mins[0] = pm->ps->mins[0];
	pm->mins[1] = pm->ps->mins[1];

	pm->maxs[0] = pm->ps->maxs[0];
	pm->maxs[1] = pm->ps->maxs[1];

	pm->mins[2] = pm->ps->mins[2];

	if (pm->ps->pm_type == PM_DEAD)
	{
		pm->maxs[2]        = pm->ps->maxs[2];   // NOTE: must set death bounding box in game code
		pm->ps->viewheight = pm->ps->deadViewHeight;
		return;
	}

	if ((pm->cmd.upmove < 0 && !(pm->ps->eFlags & EF_MOUNTEDTANK) && !(pm->ps->pm_flags & PMF_LADDER)) || pm->ps->weapon == WP_MORTAR_SET)           // duck
	{
		pm->ps->pm_flags |= PMF_DUCKED;
	}
	else
	{   // stand up if possible
		if (pm->ps->pm_flags & PMF_DUCKED)
		{
			// try to stand up
			pm->maxs[2] = pm->ps->maxs[2];
			PM_TraceAll(&trace, pm->ps->origin, pm->ps->origin);
			if (!trace.allsolid)
			{
				pm->ps->pm_flags &= ~PMF_DUCKED;
			}
		}
	}

	if (pm->ps->pm_flags & PMF_DUCKED)
	{
		pm->maxs[2]        = pm->ps->crouchMaxZ;
		pm->ps->viewheight = pm->ps->crouchViewHeight;
	}
	else
	{
		pm->maxs[2]        = pm->ps->maxs[2];
		pm->ps->viewheight = pm->ps->standViewHeight;
	}
	// done.
}



//===================================================================


/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps(void)
{
	float    bobmove;
	int      old;
	qboolean footstep;
	qboolean iswalking;
	int      animResult = -1;

	if (pm->ps->eFlags & EF_DEAD)
	{


		//if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
		if (pm->ps->pm_flags & PMF_FLAILING)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_FLAILING, qtrue);

			if (!pm->ps->pm_time)
			{
				pm->ps->pm_flags &= ~PMF_FLAILING;  // the eagle has landed
			}
		}
		else if (!pm->ps->pm_time && !(pm->ps->pm_flags & PMF_LIMBO))         // DHM - Nerve :: before going to limbo, play a wounded/fallen animation
		{
			if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
			{
				// takeoff!
				pm->ps->pm_flags |= PMF_FLAILING;
				animResult        = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_FLAILING, qtrue);
			}
			else
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_FALLEN, qtrue);
			}
		}

		return;
	}

	iswalking = qfalse;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	pm->xyspeed = sqrt(pm->ps->velocity[0] * pm->ps->velocity[0] +  pm->ps->velocity[1] * pm->ps->velocity[1]);

	// mg42, always idle
	if (pm->ps->persistant[PERS_HWEAPON_USE])
	{
		animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLE, qtrue);
		//
		return;
	}

	// swimming
	if (pm->waterlevel > 2)
	{

		if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_SWIMBK, qtrue);
		}
		else
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_SWIM, qtrue);
		}

		return;
	}

	// in the air
	if (pm->ps->groundEntityNum == ENTITYNUM_NONE)
	{
		if (pm->ps->pm_flags & PMF_LADDER)                 // on ladder
		{
			if (pm->ps->velocity[2] >= 0)
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_CLIMBUP, qtrue);
				//BG_PlayAnimName( pm->ps, "BOTH_CLIMB", ANIM_BP_BOTH, qfalse, qtrue, qfalse );
			}
			else if (pm->ps->velocity[2] < 0)
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_CLIMBDOWN, qtrue);
				//BG_PlayAnimName( pm->ps, "BOTH_CLIMB_DOWN", ANIM_BP_BOTH, qfalse, qtrue, qfalse );
			}
		}

		return;
	}

	// if not trying to move
	if (!pm->cmd.forwardmove && !pm->cmd.rightmove)
	{
		if (pm->xyspeed < 5)
		{
			pm->ps->bobCycle = 0;   // start at beginning of cycle again
		}
		if (pm->xyspeed > 120)
		{
			return; // continue what they were doing last frame, until we stop
		}

		if (pm->ps->eFlags & EF_PRONE)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLEPRONE, qtrue);
		}
		else if (pm->ps->pm_flags & PMF_DUCKED)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLECR, qtrue);
		}

		if (animResult < 0)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLE, qtrue);
		}
		//
		return;
	}

	footstep = qfalse;


	if (pm->ps->eFlags & EF_PRONE)
	{
		bobmove = 0.2;  // prone characters bob slower
		if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_PRONEBK, qtrue);
		}
		else
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_PRONE, qtrue);
		}
		// prone characters never play footsteps
	}
	else if (pm->ps->pm_flags & PMF_DUCKED)
	{
		bobmove = 0.5;  // ducked characters bob much faster
		if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_WALKCRBK, qtrue);
		}
		else
		{
			animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_WALKCR, qtrue);
		}
		// ducked characters never play footsteps
	}
	else if (pm->ps->pm_flags & PMF_BACKWARDS_RUN)
	{
		if (!(pm->cmd.buttons & BUTTON_WALKING))
		{
			bobmove  = 0.4; // faster speeds bob faster
			footstep = qtrue;
			// check for strafing
			if (pm->cmd.rightmove && !pm->cmd.forwardmove)
			{
				if (pm->cmd.rightmove > 0)
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFERIGHT, qtrue);
				}
				else
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFELEFT, qtrue);
				}
			}
			if (animResult < 0)       // if we havent found an anim yet, play the run
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_RUNBK, qtrue);
			}
		}
		else
		{
			bobmove = 0.3;
			// check for strafing
			if (pm->cmd.rightmove && !pm->cmd.forwardmove)
			{
				if (pm->cmd.rightmove > 0)
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFERIGHT, qtrue);
				}
				else
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFELEFT, qtrue);
				}
			}
			if (animResult < 0)       // if we havent found an anim yet, play the run
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_WALKBK, qtrue);
			}
		}

	}
	else
	{

		if (!(pm->cmd.buttons & BUTTON_WALKING))
		{
			bobmove  = 0.4; // faster speeds bob faster
			footstep = qtrue;
			// check for strafing
			if (pm->cmd.rightmove && !pm->cmd.forwardmove)
			{
				if (pm->cmd.rightmove > 0)
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFERIGHT, qtrue);
				}
				else
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFELEFT, qtrue);
				}
			}
			if (animResult < 0)       // if we havent found an anim yet, play the run
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_RUN, qtrue);
			}
		}
		else
		{
			bobmove = 0.3;  // walking bobs slow
			if (pm->cmd.rightmove && !pm->cmd.forwardmove)
			{
				if (pm->cmd.rightmove > 0)
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFERIGHT, qtrue);
				}
				else
				{
					animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_STRAFELEFT, qtrue);
				}
			}
			if (animResult < 0)       // if we havent found an anim yet, play the run
			{
				animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_WALK, qtrue);
			}
		}
	}

	// if no anim found yet, then just use the idle as default
	if (animResult < 0)
	{
		animResult = BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLE, qtrue);
	}

	// check for footstep / splash sounds
	old              = pm->ps->bobCycle;
	pm->ps->bobCycle = (int)(old + bobmove * pml.msec) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if (iswalking)
	{
		// sounds much more natural this way
		if (old > pm->ps->bobCycle)
		{

			if (pm->waterlevel == 0)
			{
				if (footstep && !pm->noFootsteps)
				{
					PM_AddEventExt(EV_FOOTSTEP, PM_FootstepForSurface());
				}
			}
			else if (pm->waterlevel == 1)
			{
				// splashing
				PM_AddEvent(EV_FOOTSPLASH);
			}
			else if (pm->waterlevel == 2)
			{
				// wading / swimming at surface
				PM_AddEvent(EV_SWIM);
			}
			else if (pm->waterlevel == 3)
			{
				// no sound when completely underwater
			}

		}
	}
	else if (((old + 64) ^ (pm->ps->bobCycle + 64)) & 128)
	{

		if (pm->waterlevel == 0)
		{
			// on ground will only play sounds if running
			if (footstep && !pm->noFootsteps)
			{
				PM_AddEventExt(EV_FOOTSTEP, PM_FootstepForSurface());
			}
		}
		else if (pm->waterlevel == 1)
		{
			// splashing
			PM_AddEvent(EV_FOOTSPLASH);
		}
		else if (pm->waterlevel == 2)
		{
			// wading / swimming at surface
			PM_AddEvent(EV_SWIM);
		}
		else if (pm->waterlevel == 3)
		{
			// no sound when completely underwater

		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents(void)            // FIXME?
{   //
	// if just entered a water volume, play a sound
	//
	if (!pml.previous_waterlevel && pm->waterlevel)
	{
		PM_AddEvent(EV_WATER_TOUCH);
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if (pml.previous_waterlevel && !pm->waterlevel)
	{
		PM_AddEvent(EV_WATER_LEAVE);
	}

	//
	// check for head just going under water
	//
	if (pml.previous_waterlevel != 3 && pm->waterlevel == 3)
	{
		PM_AddEvent(EV_WATER_UNDER);
	}

	//
	// check for head just coming out of water
	//
	if (pml.previous_waterlevel == 3 && pm->waterlevel != 3)
	{
		if (pm->pmext->airleft < 6000)
		{
			PM_AddEventExt(EV_WATER_CLEAR, 1);
		}
		else
		{
			PM_AddEventExt(EV_WATER_CLEAR, 0);
		}
	}
}

/*
================
PM_Animate
================
*/
#define MYTIMER_SALUTE   1133   // 17 frames, 15 fps
#define MYTIMER_DISMOUNT 667    // 10 frames, 15 fps

/*
================
PM_DropTimers
================
*/
static void PM_DropTimers(void)
{
	// drop misc timing counter
	if (pm->ps->pm_time)
	{
		if (pml.msec >= pm->ps->pm_time)
		{
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time   = 0;
		}
		else
		{
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	if (pm->ps->legsTimer > 0)
	{
		pm->ps->legsTimer -= pml.msec;
		if (pm->ps->legsTimer < 0)
		{
			pm->ps->legsTimer = 0;
		}
	}

	if (pm->ps->torsoTimer > 0)
	{
		pm->ps->torsoTimer -= pml.msec;
		if (pm->ps->torsoTimer < 0)
		{
			pm->ps->torsoTimer = 0;
		}
	}

	// first person weapon counter
	if (pm->pmext->weapAnimTimer > 0)
	{
		pm->pmext->weapAnimTimer -= pml.msec;
		if (pm->pmext->weapAnimTimer < 0)
		{
			pm->pmext->weapAnimTimer = 0;
		}
	}
}



#define LEAN_MAX    28.0f
#define LEAN_TIME_TO    200.0f  // time to get to/from full lean
#define LEAN_TIME_FR    300.0f  // time to get to/from full lean

/*
==============
PM_CalcLean

==============
*/
void PM_UpdateLean(playerState_t *ps, usercmd_t *cmd, pmove_t *tpm)
{
	vec3_t  start, end, tmins, tmaxs, right;
	int     leaning = 0;        // -1 left, 1 right
	float   leanofs = 0;
	vec3_t  viewangles;
	trace_t trace;

	if ((cmd->wbuttons & (WBUTTON_LEANLEFT | WBUTTON_LEANRIGHT))  && !cmd->forwardmove && cmd->upmove <= 0)
	{
		// if both are pressed, result is no lean
		if (cmd->wbuttons & WBUTTON_LEANLEFT)
		{
			leaning -= 1;
		}
		if (cmd->wbuttons & WBUTTON_LEANRIGHT)
		{
			leaning += 1;
		}
	}

	if (BG_PlayerMounted(ps->eFlags))
	{
		leaning = 0;    // leaning not allowed on mg42
	}

	if (ps->eFlags & EF_FIRING)
	{
		leaning = 0;    // not allowed to lean while firing

	}
	// ATVI Wolfenstein Misc #479 - initial fix to #270 would crash in g_synchronousClients 1 situation
	if (ps->weaponstate == WEAPON_FIRING && ps->weapon == WP_DYNAMITE)
	{
		leaning = 0; // not allowed while tossing dynamite

	}
	if (ps->eFlags & EF_PRONE || ps->weapon == WP_MORTAR_SET)
	{
		leaning = 0;    // not allowed to lean while prone

	}
	leanofs = ps->leanf;


	if (!leaning)      // go back to center position
	{
		if (leanofs > 0)            // right
		{   //FIXME: play lean anim backwards?
			leanofs -= (((float)pml.msec / (float)LEAN_TIME_FR) * LEAN_MAX);
			if (leanofs < 0)
			{
				leanofs = 0;
			}
		}
		else if (leanofs < 0)         // left
		{   //FIXME: play lean anim backwards?
			leanofs += (((float)pml.msec / (float)LEAN_TIME_FR) * LEAN_MAX);
			if (leanofs > 0)
			{
				leanofs = 0;
			}
		}
	}

	if (leaning)
	{
		if (leaning > 0)       // right
		{
			if (leanofs < LEAN_MAX)
			{
				leanofs += (((float)pml.msec / (float)LEAN_TIME_TO) * LEAN_MAX);
			}

			if (leanofs > LEAN_MAX)
			{
				leanofs = LEAN_MAX;
			}

		}
		else                  // left
		{
			if (leanofs > -LEAN_MAX)
			{
				leanofs -= (((float)pml.msec / (float)LEAN_TIME_TO) * LEAN_MAX);
			}

			if (leanofs < -LEAN_MAX)
			{
				leanofs = -LEAN_MAX;
			}

		}
	}

	ps->leanf = leanofs;

	if (leaning)
	{
		VectorCopy(ps->origin, start);
		start[2] += ps->viewheight;

		VectorCopy(ps->viewangles, viewangles);
		viewangles[ROLL] += leanofs / 2.0f;
		AngleVectors(viewangles, NULL, right, NULL);
		VectorMA(start, leanofs, right, end);

		VectorSet(tmins, -8, -8, -7);   // ATVI Wolfenstein Misc #472, bumped from -4 to cover gun clipping issue
		VectorSet(tmaxs, 8, 8, 4);

		if (pm)
		{
			pm->trace(&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);
		}
		else
		{
			tpm->trace(&trace, start, tmins, tmaxs, end, ps->clientNum, MASK_PLAYERSOLID);
		}

		ps->leanf *= trace.fraction;
	}


	if (ps->leanf)
	{
		cmd->rightmove = 0;     // also disallowed in cl_input ~391

	}
}



/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated isntead of a full move

    !! NOTE !! Any changes to mounted/prone view should be duplicated in BotEntityWithinView()
================
*/
// rain - take a tracemask as well - we can't use anything out of pm
void PM_UpdateViewAngles(playerState_t *ps, pmoveExt_t *pmext, usercmd_t *cmd, void(trace) (trace_t * results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentMask), int tracemask)        //----(SA)	modified
{
	short   temp;
	int     i;
	pmove_t tpm;
	vec3_t  oldViewAngles;

	// DHM - Nerve :: Added support for PMF_TIME_LOCKPLAYER
	if (ps->pm_flags & PMF_TIME_LOCKPLAYER)
	{
		return;     // no view changes at all
	}

	if (ps->pm_type != PM_SPECTATOR && ps->stats[STAT_HEALTH] <= 0)
	{

		// DHM - Nerve :: Allow players to look around while 'wounded' or lock to a medic if nearby
		temp = cmd->angles[1] + ps->delta_angles[1];
		// rain - always allow this.  viewlocking will take precedence
		// if a medic is found
		// rain - using a full short and converting on the client so that
		// we get >1 degree resolution
		ps->stats[STAT_DEAD_YAW] = temp;
		return;     // no view changes at all
	}

	VectorCopy(ps->viewangles, oldViewAngles);

	// circularly clamp the angles with deltas
	for (i = 0 ; i < 3 ; i++)
	{
		temp = cmd->angles[i] + ps->delta_angles[i];
		if (i == PITCH)
		{
			// don't let the player look up or down more than 90 degrees
			if (temp > 16000)
			{
				ps->delta_angles[i] = 16000 - cmd->angles[i];
				temp                = 16000;
			}
			else if (temp < -16000)
			{
				ps->delta_angles[i] = -16000 - cmd->angles[i];
				temp                = -16000;
			}
		}
		ps->viewangles[i] = SHORT2ANGLE(temp);
	}

	if (BG_PlayerMounted(ps->eFlags))
	{
		float yaw, oldYaw;
		float degsSec = MG42_YAWSPEED;
		float arcMin, arcMax, arcDiff;

		yaw    = ps->viewangles[YAW];
		oldYaw = oldViewAngles[YAW];

		if (yaw - oldYaw > 180)
		{
			yaw -= 360;
		}
		if (yaw - oldYaw < -180)
		{
			yaw += 360;
		}

		if (yaw > oldYaw)
		{
			if (yaw - oldYaw > degsSec * pml.frametime)
			{
				ps->viewangles[YAW] = oldYaw + degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}
		else if (oldYaw > yaw)
		{
			if (oldYaw - yaw > degsSec * pml.frametime)
			{
				ps->viewangles[YAW] = oldYaw - degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}

		// limit harc and varc

		// pitch (varc)
		arcMax = pmext->varc;
		if (ps->eFlags & EF_AAGUN_ACTIVE)
		{
			arcMin = 0;
		}
		else if (ps->eFlags & EF_MOUNTEDTANK)
		{
			float angle;

			arcMin = 14;
			arcMax = 50;

			angle = cos(DEG2RAD(AngleNormalize180(pmext->centerangles[1] - ps->viewangles[1])));
			angle = -AngleNormalize360(angle * AngleNormalize180(0 - pmext->centerangles[0]));

			pmext->centerangles[PITCH] = angle;
		}
		else
		{
			arcMin = pmext->varc / 2;
		}

		arcDiff = AngleNormalize180(ps->viewangles[PITCH] - pmext->centerangles[PITCH]);

		if (arcDiff > arcMin)
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->centerangles[PITCH] + arcMin);

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}
		else if (arcDiff < -arcMax)
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->centerangles[PITCH] - arcMax);

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}

		if (!(ps->eFlags & EF_MOUNTEDTANK))
		{
			// yaw (harc)
			arcMin  = arcMax = pmext->harc;
			arcDiff = AngleNormalize180(ps->viewangles[YAW] - pmext->centerangles[YAW]);

			if (arcDiff > arcMin)
			{
				ps->viewangles[YAW] = AngleNormalize180(pmext->centerangles[YAW] + arcMin);

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
			else if (arcDiff < -arcMax)
			{
				ps->viewangles[YAW] = AngleNormalize180(pmext->centerangles[YAW] - arcMax);

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}
	}
	else if (ps->weapon == WP_MORTAR_SET)
	{
		float degsSec = 60.f;
		float yaw, oldYaw;
		float pitch, oldPitch;
		float pitchMax = 30.f;
		float yawDiff, pitchDiff;

		yaw    = ps->viewangles[YAW];
		oldYaw = oldViewAngles[YAW];

		if (yaw - oldYaw > 180)
		{
			yaw -= 360;
		}
		if (yaw - oldYaw < -180)
		{
			yaw += 360;
		}

		if (yaw > oldYaw)
		{
			if (yaw - oldYaw > degsSec * pml.frametime)
			{
				ps->viewangles[YAW] = oldYaw + degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}
		else if (oldYaw > yaw)
		{
			if (oldYaw - yaw > degsSec * pml.frametime)
			{
				ps->viewangles[YAW] = oldYaw - degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}

		pitch    = ps->viewangles[PITCH];
		oldPitch = oldViewAngles[PITCH];

		if (pitch - oldPitch > 180)
		{
			pitch -= 360;
		}
		if (pitch - oldPitch < -180)
		{
			pitch += 360;
		}

		if (pitch > oldPitch)
		{
			if (pitch - oldPitch > degsSec * pml.frametime)
			{
				ps->viewangles[PITCH] = oldPitch + degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
			}
		}
		else if (oldPitch > pitch)
		{
			if (oldPitch - pitch > degsSec * pml.frametime)
			{
				ps->viewangles[PITCH] = oldPitch - degsSec * pml.frametime;

				// Set delta_angles properly
				ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
			}
		}

		// yaw
		yawDiff = ps->viewangles[YAW] - pmext->mountedWeaponAngles[YAW];

		if (yawDiff > 180)
		{
			yawDiff -= 360;
		}
		else if (yawDiff < -180)
		{
			yawDiff += 360;
		}

		if (yawDiff > 30)
		{
			ps->viewangles[YAW] = AngleNormalize180(pmext->mountedWeaponAngles[YAW] + 30.f);

			// Set delta_angles properly
			ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
		}
		else if (yawDiff < -30)
		{
			ps->viewangles[YAW] = AngleNormalize180(pmext->mountedWeaponAngles[YAW] - 30.f);

			// Set delta_angles properly
			ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
		}

		// pitch
		pitchDiff = ps->viewangles[PITCH] - pmext->mountedWeaponAngles[PITCH];

		if (pitchDiff > 180)
		{
			pitchDiff -= 360;
		}
		else if (pitchDiff < -180)
		{
			pitchDiff += 360;
		}

		if (pitchDiff > (pitchMax - 10.f))
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->mountedWeaponAngles[PITCH] + (pitchMax - 10.f));

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}
		else if (pitchDiff < -(pitchMax))
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->mountedWeaponAngles[PITCH] - (pitchMax));

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}
	}
	else if (ps->eFlags & EF_PRONE)
	{
		//float degsSec = 60.f;
		float /*yaw, */ oldYaw;
		trace_t         traceres; // rain - renamed
		int             newDeltaAngle = ps->delta_angles[YAW];
		float           pitchMax      = 40.f;
		float           yawDiff, pitchDiff;

		oldYaw = oldViewAngles[YAW];

		// Check if we are allowed to rotate to there
		if (ps->weapon == WP_MOBILE_MG42_SET)
		{
			pitchMax = 20.f;

			// yaw
			yawDiff = ps->viewangles[YAW] - pmext->mountedWeaponAngles[YAW];

			if (yawDiff > 180)
			{
				yawDiff -= 360;
			}
			else if (yawDiff < -180)
			{
				yawDiff += 360;
			}

			if (yawDiff > 20)
			{
				ps->viewangles[YAW] = AngleNormalize180(pmext->mountedWeaponAngles[YAW] + 20.f);

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
			else if (yawDiff < -20)
			{
				ps->viewangles[YAW] = AngleNormalize180(pmext->mountedWeaponAngles[YAW] - 20.f);

				// Set delta_angles properly
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
		}

		// pitch
		pitchDiff = ps->viewangles[PITCH] - pmext->mountedWeaponAngles[PITCH];

		if (pitchDiff > 180)
		{
			pitchDiff -= 360;
		}
		else if (pitchDiff < -180)
		{
			pitchDiff += 360;
		}

		if (pitchDiff > pitchMax)
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->mountedWeaponAngles[PITCH] + pitchMax);

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}
		else if (pitchDiff < -pitchMax)
		{
			ps->viewangles[PITCH] = AngleNormalize180(pmext->mountedWeaponAngles[PITCH] - pitchMax);

			// Set delta_angles properly
			ps->delta_angles[PITCH] = ANGLE2SHORT(ps->viewangles[PITCH]) - cmd->angles[PITCH];
		}

		// Check if we rotated into a wall with our legs, if so, undo yaw
		if (ps->viewangles[YAW] != oldYaw)
		{
			// see if we have the space to go prone
			// we know our main body isn't in a solid, check for our legs

			// rain - bugfix - use supplied trace - pm may not be set
			PM_TraceLegs(&traceres, &pmext->proneLegsOffset, ps->origin, ps->origin, NULL, ps->viewangles, pm->trace, ps->clientNum, tracemask);

			if (traceres.allsolid /* && trace.entityNum >= MAX_CLIENTS */)
			{
				// starting in a solid, no space
				ps->viewangles[YAW]   = oldYaw;
				ps->delta_angles[YAW] = ANGLE2SHORT(ps->viewangles[YAW]) - cmd->angles[YAW];
			}
			else
			{
				// all fine
				ps->delta_angles[YAW] = newDeltaAngle;
			}
		}
	}

	tpm.trace = trace;

	PM_UpdateLean(ps, cmd, &tpm);
}

/*
================
PM_CheckLadderMove

  Checks to see if we are on a ladder
================
*/
qboolean ladderforward;
vec3_t   laddervec;

void PM_CheckLadderMove(void)
{
	vec3_t  spot;
	vec3_t  flatforward;
	trace_t trace;
	float   tracedist;
	#define TRACE_LADDER_DIST   48.0
	qboolean wasOnLadder;

	if (pm->ps->pm_time)
	{
		return;
	}

	if (pml.walking)
	{
		tracedist = 1.0;
	}
	else
	{
		tracedist = TRACE_LADDER_DIST;
	}

	wasOnLadder = ((pm->ps->pm_flags & PMF_LADDER) != 0);

	pml.ladder        = qfalse;
	pm->ps->pm_flags &= ~PMF_LADDER;    // clear ladder bit
	ladderforward     = qfalse;

	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane         = qfalse;
		pml.walking             = qfalse;
		return;
	}

	// Can't climb ladders while prone
	if (pm->ps->eFlags & EF_PRONE)
	{
		return;
	}

	// check for ladder
	flatforward[0] = pml.forward[0];
	flatforward[1] = pml.forward[1];
	flatforward[2] = 0;
	VectorNormalize(flatforward);

	VectorMA(pm->ps->origin, tracedist, flatforward, spot);
	pm->trace(&trace, pm->ps->origin, pm->mins, pm->maxs, spot, pm->ps->clientNum, pm->tracemask);
	if ((trace.fraction < 1) && (trace.surfaceFlags & SURF_LADDER))
	{
		pml.ladder = qtrue;
	}

	if (pml.ladder)
	{
		VectorCopy(trace.plane.normal, laddervec);
	}

	if (pml.ladder && !pml.walking && (trace.fraction * tracedist > 1.0))
	{
		vec3_t mins;
		// if we are only just on the ladder, don't do this yet, or it may throw us back off the ladder
		pml.ladder = qfalse;
		VectorCopy(pm->mins, mins);
		mins[2] = -1;
		VectorMA(pm->ps->origin, -tracedist, laddervec, spot);
		pm->trace(&trace, pm->ps->origin, mins, pm->maxs, spot, pm->ps->clientNum, pm->tracemask);
		if ((trace.fraction < 1) && (trace.surfaceFlags & SURF_LADDER))
		{
			ladderforward     = qtrue;
			pml.ladder        = qtrue;
			pm->ps->pm_flags |= PMF_LADDER; // set ladder bit
		}
		else
		{
			pml.ladder = qfalse;
		}
	}
	else if (pml.ladder)
	{
		pm->ps->pm_flags |= PMF_LADDER; // set ladder bit
	}

	// create some up/down velocity if touching ladder
	if (pml.ladder)
	{
		if (pml.walking)
		{
			// we are currently on the ground, only go up and prevent X/Y if we are pushing forwards
			if (pm->cmd.forwardmove <= 0)
			{
				pml.ladder = qfalse;
			}
		}
	}

	// if we have just dismounted the ladder at the top, play dismount
	if (!pml.ladder && wasOnLadder && pm->ps->velocity[2] > 0)
	{
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_CLIMB_DISMOUNT, qfalse, qfalse);
	}
	// if we have just mounted the ladder
	if (pml.ladder && !wasOnLadder && pm->ps->velocity[2] < 0)        // only play anim if going down ladder
	{
		BG_AnimScriptEvent(pm->ps, pm->character->animModelInfo, ANIM_ET_CLIMB_MOUNT, qfalse, qfalse);
	}
}

/*
============
PM_LadderMove
============
*/
void PM_LadderMove(void)
{
	float  wishspeed, scale;
	vec3_t wishdir, wishvel;
	float  upscale;

	if (ladderforward)
	{
		// move towards the ladder
		VectorScale(laddervec, -200.0, wishvel);
		pm->ps->velocity[0] = wishvel[0];
		pm->ps->velocity[1] = wishvel[1];
	}

	upscale = (pml.forward[2] + 0.5) * 2.5;
	if (upscale > 1.0)
	{
		upscale = 1.0;
	}
	else if (upscale < -1.0)
	{
		upscale = -1.0;
	}

	// forward/right should be horizontal only
	pml.forward[2] = 0;
	pml.right[2]   = 0;
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	// move depending on the view, if view is straight forward, then go up
	// if view is down more then X degrees, start going down
	// if they are back pedalling, then go in reverse of above
	scale = PM_CmdScale(&pm->cmd, qfalse);
	VectorClear(wishvel);

	if (pm->cmd.forwardmove)
	{
		wishvel[2] = 0.9 * upscale * scale * (float)pm->cmd.forwardmove;
	}
//Com_Printf("wishvel[2] = %i, fwdmove = %i\n", (int)wishvel[2], (int)pm->cmd.forwardmove );

	if (pm->cmd.rightmove)
	{
		// strafe, so we can jump off ladder
		vec3_t ladder_right, ang;
		vectoangles(laddervec, ang);
		AngleVectors(ang, NULL, ladder_right, NULL);

		// if we are looking away from the ladder, reverse the right vector
		if (DotProduct(laddervec, pml.forward) < 0)
		{
			VectorInverse(ladder_right);
		}

		//VectorMA( wishvel, 0.5 * scale * (float)pm->cmd.rightmove, pml.right, wishvel );
		VectorMA(wishvel, 0.5 * scale * (float)pm->cmd.rightmove, ladder_right, wishvel);
	}

	// do strafe friction
	PM_Friction();

	if (pm->ps->velocity[0] < 1 && pm->ps->velocity[0] > -1)
	{
		pm->ps->velocity[0] = 0;
	}
	if (pm->ps->velocity[1] < 1 && pm->ps->velocity[1] > -1)
	{
		pm->ps->velocity[1] = 0;
	}

	wishspeed = VectorNormalize2(wishvel, wishdir);

	// Nico, AP or stock accel?
	if (pm->physics == PHYSICS_MODE_AP_NO_OB || pm->physics == PHYSICS_MODE_AP_OB)
	{
		PM_Accelerate(wishdir, wishspeed, pm_accelerate_AP);
	}
	else
	{
		PM_Accelerate(wishdir, wishspeed, pm_accelerate);
	}

	if (!wishvel[2])
	{
		if (pm->ps->velocity[2] > 0)
		{
			pm->ps->velocity[2] -= pm->ps->gravity * pml.frametime;
			if (pm->ps->velocity[2] < 0)
			{
				pm->ps->velocity[2] = 0;
			}
		}
		else
		{
			pm->ps->velocity[2] += pm->ps->gravity * pml.frametime;
			if (pm->ps->velocity[2] > 0)
			{
				pm->ps->velocity[2] = 0;
			}
		}
	}

	PM_StepSlideMove(qfalse);    // no gravity while going up ladder

	// always point legs forward
	pm->ps->movementDir = 0;
}

/*
================
PmoveSingle

================
*/
void trap_SnapVector(float *v);

void PmoveSingle(pmove_t *pmove)
{
	// RF, update conditional values for anim system
	BG_AnimUpdatePlayerStateConditions(pmove);

	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
	c_pmove++;

	// clear results
	pm->numtouch   = 0;
	pm->watertype  = 0;
	pm->waterlevel = 0;

	if (pm->ps->stats[STAT_HEALTH] <= 0)
	{
		pm->ps->eFlags &= ~EF_ZOOMING;
	}

	// Nico, copy pressed keys into playerstate
	// buttons, wbuttons
	pm->ps->stats[STAT_USERCMD_BUTTONS]  = pm->cmd.buttons << 8;
	pm->ps->stats[STAT_USERCMD_BUTTONS] |= pm->cmd.wbuttons;
	pm->ps->stats[STAT_USERCMD_MOVE]     = 0;

	// forwardmove
	if (pm->cmd.forwardmove > 0)
	{
		pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_FORWARD;
	}
	else if (pm->cmd.forwardmove < 0)
	{
		pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_BACKWARD;
	}

	// rightmove
	if (pm->cmd.rightmove > 0)
	{
		pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_RIGHT;
	}
	else if (pm->cmd.rightmove < 0)
	{
		pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_LEFT;
	}

	// upmove
	if (pm->cmd.upmove > 0)
	{
		pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_UP;
	}
	else if (pm->cmd.upmove < 0)
	{
		pm->ps->stats[STAT_USERCMD_MOVE] |= UMOVE_DOWN;
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if (abs(pm->cmd.forwardmove) > 64 || abs(pm->cmd.rightmove) > 64)
	{
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if (pm->cmd.buttons & BUTTON_TALK)
	{
		pm->ps->eFlags |= EF_TALK;
	}
	else
	{
		pm->ps->eFlags &= ~EF_TALK;
	}

	// set the firing flag for continuous beam weapons

	pm->ps->eFlags &= ~(EF_FIRING | EF_ZOOMING);

	if (pm->cmd.wbuttons & WBUTTON_ZOOM && pm->ps->stats[STAT_HEALTH] >= 0 && !(pm->ps->weaponDelay))
	{
		if (pm->ps->stats[STAT_KEYS] & (1 << INV_BINOCS))          // (SA) binoculars are an inventory item (inventory==keys)
		{
			if (!BG_IsScopedWeapon(pm->ps->weapon) &&          // don't allow binocs if using the sniper scope
			    !BG_PlayerMounted(pm->ps->eFlags) &&        // or if mounted on a weapon
			    // rain - #215 - don't allow binocs w/ mounted mob. MG42 or mortar either.
			    pm->ps->weapon != WP_MOBILE_MG42_SET &&
			    pm->ps->weapon != WP_MORTAR_SET)
			{

				pm->ps->eFlags |= EF_ZOOMING;
			}
		}

		// don't allow binocs if in the middle of throwing grenade
		if ((pm->ps->weapon == WP_GRENADE_LAUNCHER || pm->ps->weapon == WP_GRENADE_PINEAPPLE || pm->ps->weapon == WP_DYNAMITE) && pm->ps->grenadeTimeLeft > 0)
		{
			pm->ps->eFlags &= ~EF_ZOOMING;
		}
	}

	if (!(pm->ps->pm_flags & PMF_RESPAWNED))
	{

		// check for ammo
		if (PM_WeaponAmmoAvailable(pm->ps->weapon))
		{
			// check if zooming
			// DHM - Nerve :: Let's use the same flag we just checked above, Ok?
			if (!(pm->ps->eFlags & EF_ZOOMING))
			{
				if (!pm->ps->leanf)
				{
					if (pm->ps->weaponstate == WEAPON_READY || pm->ps->weaponstate == WEAPON_FIRING)
					{

						// all clear, fire!
						if (pm->cmd.buttons & BUTTON_ATTACK && !(pm->cmd.buttons & BUTTON_TALK))
						{
							pm->ps->eFlags |= EF_FIRING;
						}
					}
				}
			}
		}
	}


	if (pm->ps->pm_flags & PMF_RESPAWNED)
	{
		if (pm->ps->stats[STAT_PLAYER_CLASS] == PC_COVERTOPS)
		{
			pm->pmext->silencedSideArm |= 1;
		}
	}

	// clear the respawned flag if attack and use are cleared
	if (pm->ps->stats[STAT_HEALTH] > 0 &&
	    !(pm->cmd.buttons & BUTTON_ATTACK))
	{
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if (pmove->cmd.buttons & BUTTON_TALK)
	{
		// keep the talk button set tho for when the cmd.serverTime > 66 msec
		// and the same cmd is used multiple times in Pmove
		pmove->cmd.buttons     = BUTTON_TALK;
		pmove->cmd.wbuttons    = 0;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove   = 0;
		pmove->cmd.upmove      = 0;
		pmove->cmd.doubleTap   = 0;
	}

	// fretn, moved from engine to this very place
	// no movement while using a static mg42
	if (pm->ps->persistant[PERS_HWEAPON_USE])
	{
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove   = 0;
		pmove->cmd.upmove      = 0;
	}

	// clear all pmove local vars
	memset(&pml, 0, sizeof(pml));

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	if (pml.msec < 1)
	{
		pml.msec = 1;
	}
	else if (pml.msec > 200)
	{
		pml.msec = 200;
	}
	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy(pm->ps->origin, pml.previous_origin);

	// save old velocity for crashlanding
	VectorCopy(pm->ps->velocity, pml.previous_velocity);

	pml.frametime = pml.msec * 0.001;

	// update the viewangles
	if (pm->ps->pm_type != PM_FREEZE)     // Arnout: added PM_FREEZE
	{
		if (!(pm->ps->pm_flags & PMF_LIMBO))       // JPW NERVE
		{   // rain - added tracemask
			PM_UpdateViewAngles(pm->ps, pm->pmext, &pm->cmd, pm->trace, pm->tracemask);     //----(SA)	modified

		}
	}
	AngleVectors(pm->ps->viewangles, pml.forward, pml.right, pml.up);

	if (pm->cmd.upmove < 10)
	{
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if (pm->cmd.forwardmove < 0)
	{
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	}
	else if (pm->cmd.forwardmove > 0 || (pm->cmd.forwardmove == 0 && pm->cmd.rightmove))
	{
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if (pm->ps->pm_type >= PM_DEAD || pm->ps->pm_flags & (PMF_LIMBO | PMF_TIME_LOCKPLAYER))                 // DHM - Nerve
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove   = 0;
		pm->cmd.upmove      = 0;
	}

	if (pm->ps->pm_type == PM_SPECTATOR)
	{
		PM_CheckDuck();
		PM_FlyMove();
		PM_DropTimers();
		return;
	}

	if (pm->ps->pm_type == PM_NOCLIP)
	{
		PM_NoclipMove();
		PM_DropTimers();
		return;
	}

	if (pm->ps->pm_type == PM_FREEZE)
	{
		return;     // no movement at all
	}

	// ydnar: need gravity etc to affect a player with a set mortar
	if (pm->ps->weapon == WP_MORTAR_SET && pm->ps->pm_type == PM_NORMAL)
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove   = 0;
		pm->cmd.upmove      = 0;
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	if (!PM_CheckProne())
	{
		PM_CheckDuck();
	}

	// set groundentity
	PM_GroundTrace();

	if (pm->ps->pm_type == PM_DEAD)
	{
		PM_DeadMove();

		if (pm->ps->weapon == WP_MORTAR_SET)
		{
			pm->ps->weapon = WP_MORTAR;
		}
	}
	else
	{
		if (pm->ps->weapon == WP_MOBILE_MG42_SET)
		{
			if (!(pm->ps->eFlags & EF_PRONE))
			{
				PM_BeginWeaponChange(WP_MOBILE_MG42_SET, WP_MOBILE_MG42, qfalse);
#ifdef CGAMEDLL
				cg.weaponSelect = WP_MOBILE_MG42;
#endif // CGAMEDLL
			}
		}
		if (pm->ps->weapon == WP_SATCHEL_DET)
		{
			if (!(pm->ps->ammoclip[WP_SATCHEL_DET]))
			{
				PM_BeginWeaponChange(WP_SATCHEL_DET, WP_SATCHEL, qtrue);
#ifdef CGAMEDLL
				cg.weaponSelect = WP_SATCHEL;
#endif // CGAMEDLL
			}
		}
	}

	// Ridah, ladders
	PM_CheckLadderMove();

	PM_DropTimers();

	if (pml.ladder)
	{
		PM_LadderMove();
	}
	else if (pm->ps->pm_flags & PMF_TIME_WATERJUMP)
	{
		PM_WaterJumpMove();
	}
	else if (pm->waterlevel > 1)
	{
		// swimming
		PM_WaterMove();
	}
	else if (pml.walking && !(pm->ps->eFlags & EF_MOUNTEDTANK))
	{
		// walking on ground
		PM_WalkMove();
	}
	else if (!(pm->ps->eFlags & EF_MOUNTEDTANK))
	{
		// airborne
		PM_AirMove();
	}

	if (pm->ps->eFlags & EF_MOUNTEDTANK)
	{
		VectorClear(pm->ps->velocity);

		pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

		BG_AnimScriptAnimation(pm->ps, pm->character->animModelInfo, ANIM_MT_IDLE, qtrue);
	}

	/* Nico, this function does nothing because there is no more fatigue
	PM_Sprint();*/

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap some parts of playerstate to save network bandwidth
	trap_SnapVector(pm->ps->velocity);
}


/*
================
Pmove

Can be called by either the server or the client
================
*/
int Pmove(pmove_t *pmove)
{
	int finalTime;

	finalTime = pmove->cmd.serverTime;

	if (finalTime < pmove->ps->commandTime)
	{
		return (0);   // should not happen
	}

	if (finalTime > pmove->ps->commandTime + 1000)
	{
		pmove->ps->commandTime = finalTime - 1000;
	}

	// after a loadgame, prevent huge pmove's
	if (pmove->ps->pm_flags & PMF_TIME_LOAD)
	{
		if (finalTime - pmove->ps->commandTime > 50)
		{
			pmove->ps->commandTime = finalTime - 50;
		}
	}

	pmove->ps->pmove_framecount = (pmove->ps->pmove_framecount + 1) & ((1 << PS_PMOVEFRAMECOUNTBITS) - 1);

	// RF
	pm = pmove;
	PM_AdjustAimSpreadScale();

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while (pmove->ps->commandTime != finalTime)
	{
		int msec;

		msec = finalTime - pmove->ps->commandTime;

		if (pmove->pmove_fixed)
		{
			if (msec > pmove->pmove_msec)
			{
				msec = pmove->pmove_msec;
			}
		}
		else
		{
			// rain - this was 66 (15fps), but I've changed it to
			// 50 (20fps, max rate of mg42) to alleviate some of the
			// framerate dependency with the mg42.
			// in reality, this should be split according to sv_fps,
			// and pmove() shouldn't handle weapon firing
			if (msec > 50)
			{
				msec = 50;
			}
		}
		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle(pmove);

		if (pmove->ps->pm_flags & PMF_JUMP_HELD)
		{
			pmove->cmd.upmove = 20;
		}
	}

	// rain - sanity check weapon heat
	if (pmove->ps->curWeapHeat > 255)
	{
		pmove->ps->curWeapHeat = 255;
	}
	else if (pmove->ps->curWeapHeat < 0)
	{
		pmove->ps->curWeapHeat = 0;
	}

	if ((pm->ps->stats[STAT_HEALTH] <= 0 || pm->ps->pm_type == PM_DEAD) && pml.groundTrace.surfaceFlags & SURF_MONSTERSLICK)
	{
		return (pml.groundTrace.surfaceFlags);
	}
	else
	{
		return (0);
	}

}
