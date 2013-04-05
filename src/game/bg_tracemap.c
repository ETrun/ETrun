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

#ifdef CGAMEDLL
# include "../cgame/cg_local.h"
#else
# include "g_local.h"
#endif

/*
**  Map tracemap view generation
*/

#define MAX_WORLD_HEIGHT            MAX_MAP_SIZE    // maximum world height
#define MIN_WORLD_HEIGHT            -MAX_MAP_SIZE   // minimum world height

#define TRACEMAP_SIZE               256

typedef struct tracemap_s {
	qboolean loaded;
	float sky[TRACEMAP_SIZE][TRACEMAP_SIZE];
	float skyground[TRACEMAP_SIZE][TRACEMAP_SIZE];
	float ground[TRACEMAP_SIZE][TRACEMAP_SIZE];
	vec2_t world_mins, world_maxs;
	int groundfloor;
} tracemap_t;

static tracemap_t tracemap;

static vec2_t one_over_mapgrid_factor;

void etpro_FinalizeTracemapClamp(int *x, int *y);

static void BG_ClampPointToTracemapExtends(vec3_t point, vec2_t out) {

	if (point[0] < tracemap.world_mins[0]) {
		out[0] = tracemap.world_mins[0];
	} else if (point[0] > tracemap.world_maxs[0]) {
		out[0] = tracemap.world_maxs[0];
	} else {
		out[0] = point[0];
	}

	if (point[1] < tracemap.world_maxs[1]) {
		out[1] = tracemap.world_maxs[1];
	} else if (point[1] > tracemap.world_mins[1]) {
		out[1] = tracemap.world_mins[1];
	} else {
		out[1] = point[1];
	}
}

float BG_GetSkyHeightAtPoint(vec3_t pos) {
	int    i, j;
	vec2_t point;

	if (!tracemap.loaded) {
		return MAX_WORLD_HEIGHT;
	}

	BG_ClampPointToTracemapExtends(pos, point);

	i = myftol((point[0] - tracemap.world_mins[0]) * one_over_mapgrid_factor[0]);
	j = myftol((point[1] - tracemap.world_mins[1]) * one_over_mapgrid_factor[1]);

	// rain - re-clamp the points, because a rounding error can cause
	// them to go outside the array
	etpro_FinalizeTracemapClamp(&i, &j);

	return tracemap.sky[j][i];
}

float BG_GetSkyGroundHeightAtPoint(vec3_t pos) {
	int    i, j;
	vec2_t point;

	if (!tracemap.loaded) {
		return MAX_WORLD_HEIGHT;
	}

	BG_ClampPointToTracemapExtends(pos, point);

	i = myftol((point[0] - tracemap.world_mins[0]) * one_over_mapgrid_factor[0]);
	j = myftol((point[1] - tracemap.world_mins[1]) * one_over_mapgrid_factor[1]);

	// rain - re-clamp the points, because a rounding error can cause
	// them to go outside the array
	etpro_FinalizeTracemapClamp(&i, &j);

	return tracemap.skyground[j][i];
}

float BG_GetGroundHeightAtPoint(vec3_t pos) {
	int    i, j;
	vec2_t point;

	if (!tracemap.loaded) {
		return MIN_WORLD_HEIGHT;
	}

	BG_ClampPointToTracemapExtends(pos, point);

	i = myftol((point[0] - tracemap.world_mins[0]) * one_over_mapgrid_factor[0]);
	j = myftol((point[1] - tracemap.world_mins[1]) * one_over_mapgrid_factor[1]);

	// rain - re-clamp the points, because a rounding error can cause
	// them to go outside the array
	etpro_FinalizeTracemapClamp(&i, &j);

	return tracemap.ground[j][i];
}

int BG_GetTracemapGroundFloor(void) {
	if (!tracemap.loaded) {
		return MIN_WORLD_HEIGHT;
	}
	return tracemap.groundfloor;
}

// rain - re-clamp the points, because a rounding error can cause
// them to go outside the array
void etpro_FinalizeTracemapClamp(int *x, int *y) {
	if (*x < 0) {
		*x = 0;
	} else if (*x > TRACEMAP_SIZE - 1) {
		*x = TRACEMAP_SIZE - 1;
	}

	if (*y < 0) {
		*y = 0;
	} else if (*y > TRACEMAP_SIZE - 1) {
		*y = TRACEMAP_SIZE - 1;
	}
}
