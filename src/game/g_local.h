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

// g_local.h -- local definitions for game module

#include "q_shared.h"
#include "bg_public.h"
#include "g_public.h"
#include <pthread.h>
#include "../../libs/geoip/geoip.h"

//==================================================================

#define EVENT_VALID_MSEC    300

#define MG42_MULTIPLAYER_HEALTH 350             // JPW NERVE

// How long do bodies last?
#define BODY_TIME(t) (10000)

#define MAX_MG42_HEAT           1500.f

// gentity->flags
#define FL_GODMODE              0x00000010
#define FL_TEAMSLAVE            0x00000400  // not the first on the team
#define FL_NO_KNOCKBACK         0x00000800
#define FL_DROPPED_ITEM         0x00001000
#define FL_NO_BOTS              0x00002000  // spawn point not for bot use
#define FL_NO_HUMANS            0x00004000  // spawn point just for bots
#define FL_AI_GRENADE_KICK      0x00008000  // an AI has already decided to kick this grenade
#define FL_TOGGLE               0x00020000  //----(SA)	ent is toggling (doors use this for ex.)
#define FL_KICKACTIVATE         0x00040000  //----(SA)	ent has been activated by a kick (doors use this too for ex.)
#define FL_SOFTACTIVATE         0x00000040  //----(SA)	ent has been activated while 'walking' (doors use this too for ex.)
#define FL_DEFENSE_GUARD        0x00080000  // warzombie defense pose

#define FL_BLANK                0x00100000
#define FL_BLANK2               0x00200000
#define FL_NO_MONSTERSLICK      0x00400000
#define FL_NO_HEADCHECK         0x00800000

#define FL_NODRAW               0x01000000

#define TKFL_MINES              0x00000001
#define TKFL_AIRSTRIKE          0x00000002
#define TKFL_MORTAR             0x00000004

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_POS3,
	MOVER_1TO2,
	MOVER_2TO1,
	// JOSEPH 1-26-00
	MOVER_2TO3,
	MOVER_3TO2,
	// END JOSEPH

	// Rafael
	MOVER_POS1ROTATE,
	MOVER_POS2ROTATE,
	MOVER_1TO2ROTATE,
	MOVER_2TO1ROTATE
} moverState_t;

#define MAX_CONSTRUCT_STAGES 3
#define ALLOW_AXIS_TEAM         1
#define ALLOW_ALLIED_TEAM       2

// Nico, knockback value at panzerfaust
#define KNOCKBACK_VALUE         500

// Nico, map change delay
#define MAP_CHANGE_DELAY        15

//============================================================================

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;
typedef struct g_serverEntity_s g_serverEntity_t;

//====================================================================
//
typedef struct {
	char *actionString;
	qboolean (*actionFunc)(gentity_t *ent, char *params);
	int hash;
} g_script_stack_action_t;
//
typedef struct {
	//
	// set during script parsing
	g_script_stack_action_t *action;                // points to an action to perform
	char *params;
} g_script_stack_item_t;

#define G_MAX_SCRIPT_STACK_ITEMS    196

typedef struct {
	g_script_stack_item_t items[G_MAX_SCRIPT_STACK_ITEMS];
	int numItems;
} g_script_stack_t;
//
typedef struct {
	int eventNum;                           // index in scriptEvents[]
	char *params;                           // trigger targetname, etc
	g_script_stack_t stack;
} g_script_event_t;
//
typedef struct {
	char *eventStr;
	qboolean (*eventMatch)(g_script_event_t *event, char *eventParm);
	int hash;
} g_script_event_define_t;
//
// Script Flags
#define SCFL_GOING_TO_MARKER    0x1
#define SCFL_ANIMATING          0x2
#define SCFL_FIRST_CALL         0x4
//
// Scripting Status (NOTE: this MUST NOT contain any pointer vars)
typedef struct {
	int scriptStackHead, scriptStackChangeTime;
	int scriptEventIndex;       // current event containing stack of actions to perform
	// scripting system variables
	int scriptId;                   // incremented each time the script changes
	int scriptFlags;
	int actionEndTime;              // time to end the current action
	char *animatingParams;          // Gordon: read 8 lines up for why i love this code ;)
} g_script_status_t;
//
#define G_MAX_SCRIPT_ACCUM_BUFFERS 10
//
void G_Script_ScriptEvent(gentity_t *ent, char *eventStr, char *params);
//====================================================================

typedef struct g_constructible_stats_s {
	float chargebarreq;
	int health;
	int weaponclass;
	int duration;
} g_constructible_stats_t;

#define NUM_CONSTRUCTIBLE_CLASSES   3

extern g_constructible_stats_t g_constructible_classes[NUM_CONSTRUCTIBLE_CLASSES];

qboolean G_WeaponIsExplosive(meansOfDeath_t mod);
int G_GetWeaponClassForMOD(meansOfDeath_t mod);

//====================================================================

#define MAX_NETNAME         36
#define MAX_COMMANDER_TEAM_SOUNDS 16

typedef struct commanderTeamChat_s {
	int index;
} commanderTeamChat_t;

struct gentity_s {
	entityState_t s;                // communicated by server to clients
	entityShared_t r;               // shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s *client;               // NULL if not a client

	qboolean inuse;

	vec3_t instantVelocity;         // ydnar: per entity instantaneous velocity, set per frame

	char *classname;                // set in QuakeEd
	int spawnflags;                 // set in QuakeEd

	qboolean neverFree;             // if true, FreeEntity will only unlink
	                                // bodyque uses this

	int flags;                      // FL_* variables

	char *model;
	char *model2;
	int freetime;                   // level.time when the object was freed

	int eventTime;                  // events will be cleared EVENT_VALID_MSEC after set
	qboolean freeAfterEvent;
	qboolean unlinkAfterEvent;

	qboolean physicsObject;         // if true, it can be pushed by movers and fall off edges
	                                // all game items are physicsObjects,
	float physicsBounce;            // 1.0 = continuous bounce, 0.0 = no bounce
	int clipmask;                   // brushes with this content value will be collided against
	                                // when moving.  items and corpses do not collide against
	                                // players, for instance

	int realClipmask;               // Arnout: use these to backup the contents value when we go to state under construction
	int realContents;
	qboolean realNonSolidBModel;    // For script_movers with spawnflags 2 set

	// movers
	moverState_t moverState;
	int soundPos1;
	int sound1to2;
	int sound2to1;
	int soundPos2;
	int soundLoop;
	int sound2to3;
	int sound3to2;
	int soundPos3;

	int soundSoftopen;
	int soundSoftendo;
	int soundSoftclose;
	int soundSoftendc;

	gentity_t *parent;
	gentity_t *nextTrain;
	gentity_t *prevTrain;
	vec3_t pos1, pos2, pos3;

	char *message;

	int timestamp;              // body queue sinking, etc

	float angle;                // set in editor, -1 = up, -2 = down
	char *target;

	char *targetname;
	int targetnamehash;         // Gordon: adding a hash for this for faster lookups

	char *team;
	gentity_t *target_ent;

	float speed;
	float closespeed;           // for movers that close at a different speed than they open
	vec3_t movedir;

	int gDuration;
	int gDurationBack;
	vec3_t gDelta;
	vec3_t gDeltaBack;

