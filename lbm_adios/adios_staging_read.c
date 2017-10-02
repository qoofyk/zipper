/* 
 * ADIOS is freely available under the terms of the BSD license described
 * in the COPYING file in the top level directory of this source distribution.  *
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
#include "run_analysis.h"
#include "adios_adaptor.h"
#include <assert.h>
#include "transports.h"
static transport_method_t transport;

//#define DEBUG_Feng


int main (int argc, char ** argv) 
{
     /*
     * @input
     * @param NSTOP
     * @param FILESIZE2PRODUCE
     */
    if(argc !=1){
        printf("more argumetns than expected\n");
        exit(-1);
    }

    int lp = 4;

    /******************** configuration stop ***********/

#ifdef ENABLE_TIMING
    double t1, t2, t3, t4;
    double t_read_1, t_read_2, t_analy;
    t_read_1 = 0;
    t_read_2 = 0;
    t_analy = 0;
#endif

    int         rank, size;
    MPI_Comm    comm = MPI_COMM_WORLD;

    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DIMES;
    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_BP;
    ADIOS_SELECTION * sel;
    void * data = NULL;
    uint64_t start[2], count[2];

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);

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
            nodename, rank, size,
            get_major(transport), get_minor(transport) );
    assert(transport_major == ADIOS_STAGING);

    enum ADIOS_READ_METHOD method;
    if(transport_minor == DSPACES)
        method = ADIOS_READ_METHOD_DATASPACES;

    else if(transport_minor == DIMES)
        method = ADIOS_READ_METHOD_DIMES;

    else if(transport_minor == FLEXPATH)
        method = ADIOS_READ_METHOD_FLEXPATH;
    else{
        method = ADIOS_READ_METHOD_BP;
    }


    if(adios_read_init_method (method, comm, "verbose=3") !=0){
        clog_info(CLOG(MY_LOGGER),"ERROR: rank %d: adios init err with %d\n", rank, method);
        exit(-1);
    }
    else{
        clog_info(CLOG(MY_LOGGER),"adios read method init complete with %d\n", method);
    }

#ifdef HAS_KEEP
#if defined(USE_DATASPACES)
  adios_init ("adios_xmls/dbroker_dataspaces.xml", comm);
  if(rank ==0)
    clog_info(CLOG(MY_LOGGER),"rank %d: adios init complete with dataspaces\n", rank);
#elif defined(USE_DIMES)
  adios_init ("adios_xmls/dbroker_dimes.xml", comm);
    clog_info(CLOG(MY_LOGGER),"rank %d: adios init complete with dimes\n", rank);
#elif defined(USE_FLEXPATH)
  adios_init ("adios_xmls/dbroker_flexpath.xml", comm);
    clog_info(CLOG(MY_LOGGER),"rank %d: adios init complete with flexpath\n", rank);

