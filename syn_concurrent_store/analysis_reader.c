#include "do_thread.h"

//put the buffer into ring_buffer
void ring_buffer_put(GV gv, LV lv, char * buffer, int* num_avail_elements){

  ring_buffer *rb;

  rb = (ring_buffer *) gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements < rb->bufsize) {
      rb->buffer[rb->head] = buffer;
      rb->head = (rb->head + 1) % rb->bufsize;
      *num_avail_elements = ++rb->num_avail_elements;

      pthread_cond_broadcast(rb->empty);
      // pthread_cond_signal(rb->empty);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {
      lv->wait++;
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);
    }
  }
}

void read_blk_per_file(GV gv, LV lv, int last_gen_rank, int blk_id, char* buffer, int nbytes){
  char file_name[128];
  FILE *fp=NULL;
  int i=0;
  double t0=0, t1=0;

#ifndef WRITE_ONE_FILE
  sprintf(file_name, "%s/results/cid%d/cid%d", gv->filepath, last_gen_rank, blk_id);
  // fp=fopen(file_name,"rb");
#endif //WRITE_ONE_FILE

  while ((fp == NULL) && (i<TRYNUM)){
    fp = fopen(file_name, "rb");
    if (fp == NULL){
      if (i == TRYNUM-1){
        printf("Warning: Ana_Proc%d: Reader%d read empty file last_gen_rank=%d, blk_id=%d\n",
          gv->rank[0], lv->tid, last_gen_rank, blk_id);
        fflush(stdout);
      }

      i++;
      usleep(OPEN_USLEEP);
    }
  }

  if (fp != NULL){
    t0 = MPI_Wtime();
    fread(buffer, nbytes, 1, fp);
    t1 = MPI_Wtime();
    lv->only_fread_time += t1 - t0;
  }
  else{
    printf("Fatal_Error!!!!!! Ana_Proc%d: Reader%d Makeup a FAKE FILE last_gen_rank=%d, blk_id=%d\n",
      gv->rank[0], lv->tid, last_gen_rank, blk_id);
    fflush(stdout);
  }

  fclose(fp);
}

void ana_read_one_file(GV gv, LV lv, int last_gen_rank, int blk_id, char* buffer, FILE *fp, int nbytes, int* read_wrong){
  double t0=0, t1=0;
  int error=-1;
  int i=0;
  long int offset;
  int* buf_int;

  buf_int = (int*) buffer;

  //---------------------------------
  // char file_name[128];
  // FILE *fp=NULL;
  // sprintf(file_name, ADDRESS, gv->compute_process_num, gv->analysis_process_num, last_gen_rank, last_gen_rank);
  // // printf("Reader: %d %d %d %d %d %d\n r:%s\n", gv->compute_process_num, gv->analysis_process_num, last_gen_rank, last_gen_rank, lv->tid, blk_id, file_name);
  // // fflush(stdout);
  // i=0;
  // error=-1;
  // while ((fp == NULL) && (i<TRYNUM)){
  //   fp = fopen(file_name, "rb+");
  //   if (fp == NULL){
  //     if (i == TRYNUM-1){
  //       printf("Warning: Ana_Proc%d: Reader%d read empty file last_gen_rank=%d, blk_id=%d\n",
  //         gv->rank[0], lv->tid, last_gen_rank, blk_id);
  //       fflush(stdout);
  //     }

  //     i++;
  //     usleep(OPEN_USLEEP);
  //   }
  // }
  //---------------------------------

  offset = (long)blk_id * (long)gv->block_size;
  i=0;
  error=-1;

  while(error!=0){
    error=fseek(fp, offset, SEEK_SET);
    i++;
    // usleep(OPEN_USLEEP);
    if(i>TRYNUM){
      printf("Ana_Proc%d: Reader fseek error src=%d, block_id=%d, fp=%p\n",
        gv->rank[0], last_gen_rank, blk_id, (void *)fp);
      fflush(stdout);
      break;
    }
  }


  t0 = MPI_Wtime();
  error=fread(buffer, nbytes, 1, fp);
  // fflush(fp);
  /* Temp fix to read zero bug*/
  i=0;

  if((blk_id != 0) && ( (buf_int[0]<=0) || (buf_int[0]>gv->cpt_total_blks) ))
    (*read_wrong)++;

  // while( (blk_id != 0) && ( (buf_int[0]<=0) || (buf_int[0]>gv->cpt_total_blks) ) ){
  //   // fseek(fp, offset, SEEK_SET);
  //   fread(buffer, nbytes, 1, fp);
  //   i++;
  //   // usleep(OPEN_USLEEP);
  //   if(i>TRYNUM){
  //     printf("Ana_Proc%d: Reader fread error src=%d, block_id=%d, fp=%p, buf_int[0]=%d\n",
  //           gv->rank[0], last_gen_rank, blk_id, (void *)fp, buf_int[0]);
  //     fflush(stdout);
  //     break;
  //   }
  // }

  if(feof(fp)){
    perror("Ana_Reader EOF:");
    fflush(stdout);
  }

  if(ferror (fp)){
    perror("Ana_Reader error:");
    fflush(stdout);
  }

  t1 = MPI_Wtime();
  lv->only_fread_time += t1 - t0;

  //---------------------
  // fclose(fp);
}

