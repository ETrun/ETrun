#include "cg_local.h"

/*
==================
CG_DrawCheckpoints

Draw checkpoints times

@author Nico
==================
*/
void CG_DrawCheckpoints(void) {
	float x = 0, y = 0;

	if (!etr_drawCheckPoints.integer) {
		return;
	}

	// Nico, printing position
	x = CG_WideX(etr_checkPointsX.value);
	y = etr_checkPointsY.value;

	// Nico, check cg_maxCheckPoints
	if (!etr_maxCheckPoints.integer || etr_maxCheckPoints.integer < 0) {
		etr_maxCheckPoints.integer = 5;
	}

	// Nico, print check points if any and respect the printing limit (etr_maxCheckPoints)
	if (cg.timerunCheckPointChecked > 0) {
		int i, j;

		for (i = cg.timerunCheckPointChecked - 1, j = 0; i >= 0 && j < etr_maxCheckPoints.integer; --i, ++j) {
			char   status[128];
			int    cmin, csec, cmil;
			int    cdmin, cdsec, cdmil;
			float  sizex = 0.2f, sizey = 0.2f;
			int    w;
			vec4_t color;

			cmil  = cg.timerunCheckPointTime[i];
			cmin  = cmil / 60000;
			cmil -= cmin * 60000;
			csec  = cmil / 1000;
			cmil -= csec * 1000;

			cdmil  = cg.timerunCheckPointDiff[i];
			cdmin  = cdmil / 60000;
			cdmil -= cdmin * 60000;
			cdsec  = cdmil / 1000;
			cdmil -= cdsec * 1000;

			// Nico, set checkpoint default color
			BG_ParseRGBACvar(etr_checkPointsColor1.string, color);

			// Nico, print checkpoints
			switch (cg.timerunCheckStatus[i]) {
			case 0:
				Com_sprintf(status, sizeof (status), "%s", va("%02d:%02d.%03d", cmin, csec, cmil));
				break;

			case 1:
				Com_sprintf(status, sizeof (status), "%s", va("%02d:%02d.%03d", cdmin, cdsec, cdmil));
				break;

			case 2:
				// Nico, faster check point time
				BG_ParseRGBACvar(etr_checkPointsColor2.string, color);
				Com_sprintf(status, sizeof (status), "%s", va("-%02d:%02d.%03d", cdmin, cdsec, cdmil));
				break;

			case 3:
				// Nico, slower check point time
				BG_ParseRGBACvar(etr_checkPointsColor3.string, color);
				Com_sprintf(status, sizeof (status), "%s", va("+%02d:%02d.%03d", cdmin, cdsec, cdmil));
				break;
			}

			// Nico, print the check point
			w = CG_Text_Width_Ext(status, sizex, sizey, &cgs.media.limboFont1) / 2;
			CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

			// Nico, line jump
			y += 10;
		}
	}
}

