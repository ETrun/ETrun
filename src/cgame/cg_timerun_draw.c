#include "cg_local.h"

/*
==================
CG_DrawCheckpoints

Draw checkpoints times

@author Nico
==================
*/
void CG_DrawCheckpoints(void) {
	int x = 0, y = 0;

	if (!cg_drawCheckPoints.integer) {
		return;
	}

	// Nico, printing position
	x = CG_WideX(cg_checkPointsX.value);
	y = cg_checkPointsY.value;

	// Nico, check cg_maxCheckPoints
	if (!cg_maxCheckPoints.integer || cg_maxCheckPoints.integer < 0) {
		cg_maxCheckPoints.integer = 5;
	}

	// Nico, print check points if any and respect the printing limit (cg_maxCheckPoints)
	if (cg.timerunCheckPointChecked > 0) {
		int i, j;

		for (i = cg.timerunCheckPointChecked - 1, j = 0; i >= 0 && j < cg_maxCheckPoints.integer; --i, ++j) {
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
			Vector4Set(color, colorWhite[0], colorWhite[1], colorWhite[2], colorWhite[3]);

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
				Vector4Set(color, colorGreen[0], colorGreen[1], colorGreen[2], colorGreen[3]);
				Com_sprintf(status, sizeof (status), "%s", va("-%02d:%02d.%03d", cdmin, cdsec, cdmil));
				break;

			case 3:
				// Nico, slower check point time
				Vector4Set(color, colorRed[0], colorRed[1], colorRed[2], colorRed[3]);
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
	float        sizex, sizey;
	int          x, y, w;
	static vec_t speed;

	if (!cg_drawSpeedMeter.integer) {
		return;
	}

	speed = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);

	// Nico, cg.predictedPlayerState.velocity is sometimes NaN (example on asdarun1), so check it
	// http://stackoverflow.com/questions/570669/checking-if-a-double-or-float-is-nan-in-c
	if (speed != speed) {
		speed = 0;
	}

	sizex = sizey = 0.25f;

	x = CG_WideX(cg_speedMeterX.integer);
	y = cg_speedMeterY.integer;

	Com_sprintf(status, sizeof (status), "%.0f", speed);

	w = CG_Text_Width_Ext(status, sizex, sizey, &cgs.media.limboFont1) / 2;

	if (cg_drawAccel.integer && speed > cg.oldSpeed + 0.001f * cg_accelSmoothness.integer) {
		CG_Text_Paint_Ext(x - w, y, sizex, sizey, colorGreen, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else if (cg_drawAccel.integer && speed < cg.oldSpeed - 0.001f * cg_accelSmoothness.integer) {
		CG_Text_Paint_Ext(x - w, y, sizex, sizey, colorRed, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else {
		CG_Text_Paint_Ext(x - w, y, sizex, sizey, colorWhite, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
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
	float   psec;
	int     gravity;
	vec3_t  snap;
	float   rintv;
	float   v0;
	float   h0, hn;
	float   t;
	trace_t trace;
	vec3_t  start, end;
	float   n2;
	int     n;

	if (!cg_drawOB.integer || cg_thirdPerson.integer || physics.integer & PHYSICS_NO_OVERBOUNCE) {
		return;
	}

	psec    = pmove_msec.integer / 1000.0;
	gravity = cg.predictedPlayerState.gravity;
	v0      = cg.predictedPlayerState.velocity[2];
	h0      = cg.predictedPlayerState.origin[2] + cg.predictedPlayerState.mins[2];
	//CG_Printf("psec: %f, gravity: %d\n", psec, gravity);

	VectorSet(snap, 0, 0, gravity * psec);
	trap_SnapVector(snap);
	rintv = snap[2];

	// use origin from playerState?
	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, 131072, cg.refdef.viewaxis[0], end);

	CG_Trace(&trace, start, vec3_origin, vec3_origin, end,
	         cg.predictedPlayerState.clientNum, CONTENTS_SOLID);

	// we didn't hit anything
	if (trace.fraction == 1.0) {
		return;
	}

	// not a floor
	if (trace.plane.type != 2) {
		return;
	}

	t = trace.endpos[2];
	//CG_Printf("h0: %f, t: %f\n", h0, t);

	// fall ob
	a = -psec * rintv / 2;
	b = psec * (v0 - gravity * psec / 2 + rintv / 2);
	c = h0 - t;
	// n1 = (-b + sqrt(b * b - 4 * a * c)) / (2 * a);
	n2 = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);
	//CG_Printf("%f, %f\n", n1, n2);

	n  = floor(n2);
	hn = h0 + psec * n * (v0 - gravity * psec / 2 - (n - 1) * rintv / 2);
	//CG_Printf("h0: %f, v0: %f, n: %d, hn: %f, t: %f\n", h0, v0, n, hn, t);
	if (n && hn < t + 0.25 && hn > t) {
		CG_DrawStringExt(CG_WideX(SCREEN_WIDTH) / 2, 220, "F", colorWhite, qfalse, qtrue,
		                 TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
	}

	if (cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE) {
		// jump ob
		v0 += 270; // JUMP_VELOCITY
		b   = psec * (v0 - gravity * psec / 2 + rintv / 2);
		// n1 = (-b + sqrt(b * b - 4 * a * c ) ) / (2 * a);
		n2 = (-b - sqrt(b * b - 4 * a * c)) / (2 * a);
		//CG_Printf("%f, %f\n", n1, n2);

		n  = floor(n2);
		hn = h0 + psec * n * (v0 - gravity * psec / 2 - (n - 1) * rintv / 2);
		//CG_Printf("h0: %f, v0: %f, n: %d, hn: %f, t: %f\n", h0, v0, n, hn, t);
		if (hn < t + 0.25 && hn > t) {
			CG_DrawStringExt(CG_WideX(SCREEN_WIDTH) / 2 + 10, 220, "J", colorWhite, qfalse, qtrue,
			                 TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
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
#define DRAWSLICK_MAX_DISTANCE 131072
void CG_DrawSlick(void) {
	static trace_t trace;
	vec3_t         start, end;
	float          x = CG_WideX(350), y = 220, sizex, sizey;
	char           *text = "S";

	if (!cg_drawSlick.integer || cg_thirdPerson.integer) {
		return;
	}

	sizex = sizey = 0.2f;

	VectorCopy(cg.refdef.vieworg, start);
	VectorMA(start, DRAWSLICK_MAX_DISTANCE, cg.refdef.viewaxis[0], end);
	CG_Trace(&trace, start, vec3_origin, vec3_origin, end, cg.predictedPlayerState.clientNum, CONTENTS_SOLID);

	if (trace.surfaceFlags & SURF_SLICK) {
		CG_Text_Paint_Ext(x, y, sizex, sizey, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
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
	int        x = 0, y = 0, w = 0;
	int        timerunNum = cg.currentTimerun;
	int        startTime = 0;
	int        currentTimerunTime = 0;
	float      sizex = 0.25f, sizey = 0.25f;
	vec4_t     color;
	int        runBestTime;
	int        runLastTime;
	int        clientNum  = cg.clientNum; // Nico, player client num or spectated player client num
	static int needsReset = 0;

	// Nico, if level is not a timer
	if (!isTimerun.integer) {
		return;
	}

	// Nico, if cg_drawTimer is 0
	if (!cg_drawTimer.integer) {
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
	x = CG_WideX(cg_timerX.integer);
	y = cg_timerY.integer;

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
	Vector4Set(color, colorWhite[0], colorWhite[1], colorWhite[2], colorWhite[3]);

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
				Vector4Set(color, colorGreen[0], colorGreen[1], colorGreen[2], colorGreen[3]);
				Com_sprintf(status, sizeof (status), "%s", va("%02d:%02d.%03d (-%02d:%02d.%03d)", min, sec, milli, dmin, dsec, dmilli));
			} else {
				// Nico, did a slower time
				Vector4Set(color, colorRed[0], colorRed[1], colorRed[2], colorRed[3]);
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
			Vector4Set(color, colorRed[0], colorRed[1], colorRed[2], colorRed[3]);
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
	float y = 260; // Y pos
	float h = 20; // CGaz height
	float w = 300; // CGaz width

	int forward = 0;
	int right   = 0;

	vec_t  vel_size;
	vec3_t vel;
	vec4_t color;

	playerState_t *ps;

	// Nico, if cg_drawCGaz is 0
	if (!cg_drawCGaz.integer) {
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
			if (cg_drawCGaz.integer == 2) {
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

	if (cg_drawCGaz.integer == 1) {
		VectorCopy(colorBlue, color);
		CG_FillRect(SCREEN_CENTER_X - w / 2, y, w, h, color);
		CG_FillRect(SCREEN_CENTER_X - w / 2, y, 1, h, colorWhite);
		CG_FillRect(SCREEN_CENTER_X - w / 4, y, 1, h, colorWhite);
		CG_FillRect(SCREEN_CENTER_X, y, 1, h, colorWhite);
		CG_FillRect(SCREEN_CENTER_X + w / 4, y, 1, h, colorWhite);
		CG_FillRect(SCREEN_CENTER_X + w / 2, y, 1, h, colorWhite);

		if (vel_size < ps->speed * scale) {
			return;
		}

		// velocity
		if (vel_relang > -90 && vel_relang < 90) {
			CG_FillRect(SCREEN_CENTER_X + w * vel_relang / 180, y, 1, h, colorOrange);
		}

		// left/right perfect strafe
		if (vel_relang > 0) {
			ang = AngleNormalize180(vel_relang - per_angle);
			if (ang > -90 && ang < 90) {
				CG_FillRect(SCREEN_CENTER_X + w * ang / 180, y, 1, h, colorGreen);
			}
		} else {
			ang = AngleNormalize180(vel_relang + per_angle);
			if (ang > -90 && ang < 90) {
				CG_FillRect(SCREEN_CENTER_X + w * ang / 180, y, 1, h, colorGreen);
			}
		}
	} else if (cg_drawCGaz.integer == 2) {
		// CGaz 2
		vel_relang = DEG2RAD(vel_relang);
		per_angle  = DEG2RAD(per_angle);

		if (!cg_realCGaz2.integer && cg_widescreenSupport.integer) {
			CG_DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
				SCREEN_CENTER_X + CG_WideX(right), SCREEN_CENTER_Y - forward, colorCyan);
		} else {
			CG_DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
				SCREEN_CENTER_X + right, SCREEN_CENTER_Y - forward, colorCyan);
		}

		vel_size /= 5;
		if (vel_size > CG_WideX(SCREEN_WIDTH) / 2) {
			vel_size = CG_WideX(SCREEN_WIDTH) / 2;
		}

		CG_DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
			SCREEN_CENTER_X + vel_size * sin(vel_relang),
			SCREEN_CENTER_Y - vel_size * cos(vel_relang), colorRed);

		if (vel_size > SCREEN_HEIGHT / 2) {
			vel_size = SCREEN_HEIGHT / 2;
		}
		vel_size /= 2;

		if (!cg_realCGaz2.integer && cg_widescreenSupport.integer) {
			CG_DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
				SCREEN_CENTER_X + vel_size * CG_WideX(sin(vel_relang + per_angle)),
				SCREEN_CENTER_Y - vel_size * cos(vel_relang + per_angle), colorRed);
			CG_DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
				SCREEN_CENTER_X + vel_size * CG_WideX(sin(vel_relang - per_angle)),
				SCREEN_CENTER_Y - vel_size * cos(vel_relang - per_angle), colorRed);
		} else {
			CG_DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
				SCREEN_CENTER_X + vel_size * sin(vel_relang + per_angle),
				SCREEN_CENTER_Y - vel_size * cos(vel_relang + per_angle), colorRed);
			CG_DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
				SCREEN_CENTER_X + vel_size * sin(vel_relang - per_angle),
				SCREEN_CENTER_Y - vel_size * cos(vel_relang - per_angle), colorRed);
		}
	} else if (cg_drawCGaz.integer == 3) {
		accel_angle = atan2(-right, forward);
		accel_angle = AngleNormalize180(ps->viewangles[YAW] + RAD2DEG(accel_angle));

		CG_FillRect(SCREEN_CENTER_X - w / 2, y, 1, h, colorWhite);
		CG_FillRect(SCREEN_CENTER_X, y, 1, h, colorWhite);
		CG_FillRect(SCREEN_CENTER_X + w / 2, y, 1, h, colorWhite);

		if (vel_size < ps->speed * scale) {
			return;
		}

		// first case (consider left strafe)
		ang = AngleNormalize180(vel_angle + per_angle - accel_angle);
		if (ang > -CGAZ3_ANG && ang < CGAZ3_ANG) {
			// acceleration "should be on the left side" from velocity
			if (ang < 0) {
				VectorCopy(colorGreen, color);
				CG_FillRect(SCREEN_CENTER_X + w * ang / (2 * CGAZ3_ANG), y, -w * ang / (2 * CGAZ3_ANG), h, color);
			} else {
				VectorCopy(colorRed, color);
				CG_FillRect(SCREEN_CENTER_X, y, w * ang / (2 * CGAZ3_ANG), h, color);
			}
			return;
		}
		// second case (consider right strafe)
		ang = AngleNormalize180(vel_angle - per_angle - accel_angle);
		if (ang > -CGAZ3_ANG && ang < CGAZ3_ANG) {
			// acceleration "should be on the right side" from velocity
			if (ang > 0) {
				VectorCopy(colorGreen, color);
				CG_FillRect(SCREEN_CENTER_X, y, w * ang / (2 * CGAZ3_ANG), h, color);
			} else {
				VectorCopy(colorRed, color);
				CG_FillRect(SCREEN_CENTER_X + w * ang / (2 * CGAZ3_ANG), y, -w * ang / (2 * CGAZ3_ANG), h, color);
			}
			return;
		}
	} else if (cg_drawCGaz.integer == 4) {
		accel_angle = atan2(-right, forward);
		accel_angle = AngleNormalize180(ps->viewangles[YAW] + RAD2DEG(accel_angle));

		if (vel_size < ps->speed * scale) {
			return;
		}

		VectorCopy(colorGreen, color);

		// Speed direction
		// FIXME: Shows @side of screen when going backward
		//CG_FillRect(SCREEN_CENTER_X + w * vel_relang / 180, y+20, 1, h/2, colorCyan);
		CG_DrawPic(SCREEN_CENTER_X + w * vel_relang / 180, y + h, 16, 16, cgs.media.CGazArrow);

		// FIXME show proper outside border where you stop accelerating
		// first case (consider left strafe)
		ang = AngleNormalize180(vel_angle + per_angle - accel_angle);
		if (ang < 90 && ang > -90) {
			// acceleration "should be on the left side" from velocity
			CG_FillRect(SCREEN_CENTER_X + w * ang / 180, y, w / 2, h, color);
			CG_FillRect(SCREEN_CENTER_X + w * ang / 180, y, 1, h, colorGreen);
			return;

		}
		// second case (consider right strafe)
		ang = AngleNormalize180(vel_angle - per_angle - accel_angle);
		if (ang < 90 && ang > -90) {
			// acceleration "should be on the right side" from velocity
			CG_FillRect(SCREEN_CENTER_X + w * ang / 180 - w / 2, y, w / 2, h, color);
			CG_FillRect(SCREEN_CENTER_X + w * ang / 180, y, 1, h, colorGreen);
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
	vec4_t color[3];
	qboolean AP = (physics.integer == PHYSICS_MODE_AP_NO_OB || physics.integer == PHYSICS_MODE_AP_OB);
	float  rgba1[4]  = { 0.4f, 0, 0, 0.5f };
	float  rgba2[4]  = { 0, 0.4f, 0.4f, 0.5f };
	float  step      = 0;
	float  yaw       = 0;
	int    snapHud_H = 0;
	int    snapHud_Y = 0;
	int    colorID   = 0;
	int    fov       = 0;
	int    i         = 0;

	if (!cg_drawVelocitySnapping.integer || (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR)) {
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
	
	if (AP) { // only shift for AP physics
		yaw += 45;
	}
	
	snapHud_H = cg_velocitySnappingH.integer;
	snapHud_Y = cg_velocitySnappingY.integer;
	fov       = cg_velocitySnappingFov.integer;

	if (cg_drawVelocitySnapping.integer == 2) {
		for (int i = 0; i < 4; i++) {
			color[0][i] = 1.0f;
			color[1][i] = 1.0f;
		}
	} else {
		for (int i = 0; i < 4; i++) {
			color[0][i] = rgba1[i];
			color[1][i] = rgba2[i];
		}
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
#define KEYS_X   (CG_WideX(SCREEN_WIDTH) - 90)
#define KEYS_Y   210
#define DRAWKEYS_DEBOUNCE_VALUE        100
#define DRAWKEYS_MENU_CLOSING_DELAY    50
void CG_DrawKeys(void) {
	float x, y, size;
	int   i;
	int   skew;

	if (!cg_drawKeys.integer) {
		return;
	}

	// Dini, add in here for any other keysets with skew effect or related.
	if (cg_drawKeys.value == 1) {
		skew = cg_keysSize.value / 18;
	} else {
		skew = 0;
	}

	trap_R_SetColor(colorWhite);

	size = cg_keysSize.value / 3;
	i    = (cg_drawKeys.integer - 1) % NUM_KEYS_SETS;

	// first (upper) row
	// sprint (upper left)

	x = KEYS_X + cg_keysXoffset.value + 2 * skew;
	y = KEYS_Y + cg_keysYoffset.value;

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
	x  = KEYS_X + cg_keysXoffset.value + skew;
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
	x  = KEYS_X + cg_keysXoffset.value;
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
	static char displayTime[18] = { 0 };
	qtime_t     tm;

	trap_RealTime(&tm);
	displayTime[0] = '\0';
	Q_strcat(displayTime, sizeof (displayTime), va("%d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec));

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
	int   y      = 20;
	float *color;
	float sizex = 0.2f, sizey = 0.2f;
	char  lastcolor = COLOR_WHITE;
	int   charHeight;
	int   bannerShowTime = 10000;
	int   len;

	if (!cg.bannerPrintTime) {
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

	for (;;) {
		char linebuffer[1024];
		char colorchar = lastcolor;
		int  x, w;

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

		x = (CG_WideX(SCREEN_WIDTH) - w) / 2;

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
	int   x         = 0;
	int   y         = 0;
	int   starty    = 0;
	float textScale = 0.12f;
	int   speed     = 0;
	int   i         = 0;

	if (!cg_drawInfoPanel.integer) {
		return;
	}

	// Update overall max speed
	speed = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);

	if (speed > cg.overallMaxSpeed) {
		cg.overallMaxSpeed = speed;
	}

	x = INFO_PANEL_X + cg_infoPanelXoffset.value;
	y = INFO_PANEL_Y + cg_infoPanelYoffset.value;

	//CG_DrawRect_FixedBorder(x, y, INFO_PANEL_WIDTH, INFO_PANEL_HEIGHT, 1, colorWhite);

	// Print start speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, colorWhite, " Start speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.timerunStartSpeed, INFO_PANEL_FONT_ADJUST_NEEDED);

	// Colour start speed
	if (cg_minStartSpeed.value <= 0 || cg.timerunStartSpeed == 0 || cg.timerunStartSpeed >= cg_minStartSpeed.value) {
		CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, colorWhite, va("%d", cg.timerunStartSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else {
		CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, colorWhite, va("^1%d", cg.timerunStartSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}
	textScale = 0.12f;

	// Print stop speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, colorWhite, " Stop speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.timerunStopSpeed, INFO_PANEL_FONT_ADJUST_NEEDED);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, colorWhite, va("%d", cg.timerunStopSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale = 0.12f;

	// Print run max speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, colorWhite, " Run max speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.runMaxSpeed, INFO_PANEL_FONT_ADJUST_NEEDED);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, colorWhite, va("%d", cg.runMaxSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale = 0.12f;

	// Print overall max speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, colorWhite, " Overall max speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.overallMaxSpeed, INFO_PANEL_FONT_ADJUST_NEEDED);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, colorWhite, va("%d", cg.overallMaxSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale = 0.12f;

	// Print jumps count
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, colorWhite, " Jumps:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	textScale               = CG_AdjustFontSize(textScale, cg.timerunJumpCounter, INFO_PANEL_FONT_ADJUST_NEEDED);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, colorWhite, va("%d", cg.timerunJumpCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

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
			CG_Text_Paint_Ext(x, y += 10, textScale, textScale, colorWhite, va(" ^1%d", cg.timerunJumpSpeeds[i]), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		} else {
			CG_Text_Paint_Ext(x, y += 10, textScale, textScale, colorWhite, va(" %d", cg.timerunJumpSpeeds[i]), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
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
	char  *text = NULL;
	float textScale;
	int   x, y, w;

	if (cg.predictedPlayerState.pm_type == PM_NOCLIP) {
		text = "NOCLIP";
	} else if (cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		text = "SPECTATOR";
	} else {
		return;
	}

	textScale = 0.3f;

	x = CG_WideX(SCREEN_WIDTH) / 2;
	y = SCREEN_HEIGHT / 6;

	w = CG_Text_Width_Ext(text, textScale, textScale, &cgs.media.limboFont1) / 2;

	CG_Text_Paint_Ext(x - w, y, textScale, textScale, colorWhite, text, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}
