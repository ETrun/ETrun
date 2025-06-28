#include "g_local.h"
#include "g_geoip.h"

void G_Geoip_LoadMmdb(void) {
	char basePath[MAX_OSPATH];
	char filePath[MAX_OSPATH];
	int  status;

	if (!g_useGeoIP.integer) {
		level.geoipDatabaseLoaded = qfalse;
		return;
	}

	trap_Cvar_VariableStringBuffer("fs_basepath", basePath, sizeof (basePath));

	Com_sprintf(filePath, MAX_OSPATH, "%s", va("%s%cetrun%c%s", basePath, PATH_SEP, PATH_SEP, g_geoIPDbPath.string));

	status = MMDB_open(filePath, MMDB_MODE_MMAP, &level.mmdb);

	if (status != MMDB_SUCCESS) {
		G_Error("%s: could not load GeoIP database located at: %s, error: %s\n", GAME_VERSION, filePath, MMDB_strerror(status));
	}

	G_Printf("%s: GeoIP database loaded!\n", GAME_VERSION);
	level.geoipDatabaseLoaded = qtrue;
}

void G_Geoip_UnloadMmdb(void) {
	if (!g_useGeoIP.integer) {
		return;
	}

	level.geoipDatabaseLoaded = qfalse;
	MMDB_close(&level.mmdb);
}

unsigned int G_Geoip_GetCountryIdFromIsoCode(const char *isoCode) {
	unsigned int i;
	unsigned int id = 0;

	for (i = 0; i < sizeof (countries); ++i) {
		if (!strcmp(countries[i].isoCode, isoCode)) {
			id = countries[i].id;
			break;
		}
	}

	return id;
}

unsigned int G_Geoip_GetCountryCodeFromIp(const char *ip) {
	int                  gai_error, mmdb_error, status;
	MMDB_lookup_result_s result;
	MMDB_entry_data_s    entry_data;
	char                 countryCode[3];

	if (!strcmp(ip, "localhost") || level.geoipDatabaseLoaded != qtrue) {
		return 0;
	}

	result = MMDB_lookup_string(&level.mmdb, ip, &gai_error, &mmdb_error);

	if (gai_error != 0) {
		G_LogPrintf(qtrue, "GeoIP: %s cannot be located, getaddrinfo: %s.\n", ip, gai_strerror(gai_error));
		return 0;
	}

	if (mmdb_error != MMDB_SUCCESS) {
		G_LogPrintf(qtrue, "GeoIP: %s cannot be located, libmaxminddb: %s.\n", ip, MMDB_strerror(mmdb_error));
		return 0;
	}

	status = MMDB_get_value(&result.entry, &entry_data, "country", "iso_code", NULL);

	if (status != MMDB_SUCCESS || !entry_data.has_data || entry_data.type != MMDB_DATA_TYPE_UTF8_STRING) {
		G_LogPrintf(qtrue, "GeoIP: %s cannot be located.\n", ip);
		return 0;
	}

	Q_strncpyz(countryCode, entry_data.utf8_string, sizeof (countryCode));

	G_LogPrintf(qtrue, "GeoIP: %s located in %s.\n", ip, countryCode);

	return G_Geoip_GetCountryIdFromIsoCode(countryCode);
}
