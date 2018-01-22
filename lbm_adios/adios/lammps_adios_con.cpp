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

/*#define CLOG_MAIN*/
//#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "adios_read.h"
#include "adios_error.h"
#include "run_msd.h"
//#include "adios_adaptor.h"
//#include "adios_helper.h"
#include "adios.h"
#include <assert.h>
#include "transports.h"
#include "utility.h"
static transport_method_t transport;

#define SIZE_ONE (5)

#ifdef V_T
#include <VT.h>
int class_id;
int analysis_id;
#endif

//#define DEBUG_Feng
int main (int argc, char ** argv){

     /*
     * @input
     * @param NSTOP
     */
    if(argc !=2){
        printf("lammps_adios_con nsteps\n");
        exit(-1);
    }

    int nsteps = atoi(argv[1]);
    double **msd;


    /* init adios */
    int         rank, nprocs;
    MPI_Comm    comm = MPI_COMM_WORLD;


    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_DIMES;
    //enum ADIOS_READ_METHOD method = ADIOS_READ_METHOD_BP;
    ADIOS_SELECTION * sel;
    double * data = NULL;
    uint64_t start[3], count[3];

    MPI_Init (&argc, &argv);
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &nprocs);

    char nodename[256];
    int nodename_length;
    MPI_Get_processor_name(nodename, &nodename_length );


#ifdef V_T
     VT_classdef( "Analysis", &class_id );
     VT_funcdef("ANL", class_id, &analysis_id);
#endif



    /******************** configuration stop ***********/

#ifdef ENABLE_TIMING
    double t1, t2, t3, t4;
    double t_read_1, t_read_2, t_analy;
    t_read_1 = 0;
    t_read_2 = 0;
    t_analy = 0;
#endif
    int step =0;

    /*
     * get transport method
     */
    transport = get_current_transport();
    uint8_t transport_major = get_major(transport);
    uint8_t transport_minor = get_minor(transport);
    printf("%s:I am rank %d of %d, tranport code %x-%x\n",
            nodename, rank, nprocs,
            get_major(transport), get_minor(transport) );

    if(rank == 0){
      printf("stat: Consumer start at %lf \n", MPI_Wtime());
    }
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
        fprintf(stderr, "ERROR: rank %d: adios init err with %d\n", rank, method);
        exit(-1);
    }
    else{
        fprintf(stderr, "adios read method init complete with %d\n", method);
    }


    char filename[256];

    // append file name
    sprintf(filename, "atom.bp");
    ADIOS_FILE * f = adios_read_open (filename, method, comm, ADIOS_LOCKMODE_CURRENT, -1);
    //ADIOS_FILE * f = adios_read_open (filename, method, comm, ADIOS_LOCKMODE_NONE, 0);

     if (f == NULL)
    {
        printf ("rank %d, %s\n",rank, adios_errmsg());
        return -1;
    }

    ADIOS_VARINFO * v = adios_inq_var (f, "array");
    /* Using less readers to read the global array back, i.e., non-uniform */


    /* 
     * get the dimension info
     */
    uint64_t slice_size = v->dims[1]/nprocs;
    start[1] = slice_size * rank;
    if (rank == nprocs-1) /* last rank may read more lines */
        slice_size = slice_size + v->dims[2]%nprocs;
    count[1] = slice_size;


    start[0] = 0;
    count[0] = v->dims[0];

    start[2] = 0;
    count[2] = v->dims[2];

    printf("rank %d: start: (%ld, %ld, %ld), count:( %ld, %ld, %ld)\n", rank, start[0], start[1], start[2], count[0], count[1], count[2]);

    int size_one = v->dims[0];
    int nlines = slice_size*(v->dims[2]);
       
    msd =  init_msd(nsteps, size_one);

    data = (double *)malloc (nlines*size_one* sizeof (double));
    memset(data, 0, nlines*size_one*sizeof(double));
    if (data == NULL)
    {
        fprintf (stderr, "malloc failed.\n");
        return -1;
    }

    sel = adios_selection_boundingbox (v->ndim, start, count);


    printf("rank %d: adios init complete\n", rank);

    double t_start = MPI_Wtime();
    while(adios_errno != err_end_of_stream){
        if(rank == 0){
            printf("rank %d: Step %d start\n", rank, step);
        }

        // read
        adios_schedule_read (f, sel, "array", 0, 1, data);
        adios_perform_reads (f, 1);
        adios_release_step(f);
        adios_advance_step(f, 0, -1);

        t1 =MPI_Wtime(); 

#ifdef V_T
      VT_begin(analysis_id);
#endif
        calc_msd(msd, data, nlines, size_one, step);
#ifdef V_T
      VT_end(analysis_id);
#endif
        t2 =MPI_Wtime(); 

        t_analy += t2-t1;

        step ++;
    }

    /*
     * reduce
     */
    perform_msd_reduce(msd, nsteps, comm);
    free_msd(msd, size_one);

    double t_end = MPI_Wtime();
    double global_t_analy;


    /*
     * output
     */
    MPI_Reduce(&t_analy, &global_t_analy, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    
    printf("[msd]: total-start-end %.3f %.3f %.3f\n", t_end- t_start, t_start, t_end);

    // terminate the task (mandatory) by sending a quit message to the rest of the workflow
    if(rank == 0){
		printf("[msd]: max t_analysis %.3fs, now existing\n", global_t_analy);
	}


    /*
     * finalize adios
     */

    free (data);
    adios_selection_delete (sel);
    adios_free_varinfo (v);
    adios_read_close (f);
    MPI_Barrier (comm);
    adios_read_finalize_method (method);

    MPI_Finalize ();
    return 0;
}
