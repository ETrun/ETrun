#ifndef G_API_H_
# define G_API_H_

# define MODULE_NAME "APImodule.dll"
# define MODULE_DIR "etrun"

void G_loadApi();

# ifdef __unix__

# elif defined _WIN32
	# include <windows.h>
# endif

#endif