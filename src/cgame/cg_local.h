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
 * name:	cg_local.h
 *
 * desc:	The entire cgame module is unloaded and reloaded on each level change,
 *			so there is NO persistant data between levels on the client side.
 *			If you absolutely need something stored, it can either be kept
 *			by the server in the server stored userinfos, or stashed in a cvar.

 *
*/

#include "../game/q_shared.h"
#include "tr_types.h"
#include "../game/bg_public.h"
#include "cg_public.h"
#include "../ui/ui_shared.h"

#define STATS_FADE_TIME     200.0f
#define FADE_TIME           200
#define DAMAGE_DEFLECT_TIME 100
#define DAMAGE_RETURN_TIME  400
#define LAND_DEFLECT_TIME   150
#define LAND_RETURN_TIME    300
#define STEP_TIME           200
#define DUCK_TIME           100
#define PAIN_TWITCH_TIME    200
#define ZOOM_TIME           150
#define MUZZLE_FLASH_TIME   30

#define PRONE_TIME          500

#define MAX_STEP_CHANGE     32

#define MAX_VERTS_ON_POLY   10
#define MAX_MARK_POLYS      256 // JPW NERVE was 1024

#define TEAMCHAT_WIDTH      70
#define TEAMCHAT_HEIGHT     8
#define CHAT_WIDTH          428

#define NOTIFY_WIDTH        80
#define NOTIFY_HEIGHT       5

#define NUM_CROSSHAIRS      10

// Ridah, trails
#define STYPE_STRETCH   0
#define STYPE_REPEAT    1

#define TJFL_FADEIN     (1 << 0)
#define TJFL_CROSSOVER  (1 << 1)
#define TJFL_NOCULL     (1 << 2)
#define TJFL_FIXDISTORT (1 << 3)
#define TJFL_SPARKHEADFLARE (1 << 4)
#define TJFL_NOPOLYMERGE    (1 << 5)
// done.

// OSP

// Demo controls
#define DEMO_THIRDPERSONUPDATE  0
#define DEMO_RANGEDELTA         6
#define DEMO_ANGLEDELTA         4

#define MAX_WINDOW_COUNT        10
#define MAX_WINDOW_LINES        64

#define MAX_STRINGS             80
#define MAX_STRING_POOL_LENGTH  128

#define WINDOW_FONTWIDTH    8       // For non-true-type: width to scale from
#define WINDOW_FONTHEIGHT   8       // For non-true-type: height to scale from

#define WID_NONE            0x00    // General window

#define WFX_TEXTSIZING      0x01    // Size the window based on text/font setting
#define WFX_FLASH           0x02    // Alternate between bg and b2 every half second
#define WFX_TRUETYPE        0x04    // Use truetype fonts for text
// These need to be last
#define WFX_FADEIN          0x10    // Fade the window in (and back out when closing)
#define WFX_SCROLLUP        0x20    // Scroll window up from the bottom (and back down when closing)

#define WSTATE_COMPLETE     0x00    // Window is up with startup effects complete
#define WSTATE_START        0x01    // Window is "initializing" w/effects
#define WSTATE_SHUTDOWN     0x02    // Window is shutting down with effects
#define WSTATE_OFF          0x04    // Window is completely shutdown

// Nico, autodemo
#define AUTODEMO_RUN_SAVE_DELAY 1500
#define AUTODEMO_MAX_DEMOS      20

typedef struct {
	vec4_t colorBorder;         // Window border color
	vec4_t colorBackground;     // Window fill color
	vec4_t colorBackground2;    // Window fill color2 (for flashing)
	int curX;                   // Scrolling X position
	int curY;                   // Scrolling Y position
	int effects;                // Window effects
	int flashMidpoint;          // Flashing transition point (in ms)
	int flashPeriod;            // Background flashing period (in ms)
	int fontHeight;             // For non-truetype font drawing
	float fontScaleX;           // Font scale factor
	float fontScaleY;           // Font scale factor
	int fontWidth;              // For non-truetype font drawing
	float h;                    // Height
	int id;                     // Window ID for special handling (i.e. stats, motd, etc.)
	qboolean inuse;             // Activity flag
	int lineCount;              // Number of lines to display
	int lineHeight[MAX_WINDOW_LINES];   // Height property for each line
	char *lineText[MAX_WINDOW_LINES];   // Text info
	float m_x;                  // Mouse X position
	float m_y;                  // Mouse Y position

	int targetTime;             // Time to complete any defined effect
	int state;                  // Current state of the window
	int time;                   // Current window time
	float w;                    // Width
	float x;                    // Target x-coordinate
	                            //    negative values will align the window from the right minus the (window width + offset(x))
	float y;                    // Target y-coordinate
	                            //    negative values will align the window from the bottom minus the (window height + offset(y))
} cg_window_t;

typedef struct {
	qboolean fActive;
	char str[MAX_STRING_POOL_LENGTH];
} cg_string_t;

typedef struct {
	int activeWindows[MAX_WINDOW_COUNT];                // List of active windows
	int numActiveWindows;                               // Number of active windows in use
	cg_window_t window[MAX_WINDOW_COUNT];           // Static allocation of all windows
} cg_windowHandler_t;
// OSP

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct {
	int oldFrame;
	int oldFrameTime;               // time when ->oldFrame was exactly on
	qhandle_t oldFrameModel;

	int frame;
	int frameTime;                  // time when ->frame will be exactly on
	qhandle_t frameModel;

	float backlerp;

	float yawAngle;
	qboolean yawing;
	float pitchAngle;
	qboolean pitching;

	int animationNumber;            // may include ANIM_TOGGLEBIT
	int oldAnimationNumber;         // may include ANIM_TOGGLEBIT
	animation_t *animation;
	int animationTime;              // time when the first frame of the animation will be exact

	// Ridah, variable speed anims
	vec3_t oldFramePos;
	float animSpeedScale;
	int oldFrameSnapshotTime;
	// done.
} lerpFrame_t;

typedef struct {
	lerpFrame_t legs, torso;
	lerpFrame_t head;
	lerpFrame_t weap;       //----(SA)	autonomous weapon animations
	lerpFrame_t hudhead;

	int painTime;
	int painDuration;
	int painDirection;              // flip from 0 to 1
	int painAnimTorso;
	int painAnimLegs;
	int lightningFiring;

	// Ridah, so we can do fast tag grabbing
	refEntity_t bodyRefEnt, headRefEnt, gunRefEnt;
	int gunRefEntFrame;

	float animSpeed;            // for manual adjustment

	int lastFiredWeaponTime;
	int weaponFireTime;
} playerEntity_t;

//=================================================

typedef struct tag_s {
	vec3_t origin;
	vec3_t axis[3];
} tag_t;

// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s {
	entityState_t currentState;     // from cg.frame
	entityState_t nextState;        // from cg.nextFrame, if available
	qboolean interpolate;           // true if next is valid to interpolate to
	qboolean currentValid;          // true if cg.frame holds this entity

	int muzzleFlashTime;                // move to playerEntity?
	int overheatTime;
	int previousEvent;
	int previousEventSequence;              // Ridah
	int teleportFlag;

	int trailTime;                  // so missile trails can handle dropped initial packets
	int miscTime;
	int soundTime;                  // ydnar: so looping sounds can start when triggered

	playerEntity_t pe;

	vec3_t rawOrigin;
	vec3_t rawAngles;

	// exact interpolated position of entity on this frame
	vec3_t lerpOrigin;
	vec3_t lerpAngles;

	vec3_t lastLerpAngles;          // (SA) for remembering the last position when a state changes
	vec3_t lastLerpOrigin;          // Gordon: Added for linked trains player adjust prediction

	// Ridah, trail effects
	int headJuncIndex, headJuncIndex2;
	int lastTrailTime;
	// done.

	// Ridah
	vec3_t fireRiseDir;             // if standing still this will be up, otherwise it'll point away from movement dir
	int lastFuseSparkTime;

	// client side dlights
	int dl_frame;
	int dl_oldframe;
	float dl_backlerp;
	int dl_time;
	char dl_stylestring[64];
	int dl_sound;
	int dl_atten;

	lerpFrame_t lerpFrame;      //----(SA)	added
	vec3_t highlightOrigin;             // center of the geometry.  for things like corona placement on treasure
	qboolean usehighlightOrigin;

	refEntity_t refEnt;
	int processedFrame;                 // frame we were last added to the scene

	int voiceChatSprite;                    // DHM - Nerve
	int voiceChatSpriteTime;                // DHM - Nerve

	// item highlighting
	int highlightTime;
	qboolean highlighted;

	// spline stuff
	vec3_t origin2;
	splinePath_t *backspline;
	float backdelta;
	qboolean back;
	qboolean moving;

	int tankframe;
	int tankparent;
	tag_t mountedMG42Base;
	tag_t mountedMG42Nest;
	tag_t mountedMG42;
	tag_t mountedMG42Player;
	tag_t mountedMG42Flash;

	qboolean akimboFire;

	// Gordon: tagconnect cleanup..
	int tagParent;
	char tagName[MAX_QPATH];
} centity_t;

//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s {
	struct markPoly_s *prevMark, *nextMark;
	int time;
	qhandle_t markShader;
	qboolean alphaFade;         // fade alpha instead of rgb
	float color[4];
	poly_t poly;
	polyVert_t verts[MAX_VERTS_ON_POLY];

	int duration;           // Ridah
} markPoly_t;

//----(SA)	moved in from cg_view.c
typedef enum {
	ZOOM_NONE,
	ZOOM_BINOC,
	ZOOM_SNIPER,
	ZOOM_SNOOPER,
	ZOOM_FG42SCOPE,
	ZOOM_MG42,
} EZoom_t;

typedef enum {
	ZOOM_OUT,   // widest angle
	ZOOM_IN,    // tightest angle (approaching 0)
	ZOOM_SCOPE  // suburb, for zoomed scope
} EZoomInOut_t;

//----(SA)	end

typedef enum {
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SPARK,
	LE_DEBRIS,
	LE_BLOOD,
	LE_FUSE_SPARK,
	LE_MOVING_TRACER,
	LE_EMITTER
} leType_t;

typedef enum {
	LEF_PUFF_DONT_SCALE = 0x0001            // do not scale size over time
	, LEF_TUMBLE        = 0x0002            // tumble over time, used for ejecting shells
	, LEF_NOFADEALPHA   = 0x0004            // Ridah, sparks
	, LEF_SMOKING       = 0x0008            // (SA) smoking
	, LEF_TUMBLE_SLOW   = 0x0010            // slow down tumble on hitting ground
} leFlag_t;

typedef enum {
	LEMT_NONE,
	LEMT_BLOOD
} leMarkType_t;         // fragment local entities can leave marks on walls

typedef enum {
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_ROCK,
	LEBS_WOOD,
	LEBS_BRASS,
	LEBS_METAL,
	LEBS_BONE
} leBounceSoundType_t;  // fragment local entities can make sounds on impacts

typedef struct localEntity_s {
	struct localEntity_s *prev, *next;
	leType_t leType;
	int leFlags;

	int startTime;
	int endTime;
	int fadeInTime;

	float lifeRate;                     // 1.0 / (endTime - startTime)

	trajectory_t pos;
	trajectory_t angles;

	float bounceFactor;                 // 0.0 = no bounce, 1.0 = perfect

	float color[4];

	float radius;

	float light;
	vec3_t lightColor;

	leMarkType_t leMarkType;            // mark to leave on fragment impact
	leBounceSoundType_t leBounceSoundType;

	refEntity_t refEntity;

	// Ridah
	int lightOverdraw;
	int lastTrailTime;
	int headJuncIndex, headJuncIndex2;
	float effectWidth;
	int effectFlags;
	struct localEntity_s *chain;        // used for grouping entities (like for flamethrower junctions)
	int onFireStart, onFireEnd;
	int ownerNum;
	int lastSpiritDmgTime;

	int loopingSound;

	int breakCount;                     // break-up this many times before we can break no more
	float sizeScale;
	// done.
} localEntity_t;

//======================================================================

