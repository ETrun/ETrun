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
 * name:		cg_players.c
 *
 * desc:		handle the media and animation for player entities
 *
*/

#include "cg_local.h"

#define SWING_RIGHT 1
#define SWING_LEFT  2
#define SWINGSPEED  0.3

/*
================
CG_EntOnFire
================
*/
qboolean CG_EntOnFire(centity_t *cent) {
	if (cent->currentState.number == cg.snap->ps.clientNum) {
		// TAT 11/15/2002 - the player is always starting out on fire, which is easily seen in cinematics
		//		so make sure onFireStart is not 0
		return  (cg.snap->ps.onFireStart
		         && (cg.snap->ps.onFireStart < cg.time)
		         && (cg.snap->ps.onFireStart + 2000) > cg.time);
	}
	return  (cent->currentState.onFireStart < cg.time) && (cent->currentState.onFireEnd > cg.time);
}

/*
================
CG_IsCrouchingAnim
================
*/
qboolean CG_IsCrouchingAnim(animModelInfo_t *animModelInfo, int animNum) {
	animation_t *anim;

	// FIXME: make compatible with new scripting
	animNum &= ~ANIM_TOGGLEBIT;
	//
	anim = BG_GetAnimationForIndex(animModelInfo, animNum);
	//
	if (anim->movetype & ((1 << ANIM_MT_IDLECR) | (1 << ANIM_MT_WALKCR) | (1 << ANIM_MT_WALKCRBK))) {
		return qtrue;
	}
	//
	return qfalse;
}

/*
================
CG_CustomSound
================
*/
sfxHandle_t CG_CustomSound(const char *soundName) {
	if (soundName[0] != '*') {
		return trap_S_RegisterSound(soundName);
	}

	return 0;
}

/*
=============================================================================

CLIENT INFO

=============================================================================
*/

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo(int clientNum) {
	int i;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	for (i = 0 ; i < MAX_GENTITIES ; i++) {
		if (cg_entities[i].currentState.clientNum == clientNum && cg_entities[i].currentState.eType == ET_PLAYER) {
			CG_ResetPlayerEntity(&cg_entities[i]);
		}
	}
}

void CG_LimboPanel_SendSetupMsg(qboolean forceteam);

