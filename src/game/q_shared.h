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

#ifndef __Q_SHARED_H
#define __Q_SHARED_H

// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#define CONFIG_NAME     "etconfig.cfg"

#if defined _WIN32 && !defined __GNUC__
# pragma warning(disable : 4244) // 'conversion' conversion from 'type1' to 'type2', possible loss of data
#endif

#if (defined _MSC_VER)
# define Q_EXPORT __declspec(dllexport)
#elif (defined __SUNPRO_C)
# define Q_EXPORT __global
#elif ((__GNUC__ >= 3) && (!__EMX__) && (!sun))
# define Q_EXPORT __attribute__((visibility("default")))
#else
# define Q_EXPORT
#endif

// suburb, widescreen support
#define RATIO43      (4.0f / 3.0f)
#define RPRATIO43    (1 / RATIO43)

/**********************************************************************
  VM Considerations

  The VM can not use the standard system headers because we aren't really
  using the compiler they were meant for.  We use bg_lib.h which contains
  prototypes for the functions we define for our own use in bg_lib.c.

  When writing mods, please add needed headers HERE, do not start including
  stuff like <stdio.h> in the various .c files that make up each of the VMs
  since you will be including system headers files can will have issues.

  Remember, if you use a C library function that is not defined in bg_lib.c,
  you will have to add your own version for support in the VM.

 **********************************************************************/

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <sys/stat.h> // rain
#include <float.h>

// Nico, use stdint for portability
#include <stdint.h>

// use MSVC inline asm version of C functions
#if defined _M_IX86
# define id386   1
#else
# define id386   0
#endif

// for windows fastcall option

#define QDECL

//bani
//======================= GNUC DEFINES ==================================
#ifdef __GNUC__
# define _attribute(x) __attribute__(x)
#else
# define _attribute(x)
#endif

//======================= WIN32 DEFINES =================================

#ifdef WIN32

# undef QDECL
# define QDECL   __cdecl

#endif

typedef unsigned char byte;

typedef enum { qfalse, qtrue }    qboolean;

typedef int qhandle_t;
typedef int sfxHandle_t;
typedef int fileHandle_t;
typedef int clipHandle_t;

#define     SND_CUTOFF_ALL      0x008   // Cut off all sounds on this channel
#define     SND_NOCUT           0x010   // Don't cut off.  Always let finish (overridden by SND_CUTOFF_ALL)
#define     SND_NO_ATTENUATION  0x020   // don't attenuate (even though the sound is in voice channel, for example)

#ifndef NULL
# define NULL ((void *)0)
#endif

#define MAX_QINT            0x7fffffff
#define MIN_QINT            (-MAX_QINT - 1)

// TTimo gcc: was missing, added from Q3 source
#ifndef max
# define max(x, y) (((x) > (y)) ? (x) : (y))
# define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

// angle indexes
#define PITCH               0       // up / down
#define YAW                 1       // left / right
#define ROLL                2       // fall over

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define MAX_STRING_CHARS    1024    // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS   256     // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS     1024    // max length of an individual token

#define MAX_INFO_STRING     1024
#define MAX_INFO_KEY        1024
#define MAX_INFO_VALUE      1024

#define BIG_INFO_STRING     8192    // used for system info key only
#define BIG_INFO_KEY        8192
#define BIG_INFO_VALUE      8192

#define MAX_QPATH           64      // max length of a quake game pathname
#define MAX_OSPATH          256     // max length of a filesystem pathname

// rain - increased to 36 to match MAX_NETNAME, fixes #13 - UI stuff breaks
// with very long names
#define MAX_NAME_LENGTH     36      // max length of a client name

#define MAX_SAY_TEXT        150

#define MAX_VA_STRING       32000
typedef enum {
	MESSAGE_EMPTY = 0,
	MESSAGE_WAITING,        // rate/packet limited
	MESSAGE_WAITING_OVERFLOW,   // packet too large with message
} messageStatus_t;

// paramters for command buffer stuffing
typedef enum {
	EXEC_NOW,           // don't return until completed, a VM should NEVER use this,
	                    // because some commands might cause the VM to be unloaded...
	EXEC_INSERT,        // insert at current position, but don't run yet
	EXEC_APPEND         // add to end of the command buffer (normal case)
} cbufExec_t;

//
// these aren't needed by any of the VMs.  put in another header?
//
#define MAX_MAP_AREA_BYTES      32      // bit vector of area visibility

// parameters to the main Error routine
typedef enum {
	ERR_FATAL,                  // exit the entire game with a popup window
	ERR_VID_FATAL,              // exit the entire game with a popup window and doesn't delete profile.pid
	ERR_DROP,                   // print to console and disconnect from game
	ERR_SERVERDISCONNECT,       // don't kill server
	ERR_DISCONNECT,             // client disconnected from the server
	ERR_NEED_CD,                // pop up the need-cd dialog
	ERR_AUTOUPDATE
} errorParm_t;

// font rendering values used by ui and cgame

#define PROP_GAP_WIDTH          3
#define PROP_SPACE_WIDTH        8
#define PROP_HEIGHT             27
#define PROP_SMALL_SIZE_SCALE   0.75

#define BLINK_DIVISOR           200
#define PULSE_DIVISOR           75

#define UI_LEFT         0x00000000  // default
#define UI_CENTER       0x00000001
#define UI_RIGHT        0x00000002
#define UI_FORMATMASK   0x00000007
#define UI_SMALLFONT    0x00000010
#define UI_BIGFONT      0x00000020  // default
#define UI_GIANTFONT    0x00000040
#define UI_DROPSHADOW   0x00000800
#define UI_BLINK        0x00001000
#define UI_INVERSE      0x00002000
#define UI_PULSE        0x00004000
// JOSEPH 10-24-99
#define UI_MENULEFT     0x00008000
#define UI_MENURIGHT    0x00010000
#define UI_EXSMALLFONT  0x00020000
#define UI_MENUFULL     0x00080000
// END JOSEPH