/*
==================
CG_DrawSpeedMeter

Draw speed meter

@author Nico
==================
*/
void CG_DrawSpeedMeter(void) {
	char         status[128];
	float        sizex, sizey, x, y;
	int          w;
	static vec_t speed;
	vec4_t       color;

	if (!etr_drawSpeedMeter.integer) {
		return;
	}

	speed = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);

	// Nico, cg.predictedPlayerState.velocity is sometimes NaN (example on asdarun1), so check it
	// http://stackoverflow.com/questions/570669/checking-if-a-double-or-float-is-nan-in-c
	if (speed != speed) {
		speed = 0;
	}

	sizex = sizey = 0.25f;

	x = CG_WideX(etr_speedMeterX.value);
	y = etr_speedMeterY.value;

	Com_sprintf(status, sizeof (status), "%.0f", speed);

	w = CG_Text_Width_Ext(status, sizex, sizey, &cgs.media.limboFont1) / 2;
	BG_ParseRGBACvar(etr_speedMeterColor1.string, color);

	if (etr_drawAccel.integer && speed > cg.oldSpeed + 0.001f * etr_accelSmoothness.integer) {
		BG_ParseRGBACvar(etr_speedMeterColor2.string, color);
		CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else if (etr_drawAccel.integer && speed < cg.oldSpeed - 0.001f * etr_accelSmoothness.integer) {
		BG_ParseRGBACvar(etr_speedMeterColor3.string, color);
		CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else {
		CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}

	cg.oldSpeed = speed;
}

/*
==================
CG_DrawOB

OB detector from TJMod

@author Nico
==================
*/
void CG_DrawOB(void) {
	double  a, b, c;
	float   psec, rintv, v0, h0, hn, t, n2, scale, x, y;
	int     gravity, n;
	trace_t trace;
	vec3_t  start, end, snap;
	vec4_t  color;

	if (!etr_drawOB.integer || cg_thirdPerson.integer || physics.integer & PHYSICS_NO_OVERBOUNCE) {
		return;
	}

	psec    = pmove_msec.integer / 1000.0;
	gravity = cg.predictedPlayerState.gravity;
	v0      = cg.predictedPlayerState.velocity[2];
	h0      = cg.predictedPlayerState.origin[2] + cg.predictedPlayerState.mins[2];

	VectorSet(snap, 0, 0, gravity * psec);
	trap_SnapVector(snap);
	rintv = snap[2];

	// use origin from playerState?
	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, TRACE_MAX_DISTANCE, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.predictedPlayerState.clientNum, CONTENTS_SOLID);

	// we didn't hit anything
	if (trace.fraction == 1.0) {
		return;
	}

	// not a floor
	if (trace.plane.type != 2) {
		return;
	}

	t = trace.endpos[2];

	
	x = CG_WideX(etr_OBX.value);
	y = etr_OBY.value;
	BG_ParseRGBACvar(etr_OBColor.string, color);
	scale = 0.15f;

	// fall ob
	a = -psec * rintv / 2;
	b = psec * (v0 - gravity * psec / 2 + rintv / 2);
	c = h0 - t;
	n2 = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);

	n  = floor(n2);
	hn = h0 + psec * n * (v0 - gravity * psec / 2 - (n - 1) * rintv / 2);
	if (n && hn < t + 0.25 && hn > t) {
		CG_Text_Paint_Ext(x, y, scale, scale, color, "F", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}

	if (cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE) {
		// jump ob
		v0 += 270; // JUMP_VELOCITY
		b   = psec * (v0 - gravity * psec / 2 + rintv / 2);
		n2 = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);

		n  = floor(n2);
		hn = h0 + psec * n * (v0 - gravity * psec / 2 - (n - 1) * rintv / 2);

		if (hn < t + 0.25 && hn > t) {
			CG_Text_Paint_Ext(x + 10, y, scale, scale, color, "J", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
	}
}

/*
==================
CG_DrawSlick

Slick detector

@author suburb
==================
*/
void CG_DrawSlick(void) {
	static trace_t trace;
	vec3_t         start, end;
	float          scale;
	float          x, y;
	vec4_t         color;

	if (!etr_drawSlick.integer || cg_thirdPerson.integer) {
		return;
	}

	x = CG_WideX(etr_slickX.value);
	y = etr_slickY.value;
	BG_ParseRGBACvar(etr_slickColor.string, color);
	scale = 0.15f;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, TRACE_MAX_DISTANCE, cg.refdef.viewaxis[0], end);
	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.predictedPlayerState.clientNum, CONTENTS_SOLID);

	if (trace.surfaceFlags & SURF_SLICK) {
		CG_Text_Paint_Ext(x, y, scale, scale, color, "S", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
}

/*
==================
CG_DrawTimer

Draw timer

@author Nico
==================
*/
void CG_DrawTimer(void) {
	char       status[128];
	int        min = 0, sec = 0, milli = 0;
	int        w = 0;
	int        timerunNum = cg.currentTimerun;
	int        startTime = 0;
	int        currentTimerunTime = 0;
	float      sizex = 0.25f, sizey = 0.25f, x, y;
	vec4_t     color;
	int        runBestTime;
	int        runLastTime;
	int        clientNum  = cg.clientNum; // Nico, player client num or spectated player client num
	static int needsReset = 0;

	// Nico, if level is not a timer
	if (!isTimerun.integer || !etr_drawTimer.integer) {
		return;
	}

	// Nico, should we display spectated player values or real player values?
	if (cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) {
		clientNum = cg.snap->ps.clientNum;
	}

	runBestTime = cg.timerunBestTime[clientNum][timerunNum];
	runLastTime = cg.timerunLastTime[clientNum][timerunNum];

	// Nico, check if timer needs reset
	// note: this should be moved somewhere else
	if (!cg.timerunActive) {
		// Nico, timer will be reseted next time timerun becomes active
		needsReset = 1;
	}

	if (needsReset && cg.timerunActive) {
		// Nico, reset timer
		cg.timerunFinishedTime[clientNum] = 0;
		needsReset                        = 0;
	}
	//

	// Nico, set timer position
	x = CG_WideX(etr_timerX.value);
	y = etr_timerY.value;

	// Nico, get fixed timerun start time
	startTime = cg.timerunStartTime - 500;

	if (cg.timerunActive) {
		// Nico, timerun active, run not finished yet
		milli = currentTimerunTime = cg.time - startTime;
	} else if (cg.timerunFinishedTime[clientNum]) {
		// Nico, timerun inactive, run finished
		milli = currentTimerunTime = cg.timerunFinishedTime[clientNum];
	} else {
		// Nico, timerun not active, run not finished
		milli = currentTimerunTime;
	}

	// Nico, extract min:sec.milli
	min    = milli / 60000;
	milli -= min * 60000;
	sec    = milli / 1000;
	milli -= sec * 1000;

	// Nico, set timer default color
	BG_ParseRGBACvar(etr_timerColor1.string, color);

	if (cg.timerunFinishedTime[clientNum]) {
		// Nico, timerun finished

		// Compare with client rec
		if (runBestTime > 0 && runLastTime != runBestTime) {
			int dmin, dsec, dmilli;

			// Nico, did a different time, compute the delta
			dmilli  = abs(runLastTime - runBestTime);
			dmin    = dmilli / 60000;
			dmilli -= dmin * 60000;
			dsec    = dmilli / 1000;
			dmilli -= dsec * 1000;

			if (runLastTime < runBestTime) {
				// Nico, did a better time
				BG_ParseRGBACvar(etr_timerColor2.string, color);
				Com_sprintf(status, sizeof (status), "%s", va("%02d:%02d.%03d (-%02d:%02d.%03d)", min, sec, milli, dmin, dsec, dmilli));
			} else {
				// Nico, did a slower time
				BG_ParseRGBACvar(etr_timerColor3.string, color);
				Com_sprintf(status, sizeof (status), "%s", va("%02d:%02d.%03d (+%02d:%02d.%03d)", min, sec, milli, dmin, dsec, dmilli));
			}
		} else if (runBestTime > 0) {
			// Nico, did the same time
			Com_sprintf(status, sizeof (status), "%s", va("%02d:%02d.%03d (+00:00.000)", min, sec, milli));
		} else {
			// Nico, first time
			Com_sprintf(status, sizeof (status), "%s", va("%02d:%02d.%03d", min, sec, milli));
		}
	} else {
		// Nico, timerun not finished yet

		// Nico, you won't beat the rec this time, turn timer to red color
		if (runBestTime > 0 && currentTimerunTime > runBestTime + 16) {
			BG_ParseRGBACvar(etr_timerColor3.string, color);
		}

		Com_sprintf(status, sizeof (status), "%s", va("%02d:%02d.%03d", min, sec, milli));
	}

	// Nico, print the timer
	w = CG_Text_Width_Ext(status, sizex, sizey, &cgs.media.limboFont1) / 2;
	CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

extern float pm_stopspeed;
extern float pm_accelerate;
extern float pm_airaccelerate;
extern float pm_friction;

// Nico, AP accel
extern float pm_accelerate_AP;

typedef struct {
	vec3_t forward, right, up;
	float frametime;

	int msec;

	qboolean walking;
	qboolean groundPlane;
	trace_t groundTrace;

	float impactSpeed;

	vec3_t previous_origin;
	vec3_t previous_velocity;
	int previous_waterlevel;

	// Ridah, ladders
	qboolean ladder;
} pml_t;
extern pml_t pml;

/*
==================
CG_DrawCGaz

Draw CGaz from TJMod

@author Nico
==================
*/
#define CGAZ3_ANG 20
void CG_DrawCGaz(void) {
	float vel_angle; // absolute velocity angle
	float vel_relang; // relative velocity angle to viewangles[1]
	float per_angle;
	float accel_angle;
	float a;
	float accel;
	float scale;
	float ang;
	float h = 20; // CGaz height
	float w = 300; // CGaz width
	float x, y;

	int forward = 0;
	int right   = 0;

	vec_t  vel_size;
	vec3_t vel;
	vec4_t color;

	playerState_t *ps;

	// Nico, if etr_drawCGaz is 0
	if (!etr_drawCGaz.integer) {
		return;
	}

	ps = &cg.predictedPlayerState;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	a        = 0.15f;
	a        = (a > 1.0f) ? 1.0f : (a < 0.0f) ? 0.0f : a;
	color[3] = a;
	VectorCopy(ps->velocity, vel);

	// for a simplicity water, ladder etc. calculations are omitted
	// only air, ground and ice movement is important
	if (pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK)) {
		// apply friction
		float speed;

		speed = VectorLength(vel);
		if (speed > 0) {
			float newspeed, drop;

			drop = 0;

			// if getting knocked back, no friction
			if (!(ps->pm_flags & PMF_TIME_KNOCKBACK)) {
				float control = speed < pm_stopspeed ? pm_stopspeed : speed;
				drop += control * pm_friction * pmove_msec.integer / 1000;
			}
			newspeed = speed - drop;
			if (newspeed < 0) {
				newspeed = 0;
			}
			newspeed /= speed;
			VectorScale(vel, newspeed, vel);
		}

		// on ground

		// Nico, AP or stock accel?
		if (physics.integer == PHYSICS_MODE_AP_NO_OB || physics.integer == PHYSICS_MODE_AP_OB) {
			// suburb, CGaz 2 on ground fix
			if (etr_drawCGaz.integer == 2) {
				accel = pm_accelerate_AP - 10.0f;
			} else {
				accel = pm_accelerate_AP;
			}
		} else {
			accel = pm_accelerate;
		}
	} else {
		// in air or on ice, no friction
		accel = pm_airaccelerate;
	}

	vel_size = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
	accel    = accel * ps->speed * pmove_msec.integer / 1000;

	// based on PM_CmdScale from bg_pmove.c
	scale     = cg.keyDown[0] ? ps->sprintSpeedScale : ps->runSpeedScale;
	per_angle = (ps->speed - accel) / vel_size * scale;
	if (per_angle < 1) {
		per_angle = RAD2DEG(acos(per_angle));
	} else {
		per_angle = ps->viewangles[YAW];
	}

	vel_angle  = AngleNormalize180(RAD2DEG(atan2(vel[1], vel[0])));
	vel_relang = AngleNormalize180(ps->viewangles[YAW] - vel_angle);

	// parse usercmd
	if (cg.keyDown[1]) {
		forward = 127;
	} else if (cg.keyDown[6]) {
		forward = -128;
	}

	if (cg.keyDown[4]) {
		right = 127;
	} else if (cg.keyDown[3]) {
		right = -128;
	}

	x = CG_WideX(etr_CGazX.value) - 0.5f;
	y = etr_CGazY.value - 0.5f;

	if (etr_drawCGaz.integer == 1) {
		y += 20;
		CG_FillRect(x - w / 2, y, w, h, colorBlue);
		CG_FillRect(x - w / 2, y, 1, h, colorWhite);
		CG_FillRect(x - w / 4, y, 1, h, colorWhite);
		CG_FillRect(x, y, 1, h, colorWhite);
		CG_FillRect(x + w / 4, y, 1, h, colorWhite);
		CG_FillRect(x + w / 2, y, 1, h, colorWhite);

		if (vel_size < ps->speed * scale) {
			return;
		}

		// velocity
		if (vel_relang > -90 && vel_relang < 90) {
			CG_FillRect(x + w * vel_relang / 180, y, 1, h, colorOrange);
		}

		// left/right perfect strafe
		if (vel_relang > 0) {
			ang = AngleNormalize180(vel_relang - per_angle);
			if (ang > -90 && ang < 90) {
				CG_FillRect(x + w * ang / 180, y, 1, h, colorGreen);
			}
		} else {
			ang = AngleNormalize180(vel_relang + per_angle);
			if (ang > -90 && ang < 90) {
				CG_FillRect(x + w * ang / 180, y, 1, h, colorGreen);
			}
		}
	} else if (etr_drawCGaz.integer == 2) {
		// CGaz 2
		vel_relang = DEG2RAD(vel_relang);
		per_angle  = DEG2RAD(per_angle);

		BG_ParseRGBACvar(etr_CGaz2Color2.string, color);

		if (!etr_realCGaz2.integer && etr_widescreenSupport.integer) {
			CG_DrawLine(x, y, x + CG_WideX(right), y - forward, color);
		} else {
			CG_DrawLine(x, y, x + right, y - forward, color);
		}

		vel_size /= 5;
		if (vel_size > CG_WideX(SCREEN_WIDTH) / 2) {
			vel_size = CG_WideX(SCREEN_WIDTH) / 2;
		}

		BG_ParseRGBACvar(etr_CGaz2Color1.string, color);

		CG_DrawLine(x, y, x + vel_size * sin(vel_relang), y - vel_size * cos(vel_relang), color);

		if (vel_size > SCREEN_HEIGHT / 2) {
			vel_size = SCREEN_HEIGHT / 2;
		}
		vel_size /= 2;

		if (!etr_realCGaz2.integer && etr_widescreenSupport.integer) {
			CG_DrawLine(x, y, x + vel_size * CG_WideX(sin(vel_relang + per_angle)), y - vel_size * cos(vel_relang + per_angle), color);
			CG_DrawLine(x, y, x + vel_size * CG_WideX(sin(vel_relang - per_angle)), y - vel_size * cos(vel_relang - per_angle), color);
		} else {
			CG_DrawLine(x, y, x + vel_size * sin(vel_relang + per_angle), y - vel_size * cos(vel_relang + per_angle), color);
			CG_DrawLine(x, y, x + vel_size * sin(vel_relang - per_angle), y - vel_size * cos(vel_relang - per_angle), color);
		}
	} else if (etr_drawCGaz.integer == 3) {
		y += 20;
		accel_angle = atan2(-right, forward);
		accel_angle = AngleNormalize180(ps->viewangles[YAW] + RAD2DEG(accel_angle));

		CG_FillRect(x - w / 2, y, 1, h, colorWhite);
		CG_FillRect(x, y, 1, h, colorWhite);
		CG_FillRect(x + w / 2, y, 1, h, colorWhite);

		if (vel_size < ps->speed * scale) {
			return;
		}

		// first case (consider left strafe)
		ang = AngleNormalize180(vel_angle + per_angle - accel_angle);
		if (ang > -CGAZ3_ANG && ang < CGAZ3_ANG) {
			// acceleration "should be on the left side" from velocity
			if (ang < 0) {
				VectorCopy(colorGreen, color);
				CG_FillRect(x + w * ang / (2 * CGAZ3_ANG), y, -w * ang / (2 * CGAZ3_ANG), h, color);
			} else {
				VectorCopy(colorRed, color);
				CG_FillRect(x, y, w * ang / (2 * CGAZ3_ANG), h, color);
			}
			return;
		}
		// second case (consider right strafe)
		ang = AngleNormalize180(vel_angle - per_angle - accel_angle);
		if (ang > -CGAZ3_ANG && ang < CGAZ3_ANG) {
			// acceleration "should be on the right side" from velocity
			if (ang > 0) {
				VectorCopy(colorGreen, color);
				CG_FillRect(x, y, w * ang / (2 * CGAZ3_ANG), h, color);
			} else {
				VectorCopy(colorRed, color);
				CG_FillRect(x + w * ang / (2 * CGAZ3_ANG), y, -w * ang / (2 * CGAZ3_ANG), h, color);
			}
			return;
		}
	} else if (etr_drawCGaz.integer == 4) {
		accel_angle = atan2(-right, forward);
		accel_angle = AngleNormalize180(ps->viewangles[YAW] + RAD2DEG(accel_angle));

		if (vel_size < ps->speed * scale) {
			return;
		}

		VectorCopy(colorGreen, color);

		// Speed direction
		// FIXME: Shows @side of screen when going backward
		//CG_FillRect(x + w * vel_relang / 180, y+20, 1, h/2, colorCyan);
		CG_DrawPic(x + w * vel_relang / 180, y + h, 16, 16, cgs.media.CGazArrow);

		// FIXME show proper outside border where you stop accelerating
		// first case (consider left strafe)
		ang = AngleNormalize180(vel_angle + per_angle - accel_angle);
		if (ang < 90 && ang > -90) {
			// acceleration "should be on the left side" from velocity
			CG_FillRect(x + w * ang / 180, y, w / 2, h, color);
			CG_FillRect(x + w * ang / 180, y, 1, h, colorGreen);
			return;

		}
		// second case (consider right strafe)
		ang = AngleNormalize180(vel_angle - per_angle - accel_angle);
		if (ang < 90 && ang > -90) {
			// acceleration "should be on the right side" from velocity
			CG_FillRect(x + w * ang / 180 - w / 2, y, w / 2, h, color);
			CG_FillRect(x + w * ang / 180, y, 1, h, colorGreen);
			return;
		}
	}
}

/*
==================
sortSnapZones

Detached function to sort velocity snapping zones (taken from iodfengine)

@author suburb
==================
*/
static int QDECL sortSnapZones(const void *a, const void *b) {
	return *(float *)a - *(float *)b;
}

/*
==================
CG_DrawVelocitySnapping

Draw velocity snapping zones (core math taken from iodfengine)

@author suburb
==================
*/
void CG_DrawVelocitySnapping(void) {
	vec4_t color[2];
	float  step      = 0;
	float  yaw       = 0;
	float  snapHud_H = 0;
	float  snapHud_Y = 0;
	int    colorID   = 0;
	int    fov       = 0;
	int    i         = 0;

	if (!etr_drawVelocitySnapping.integer || (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)) {
		return;
	}

	// check whether snapSpeed needs to be updated
	if (cg.snapSpeed != cg.snap->ps.speed * DF_TO_ET_GROUNDSPEED) {
		cg.snapSpeed  = cg.snap->ps.speed * DF_TO_ET_GROUNDSPEED;
		cg.snapSpeed /= 125;
		cg.snapCount  = 0;

		for (step = floor(cg.snapSpeed + 0.5) - 0.5; step > 0 && cg.snapCount < (int) (sizeof (cg.snapZones) - 2); step--) {
			cg.snapZones[cg.snapCount] = RAD2DEG(acos(step / cg.snapSpeed));
			cg.snapCount++;
			cg.snapZones[cg.snapCount] = RAD2DEG(asin(step / cg.snapSpeed));
			cg.snapCount++;
		}

		qsort(cg.snapZones, cg.snapCount, sizeof (cg.snapZones[0]), sortSnapZones);
		cg.snapZones[cg.snapCount] = cg.snapZones[0] + 90;
	}

	// draw snapping
	yaw = cg.predictedPlayerState.viewangles[YAW];

	if (physics.integer == PHYSICS_MODE_AP_NO_OB || physics.integer == PHYSICS_MODE_AP_OB) { // only shift for AP physics
		yaw += 45;
	}

	snapHud_H = etr_velocitySnappingH.value;
	snapHud_Y = etr_velocitySnappingY.value;
	fov       = etr_velocitySnappingFov.integer;

	if (etr_drawVelocitySnapping.integer == 2) {
		BG_ParseRGBACvar(etr_velocitySnapping2Color.string, color[0]);
		BG_ParseRGBACvar(etr_velocitySnapping2Color.string, color[1]);
	} else {
		BG_ParseRGBACvar(etr_velocitySnapping1Color1.string, color[0]);
		BG_ParseRGBACvar(etr_velocitySnapping1Color2.string, color[1]);
	}

	for (i = 0; i < cg.snapCount; i++) {
		CG_FillAngleYaw(cg.snapZones[i], cg.snapZones[i + 1], yaw, snapHud_Y, snapHud_H, fov, color[colorID]);
		CG_FillAngleYaw(cg.snapZones[i] + 90, cg.snapZones[i + 1] + 90, yaw, snapHud_Y, snapHud_H, fov, color[colorID]);
		colorID ^= 1;
	}
}

/*
==================
CG_DrawKeys

Draw keys from TJMod

@author Nico
==================
*/
void CG_DrawKeys(void) {
	float x, y, size;
	int   i;
	int   skew;

	if (!etr_drawKeys.integer) {
		return;
	}

	// Dini, add in here for any other keysets with skew effect or related.
	if (etr_drawKeys.integer == 1) {
		skew = etr_keysSize.value / 18;
	} else {
		skew = 0;
	}

	trap_R_SetColor(colorWhite);

	size = etr_keysSize.value / 3;
	i    = (etr_drawKeys.integer - 1) % NUM_KEYS_SETS;

	// first (upper) row
	// sprint (upper left)

	x = CG_WideX(etr_keysX.value) + 2 * skew;
	y = etr_keysY.value;

	if (cg.keyDown[0]) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].SprintPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].SprintNotPressedShader);
	}

	// forward
	x += size;
	if (cg.keyDown[1]) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].ForwardPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].ForwardNotPressedShader);
	}

	// jump (upper right)
	x += size;
	if (cg.keyDown[2]) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].JumpPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].JumpNotPressedShader);
	}

	// second (middle) row
	// left
	x = CG_WideX(etr_keysX.value) + skew;
	y += size;
	if (cg.keyDown[3]) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].LeftPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].LeftNotPressedShader);
	}

	// right
	x += 2 * size;
	if (cg.keyDown[4]) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].RightPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].RightNotPressedShader);
	}

	// third (bottom) row
	x = CG_WideX(etr_keysX.value);
	y += size;
	// prone (bottom left)
	if (cg.keyDown[5]) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].PronePressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].ProneNotPressedShader);
	}

	// backward
	x += size;
	if (cg.keyDown[6]) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].BackwardPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].BackwardNotPressedShader);
	}

	// crouch (bottom right)
	x += size;
	if (cg.keyDown[7]) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].CrouchPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].CrouchNotPressedShader);
	}
}

