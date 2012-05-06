#ifndef G_API_H_
# define G_API_H_

# include <pthread.h>

# define API_INTERFACE_NAME "API_query"
# define QUERY_MAX_SIZE 1024
# define CHAR_SEPARATOR "/"

void G_callAPI(char *command, char *result, int count, ...);
void G_loadAPI();
void G_unloadAPI();
void G_API_mapRecords(char *mapName, char *result);
void G_API_login(char *authToken, int clientNum, char *result);

#if defined _WIN32
	# include <strsafe.h>
	# include <windows.h>
# else
	# include <dlfcn.h>
# endif

struct query_s {
	char cmd[64];
	char query[QUERY_MAX_SIZE];
	char *result;
};

typedef void* (*handler_t)(void *);

typedef struct {
	char *cmd;
	handler_t handler;
} api_glue_t;

#endif