	int nextthink;
	void (*free)(gentity_t *self);
	void (*think)(gentity_t *self);
	void (*reached)(gentity_t *self);           // movers call this when hitting endpoint
	void (*blocked)(gentity_t *self, gentity_t *other);
	void (*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void (*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void (*pain)(gentity_t *self, gentity_t *attacker, int damage, vec3_t point);
	void (*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);

	int pain_debounce_time;
	int fly_sound_debounce_time;            // wind tunnel

	int health;

	qboolean takedamage;

	int damage;
	int splashDamage;           // quad will increase this without increasing radius
	int splashRadius;
	int methodOfDeath;
	int splashMethodOfDeath;

	int count;

	gentity_t *chain;
	gentity_t *enemy;
	gentity_t *activator;
	gentity_t *teamchain;       // next entity in team
	gentity_t *teammaster;      // master of the team

	meansOfDeath_t deathType;

	int watertype;
	int waterlevel;

	int noise_index;

	// timing variables
	float wait;
	float random;

	// Nico, last trigger time for all clients
	int triggerTime[MAX_CLIENTS];

	// Rafael - sniper variable
	// sniper uses delay, random, radius
	int radius;
	float delay;

	// JOSEPH 10-11-99
	int TargetFlag;
	float duration;
	vec3_t rotate;
	vec3_t TargetAngles;
	// END JOSEPH

	gitem_t *item;              // for bonus items

	// Ridah, AI fields
	char *aiName;
	int aiTeam;
	void (*AIScript_AlertEntity)(gentity_t *ent);
	// done.

	char *aiSkin;

	vec3_t dl_color;
	char *dl_stylestring;
	char *dl_shader;
	int dl_atten;

	int key;                    // used by:  target_speaker->nopvs,

	qboolean active;

	// Rafael - mg42
	float harc;
	float varc;

	int props_frame_state;

	int start_size;
	int end_size;

	// Rafael props

	qboolean isProp;

	int mg42BaseEnt;

	gentity_t *melee;

	char *spawnitem;

	int flameQuota, flameQuotaTime, flameBurnEnt;

	int count2;

	int grenadeExplodeTime;         // we've caught a grenade, which was due to explode at this time
	int grenadeFired;               // the grenade entity we last fired

	char *track;

	// entity scripting system
	char *scriptName;

	int numScriptEvents;
	g_script_event_t *scriptEvents;     // contains a list of actions to perform for each event type
	g_script_status_t scriptStatus;     // current status of scripting
	// the accumulation buffer
	int scriptAccumBuffer[G_MAX_SCRIPT_ACCUM_BUFFERS];

	qboolean AASblocking;
	vec3_t AASblocking_mins, AASblocking_maxs;
	float accuracy;

	char tagName[MAX_QPATH];            // name of the tag we are attached to
	gentity_t *tagParent;
	gentity_t *tankLink;

	int lastHintCheckTime;                  // DHM - Nerve
	int voiceChatSquelch;                   // DHM - Nerve
	int voiceChatPreviousTime;              // DHM - Nerve
	int lastBurnedFrameNumber;              // JPW - Nerve   : to fix FT instant-kill exploit

	entState_t entstate;
	char *constages;
	char *desstages;
	char *damageparent;
	int conbmodels[MAX_CONSTRUCT_STAGES + 1];
	int desbmodels[MAX_CONSTRUCT_STAGES];
	int partofstage;

	int allowteams;

	int spawnTime;

	gentity_t *dmgparent;
	qboolean dmginloop;

	int spawnCount;                         // incremented each time this entity is spawned

	int tagNumber;              // Gordon: "handle" to a tag header

	int linkTagTime;

	splinePath_t *backspline;
	vec3_t backorigin;
	float backdelta;
	qboolean back;
	qboolean moving;

	// TAT 10/13/2002 - for seek cover sequence - we need a pointer to a server entity
	//		@ARNOUT - does this screw up the save game?
	g_serverEntity_t *serverEntity;

	// What sort of surface are we standing on?
	int surfaceFlags;

	char tagBuffer[16];

	// bleh - ugly
	int backupWeaponTime;
	int mg42weapHeat;

	vec3_t oldOrigin;

	qboolean runthisframe;

	g_constructible_stats_t constructibleStats;

	//bani
	int etpro_misc_1;

	// Nico, run name used by target_starttimer, target_stoptimer and target_checkpoint
	char *timerunName;

	// suburb, flood protection
	qboolean isBeingDropped;
};

typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW
} spectatorState_t;

typedef enum {
	COMBATSTATE_COLD,
	COMBATSTATE_DAMAGEDEALT,
	COMBATSTATE_DAMAGERECEIVED,
	COMBATSTATE_KILLEDPLAYER
} combatstate_t;

typedef enum {
	TEAM_BEGIN,     // Beginning a team game, spawn at base
	TEAM_ACTIVE     // Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t state;

	int location[2];
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define FOLLOW_ACTIVE1  -1
#define FOLLOW_ACTIVE2  -2

// Nico, position saving structure
#define MAX_SAVED_POSITIONS 6 // per team
typedef struct {
	qboolean valid;
	vec3_t origin;
	vec3_t vangles;
	int weapon;
} save_position_t;

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t sessionTeam;
	int spectatorTime;              // Nico, note: don't remove (refs #43)
	spectatorState_t spectatorState;
	int spectatorClient;            // for chasecam and follow mode
	int playerType;                 // DHM - Nerve :: for GT_WOLF
	int playerWeapon;               // DHM - Nerve :: for GT_WOLF
	int playerWeapon2;              // Gordon: secondary weapon
	int spawnObjectiveIndex;         // JPW NERVE index of objective to spawn nearest to (returned from UI)
	int latchPlayerType;            // DHM - Nerve :: for GT_WOLF not archived
	int latchPlayerWeapon;          // DHM - Nerve :: for GT_WOLF not archived
	int latchPlayerWeapon2;         // Gordon: secondary weapon
	int ignoreClients[MAX_CLIENTS / (sizeof (int) * 8)];
	qboolean muted;

	// OSP
	int referee;
	int spec_team;
	// OSP

	// Nico, client session timerun related
	int timerunLastTime[MAX_TIMERUNS];
	int timerunBestTime[MAX_TIMERUNS];
	int timerunBestCheckpointTimes[MAX_TIMERUNS][MAX_TIMERUN_CHECKPOINTS];
	int timerunCheckpointWereLoaded[MAX_TIMERUNS];

	int timerunBestSpeed[MAX_TIMERUNS];        // Best speed made with timerunBestTime

	qboolean timerunActive;
	int timerunStartTime;           // absolute start time
	int timerunCheckpointTimes[MAX_TIMERUN_CHECKPOINTS];
	int timerunCheckpointsPassed;
	char *currentTimerun;       // current timerun name
	int currentTimerunNum;        // current timerun num

	// Speed values of the current run
	float startSpeed;
	float stopSpeed;
	int maxSpeed;

	// Overall max speed for the session
	int overallMaxSpeed;

	// Nico, end of client session timerun related

	// Nico, flood protection
	int nextReliableTime;
	int numReliableCmds;
	int thresholdTime;

	// Nico, save/load
	save_position_t alliesSaves[MAX_SAVED_POSITIONS];
	save_position_t axisSaves[MAX_SAVED_POSITIONS];

	// Nico, was it a selfkill last time you died?
	qboolean lastDieWasASelfkill;

	// Nico, login status
	qboolean logged;

	// Nico, speclock
	qboolean specLocked;
	int specInvitedClients[MAX_CLIENTS / (sizeof (int) * 8)];
	qboolean freeSpec;

	// Nico, country code (GeoIP)
	unsigned int countryCode;
} clientSession_t;

#define PICKUP_ACTIVATE 0   // pickup items only when using "+activate"
#define PICKUP_TOUCH    1   // pickup items when touched
#define PICKUP_FORCE    2   // pickup the next item when touched (and reset to PICKUP_ACTIVATE when done)

typedef struct ipFilter_s {
	unsigned mask;
	unsigned compare;
} ipFilter_t;

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t connected;
	usercmd_t cmd;                  // we would lose angles if not persistant
	usercmd_t oldcmd;               // previous command processed by pmove()
	qboolean localClient;           // true if "ip" info key is "localhost"
	qboolean predictItemPickup;     // based on cg_predictItems userinfo
	qboolean pmoveFixed;            //
	char netname[MAX_NETNAME];

	int autoActivate;               // based on cg_autoactivate userinfo		(uses the PICKUP_ values above)

	int maxHealth;                  // for handicapping
	int enterTime;                  // level.time the client entered the game
	playerTeamState_t teamState;    // status in teamplay games
	int voteCount;                  // to prevent people from constantly calling votes
	int teamVoteCount;              // to prevent people from constantly calling votes

