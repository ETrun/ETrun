#ifdef CGAMEDLL
	#include "../cgame/cg_local.h"
#else
	#include "q_shared.h"
	#include "bg_public.h"
#endif // CGAMEDLL

#include "bg_local.h"

// JPW NERVE -- stuck this here so it can be seen client & server side
float Com_GetFlamethrowerRange( void ) {
	return 2500; // multiplayer range is longer for balance
}
// jpw

int PM_IdleAnimForWeapon( int weapon ) {
	switch ( weapon ) {
	case WP_GPG40:
	case WP_M7:
	case WP_SATCHEL_DET:
	case WP_MORTAR_SET:
	case WP_MEDIC_ADRENALINE:
	case WP_MOBILE_MG42_SET:
		return WEAP_IDLE2;

	default:
		return WEAP_IDLE1;
	}
}

int PM_AltSwitchFromForWeapon( int weapon ) {
	switch ( weapon ) {
	default:
		return WEAP_ALTSWITCHFROM;
	}
}

int PM_AltSwitchToForWeapon( int weapon ) {
	switch ( weapon ) {
	case WP_GPG40:
	case WP_M7:
	case WP_MORTAR:
	case WP_MOBILE_MG42:
		return WEAP_ALTSWITCHFROM;

	default:
		return WEAP_ALTSWITCHTO;
	}
}

int PM_AttackAnimForWeapon( int weapon ) {
	switch ( weapon ) {
	case WP_GPG40:
	case WP_M7:
	case WP_SATCHEL_DET:
	case WP_MEDIC_ADRENALINE:
	case WP_MOBILE_MG42_SET:
		return WEAP_ATTACK2;

	default:
		return WEAP_ATTACK1;
	}
}

int PM_LastAttackAnimForWeapon( int weapon ) {
	switch ( weapon ) {
	case WP_GPG40:
	case WP_M7:
	case WP_MOBILE_MG42_SET:
		return WEAP_ATTACK2;
	case WP_MORTAR_SET:
		return WEAP_ATTACK1;

	default:
		return WEAP_ATTACK_LASTSHOT;
	}
}

int PM_ReloadAnimForWeapon( int weapon ) {
	switch ( weapon ) {
	case WP_GPG40:
	case WP_M7:
		return WEAP_RELOAD2;
	case WP_MOBILE_MG42_SET:
		return WEAP_RELOAD3;
	default:
		return WEAP_RELOAD1;
	}
}

int PM_RaiseAnimForWeapon( int weapon ) {
	switch ( weapon ) {
	case WP_GPG40:
	case WP_M7:
		return WEAP_RELOAD3;
	case WP_MOBILE_MG42_SET:
		return WEAP_DROP2;
	case WP_SATCHEL_DET:
		return WEAP_RELOAD2;

	default:
		return WEAP_RAISE;
	}
}

int PM_DropAnimForWeapon( int weapon ) {
	switch ( weapon ) {
	case WP_GPG40:
	case WP_M7:
		return WEAP_DROP2;
	case WP_SATCHEL_DET:
		return WEAP_RELOAD1;

	default:
		return WEAP_DROP;
	}
}

/*
==============
PM_StartWeaponAnim
==============
*/
static void PM_StartWeaponAnim( int anim ) {
	if ( pm->ps->pm_type >= PM_DEAD ) {
		return;
	}

	if ( pm->pmext->weapAnimTimer > 0 ) {
		return;
	}

	if ( pm->cmd.weapon == WP_NONE ) {
		return;
	}

	pm->ps->weapAnim = ( ( pm->ps->weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
}

void PM_ContinueWeaponAnim( int anim ) {
	if ( pm->cmd.weapon == WP_NONE ) {
		return;
	}

	if ( ( pm->ps->weapAnim & ~ANIM_TOGGLEBIT ) == anim ) {
		return;
	}
	if ( pm->pmext->weapAnimTimer > 0 ) {
		return;     // a high priority animation is running
	}
	PM_StartWeaponAnim( anim );
}

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity( vec3_t in, vec3_t normal, vec3_t out, float overbounce ) {
	float backoff;
	float change;
	int i;

	backoff = DotProduct( in, normal );

	if ( backoff < 0 ) {
		backoff *= overbounce;
	} else {
		backoff /= overbounce;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}


static void PM_ReloadClip( int weapon );

/*
===============
PM_BeginWeaponChange
===============
*/
void PM_BeginWeaponChange( int oldweapon, int newweapon, qboolean reload ) {    //----(SA)	modified to play 1st person alt-mode transition animations.
	int switchtime;
	qboolean altSwitchAnim = qfalse;

	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;     // don't allow weapon switch until all buttons are up
	}

	if ( newweapon <= WP_NONE || newweapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( !( COM_BitCheck( pm->ps->weapons, newweapon ) ) ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING || pm->ps->weaponstate == WEAPON_DROPPING_TORELOAD ) {
		return;
	}

	// Gordon: don't allow change during spinup
	if ( pm->ps->weaponDelay ) {
		return;
	}

	// don't allow switch if you're holding a hot potato or dynamite
	if ( pm->ps->grenadeTimeLeft > 0 ) {
		return;
	}

	pm->ps->nextWeapon = newweapon;

	switch ( newweapon ) {
	case WP_CARBINE:
	case WP_KAR98:
		if ( newweapon != weapAlts[oldweapon] ) {
			PM_AddEvent( EV_CHANGE_WEAPON );
		}
		break;
	case WP_DYNAMITE:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_SMOKE_BOMB:
		// initialize the timer on the potato you're switching to
		pm->ps->grenadeTimeLeft = 0;
		PM_AddEvent( EV_CHANGE_WEAPON );
		break;
	case WP_MORTAR_SET:
		if ( pm->ps->eFlags & EF_PRONE ) {
			return;
		}

		if ( pm->waterlevel == 3 ) {
			return;
		}
		PM_AddEvent( EV_CHANGE_WEAPON );
		break;
	default:
		//----(SA)	only play the weapon switch sound for the player
		PM_AddEvent( reload ? EV_CHANGE_WEAPON_2 : EV_CHANGE_WEAPON );
		break;
	}

	// it's an alt mode, play different anim
	if ( newweapon == weapAlts[oldweapon] ) {
		PM_StartWeaponAnim( PM_AltSwitchFromForWeapon( oldweapon ) );
	} else {
		PM_StartWeaponAnim( PM_DropAnimForWeapon( oldweapon ) );
	}

	switchtime = 250;   // dropping/raising usually takes 1/4 sec.
	// sometimes different switch times for alt weapons
	switch ( oldweapon ) {
	case WP_CARBINE:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
			if ( !pm->ps->ammoclip[newweapon] && pm->ps->ammo[newweapon] ) {
				PM_ReloadClip( newweapon );
			}
		}
		break;
	case WP_M7:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
		}
		break;
	case WP_KAR98:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
			if ( !pm->ps->ammoclip[newweapon] && pm->ps->ammo[newweapon] ) {
				PM_ReloadClip( newweapon );
			}
		}
		break;
	case WP_GPG40:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
		}
		break;
	case WP_LUGER:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
		}
		break;
	case WP_SILENCER:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 1000;
			//switchtime = 0;
			altSwitchAnim = qtrue;
		}
		break;
	case WP_COLT:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
		}
		break;
	case WP_SILENCED_COLT:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 1000;
			//switchtime = 1300;
			//switchtime = 0;
			altSwitchAnim = qtrue;
		}
		break;
	case WP_FG42:
	case WP_FG42SCOPE:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 50;        // fast
		}
		break;
	case WP_MOBILE_MG42:
		if ( newweapon == weapAlts[oldweapon] ) {
			vec3_t axis[3];

			switchtime = 0;

			VectorCopy( pml.forward, axis[0] );
			VectorCopy( pml.right, axis[2] );
			CrossProduct( axis[0], axis[2], axis[1] );
			AxisToAngles( axis, pm->pmext->mountedWeaponAngles );
		}
	case WP_MOBILE_MG42_SET:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
		}
		break;
	case WP_MORTAR:
		if ( newweapon == weapAlts[oldweapon] ) {
			vec3_t axis[3];

			switchtime = 0;

			VectorCopy( pml.forward, axis[0] );
			VectorCopy( pml.right, axis[2] );
			CrossProduct( axis[0], axis[2], axis[1] );
			AxisToAngles( axis, pm->pmext->mountedWeaponAngles );
		}
		break;
	case WP_MORTAR_SET:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
		}
		break;
	}

	// play an animation
	if ( altSwitchAnim ) {
		if ( pm->ps->eFlags & EF_PRONE ) {
			BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_UNDO_ALT_WEAPON_MODE_PRONE, qfalse, qfalse );
		} else {
			BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_UNDO_ALT_WEAPON_MODE, qfalse, qfalse );
		}
	} else {
		BG_AnimScriptEvent( pm->ps,pm->character->animModelInfo,  ANIM_ET_DROPWEAPON, qfalse, qfalse );
	}

	if ( reload ) {
		pm->ps->weaponstate = WEAPON_DROPPING_TORELOAD;
	} else {
		pm->ps->weaponstate = WEAPON_DROPPING;
	}

	pm->ps->weaponTime += switchtime;
}


