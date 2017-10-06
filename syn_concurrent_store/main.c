#include "do_thread.h"

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

	ring_buffer producer_rb;
	ring_buffer consumer_rb;

	double t0=0, t1=0;

	if(argc != 13) {
		fprintf(stderr, "Usage: %s writer#, reader#, block_size*64KB, num_blks, computer_group_size, analysis_process_num, usleep\n", argv[0]);
		exit(1);
	}

	gv    = (GV) malloc(sizeof(*gv));

	gv->compute_generator_num = atoi(argv[1]);
	gv->compute_writer_num = atoi(argv[2]);
	gv->analysis_reader_num = atoi(argv[3]);
	gv->analysis_writer_num = atoi(argv[4]);

	gv->data_id = 0;
	// gv->mpi_send_progress_counter = 0;

	gv->block_size = atoi(argv[5])*1024*64; //64K B

  	gv->cpt_total_blks = atoi(argv[6]);
  	gv->writer_thousandth  = atoi(argv[7]);
  	gv->writer_blk_num = gv->cpt_total_blks*gv->writer_thousandth/1000;
  	gv->sender_blk_num = gv->cpt_total_blks - gv->writer_blk_num;
  	// gv->total_file = gv->cpt_total_blks*gv->block_size/(1024.0*1024.0); //MB
  	gv->total_file = gv->cpt_total_blks*atoi(argv[5])/16.0; //MB

  	gv->compute_data_len = sizeof(char)*gv->block_size+sizeof(int)*(gv->writer_blk_num+1);  //written_id_array + mpi_send_count
  	gv->analysis_data_len = sizeof(char)*gv->block_size+sizeof(int)*4; //src,blk_id,writer_state,consumer_state

  	gv->computer_group_size=atoi(argv[8]);
  	gv->analysis_process_num=atoi(argv[9]);
  	gv->lp=atoi(argv[10]);
  	gv->computation_lp=atoi(argv[11]);
  	// gv->utime=atoi(argv[11]);
  	gv->writer_prb_thousandth = atoi(argv[12]);
  	//gv->msleep = atof(argv[8]);
  	gv->compute_process_num = gv->computer_group_size * gv->analysis_process_num;
  	gv->calc_counter = 0;
  	// gv->prefetch_counter = 0;

  	gv->filepath = getenv("SCRATCH_DIR");
    if(gv->filepath == NULL){
        fprintf(stderr, "scratch dir is not set!\n");
    }

	// MPI_Init(&argc, &argv);
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &gv->rank[0]);
	MPI_Comm_size(MPI_COMM_WORLD, &gv->size[0]);
	MPI_Get_processor_name(gv->processor_name, &gv->namelen);
	// printf("Hello world! I’m rank %d of %d on %s\n", gv->rank[0], gv->size[0], gv->processor_name);
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


#ifdef ADD_PAPI
	/* Set TESTS_QUIET variable */
    tests_quiet( argc, argv );

    int papi_retval;
    /* PAPI Initialization */
    papi_retval = PAPI_library_init( PAPI_VER_CURRENT );
    if ( papi_retval != PAPI_VER_CURRENT ) {
        test_fail(__FILE__, __LINE__,"PAPI_library_init failed\n", papi_retval);
    }

    if (!TESTS_QUIET) {
        printf("Proc%d: Trying all infiniband events\n", gv->rank[0]);
    }

	if (PAPI_thread_init(pthread_self) != PAPI_OK){ //in main function
    	exit(1);
  	}
  	printf("papi threads initialized\n");
  	fflush(stdout);
#endif //ADD_PAPI

	if (gv->color == 0){
//---------------------------- Init DataBroker ---------------------------------------------------//
    	// generator_thread <--> compute_writer, compute_sender
    	int num_total_thrds = gv->compute_writer_num+2;
		lvs   = (LV) malloc(sizeof(*lvs)*num_total_thrds);
		thrds = (pthread_t*) malloc(sizeof(pthread_t)*num_total_thrds);
		attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*num_total_thrds);

		gv->all_lvs = lvs;

		//producer_ring_buffer initialize
	    producer_rb.bufsize = PRODUCER_RINGBUFFER_TOTAL_MEMORY/gv->block_size;
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
	    	printf("Comp_Proc%04d of %d on %s: produce %.1fMB, cpt_total_blks=%d, writer_blk#=%d, sender_blk#=%d, \
PRB %.3fGB, size=%d\n",
			gv->rank[0], gv->size[1], gv->processor_name, gv->total_file, gv->cpt_total_blks, gv->writer_blk_num, gv->sender_blk_num,
			PRODUCER_RINGBUFFER_TOTAL_MEMORY/(1024.0*1024.0*1024.0), gv->producer_rb_p->bufsize);
		    fflush(stdout);
	    }

		gv->written_id_array = (int *) malloc(sizeof(int)*gv->writer_blk_num);
		gv->send_tail = 0;

		gv->flag_sender_get_finalblk = 0;
		gv->flag_writer_get_finalblk = 0;
		gv->writer_exit = 0;

