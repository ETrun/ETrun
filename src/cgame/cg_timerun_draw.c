#include "cg_local.h"

/**
 * Draw checkpoints times
 *
 * @author Nico
 */
void CG_DrawCheckpoints(void) {
	int x = 0, y = 0;

	if (!cg_drawCheckPoints.integer) {
		return;
	}

	// Nico, printing position
	x = cg_checkPointsX.value;
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

/**
 * Draw speed meter
 *
 * @author Nico
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

	x = cg_speedMeterX.integer;
	y = cg_speedMeterY.integer;

	Com_sprintf(status, sizeof (status), "%.0f", speed);

	w = CG_Text_Width_Ext(status, sizex, sizey, &cgs.media.limboFont1) / 2;

	CG_Text_Paint_Ext(x - w, y, sizex, sizey, colorWhite, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/**
 * OB detector from TJMod
 *
 * @author Nico
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
		CG_DrawStringExt(320, 220, "F", colorWhite, qfalse, qtrue,
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
			CG_DrawStringExt(330, 220, "J", colorWhite, qfalse, qtrue,
			                 TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
	}
}

/**
 * Slick detector
 *
 * @author suburb
 */
#define DRAWSLICK_MAX_DISTANCE 131072
void CG_DrawSlick (void) {
	static trace_t trace;
	vec3_t start, end;
	float x = 350, y = 220, sizex, sizey;
	char *text = "S";

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

/**
 * Draw timer
 *
 * @author Nico
 */
void CG_DrawTimer(void) {
	char       status[128];
	int        min                = 0, sec = 0, milli = 0;
	int        x                  = 0, y = 0, w = 0;
	int        timerunNum         = cg.currentTimerun;
	int        startTime          = 0;
	int        currentTimerunTime = 0;
	float      sizex              = 0.25f, sizey = 0.25f;
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
	x = cg_timerX.integer;
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
		if (runBestTime > 0 && currentTimerunTime > runBestTime) {
			Vector4Set(color, colorRed[0], colorRed[1], colorRed[2], colorRed[3]);
		}

		Com_sprintf(status, sizeof (status), "%s", va("%02d:%02d.%03d", min, sec, milli));
	}

	// Nico, print the timer
	w = CG_Text_Width_Ext(status, sizex, sizey, &cgs.media.limboFont1) / 2;
	CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

// Dzikie CGaz 3rd party functions
#define SCREEN_CENTER_X ((SCREEN_WIDTH / 2) - 1)
#define SCREEN_CENTER_Y ((SCREEN_HEIGHT / 2) - 1)
#define CGAZ3_ANG 20

static void PutPixel(float x, float y) {
	if (x > 0 && x < SCREEN_WIDTH && y > 0 && y < SCREEN_HEIGHT) {
		CG_DrawPic(x, y, 1, 1, cgs.media.whiteShader);
	}
}

static void DrawLine(float x1, float y1, float x2, float y2, vec4_t color) {
	float len, stepx, stepy;
	float i;

	trap_R_SetColor(color);
	len   = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
	len   = sqrt(len);
	stepx = (x2 - x1) / len;
	stepy = (y2 - y1) / len;
	for (i = 0; i < len; ++i) {
		PutPixel(x1, y1);
		x1 += stepx;
		y1 += stepy;
	}
	trap_R_SetColor(NULL);
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

/**
 * Draw CGaz from TJMod
 *
 * @author Nico
 */
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
			accel = pm_accelerate_AP;
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
	scale     = ps->stats[STAT_USERCMD_BUTTONS] & (BUTTON_SPRINT << 8) ? ps->sprintSpeedScale : ps->runSpeedScale;
	per_angle = (ps->speed - accel) / vel_size * scale;
	if (per_angle < 1) {
		per_angle = RAD2DEG(acos(per_angle));
	} else {
		per_angle = ps->viewangles[YAW];
	}

	vel_angle  = AngleNormalize180(RAD2DEG(atan2(vel[1], vel[0])));
	vel_relang = AngleNormalize180(ps->viewangles[YAW] - vel_angle);

	// parse usercmd
	if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_FORWARD) {
		forward = 127;
	} else if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_BACKWARD) {
		forward = -128;
	}

	if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_RIGHT) {
		right = 127;
	} else if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_LEFT) {
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

		DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
		         SCREEN_CENTER_X + right, SCREEN_CENTER_Y - forward, colorCyan);

		vel_size /= 5;
		DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
		         SCREEN_CENTER_X + vel_size * sin(vel_relang),
		         SCREEN_CENTER_Y - vel_size * cos(vel_relang), colorRed);
		if (vel_size > SCREEN_HEIGHT / 2) {
			vel_size = SCREEN_HEIGHT / 2;
		}
		vel_size /= 2;
		DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
		         SCREEN_CENTER_X + vel_size * sin(vel_relang + per_angle),
		         SCREEN_CENTER_Y - vel_size * cos(vel_relang + per_angle), colorRed);
		DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
		         SCREEN_CENTER_X + vel_size * sin(vel_relang - per_angle),
		         SCREEN_CENTER_Y - vel_size * cos(vel_relang - per_angle), colorRed);
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

