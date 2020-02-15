// code modified from
// https://github.com/redis/hiredis/blob/master/examples/example.c
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/logging.h"

#include <vector>
#include <hiredis.h>

int main(int argc, char **argv) {
  unsigned int j, isunix = 0;
  redisContext *c;
  redisReply *reply;
  const char *hostname = (argc > 1) ? argv[1] : "127.0.0.1";

  MPI_Status status;
  int taskid, numtasks;
  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(comm, &taskid);
  MPI_Comm_size(comm, &numtasks);

  if (argc > 2) {
    if (*argv[2] == 'u' || *argv[2] == 'U') {
      isunix = 1;
      /* in this case, host is the path to the unix socket */
      printf("Will connect to unix socket @%s\n", hostname);
    }
  }

  int port = (argc > 2) ? atoi(argv[2]) : 6379;

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

  int nr_steps = 100;
  int nr_local_atoms = 1000;
  float value_x, value_y, value_z;
  const char *stream_name = "atoms";
  value_x = 0.0;
  value_y = 0.0;
  value_z = 0.0;
  double t1, t2;

  std::vector<double> time_stats;
  for (int step = 0; step < nr_steps; step++) {
    // simulate all process advance one step
    MPI_Barrier(comm);
    t1 = MPI_Wtime();
    for (int atom_id = 0; atom_id < nr_local_atoms; atom_id++) {
      // TODO: using binary-safe floating point numbers
#if 0
        reply = redisCommand(c,"XADD %s MAXLEN ~ 1000000 * stepid %ld atomid %ld x %b y %b z %b",
            stream_name,
            step,
            atom_id,
            &value_x,
            sizeof(value_x),
            &value_y,
            sizeof(value_y),
            &value_z,
            sizeof(value_z));
#else
      reply = (redisReply *)redisCommand(
          c, "XADD %s MAXLEN ~ 1000000 * step %ld atomid %ld x %f y %f z %f",
          stream_name, step, atom_id, value_x, value_y, value_z);

#endif
      if (reply->type == REDIS_REPLY_STRING) {
        PDBG(
            "XADD(client%d, step %d, atom %d) to stream(%s) (binary API): %s\n",
            taskid, step, atom_id, stream_name, reply->str);
      }
      freeReplyObject(reply);
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
    printf("[proc %d:]");
    for(auto iter = time_stats.begin(); iter != time_stats.end(); iter ++){
      printf(" %.3f", *iter);
    }
    printf("\n");
  }

  /* Disconnects and frees the context */
  redisFree(c);

  MPI_Finalize();
  return 0;
}
