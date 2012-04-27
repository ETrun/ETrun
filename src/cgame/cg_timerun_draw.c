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
/*
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
	// float n1; @unused
	float n2;
	int n;

	if (cg_thirdPerson.integer)
		return;

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
	if (trace.fraction == 1.0)
		return;

	// not a floor
	if (trace.plane.type != 2)
		return;

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
	if (n && hn < t + 0.25 && hn > t)
		CG_DrawStringExt(320, 220, "F", colorWhite, qfalse, qtrue,
				TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);

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
		if (hn < t + 0.25 && hn > t)
			CG_DrawStringExt(330, 220, "J", colorWhite, qfalse, qtrue,
					TINYCHAR_WIDTH, TINYCHAR_HEIGHT, 0);
	}
}*/

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
