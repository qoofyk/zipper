#include "do_thread.h"

char* sender_ring_buffer_get(GV gv, LV lv){
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
		// *num_avail_elements = --rb->num_avail_elements;
		--rb->num_avail_elements;
		pthread_cond_signal(rb->full);
		pthread_mutex_unlock(rb->lock_ringbuffer);
		return pointer;
    } else {
    	lv->wait++;
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
	int mix_cnt=0, errorcode=0, long_msg_id=0, mix_msg_id=0, disk_id=0;
	double mix_send_time=0, pure_mpi_send_time=0;
	int disk_msg_flag=0;
	char exit_flag = 0;
	int block_id, prog=0;
	char my_exit_flag=0;
	// int num_avail_elements=0, remain=0;
	// printf("Compute %d Sender %d start running!\n", gv->rank[0], lv->tid);
	// fflush(stdout);

#ifdef ADD_PAPI
	int retval,cid,numcmp;
    int EventSet = PAPI_NULL;
    long long *values = 0;
    int *codes = 0;
    char *names = 0;
    int code;
    int total_events=0;
    int r;
    const PAPI_component_info_t *cmpinfo = NULL;

	numcmp = PAPI_num_components();

    for(cid=0; cid<numcmp; cid++) {

        if ( (cmpinfo = PAPI_get_component_info(cid)) == NULL) {
            test_fail(__FILE__, __LINE__,"PAPI_get_component_info failed\n",-1);
        }

        if (!TESTS_QUIET) {
            printf("Proc%d: Component %d - %d events - %s\n", gv->rank[0], cid,
                cmpinfo->num_native_events, cmpinfo->name);
        }

        if ( strstr(cmpinfo->name, "infiniband") == NULL) {
            continue;
        }
        if (cmpinfo->disabled) {
            test_skip(__FILE__,__LINE__,"Component infiniband is disabled", 0);
            continue;
        }

        values = (long long*) malloc(sizeof(long long) * cmpinfo->num_native_events);
        codes = (int*) malloc(sizeof(int) * cmpinfo->num_native_events);
        names = (char*) malloc(PAPI_MAX_STR_LEN * cmpinfo->num_native_events);

        EventSet = PAPI_NULL;

        retval = PAPI_create_eventset( &EventSet );
        if (retval != PAPI_OK) {
            test_fail(__FILE__, __LINE__, "PAPI_create_eventset()", retval);
        }

        code = PAPI_NATIVE_MASK;

        r = PAPI_enum_cmp_event( &code, PAPI_ENUM_FIRST, cid );
        i = 0;
        while ( r == PAPI_OK ) {

            retval = PAPI_event_code_to_name( code, &names[i*PAPI_MAX_STR_LEN] );
            if ( retval != PAPI_OK ) {
                test_fail( __FILE__, __LINE__, "PAPI_event_code_to_name", retval );
            }
            codes[i] = code;

            retval = PAPI_add_event( EventSet, code );
            if (retval != PAPI_OK) {
                test_fail(__FILE__, __LINE__, "PAPI_add_event()", retval);
            }

            total_events++;

            r = PAPI_enum_cmp_event( &code, PAPI_ENUM_EVENTS, cid );
            i += 1;
        }

        retval = PAPI_start( EventSet );
        if (retval != PAPI_OK) {
            test_fail(__FILE__, __LINE__, "PAPI_start()", retval);
        }
#endif //ADD_PAPI


	ring_buffer *rb = gv->producer_rb_p;

	t2 = MPI_Wtime();
	while(1){

		buffer = sender_ring_buffer_get(gv, lv);

		if(buffer != NULL){
			tmp_int_ptr = (int*)(buffer + sizeof(char)*gv->block_size);
			my_count++;

			block_id = ((int*)buffer)[0];

			if (block_id != EXIT_BLK_ID){

				// if(num_avail_elements>0) remain++;

				// printf("Comp_Proc%d: Sender%d get block_id %d\n", gv->rank[0], lv->tid, block_id);
				// fflush(stdout);

				//check disk_id_array
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
					tmp_int_ptr[0] = send_counter;
					gv->send_tail = 0;
				}
				pthread_mutex_unlock(&gv->lock_disk_id_arr);

				// MIX_MSG
				if (send_counter > 0){

					// printf("Compute %d Sender send a MIX message send_counter=%d\n", gv->rank[0], send_counter);
					// fflush(stdout);

					t0 = MPI_Wtime();
					mix_cnt = gv->block_size + sizeof(int)*(send_counter+1);
					errorcode = MPI_Send(buffer, mix_cnt, MPI_CHAR, dest, MIX_MPI_DISK_TAG, MPI_COMM_WORLD);
					check_MPI_success(gv, errorcode);
					t1 = MPI_Wtime();

					mix_send_time += t1 - t0;
					mix_msg_id++;
					disk_id += send_counter;

					prog += send_counter+1;
					send_counter = 0;
					free(buffer);

				}
				// LONG_MSG
				else{

					if(block_id<0){
						printf("Comp_Proc%d: Sender%d prepare to send *LONG_MSG* blkid=%d\n", gv->rank[0], lv->tid, block_id);
						fflush(stdout);
					}

					t0 = MPI_Wtime();
					errorcode = MPI_Send(buffer, gv->block_size, MPI_CHAR, dest, MPI_MSG_TAG, MPI_COMM_WORLD);
					check_MPI_success(gv, errorcode);
					t1 = MPI_Wtime();
					pure_mpi_send_time += t1 - t0;

					prog++;
					long_msg_id++;
					free(buffer);

				}
			}
			else{
#ifdef DEBUG_PRINT
				printf("Comp_Proc%d: Sender%d Get exit flag msg and prepare to quit\n",
					gv->rank[0], lv->tid);
				fflush(stdout);
#endif //DEBUG_PRINT

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

#ifdef DEBUG_PRINT
			printf("Comp_Proc%d: Sender%d *Discover* *Writer* Get exit flag msg and prepare to quit\n",
					gv->rank[0], lv->tid);
			fflush(stdout);
#endif //DEBUG_PRINT

			my_exit_flag=1;
		}

		if(my_exit_flag==1){

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

				//send EXIT msg
				errorcode = MPI_Send(&exit_flag, sizeof(char), MPI_CHAR, dest, EXIT_MSG_TAG, MPI_COMM_WORLD);
				check_MPI_success(gv, errorcode);
			}

			// Special case: sender finish its job early and wait for writer to finish
			if (disk_msg_flag == 1){

#ifdef DEBUG_PRINT
				printf("Comp_Proc%d: ---Special case--- Sender%d wait for Writer and send the last msg with %d blocks!!!!---###---\n",
					gv->rank[0], lv->tid, remain_disk_id);
				fflush(stdout);
#endif //DEBUG_PRINT

				errorcode = MPI_Send(gv->written_id_array, remain_disk_id*sizeof(int), MPI_CHAR, dest, DISK_TAG, MPI_COMM_WORLD);
				check_MPI_success(gv, errorcode);

				disk_id += remain_disk_id;

				//send EXIT msg
				errorcode = MPI_Send(&exit_flag, sizeof(char), MPI_CHAR, dest, EXIT_MSG_TAG, MPI_COMM_WORLD);
				check_MPI_success(gv, errorcode);
			}

			break;
		}
	}
	t3 = MPI_Wtime();

#ifdef ADD_PAPI
	retval = PAPI_stop( EventSet, values);
        if (retval != PAPI_OK) {
            test_fail(__FILE__, __LINE__, "PAPI_stop()", retval);
        }

        printf("Papi Stop: I am Proc%d, TESTS_QUIET=%d\n", gv->rank[0], TESTS_QUIET);
        fflush(stdout);

        if (!TESTS_QUIET) {
           for (i=0 ; i<cmpinfo->num_native_events ; ++i)
               printf("Proc%d: %#x %-24s = %lld\n", gv->rank[0], codes[i], names+i*PAPI_MAX_STR_LEN, values[i]);
        }

        retval = PAPI_cleanup_eventset( EventSet );
        if (retval != PAPI_OK) {
            test_fail(__FILE__, __LINE__, "PAPI_cleanup_eventset()", retval);
        }

        retval = PAPI_destroy_eventset( &EventSet );
        if (retval != PAPI_OK) {
            test_fail(__FILE__, __LINE__, "PAPI_destroy_eventset()", retval);
        }

        free(names);
        free(codes);
        free(values);
    }

    if (total_events==0) {
        test_skip(__FILE__,__LINE__,"No infiniband events found", 0);
    }
#endif //ADD_PAPI


	printf("Comp_Proc%04d: Sender%d T_total=%.3f, prog=%d, \
T_mix=%.3f, T_long=%.3f, T_total_send=%.3f, M_mix=%d, disk=%d, M_long=%d, empty_wait=%d\n",
    gv->rank[0], lv->tid, t3-t2, prog,
    mix_send_time, pure_mpi_send_time, mix_send_time+pure_mpi_send_time, mix_msg_id, disk_id, long_msg_id, lv->wait);
  fflush(stdout);


}
