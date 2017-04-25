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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "adios_read.h"
#include "adios_error.h"
//#include "adios_read_global.h"
#include "run_analysis.h"
#include "utility.h"
#include "common_utility.h"
#include "ds_adaptor.h"

#ifdef RAW_DSPACES
static char var_name[STRING_LENGTH];
static size_t elem_size=sizeof(double);
#endif
        

#define DEBUG_Feng

int main (int argc, char ** argv) 
{
    if(argc !=3){
        printf("need to specify timstop scratch path for i/o\n");
        exit(-1);
    }
    char filepath[256];
    int nstop = atoi(argv[1]);
    strcpy(filepath, argv[2]);

    int lp = 4;

    /******************** configuration stop ***********/

    int         rank, size;
    MPI_Comm    comm = MPI_COMM_WORLD;
    void * data = NULL;
    uint64_t start[2], count[2];

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);

    int timestep;
    
    /**** use index file to keep track of current step *****/
    MPI_Barrier(comm);

#ifdef RAW_DSPACES
    char msg[STRING_LENGTH];
    printf("trying init dspaces for %d process\n", nprocs);
    ret = dspaces_init(nprocs, 2, &gcomm, NULL);

    printf("dspaces init successfuly \n");

    if(ret == 0){
        sprintf(msg, "dataspaces init successfully");
        my_message(msg, rank, LOG_CRITICAL);
    }else{
        sprintf(msg, "dataspaces init error");
        my_message(msg, rank, LOG_CRITICAL);
        exit(-1);
    }

    /*
    * set bounds and dspaces variables
    */
    sprintf(var_name, "atom");


    // data layout
//#ifdef FORCE_GDIM
    int n = dims_cube[0]*dims_cube[1]*dime_cube[2];
    uint64_t gdims[2] = {2, n};
    dspaces_define_gdim(var_name, 2,gdims);
//#endif


#endif

    for(timestep=0; timestep < MAX_STEP; timestep++){
    
        uint64_t slice_size;
        int start[2], count[2];
                   
        slice_size = n/size;

        data = malloc (slice_size * SIZEONE* sizeof (double));
        if (data == NULL)
        {
            fprintf (stderr, "malloc failed.\n");
            return -1;
        }

        start[0] = slice_size * rank;
        if (rank == size-1) /* last rank may read more lines */
            slice_size = slice_size + v->dims[0]%size;
        count[0] = slice_size;

        start[1] = 0;
        count[1] = 2;
                //printf("rank %d: start: (%ld, %ld), count:( %ld, %ld)\n", rank, start[0], start[1], count[0], count[1]);

        int bounds[6] = {0};
        double time_comm;
        bounds[1]=start[0];
        bounds[0]=start[1]
        bounds[4]=start[0]+count[0]-1;
        bounds[3]=start[1]+count[1]-1;

        get_common_buffer(step,2, bounds,rank, &comm, var_name, (void **)&data, elem_size, &time_comm);
        if(rank ==0)
            printf("Step %d read\n", timestep);
        // analysis
        run_analysis(data, slice_size, lp);
        if(rank == 0)
            printf("Step %d moments calculated\n", timestep);
    }
    MPI_Barrier(comm);
    double t_end = get_cur_time();
    if(rank == 0){
      printf("stat:Consumer end  at %lf \n", t_end);
    }

    free (data);

#ifdef RAW_DSPACES
    dspaces_finalize();
#endif
    

    MPI_Finalize ();
    printf("rank %d: exit\n", rank);
    return 0;
}
