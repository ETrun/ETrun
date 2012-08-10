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

// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definately
// be a valid snapshot this frame

#include "cg_local.h"

#define SCOREPARSE_COUNT    9

void CG_LimboMenu_f();


/*
=================
CG_ParseScores

=================
*/
// Gordon: NOTE: team doesnt actually signify team, think i was on drugs that day.....
static void CG_ParseScore( team_t team ) {
	int i, j, powerups;
	int numScores;

	if ( team == TEAM_AXIS ) {
		cg.numScores = 0;
	}

	numScores = atoi( CG_Argv( 1 ) );

	for ( j = 0; j < numScores; j++ ) {
		i = cg.numScores;

		cg.scores[i].client = atoi(         CG_Argv( 2 + ( j * 12 ) ) );// Nico, *7 => *12

		cg.scores[i].ping = atoi(           CG_Argv( 3 + ( j * 12 ) ) );// Nico, 2 => 1, *7 => *12
		cg.scores[i].time = atoi(           CG_Argv( 4 + ( j * 12 ) ) );// Nico, 3 => 2, *7 => *12
		powerups = atoi(                    CG_Argv( 5 + ( j * 12 ) ) );// Nico, 4 => 3, *7 => *12
		cg.scores[i].playerClass = atoi(    CG_Argv( 6 + ( j * 12 ) ) );// Nico, 5 => 4, *7 => *12

		// Nico, timerun best time
		cg.scores[i].timerunBestTime = atoi(CG_Argv(7 + (j * 12)));

		// Nico, timerun best speed
		cg.scores[i].timerunBestSpeed = atoi(CG_Argv(8 + (j * 12)));

		// Nico, timerun status
		cg.scores[i].timerunStatus = atoi(CG_Argv(9 + (j * 12)));

		// Nico, followed client
		cg.scores[i].followedClient = atoi(CG_Argv(10 + (j * 12)));

		// Nico, login status
		cg.scores[i].logged = atoi(CG_Argv(11 + (j * 12)));

		// Nico, cgaz setting
		cg.scores[i].cgaz = atoi(CG_Argv(12 + (j * 12)));

		// Nico, speclocked status
		cg.scores[i].speclocked = atoi(CG_Argv(13 + (j * 12)));

		if ( cg.scores[i].client < 0 || cg.scores[i].client >= MAX_CLIENTS ) {
			cg.scores[i].client = 0;
		}

		cgs.clientinfo[ cg.scores[i].client ].powerups = powerups;

		cg.scores[i].team = cgs.clientinfo[cg.scores[i].client].team;

		cg.numScores++;
	}
}

/*
=================
CG_ParseTeamInfo

=================
*/
#define NUMARGS 5
static void CG_ParseTeamInfo( void ) {
	int i;
	int client;
	int numSortedTeamPlayers;

	numSortedTeamPlayers = atoi( CG_Argv( 1 ) );

	for ( i = 0 ; i < numSortedTeamPlayers ; i++ ) {
		client = atoi( CG_Argv( i * NUMARGS + 2 ) );

		cgs.clientinfo[ client ].location[0] =          atoi( CG_Argv( i * NUMARGS + 3 ) );
		cgs.clientinfo[ client ].location[1] =          atoi( CG_Argv( i * NUMARGS + 4 ) );
		cgs.clientinfo[ client ].health =               atoi( CG_Argv( i * NUMARGS + 5 ) );
		cgs.clientinfo[ client ].powerups =             atoi( CG_Argv( i * NUMARGS + 6 ) );
	}
}

/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo( void ) {
	const char  *info;
	char    *mapname;

	info = CG_ConfigString( CS_SERVERINFO );

	cg_antilag.integer = cgs.antilag = atoi( Info_ValueForKey( info, "g_antilag" ) );
	if ( !cgs.localServer ) {
		trap_Cvar_Set( "g_antilag", va( "%i", cgs.antilag ) );
		trap_Cvar_Update( &cg_antilag );
	}

	cgs.maxclients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	mapname = Info_ValueForKey( info, "mapname" );
	Q_strncpyz( cgs.rawmapname, mapname, sizeof( cgs.rawmapname ) );
	Com_sprintf( cgs.mapname, sizeof( cgs.mapname ), "maps/%s.bsp", mapname );

	cgs.weaponRestrictions = atoi( Info_ValueForKey( info, "g_heavyWeaponRestriction" ) ) * 0.01f;

	// TTimo - make this available for ingame_callvote
	trap_Cvar_Set( "cg_ui_voteFlags", ( ( authLevel.integer == RL_NONE ) ? Info_ValueForKey( info, "voteFlags" ) : "0" ) );
}

/*
==================
CG_ParseOIDInfo
==================
*/

oidInfo_t* CG_OIDInfoForEntityNum( int num ) {
	int i;

	for ( i = 0; i < MAX_OID_TRIGGERS; i++ ) {
		if ( cgs.oidInfo[ i ].entityNum == num ) {
			return &cgs.oidInfo[ i ];
		}
	}

	return NULL;
}

void CG_ParseOIDInfo( int num ) {
	const char* info;
	const char* cs;
	int index = num - CS_OID_DATA;

	info = CG_ConfigString( num );

	memset( &cgs.oidInfo[ index ], 0, sizeof( cgs.oidInfo[ 0 ] ) );

	if ( !info || !*info ) {
		return;
	}

	cs = Info_ValueForKey( info, "s" );
	if ( cs && *cs ) {
		cgs.oidInfo[ index ].spawnflags = atoi( cs );
	}

	cs = Info_ValueForKey( info, "cia" );
	if ( cs && *cs ) {
		cgs.oidInfo[ index ].customimageallies = cgs.gameShaders[atoi( cs )];
	}

	cs = Info_ValueForKey( info, "cix" );
	if ( cs && *cs ) {
		cgs.oidInfo[ index ].customimageaxis = cgs.gameShaders[atoi( cs )];
	}

	cs = Info_ValueForKey( info, "o" );
	if ( cs && *cs ) {
		cgs.oidInfo[ index ].objflags = atoi( cs );
	}

	cs = Info_ValueForKey( info, "e" );
	if ( cs && *cs ) {
		cgs.oidInfo[ index ].entityNum = atoi( cs );
	}

	cs = Info_ValueForKey( info, "n" );
	if ( cs && *cs ) {
		Q_strncpyz( cgs.oidInfo[ index ].name, cs, sizeof( cgs.oidInfo[ 0 ].name ) );
	}

	cs = Info_ValueForKey( info, "x" );
	if ( cs && *cs ) {
		cgs.oidInfo[ index ].origin[0] = atoi( cs );
	}

	cs = Info_ValueForKey( info, "y" );
	if ( cs && *cs ) {
		cgs.oidInfo[ index ].origin[1] = atoi( cs );
	}

	cs = Info_ValueForKey( info, "z" );
	if ( cs && *cs ) {
		cgs.oidInfo[ index ].origin[2] = atoi( cs );
	}
}

void CG_ParseOIDInfos( void ) {
	int i;

	for ( i = 0; i < MAX_OID_TRIGGERS; i++ ) {
		CG_ParseOIDInfo( CS_OID_DATA + i );
	}
}

/*
==================
CG_ParseWolfinfo

NERVE - SMF
==================
*/
void CG_ParseWolfinfo( void ) {
	int old_gs = cgs.gamestate;
	const char *info;

	info = CG_ConfigString( CS_WOLFINFO );

	cgs.gamestate = atoi( Info_ValueForKey( info, "gamestate" ) );

	// OSP - Announce game in progress if we are really playing
	if ( old_gs != GS_PLAYING && cgs.gamestate == GS_PLAYING ) {
		Pri( "^1FIGHT!\n" );
		CPri( "^1FIGHT!\n" );
	}

	if ( !cgs.localServer ) {
		trap_Cvar_Set( "gamestate", va( "%i", cgs.gamestate ) );
	}
}

