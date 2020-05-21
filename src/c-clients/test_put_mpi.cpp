// code modified from
// https://github.com/redis/hiredis/blob/master/examples/example.c
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/logging.h"
#include <getopt.h>

#include <vector>
#include <hiredis.h>
#define USE_PIPELINE

static char *help_str =
    "Usage: %s [-n nr_local_atoms] [-i iterations] hostname\n";


int main(int argc, char **argv) {
  unsigned int j, isunix = 0;
  int opt;
  redisContext *c;
  redisReply *reply;

  /* default options*/
  const char * hostname = "127.0.0.1";
  int port = 6379;;
  int nr_steps = 30;
  int nr_local_atoms = 1000;

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
      nr_local_atoms = atoi(optarg);
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
  PINF("running exp with server(%s:%d), with nr_local_atoms(%d), iterations(%d)",
       hostname, port, nr_local_atoms, nr_steps);


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

#if 0
    /* Set a key */
    reply = redisCommand(c,"SET %s %s", "foo", "hello world");
    printf("SET: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key using binary safe API */
    reply = redisCommand(c,"SET %b %b", "bar", (size_t) 3, "hello", (size_t) 5);
    printf("SET (binary API): %s\n", reply->str);
    freeReplyObject(reply);

    /* Try a GET and two INCR */
    reply = redisCommand(c,"GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);

    reply = redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
    /* again ... */
    reply = redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);

    /* Create a list of numbers, from 0 to 9 */
    reply = redisCommand(c,"DEL mylist");
    freeReplyObject(reply);
    for (j = 0; j < 10; j++) {
        char buf[64];

        snprintf(buf,64,"%u",j);
        reply = redisCommand(c,"LPUSH mylist element-%s", buf);
        freeReplyObject(reply);
    }

    /* Let's check what we have inside the list */
    reply = redisCommand(c,"LRANGE mylist 0 -1");
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%u) %s\n", j, reply->element[j]->str);
        }
    }
    freeReplyObject(reply);
#endif

  /* writing some floatting number with binary-safe string */

  float value_x, value_y, value_z;
  const char *stream_name = "atoms";
  value_x = 0.0;
  value_y = 0.0;
  value_z = 0.0;
  double t1, t2;

  int queue_len = 128;
  int ii = 0;

  std::vector<double> time_stats;
  for (int step = 0; step < nr_steps; step++) {
    // simulate all process advance one step
    MPI_Barrier(comm);
    t1 = MPI_Wtime();
    for (int atom_id = 0; atom_id < nr_local_atoms; atom_id++) {
      // TODO: using binary-safe floating point numbers
#ifdef USE_PIPELINE
      redisAppendCommand(
          c, "XADD %s MAXLEN ~ 1000000 * step %ld atomid %ld x %f y %f z %f",
          stream_name, step, atom_id, value_x, value_y, value_z);
      if(atom_id > 0 && atom_id % queue_len == 0){
        for(ii = 0; ii < queue_len; ii ++){
          redisGetReply(c, (void **)(&reply));
          freeReplyObject(reply);
        }
      }
#else
      reply = (redisReply *)redisCommand(
          c, "XADD %s MAXLEN ~ 1000000 * step %ld atomid %ld x %f y %f z %f",
          stream_name, step, atom_id, value_x, value_y, value_z);

      if (reply->type == REDIS_REPLY_STRING) {
        PDBG(
            "XADD(client%d, step %d, atom %d) to stream(%s) (binary API): %s\n",
            taskid, step, atom_id, stream_name, reply->str);
      }
      freeReplyObject(reply);

#endif
    }
    t2 = MPI_Wtime();
    printf("step = %d, proc= %d, time =%.3f for %d atoms\n", step, taskid, t2 - t1, nr_local_atoms);
    time_stats.push_back(t2-t1);
    value_x += 1.0;
    value_y += 1.0;
    value_z += 1.0;
  }

  MPI_Barrier(comm);
  if(taskid == 0){
    double all_time_used = 0;
    printf("[proc %d:]");
    for(auto iter = time_stats.begin(); iter != time_stats.end(); iter ++){
      printf(" %.3f", *iter);
      all_time_used += *iter;
    }
    printf("\n");
    printf("average time %.3f s\n", all_time_used/time_stats.size());
  }


  /* Disconnects and frees the context */
  redisFree(c);

  MPI_Finalize();
  return 0;
}
