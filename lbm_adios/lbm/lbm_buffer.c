#include "lbm.h"
#include "lbm_buffer.h"

extern double only_lbm_time, init_lbm_time; // timer can also be placed outside
extern double u_r;
extern double u[nx][ny][nz],v[nx][ny][nz];

static int    rank, nprocs; // will initialize in alloc_buffer


status_t lbm_alloc_buffer(MPI_Comm *pcomm, size_t nlocal, size_t size_one, double **pbuff){
        /* alloc io buffer */
        MPI_Comm comm = *pcomm;
        MPI_Comm_rank (comm, &rank);
        MPI_Comm_size (comm, &nprocs);

		*pbuff = (double *)malloc(nlocal*sizeof(double)*size_one);
		if(NULL == *pbuff) return S_FAIL;
        if(rank == 0){
            printf("[LBM INFO]: io buffer allocated\n");
        }
        return S_OK;
}

status_t lbm_get_buffer(double *buffer){		
    int gi, gj, gk;
    int count=0;

    for(gi = 0; gi < nx; gi++){
        for(gj = 0; gj < ny; gj++){
            for(gk = 0; gk < nz; gk++){
                *((double *)(buffer+count))=u_r*u[gi][gj][gk];
                *((double *)(buffer+count+1))=u_r*v[gi][gj][gk];
#ifdef debug_1
                printf("(%d %d %d), u_r=_%lf u=%lf v= %lf\n", gi, gj, gk, u_r, u[gi][gj][gk],v[gi][gj][gk]);
#endif
                count+=2;
            }
        }
    }

    return S_OK;
}

status_t lbm_free_buffer(MPI_Comm *pcomm, double *buffer){
    MPI_Comm comm = *pcomm;
	double global_t_cal=0;
	MPI_Reduce(&only_lbm_time, &global_t_cal, 1, MPI_DOUBLE, MPI_MAX, 0, comm);

	if(rank == 0){
		printf("t_prepare:%f s, max t_cal %f s\n", init_lbm_time, global_t_cal);
	}

	// MPI_Barrier(comm1d);

	if(buffer){
		free(buffer);
		//printf("io buffer freed\n");
	}
    return S_OK;
}




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
	int nlocal; //nlines processed by each process
	int size_one = SIZE_ONE; // each line stores 2 doubles
	double *buffer; // buffer address

    int nsteps =atoi(argv[1]);
    
    int dims_cube[3] = {filesize2produce/4,filesize2produce/4,filesize2produce};
    //strcpy(filepath, argv[3]);

	MPI_Init(&argc, &argv);

    MPI_Comm comm = MPI_COMM_WORLD;
    
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
		if(S_OK != lbm_advance_step(&comm)){
			fprintf(stderr, "[lbm]: err when process step %d\n", i);
		}
	
		// get the buffer
		if(S_OK != lbm_get_buffer(buffer)){
			fprintf(stderr, "[lbm]: err when updated buffer at step %d\n", i);

		}

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
