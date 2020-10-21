/*
 * Description:
 *
 * Author: Feng Li
 * e-mail: fengggli@yahoo.com
 */

#ifndef ELATIC_BROKER_H_
#define ELATIC_BROKER_H_
#include "mpi.h"
#include <string>
#include <vector>

#ifdef __cplusplus
extern "C" {
  #include "hiredis.h"
#endif

#ifdef __cplusplus
}

//#define CLOUD_PORT (6379)
#define CLOUD_PORT (30379)


typedef struct{
  std::string hostname;
  int port;
  int local_id; // id inside the group
}broker_connection_t;

// broker_ctx mycontext;
typedef struct{
  char stream_name[80];
  redisContext *redis_context;
  broker_connection_t conn;

  // pipelining length
  bool is_async = false;
  size_t queue_len;
  size_t nr_queued = 0; // items has been put into the queue

  // for debugging use
  MPI_Comm comm;

  size_t max_write_size=0; // set in the first run, in bytes
  std::vector<double> time_stats;
}broker_ctx;



/**
 * @field
 * @field_name: identifier of this stream.
 * @parm groupid: currently not used.
 */

broker_ctx* broker_init(const char *field_name, MPI_Comm comm);

broker_ctx* broker_init_async(const char *field_name, MPI_Comm comm, int queue_len);

int broker_put(broker_ctx *context, int stepid, std::string values);

int broker_finalize(broker_ctx * context);
#endif
#endif
