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

#define AXIS_OBJECTIVE      1
#define ALLIED_OBJECTIVE    2
#define OBJECTIVE_DESTROYED 4
#define CONSTRUCTIBLE_START_BUILT   1
#define CONSTRUCTIBLE_INVULNERABLE  2
#define AXIS_CONSTRUCTIBLE          4
#define ALLIED_CONSTRUCTIBLE        8
#define CONSTRUCTIBLE_BLOCK_PATHS_WHEN_BUILD    16
#define CONSTRUCTIBLE_NO_AAS_BLOCKING           32
#define EXPLOSIVE_START_INVIS       1
#define EXPLOSIVE_TOUCHABLE         2
#define EXPLOSIVE_USESHADER         4
#define EXPLOSIVE_LOWGRAV           8
#define EXPLOSIVE_TANK              32

// Prototypes
const char *TeamName(int team);
void Team_DroppedFlagThink(gentity_t *ent);
gentity_t *SelectRandomTeamSpawnPoint(team_t team, int spawnObjective);
void Team_ReturnFlag(gentity_t *ent);
void TeamplayInfoMessage(team_t team);
void CheckTeamStatus(void);
int Pickup_Team(gentity_t *ent, gentity_t *other);
