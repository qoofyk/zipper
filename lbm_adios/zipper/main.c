/*#include "concurrent.h"*/
/*#include "lbm.h"*/
#include "run_module_lbm.h"

#ifdef WRITE_ONE_FILE
void comp_open_one_big_file(GV gv){
  char file_name[256];
  int i=0;

  sprintf(file_name, "%s/results/cid%d", gv->filepath, gv->rank[0]);
  // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
  while(gv->fp==NULL){

    gv->fp=fopen(file_name,"wb");

    i++;

    if(i>1)
		usleep(OPEN_USLEEP);

	if(i>TRYNUM){
    	printf("Fatal Error: Comp_Proc%d open an empty big file\n", gv->rank[0]);
    	fflush(stdout);
    	break;
	}
  }

}
#endif //WRITE_ONE_FILE

#ifdef WRITE_ONE_FILE
void ana_open_one_big_file(GV gv){
  int i, j, src;
  char file_name[256];

  for(j=0;j<gv->computer_group_size;j++){

    src=(gv->rank[0]-gv->compute_process_num)*gv->computer_group_size+j;

    sprintf(file_name, "%s/results/cid%d", gv->filepath, src);

    i=0;

    while((gv->ana_read_fp[j]==NULL) || (gv->ana_write_fp[j]==NULL)){

		gv->ana_read_fp[j]=fopen(file_name,"rb");
		gv->ana_write_fp[j]=fopen(file_name,"rb+");

		i++;
		if(i>1)
			usleep(OPEN_USLEEP);

		if(i>TRYNUM){
	    	printf("Fatal Error: Ana_Proc%d read an empty big file from%d\n", gv->rank[0], src);
	    	fflush(stdout);
	    	break;
		}

    }

  }
}
#endif //WRITE_ONE_FILE

