#include "g_api.h"
#include "g_local.h"

#if defined _WIN32
	HMODULE api_module;
# else
	void *api_module;
# endif


static qboolean loadModule() {

#ifdef OS_WINDOWS
	api_module = LoadLibrary(MODULE_DIR"/"MODULE_NAME);
#else
	api_module = dlopen(MODULE_DIR"/"MODULE_NAME, RTLD_LAZY);
#endif
	
	if (api_module == NULL) {
		return qfalse;
	}
	return qtrue;
}

void G_loadApi() {
	if (!loadModule()) {
		G_Error("Error loading %s\n", MODULE_DIR"/"MODULE_NAME);
	}
}