	qboolean teamInfo;              // send team overlay updates?

	qboolean bAutoReloadAux;            // TTimo - auxiliary storage for pmoveExt_t::bAutoReload, to achieve persistance

	int applicationClient;              // Gordon: this client has requested to join your fireteam
	int applicationEndTime;             // Gordon: you have X seconds to reply or this message will self destruct!

	int invitationClient;               // Gordon: you have been invited to join this client's fireteam
	int invitationEndTime;              // Gordon: quickly now, chop chop!.....

	int propositionClient;              // Gordon: propositionClient2 has requested you invite this client to join the fireteam
	int propositionClient2;             // Gordon:
	int propositionEndTime;             // Gordon: tick, tick, tick....

	int lastSpawnTime;

	// OSP
	unsigned int autoaction;            // End-of-match auto-requests
	unsigned int clientFlags;           // Client settings that need server involvement
	unsigned int clientMaxPackets;      // Client com_maxpacket settings
	unsigned int clientTimeNudge;       // Client cl_timenudge settings
	int cmd_debounce;                   // Dampening of command spam
	unsigned int invite;                // Invitation to a team to join

	int panzerDropTime;                 // Time which a player dropping panzer still "has it" if limiting panzer counts
	int panzerSelectTime;               // *when* a client selected a panzer as spawn weapon
	// OSP

	bg_character_t *character;
	int characterIndex;

	// Nico, IP address
	char ip[IP_MAX_LENGTH];

	// Nico, player max rate
	unsigned int rate;

	// Nico, player snaps
	unsigned int snaps;

	// Nico, max FPS
	int maxFPS;

	// Nico, name changes limit
	int nameChanges;

	// Nico, auth token
	char authToken[MAX_QPATH];

	// Nico, auto login
	qboolean autoLogin;

	// Nico, load view angles on load
	int loadViewAngles;

	// Nico, load weapon on load
	int loadWeapon;

	// Nico, automatically load player position when he gets killed (except /kill)
	int autoLoad;

	// Nico, client draw cgaz setting
	int cgaz;

	// suburb, velocity snapping
	int snapping;

	// Nico, hideme
	int hideme;

	// Nico, client auto demo record setting
	int autoDemo;
	int keepAllDemos;

	// Nico, autoload checkpoints
	int autoLoadCheckpoints;

	// suburb, noclip speed scale
	int noclipSpeed;

	// suburb, prevent pronebug & wallbug
	vec3_t oldPosition;
	int lastBuggingCheck;
	int oldZvelocity;
	qboolean buggedLastFrame;

	// suburb, prevent trigger bug
	int lastLoadedTime;
	qboolean isTouchingTrigger;
	qboolean isTouchingJumppad;

	// suburb, inactivity drop
	vec3_t oldViewangles;

	// suburb, yawspeed
	int yawspeed;

	// suburb, pitchspeed
	int pitchspeed;
} clientPersistant_t;

typedef struct {
	vec3_t mins;
	vec3_t maxs;

	vec3_t origin;

	int time;
} clientMarker_t;

#define MAX_CLIENT_MARKERS 10

#define LT_SPECIAL_PICKUP_MOD   3       // JPW NERVE # of times (minus one for modulo) LT must drop ammo before scoring a point
#define MEDIC_SPECIAL_PICKUP_MOD    4   // JPW NERVE same thing for medic

// Gordon: debris test
typedef struct debrisChunk_s {
	vec3_t origin;
	int model;
	vec3_t velocity;
	char target[32];
	char targetname[32];
} debrisChunk_t;

#define MAX_DEBRISCHUNKS        256
// ===================

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t ps;               // communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t pers;
	clientSession_t sess;

	qboolean noclip;

	int lastCmdTime;                // level.time of last usercmd_t, for EF_CONNECTION
	                                // we can't just use pers.lastCommand.time, because
	                                // of the g_sycronousclients case
	int buttons;
	int oldbuttons;
	int latched_buttons;

	int wbuttons;
	int oldwbuttons;
	int latched_wbuttons;
	vec3_t oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int damage_blood;               // damage taken out of health
	int damage_knockback;           // impact damage
	vec3_t damage_from;             // origin for vector calculation
	qboolean damage_fromWorld;      // if true, don't use the damage_from vector

	// timers

	int inactivityTime;             // kick players when time > this
	qboolean inactivityWarning;     // qtrue if the five seoond warning has been given

	int airOutTime;

	qboolean fireHeld;              // used for hook
	gentity_t *hook;                // grapple hook if out

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int timeResidual;

	float currentAimSpreadScale;

	gentity_t *cameraPortal;                // grapple hook if out
	vec3_t cameraOrigin;

	int dropWeaponTime;         // JPW NERVE last time a weapon was dropped
	int lastBurnTime;         // JPW NERVE last time index for flamethrower burn
	int PCSpecialPickedUpCount;         // JPW NERVE used to count # of times somebody's picked up this LTs ammo (or medic health) (for scoring)
	int saved_persistant[MAX_PERSISTANT];           // DHM - Nerve :: Save ps->persistant here during Limbo

	gentity_t *touchingTOI;     // Arnout: the trigger_objective_info a player is touching this frame

	int lastConstructibleBlockingWarnTime;

	int speedScale;

	int topMarker;
	clientMarker_t clientMarkers[MAX_CLIENT_MARKERS];
	clientMarker_t backupMarker;

	gentity_t *tempHead;        // Gordon: storing a temporary head for bullet head shot detection
	gentity_t *tempLeg;         // Arnout: storing a temporary leg for bullet head shot detection

	int flagParent;

	// the next 2 are used to play the proper animation on the body
	int torsoDeathAnim;
	int legsDeathAnim;

	int lastSpammyCentrePrintTime;
	pmoveExt_t pmext;

	int lastHealTimes[2];
	int lastAmmoTimes[2];

	qboolean wantsscore;
};

typedef struct {
	char modelname[32];
	int model;
} brushmodelInfo_t;

typedef struct limbo_cam_s {
	qboolean hasEnt;
	int targetEnt;
	vec3_t angles;
	vec3_t origin;
	qboolean spawn;
	int info;
} limbo_cam_t;

#define MAX_LIMBO_CAMS 32

// this structure is cleared as each map is entered
#define MAX_SPAWN_VARS          64
#define MAX_SPAWN_VARS_CHARS    2048
#define VOTE_MAXSTRING          256     // Same value as MAX_STRING_TOKENS

#define MAX_SCRIPT_ACCUM_BUFFERS    8

#define MAX_BUFFERED_CONFIGSTRINGS 128

