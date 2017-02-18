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

// int mark(GV gv, double* buffer, int nbytes, int block_id){
//   //*((long int *)&(buffer[nbytes-8]))=block_id;
//   int step, new_id;
//   *((double *)(buffer+nbytes-1))=block_id;
//   step = *((double *)(buffer+nbytes-2));
//   new_id = (step-1)*gv->X*gv->Y*gv->Z+gv->rank[0]*gv->X*gv->Y*gv->Z+block_id;
//   return new_id;
// }

void write_blk(GV gv, LV lv, int block_id, char* buffer, int nbytes, FILE *fp){

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
  int block_id=0,my_count=0;
  char file_name[128];
  FILE *fp=NULL;
  int i=0;
  // int step=0,CI=0,CJ=0,CK=0;
  //int new_id;
  double t0=0,t1=0,t2=0,t3=0, t4=0, t5=0,wait_lock=0;
  char* pointer=NULL;
  // printf("Compute Process %d writer thread %d is running!\n",gv->rank[0], lv->tid);

  t2 = get_cur_time();

  while(1){

      if(my_count>=gv->cpt_total_blks) break;

      pointer = producer_ring_buffer_get(gv,lv);
      if (pointer == NULL) {
        pthread_exit(NULL);
      }
      else{
        //check_blk(gv, pointer,gv->block_size,block_id);
        //new_id =mark(gv, pointer,gv->block_size, block_id);
        //printf("Node %d prepare to write %d block\n", gv->rank[0], new_id);

        block_id = *((int*)pointer);
        // step=*(int *)(pointer+sizeof(int));
        // CI=*(int *)(pointer+sizeof(int)*2);
        // CJ=*(int *)(pointer+sizeof(int)*3);
        // CK=*(int *)(pointer+sizeof(int)*4);

        // printf("Compute Process %d producer %d get block_id%d\n",
        //   gv->rank[0], lv->tid, block_id);

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
        write_blk(gv, lv, block_id, pointer+sizeof(int), gv->block_size, fp);
        t1 = get_cur_time();
        lv->write_time += t1 - t0;
        my_count++;

        t4 = get_cur_time();
        pthread_mutex_lock(&gv->lock_producer_progress);
        //check pointer bound
        gv->written_id_array[gv->send_tail]=block_id;
        gv->send_tail++;
        pthread_mutex_unlock(&gv->lock_producer_progress);
        t5 = get_cur_time();
        wait_lock += t5-t4;

        free(pointer);
      }

  }
  fclose(fp);
  t3 = get_cur_time();

  printf("Compute Process %d Producer %d write_time = %f, Write_Time/Block=%f ms, only_fwrite_time/block=%f ms, wait_lock = %f, total time is %f\n",
    gv->rank[0], lv->tid, lv->write_time, lv->write_time/gv->cpt_total_blks*1000, lv->only_fwrite_time/gv->cpt_total_blks*1000, wait_lock, t3-t2);
}
