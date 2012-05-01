#include "g_api.h"
#include "g_local.h"

HMODULE api_module;

static qboolean loadModule() {
	api_module = LoadLibrary(MODULE_DIR"/"MODULE_NAME);

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