typedef struct voteInfo_s {
	char voteString[MAX_STRING_CHARS];
	int voteTime;                       // level.time vote was called
	int voteYes;
	int voteNo;
	int numVotingClients;               // set by CalculateRanks
	int numVotingTeamClients[2];
	int (*vote_fn)(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
	char vote_value[VOTE_MAXSTRING];        // Desired vote item setting.
	// Nico, used to check if voter switches team
	char voter_team;
	int voter_cn;
	// suburb, used for vote_delay
	int lastVoteTime;
} voteInfo_t;

// Nico, delayed map change
typedef struct delayedMapChange_s {
	char passedVote[VOTE_MAXSTRING];
	int timeChange;
	qboolean pendingChange;
	qboolean disabledWatcher;
} delayedMapChange_t;

typedef struct {
	struct gclient_s *clients;          // [maxclients]

	struct gentity_s *gentities;
	int gentitySize;
	int num_entities;               // current number, <= MAX_GENTITIES

	fileHandle_t logFile; // Nico, note: this is for g_log
	fileHandle_t debugLogFile; // Nico, debug log
	fileHandle_t chatLogFile; // Nico, chat log

	char rawmapname[MAX_QPATH];

	// store latched cvars here that we want to get at often
	int maxclients;

	int framenum;
	int time;                           // in msec
	int previousTime;                   // so movers can back up when blocked
	int frameTime;                      // Gordon: time the frame started, for antilag stuff

	int startTime;                      // level.time the map was started

	int lastTeamLocationTime;           // last time of client team location update

	int numConnectedClients;
	int numPlayingClients;              // connected, non-spectators
	int sortedClients[MAX_CLIENTS];             // sorted by score
	int follow1, follow2;               // clientNums for auto-follow spectators

	voteInfo_t voteInfo;

	// Nico, delayed map change
	delayedMapChange_t delayedMapChange;

	int numTeamClients[2];
	int numVotingTeamClients[2];

	// spawn variables
	qboolean spawning;                  // the G_Spawn*() functions are valid
	int numSpawnVars;
	char *spawnVars[MAX_SPAWN_VARS][2]; // key / value pairs
	int numSpawnVarChars;
	char spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// Nico, note: keep these 2 vars
	vec3_t intermission_origin;         // also used for spectator spawns
	vec3_t intermission_angle;

	vec3_t spawntargets[MAX_MULTI_SPAWNTARGETS];      // coordinates of spawn targets
	int numspawntargets;         // # spawntargets in this map

	// RF, entity scripting
	qboolean useAPImapscript; // Nico, if API has a script for the current maps, this will be set to true
	char *scriptEntity;

	// player/AI model scripting (server repository)
	animScriptData_t animScriptData;

	int numOidTriggers;                 // DHM - Nerve

	int globalAccumBuffer[MAX_SCRIPT_ACCUM_BUFFERS];

	int soldierChargeTime[2];
	int medicChargeTime[2];
	int engineerChargeTime[2];
	int lieutenantChargeTime[2];
	int covertopsChargeTime[2];

	int lastMapEntityUpdate;

	float soldierChargeTimeModifier[2];
	float medicChargeTimeModifier[2];
	float engineerChargeTimeModifier[2];
	float lieutenantChargeTimeModifier[2];
	float covertopsChargeTimeModifier[2];

	brushmodelInfo_t brushModelInfo[128];
	int numBrushModels;
	gentity_t *gameManager;

	// Gordon: for multiplayer fireteams
	fireteamData_t fireTeams[MAX_FIRETEAMS];

	qboolean ccLayers;

	// OSP
	qboolean fLocalHost;

	int server_settings;
	int timeCurrent;                        // Real game clock
	int timeDelta;                          // Offset from internal clock - used to calculate real match time
	// OSP

	qboolean mapcoordsValid, tracemapLoaded;
	vec2_t mapcoordsMins, mapcoordsMaxs;

// Gordon: debris test
	int numDebrisChunks;
	debrisChunk_t debrisChunks[MAX_DEBRISCHUNKS];
// ===================

	qboolean disableTankExit;
	qboolean disableTankEnter;

	int axisAutoSpawn, alliesAutoSpawn;

	limbo_cam_t limboCams[MAX_LIMBO_CAMS];
	int numLimboCams;

	commanderTeamChat_t commanderSounds[2][MAX_COMMANDER_TEAM_SOUNDS];

	qboolean tempTraceIgnoreEnts[MAX_GENTITIES];

	// Nico, timerun level vars
	int numTimeruns;
	char *timerunsNames[MAX_TIMERUNS];
	int numCheckpoints[MAX_TIMERUNS];

	// timerun records
	int timerunRecordsTimes[MAX_TIMERUNS];          // in miliseconds
	char timerunRecordsPlayers[MAX_TIMERUNS][MAX_NETNAME];

	// Nico, note: set to true if there's at least one target_(starttimer|stoptimer|checkpoint)
	qboolean isTimerun;

	int rocketRun;
	// Nico, end of timerun level vars
} level_locals_t;

typedef struct {
	char mapnames[MAX_MAPS_PER_CAMPAIGN][MAX_QPATH];
	int mapCount;
	int current;

	char shortname[256];
	char next[256];
	int typeBits;
} g_campaignInfo_t;

//
// g_spawn.c
//
#define     G_SpawnString(key, def, out) G_SpawnStringExt(key, def, out, __FILE__, __LINE__)
#define     G_SpawnFloat(key, def, out) G_SpawnFloatExt(key, def, out, __FILE__, __LINE__)
#define     G_SpawnInt(key, def, out) G_SpawnIntExt(key, def, out, __FILE__, __LINE__)
#define     G_SpawnVector(key, def, out) G_SpawnVectorExt(key, def, out, __FILE__, __LINE__)
#define     G_SpawnVector2D(key, def, out) G_SpawnVector2DExt(key, def, out, __FILE__, __LINE__)

qboolean    G_SpawnStringExt(const char *key, const char *defaultString, char **out, const char *file, int line);   // spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean    G_SpawnFloatExt(const char *key, const char *defaultString, float *out, const char *file, int line);
qboolean    G_SpawnIntExt(const char *key, const char *defaultString, int *out, const char *file, int line);
qboolean    G_SpawnVectorExt(const char *key, const char *defaultString, float *out, const char *file, int line);
qboolean    G_SpawnVector2DExt(const char *key, const char *defaultString, float *out, const char *file, int line);

void        G_SpawnEntitiesFromString(void);
char *G_NewString(const char *string);
// Ridah
qboolean G_CallSpawn(gentity_t *ent);
// done.
char *G_AddSpawnVarToken(const char *string);
void G_ParseField(const char *key, const char *value, gentity_t *ent);

//
// g_cmds.c
//
void Cmd_Score_f(gentity_t *ent);
char *ConcatArgs(int start);
void StopFollowing(gentity_t *ent);
void G_TeamDataForString(const char *teamstr, int clientNum, team_t *team, spectatorState_t *sState, int *specClient);
qboolean SetTeam(gentity_t *ent, char *s, weapon_t w1, weapon_t w2, qboolean setweapons);
void G_SetClientWeapons(gentity_t *ent, weapon_t w1, weapon_t w2, qboolean updateclient);
void Cmd_FollowCycle_f(gentity_t *ent, int dir);
void Cmd_Kill_f(gentity_t *ent);
int ClientNumbersFromString(char *s, int *plist);
int ClientNumberFromString(gentity_t *to, char *s);
void SanitizeString(char *in, char *out, qboolean fToLower);
void Cmd_Load_f(gentity_t *ent);
void Cmd_Save_f(gentity_t *ent);
void Cmd_SpecLock_f(gentity_t *ent, unsigned int dwCommand, qboolean lock);
void Cmd_SpecInvite_f(gentity_t *ent, unsigned int dwCommand, qboolean invite);
void G_SayTo(gentity_t *ent, gentity_t *other, int mode, int color, const char *name, const char *message, qboolean encoded);
qboolean Cmd_CallVote_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void Cmd_StartCamera_f(gentity_t *ent);
void Cmd_Follow_f(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void Cmd_Say_f(gentity_t *ent, int mode, qboolean arg0, qboolean encoded);
void Cmd_Team_f(gentity_t *ent);
void Cmd_PrivateMessage_f(gentity_t *ent);
void Cmd_Help_f(gentity_t *ent);
void Cmd_Abort_f(gentity_t *ent);
void Cmd_Tutorial_f(gentity_t *ent);
void G_SendScore(gentity_t *client);

//
// g_apicmds.c
//
void Cmd_Login_f(gentity_t *ent);
void Cmd_Logout_f(gentity_t *ent);
void Cmd_Records_f(gentity_t *ent);
void Cmd_LoadCheckpoints_f(gentity_t *ent);
void Cmd_LoadCheckpoints_real(gentity_t *ent, char *userName, int runNum);
void Cmd_Rank_f(gentity_t *ent);

// Nico, flood protection
typedef struct {
	char *cmd;
	qboolean isProtected;
	void (*function)(gentity_t *ent);
	qboolean inHelp;   // Nico, is command shown in /help command
	char *desc;       // Nico, description
	char *usage;       // Nico, usage text
} command_t;

qboolean ClientIsFlooding(gentity_t *ent);
// Nico, end of flood protection

//
// g_items.c
//
void G_RunItem(gentity_t *ent);
void RespawnItem(gentity_t *ent);
void PrecacheItem(gitem_t *it);
gentity_t *Drop_Item(gentity_t *ent, gitem_t *item, float angle, qboolean novelocity);
gentity_t *LaunchItem(gitem_t *item, vec3_t origin, vec3_t velocity, int ownerNum);
void SetRespawn(gentity_t *ent, float delay);
void G_SpawnItem(gentity_t *ent, gitem_t *item);
void FinishSpawningItem(gentity_t *ent);
void Think_Weapon(gentity_t *ent);
int ArmorIndex(gentity_t *ent);
void Fill_Clip(playerState_t *ps, int weapon);
int Add_Ammo(gentity_t *ent, int weapon, int count, qboolean fillClip);
void Touch_Item(gentity_t *ent, gentity_t *other, trace_t *trace);

weapon_t G_GetPrimaryWeaponForClient(gclient_t *client);
void G_DropWeapon(gentity_t *ent, weapon_t weapon);

// Touch_Item_Auto is bound by the rules of autoactivation (if cg_autoactivate is 0, only touch on "activate")
void Touch_Item_Auto(gentity_t *ent, gentity_t *other, trace_t *trace);

void Prop_Break_Sound(gentity_t *ent);
void Spawn_Shard(gentity_t *ent, gentity_t *inflictor, int quantity, int type);

//
// g_utils.c
//
// Ridah
int G_FindConfigstringIndex(const char *name, int start, int max, qboolean create);
// done.
int     G_ModelIndex(char *name);
int     G_SoundIndex(const char *name);
int     G_SkinIndex(const char *name);
int     G_ShaderIndex(char *name);
int     G_CharacterIndex(const char *name);
int     G_StringIndex(const char *string);
qboolean G_AllowTeamsAllowed(gentity_t *ent, gentity_t *activator);
void    G_UseEntity(gentity_t *ent, gentity_t *other, gentity_t *activator);
void    G_TeamCommand(team_t team, char *cmd);
gentity_t *G_Find(gentity_t *from, int fieldofs, const char *match);
gentity_t *G_FindByTargetname(gentity_t *from, const char *match);
gentity_t *G_FindByTarget(gentity_t *from, const char *match);  // Nico, find by target
gentity_t *G_FindByTargetnameFast(gentity_t *from, const char *match, int hash);
gentity_t *G_PickTarget(char *targetname);
void    G_UseTargets(gentity_t *ent, gentity_t *activator);
void    G_SetMovedir(vec3_t angles, vec3_t movedir);

void    G_InitGentity(gentity_t *e);
gentity_t *G_Spawn(void);
gentity_t *G_TempEntity(vec3_t origin, int event);
gentity_t *G_PopupMessage(popupMessageType_t type);
void    G_Sound(gentity_t *ent, int soundIndex);
void    G_AnimScriptSound(int soundIndex, vec3_t org, int client);
void    G_FreeEntity(gentity_t *e);
void    G_TouchTriggers(gentity_t *ent);
void    G_TouchSolids(gentity_t *ent);

float *tv(float x, float y, float z);
char *vtos(const vec3_t v);

void G_AddPredictableEvent(gentity_t *ent, int event, int eventParm);
void G_AddEvent(gentity_t *ent, int event, int eventParm);
void G_SetOrigin(gentity_t *ent, vec3_t origin);
void AddRemap(const char *oldShader, const char *newShader, float timeOffset);
const char *BuildShaderStateConfig();
void G_SetAngle(gentity_t *ent, vec3_t angle);

qboolean infront(gentity_t *self, gentity_t *other);

void G_ProcessTagConnect(gentity_t *ent, qboolean clearAngles);

void G_SetEntState(gentity_t *ent, entState_t state);

team_t G_GetTeamFromEntity(gentity_t *ent);
void my_sleep(unsigned milliseconds);

//
// g_combat.c
//
void G_AdjustedDamageVec(gentity_t *ent, vec3_t origin, vec3_t vec);
qboolean CanDamage(gentity_t *targ, vec3_t origin);
void G_Damage(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod);
qboolean G_RadiusDamage(vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod);
qboolean etpro_RadiusDamage(vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod, qboolean clientsonly);
gentity_t *G_BuildHead(gentity_t *ent);
gentity_t *G_BuildLeg(gentity_t *ent);

// damage flags
#define DAMAGE_RADIUS               0x00000001  // damage was indirect
#define DAMAGE_HALF_KNOCKBACK       0x00000002  // Gordon: do less knockback
#define DAMAGE_NO_KNOCKBACK         0x00000008  // do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION        0x00000020  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_TEAM_PROTECTION   0x00000010  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_DISTANCEFALLOFF      0x00000040  // distance falloff

//
// g_missile.c
//
void G_RunMissile(gentity_t *ent);

// Rafael zombiespit
void G_RunDebris(gentity_t *ent);

//DHM - Nerve :: server side flamethrower collision
void G_RunFlamechunk(gentity_t *ent);

//----(SA) removed unused q3a weapon firing
gentity_t *fire_flamechunk(gentity_t *self, vec3_t start, vec3_t dir);

gentity_t *fire_grenade(gentity_t *self, vec3_t start, vec3_t aimdir, int grenadeWPID);
gentity_t *fire_rocket(gentity_t *self, vec3_t start, vec3_t dir);

#define Fire_Lead(ent, activator, spread, damage, muzzle, forward, right, up) Fire_Lead_Ext(ent, activator, spread, damage, muzzle, forward, right, up, MOD_MACHINEGUN)
void Fire_Lead_Ext(gentity_t *ent, gentity_t *activator, float spread, int damage, vec3_t muzzle, vec3_t forward, vec3_t right, vec3_t up, int mod);
qboolean visible(gentity_t *self, gentity_t *other);

gentity_t *fire_mortar(gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_flamebarrel(gentity_t *self, vec3_t start, vec3_t dir);
// done

//
// g_mover.c
//
gentity_t *G_TestEntityPosition(gentity_t *ent);
void G_RunMover(gentity_t *ent);
qboolean G_MoverPush(gentity_t *pusher, vec3_t move, vec3_t amove, gentity_t **obstacle);
void Use_BinaryMover(gentity_t *ent, gentity_t *other, gentity_t *activator);

void G_TryDoor(gentity_t *ent, gentity_t *activator);   //----(SA)	added

void InitMoverRotate(gentity_t *ent);

void InitMover(gentity_t *ent);
void SetMoverState(gentity_t *ent, moverState_t moverState, int time);

void func_constructible_underconstructionthink(gentity_t *ent);

//
// g_tramcar.c
//
void Reached_Tramcar(gentity_t *ent);

//
// g_trigger.c
//
void InitTrigger(gentity_t *self);
void AimAtTarget(gentity_t *self);
void explosive_indicator_think(gentity_t *ent);
void Think_SetupObjectiveInfo(gentity_t *ent);

//
// g_misc.c
//
void TeleportPlayer(gentity_t *player, vec3_t origin, vec3_t angles);
void mg42_fire(gentity_t *other);
void mg42_stopusing(gentity_t *self);
void aagun_fire(gentity_t *other);

//
// g_weapon.c
//
qboolean AccuracyHit(gentity_t *target, gentity_t *attacker);
void CalcMuzzlePoint(gentity_t *ent, int weapon, vec3_t right, vec3_t up, vec3_t muzzlePoint);
void SnapVectorTowards(vec3_t v, vec3_t to);
gentity_t *weapon_grenadelauncher_fire(gentity_t *ent, int grenadeWPID);

void G_FadeItems(gentity_t *ent, int modType);

qboolean G_ExplodeSatchels(gentity_t *ent);
void G_FreeSatchel(gentity_t *ent);
int G_GetWeaponDamage(int weapon);

void CalcMuzzlePoints(gentity_t *ent, int weapon);
void CalcMuzzlePointForActivate(gentity_t *ent, vec3_t muzzlePoint);

//
// g_client.c
//
team_t TeamCount(int ignoreClientNum, int team);            // NERVE - SMF - merge from team arena
team_t PickTeam(int ignoreClientNum);
void SetClientViewAngle(gentity_t *ent, vec3_t angle);
gentity_t *SelectSpawnPoint(vec3_t avoidPoint, vec3_t origin, vec3_t angles);
void InitClientPersistant(gclient_t *client);
void InitClientResp(gclient_t *client);
void ClientSpawn(gentity_t *ent);
void player_die(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
void CalculateRanks(void);
qboolean SpotWouldTelefrag(gentity_t *spot);
qboolean G_CheckForExistingModelInfo(bg_playerclass_t *classInfo, const char *modelName, animModelInfo_t **modelInfo);
void SetWolfSpawnWeapons(gclient_t *client);
void limbo(gentity_t *ent);
void reinforce(gentity_t *ent);   // JPW NERVE

//
// g_character.c
//

qboolean G_RegisterCharacter(const char *characterFile, bg_character_t *character);
void G_RegisterPlayerClasses(void);
void G_UpdateCharacter(gclient_t *client);

//
// g_svcmds.c
//
qboolean ConsoleCommand(void);
void G_ProcessIPBans(void);
qboolean G_FilterIPBanPacket(char *from);
void G_AddXPBackup(gentity_t *ent);
void G_StoreXPBackup(void);
void G_ClearXPBackup(void);
void G_ReadXPBackup(void);
void AddIPBan(const char *str);

//
// g_weapon.c
//
void FireWeapon(gentity_t *ent);
void G_BurnMeGood(gentity_t *self, gentity_t *body);

//
// g_log.c
//
void G_LogCrash(const char *s, qboolean printIt);
void QDECL G_LogPrintf(qboolean printIt, const char *fmt, ...) _attribute((format(printf, 2, 3)));
void QDECL G_LogDebug(const char *functionName, const char *severity, const char *fmt, ...) _attribute((format(printf, 3, 4)));
void QDECL G_LogChat(const char *type, const char *fmt, ...) _attribute((format(printf, 2, 3)));

//
// g_main.c
//

void FindIntermissionPoint(void); // Nico, note: this function is needed
void G_RunThink(gentity_t *ent);
void QDECL G_Printf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void QDECL G_DPrintf(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void QDECL G_Error(const char *fmt, ...) _attribute((format(printf, 1, 2)));
void G_ShutdownGame(int restart);
qboolean G_enable_delayed_map_change_watcher();
void G_disable_delayed_map_change_watcher();
void G_install_timelimit();
int G_randommap(void);

//
// g_client.c
//
char *ClientConnect(int clientNum, qboolean firstTime);
void ClientUserinfoChanged(int clientNum);
void ClientDisconnect(int clientNum);
void ClientBegin(int clientNum);
void ClientCommand(int clientNum);

//
// g_active.c
//
void ClientThink(int clientNum);
void ClientEndFrame(gentity_t *ent);
void G_RunClient(gentity_t *ent);
qboolean ClientOutOfAmmo(int client);
// to use smoke grenade?
qboolean ReadyToThrowSmoke(gentity_t *ent);
// Are we ready to construct?  Optionally, will also update the time while we are constructing
qboolean ReadyToConstruct(gentity_t *ent, gentity_t *constructible, qboolean updateState);

//
// g_team.c
//
qboolean OnSameTeam(gentity_t *ent1, gentity_t *ent2);

//
// g_mem.c
//
void *G_Alloc(int size);
void G_InitMemory(void);
void Svcmd_GameMem_f(void);

//
// g_session.c
//
void G_ReadSessionData(gclient_t *client);
void G_InitSessionData(gclient_t *client);

void G_InitWorldSession(void);
void G_WriteSessionData(qboolean restart);

//returns the number of the client with the given name
int ClientFromName(char *name);

// g_cmd.c
void Cmd_Activate_f(gentity_t *ent);
void Cmd_Activate2_f(gentity_t *ent);
qboolean Do_Activate_f(gentity_t *ent, gentity_t *traceEnt);
void G_LeaveTank(gentity_t *ent, qboolean position);

// Ridah

// g_script.c
void G_Script_ScriptParse(gentity_t *ent);
qboolean G_Script_ScriptRun(gentity_t *ent);
void G_Script_ScriptEvent(gentity_t *ent, char *eventStr, char *params);
void G_Script_ScriptLoad(void);
void G_Script_EventStringInit(void);

void mountedmg42_fire(gentity_t *other);
void script_mover_use(gentity_t *ent, gentity_t *other, gentity_t *activator);
void script_mover_blocked(gentity_t *ent, gentity_t *other);

// g_props.c
void Props_Chair_Skyboxtouch(gentity_t *ent);

// Nico, g_target.c
int GetTimerunNum(char *name);
void notify_timerun_stop(gentity_t *activator, int finishTime);
void saveDemo(gentity_t *ent);

// Nico, g_crash.c
#if defined _WIN32 && !defined _WIN64
void win32_initialize_handler();
void win32_deinitialize_handler();
#endif

#include "g_team.h" // teamplay specific stuff

// Nico, active threads counter
extern int activeThreadsCounter;

// Nico, global threads
extern pthread_t globalThreads[];

// Nico, threading enabled/disabled
extern qboolean threadingAllowed;

extern level_locals_t   level;
extern gentity_t        g_entities[];   //DAJ was explicit set to MAX_ENTITIES
extern g_campaignInfo_t g_campaigns[];

#define FOFS(x) ((size_t)&(((gentity_t *)0)->x))

extern vmCvar_t g_log;
extern vmCvar_t g_dedicated;
extern vmCvar_t g_maxclients;               // allow this many total, including spectators
extern vmCvar_t g_restarted;
extern vmCvar_t g_password;
extern vmCvar_t sv_privatepassword;
extern vmCvar_t g_inactivity;
extern vmCvar_t g_debugMove;
extern vmCvar_t g_motd;
extern vmCvar_t voteFlags;
extern vmCvar_t g_filtercams;
extern vmCvar_t g_voiceChatsAllowed;        // DHM - Nerve :: number before spam control
extern vmCvar_t g_needpass;
extern vmCvar_t g_banIPs;
extern vmCvar_t g_filterBan;
extern vmCvar_t g_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

//Rafael
extern vmCvar_t g_scriptName;           // name of script file to run (instead of default for that map)

extern vmCvar_t g_scriptDebug;
extern vmCvar_t g_developer;

extern vmCvar_t g_footstepAudibleRange;
// JPW NERVE multiplayer
extern vmCvar_t g_medicChargeTime;
extern vmCvar_t g_engineerChargeTime;
extern vmCvar_t g_LTChargeTime;
extern vmCvar_t g_soldierChargeTime;
// jpw

extern vmCvar_t g_covertopsChargeTime;

// What level of detail do we want script printing to go to.
extern vmCvar_t g_scriptDebugLevel;

extern vmCvar_t g_gamestate;
extern vmCvar_t g_gametype;
// -NERVE - SMF

//Gordon
extern vmCvar_t g_antilag;

// OSP
extern vmCvar_t refereePassword;
extern vmCvar_t g_spectatorInactivity;
extern vmCvar_t server_motd0;
extern vmCvar_t server_motd1;
extern vmCvar_t server_motd2;
extern vmCvar_t server_motd3;
extern vmCvar_t server_motd4;
extern vmCvar_t server_motd5;
extern vmCvar_t team_maxPanzers;
extern vmCvar_t team_maxplayers;
extern vmCvar_t team_nocontrols;
//
// NOTE!!! If any vote flags are added, MAKE SURE to update the voteFlags struct in bg_misc.c w/appropriate info,
//         menudef.h for the mask and g_main.c for vote_allow_* flag updates
//
extern vmCvar_t vote_allow_kick;
extern vmCvar_t vote_allow_map;
extern vmCvar_t vote_allow_randommap;
extern vmCvar_t vote_allow_referee;
extern vmCvar_t vote_allow_antilag;
extern vmCvar_t vote_allow_muting;
extern vmCvar_t vote_limit;
extern vmCvar_t vote_delay;
extern vmCvar_t vote_percent;

// Nico, beginning of ETrun server cvars

// Max connections per IP
extern vmCvar_t g_maxConnsPerIP;

// Game physics
extern vmCvar_t physics;

// Enable certain map entities
extern vmCvar_t g_enableMapEntities;

// Force timer reset, i.e. bypass "wait 9999" on start triggers
extern vmCvar_t g_forceTimerReset;

// Is level a timerun?
extern vmCvar_t isTimerun;

// Flood protection
extern vmCvar_t g_floodProtect;
extern vmCvar_t g_floodThreshold;
extern vmCvar_t g_floodWait;

// Name changes limit
extern vmCvar_t g_maxNameChanges;

// API module
extern vmCvar_t g_useAPI;
extern vmCvar_t g_APImoduleName;

// Hold doors open
extern vmCvar_t g_holdDoorsOpen;

// Disable drowning
extern vmCvar_t g_disableDrowning;

// Mapscript support
extern vmCvar_t g_mapScriptDirectory;

// Cup mode
extern vmCvar_t g_cupMode;
extern vmCvar_t g_cupKey;

// Timelimit mode
extern vmCvar_t g_timelimit;

// Logging
extern vmCvar_t g_debugLog;
extern vmCvar_t g_chatLog;

// GeoIP
extern vmCvar_t g_useGeoIP;
extern vmCvar_t g_geoIPDbPath;

// Strict save/load
extern vmCvar_t g_strictSaveLoad;

// Nico, end of ETrun cvars

void    trap_Printf(const char *fmt);
void    trap_Error(const char *fmt);
int     trap_Milliseconds(void);
int     trap_Argc(void);
void    trap_Argv(int n, char *buffer, int bufferLength);
void    trap_Args(char *buffer, int bufferLength);
int     trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode);
void    trap_FS_Read(void *buffer, int len, fileHandle_t f);
int     trap_FS_Write(const void *buffer, int len, fileHandle_t f);
void    trap_FS_FCloseFile(fileHandle_t f);
int     trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize);
void    trap_SendConsoleCommand(int exec_when, const char *text);
void    trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags);
void    trap_Cvar_Update(vmCvar_t *cvar);
void    trap_Cvar_Set(const char *var_name, const char *value);
int     trap_Cvar_VariableIntegerValue(const char *var_name);
float   trap_Cvar_VariableValue(const char *var_name);
void    trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize);
void    trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *gameClients, int sizeofGameClient);
void    trap_DropClient(int clientNum, const char *reason, int length);
void    trap_SendServerCommand(int clientNum, const char *text);
void    trap_SetConfigstring(int num, const char *string);
void    trap_GetConfigstring(int num, char *buffer, int bufferSize);
void    trap_GetUserinfo(int num, char *buffer, int bufferSize);
void    trap_SetUserinfo(int num, const char *buffer);
void    trap_GetServerinfo(char *buffer, int bufferSize);
void    trap_SetBrushModel(gentity_t *ent, const char *name);
void    trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void    trap_TraceCapsule(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void    trap_TraceCapsuleNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
int     trap_PointContents(const vec3_t point, int passEntityNum);
qboolean trap_InPVS(const vec3_t p1, const vec3_t p2);
void    trap_AdjustAreaPortalState(gentity_t *ent, qboolean open);
void    trap_LinkEntity(gentity_t *ent);
void    trap_UnlinkEntity(gentity_t *ent);
int     trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount);
qboolean trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const gentity_t *ent);
void    trap_GetUsercmd(int clientNum, usercmd_t *cmd);
qboolean    trap_GetEntityToken(char *buffer, int bufferSize);
qboolean trap_GetTag(int clientNum, int tagFileNumber, char *tagName, orientation_t *or);
qboolean trap_LoadTag(const char *filename);
int     trap_RealTime(qtime_t *qtime);
void    trap_SnapVector(float *v);

