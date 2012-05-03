#ifndef G_API_H_

#include <strsafe.h>

# define G_API_H_

#if defined _WIN32
	#	define MODULE_NAME "APImodule.dll"
# else
	#	define MODULE_NAME "APImodule.so"
# endif

# define MODULE_DIR "etrun"
# define API_INTERFACE_NAME "API_query"

void G_loadApi();

#if defined _WIN32
	# include <windows.h>
# else
	# include <dlfcn.h>
# endif

#endif
