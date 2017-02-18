/********************************************************
Copyright YUANKUN FU
Brief desc of the file: Prefetch thread
********************************************************/
#include "do_thread.h"

void read_blk(GV gv, LV lv, char* buffer, int nbytes, int last_gen_rank, int last_blk_id, FILE *fp){

  double t0=0,t1=0;
  int error=0;

  error=fseek(fp, last_blk_id*gv->block_size, SEEK_SET);
  if(error==-1)
    perror("fseek error:");

  t0 = get_cur_time();
  fread(buffer, nbytes, 1, fp);
  if(ferror(fp)==-1)
    perror("fread error:");
  t1 = get_cur_time();
  lv->only_fread_time += t1 - t0;


}

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
      pthread_cond_signal(rb->empty);
      pthread_mutex_unlock(rb->lock_ringbuffer);
      return;
    } else {
      pthread_cond_wait(rb->full, rb->lock_ringbuffer);
    }
  }
}

//read file from disk
void prefetch_read_fileblk(GV gv,LV lv){
  int last_gen_rank=0,last_blk_id=0;
  char file_name[128];
  FILE **fp=NULL;
  // int step,CI,CJ,CK;
  //int last_read_blk=0;
  //int block_id = 0;
  double t0=0,t1=0,t2=0,t3=0;
  int i=0,j=0;
  char* new_buffer=NULL;
  char flag=0;
  char* open;
  fp = (FILE**)malloc(sizeof(FILE*)*gv->computer_group_size);
  open = (char*)malloc(sizeof(char)*gv->computer_group_size);
  for(j=0;j<gv->computer_group_size;j++){
    fp[j] = NULL;
    open[j] = 0;
  }


  // // fopen file
  // for(j=0;j<gv->computer_group_size;j++){
  //   i=0;
  //   sprintf(file_name, ADDRESS, gv->num_compute_nodes, gv->num_analysis_nodes,
  //     (gv->rank[0]-gv->num_compute_nodes)*gv->computer_group_size+j, (gv->rank[0]-gv->num_compute_nodes)*gv->computer_group_size+j);
  //   while((fp[j]==NULL) && (i<TRYNUM)){
  //     fp[j]=fopen(file_name,"rb");
  //     if(fp[j]==NULL){
  //       if(i==TRYNUM-1){
  //         perror("Error: ");
  //         printf("Warning: Analysis Process %d Reader %d open empty file j=%d, own=%d\n",
  //           gv->rank[0], lv->tid, j, (gv->rank[0]-gv->num_compute_nodes)*gv->computer_group_size+j);
  //         fflush(stdout);
  //       }

  //       i++;
  //       usleep(40000);
  //     }
  //   }
  // }


  while(1) {
    pthread_mutex_lock(&gv->lock_recv);
    if(gv->prefetch_counter>=gv->ana_total_blks){
      pthread_mutex_unlock(&gv->lock_recv);
      break;
    }

    if(gv->recv_tail>0){
      flag = 1;
      //printf("Prefetcher %d read recv_tail = %d\n", lv->tid, gv->recv_tail);
      last_gen_rank = gv->prefetch_id_array[gv->recv_tail-2]; // get a snapshot of which block has been generated
      last_blk_id = gv->prefetch_id_array[gv->recv_tail-1];
      // step = gv->recv_block_id_array[gv->recv_tail-4];
      // CI = gv->recv_block_id_array[gv->recv_tail-3];
      // CJ = gv->recv_block_id_array[gv->recv_tail-2];
      // CK = gv->recv_block_id_array[gv->recv_tail-1];
      //printf("Node %d Prefetcher %d get lock last_gen_rank = %d, step = %d, CI=%d, CJ=%d, CK=%d\n", gv->rank[0], lv->tid, last_gen_rank, step, CI, CJ, CK);
      gv->recv_tail-=2;
      //printf("Now, Node %d Prefetcher %d minus tail=%d\n", gv->rank[0],lv->tid,gv->recv_tail);
      gv->prefetch_counter++;
    }

    pthread_mutex_unlock(&gv->lock_recv);


    if(flag == 1){

      new_buffer = (char*) malloc(gv->analysis_data_len);

      *(int *)(new_buffer)=last_gen_rank;
      *(int *)(new_buffer+sizeof(int))=last_blk_id;
      // *(int *)(new_buffer+sizeof(int))=step;
      // *(int *)(new_buffer+sizeof(int)*2)=CI;
      // *(int *)(new_buffer+sizeof(int)*3)=CJ;
      // *(int *)(new_buffer+sizeof(int)*4)=CK;

      // if(mycount==0){
      //   // fopen file
      //   for(j=0;j<gv->computer_group_size;j++){
      //     i=0;
      //     sprintf(file_name, ADDRESS, gv->num_compute_nodes, gv->num_analysis_nodes,
      //       (gv->rank[0]-gv->num_compute_nodes)*gv->computer_group_size+j, (gv->rank[0]-gv->num_compute_nodes)*gv->computer_group_size+j);
      //     while((fp[j]==NULL) && (i<TRYNUM)){
      //       fp[j]=fopen(file_name,"rb");
      //       if(fp[j]==NULL){
      //         if(i==TRYNUM-1){
      //           perror("Error: ");
      //           printf("Warning: Analysis Process %d Reader %d open empty file j=%d, own=%d\n",
      //             gv->rank[0], lv->tid, j, (gv->rank[0]-gv->num_compute_nodes)*gv->computer_group_size+j);
      //           fflush(stdout);
      //         }

      //         i++;
      //         usleep(5000);
      //       }
      //     }
      //   }
      // }

      // fopen file
      for(j=0;j<gv->computer_group_size;j++){
        if(open[j]==0 && (last_gen_rank)==((gv->rank[0]-gv->num_compute_nodes)*gv->computer_group_size+j)){
          open[j]=1;
          // #define ADDRESS "/N/dc2/scratch/fuyuan/LBM/LBM%03dvs%03d/cid%03d/2lbm_cid%03d"
          sprintf(file_name,ADDRESS,gv->num_compute_nodes, gv->num_analysis_nodes,last_gen_rank, last_gen_rank);
          // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
          while((fp[last_gen_rank%gv->computer_group_size]==NULL) && (i<TRYNUM)){
            fp[last_gen_rank%gv->computer_group_size]=fopen(file_name,"rb");
            if(fp[last_gen_rank%gv->computer_group_size]==NULL){
              if(i==TRYNUM-1){
                perror("Error: ");
                printf("Warning: Analysis Process %d Reader %d read empty file last_gen_rank=%d, last_blk_id=%d\n",
                  gv->rank[0], lv->tid, last_gen_rank, last_blk_id);
                fflush(stdout);
              }

              i++;
              usleep(1000);
            }
          }
        }
      }

      t0 = get_cur_time();
      read_blk(gv, lv, new_buffer+sizeof(int)*2, gv->block_size,
        last_gen_rank, last_blk_id,
        fp[last_gen_rank%gv->computer_group_size]);    //read file block to buffer memory
      t1 = get_cur_time();
      lv->read_time += t1 - t0;
      // mycount++;

      t2 = get_cur_time();
      ring_buffer_put(gv,lv,new_buffer);
      t3 = get_cur_time();
      lv->ring_buffer_put_time += t3 -t2;

      flag = 0;
    }

  }

  for(j=0;j<gv->computer_group_size;j++)
    fclose(fp[j]);
}

void prefetch_thread(GV gv,LV lv){

  double t0=0, t1=0;

  // printf("Analysis Process %d reader thread %d is running!\n",gv->rank[0], lv->tid);
  t0 = get_cur_time();
  prefetch_read_fileblk(gv,lv);
  t1 = get_cur_time();

  //printf("prefetch %d wait_lock2 = %f\n", lv->tid, lv->wait_lock2);
  //printf("prefetch %d wait_lock3 = %f\n", lv->tid, lv->wait_lock3);
  printf("Analysis Process %d Reader %d io_read_time= %f, only_fread_time= %f, ring_buffer_put_time= %f, total_time= %f\n",
    gv->rank[0], lv->tid, lv->read_time, lv->only_fread_time,
    lv->ring_buffer_put_time, t1-t0);

}
