#include "common_utility.h"

double get_cur_time() {
    struct timeval   tv;
    struct timezone  tz;
    double cur_time;

    gettimeofday(&tv, &tz);
    cur_time = tv.tv_sec + tv.tv_usec / 1000000.0;

    return cur_time;
} 

void my_message(char *msg, int rank, int level){
    // only log high-priority level
    if(level > LOG_FILTER){
        return;
    }

    if(rank <0)
        printf("**sequential: %s\n", msg);
    else
        printf("**rank %d: %s\n", rank, msg);
}