typedef enum {
	h_high,
	h_low,
	h_dontcare
} ha_pref;

/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
# define M_PI        3.14159265358979323846f // matches value in gcc v2 math.h
#endif

#define NUMVERTEXNORMALS    162
extern vec3_t bytedirs[NUMVERTEXNORMALS];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH        640
#define SCREEN_HEIGHT       480

#define TINYCHAR_WIDTH      6
#define TINYCHAR_HEIGHT     12

#define MINICHAR_WIDTH      7
#define MINICHAR_HEIGHT     14

#define SMALLCHAR_WIDTH     8
#define SMALLCHAR_HEIGHT    16

#define BIGCHAR_WIDTH       16
#define BIGCHAR_HEIGHT      16

#define GIANTCHAR_WIDTH     32
#define GIANTCHAR_HEIGHT    48

extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorOrange;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;
extern vec4_t colorMdRed;
extern vec4_t colorMdGreen;
extern vec4_t colorDkGreen;
extern vec4_t colorMdCyan;
extern vec4_t colorMdYellow;
extern vec4_t colorMdOrange;
extern vec4_t colorMdBlue;

extern vec4_t clrBrown;
extern vec4_t clrBrownDk;
extern vec4_t clrBrownLine;
extern vec4_t clrBrownText;
extern vec4_t clrBrownTextDk;
extern vec4_t clrBrownTextDk2;
extern vec4_t clrBrownTextLt;
extern vec4_t clrBrownTextLt2;
extern vec4_t clrBrownLineFull;

#define GAME_INIT_FRAMES    6
#define FRAMETIME           100                 // msec

#define Q_COLOR_ESCAPE  '^'
#define Q_IsColorString(p)  (p && *(p) == Q_COLOR_ESCAPE && *((p) + 1) && *((p) + 1) != Q_COLOR_ESCAPE)

#define COLOR_BLACK     '0'
#define COLOR_RED       '1'
#define COLOR_GREEN     '2'
#define COLOR_YELLOW    '3'
#define COLOR_BLUE      '4'
#define COLOR_CYAN      '5'
#define COLOR_MAGENTA   '6'
#define COLOR_WHITE     '7'
#define COLOR_ORANGE    '8'
#define COLOR_MDGREY    '9'
#define COLOR_LTGREY    ':'
#define COLOR_MDGREEN   '<'
#define COLOR_MDYELLOW  '='
#define COLOR_MDBLUE    '>'
#define COLOR_MDRED     '?'
#define COLOR_LTORANGE  'A'
#define COLOR_MDCYAN    'B'
#define COLOR_MDPURPLE  'C'
#define COLOR_NULL      '*'

#define COLOR_BITS  31
#define ColorIndex(c)   (((c) - '0') & COLOR_BITS)

#define S_COLOR_BLACK       "^0"
#define S_COLOR_RED         "^1"
#define S_COLOR_GREEN       "^2"
#define S_COLOR_YELLOW      "^3"
#define S_COLOR_BLUE        "^4"
#define S_COLOR_CYAN        "^5"
#define S_COLOR_MAGENTA     "^6"
#define S_COLOR_WHITE       "^7"
#define S_COLOR_ORANGE      "^8"
#define S_COLOR_MDGREY      "^9"
#define S_COLOR_LTGREY      "^:"
#define S_COLOR_MDGREEN     "^<"
#define S_COLOR_MDYELLOW    "^="
#define S_COLOR_MDBLUE      "^>"
#define S_COLOR_MDRED       "^?"
#define S_COLOR_LTORANGE    "^A"
#define S_COLOR_MDCYAN      "^B"
#define S_COLOR_MDPURPLE    "^C"
#define S_COLOR_NULL        "^*"

extern vec4_t g_color_table[32];

#define MAKERGB(v, r, g, b) v[0]     = r; v[1] = g; v[2] = b
#define MAKERGBA(v, r, g, b, a) v[0] = r; v[1] = g; v[2] = b; v[3] = a

// Hex Color string support
#define gethex(ch) ((ch) > '9' ? ((ch) >= 'a' ? ((ch) - 'a' + 10) : ((ch) - '7')) : ((ch) - '0'))
#define ishex(ch)  ((ch) && (((ch) >= '0' && (ch) <= '9') || ((ch) >= 'A' && (ch) <= 'F') || ((ch) >= 'a' && (ch) <= 'f')))
// check if it's format rrggbb r,g,b e {0..9} U {A...F}
#define Q_IsHexColorString(p) (ishex(*(p)) && ishex(*((p) + 1)) && ishex(*((p) + 2)) && ishex(*((p) + 3)) && ishex(*((p) + 4)) && ishex(*((p) + 5)))
#define Q_HexColorStringHasAlpha(p) (ishex(*((p) + 6)) && ishex(*((p) + 7)))

#define DEG2RAD(a) (((a) * M_PI) / 180.0F)
#define RAD2DEG(a) (((a) * 180.0f) / M_PI)

struct cplane_s;

extern vec3_t vec3_origin;
extern vec3_t axisDefault[3];

float Q_fabs(float f);
float Q_rsqrt(float f);         // reciprocal square root

// fast float to int conversion
#if id386 && !((defined __linux__ || defined __FreeBSD__ || defined __GNUC__) && (defined __i386__))       // rb010123
long myftol(float f);
#elif defined(__APPLE__)
# define myftol(x) (long)(x)
#else
extern long int lrintf(float x);
# define myftol(x) lrintf(x)
#endif

// this isn't a real cheap function to call!
int DirToByte(vec3_t dir);
void ByteToDir(int b, vec3_t dir);

