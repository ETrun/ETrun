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



// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

pmove_t cg_pmove;

static int       cg_numSolidEntities;
static centity_t *cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static int       cg_numSolidFTEntities;
static centity_t *cg_solidFTEntities[MAX_ENTITIES_IN_SNAPSHOT];
static int       cg_numTriggerEntities;
static centity_t *cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList(void) {
	int           i;
	centity_t     *cent;
	snapshot_t    *snap;
	entityState_t *ent;

	cg_numSolidEntities   = 0;
	cg_numSolidFTEntities = 0;
	cg_numTriggerEntities = 0;

	if (cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport) {
		snap = cg.nextSnap;
	} else {
		snap = cg.snap;
	}

	for (i = 0 ; i < snap->numEntities ; i++) {
		cent = &cg_entities[snap->entities[i].number];
		ent  = &cent->currentState;

		// rain - don't clip against temporarily non-solid SOLID_BMODELS
		// (e.g. constructibles); use current state so prediction isn't fubar
		if (cent->currentState.solid == SOLID_BMODEL &&
		    (cent->currentState.eFlags & EF_NONSOLID_BMODEL)) {
			continue;
		}

		if (ent->eType == ET_ITEM ||
		    ent->eType == ET_PUSH_TRIGGER ||
		    ent->eType == ET_TELEPORT_TRIGGER ||
		    ent->eType == ET_CONCUSSIVE_TRIGGER ||
		    ent->eType == ET_OID_TRIGGER
#ifdef VISIBLE_TRIGGERS
		    || ent->eType == ET_TRIGGER_MULTIPLE
		    || ent->eType == ET_TRIGGER_FLAGONLY
		    || ent->eType == ET_TRIGGER_FLAGONLY_MULTIPLE
#endif
		    ) {

			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if (ent->eType == ET_CONSTRUCTIBLE) {
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
		}

		if (cent->nextState.solid) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;

			cg_solidFTEntities[cg_numSolidFTEntities] = cent;
			cg_numSolidFTEntities++;
			continue;
		}
	}
}

/*
====================
CG_ClipMoveToEntities

====================
*/
/* Nico, add an extra argument to enable/disable tracing players
static void CG_ClipMoveToEntities( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
                                   int skipNumber, int mask, int capsule, trace_t *tr ) {*/
static void CG_ClipMoveToEntities(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
                                  int skipNumber, int mask, int capsule, qboolean tracePlayers, trace_t *tr) {
	int           i, x, zd, zu;
	trace_t       trace;
	entityState_t *ent;
	clipHandle_t  cmodel;
	vec3_t        bmins, bmaxs;
	vec3_t        origin, angles;
	centity_t     *cent;

	for (i = 0 ; i < cg_numSolidEntities ; i++) {
		cent = cg_solidEntities[i];
		ent  = &cent->currentState;

		/* Nico, also continue if ent is a player and tracePlayers is false
		if ( ent->number == skipNumber ) {*/
		if (ent->number == skipNumber || (!tracePlayers && ent->eType == ET_PLAYER)) {
			continue;
		}

		if (ent->solid == SOLID_BMODEL) {
			// special value for bmodel
			cmodel = trap_CM_InlineModel(ent->modelindex);
			BG_EvaluateTrajectory(&cent->currentState.apos, cg.physicsTime, angles, qtrue, cent->currentState.effect2Time);
			BG_EvaluateTrajectory(&cent->currentState.pos, cg.physicsTime, origin, qfalse, cent->currentState.effect2Time);
		} else {
			// Nico, see g_misc.c SP_func_fakebrush...
			if (ent->eFlags & EF_FAKEBMODEL) {
				VectorCopy(ent->origin2, bmins);
				VectorCopy(ent->angles2, bmaxs);
			} else {
				// encoded bbox
				x  = (ent->solid & 255);
				zd = ((ent->solid >> 8) & 255);
				zu = ((ent->solid >> 16) & 255) - 32;

				bmins[0] = bmins[1] = -x;
				bmaxs[0] = bmaxs[1] = x;
				bmins[2] = -zd;
				bmaxs[2] = zu;
			}

			cmodel = trap_CM_TempBoxModel(bmins, bmaxs);

			VectorCopy(vec3_origin, angles);
			VectorCopy(cent->lerpOrigin, origin);
		}
		// MrE: use bbox of capsule
		if (capsule) {
			trap_CM_TransformedCapsuleTrace(&trace, start, end,
			                                mins, maxs, cmodel, mask, origin, angles);
		} else {
			trap_CM_TransformedBoxTrace(&trace, start, end,
			                            mins, maxs, cmodel, mask, origin, angles);
		}

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			*tr             = trace;
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;
		}
		if (tr->allsolid) {
			return;
		}
	}
}

