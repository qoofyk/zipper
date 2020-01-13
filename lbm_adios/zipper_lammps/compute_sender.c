#include "concurrent.h"

char* sender_ring_buffer_get(GV gv, LV lv){
	char* pointer;
	ring_buffer *rb = gv->producer_rb_p;

	pthread_mutex_lock(rb->lock_ringbuffer);
	while (1) {
		if(gv->flag_writer_get_finalblk==1){
			pthread_mutex_unlock(rb->lock_ringbuffer);
			return NULL;
		}

		if(rb->num_avail_elements > 0){
			pointer = rb->buffer[rb->tail];
			rb->tail = (rb->tail + 1) % rb->bufsize;
			--rb->num_avail_elements;
			pthread_cond_signal(rb->full);
			pthread_mutex_unlock(rb->lock_ringbuffer);
			return pointer;
		}
		else{
			lv->wait++;
			pthread_cond_wait(rb->empty, rb->lock_ringbuffer);
		}
	}
}

void compute_sender_thread(GV gv, LV lv){

	int dest = gv->rank[0]/gv->computer_group_size + gv->computer_group_size*gv->analysis_process_num;
	int my_count=0, send_counter=0, i, prog=0;
	double t0=0, t1=0, t2=0, t3=0;
	char* buffer=NULL;
	int* tmp_int_ptr;
	int num_send_char=0, errorcode, long_msg_id=0, mix_msg_id=0, disk_id=0;
	double mix_send_time=0, pure_mpi_send_time=0;
	int disk_msg_flag=0;
	char exit_flag=0;
	int block_id=0, dump_lines_in_this_blk=0;
	char my_exit_flag=0;

	ring_buffer *rb = gv->producer_rb_p;
	// printf("Comp_Proc%d: Sender%d start running!\n", gv->rank[0], lv->tid);
	// fflush(stdout);

	t2 = MPI_Wtime();
	while (1){

		buffer = sender_ring_buffer_get(gv, lv);

		if(buffer != NULL){

			block_id = ((int*)buffer)[0];
			dump_lines_in_this_blk = ((int*)buffer)[2];

			// tmp_int_ptr = (int*)(buffer + sizeof(int) + sizeof(char)*gv->block_size); -- block_size varied! Cannot use this!
			tmp_int_ptr = (int*)(buffer + sizeof(int)*3 + sizeof(double)*dump_lines_in_this_blk*5);
			my_count++;

			if (block_id != EXIT_BLK_ID){

				// printf("Comp_Proc%d: Sender%d get block_id %d\n", gv->rank[0], lv->tid, block_id);
				// fflush(stdout);

				//check written_id_array
				pthread_mutex_lock(&gv->lock_disk_id_arr);
				if (gv->send_tail>0){
					//printf("Compute %d send a message send_counter=%d\n", gv->rank[0], send_counter);
					// fflush(stdout);
					for (i=0; i<gv->send_tail; i++){
						tmp_int_ptr[i+1] = gv->written_id_array[i];
						// printf("Sender: tmp_int_ptr[i+1]=%d\n", tmp_int_ptr[i+1]);
						// fflush(stdout);
						send_counter++;
					}

					tmp_int_ptr[0] = send_counter; //Lammps_new_meaning: how many total integer are saved, (blkid+dump_lines_in_this_blk)

					gv->send_tail = 0;
				}
				pthread_mutex_unlock(&gv->lock_disk_id_arr);

				// Mix_msg
				if (send_counter > 0){

#ifdef DEBUG_PRINT
					printf("Comp_Proc%d: Sender%d --Prepare to send a MIX_MSG-- send_counter=%d with blkid=%d\n",
						gv->rank[0], lv->tid, send_counter, block_id);
					fflush(stdout);
#endif //DEBUG_PRINT

					t0 = MPI_Wtime();
					//blkid + time_step + dump_lines_in_this_blk || + data + || send_cnt+written_id_array
					num_send_char = sizeof(int)*3 + sizeof(double)*dump_lines_in_this_blk*5 + sizeof(int)*(send_counter+1);
					errorcode = MPI_Send(buffer, num_send_char, MPI_CHAR, dest, MIX_MPI_DISK_TAG, MPI_COMM_WORLD);
					check_MPI_success(gv, errorcode);
					t1 = MPI_Wtime();

#ifdef DEBUG_PRINT
					printf("Comp_Proc%d: Sender%d --SUCCESS send a MIX_MSG-- send_counter=%d with blkid=%d\n",
						gv->rank[0], lv->tid, send_counter, block_id);
					fflush(stdout);
#endif //DEBUG_PRINT
					mix_send_time += t1 - t0;
					mix_msg_id++;
					// lammps_change: send_counter/2 means 1 block data
					disk_id += send_counter/2;

					prog += send_counter/2+1;
					send_counter = 0;
					free(buffer);

				}
				// Long_msg
				else{

					if(block_id<0){
						printf("Fatal Error: Comp_Proc%d: Sender%d prepare to send *LONG_MSG* blkid=%d\n", gv->rank[0], lv->tid, block_id);
						fflush(stdout);
						exit(1);
					}

#ifdef DEBUG_PRINT
					printf("Comp_Proc%d: Sender%d **Prepare send LONG_MSG** blkid=%d\n", gv->rank[0], lv->tid, block_id);
					fflush(stdout);
#endif //DEBUG_PRINT

					t0 = MPI_Wtime();
					num_send_char = sizeof(int)*3 + sizeof(double)*dump_lines_in_this_blk*5;
					errorcode = MPI_Send(buffer, num_send_char, MPI_CHAR, dest, MPI_MSG_TAG, MPI_COMM_WORLD);
					check_MPI_success(gv, errorcode);
					t1 = MPI_Wtime();
					pure_mpi_send_time += t1 - t0;

#ifdef DEBUG_PRINT
					printf("Comp_Proc%d: Sender%d **SUCCESS send LONG_MSG** blkid=%d\n", gv->rank[0], lv->tid, block_id);
					fflush(stdout);
#endif //DEBUG_PRINT

					prog++;
					long_msg_id++;
					free(buffer);

				}
			}
			else{//Get exit flag msg
#ifdef DEBUG_PRINT
				printf("Comp_Proc%d: Sender%d Get exit flag msg and prepare to quit\n",
					gv->rank[0], lv->tid);
				fflush(stdout);
#endif //DEBUG_PRINT

				pthread_mutex_lock(rb->lock_ringbuffer);
				gv->flag_sender_get_finalblk = 1;
				rb->tail = (rb->tail + 1) % rb->bufsize;
      			--rb->num_avail_elements;
				pthread_cond_signal(rb->empty);
				pthread_mutex_unlock(rb->lock_ringbuffer);

				free(buffer);

				my_exit_flag=1;
			}
		}
		else{//writer get the final msg.

#ifdef DEBUG_PRINT
			printf("Comp_Proc%d: Sender%d *Discover* *Writer* Get exit flag msg and prepare to quit\n",
					gv->rank[0], lv->tid);
			fflush(stdout);
#endif //DEBUG_PRINT

			my_exit_flag=2;
		}

		if(my_exit_flag!=0){

			//wait for writer to exit; force EXIT be the last message
			while(gv->writer_exit==0);

			int remain_disk_id=0;

			pthread_mutex_lock(&gv->lock_disk_id_arr);
			if (gv->send_tail>0){
				disk_msg_flag = 1;
				remain_disk_id = gv->send_tail;
				gv->send_tail = 0;
			}
			pthread_mutex_unlock(&gv->lock_disk_id_arr);

			if (disk_msg_flag == 0){

#ifdef DEBUG_PRINT
				printf("Comp_Proc%d: ---Normal case--- Sender%d longer than writer !!!!---###---\n",
					gv->rank[0], lv->tid);
				fflush(stdout);
#endif //DEBUG_PRINT

				// send EXIT msg
				errorcode = MPI_Send(&exit_flag, 1, MPI_CHAR, dest, EXIT_MSG_TAG, MPI_COMM_WORLD);
				check_MPI_success(gv, errorcode);
			}

			// Special case: sender finish its job early and wait for writer to finish
			if (disk_msg_flag == 1){

#ifdef DEBUG_PRINT
				printf("Comp_Proc%d: ---Special case--- Sender%d wait for Writer and send the last msg with %d blocks!!!!---###---\n",
					gv->rank[0], lv->tid, gv->send_tail);
				fflush(stdout);
#endif //DEBUG_PRINT

				num_send_char = remain_disk_id*sizeof(int);
				errorcode = MPI_Send(gv->written_id_array, num_send_char, MPI_CHAR, dest, DISK_TAG, MPI_COMM_WORLD);
				check_MPI_success(gv, errorcode);

				disk_id += remain_disk_id/2;
				prog += remain_disk_id/2;
				//send EXIT msg
				errorcode = MPI_Send(&exit_flag, 1, MPI_CHAR, dest, EXIT_MSG_TAG, MPI_COMM_WORLD);
				check_MPI_success(gv, errorcode);
			}

			break;
		}

	}
	t3 = MPI_Wtime();

	printf("Comp_Proc%04d: Sender%d T_total=%.3f, prog=%d, \
T_mix=%.3f, T_long=%.3f, T_total_send=%.3f, M_mix=%d, disk_id=%d, M_long=%d, empty_wait=%d\n",
		   gv->rank[0], lv->tid, t3 - t2, prog,
		   mix_send_time, pure_mpi_send_time, mix_send_time+pure_mpi_send_time, mix_msg_id, disk_id, long_msg_id, lv->wait);
	fflush(stdout);

}