void G_ExplodeMissile(gentity_t *ent);

// g_antilag.c
void G_StoreClientPosition(gentity_t *ent);
void G_AdjustClientPositions(gentity_t *ent, int time, qboolean forward);
void G_ResetMarkers(gentity_t *ent);
void G_HistoricalTrace(gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);
void G_HistoricalTraceBegin(gentity_t *ent);
void G_HistoricalTraceEnd(gentity_t *ent);
void G_Trace(gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);

#define BODY_VALUE(ENT) ENT->watertype
#define BODY_TEAM(ENT) ENT->s.modelindex
#define BODY_CLASS(ENT) ENT->s.modelindex2
#define BODY_CHARACTER(ENT) ENT->s.onFireStart

#define MAX_FIRE_TEAMS 8

typedef struct {
	char name[32];
	char clientbits[8];
	char requests[8];
	int leader;
	qboolean open;
	qboolean valid;
} fireteam_t;

void Cmd_FireTeam_MP_f(gentity_t *ent);

//g_teammapdata.c

typedef struct mapEntityData_s {
	vec3_t org;
	int yaw;
	int data;
	char type;
	int startTime;
	int singleClient;

	int status;
	int entNum;
	struct mapEntityData_s *next, *prev;
} mapEntityData_t;

typedef struct mapEntityData_Team_s {
	mapEntityData_t mapEntityData_Team[MAX_GENTITIES];
	mapEntityData_t *freeMapEntityData;                 // single linked list
	mapEntityData_t activeMapEntityData;                // double linked list
} mapEntityData_Team_t;