/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange( void ) {
	int oldweapon, newweapon, switchtime;
	qboolean altSwitchAnim = qfalse;
	qboolean doSwitchAnim = qtrue;

	newweapon = pm->ps->nextWeapon;
//	pm->ps->nextWeapon = newweapon;
	if ( newweapon < WP_NONE || newweapon >= WP_NUM_WEAPONS ) {
		newweapon = WP_NONE;
	}

	if ( !( COM_BitCheck( pm->ps->weapons, newweapon ) ) ) {
		newweapon = WP_NONE;
	}

	oldweapon = pm->ps->weapon;

	pm->ps->weapon = newweapon;

	if ( pm->ps->weaponstate == WEAPON_DROPPING_TORELOAD ) {
		pm->ps->weaponstate = WEAPON_RAISING_TORELOAD;
	} else {
		pm->ps->weaponstate = WEAPON_RAISING;
	}

	switch ( newweapon )
	{
		// don't really care about anim since these weapons don't show in view.
		// However, need to set the animspreadscale so they are initally at worst accuracy
	case WP_K43_SCOPE:
	case WP_GARAND_SCOPE:
	case WP_FG42SCOPE:
		pm->ps->aimSpreadScale = 255;               // initially at lowest accuracy
		pm->ps->aimSpreadScaleFloat = 255.0f;       // initially at lowest accuracy
		break;
	case WP_SILENCER:
		pm->pmext->silencedSideArm |= 1;
		break;
	case WP_LUGER:
		pm->pmext->silencedSideArm &= ~1;
		break;
	case WP_SILENCED_COLT:
		pm->pmext->silencedSideArm |= 1;
		break;
	case WP_COLT:
		pm->pmext->silencedSideArm &= ~1;
		break;
	case WP_CARBINE:
		pm->pmext->silencedSideArm &= ~2;
		break;
	case WP_M7:
		pm->pmext->silencedSideArm |= 2;
		break;
	case WP_KAR98:
		pm->pmext->silencedSideArm &= ~2;
		break;
	case WP_GPG40:
		pm->pmext->silencedSideArm |= 2;
		break;
	default:
		break;
	}

	// doesn't happen too often (player switched weapons away then back very quickly)
	if ( oldweapon == newweapon ) {
		return;
	}

	// dropping/raising usually takes 1/4 sec.
	switchtime = 250;

	// sometimes different switch times for alt weapons
	switch ( newweapon ) {
	case WP_LUGER:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
			altSwitchAnim = qtrue ;
		}
		break;
	case WP_SILENCER:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 1190;
			altSwitchAnim = qtrue ;
		}
		break;
	case WP_COLT:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 0;
			altSwitchAnim = qtrue ;
		}
		break;
	case WP_SILENCED_COLT:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 1190;
			altSwitchAnim = qtrue ;
		}
		break;
	case WP_CARBINE:
		if ( newweapon == weapAlts[oldweapon] ) {
			if ( pm->ps->ammoclip[ BG_FindAmmoForWeapon( oldweapon ) ] ) {
				switchtime = 1347;
			} else {
				switchtime = 0;
				doSwitchAnim = qfalse;
			}
			altSwitchAnim = qtrue ;
		}
		break;
	case WP_M7:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 2350;
			altSwitchAnim = qtrue ;
		}
		break;
	case WP_KAR98:
		if ( newweapon == weapAlts[oldweapon] ) {
			if ( pm->ps->ammoclip[ BG_FindAmmoForWeapon( oldweapon ) ] ) {
				switchtime = 1347;
			} else {
				switchtime = 0;
				doSwitchAnim = qfalse;
			}
			altSwitchAnim = qtrue ;
		}
		break;
	case WP_GPG40:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 2350;
			altSwitchAnim = qtrue ;
		}
		break;
	case WP_FG42:
	case WP_FG42SCOPE:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 50;        // fast
		}
		break;
	case WP_MOBILE_MG42:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 1722;
		}
		break;
	case WP_MOBILE_MG42_SET:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 1250;
		}
		break;
	case WP_MORTAR:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 1000;
			altSwitchAnim = qtrue;
		}
		break;
	case WP_MORTAR_SET:
		if ( newweapon == weapAlts[oldweapon] ) {
			switchtime = 1667;
			altSwitchAnim = qtrue;
		}
		break;
	}

	pm->ps->weaponTime += switchtime;

	BG_UpdateConditionValue( pm->ps->clientNum, ANIM_COND_WEAPON, newweapon, qtrue );

	// play an animation
	if ( doSwitchAnim ) {
		if ( altSwitchAnim ) {
			if ( pm->ps->eFlags & EF_PRONE ) {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_DO_ALT_WEAPON_MODE_PRONE, qfalse, qfalse );
			} else {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_DO_ALT_WEAPON_MODE, qfalse, qfalse );
			}
		} else {
			if ( pm->ps->eFlags & EF_PRONE ) {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_RAISEWEAPONPRONE, qfalse, qfalse );
			} else {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_RAISEWEAPON, qfalse, qfalse );
			}
		}

		// alt weapon switch was played when switching away, just go into idle
		if ( weapAlts[oldweapon] == newweapon ) {
			PM_StartWeaponAnim( PM_AltSwitchToForWeapon( newweapon ) );
		} else {
			PM_StartWeaponAnim( PM_RaiseAnimForWeapon( newweapon ) );
		}
	}
}


/*
==============
PM_ReloadClip
==============
*/
static void PM_ReloadClip( int weapon ) {
	int ammoreserve, ammoclip, ammomove;

	ammoreserve = pm->ps->ammo[ BG_FindAmmoForWeapon( weapon )];
	ammoclip    = pm->ps->ammoclip[BG_FindClipForWeapon( weapon )];

	ammomove = GetAmmoTableData( weapon )->maxclip - ammoclip;

	if ( ammoreserve < ammomove ) {
		ammomove = ammoreserve;
	}

	if ( ammomove ) {
		pm->ps->ammo[ BG_FindAmmoForWeapon( weapon )] -= ammomove;
		pm->ps->ammoclip[BG_FindClipForWeapon( weapon )] += ammomove;
	}

	// reload akimbo stuff
	if ( BG_IsAkimboWeapon( weapon ) ) {
		PM_ReloadClip( BG_AkimboSidearm( weapon ) );
	}
}

/*
==============
PM_FinishWeaponReload
==============
*/

static void PM_FinishWeaponReload( void ) {
	PM_ReloadClip( pm->ps->weapon );          // move ammo into clip
	pm->ps->weaponstate = WEAPON_READY;     // ready to fire
	PM_StartWeaponAnim( PM_IdleAnimForWeapon( pm->ps->weapon ) );
}


/*
==============
PM_CheckforReload
==============
*/
void PM_CheckForReload( int weapon ) {
	qboolean autoreload;
	qboolean reloadRequested;
	int clipWeap, ammoWeap;

	if ( pm->noWeapClips ) { // no need to reload
		return;
	}

	// GPG40 and M7 don't reload
	if ( weapon == WP_GPG40 || weapon == WP_M7 ) {
		return;
	}

	// user is forcing a reload (manual reload)
	reloadRequested = (qboolean)( pm->cmd.wbuttons & WBUTTON_RELOAD );

	switch ( pm->ps->weaponstate ) {
	case WEAPON_RAISING:
	case WEAPON_RAISING_TORELOAD:
	case WEAPON_DROPPING:
	case WEAPON_DROPPING_TORELOAD:
	case WEAPON_READYING:
	case WEAPON_RELAXING:
	case WEAPON_RELOADING:
		return;
	default:
		break;
	}

	autoreload = pm->pmext->bAutoReload || !IS_AUTORELOAD_WEAPON( weapon );
	clipWeap = BG_FindClipForWeapon( weapon );
	ammoWeap = BG_FindAmmoForWeapon( weapon );

	switch ( weapon ) {
	case WP_FG42SCOPE:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
		if ( reloadRequested && pm->ps->ammo[ammoWeap] && pm->ps->ammoclip[clipWeap] < GetAmmoTableData( weapon )->maxclip ) {
			PM_BeginWeaponChange( weapon, weapAlts[weapon], !( pm->ps->ammo[ammoWeap] ) ? qfalse : qtrue );
		}
		return;
	default:
		break;
	}

	if ( pm->ps->weaponTime <= 0 ) {
		qboolean doReload = qfalse;

		if ( reloadRequested ) {
			if ( pm->ps->ammo[ammoWeap] ) {
				if ( pm->ps->ammoclip[clipWeap] < GetAmmoTableData( weapon )->maxclip ) {
					doReload = qtrue;
				}

				// akimbo should also check other weapon status
				if ( BG_IsAkimboWeapon( weapon ) ) {
					if ( pm->ps->ammoclip[BG_FindClipForWeapon( BG_AkimboSidearm( weapon ) )] < GetAmmoTableData( BG_FindClipForWeapon( BG_AkimboSidearm( weapon ) ) )->maxclip ) {
						doReload = qtrue;
					}
				}
			}
		} else if ( autoreload ) {
			if ( !pm->ps->ammoclip[clipWeap] && pm->ps->ammo[ammoWeap] ) {
				if ( BG_IsAkimboWeapon( weapon ) ) {
					if ( !pm->ps->ammoclip[BG_FindClipForWeapon( BG_AkimboSidearm( weapon ) )] ) {
						doReload = qtrue;
					}
				} else {
					doReload = qtrue;
				}
			}
		}

		if ( doReload ) {
			PM_BeginWeaponReload( weapon );
		}
	}



}