/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo(int clientNum) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char   *configstring;
	const char   *v;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString(clientNum + CS_PLAYERS);
	if (!*configstring) {
		memset(ci, 0, sizeof (*ci));
		return;     // player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset(&newInfo, 0, sizeof (newInfo));

	// Gordon: grabbing some older stuff, if it's a new client, tinfo will update within one second anyway, otherwise you get the health thing flashing red
	// NOTE: why are we bothering to do all this setting up of a new clientInfo_t anyway? it was all for deffered clients iirc, which we dont have
	newInfo.location[0]  = ci->location[0];
	newInfo.location[1]  = ci->location[1];
	newInfo.health       = ci->health;
	newInfo.fireteamData = ci->fireteamData;
	newInfo.clientNum    = clientNum;
	newInfo.selected     = ci->selected;

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz(newInfo.name, v, sizeof (newInfo.name));
	Q_strncpyz(newInfo.cleanname, v, sizeof (newInfo.cleanname));
	Q_CleanStr(newInfo.cleanname);

	// team
	v            = Info_ValueForKey(configstring, "t");
	newInfo.team = atoi(v);

	// class
	v           = Info_ValueForKey(configstring, "c");
	newInfo.cls = atoi(v);

	// rank
	v            = Info_ValueForKey(configstring, "r");
	newInfo.rank = atoi(v);

	v                = Info_ValueForKey(configstring, "f");
	newInfo.fireteam = atoi(v);

	v = Info_ValueForKey(configstring, "ch");
	if (*v) {
		newInfo.character = cgs.gameCharacters[atoi(v)];
	}

	// Gordon: weapon and latchedweapon ( FIXME: make these more secure )
	v              = Info_ValueForKey(configstring, "w");
	newInfo.weapon = atoi(v);

	v                     = Info_ValueForKey(configstring, "lw");
	newInfo.latchedweapon = atoi(v);

	v                       = Info_ValueForKey(configstring, "sw");
	newInfo.secondaryweapon = atoi(v);

	v                 = Info_ValueForKey(configstring, "ref");
	newInfo.refStatus = atoi(v);

	// Nico, pmove_fixed
	v                  = Info_ValueForKey(configstring, "pm");
	newInfo.pmoveFixed = atoi(v);

	// Nico, login status
	v              = Info_ValueForKey(configstring, "l");
	newInfo.logged = (atoi(v) == 1) ? qtrue : qfalse;

	// Nico, hideme
	v              = Info_ValueForKey(configstring, "h");
	newInfo.hideme = (atoi(v) == 1) ? qtrue : qfalse;

	// Nico, country code
	v                  = Info_ValueForKey(configstring, "cc");
	newInfo.countryCode = atoi(v);

	// Gordon: detect rank/skill changes client side
	if (clientNum == cg.clientNum) {
		trap_Cvar_Set("authLevel", va("%i", newInfo.refStatus));

		if (newInfo.refStatus != ci->refStatus) {
			if (newInfo.refStatus <= RL_NONE) {
				const char *info = CG_ConfigString(CS_SERVERINFO);

				trap_Cvar_Set("cg_ui_voteFlags", Info_ValueForKey(info, "voteFlags"));
				CG_Printf("[cgnotify]^3*** You have been stripped of your referee status! ***\n");

			} else {
				trap_Cvar_Set("cg_ui_voteFlags", "0");
				CG_Printf("[cgnotify]^2*** You have been authorized \"%s\" status ***\n", ((newInfo.refStatus == RL_RCON) ? "rcon" : "referee"));
				CG_Printf("Type: ^3ref^7 (by itself) for a list of referee commands.\n");
			}
		}
	}


	// rain - passing the clientNum since that's all we need, and we
	// can't calculate it properly from the clientinfo
	CG_LoadClientInfo(clientNum);

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci               = newInfo;

	// make sure we have a character set
	if (!ci->character) {
		ci->character = BG_GetCharacter(ci->team, ci->cls);
	}

	// Gordon: need to resort the fireteam list, incase ranks etc have changed
	CG_SortClientFireteam();
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/

/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetLerpFrameAnimation(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation) {
	animation_t *anim;

	bg_character_t *character = CG_CharacterForClientinfo(ci, cent);

	if (!character) {
		return;
	}

	lf->animationNumber = newAnimation;
	newAnimation       &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= character->animModelInfo->numAnimations) {
		CG_Error("CG_SetLerpFrameAnimation: Bad animation number: %i", newAnimation);
	}

	anim = character->animModelInfo->animations[newAnimation];

	lf->animation     = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if (cg_debugAnim.integer == 1) {
		CG_Printf("Anim: %i, %s\n", newAnimation, character->animModelInfo->animations[newAnimation]->name);
	}
}

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
void CG_RunLerpFrame(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale) {
	int         f;
	animation_t *anim;

	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if (ci && (newAnimation != lf->animationNumber || !lf->animation)) {
		CG_SetLerpFrameAnimation(cent, ci, lf, newAnimation);
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime) {
		lf->oldFrame      = lf->frame;
		lf->oldFrameTime  = lf->frameTime;
		lf->oldFrameModel = lf->frameModel;

		// get the next frame based on the animation
		anim = lf->animation;
		if (!anim->frameLerp) {
			return;     // shouldn't happen
		}
		if (cg.time < lf->animationTime) {
			lf->frameTime = lf->animationTime;      // initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f  = (lf->frameTime - lf->animationTime) / anim->frameLerp;
		f *= speedScale;        // adjust for haste, etc
		if (f >= anim->numFrames) {
			f -= anim->numFrames;
			if (anim->loopFrames) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame      = anim->firstFrame + f;
		lf->frameModel = anim->mdxFile;

		if (cg.time > lf->frameTime) {
			lf->frameTime = cg.time;
			if (cg_debugAnim.integer) {
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
	}

	if (lf->frameTime > cg.time + 200) {
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}


/*
===============
CG_ClearLerpFrame
===============
*/
static void CG_ClearLerpFrame(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int animationNumber) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation(cent, ci, lf, animationNumber);
	if (lf->animation) {
		lf->oldFrame      = lf->frame = lf->animation->firstFrame;
		lf->oldFrameModel = lf->frameModel = lf->animation->mdxFile;
	}
}

/*
===============
CG_SetLerpFrameAnimationRate

may include ANIM_TOGGLEBIT
===============
*/
void CG_SetLerpFrameAnimationRate(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation) {
	animation_t *anim, *oldanim;
	int         transitionMin = -1;
	int         oldAnimNum;
	qboolean    firstAnim = qfalse;

	bg_character_t *character = CG_CharacterForClientinfo(ci, cent);

	if (!character) {
		return;
	}

	oldanim    = lf->animation;
	oldAnimNum = lf->animationNumber;

	if (!lf->animation) {
		firstAnim = qtrue;
	}

	lf->animationNumber = newAnimation;
	newAnimation       &= ~ANIM_TOGGLEBIT;

	if (newAnimation < 0 || newAnimation >= character->animModelInfo->numAnimations) {
		CG_Error("CG_SetLerpFrameAnimationRate: Bad animation number: %i", newAnimation);
	}

	anim = character->animModelInfo->animations[newAnimation];

	lf->animation     = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if (!(anim->flags & ANIMFL_FIRINGANIM) || (lf != &cent->pe.torso)) {
		if ((lf == &cent->pe.legs) && (CG_IsCrouchingAnim(character->animModelInfo, newAnimation) != CG_IsCrouchingAnim(character->animModelInfo, oldAnimNum))) {
			if (anim->moveSpeed || (anim->movetype & ((1 << ANIM_MT_TURNLEFT) | (1 << ANIM_MT_TURNRIGHT)))) {           // if unknown movetype, go there faster
				transitionMin = lf->frameTime + 200;    // slowly raise/drop
			} else {
				transitionMin = lf->frameTime + 350;    // slowly raise/drop
			}
		} else if (anim->moveSpeed) {
			transitionMin = lf->frameTime + 120;    // always do some lerping (?)
		} else {   // not moving, so take your time
			transitionMin = lf->frameTime + 170;    // always do some lerping (?)

		}
		if (oldanim && oldanim->animBlend) {   //transitionMin < lf->frameTime + oldanim->animBlend) {
			transitionMin     = lf->frameTime + oldanim->animBlend;
			lf->animationTime = transitionMin;
		} else {
			// slow down transitions according to speed
			if (anim->moveSpeed && lf->animSpeedScale < 1.0) {
				lf->animationTime += anim->initialLerp;
			}

			if (lf->animationTime < transitionMin) {
				lf->animationTime = transitionMin;
			}
		}
	}

	// if first anim, go immediately
	if (firstAnim) {
		lf->frameTime     = cg.time - 1;
		lf->animationTime = cg.time - 1;
		lf->frame         = anim->firstFrame;
		lf->frameModel    = anim->mdxFile;
	}

	if (cg_debugAnim.integer == 1) {             // DHM - Nerve :: extra debug info
		CG_Printf("Anim: %i, %s\n", newAnimation, character->animModelInfo->animations[newAnimation]->name);
	}
}

/*
===============
CG_RunLerpFrameRate

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
void CG_RunLerpFrameRate(clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, centity_t *cent, int recursion) {
	int         f;
	animation_t *anim, *oldAnim;
	animation_t *otherAnim = NULL;
	qboolean    isLadderAnim;

#define ANIM_SCALEMAX_LOW   1.1f
#define ANIM_SCALEMAX_HIGH  1.6f

#define ANIM_SPEEDMAX_LOW   100
#define ANIM_SPEEDMAX_HIGH  20

	// debugging tool to get no animations
	if (cg_animSpeed.integer == 0) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	isLadderAnim = lf->animation && (lf->animation->flags & ANIMFL_LADDERANIM);

	oldAnim = lf->animation;

	// see if the animation sequence is switching
	if (newAnimation != lf->animationNumber || !lf->animation) {
		CG_SetLerpFrameAnimationRate(cent, ci, lf, newAnimation);
	}

	// Ridah, make sure the animation speed is updated when possible
	anim = lf->animation;

	// check for forcing last frame
	if (cent->currentState.eFlags & EF_FORCE_END_FRAME
	    // xkan, 12/27/2002 - In SP, corpse also stays at the last frame (of the death animation)
	    // so that the death animation can end up in different positions
	    // and the body will stay in that position
	    || (cent->currentState.eType == ET_CORPSE)) {
		lf->oldFrame      = lf->frame = anim->firstFrame + anim->numFrames - 1;
		lf->oldFrameModel = lf->frameModel = anim->mdxFile;
		lf->backlerp      = 0;
		return;
	}

	if (anim->moveSpeed && lf->oldFrameSnapshotTime) {
		float moveSpeed;

		// calculate the speed at which we moved over the last frame
		if (cg.latestSnapshotTime != lf->oldFrameSnapshotTime && cg.nextSnap) {
			if (cent->currentState.number == cg.snap->ps.clientNum) {
				if (isLadderAnim) {   // only use Z axis for speed
					lf->oldFramePos[0] = cent->lerpOrigin[0];
					lf->oldFramePos[1] = cent->lerpOrigin[1];
				} else {      // only use x/y axis
					lf->oldFramePos[2] = cent->lerpOrigin[2];
				}
				moveSpeed = Distance(cent->lerpOrigin, lf->oldFramePos) / ((float)(cg.time - lf->oldFrameTime) / 1000.0);
			} else {
				if (isLadderAnim) {   // only use Z axis for speed
					lf->oldFramePos[0] = cent->currentState.pos.trBase[0];
					lf->oldFramePos[1] = cent->currentState.pos.trBase[1];
				}
				moveSpeed = Distance(cent->lerpOrigin, lf->oldFramePos) / ((float)(cg.time - lf->oldFrameTime) / 1000.0);
			}
			//
			// convert it to a factor of this animation's movespeed
			lf->animSpeedScale       = moveSpeed / (float)anim->moveSpeed;
			lf->oldFrameSnapshotTime = cg.latestSnapshotTime;
		}
	} else {
		// move at normal speed
		lf->animSpeedScale       = 1.0;
		lf->oldFrameSnapshotTime = cg.latestSnapshotTime;
	}
	// adjust with manual setting (pain anims)
	lf->animSpeedScale *= cent->pe.animSpeed;

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if (cg.time >= lf->frameTime) {
		lf->oldFrame      = lf->frame;
		lf->oldFrameTime  = lf->frameTime;
		lf->oldFrameModel = lf->frameModel;
		VectorCopy(cent->lerpOrigin, lf->oldFramePos);

		// restrict the speed range
		if (lf->animSpeedScale < 0.25) {     // if it's too slow, then a really slow spped, combined with a sudden take-off, can leave them playing a really slow frame while they a moving really fast
			if (lf->animSpeedScale < 0.01 && isLadderAnim) {
				lf->animSpeedScale = 0.0;
			} else {
				lf->animSpeedScale = 0.25;
			}
		} else if (lf->animSpeedScale > ANIM_SCALEMAX_LOW) {

			if (!(anim->flags & ANIMFL_LADDERANIM)) {
				// allow slower anims to speed up more than faster anims
				if (anim->moveSpeed > ANIM_SPEEDMAX_LOW) {
					lf->animSpeedScale = ANIM_SCALEMAX_LOW;
				} else if (anim->moveSpeed < ANIM_SPEEDMAX_HIGH) {
					if (lf->animSpeedScale > ANIM_SCALEMAX_HIGH) {
						lf->animSpeedScale = ANIM_SCALEMAX_HIGH;
					}
				} else {
					lf->animSpeedScale = ANIM_SCALEMAX_HIGH - (ANIM_SCALEMAX_HIGH - ANIM_SCALEMAX_LOW) * (float)(anim->moveSpeed - ANIM_SPEEDMAX_HIGH) / (float)(ANIM_SPEEDMAX_LOW - ANIM_SPEEDMAX_HIGH);
				}
			} else if (lf->animSpeedScale > 4.0) {
				lf->animSpeedScale = 4.0;
			}

		}

		if (lf == &cent->pe.legs) {
			otherAnim = cent->pe.torso.animation;
		} else if (lf == &cent->pe.torso) {
			otherAnim = cent->pe.legs.animation;
		}

		// get the next frame based on the animation
		if (!lf->animSpeedScale) {
			// stopped on the ladder, so stay on the same frame
			f              = lf->frame - anim->firstFrame;
			lf->frameTime += anim->frameLerp;       // don't wait too long before starting to move again
		} else if (lf->oldAnimationNumber != lf->animationNumber &&
		           (!anim->moveSpeed || lf->oldFrame < anim->firstFrame || lf->oldFrame >= anim->firstFrame + anim->numFrames)) {     // Ridah, added this so walking frames don't always get reset to 0, which can happen in the middle of a walking anim, which looks wierd
			lf->frameTime = lf->animationTime;      // initial lerp
			if (oldAnim && anim->moveSpeed) {     // keep locomotions going continuously
				f = (lf->frame - oldAnim->firstFrame) + 1;
				while (f < 0) {
					f += anim->numFrames;
				}
			} else {
				f = 0;
			}
		} else if ((lf == &cent->pe.legs) && otherAnim && !(anim->flags & ANIMFL_FIRINGANIM) && ((lf->animationNumber & ~ANIM_TOGGLEBIT) == (cent->pe.torso.animationNumber & ~ANIM_TOGGLEBIT)) && (!anim->moveSpeed)) {
			// legs should synch with torso
			f = cent->pe.torso.frame - otherAnim->firstFrame;
			if (f >= anim->numFrames || f < 0) {
				f = 0;  // wait at the start for the legs to catch up (assuming they are still in an old anim)
			}
			lf->frameTime = cent->pe.torso.frameTime;
		} else if ((lf == &cent->pe.torso) && otherAnim && !(anim->flags & ANIMFL_FIRINGANIM) && ((lf->animationNumber & ~ANIM_TOGGLEBIT) == (cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT)) && (otherAnim->moveSpeed)) {
			// torso needs to sync with legs
			f = cent->pe.legs.frame - otherAnim->firstFrame;
			if (f >= anim->numFrames || f < 0) {
				f = 0;  // wait at the start for the legs to catch up (assuming they are still in an old anim)
			}
			lf->frameTime = cent->pe.legs.frameTime;
		} else {
			lf->frameTime = lf->oldFrameTime + (int)((float)anim->frameLerp * (1.0 / lf->animSpeedScale));
			if (lf->frameTime < cg.time) {
				lf->frameTime = cg.time;
			}

			// check for skipping frames (eg. death anims play in slo-mo if low framerate)
			if (anim->flags & ANIMFL_REVERSED) {
				if (cg.time > lf->frameTime && !anim->moveSpeed) {
					f = (anim->numFrames - 1) - ((lf->frame - anim->firstFrame) - (1 + (cg.time - lf->frameTime) / anim->frameLerp));
				} else {
					f = (anim->numFrames - 1) - ((lf->frame - anim->firstFrame) - 1);
				}
			} else {
				if (cg.time > lf->frameTime && !anim->moveSpeed) {
					f = (lf->frame - anim->firstFrame) + 1 + (cg.time - lf->frameTime) / anim->frameLerp;
				} else {
					f = (lf->frame - anim->firstFrame) + 1;
				}
			}

			if (f < 0) {
				f = 0;
			}
		}
		//f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		if (f >= anim->numFrames) {
			f -= anim->numFrames;
			if (anim->loopFrames) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		if (anim->flags & ANIMFL_REVERSED) {
			lf->frame      = anim->firstFrame + anim->numFrames - 1 - f;
			lf->frameModel = anim->mdxFile;
		} else {
			lf->frame      = anim->firstFrame + f;
			lf->frameModel = anim->mdxFile;
		}

		if (cg.time > lf->frameTime) {

			// Ridah, run the frame again until we move ahead of the current time, fixes walking speeds for zombie
			if (/*!anim->moveSpeed ||*/ recursion > 4) {
				lf->frameTime = cg.time;
			} else {
				CG_RunLerpFrameRate(ci, lf, newAnimation, cent, recursion + 1);
			}

			if (cg_debugAnim.integer > 3) {
				CG_Printf("Clamp lf->frameTime\n");
			}
		}
		lf->oldAnimationNumber = lf->animationNumber;
	}

	// Gordon: BIG hack, occaisionaly (VERY occaisionally), the frametime gets totally wacked
	if (lf->frameTime > cg.time + 5000) {
		lf->frameTime = cg.time;
	}

	if (lf->oldFrameTime > cg.time) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if (lf->frameTime == lf->oldFrameTime) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)(cg.time - lf->oldFrameTime) / (lf->frameTime - lf->oldFrameTime);
	}
}

