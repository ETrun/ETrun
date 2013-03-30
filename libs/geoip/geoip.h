#ifndef GEOIP_H
# define GEOIP_H

typedef struct GeoIPTag {
    int GeoIPDatabase;
    unsigned char *cache;
	unsigned int memsize;
} GeoIP;

unsigned long GeoIP_addr_to_num(const char *addr);
unsigned int GeoIP_seek_record(GeoIP *gi, unsigned long ipnum);
void GeoIP_open(char *path);
void GeoIP_close(void);

extern GeoIP * gidb;

#endif /* GEOIP_H */