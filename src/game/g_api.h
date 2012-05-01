#ifndef G_API_H_

# define G_API_H_

# define MODULE_NAME "APImodule.dll"
# define MODULE_DIR "etrun"

void G_loadApi();

#ifdef OS_WINDOWS
	# include <windows.h>
# else
	# include <dlfcn.h>
# endif

#endif