/*
==================
CG_ParseSpawns
==================
*/
void CG_ParseSpawns( void ) {
	const char *info;
	const char *s;
	int i;
	int newteam;

	info = CG_ConfigString( CS_MULTI_INFO );
	s = Info_ValueForKey( info, "numspawntargets" );

	if ( !s || !strlen( s ) ) {
		return;
	}

	// first index is for autopicking
	Q_strncpyz( cg.spawnPoints[0], CG_TranslateString( "Auto Pick" ), MAX_SPAWNDESC );

	cg.spawnCount = atoi( s ) + 1;

	for ( i = 1; i < cg.spawnCount; i++ ) {
		info = CG_ConfigString( CS_MULTI_SPAWNTARGETS + i - 1 );

		s = Info_ValueForKey( info, "spawn_targ" );

		if ( !s || !strlen( s ) ) {
			return;
		}

		Q_strncpyz( cg.spawnPoints[i], CG_TranslateString( s ), MAX_SPAWNDESC );

		s = Info_ValueForKey( info, "x" );
		if ( !s || !strlen( s ) ) {
			return;
		}
		cg.spawnCoordsUntransformed[i][0] = cg.spawnCoords[i][0] = atof( s );

		s = Info_ValueForKey( info, "y" );
		if ( !s || !strlen( s ) ) {
			return;
		}
		cg.spawnCoordsUntransformed[i][1] = cg.spawnCoords[i][1] = atof( s );

		if ( cgs.ccLayers ) {
			s = Info_ValueForKey( info, "z" );
			if ( !s || !strlen( s ) ) {
				return;
			}
			cg.spawnCoordsUntransformed[i][2] = cg.spawnCoords[i][2] = atof( s );
		}

		s = Info_ValueForKey( info, "t" );

		newteam = atoi( s );
		if ( cg.spawnTeams[i] != newteam ) {
			cg.spawnTeams_old[i] = cg.spawnTeams[i];
			cg.spawnTeams_changeTime[i] = cg.time;
			cg.spawnTeams[i] = newteam;
		}

		s = Info_ValueForKey( info, "c" );
		cg.spawnPlayerCounts[i] = atoi( s );
	}
}

/*
=====================
CG_ParseScreenFade
=====================
*/
static void CG_ParseScreenFade( void ) {
	const char  *info;
	char *token;

	info = CG_ConfigString( CS_SCREENFADE );

	token = COM_Parse( (char **)&info );
	cgs.fadeAlpha = atof( token );

	token = COM_Parse( (char **)&info );
	cgs.fadeStartTime = atoi( token );
	token = COM_Parse( (char **)&info );
	cgs.fadeDuration = atoi( token );

	if ( cgs.fadeStartTime + cgs.fadeDuration < cg.time ) {
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	}
}


/*
==============
CG_ParseFog
	float near dist
	float far dist
	float density
	float[3] r,g,b
	int		time
==============
*/
static void CG_ParseFog( void ) {
	const char  *info;
	char *token;
	float ne, fa, r, g, b, density;
	int time;

	info = CG_ConfigString( CS_FOGVARS );

	token = COM_Parse( (char **)&info );    ne = atof( token );
	token = COM_Parse( (char **)&info );    fa = atof( token );
	token = COM_Parse( (char **)&info );    density = atof( token );
	token = COM_Parse( (char **)&info );    r = atof( token );
	token = COM_Parse( (char **)&info );    g = atof( token );
	token = COM_Parse( (char **)&info );    b = atof( token );
	token = COM_Parse( (char **)&info );    time = atoi( token );

	if ( fa ) {    // far of '0' from a target_fog means "return to map fog"
		trap_R_SetFog( FOG_SERVER, (int)ne, (int)fa, r, g, b, density + .1 );
		trap_R_SetFog( FOG_CMD_SWITCHFOG, FOG_SERVER, time, 0, 0, 0, 0 );
	} else {
		trap_R_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP, time, 0, 0, 0, 0 );
	}
}

static void CG_ParseGlobalFog( void ) {
	const char  *info;
	char        *token;
	qboolean restore;
	float r, g, b, depthForOpaque;
	int duration;

	info = CG_ConfigString( CS_GLOBALFOGVARS );

	token = COM_Parse( (char **)&info );    restore = atoi( token );
	token = COM_Parse( (char **)&info );    duration = atoi( token );

	if ( restore ) {
		trap_R_SetGlobalFog( qtrue, duration, 0.f, 0.f, 0.f, 0 );
	} else {
		token = COM_Parse( (char **)&info );    r = atof( token );
		token = COM_Parse( (char **)&info );    g = atof( token );
		token = COM_Parse( (char **)&info );    b = atof( token );
		token = COM_Parse( (char **)&info );    depthForOpaque = atof( token );

		trap_R_SetGlobalFog( qfalse, duration, r, g, b, depthForOpaque );
	}
}

// Parse server version info (for demo playback compatibility)
void CG_ParseServerVersionInfo( const char *pszVersionInfo ) {
	// This will expand to a tokenized string, eventually but for
	// now we only need to worry about 1 number :)
	cgs.game_versioninfo = atoi( pszVersionInfo );
}

// Parse reinforcement offsets
void CG_ParseReinforcementTimes( const char *pszReinfSeedString ) {
	const char *tmp = pszReinfSeedString, *tmp2;
	unsigned int i, j, dwDummy = 0;// Nico, note, no not remove dwDummy
	unsigned int dwOffset[TEAM_NUM_TEAMS];

	dwDummy = dwDummy;// Nico, ugly fix to calm down GCC

#define GETVAL( x,y ) if ( ( tmp = strchr( tmp, ' ' ) ) == NULL ) {return;} x = atoi( ++tmp ) / y;

	dwOffset[TEAM_ALLIES] = atoi( pszReinfSeedString ) >> REINF_BLUEDELT;
	GETVAL( dwOffset[TEAM_AXIS], ( 1 << REINF_REDDELT ) );
	tmp2 = tmp;

	for ( i = TEAM_AXIS; i <= TEAM_ALLIES; i++ ) {
		tmp = tmp2;
		for ( j = 0; j < MAX_REINFSEEDS; j++ ) {
			if ( j == dwOffset[i] ) {
				GETVAL( cgs.aReinfOffset[i], aReinfSeeds[j] );
				cgs.aReinfOffset[i] *= 1000;
				break;
			}
			GETVAL( dwDummy, 1 );
		}
	}
}


/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues( void ) {
	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );

	// rain - set all of this crap in cgs - it won't be set if it doesn't
	// change, otherwise.  consider:
	// vote was called 5 minutes ago for 'Match Reset'.  you connect.
	// you're sent that value for CS_VOTE_STRING, but ignore it, so
	// you have nothing to use if another 'Match Reset' vote is called
	// (no update will be sent because the string will be the same.)

	cgs.voteTime = atoi( CG_ConfigString( CS_VOTE_TIME ) );
	cgs.voteYes = atoi( CG_ConfigString( CS_VOTE_YES ) );
	cgs.voteNo = atoi( CG_ConfigString( CS_VOTE_NO ) );
	Q_strncpyz( cgs.voteString, CG_ConfigString( CS_VOTE_STRING ), sizeof( cgs.voteString ) );

	// OSP
	CG_ParseServerVersionInfo( CG_ConfigString( CS_VERSIONINFO ) );
	CG_ParseReinforcementTimes( CG_ConfigString( CS_REINFSEEDS ) );
	// OSP
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged( void ) {
	char originalShader[MAX_QPATH];
	char newShader[MAX_QPATH];
	char timeOffset[16];
	const char *o;
	char *n,*t;

	o = CG_ConfigString( CS_SHADERSTATE );
	while ( o && *o ) {
		n = strstr( o, "=" );
		if ( n && *n ) {
			strncpy( originalShader, o, n - o );
			originalShader[n - o] = 0;
			n++;
			t = strstr( n, ":" );
			if ( t && *t ) {
				strncpy( newShader, n, t - n );
				newShader[t - n] = 0;
			} else {
				break;
			}
			t++;
			o = strstr( t, "@" );
			if ( o ) {
				strncpy( timeOffset, t, o - t );
				timeOffset[o - t] = 0;
				o++;
				trap_R_RemapShader( cgs.gameShaderNames[atoi( originalShader )],
									cgs.gameShaderNames[atoi( newShader )],
									timeOffset );
			}
		} else {
			break;
		}
	}
}

