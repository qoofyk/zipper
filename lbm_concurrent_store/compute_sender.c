#include "concurrent.h"

char* sender_ring_buffer_get(GV gv,LV lv){
  char* pointer;
  ring_buffer *rb = gv->producer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
  	if(gv->flag_writer_get_finalblk==1){
			pthread_mutex_unlock(rb->lock_ringbuffer);
			return NULL;
	}

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

void compute_sender_thread(GV gv,LV lv){

	int dest = gv->rank[0]/gv->computer_group_size + gv->compute_process_num;
	int my_count=0, send_counter=0, i;
	double t0=0, t1=0, t2=0,t3=0;
	char* buffer=NULL;
	int* tmp_int_ptr;
	int mix_cnt=0, errorcode, long_msg_id=0, mix_msg_id=0, disk_id=0;
	double mix_send_time=0, pure_mpi_send_time=0;
	int disk_msg_flag=0;
	char exit_flag = 0;
	int block_id;
	char my_exit_flag=0;
	// printf("Compute %d Sender %d start running!\n", gv->rank[0], lv->tid);
	// fflush(stdout);

	ring_buffer *rb = gv->producer_rb_p;

	t2 = get_cur_time();
	while(1){

		buffer = sender_ring_buffer_get(gv, lv);

		if(buffer != NULL){
			tmp_int_ptr = (int*)(buffer + sizeof(int) + sizeof(char)*gv->block_size);
			my_count++;

			block_id = ((int*)buffer)[0];

			if (block_id != EXIT_BLK_ID){
				// printf("Comp_Proc%d: Sender%d get block_id %d\n", gv->rank[0], lv->tid, block_id);
				// fflush(stdout);
				//check disk_id_array
				pthread_mutex_lock(&gv->lock_writer_progress);
				if (gv->send_tail>0){
					//printf("Compute %d send a message send_counter=%d\n", gv->rank[0], send_counter);
					// fflush(stdout);
					for (i = 0; i<gv->send_tail; i++){
						tmp_int_ptr[i+1] = gv->written_id_array[i];
						// printf("Sender: tmp_int_ptr[i+1]=%d\n", tmp_int_ptr[i+1]);
						// fflush(stdout);
						send_counter++;
					}
					tmp_int_ptr[0] = send_counter;
					gv->send_tail = 0;
				}
				pthread_mutex_unlock(&gv->lock_writer_progress);

				// Mix_msg
				if (send_counter > 0){
					// printf("Compute %d Sender send a MIX message send_counter=%d\n", gv->rank[0], send_counter);
					// fflush(stdout);

					t0 = get_cur_time();
					mix_cnt = sizeof(int) + gv->block_size + sizeof(int)*(send_counter + 1);
					errorcode = MPI_Send(buffer, mix_cnt, MPI_CHAR, dest, MIX_MPI_DISK_TAG, MPI_COMM_WORLD);
					check_MPI_success(gv, errorcode);
					t1 = get_cur_time();
					mix_send_time += t1 - t0;
					mix_msg_id++;
					disk_id += send_counter;

					gv->mpi_send_progress_counter += send_counter + 1;
					send_counter = 0;
					free(buffer);

				}
				// Long_msg
				else{

					if(block_id<0){
						printf("Comp_Proc%d: Sender%d prepare to send *LONG_MSG* blkid=%d\n", gv->rank[0], lv->tid, block_id);
						fflush(stdout);
					}

					t0 = get_cur_time();
					errorcode = MPI_Send(buffer, gv->block_size+sizeof(int), MPI_CHAR, dest, MPI_MSG_TAG, MPI_COMM_WORLD);
					check_MPI_success(gv, errorcode);
					t1 = get_cur_time();
					pure_mpi_send_time += t1 - t0;


					gv->mpi_send_progress_counter++;
					long_msg_id++;
					free(buffer);

				}
			}
			else{

				printf("Comp_Proc%d: Sender%d Get exit flag msg and prepare to quit\n",
					gv->rank[0], lv->tid);
				fflush(stdout);

				pthread_mutex_lock(rb->lock_ringbuffer);
				gv->flag_sender_get_finalblk = 1;
				rb->tail = (rb->tail + 1) % rb->bufsize;
      			rb->num_avail_elements--;
				pthread_cond_signal(rb->empty);
				pthread_mutex_unlock(rb->lock_ringbuffer);

				free(buffer);

				my_exit_flag=1;
			}
		}
		else{//writer get the final msg.
			printf("Comp_Proc%d: Sender%d *Discover* *Writer* Get exit flag msg and prepare to quit\n",
					gv->rank[0], lv->tid);
			fflush(stdout);
			my_exit_flag=1;
		}

		if(my_exit_flag==1){
			pthread_mutex_lock(&gv->lock_writer_progress);
			if (gv->send_tail>0){
				disk_msg_flag = 1;
			}
			pthread_mutex_unlock(&gv->lock_writer_progress);

			if (disk_msg_flag == 0){
				printf("Comp_Proc%d: ---Normal case--- Sender%d longer than writer !!!!---###---\n",
					gv->rank[0], lv->tid);
				fflush(stdout);

				//send EXIT msg
				errorcode = MPI_Send(&exit_flag, sizeof(char), MPI_CHAR, dest, EXIT_MSG_TAG, MPI_COMM_WORLD);
				check_MPI_success(gv, errorcode);
			}

			// Special case: sender finish its job early and wait for writer to finish
			if (disk_msg_flag == 1){
				printf("Comp_Proc%d: ---Special case--- Sender%d wait for Writer and send the last msg with %d blocks!!!!---###---\n",
					gv->rank[0], lv->tid, gv->send_tail);
				fflush(stdout);
				errorcode = MPI_Send(gv->written_id_array, gv->send_tail*sizeof(int), MPI_CHAR, dest, DISK_TAG, MPI_COMM_WORLD);
				check_MPI_success(gv, errorcode);

				//send EXIT msg
				errorcode = MPI_Send(&exit_flag, sizeof(char), MPI_CHAR, dest, EXIT_MSG_TAG, MPI_COMM_WORLD);
				check_MPI_success(gv, errorcode);
			}

			break;
		}
	}
	t3 = get_cur_time();

	printf("Comp_Proc%3d: Sender%d T_total=%.3f, mpi_send_progress_counter=%d,\
T_mix_send=%.3f, T_pure_mpi_send=%.3f, T_total_send=%.3f, mix_msg_id=%d, disk_id=%d, long_msg_id=%d\n",
    gv->rank[0], lv->tid, t3-t2, gv->mpi_send_progress_counter,
    mix_send_time, pure_mpi_send_time, mix_send_time+pure_mpi_send_time, mix_msg_id, disk_id, long_msg_id);
  fflush(stdout);


}
