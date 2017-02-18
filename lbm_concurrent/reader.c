#include "concurrent.h"

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
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);
    }
  }
}

void read_blk(GV gv, LV lv,int last_gen_rank,int blk_id, char* buffer, int nbytes){
  char file_name[128];
  FILE *fp=NULL;
  int i=0;
  double t0=0, t1=0;
  //"/N/dc2/scratch/fuyuan/LBMconcurrent/LBMcon%03dvs%03d/cid%03d/2lbm_cid%03dblk%d.d"
  sprintf(file_name,ADDRESS,gv->num_compute_nodes, gv->num_analysis_nodes, last_gen_rank, last_gen_rank, blk_id);
  // fp=fopen(file_name,"rb");

  while((fp==NULL) && (i<TRYNUM)){
    fp=fopen(file_name,"rb");
    if(fp==NULL){
      if(i==TRYNUM-1){
        printf("Warning: Analysis Process %d Reader %d read an empty file last_gen_rank=%d, blk_id=%d\n",
          gv->rank[0], lv->tid, last_gen_rank, blk_id);
        fflush(stdout);
      }
      i++;
      usleep(100);
    }
  }

  if(fp!=NULL){
    t0 = get_cur_time();
    fread(buffer, nbytes, 1, fp);
    t1 = get_cur_time();
    lv->only_fread_time += t1 - t0;
  }
  else{
    printf("Fatal Error!!!!!! Analysis Process %d Reader %d Makeup a FAKE FILE last_gen_rank=%d, blk_id=%d\n",
          gv->rank[0], lv->tid, last_gen_rank, blk_id);
    fflush(stdout);
  }

  fclose(fp);
}

void reader_thread(GV gv,LV lv) {

  int last_gen_rank=0;
  int block_id = 0, read_file_cnt=0;
  double t0=0,t1=0,t2=0,t3=0;

  char* new_buffer=NULL;
  char flag=0;

  // printf("Analysis Node %d Reader thread %d is running!\n",gv->rank[0], lv->tid);

  t2 = get_cur_time();

  // new_buffer = (char*) malloc(sizeof(char)*gv->block_size);

  while(1){

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

      *(int *)(new_buffer)=last_gen_rank;
      *(int *)(new_buffer+sizeof(int))=block_id;

      // printf("Ana %d Reader %d start read rank%d blk_id%d\n",
      //   gv->rank[0], lv->tid, last_gen_rank, block_id);
      // fflush(stdout);
      t0 = get_cur_time();
      read_blk(gv, lv,last_gen_rank, block_id, new_buffer+(1+1)*sizeof(int),gv->block_size);    //read file block to buffer memory
      t1 = get_cur_time();
      lv->read_time += t1 - t0;
      read_file_cnt++;

      t0 = get_cur_time();
      ring_buffer_put(gv,lv,new_buffer);
      t1 = get_cur_time();
      lv->ring_buffer_put_time += t1 - t0;

      flag = 0;

      pthread_mutex_lock(&gv->lock_recv);
      gv->ana_progress++;
      pthread_mutex_unlock(&gv->lock_recv);
    }

  }

  t3 = get_cur_time();
  printf("Analysis Process %d Reader %d total_time= %f, io_read_time is %f only_fread_time/Block= %f with %d blocks\n",
    gv->rank[0], lv->tid, t3-t2, lv->read_time, lv->only_fread_time/gv->ana_total_blks, read_file_cnt);
  fflush(stdout);
}
