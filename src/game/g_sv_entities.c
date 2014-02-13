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
 * name:		g_sv_entities.c
 *
 * desc:		Server sided only map entities
*/

#include "g_local.h"

// TAT 11/13/2002
//		The SP uses entities that have no physical manifestation in the game, are are used simply
//		as locational indicators - seek cover spots, and ai markers, for example
//		This is an alternate system so those don't clutter up the real game entities

// how many of these entities?
#define MAX_SERVER_ENTITIES 4096

// for now, statically allocate them
g_serverEntity_t g_serverEntities[MAX_SERVER_ENTITIES];
int              numServerEntities;


// clear out all the sp entities
void InitServerEntities(void) {
	memset(g_serverEntities, 0, sizeof (g_serverEntities));
	numServerEntities = 0;
}

// TAT - create the server entities for the current map
static void CreateMapServerEntities() {
	char info[1024];
	char mapname[128];

	trap_GetServerinfo(info, sizeof (info));

	Q_strncpyz(mapname, Info_ValueForKey(info, "mapname"), sizeof (mapname));
}

// These server entities don't get to update every frame, but some of them have to set themselves up
//		after they've all been created
//		So we want to give each entity the chance to set itself up after it has been created
void InitialServerEntitySetup() {
	int              i;

	// TAT - create the server entities for the current map
	//		these are read from an additional file
	CreateMapServerEntities();

	for (i = 0; i < numServerEntities; ++i) {
		g_serverEntity_t *ent = &g_serverEntities[i];

		// if this entity is in use and has a setup function
		if (ent->inuse && ent->setup) {
			// call it
			ent->setup(ent);
		}
	}
}