/*
===============
CG_ClearLerpFrameRate
===============
*/
void CG_ClearLerpFrameRate(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int animationNumber) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimationRate(cent, ci, lf, animationNumber);
	if (lf->animation) {
		lf->oldFrame      = lf->frame = lf->animation->firstFrame;
		lf->oldFrameModel = lf->frameModel = lf->animation->mdxFile;
	}
}

/*
===============
CG_PlayerAnimation
===============
*/
static void CG_PlayerAnimation(centity_t *cent, refEntity_t *body) {
	clientInfo_t   *ci;
	int            clientNum;
	int            animIndex, tempIndex;
	bg_character_t *character;

	clientNum = cent->currentState.clientNum;

	ci        = &cgs.clientinfo[clientNum];
	character = CG_CharacterForClientinfo(ci, cent);

	if (!character) {
		return;
	}

	if (cg_noPlayerAnims.integer) {
		body->frame      = body->oldframe = body->torsoFrame = body->oldTorsoFrame = 0;
		body->frameModel = body->oldframeModel = body->torsoFrameModel = body->oldTorsoFrameModel = character->animModelInfo->animations[0]->mdxFile;
		return;
	}

	// default to whatever the legs are currently doing
	animIndex = cent->currentState.legsAnim;

	// do the shuffle turn frames locally
	if (!(cent->currentState.eFlags & EF_DEAD) && cent->pe.legs.yawing) {
		//CG_Printf("turn: %i\n", cg.time );
		tempIndex = BG_GetAnimScriptAnimation(clientNum, character->animModelInfo, cent->currentState.aiState, (cent->pe.legs.yawing == SWING_RIGHT ? ANIM_MT_TURNRIGHT : ANIM_MT_TURNLEFT));
		if (tempIndex > -1) {
			animIndex = tempIndex;
		}
	}
	// run the animation
	CG_RunLerpFrameRate(ci, &cent->pe.legs, animIndex, cent, 0);

	body->oldframe      = cent->pe.legs.oldFrame;
	body->frame         = cent->pe.legs.frame;
	body->backlerp      = cent->pe.legs.backlerp;
	body->frameModel    = cent->pe.legs.frameModel;
	body->oldframeModel = cent->pe.legs.oldFrameModel;

	CG_RunLerpFrameRate(ci, &cent->pe.torso, cent->currentState.torsoAnim, cent, 0);

	body->oldTorsoFrame      = cent->pe.torso.oldFrame;
	body->torsoFrame         = cent->pe.torso.frame;
	body->torsoBacklerp      = cent->pe.torso.backlerp;
	body->torsoFrameModel    = cent->pe.torso.frameModel;
	body->oldTorsoFrameModel = cent->pe.torso.oldFrameModel;
}

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles(float destination, float swingTolerance, float clampTolerance,
                           float speed, float *angle, qboolean *swinging) {
	float swing;
	float move;
	float scale;

	if (!*swinging) {
		// see if a swing should be started
		swing = AngleSubtract(*angle, destination);
		if (swing > swingTolerance || swing < -swingTolerance) {
			*swinging = qtrue;
		}
	}

	if (!*swinging) {
		return;
	}

	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing  = AngleSubtract(destination, *angle);
	scale  = fabs(swing);
	scale *= 0.05f;
	if (scale < 0.5) {
		scale = 0.5;
	}

	// swing towards the destination angle
	if (swing >= 0) {
		move = cg.frametime * scale * speed;
		if (move >= swing) {
			move      = swing;
			*swinging = qfalse;
		} else {
			*swinging = SWING_LEFT;     // left
		}
		*angle = AngleMod(*angle + move);
	} else if (swing < 0) {
		move = cg.frametime * scale * -speed;
		if (move <= swing) {
			move      = swing;
			*swinging = qfalse;
		} else {
			*swinging = SWING_RIGHT;    // right
		}
		*angle = AngleMod(*angle + move);
	}

	// clamp to no more than tolerance
	swing = AngleSubtract(destination, *angle);
	if (swing > clampTolerance) {
		*angle = AngleMod(destination - (clampTolerance - 1));
	} else if (swing < -clampTolerance) {
		*angle = AngleMod(destination + (clampTolerance - 1));
	}
}

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch(centity_t *cent, vec3_t torsoAngles) {
	int   t;
	float f;
	int   duration;

	if (!cent->pe.animSpeed) {
		// we need to inititialize this stuff
		cent->pe.painAnimLegs  = -1;
		cent->pe.painAnimTorso = -1;
		cent->pe.animSpeed     = 1.0;
	}

	if (cent->currentState.eFlags & EF_DEAD) {
		cent->pe.painAnimLegs  = -1;
		cent->pe.painAnimTorso = -1;
		cent->pe.animSpeed     = 1.0;
		return;
	}

	if (cent->pe.painDuration) {
		duration = cent->pe.painDuration;
	} else {
		duration = PAIN_TWITCH_TIME;
	}

	t = cg.time - cent->pe.painTime;
	if (t >= duration) {
		return;
	}

	f = 1.0 - (float)t / duration;
	if (cent->pe.painDirection) {
		torsoAngles[ROLL] += 20 * f;
	} else {
		torsoAngles[ROLL] -= 20 * f;
	}
}

