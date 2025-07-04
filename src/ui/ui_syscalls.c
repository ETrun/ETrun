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

#include "ui_local.h"

// this file is only included when building a dll

static intptr_t(QDECL * syscall)(intptr_t arg, ...) = (intptr_t(QDECL *)(intptr_t, ...)) - 1;

Q_EXPORT void dllEntry(intptr_t(QDECL *syscallptr)(intptr_t arg, ...)) {
	syscall = syscallptr;
}

void trap_Print(const char *string) {
	syscall(UI_PRINT, string);
}

void trap_Error(const char *string) {
	syscall(UI_ERROR, string);
}

int trap_Milliseconds(void) {
	return syscall(UI_MILLISECONDS);
}

void trap_Cvar_Register(vmCvar_t *cvar, const char *var_name, const char *value, int flags) {
	syscall(UI_CVAR_REGISTER, cvar, var_name, value, flags);
}

void trap_Cvar_Update(vmCvar_t *cvar) {
	syscall(UI_CVAR_UPDATE, cvar);
}

void trap_Cvar_Set(const char *var_name, const char *value) {
	syscall(UI_CVAR_SET, var_name, value);
}

// Nico, fixed strict-aliasing rules break
float trap_Cvar_VariableValue(const char *var_name) {
	float_int_u temp_u;

	temp_u.i = syscall(UI_CVAR_VARIABLEVALUE, var_name);
	return *(float *)&temp_u.f;
}

void trap_Cvar_VariableStringBuffer(const char *var_name, char *buffer, int bufsize) {
	syscall(UI_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize);
}

void trap_Cvar_SetValue(const char *var_name, float value) {
	syscall(UI_CVAR_SETVALUE, var_name, PASSFLOAT(value));
}

int trap_Argc(void) {
	return syscall(UI_ARGC);
}

void trap_Argv(int n, char *buffer, int bufferLength) {
	syscall(UI_ARGV, n, buffer, bufferLength);
}

void trap_Cmd_ExecuteText(int exec_when, const char *text) {
	syscall(UI_CMD_EXECUTETEXT, exec_when, text);
}

void trap_AddCommand(const char *cmdName) {
	syscall(UI_ADDCOMMAND, cmdName);
}

int trap_FS_FOpenFile(const char *qpath, fileHandle_t *f, fsMode_t mode) {
	return syscall(UI_FS_FOPENFILE, qpath, f, mode);
}

void trap_FS_Read(void *buffer, int len, fileHandle_t f) {
	syscall(UI_FS_READ, buffer, len, f);
}

void trap_FS_Write(const void *buffer, int len, fileHandle_t f) {
	syscall(UI_FS_WRITE, buffer, len, f);
}

void trap_FS_FCloseFile(fileHandle_t f) {
	syscall(UI_FS_FCLOSEFILE, f);
}

int trap_FS_GetFileList(const char *path, const char *extension, char *listbuf, int bufsize) {
	return syscall(UI_FS_GETFILELIST, path, extension, listbuf, bufsize);
}

int trap_FS_Delete(const char *filename) {
	return syscall(UI_FS_DELETEFILE, filename);
}

qhandle_t trap_R_RegisterModel(const char *name) {
	return syscall(UI_R_REGISTERMODEL, name);
}

qhandle_t trap_R_RegisterSkin(const char *name) {
	return syscall(UI_R_REGISTERSKIN, name);
}

void trap_R_RegisterFont(const char *fontName, int pointSize, fontInfo_t *font) {
	syscall(UI_R_REGISTERFONT, fontName, pointSize, font);
}

qhandle_t trap_R_RegisterShaderNoMip(const char *name) {
	return syscall(UI_R_REGISTERSHADERNOMIP, name);
}

void trap_R_ClearScene(void) {
	syscall(UI_R_CLEARSCENE);
}

void trap_R_AddRefEntityToScene(const refEntity_t *re) {
	syscall(UI_R_ADDREFENTITYTOSCENE, re);
}

void trap_R_AddPolyToScene(qhandle_t hShader, int numVerts, const polyVert_t *verts) {
	syscall(UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts);
}

void    trap_R_AddLightToScene(const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags) {
	syscall(UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT(radius), PASSFLOAT(intensity),
	        PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b), hShader, flags);
}

