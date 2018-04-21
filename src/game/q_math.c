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

// q_math.c -- stateless support routines that are included in each code module
#include "q_shared.h"

vec3_t vec3_origin    = { 0, 0, 0 };
vec3_t axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

vec4_t colorBlack    = { 0, 0, 0, 1 };
vec4_t colorRed      = { 1, 0, 0, 1 };
vec4_t colorGreen    = { 0, 1, 0, 1 };
vec4_t colorBlue     = { 0, 0, 1, 1 };
vec4_t colorYellow   = { 1, 1, 0, 1 };
vec4_t colorOrange   = { 1, 0.5, 0, 1 };
vec4_t colorMagenta  = { 1, 0, 1, 1 };
vec4_t colorCyan     = { 0, 1, 1, 1 };
vec4_t colorWhite    = { 1, 1, 1, 1 };
vec4_t colorLtGrey   = { 0.75, 0.75, 0.75, 1 };
vec4_t colorMdGrey   = { 0.5, 0.5, 0.5, 1 };
vec4_t colorDkGrey   = { 0.25, 0.25, 0.25, 1 };
vec4_t colorMdRed    = { 0.5, 0, 0, 1 };
vec4_t colorMdGreen  = { 0, 0.5, 0, 1 };
vec4_t colorDkGreen  = { 0, 0.20f, 0, 1 };
vec4_t colorMdCyan   = { 0, 0.5, 0.5, 1 };
vec4_t colorMdYellow = { 0.5, 0.5, 0, 1 };
vec4_t colorMdOrange = { 0.5, 0.25, 0, 1 };
vec4_t colorMdBlue   = { 0, 0, 0.5, 1 };

vec4_t clrBrown         = { 0.68f, 0.68f, 0.56f, 1.f };
vec4_t clrBrownDk       = { 0.58f * 0.75f, 0.58f * 0.75f, 0.46f * 0.75f, 1.f };
vec4_t clrBrownLine     = { 0.0525f, 0.05f, 0.025f, 0.2f };
vec4_t clrBrownLineFull = { 0.0525f, 0.05f, 0.025f, 1.f };

vec4_t clrBrownTextLt2 = { 108 * 1.8f / 255.f, 88 * 1.8f / 255.f, 62 * 1.8f / 255.f, 1.f };
vec4_t clrBrownTextLt  = { 108 * 1.3f / 255.f, 88 * 1.3f / 255.f, 62 * 1.3f / 255.f, 1.f };
vec4_t clrBrownText    = { 108 / 255.f, 88 / 255.f, 62 / 255.f, 1.f };
vec4_t clrBrownTextDk  = { 20 / 255.f, 2 / 255.f, 0 / 255.f, 1.f };
vec4_t clrBrownTextDk2 = { 108 * 0.75f / 255.f, 88 * 0.75f / 255.f, 62 * 0.75f / 255.f, 1.f };