#define DotProduct(x, y)         ((x)[0] * (y)[0] + (x)[1] * (y)[1] + (x)[2] * (y)[2])
#define VectorSubtract(a, b, c)   ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1], (c)[2] = (a)[2] - (b)[2])
#define VectorAdd(a, b, c)        ((c)[0] = (a)[0] + (b)[0], (c)[1] = (a)[1] + (b)[1], (c)[2] = (a)[2] + (b)[2])
#define VectorCopy(a, b)         ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2])
#define VectorScale(v, s, o)    ((o)[0] = (v)[0] * (s), (o)[1] = (v)[1] * (s), (o)[2] = (v)[2] * (s))
#define VectorMA(v, s, b, o)    ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), (o)[2] = (v)[2] + (b)[2] * (s))

#define VectorClear(a)              ((a)[0] = (a)[1] = (a)[2] = 0)
#define VectorNegate(a, b)           ((b)[0] = -(a)[0], (b)[1] = -(a)[1], (b)[2] = -(a)[2])
#define VectorSet(v, x, y, z)       ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z))

#define Vector2Set(v, x, y)         ((v)[0] = (x), (v)[1] = (y))
#define Vector2Copy(a, b)            ((b)[0] = (a)[0], (b)[1] = (a)[1])
#define Vector2Subtract(a, b, c)      ((c)[0] = (a)[0] - (b)[0], (c)[1] = (a)[1] - (b)[1])

#define Vector4Set(v, x, y, z, n)   ((v)[0] = (x), (v)[1] = (y), (v)[2] = (z), (v)[3] = (n))
#define Vector4Copy(a, b)            ((b)[0] = (a)[0], (b)[1] = (a)[1], (b)[2] = (a)[2], (b)[3] = (a)[3])
#define Vector4MA(v, s, b, o)       ((o)[0] = (v)[0] + (b)[0] * (s), (o)[1] = (v)[1] + (b)[1] * (s), (o)[2] = (v)[2] + (b)[2] * (s), (o)[3] = (v)[3] + (b)[3] * (s))
#define Vector4Average(v, b, s, o)  ((o)[0] = ((v)[0] * (1 - (s))) + ((b)[0] * (s)), (o)[1] = ((v)[1] * (1 - (s))) + ((b)[1] * (s)), (o)[2] = ((v)[2] * (1 - (s))) + ((b)[2] * (s)), (o)[3] = ((v)[3] * (1 - (s))) + ((b)[3] * (s)))

#define SnapVector(v) { v[0] = ((int)(v[0])); v[1] = ((int)(v[1])); v[2] = ((int)(v[2])); }

float RadiusFromBounds(const vec3_t mins, const vec3_t maxs);
void AddPointToBounds(const vec3_t v, vec3_t mins, vec3_t maxs);
int VectorCompare(const vec3_t v1, const vec3_t v2);
vec_t VectorLength(const vec3_t v);
vec_t VectorLengthSquared(const vec3_t v);
vec_t Distance(const vec3_t p1, const vec3_t p2);
vec_t DistanceSquared(const vec3_t p1, const vec3_t p2);
void CrossProduct(const vec3_t v1, const vec3_t v2, vec3_t cross);
vec_t VectorNormalize(vec3_t v);         // returns vector length
void VectorNormalizeFast(vec3_t v);       // does NOT return vector length, uses rsqrt approximation
vec_t VectorNormalize2(const vec3_t v, vec3_t out);
void VectorInverse(vec3_t v);

int     Q_rand(int *seed);
float   Q_random(int *seed);
float   Q_crandom(int *seed);

#define random()    ((rand() & 0x7fff) / ((float)0x7fff))
#define crandom()   (2.0 * (random() - 0.5))

void vectoangles(const vec3_t value1, vec3_t angles);
float vectoyaw(const vec3_t vec);
void AnglesToAxis(const vec3_t angles, vec3_t axis[3]);
void AxisToAngles(vec3_t axis[3], vec3_t angles);
float VectorDistance(vec3_t v1, vec3_t v2);
float VectorDistanceSquared(vec3_t v1, vec3_t v2);

void AxisClear(vec3_t axis[3]);
void AxisCopy(vec3_t in[3], vec3_t out[3]);

float   AngleMod(float a);
float   LerpAngle(float from, float to, float frac);
void    LerpPosition(vec3_t start, vec3_t end, float frac, vec3_t out);
float   AngleSubtract(float a1, float a2);
void    AnglesSubtract(vec3_t v1, vec3_t v2, vec3_t v3);

float AngleNormalize360(float angle);
float AngleNormalize180(float angle);
float AngleDelta(float angle1, float angle2);

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);
void RotateAroundDirection(vec3_t axis[3], float yaw);
// perpendicular vector could be replaced by this