#else 
#error("define stating transport method");
#endif
#endif

    int timestep = 0;

    char filename[256];

    // append file name
    sprintf(filename,"%s/%s.bp", filepath, "atom");
    ADIOS_FILE * f = adios_read_open (filename, method, comm, ADIOS_LOCKMODE_CURRENT, 300);
    //ADIOS_FILE * f = adios_read_open (filename, method, comm, ADIOS_LOCKMODE_NONE, 0);

     if (f == NULL)
    {
        printf ("rank %d, %s\n",rank, adios_errmsg());
        return -1;
    }
    
    if(rank ==0)
        clog_info(CLOG(MY_LOGGER),"reader opened the stream\n");

    ADIOS_VARINFO * v = adios_inq_var (f, "atom");

    /* Using less readers to read the global array back, i.e., non-uniform */
    uint64_t slice_size = v->dims[0]/size;
    start[0] = slice_size * rank;
    if (rank == size-1) /* last rank may read more lines */
        slice_size = slice_size + v->dims[0]%size;
    count[0] = slice_size;

    start[1] = 0;
    count[1] = v->dims[1];
    clog_info(CLOG(MY_LOGGER),"rank %d: start: (%ld, %ld), count:( %ld, %ld)\n", rank, start[0], start[1], count[0], count[1]);
       

    data = malloc (slice_size * v->dims[1]* sizeof (double));
    if (data == NULL)
    {
        fprintf (stderr, "malloc failed.\n");
        return -1;
    }

    sel = adios_selection_boundingbox (v->ndim, start, count);

    //clog_info(CLOG(MY_LOGGER),"rank %d: adios init complete\n", rank);

    // get read status, before perform mpiio!, so streaming steps are not affected
    int errno_streaming_read = adios_errno;
    //for(timestep = 0; timestep < 10;){
    while(errno_streaming_read != err_end_of_stream){
        clog_info(CLOG(MY_LOGGER),"rank %d: Step %d start\n", rank, timestep);
           /* Read a subset of the temperature array */
        // 0:not used for strea; 1: must be set in stream
        adios_schedule_read (f, sel, "atom", 0, 1, data);
        t1 = get_cur_time();

        
        // block until read complete
        adios_perform_reads (f, 1);

        clog_debug(CLOG(MY_LOGGER),"    [DEBUG]:read is performed");
        t2 = get_cur_time();
        t_read_1 += t2-t1;

        adios_release_step(f);

        clog_debug(CLOG(MY_LOGGER),"previous step released");
        // advance to (1)the next availibale step (2)blocked if not unavailble
        adios_advance_step(f, 0, -1);

        clog_debug(CLOG(MY_LOGGER),"successfully step into next available step");
        
        t3 = get_cur_time();

        t_read_2 += t3-t2;

        /*
        for (i = 0; i < slice_size; i++) {
            if(* ((double *)data + i * v->dims[1])  -0 < 0.001){
                printf("ERROR: rank %d get zero lines in slice %d \n", rank, i);
                exit(-1);
            }
        }
        */

#ifdef DEBUG_Feng
        for (i = 0; i < slice_size; i++) {
            printf ("rank %d: [%" PRIu64 ",%d:%" PRIu64 "]", rank, start[0]+i, 0, slice_size);
            for (j = 0; j < v->dims[1]; j++){
                printf (" %6.6g", * ((double *)data + i * v->dims[1]  + j ));
            }
            printf("|");
            printf ("\n");
        }
#endif
        //printf("rank %d: Step %d read\n", rank, timestep);

        errno_streaming_read = adios_errno;

// keep 
#ifdef HAS_KEEP
        if(timestep == 0)
            insert_into_adios(filepath, "restart",-1, slice_size, v->dims[1], data,"w", &comm);
        else
            insert_into_adios(filepath, "restart", -1, slice_size, v->dims[1], data,"a", &comm);
        if(rank ==0)
            printf("rank %d: Step %d data kept\n", rank, timestep);
#endif

        // analysis
        run_analysis(data, slice_size, lp );
        t4 = get_cur_time();
        t_analy += t4-t3;

        clog_info(CLOG(MY_LOGGER),"rank %d: Step %d moments calculated, t_read %lf, t_advance %lf, t_analy %lf\n", rank, timestep, t2-t1, t3-t2, t4-t3);
        timestep ++;
    }

#ifdef ENABLE_TIMING
  MPI_Barrier(comm);
  double t_end = get_cur_time();
  if(rank == 0){
      clog_info(CLOG(MY_LOGGER),"stat:Consumer end  at %lf \n", t_end);
      clog_info(CLOG(MY_LOGGER),"stat:time for read %f s; time for advancing step %f s; time for analyst %f s\n", t_read_1, t_read_2, t_analy);
  }
#endif 
    free (data);
    adios_selection_delete (sel);
    adios_free_varinfo (v);
    adios_read_close (f);

    MPI_Barrier (comm);
    adios_read_finalize_method (method);
#ifdef HAS_KEEP
  adios_finalize (rank);
  if(rank == 0)
  printf("rank %d: adios finalize complete\n", rank); 
#endif

    /*
    * close logger
    */
    clog_free(MY_LOGGER);

    MPI_Finalize ();
    printf("rank %d: exit\n", rank);

    return 0;
}
