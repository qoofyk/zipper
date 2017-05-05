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
#ifdef ENABLE_TIMING
    double t0, t1, t2, t3, t4;
    double t_prepare, t_get, t_close, t_analy;
    t_prepare=0;
    t_get = 0;
    t_close = 0;
    t_analy = 0;
#endif



    int         rank, nprocs;
    MPI_Comm    comm = MPI_COMM_WORLD;
    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DATASPACES;
    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DIMES;
    enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_BP;
    ADIOS_SELECTION * sel;
    void * data = NULL;
    uint64_t start[2], count[2];

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &nprocs);

    int timestep = -1;
    
    /**** use index file to keep track of current step *****/
    int fd; 
    char step_index_file [256];
    int time_stamp = -1;// flag read from producer
    sprintf(step_index_file, "%s/stamp.file", filepath);

    MPI_Barrier(comm);

    adios_read_init_method (method, comm, "verbose=3");
    printf("rank %d:reader init\n", rank);

    char filename_atom[256];
    ADIOS_FILE * f;

    ADIOS_VARINFO * v;

    /* Using less readers to read the global array back, i.e., non-uniform */
    uint64_t slice_size;
      
    //for(timestep = 0; timestep < 10;){
    //
    // write will send -2 to mark end

    // flag whether producer finishes all the steps
    int has_more = 1;
    while(has_more){
        if(rank ==0){
            fd = open(step_index_file, O_RDONLY);
            if(fd == -1){
                perror("indexfile not here wait for 1 s \n");
       
            }
            else{
                flock(fd, LOCK_SH);
                read(fd,  &time_stamp,  sizeof(int));
                flock(fd, LOCK_UN);
                close(fd);
            }
            if(time_stamp  == -2){
                printf("producer  terminate\n");
                time_stamp = nstop-1;
                // run this gap then exit
            }

        }
        // broadcast stamp
        MPI_Bcast(&time_stamp, 1, MPI_INT, 0, comm);

        if(time_stamp ==-1){
                sleep(1);
                continue;
        }

        if(time_stamp == nstop -1){
            has_more = 0;
        }
        // new step avaible from producer
        while(timestep < time_stamp){
            timestep++;
            if(rank ==0)
                printf("----reader opened step %d\n", timestep);
            sprintf(filename_atom, "%s/atom_%d.bp", filepath, timestep);

            t0 = get_cur_time();
            f = adios_read_open_file(filename_atom, method, comm);


            // the first timestep will get variable metadata
            if(timestep == 0){
                 if (f == NULL)
                {
                    printf ("%s\n", adios_errmsg());
                    return -1;
                }
                
                v = adios_inq_var (f, "atom");
                slice_size = v->dims[0]/nprocs;

                data = malloc (slice_size * v->dims[1]* sizeof (double));
                if (data == NULL)
                {
                    fprintf (stderr, "malloc failed.\n");
                    return -1;
                }

                start[0] = slice_size * rank;
                if (rank == nprocs-1) /* last rank may read more lines */
                    slice_size = slice_size + v->dims[0]%nprocs;
                count[0] = slice_size;

                start[1] = 0;
                count[1] = v->dims[1];
                //printf("rank %d: start: (%ld, %ld), count:( %ld, %ld)\n", rank, start[0], start[1], count[0], count[1]);
                sel = adios_selection_boundingbox (v->ndim, start, count);
            }

            /* Read a subset of the temperature array */
            adios_schedule_read (f, sel, "atom", 0, 1, data);

            // timer for open and schedule
            t1 = get_cur_time();
            t_prepare+= t1-t0;
            
            // timer for actual read
            adios_perform_reads (f, 1);
            t2 = get_cur_time();
            t_get += t2-t1;

            // timer for closing file
            adios_read_close (f);
            t3 = get_cur_time();
            t_close += t3-t2;

            if(rank ==0)
                printf("Step %d read\n", timestep);
            // analysis
            run_analysis(data, slice_size, lp);

            t4 = get_cur_time();
            t_analy += t4-t3;
            if(rank == 0){
                printf("rank %d: Step %d moments calculated,t_prepare %lf, t_read %lf, t_close %lf, t_analy %lf\n", rank, timestep, t1-t0, t2-t1, t3-t2, t4-t3);
            }
        }
    }

    free (data);
    adios_selection_delete (sel);
    adios_free_varinfo (v);

#ifdef ENABLE_TIMING
    MPI_Barrier(comm);
    double t_end = get_cur_time();

        double global_t_prepare=0;
        double global_t_get=0;
        double global_t_close=0;
        double global_t_analy=0;
        MPI_Reduce(&t_prepare, &global_t_prepare, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
        MPI_Reduce(&t_get, &global_t_get, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
        MPI_Reduce(&t_close, &global_t_close, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
        MPI_Reduce(&t_analy, &global_t_analy, 1, MPI_DOUBLE, MPI_SUM, 0, comm);
 
    if(rank == 0){
      printf("stat:Consumer end  at %lf \n", t_end);
      printf("stat:time for prepare %fs, read %f s; time for close %f s; time for analy %f s\n",global_t_prepare/nprocs, global_t_get/nprocs, global_t_close/nprocs, global_t_analy/nprocs);
    }
#endif
    

    MPI_Barrier (comm);
    adios_read_finalize_method (method);
    MPI_Finalize ();
    printf("rank %d: exit\n", rank);
    return 0;
}
