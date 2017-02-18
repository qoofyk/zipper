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

void write_blk(GV gv, LV lv, int blk_id, char* buffer, int nbytes, FILE *fp){
	double t0=0,t1=0;
	int error=-1;
	int i=0;

	while((error==-1) && (i<TRYNUM)){
		error=fseek(fp, blk_id*gv->block_size, SEEK_SET);
  		if(error==-1){
  			if(i==TRYNUM-1){
  				perror("Compute Process fseek error:");
  				printf("Warning: Compute Process %d fseek error block_id=%d,*fp=%x\n",
  					gv->rank[0], blk_id, fp);
  				fflush(stdout);
  			}
  			i++;
            usleep(10000);
  		}
	}


	t0 = get_cur_time();
	error=fwrite(buffer, nbytes, 1, fp);
	if(error==0){
		perror("Write error:");
		fflush(stdout);
	}

	t1 = get_cur_time();
	lv->only_fwrite_time += t1 - t0;
}

void writer_thread(GV gv,LV lv) {

	int block_id=0,my_count=0;
	double t0=0,t1=0,t2=0,t3=0;
	char* buffer=NULL;
	char file_name[128];
	FILE *fp=NULL;
	int i=0;
	// int dest = gv->rank[0]/gv->computer_group_size + gv->computer_group_size*gv->num_analysis_nodes;

	// printf("Compute Node %d Writer thread %d is running!\n",gv->rank[0],lv->tid);

	t2 = get_cur_time();

	while(1){
		if(my_count >= gv->writer_blk_num) {
        	pthread_mutex_lock(&gv->lock_writer_done);
			gv->writer_done=1;
			pthread_mutex_unlock(&gv->lock_writer_done);
			break;
		}

		//get pointer from PRB
    	buffer = producer_ring_buffer_get(gv,lv);

		block_id = *((int*)buffer);
		// printf("Compute %d Writer %d start to write block_id=%d\n", gv->rank[0], lv->tid, block_id);
		// fflush(stdout);

		// fopen a file one time
        if(my_count==0){
          // "/N/dc2/scratch/fuyuan/LBM/LBM%03dvs%03d/cid%03d/2lbm_cid%03dblk%d.d"
          sprintf(file_name,ADDRESS,gv->num_compute_nodes, gv->num_analysis_nodes, gv->rank[0], gv->rank[0]);
          // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
          while((fp==NULL) && (i<TRYNUM)){
            fp=fopen(file_name,"w+");
            if(fp==NULL){
              if(i==TRYNUM-1){
                perror("Error: ");
                printf("Warning: Compute Process %d Writer %d write an empty file last_gen_rank=%d, block_id=%d\n",
                    gv->rank[0], lv->tid, gv->rank[0], block_id);
                fflush(stdout);
              }

              i++;
              usleep(5000);
            }
          }
        }

		t0 = get_cur_time();
		write_blk(gv, lv, my_count, buffer, gv->block_size, fp);
		t1 = get_cur_time();
		lv->write_time += t1 - t0;
		my_count++;

		//add to disk_id_array
		pthread_mutex_lock(&gv->lock_writer_progress);
        gv->written_id_array[gv->send_tail]=my_count;
        gv->send_tail++;
        pthread_mutex_unlock(&gv->lock_writer_progress);

		free(buffer);

		if(my_count >= gv->writer_blk_num) {
        	pthread_mutex_lock(&gv->lock_writer_done);
			gv->writer_done=1;
			pthread_mutex_unlock(&gv->lock_writer_done);
			break;
		}
	}

	if(my_count>0)
		fclose(fp);
	t3 = get_cur_time();

	// if(my_count!=gv->writer_blk_num)
	// 	printf("Writer my_count error!\n");

	printf("Compute Process %d Writer %d write_time= %f , only_fwrite_time=%f, total time= %f, my_count=%d\n",
	gv->rank[0], lv->tid, lv->write_time, lv->only_fwrite_time, t3-t2, my_count);
	// printf("Node%d Producer %d Write_Time/Block= %f only_fwrite_time/Block= %f, SPEED= %fKB/s\n",
	//   gv->rank[0], lv->tid,  lv->write_time/gv->total_blks, lv->only_fwrite_time/gv->total_blks, gv->total_file/(lv->write_time));

  // return NULL;
}
