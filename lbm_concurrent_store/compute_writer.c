#include "concurrent.h"

char* producer_ring_buffer_get(GV gv,LV lv){
  char* pointer;
  ring_buffer *rb = gv->producer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements > 0) {
      pointer = rb->buffer[rb->tail];
      rb->tail = (rb->tail + 1) % rb->bufsize;
      rb->num_avail_elements--;
      pthread_cond_signal(rb->full);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return pointer;
      }
    else {
      pthread_cond_wait(rb->empty, rb->lock_ringbuffer);
    }
  }
}

void write_blk(GV gv, LV lv,int blk_id, char* buffer, int nbytes){
	char file_name[128];
	FILE *fp=NULL;
	double t0=0,t1=0;
	int i=0;
	//"/N/dc2/scratch/fuyuan/LBMconcurrent/LBMcon%03dvs%03d/cid%03d/2lbm_cid%03dblk%d.d"
	sprintf(file_name,ADDRESS,gv->compute_process_num, gv->analysis_process_num, gv->rank[0], gv->rank[0], blk_id);
	// printf("%d %d %d %d %d %d \n %s\n", gv->num_compute_nodes, gv->num_analysis_nodes, gv->rank[0], gv->rank[0], lv->tid, blk_id,file_name);
	// fflush(stdout);

	while((fp==NULL) && (i<TRYNUM)){
	    fp=fopen(file_name,"wb");
	    if(fp==NULL){
	    	if(i==TRYNUM-1){
	    		printf("Warning: Compute Process %d Writer %d write empty file last_gen_rank=%d, blk_id=%d\n",
			        gv->rank[0], lv->tid, gv->rank[0], blk_id);
			    fflush(stdout);
	    	}
	      	i++;
	      	// usleep(500000);
	      	sleep(1);
	    }
	}

	t0 = get_cur_time();
	fwrite(buffer, nbytes, 1, fp);
	t1 = get_cur_time();
	lv->only_fwrite_time += t1 - t0;

	fclose(fp);
}

void compute_writer_thread(GV gv,LV lv) {

	int block_id=0,my_count=0;
	double t0=0,t1=0,t2=0,t3=0;
	char* buffer=NULL;
	ring_buffer *rb = gv->producer_rb_p;
	// int dest = gv->rank[0]/gv->computer_group_size + gv->computer_group_size*gv->analysis_process_num;

	// printf("Compute Process %d Writer thread %d is running!\n",gv->rank[0],lv->tid);

	t2 = get_cur_time();

	while(1){
		if(my_count >= gv->writer_blk_num) {
        	pthread_mutex_lock(&gv->lock_writer_done);
			gv->writer_done=1;
			pthread_mutex_unlock(&gv->lock_writer_done);
			break;
		}

		//Robbing case: Writer send last disk msg
    	pthread_mutex_lock(&gv->lock_writer_done);
		if(gv->writer_done==1){
			pthread_mutex_unlock(&gv->lock_writer_done);

			pthread_mutex_lock(&gv->lock_writer_quit);
			gv->writer_quit = 1;
			pthread_mutex_unlock(&gv->lock_writer_quit);

			pthread_cond_broadcast(rb->empty);

			break;
		}
		pthread_mutex_unlock(&gv->lock_writer_done);

		//get pointer from PRB
		pthread_mutex_lock(&gv->lock_id_get);
    	gv->id_get++;
    	if(gv->id_get > gv->cpt_total_blks){
			pthread_mutex_unlock(&gv->lock_id_get);
			break;
		}
    	pthread_mutex_unlock(&gv->lock_id_get);

    	buffer = producer_ring_buffer_get(gv,lv);

		block_id = *((int*)buffer);
		// printf("Compute %d Writer %d start to write block_id=%d\n", gv->rank[0], lv->tid, block_id);
		// fflush(stdout);
		t0 = get_cur_time();
		write_blk(gv, lv, block_id, buffer+sizeof(int), gv->block_size);
		t1 = get_cur_time();
		lv->write_time += t1 - t0;
		my_count++;

		//add to disk_id_array
		pthread_mutex_lock(&gv->lock_writer_progress);
        gv->written_id_array[gv->send_tail]=block_id;
        gv->send_tail++;
        pthread_mutex_unlock(&gv->lock_writer_progress);

		free(buffer);

	}


	t3 = get_cur_time();

	// if(my_count!=gv->writer_blk_num)
	// 	printf("Writer my_count error!\n");

	printf("Compute Process %d Writer %d write_time= %f , only_fwrite_time=%f, total time= %f, with %d blocks\n",
	gv->rank[0], lv->tid, lv->write_time, lv->only_fwrite_time, t3-t2, my_count);
	// printf("Node%d Producer %d Write_Time/Block= %f only_fwrite_time/Block= %f, SPEED= %fKB/s\n",
	//   gv->rank[0], lv->tid,  lv->write_time/gv->total_blks, lv->only_fwrite_time/gv->total_blks, gv->total_file/(lv->write_time));

  // return NULL;
}