void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]);
void AngleVectors(const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void PerpendicularVector(vec3_t dst, const vec3_t src);

// Ridah
void GetPerpendicularViewVector(const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up);
void ProjectPointOntoVector(vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj);

// suburb
int GetDigits(float number);

char *getPhysicsName(char *physicsName, int physicsValue);
char *getPhysicsDesc(char *physicsName, int physicsValue);

//=============================================

float Com_Clamp(float min, float max, float value);

char *COM_SkipPath(char *pathname);
void    COM_StripExtension(const char *in, char *out);
void COM_StripFilename(char *in, char *out);
void    COM_BeginParseSession(const char *name);
void    COM_RestoreParseSession(char **data_p);
int     COM_GetCurrentParseLine(void);
char *COM_Parse(char **data_p);
char *COM_ParseExt(char **data_p, qboolean allowLineBreak);
void    COM_ParseError(char *format, ...) _attribute((format(printf, 1, 2)));

qboolean COM_BitCheck(const int array[], int bitNum);
void COM_BitSet(int array[], int bitNum);
void COM_BitClear(int array[], int bitNum);

// Nico, unions for fix against strict-aliasing rule break
typedef union {
	float f;
	int i;
} float_int_u;

typedef union {
	float f;
	long l;
} float_long_u;

int PASSFLOAT(float x);

#define MAX_TOKENLENGTH     1024

#ifndef TT_STRING
//token types
# define TT_STRING                   1          // string
# define TT_LITERAL                  2          // literal
# define TT_NUMBER                   3          // number
# define TT_NAME                     4          // name
# define TT_PUNCTUATION              5          // punctuation
#endif

typedef struct pc_token_s {
	int type;
	int subtype;
	int intvalue;
	float floatvalue;
	char string[MAX_TOKENLENGTH];
	int line;
	int linescrossed;
} pc_token_t;

// data is an in/out parm, returns a parsed out token

void    COM_MatchToken(char **buf_p, char *match);

void SkipBracedSection(char **program);
void SkipRestOfLine(char **data);

void Parse1DMatrix(char **buf_p, int x, float *m);

extern void trap_Error(const char *fmt);
extern void QDECL CG_Error(const char *msg, ...);
extern void QDECL G_Error(const char *msg, ...);
void QDECL Com_Error(int level, const char *error, ...) _attribute((format(printf, 2, 3)));
extern void trap_Print(const char *fmt);
extern void QDECL CG_Printf(const char *msg, ...);
extern void QDECL G_Printf(const char *msg, ...);
void QDECL Com_Printf(const char *msg, ...) _attribute((format(printf, 1, 2)));
void QDECL Com_sprintf(char *dest, int size, const char *fmt, ...) _attribute((format(printf, 3, 4)));

// mode parm for FS_FOpenFile
typedef enum {
	FS_READ,
	FS_WRITE,
	FS_APPEND,
	FS_APPEND_SYNC
} fsMode_t;

typedef enum {
	FS_SEEK_CUR,
	FS_SEEK_END,
	FS_SEEK_SET
} fsOrigin_t;

//=============================================

int Q_isupper(int c);

// portable case insensitive compare
int     Q_stricmp(const char *s1, const char *s2);
int     Q_strncmp(const char *s1, const char *s2, int n);
int     Q_stricmpn(const char *s1, const char *s2, int n);
char *Q_strlwr(char *s1);
char *Q_strupr(char *s1);

// buffer size safe library replacements
void    Q_strncpyz(char *dest, const char *src, int destsize);
void    Q_strcat(char *dest, int size, const char *src);

// removes color sequences from string
char *Q_CleanStr(char *string);
// removes whitespaces and other bad directory characters
char *Q_CleanDirName(char *dirname);

int Q_vsnprintf(char *str, size_t size, const char *format, va_list ap);

//=============================================

char *QDECL va(const char *format, ...) _attribute((format(printf, 1, 2)));
float *tv(float x, float y, float z);

//=============================================

//
// key / value info strings
//
char *Info_ValueForKey(const char *s, const char *key);
void Info_RemoveKey(char *s, const char *key);
void Info_SetValueForKey(char *s, const char *key, const char *value);
qboolean Info_Validate(const char *s);

/*
==========================================================

CVARS (console variables)

Many variables can be used for cheating purposes, so when
cheats is zero, force all unspecified variables to their
default values.
==========================================================
*/

#define CVAR_ARCHIVE        1   // set to cause it to be saved to vars.rc
                                // used for system variables, not for player
                                // specific configurations
#define CVAR_USERINFO       2   // sent to server on connect or change
#define CVAR_SERVERINFO     4   // sent in response to front end requests
#define CVAR_SYSTEMINFO     8   // these cvars will be duplicated on all clients
#define CVAR_INIT           16  // don't allow change from console at all,
                                // but can be set from the command line
#define CVAR_LATCH          32  // will only change when C code next does
                                // a Cvar_Get(), so it can't be changed
                                // without proper initialization.  modified
                                // will be set, even though the value hasn't
                                // changed yet
#define CVAR_ROM            64  // display only, cannot be set by user at all
#define CVAR_USER_CREATED   128 // created by a set command
#define CVAR_TEMP           256 // can be set even when cheats are disabled, but is not archived
#define CVAR_CHEAT          512 // can not be changed if cheats are disabled
#define CVAR_NORESTART      1024    // do not clear when a cvar_restart is issued
#define CVAR_WOLFINFO       2048    // DHM - NERVE :: Like userinfo, but for wolf multiplayer info

#define CVAR_UNSAFE         4096    // ydnar: unsafe system cvars (renderer, sound settings, anything that might cause a crash)
#define CVAR_SERVERINFO_NOUPDATE        8192    // gordon: WONT automatically send this to clients, but server browsers will see it

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s {
	char *name;
	char *string;
	char *resetString;              // cvar_restart will reset to this value
	char *latchedString;            // for CVAR_LATCH vars
	int flags;
	qboolean modified;              // set each time the cvar is changed
	int modificationCount;          // incremented each time the cvar is changed
	float value;                    // atof( string )
	int integer;                    // atoi( string )
	struct cvar_s *next;
	struct cvar_s *hashNext;
} cvar_t;

#define MAX_CVAR_VALUE_STRING   256

typedef int cvarHandle_t;

// the modules that run in the virtual machine can't access the cvar_t directly,
// so they must ask for structured updates
typedef struct {
	cvarHandle_t handle;
	int modificationCount;
	float value;
	int integer;
	char string[MAX_CVAR_VALUE_STRING];
} vmCvar_t;

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

#include "surfaceflags.h"            // shared with the q3map utility

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s {
	vec3_t normal;
	float dist;
	byte type;              // for fast side tests: 0,1,2 = axial, 3 = nonaxial
	byte signbits;          // signx + (signy<<1) + (signz<<2), used as lookup during collision
	byte pad[2];
} cplane_t;

// a trace is returned when a box is swept through the world
typedef struct {
	qboolean allsolid;      // if true, plane is not valid
	qboolean startsolid;    // if true, the initial point was in a solid area
	float fraction;         // time completed, 1.0 = didn't hit anything
	vec3_t endpos;          // final position
	cplane_t plane;         // surface normal at impact, transformed to world space
	int surfaceFlags;           // surface hit
	int contents;           // contents on other side of surface hit
	int entityNum;          // entity the contacted sirface is a part of
} trace_t;

// markfragments are returned by CM_MarkFragments()
typedef struct {
	int firstPoint;
	int numPoints;
} markFragment_t;

typedef struct {
	vec3_t origin;
	vec3_t axis[3];
} orientation_t;

//=====================================================================

// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE        0x0001
#define KEYCATCH_UI             0x0002
#define KEYCATCH_MESSAGE        0x0004
#define KEYCATCH_CGAME          0x0008

// sound channels
// channel 0 never willingly overrides
// other channels will allways override a playing sound on that channel
typedef enum {
	CHAN_AUTO,
	CHAN_LOCAL,     // menu sounds, etc
	CHAN_WEAPON,
	CHAN_VOICE,
	CHAN_ITEM,
	CHAN_BODY,
	CHAN_LOCAL_SOUND,   // chat messages, etc
	CHAN_ANNOUNCER,     // announcer voices, etc
	CHAN_VOICE_BG,  // xkan - background sound for voice (radio static, etc.)
} soundChannel_t;

/*
========================================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

========================================================================
*/
#define ANIM_BITS       10

#define ANGLE2SHORT(x)  ((int)((x) * 65536 / 360) & 65535)
#define SHORT2ANGLE(x)  ((x) * (360.0 / 65536))

#define SNAPFLAG_RATE_DELAYED   1
#define SNAPFLAG_NOT_ACTIVE     2   // snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT    4   // toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define MAX_CLIENTS         64 // JPW NERVE back to q3ta default was 128		// absolute limit

#define GENTITYNUM_BITS     10  // JPW NERVE put q3ta default back for testing	// don't need to send any more
#define MAX_GENTITIES       (1 << GENTITYNUM_BITS)

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define ENTITYNUM_NONE      (MAX_GENTITIES - 1)
#define ENTITYNUM_WORLD     (MAX_GENTITIES - 2)
#define ENTITYNUM_MAX_NORMAL    (MAX_GENTITIES - 2)

#define MAX_MODELS          256     // these are sent over the net as 8 bits (Gordon: upped to 9 bits, erm actually it was already at 9 bits, wtf? NEVAR TRUST GAMECODE COMMENTS, comments are evil :E, lets hope it doesnt horribly break anything....)
#define MAX_SOUNDS          256     // so they cannot be blindly increased
#define MAX_CS_SKINS        64
#define MAX_CSSTRINGS       32

#define MAX_CS_SHADERS      32
#define MAX_SERVER_TAGS     256
#define MAX_TAG_FILES       64

#define MAX_MULTI_SPAWNTARGETS  16 // JPW NERVE

#define MAX_CONFIGSTRINGS   1024

#define MAX_DLIGHT_CONFIGSTRINGS    16
#define MAX_SPLINE_CONFIGSTRINGS    8

#define PARTICLE_SNOW128    1
#define PARTICLE_SNOW64     2
#define PARTICLE_SNOW32     3
#define PARTICLE_SNOW256    0

#define PARTICLE_BUBBLE8    4
#define PARTICLE_BUBBLE16   5
#define PARTICLE_BUBBLE32   6
#define PARTICLE_BUBBLE64   7

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define CS_SERVERINFO       0       // an info string with all the serverinfo cvars
#define CS_SYSTEMINFO       1       // an info string for server system to client system configuration (timescale, etc)

#define RESERVED_CONFIGSTRINGS  2   // game can't modify below this, only the system can

#define MAX_GAMESTATE_CHARS 16000
typedef struct {
	int stringOffsets[MAX_CONFIGSTRINGS];
	char stringData[MAX_GAMESTATE_CHARS];
	int dataCount;
} gameState_t;

// xkan, 1/10/2003 - adapted from original SP
typedef enum {
	AISTATE_RELAXED,
	AISTATE_QUERY,
	AISTATE_ALERT,
	AISTATE_COMBAT,

	MAX_AISTATES
} aistateEnum_t;

#define REF_FORCE_DLIGHT    (1 << 31)   // RF, passed in through overdraw parameter, force this dlight under all conditions
#define REF_JUNIOR_DLIGHT   (1 << 30)   // (SA) this dlight does not light surfaces.  it only affects dynamic light grid
#define REF_DIRECTED_DLIGHT (1 << 29)   // ydnar: global directional light, origin should be interpreted as a normal vector

// bit field limits
#define MAX_STATS               16
#define MAX_PERSISTANT          16
#define MAX_POWERUPS            16
#define MAX_WEAPONS             64  // (SA) and yet more!
#define MAX_EVENTS              4   // max events per frame before we drop events

#define PS_PMOVEFRAMECOUNTBITS  6

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c
// (Gordon: unless it doesnt need transmitted over the network, in which case it should prolly go in the new pmext struct anyway)

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)
typedef struct playerState_s {
	int commandTime;            // cmd->serverTime of last executed command
	int pm_type;
	int bobCycle;               // for view bobbing and footstep generation
	int pm_flags;               // ducked, jump_held, etc
	int pm_time;

	vec3_t origin;
	vec3_t velocity;
	int weaponTime;
	int weaponDelay;            // for weapons that don't fire immediately when 'fire' is hit (grenades, venom, ...)
	int grenadeTimeLeft;            // for delayed grenade throwing.  this is set to a #define for grenade
	                                // lifetime when the attack button goes down, then when attack is released
	                                // this is the amount of time left before the grenade goes off (or if it
	                                // gets to 0 while in players hand, it explodes)
	int gravity;
	float leanf;                // amount of 'lean' when player is looking around corner //----(SA)	added

	int speed;
	int delta_angles[3];            // add to command angles to get view direction
	                                // changed by spawns, rotating objects, and teleporters

	int groundEntityNum;        // ENTITYNUM_NONE = in air

	int legsTimer;              // don't change low priority animations until this runs out
	int legsAnim;               // mask off ANIM_TOGGLEBIT

	int torsoTimer;             // don't change low priority animations until this runs out
	int torsoAnim;              // mask off ANIM_TOGGLEBIT

	int movementDir;            // a number 0 to 7 that represents the reletive angle
	                            // of movement to the view angle (axial and diagonals)
	                            // when at rest, the value will remain unchanged
	                            // used to twist the legs during strafing

	int eFlags;                 // copied to entityState_t->eFlags

	int eventSequence;          // pmove generated events
	int events[MAX_EVENTS];
	int eventParms[MAX_EVENTS];
	int oldEventSequence;           // so we can see which events have been added since we last converted to entityState_t

	int externalEvent;          // events set on player from another source
	int externalEventParm;
	int externalEventTime;

	int clientNum;              // ranges from 0 to MAX_CLIENTS-1

	// weapon info
	int weapon;                 // copied to entityState_t->weapon
	int weaponstate;

	// item info
	int item;

	vec3_t viewangles;          // for fixed views
	int viewheight;

	// damage feedback
	int damageEvent;            // when it changes, latch the other parms
	int damageYaw;
	int damagePitch;
	int damageCount;

	int stats[MAX_STATS];
	int persistant[MAX_PERSISTANT];         // stats that aren't cleared on death
	int powerups[MAX_POWERUPS];         // level.time that the powerup runs out
	int ammo[MAX_WEAPONS];              // total amount of ammo
	int ammoclip[MAX_WEAPONS];          // ammo in clip
	int holdable[16];
	int holding;                        // the current item in holdable[] that is selected (held)
	int weapons[MAX_WEAPONS / (sizeof (int) * 8)];       // 64 bits for weapons held

	// Ridah, allow for individual bounding boxes
	vec3_t mins, maxs;
	float crouchMaxZ;
	float crouchViewHeight, standViewHeight, deadViewHeight;
	// variable movement speed
	float runSpeedScale, sprintSpeedScale, crouchSpeedScale;
	// done.

	// Ridah, view locking for mg42
	int viewlocked;
	int viewlocked_entNum;

	float friction;

	int nextWeapon;
	int teamNum;                        // Arnout: doesn't seem to be communicated over the net

	// RF, burning effect is required for view blending effect
	int onFireStart;

	int serverCursorHint;               // what type of cursor hint the server is dictating
	int serverCursorHintVal;            // a value (0-255) associated with the above

	trace_t serverCursorHintTrace;      // not communicated over net, but used to store the current server-side cursorhint trace

	// ----------------------------------------------------------------------
	// So to use persistent variables here, which don't need to come from the server,
	// we could use a marker variable, and use that to store everything after it
	// before we read in the new values for the predictedPlayerState, then restore them
	// after copying the structure recieved from the server.

	// Arnout: use the pmoveExt_t structure in bg_public.h to store this kind of data now (presistant on client, not network transmitted)

	int ping;                   // server to game info for scoreboard
	int pmove_framecount;
	int entityEventSequence;

	int sprintExertTime; // Nico, note keep this even if it's unused

	// JPW NERVE -- value for all multiplayer classes with regenerating "class weapons" -- ie LT artillery, medic medpack, engineer build points, etc
	int classWeaponTime;                // Arnout : DOES get send over the network
	int jumpTime;                   // used in MP to prevent jump accel
	// jpw

	int weapAnim;                   // mask off ANIM_TOGGLEBIT										//----(SA)	added		// Arnout : DOES get send over the network

	qboolean releasedFire;

	float aimSpreadScaleFloat;          // (SA) the server-side aimspreadscale that lets it track finer changes but still only
	                                    // transmit the 8bit int to the client
	int aimSpreadScale;                 // 0 - 255 increases with angular movement		// Arnout : DOES get send over the network
	int lastFireTime;                   // used by server to hold last firing frame briefly when randomly releasing trigger (AI)

	int quickGrenTime;

	int leanStopDebounceTime;

//----(SA)	added

	// seems like heat and aimspread could be tied together somehow, however, they (appear to) change at different rates and
	// I can't currently see how to optimize this to one server->client transmission "weapstatus" value.
	int weapHeat[MAX_WEAPONS];          // some weapons can overheat.  this tracks (server-side) how hot each weapon currently is.
	int curWeapHeat;                    // value for the currently selected weapon (for transmission to client)		// Arnout : DOES get send over the network
	int identifyClient;                 // NERVE - SMF

	// Nico, as I cnanot add fields in this struct,
	// I will use an existing one as jump counter
	int identifyClientHealth; // Nico, jump counter

	aistateEnum_t aiState;          // xkan, 1/10/2003
} playerState_t;