typedef struct {
	int client;
	int ping;
	int time;
	int team;

	// Nico, timerun best time
	int timerunBestTime;

	// Nico, timerun best speed
	int timerunBestSpeed;

	// Nico, timerun status
	int timerunStatus;

	// Nico, followed client
	int followedClient;

	// Nico, client login status
	int logged;

	// Nico, client cgaz setting
	int cgaz;

	// Nico, speclock status
	int speclocked;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define MAX_CUSTOM_SOUNDS   32
typedef struct clientInfo_s {
	qboolean infoValid;

	int clientNum;

	char name[MAX_QPATH];
	char cleanname[MAX_QPATH];
	team_t team;

	int score;                      // updated by score servercmds
	int location[2];                // location in 2d for team mode
	int health;                     // you only get this info about your teammates
	int curWeapon;
	int breathPuffTime;
	int cls;
	int blinkTime;              //----(SA)

	int handshake;
	int rank;
	qboolean ccSelected;
	int fireteam;

	int weapon;
	int secondaryweapon;
	int latchedweapon;

	int refStatus;

	bg_character_t *character;

	// Gordon: caching fireteam pointer here, better than trying to work it out all the time
	fireteamData_t *fireteamData;

	// Gordon: for fireteams, has been selected
	qboolean selected;

	// OSP - per client MV ps info
	int ammo;
	int ammoclip;
	int chargeTime;
	qboolean fCrewgun;
	int cursorHint;
	int grenadeTimeLeft;                // Actual time remaining
	int grenadeTimeStart;               // Time trigger base to compute TimeLeft
	int hintTime;

	int weapHeat;
	int weaponState;
	int weaponState_last;

	// Nico, pmove_fixed
	int pmoveFixed;

	// Nico, login status
	qboolean logged;

	// Nico, hideme
	qboolean hideme;

	// Nico, country code (GeoIP)
	unsigned int countryCode;
} clientInfo_t;

typedef enum {
	W_PART_1,
	W_PART_2,
	W_PART_3,
	W_PART_4,
	W_PART_5,
	W_PART_6,
	W_PART_7,
	W_MAX_PARTS
} barrelType_t;

typedef enum {
	W_TP_MODEL,         //	third person model
	W_FP_MODEL,         //	first person model
	W_PU_MODEL,         //	pickup model
	W_NUM_TYPES
} modelViewType_t;

typedef struct partModel_s {
	char tagName[MAX_QPATH];
	qhandle_t model;
	qhandle_t skin[3];              // 0: neutral, 1: axis, 2: allied
} partModel_t;

typedef struct weaponModel_s {
	qhandle_t model;
	qhandle_t skin[3];              // 0: neutral, 1: axis, 2: allied
} weaponModel_t;

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s {
	qboolean registered;

	animation_t weapAnimations[MAX_WP_ANIMATIONS];

	qhandle_t handsModel;               // the hands don't actually draw, they just position the weapon

	qhandle_t standModel;               // not drawn.  tags used for positioning weapons for pickup
	qboolean droppedAnglesHack;

	weaponModel_t weaponModel[W_NUM_TYPES];
	partModel_t partModels[W_NUM_TYPES][W_MAX_PARTS];
	qhandle_t flashModel[W_NUM_TYPES];
	qhandle_t modModels[6];         // like the scope for the rifles

	vec3_t flashDlightColor;
	sfxHandle_t flashSound[4];          // fast firing weapons randomly choose
	sfxHandle_t flashEchoSound[4];      //----(SA)	added - distant gun firing sound
	sfxHandle_t lastShotSound[4];       // sound of the last shot can be different (mauser doesn't have bolt action on last shot for example)

	qhandle_t weaponIcon[2];            //----(SA)	[0] is weap icon, [1] is highlight icon
	qhandle_t ammoIcon;

	qhandle_t missileModel;
	qhandle_t missileAlliedSkin;
	qhandle_t missileAxisSkin;
	sfxHandle_t missileSound;
	void (*missileTrailFunc)(centity_t *);
	float missileDlight;
	vec3_t missileDlightColor;
	int missileRenderfx;

	void (*ejectBrassFunc)(centity_t *);

	sfxHandle_t readySound;             // an amibient sound the weapon makes when it's /not/ firing
	sfxHandle_t firingSound;
	sfxHandle_t overheatSound;
	sfxHandle_t reloadSound;
	sfxHandle_t reloadFastSound;

	sfxHandle_t spinupSound;        //----(SA)	added // sound started when fire button goes down, and stepped on when the first fire event happens
	sfxHandle_t spindownSound;      //----(SA)	added // sound called if the above is running but player doesn't follow through and fire

	sfxHandle_t switchSound;
} weaponInfo_t;

// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct {
	qboolean registered;
	qhandle_t models[MAX_ITEM_MODELS];
	qhandle_t icons[MAX_ITEM_ICONS];
} itemInfo_t;

typedef struct {
	int itemNum;
} powerupInfo_t;

#define MAX_VIEWDAMAGE  8
typedef struct {
	int damageTime, damageDuration;
	float damageX, damageY, damageValue;
} viewDamage_t;
//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS    16

#define MAX_SPAWN_VARS          64
#define MAX_SPAWN_VARS_CHARS    2048

#define MAX_SPAWNPOINTS 32
#define MAX_SPAWNDESC   128

#define MAX_BUFFERED_SOUNDSCRIPTS 16

#define MAX_SOUNDSCRIPT_SOUNDS 16

typedef struct soundScriptHandle_s {
	char filename[MAX_QPATH];
	sfxHandle_t sfxHandle;
} soundScriptHandle_t;

typedef struct soundScriptSound_s {
	soundScriptHandle_t sounds[MAX_SOUNDSCRIPT_SOUNDS];

	int numsounds;
	int lastPlayed;

	struct soundScriptSound_s *next;
} soundScriptSound_t;

typedef struct soundScript_s {
	int index;
	char name[MAX_QPATH];
	int channel;
	int attenuation;
	qboolean streaming;
	qboolean looping;
	qboolean random;    // TODO
	int numSounds;
	soundScriptSound_t *soundList;          // pointer into the global list of soundScriptSounds (defined below)

	struct soundScript_s *nextHash;         // next soundScript in our hashTable list position
} soundScript_t;

typedef struct {
	int x, y, z;
	int yaw;
	int data;
	char type;

	team_t team;
} mapEntityData_t;

// START	xkan, 8/29/2002
// the most buddies we can have
#define MAX_NUM_BUDDY  6
// END		xkan, 8/29/2002

typedef enum {
	SHOW_OFF,
	SHOW_SHUTDOWN,
	SHOW_ON
} showView_t;

#define MAX_BACKUP_STATES (CMD_BACKUP + 2)
// Nico, used to show pressed keys
#define NUM_KEYS_SETS       3
#define KEYS_AMOUNT         8
typedef struct {
	int clientFrame;                // incremented each frame

	int clientNum;

	qboolean demoPlayback;

	// there are only one or two snapshot_t that are relevent at a time
	int latestSnapshotNum;          // the number of snapshots the client system has received
	int latestSnapshotTime;         // the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t *snap;               // cg.snap->serverTime <= cg.time
	snapshot_t *nextSnap;           // cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t activeSnapshots[2];

	float frameInterpolation;       // (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean thisFrameTeleport;
	qboolean nextFrameTeleport;

	int frametime;              // cg.time - cg.oldTime

	int time;                   // this is the time value that the client
	                            // is rendering at.
	int oldTime;                // time at last frame, used for missile trails and prediction checking

	int physicsTime;            // either cg.snap->time or cg.nextSnap->time

	qboolean renderingThirdPerson;          // during deaths, chasecams, etc

	// prediction state
	qboolean hyperspace;                // true if prediction has hit a trigger_teleport
	playerState_t predictedPlayerState;
	centity_t predictedPlayerEntity;
	qboolean validPPS;                  // clear until the first call to CG_PredictPlayerState
	int predictedErrorTime;
	vec3_t predictedError;

	int eventSequence;
	int predictableEvents[MAX_PREDICTED_EVENTS];

	float stepChange;                   // for stair up smoothing
	int stepTime;

	float duckChange;                   // for duck viewheight smoothing
	int duckTime;

	float landChange;                   // for landing hard
	int landTime;

	// input state sent to server
	int weaponSelect;

	// auto rotating items
	vec3_t autoAnglesSlow;
	vec3_t autoAxisSlow[3];
	vec3_t autoAngles;
	vec3_t autoAxis[3];
	vec3_t autoAnglesFast;
	vec3_t autoAxisFast[3];

	// view rendering
	refdef_t refdef;
	vec3_t refdefViewAngles;            // will be converted to refdef.viewaxis

	// zoom key
	qboolean zoomed;
	qboolean zoomedBinoc;
	int zoomedScope;            //----(SA)	changed to int
	int zoomTime;
	float zoomSensitivity;
	float zoomval;

	// information screen text during loading
	char infoScreenText[MAX_STRING_CHARS];

	// scoreboard
	int scoresRequestTime;
	int numScores;
	int selectedScore;

	score_t scores[MAX_CLIENTS];
	qboolean showScores;
	qboolean scoreBoardShowing;
	int scoreFadeTime;

	qboolean lightstylesInited;

	// centerprinting
	int centerPrintTime;
	int centerPrintCharWidth;
	int centerPrintY;
	char centerPrint[1024];
	int centerPrintLines;
	int centerPrintPriority;                    // NERVE - SMF

	// fade in/out
	int fadeTime;
	float fadeRate;
	vec4_t fadeColor1;
	vec4_t fadeColor2;

	// crosshair client ID
	int crosshairClientNum;
	int crosshairClientTime;

	qboolean crosshairNotLookingAtClient;

	qboolean filtercams;

	int identifyClientRequest;              // NERVE - SMF

//----(SA)	added
	// cursorhints
	int cursorHintIcon;
	int cursorHintTime;
	int cursorHintFade;
	int cursorHintValue;
//----(SA)	end

	// attacking player
	int attackerTime;
	int voiceTime;
	//==========================

	int weaponSelectTime;

	// blend blobs
	viewDamage_t viewDamage[MAX_VIEWDAMAGE];
	float damageTime;           // last time any kind of damage was recieved

	int grenLastTime;

	int switchbackWeapon;
	int lastFiredWeapon;
	int lastFiredWeaponTime;
	int painTime;
	int weaponFireTime;

	int lastWeapSelInBank[MAX_WEAP_BANKS_MP];           // remember which weapon was last selected in a bank for 'weaponbank' commands //----(SA)	added
// JPW FIXME NOTE: max_weap_banks > max_weap_banks_mp so this should be OK, but if that changes, change this too

	// view movement
	float v_dmg_pitch;
	float v_dmg_roll;

	vec3_t kick_angles;         // weapon kicks
	vec3_t kick_origin;

	// RF, view flames when getting burnt
	int v_noFireTime;

	// temp working variables for player view
	float bobfracsin;
	int bobcycle;
	float lastvalidBobfracsin;
	int lastvalidBobcycle;
	float xyspeed;

	// development tool
	refEntity_t testModelEntity;
	char testModelName[MAX_QPATH];
	qboolean testGun;

	// RF, new kick angles
	vec3_t kickAVel;            // for damage feedback, weapon recoil, etc
	                            // This is the angular velocity, to give a smooth
	                            // rotational feedback, rather than sudden jerks
	vec3_t kickAngles;          // for damage feedback, weapon recoil, etc
	                            // NOTE: this is not transmitted through MSG.C stream
	                            // since weapon kicks are client-side, and damage feedback
	                            // is rare enough that we can transmit that as an event
	float recoilPitch, recoilPitchAngle;

	// Duffy
	qboolean cameraMode;        // if rendering from a camera
	// Duffy end

	int oidPrintTime;
	int oidPrintCharWidth;
	char oidPrint[1024];
	int oidPrintLines;

	// for voice chat buffer
	int voiceChatTime;
	int voiceChatBufferIn;
	int voiceChatBufferOut;

	int newCrosshairIndex;
	qhandle_t crosshairShaderAlt[NUM_CROSSHAIRS];

	int cameraShakeTime;
	float cameraShakePhase;
	float cameraShakeScale;
	float cameraShakeLength;

	// -NERVE - SMF

	// spawn variables
	qboolean spawning;                  // the CG_Spawn*() functions are valid
	int numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2];         // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	vec2_t mapcoordsMins;
	vec2_t mapcoordsMaxs;
	qboolean mapcoordsValid;

	int numMiscGameModels;

	qboolean showGameView;
	qboolean showFireteamMenu;

	char spawnPoints[MAX_SPAWNPOINTS][MAX_SPAWNDESC];
	vec3_t spawnCoordsUntransformed[MAX_SPAWNPOINTS];
	vec3_t spawnCoords[MAX_SPAWNPOINTS];
	team_t spawnTeams[MAX_SPAWNPOINTS];
	int spawnCount;

	cg_string_t aStringPool[MAX_STRINGS];
	int demohelpWindow;
	cg_window_t *motdWindow;

	refdef_t *refdef_current;                   // Handling of some drawing elements for MV

	int spechelpWindow;
	cg_window_t *windowCurrent;                 // Current window to update.. a bit of a hack :p
	cg_windowHandler_t winHandler;
	vec4_t xhairColor;
	vec4_t xhairColorAlt;

	pmoveExt_t pmext;

	int ltChargeTime[2];
	int soldierChargeTime[2];
	int engineerChargeTime[2];
	int medicChargeTime[2];
	int covertopsChargeTime[2];
	int binocZoomTime;

	int proneMovingTime;
	fireteamData_t fireTeams[32];

	centity_t *satchelCharge;

	qboolean skyboxEnabled;
	vec3_t skyboxViewOrg;
	vec_t skyboxViewFov;

	vec3_t tankflashorg;

	qboolean editingSpeakers;

	qboolean serverRespawning;

	soundScript_t *bufferSoundScripts[MAX_BUFFERED_SOUNDSCRIPTS];
	int bufferedSoundScriptEndTime;
	int numbufferedSoundScripts;

	int waterundertime;

	// Nico, beginning of ETrun client variables

	// Checkpoints related vars
	int timerunCheckPointChecked;
	int timerunCheckPointDiff[MAX_TIMERUN_CHECKPOINTS];
	int timerunCheckPointTime[MAX_TIMERUN_CHECKPOINTS];
	char timerunCheckStatus[MAX_TIMERUN_CHECKPOINTS];           // Nico, note: this is 0 or 1

	// Timer related vars
	int timerunStartTime;
	int timerunFinishedTime[MAX_CLIENTS];
	int timerunBestTime[MAX_CLIENTS][MAX_TIMERUNS];
	int timerunLastTime[MAX_CLIENTS][MAX_TIMERUNS];

	// Timerun vars
	int timerunActive;
	int currentTimerun;

	// Auto demo
	int runsave;
	char runsavename[MAX_TOKEN_CHARS];
	int currentdemo;
	int startedNewDemo;
	qboolean ignoreNextStart;
	int rs_time;
	int rs_keep;
	int nd_time;
	int nd_keep;

	// Banner printing
	int bannerPrintTime;
	char bannerPrint[1024];

	// Client side login status
	int isLogged;

	// Infos
	int timerunStartSpeed;
	int timerunStopSpeed;
	int runMaxSpeed;
	int overallMaxSpeed;
	int timerunJumpCounter;
	int timerunJumpSpeeds[256];

	// suburb, keycatcher causing CGaz & drawkeys flickering fix
	int keyTimes[KEYS_AMOUNT];
	qboolean keyDown[KEYS_AMOUNT];
	int lastClosedMenuTime;
	qboolean consoleIsUp;
	qboolean UIisUp;
	qboolean limboIsUp;

	// suburb, Velocity Snapping
	float snapZones[128]; // 128 being the max amount of drawn snapzones
	float snapSpeed;
	int snapCount;
	qboolean lastSnapShifted;

	// suburb, Accel HUD
	float oldSpeed;

	// suburb, autodemo UI
	qboolean stoppingAndSavingDemo;
	int lastUnableToSaveDemoTime;
	int lastSavingDemoTime;

	// suburb, draw triggers
	int drawTriggersCount;
	int lastGetTriggerDistancesTime;
	int drawTriggerEntIndexes[MAX_ENTITIES + 1];
	float drawTriggerDistances[MAX_ENTITIES + 1];

	// Nico, end of ETrun client variables
} cg_t;

