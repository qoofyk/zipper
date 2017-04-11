#include "concurrent.h"

void* analysis_writer_ring_buffer_read_tail(GV gv,LV lv, int* block_id_p, int* source_p, int* writer_state_p){

  void* pointer;

  ring_buffer *rb = gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);

  while(1) {

  	if(gv->ana_writer_done==1){
		pthread_mutex_unlock(rb->lock_ringbuffer);
		return NULL;
	}

    if (rb->num_avail_elements > 0) {
		pointer = rb->buffer[rb->tail];
		*source_p = ((int*)pointer)[0];
		*block_id_p = ((int*)pointer)[1];
		*writer_state_p = ((int*)pointer)[2];

// #ifdef DEBUG_PRINT
// 		printf("Ana_Proc%d: Writer%d ****GET a TAIL**** rb->num_avail_elements=%d, rb->tail=%d\n", gv->rank[0], lv->tid, rb->num_avail_elements, rb->tail);
// 		fflush(stdout);
// #endif //DEBUG_PRINT

		pthread_mutex_unlock(rb->lock_ringbuffer);
		return pointer;
     }
    else {

#ifdef DEBUG_PRINT
		printf("Ana_Proc%d: Writer%d ****Prepare to Sleep!**** rb->num_avail_elements=%d, rb->tail=%d\n", gv->rank[0], lv->tid, rb->num_avail_elements, rb->tail);
		fflush(stdout);
#endif //DEBUG_PRINT

		pthread_cond_wait(rb->empty, rb->lock_ringbuffer);

#ifdef DEBUG_PRINT
		printf("Ana_Proc%d: Writer%d Wake up! rb->num_avail_elements=%d, rb->tail=%d\n", gv->rank[0], lv->tid, rb->num_avail_elements, rb->tail);
		fflush(stdout);
#endif //DEBUG_PRINT
    }
  }
}


void analysis_write_blk_per_file(GV gv, LV lv, int source, int blk_id, void* buffer, int nbytes){
	char file_name[128];
	FILE *fp=NULL;
	double t0=0,t1=0;
	int i=0;
	//"/N/dc2/scratch/fuyuan/LBMconcurrentstore/LBMcon%03dvs%03d/cid%03d/2lbm_cid%03dblk%d.d"
	sprintf(file_name,ADDRESS,gv->compute_process_num, gv->analysis_process_num, source, source, blk_id);
	// printf("%d %d %d %d %d %d \n %s\n", gv->compute_process_num, gv->analysis_process_num, gv->rank[0], gv->rank[0], lv->tid, blk_id,file_name);
	// fflush(stdout);

	while((fp==NULL) && (i<TRYNUM)){
		fp=fopen(file_name,"w");
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
		printf("!!!!! Missing Writing File: Analysis Process %d Writer %d write Missing a file last_gen_rank=%d, blk_id=%d !!!!!!!!!!!!!!!\n",
				    gv->rank[0], lv->tid, source, blk_id);
		fflush(stdout);
	}

	fclose(fp);
}

void analysis_writer_thread(GV gv, LV lv) {

	int source=0, block_id=0, my_count=0;
	double t0=0, t1=0, t2=0, t3=0;
	void* pointer=NULL;
	int writer_state;

	ring_buffer *rb = gv->consumer_rb_p;
	// int free_count=0;
	// int dest = gv->rank[0]/gv->computer_group_size + gv->computer_group_size*gv->analysis_process_num;

	// printf("Ana_Proc%d: Writer%d is running!\n", gv->rank[0], lv->tid);
	// fflush(stdout);

	t2 = get_cur_time();

	while(1){

// #ifdef NOKEEP
// 		if(gv->analysis_writer_blk_num==0){
// 			break;
// 		}
// #endif //NOKEEP

		//get pointer from PRB

    	pointer = analysis_writer_ring_buffer_read_tail(gv, lv, &source, &block_id, &writer_state);

		if(pointer!=NULL){

#ifdef DEBUG_PRINT
			printf("Ana_Proc%d: Writer%d ***GET A pointer*** write source=%d block_id=%d, my_count=%d\n",
				gv->rank[0], lv->tid, source, block_id, my_count);
			fflush(stdout);
#endif //DEBUG_PRINT


			if (block_id != EXIT_BLK_ID){

				if(writer_state==NOT_ON_DISK){

#ifdef KEEP
					t0 = get_cur_time();
					analysis_write_blk_per_file(gv, lv, source, block_id, pointer+sizeof(int)*4, gv->block_size);
					t1 = get_cur_time();
					lv->write_time += t1 - t0;
#endif //KEEP
					my_count++;


#ifdef DEBUG_PRINT
					printf("Ana_Proc%d: Writer%d ***Prepare-Assign-State and Prepare to Sleep*** write source=%d block_id=%d, my_count=%d\n",
						gv->rank[0], lv->tid, ((int*)pointer)[0], ((int*)pointer)[1], my_count);
					fflush(stdout);
#endif //DEBUG_PRINT

					pthread_mutex_lock(rb->lock_ringbuffer);
					((int*)pointer)[2] = ON_DISK;
					pthread_cond_wait(rb->new_tail, rb->lock_ringbuffer);
					pthread_mutex_unlock(rb->lock_ringbuffer);

					if(my_count%WRITER_COUNT==0)
						printf("Ana_Proc%d: Writer%d my_count %d\n", gv->rank[0], lv->tid, my_count);

#ifdef DEBUG_PRINT
					printf("Ana_Proc%d: Writer%d ***Finish-Write-state and Wake up*** source=%d block_id=%d, my_count=%d\n",
						gv->rank[0], lv->tid, source, block_id, my_count);
					fflush(stdout);
#endif //DEBUG_PRINT

				}
				else{// this block has already been written on disk, then writer continue read tail

// #ifdef DEBUG_PRINT
// 					printf("Ana_Proc%d: Writer%d ***Get a ON_DISK block and Prepare to sleep*** source=%d block_id=%d, my_count=%d\n",
// 						gv->rank[0], lv->tid, source, block_id, my_count);
// 					fflush(stdout);
// #endif //DEBUG_PRINT

// #ifdef DEBUG_PRINT
// 					printf("Ana_Proc%d: Writer%d ***Wake up*** source=%d block_id=%d, my_count=%d\n",
// 						gv->rank[0], lv->tid, source, block_id, my_count);
// 					fflush(stdout);
// #endif //DEBUG_PRINT

				}
			}
			else{// writer get the final block EXIT_BLK_ID, then continue read tail

// #ifdef DEBUG_PRINT
// 					printf("Ana_Proc%d: Writer%d ***Get a EXIT_BLK_ID and Prepare to sleep*** source=%d block_id=%d, my_count=%d\n",
// 						gv->rank[0], lv->tid, source, block_id, my_count);
// 					fflush(stdout);
// #endif //DEBUG_PRINT


// #ifdef DEBUG_PRINT
// 					printf("Ana_Proc%d: Writer%d ***Wake up*** source=%d block_id=%d, my_count=%d\n",
// 						gv->rank[0], lv->tid, source, block_id, my_count);
// 					fflush(stdout);
// #endif //DEBUG_PRINT

			}

		}
		else{//get a NULL pointer means: A_consumer got the final block and require A_writer to exit
			printf("Ana_Proc%d: A_Writer%d get NULL to exit\n", gv->rank[0], lv->tid);
			fflush(stdout);
			break;
		}

		if (gv->ana_writer_done==1) {
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
