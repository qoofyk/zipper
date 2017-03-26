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

#define DEBUG_Feng

//#define HAS_KEEP

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

    int         rank, size, i, j, k;
    MPI_Comm    comm = MPI_COMM_WORLD;
    enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DATASPACES;
    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DIMES;
    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_BP;
    ADIOS_SELECTION * sel;
    void * data = NULL;
    uint64_t start[2], count[2];

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);
#ifdef HAS_KEEP
    adios_init ("adios_global.xml", comm);
    printf("rank %d: adios init complete\n", rank);
#endif



    adios_read_init_method (method, comm, "verbose=3");
    int timestep = 0;

    char filename[256];

    // append file name
    sprintf(filename,"%s/%s.bp", filepath, "atom");
    ADIOS_FILE * f = adios_read_open (filename, method, comm, ADIOS_LOCKMODE_CURRENT, 0);

     if (f == NULL)
    {
        printf ("%s\n", adios_errmsg());
        return -1;
    }
    
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
    printf("rank %d: start: (%d, %d), count:( %d, %d)\n", rank, start[0], start[1], count[0], count[1]);
       

    data = malloc (slice_size * v->dims[1]* sizeof (double));
    if (data == NULL)
    {
        fprintf (stderr, "malloc failed.\n");
        return -1;
    }

    sel = adios_selection_boundingbox (v->ndim, start, count);


    printf("rank %d: adios init complete\n", rank);
    //for(timestep = 0; timestep < 10;){
    while(adios_errno != err_end_of_stream){
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
        printf("rank %d: Step %d read\n", rank, timestep);
// keep 
#ifdef HAS_KEEP
        insert_into_adios(filepath, "restart", slice_size, v->dims[1], data, &comm);
#endif

        // analysis
        run_analysis(data, slice_size, lp );

        printf("rank %d: Step %d moments calculated\n", rank, timestep);
        timestep ++;

    }
    free (data);
    adios_selection_delete (sel);
    adios_free_varinfo (v);
    adios_read_close (f);

    MPI_Barrier (comm);
    adios_read_finalize_method (method);
#ifdef HAS_KEEP
  adios_finalize (rank);
  printf("rank %d: adios finalize complete\n", rank); 
#endif

    MPI_Finalize ();
    printf("rank %d: exit\n", rank);
    return 0;
}