int main(int argc, char **argv){

	MPI_Comm mycomm;
	LV              lvs;
	GV              gv;
	int provided;

	// char *cname[] = { "Compute_Group", "Analysis_Group", "BLUE" };

	int i;

	pthread_t       *thrds;
	pthread_attr_t  *attrs;
	void            *retval;

	double t0=0,t1=0;


	if(argc != 13) {
		fprintf(stderr, "Usage: %s generator_num, writer_num, reader#, writer_thousandth, \
computer_group_size, num_analysis_nodes, cubex, cubez, step_stop, n_moment\n", argv[0]);
		exit(1);
	}

	gv    = (GV) malloc(sizeof(*gv));


	//init lbm
	gv->cubex	=	atoi(argv[8]);
	gv->cubey	=	atoi(argv[8]);
	gv->cubez	=	atoi(argv[9]);

	// int myfilesize2produce = atoi(argv[13]);
 //    int dims_cube[3] = {myfilesize2produce/4,myfilesize2produce/4,myfilesize2produce};
 //    int nx, ny, nz;

	// nx = dims_cube[0];
 //    ny = dims_cube[1];
 //    nz = dims_cube[2];

    gv->X 	=	nx/gv->cubex;
	gv->Y 	=	ny/gv->cubey;
	gv->Z 	= 	nz/gv->cubez;

	gv->step_stop = atoi(argv[10]);

	gv->n_moments = atoi(argv[11]); //Loop times in Analysis calc_n_moments
	gv->writer_prb_thousandth = atoi(argv[12]); //writer start to get element

	//init IObox
	gv->compute_generator_num = atoi(argv[1]);
	gv->compute_writer_num = atoi(argv[2]);
	gv->analysis_reader_num = atoi(argv[3]);
	gv->analysis_writer_num = atoi(argv[4]);

	gv->data_id = 0;
	// gv->mpi_send_progress_counter = 0;

	gv->block_size = sizeof(int)*4+sizeof(double)*(gv->cubex*gv->cubey*gv->cubez*EACH_FLUID_NUM_DOUBLE); //64K B+4B,step,ci,cj,ck,data

  	gv->cpt_total_blks = gv->X*gv->Y*gv->Z*gv->step_stop;
  	gv->writer_thousandth  = atoi(argv[5]);
  	gv->writer_blk_num = gv->cpt_total_blks*gv->writer_thousandth/1000;
  	gv->sender_blk_num = gv->cpt_total_blks - gv->writer_blk_num;
  	gv->total_file = gv->cpt_total_blks*gv->block_size/(1024.0*1024.0); //KB

  	gv->compute_data_len = sizeof(int)+sizeof(char)*gv->block_size+sizeof(int)*(gv->writer_blk_num+1);//blk_id,step,ci,cj,ck,data,written_id
  	gv->analysis_data_len = sizeof(int)*4+sizeof(char)*gv->block_size; // src,blkid,writer_state,consumer_state | step,ci,cj,ck,data

  	gv->computer_group_size=atoi(argv[6]);
  	gv->analysis_process_num=atoi(argv[7]);
  	//gv->msleep = atof(argv[8]);
  	gv->compute_process_num = gv->computer_group_size * gv->analysis_process_num;
  	gv->calc_counter = 0;
  	// gv->prefetch_counter = 0;

  	gv->nproc_per_mac = atoi(getenv("nproc_per_mac"));
  	gv->filepath = getenv("SCRATCH_DIR");
    if(gv->filepath == NULL){
        fprintf(stderr, "scratch dir is not set!\n");
    }

	// MPI_Init(&argc, &argv);
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &gv->rank[0]);
	MPI_Comm_size(MPI_COMM_WORLD, &gv->size[0]);
	MPI_Get_processor_name(gv->processor_name, &gv->namelen);
	// printf("Hello world! I'm rank %d of %d on %s with provided=%d, block_size=%d, cube_size=%d double\n",
	// 	gv->rank[0], gv->size[0], gv->processor_name, provided, gv->block_size, gv->cubex*gv->cubey*gv->cubez*2);
	// fflush(stdout);

	if(gv->rank[0]<gv->compute_process_num)
	  //Compute Group
	  gv->color = 0;
	else
	  //Analysis Group
	  gv->color = 1;

	MPI_Comm_split(MPI_COMM_WORLD, gv->color, gv->rank[0], &mycomm);

	MPI_Comm_rank(mycomm, &gv->rank[1]);

	MPI_Comm_size(mycomm, &gv->size[1]);

	// printf("%d: I’m  rank %d of %d in the %s context\n", gv->rank[0], gv->rank[1], gv->size[1], cname[gv->color]);
	// fflush(stdout);

	MPI_Barrier(MPI_COMM_WORLD);

	//debug_print(gv->rank[0]);

	if (gv->color == 0){

//---------------------------- Init DataBroker ---------------------------------------------------//
		//LBM <--> comp_writer, comp_sender
		int num_total_thrds = gv->compute_writer_num+1;
		lvs   = (LV) malloc(sizeof(*lvs)*num_total_thrds);
		thrds = (pthread_t*) malloc(sizeof(pthread_t)*num_total_thrds);
		attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*num_total_thrds);

		gv->all_lvs = lvs;

		//producer_ring_buffer initialize
		ring_buffer producer_rb;
	    producer_rb.bufsize = PRODUCER_RINGBUFFER_TOTAL_MEMORY/(gv->block_size-4*sizeof(int));
	    producer_rb.head = 0;
	    producer_rb.tail = 0;
	    producer_rb.num_avail_elements = 0;
	    producer_rb.buffer = (char**)malloc(sizeof(char*)*producer_rb.bufsize);
	    producer_rb.lock_ringbuffer = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	    producer_rb.full = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	    producer_rb.empty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	    pthread_mutex_init(producer_rb.lock_ringbuffer, NULL);
	    pthread_cond_init(producer_rb.full, NULL);
	    pthread_cond_init(producer_rb.empty, NULL);
	    gv->producer_rb_p = &producer_rb;

	    //gv->producer_rb_p = rb_init(gv,PRODUCER_RINGBUFFER_TOTAL_MEMORY,&producer_rb);
	    //compute node
	    if(gv->rank[0]==0 || gv->rank[0]==(gv->compute_process_num-1)){
	    	printf("Comp_Proc%04d of %d on %s: Gen %.1fMB, cpt_total_blks=%d, H_writer_blk#=%d, sender_blk#=%d, \
PRB %.3fGB, size=%d, blk_size=%d, cube_size=%d double\n",
			gv->rank[0], gv->size[1], gv->processor_name, gv->total_file, gv->cpt_total_blks, gv->writer_blk_num, gv->sender_blk_num,
			PRODUCER_RINGBUFFER_TOTAL_MEMORY/(1024.0*1024.0*1024.0), gv->producer_rb_p->bufsize,
			gv->block_size, gv->cubex*gv->cubey*gv->cubez*EACH_FLUID_NUM_DOUBLE);
		    fflush(stdout);
	    }

	    gv->writer_on = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	    pthread_cond_init(gv->writer_on, NULL);
		gv->written_id_array = (int *) malloc(sizeof(int)*gv->writer_blk_num);
		gv->send_tail = 0;

		gv->flag_sender_get_finalblk = 0;
		gv->flag_writer_get_finalblk = 0;
		gv->writer_exit = 0;
		// pthread_mutex_init(&gv->lock_writer_exit, NULL);
		// pthread_cond_init(&gv->writer_exit, NULL);

