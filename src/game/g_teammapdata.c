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

#include "g_local.h"

/*
===================
G_InitMapEntityData
===================
*/
void G_InitMapEntityData(mapEntityData_Team_t *teamList) {
	int             i;
	mapEntityData_t *trav, *lasttrav;

	memset(teamList, 0, sizeof (mapEntityData_Team_t));

	teamList->activeMapEntityData.next = &teamList->activeMapEntityData;
	teamList->activeMapEntityData.prev = &teamList->activeMapEntityData;
	teamList->freeMapEntityData        = teamList->mapEntityData_Team;

	for (i = 0, trav = teamList->mapEntityData_Team + 1, lasttrav = teamList->mapEntityData_Team ; i < MAX_GENTITIES - 1 ; ++i, ++trav) {
		lasttrav->next = trav;
		lasttrav       = trav;
	}
}

/*
==================
G_FreeMapEntityData

returns next entity in the array
==================
*/
mapEntityData_t *G_FreeMapEntityData(mapEntityData_Team_t *teamList, mapEntityData_t *mEnt) {
	mapEntityData_t *ret = mEnt->next;

	if (!mEnt->prev) {
		G_Error("G_FreeMapEntityData: not active");
	}

	// remove from the doubly linked active list
	mEnt->prev->next = mEnt->next;
	mEnt->next->prev = mEnt->prev;

	// the free list is only singly linked
	mEnt->next                  = teamList->freeMapEntityData;
	teamList->freeMapEntityData = mEnt;

	return ret;
}

/*
===================
G_AllocMapEntityData
===================
*/
mapEntityData_t *G_AllocMapEntityData(mapEntityData_Team_t *teamList) {
	mapEntityData_t *mEnt;

	if (!teamList->freeMapEntityData) {
		// no free entities - bomb out
		G_Error("G_AllocMapEntityData: out of entities");
	}

	mEnt                        = teamList->freeMapEntityData;
	teamList->freeMapEntityData = teamList->freeMapEntityData->next;

	memset(mEnt, 0, sizeof (*mEnt));

	mEnt->singleClient = -1;

	// link into the active list
	mEnt->next                               = teamList->activeMapEntityData.next;
	mEnt->prev                               = &teamList->activeMapEntityData;
	teamList->activeMapEntityData.next->prev = mEnt;
	teamList->activeMapEntityData.next       = mEnt;
	return mEnt;
}

/*
===================
G_FindMapEntityData
===================
*/
mapEntityData_t *G_FindMapEntityData(mapEntityData_Team_t *teamList, int entNum) {
	mapEntityData_t *mEnt;

	for (mEnt = teamList->activeMapEntityData.next; mEnt && mEnt != &teamList->activeMapEntityData; mEnt = mEnt->next) {
		if (mEnt->singleClient >= 0) {
			continue;
		}
		if (entNum == mEnt->entNum) {
			return mEnt;
		}
	}

	// not found
	return NULL;
}

/*
===============================
G_FindMapEntityDataSingleClient
===============================
*/
mapEntityData_t *G_FindMapEntityDataSingleClient(mapEntityData_Team_t *teamList, mapEntityData_t *start, int entNum, int clientNum) {
	mapEntityData_t *mEnt;

	if (start) {
		mEnt = start->next;
	} else {
		mEnt = teamList->activeMapEntityData.next;
	}

	for ( ; mEnt && mEnt != &teamList->activeMapEntityData; mEnt = mEnt->next) {
		if (clientNum == -1) {
			if (mEnt->singleClient < 0) {
				continue;
			}
		} else if (mEnt->singleClient >= 0 && clientNum != mEnt->singleClient) {
			continue;
		}
		if (entNum == mEnt->entNum) {
			return mEnt;
		}
	}

	// not found
	return NULL;
}

////////////////////////////////////////////////////////////////////

// some culling bits
typedef struct plane_s {
	vec3_t normal;
	float dist;
} plane_t;

static plane_t frustum[4];

