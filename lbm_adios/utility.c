#include "utility.h"
double get_cur_time() {
  struct timeval   tv;
  struct timezone  tz;
  double cur_time;

  gettimeofday(&tv, &tz);
  cur_time = tv.tv_sec + tv.tv_usec / 1000000.0;
  //printf("%f\n",cur_time);

  return cur_time;
}

void check_malloc(void * pointer){
  if (pointer == NULL) {
    perror("Malloc error!\n");
    fprintf (stderr, "at %s, line %d.\n", __FILE__, __LINE__);
    exit(1);
  }
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