extern mapEntityData_Team_t mapEntityData[2];

void G_InitMapEntityData(mapEntityData_Team_t *teamList);
mapEntityData_t *G_FreeMapEntityData(mapEntityData_Team_t *teamList, mapEntityData_t *mEnt);
mapEntityData_t *G_AllocMapEntityData(mapEntityData_Team_t *teamList);
mapEntityData_t *G_FindMapEntityData(mapEntityData_Team_t *teamList, int entNum);
mapEntityData_t *G_FindMapEntityDataSingleClient(mapEntityData_Team_t *teamList, mapEntityData_t *start, int entNum, int clientNum);

void G_ResetTeamMapData();
void G_UpdateTeamMapData();

void G_SetupFrustum(gentity_t *ent);
void G_SetupFrustum_ForBinoculars(gentity_t *ent);
qboolean G_VisibleFromBinoculars(gentity_t *viewer, gentity_t *ent, vec3_t origin);

typedef enum {
	SM_NEED_MEDIC,
	SM_NEED_ENGINEER,
	SM_NEED_LT,
	SM_NEED_COVERTOPS,
	SM_LOST_MEN,
	SM_OBJ_CAPTURED,
	SM_OBJ_LOST,
	SM_OBJ_DESTROYED,
	SM_CON_COMPLETED,
	SM_CON_FAILED,
	SM_CON_DESTROYED,
	SM_NUM_SYS_MSGS,
} sysMsg_t;

