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

  nr_steps = config.nr_steps;
  nr_local_fluids = config.nr_local_fluids;
  is_dry_run = config.is_dry_run;

  if(taskid == 0){
    PINF("running exp with nr_local_fluids(%d), iterations(%d)",
       nr_local_fluids, nr_steps);
  }

  struct timeval timeout = {1, 500000}; // 1.5 seconds

  if(!is_dry_run){
		field_name= "region";
    context = broker_init(field_name, comm);
  }

  /* writing some floatting number with binary-safe string */

  float *v0_values = new float[nr_local_fluids];
  
  double t1, t2, t3, t4;


  char str_time[80], str_time_start[80];
  get_utc_time(str_time_start);
  if(taskid == 0){
    PINF("-- Simulation started, %s", str_time_start);
  }

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
    std::string values;

    t3 = MPI_Wtime();

    // usleep(100000);
    // This is expensive!
    for (int atom_id = 0; atom_id < nr_local_fluids; atom_id++) {
      // TODO: using binary-safe floating point numbers, 6 precision: 9 chars
      values.append(std::to_string(v0_values[atom_id]));
      values.append(",");
    }

    t4= MPI_Wtime();

    PDBG("Append values, time spent %.6f s ", t4 - t3);
    // std::cout << "command:" << commandString << std::endl;
  if(!is_dry_run){
		if(0 != broker_put(context, step, values)) break;
  }

    t2 = MPI_Wtime();
    if(taskid == 0 && (step %(nr_steps/20) == 0)){
      PINF("Executing... (%d/%d):", step, nr_steps);
      PINF("   step = %d: generate %.6f, prepare: %.6f, write=%.6f, seconds for %d fluids", step, t3 - t1, t4-t3, t2-t4, nr_local_fluids);
    }
    // usleep(500000);
  }
	// wait for the worker

  MPI_Barrier(comm);
  if(taskid == 0){
    get_utc_time(str_time);

    PINF("\nSTATS:SimuStart\tSimuEnd");
    PINF("STATS:%s\t%s\n", str_time_start, str_time);
  }

  /* Disconnects and frees the context */
  if(!is_dry_run){
    broker_finalize(context);
  }
  delete []v0_values;

  MPI_Finalize();
  return 0;
}
