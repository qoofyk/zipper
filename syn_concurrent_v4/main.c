#include "concurrent.h"

int main(int argc, char **argv){

	// MPI_Comm mycomm;
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

	double t0=0,t1=0,t2=0,t3=0;

	if(argc != 11) {
		fprintf(stderr, "Usage: %s writer#, reader#, block_size*64KB, num_blks, computer_group_size, num_analysis_nodes, msleep\n", argv[0]);
		exit(1);
	}

	gv    = (GV) malloc(sizeof(*gv));

	gv->generator_num = atoi(argv[1]);
	gv->writer_num = atoi(argv[2]);
	gv->reader_num = atoi(argv[3]);

	gv->data_id = 0;
	gv->mpi_send_progress_counter = 0;

	gv->block_size = atoi(argv[4])*1024*64; //64K B

  	gv->cpt_total_blks = atoi(argv[5]);
  	gv->writer_thousandth  = atoi(argv[6]);
  	gv->writer_blk_num = gv->cpt_total_blks*gv->writer_thousandth/1000;
  	gv->sender_blk_num = gv->cpt_total_blks - gv->writer_blk_num;
  	gv->total_file = (long)gv->cpt_total_blks*(long)gv->block_size/1024; //KB
  	gv->compute_data_len = sizeof(char)*gv->block_size+sizeof(int)*(gv->writer_blk_num+1);
  	gv->analysis_data_len = sizeof(char)*gv->block_size+sizeof(int)*(1+1); // src,blkid,data


  	gv->computer_group_size=atoi(argv[7]);
  	gv->num_analysis_nodes=atoi(argv[8]);
  	gv->lp=atoi(argv[9]);
  	gv->utime=atoi(argv[10]);
  	//gv->msleep = atof(argv[8]);
  	gv->num_compute_nodes = gv->computer_group_size * gv->num_analysis_nodes;
  	gv->calc_counter = 0;
  	// gv->prefetch_counter = 0;

	// MPI_Init(&argc, &argv);
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &gv->rank[0]);
	MPI_Comm_size(MPI_COMM_WORLD, &gv->size[0]);
	MPI_Get_processor_name(gv->processor_name, &gv->namelen);
	// printf("Hello world! I’m rank %d of %d on %s\n", gv->rank[0], gv->size[0], gv->processor_name);
	// fflush(stdout);

	if(gv->rank[0]<gv->num_compute_nodes)
	  //Compute Group
	  gv->color = 0;
	else
	  //Analysis Group
	  gv->color = 1;

	// MPI_Comm_split(MPI_COMM_WORLD, gv->color, gv->rank[0],&mycomm);
	// MPI_Comm_rank(mycomm, &gv->rank[1]);
	// MPI_Comm_size(mycomm, &gv->size[1]);
	// printf("%d: I’m  rank %d of %d in the %s context\n", gv->rank[0], gv->rank[1], gv->size[1], cname[gv->color]);
	// fflush(stdout);

	MPI_Barrier(MPI_COMM_WORLD);

	//debug_print(gv->rank[0]);

	if (gv->color == 0){
		lvs   = (LV) malloc(sizeof(*lvs)*(gv->writer_num+1+1));
		thrds = (pthread_t*) malloc(sizeof(pthread_t)*(gv->writer_num+1+1));
		attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*(gv->writer_num+1+1));

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
		if(gv->rank[0]==0){
			printf("Compute Process %d cpt_total_blks = %d, writer_blk_num=%d, sender_blk_num=%d\n",
				gv->rank[0], gv->cpt_total_blks, gv->writer_blk_num, gv->sender_blk_num);
			printf("Compute Process %d: PRODUCER_Ringbuffer %.3fGB, size is %d member\n",
	    		gv->rank[0], PRODUCER_RINGBUFFER_TOTAL_MEMORY/(1024.0*1024.0*1024.0),gv->producer_rb_p->bufsize);
			fflush(stdout);
		}

		gv->written_id_array = (int *) malloc(sizeof(int)*gv->writer_blk_num);
		gv->send_tail = 0;
		gv->disk_id = 0;
		gv->writer_done = 0;
	    gv->id_get = 0;
	    gv->writer_quit = 0;

		//init lock
		pthread_mutex_init(&gv->lock_block_id,NULL);
	    pthread_mutex_init(&gv->lock_writer_progress, NULL);
	    pthread_mutex_init(&gv->lock_writer_done, NULL);
	    pthread_mutex_init(&gv->lock_id_get, NULL);
	    pthread_mutex_init(&gv->lock_writer_quit, NULL);

		t0=get_cur_time();

		/* Create threads */
		for(i = 0; i < (gv->writer_num+1+1); i++) {
		  init_lv(lvs+i, i, gv);
		  if(pthread_attr_init(attrs+i)) perror("attr_init()");
		  if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
		  if(pthread_create(thrds+i, attrs+i, compute_node_do_thread, lvs+i)) {
		    perror("pthread_create()");
		    exit(1);
		  }
		}

		/* Join threads */
		for(i = 0; i < (gv->writer_num+1+1); i++) {
		  pthread_join(thrds[i], &retval);
		  // printf("Compute Node %d Thread %d is finished\n", gv->rank[0] ,i);
		}

		t1=get_cur_time();
		free(lvs);
		free(attrs);
		free(thrds);

		printf("Compute Process %d compute job finish on %s, total_time=%f!\n", gv->rank[0], gv->processor_name,t1-t0);
		fflush(stdout);
	}
	else if (gv->color == 1){
		//ana node
		gv->ana_total_blks = gv->computer_group_size * gv->cpt_total_blks;
		gv->reader_blk_num = gv->computer_group_size * gv->writer_blk_num;

		lvs   = (LV) malloc(sizeof(*lvs)*(gv->reader_num+1+1));
		thrds = (pthread_t*) malloc(sizeof(pthread_t)*(gv->reader_num+1+1));
		attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*(gv->reader_num+1+1));

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
	    pthread_mutex_init(consumer_rb.lock_ringbuffer, NULL);
	    pthread_cond_init(consumer_rb.full, NULL);
	    pthread_cond_init(consumer_rb.empty, NULL);
	    gv->consumer_rb_p = &consumer_rb;
	    //gv->consumer_rb_p = rb_init(gv,CONSUMER_RINGBUFFER_TOTAL_MEMORY,&consumer_rb);
	    if(gv->rank[0]==gv->num_compute_nodes || gv->rank[0]==(gv->num_analysis_nodes+gv->num_compute_nodes-1)){
	    	printf("Ana Process %d ana_total_blks = %d\n", gv->rank[0], gv->ana_total_blks);
		    printf("Ana Process %d: CONSUMER_Ringbuffer %.3fGB, size is %d member\n",
		    	gv->rank[0], CONSUMER_RINGBUFFER_TOTAL_MEMORY/(1024.0*1024.0*1024.0),gv->consumer_rb_p->bufsize);
		    fflush(stdout);
	    }

	    gv->ana_progress = 0;
		gv->mpi_recv_progress_counter = 0;
	    gv->org_recv_buffer = (char *) malloc(gv->compute_data_len); //long message+
	    check_malloc(gv->org_recv_buffer);

	    // prfetch threads 1+1:cid+blkid
	    gv->prefetch_id_array = (int *) malloc(sizeof(int)*(1+1)*gv->ana_total_blks);
	    gv->recv_tail = 0;
	    check_malloc(gv->prefetch_id_array);

	    //initialize lock
	    pthread_mutex_init(&gv->lock_recv,NULL);
	    // pthread_mutex_init(&gv->lock_prefetcher_progress, NULL);

	    t2=get_cur_time();
		/* Create threads */
		for(i = 0; i < (gv->reader_num+1+1); i++) {
		  init_lv(lvs+i, i, gv);
		  if(pthread_attr_init(attrs+i)) perror("attr_init()");
		  if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
		  if(pthread_create(thrds+i, attrs+i, analysis_node_do_thread, lvs+i)) {
		    perror("pthread_create()");
		    exit(1);
		  }
		}

		/* Join threads */
		for(i = 0; i < (gv->reader_num+1+1); i++) {
		  pthread_join(thrds[i], &retval);
		  // printf("Analysis Node %d Thread %d is finished\n", gv->rank[0], i);
		}

		free(lvs);
		free(attrs);
		free(thrds);

		t3=get_cur_time();
		printf("Analysis Node %d on %s analysis job  total time = %f\n",gv->rank[0], gv->processor_name,t3-t2);
	}
	else{
		printf("Error!\n");
	}

	// MPI_Comm_free(&mycomm);
	MPI_Finalize();

	free(gv);

	return 0;
}

