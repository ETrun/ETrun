#include "g_local.h"

/**
* Log (and print) a crash message
*/
void G_LogCrash(const char *s, qboolean printIt) {
	char       string[1024] = { 0 };
	qtime_t ct;

	trap_RealTime(&ct);

	if (printIt) {
		G_Printf("%s", s);
	}

	Com_sprintf(string, sizeof(string), "[%04d-%02d-%02d %02d:%02d:%02d] %s",
		1900 + ct.tm_year, 1 + ct.tm_mon, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec, s);

	if (level.logFile) {
		trap_FS_Write(string, strlen(string), level.logFile);
	} else {
		G_Printf("G_LogCrash: error while logging\n");
	}
}

/*
=================
G_LogPrintf

Print to the logfile with a date if it is open
=================
*/
void QDECL G_LogPrintf(qboolean printIt, const char *fmt, ...) {
	va_list argptr;
	char    string[1024];
	qtime_t ct;
	int     min, tens, sec, l;

	trap_RealTime(&ct);

	sec = level.time / 1000;
	min = sec / 60;
	sec -= min * 60;
	tens = sec / 10;
	sec -= tens * 10;

	Com_sprintf(string, sizeof(string), "[%04d-%02d-%02d %02d:%02d:%02d] [%d:%02d:%02d] ",
		1900 + ct.tm_year, 1 + ct.tm_mon, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec, min, tens, sec);

	l = strlen(string);

	va_start(argptr, fmt);
	Q_vsnprintf(string + l, sizeof(string) - l, fmt, argptr);
	va_end(argptr);

	if (printIt) {
		G_Printf("%s", string + l);
	}

	if (!level.logFile) {
		return;
	}

	trap_FS_Write(string, strlen(string), level.logFile);
}

/*
=================
G_LogDebug

Print to the debug logfile with a date if it is open
=================
*/
void QDECL G_LogDebug(const char *functionName, const char *severity, const char *fmt, ...) {
	va_list    argptr;
	char       string[1024] = { 0 };
	qtime_t ct;
	int     l = 0;

	trap_RealTime(&ct);

	Com_sprintf(string, sizeof(string), "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s] ",
		1900 + ct.tm_year, 1 + ct.tm_mon, ct.tm_mday, ct.tm_hour, ct.tm_min, ct.tm_sec, functionName, severity);

	l = strlen(string);

	va_start(argptr, fmt);
	Q_vsnprintf(string + l, sizeof(string) - l, fmt, argptr);
	va_end(argptr);

	if (!level.debugLogFile) {
		return;
	}

	trap_FS_Write(string, strlen(string), level.debugLogFile);
}