#ifndef MPI_NOKEEP
#ifdef WRITE_ONE_FILE
		//init gv->fp
    	gv->fp = NULL;
		comp_open_one_big_file(gv);
		MPI_Barrier(MPI_COMM_WORLD);
		// printf("Comp_Proc%d: Pass MPI_Barrier\n", gv->rank[0]);
		// fflush(stdout);
#endif //WRITE_ONE_FILE
#endif //MPI_NOKEEP
		//init lock
		pthread_mutex_init(&gv->lock_block_id, NULL);
	    pthread_mutex_init(&gv->lock_disk_id_arr, NULL);
	    pthread_mutex_init(&gv->lock_writer_done, NULL);

#ifdef ADD_NETWORK
		//print network info
		if(gv->rank[0]%gv->nproc_per_mac==0){
			char command[256];

			sprintf(command, "/usr/sbin/opagetvf >> %s/Comp%d_Diskpercent%d", gv->filepath, gv->rank[0], gv->writer_thousandth);
			system(command);

			sprintf(command, "/usr/sbin/opafabricinfo >> %s/Comp%d_Diskpercent%d", gv->filepath, gv->rank[0], gv->writer_thousandth);
			system(command);

			sprintf(command, "/usr/sbin/opasmaquery -o nodeaggr >> %s/Comp%d_Diskpercent%d", gv->filepath, gv->rank[0], gv->writer_thousandth);
			system(command);

			sprintf(command, "/usr/sbin/opasmaquery -o slsc >> %s/Comp%d_Diskpercent%d", gv->filepath, gv->rank[0], gv->writer_thousandth);
			system(command);

		   	sprintf(command, "/usr/sbin/opapmaquery -o clearportstatus -n 0x2 >> %s/Comp%d_Diskpercent%d", gv->filepath, gv->rank[0], gv->writer_thousandth);
			system(command);
		}
#endif
//--------------------------------------------------------------------------------------------//

		t0=MPI_Wtime();

		/* Create threads */
		for(i=0; i<num_total_thrds; i++) {
		  init_lv(lvs+i, i, gv);
		  if(pthread_attr_init(attrs+i)) perror("attr_init()");
		  if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
		  if(pthread_create(thrds+i, attrs+i, compute_node_do_thread, lvs+i)) {
		    perror("pthread_create()");
		    exit(1);
		  }
		}

		// run_lbm(gv, dims_cube, &mycomm);
		run_module_lbm(gv, &mycomm);


		/* Join threads */
		for(i = 0; i < num_total_thrds; i++) {
		  pthread_join(thrds[i], &retval);
		  // printf("Compute Node %d Thread %d is finished\n", gv->rank[0] ,i);
		}

#ifndef MPI_NOKEEP
#ifdef WRITE_ONE_FILE
		fclose(gv->fp);
