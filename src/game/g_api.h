#ifndef G_API_H_
# define G_API_H_

# include "../APImodule/status.h"
# include <pthread.h>

# define API_INTERFACE_NAME "API_query"

void G_callAPI(char *result, char *query);
void G_loadAPI();
void G_unloadAPI();

#if defined _WIN32
	# include <strsafe.h>
	# include <windows.h>
# else
	# include <dlfcn.h>
# endif

struct query_s {
	char query[256];
	char *result;
};

#endif
