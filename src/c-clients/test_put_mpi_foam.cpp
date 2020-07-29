// code modified from
// https://github.com/redis/hiredis/blob/master/examples/example.c
// #define DEBUG
#include "config.h"
#include "common/utility.h"
#include "common/logging.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <cmath>
#include <getopt.h>

#include <iostream>
#include <vector>
#include <hiredis.h>
#define USE_PIPELINE
#define USE_PATTENED_DATA // make sure fields doesn't go to high to explode pydmd


// can comment this for a dry run
#define RUN_SYNC
// #define RUN_ASYNC // broke!

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

redisContext *c;

#ifdef RUN_ASYNC
std::mutex m;
std::condition_variable cv;
// std::string data;
bool ready = false;
bool processed = false;

void worker_thread()
{
  redisReply *reply;
  std::unique_lock<std::mutex> lk(m);
  while(1){
    // Wait until main() sends data
    cv.wait(lk, []{return ready;});
 
    // after the wait, we own the lock.
    // std::cout << "Worker thread is processing data\n";
    // data += " after processing";

		redisGetReply(c, (void **)(&reply));
    if(reply == NULL || reply->type == REDIS_REPLY_ERROR){
      PERR("Error on hiredis command ");
      if(reply)
        PERR("error info: %s", reply->str);
      exit(-1);
    }
    else{
		  freeReplyObject(reply);
      // Send data back to main()
      processed = true;
    }
    // std::cout << "Worker thread signals data processing completed\n";
 
    // Manual unlocking is done before notifying, to avoid waking up
    // the waiting thread only to block again (see notify_one for details)
    lk.unlock();
		cv.notify_one();
  }
}

#endif

int main(int argc, char **argv) {
  runtime_config_t config;
  unsigned int j;
  int opt;
  redisReply *reply;

  /* default options*/
  const char * hostname;
  int port;
  int nr_steps;
  int nr_local_fluids;
  unsigned int isunix = 0;

  // Init MPI
  MPI_Status status;
  int taskid, numtasks;
  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(comm, &taskid);
  MPI_Comm_size(comm, &numtasks);

  int ret = read_config(&config, argc, argv);
  if(ret < 0) return ret;
  hostname = config.hostname;
  port =  config.port;
  nr_steps = config.nr_steps;
  nr_local_fluids = config.nr_local_fluids;
  isunix = config.isunix;
  PINF("running exp with server(%s:%d), with nr_local_fluids(%d), iterations(%d)",
       hostname, port, nr_local_fluids, nr_steps);

  PINF("I am proc(%d) of all (%d)", taskid, numtasks);

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

  /* auth */
  reply = (redisReply *)redisCommand(c, "auth 5ba4239a1a2b7cd8131da1e557f4264df7ef2083f8895eab1d30384f870a9d87");
  printf("auth: %s\n", reply->str);
  freeReplyObject(reply);

  /* PING server */
  reply = (redisReply *)redisCommand(c, "PING");
  printf("PING: %s\n", reply->str);
  freeReplyObject(reply);


  /* writing some floatting number with binary-safe string */

  char stream_name[80];
  sprintf(stream_name, "region%d", numtasks * (config.id_in_group) + taskid);

  float *v0_values = new float[nr_local_fluids];
  
  double t1, t2;

  int queue_len = 128;
  int ii = 0;

  std::vector<double> time_stats;

#ifdef RUN_ASYNC
	std::thread worker(worker_thread);
#endif

  char str_time[80];
  get_utc_time(str_time);
  if(taskid == 0)
    PINF("-- Simulation started, %s", str_time);

  for (int step = 0; step < nr_steps; step++) {
    // simulate all process advance one step
    MPI_Barrier(comm);
    t1 = MPI_Wtime();

    double x_bounds[2] = {-5,5};
    double t_bounds[2] = {0, 4*3.14};
    double x_span=x_bounds[1] - x_bounds[0];
    double t_span=t_bounds[1] - t_bounds[0];
    for(int i = 0; i < nr_local_fluids; i++){
#ifndef USE_PATTENED_DATA
      v0_values[i] =  1;
#else
      double x = x_bounds[0] + i*(x_span/(nr_local_fluids-1));
      double t = t_bounds[0] + step*(t_span/(nr_steps-1));
      double out = 1.1 + sin(x+3*t);
      PDBG("-----fluid %d:out %.6f= 1.1 + sin(%.6f + 3*%.6f)", i, out, x, t);
      v0_values[i] =  out;
#endif
    }
    std::string commandString = "XADD ";
    commandString.append(stream_name);
    commandString.append(" MAXLEN ~ 100 * ");
    commandString.append(" step ");
    commandString.append(std::to_string(step));
    // commandString.append(" field ")
    // commandString.append(fieldname)
    commandString.append(" valuelist ");

    double t3 = MPI_Wtime();

    // This is expensive!
    for (int atom_id = 0; atom_id < nr_local_fluids; atom_id++) {
      // TODO: using binary-safe floating point numbers, 6 precision: 9 chars
      commandString.append(std::to_string(v0_values[atom_id]));
      commandString.append(",");
    }

    PDBG("Append values, time spent %.6f s ", MPI_Wtime() - t3);
    // std::cout << "command:" << commandString << std::endl;
    #ifdef RUN_SYNC
    reply = (redisReply *)redisCommand(c, commandString.c_str());
    if(reply == NULL || reply->type == REDIS_REPLY_ERROR){
      PERR("Error on hiredis command ");
      if(reply)
        PERR("error info: %s", reply->str);
      break;
    }
    freeReplyObject(reply);
    #elif defined RUN_ASYNC
    /* get reply from previous command*/
    if(step >0){
			// wait for the worker
			{
					std::unique_lock<std::mutex> lk(m);
					cv.wait(lk, []{return processed;});
			}
    	// std::cout << "Back in main(), data = " << data << '\n';
    }
    redisAppendCommand(
          c, commandString.c_str());
		// data = "Example data";
    // send data to the worker thread
    {
        std::lock_guard<std::mutex> lk(m);
        ready = true;
        // std::cout << "main() signals data ready for processing\n";
    }
    cv.notify_one();
    #else
    // dry run
    #endif

    t2 = MPI_Wtime();
    PDBG("step = %d, proc= %d, time =%.6f for %d fluids", step, taskid, t2 - t1, nr_local_fluids);
    time_stats.push_back(t2-t1);

    //usleep(500000);
  }
	// wait for the worker

#if defined RUN_ASYNC
	{
			std::unique_lock<std::mutex> lk(m);
			cv.wait(lk, []{return processed;});
	}
	worker.join();
#endif

  MPI_Barrier(comm);
  if(taskid == 0){
    get_utc_time(str_time);
    PINF("-- Simulation ended, %s", str_time);
    double all_time_used = 0;
    printf("[proc %d:]", taskid);
    for(auto iter = time_stats.begin(); iter != time_stats.end(); iter ++){
      PDBG(" %.3f", *iter);
      all_time_used += *iter;
    }
    printf("\n");
    PINF("Average time %.6f s", all_time_used/time_stats.size());
    double iops_per_proc = 1/(all_time_used/time_stats.size());
    PINF("IOPS: %.3fK(per proc), %.3fK(all)  s", iops_per_proc/1000, iops_per_proc*numtasks/1000);
    PINF("Throughput):%.6f MB/s(per proc), %.6f MB/s (all)", iops_per_proc*nr_local_fluids*9/1000000, iops_per_proc*nr_local_fluids*9*numtasks/1000000);
  }

  /* Disconnects and frees the context */
  redisFree(c);
  delete []v0_values;

  MPI_Finalize();
  return 0;
}
