/*
 * @author Feng Li, IUPUI
 * @date   2017
 */

#ifndef UTILITY_H
#define UTILITY_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>

#include <time.h>
#include "logging.h"

//#define SIZE_ONE (2)

// signed to unsigned, could also be implemented with bit operations
#define EVAL(a)  ((uint64_t)(a))

// status
typedef int status_t;
#define S_OK (0)
#define S_FAIL (-1)

#define KB(x) ((x) << 10)
#define MB(x) ((x) << 20)
#define GB(x) ((x) << 30)

#ifndef STRING_LENGTH
#define STRING_LENGTH (160)
#endif


#define RANK_SEQUENTIAL (-1)

#define SEC_PER_DAY   (86400)
#define SEC_PER_HOUR  (3600)
#define SEC_PER_MIN   (60)

/** Return current time in second*/
static double get_cur_time() {
  struct timeval   tv;
  struct timezone  tz;
  double cur_time;

  gettimeofday(&tv, &tz);
  cur_time = tv.tv_sec + tv.tv_usec / 1000000.0;
  //printf("%f\n",cur_time);

  return cur_time;
}

static void get_utc_time(char * str_time){
   time_t now = time(0);
   // convert now to tm struct for UTC
   struct tm *gmtm = gmtime(&now);
   char *dt = asctime(gmtm);

  struct timeval   tv;
  struct timezone  tz;
  double cur_time;

  gettimeofday(&tv, &tz);
  int msec = tv.tv_usec / 1000;
  int hms = (tv.tv_sec) % SEC_PER_DAY;
  int hour = hms/SEC_PER_HOUR;
  int min = (hms%SEC_PER_HOUR)/SEC_PER_MIN;
  int sec = (hms%SEC_PER_MIN);

  sprintf(str_time, "%02d:%02d:%02d.%03d", hour, min, sec, msec);
}

static void check_malloc(void * pointer){
  if (pointer == NULL) {
    perror("Malloc error!\n");
    fprintf (stderr, "at %s, line %d.\n", __FILE__, __LINE__);
    exit(1);
  }
}

#ifdef __cplusplus
}
#endif

#endif
