#include "do_thread.h"

void recv_ring_buffer_put(GV gv, LV lv, char* buffer, int* num_avail_elements){

  ring_buffer *rb = gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements < rb->bufsize) {
      rb->buffer[rb->head] = buffer;

#ifdef DEBUG_PRINT
    printf("Ana_Proc%d: Receiver%d ****Put-a-pointer-in-CRB**** src=%d, block_id=%d, rb->num_avail_elements=%d, @ rb->head=%d\n",
      gv->rank[0], lv->tid, ((int*)buffer)[0], ((int*)buffer)[1], rb->num_avail_elements, rb->head);
    fflush(stdout);
#endif //DEBUG_PRINT

      rb->head = (rb->head+1) % rb->bufsize;
      *num_avail_elements = ++rb->num_avail_elements;

      pthread_cond_broadcast(rb->empty);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {

#ifdef DEBUG_PRINT
    printf("Ana_Proc%d: Receiver%d Prepare to Sleep! rb->num_avail_elements=%d, rb->head=%d\n", gv->rank[0], lv->tid, rb->num_avail_elements, rb->head);
    fflush(stdout);
#endif //DEBUG_PRINT

      lv->wait++;
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);

#ifdef DEBUG_PRINT
    printf("Ana_Proc%d: Receiver%d Wake up! rb->num_avail_elements=%d, rb->head=%d\n", gv->rank[0], lv->tid, rb->num_avail_elements, rb->head);
    fflush(stdout);
#endif //DEBUG_PRINT
    }
  }
}

// void copy_msg_int(int* temp1,int* temp2,int num_int){
//   for(int i=0;i<num_int;i++)
//     temp1[i]=temp2[i];
// }

void make_prefetch_id(GV gv, int src, int num_int, int* tmp_int_ptr){
  int i;
  int* temp1 = (int*) gv->prefetch_id_array;
  int* temp2 = tmp_int_ptr;

  for(i=0;i<num_int;i++){
    temp1[gv->recv_head] = src;
    temp1[gv->recv_head+1] = temp2[i];
    // printf("Receiver MIX: written id= %d\n", temp2[i]);
    // fflush(stdout);
    // if(temp2[i]>gv->ana_total_blks){
    //   printf("Error! Compute %d Receiver get temp2[i]=%d\n", temp2[i], gv->rank[0]);
    //   fflush(stdout);
    // }

    // if(temp2[i]==0){
    //   printf("Ana_Proc%d: Receiver Get a MIX_msg! In make_prefetch_id, temp2[i]=%d, prog=%d\n",
    //       gv->rank[0], temp2[i], prog);
    //   fflush(stdout);
    // }

    gv->recv_head+=2;
    gv->recv_avail+=2;
  }
  //printf("After copy short_msg, Ana Node %d Receive thread recv_tail = %d\n", gv->rank[0], gv->recv_tail);
  // fflush(stdout);
}