#define NUM_FUNNEL_SPRITES  21

#define MAX_LOCKER_DEBRIS   5

typedef struct {
	qhandle_t ForwardPressedShader;
	qhandle_t ForwardNotPressedShader;
	qhandle_t BackwardPressedShader;
	qhandle_t BackwardNotPressedShader;
	qhandle_t RightPressedShader;
	qhandle_t RightNotPressedShader;
	qhandle_t LeftPressedShader;
	qhandle_t LeftNotPressedShader;
	qhandle_t JumpPressedShader;
	qhandle_t JumpNotPressedShader;
	qhandle_t CrouchPressedShader;
	qhandle_t CrouchNotPressedShader;
	qhandle_t SprintPressedShader;
	qhandle_t SprintNotPressedShader;
	qhandle_t PronePressedShader;
	qhandle_t ProneNotPressedShader;
} keys_set_t;

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct {
	qhandle_t charsetShader;
	// JOSEPH 4-17-00
	qhandle_t menucharsetShader;
	// END JOSEPH
	qhandle_t charsetProp;
	qhandle_t charsetPropGlow;
	qhandle_t charsetPropB;
	qhandle_t whiteShader;

// JPW NERVE
	qhandle_t hudSprintBar;
// jpw
	qhandle_t teamStatusBar;

	// debris
	qhandle_t debBlock[6];
	qhandle_t debRock[3];
	qhandle_t debFabric[3];
	qhandle_t debWood[6];

	qhandle_t machinegunBrassModel;
	qhandle_t panzerfaustBrassModel;    //----(SA)	added

	// Rafael
	qhandle_t smallgunBrassModel;

	qhandle_t railCoreShader;
	qhandle_t ropeShader;

	qhandle_t spawnInvincibleShader;

	qhandle_t voiceChatShader;
	qhandle_t balloonShader;
	qhandle_t objectiveShader;

	qhandle_t tracerShader;
	qhandle_t crosshairShader[NUM_CROSSHAIRS];
	qhandle_t lagometerShader;
	qhandle_t backTileShader;

	qhandle_t reticleShaderSimple;
	qhandle_t binocShaderSimple;
// JPW NERVE
	qhandle_t nerveTestShader;
	qhandle_t idTestShader;
// jpw
	qhandle_t smokePuffShader;
	qhandle_t smokePuffRageProShader;
	qhandle_t shotgunSmokePuffShader;
	qhandle_t waterBubbleShader;

//----(SA)	cursor hints
	// would be nice to specify these in the menu scripts instead of permanent handles...
	qhandle_t usableHintShader;
	qhandle_t notUsableHintShader;
	qhandle_t doorHintShader;
	qhandle_t doorRotateHintShader;
	qhandle_t doorLockHintShader;
	qhandle_t doorRotateLockHintShader;
	qhandle_t mg42HintShader;
	qhandle_t breakableHintShader;
	qhandle_t chairHintShader;
	qhandle_t alarmHintShader;
	qhandle_t healthHintShader;
	qhandle_t treasureHintShader;
	qhandle_t knifeHintShader;
	qhandle_t ladderHintShader;
	qhandle_t buttonHintShader;
	qhandle_t waterHintShader;
	qhandle_t cautionHintShader;
	qhandle_t dangerHintShader;
	qhandle_t secretHintShader;
	qhandle_t qeustionHintShader;
	qhandle_t exclamationHintShader;
	qhandle_t clipboardHintShader;
	qhandle_t weaponHintShader;
	qhandle_t ammoHintShader;
	qhandle_t armorHintShader;
	qhandle_t powerupHintShader;
	qhandle_t holdableHintShader;
	qhandle_t inventoryHintShader;

	qhandle_t hintPlrFriendShader;
	qhandle_t hintPlrNeutralShader;
	qhandle_t hintPlrEnemyShader;
	qhandle_t hintPlrUnknownShader;

	// DHM - Nerve :: Multiplayer hints
	qhandle_t buildHintShader;
	qhandle_t disarmHintShader;
	qhandle_t dynamiteHintShader;
	// dhm - end

	qhandle_t tankHintShader;
	qhandle_t satchelchargeHintShader;
	qhandle_t uniformHintShader;

	// Rafael
	qhandle_t snowShader;
	qhandle_t oilParticle;
	qhandle_t oilSlick;
	// done.

	// Rafael - cannon
	qhandle_t smokePuffShaderdirty;
	qhandle_t smokePuffShaderb1;
	qhandle_t smokePuffShaderb2;
	qhandle_t smokePuffShaderb3;
	qhandle_t smokePuffShaderb4;
	qhandle_t smokePuffShaderb5;
	// done

	qhandle_t viewFlashFire[16];
	// done

	// Rafael shards
	qhandle_t shardGlass1;
	qhandle_t shardGlass2;
	qhandle_t shardWood1;
	qhandle_t shardWood2;
	qhandle_t shardMetal1;
	qhandle_t shardMetal2;
	// done

	qhandle_t shardRubble1;
	qhandle_t shardRubble2;
	qhandle_t shardRubble3;

	qhandle_t shardJunk[MAX_LOCKER_DEBRIS];

	qhandle_t numberShaders[11];

	qhandle_t shadowFootShader;
	qhandle_t shadowTorsoShader;

	// wall mark shaders
	qhandle_t wakeMarkShader;
	qhandle_t wakeMarkShaderAnim;

	qhandle_t bulletMarkShader;
	qhandle_t bulletMarkShaderMetal;
	qhandle_t bulletMarkShaderWood;
	qhandle_t bulletMarkShaderGlass;
	qhandle_t burnMarkShader;

	qhandle_t flamebarrel;
	qhandle_t mg42muzzleflash;

	qhandle_t waterSplashModel;
	qhandle_t waterSplashShader;

	qhandle_t thirdPersonBinocModel;    //----(SA)	added

	// weapon effect shaders
	qhandle_t rocketExplosionShader;

	// special effects models
	qhandle_t teleportEffectModel;
	qhandle_t teleportEffectShader;

	qhandle_t sparkParticleShader;
	qhandle_t smokeTrailShader;

	qhandle_t flamethrowerFireStream;

	qhandle_t onFireShader, onFireShader2;
	qhandle_t sparkFlareShader;
	qhandle_t spotLightShader;
	qhandle_t spotLightBeamShader;
	qhandle_t smokeParticleShader;

	// DHM - Nerve :: bullet hitting dirt
	qhandle_t dirtParticle1Shader;
	qhandle_t dirtParticle2Shader;
	qhandle_t dirtParticle3Shader;

	qhandle_t genericConstructionShader;

	// sounds
	sfxHandle_t noFireUnderwater;
	sfxHandle_t selectSound;
	sfxHandle_t landHurt;

	sfxHandle_t footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t sfx_rockexp;
	sfxHandle_t sfx_rockexpDist;
	sfxHandle_t sfx_rockexpWater;
	sfxHandle_t sfx_satchelexp;
	sfxHandle_t sfx_satchelexpDist;

	sfxHandle_t sfx_mortarexp[4];
	sfxHandle_t sfx_mortarexpDist;
	sfxHandle_t sfx_grenexp;
	sfxHandle_t sfx_grenexpDist;
	sfxHandle_t sfx_brassSound[BRASSSOUND_MAX][3];
	sfxHandle_t sfx_rubbleBounce[3];

	sfxHandle_t sfx_bullet_metalhit[5];
	sfxHandle_t sfx_bullet_woodhit[5];
	sfxHandle_t sfx_bullet_glasshit[5];
	sfxHandle_t sfx_bullet_stonehit[5];
	sfxHandle_t sfx_bullet_waterhit[5];

	sfxHandle_t sfx_dynamiteexp;
	sfxHandle_t sfx_dynamiteexpDist;
	sfxHandle_t sfx_knifehit[5];

	sfxHandle_t noAmmoSound;
	sfxHandle_t landSound[FOOTSTEP_TOTAL];
	sfxHandle_t gibSound;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;
	sfxHandle_t watrGaspSound;

	sfxHandle_t underWaterSound;
	sfxHandle_t fireSound;
	sfxHandle_t waterSound;

	sfxHandle_t grenadePulseSound4;
	sfxHandle_t grenadePulseSound3;
	sfxHandle_t grenadePulseSound2;
	sfxHandle_t grenadePulseSound1;

	// Ridah
	sfxHandle_t flameSound;
	sfxHandle_t flameBlowSound;
	sfxHandle_t flameStartSound;
	sfxHandle_t flameStreamSound;
	sfxHandle_t flameCrackSound;
	sfxHandle_t boneBounceSound;

	sfxHandle_t grenadebounce[FOOTSTEP_TOTAL][2];

	sfxHandle_t dynamitebounce1;    //----(SA)	added

	sfxHandle_t satchelbounce1;

	qhandle_t cursor;
	qhandle_t selectCursor;
	qhandle_t sizeCursor;

	sfxHandle_t minePrimedSound;
	sfxHandle_t buildDecayedSound;

	sfxHandle_t sndLimboSelect;
	sfxHandle_t sndLimboFocus;
	sfxHandle_t sndLimboCancel;

	qhandle_t hWeaponSnd;
	qhandle_t hWeaponEchoSnd;
	qhandle_t hWeaponHeatSnd;

	qhandle_t hWeaponSnd_2;
	qhandle_t hWeaponEchoSnd_2;
	qhandle_t hWeaponHeatSnd_2;

	qhandle_t hMountedMG42Base;     //	trap_R_RegisterModel( "models/mapobjects/tanks_sd/mg42nestbase.md3" );
	qhandle_t hMountedMG42Nest;     //	trap_R_RegisterModel( "models/mapobjects/tanks_sd/mg42nest.md3" );
	qhandle_t hMountedMG42;         //	trap_R_RegisterModel( "models/mapobjects/tanks_sd/mg42.md3" );
	qhandle_t hMountedBrowning;
	qhandle_t hMountedFPMG42;
	qhandle_t hMountedFPBrowning;

	// Gordon: new limbo stuff
	fontInfo_t limboFont1;
	fontInfo_t limboFont1_lo;
	fontInfo_t limboFont2;
	qhandle_t limboNumber_roll;
	qhandle_t limboNumber_back;
	qhandle_t limboStar_roll;
	qhandle_t limboStar_back;
	qhandle_t limboWeaponNumber_off;
	qhandle_t limboWeaponNumber_on;
	qhandle_t limboWeaponCard;
	qhandle_t limboWeaponCardSurroundH;
	qhandle_t limboWeaponCardSurroundV;
	qhandle_t limboWeaponCardSurroundC;
	qhandle_t limboWeaponCardOOS;
	qhandle_t limboLight_on;
	qhandle_t limboLight_on2;
	qhandle_t limboLight_off;
	qhandle_t limboClassButtons[NUM_PLAYER_CLASSES];
	qhandle_t limboClassButton2Back_on;
	qhandle_t limboClassButton2Back_off;
	qhandle_t limboClassButton2Wedge_on;
	qhandle_t limboClassButton2Wedge_off;
	qhandle_t limboClassButtons2[NUM_PLAYER_CLASSES];
	qhandle_t limboTeamButtonBack_on;
	qhandle_t limboTeamButtonBack_off;
	qhandle_t limboTeamButtonAllies;
	qhandle_t limboTeamButtonAxis;
	qhandle_t limboTeamButtonSpec;

	qhandle_t limboWeaponBlendThingy;

	qhandle_t limboCounterBorder;
	qhandle_t limboWeaponCard1;
	qhandle_t limboWeaponCard2;
	qhandle_t limboWeaponCardArrow;
	qhandle_t limboObjectiveBack[3];
	qhandle_t limboClassBar;

	qhandle_t cursorIcon;

	qhandle_t hudPowerIcon;
	qhandle_t hudHealthIcon;

	qhandle_t pmImages[PM_NUM_TYPES];
	qhandle_t pmImageAlliesConstruct;
	qhandle_t pmImageAxisConstruct;
	qhandle_t pmImageAlliesMine;
	qhandle_t pmImageAxisMine;
	qhandle_t hintKey;

	qhandle_t hudDamagedStates[4];

	qhandle_t browningIcon;

	qhandle_t disconnectIcon;

	qhandle_t fireteamicons[6];

	// Nico, used to show pressed keys
	keys_set_t keys[NUM_KEYS_SETS];

	// Nico, CGaz arrow
	qhandle_t CGazArrow;

	// Nico, ETrun logo
	qhandle_t modLogo;

	// Nico, world flags
	qhandle_t worldFlags;

	// suburb, draw triggers
	qhandle_t checkpointTrigger;
	qhandle_t customTrigger;
	qhandle_t pushTrigger;
	qhandle_t startTrigger;
	qhandle_t stopTrigger;
	qhandle_t teleportTrigger;
	qhandle_t checkpointTriggerEdges;
	qhandle_t customTriggerEdges;
	qhandle_t pushTriggerEdges;
	qhandle_t startTriggerEdges;
	qhandle_t stopTriggerEdges;
	qhandle_t teleportTriggerEdges;
} cgMedia_t;