/*
==================
CG_UpdateKeysAndMenus

Used for UI flickering fix

@author suburb
==================
*/
#define DRAWKEYS_DEBOUNCE_VALUE        100
#define DRAWKEYS_MENU_CLOSING_DELAY    50
void CG_UpdateKeysAndMenus(void) {
	playerState_t *ps = &cg.predictedPlayerState;
	qboolean      downNow[KEYS_AMOUNT];
	qboolean      anyMenuClosedRecently = qfalse;
	int           i;

	// get the current buttons
	downNow[0] = ps->stats[STAT_USERCMD_BUTTONS] & (BUTTON_SPRINT << 8);
	downNow[1] = ps->stats[STAT_USERCMD_MOVE] & UMOVE_FORWARD;
	downNow[2] = ps->stats[STAT_USERCMD_MOVE] & UMOVE_UP;
	downNow[3] = ps->stats[STAT_USERCMD_MOVE] & UMOVE_LEFT;
	downNow[4] = ps->stats[STAT_USERCMD_MOVE] & UMOVE_RIGHT;
	downNow[5] = ps->stats[STAT_USERCMD_BUTTONS] & WBUTTON_PRONE;
	downNow[6] = ps->stats[STAT_USERCMD_MOVE] & UMOVE_BACKWARD;
	downNow[7] = ps->stats[STAT_USERCMD_MOVE] & UMOVE_DOWN;

	// check whether console is up
	if (!cg.consoleIsUp && trap_Key_GetCatcher() & KEYCATCH_CONSOLE) {
		cg.consoleIsUp = qtrue;
	} else if (cg.consoleIsUp && !(trap_Key_GetCatcher() & KEYCATCH_CONSOLE)) {
		cg.lastClosedMenuTime = cg.time;
		cg.consoleIsUp        = qfalse;
	}

	// check whether any UI menu is up
	if (!cg.UIisUp && trap_Key_GetCatcher() & KEYCATCH_UI) {
		cg.UIisUp = qtrue;
	} else if (cg.UIisUp && !(trap_Key_GetCatcher() & KEYCATCH_UI)) {
		cg.lastClosedMenuTime = cg.time;
		cg.UIisUp             = qfalse;
	}

	// check whether limbo menu is up
	if (!cg.limboIsUp && trap_Key_GetCatcher() & KEYCATCH_CGAME) {
		cg.limboIsUp = qtrue;
	} else if (cg.limboIsUp && !(trap_Key_GetCatcher() & KEYCATCH_CGAME)) {
		cg.lastClosedMenuTime = cg.time;
		cg.limboIsUp          = qfalse;
	}

	// check whether any menu has closed within the last 50ms
	if (!cg.consoleIsUp && !cg.UIisUp && !cg.limboIsUp && (cg.time - cg.lastClosedMenuTime < DRAWKEYS_MENU_CLOSING_DELAY)) {
		anyMenuClosedRecently = qtrue;
	}

	// check whether flickering is even possible
	for (i = 0; i < KEYS_AMOUNT; ++i) {
		if (cg.consoleIsUp || cg.UIisUp || cg.limboIsUp || anyMenuClosedRecently) {
			if (cg.keyDown[i] != downNow[i]) {
				if (cg.keyTimes[i] == 0) {
					cg.keyTimes[i] = cg.time;
				} else if (cg.time - cg.keyTimes[i] > DRAWKEYS_DEBOUNCE_VALUE) {
					// require it to be unchanged for 100ms before we care
					cg.keyTimes[i] = 0;
					cg.keyDown[i]  = downNow[i];
				}
			} else if (cg.keyTimes[i] != 0) {
				// so if it needs a new full 50ms
				cg.keyTimes[i] = 0;
			}
		} else {
			cg.keyDown[i] = downNow[i];
		}
	}
}