/*
==============
PM_SwitchIfEmpty
==============
*/
static void PM_SwitchIfEmpty( void ) {
	// weapon from here down will be a thrown explosive
	if ( pm->ps->weapon != WP_GRENADE_LAUNCHER &&
		 pm->ps->weapon != WP_GRENADE_PINEAPPLE &&
		 pm->ps->weapon != WP_DYNAMITE &&
		 pm->ps->weapon != WP_SMOKE_BOMB &&
		 pm->ps->weapon != WP_LANDMINE ) {
		return;
	}

	if ( pm->ps->ammoclip[ BG_FindClipForWeapon( pm->ps->weapon )] ) { // still got ammo in clip
		return;
	}

	if ( pm->ps->ammo[ BG_FindAmmoForWeapon( pm->ps->weapon )] ) { // still got ammo in reserve
		return;
	}

	// If this was the last one, remove the weapon and switch away before the player tries to fire next

	// NOTE: giving grenade ammo to a player will re-give him the weapon (if you do it through add_ammo())
	switch ( pm->ps->weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
		COM_BitClear( pm->ps->weapons, pm->ps->weapon );
		break;
	default:
		break;
	}

	PM_AddEvent( EV_NOAMMO );
}


/*
==============
PM_WeaponUseAmmo
	accounts for clips being used/not used
==============
*/
void PM_WeaponUseAmmo( int wp, int amount ) {
	int takeweapon;

	if ( pm->noWeapClips ) {
		pm->ps->ammo[ BG_FindAmmoForWeapon( wp )] -= amount;
	} else {
		takeweapon = BG_FindClipForWeapon( wp );

		if ( BG_IsAkimboWeapon( wp ) ) {
			if ( !BG_AkimboFireSequence( wp, pm->ps->ammoclip[BG_FindClipForWeapon( wp )], pm->ps->ammoclip[BG_FindClipForWeapon( BG_AkimboSidearm( wp ) )] ) ) {
				takeweapon = BG_AkimboSidearm( wp );
			}
		}

		pm->ps->ammoclip[takeweapon] -= amount;
	}
}


/*
==============
PM_WeaponAmmoAvailable
	accounts for clips being used/not used
==============
*/
int PM_WeaponAmmoAvailable( int wp ) {
	int takeweapon;

	if ( pm->noWeapClips ) {
		return pm->ps->ammo[ BG_FindAmmoForWeapon( wp )];
	} else {
		//return pm->ps->ammoclip[BG_FindClipForWeapon( wp )];
		takeweapon = BG_FindClipForWeapon( wp );

		if ( BG_IsAkimboWeapon( wp ) ) {
			if ( !BG_AkimboFireSequence( wp, pm->ps->ammoclip[BG_FindClipForWeapon( wp )], pm->ps->ammoclip[BG_FindClipForWeapon( BG_AkimboSidearm( wp ) )] ) ) {
				takeweapon = BG_AkimboSidearm( wp );
			}
		}

		return pm->ps->ammoclip[takeweapon];
	}
}

/*
==============
PM_WeaponClipEmpty
	accounts for clips being used/not used
==============
*/
int PM_WeaponClipEmpty( int wp ) {
	if ( pm->noWeapClips ) {
		if ( !( pm->ps->ammo[ BG_FindAmmoForWeapon( wp )] ) ) {
			return 1;
		}
	} else {
		if ( !( pm->ps->ammoclip[BG_FindClipForWeapon( wp )] ) ) {
			return 1;
		}
	}

	return 0;
}


/*
==============
PM_CoolWeapons
==============
*/
void PM_CoolWeapons( void ) {
	int wp, maxHeat;

	for ( wp = 0; wp < WP_NUM_WEAPONS; wp++ ) {

		// if you have the weapon
		if ( COM_BitCheck( pm->ps->weapons, wp ) ) {
			// and it's hot
			if ( pm->ps->weapHeat[wp] ) {
				pm->ps->weapHeat[wp] -= ( (float)GetAmmoTableData( wp )->coolRate * pml.frametime );

				if ( pm->ps->weapHeat[wp] < 0 ) {
					pm->ps->weapHeat[wp] = 0;
				}

			}
		}
	}

	// a weapon is currently selected, convert current heat value to 0-255 range for client transmission
	if ( pm->ps->weapon ) {
		if ( pm->ps->persistant[PERS_HWEAPON_USE] || pm->ps->eFlags & EF_MOUNTEDTANK ) {
			// rain - floor to prevent 8-bit wrap
			pm->ps->curWeapHeat = floor( ( ( (float)pm->ps->weapHeat[WP_DUMMY_MG42] / MAX_MG42_HEAT ) ) * 255.0f );
		} else {
			// rain - #172 - don't divide by 0
			maxHeat = GetAmmoTableData( pm->ps->weapon )->maxHeat;

			// rain - floor to prevent 8-bit wrap
			if ( maxHeat != 0 ) {
				pm->ps->curWeapHeat = floor( ( ( (float)pm->ps->weapHeat[pm->ps->weapon] / (float)maxHeat ) ) * 255.0f );
			} else {
				pm->ps->curWeapHeat = 0;
			}
		}
	}

}

/*
==============
PM_AdjustAimSpreadScale
==============
*/
//#define	AIMSPREAD_DECREASE_RATE		300.0f
#define AIMSPREAD_DECREASE_RATE     200.0f      // (SA) when I made the increase/decrease floats (so slower weapon recover could happen for scoped weaps) the average rate increased significantly
#define AIMSPREAD_INCREASE_RATE     800.0f
#define AIMSPREAD_VIEWRATE_MIN      30.0f       // degrees per second
#define AIMSPREAD_VIEWRATE_RANGE    120.0f      // degrees per second

void PM_AdjustAimSpreadScale( void ) {
//	int		increase, decrease, i;
	int i;
	float increase, decrease;       // (SA) was losing lots of precision on slower weapons (scoped)
	float viewchange, cmdTime, wpnScale;

	// all weapons are very inaccurate in zoomed mode
	if ( pm->ps->eFlags & EF_ZOOMING ) {
		pm->ps->aimSpreadScale = 255;
		pm->ps->aimSpreadScaleFloat = 255;
		return;
	}

	cmdTime = (float)( pm->cmd.serverTime - pm->oldcmd.serverTime ) / 1000.0;

	wpnScale = 0.0f;
	switch ( pm->ps->weapon ) {
	case WP_LUGER:
	case WP_SILENCER:
	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_SILENCEDLUGER:
// rain - luger and akimbo are supposed to be balanced
//		wpnScale = 0.5f;
//		break;
	case WP_COLT:
	case WP_SILENCED_COLT:
	case WP_AKIMBO_COLT:
	case WP_AKIMBO_SILENCEDCOLT:
		wpnScale = 0.4f;        // doesn't fire as fast, but easier to handle than luger
		break;
	case WP_MP40:
		wpnScale = 0.6f;        // 2 handed, but not as long as mauser, so harder to keep aim
		break;
	case WP_GARAND:
		wpnScale = 0.5f;
		break;
	case WP_K43_SCOPE:
	case WP_GARAND_SCOPE:
	case WP_FG42SCOPE:
		wpnScale = 10.f;
		break;
	case WP_K43:
		wpnScale = 0.5f;
		break;
	case WP_MOBILE_MG42:
	case WP_MOBILE_MG42_SET:
		wpnScale = 0.9f;
		break;
	case WP_FG42:
		wpnScale = 0.6f;
		break;
	case WP_THOMPSON:
		wpnScale = 0.6f;
		break;
	case WP_STEN:
		wpnScale = 0.6f;
		break;
	case WP_KAR98:
	case WP_CARBINE:
		wpnScale = 0.5f;
		break;
	}

	if ( wpnScale ) {

		// JPW NERVE crouched players recover faster (mostly useful for snipers)
		if ( pm->ps->eFlags & EF_CROUCHING || pm->ps->eFlags & EF_PRONE ) {
			wpnScale *= 0.5;
		}

		decrease = ( cmdTime * AIMSPREAD_DECREASE_RATE ) / wpnScale;

		viewchange = 0;
		// take player movement into account (even if only for the scoped weapons)
		// TODO: also check for jump/crouch and adjust accordingly
		if ( BG_IsScopedWeapon( pm->ps->weapon ) ) {
			for ( i = 0; i < 2; i++ ) {
				viewchange += fabs( pm->ps->velocity[i] );
			}
		} else {
			// take player view rotation into account
			for ( i = 0; i < 2; i++ ) {
				viewchange += fabs( SHORT2ANGLE( pm->cmd.angles[i] ) - SHORT2ANGLE( pm->oldcmd.angles[i] ) );
			}
		}

		viewchange = (float)viewchange / cmdTime;   // convert into this movement for a second
		viewchange -= AIMSPREAD_VIEWRATE_MIN / wpnScale;
		if ( viewchange <= 0 ) {
			viewchange = 0;
		} else if ( viewchange > ( AIMSPREAD_VIEWRATE_RANGE / wpnScale ) ) {
			viewchange = AIMSPREAD_VIEWRATE_RANGE / wpnScale;
		}

		// now give us a scale from 0.0 to 1.0 to apply the spread increase
		viewchange = viewchange / (float)( AIMSPREAD_VIEWRATE_RANGE / wpnScale );

		increase = (int)( cmdTime * viewchange * AIMSPREAD_INCREASE_RATE );
	} else {
		increase = 0;
		decrease = AIMSPREAD_DECREASE_RATE;
	}

	// update the aimSpreadScale
	pm->ps->aimSpreadScaleFloat += ( increase - decrease );
	if ( pm->ps->aimSpreadScaleFloat < 0 ) {
		pm->ps->aimSpreadScaleFloat = 0;
	}
	if ( pm->ps->aimSpreadScaleFloat > 255 ) {
		pm->ps->aimSpreadScaleFloat = 255;
	}

	pm->ps->aimSpreadScale = (int)pm->ps->aimSpreadScaleFloat;  // update the int for the client
}