/*
===============
CG_PlayerAngles

Handles seperate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
static void CG_PlayerAngles(centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3]) {
	vec3_t         legsAngles, torsoAngles, headAngles;
	float          dest;
	vec3_t         velocity;
	float          speed;
	float          clampTolerance;
	int            legsSet;
	clientInfo_t   *ci;
	bg_character_t *character;

	ci = &cgs.clientinfo[cent->currentState.clientNum];

	character = CG_CharacterForClientinfo(ci, cent);

	if (!character) {
		return;
	}

	legsSet = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;

	VectorCopy(cent->lerpAngles, headAngles);
	headAngles[YAW] = AngleMod(headAngles[YAW]);
	VectorClear(legsAngles);
	VectorClear(torsoAngles);

	// --------- yaw -------------

	// allow yaw to drift a bit, unless these conditions don't allow them
	if (!(BG_GetConditionBitFlag(cent->currentState.clientNum, ANIM_COND_MOVETYPE, ANIM_MT_IDLE) ||
	      BG_GetConditionBitFlag(cent->currentState.clientNum, ANIM_COND_MOVETYPE, ANIM_MT_IDLECR))) {

		// always point all in the same direction
		cent->pe.torso.yawing   = qtrue; // always center
		cent->pe.torso.pitching = qtrue;    // always center
		cent->pe.legs.yawing    = qtrue; // always center

		// if firing, make sure torso and head are always aligned
	} else if (BG_GetConditionValue(cent->currentState.clientNum, ANIM_COND_FIRING, qtrue)) {
		cent->pe.torso.yawing   = qtrue; // always center
		cent->pe.torso.pitching = qtrue;    // always center
	}

	// adjust legs for movement dir
	if (cent->currentState.eFlags & EF_DEAD || cent->currentState.eFlags & EF_MOUNTEDTANK) {
		// don't let dead bodies twitch
		legsAngles[YAW] = headAngles[YAW];
	} else {
		legsAngles[YAW] = headAngles[YAW] + cent->currentState.angles2[YAW];

		if (!(cent->currentState.eFlags & EF_FIRING)) {
			torsoAngles[YAW] = headAngles[YAW] + 0.35 * cent->currentState.angles2[YAW];
			clampTolerance   = 90;
		} else {      // must be firing
			torsoAngles[YAW] = headAngles[YAW]; // always face firing direction
			clampTolerance   = 60;
		}

		// torso
		CG_SwingAngles(torsoAngles[YAW], 25, clampTolerance, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing);

		// if the legs are yawing (facing heading direction), allow them to rotate a bit, so we don't keep calling
		// the legs_turn animation while an AI is firing, and therefore his angles will be randomizing according to their accuracy

		clampTolerance = 150;

		if (BG_GetConditionBitFlag(ci->clientNum, ANIM_COND_MOVETYPE, ANIM_MT_IDLE)) {
			cent->pe.legs.yawing = qfalse; // set it if they really need to swing
			CG_SwingAngles(legsAngles[YAW], 20, clampTolerance, 0.5 * cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);
		} else if (strstr(BG_GetAnimString(character->animModelInfo, legsSet), "strafe")) {
			cent->pe.legs.yawing = qfalse; // set it if they really need to swing
			legsAngles[YAW]      = headAngles[YAW];
			CG_SwingAngles(legsAngles[YAW], 0, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);
		} else if (cent->pe.legs.yawing) {
			CG_SwingAngles(legsAngles[YAW], 0, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);
		} else {
			CG_SwingAngles(legsAngles[YAW], 40, clampTolerance, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing);
		}

		torsoAngles[YAW] = cent->pe.torso.yawAngle;
		legsAngles[YAW]  = cent->pe.legs.yawAngle;
	}

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if (headAngles[PITCH] > 180) {
		dest = (-360 + headAngles[PITCH]) * 0.75;
	} else {
		dest = headAngles[PITCH] * 0.75;
	}

	if (cent->currentState.eFlags & EF_PRONE) {
		torsoAngles[PITCH] = legsAngles[PITCH] - 3;
	} else {
		CG_SwingAngles(dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching);
		torsoAngles[PITCH] = cent->pe.torso.pitchAngle;
	}


	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy(cent->currentState.pos.trDelta, velocity);
	speed = VectorNormalize(velocity);
	if (speed) {
		vec3_t axis[3];
		float  side;

		speed *= 0.05f;

		AnglesToAxis(legsAngles, axis);
		side              = speed * DotProduct(velocity, axis[1]);
		legsAngles[ROLL] -= side;

		side               = speed * DotProduct(velocity, axis[0]);
		legsAngles[PITCH] += side;
	}

	// pain twitch
	CG_AddPainTwitch(cent, torsoAngles);

	// pull the angles back out of the hierarchial chain
	AnglesSubtract(headAngles, torsoAngles, headAngles);
	AnglesSubtract(torsoAngles, legsAngles, torsoAngles);
	AnglesToAxis(legsAngles, legs);
	AnglesToAxis(torsoAngles, torso);
	AnglesToAxis(headAngles, head);
}

/*
==============
CG_BreathPuffs
==============
*/
static void CG_BreathPuffs(centity_t *cent, refEntity_t *head) {
	clientInfo_t *ci;
	vec3_t       up, forward;
	int          contents;
	vec3_t       mang, morg, maxis[3];

	ci = &cgs.clientinfo[cent->currentState.number];

	if (!cg_enableBreath.integer) {
		return;
	}

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		return;
	}

	if (!(cent->currentState.eFlags & EF_DEAD)) {
		return;
	}

	// allow cg_enableBreath to force everyone to have breath
	if (!(cent->currentState.eFlags & EF_BREATH)) {
		return;
	}

	contents = CG_PointContents(head->origin, 0);
	if (contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		return;
	}
	if (ci->breathPuffTime > cg.time) {
		return;
	}

	CG_GetOriginForTag(head, "tag_mouth", 0, morg, maxis);
	AxisToAngles(maxis, mang);
	AngleVectors(mang, forward, NULL, up);

	//push the origin out a tad so it's not right in the guys face (tad==4)
	VectorMA(morg, 4, forward, morg);

	forward[0] = up[0] * 8 + forward[0] * 5;
	forward[1] = up[1] * 8 + forward[1] * 5;
	forward[2] = up[2] * 8 + forward[2] * 5;

	CG_SmokePuff(morg, forward, 4, 1, 1, 1, 0.5f, 2000, cg.time, cg.time + 400, 0, cgs.media.shotgunSmokePuffShader);

	ci->breathPuffTime = cg.time + 3000 + random() * 1000;
}