//====================================================================

//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//
#define BUTTON_ATTACK       1
#define BUTTON_TALK         2           // displays talk balloon and disables actions
#define BUTTON_WALKING      16          // walking can't just be infered from MOVE_RUN
                                        // because a key pressed late in the frame will
                                        // only generate a small move value for that frame
                                        // walking will use different animations and
                                        // won't generate footsteps
//----(SA)	added
#define BUTTON_SPRINT       32
#define BUTTON_ACTIVATE     64
//----(SA)	end

#define BUTTON_ANY          128         // any key whatsoever

//----(SA) wolf buttons
#define WBUTTON_ATTACK2     1
#define WBUTTON_ZOOM        2
#define WBUTTON_RELOAD      8
#define WBUTTON_LEANLEFT    16
#define WBUTTON_LEANRIGHT   32
#define WBUTTON_DROP        64 // JPW NERVE
#define WBUTTON_PRONE       128 // Arnout: wbutton now
//----(SA) end

#define MOVE_RUN            120         // if forwardmove or rightmove are >= MOVE_RUN,
                                        // then BUTTON_WALKING should be set

// Arnout: doubleTap buttons - DT_NUM can be max 8
typedef enum {
	DT_NONE,
	DT_MOVELEFT,
	DT_MOVERIGHT,
	DT_FORWARD,
	DT_BACK,
	DT_LEANLEFT,
	DT_LEANRIGHT,
	DT_UP,
	DT_NUM
} dtType_t;

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s {
	int serverTime;
	byte buttons;
	byte wbuttons;
	byte weapon;
	byte flags;
	int angles[3];

	signed char forwardmove, rightmove, upmove;
	byte doubleTap;             // Arnout: only 3 bits used

	// rain - in ET, this can be any entity, and it's used as an array
	// index, so make sure it's unsigned
	byte identClient;           // NERVE - SMF
} usercmd_t;