#define weaponstateFiring ( pm->ps->weaponstate == WEAPON_FIRING || pm->ps->weaponstate == WEAPON_FIRINGALT )

#define GRENADE_DELAY   250

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/

#define VENOM_LOW_IDLE  WEAP_IDLE1
#define VENOM_HI_IDLE   WEAP_IDLE2
#define VENOM_RAISE     WEAP_ATTACK1
#define VENOM_ATTACK    WEAP_ATTACK2
#define VENOM_LOWER     WEAP_ATTACK_LASTSHOT

//#define DO_WEAPON_DBG 1

void PM_Weapon( void ) {
	int addTime = 0;         // TTimo: init
	int ammoNeeded;
	qboolean delayedFire;       //----(SA)  true if the delay time has just expired and this is the frame to send the fire event
	int aimSpreadScaleAdd;
	int weapattackanim;
	qboolean akimboFire;
#ifdef DO_WEAPON_DBG
	static int weaponstate_last = -1;
#endif

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED ) {
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	// check for dead player
	if ( pm->ps->stats[STAT_HEALTH] <= 0 ) {
		if ( (!pm->ps->pm_flags) & PMF_LIMBO ) {
			PM_CoolWeapons();
		}

		//pm->ps->weapon = WP_NONE;
		return;
	}

	// special mounted mg42 handling
	switch ( pm->ps->persistant[PERS_HWEAPON_USE] ) {
	case 1:
//			PM_CoolWeapons(); // Gordon: Arnout says this is how it's wanted ( bleugh ) no cooldown on weaps while using mg42, but need to update heat on mg42 itself
		if ( pm->ps->weapHeat[WP_DUMMY_MG42] ) {
			pm->ps->weapHeat[WP_DUMMY_MG42] -= ( 300.f * pml.frametime );

			if ( pm->ps->weapHeat[WP_DUMMY_MG42] < 0 ) {
				pm->ps->weapHeat[WP_DUMMY_MG42] = 0;
			}

			// rain - floor() to prevent 8-bit wrap
			pm->ps->curWeapHeat = floor( ( ( (float)pm->ps->weapHeat[WP_DUMMY_MG42] / MAX_MG42_HEAT ) ) * 255.0f );
		}

		if ( pm->ps->weaponTime > 0 ) {
			pm->ps->weaponTime -= pml.msec;
			if ( pm->ps->weaponTime <= 0 ) {
				if ( !( pm->cmd.buttons & BUTTON_ATTACK ) ) {
					pm->ps->weaponTime = 0;
					return;
				}
			} else {
				return;
			}
		}

		if ( pm->cmd.buttons & BUTTON_ATTACK ) {
			pm->ps->weapHeat[WP_DUMMY_MG42] += MG42_RATE_OF_FIRE_MP;

			PM_AddEvent( EV_FIRE_WEAPON_MG42 );

			pm->ps->weaponTime += MG42_RATE_OF_FIRE_MP;

			BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue );
			pm->ps->viewlocked = 2;         // this enable screen jitter when firing

			if ( pm->ps->weapHeat[WP_DUMMY_MG42] >= MAX_MG42_HEAT ) {
				pm->ps->weaponTime = MAX_MG42_HEAT;     // cap heat to max
				PM_AddEvent( EV_WEAP_OVERHEAT );
				pm->ps->weaponTime = 2000;          // force "heat recovery minimum" to 2 sec right now
			}
		}
		return;
	case 2:
		if ( pm->ps->weaponTime > 0 ) {
			pm->ps->weaponTime -= pml.msec;
			if ( pm->ps->weaponTime <= 0 ) {
				if ( !( pm->cmd.buttons & BUTTON_ATTACK ) ) {
					pm->ps->weaponTime = 0;
					return;
				}
			} else {
				return;
			}
		}

		if ( pm->cmd.buttons & BUTTON_ATTACK ) {
			PM_AddEvent( EV_FIRE_WEAPON_AAGUN );

			pm->ps->weaponTime += AAGUN_RATE_OF_FIRE;

			BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue );
		}
		return;
	}

	if ( pm->ps->eFlags & EF_MOUNTEDTANK ) {
		if ( pm->ps->weapHeat[WP_DUMMY_MG42] ) {
			pm->ps->weapHeat[WP_DUMMY_MG42] -= ( 300.f * pml.frametime );

			if ( pm->ps->weapHeat[WP_DUMMY_MG42] < 0 ) {
				pm->ps->weapHeat[WP_DUMMY_MG42] = 0;
			}

			// rain - floor() to prevent 8-bit wrap
			pm->ps->curWeapHeat = floor( ( ( (float)pm->ps->weapHeat[WP_DUMMY_MG42] / MAX_MG42_HEAT ) ) * 255.0f );
		}

		if ( pm->ps->weaponTime > 0 ) {
			pm->ps->weaponTime -= pml.msec;
			if ( pm->ps->weaponTime <= 0 ) {
				if ( !( pm->cmd.buttons & BUTTON_ATTACK ) ) {
					pm->ps->weaponTime = 0;
					return;
				}
			} else {
				return;
			}
		}

		if ( pm->cmd.buttons & BUTTON_ATTACK ) {
			pm->ps->weapHeat[WP_DUMMY_MG42] += MG42_RATE_OF_FIRE_MP;

			PM_AddEvent( EV_FIRE_WEAPON_MOUNTEDMG42 );

			pm->ps->weaponTime += MG42_RATE_OF_FIRE_MP;

			BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue );

			if ( pm->ps->weapHeat[WP_DUMMY_MG42] >= MAX_MG42_HEAT ) {
				pm->ps->weaponTime = MAX_MG42_HEAT; // cap heat to max
				PM_AddEvent( EV_WEAP_OVERHEAT );
				pm->ps->weaponTime = 2000;      // force "heat recovery minimum" to 2 sec right now
			}

		}
		return;
	}

	pm->watertype = 0;

	if ( BG_IsAkimboWeapon( pm->ps->weapon ) ) {
		akimboFire = BG_AkimboFireSequence( pm->ps->weapon, pm->ps->ammoclip[BG_FindClipForWeapon( pm->ps->weapon )], pm->ps->ammoclip[BG_FindClipForWeapon( BG_AkimboSidearm( pm->ps->weapon ) )] );
	} else {
		akimboFire = qfalse;
	}

	// TTimo
	// show_bug.cgi?id=416
#ifdef DO_WEAPON_DBG
	if ( pm->ps->weaponstate != weaponstate_last ) {
	#ifdef CGAMEDLL
		Com_Printf( " CGAMEDLL\n" );
	#else
		Com_Printf( "!CGAMEDLL\n" );
	#endif
		switch ( pm->ps->weaponstate ) {
		case WEAPON_READY:
			Com_Printf( " -- WEAPON_READY\n" );
			break;
		case WEAPON_RAISING:
			Com_Printf( " -- WEAPON_RAISING\n" );
			break;
		case WEAPON_RAISING_TORELOAD:
			Com_Printf( " -- WEAPON_RAISING_TORELOAD\n" );
			break;
		case WEAPON_DROPPING:
			Com_Printf( " -- WEAPON_DROPPING\n" );
			break;
		case WEAPON_READYING:
			Com_Printf( " -- WEAPON_READYING\n" );
			break;
		case WEAPON_RELAXING:
			Com_Printf( " -- WEAPON_RELAXING\n" );
			break;
		case WEAPON_DROPPING_TORELOAD:
			Com_Printf( " -- WEAPON_DROPPING_TORELOAD\n" );
			break;
		case WEAPON_FIRING:
			Com_Printf( " -- WEAPON_FIRING\n" );
			break;
		case WEAPON_FIRINGALT:
			Com_Printf( " -- WEAPON_FIRINGALT\n" );
			break;
		case WEAPON_RELOADING:
			Com_Printf( " -- WEAPON_RELOADING\n" );
			break;
		}
		weaponstate_last = pm->ps->weaponstate;
	}
