#ifndef G_API_H_

# define G_API_H_

#ifdef OS_WINDOWS
	#	define MODULE_NAME "APImodule.dll"
# else
	#	define MODULE_NAME "APImodule.so"
# endif

# define MODULE_DIR "etrun"

void G_loadApi();

#ifdef OS_WINDOWS
	# include <windows.h>
# else
	# include <dlfcn.h>
# endif

#endif
