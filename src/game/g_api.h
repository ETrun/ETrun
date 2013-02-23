#ifndef G_API_H_
#define G_API_H_

#define API_INTERFACE_NAME "API_query"
#define API_INIT_NAME "API_init"
#define API_CLEAN_NAME "API_clean"
#define RESPONSE_MAX_SIZE 4096// Nico, note: 2048 is too low for maps like shorties
#define QUERY_MAX_SIZE 1024
#define CHAR_SEPARATOR "/"
#define THREADS_MAX 32  // Maximum threads at the same time

qboolean G_callAPI(char *command, char *result, gentity_t *ent, int count, ...);
void G_loadAPI();
void G_unloadAPI();
qboolean G_API_login(char *result, gentity_t *ent, char *authToken);
qboolean G_API_mapRecords(char *result, gentity_t *ent, char *mapName);
qboolean G_API_check(char *result, gentity_t *ent);
qboolean G_API_sendRecord(char *result, gentity_t *ent, char *mapName, char *runName,
                      char *authToken, char *data, char *etrunVersion);
qboolean G_API_getPlayerCheckpoints(char *result, gentity_t *ent, char *userName, char *mapName, char *runName, int runNum, char *authToken);
qboolean G_API_randommap(char *result, gentity_t *ent, char *mapName);
qboolean G_API_mapRank(char *result, gentity_t *ent, char *mapName, char *optUserName, char *optMapName, char *optRunName, char *optPhysicsName, char *authToken);

#if defined _WIN32
# include <windows.h>
#else
# include <dlfcn.h>
#endif

struct query_s {
	char cmd[64];
	char query[QUERY_MAX_SIZE];
	char *result;
	gentity_t *ent;
};

typedef void * (*handler_t)(void *);

typedef struct {
	char *cmd;
	handler_t handler;
} api_glue_t;

#endif