#endif

	// weapon cool down
	PM_CoolWeapons();

	// check for weapon recoil
	// do the recoil before setting the values, that way it will be shown next frame and not this
	if ( pm->pmext->weapRecoilTime ) {
		vec3_t muzzlebounce;
		int i, deltaTime;

		deltaTime = pm->cmd.serverTime - pm->pmext->weapRecoilTime;
		VectorCopy( pm->ps->viewangles, muzzlebounce );

		if ( deltaTime > pm->pmext->weapRecoilDuration ) {
			deltaTime = pm->pmext->weapRecoilDuration;
		}

		for ( i = pm->pmext->lastRecoilDeltaTime; i < deltaTime; i += 15 ) {
			if ( pm->pmext->weapRecoilPitch > 0.f ) {
				muzzlebounce[PITCH] -= 2*pm->pmext->weapRecoilPitch*cos( 2.5*(i) / pm->pmext->weapRecoilDuration );
				muzzlebounce[PITCH] -= 0.25 * random() * ( 1.0f - ( i ) / pm->pmext->weapRecoilDuration );
			}

			if ( pm->pmext->weapRecoilYaw > 0.f ) {
				muzzlebounce[YAW] += 0.5*pm->pmext->weapRecoilYaw*cos( 1.0 - (i)*3 / pm->pmext->weapRecoilDuration );
				muzzlebounce[YAW] += 0.5 * crandom() * ( 1.0f - ( i ) / pm->pmext->weapRecoilDuration );
			}
		}

		// set the delta angle
		for ( i = 0; i < 3; i++ ) {
			int cmdAngle;

			cmdAngle = ANGLE2SHORT( muzzlebounce[i] );
			pm->ps->delta_angles[i] = cmdAngle - pm->cmd.angles[i];
		}
		VectorCopy( muzzlebounce, pm->ps->viewangles );

		if ( deltaTime == pm->pmext->weapRecoilDuration ) {
			pm->pmext->weapRecoilTime = 0;
			pm->pmext->lastRecoilDeltaTime = 0;
		} else {
			pm->pmext->lastRecoilDeltaTime = deltaTime;
		}
	}

	delayedFire = qfalse;

	if ( pm->ps->weapon == WP_GRENADE_LAUNCHER || pm->ps->weapon == WP_GRENADE_PINEAPPLE || pm->ps->weapon == WP_DYNAMITE || pm->ps->weapon == WP_SMOKE_BOMB ) {
		if ( pm->ps->grenadeTimeLeft > 0 ) {
			qboolean forcethrow = qfalse;

			if ( pm->ps->weapon == WP_DYNAMITE ) {
				pm->ps->grenadeTimeLeft += pml.msec;

				// JPW NERVE -- in multiplayer, dynamite becomes strategic, so start timer @ 30 seconds
				if ( pm->ps->grenadeTimeLeft < 5000 ) {
					pm->ps->grenadeTimeLeft = 5000;
				}

			} else {
				pm->ps->grenadeTimeLeft -= pml.msec;

				if ( pm->ps->grenadeTimeLeft <= 100 ) { // give two frames advance notice so there's time to launch and detonate
					forcethrow = qtrue;

					pm->ps->grenadeTimeLeft = 100;
				}
			}

			if ( !( pm->cmd.buttons & BUTTON_ATTACK ) || forcethrow || pm->ps->eFlags & EF_PRONE_MOVING ) {
				if ( pm->ps->weaponDelay == GetAmmoTableData( pm->ps->weapon )->fireDelayTime || forcethrow ) {
					// released fire button.  Fire!!!
					if ( pm->ps->eFlags & EF_PRONE ) {
						if ( akimboFire ) {
							BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON2PRONE, qfalse, qtrue );
						} else {
							BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qfalse, qtrue );
						}
					} else {
						if ( akimboFire ) {
							BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON2, qfalse, qtrue );
						} else {
							BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue );
						}
					}
				}
			} else {
				return;
			}
		}
	}

	if ( pm->ps->weaponDelay > 0 ) {
		pm->ps->weaponDelay -= pml.msec;

		if ( pm->ps->weaponDelay <= 0 ) {
			pm->ps->weaponDelay = 0;
			delayedFire = qtrue;        // weapon delay has expired.  Fire this frame

			// double check the player is still holding the fire button down for these weapons
			// so you don't get a delayed "non-fire" (fire hit and released, then shot fires)
			switch ( pm->ps->weapon ) {
			default:
				break;
			}
		}
	}

	if ( pm->ps->weaponstate == WEAPON_RELAXING ) {
		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	if ( pm->ps->eFlags & EF_PRONE_MOVING && !delayedFire ) {
		return;
	}

	// make weapon function
	if ( pm->ps->weaponTime > 0 ) {
		pm->ps->weaponTime -= pml.msec;
		if ( !( pm->cmd.buttons & BUTTON_ATTACK ) && pm->ps->weaponTime < 0 ) {
			pm->ps->weaponTime = 0;
		}

		// Gordon: aha, THIS is the kewl quick fire mode :)
		// JPW NERVE -- added back for multiplayer pistol balancing
		if ( pm->ps->weapon == WP_LUGER || pm->ps->weapon == WP_COLT || pm->ps->weapon == WP_SILENCER || pm->ps->weapon == WP_SILENCED_COLT ||
			 pm->ps->weapon == WP_KAR98 || pm->ps->weapon == WP_K43 || pm->ps->weapon == WP_CARBINE || pm->ps->weapon == WP_GARAND ||
			 pm->ps->weapon == WP_GARAND_SCOPE || pm->ps->weapon == WP_K43_SCOPE || BG_IsAkimboWeapon( pm->ps->weapon ) ) {
// rain - moved releasedFire into pmext instead of ps
			if ( pm->pmext->releasedFire ) {
				if ( pm->cmd.buttons & BUTTON_ATTACK ) {
					// rain - akimbo weapons only have a 200ms delay, so
					// use a shorter time for quickfire (#255)
					if ( BG_IsAkimboWeapon( pm->ps->weapon ) ) {
						if ( pm->ps->weaponTime <= 50 ) {
							pm->ps->weaponTime = 0;
						}
					} else {
						if ( pm->ps->weaponTime <= 150 ) {
							pm->ps->weaponTime = 0;
						}
					}
				}
			} else if ( !( pm->cmd.buttons & BUTTON_ATTACK ) ) {
// rain - moved releasedFire into pmext instead of ps
				pm->pmext->releasedFire = qtrue;
			}
		}
	}


	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising

	if ( ( pm->ps->weaponTime <= 0 || ( !weaponstateFiring && pm->ps->weaponDelay <= 0 ) ) && !delayedFire ) {
		if ( pm->ps->weapon != pm->cmd.weapon ) {
			PM_BeginWeaponChange( pm->ps->weapon, pm->cmd.weapon, qfalse ); //----(SA)	modified
		}
	}

	if ( pm->ps->weaponDelay > 0 ) {
		return;
	}

	// check for clip change
	PM_CheckForReload( pm->ps->weapon );

	if ( pm->ps->weaponTime > 0 || pm->ps->weaponDelay > 0 ) {
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RELOADING ) {
		PM_FinishWeaponReload();
	}

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING || pm->ps->weaponstate == WEAPON_DROPPING_TORELOAD ) {
		PM_FinishWeaponChange();
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_RAISING ) {
		pm->ps->weaponstate = WEAPON_READY;

		PM_StartWeaponAnim( PM_IdleAnimForWeapon( pm->ps->weapon ) );
		return;
	} else if ( pm->ps->weaponstate == WEAPON_RAISING_TORELOAD ) {
		pm->ps->weaponstate = WEAPON_READY;

		PM_BeginWeaponReload( pm->ps->weapon );

		return;
	}


	if ( pm->ps->weapon == WP_NONE ) { // this is possible since the player starts with nothing
		return;
	}


	// JPW NERVE -- in multiplayer, don't allow panzerfaust or dynamite to fire if charge bar isn't full
	if ( pm->ps->weapon == WP_PANZERFAUST ) {
		if ( pm->ps->eFlags & EF_PRONE ) {
			return;
		}

		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < pm->soldierChargeTime ) {
			return;
		}
	}

	if ( pm->ps->weapon == WP_GPG40 || pm->ps->weapon == WP_M7 ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < ( pm->engineerChargeTime * 0.5f ) ) {
			return;
		}
	}

	if ( pm->ps->weapon == WP_MORTAR_SET ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < ( pm->soldierChargeTime * 0.5f ) ) {
			return;
		}

		if ( !delayedFire ) {
			pm->ps->weaponstate = WEAPON_READY;
		}
	}

	if ( pm->ps->weapon == WP_SMOKE_BOMB || pm->ps->weapon == WP_SATCHEL ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < pm->covertopsChargeTime ) {
			return;
		}
	}

	if ( pm->ps->weapon == WP_LANDMINE ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < ( pm->engineerChargeTime * 0.5f ) ) {
			return;
		}
	}

	if ( pm->ps->weapon == WP_DYNAMITE ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < pm->engineerChargeTime ) {
			return;
		}
	}

	if ( pm->ps->weapon == WP_AMMO ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < ( pm->ltChargeTime * 0.25f ) ) {
			// rain - #202 - ^^ properly check ltChargeTime here, not medicChargeTime
			if ( pm->cmd.buttons & BUTTON_ATTACK ) {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_NOPOWER, qtrue, qfalse );
			}
			return;
		}
	}

	if ( pm->ps->weapon == WP_MEDKIT ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < ( pm->medicChargeTime * 0.25f ) ) {
			if ( pm->cmd.buttons & BUTTON_ATTACK ) {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_NOPOWER, qtrue, qfalse );
			}
			return;
		}
	}

	if ( pm->ps->weapon == WP_SMOKE_MARKER ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < pm->ltChargeTime ) {
			return;
		}
	}

	if ( pm->ps->weapon == WP_MEDIC_ADRENALINE ) {
		if ( pm->cmd.serverTime - pm->ps->classWeaponTime < pm->medicChargeTime ) {
			return;
		}
	}

	// check for fire
	// if not on fire button and there's not a delayed shot this frame...
	// consider also leaning, with delayed attack reset
	if ( ( !( pm->cmd.buttons & ( BUTTON_ATTACK | WBUTTON_ATTACK2 ) ) && !delayedFire ) ||
		 ( pm->ps->leanf != 0 && pm->ps->weapon != WP_GRENADE_LAUNCHER && pm->ps->weapon != WP_GRENADE_PINEAPPLE && pm->ps->weapon != WP_SMOKE_BOMB ) ) {
		pm->ps->weaponTime  = 0;
		pm->ps->weaponDelay = 0;

		if ( weaponstateFiring ) { // you were just firing, time to relax
			PM_ContinueWeaponAnim( PM_IdleAnimForWeapon( pm->ps->weapon ) );
		}

		pm->ps->weaponstate = WEAPON_READY;
		return;
	}

	// a not mounted mortar can't fire
	if ( pm->ps->weapon == WP_MORTAR ) {
		return;
	}

	// player is zooming - no fire
	// JPW NERVE in MP, LT needs to zoom to call artillery
	if ( pm->ps->eFlags & EF_ZOOMING ) {
#ifdef GAMEDLL
		if ( pm->ps->stats[STAT_PLAYER_CLASS] == PC_FIELDOPS ) {
			pm->ps->weaponTime += 500;
			PM_AddEvent( EV_FIRE_WEAPON );
		}
#endif
		return;
	}

	// player is underwater - no fire
	if ( pm->waterlevel == 3 ) {
		if ( pm->ps->weapon != WP_KNIFE &&
			 pm->ps->weapon != WP_GRENADE_LAUNCHER &&
			 pm->ps->weapon != WP_GRENADE_PINEAPPLE &&
			 pm->ps->weapon != WP_DYNAMITE &&
			 pm->ps->weapon != WP_LANDMINE &&
			 pm->ps->weapon != WP_TRIPMINE &&
			 pm->ps->weapon != WP_SMOKE_BOMB ) {
			PM_AddEvent( EV_NOFIRE_UNDERWATER );    // event for underwater 'click' for nofire
			pm->ps->weaponTime  = 500;
			pm->ps->weaponDelay = 0;                // avoid insta-fire after water exit on delayed weapon attacks
			return;
		}
	}

	// start the animation even if out of ammo
	switch ( pm->ps->weapon )
	{
	default:
		if ( !weaponstateFiring ) {
			// delay so the weapon can get up into position before firing (and showing the flash)
			pm->ps->weaponDelay = GetAmmoTableData( pm->ps->weapon )->fireDelayTime;
		} else {
			if ( pm->ps->eFlags & EF_PRONE ) {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qfalse, qtrue );
			} else {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue );
			}
		}
		break;
		// machineguns should continue the anim, rather than start each fire
	case WP_MP40:
	case WP_THOMPSON:
	case WP_STEN:
	case WP_MEDKIT:                     // NERVE - SMF
	case WP_PLIERS:                     // NERVE - SMF
	case WP_SMOKE_MARKER:               // NERVE - SMF
	case WP_FG42:
	case WP_FG42SCOPE:
	case WP_MOBILE_MG42:
	case WP_MOBILE_MG42_SET:
	case WP_LOCKPICK:

		if ( !weaponstateFiring ) {
			pm->ps->weaponDelay = GetAmmoTableData( pm->ps->weapon )->fireDelayTime;
		} else {
			if ( pm->ps->eFlags & EF_PRONE ) {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qtrue, qtrue );
			} else {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qtrue, qtrue );
			}
		}
		break;
	case WP_PANZERFAUST:
	case WP_LUGER:
	case WP_COLT:
	case WP_GARAND:
	case WP_K43:
	case WP_KAR98:
	case WP_CARBINE:
	case WP_GPG40:
	case WP_M7:
	case WP_SILENCER:
	case WP_SILENCED_COLT:
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
	case WP_AKIMBO_COLT:
	case WP_AKIMBO_SILENCEDCOLT:
	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_SILENCEDLUGER:
		if ( !weaponstateFiring ) {
			// JPW NERVE -- pfaust has spinup time in MP
			if ( pm->ps->weapon == WP_PANZERFAUST ) {
				PM_AddEvent( EV_SPINUP );
			}
			// jpw

			pm->ps->weaponDelay = GetAmmoTableData( pm->ps->weapon )->fireDelayTime;
		} else {
			if ( pm->ps->eFlags & EF_PRONE ) {
				if ( akimboFire ) {
					BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON2PRONE, qfalse, qtrue );
				} else {
					BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qfalse, qtrue );
				}
			} else {
				if ( akimboFire ) {
					BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON2, qfalse, qtrue );
				} else {
					BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue );
				}
			}
		}
		break;
	case WP_MORTAR_SET:
		if ( !weaponstateFiring ) {
			PM_AddEvent( EV_SPINUP );
			PM_StartWeaponAnim( PM_AttackAnimForWeapon( WP_MORTAR_SET ) );
			pm->ps->weaponDelay = GetAmmoTableData( pm->ps->weapon )->fireDelayTime;
		} else {
			if ( pm->ps->eFlags & EF_PRONE ) {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qfalse, qtrue );
			} else {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue );
			}
		}
		break;

		// melee
	case WP_KNIFE:
		if ( !delayedFire ) {
			if ( pm->ps->eFlags & EF_PRONE ) {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qfalse, qfalse );
			} else {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qfalse );
			}
		}
		break;

		// throw
	case WP_DYNAMITE:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_SMOKE_BOMB:
		if ( !delayedFire ) {
			if ( PM_WeaponAmmoAvailable( pm->ps->weapon ) ) {
				if ( pm->ps->weapon == WP_DYNAMITE ) {
					pm->ps->grenadeTimeLeft = 50;
				} else {
					pm->ps->grenadeTimeLeft = 4000;     // start at four seconds and count down
				}

				PM_StartWeaponAnim( PM_AttackAnimForWeapon( pm->ps->weapon ) );
			}

			pm->ps->weaponDelay = GetAmmoTableData( pm->ps->weapon )->fireDelayTime;
		}
		break;
	case WP_LANDMINE:
		if ( !delayedFire ) {
			if ( PM_WeaponAmmoAvailable( pm->ps->weapon ) ) {
				if ( pm->ps->eFlags & EF_PRONE ) {
					BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON2PRONE, qfalse, qtrue );
				} else {
					BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue );
				}
			}

			pm->ps->weaponDelay = GetAmmoTableData( pm->ps->weapon )->fireDelayTime;
		}
		break;
	case WP_TRIPMINE:
	case WP_SATCHEL:
		if ( !delayedFire ) {
			if ( PM_WeaponAmmoAvailable( pm->ps->weapon ) ) {
				PM_StartWeaponAnim( PM_AttackAnimForWeapon( pm->ps->weapon ) );
			}

			pm->ps->weaponDelay = GetAmmoTableData( pm->ps->weapon )->fireDelayTime;
		}
		break;
	case WP_SATCHEL_DET:
		if ( !weaponstateFiring ) {
			PM_AddEvent( EV_SPINUP );
			pm->ps->weaponDelay = GetAmmoTableData( pm->ps->weapon )->fireDelayTime;
			PM_ContinueWeaponAnim( PM_AttackAnimForWeapon( WP_SATCHEL_DET ) );
		} else {
			if ( pm->ps->eFlags & EF_PRONE ) {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPONPRONE, qfalse, qtrue );
			} else {
				BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_FIREWEAPON, qfalse, qtrue );
			}
		}

		break;
	}

	pm->ps->weaponstate = WEAPON_FIRING;

	// check for out of ammo

	ammoNeeded = GetAmmoTableData( pm->ps->weapon )->uses;

	if ( pm->ps->weapon ) {
		int ammoAvailable;
		qboolean reloading, playswitchsound = qtrue;

		ammoAvailable = PM_WeaponAmmoAvailable( pm->ps->weapon );

		if ( ammoNeeded > ammoAvailable ) {
			// you have ammo for this, just not in the clip
			reloading = (qboolean)( ammoNeeded <= pm->ps->ammo[ BG_FindAmmoForWeapon( pm->ps->weapon )] );

			// if not in auto-reload mode, and reload was not explicitely requested, just play the 'out of ammo' sound
			if ( !pm->pmext->bAutoReload && IS_AUTORELOAD_WEAPON( pm->ps->weapon ) && !( pm->cmd.wbuttons & WBUTTON_RELOAD ) ) {
				reloading = qfalse;
			}

			switch ( pm->ps->weapon ) {
				// Ridah, only play if using a triggered weapon
			case WP_DYNAMITE:
			case WP_GRENADE_LAUNCHER:
			case WP_GRENADE_PINEAPPLE:
			case WP_LANDMINE:
			case WP_TRIPMINE:
			case WP_SMOKE_BOMB:
				playswitchsound = qfalse;
				break;

				// some weapons not allowed to reload.  must switch back to primary first
			case WP_FG42SCOPE:
			case WP_GARAND_SCOPE:
			case WP_K43_SCOPE:
				reloading = qfalse;
				break;
			}

			if ( playswitchsound ) {
				if ( reloading ) {
					PM_AddEvent( EV_EMPTYCLIP );
				} else {
					PM_AddEvent( EV_NOAMMO );
				}
			}

			if ( reloading ) {
				PM_ContinueWeaponAnim( PM_ReloadAnimForWeapon( pm->ps->weapon ) );
			} else {
				PM_ContinueWeaponAnim( PM_IdleAnimForWeapon( pm->ps->weapon ) );
				pm->ps->weaponTime += 500;
			}

			return;
		}
	}

	if ( pm->ps->weaponDelay > 0 ) {
		// if it hits here, the 'fire' has just been hit and the weapon dictated a delay.
		// animations have been started, weaponstate has been set, but no weapon events yet. (except possibly EV_NOAMMO)
		// checks for delayed weapons that have already been fired are return'ed above.
		return;
	}

	if ( !( pm->ps->eFlags & EF_PRONE ) && ( pml.groundTrace.surfaceFlags & SURF_SLICK ) ) {
		float fwdmove_knockback = 0.f;

		switch ( pm->ps->weapon ) {
		case WP_MOBILE_MG42:    fwdmove_knockback = 400.f;
			break;
		case WP_PANZERFAUST:    fwdmove_knockback = 32000.f;
			break;
		case WP_FLAMETHROWER:   fwdmove_knockback = 2000.f;
			break;
		}

		if ( fwdmove_knockback > 0.f ) {
			// Add some knockback on slick
			vec3_t kvel;
			float mass = 200;

			if ( DotProduct( pml.forward, pm->ps->velocity ) > 0  ) {
				VectorScale( pml.forward, -1.f * ( fwdmove_knockback / mass ), kvel );    // -1 as we get knocked backwards
			} else {
				VectorScale( pml.forward, -1.f * ( fwdmove_knockback / mass ), kvel );    // -1 as we get knocked backwards
			}

			VectorAdd( pm->ps->velocity, kvel, pm->ps->velocity );

			if ( !pm->ps->pm_time ) {
				pm->ps->pm_time = 100;
				pm->ps->pm_flags |= PMF_TIME_KNOCKBACK;
			}
		}
	}

	// take an ammo away if not infinite
	if ( PM_WeaponAmmoAvailable( pm->ps->weapon ) != -1 ) {
		// Rafael - check for being mounted on mg42
		if ( !( pm->ps->persistant[PERS_HWEAPON_USE] ) && !( pm->ps->eFlags & EF_MOUNTEDTANK ) ) {
			PM_WeaponUseAmmo( pm->ps->weapon, ammoNeeded );
		}
	}


	// fire weapon

	// add weapon heat
	if ( GetAmmoTableData( pm->ps->weapon )->maxHeat ) {
		pm->ps->weapHeat[pm->ps->weapon] += GetAmmoTableData( pm->ps->weapon )->nextShotTime;
	}

	// first person weapon animations

	// if this was the last round in the clip, play the 'lastshot' animation
	// this animation has the weapon in a "ready to reload" state
	if ( BG_IsAkimboWeapon( pm->ps->weapon ) ) {
		if ( akimboFire ) {
			weapattackanim = WEAP_ATTACK1;
		} else {
			weapattackanim = WEAP_ATTACK2;
		}
	} else {
		if ( PM_WeaponClipEmpty( pm->ps->weapon ) ) {
			weapattackanim = PM_LastAttackAnimForWeapon( pm->ps->weapon );
		} else {
			weapattackanim = PM_AttackAnimForWeapon( pm->ps->weapon );
		}
	}

	switch ( pm->ps->weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_K43:
	case WP_KAR98:
	case WP_GPG40:
	case WP_CARBINE:
	case WP_M7:
	case WP_LANDMINE:
	case WP_TRIPMINE:
	case WP_SMOKE_BOMB:
		PM_StartWeaponAnim( weapattackanim );
		break;

	case WP_MP40:
	case WP_THOMPSON:
	case WP_STEN:
	case WP_MEDKIT:
	case WP_PLIERS:
	case WP_SMOKE_MARKER:
	case WP_SATCHEL_DET:
	case WP_MOBILE_MG42:
	case WP_MOBILE_MG42_SET:
	case WP_LOCKPICK:
		PM_ContinueWeaponAnim( weapattackanim );
		break;

	case WP_MORTAR_SET:
		break;      // no animation

	default:
		// RF, testing
//			PM_ContinueWeaponAnim(weapattackanim);
		PM_StartWeaponAnim( weapattackanim );
		break;
	}

	// JPW NERVE -- in multiplayer, pfaust fires once then switches to pistol since it's useless for a while
	if ( ( pm->ps->weapon == WP_PANZERFAUST ) || ( pm->ps->weapon == WP_SMOKE_MARKER ) || ( pm->ps->weapon == WP_DYNAMITE ) || ( pm->ps->weapon == WP_SMOKE_BOMB ) || ( pm->ps->weapon == WP_LANDMINE ) || ( pm->ps->weapon == WP_SATCHEL ) ) {
		PM_AddEvent( EV_NOAMMO );
	}
	// jpw

	if ( pm->ps->weapon == WP_SATCHEL ) {
		pm->ps->ammoclip[WP_SATCHEL_DET] = 1;
		pm->ps->ammo[WP_SATCHEL] = 0;
		pm->ps->ammoclip[WP_SATCHEL] = 0;
		PM_BeginWeaponChange( WP_SATCHEL, WP_SATCHEL_DET, qfalse );
	}

	// WP_M7 and WP_GPG40 run out of ammo immediately after firing their last grenade
	if ( ( pm->ps->weapon == WP_M7 || pm->ps->weapon == WP_GPG40 ) && !pm->ps->ammo[ BG_FindAmmoForWeapon( pm->ps->weapon )] ) {
		PM_AddEvent( EV_NOAMMO );
	}

	if ( pm->ps->weapon == WP_MORTAR_SET && !pm->ps->ammo[WP_MORTAR] ) {
		PM_AddEvent( EV_NOAMMO );
		//PM_BeginWeaponChange( WP_MORTAR_SET, WP_MORTAR, qfalse );
	}

	if ( BG_IsAkimboWeapon( pm->ps->weapon ) ) {
		if ( akimboFire ) {
			PM_AddEvent( EV_FIRE_WEAPON );
		} else {
			PM_AddEvent( EV_FIRE_WEAPONB );
		}
	} else {
		if ( PM_WeaponClipEmpty( pm->ps->weapon ) ) {
			PM_AddEvent( EV_FIRE_WEAPON_LASTSHOT );
		} else {
			PM_AddEvent( EV_FIRE_WEAPON );
		}
	}

	// RF
