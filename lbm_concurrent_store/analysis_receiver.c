#include "concurrent.h"

void recv_ring_buffer_put(GV gv,LV lv,char * buffer){

  ring_buffer *rb = gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements < rb->bufsize) {
      rb->buffer[rb->head] = buffer;
      rb->head = (rb->head + 1) % rb->bufsize;
      rb->num_avail_elements++;
      pthread_cond_signal(rb->empty);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);
    }
  }
}

void copy_msg_int(int* temp1,int* temp2,int num_int){
  for(int i=0;i<num_int;i++)
    temp1[i]=temp2[i];
}

void make_prefetch_id(GV gv, int cid, int num_int,int* temp_int_pointer){
  int i;
  int* temp1 = (int*) gv->prefetch_id_array;
  int* temp2 = temp_int_pointer;
  for(i=1;i<=num_int;i++){
    temp1[gv->recv_tail]=cid;
    temp1[gv->recv_tail+1]=temp2[i];
    // printf("Receiver MIX: written id= %d\n", temp2[i]);
    // fflush(stdout);
    // if(temp2[i]>gv->ana_total_blks){
    //   printf("Error! Compute %d Receiver get temp2[i]=%d\n", temp2[i], gv->rank[0]);
    //   fflush(stdout);
    // }
    gv->recv_tail+=2;
  }
  //printf("After copy short_msg, Ana Node %d Receive thread recv_tail = %d\n", gv->rank[0], gv->recv_tail);
  // fflush(stdout);
}

void make_prefetch_id_v2(GV gv, int cid, int num_int,int* temp_int_pointer){
  int i;
  int* temp1 = (int*) gv->prefetch_id_array;
  int* temp2 = temp_int_pointer;
  for(i=0;i<num_int;i++){
    temp1[gv->recv_tail]=cid;
    temp1[gv->recv_tail+1]=temp2[i];
    // printf("Disk: written id= %d\n", temp2[i]);
    // fflush(stdout);
    // if(temp2[i]>gv->ana_total_blks){
    //   printf("Error! Compute %d Receiver get temp2[i]=%d\n", temp2[i], gv->rank[0]);
    //   fflush(stdout);
    // }
    gv->recv_tail+=2;
  }
  //printf("After copy short_msg, Ana Node %d Receive thread recv_tail = %d\n", gv->rank[0], gv->recv_tail);
  // fflush(stdout);
}

