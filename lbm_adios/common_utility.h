// some common functios 
#ifndef COMMON_UTILITY_H
#define COMMON_UTILITY_H
  
#ifdef __cplusplus
extern "C" {
#endif

#include <sys/time.h>
#include <stdio.h>


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




/*******************************
 * for sequential use
 *******************************/

// get current time
double get_cur_time(); 


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


#ifdef __cplusplus
}
#endif
#endif