void analysis_reader_thread(GV gv,LV lv) {

  int last_gen_rank=0;
  int block_id=0, read_file_cnt=0;
  double t0=0, t1=0, t2=0, t3=0;

  char* new_buffer=NULL;
  char flag=0;

  int num_avail_elements=0, full=0;
  int recv_avail=0;
  double disk_arr_wait_time=0;
  int read_wrong=0;

  // printf("Analysis Node %d Reader thread %d is running!\n",gv->rank[0], lv->tid);
  // fflush(stdout);

  t2 = MPI_Wtime();

  if(gv->reader_blk_num==0){

    if(gv->rank[0]==gv->compute_process_num || gv->rank[0]==(gv->compute_process_num+gv->analysis_process_num-1)){
      printf("Ana_Proc%d: Reader%d is turned off\n", gv->rank[0], lv->tid);
      fflush(stdout);
    }

  }
  else{
    while(1){
      flag = 0;

      if ( (gv->ana_reader_done==1) && (recv_avail==0))
        break;

      t0 = MPI_Wtime();
      pthread_mutex_lock(&gv->lock_recv);
      if(gv->recv_avail>0){
        flag = 1;
        //printf("Prefetcher %d read recv_tail = %d\n", lv->tid, gv->recv_tail);
        last_gen_rank = gv->prefetch_id_array[gv->recv_tail]; // get a snapshot of which block has been generated
        block_id = gv->prefetch_id_array[gv->recv_tail+1];

        // if(gv->prefetch_counter%1000==0)
        //   printf("!!!!!!!!!Node %d Prefetcher %d get lock, prefetch_counter=%ld, last_gen_rank = %d, step = %d, CI=%d, CJ=%d, CK=%d\n", gv->rank[0], lv->tid, gv->prefetch_counter,last_gen_rank, step, CI, CJ, CK);
        gv->recv_tail+=2;
        gv->recv_avail-=2;
        //printf("Now, Node %d Prefetcher %d minus tail=%d\n", gv->rank[0],lv->tid,gv->recv_tail);
        // gv->prefetch_counter++;
      }
      recv_avail=gv->recv_avail;
      pthread_mutex_unlock(&gv->lock_recv);
      t1 = MPI_Wtime();
      disk_arr_wait_time += t1 - t0;

      if(flag == 1){

        new_buffer = (char *) malloc(gv->analysis_data_len);
        check_malloc(new_buffer);

        ((int*)new_buffer)[0] = last_gen_rank;
        ((int*)new_buffer)[1] = block_id;
        ((int*)new_buffer)[2] = ON_DISK;
        ((int*)new_buffer)[3] = NOT_CALC;

#ifdef DEBUG_PRINT
        printf("Ana_Proc%d: Reader%d starts read src=%d blk_id=%d\n",
          gv->rank[0], lv->tid, last_gen_rank, block_id);
        fflush(stdout);
#endif //DEBUG_PRINT

        t0 = MPI_Wtime();

#ifdef WRITE_ONE_FILE
        ana_read_one_file(gv, lv, last_gen_rank, block_id, new_buffer+4*sizeof(int), gv->ana_read_fp[last_gen_rank%gv->computer_group_size], gv->block_size, &read_wrong);
#else
        read_blk_per_file(gv, lv, last_gen_rank, block_id, new_buffer+4*sizeof(int), gv->block_size);    //read file block to buffer memory
#endif //WRITE_ONE_FILE

#ifdef DEBUG_PRINT
        if(((int*)new_buffer)[4]==0){
          printf("+++++++++++++++++Ana_Proc%d: Reader%d starts read src=%d blk_id=%d, ((int*)new_buffer)[4]=%d, ((int*)new_buffer)[5]=%d\n",
            gv->rank[0], lv->tid, last_gen_rank, block_id, ((int*)new_buffer)[4], ((int*)new_buffer)[5]);
          fflush(stdout);
        }
#endif //DEBUG_PRINT

        t1 = MPI_Wtime();
        lv->read_time += t1 - t0;
        read_file_cnt++;

#ifdef DEBUG_PRINT
        printf("Ana_Proc%d: Reader%d finish read src=%d blk_id=%d\n",
          gv->rank[0], lv->tid, last_gen_rank, block_id);
        fflush(stdout);
#endif //DEBUG_PRINT

        t0 = MPI_Wtime();
        ring_buffer_put(gv, lv, new_buffer, &num_avail_elements);
        t1 = MPI_Wtime();
        lv->ring_buffer_put_time += t1 - t0;

        if(num_avail_elements == gv->consumer_rb_p->bufsize)
          full++;

      }

      if(read_file_cnt>=gv->reader_blk_num)
        break;
    }
  }


  t3 = MPI_Wtime();
  printf("Ana_Proc%04d: Reader%d T_total=%.3f, T_ana_read=%.3f, T_fread=%.3f, T_put=%.3f, T_Darr_wt=%.3f, cnt=%d, full=%d, wait=%d, read_wrong=%d\n",
    gv->rank[0], lv->tid, t3-t2, lv->read_time, lv->only_fread_time, lv->ring_buffer_put_time, disk_arr_wait_time, read_file_cnt, full, lv->wait, read_wrong);
  fflush(stdout);
}