//===================================================================

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define SOLID_BMODEL    0xffffff

typedef enum {
	TR_STATIONARY,
	TR_INTERPOLATE,             // non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_LINEAR_STOP_BACK,        //----(SA)	added.  so reverse movement can be different than forward
	TR_SINE,                    // value = base + sin( time / duration ) * delta
	TR_GRAVITY,
	// Ridah
	TR_GRAVITY_LOW,
	TR_GRAVITY_FLOAT,           // super low grav with no gravity acceleration (floating feathers/fabric/leaves/...)
	TR_GRAVITY_PAUSED,          //----(SA)	has stopped, but will still do a short trace to see if it should be switched back to TR_GRAVITY
	TR_ACCELERATE,
	TR_DECCELERATE,
	// Gordon
	TR_SPLINE,
	TR_LINEAR_PATH
} trType_t;

typedef struct {
	trType_t trType;
	int trTime;
	int trDuration;             // if non 0, trTime + trDuration = stop time
//----(SA)	removed
	vec3_t trBase;
	vec3_t trDelta;             // velocity, etc
//----(SA)	removed
} trajectory_t;

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)

typedef enum {
	ET_GENERAL,
	ET_PLAYER,
	ET_ITEM,
	ET_MISSILE,
	ET_MOVER,
	ET_BEAM,
	ET_PORTAL,
	ET_SPEAKER,
	ET_PUSH_TRIGGER,
	ET_TELEPORT_TRIGGER,
	ET_INVISIBLE,
	ET_CONCUSSIVE_TRIGGER,  // JPW NERVE trigger for concussive dust particles
	ET_OID_TRIGGER,         // DHM - Nerve :: Objective Info Display
	ET_EXPLOSIVE_INDICATOR, // NERVE - SMF

	//---- (SA) Wolf
	ET_EXPLOSIVE,           // brush that will break into smaller bits when damaged
	ET_EF_SPOTLIGHT,
	ET_ALARMBOX,
	ET_CORONA,
	ET_TRAP,

	ET_GAMEMODEL,           // misc_gamemodel.  similar to misc_model, but it's a dynamic model so we have LOD
	ET_FOOTLOCKER,  //----(SA)	added
	//---- end

	ET_FLAMEBARREL,
	ET_FP_PARTS,

	// FIRE PROPS
	ET_FIRE_COLUMN,
	ET_FIRE_COLUMN_SMOKE,
	ET_RAMJET,

	ET_FLAMETHROWER_CHUNK,      // DHM - NERVE :: Used in server side collision detection for flamethrower

	ET_EXPLO_PART,

	ET_PROP,

	ET_AI_EFFECT,

	ET_CAMERA,
	ET_MOVERSCALED,

	ET_CONSTRUCTIBLE_INDICATOR,
	ET_CONSTRUCTIBLE,
	ET_CONSTRUCTIBLE_MARKER,
	ET_BOMB,
	ET_WAYPOINT,
	ET_BEAM_2,
	ET_TANK_INDICATOR,
	ET_TANK_INDICATOR_DEAD,
	// Start - TAT - 8/29/2002
	// An indicator object created by the bot code to show where the bots are moving to
	ET_BOTGOAL_INDICATOR,
	// End - TA - 8/29/2002
	ET_CORPSE,              // Arnout: dead player
	ET_SMOKER,              // Arnout: target_smoke entity

	ET_TEMPHEAD,            // Gordon: temporary head for clients for bullet traces
	ET_MG42_BARREL,         // Arnout: MG42 barrel
	ET_TEMPLEGS,            // Arnout: temporary leg for clients for bullet traces
	ET_TRIGGER_MULTIPLE,
	ET_TRIGGER_FLAGONLY,
	ET_TRIGGER_FLAGONLY_MULTIPLE,
	ET_GAMEMANAGER,
	ET_AAGUN,
	ET_CABINET_H,
	ET_CABINET_A,
	ET_HEALER,
	ET_SUPPLIER,

	ET_LANDMINE_HINT,       // Gordon: landmine hint for botsetgoalstate filter
	ET_ATTRACTOR_HINT,      // Gordon: attractor hint for botsetgoalstate filter
	ET_SNIPER_HINT,         // Gordon: sniper hint for botsetgoalstate filter
	ET_LANDMINESPOT_HINT,   // Gordon: landminespot hint for botsetgoalstate filter

	ET_COMMANDMAP_MARKER,

	ET_WOLF_OBJECTIVE,

	ET_EVENTS               // any of the EV_* events can be added freestanding
	                        // by setting eType to ET_EVENTS + eventNum
	                        // this avoids having to set eFlags and eventNum
} entityType_t;