// rain - moved releasedFire into pmext instead of ps
	pm->pmext->releasedFire = qfalse;
	pm->ps->lastFireTime = pm->cmd.serverTime;

	aimSpreadScaleAdd = 0;

	switch ( pm->ps->weapon ) {
	case WP_KNIFE:
	case WP_PANZERFAUST:
	case WP_DYNAMITE:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_FLAMETHROWER:
	case WP_GPG40:
	case WP_M7:
	case WP_LANDMINE:
	case WP_TRIPMINE:
	case WP_SMOKE_BOMB:
	case WP_MORTAR_SET:
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;
		break;

	case WP_LUGER:
	case WP_SILENCER:
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;
		aimSpreadScaleAdd = 20;
		break;

	case WP_COLT:
	case WP_SILENCED_COLT:
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;
		aimSpreadScaleAdd = 20;
		break;

	case WP_AKIMBO_COLT:
	case WP_AKIMBO_SILENCEDCOLT:
		// if you're firing an akimbo weapon, and your other gun is dry,
		// nextshot needs to take 2x time
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;

		// added check for last shot in both guns so there's no delay for the last shot
		if ( !pm->ps->ammoclip[BG_FindClipForWeapon( pm->ps->weapon )] ) {
			if ( !akimboFire ) {
				addTime = 2 * GetAmmoTableData( pm->ps->weapon )->nextShotTime;
			}
		} else if ( !pm->ps->ammoclip[BG_FindClipForWeapon( BG_AkimboSidearm( pm->ps->weapon ) )] ) {
			if ( akimboFire ) {
				addTime = 2 * GetAmmoTableData( pm->ps->weapon )->nextShotTime;
			}
		}

		aimSpreadScaleAdd = 20;
		break;

	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_SILENCEDLUGER:
		// if you're firing an akimbo weapon, and your other gun is dry,
		// nextshot needs to take 2x time
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;

		// rain - fixed the swapped usage of akimboFire vs. the colt
		// so that the last shot isn't delayed
		if ( !pm->ps->ammoclip[BG_FindClipForWeapon( pm->ps->weapon )] ) {
			if ( !akimboFire ) {
				addTime = 2 * GetAmmoTableData( pm->ps->weapon )->nextShotTime;
			}
		} else if ( !pm->ps->ammoclip[BG_FindClipForWeapon( BG_AkimboSidearm( pm->ps->weapon ) )] ) {
			if ( akimboFire ) {
				addTime = 2 * GetAmmoTableData( pm->ps->weapon )->nextShotTime;
			}
		}

// rain - colt and luger are supposed to be balanced
//		aimSpreadScaleAdd = 35;
		aimSpreadScaleAdd = 20;
		break;

	case WP_GARAND:
	case WP_K43:
	case WP_KAR98:
	case WP_CARBINE:
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;
		aimSpreadScaleAdd = 50;
		break;

	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;

		aimSpreadScaleAdd = 200;
		// jpw

		break;

	case WP_FG42:
	case WP_FG42SCOPE:
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;
		aimSpreadScaleAdd = 200 / 2.f;
		break;

	case WP_MP40:
	case WP_THOMPSON:
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;
		aimSpreadScaleAdd = 15 + rand() % 10;   // (SA) new values for DM
		break;

	case WP_STEN:
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;
		aimSpreadScaleAdd = 15 + rand() % 10;   // (SA) new values for DM
		break;

	case WP_MOBILE_MG42:
	case WP_MOBILE_MG42_SET:
		if ( weapattackanim == WEAP_ATTACK_LASTSHOT ) {
			addTime = 2000;
		} else {
			addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;
		}
		aimSpreadScaleAdd = 20;
		break;
// JPW NERVE
	case WP_MEDIC_SYRINGE:
	case WP_MEDIC_ADRENALINE:
	case WP_AMMO:
		// TAT 1/30/2003 - lockpick will use value in table too
	case WP_LOCKPICK:
		addTime = GetAmmoTableData( pm->ps->weapon )->nextShotTime;
		break;
// jpw
		// JPW: engineers disarm bomb "on the fly" (high sample rate) but medics & LTs throw out health pack/smoke grenades slow
		// NERVE - SMF
	case WP_PLIERS:
		addTime = 50;
		break;
	case WP_MEDKIT:
		addTime = 1000;
		break;
	case WP_SMOKE_MARKER:
		addTime = 1000;
		break;
		// -NERVE - SMF
	default:
		break;
	}

	// set weapon recoil
	pm->pmext->lastRecoilDeltaTime = 0;

	switch ( pm->ps->weapon ) {
	case WP_GARAND_SCOPE:
	case WP_K43_SCOPE:
		pm->pmext->weapRecoilTime = pm->cmd.serverTime;
		pm->pmext->weapRecoilDuration = 300;
		pm->pmext->weapRecoilYaw = crandom() * .5f;
		pm->pmext->weapRecoilPitch = .5f;
		break;
	case WP_MOBILE_MG42:
		pm->pmext->weapRecoilTime = pm->cmd.serverTime;
		pm->pmext->weapRecoilDuration = 200;
		if ( pm->ps->pm_flags & PMF_DUCKED || pm->ps->eFlags & EF_PRONE ) {
			pm->pmext->weapRecoilYaw = crandom() * .5f;
			pm->pmext->weapRecoilPitch = .45f * random() * .15f;
		} else {
			pm->pmext->weapRecoilYaw = crandom() * .25f;
			pm->pmext->weapRecoilPitch = .75f * random() * .2f;
		}
		break;
	case WP_FG42SCOPE:
		pm->pmext->weapRecoilTime = pm->cmd.serverTime;
		pm->pmext->weapRecoilDuration = 100;
		pm->pmext->weapRecoilYaw = 0.f;
		pm->pmext->weapRecoilPitch = .45f * random() * .15f;
		break;
	case WP_LUGER:
	case WP_SILENCER:
	case WP_AKIMBO_LUGER:
	case WP_AKIMBO_SILENCEDLUGER:
	case WP_COLT:
	case WP_SILENCED_COLT:
	case WP_AKIMBO_COLT:
	case WP_AKIMBO_SILENCEDCOLT:
		pm->pmext->weapRecoilTime = pm->cmd.serverTime;
		pm->pmext->weapRecoilDuration = 100;

		pm->pmext->weapRecoilYaw = 0.f; //crandom() * .1f;
		pm->pmext->weapRecoilPitch = .45f * random() * .15f;

		break;
	default:
		pm->pmext->weapRecoilTime = 0;
		pm->pmext->weapRecoilYaw = 0.f;
		break;
	}

	// check for overheat

	// the weapon can overheat, and it's hot
	if ( GetAmmoTableData( pm->ps->weapon )->maxHeat && pm->ps->weapHeat[pm->ps->weapon] ) {
		// it is overheating
		if ( pm->ps->weapHeat[pm->ps->weapon] >= GetAmmoTableData( pm->ps->weapon )->maxHeat ) {
			pm->ps->weapHeat[pm->ps->weapon] = GetAmmoTableData( pm->ps->weapon )->maxHeat;   // cap heat to max
			PM_AddEvent( EV_WEAP_OVERHEAT );
			addTime = 2000;     // force "heat recovery minimum" to 2 sec right now
		}
	}

	pm->ps->aimSpreadScaleFloat += 3.0 * aimSpreadScaleAdd;
	if ( pm->ps->aimSpreadScaleFloat > 255 ) {
		pm->ps->aimSpreadScaleFloat = 255;
	}

	pm->ps->aimSpreadScale = (int)( pm->ps->aimSpreadScaleFloat );

	pm->ps->weaponTime += addTime;

	PM_SwitchIfEmpty();
}

