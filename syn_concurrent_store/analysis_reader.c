#include "do_thread.h"

//put the buffer into ring_buffer
void ring_buffer_put(GV gv,LV lv,char * buffer){

  ring_buffer *rb;

  rb = (ring_buffer *) gv->consumer_rb_p;

  pthread_mutex_lock(rb->lock_ringbuffer);
  while(1) {
    if (rb->num_avail_elements < rb->bufsize) {
      rb->buffer[rb->head] = buffer;
      rb->head = (rb->head + 1) % rb->bufsize;
      rb->num_avail_elements++;
      pthread_cond_broadcast(rb->empty);
      // pthread_cond_signal(rb->empty);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);
    }
  }
}

void read_blk(GV gv, LV lv,int last_gen_rank,int blk_id, char* buffer){
  char file_name[128];
  FILE *fp=NULL;
  int i=0;
  double t0=0, t1=0;
  //"/N/dc2/scratch/fuyuan/mb/2vmb/mbexp%03dvs%03d/cid%3d/cid%03dthrd%2dblk%d.d"
  sprintf(file_name,ADDRESS,gv->compute_process_num, gv->analysis_process_num, last_gen_rank, last_gen_rank, blk_id);
  // fp=fopen(file_name,"rb");


  while((fp==NULL) && (i<TRYNUM)){
    fp=fopen(file_name,"rb");
      if(fp==NULL){
        if(i==TRYNUM-1){
          printf("Warning: Analysis Process %d Reader %d READ empty file last_gen_rank=%d, blk_id=%d\n",
            gv->rank[0], lv->tid, last_gen_rank, blk_id);
          fflush(stdout);
        }
        i++;
        usleep(100);
    }
  }

  t0 = get_cur_time();
  fread(buffer, gv->block_size, 1, fp);
  t1 = get_cur_time();
  lv->only_fread_time += t1 - t0;

  fclose(fp);
}

void analysis_reader_thread(GV gv,LV lv) {

  int last_gen_rank=0;
  int block_id = 0, read_file_cnt=0;
  double t0=0,t1=0,t2=0,t3=0;

  char* new_buffer=NULL;
  int* temp_int_pointer;
  char flag=0;

  // printf("Analysis Process %d Reader thread %d is running!\n",gv->rank[0], lv->tid);
  // fflush(stdout);

  t2 = get_cur_time();

  // new_buffer = (char*) malloc(sizeof(char)*gv->block_size);

  while(1){
    flag = 0;

    if(gv->reader_blk_num==0) break;

    pthread_mutex_lock(&gv->lock_recv);
    if(gv->ana_progress>=gv->ana_total_blks){
      pthread_mutex_unlock(&gv->lock_recv);
      break;
    }

    if(gv->recv_tail>0){
      flag = 1;
      //printf("Prefetcher %d read recv_tail = %d\n", lv->tid, gv->recv_tail);
      last_gen_rank = gv->prefetch_id_array[gv->recv_tail-2]; // get a snapshot of which block has been generated
      block_id = gv->prefetch_id_array[gv->recv_tail-1];

      // if(gv->prefetch_counter%1000==0)
      //   printf("!!!!!!!!!Node %d Prefetcher %d get lock, prefetch_counter=%ld, last_gen_rank = %d, step = %d, CI=%d, CJ=%d, CK=%d\n", gv->rank[0], lv->tid, gv->prefetch_counter,last_gen_rank, step, CI, CJ, CK);
      gv->recv_tail-=2;
      //printf("Now, Node %d Prefetcher %d minus tail=%d\n", gv->rank[0],lv->tid,gv->recv_tail);
      // gv->prefetch_counter++;
    }

    pthread_mutex_unlock(&gv->lock_recv);


    if(flag == 1){

      new_buffer = (char*) malloc(gv->analysis_data_len);
      temp_int_pointer = (int*)new_buffer;
      temp_int_pointer[0]=last_gen_rank;
      temp_int_pointer[1]=block_id;
      // temp_int_pointer[2]=READ_DONE;
      new_buffer[8] = ON_DISK;
      new_buffer[9] = NOT_CALC;

      #ifdef DEBUG_PRINT
      printf("$$$---$$$ Analysis Process %d Reader %d start read rank%d blk_id%d\n",
        gv->rank[0], lv->tid, last_gen_rank, block_id);
      fflush(stdout);
      #endif //DEBUG_PRINT

      t0 = get_cur_time();
      read_blk(gv, lv,last_gen_rank, block_id, new_buffer+(1+1+1)*sizeof(int));    //read file block to buffer memory
      t1 = get_cur_time();
      lv->read_time += t1 - t0;
      read_file_cnt++;

      t0 = get_cur_time();
      ring_buffer_put(gv,lv,new_buffer);
      t1 = get_cur_time();
      lv->ring_buffer_put_time += t1 - t0;

      pthread_mutex_lock(&gv->lock_recv);
      gv->ana_progress++;
      pthread_mutex_unlock(&gv->lock_recv);

    }

  }

  t3 = get_cur_time();
  printf("Analysis Process %d Reader %d total_time= %f, io_read_time= %f only_fread_time= %f with %d blocks\n",
    gv->rank[0], lv->tid, t3-t2, lv->read_time, lv->only_fread_time, read_file_cnt);
  fflush(stdout);
}
