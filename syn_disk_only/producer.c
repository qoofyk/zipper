/********************************************************
Copyright YUANKUN FU
Brief desc of the file: Producer thread, get blk from producer_ring_buffer, write_blk
********************************************************/
#include "do_thread.h"

// void check_blk(GV gv, char* buffer, long int nbytes, long int block_id){
//   long int i;
//   int j;
//   register int x;
//   register double y;

//   //printf("start calc!\n");
//   //printf("before calc data is %d\n",buffer[0]);

//   for(j=0;j<gv->lp;j++){
//     for(i = 0; i < (nbytes/4); i=i+4) {
//         x = *(int *)(buffer+i);
//         if(x!=0x05040302){
//           printf("x = %x block_id = %ld\n", x, block_id);
//           printf("PRO Checking!!! Exp2 %ld\n", i);
//           printf("PRO Checking!!! Exp2 consumer_count = %ld Wrong!\n", gv->calc_counter);
//         }
//         y = sqrt((double)x);
//     }
//   }
// }

char* producer_ring_buffer_get(GV gv,LV lv){
  char* pointer;
  ring_buffer *rb;

  rb = (ring_buffer *) gv->producer_rb_p;

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

void mark(char* buffer, int nbytes, int block_id){
  //*((long int *)&(buffer[nbytes-8]))=block_id;
  *((int *)(buffer+nbytes-8))=block_id;
}

void write_blk(GV gv, LV lv, int blk_id, char* buffer, int nbytes, FILE *fp){
  double t0=0,t1=0;
  int error=0;

  t0 = get_cur_time();
  error=fwrite(buffer, nbytes, 1, fp);
  if(error==0)
      perror("Write error:");
  t1 = get_cur_time();
  lv->only_fwrite_time += t1 - t0;

}

void producer_thread(GV gv,LV lv){
  int block_id=0;
  double t0=0,t1=0,t2=0,t3=0, t4=0, t5=0,wait_lock=0;
  char* pointer=NULL;
  char file_name[128];
  FILE *fp=NULL;
  int i=0,my_count=0;
  // printf("Compute Node%d producer thread %d is running!\n",gv->rank[0],lv->tid);

  t2 = get_cur_time();

  while(1){
      pthread_mutex_lock(&gv->lock_blk_id);
      block_id = gv->id_counter++;
      pthread_mutex_unlock(&gv->lock_blk_id);

      if(block_id>=gv->total_blks) break;

      pointer = producer_ring_buffer_get(gv,lv);
      if (pointer == NULL) {
        pthread_exit(NULL);
      }
      else{
        //check_blk(gv, pointer,gv->block_size,block_id);
        mark(pointer,gv->block_size, block_id);

        // fopen a file one time
        if(block_id==0){
          // "/N/dc2/scratch/fuyuan/LBM/LBM%03dvs%03d/cid%03d/2lbm_cid%03dblk%d.d"
          sprintf(file_name,ADDRESS,gv->num_compute_nodes, gv->num_analysis_nodes, gv->rank[0], gv->rank[0]);
          // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
          while((fp==NULL) && (i<TRYNUM)){
            fp=fopen(file_name,"wb");
            if(fp==NULL){
              if(i==TRYNUM-1){
                perror("Error: ");
                printf("Warning: Compute Process %d Writer %d write an empty file last_gen_rank=%d, block_id=%d\n",
                    gv->rank[0], lv->tid, gv->rank[0], block_id);
                fflush(stdout);
              }

              i++;
              usleep(5000);
            }
          }
        }

        t0 = get_cur_time();
        write_blk(gv, lv, block_id,pointer,gv->block_size,fp);
        t1 = get_cur_time();
        lv->write_time += t1 - t0;
        my_count++;

        t4 = get_cur_time();
        pthread_mutex_lock(&gv->lock_producer_progress);
        gv->written_id_array[gv->send_tail]=block_id;
        gv->send_tail++;
        gv->progress_counter++;
        pthread_mutex_unlock(&gv->lock_producer_progress);
        t5 = get_cur_time();
        wait_lock += t5-t4;

        free(pointer);
      }

  }
  if(my_count>0)
    fclose(fp);

  t3 = get_cur_time();

  printf("Compute Node %d Producer %d write_time = %f, Write_Time/Block=%f, wait_lock = %f, total time is %f\n",
    gv->rank[0], lv->tid, lv->write_time, lv->write_time/gv->total_blks, wait_lock, t3-t2);
}