typedef struct {
	char description[1024];
	char axiswintext[1024];
	char alliedwintext[1024];
	char longname[128];
	vec2_t mappos;
} arenaInfo_t;

typedef struct {
	char campaignDescription[2048];
	char campaignName[128];
	char mapnames[MAX_MAPS_PER_CAMPAIGN][MAX_QPATH];
	vec2_t mappos[MAX_MAPS_PER_CAMPAIGN];
	arenaInfo_t arenas[MAX_MAPS_PER_CAMPAIGN];
	int mapCount;
	int current;
	vec2_t mapTC[2];
} cg_campaignInfo_t;

#define MAX_COMMAND_INFO MAX_CLIENTS

#define MAX_STATIC_GAMEMODELS   1024

typedef struct cg_gamemodel_s {
	qhandle_t model;
	vec3_t org;
	vec3_t axes[3];
	vec_t radius;
} cg_gamemodel_t;

typedef struct oidInfo_s {
	int spawnflags;
	qhandle_t customimageallies;
	qhandle_t customimageaxis;
	int entityNum;
	int objflags;
	char name[MAX_QPATH];
	vec3_t origin;
} oidInfo_t;

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct {
	gameState_t gameState;              // gamestate from server
	glconfig_t glconfig;                // rendering configuration
	float screenXScale;                 // derived from glconfig
	float screenYScale;
	float screenXBias;

	int serverCommandSequence;              // reliable command stream counter
	int processedSnapshotNum;            // the number of snapshots cgame has requested

	qboolean localServer;               // detected on startup by checking sv_running

	// parsed from serverinfo

	int antilag;

	int maxclients;
	char mapname[MAX_QPATH];
	char rawmapname[MAX_QPATH];
	char redTeam[MAX_QPATH];                // A team
	char blueTeam[MAX_QPATH];               // B team
	float weaponRestrictions;

	int voteTime;
	int voteYes;
	int voteNo;
	qboolean voteModified;                  // beep whenever changed
	char voteString[MAX_STRING_TOKENS];

	int teamVoteTime[2];
	int teamVoteYes[2];
	int teamVoteNo[2];
	qboolean teamVoteModified[2];           // beep whenever changed
	char teamVoteString[2][MAX_STRING_TOKENS];

	int levelStartTime;

	//
	// locally derived information from gamestate
	//
	qhandle_t gameModels[MAX_MODELS];
	char gameShaderNames[MAX_CS_SHADERS][MAX_QPATH];
	qhandle_t gameShaders[MAX_CS_SHADERS];
	qhandle_t gameModelSkins[MAX_MODELS];
	bg_character_t *gameCharacters[MAX_CHARACTERS];
	sfxHandle_t gameSounds[MAX_SOUNDS];

	int numInlineModels;
	qhandle_t inlineDrawModel[MAX_MODELS];
	vec3_t inlineModelMidpoints[MAX_MODELS];

	clientInfo_t clientinfo[MAX_CLIENTS];

	// teamchat width is *3 because of embedded color codes
	char teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH * 3 + 1];
	int teamChatMsgTimes[TEAMCHAT_HEIGHT];
	team_t teamChatMsgTeams[TEAMCHAT_HEIGHT];
	int teamChatPos;
	int teamLastChatPos;

	// New notify mechanism for obits
	char notifyMsgs[NOTIFY_HEIGHT][NOTIFY_WIDTH * 3 + 1];
	int notifyMsgTimes[NOTIFY_HEIGHT];
	int notifyPos;
	int notifyLastPos;

	int cursorX;
	int cursorY;
	qboolean eventHandling;
	qboolean mouseCaptured;
	qboolean sizingHud;
	void *capturedItem;
	qhandle_t activeCursor;

	// screen fading
	float fadeAlpha, fadeAlphaCurrent;
	int fadeStartTime;
	int fadeDuration;

	// media
	cgMedia_t media;

	// player/AI model scripting (client repository)
	animScriptData_t animScriptData;

	int currentVoiceClient;

	int minclients;
	gamestate_t gamestate;

	float smokeWindDir; // JPW NERVE for smoke puffs & wind (arty, airstrikes, bullet impacts)

	qboolean campaignInfoLoaded;
	cg_campaignInfo_t campaignData;

	qboolean arenaInfoLoaded;
	arenaInfo_t arenaData;

	centity_t *gameManager;

	int invitationClient;
	int invitationEndTime;

	int applicationClient;
	int applicationEndTime;

	int propositionClient;
	int propositionClient2;
	int propositionEndTime;

	bg_character_t *offscreenCmdr;

	// OSP
	int aviDemoRate;                                    // Demo playback recording
	int cursorUpdate;                                   // Timeout for mouse pointer view

	qboolean fResize;                                   // MV window "resize" status
	qboolean fSelect;                                   // MV window "select" status
	qboolean fKeyPressed[256];                          // Key status to get around console issues
	int timescaleUpdate;                                // Timescale display for demo playback
	int thirdpersonUpdate;

	cg_gamemodel_t miscGameModels[MAX_STATIC_GAMEMODELS];

	int ccSelectedTeam;                     // ( 1 = ALLIES, 0 = AXIS )
	int ccSelectedWeaponNumber;
	int ccSelectedClass;
	int ccSelectedWeapon;
	int ccSelectedWeapon2;

	qboolean dbShowing;

	int ftMenuPos;
	int ftMenuMode;
	int ftMenuModeEx;

	qboolean limboLoadoutSelected;
	qboolean limboLoadoutModified;

	oidInfo_t oidInfo[MAX_OID_TRIGGERS];

	qboolean initing;

	// suburb, widescreen support
	float adr43;
	float r43da;
	float wideXoffset;
} cgs_t;

// Nico, score structure for scoreboard
typedef struct {
	int scoreId;
	int clientNum;
	char name[MAX_QPATH];
	int team;    // 1 = axis, 2 = allies, 3 = spectator
	int timerunBestTime;
	int timerunBestSpeed;
	int timerunStatus;
	int clientLogged;
	int clientCGaz;
	int clientHidden;
	int ping;
	int followedClient;
	char followedClientName[MAX_QPATH];
	int speclocked;
	int countryCode;
} s_timerunScores;

//==============================================================================

extern cgs_t        cgs;
extern cg_t         cg;
extern centity_t    cg_entities[MAX_GENTITIES];
extern weaponInfo_t cg_weapons[MAX_WEAPONS];
extern itemInfo_t   cg_items[MAX_ITEMS];
extern markPoly_t   cg_markPolys[MAX_MARK_POLYS];

