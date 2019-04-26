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

// this file is only included when building a dll

static intptr_t(QDECL * syscall)(intptr_t arg, ...) = (intptr_t(QDECL *)(intptr_t, ...)) - 1;

Q_EXPORT void dllEntry(intptr_t (QDECL *syscallptr)(intptr_t arg, ...)) {
	syscall = syscallptr;
}

void    trap_Printf(const char *fmt) {
	syscall(G_PRINT, fmt);
}

void    trap_Error(const char *fmt) {
	syscall(G_ERROR, fmt);
}

int     trap_Milliseconds(void) {
	return syscall(G_MILLISECONDS);
}
int     trap_Argc(void) {
	return syscall(G_ARGC);
}

void    trap_Argv(int n, char *buffer, int bufferLength) {
	syscall(G_ARGV, n, buffer, bufferLength);
}

int     trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode) {
	return syscall(G_FS_FOPEN_FILE, qpath, f, mode);
}

void    trap_FS_Read(void *buffer, int len, fileHandle_t f) {
	syscall(G_FS_READ, buffer, len, f);
}

int     trap_FS_Write(const void *buffer, int len, fileHandle_t f) {
	return syscall(G_FS_WRITE, buffer, len, f);
}

void    trap_FS_FCloseFile(fileHandle_t f) {
	syscall(G_FS_FCLOSE_FILE, f);
}

int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize) {
	return syscall(G_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

void    trap_SendConsoleCommand(int exec_when, const char *text) {
	syscall(G_SEND_CONSOLE_COMMAND, exec_when, text);
}

void    trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags) {
	syscall(G_CVAR_REGISTER, cvar, var_name, value, flags);
}

void    trap_Cvar_Update(vmCvar_t *cvar) {
	syscall(G_CVAR_UPDATE, cvar);
}

void trap_Cvar_Set(const char *var_name, const char *value) {
	syscall(G_CVAR_SET, var_name, value);
}

int trap_Cvar_VariableIntegerValue(const char *var_name) {
	return syscall(G_CVAR_VARIABLE_INTEGER_VALUE, var_name);
}

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize) {
	syscall(G_CVAR_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize);
}

void trap_LocateGameData(gentity_t *gEnts, int numGEntities, int sizeofGEntity_t,
                         playerState_t *clients, int sizeofGClient) {
	syscall(G_LOCATE_GAME_DATA, gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGClient);
}

void trap_DropClient(int clientNum, const char *reason, int length) {
	syscall(G_DROP_CLIENT, clientNum, reason, length);
}

void trap_SendServerCommand(int clientNum, const char *text) {
	// rain - #433 - commands over 1022 chars will crash 01
	// client engine upon receipt, so ignore them
	// Nico, truncate the command before logging it
	// http://games.chruker.dk/enemy_territory/modding_project_bugfix.php?bug_id=001
	if (strlen(text) > 1022) {
		G_LogPrintf(qtrue, "\n%s: trap_SendServerCommand( %d, ... ) length exceeds 1022.\n", GAME_VERSION, clientNum);
		G_LogPrintf(qtrue, "%s: text [%.950s]... truncated\n", GAME_VERSION, text);
		return;
	}
	syscall(G_SEND_SERVER_COMMAND, clientNum, text);
}

void trap_SetConfigstring(int num, const char *string) {
	syscall(G_SET_CONFIGSTRING, num, string);
}

void trap_GetConfigstring(int num, char *buffer, int bufferSize) {
	syscall(G_GET_CONFIGSTRING, num, buffer, bufferSize);
}

void trap_GetUserinfo(int num, char *buffer, int bufferSize) {
	syscall(G_GET_USERINFO, num, buffer, bufferSize);
}

void trap_SetUserinfo(int num, const char *buffer) {
	syscall(G_SET_USERINFO, num, buffer);
}

void trap_GetServerinfo(char *buffer, int bufferSize) {
	syscall(G_GET_SERVERINFO, buffer, bufferSize);
}

void trap_SetBrushModel(gentity_t *ent, const char *name) {
	syscall(G_SET_BRUSH_MODEL, ent, name);
}

void trap_Trace(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	syscall(G_TRACE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

void trap_TraceCapsule(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	// Nico, silent GCC
	(void)passEntityNum;

	syscall(G_TRACECAPSULE, results, start, mins, maxs, end, passEntityNum, contentmask);
}

void trap_TraceCapsuleNoEnts(trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask) {
	// Nico, silent GCC
	(void)passEntityNum;

	syscall(G_TRACECAPSULE, results, start, mins, maxs, end, -2, contentmask);
}

int trap_PointContents(const vec3_t point, int passEntityNum) {
	return syscall(G_POINT_CONTENTS, point, passEntityNum);
}

qboolean trap_InPVS(const vec3_t p1, const vec3_t p2) {
	return syscall(G_IN_PVS, p1, p2);
}

void trap_AdjustAreaPortalState(gentity_t *ent, qboolean open) {
	syscall(G_ADJUST_AREA_PORTAL_STATE, ent, open);
}

void trap_LinkEntity(gentity_t *ent) {
	syscall(G_LINKENTITY, ent);
}

void trap_UnlinkEntity(gentity_t *ent) {
	syscall(G_UNLINKENTITY, ent);
}

int trap_EntitiesInBox(const vec3_t mins, const vec3_t maxs, int *list, int maxcount) {
	return syscall(G_ENTITIES_IN_BOX, mins, maxs, list, maxcount);
}

qboolean trap_EntityContactCapsule(const vec3_t mins, const vec3_t maxs, const gentity_t *ent) {
	return syscall(G_ENTITY_CONTACTCAPSULE, mins, maxs, ent);
}

void trap_GetUsercmd(int clientNum, usercmd_t *cmd) {
	syscall(G_GET_USERCMD, clientNum, cmd);
}

qboolean trap_GetEntityToken(char *buffer, int bufferSize) {
	return syscall(G_GET_ENTITY_TOKEN, buffer, bufferSize);
}

int trap_RealTime(qtime_t *qtime) {
	return syscall(G_REAL_TIME, qtime);
}

void trap_SnapVector(float *v) {
	syscall(G_SNAPVECTOR, v);
	return;
}

qboolean trap_GetTag(int clientNum, int tagFileNumber, char *tagName, orientation_t *or) {
	return syscall(G_GETTAG, clientNum, tagFileNumber, tagName, or);
}

qboolean trap_LoadTag(const char *filename) {
	return syscall(G_REGISTERTAG, filename);
}

int trap_PC_AddGlobalDefine(char *define) {
	return syscall(BOTLIB_PC_ADD_GLOBAL_DEFINE, define);
}

int trap_PC_LoadSource(const char *filename) {
	return syscall(BOTLIB_PC_LOAD_SOURCE, filename);
}

int trap_PC_FreeSource(int handle) {
	return syscall(BOTLIB_PC_FREE_SOURCE, handle);
}

int trap_PC_ReadToken(int handle, pc_token_t *pc_token) {
	return syscall(BOTLIB_PC_READ_TOKEN, handle, pc_token);
}

int trap_PC_SourceFileAndLine(int handle, char *filename, int *line) {
	return syscall(BOTLIB_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}
