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

static char *help_str =
    "Usage: %s [-n nr_local_fluids] [-i iterations] hostname\n";

typedef struct{
  char * hostname = "127.0.0.1";
  int port = 6379;

  // only for sythetic put_mpi_foam
  int nr_local_fluids = 30;
  int nr_steps = 30;
  int isunix = 0; // use domain socket

}runtime_config_t;

int read_config(runtime_config_t *ptr_config, int argc, char * argv[]){
  int opt;
  // User option
  while ((opt = getopt(argc, argv, "h:p:i:n:")) != -1) {
    fprintf(stderr, "passing option %c\n", opt);
    switch (opt) {
    case 'p':
      ptr_config->port = atoi(optarg);
      break;
    case 'i':
      ptr_config->nr_steps = atoi(optarg);
      break;

    case 'n':
      ptr_config->nr_local_fluids = atoi(optarg);
      break;

    case 'u':
      ptr_config->isunix = 1;
      /* in this case, host is the path to the unix socket */
      // printf("Will connect to unix socket @%s\n", hostname);
      break;


    default: /* '?' */
      fprintf(stderr, help_str, argv[0]);
      return -1;
    }
  }
  if (optind >= argc) {
    fprintf(stderr, "Expected argument after options\n");
    fprintf(stderr,help_str);
    return -1;
  }

  printf("optind=%d\n", optind);
  ptr_config->hostname = argv[optind];
  return 0;
}
extern "C" {
#endif

#ifdef __cplusplus
}
#endif
#endif