extern vmCvar_t cg_centertime;
extern vmCvar_t cg_runpitch;
extern vmCvar_t cg_runroll;
extern vmCvar_t cg_bobup;
extern vmCvar_t cg_bobpitch;
extern vmCvar_t cg_bobroll;
extern vmCvar_t cg_bobyaw;
extern vmCvar_t cg_swingSpeed;
extern vmCvar_t cg_shadows;
extern vmCvar_t cg_draw2D;
extern vmCvar_t cg_drawFPS;
extern vmCvar_t cg_FPSXoffset;
extern vmCvar_t cg_FPSYoffset;
extern vmCvar_t cg_FPSColor;
extern vmCvar_t cg_drawClock;
extern vmCvar_t cg_clockXoffset;
extern vmCvar_t cg_clockYoffset;
extern vmCvar_t cg_clockColor;
extern vmCvar_t cg_drawSnapshot;
extern vmCvar_t cg_snapshotXoffset;
extern vmCvar_t cg_snapshotYoffset;
extern vmCvar_t cg_snapshotColor;
extern vmCvar_t cg_drawCrosshair;
extern vmCvar_t cg_drawCrosshairNames;
extern vmCvar_t cg_drawCrosshairPickups;
extern vmCvar_t cg_useWeapsForZoom;
extern vmCvar_t cg_weaponCycleDelay;            //----(SA)	added
extern vmCvar_t cg_cycleAllWeaps;
extern vmCvar_t cg_crosshairX;
extern vmCvar_t cg_crosshairY;
extern vmCvar_t cg_crosshairSize;
extern vmCvar_t cg_crosshairHealth;
extern vmCvar_t cg_drawStatus;
extern vmCvar_t cg_animSpeed;
extern vmCvar_t cg_debugAnim;
extern vmCvar_t cg_debugPosition;
extern vmCvar_t cg_debugEvents;
extern vmCvar_t cg_drawSpreadScale;
extern vmCvar_t cg_railTrailTime;
extern vmCvar_t cg_errorDecay;
extern vmCvar_t cg_nopredict;
extern vmCvar_t cg_noPlayerAnims;
extern vmCvar_t cg_showmiss;
extern vmCvar_t cg_footsteps;
extern vmCvar_t cg_markTime;
extern vmCvar_t cg_brassTime;
extern vmCvar_t cg_gun_frame;
extern vmCvar_t cg_gun_x;
extern vmCvar_t cg_gun_y;
extern vmCvar_t cg_gun_z;
extern vmCvar_t cg_drawGun;
extern vmCvar_t cg_drawPlayerWeaponIcon;
extern vmCvar_t cg_playerWeaponIconXoffset;
extern vmCvar_t cg_playerWeaponIconYoffset;
extern vmCvar_t cg_cursorHints;
extern vmCvar_t cg_letterbox;           //----(SA)	added
extern vmCvar_t cg_tracerChance;
extern vmCvar_t cg_tracerWidth;
extern vmCvar_t cg_tracerLength;
extern vmCvar_t cg_tracerSpeed;
extern vmCvar_t cg_autoswitch;
extern vmCvar_t cg_ignore;
extern vmCvar_t cg_fov;
extern vmCvar_t cg_zoomDefaultSniper;
extern vmCvar_t cg_zoomStepSniper;
extern vmCvar_t cg_zoomStepBinoc;
extern vmCvar_t cg_thirdPersonRange;
extern vmCvar_t cg_thirdPersonAngle;
extern vmCvar_t cg_thirdPerson;
extern vmCvar_t cg_stereoSeparation;
extern vmCvar_t cg_drawLagometer;
extern vmCvar_t cg_lagometerXoffset;
extern vmCvar_t cg_lagometerYoffset;
extern vmCvar_t cg_drawConnectionIssues;
extern vmCvar_t cg_connectionInterruptedX;
extern vmCvar_t cg_connectionInterruptedY;
extern vmCvar_t cg_connectionInterruptedColor;
extern vmCvar_t cg_drawChat;
extern vmCvar_t cg_chatTime;
extern vmCvar_t cg_chatHeight;
extern vmCvar_t cg_chatX;
extern vmCvar_t cg_chatY;
extern vmCvar_t cg_stats;
extern vmCvar_t cg_coronafardist;
extern vmCvar_t cg_coronas;
extern vmCvar_t cg_buildScript;
extern vmCvar_t cg_paused;
extern vmCvar_t cg_predictItems;
extern vmCvar_t cg_teamChatsOnly;
extern vmCvar_t cg_noVoiceChats;                    // NERVE - SMF
extern vmCvar_t cg_noVoiceText;                     // NERVE - SMF
extern vmCvar_t cg_enableBreath;
extern vmCvar_t cg_autoactivate;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;
extern vmCvar_t etr_noclipSpeed;
extern vmCvar_t cg_timescale;
extern vmCvar_t cg_noTaunt;             // NERVE - SMF
extern vmCvar_t cg_voiceSpriteTime;             // DHM - Nerve
// Rafael - particle switch
extern vmCvar_t cg_wolfparticles;
// done
extern vmCvar_t cg_norender;
extern vmCvar_t cg_skybox;
extern vmCvar_t cg_drawNotifyText;
extern vmCvar_t cg_quickMessageAlt;
extern vmCvar_t cg_drawSpectatorMessage;
extern vmCvar_t cg_spectatorMessageX;
extern vmCvar_t cg_spectatorMessageY;
extern vmCvar_t cg_spectatorMessageColor;
extern vmCvar_t cg_antilag;
extern vmCvar_t developer;

// OSP
extern vmCvar_t authLevel;
extern vmCvar_t cg_autoAction;
extern vmCvar_t cg_autoReload;
extern vmCvar_t cg_crosshairAlpha;
extern vmCvar_t cg_crosshairAlphaAlt;
extern vmCvar_t cg_crosshairColor;
extern vmCvar_t cg_crosshairColorAlt;
extern vmCvar_t cg_crosshairPulse;
extern vmCvar_t cg_drawPlayerStatus;
extern vmCvar_t cg_drawWeaponHeat;
extern vmCvar_t cg_weaponHeatX;
extern vmCvar_t cg_weaponHeatY;
extern vmCvar_t cg_drawPlayerAmmo;
extern vmCvar_t cg_playerAmmoXoffset;
extern vmCvar_t cg_playerAmmoYoffset;
extern vmCvar_t cg_playerAmmoColor;
extern vmCvar_t cg_drawPlayerHealthBar;
extern vmCvar_t cg_playerHealthBarXoffset;
extern vmCvar_t cg_playerHealthBarYoffset;
extern vmCvar_t cg_drawWeaponRechargeBar;
extern vmCvar_t cg_weaponRechargeBarXoffset;
extern vmCvar_t cg_weaponRechargeBarYoffset;
extern vmCvar_t cg_drawWeaponIconFlash;
extern vmCvar_t cg_drawPlayerStats;
extern vmCvar_t cg_playerStatsXoffset;
extern vmCvar_t cg_playerStatsYoffset;
extern vmCvar_t cg_playerStatsColor;
extern vmCvar_t cg_noAmmoAutoSwitch;
extern vmCvar_t cg_printObjectiveInfo;
extern vmCvar_t cg_specHelp;
extern vmCvar_t cg_uinfo;
extern vmCvar_t cg_useScreenshotJPEG;
extern vmCvar_t demo_avifpsF1;
extern vmCvar_t demo_avifpsF2;
extern vmCvar_t demo_avifpsF3;
extern vmCvar_t demo_avifpsF4;
extern vmCvar_t demo_avifpsF5;
extern vmCvar_t demo_drawTimeScale;
extern vmCvar_t demo_infoWindow;

// engine mappings
extern vmCvar_t int_cl_maxpackets;
extern vmCvar_t int_cl_timenudge;

// suburb, yawspeed & pitchspeed
extern vmCvar_t int_cl_yawspeed;
extern vmCvar_t int_cl_pitchspeed;

extern vmCvar_t cg_rconPassword;
extern vmCvar_t cg_refereePassword;
extern vmCvar_t cg_atmosphericEffects;
extern vmCvar_t cg_drawFireteamOverlay;
extern vmCvar_t cg_fireteamOverlayX;
extern vmCvar_t cg_fireteamOverlayY;
extern vmCvar_t cg_drawFireteamInvitaitons;
extern vmCvar_t cg_drawVote;
extern vmCvar_t cg_drawSmallPopupIcons;

// bani - demo recording cvars
extern vmCvar_t cl_demorecording;
extern vmCvar_t cl_demofilename;
extern vmCvar_t cl_demooffset;
extern vmCvar_t cl_waverecording;
extern vmCvar_t cl_wavefilename;
extern vmCvar_t cl_waveoffset;

// Nico, beginning of ETrun client cvars

// Game physics
extern vmCvar_t physics;

// Is level a timerun?
extern vmCvar_t isTimerun;

// Speed meter
extern vmCvar_t etr_drawSpeedMeter;
extern vmCvar_t etr_speedMeterX;
extern vmCvar_t etr_speedMeterY;
extern vmCvar_t etr_speedMeterColor1;
extern vmCvar_t etr_speedMeterColor2;
extern vmCvar_t etr_speedMeterColor3;

// Accel HUD
extern vmCvar_t etr_drawAccel;
extern vmCvar_t etr_accelSmoothness;

// Timer
extern vmCvar_t etr_drawTimer;
extern vmCvar_t etr_timerX;
extern vmCvar_t etr_timerY;
extern vmCvar_t etr_timerColor1;
extern vmCvar_t etr_timerColor2;
extern vmCvar_t etr_timerColor3;

// Check points
extern vmCvar_t etr_drawCheckPoints;
extern vmCvar_t etr_checkPointsX;
extern vmCvar_t etr_checkPointsY;
extern vmCvar_t etr_maxCheckPoints;
extern vmCvar_t etr_checkPointsColor1;
extern vmCvar_t etr_checkPointsColor2;
extern vmCvar_t etr_checkPointsColor3;

// Slick detector
extern vmCvar_t etr_drawSlick;
extern vmCvar_t etr_slickX;
extern vmCvar_t etr_slickY;
extern vmCvar_t etr_slickColor;

// OB detector
extern vmCvar_t etr_drawOB;
extern vmCvar_t etr_OBColor;
extern vmCvar_t etr_OBX;
extern vmCvar_t etr_OBY;

// Hide other players
extern vmCvar_t etr_hideOthers;
extern vmCvar_t etr_hideRange;

// Auth related
extern vmCvar_t etr_authToken;
extern vmCvar_t etr_autoLogin;

// CGaz
extern vmCvar_t etr_drawCGaz;
extern vmCvar_t etr_realCGaz2;
extern vmCvar_t etr_CGazX;
extern vmCvar_t etr_CGazY;
extern vmCvar_t etr_CGaz2Color1;
extern vmCvar_t etr_CGaz2Color2;

// Velocity Snapping
extern vmCvar_t etr_drawVelocitySnapping;
extern vmCvar_t etr_velocitySnappingH;
extern vmCvar_t etr_velocitySnappingY;
extern vmCvar_t etr_velocitySnappingFov;
extern vmCvar_t etr_velocitySnapping1Color1;
extern vmCvar_t etr_velocitySnapping1Color2;
extern vmCvar_t etr_velocitySnapping2Color;

// Load view angles on load
extern vmCvar_t etr_loadViewAngles;

// Load weapon on load
extern vmCvar_t etr_loadWeapon;

// Show pressed keys
extern vmCvar_t etr_drawKeys;
extern vmCvar_t etr_keysX;
extern vmCvar_t etr_keysY;
extern vmCvar_t etr_keysSize;

// Banner prints
extern vmCvar_t etr_drawBannerPrint;
extern vmCvar_t etr_bannerPrintX;
extern vmCvar_t etr_bannerPrintY;

// Automatically load player position when he gets killed (except /kill)
extern vmCvar_t etr_autoLoad;

// View log (ET Console)
extern vmCvar_t etr_viewLog;

// Hide me
extern vmCvar_t etr_hideMe;

// Auto demo
extern vmCvar_t etr_autoDemo;
extern vmCvar_t etr_keepAllDemos;
extern vmCvar_t etr_autoDemoStopDelay;

extern vmCvar_t etr_drawStatusline;
extern vmCvar_t etr_statuslineX;
extern vmCvar_t etr_statuslineY;
extern vmCvar_t etr_statuslineColor;

// Popups
extern vmCvar_t etr_numPopups;
extern vmCvar_t etr_popupTime;
extern vmCvar_t etr_popupStayTime;
extern vmCvar_t etr_popupFadeTime;
extern vmCvar_t etr_popupGrouped;
extern vmCvar_t etr_drawPopups;
extern vmCvar_t etr_popupX;
extern vmCvar_t etr_popupY;

// Automatically load checkpoints
extern vmCvar_t etr_autoLoadCheckpoints;

// Persistant speclock
extern vmCvar_t etr_specLock;

// Info panel
extern vmCvar_t etr_drawInfoPanel;
extern vmCvar_t etr_infoPanelXoffset;
extern vmCvar_t etr_infoPanelYoffset;
extern vmCvar_t etr_infoPanelColor1;
extern vmCvar_t etr_infoPanelColor2;

// Spectator state
extern vmCvar_t etr_drawSpectatorState;
extern vmCvar_t etr_spectatorStateX;
extern vmCvar_t etr_spectatorStateY;
extern vmCvar_t etr_spectatorStateColor;

