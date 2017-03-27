#include "concurrent.h"

char* analysis_writer_ring_buffer_read_tail(GV gv,LV lv, char* writer_state_p){
  char* pointer;
  // int* temp_int_pointer;
  ring_buffer *rb = gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements > 0) {
		pointer = rb->buffer[rb->tail];
		// temp_int_pointer = (int*) pointer;
		*writer_state_p = pointer[8];
		pthread_mutex_unlock(rb->lock_ringbuffer);
		return pointer;
     }
    else {
		pthread_cond_wait(rb->empty, rb->lock_ringbuffer);
    }
  }
}

void analysis_writer_ring_buffer_move_tail(GV gv,LV lv,int* flag_p, char* pointer){

  	ring_buffer *rb = gv->consumer_rb_p;

	if(pointer!=NULL){
  		while(1){
  			pthread_mutex_lock(rb->lock_ringbuffer);
			if(pointer[9] == CALC_DONE){
				rb->tail = (rb->tail + 1) % rb->bufsize;
				rb->num_avail_elements--;

				pthread_cond_signal(rb->full);
				pthread_mutex_unlock(rb->lock_ringbuffer);
				// the one who last know the state == both_done will free the pointer, in case of the last error case
			  	*flag_p=1;
				return;
			}
			pthread_mutex_unlock(rb->lock_ringbuffer);
		}
  	}
}

void analysis_write_blk(GV gv, LV lv, int source, int blk_id, char* buffer, int nbytes){
	char file_name[128];
	FILE *fp=NULL;
	double t0=0,t1=0;
	int i=0;
	//"/N/dc2/scratch/fuyuan/LBMconcurrentstore/LBMcon%03dvs%03d/cid%03d/2lbm_cid%03dblk%d.d"
	sprintf(file_name,ADDRESS,gv->compute_process_num, gv->analysis_process_num, source, source, blk_id);
	// printf("%d %d %d %d %d %d \n %s\n", gv->compute_process_num, gv->analysis_process_num, gv->rank[0], gv->rank[0], lv->tid, blk_id,file_name);
	// fflush(stdout);

	while((fp==NULL) && (i<TRYNUM)){
		fp=fopen(file_name,"wb");
		if(fp==NULL){
			if(i==TRYNUM-1){
				printf("Warning: Analysis Process %d Writer %d write empty file last_gen_rank=%d, blk_id=%d\n",
				    gv->rank[0], lv->tid, source, blk_id);
			  	fflush(stdout);
			}

		  i++;
		  sleep(1);
		}
	}

	if(fp!=NULL){
		t0 = get_cur_time();
		fwrite(buffer, nbytes, 1, fp);
		t1 = get_cur_time();
		lv->only_fwrite_time += t1 - t0;
	}
	else{
		printf("!!!!! Missing Writing File: Analysis Process %d Writer %d write Missing a file last_gen_rank=%d, blk_id=%d !!!!!!!!!!!!!!!!!!!!!\n",
				    gv->rank[0], lv->tid, source, blk_id);
		fflush(stdout);
	}

	fclose(fp);
}

void analysis_writer_thread(GV gv,LV lv) {

	int source=0,block_id=0,my_count=0;
	double t0=0,t1=0,t2=0,t3=0;
	char* pointer=NULL;
	int* temp_int_pointer;
	// int flag=0;
	char writer_state;
	int num_exit_flag = 0;

	ring_buffer *rb = gv->consumer_rb_p;
	// int free_count=0;
	// int dest = gv->rank[0]/gv->computer_group_size + gv->computer_group_size*gv->analysis_process_num;

	// printf("Analysis Process %d Writer thread %d is running!\n",gv->rank[0],lv->tid);
	// fflush(stdout);

	t2 = get_cur_time();

	while(1){
		if(gv->analysis_writer_blk_num==0){
			break;
		}

		//get pointer from PRB
		// flag=0;
    	pointer = analysis_writer_ring_buffer_read_tail(gv,lv,&writer_state);

		if(pointer!=NULL){

			if(writer_state==NOT_ON_DISK){
				temp_int_pointer=(int*)pointer;
				source = temp_int_pointer[0];
				block_id = temp_int_pointer[1];

#ifdef KEEP
				t0 = get_cur_time();
				analysis_write_blk(gv, lv, source, block_id, pointer+sizeof(int)*3, gv->block_size);
				t1 = get_cur_time();
				lv->write_time += t1 - t0;
				my_count++;
#endif //KEEP

				pthread_mutex_lock(rb->lock_ringbuffer);
				pointer[8] = ON_DISK;
				pthread_mutex_unlock(rb->lock_ringbuffer);

				if(my_count%WRITER_COUNT==0)
					printf("Ana_Proc%d: Writer%d my_count %d\n", gv->rank[0], lv->tid, my_count);

#ifdef DEBUG_PRINT
				printf("Ana_Proc%d: --NOT_ON_DISK-- Writer%d finish write source=%d block_id=%d, my_count=%d\n",
					gv->rank[0], lv->tid, source, block_id,my_count);
				fflush(stdout);
#endif //DEBUG_PRINT
			}
		}

		else{
			num_exit_flag++;
		}


		if (num_exit_flag >= gv->computer_group_size) {
			break;
		}
	}

	t3 = get_cur_time();

	// if(my_count!=gv->analysis_writer_blk_num)
	// 	printf("Analysis Writer my_count error!\n");

	printf("Ana_Proc%d: Writer%d T_write=%.3f, T_only_fwrite=%.3f, T_total=%.3f, my_count=%d\n",
		gv->rank[0], lv->tid, lv->write_time, lv->only_fwrite_time, t3 - t2, my_count);
	fflush(stdout);
	// printf("Node%d Producer %d Write_Time/Block= %f only_fwrite_time/Block= %f, SPEED= %fKB/s\n",
	//   gv->rank[0], lv->tid,  lv->write_time/gv->total_blks, lv->only_fwrite_time/gv->total_blks, gv->total_file/(lv->write_time));

}