vec4_t g_color_table[32] =
{
	{ 0.0,  0.0,  0.0,  1.0 },          // 0 - black		0
	{ 1.0,  0.0,  0.0,  1.0 },          // 1 - red			1
	{ 0.0,  1.0,  0.0,  1.0 },          // 2 - green		2
	{ 1.0,  1.0,  0.0,  1.0 },          // 3 - yellow		3
	{ 0.0,  0.0,  1.0,  1.0 },          // 4 - blue			4
	{ 0.0,  1.0,  1.0,  1.0 },          // 5 - cyan			5
	{ 1.0,  0.0,  1.0,  1.0 },          // 6 - purple		6
	{ 1.0,  1.0,  1.0,  1.0 },          // 7 - white		7
	{ 1.0,  0.5,  0.0,  1.0 },          // 8 - orange		8
	{ 0.5,  0.5,  0.5,  1.0 },          // 9 - md.grey		9
	{ 0.75, 0.75, 0.75, 1.0 },          // : - lt.grey		10		// lt grey for names
	{ 0.75, 0.75, 0.75, 1.0 },          // ; - lt.grey		11
	{ 0.0,  0.5,  0.0,  1.0 },          // < - md.green		12
	{ 0.5,  0.5,  0.0,  1.0 },          // = - md.yellow	13
	{ 0.0,  0.0,  0.5,  1.0 },          // > - md.blue		14
	{ 0.5,  0.0,  0.0,  1.0 },          // ? - md.red		15
	{ 0.5,  0.25, 0.0,  1.0 },          // @ - md.orange	16
	{ 1.0,  0.6f, 0.1f, 1.0 },          // A - lt.orange	17
	{ 0.0,  0.5,  0.5,  1.0 },          // B - md.cyan		18
	{ 0.5,  0.0,  0.5,  1.0 },          // C - md.purple	19
	{ 0.0,  0.5,  1.0,  1.0 },          // D				20
	{ 0.5,  0.0,  1.0,  1.0 },          // E				21
	{ 0.2f, 0.6f, 0.8f, 1.0 },          // F				22
	{ 0.8f, 1.0,  0.8f, 1.0 },          // G				23
	{ 0.0,  0.4f, 0.2f, 1.0 },          // H				24
	{ 1.0,  0.0,  0.2f, 1.0 },          // I				25
	{ 0.7f, 0.1f, 0.1f, 1.0 },          // J				26
	{ 0.6f, 0.2f, 0.0,  1.0 },          // K				27
	{ 0.8f, 0.6f, 0.2f, 1.0 },          // L				28
	{ 0.6f, 0.6f, 0.2f, 1.0 },          // M				29
	{ 1.0,  1.0,  0.75, 1.0 },          // N				30
	{ 1.0,  1.0,  0.5,  1.0 },          // O				31
};