/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
DHM - Nerve :: added height parameter
===============
*/
static void CG_PlayerFloatSprite(centity_t *cent, qhandle_t shader, int height) {
	int         rf;
	refEntity_t ent;

	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		rf = RF_THIRD_PERSON;       // only show in mirrors
	} else {
		rf = 0;
	}

	memset(&ent, 0, sizeof (ent));
	VectorCopy(cent->lerpOrigin, ent.origin);
	ent.origin[2] += height;            // DHM - Nerve :: was '48'

	// Account for ducking
	if (cent->currentState.clientNum == cg.snap->ps.clientNum) {
		if (cg.snap->ps.pm_flags & PMF_DUCKED) {
			ent.origin[2] -= 18;
		}
	} else {
		if ((qboolean)cent->currentState.animMovetype) {
			ent.origin[2] -= 18;
		}
	}

	ent.reType        = RT_SPRITE;
	ent.customShader  = shader;
	ent.radius        = 6.66f;
	ent.renderfx      = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene(&ent);
}



/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites(centity_t *cent) {
	int team;

	if (cent->currentState.powerups & (1 << PW_REDFLAG) ||
	    cent->currentState.powerups & (1 << PW_BLUEFLAG)) {
		CG_PlayerFloatSprite(cent, cgs.media.objectiveShader, 56);
		return;
	}

	if (cent->currentState.eFlags & EF_CONNECTION) {
		CG_PlayerFloatSprite(cent, cgs.media.disconnectIcon, 48);
		return;
	}

	if (cent->currentState.powerups & (1 << PW_INVULNERABLE)) {
		CG_PlayerFloatSprite(cent, cgs.media.spawnInvincibleShader, 56);
		return;
	}

	team = cgs.clientinfo[cent->currentState.clientNum].team;

	// DHM - Nerve :: show voice chat signal so players know who's talking
	if (cent->voiceChatSpriteTime > cg.time && cg.snap->ps.persistant[PERS_TEAM] == team) {
		CG_PlayerFloatSprite(cent, cent->voiceChatSprite, 56);
		return;
	}

	// DHM - Nerve :: only show talk icon to team-mates
	if (cent->currentState.eFlags & EF_TALK && cg.snap->ps.persistant[PERS_TEAM] == team) {
		CG_PlayerFloatSprite(cent, cgs.media.balloonShader, 48);
		return;
	}

	{
		fireteamData_t *ft = CG_IsOnFireteam(cent->currentState.number);
		if (ft) {
			if (ft == CG_IsOnFireteam(cg.clientNum) && cgs.clientinfo[cent->currentState.number].selected) {
				CG_PlayerFloatSprite(cent, cgs.media.fireteamicons[ft->ident], 56);
			}
		}
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define SHADOW_DISTANCE     64
#define ZOFS    6.0
#define SHADOW_MIN_DIST 250.0
#define SHADOW_MAX_DIST 512.0

typedef struct {
	char *tagname;
	float size;
	qhandle_t shader;
} shadowPart_t;

static qboolean CG_PlayerShadow(centity_t *cent, float *shadowPlane) {
	vec3_t       end;
	trace_t      trace;
	float        dist, distFade;
	int          tagIndex, subIndex;
	vec3_t       origin, angles, axis[3];
	vec4_t       projection    = { 0, 0, -1, 64 };
	shadowPart_t shadowParts[] =
	{
		{ "tag_footleft",  10, 0 },
		{ "tag_footright", 10, 0 },
		{ "tag_torso",     18, 0 },
		{ NULL,            0,  0 }
	};

	shadowParts[0].shader = cgs.media.shadowFootShader;     //DAJ pulled out of initliization
	shadowParts[1].shader = cgs.media.shadowFootShader;
	shadowParts[2].shader = cgs.media.shadowTorsoShader;

	*shadowPlane = 0;

	if (cg_shadows.integer == 0) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy(cent->lerpOrigin, end);
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace(&trace, cent->lerpOrigin, end, NULL, NULL, 0, MASK_PLAYERSOLID);

	*shadowPlane = trace.endpos[2] + 1;

	if (cg_shadows.integer != 1) {      // no mark for stencil or projection shadows
		return qtrue;
	}

	// no shadows when dead
	if (cent->currentState.eFlags & EF_DEAD) {
		return qfalse;
	}

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
	dist     = VectorDistance(cent->lerpOrigin, cg.refdef_current->vieworg); //%	cg.snap->ps.origin );
	distFade = 1.0f;
	if (!(cent->currentState.eFlags & EF_ZOOMING) && (dist > SHADOW_MIN_DIST)) {
		if (dist > SHADOW_MAX_DIST) {
			if (dist > SHADOW_MAX_DIST * 2) {
				return qfalse;
			} else {   // fade out
				distFade = 1.0f - ((dist - SHADOW_MAX_DIST) / SHADOW_MAX_DIST);
			}

			if (distFade > 1.0f) {
				distFade = 1.0f;
			} else if (distFade < 0.0f) {
				distFade = 0.0f;
			}
		}

		// set origin
		VectorCopy(cent->lerpOrigin, origin);

		// project it onto the shadow plane
		if (origin[2] < *shadowPlane) {
			origin[2] = *shadowPlane;
		}

		// ydnar: add a bit of height so foot shadows don't clip into sloped geometry as much
		origin[2] += 18.0f;

		//%	alpha *= distFade;

		// ydnar: decal remix
		//%	CG_ImpactMark( cgs.media.shadowTorsoShader, trace.endpos, trace.plane.normal,
		//%		0, alpha,alpha,alpha,1, qfalse, 16, qtrue, -1 );
		CG_ImpactMark(cgs.media.shadowTorsoShader, origin, projection, 18.0f,
		              cent->lerpAngles[YAW], distFade, distFade, distFade, distFade, -1);
		return qtrue;
	}

	if (dist < SHADOW_MAX_DIST) {     // show more detail
		// now add shadows for the various body parts
		for (tagIndex = 0; shadowParts[tagIndex].tagname; tagIndex++) {
			// grab each tag with this name
			for (subIndex = 0; (subIndex = CG_GetOriginForTag(&cent->pe.bodyRefEnt, shadowParts[tagIndex].tagname, subIndex, origin, axis)) >= 0; subIndex++) {
				// project it onto the shadow plane
				if (origin[2] < *shadowPlane) {
					origin[2] = *shadowPlane;
				}

				// ydnar: add a bit of height so foot shadows don't clip into sloped geometry as much
				origin[2] += 5.0f;

				AxisToAngles(axis, angles);

				// ydnar: decal remix
				//%	CG_ImpactMark( shadowParts[tagIndex].shader, origin, trace.plane.normal,
				//%		angles[YAW]/*cent->pe.legs.yawAngle*/, alpha,alpha,alpha,1, qfalse, shadowParts[tagIndex].size, qtrue, -1 );

				//%	CG_ImpactMark( shadowParts[ tagIndex ].shader, origin, up,
				//%			cent->lerpAngles[ YAW ], 1.0f, 1.0f, 1.0f, 1.0f, qfalse, shadowParts[ tagIndex ].size, qtrue, -1 );
				CG_ImpactMark(shadowParts[tagIndex].shader, origin, projection, shadowParts[tagIndex].size,
				              angles[YAW], distFade, distFade, distFade, distFade, -1);
			}
		}
	}

	return qtrue;
}

/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash(centity_t *cent) {
	vec3_t     start, end;
	trace_t    trace;
	int        contents;
	polyVert_t verts[4];

	if (!cg_shadows.integer) {
		return;
	}

	VectorCopy(cent->lerpOrigin, end);
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CG_PointContents(end, 0);
	if (!(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))) {
		return;
	}

	VectorCopy(cent->lerpOrigin, start);
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents(start, 0);
	if (contents & (CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA)) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace(&trace, start, end, NULL, NULL, 0, (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA));

	if (trace.fraction == 1.0) {
		return;
	}

	// create a mark polygon
	VectorCopy(trace.endpos, verts[0].xyz);
	verts[0].xyz[0]     -= 32;
	verts[0].xyz[1]     -= 32;
	verts[0].st[0]       = 0;
	verts[0].st[1]       = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[1].xyz);
	verts[1].xyz[0]     -= 32;
	verts[1].xyz[1]     += 32;
	verts[1].st[0]       = 0;
	verts[1].st[1]       = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[2].xyz);
	verts[2].xyz[0]     += 32;
	verts[2].xyz[1]     += 32;
	verts[2].st[0]       = 1;
	verts[2].st[1]       = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy(trace.endpos, verts[3].xyz);
	verts[3].xyz[0]     += 32;
	verts[3].xyz[1]     -= 32;
	verts[3].st[0]       = 1;
	verts[3].st[1]       = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene(cgs.media.wakeMarkShader, 4, verts);
}

