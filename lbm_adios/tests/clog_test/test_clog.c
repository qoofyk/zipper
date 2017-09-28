/*
 * @brief  test clog 
 *
 * @author Feng Li, lifen@iupui.edu, IUPUI
 * @date   2017
 */

#define CLOG_MAIN
#include <clog.h>
#include <stdio.h>

const int MY_LOGGER = 0;

int main(int argc, char *argv[]){

  int r;
  r = clog_init_path(MY_LOGGER, "my_log.txt");
  if (r != 0) {
      fprintf(stderr, "Logger initialization failed.\n");
      return 1;
  }
  clog_info(CLOG(MY_LOGGER), "Hello, world!");
  clog_free(MY_LOGGER);
  return 0;
}


