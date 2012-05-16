#include "cg_local.h"

/* Draw checkpoints times
 *
 * @author Nico
 */
void CG_DrawCheckpoints(void) {
	char status[128];
	int i = 0;
	int j = 0;
	int	cmin = 0, csec = 0, cmil = 0;
	int	cdmin = 0, cdsec = 0, cdmil = 0;
	float sizex = 0.2f, sizey = 0.2f;
	int x = 0, y = 0, w = 0;
	vec4_t color;

	// Nico, checkpoints
	if (cg_drawCheckPoints.integer) {

		// Nico, printing position
		x = cg_checkPointsX.value;
		y = cg_checkPointsY.value;

		// Nico, check cg_maxCheckPoints
		if (!cg_maxCheckPoints.integer || cg_maxCheckPoints.integer < 0) {
			cg_maxCheckPoints.integer = 5;
		}

		// Nico, print check points if any and respect the printing limit (cg_maxCheckPoints)
		if (cg.timerunCheckPointChecked > 0) {
			for (i = cg.timerunCheckPointChecked - 1, j = 0; i >= 0 && j < cg_maxCheckPoints.integer; --i, ++j) {
				cmil = cg.timerunCheckPointTime[i];
				cmin = cmil / 60000;
				cmil -= cmin * 60000;
				csec = cmil / 1000;
				cmil -= csec * 1000;

				cdmil = cg.timerunCheckPointDiff[i];
				cdmin = cdmil / 60000;
				cdmil -= cdmin * 60000;
				cdsec = cdmil / 1000;
				cdmil -= cdsec * 1000;

				// Nico, set checkpoint default color
				Vector4Set(color, colorWhite[0], colorWhite[1], colorWhite[2], colorWhite[3]);

				// Nico, no best time yet, print the check point times
				if (!cg.timerunBestTime[cg.currentTimerun]) {
					Com_sprintf(status, sizeof(status), "%s", va("%02d:%02d.%03d", cmin, csec, cmil));
				} else if (cg.timerunCheckPointDiff[i] == 0) {
					// Nico, same check point time
					Com_sprintf(status, sizeof(status), "%s", va("%02d:%02d.%03d", cdmin, cdsec, cdmil));
				} else if (cg.timerunCheckIsFaster[i] == 1) {
					// Nico, faster check point time
					Vector4Set(color, colorGreen[0], colorGreen[1], colorGreen[2], colorGreen[3]);
					Com_sprintf(status, sizeof(status), "%s", va("-%02d:%02d.%03d", cdmin, cdsec, cdmil));
				} else {
					// Nico, slower check point time
					Vector4Set(color, colorRed[0], colorRed[1], colorRed[2], colorRed[3]);
					Com_sprintf(status, sizeof(status), "%s", va("+%02d:%02d.%03d", cdmin, cdsec, cdmil));
				}

				// Nico, print the check point
				w = CG_Text_Width_Ext( status, sizex, sizey, &cgs.media.limboFont1 ) / 2;
				CG_Text_Paint_Ext(x - w, y, sizex, sizey, color, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);

				// Nico, line jump
				y += 10;
			}
		}
	}
}

/* Draw speed meter
 *
 * @author Nico
 */
void CG_DrawSpeedMeter(void) {
	char status[128];
	float sizex, sizey;
	int x, y, w;
	static vec_t speed;	

	if (!cg_drawSpeedMeter.integer) {
		return;
	}

	speed = sqrt(cg.predictedPlayerState.velocity[0] * cg.predictedPlayerState.velocity[0] + cg.predictedPlayerState.velocity[1] * cg.predictedPlayerState.velocity[1]);

	sizex = sizey = 0.25f;

	x = cg_speedMeterX.integer;
	y = cg_speedMeterY.integer;
	
	Com_sprintf(status, sizeof(status), "%.0f", speed);

	w = CG_Text_Width_Ext( status, sizex, sizey, &cgs.media.limboFont1 ) / 2;

	CG_Text_Paint_Ext(x - w, y, sizex, sizey, colorWhite, status, 0, 0, ITEM_TEXTSTYLE_SHADOWED, &cgs.media.limboFont1);
}

/* OB detector from TJMod
 *
 * @author Nico
 */
void CG_DrawOB(void) {
	double a, b, c;
	float psec;
	int gravity;
	vec3_t snap;
	float rintv;
	float v0;
	float h0, hn;
	float t;
	trace_t trace;
	vec3_t start, end;
	float n2;
	int n;

	if (!cg_drawOB.integer || cg_thirdPerson.integer || physics.integer & PHYSICS_NO_OVERBOUNCE) {
		return;
	}

	psec = pmove_msec.integer / 1000.0;
	gravity = cg.predictedPlayerState.gravity;
	v0 = cg.predictedPlayerState.velocity[2];
	h0 = cg.predictedPlayerState.origin[2] + cg.predictedPlayerState.mins[2];
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

	n = floor(n2);
	hn = h0 + psec * n * (v0 - gravity * psec / 2 - (n - 1) * rintv / 2);
	//CG_Printf("h0: %f, v0: %f, n: %d, hn: %f, t: %f\n", h0, v0, n, hn, t);
	if (n && hn < t + 0.25 && hn > t) {
		CG_DrawStringExt(320, 220, "F", colorWhite, qfalse, qtrue,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
	}

	if (cg.predictedPlayerState.groundEntityNum != ENTITYNUM_NONE) {
		// jump ob
		v0 += 270; // JUMP_VELOCITY
		b = psec * (v0 - gravity * psec / 2 + rintv / 2);
		// n1 = (-b + sqrt(b * b - 4 * a * c ) ) / (2 * a);
		n2 = (-b - sqrt(b * b - 4 * a * c ) ) / (2 * a);
		//CG_Printf("%f, %f\n", n1, n2);

		n = floor(n2);
		hn = h0 + psec * n * (v0 - gravity * psec / 2 - ( n - 1 ) * rintv / 2);
		//CG_Printf("h0: %f, v0: %f, n: %d, hn: %f, t: %f\n", h0, v0, n, hn, t);
		if (hn < t + 0.25 && hn > t) {
			CG_DrawStringExt(330, 220, "J", colorWhite, qfalse, qtrue,
					TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
		}
	}
}

/* Draw timer
 *
 * @author Nico
 */
void CG_DrawTimer(void) {
	char status[128];
	int	min = 0, sec = 0, milli = 0;
	int	dmin = 0, dsec = 0, dmilli = 0;
	int x = 0, y = 0, w = 0;
	int timerunNum = 0;
	int startTime = 0;
	int currentTimerunTime = 0;
	float sizex = 0.25f, sizey = 0.25f;
	vec4_t	color;
	static int needsReset = 0;
	
	// Nico, if level is not a timer
	if (!isTimerun.integer) {
		return;
	}

	timerunNum = cg.currentTimerun;

	// Nico, check if timer needs reset
	// note: this should be move somewhere else
	if (!cg.timerunActive) {
		// Nico, timer will be reseted next time timerun becomes active
		needsReset = 1;
	}

	if (needsReset && cg.timerunActive) {
		// Nico, reset timer
		cg.timerunFinishedTime = 0;
		needsReset = 0;
	}
	//

	// Nico, if cg_drawTimer is 0
	if (!cg_drawTimer.integer) {
		return;
	}

	// Nico, set timer position
	x = cg_timerX.integer;
	y = cg_timerY.integer;

	// Nico, get fixed timerun start time
	startTime = cg.timerunStartTime - 500;

	if (cg.timerunActive) {
		// Nico, timerun active, run not finished yet
		milli = currentTimerunTime = cg.time - startTime;
	} else if (cg.timerunFinishedTime) {
		// Nico, timerun inactive, run finished
		milli = currentTimerunTime = cg.timerunFinishedTime;
	} else {
		// Nico, timerun not active, run not finished
		milli = currentTimerunTime;
	}

	// Nico, extract min:sec.milli
	min = milli / 60000;
	milli -= min * 60000;
	sec = milli / 1000;
	milli -= sec * 1000;

	// Nico, set timer default color
	Vector4Set(color, colorWhite[0], colorWhite[1], colorWhite[2], colorWhite[3]);

	if (cg.timerunFinishedTime) {
		// Nico, timerun finished
		// Compare with client rec
		if (cg.timerunBestTime[timerunNum] > 0 && cg.timerunLastTime[timerunNum] != cg.timerunBestTime[timerunNum]) {
			// Nico, did a different time, compute the delta
			dmilli = abs(cg.timerunLastTime[timerunNum] - cg.timerunBestTime[timerunNum]);
			dmin = dmilli / 60000;
			dmilli -= dmin * 60000;
			dsec = dmilli / 1000;
			dmilli -= dsec * 1000;

			if (cg.timerunLastTime[timerunNum] < cg.timerunBestTime[timerunNum]) {
				// Nico, did a better time
				Vector4Set(color, colorGreen[0], colorGreen[1], colorGreen[2], colorGreen[3]);
				Com_sprintf(status, sizeof(status), "%s", va("%02d:%02d.%03d (-%02d:%02d.%03d)", min, sec, milli, dmin, dsec, dmilli));
			} else {
				// Nico, did a slower time
				Vector4Set(color, colorRed[0], colorRed[1], colorRed[2], colorRed[3]);
				Com_sprintf(status, sizeof(status), "%s", va("%02d:%02d.%03d (+%02d:%02d.%03d)", min, sec, milli, dmin, dsec, dmilli));
			}
		} else if (cg.timerunBestTime[timerunNum] > 0) {
			// Nico, did the same time
			Com_sprintf(status, sizeof(status), "%s", va("%02d:%02d.%03d (+00:00.000)", min, sec, milli));		
		} else {
			// Nico, first time
			Com_sprintf(status, sizeof(status), "%s", va("%02d:%02d.%03d", min, sec, milli));	
		}
	} else {
		// Nico, timerun not finished yet

		// Nico, you won't beat the rec this time, turn timer to red color
		if (cg.timerunBestTime[timerunNum] > 0 && currentTimerunTime > cg.timerunBestTime[timerunNum]) {
			Vector4Set(color, colorRed[0], colorRed[1], colorRed[2], colorRed[3]);
		}

		Com_sprintf(status, sizeof(status), "%s", va("%02d:%02d.%03d", min, sec, milli));
	}

	// Nico, print the timer
	w = CG_Text_Width_Ext( status, sizex, sizey, &cgs.media.limboFont1 ) / 2;
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
	len = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
	len = sqrt(len);
	stepx = (x2 - x1) / len;
	stepy = (y2 - y1) / len;
	for (i = 0; i < len; i++) {
		PutPixel(x1, y1);
		x1 += stepx;
		y1 += stepy;
	}
	trap_R_SetColor(NULL);
}

extern float	pm_stopspeed;
extern float	pm_accelerate;
extern float	pm_airaccelerate;
extern float	pm_friction;

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
extern pml_t	pml;

/* Draw CGaz from TJMod
 *
 * @author Nico
 */
void CG_DrawCGaz(void) {
	float vel_angle; // absolute velocity angle
	float vel_relang; // relative velocity angle to viewangles[1]
	float per_angle;
	float a;
	float accel, scale;

	int forward = 0;
	int right = 0;

	vec_t vel_size;
	vec3_t vel;

	playerState_t *ps;

	// Nico, if cg_drawCGaz is 0
	if (!cg_drawCGaz.integer) {
		return;
	}

	ps = &cg.predictedPlayerState;

	if (ps->persistant[PERS_TEAM] == TEAM_SPECTATOR) {
		return;
	}

	a = 0.15;
	a = (a > 1.0f) ? 1.0f : (a < 0.0f) ? 0.0f : a;

	VectorCopy(ps->velocity, vel);

	// for a simplicity water, ladder etc. calculations are omitted
	// only air, ground and ice movement is important
	if (pml.walking && !(pml.groundTrace.surfaceFlags & SURF_SLICK)) {
		// apply friction
		float	speed, newspeed, control;
		float	drop;

		speed = VectorLength(vel);
		if (speed > 0) {
			drop = 0;

			// if getting knocked back, no friction
			if (!(ps->pm_flags & PMF_TIME_KNOCKBACK)) {
				control = speed < pm_stopspeed ? pm_stopspeed : speed;
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
		accel = pm_accelerate;
	}
	else {
		// in air or on ice, no friction
		accel = pm_airaccelerate;
	}

	vel_size = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);
	accel = accel * ps->speed * pmove_msec.integer / 1000;

	// based on PM_CmdScale from bg_pmove.c
	scale = ps->stats[STAT_USERCMD_BUTTONS] & (BUTTON_SPRINT << 8) ? ps->sprintSpeedScale : ps->runSpeedScale;
	per_angle = (ps->speed - accel) / vel_size * scale;
	if (per_angle < 1) {
		per_angle = RAD2DEG(acos(per_angle));
	} else {
		per_angle = ps->viewangles[YAW];
	}

	vel_angle = AngleNormalize180(RAD2DEG(atan2(vel[1], vel[0])));
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

	// CGaz 2
	vel_relang = DEG2RAD(vel_relang);
	per_angle = DEG2RAD(per_angle);

	DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
		SCREEN_CENTER_X + right, SCREEN_CENTER_Y - forward, colorCyan);

	vel_size /= 5;
	DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
		SCREEN_CENTER_X + vel_size * sin(vel_relang),
		SCREEN_CENTER_Y - vel_size * cos(vel_relang), colorRed);
	if (vel_size > SCREEN_HEIGHT / 2)
		vel_size = SCREEN_HEIGHT / 2;
	vel_size /= 2;
	DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
		SCREEN_CENTER_X + vel_size * sin(vel_relang + per_angle),
		SCREEN_CENTER_Y - vel_size * cos(vel_relang + per_angle), colorRed);
	DrawLine(SCREEN_CENTER_X, SCREEN_CENTER_Y,
		SCREEN_CENTER_X + vel_size * sin(vel_relang - per_angle),
		SCREEN_CENTER_Y - vel_size * cos(vel_relang - per_angle), colorRed);
	return;
}