/*
========================
G_SetupFrustum
========================
*/
void G_SetupFrustum(gentity_t *ent) {
	int    i;
	float  xs, xc;
	float  ang;
	vec3_t axis[3];
	vec3_t vieworg;

	ang = (90 / 180.f) * M_PI * 0.5f;
	xs  = sin(ang);
	xc  = cos(ang);

	AnglesToAxis(ent->client->ps.viewangles, axis);

	VectorScale(axis[0], xs, frustum[0].normal);
	VectorMA(frustum[0].normal, xc, axis[1], frustum[0].normal);

	VectorScale(axis[0], xs, frustum[1].normal);
	VectorMA(frustum[1].normal, -xc, axis[1], frustum[1].normal);

	ang = (90 / 180.f) * M_PI * 0.5f;
	xs  = sin(ang);
	xc  = cos(ang);

	VectorScale(axis[0], xs, frustum[2].normal);
	VectorMA(frustum[2].normal, xc, axis[2], frustum[2].normal);

	VectorScale(axis[0], xs, frustum[3].normal);
	VectorMA(frustum[3].normal, -xc, axis[2], frustum[3].normal);

	VectorCopy(ent->client->ps.origin, vieworg);
	vieworg[2] += ent->client->ps.viewheight;

	for (i = 0 ; i < 4 ; ++i) {
		frustum[i].dist = DotProduct(vieworg, frustum[i].normal);
	}
}

void G_SetupFrustum_ForBinoculars(gentity_t *ent) {
#define BINOCULAR_ANGLE 10.0f
	int    i;
	float  xs, xc;
	float  ang;
	vec3_t axis[3];
	vec3_t vieworg;
	float  baseAngle = 0;

	baseAngle = BINOCULAR_ANGLE;

	ang = (baseAngle / 180.f) * M_PI * 0.5f;
	xs  = sin(ang);
	xc  = cos(ang);

	AnglesToAxis(ent->client->ps.viewangles, axis);

	VectorScale(axis[0], xs, frustum[0].normal);
	VectorMA(frustum[0].normal, xc, axis[1], frustum[0].normal);

	VectorScale(axis[0], xs, frustum[1].normal);
	VectorMA(frustum[1].normal, -xc, axis[1], frustum[1].normal);

	ang = (baseAngle / 180.f) * M_PI * 0.5f;
	xs  = sin(ang);
	xc  = cos(ang);

	VectorScale(axis[0], xs, frustum[2].normal);
	VectorMA(frustum[2].normal, xc, axis[2], frustum[2].normal);

	VectorScale(axis[0], xs, frustum[3].normal);
	VectorMA(frustum[3].normal, -xc, axis[2], frustum[3].normal);

	VectorCopy(ent->client->ps.origin, vieworg);
	vieworg[2] += ent->client->ps.viewheight;

	for (i = 0 ; i < 4 ; ++i) {
		frustum[i].dist = DotProduct(vieworg, frustum[i].normal);
	}
}

/*
========================
G_CullPointAndRadius - returns true if not culled
========================
*/
static qboolean G_CullPointAndRadius(vec3_t pt, float radius) {
	int i;

	// check against frustum planes
	for (i = 0 ; i < 4 ; ++i) {
		plane_t *frust = &frustum[i];
		float   dist;

		dist = DotProduct(pt, frust->normal) - frust->dist;
		if (dist < -radius || dist <= radius) {
			return qfalse;
		}
	}

	return qtrue;
}

qboolean G_VisibleFromBinoculars(gentity_t *viewer, gentity_t *ent, vec3_t origin) {
	vec3_t  vieworg;
	trace_t trace;

	VectorCopy(viewer->client->ps.origin, vieworg);
	vieworg[2] += viewer->client->ps.viewheight;

	if (!G_CullPointAndRadius(origin, 0)) {
		return qfalse;
	}

	if (!trap_InPVS(vieworg, origin)) {
		return qfalse;
	}

	trap_Trace(&trace, vieworg, NULL, NULL, origin, viewer->s.number, MASK_SHOT);

	if (trace.fraction != 1.f) {
		if (ent) {
			if (trace.entityNum != ent->s.number) {
				return qfalse;
			}
			return qtrue;
		}
		return qfalse;
	}

	return qtrue;
}

void G_ResetTeamMapData(void) {
	G_InitMapEntityData(&mapEntityData[0]);
	G_InitMapEntityData(&mapEntityData[1]);
}

