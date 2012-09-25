#include <errno.h>
#include <fcntl.h>              /* for nonblocking */
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>


#define bzero(ptr, n)           memset(ptr, 0, n)

#define LOCAL_PORT              5555
#define REMOT_PORT		6666
#define LISTENQ                 8

#define MAX_SIZE	300

struct transpata {
	char tag[4];
	//++++++for gps
	double latitude;
	double longitude;
	//------
	//++++++for MAG sensor
	float mx;
	float my;
	float mz;
};

extern int make_nmea(int fd, double geo_long, double geo_lat, double geo_alt, double geo_sat, double geo_sat2);
