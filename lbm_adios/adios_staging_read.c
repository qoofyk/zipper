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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "adios_read.h"
#include "adios_error.h"
#include "run_analysis.h"
#include "adios_adaptor.h"
#include "utility.h"

//#define DEBUG_Feng


int main (int argc, char ** argv) 
{
    if(argc !=2){
        printf("need to specify scratch path for i/o\n");
        exit(-1);
    }
    char filepath[256];
    strcpy(filepath, argv[1]);

    int lp = 4;

    /******************** configuration stop ***********/

    int         rank, size;
    MPI_Comm    comm = MPI_COMM_WORLD;

#if defined(USE_DATASPACES)
    enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DATASPACES;
#elif defined(USE_DIMES)
    enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DIMES;
#else
    enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_BP;
#endif
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
    printf("%s:I am rank %d of %d\n",nodename, rank, size);


    if(adios_read_init_method (method, comm, "verbose=3") !=0){
        printf("ERROR: rank %d: adios init err with %d\n", rank, method);
        exit(-1);
    }
    else{
        if(rank == 0)
            printf("rank %d: adios read method init complete\n", rank);
    }

#ifdef HAS_KEEP
#if defined(USE_DATASPACES)
  adios_init ("adios_xmls/dbroker_dataspaces.xml", comm);
  if(rank ==0)
    printf("rank %d: adios init complete with dataspaces\n", rank);
#elif defined(USE_DIMES)
  adios_init ("adios_xmls/dbroker_dimes.xml", comm);
  if(rank ==0)
    printf("rank %d: adios init complete with dimes\n", rank);
#else 
#error("define stating transport method");
#endif
#endif

    int timestep = 0;

    char filename[256];

    // append file name
    sprintf(filename,"%s/%s.bp", filepath, "atom");
    ADIOS_FILE * f = adios_read_open (filename, method, comm, ADIOS_LOCKMODE_CURRENT, 0);

     if (f == NULL)
    {
        printf ("rank %d, %s\n",rank, adios_errmsg());
        return -1;
    }
    
    if(rank ==0)
        printf("reader opened the stream\n");

    ADIOS_VARINFO * v = adios_inq_var (f, "atom");

    /* Using less readers to read the global array back, i.e., non-uniform */
    uint64_t slice_size = v->dims[0]/size;
    start[0] = slice_size * rank;
    if (rank == size-1) /* last rank may read more lines */
        slice_size = slice_size + v->dims[0]%size;
    count[0] = slice_size;

    start[1] = 0;
    count[1] = v->dims[1];
    printf("rank %d: start: (%ld, %ld), count:( %ld, %ld)\n", rank, start[0], start[1], count[0], count[1]);
       

    data = malloc (slice_size * v->dims[1]* sizeof (double));
    if (data == NULL)
    {
        fprintf (stderr, "malloc failed.\n");
        return -1;
    }

    sel = adios_selection_boundingbox (v->ndim, start, count);

    //printf("rank %d: adios init complete\n", rank);

    // get read status, before perform mpiio!, so streaming steps are not affected
    int errno_streaming_read = adios_errno;
    //for(timestep = 0; timestep < 10;){
    while(errno_streaming_read != err_end_of_stream){
        if(rank == 0)
            printf("rank %d: Step %d start\n", rank, timestep);
        //ADIOS_FILE * f = adios_read_open ("adios_global.bp", method, comm, ADIOS_LOCKMODE_NONE, 0);
        //ADIOS_FILE * f = adios_read_open ("adios_global.bp", method, comm, ADIOS_LOCKMODE_ALL, 0);
           /* Read a subset of the temperature array */
        adios_schedule_read (f, sel, "atom", 0, 1, data);
        adios_perform_reads (f, 1);

        adios_release_step(f);
        adios_advance_step(f, 0, -1);

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
            insert_into_adios(filepath, "restart", slice_size, v->dims[1], data,"w", &comm);
        else
            insert_into_adios(filepath, "restart", slice_size, v->dims[1], data,"a", &comm);
        if(rank ==0)
            printf("rank %d: Step %d data kept\n", rank, timestep);
#endif

        // analysis
        run_analysis(data, slice_size, lp );

        if(rank ==0)
            printf("rank %d: Step %d moments calculated\n", rank, timestep);
        timestep ++;

    }

#ifdef ENABLE_TIMING
  MPI_Barrier(comm);
  double t_end = get_cur_time();
  if(rank == 0){
      printf("stat:Consumer end  at %lf \n", t_end);
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

    MPI_Finalize ();
    printf("rank %d: exit\n", rank);
    return 0;
}