vec3_t bytedirs[NUMVERTEXNORMALS] =
{
	{ -0.525731f, 0.000000,   0.850651f  }, { -0.442863f, 0.238856f,  0.864188f  },
	{ -0.295242f, 0.000000,   0.955423f  }, { -0.309017f, 0.500000,   0.809017f  },
	{ -0.162460f, 0.262866f,  0.951056f  }, { 0.000000,   0.000000,   1.000000   },
	{ 0.000000,   0.850651f,  0.525731f  }, { -0.147621f, 0.716567f,  0.681718f  },
	{ 0.147621f,  0.716567f,  0.681718f  }, { 0.000000,   0.525731f,  0.850651f  },
	{ 0.309017f,  0.500000,   0.809017f  }, { 0.525731f,  0.000000,   0.850651f  },
	{ 0.295242f,  0.000000,   0.955423f  }, { 0.442863f,  0.238856f,  0.864188f  },
	{ 0.162460f,  0.262866f,  0.951056f  }, { -0.681718f, 0.147621f,  0.716567f  },
	{ -0.809017f, 0.309017f,  0.500000   }, { -0.587785f, 0.425325f,  0.688191f  },
	{ -0.850651f, 0.525731f,  0.000000   }, { -0.864188f, 0.442863f,  0.238856f  },
	{ -0.716567f, 0.681718f,  0.147621f  }, { -0.688191f, 0.587785f,  0.425325f  },
	{ -0.500000,  0.809017f,  0.309017f  }, { -0.238856f, 0.864188f,  0.442863f  },
	{ -0.425325f, 0.688191f,  0.587785f  }, { -0.716567f, 0.681718f,  -0.147621f },
	{ -0.500000,  0.809017f,  -0.309017f }, { -0.525731f, 0.850651f,  0.000000   },
	{ 0.000000,   0.850651f,  -0.525731f }, { -0.238856f, 0.864188f,  -0.442863f },
	{ 0.000000,   0.955423f,  -0.295242f }, { -0.262866f, 0.951056f,  -0.162460f },
	{ 0.000000,   1.000000,   0.000000   }, { 0.000000,   0.955423f,  0.295242f  },
	{ -0.262866f, 0.951056f,  0.162460f  }, { 0.238856f,  0.864188f,  0.442863f  },
	{ 0.262866f,  0.951056f,  0.162460f  }, { 0.500000,   0.809017f,  0.309017f  },
	{ 0.238856f,  0.864188f,  -0.442863f }, { 0.262866f,  0.951056f,  -0.162460f },
	{ 0.500000,   0.809017f,  -0.309017f }, { 0.850651f,  0.525731f,  0.000000   },
	{ 0.716567f,  0.681718f,  0.147621f  }, { 0.716567f,  0.681718f,  -0.147621f },
	{ 0.525731f,  0.850651f,  0.000000   }, { 0.425325f,  0.688191f,  0.587785f  },
	{ 0.864188f,  0.442863f,  0.238856f  }, { 0.688191f,  0.587785f,  0.425325f  },
	{ 0.809017f,  0.309017f,  0.500000   }, { 0.681718f,  0.147621f,  0.716567f  },
	{ 0.587785f,  0.425325f,  0.688191f  }, { 0.955423f,  0.295242f,  0.000000   },
	{ 1.000000,   0.000000,   0.000000   }, { 0.951056f,  0.162460f,  0.262866f  },
	{ 0.850651f,  -0.525731f, 0.000000   }, { 0.955423f,  -0.295242f, 0.000000   },
	{ 0.864188f,  -0.442863f, 0.238856f  }, { 0.951056f,  -0.162460f, 0.262866f  },
	{ 0.809017f,  -0.309017f, 0.500000   }, { 0.681718f,  -0.147621f, 0.716567f  },
	{ 0.850651f,  0.000000,   0.525731f  }, { 0.864188f,  0.442863f,  -0.238856f },
	{ 0.809017f,  0.309017f,  -0.500000  }, { 0.951056f,  0.162460f,  -0.262866f },
	{ 0.525731f,  0.000000,   -0.850651f }, { 0.681718f,  0.147621f,  -0.716567f },
	{ 0.681718f,  -0.147621f, -0.716567f }, { 0.850651f,  0.000000,   -0.525731f },
	{ 0.809017f,  -0.309017f, -0.500000  }, { 0.864188f,  -0.442863f, -0.238856f },
	{ 0.951056f,  -0.162460f, -0.262866f }, { 0.147621f,  0.716567f,  -0.681718f },
	{ 0.309017f,  0.500000,   -0.809017f }, { 0.425325f,  0.688191f,  -0.587785f },
	{ 0.442863f,  0.238856f,  -0.864188f }, { 0.587785f,  0.425325f,  -0.688191f },
	{ 0.688191f,  0.587785f,  -0.425325f }, { -0.147621f, 0.716567f,  -0.681718f },
	{ -0.309017f, 0.500000,   -0.809017f }, { 0.000000,   0.525731f,  -0.850651f },
	{ -0.525731f, 0.000000,   -0.850651f }, { -0.442863f, 0.238856f,  -0.864188f },
	{ -0.295242f, 0.000000,   -0.955423f }, { -0.162460f, 0.262866f,  -0.951056f },
	{ 0.000000,   0.000000,   -1.000000  }, { 0.295242f,  0.000000,   -0.955423f },
	{ 0.162460f,  0.262866f,  -0.951056f }, { -0.442863f, -0.238856f, -0.864188f },
	{ -0.309017f, -0.500000,  -0.809017f }, { -0.162460f, -0.262866f, -0.951056f },
	{ 0.000000,   -0.850651f, -0.525731f }, { -0.147621f, -0.716567f, -0.681718f },
	{ 0.147621f,  -0.716567f, -0.681718f }, { 0.000000,   -0.525731f, -0.850651f },
	{ 0.309017f,  -0.500000,  -0.809017f }, { 0.442863f,  -0.238856f, -0.864188f },
	{ 0.162460f,  -0.262866f, -0.951056f }, { 0.238856f,  -0.864188f, -0.442863f },
	{ 0.500000,   -0.809017f, -0.309017f }, { 0.425325f,  -0.688191f, -0.587785f },
	{ 0.716567f,  -0.681718f, -0.147621f }, { 0.688191f,  -0.587785f, -0.425325f },
	{ 0.587785f,  -0.425325f, -0.688191f }, { 0.000000,   -0.955423f, -0.295242f },
	{ 0.000000,   -1.000000,  0.000000   }, { 0.262866f,  -0.951056f, -0.162460f },
	{ 0.000000,   -0.850651f, 0.525731f  }, { 0.000000,   -0.955423f, 0.295242f  },
	{ 0.238856f,  -0.864188f, 0.442863f  }, { 0.262866f,  -0.951056f, 0.162460f  },
	{ 0.500000,   -0.809017f, 0.309017f  }, { 0.716567f,  -0.681718f, 0.147621f  },
	{ 0.525731f,  -0.850651f, 0.000000   }, { -0.238856f, -0.864188f, -0.442863f },
	{ -0.500000,  -0.809017f, -0.309017f }, { -0.262866f, -0.951056f, -0.162460f },
	{ -0.850651f, -0.525731f, 0.000000   }, { -0.716567f, -0.681718f, -0.147621f },
	{ -0.716567f, -0.681718f, 0.147621f  }, { -0.525731f, -0.850651f, 0.000000   },
	{ -0.500000,  -0.809017f, 0.309017f  }, { -0.238856f, -0.864188f, 0.442863f  },
	{ -0.262866f, -0.951056f, 0.162460f  }, { -0.864188f, -0.442863f, 0.238856f  },
	{ -0.809017f, -0.309017f, 0.500000   }, { -0.688191f, -0.587785f, 0.425325f  },
	{ -0.681718f, -0.147621f, 0.716567f  }, { -0.442863f, -0.238856f, 0.864188f  },
	{ -0.587785f, -0.425325f, 0.688191f  }, { -0.309017f, -0.500000,  0.809017f  },
	{ -0.147621f, -0.716567f, 0.681718f  }, { -0.425325f, -0.688191f, 0.587785f  },
	{ -0.162460f, -0.262866f, 0.951056f  }, { 0.442863f,  -0.238856f, 0.864188f  },
	{ 0.162460f,  -0.262866f, 0.951056f  }, { 0.309017f,  -0.500000f, 0.809017f  },
	{ 0.147621f,  -0.716567f, 0.681718f  }, { 0.000000,   -0.525731f, 0.850651f  },
	{ 0.425325f,  -0.688191f, 0.587785f  }, { 0.587785f,  -0.425325f, 0.688191f  },
	{ 0.688191f,  -0.587785f, 0.425325f  }, { -0.955423f, 0.295242f,  0.000000   },
	{ -0.951056f, 0.162460f,  0.262866f  }, { -1.000000,  0.000000,   0.000000   },
	{ -0.850651f, 0.000000,   0.525731f  }, { -0.955423f, -0.295242f, 0.000000   },
	{ -0.951056f, -0.162460f, 0.262866f  }, { -0.864188f, 0.442863f,  -0.238856f },
	{ -0.951056f, 0.162460f,  -0.262866f }, { -0.809017f, 0.309017f,  -0.500000  },
	{ -0.864188f, -0.442863f, -0.238856f }, { -0.951056f, -0.162460f, -0.262866f },
	{ -0.809017f, -0.309017f, -0.500000  }, { -0.681718f, 0.147621f,  -0.716567f },
	{ -0.681718f, -0.147621f, -0.716567f }, { -0.850651f, 0.000000,   -0.525731f },
	{ -0.688191f, 0.587785f,  -0.425325f }, { -0.587785f, 0.425325f,  -0.688191f },
	{ -0.425325f, 0.688191f,  -0.587785f }, { -0.425325f, -0.688191f, -0.587785f },
	{ -0.587785f, -0.425325f, -0.688191f }, { -0.688191f, -0.587785f, -0.425325f }
};