/*
===============
CG_ChargeTimesChanged
===============
*/
void CG_ChargeTimesChanged( void ) {
	const char *info;

	info = CG_ConfigString( CS_CHARGETIMES );

	cg.soldierChargeTime[0] = atoi( Info_ValueForKey( info, "axs_sld" ) );
	cg.soldierChargeTime[1] = atoi( Info_ValueForKey( info, "ald_sld" ) );
	cg.medicChargeTime[0] = atoi( Info_ValueForKey( info, "axs_mdc" ) );
	cg.medicChargeTime[1] = atoi( Info_ValueForKey( info, "ald_mdc" ) );
	cg.engineerChargeTime[0] = atoi( Info_ValueForKey( info, "axs_eng" ) );
	cg.engineerChargeTime[1] = atoi( Info_ValueForKey( info, "ald_eng" ) );
	cg.ltChargeTime[0] = atoi( Info_ValueForKey( info, "axs_lnt" ) );
	cg.ltChargeTime[1] = atoi( Info_ValueForKey( info, "ald_lnt" ) );
	cg.covertopsChargeTime[0] = atoi( Info_ValueForKey( info, "axs_cvo" ) );
	cg.covertopsChargeTime[1] = atoi( Info_ValueForKey( info, "ald_cvo" ) );
}

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified( void ) {
	const char  *str;
	int num;

	num = atoi( CG_Argv( 1 ) );

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState( &cgs.gameState );

	// look up the individual string that was modified
	str = CG_ConfigString( num );

	// do something with it if necessary
	if ( num == CS_MUSIC ) {
		CG_StartMusic();
	} else if ( num == CS_MUSIC_QUEUE ) {
		CG_QueueMusic();
	} else if ( num == CS_SERVERINFO ) {
		CG_ParseServerinfo();
	} 
	else if ( num == CS_WOLFINFO ) {      // NERVE - SMF
		CG_ParseWolfinfo();
	}
	else if ( num >= CS_MULTI_SPAWNTARGETS && num < CS_MULTI_SPAWNTARGETS + MAX_MULTI_SPAWNTARGETS ) {
		CG_ParseSpawns();
	} else if ( num == CS_VERSIONINFO ) {
		CG_ParseServerVersionInfo( str );         // OSP - set versioning info for older demo playback
	} else if ( num == CS_REINFSEEDS ) {
		CG_ParseReinforcementTimes( str );        // OSP - set reinforcement times for each team
	} else if ( num == CS_LEVEL_START_TIME ) {
		cgs.levelStartTime = atoi( str );
	}
	else if ( num == CS_VOTE_TIME ) {
		cgs.voteTime = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_YES ) {
		cgs.voteYes = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_NO ) {
		cgs.voteNo = atoi( str );
		cgs.voteModified = qtrue;
	} else if ( num == CS_VOTE_STRING ) {
		Q_strncpyz( cgs.voteString, str, sizeof( cgs.voteString ) );
	}
	else if ( num == CS_SCREENFADE ) {
		CG_ParseScreenFade();
	} else if ( num == CS_FOGVARS ) {
		CG_ParseFog();
	} else if ( num == CS_GLOBALFOGVARS ) {
		CG_ParseGlobalFog();
	} else if ( num >= CS_MODELS && num < CS_MODELS + MAX_MODELS ) {
		cgs.gameModels[ num - CS_MODELS ] = trap_R_RegisterModel( str );
	} else if ( num >= CS_SOUNDS && num < CS_SOUNDS + MAX_SOUNDS ) {
		if ( str[0] != '*' ) {   // player specific sounds don't register here

			// Ridah, register sound scripts seperately
			if ( !strstr( str, ".wav" ) ) {
				CG_SoundScriptPrecache( str );
			} else {
				cgs.gameSounds[ num - CS_SOUNDS] = trap_S_RegisterSound( str, qfalse );   //FIXME: add a compress flag?
			}

		}
	} else if ( num >= CS_SHADERS && num < CS_SHADERS + MAX_CS_SHADERS ) {
		cgs.gameShaders[ num - CS_SHADERS ] = str[0] == '*' ? trap_R_RegisterShader( str + 1 ) : trap_R_RegisterShaderNoMip( str );
		Q_strncpyz( cgs.gameShaderNames[num - CS_SHADERS], str[0] == '*' ? str + 1 : str, MAX_QPATH );
	} else if ( num >= CS_SKINS && num < CS_SKINS + MAX_CS_SKINS ) {
		cgs.gameModelSkins[ num - CS_SKINS ] = trap_R_RegisterSkin( str );
	} else if ( num >= CS_CHARACTERS && num < CS_CHARACTERS + MAX_CHARACTERS ) {
		if ( !BG_FindCharacter( str ) ) {
			cgs.gameCharacters[ num - CS_CHARACTERS ] = BG_FindFreeCharacter( str );

			Q_strncpyz( cgs.gameCharacters[ num - CS_CHARACTERS ]->characterFile, str, sizeof( cgs.gameCharacters[ num - CS_CHARACTERS ]->characterFile ) );

			if ( !CG_RegisterCharacter( str, cgs.gameCharacters[ num - CS_CHARACTERS ] ) ) {
				CG_Error( "ERROR: CG_ConfigStringModified: failed to load character file '%s'\n", str );
			}
		}
	} else if ( num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS ) {
		CG_NewClientInfo( num - CS_PLAYERS );
	} else if ( num >= CS_DLIGHTS && num < CS_DLIGHTS + MAX_DLIGHT_CONFIGSTRINGS ) {
		// FIXME - dlight changes ignored!
	} else if ( num == CS_SHADERSTATE ) {
		CG_ShaderStateChanged();
	} else if ( num == CS_CHARGETIMES ) {
		CG_ChargeTimesChanged();
	} else if ( num >= CS_FIRETEAMS && num < CS_FIRETEAMS + MAX_FIRETEAMS ) {
		CG_ParseFireteams();
	} else if ( num == CS_SKYBOXORG ) {
		CG_ParseSkyBox();
	} else if ( num >= CS_TAGCONNECTS && num < CS_TAGCONNECTS + MAX_TAGCONNECTS ) {
		CG_ParseTagConnect( num );
	}

	if ( num == CS_FILTERCAMS ) {
		cg.filtercams = atoi( str ) ? qtrue : qfalse;
	} else if ( num >= CS_OID_DATA && num < CS_OID_DATA + MAX_OID_TRIGGERS ) {
		CG_ParseOIDInfo( num );
	}
}