typedef struct entityState_s {
	int number;                     // entity index
	entityType_t eType;             // entityType_t
	int eFlags;

	trajectory_t pos;       // for calculating position
	trajectory_t apos;      // for calculating angles

	int time;
	int time2;

	vec3_t origin;
	vec3_t origin2;

	vec3_t angles;
	vec3_t angles2;

	int otherEntityNum;     // shotgun sources, etc
	int otherEntityNum2;

	int groundEntityNum;        // -1 = in air

	int constantLight;      // r + (g<<8) + (b<<16) + (intensity<<24)
	int dl_intensity;       // used for coronas
	int loopSound;          // constantly loop this sound

	int modelindex;
	int modelindex2;
	int clientNum;          // 0 to (MAX_CLIENTS - 1), for players and corpses
	int frame;

	int solid;              // for client side prediction, trap_linkentity sets this properly

	// old style events, in for compatibility only
	int event;
	int eventParm;

	int eventSequence;      // pmove generated events
	int events[MAX_EVENTS];
	int eventParms[MAX_EVENTS];

	// for players
	int powerups;           // bit flags	// Arnout: used to store entState_t for non-player entities (so we know to draw them translucent clientsided)
	int weapon;             // determines weapon and flash model, etc
	int legsAnim;           // mask off ANIM_TOGGLEBIT
	int torsoAnim;          // mask off ANIM_TOGGLEBIT

	int density;            // for particle effects

	int dmgFlags;           // to pass along additional information for damage effects for players/ Also used for cursorhints for non-player entities

	// Ridah
	int onFireStart, onFireEnd;

	int nextWeapon;
	int teamNum;

	int effect1Time, effect2Time, effect3Time;

	aistateEnum_t aiState;      // xkan, 1/10/2003
	int animMovetype;       // clients can't derive movetype of other clients for anim scripting system
} entityState_t;