//==============================================================

int     Q_rand(int *seed) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

float   Q_random(int *seed) {
	return (Q_rand(seed) & 0xffff) / (float)0x10000;
}

float   Q_crandom(int *seed) {
	return 2.0 * (Q_random(seed) - 0.5);
}

//=======================================================

// this isn't a real cheap function to call!
int DirToByte(vec3_t dir) {
	int   i, best;
	float bestd;

	if (!dir) {
		return 0;
	}

	bestd = 0;
	best  = 0;
	for (i = 0 ; i < NUMVERTEXNORMALS ; ++i) {
		float d;

		d = DotProduct(dir, bytedirs[i]);
		if (d > bestd) {
			bestd = d;
			best  = i;
		}
	}

	return best;
}

void ByteToDir(int b, vec3_t dir) {
	if (b < 0 || b >= NUMVERTEXNORMALS) {
		VectorCopy(vec3_origin, dir);
		return;
	}
	VectorCopy(bytedirs[b], dir);
}

/*
===============
RotatePointAroundVector

This is not implemented very well...
===============
*/
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point,
                             float degrees) {
	float  m[3][3];
	float  im[3][3];
	float  zrot[3][3];
	float  tmpmat[3][3];
	float  rot[3][3];
	int    i;
	vec3_t vr, vup, vf;
	float  rad;

	vf[0] = dir[0];
	vf[1] = dir[1];
	vf[2] = dir[2];

	PerpendicularVector(vr, dir);
	CrossProduct(vr, vf, vup);

	m[0][0] = vr[0];
	m[1][0] = vr[1];
	m[2][0] = vr[2];

	m[0][1] = vup[0];
	m[1][1] = vup[1];
	m[2][1] = vup[2];

	m[0][2] = vf[0];
	m[1][2] = vf[1];
	m[2][2] = vf[2];

	memcpy(im, m, sizeof (im));

	im[0][1] = m[1][0];
	im[0][2] = m[2][0];
	im[1][0] = m[0][1];
	im[1][2] = m[2][1];
	im[2][0] = m[0][2];
	im[2][1] = m[1][2];

	memset(zrot, 0, sizeof (zrot));
	zrot[1][1] = zrot[2][2] = 1.0F;

	rad        = DEG2RAD(degrees);
	zrot[0][0] = cos(rad);
	zrot[0][1] = sin(rad);
	zrot[1][0] = -sin(rad);
	zrot[1][1] = cos(rad);

	MatrixMultiply(m, zrot, tmpmat);
	MatrixMultiply(tmpmat, im, rot);

	for (i = 0; i < 3; ++i) {
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];
	}
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection(vec3_t axis[3], float yaw) {

	// create an arbitrary axis[1]
	PerpendicularVector(axis[1], axis[0]);

	// rotate it around axis[0] by yaw
	if (yaw) {
		vec3_t temp;

		VectorCopy(axis[1], temp);
		RotatePointAroundVector(axis[1], axis[0], temp, yaw);
	}

	// cross to get axis[2]
	CrossProduct(axis[0], axis[1], axis[2]);
}