void analysis_receiver_thread(GV gv,LV lv){
  int recv_int=0,block_id=0;
  double t0=0, t1=0,t2=0,t3=0,t4=0,t5=0, wait_lock=0;
  double receive_time=0;
  MPI_Status status;
  int errorcode,long_msg_id=0,mix_msg_id=0,disk_id=0;
  int* temp_int_pointer;
  char* new_buffer=NULL;

  // printf("Ana Node %d Receiveing thread %d Start receive!\n",gv->rank[0], lv->tid);

  t0 = get_cur_time();
  while(1){
    if(gv->mpi_recv_progress_counter>=gv->ana_total_blks) {
        break;
    }

    // #ifdef DEBUG_PRINT
    // printf("Prepare to receive!\n");
    // fflush(stdout);
    // #endif //DEBUG_PRINT

    t2 = get_cur_time();
    errorcode = MPI_Recv(gv->org_recv_buffer, gv->compute_data_len, MPI_CHAR, MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD, &status);
    if(errorcode!= MPI_SUCCESS){
        printf("Ana Node %d Error MPI receive!\n",gv->rank[0]);
        fflush(stdout);
        exit(1);
    }
    t3 = get_cur_time();
    receive_time += t3-t2;

    // #ifdef DEBUG_PRINT
    // printf("Already received!\n");
    // fflush(stdout);
    // #endif //DEBUG_PRINT

    if(status.MPI_TAG==MPI_MSG_TAG){
      #ifdef DEBUG_PRINT
      printf("Enter MPI MSG!\n");
      fflush(stdout);
      #endif //DEBUG_PRINT
      /***************************receive a long_msg***************************************/
      // pthread_mutex_lock(&gv->lock_recv);
      // gv->prefetch_counter++;
      // pthread_mutex_unlock(&gv->lock_recv);

      // if(long_messageind%3000==0)
      //   printf("LONG!____Node %d Receive thread %d mpi_recv_progress_counter=%ld, long_messageind= %ld, short_messageind=%ld\n", gv->rank[0], lv->tid, gv->mpi_recv_progress_counter,long_messageind,short_messageind);

      gv->mpi_recv_progress_counter++;
      long_msg_id++;
      #ifdef DEBUG_PRINT
      printf("Get a Long msg!long_messageind=%d,gv->mpi_recv_progress_counter=%d\n",
        long_msg_id,gv->mpi_recv_progress_counter);
      fflush(stdout);
      #endif //DEBUG_PRINT

      new_buffer = (char*) malloc(gv->analysis_data_len);
      temp_int_pointer = (int*)new_buffer;
      check_malloc(new_buffer);
      temp_int_pointer[0] = status.MPI_SOURCE;
      block_id =*((int *)(gv->org_recv_buffer));
      temp_int_pointer[1] = block_id;
      // temp_int_pointer[2] = BLANK;
      new_buffer[8] = NOT_ON_DISK;
      new_buffer[9] = NOT_CALC;
      copy_msg_int(temp_int_pointer+3,(int*)(gv->org_recv_buffer+sizeof(int)),gv->block_size/sizeof(int));
      recv_ring_buffer_put(gv,lv,new_buffer);
    }

    else if (status.MPI_TAG == MIX_MPI_DISK_TAG) {
      #ifdef DEBUG_PRINT
      printf("Enter MIX_MPI_DISK_TAG MSG!\n");
      fflush(stdout);
      #endif //DEBUG_PRINT
      /***************************RECEIVE DISK INDEX ARRAY***************************************/
      temp_int_pointer = (int*)(gv->org_recv_buffer+sizeof(int)+sizeof(char)*gv->block_size);
      recv_int=*(int *)(temp_int_pointer);
      disk_id += recv_int;
      mix_msg_id++;
      gv->mpi_recv_progress_counter += recv_int+1;
      // #ifdef DEBUG_PRINT
      // printf("mix_msg_id:- recv_int=%d,gv->mpi_recv_progress_counter=%d,temp_int_pointer[1]=%d\n",
      //   recv_int,gv->mpi_recv_progress_counter,temp_int_pointer[1]);
      // fflush(stdout);
      // #endif //DEBUG_PRINT

      // statistic !!!!!!!!
      t4 = get_cur_time();
      pthread_mutex_lock(&gv->lock_recv);
      // gv->prefetch_counter++;
      make_prefetch_id(gv, status.MPI_SOURCE, recv_int,temp_int_pointer+1);
      pthread_mutex_unlock(&gv->lock_recv);
      t5 = get_cur_time();
      wait_lock += t5-t4;

      new_buffer = (char*) malloc(gv->analysis_data_len);
      temp_int_pointer = (int*)new_buffer;
      check_malloc(new_buffer);
      temp_int_pointer[0] = status.MPI_SOURCE;
      block_id =*((int *)(gv->org_recv_buffer));

      // #ifdef DEBUG_PRINT
      // printf("Analysis Process %d Receiver %d Get a MIX msg! long_messageind=%d,gv->mpi_recv_progress_counter=%d,block_id=%d\n",
      //   gv->rank[0], lv->tid, mix_msg_id,gv->mpi_recv_progress_counter,block_id);
      // fflush(stdout);
      // #endif //DEBUG_PRINT

      temp_int_pointer[1] = block_id;
      // temp_int_pointer[2] = BLANK;
      new_buffer[8] = NOT_ON_DISK;
      new_buffer[9] = NOT_CALC;
      copy_msg_int(temp_int_pointer+3,(int*)(gv->org_recv_buffer+sizeof(int)),gv->block_size/sizeof(int));

      recv_ring_buffer_put(gv,lv,new_buffer);
      //printf("Node 2 RECEIVE thread %d using recv_progress_counter = %d\n", lv->tid, gv->recv_progress_counter);

    }
    else if(status.MPI_TAG == DISK_TAG){
      MPI_Get_count(&status, MPI_CHAR, &recv_int);
      recv_int=recv_int/sizeof(int);
      printf("pure_disk_msg:- recv_int=%d,gv->mpi_recv_progress_counter=%d\n",
        recv_int,gv->mpi_recv_progress_counter);
      fflush(stdout);


      temp_int_pointer=(int*)gv->org_recv_buffer;
      // for(int i=0;i<recv_int;i++){
      //   printf("%d ", temp_int_pointer[i]);
      //   fflush(stdout);
      // }
      // printf("\n");
      t4 = get_cur_time();
      pthread_mutex_lock(&gv->lock_recv);
      make_prefetch_id_v2(gv, status.MPI_SOURCE, recv_int, temp_int_pointer);
      // temp_int_pointer=(int*) gv->prefetch_id_array;
      // for(int i=0;i<gv->recv_tail;i++){
      //   printf("%d ", temp_int_pointer[i]);
      //   fflush(stdout);
      // }
      // printf("\n");
      pthread_mutex_unlock(&gv->lock_recv);
      t5 = get_cur_time();
      wait_lock += t5-t4;

      gv->mpi_recv_progress_counter += recv_int;
    }
    else{
      printf("Analysis Process %d receive error!!!!! Get a Tag=%d\n", gv->rank[0], status.MPI_TAG);
      fflush(stdout);
      // exit(1);
    }
  }
  t1 = get_cur_time();

  printf("Analysis Process %d Receive thread %d total time is %f, mpi_recv_progress_counter=%d, \
receive_time = %f, wait_lock = %f, long_msg_id=%d, mix_msg_id= %d,disk_id=%d\n",
    gv->rank[0], lv->tid, t1-t0, gv->mpi_recv_progress_counter,
    receive_time, wait_lock, long_msg_id ,mix_msg_id,disk_id);

}
