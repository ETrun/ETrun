#ifndef G_API_H_
# define G_API_H_

# define API_INTERFACE_NAME "API_query"
# define API_INIT_NAME "API_init"
# define API_CLEAN_NAME "API_clean"
# define RESPONSE_MAX_SIZE 2048
# define QUERY_MAX_SIZE 1024
# define CHAR_SEPARATOR "/"

void G_callAPI(char *command, char *result, gentity_t *ent, int count, ...);
void G_loadAPI();
void G_unloadAPI();
void G_API_login(char *result, gentity_t *ent, char *authToken);
void G_API_mapRecords(char *result, gentity_t *ent, char *mapName);
void G_API_check(char *result, gentity_t *ent);
void G_API_sendRecord(char *result, gentity_t *ent, char *mapName, char *runName, 
					  char *authToken, char *data, char *etrunVersion);
void G_API_getPlayerCheckpoints(char *result, gentity_t *ent, char *mapName, char *runName, int runNum, char *authToken);
void G_API_randommap(char *result, gentity_t *ent, char *mapName);

#if defined _WIN32
	# include <windows.h>
# else
	# include <dlfcn.h>
# endif

struct query_s {
	char cmd[64];
	char query[QUERY_MAX_SIZE];
	char *result;
	gentity_t *ent;
};

typedef void* (*handler_t)(void *);

typedef struct {
	char *cmd;
	handler_t handler;
} api_glue_t;

#endif
