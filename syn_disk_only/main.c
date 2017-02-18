#include "do_thread.h"

int main(int argc, char **argv){

	MPI_Comm mycomm;


	// char *cname[] = { "BLACK", "WHITE", "BLUE" };

	LV              lvs;
	GV              gv;
	pthread_t       *thrds;
	pthread_attr_t  *attrs;
	void            *retval;

	ring_buffer producer_rb;
	ring_buffer consumer_rb;

	double t0=0,t1=0;

	if(argc != 9) {
		fprintf(stderr, "Usage: %s generator_num, producer#, prefetcher#, step\n", argv[0]);
		exit(1);
	}


	gv    = (GV) malloc(sizeof(*gv));
	//init lbm
	// gv->cubex=atoi(argv[1]);
	// gv->cubey=atoi(argv[1]);
	// gv->cubez=atoi(argv[2]);

	// gv->X=nx/gv->cubex;
	// gv->Y=ny/gv->cubey;
	// gv->Z=nz/gv->cubez;

	gv->generator_num = atoi(argv[1]);
	gv->producer_num = atoi(argv[2]);   //num of producer threads
	gv->prefetcher_num = atoi(argv[3]); //num of prefetcher threads
	//gv->step_stop = atoi(argv[5]);

	gv->computer_group_size=atoi(argv[4]);
	gv->num_analysis_nodes=atoi(argv[5]);
	gv->num_compute_nodes=gv->computer_group_size*gv->num_analysis_nodes;

	gv->generator_counter = 0;
	gv->id_counter = 0;
	gv->progress_counter = 0;
	gv->prefetch_counter = 0;
	gv->calc_counter = 0;

	gv->block_size = atoi(argv[6])*1024*64; //64KB
  	//gv->total_blks = atol(argv[7])*TOTAL_FILE2PRODUCE_1GB/(gv->computer_group_size*gv->num_analysis_nodes*gv->block_size);
	gv->total_blks = atol(argv[7]);
	gv->lp = atoi(argv[8]);

	//sleep(20);

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &gv->rank[0]);
	MPI_Comm_size(MPI_COMM_WORLD, &gv->size[0]);
	MPI_Get_processor_name(gv->processor_name, &gv->namelen);
	// printf("Hello world! I’m  rank %d of %d on %s\n", gv->rank[0], gv->size[0], gv->processor_name);

	//debug_print(gv->rank[0]);

	if(gv->rank[0]<gv->computer_group_size*gv->num_analysis_nodes)
	  //Compute Group
	  gv->color = 0;
	else
	  //Analysis Group
	  gv->color = 1;

	MPI_Comm_split(MPI_COMM_WORLD, gv->color, gv->rank[0],&mycomm);

	MPI_Comm_rank(mycomm, &gv->rank[1]);

	MPI_Comm_size(mycomm, &gv->size[1]);

	// printf("%d: I’m  rank %d of %d in the %s context\n", gv->rank[0], gv->rank[1], gv->size[1], cname[gv->color]);

	MPI_Barrier(MPI_COMM_WORLD);

	t0=get_cur_time();

	if (gv->color == 0){ /* Group 0 communicates with group 1. */
		//MPI_Status status;
		int i;

		//double t2=0, t3=0;
		//int errorcode;


		lvs   = (LV) malloc(sizeof(*lvs)*(gv->generator_num + gv->producer_num+1));
		thrds = (pthread_t*) malloc(sizeof(pthread_t)*(gv->generator_num + gv->producer_num+1));
		attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*(gv->generator_num + gv->producer_num+1));

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
	    //printf("Node 1 Total files to produce is %ldB\n", atol(argv[5])*TOTAL_FILE2PRODUCE_1GB);
	    if(gv->rank[0]==0){
	    	printf("Node %d total_blks = %ld\n", gv->rank[0], gv->total_blks);
	    	printf("Node %d: PRODUCER_Ringbuffer %ldGB, size is %d member\n",
	    	gv->rank[0], PRODUCER_RINGBUFFER_TOTAL_MEMORY/(1024*1024*1024),gv->producer_rb_p->bufsize);
	    }


	    gv->written_id_array = (int *) malloc(sizeof(int)*gv->total_blks);
	    gv->send_tail = 0;
	    // for(j=0;j<gv->total_blks;j++)
	    //   gv->block_id_array[j] = BLANK;

	    //mpi send
	    gv->mpi_send_progress_counter = 0;
	    gv->mpi_send_block_id_array = (int *) malloc(sizeof(int)*(gv->total_blks+1));
	    // for(j=0;j<gv->total_blks;j++)
	    //   gv->mpi_send_block_id_array[j] = BLANK;

	    //initialize lock
	    pthread_mutex_init(&gv->lock_generator,NULL);
	    pthread_mutex_init(&gv->lock_blk_id, NULL);
	    pthread_mutex_init(&gv->lock_producer_progress, NULL);

	    /* Create threads */

	    for(i = 0; i < (gv->generator_num+gv->producer_num+1); i++) {
	      init_lv(lvs+i, i, gv);
	      if(pthread_attr_init(attrs+i)) perror("attr_init()");
	      if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
	      if(pthread_create(thrds+i, attrs+i, node1_do_thread, lvs+i)) {
	        perror("pthread_create()");
	        exit(1);
	      }
	    }

	    //---------------------------------------------BEGIN Synthetic Application -----------------------------------------------

	    //t2 = get_cur_time();

	    /* Join threads */
	    for(i = 0; i < (gv->generator_num+gv->producer_num+1); i++) {
	      pthread_join(thrds[i], &retval);
	      // printf("Node %d Thread %d is finished\n", gv->rank[0],i);
	    }

	    free(gv->written_id_array);
	    free(producer_rb.buffer);
	    free(gv->mpi_send_block_id_array);

	    // printf("Node %d done!\n", gv->rank[0]);
	    //printf("Eventually, Node %d generator_counter = %d\n", gv->rank[0], gv->generator_counter);
	    printf("Eventually, Node %d id_counter = %ld, BUT %ld, progress_counter = %ld\n",
	    	gv->rank[0], gv->id_counter, gv->id_counter-gv->producer_num,gv->progress_counter);

	    free(lvs);
		free(attrs);
		free(thrds);

	   }
	else if (gv->color == 1){
	   	//Analysis node
	   	gv->total_blks=gv->computer_group_size*gv->total_blks;

	   	lvs   = (LV) malloc(sizeof(*lvs)*(gv->prefetcher_num+2));
		thrds = (pthread_t*) malloc(sizeof(pthread_t)*(gv->prefetcher_num+2));
		attrs = (pthread_attr_t*) malloc(sizeof(pthread_attr_t)*(gv->prefetcher_num+2));

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
	    if(gv->rank[0]==gv->num_compute_nodes || gv->rank[0]==(gv->num_compute_nodes+gv->num_analysis_nodes-1)){
	    	printf("Node %d total_blks = %ld\n", gv->rank[0], gv->total_blks);
	    	printf("Node %d: CONSUMER_Ringbuffer %ldGB, size is %d member\n",
	    	gv->rank[0], CONSUMER_RINGBUFFER_TOTAL_MEMORY/(1024*1024*1024),gv->consumer_rb_p->bufsize);
	    }


	    //mpi receive
	    gv->mpi_recv_progress_counter = 0;
	    gv->mpi_recv_block_id_array = (int *) malloc(sizeof(int)*(gv->total_blks/gv->computer_group_size+1));


	    gv->recv_progress_counter = 0;
	    gv->recv_block_id_array = (int *) malloc(sizeof(int)*2*gv->total_blks);
	    gv->recv_tail = 0;

	    //initialize lock
	    pthread_mutex_init(&gv->lock_recv,NULL);
	    pthread_mutex_init(&gv->lock_prefetcher_progress, NULL);

	    /* Create threads */
	    int i=0;
	    for(i = 0; i < (gv->prefetcher_num+2); i++) {
	      init_lv(lvs+i, i, gv);
	      if(pthread_attr_init(attrs+i)) perror("attr_init()");
	      if(pthread_attr_setscope(attrs+i, PTHREAD_SCOPE_SYSTEM)) perror("attr_setscope()");
	      if(pthread_create(thrds+i, attrs+i, node2_do_thread, lvs+i)) {
	        perror("pthread_create()");
	        exit(1);
	      }
	    }

	    /* Join threads */
	    for(i = 0; i < (gv->prefetcher_num+2); i++) {
	      pthread_join(thrds[i], &retval);
	      // printf("Node %d Thread %d is finished\n", gv->rank[0],i);
	    }

	    free(gv->mpi_recv_block_id_array);
	    free(gv->recv_block_id_array);
	    free(consumer_rb.buffer);

	    // printf("Node %d done!\n",gv->rank[0]);
	    // printf("Eventually, Node %d prefetch_counter \n", gv->rank[0], gv->prefetch_counter);
	    printf("Eventually, Node %d prefetch_counter= %ld, calc_counter= %ld\n",
	    	gv->rank[0], gv->prefetch_counter, gv->calc_counter);

	    // if(rank[0]==4){
	    //     int recv_counter;
	    //     MPI_Recv(&recv_counter, 1,MPI_INT,0,456,MPI_COMM_WORLD, &status);
	    //     printf("My rank is %d, my new rank %d, I am an analysis node. recv_counter=%d \n", rank[0], rank[1],recv_counter);
	    // }
	    free(lvs);
		free(attrs);
		free(thrds);
	}
	else{
		printf("Error!\n");
	}

	  MPI_Comm_free(&mycomm);

	  MPI_Finalize();

	  t1=get_cur_time();
	  printf("Node %d on %s Finally total time = %f\n", gv->rank[0], gv->processor_name,t1-t0);

	  free(gv);

	  return 0;
}