/**
 * Draw keys from TJMod
 *
 * @author Nico
 */
void CG_DrawKeys(void) {
	playerState_t *ps;
	float         x, y, size;
	int           i;
	int           skew;

	if (cg_drawKeys.integer <= 0) {
		return;
	}

	// some checks
	if (cg_keysX.value < 0 || cg_keysY.value < 0 || cg_keysX.value > SCREEN_WIDTH || cg_keysY.value > SCREEN_HEIGHT) {
		return;
	}

	// Dini, add in here for any other keysets with skew effect or related.
	if (cg_drawKeys.value == 1) {
		skew = cg_keysSize.value / 18;
	} else {
		skew = 0;
	}

	ps = &cg.predictedPlayerState;

	trap_R_SetColor(colorWhite);

	size = cg_keysSize.value / 3;
	i    = (cg_drawKeys.integer - 1) % NUM_KEYS_SETS;

	// first (upper) row
	// sprint (upper left)
	x = cg_keysX.value + 2 * skew;
	y = cg_keysY.value;
	if (ps->stats[STAT_USERCMD_BUTTONS] & (BUTTON_SPRINT << 8)) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].SprintPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].SprintNotPressedShader);
	}

	// forward
	x += size;
	if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_FORWARD) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].ForwardPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].ForwardNotPressedShader);
	}

	// jump (upper right)
	x += size;
	if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_UP) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].JumpPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].JumpNotPressedShader);
	}

	// second (middle) row
	// left
	x  = cg_keysX.value + skew;
	y += size;
	if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_LEFT) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].LeftPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].LeftNotPressedShader);
	}

	// right
	x += 2 * size;
	if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_RIGHT) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].RightPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].RightNotPressedShader);
	}

	// third (bottom) row
	x  = cg_keysX.value;
	y += size;
	// prone (bottom left)
	if (ps->stats[STAT_USERCMD_BUTTONS] & WBUTTON_PRONE) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].PronePressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].ProneNotPressedShader);
	}

	// backward
	x += size;
	if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_BACKWARD) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].BackwardPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].BackwardNotPressedShader);
	}

	// crouch (bottom right)
	x += size;
	if (ps->stats[STAT_USERCMD_MOVE] & UMOVE_DOWN) {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].CrouchPressedShader);
	} else {
		CG_DrawPic(x, y, size, size, cgs.media.keys[i].CrouchNotPressedShader);
	}
}

/**
 * Draw clock from TJMod
 *
 * @author Nico
 */