#ifdef WRITE_ONE_FILE
		//init gv->fp
    	gv->fp = NULL;
		comp_open_one_big_file(gv);
		MPI_Barrier(MPI_COMM_WORLD);
#endif //WRITE_ONE_FILE

		// printf("After comp open!\n");
	 //    fflush(stdout);

		//init lock
		pthread_mutex_init(&gv->lock_block_id, NULL);
	    pthread_mutex_init(&gv->lock_disk_id_arr, NULL);
	    pthread_mutex_init(&gv->lock_writer_done, NULL);
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

		/* Join threads */
		for(i=0; i <num_total_thrds; i++) {
		  pthread_join(thrds[i], &retval);
		  // printf("Compute Process %d Thread %d is finished\n", gv->rank[0] ,i);
		}

#ifdef WRITE_ONE_FILE
    	fclose(gv->fp);
#endif //WRITE_ONE_FILE

		t1=MPI_Wtime();

		free(lvs);
		free(attrs);
		free(thrds);

		printf("Comp_Proc%04d: Task finish on %s, T_comp_total=%.3f\n", gv->rank[0], gv->processor_name, t1-t0);
		fflush(stdout);
	}
	else{
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
	    consumer_rb.bufsize = CONSUMER_RINGBUFFER_TOTAL_MEMORY/gv->block_size;
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
	    	printf("Ana_Proc%04d of %d on %s: ana_total_#=%d, reader_blk#=%d, ana_writer_blk#=%d, \
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

	    gv->prefetch_id_array = (int *) malloc(sizeof(int)*2*gv->ana_total_blks); //2: src, blk_id
	    gv->recv_head = 0;
	    gv->recv_tail = 0;
	    gv->recv_avail = 0;
	    check_malloc(gv->prefetch_id_array);

	    // printf("Before ana open!\n");
	    // fflush(stdout);

#ifdef WRITE_ONE_FILE
    //wait for compute_proc create big files
    MPI_Barrier(MPI_COMM_WORLD);

    //init every ana_fp[i]
    gv->ana_read_fp = (FILE **)malloc(sizeof(FILE *) * gv->computer_group_size);
    gv->ana_write_fp = (FILE **)malloc(sizeof(FILE *) * gv->computer_group_size);
    for(i=0; i<gv->computer_group_size; i++){
    	gv->ana_read_fp[i]=NULL;
    	gv->ana_write_fp[i]=NULL;
    }
    ana_open_one_big_file(gv);
#endif //WRITE_ONE_FILE

    	// printf("After ana open!\n");
	    // fflush(stdout);

	    //initialize lock
	    pthread_mutex_init(&gv->lock_recv_disk_id_arr, NULL);
	    // pthread_mutex_init(&gv->lock_prefetcher_progress, NULL);

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
		for(i=0; i<num_total_thrds; i++) {
		  pthread_join(thrds[i], &retval);
		  // printf("Analysis Process %d Thread %d is finished\n", gv->rank[0], i);
		}

#ifdef WRITE_ONE_FILE
    // for(i=0; i<gv->computer_group_size; i++){

    //   printf("Ana_Proc%04d: prepare to close big file%d\n", gv->rank[0], i);
    //   fflush(stdout);

    //   if(gv->ana_fp[i]==NULL){
    //     printf("Ana_Proc%04d: big file%d pointer is NULL\n", gv->rank[0], i);
    //     fflush(stdout);
    //   }
    //   else{
    //     // if(fclose(gv->ana_fp[i])){
    //     //    printf("error closing file.");
    //     //    exit(-1);
    //     // }
    //   }


    //   printf("Ana_Proc%04d: successfully close big file %d\n", gv->rank[0], i);
    //   fflush(stdout);
    // }
		free(gv->ana_read_fp);
    	free(gv->ana_write_fp);
#endif //WRITE_ONE_FILE

		free(lvs);
		free(attrs);
		free(thrds);

		t1=MPI_Wtime();
		printf("Ana_Proc%04d: Task finish on %s, T_ana_total=%.3f\n",
		  gv->rank[0], gv->processor_name, t1-t0);
		fflush(stdout);
	}

	// MPI_Barrier(MPI_COMM_WORLD);
	MPI_Comm_free(&mycomm);
	MPI_Finalize();

	free(gv);

	return 0;
}

