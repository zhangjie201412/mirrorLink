#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "stralloc.h"

int
make_and_send(int fd, double geo_long, double geo_lat, double geo_alt, double geo_sat, double geo_sat2)
{

    // GEO_SAT2 provides bug backwards compatibility.
    enum { GEO_LONG = 0, GEO_LAT, GEO_ALT, GEO_SAT, GEO_SAT2, NUM_GEO_PARAMS };
//    char*   p = args;
    int     top_param = -1;
    double  params[ NUM_GEO_PARAMS ];
    int     n_satellites = 1;

    static  int     last_time = 0;
    static  double  last_altitude = 0.;
#if 0
    if (!p)
        p = "";

    /* tokenize */
    while (*p) {
        char*   end;
        double  val = strtod( p, &end );

	printf("val: %f\nend: %s\n", val, end);
        if (end == p) {
//            control_write( client, "KO: argument '%s' is not a number\n", p );
            return -1;
        }

        params[++top_param] = val;
        if (top_param + 1 == NUM_GEO_PARAMS)
            break;

        p = end;
        while (*p && (p[0] == ' ' || p[0] == '\t'))
            p += 1;
    }

    /* sanity check */
    if (top_param < GEO_LAT) {
//        control_write( client, "KO: not enough arguments: see 'help geo fix' for details\r\n" );
        return -1;
    }

    /* check number of satellites, must be integer between 1 and 12 */
    if (top_param >= GEO_SAT) {
        int sat_index = (top_param >= GEO_SAT2) ? GEO_SAT2 : GEO_SAT;
        n_satellites = (int) params[sat_index];
        if (n_satellites != params[sat_index]
            || n_satellites < 1 || n_satellites > 12) {
//            control_write( client, "KO: invalid number of satellites. Must be an integer between 1 and 12\r\n");
            return -1;
        }
    }
#endif
#define NUM_GEO_PARAMS		5

    int i = 0;

    params[0] = geo_long;
    params[1] = geo_lat;
    params[2] = geo_alt;
    params[3] = geo_sat;
    params[4] = geo_sat2;

#if 0
    printf("================\n");
    for(i = 0; i < NUM_GEO_PARAMS; i++) {
    	printf("%f\n", params[i]);
    }
    printf("================\n");
#endif
    /* generate an NMEA sentence for this fix */
    {
        STRALLOC_DEFINE(s);
        double   val;
        int      deg, min;
        char     hemi;

        /* format overview:
         *    time of fix      123519     12:35:19 UTC
         *    latitude         4807.038   48 degrees, 07.038 minutes
         *    north/south      N or S
         *    longitude        01131.000  11 degrees, 31. minutes
         *    east/west        E or W
         *    fix quality      1          standard GPS fix
         *    satellites       1 to 12    number of satellites being tracked
         *    HDOP             <dontcare> horizontal dilution
         *    altitude         546.       altitude above sea-level
         *    altitude units   M          to indicate meters
         *    diff             <dontcare> height of sea-level above ellipsoid
         *    diff units       M          to indicate meters (should be <dontcare>)
         *    dgps age         <dontcare> time in seconds since last DGPS fix
         *    dgps sid         <dontcare> DGPS station id
         */

        /* first, the time */
        stralloc_add_format( s, "$GPGGA,%06d", last_time );


        last_time ++;

        /* then the latitude */
        hemi = 'N';
        val  = params[GEO_LAT];
        if (val < 0) {
            hemi = 'S';
            val  = -val;
        }
        deg = (int) val;
        val = 60*(val - deg);
        min = (int) val;
        val = 10000*(val - min);
        stralloc_add_format( s, ",%02d%02d.%04d,%c", deg, min, (int)val, hemi );

        /* the longitude */
        hemi = 'E';
        val  = params[GEO_LONG];
        if (val < 0) {
            hemi = 'W';
            val  = -val;
        }
        deg = (int) val;
        val = 60*(val - deg);
        min = (int) val;
        val = 10000*(val - min);
        stralloc_add_format( s, ",%02d%02d.%04d,%c", deg, min, (int)val, hemi );

        /* bogus fix quality, satellite count and dilution */
        stralloc_add_format( s, ",1,%02d,", n_satellites );

        /* optional altitude + bogus diff */
        if (top_param >= GEO_ALT) {
            stralloc_add_format( s, ",%.1g,M,0.,M", params[GEO_ALT] );
            last_altitude = params[GEO_ALT];
        } else {
            stralloc_add_str( s, ",,,," );
        }
        /* bogus rest and checksum */
        stralloc_add_str( s, ",,,*47" );

        /* send it, then free */
	printf("%s\n", stralloc_cstr(s));
//        android_gps_send_nmea( stralloc_cstr(s) );
        stralloc_reset( s );
    }
    return 0;
}
