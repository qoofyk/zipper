#include "concurrent.h"

void recv_ring_buffer_put(GV gv, LV lv, char * buffer){

  ring_buffer *rb = gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while (1) {
    if (rb->num_avail_elements < rb->bufsize) {
      rb->buffer[rb->head] = buffer;

#ifdef DEBUG_PRINT
    printf("Ana_Proc%d: Receiver%d ****Put-a-pointer-in-CRB**** src=%d, block_id=%d, rb->num_avail_elements=%d, @ rb->head=%d\n",
      gv->rank[0], lv->tid, ((int*)buffer)[0], ((int*)buffer)[1], rb->num_avail_elements, rb->head);
    fflush(stdout);
#endif //DEBUG_PRINT

      rb->head = (rb->head + 1) % rb->bufsize;
      rb->num_avail_elements++;

      pthread_cond_broadcast(rb->empty);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    }
    else {

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

// void copy_msg_int(int* temp1, int* temp2, int num_int){
//   for (int i = 0; i<num_int; i++)
//     temp1[i] = temp2[i];
// }

void make_prefetch_id(GV gv, int src, int num_int, int* tmp_int_ptr){
  int i;
  int* temp1 = (int*)gv->prefetch_id_array;
  int* temp2 = tmp_int_ptr;

  //LAMMPS_CHANGE: i+=2
  for (i=0; i<num_int; i+=2){
    temp1[gv->recv_tail] = src;
    temp1[gv->recv_tail+1] = temp2[i];
    temp1[gv->recv_tail+2] = temp2[i+1];
#ifdef DEBUG_PRINT
    printf("Ana%d: Receiver MIX make_prefetch_id: src=%d id=%d dump_lines_in_this_blk=%d\n",
      gv->rank[0], src, temp2[i], temp2[i+1]);
    fflush(stdout);
#endif //DEBUG_PRINT
    // if(temp2[i]>gv->ana_total_blks){
    //   printf("Error! Ana%d Receiver get temp2[i]=%d\n", temp2[i], gv->rank[0]);
    //   fflush(stdout);
    // }
    gv->recv_head+=3;
    gv->recv_avail+=3;
  }
  //printf("After copy short_msg, Ana Node %d Receive thread recv_tail = %d\n", gv->rank[0], gv->recv_tail);
  // fflush(stdout);
}


void analysis_receiver_thread(GV gv, LV lv){
  int recv_int=0, block_id=0, source=0;
  double t0=0, t1=0, t2=0, t3=0, t4=0, t5=0, mkidarr_time=0;
  double receive_time=0;
  MPI_Status status;
  int errorcode, long_msg_cnt=0, mix_msg_cnt=0, disk_id=0;
  int* tmp_int_ptr;
  char* new_buffer=NULL;  //2 int = 1 double; to be aligned
  int num_exit_flag=0;
  int dump_lines_in_this_blk;
  int prog=0;
  // printf("Ana_Proc%d: Receiver%d Start receive!\n",gv->rank[0], lv->tid);
  // fflush(stdout);

  t0 = MPI_Wtime();
  while (1){

// #ifdef DEBUG_PRINT
    // printf("Prepare to receive!\n");
    // fflush(stdout);
// #endif //DEBUG_PRINT

    t2 = MPI_Wtime();
    errorcode = MPI_Recv(gv->org_recv_buffer, gv->compute_data_len, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    if (errorcode != MPI_SUCCESS){
      printf("Ana_Proc%d: Error MPI receive!\n", gv->rank[0]);
      fflush(stdout);
      exit(1);
    }
    t3 = MPI_Wtime();
    receive_time += t3 - t2;

    // #ifdef DEBUG_PRINT
    // printf("Already received!\n");
    // fflush(stdout);
    // #endif //DEBUG_PRINT

    if (status.MPI_TAG == MPI_MSG_TAG){
#ifdef DEBUG_PRINT
      printf("Enter MPI MSG!\n");
      fflush(stdout);
#endif //DEBUG_PRINT
      /***************************receive a long_msg***************************************/
      // pthread_mutex_lock(&gv->lock_recv_disk_id_arr);
      // gv->prefetch_counter++;
      // pthread_mutex_unlock(&gv->lock_recv_disk_id_arr);

      // if(long_messageind%3000==0)
      //   printf("LONG!____Node %d Receive thread %d mpi_recv_progress_counter=%ld, long_messageind= %ld, short_messageind=%ld\n", gv->rank[0], lv->tid, prog,long_messageind,short_messageind);

      prog++;
      long_msg_cnt++;

#ifdef DEBUG_PRINT
      int count;
      MPI_Get_count(&status, MPI_CHAR, &count);

      printf("Ana_Proc%d: Receiver%d *****Recv a LONG_MSG**** --src=%d block_id=%d-- long_msg_cnt=%d, gv->mpi_recv_prog_cnt=%d, get_cnt=%d\n",
        gv->rank[0], lv->tid, status.MPI_SOURCE, ((int *)gv->org_recv_buffer)[0], long_msg_cnt, prog, count);
      fflush(stdout);
#endif //DEBUG_PRINT

      new_buffer = (char *)malloc(gv->analysis_data_len);
      tmp_int_ptr = (int *)new_buffer;
      check_malloc(new_buffer);

      source = status.MPI_SOURCE;
      tmp_int_ptr[0] = source;
      block_id = ((int *)gv->org_recv_buffer)[0];
      tmp_int_ptr[1] = block_id;
      tmp_int_ptr[2] = NOT_ON_DISK;
      tmp_int_ptr[3] = NOT_CALC;
      tmp_int_ptr[4] = ((int *)gv->org_recv_buffer)[1]; //time_step
      tmp_int_ptr[5] = ((int *)gv->org_recv_buffer)[2]; //dump_lines_per_blk
      dump_lines_in_this_blk = tmp_int_ptr[5];

// #ifdef DEBUG_PRINT
      if(block_id<0){
        printf("Ana_Proc%d: Receiver%d ___Before_memcpy_LONG_MSG___ src=%d, block_id=%d, write=%d, calc=%d, time_step=%d, dump_lines_in_this_blk=%d\n",
          gv->rank[0], lv->tid, tmp_int_ptr[0], tmp_int_ptr[1], tmp_int_ptr[2], tmp_int_ptr[3], tmp_int_ptr[4], tmp_int_ptr[5]);
        fflush(stdout);
      }
// #endif //DEBUG_PRINT

      memcpy(new_buffer+sizeof(int)*6, gv->org_recv_buffer+sizeof(int)*3, sizeof(double)*5*dump_lines_in_this_blk);
      // copy_msg_int(tmp_int_ptr + 3, (int*)(gv->org_recv_buffer + sizeof(int)), gv->block_size / sizeof(int));

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Receiver%d ___After_memcpy_LONG_MSG___ with block_id=%d, long_msg_cnt=%d, prog=%d\n",
        gv->rank[0], lv->tid, block_id, long_msg_cnt, prog);
      fflush(stdout);
#endif //DEBUG_PRINT

      t4 = MPI_Wtime();
      recv_ring_buffer_put(gv, lv, new_buffer);
      t5 = MPI_Wtime();
      lv->ring_buffer_put_time += t5-t4;

    }

    else if (status.MPI_TAG == MIX_MPI_DISK_TAG) {

#ifdef DEBUG_PRINT
      printf("Enter MIX_MPI_DISK_TAG MSG!\n");
      fflush(stdout);
#endif //DEBUG_PRINT

      /***************************RECEIVE DISK INDEX ARRAY***************************************/
      tmp_int_ptr = (int*)(gv->org_recv_buffer + sizeof(int)*3 + sizeof(double)*5*dump_lines_in_this_blk);
      // recv_int = send_counter
      recv_int = *(int *)(tmp_int_ptr);
      disk_id += recv_int;
      mix_msg_cnt++;
      prog += recv_int+1;

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Receiver%d recv_int=%d, prog=%d, tmp_int_ptr[1]=%d\n",
        gv->rank[0], lv->tid, recv_int, prog, tmp_int_ptr[1]);
      fflush(stdout);
#endif //DEBUG_PRINT

      // statistic !!!!!!!!
      t4 = MPI_Wtime();
      pthread_mutex_lock(&gv->lock_recv_disk_id_arr);
      make_prefetch_id(gv, status.MPI_SOURCE, recv_int, tmp_int_ptr + 1);
      pthread_mutex_unlock(&gv->lock_recv_disk_id_arr);
      t5 = MPI_Wtime();
      mkidarr_time += t5 - t4;

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Receiver%d ----After_make_prefetch_id a MIX_MSG---- src=%d, mix_msg_cnt=%d, prog=%d\n",
        gv->rank[0], lv->tid, status.MPI_SOURCE, mix_msg_cnt, prog);
      fflush(stdout);
#endif //DEBUG_PRINT

      //prepare to put the long msg
      new_buffer = (char *)malloc(gv->analysis_data_len);
      tmp_int_ptr = (int *)new_buffer;
      check_malloc(new_buffer);

      tmp_int_ptr[0] = status.MPI_SOURCE;
      block_id = ((int *)gv->org_recv_buffer)[0];
      tmp_int_ptr[1] = block_id;
      tmp_int_ptr[2] = NOT_ON_DISK;
      tmp_int_ptr[3] = NOT_CALC;
      tmp_int_ptr[4] = ((int *)gv->org_recv_buffer)[1]; //time_step
      tmp_int_ptr[5] = ((int *)gv->org_recv_buffer)[2]; //dump_lines_in_this_blk
      dump_lines_in_this_blk = tmp_int_ptr[5];

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Receiver%d ----Recv a MIX_MSG---- src=%d, block_id=%d, dump_lines_in_this_blk=%d, mix_msg_cnt=%d\n",
        gv->rank[0], lv->tid, status.MPI_SOURCE, block_id, dump_lines_in_this_blk, mix_msg_cnt);
      fflush(stdout);
#endif //DEBUG_PRINT

      memcpy(tmp_int_ptr+6, gv->org_recv_buffer+sizeof(int)*3, sizeof(double)*5*dump_lines_in_this_blk);
      // copy_msg_int(tmp_int_ptr + 3, (int*)(gv->org_recv_buffer + sizeof(int)), gv->block_size / sizeof(int));

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Receiver%d ----After_memcpy a MIX_MSG---- src=%d, block_id=%d, dump_lines_in_this_blk=%d, mix_msg_cnt=%d\n",
        gv->rank[0], lv->tid, status.MPI_SOURCE, block_id, dump_lines_in_this_blk, mix_msg_cnt);
      fflush(stdout);
#endif //DEBUG_PRINT

      t4 = MPI_Wtime();
      recv_ring_buffer_put(gv, lv, new_buffer);
      t5 = MPI_Wtime();
      lv->ring_buffer_put_time += t5-t4;

    }
    else if (status.MPI_TAG == DISK_TAG){

      //get each block with blkid+dump_lines_in_this_blk
      MPI_Get_count(&status, MPI_CHAR, &recv_int);
      recv_int = recv_int / sizeof(int);
      prog += recv_int/2;

// #ifdef DEBUG_PRINT
      printf("pure_disk_msg:- recv_int=%d, prog=%d\n",
        recv_int, prog);
      fflush(stdout);
// #endif //DEBUG_PRINT

      tmp_int_ptr = (int*)gv->org_recv_buffer;

      t4 = MPI_Wtime();
      pthread_mutex_lock(&gv->lock_recv_disk_id_arr);
      make_prefetch_id(gv, status.MPI_SOURCE, recv_int, tmp_int_ptr);
      pthread_mutex_unlock(&gv->lock_recv_disk_id_arr);
      t5 = MPI_Wtime();
      mkidarr_time += t5 - t4;

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
      printf("Ana_Proc%d: Receiver%d --GET-- a *EXIT_MSG_TAG* from src=%d with block_id=%d, num_exit_flag=%d\n",
        gv->rank[0], lv->tid, ((int*)new_buffer)[0], ((int*)new_buffer)[1], num_exit_flag);
      fflush(stdout);
#endif //DEBUG_PRINT

      if(num_exit_flag==gv->computer_group_size){

#ifdef DEBUG_PRINT
        printf("Ana_Proc%04d: Receiver%d Ready to put the last EXIT, num_exit_flag=%d\n",
          gv->rank[0], lv->tid, num_exit_flag);
        fflush(stdout);
#endif //DEBUG_PRINT

        gv->recv_exit = 1;

        while(gv->reader_exit==0);

        t4 = MPI_Wtime();
        recv_ring_buffer_put(gv, lv, new_buffer);
        t5 = MPI_Wtime();
        lv->ring_buffer_put_time += t5-t4;

        break;
      }

      t4 = MPI_Wtime();
      recv_ring_buffer_put(gv, lv, new_buffer);
      t5 = MPI_Wtime();
      lv->ring_buffer_put_time += t5-t4;

#ifdef DEBUG_PRINT
      printf("Ana_Proc%d: Receiver%d --INSERT-- a *EXIT_MSG_TAG* from src=%d with block_id=%d, num_exit_flag=%d\n",
        gv->rank[0], lv->tid, ((int*)new_buffer)[0], ((int*)new_buffer)[1], num_exit_flag);
      fflush(stdout);
#endif //DEBUG_PRINT

    }
    else{
      printf("Ana_Proc%d: receive error!!!!! Get a Tag=%d\n", gv->rank[0], status.MPI_TAG);
      fflush(stdout);
      // exit(1);
    }
  }
  t1 = MPI_Wtime();

  printf("Ana_Proc%04d: Receiver%d T_total=%.3f, prog=%d, \
T_recv_wait=%.3f, T_put=%.3f,T_wait_lock=%.3f, M_long=%d, mix_msg_cnt=%d, disk_id=%d, full_wait=%d, #_exit=%d\n",
       gv->rank[0], lv->tid, t1 - t0, prog,
       receive_time, lv->ring_buffer_put_time, mkidarr_time, long_msg_cnt, mix_msg_cnt, disk_id, lv->wait, num_exit_flag);
  fflush(stdout);
}