typedef enum {
	CA_UNINITIALIZED,
	CA_DISCONNECTED,    // not talking to a server
	CA_AUTHORIZING,     // not used any more, was checking cd key
	CA_CONNECTING,      // sending request packets to the server
	CA_CHALLENGING,     // sending challenge packets to the server
	CA_CONNECTED,       // netchan_t established, getting gamestate
	CA_LOADING,         // only during cgame initialization, never during main loop
	CA_PRIMED,          // got gamestate, waiting for first frame
	CA_ACTIVE,          // game views should be displayed
	CA_CINEMATIC        // playing a cinematic or a static pic, not connected to a server
} connstate_t;

// font support

#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND 127
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1
typedef struct {
	int height;       // number of scan lines
	int top;          // top of glyph in buffer
	int bottom;       // bottom of glyph in buffer
	int pitch;        // width for copying
	int xSkip;        // x adjustment
	int imageWidth;   // width of actual image
	int imageHeight;  // height of actual image
	float s;          // x offset in image where glyph starts
	float t;          // y offset in image where glyph starts
	float s2;
	float t2;
	qhandle_t glyph;  // handle to the shader with the glyph
	char shaderName[32];
} glyphInfo_t;

typedef struct {
	glyphInfo_t glyphs[GLYPHS_PER_FONT];
	float glyphScale;
	char name[MAX_QPATH];
} fontInfo_t;

#define Square(x) ((x) * (x))

// real time
//=============================================

typedef struct qtime_s {
	int tm_sec;     /* seconds after the minute - [0,59] */
	int tm_min;     /* minutes after the hour - [0,59] */
	int tm_hour;    /* hours since midnight - [0,23] */
	int tm_mday;    /* day of the month - [1,31] */
	int tm_mon;     /* months since January - [0,11] */
	int tm_year;    /* years since 1900 */
	int tm_wday;    /* days since Sunday - [0,6] */
	int tm_yday;    /* days since January 1 - [0,365] */
	int tm_isdst;   /* daylight savings time flag */
} qtime_t;

// server browser sources
#define AS_LOCAL        0
#define AS_GLOBAL       1           // NERVE - SMF - modified
#define AS_FAVORITES    2

// cinematic states
typedef enum {
	FMV_IDLE,
	FMV_PLAY,       // play
	FMV_EOF,        // all other conditions, i.e. stop/EOF/abort
	FMV_ID_BLT,
	FMV_ID_IDLE,
	FMV_LOOPED,
	FMV_ID_WAIT
} e_status;

typedef enum _flag_status {
	FLAG_ATBASE = 0,
	FLAG_TAKEN,         // CTF
	FLAG_TAKEN_RED,     // One Flag CTF
	FLAG_TAKEN_BLUE,    // One Flag CTF
	FLAG_DROPPED
} flagStatus_t;

#define MAX_SERVERSTATUSREQUESTS    16

// NERVE - SMF - wolf server/game states
typedef enum {
	GS_INITIALIZE = -1,
	GS_PLAYING,
	GS_WARMUP_COUNTDOWN,
	GS_WARMUP,
	GS_INTERMISSION,
	GS_RESET
} gamestate_t;

#define SQR(a) ((a) * (a))

// Nico, flags for physics
#define PHYSICS_NORMAL              0
#define PHYSICS_FLAT_JUMPING        1
#define PHYSICS_NO_FALLDAMAGE       2
#define PHYSICS_RAMPBOUNCE          4
#define PHYSICS_AIRCONTROL          8
#define PHYSICS_NO_OVERBOUNCE       16
#define PHYSICS_UPMOVE_BUG_FIX      32
#define PHYSICS_DOUBLEJUMP          64
#define PHYSICS_SLICK_CONTROL       128

// Nico, physics mode
#define PHYSICS_MODE_AP_NO_OB       255
#define PHYSICS_MODE_AP_OB          239
#define PHYSICS_MODE_VQ3_NO_OB      19
#define PHYSICS_MODE_VQ3_OB         3
#define PHYSICS_MODE_VET            0

// Nico, IP max length
#define IP_MAX_LENGTH               46

#endif  // __Q_SHARED_H
