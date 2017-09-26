/*
 * @author Feng Li, IUPUI
 * @date   2017
 */

#ifndef UTILITY_H
#define UTILITY_H

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

#define SIZE_ONE (2)



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





// get current time
double get_cur_time();

// check malloc NULL
void check_malloc(void * pointer);

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
void my_message(char *msg, int rank, int level);

#endif