void CG_DrawClock(float x, float y, float scale, qboolean shadowed) {
	char    displayTime[18] = { 0 };
	qtime_t tm;
	vec4_t  clr = { 1.0f, 1.0f, 1.0f, 0.8f };

	trap_RealTime(&tm);
	displayTime[0] = '\0';

	Q_strcat(displayTime, sizeof (displayTime),
	         va("Time: %d:%02d",
	            ((tm.tm_hour == 0 || tm.tm_hour == 12) ? 12 : tm.tm_hour % 12),
	            tm.tm_min));
	Q_strcat(displayTime, sizeof (displayTime),
	         va(":%02d", tm.tm_sec));
	Q_strcat(displayTime, sizeof (displayTime),
	         (tm.tm_hour < 12) ? " am" : " pm");

	if (shadowed == qtrue) {
		CG_Text_Paint_Ext(x, y, scale, scale, clr, displayTime, 0, 24, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else {
		CG_Text_Paint_Ext(x, y, scale, scale, clr, displayTime, 0, 24, 0, &cgs.media.limboFont1);
	}
}

/**
 * Print banner
 * @source: TJMod
 *
 * @author Nico
 */
void CG_DrawBannerPrint(void) {
	char  *start = cg.bannerPrint;
	int   l      = 0;
	int   y      = 20;
	float *color;
	float sizex     = 0.2f, sizey = 0.2f;
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

	for (;; ) {
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

		x = (SCREEN_WIDTH - w) / 2;

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

/**
* Update jump speeds
*
* @author suburb (taken from the CG_DrawInfoPanel function)
*/
void CG_UpdateJumpSpeeds(void){
	int speed = 0;

	if (cg.timerunJumpCounter < cg.predictedPlayerState.identifyClientHealth && cg.timerunJumpCounter < (int)(sizeof(cg.timerunJumpSpeeds) / sizeof(cg.timerunJumpSpeeds[0]))) {
		speed = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);
		cg.timerunJumpSpeeds[cg.timerunJumpCounter] = speed;
		cg.timerunJumpCounter = cg.predictedPlayerState.identifyClientHealth;
	}
}

/**
 * Print info panel
 *
 * @author Nico
 */
#define INFO_PANEL_WIDTH                    100
#define INFO_PANEL_HEIGHT                   145
#define INFO_PANEL_X_PADDING                82
#define INFO_PANEL_MAX_COLUMNS              5 // Nico, INFO_PANEL_MAX_COLUMNS * INFO_PANEL_MAX_JUMPS_PER_COLUMN must
#define INFO_PANEL_MAX_JUMPS_PER_COLUMN     9 // stay < size of cg.timerunJumpSpeeds array
#define INFO_PANEL_FONT_ADJUST_NEEDED       10000
void CG_DrawInfoPanel(void) {
	int    x                = 0;
	int    y                = 0;
	int    starty           = 0;
	vec4_t panelBgColor     = { 0.f, 0.f, 0.f, .5f };
	vec4_t textColor        = { 1.0f, 1.0f, 1.0f, 0.8f };
	float  textScale        = 0.12f;
	float  textScaleFactor  = 0;
	int    speed            = 0;
	int    i                = 0;

	if (!cg_drawInfoPanel.integer) {
		return;
	}

	// Perform some checks
	if (cg_infoPanelX.value < 0 || cg_infoPanelY.value < 0 || cg_infoPanelX.value > SCREEN_WIDTH || cg_infoPanelY.value > SCREEN_HEIGHT) {
		return;
	}

	// Update overall max speed
	speed = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);

	if (speed > cg.overallMaxSpeed) {
		cg.overallMaxSpeed = speed;
	}

	x = cg_infoPanelX.value;
	y = cg_infoPanelY.value;

	CG_FillRect(x, y, INFO_PANEL_WIDTH, INFO_PANEL_HEIGHT, panelBgColor);
	CG_DrawRect_FixedBorder(x, y, INFO_PANEL_WIDTH, INFO_PANEL_HEIGHT, 1, colorBlack);

	// Print start speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, textColor, " Start speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	// Colored start speed
	if (cg_minStartSpeed.value <= 0 || cg.timerunStartSpeed == 0 || cg.timerunStartSpeed >= cg_minStartSpeed.value) {
		CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, textColor, va("%d", cg.timerunStartSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	} else {
		CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, textColor, va("^1%d", cg.timerunStartSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	}

	// Print stop speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, textColor, " Stop speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, textColor, va("%d", cg.timerunStopSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	// Print run top speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, textColor, " Run max speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, textColor, va("%d", cg.runMaxSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	// Print overall max speed
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, textColor, " Overall max speed:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, textColor, va("%d", cg.overallMaxSpeed), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	// Print jumps count
	CG_Text_Paint_Ext(x, y += 10, textScale, textScale, textColor, " Jumps:", 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
	CG_Text_Paint_Ext(x + INFO_PANEL_X_PADDING, y, textScale, textScale, textColor, va("%d", cg.timerunJumpCounter), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

	// Print jump speeds
	starty = y;
	for (i = 0; i < cg.timerunJumpCounter && i < INFO_PANEL_MAX_COLUMNS * INFO_PANEL_MAX_JUMPS_PER_COLUMN; ++i) {
		if (i > 0 && i % INFO_PANEL_MAX_JUMPS_PER_COLUMN == 0) {
			y  = starty;
			x += 20;
		}

		// suburb, decrease font size for higher speeds
		textScale = 0.12f;

		if (cg.timerunJumpSpeeds[i] >= INFO_PANEL_FONT_ADJUST_NEEDED){
			textScaleFactor = 0.02f * GetDigits(cg.timerunJumpSpeeds[i]) - 0.08f;
			textScale -= textScaleFactor;
		}

		// If speed at jump n is slower than speed at jump n - 1, use red color
		if (i > 0 && cg.timerunJumpSpeeds[i] < cg.timerunJumpSpeeds[i - 1]) {
			CG_Text_Paint_Ext(x, y += 10, textScale, textScale, textColor, va(" ^1%d", cg.timerunJumpSpeeds[i]), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		} else {
			CG_Text_Paint_Ext(x, y += 10, textScale, textScale, textColor, va(" %d", cg.timerunJumpSpeeds[i]), 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
		}
	}
}
