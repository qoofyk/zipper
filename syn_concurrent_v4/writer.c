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

void write_blk(GV gv, LV lv, int write_position, char* buffer, int nbytes, FILE *fp){
	double t0=0,t1=0;
	int error=-1;
	int i=0;

	while((error==-1) && (i<TRYNUM)){
		error=fseek(fp, write_position*gv->block_size, SEEK_SET);
  		if(error==-1){
  			if(i==TRYNUM-1){
  				perror("Compute Process fseek error:");
  				printf("Warning: Compute Process %d fseek error block_id=%d,*fp=%x\n",
  					gv->rank[0], write_position, fp);
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
  //Make sure the content was written to disk
  fflush(fp);
	t1 = get_cur_time();
	lv->only_fwrite_time += t1 - t0;
}

void cp_read_blk(GV gv, LV lv,int last_gen_rank, int last_file_position, char* buffer, FILE *fp){
  double t0=0, t1=0;
  int error=-1;
  int i=0;

  while((error==-1) && (i<TRYNUM)){
    error=fseek(fp, last_file_position*gv->block_size, SEEK_SET);
      if(error==-1){
        if(i==TRYNUM-1){
          perror("Compute Process Read fseek error:");
          printf("Warning: Compute Process %d fseek error last_file_position=%d,*fp=%x\n",
            gv->rank[0], last_file_position,*fp);
          fflush(stdout);
        }
        i++;
        usleep(20000);
      }
  }


  t0 = get_cur_time();
  fread(buffer, gv->block_size, 1, fp);
  if(ferror(fp)==-1){
    perror("fread error:");
    fflush(stdout);
  }

  t1 = get_cur_time();
  lv->only_fread_time += t1 - t0;
}

void writer_thread(GV gv,LV lv) {

	int dest = gv->rank[0]/gv->computer_group_size + gv->computer_group_size*gv->num_analysis_nodes;
	int block_id=0,my_count=0;
	double t0=0,t1=0,t2=0,t3=0;
	char* buffer=NULL;

	// double wr_rd_time=0.0;
	// char * new_buffer=NULL;
	// int mark, mark2;
	char file_name[128];
	FILE *fp=NULL;
	int i=0;
	// printf("Compute Node %d Writer thread %d is running!\n",gv->rank[0],lv->tid);

	// disk_msg = (char*) malloc (sizeof(int)*gv->writer_blk_num);

	t2 = get_cur_time();

	while(1){
		//Normal Case: writer quit as expected
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


		// fopen a file one time
    if(my_count==0){
      // "/N/dc2/scratch/fuyuan/concurrent/syn/mbexp%03dvs%03d/cid%03d/cid%03d.d"
      sprintf(file_name,ADDRESS,gv->num_compute_nodes, gv->num_analysis_nodes, gv->rank[0], gv->rank[0]);
      // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
      while((fp==NULL) && (i<TRYNUM)){
        fp=fopen(file_name,"wb+");
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

		// mark = *((int *)(buffer+gv->block_size-8));
		// new_buffer = (char*)malloc(gv->block_size);
		// cp_read_blk(gv, lv, gv->rank[0], my_count, new_buffer,fp);
		// mark2 = *((int *)(new_buffer+gv->block_size-8));
		// t1 = get_cur_time();
		// wr_rd_time += t1 - t0;
		// printf("Compute %d Writer %d start to write block_id=%d, MARK=%d, my_count=%d, mark2=%d\n", gv->rank[0], lv->tid, block_id, mark, my_count, mark2);
		// fflush(stdout);

		//add to disk_id_array
		pthread_mutex_lock(&gv->lock_writer_progress);
    gv->written_id_array[gv->send_tail]=my_count;
    gv->send_tail++;
    pthread_mutex_unlock(&gv->lock_writer_progress);

    my_count++;

		free(buffer);
		// free(new_buffer);
		// if(gv->rank[0]==0 && my_count%100==0)
		// if(my_count%100==0)
        	// printf("Compute Process %d Writer %d my_count= %d\n", gv->rank[0], lv->tid, my_count);

	}

	if(my_count>0)
		fclose(fp);

	t3 = get_cur_time();

	// if(my_count!=gv->writer_blk_num)
	// 	printf("Writer my_count error!\n");

  printf("Compute Process%d Writer%d write_time=%f, only_fwrite_time=%f, total time=%f, my_count=%d\n",
    gv->rank[0], lv->tid, lv->write_time, lv->only_fwrite_time, t3-t2, my_count);
	// printf("Compute Process%d Writer%d write_time=%f, only_fwrite_time=%f, wr_rd_time=%f, total time=%f, my_count=%d\n",
	// 	gv->rank[0], lv->tid, lv->write_time, lv->only_fwrite_time, wr_rd_time, t3-t2, my_count);
	// printf("Node%d Producer %d Write_Time/Block= %f only_fwrite_time/Block= %f, SPEED= %fKB/s\n",
	//   gv->rank[0], lv->tid,  lv->write_time/gv->total_blks, lv->only_fwrite_time/gv->total_blks, gv->total_file/(lv->write_time));

  // return NULL;
}