// Country flags
extern vmCvar_t etr_countryFlags;

// Minimum start speed
extern vmCvar_t etr_minStartSpeed;

// suburb, widescreen support
extern vmCvar_t etr_widescreenSupport;
extern vmCvar_t etr_realFov;

// suburb, event cvars
extern vmCvar_t etr_onRunStart;
extern vmCvar_t etr_onRunStop;

// Draw triggers
extern vmCvar_t etr_drawTriggers;
extern vmCvar_t etr_triggersDrawScale;
extern vmCvar_t etr_triggersDrawEdges;

// Prints
extern vmCvar_t etr_pickupPrints;
extern vmCvar_t etr_autoDemoPrints;

// Nico, end of ETrun cvars

//
// cg_main.c
//
const char *CG_ConfigString(int index);
int CG_ConfigStringCopy(int index, char *buff, int buffsize);
const char *CG_Argv(int arg);

float CG_Cvar_Get(const char *cvar);

char *CG_generateFilename(void);

void CG_LoadObjectiveData(void);
void CG_SetupDlightstyles(void);

void QDECL CG_Printf(const char *msg, ...);
void QDECL CG_Error(const char *msg, ...);

void CG_StartMusic(void);
void CG_QueueMusic(void);

void CG_UpdateCvars(void);

int CG_CrosshairPlayer(void);
int CG_LastAttacker(void);
void CG_KeyEvent(int key, qboolean down);
void CG_MouseEvent(int x, int y);
void CG_EventHandling(int type, qboolean fForced);

qboolean CG_GetTag(int clientNum, char *tagname, orientation_t *or);
qboolean CG_GetWeaponTag(int clientNum, char *tagname, orientation_t *or);

void CG_EncodeQP(const char *in, char *out, int maxlen);
void CG_DecodeQP(char *line);

//
// cg_view.c
//
void CG_TestModel_f(void);
void CG_TestGun_f(void);
void CG_TestModelNextFrame_f(void);
void CG_TestModelPrevFrame_f(void);
void CG_TestModelNextSkin_f(void);
void CG_TestModelPrevSkin_f(void);
void CG_ZoomIn_f(void);
void CG_ZoomOut_f(void);

void CG_SetupFrustum(void);
qboolean CG_CullPoint(vec3_t pt);
qboolean CG_CullPointAndRadius(const vec3_t pt, vec_t radius);

void CG_DrawActiveFrame(int serverTime, stereoFrame_t stereoView, qboolean demoPlayback);
void CG_DrawSkyBoxPortal(qboolean fLocalView);
void CG_Concussive(centity_t *cent);

void CG_Letterbox(float xsize, float ysize, qboolean center);
int CG_CalcViewValues(void);

//
// cg_drawtools.c
//

// suburb, widescreen support
qboolean CG_Is43Screen(void);
float CG_WideX(float x);
float CG_WideXoffset(void);

void CG_AdjustFrom640(float *x, float *y, float *w, float *h);
void CG_FillRect(float x, float y, float width, float height, const float *color);
void CG_DrawPic(float x, float y, float width, float height, qhandle_t hShader);
void CG_DrawPicST(float x, float y, float width, float height, float s0, float t0, float s1, float t1, qhandle_t hShader);
void CG_DrawChar(int x, int y, int width, int height, int ch);
void CG_FilledBar(float x, float y, float w, float h, float *startColor, float *endColor, const float *bgColor, float frac, int flags);
// JOSEPH 10-26-99
void CG_DrawStretchPic(float x, float y, float width, float height, qhandle_t hShader);
// END JOSEPH
void CG_DrawString(float x, float y, const char *string,
                   float charWidth, float charHeight, const float *modulate);

void CG_DrawStringExt(int x, int y, const char *string, float *setColor,
                      qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars);
// JOSEPH 4-17-00
void CG_DrawStringExt2(int x, int y, const char *string, const float *setColor,
                       qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars);
void CG_DrawStringExt_Shadow(int x, int y, const char *string, const float *setColor,
                             qboolean forceColor, int shadow, int charWidth, int charHeight, int maxChars);
// END JOSEPH
void CG_DrawBigString(int x, int y, const char *s, float alpha);
int CG_DrawStrlen(const char *str);

float *CG_FadeColor(int startMsec, int totalMsec);
void CG_TileClear(void);
void CG_ColorForHealth(vec4_t hcolor);

// new hud stuff
void CG_DrawRect(float x, float y, float width, float height, float size, const float *color);
void CG_DrawRect_FixedBorder(float x, float y, float width, float height, int border, const float *color);
void CG_DrawSides(float x, float y, float w, float h, float size);
void CG_DrawTopBottom(float x, float y, float w, float h, float size);
void CG_DrawTopBottom_NoScale(float x, float y, float w, float h, float size);

// suburb, for velocity snapping
void CG_FillAngleYaw(float start, float end, float viewangle, float y, float height, int fov, const float *color);

// NERVE - SMF - localization functions
void CG_InitTranslation(void);
char *CG_TranslateString(const char *string);
void CG_SaveTransTable(void);
void CG_ReloadTranslation(void);

// suburb
float CG_AdjustFontSize(float textScale, int valueToPrint, int border);
void CG_PutPixel(float x, float y);
void CG_DrawLine(float x1, float y1, float x2, float y2, vec4_t color);

//
// cg_draw.c, cg_newDraw.c
//
extern char cg_fxflags;  // JPW NERVE

void CG_AddLagometerFrameInfo(void);
void CG_AddLagometerSnapshotInfo(snapshot_t *snap);
void CG_CenterPrint(const char *str, int y, int charWidth);
void CG_PriorityCenterPrint(const char *str, int y, int charWidth, int priority);       // NERVE - SMF
void CG_ObjectivePrint(const char *str, int charWidth);                         // NERVE - SMF
void CG_DrawActive(stereoFrame_t stereoView);
void CG_CheckForCursorHints(void);
void CG_Text_Paint_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *font);
void CG_Text_Paint_Centred_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust, int limit, int style, fontInfo_t *font);
void CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
void CG_Text_SetActiveFont(int font);
int CG_Text_Width_Ext(const char *text, float scale, int limit, fontInfo_t *font);
int CG_Text_Width(const char *text, float scale, int limit);
int CG_Text_Height_Ext(const char *text, float scale, int limit, fontInfo_t *font);
int CG_Text_Height(const char *text, float scale, int limit);
qboolean CG_OwnerDrawVisible(int flags);
void CG_RunMenuScript(char **args);
void CG_Text_PaintChar_Ext(float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2, float t2, qhandle_t hShader);
void CG_DrawCursorhint(void);
void CG_DrawWeapStability(void);
void CG_DrawWeapHeat(int align);
void CG_DrawPlayerWeaponIcon(int align, vec4_t *refcolor);
void CG_Fade(int a, int time, int duration);
const char *CG_PickupItemText(int item);
void CG_StartShakeCamera(float p);

// Nico
// cg_timerun_draw.c
//
void CG_DrawCheckpoints(void);
void CG_DrawSpeedMeter(void);
void CG_DrawOB(void);
void CG_DrawSlick(void);
void CG_DrawTimer(void);
void CG_DrawCGaz(void);
void CG_DrawVelocitySnapping(void);
void CG_DrawKeys(void);
void CG_DrawScoresClock(float x, float y, float scale);
char *CG_GetClock(void);
void CG_DrawBannerPrint(void);
void CG_DrawDemoRecording(void);
void CG_DrawInfoPanel(void);
void CG_DrawSpectatorState(void);
void CG_UpdateJumpSpeeds(void);
void CG_UpdateKeysAndMenus(void);
void CG_DrawTriggers(void);
void CG_AddShaderToBox(vec3_t mins, vec3_t maxs, qhandle_t boxShader, qhandle_t edgesShader, int addEdges);

//
// cg_players.c
//
qboolean CG_EntOnFire(centity_t *cent);      // Ridah
void CG_Player(centity_t *cent);
void CG_ResetPlayerEntity(centity_t *cent);
void CG_AddRefEntityWithPowerups(refEntity_t *ent, entityState_t *es, const vec3_t fireRiseDir);
void CG_NewClientInfo(int clientNum);
void CG_RunLerpFrame(centity_t *cent, clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale);
sfxHandle_t CG_CustomSound(const char *soundName);

// Rafael particles
extern qboolean initparticles;

//
// cg_predict.c
//
void CG_BuildSolidList(void);
int CG_PointContents(const vec3_t point, int passEntityNum);
void CG_Trace(trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int mask);
void CG_PredictPlayerState(void);

//
// cg_events.c
//
void CG_CheckEvents(centity_t *cent);
void CG_EntityEvent(centity_t *cent, vec3_t position);
void CG_PainEvent(centity_t *cent, int health);
void CG_PrecacheFXSounds(void);

//
// cg_ents.c
//
void CG_SetEntitySoundPosition(centity_t *cent);
void CG_AddPacketEntities(void);
void CG_Beam(centity_t *cent);
void CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out, vec3_t outDeltaAngles);
void CG_AddCEntity(centity_t *cent);
qboolean CG_AddCEntity_Filter(centity_t *cent);
qboolean CG_AddLinkedEntity(centity_t *cent, qboolean ignoreframe, int atTime);
void CG_PositionEntityOnTag(refEntity_t *entity, const refEntity_t *parent, const char *tagName, int startIndex, vec3_t *offset);
void CG_PositionRotatedEntityOnTag(refEntity_t *entity, const refEntity_t *parent, const char *tagName);

//
// cg_weapons.c
//
void CG_RocketTrail(centity_t *ent);
void CG_GetWindVector(vec3_t dir);
void CG_LastWeaponUsed_f(void);       //----(SA)	added
void CG_NextWeaponInBank_f(void);     //----(SA)	added
void CG_PrevWeaponInBank_f(void);     //----(SA)	added
void CG_AltWeapon_f(void);
void CG_NextWeapon_f(void);
void CG_PrevWeapon_f(void);
void CG_Weapon_f(void);
void CG_WeaponBank_f(void);
qboolean CG_WeaponSelectable(int i);
int CG_WeaponIndex(int weapnum, int *bank, int *cycle);

void CG_FinishWeaponChange(int lastweap, int newweap);

void CG_RegisterWeapon(int weaponNum, qboolean force);
void CG_RegisterItemVisuals(int itemNum);

void CG_FireWeapon(centity_t *cent);     //----(SA)	modified.
void CG_AddBulletParticles(vec3_t origin, vec3_t dir, int speed, int count, float randScale);
void CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, int surfaceFlags);     //	(SA) modified to send missilehitwall surface parameters

void CG_MissileHitWallSmall(vec3_t origin, vec3_t dir);
void CG_DrawTracer(vec3_t start, vec3_t finish);

// Rafael
void CG_MG42EFX(centity_t *cent);

void CG_FLAKEFX(centity_t *cent, int whichgun);

void CG_MortarEFX(centity_t *cent);

// Ridah
qboolean CG_MonsterUsingWeapon(centity_t *cent, int aiChar, int weaponNum);

// Rafael
void CG_MissileHitWall2(int weapon, int clientNum, vec3_t origin, vec3_t dir);
// done

void CG_MissileHitPlayer(int weapon, vec3_t origin, vec3_t dir);
void CG_Tracer(vec3_t source, vec3_t dest, int sparks);
void CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle);
void CG_Bullet(vec3_t end, int sourceEntityNum, qboolean flesh, int fleshEntityNum, int otherEntNum2, float waterfraction, int seed);

void CG_RailTrail(vec3_t start, vec3_t end);
void CG_GrappleTrail(centity_t *ent, const weaponInfo_t *wi);
void CG_AddViewWeapon(playerState_t *ps);
void CG_AddPlayerWeapon(refEntity_t *parent, playerState_t *ps, centity_t *cent);

void CG_OutOfAmmoChange(qboolean allowforceswitch);

//----(SA) added to header to access from outside cg_weapons.c
void CG_AddDebris(vec3_t origin, vec3_t dir, int speed, int duration, int count);
//----(SA) done

//
// cg_marks.c
//
void    CG_InitMarkPolys(void);
void    CG_AddMarks(void);
void    CG_ImpactMark(qhandle_t markShader,
                      vec3_t origin, vec4_t projection, float radius, float orientation,
                      float r, float g, float b, float a, int lifeTime);