#endif //WRITE_ONE_FILE
#endif //MPI_NOKEEP

		t1=MPI_Wtime();
		free(lvs);
		free(attrs);
		free(thrds);

		printf("Comp_Proc%04d: Task finish on %s, T_comp_total=%.3f\n", gv->rank[0], gv->processor_name, t1-t0);
		fflush(stdout);
	}
	else {
		//Ana_Proc
		gv->ana_total_blks = gv->computer_group_size * gv->cpt_total_blks;

		gv->reader_blk_num = gv->computer_group_size * gv->writer_blk_num;
		gv->analysis_writer_blk_num = gv->ana_total_blks - gv->reader_blk_num;

		//ana_receiver, ana_reader <--> ana_writer, ana_consumer
		int num_total_thrds= gv->analysis_reader_num+gv->analysis_writer_num+2;
		lvs   = (LV) malloc(sizeof(*lvs)*num_total_thrds);
		thrds = (pthread_t*) malloc(sizeof(pthread_t)*num_total_thrds);
		attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*num_total_thrds);

		gv->all_lvs = lvs;

		//Consumer_ring_buffer initialize
		ring_buffer consumer_rb;
	    consumer_rb.bufsize = CONSUMER_RINGBUFFER_TOTAL_MEMORY/(gv->block_size-4*sizeof(int));
	    consumer_rb.head = 0;
	    consumer_rb.tail = 0;
	    consumer_rb.num_avail_elements = 0;
	    consumer_rb.buffer = (char**)malloc(sizeof(char*)*consumer_rb.bufsize);
	    consumer_rb.lock_ringbuffer = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
	    consumer_rb.full = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	    consumer_rb.empty = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	    consumer_rb.new_tail = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	    pthread_mutex_init(consumer_rb.lock_ringbuffer, NULL);
	    pthread_cond_init(consumer_rb.full, NULL);
	    pthread_cond_init(consumer_rb.empty, NULL);
	    pthread_cond_init(consumer_rb.new_tail, NULL);
	    gv->consumer_rb_p = &consumer_rb;

	    //gv->consumer_rb_p = rb_init(gv,CONSUMER_RINGBUFFER_TOTAL_MEMORY,&consumer_rb);

	    if(gv->rank[0]==gv->compute_process_num || gv->rank[0]==(gv->compute_process_num+gv->analysis_process_num-1)){
	    	printf("Ana_Proc%d of %d on %s: ana_total_blks=%d, reader_blk#=%d, analysis_writer_blk#=%d, \
CRB %.3fGB, size=%d\n",
	    		gv->rank[0], gv->size[1], gv->processor_name, gv->ana_total_blks, gv->reader_blk_num, gv->analysis_writer_blk_num,
	    		CONSUMER_RINGBUFFER_TOTAL_MEMORY/(1024.0*1024.0*1024.0), gv->consumer_rb_p->bufsize);
		    fflush(stdout);
	    }

		// gv->mpi_recv_progress_counter = 0;
	    gv->org_recv_buffer = (char *) malloc(gv->compute_data_len); //max length MIX_MSG
	    check_malloc(gv->org_recv_buffer);

	    // prfetch threads 1+1:cid+blkid
	    gv->recv_exit 		= 0;
	    gv->reader_exit 	= 0;
	    gv->ana_writer_exit = 0;

	    gv->prefetch_id_array = (int *) malloc(sizeof(int)*2*gv->reader_blk_num); //2: src, blk_id
	    gv->recv_head 		= 0;
	    gv->recv_tail 		= 0;
	    gv->recv_avail 		= 0;
	    check_malloc(gv->prefetch_id_array);
	    //initialize lock
	    gv->reader_on = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
	    pthread_cond_init(gv->reader_on, NULL);
	    pthread_mutex_init(&gv->lock_recv_disk_id_arr, NULL);

#ifndef MPI_NOKEEP
#ifdef WRITE_ONE_FILE
	    MPI_Barrier(MPI_COMM_WORLD);
	    //init every ana_fp[i]
		gv->ana_read_fp = (FILE **)malloc(sizeof(FILE *) * gv->computer_group_size);
		gv->ana_write_fp = (FILE **)malloc(sizeof(FILE *) * gv->computer_group_size);
		for(i=0; i<gv->computer_group_size; i++){
			gv->ana_read_fp[i]=NULL;
			gv->ana_write_fp[i]=NULL;
		}
		ana_open_one_big_file(gv);
		// printf("Ana_Proc%d: Pass MPI_Barrier\n", gv->rank[0]);
		// fflush(stdout);
#endif //WRITE_ONE_FILE
#endif //MPI_NOKEEP


	    t0=MPI_Wtime();

		/* Create threads */
		for(i=0; i<num_total_thrds; i++) {
		  init_lv(lvs+i, i, gv);
		  if(pthread_attr_init(attrs+i)) perror("attr_init()");
		  if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
		  if(pthread_create(thrds+i, attrs+i, analysis_node_do_thread, lvs+i)) {
		    perror("pthread_create()");
		    exit(1);
		  }
		}

		/* Join threads */
		for(i = 0; i < num_total_thrds; i++) {
		  pthread_join(thrds[i], &retval);
		  // printf("Analysis Node %d Thread %d is finished\n", gv->rank[0], i);
		}

#ifndef MPI_NOKEEP
#ifdef WRITE_ONE_FILE
		// for(i=0; i<gv->computer_group_size; i++)
		// 	fclose(gv->ana_fp[i]);
		free(gv->ana_read_fp);
    	free(gv->ana_write_fp);
#endif //WRITE_ONE_FILE
#endif //MPI_NOKEEP

		free(lvs);
		free(attrs);
		free(thrds);

		t1=MPI_Wtime();
		printf("Ana_Proc%04d on %s: Analysis Task finish! T_ana_total=%.3f\n",
			gv->rank[0], gv->processor_name, t1-t0);
		fflush(stdout);
	}

	MPI_Comm_free(&mycomm);
	MPI_Finalize();

	free(gv);

	return 0;
}

