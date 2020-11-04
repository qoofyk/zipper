/*
 * Description:
 *
 * Author: Feng Li
 * e-mail: fengggli@yahoo.com
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#ifdef __cplusplus
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include "stdlib.h"

static const char *help_str =
    "Usage: %s [-n nr_local_fluids] [-i iterations]\n";

typedef struct{
  int nr_local_fluids = 30;
  int nr_steps = 30;
  double time_in_sec = 100.0;
  bool is_dry_run=false;

  // only for sythetic put_mpi_foam
  const char * hostname = "127.0.0.1";
  int port = 6379;
  int isunix = 0; // use domain socket
  int id_in_group =0; // idx of nodes which share the same redis

}runtime_config_t;

int read_config(runtime_config_t *ptr_config, int argc, char * argv[]){
  int opt;
  // User option
  while ((opt = getopt(argc, argv, "h:p:i:t:n:d")) != -1) {
    // fprintf(stderr, "passing option %c\n", opt);
    switch (opt) {
    case 'n':
      ptr_config->nr_local_fluids = atoi(optarg);
      break;

    case 'i':
      ptr_config->nr_steps = atoi(optarg);
      break;

    case 't':
      ptr_config->time_in_sec = (double)(atoi(optarg));
      break;


    case 'd':
      ptr_config->is_dry_run = true;
      break;

    /* options in below are deprecated*/
    case 'u':
      ptr_config->isunix = 1;
      /* in this case, host is the path to the unix socket */
      // printf("Will connect to unix socket @%s\n", hostname);
      break;
    case 'h':
      ptr_config->hostname = optarg;
      /* in this case, host is the path to the unix socket */
      // printf("Will connect to unix socket @%s\n", hostname);
      break;
    case 'p':
      ptr_config->port = atoi(optarg);
      break;

    default: /* '?' */
      fprintf(stderr, help_str, argv[0]);
      return -1;
    }
  }
  return 0;
}
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif
