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
#include <sys/file.h>

        
        

#define DEBUG_Feng

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
    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DATASPACES;
    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DIMES;
    enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_BP;
    ADIOS_SELECTION * sel;
    void * data = NULL;
    uint64_t start[2], count[2];

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &size);

    int timestep = -1;
    
    /**** use index file to keep track of current step *****/
    int fd; 
    char step_index_file [256];
    int time_stamp = -1;// flag read from producer
    sprintf(step_index_file, "%s/stamp.file", filepath);

    if(rank == 0){
        fd = open(step_index_file, O_WRONLY);
        flock(fd, LOCK_EX);
        write(fd,  &time_stamp,  sizeof(int));
        close(fd);
        flock(fd, LOCK_UN);
    }

    MPI_Barrier(comm);

    adios_read_init_method (method, comm, "verbose=3");

    char filename_atom[256];
    sprintf(filename_atom, "%s/atom.bp", filepath);
    ADIOS_FILE * f = adios_read_open (filename_atom, method, comm, ADIOS_LOCKMODE_CURRENT, 0);

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
    //
    // write will send -2 to mark end
    while(time_stamp != -2){
        fd = open(step_index_file, O_RDONLY);
        flock(fd, LOCK_SH);
        read(fd,  &time_stamp,  sizeof(int));
        flock(fd, LOCK_UN);
        close(fd);
        
        // new step avaible from producer
        if(time_stamp > timestep){
            timestep = time_stamp;
            printf("rank %d: Step %d start\n", rank, timestep);
            //ADIOS_FILE * f = adios_read_open ("adios_global.bp", method, comm, ADIOS_LOCKMODE_NONE, 0);
            //ADIOS_FILE * f = adios_read_open ("adios_global.bp", method, comm, ADIOS_LOCKMODE_ALL, 0);
               /* Read a subset of the temperature array */
            adios_schedule_read (f, sel, "atom", 0, 1, data);
            adios_perform_reads (f, 1);

            adios_release_step(f);
            // newest available step
            adios_advance_step(f, 1, -1);

            printf("rank %d: Step %d read\n", rank, timestep);

            // analysis
            run_analysis(data, slice_size, lp );
            printf("rank %d: Step %d moments calculated\n", rank, timestep);
        }

    }
    free (data);
    adios_selection_delete (sel);
    adios_free_varinfo (v);
    adios_read_close (f);

    MPI_Barrier (comm);
    adios_read_finalize_method (method);
    MPI_Finalize ();
    printf("rank %d: exit\n", rank);
    return 0;
}