/*
==============
PM_BeginWeaponReload
==============
*/
void PM_BeginWeaponReload( int weapon ) {
	gitem_t* item;
	int reloadTime;

	// only allow reload if the weapon isn't already occupied (firing is okay)
	if ( pm->ps->weaponstate != WEAPON_READY && pm->ps->weaponstate != WEAPON_FIRING ) {
		return;
	}

	if ( ( ( weapon == WP_CARBINE ) && pm->ps->ammoclip[WP_CARBINE] != 0 ) || ( ( weapon == WP_MOBILE_MG42 || weapon == WP_MOBILE_MG42_SET ) && pm->ps->ammoclip[WP_MOBILE_MG42] != 0 ) || ( ( weapon == WP_GARAND || weapon == WP_GARAND_SCOPE ) && pm->ps->ammoclip[WP_GARAND] != 0 ) ) {
		return; // Gordon: no reloading of the carbine until clip is empty
	}

	if ( ( weapon <= WP_NONE || weapon > WP_DYNAMITE ) && !( weapon >= WP_KAR98 && weapon < WP_NUM_WEAPONS ) ) {
		return;
	}

	item = BG_FindItemForWeapon( weapon );
	if ( !item ) {
		return;
	}
	// Gordon: fixing reloading with a full clip
	if ( pm->ps->ammoclip[item->giAmmoIndex] >= GetAmmoTableData( weapon )->maxclip ) {
		return;
	}

	// no reload when leaning (this includes manual and auto reloads)
	if ( pm->ps->leanf ) {
		return;
	}

	// (SA) easier check now that the animation system handles the specifics
	switch ( weapon ) {
	case WP_DYNAMITE:
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_SMOKE_BOMB:
		break;

	default:
		// DHM - Nerve :: override current animation (so reloading after firing will work)
		if ( pm->ps->eFlags & EF_PRONE ) {
			BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_RELOADPRONE, qfalse, qtrue );
		} else {
			BG_AnimScriptEvent( pm->ps, pm->character->animModelInfo, ANIM_ET_RELOAD, qfalse, qtrue );
		}
		break;
	}

	if ( weapon != WP_MORTAR && weapon != WP_MORTAR_SET ) {
		PM_ContinueWeaponAnim( PM_ReloadAnimForWeapon( pm->ps->weapon ) );
	}


	// okay to reload while overheating without tacking the reload time onto the end of the
	// current weaponTime (the reload time is partially absorbed into the overheat time)
	reloadTime = GetAmmoTableData( weapon )->reloadTime;

	if ( pm->ps->weaponstate == WEAPON_READY ) {
		pm->ps->weaponTime += reloadTime;
	} else if ( pm->ps->weaponTime < reloadTime ) {
		pm->ps->weaponTime += ( reloadTime - pm->ps->weaponTime );
	}

	pm->ps->weaponstate = WEAPON_RELOADING;
	PM_AddEvent( EV_FILL_CLIP );    // play reload sound
}