void analysis_receiver_thread(GV gv,LV lv){
  int recv_int=0, block_id=0, source=0;
  double t0=0, t1=0, t2=0, t3=0, t4=0, t5=0, mkidarr_time=0;
  double receive_time=0;
  MPI_Status status;
  int errorcode, long_msg_id=0, mix_msg_id=0, disk_id=0;
  int* tmp_int_ptr;
  char* new_buffer=NULL;
  int num_exit_flag = 0;
  int num_avail_elements=0, full=0, prog=0;

  // printf("Analysis Process %d Receiveing thread %d Start receive!\n",gv->rank[0], lv->tid);
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
    int i;
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

  t0 = MPI_Wtime();
  while(1){

    if ((num_exit_flag >= gv->computer_group_size) && (prog>=gv->ana_total_blks)) {
      gv->ana_reader_done=1;
      break;
    }

    // #ifdef DEBUG_PRINT
    // printf("Prepare to receive!\n");
    // fflush(stdout);
    // #endif //DEBUG_PRINT

    t2 = MPI_Wtime();
    errorcode = MPI_Recv(gv->org_recv_buffer, gv->compute_data_len, MPI_CHAR, MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD, &status);
    if(errorcode!= MPI_SUCCESS){
        printf("Analysis Process %d Error MPI receive!\n",gv->rank[0]);
        fflush(stdout);
        exit(1);
    }
    t3 = MPI_Wtime();
    receive_time += t3-t2;

    // #ifdef DEBUG_PRINT
    // printf("Already received!\n");
    // fflush(stdout);
    // #endif //DEBUG_PRINT

    if(status.MPI_TAG==MPI_MSG_TAG){
      // #ifdef DEBUG_PRINT
      // printf("Enter MPI MSG!\n");
      // fflush(stdout);
      // #endif //DEBUG_PRINT
      /***************************receive a long_msg***************************************/
      // pthread_mutex_lock(&gv->lock_recv_disk_id_arr);
      // gv->prefetch_counter++;
      // pthread_mutex_unlock(&gv->lock_recv_disk_id_arr);

      // if(long_messageind%3000==0)
      //   printf("LONG!____Node %d Receive thread %d mpi_recv_progress_counter=%ld, long_messageind= %ld, short_messageind=%ld\n", gv->rank[0], lv->tid, prog,long_messageind,short_messageind);

      prog++;
      long_msg_id++;

#ifdef DEBUG_PRINT
      int count;
      MPI_Get_count(&status, MPI_CHAR, &count);
      printf("Ana_Proc%d: Receiver *LONG MSG* --src=%d-- num_long_msg=%d, prog=%d, get_cnt=%d\n",
        gv->rank[0], status.MPI_SOURCE, long_msg_id, prog, count);
      fflush(stdout);
#endif //DEBUG_PRINT

      new_buffer = (char*) malloc(gv->analysis_data_len);
      tmp_int_ptr = (int*)new_buffer;
      check_malloc(new_buffer);

      source = status.MPI_SOURCE;
      tmp_int_ptr[0] = source;
      block_id = ((int *)gv->org_recv_buffer)[0];
      tmp_int_ptr[1] = block_id;
      tmp_int_ptr[2] = NOT_ON_DISK;
      tmp_int_ptr[3] = NOT_CALC;

      memcpy(new_buffer+sizeof(int)*4, gv->org_recv_buffer, gv->block_size);
      // copy_msg_int(temp_int_pointer+3,(int*)gv->org_recv_buffer,gv->block_size/sizeof(int));

#ifdef DEBUG_PRINT
      if(tmp_int_ptr[4]==0){
        printf("Ana_Proc%d: Receiver%d Get a Long_msg! block_id=%d, prog=%d tmp_int_ptr[4]==%d\n",
          gv->rank[0], lv->tid, block_id, prog, tmp_int_ptr[4]);
        fflush(stdout);
      }
#endif //DEBUG_PRINT

      t4 = MPI_Wtime();
      recv_ring_buffer_put(gv, lv, new_buffer, &num_avail_elements);
      t5 = MPI_Wtime();
      lv->ring_buffer_put_time += t5-t4;


      if(num_avail_elements == gv->consumer_rb_p->bufsize)
        full++;

    }

    else if (status.MPI_TAG == MIX_MPI_DISK_TAG) {
      // #ifdef DEBUG_PRINT
      // printf("Enter MIX_MPI_DISK_TAG MSG!\n");
      // fflush(stdout);
      // #endif //DEBUG_PRINT
      /***************************RECEIVE DISK INDEX ARRAY***************************************/
      tmp_int_ptr = (int*)(gv->org_recv_buffer+sizeof(char)*gv->block_size);
      recv_int=*((int *)(tmp_int_ptr));
      disk_id += recv_int;
      mix_msg_id++;
      prog += recv_int+1;

      // #ifdef DEBUG_PRINT
      // printf("mix_msg_id:- recv_int=%d,prog=%d,tmp_int_ptr[1]=%d\n",
      //   recv_int,prog,tmp_int_ptr[1]);
      // fflush(stdout);
      // #endif //DEBUG_PRINT

      // statistic !!!!!!!!
      t4 = MPI_Wtime();
      pthread_mutex_lock(&gv->lock_recv_disk_id_arr);
      // gv->prefetch_counter++;
      make_prefetch_id(gv, status.MPI_SOURCE, recv_int, tmp_int_ptr+1);
      pthread_mutex_unlock(&gv->lock_recv_disk_id_arr);
      t5 = MPI_Wtime();
      mkidarr_time += t5-t4;

      new_buffer = (char*) malloc(gv->analysis_data_len);
      tmp_int_ptr = (int*)new_buffer;
      check_malloc(new_buffer);
      tmp_int_ptr[0] = status.MPI_SOURCE;
      block_id =*((int *)(gv->org_recv_buffer));
      tmp_int_ptr[1] = block_id;
      tmp_int_ptr[2] = NOT_ON_DISK;
      tmp_int_ptr[3] = NOT_CALC;

      // #ifdef DEBUG_PRINT
      // printf("Analysis Process %d Receiver %d Get a MIX msg! long_messageind=%d,prog=%d,block_id=%d\n",
      //   gv->rank[0], lv->tid, mix_msg_id,prog,block_id);
      // fflush(stdout);
      // #endif //DEBUG_PRINT

      memcpy(new_buffer+sizeof(int)*4, gv->org_recv_buffer, gv->block_size);

#ifdef DEBUG_PRINT
      if(tmp_int_ptr[4]==0){ //first real data
        printf("Ana_Proc%d: Receiver%d Get a MIX_msg! block_id=%d, prog=%d tmp_int_ptr[4]==%d\n",
          gv->rank[0], lv->tid, block_id, prog, tmp_int_ptr[4]);
        fflush(stdout);
      }
#endif //DEBUG_PRINT

      t4 = MPI_Wtime();
      recv_ring_buffer_put(gv, lv, new_buffer, &num_avail_elements);
      t5 = MPI_Wtime();
      lv->ring_buffer_put_time += t5-t4;

      if(num_avail_elements == gv->consumer_rb_p->bufsize)
        full++;

    }
    else if(status.MPI_TAG == DISK_TAG){

      MPI_Get_count(&status, MPI_CHAR, &recv_int);
      recv_int=recv_int/sizeof(int);
      disk_id += recv_int;

      printf("PURE_DISK_MSG:- recv_int=%d, prog=%d\n",
        recv_int,prog);
      fflush(stdout);

      tmp_int_ptr=(int*)gv->org_recv_buffer;
      // for(int i=0;i<recv_int;i++){
      //   printf("%d ", tmp_int_ptr[i]);
      //   fflush(stdout);
      // }
      // printf("\n");
      t4 = MPI_Wtime();
      pthread_mutex_lock(&gv->lock_recv_disk_id_arr);
      make_prefetch_id(gv, status.MPI_SOURCE, recv_int, tmp_int_ptr);
      // tmp_int_ptr=(int*) gv->prefetch_id_array;
      // for(int i=0;i<gv->recv_tail;i++){
      //   printf("%d ", tmp_int_ptr[i]);
      //   fflush(stdout);
      // }
      // printf("\n");
      pthread_mutex_unlock(&gv->lock_recv_disk_id_arr);
      t5 = MPI_Wtime();
      mkidarr_time += t5-t4;

      prog += recv_int;
    }
    else if (status.MPI_TAG == EXIT_MSG_TAG){

      num_exit_flag++;

      new_buffer = (char *)malloc(gv->analysis_data_len);
      check_malloc(new_buffer);

      ((int*)new_buffer)[0] = status.MPI_SOURCE;
      ((int*)new_buffer)[1] = EXIT_BLK_ID;
      ((int*)new_buffer)[2] = ON_DISK;
      ((int*)new_buffer)[3] = CALC_DONE;

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Receiver%d get a *EXIT_MSG_TAG* from src=%d with block_id=%d, num_exit_flag=%d\n",
        gv->rank[0], lv->tid, ((int*)new_buffer)[0], ((int*)new_buffer)[1], num_exit_flag);
      fflush(stdout);
#endif //DEBUG_PRINT

      t4 = MPI_Wtime();
      recv_ring_buffer_put(gv, lv, new_buffer, &num_avail_elements);
      t5 = MPI_Wtime();
      lv->ring_buffer_put_time += t5-t4;

      if(num_avail_elements == gv->consumer_rb_p->bufsize)
        full++;
    }
    else{
      printf("Analysis Process %d receive error!\n", gv->rank[0]);
      fflush(stdout);
      exit(1);
    }
  }
  t1 = MPI_Wtime();

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

  printf("Ana_Proc%04d: Receiver%d T_total=%.3f, prog=%d, \
T_recv_wait=%.3f, T_put=%.3f, T_mkidarr=%.3f, M_long=%d, M_mix=%d, disk=%d, full_wait=%d\n",
     gv->rank[0], lv->tid, t1 - t0, prog,
     receive_time, lv->ring_buffer_put_time, mkidarr_time, long_msg_id, mix_msg_id, disk_id, lv->wait);
  fflush(stdout);
}
