/* 
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.
 *
 * Copyright (c) 2008 - 2009.  UT-BATTELLE, LLC. All rights reserved.
 */

/* ADIOS C Example: read global arrays from a BP file
 *
 * This code is using the generic read API, which can read in
 * arbitrary slices of an array and thus we can read in an array
 * on arbitrary number of processes (provided our code is smart 
 * enough to do the domain decomposition).
 *
 * Run this example after adios_global, which generates 
 * adios_global.bp. Run this example on equal or less 
 * number of processes since we decompose only on one 
 * dimension of the global array here. 
*/

#define CLOG_MAIN
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "adios_read.h"
#include "adios_error.h"
//#include "adios_read_global.h"
#include "run_analysis.h"
#include "ds_adaptor.h"
#include "assert.h"

#include "transports.h"
static transport_method_t transport;

//#ifdef RAW_DSPACES
static char var_name[STRING_LENGTH];
static size_t elem_size=sizeof(double);
//#endif
        

#define DEBUG_Feng

int main (int argc, char ** argv) 
{
    /*
     * @input
     *
     * @param NSTOP
     * @FILESIZE2PRODCE
     * @PROCS_PRODUCER
     */
    if(argc !=4){
        printf("need to specify timstop total_file_size scratch procs_prod\n");
        exit(-1);
    }
    int nstop;
    int filesize2produce;
    int nprocs_producer;

    nstop= atoi(argv[1]);
    filesize2produce = atoi(argv[2]);
    nprocs_producer= atoi(argv[3]);
    

    int lp = 4;

    /******************** configuration stop ***********/
#ifdef ENABLE_TIMING
    double t1, t2, t3;
    double t_read_1, t_read_2, t_analy;
    t_read_1 = 0;
    t_read_2 = 0;
    t_analy = 0;
#endif


    int         rank, nprocs;
    MPI_Comm    comm = MPI_COMM_WORLD;
    void * data = NULL;
    uint64_t start[2], count[2];

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &nprocs);

    char nodename[256];
    int nodename_length;
    MPI_Get_processor_name(nodename, &nodename_length );

    /*
     * init the clog
     */
    
    int r;

    char *filepath = getenv("SCRATCH_DIR");
    if(filepath == NULL){
        fprintf(stderr, "scratch dir is not set!\n");
    }

    if(rank == 0){
        r = clog_init_fd(MY_LOGGER, 1);
    }
    else{
        char log_path[256];
        sprintf(log_path,"%s/results/consumer_%d.clog",filepath, rank);
        r = clog_init_path(MY_LOGGER, log_path);
    }
    if (r != 0) {
      fprintf(stderr, "Logger initialization failed.\n");
      return 1;
    }

    /*
     * get transport method
     */
    transport = get_current_transport();
    uint8_t transport_major = get_major(transport);
    uint8_t transport_minor = get_minor(transport);
    clog_info(CLOG(MY_LOGGER),"%s:I am rank %d of %d, tranport code %x-%x\n",
            nodename, rank, nprocs,
            get_major(transport), get_minor(transport) );
    assert(transport_major ==  NATIVE_STAGING);


    int timestep;
    
    /**** use index file to keep track of current step *****/
    MPI_Barrier(comm);

//#ifdef RAW_DSPACES
    char msg[STRING_LENGTH];
    int ret=-1;
    clog_info(CLOG(MY_LOGGER),"trying init dspaces for %d process\n", nprocs);
    ret = dspaces_init(nprocs, 2, &comm, NULL);

    //clog_info(CLOG(MY_LOGGER),"dspaces init successfuly \n");

    if(ret == 0){
        clog_info(CLOG(MY_LOGGER),"dataspaces init successfully");
    }else{
        clog_error(CLOG(MY_LOGGER),"dataspaces init err");
        exit(-1);
    }

    /*
    * set bounds and dspaces variables
    */
    sprintf(var_name, "atom");

    int dims_cube[3] = {filesize2produce/4,filesize2produce/4,filesize2produce};
    // each producer process 
    int n = dims_cube[0]*dims_cube[1]*dims_cube[2];
    // how many lines in global
    int global_size= n*nprocs_producer;
    //uint64_t gdims[2] = {2, global_size};
    //dspaces_define_gdim(var_name, 2,gdims);
    uint64_t slice_size;
               
    slice_size = (global_size)/nprocs;

    data = malloc (slice_size * SIZE_ONE* sizeof (double));
    if (data == NULL)
    {
        fprintf (stderr, "malloc failed.\n");
        return -1;
    }

    start[0] = slice_size * rank;
    if (rank == nprocs-1) /* last rank may read more lines */
        slice_size = slice_size + global_size%nprocs;
    count[0] = slice_size;

    start[1] = 0;
    count[1] = 2;
    clog_info(CLOG(MY_LOGGER),"start: (%ld, %ld), count:( %ld, %ld)\n", start[0], start[1], count[0], count[1]);

    int bounds[6] = {0};
    double time_comm = 0;;
    bounds[1]=start[0];
    bounds[0]=start[1];
    bounds[4]=start[0]+count[0]-1;
    bounds[3]=start[1]+count[1]-1;
//#endif
//#endif

    for(timestep=0; timestep < nstop; timestep++){

//#ifdef RAW_DSPACES
        t1 =MPI_Wtime(); 
        get_common_buffer(transport_minor, timestep,2, bounds,rank, var_name, (void **)&data, elem_size, &time_comm);
        t2 =MPI_Wtime(); 
        // all time spent by get_common_buffer
        t_read_1 += t2-t1;
        // actual communication time
        t_read_2 += time_comm;
//#endif
        if(rank ==0)
            clog_info(CLOG(MY_LOGGER),"Step %d read\n", timestep);
        // analysis
        run_analysis(data, slice_size, lp);

        t3 =MPI_Wtime(); 
        t_analy += t3-t2;

        //if(rank ==0)
            clog_info(CLOG(MY_LOGGER),"Step %d moments calculated, t_read %lf, t_advance %lf, t_analy %lf\n", timestep, t2-t1, time_comm, t3-t2);

    }

    free (data);
    MPI_Barrier(comm);
    double t_end = MPI_Wtime();

        double global_t_cal=0;
        double global_t_read=0;
        double global_t_get=0;
        MPI_Reduce(&t_analy, &global_t_cal, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
        MPI_Reduce(&t_read_1, &global_t_read, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
        MPI_Reduce(&t_read_2, &global_t_get, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
    if(rank == 0){
      clog_info(CLOG(MY_LOGGER),"stat:Consumer end  at %lf \n", t_end);
      clog_info(CLOG(MY_LOGGER),"stat:time for read %f s; time for ds_get %f s; time for analyst %f s\n", global_t_read/nprocs, global_t_get/nprocs, global_t_cal/nprocs);
    }


//#ifdef RAW_DSPACES
    dspaces_finalize();
//#endif

    clog_info(CLOG(MY_LOGGER),"rank %d: exit\n", rank);

    /*
   * close logger
   */
    clog_free(MY_LOGGER);

    MPI_Finalize ();
    return 0;
}