void vectoangles(const vec3_t value1, vec3_t angles) {
	float yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0) {
		yaw = 0;
		if (value1[2] > 0) {
			pitch = 90;
		} else {
			pitch = 270;
		}
	} else {
		float forward;

		if (value1[0]) {
			yaw = (atan2(value1[1], value1[0]) * 180 / M_PI);
		} else if (value1[1] > 0) {
			yaw = 90;
		} else {
			yaw = 270;
		}
		if (yaw < 0) {
			yaw += 360;
		}

		forward = sqrt(value1[0] * value1[0] + value1[1] * value1[1]);
		pitch   = (atan2(value1[2], forward) * 180 / M_PI);
		if (pitch < 0) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW]   = yaw;
	angles[ROLL]  = 0;
}

/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis(const vec3_t angles, vec3_t axis[3]) {
	vec3_t right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors(angles, axis[0], right, axis[2]);
	VectorSubtract(vec3_origin, right, axis[1]);
}

void AxisClear(vec3_t axis[3]) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy(vec3_t in[3], vec3_t out[3]) {
	VectorCopy(in[0], out[0]);
	VectorCopy(in[1], out[1]);
	VectorCopy(in[2], out[2]);
}

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal) {
	float  d;
	vec3_t n;
	float  inv_denom;

	inv_denom = 1.0F / DotProduct(normal, normal);

	d = DotProduct(normal, p) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

//============================================================================

/*
** float q_rsqrt( float number )
* (strict-aliasing rule break fixed by Nico)
*
*/
float Q_rsqrt(float number) {
	long         i;
	float        x2, y;
	float_long_u number_u;
	float_long_u i_u;

	number_u.f = number;
	x2         = number_u.f * 0.5F;
	i          = *( long * ) &number_u.l; // evil floating point bit level hacking
	i_u.l      = 0x5f3759df - (i >> 1); // what the fuck?
	y          = *( float * ) &i_u.f;
	y          = y * (1.5F - (x2 * y * y)); // 1st iteration

	return y;
}

// Nico, strict-aliasing rule break fixed
float Q_fabs(float f) {
	float_int_u f_u;
	float_int_u tmp_u;

	f_u.f   = f;
	tmp_u.i = (*(int *)&f_u.i) & 0x7FFFFFFF;

	return *(float *)&tmp_u.f;
}

#if id386 && !((defined __linux__ || defined __FreeBSD__ || defined __GNUC__) && (defined __i386__))       // rb010123
long myftol(float f) {
	static int tmp;
	__asm fld f
	__asm fistp tmp
	__asm mov eax, tmp
}
#endif

//============================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle(float from, float to, float frac) {
	if (to - from > 180) {
		to -= 360;
	}
	if (to - from < -180) {
		to += 360;
	}

	return from + frac * (to - from);
}

