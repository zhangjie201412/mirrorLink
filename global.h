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

struct transpata {
	//++++++for gps
	double latitude;
	double longitude;
	//------
};