// Rafael particles
//
// cg_particles.c
//
void    CG_ClearParticles(void);
void    CG_AddParticles(void);
void    CG_ParticleSmoke(qhandle_t pshader, centity_t *cent);
void    CG_ParticleSnowFlurry(qhandle_t pshader, centity_t *cent);
void    CG_ParticleBulletDebris(vec3_t org, vec3_t vel, int duration);
void    CG_ParticleDirtBulletDebris_Core(vec3_t org, vec3_t vel, int duration, float width, float height, float alpha, qhandle_t shader);
void    CG_ParticleSparks(vec3_t org, vec3_t vel, int duration, float x, float y, float speed);

// Ridah
void CG_ParticleExplosion(char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd, qboolean dlight);

// Rafael snow pvs check
void    CG_SnowLink(centity_t *cent, qboolean particleOn);
// done.

void CG_ParticleImpactSmokePuff(qhandle_t pshader, vec3_t origin);
void CG_ParticleImpactSmokePuffExtended(qhandle_t pshader, vec3_t origin, int lifetime, int vel, int acc, int maxroll, float alpha, float size);        // (SA) so I can add more parameters without screwing up the one that's there
void CG_Particle_OilParticle(qhandle_t pshader, vec3_t origin, vec3_t origin2, int ptime, int snum);
void CG_Particle_OilSlick(qhandle_t pshader, centity_t *cent);
void CG_OilSlickRemove(centity_t *cent);
// done

// Ridah, trails
//
// cg_trails.c
//
// rain - usedby for zinx's trail fixes
int CG_AddTrailJunc(int headJuncIndex, void *usedby, qhandle_t shader, int spawnTime, int sType, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth, int flags, vec3_t colorStart, vec3_t colorEnd, float sRatio, float animSpeed);
int CG_AddSparkJunc(int headJuncIndex, void *usedby, qhandle_t shader, vec3_t pos, int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth);
int CG_AddSmokeJunc(int headJuncIndex, void *usedby, qhandle_t shader, vec3_t pos, int trailLife, float alpha, float startWidth, float endWidth);
void CG_AddTrails(void);
void CG_ClearTrails(void);
// done.

//
// cg_sound.c
//

// Ridah, sound scripting
int CG_SoundScriptPrecache(const char *name);
int CG_SoundPlaySoundScript(const char *name, vec3_t org, int entnum, qboolean buffer);
void CG_UpdateBufferedSoundScripts(void);
// TTimo: prototype must match animScriptData_t::playSound
void CG_SoundPlayIndexedScript(int index, vec3_t org, int entnum);
void CG_SoundInit(void);
// done.
void CG_SetViewanglesForSpeakerEditor(void);
void CG_SpeakerEditorDraw(void);
void CG_SpeakerEditor_KeyHandling(int key, qboolean down);

void CG_SpeakerEditorMouseMove_Handling(int x, int y);
void CG_ActivateEditSoundMode(void);
void CG_DeActivateEditSoundMode(void);
void CG_ModifyEditSpeaker(void);
void CG_UndoEditSpeaker(void);
void CG_ToggleActiveOnScriptSpeaker(int index);
void CG_UnsetActiveOnScriptSpeaker(int index);
void CG_SetActiveOnScriptSpeaker(int index);
void CG_AddScriptSpeakers(void);

// Ridah, flamethrower
void CG_FireFlameChunks(centity_t *cent, vec3_t origin, vec3_t angles, float speedScale, qboolean firing);
void CG_InitFlameChunks(void);
void CG_AddFlameChunks(void);
void CG_UpdateFlamethrowerSounds(void);
// done.

//
// cg_localents.c
//
void    CG_InitLocalEntities(void);
localEntity_t *CG_AllocLocalEntity(void);
void    CG_AddLocalEntities(void);

//
// cg_effects.c
//
int CG_GetOriginForTag(refEntity_t *parent, char *tagName, int startIndex, vec3_t org, vec3_t axis[3]);
localEntity_t *CG_SmokePuff(const vec3_t p,
                            const vec3_t vel,
                            float radius,
                            float r, float g, float b, float a,
                            float duration,
                            int startTime,
                            int fadeInTime,
                            int leFlags,
                            qhandle_t hShader);

void CG_BubbleTrail(vec3_t start, vec3_t end, float size, float spacing);

// Ridah
void CG_ClearFlameChunks(void);
// done.

//----(SA)
void CG_Spotlight(centity_t *cent, float *color, vec3_t start, vec3_t dir, int segs, float range, int startWidth, float coneAngle, int flags);
#define SL_NOTRACE          0x001   // don't do a trace check for shortening the beam, always draw at full 'range' length
#define SL_NODLIGHT         0x002   // don't put a dlight at the end
#define SL_NOSTARTCAP       0x004   // dont' cap the start circle
#define SL_LOCKTRACETORANGE 0x010   // only trace out as far as the specified range (rather than to max spot range)
#define SL_NOFLARE          0x020   // don't draw a flare when the light is pointing at the camera
#define SL_NOIMPACT         0x040   // don't draw the impact mark
#define SL_LOCKUV           0x080   // lock the texture coordinates at the 'true' length of the requested beam.
#define SL_NOCORE           0x100   // don't draw the center 'core' beam
#define SL_TRACEWORLDONLY   0x200
//----(SA)	done

void CG_RumbleEfx(float pitch, float yaw);

void InitSmokeSprites(void);
void CG_RenderSmokeGrenadeSmoke(centity_t *cent, const weaponInfo_t *weapon);
void CG_AddSmokeSprites(void);

//
// cg_snapshot.c
//
void CG_ProcessSnapshots(void);

//
// cg_spawn.c
//
qboolean    CG_SpawnString(const char *key, const char *defaultString, char **out);
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean    CG_SpawnFloat(const char *key, const char *defaultString, float *out);
qboolean    CG_SpawnInt(const char *key, const char *defaultString, int *out);
qboolean    CG_SpawnVector(const char *key, const char *defaultString, float *out);
void        CG_ParseEntitiesFromString(void);

//
// cg_info.c
//
void CG_LoadingString(const char *s);
void CG_DrawInformation(qboolean forcerefresh);
void CG_DemoClick(int key, qboolean down);
void CG_ShowHelp_Off(int *status);
void CG_ShowHelp_On(int *status);
void CG_DrawOverlays(void);

//
// cg_scoreboard.c
//
qboolean CG_DrawScoreboard(void);

typedef struct {
	lerpFrame_t legs;
	lerpFrame_t torso;

	vec3_t headOrigin;          // used for centering talking heads

	vec3_t viewAngles;
	vec3_t moveAngles;

	int numPendingAnimations;

	float y, z;

	int teamNum;
	int classNum;
} playerInfo_t;

typedef enum {
	ANIM_IDLE,
	ANIM_RAISE,
} animType_t;

void CG_ParseFireteams(void);
void CG_ParseOIDInfos(void);

//
// cg_consolecmds.c
//
extern const char *aMonths[12];
qboolean CG_ConsoleCommand(void);
void CG_InitConsoleCommands(void);
void CG_ScoresDown_f(void);
void CG_ScoresUp_f(void);
void CG_autoRecord_f(void);
void CG_autoScreenShot_f(void);
void CG_keyOn_f(void);
void CG_keyOff_f(void);

//
// cg_servercmds.c
//
void CG_ExecuteNewServerCommands(int latestSequence);
void CG_ParseServerinfo(void);
void CG_ParseWolfinfo(void);            // NERVE - SMF
void CG_ParseSpawns(void);
void CG_SetConfigValues(void);
void CG_ShaderStateChanged(void);
void CG_ChargeTimesChanged(void);
void CG_LoadVoiceChats();               // NERVE - SMF
void CG_PlayBufferedVoiceChats();       // NERVE - SMF
void CG_AddToNotify(const char *str);
void CG_BannerPrint(const char *str);
const char *CG_LocalizeServerCommand(const char *buf);
//
// cg_playerstate.c
//
void CG_Respawn();
void CG_TransitionPlayerState(playerState_t *ps, playerState_t *ops);

//
// cg_atmospheric.c
//
void CG_EffectParse(const char *effectstr);
void CG_AddAtmosphericEffects();

//===============================================

//
// system traps
// These functions are how the cgame communicates with the main game system
//

void trap_PumpEventLoop(void);

// print message on the local console
void        trap_Print(const char *fmt);

// abort the game
void        trap_Error(const char *fmt);

// milliseconds should only be used for performance tuning, never
// for anything game related.  Get time from the CG_DrawActiveFrame parameter
int         trap_Milliseconds(void);
int         trap_RealTime(qtime_t *qtime);

// console variable interaction
void        trap_Cvar_Register(vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags);
void        trap_Cvar_Update(vmCvar_t *vmCvar);
void        trap_Cvar_Set(const char *var_name, const char *value);
void        trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);

// ServerCommand and ConsoleCommand parameter access
int         trap_Argc(void);
void        trap_Argv(int n, char *buffer, int bufferLength);
void        trap_Args(char *buffer, int bufferLength);

// filesystem access
// returns length of file
int         trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void        trap_FS_Read(void *buffer, int len, fileHandle_t f);
void        trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void        trap_FS_FCloseFile(fileHandle_t f);
int         trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
int         trap_FS_Delete(const char *filename);

// add commands to the local console as if they were typed in
// for map changing, etc.  The command is not executed immediately,
// but will be executed in order the next time console commands
// are processed
void        trap_SendConsoleCommand(const char *text);

// register a command name so the console can perform command completion.
// FIXME: replace this with a normal console command "defineCommand"?
void        trap_AddCommand(const char *cmdName);

// send a string to the server over the network
void        trap_SendClientCommand(const char *s);

// force a screen update, only used during gamestate load
void        trap_UpdateScreen(void);

// model collision
void        trap_CM_LoadMap(const char *mapname);
int         trap_CM_NumInlineModels(void);
clipHandle_t trap_CM_InlineModel(int index);        // 0 = world, 1+ = bmodels
clipHandle_t trap_CM_TempBoxModel(const vec3_t mins, const vec3_t maxs);
int         trap_CM_PointContents(const vec3_t p, clipHandle_t model);
int         trap_CM_TransformedPointContents(const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles);
void        trap_CM_BoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
                             const vec3_t mins, const vec3_t maxs,
                             clipHandle_t model, int brushmask);
void        trap_CM_TransformedBoxTrace(trace_t *results, const vec3_t start, const vec3_t end,
                                        const vec3_t mins, const vec3_t maxs,
                                        clipHandle_t model, int brushmask,
                                        const vec3_t origin, const vec3_t angles);

void        trap_CM_CapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
                                 const vec3_t mins, const vec3_t maxs,
                                 clipHandle_t model, int brushmask);
void        trap_CM_TransformedCapsuleTrace(trace_t *results, const vec3_t start, const vec3_t end,
                                            const vec3_t mins, const vec3_t maxs,
                                            clipHandle_t model, int brushmask,
                                            const vec3_t origin, const vec3_t angles);

// ydnar: projects a decal onto brush model surfaces
void        trap_R_ProjectDecal(qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime);

// normal sounds will have their volume dynamically changed as their entity
// moves and the listener moves
void        trap_S_StartSound(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx);
void        trap_S_StartSoundVControl(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int volume);
void        trap_S_StartSoundEx(vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags);
void        trap_S_StopStreamingSound(int entnum);    // usually AI.  character is talking and needs to be shut up /now/
int         trap_S_GetSoundLength(sfxHandle_t sfx);
int         trap_S_GetCurrentSoundTime(void);   // ydnar

// a local sound is always played full volume
void        trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum);
void        trap_S_ClearLoopingSounds(void);
void        trap_S_ClearSounds(qboolean killmusic);
void        trap_S_AddLoopingSound(const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int volume, int soundTime);
void        trap_S_AddRealLoopingSound(const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range, int volume, int soundTime);
void        trap_S_UpdateEntityPosition(int entityNum, const vec3_t origin);