void G_UpdateTeamMapData_Construct(gentity_t *ent) {
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	if (ent->s.teamNum == 3) {
		teamList = &mapEntityData[0];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.pos.trBase, mEnt->org);
		mEnt->data      = mEnt->entNum;
		mEnt->type      = ME_CONSTRUCT;
		mEnt->startTime = level.time;
		mEnt->yaw       = 0;

		teamList = &mapEntityData[1];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.pos.trBase, mEnt->org);
		mEnt->data      = mEnt->entNum;
		mEnt->type      = ME_CONSTRUCT;
		mEnt->startTime = level.time;
		mEnt->yaw       = 0;

		return;
	}

	if (ent->s.teamNum == TEAM_AXIS) {
		teamList = &mapEntityData[0];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.pos.trBase, mEnt->org);
		mEnt->data      = mEnt->entNum;
		mEnt->type      = ME_CONSTRUCT;
		mEnt->startTime = level.time;
		mEnt->yaw       = 0;
	}

	if (ent->s.teamNum == TEAM_ALLIES) {
		teamList = &mapEntityData[1];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.pos.trBase, mEnt->org);
		mEnt->data      = mEnt->entNum;
		mEnt->type      = ME_CONSTRUCT;
		mEnt->startTime = level.time;
		mEnt->yaw       = 0;
	}
}

void G_UpdateTeamMapData_Tank(gentity_t *ent) {
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	teamList = &mapEntityData[0];
	mEnt     = G_FindMapEntityData(teamList, num);
	if (!mEnt) {
		mEnt         = G_AllocMapEntityData(teamList);
		mEnt->entNum = num;
	}
	VectorCopy(ent->s.pos.trBase, mEnt->org);
	mEnt->data      = ent->s.modelindex2;
	mEnt->startTime = level.time;
	if (ent->s.eType == ET_TANK_INDICATOR_DEAD) {
		mEnt->type = ME_TANK_DEAD;
	} else {
		mEnt->type = ME_TANK;
	}
	mEnt->yaw = 0;

	teamList = &mapEntityData[1];
	mEnt     = G_FindMapEntityData(teamList, num);
	if (!mEnt) {
		mEnt         = G_AllocMapEntityData(teamList);
		mEnt->entNum = num;
	}
	VectorCopy(ent->s.pos.trBase, mEnt->org);
	mEnt->data      = ent->s.modelindex2;
	mEnt->startTime = level.time;
	if (ent->s.eType == ET_TANK_INDICATOR_DEAD) {
		mEnt->type = ME_TANK_DEAD;
	} else {
		mEnt->type = ME_TANK;
	}
	mEnt->yaw = 0;
}

void G_UpdateTeamMapData_Destruct(gentity_t *ent) {
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	if (ent->s.teamNum == TEAM_AXIS) {
		teamList = &mapEntityData[1];   // inverted
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.pos.trBase, mEnt->org);
		mEnt->data      = mEnt->entNum;
		mEnt->startTime = level.time;
		mEnt->type      = ME_DESTRUCT;
		mEnt->yaw       = 0;
	} else {
		if (ent->parent->target_ent &&
		    (ent->parent->target_ent->s.eType == ET_CONSTRUCTIBLE || ent->parent->target_ent->s.eType == ET_EXPLOSIVE) &&
		    (ent->parent->spawnflags & ((1 << 6) | (1 << 4)))) {
			teamList = &mapEntityData[1];   // inverted
			mEnt     = G_FindMapEntityData(teamList, num);
			if (!mEnt) {
				mEnt         = G_AllocMapEntityData(teamList);
				mEnt->entNum = num;
			}
			VectorCopy(ent->s.pos.trBase, mEnt->org);
			mEnt->data      = mEnt->entNum;
			mEnt->startTime = level.time;
			mEnt->type      = ME_DESTRUCT_2;
			mEnt->yaw       = 0;
		}
	}

	if (ent->s.teamNum == TEAM_ALLIES) {
		teamList = &mapEntityData[0];   // inverted
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.pos.trBase, mEnt->org);
		mEnt->data      = mEnt->entNum;
		mEnt->startTime = level.time;
		mEnt->type      = ME_DESTRUCT;
		mEnt->yaw       = 0;
	} else {
		if (ent->parent->target_ent &&
		    (ent->parent->target_ent->s.eType == ET_CONSTRUCTIBLE || ent->parent->target_ent->s.eType == ET_EXPLOSIVE) &&
		    (ent->parent->spawnflags & ((1 << 6) | (1 << 4)))) {
			teamList = &mapEntityData[0];   // inverted
			mEnt     = G_FindMapEntityData(teamList, num);
			if (!mEnt) {
				mEnt         = G_AllocMapEntityData(teamList);
				mEnt->entNum = num;
			}
			VectorCopy(ent->s.pos.trBase, mEnt->org);
			mEnt->data      = mEnt->entNum;
			mEnt->startTime = level.time;
			mEnt->type      = ME_DESTRUCT_2;
			mEnt->yaw       = 0;
		}
	}
}

