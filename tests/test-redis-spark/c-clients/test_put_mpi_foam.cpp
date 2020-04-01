// code modified from
// https://github.com/redis/hiredis/blob/master/examples/example.c
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "include/logging.h"
#include <getopt.h>

#include <vector>
#include <hiredis.h>
#define USE_PIPELINE

static char *help_str =
    "Usage: %s [-n nr_local_fluids] [-i iterations] hostname\n";


int main(int argc, char **argv) {
  unsigned int j, isunix = 0;
  int opt;
  redisContext *c;
  redisReply *reply;

  /* default options*/
  const char * hostname = "127.0.0.1";
  int port = 6379;;
  int nr_steps = 30;
  int nr_local_fluids = 1000;

  // Init MPI
  MPI_Status status;
  int taskid, numtasks;
  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(comm, &taskid);
  MPI_Comm_size(comm, &numtasks);

  // User option
  while ((opt = getopt(argc, argv, "h:p:i:n:")) != -1) {
    fprintf(stderr, "passing option %c\n", opt);
    switch (opt) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'i':
      nr_steps = atoi(optarg);
      break;

    case 'n':
      nr_local_fluids = atoi(optarg);
      break;

    case 'u':
      isunix = 1;
      /* in this case, host is the path to the unix socket */
      printf("Will connect to unix socket @%s\n", hostname);
      break;


    default: /* '?' */
      fprintf(stderr, help_str, argv[0]);
      exit(EXIT_FAILURE);
    }
  }
  if (optind >= argc) {
    fprintf(stderr, "Expected argument after options\n");
    fprintf(stderr,help_str);
    exit(EXIT_FAILURE);
  }

  printf("optind=%d\n", optind);
  hostname = argv[optind];
  PINF("running exp with server(%s:%d), with nr_local_fluids(%d), iterations(%d)",
       hostname, port, nr_local_fluids, nr_steps);


  struct timeval timeout = {1, 500000}; // 1.5 seconds
  if (isunix) {
    c = redisConnectUnixWithTimeout(hostname, timeout);
  } else {
    c = redisConnectWithTimeout(hostname, port, timeout);
  }
  if (c == NULL || c->err) {
    if (c) {
      printf("Connection error: %s\n", c->errstr);
      redisFree(c);
    } else {
      printf("Connection error: can't allocate redis context\n");
    }
    exit(1);
  }

  /* PING server */
  reply = (redisReply *)redisCommand(c, "PING");
  printf("PING: %s\n", reply->str);
  freeReplyObject(reply);


  /* writing some floatting number with binary-safe string */

  const char *stream_name = "fluids";

  float *v0_values = new float[nr_local_fluids];
  for(int i = 0; i < nr_local_fluids; i++){
    v0_values[i] = taskid*100.0;
  }
  
  double t1, t2;

  int queue_len = 128;
  int ii = 0;

  std::vector<double> time_stats;
  for (int step = 0; step < nr_steps; step++) {
    // simulate all process advance one step
    MPI_Barrier(comm);
    t1 = MPI_Wtime();
    std::string commandString = "XADD ";
    commandString.append(stream_name);
    commandString.append(" MAXLEN ~ 1000000 * ");
    commandString.append(" step ");
    commandString.append(std::to_string(step));
    commandString.append(" region_id ");
    commandString.append(std::to_string(taskid));
    // commandString.append(" field ")
    // commandString.append(fieldname)
    commandString.append(" valuelist ");

    for (int atom_id = 0; atom_id < nr_local_fluids; atom_id++) {
      // TODO: using binary-safe floating point numbers
      commandString.append(std::to_string(v0_values[atom_id]));
      commandString.append(",");
    }
    std::cout << "command:" << commandString << std::endl;
    #if 1
      std::cout << "Writing!" << std::endl;
      redisAppendCommand(
          c, commandString.c_str());
      if(step > 0 && step % queue_len == 0){
        for(ii = 0; ii < queue_len; ii ++){
          redisGetReply(c, (void **)(&reply));
          freeReplyObject(reply);
        }
      }
    #endif
    redisGetReply(c, (void **)(&reply));
    freeReplyObject(reply);

    t2 = MPI_Wtime();
    printf("step = %d, proc= %d, time =%.3f for %d fluids\n", step, taskid, t2 - t1, nr_local_fluids);
    time_stats.push_back(t2-t1);
    for(int i = 0; i < nr_local_fluids; i++){
      v0_values[i] += 1;
    }
    usleep(500000);
  }

  MPI_Barrier(comm);
  if(taskid == 0){
    double all_time_used = 0;
    printf("[proc %d:]", taskid);
    for(auto iter = time_stats.begin(); iter != time_stats.end(); iter ++){
      printf(" %.3f", *iter);
      all_time_used += *iter;
    }
    printf("\n");
    printf("average time %.3f s\n", all_time_used/time_stats.size());
  }


  /* Disconnects and frees the context */
  redisFree(c);
  delete []v0_values;

  MPI_Finalize();
  return 0;
}
