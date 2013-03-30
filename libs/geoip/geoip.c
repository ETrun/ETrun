#include "geoip.h"
#include "../../src/game/g_local.h"

// Nico, geoip from etpub

GeoIP *gidb = NULL;

unsigned long GeoIP_addr_to_num(const char *addr) {
	unsigned int    c, octet, t;
	unsigned long   ipnum;
	int             i = 3;

	octet = ipnum = 0;
	while ((c = *addr++) != 0) {
		if (c == '.') {
			if (octet > 255) {
				return 0;
			}
			ipnum <<= 8;
			ipnum += octet;
			i--;
			octet = 0;
		} else {
			t = octet;
			octet <<= 3;
			octet += t;
			octet += t;
			c -= '0';
			if (c > 9)
				return 0;
			octet += c;
		}
	}
	if ((octet > 255) || (i != 0)) {
		return 0;
	}
	ipnum <<= 8;
	return ipnum + octet;
}

unsigned int GeoIP_seek_record(GeoIP *gi, unsigned long ipnum) {
	int depth;
	unsigned int x = 0;
	unsigned int step = 0;
	const unsigned char *buf = NULL;

	for (depth = 31; depth >= 0; depth--) {
		step = 6 * x;

		if (step + 6 >= gi->memsize) {
			LDE("error traversing database for ipnum = %lu\n", ipnum);
			return 255;
		}

		buf = gi->cache + step;

		if (ipnum & (1 << depth)) {
				x = (buf[3] << 0) + (buf[4] << 8) + (buf[5] << 16);
		} else {
				x = (buf[0] << 0) + (buf[1] << 8) + (buf[2] << 16);
		}

		if (x >= 16776960) {
			return x - 16776960;
		}
	}

	LDE("error traversing dtabase for ipnum = %lu\n", ipnum);
	return 255;
}

void GeoIP_open(char *path) {
	gidb = (GeoIP *)malloc(sizeof(GeoIP));

	if (gidb == NULL) {
		G_Error("GeoIP: Memory allocation error for GeoIP struct\n");
		return;
	}

	gidb->memsize = trap_FS_FOpenFile(path, &gidb->GeoIPDatabase,FS_READ);

	if ((int)gidb->memsize < 0) {
		free(gidb);
		gidb = NULL;
		G_Error("GeoIP: Error opening database\n");
		return;
	} else if (gidb->memsize == 0) {
		trap_FS_FCloseFile(gidb->GeoIPDatabase);
		free(gidb);
		gidb = NULL;
		G_Error("GeoIP: Error zero-sized database file\n");
		return;
	}
	gidb->cache = (unsigned char *) calloc(gidb->memsize + 1,sizeof(unsigned char));
	if (gidb->cache != NULL) {
		trap_FS_Read(gidb->cache, gidb->memsize, gidb->GeoIPDatabase);
		trap_FS_FCloseFile(gidb->GeoIPDatabase);
		return;
	}
	trap_FS_FCloseFile(gidb->GeoIPDatabase);
	free(gidb);
	gidb = NULL;
	G_Error("GeoIP: Memory allocation error for GeoIP cache\n");
}

void GeoIP_close(void) {
	if (gidb != NULL) {
		free(gidb->cache);
		gidb->cache = NULL;
		free(gidb);
		gidb = NULL;
	}
}