void G_UpdateTeamMapData_Player(gentity_t *ent, qboolean forceAllied, qboolean forceAxis) {
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	if (ent->client) {
		switch (ent->client->sess.sessionTeam) {
		case TEAM_AXIS:
			forceAxis = qtrue;
			break;

		case TEAM_ALLIES:
			forceAllied = qtrue;
			break;

		default:
			break;
		}
	}

	if (forceAxis && ent->client && !(ent->client->ps.pm_flags & PMF_LIMBO) /*ent->health > 0*/) {
		teamList = &mapEntityData[0];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->client->ps.origin, mEnt->org);
		mEnt->yaw       = ent->client->ps.viewangles[YAW];
		mEnt->data      = num;
		mEnt->startTime = level.time;

		if (ent->health <= 0) {
			mEnt->type = ME_PLAYER_REVIVE;
		} else {
			mEnt->type = ME_PLAYER;
		}
	}

	if (forceAllied && ent->client && !(ent->client->ps.pm_flags & PMF_LIMBO) /*ent->health > 0*/) {
		teamList = &mapEntityData[1];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}

		VectorCopy(ent->client->ps.origin, mEnt->org);
		mEnt->yaw       = ent->client->ps.viewangles[YAW];
		mEnt->data      = num;
		mEnt->startTime = level.time;
		if (ent->health <= 0) {
			mEnt->type = ME_PLAYER_REVIVE;
		} else {
			mEnt->type = ME_PLAYER;
		}
	}
}

void G_UpdateTeamMapData_CommandmapMarker(gentity_t *ent) {
	int                  num = ent - g_entities;
	mapEntityData_Team_t *teamList;
	mapEntityData_t      *mEnt;

	if (!ent->parent) {
		return;
	}

	if (ent->entstate != STATE_DEFAULT) {
		return;
	}

	if (ent->parent->spawnflags & ALLIED_OBJECTIVE) {
		teamList = &mapEntityData[0];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.origin, mEnt->org);
		mEnt->data      = ent->parent->s.teamNum; //-V595
		mEnt->startTime = level.time;
		mEnt->type      = ME_COMMANDMAP_MARKER;
		mEnt->yaw       = 0;
	}

	if (ent->parent->spawnflags & AXIS_OBJECTIVE) {
		teamList = &mapEntityData[1];
		mEnt     = G_FindMapEntityData(teamList, num);
		if (!mEnt) {
			mEnt         = G_AllocMapEntityData(teamList);
			mEnt->entNum = num;
		}
		VectorCopy(ent->s.origin, mEnt->org);
		mEnt->data      = ent->parent ? ent->parent->s.teamNum : -1;
		mEnt->startTime = level.time;
		mEnt->type      = ME_COMMANDMAP_MARKER;
		mEnt->yaw       = 0;
	}
}