/*
=================
LerpPosition

=================
*/

void LerpPosition(vec3_t start, vec3_t end, float frac, vec3_t out) {
	vec3_t dist;

	VectorSubtract(end, start, dist);
	VectorMA(start, frac, dist, out);
}

/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float AngleSubtract(float a1, float a2) {
	float a = a1 - a2;

	while (a > 180) {
		a -= 360;
	}
	while (a < -180) {
		a += 360;
	}
	return a;
}

void AnglesSubtract(vec3_t v1, vec3_t v2, vec3_t v3) {
	v3[0] = AngleSubtract(v1[0], v2[0]);
	v3[1] = AngleSubtract(v1[1], v2[1]);
	v3[2] = AngleSubtract(v1[2], v2[2]);
}

float AngleMod(float a) {
	return (360.0 / 65536) * ((int)(a * (65536 / 360.0)) & 65535);
}

/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360(float angle) {
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}

/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180(float angle) {
	angle = AngleNormalize360(angle);
	if (angle > 180.0) {
		angle -= 360.0;
	}
	return angle;
}

/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta(float angle1, float angle2) {
	return AngleNormalize180(angle1 - angle2);
}

//============================================================

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds(const vec3_t mins, const vec3_t maxs) {
	int    i;
	vec3_t corner;

	for (i = 0 ; i < 3 ; ++i) {
		float a, b;

		a         = Q_fabs(mins[i]);
		b         = Q_fabs(maxs[i]);
		corner[i] = a > b ? a : b;
	}

	return VectorLength(corner);
}

void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs) {
	if (v[0] < mins[0]) {
		mins[0] = v[0];
	}
	if (v[0] > maxs[0]) {
		maxs[0] = v[0];
	}

	if (v[1] < mins[1]) {
		mins[1] = v[1];
	}
	if (v[1] > maxs[1]) {
		maxs[1] = v[1];
	}

	if (v[2] < mins[2]) {
		mins[2] = v[2];
	}
	if (v[2] > maxs[2]) {
		maxs[2] = v[2];
	}
}