void trap_R_AddCoronaToScene(const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible) {
	syscall(UI_R_ADDCORONATOSCENE, org, PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b), PASSFLOAT(scale), id, visible);
}

void trap_R_RenderScene(const refdef_t *fd) {
	syscall(UI_R_RENDERSCENE, fd);
}

void trap_R_SetColor(const float *rgba) {
	syscall(UI_R_SETCOLOR, rgba);
}

void trap_R_Add2dPolys(polyVert_t *verts, int numverts, qhandle_t hShader) {
	syscall(UI_R_DRAW2DPOLYS, verts, numverts, hShader);
}

void trap_R_DrawStretchPic(float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader) {
	syscall(UI_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader);
}

void trap_R_ModelBounds(clipHandle_t model, vec3_t mins, vec3_t maxs) {
	syscall(UI_R_MODELBOUNDS, model, mins, maxs);
}

void trap_UpdateScreen(void) {
	syscall(UI_UPDATESCREEN);
}

void trap_S_StartLocalSound(sfxHandle_t sfx, int channelNum) {
	syscall(UI_S_STARTLOCALSOUND, sfx, channelNum, 127 /* Gordon: default volume always for the moment*/);
}

sfxHandle_t trap_S_RegisterSound(const char *sample) {
	int i = syscall(UI_S_REGISTERSOUND, sample, qfalse);

#ifdef DEBUG
	if (i == 0) {
		Com_Printf("^1Warning: Failed to load sound: %s\n", sample);
	}
#endif
	return i;
}

void    trap_S_FadeBackgroundTrack(float targetvol, int time, int num) {     // yes, i know.  fadebackground coming in, fadestreaming going out.  will have to see where functionality leads...
	syscall(UI_S_FADESTREAMINGSOUND, PASSFLOAT(targetvol), time, num);     // 'num' is '0' if it's music, '1' if it's "all streaming sounds"
}

void    trap_S_FadeAllSound(float targetvol, int time, qboolean stopsound) {
	syscall(UI_S_FADEALLSOUNDS, PASSFLOAT(targetvol), time, stopsound);
}

void trap_Key_KeynumToStringBuf(int keynum, char *buf, int buflen) {
	syscall(UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen);
}

void trap_Key_GetBindingBuf(int keynum, char *buf, int buflen) {
	syscall(UI_KEY_GETBINDINGBUF, keynum, buf, buflen);
}

// binding MUST be lower case
void trap_Key_KeysForBinding(const char *binding, int *key1, int *key2) {
	syscall(UI_KEY_BINDINGTOKEYS, binding, key1, key2);
}

void trap_Key_SetBinding(int keynum, const char *binding) {
	syscall(UI_KEY_SETBINDING, keynum, binding);
}

qboolean trap_Key_IsDown(int keynum) {
	return syscall(UI_KEY_ISDOWN, keynum);
}

qboolean trap_Key_GetOverstrikeMode(void) {
	return syscall(UI_KEY_GETOVERSTRIKEMODE);
}

void trap_Key_SetOverstrikeMode(qboolean state) {
	syscall(UI_KEY_SETOVERSTRIKEMODE, state);
}

void trap_Key_ClearStates(void) {
	syscall(UI_KEY_CLEARSTATES);
}

int trap_Key_GetCatcher(void) {
	return syscall(UI_KEY_GETCATCHER);
}

void trap_Key_SetCatcher(int catcher) {
	syscall(UI_KEY_SETCATCHER, catcher);
}

void trap_GetClientState(uiClientState_t *state) {
	syscall(UI_GETCLIENTSTATE, state);
}

void trap_GetGlconfig(glconfig_t *glconfig) {
	syscall(UI_GETGLCONFIG, glconfig);
}

int trap_GetConfigString(int index, char *buff, int buffsize) {
	return syscall(UI_GETCONFIGSTRING, index, buff, buffsize);
}

// NERVE - SMF
qboolean trap_LAN_UpdateVisiblePings(int source) {
	return syscall(UI_LAN_UPDATEVISIBLEPINGS, source);
}

int trap_LAN_GetServerCount(int source) {
	return syscall(UI_LAN_GETSERVERCOUNT, source);
}

int trap_LAN_CompareServers(int source, int sortKey, int sortDir, int s1, int s2) {
	return syscall(UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2);
}