static void CG_ClipMoveToEntities_FT(const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask, int capsule, trace_t *tr) {
	int           i, x, zd, zu;
	trace_t       trace;
	entityState_t *ent;
	clipHandle_t  cmodel;
	vec3_t        bmins, bmaxs;
	vec3_t        origin, angles;
	centity_t     *cent;

	for (i = 0 ; i < cg_numSolidFTEntities ; i++) {
		cent = cg_solidFTEntities[i];
		ent  = &cent->currentState;

		if (ent->number == skipNumber) {
			continue;
		}

		if (ent->solid == SOLID_BMODEL) {
			// special value for bmodel
			cmodel = trap_CM_InlineModel(ent->modelindex);
			BG_EvaluateTrajectory(&cent->currentState.apos, cg.physicsTime, angles, qtrue, cent->currentState.effect2Time);
			BG_EvaluateTrajectory(&cent->currentState.pos, cg.physicsTime, origin, qfalse, cent->currentState.effect2Time);
		} else {
			// encoded bbox
			x  = (ent->solid & 255);
			zd = ((ent->solid >> 8) & 255);
			zu = ((ent->solid >> 16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempCapsuleModel(bmins, bmaxs);

			VectorCopy(vec3_origin, angles);
			VectorCopy(cent->lerpOrigin, origin);
		}
		// MrE: use bbox of capsule
		if (capsule) {
			trap_CM_TransformedCapsuleTrace(&trace, start, end,
			                                mins, maxs, cmodel, mask, origin, angles);
		} else {
			trap_CM_TransformedBoxTrace(&trace, start, end,
			                            mins, maxs, cmodel, mask, origin, angles);
		}

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			*tr             = trace;
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;
		}
		if (tr->allsolid) {
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void    CG_Trace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
                 int skipNumber, int mask) {
	trace_t t;

	trap_CM_BoxTrace(&t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models

	CG_ClipMoveToEntities(start, mins, maxs, end, skipNumber, mask, qfalse, qtrue, &t);

	*result = t;
}

void    CG_FTTrace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask) {
	trace_t t;


	trap_CM_BoxTrace(&t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models

	CG_ClipMoveToEntities_FT(start, mins, maxs, end, skipNumber, mask, qfalse, &t);

	*result = t;
}

/* Nico, from TJMod
 * This function doesn't trace players but other entities with
 * CONTENTS_BODY (content of a temporary box brush) are still traced.
 */
void CG_TraceCapsuleNoPlayers(trace_t *result, const vec3_t start, const vec3_t mins,
                              const vec3_t maxs, const vec3_t end, int skipNumber, int mask) {
	trace_t t;

	trap_CM_CapsuleTrace(&t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities(start, mins, maxs, end, skipNumber, mask, qtrue, qfalse, &t);

	*result = t;
}

void CG_TraceCapsule_World(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask) {
	trace_t t;

	// Nico, silent GCC
	(void)skipNumber;

	trap_CM_CapsuleTrace(&t, start, end, mins, maxs, 0, mask);

	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	*result     = t;
}
/*
================
CG_PointContents
================
*/
int     CG_PointContents(const vec3_t point, int passEntityNum) {
	int           i;
	entityState_t *ent;
	centity_t     *cent;
	clipHandle_t  cmodel;
	int           contents;

	contents = trap_CM_PointContents(point, 0);

	for (i = 0 ; i < cg_numSolidEntities ; i++) {
		cent = cg_solidEntities[i];

		ent = &cent->currentState;

		if (ent->number == passEntityNum) {
			continue;
		}

		if (ent->solid != SOLID_BMODEL) {   // special value for bmodel
			continue;
		}

		cmodel = trap_CM_InlineModel(ent->modelindex);
		if (!cmodel) {
			continue;
		}

		contents |= trap_CM_TransformedPointContents(point, cmodel, cent->lerpOrigin, cent->lerpAngles);
		// Gordon: again, need to use the projected water position to allow for moving entity based water.
	}

	return contents;
}


/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState(qboolean grabAngles) {
	float         f;
	int           i;
	playerState_t *out;
	snapshot_t    *prev, *next;

	out  = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if (grabAngles) {
		usercmd_t cmd;
		int       cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd(cmdNum, &cmd);

		// rain - added tracemask
		PM_UpdateViewAngles(out, &cg.pmext, &cmd, CG_Trace, MASK_PLAYERSOLID);
	}

	// if the next frame is a teleport, we can't lerp to it
	if (cg.nextFrameTeleport) {
		return;
	}

	if (!next || next->serverTime <= prev->serverTime) {
		return;
	}

	f = (float)(cg.time - prev->serverTime) / (next->serverTime - prev->serverTime);

	i = next->ps.bobCycle;
	if (i < prev->ps.bobCycle) {
		i += 256;       // handle wraparound
	}
	out->bobCycle = prev->ps.bobCycle + f * (i - prev->ps.bobCycle);

	for (i = 0 ; i < 3 ; i++) {
		out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i]);
		if (!grabAngles) {
			out->viewangles[i] = LerpAngle(
			    prev->ps.viewangles[i], next->ps.viewangles[i], f);
		}
		out->velocity[i] = prev->ps.velocity[i] + f * (next->ps.velocity[i] - prev->ps.velocity[i]);
	}
}

/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction(void) {
	int           i;
	entityState_t *ent;
	clipHandle_t  cmodel;
	centity_t     *cent;
	qboolean      spectator;
	const char    *cs;

	// dead clients don't activate triggers
	if (cg.predictedPlayerState.stats[STAT_HEALTH] <= 0) {
		return;
	}

	spectator = ((cg.predictedPlayerState.pm_type == PM_SPECTATOR) || (cg.predictedPlayerState.pm_flags & PMF_LIMBO));       // JPW NERVE

	if (cg.predictedPlayerState.pm_type != PM_NORMAL && !spectator) {
		return;
	}

	for (i = 0 ; i < cg_numTriggerEntities ; i++) {
		cent = cg_triggerEntities[i];
		ent  = &cent->currentState;

		if (ent->eType == ET_ITEM && !spectator && (cg.predictedPlayerState.groundEntityNum == ENTITYNUM_WORLD)) {
			continue;
		}

		if (ent->solid != SOLID_BMODEL) {
			continue;
		}

		// Gordon: er, this lookup was wrong...
		cmodel = cgs.inlineDrawModel[ent->modelindex];
		if (!cmodel) {
			continue;
		}

		if (ent->eType == ET_CONSTRUCTIBLE ||
		    ent->eType == ET_OID_TRIGGER
#ifdef VISIBLE_TRIGGERS
		    || ent->eType == ET_TRIGGER_MULTIPLE
		    || ent->eType == ET_TRIGGER_FLAGONLY
		    || ent->eType == ET_TRIGGER_FLAGONLY_MULTIPLE
#endif
		    ) {
			vec3_t mins, maxs, pmins, pmaxs;

			if (ent->eType == ET_CONSTRUCTIBLE && ent->aiState) {
				continue;
			}

			trap_R_ModelBounds(cmodel, mins, maxs);

			VectorAdd(cent->lerpOrigin, mins, mins);
			VectorAdd(cent->lerpOrigin, maxs, maxs);

#ifdef VISIBLE_TRIGGERS
			if (ent->eType == ET_TRIGGER_MULTIPLE || ent->eType == ET_TRIGGER_FLAGONLY || ent->eType == ET_TRIGGER_FLAGONLY_MULTIPLE) {
			} else
#endif
			{
				// expand the bbox a bit
				VectorSet(mins, mins[0] - 48, mins[1] - 48, mins[2] - 48);
				VectorSet(maxs, maxs[0] + 48, maxs[1] + 48, maxs[2] + 48);
			}

			VectorAdd(cg.predictedPlayerState.origin, cg_pmove.mins, pmins);
			VectorAdd(cg.predictedPlayerState.origin, cg_pmove.maxs, pmaxs);

#ifdef VISIBLE_TRIGGERS
			CG_RailTrail(mins, maxs, 1);
#endif

			if (!BG_BBoxCollision(pmins, pmaxs, mins, maxs)) {
				continue;
			}

			cs = NULL;
			if (ent->eType == ET_OID_TRIGGER) {
				cs = CG_ConfigString(CS_OID_TRIGGERS + ent->teamNum);
			} else if (ent->eType == ET_CONSTRUCTIBLE) {
				cs = CG_ConfigString(CS_OID_TRIGGERS + ent->otherEntityNum2);
			}

			if (cs) {
				CG_ObjectivePrint(va("You are near %s\n", cs), SMALLCHAR_WIDTH);
			}

			continue;
		}
	}
}

#define RESET_PREDICTION                        \
	useCommand = current - CMD_BACKUP + 1;


/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction. (this is "dead reckoning" and would definately
be nice to have in there (SA))

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/

// rain - we need to keep pmext around for old frames, because Pmove()
// fills in some values when it does prediction.  This in itself is fine,
// but the prediction loop starts in the past and predicts from the
// snapshot time up to the current time, and having things like jumpTime
// appear to be set for prediction runs where they previously weren't
// is a Bad Thing.  This is my bugfix for #166.

pmoveExt_t oldpmext[CMD_BACKUP];

void CG_PredictPlayerState(void) {
	int           cmdNum, current;
	playerState_t oldPlayerState;
	qboolean      moved;
	usercmd_t     oldestCmd;
	usercmd_t     latestCmd;
	vec3_t        deltaAngles;
	pmoveExt_t    pmext;

	cg.hyperspace = qfalse; // will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if (!cg.validPPS) {
		cg.validPPS             = qtrue;
		cg.predictedPlayerState = cg.snap->ps;
	}

	// demo playback just copies the moves
	if (cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW)) {
		CG_InterpolatePlayerState(qfalse);
		return;
	}

	// non-predicting local movement will grab the latest angles
	if (cg_nopredict.integer) {
		cg_pmove.ps    = &cg.predictedPlayerState;
		cg_pmove.pmext = &cg.pmext;

		cg.pmext.airleft = (cg.waterundertime - cg.time);

		// Arnout: are we using an mg42?
		if (cg_pmove.ps->eFlags & EF_MG42_ACTIVE || cg_pmove.ps->eFlags & EF_AAGUN_ACTIVE) {
			cg.pmext.harc = cg_entities[cg_pmove.ps->viewlocked_entNum].currentState.origin2[0];
			cg.pmext.varc = cg_entities[cg_pmove.ps->viewlocked_entNum].currentState.origin2[1];

			VectorCopy(cg_entities[cg_pmove.ps->viewlocked_entNum].currentState.angles2, cg.pmext.centerangles);

			cg.pmext.centerangles[PITCH] = AngleNormalize180(cg.pmext.centerangles[PITCH]);
			cg.pmext.centerangles[YAW]   = AngleNormalize180(cg.pmext.centerangles[YAW]);
			cg.pmext.centerangles[ROLL]  = AngleNormalize180(cg.pmext.centerangles[ROLL]);
		}

		CG_InterpolatePlayerState(qtrue);
		return;
	}

	if (cg_pmove.ps && cg_pmove.ps->eFlags & EF_MOUNTEDTANK) {
		centity_t *tank = &cg_entities[cg_entities[cg.snap->ps.clientNum].tagParent];

		cg.pmext.centerangles[YAW]   = tank->lerpAngles[YAW];
		cg.pmext.centerangles[PITCH] = tank->lerpAngles[PITCH];
	}

	// prepare for pmove
	cg_pmove.ps        = &cg.predictedPlayerState;
	cg_pmove.pmext     = &pmext;
	cg_pmove.character = CG_CharacterForClientinfo(&cgs.clientinfo[cg.snap->ps.clientNum], &cg_entities[cg.snap->ps.clientNum]);
	cg.pmext.airleft   = (cg.waterundertime - cg.time);

	// Arnout: are we using an mg42?
	if (cg_pmove.ps->eFlags & EF_MG42_ACTIVE || cg_pmove.ps->eFlags & EF_AAGUN_ACTIVE) {
		cg.pmext.harc = cg_entities[cg_pmove.ps->viewlocked_entNum].currentState.origin2[0];
		cg.pmext.varc = cg_entities[cg_pmove.ps->viewlocked_entNum].currentState.origin2[1];

		VectorCopy(cg_entities[cg_pmove.ps->viewlocked_entNum].currentState.angles2, cg.pmext.centerangles);

		cg.pmext.centerangles[PITCH] = AngleNormalize180(cg.pmext.centerangles[PITCH]);
		cg.pmext.centerangles[YAW]   = AngleNormalize180(cg.pmext.centerangles[YAW]);
		cg.pmext.centerangles[ROLL]  = AngleNormalize180(cg.pmext.centerangles[ROLL]);
	} else if (cg_pmove.ps->eFlags & EF_MOUNTEDTANK) {
		centity_t *tank = &cg_entities[cg_entities[cg.snap->ps.clientNum].tagParent];

		cg.pmext.centerangles[PITCH] = tank->lerpAngles[PITCH];
	}

	cg_pmove.trace = CG_TraceCapsuleNoPlayers;

	cg_pmove.pointcontents = CG_PointContents;

	cg_pmove.tracemask = MASK_PLAYERSOLID;
	if (cg_pmove.ps->pm_type == PM_DEAD) {
		cg_pmove.ps->eFlags |= EF_DEAD;
	} else if (cg_pmove.ps->pm_type == PM_SPECTATOR) {
		cg_pmove.trace = CG_TraceCapsule_World;
	}
	// Nico, end of ghost players

	cg_pmove.noFootsteps = qfalse;
	cg_pmove.noWeapClips = qfalse;

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.predictedPlayerState;

	current = trap_GetCurrentCmdNumber();

	// rain - fill in the current cmd with the latest prediction from
	// cg.pmext (#166)
	memcpy(&oldpmext[current & CMD_MASK], &cg.pmext, sizeof (pmoveExt_t));

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd(cmdNum, &oldestCmd);
	if (oldestCmd.serverTime > cg.snap->ps.commandTime
	    && oldestCmd.serverTime < cg.time) {    // special check for map_restart
		if (cg_showmiss.integer) {
			CG_Printf("exceeded PACKET_BACKUP on commands\n");
		}
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd(current, &latestCmd);

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to
	// be ahead of everything else anyway
	// rain - NEIN - this'll cause us to execute events from the next frame
	// early, resulting in doubled events and the like.  it seems to be
	// worse as far as prediction, too, so BLAH at id. (#405)

	cg.predictedPlayerState = cg.snap->ps;
	cg.physicsTime          = cg.snap->serverTime;

	if (pmove_msec.integer < 8) {
		trap_Cvar_Set("pmove_msec", "8");
	} else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
	}

	cg_pmove.pmove_fixed = pmove_fixed.integer; // | cg_pmove_fixed.integer;
	cg_pmove.pmove_msec  = pmove_msec.integer;

	// Nico, game physics
	cg_pmove.physics = physics.integer;

	cg_pmove.isTimerun        = isTimerun.integer;
	cg_pmove.timerunActive    = cg.timerunActive;
	cg_pmove.timerunStartTime = cg.timerunStartTime;

	// Nico, store login status in pmove
	cg_pmove.isLogged = cg.isLogged;

	// run cmds
	moved = qfalse;
	for (cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++) {
		// get the command
		trap_GetUserCmd(cmdNum, &cg_pmove.cmd);
		// get the previous command
		trap_GetUserCmd(cmdNum - 1, &cg_pmove.oldcmd);

		// don't do anything if the time is before the snapshot player time
		if (cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime) {
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if (cg_pmove.cmd.serverTime > latestCmd.serverTime) {
			continue;
		}

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if (cg.predictedPlayerState.commandTime == oldPlayerState.commandTime) {
			vec3_t delta;
			float  len;

			if (BG_PlayerMounted(cg_pmove.ps->eFlags)) {
				// no prediction errors here, we're locked in place
				VectorClear(cg.predictedError);
			} else if (cg.thisFrameTeleport) {
				// a teleport will not cause an error decay
				VectorClear(cg.predictedError);
				if (cg_showmiss.integer) {
					CG_Printf("PredictionTeleport\n");
				}
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t adjusted;
				CG_AdjustPositionForMover(cg.predictedPlayerState.origin, cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted, deltaAngles);
				// RF, add the deltaAngles (fixes jittery view while riding trains)
				// ydnar: only do this if player is prone or using set mortar
				if ((cg.predictedPlayerState.eFlags & EF_PRONE) || cg.weaponSelect == WP_MORTAR_SET) {
					cg.predictedPlayerState.delta_angles[YAW] += ANGLE2SHORT(deltaAngles[YAW]);
				}

				if (cg_showmiss.integer) {
					if (!VectorCompare(oldPlayerState.origin, adjusted)) {
						CG_Printf("prediction error\n");
					}
				}
				VectorSubtract(oldPlayerState.origin, adjusted, delta);
				len = VectorLength(delta);
				if (len > 0.1) {
					if (cg_showmiss.integer) {
						CG_Printf("Prediction miss: %f\n", len);
					}
					if (cg_errorDecay.integer) {
						int   t;
						float f;

						t = cg.time - cg.predictedErrorTime;
						f = (cg_errorDecay.value - t) / cg_errorDecay.value;
						if (f < 0) {
							f = 0;
						}
						if (f > 0 && cg_showmiss.integer) {
							CG_Printf("Double prediction decay: %f\n", f);
						}
						VectorScale(cg.predictedError, f, cg.predictedError);
					} else {
						VectorClear(cg.predictedError);
					}
					VectorAdd(delta, cg.predictedError, cg.predictedError);
					cg.predictedErrorTime = cg.oldTime;
				}
			}
		}

		// don't predict gauntlet firing, which is only supposed to happen
		// when it actually inflicts damage
		cg_pmove.gauntletHit = qfalse;

		if (cg_pmove.pmove_fixed) {
			cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + pmove_msec.integer - 1) / pmove_msec.integer) * pmove_msec.integer;
		}

		// ydnar: if server respawning, freeze the player
		if (cg.serverRespawning) {
			cg_pmove.ps->pm_type = PM_FREEZE;
		}

		// rain - only fill in the charge times if we're on a playing team
		if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_AXIS || cg.snap->ps.persistant[PERS_TEAM] == TEAM_ALLIES) {
			cg_pmove.ltChargeTime        = cg.ltChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
			cg_pmove.soldierChargeTime   = cg.soldierChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
			cg_pmove.engineerChargeTime  = cg.engineerChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
			cg_pmove.medicChargeTime     = cg.medicChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
			cg_pmove.covertopsChargeTime = cg.covertopsChargeTime[cg.snap->ps.persistant[PERS_TEAM] - 1];
		}

		// rain - copy the pmext as it was just before we
		// previously ran this cmd (or, this will be the
		// current predicted data if this is the current cmd)  (#166)
		memcpy(&pmext, &oldpmext[cmdNum & CMD_MASK], sizeof (pmoveExt_t));

		fflush(stdout);

		Pmove(&cg_pmove);

		moved = qtrue;

		// add push trigger movement effects
		CG_TouchTriggerPrediction();
	}

	if (cg_showmiss.integer > 1) {
		CG_Printf("[%i : %i] ", cg_pmove.cmd.serverTime, cg.time);
	}

	if (!moved) {
		if (cg_showmiss.integer) {
			CG_Printf("not moved\n");
		}
		return;
	}

	// restore pmext
	memcpy(&cg.pmext, &pmext, sizeof (pmoveExt_t));

	CG_AdjustPositionForMover(cg.predictedPlayerState.origin, cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.time, cg.predictedPlayerState.origin, deltaAngles);

	// fire events and other transition triggered things
	CG_TransitionPlayerState(&cg.predictedPlayerState, &oldPlayerState);

	// ydnar: shake player view here, rather than fiddle with view angles
	if (cg.time > cg.cameraShakeTime) {
		cg.cameraShakeScale = 0;
	} else {
		float x;


		// starts at 1, approaches 0 over time
		x = (cg.cameraShakeTime - cg.time) / cg.cameraShakeLength;

		// move
		cg.predictedPlayerState.origin[2] +=
		    sin(M_PI * 8 * 13 + cg.cameraShakePhase) * x * 6.0f * cg.cameraShakeScale;

		cg.predictedPlayerState.origin[1] +=
		    sin(M_PI * 17 * x + cg.cameraShakePhase) * x * 6.0f * cg.cameraShakeScale;

		cg.predictedPlayerState.origin[0] +=
		    cos(M_PI * 7 * x + cg.cameraShakePhase) * x * 6.0f * cg.cameraShakeScale;
	}
}
