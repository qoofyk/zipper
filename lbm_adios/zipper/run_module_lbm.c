#include "run_module_lbm.h"

extern double u[nx][ny][nz],v[nx][ny][nz];
extern double u_r;

status_t insert_zipper(GV gv){
    int i, j, k;
	/*create_prb_element*/
	char* buffer;
	// FILE *fp;
	// char file_name[64];
	int count=0;
	int full=0;

	// cubex=4;
	// cubey=4;
	// cubez=4;

	// X=nx/cubex;
	// Y=ny/cubey;
	// Z=nz/cubez;

	// #ifdef DEBUG_PRINT
	// printf("Compute Node %d Generator start create_prb_element!\n",gv->rank[0]);
	// fflush(stdout);
	// #endif //DEBUG_PRINT

	for(gv->CI=0;gv->CI<gv->X;gv->CI++)
	  for(gv->CJ=0;gv->CJ<gv->Y;gv->CJ++)
	    for(gv->CK=0;gv->CK<gv->Z;gv->CK++){
	      gv->originx=gv->CI*gv->cubex;
	      gv->originy=gv->CJ*gv->cubey;
	      gv->originz=gv->CK*gv->cubez;

	      buffer = (char*) malloc(sizeof(char)*gv->compute_data_len);
	      check_malloc(buffer);

	      ((int *)buffer)[0] = gv->data_id++;
	      ((int *)buffer)[1] = gv->step;
	      ((int *)buffer)[2] = gv->CI;
	      ((int *)buffer)[3] = gv->CJ;
	      ((int *)buffer)[4] = gv->CK;

	      count=0;
	      for(i=0;i<gv->cubex;i++)
	        for(j=0;j<gv->cubey;j++)
	          for(k=0;k<gv->cubez;k++){
	            gv->gi=gv->originx+i;
	            gv->gj=gv->originy+j;
	            gv->gk=gv->originz+k;

	            //ur: reference velocity
	            ((double *)(buffer+sizeof(int)*5))[count]   = u_r*u[gv->gi][gv->gj][gv->gk]; 	//vx
                ((double *)(buffer+sizeof(int)*5))[count+1] = u_r*v[gv->gi][gv->gj][gv->gk];	//vy
                // ((double *)(buffer+sizeof(int)*5))[count+2] = u_r*w[gv->gi][gv->gj][gv->gk];	//vz
                // ((double *)(buffer+sizeof(int)*5))[count+3] = rho[gv->gi][gv->gj][gv->gk];	//rho

#ifdef ORIGINAL_OUTPUT
                if(gv->step%100==0 && i==(gv->cubex/2) && (j==(gv->cubey-8)||j==8) && (k==8 || k==(gv->cubez-8)) ){
                	printf("Proc%d: step=%d, (%d %d %d), u_r=_%.3f, u_r*u=%.3f, u_r*v=%.3f, u_r*w=%e, rho=%e\n",
                		gv->rank[0], gv->step, gv->gi, gv->gj, gv->gk, u_r,
                		u_r*u[gv->gi][gv->gj][gv->gk], u_r*v[gv->gi][gv->gj][gv->gk], w[gv->gi][gv->gj][gv->gk],
                		rho[i][j][k]);
                	fflush(stdout);
                }

#endif
	            count+=EACH_FLUID_NUM_DOUBLE;
	          }
	      //total produce X*Y*Z blocks each step

	      producer_ring_buffer_put(gv, buffer, &full);

	      // sprintf(file_name,"/N/dc2/scratch/fuyuan/inter/id%d_v&u_step%03d_blk_k%04d_j%04d_i%04d.data",myid,step,CI,CJ,CK);
	      // fp=fopen(file_name,"wb");
	      // fwrite(buffer, count, 1, fp);
	      // fclose(fp);
	    }

    return S_OK;
}

status_t generate_exit_msg(GV gv){
	char* buffer;
	int full;

	// generate exit message
	buffer = (char*) malloc(sizeof(char)*gv->compute_data_len);
	check_malloc(buffer);

	((int *)buffer)[0]= EXIT_BLK_ID;
    // ((int *)buffer)[1]= -1;
    // ((int *)buffer)[2]= -1;

#ifdef DEBUG_PRINT
    printf("Comp_Proc%d: LBM generate the EXIT block_id=%d in timestep=%d with total_blks=%d\n",
      gv->rank[0], ((int *)buffer)[0], gv->step, gv->data_id);
    fflush(stdout);
#endif //DEBUG_PRINT

    producer_ring_buffer_put(gv, buffer, &full);

    return S_OK;
}


status_t run_module_lbm(GV gv, MPI_Comm *pcomm){

	int i;

	/*those are all the information io libaray need to know about*/
    MPI_Comm comm=*pcomm;
	int nlocal; //nlines processed by each process
	int size_one = SIZE_ONE; // each line stores 2 doubles
	double *buffer; // buffer address

    int nsteps =gv->step_stop;

    // int filesize2produce = atoi(argv[12]);
    int dims_cube[3] = {filesize2produce/4,filesize2produce/4,filesize2produce};
    //strcpy(filepath, argv[3]);

    /* prepare */
    // MPI_Init(&argc, &argv);
    // comm = MPI_COMM_WORLD;

    int    rank, nprocs;
    MPI_Comm_rank (comm, &rank);
    // MPI_Comm_size (comm, &nprocs);

	nlocal = dims_cube[0]*dims_cube[1]*dims_cube[2];
	// lbm_alloc_buffer(&comm, nlocal, size_one, &buffer);

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

		status_t status = insert_zipper(gv);
	}

	//generate exit message
	status_t status = generate_exit_msg(gv);

	if(S_OK != lbm_finalize(&comm)){
		fprintf(stderr, "[lbm]: err when finalized\n");
	}


    MPI_Barrier(comm);
    //printf("[lbm]: reached the barrier\n");

cleanup:
  MPI_Finalize();
  if(rank == 0)
    printf("now exit! \n");
  return S_OK;
}
