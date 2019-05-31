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

#include "q_shared.h"
#include "bg_public.h"

bg_playerclass_t bg_allies_playerclasses[NUM_PLAYER_CLASSES] =
{
	{
		PC_SOLDIER,
		"characters/temperate/allied/soldier.char",
		"ui/assets/mp_gun_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_THOMPSON,
			WP_MOBILE_MG42,
			WP_FLAMETHROWER,
			WP_PANZERFAUST,
			WP_MORTAR
		},
		0,
		0
	},

	{
		PC_MEDIC,
		"characters/temperate/allied/medic.char",
		"ui/assets/mp_health_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_THOMPSON,
		},
		0,
		0
	},

	{
		PC_ENGINEER,
		"characters/temperate/allied/engineer.char",
		"ui/assets/mp_wrench_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_THOMPSON,
			WP_CARBINE,
		},
		0,
		0
	},

	{
		PC_FIELDOPS,
		"characters/temperate/allied/fieldops.char",
		"ui/assets/mp_ammo_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_THOMPSON,
		},
		0,
		0
	},

	{
		PC_COVERTOPS,
		"characters/temperate/allied/cvops.char",
		"ui/assets/mp_spy_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_STEN,
			WP_FG42,
			WP_GARAND,
		},
		0,
		0
	},
};

bg_playerclass_t bg_axis_playerclasses[NUM_PLAYER_CLASSES] =
{
	{
		PC_SOLDIER,
		"characters/temperate/axis/soldier.char",
		"ui/assets/mp_gun_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_MP40,
			WP_MOBILE_MG42,
			WP_FLAMETHROWER,
			WP_PANZERFAUST,
			WP_MORTAR
		},
		0,
		0
	},

	{
		PC_MEDIC,
		"characters/temperate/axis/medic.char",
		"ui/assets/mp_health_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_MP40,
		},
		0,
		0
	},

	{
		PC_ENGINEER,
		"characters/temperate/axis/engineer.char",
		"ui/assets/mp_wrench_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_MP40,
			WP_KAR98,
		},
		0,
		0
	},

	{
		PC_FIELDOPS,
		"characters/temperate/axis/fieldops.char",
		"ui/assets/mp_ammo_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_MP40,
		},
		0,
		0
	},

	{
		PC_COVERTOPS,
		"characters/temperate/axis/cvops.char",
		"ui/assets/mp_spy_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_STEN,
			WP_FG42,
			WP_K43,
		},
		0,
		0
	},
};

bg_playerclass_t *BG_GetPlayerClassInfo(int team, int cls) {
	bg_playerclass_t *teamList;

	if (cls < PC_SOLDIER || cls >= NUM_PLAYER_CLASSES) {
		cls = PC_SOLDIER;
	}

	switch (team) {
	default:
	case TEAM_AXIS:
		teamList = bg_axis_playerclasses;
		break;
	case TEAM_ALLIES:
		teamList = bg_allies_playerclasses;
		break;
	}

	return &teamList[cls];
}

// suburb, extended function to handle non-primary weapons too
// note: only the ones actually obtainable in etrun being handled
qboolean BG_ClassHasWeapon(int classnum, team_t team, weapon_t weapon) {
	bg_playerclass_t *classInfo;

	if (!weapon) {
		return qfalse;
	}

	if (team == TEAM_ALLIES) {
		classInfo = &bg_allies_playerclasses[classnum];
	} else if (team == TEAM_AXIS) {
		classInfo = &bg_axis_playerclasses[classnum];
	} else {
		return qfalse;
	}

	// suburb, all classes got these
	if (weapon == WP_KNIFE || weapon == WP_BINOCULARS) {
		return qtrue;
	}

	// suburb, check pistols
	if ((weapon == WP_COLT && team == TEAM_ALLIES) || (weapon == WP_LUGER && team == TEAM_AXIS) ||
		(((weapon == WP_SILENCED_COLT && team == TEAM_ALLIES) || (weapon == WP_SILENCER && team == TEAM_AXIS)) && classnum == PC_COVERTOPS)) {
		return qtrue;
	}

	// suburb, check special weapons
	if ((weapon == WP_MEDKIT && classnum == PC_MEDIC) || (weapon == WP_PLIERS && classnum == PC_ENGINEER)) {
		return qtrue;
	}

	// suburb, check primary weapons
	for (int i = 0; i < MAX_WEAPS_PER_CLASS; ++i) {
		if (classInfo->classWeapons[i] == weapon) {
			return qtrue;
		}
	}
	return qfalse;
}

qboolean BG_WeaponIsPrimaryForClassAndTeam(int classnum, team_t team, weapon_t weapon) {
	if (BG_ClassHasWeapon(classnum, team, weapon) && BG_WeaponIsPrimary(weapon)) {
		return qtrue;
	}
	return qfalse;
}

// suburb, added check to be able to extend class weapons without breaking anything
qboolean BG_WeaponIsPrimary(weapon_t weapon) {
	switch (weapon) {
	case WP_MP40:
	case WP_THOMPSON:
	case WP_MOBILE_MG42:
	case WP_FLAMETHROWER:
	case WP_PANZERFAUST:
	case WP_MORTAR:
	case WP_CARBINE:
	case WP_KAR98:
	case WP_STEN:
	case WP_FG42:
	case WP_GARAND:
	case WP_K43:
		return qtrue;
	default:
		return qfalse;
	}
	return qfalse;
}

const char *BG_ClassnameForNumber(int classNum) {
	switch (classNum) {
	case PC_SOLDIER:
		return "Soldier";
	case PC_MEDIC:
		return "Medic";
	case PC_ENGINEER:
		return "Engineer";
	case PC_FIELDOPS:
		return "Field Ops";
	case PC_COVERTOPS:
		return "Covert Ops";
	default:
		return "^1ERROR";
	}
}
