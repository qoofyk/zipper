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
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <math.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <errno.h>

#include "logging.h"

//#define SIZE_ONE (2)



#ifndef STRING_LENGTH
#define STRING_LENGTH (160)
#endif


#define RANK_SEQUENTIAL (-1)


// current log filter
#define LOG_FILTER (5)

// different log level
#define LOG_CRITICAL (1)
#define LOG_WARNING (5)
#define LOG_INFO (10)
#define LOG_VERB (15)





static double get_cur_time() {
  struct timeval   tv;
  struct timezone  tz;
  double cur_time;

  gettimeofday(&tv, &tz);
  cur_time = tv.tv_sec + tv.tv_usec / 1000000.0;
  //printf("%f\n",cur_time);

  return cur_time;
}

static void check_malloc(void * pointer){
  if (pointer == NULL) {
    perror("Malloc error!\n");
    fprintf (stderr, "at %s, line %d.\n", __FILE__, __LINE__);
    exit(1);
  }
}

/*******************************
 * parallel use
 * input:
 *  msg: msg to output
 *  rank, current rank, -1 for sequential
 *  level, log level:
 *      1: CRITICAL
 *      5: WARNING
 *      10:INFO
 *      15:VER_INFO
 *******************************/

static const int MY_LOGGER = 0;

#ifdef __cplusplus
}
#endif

#endif
