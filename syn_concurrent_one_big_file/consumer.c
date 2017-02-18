/********************************************************
Copyright YUANKUN FU
Brief desc of the file: consumer thread
********************************************************/
#include "concurrent.h"
void simple_verify(GV gv, LV lv, char* buffer,  int nbytes, int source, int block_id){
  int i,j;
  double sum=0,tmp=0;
  register int x;
  // int check = *(int *)(buffer);

  //printf("start calc!\n");
  //printf("before calc data is %d\n",buffer[0]);
  //gv->computeid=*(int *)(buffer);
  // printf("calc=%d\n", (nbytes/4-2)*gv->lp);

  for(i = 0; i < (nbytes/4-2); i=i+4) {
    for(j=0;j<gv->lp;j++){
      x = *(int *)(buffer+i);
      // if(x!=block_id+i){
      //   printf("Synthetic concurrent %d Wrong!\n", i);
      //   printf("x = %d\n", x);
      //   printf("Synthetic concurrent consumer_count = %d Wrong!\n", block_id);
      //   fflush(stdout);
      //   return;
      // }
      tmp = sqrt((double)x);
   }
   sum = sum + tmp;
  }

}

// void read_mark_remove(GV gv, char* pointer,  int nbytes){
//   char file_name[64];
//   //FILE *fp;
//   int blk_id;
//   //int computeid=0;

//   //computeid=*(int *)(pointer);
//   blk_id = *((int *)(pointer+nbytes-8));
//   sprintf(file_name,ADDRESS,gv->num_compute_nodes, gv->num_analysis_nodes,gv->computeid,gv->computeid,blk_id);
//   // sprintf(file_name,"/var/tmp/exp2_file_blk%d.data",blk_id);
//   // if(remove(file_name)== -1){
//   //   printf("Consumer cannot remove file %d!\n",blk_id);
//   // }
//   remove(file_name);
//   //printf("Consumer remove file %d Done!\n",blk_id);
// }


//Consumer
char* ring_buffer_get(GV gv,LV lv){
  char* pointer;

  ring_buffer *rb;

  rb = (ring_buffer *) gv->consumer_rb_p;

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

void consumer_thread(GV gv,LV lv){
  double t0=0, t1=0,t2=0,t3=0,t6=0,t7=0;
  char* pointer=NULL;
  double wait_time=0;
  int* temp_int_pointer;
  int source, block_id;
  // printf("Node %d consumer thread %d is running!\n",gv->rank[0], lv->tid);

  t2 = get_cur_time();
  while(1) {

    t6 = get_cur_time();
    pointer = ring_buffer_get(gv,lv);
    t7 = get_cur_time();
    wait_time += t7-t6;

    if (pointer == NULL) {
      pthread_exit(NULL);
    }
    else{
      temp_int_pointer=(int*)pointer;
      source = temp_int_pointer[0];
      block_id = temp_int_pointer[1];

      t0 = get_cur_time();
      simple_verify(gv, lv, pointer+sizeof(int)*2, gv->block_size, source, block_id);
      t1 = get_cur_time();
      lv->calc_time += t1 - t0;

      // t4 = get_cur_time();
      // read_mark_remove(gv, pointer, gv->block_size);
      // t5 = get_cur_time();
      // lv->remove_time += t5 - t4;

      gv->calc_counter++;

      // if(gv->calc_counter%10000==0)
      //   printf("Analysis Process %d Consumer %d calc_counter %d\n", gv->rank[0], lv->tid, gv->calc_counter);

      free(pointer);
    }

    if(gv->calc_counter >= gv->ana_total_blks) break;
  }
  t3 = get_cur_time();

  printf("Node %d ####Exp2 consumer total ####total consumer time= %f,calc_time= %f, wait_time= %f, calc_counter=%d\n",
   gv->rank[0], t3-t2, lv->calc_time, wait_time, gv->calc_counter);
  fflush(stdout);
}