//==========================================================================

/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups(refEntity_t *ent, entityState_t *es, const vec3_t fireRiseDir) {
	centity_t   *cent;
	refEntity_t backupRefEnt; //, parentEnt;
	qboolean    onFire = qfalse;
	float       alpha  = 0.0;
	float       fireStart, fireEnd;

	cent = &cg_entities[es->number];

	ent->entityNum = es->number;

	backupRefEnt = *ent;

	if (CG_EntOnFire(&cg_entities[es->number])) {
		ent->reFlags |= REFLAG_FORCE_LOD;
	}

	trap_R_AddRefEntityToScene(ent);

	if (CG_EntOnFire(&cg_entities[es->number])) {
		onFire = qtrue;
		// set the alpha
		if (ent->entityNum == cg.snap->ps.clientNum) {
			fireStart = cg.snap->ps.onFireStart;
			fireEnd   = cg.snap->ps.onFireStart + 1500;
		} else {
			fireStart = es->onFireStart;
			fireEnd   = es->onFireEnd;
		}

		alpha = (cg.time - fireStart) / 1500.0;
		if (alpha > 1.0) {
			alpha = (fireEnd - cg.time) / 1500.0;
			if (alpha > 1.0) {
				alpha = 1.0;
			}
		}
	}

	if (onFire) {
		if (alpha < 0.0) {
			alpha = 0.0;
		}
		ent->shaderRGBA[3] = ( unsigned char )(255.0 * alpha);
		VectorCopy(fireRiseDir, ent->fireRiseDir);
		if (VectorCompare(ent->fireRiseDir, vec3_origin)) {
			VectorSet(ent->fireRiseDir, 0, 0, 1);
		}
		ent->customShader = cgs.media.onFireShader;
		trap_R_AddRefEntityToScene(ent);

		ent->customShader = cgs.media.onFireShader2;
		trap_R_AddRefEntityToScene(ent);

		if (ent->hModel == cent->pe.bodyRefEnt.hModel) {
			trap_S_AddLoopingSound(ent->origin, vec3_origin, cgs.media.flameCrackSound, (int)(255.0 * alpha), 0);
		}
	}

	*ent = backupRefEnt;
}

/*
===============
CG_AnimPlayerConditions

    predict, or calculate condition for this entity, if it is not the local client
===============
*/
void CG_AnimPlayerConditions(bg_character_t *character, centity_t *cent) {
	entityState_t *es;
	int           legsAnim;

	if (!character) {
		return;
	}
	if (cg.snap && cg.snap->ps.clientNum == cent->currentState.number && !cg.renderingThirdPerson) {
		return;
	}

	es = &cent->currentState;

	// WEAPON
	if (es->eFlags & EF_ZOOMING) {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_WEAPON, WP_BINOCULARS, qtrue);
	} else {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_WEAPON, es->weapon, qtrue);
	}

	// MOUNTED
	if ((es->eFlags & EF_MG42_ACTIVE) || (es->eFlags & EF_MOUNTEDTANK)) {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_MOUNTED, MOUNTED_MG42, qtrue);
	} else if (es->eFlags & EF_AAGUN_ACTIVE) {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_MOUNTED, MOUNTED_AAGUN, qtrue);
	} else {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_MOUNTED, MOUNTED_UNUSED, qtrue);
	}

	// UNDERHAND
	BG_UpdateConditionValue(es->clientNum, ANIM_COND_UNDERHAND, cent->lerpAngles[0] > 0, qtrue);

	if (es->eFlags & EF_CROUCHING) {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_CROUCHING, qtrue, qtrue);
	} else {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_CROUCHING, qfalse, qtrue);
	}

	if (es->eFlags & EF_FIRING) {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_FIRING, qtrue, qtrue);
	} else {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_FIRING, qfalse, qtrue);
	}

	// reverse engineer the legs anim -> movetype (if possible)
	legsAnim = es->legsAnim & ~ANIM_TOGGLEBIT;
	if (character->animModelInfo->animations[legsAnim]->movetype) {
		BG_UpdateConditionValue(es->clientNum, ANIM_COND_MOVETYPE, character->animModelInfo->animations[legsAnim]->movetype, qfalse);
	}

}