int VectorCompare(const vec3_t v1, const vec3_t v2) {
	if (v1[0] != v2[0] || v1[1] != v2[1] || v1[2] != v2[2]) {
		return 0;
	}

	return 1;
}

vec_t VectorNormalize(vec3_t v) {
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

	length = sqrt(length);

	if (length) {
		float ilength = 1 / length;

		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

//
// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length
//
void VectorNormalizeFast(vec3_t v) {
	float ilength;

	ilength = Q_rsqrt(DotProduct(v, v));

	v[0] *= ilength;
	v[1] *= ilength;
	v[2] *= ilength;
}

vec_t VectorNormalize2(const vec3_t v, vec3_t out) {
	float length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];

	length = sqrt(length);

	if (length) {
		float ilength = 1 / length;

		out[0] = v[0] * ilength;
		out[1] = v[1] * ilength;
		out[2] = v[2] * ilength;
	} else {
		VectorClear(out);
	}

	return length;
}

void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross) {
	cross[0] = v1[1] * v2[2] - v1[2] * v2[1];
	cross[1] = v1[2] * v2[0] - v1[0] * v2[2];
	cross[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

vec_t VectorLength(const vec3_t v) {
	return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

vec_t VectorLengthSquared(const vec3_t v) {
	return (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

vec_t Distance(const vec3_t p1, const vec3_t p2) {
	vec3_t v;

	VectorSubtract(p2, p1, v);
	return VectorLength(v);
}

vec_t DistanceSquared(const vec3_t p1, const vec3_t p2) {
	vec3_t v;

	VectorSubtract(p2, p1, v);
	return v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
}

void VectorInverse(vec3_t v) {
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

/*
================
MatrixMultiply
================
*/
void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] + in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] + in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] + in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] + in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] + in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] + in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] + in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] + in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] + in1[2][2] * in2[2][2];
}

