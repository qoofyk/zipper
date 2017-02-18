#include "concurrent.h"

char* sender_ring_buffer_get(GV gv,LV lv){
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
    } else {
      pthread_cond_wait(rb->empty, rb->lock_ringbuffer);
    }
  }
}

void compute_sender_thread(GV gv,LV lv){

	int dest = gv->rank[0]/gv->computer_group_size + gv->computer_group_size*gv->analysis_process_num;
	int my_count=0,send_counter=0,i;
	double t0=0,t1=0,t2=0,t3=0;
	char* buffer=NULL;
	int* temp_int_pointer;
	int mix_cnt=0,errorcode,long_msg_id=0,mix_msg_id=0;
	double mix_send_time=0, only_mpi_send_time=0;
	int writer_done_flag=0,disk_msg_flag=0;
	int current_id=0;
	char* disk_msg;

	// printf("Compute %d Sender %d start running!\n", gv->rank[0], lv->tid);
	// fflush(stdout);

	t2 = get_cur_time();
	while(1){

		//get pointer from PRB
		pthread_mutex_lock(&gv->lock_id_get);
    	gv->id_get++;
    	if(gv->id_get > gv->cpt_total_blks){
			pthread_mutex_unlock(&gv->lock_id_get);
			break;
		}
		pthread_mutex_unlock(&gv->lock_id_get);

		buffer = sender_ring_buffer_get(gv,lv);
		temp_int_pointer = (int*)(buffer+sizeof(int)+sizeof(char)*gv->block_size);
		my_count++;
		send_counter=0;

		// if(gv->rank[0]==0 && my_count%SENDER_COUNT==0){
		// 	printf("Compute %d Sender %d GET %d\n", gv->rank[0], lv->tid, my_count);
		// 	fflush(stdout);
		// }


		//check disk_id_array
		pthread_mutex_lock(&gv->lock_writer_progress);
	    if(gv->send_tail>0){
	  //   	printf("Compute %d send a message send_counter=%d\n", gv->rank[0], send_counter);
			// fflush(stdout);
			for(i=0;i<gv->send_tail;i++){
				temp_int_pointer[i+1]=gv->written_id_array[i];
				// printf("Sender: temp_int_pointer[i+1]=%d\n", temp_int_pointer[i+1]);
				// fflush(stdout);
				send_counter++;
			}
			temp_int_pointer[0]=send_counter;
			gv->send_tail = 0;
	    }
	    pthread_mutex_unlock(&gv->lock_writer_progress);

	    // Mix_msg
	    if(send_counter > 0){
			// printf("Compute %d Sender send a MIX message send_counter=%d\n", gv->rank[0], send_counter);
			// fflush(stdout);

			t0 = get_cur_time();
			mix_cnt = sizeof(int)+sizeof(char)*gv->block_size+sizeof(int)*(send_counter+1);
			errorcode = MPI_Send(buffer,mix_cnt,MPI_CHAR,dest,MIX_MPI_DISK_TAG,MPI_COMM_WORLD);
			check_MPI_success(gv, errorcode);
			t1 = get_cur_time();
			mix_send_time += t1-t0;
			mix_msg_id++;
			gv->disk_id += send_counter;

			gv->mpi_send_progress_counter += send_counter+1;
			// send_counter = 0;
			free(buffer);
			// if(gv->mpi_send_progress_counter>=gv->cpt_total_blks)
			// 	break;
	    }
	    // Long_msg
	    else{
		    t0 = get_cur_time();
		    errorcode = MPI_Send(buffer,gv->block_size+sizeof(int),MPI_CHAR,dest,MPI_MSG_TAG,MPI_COMM_WORLD);
		    check_MPI_success(gv, errorcode);
		    t1 = get_cur_time();
		    only_mpi_send_time += t1-t0;

		 //    printf("Compute %d Sender %d finish sending %d\n", gv->rank[0], lv->tid, block_id);
			// fflush(stdout);

		    gv->mpi_send_progress_counter++;
		    long_msg_id++;
		    free(buffer);
	      	// if(gv->mpi_send_progress_counter>=gv->cpt_total_blks)
	      	// 	break;
	    }

	    if(my_count >= gv->sender_blk_num){

			pthread_mutex_lock(&gv->lock_writer_done);
			writer_done_flag=gv->writer_done;
			if(writer_done_flag==0)
				gv->writer_done=1;
			pthread_mutex_unlock(&gv->lock_writer_done);

			// Normal case: writer set writer_done_flag=1
			if(writer_done_flag==1){
				send_counter=0;
				pthread_mutex_lock(&gv->lock_writer_progress);
				if(gv->send_tail>0){
		  			disk_msg_flag=1;
		  			disk_msg = (char*) malloc (sizeof(int)*gv->send_tail);
		  			temp_int_pointer = (int*) disk_msg;
		  			for(i=0;i<gv->send_tail;i++){
						temp_int_pointer[i]=gv->written_id_array[i];
						send_counter++;
					}
					gv->send_tail = 0;
		    	}
		    	pthread_mutex_unlock(&gv->lock_writer_progress);

		    	if(disk_msg_flag==0){
		    		printf("---###---Compute Process %d Normal case: Sender && writer Quit ---###---\n",
		    			gv->rank[0]);
					fflush(stdout);
		    	}

				// Normal case: sender finish its job early and wait for writer to finish
				if(disk_msg_flag==1){
					printf("---###---Compute Process %d Normal case: Sender send Last Disk Msg with %d blocks, block_id=%d!!!!---###---\n",
						gv->rank[0], send_counter, temp_int_pointer[0]);
					fflush(stdout);
					errorcode = MPI_Send(disk_msg,send_counter*sizeof(int),MPI_CHAR,dest,DISK_TAG,MPI_COMM_WORLD);
		    		check_MPI_success(gv, errorcode);
		    		gv->mpi_send_progress_counter = gv->mpi_send_progress_counter + send_counter;
		    		gv->disk_id = gv->disk_id + send_counter;
		    		free(disk_msg);
				}

			}

			// Robbing case: sender set writer_done_flag=1
			if(writer_done_flag==0){
				printf("---###---Compute Process %d Sender%d Start Robbing---###---\n",
		    			gv->rank[0], lv->tid);
				fflush(stdout);
				while(1){
					//get pointer from PRB
					pthread_mutex_lock(&gv->lock_id_get);
			    	gv->id_get++;
			    	current_id = gv->id_get;
			    	if(gv->id_get > gv->cpt_total_blks){
						pthread_mutex_unlock(&gv->lock_id_get);

						printf("---###---Compute Process %d Sender get current_id=%d And Prepare to Quit---###---\n",
		    				gv->rank[0],current_id);
						fflush(stdout);

						while(1){
							if(gv->writer_quit==1){
								send_counter=0;
								pthread_mutex_lock(&gv->lock_writer_progress);
								if(gv->send_tail>0){
						  			disk_msg_flag=1;
						  			disk_msg = (char*) malloc (sizeof(int)*gv->send_tail);
						  			temp_int_pointer = (int*) disk_msg;
						  			for(i=0;i<gv->send_tail;i++){
										temp_int_pointer[i]=gv->written_id_array[i];
										send_counter++;
									}
									gv->send_tail = 0;
						    	}
						    	pthread_mutex_unlock(&gv->lock_writer_progress);

						    	if(disk_msg_flag==0){
						    		printf("---###---Compute Process %d Robbing case: Sender && writer Quit ---###---\n",
						    			gv->rank[0]);
									fflush(stdout);
						    	}

								// Robbing case
								if(disk_msg_flag==1){
									printf("---###---Compute Process%d Robbing case: Sender %d send Last Disk Msg with %d blocks, block_id=%d!!!!---###---\n",
										gv->rank[0], lv->tid, send_counter, temp_int_pointer[0]);
									fflush(stdout);
									errorcode = MPI_Send(disk_msg,send_counter*sizeof(int),MPI_CHAR,dest,DISK_TAG,MPI_COMM_WORLD);
						    		check_MPI_success(gv, errorcode);
						    		gv->mpi_send_progress_counter = gv->mpi_send_progress_counter + send_counter;
						    		gv->disk_id = gv->disk_id + send_counter;
						    		free(disk_msg);
								}

								break;
							}
						}

						break;
					}
			    	pthread_mutex_unlock(&gv->lock_id_get);

			  //   	printf("---###---Compute Process %d Sender get current_id=%d---###---\n",
		   //  			gv->rank[0],current_id);
					// fflush(stdout);

					buffer = sender_ring_buffer_get(gv,lv);
					// Note: Different from Synthetic 1 more int!
					temp_int_pointer = (int*)(buffer+sizeof(int)+sizeof(char)*gv->block_size);
					my_count++;
					// if(my_count%100==0){
						// printf("Compute Process %d Sender get current_id=%d, my_count=%d\n",
		    // 			gv->rank[0], current_id, my_count);
						// fflush(stdout);
					// }

					send_counter=0;

					//check disk_id_array
					pthread_mutex_lock(&gv->lock_writer_progress);
				    if(gv->send_tail>0){
				  		// printf("Compute %d send a message send_counter=%d\n", gv->rank[0], send_counter);
						// fflush(stdout);
						for(i=0;i<gv->send_tail;i++){
							temp_int_pointer[i+1]=gv->written_id_array[i];
							// printf("Sender: temp_int_pointer[i+1]=%d\n", temp_int_pointer[i+1]);
							// fflush(stdout);
							send_counter++;
						}
						temp_int_pointer[0]=send_counter;
						gv->send_tail = 0;
				    }
				    pthread_mutex_unlock(&gv->lock_writer_progress);

				    // Mix_msg
				    if(send_counter > 0){
						// printf("Compute %d Sender send a MIX message send_counter=%d\n", gv->rank[0], send_counter);
						// fflush(stdout);

						t0 = get_cur_time();
						mix_cnt = sizeof(int)+sizeof(char)*gv->block_size+sizeof(int)*(send_counter+1);
						errorcode = MPI_Send(buffer,mix_cnt,MPI_CHAR,dest,MIX_MPI_DISK_TAG,MPI_COMM_WORLD);
						check_MPI_success(gv, errorcode);
						t1 = get_cur_time();
						mix_send_time += t1-t0;
						mix_msg_id++;
						gv->disk_id += send_counter;

						gv->mpi_send_progress_counter += send_counter+1;
						// send_counter = 0;
						free(buffer);
						// if(gv->mpi_send_progress_counter>=gv->cpt_total_blks)
						// 	break;
				    }
				    // Long_msg
				    else{
					    t0 = get_cur_time();
					    errorcode = MPI_Send(buffer,gv->block_size+sizeof(int),MPI_CHAR,dest,MPI_MSG_TAG,MPI_COMM_WORLD);
					    check_MPI_success(gv, errorcode);
					    t1 = get_cur_time();
					    only_mpi_send_time += t1-t0;

					 //    block_id=*(int*)buffer;
					 //    int count=518448;
					 //    printf("Compute %d Sender %d finish sending %d buffer[518448]=%f\n",
					 //    	gv->rank[0], lv->tid, block_id, *((double *)(buffer+20+count+8)) );
						// fflush(stdout);

					    gv->mpi_send_progress_counter++;
					    long_msg_id++;
					    free(buffer);
				      	// if(gv->mpi_send_progress_counter>=gv->cpt_total_blks)
				      	// 	break;
				    }

				}
			}

			break;
		}
	}
	t3 = get_cur_time();

	printf("Compute Process %d Sender %d total_time=%f, mpi_send_progress_counter=%d,\
mix_send_time=%f, only_mpi_send_time=%f, mix_msg_id=%d, disk_id=%d, long_msg_id=%d\n",
    	gv->rank[0], lv->tid, t3-t2, gv->mpi_send_progress_counter,
    	mix_send_time, only_mpi_send_time, mix_msg_id, gv->disk_id, long_msg_id);
  	fflush(stdout);


}