void G_CheckForNeededClasses(void);
void G_SendSystemMessage(sysMsg_t message, int team);
int G_GetSysMessageNumber(const char *sysMsg);
void G_AddClientToFireteam(int entityNum, int leaderNum);
void G_InviteToFireTeam(int entityNum, int otherEntityNum);
void G_UpdateFireteamConfigString(fireteamData_t *ft);
void G_RemoveClientFromFireteams(int entityNum, qboolean update, qboolean print);

void G_PrintClientSpammyCenterPrint(int entityNum, char *text);

void aagun_fire(gentity_t *other);

// TAT 11/13/2002
//		Server only entities

struct g_serverEntity_s {
	qboolean inuse;

	char *classname;                // set in QuakeEd
	int spawnflags;                 // set in QuakeEd

	// our parent entity
	g_serverEntity_t *parent;

	// pointer to the next server entity
	g_serverEntity_t *nextServerEntity;
	g_serverEntity_t *target_ent;
	g_serverEntity_t *chain;

	char *name;
	char *target;

	vec3_t origin;                  // Let's try keeping this simple, and just having an origin
	vec3_t angles;                  // facing angle (for a seek cover spot)
	int number;                     // identifier for this entity
	int team;                       // which team?  seek cover spots need this
	int areaNum;                    // This thing doesn't move, so we should only need to calc the areanum once