/*
===============
CG_Player
===============
*/
void CG_Player(centity_t *cent) {
	clientInfo_t   *ci;
	refEntity_t    body;
	refEntity_t    head;
	refEntity_t    acc;
	vec3_t         playerOrigin = {0, 0, 0};
	vec3_t         lightorigin;
	int            clientNum, i;
	int            renderfx;
	float          shadowPlane;
	qboolean       usingBinocs = qfalse;
	centity_t      *cgsnap;
	bg_character_t *character;
	float          hilightIntensity = 0.f;

	cgsnap = &cg_entities[cg.snap->ps.clientNum];

	shadowPlane = 0.0;                                              // ditto

	// if set to invisible, skip
	if (cent->currentState.eFlags & EF_NODRAW) {
		return;
	}

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if (clientNum < 0 || clientNum >= MAX_CLIENTS) {
		CG_Error("Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[clientNum];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if (!ci->infoValid) {
		return;
	}

	// Nico, don't draw if hiding others is enabled and distance to the player is < cg_hideRange
	if (cg_hideOthers.integer && ci->clientNum != cg.clientNum && Distance(cgsnap->lerpOrigin, cent->lerpOrigin) < cg_hideRange.integer) {
		return;
	}

	// Nico, don't draw if hideme is ON
	if (ci->hideme) {
		return;
	}

	character = CG_CharacterForClientinfo(ci, cent);

	if (cent->currentState.eFlags & EF_MOUNTEDTANK) {
		VectorCopy(cg_entities[cg_entities[cent->currentState.clientNum].tagParent].mountedMG42Player.origin, playerOrigin);
	} else if (cent->currentState.eFlags & EF_MG42_ACTIVE || cent->currentState.eFlags & EF_AAGUN_ACTIVE) {        // Arnout: see if we're attached to a gun
		centity_t *mg42;
		int       num;

		// find the mg42 we're attached to
		for (num = 0 ; num < cg.snap->numEntities ; num++) {
			mg42 = &cg_entities[cg.snap->entities[num].number];
			if (mg42->currentState.eType == ET_MG42_BARREL &&
			    mg42->currentState.otherEntityNum == cent->currentState.number) {
				// found it, clamp behind gun
				vec3_t forward, right, up;

				//AngleVectors (mg42->s.apos.trBase, forward, right, up);
				AngleVectors(cent->lerpAngles, forward, right, up);
				VectorMA(mg42->currentState.pos.trBase, -36, forward, playerOrigin);
				playerOrigin[2] = cent->lerpOrigin[2];
				break;
			}
		}

		if (num == cg.snap->numEntities) {
			VectorCopy(cent->lerpOrigin, playerOrigin);
		}
	} else {
		VectorCopy(cent->lerpOrigin, playerOrigin);
	}

	memset(&body, 0, sizeof (body));
	memset(&head, 0, sizeof (head));
	memset(&acc, 0, sizeof (acc));

	// get the rotation information
	CG_PlayerAngles(cent, body.axis, body.torsoAxis, head.axis);

	// FIXME: move this into CG_PlayerAngles
	if (cgsnap == cent && (cg.snap->ps.pm_flags & PMF_LADDER)) {
		memcpy(body.torsoAxis, body.axis, sizeof (body.torsoAxis));
	}

	// copy the torso rotation to the accessories
	AxisCopy(body.torsoAxis, acc.axis);

	// calculate client-side conditions
	CG_AnimPlayerConditions(character, cent);

	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation(cent, &body);

	// forcibly set binoc animation
	if (cent->currentState.eFlags & EF_ZOOMING) {
		usingBinocs = qtrue;
	}

	// add the any sprites hovering above the player
	// rain - corpses don't get icons (fireteam check ran out of bounds)
	if (cent->currentState.eType != ET_CORPSE) {
		CG_PlayerSprites(cent);
	}

	// add a water splash if partially in and out of water
	CG_PlayerSplash(cent);

	// get the player model information
	renderfx = 0;
	if (cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		renderfx = RF_THIRD_PERSON;         // only draw in mirrors
	}

	// draw the player in cameras
	if (cg.cameraMode) {
		renderfx &= ~RF_THIRD_PERSON;
	}

	renderfx |= RF_LIGHTING_ORIGIN;         // use the same origin for all

	// set renderfx for accessories
	acc.renderfx = renderfx;

	VectorCopy(playerOrigin, lightorigin);
	lightorigin[2] += 31;

	{
		vec3_t dist;
		vec_t  distSquared;

		VectorSubtract(lightorigin, cg.refdef_current->vieworg, dist);
		distSquared = VectorLengthSquared(dist);
		if (distSquared > Square(384.f)) {
			renderfx |= RF_MINLIGHT;

			distSquared -= Square(384.f);

			if (distSquared > Square(768.f)) {
				hilightIntensity = 1.f;
			} else {
				hilightIntensity = 1.f * (distSquared / Square(768.f));
			}
		}
	}

	body.hilightIntensity = hilightIntensity;
	head.hilightIntensity = hilightIntensity;
	acc.hilightIntensity  = hilightIntensity;

	//
	// add the body
	//
	if (cent->currentState.eType == ET_CORPSE && cent->currentState.time2 == 1) {
		body.hModel     = character->undressedCorpseModel;
		body.customSkin = character->undressedCorpseSkin;
	} else {
		body.customSkin = character->skin;
		body.hModel     = character->mesh;
	}

	VectorCopy(playerOrigin, body.origin);
	VectorCopy(lightorigin, body.lightingOrigin);
	body.shadowPlane = shadowPlane;
	body.renderfx    = renderfx;
	VectorCopy(body.origin, body.oldorigin);    // don't positionally lerp at all

	cent->pe.bodyRefEnt = body;

	// if the model failed, allow the default nullmodel to be displayed
	// Gordon: whoever wrote that comment sucks
	if (!body.hModel) {
		return;
	}

	// (SA) only need to set this once...
	VectorCopy(lightorigin, acc.lightingOrigin);

	CG_AddRefEntityWithPowerups(&body, &cent->currentState, cent->fireRiseDir);

	//
	// add the head
	//
	head.hModel = character->hudhead;
	if (!head.hModel) {
		return;
	}
	head.customSkin = character->hudheadskin;

	VectorCopy(lightorigin, head.lightingOrigin);

	CG_PositionRotatedEntityOnTag(&head, &body, "tag_head");

	head.shadowPlane = shadowPlane;
	head.renderfx    = renderfx;

	if (cent->currentState.eFlags & EF_FIRING) {
		cent->pe.lastFiredWeaponTime = 0;
		cent->pe.weaponFireTime     += cg.frametime;
	} else {
		if (cent->pe.weaponFireTime > 500 && cent->pe.weaponFireTime) {
			cent->pe.lastFiredWeaponTime = cg.time;
		}

		cent->pe.weaponFireTime = 0;
	}

	if (cent->currentState.eType != ET_CORPSE && !(cent->currentState.eFlags & EF_DEAD)) {
	} else {
		head.frame    = 0;
		head.oldframe = 0;
		head.backlerp = 0.f;
	}

	// set blinking flag
	CG_AddRefEntityWithPowerups(&head, &cent->currentState, cent->fireRiseDir);

	cent->pe.headRefEnt = head;

	// add the

	CG_PlayerShadow(cent, &shadowPlane);

	// set the shadowplane for accessories
	acc.shadowPlane = shadowPlane;

	CG_BreathPuffs(cent, &head);

	//
	// add the gun / barrel / flash
	//
	if (!(cent->currentState.eFlags & EF_DEAD) /*&& !usingBinocs*/) {              // NERVE - SMF
		CG_AddPlayerWeapon(&body, NULL, cent);
	}

	//
	// add binoculars (if it's not the player)
	//
	if (usingBinocs) {           // NERVE - SMF
		acc.hModel = cgs.media.thirdPersonBinocModel;
		CG_PositionEntityOnTag(&acc, &body, "tag_weapon", 0, NULL);
		CG_AddRefEntityWithPowerups(&acc, &cent->currentState, cent->fireRiseDir);
	}

	//
	// add accessories
	//
	for (i = ACC_BELT_LEFT; i < ACC_MAX; i++) {
		if (!(character->accModels[i])) {
			continue;
		}
		acc.hModel     = character->accModels[i];
		acc.customSkin = character->accSkins[i];

		// Gordon: looted corpses dont have any accsserories, evil looters :E
		if (!(cent->currentState.eType == ET_CORPSE && cent->currentState.time2 == 1)) {
			switch (i) {
			case ACC_BELT_LEFT:
				CG_PositionEntityOnTag(&acc, &body, "tag_bright", 0, NULL);
				break;
			case ACC_BELT_RIGHT:
				CG_PositionEntityOnTag(&acc, &body, "tag_bleft", 0, NULL);
				break;

			case ACC_BELT:
				CG_PositionEntityOnTag(&acc, &body, "tag_ubelt", 0, NULL);
				break;
			case ACC_BACK:
				CG_PositionEntityOnTag(&acc, &body, "tag_back", 0, NULL);
				break;

			case ACC_HAT:               //hat
			case ACC_RANK:
				if (cent->currentState.eFlags & EF_HEADSHOT) {
					continue;
				}
			case ACC_MOUTH2:            // hat2
			case ACC_MOUTH3:            // hat3

				if (i == ACC_RANK) {
					if (ci->rank == 0) {
						continue;
					}
				}

				CG_PositionEntityOnTag(&acc, &head, "tag_mouth", 0, NULL);
				break;

			// weapon and weapon2
			// these are used by characters who have permanent weapons attached to their character in the skin
			case ACC_WEAPON:        // weap
				CG_PositionEntityOnTag(&acc, &body, "tag_weapon", 0, NULL);
				break;
			case ACC_WEAPON2:       // weap2
				CG_PositionEntityOnTag(&acc, &body, "tag_weapon2", 0, NULL);
				break;


			default:
				continue;
			}

			CG_AddRefEntityWithPowerups(&acc, &cent->currentState, cent->fireRiseDir);
		}
	}
}

//=====================================================================

extern void CG_ClearWeapLerpFrame(clientInfo_t *ci, lerpFrame_t *lf, int animationNumber);

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity(centity_t *cent) {
	if (!(cent->currentState.eFlags & EF_DEAD)) {
		CG_ClearLerpFrameRate(cent, &cgs.clientinfo[cent->currentState.clientNum], &cent->pe.legs, cent->currentState.legsAnim);
		CG_ClearLerpFrame(cent, &cgs.clientinfo[cent->currentState.clientNum], &cent->pe.torso, cent->currentState.torsoAnim);

		memset(&cent->pe.legs, 0, sizeof (cent->pe.legs));
		cent->pe.legs.yawAngle   = cent->rawAngles[YAW];
		cent->pe.legs.yawing     = qfalse;
		cent->pe.legs.pitchAngle = 0;
		cent->pe.legs.pitching   = qfalse;

		memset(&cent->pe.torso, 0, sizeof (cent->pe.legs));
		cent->pe.torso.yawAngle   = cent->rawAngles[YAW];
		cent->pe.torso.yawing     = qfalse;
		cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
		cent->pe.torso.pitching   = qfalse;
	}

	BG_EvaluateTrajectory(&cent->currentState.pos, cg.time, cent->lerpOrigin, qfalse, cent->currentState.effect2Time);
	BG_EvaluateTrajectory(&cent->currentState.apos, cg.time, cent->lerpAngles, qtrue, cent->currentState.effect2Time);

	VectorCopy(cent->lerpOrigin, cent->rawOrigin);
	VectorCopy(cent->lerpAngles, cent->rawAngles);

	if (cg_debugPosition.integer) {
		CG_Printf("%i ResetPlayerEntity yaw=%i\n", cent->currentState.number, cent->pe.torso.yawAngle);
	}

	cent->pe.painAnimLegs  = -1;
	cent->pe.painAnimTorso = -1;
	cent->pe.animSpeed     = 1.0;

}

/*
===============
CG_GetTag
===============
*/
qboolean CG_GetTag(int clientNum, char *tagname, orientation_t *or) {
	clientInfo_t *ci;
	centity_t    *cent;
	refEntity_t  *refent;
	vec3_t       tempAxis[3];
	vec3_t       org;
	int          i;

	ci = &cgs.clientinfo[clientNum];

	if (cg.snap && clientNum == cg.snap->ps.clientNum && cg.renderingThirdPerson) {
		cent = &cg.predictedPlayerEntity;
	} else {
		cent = &cg_entities[ci->clientNum];
		if (!cent->currentValid) {
			return qfalse;      // not currently in PVS
		}
	}

	refent = &cent->pe.bodyRefEnt;

	if (trap_R_LerpTag(or, refent, tagname, 0) < 0) {
		return qfalse;
	}

	VectorCopy(refent->origin, org);

	for (i = 0 ; i < 3 ; i++) {
		VectorMA(org, or->origin[i], refent->axis[i], org);
	}

	VectorCopy(org, or->origin);

	// rotate with entity
	MatrixMultiply(refent->axis, or->axis, tempAxis);
	memcpy(or->axis, tempAxis, sizeof (vec3_t) * 3);

	return qtrue;
}

/*
===============
CG_GetWeaponTag
===============
*/
qboolean CG_GetWeaponTag(int clientNum, char *tagname, orientation_t *or) {
	clientInfo_t *ci;
	centity_t    *cent;
	refEntity_t  *refent;
	vec3_t       tempAxis[3];
	vec3_t       org;
	int          i;

	ci = &cgs.clientinfo[clientNum];

	if (cg.snap && clientNum == cg.snap->ps.clientNum && cg.renderingThirdPerson) {
		cent = &cg.predictedPlayerEntity;
	} else {
		cent = &cg_entities[ci->clientNum];
		if (!cent->currentValid) {
			return qfalse;      // not currently in PVS
		}
	}

	if (cent->pe.gunRefEntFrame < cg.clientFrame - 1) {
		return qfalse;
	}

	refent = &cent->pe.gunRefEnt;

	if (trap_R_LerpTag(or, refent, tagname, 0) < 0) {
		return qfalse;
	}

	VectorCopy(refent->origin, org);

	for (i = 0 ; i < 3 ; i++) {
		VectorMA(org, or->origin[i], refent->axis[i], org);
	}

	VectorCopy(org, or->origin);

	// rotate with entity
	MatrixMultiply(refent->axis, or->axis, tempAxis);
	memcpy(or->axis, tempAxis, sizeof (vec3_t) * 3);

	return qtrue;
}

// =============
// Menu Versions
// =============

animation_t *CG_GetLimboAnimation(playerInfo_t *pi, const char *name) {
	int            i;
	bg_character_t *character = BG_GetCharacter(pi->teamNum, pi->classNum);

	if (!character) {
		return NULL;
	}

	for (i = 0; i < character->animModelInfo->numAnimations; i++) {
		if (!Q_stricmp(character->animModelInfo->animations[i]->name, name)) {
			return character->animModelInfo->animations[i];
		}
	}

	return character->animModelInfo->animations[0]; // safe fallback so we never end up without an animation (which will crash the game)
}

weaponType_t weaponTypes[] =
{
	{ WP_MP40,                 "MP 40"    },
	{ WP_THOMPSON,             "THOMPSON" },
	{ WP_STEN,                 "STEN",    },
	{ WP_PANZERFAUST,          "PANZERFAUST",},
	{ WP_FLAMETHROWER,         "FLAMETHROWER",},
	{ WP_KAR98,                "K43",     },
	{ WP_CARBINE,              "M1 GARAND",},
	{ WP_FG42,                 "FG42",    },
	{ WP_GARAND,               "M1 GARAND",},
	{ WP_MOBILE_MG42,          "MOBILE MG42",},
	{ WP_K43,                  "K43",     },
	{ WP_MORTAR,               "MORTAR",  },
	{ WP_COLT,                 "COLT",    },
	{ WP_LUGER,                "LUGER",   },
	{ WP_AKIMBO_COLT,          "AKIMBO COLTS",},
	{ WP_AKIMBO_LUGER,         "AKIMBO LUGERS",},
	{ WP_SILENCED_COLT,        "COLT",    },
	{ WP_SILENCER,             "LUGER",   },
	{ WP_AKIMBO_SILENCEDCOLT,  "AKIMBO COLTS",},
	{ WP_AKIMBO_SILENCEDLUGER, "AKIMBO LUGERS",},
	{ WP_NONE,                 NULL,      },
	{ -1,                      NULL,      },
};

weaponType_t *WM_FindWeaponTypeForWeapon(weapon_t weapon) {
	weaponType_t *w = weaponTypes;

	if (!weapon) {
		return NULL;
	}

	while ((int)w->weapindex != -1) {
		if (w->weapindex == weapon) {
			return w;
		}
		w++;
	}
	return NULL;
}

void WM_RegisterWeaponTypeShaders() {
	weaponType_t *w = weaponTypes;

	while (w->weapindex) {
		w++;
	}
}