void G_UpdateTeamMapData(void) {
	int             i, j;
	gentity_t       *ent, *ent2;
	mapEntityData_t *mEnt;

	if (level.time - level.lastMapEntityUpdate < 500) {
		return;
	}
	level.lastMapEntityUpdate = level.time;

	for (i = 0, ent = g_entities; i < level.num_entities; ++i, ++ent) {
		if (!ent->inuse) {
			continue;
		}

		switch (ent->s.eType) {
		case ET_PLAYER:
			G_UpdateTeamMapData_Player(ent, qfalse, qfalse);
			for (j = 0; j < 2; ++j) {
				mapEntityData_Team_t *teamList = &mapEntityData[j];

				mEnt = G_FindMapEntityDataSingleClient(teamList, NULL, ent->s.number, -1);

				while (mEnt) {
					VectorCopy(ent->client->ps.origin, mEnt->org);
					mEnt->yaw = ent->client->ps.viewangles[YAW];
					mEnt      = G_FindMapEntityDataSingleClient(teamList, mEnt, ent->s.number, -1);
				}
			}
			break;
		case ET_CONSTRUCTIBLE_INDICATOR:
			if (ent->parent && ent->parent->entstate == STATE_DEFAULT) {
				G_UpdateTeamMapData_Construct(ent);
			}
			break;
		case ET_EXPLOSIVE_INDICATOR:
			if (ent->parent && ent->parent->entstate == STATE_DEFAULT) {
				G_UpdateTeamMapData_Destruct(ent);
			}
			break;
		case ET_TANK_INDICATOR:
		case ET_TANK_INDICATOR_DEAD:
			G_UpdateTeamMapData_Tank(ent);
			break;
		case ET_MISSILE:
			break;
		case ET_COMMANDMAP_MARKER:
			G_UpdateTeamMapData_CommandmapMarker(ent);
			break;
		default:
			break;
		}
	}

	for (i = 0, ent = g_entities; i < level.num_entities; ++i, ++ent) {
		if (!ent->inuse || !ent->client) {
			continue;
		}
		if (ent->client->sess.playerType == PC_COVERTOPS && ent->health > 0) {
			qboolean f1 = ent->client->sess.sessionTeam == TEAM_ALLIES ? qtrue : qfalse;
			qboolean f2 = ent->client->sess.sessionTeam == TEAM_AXIS ?   qtrue : qfalse;

			G_SetupFrustum(ent);

			for (j = 0, ent2 = g_entities; j < level.num_entities; ++j, ++ent2) {
				if (!ent2->inuse || ent2 == ent) {
					continue;
				}

				switch (ent2->s.eType) {
				case ET_PLAYER:
				{
					vec3_t pos[3];
					VectorCopy(ent2->client->ps.origin, pos[0]);
					pos[0][2] += ent2->client->ps.mins[2];
					VectorCopy(ent2->client->ps.origin, pos[1]);
					VectorCopy(ent2->client->ps.origin, pos[2]);
					pos[2][2] += ent2->client->ps.maxs[2];
					if (ent2->health > 0 && (G_VisibleFromBinoculars(ent, ent2, pos[0]) ||
					                         G_VisibleFromBinoculars(ent, ent2, pos[1]) ||
					                         G_VisibleFromBinoculars(ent, ent2, pos[2]))) {
						if (ent2->client->sess.sessionTeam != ent->client->sess.sessionTeam) {
							int k;

							switch (ent2->client->sess.sessionTeam) {
							case TEAM_AXIS:
								mEnt = G_FindMapEntityData(&mapEntityData[0], ent2 - g_entities);
								if (mEnt && level.time - mEnt->startTime > 5000) {
									for (k = 0; k < MAX_CLIENTS; ++k) {
										if (g_entities[k].inuse && g_entities[k].client && g_entities[k].client->sess.sessionTeam == ent->client->sess.sessionTeam) {
											trap_SendServerCommand(k, va("tt \"ENEMY SPOTTED <STOP> CHECK COMMAND MAP FOR DETAILS <STOP>\"\n"));
										}
									}
								}
								break;

							case TEAM_ALLIES:
								mEnt = G_FindMapEntityData(&mapEntityData[1], ent2 - g_entities);
								if (mEnt && level.time - mEnt->startTime > 5000) {
									for (k = 0; k < MAX_CLIENTS; ++k) {
										if (g_entities[k].inuse && g_entities[k].client && g_entities[k].client->sess.sessionTeam == ent->client->sess.sessionTeam) {
											trap_SendServerCommand(k, va("tt \"ENEMY SPOTTED <STOP> CHECK COMMAND MAP FOR DETAILS <STOP>\"\n"));
										}
									}
								}
								break;

							default:
								break;
							}
						}

						G_UpdateTeamMapData_Player(ent2, f1, f2);
					}
					break;
				}
				default:
					break;
				}
			}

			if (ent->client->ps.eFlags & EF_ZOOMING) {
				G_SetupFrustum_ForBinoculars(ent);

				for (j = 0, ent2 = g_entities; j < level.num_entities; ++j, ++ent2) {
					if (!ent2->inuse || ent2 == ent) {
						continue;
					}

					switch (ent2->s.eType) {
					case ET_MISSILE:
						break;
					default:
						break;
					}
				}
			}
		}
	}
}
