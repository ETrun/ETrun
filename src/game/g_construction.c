/*
 * name:		g_construction.c
 *
 * desc:		construction related function moved from botai
 *
*/
#include "g_local.h"

// returns qtrue if a construction is under way on this ent, even before it hits any stages
qboolean G_ConstructionBegun( gentity_t* ent ) {
	if( G_ConstructionIsPartlyBuilt( ent ) ) {
		return qtrue;
	}
	
	if( ent->s.angles2[0] ) {
		return qtrue;
	}

	return qfalse;
}

// returns qtrue if all stage are built
qboolean G_ConstructionIsFullyBuilt( gentity_t* ent ) {
	if( ent->s.angles2[1] != 1 ) {
		return qfalse;
	}
	return qtrue;
}

// returns qtrue if 1 stage or more is built
qboolean G_ConstructionIsPartlyBuilt( gentity_t* ent ) {
	if( G_ConstructionIsFullyBuilt( ent ) ) {
		return qtrue;
	}

	if( ent->count2 ) {
		if( !ent->grenadeFired ) {
			return qfalse;
		} else {
			return qtrue;
		}
	}

	return qfalse;
}

// returns the constructible for this team that is attached to this toi
gentity_t* G_ConstructionForTeam( gentity_t* toi, team_t team ) {
	gentity_t* targ = toi->target_ent;
	if(!targ || targ->s.eType != ET_CONSTRUCTIBLE) {
		return NULL;
	}

	if( targ->spawnflags & 4 ) {
		if( team == TEAM_ALLIES ) {
			return targ->chain;
		}
	} else if( targ->spawnflags & 8 ) {
		if( team == TEAM_AXIS ) {
			return targ->chain;
		}
	}

	return targ;
}

gentity_t* G_IsConstructible( team_t team, gentity_t* toi ) {
	gentity_t* ent;

	if( !toi || toi->s.eType != ET_OID_TRIGGER ) { 
		return NULL;
	}

	if( !(ent = G_ConstructionForTeam( toi, team )) ) {
		return NULL;
	}

	if( G_ConstructionIsFullyBuilt( ent ) ) {
		return NULL;
	}

	if( ent->chain && G_ConstructionBegun( ent->chain ) ) {
		return NULL;
	}

	return ent;
}