// repatialize recalculates the volumes of sound as they should be heard by the
// given entityNum and position
void trap_S_Respatialize(int entityNum, const vec3_t origin, vec3_t axis[3], int inwater);
sfxHandle_t trap_S_RegisterSound(const char *sample);          // returns buzz if not found
void        trap_S_StartBackgroundTrack(const char *intro, const char *loop, int fadeupTime);   // empty name stops music
void        trap_S_FadeBackgroundTrack(float targetvol, int time, int num);
void        trap_S_StopBackgroundTrack(void);
int         trap_S_StartStreamingSound(const char *intro, const char *loop, int entnum, int channel, int attenuation);
void        trap_S_FadeAllSound(float targetvol, int time, qboolean stopsounds);

void        trap_R_LoadWorldMap(const char *mapname);

// all media should be registered during level startup to prevent
// hitches during gameplay
qhandle_t   trap_R_RegisterModel(const char *name);             // returns rgb axis if not found
qhandle_t   trap_R_RegisterSkin(const char *name);              // returns all white if not found
qhandle_t   trap_R_RegisterShader(const char *name);            // returns all white if not found
qhandle_t   trap_R_RegisterShaderNoMip(const char *name);           // returns all white if not found

qboolean    trap_R_GetSkinModel(qhandle_t skinid, const char *type, char *name);            //----(SA) added
qhandle_t   trap_R_GetShaderFromModel(qhandle_t modelid, int surfnum, int withlightmap);    //----(SA)	added

// a scene is built up by calls to R_ClearScene and the various R_Add functions.
// Nothing is drawn until R_RenderScene is called.
void        trap_R_ClearScene(void);
void        trap_R_AddRefEntityToScene(const refEntity_t *re);

// polys are intended for simple wall marks, not really for doing
// significant construction
void        trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts);
void        trap_R_AddPolyBufferToScene(polyBuffer_t *pPolyBuffer);
// Ridah
void        trap_R_AddPolysToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys);
// done.
void        trap_R_AddLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags);
void        trap_R_AddCoronaToScene(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible);
void        trap_R_RenderScene(const refdef_t *fd);
void        trap_R_SetColor(const float *rgba);     // NULL = 1,1,1,1
void        trap_R_DrawStretchPic(float x, float y, float w, float h,
                                  float s1, float t1, float s2, float t2, qhandle_t hShader);
void        trap_R_Add2dPolys(polyVert_t *verts, int numverts, qhandle_t hShader);
void        trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs);
int         trap_R_LerpTag(orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex);
void        trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset);

// Set fog
void    trap_R_SetFog(int fogvar, int var1, int var2, float r, float g, float b, float density);
void    trap_R_SetGlobalFog(qboolean restore, int duration, float r, float g, float b, float depthForOpaque);

// The glconfig_t will not change during the life of a cgame.
// If it needs to change, the entire cgame will be restarted, because
// all the qhandle_t are then invalid.
void        trap_GetGlconfig(glconfig_t *glconfig);

// the gamestate should be grabbed at startup, and whenever a
// configstring changes
void        trap_GetGameState(gameState_t *gamestate);

// cgame will poll each frame to see if a newer snapshot has arrived
// that it is interested in.  The time is returned seperately so that
// snapshot latency can be calculated.
void        trap_GetCurrentSnapshotNumber(int *snapshotNumber, int *serverTime);

// a snapshot get can fail if the snapshot (or the entties it holds) is so
// old that it has fallen out of the client system queue
qboolean    trap_GetSnapshot(int snapshotNumber, snapshot_t *snapshot);

// retrieve a text command from the server stream
// the current snapshot will hold the number of the most recent command
// qfalse can be returned if the client system handled the command
// argc() / argv() can be used to examine the parameters of the command
qboolean    trap_GetServerCommand(int serverCommandNumber);

// returns the most recent command number that can be passed to GetUserCmd
// this will always be at least one higher than the number in the current
// snapshot, and it may be quite a few higher if it is a fast computer on
// a lagged connection
int         trap_GetCurrentCmdNumber(void);

qboolean    trap_GetUserCmd(int cmdNumber, usercmd_t *ucmd);

// used for the weapon/holdable select and zoom
void        trap_SetUserCmdValue(int stateValue, int flags, float sensitivityScale, int mpIdentClient);
void        trap_SetClientLerpOrigin(float x, float y, float z);        // DHM - Nerve
int         trap_MemoryRemaining(void);
void        trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font);
qboolean    trap_Key_IsDown(int keynum);
int         trap_Key_GetCatcher(void);
void        trap_Key_SetCatcher(int catcher);
void        trap_Key_KeysForBinding(const char *binding, int *key1, int *key2);
qboolean    trap_Key_GetOverstrikeMode(void);
void        trap_Key_SetOverstrikeMode(qboolean state);

// RF
void trap_SendMoveSpeedsToGame(int entnum, char *movespeeds);

void trap_UI_Popup(int arg0);

// NERVE - SMF
qhandle_t getTestShader(void);   // JPW NERVE shhh
void trap_Key_GetBindingBuf(int keynum, char *buf, int buflen);
void trap_Key_SetBinding(int keynum, const char *binding);
void trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen);
// -NERVE - SMF

void trap_TranslateString(const char *string, char *buf);       // NERVE - SMF - localization

void trap_SnapVector(float *v);

qboolean    trap_GetEntityToken(char *buffer, int bufferSize);
qboolean    trap_R_inPVS(const vec3_t p1, const vec3_t p2);
void        trap_GetHunkData(int *hunkused, int *hunkexpected);

// Duffy, camera stuff
#define CAM_PRIMARY 0   // the main camera for cutscenes, etc.
qboolean    trap_loadCamera(int camNum, const char *name);
void        trap_startCamera(int camNum, int time);
void        trap_stopCamera(int camNum);
qboolean    trap_getCameraInfo(int camNum, int time, vec3_t *origin, vec3_t *angles, float *fov);
void        CG_StartCamera(const char *name, qboolean startBlack);
void        CG_StopCamera(void);

//----(SA)	added
int         CG_LoadCamera(const char *name);
//----(SA)	end

void CG_LocateArena(void);
void CG_CloseMenus();
void CG_LimboMenu_f(void);

typedef struct {
	weapon_t weapindex;
	const char *desc;
} weaponType_t;

extern weaponType_t weaponTypes[];
weaponType_t *WM_FindWeaponTypeForWeapon(weapon_t weapon);

// Gordon: Fireteam stuff
#define CG_IsOnFireteam(clientNum) cgs.clientinfo [clientNum].fireteamData
fireteamData_t *CG_IsOnSameFireteam(int clientNum, int clientNum2);
fireteamData_t *CG_IsFireTeamLeader(int clientNum);

void CG_SortClientFireteam();

void CG_DrawFireTeamOverlay(void);
clientInfo_t *CG_SortedFireTeamPlayerForPosition(int pos, int max);
qboolean CG_FireteamHasClass(int classnum, qboolean selectedonly);
const char *CG_BuildSelectedFirteamString(void);

// cg_window.c
void CG_initStrings(void);
void CG_removeStrings(cg_window_t *w);
void CG_windowDraw(void);
void CG_windowFree(cg_window_t *w);
void CG_windowInit(void);
void CG_windowNormalizeOnText(cg_window_t *w);
// OSP

void CG_SetupCabinets(void);

extern displayContextDef_t cgDC;
void CG_ParseSkyBox(void);
void CG_ParseTagConnect(int tagNum);
void CG_ParseTagConnects(void);

//
// cg_ents.c
//

void CG_AttachBitsToTank(centity_t *tank, refEntity_t *mg42base, refEntity_t *mg42upper, refEntity_t *mg42gun, refEntity_t *player, refEntity_t *flash, vec_t *playerangles, const char *tagName, qboolean browning);

//
// cg_character.c
//

qboolean CG_RegisterCharacter(const char *characterFile, bg_character_t *character);
bg_character_t *CG_CharacterForClientinfo(clientInfo_t *ci, centity_t *cent);
void CG_RegisterPlayerClasses(void);

//
// cg_polybus.c
//

polyBuffer_t *CG_PB_FindFreePolyBuffer(qhandle_t shader, int numVerts, int numIndicies);
void CG_PB_ClearPolyBuffers(void);
void CG_PB_RenderPolyBuffers(void);

//
// cg_limbopanel.c
//

void CG_LimboPanel_KeyHandling(int key, qboolean down);

qboolean CG_LimboPanel_WeaponLights_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_WeaponPanel_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_WeaponPanel_KeyUp(panel_button_t *button, int key);

qboolean CG_LimboPanel_TeamButton_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_ClassButton_KeyDown(panel_button_t *button, int key);
qboolean CG_LimboPanel_OkButton_KeyDown(panel_button_t *button, int key);

qboolean CG_LimboPanel_CancelButton_KeyDown(panel_button_t *button, int key);

void CG_LimboPanel_ClassBar_Draw(panel_button_t *button);

void CG_LimboPanel_RenderTeamButton(panel_button_t *button);
void CG_LimboPanel_RenderClassButton(panel_button_t *button);

void CG_LimboPanel_RenderLight(panel_button_t *button);
void CG_LimboPanel_WeaponLights(panel_button_t *button);

void CG_LimboPanel_WeaponPanel(panel_button_t *button);
void CG_LimboPanel_Border_Draw(panel_button_t *button);

void CG_LimboPanel_RenderCounter(panel_button_t *button);
void CG_LimboPanel_NameEditFinish(panel_button_t *button);

void CG_LimboPanel_Setup(void);
void CG_LimboPanel_Init(void);

void                CG_LimboPanel_GetWeaponCardIconData(weapon_t weap, qhandle_t *shader, float *w, float *h, float *s0, float *t0, float *s1, float *t1);

qboolean            CG_LimboPanel_Draw(void);
team_t              CG_LimboPanel_GetTeam(void);
team_t              CG_LimboPanel_GetRealTeam(void);
int                 CG_LimboPanel_GetClass(void);
int                 CG_LimboPanel_WeaponCount(void);
int                 CG_LimboPanel_WeaponCount_ForSlot(int number);
int                 CG_LimboPanel_GetSelectedWeaponNum(void);
void                CG_LimboPanel_SetSelectedWeaponNum(int number);
bg_playerclass_t *CG_LimboPanel_GetPlayerClass(void);
weapon_t            CG_LimboPanel_GetSelectedWeapon(void);
weapon_t            CG_LimboPanel_GetWeaponForNumber(int number, int slot, qboolean ignoreDisabled);
qboolean            CG_LimboPanel_WeaponIsDisabled(int weap);
qboolean            CG_LimboPanel_RealWeaponIsDisabled(weapon_t weap);
void                CG_LimboPanel_SetSelectedWeaponNumForSlot(int index, int number);
weapon_t            CG_LimboPanel_GetSelectedWeaponForSlot(int index);

//
// cg_popupmessages.c
//

void CG_InitPM(void);
void CG_InitPMGraphics(void);
void CG_UpdatePMLists(void);
void CG_AddPMItem(popupMessageType_t type, const char *message, qhandle_t shader);
void CG_DrawPMItems(void);
void CG_DrawPMItemsBig(void);
const char *CG_GetPMItemText(centity_t *cent);
void CG_PlayPMItemSound(centity_t *cent);
qhandle_t CG_GetPMItemIcon(centity_t *cent);

//
// cg_loadpanel.c
//

void CG_LoadPanel_DrawPin(const char *text, float px, float py, float sx, float sy, qhandle_t shader, float pinsize, float backheight);
void CG_LoadPanel_RenderCampaignPins(panel_button_t *button);
void CG_LoadPanel_RenderMissionDescriptionText(panel_button_t *button);

void CG_LoadPanel_RenderPercentageMeter(panel_button_t *button);
void CG_LoadPanel_RenderContinueButton(panel_button_t *button);
void CG_LoadPanel_RenderLoadingBar(panel_button_t *button);
void CG_LoadPanel_KeyHandling(int key, qboolean down);
void CG_DrawConnectScreen(qboolean interactive, qboolean forcerefresh);

//
// cg_fireteams.c
//

void CG_Fireteams_KeyHandling(int key, qboolean down);
qboolean CG_FireteamCheckExecKey(int key, qboolean doaction);
void CG_Fireteams_Draw(void);
void CG_Fireteams_Setup(void);

void CG_Fireteams_MenuText_Draw(panel_button_t *button);
void CG_Fireteams_MenuTitleText_Draw(panel_button_t *button);