void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	float        angle;
	static float sr, sp, sy, cr, cp, cy;

	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI * 2 / 360);
	sy    = sin(angle);
	cy    = cos(angle);

	angle = angles[PITCH] * (M_PI * 2 / 360);
	sp    = sin(angle);
	cp    = cos(angle);

	angle = angles[ROLL] * (M_PI * 2 / 360);
	sr    = sin(angle);
	cr    = cos(angle);

	if (forward) {
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if (right) {
		right[0] = (-1 * sr * sp * cy + -1 * cr * -sy);
		right[1] = (-1 * sr * sp * sy + -1 * cr * cy);
		right[2] = -1 * sr * cp;
	}
	if (up) {
		up[0] = (cr * sp * cy + -sr * -sy);
		up[1] = (cr * sp * sy + -sr * cy);
		up[2] = cr * cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector(vec3_t dst, const vec3_t src) {
	int    pos;
	int    i;
	float  minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for (pos = 0, i = 0; i < 3; ++i) {
		if (Q_fabs(src[i]) < minelem) {
			pos     = i;
			minelem = Q_fabs(src[i]);
		}
	}
	tempvec[0]   = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane(dst, tempvec, src);

	/*
	** normalize the result
	*/
	VectorNormalize(dst);
}

// Ridah
/*
=================
GetPerpendicularViewVector

  Used to find an "up" vector for drawing a sprite so that it always faces the view as best as possible
=================
*/
void GetPerpendicularViewVector(const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up) {
	vec3_t v1, v2;

	VectorSubtract(point, p1, v1);
	VectorNormalize(v1);

	VectorSubtract(point, p2, v2);
	VectorNormalize(v2);

	CrossProduct(v1, v2, up);
	VectorNormalize(up);
}

/*
================
ProjectPointOntoVector
================
*/
void ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj) {
	vec3_t pVec, vec;

	VectorSubtract(point, vStart, pVec);
	VectorSubtract(vEnd, vStart, vec);
	VectorNormalize(vec);
	// project onto the directional vector for this segment
	VectorMA(vStart, DotProduct(pVec, vec), vec, vProj);
}

float vectoyaw(const vec3_t vec) {
	float yaw;

	if (vec[YAW] == 0 && vec[PITCH] == 0) {
		yaw = 0;
	} else {
		if (vec[PITCH]) {
			yaw = (atan2(vec[YAW], vec[PITCH]) * 180 / M_PI);
		} else if (vec[YAW] > 0) {
			yaw = 90;
		} else {
			yaw = 270;
		}
		if (yaw < 0) {
			yaw += 360;
		}
	}

	return yaw;
}

/*
=================
AxisToAngles

  Used to convert the MD3 tag axis to MDC tag angles, which are much smaller

  This doesn't have to be fast, since it's only used for conversion in utils, try to avoid
  using this during gameplay
=================
*/
void AxisToAngles(vec3_t axis[3], vec3_t angles) {
	vec3_t right, roll_angles, tvec;

	// first get the pitch and yaw from the forward vector
	vectoangles(axis[0], angles);

	// now get the roll from the right vector
	VectorCopy(axis[1], right);
	// get the angle difference between the tmpAxis[2] and axis[2] after they have been reverse-rotated
	RotatePointAroundVector(tvec, axisDefault[2], right, -angles[YAW]);
	RotatePointAroundVector(right, axisDefault[1], tvec, -angles[PITCH]);
	// now find the angles, the PITCH is effectively our ROLL
	vectoangles(right, roll_angles);
	roll_angles[PITCH] = AngleNormalize180(roll_angles[PITCH]);
	// if the yaw is more than 90 degrees difference, we should adjust the pitch
	if (DotProduct(right, axisDefault[1]) < 0) {
		if (roll_angles[PITCH] < 0) {
			roll_angles[PITCH] = -90 + (-90 - roll_angles[PITCH]);
		} else {
			roll_angles[PITCH] = 90 + (90 - roll_angles[PITCH]);
		}
	}

	angles[ROLL] = -roll_angles[PITCH];
}

float VectorDistance(vec3_t v1, vec3_t v2) {
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLength(dir);
}

float VectorDistanceSquared(vec3_t v1, vec3_t v2) {
	vec3_t dir;

	VectorSubtract(v2, v1, dir);
	return VectorLengthSquared(dir);
}

/*
=================
GetDigits

Returns the amount of digits a float has

@author suburb
=================
*/
int GetDigits(float number) {
	int count = 0;

	number = fabs(number);

	// more elegant but slower in performance
	/*if (number >= 1){
	    count = floor(log10(number)) + 1;
	}*/

	if (number >= 1000000000000000000) {
		count = 19;
	} else if (number >= 100000000000000000) {
		count = 18;
	} else if (number >= 10000000000000000) {
		count = 17;
	} else if (number >= 1000000000000000) {
		count = 16;
	} else if (number >= 100000000000000) {
		count = 15;
	} else if (number >= 10000000000000) {
		count = 14;
	} else if (number >= 1000000000000) {
		count = 13;
	} else if (number >= 100000000000) {
		count = 12;
	} else if (number >= 10000000000) {
		count = 11;
	} else if (number >= 1000000000) {
		count = 10;
	} else if (number >= 100000000) {
		count = 9;
	} else if (number >= 10000000) {
		count = 8;
	} else if (number >= 1000000) {
		count = 7;
	} else if (number >= 100000) {
		count = 6;
	} else if (number >= 10000) {
		count = 5;
	} else if (number >= 1000) {
		count = 4;
	} else if (number >= 100) {
		count = 3;
	} else if (number >= 10) {
		count = 2;
	} else if (number >= 1) {
		count = 1;
	}
	return count;
}