/*
==================
CG_DrawScoresClock

Draw clock from TJMod

@author Nico
==================
*/
void CG_DrawScoresClock(float x, float y, float scale) {
	char displayTime[18] = { 0 };

	Q_strcat(displayTime, sizeof (displayTime), va("Time: %s", CG_GetClock()));

	CG_Text_Paint_Ext(x, y, scale, scale, colorWhite, displayTime, 0, 24, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/*
==================
CG_GetClock

Returns realtime in the format "hh:mm:ss"

@author suburb
==================
*/
char *CG_GetClock(void) {
	static char displayTime[19] = { 0 };
	qtime_t     tm;

	trap_RealTime(&tm);
	displayTime[0] = '\0';
	Q_strcat(displayTime, sizeof (displayTime), va("%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec));

	return displayTime;
}

/*
==================
CG_DrawBannerPrint

Print banner from TJmod

@author Nico
==================
*/
void CG_DrawBannerPrint(void) {
	char  *start = cg.bannerPrint;
	int   l      = 0;
	float *color;
	float sizex = 0.2f, sizey = 0.2f, x, y;
	char  lastcolor = COLOR_WHITE;
	int   charHeight;
	int   bannerShowTime = 10000;
	int   len, w;

	if (!etr_drawBannerPrint.integer || !cg.bannerPrintTime) {
		return;
	}

	color = CG_FadeColor(cg.bannerPrintTime, bannerShowTime);
	if (!color) {
		cg.bannerPrintTime = 0;
		return;
	}

	trap_R_SetColor(color);

	charHeight = CG_Text_Height_Ext("A", sizey, 0, &cgs.media.limboFont2);

	len = strlen(cg.bannerPrint);
	y = etr_bannerPrintY.value;

	for (;;) {
		char linebuffer[1024];
		char colorchar = lastcolor;

		for (l = 0; l < len; ++l) {
			if (!start[l] || start[l] == '\n') {
				break;
			}
			if (Q_IsColorString(&start[l])) {
				lastcolor = start[l + 1];
			}
			linebuffer[l] = start[l];
		}
		linebuffer[l] = 0;

		w = CG_Text_Width_Ext(linebuffer, sizex, 0, &cgs.media.limboFont2);

		x = (CG_WideX(etr_bannerPrintX.value * 2) - w) / 2;

		CG_Text_Paint_Ext(x, y, sizex, sizey, color, va("^%c%s", colorchar, linebuffer), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);

		y += charHeight * 2;

		while (*start && *start != '\n') {
			start++;
		}
		if (!*start) {
			break;
		}
		start++;
	}
	trap_R_SetColor(NULL);
}

/*
==================
CG_DrawDemoRecording

Draw statusline while recording

@author suburb
==================
*/
#define SHOW_DEMO_SAVED 5000
void CG_DrawDemoRecording(void) {
	char   status[1024];
	char   demostatus[128];
	char   wavestatus[128];
	vec4_t color;

	if ((!cl_demorecording.integer && !cl_waverecording.integer) || !etr_drawStatusline.integer) {
		return;
	}

	if (cl_demorecording.integer) {
		Com_sprintf(demostatus, sizeof (demostatus), " Demo: %s [%iKB] ", cl_demofilename.string, cl_demooffset.integer / 1024);
	} else {
		strncpy(demostatus, "", sizeof (demostatus));
	}

	if (cl_waverecording.integer) {
		Com_sprintf(wavestatus, sizeof (demostatus), " Audio: %s [%iKB] ", cl_wavefilename.string, cl_waveoffset.integer / 1024);
	} else {
		strncpy(wavestatus, "", sizeof (wavestatus));
	}

	Com_sprintf(status, sizeof (status), "Recording%s%s", demostatus, wavestatus);

	BG_ParseRGBACvar(etr_statuslineColor.string, color);

	// Nico, add recording red dot
	CG_Text_Paint_Ext(etr_statuslineX.value, etr_statuslineY.value, 0.5f, 0.5f, colorRed, ".", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
	CG_Text_Paint_Ext(etr_statuslineX.value + 14, etr_statuslineY.value, 0.14f, 0.14f, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	// suburb, additional UI to replace console printouts
	if (!cg.UIisUp) {
		if (cg.stoppingAndSavingDemo) {
			CG_Text_Paint_Ext(etr_statuslineX.value, etr_statuslineY.value + 10, 0.5f, 0.5f, colorYellow, ".", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			CG_Text_Paint_Ext(etr_statuslineX.value + 14, etr_statuslineY.value + 10, 0.14f, 0.14f, color, "Stopping demo", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		} else if (cg.time - cg.lastUnableToSaveDemoTime < SHOW_DEMO_SAVED && cg.lastUnableToSaveDemoTime != 0) {
			CG_Text_Paint_Ext(etr_statuslineX.value, etr_statuslineY.value + 10, 0.5f, 0.5f, colorRed, ".", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			CG_Text_Paint_Ext(etr_statuslineX.value + 14, etr_statuslineY.value + 10, 0.14f, 0.14f, color, "Unable to save demo", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		} else if (cg.time - cg.lastSavingDemoTime < SHOW_DEMO_SAVED && cg.lastSavingDemoTime != 0) {
			CG_Text_Paint_Ext(etr_statuslineX.value, etr_statuslineY.value + 10, 0.5f, 0.5f, colorGreen, ".", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont2);
			CG_Text_Paint_Ext(etr_statuslineX.value + 14, etr_statuslineY.value + 10, 0.14f, 0.14f, color, "Demo saved", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
	}
}

/*
==================
CG_UpdateJumpSpeeds

Update jump speeds, taken from the CG_DrawInfoPanel function

@author suburb
==================
*/
void CG_UpdateJumpSpeeds(void) {
	int speed = 0;

	if (cg.timerunJumpCounter < cg.predictedPlayerState.identifyClientHealth && cg.timerunJumpCounter < (int)(sizeof (cg.timerunJumpSpeeds) / sizeof (cg.timerunJumpSpeeds[0]))) {
		speed                                       = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);
		cg.timerunJumpSpeeds[cg.timerunJumpCounter] = speed;
		cg.timerunJumpCounter                       = cg.predictedPlayerState.identifyClientHealth;
	}
}

/*
==================
CG_DrawInfoPanel

Print info panel

@author Nico
==================
*/
#define INFO_PANEL_WIDTH                    100
#define INFO_PANEL_HEIGHT                   145
#define INFO_PANEL_X_PADDING                82
#define INFO_PANEL_MAX_COLUMNS              5 // Nico, INFO_PANEL_MAX_COLUMNS * INFO_PANEL_MAX_JUMPS_PER_COLUMN must
#define INFO_PANEL_MAX_JUMPS_PER_COLUMN     9 // stay < size of cg.timerunJumpSpeeds array
#define INFO_PANEL_FONT_ADJUST_NEEDED       10000
#define UPPERRIGHT_X                        CG_WideX(SCREEN_WIDTH - 6)
#define INFO_PANEL_X                        (UPPERRIGHT_X - INFO_PANEL_WIDTH + 3)
#define INFO_PANEL_Y                        2
void CG_DrawInfoPanel(void) {
	float  x         = 0;
	float  y         = 0;
	float  textScale = 0.12f;
	int    starty    = 0;
	int    speed     = 0;
	int    i         = 0;
	vec4_t color;

	if (!etr_drawInfoPanel.integer) {
		return;
	}

	// Update overall max speed
	speed = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);

	if (speed > cg.overallMaxSpeed) {
		cg.overallMaxSpeed = speed;
	}

	x = INFO_PANEL_X + etr_infoPanelXoffset.value;
	y = INFO_PANEL_Y + etr_infoPanelYoffset.value;
	BG_ParseRGBACvar(etr_infoPanelColor1.string, color);

	// Print start speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, color, " Start speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.timerunStartSpeed, INFO_PANEL_FONT_ADJUST_NEEDED);

	// Colour start speed
	if (etr_minStartSpeed.value <= 0 || cg.timerunStartSpeed == 0 || cg.timerunStartSpeed >= etr_minStartSpeed.value) {
		CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, color, va("%d", cg.timerunStartSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else {
		CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, color, va("^1%d", cg.timerunStartSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	textScale = 0.12f;

	// Print stop speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, color, " Stop speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.timerunStopSpeed, INFO_PANEL_FONT_ADJUST_NEEDED);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, color, va("%d", cg.timerunStopSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale = 0.12f;

	// Print run max speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, color, " Run max speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.runMaxSpeed, INFO_PANEL_FONT_ADJUST_NEEDED);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, color, va("%d", cg.runMaxSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale = 0.12f;

	// Print overall max speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, color, " Overall max speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.overallMaxSpeed, INFO_PANEL_FONT_ADJUST_NEEDED);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, color, va("%d", cg.overallMaxSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale = 0.12f;

	// Print jumps count
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, color, " Jumps:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.timerunJumpCounter, INFO_PANEL_FONT_ADJUST_NEEDED);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, color, va("%d", cg.timerunJumpCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	// Print jump speeds
	starty = y;
	for (i = 0; i < cg.timerunJumpCounter && i < INFO_PANEL_MAX_COLUMNS * INFO_PANEL_MAX_JUMPS_PER_COLUMN; ++i) {
		if (i > 0 && i % INFO_PANEL_MAX_JUMPS_PER_COLUMN == 0) {
			y  = starty;
			x += 20;
		}

		textScale = CG_AdjustFontSize(0.12f, cg.timerunJumpSpeeds[i], INFO_PANEL_FONT_ADJUST_NEEDED);

		// If speed at jump n is slower than speed at jump n - 1, use red color
		if (i > 0 && cg.timerunJumpSpeeds[i] < cg.timerunJumpSpeeds[i - 1]) {
			BG_ParseRGBACvar(etr_infoPanelColor2.string, color);
			CG_Text_Paint_Ext(x, y += 10, textScale, textScale, color, va(" %d", cg.timerunJumpSpeeds[i]), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		} else {
			BG_ParseRGBACvar(etr_infoPanelColor1.string, color);
			CG_Text_Paint_Ext(x, y += 10, textScale, textScale, color, va(" %d", cg.timerunJumpSpeeds[i]), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
	}
}

/*
=================
CG_DrawSpectatorState

Draws text on hud if noclipping / in free spectator

@author suburb
=================
*/
void CG_DrawSpectatorState(void) {
	char   *text = NULL;
	float  textScale;
	float  x, y;
	int    w;
	vec4_t color;

	if (!etr_drawSpectatorState.integer) {
		return;
	}

	if (cg.predictedPlayerState.pm_type == PM_NOCLIP) {
		text = "NOCLIP";
	} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		text = "SPECTATOR";
	} else {
		return;
	}

	textScale = 0.3f;

	w = CG_Text_Width_Ext(text, textScale, textScale, &cgs.media.limboFont1);
	x = (CG_WideX(etr_spectatorStateX.value * 2) - w) / 2;
	y = etr_spectatorStateY.value;
	BG_ParseRGBACvar(etr_spectatorStateColor.string, color);

	CG_Text_Paint_Ext(x, y, textScale, textScale, color, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/*
===============
CG_DrawTriggers

Add custom shaders to triggers

@author suburb
===============
*/
#define TRIGGERS_DISTANCE_UPDATE_TIME 1000
void CG_DrawTriggers(void) {
	centity_t    *cent;
	clipHandle_t cmodel;
	qhandle_t    triggerShader, edgesShader;
	vec3_t       mins, maxs, center;

	if (!etr_drawTriggers.integer) {
		return;
	}

	// get distances from player to ents to draw triggers in order of distance, 
	// like this close triggers should always draw despite of max polys count
	if (cg.time > cg.lastGetTriggerDistancesTime + TRIGGERS_DISTANCE_UPDATE_TIME) { // don't do every frame to prevent lag
		cg.drawTriggersCount = 0;
		for (int i = 0; i < MAX_ENTITIES + 1; ++i) { // loop through all entities
			cent = &cg_entities[i];

			// only bother with the following types
			if (etr_drawTriggers.integer == 1) {
				if (cent->currentState.eType != ET_TRIGGER_MULTIPLE &&
					cent->currentState.eType != ET_PUSH_TRIGGER &&
					cent->currentState.eType != ET_TELEPORT_TRIGGER) {
					continue;
				}
			} else { // draw some more, replaces railtrails trigger debug function
				if (cent->currentState.eType != ET_TRIGGER_MULTIPLE &&
					cent->currentState.eType != ET_PUSH_TRIGGER &&
					cent->currentState.eType != ET_TELEPORT_TRIGGER &&
					cent->currentState.eType != ET_TRIGGER_FLAGONLY &&
					cent->currentState.eType != ET_TRIGGER_FLAGONLY_MULTIPLE &&
					cent->currentState.eType != ET_OID_TRIGGER &&
					cent->currentState.eType != ET_CONCUSSIVE_TRIGGER &&
					cent->currentState.eType != ET_CONSTRUCTIBLE) {
					continue;
				}
			}

			// always draw start trigger, it is the trigger that is most likely seen within those 1s of update time
			if (cent->currentState.eType == ET_TRIGGER_MULTIPLE && cent->currentState.nextWeapon == 1) { 
				cg.drawTriggerEntIndexes[0] = cent->currentState.number; // index 0 reserved for start trigger
				cg.drawTriggersCount++;
				continue;
			}

			// get distance to brush center and store ent index
			cmodel = cgs.inlineDrawModel[cent->currentState.modelindex];
			if (!cmodel) {
				continue;
			}
			trap_R_ModelBounds(cmodel, mins, maxs);
			VectorSet(center, (mins[0] + maxs[0]) / 2, (mins[1] + maxs[1]) / 2, (mins[2] + maxs[2]) / 2);
			cg.drawTriggerDistances[cg.drawTriggersCount] = VectorDistance(cg.refdef_current->vieworg, center);
			cg.drawTriggerEntIndexes[cg.drawTriggersCount] = cent->currentState.number;

			// sort by ascending distance
			for (int j = cg.drawTriggersCount; j > 1; --j) { // don't sort index 0
				if (cg.drawTriggerDistances[j] < cg.drawTriggerDistances[j - 1]) {
					float temp = cg.drawTriggerDistances[j - 1];
					cg.drawTriggerDistances[j - 1] = cg.drawTriggerDistances[j];
					cg.drawTriggerDistances[j] = temp;
					int temp2 = cg.drawTriggerEntIndexes[j - 1];
					cg.drawTriggerEntIndexes[j - 1] = cg.drawTriggerEntIndexes[j];
					cg.drawTriggerEntIndexes[j] = temp2;
				}
			}

			cg.drawTriggersCount++;
		}
		cg.lastGetTriggerDistancesTime = cg.time;
	}

	// actually draw triggers
	for (int i = 0; i < cg.drawTriggersCount; ++i) { // loop through relevant entities indexes
		cent = &cg_entities[cg.drawTriggerEntIndexes[i]];

		cmodel = cgs.inlineDrawModel[cent->currentState.modelindex];
		if (!cmodel) {
			continue;
		}
		trap_R_ModelBounds(cmodel, mins, maxs);
		VectorSet(mins, mins[0] - etr_triggersDrawScale.value, mins[1] - etr_triggersDrawScale.value, mins[2] - etr_triggersDrawScale.value);
		VectorSet(maxs, maxs[0] + etr_triggersDrawScale.value, maxs[1] + etr_triggersDrawScale.value, maxs[2] + etr_triggersDrawScale.value);

		triggerShader = cgs.media.customTrigger;
		edgesShader = cgs.media.customTriggerEdges;
		if (cent->currentState.eType == ET_TELEPORT_TRIGGER) {
			triggerShader = cgs.media.teleportTrigger;
			edgesShader = cgs.media.teleportTriggerEdges;
		} else if (cent->currentState.eType == ET_PUSH_TRIGGER) {
			triggerShader = cgs.media.pushTrigger;
			edgesShader = cgs.media.pushTriggerEdges;
		} else if (cent->currentState.eType == ET_TRIGGER_MULTIPLE) {
			if (cent->currentState.nextWeapon == 1) { // use hijacked var for multiple trigger type
				triggerShader = cgs.media.startTrigger;
				edgesShader = cgs.media.startTriggerEdges;
			} else if (cent->currentState.nextWeapon == 2) {
				triggerShader = cgs.media.stopTrigger;
				edgesShader = cgs.media.stopTriggerEdges;
			} else if (cent->currentState.nextWeapon == 3) {
				triggerShader = cgs.media.checkpointTrigger;
				edgesShader = cgs.media.checkpointTriggerEdges;
			}
		}

		CG_AddShaderToBox(mins, maxs, triggerShader, edgesShader, etr_triggersDrawEdges.integer);
	}
}

/*
===============
CG_AddPolyByPoints

Help function of CG_AddShaderToBox

@author suburb
===============
*/
static void CG_AddPolyByPoints(vec3_t p1, vec3_t p2, vec3_t p3, qhandle_t shader) {
	polyVert_t verts[3];

	VectorCopy(p1, verts[0].xyz);
	VectorCopy(p2, verts[1].xyz);
	VectorCopy(p3, verts[2].xyz);

	trap_R_AddPolyToScene(shader, 3, verts);
}

/*
===============
CG_AddShaderToBox

Add custom shaders to boxes using polyVert struct
PolyVerts count is limited by the engine, easily reaching max if using edges

@author suburb
===============
*/
#define TRIGGERS_EDGE_THICKNESS 1.0f
void CG_AddShaderToBox(vec3_t mins, vec3_t maxs, qhandle_t boxShader, qhandle_t edgesShader, int addEdges) {
	vec3_t diff, p1, p2, p3, p4, p5, p6, temp;

	VectorSubtract(mins, maxs, diff);

	VectorCopy(mins, p1);
	VectorCopy(mins, p2);
	VectorCopy(mins, p3);
	VectorCopy(maxs, p4);
	VectorCopy(maxs, p5);
	VectorCopy(maxs, p6);

	p1[0] -= diff[0];
	p2[1] -= diff[1];
	p3[2] -= diff[2];
	p4[0] += diff[0];
	p5[1] += diff[1];
	p6[2] += diff[2];

	// bottom side
	CG_AddPolyByPoints(mins, p1, p2, boxShader);
	CG_AddPolyByPoints(p1, p2, p6, boxShader);

	// front side
	CG_AddPolyByPoints(mins, p2, p4, boxShader);
	CG_AddPolyByPoints(mins, p3, p4, boxShader);

	// back side
	CG_AddPolyByPoints(p1, p5, p6, boxShader);
	CG_AddPolyByPoints(p5, p6, maxs, boxShader);

	// left side
	CG_AddPolyByPoints(p2, p4, maxs, boxShader);
	CG_AddPolyByPoints(p2, p6, maxs, boxShader);

	// right side
	CG_AddPolyByPoints(mins, p1, p5, boxShader);
	CG_AddPolyByPoints(mins, p3, p5, boxShader);

	// top side
	CG_AddPolyByPoints(p4, p5, maxs, boxShader);
	CG_AddPolyByPoints(p3, p4, p5, boxShader);

	if (addEdges) {
		// bottom front edge
		VectorSet(temp, mins[0] + TRIGGERS_EDGE_THICKNESS, mins[1], mins[2] + TRIGGERS_EDGE_THICKNESS);
		CG_AddShaderToBox(temp, p2, edgesShader, edgesShader, 0);

		// bottom back edge
		VectorSet(temp, p1[0] - TRIGGERS_EDGE_THICKNESS, p1[1], p1[2] + TRIGGERS_EDGE_THICKNESS);
		CG_AddShaderToBox(temp, p6, edgesShader, edgesShader, 0);

		// bottom left edge
		VectorSet(temp, p2[0], p2[1] - TRIGGERS_EDGE_THICKNESS, p2[2] + TRIGGERS_EDGE_THICKNESS);
		CG_AddShaderToBox(temp, p6, edgesShader, edgesShader, 0);

		// bottom right edge
		VectorSet(temp, mins[0], mins[1] + TRIGGERS_EDGE_THICKNESS, mins[2] + TRIGGERS_EDGE_THICKNESS);
		CG_AddShaderToBox(temp, p1, edgesShader, edgesShader, 0);

		// front left edge
		VectorSet(temp, p2[0] + TRIGGERS_EDGE_THICKNESS, p2[1] - TRIGGERS_EDGE_THICKNESS, p2[2]);
		CG_AddShaderToBox(temp, p4, edgesShader, edgesShader, 0);

		// front right edge
		VectorSet(temp, mins[0] + TRIGGERS_EDGE_THICKNESS, mins[1] + TRIGGERS_EDGE_THICKNESS, mins[2]);
		CG_AddShaderToBox(temp, p3, edgesShader, edgesShader, 0);

		// back left edge
		VectorSet(temp, p6[0] - TRIGGERS_EDGE_THICKNESS, p6[1] - TRIGGERS_EDGE_THICKNESS, p6[2]);
		CG_AddShaderToBox(temp, maxs, edgesShader, edgesShader, 0);

		// back right edge
		VectorSet(temp, p1[0] - TRIGGERS_EDGE_THICKNESS, p1[1] + TRIGGERS_EDGE_THICKNESS, p1[2]);
		CG_AddShaderToBox(temp, p5, edgesShader, edgesShader, 0);

		// top front edge
		VectorSet(temp, p3[0] + TRIGGERS_EDGE_THICKNESS, p3[1], p3[2] - TRIGGERS_EDGE_THICKNESS);
		CG_AddShaderToBox(temp, p4, edgesShader, edgesShader, 0);

		// top back edge
		VectorSet(temp, p5[0] - TRIGGERS_EDGE_THICKNESS, p5[1], p5[2] - TRIGGERS_EDGE_THICKNESS);
		CG_AddShaderToBox(temp, maxs, edgesShader, edgesShader, 0);

		// top left edge
		VectorSet(temp, p4[0], p4[1] - TRIGGERS_EDGE_THICKNESS, p4[2] - TRIGGERS_EDGE_THICKNESS);
		CG_AddShaderToBox(temp, maxs, edgesShader, edgesShader, 0);

		// top right edge
		VectorSet(temp, p3[0], p3[1] + TRIGGERS_EDGE_THICKNESS, p3[2] - TRIGGERS_EDGE_THICKNESS);
		CG_AddShaderToBox(temp, p5, edgesShader, edgesShader, 0);
	}
}