	void (*setup)(g_serverEntity_t *self);              // Setup function called once after all server objects are created
};

// clear out all the sp entities
void InitServerEntities(void);
// These server entities don't get to update every frame, but some of them have to set themselves up
//		after they've all been created
//		So we want to give each entity the chance to set itself up after it has been created
void InitialServerEntitySetup();

#define SE_FOFS(x) ((int)&(((g_serverEntity_t *)0)->x))

// HRESULTS
#define G_OK            0
#define G_INVALID       -1
#define G_NOTFOUND  -2

#define AP(x) trap_SendServerCommand(-1, x)                     // Print to all
#define CP(x) trap_SendServerCommand(ent - g_entities, x)         // Print to an ent
#define CPx(x, y) trap_SendServerCommand(x, y)                  // Print to id = x

#if defined _WIN32
# define __func__ __FUNCTION__
#endif

// Nico, log macros with function name
// LDI -> Log Debug Info
// LDE -> Log Debug Error
#define LDI(format, ...) G_LogDebug(__func__, "INFO", format, __VA_ARGS__)
#define LDE(format, ...) G_LogDebug(__func__, "ERROR", format, __VA_ARGS__)

#define HELP_COLUMNS    4

#define CMD_DEBOUNCE    5000    // 5s between cmds

#define EOM_WEAPONSTATS 0x01    // Dump of player weapon stats at end of match.
#define EOM_MATCHINFO   0x02    // Dump of match stats at end of match.

// Remember: Axis = RED, Allies = BLUE ... right?!

// Team extras
typedef struct {
	qboolean team_lock;
	char team_name[24];
	int team_score;
} team_info;

///////////////////////
// g_main.c
//
void G_InitGame(int levelTime, int randomSeed);
void G_RunFrame(int levelTime);
void G_ShutdownGame(int restart);
void G_UpdateCvars(void);

///////////////////////
// g_cmds_ext.c
//
qboolean G_commandCheck(gentity_t *ent, char *cmd);
qboolean G_commandHelp(gentity_t *ent, char *pszCommand, unsigned int dwCommand);
qboolean G_cmdDebounce(gentity_t *ent, const char *pszCommand);
void G_commands_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
void G_players_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fDump);
void G_VoiceTo(gentity_t *ent, gentity_t *other, int mode, const char *id, qboolean voiceonly);

///////////////////////
// g_match.c
//
qboolean G_allowPanzer(gentity_t *ent);
int G_checkServerToggle(vmCvar_t *cv);
void G_globalSound(char *sound);
void G_initMatch(void);
void G_loadMatchGame(void);
void G_printFull(char *str, gentity_t *ent);

///////////////////////
// g_referee.c
//
void Cmd_AuthRcon_f(gentity_t *ent);
void G_ref_cmd(gentity_t *ent, unsigned int dwCommand, qboolean fValue);
qboolean G_refCommandCheck(gentity_t *ent, char *cmd);
void G_refHelp_cmd(gentity_t *ent);
void G_refLockTeams_cmd(gentity_t *ent, qboolean fLock);
void G_refPlayerPut_cmd(gentity_t *ent, int team_id);
void G_refRemove_cmd(gentity_t *ent);
void G_refWarning_cmd(gentity_t *ent);
void G_refMute_cmd(gentity_t *ent, qboolean mute);
int  G_refClientnumForName(gentity_t *ent, const char *name);
void G_refPrintf(gentity_t *ent, const char *fmt, ...) _attribute((format(printf, 2, 3)));
void G_PlayerBan(void);
void G_MakeReferee(void);
void G_RemoveReferee(void);

///////////////////////
// g_team.c
//
extern char      *aTeams[TEAM_NUM_TEAMS];
extern team_info teamInfo[TEAM_NUM_TEAMS];

qboolean G_AllowFollow(gentity_t *ent, gentity_t *other);
qboolean G_DesiredFollow(gentity_t *ent, gentity_t *other);
qboolean G_teamJoinCheck(int team_num, gentity_t *ent);
void G_teamReset(int team_num);

///////////////////////
// g_vote.c
//
int  G_voteCmdCheck(gentity_t *ent, char *arg, char *arg2, qboolean fRefereeCmd);
void G_voteFlags(void);
void G_voteHelp(gentity_t *ent, qboolean fShowVote);
void G_playersMessage(gentity_t *ent);
// Actual voting commands
int G_Kick_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Mute_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_UnMute_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
void G_delay_map_change(char *mapName, int delay); // Nico, function to delay a map change
void *G_delayed_map_change_watcher(void *arg); // Nico, thread used to check map changes
int G_Map_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Randommap_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_MapRestart_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Referee_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_Unreferee_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
int G_AntiLag_v(gentity_t *ent, unsigned int dwVoteIndex, char *arg, char *arg2, qboolean fRefereeCmd);
void G_LinkDebris(void);
void G_LinkDamageParents(void);
int EntsThatRadiusCanDamage(vec3_t origin, float radius, int *damagedList);
gentity_t *G_FindSmokeBomb(gentity_t *start);
gentity_t *G_FindDynamite(gentity_t *start);
void G_SetTargetName(gentity_t *ent, char *targetname);
void G_KillEnts(const char *target, gentity_t *ignore, gentity_t *killer);
void trap_EngineerTrace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask);

qboolean G_ConstructionIsPartlyBuilt(gentity_t *ent);
qboolean G_TankIsOccupied(gentity_t *ent);
qboolean G_TankIsMountable(gentity_t *ent, gentity_t *other);

// Nico, prototypes of functions in g_construction.c
qboolean G_ConstructionBegun(gentity_t *ent);
qboolean G_ConstructionIsFullyBuilt(gentity_t *ent);
qboolean G_ConstructionIsPartlyBuilt(gentity_t *ent);
gentity_t *G_ConstructionForTeam(gentity_t *toi, team_t team);
gentity_t *G_IsConstructible(team_t team, gentity_t *toi);
// End Nico

qboolean G_EmplacedGunIsRepairable(gentity_t *ent, gentity_t *other);
qboolean G_EmplacedGunIsMountable(gentity_t *ent, gentity_t *other);
void G_CheckForCursorHints(gentity_t *ent);

qboolean G_IsFireteamLeader(int entityNum, fireteamData_t **teamNum);
void G_RegisterFireteam(int entityNum, qboolean priv);

void SetPlayerSpawn(gentity_t *ent, int spawn, qboolean update);
void G_UpdateSpawnCounts(void);

void G_SetConfigStringValue(int num, const char *key, const char *value);

void G_ResetTempTraceIgnoreEnts(void);
void G_TempTraceIgnoreEntity(gentity_t *ent);
void G_TempTraceIgnorePlayersAndBodies(void);

qboolean G_CanPickupWeapon(weapon_t weapon, gentity_t *ent);

// Nico, flags enabling map entities
#define MAP_FORCE_KILL_ENTITIES         1
#define MAP_FORCE_HURT_ENTITIES         2
#define MAP_JUMPPADS                4
#define MAP_VELOCITY_JUMPPADS       8
#define MAP_LOCATION_JUMPPADS       16
#define MAP_DISABLE_HURT_ENTITIES   32

// Nico, some limits for some cvars & client settings
#define MIN_PLAYER_FPS_VALUE            40
#define MAX_PLAYER_FPS_VALUE            999
#define MAX_PLAYER_PING                 400
#define MIN_PLAYER_MAX_PACKETS_VALUE    30
#define MAX_PLAYER_MAX_PACKETS_VALUE    100
#define FORCED_PLAYER_TIMENUDGE_VALUE   0
#define MIN_PLAYER_RATE_VALUE           5000
#define MAX_PLAYER_RATE_VALUE           32000
#define MIN_PLAYER_SNAPS_VALUE          0
#define MAX_PLAYER_SNAPS_VALUE          20
