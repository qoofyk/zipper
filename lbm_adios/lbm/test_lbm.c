#include "lbm.h"
#include "lbm_buffer.h"

#include <VT.h>

#define V_T
/*
 * test driver for lbm code
 */


int main(int argc, char * argv[]){

    /*
     * @input
     * @param NSTOP
     * @param FILESIZE2PRODUCE
     */
    if(argc !=2){
        printf("run_lbm nstep\n");
        exit(-1);
    }
	int i;

    


	/*those are all the information io libaray need to know about*/
    MPI_Comm comm;
	int nlocal; //nlines processed by each process
	int size_one = SIZE_ONE; // each line stores 2 doubles
	double *buffer; // buffer address

    int nsteps =atoi(argv[1]);
    
    int dims_cube[3] = {filesize2produce/4,filesize2produce/4,filesize2produce};
    //strcpy(filepath, argv[3]);

    /* prepare */
    MPI_Init(&argc, &argv);
    comm = MPI_COMM_WORLD;

#ifdef V_T
      int class_id;
      int advance_step_id, get_buffer_id;
      
      VT_classdef( "Computation", &class_id );
      VT_funcdef("ADVSTEP", class_id, &advance_step_id);
      VT_funcdef("GETBUF", class_id, &get_buffer_id);
#endif







    int    rank, nprocs;
    MPI_Comm_rank (comm, &rank);
    MPI_Comm_size (comm, &nprocs);

	nlocal = dims_cube[0]*dims_cube[1]*dims_cube[2];
	lbm_alloc_buffer(&comm, nlocal, size_one, &buffer);

	/* init lbm with dimension info*/
	if( S_FAIL == lbm_init(&comm, nsteps)){
		printf("[lbm]: init not success, now exit\n");
		goto cleanup;
	}


	if(rank == 0){
		printf("[lbm]: init with nlocal = %d size_one = %d\n", nlocal, size_one);
	}
	for(i = 0; i< nsteps; i++){

       // advance step
#ifdef V_T
      VT_begin(advance_step_id);
#endif
		if(S_OK != lbm_advance_step(&comm)){
			fprintf(stderr, "[lbm]: err when process step %d\n", i);
		}
#ifdef V_T
      VT_end(advance_step_id);
#endif
	
		// get the buffer
#ifdef V_T
      VT_begin(get_buffer_id);
#endif
		if(S_OK != lbm_get_buffer(buffer)){
			fprintf(stderr, "[lbm]: err when updated buffer at step %d\n", i);

		}
#ifdef V_T
      VT_end(get_buffer_id);
#endif

		// replace this line with different i/o libary
		if(S_OK != lbm_io_template(&comm, buffer, nlocal, size_one)){
			fprintf(stderr,"[lbm]: error when writing step %d \n", i);
		}
	}

	if(S_OK != lbm_finalize(&comm)){
		fprintf(stderr, "[lbm]: err when finalized\n");
	}
	if(S_OK != lbm_free_buffer(&comm, buffer)){
		fprintf(stderr, "[lbm]: err when free and summarize\n");
	}

	//run_lbm(filepath, step_stop, dims_cube, &comm);
    MPI_Barrier(comm);
    //printf("[lbm]: reached the barrier\n");

cleanup:
  MPI_Finalize();
  if(rank == 0)
    printf("now exit! \n");
  return 0;
}
