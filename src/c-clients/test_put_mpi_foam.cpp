// code modified from
// https://github.com/redis/hiredis/blob/master/examples/example.c
// #define DEBUG
#include "c-clients/elastic_broker.h"
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
#define USE_PIPELINE
#define USE_PATTENED_DATA // make sure fields doesn't go to high to explode pydmd


#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef V_T
#include <VT.h>
int vt_class_id;
int vt_generate_id,
    vt_transform_id,
    vt_put_id;
#endif

// #define APP_HAS_BARRIER
#define SLEEP_USECS (80000)

redisContext *c;

int main(int argc, char **argv) {
  runtime_config_t config;
  unsigned int j;
  int opt;

  /* default options*/
  int nr_steps;
  int nr_local_fluids;
  bool is_dry_run;

  const char * field_name= "region";
  broker_ctx * context; 

  // Init MPI
  MPI_Status status;
  int taskid, numtasks;
  MPI_Comm comm = MPI_COMM_WORLD;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(comm, &taskid);
  MPI_Comm_size(comm, &numtasks);

  int ret = read_config(&config, argc, argv);
  if(ret < 0) return ret;

  // nr_steps = config.nr_steps;
  nr_local_fluids = config.nr_local_fluids;
  is_dry_run = config.is_dry_run;

#ifdef V_T
      //VT_initialize(NULL, NULL);
      VT_classdef( "EBroker", &vt_class_id );
      VT_funcdef("GE", vt_class_id, &vt_generate_id); //collsion
      VT_funcdef("TR", vt_class_id, &vt_transform_id);// streaming
      VT_funcdef("PU", vt_class_id, &vt_put_id);// boundry
#endif


  if(taskid == 0){
    PINF("running exp with nr_local_fluids(%d), time(%.1f), sleep (%d)us",
       nr_local_fluids, config.time_in_sec, SLEEP_USECS);
  }

  struct timeval timeout = {1, 500000}; // 1.5 seconds

  if(!is_dry_run){
		field_name= "region";
    const char* str_queue_len = std::getenv("BROKER_QUEUE_LEN");
    if(str_queue_len == nullptr){
      PINF("Initializing without pipelining.. ");

      context = broker_init(field_name, comm);
    }
    else{
      int queue_len = atoi(str_queue_len);
      context = broker_init_async(field_name, comm, queue_len);
    }

  }

  /* writing some floatting number with binary-safe string */

  float *v0_values = new float[nr_local_fluids];
  
  double t1, t2, t3, t4;


  char str_time[80], str_time_start[80];
  get_utc_time(str_time_start);
  if(taskid == 0){
    PINF("-- Simulation started, %s", str_time_start);
  }

  double cur_time = context->t_start;
  int step = 0;
  while(cur_time - context->t_start < config.time_in_sec) {
    
    // simulate all process advance one step
#ifdef APP_HAS_BARRIER
    MPI_Barrier(comm);
#endif
    t1 = MPI_Wtime();
#ifdef V_T
      VT_begin(vt_generate_id);
#endif

    // initialize
    srand(time(NULL));
    for(int i = 0; i < nr_local_fluids; i++){
      v0_values[i] =  (float)rand()/(float)RAND_MAX;;
    }
    std::string values;

    usleep(SLEEP_USECS);
    t3 = MPI_Wtime();
#ifdef V_T
      VT_end(vt_generate_id);
      VT_begin(vt_transform_id);
#endif

    // This is expensive!
    for (int atom_id = 0; atom_id < nr_local_fluids; atom_id++) {
      // TODO: using binary-safe floating point numbers, 6 precision: 9 chars
      values.append(std::to_string(v0_values[atom_id]));
      values.append(",");
    }

    t4= MPI_Wtime();

#ifdef V_T
      VT_end(vt_transform_id);
      VT_begin(vt_put_id);
#endif

    PDBG("Append values, time spent %.6f s ", t4 - t3);
    // std::cout << "command:" << commandString << std::endl;
  if(!is_dry_run){
		if(0 != broker_put(context, step, values)) break;
  }

    t2 = MPI_Wtime();
#ifdef V_T
      VT_end(vt_put_id);
#endif
    if(taskid == 0 && (step %(100) == 0)){
      PINF("Executing... step %d, (%.2f percent), %.3f second", step, 100*(cur_time - context->t_start)/(config.time_in_sec), cur_time - context->t_start);
      PINF("   step = %d: generate %.6f, prepare: %.6f, write=%.6f, seconds for %d fluids", step, t3 - t1, t4-t3, t2-t4, nr_local_fluids);
    }
    step += 1;
    cur_time = t2;
  } // this loop end when time-out
  // PINF(" process %d: finished  at %.3f second", taskid, cur_time- context->t_start);
	// wait for the worker

  MPI_Barrier(comm);
  if(taskid == 0){
    get_utc_time(str_time);

    PINF("\nSTATS:SimuStart\tSimuEnd, elapsed");
    PINF("STATS:%s\t%s\t%.3f\n", str_time_start, str_time, MPI_Wtime() - context->t_start);
  }

  /* Disconnects and frees the context */
  if(!is_dry_run){
    broker_finalize(context);
  }
  delete []v0_values;
  
#ifdef V_T
  VT_finalize();
#endif
  MPI_Finalize();

  return 0;
}