/*
=======================
CG_AddToTeamChat

=======================
*/
static void CG_AddToTeamChat( const char *str, int clientnum ) {
	int len;
	char *p, *ls;
	int lastcolor;
	int chatHeight;

	if ( cg_teamChatHeight.integer < TEAMCHAT_HEIGHT ) {
		chatHeight = cg_teamChatHeight.integer;
	} else {
		chatHeight = TEAMCHAT_HEIGHT;
	}

	if ( chatHeight <= 0 || cg_teamChatTime.integer <= 0 ) {
		// team chat disabled, dump into normal chat
		cgs.teamChatPos = cgs.teamLastChatPos = 0;
		return;
	}

	len = 0;

	p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while ( *str ) {
		if ( len > TEAMCHAT_WIDTH - 1 ) {
			if ( ls ) {
				str -= ( p - ls );
				str++;
				p -= ( p - ls );
			}
			*p = 0;

			cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
			cgs.teamChatMsgTeams[cgs.teamChatPos % chatHeight] = cgs.clientinfo[ clientnum ].team;

			cgs.teamChatPos++;
			p = cgs.teamChatMsgs[cgs.teamChatPos % chatHeight];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}

		if ( Q_IsColorString( str ) ) {
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if ( *str == ' ' ) {
			ls = p;
		}
		*p++ = *str++;
		len++;
	}
	*p = 0;

	cgs.teamChatMsgTeams[cgs.teamChatPos % chatHeight] = cgs.clientinfo[ clientnum ].team;
	cgs.teamChatMsgTimes[cgs.teamChatPos % chatHeight] = cg.time;
	cgs.teamChatPos++;

	if ( cgs.teamChatPos - cgs.teamLastChatPos > chatHeight ) {
		cgs.teamLastChatPos = cgs.teamChatPos - chatHeight;
	}
}

/*
=======================
CG_AddToNotify

=======================
*/
void CG_AddToNotify( const char *str ) {
	int len;
	char *p, *ls;
	int lastcolor;
	int chatHeight;
	float notifytime;
	char var[MAX_TOKEN_CHARS];

	trap_Cvar_VariableStringBuffer( "con_notifytime", var, sizeof( var ) );
	notifytime = atof( var ) * 1000;

	chatHeight = NOTIFY_HEIGHT;

	if ( chatHeight <= 0 || notifytime <= 0 ) {
		// team chat disabled, dump into normal chat
		cgs.notifyPos = cgs.notifyLastPos = 0;
		return;
	}

	len = 0;

	p = cgs.notifyMsgs[cgs.notifyPos % chatHeight];
	*p = 0;

	lastcolor = '7';

	ls = NULL;
	while ( *str ) {
		if ( len > NOTIFY_WIDTH - 1 || ( *str == '\n' && ( *( str + 1 ) != 0 ) ) ) {
			if ( ls ) {
				str -= ( p - ls );
				str++;
				p -= ( p - ls );
			}
			*p = 0;

			cgs.notifyMsgTimes[cgs.notifyPos % chatHeight] = cg.time;

			cgs.notifyPos++;
			p = cgs.notifyMsgs[cgs.notifyPos % chatHeight];
			*p = 0;
			*p++ = Q_COLOR_ESCAPE;
			*p++ = lastcolor;
			len = 0;
			ls = NULL;
		}

		if ( Q_IsColorString( str ) ) {
			*p++ = *str++;
			lastcolor = *str;
			*p++ = *str++;
			continue;
		}
		if ( *str == ' ' ) {
			ls = p;
		}
		while ( *str == '\n' ) {
			// TTimo gcc warning: value computed is not used
			// was *str++;
			str++;
		}

		if ( *str ) {
			*p++ = *str++;
			len++;
		}
	}
	*p = 0;

	cgs.notifyMsgTimes[cgs.notifyPos % chatHeight] = cg.time;
	cgs.notifyPos++;

	if ( cgs.notifyPos - cgs.notifyLastPos > chatHeight ) {
		cgs.notifyLastPos = cgs.notifyPos - chatHeight;
	}
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart( void ) {
	if ( cg_showmiss.integer ) {
		CG_Printf( "CG_MapRestart\n" );
	}

	memset( &cg.lastWeapSelInBank[0], 0, MAX_WEAP_BANKS_MP * sizeof( int ) );   // clear weapon bank selections

	cg.numbufferedSoundScripts = 0;

	cg.centerPrintTime = 0; // reset centerprint counter so previous messages don't re-appear
	cg.itemPickupTime = 0;  // reset item pickup counter so previous messages don't re-appear
	cg.cursorHintFade = 0;  // reset cursor hint timer

	// (SA) clear zoom (so no warpies)
	cg.zoomedBinoc = qfalse;
	cg.zoomedScope = qfalse;
	cg.zoomTime = 0;
	cg.zoomval = 0;
	cgs.invitationEndTime = 0;
	cgs.applicationEndTime = 0;
	cgs.propositionEndTime = 0;

	// reset fog to world fog (if present)
	trap_R_SetFog( FOG_CMD_SWITCHFOG, FOG_MAP,20,0,0,0,0 );

	// clear pmext
	memset( &cg.pmext, 0, sizeof( cg.pmext ) );

	cg.pmext.bAutoReload = ( cg_autoReload.integer > 0 );

	numSplinePaths = 0;
	numPathCorners = 0;
	cg.numOIDtriggers2 = 0;

	cgs.fadeStartTime = 0;
	cgs.fadeAlpha = 0;
	trap_Cvar_Set( "cg_letterbox", "0" );

	CG_ParseWolfinfo();

	CG_ParseEntitiesFromString();

	CG_LoadObjectiveData();

	CG_InitLocalEntities();
	CG_InitMarkPolys();

	cg.editingSpeakers = qfalse;

	BG_BuildSplinePaths();

	InitSmokeSprites();

	//Rafael particles
	CG_ClearParticles();

	// Ridah, trails
//	CG_ClearTrails ();
	// done.

	// Ridah
	CG_ClearFlameChunks();
	CG_SoundInit();
	// done.

	cg.lightstylesInited = qfalse;
	cg.mapRestart = qtrue;

	cgs.voteTime = 0;

	CG_StartMusic();

	trap_S_ClearLoopingSounds();
	trap_S_ClearSounds( qfalse );

	// ydnar
	trap_R_ClearDecals();

	cg.latchAutoActions = qfalse;
	cg.latchVictorySound = qfalse;          // NERVE - SMF
// JPW NERVE -- reset render flags
	cg_fxflags = 0;
// jpw

	// we really should clear more parts of cg here and stop sounds
	cg.v_dmg_time = 0;
	cg.v_noFireTime = 0;
	cg.v_fireTime = 0;

	cg.filtercams = atoi( CG_ConfigString( CS_FILTERCAMS ) ) ? qtrue : qfalse;

	CG_ChargeTimesChanged();

	CG_ParseFireteams();

	CG_ParseOIDInfos();

	CG_InitPM();

	CG_ParseSpawns();

	CG_ParseTagConnects();

	trap_Cvar_Set( "cg_thirdPerson", "0" );
}
// NERVE - SMF


#define MAX_VOICEFILES      8

// TAT - 10/28/2002 we've got some really big VO files now
#define MAX_VOICEFILESIZE   32768
#define MAX_VOICECHATS      272
// TAT - NOTE: If we're worried about space - do we really need 96 possible sounds for any one chat?
//			I think this is used to allow multiple sound clips for one command, so do we need 96 available selection sounds?
#define MAX_VOICESOUNDS     32

#define MAX_CHATSIZE        64
#define MAX_HEADMODELS      64

typedef struct voiceChat_s
{
	char id[64];
	int numSounds;
	sfxHandle_t sounds[MAX_VOICESOUNDS];
	char chats[MAX_VOICESOUNDS][MAX_CHATSIZE];
	qhandle_t sprite[MAX_VOICESOUNDS];          // DHM - Nerve
} voiceChat_t;

typedef struct voiceChatList_s
{
	char name[64];
	int gender;
	int numVoiceChats;
	voiceChat_t voiceChats[MAX_VOICECHATS];
} voiceChatList_t;

typedef struct headModelVoiceChat_s
{
	char headmodel[64];
	int voiceChatNum;
} headModelVoiceChat_t;

voiceChatList_t voiceChatLists[MAX_VOICEFILES];
headModelVoiceChat_t headModelVoiceChat[MAX_HEADMODELS];

/*
=================
CG_ParseVoiceChats
=================
*/
int CG_ParseVoiceChats( const char *filename, voiceChatList_t *voiceChatList, int maxVoiceChats ) {
	int len, i;
	int current = 0;
	fileHandle_t f;
	char buf[MAX_VOICEFILESIZE];
	char **p, *ptr;
	char *token;
	voiceChat_t *voiceChats;
	qboolean compress;

	compress = qtrue;
	if ( cg_buildScript.integer ) {
		compress = qfalse;
	}

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( S_COLOR_RED "voice chat file not found: %s\n", filename ) );
		return qfalse;
	}
	if ( len >= MAX_VOICEFILESIZE ) {
		trap_Print( va( S_COLOR_RED "voice chat file too large: %s is %i, max allowed is %i", filename, len, MAX_VOICEFILESIZE ) );
		trap_FS_FCloseFile( f );
		return qfalse;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	ptr = buf;
	p = &ptr;

	Com_sprintf( voiceChatList->name, sizeof( voiceChatList->name ), "%s", filename );
	voiceChats = voiceChatList->voiceChats;
	for ( i = 0; i < maxVoiceChats; i++ ) {
		voiceChats[i].id[0] = 0;
	}
	token = COM_ParseExt( p, qtrue );
	if ( !token || token[0] == 0 ) {
		return qtrue;
	}
	if ( !Q_stricmp( token, "female" ) ) {
		voiceChatList->gender = GENDER_FEMALE;
	} else if ( !Q_stricmp( token, "male" ) )        {
		voiceChatList->gender = GENDER_MALE;
	} else if ( !Q_stricmp( token, "neuter" ) )        {
		voiceChatList->gender = GENDER_NEUTER;
	} else {
		trap_Print( va( S_COLOR_RED "expected gender not found in voice chat file: %s\n", filename ) );
		return qfalse;
	}

	// Gordon: setting before call so we can load multiple files into one list
	// TAT - 10/28/2002 - if you really want to be able to load multiple files, you should take out the loop
	//		above that clears out all the commands "voiceChats[i].id[0] = 0;"
	//		We don't even want the MP voice chats in SP, so no need anyway
	voiceChatList->numVoiceChats = 0;
	while ( 1 ) {
		token = COM_ParseExt( p, qtrue );
		if ( !token || token[0] == 0 ) {
			return qtrue;
		}

		Com_sprintf( voiceChats[voiceChatList->numVoiceChats].id, sizeof( voiceChats[voiceChatList->numVoiceChats].id ), "%s", token );
		token = COM_ParseExt( p, qtrue );
		if ( Q_stricmp( token, "{" ) ) {
			trap_Print( va( S_COLOR_RED "expected { found %s in voice chat file: %s\n", token, filename ) );
			return qfalse;
		}
		voiceChats[voiceChatList->numVoiceChats].numSounds = 0;
		current = voiceChats[voiceChatList->numVoiceChats].numSounds;

		while ( 1 ) {
			token = COM_ParseExt( p, qtrue );
			if ( !token || token[0] == 0 ) {
				return qtrue;
			}
			if ( !Q_stricmp( token, "}" ) ) {
				break;
			}
			voiceChats[voiceChatList->numVoiceChats].sounds[current] = trap_S_RegisterSound( token, compress );
			token = COM_ParseExt( p, qtrue );
			if ( !token || token[0] == 0 ) {
				return qtrue;
			}
			Com_sprintf( voiceChats[voiceChatList->numVoiceChats].chats[current], MAX_CHATSIZE, "%s", token );

			// DHM - Nerve :: Specify sprite shader to show above player's head
			token = COM_ParseExt( p, qfalse );
			if ( !Q_stricmp( token, "}" ) || !token || token[0] == 0 ) {
				voiceChats[voiceChatList->numVoiceChats].sprite[current] = trap_R_RegisterShader( "sprites/voiceChat" );
				COM_RestoreParseSession( p );
			} else {
				voiceChats[voiceChatList->numVoiceChats].sprite[current] = trap_R_RegisterShader( token );
				if ( voiceChats[voiceChatList->numVoiceChats].sprite[current] == 0 ) {
					voiceChats[voiceChatList->numVoiceChats].sprite[current] = trap_R_RegisterShader( "sprites/voiceChat" );
				}
			}
			// dhm - end

			voiceChats[voiceChatList->numVoiceChats].numSounds++;
			current = voiceChats[voiceChatList->numVoiceChats].numSounds;

			if ( voiceChats[voiceChatList->numVoiceChats].numSounds >= MAX_VOICESOUNDS ) {
				break;
			}
		}

		voiceChatList->numVoiceChats++;
		if ( voiceChatList->numVoiceChats >= maxVoiceChats ) {
			return qtrue;
		}
	}
	return qtrue;
}

/*
=================
CG_LoadVoiceChats
=================
*/
void CG_LoadVoiceChats( void ) {
	int size;

	size = trap_MemoryRemaining();
	voiceChatLists[0].numVoiceChats = 0;
	voiceChatLists[1].numVoiceChats = 0;

	CG_ParseVoiceChats( "scripts/wm_axis_chat.voice", &voiceChatLists[0], MAX_VOICECHATS );
	CG_ParseVoiceChats( "scripts/wm_allies_chat.voice", &voiceChatLists[1], MAX_VOICECHATS );

	CG_Printf( "voice chat memory size = %d\n", size - trap_MemoryRemaining() );
}

/*
=================
CG_HeadModelVoiceChats
=================
*/
int CG_HeadModelVoiceChats( char *filename ) {
	int len, i;
	fileHandle_t f;
	char buf[MAX_VOICEFILESIZE];
	char **p, *ptr;
	char *token;

	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( !f ) {
		trap_Print( va( "voice chat file not found: %s\n", filename ) );
		return -1;
	}
	if ( len >= MAX_VOICEFILESIZE ) {
		trap_Print( va( S_COLOR_RED "voice chat file too large: %s is %i, max allowed is %i", filename, len, MAX_VOICEFILESIZE ) );
		trap_FS_FCloseFile( f );
		return -1;
	}

	trap_FS_Read( buf, len, f );
	buf[len] = 0;
	trap_FS_FCloseFile( f );

	ptr = buf;
	p = &ptr;

	token = COM_ParseExt( p, qtrue );
	if ( !token || token[0] == 0 ) {
		return -1;
	}

	for ( i = 0; i < MAX_VOICEFILES; i++ ) {
		if ( !Q_stricmp( token, voiceChatLists[i].name ) ) {
			return i;
		}
	}

	//FIXME: maybe try to load the .voice file which name is stored in token?

	return -1;
}


/*
=================
CG_GetVoiceChat
=================
*/
int CG_GetVoiceChat( voiceChatList_t *voiceChatList, const char *id, sfxHandle_t *snd, qhandle_t *sprite, char **chat ) {
	int i, rnd;

	for ( i = 0; i < voiceChatList->numVoiceChats; i++ ) {
		if ( !Q_stricmp( id, voiceChatList->voiceChats[i].id ) ) {
			rnd = random() * voiceChatList->voiceChats[i].numSounds;
			*snd = voiceChatList->voiceChats[i].sounds[rnd];
			*sprite = voiceChatList->voiceChats[i].sprite[rnd];
			*chat = voiceChatList->voiceChats[i].chats[rnd];
			return qtrue;
		}
	}
	return qfalse;
}

/*
=================
CG_VoiceChatListForClient
=================
*/
voiceChatList_t *CG_VoiceChatListForClient( int clientNum ) {
	if ( cgs.clientinfo[ clientNum ].team == TEAM_AXIS ) {
		return &voiceChatLists[0];
	} else {
		return &voiceChatLists[1];
	}
}

#define MAX_VOICECHATBUFFER     32

typedef struct bufferedVoiceChat_s
{
	int clientNum;
	sfxHandle_t snd;
	qhandle_t sprite;
	int voiceOnly;
	char cmd[MAX_SAY_TEXT];
	char message[MAX_SAY_TEXT];
	vec3_t origin;          // NERVE - SMF
} bufferedVoiceChat_t;

bufferedVoiceChat_t voiceChatBuffer[MAX_VOICECHATBUFFER];

/*
=================
CG_PlayVoiceChat
=================
*/
void CG_PlayVoiceChat( bufferedVoiceChat_t *vchat ) {
	if ( !cg_noVoiceChats.integer ) {
		trap_S_StartLocalSound( vchat->snd, CHAN_VOICE );

		// Arnout: don't show icons for the HQ (clientnum -1)
		if ( vchat->clientNum != -1 ) {
			// DHM - Nerve :: Show icon above head
			if ( vchat->clientNum == cg.snap->ps.clientNum ) {
				cg.predictedPlayerEntity.voiceChatSprite = vchat->sprite;
				if ( vchat->sprite == cgs.media.voiceChatShader ) {
					cg.predictedPlayerEntity.voiceChatSpriteTime = cg.time + cg_voiceSpriteTime.integer;
				} else {
					cg.predictedPlayerEntity.voiceChatSpriteTime = cg.time + cg_voiceSpriteTime.integer * 2;
				}
			} else {
				cg_entities[ vchat->clientNum ].voiceChatSprite = vchat->sprite;
				VectorCopy( vchat->origin, cg_entities[ vchat->clientNum ].lerpOrigin );            // NERVE - SMF
				if ( vchat->sprite == cgs.media.voiceChatShader ) {
					cg_entities[ vchat->clientNum ].voiceChatSpriteTime = cg.time + cg_voiceSpriteTime.integer;
				} else {
					cg_entities[ vchat->clientNum ].voiceChatSpriteTime = cg.time + cg_voiceSpriteTime.integer * 2;
				}
			}
			// dhm - end
		}

	}
	if ( !vchat->voiceOnly && !cg_noVoiceText.integer ) {
		CG_AddToTeamChat( vchat->message, vchat->clientNum );
		CG_Printf( va( "[skipnotify]: %s\n", vchat->message ) ); // JPW NERVE
	}
	voiceChatBuffer[cg.voiceChatBufferOut].snd = 0;
}

/*
=====================
CG_PlayBufferedVoieChats
=====================
*/
void CG_PlayBufferedVoiceChats( void ) {
	if ( cg.voiceChatTime < cg.time ) {
		if ( cg.voiceChatBufferOut != cg.voiceChatBufferIn && voiceChatBuffer[cg.voiceChatBufferOut].snd ) {
			//
			CG_PlayVoiceChat( &voiceChatBuffer[cg.voiceChatBufferOut] );
			//
			cg.voiceChatBufferOut = ( cg.voiceChatBufferOut + 1 ) % MAX_VOICECHATBUFFER;
			cg.voiceChatTime = cg.time + 1000;
		}
	}
}

/*
=====================
CG_AddBufferedVoiceChat
=====================
*/
void CG_AddBufferedVoiceChat( bufferedVoiceChat_t *vchat ) {
// JPW NERVE new system doesn't buffer but overwrites vchats FIXME put this on a cvar to choose which to use
	memcpy( &voiceChatBuffer[0],vchat,sizeof( bufferedVoiceChat_t ) );
	cg.voiceChatBufferIn = 0;
	CG_PlayVoiceChat( &voiceChatBuffer[0] );
}

/*
=================
CG_VoiceChatLocal
=================
*/
void CG_VoiceChatLocal( int mode, qboolean voiceOnly, int clientNum, int color, const char *cmd, vec3_t origin ) {
	char *chat;
	voiceChatList_t *voiceChatList;
	clientInfo_t *ci;
	sfxHandle_t snd;
	qhandle_t sprite;
	bufferedVoiceChat_t vchat;
	const char *loc = " ";           // NERVE - SMF

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	cgs.currentVoiceClient = clientNum;

	voiceChatList = CG_VoiceChatListForClient( clientNum );

	if ( CG_GetVoiceChat( voiceChatList, cmd, &snd, &sprite, &chat ) ) {
		//
		if ( mode == SAY_TEAM || !cg_teamChatsOnly.integer ) {
			vchat.clientNum = clientNum;
			vchat.snd = snd;
			vchat.sprite = sprite;
			vchat.voiceOnly = voiceOnly;
			VectorCopy( origin, vchat.origin );     // NERVE - SMF
			Q_strncpyz( vchat.cmd, cmd, sizeof( vchat.cmd ) );

			if ( mode != SAY_ALL ) {
				// NERVE - SMF - get location
				loc = BG_GetLocationString( origin );
				if ( !loc || !*loc ) {
					loc = " ";
				}
			}

			if ( mode == SAY_TEAM ) {
				Com_sprintf( vchat.message, sizeof( vchat.message ), "(%s)%c%c(%s): %c%c%s",
							 ci->name, Q_COLOR_ESCAPE, COLOR_YELLOW, loc, Q_COLOR_ESCAPE, color, CG_TranslateString( chat ) );
			} else if ( mode == SAY_BUDDY ) {
				Com_sprintf( vchat.message, sizeof( vchat.message ), "<%s>%c%c<%s>: %c%c%s",
							 ci->name, Q_COLOR_ESCAPE, COLOR_YELLOW, loc, Q_COLOR_ESCAPE, color, CG_TranslateString( chat ) );
			} else {
				Com_sprintf( vchat.message, sizeof( vchat.message ), "%s%c%c: %c%c%s",
							 ci->name, Q_COLOR_ESCAPE, COLOR_YELLOW, Q_COLOR_ESCAPE, color, CG_TranslateString( chat ) );
			}
			CG_AddBufferedVoiceChat( &vchat );
		}
	}
}

/*
=================
CG_VoiceChat
=================
*/
void CG_VoiceChat( int mode ) {
	const char *cmd;
	int clientNum, color;
	qboolean voiceOnly;
	vec3_t origin;          // NERVE - SMF

	voiceOnly = atoi( CG_Argv( 1 ) );
	clientNum = atoi( CG_Argv( 2 ) );
	color = atoi( CG_Argv( 3 ) );

	if ( mode != SAY_ALL ) {
		// NERVE - SMF - added origin
		origin[0] = atoi( CG_Argv( 5 ) );
		origin[1] = atoi( CG_Argv( 6 ) );
		origin[2] = atoi( CG_Argv( 7 ) );
	}

	cmd = CG_Argv( 4 );

	if ( cg_noTaunt.integer != 0 ) {
		if ( !strcmp( cmd, VOICECHAT_KILLINSULT )  || !strcmp( cmd, VOICECHAT_TAUNT ) || \
			 !strcmp( cmd, VOICECHAT_DEATHINSULT ) || !strcmp( cmd, VOICECHAT_KILLGAUNTLET ) ||	\
			 !strcmp( cmd, VOICECHAT_PRAISE ) ) {
			return;
		}
	}

	CG_VoiceChatLocal( mode, voiceOnly, clientNum, color, cmd, origin );
}
// -NERVE - SMF

/*
=================
CG_RemoveChatEscapeChar
=================
*/
static void CG_RemoveChatEscapeChar( char *text ) {
	int i, l;

	l = 0;
	for ( i = 0; text[i]; i++ ) {
		if ( text[i] == '\x19' ) {
			continue;
		}
		text[l++] = text[i];
	}
	text[l] = '\0';
}

/*
=================
CG_LocalizeServerCommand

NERVE - SMF - localize string sent from server

- localization is ON by default.
- use [lof] in string to turn OFF
- use [lon] in string to turn back ON
=================
*/
const char* CG_LocalizeServerCommand( const char *buf ) {
	static char token[MAX_TOKEN_CHARS];
	char temp[MAX_TOKEN_CHARS];
	qboolean togloc = qtrue;
	const char *s;
	int i, prev;

	memset( token, 0, sizeof( token ) );
	s = buf;
	prev = 0;

	for ( i = 0; *s; i++, s++ ) {
		// TTimo:
		// line was: if ( *s == '[' && !Q_strncmp( s, "[lon]", 5 ) || !Q_strncmp( s, "[lof]", 5 ) ) {
		// || prevails on &&, gcc warning was 'suggest parentheses around && within ||'
		// modified to the correct behaviour
		if ( *s == '[' && ( !Q_strncmp( s, "[lon]", 5 ) || !Q_strncmp( s, "[lof]", 5 ) ) ) {

			if ( togloc ) {
				memset( temp, 0, sizeof( temp ) );
				strncpy( temp, buf + prev, i - prev );
				strcat( token, CG_TranslateString( temp ) );
			} else {
				strncat( token, buf + prev, i - prev );
			}

			if ( s[3] == 'n' ) {
				togloc = qtrue;
			} else {
				togloc = qfalse;
			}

			i += 5;
			s += 5;
			prev = i;
		}
	}

	if ( togloc ) {
		memset( temp, 0, sizeof( temp ) );
		strncpy( temp, buf + prev, i - prev );
		strcat( token, CG_TranslateString( temp ) );
	} else {
		strncat( token, buf + prev, i - prev );
	}

	return token;
}
// -NERVE - SMF

/**
 * Create banner to be printed
 * @source: TJMod
 *
 * @author Nico
 */
#define BP_LINEWIDTH 80
static void CG_BannerPrint(const char *str) {
	char buff[MAX_STRING_CHARS] = {0};
	int	i = 0;
	int len = 0;
	int textlen = 0;
	qboolean neednewline = qfalse;

	Q_strncpyz(cg.bannerPrint, str, sizeof (cg.bannerPrint));

	// turn spaces into newlines, if we've run over the linewidth
	len = strlen(cg.bannerPrint);
	for (i = 0, textlen = 0; i < len; ++i, ++textlen) {

		// "\n" in center/banner prints are seen as new lines.
		// kw: this is also done serverside in etpub
		if (cg.bannerPrint[i] == '\\' && cg.bannerPrint[i+1] == 'n') {
			Q_strncpyz(buff, &cg.bannerPrint[i + 2], sizeof (buff));
			cg.bannerPrint[i] = '\n';
			cg.bannerPrint[i + 1] = 0;
			Q_strcat(cg.bannerPrint, sizeof (cg.bannerPrint), buff);
		}

		if (cg.bannerPrint[i] == '\n') {
			textlen = 0;
		}

		if (Q_IsColorString(&cg.bannerPrint[i])) {
			textlen -= 2;
		}

		// NOTE: subtracted a few chars here so long words still
		// get displayed properly
		if (textlen % (BP_LINEWIDTH - 10) == 0 && textlen > 0) {
			neednewline = qtrue;
		}

		if (cg.bannerPrint[i] == ' ' && neednewline) {
			cg.bannerPrint[i] = '\n';
			textlen = 0;
			neednewline = qfalse;
		}

		// if still to long just cut it at BP_LINEWIDTH
		if (textlen % BP_LINEWIDTH == 0 && textlen > 0) {
			Q_strncpyz(buff, &cg.bannerPrint[i], sizeof (buff));
			cg.bannerPrint[i] = '\n';
			cg.bannerPrint[i + 1] = 0;
			Q_strcat(cg.bannerPrint, sizeof (cg.bannerPrint), buff);
			textlen = 0;
			neednewline = qfalse;
		}
	}

	// post-editing to print text correctly into the console
	for (i = 0, len = 0; i < strlen(cg.bannerPrint); ++i) {
		// replace newlines with spaces
		if (cg.bannerPrint[i] == '\n' ) {
			if (len != 0 && buff[len - 1] != ' ') {
				buff[len] = ' ';
				len++;
			}
			continue;
		}
		// no spaces at the beginning of the string
		if (len == 0 && cg.bannerPrint[i] == ' ')  {
			continue;
		}
		buff[len] = cg.bannerPrint[i];
		len++;
	}
	buff[len] = 0;

	CG_Printf("^9banner: ^7%s\n", buff);

	cg.bannerPrintTime = cg.time;
}

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
Cmd_Argc() / Cmd_Argv()
=================
*/
static void CG_ServerCommand( void ) {
	const char  *cmd;
	char text[MAX_SAY_TEXT];
	qboolean enc = qfalse; // used for enc_chat, enc_tchat
	static int currentdemo = 0, ignoreNextStart = 0;

	cmd = CG_Argv( 0 );

	if ( !cmd[0] ) {
		// server claimed the command
		return;
	}

	if ( !strcmp( cmd, "tinfo" ) ) {
		CG_ParseTeamInfo();
		return;
	}
	if ( !strcmp( cmd, "sc0" ) ) {
		CG_ParseScore( TEAM_AXIS );
		return;
	} else if ( !strcmp( cmd, "sc1" ) ) {
		CG_ParseScore( TEAM_ALLIES );
		return;
	}

	if ( !Q_stricmp( cmd, "cpm" ) ) {
		CG_AddPMItem( PM_MESSAGE, CG_LocalizeServerCommand( CG_Argv( 1 ) ), cgs.media.voiceChatShader );
		return;
	}

	if ( !Q_stricmp( cmd, "cp" ) ) {
		// NERVE - SMF
		int args = trap_Argc();
		char *s;

		if ( args >= 3 ) {
			s = CG_TranslateString( CG_Argv( 1 ) );

			if ( args == 4 ) {
				s = va( "%s%s", CG_Argv( 3 ), s );
			}

			// OSP - for client logging
			if ( cg_printObjectiveInfo.integer > 0 && ( args == 4 || atoi( CG_Argv( 2 ) ) > 1 ) ) {
				CG_Printf( "[cgnotify]*** ^3INFO: ^5%s\n", CG_LocalizeServerCommand( CG_Argv( 1 ) ) );
			}
			CG_PriorityCenterPrint( s, SCREEN_HEIGHT - ( SCREEN_HEIGHT * 0.20 ), SMALLCHAR_WIDTH, atoi( CG_Argv( 2 ) ) );
		} else {
			CG_CenterPrint( CG_LocalizeServerCommand( CG_Argv( 1 ) ), SCREEN_HEIGHT - ( SCREEN_HEIGHT * 0.20 ), SMALLCHAR_WIDTH );  //----(SA)	modified
		}
		return;
	}

	if ( !Q_stricmp( cmd, "reqforcespawn" ) ) {
		return;
	}

	if ( !Q_stricmp( cmd, "sdbg" ) ) {
		CG_StatsDebugAddText( CG_Argv( 1 ) );
		return;
	}

	if ( !Q_stricmp( cmd, "cs" ) ) {
		CG_ConfigStringModified();
		return;
	}

	if ( !Q_stricmp( cmd, "print" ) ) {
		CG_Printf( "[cgnotify]%s", CG_LocalizeServerCommand( CG_Argv( 1 ) ) );
		return;
	}

	if (!Q_stricmp(cmd, "chat") || (enc = !Q_stricmp(cmd, "enc_chat"))) {
		const char *s;

		if ( cg_teamChatsOnly.integer ) {
			return;
		}

		if ( atoi( CG_Argv( 3 ) ) ) {
			s = CG_LocalizeServerCommand( CG_Argv( 1 ) );
		} else {
			s = CG_Argv( 1 );
		}

		Q_strncpyz( text, s, MAX_SAY_TEXT );
		CG_RemoveChatEscapeChar( text );
		if (enc) {
			CG_DecodeQP(text);
		}
		CG_AddToTeamChat( text, atoi( CG_Argv( 2 ) ) );
		CG_Printf( "%s\n", text );

		return;
	}

	if (!Q_stricmp(cmd, "tchat") || (enc = !Q_stricmp(cmd, "enc_tchat"))) {
		const char *s;

		if ( atoi( CG_Argv( 3 ) ) ) {
			s = CG_LocalizeServerCommand( CG_Argv( 1 ) );
		} else {
			s = CG_Argv( 1 );
		}

		Q_strncpyz( text, s, MAX_SAY_TEXT );
		CG_RemoveChatEscapeChar( text );
		if (enc) {
			CG_DecodeQP(text);
		}
		CG_AddToTeamChat( text, atoi( CG_Argv( 2 ) ) );
		CG_Printf( "%s\n", text ); // JPW NERVE

		return;
	}

	if ( !Q_stricmp( cmd, "vchat" ) ) {
		CG_VoiceChat( SAY_ALL );            // NERVE - SMF - enabled support
		return;
	}

	if ( !Q_stricmp( cmd, "vtchat" ) ) {
		CG_VoiceChat( SAY_TEAM );           // NERVE - SMF - enabled support
		return;
	}

	if ( !Q_stricmp( cmd, "vbchat" ) ) {
		CG_VoiceChat( SAY_BUDDY );
		return;
	}

	if ( !Q_stricmp( cmd, "map_restart" ) ) {
		CG_MapRestart();
		return;
	}

	if ( !Q_stricmp( cmd, "startCam" ) ) {
		CG_StartCamera( CG_Argv( 1 ), atoi( CG_Argv( 2 ) ) );
		return;
	}

	if ( !Q_stricmp( cmd, "SetInitialCamera" ) ) {
		CG_SetInitialCamera( CG_Argv( 1 ), atoi( CG_Argv( 2 ) ) );
		return;
	}

	if ( !Q_stricmp( cmd, "stopCam" ) ) {
		CG_StopCamera();
		return;
	}

	if ( !Q_stricmp( cmd, "setspawnpt" ) ) {
		cg.selectedSpawnPoint = atoi( CG_Argv( 1 ) ) + 1;
		return;
	}

	if ( !strcmp( cmd, "rockandroll" ) ) {   // map loaded, game is ready to begin.
		// Arnout: FIXME: re-enable when we get menus that deal with fade properly
//		CG_Fade(0, 0, 0, 255, cg.time, 0);		// go black
		//trap_UI_Popup("pregame");				// start pregame menu
		//trap_Cvar_Set("cg_norender", "1");	// don't render the world until the player clicks in and the 'playerstart' func has been called (g_main in G_UpdateCvars() ~ilne 949)

		trap_S_FadeAllSound( 1.0f, 1000, qfalse );    // fade sound up

		return;
	}

	if ( !Q_stricmp( cmd, "application" ) ) {
		cgs.applicationEndTime = cg.time + 20000;
		cgs.applicationClient = atoi( CG_Argv( 1 ) );

		if ( cgs.applicationClient < 0 ) {
			cgs.applicationEndTime = cg.time + 10000;
		}

		return;
	}

	if ( !Q_stricmp( cmd, "invitation" ) ) {
		cgs.invitationEndTime = cg.time + 20000;
		cgs.invitationClient = atoi( CG_Argv( 1 ) );

		if ( cgs.invitationClient < 0 ) {
			cgs.invitationEndTime = cg.time + 10000;
		}

		return;
	}

	if ( !Q_stricmp( cmd, "proposition" ) ) {
		cgs.propositionEndTime = cg.time + 20000;
		cgs.propositionClient = atoi( CG_Argv( 1 ) );
		cgs.propositionClient2 = atoi( CG_Argv( 2 ) );

		if ( cgs.propositionClient < 0 ) {
			cgs.propositionEndTime = cg.time + 10000;
		}

		return;
	}

	if ( Q_stricmp( cmd, "remapShader" ) == 0 ) {
		if ( trap_Argc() == 4 ) {
			trap_R_RemapShader( CG_Argv( 1 ), CG_Argv( 2 ), CG_Argv( 3 ) );
		}
	}

//GS Copied in code from old source for mu_start, mu_play & mu_stop
	//
	// music
	//

	// loops \/
	if ( !Q_stricmp( cmd, "mu_start" ) ) {   // has optional parameter for fade-up time
		int fadeTime = 0;   // default to instant start

		Q_strncpyz( text, CG_Argv( 2 ), MAX_SAY_TEXT );
		// Nico, always true warning fix
		if ( text != NULL && strlen( text ) ) {
			fadeTime = atoi( text );
		}

		trap_S_StartBackgroundTrack( CG_Argv( 1 ), CG_Argv( 1 ), fadeTime );
		return;
	}
	// plays once then back to whatever the loop was \/
	if ( !Q_stricmp( cmd, "mu_play" ) ) {    // has optional parameter for fade-up time
		int fadeTime = 0;   // default to instant start

		Q_strncpyz( text, CG_Argv( 2 ), MAX_SAY_TEXT );
		// Nico, always true warning fix
		if ( text != NULL && strlen( text ) ) {
			fadeTime = atoi( text );
		}

		trap_S_StartBackgroundTrack( CG_Argv( 1 ), "onetimeonly", fadeTime );
		return;
	}

	if ( !Q_stricmp( cmd, "mu_stop" ) ) {    // has optional parameter for fade-down time
		int fadeTime = 0;   // default to instant stop

		Q_strncpyz( text, CG_Argv( 1 ), MAX_SAY_TEXT );
		// Nico, always true warning fix
		if ( text != NULL && strlen( text ) ) {
			fadeTime = atoi( text );
		}

		trap_S_FadeBackgroundTrack( 0.0f, fadeTime, 0 );
		trap_S_StartBackgroundTrack( "", "", -2 ); // '-2' for 'queue looping track' (QUEUED_PLAY_LOOPED)
		return;
	}
	if ( !Q_stricmp( cmd, "mu_fade" ) ) {
		trap_S_FadeBackgroundTrack( atof( CG_Argv( 1 ) ), atoi( CG_Argv( 2 ) ), 0 );
		return;
	}

	if ( !Q_stricmp( cmd, "snd_fade" ) ) {
		trap_S_FadeAllSound( atof( CG_Argv( 1 ) ), atoi( CG_Argv( 2 ) ), atoi( CG_Argv( 3 ) ) );
		return;
	}

	if ( !Q_stricmp( cmd, "ftCommands" ) ) {
		char info[MAX_INFO_STRING];
		trap_Argv( 1, info, sizeof( info ) );

		return;
	}

	// ensure a file gets into a build (mainly for scripted music calls)
	if ( !Q_stricmp( cmd, "addToBuild" ) ) {
		fileHandle_t f;

		if ( !cg_buildScript.integer ) {
			return;
		}

		// just open the file so it gets copied to the build dir
		//CG_FileTouchForBuild(CG_Argv(1));
		trap_FS_FOpenFile( CG_Argv( 1 ), &f, FS_READ );
		trap_FS_FCloseFile( f );
		return;
	}

	// ydnar: bug 267: server sends this command when it's about to kill the current server, before the client can reconnect
	if ( !Q_stricmp( cmd, "spawnserver" ) ) {
		// print message informing player the server is restarting with a new map
		CG_PriorityCenterPrint( va( "%s", CG_TranslateString( "^3Server Restarting" ) ), SCREEN_HEIGHT - ( SCREEN_HEIGHT * 0.25 ), SMALLCHAR_WIDTH, 999999 );

		// hack here
		cg.serverRespawning = qtrue;

		// fade out over the course of 5 seconds, should be enough (nuking: atvi bug 3793)
		//%	CG_Fade( 0, 0, 0, 255, cg.time, 5000 );

		return;
	}

	// Nico, pmove_fixed
	if (!Q_stricmp(cmd, "pmoveon")) {
		trap_SendConsoleCommand("set pmove_fixed 1\n");
		CG_AddPMItem(PM_MESSAGE, "pmove_fixed has been set to 1\n", cgs.media.voiceChatShader);
		return;
	}

	// Nico, force max FPS
	if (!Q_stricmp(cmd, "resetMaxFPS")) {
		trap_SendConsoleCommand("set com_maxfps 125\n");
		CG_AddPMItem(PM_MESSAGE, "com_maxfps has been set to 125\n", cgs.media.voiceChatShader);
		return;
	}
	
	// Nico, timer start/stop/check related

	if (!Q_stricmp(cmd, "timerun_start")) {
		cg.timerunActive = 1;
		cg.timerunCheckPointChecked = 0;
		cg.currentTimerun = atoi(CG_Argv(1));
		cg.timerunStartTime = atoi(CG_Argv(2));

		if (!cg.timerunBestTime[cg.currentTimerun] || cg.timerunLastTime[cg.currentTimerun] < cg.timerunBestTime[cg.currentTimerun]) {
			cg.timerunBestTime[cg.currentTimerun] = cg.timerunLastTime[cg.currentTimerun];
		}
		return;
	}

	if (!Q_stricmp(cmd, "timerun_start_spec")) {
		// Nico, only for specs
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR) {
			return;
		}
		cg.timerunActive = 1;
		cg.timerunCheckPointChecked = 0;
		cg.currentTimerun = atoi(CG_Argv(1));
		cg.timerunStartTime = atoi(CG_Argv(2));

		if (!cg.timerunBestTime[cg.currentTimerun] || cg.timerunLastTime[cg.currentTimerun] < cg.timerunBestTime[cg.currentTimerun]) {
			cg.timerunBestTime[cg.currentTimerun] = cg.timerunLastTime[cg.currentTimerun];
		}
		return;
	}

	if (!Q_stricmp(cmd, "timerun_check")) {
		cg.timerunCheckPointDiff[cg.timerunCheckPointChecked] = atoi(CG_Argv(1));
		cg.timerunCheckPointTime[cg.timerunCheckPointChecked] = atoi(CG_Argv(2));
		cg.timerunCheckStatus[cg.timerunCheckPointChecked++] = atoi(CG_Argv(3));

		if (cg.timerunCheckPointChecked > MAX_TIMERUN_CHECKPOINTS) {
			CG_Error("MAX_TIMERUN_CHECKPOINTS limit reached\n");
		}
		return;
	}

	if (!Q_stricmp(cmd, "timerun_check_spec")) {
		// Nico, only for specs
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR) {
			return;
		}
		cg.timerunCheckPointDiff[cg.timerunCheckPointChecked] = atoi(CG_Argv(1));
		cg.timerunCheckPointTime[cg.timerunCheckPointChecked] = atoi(CG_Argv(2));
		cg.timerunCheckStatus[cg.timerunCheckPointChecked++] = atoi(CG_Argv(3));

		if (cg.timerunCheckPointChecked > MAX_TIMERUN_CHECKPOINTS) {
			CG_Error("MAX_TIMERUN_CHECKPOINTS limit reached\n");
		}
		return;
	}

	if (!Q_stricmp(cmd, "timerun_stop")) {
		cg.timerunActive = 0;

		if (atoi(CG_Argv(2))) {
			cg.timerunLastTime[atoi(CG_Argv(1))] = cg.timerunFinishedTime = atoi(CG_Argv(2));
		}
		return;
	}

	if (!Q_stricmp(cmd, "timerun_stop_spec")) {
		// Nico, only for specs
		if (cgs.clientinfo[cg.clientNum].team != TEAM_SPECTATOR) {
			return;
		}
		cg.timerunActive = 0;

		if (atoi(CG_Argv(2))) {
			cg.timerunLastTime[atoi(CG_Argv(1))] = cg.timerunFinishedTime = atoi(CG_Argv(2));
		}
		return;
	}
	// Nico, end of timer start/stop/check related

	// Nico, auto demo related
	if (!Q_stricmp(cmd, "runSave"))  {
		if (!cg_autoDemo.integer || cg.demoPlayback) {
			return;
		}

		cg.startedNewDemo = 0;
		cg.runsave = 1;
		Com_sprintf(cg.runsavename, sizeof (cg.runsavename), "%s", CG_Argv(1));
		cg.currentdemo = currentdemo;
		return;
	}

	if (!Q_stricmp(cmd, "tempDemoStart")) {
		if (!cg_autoDemo.integer || cg.demoPlayback) {
			return;
		}

		if (currentdemo > 20) {
			currentdemo = 1;
		}

		// Sent from stoptimer, do a 1 sec delay.
		if (trap_Argc() > 1) {
			currentdemo++;
			cg.startedNewDemo = currentdemo+1;
			ignoreNextStart = 1;
		} else if (!ignoreNextStart) {
			currentdemo++;
			cg.startedNewDemo = 1;
			trap_SendConsoleCommand(va("stoprecord\n"));
			trap_SendConsoleCommand(va("record temp_%i\n", currentdemo));
			ignoreNextStart = 1;
		} else {
			ignoreNextStart = 0;
		}
		return;
	}
	// Nico, end of auto demo related

	// Nico, banner printing
	if (!Q_stricmp(cmd, "bp")) {
		CG_BannerPrint(CG_LocalizeServerCommand(CG_Argv(1)));
		return;
	}
	// Nico, end of banner printing

	// Nico, login notifications
	if (!Q_stricmp(cmd, "client_login")) {
		cg.isLogged = 1;
		return;
	}

	if (!Q_stricmp(cmd, "client_logout")) {
		cg.isLogged = 0;
		return;
	}
	// Nico, end of login notifications

	CG_Printf( "Unknown client game command: %s\n", cmd );
}


/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands( int latestSequence ) {
	while ( cgs.serverCommandSequence < latestSequence ) {
		if ( trap_GetServerCommand( ++cgs.serverCommandSequence ) ) {
			CG_ServerCommand();
		}
	}
}

