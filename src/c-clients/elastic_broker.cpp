/*
 * Description:
 *
 * Author: Feng Li
 * e-mail: fengggli@yahoo.com
 */
#include "elastic_broker.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>

#include "common/logging.h"

/** Get the endpoint information of this mpirank*/
broker_connection_t get_connection(int rank, std::string endpoint_filepath){
  broker_connection_t conn;
  std::ifstream in_file(endpoint_filepath);
  std::string this_line;

  for(auto i = 0; i<= rank; i++){
    std::getline(in_file, this_line);

    std::istringstream iss(this_line);
    iss >>  conn.hostname >> conn.local_id;
  }
  conn.port = CLOUD_PORT;
  return conn;
}

broker_ctx* broker_init_async(const char *field_name, MPI_Comm comm, int queue_len){
  int mpi_rank, mpi_size;

  MPI_Comm_rank(comm, &mpi_rank);
  MPI_Comm_size(comm, &mpi_size);

  if(mpi_rank == 0){
    PINF("Broker: Initializing pipelining, queue_len = %d.. ", queue_len);
  }
  broker_ctx * context= broker_init(field_name, comm);
  context->is_async=true;
  context->queue_len=queue_len;
  context->t_start = MPI_Wtime();
  return context;
}

broker_ctx* broker_init(const char *field_name, MPI_Comm comm){
    int mpi_rank, mpi_size;

    MPI_Comm_rank(comm, &mpi_rank);
    MPI_Comm_size(comm, &mpi_size);

    if(mpi_rank == 0){
      PINF("Broker: nr_procs = %d", mpi_size);
    }

    broker_ctx * context =  new broker_ctx();

    // this has been testedin windowardound-building-allrun
    const char* endpoint_filepath = std::getenv("BROKER_ENDPOINT_FILE");
    if(endpoint_filepath == nullptr){
      throw std::runtime_error("envvar BROKER_ENDPOINT_FILE not defined");
      /*PWRN("Using default endpoint file");*/
      /*endpoint_filepath = "test_endpoints_file.ini";*/
    }
    broker_connection_t conn = get_connection(mpi_rank,  endpoint_filepath);


    sprintf(context->stream_name, "%s%d",field_name, conn.local_id);

    PDBG("mpirank: %d, writing stream (%s) to endpoint host: %s", mpi_rank, context->stream_name, conn.hostname.c_str());

    redisContext *c;
    redisReply *reply;

    unsigned int isunix = 0;

    int ii = 0;
      // mpi_rank =1;

    const char *hostname = conn.hostname.c_str();
    PDBG("running exp with endpoint (%s:%d), my mpi_rank = %d",
       hostname, conn.port, mpi_rank);

    struct timeval timeout = {1, 500000}; // 1.5 seconds
    if (isunix) {
      c = redisConnectUnixWithTimeout(hostname, timeout);
    } else {
      c = redisConnectWithTimeout(hostname, conn.port, timeout);
      // c = redisConnectNonBlock(hostname, conn.port);
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
    /* Suppress hiredis cleanup of unused buffers for max speed. */
    // c->reader->maxbuf = 0;

    /* PING server */
    reply = (redisReply *)redisCommand(c, "auth 5ba4239a1a2b7cd8131da1e557f4264df7ef2083f8895eab1d30384f870a9d87");
#ifdef DEBUG
    printf("auth: %s\n", reply->str);
#endif

    freeReplyObject(reply);

    /* PING server */
    reply = (redisReply *)redisCommand(c, "PING");
#ifdef DEBUG
    printf("PING: %s\n", reply->str);
#endif
    freeReplyObject(reply);

    context->redis_context= c;
    context->conn = conn;
    context->comm= comm;
    return context;
}

/** Asynchonous write using pipeline*/
int do_put_async(broker_ctx* context, const char* buffer){
  int ret = 0;
  redisContext *c = context->redis_context;
  redisReply *reply;

  redisAppendCommand(c, buffer);
  context->nr_queued ++;

  if(context-> nr_queued == context->queue_len){
    while(context->nr_queued){
      redisGetReply(c,(void **)&reply); // reply for SET
      if(reply == NULL || reply->type == REDIS_REPLY_ERROR){
        if(reply){
          PERR("HIREDIS xadd: error info in reply: %s", reply->str);
          freeReplyObject(reply);
        }
        else{
          PERR("HIREDIS xadd: error info in Context, %s", c->errstr);
        }
        ret = -1;
        break;
      }
      context->nr_queued --;
    }
  }
  return ret;
}

int broker_put(broker_ctx *context, int stepid, std::string values){
  double t_start, t_end;

  if(values.length() > context->max_write_size){
    context->max_write_size = values.length();
  }
  t_start = MPI_Wtime();

  std::string stream_name = context->stream_name;
  redisContext *c = context->redis_context;
  redisReply *reply;

  std::string commandString = "XADD ";
  commandString.append(stream_name);
  // commandString.append(" MAXLEN ~ 1000 * ");
  commandString.append(" * ");
  commandString.append(" step ");
  commandString.append(std::to_string(stepid));
  commandString.append(" localid ");
  commandString.append(std::to_string(context->conn.local_id));

  // commandString.append(" field ")
  // commandString.append(fieldname)
  commandString.append(" valuelist ");

  commandString.append(values);

  // PINF("---- rank (%d), command: %s",context->global_id, commandString.substr(0, 30).c_str());

  int ret = 0;

  if(context->is_async == false){
    reply = (redisReply *)redisCommand(c, commandString.c_str());
    if(reply == NULL || reply->type == REDIS_REPLY_ERROR){
      if(reply){
        PERR("HIREDIS xadd: error info in reply: %s", reply->str);
        freeReplyObject(reply);
      }
      else{
        PERR("HIREDIS xadd: error info in Context, %s", c->errstr);
      }
      ret = -1;
    }
  }
  else{
    ret = do_put_async(context, commandString.c_str());
  }

  t_end= MPI_Wtime();
  context->time_stats.push_back(t_end - t_start);
  return ret;
}

void broker_print_stats(broker_ctx *context){
  int mpi_rank, mpi_size;

  MPI_Comm_rank(context->comm, &mpi_rank);
  MPI_Comm_size(context->comm, &mpi_size);
  double write_time_this_proc= 0;
  double write_time_avg = 0;
  double write_time_std= 0;

  std::vector<double> time_stats = context->time_stats;

  for(auto iter = time_stats.begin(); iter != time_stats.end(); iter ++){
    PDBG(" %.3f", *iter);
    write_time_this_proc += *iter;
  }
  write_time_this_proc /= time_stats.size(); // spent for each write
  MPI_Reduce(&write_time_this_proc, &write_time_avg, 1, MPI_DOUBLE, MPI_SUM, 0, context->comm);
  if(mpi_rank == 0){
    write_time_avg /=  mpi_size;
  }
  MPI_Bcast(&write_time_avg, 1, MPI_DOUBLE, 0, context->comm);
  double diff_square = (write_time_this_proc - write_time_avg)*(write_time_this_proc - write_time_avg);

  MPI_Reduce(&diff_square, &write_time_std, 1, MPI_DOUBLE, MPI_SUM, 0, context->comm);
  unsigned long items_local, items_global;
  items_local = time_stats.size();
  MPI_Reduce(&items_local, &items_global, 1, MPI_UNSIGNED_LONG, MPI_SUM, 0, context->comm);

  if(mpi_rank == 0){
    write_time_std/= mpi_size;
    write_time_std = sqrt(write_time_std);
    PINF("BROKER: write time avg/std: %.6f %.6f", write_time_avg, write_time_std);
    double write_size_in_MB=(context->max_write_size)/1000000.0;
    double throughput_MB_calculated=write_size_in_MB/write_time_avg;
    double throughput_MB_measured=
      write_size_in_MB*items_global/(context->t_end - context->t_start);
    PINF("%lu total items, each %.6f MB", items_global, write_size_in_MB);
    PINF("BROKER: write_avg\twrite_std\ttp_calculated(MB/s)\ttp_measured\twrite_size_per_step(MB)");
    PINF("BROKER STATS: %.6f\t%.6f\t%.6f\t%.6f\t%.6f\n", write_time_avg, write_time_std, throughput_MB_calculated, throughput_MB_measured ,write_size_in_MB);
  }
}

int broker_finalize(broker_ctx * context){
  int status = 0;

  if(context->is_async){
    redisReply * reply;
    redisContext *c = context->redis_context;
    for(auto i = 0; i< context->nr_queued; i++){
      redisGetReply(c,(void **)&reply); // reply for SET
      if(reply == NULL || reply->type == REDIS_REPLY_ERROR){
        if(reply){
          PERR("HIREDIS xadd: error info in reply: %s", reply->str);
          freeReplyObject(reply);
        }
        else{
          PERR("HIREDIS xadd: error info in Context, %s", c->errstr);
        }
        status = -1;
        break;
      }
    }
    // sync all context in the pipeline
  }
  context->t_end = MPI_Wtime();

  redisFree(context->redis_context);
  broker_print_stats(context);
  return status;
}