void trap_LAN_GetServerAddressString(int source, int n, char *buf, int buflen) {
	syscall(UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen);
}

void trap_LAN_GetServerInfo(int source, int n, char *buf, int buflen) {
	syscall(UI_LAN_GETSERVERINFO, source, n, buf, buflen);
}

int trap_LAN_AddServer(int source, const char *name, const char *addr) {
	return syscall(UI_LAN_ADDSERVER, source, name, addr);
}

void trap_LAN_RemoveServer(int source, const char *addr) {
	syscall(UI_LAN_REMOVESERVER, source, addr);
}

int trap_LAN_GetServerPing(int source, int n) {
	return syscall(UI_LAN_GETSERVERPING, source, n);
}

int trap_LAN_ServerIsVisible(int source, int n) {
	return syscall(UI_LAN_SERVERISVISIBLE, source, n);
}

int trap_LAN_ServerStatus(const char *serverAddress, char *serverStatus, int maxLen) {
	return syscall(UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen);
}

qboolean trap_LAN_ServerIsInFavoriteList(int source, int n) {
	return syscall(UI_LAN_SERVERISINFAVORITELIST, source, n);
}

void trap_LAN_SaveCachedServers(void) {
	syscall(UI_LAN_SAVECACHEDSERVERS);
}

void trap_LAN_LoadCachedServers(void) {
	syscall(UI_LAN_LOADCACHEDSERVERS);
}

void trap_LAN_MarkServerVisible(int source, int n, qboolean visible) {
	syscall(UI_LAN_MARKSERVERVISIBLE, source, n, visible);
}

void trap_LAN_ResetPings(int n) {
	syscall(UI_LAN_RESETPINGS, n);
}
// -NERVE - SMF

int trap_MemoryRemaining(void) {
	return syscall(UI_MEMORY_REMAINING);
}

int trap_PC_AddGlobalDefine(char *define) {
	return syscall(UI_PC_ADD_GLOBAL_DEFINE, define);
}

int trap_PC_RemoveAllGlobalDefines(void) {
	return syscall(UI_PC_REMOVE_ALL_GLOBAL_DEFINES);
}

int trap_PC_LoadSource(const char *filename) {
	return syscall(UI_PC_LOAD_SOURCE, filename);
}

int trap_PC_FreeSource(int handle) {
	return syscall(UI_PC_FREE_SOURCE, handle);
}

int trap_PC_ReadToken(int handle, pc_token_t *pc_token) {
	return syscall(UI_PC_READ_TOKEN, handle, pc_token);
}

int trap_PC_SourceFileAndLine(int handle, char *filename, int *line) {
	return syscall(UI_PC_SOURCE_FILE_AND_LINE, handle, filename, line);
}

void trap_S_StopBackgroundTrack(void) {
	syscall(UI_S_STOPBACKGROUNDTRACK);
}

void trap_S_StartBackgroundTrack(const char *intro, const char *loop, int fadeupTime) {
	syscall(UI_S_STARTBACKGROUNDTRACK, intro, loop, fadeupTime);
}

int trap_RealTime(qtime_t *qtime) {
	return syscall(UI_REAL_TIME, qtime);
}

void    trap_R_RemapShader(const char *oldShader, const char *newShader, const char *timeOffset) {
	syscall(UI_R_REMAP_SHADER, oldShader, newShader, timeOffset);
}

#define MAX_VA_STRING       32000

char *trap_TranslateString(const char *string) {
	static char staticbuf[2][MAX_VA_STRING];
	static int  bufcount = 0;
	char        *buf;

	buf = staticbuf[bufcount++ % 2];

	Q_strncpyz(buf, string, MAX_VA_STRING);

	return buf;
}
// -NERVE - SMF

// DHM - Nerve
void trap_CheckAutoUpdate(void) {
	syscall(UI_CHECKAUTOUPDATE);
}

void trap_GetAutoUpdate(void) {
	syscall(UI_GET_AUTOUPDATE);
}
// DHM - Nerve

void trap_openURL(const char *s) {
	syscall(UI_OPENURL, s);
}

void trap_GetHunkData(int *hunkused, int *hunkexpected) {
	syscall(UI_GETHUNKDATA, hunkused, hunkexpected);
}
